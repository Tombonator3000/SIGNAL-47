#include "S47EveningPack.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "../../../../Narrative/EveningShift/generated/EveningPackData.inl"

namespace
{
	TArray<FString> ReadStrings(const TArray<TSharedPtr<FJsonValue>>& Values)
	{
		TArray<FString> Result;
		for (const auto& Value : Values) Result.Add(Value->AsString());
		return Result;
	}
	FS47EveningRequirements ReadRequirements(const TSharedPtr<FJsonObject>& Json)
	{
		FS47EveningRequirements Result;
		Result.All = ReadStrings(Json->GetArrayField(TEXT("all")));
		Result.None = ReadStrings(Json->GetArrayField(TEXT("none")));
		return Result;
	}
	FS47EveningEffects ReadEffects(const TSharedPtr<FJsonObject>& Json)
	{
		FS47EveningEffects Result;
		for (const auto& Pair : Json->GetObjectField(TEXT("set_flags"))->Values)
			Result.Flags.Add(Pair.Key, Pair.Value->AsBool());
		Result.Documents = ReadStrings(Json->GetArrayField(TEXT("give_documents")));
		return Result;
	}
	TArray<TSharedPtr<FJsonValue>> WriteStrings(const TArray<FString>& Values)
	{
		TArray<TSharedPtr<FJsonValue>> Result;
		for (const auto& Value : Values) Result.Add(MakeShared<FJsonValueString>(Value));
		return Result;
	}
}

bool FS47EveningPack::Initialize(FString& Error)
{
	Error.Reset();
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S47EveningPackJson), Root) || !Root.IsValid())
	{
		Error = TEXT("Evening pack JSON could not be decoded");
		return false;
	}
	Scenes.Reset(); DocumentData.Reset(); Strings.Reset(); Aliases.Reset(); Defaults.Reset(); External.Reset();
	const auto Pack = Root->GetObjectField(TEXT("pack"));
	Phases = ReadStrings(Pack->GetArrayField(TEXT("phases")));
	const auto Actors = Pack->GetObjectField(TEXT("actors"));
	for (const auto& Pair : Pack->GetObjectField(TEXT("flags"))->Values)
		Defaults.Add(Pair.Key, Pair.Value->AsObject()->GetBoolField(TEXT("default")));
	for (const auto& Pair : Pack->GetObjectField(TEXT("external_flags"))->Values) External.Add(Pair.Key, false);
	for (const auto& Pair : Pack->GetObjectField(TEXT("recording_aliases"))->Values) Aliases.Add(Pair.Key, Pair.Value->AsString());
	for (const auto& Value : Root->GetArrayField(TEXT("modules")))
	{
		const auto Module = Value->AsObject();
		for (const auto& Pair : Module->GetObjectField(TEXT("flags"))->Values)
			Defaults.Add(Pair.Key, Pair.Value->AsObject()->GetBoolField(TEXT("default")));
		for (const auto& SceneValue : Module->GetArrayField(TEXT("scenes")))
		{
			const auto Json = SceneValue->AsObject();
			FS47EveningScene Scene;
			Scene.Id = Json->GetStringField(TEXT("id"));
			Scene.Phase = Json->GetStringField(TEXT("phase"));
			Scene.Channel = Json->GetStringField(TEXT("channel"));
			Scene.Mode = Json->GetStringField(TEXT("mode"));
			Scene.Entry = Json->GetStringField(TEXT("entry"));
			Scene.Requires = ReadRequirements(Json->GetObjectField(TEXT("requires")));
			Scene.OnComplete = ReadEffects(Json->GetObjectField(TEXT("on_complete")));
			for (const auto& LineValue : Json->GetArrayField(TEXT("lines")))
			{
				const auto LineJson = LineValue->AsObject();
				FS47EveningLine Line;
				Line.Id = LineJson->GetStringField(TEXT("id"));
				Line.Speaker = LineJson->GetStringField(TEXT("speaker"));
				Line.Text = LineJson->GetStringField(TEXT("text"));
				if (!Actors->HasTypedField<EJson::Object>(Line.Speaker))
				{
					Error = TEXT("Unknown evening actor: ") + Line.Speaker;
					return false;
				}
				const auto Actor = Actors->GetObjectField(Line.Speaker);
				Line.bRecord = Actor->GetBoolField(TEXT("record"));
				Actor->TryGetStringField(TEXT("voice_profile"), Line.VoiceProfile);
				if (!ReadStrings(Actor->GetArrayField(TEXT("allowed_channels"))).Contains(Scene.Channel))
				{
					Error = TEXT("Forbidden evening actor/channel: ") + Line.Speaker + TEXT("/") + Scene.Channel;
					return false;
				}
				Line.RecordingId = RecordingId(Line.Id);
				Scene.Lines.Add(MoveTemp(Line));
			}
			for (const auto& ChoiceValue : Json->GetArrayField(TEXT("choices")))
			{
				const auto ChoiceJson = ChoiceValue->AsObject();
				FS47EveningChoice Choice;
				Choice.Id = ChoiceJson->GetStringField(TEXT("id"));
				Choice.Label = ChoiceJson->GetStringField(TEXT("label"));
				Choice.Next = ChoiceJson->GetStringField(TEXT("next"));
				Choice.Requires = ReadRequirements(ChoiceJson->GetObjectField(TEXT("requires")));
				Choice.OnSelect = ReadEffects(ChoiceJson->GetObjectField(TEXT("on_select")));
				Scene.Choices.Add(MoveTemp(Choice));
			}
			const FString Id = Scene.Id;
			Scenes.Add(Id, MoveTemp(Scene));
		}
		for (const auto& DocumentValue : Module->GetArrayField(TEXT("documents")))
		{
			const auto Json = DocumentValue->AsObject();
			FS47EveningDocument Doc;
			Doc.Id = Json->GetStringField(TEXT("id")); Doc.Title = Json->GetStringField(TEXT("title"));
			Doc.Kind = Json->GetStringField(TEXT("kind")); Doc.Stamp = Json->GetStringField(TEXT("stamp"));
			Doc.Body = ReadStrings(Json->GetArrayField(TEXT("body")));
			const FString Id = Doc.Id;
			DocumentData.Add(Id, MoveTemp(Doc));
		}
		for (const auto& StringValue : Module->GetArrayField(TEXT("strings")))
		{
			const auto Json = StringValue->AsObject();
			Strings.Add(Json->GetStringField(TEXT("id")), Json->GetStringField(TEXT("text")));
		}
	}
	// Fail closed if authored effects ever attempt to write a world flag.
	const auto ValidEffects = [this](const FS47EveningEffects& Effects)
	{
		for (const auto& Pair : Effects.Flags) if (!Defaults.Contains(Pair.Key) || External.Contains(Pair.Key)) return false;
		for (const auto& Id : Effects.Documents) if (!DocumentData.Contains(Id)) return false;
		return true;
	};
	for (const auto& Pair : Scenes)
	{
		const auto& Scene = Pair.Value;
		if (!Phases.Contains(Scene.Phase) || !ValidEffects(Scene.OnComplete) || (Scene.Mode == TEXT("fixed") && !Scene.Choices.IsEmpty()))
		{
			Error = TEXT("Invalid evening scene: ") + Scene.Id; return false;
		}
		for (const auto& Choice : Scene.Choices)
		{
			const auto* Target = Scenes.Find(Choice.Next);
			if (!Target || Target->Entry != TEXT("choice") || Target->Phase != Scene.Phase || !ValidEffects(Choice.OnSelect))
			{
				Error = TEXT("Invalid evening choice: ") + Choice.Id; return false;
			}
		}
	}
	for (const auto& Pair : Aliases)
	{
		const FS47EveningLine* Alias = nullptr;
		const FS47EveningLine* Source = nullptr;
		for (const auto& Scene : Scenes) for (const auto& Line : Scene.Value.Lines)
		{
			if (Line.Id == Pair.Key) Alias = &Line;
			if (Line.Id == Pair.Value) Source = &Line;
		}
		if (!Alias || !Source || Aliases.Contains(Pair.Value) || !Alias->bRecord || !Source->bRecord ||
			Alias->Speaker != Source->Speaker || Alias->VoiceProfile != Source->VoiceProfile || Alias->Text != Source->Text)
		{
			Error = TEXT("Invalid evening recording alias: ") + Pair.Key; return false;
		}
	}
	Reset();
	return true;
}

void FS47EveningPack::Reset()
{
	Flags = Defaults;
	for (auto& Pair : External) Pair.Value = false;
	Phase = TEXT("commute"); ActiveScene.Reset(); PendingChoice.Reset(); Cursor = 0; OwnedDocuments.Reset();
}

bool FS47EveningPack::SetPhase(const FString& Value)
{
	if (!ActiveScene.IsEmpty() || !Phases.Contains(Value)) return false;
	Phase = Value;
	return true;
}

bool FS47EveningPack::SetExternal(const FString& Id, bool bValue)
{
	bool* Value = External.Find(Id);
	if (!Value) return false;
	*Value = bValue;
	return true;
}

bool FS47EveningPack::Flag(const FString& Id) const
{
	const bool* Value = External.Find(Id);
	return Value ? *Value : Flags.FindRef(Id);
}

bool FS47EveningPack::Meets(const FS47EveningRequirements& Requires, const TMap<FString, bool>& Internal) const
{
	const auto Read = [this, &Internal](const FString& Id)
	{
		const bool* Value = External.Find(Id);
		return Value ? *Value : Internal.FindRef(Id);
	};
	for (const auto& Id : Requires.All) if (!Read(Id)) return false;
	for (const auto& Id : Requires.None) if (Read(Id)) return false;
	return true;
}

bool FS47EveningPack::CanStart(const FString& Id) const
{
	const auto* Scene = Scenes.Find(Id);
	return ActiveScene.IsEmpty() && Scene && Scene->Entry == TEXT("trigger") && Scene->Phase == Phase && Meets(Scene->Requires, Flags);
}

bool FS47EveningPack::Start(const FString& Id)
{
	if (!CanStart(Id)) return false;
	ActiveScene = Id; Cursor = 0; PendingChoice.Reset();
	if (CurrentScene()->Lines.IsEmpty() && CurrentScene()->Choices.IsEmpty()) CompleteLeaf();
	return true;
}

const FS47EveningScene* FS47EveningPack::CurrentScene() const { return Scenes.Find(ActiveScene); }

const FS47EveningLine* FS47EveningPack::CurrentLine() const
{
	const auto* Scene = CurrentScene();
	return Scene && Scene->Lines.IsValidIndex(Cursor) ? &Scene->Lines[Cursor] : nullptr;
}

void FS47EveningPack::Apply(const FS47EveningEffects& Effects)
{
	for (const auto& Pair : Effects.Flags) if (Defaults.Contains(Pair.Key) && !External.Contains(Pair.Key)) Flags.Add(Pair.Key, Pair.Value);
	for (const auto& Id : Effects.Documents) if (DocumentData.Contains(Id)) OwnedDocuments.AddUnique(Id);
}

void FS47EveningPack::CompleteLeaf()
{
	const auto* Scene = CurrentScene();
	if (!Scene || !Scene->Choices.IsEmpty()) return;
	Apply(Scene->OnComplete);
	ActiveScene.Reset(); PendingChoice.Reset(); Cursor = 0;
}

bool FS47EveningPack::Advance()
{
	const auto* Scene = CurrentScene();
	if (!Scene || Cursor >= Scene->Lines.Num()) return false;
	++Cursor;
	PendingChoice.Reset();
	if (Cursor == Scene->Lines.Num() && Scene->Choices.IsEmpty()) CompleteLeaf();
	return true;
}

bool FS47EveningPack::Eligible(const FS47EveningChoice& Choice) const
{
	if (!Meets(Choice.Requires, Flags)) return false;
	const auto* Target = Scenes.Find(Choice.Next);
	if (!Target || Target->Entry != TEXT("choice") || Target->Phase != Phase) return false;
	// Food response requirements deliberately depend on the purchase in on_select.
	auto Prospective = Flags;
	for (const auto& Pair : Choice.OnSelect.Flags)
	{
		if (!Defaults.Contains(Pair.Key) || External.Contains(Pair.Key)) return false;
		Prospective.Add(Pair.Key, Pair.Value);
	}
	return Meets(Target->Requires, Prospective);
}

TArray<const FS47EveningChoice*> FS47EveningPack::Choices() const
{
	TArray<const FS47EveningChoice*> Result;
	const auto* Scene = CurrentScene();
	if (Scene && Cursor == Scene->Lines.Num())
		for (const auto& Choice : Scene->Choices) if (Eligible(Choice)) Result.Add(&Choice);
	return Result;
}

bool FS47EveningPack::Select(const FString& Id)
{
	for (const auto* Choice : Choices()) if (Choice->Id == Id)
	{
		// No callbacks between payment and storing the response. Save observes both.
		const FString Target = Choice->Next;
		Apply(Choice->OnSelect);
		PendingChoice = Id; ActiveScene = Target; Cursor = 0;
		if (CurrentScene()->Lines.IsEmpty() && CurrentScene()->Choices.IsEmpty()) CompleteLeaf();
		return true;
	}
	return false;
}

FString FS47EveningPack::RecordingId(const FString& Cue) const
{
	const FString* Alias = Aliases.Find(Cue);
	return Alias ? *Alias : Cue;
}

TSharedPtr<FJsonObject> FS47EveningPack::Save() const
{
	const auto Json = MakeShared<FJsonObject>();
	Json->SetNumberField(TEXT("version"), 1);
	Json->SetStringField(TEXT("pack_id"), TEXT("signal47_evening_shift"));
	Json->SetStringField(TEXT("phase"), Phase);
	Json->SetStringField(TEXT("active_scene"), ActiveScene);
	Json->SetNumberField(TEXT("next_line"), Cursor);
	Json->SetStringField(TEXT("pending_choice"), PendingChoice);
	const auto SavedFlags = MakeShared<FJsonObject>();
	for (const auto& Pair : Flags) SavedFlags->SetBoolField(Pair.Key, Pair.Value);
	Json->SetObjectField(TEXT("flags"), SavedFlags);
	Json->SetArrayField(TEXT("documents"), WriteStrings(OwnedDocuments));
	// External context is rebuilt from the host's actual saved chapter/world state.
	return Json;
}

bool FS47EveningPack::Load(const TSharedPtr<FJsonObject>& Json, FString& Error)
{
	Error = TEXT("Invalid evening save");
	if (!Json.IsValid() || Scenes.IsEmpty()) return false;
	double Version = 0, SavedCursor = 0;
	FString PackId, SavedPhase, SavedScene, SavedChoice;
	const TSharedPtr<FJsonObject>* SavedFlags = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* SavedDocs = nullptr;
	if (!Json->TryGetNumberField(TEXT("version"), Version) || Version != 1 ||
		!Json->TryGetStringField(TEXT("pack_id"), PackId) || PackId != TEXT("signal47_evening_shift") ||
		!Json->TryGetStringField(TEXT("phase"), SavedPhase) || !Phases.Contains(SavedPhase) ||
		!Json->TryGetStringField(TEXT("active_scene"), SavedScene) ||
		!Json->TryGetStringField(TEXT("pending_choice"), SavedChoice) ||
		!Json->TryGetNumberField(TEXT("next_line"), SavedCursor) ||
		!Json->TryGetObjectField(TEXT("flags"), SavedFlags) ||
		!Json->TryGetArrayField(TEXT("documents"), SavedDocs)) return false;
	const auto* Scene = Scenes.Find(SavedScene);
	if (!FMath::IsFinite(SavedCursor) || SavedCursor < 0 || SavedCursor > MAX_int32 || SavedCursor != FMath::FloorToDouble(SavedCursor)) return false;
	if (SavedScene.IsEmpty())
	{
		if (SavedCursor != 0 || !SavedChoice.IsEmpty()) return false;
	}
	else if (!Scene || Scene->Phase != SavedPhase || SavedCursor > Scene->Lines.Num() ||
		(SavedCursor == Scene->Lines.Num() && Scene->Choices.IsEmpty())) return false;
	if (!SavedChoice.IsEmpty())
	{
		bool bFound = false;
		for (const auto& Pair : Scenes) for (const auto& Choice : Pair.Value.Choices)
			if (Choice.Id == SavedChoice && Choice.Next == SavedScene && SavedCursor == 0) bFound = true;
		if (!bFound) return false;
	}
	auto NewFlags = Defaults;
	for (const auto& Pair : (*SavedFlags)->Values)
	{
		bool bValue = false;
		if (!Defaults.Contains(Pair.Key) || External.Contains(Pair.Key) || Pair.Value->Type != EJson::Boolean || !Pair.Value->TryGetBool(bValue)) return false;
		NewFlags.Add(Pair.Key, bValue);
	}
	if (Scene)
	{
		// Context is restored by the host after Load; internal preconditions still
		// protect against e.g. a pending burger response with its payment missing.
		for (const auto& Id : Scene->Requires.All) if (!External.Contains(Id) && !NewFlags.FindRef(Id)) return false;
		for (const auto& Id : Scene->Requires.None) if (!External.Contains(Id) && NewFlags.FindRef(Id)) return false;
	}
	TArray<FString> NewDocs;
	for (const auto& Value : *SavedDocs)
	{
		FString Id;
		if (!Value->TryGetString(Id) || !DocumentData.Contains(Id)) return false;
		NewDocs.AddUnique(Id);
	}
	// Commit only after the entire snapshot has passed validation.
	Flags = MoveTemp(NewFlags); OwnedDocuments = MoveTemp(NewDocs);
	Phase = SavedPhase; ActiveScene = SavedScene; PendingChoice = SavedChoice; Cursor = static_cast<int32>(SavedCursor);
	for (auto& Pair : External) Pair.Value = false;
	Error.Reset();
	return true;
}

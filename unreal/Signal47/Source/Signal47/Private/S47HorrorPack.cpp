#include "S47HorrorPack.h"
#include "S47Json.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "../../../../Narrative/HorrorStory/generated/HorrorPackData.inl"

namespace
{
	TArray<FString> HorrorReadStrings(const TArray<TSharedPtr<FJsonValue>>& Values)
	{
		TArray<FString> Result;
		for (const auto& Value : Values) Result.Add(Value->AsString());
		return Result;
	}
	FS47EveningRequirements HorrorReadRequirements(const TSharedPtr<FJsonObject>& Json)
	{
		FS47EveningRequirements Result;
		Result.All = HorrorReadStrings(Json->GetArrayField(TEXT("all")));
		Result.None = HorrorReadStrings(Json->GetArrayField(TEXT("none")));
		return Result;
	}
	FS47EveningEffects HorrorReadEffects(const TSharedPtr<FJsonObject>& Json)
	{
		FS47EveningEffects Result;
		for (const auto& Pair : Json->GetObjectField(TEXT("set_flags"))->Values)
			Result.Flags.Add(FString(*Pair.Key), Pair.Value->AsBool());
		Result.Documents = HorrorReadStrings(Json->GetArrayField(TEXT("give_documents")));
		return Result;
	}
	TArray<TSharedPtr<FJsonValue>> HorrorWriteStrings(const TArray<FString>& Values)
	{
		TArray<TSharedPtr<FJsonValue>> Result;
		for (const auto& Value : Values) Result.Add(MakeShared<FJsonValueString>(Value));
		return Result;
	}
}

bool FS47HorrorPack::Initialize(FString& Error)
{
	Error.Reset();
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S47HorrorPackJson), Root) || !Root.IsValid())
	{
		Error = TEXT("Horror pack JSON could not be decoded");
		return false;
	}
	Scenes.Reset(); DocumentData.Reset(); Strings.Reset(); Aliases.Reset(); Defaults.Reset(); External.Reset();
	Registry.Reset(); ActorNames.Reset(); AuthoredSceneIds.Reset();
	const auto Pack = Root->GetObjectField(TEXT("pack"));
	if (Pack->GetStringField(TEXT("pack_id")) != TEXT("signal47_horror_story_h1"))
	{ Error = TEXT("Unexpected horror pack identity"); return false; }
	Phases = HorrorReadStrings(Pack->GetArrayField(TEXT("phases")));
	const auto Actors = Pack->GetObjectField(TEXT("actors"));
	for (const auto& Actor : Actors->Values)
		ActorNames.Add(FString(*Actor.Key), Actor.Value->AsObject()->GetStringField(TEXT("display_name")));
	const TSharedPtr<FJsonObject>* Takes = nullptr;
	Pack->TryGetObjectField(TEXT("_takes"), Takes);
	for (const auto& Pair : Pack->GetObjectField(TEXT("flags"))->Values)
		Defaults.Add(FString(*Pair.Key), Pair.Value->AsObject()->GetBoolField(TEXT("default")));
	for (const auto& Pair : Pack->GetObjectField(TEXT("external_flags"))->Values) External.Add(FString(*Pair.Key), false);
	for (const auto& Pair : Pack->GetObjectField(TEXT("recording_aliases"))->Values) Aliases.Add(FString(*Pair.Key), Pair.Value->AsString());
	for (const auto& Value : Root->GetArrayField(TEXT("modules")))
	{
		const auto Module = Value->AsObject();
		for (const auto& Pair : Module->GetObjectField(TEXT("flags"))->Values)
			Defaults.Add(FString(*Pair.Key), Pair.Value->AsObject()->GetBoolField(TEXT("default")));
		for (const auto& SceneValue : Module->GetArrayField(TEXT("scenes")))
		{
			const auto Json = SceneValue->AsObject();
			FS47EveningScene Scene;
			Scene.Id = Json->GetStringField(TEXT("id"));
			Scene.Phase = Json->GetStringField(TEXT("phase"));
			Scene.Channel = Json->GetStringField(TEXT("channel"));
			Scene.Mode = Json->GetStringField(TEXT("mode"));
			Scene.Entry = Json->GetStringField(TEXT("entry"));
			Scene.Requires = HorrorReadRequirements(Json->GetObjectField(TEXT("requires")));
			Scene.OnComplete = HorrorReadEffects(Json->GetObjectField(TEXT("on_complete")));
			for (const auto& LineValue : Json->GetArrayField(TEXT("lines")))
			{
				const auto LineJson = LineValue->AsObject();
				FS47EveningLine Line;
				Line.Id = LineJson->GetStringField(TEXT("id"));
				Line.Speaker = LineJson->GetStringField(TEXT("speaker"));
				Line.Text = LineJson->GetStringField(TEXT("text"));
				if (!Actors->HasTypedField<EJson::Object>(Line.Speaker))
				{
					Error = TEXT("Unknown horror actor: ") + Line.Speaker;
					return false;
				}
				const auto Actor = Actors->GetObjectField(Line.Speaker);
				Line.bRecord = Actor->GetBoolField(TEXT("record"));
				Actor->TryGetStringField(TEXT("voice_profile"), Line.VoiceProfile);
				if (!HorrorReadStrings(Actor->GetArrayField(TEXT("allowed_channels"))).Contains(Scene.Channel))
				{
					Error = TEXT("Forbidden horror actor/channel: ") + Line.Speaker + TEXT("/") + Scene.Channel;
					return false;
				}
				Line.RecordingId = RecordingId(Line.Id);
				const TSharedPtr<FJsonObject>* Take = nullptr;
				if (Line.bRecord && Takes && (*Takes)->TryGetObjectField(Line.RecordingId, Take))
				{
					(*Take)->TryGetNumberField(TEXT("duration_seconds"), Line.Duration);
					(*Take)->TryGetNumberField(TEXT("speech_start_seconds"), Line.SpeechStart);
					(*Take)->TryGetNumberField(TEXT("speech_end_seconds"), Line.SpeechEnd);
					if (!FMath::IsFinite(Line.Duration) || !FMath::IsFinite(Line.SpeechStart) || !FMath::IsFinite(Line.SpeechEnd) ||
						Line.Duration <= 0 || Line.SpeechStart < 0 || Line.SpeechEnd <= Line.SpeechStart || Line.SpeechEnd > Line.Duration)
					{ Error = TEXT("Invalid measured horror timing: ") + Line.Id; return false; }
				}
				Scene.Lines.Add(MoveTemp(Line));
			}
			for (const auto& ChoiceValue : Json->GetArrayField(TEXT("choices")))
			{
				const auto ChoiceJson = ChoiceValue->AsObject();
				FS47EveningChoice Choice;
				Choice.Id = ChoiceJson->GetStringField(TEXT("id"));
				Choice.Label = ChoiceJson->GetStringField(TEXT("label"));
				Choice.Next = ChoiceJson->GetStringField(TEXT("next"));
				Choice.Requires = HorrorReadRequirements(ChoiceJson->GetObjectField(TEXT("requires")));
				Choice.OnSelect = HorrorReadEffects(ChoiceJson->GetObjectField(TEXT("on_select")));
				Scene.Choices.Add(MoveTemp(Choice));
			}
			const FString Id = Scene.Id;
			if (Scenes.Contains(Id)) { Error = TEXT("Duplicate horror scene: ") + Id; return false; }
			AuthoredSceneIds.Add(Id);
			Scenes.Add(Id, MoveTemp(Scene));
		}
		for (const auto& DocumentValue : Module->GetArrayField(TEXT("documents")))
		{
			const auto Json = DocumentValue->AsObject();
			FS47EveningDocument Doc;
			Doc.Id = Json->GetStringField(TEXT("id")); Doc.Title = Json->GetStringField(TEXT("title"));
			Doc.Kind = Json->GetStringField(TEXT("kind")); Doc.Stamp = Json->GetStringField(TEXT("stamp"));
			Doc.Body = HorrorReadStrings(Json->GetArrayField(TEXT("body")));
			const FString Id = Doc.Id;
			DocumentData.Add(Id, MoveTemp(Doc));
		}
		for (const auto& StringValue : Module->GetArrayField(TEXT("strings")))
		{
			const auto Json = StringValue->AsObject();
			Strings.Add(Json->GetStringField(TEXT("id")), Json->GetStringField(TEXT("text")));
		}
	}
	const auto RegistryJson = Root->GetObjectField(TEXT("registry"));
	if (RegistryJson->GetNumberField(TEXT("schema_version")) != 1 ||
		RegistryJson->GetStringField(TEXT("pack_id")) != TEXT("signal47_horror_story_h1"))
	{ Error = TEXT("Invalid horror scene registry"); return false; }
	for (const auto& Value : RegistryJson->GetArrayField(TEXT("scenes")))
	{
		const auto Json = Value->AsObject();
		FS47HorrorSceneRegistration Registration;
		Registration.Id = Json->GetStringField(TEXT("id"));
		Registration.ScenarioIds = HorrorReadStrings(Json->GetArrayField(TEXT("scenario_ids")));
		Registration.MissionIds = HorrorReadStrings(Json->GetArrayField(TEXT("mission_ids")));
		const auto* Scene = Scenes.Find(Registration.Id);
		if (!Scene || Registry.Contains(Registration.Id) || Registration.ScenarioIds.IsEmpty() ||
			Registration.MissionIds.IsEmpty() || Json->GetStringField(TEXT("phase")) != Scene->Phase)
		{ Error = TEXT("Invalid horror scene registration: ") + Registration.Id; return false; }
		const FString Id = Registration.Id;
		Registry.Add(Id, MoveTemp(Registration));
	}
	if (Registry.Num() != Scenes.Num())
	{ Error = TEXT("Horror registry omits authored scenes"); return false; }
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
			Error = TEXT("Invalid horror scene: ") + Scene.Id; return false;
		}
		for (const auto& Choice : Scene.Choices)
		{
			const auto* Target = Scenes.Find(Choice.Next);
			if (!Target || Target->Entry != TEXT("choice") || Target->Phase != Scene.Phase || !ValidEffects(Choice.OnSelect))
			{
				Error = TEXT("Invalid horror choice: ") + Choice.Id; return false;
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
			Error = TEXT("Invalid horror recording alias: ") + Pair.Key; return false;
		}
	}
	Reset();
	return true;
}

void FS47HorrorPack::Reset()
{
	Flags = Defaults;
	ClearExternal();
	Phase = TEXT("commute"); ActiveScene.Reset(); PendingChoice.Reset(); Cursor = 0; OwnedDocuments.Reset();
}

void FS47HorrorPack::ClearExternal()
{
	for (auto& Pair : External) Pair.Value = false;
}

void FS47HorrorPack::Cancel()
{
	ActiveScene.Reset(); PendingChoice.Reset(); Cursor = 0;
}

TArray<FString> FS47HorrorPack::SceneIdsForScenario(const FString& Id) const
{
	TArray<FString> Result;
	for (const auto& Sid : AuthoredSceneIds)
		if (const auto* Entry = Registry.Find(Sid); Entry && Entry->ScenarioIds.Contains(Id)) Result.Add(Sid);
	return Result;
}

TArray<FString> FS47HorrorPack::ExternalIds() const
{
	TArray<FString> Result;
	External.GetKeys(Result);
	Result.Sort();
	return Result;
}

bool FS47HorrorPack::SetPhase(const FString& Value)
{
	if (!ActiveScene.IsEmpty() || !Phases.Contains(Value)) return false;
	Phase = Value;
	return true;
}

bool FS47HorrorPack::SetExternal(const FString& Id, bool bValue)
{
	bool* Value = External.Find(Id);
	if (!Value) return false;
	*Value = bValue;
	return true;
}

bool FS47HorrorPack::Flag(const FString& Id) const
{
	const bool* Value = External.Find(Id);
	return Value ? *Value : Flags.FindRef(Id);
}

bool FS47HorrorPack::Meets(const FS47EveningRequirements& Requires, const TMap<FString, bool>& Internal) const
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

bool FS47HorrorPack::CanStart(const FString& Id) const
{
	const auto* Scene = Scenes.Find(Id);
	return ActiveScene.IsEmpty() && Scene && Scene->Entry == TEXT("trigger") && Scene->Phase == Phase && Meets(Scene->Requires, Flags);
}

bool FS47HorrorPack::Start(const FString& Id)
{
	if (!CanStart(Id)) return false;
	ActiveScene = Id; Cursor = 0; PendingChoice.Reset();
	if (CurrentScene()->Lines.IsEmpty() && CurrentScene()->Choices.IsEmpty()) CompleteLeaf();
	return true;
}

const FS47EveningScene* FS47HorrorPack::CurrentScene() const { return Scenes.Find(ActiveScene); }

const FS47EveningLine* FS47HorrorPack::CurrentLine() const
{
	const auto* Scene = CurrentScene();
	return Scene && Scene->Lines.IsValidIndex(Cursor) ? &Scene->Lines[Cursor] : nullptr;
}

void FS47HorrorPack::Apply(const FS47EveningEffects& Effects)
{
	for (const auto& Pair : Effects.Flags) if (Defaults.Contains(Pair.Key) && !External.Contains(Pair.Key)) Flags.Add(Pair.Key, Pair.Value);
	for (const auto& Id : Effects.Documents) if (DocumentData.Contains(Id)) OwnedDocuments.AddUnique(Id);
}

void FS47HorrorPack::CompleteLeaf()
{
	const auto* Scene = CurrentScene();
	if (!Scene || !Scene->Choices.IsEmpty()) return;
	// A pending response can be restored before external world/photo facts are rebuilt.
	if (!Meets(Scene->Requires, Flags))
	{
		ActiveScene.Reset(); PendingChoice.Reset(); Cursor = 0;
		return;
	}
	Apply(Scene->OnComplete);
	ActiveScene.Reset(); PendingChoice.Reset(); Cursor = 0;
}

bool FS47HorrorPack::Advance()
{
	const auto* Scene = CurrentScene();
	if (!Scene || Cursor >= Scene->Lines.Num()) return false;
	// A restored scene remains pending until the host has reconstructed its facts.
	// Losing an observation or its physical prerequisite cannot award the result.
	if (!Meets(Scene->Requires, Flags)) return false;
	++Cursor;
	PendingChoice.Reset();
	if (Cursor == Scene->Lines.Num() && Scene->Choices.IsEmpty()) CompleteLeaf();
	return true;
}

bool FS47HorrorPack::Eligible(const FS47EveningChoice& Choice) const
{
	if (!Meets(Choice.Requires, Flags)) return false;
	const auto* Target = Scenes.Find(Choice.Next);
	if (!Target || Target->Entry != TEXT("choice") || Target->Phase != Phase) return false;
	// Evaluate the response against the prospective selection flags atomically.
	auto Prospective = Flags;
	for (const auto& Pair : Choice.OnSelect.Flags)
	{
		if (!Defaults.Contains(Pair.Key) || External.Contains(Pair.Key)) return false;
		Prospective.Add(Pair.Key, Pair.Value);
	}
	return Meets(Target->Requires, Prospective);
}

TArray<const FS47EveningChoice*> FS47HorrorPack::Choices() const
{
	TArray<const FS47EveningChoice*> Result;
	const auto* Scene = CurrentScene();
	if (Scene && Cursor == Scene->Lines.Num() && Meets(Scene->Requires, Flags))
		for (const auto& Choice : Scene->Choices) if (Eligible(Choice)) Result.Add(&Choice);
	return Result;
}

bool FS47HorrorPack::Select(const FString& Id)
{
	for (const auto* Choice : Choices()) if (Choice->Id == Id)
	{
		// Selection effects and pending response are a single save transaction.
		const FString Target = Choice->Next;
		Apply(Choice->OnSelect);
		PendingChoice = Id; ActiveScene = Target; Cursor = 0;
		if (CurrentScene()->Lines.IsEmpty() && CurrentScene()->Choices.IsEmpty()) CompleteLeaf();
		return true;
	}
	return false;
}

FString FS47HorrorPack::RecordingId(const FString& Cue) const
{
	const FString* Alias = Aliases.Find(Cue);
	return Alias ? *Alias : Cue;
}

TSharedPtr<FJsonObject> FS47HorrorPack::Save() const
{
	const auto Json = MakeShared<FJsonObject>();
	Json->SetNumberField(TEXT("version"), 1);
	Json->SetStringField(TEXT("pack_id"), TEXT("signal47_horror_story_h1"));
	Json->SetStringField(TEXT("phase"), Phase);
	Json->SetStringField(TEXT("active_scene"), ActiveScene);
	Json->SetNumberField(TEXT("next_line"), Cursor);
	Json->SetStringField(TEXT("pending_choice"), PendingChoice);
	const auto SavedFlags = MakeShared<FJsonObject>();
	for (const auto& Pair : Flags) SavedFlags->SetBoolField(Pair.Key, Pair.Value);
	Json->SetObjectField(TEXT("flags"), SavedFlags);
	Json->SetArrayField(TEXT("documents"), HorrorWriteStrings(OwnedDocuments));
	// External context is rebuilt from the host's actual saved chapter/world state.
	return Json;
}

bool FS47HorrorPack::Load(const TSharedPtr<FJsonObject>& Json, FString& Error)
{
	Error = TEXT("Invalid horror save");
	if (!Json.IsValid() || Scenes.IsEmpty()) return false;
	double Version = 0, SavedCursor = 0;
	FString PackId, SavedPhase, SavedScene, SavedChoice;
	const TSharedPtr<FJsonObject>* SavedFlags = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* SavedDocs = nullptr;
	if (!Json->HasTypedField<EJson::Number>(TEXT("version")) || !Json->TryGetNumberField(TEXT("version"), Version) || Version != 1 ||
		!Json->TryGetStringField(TEXT("pack_id"), PackId) || PackId != TEXT("signal47_horror_story_h1") ||
		!Json->TryGetStringField(TEXT("phase"), SavedPhase) || !Phases.Contains(SavedPhase) ||
		!Json->TryGetStringField(TEXT("active_scene"), SavedScene) ||
		!Json->TryGetStringField(TEXT("pending_choice"), SavedChoice) ||
		!Json->HasTypedField<EJson::Number>(TEXT("next_line")) || !Json->TryGetNumberField(TEXT("next_line"), SavedCursor) ||
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
		const FString Id(*Pair.Key);
		if (!Defaults.Contains(Id) || External.Contains(Id) || Pair.Value->Type != EJson::Boolean || !Pair.Value->TryGetBool(bValue)) return false;
		NewFlags.Add(Id, bValue);
	}
	if (Scene)
	{
		// Context is restored by the host after Load; internal preconditions still
		// protect against pending responses whose authored prerequisites are missing.
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

bool FS47HorrorPack::SelfTest(FString& Error)
{
	FS47HorrorPack P;
	if (!P.Initialize(Error)) return false;
#define HORROR_CHECK(Condition, Message) do { if (!(Condition)) { Error = TEXT(Message); return false; } } while (false)
	HORROR_CHECK(P.Scenes.Num() == 79 && P.Registry.Num() == 79 && P.DocumentData.Num() == 16 && P.External.Num() == 96,
		"Horror source contains every baseline scene, document and external gate");
	HORROR_CHECK(P.FindRegistration(TEXT("H01_BRIEF"))->ScenarioIds.Contains(TEXT("SC01")) &&
		P.FindRegistration(TEXT("H02_CHECK"))->ScenarioIds.Contains(TEXT("SC02")), "Original H01/H02 cards retain their scenario mapping");
	HORROR_CHECK(!P.SetExternal(TEXT("hr_h01_recorded"), true) && !P.SetExternal(TEXT("ctx_unknown"), true),
		"Host can write declared external gates only");
	HORROR_CHECK(!P.CanStart(TEXT("H01_BRIEF")), "Initial phase and facts do not grant a story scene");
	HORROR_CHECK(P.SetPhase(TEXT("handover")) && !P.CanStart(TEXT("H01_BRIEF")), "H01 cannot start from phase alone");
	HORROR_CHECK(P.SetExternal(TEXT("ctx_hr_h01_ready"), true) && P.Start(TEXT("H01_BRIEF")), "H01 starts after real ready context");
	HORROR_CHECK(!P.SetPhase(TEXT("signal")) && P.CurrentLine(), "Active dialogue cannot jump phase");
	const FString FirstLine = P.CurrentLine()->Id;
	const auto Pending = P.Save();
	HORROR_CHECK(P.Load(Pending, Error) && P.CurrentLine() && P.CurrentLine()->Id == FirstLine && !P.Advance(),
		"Pending dialogue restores without borrowing old external context");
	P.SetExternal(TEXT("ctx_hr_h01_ready"), true);
	while (P.CurrentLine()) HORROR_CHECK(P.Advance(), "Restored H01 brief advances with rebuilt facts");
	HORROR_CHECK(P.Flag(TEXT("hr_h01_brief_presented")) && !P.Flag(TEXT("hr_h01_recorded")),
		"Brief records presentation, not completed physical inspection");
	HORROR_CHECK(!P.Start(TEXT("H01_CHECK")), "Presentation cannot fabricate a physical result");
	P.SetExternal(TEXT("ctx_hr_h01_checked"), true);
	HORROR_CHECK(P.Start(TEXT("H01_CHECK")), "Physical result permits H01 check scene");
	P.SetExternal(TEXT("ctx_hr_h01_checked"), false);
	HORROR_CHECK(!P.Advance() && !P.Flag(TEXT("hr_h01_recorded")), "Revoked fact blocks pending conclusion");
	P.Cancel();
	HORROR_CHECK(!P.CurrentScene() && !P.Flag(TEXT("hr_h01_recorded")), "Cancellation awards no completion");
	P.SetExternal(TEXT("ctx_hr_h01_checked"), true);
	HORROR_CHECK(P.Start(TEXT("H01_CHECK")), "Cancelled physical result can be presented later");
	while (P.CurrentLine()) HORROR_CHECK(P.Advance(), "H01 check advances");
	HORROR_CHECK(P.Flag(TEXT("hr_h01_recorded")), "Checked H01 presentation is preserved");
	const auto Good = P.Save();
	const FString Before = S47J::Write(Good);
	const auto BadFlag = S47J::Clone(Good);
	BadFlag->GetObjectField(TEXT("flags"))->SetBoolField(TEXT("ctx_hr_h02_checked"), true);
	HORROR_CHECK(!P.Load(BadFlag, Error) && P.Flag(TEXT("hr_h01_recorded")), "Saved script flags cannot smuggle world context");
	const auto BadVersion = S47J::Clone(Good); BadVersion->SetBoolField(TEXT("version"), true);
	HORROR_CHECK(!P.Load(BadVersion, Error), "Boolean save version is not numeric schema version");
	const auto BadCursor = S47J::Clone(Good); BadCursor->SetNumberField(TEXT("next_line"), 0.5);
	HORROR_CHECK(!P.Load(BadCursor, Error), "Fractional dialogue cursor is rejected");
	const auto BadDoc = S47J::Clone(Good);
	BadDoc->SetArrayField(TEXT("documents"), {MakeShared<FJsonValueString>(TEXT("UNAUTHORED"))});
	HORROR_CHECK(!P.Load(BadDoc, Error), "Unknown document is rejected");
	HORROR_CHECK(S47J::Write(P.Save()) == Before, "Rejected restore leaves the active pack unchanged");

	// Isolated adapter tests supply explicit synthetic context. These test data
	// gates only; they are not evidence that the corresponding world action ran.
	for (const FString& Id : P.AuthoredSceneIds)
	{
		P.Reset();
		const auto* Scene = P.FindScene(Id);
		HORROR_CHECK(Scene && P.SetPhase(Scene->Phase), "Every registered scene has a known phase");
		for (const FString& Gate : Scene->Requires.All)
		{
			if (P.External.Contains(Gate)) P.SetExternal(Gate, true);
			else P.Flags.Add(Gate, true);
		}
		for (const FString& Gate : Scene->Requires.None)
		{
			if (P.External.Contains(Gate)) P.SetExternal(Gate, false);
			else P.Flags.Add(Gate, false);
		}
		HORROR_CHECK(P.CanStart(Id), "A registered trigger starts with its declared facts");
		for (const FString& Gate : Scene->Requires.All)
		{
			if (!P.External.Contains(Gate)) continue;
			P.SetExternal(Gate, false);
			HORROR_CHECK(!P.CanStart(Id), "Every required world gate independently blocks its scene");
			P.SetExternal(Gate, true);
		}
		HORROR_CHECK(P.Start(Id), "Registered scene begins after explicit synthetic facts");
		while (P.CurrentLine()) HORROR_CHECK(P.Advance(), "Every authored line is acknowledged exactly once");
		HORROR_CHECK(!P.CurrentScene(), "Baseline authored scenes complete without an undeclared choice");
		const auto Snapshot = S47J::Clone(P.Save());
		HORROR_CHECK(P.Load(Snapshot, Error), "Every completed authored scene round-trips its pack state");
		for (const FString& Gate : P.ExternalIds()) HORROR_CHECK(!P.Flag(Gate), "Load never restores a world fact from dialogue");
	}
#undef HORROR_CHECK
	Error.Reset();
	return true;
}

#include "S47Saves.h"

#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "S47Image.h"
#include "Slate/WidgetRenderer.h"
#include "SS47Widgets.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Saves, Log, All);

namespace
{
	const FString REF = TEXT("idb:"), LS_REF = TEXT("ls:");

	bool IsPhotoUrl(const FString& U) { return U.StartsWith(TEXT("data:")) || U.StartsWith(REF) || U.StartsWith(LS_REF); }

	// photos(): every photograph in a state, wherever it sits (chapter one's prints, later chapters):
	// an object with a string id and a string url that is a picture or a reference to one
	void Photos(const TSharedPtr<FJsonValue>& V, TArray<TSharedPtr<FJsonObject>>& Out, int32 Depth = 0)
	{
		if (!V.IsValid() || Depth > 6) return;
		if (V->Type == EJson::Object)
		{
			const TSharedPtr<FJsonObject> O = V->AsObject();
			if (!O.IsValid()) return;
			if (O->HasTypedField<EJson::String>(TEXT("id")) && O->HasTypedField<EJson::String>(TEXT("url")) && IsPhotoUrl(O->GetStringField(TEXT("url"))))
			{
				Out.Add(O);
				return;
			}
			for (const auto& KV : O->Values) Photos(KV.Value, Out, Depth + 1);
		}
		else if (V->Type == EJson::Array)
		{
			for (const TSharedPtr<FJsonValue>& X : V->AsArray()) Photos(X, Out, Depth + 1);
		}
	}
	TArray<TSharedPtr<FJsonObject>> PhotosOf(const S47J::FObj& State)
	{
		TArray<TSharedPtr<FJsonObject>> Out;
		if (State.IsValid()) Photos(MakeShared<FJsonValueObject>(State), Out);
		return Out;
	}

	// A stand-in print when a photograph has gone missing (site data cleared in between): the paper,
	// the black of the print and PRINT NOT AVAILABLE on it, 508 x 360 (saves.ts missingPrint)
	FString DrawMissingPrint()
	{
		const FLinearColor Paper = FLinearColor::FromSRGBColor(FColor(0xeb, 0xe5, 0xd3)), Black = FLinearColor::FromSRGBColor(FColor(0x1b, 0x1a, 0x18));
		TArray<FColor> Px;
		UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>();
		if (RT && FSlateApplication::IsInitialized())
		{
			RT->InitCustomFormat(508, 360, PF_B8G8R8A8, false);
			RT->UpdateResourceImmediate(true);
			const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
			TSharedRef<SWidget> W = SNew(SBorder).BorderImage(White).BorderBackgroundColor(Paper).Padding(0)
				[
					SNew(SCanvas)
					+ SCanvas::Slot().Position(FVector2D(14, 14)).Size(FVector2D(480, 300))[SNew(SBorder).BorderImage(White).BorderBackgroundColor(Black)]
					// g.font = '22px "Special Elite"', centred on x 254 with the baseline at 170
					+ SCanvas::Slot().Position(FVector2D(0, 170 - 22)).Size(FVector2D(508, 30))
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("PRINT NOT AVAILABLE"))).Font(S47Style::Body(22.f)).Justification(ETextJustify::Center)
						.ColorAndOpacity(FLinearColor::FromSRGBColor(FColor(0xd8, 0xd0, 0xbc)))
					]
				];
			FWidgetRenderer R(true);
			R.DrawWidget(RT, W, FVector2D(508, 360), 0.f);
			FlushRenderingCommands();
			S47Image::ReadPixels(RT, Px);
		}
		if (Px.Num() != 508 * 360)
		{
			// (no renderer: the paper and the black without the words)
			Px.SetNumUninitialized(508 * 360);
			for (int32 Y = 0; Y < 360; ++Y) for (int32 X = 0; X < 508; ++X)
				Px[Y * 508 + X] = (X >= 14 && X < 494 && Y >= 14 && Y < 314 ? Black : Paper).ToFColor(true);
		}
		return S47Image::ToDataUrl(Px, 508, 360, true, 80);
	}
}

// ---------- names and keys ----------

FString S47Saves::SaveId(int32 CaseId, const FString& Kind, int32 Slot)
{
	return FString::Printf(TEXT("%d:%s:%d"), CaseId, *Kind, Slot);
}

FString S47Saves::PhotoKey(const FString& Url)
{
	// cyrb53: a quick 53-bit string hash, plenty to tell photographs apart (on UTF-16 code units,
	// as charCodeAt; the multiplications wrap at 32 bits as Math.imul)
	uint32 H1 = 0xdeadbeefu, H2 = 0x41c6ce57u;
	for (int32 i = 0; i < Url.Len(); ++i)
	{
		const uint32 C = uint32(Url[i]);
		H1 = (H1 ^ C) * 2654435761u;
		H2 = (H2 ^ C) * 1597334677u;
	}
	H1 = ((H1 ^ (H1 >> 16)) * 2246822507u) ^ ((H2 ^ (H2 >> 13)) * 3266489909u);
	H2 = ((H2 ^ (H2 >> 16)) * 2246822507u) ^ ((H1 ^ (H1 >> 13)) * 3266489909u);
	uint64 V = (uint64(H2 & 2097151u) << 32) | uint64(H1);
	// toString(36)
	FString S;
	do { const int32 D = int32(V % 36); S.InsertAt(0, TCHAR(D < 10 ? TEXT('0') + D : TEXT('a') + D - 10)); V /= 36; } while (V);
	return TEXT("p") + S;
}

double S47Saves::NowMs()
{
	return (FDateTime::UtcNow() - FDateTime(1970, 1, 1)).GetTotalMilliseconds();
}

FString S47State::CheckpointTitle(const FString& Checkpoint)
{
	// state.ts CHAPTERS
	if (Checkpoint == TEXT("routine")) return TEXT("Evening Shift: Reference Round");
 if (Checkpoint == TEXT("nightService")) return TEXT("Night Shift: Caliche Feeder / Residual");
	if (Checkpoint == TEXT("residual")) return TEXT("Prologue: Night Shift");
	if (Checkpoint == TEXT("chapter1")) return TEXT("Chapter 1: The Second Exposure");
	if (Checkpoint == TEXT("chapter2")) return TEXT("Chapter 2: The Amended Record");
	if (Checkpoint == TEXT("chapter3")) return TEXT("Chapter 3: The Survey Station");
	return FString();
}

// ---------- the card ----------

void FS47SaveMeta::Write(const S47J::FObj& O) const
{
	S47J::Set(O, TEXT("id"), Id);
	S47J::Set(O, TEXT("caseId"), CaseId);
	S47J::Set(O, TEXT("kind"), Kind);
	S47J::Set(O, TEXT("slot"), Slot);
	S47J::Set(O, TEXT("savedAt"), SavedAt);
	S47J::Set(O, TEXT("playtime"), Playtime);
	S47J::Set(O, TEXT("chapter"), Chapter);
	S47J::Set(O, TEXT("place"), Place);
	S47J::Set(O, TEXT("clock"), Clock);
	S47J::SetOrNull(O, TEXT("thumb"), Thumb);
	S47J::Set(O, TEXT("photos"), Photos);
	S47J::Set(O, TEXT("version"), Version);
}

FS47SaveMeta FS47SaveMeta::Read(const S47J::FObj& O)
{
	FS47SaveMeta M;
	M.Id = S47J::Str(O, TEXT("id"));
	M.CaseId = S47J::Int(O, TEXT("caseId"));
	M.Kind = S47J::Str(O, TEXT("kind"));
	M.Slot = S47J::Int(O, TEXT("slot"));
	M.SavedAt = S47J::Num(O, TEXT("savedAt"));
	M.Playtime = S47J::Num(O, TEXT("playtime"));
	M.Chapter = S47J::Str(O, TEXT("chapter"));
	M.Place = S47J::Str(O, TEXT("place"));
	M.Clock = S47J::Str(O, TEXT("clock"));
	M.Thumb = S47J::Str(O, TEXT("thumb"));
	M.Photos = S47J::Strs(O, TEXT("photos"));
	M.Version = S47J::Int(O, TEXT("version"), 2);
	return M;
}

// ---------- the store ----------

FS47SaveStore::FS47SaveStore(const FString& InDir)
	: Dir(InDir.IsEmpty() ? FPaths::ProjectSavedDir() / TEXT("SaveGames") : InDir)
{
	Now = [] { return S47Saves::NowMs(); };
	MissingPrint = [] { return DrawMissingPrint(); };
}

FString FS47SaveStore::SavePath(const FString& Id) const { return Dir / (TEXT("save-") + Id.Replace(TEXT(":"), TEXT("-")) + TEXT(".json")); }
FString FS47SaveStore::PhotoPath(const FString& Key) const { return Dir / TEXT("photos") / (Key + TEXT(".txt")); }
FString FS47SaveStore::ActivePath() const { return Dir / TEXT("active.json"); }

bool FS47SaveStore::WriteFile(const FString& Path, const FString& Text) const
{
	// written beside and moved into place, so a save is never half there
	const FString Tmp = Path + TEXT(".tmp");
	if (!FFileHelper::SaveStringToFile(Text, *Tmp, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return false;
	return IFileManager::Get().Move(*Path, *Tmp, true, true);
}

void FS47SaveStore::Init()
{
	Metas.Empty();
	Stored.Empty();
	IFileManager& F = IFileManager::Get();
	F.MakeDirectory(*(Dir / TEXT("photos")), true);
	bFallback = !F.DirectoryExists(*(Dir / TEXT("photos")));
	TArray<FString> Files;
	F.FindFiles(Files, *(Dir / TEXT("save-*.json")), true, false);
	for (const FString& N : Files)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *(Dir / N))) continue;
		const S47J::FObj R = S47J::Read(Text);
		if (!R.IsValid()) continue;
		const FS47SaveMeta M = FS47SaveMeta::Read(R);
		if (!M.Id.IsEmpty()) Metas.Add(M.Id, M);
	}
	TArray<FString> Keys;
	F.FindFiles(Keys, *(Dir / TEXT("photos") / TEXT("*.txt")), true, false);
	for (const FString& K : Keys) Stored.Add(FPaths::GetBaseFilename(K));
	UE_LOG(LogS47Saves, Display, TEXT("S47: saves: %d in %s, %d photographs"), Metas.Num(), *Dir, Stored.Num());
}

// ---------- reading ----------

TArray<FS47SaveMeta> FS47SaveStore::List(int32 CaseId) const
{
	TArray<FS47SaveMeta> L;
	for (const TPair<FString, FS47SaveMeta>& KV : Metas) if (CaseId == 0 || KV.Value.CaseId == CaseId) L.Add(KV.Value);
	L.StableSort([](const FS47SaveMeta& A, const FS47SaveMeta& B) { return A.SavedAt > B.SavedAt; });
	return L;
}

TOptional<FS47SaveMeta> FS47SaveStore::Latest(int32 CaseId) const
{
	const TArray<FS47SaveMeta> L = List(CaseId);
	return L.Num() ? TOptional<FS47SaveMeta>(L[0]) : TOptional<FS47SaveMeta>();
}

TArray<int32> FS47SaveStore::UsedCases() const
{
	TArray<int32> C;
	for (const TPair<FString, FS47SaveMeta>& KV : Metas) C.AddUnique(KV.Value.CaseId);
	C.Sort();
	return C;
}

int32 FS47SaveStore::FreeCase() const
{
	const TArray<int32> Used = UsedCases();
	for (int32 C = 1; C <= S47Saves::CASES; ++C) if (!Used.Contains(C)) return C;
	return 0;
}

int32 FS47SaveStore::Active() const
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *ActivePath())) return 0;
	const int32 C = S47J::Int(S47J::Read(Text), TEXT("activeCase"));
	return C >= 1 && C <= S47Saves::CASES ? C : 0;
}

void FS47SaveStore::SetActive(int32 CaseId)
{
	const S47J::FObj O = S47J::New();
	S47J::Set(O, TEXT("activeCase"), CaseId);
	WriteFile(ActivePath(), S47J::Write(O));
}

TOptional<FS47SaveMeta> FS47SaveStore::ContinueSave() const
{
	const int32 A = Active();
	TOptional<FS47SaveMeta> M = A ? Latest(A) : TOptional<FS47SaveMeta>();
	return M ? M : Latest();
}

TOptional<FString> FS47SaveStore::ReadPhoto(const FString& Key)
{
	if (FailReads > 0) { --FailReads; return {}; }
	FString Url;
	if (!FFileHelper::LoadFileToString(Url, *PhotoPath(Key)) || Url.IsEmpty()) return {};
	return Url;
}

S47J::FObj FS47SaveStore::Load(const FString& Id)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *SavePath(Id))) return nullptr;
	const S47J::FObj R = S47J::Read(Text);
	const S47J::FObj State = S47J::Obj(R, TEXT("state"));
	if (!State.IsValid()) return nullptr;
	for (const TSharedPtr<FJsonObject>& P : PhotosOf(State))
	{
		const FString Url = P->GetStringField(TEXT("url")), PhotoId = P->GetStringField(TEXT("id"));
		const bool bLocal = Url.StartsWith(LS_REF);
		if (!bLocal && !Url.StartsWith(REF)) continue;
		const FString Key = Url.Mid(bLocal ? LS_REF.Len() : REF.Len());
		// one more try: a failed read is often a one-off
		TOptional<FString> Got = ReadPhoto(Key);
		if (!Got) Got = ReadPhoto(Key);
		if (Got)
		{
			if (bLocal == bFallback) Stored.Add(Key);
			P->SetStringField(TEXT("url"), *Got);
			P->RemoveField(TEXT("unavailable"));
			Unread.Remove(PhotoId);
		}
		else
		{
			const FString Shown = MissingPrint ? MissingPrint() : FString();
			Unread.Add(PhotoId, { Shown, Url });
			P->SetStringField(TEXT("url"), Shown);
			// A visible stand-in is not photographic evidence for a new investigation.
			P->SetBoolField(TEXT("unavailable"), true);
		}
	}
	SetActive(S47J::Int(R, TEXT("caseId")));
	return State;
}

// ---------- writing ----------

TOptional<FS47SaveMeta> FS47SaveStore::Save(int32 CaseId, const FString& Kind, int32 Slot, const S47J::FObj& State, const FS47SaveCard& Card, FString& OutError)
{
	// (the web version queues writes, as the game goes on while one waits its turn; here a write
	// is done before the call returns, so there is nothing to queue)
	const int32 S = Slot < 0 ? NextAuto(CaseId) : Slot;
	return WriteRecord(CaseId, Kind, S, State, Card, OutError);
}

int32 FS47SaveStore::NextAuto(int32 CaseId)
{
	// Autosaves take turns; one written less than a minute and a half ago is updated in place.
	TArray<FS47SaveMeta> Autos = List(CaseId).FilterByPredicate([](const FS47SaveMeta& M) { return M.Kind == TEXT("auto"); });
	const bool bFresh = Generation.Remove(CaseId) == 0;
	if (bFresh && Autos.Num() && Now() - Autos[0].SavedAt < 90000) return Autos[0].Slot;
	for (int32 i = 0; i < S47Saves::AUTO; ++i) if (!Autos.ContainsByPredicate([i](const FS47SaveMeta& M) { return M.Slot == i; })) return i;
	return Autos.Last().Slot; // the oldest
}

TOptional<FS47SaveMeta> FS47SaveStore::WriteRecord(int32 CaseId, const FString& Kind, int32 Slot, const S47J::FObj& State, const FS47SaveCard& Card, FString& OutError)
{
	const S47J::FObj Slim = S47J::Clone(State);
	TArray<FString> Keys;
	TArray<TPair<FString, FString>> ToStore;
	for (const TSharedPtr<FJsonObject>& P : PhotosOf(Slim))
	{
		FString Url = P->GetStringField(TEXT("url"));
		const FUnread* Lost = Unread.Find(P->GetStringField(TEXT("id")));
		if (Lost && Lost->Shown == Url) { Url = Lost->Ref; P->SetStringField(TEXT("url"), Url); } // still the stand-in: keep pointing at the original
		if (Url.StartsWith(REF)) { Keys.Add(Url.Mid(REF.Len())); continue; }
		if (Url.StartsWith(LS_REF)) { Keys.Add(Url.Mid(LS_REF.Len())); continue; }
		const FString Key = S47Saves::PhotoKey(Url);
		if (!Stored.Contains(Key) && !ToStore.ContainsByPredicate([&](const TPair<FString, FString>& T) { return T.Key == Key; })) ToStore.Add({ Key, Url });
		P->SetStringField(TEXT("url"), REF + Key);
		Keys.Add(Key);
	}
	FS47SaveMeta Meta;
	Meta.Id = S47Saves::SaveId(CaseId, Kind, Slot);
	Meta.CaseId = CaseId; Meta.Kind = Kind; Meta.Slot = Slot; Meta.SavedAt = Now();
	Meta.Playtime = FMath::RoundHalfFromZero(Card.Playtime); Meta.Chapter = Card.Chapter; Meta.Place = Card.Place; Meta.Clock = Card.Clock; Meta.Thumb = Card.Thumb;
	for (const FString& K : Keys) Meta.Photos.AddUnique(K);
	const S47J::FObj Record = S47J::New();
	Meta.Write(Record);
	Record->SetObjectField(TEXT("state"), Slim);
	// photographs first, so a save never points at a print that is not there (all or none, as
	// the web version's one transaction)
	TArray<FString> Added;
	for (const TPair<FString, FString>& T : ToStore)
	{
		if (!WriteFile(PhotoPath(T.Key), T.Value))
		{
			for (const FString& K : Added) IFileManager::Get().Delete(*PhotoPath(K));
			OutError = TEXT("The photographs could not be stored.");
			return {};
		}
		Added.Add(T.Key);
	}
	for (const FString& K : Added) Stored.Add(K);
	if (!WriteFile(SavePath(Meta.Id), S47J::Write(Record))) { OutError = TEXT("The save could not be written."); return {}; }
	Metas.Add(Meta.Id, Meta);
	// a new photograph under the same id has replaced the one that could not be read
	for (const TSharedPtr<FJsonObject>& P : PhotosOf(State))
	{
		const FString Url = P->GetStringField(TEXT("url")), PhotoId = P->GetStringField(TEXT("id"));
		const FUnread* U = Unread.Find(PhotoId);
		if (Url.StartsWith(TEXT("data:")) && (!U || U->Shown != Url)) Unread.Remove(PhotoId);
	}
	Sweep();
	UE_LOG(LogS47Saves, Display, TEXT("S47: saved %s (%s, %s, %s)"), *Meta.Id, *Meta.Chapter, *Meta.Place, *Meta.Clock);
	return Meta;
}

bool FS47SaveStore::Remove(const FString& Id, FString& OutError)
{
	if (!IFileManager::Get().Delete(*SavePath(Id), false, true, true)) { OutError = TEXT("The save could not be deleted."); return false; }
	Metas.Remove(Id);
	Sweep();
	return true;
}

bool FS47SaveStore::RemoveCase(int32 CaseId, FString& OutError)
{
	for (const FS47SaveMeta& M : List(CaseId)) if (!Remove(M.Id, OutError)) return false;
	if (Active() == CaseId) IFileManager::Get().Delete(*ActivePath(), false, true, true);
	return true;
}

void FS47SaveStore::Sweep()
{
	// Photographs no save points at any more are deleted.
	TSet<FString> Used;
	for (const TPair<FString, FS47SaveMeta>& KV : Metas) for (const FString& K : KV.Value.Photos) Used.Add(K);
	TArray<FString> Stale;
	for (const FString& K : Stored) if (!Used.Contains(K)) Stale.Add(K);
	for (const FString& K : Stale) if (IFileManager::Get().Delete(*PhotoPath(K), false, true, true)) Stored.Remove(K);
}

// ---------- self test (after tools/saves.cjs) ----------

bool S47Saves::SelfTest(TArray<FString>& Out)
{
	bool bOk = true;
	auto Check = [&](const TCHAR* Name, bool bPass, const FString& Got = FString())
	{
		Out.Add(bPass ? FString::Printf(TEXT("ok  saves: %s"), Name) : FString::Printf(TEXT("FAIL saves: %s %s"), Name, *Got));
		bOk &= bPass;
	};
	// the photo keys, against the web version's own hash (node, 22 October 2026)
	Check(TEXT("photoKey empty"), PhotoKey(TEXT("")) == TEXT("pwvjl67o803"), PhotoKey(TEXT("")));
	Check(TEXT("photoKey a"), PhotoKey(TEXT("a")) == TEXT("p262p94epp1d"), PhotoKey(TEXT("a")));
	Check(TEXT("photoKey original"), PhotoKey(TEXT("data:image/jpeg;base64,T1JJR0lOQUxfUEhPVE8=")) == TEXT("p5kxyyvb0r9"));
	Check(TEXT("photoKey second"), PhotoKey(TEXT("data:image/jpeg;base64,U0VDT05EX1BIT1RP")) == TEXT("p1r2blb8iidk"));
	Check(TEXT("photoKey title"), PhotoKey(TEXT("SIGNAL / 47")) == TEXT("p15snohen7th"));
	Check(TEXT("saveId"), SaveId(2, TEXT("auto"), 1) == TEXT("2:auto:1"));
	Check(TEXT("checkpoint title"), S47State::CheckpointTitle(TEXT("chapter2")) == TEXT("Chapter 2: The Amended Record") && S47State::CheckpointTitle(TEXT("chapter4")).IsEmpty());

	const FString Root = FPaths::ProjectSavedDir() / TEXT("SaveGames") / FString::Printf(TEXT("SelfTest-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	const FString ORIGINAL = TEXT("data:image/jpeg;base64,T1JJR0lOQUxfUEhPVE8="), SECOND = TEXT("data:image/jpeg;base64,U0VDT05EX1BIT1RP"), STAND_IN = TEXT("data:image/jpeg;base64,UExBQ0VIT0xERVI=");
	double Clock = 1000000;
	int32 N = 0;
	auto Open = [&](const FString& D)
	{
		TSharedRef<FS47SaveStore> S = MakeShared<FS47SaveStore>(D);
		S->Now = [&Clock] { return Clock; };
		S->MissingPrint = [STAND_IN] { return STAND_IN; };
		S->Init();
		return S;
	};
	auto Fresh = [&] { return Root / FString::FromInt(++N); };
	FS47SaveCard Card;
	Card.Chapter = TEXT("Chapter 1: The Second Exposure"); Card.Place = TEXT("Photo lab"); Card.Clock = TEXT("02:40"); Card.Playtime = 600;
	auto CaseWith = [](const FString& Url)
	{
		// { v: 2, checkpoint: 'chapter1', area: 'saro', pose: null, case: { s: { f1: { id: 'frame01', url } }, notes: [], clock: 9600 } }
		const S47J::FObj F1 = S47J::New(); S47J::Set(F1, TEXT("id"), TEXT("frame01")); S47J::Set(F1, TEXT("url"), Url);
		const S47J::FObj S = S47J::New(); S47J::Set(S, TEXT("f1"), F1);
		const S47J::FObj C = S47J::New(); S47J::Set(C, TEXT("s"), S); S47J::Set(C, TEXT("notes"), TArray<FString>()); S47J::Set(C, TEXT("clock"), 9600.0);
		const S47J::FObj G = S47J::New(); S47J::Set(G, TEXT("v"), 2); S47J::Set(G, TEXT("checkpoint"), TEXT("chapter1")); S47J::Set(G, TEXT("area"), TEXT("saro"));
		S47J::SetNull(G, TEXT("pose")); S47J::Set(G, TEXT("case"), C);
		return G;
	};
	auto F1Url = [](const S47J::FObj& G) { return S47J::Str(S47J::Obj(S47J::Obj(S47J::Obj(G, TEXT("case")), TEXT("s")), TEXT("f1")), TEXT("url")); };
	auto IdOf = [](const TOptional<FS47SaveMeta>& M) { return M ? M->Id : FString(TEXT("(failed)")); };
	auto CountPhotos = [](const FString& D) { TArray<FString> F; IFileManager::Get().FindFiles(F, *(D / TEXT("photos") / TEXT("*.txt")), true, false); return F.Num(); };
	FString Err;

	{
		// autosaves take turns: an update within 90 s, a new generation after that, the oldest is replaced
		const FString D = Fresh();
		TSharedRef<FS47SaveStore> S = Open(D);
		FString Ids = IdOf(S->Save(1, TEXT("auto"), -1, CaseWith(ORIGINAL), Card, Err));
		Clock += 30000; Ids += TEXT(" ") + IdOf(S->Save(1, TEXT("auto"), -1, CaseWith(ORIGINAL), Card, Err));
		S->NewGeneration(1); Clock += 1000; Ids += TEXT(" ") + IdOf(S->Save(1, TEXT("auto"), -1, CaseWith(ORIGINAL), Card, Err));
		Clock += 100000; Ids += TEXT(" ") + IdOf(S->Save(1, TEXT("auto"), -1, CaseWith(ORIGINAL), Card, Err));
		Clock += 100000; Ids += TEXT(" ") + IdOf(S->Save(1, TEXT("auto"), -1, CaseWith(ORIGINAL), Card, Err));
		Check(TEXT("autosaves take turns"), Ids == TEXT("1:auto:0 1:auto:0 1:auto:1 1:auto:2 1:auto:0"), Ids);
		Check(TEXT("three autosaves"), S->List(1).Num() == 3);
	}
	{
		// manual saves keep their own slots, and cases are separate
		const FString D = Fresh();
		TSharedRef<FS47SaveStore> S = Open(D);
		S->Save(1, TEXT("auto"), -1, CaseWith(ORIGINAL), Card, Err);
		S->Save(1, TEXT("manual"), 2, CaseWith(ORIGINAL), Card, Err);
		Clock += 1000;
		S->Save(3, TEXT("manual"), 0, CaseWith(SECOND), Card, Err);
		const TArray<int32> Used = S->UsedCases();
		Check(TEXT("used cases"), Used.Num() == 2 && Used[0] == 1 && Used[1] == 3);
		Check(TEXT("free case"), S->FreeCase() == 2);
		Check(TEXT("manual slot"), S->List(1).ContainsByPredicate([](const FS47SaveMeta& M) { return M.Id == TEXT("1:manual:2"); }) && S->List(3).Num() == 1);
		Check(TEXT("continue: the case played last"), S->ContinueSave() && S->ContinueSave()->CaseId == 3);
	}
	{
		// a photograph is stored once, saves point at it, and loading brings it back
		const FString D = Fresh();
		TSharedRef<FS47SaveStore> S = Open(D);
		const TOptional<FS47SaveMeta> M1 = S->Save(1, TEXT("auto"), -1, CaseWith(ORIGINAL), Card, Err);
		S->Save(1, TEXT("manual"), 0, CaseWith(ORIGINAL), Card, Err);
		Check(TEXT("one copy of the print"), CountPhotos(D) == 1);
		FString Text;
		FFileHelper::LoadFileToString(Text, *(D / TEXT("save-1-auto-0.json")));
		Check(TEXT("the save holds a reference"), F1Url(S47J::Obj(S47J::Read(Text), TEXT("state"))).StartsWith(TEXT("idb:p")));
		TSharedRef<FS47SaveStore> Again = Open(D);
		Check(TEXT("loading brings it back"), F1Url(Again->Load(IdOf(M1))) == ORIGINAL);
		Check(TEXT("the card"), Again->List().Num() == 2 && Again->List()[0].Place == TEXT("Photo lab") && Again->List()[0].Playtime == 600 && Again->Active() == 1);
	}
	{
		// a photograph no save points at is deleted, one still in use is kept
		const FString D = Fresh();
		TSharedRef<FS47SaveStore> S = Open(D);
		const TOptional<FS47SaveMeta> A = S->Save(1, TEXT("manual"), 0, CaseWith(ORIGINAL), Card, Err);
		const TOptional<FS47SaveMeta> B = S->Save(1, TEXT("manual"), 1, CaseWith(SECOND), Card, Err);
		S->Save(2, TEXT("manual"), 0, CaseWith(SECOND), Card, Err);
		const int32 Two = CountPhotos(D);
		S->Remove(IdOf(A), Err);
		const int32 One = CountPhotos(D);
		S->Remove(IdOf(B), Err);
		Check(TEXT("unused prints are deleted"), Two == 2 && One == 1 && CountPhotos(D) == 1);
		S->RemoveCase(2, Err);
		Check(TEXT("remove a case"), S->List().Num() == 0 && CountPhotos(D) == 0);
	}
	{
		// a photograph that cannot be read shows the stand-in, and the next save keeps the original
		const FString D = Fresh();
		TSharedRef<FS47SaveStore> S = Open(D);
		const TOptional<FS47SaveMeta> M = S->Save(1, TEXT("auto"), -1, CaseWith(ORIGINAL), Card, Err);
		TSharedRef<FS47SaveStore> Again = Open(D);
		Again->FailReads = 2;
		const S47J::FObj State = Again->Load(IdOf(M));
		Check(TEXT("the stand-in is shown"), F1Url(State) == STAND_IN);
		Check(TEXT("missing print is explicitly unavailable as new evidence"), S47J::Bool(S47J::Obj(S47J::Obj(S47J::Obj(State,TEXT("case")),TEXT("s")),TEXT("f1")),TEXT("unavailable")));
		Again->Save(1, TEXT("manual"), 0, State, Card, Err);
		FString Text;
		FFileHelper::LoadFileToString(Text, *(D / TEXT("save-1-manual-0.json")));
		Check(TEXT("the save still points at the original"), F1Url(S47J::Obj(S47J::Read(Text), TEXT("state"))) == TEXT("idb:") + PhotoKey(ORIGINAL));
		Check(TEXT("the stand-in is never stored"), CountPhotos(D) == 1);
		const auto Recovered = Open(D)->Load(TEXT("1:manual:0"));
		Check(TEXT("the original comes back"), F1Url(Recovered) == ORIGINAL);
		Check(TEXT("a recovered original becomes available again"), !S47J::Bool(S47J::Obj(S47J::Obj(S47J::Obj(Recovered,TEXT("case")),TEXT("s")),TEXT("f1")),TEXT("unavailable")));
		// a new photograph under the same id replaces the one that could not be read
		TSharedRef<FS47SaveStore> Third = Open(D);
		Third->FailReads = 2;
		const S47J::FObj Lost = Third->Load(IdOf(M));
		S47J::Obj(S47J::Obj(S47J::Obj(Lost, TEXT("case")), TEXT("s")), TEXT("f1"))->SetStringField(TEXT("url"), SECOND);
		Third->Save(1, TEXT("manual"), 1, Lost, Card, Err);
		Check(TEXT("a new print replaces a lost one"), F1Url(Open(D)->Load(TEXT("1:manual:1"))) == SECOND);
	}
	IFileManager::Get().DeleteDirectory(*Root, false, true);
	return bOk;
}

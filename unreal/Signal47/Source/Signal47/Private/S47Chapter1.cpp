// story/Chapter1.ts: chapter one, "The Second Exposure"
#include "S47Chapter1.h"
#include "S47Keys.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformFileManager.h"
#include "InputCoreTypes.h"
#include "Misc/Base64.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "S47Audio.h"
#include "S47Drawings.h"
#include "S47HUD.h"
#include "S47Image.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Panels.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47Yard.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Chapter1, Log, All);

namespace
{
	const double MARK_R1 = 0.095, MARK_R2 = 0.115; // how close a click must be to a reference, in print UV
	const double VANE_TIME = 1.8, TANK_TIME = 2.2, FIX_TIME = 1.5;

	FString ClockText(double Sec, bool bSeconds = true) { return S47Text::ClockText(Sec, bSeconds); }

	// core/hints.ts firstTime: a hint is said once on this machine (the web version keeps the list
	// in localStorage 's47.hints'; here a small file under Saved)
	bool FirstTime(const FString& Id)
	{
		static TSet<FString> Said;
		static bool bLoaded = false;
		const FString Path = FPaths::ProjectSavedDir() / TEXT("S47Hints.json");
		if (!bLoaded)
		{
			bLoaded = true;
			FString Text;
			TArray<TSharedPtr<FJsonValue>> A;
			if (FFileHelper::LoadFileToString(Text, *Path) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), A))
			{
				for (const TSharedPtr<FJsonValue>& V : A) { FString S; if (V.IsValid() && V->TryGetString(S)) Said.Add(S); }
			}
		}
		if (Said.Contains(Id)) return false;
		Said.Add(Id);
		TArray<TSharedPtr<FJsonValue>> A;
		for (const FString& S : Said) A.Add(MakeShared<FJsonValueString>(S));
		FString Out;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> W = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
		if (FJsonSerializer::Serialize(A, W)) FFileHelper::SaveStringToFile(Out, *Path); // (not kept: no matter)
		return true;
	}

	FS47Sound Snd(float Gain, float Rate = 1.f, TOptional<FVector> At = {})
	{
		FS47Sound O;
		O.Gain = Gain; O.Rate = Rate; O.At = At;
		return O;
	}

	// [d.page, d.transcript].filter(Boolean).join('\n\n')
	FString Full(const FS47Doc& D)
	{
		TArray<FString> P;
		if (!D.Page.IsEmpty()) P.Add(D.Page);
		if (!D.Transcript.IsEmpty()) P.Add(D.Transcript);
		return FString::Join(P, TEXT("\n\n"));
	}
}

// ---------- CaseState ----------

FS47CaseState FS47CaseState::Fresh(double EventClock)
{
	FS47CaseState S;
	S.EventClock = EventClock;
	return S;
}

TSharedPtr<FJsonObject> FS47CaseState::ToJson() const
{
	using namespace S47J;
	FObj O = New();
	Set(O, TEXT("v"), V);
	Set(O, TEXT("stage"), Stage);
	Set(O, TEXT("eventClock"), EventClock);
	Set(O, TEXT("camera"), bCamera); Set(O, TEXT("door"), bDoor); Set(O, TEXT("labDoor"), bLabDoor); Set(O, TEXT("log"), bLog); Set(O, TEXT("returned"), bReturned);
	if (F1) Set(O, TEXT("f1"), F1->ToJson());
	if (F2) Set(O, TEXT("f2"), F2->ToJson());
	Set(O, TEXT("dev1"), Dev1); Set(O, TEXT("dev2"), Dev2);
	SetUv(O, TEXT("mark1"), Mark1);
	Set(O, TEXT("ref"), bRef);
	SetOrNull(O, TEXT("hyp"), Hyp);
	SetOrNull(O, TEXT("method"), Method);
	Set(O, TEXT("observed"), bObserved);
	SetUv(O, TEXT("mark2"), Mark2);
	Set(O, TEXT("concluded"), bConcluded); Set(O, TEXT("filed"), bFiled);
	Set(O, TEXT("wrongRef"), WrongRef); Set(O, TEXT("wrongHyp"), WrongHyp); Set(O, TEXT("wrongConcl"), WrongConcl); Set(O, TEXT("rejected"), Rejected);
	if (Sheet) Set(O, TEXT("sheet"), *Sheet);
	if (Links) Set(O, TEXT("links"), *Links);
	// (Unreal only: prints fogged at the lab door; not there when none were, as the web version writes it)
	if (Fog1 > 0) Set(O, TEXT("fog1"), Fog1);
	if (Fog2 > 0) Set(O, TEXT("fog2"), Fog2);
	return O;
}

FS47CaseState FS47CaseState::FromJson(const TSharedPtr<FJsonObject>& O, double EventClock)
{
	using namespace S47J;
	FS47CaseState S = Fresh(EventClock);
	if (!O.IsValid()) return S;
	S.Stage = Str(O, TEXT("stage"), S.Stage);
	S.EventClock = Num(O, TEXT("eventClock"), S.EventClock);
	S.bCamera = Bool(O, TEXT("camera")); S.bDoor = Bool(O, TEXT("door")); S.bLabDoor = Bool(O, TEXT("labDoor")); S.bLog = Bool(O, TEXT("log")); S.bReturned = Bool(O, TEXT("returned"));
	S.F1 = FS47Photo::FromJson(Obj(O, TEXT("f1")));
	S.F2 = FS47Photo::FromJson(Obj(O, TEXT("f2")));
	S.Dev1 = Int(O, TEXT("dev1")); S.Dev2 = Int(O, TEXT("dev2"));
	S.Mark1 = Uv(O, TEXT("mark1"));
	S.bRef = Bool(O, TEXT("ref"));
	S.Hyp = Str(O, TEXT("hyp"));
	S.Method = Str(O, TEXT("method"));
	S.bObserved = Bool(O, TEXT("observed"));
	S.Mark2 = Uv(O, TEXT("mark2"));
	S.bConcluded = Bool(O, TEXT("concluded")); S.bFiled = Bool(O, TEXT("filed"));
	S.WrongRef = Int(O, TEXT("wrongRef")); S.WrongHyp = Int(O, TEXT("wrongHyp")); S.WrongConcl = Int(O, TEXT("wrongConcl")); S.Rejected = Int(O, TEXT("rejected"));
	bool bSheet = false;
	if (O->TryGetBoolField(TEXT("sheet"), bSheet)) S.Sheet = bSheet;
	if (O->HasTypedField<EJson::Array>(TEXT("links"))) S.Links = Strs(O, TEXT("links"));
	S.Fog1 = Int(O, TEXT("fog1")); S.Fog2 = Int(O, TEXT("fog2"));
	return S;
}

// ---------- deps ----------

US47UI* US47Chapter1::Ui() const { return M ? M->UI.Get() : nullptr; }
US47Audio* US47Chapter1::Au() const { return M ? M->Audio.Get() : nullptr; }
US47FieldCamera* US47Chapter1::Fcam() const { return M ? M->FieldCamera.Get() : nullptr; }
void US47Chapter1::Toast(const FString& T, float Secs) const { if (US47UI* U = Ui()) U->Toast(T, Secs); }
void US47Chapter1::NoteIt(const FString& T) { if (Host) Host->Note(T); else if (!TestNotes.Contains(T)) TestNotes.Add(T); }
void US47Chapter1::After(double Secs, TFunction<void()> Fn, const FString& Tag) { if (Host) Host->After(Secs, MoveTemp(Fn), Tag); }
double US47Chapter1::Gt() const { return Host ? Host->Gt() : 0.0; }
bool US47Chapter1::InDark(const FString& Text) const
{
	if (!(Dark && Dark())) return false;
	Toast(Text, 2.8f);
	return true;
}

// ---------- construction (the TypeScript constructor, and main.ts's part) ----------

US47Chapter1* US47Chapter1::Register()
{
	if (!M || !Host) return this;
	// main.ts: the field camera, the yard
	M->FieldCamera = NewObject<US47FieldCamera>(M);
	M->FieldCamera->Init(M);
	Yard = NewObject<US47Yard>(this);
	Yard->Init(M);
	Interactables();
	BuildBook();

	TWeakObjectPtr<US47Chapter1> W = this;
	// main.ts input.onKey: C raises and lowers the camera; Space, and Use while it is up, is the
	// shutter (E, a click); Esc or P while it is up lowers it
	M->OnAction(TEXT("camera"), [W]
	{
		if (!W.IsValid() || W->Host->IsCinematic()) return false;
		W->ToggleCamera();
		return true;
	});
	M->OnAction(TEXT("shutter"), [W]
	{
		if (!W.IsValid() || !W->Fcam() || !W->Fcam()->bRaised) return false;
		W->Shutter();
		return true;
	});
	// (Use's keys as bound, S47Keys, and a click; the pause's keys)
	auto UseShutter = [W]
	{
		if (!W.IsValid() || W->Host->IsCinematic() || !W->Fcam() || !W->Fcam()->bRaised) return false;
		W->Shutter(); // the camera is up: Use is the shutter
		return true;
	};
	M->OnAction(TEXT("use"), UseShutter);
	M->OnKey(EKeys::LeftMouseButton, UseShutter);
	M->OnAction(TEXT("pause"), [W]
	{
		if (!W.IsValid() || !W->Fcam() || !W->Fcam()->bRaised) return false;
		W->ToggleCamera();
		return true;
	});
	// main.ts step: nothing is picked or offered while the camera is up
	if (M->Inter) M->Inter->Holds.Add([W] { return W.IsValid() && W->Fcam() && W->Fcam()->bRaised; });

	// Prologue.jumpChapter1: everything the prologue leaves behind, then chapter one (fresh or
	// from the saved case)
	Host->Jumps.Add(TEXT("chapter1"), [W]
	{
		if (!W.IsValid()) return;
		AS47Story* H = W->Host;
		const S47J::FObj Saved = H->LoadCase ? H->LoadCase() : S47J::FObj();
		H->AfterPrologue(Saved);
		H->BeginChapter(TEXT("ch1"), Saved);
	});

	// main.ts ch1.onEnd: Ward has the report, and the night goes on in the records room (if SARO
	// is going dark just then, the chapter waits for the light: that comes with story/saroDark.ts;
	// the card's ring with chapter two)
	OnEnd = [W](const FString& Method)
	{
		if (!W.IsValid()) return;
		AS47Story* H = W->Host;
		const FString Recap = FString::Printf(TEXT("%s The eye saw one reference; the film kept two. One line on the receiver print is still unexplained: -39 LY."),
			Method == TEXT("passive") ? TEXT("The shielded lamp did not explain it.") : TEXT("Simple encoder drift did not explain it."));
		if (H->Chapter(TEXT("ch2")))
		{
			TWeakObjectPtr<AS47Story> Hw = H;
			H->ChapterBreak({ TEXT("LOCAL INCIDENT S-03 / B-12 // FILED"), Recap, TEXT("CHAPTER TWO"), TEXT("THE AMENDED RECORD"), [Hw] { if (Hw.IsValid()) Hw->BeginChapter(TEXT("ch2")); } });
			return;
		}
		// (until chapter two is ported: the card the prologue ends on without a chapter)
		if (W->Ui()) W->Ui()->Fade(true, TEXT(""));
		H->After(1.6, [W]
		{
			if (!W.IsValid()) return;
			W->Host->SetCinematic(true);
			if (W->M->Hud) W->M->Hud->EndCard({ TEXT("CHAPTER ONE: THE SECOND EXPOSURE"), TEXT(""), TEXT("Chapter 2, The Amended Record, is not in the Unreal version yet."), TEXT("The web version plays on from here.") });
		});
	};
	return this;
}

void US47Chapter1::BeginDestroy()
{
	if (M && BookPtr) M->Books.Remove(BookPtr.Get());
	Super::BeginDestroy();
}

void US47Chapter1::BuildBook()
{
	TMap<FString, FS47BookNote> Notes;
	Notes.Add(TEXT("n.ch1.ref"), { TEXT("The marked foreground detail matches B-12 and the S-03 sight line, not R-07."), TEXT("P01") });
	Notes.Add(TEXT("n.ch1.light"), { TEXT("Test reflected work light by shielding the lamp while leaving the motor isolated."), TEXT("P02") });
	Notes.Add(TEXT("n.ch1.drift"), { TEXT("Test reference/encoder drift with a commanded move to a known heading."), TEXT("P02") });
	Notes.Add(TEXT("n.ch1.retain"), { TEXT("Both exposures keep a second pale mark at the earlier alignment."), TEXT("P03") });
	Notes.Add(TEXT("n.ch1.control"), { TEXT("The eye saw one physical stripe. The control changed the vane; the film kept two."), TEXT("P03") });
	auto Rule = [](const TCHAR* A, const TCHAR* B, const TCHAR* Note, const FString& Text, const TCHAR* Wrong = TEXT(""), const TCHAR* Group = TEXT(""), TFunction<bool()> Restore = nullptr)
	{
		FS47BookRule R;
		R.A = A; R.B = B; R.Note = Note; R.Text = Text; R.Wrong = Wrong; R.Group = Group; R.Restore = MoveTemp(Restore);
		return R;
	};
	TArray<FS47BookRule> Rules = {
		Rule(TEXT("frame01.detail"), TEXT("refsheet.b12"), TEXT("n.ch1.ref"), Notes[TEXT("n.ch1.ref")].Text),
		Rule(TEXT("frame01.detail"), TEXT("refsheet.r07"), TEXT(""), TEXT("R-07 has THREE HORIZONTAL BARS. Compare that silhouette with the vertical feature you marked. The sheet does not place R-07 on this sightline."), TEXT("P01")),
		Rule(TEXT("hyp.light"), TEXT("refsheet.lamp"), TEXT("n.ch1.light"), Notes[TEXT("n.ch1.light")].Text, TEXT(""), TEXT("ch1.hyp"), [this] { return S.Hyp == TEXT("light"); }),
		Rule(TEXT("hyp.drift"), TEXT("refsheet.drive"), TEXT("n.ch1.drift"), Notes[TEXT("n.ch1.drift")].Text, TEXT(""), TEXT("ch1.hyp"), [this] { return S.Hyp == TEXT("drift"); }),
		Rule(TEXT("hyp.command"), TEXT("s03log.zero"), TEXT(""), TEXT("The S-03 log records ZERO commands during the first movement. A second command does not explain this record. Test light or the local reference instead."), TEXT("P02")),
		Rule(TEXT("frame01.extra"), TEXT("frame02.extra"), TEXT("n.ch1.retain"), Notes[TEXT("n.ch1.retain")].Text),
		Rule(TEXT("observation.stripe"), TEXT("frame02.detail"), TEXT("n.ch1.control"), Notes[TEXT("n.ch1.control")].Text),
		// (its text is a getter, lampReply(), in the TypeScript; the wrong reply below always
		// answers for it, so the rule's own text is never what is shown)
		Rule(TEXT("frame02.extra"), TEXT("observation.lamp"), TEXT(""), LampReply(), TEXT("P03")),
		Rule(TEXT("frame02.extra"), TEXT("s03log.zero"), TEXT(""), TEXT("The first S-03 movement had no command. The control used a different local reference. These records do not support two antenna commands."), TEXT("P03")),
	};
	FS47BookSpec B;
	B.Key = TEXT("ch1");
	B.Owns = [](const FString& N)
	{
		// /^(frame0[12]|refsheet|s03log|hyp|observation)\./ or /^(P01|P02|P03)$/
		for (const TCHAR* P : { TEXT("frame01."), TEXT("frame02."), TEXT("refsheet."), TEXT("s03log."), TEXT("hyp."), TEXT("observation.") }) if (N.StartsWith(P, ESearchCase::CaseSensitive)) return true;
		return N.Equals(TEXT("P01"), ESearchCase::CaseSensitive) || N.Equals(TEXT("P02"), ESearchCase::CaseSensitive) || N.Equals(TEXT("P03"), ESearchCase::CaseSensitive);
	};
	B.Notes = Notes;
	B.Rules = Rules;
	{
		FS47BookQuestion Q;
		Q.Id = TEXT("P01"); Q.Stamp = TEXT("P01 / REFERENCE"); Q.Text = TEXT("Which installation is the marked foreground detail?"); Q.Slots = 1; Q.X = 590; Q.Y = 230; Q.W = 272;
		Q.Open = [this] { return S.Dev1 >= 3 || S.bRef; };
		Q.Done = [this] { return S.bRef ? FString(TEXT("B-12 selected. Its installation sheet specifies ONE ivory stripe on a movable vane.")) : FString(); };
		Q.Record = [this] { return MatchReference(TEXT("B-12")); };
		Q.Hints = {
			TEXT("Compare a manufactured feature on CONTACT PRINT 01 with the installation sheet."),
			TEXT("Count the stripes and check the sight line from the S-03 apron."),
			TEXT("B-12 has a movable vane and one stripe. R-07 has three bars on the west fence."),
			TEXT("Mark CONTACT PRINT 01 at the archive bench, then thread its marked detail to B-12 on the installation sheet.") };
		B.Questions.Add(Q);
	}
	{
		FS47BookQuestion Q;
		Q.Id = TEXT("P02"); Q.Stamp = TEXT("P02 / HYPOTHESIS"); Q.Text = TEXT("What could put an extra stripe on the film, and how can you test it?"); Q.Slots = 1; Q.X = 888; Q.Y = 300; Q.W = 272;
		Q.Open = [this] { return S.bRef; };
		Q.Done = [this] { return S.Hyp.IsEmpty() ? FString() : FString::Printf(TEXT("Working hypothesis: %s. The field test is at B-12."), S.Hyp == TEXT("light") ? TEXT("reflected work light") : TEXT("reference/encoder drift")); };
		Q.Record = [this] { return ChooseHypothesis(S.Links && S.Links->Contains(TEXT("hyp.drift|refsheet.drive")) ? TEXT("drift") : TEXT("light")); };
		Q.Hints = {
			TEXT("Choose a cause that a local control can test."),
			TEXT("The lamp and the reference drive have independent controls."),
			TEXT("Stray light can be tested by shielding the lamp; drift by moving the reference to a known heading."),
			TEXT("Thread stray light to the independent lamp, or encoder drift to the local reference drive. The last of these threads is the working hypothesis.") };
		B.Questions.Add(Q);
	}
	{
		FS47BookQuestion Q;
		Q.Id = TEXT("P03"); Q.Doc = TEXT("finding"); Q.Stamp = TEXT("P03 / TWO EXPOSURES"); Q.Text = TEXT("What do the two exposures and the direct control observation support?"); Q.Slots = 2; Q.X = 590; Q.Y = 410; Q.W = 272;
		Q.Open = [this] { return S.Dev2 >= 3 || S.bConcluded; };
		Q.Done = [this] { return S.bConcluded ? FString(TEXT("The physical reference changed. The film retains a second reference. File both prints at the records desk.")) : FString(); };
		Q.Record = [this] { return Conclude(TEXT("second")); };
		Q.Hints = {
			TEXT("Mark the same assembly in FRAME 02 at the archive bench, then compare the prints."),
			TEXT("What changed under the control, and what kept its earlier alignment?"),
			TEXT("The direct observation saw one physical stripe. Both prints keep a second mark."),
			TEXT("Thread the extra marks in FRAME 01 and FRAME 02 together, and the one physical stripe in the observation to the changed vane in FRAME 02.") };
		B.Questions.Add(Q);
	}
	B.Links = [this]() -> TArray<FString>& { if (!S.Links) S.Links.Emplace(); return *S.Links; };
	B.Wrong = [this](const FString& Q, const FS47BookRule& R) -> TOptional<FS47Result>
	{
		if (Q == TEXT("P01")) return MatchReference(TEXT("R-07"));
		if (Q == TEXT("P02")) return ChooseHypothesis(TEXT("command"));
		return Conclude(R.B == TEXT("observation.lamp") ? TEXT("lamp") : TEXT("commands"));
	};
	B.Save = [this] { Save(); };
	B.Cards = [this] { return BoardCards(); };
	BookPtr = MakeUnique<FS47RuleBook>(MoveTemp(B));
}

// ---------- lifecycle ----------

void US47Chapter1::Begin(const S47J::FObj& Saved)
{
	// Prologue.beginChapter1(saved). (AS47Story::BeginChapter has set the phase, ended any
	// cinematic and cleared the prologue's timers: nothing from the prologue may fire inside the
	// chapter.) A new or restored chapter one; chapter two begins after it.
	S47J::FObj Case = Saved, State = S47J::Obj(Saved, TEXT("s"));
	if (!State.IsValid() && S47J::Has(Saved, TEXT("stage"))) { State = Saved; Case = nullptr; }
	if (Host)
	{
		if (US47Chapter* C2 = Host->Chapter(TEXT("ch2"))) C2->Reset();
		Host->SetCinematic(false);
		if (Saved.IsValid())
		{
			if (Case.IsValid())
			{
				Host->NotesMutable() = S47J::Strs(Case, TEXT("notes"));
				Host->SetClock(S47J::Num(Case, TEXT("clock"), Host->Clock()));
			}
		}
		else if (M && M->Player) M->Player->Place(3.4, 1.4, -PI / 2);
	}
	if (US47UI* U = Ui()) U->Fade(false);
	BeginCase(State, S47J::Has(State, TEXT("eventClock")) ? S47J::Num(State, TEXT("eventClock")) : (Host ? Host->EventClockSec() : 0.0));
	if (Host && Host->OnCheckpoint) Host->OnCheckpoint(TEXT("chapter1"));
	static bool bDriven = false;
	FString Stop;
	if (!bDriven && FParse::Value(FCommandLine::Get(), TEXT("S47Ch1Drive="), Stop)) { bDriven = true; TestDrive(Stop); }
}

void US47Chapter1::BeginCase(const S47J::FObj& Saved, double InEventClock)
{
	// Chapter1.begin
	Reset();
	Active = true;
	S = Saved.IsValid() ? FS47CaseState::FromJson(Saved, InEventClock) : FS47CaseState::Fresh(InEventClock);
	bool bSheet = false;
	S.Sheet = Saved.IsValid() && Saved->TryGetBoolField(TEXT("sheet"), bSheet) ? bSheet : S.bRef;
	if (M && BookPtr) M->Books.AddUnique(BookPtr.Get()); // the board reads the books of the chapters that have started
	ApplyWorld();
	// the motor bus cabinet hums; so does the safelight ballast in the lab
	if (US47Audio* A = Au())
	{
		A->Hum(TEXT("s03"), FVector(12.3, 1.0, -11.0), 0.07f, 60.f);
		A->Hum(TEXT("safelight"), FVector(15.0, 2.4, -5.6), 0.025f, 120.f);
	}
	if (!Saved.IsValid())
	{
		NoteIt(TEXT("The array left its track. Check the local motor controller in the east service yard."));
		Toast(TEXT("Service access released. East door."), 3.6f);
		Save();
	}
}

void US47Chapter1::Reset()
{
	Active = false;
	if (US47Audio* A = Au()) A->StopHums();
	S = FS47CaseState::Fresh(0);
	Busy.Reset(); bSettled = false; AnimVane.Reset(); AnimFix.Reset();
	++DevGen; Darkroom(false);
	if (US47FieldCamera* F = Fcam()) { F->Raise(false); F->bHave = false; }
	if (US47UI* U = Ui()) { U->Viewfinder(false); U->CameraButton(false); U->Objective(FString()); }
	ShowCamera(true);
	if (Yard)
	{
		Yard->SetVane(0); Yard->SetWorkLamp(1);
		Yard->WetPrint.SetVisible(false);
		for (FS47Obj& P : Yard->DryPrints) P.SetVisible(false);
	}
	if (M && BookPtr) M->Books.Remove(BookPtr.Get());
}

void US47Chapter1::ApplyWorld()
{
	// Puts the world in the state described by S (after begin or a restore).
	ShowCamera(!S.bCamera);
	if (US47FieldCamera* F = Fcam()) F->bHave = S.bCamera;
	if (US47UI* U = Ui()) U->CameraButton(S.bCamera && Touch());
	// a case from before the doors were free remembers which of its doors were open
	if (M && M->Web)
	{
		if (S.bDoor) M->Web->SetDoorOpen(TEXT("east"), true, true);
		if (S.bLabDoor) M->Web->SetDoorOpen(TEXT("lab"), true, true);
		bEastOpen = M->Web->IsDoorOpen(TEXT("doorEast"));
		bLabOpen = M->Web->IsDoorOpen(TEXT("labDoor"));
	}
	if (!S.Method.IsEmpty())
	{
		if (Yard)
		{
			Yard->SetVane(S.Method == TEXT("passive") ? -60 : 45);
			Yard->SetWorkLamp(S.Method == TEXT("passive") ? 0 : 1.35);
		}
		bSettled = true;
	}
	if (S.F1 && S.Dev1 >= 3) HangPrint(0, *S.F1);
	if (S.F2 && S.Dev2 >= 3) HangPrint(1, *S.F2);
	// documents filed so far
	if (S.bLog) File(DocS03());
	if (S.F1 && S.Dev1 >= 3) File(DocPhoto(*S.F1));
	if (S.Sheet.Get(false) || S.bRef) File(DocSheet());
	if (S.bObserved) File(DocObservation());
	if (S.F2 && S.Dev2 >= 3) File(DocPhoto(*S.F2));
	if (S.bConcluded) File(DocFinding());
}

void US47Chapter1::ShowCamera(bool bOn)
{
	// Hidden objects still catch raycasts in three.js, so the collected camera leaves layer 0; here
	// its meshes stop answering the line of sight.
	if (!M || !M->Web) return;
	const FS47Obj Cam = M->Web->Obj(TEXT("room.fieldCamera"));
	Cam.Traverse([bOn](FS47Obj O)
	{
		const FS47WebNode* N = O.W->Node(O.I);
		if (UPrimitiveComponent* P = N ? Cast<UPrimitiveComponent>(N->Comp.Get()) : nullptr) P->SetCollisionEnabled(bOn ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	});
	FS47Obj(Cam).SetVisible(bOn);
}

void US47Chapter1::Save()
{
	if (SaveHook) SaveHook();
	if (M && M->Board.Refresh) M->Board.Refresh();
}

bool US47Chapter1::File(const FS47Doc& Doc)
{
	if (!Host) return true;
	TArray<FS47Doc>& Docs = Host->DocsMutable();
	const int32 I = Docs.IndexOfByPredicate([&](const FS47Doc& X) { return X.Id == Doc.Id; });
	if (I >= 0) { Docs[I] = Doc; return false; }
	Docs.Add(Doc);
	return true;
}

void US47Chapter1::StageTo(const FString& St) { S.Stage = St; Save(); }

// ---------- objective line and notebook ----------

FString US47Chapter1::Objective() const
{
	const FString LabName = TEXT("PHOTO LAB");
	const FString& St = S.Stage;
	if (St == TEXT("collect-camera")) return TEXT("FIELD KIT // COLLECT THE CAMERA BESIDE THE EAST DOOR");
	if (St == TEXT("motor-log")) return TEXT("S-03 // READ THE MOTOR CABINET IN THE EAST SERVICE YARD");
	if (St == TEXT("first-exposure")) return Touch() ? TEXT("S-03 APRON // CAMERA, THEN SHUTTER: EXPOSE THE CENTRAL ANTENNA") : TEXT("S-03 APRON // C: CAMERA / SPACE: EXPOSE THE CENTRAL ANTENNA");
	if (St == TEXT("develop-first") || St == TEXT("develop-second"))
	{
		const int32 Dev = St == TEXT("develop-first") ? S.Dev1 : S.Dev2;
		if (Busy == TEXT("tank")) return LabName + TEXT(" // THE FILM IS IN THE TANK; FINISH THE PROCESS AT THE BENCH");
		if (Busy == TEXT("fix")) return LabName + TEXT(" // THE PRINT IS IN THE FIXER");
		// (Unreal only) the darkroom's door
		if (Dev <= 1 && bLabOpen) return LabName + TEXT(" // DARKROOM: SHUT THE DOOR BEFORE DEVELOPING");
		if (Dev == 0 && (St == TEXT("develop-first") ? S.Fog1 : S.Fog2) > 0) return LabName + TEXT(" // THE PRINT FOGGED: SHUT THE DOOR, PRINT AGAIN FROM THE NEGATIVE");
		if (Dev == 0) return St == TEXT("develop-first") ? TEXT("SEALED FILM // ") + LabName + TEXT(" EAST OF THE WALK / PROCESS AT THE WET BENCH") : TEXT("CONTROL FILM // ") + LabName + TEXT(" / PROCESS AT THE WET BENCH");
		if (Dev == 1) return LabName + TEXT(" // TRANSFER THE CONTACT PRINT TO THE FIXER");
		return LabName + TEXT(" // COLLECT THE PROCESSED PRINT");
	}
	if (St == TEXT("interpret-first"))
	{
		if (!S.Mark1) return TEXT("FRAME 01 // INSPECT THE PRINT AND MARK A COMPARABLE DETAIL");
		if (!S.bRef) return LabName + TEXT(" ARCHIVE // READ THE REFERENCE SHEET AND MATCH THE DETAIL ON THE CASE BOARD");
		return TEXT("CASE BOARD // RECORD A TESTABLE EXPLANATION");
	}
	if (St == TEXT("choose-control")) return TEXT("B-12 // FOLLOW THE EAST WALK NORTH TO THE LOCAL REFERENCE CONTROL");
	if (St == TEXT("observe-control")) return bSettled ? TEXT("B-12 // NOTE THE DIRECT OBSERVATION AFTER THE VANE SETTLES") : TEXT("B-12 // WAIT FOR THE REFERENCE VANE TO SETTLE");
	if (St == TEXT("second-exposure")) return Touch() ? TEXT("B-12 // FROM THE SIGHT LINE MARK: CAMERA, SHUTTER") : TEXT("B-12 // C: CAMERA / SPACE: RECORD THE CONTROLLED REFERENCE");
	if (St == TEXT("compare-exposures")) return S.Mark2 ? TEXT("CASE BOARD // COMPARE THE PRINTS WITH THE CONTROL OBSERVATION AND RECORD THE FINDING") : TEXT("TWO PRINTS // MARK THE SAME REFERENCE IN FRAME 02 AT THE ARCHIVE BENCH");
	if (St == TEXT("file-report")) return LabName + TEXT(" // FILE THE TWO-EXPOSURE REPORT AT THE RECORDS DESK");
	if (St == TEXT("complete")) return TEXT("LOCAL CASE FILED // THE SECOND EXPOSURE");
	return FString();
}

TArray<FS47Task> US47Chapter1::Tasks() const
{
	const TArray<FS47Task> All = {
		{ TEXT("Collect the field camera by the east door"), S.bCamera },
		{ TEXT("Read the S-03 motor bus in the service yard"), S.bLog },
		{ TEXT("Expose frame 01 from the S-03 apron"), S.F1.IsSet() },
		{ TEXT("Develop frame 01 in the photo lab"), S.Dev1 >= 3 },
		{ TEXT("Mark a detail you can check at the site"), S.Mark1.IsSet() },
		{ TEXT("Match it on the reference sheet"), S.bRef },
		{ TEXT("Choose an explanation you can test"), !S.Hyp.IsEmpty() },
		{ TEXT("Run a control at B-12"), !S.Method.IsEmpty() },
		{ TEXT("Note what you see at B-12"), S.bObserved },
		{ TEXT("Expose frame 02 from the sight line"), S.F2.IsSet() },
		{ TEXT("Develop frame 02"), S.Dev2 >= 3 },
		{ TEXT("Compare the two exposures"), S.bConcluded },
		{ TEXT("File the local report"), S.bFiled },
	};
	const int32 Next = All.IndexOfByPredicate([](const FS47Task& T) { return !T.bDone; });
	if (Next < 0) return All;
	TArray<FS47Task> Out;
	for (int32 i = 0; i <= Next; ++i) Out.Add(All[i]);
	return Out;
}

// ---------- interactables ----------

void US47Chapter1::Interactables()
{
	if (!M || !M->Inter || !M->Web) return;
	US47Inter* Inter = M->Inter;
	AS47Web* Web = M->Web;
	auto Pos = [](const FS47Obj& O) { return O.WorldPosition(); };

	{
		FS47Interactable It;
		It.Id = TEXT("fieldCamera"); It.Object = Web->Obj(TEXT("room.objs.fieldCamera")); It.Range = 2.6;
		It.Label = [this] { return !Active || S.bCamera ? FString() : FString(TEXT("Collect field camera")); };
		It.Use = [this]
		{
			S.bCamera = true;
			ShowCamera(false);
			if (US47FieldCamera* F = Fcam()) F->bHave = true;
			if (US47UI* U = Ui()) U->CameraButton(Touch());
			if (US47Audio* A = Au()) A->Sfx(TEXT("click"), Snd(0.6f));
			NoteIt(TEXT("Collected the field camera. Expose the antenna profile from S-03, then process the film in the photo lab."));
			Toast(!FirstTime(TEXT("camera")) ? TEXT("Field camera.") : Touch() ? TEXT("Field camera. The Camera button raises it.") : FString::Printf(TEXT("Field camera. %s to raise, %s to expose."), *S47Keys::Said(TEXT("camera")), *S47Keys::Said(TEXT("shutter"))), 3.6f);
			if (S.Stage == TEXT("collect-camera")) StageTo(S.bLog ? TEXT("first-exposure") : TEXT("motor-log")); else Save();
		};
		Inter->Add(It);
	}
	{
		FS47Interactable It;
		It.Id = TEXT("motorBus"); It.Object = Yard->Obj(TEXT("motorBus")); It.Range = 2.4;
		It.Label = [this] { return !Active ? FString() : S.bLog ? FString(TEXT("Read motor bus S-03")) : FString(TEXT("Check motor bus S-03")); };
		It.Use = [this, Pos]
		{
			if (InDark(TEXT("The inspection glass is dark. No power to the cabinet."))) return;
			if (US47Audio* A = Au()) A->Sfx(TEXT("click"), Snd(0.5f, 1.f, Pos(Yard->Obj(TEXT("motorBus")))));
			const FS47Doc Doc = DocS03();
			if (US47UI* U = Ui()) U->Document(Doc, [this, Doc]
			{
				if (S.bLog) return;
				S.bLog = true;
				File(Doc);
				NoteIt(TEXT("Filed: S-03 / No command recorded."));
				NoteIt(TEXT("S-03 registered a physical heading change without a motor command."));
				if (S.Stage == TEXT("motor-log")) StageTo(TEXT("first-exposure"));
				else if (S.Stage == TEXT("collect-camera")) { Save(); Toast(TEXT("The field camera is still on the shelf by the east door.")); }
				else Save();
			});
		};
		Inter->Add(It);
	}
	{
		FS47Interactable It;
		It.Id = TEXT("wetBench"); It.Object = Yard->Obj(TEXT("wetBench")); It.Range = 2.6;
		It.Label = [this] { return BenchLabel(); };
		It.Use = [this] { if (!InDark(TEXT("Too dark to work at the bench. Wait for the light."))) UseWetBench(); };
		Inter->Add(It);
	}
	{
		// (Unreal only, on Tom's request) the darkroom's card on the lab door: the doors are labelled by
		// the story (AS47Story::Label, through the fallback); while chapter one runs, the lab door says
		// what light does in there
		const TFunction<FString(const FString&)> Prev = Inter->FallbackLabel;
		const TWeakObjectPtr<US47Chapter1> Self = this;
		Inter->FallbackLabel = [Prev, Self](const FString& Id)
		{
			FString L = Prev ? Prev(Id) : FString();
			if (Id == TEXT("labDoor") && !L.IsEmpty() && Self.IsValid() && Self->Active && !Self->bLabOpen)
				L += Self->Developing() ? TEXT(" (darkroom in use: light will fog the print)") : TEXT(" (darkroom: keep the door shut while developing)");
			return L;
		};
	}
	{
		FS47Interactable It;
		It.Id = TEXT("archive"); It.Object = Yard->Obj(TEXT("archive")); It.Range = 2.4;
		It.Label = [this]
		{
			if (!Active) return FString();
			if (S.Stage == TEXT("interpret-first")) return FString(!S.Mark1 ? TEXT("Examine contact print") : TEXT("Reference file"));
			if (S.Stage == TEXT("compare-exposures")) return FString(TEXT("Compare contact prints"));
			if (S.F1 && S.Dev1 >= 3) return FString(TEXT("Reference file"));
			return FString(TEXT("Archive bench"));
		};
		It.Use = [this] { if (!InDark(TEXT("Too dark to see the prints. Wait for the light."))) UseArchive(); };
		Inter->Add(It);
	}
	{
		FS47Interactable It;
		It.Id = TEXT("b12Control"); It.Object = Yard->Obj(TEXT("b12Control")); It.Range = 2.4;
		It.Label = [this]
		{
			if (!Active) return FString();
			if (S.Stage == TEXT("observe-control")) return bSettled ? FString(TEXT("Note the direct observation")) : FString();
			if (!S.Method.IsEmpty()) return FString(TEXT("B-12 / review control settings"));
			return FString(TEXT("B-12 / local reference control"));
		};
		It.Use = [this] { if (!InDark(TEXT("The B-12 control is dead. No power."))) UseB12(); };
		Inter->Add(It);
	}
	{
		FS47Interactable It;
		It.Id = TEXT("vane"); It.Object = Yard->Obj(TEXT("vane")); It.Range = 3.2;
		It.Label = [this] { return Active ? FString(TEXT("B-12 reference vane")) : FString(); };
		It.Use = [this]
		{
			Toast(S.Method == TEXT("passive") ? TEXT("The vane is folded into its shield stop. One stripe. The lamp is dark.")
				: S.Method == TEXT("active") ? TEXT("The vane stands at 042 degrees. One ivory stripe. One fixed bracket.")
				: TEXT("One ivory stripe on a movable vane. The fixed bracket carries the station number."), 4.f);
		};
		Inter->Add(It);
	}
	{
		FS47Interactable It;
		It.Id = TEXT("records"); It.Object = Yard->Obj(TEXT("records")); It.Range = 2.4;
		It.Label = [this] { return !Active ? FString() : S.Stage == TEXT("file-report") ? FString(TEXT("File local investigation")) : S.bFiled ? FString(TEXT("Read filed chapter report")) : FString(TEXT("Night report")); };
		It.Use = [this]
		{
			if (InDark(TEXT("Too dark to read the report. Wait for the light."))) return;
			if (S.Stage == TEXT("file-report") || S.bFiled) S47Panels::LocalReport(Ui(), ReportText(), S.bFiled, [this] { FileReport(); });
			else Toast(TEXT("SARO night report: observation, method, conclusion. Nothing to file yet."));
		};
		Inter->Add(It);
	}
	{
		FS47Interactable It;
		It.Id = TEXT("r07"); It.Object = Yard->Obj(TEXT("r07")); It.Range = 3.5;
		It.Label = [this] { return Active ? FString(TEXT("R-07 marker")) : FString(); };
		It.Use = [this] { Toast(TEXT("R-07. Three horizontal bars on the west fence. Not on the sight line from the apron."), 4.f); };
		Inter->Add(It);
	}
}

// The doors are free (world/Doors.ts); the case remembers which ones were opened.
void US47Chapter1::DoorChanged(const FString& Id, bool bOpen)
{
	if (!Active) return;
	if (Id == TEXT("east") && bOpen && !S.bDoor) { S.bDoor = true; Save(); }
	if (Id == TEXT("lab") && bOpen && !S.bLabDoor) { S.bLabDoor = true; Save(); }
	// (Unreal only) light into the darkroom while a print is in the process
	if (Id == TEXT("lab") && bOpen && Developing()) Fog();
}

// ---------- wet bench ----------

TOptional<US47Chapter1::FDevFrame> US47Chapter1::DevFrame() const
{
	if (S.Stage == TEXT("develop-first") && S.F1) return FDevFrame{ 1, *S.F1 };
	if (S.Stage == TEXT("develop-second") && S.F2) return FDevFrame{ 2, *S.F2 };
	return {};
}

void US47Chapter1::UseWetBench()
{
	if (Lab && Lab()) return;
	const TOptional<FDevFrame> F = DevFrame();
	if (!F)
	{
		S47Panels::WetBench(Ui(), FString(), 0, TEXT("The tank is empty.\nExpose a frame from the S-03 apron before loading film. The archive bench holds the local reference plan."), [] {});
		return;
	}
	if (Busy == TEXT("tank")) { Toast(TEXT("The tank is sealed. Give it a moment.")); return; }
	if (!Busy.IsEmpty()) return;
	const int32 Dev = F->Which == 1 ? S.Dev1 : S.Dev2;
	// (Unreal only, on Tom's request) a darkroom: nothing goes into the process with the door open
	if (Dev <= 1 && bLabOpen) { Toast(TEXT("Darkroom. Keep the door shut while developing. Shut the door first."), 3.4f); return; }
	const bool bAgain = Dev == 0 && (F->Which == 1 ? S.Fog1 : S.Fog2) > 0;
	const FString Label = F->Which == 1 ? TEXT("FRAME 01 / S-03 ARRAY PROFILE") : TEXT("FRAME 02 / B-12 CONTROL");
	const int32 Step = Dev == 0 ? 0 : Dev == 1 ? 2 : 3;
	const int32 Which = F->Which;
	const FS47Photo Photo = F->Photo;
	S47Panels::WetBench(Ui(), Label, Step, FString(), [this, Dev, Which, Photo, bAgain]
	{
		const FVector At = Yard ? Yard->WetPrint.WorldPosition() : FVector::ZeroVector;
		US47Audio* A = Au();
		// (Unreal: the door may have opened while the panel was up)
		if (Dev <= 1 && bLabOpen) { Toast(TEXT("Darkroom. Keep the door shut while developing. Shut the door first."), 3.4f); return; }
		const int32 Gen = DevGen;
		if (Dev == 0)
		{
			Busy = TEXT("tank");
			if (A) A->PourThree(At);
			// (Unreal: the darkroom's rule, and the fogged print thrown out for a new one from the negative)
			if (bAgain) ClearWetPrint();
			Toast(bAgain ? TEXT("Fresh paper under the negative. Keep the door shut until the print is fixed.") : TEXT("Sealed tank loaded. Keep the door shut until the print is fixed."), 3.f);
			After(TANK_TIME, [this, Which, Gen]
			{
				if (Gen != DevGen) return; // (fogged at the door meanwhile)
				Busy.Reset();
				SetDev(Which, 1);
				if (US47Audio* A2 = Au()) A2->Beep(1500, 0.12f, 0.05f);
				Toast(TEXT("Contact print ready. Transfer it to the fixer."), 3.f);
			}, TEXT("lab"));
		}
		else if (Dev == 1)
		{
			Busy = TEXT("fix");
			if (A) A->PourThree(At);
			ShowWetPrint(Photo);
			AnimFix = Gt();
			Toast(TEXT("Print in the fixer. The image is becoming stable."), 3.f);
			After(FIX_TIME, [this, Which, Gen]
			{
				if (Gen != DevGen) return;
				Busy.Reset();
				SetDev(Which, 2);
				if (US47Audio* A2 = Au()) A2->Beep(1500, 0.12f, 0.05f);
				Toast(TEXT("Print fixed. Collect it at the bench."), 3.f);
			}, TEXT("lab"));
		}
		else Collect(Which, Photo);
	});
}

void US47Chapter1::SetDev(int32 Which, int32 V) { if (Which == 1) S.Dev1 = V; else S.Dev2 = V; Save(); }

FString US47Chapter1::BenchLabel() const
{
	if (!Active) return FString();
	const TOptional<FDevFrame> F = DevFrame();
	if (!F) return FString(TEXT("Wet bench"));
	if (Busy == TEXT("tank")) return FString(TEXT("Check processing tank"));
	if (Busy == TEXT("fix")) return FString();
	const int32 Dev = F->Which == 1 ? S.Dev1 : S.Dev2;
	// (Unreal only) the darkroom's door, and a print again from the negative after a fogged one
	if (Dev <= 1 && bLabOpen) return FString(TEXT("Shut the door first"));
	if (Dev == 0 && (F->Which == 1 ? S.Fog1 : S.Fog2) > 0) return FString(TEXT("Print again from the negative"));
	return FString(Dev == 0 ? TEXT("Process sealed film") : Dev == 1 ? TEXT("Transfer print to fixer") : TEXT("Collect processed print"));
}

// ---------- (Unreal only, on Tom's request, 22.10) the darkroom ----------
// "Fotorommet bør ha at døra må være lukket, ellers ødelegger man fotofremkallingen." The web version
// processes in daylight. Here the lab is a darkroom: a print is in the process from the tank until it
// is fixed, the lab's white lamps go out and the safelights come up while it is, nothing goes in with
// the door open, and the door opened on it fogs the print. The negative is kept, so the print can be
// made again, as often as it takes (the case never stops here).

bool US47Chapter1::Developing() const
{
	const TOptional<FDevFrame> F = DevFrame();
	if (!F) return false;
	return Busy == TEXT("tank") || Busy == TEXT("fix") || (F->Which == 1 ? S.Dev1 : S.Dev2) == 1;
}

void US47Chapter1::Fog()
{
	const TOptional<FDevFrame> F = DevFrame();
	if (!F) return;
	const int32 Which = F->Which;
	++DevGen; // (the step's timers find a newer generation and let it be)
	Busy.Reset(); AnimFix.Reset();
	Darkroom(false);
	if (Which == 1) ++S.Fog1; else ++S.Fog2;
	SetDev(Which, 0);
	// the print in the tray as the light left it: white to the edges, grain over what was there
	if (Yard)
	{
		UTexture2D* Tex = FoggedTex(F->Photo);
		SetMap(Yard->WetPrint, Tex ? Tex : TexOf(F->Photo), Tex ? 1.0 : 3.0); // (undecodable: the print itself, washed out)
		Yard->WetPrint.SetVisible(true);
	}
	if (US47Audio* A = Au()) A->Beep(330, 0.4f, 0.04f, 0.2f, TEXT("sine"));
	const FString Frame = Which == 1 ? TEXT("FRAME 01") : TEXT("FRAME 02");
	Toast(FString::Printf(TEXT("Light in the darkroom. The print fogs white to the edges; %s is useless like this. The negative is still good: shut the door and print it again."), *Frame), 5.f);
	NoteIt(FString::Printf(TEXT("Opened the lab door while %s was developing. The print fogged and is useless; the negative is intact, a new print can be made."), *Frame.Replace(TEXT("FRAME"), TEXT("frame"))));
}

void US47Chapter1::Darkroom(bool bOn)
{
	if (bOn == bDarkroom || !M || !M->Web) return;
	AS47Web* W = M->Web;
	const int32 Set = W->FloodSet(TEXT("lab"));
	if (Set < 0) return;
	bDarkroom = bOn;
	// (the lab's pools through the world's light API: the real lights follow their pools. The red ones
	// are the safelights, ServiceYard.ts; the rest are the archive's task lamp and the desk's pool.)
	for (int32 K = 0; K < W->FloodCount(Set); ++K)
	{
		const FLinearColor C = W->FloodCol(Set, K);
		const bool bRed = C.R > 0 && C.G < 0.35f * C.R && C.B < 0.35f * C.R;
		if (bOn)
		{
			const float A = W->FloodPos(Set, K).A;
			LampBase.Add(K, A);
			W->SetFlood(Set, K, bRed ? A * 1.5f : 0.f);
		}
		else if (const float* A = LampBase.Find(K)) W->SetFlood(Set, K, *A);
	}
	if (!bOn) LampBase.Reset();
}

UTexture2D* US47Chapter1::FoggedTex(const FS47Photo& Photo)
{
	const FString Key = Photo.Id + TEXT("#fogged");
	if (TObjectPtr<UTexture2D>* T = Images.Find(Key); T && *T) return *T;
	TArray<FColor> Px;
	int32 W = 0, H = 0;
	if (!S47Image::DecodePixels(Photo.Url, Px, W, H)) return nullptr;
	// a wash of white over all of it, heavier on the side the door's light came from, and the paper's
	// silver gone to grain
	FRandomStream R(int32(GetTypeHash(Photo.Id)));
	for (int32 Y = 0; Y < H; ++Y) for (int32 X = 0; X < W; ++X)
	{
		FColor& C = Px[Y * W + X];
		const double Wash = 0.7 + 0.22 * (1.0 - double(X) / FMath::Max(1, W - 1));
		const double Grain = (R.FRand() - 0.5) * 64;
		auto Ch = [Wash, Grain](uint8 V, double Paper) { return uint8(FMath::Clamp(V + (255 * Paper - V) * Wash + Grain, 0.0, 255.0)); };
		C = FColor(Ch(C.R, 0.97), Ch(C.G, 0.95), Ch(C.B, 0.9), 255);
	}
	UTexture2D* T = S47Image::FromPixels(Px, W, H);
	Images.Add(Key, T);
	return T;
}

void US47Chapter1::Collect(int32 Which, const FS47Photo& Photo)
{
	if (Yard) Yard->WetPrint.SetVisible(false);
	HangPrint(Which - 1, Photo);
	if (US47Audio* A = Au()) A->Sfx(TEXT("thudSoft"), Snd(0.3f, 1.5f));
	if (Which == 1)
	{
		S.Dev1 = 3;
		File(DocPhoto(Photo));
		NoteIt(TEXT("Filed: PHOTO 01 / S-03 contact print."));
		NoteIt(TEXT("Processed frame 01. A contact print is ready for close inspection; the facility sheet is at the archive bench."));
		StageTo(TEXT("interpret-first"));
		OpenPrint1();
	}
	else
	{
		S.Dev2 = 3;
		File(DocPhoto(Photo));
		NoteIt(TEXT("Filed: PHOTO 02 / B-12 control print."));
		NoteIt(TEXT("Processed the control print. Both original exposures are available for comparison."));
		StageTo(TEXT("compare-exposures"));
		Toast(TEXT("Both prints are ready. Compare them at the archive bench."), 3.5f);
		OpenCompare();
	}
}

UTexture2D* US47Chapter1::TexOf(const FS47Photo& Photo)
{
	TObjectPtr<UTexture2D>* T = Images.Find(Photo.Id);
	if (!T || !*T)
	{
		UTexture2D* New = S47Image::FromDataUrl(Photo.Url);
		Images.Add(Photo.Id, New);
		return New;
	}
	return *T;
}

void US47Chapter1::SetMap(const FS47Obj& Mesh, UTexture2D* Tex, double Color)
{
	// (a picture decoded here has its top row first, as a render target: the rows the other way
	// from three's flipped upload, as the CRTs have them)
	Mesh.SetMap(Tex, true);
	Mesh.SetColor(FLinearColor(float(Color), float(Color), float(Color), 1.f)); // color.setScalar(color)
}

void US47Chapter1::ShowWetPrint(const FS47Photo& Photo)
{
	if (!Yard) return;
	SetMap(Yard->WetPrint, TexOf(Photo), 0);
	Yard->WetPrint.SetVisible(true);
}

void US47Chapter1::FixPrint(const FS47Photo& Photo) { ShowWetPrint(Photo); AnimFix = Gt(); }
void US47Chapter1::ClearWetPrint() { if (Yard) Yard->WetPrint.SetVisible(false); }

void US47Chapter1::HangPrint(int32 I, const FS47Photo& Photo)
{
	if (!Yard || !Yard->DryPrints.IsValidIndex(I)) return;
	FS47Obj P = Yard->DryPrints[I];
	SetMap(P, TexOf(Photo), 1);
	P.SetVisible(true);
}

// ---------- the case board ----------

TArray<FS47BoardCard> US47Chapter1::BoardCards() const
{
	if (!Active) return {};
	TArray<FS47BoardCard> Out;
	auto Line = [](const TCHAR* Id, const FString& Text) { FS47BoardLine L; L.Id = Id; L.Text = Text; return L; };
	{
		FS47BoardCard C;
		C.Id = TEXT("frame01"); C.Doc = TEXT("frame01"); C.Stamp = TEXT("FRAME 01"); C.Title = TEXT("S-03 APRON"); C.Kind = S.Dev1 >= 3 ? TEXT("photo") : TEXT("sealed"); C.bHave = S.F1.IsSet();
		C.Where = TEXT("Expose the central antenna from the S-03 apron and process the film in the photo lab."); C.X = 14; C.Y = 14; C.W = 272;
		if (S.Dev1 >= 3 && S.F1) C.Image = S.F1->Url;
		if (S.F1 && S.Dev1 >= 3) C.Full = Full(DocPhoto(*S.F1));
		C.Text = S.Dev1 < 3 ? TEXT("Sealed film. Process it at the wet bench.") : !S.Mark1 ? TEXT("Inspect this print at the archive bench and mark a comparable detail.") : TEXT("");
		if (S.Mark1) C.Lines = {
			Line(TEXT("frame01.detail"), TEXT("Marked foreground detail: a vertical reference vane below the central antenna.")),
			Line(TEXT("frame01.extra"), TEXT("A second pale mark beside the reference stripe.")) };
		Out.Add(C);
	}
	{
		FS47BoardCard C;
		C.Id = TEXT("refsheet"); C.Doc = TEXT("refsheet"); C.Stamp = TEXT("11-86"); C.Title = TEXT("INSTALLATION SHEET"); C.bHave = S.Sheet.Get(false) || S.bRef;
		C.Where = TEXT("Read the reference file at the photo lab archive bench."); C.Full = Full(DocSheet()); C.X = 302; C.Y = 14; C.W = 272;
		C.Lines = {
			Line(TEXT("refsheet.b12"), TEXT("B-12: ONE ivory stripe on a movable vane, on the S-03 sight line.")),
			Line(TEXT("refsheet.r07"), TEXT("R-07: THREE horizontal bars on the west fence, off the sight line.")),
			Line(TEXT("refsheet.lamp"), TEXT("A local work lamp with an independent shield control.")),
			Line(TEXT("refsheet.drive"), TEXT("An isolated reference drive: move the vane to a known heading.")) };
		Out.Add(C);
	}
	{
		FS47BoardCard C;
		C.Id = TEXT("s03log"); C.Doc = TEXT("s03log"); C.Stamp = TEXT("S-03"); C.Title = TEXT("MOTOR BUS"); C.bHave = S.bLog;
		C.Where = TEXT("Read the motor cabinet in the east service yard."); C.Full = Full(DocS03()); C.X = 590; C.Y = 14; C.W = 272;
		C.Lines = {
			Line(TEXT("s03log.zero"), TEXT("COMMANDS RECEIVED: 0. LOCAL OVERRIDE: NONE.")),
			Line(TEXT("s03log.heading"), TEXT("Encoder: 026 degrees. Scheduled: 042 degrees.")) };
		Out.Add(C);
	}
	{
		FS47BoardCard C;
		C.Id = TEXT("hypotheses"); C.Stamp = TEXT("WORKING NOTES"); C.Title = TEXT("A CAUSE TO TEST"); C.Kind = TEXT("thing"); C.bHave = S.bRef;
		C.Where = TEXT("Identify the marked reference before choosing a cause to test."); C.X = 888; C.Y = 14; C.W = 272;
		C.Lines = {
			Line(TEXT("hyp.light"), TEXT("Stray light: a reflected work lamp could add a stripe.")),
			Line(TEXT("hyp.drift"), TEXT("Encoder drift: the local reference could disagree with its heading.")),
			Line(TEXT("hyp.command"), TEXT("A second motor command could explain the extra reference.")) };
		Out.Add(C);
	}
	{
		FS47BoardCard C;
		C.Id = TEXT("frame02"); C.Doc = TEXT("frame02"); C.Stamp = TEXT("FRAME 02"); C.Title = TEXT("B-12 CONTROL"); C.Kind = S.Dev2 >= 3 ? TEXT("photo") : TEXT("sealed"); C.bHave = S.F2.IsSet();
		C.Where = TEXT("Run a B-12 control, note the observation, expose FRAME 02 and develop it."); C.X = 14; C.Y = 392; C.W = 272;
		if (S.Dev2 >= 3 && S.F2) C.Image = S.F2->Url;
		if (S.F2 && S.Dev2 >= 3) C.Full = Full(DocPhoto(*S.F2));
		C.Text = S.Dev2 < 3 ? TEXT("Sealed control film. Process it at the wet bench.") : !S.Mark2 ? TEXT("Mark the same reference in FRAME 02 at the archive bench.") : TEXT("");
		if (S.Mark2) C.Lines = {
			Line(TEXT("frame02.detail"), S.Method == TEXT("passive") ? TEXT("The physical reference vane is folded into its shield stop.") : TEXT("The physical reference vane has turned to the commanded 042 degrees.")),
			Line(TEXT("frame02.extra"), TEXT("The second pale mark keeps the earlier alignment.")) };
		Out.Add(C);
	}
	{
		FS47BoardCard C;
		C.Id = TEXT("observation"); C.Doc = TEXT("observation"); C.Stamp = S.Method == TEXT("active") ? TEXT("ACTIVE") : TEXT("PASSIVE"); C.Title = TEXT("DIRECT OBSERVATION"); C.bHave = S.bObserved;
		C.Where = TEXT("Note what you see at B-12 after the vane settles."); C.Full = Full(DocObservation()); C.X = 302; C.Y = 392; C.W = 272;
		C.Lines = {
			Line(TEXT("observation.stripe"), TEXT("The eye sees ONE physical ivory stripe and one fixed bracket.")),
			Line(TEXT("observation.lamp"), S.Method == TEXT("passive") ? TEXT("Work lamp: shielded. Motor: isolated.") : TEXT("Work lamp: lit. Reference command and encoder: 042 degrees.")) };
		Out.Add(C);
	}
	return Out;
}

// ---------- prints and the archive bench ----------

void US47Chapter1::OpenPrint1()
{
	if (!S.F1) return;
	S47Panels::FPrintView O;
	O.Title = TEXT("CONTACT PRINT 01");
	O.Url = S.F1->Url;
	O.Mark = S.Mark1;
	O.Intro = S.Mark1 ? TEXT("DETAIL MARKED // Now identify this manufactured reference using the facility sheet at the archive bench.") : TEXT("CLICK A DETAIL IN THE PRINT. Find something that can also be checked directly at the facility. Zoom and move the crop if necessary.");
	O.Hints = {
		TEXT("Look for a manufactured reference feature, not the shape of the antenna itself."),
		TEXT("The nearer equipment below the central dish carries a stripe. Its mounting appears in the facility reference sheet."),
		TEXT("The B-12 assembly is in the lower foreground, in front of the dish. Mark its cream stripe and compare the number of stripes with the sheet."),
	};
	O.OnMark = [this](double U, double V) -> FS47Result
	{
		if (!S.F1 || !Near(*S.F1, U, V, MARK_R1)) return { false, TEXT("No comparable reference feature at that point. Try a different manufactured detail, or request a hint.") };
		if (!S.Mark1) { S.Mark1 = FVector2D(U, V); Save(); }
		return { true, TEXT("DETAIL MARKED // Now identify this manufactured reference using the facility sheet at the archive bench.") };
	};
	S47Panels::PrintView(Ui(), O);
}

bool US47Chapter1::Near(const FS47Photo& Photo, double U, double V, double R) const
{
	for (const TPair<FString, FVector2D>& T : Photo.Targets) if (FMath::Sqrt(FMath::Square(T.Value.X - U) + FMath::Square(T.Value.Y - V)) <= R) return true;
	return false;
}

void US47Chapter1::UseArchive()
{
	if (S.Stage == TEXT("interpret-first") && !S.Mark1) { OpenPrint1(); return; }
	if (S.Stage == TEXT("compare-exposures")) { OpenCompare(); return; }
	if (S.F1 && S.Dev1 >= 3) { OpenReference(); return; }
	Toast(TEXT("Archive bench: the facility reference sheet, a loupe and an empty print tray."), 3.5f);
}

void US47Chapter1::OpenReference()
{
	if (File(DocSheet())) NoteIt(TEXT("Filed: B-12 / Reference installation sheet."));
	if (!S.Sheet.Get(false)) { S.Sheet = true; Save(); }
	if (M && M->Board.Open) { M->Board.Open(TEXT("PHOTO LAB"), TEXT("ch1")); return; }
	S47Panels::FReferenceFile O;
	O.Sheet = {
		TEXT("INSTALLATION SHEET 11-86 / LOCAL SURVEY REFERENCES"),
		TEXT("B-12: one ivory stripe on a movable reference vane. The fixed bracket carries the station number. A local lamp and an isolated reference drive allow two different checks."),
		TEXT("R-07: three horizontal bars, on the west fence. It does not lie between the S-03 service apron and the central antenna."),
		TEXT("SIGHTLINE FROM SERVICE APRON: S-03 APRON > B-12 REFERENCE > CENTRAL ANTENNA"),
		TEXT("A contact print can preserve an optical error as well as a physical object. Make a direct observation and change one known condition before trusting a cause."),
	};
	O.bRefDone = S.bRef;
	O.Hyp = S.Hyp;
	O.OnRef = [this](const FString& C) { return MatchReference(C); };
	O.OnHyp = [this](const FString& C) { return ChooseHypothesis(C); };
	S47Panels::ReferenceFile(Ui(), O);
}

void US47Chapter1::OpenCompare()
{
	if (!S.F1 || !S.F2) return;
	const bool bBoard = M && M->Board.Open;
	if (S.Mark2 && bBoard) { M->Board.Open(TEXT("PHOTO LAB"), TEXT("ch1")); return; }
	S47Panels::FTwoExposures O;
	O.Url1 = S.F1->Url; O.Url2 = S.F2->Url; O.Mark2 = S.Mark2; O.bConcluded = S.bConcluded;
	O.Hints = {
		TEXT("Mark the same assembly in the right-hand print. What moved when the control changed? What did not?"),
		TEXT("Look at the ivory reference stripe and the second pale mark beside it. The second mark keeps the original alignment while the physical vane turns."),
	};
	O.OnMark = [this](double U, double V) -> FS47Result
	{
		if (!S.F2 || !Near(*S.F2, U, V, MARK_R2)) return { false, TEXT("No comparable reference feature at that point. Mark the same assembly you marked in FRAME 01.") };
		if (!S.Mark2) { S.Mark2 = FVector2D(U, V); Save(); }
		return { true, TEXT("Reference marked in FRAME 02. Now record what the two prints support.") };
	};
	if (bBoard) O.OnBoard = [this] { if (M && M->Board.Open) M->Board.Open(TEXT("PHOTO LAB"), TEXT("ch1")); };
	else O.OnConclude = [this](const FString& C) { return Conclude(C); };
	S47Panels::TwoExposures(Ui(), O);
}

FS47Result US47Chapter1::MatchReference(const FString& C)
{
	if (!S.Sheet.Get(false) && !S.bRef) return { false, TEXT("Read the installation sheet at the photo lab archive bench first.") };
	if (!S.Mark1) return { false, TEXT("Mark the detail on CONTACT PRINT 01 first.") };
	if (C == TEXT("R-07"))
	{
		S.WrongRef++;
		NoteIt(TEXT("Rejected R-07 as the photographed reference: its three-bar pattern does not match the marked foreground detail."));
		Save();
		return { false, TEXT("R-07 has THREE HORIZONTAL BARS. Compare that silhouette with the vertical feature you marked. The sheet does not place R-07 on this sightline.") };
	}
	if (!S.bRef)
	{
		S.bRef = true;
		NoteIt(TEXT("Matched the marked foreground reference to B-12 using the installation sheet and the S-03 sightline."));
		Save();
	}
	return { true, TEXT("B-12 selected. Its installation sheet specifies ONE ivory stripe on a movable vane. Choose a cause you can test.") };
}

FS47Result US47Chapter1::ChooseHypothesis(const FString& C)
{
	if (!S.bRef) return { false, TEXT("Match the marked detail to B-12 before choosing a cause to test.") };
	if (!S.Hyp.IsEmpty()) return { true, TEXT("A working hypothesis is on file. The field test is at B-12, north of the S-03 apron.") };
	if (C == TEXT("command"))
	{
		S.WrongHyp++;
		NoteIt(TEXT("Hypothesis rejected: a second motor command contradicts the S-03 zero-command record."));
		Save();
		return { false, TEXT("The S-03 log records ZERO commands during the first movement. A second command does not explain this record. Test light or the local reference instead.") };
	}
	S.Hyp = C;
	NoteIt(C == TEXT("light") ? TEXT("Working hypothesis: an extra stripe caused by reflected work light.") : TEXT("Working hypothesis: a mismatch caused by the local reference or encoder."));
	StageTo(TEXT("choose-control"));
	Toast(TEXT("Field test available. B-12, north of the S-03 apron."), 4.f);
	return { true, C == TEXT("light")
		? TEXT("Working hypothesis: reflected work light. At B-12, shielding the lamp changes the light while leaving the motor isolated.")
		: TEXT("Working hypothesis: reference/encoder drift. At B-12, a commanded move provides a known reference state.") };
}

FS47Result US47Chapter1::Conclude(const FString& C)
{
	if (S.bConcluded) return { true, TEXT("Finding recorded. File both prints at the records desk.") };
	if (S.Dev1 < 3 || S.Dev2 < 3 || !S.bObserved) return { false, TEXT("Develop both prints and note the B-12 control observation first.") };
	if (!S.Mark2) return { false, TEXT("Mark the reference detail in FRAME 02 before recording a conclusion.") };
	if (C == TEXT("lamp"))
	{
		S.WrongConcl++; Save();
		return { false, LampReply() };
	}
	if (C == TEXT("commands"))
	{
		S.WrongConcl++; Save();
		return { false, TEXT("The first S-03 movement had no command. The control used a different local reference. These records do not support two antenna commands.") };
	}
	S.bConcluded = true;
	File(DocFinding());
	NoteIt(TEXT("Filed: Two exposures / Local finding."));
	NoteIt(FString::Printf(TEXT("The second exposure reproduces the extra reference stripe under a controlled condition. The local test rules out %s as its source."), S.Method == TEXT("passive") ? TEXT("the work lamp") : TEXT("simple reference/encoder drift")));
	if (US47Audio* A = Au()) A->Signature(0.45f);
	StageTo(TEXT("file-report"));
	return { true, TEXT("FINDING RECORDED // The physical reference changed. The film retains a second reference. File both prints at the records desk.") };
}

FString US47Chapter1::LampReply() const
{
	return S.Method == TEXT("passive")
		? TEXT("The lamp was shielded for frame 02. Its extra mark cannot be explained by that lamp remaining on.")
		: TEXT("The vane moved to a known angle while the second mark kept its old alignment. A lamp reflection alone does not account for the fixed second reference.");
}

// ---------- B-12 ----------

void US47Chapter1::UseB12()
{
	if (S.Stage == TEXT("observe-control"))
	{
		if (!bSettled) return;
		S.bObserved = true;
		File(DocObservation());
		NoteIt(TEXT("Filed: B-12 / Direct control observation."));
		NoteIt(S.Method == TEXT("passive")
			? TEXT("Direct control observation: one physical ivory stripe. The work lamp is shielded and motor remains isolated.")
			: TEXT("Direct control observation: one physical ivory stripe. The local reference responds to the 042-degree command."));
		if (US47UI* U = Ui()) U->Document(DocObservation(), [] {});
		StageTo(TEXT("second-exposure"));
		return;
	}
	if (!S.Method.IsEmpty()) { S47Panels::B12Control(Ui(), S.Method, [](const FString&) {}, ObservationText()); return; }
	if (S.Stage != TEXT("choose-control"))
	{
		Toast(TEXT("The local control is available, but a useful test needs a question. Examine the print and the reference sheet in the photo lab first."), 4.5f);
		return;
	}
	S47Panels::B12Control(Ui(), FString(), [this](const FString& Mth)
	{
		S.Method = Mth;
		const double Target = Mth == TEXT("passive") ? -60 : 45;
		AnimVane = FVaneAnim{ 0, Target, Gt() };
		bSettled = false;
		if (US47Audio* A = Au()) A->Servo(float(VANE_TIME), Yard ? Yard->VaneObj.WorldPosition() : FVector::ZeroVector);
		if (Mth == TEXT("passive")) { if (Yard) Yard->SetWorkLamp(0); Toast(TEXT("Lamp shield engaged. Motor isolated."), 3.f); }
		else { if (Yard) Yard->SetWorkLamp(1.35); Toast(TEXT("Reference drive commanding 042 degrees."), 3.f); }
		StageTo(TEXT("observe-control"));
		After(VANE_TIME, [this]
		{
			bSettled = true;
			Toast(TEXT("B-12 settled. Note the direct observation at the control."), 3.5f);
		}, TEXT("vane"));
	});
}

// ---------- camera ----------

void US47Chapter1::ToggleCamera()
{
	US47FieldCamera* F = Fcam();
	US47UI* U = Ui();
	if (!Active || !F || !F->bHave) return;
	if (U && U->Modal()) return;
	F->Raise(!F->bRaised);
	if (US47Audio* A = Au()) A->Sfx(TEXT("click"), Snd(0.35f, F->bRaised ? 1.2f : 0.9f));
	if (U)
	{
		U->CameraButton(Touch(), F->bRaised);
		if (!F->bRaised) U->Viewfinder(false);
	}
}

FString US47Chapter1::FrameNo() const
{
	return Field ? Field->FrameNo() : S.F2 ? TEXT("FRAME 03") : S.F1 ? TEXT("FRAME 02") : TEXT("FRAME 01");
}

FVector US47Chapter1::FeedPosition(double X, double Z, double Scale, double Az, double El)
{
	// Dish.matrices and DishArray.feedPosition: (0, APEX_Y, 0) through root * T(0, 7.8, 0) *
	// Ry(-az) * T(0, 4.6, 0) * Rx(-(90 - el)), APEX_Y = 1.1 + F (F = 7.2)
	const double Deg = PI / 180, ApexY = 1.1 + 7.2;
	FVector V(0, ApexY, 0);
	const double Ax = -(90 - El) * Deg;
	V = FVector(V.X, V.Y * FMath::Cos(Ax) - V.Z * FMath::Sin(Ax), V.Y * FMath::Sin(Ax) + V.Z * FMath::Cos(Ax));
	V.Y += 4.6;
	const double Ay = -Az * Deg;
	V = FVector(V.X * FMath::Cos(Ay) + V.Z * FMath::Sin(Ay), V.Y, -V.X * FMath::Sin(Ay) + V.Z * FMath::Cos(Ay));
	V.Y += 7.8;
	return V * Scale + FVector(X, 0, Z);
}

FVector US47Chapter1::S03Centre() const
{
	// ext.dishes.find(S-03): its feed, half way to (x, 12.4 * scale, z)
	double X = 9.5, Z = -42, Scale = 0.9, Az = 26, El = 32;
	for (const TSharedPtr<FJsonValue>& V : US47Yard::ManifestSection(TEXT("dishes")))
	{
		const S47J::FObj D = V->AsObject();
		if (S47J::Str(D, TEXT("id")) != TEXT("S-03")) continue;
		X = S47J::Num(D, TEXT("x"), X); Z = S47J::Num(D, TEXT("z"), Z); Scale = S47J::Num(D, TEXT("scale"), Scale);
	}
	if (M && M->Web)
	{
		for (int32 i = 0; i < M->Web->DishCount(); ++i) if (M->Web->DishId(i) == TEXT("S-03")) { Az = M->Web->DishAz(i); El = M->Web->DishEl(i); }
	}
	const FVector Feed = FeedPosition(X, Z, Scale, Az, El);
	return FMath::Lerp(Feed, FVector(X, 12.4 * Scale, Z), 0.5);
}

FS47ShotCheck US47Chapter1::Check() const
{
	// Why the shutter would (not) fire right now. Mirrors the Unity rules. bNear marks where the
	// case frame keeps the shutter: the S-03 apron until frame 01, and the B-12 sight line mark.
	if (Field) return Field->Check();
	const US47FieldCamera* F = Fcam();
	const FVector P = M && M->Player ? M->Player->Pos : FVector::ZeroVector;
	using namespace S47YardPlaces;
	const bool bOnApron = !S.F1 && P.X >= ApronMinX && P.X <= ApronMaxX && P.Z >= ApronMinZ && P.Z <= ApronMaxZ;
	const bool bDark = Dark && Dark();
	if (!S.bLog) return { TEXT("READ THE S-03 MOTOR LOG FIRST"), false, bOnApron };
	if (!S.F1)
	{
		if (!bOnApron) return { TEXT("MOVE TO THE S-03 SERVICE APRON"), false };
		if (bDark) return { TEXT("TOO DARK FOR THE FRAME // WAIT FOR THE LIGHT"), false, true };
		const FVector C = S03Centre();
		if (!F || !F->InFrame(C, 0.36, 0.64, 0.30, 0.65)) return { TEXT("ALIGN THE CENTRAL ANTENNA IN THE FRAME"), false, true };
		if (!F->ClearLine(C, Yard ? Yard->Occluders : TArray<FS47Obj>())) return { TEXT("VIEW OBSTRUCTED // FIND A CLEAR LINE OF SIGHT"), false, true };
		return { TEXT("REFERENCE IN FRAME // READY"), true };
	}
	if (S.F2) return { TEXT("CONTROL FRAME EXPOSED // RETURN TO THE PHOTO LAB"), false };
	if (S.Dev1 < 3) return { TEXT("UNPROCESSED FILM // PHOTO LAB"), false };
	if (S.Hyp.IsEmpty()) return { TEXT("REVIEW FRAME 01 AND THE B-12 REFERENCE SHEET"), false };
	const bool bAtMark = FMath::Sqrt(FMath::Square(P.X - SightMark().X) + FMath::Square(P.Z - SightMark().Z)) <= 3.5;
	if (S.Method.IsEmpty() || !bSettled || !S.bObserved) return { !S.Method.IsEmpty() && !bSettled ? TEXT("WAIT FOR THE REFERENCE VANE TO SETTLE") : TEXT("NOTE THE B-12 OBSERVATION FIRST"), false, bAtMark && !S.Method.IsEmpty() };
	if (!bAtMark) return { TEXT("MOVE TO THE B-12 SIGHT LINE MARK"), false };
	if (bDark) return { TEXT("TOO DARK FOR THE FRAME // WAIT FOR THE LIGHT"), false, true };
	const FVector V = Yard ? Yard->VaneObj.WorldPosition() : Vane();
	if (!F || !F->InFrame(V, 0.22, 0.78, 0.23, 0.78)) return { TEXT("FRAME THE B-12 REFERENCE VANE AND ITS FIXED BRACKET"), false, true };
	if (!F->ClearLine(V, Yard ? Yard->Occluders : TArray<FS47Obj>())) return { TEXT("REFERENCE OBSTRUCTED // FIND A CLEAR LINE OF SIGHT"), false, true };
	return { TEXT("REFERENCE IN FRAME // READY"), true };
}

void US47Chapter1::FreeLine(FS47Viewfinder& V) const
{
	// The free frames' line in the viewfinder: ready (here the shutter takes one), or how many are left.
	if (!Free) return;
	const int32 Left = Free->Left ? Free->Left() : 0;
	const bool bHere = !Status.bOk && !Status.bNear;
	V.bHasFree = true;
	V.FreeText = bHere ? (Left ? FString::Printf(TEXT("FREE FRAME // %d LEFT"), Left) : FString(TEXT("NO FREE FRAMES LEFT"))) : FString::Printf(TEXT("%d FREE FRAMES LEFT"), Left);
	V.bFreeOk = bHere && Left > 0;
}

void US47Chapter1::Shutter()
{
	US47FieldCamera* F = Fcam();
	US47UI* U = Ui();
	US47Audio* A = Au();
	if (!F || !F->bRaised) return;
	const FS47ShotCheck C = Check();
	if (!C.bOk)
	{
		// away from the case's subjects the shutter takes a free frame (the album) while the roll lasts
		const FS47FreeShots* FreeShots = !C.bNear && Free ? &*Free : nullptr;
		if (FreeShots && FreeShots->Left && FreeShots->Left() > 0) { if (FreeShots->Expose) FreeShots->Expose(); return; }
		if (Field) { if (Field->Rejected) Field->Rejected(); } else S.Rejected++;
		if (A) A->Sfx(TEXT("click"), Snd(0.3f, 0.7f));
		Toast(FreeShots ? FString(TEXT("No free frames left on the roll.")) : C.Text + TEXT(". No film used."), 2.6f);
		return;
	}
	TWeakObjectPtr<US47Chapter1> W = this;
	if (Field)
	{
		if (Field->Expose) Field->Expose();
		After(0.6, [W] { if (W.IsValid() && W->Fcam() && W->Fcam()->bRaised) W->ToggleCamera(); });
		return;
	}
	const bool bFirst = !S.F1;
	const FString Id = bFirst ? TEXT("frame01") : TEXT("frame02");
	const double Clock = Host ? Host->Clock() : 0.0;
	const FVector Stripe = Yard ? Yard->VaneStripe.WorldPosition() : FVector::ZeroVector;
	const FVector EchoP = Yard ? Yard->Echo.WorldPosition() : FVector::ZeroVector;
	F->Sync();
	TArray<TPair<FString, FVector2D>> Targets;
	for (const TPair<FString, FVector>& K : { TPair<FString, FVector>(TEXT("vane"), Stripe), TPair<FString, FVector>(TEXT("echo"), EchoP) })
	{
		const FS47Projection R = F->Project(K.Value);
		if (R.bFront) Targets.Add({ K.Key, FVector2D(R.U, R.V) });
	}
	FS47ExposeOptions O;
	O.Before = [W] { if (W.IsValid() && W->Yard) W->Yard->Echo.SetVisible(true); }; // (and ext.dishArray.cull(photoCam): see US47FieldCamera::Render)
	O.After = [W] { if (W.IsValid() && W->Yard) W->Yard->Echo.SetVisible(false); };
	O.Restore = [] {}; // this.d.view.restore(): the screen was never changed
	O.Caption = bFirst ? FString(TEXT("S-03 // ARRAY PROFILE")) : FString::Printf(TEXT("B-12 // %s"), S.Method == TEXT("passive") ? TEXT("SHIELDED CONTROL") : TEXT("CONTROL 042"));
	O.Time = ClockText(Clock);
	O.Seed = FMath::FloorToDouble(Clock * 1000);
	TArray<FColor> Print;
	F->Expose(O, Print);
	// (this.d.view.draw(): the next frame draws as usual)
	FS47Photo Photo;
	Photo.Id = Id;
	Photo.Subject = bFirst ? TEXT("S-03 / array profile") : TEXT("B-12 / controlled reference");
	Photo.Method = bFirst ? FString(TEXT("baseline")) : S.Method;
	Photo.Clock = Clock;
	Photo.Url = S47Image::ToDataUrl(Print, US47FieldCamera::PRINT_W, US47FieldCamera::PRINT_H, true, 86);
	Photo.Targets = Targets;
	UE_LOG(LogS47Chapter1, Display, TEXT("S47: %s exposed, %d targets, %d kB"), *Id, Targets.Num(), Photo.Url.Len() / 1024);
	if (A) A->Shutter();
	if (U) U->ShutterFlash();
	if (bFirst)
	{
		S.F1 = Photo;
		NoteIt(TEXT("Frame 01 exposed. Sealed film awaits processing in the photo lab."));
		Toast(TEXT("Frame 01 exposed. Process the film in the photo lab."), 3.5f);
		StageTo(TEXT("develop-first"));
	}
	else
	{
		S.F2 = Photo;
		NoteIt(TEXT("Frame 02 exposed. Sealed film awaits processing in the photo lab."));
		NoteIt(FString::Printf(TEXT("The control exposure preserves the local test condition: %s."), S.Method == TEXT("passive") ? TEXT("lamp shielded, motor isolated") : TEXT("reference driven to 042 degrees")));
		Toast(TEXT("Control frame exposed. Return to the photo lab."), 3.5f);
		StageTo(TEXT("develop-second"));
	}
	After(0.6, [W] { if (W.IsValid() && W->Fcam() && W->Fcam()->bRaised) W->ToggleCamera(); });
}

// ---------- report and ending ----------

FString US47Chapter1::ReportText() const
{
	const bool bPassive = S.Method == TEXT("passive");
	return FString::Join(TArray<FString>{
		TEXT("SARO / LOCAL INCIDENT S-03 + B-12\nOPERATOR: REYES    FOR: DR. E. WARD"),
		TEXT("S-03: encoder 026; scheduled 042; commands 0.\nFrame 01: foreground reference does not match the one-stripe installation sheet."),
		FString::Printf(TEXT("CONTROL: %s\nDirect observation: one physical stripe.\nFrame 02: the physical vane changes; a second stripe retains its earlier alignment."), bPassive ? TEXT("lamp shielded, motor isolated.") : TEXT("local reference commanded to 042; encoder and visible vane agree.")),
		FString::Printf(TEXT("SUPPORTED: the discrepancy persists in the photographic record under the chosen control. %s"), bPassive ? TEXT("The work lamp is not sufficient to explain it.") : TEXT("Simple reference/encoder drift is not sufficient to explain it.")),
		TEXT("LIMIT: one local test does not establish the cause of the array movement or the signal. Preserve the negatives and the original receiver printout."),
	}, TEXT("\n\n"));
}

void US47Chapter1::FileReport()
{
	if (S.bFiled) return;
	S.bFiled = true;
	NoteIt(TEXT("LOCAL CASE CLOSED: S-03 and B-12 records filed together. Preserve negatives. Follow-up: why does the original return report a distance of -39 LY?"));
	if (US47Audio* A = Au()) A->Sfx(TEXT("thudSoft"), Snd(0.4f));
	StageTo(TEXT("complete"));
	TWeakObjectPtr<US47Chapter1> W = this;
	After(0.8, [W] { if (W.IsValid() && W->OnEnd) W->OnEnd(W->S.Method.IsEmpty() ? FString(TEXT("passive")) : W->S.Method); });
}

// ---------- documents ----------

FS47Doc US47Chapter1::DocS03() const
{
	// since then, the array put back on the schedule from the console and gone back by itself:
	// each command the controller had, and each turn back that nobody commanded
	const TOptional<FS47S03Since> Since = S03Since ? S03Since() : TOptional<FS47S03Since>();
	FString More;
	if (Since)
	{
		More = FString::Printf(TEXT(" Since then the controller has had %s from the console, to the schedule. %s to 042, and back to 026 with no command."),
			Since->Cmds == 1 ? TEXT("one command") : *FString::Printf(TEXT("%d commands"), Since->Cmds), Since->Cmds == 1 ? TEXT("It went") : TEXT("Each time it went"));
	}
	FS47Doc D;
	D.Id = TEXT("s03log"); D.Kind = TEXT("typed"); D.Title = TEXT("S-03 / No command recorded"); D.Stamp = TEXT("MOTOR BUS");
	D.Page = FString::Printf(TEXT("SARO ARRAY / MOTOR BUS S-03\nLOCAL CONTROLLER LOG // %s\n\nENCODER HEADING:   026 DEGREES\nSCHEDULED HEADING: 042 DEGREES\n\nCOMMANDS RECEIVED: 0\nLOCAL OVERRIDE:    NONE%s"),
		*ClockText(S.EventClock, false), Since ? *(TEXT("\n\n") + Since->Text) : TEXT(""));
	D.Transcript = FString::Printf(TEXT("The antenna moved. The controller did not send a command.%s\n\nA thin line of condensation runs across the warm inspection glass."), *More);
	return D;
}

void US47Chapter1::RefileS03() { if (S.bLog) File(DocS03()); }

FS47Doc US47Chapter1::DocSheet() const
{
	FS47Doc D;
	D.Id = TEXT("refsheet"); D.Kind = TEXT("typed"); D.Title = TEXT("B-12 / Reference installation sheet"); D.Stamp = TEXT("11-86");
	D.Page = TEXT("SARO / INSTALLATION SHEET 11-86\nLOCAL SURVEY REFERENCES\n\nB-12  ONE IVORY STRIPE\n      MOVABLE VANE\n      FIXED BRACKET: STATION NO.\nR-07  THREE HORIZONTAL BARS\n      WEST FENCE\n\nSIGHTLINE FROM SERVICE APRON:\nS-03 APRON > B-12 > CENTRAL ANTENNA");
	D.Transcript = TEXT("Local survey references: B-12 is the movable single ivory stripe north of the S-03 apron. R-07 uses three horizontal bars. The warm inspection lamp and local reference drive have independent controls.");
	return D;
}

FS47Doc US47Chapter1::DocPhoto(const FS47Photo& P) const
{
	const bool bFirst = P.Id == TEXT("frame01");
	FS47Doc D;
	D.Id = P.Id; D.Kind = TEXT("photo"); D.Image = P.Url; D.Page = FString();
	D.Title = bFirst ? TEXT("PHOTO 01 / S-03 contact print") : TEXT("PHOTO 02 / B-12 control print");
	D.Transcript = bFirst
		? FString::Printf(TEXT("Exposed from the S-03 apron at %s. The central antenna, and below it the B-12 reference vane in the lower foreground.\n\nThe installation sheet says B-12 carries one stripe."), *ClockText(P.Clock))
		: FString::Printf(TEXT("Exposed from the B-12 sight line at %s, under the %s control."), *ClockText(P.Clock), P.Method == TEXT("passive") ? TEXT("shielded, passive") : TEXT("active, 042-degree"));
	return D;
}

FString US47Chapter1::ObservationText() const
{
	return S.Method == TEXT("passive")
		? TEXT("B-12 / PASSIVE CONTROL\n\nLamp: shielded, no direct illumination.\nMotor: isolated, no drive command.\nDirect observation: one ivory stripe on the folded reference vane; one fixed bracket.\n\nMake a control exposure from the apron without changing this setup.")
		: TEXT("B-12 / ACTIVE CONTROL\n\nLocal command: reference 042 degrees.\nEncoder: 042 degrees, reference moved and settled.\nDirect observation: one ivory stripe on the turned vane; one fixed bracket.\nWork lamp remains lit.\n\nMake a control exposure without changing this setup.");
}

FS47Doc US47Chapter1::DocObservation() const
{
	FS47Doc D;
	D.Id = TEXT("observation"); D.Kind = TEXT("typed"); D.Title = TEXT("B-12 / Direct control observation"); D.Stamp = S.Method == TEXT("passive") ? TEXT("PASSIVE") : TEXT("ACTIVE");
	D.Page = ObservationText();
	D.Transcript = TEXT("One physical stripe on the vane. Expose frame 02 from the B-12 sight line mark without changing the setup.");
	return D;
}

FS47Doc US47Chapter1::DocFinding() const
{
	FS47Doc D;
	D.Id = TEXT("finding"); D.Kind = TEXT("typed"); D.Title = TEXT("Two exposures / Local finding"); D.Stamp = TEXT("LOCAL");
	D.Page = ReportText();
	D.Transcript = TEXT("The eye saw one reference. The film kept two.");
	return D;
}

// ---------- per frame ----------

void US47Chapter1::Update(double Dt, double T)
{
	// `lead` is false while a later chapter runs: the yard and the lab stay as they were, but
	// that chapter owns the objective line.
	if (!Active) return;
	const bool bLead = Host && Host->ChapterKey == TEXT("ch1");
	const double G = Gt();
	if (AnimVane)
	{
		const FVaneAnim& A = *AnimVane;
		const double K = FMath::Min(1.0, (G - A.T0) / VANE_TIME);
		const double E = K < 0.5 ? 2 * K * K : 1 - FMath::Pow(-2 * K + 2, 2.0) / 2;
		if (Yard) Yard->SetVane(A.From + (A.To - A.From) * E);
		if (K >= 1) AnimVane.Reset();
	}
	if (AnimFix)
	{
		const double K = FMath::Min(1.0, (G - *AnimFix) / FIX_TIME);
		if (Yard) Yard->WetPrint.SetColor(FLinearColor(float(K * K), float(K * K), float(K * K), 1.f));
		if (K >= 1) AnimFix.Reset();
	}
	BusT -= Dt;
	if (BusT <= 0) { BusT = 0.25; if (Yard) Yard->DrawBus(26, 0, true, T); }
	// (Unreal only) the lab under the safelight alone while a print is in the process (left alone in
	// the outage: saroPower has the lamps then)
	if (!(Dark && Dark())) Darkroom(Developing());

	// walking back into the control room with the log
	const FVector P = M && M->Player ? M->Player->Pos : FVector(100, 0, 100);
	if (!S.bReturned && S.bLog && FMath::Abs(P.X) < 5.9 && FMath::Abs(P.Z) < 4.4)
	{
		S.bReturned = true;
		NoteIt(TEXT("Returned to the control room with the S-03 motor log. The unauthorized alignment is documented."));
		Toast(TEXT("S-03 investigation filed."), 2.6f);
		Save();
	}

	US47FieldCamera* F = Fcam();
	US47UI* U = Ui();
	// main.ts ui.onModalChange: a paper or a panel that opens puts the camera down
	if (F && F->bRaised && U && U->Modal())
	{
		F->Raise(false);
		U->Viewfinder(false);
		U->CameraButton(Touch() && F->bHave, false);
	}
	if (F && F->bRaised)
	{
		F->Sync();
		if (U && !U->Modal())
		{
			Status = Check();
			const FS47FrameRect R = F->FrameRect();
			FS47Viewfinder V;
			V.X = float(R.X); V.Y = float(R.Y); V.W = float(R.W); V.H = float(R.H);
			V.Frame = FrameNo(); V.Status = Status.Text; V.bOk = Status.bOk; V.bTouch = Touch();
			FreeLine(V);
			U->Viewfinder(true, V);
		}
	}
	if (bLead && U) U->Objective(Objective());

	// Doors.onChange -> Prologue.doorChanged: the doors as they are now against last frame
	if (M && M->Web)
	{
		const bool bEast = M->Web->IsDoorOpen(TEXT("doorEast")), bLab = M->Web->IsDoorOpen(TEXT("labDoor"));
		if (bEast != bEastOpen) { bEastOpen = bEast; DoorChanged(TEXT("east"), bEast); }
		if (bLab != bLabOpen) { bLabOpen = bLab; DoorChanged(TEXT("lab"), bLab); }
	}
}

// ---------- the test drive ----------

void US47Chapter1::TestDrive(const FString& Stop)
{
	// tools/chapter1.py's moves, done from here: the camera from the shelf, the S-03 log, FRAME 01
	// from the apron, the bench, the drying line, the loupe. Each stop holds the game there.
	TWeakObjectPtr<US47Chapter1> W = this;
	auto Say = [](const FString& T) { UE_LOG(LogS47Chapter1, Display, TEXT("S47: drive: %s"), *T); };
	auto Aim = [W](double Px, double Pz, const FVector& T, double Dy)
	{
		AS47Walker* P = W->M->Player;
		P->Place(Px, Pz, FMath::Atan2(-(T.X - Px), -(T.Z - Pz)));
		P->Pitch = FMath::Atan2(T.Y - 1.62, FMath::Sqrt(FMath::Square(T.X - Px) + FMath::Square(T.Z - Pz))) + Dy;
	};
	if (!M || !M->Player || !M->Inter || !Host) return;
	Host->After(0.5, [W, Say]
	{
		if (!W.IsValid()) return;
		W->M->Inter->UseId(TEXT("fieldCamera"));
		W->M->Inter->UseId(TEXT("motorBus"));
		if (W->Ui()) W->Ui()->Close(); // the S-03 log read and put back
		Say(FString::Printf(TEXT("camera %d, log %d, stage %s"), W->S.bCamera, W->S.bLog, *W->S.Stage));
		// -S47Ch1Dump=<dir>: the motor bus screen as drawn
		FString Dir;
		TArray<FColor> Px;
		if (FParse::Value(FCommandLine::Get(), TEXT("S47Ch1Dump="), Dir) && W->Yard && S47Image::ReadPixels(W->Yard->BusTarget(), Px))
		{
			const FString Url = S47Image::ToDataUrl(Px, 256, 160, false);
			TArray<uint8> Png;
			if (FBase64::Decode(Url.Mid(Url.Find(TEXT(",")) + 1), Png)) FFileHelper::SaveArrayToFile(Png, *(Dir / TEXT("bus.png")));
			// and the drawings (story/drawings.ts)
			const TPair<const TCHAR*, FString> Drawings[] = { { TEXT("arrangement"), S47Drawings::Arrangement(true) }, { TEXT("amended"), S47Drawings::Arrangement(false) },
				{ TEXT("sleeve"), S47Drawings::SleeveMark() }, { TEXT("icon-bar"), S47Drawings::MarkIcon(TEXT("triangle-bar")) }, { TEXT("icon-bars"), S47Drawings::MarkIcon(TEXT("three-bars")) } };
			for (const TPair<const TCHAR*, FString>& D : Drawings)
			{
				TArray<uint8> Bytes;
				if (FBase64::Decode(D.Value.Mid(D.Value.Find(TEXT(",")) + 1), Bytes)) FFileHelper::SaveArrayToFile(Bytes, *(Dir / FString(D.Key) + TEXT(".png")));
			}
		}
	});
	Host->After(1.0, [W, Aim, Say]
	{
		if (!W.IsValid()) return;
		Aim(10.4, -11.6, W->S03Centre(), -0.06);
		W->ToggleCamera();
		Say(TEXT("camera raised at the apron"));
	});
	if (Stop == TEXT("viewfinder")) { Host->After(2.0, [W, Say] { if (W.IsValid()) Say(TEXT("viewfinder: ") + W->Status.Text); }); return; }
	Host->After(2.0, [W, Say]
	{
		if (!W.IsValid()) return;
		Say(TEXT("before the shutter: ") + W->Check().Text);
		W->Shutter();
		Say(FString::Printf(TEXT("frame 01 %s, stage %s, targets %d"), W->S.F1 ? TEXT("exposed") : TEXT("refused"), *W->S.Stage, W->S.F1 ? W->S.F1->Targets.Num() : 0));
		// -S47Ch1Dump=<dir>: the print as the save holds it, for a look
		FString Dir;
		if (W->S.F1 && FParse::Value(FCommandLine::Get(), TEXT("S47Ch1Dump="), Dir))
		{
			TArray<uint8> Jpeg;
			const FString& Url = W->S.F1->Url;
			if (FBase64::Decode(Url.Mid(Url.Find(TEXT(",")) + 1), Jpeg)) FFileHelper::SaveArrayToFile(Jpeg, *(Dir / TEXT("frame01.jpg")));
			for (const TPair<FString, FVector2D>& T : W->S.F1->Targets) Say(FString::Printf(TEXT("target %s at %.3f %.3f"), *T.Key, T.Value.X, T.Value.Y));
		}
	});
	if (Stop == TEXT("photo")) { Host->After(3.0, [W] { if (W.IsValid() && W->S.F1 && W->Ui()) W->Ui()->Document(W->DocPhoto(*W->S.F1)); }); return; }
	if (Stop == TEXT("bench")) { Host->After(3.0, [W] { if (W.IsValid()) W->M->Inter->UseId(TEXT("wetBench")); }); return; }
	Host->After(3.0, [W, Aim, Say]
	{
		if (!W.IsValid() || !W->S.F1) return;
		// in the lab: the print into the fixer (wet), or developed and hung up
		Aim(15.4, -3.9, FVector(15.4, 1.0, -5.4), 0.0);
		W->S.Dev1 = 1;
		W->FixPrint(*W->S.F1);
		Say(TEXT("the print in the fixer"));
	});
	if (Stop == TEXT("wet")) return;
	Host->After(5.0, [W, Aim, Stop, Say]
	{
		if (!W.IsValid() || !W->S.F1) return;
		W->Collect(1, *W->S.F1);
		if (Stop != TEXT("print") && W->Ui()) W->Ui()->Close(); // (game time stands while the loupe is open)
		if (Stop == TEXT("lab")) Aim(15.9, -1.2, FVector(15.9, 1.99, -3.0), 0.0);
		Say(FString::Printf(TEXT("collected: stage %s, docs filed"), *W->S.Stage));
	});
	if (Stop == TEXT("lab") || Stop == TEXT("print")) return;
	if (Stop == TEXT("reference") || Stop == TEXT("b12") || Stop == TEXT("report"))
	{
		Host->After(6.0, [W, Stop]
		{
			if (!W.IsValid() || !W->S.F1) return;
			W->S.Mark1 = FVector2D(0.5, 0.5);
			if (Stop == TEXT("reference")) { W->M->Inter->UseId(TEXT("archive")); return; }
			W->S.Sheet = true; W->S.bRef = true; W->S.Hyp = TEXT("light"); W->S.Stage = TEXT("choose-control");
			if (Stop == TEXT("b12")) { W->M->Inter->UseId(TEXT("b12Control")); return; }
			W->S.Method = TEXT("passive"); W->S.bObserved = true; W->S.bConcluded = true; W->S.Stage = TEXT("file-report");
			W->M->Inter->UseId(TEXT("records"));
		});
		return;
	}
	Host->After(6.0, [W, Say, Stop]
	{
		if (!W.IsValid() || !W->S.F1) return;
		if (W->Ui()) W->Ui()->Close();
		// the rest of the case as the board would record it, a second exposure taken the same way
		const TPair<FString, FVector2D>* Vane = W->S.F1->Targets.FindByPredicate([](const TPair<FString, FVector2D>& T) { return T.Key == TEXT("vane"); });
		W->S.Mark1 = Vane ? Vane->Value : FVector2D(0.5, 0.5);
		W->S.Sheet = true; W->S.bRef = true; W->S.Hyp = TEXT("light"); W->S.Method = TEXT("passive"); W->S.bObserved = true; W->bSettled = true;
		W->Yard->SetVane(-60); W->Yard->SetWorkLamp(0);
		W->S.Stage = TEXT("second-exposure");
		W->M->Player->Place(10.0, -15.0, 0);
		const FVector V = W->Yard->VaneObj.WorldPosition();
		W->M->Player->Place(10.0, -15.0, FMath::Atan2(-(V.X - 10.0), -(V.Z + 15.0)));
		W->M->Player->Pitch = FMath::Atan2(V.Y - 1.62, FMath::Sqrt(FMath::Square(V.X - 10.0) + FMath::Square(V.Z + 15.0)));
		W->ToggleCamera();
		Say(TEXT("at the B-12 sight line mark"));
	});
	Host->After(7.0, [W, Say, Stop]
	{
		if (!W.IsValid()) return;
		Say(TEXT("before the second shutter: ") + W->Check().Text);
		W->Shutter();
		if (!W->S.F2) return;
		W->S.Dev2 = 3;
		W->HangPrint(1, *W->S.F2);
		W->S.Stage = TEXT("compare-exposures");
		if (Stop == TEXT("board") && W->M->Board.Open) W->M->Board.Open(TEXT("PHOTO LAB"), TEXT("ch1"));
		else W->OpenCompare();
		Say(TEXT("two exposures"));
	});
}

// ---------- self test ----------

bool US47Chapter1::SelfTest(TArray<FString>& Lines)
{
	// the field camera's and the drawings' checks go with chapter one's
	bool bOk = US47FieldCamera::SelfTest(Lines);
	bOk &= S47Drawings::SelfTest(Lines);
	auto Expect = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s chapter one: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };

	// the S-03 feed and the frame's centre (values from three, Dish.ts at az 26, el 32)
	{
		const FVector F = FeedPosition(9.5, -42, 0.9, 26, 32);
		const FVector C = FMath::Lerp(F, FVector(9.5, 12.4 * 0.9, -42), 0.5);
		Expect(F.Equals(FVector(12.277046, 15.118497, -47.693788), 1e-4) && C.Equals(FVector(10.888523, 13.139248, -44.846894), 1e-4), FString::Printf(TEXT("S-03 feed %s"), *F.ToString()));
	}

	// the saved case: field names, nulls, the merge with a fresh one
	{
		FS47CaseState St = FS47CaseState::Fresh(8112);
		St.Stage = TEXT("compare-exposures"); St.bCamera = true; St.bLog = true; St.Dev1 = 3; St.Dev2 = 3; St.Mark1 = FVector2D(0.5, 0.75); St.bRef = true; St.Hyp = TEXT("light");
		FS47Photo P; P.Id = TEXT("frame01"); P.Subject = TEXT("S-03 / array profile"); P.Method = TEXT("baseline"); P.Clock = 9067.25; P.Url = TEXT("data:image/jpeg;base64,AAAA");
		P.Targets = { { TEXT("vane"), FVector2D(0.625, 0.75) }, { TEXT("echo"), FVector2D(0.5, 0.75) } };
		St.F1 = P; St.Sheet = true; St.Links = TArray<FString>{ TEXT("frame01.detail|refsheet.b12") };
		const S47J::FObj O = St.ToJson();
		const FString J = S47J::Write(O);
		Expect(J.Contains(TEXT("\"labDoor\":false")) && J.Contains(TEXT("\"method\":null")) && J.Contains(TEXT("\"hyp\":\"light\"")) && J.Contains(TEXT("\"mark1\":[0.5,0.75]")) && J.Contains(TEXT("\"targets\":{\"vane\":[0.625,0.75],\"echo\":[0.5,0.75]}"))
			&& J.Contains(TEXT("\"wrongConcl\":0")) && J.Contains(TEXT("\"links\":[\"frame01.detail|refsheet.b12\"]")) && !J.Contains(TEXT("\"f2\"")) && !J.Contains(TEXT("\"mark2\"")), TEXT("the saved case writes the web version's fields"));
		const FS47CaseState B = FS47CaseState::FromJson(S47J::Read(J), 0);
		Expect(B.Stage == St.Stage && B.EventClock == 8112 && B.F1 && B.F1->Targets.Num() == 2 && B.F1->Targets[1].Key == TEXT("echo") && B.Mark1 && B.Mark1->Y == 0.75 && B.Hyp == TEXT("light") && B.Method.IsEmpty() && B.Sheet.Get(false) && B.Links && B.Links->Num() == 1 && !B.F2, TEXT("and reads them back"));
		const FS47CaseState C = FS47CaseState::FromJson(S47J::Read(TEXT("{\"stage\":\"motor-log\",\"camera\":true}")), 8112);
		Expect(C.Stage == TEXT("motor-log") && C.bCamera && C.EventClock == 8112 && C.Dev1 == 0 && !C.Sheet && !C.Links, TEXT("an old save merges with a fresh case"));
	}

	// (Unreal only) the darkroom: the door open stops the process before it starts, light on a print in
	// the process fogs it and keeps the negative, the fog is saved, and the print can be made again
	{
		US47Chapter1* C = NewObject<US47Chapter1>();
		C->Active = true;
		C->S = FS47CaseState::Fresh(8112);
		C->S.Stage = TEXT("develop-first"); C->S.bCamera = true; C->S.bLog = true;
		C->S.F1 = FS47Photo(); C->S.F1->Id = TEXT("frame01");
		Expect(C->BenchLabel() == TEXT("Process sealed film") && !C->Developing(), TEXT("darkroom: the sealed film at the bench, nothing in the process"));
		C->bLabOpen = true;
		Expect(C->BenchLabel() == TEXT("Shut the door first") && C->Objective() == TEXT("PHOTO LAB // DARKROOM: SHUT THE DOOR BEFORE DEVELOPING"), TEXT("darkroom: the door open, the bench says shut it first"));
		C->bLabOpen = false;
		C->S.Dev1 = 1;
		Expect(C->Developing() && C->BenchLabel() == TEXT("Transfer print to fixer"), TEXT("darkroom: the contact print waiting for the fixer is in the process"));
		C->DoorChanged(TEXT("lab"), true);
		Expect(C->S.Dev1 == 0 && C->S.Fog1 == 1 && !C->Developing() && C->S.bLabDoor && C->TestNotes.Num() == 1 && C->TestNotes[0].Contains(TEXT("fogged")), TEXT("darkroom: the door opened on it fogs the print, a note"));
		C->bLabOpen = false;
		Expect(C->BenchLabel() == TEXT("Print again from the negative") && C->Objective().Contains(TEXT("PRINT AGAIN FROM THE NEGATIVE")), TEXT("darkroom: the negative kept, a print again"));
		const FString J = S47J::Write(C->S.ToJson());
		const FS47CaseState B = FS47CaseState::FromJson(S47J::Read(J), 0);
		Expect(J.Contains(TEXT("\"fog1\":1")) && !J.Contains(TEXT("fog2")) && B.Fog1 == 1 && B.Fog2 == 0 && B.Dev1 == 0, TEXT("darkroom: the fogged print saved and read back"));
		C->Busy = TEXT("tank");
		const int32 Gen = C->DevGen;
		C->DoorChanged(TEXT("lab"), false);
		Expect(C->Developing() && C->S.Fog1 == 1, TEXT("darkroom: shutting the door does nothing to it"));
		C->DoorChanged(TEXT("lab"), true);
		Expect(C->S.Fog1 == 2 && C->Busy.IsEmpty() && C->DevGen != Gen, TEXT("darkroom: light into the tank fogs it too, its timers let go"));
		C->S.Stage = TEXT("interpret-first"); C->S.Dev1 = 3;
		C->DoorChanged(TEXT("lab"), true);
		Expect(!C->Developing() && C->S.Fog1 == 2, TEXT("darkroom: a collected print is safe in the light"));
	}

	// the chapter without a world: the board's rules, played as tools/chapter1.py plays them
	for (const TCHAR* Method : { TEXT("passive"), TEXT("active") })
	{
		US47Chapter1* C = NewObject<US47Chapter1>();
		C->BuildBook();
		C->Active = true;
		C->S = FS47CaseState::Fresh(8112);
		Expect(C->Tasks().Num() == 1 && C->Objective() == TEXT("FIELD KIT // COLLECT THE CAMERA BESIDE THE EAST DOOR"), TEXT("a fresh case: one task, the camera"));
		FS47RuleBook& B = *C->Book();
		C->S.bCamera = true; C->S.bLog = true; C->S.Stage = TEXT("interpret-first"); C->S.Dev1 = 3; C->S.Sheet = true; C->S.Mark1 = FVector2D(0.6, 0.7);
		C->S.F1 = FS47Photo(); C->S.F1->Id = TEXT("frame01");
		Expect(C->Objective() == TEXT("PHOTO LAB ARCHIVE // READ THE REFERENCE SHEET AND MATCH THE DETAIL ON THE CASE BOARD"), TEXT("objective after the mark"));
		Expect(!B.Connect(TEXT("x.a"), TEXT("y.b"), false).IsSet(), TEXT("pins of another chapter are not chapter one's"));
		TOptional<FS47Result> R = B.Connect(TEXT("frame01.detail"), TEXT("refsheet.r07"), false);
		Expect(R && !R->bOk && R->Text.Contains(TEXT("THREE HORIZONTAL BARS")) && C->S.WrongRef == 1, TEXT("R-07 rejected"));
		R = B.Connect(TEXT("frame01.detail"), TEXT("refsheet.b12"), false);
		Expect(R && R->bOk && !C->S.bRef && R->Text.EndsWith(TEXT("P01 has what it needs: RECORD it.")), TEXT("the matching thread fills P01; RECORD is still needed"));
		R = B.Record(TEXT("P01"));
		Expect(R && R->bOk && R->Text.StartsWith(TEXT("B-12 selected.")) && C->S.bRef, TEXT("B-12 matched"));
		R = B.Connect(TEXT("hyp.command"), TEXT("s03log.zero"), false);
		Expect(R && !R->bOk && R->Text.Contains(TEXT("ZERO commands")) && C->S.WrongHyp == 1, TEXT("the motor command rejected"));
		const bool bLight = FString(Method) == TEXT("passive");
		B.Connect(TEXT("hyp.light"), TEXT("refsheet.lamp"), false);
		B.Connect(TEXT("hyp.drift"), TEXT("refsheet.drive"), false);
		if (bLight) B.Connect(TEXT("hyp.light"), TEXT("refsheet.lamp"), false);
		Expect(C->S.Hyp.IsEmpty() && C->S.Links->Num() == 2, TEXT("one hypothesis thread in the group, not yet recorded"));
		R = B.Record(TEXT("P02"));
		Expect(R && R->bOk && C->S.Hyp == (bLight ? TEXT("light") : TEXT("drift")) && C->S.Stage == TEXT("choose-control"), FString::Printf(TEXT("%s hypothesis accepted"), bLight ? TEXT("light") : TEXT("drift")));
		// B-12, frame 02, the comparison
		C->S.Method = Method; C->S.bObserved = true; C->S.Dev2 = 3; C->S.F2 = FS47Photo(); C->S.F2->Id = TEXT("frame02"); C->S.Stage = TEXT("compare-exposures");
		R = B.Connect(TEXT("frame02.extra"), TEXT("observation.lamp"), false);
		Expect(R && !R->bOk && R->Text.Contains(TEXT("Mark the reference detail in FRAME 02")), TEXT("no conclusion before frame 02 is marked"));
		C->S.Mark2 = FVector2D(0.6, 0.7);
		R = B.Connect(TEXT("frame02.extra"), TEXT("observation.lamp"), false);
		Expect(R && !R->bOk && R->Text.Contains(bLight ? TEXT("lamp was shielded") : TEXT("vane moved to a known angle")) && C->S.WrongConcl == 1, TEXT("the lamp conclusion rejected with the chosen control's reply"));
		B.Connect(TEXT("frame01.extra"), TEXT("frame02.extra"), false);
		Expect(!B.Record(TEXT("P03"))->bOk, TEXT("one comparison thread cannot record the finding"));
		B.Connect(TEXT("observation.stripe"), TEXT("frame02.detail"), false);
		R = B.Record(TEXT("P03"));
		Expect(R && R->bOk && R->Text.StartsWith(TEXT("FINDING RECORDED")) && C->S.bConcluded && C->S.Stage == TEXT("file-report") && C->S.Links->Num() == 4, TEXT("the second reference concluded; four threads"));
		Expect(C->ReportText().Contains(bLight ? TEXT("CONTROL: lamp shielded, motor isolated.") : TEXT("CONTROL: local reference commanded to 042; encoder and visible vane agree.")), TEXT("the report names the control"));
		Expect(C->BoardCards().Num() == 6 && C->BoardCards()[4].Lines.Num() == 2, TEXT("six cards on the board"));
		C->S.bFiled = true;
		Expect(C->Tasks().Num() == 13, TEXT("all thirteen tasks once filed"));
		// a save without threads gets its recorded findings' threads back
		C->S.Links.Reset();
		B.Backfill();
		Expect(C->S.Links && C->S.Links->Num() == 4 && C->S.Links->Contains(bLight ? TEXT("hyp.light|refsheet.lamp") : TEXT("hyp.drift|refsheet.drive")), TEXT("backfill restores the chosen hypothesis's thread"));
	}
	return bOk;
}

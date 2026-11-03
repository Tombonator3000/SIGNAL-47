#include "S47WorldLife.h"

#include "S47ArrayShot.h"
#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Json.h"
#include "S47Main.h"
#include "S47Sky.h"
#include "S47SkyEventSpec.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47MeteorPassage, Log, All);

// FX-01 / PLAY-31-23 targeted runtime contract:
//   -S47MeteorPassageContract -S47Jump=event -S47Quality=high|low -UserDir=/tmp/... -S47MeteorPassageProof=/tmp/...
// The lifecycle is US47WorldLife's (pending/running/done, raw JSON save); this contract only drives
// the player and Flow and asserts what Life.Step/Flow produce. The real event arms the passage while
// the player is indoors; the real chapter-one start puts the player in the control room; Main computes
// Space from the Walker's own position, and the walks use the Walker's own input/collision path
// (TestMove, MoveBy). Every explicit test-only fixture (saved-pose edit, view yaw/pitch, story hold,
// area switch without travel, clock jump, JSON field replacement) is logged "TEST-ONLY" and listed
// in result.json. Production code never moves the camera for the passage.
namespace
{
	// Positions (three metres) of the control room's east service door route (as the SC01/evening
	// contracts walk it): room -> door plane x=6.06 -> service landing -> east walk -> open yard.
	const FVector2D InRoom(4.2, 1.6), DoorIn(5.2, 1.6), DoorPlane(6.1, 1.6), Landing(7.6, 1.6), EastWalk(9.3, 1.6), Yard(9.3, -9.0);
	const double End = S47SkyEvent::PassageLength + S47SkyEvent::PassageTail;

	struct FPv { bool bV2 = false; FString Phase; double Elapsed = -1; bool bExit = false; };
	FPv ParseV2(const TSharedPtr<FJsonValue>& V)
	{
		FPv P;
		if (!V || V->Type != EJson::Object) return P;
		const S47J::FObj O = V->AsObject();
		const TSharedPtr<FJsonValue> Ver = O->TryGetField(TEXT("v")), Ph = O->TryGetField(TEXT("phase")), El = O->TryGetField(TEXT("elapsed")), Ex = O->TryGetField(TEXT("exitReady"));
		if (!Ver || Ver->Type != EJson::Number || Ver->AsNumber() != 2 || !Ph || Ph->Type != EJson::String || !El || El->Type != EJson::Number || !Ex || Ex->Type != EJson::Boolean) return P;
		P.bV2 = true; P.Phase = Ph->AsString(); P.Elapsed = El->AsNumber(); P.bExit = Ex->AsBool();
		return P;
	}
	FString RawText(const TSharedPtr<FJsonValue>& V)
	{
		if (!V) return TEXT("<missing>");
		const S47J::FObj Box = S47J::New(); Box->SetField(TEXT("p"), V);
		return S47J::Write(Box);
	}
	TArray<FString> Texts(const TArray<TSharedPtr<FJsonValue>>& A) { TArray<FString> Out; for (const auto& V : A) Out.Add(RawText(V)); return Out; }

	// One save-compatibility case restored through the real Flow.
	struct FLoadCase
	{
		FString Name, Base, Expect;      // Base: done | presignal | exitready; Expect: idle | pending | done
		TSharedPtr<FJsonValue> Value;    // null pointer: the field is removed (missing legacy field)
		bool bRetain = false;            // malformed/future: PassageSave must return it verbatim
		bool bExit = false;              // expected exitReady right after load
		double Clock = -1;               // >= 0: test-only case.clock fixture
		bool bStarts = false;            // a genuine saved exit: must start running from 0 on free outdoor play
		bool bRoundTrip = false;         // also GameState -> disk -> reader keeps the raw value
	};

	class FPassageContract
	{
	public:
		FString Root, Tier;
		int32 Phase = 0, Checks = 0, Restores = 0, MaxHeads = 0, CaseI = 0, CrossSteps = 0, AfterAimed = 0;
		double T = 0, PhaseT = 0, Ev = 0, AimAt = -1, HeldE = -1, HeldPT = -1, HeldClock = 0, FreeHold = 0, SavedE = -1;
		int32 HeldHeads = 0, HeldSlots = 0, HeldStarted = 0, HeldMasked = 0, HeldHidden = 0, StartedAtDone = 0, Leg = 0;
		bool bDone = false, bBackground = false, bAimedDone = false, bAimShot = false, bInside = false, bLookLogged = false, bAimLogged = false;
		TArray<FString> ShotNames, Fixtures;
		TArray<FVector2D> Route; int32 WalkI = 0; FVector LastFoot = FVector::ZeroVector; double LastProgress = 0, LastWalkT = 0; bool bRun = false;
		S47J::FObj Presignal, PendingIndoors, ExitReady, DoneState;
		TArray<FLoadCase> Cases;

		US47WorldLife* L = nullptr; US47Main* M = nullptr; FS47Sky* Sky = nullptr;

		void Step(US47WorldLife* InL, US47Main* InM, double Dt);

	private:
		void Go(int32 Next) { Phase = Next; PhaseT = T; UE_LOG(LogS47MeteorPassage, Display, TEXT("S47: meteor passage contract phase %d at %.2f s (passage %s %.3f s, Sky T %.3f)"), Next, T, *L->PassagePhase(), L->PassageElapsed(), Sky->PassageT); }
		double In() const { return T - PhaseT; }
		void Finish(bool bPass, const FString& Why)
		{
			bDone = true;
			if (M && M->Player) { M->Player->TestMove.Reset(); M->Player->bTestRun = false; }
			const S47J::FObj O = S47J::New();
			S47J::Set(O, TEXT("status"), bPass ? TEXT("PASS") : TEXT("FAIL"));
			S47J::Set(O, TEXT("reason"), Why); S47J::Set(O, TEXT("phase"), Phase); S47J::Set(O, TEXT("checks"), Checks);
			S47J::Set(O, TEXT("tier"), Tier); S47J::Set(O, TEXT("maxHeads"), MaxHeads); S47J::Set(O, TEXT("restores"), Restores);
			S47J::Set(O, TEXT("screenshots"), ShotNames); S47J::Set(O, TEXT("testOnlyFixtures"), Fixtures);
			if (!Root.IsEmpty()) FFileHelper::SaveStringToFile(S47J::Write(O, true), *(Root / TEXT("result.json")));
		}
		bool Check(bool bGood, const FString& What)
		{
			++Checks;
			UE_LOG(LogS47MeteorPassage, Display, TEXT("S47: meteor passage contract %s: %s"), bGood ? TEXT("PASS") : TEXT("FAIL"), *What);
			if (bGood) return true;
			Finish(false, What);
			FPlatformMisc::RequestExitWithStatus(false, 1);
			return false;
		}
		// Per-frame invariants: only a violation is logged and counted.
		bool Hold(bool bGood, const FString& What) { return bGood || Check(false, What); }
		void Fixture(const FString& What)
		{
			Fixtures.Add(What);
			UE_LOG(LogS47MeteorPassage, Display, TEXT("S47: meteor passage contract TEST-ONLY fixture: %s"), *What);
		}
		void Shot(const FString& Name)
		{
			const FString File = FString::Printf(TEXT("%02d-%s-%s.png"), ShotNames.Num(), *Tier, *Name);
			FScreenshotRequest::RequestScreenshot(Root / File, false, false);
			ShotNames.Add(File);
			UE_LOG(LogS47MeteorPassage, Display, TEXT("S47: meteor passage screenshot %s at passage %.3f s: heads %d masked %d hidden %d slots %d; eye %s yaw %.3f pitch %.3f; %s"),
				*File, Sky->PassageT, Sky->PassageHeads, Sky->PassageMasked, Sky->PassageHidden, Sky->PassageSlotsVisible(), *M->Player->EyeUE().ToString(), M->Player->Yaw, M->Player->Pitch, *Sky->PassageDebug());
		}
		bool SaveDisk(const FString& Name, const S47J::FObj& State) { return State.IsValid() && FFileHelper::SaveStringToFile(S47J::Write(State, true), *(Root / Name)); }
		S47J::FObj LoadDisk(const FString& Name) const { FString Bytes; return FFileHelper::LoadFileToString(Bytes, *(Root / Name)) ? S47J::Read(Bytes) : S47J::FObj(); }
		void Restore(const S47J::FObj& State, const FString& Why)
		{
			US47Flow::FStart St; St.State = State; St.bDev = true; St.CaseId = M->Flow->CaseId; St.Playtime = M->Playtime;
			M->Flow->StartGame(St); ++Restores;
			UE_LOG(LogS47MeteorPassage, Display, TEXT("S47: meteor passage contract restored through Flow::StartGame: %s -> %s %.3f s"), *Why, *L->PassagePhase(), L->PassageElapsed());
		}
		FPv Saved() const { return ParseV2(L->PassageSave()); }
		bool Indoor() const { return M->Space == TEXT("room") || M->Space == TEXT("lab"); }
		bool Saro() const { return M->World && M->World->Area == TEXT("saro"); }
		// Life's free outdoor gate, observed from outside (S47WorldLife.cpp PassageStep: Free).
		bool FreeOutdoor() const
		{
			return M->Mode == US47Main::EMode::Play && !M->IsFreeform() && !M->Flow->bStarting && !S47Startup::Blocking() && !M->Story->IsCinematic()
				&& !M->World->bBusy && !M->World->bDriving && Saro() && M->Space == TEXT("yard") && !M->ModalOpen() && !M->bPausedByMenu && !M->Story->IsPaused();
		}
		bool HiddenNow() const { return Sky->PassageT < 0 && Sky->PassageHeads == 0 && Sky->PassageSlotsVisible() == 0; }
		// Saved evidence (Seen, album photos, notes) of the state just loaded must still be in the live state.
		bool Evidence(const S47J::FObj& Loaded, const FString& Name)
		{
			const S47J::FObj Now = M->Flow->GameState();
			const TArray<FString> SeenA = Texts(S47J::Arr(Loaded, TEXT("seen"))), SeenB = Texts(S47J::Arr(Now, TEXT("seen")));
			bool bOk = true; int32 Missing = 0;
			for (const FString& S : SeenA) if (!SeenB.Contains(S)) { bOk = false; ++Missing; }
			const TSharedPtr<FJsonValue> AlbumA = Loaded->TryGetField(TEXT("album")), AlbumB = Now->TryGetField(TEXT("album"));
			if (AlbumA && AlbumA->Type == EJson::Array && AlbumB && AlbumB->Type == EJson::Array)
			{
				const TArray<FString> A = Texts(AlbumA->AsArray()), B = Texts(AlbumB->AsArray());
				for (const FString& S : A) if (!B.Contains(S)) { bOk = false; ++Missing; }
			}
			else if (RawText(AlbumA) != RawText(AlbumB)) { bOk = false; ++Missing; }
			// notes: the saved ones in order (a load may append its own legacy note, never remove or change one)
			const TArray<FString> NotesA = S47J::Strs(S47J::Obj(Loaded, TEXT("case")), TEXT("notes")), NotesB = S47J::Strs(S47J::Obj(Now, TEXT("case")), TEXT("notes"));
			int32 J = 0;
			for (const FString& N : NotesA) { while (J < NotesB.Num() && NotesB[J] != N) ++J; if (J >= NotesB.Num()) { bOk = false; ++Missing; break; } ++J; }
			return Check(bOk, FString::Printf(TEXT("%s: saved Seen (%d), album and notes (%d) unchanged by the passage load (%d missing)"), *Name, SeenA.Num(), NotesA.Num(), Missing));
		}
		void Face(bool bAim)
		{
			// the hole's world direction (the sky turned by the area's yaw), as the old fixture computed it
			const FVector D0 = S47SkyDir(STAR_HOLE.Az, STAR_HOLE.El);
			const double Yw = M->World->SkyYaw();
			const FVector D(D0.X * FMath::Cos(Yw) + D0.Z * FMath::Sin(Yw), D0.Y, -D0.X * FMath::Sin(Yw) + D0.Z * FMath::Cos(Yw));
			M->Player->Yaw = FMath::Atan2(-D.X, -D.Z);
			M->Player->Pitch = bAim ? FMath::Asin(D.Y) : 0.0;
			if (bAim && !bAimLogged) { bAimLogged = true; Fixture(TEXT("aimed fixture: view yaw/pitch set straight at the star hole (026 / 32) for the occlusion image; not natural play")); }
			if (!bAim && !bLookLogged) { bLookLogged = true; Fixture(TEXT("natural view: standing eye height, view yaw turned towards azimuth 026 (as a mouse turn would), pitch 0; no look-up")); }
		}
		void StartWalk(const TArray<FVector2D>& R, bool bInRun)
		{
			Route = R; WalkI = 0; LastFoot = M->Player->Pos; LastProgress = LastWalkT = T; bRun = bInRun;
			M->Player->TestMove.Reset(); M->Player->bTestRun = false;
		}
		// 1 arrived, 0 walking, -1 failed. Native Walker input (TestMove) and its own collision.
		int32 Walk()
		{
			AS47Walker* P = M->Player;
			if (M->bPausedByMenu || M->ModalOpen() || M->Story->IsPaused() || M->Story->IsCinematic()) { P->TestMove.Reset(); LastFoot = P->Pos; LastProgress = LastWalkT = T; return 0; }
			const FVector At = P->Pos;
			const double Moved = FVector2D(At.X - LastFoot.X, At.Z - LastFoot.Z).Size();
			// A native Walker step scales with the actual frame duration; area loading can make a
			// frame longer than 0.2 s. Bound travel by its real run speed, not a fixed distance.
			const double WalkDt = FMath::Max(0.0, T - LastWalkT), Budget = FMath::Max(.05, P->Speed * 1.6 * WalkDt + .05);
			LastWalkT = T;
			if (Moved > Budget) { Check(false, FString::Printf(TEXT("walk moved %.2f m in %.3f s (Walker budget %.2f m)"), Moved, WalkDt, Budget)); return -1; }
			LastFoot = At; if (Moved > .002) LastProgress = T;
			if (WalkI >= Route.Num()) { P->TestMove.Reset(); P->bTestRun = false; return 1; }
			const FVector2D D = Route[WalkI] - FVector2D(At.X, At.Z);
			if (D.Size() < .12) { ++WalkI; P->TestMove.Reset(); LastProgress = T; return 0; }
			if (T - LastProgress > 5) { Check(false, FString::Printf(TEXT("native Walker path blocked at %.2f %.2f towards %.2f %.2f (doorEast open %d)"), At.X, At.Z, Route[WalkI].X, Route[WalkI].Y, M->Web->IsDoorOpen(TEXT("doorEast")))); return -1; }
			P->Yaw = FMath::Atan2(-D.X, -D.Y); P->Pitch = -.06; P->bTestRun = bRun;
			P->TestMove = FVector2D(0, -FMath::Min(1., D.Size() * 2));
			return 0;
		}
		bool EnsureDoor()
		{
			if (!M->Web->IsDoorOpen(TEXT("doorEast")) && M->Inter) { M->Inter->UseId(TEXT("doorEast")); UE_LOG(LogS47MeteorPassage, Display, TEXT("S47: meteor passage contract uses the east service door (production interaction)")); }
			return Check(M->Web->IsDoorOpen(TEXT("doorEast")), TEXT("the east service door is open for the walk"));
		}
		// The renderer sees exactly Life's time whenever Life presents it.
		bool Monitor()
		{
			if (Sky->PassageT >= 0)
			{
				MaxHeads = FMath::Max(MaxHeads, Sky->PassageHeads);
				if (Sky->PassageActive()) bBackground |= Sky->MeteorVisible();
				if (!Hold(L->PassagePhase() == TEXT("running") && Saro() && M->Space == TEXT("yard"), TEXT("streaks are drawn only while running, in SARO, outside"))) return false;
				if (!Hold(FMath::IsNearlyEqual(Sky->PassageT, FMath::Min(L->PassageElapsed(), End), 1e-9), FString::Printf(TEXT("renderer time %.4f is Life's elapsed %.4f"), Sky->PassageT, L->PassageElapsed()))) return false;
				if (!Hold(Sky->PassageHeads <= FS47Sky::PassageMaxHeads(Sky->bPassageLow) && Sky->PassageSlotsVisible() <= FS47Sky::PASSAGE_SLOTS, TEXT("heads within the tier's schedule cap and the strip pool"))) return false;
			}
			return true;
		}
		double NextHidden(double After) const
		{
			TArray<double> H = FS47Sky::PassageHiddenMoments(Sky->bPassageLow); H.Sort();
			for (double V : H) if (V >= After && V < End - .5) return V;
			return -1;
		}
		void BuildCases();
	};

	enum : int32
	{
		PSetup, PPresignal, PProloguePending, PCh1Indoors, PMenuIndoors, PRestorePending, PHoldPendingOutdoors, PWalkIn, PWalkToDoor,
		PCrossHeld, PHeldExit, PHeldExitMenu, PStart, PWalkOut, PNaturalA, PAimed, PAimedHeld, PMenuOut, PMenuOutHeld, PMenuResumed,
		PNaturalB, PRestoreRunning, PResumeRunning, PArea, PAreaHeld, PAreaBack, PWalkInRunning, PInsideHold, PWalkOutRunning, PAimLate,
		PToDone, PAfterDone, PDoneRepeat, PRestoreDone, PDoneHeld, PCases, PCaseHold
	};
}

void FPassageContract::BuildCases()
{
	auto N = [](double V) { return TSharedPtr<FJsonValue>(MakeShared<FJsonValueNumber>(V)); };
	auto B = [](bool V) { return TSharedPtr<FJsonValue>(MakeShared<FJsonValueBoolean>(V)); };
	auto S = [](const TCHAR* V) { return TSharedPtr<FJsonValue>(MakeShared<FJsonValueString>(V)); };
	auto V1 = [&N, &B](bool bSpent)
	{
		const S47J::FObj O = S47J::New(); O->SetField(TEXT("v"), N(1)); O->SetField(TEXT("done"), B(bSpent));
		return TSharedPtr<FJsonValue>(MakeShared<FJsonValueObject>(O));
	};
	auto Obj4 = [&N](double Ver, const TSharedPtr<FJsonValue>& Ph, const TSharedPtr<FJsonValue>& El, const TSharedPtr<FJsonValue>& Ex, const TSharedPtr<FJsonValue>& Keep)
	{
		const S47J::FObj O = S47J::New(); O->SetField(TEXT("v"), N(Ver)); O->SetField(TEXT("phase"), Ph); O->SetField(TEXT("elapsed"), El); O->SetField(TEXT("exitReady"), Ex);
		if (Keep) O->SetField(TEXT("keep"), Keep);
		return TSharedPtr<FJsonValue>(MakeShared<FJsonValueObject>(O));
	};
	const double Spent = S47J::Num(S47J::Obj(DoneState, TEXT("case")), TEXT("clock"), -1);
	auto Add = [this](FLoadCase C) { Cases.Add(MoveTemp(C)); };
	{ FLoadCase C; C.Name = TEXT("v2 pending with a real saved exit"); C.Base = TEXT("exitready"); C.Expect = TEXT("pending"); C.bExit = true; C.bStarts = true; C.Value = ExitReady->TryGetField(TEXT("meteorPassage")); Add(C); }
	{ FLoadCase C; C.Name = TEXT("v1 done:true"); C.Base = TEXT("done"); C.Expect = TEXT("done"); C.Value = V1(true); Add(C); }
	{ FLoadCase C; C.Name = FString::Printf(TEXT("missing field after the signal, spent by its clock (event + %.1f s)"), Spent - Ev); C.Base = TEXT("done"); C.Expect = TEXT("done"); Add(C); }
	{ FLoadCase C; C.Name = TEXT("v1 done:false after the signal, spent by its clock"); C.Base = TEXT("done"); C.Expect = TEXT("done"); C.Value = V1(false); Add(C); }
	{ FLoadCase C; C.Name = TEXT("missing field after the signal, inside the old 11.2 s window (case.clock = event + 8 s)"); C.Base = TEXT("done"); C.Expect = TEXT("pending"); C.Clock = Ev + 8; Add(C); }
	{ FLoadCase C; C.Name = TEXT("malformed JSON null"); C.Base = TEXT("done"); C.Expect = TEXT("done"); C.bRetain = true; C.Value = MakeShared<FJsonValueNull>(); Add(C); }
	{ FLoadCase C; C.Name = TEXT("malformed number"); C.Base = TEXT("done"); C.Expect = TEXT("done"); C.bRetain = true; C.Value = N(7); Add(C); }
	{ FLoadCase C; C.Name = TEXT("malformed array"); C.Base = TEXT("done"); C.Expect = TEXT("done"); C.bRetain = true; C.Value = MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{ N(1), S(TEXT("x")) }); Add(C); }
	{ FLoadCase C; C.Name = TEXT("v2 with a string elapsed (no coercion)"); C.Base = TEXT("done"); C.Expect = TEXT("done"); C.bRetain = true; C.Value = Obj4(2, S(TEXT("running")), S(TEXT("5")), B(true), nullptr); Add(C); }
	{ FLoadCase C; C.Name = TEXT("v2 with an unknown phase"); C.Base = TEXT("done"); C.Expect = TEXT("done"); C.bRetain = true; C.Value = Obj4(2, S(TEXT("flying")), N(1), B(false), nullptr); Add(C); }
	{ FLoadCase C; C.Name = TEXT("future v3 object"); C.Base = TEXT("done"); C.Expect = TEXT("done"); C.bRetain = true; C.bRoundTrip = true; C.Value = Obj4(3, S(TEXT("running")), N(5), B(true), S(TEXT("future data"))); Add(C); }
	{ FLoadCase C; C.Name = TEXT("real pre-signal save as written (v2 idle)"); C.Base = TEXT("presignal"); C.Expect = TEXT("idle"); C.Value = Presignal->TryGetField(TEXT("meteorPassage")); Add(C); }
	{ FLoadCase C; C.Name = TEXT("pre-signal save, missing field"); C.Base = TEXT("presignal"); C.Expect = TEXT("idle"); Add(C); }
	{ FLoadCase C; C.Name = TEXT("pre-signal save, v1 done:false"); C.Base = TEXT("presignal"); C.Expect = TEXT("idle"); C.Value = V1(false); Add(C); }
}

void FPassageContract::Step(US47WorldLife* InL, US47Main* InM, double Dt)
{
	L = InL; M = InM;
	if (bDone || !L || !M || !L->Sky) return;
	if (!M->Story || !M->Flow || !M->World || !M->Player || !M->Web || M->Mode != US47Main::EMode::Play) return;
	Sky = L->Sky.Get();
	T += Dt;
	Tier = Sky->bPassageLow ? TEXT("Low") : TEXT("High");
	if (T > 480) { Check(false, FString::Printf(TEXT("bounded contract timed out in phase %d"), Phase)); return; }
	const double Limit = Phase == PProloguePending ? 150 : Phase == PToDone ? End + 20 : 60;
	if (In() > Limit) { Check(false, FString::Printf(TEXT("phase %d took longer than %.0f s"), Phase, Limit)); return; }
	if (!Monitor()) return;
	const FString LP = L->PassagePhase();

	if (Phase == PSetup)
	{
		FString User, Quality;
		FParse::Value(FCommandLine::Get(), TEXT("UserDir="), User);
		FParse::Value(FCommandLine::Get(), TEXT("S47MeteorPassageProof="), Root);
		if (!Check(Root.StartsWith(TEXT("/tmp/")) && User.StartsWith(TEXT("/tmp/")), TEXT("isolated /tmp UserDir and /tmp proof directory"))) return;
		IFileManager::Get().MakeDirectory(*Root, true);
		if (FParse::Value(FCommandLine::Get(), TEXT("S47Quality="), Quality))
			if (!Check(Sky->bPassageLow == (Quality.ToLower() == TEXT("low") || Quality.ToLower() == TEXT("verylow") || Quality.ToLower() == TEXT("medium")), FString::Printf(TEXT("-S47Quality=%s gives the %s passage tier"), *Quality, *Tier))) return;
		const FPv P = Saved();
		if (!Check(LP == TEXT("idle") && L->PassageElapsed() == 0 && P.bV2 && P.Phase == TEXT("idle") && !P.bExit && Sky->PassageStarted == 0 && Sky->PassageT < 0, TEXT("a new night starts idle and saves v2 idle"))) return;
		if (!Check(Sky->PassageSlotsBuilt() == FS47Sky::PASSAGE_SLOTS, FString::Printf(TEXT("%d passage strips built with the live sky"), Sky->PassageSlotsBuilt()))) return;
		TArray<FString> Lines; const bool bSelf = FS47Sky::PassageSelfTest(Lines);
		for (const FString& Ln : Lines) UE_LOG(LogS47MeteorPassage, Display, TEXT("S47: passage selftest: %s"), *Ln);
		const bool bLow = Sky->bPassageLow;
		if (!Check(bSelf && FS47Sky::PassageCount(bLow) > 0 && FS47Sky::PassageCrossers(bLow) >= 3 && FS47Sky::PassageHiddenMoments(bLow).Num() >= 3 && (bLow || FS47Sky::PassageMaxHeads(false) >= 3),
			FString::Printf(TEXT("%s schedule: %d streaks, %d crossing behind the field, %d hidden moments, at most %d at once"), *Tier, FS47Sky::PassageCount(bLow), FS47Sky::PassageCrossers(bLow), FS47Sky::PassageHiddenMoments(bLow).Num(), FS47Sky::PassageMaxHeads(bLow)))) return;
		Go(PPresignal);
		return;
	}
	if (Phase == PPresignal)
	{
		if (!M->Story->At(ES47Phase::Turning))
		{
			// The event has a real EventClock already; without the turning dishes nothing is armed.
			if (!Hold(LP == TEXT("idle") && HiddenNow() && Sky->HoleFrom == TNumericLimits<double>::Max(), FString::Printf(TEXT("before the dishes turn: idle and no hole (phase %s, story at event %d)"), *LP, M->Story->At(ES47Phase::Event)))) return;
			if (!Presignal && M->Story->At(ES47Phase::Event) && !M->Story->IsCinematic())
			{
				const S47J::FObj State = M->Flow->GameState();
				const FPv P = ParseV2(State->TryGetField(TEXT("meteorPassage")));
				if (!Check(S47J::Str(State, TEXT("checkpoint")) == TEXT("residual") && P.bV2 && P.Phase == TEXT("idle") && SaveDisk(TEXT("presignal-residual.json"), State), TEXT("real pre-signal state (event, dishes not turned) written to disk with v2 idle"))) return;
				Presignal = LoadDisk(TEXT("presignal-residual.json"));
				if (!Check(Presignal.IsValid(), TEXT("fresh reader parses the pre-signal save"))) return;
			}
			return;
		}
		Ev = M->Story->EventClockSec();
		const FPv P = Saved();
		if (!Check(Presignal.IsValid(), TEXT("the pre-signal save was captured before the dishes turned"))) return;
		if (!Check(LP == TEXT("pending") && P.bV2 && P.Phase == TEXT("pending") && !P.bExit && L->PassageElapsed() == 0 && Indoor() && HiddenNow(),
			FString::Printf(TEXT("the genuine signal (dishes turning) arms pending while indoors (Space %s), nothing drawn"), *M->Space))) return;
		if (!Check(Sky->HoleFrom == Ev + STAR_HOLE.After, FString::Printf(TEXT("HoleFrom %.2f is the real EventClock %.2f + %.0f s"), Sky->HoleFrom, Ev, STAR_HOLE.After))) return;
		Go(PProloguePending);
		return;
	}
	if (Phase == PProloguePending)
	{
		const FPv P = Saved();
		if (!Hold(LP == TEXT("pending") && !P.bExit && HiddenNow() && Sky->PassageStarted == 0 && M->Space != TEXT("yard"), FString::Printf(TEXT("through the turning/card the passage stays pending indoors (phase %s, Space %s)"), *LP, *M->Space))) return;
		if (M->Story->ChapterKey != TEXT("ch1") || M->Story->IsCinematic() || M->ModalOpen() || !M->Flow->GameSaveBlock().IsEmpty() || M->Flow->CheckpointName() != TEXT("chapter1") || M->Space != TEXT("room")) return;
		Go(PCh1Indoors);
		return;
	}
	if (Phase == PCh1Indoors)
	{
		if (In() < .6) return;
		const FPv P = Saved();
		if (!Check(LP == TEXT("pending") && !P.bExit && M->Space == TEXT("room") && HiddenNow(), TEXT("chapter one begins in the control room with the passage still pending, unconsumed"))) return;
		const S47J::FObj State = M->Flow->GameState();
		const FPv D = ParseV2(State->TryGetField(TEXT("meteorPassage")));
		if (!Check(D.bV2 && D.Phase == TEXT("pending") && D.Elapsed == 0 && !D.bExit && SaveDisk(TEXT("pending-indoors.json"), State), TEXT("real chapter-one save written with meteorPassage {v:2, pending, 0, exitReady:false}"))) return;
		PendingIndoors = LoadDisk(TEXT("pending-indoors.json"));
		if (!Check(PendingIndoors.IsValid(), TEXT("fresh reader parses the pending save"))) return;
		M->Flow->OpenPause();
		if (!Check(M->bPausedByMenu && M->ModalOpen(), TEXT("the production pause menu opens indoors"))) return;
		Go(PMenuIndoors);
		return;
	}
	if (Phase == PMenuIndoors)
	{
		if (!Hold(LP == TEXT("pending") && !Saved().bExit && HiddenNow() && Sky->PassageStarted == 0, TEXT("pending is not consumed under the indoor menu"))) return;
		if (In() < 1.0) return;
		M->UI->Close();
		if (!Check(!M->bPausedByMenu && !M->ModalOpen() && LP == TEXT("pending"), TEXT("Resume closes the menu; still pending"))) return;
		Go(PRestorePending);
		return;
	}
	if (Phase == PRestorePending)
	{
		const S47J::FObj State = S47J::Clone(PendingIndoors);
		const S47J::FObj Pose = S47J::New(); S47J::Set(Pose, TEXT("x"), Yard.X); S47J::Set(Pose, TEXT("z"), Yard.Y); S47J::Set(Pose, TEXT("yaw"), 0.0); S47J::Set(Pose, TEXT("pitch"), 0.0);
		State->SetObjectField(TEXT("pose"), Pose);
		Fixture(TEXT("pending save's pose edited to the open yard (9.3, -9) so the load itself starts outdoors"));
		Restore(State, TEXT("pending-indoors.json with an outdoor pose"));
		const FPv P = Saved();
		if (!Check(L->PassagePhase() == TEXT("pending") && P.bV2 && !P.bExit && L->PassageElapsed() == 0 && Sky->PassageT < 0, TEXT("disk pending restores pending, exitReady false, hidden until sampled"))) return;
		if (!Evidence(State, TEXT("pending restore"))) return;
		FreeHold = 0;
		Go(PHoldPendingOutdoors);
		return;
	}
	if (Phase == PHoldPendingOutdoors)
	{
		const FPv P = Saved();
		if (!Hold(L->PassagePhase() == TEXT("pending") && !P.bExit && HiddenNow() && Sky->PassageStarted == 0, TEXT("a load outdoors is a seed, never an exit: no latch, no start"))) return;
		if (FreeOutdoor()) FreeHold += Dt;
		if (FreeHold < 1.5) return;
		if (!Check(FVector2D(M->Player->Pos.X - Yard.X, M->Player->Pos.Z - Yard.Y).Size() < .3 && M->Space == TEXT("yard"),
			FString::Printf(TEXT("1.5 s of free outdoor play after the load (hole %.2f) stayed pending without an exit"), L->HoleNow()))) return;
		if (!EnsureDoor()) return;
		StartWalk({ EastWalk, Landing, DoorPlane, DoorIn, InRoom }, true);
		bInside = false;
		Go(PWalkIn);
		return;
	}
	if (Phase == PWalkIn)
	{
		if (Indoor()) bInside = true;
		if (!Hold(L->PassagePhase() == TEXT("pending") && !Saved().bExit && HiddenNow(), TEXT("walking in does not latch an exit"))) return;
		const int32 W = Walk(); if (W < 0) return; if (W == 0) return;
		if (!Check(bInside && M->Space == TEXT("room"), TEXT("walked back into the control room through the east door"))) return;
		StartWalk({ FVector2D(5.5, 1.6) }, false);
		Go(PWalkToDoor);
		return;
	}
	if (Phase == PWalkToDoor)
	{
		const int32 W = Walk(); if (W < 0) return; if (W == 0) return;
		if (!Check(M->Space == TEXT("room") && L->PassagePhase() == TEXT("pending"), TEXT("standing just inside the east door, still pending"))) return;
		M->Story->SetPaused(true);
		Fixture(TEXT("story held (what an open panel does) while the last steps through the door are taken with the Walker's own MoveBy collision path"));
		CrossSteps = 0;
		Go(PCrossHeld);
		return;
	}
	if (Phase == PCrossHeld)
	{
		if (M->Space != TEXT("yard"))
		{
			if (!Hold(L->PassagePhase() == TEXT("pending") && !Saved().bExit, TEXT("no latch before the real crossing"))) return;
			if (++CrossSteps > 80) { Check(false, FString::Printf(TEXT("MoveBy through the east door never reached the yard (at %.2f %.2f)"), M->Player->Pos.X, M->Player->Pos.Z)); return; }
			M->Player->MoveBy(.04, 0);
			return;
		}
		const FPv P = Saved();
		if (!Check(L->PassagePhase() == TEXT("pending") && P.bExit && HiddenNow() && Sky->PassageStarted == 0 && L->PassageElapsed() == 0,
			FString::Printf(TEXT("room -> yard on foot (Main's own Space) latches exitReady; held story does not start it (at %.2f %.2f)"), M->Player->Pos.X, M->Player->Pos.Z))) return;
		const S47J::FObj State = M->Flow->GameState();
		const FPv D = ParseV2(State->TryGetField(TEXT("meteorPassage")));
		if (!Check(D.bV2 && D.Phase == TEXT("pending") && D.bExit && SaveDisk(TEXT("pending-exitready.json"), State), TEXT("the latched exit is saved as {v:2, pending, exitReady:true}"))) return;
		ExitReady = LoadDisk(TEXT("pending-exitready.json"));
		if (!Check(ExitReady.IsValid(), TEXT("fresh reader parses the latched-exit save"))) return;
		Go(PHeldExit);
		return;
	}
	if (Phase == PHeldExit)
	{
		if (!Hold(L->PassagePhase() == TEXT("pending") && Saved().bExit && HiddenNow() && Sky->PassageStarted == 0, TEXT("a held story keeps the exit latched without starting"))) return;
		if (In() < 1.0) return;
		M->Flow->OpenPause();
		if (!Check(M->bPausedByMenu && M->ModalOpen(), TEXT("the production pause menu opens on the latched exit"))) return;
		Go(PHeldExitMenu);
		return;
	}
	if (Phase == PHeldExitMenu)
	{
		if (!Hold(L->PassagePhase() == TEXT("pending") && Saved().bExit && HiddenNow() && Sky->PassageStarted == 0, TEXT("the menu holds the latched exit without starting"))) return;
		if (In() < .6) return;
		M->UI->Close();
		M->Story->SetPaused(false);
		if (!Check(!M->bPausedByMenu && !M->ModalOpen() && !M->Story->IsPaused(), TEXT("menu resumed and the test-only story hold released"))) return;
		Go(PStart);
		return;
	}
	if (Phase == PStart)
	{
		if (L->PassagePhase() == TEXT("pending"))
		{
			if (!Hold(Saved().bExit && HiddenNow(), TEXT("waiting for the hole keeps the latch and draws nothing"))) return;
			return;
		}
		if (!Check(L->PassagePhase() == TEXT("running") && FreeOutdoor() && L->HoleNow() >= .9 && L->PassageElapsed() < .1 && Sky->PassageT >= 0,
			FString::Printf(TEXT("free outdoor play with the hole at %.2f starts the passage at %.3f s"), L->HoleNow(), L->PassageElapsed()))) return;
		if (!Check(FMath::IsNearlyEqual(L->HoleNow(), Sky->HoleK, 1e-12), TEXT("HoleNow is the sky's HoleK"))) return;
		StartWalk({ EastWalk, Yard }, true);
		Go(PWalkOut);
		return;
	}
	if (Phase == PWalkOut)
	{
		if (!Hold(L->PassagePhase() == TEXT("running"), TEXT("running while walking out"))) return;
		const int32 W = Walk(); if (W < 0) return; if (W == 0) return;
		Face(false);
		Go(PNaturalA);
		return;
	}
	if (Phase == PNaturalA)
	{
		if (In() < .5 || Sky->PassageHeads < 1) { if (In() > 6) Check(false, TEXT("no streak drawn within 6 s at the natural spot")); return; }
		Shot(FString::Printf(TEXT("natural-standing-%04.1fs-%dheads"), Sky->PassageT, Sky->PassageHeads));
		const double H = NextHidden(L->PassageElapsed() + 1.5);
		if (H > 0 && H <= L->PassageElapsed() + 8) { AimAt = H; AfterAimed = PMenuOut; Go(PAimed); }
		else Go(PMenuOut);
		return;
	}
	if (Phase == PAimed)
	{
		if (!Hold(L->PassagePhase() == TEXT("running"), TEXT("running before the aimed fixture"))) return;
		if (L->PassageElapsed() < AimAt - 1.5) return;
		Face(true);
		if (Sky->PassageHidden > 0 && Sky->PassageT >= AimAt - .6)
		{
			M->Story->SetPaused(true);
			Fixture(FString::Printf(TEXT("story held at passage %.3f s (scheduled hidden moment %.3f s) for the aimed occlusion image"), Sky->PassageT, AimAt));
			HeldPT = Sky->PassageT; HeldHeads = Sky->PassageHeads; HeldMasked = Sky->PassageMasked; HeldHidden = Sky->PassageHidden;
			Go(PAimedHeld);
			return;
		}
		if (L->PassageElapsed() > AimAt + .6) Check(false, FString::Printf(TEXT("no wholly hidden streak reported by the sky around the scheduled hidden moment %.3f s"), AimAt));
		return;
	}
	if (Phase == PAimedHeld)
	{
		if (!Hold(Sky->PassageT == HeldPT && Sky->PassageHidden == HeldHidden && Sky->PassageHeads == HeldHeads, TEXT("the held aimed moment does not move"))) return;
		if (In() >= .25 && !bAimShot)
		{
			bAimShot = true;
			if (!Check(Sky->PassageHidden > 0 && Sky->PassageMasked > 0 && (Sky->bPassageLow || Sky->PassageHeads >= 3) && FMath::Abs(Sky->PassageT - AimAt) < .6,
				FString::Printf(TEXT("aimed fixture at %.3f s: %d streak(s) wholly behind the field (head and tail), %d reaching its rim, %d drawn at once"), Sky->PassageT, Sky->PassageHidden, Sky->PassageMasked, Sky->PassageHeads))) return;
			Shot(FString::Printf(TEXT("aimed-fixture-hidden-%04.1fs-%dheads-%dhidden"), Sky->PassageT, Sky->PassageHeads, Sky->PassageHidden));
		}
		if (In() < .6) return;
		M->Story->SetPaused(false);
		bAimedDone = true;
		Face(false);
		Go(AfterAimed);
		return;
	}
	if (Phase == PMenuOut)
	{
		if (In() < .2) return;
		M->Flow->OpenPause();
		if (!Check(M->bPausedByMenu && M->ModalOpen() && L->PassagePhase() == TEXT("running") && Sky->PassageT >= 0, TEXT("the production pause menu opens outdoors during the passage"))) return;
		HeldE = L->PassageElapsed(); HeldPT = Sky->PassageT; HeldHeads = Sky->PassageHeads; HeldSlots = Sky->PassageSlotsVisible(); HeldStarted = Sky->PassageStarted;
		HeldMasked = Sky->PassageMasked; HeldHidden = Sky->PassageHidden; HeldClock = M->Story->Clock();
		Go(PMenuOutHeld);
		return;
	}
	if (Phase == PMenuOutHeld)
	{
		if (!Hold(L->PassageElapsed() == HeldE && Sky->PassageT == HeldPT && Sky->PassageHeads == HeldHeads && Sky->PassageSlotsVisible() == HeldSlots && Sky->PassageStarted == HeldStarted
			&& Sky->PassageMasked == HeldMasked && Sky->PassageHidden == HeldHidden && M->Story->Clock() == HeldClock && L->PassagePhase() == TEXT("running"),
			FString::Printf(TEXT("the outdoor menu freezes elapsed %.3f and the image (%d streaks, %d strips)"), HeldE, HeldHeads, HeldSlots))) return;
		if (In() < 1.2) return;
		M->UI->Close();
		if (!Check(!M->bPausedByMenu && !M->ModalOpen(), FString::Printf(TEXT("held 1.2 s under the menu at %.3f s; Resume"), HeldE))) return;
		Go(PMenuResumed);
		return;
	}
	if (Phase == PMenuResumed)
	{
		if (In() < .6) return;
		if (!Check(L->PassagePhase() == TEXT("running") && L->PassageElapsed() > HeldE && L->PassageElapsed() <= HeldE + In() + .05,
			FString::Printf(TEXT("after Resume the same passage continues from %.3f to %.3f s (no restart)"), HeldE, L->PassageElapsed()))) return;
		Go(PNaturalB);
		return;
	}
	if (Phase == PNaturalB)
	{
		if (In() < .3 || Sky->PassageHeads < 1) { if (In() > 6) Check(false, TEXT("no streak drawn within 6 s for the second natural image")); return; }
		Shot(FString::Printf(TEXT("natural-standing-%04.1fs-%dheads"), Sky->PassageT, Sky->PassageHeads));
		Go(PRestoreRunning);
		return;
	}
	if (Phase == PRestoreRunning)
	{
		const S47J::FObj State = M->Flow->GameState();
		const FPv D = ParseV2(State->TryGetField(TEXT("meteorPassage")));
		if (!Check(D.bV2 && D.Phase == TEXT("running") && D.Elapsed == L->PassageElapsed() && !D.bExit && SaveDisk(TEXT("running.json"), State),
			FString::Printf(TEXT("real outdoor save written with {v:2, running, %.3f}"), D.Elapsed))) return;
		const S47J::FObj Disk = LoadDisk(TEXT("running.json"));
		if (!Check(Disk.IsValid(), TEXT("fresh reader parses the running save"))) return;
		SavedE = ParseV2(Disk->TryGetField(TEXT("meteorPassage"))).Elapsed;
		Restore(Disk, TEXT("running.json"));
		if (!Check(L->PassagePhase() == TEXT("running") && FMath::IsNearlyEqual(L->PassageElapsed(), SavedE, 1e-6) && Sky->PassageT < 0,
			FString::Printf(TEXT("disk running restores running at %.3f s, hidden until its place is sampled"), L->PassageElapsed()))) return;
		if (!Evidence(Disk, TEXT("running restore"))) return;
		Go(PResumeRunning);
		return;
	}
	if (Phase == PResumeRunning)
	{
		if (Sky->PassageT < 0) return;
		if (!Check(L->PassagePhase() == TEXT("running") && M->Space == TEXT("yard") && Sky->PassageT >= SavedE - 1e-6 && Sky->PassageT <= SavedE + .25,
			FString::Printf(TEXT("the restored passage resumes at %.3f s (saved %.3f), never from the start"), Sky->PassageT, SavedE))) return;
		Face(false);
		Go(PArea);
		return;
	}
	if (Phase == PArea)
	{
		if (In() < .3) return;
		M->World->Enter(TEXT("road"));
		Fixture(TEXT("World::Enter(road) without travel, standing still, to put the player in another area; then Enter(saro) and Place at the same spot"));
		HeldE = L->PassageElapsed();
		Go(PAreaHeld);
		return;
	}
	if (Phase == PAreaHeld)
	{
		if (In() > .1 && !Hold(M->World->Area == TEXT("road") && HiddenNow() && L->PassageElapsed() == HeldE && L->PassagePhase() == TEXT("running"),
			FString::Printf(TEXT("in another area (Space %s) the passage is hidden and held at %.3f s"), *M->Space, HeldE))) return;
		if (In() < 1.5) return;
		const FVector P = M->Player->Pos; const double Yaw = M->Player->Yaw;
		M->World->Enter(TEXT("saro"));
		M->Player->Place(P.X, P.Z, Yaw); Face(false);
		Go(PAreaBack);
		return;
	}
	if (Phase == PAreaBack)
	{
		if (Sky->PassageT < 0) return;
		if (!Check(L->PassagePhase() == TEXT("running") && Sky->PassageT >= HeldE - 1e-9 && Sky->PassageT <= HeldE + .25,
			FString::Printf(TEXT("back in SARO the same passage resumes at %.3f s (held %.3f)"), Sky->PassageT, HeldE))) return;
		if (!EnsureDoor()) return;
		StartWalk({ EastWalk, Landing, DoorPlane, DoorIn, InRoom }, true);
		bInside = false;
		Go(PWalkInRunning);
		return;
	}
	if (Phase == PWalkInRunning || Phase == PInsideHold || Phase == PWalkOutRunning)
	{
		if (!Hold(L->PassagePhase() == TEXT("running"), FString::Printf(TEXT("the passage is still running for the inside hold (phase %s at %.3f s)"), *L->PassagePhase(), L->PassageElapsed()))) return;
		if (Indoor())
		{
			if (!bInside) { bInside = true; HeldE = L->PassageElapsed(); UE_LOG(LogS47MeteorPassage, Display, TEXT("S47: meteor passage contract: inside at %.3f s"), HeldE); }
			if (!Hold(HiddenNow() && L->PassageElapsed() == HeldE, FString::Printf(TEXT("inside the building the passage is hidden and held at %.3f s"), HeldE))) return;
		}
		if (Phase == PWalkInRunning)
		{
			const int32 W = Walk(); if (W < 0) return; if (W == 0) return;
			if (!Check(bInside && M->Space == TEXT("room"), TEXT("walked into the control room during the passage"))) return;
			Go(PInsideHold);
			return;
		}
		if (Phase == PInsideHold)
		{
			if (In() < 2.0) return;
			if (!Check(L->PassageElapsed() == HeldE && HiddenNow(), FString::Printf(TEXT("2 s inside: hidden, elapsed held at %.3f s"), HeldE))) return;
			StartWalk({ DoorIn, DoorPlane, Landing, EastWalk, Yard }, true);
			Go(PWalkOutRunning);
			return;
		}
		if (M->Space == TEXT("yard") && Sky->PassageT >= 0 && bInside)
		{
			bInside = false;
			if (!Check(Sky->PassageT >= HeldE - 1e-9 && Sky->PassageT <= HeldE + .25, FString::Printf(TEXT("out again, the same passage resumes at %.3f s (held %.3f), not restarted"), Sky->PassageT, HeldE))) return;
		}
		const int32 W = Walk(); if (W < 0) return; if (W == 0) return;
		if (!Check(!bInside, TEXT("the re-exit was sampled outdoors"))) return;
		Face(false);
		if (bAimedDone) Go(PToDone); else Go(PAimLate);
		return;
	}
	if (Phase == PAimLate)
	{
		const double H = NextHidden(L->PassageElapsed() + 1.5);
		if (!Check(H > 0, FString::Printf(TEXT("a scheduled hidden moment remains after %.2f s for the aimed fixture"), L->PassageElapsed()))) return;
		AimAt = H; AfterAimed = PToDone;
		Go(PAimed);
		return;
	}
	if (Phase == PToDone)
	{
		if (L->PassagePhase() == TEXT("running")) return;
		const FPv P = Saved();
		if (!Check(L->PassagePhase() == TEXT("done") && L->PassageElapsed() == End && P.bV2 && P.Phase == TEXT("done") && P.Elapsed == End && Sky->bPassageDone && Sky->PassageT < 0,
			FString::Printf(TEXT("after %.1f s of presentation the passage is done (terminal) and saved done"), End))) return;
		const bool bLow = Sky->bPassageLow;
		if (!Check(MaxHeads >= (bLow ? 1 : 3) && MaxHeads <= FS47Sky::PassageMaxHeads(bLow) && Sky->PassageStarted <= FS47Sky::PassageCount(bLow) && Sky->PassageStarted > 0,
			FString::Printf(TEXT("%s: up to %d streaks at once (schedule cap %d), %d begun since the last load (of %d)"), *Tier, MaxHeads, FS47Sky::PassageMaxHeads(bLow), Sky->PassageStarted, FS47Sky::PassageCount(bLow)))) return;
		if (!Check(!bBackground, TEXT("no rare shooting star crossed while the passage was drawn"))) return;
		StartedAtDone = Sky->PassageStarted;
		Go(PAfterDone);
		return;
	}
	if (Phase == PAfterDone)
	{
		if (!Hold(L->PassagePhase() == TEXT("done") && HiddenNow(), TEXT("done stays done and draws nothing"))) return;
		if (In() < 1.5) return;
		Shot(TEXT("after-done"));
		if (!EnsureDoor()) return;
		StartWalk({ EastWalk, Landing, DoorPlane, DoorIn, InRoom }, true); Leg = 0;
		Go(PDoneRepeat);
		return;
	}
	if (Phase == PDoneRepeat)
	{
		if (!Hold(L->PassagePhase() == TEXT("done") && HiddenNow() && Sky->PassageStarted == StartedAtDone && !Saved().bExit, TEXT("another inside -> outside crossing after done replays nothing"))) return;
		if (Leg < 2) { const int32 W = Walk(); if (W < 0) return; if (W == 0) return; }
		if (Leg == 0) { if (!Check(M->Space == TEXT("room"), TEXT("inside again after the passage"))) return; StartWalk({ DoorIn, DoorPlane, Landing, EastWalk, Yard }, true); Leg = 1; return; }
		if (Leg == 1) { if (!Check(M->Space == TEXT("yard"), TEXT("outside again after the passage"))) return; Leg = 2; PhaseT = T; return; }
		if (In() < 1.5) return;
		Go(PRestoreDone);
		return;
	}
	if (Phase == PRestoreDone)
	{
		const S47J::FObj State = M->Flow->GameState();
		const FPv D = ParseV2(State->TryGetField(TEXT("meteorPassage")));
		if (!Check(D.bV2 && D.Phase == TEXT("done") && D.Elapsed == End && SaveDisk(TEXT("done.json"), State), TEXT("real outdoor save written with {v:2, done}"))) return;
		DoneState = LoadDisk(TEXT("done.json"));
		if (!Check(DoneState.IsValid(), TEXT("fresh reader parses the done save"))) return;
		Restore(S47J::Clone(DoneState), TEXT("done.json"));
		if (!Check(L->PassagePhase() == TEXT("done") && L->PassageElapsed() == End && HiddenNow() && Sky->PassageStarted == 0, TEXT("disk done restores done"))) return;
		if (!Evidence(DoneState, TEXT("done restore"))) return;
		M->Story->SetClock(Ev + S47SkyEvent::LegacyPassageStartAfter + 3);
		Fixture(TEXT("story clock set back to event + 7 s (inside the old clock window) after the done restore"));
		Go(PDoneHeld);
		return;
	}
	if (Phase == PDoneHeld)
	{
		if (!Hold(L->PassagePhase() == TEXT("done") && HiddenNow() && Sky->PassageStarted == 0, TEXT("a done restore with the clock put back replays nothing"))) return;
		if (In() < 2.0) return;
		if (!Check(S47J::Num(S47J::Obj(DoneState, TEXT("case")), TEXT("clock"), -1) - Ev > S47SkyEvent::LegacyPassageStartAfter + S47SkyEvent::LegacyPassageWindow, TEXT("fixture precondition: the done save's clock is past the old window"))) return;
		BuildCases(); CaseI = 0;
		Go(PCases);
		return;
	}
	if (Phase == PCases)
	{
		if (CaseI >= Cases.Num())
		{
			Finish(true, TEXT("complete"));
			UE_LOG(LogS47MeteorPassage, Display, TEXT("S47: meteor passage contract PASS complete (%d checks, %d screenshots, %d restores, %s)"), Checks, ShotNames.Num(), Restores, *Tier);
			M->Story->After(1., [] { FPlatformMisc::RequestExit(false); });
			return;
		}
		const FLoadCase& C = Cases[CaseI];
		const S47J::FObj Base = C.Base == TEXT("presignal") ? Presignal : C.Base == TEXT("exitready") ? ExitReady : DoneState;
		const S47J::FObj State = S47J::Clone(Base);
		if (C.Value) State->SetField(TEXT("meteorPassage"), C.Value); else State->RemoveField(TEXT("meteorPassage"));
		if (C.Clock >= 0)
		{
			const S47J::FObj Case = S47J::Obj(State, TEXT("case"));
			if (!Check(Case.IsValid(), TEXT("the chapter-one save has a case for the clock fixture"))) return;
			S47J::Set(Case, TEXT("clock"), C.Clock);
			Fixture(FString::Printf(TEXT("%s: saved case.clock set to %.2f (event + %.2f s)"), *C.Name, C.Clock, C.Clock - Ev));
		}
		Fixture(FString::Printf(TEXT("load case '%s': meteorPassage %s"), *C.Name, *RawText(C.Value)));
		Restore(State, C.Name);
		const FPv P = Saved();
		const TSharedPtr<FJsonValue> Out = L->PassageSave(), InState = M->Flow->GameState()->TryGetField(TEXT("meteorPassage"));
		if (!Check(L->PassagePhase() == C.Expect, FString::Printf(TEXT("%s -> %s (got %s)"), *C.Name, *C.Expect, *L->PassagePhase()))) return;
		if (C.bRetain)
		{
			if (!Check(RawText(Out) == RawText(C.Value) && RawText(InState) == RawText(C.Value) && HiddenNow(), FString::Printf(TEXT("%s retained verbatim (%s), playback disabled"), *C.Name, *RawText(Out)))) return;
		}
		else if (!Check(P.bV2 && P.Phase == C.Expect && P.bExit == C.bExit && RawText(InState) == RawText(Out), FString::Printf(TEXT("%s saves v2 %s exitReady %d"), *C.Name, *C.Expect, C.bExit))) return;
		if (!Evidence(State, C.Name)) return;
		FreeHold = 0;
		Go(PCaseHold);
		return;
	}
	if (Phase == PCaseHold)
	{
		const FLoadCase& C = Cases[CaseI];
		if (C.bStarts)
		{
			if (L->PassagePhase() == TEXT("pending")) { if (!Hold(Saved().bExit && HiddenNow(), TEXT("the saved exit waits for free outdoor play"))) return; if (In() < 4) return; Check(false, TEXT("a genuine saved exit did not start on free outdoor play")); return; }
			if (!Check(L->PassagePhase() == TEXT("running") && L->PassageElapsed() < .25 && FreeOutdoor() && L->HoleNow() >= .9, FString::Printf(TEXT("%s: the saved real exit starts the passage at %.3f s"), *C.Name, L->PassageElapsed()))) return;
			++CaseI; Go(PCases);
			return;
		}
		if (!Hold(L->PassagePhase() == C.Expect && HiddenNow() && Sky->PassageStarted == 0 && (C.Expect != TEXT("pending") || !Saved().bExit), FString::Printf(TEXT("%s: stays %s and draws nothing"), *C.Name, *C.Expect))) return;
		if (C.Base == TEXT("presignal") && !Hold(Sky->HoleFrom == TNumericLimits<double>::Max(), TEXT("pre-signal load has no hole"))) return;
		if (FreeOutdoor()) FreeHold += Dt;
		if (In() < 1.2) return;
		if (C.Expect == TEXT("pending") && !Check(FreeHold >= .8, FString::Printf(TEXT("%s: %.1f s of free outdoor play after the load manufactured no exit"), *C.Name, FreeHold))) return;
		if (C.bRoundTrip)
		{
			const FString Name = FString::Printf(TEXT("case-%02d.json"), CaseI);
			const S47J::FObj Again = SaveDisk(Name, M->Flow->GameState()) ? LoadDisk(Name) : S47J::FObj();
			if (!Check(Again.IsValid() && RawText(Again->TryGetField(TEXT("meteorPassage"))) == RawText(C.Value), FString::Printf(TEXT("%s survives a second save/disk/reader cycle verbatim"), *C.Name))) return;
		}
		++CaseI; Go(PCases);
		return;
	}
}

TFunction<void(double)> US47WorldLife::MakePassageContract()
{
	TSharedRef<FPassageContract> C = MakeShared<FPassageContract>();
	TWeakObjectPtr<US47WorldLife> Self = this;
	return [Self, C](double Dt)
	{
		US47WorldLife* L = Self.Get();
		if (L) C->Step(L, L->M, Dt);
	};
}

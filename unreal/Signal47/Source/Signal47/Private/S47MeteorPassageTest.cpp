#include "S47WorldLife.h"

#include "S47ArrayShot.h"
#include "S47Flow.h"
#include "S47Json.h"
#include "S47Main.h"
#include "S47Sky.h"
#include "S47Story.h"
#include "S47Walker.h"
#include "S47World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47MeteorPassage, Log, All);

// FX-01's targeted runtime check: -S47MeteorPassageContract with -S47Jump=event, an isolated /tmp
// UserDir and -S47MeteorPassageProof=/tmp/... for the evidence. The real event runs: the passage is
// timed from the story's own EventClock, seen once in the natural window shot, then (test only) the
// shot is stopped and the eye placed in the yard at x=-1 z=-15 looking at 026 / 32. A held story
// clock (what an open panel does) freezes it; it is spent once; a real chapter-one save restored
// through Flow keeps it spent even with the clock put back inside it; an older save without the field
// is marked spent from its clock. Production code never moves the camera for the passage.
namespace
{
	struct FPassageContract
	{
		FString Root;
		int32 Phase = 0, Checks = 0, Shots = 0, MaxHeads = 0, Natural = 0;
		double T = 0, Since = 0, Ev = 0, From = 0, LastShot = -10, HeldClock = 0, HeldPhase = 0;
		int32 HeldHeads = 0, HeldStage = 0;
		bool bDone = false, bBackground = false, bPaused = false, bOutside = false;
		S47J::FObj Ch1State;
	};
}

TFunction<void(double)> US47WorldLife::MakePassageContract()
{
	TSharedRef<FPassageContract> C = MakeShared<FPassageContract>();
	FParse::Value(FCommandLine::Get(), TEXT("S47MeteorPassageProof="), C->Root);
	TWeakObjectPtr<US47WorldLife> Self = this;
	return [Self, C](double Dt)
	{
		US47WorldLife* L = Self.Get();
		FPassageContract& S = *C;
		if (!L || S.bDone) return;
		US47Main* M = L->M;
		if (!M || !M->Story || !M->Flow || !M->World || !M->Player || !L->Sky || M->Mode != US47Main::EMode::Play) return;
		FS47Sky& Sky = *L->Sky;
		S.T += Dt;
		auto Finish = [&S](bool bPass, const FString& Why)
		{
			S.bDone = true;
			const S47J::FObj O = S47J::New();
			S47J::Set(O, TEXT("status"), bPass ? TEXT("PASS") : TEXT("FAIL"));
			S47J::Set(O, TEXT("reason"), Why); S47J::Set(O, TEXT("phase"), S.Phase); S47J::Set(O, TEXT("checks"), S.Checks);
			S47J::Set(O, TEXT("screenshots"), S.Shots); S47J::Set(O, TEXT("maxHeads"), S.MaxHeads);
			if (!S.Root.IsEmpty()) FFileHelper::SaveStringToFile(S47J::Write(O, true), *(S.Root / TEXT("result.json")));
		};
		auto Check = [&S, &Finish](bool bGood, const FString& What)
		{
			++S.Checks;
			UE_LOG(LogS47MeteorPassage, Display, TEXT("S47: meteor passage contract %s: %s"), bGood ? TEXT("PASS") : TEXT("FAIL"), *What);
			if (bGood) return true;
			Finish(false, What);
			FPlatformMisc::RequestExitWithStatus(false, 1);
			return false;
		};
		TWeakObjectPtr<US47WorldLife> Life = L;
		auto Shot = [&S, Life, M](const FString& Name)
		{
			FScreenshotRequest::RequestScreenshot(S.Root / Name, false, false);
			++S.Shots; S.LastShot = S.T;
			UE_LOG(LogS47MeteorPassage, Display, TEXT("S47: meteor passage screenshot %s"), *Name);
			if (const US47WorldLife* Lf = Life.Get(); Lf && Lf->Sky) UE_LOG(LogS47MeteorPassage, Display, TEXT("S47: meteor passage strip: %s eye %s"), *Lf->Sky->PassageDebug(), *M->Player->EyeUE().ToString());
		};
		auto Aim = [M]()
		{
			// the eye in the yard, towards the star hole in world terms (the sky turned by the area's yaw)
			const FVector D0 = S47SkyDir(STAR_HOLE.Az, STAR_HOLE.El);
			const double Yw = M->World->SkyYaw();
			const FVector D(D0.X * FMath::Cos(Yw) + D0.Z * FMath::Sin(Yw), D0.Y, -D0.X * FMath::Sin(Yw) + D0.Z * FMath::Cos(Yw));
			M->Player->Yaw = FMath::Atan2(-D.X, -D.Z); M->Player->Pitch = FMath::Asin(D.Y);
		};
		const bool bLow = Sky.bPassageLow;
		if (S.T > 180) { Check(false, FString::Printf(TEXT("timed out in phase %d"), S.Phase)); return; }
		if (S.Phase == 0)
		{
			FString User;
			FParse::Value(FCommandLine::Get(), TEXT("UserDir="), User);
			if (!Check(S.Root.StartsWith(TEXT("/tmp/")) && User.StartsWith(TEXT("/tmp/")), TEXT("isolated /tmp UserDir and /tmp proof directory"))) return;
			IFileManager::Get().MakeDirectory(*S.Root, true);
			if (!Check(!Sky.bPassageDone && Sky.PassageStarted == 0, TEXT("a new night starts with the passage unspent"))) return;
			if (!Check(Sky.PassageSlotsBuilt() == FS47Sky::PASSAGE_SLOTS, FString::Printf(TEXT("%d passage strips built and precached with the live sky"), Sky.PassageSlotsBuilt()))) return;
			S.Phase = 1;
		}
		if (S.Phase == 1)
		{
			if (!M->Story->At(ES47Phase::Turning)) { if (Sky.PassageHeads || Sky.PassageSlotsVisible()) Check(false, TEXT("no streak before the dishes turn")); return; }
			S.Ev = M->Story->EventClockSec(); S.From = S.Ev + STAR_HOLE.After - FS47Sky::PASSAGE_LEAD;
			const double Clock = M->Story->Clock();
			if (!Check(Sky.HoleFrom == S.Ev + STAR_HOLE.After, FString::Printf(TEXT("HoleFrom %.2f is the real EventClock %.2f + %.0f s"), Sky.HoleFrom, S.Ev, STAR_HOLE.After))) return;
			if (!Check(Clock < S.From && !Sky.PassageActive() && Sky.PassageHeads == 0,
				FString::Printf(TEXT("dishes turn at clock %.2f (event + %.2f s); the passage waits for event + %.0f s"), Clock, Clock - S.Ev, STAR_HOLE.After - FS47Sky::PASSAGE_LEAD))) return;
			S.Phase = 2;
			return;
		}
		if (Sky.PassageActive())
		{
			S.MaxHeads = FMath::Max(S.MaxHeads, Sky.PassageHeads);
			S.bBackground |= Sky.MeteorVisible();
			if (Sky.PassageStarted == 1 && Sky.PassageHeads && M->Story->Clock() < S.From - .01) { Check(false, TEXT("first streak no earlier than event + 4 s")); return; }
		}
		if (S.Phase == 2)
		{
			// The natural view first: the window shot the story takes while the array turns.
			if (Sky.PassageT >= .3 && !S.Natural)
			{
				S.Natural = 1;
				if (!Check(Sky.PassageStarted >= 1 && FMath::IsNearlyEqual(M->Story->Clock() - Sky.PassageT, S.From, .05),
					FString::Printf(TEXT("passage begins at clock %.2f = event + %.2f s"), M->Story->Clock() - Sky.PassageT, M->Story->Clock() - Sky.PassageT - S.Ev))) return;
				Shot(TEXT("00-natural-window-shot.png"));
			}
			if (Sky.PassageT < 1.0) return;
			// Test only: the eye in the yard under the field, the story's window shot set aside.
			if (M->Story->Shot.IsValid() && M->Story->Shot->IsOn()) M->Story->Shot->Stop();
			M->Player->Place(-1, -15, 0); Aim(); S.bOutside = true;
			UE_LOG(LogS47MeteorPassage, Display, TEXT("S47: meteor passage contract: eye in the yard at x=-1 z=-15 towards 026 / 32 (test only)"));
			S.Phase = 3;
			return;
		}
		if (S.bOutside && !Sky.bPassageDone) { if (M->Story->Shot.IsValid() && M->Story->Shot->IsOn()) M->Story->Shot->Stop(); Aim(); }
		if (S.Phase == 3)
		{
			if (Sky.PassageActive() && Sky.PassageLongest() >= .75 && S.T - S.LastShot > .45 && S.Shots < 12 && Sky.PassageT > 1.25)
				Shot(FString::Printf(TEXT("%02d-passage-%04.1fs-%dheads.png"), S.Shots, Sky.PassageT, Sky.PassageHeads));
			if (!S.bPaused && Sky.PassageT >= 4.0 && Sky.PassageLongest() >= .85)
			{
				// What an open panel or the menu's held story does: the clock stops, the passage with it.
				S.bPaused = true; M->Story->SetPaused(true);
				S.HeldClock = M->Story->Clock(); S.HeldPhase = Sky.PassageT; S.HeldHeads = Sky.PassageHeads; S.Since = S.T;
				S.Phase = 4;
				return;
			}
			if (Sky.bPassageDone) { S.Phase = 5; S.Since = -1; }
			else return;
		}
		if (S.Phase == 4)
		{
			// Frozen: check, then an A/B pair of the same held moment (strips drawn, strips hidden).
			const double H = S.T - S.Since;
			if (S.HeldStage == 0 && H >= .3)
			{
				if (!Check(M->Story->Clock() == S.HeldClock && Sky.PassageT == S.HeldPhase && Sky.PassageHeads == S.HeldHeads && Sky.PassageSlotsVisible() == S.HeldHeads,
					FString::Printf(TEXT("held story clock freezes the passage at %.2f s with %d streaks drawn"), S.HeldPhase, S.HeldHeads))) return;
				Shot(TEXT("10-held-A-with-streaks.png")); S.HeldStage = 1;
			}
			else if (S.HeldStage == 1 && H >= .5) { Sky.bPassageHiddenForTest = true; S.HeldStage = 2; }
			else if (S.HeldStage == 2 && H >= .8) { Shot(TEXT("11-held-B-streaks-hidden.png")); S.HeldStage = 3; }
			else if (S.HeldStage == 3 && H >= 1.1)
			{
				Sky.bPassageHiddenForTest = false;
				if (!Check(M->Story->Clock() == S.HeldClock && Sky.PassageT == S.HeldPhase, TEXT("still frozen after 1.1 s held"))) return;
				M->Story->SetPaused(false);
				S.Phase = 3;
			}
			return;
		}
		if (S.Phase == 5)
		{
			if (S.Since < 0)
			{
				S.Since = S.T;
				const int32 Want = FS47Sky::PassageCount(bLow);
				if (!Check(Sky.PassageStarted == Want && S.MaxHeads >= 1 && S.MaxHeads <= FS47Sky::PassageMaxHeads(bLow),
					FString::Printf(TEXT("%s: all %d streaks once, at most %d at once (cap %d)"), bLow ? TEXT("Low") : TEXT("High"), Sky.PassageStarted, S.MaxHeads, FS47Sky::PassageMaxHeads(bLow)))) return;
				if (!Check(!S.bBackground, TEXT("no rare shooting star crosses during the passage"))) return;
				const double Spent = M->Story->Clock() - S.From;
				if (!Check(Spent > FS47Sky::PASSAGE_LEN + FS47Sky::PASSAGE_TAIL && Spent < FS47Sky::PASSAGE_LEN + FS47Sky::PASSAGE_TAIL + 1.5 + .3,
					FString::Printf(TEXT("spent %.2f s of clock after it began (11.2 s, plus the held second)"), Spent))) return;
				const S47J::FObj State = M->Flow->GameState();
				const S47J::FObj P = S47J::Obj(State, TEXT("meteorPassage"));
				if (!Check(P.IsValid() && S47J::Num(P, TEXT("v")) == 1 && P->HasTypedField<EJson::Boolean>(TEXT("done")) && P->GetBoolField(TEXT("done")),
					TEXT("the case's state records it spent (meteorPassage.done)"))) return;
				return;
			}
			if (S.T - S.Since < 1.5) return;
			if (!Check(Sky.PassageSlotsVisible() == 0 && Sky.PassageHeads == 0, TEXT("the sky is quiet again after the passage"))) return;
			Shot(TEXT("20-after.png"));
			S.Phase = 6; S.Since = S.T;
			return;
		}
		if (S.Phase == 6)
		{
			// The next real resume point: chapter one, after the card.
			if (M->Story->ChapterKey != TEXT("ch1") || M->Story->IsCinematic() || !M->Flow->GameSaveBlock().IsEmpty()) return;
			const FString Path = S.Root / TEXT("chapter1-save.json");
			if (!Check(M->Flow->CheckpointName() == TEXT("chapter1") && FFileHelper::SaveStringToFile(S47J::Write(M->Flow->GameState(), true), *Path),
				TEXT("write the real chapter-one case to disk"))) return;
			FString Bytes; FFileHelper::LoadFileToString(Bytes, *Path);
			S.Ch1State = S47J::Read(Bytes);
			if (!Check(S.Ch1State.IsValid(), TEXT("fresh reader parses the saved case"))) return;
			US47Flow::FStart Start; Start.State = S47J::Clone(S.Ch1State); Start.bDev = true; Start.CaseId = M->Flow->CaseId;
			M->Flow->StartGame(Start);
			if (!Check(Sky.bPassageDone && Sky.PassageStarted == 0, TEXT("the restored case keeps it spent"))) return;
			// Put the clock back inside the passage (an older clock, a jump): it still does not play.
			M->Story->SetClock(S.From + 3);
			S.Phase = 7; S.Since = S.T;
			return;
		}
		if (S.Phase == 7)
		{
			if (Sky.PassageHeads || Sky.PassageSlotsVisible() || Sky.PassageActive()) { Check(false, TEXT("no replay after a load with the clock inside the passage")); return; }
			if (S.T - S.Since < 2.5) return;
			if (!Check(Sky.PassageStarted == 0 && Sky.bPassageDone, TEXT("no replay after a load, with the clock put back inside the passage for 2.5 s"))) return;
			// An older save without the field: past the passage by its clock, it is marked spent unseen.
			const S47J::FObj Old = S47J::Clone(S.Ch1State); Old->RemoveField(TEXT("meteorPassage"));
			US47Flow::FStart Start; Start.State = Old; Start.bDev = true; Start.CaseId = M->Flow->CaseId;
			M->Flow->StartGame(Start);
			S.Phase = 8; S.Since = S.T;
			return;
		}
		if (S.Phase == 8)
		{
			if (S.T - S.Since < 1.0) return;
			if (!Check(Sky.bPassageDone && Sky.PassageStarted == 0 && Sky.PassageSlotsVisible() == 0, TEXT("an older chapter-one save without the field is spent by its clock, unseen"))) return;
			Finish(true, TEXT("complete"));
			UE_LOG(LogS47MeteorPassage, Display, TEXT("S47: meteor passage contract PASS complete (%d checks, %d screenshots, %s)"), S.Checks, S.Shots, bLow ? TEXT("Low") : TEXT("High"));
			M->Story->After(1., [] { FPlatformMisc::RequestExit(false); });
		}
	};
}

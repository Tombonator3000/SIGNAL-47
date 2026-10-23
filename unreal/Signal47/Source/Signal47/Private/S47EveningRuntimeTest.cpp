#include "S47Evening.h"

#include "S47Audio.h"

#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47RecordedDialogue.h"
#include "S47Story.h"
#include "S47Timeline.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

// Integration controls: real drive/walk controllers, API dialogue transactions and
// rendered HUD captures. This is deliberately not a claim of human keyboard input.
void FS47Evening::ControlTest(double Dt)
{
	const auto Fail = [this](const FString& Why)
	{
		bRouteFailed = true; bRouteDone = true;
		if (M && M->Player) M->Player->TestMove.Reset();
		UE_LOG(LogTemp, Error, TEXT("S47: evening route %s FAIL stage %d at %.2f %.2f: %s"), *RouteName, RouteStage, M->Player->Pos.X, M->Player->Pos.Z, *Why);
		FPlatformMisc::RequestExit(false);
	};
	if (!bRouteInitialized)
	{
		bRouteInitialized = true;
		if (!FParse::Value(FCommandLine::Get(), TEXT("S47EveningRoute="), RouteName)) return;
		if (RouteName != TEXT("direct") && RouteName != TEXT("burger") && RouteName != TEXT("sandwich") && RouteName != TEXT("water"))
		{
			Fail(TEXT("unknown route; expected direct, burger, sandwich or water")); return;
		}
		FString CaseArg, UserDir;
		const bool bCase = FParse::Value(FCommandLine::Get(), TEXT("S47Case="), CaseArg);
		bool bSafe = M->Flow && M->Flow->bDevRun && !bCase;
		if (bCase && FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir))
		{
			UserDir = FPaths::ConvertRelativePathToFull(UserDir);
			FPaths::CollapseRelativeDirectories(UserDir);
			bSafe = UserDir.StartsWith(TEXT("/tmp/")) || UserDir.StartsWith(TEXT("/var/tmp/"));
		}
		if (!bSafe) { Fail(TEXT("test requires devRun without S47Case, or explicit isolated -UserDir under /tmp/ or /var/tmp/")); return; }
		FParse::Value(FCommandLine::Get(), TEXT("S47EveningShots="), RouteShots);
		if (!RouteShots.IsEmpty()) IFileManager::Get().MakeDirectory(*RouteShots, true);
		bRouteAudio = FParse::Param(FCommandLine::Get(), TEXT("S47EveningAudio"));
		if (bRouteAudio && RouteName != TEXT("burger")) { Fail(TEXT("S47EveningAudio requires S47EveningRoute=burger")); return; }
		RouteStage = 1; RouteHold = 3; RouteLastClock = M->Story->Clock();
		if (bRouteAudio) { Voice->SetLevelDiagnostics(true); RadioVoice->SetLevelDiagnostics(true); }
		if (bRouteAudio) UE_LOG(LogTemp, Display, TEXT("S47: evening audio controls START: complete takes, receiver interruption, context pause and disk restore; levels logged by recorded queue, human listening UNVERIFIED"));
		UE_LOG(LogTemp, Display, TEXT("S47: evening route %s START: physical controllers, API dialogue, isolated disk save/full-world restore; no human-input claim"), *RouteName);
	}
	if (RouteName.IsEmpty() || bRouteDone) return;
	if (!M || !M->World || !M->Player || !M->Story || !M->Flow || !M->UI) { Fail(TEXT("runtime dependency missing")); return; }
	RouteElapsed += Dt; RouteStageElapsed += Dt;
	if (RouteElapsed > (bRouteAudio ? 900 : 600)) { Fail(FString::Printf(TEXT("route timeout, area %s truck %s player %.2f %.2f"), *M->World->Area, *M->World->TruckAt, M->Player->Pos.X, M->Player->Pos.Z)); return; }
	if (const auto* Truck = M->World->CabTruck())
	{
		if (RouteLastLeg == M->World->Leg)
		{
			const double Moved = FVector::Dist(Truck->Position, RouteLastPos);
			if (Moved < 100) RouteDistance += Moved; // exclude coordinate transfer between areas
		}
		RouteLastLeg = M->World->Leg; RouteLastPos = Truck->Position;
	}
	if (Pack.Flag(TEXT("intro_arrived")) && !bRouteArrived)
	{
		bRouteArrived = true; ++RouteArrivalCount;
		if (M->Story->Clock() < S47Timeline::ShiftStart || M->Story->Clock() > S47Timeline::ShiftStart + 1) { Fail(TEXT("observed parking clock outside the first second of 18:00")); return; }
		UE_LOG(LogTemp, Display, TEXT("S47: evening route %s PASS observed actual arrival within first second of 18:00, driven %.1f m"), *RouteName, RouteDistance);
	}
	if (bRouteArrived && M->Story->Clock() + 0.001 < RouteLastClock) { Fail(TEXT("clock moved backwards after arrival")); return; }
	RouteLastClock = M->Story->Clock();
	if (bRouteAudio)
	{
		if (!M->Audio || !M->Audio->Ctx || !M->Audio->bCharacterVoices || !Voice || !RadioVoice) { Fail(TEXT("audio controls require a running audio context and enabled character voices")); return; }
		if (!bRouteAudioRadioInterrupted && Commute() && M->World->bDriving)
		{
			const auto* RadioLine = RadioPack.CurrentLine();
			if (RouteAudioObservedLine.StartsWith(TEXT("radio-off:")))
			{
				const FString Id = RouteAudioObservedLine.Mid(10);
				if (!RadioLine || RadioLine->Id != Id || RadioVoice->Playing() || RadioVoice->Loading()) { Fail(TEXT("receiver-off did not stop audio and retain its line cursor")); return; }
				bRadioOn = true; RouteAudioObservedLine = TEXT("radio-on:") + Id;
			}
			else if (RouteAudioObservedLine.StartsWith(TEXT("radio-on:")))
			{
				const FString Id = RouteAudioObservedLine.Mid(9);
				if (!RadioLine || RadioLine->Id != Id || !RadioVoice->Playing()) { Fail(TEXT("receiver-on did not restart the interrupted actual take")); return; }
				bRouteAudioRadioInterrupted = true; RouteAudioObservedLine.Empty();
				UE_LOG(LogTemp, Display, TEXT("S47: evening audio PASS receiver off/on interruption retains and restarts %s"), *Id);
			}
			else if (RadioLine && RadioVoice->Playing())
			{
				const auto Status = RadioVoice->Status();
				if (Status.Clips.Num() != 1 || Status.Clips[0].Duration <= 0) { Fail(TEXT("host queue did not decode exactly one measured take")); return; }
				RouteAudioPlayedLines.Add(TEXT("radio:") + RadioLine->Id);
				RouteAudioObservedLine = TEXT("radio-off:") + RadioLine->Id; bRadioOn = false;
			}
		}
		if (!bRouteAudioRadioLevels && RadioVoice->Playing())
		{
			const auto Levels = RadioVoice->AudioLevels();
			if (Levels.SourceSamples > 0 && Levels.RoutedSamples > 0 && Levels.SourcePeak > 0 && Levels.RoutedPeak > 0 && Levels.SourceRms > 0 && Levels.RoutedRms > 0 && Levels.BusGain > 0 && Levels.Channel == TEXT("car-radio") && Levels.bReceiverFilter)
			{
				bRouteAudioRadioLevels = true;
				UE_LOG(LogTemp, Display, TEXT("S47: evening audio PASS host rendered samples with positive source/routed RMS, peak and receiver bus gain: %s %.6f/%.6f, gain %.3f"), *Levels.ClipId, Levels.SourceRms, Levels.RoutedRms, Levels.BusGain);
			}
		}
		if (const auto* Line = Pack.CurrentLine(); Line && Line->bRecord && Voice->Playing())
		{
			const auto Status = Voice->Status();
			FS47DialogueInput Input; Input.Voice = Line->VoiceProfile; Input.Lines.Add({Line->Speaker, Line->Text});
			if (!Voice->Available(Input) || Status.Clips.Num() != 1 || Status.Clips[0].Duration <= 0 || Status.Text != TOptional<FString>(Line->Text) || FMath::Abs(Status.Clips[0].Duration - Line->Duration) > 0.002) { Fail(TEXT("foreground queue unavailable, unmeasured or on the wrong line: ") + Line->Id); return; }
			if (RouteAudioObservedLine != Line->Id)
			{
				RouteAudioObservedLine = Line->Id; RouteAudioLineStart = RouteElapsed;
				RouteAudioPlayedLines.Add(Line->Id);
				UE_LOG(LogTemp, Display, TEXT("S47: evening audio observed actual take %s, decoded %.3f s, measured %.3f s, subtitle %.3f..%.3f s, channel %s"), *Line->Id, Status.Clips[0].Duration, Line->Duration, Line->SpeechStart, Line->SpeechEnd, *Pack.CurrentScene()->Channel);
			}
			if (!bRouteAudioPauseChecked && RouteStage == 4)
			{
				bRouteAudioPauseChecked = true; RouteAudioPauseAt = RouteElapsed;
				M->Audio->Suspend(true); RouteAudioPauseClock = M->Audio->Ctx->CurrentTime();
			}
		}
		if (RouteAudioPauseAt >= 0)
		{
			if (M->Audio->Ctx->State() != TEXT("suspended") || M->Audio->Ctx->CurrentTime() != RouteAudioPauseClock || !Voice->Playing() || !Pack.CurrentLine() || Pack.CurrentLine()->Id != RouteAudioObservedLine) { M->Audio->Suspend(false); Fail(TEXT("context pause changed the sound clock, live queue or dialogue cursor")); return; }
			if (RouteElapsed - RouteAudioPauseAt < 0.5) return;
			M->Audio->Suspend(false); RouteAudioPauseAt = -1;
			UE_LOG(LogTemp, Display, TEXT("S47: evening audio PASS 0.5 s context suspend/resume: frozen audio clock, live queue and same cursor; UI pause input not exercised"));
		}
	}
	if (RouteHold > 0) { RouteHold -= Dt; return; }
	const auto Stage = [this](int32 Next, double Hold = 0)
	{
		RouteStage = Next; RouteStageElapsed = 0; RouteHold = Hold;
		UE_LOG(LogTemp, Display, TEXT("S47: evening route %s stage %d, %.1f s, area %s"), *RouteName, Next, RouteElapsed, *M->World->Area);
	};
	const auto Shot = [this](const TCHAR* Name)
	{
		if (RouteShots.IsEmpty()) return;
		const FString Path = FPaths::Combine(RouteShots, RouteName + TEXT("-") + Name + TEXT(".png"));
		FScreenshotRequest::RequestScreenshot(Path, true, false);
		UE_LOG(LogTemp, Display, TEXT("S47: evening route screenshot requested with HUD: %s"), *Path);
	};
	const auto Walk = [this]()
	{
		if (!RouteWalk.IsValidIndex(RouteWalkIndex)) { M->Player->TestMove.Reset(); return true; }
		const FVector Delta = RouteWalk[RouteWalkIndex] - M->Player->Pos;
		const double Dist = FMath::Sqrt(Delta.X * Delta.X + Delta.Z * Delta.Z);
		if (Dist < 0.35) { ++RouteWalkIndex; M->Player->TestMove.Reset(); return false; }
		M->Player->Yaw = FMath::Atan2(-Delta.X, -Delta.Z);
		M->Player->Pitch = -0.06;
		M->Player->TestMove = FVector2D(0, -FMath::Min(1.0, Dist));
		return false;
	};
	const auto DinerWalk = [this](bool bReturn)
	{
		RouteWalk.Reset(); RouteWalkIndex = 0;
		const TArray<FString> Names = bReturn ? TArray<FString>{TEXT("inside"), TEXT("outside"), TEXT("arrive")} : TArray<FString>{TEXT("outside"), TEXT("inside"), TEXT("stool")};
		for (const FString& Name : Names)
		{
			const FS47Pose P = M->Web->Pose(TEXT("world.diner.anchors.") + Name);
			if (P.bValid) RouteWalk.Add(FVector(P.X, 0, P.Z));
		}
	};
	const auto ReadLine = [this, &Fail]()
	{
		if (bRouteAudio)
		{
			const auto* Line = Pack.CurrentLine();
			if (Line && Line->bRecord)
			{
				const auto Status = Voice->Status();
				if (!RouteAudioPlayedLines.Contains(Line->Id) || Status.Clips.Num() != 1 || Status.Clips[0].Duration <= 0 || Status.State == TEXT("error")) { Fail(TEXT("actual recorded line was not observed playing: ") + Line->Id); return; }
				if (Voice->Playing() || Voice->Loading())
				{
					if (RouteElapsed - RouteAudioLineStart > Status.Clips[0].Duration + 10) Fail(TEXT("recorded line did not finish on its measured duration: ") + Line->Id);
					return;
				}
				if (RouteElapsed - RouteAudioLineStart + 0.1 < Status.Clips[0].Duration) { Fail(TEXT("recorded line stopped before its measured duration: ") + Line->Id); return; }
				const auto Levels = Voice->AudioLevels();
				if (Levels.SourceSamples <= 0 || Levels.RoutedSamples <= 0 || Levels.PcmPeak <= 0 || Levels.SourcePeak <= 0 || Levels.SourceRms <= 0 || Levels.RoutedPeak <= 0 || Levels.RoutedRms <= 0 || Levels.BusGain <= 0 || Levels.Channel != Pack.CurrentScene()->Channel || Levels.bReceiverFilter != (Pack.CurrentScene()->Channel != TEXT("physical"))) { Fail(TEXT("recorded take produced no measured source/routed audio: ") + Line->Id); return; }
				UE_LOG(LogTemp, Display, TEXT("S47: evening audio PASS complete take %s, %.3f s decoded, %.3f s observed wall time including pauses/restores"), *Line->Id, Status.Clips[0].Duration, RouteElapsed - RouteAudioLineStart);
			}
		}
		if (Voice) Voice->Stop();
		M->UI->Close(true); bDialogueOpen = false;
		Pack.Advance(); Changed(); ShowDialogue(); RouteHold = 0.45;
	};
	const auto Reply = [this](const FString& Id)
	{
		if (!Pack.Select(Id)) return false;
		M->UI->Close(true); bDialogueOpen = false; Changed(); ShowDialogue(); RouteHold = 0.45;
		return true;
	};
	if (RouteStage == 1)
	{
		if (!Commute() || !M->World->bDriving) { Fail(TEXT("new route did not begin in a physically driven commute")); return; }
		Shot(TEXT("commute")); Stage(2, 2); return;
	}
	if (RouteStage == 2 || RouteStage == 6)
	{
		if (M->World->bDriving || M->World->bBusy || M->Story->IsCinematic()) return;
		if (bRouteArrived) { Stage(9,2); return; }
		if (RouteName == TEXT("direct") || M->World->TruckAt != TEXT("diner") || !M->World->TruckOnLot()) { Fail(TEXT("drive stopped outside the expected diner/SARO parking zone")); return; }
		if (Pack.Flag(TEXT("handover_done")) || M->Story->Begun.Contains(TEXT("ch5"))) { Fail(TEXT("early diner started later story")); return; }
		DinerWalk(false);
		if (RouteWalk.Num() != 3) { Fail(TEXT("diner walking anchors unavailable")); return; }
		Stage(3); return;
	}
	if (RouteStage == 3)
	{
		if (RouteStageElapsed > 60) { Fail(TEXT("physical walk from diner parking to counter blocked")); return; }
		if (!Walk()) return;
		if (!Use(TEXT("diner:waitress")) || !Pack.CurrentScene() || Pack.CurrentScene()->Id != TEXT("I02_DINER_HELLO")) { Fail(TEXT("counter trigger did not open early diner dialogue")); return; }
		Stage(4, 1); return;
	}
	if (RouteStage == 4)
	{
		const auto* Scene = Pack.CurrentScene();
		if (!Scene)
		{
			const bool Meal = RouteName != TEXT("water");
			if (Pack.Flag(TEXT("food_bought")) != Meal || Pack.Flag(TEXT("food_taken")) != (RouteName == TEXT("burger")) || Pack.Flag(TEXT("food_eaten")) != (RouteName == TEXT("sandwich"))) { Fail(TEXT("diner route meal flags disagree")); return; }
			if (Meal && !bRoutePaidRestored) { Fail(TEXT("purchase/pending reply roundtrip was not exercised")); return; }
			UE_LOG(LogTemp, Display, TEXT("S47: evening route %s PASS diner result, one order, no ch5 transition"), *RouteName);
			DinerWalk(true); Stage(5); return;
		}
		const bool PaidResponse = Scene->Id == TEXT("I04_BURGER") || Scene->Id == TEXT("I05_SANDWICH");
		if (PaidResponse && !bRoutePaidRestored)
		{
			const FVector PlayerBefore = M->Player->Pos;
			const FString Pending = Pack.GetPendingChoice(), Response = Scene->Id;
			const int32 DocumentsBefore = Pack.Documents().Num();
			const bool BurgerBefore = Pack.Flag(TEXT("arrival_order_burger"));
			const bool SandwichBefore = Pack.Flag(TEXT("arrival_order_sandwich"));
			if (Pending.IsEmpty() || Pack.NextLine() != 0 || !Pack.Flag(TEXT("food_bought"))) { Fail(TEXT("purchase did not atomically establish its pending response")); return; }
			const FString StoreDir = FPaths::ProjectSavedDir() / TEXT("EveningRuntimeControls") / RouteName;
			FS47SaveStore Writer(StoreDir); Writer.Init();
			FS47SaveCard Card;
			Card.Chapter = TEXT("Evening opening / paid pending response");
			Card.Place = TEXT("Mesa Diner"); Card.Clock = TEXT("Late afternoon"); Card.Playtime = M->Playtime;
			FString Error;
			const auto Meta = Writer.Save(1, TEXT("manual"), 0, M->Flow->GameState(), Card, Error);
			if (!Meta.IsSet()) { Fail(TEXT("isolated disk save failed: ") + Error); return; }
			// A fresh store must rediscover and read the actual transaction from disk.
			FS47SaveStore Reader(StoreDir); Reader.Init();
			const auto Loaded = Reader.Load(Meta->Id);
			const auto SavedPack = S47J::Obj(S47J::Obj(Loaded, TEXT("evening")), TEXT("pack"));
			if (!Loaded || !SavedPack || S47J::Str(SavedPack, TEXT("pending_choice")) != Pending || S47J::Str(SavedPack, TEXT("active_scene")) != Response || S47J::Int(SavedPack, TEXT("next_line"), -1) != 0 || !S47J::Bool(S47J::Obj(SavedPack, TEXT("flags")), TEXT("food_bought"))) { Fail(TEXT("fresh disk store did not recover purchase and pending response together")); return; }
			if (!RouteShots.IsEmpty()) FFileHelper::SaveStringToFile(S47J::Write(Loaded, true), *FPaths::Combine(RouteShots, RouteName + TEXT("-paid-pending.disk-save.json")));
			// Preserve the control harness through Stop/Begin, then use normal full restore.
			// Case zero + dev mode prevents Flow from changing any real active case/store.
			bRoutePaidRestored = true;
			US47Flow::FStart Restore;
			Restore.State = Loaded; Restore.CaseId = 0; Restore.bDev = true; Restore.Playtime = M->Playtime;
			M->Flow->StartGame(Restore);
			if (!bEnabled || !M->Flow->bDevRun || M->Flow->CaseId != 0 || M->World->Area != TEXT("diner") || M->World->bDriving || !M->Player->Pos.Equals(PlayerBefore, 0.015) || !Pack.Flag(TEXT("food_bought")) || Pack.GetPendingChoice() != Pending || !Pack.CurrentScene() || Pack.CurrentScene()->Id != Response || Pack.NextLine() != 0 || Pack.Flag(TEXT("arrival_order_burger")) != BurgerBefore || Pack.Flag(TEXT("arrival_order_sandwich")) != SandwichBefore || Pack.Documents().Num() != DocumentsBefore || Pack.Select(Pending)) { Fail(TEXT("full disk/world restore changed player, order, pending response or dev isolation")); return; }
			UE_LOG(LogTemp, Display, TEXT("S47: evening route %s PASS isolated FS47SaveStore disk transaction + fresh store Load + full Flow.StartGame restore: purchase/pending cursor0 and diner pose retained, no duplicate purchase; store %s"), *RouteName, *StoreDir);
			RouteHold = 0.8; return; // allow StartGame's fade-in before the HUD capture
		}
		if (PaidResponse && !bRouteOrderShot) { ShowDialogue(); Shot(TEXT("diner-order")); bRouteOrderShot = true; RouteHold = 2; return; }
		if (!bRouteOrderShot && RouteName == TEXT("water") && Scene->Id == TEXT("I08_WATER")) { Shot(TEXT("diner-order")); bRouteOrderShot = true; RouteHold = 2; return; }
		if (Pack.CurrentLine()) { ReadLine(); return; }
		FString Id;
		if (Scene->Id == TEXT("I02_DINER_HELLO")) Id = RouteName == TEXT("water") ? TEXT("I02_WATER") : TEXT("I02_MENU");
		else if (Scene->Id == TEXT("I03_MENU")) Id = RouteName == TEXT("sandwich") ? TEXT("I03_SANDWICH") : TEXT("I03_BURGER");
		else if (Scene->Id == TEXT("I04_BURGER")) Id = TEXT("I04_TAKE");
		else if (Scene->Id == TEXT("I05_SANDWICH")) Id = TEXT("I05_EAT");
		else if (Scene->Id == TEXT("I08_WATER")) Id = TEXT("I08_LEAVE");
		if (Id.IsEmpty() || !Reply(Id)) { Fail(TEXT("unexpected diner choice menu: ") + Scene->Id); return; }
		return;
	}
	if (RouteStage == 5)
	{
		if (RouteStageElapsed > 60) { Fail(TEXT("physical return walk to diner car blocked")); return; }
		if (!Walk()) return;
		if (!Use(TEXT("truck")) || (!M->World->bDriving && !M->World->bBusy)) { Fail(TEXT("return car action did not schedule SARO drive")); return; }
		double Auto = 24; FParse::Value(FCommandLine::Get(), TEXT("S47Auto="), Auto); M->World->Autopilot = Auto;
		Stage(6); return;
	}
	if (RouteStage == 9) {Stage(10);return;}
	if (RouteStage == 10)
	{
		if (RouteArrivalCount != 1 || RouteDistance < 100 || !Opening() || Commute() || M->World->bDriving || !Pack.Documents().IsEmpty()) { Fail(TEXT("parking checkpoint state/travel/documents invalid")); return; }
		const double Before = M->Story->Clock(); Parked(TEXT("saro"));
		if (M->Story->Clock() != Before || Pack.CanStart(TEXT("I11_SARO_ARRIVAL"))) { Fail(TEXT("repeated parking reset the clock or arrival")); return; }
		// The service pad has a rail across its north edge. Walk behind the truck,
		// then through the real opening on its east side instead of through the rail.
		Shot(TEXT("parking")); RouteWalk = {FVector(7.4,0,12.0), FVector(10.5,0,12.0), FVector(10.5,0,2.0), FVector(7.5,0,1.6)}; RouteWalkIndex = 0;
		Stage(11, 2); return;
	}
	if (RouteStage == 11)
	{
		if (RouteStageElapsed > 60) { Fail(TEXT("physical SARO walk to service door blocked")); return; }
		if (!Walk()) return;
		if (M->Inter) M->Inter->UseId(TEXT("doorEast"));
		RouteWalk = {FVector(5.2,0,1.6), FVector(4.1,0,1.0)}; RouteWalkIndex = 0; Stage(12); return;
	}
	if (RouteStage == 12)
	{
		if (RouteStageElapsed > 60) { Fail(TEXT("physical walk through service door to phone blocked")); return; }
		if (!Walk()) return;
		const FVector Phone = M->Web->Obj(TEXT("room.objs.phone")).WorldPosition();
		const FVector Delta = Phone - M->Player->Pos;
		M->Player->Yaw = FMath::Atan2(-Delta.X, -Delta.Z);
		if (!bRing || !Use(TEXT("phone")) || !Pack.CurrentScene() || Pack.CurrentScene()->Id != TEXT("D01_HANDOVER") || Pack.CurrentScene()->Channel != TEXT("phone")) { Fail(TEXT("18:00 ringing phone did not begin D01")); return; }
		Shot(TEXT("18-phone")); Stage(17, 2); return;
	}
	if (RouteStage == 17)
	{
		if (bRouteAudio && (!Voice || !Voice->Playing())) { Fail(TEXT("phone disk restore was not initiated while the actual D01 take was playing")); return; }
		if (bRouteAudio) UE_LOG(LogTemp, Display, TEXT("S47: evening audio saving during live D01 take; restored cursor must replay through the measured phone route"));
		const FString SceneBefore=Pack.CurrentScene()?Pack.CurrentScene()->Id:FString();
		const int32 CursorBefore=Pack.NextLine(); const double ClockBefore=M->Story->Clock();
		const FVector PoseBefore=M->Player->Pos;
		const FString StoreDir=FPaths::ProjectSavedDir()/TEXT("EveningRuntimeControls")/RouteName;
		FS47SaveStore Writer(StoreDir); Writer.Init();
		FS47SaveCard Card; Card.Chapter=TEXT("Evening opening / phone in progress"); Card.Place=TEXT("SARO"); Card.Clock=TEXT("18:00");
		FString Error; const auto Meta=Writer.Save(1,TEXT("manual"),1,M->Flow->GameState(),Card,Error);
		if (!Meta) { Fail(TEXT("phone disk save failed: ")+Error); return; }
		FS47SaveStore Reader(StoreDir); Reader.Init(); const auto Saved=Reader.Load(Meta->Id);
		if (!Saved) {Fail(TEXT("phone disk load failed"));return;}
		if (!RouteShots.IsEmpty()) FFileHelper::SaveStringToFile(S47J::Write(Saved,true),*FPaths::Combine(RouteShots,RouteName+TEXT("-phone.disk-save.json")));
		US47Flow::FStart Restore; Restore.State=Saved; Restore.CaseId=0; Restore.bDev=true; Restore.Playtime=M->Playtime;
		M->Flow->StartGame(Restore);
		if (!Pack.Flag(TEXT("intro_arrived")) || Commute() || !Pack.CurrentScene() || Pack.CurrentScene()->Id!=SceneBefore || Pack.NextLine()!=CursorBefore || M->Story->Clock()!=ClockBefore || !M->Player->Pos.Equals(PoseBefore,0.015) || !Pack.Documents().IsEmpty() || Pack.CanStart(TEXT("I11_SARO_ARRIVAL")))
		{Fail(TEXT("phone restore changed arrival, clock, pose, scene, cursor or papers"));return;}
		UE_LOG(LogTemp,Display,TEXT("S47: evening route %s PASS phone disk/full-world restore: arrival, clock, pose and D01 cursor retained"),*RouteName);
		Stage(13,1); return;
	}
	if (RouteStage == 13)
	{
		if (Pack.CurrentLine()) { ReadLine(); return; }
		if (const auto* Scene = Pack.CurrentScene())
		{
			FString Id;
			if (Scene->Id == TEXT("D01_HANDOVER")) Id = Pack.Flag(TEXT("food_bought")) ? TEXT("D01_CH_FOOD") : TEXT("D01_CH_NOFOOD");
			else if (Scene->Id == TEXT("D01_FOOD")) Id = TEXT("D01_CH_FOOD_BRIEF");
			else if (Scene->Id == TEXT("D01_NOFOOD")) Id = TEXT("D01_CH_NOFOOD_BRIEF");
			if (Id.IsEmpty() || !Reply(Id)) { Fail(TEXT("unexpected handover menu")); return; }
			return;
		}
		if (!Pack.Flag(TEXT("handover_done")) || !Pack.Flag(TEXT("intro_completed")) || Opening() || M->Story->bOpeningHeld || RouteArrivalCount != 1) { Fail(TEXT("handover did not release ordinary story once")); return; }
		for (const FString& Id : TArray<FString>{TEXT("EV_HANDOVER"), TEXT("EV_SERVICE_LOG"), TEXT("EV_RADIO_SCHEDULE")})
		{
			int32 Count = 0;
			for (const auto& Doc : M->Story->Docs()) if (Doc.Id == Id) ++Count;
			if (Count != 1) { Fail(TEXT("handover document missing/duplicated: ") + Id); return; }
		}
		const double Clock = M->Story->Clock(); Parked(TEXT("saro"));
		if (Pack.CanStart(TEXT("D01_HANDOVER")) || M->Story->Clock() != Clock) { Fail(TEXT("completed arrival/phone can repeat")); return; }
		UE_LOG(LogTemp, Display, TEXT("S47: evening route %s PASS D01 by phone, three stable papers once, old routine released; clock monotonic"), *RouteName);
		RouteWalk = {FVector(5.2,0,1.6), FVector(7.5,0,1.6), FVector(8.5,0,2.2)}; RouteWalkIndex = 0;
		Stage(14); return;
	}
	if (RouteStage == 14)
	{
		if (RouteStageElapsed > 60) { Fail(TEXT("physical exit for dusk view blocked")); return; }
		if (!Walk()) return;
		M->Player->Yaw = -1.15; M->Player->Pitch = 0.12;
		M->Story->SetClock(FMath::Max(M->Story->Clock(), -5.0 * 3600 - 25 * 60));
		UE_LOG(LogTemp, Display, TEXT("S47: evening route dusk uses a forward-only 18:35 rendering setup after completed opening, not measured natural playtime"));
		Stage(15, 2); return;
	}
	if (RouteStage == 15) { Shot(TEXT("dusk")); Stage(16, 2); return; }
	if (RouteStage == 16)
	{
		if (!RouteShots.IsEmpty())
		{
			TArray<FString> Names = {TEXT("commute"), TEXT("parking"), TEXT("18-phone"), TEXT("dusk")};
			if (RouteName != TEXT("direct")) Names.Add(TEXT("diner-order"));
			for (const FString& Name : Names)
			{
				const FString Path = FPaths::Combine(RouteShots, RouteName + TEXT("-") + Name + TEXT(".png"));
				if (IFileManager::Get().FileSize(*Path) < 1000) { Fail(TEXT("requested screenshot missing/empty: ") + Path); return; }
			}
		}
		if (bRouteAudio)
		{
			if (!bRouteAudioPauseChecked || !bRouteAudioRadioInterrupted || !bRouteAudioRadioLevels || !RouteAudioPlayedLines.Contains(TEXT("I02_DINER_HELLO_001")) || !RouteAudioPlayedLines.Contains(TEXT("D01_HANDOVER_001"))) { Fail(TEXT("audio route missing host interrupt, waitress pause or phone playback evidence")); return; }
			UE_LOG(LogTemp, Display, TEXT("S47: evening audio PASS COMPLETE: %d unique actual takes observed, host receiver interrupt/restart, diner context pause/resume, paid reply and in-progress phone disk/full-world restore; human listening UNVERIFIED"), RouteAudioPlayedLines.Num());
		}
		bRouteDone = true;
		UE_LOG(LogTemp, Display, TEXT("S47: evening route %s PASS COMPLETE: %.1f s, %.1f driven m, physical walk/drive, API dialogue, isolated disk/world save controls for paid routes, 18:00 arrival once, requested UI screenshots verified; human input/audio listening UNVERIFIED"), *RouteName, RouteElapsed, RouteDistance);
		FPlatformMisc::RequestExit(false);
	}
}

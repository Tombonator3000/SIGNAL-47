#include "S47HUD.h"
#include "S47MenuPointerTest.h"
#include "S47TitleEraseSound.h"
#include "S47FreeformInWorldTest.h"
#include "S47FieldRadio.h"
#include "S47FreeformTest.h"
#include "S47FreeformJobs.h"
#include "S47FreeformJourneyTest.h"
#include "S47SaroResumeTest.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47RecoveryCheck.h"
#include "S47TripoCreature.h"
#include "S47RoswellPreview.h"
#include "S47RoswellAIProbe.h"
#include "S47RoswellPoseProbe.h"
#include "S47Horror.h"
#include "S47Observing.h"
#include "S47Spine.h"
#include "S47DevEvents.h"
#include "S47HorrorTest.h"
#include "S47Gamepad.h"
#include "S47Evening.h"
#include "S47Substation.h"
#include "S47Office.h"
#include "S47T02Test.h"
#include "S47Graphics.h"
#include "S47GraphicsTest.h"
#include "S47SkyVisitors.h"
#include "S47DaleDocumentsTest.h"
#include "S47Anomaly.h"
#include "S47AnomalyWorld.h"
#include "S47AnomalyTest.h"
#include "S47Props.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "RHI.h"
#include "RenderingThread.h"
#include "S47ArrayShot.h"
#include "S47Audio.h"
#include "S47NightLife.h"
#include "S47Ops.h"
#include "S47Rx.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47Story.h"
#include "S47BoardRules.h"
#include "S47CaseBoard.h"
#include "S47Chapter4.h"
#include "S47Chapter5.h"
#include "S47Chapter6.h"
#include "S47Inter.h"
#include "S47Chapter2.h"
#include "S47Chapter3.h"
#include "S47Chapters.h"
#include "S47Flow.h"
#include "S47FieldCamera.h"
#include "S47Drive.h"
#include "S47Chapter1.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47NightEvents.h"
#include "S47Radio.h"
#include "S47TripRadio.h"
#include "S47Score.h"
#include "S47Media.h"
#include "S47Phones.h"
#include "S47Station.h"
#include "S47SaroNight.h"
#include "S47Reel047.h"
#include "S47LineLights.h"
#include "S47DishArray.h"
#include "S47ArchiveDishes.h"
#include "S47Away.h"
#include "S47ArchivePhone.h"
#include "S47CB.h"
#include "S47OfficePhone.h"
#include "S47Seen.h"
#include "S47StationTaken.h"
#include "S47DishesFollow.h"
#include "S47Abduction.h"
#include "S47TownLayer.h"
#include "S47SaroDark.h"
#include "S47Curve.h"
#include "S47DecoderDesk.h"
#include "S47ReadAloud.h"
#include "S47GameStyle.h"
#include "SS47Notebook.h"
#include "S47Glimpses.h"
#include "S47MorningHolds.h"
#include "S47TownEvents.h"
#include "S47Roadside.h"
#include "S47NpcDialogue.h"
#include "S47LostNights.h"
#include "S47Caliche.h"
#include "S47Town.h"
#include "S47Saves.h"
#include "S47Keys.h"
#include "S47Theme.h"
#include "S47Startup.h"
#include "Containers/Ticker.h"
#include "Internationalization/Regex.h"
#include "S47UI.h"
#include "S47World.h"
#include "S47WorldLife.h"
#include "S47Walker.h"
#include "SS47Crt.h"
#include "SS47ScreenView.h"
#include "SS47Widgets.h"
#include "Slate/WidgetRenderer.h"
#include "TimerManager.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47HUD, Log, All);

AS47HUD::AS47HUD()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void AS47HUD::BeginPlay()
{
	Super::BeginPlay();
	FTimerHandle T;
	GetWorldTimerManager().SetTimer(T, this, &AS47HUD::Start, 0.1f, false);
}

void AS47HUD::Start()
{
	UGameViewportClient* Viewport = GEngine ? GEngine->GameViewport : nullptr;
	APlayerController* PC = GetOwningPlayerController();
	if (!Viewport || !PC) return;
	S47Gamepad::Init(PC);
	for (TActorIterator<AS47Web> It(GetWorld()); It; ++It) Saro = *It;
	for (TActorIterator<AS47Story> It(GetWorld()); It; ++It) Story = *It;
	Walker = Cast<AS47Walker>(PC->GetPawn());
	Audio = NewObject<US47Audio>(this);
	Audio->Init(GetWorld());
	UE_LOG(LogS47HUD, Display, TEXT("S47: station %s, story %s, walker %s, RHI %s"), Saro.IsValid() ? TEXT("found") : TEXT("MISSING"), Story.IsValid() ? TEXT("found") : TEXT("MISSING"), Walker.IsValid() ? TEXT("found") : TEXT("MISSING"), GDynamicRHI ? GDynamicRHI->GetName() : TEXT("none"));

	if (FParse::Param(FCommandLine::Get(), TEXT("S47SelfTest")))
	{
		TArray<FString> Lines;
		bool bOk = S47Docs::SelfTest(Lines);
		bOk &= FS47OpsTerminal::SelfTest(Lines);
		bOk &= S47RxSelfTest(Lines);
		bOk &= FS47RuleBook::SelfTest(Lines);
		bOk &= US47Chapter2::SelfTest(Lines);
		bOk &= US47Audio::SelfTest(Lines);
		bOk &= S47Saves::SelfTest(Lines) & US47Flow::SelfTest(Lines);
		bOk &= S47Drive::SelfTest(Lines);
		bOk &= US47Chapter1::SelfTest(Lines); // (with the field camera's and the drawings')
		bOk &= US47CaseBoard::SelfTest(Lines);
		bOk &= US47Night::SelfTest(Lines); // (the seen log, the lines, the curve, the night's events, the calls, the glimpses, the flashlight, the film's face, the album)
		bOk &= FS47Radio::SelfTest(Lines) & FS47TripRadio::SelfTest(Lines) & FS47Score::SelfTest(Lines) & FS47MediaSource::SelfTest(Lines) & FS47RadioVoice::SelfTest(Lines);
		bOk &= US47TownLayer::SelfTest(Lines);
		bOk &= US47Phones::SelfTest(Lines); // (the archive's phone and its dishes, the roof, the office's telephone, the field book, the looks, the thread)
		bOk &= US47SaroNight::SelfTest(Lines); // (SARO dark, the array put back, the dishes that follow, the lights on B, the reels, the recording)
		bOk &= US47Chapter3::SelfTest(Lines); // (with stationTaken's)
		bOk &= US47Away::SelfTest(Lines); // (the time away, the trucks, the lost nights, the third one, the star hole, FLASH's frames)
		bOk &= US47Chapter4::SelfTest(Lines); // (with the walkie's and the CB's)
		bOk &= US47Chapter5::SelfTest(Lines); // (with the map overlay's)
		bOk &= US47WorldLife::SelfTest(Lines); // (the glows' blink, the wind, the sky's clock, the flora's cells as the web version draws them)
		bOk &= US47Chapter6::SelfTest(Lines); // (with Ending's real-time sequence)
		bOk &= FS47DecoderDesk::SelfTest(Lines) & FS47ReadAloud::SelfTest(Lines); // (the signal processor's desk and panel; read-aloud's text matching)
		bOk &= FS47GraphicsSettings::SelfTest(Lines);
		bOk &= S47Keys::SelfTest(Lines) & S47Theme::SelfTest(Lines) & S47Gamepad::SelfTest(Lines); // (the keys' binding map; the menus' words: the chapter line, the tape's date)
		bOk &= S47Game::SelfTest(Lines) & SS47Notebook::SelfTest(Lines); // (the HUD's day line, the captions' speakers; the notebook's keys)
		bOk &= FS47ArrayShot::SelfTest(Lines) & FS47NightLife::SelfTest(Lines); // (Unreal only: the camera at the window while the array turns, the quicker turn; the moths' lamps)
		bOk &= FS47Evening::SelfTest(Lines);
		bOk &= FS47Horror::SelfTest(Lines);
		bOk &= FS47Observing::SelfTest(Lines);
		bOk &= FS47FreeformSession::SelfTest(Lines);
		bOk &= FS47FreeformJobs::SelfTest(Lines);
		bOk &= FS47FreeformDirector::SelfTest(Lines);
		bOk &= FS47FreeformWorld::SelfTest(Lines);
		bOk &= FS47Spine::SelfTest(Lines);
		bOk &= FS47SC04::SelfTest(Lines);
		bOk &= FS47SC08::SelfTest(Lines);
		bOk &= FS47SC11::SelfTest(Lines);
		bOk &= FS47SkyVisitors::SelfTest(Lines);
		bOk &= FS47FieldRadio::SelfTest(Lines);
		bOk &= FS47TitleEraseSound::SelfTest(Lines);
		bOk &= FS47EncounterSound::SelfTest(Lines);
		bOk &= FS47Office::SelfTest(Lines);
		bOk &= FS47Substation::SelfTest(Lines);
		bOk &= FS47Anomaly::SelfTest(Lines);
		bOk &= FS47AnomalyWorld::SelfTest(Lines);
		bOk &= AS47Props::SelfTest(Lines); // (Unreal only: the free models' data files, Config/Props)
		bOk &= SS47ScreenView::SelfTest(Lines); // (the station's screens in use: the camera's aim, the glass's pixels; the board's cursor)
		// Several modules prefix their failures ("props: FAIL ...") or put FAIL
		// after the check name. Keep those diagnostics when the overall test fails.
		for (const FString& L : Lines)
			if (L.StartsWith(TEXT("FAIL")) || L.Contains(TEXT(": FAIL")) || L.EndsWith(TEXT(" FAIL")))
				UE_LOG(LogS47HUD, Warning, TEXT("S47: %s"), *L);
		UE_LOG(LogS47HUD, Display, TEXT("S47: self test %s (%d checks)"), bOk ? TEXT("OK") : TEXT("FAILED"), Lines.Num());
	}

	// the deps every ported module is handed (main.ts)
	US47Main* M = US47Main::Get(this);
	M->Web = Saro.Get();
	M->Story = Story.Get();
	M->Hud = this;
	M->Player = Walker.Get();
	M->Audio = Audio;
	M->Inter = NewObject<US47Inter>(M);
	M->Inter->Init(Saro.Get());
	{
		TWeakObjectPtr<AS47Story> St = Story;
		M->Inter->FallbackLabel = [St](const FString& Id) { return St.IsValid() ? St->Label(Id) : FString(); };
		M->Inter->FallbackUse = [St](const FString& Id) { if (St.IsValid()) St->Use(Id); };
	}
	M->UI = NewObject<US47UI>(M);
	M->UI->Init(this);
	M->World = NewObject<US47World>(M);
	M->World->Init(M);
	M->CaseBoard = NewObject<US47CaseBoard>(M); M->CaseBoard->Init(M);
	// Before the chapters: chapter two's and four's ends route through the spine's placeholders.
	if (!M->Spine) M->Spine = MakeShared<FS47Spine>(M);
	if (!M->DevEvents) M->DevEvents = MakeShared<FS47DevEvents>(M);
	if (!M->FieldRadio) M->FieldRadio = MakeShared<FS47FieldRadio>(M);
	if (Story.IsValid()) S47RegisterChapters(Story.Get(), M);
	if (Walker.IsValid() && Saro.IsValid())
	{
		Walker->Web = Saro;
		Walker->Inter = M->Inter;
		TWeakObjectPtr<AS47HUD> Self = this;
		Walker->OnUse = [Self](const FString& Id) { if (Self.IsValid()) Self->Use(Id); };
		Walker->SetActive(true);
		UE_LOG(LogS47HUD, Display, TEXT("S47: web world %s"), *Saro->Summary());
	}
	// (ui.showHud: the HUD is shown in play, not on the title)
	auto Playing = [this] { const US47Main* Mn = US47Main::Get(this); return !Mn || Mn->Mode == US47Main::EMode::Play; };
	Hud = SNew(SS47Hud).Visibility(EVisibility::HitTestInvisible)
		.Prompt_Lambda([this] { return FText::FromString(PromptLabel); })
		.PromptAlt_Lambda([this] { return FText::FromString(PromptAlt); })
		// (in the cab only what the drive offers: the walker's last look is not a prompt there)
		.ShowPrompt_Lambda([this, Playing, M] { return !Modal.IsValid() && Playing() && (bPromptOwn || !(M->World && M->World->bDriving)); })
		.Clock_Lambda([this, Playing, M]
		{
			if (Story.IsValid())
				if (const auto* C = Cast<US47Chapter6>(Story->Chapter(TEXT("ch6"))); C && C->bActive
					&& (C->Stage == TEXT("pullout") || C->Stage == TEXT("end"))) return FText::GetEmpty();
			// The watch leaves the picture with the driver; captions and pause controls retain their own widgets.
			return FText::FromString(Story.IsValid() && Playing() && !Modal.IsValid() && !(M->Evening && M->Evening->Commute()) ? Story->ClockText(false) : FString());
		})
		.ShowAim_Lambda([this, Playing, M] { return !Modal.IsValid() && Playing() && !(M->World && M->World->bDriving); });
	Viewport->AddViewportWidgetContent(Hud.ToSharedRef(), 20);
	PC->bShowMouseCursor = false;
	FInputModeGameOnly Mode;
	Mode.SetConsumeCaptureMouseDown(false);
	PC->SetInputMode(Mode);

	if (Story.IsValid())
	{
		Story->Bind(Saro.Get(), this, Audio);
		if (Saro.IsValid())
		{
			TWeakObjectPtr<US47Audio> A = Audio;
			TWeakObjectPtr<AS47HUD> Self = this;
			Saro->OnMugShatter = [A, Self](const FVector& P) { if (A.IsValid()) A->Play(TEXT("ceramic"), 1.f, P + FVector(0, 0, 20)); };
		}
		Audio->StartRoomTone();
		Audio->StartNight(); // main.ts: the night outside (core/ambience.ts)
		if (Saro.IsValid())
		{
			TWeakObjectPtr<US47Audio> A = Audio;
			TWeakObjectPtr<AS47Story> St = Story;
			// Doors.toggle: the latch, a creak, then the leaf meets the frame; the story hears it
			Saro->OnDoor = [A, St](const FString& Id, bool bOpen, const FVector& At, bool bHeavy)
			{
				if (St.IsValid()) St->DoorChanged(Id, bOpen);
				if (!A.IsValid()) return;
				A->Play(TEXT("door_latch"), bHeavy ? 0.55f : 0.45f, At, bOpen ? 1.f : 0.85f);
				A->Play(FMath::FRand() < 0.5f ? TEXT("door_creak0") : TEXT("door_creak1"), bHeavy ? 0.16f : 0.12f, At, bHeavy ? 0.8f : 1.f);
				if (!bOpen) A->Play(bHeavy ? TEXT("door_heavy") : TEXT("door_light"), bHeavy ? 0.32f : 0.4f, At);
			};
		}
		// main.ts boot(): the title, or with a test flag straight into play (-S47Jump= or the night's start)
		M->Flow = NewObject<US47Flow>(M);
		M->Flow->Init(M);
		// the case board's layout goes with the case (Prologue.boardSave), and its threads save
		if (US47CaseBoard* CB = M->CaseBoard)
		{
			TWeakObjectPtr<US47CaseBoard> Wb = CB;
			M->Flow->SaveBoard = [Wb] { return Wb.IsValid() ? Wb->Save() : S47J::FObj(); };
			M->Flow->LoadBoard = [Wb](const S47J::FObj& O) { if (Wb.IsValid()) Wb->Load(O); };
			CB->OnSave = [M] { M->RequestAutosave(false); };
		}
		M->Flow->HaveCamera = [M] { return M->FieldCamera && M->FieldCamera->bHave; };
		// main.ts: the night's layer (the seen log, the curve, the night's events, the calls, the glimpses, the flashlight, the album)
		M->Night = NewObject<US47Night>(M); M->Night->Init(M);
		{
			// main.ts nightEvents' radio hooks: the truck's radio and the bulletin on 1420
			FS47NightHooks& H = M->Night->Hooks;
			H.CarRadioOn = [M] { return M->CarRadio.IsValid() && M->CarRadio->bOn; };
			H.Radio1420 = [M] { if (!M->CarRadio.IsValid() || !M->Radio.IsValid() || !M->CarRadio->Live() || M->CarRadio->Station != TEXT("caliche")) return false; const TSharedPtr<FS47Station>* St = M->Radio->Stations.Find(TEXT("caliche")); return St && St->IsValid() && (*St)->Free(); };
			H.CarRadioDrift = [M](double Secs) { return M->CarRadio.IsValid() && M->CarRadio->Live() && M->CarRadio->Drift(Secs); };
			H.SetCarRadio = [M](const FString& St) { if (M->CarRadio.IsValid()) M->CarRadio->Set(true, St); };
			H.Bulletin = [M]() -> double
			{
				const TSharedPtr<FS47Station>* St = M->Radio.IsValid() ? M->Radio->Stations.Find(TEXT("caliche")) : nullptr;
				if (!St || !St->IsValid()) return 0;
				TArray<FS47RadioLine> Lines;
				for (const FS47BulletinLine& L : S47Bulletin()) { FS47RadioLine R; R.Text = L.Text; R.Who = L.Who; R.By = L.By; Lines.Add(R); }
				return (*St)->CutIn(Lines, M->Story ? M->Story->Clock() : 0);
			};
			H.BulletinProgress = [M]() -> TOptional<FS47ArchiveProgress>
			{
				const TSharedPtr<FS47Station>* St = M->Radio.IsValid() ? M->Radio->Stations.Find(TEXT("caliche")) : nullptr;
				return St && St->IsValid() ? (*St)->ArchiveStatus() : TOptional<FS47ArchiveProgress>();
			};
			H.BulletinHeard = [M]
			{
				return M->World && M->World->bDriving && M->CarRadio.IsValid() && M->CarRadio->Live() && M->CarRadio->Station == TEXT("caliche") && M->CarRadio->Power > 0.5
					&& FMath::Fmod(M->Story ? M->Story->Clock() : 0.0, 86400.0) < S47Radio::SIGN_OFF;
			};
		}
		// main.ts: the phones, the archive and the records (the archive's phone, the office's telephone, the roof, the field book, the looks)
		M->Phones = NewObject<US47Phones>(M); M->Phones->Init(M);
		if (US47Chapter3* C3 = Story.IsValid() ? Cast<US47Chapter3>(Story->Chapter(TEXT("ch3"))) : nullptr)
		{
			// main.ts: the field book keeps its state in chapter three's save (ch3.s.fb), knows frames 3
			// and 4, and the station's drawer is where it is found
			TWeakObjectPtr<US47Chapter3> W3 = C3;
			M->Phones->FbGet = [W3] { return W3.IsValid() ? W3->S.Fb : TSharedPtr<FJsonValue>(); };
			M->Phones->FbSet = [W3](const TSharedPtr<FJsonValue>& V) { if (W3.IsValid()) W3->S.Fb = V; };
			M->Phones->Frames = [W3]() -> TPair<TOptional<double>, TOptional<double>>
			{
				if (!W3.IsValid()) return {};
				return { W3->S.F3 ? TOptional<double>(W3->S.F3->Clock) : TOptional<double>(), W3->S.F4 ? TOptional<double>(W3->S.F4->Clock) : TOptional<double>() };
			};
			M->Phones->StationDrawer = [W3](bool bOpen, bool bNow) { if (W3.IsValid() && W3->Site()) W3->Site()->SetDrawer(bOpen, bNow); };
		}
		// main.ts: SARO's own happenings (SARO dark, the array, the dishes that follow, the lights on B, the reels, the recording)
		M->SaroNight = NewObject<US47SaroNight>(M); M->SaroNight->Init(M);
		if (US47Phones* Ph = M->Phones; Ph && M->SaroNight)
		{
			// main.ts: what SARO's own happenings and the archive's phone know of each other
			US47SaroNight* Sn = M->SaroNight;
			TWeakObjectPtr<US47SaroNight> Ws = Sn;
			TWeakObjectPtr<US47Phones> Wp = Ph;
			Sn->Zoom = [M] { return M->ZoomK; };
			Sn->TipMoving = [Wp] { return Wp.IsValid() && Wp->ArrayTip.IsValid() && Wp->ArrayTip->Moving(); };
			Sn->TipHolds = [Wp] { return Wp.IsValid() && Wp->ArrayTip.IsValid() && Wp->ArrayTip->Holds(); };
			Ph->Dark = [M] { return M->Story && M->Story->bDark; };                       // game.dark
			// cb.busy (core/cb.ts: the CB under the truck's dash, made with chapter four)
			Ph->CbBusy = [M] { return M->CB.IsValid() && M->CB->bBusy; };
			Sn->CbBusy = [M] { return M->CB.IsValid() && M->CB->bBusy; };
			Ph->DishRogue = [Ws](int32 I) { return Ws.IsValid() && Ws->Dishes.IsValid() && Ws->Dishes->Dishes.IsValidIndex(I) && Ws->Dishes->Dishes[I].bRogue; };
			Ph->DishBack = [Ws](int32 I) { return Ws.IsValid() && Ws->Dishes.IsValid() && Ws->Dishes->Dishes.IsValidIndex(I) && Ws->Dishes->Dishes[I].bBack; };
			// (the array tipped by the archive's phone blinks its red lamps as one: ext.dishArray.beaconPhase)
			Ph->DishArray = [Ws] { return Ws.IsValid() ? Ws->Dishes.Get() : nullptr; };
			Ph->Foretell = [Ws] { if (Ws.IsValid() && Ws->LineLights.IsValid()) Ws->LineLights->bAnyWay = true; }; // (pasture.now first: US47TownLayer::Init wraps it)
			// reel 047 on the processor's deck (story/reel047.ts as the archive's deck)
			Ph->Reel047.Playing = [Ws] { return Ws.IsValid() && Ws->Reel047.IsValid() && Ws->Reel047->bPlaying; };
			Ph->Reel047.Finished = [Ws] { return Ws.IsValid() && Ws->Reel047.IsValid() && Ws->Reel047->bFinished; };
			Ph->Reel047.Update = [Ws](double Dt) { if (Ws.IsValid() && Ws->Reel047.IsValid()) Ws->Reel047->Update(Dt); };
			Ph->Reel047.Play = [Ws](TFunction<void(const FString&)> Cue) { if (Ws.IsValid() && Ws->Reel047.IsValid()) Ws->Reel047->Play([Cue](const FS47ReelCue& C) { if (Cue) Cue(C.Kind); }); };
			Ph->Reel047.Reset = [Ws] { if (Ws.IsValid() && Ws->Reel047.IsValid()) Ws->Reel047->Reset(); };
		}
		// main.ts: the time away and the trucks (the abduction, taken in the truck, the truck on B, the lost nights, the third one, the star hole)
		M->Away = NewObject<US47Away>(M); M->Away->Init(M);
		{
			// main.ts: the time away and what hangs on it (SARO's dishes that follow, the station taken in
			// chapter three, the third one on film, the archive's count)
			US47Away* Aw = M->Away;
			TWeakObjectPtr<US47Away> Wa = Aw;
			TWeakObjectPtr<US47Chapter6> W6 = Cast<US47Chapter6>(Story->Chapter(TEXT("ch6")));
			Aw->Ch6TurnKey = [W6] { return W6.IsValid() && W6->TurnKey(); };
			Aw->Ch6TryDoor = [W6] { return W6.IsValid() && W6->TryDoor(); };
			Aw->Ch6DoorOffered = [W6] { return W6.IsValid() && W6->DoorOffered(); };
			Aw->Ch6CbDue = [W6] { return W6.IsValid() && W6->CbDue(); };
			Aw->Ch6ThirdAt = [W6] { return W6.IsValid() ? W6->ThirdAt() : TOptional<FVector>(); };
			Aw->Ch6OldMi = [W6, M]() -> TOptional<double>
			{
				// game.ch6.active && game.ch6.stage !== 'end' && tr && world.oldRoad (truckBIn; the area is US47Away's)
				if (!W6.IsValid() || !W6->bActive || W6->Stage == TEXT("end") || !M->World || !M->World->bDriving || !M->World->OldRoad() || !M->World->OldDrive()) return {};
				const FVector P = M->World->OldDrive()->Pos;
				return M->World->OldRoad()->Where(P.X, P.Z).Mi;
			};
			if (M->World) M->World->CabWantsKey = [W6] { return W6.IsValid() && W6->WantsKey(); };
			if (US47SaroNight* Sn = M->SaroNight)
			{
				FS47AbductView& V = Sn->Abduct;
				V.Stage = [Wa] { return Wa.IsValid() && Wa->Abduction.IsValid() ? Wa->Abduction->Stage : FString(TEXT("calm")); };
				V.Warn = [Wa]() -> TOptional<FS47FollowIn::FWarn>
				{
					if (!Wa.IsValid() || !Wa->Abduction.IsValid()) return {};
					const auto W = Wa->Abduction->Warn();
					if (!W) return {};
					FS47FollowIn::FWarn F; F.T = W->T; F.Len = W->Len;
					return F;
				};
				V.BackAt = [Wa] { return Wa.IsValid() && Wa->Abduction.IsValid() ? Wa->Abduction->BackAt : TOptional<FVector>(); };
				V.Last = [Wa] { return Wa.IsValid() && Wa->Abduction.IsValid() ? Wa->Abduction->Data.Last : -1e9; };
				V.Over = [Wa] { return Wa.IsValid() && Wa->Abduction.IsValid() && !Wa->Abduction->OverPhase().IsEmpty(); };
				V.GetDishes = [Wa] { return Wa.IsValid() && Wa->Abduction.IsValid() ? FS47DishesSave::Copy(Wa->Abduction->Data.Dishes) : TOptional<FS47DishesSave>(); };
				V.SetDishes = [Wa](const FS47DishesSave& D) { if (Wa.IsValid() && Wa->Abduction.IsValid()) Wa->Abduction->Data.Dishes = D.ToJson(); };
				V.ClearDishes = [Wa] { if (Wa.IsValid() && Wa->Abduction.IsValid()) Wa->Abduction->Data.Dishes.Reset(); };
				// the corner clock in seconds: abduction.warnClock ?? saroDark.clockShow ?? archive.clockShow ?? ch6.clockShow
				Sn->ClockBefore = [Wa]
				{
					if (!Wa.IsValid() || !Wa->Abduction.IsValid()) return FString();
					const auto Wc = Wa->Abduction->WarnClock();
					return Wc ? Wc->Hm + TEXT(":") + Wc->Ss : FString();
				};
				TWeakObjectPtr<US47Phones> Wp = M->Phones;
				Sn->ClockAfter = [Wp, W6]
				{
					if (Wp.IsValid() && Wp->Archive.IsValid() && Wp->Archive->ClockShow) return Wp->Archive->ClockShow->Hm + TEXT(":") + Wp->Archive->ClockShow->Ss;
					return W6.IsValid() ? W6->ClockShow() : FString();
				};
				// the recording's walk (story/recording.ts): recs: () => abduction.recordings(),
				// hold: (n, h) => abduction.holdRecord(n, h); the figures are the time away's own
				V.Recordings = [Wa] { return Wa.IsValid() && Wa->Abduction.IsValid() ? Wa->Abduction->Recordings() : TArray<TSharedPtr<FS47Rec>>(); };
				V.HoldRecord = [Wa](int32 N, bool bHeld) { if (Wa.IsValid() && Wa->Abduction.IsValid()) Wa->Abduction->HoldRecord(N, bHeld); };
				// (recWalk.reset() when Missing time is turned off while it walks)
				TWeakObjectPtr<US47SaroNight> Ws = Sn;
				Aw->RecWalkReset = [Ws] { if (Ws.IsValid() && Ws->RecWalk.IsValid()) Ws->RecWalk->Reset(); };
				// arrayBack.update before ab.update (the time away may not begin while the array is on the schedule)
				Aw->BeforeAbduction = [Ws](double Dt, bool bModal) { if (Ws.IsValid()) Ws->StepArray(Dt, bModal); };
				// the lost nights play Earl's reel on SARO's deck (main.ts new LostNights({ tape: reelTape }): core/reelTape.ts)
				Aw->ReelTape.Busy = [Ws] { return Ws.IsValid() && Ws->ReelTape.IsValid() && Ws->ReelTape->bBusy; };
				Aw->ReelTape.Play = [Ws](TFunction<void(const FS47LostTape::FCue&)> Cue, TFunction<void(bool)> Done)
				{
					if (!Ws.IsValid() || !Ws->ReelTape.IsValid()) { if (Done) Done(false); return; }
					Ws->ReelTape->Play([Cue](const FS47TapeCue& C)
					{
						if (!Cue) return;
						FS47LostTape::FCue X; X.Kind = C.Kind; X.Text = C.Text; X.I = C.I;
						Cue(X);
					}, MoveTemp(Done));
				};
				Aw->ReelTape.Stop = [Ws] { if (Ws.IsValid() && Ws->ReelTape.IsValid()) Ws->ReelTape->Stop(); };
			}
			if (M->Phones) M->Phones->AbductionCount = [Wa] { return Wa.IsValid() && Wa->Abduction.IsValid() ? Wa->Abduction->Data.Count : 0; };
			if (US47Chapter3* C3 = Story.IsValid() ? Cast<US47Chapter3>(Story->Chapter(TEXT("ch3"))) : nullptr)
			{
				TWeakObjectPtr<US47Chapter3> W3 = C3;
				Aw->StationIdle = [W3] { return !W3.IsValid() || !W3->StationTaken || W3->StationTaken->State == TEXT("idle"); };
				Aw->StationBegin = [W3](double Clock) { if (W3.IsValid() && W3->StationTaken) W3->StationTaken->Begin(Clock); };
				// main.ts, after ab.update: stationTaken.update, and the station passed after all (the recording's
				// walk and the dishes that follow are stepped by S47SaroNight, later in the frame)
				Aw->AfterAbduction = [W3, Wa, M](double Dt, bool)
				{
					if (!W3.IsValid() || !W3->StationTaken || !Wa.IsValid() || !Wa->Abduction.IsValid()) return;
					FS47Abduction& Ab = *Wa->Abduction;
					US47StationTaken::FIn In;
					In.Stage = Ab.Stage;
					if (const auto Wn = Ab.Warn()) In.WarnT = Wn->T;
					In.Clock = M->Story ? M->Story->Clock() : 0;
					W3->StationTaken->Update(Dt, In);
					if (W3->StationTaken->Last == TEXT("passed") && Ab.Data.Station == TEXT("taken")) Ab.Data.Station = TEXT("passed");
				};
				C3->Third = [Wa](US47FieldCamera* Cam) -> TOptional<FS47FilmShot>
				{
					if (!Wa.IsValid() || !Cam) return {};
					// fcam.photoCam in three's terms
					FS47Cam C;
					C.Pos = S47Three::ToThree(Cam->PhotoLoc);
					auto Dir = [](const FVector& U) { return FVector(U.X, U.Z, U.Y); };
					const FRotationMatrix R(Cam->PhotoRot);
					C.Forward = Dir(R.GetUnitAxis(EAxis::X)); C.Right = Dir(R.GetUnitAxis(EAxis::Y)); C.Up = Dir(R.GetUnitAxis(EAxis::Z));
					C.Fov = US47FieldCamera::PHOTO_FOV; C.Aspect = 960.0 / 600.0;
					return Wa->ThirdShot(C);
				};
				C3->OnBand = [Wa] { if (Wa.IsValid()) Wa->ThirdNoticed(); };
			}
			if (US47Chapter4* C4 = Story.IsValid() ? Cast<US47Chapter4>(Story->Chapter(TEXT("ch4"))) : nullptr)
			{
				// main.ts: ch4.taken (N. Vega sees the mark), the office's telephone in room 6, and the CB
				// held back while the time away runs (motel.cb)
				C4->Taken = [Wa] { return Wa.IsValid() && Wa->Taken(); };
				if (US47Phones* Ph = M->Phones; Ph && Ph->OfficePhone.IsValid())
				{
					TWeakObjectPtr<US47Phones> Wp = Ph;
					C4->OfficePhone = US47Chapter4::FOfficePhone{
						[Wp] { return Wp.IsValid() && Wp->OfficePhone.IsValid() ? Wp->OfficePhone->Label() : FString(); },
						[Wp] { return Wp.IsValid() && Wp->OfficePhone.IsValid() && Wp->OfficePhone->Use(); } };
				}
				TFunction<bool(const TArray<FS47CBLine>&, TFunction<void()>)> Cb = C4->Cb;
				C4->Cb = [Wa, Cb](const TArray<FS47CBLine>& L, TFunction<void()> OnEnd) { return !(Wa.IsValid() && Wa->CbBlocked()) && Cb && Cb(L, OnEnd); };
			}
		}
		M->TownLayer = NewObject<US47TownLayer>(M); M->TownLayer->Init(M);
		if (US47TownLayer* Tl = M->TownLayer)
		{
			// main.ts: what the town and the time away know of each other (Earl's reel is the lost nights';
			// the woman and the smoker are among people; the other voices hush for the reel)
			TWeakObjectPtr<US47TownLayer> Wt = Tl;
			if (US47Away* Aw = M->Away)
			{
				TWeakObjectPtr<US47Away> Wa = Aw;
				Aw->TownWomanIn = [Wt] { return Wt.IsValid() && Wt->Town.IsValid() && Wt->Town->bWomanIn; };
				Aw->TownSmokerIn = [Wt] { return Wt.IsValid() && Wt->Town.IsValid() && Wt->Town->bSmokerIn; };
				Aw->HushVoices = [Wt, M] { if (Wt.IsValid() && Wt->Caliche.IsValid()) Wt->Caliche->StopSpeech(); if (M->UI) M->UI->StopSpeech(); if (M->CB) M->CB->Stop(); };
				if (Tl->Caliche.IsValid())
				{
					FS47Caliche::FReel& R = Tl->Caliche->Reel;
					R.Ready = [Wa] { return Wa.IsValid() && Wa->LostNights.IsValid() && Wa->LostNights->ReelReady(); };
					R.Play = [Wa](TFunction<void(const TArray<FS47TalkLine>&)> Lines, TFunction<void(bool)> Done)
					{
						if (!Wa.IsValid() || !Wa->LostNights.IsValid()) { if (Done) Done(false); return; }
						Wa->LostNights->PlayReel([Lines](const TArray<FS47LostLine>& L)
						{
							TArray<FS47TalkLine> Out;
							for (const FS47LostLine& X : L) Out.Add(FS47TalkLine{ X.Key, X.Value });
							if (Lines) Lines(Out);
						}, Done);
					};
					R.Stop = [Wa] { if (Wa.IsValid() && Wa->LostNights.IsValid()) Wa->LostNights->StopReel(); };
				}
			}
			if (US47Chapter3* C3 = Story.IsValid() ? Cast<US47Chapter3>(Story->Chapter(TEXT("ch3"))) : nullptr)
				C3->FreeShot = [Wt] { return Wt.IsValid() && Wt->FreeShot(); };   // ninth.freeShot
		}
		// main.ts: the world's own life (the sky, the glows, the ground cover, the air, the night's life, the windows, Ultra's lamps)
		M->Life = NewObject<US47WorldLife>(M); M->Life->Init(M);
		{
			// main.ts: world.lifeQuiet, saroDark.adapt in the room's hemisphere light, morning.busy
			US47WorldLife::FHooks& H = M->Life->Hooks;
			H.LifeQuiet = [M]
			{
				if (M->Mode != US47Main::EMode::Play || (M->Story && M->Story->IsCinematic())) return true;
				if (M->TownLayer && M->TownLayer->Roadside.IsValid() && M->TownLayer->Roadside->bSilent) return true;
				if (M->Night && M->Night->Glimpses.IsValid() && !M->Night->Glimpses->Up.IsEmpty()) return true;
				if (M->TownLayer && M->TownLayer->TownEvents.IsValid() && M->TownLayer->TownEvents->Now != TEXT("none")) return true;
				return M->Night && M->Night->Curve.IsValid() && M->Night->Curve->Phase == TEXT("peak") && !M->Night->Curve->bFree;
			};
			H.SaroDarkAdapt = [M] { return M->SaroNight && M->SaroNight->Dark.IsValid() ? M->SaroNight->Dark->Adapt : 1.0; };
			H.MorningBusy = [M] { return M->TownLayer && M->TownLayer->Morning.IsValid() && M->TownLayer->Morning->Busy(); };
			// game.ch6.skyLift, game.ch6.sunElev (chapter six puts them on the world: AS47Web's overrides)
			H.SkyLift = [M] { return M->Web ? M->Web->DawnLift() : TOptional<double>(); };
			H.SunElev = [M] { return M->Web ? M->Web->SunElevation() : TOptional<double>(); };
		}
		// main.ts musicHeld: the music held while the recording walks, the face's quiet, the steps on the
		// roof, Earl's reel, SARO dark, the office's telephone, the archive's phone and the array put back
		// (the roadside, the television, the ninth, the morning, the time away and the truck on B are set
		// by their layers)
		if (M->Score.IsValid() && M->Night)
		{
			FS47Score::FHeldHooks& Hd = M->Score->Held;
			const FS47NightHooks& H = M->Night->Hooks;
			Hd.RecWalkBusy = H.RecWalkBusy;
			Hd.FaceQuiet = [M] { return M->Night ? M->Night->FaceQuiet : 0.0; };
			Hd.RoofStepsBusy = H.RoofStepsBusy; Hd.ReelTapeBusy = H.ReelTapeBusy; Hd.SaroDarkBusy = H.SaroDarkBusy;
			Hd.OfficePhoneBusy = H.OfficePhoneBusy; Hd.ArchiveBusy = H.ArchiveBusy; Hd.ArrayBackBusy = H.ArrayBackBusy;
		}
		M->Evening = MakeShared<FS47Evening>(M);
		M->SkyVisitors = MakeShared<FS47SkyVisitors>(M);
		M->Office = MakeShared<FS47Office>(M);
		M->Office->Init();
		if (auto* C2 = Cast<US47Chapter2>(M->Story->Chapter(TEXT("ch2"))))
		{
			const auto PreviousEnd = C2->OnEnd; // includes the existing roof-step/phone quiet wrapper
			TWeakObjectPtr<US47Chapter2> Weak = C2;
			M->Office->CanDepart = [M, Weak]
			{
				return Weak.IsValid() && M->Story->ChapterKey == TEXT("ch2") && Weak->S.Stage == TEXT("complete") &&
				 Weak->S.bCalled && Weak->S.bP04 && Weak->S.bP05 && M->Office->HasFieldSheet() &&
				 !M->Story->IsCinematic() && !M->ModalOpen() && !M->World->bBusy;
			};
			M->Office->OnDepart = [PreviousEnd] { if (PreviousEnd) PreviousEnd(); };
			C2->OnEnd = [M, PreviousEnd]
			{
				if (M->Office && M->Office->RouteRequired() && !M->Office->DepartureReleased())
				{
					M->UI->Toast(M->Office->HasFieldSheet() ? TEXT("Field sheet retained. Use the Operations departure card when you are ready for STATION 01.")
					 : TEXT("Before leaving, collect the Field checks copy from Operations. The service drawer holds the spare key."), 5);
					M->RequestAutosave(); return;
				}
				if (PreviousEnd) PreviousEnd();
			};
		}
		M->Anomaly = MakeShared<FS47Anomaly>(M);
		M->AnomalyWorld = MakeShared<FS47AnomalyWorld>(M);
		M->AnomalyTest = MakeShared<FS47AnomalyTest>(M);
		M->Substation = MakeShared<FS47Substation>(M);
		M->Substation->Begin();
		M->T02Test = MakeShared<FS47T02Test>(M);
		M->GraphicsTest = MakeShared<FS47GraphicsTest>(M);
		M->DaleDocumentsTest = MakeShared<FS47DaleDocumentsTest>(M);
		M->Freeform = MakeShared<FS47FreeformSession>(M);
		M->Observing = MakeShared<FS47Observing>(M);
		M->Horror = MakeShared<FS47Horror>(M);
		M->HorrorTest = MakeShared<FS47HorrorTest>(M);
		M->FreeformInWorldTest = MakeShared<FS47FreeformInWorldTest>(M);
		M->FreeformTest = MakeShared<FS47FreeformTest>(M);
		M->FreeformJourneyTest = MakeShared<FS47FreeformJourneyTest>(M);
		M->SaroResumeTest = MakeShared<FS47SaroResumeTest>(M);
		M->Flow->Boot();
		FString FaceAt;
		if (FParse::Value(FCommandLine::Get(), TEXT("S47Face="), FaceAt)) Face(FaceAt);
	}
// Slate writes linear colour; the sRGB CRT target encodes it once.
	Renderer = new FWidgetRenderer(false);
	const SS47Crt::EKind Kinds[4] = { SS47Crt::EKind::Status, SS47Crt::EKind::Spectrum, SS47Crt::EKind::Tracking, SS47Crt::EKind::Ops };
	for (int32 i = 0; i < 4; ++i) Crts[i] = SNew(SS47Crt).Kind(Kinds[i]).Story(Story.Get()).Saro(Saro.Get());
	// Create the runtime-only sky/materials and actually render the CRT targets before waiting.
	M->Step(0);
	DrawCrts(0.1f);
	TWeakObjectPtr<AS47HUD> StartupSelf = this;
	S47Startup::Prepare(PC, M, [StartupSelf] { if (StartupSelf.IsValid()) StartupSelf->StartTests(); });
}

void AS47HUD::StartTests()
{
	S47MenuPointerTest::Start(US47Main::Get(this));
	S47CreaturePreview::Start(US47Main::Get(this));
	S47RoswellPreview::Start(US47Main::Get(this));
	S47RoswellAIProbe::Start(US47Main::Get(this));
	S47RoswellPoseProbe::Start(US47Main::Get(this));
	if (FParse::Param(FCommandLine::Get(), TEXT("S47SmokeTest")))
	{
		float Seconds = 3.f;
		FParse::Value(FCommandLine::Get(), TEXT("S47Seconds="), Seconds);
		// (on the core ticker, not the world's timers: those stand still while the pause menu holds the game)
		TWeakObjectPtr<AS47HUD> Self = this;
		FString Shot;
		FParse::Value(FCommandLine::Get(), TEXT("S47Shot="), Shot);
		// (a long frame, a picture painted on the CPU, can bring the shot and the end into one frame:
		// the end waits for the shot's file, up to ten seconds more)
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Shot](float)
		{
			TSharedRef<int32> Waits = MakeShared<int32>(0);
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Shot, Waits](float)
			{
				if (!Self.IsValid()) return false;
				if (!Shot.IsEmpty() && !IFileManager::Get().FileExists(*Shot) && ++*Waits <= 20) return true;
				Self->FinishSmokeTest();
				return false;
			}), 0.5f);
			return false;
		}), FMath::Max(0.f, Seconds - 0.5f));
		if (!Shot.IsEmpty())
		{
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Shot](float) { FScreenshotRequest::RequestScreenshot(Shot, true, false); return false; }), FMath::Max(0.5f, Seconds - 1.f));
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("S47Use")))
		{
			FTimerHandle UseTimer;
			GetWorldTimerManager().SetTimer(UseTimer, [this] { if (Walker.IsValid() && !Walker->FocusedId().IsEmpty()) Use(Walker->FocusedId()); else UE_LOG(LogS47HUD, Warning, TEXT("S47: nothing to use in front of the player")); }, 2.5f, false);
		}
		FString View;
		if (FParse::Value(FCommandLine::Get(), TEXT("S47Open="), View))
		{
			// -S47Press=Down,Enter: then the keys, as from the keyboard (S47Keys::TestPresses), from the
			// moment the view is open (a slow first frame must not send them before it)
			FString Press;
			FParse::Value(FCommandLine::Get(), TEXT("S47Press="), Press, false);
			FTimerHandle OpenTimer;
			GetWorldTimerManager().SetTimer(OpenTimer, [this, View, Press]
			{
				OpenView(View);
				if (!Press.IsEmpty()) FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Press](float) { S47Keys::TestPresses(Press, 0.5f); return false; }), 0.6f);
			}, 2.0f, false);
		}
	}
}

void AS47HUD::OpenView(const FString& View)
{
	if (View == TEXT("notebook")) OpenNotebook();
	else if (View == TEXT("console")) { if (Story.IsValid()) { Story->bConsoleOpen = true; } OpenConsole(); }
	else if (View == TEXT("ops")) OpenOps();
	else if (View == TEXT("log")) ShowDocument(S47Docs::ShiftLog(), [] {});
	else if (View.StartsWith(TEXT("doc:")) && Story.IsValid()) { for (const FS47Doc& D : Story->Docs()) if (D.Id == View.Mid(4)) { ShowDocument(D, [] {}); break; } } // a paper the story holds, by its id
	else if (View == TEXT("board")) { if (US47Main* M = US47Main::Get(this); M && M->CaseBoard) M->CaseBoard->Open(); }
	else if (View == TEXT("pause")) { if (US47Main* M = US47Main::Get(this); M && M->Flow) M->Flow->OpenPause(); }
	else if (US47Main* M = US47Main::Get(this); M && M->Flow) M->Flow->OpenTestView(View);
	UE_LOG(LogS47HUD, Display, TEXT("S47: opened %s"), *View);
}

void AS47HUD::Use(const FString& Id)
{
	if (Modal.IsValid()) return;
	UE_LOG(LogS47HUD, Display, TEXT("S47: used %s"), *Id);
	if (US47Main* M = US47Main::Get(this); M && M->Inter) { if (!(M->IsFreeform() && M->Freeform && M->Freeform->Use(Id)) && !(M->Spine && M->Spine->Use(Id)) && !(M->Observing && M->Observing->Use(Id)) && !(M->Horror && M->Horror->Use(Id)) && !(M->Evening && M->Evening->Use(Id))) M->Inter->UseId(Id); }
	else if (Story.IsValid()) Story->Use(Id);
}

// ---------- the story's requests ----------

void AS47HUD::Toast(const FString& Text, float Secs)
{
	if (Hud.IsValid()) Hud->Toast(Text, Secs);
}

void AS47HUD::SetModal(TSharedPtr<SWidget> Widget, TFunction<void()> OnClosed)
{
	UGameViewportClient* Viewport = GEngine ? GEngine->GameViewport : nullptr;
	APlayerController* PC = GetOwningPlayerController();
	if (!Viewport || !PC) return;
	// A dialogue line replaced by the next one closes and reopens the panel. Keep the pointer where
	// the player left it instead of where the brief game-mode capture parked it (FIX-06).
	const bool bReopen = !Modal.IsValid() && LastModalClosedAt > 0 && FPlatformTime::Seconds() - LastModalClosedAt < .75;
	const FVector2D KeepCursor = Modal.IsValid() && FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetCursorPos() : LastModalCursor;
	const bool bKeep = Modal.IsValid() || bReopen;
	CloseModal();
	Modal = Widget;
	ModalClosed = MoveTemp(OnClosed);
	Viewport->AddViewportWidgetContent(Widget.ToSharedRef(), 30);
	if (FSlateApplication::IsInitialized()) FSlateApplication::Get().SetAllUserFocus(Widget, EFocusCause::SetDirectly);
	PC->bShowMouseCursor = true;
	PC->SetInputMode(FInputModeUIOnly());
	if (bKeep && FSlateApplication::IsInitialized()) FSlateApplication::Get().SetCursorPos(KeepCursor);
	if (Walker.IsValid()) Walker->SetActive(false);
	if (Story.IsValid()) Story->SetPaused(true);
}

void AS47HUD::CloseModal()
{
	UGameViewportClient* Viewport = GEngine ? GEngine->GameViewport : nullptr;
	APlayerController* PC = GetOwningPlayerController();
	if (!Modal.IsValid() || !Viewport || !PC) return;
	if (FSlateApplication::IsInitialized()) LastModalCursor = FSlateApplication::Get().GetCursorPos();
	LastModalClosedAt = FPlatformTime::Seconds();
	Viewport->RemoveViewportWidgetContent(Modal.ToSharedRef());
	Modal.Reset();
	FInputModeGameOnly Mode;
	Mode.SetConsumeCaptureMouseDown(false);
	PC->SetInputMode(Mode);
	PC->bShowMouseCursor = false;
	const US47Main* Mn = US47Main::Get(this);
	if (Walker.IsValid() && !bEnded && (!Mn || (Mn->Mode == US47Main::EMode::Play && !(Mn->Evening && Mn->Evening->MealBusy())))) Walker->SetActive(true);
	if (Story.IsValid()) Story->SetPaused(false);
	TFunction<void()> Fn = MoveTemp(ModalClosed);
	ModalClosed = nullptr;
	if (Fn) Fn();
}

void AS47HUD::ShowDocument(const FS47Doc& Doc, TFunction<void()> OnClose)
{
	TWeakObjectPtr<AS47HUD> Self = this;
	if (Audio) Audio->Play(TEXT("paper0"), 0.5f);
	// UI.document: the page read by Kinski (Read document, without its pencil marks and rules) and the
	// transcript by the description reader (Read description), each when that reader is on: the view's
	// own actions (SS47Document)
	US47UI* UI = US47Main::Get(this)->UI;
	if (UI) UI->StopSpeech();
	SetModal(SNew(SS47Document).Doc(Doc).UI(UI).OnClose([Self] { if (Self.IsValid()) Self->CloseModal(); }), MoveTemp(OnClose));
	UE_LOG(LogS47HUD, Display, TEXT("S47: document %s"), *Doc.Id);
}

void AS47HUD::OpenConsole()
{
	TWeakObjectPtr<AS47HUD> Self = this;
	TWeakObjectPtr<US47Audio> A = Audio;
	TWeakObjectPtr<AS47Story> S = Story;
	// on the centre CRT itself, the camera up to its glass (SS47ScreenView); the modal closes when it is back
	const TSharedRef<SS47ScreenView> V = SNew(SS47ScreenView).Screen(1).Web(Saro.Get()).Walker(Walker.Get()).OnLeft([Self] { if (Self.IsValid()) Self->CloseModal(); });
	const TWeakPtr<SS47ScreenView> W = V;
	const TSharedRef<SS47Console> C = SNew(SS47Console).Story(Story.Get()).OnClose([W] { if (const TSharedPtr<SS47ScreenView> P = W.Pin()) P->Leave(); }).OnClick([A] { if (A.IsValid()) A->Play(TEXT("click"), 0.5f); });
	V->SetScreen(C, C);
	SetModal(V, [S] { if (S.IsValid()) S->ConsoleClosed(); });
	UE_LOG(LogS47HUD, Display, TEXT("S47: console opened"));
}

void AS47HUD::OpenOps()
{
	TWeakObjectPtr<AS47HUD> Self = this;
	// on the west desk's CRT (screen 3), the command line taking the keys (E is typed there)
	const TSharedRef<SS47ScreenView> V = SNew(SS47ScreenView).Screen(3).Web(Saro.Get()).Walker(Walker.Get()).bUseLeaves(false).OnLeft([Self] { if (Self.IsValid()) Self->CloseModal(); });
	const TWeakPtr<SS47ScreenView> W = V;
	const TSharedRef<SS47Ops> O = SNew(SS47Ops).Story(Story.Get()).OnClose([W] { if (const TSharedPtr<SS47ScreenView> P = W.Pin()) P->Leave(); });
	V->SetScreen(O, O->Line());
	SetModal(V, nullptr);
	UE_LOG(LogS47HUD, Display, TEXT("S47: ops terminal opened"));
}

void AS47HUD::OpenNotebook()
{
	// game.openNotebook (S47Flow: ui/UI.ts notebook)
	if (US47Main* M = US47Main::Get(this); M && M->Flow) M->Flow->OpenNotebook();
	UE_LOG(LogS47HUD, Display, TEXT("S47: notebook opened"));
}

void AS47HUD::Fade(bool bOn, const FString& Text)
{
	if (Hud.IsValid()) Hud->Fade(bOn, Text);
}

void AS47HUD::Place(double X, double Z, double Yaw)
{
	if (Walker.IsValid()) Walker->Place(X, Z, Yaw);
}

void AS47HUD::Face(const FString& Spot)
{
	if (!Walker.IsValid() || !Saro.IsValid()) return;
	// fixed spots in the web version's terms, or one metre and a bit in front of an item, facing it
	if (Spot == TEXT("room")) { const FVector Sp = Saro->SpawnThree(); Walker->Place(Sp.X, Sp.Z, Sp.Y); }
	else if (Spot == TEXT("window")) Walker->Place(0.0, -3.4, 0.0);
	else if (Spot == TEXT("outside")) Walker->Place(9.0, -8.0, 0.0);
	else if (Spot == TEXT("dish")) Walker->Place(9.5, -18.5, 0.0);
	else if (Spot.StartsWith(TEXT("at:")))
	{
		// at:x;z;yaw;pitch in the web version's terms (the command line stops a value at a comma)
		TArray<FString> V;
		Spot.Mid(3).ParseIntoArray(V, TEXT(";"));
		if (V.Num() >= 3) Walker->Place(FCString::Atod(*V[0]), FCString::Atod(*V[1]), FCString::Atod(*V[2]));
		if (V.Num() >= 4) Walker->Pitch = FCString::Atod(*V[3]);
	}
	else if (const FS47WebItem* It = Saro->Item(Spot))
	{
		const FVector C = It->bBox ? (It->Min + It->Max) * 0.5 : S47Three::ToThree(Saro->WorldUE(It->Node));
		FVector2D Away(-C.X, -C.Z);
		if (Away.Size() < 0.1) Away = FVector2D(0, 1);
		Away.Normalize();
		const double D = FMath::Min(1.3, It->Range * 0.6);
		const double Px = C.X + Away.X * D, Pz = C.Z + Away.Y * D;
		Walker->Place(Px, Pz, FMath::Atan2(-(C.X - Px), -(C.Z - Pz)));
		Walker->Pitch = FMath::Atan2(C.Y - 1.62, D);
	}
	UE_LOG(LogS47HUD, Display, TEXT("S47: walker placed at %s"), *Spot);
}

void AS47HUD::Checkpoint(const FString& Name)
{
	UE_LOG(LogS47HUD, Display, TEXT("S47: checkpoint %s"), *Name);
}

void AS47HUD::Shake(float Strength)
{
	if (Walker.IsValid()) Walker->Shake = Strength;
}

void AS47HUD::EndCard(const TArray<FString>& Lines)
{
	bEnded = true;
	if (US47Main* Mn = US47Main::Get(this)) Mn->Mode = US47Main::EMode::End; // (main.ts showCard: mode 'end')
	TWeakObjectPtr<AS47HUD> Self = this;
	SetModal(SNew(SS47EndCard).Lines(Lines)
		.OnAgain([Self] { if (Self.IsValid()) { if (US47Main* Mn = US47Main::Get(Self.Get())) Mn->Mode = US47Main::EMode::Play; Self->bEnded = false; Self->CloseModal(); Self->Fade(false, TEXT("")); if (Self->Story.IsValid()) Self->Story->Start(); } })
		.OnQuit([Self] { if (Self.IsValid()) UKismetSystemLibrary::QuitGame(Self.Get(), Self->GetOwningPlayerController(), EQuitPreference::Quit, false); }), nullptr);
	UE_LOG(LogS47HUD, Display, TEXT("S47: end card"));
}

void AS47HUD::LookTowards(const FVector& T, float K)
{
	// Prologue.update: the gentle look towards the array
	if (!Walker.IsValid()) return;
	AS47Walker* W = Walker.Get();
	const double Dx = T.X - W->Pos.X, Dz = T.Z - W->Pos.Z;
	const double Yaw = FMath::Atan2(-Dx, -Dz), Pitch = FMath::Atan2(T.Y - W->Eye, FMath::Sqrt(Dx * Dx + Dz * Dz));
	double Dy = Yaw - W->Yaw;
	Dy = FMath::Atan2(FMath::Sin(Dy), FMath::Cos(Dy));
	W->Yaw += Dy * K;
	W->Pitch += (Pitch - W->Pitch) * K;
}

// ---------- per frame ----------

void AS47HUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const bool bWasStarting = S47Startup::Blocking();
	S47Startup::Tick();
	// The frame which releases the loading screen still belongs to warmup. Do not charge its
	// delta to the story, audio or the newly scheduled smoke-test actions.
	if (bWasStarting) { DrawCrts(DeltaSeconds); return; }
	if (Audio) Audio->Tick(DeltaSeconds);
	APlayerController* PC = GetOwningPlayerController();
	// (Tab and the notebook's other keys are the flow's, registered with M->OnKey)
	// the prompt for what the player looks at (UI.prompt), unless something set its own this frame
	US47Main* M = US47Main::Get(this);
	const bool bPlaying = !M || M->Mode == US47Main::EMode::Play;
	if (M) M->Step(DeltaSeconds);
	if (Walker.IsValid() && FParse::Param(FCommandLine::Get(), TEXT("S47GamepadLog")))
	{
		static double Next = 0; const double Now = FPlatformTime::Seconds();
		if (Now >= Next) { Next = Now + .25; const FVector2D Move = S47Gamepad::Move(), Drive = S47Gamepad::Drive();
			UE_LOG(LogS47HUD, Display, TEXT("S47: pad state mode=%s modal=%d move=%.3f,%.3f drive=%.3f,%.3f pos=%.3f,%.3f yaw=%.4f pitch=%.4f"), S47Gamepad::UsingGamepad()?TEXT("xbox"):TEXT("keyboard"), Modal.IsValid()?1:0, Move.X,Move.Y,Drive.X,Drive.Y,Walker->Pos.X,Walker->Pos.Z,Walker->Yaw,Walker->Pitch);
			if (M && M->World && M->World->bDriving) if (const FS47DriveController* D = M->World->CabDrive())
				UE_LOG(LogS47HUD, Display, TEXT("S47: pad vehicle speed=%.4f pos=%.3f,%.3f heading=%.4f"), D->Speed, D->Pos.X, D->Pos.Z, D->Heading);
		}
	}
	// the keys the ported modules answer (main.ts input.onKey)
	if (M && PC)
	{
		TSet<FKey> Taken;
		for (const US47Main::FKeyHandler& K : M->Keys)
		{
			if (Modal.IsValid() && !K.bEvenModal) continue;
			// (an action's keys as S47Keys has them bound; the handler answers once a frame)
			const TArray<FKey, TInlineAllocator<2>> Ks = K.Action.IsNone() ? TArray<FKey, TInlineAllocator<2>>{ K.Key } : S47Keys::KeysOf(K.Action);
			for (const FKey& Key : Ks)
			{
				if (!Key.IsValid() || Taken.Contains(Key) || !S47Keys::WasPressed(PC, Key)) continue;
				if (K.Fn && K.Fn()) Taken.Add(Key);
				break;
			}
		}
	}
	if (!Modal.IsValid() && Walker.IsValid() && Story.IsValid())
	{
		if (!bPromptSet)
		{
			const FString F = Walker->FocusedId();
			const FString L = F.IsEmpty() || !M || !M->Inter || !bPlaying ? FString() : M->Inter->LabelOf(F);
			PromptLabel = L;
			PromptAlt.Reset();
			bPromptOwn = false;
		}
		// (input.enabled: only in play; the title and the end card hold the player)
		Walker->SetActive(bPlaying && !Story->IsCinematic() && !(M && M->Evening && M->Evening->MealBusy()) && !bEnded);
	}
	bPromptSet = false;
	DrawCrts(DeltaSeconds);
}

void AS47HUD::DrawCrts(float DeltaSeconds)
{
	CrtTimer += DeltaSeconds;
	if (CrtTimer < 1.f / 12.f || !Renderer || !Saro.IsValid() || !Story.IsValid()) return;
	const float Dt = CrtTimer;
	CrtTimer = 0.f;
	for (int32 i = 0; i < 4; ++i)
	{
		if (!Crts[i].IsValid()) continue;
		if (i < 3 && !Story->bPowered) continue;
		if (UTextureRenderTarget2D* RT = Saro->CrtTarget(i)) Renderer->DrawWidget(RT, Crts[i].ToSharedRef(), FVector2D(AS47Web::CrtW, AS47Web::CrtH), Dt);
	}
	// the ops screen glows as soon as the station has power
	Saro->SetCrtPowered(3, Story->bPowered);
}

void AS47HUD::FinishSmokeTest()
{
	FVector2D Size(0, 0);
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(Size);
	UE_LOG(LogS47HUD, Display, TEXT("S47: smoke test OK, RHI %s, frame %llu, viewport %dx%d, parts %d, phase %d, modal %s"),
		GDynamicRHI ? GDynamicRHI->GetName() : TEXT("none"), (unsigned long long)GFrameCounter, int32(Size.X), int32(Size.Y),
		Saro.IsValid() ? Saro->PartCount() : 0, Story.IsValid() ? (int32)Story->Phase() : -1, Modal.IsValid() ? TEXT("open") : TEXT("none"));
	if (!S47RecoveryCheck::Run(US47Main::Get(this))) { FPlatformMisc::RequestExitWithStatus(false,1);return; }
	FPlatformMisc::RequestExit(false, TEXT("S47SmokeTest"));
}

void AS47HUD::EndPlay(const EEndPlayReason::Type Reason)
{
	S47Gamepad::Shutdown();
	if (Renderer) { BeginCleanup(Renderer); Renderer = nullptr; }
	if (GEngine && GEngine->GameViewport)
	{
		if (Hud.IsValid()) GEngine->GameViewport->RemoveViewportWidgetContent(Hud.ToSharedRef());
		if (Modal.IsValid()) GEngine->GameViewport->RemoveViewportWidgetContent(Modal.ToSharedRef());
	}
	Hud.Reset();
	Modal.Reset();
	for (int32 i = 0; i < 4; ++i) Crts[i].Reset();
	Super::EndPlay(Reason);
}

void AS47HUD::SetPrompt(const FString& Label, const FString& Alt)
{
	// UI.prompt: E and the label, Q and the second thing (the HUD draws the keycaps)
	PromptLabel = Label;
	PromptAlt = Alt;
	bPromptOwn = true;
	bPromptSet = true;
}

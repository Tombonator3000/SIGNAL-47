#include "S47HUD.h"

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
#include "S47Audio.h"
#include "S47Ops.h"
#include "S47Rx.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47Story.h"
#include "S47BoardRules.h"
#include "S47CaseBoard.h"
#include "S47Inter.h"
#include "S47Chapter2.h"
#include "S47Chapters.h"
#include "S47Flow.h"
#include "S47FieldCamera.h"
#include "S47Drive.h"
#include "S47Chapter1.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Phones.h"
#include "S47Saves.h"
#include "Containers/Ticker.h"
#include "S47UI.h"
#include "S47World.h"
#include "S47Walker.h"
#include "SS47Crt.h"
#include "SS47Widgets.h"
#include "Slate/WidgetRenderer.h"
#include "TimerManager.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47HUD, Log, All);

AS47HUD::AS47HUD()
{
	PrimaryActorTick.bCanEverTick = true;
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
		bOk &= US47Phones::SelfTest(Lines); // (the archive's phone and its dishes, the roof, the office's telephone, the field book, the looks, the thread)
		for (const FString& L : Lines) if (L.StartsWith(TEXT("FAIL"))) UE_LOG(LogS47HUD, Warning, TEXT("S47: %s"), *L);
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
		.Prompt_Lambda([this] { return Prompt; })
		.Clock_Lambda([this, Playing] { return FText::FromString(Story.IsValid() && Playing() && !Modal.IsValid() ? Story->ClockText(false) : FString()); })
		.ShowAim_Lambda([this, Playing] { return !Modal.IsValid() && Playing(); });
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
		// main.ts: the phones, the archive and the records (the archive's phone, the office's telephone, the roof, the field book, the looks)
		M->Phones = NewObject<US47Phones>(M); M->Phones->Init(M);
		M->Flow->Boot();
		FString FaceAt;
		if (FParse::Value(FCommandLine::Get(), TEXT("S47Face="), FaceAt)) Face(FaceAt);
	}
	Renderer = new FWidgetRenderer(true);
	const SS47Crt::EKind Kinds[4] = { SS47Crt::EKind::Status, SS47Crt::EKind::Spectrum, SS47Crt::EKind::Tracking, SS47Crt::EKind::Ops };
	for (int32 i = 0; i < 4; ++i) Crts[i] = SNew(SS47Crt).Kind(Kinds[i]).Story(Story.Get()).Saro(Saro.Get());

	if (FParse::Param(FCommandLine::Get(), TEXT("S47SmokeTest")))
	{
		float Seconds = 3.f;
		FParse::Value(FCommandLine::Get(), TEXT("S47Seconds="), Seconds);
		// (on the core ticker, not the world's timers: those stand still while the pause menu holds the game)
		TWeakObjectPtr<AS47HUD> Self = this;
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self](float) { if (Self.IsValid()) Self->FinishSmokeTest(); return false; }), Seconds);
		FString Shot;
		if (FParse::Value(FCommandLine::Get(), TEXT("S47Shot="), Shot))
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
			FTimerHandle OpenTimer;
			GetWorldTimerManager().SetTimer(OpenTimer, [this, View] { OpenView(View); }, 2.0f, false);
		}
	}
}

void AS47HUD::OpenView(const FString& View)
{
	if (View == TEXT("notebook")) OpenNotebook();
	else if (View == TEXT("console")) { if (Story.IsValid()) { Story->bConsoleOpen = true; } OpenConsole(); }
	else if (View == TEXT("ops")) OpenOps();
	else if (View == TEXT("log")) ShowDocument(S47Docs::ShiftLog(), [] {});
	else if (View == TEXT("board")) { if (US47Main* M = US47Main::Get(this); M && M->CaseBoard) M->CaseBoard->Open(); }
	else if (View == TEXT("pause")) { if (US47Main* M = US47Main::Get(this); M && M->Flow) M->Flow->OpenPause(); }
	else if (US47Main* M = US47Main::Get(this); M && M->Flow) M->Flow->OpenTestView(View);
	UE_LOG(LogS47HUD, Display, TEXT("S47: opened %s"), *View);
}

void AS47HUD::Use(const FString& Id)
{
	if (Modal.IsValid()) return;
	UE_LOG(LogS47HUD, Display, TEXT("S47: used %s"), *Id);
	if (US47Main* M = US47Main::Get(this); M && M->Inter) M->Inter->UseId(Id);
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
	CloseModal();
	Modal = Widget;
	ModalClosed = MoveTemp(OnClosed);
	Viewport->AddViewportWidgetContent(Widget.ToSharedRef(), 30);
	if (FSlateApplication::IsInitialized()) FSlateApplication::Get().SetAllUserFocus(Widget, EFocusCause::SetDirectly);
	PC->bShowMouseCursor = true;
	PC->SetInputMode(FInputModeUIOnly());
	if (Walker.IsValid()) Walker->SetActive(false);
	if (Story.IsValid()) Story->SetPaused(true);
}

void AS47HUD::CloseModal()
{
	UGameViewportClient* Viewport = GEngine ? GEngine->GameViewport : nullptr;
	APlayerController* PC = GetOwningPlayerController();
	if (!Modal.IsValid() || !Viewport || !PC) return;
	Viewport->RemoveViewportWidgetContent(Modal.ToSharedRef());
	Modal.Reset();
	FInputModeGameOnly Mode;
	Mode.SetConsumeCaptureMouseDown(false);
	PC->SetInputMode(Mode);
	PC->bShowMouseCursor = false;
	const US47Main* Mn = US47Main::Get(this);
	if (Walker.IsValid() && !bEnded && (!Mn || Mn->Mode == US47Main::EMode::Play)) Walker->SetActive(true);
	if (Story.IsValid()) Story->SetPaused(false);
	TFunction<void()> Fn = MoveTemp(ModalClosed);
	ModalClosed = nullptr;
	if (Fn) Fn();
}

void AS47HUD::ShowDocument(const FS47Doc& Doc, TFunction<void()> OnClose)
{
	TWeakObjectPtr<AS47HUD> Self = this;
	if (Audio) Audio->Play(TEXT("paper0"), 0.5f);
	SetModal(SNew(SS47Document).Doc(Doc).OnClose([Self] { if (Self.IsValid()) Self->CloseModal(); }), MoveTemp(OnClose));
	UE_LOG(LogS47HUD, Display, TEXT("S47: document %s"), *Doc.Id);
}

void AS47HUD::OpenConsole()
{
	TWeakObjectPtr<AS47HUD> Self = this;
	TWeakObjectPtr<US47Audio> A = Audio;
	TWeakObjectPtr<AS47Story> S = Story;
	SetModal(SNew(SS47Console).Story(Story.Get()).OnClose([Self] { if (Self.IsValid()) Self->CloseModal(); }).OnClick([A] { if (A.IsValid()) A->Play(TEXT("click"), 0.5f); }),
		[S] { if (S.IsValid()) S->ConsoleClosed(); });
	UE_LOG(LogS47HUD, Display, TEXT("S47: console opened"));
}

void AS47HUD::OpenOps()
{
	TWeakObjectPtr<AS47HUD> Self = this;
	SetModal(SNew(SS47Ops).Story(Story.Get()).OnClose([Self] { if (Self.IsValid()) Self->CloseModal(); }), nullptr);
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
	if (Audio) Audio->Tick(DeltaSeconds);
	APlayerController* PC = GetOwningPlayerController();
	// (Tab and the notebook's other keys are the flow's, registered with M->OnKey)
	// the prompt for what the player looks at (UI.prompt), unless something set its own this frame
	US47Main* M = US47Main::Get(this);
	const bool bPlaying = !M || M->Mode == US47Main::EMode::Play;
	if (M) M->Step(DeltaSeconds);
	// the keys the ported modules answer (main.ts input.onKey)
	if (M && PC)
	{
		TSet<FKey> Taken;
		for (const US47Main::FKeyHandler& K : M->Keys)
		{
			if (Taken.Contains(K.Key) || (Modal.IsValid() && !K.bEvenModal) || !PC->WasInputKeyJustPressed(K.Key)) continue;
			if (K.Fn && K.Fn()) Taken.Add(K.Key);
		}
	}
	if (!Modal.IsValid() && Walker.IsValid() && Story.IsValid())
	{
		if (!bPromptSet)
		{
			const FString F = Walker->FocusedId();
			const FString L = F.IsEmpty() || !M || !M->Inter || !bPlaying ? FString() : M->Inter->LabelOf(F);
			Prompt = L.IsEmpty() ? FText::GetEmpty() : FText::FromString(L.ToUpper() + TEXT("   [E]"));
		}
		// (input.enabled: only in play; the title and the end card hold the player)
		Walker->SetActive(bPlaying && !Story->IsCinematic() && !bEnded);
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
	FPlatformMisc::RequestExit(false, TEXT("S47SmokeTest"));
}

void AS47HUD::EndPlay(const EEndPlayReason::Type Reason)
{
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
	// UI.prompt: "[E] label   [Q] alt"
	FString P = Label.IsEmpty() ? FString() : Label.ToUpper() + TEXT("   [E]");
	if (!Alt.IsEmpty()) P += TEXT("      ") + Alt.ToUpper() + TEXT("   [Q]");
	Prompt = FText::FromString(P);
	bPromptSet = true;
}

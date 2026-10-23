#include "S47Startup.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "MoviePlayer.h"
#include "PipelineStateCache.h"
#include "S47Audio.h"
#include "S47Main.h"
#include "S47Theme.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "Serialization/Archive.h"
#include "ShaderCompiler.h"
#include "ShaderPipelineCache.h"
#include "UObject/CoreUObjectDelegates.h"
#include "UObject/UObjectIterator.h"
#include "UnrealClient.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Startup, Log, All);

namespace
{
	struct FStartupState
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<APlayerController> PC;
		TWeakObjectPtr<US47Main> Main;
		TSharedPtr<SWidget> Screen;
		TWeakPtr<SWidget> PreviousFocus;
		TFunction<void()> OnReady;
		TUniquePtr<FArchive> Trace;
		double BeginAt = 0, PreparedAt = 0, ReadyAt = 0, LastFrameAt = 0, QuietAt = 0, LastScanAt = 0, LastLogAt = 0;
		uint32 Remaining = 0, RhiRequests = 0, MaterialJobs = 0, ShaderJobs = 0, Components = 0;
		uint32 QuietFrames = 0;
		uint64 ReadyFrame = 0;
		bool bStarted = false, bPrepared = false, bReady = false, bBypass = false, bPausedByUs = false, bMenuPausedAtPrepare = false;
		bool bCursor = false, bWebTickWhenPaused = false, bPlayerActive = false, bAudioSuspended = false, bShotRequested = false;
		int32 VisibleSecond = -1, VisibleFrames = 0;
		double VisibleMaxMs = 0;
		TArray<TWeakObjectPtr<UPrimitiveComponent>> Primitives;
	};
	FStartupState GStartup;
	FDelegateHandle GPreLoadMap;

	void WriteTrace(const FString& Line)
	{
		if (!GStartup.Trace) return;
		FTCHARToUTF8 Bytes(*Line);
		GStartup.Trace->Serialize(const_cast<ANSICHAR*>(Bytes.Get()), Bytes.Length());
	}

	class SS47Loading : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SS47Loading) : _bLive(false) {}
			SLATE_ARGUMENT(bool, bLive)
		SLATE_END_ARGS()
		void Construct(const FArguments& Args)
		{
			const bool bLive = Args._bLive;
			ChildSlot
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder).BorderImage(S47Theme::White()).BorderBackgroundColor(S47Theme::Night()).Padding(0)
					[
						SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(S47Theme::Pad(0, 0, 0, 22))
						[ SNew(SS47Osd).Text(FString(TEXT("SIGNAL / 47"))).Px(48) ]
						+ SVerticalBox::Slot().AutoHeight()
						[ SNew(STextBlock).Text(FText::FromString(TEXT("SARO  /  SYSTEM INITIALIZATION"))).Font(S47Theme::Term(22, 1)).ColorAndOpacity(S47Theme::Amber()) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(S47Theme::Pad(0, 16, 0, 0))
						[
							SNew(STextBlock).Text_Lambda([bLive]
							{
								if (!bLive || !GStartup.bPrepared) return FText::FromString(TEXT("CONNECTING RECEIVER..."));
								return FText::FromString(GStartup.Remaining || GStartup.MaterialJobs || GStartup.ShaderJobs
									? TEXT("WARMING RECEIVER...  PLEASE STAND BY") : TEXT("ACQUIRING SIGNAL..."));
							}).Font(S47Theme::Term(22)).ColorAndOpacity(S47Theme::Phos())
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(S47Theme::Pad(0, 30, 0, 0))
						[ SNew(STextBlock).Text(FText::FromString(TEXT("NEW MEXICO     /     APR.13 1986"))).Font(S47Theme::Term(18)).ColorAndOpacity(S47Theme::Mute()) ]
					]
					]
				]
				+ SOverlay::Slot()[S47Theme::Scanlines(0.1f)]
			];
		}
		virtual bool SupportsKeyboardFocus() const override { return true; }
		virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent&) override { return FReply::Handled(); }
		virtual FReply OnKeyUp(const FGeometry&, const FKeyEvent&) override { return FReply::Handled(); }
		virtual FReply OnKeyChar(const FGeometry&, const FCharacterEvent&) override { return FReply::Handled(); }
		virtual FReply OnAnalogValueChanged(const FGeometry&, const FAnalogInputEvent&) override { return FReply::Handled(); }
		virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
		virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
		virtual FReply OnMouseWheel(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	};

	void ShowScreen()
	{
		if (!GStartup.Screen.IsValid() && GEngine && GEngine->GameViewport)
		{
			GStartup.Screen = SNew(SS47Loading).bLive(true);
			GEngine->GameViewport->AddViewportWidgetContent(GStartup.Screen.ToSharedRef(), 10000);
		}
	}

	void ScanComponents(bool bPrecache = true)
	{
		GStartup.Primitives.Reset();
		for (TObjectIterator<UPrimitiveComponent> It; It; ++It)
		{
			UPrimitiveComponent* C = *It;
			if (C->IsTemplate() || C->GetWorld() != GStartup.World.Get() || !C->IsRegistered()) continue;
			// Runtime-created meshes and material instances do not pass through PostLoad.
			if (bPrecache && !C->GetPSOPrecacheComponentData().IsPSOPrecacheCalled()) C->PrecachePSOs();
			GStartup.Primitives.Add(C);
		}
		GStartup.Components = GStartup.Primitives.Num();
	}

	void SampleQueues()
	{
		GStartup.MaterialJobs = 0;
		for (const TWeakObjectPtr<UPrimitiveComponent>& C : GStartup.Primitives) if (C.IsValid() && C->IsPSOPrecaching()) ++GStartup.MaterialJobs;
		GStartup.RhiRequests = PipelineStateCache::NumActivePrecacheRequests();
		GStartup.Remaining = FShaderPipelineCache::NumPrecompilesRemaining();
		GStartup.ShaderJobs = GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : 0;
	}

	void Finish()
	{
		GStartup.bReady = true;
		GStartup.ReadyFrame = GFrameCounter;
		GStartup.ReadyAt = FPlatformTime::Seconds();
		FShaderPipelineCache::SetBatchMode(FShaderPipelineCache::BatchMode::Background);
		// Losing application focus can open the pause menu during loading. It owns that pause
		// and its input focus, so finishing warmup must not silently resume it.
		const bool bMenuPaused = GStartup.Main.IsValid() && GStartup.Main->bPausedByMenu;
		const bool bNewMenuPause = bMenuPaused && !GStartup.bMenuPausedAtPrepare;
		if (!GStartup.bBypass)
		{
			if (GStartup.Main.IsValid())
			{
				if (GStartup.Main->Web) GStartup.Main->Web->PrimaryActorTick.bTickEvenWhenPaused = GStartup.bWebTickWhenPaused;
				if (GStartup.Main->Player) GStartup.Main->Player->SetActive(GStartup.bPlayerActive && !bMenuPaused);
				if (GStartup.Main->Audio) GStartup.Main->Audio->Suspend(GStartup.bAudioSuspended || bMenuPaused);
			}
			if (GStartup.bPausedByUs && !bMenuPaused && GStartup.World.IsValid()) UGameplayStatics::SetGamePaused(GStartup.World.Get(), false);
			if (GStartup.PC.IsValid() && !bNewMenuPause)
			{
				APlayerController* PC = GStartup.PC.Get();
				PC->FlushPressedKeys();
				PC->bShowMouseCursor = GStartup.bCursor;
				if (GStartup.bCursor)
				{
					FInputModeUIOnly Mode;
					Mode.SetWidgetToFocus(GStartup.PreviousFocus.Pin());
					Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
					PC->SetInputMode(Mode);
				}
				else
				{
					FInputModeGameOnly Mode;
					Mode.SetConsumeCaptureMouseDown(false);
					PC->SetInputMode(Mode);
				}
			}
		}
		if (GStartup.Screen.IsValid() && GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(GStartup.Screen.ToSharedRef());
		GStartup.Screen.Reset();
		if (!GStartup.bBypass && !bNewMenuPause && FSlateApplication::IsInitialized())
		{
			if (TSharedPtr<SWidget> Focus = GStartup.PreviousFocus.Pin()) FSlateApplication::Get().SetAllUserFocus(Focus, EFocusCause::SetDirectly);
			else FSlateApplication::Get().SetAllUserFocusToGameViewport();
		}
		UE_LOG(LogS47Startup, Display, TEXT("S47: startup READY after %.3fs; remaining=%u rhi=%u material=%u shader=%u components=%u quietFrames=%u bypass=%d; gameplay and test clocks begin now"),
			GStartup.ReadyAt - GStartup.BeginAt, GStartup.Remaining, GStartup.RhiRequests, GStartup.MaterialJobs, GStartup.ShaderJobs, GStartup.Components, GStartup.QuietFrames, GStartup.bBypass);
		if (GStartup.Trace) GStartup.Trace->Flush();
		TFunction<void()> Fn = MoveTemp(GStartup.OnReady);
		if (Fn) Fn();
	}
}

void S47Startup::Initialize()
{
	if (IsRunningCommandlet() || IsRunningDedicatedServer()) return;
	// MoviePlayer keeps the same terminal visible during synchronous world construction. The
	// viewport copy then covers the rendered warmup, so no unfinished world frame is exposed.
	GPreLoadMap = FCoreUObjectDelegates::PreLoadMap.AddLambda([](const FString&)
	{
		if (GIsEditor || FParse::Param(FCommandLine::Get(), TEXT("S47NoStartup")) || !IsMoviePlayerEnabled()) return;
		FLoadingScreenAttributes Loading;
		Loading.bMoviesAreSkippable = false;
		Loading.bAutoCompleteWhenLoadingCompletes = true;
		Loading.WidgetLoadingScreen = SNew(SS47Loading).bLive(false);
		GetMoviePlayer()->SetupLoadingScreen(Loading);
	});
}

void S47Startup::Shutdown()
{
	if (GPreLoadMap.IsValid()) FCoreUObjectDelegates::PreLoadMap.Remove(GPreLoadMap);
	GPreLoadMap.Reset();
	if (GStartup.Screen.IsValid() && GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(GStartup.Screen.ToSharedRef());
	GStartup = FStartupState();
}

void S47Startup::Begin(UWorld* World)
{
	if (GStartup.Screen.IsValid() && GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(GStartup.Screen.ToSharedRef());
	GStartup = FStartupState();
	GStartup.World = World;
	GStartup.BeginAt = GStartup.LastFrameAt = FPlatformTime::Seconds();
	GStartup.bStarted = true;
	GStartup.bBypass = FParse::Param(FCommandLine::Get(), TEXT("S47NoStartup")) || !FApp::CanEverRender();
	FString TracePath;
	if (FParse::Value(FCommandLine::Get(), TEXT("S47StartupTrace="), TracePath))
	{
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(TracePath), true);
		GStartup.Trace.Reset(IFileManager::Get().CreateFileWriter(*TracePath));
		WriteTrace(TEXT("frame,wall_s,visible_s,covered,frame_ms,pso_remaining,rhi_requests,material_jobs,shader_jobs,components\n"));
	}
	if (!GStartup.bBypass)
	{
		FShaderPipelineCache::SetBatchMode(FShaderPipelineCache::BatchMode::Fast);
		if (FShaderPipelineCache::IsBatchingPaused()) FShaderPipelineCache::ResumeBatching();
		ShowScreen();
	}
	UE_LOG(LogS47Startup, Display, TEXT("S47: startup BEGIN; precache=%d filecache=%d bypass=%d"),
		int32(GRHISupportsPSOPrecaching), int32(GRHISupportsPipelineFileCache), GStartup.bBypass);
}

void S47Startup::Prepare(APlayerController* PC, US47Main* Main, TFunction<void()> OnReady)
{
	if (GStartup.bPrepared) return;
	GStartup.PC = PC;
	GStartup.Main = Main;
	GStartup.OnReady = MoveTemp(OnReady);
	GStartup.bPrepared = true;
	GStartup.PreparedAt = FPlatformTime::Seconds();
	GStartup.bCursor = PC && PC->bShowMouseCursor;
	GStartup.bMenuPausedAtPrepare = Main && Main->bPausedByMenu;
	if (GStartup.bBypass) { ScanComponents(false); SampleQueues(); Finish(); return; }
	ShowScreen();
	if (Main && Main->Player)
	{
		GStartup.bPlayerActive = Main->Player->IsActive();
		Main->Player->SetActive(false);
		Main->Player->Tick(0);
	}
	if (Main && Main->Audio)
	{
		GStartup.bAudioSuspended = Main->Audio->Ctx.IsValid() && Main->Audio->Ctx->State() == TEXT("suspended");
		Main->Audio->Suspend(true);
	}
	if (Main && Main->Web)
	{
		GStartup.bWebTickWhenPaused = Main->Web->PrimaryActorTick.bTickEvenWhenPaused;
		Main->Web->PrimaryActorTick.bTickEvenWhenPaused = true;
	}
	// World timers stand still too, including test input and chapter timers created during Init.
	if (GStartup.World.IsValid() && !UGameplayStatics::IsGamePaused(GStartup.World.Get()))
	{
		GStartup.bPausedByUs = UGameplayStatics::SetGamePaused(GStartup.World.Get(), true);
		if (!GStartup.bPausedByUs) UE_LOG(LogS47Startup, Error, TEXT("S47: startup could not pause the world's test timers"));
	}
	if (GStartup.Screen.IsValid() && FSlateApplication::IsInitialized())
	{
		GStartup.PreviousFocus = FSlateApplication::Get().GetKeyboardFocusedWidget();
		if (PC)
		{
			// Focus alone does not suppress the viewport's GameOnly input path.
			PC->FlushPressedKeys();
			PC->bShowMouseCursor = false;
			FInputModeUIOnly Mode;
			Mode.SetWidgetToFocus(GStartup.Screen);
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(Mode);
		}
		FSlateApplication::Get().SetAllUserFocus(GStartup.Screen, EFocusCause::SetDirectly);
	}
	ScanComponents();
}

void S47Startup::Tick()
{
	if (!GStartup.bStarted) return;
	const double Now = FPlatformTime::Seconds();
	const bool bWasReady = GStartup.bReady;
	const double FrameSeconds = Now - GStartup.LastFrameAt;
	GStartup.LastFrameAt = Now;
	// Keep the measurement columns live during the first visible seconds, including the
	// -S47NoStartup baseline. A READY flag must not turn later compile work into false zeroes.
	if (bWasReady && GStartup.Trace) SampleQueues();
	if (!GStartup.bReady && GStartup.bPrepared)
	{
		if (!GStartup.bShotRequested && Now - GStartup.PreparedAt >= 0.5)
		{
			GStartup.bShotRequested = true;
			FString Shot;
			if (FParse::Value(FCommandLine::Get(), TEXT("S47StartupShot="), Shot)) FScreenshotRequest::RequestScreenshot(Shot, true, false);
		}
		if (Now - GStartup.LastScanAt >= 0.25)
		{
			const uint32 Before = GStartup.Components;
			ScanComponents();
			if (Before != GStartup.Components) { GStartup.QuietAt = 0; GStartup.QuietFrames = 0; }
			GStartup.LastScanAt = Now;
		}
		SampleQueues();
		// Counting only the file cache can return zero before material collectors have queued their
		// RHI jobs. Require both, then sustained rendered frames for RT, VSM, TSR and sky captures.
		// A frame cap or a slow GPU must not create an endless loading screen. Queue completion
		// followed by 45 frames and a wall-time quiet period is independent of the chosen cap.
		const bool bQuiet = !GStartup.Remaining && !GStartup.RhiRequests && !GStartup.MaterialJobs && !GStartup.ShaderJobs;
		if (bQuiet)
		{
			if (!GStartup.QuietAt) GStartup.QuietAt = Now;
			++GStartup.QuietFrames;
		}
		else { GStartup.QuietAt = 0; GStartup.QuietFrames = 0; }
		if (Now - GStartup.LastLogAt >= 1)
		{
			UE_LOG(LogS47Startup, Display, TEXT("S47: startup WAIT t=%.3f remaining=%u rhi=%u material=%u shader=%u frame=%.3fms quiet=%u"),
				Now - GStartup.BeginAt, GStartup.Remaining, GStartup.RhiRequests, GStartup.MaterialJobs, GStartup.ShaderJobs, FrameSeconds * 1000, GStartup.QuietFrames);
			GStartup.LastLogAt = Now;
		}
		if (Now - GStartup.PreparedAt >= 3 && GStartup.QuietAt && Now - GStartup.QuietAt >= 1.25 && GStartup.QuietFrames >= 45) Finish();
	}
	const double Visible = bWasReady ? Now - GStartup.ReadyAt : -1;
	// Include the first sample at or beyond 10 s so the analyser can prove a complete window.
	if (!bWasReady || Visible < 10 || GStartup.Trace)
	{
		WriteTrace(FString::Printf(TEXT("%llu,%.6f,%.6f,%d,%.6f,%u,%u,%u,%u,%u\n"), static_cast<unsigned long long>(GFrameCounter),
			Now - GStartup.BeginAt, Visible, bWasReady ? 0 : 1, FrameSeconds * 1000,
			GStartup.Remaining, GStartup.RhiRequests, GStartup.MaterialJobs, GStartup.ShaderJobs, GStartup.Components));
	}
	if (bWasReady && Visible >= 10 && GStartup.Trace) { GStartup.Trace->Flush(); GStartup.Trace.Reset(); }
	if (GStartup.bReady && Visible >= 0 && Visible < 11)
	{
		const int32 Second = FMath::FloorToInt(Visible);
		if (Second != GStartup.VisibleSecond)
		{
			if (GStartup.VisibleSecond >= 0) UE_LOG(LogS47Startup, Display, TEXT("S47: startup VISIBLE second=%d frames=%d max=%.3fms"), GStartup.VisibleSecond, GStartup.VisibleFrames, GStartup.VisibleMaxMs);
			GStartup.VisibleSecond = Second;
			GStartup.VisibleFrames = 0;
			GStartup.VisibleMaxMs = 0;
		}
		++GStartup.VisibleFrames;
		GStartup.VisibleMaxMs = FMath::Max(GStartup.VisibleMaxMs, FrameSeconds * 1000);
	}
}

// Pause ticks have no actor ordering. The web actor must still use its warmup branch if the
// HUD releases the gate earlier in the same frame, otherwise its gameplay clock gets that delta.
bool S47Startup::Blocking() { return GStartup.bStarted && !GStartup.bBypass && (!GStartup.bReady || GFrameCounter == GStartup.ReadyFrame); }
bool S47Startup::Ready() { return !GStartup.bStarted || GStartup.bReady; }
double S47Startup::Elapsed() { return GStartup.bStarted ? FPlatformTime::Seconds() - GStartup.BeginAt : 0; }
double S47Startup::SecondsSinceReady() { return GStartup.bReady ? FPlatformTime::Seconds() - GStartup.ReadyAt : 0; }

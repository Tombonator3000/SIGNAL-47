#include "S47HUD.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "RHI.h"
#include "S47Saro.h"
#include "S47Walker.h"
#include "SS47Widgets.h"
#include "TimerManager.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47HUD, Log, All);

AS47HUD::AS47HUD()
{
	PrimaryActorTick.bCanEverTick = true;
}

FString AS47HUD::ClockText() const
{
	const int32 S = int32(ClockSeconds()) % 86400;
	return FString::Printf(TEXT("%02d:%02d:%02d"), S / 3600, (S / 60) % 60, S % 60);
}

void AS47HUD::BeginPlay()
{
	Super::BeginPlay();
	// The station is spawned in InitGame; the player and the viewport exist by the first tick.
	FTimerHandle T;
	GetWorldTimerManager().SetTimer(T, this, &AS47HUD::Start, 0.1f, false);
}

void AS47HUD::Start()
{
	UGameViewportClient* Viewport = GEngine ? GEngine->GameViewport : nullptr;
	APlayerController* PC = GetOwningPlayerController();
	if (!Viewport || !PC) return;
	for (TActorIterator<AS47Saro> It(GetWorld()); It; ++It) Saro = *It;
	Walker = Cast<AS47Walker>(PC->GetPawn());
	UE_LOG(LogS47HUD, Display, TEXT("S47: station %s, walker %s, RHI %s"), Saro.IsValid() ? TEXT("found") : TEXT("MISSING"), Walker.IsValid() ? TEXT("found") : TEXT("MISSING"), GDynamicRHI ? GDynamicRHI->GetName() : TEXT("none"));

	FString Face = TEXT("room");
	FParse::Value(FCommandLine::Get(), TEXT("S47Face="), Face);
	if (Walker.IsValid() && Saro.IsValid())
	{
		Walker->PlaceAt(Saro->StandingSpot(Face));
		TWeakObjectPtr<AS47HUD> Self = this;
		Walker->OnUse = [Self](const FString& Id) { if (Self.IsValid()) Self->Use(Id); };
		Walker->SetActive(true);
		UE_LOG(LogS47HUD, Display, TEXT("S47: walker placed at %s"), *Face);
	}
	Hud = SNew(SS47Hud).Visibility(EVisibility::HitTestInvisible).Prompt_Lambda([this] { return Prompt; }).Clock_Lambda([this] { return FText::FromString(ClockText()); });
	Viewport->AddViewportWidgetContent(Hud.ToSharedRef(), 20);
	PC->bShowMouseCursor = false;
	FInputModeGameOnly Mode;
	Mode.SetConsumeCaptureMouseDown(false);
	PC->SetInputMode(Mode);

	if (FParse::Param(FCommandLine::Get(), TEXT("S47SmokeTest")))
	{
		float Seconds = 3.f;
		FParse::Value(FCommandLine::Get(), TEXT("S47Seconds="), Seconds);
		GetWorldTimerManager().SetTimer(SmokeTimer, this, &AS47HUD::FinishSmokeTest, Seconds, false);
		FString Shot;
		if (FParse::Value(FCommandLine::Get(), TEXT("S47Shot="), Shot))
		{
			FTimerHandle ShotTimer;
			GetWorldTimerManager().SetTimer(ShotTimer, [Shot] { FScreenshotRequest::RequestScreenshot(Shot, true, false); }, FMath::Max(0.5f, Seconds - 1.f), false);
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("S47Use")))
		{
			FTimerHandle UseTimer;
			GetWorldTimerManager().SetTimer(UseTimer, [this] { if (Walker.IsValid() && !Walker->FocusedId().IsEmpty()) Use(Walker->FocusedId()); else UE_LOG(LogS47HUD, Warning, TEXT("S47: nothing to use in front of the player")); }, 2.5f, false);
		}
	}
}

void AS47HUD::Use(const FString& Id)
{
	UE_LOG(LogS47HUD, Display, TEXT("S47: used %s"), *Id);
	if (Id == TEXT("crt")) OpenTerminal();
	else if (Id.StartsWith(TEXT("door")) && Saro.IsValid()) Saro->ToggleDoor(Id);
	else if (Id == TEXT("phone")) Prompt = FText::FromString(TEXT("THE LINE IS QUIET"));
}

void AS47HUD::OpenTerminal()
{
	UGameViewportClient* Viewport = GEngine ? GEngine->GameViewport : nullptr;
	APlayerController* PC = GetOwningPlayerController();
	if (bTerminalOpen || !Viewport || !PC) return;
	TWeakObjectPtr<AS47HUD> Self = this;
	Terminal = SNew(SS47Terminal).Clock_Lambda([this] { return FText::FromString(ClockText()); }).OnClose([Self] { if (Self.IsValid()) Self->CloseTerminal(); });
	Viewport->AddViewportWidgetContent(Terminal.ToSharedRef(), 30);
	if (FSlateApplication::IsInitialized()) FSlateApplication::Get().SetAllUserFocus(Terminal, EFocusCause::SetDirectly);
	PC->SetInputMode(FInputModeUIOnly());
	if (Walker.IsValid()) Walker->SetActive(false);
	if (Saro.IsValid()) Saro->SetCrtPowered(true);
	bTerminalOpen = true;
	Prompt = FText::GetEmpty();
	UE_LOG(LogS47HUD, Display, TEXT("S47: terminal opened"));
}

void AS47HUD::CloseTerminal()
{
	UGameViewportClient* Viewport = GEngine ? GEngine->GameViewport : nullptr;
	APlayerController* PC = GetOwningPlayerController();
	if (!bTerminalOpen || !Viewport || !PC) return;
	if (Terminal.IsValid()) Viewport->RemoveViewportWidgetContent(Terminal.ToSharedRef());
	Terminal.Reset();
	FInputModeGameOnly Mode;
	Mode.SetConsumeCaptureMouseDown(false);
	PC->SetInputMode(Mode);
	PC->bShowMouseCursor = false;
	if (Walker.IsValid()) Walker->SetActive(true);
	bTerminalOpen = false;
	UE_LOG(LogS47HUD, Display, TEXT("S47: terminal closed"));
}

void AS47HUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	if (bTerminalOpen || !Walker.IsValid()) return;
	const FString F = Walker->FocusedId();
	if (F == TEXT("crt")) Prompt = FText::FromString(TEXT("STATUS TERMINAL  ·  E"));
	else if (F == TEXT("phone")) Prompt = FText::FromString(TEXT("TELEPHONE  ·  E"));
	else if (F.StartsWith(TEXT("door"))) Prompt = FText::FromString(Saro.IsValid() && Saro->IsDoorOpen(F) ? TEXT("DOOR  ·  E CLOSE") : TEXT("DOOR  ·  E OPEN"));
	else if (!Prompt.ToString().StartsWith(TEXT("THE LINE"))) Prompt = FText::GetEmpty();
}

void AS47HUD::FinishSmokeTest()
{
	FVector2D Size(0, 0);
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(Size);
	UE_LOG(LogS47HUD, Display, TEXT("S47: smoke test OK, RHI %s, frame %llu, viewport %dx%d, parts %d, terminal %s"),
		GDynamicRHI ? GDynamicRHI->GetName() : TEXT("none"), (unsigned long long)GFrameCounter, int32(Size.X), int32(Size.Y),
		Saro.IsValid() ? Saro->PartCount() : 0, bTerminalOpen ? TEXT("open") : TEXT("closed"));
	FPlatformMisc::RequestExit(false, TEXT("S47SmokeTest"));
}

void AS47HUD::EndPlay(const EEndPlayReason::Type Reason)
{
	if (GEngine && GEngine->GameViewport)
	{
		if (Hud.IsValid()) GEngine->GameViewport->RemoveViewportWidgetContent(Hud.ToSharedRef());
		if (Terminal.IsValid()) GEngine->GameViewport->RemoveViewportWidgetContent(Terminal.ToSharedRef());
	}
	Hud.Reset();
	Terminal.Reset();
	Super::EndPlay(Reason);
}

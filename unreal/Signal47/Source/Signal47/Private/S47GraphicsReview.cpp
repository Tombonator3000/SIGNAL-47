#include "S47GraphicsReview.h"
#include "S47Web.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Walker.h"
#include "S47World.h"
#include "S47Legs.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47Chapter3.h"
#include "S47Station.h"
#include "S47Night.h"
#include "S47Flashlight.h"
#include "S47Truck.h"
#include "S47Flow.h"
#include "S47UI.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderTimer.h"
#include "DynamicRHI.h"
#include "UnrealClient.h"

namespace S47GraphicsReview
{
enum class EState { World, RackOnline, RackDark, IndicatorsOnline, IndicatorsDark, ReceiverOnline, ReceiverDark,
 FlashlightOff, FlashlightOn, HeadlightsOff, HeadlightsOn, MenuTitle, MenuPause, MenuSettings, MenuSave, MenuLoad, MenuNotebook };
struct FCamera { const TCHAR* Name; const TCHAR* Area; double X, Z, Yaw, Pitch; int32 Lab = -1; EState State = EState::World; };
static constexpr int32 WorldCameraCount = 20;
// three.js metres, yaw and pitch in radians. Kept identical for baseline, candidate and PSO recording.
static const FCamera Cameras[] = {
 { TEXT("control"), TEXT("saro"), 2.8, 3.5, 0.22, -0.10 },
 { TEXT("crt"), TEXT("saro"), 1.8, -0.8, 0, -0.10 },
 { TEXT("annex"), TEXT("saro"), -2.5, 8.0, -1.2, -0.15 },
 { TEXT("corridor"), TEXT("saro"), 3.2, 4.5, -1.5708, -0.06 },
 { TEXT("lab-normal"), TEXT("saro"), 15.5, -1, 0, -0.12, 0 },
 { TEXT("lab-safe"), TEXT("saro"), 15.5, -1, 0, -0.12, 1 },
 { TEXT("lab-print"), TEXT("saro"), 15.5, -1, 0, -0.12, 2 },
 { TEXT("yard"), TEXT("saro"), 9, -8, 0, -0.04 },
 { TEXT("service-yard"), TEXT("saro"), 11.2, -11, -1.5708, -0.2 },
 { TEXT("lot"), TEXT("saro"), 7, 19, 0.08, 0.02 },
 { TEXT("motel"), TEXT("saro"), -39, 684, 0.7, -0.05 },
 { TEXT("room6"), TEXT("room6"), -1.1, -7997.7, -0.15, -0.12 },
 { TEXT("station"), TEXT("station01"), -21.4, 8030.2, -0.5227048, -0.03 },
 { TEXT("hut"), TEXT("station01"), -10.6, 8004.2, 0.35, -0.12 },
 { TEXT("highway"), TEXT("road"), 8002, 1978, -0.05, -0.03 },
 { TEXT("diner-outside"), TEXT("diner"), 7990, 2013, 0.85, -0.03 },
 { TEXT("diner"), TEXT("diner"), 7977.15, 1999.95, 1.5708, -0.12 },
 { TEXT("caliche"), TEXT("diner"), 7998.5, 2040, 3.14159, -0.02 },
 { TEXT("old-road"), TEXT("roswell"), 19080.25, -4575.53, 0.1, -0.02 },
 { TEXT("cab"), TEXT("saro"), 0, 0, 0, 0 },
 // Extra state coverage is opt-in; the original twenty cameras retain their baseline views.
 { TEXT("rack-online"), TEXT("saro"), 0, 0, 0, 0, -1, EState::RackOnline },
 { TEXT("rack-dark"), TEXT("saro"), 0, 0, 0, 0, -1, EState::RackDark },
 { TEXT("indicators-online"), TEXT("saro"), -0.4, 2.2, UE_DOUBLE_PI, -0.35, -1, EState::IndicatorsOnline },
 { TEXT("indicators-dark"), TEXT("saro"), -0.4, 2.2, UE_DOUBLE_PI, -0.35, -1, EState::IndicatorsDark },
 { TEXT("receiver-online"), TEXT("station01"), -11.85, 8003.3, 0, -0.4, -1, EState::ReceiverOnline },
 { TEXT("receiver-dark"), TEXT("station01"), -11.85, 8003.3, 0, -0.4, -1, EState::ReceiverDark },
 { TEXT("flashlight-off"), TEXT("saro"), 11.2, -11, -1.5708, -0.2, -1, EState::FlashlightOff },
 { TEXT("flashlight-on"), TEXT("saro"), 11.2, -11, -1.5708, -0.2, -1, EState::FlashlightOn },
 { TEXT("headlights-off"), TEXT("saro"), 0, 0, 0, 0, -1, EState::HeadlightsOff },
 { TEXT("headlights-on"), TEXT("saro"), 0, 0, 0, 0, -1, EState::HeadlightsOn },
 { TEXT("menu-title"), TEXT("saro"), 2.8, 3.5, 0.22, -0.10, -1, EState::MenuTitle },
 { TEXT("menu-pause"), TEXT("saro"), 2.8, 3.5, 0.22, -0.10, -1, EState::MenuPause },
 { TEXT("menu-settings"), TEXT("saro"), 2.8, 3.5, 0.22, -0.10, -1, EState::MenuSettings },
 { TEXT("menu-save"), TEXT("saro"), 2.8, 3.5, 0.22, -0.10, -1, EState::MenuSave },
 { TEXT("menu-load"), TEXT("saro"), 2.8, 3.5, 0.22, -0.10, -1, EState::MenuLoad },
 { TEXT("menu-notebook"), TEXT("saro"), 2.8, 3.5, 0.22, -0.10, -1, EState::MenuNotebook },
 { TEXT("terminal-online"), TEXT("saro"), -0.05, 3.4, 2.5, -0.1, -1, EState::IndicatorsOnline },
 { TEXT("terminal-dark"), TEXT("saro"), -0.05, 3.4, 2.5, -0.1, -1, EState::IndicatorsDark }
};
void Tick(AS47Web* W, float)
{
 FString Directory;
 if (!FParse::Value(FCommandLine::Get(), TEXT("S47GraphicsReview="), Directory) || UGameplayStatics::IsGamePaused(W)) return;
 US47Main* M = US47Main::Get(W);
 if (!M || !M->World || !M->Player || !M->Story) return;
 struct FState {
  TWeakObjectPtr<AS47Web> World;
  double BeginAt = 0, CameraAt = 0, LastAt = 0, FirstSampleAt = 0, LastSampleAt = 0, InitialEye = 1.62;
  uint64 FirstFrame = 0, LastFrame = 0;
  int32 Step = -1, Slot = 0;
  bool Shot = false, Done = false, CabReady = false;
  TArray<int32> Selection;
  TSharedPtr<SWidget> Title;
  TArray<float> Frames, Game, Draw, Gpu;
 };
 static FState S;
 const double Now = FPlatformTime::Seconds();
 if (S.World != W) {
  S = FState(); S.World = W; S.BeginAt = S.LastAt = Now; S.InitialEye = M->Player->Eye;
  IFileManager::Get().MakeDirectory(*Directory, true);
  FString Selected;
  if (!FParse::Value(FCommandLine::Get(), TEXT("S47GraphicsViews="), Selected, false))
   FParse::Value(FCommandLine::Get(), TEXT("S47GraphicsView="), Selected);
  if (Selected.IsEmpty()) {
   for (int32 I = 0; I < WorldCameraCount; ++I) S.Selection.Add(I);
  } else {
   TArray<FString> Names; Selected.ParseIntoArray(Names, TEXT(","));
   for (const FString& Name : Names) {
    int32 Found = INDEX_NONE;
    for (int32 I = 0; I < int32(UE_ARRAY_COUNT(Cameras)); ++I) if (Name == Cameras[I].Name) { Found = I; break; }
    if (Found == INDEX_NONE || S.Selection.Contains(Found)) {
     UE_LOG(LogTemp, Error, TEXT("S47: invalid/duplicate graphics camera %s"), *Name);
     S.Done = true; return;
    }
    S.Selection.Add(Found);
   }
  }
  const FString Header = TEXT("view,clock,samples,sample_seconds,first_frame,last_frame,frame_sum_ms,mean_ms,p95_ms,game_ms,draw_ms,gpu_ms\n");
  if (!FFileHelper::SaveStringToFile(Header, *(Directory / TEXT("frames.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) {
   UE_LOG(LogTemp, Error, TEXT("S47: could not write graphics frames.csv")); S.Done = true; return;
  }
 }
 if (S.Done) return;
 const double FrameMs = (Now - S.LastAt) * 1000;
 S.LastAt = Now;
 float Warmup = 8.f, Hold = 7.f;
 FParse::Value(FCommandLine::Get(), TEXT("S47GraphicsWarmup="), Warmup);
 FParse::Value(FCommandLine::Get(), TEXT("S47GraphicsHold="), Hold);
 Hold = FMath::Max(4.f, Hold);
 M->Story->SetPaused(true);
 FString Clock = TEXT("02:15"); FParse::Value(FCommandLine::Get(), TEXT("S47GraphicsClock="), Clock);
 TArray<FString> Parts; Clock.ParseIntoArray(Parts, TEXT(":"));
 if (Parts.Num() >= 2) M->Story->SetClock(FCString::Atod(*Parts[0]) * 3600 + FCString::Atod(*Parts[1]) * 60);
 if (Now - S.BeginAt < Warmup) return;
 if (S.Slot >= S.Selection.Num()) {
  M->Player->Eye = S.InitialEye;
  // The review's static state must not retain Slate resources past renderer shutdown.
  // Normally the next camera releases the title; a title-last batch has no next camera.
  if (M->UI) { M->UI->Close(true); if (S.Title) M->UI->RemoveScreen(S.Title); }
  S.Title.Reset();
  S.Done = true;
  UE_LOG(LogTemp, Display, TEXT("S47: graphics review complete; captured=%d clock=%s"), S.Selection.Num(), *Clock);
  return;
 }
 const int32 Next = S.Selection[S.Slot];
 const FCamera& V = Cameras[Next];
 const bool TerminalFront = FString(V.Name) == TEXT("terminal-online") || FString(V.Name) == TEXT("terminal-dark");
 M->Player->Eye = TerminalFront ? 1.0 : S.InitialEye;
 const bool InCab = FString(V.Name) == TEXT("cab") || V.State == EState::HeadlightsOff || V.State == EState::HeadlightsOn;
 if (Next != S.Step) {
  auto SaroPowerState = [](EState State) { return State == EState::RackOnline || State == EState::RackDark || State == EState::IndicatorsOnline || State == EState::IndicatorsDark; };
  if (S.Step >= 0 && SaroPowerState(Cameras[S.Step].State) && !SaroPowerState(V.State)) {
   M->Story->bPowered = true; M->Story->Rx.bPowered = true; M->Story->Rx.Stage = 3; M->Story->PowerOut(false);
   W->SetLightsScale(1.f, 1.f); W->SetTubes(true); W->SetLever(true); W->SetLeds(true);
   for (int32 I = 0; I < 4; ++I) W->SetCrtPowered(I, true);
  }
  S.Step = Next; S.CameraAt = Now; S.Shot = false; S.CabReady = false;
  S.Frames.Reset(); S.Game.Reset(); S.Draw.Reset(); S.Gpu.Reset();
  S.FirstFrame = S.LastFrame = 0;
  if (M->UI) { M->UI->Close(true); if (S.Title) M->UI->RemoveScreen(S.Title); S.Title.Reset(); }
  if (M->Night && M->Night->Flashlight) M->Night->Flashlight->bOn = false;
  // StopDriving is synchronous (unlike Park); consecutive cab-light states reuse the cab.
  if (M->World->bDriving && !InCab) M->World->StopDriving();
  M->World->Enter(V.Area);
  if (InCab) {
   if (!M->World->bDriving) { M->World->PlaceTruck(TEXT("saro")); M->World->Roam(TEXT("saro")); }
   M->World->HoldTruck(1);
   if (FS47Truck* Truck = M->World->CabTruck()) { Truck->SetLightLevel(1); Truck->SetHeadlights(V.State != EState::HeadlightsOff); }
  }
  if (V.State == EState::RackOnline || V.State == EState::RackDark || V.State == EState::IndicatorsOnline || V.State == EState::IndicatorsDark) {
   const bool Online = V.State == EState::RackOnline || V.State == EState::IndicatorsOnline;
   M->Story->bPowered = Online; M->Story->Rx.bPowered = Online; M->Story->Rx.Stage = Online ? 3 : 0; M->Story->PowerOut(!Online);
   // Story is paused during a tour, so drive its real lamp inputs here as well.
   // These uniform power changes preserve the recorded shader/PSO permutations.
   W->SetLightsScale(Online ? 1.f : 0.f, Online ? 1.f : 0.f);
   W->SetTubes(Online);
   W->SetLever(Online); W->SetLeds(Online);
   for (int32 I = 0; I < 4; ++I) W->SetCrtPowered(I, Online);
  }
  if (V.State == EState::ReceiverOnline || V.State == EState::ReceiverDark) {
   US47Chapter3* Chapter = Cast<US47Chapter3>(M->Story->Chapter(TEXT("ch3")));
   if (!Chapter || !Chapter->Station) { UE_LOG(LogTemp, Error, TEXT("S47: graphics receiver hook unavailable")); S.Done = true; return; }
   Chapter->Station->SetReceiver(V.State == EState::ReceiverOnline);
  }
  if (V.State == EState::FlashlightOn && M->Night && M->Night->Flashlight) {
   M->Night->Flashlight->bHave = true; M->Night->Flashlight->bOn = true;
  }
  if (M->UI && M->Flow) {
   if (V.State == EState::MenuTitle) { FS47TitleOpts O; O.bGate = false; S.Title = M->UI->Title(O); }
   if (V.State == EState::MenuPause) M->UI->Pause(M->Flow->Settings());
   if (V.State == EState::MenuSettings) M->Flow->OpenTestView(TEXT("settings:picture"));
   if (V.State == EState::MenuSave) M->Flow->OpenTestView(TEXT("save"));
   if (V.State == EState::MenuLoad) M->Flow->OpenTestView(TEXT("load"));
   if (V.State == EState::MenuNotebook) M->Flow->OpenTestView(TEXT("notebook:findings"));
  }
  if (V.Lab >= 0) {
   W->SetDoorOpen(TEXT("lab"), V.Lab == 0, true);
   W->Light(TEXT("lab.normal")).SetIntensity(V.Lab == 0 ? 1 : 0);
   W->Light(TEXT("lab.safelight")).SetIntensity(V.Lab != 0 ? 1 : 0);
   W->Light(TEXT("lab.enlarger")).SetIntensity(V.Lab == 2 ? 1 : 0);
  }
  UE_LOG(LogTemp, Display, TEXT("S47: graphics camera %s area=%s x=%.5f z=%.5f yaw=%.7f pitch=%.5f clock=%s"), V.Name, V.Area, V.X, V.Z, V.Yaw, V.Pitch, *Clock);
 }
 if (InCab) {
  // Roam schedules the real cab entry and Fade(false) on Story::After(0.4).
  // Let just that transition run before freezing the review clock again.
  if (!M->World->bDriving) { M->Story->SetPaused(false); return; }
  if (!S.CabReady) {
   S.CabReady = true; S.CameraAt = Now;
   UE_LOG(LogTemp, Display, TEXT("S47: graphics cab ready; transition complete, sample window restarted"));
  }
  M->Story->SetPaused(true);
  // StartTrip turns its lamps on after its transition; hold the requested state throughout.
  M->World->HoldTruck(1);
  if (FS47Truck* Truck = M->World->CabTruck()) Truck->SetHeadlights(V.State != EState::HeadlightsOff);
 }
 if (FString(V.Name) != TEXT("cab") && V.State != EState::HeadlightsOff && V.State != EState::HeadlightsOn) {
  double X = V.X, Z = V.Z, Yaw = V.Yaw, Pitch = V.Pitch;
  if (V.State == EState::RackOnline || V.State == EState::RackDark) {
   const FVector Rack = M->Story->RackAt();
   // The receiver's button/front panels face west. Keep the lens on their front side.
   X = Rack.X - 0.95; Z = Rack.Z; Yaw = -UE_DOUBLE_HALF_PI; Pitch = -0.26;
  }
  // Auxiliary CB and terminal-front states use their authored diagnostic camera values.
  // Diagnostic teleports must start above the authored room floor, not the previous motel terrain.
  if (FString(V.Area) == TEXT("room6")) M->Player->FloorY = 0;
  if (FString(V.Area) == TEXT("road")) {
   const FVector Origin = S47Legs::ORIGINS(TEXT("road"));
   M->Player->FloorY = Origin.Y + S47RoadTerrain::SurfaceY(V.X - Origin.X, V.Z - Origin.Z);
  }
  M->Player->Place(X, Z, Yaw); M->Player->Pitch = Pitch;
 }
 const double Since = Now - S.CameraAt;
 const FString ShotPath = Directory / (FString(V.Name) + TEXT(".png"));
 if (S.Shot) {
  // A request alone is not a completed capture. Keep this camera until the file exists;
  // the Python proof validates its PNG chunks, resolution and matching CSV sample.
  if (IFileManager::Get().FileSize(*ShotPath) > 32 && Since >= Hold) {
   UE_LOG(LogTemp, Display, TEXT("S47: graphics captured %s"), V.Name);
   ++S.Slot;
  }
  return;
 }
 if (Since >= Hold * 0.5 && Since < Hold - 0.3) {
  if (S.Frames.IsEmpty()) { S.FirstFrame = GFrameCounter; S.FirstSampleAt = Now - FrameMs / 1000; }
  S.LastFrame = GFrameCounter; S.LastSampleAt = Now;
  // Raw wall time includes hitches; world DeltaTime may be clamped by the engine.
  S.Frames.Add(float(FrameMs)); S.Game.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));
  S.Draw.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime)); S.Gpu.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));
 }
 if (Since < Hold - 0.3 || S.Frames.IsEmpty()) return;
 const FVector ActualEye = S47Three::ToThree(M->Player->EyeUE());
 UE_LOG(LogTemp, Display, TEXT("S47: graphics actual pose %s area=%s floor=%.5f eyeThree=(%.5f,%.5f,%.5f)"),
  V.Name, V.Area, M->Player->FloorY, ActualEye.X, ActualEye.Y, ActualEye.Z);
 if (FString(V.Area) == TEXT("room6") && FMath::Abs(M->Player->FloorY) > 0.3) {
  UE_LOG(LogTemp, Error, TEXT("S47: graphics room6 camera is outside authored floor height")); S.Done = true; return;
 }
 S.Shot = true;
 auto Mean = [](const TArray<float>& A) { double N = 0; for (float X : A) N += X; return N / A.Num(); };
 TArray<float> Ordered = S.Frames; Ordered.Sort();
 const float P95 = Ordered[FMath::Clamp(FMath::CeilToInt(Ordered.Num() * .95) - 1, 0, Ordered.Num() - 1)];
 const double Average = Mean(S.Frames);
 const FString Row = FString::Printf(TEXT("%s,%s,%d,%.6f,%llu,%llu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n"),
  V.Name, *Clock, S.Frames.Num(), S.LastSampleAt - S.FirstSampleAt,
  static_cast<unsigned long long>(S.FirstFrame), static_cast<unsigned long long>(S.LastFrame),
  Average * S.Frames.Num(), Average, P95, Mean(S.Game), Mean(S.Draw), Mean(S.Gpu));
 if (!FFileHelper::SaveStringToFile(Row, *(Directory / TEXT("frames.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append)) {
  UE_LOG(LogTemp, Error, TEXT("S47: could not append graphics sample")); S.Done = true; return;
 }
 FScreenshotRequest::RequestScreenshot(ShotPath, true, false);
 UE_LOG(LogTemp, Display, TEXT("S47: graphics sample %s"), *Row.TrimEnd());
}
}

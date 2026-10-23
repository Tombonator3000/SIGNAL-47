#include "S47GraphicsReview.h"
#include "S47Web.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Walker.h"
#include "S47World.h"
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
struct FCamera { const TCHAR* Name; const TCHAR* Area; double X, Z, Yaw, Pitch; int32 Lab = -1; };
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
 { TEXT("room6"), TEXT("room6"), -1.4, -7995.7, 0.15, -0.12 },
 { TEXT("station"), TEXT("station01"), -21.4, 8030.2, -0.5227048, -0.03 },
 { TEXT("hut"), TEXT("station01"), -10.6, 8004.2, 0.35, -0.12 },
 { TEXT("highway"), TEXT("road"), 8002, 1978, -0.05, -0.03 },
 { TEXT("diner-outside"), TEXT("diner"), 7990, 2013, 0.85, -0.03 },
 { TEXT("diner"), TEXT("diner"), 7977.15, 1999.95, 1.5708, -0.12 },
 { TEXT("caliche"), TEXT("diner"), 7998.5, 2040, 3.14159, -0.02 },
 { TEXT("old-road"), TEXT("roswell"), 19080.25, -4575.53, 0.1, -0.02 },
 { TEXT("cab"), TEXT("saro"), 0, 0, 0, 0 }
};
void Tick(AS47Web* W, float)
{
 FString Directory;
 if (!FParse::Value(FCommandLine::Get(), TEXT("S47GraphicsReview="), Directory) || UGameplayStatics::IsGamePaused(W)) return;
 US47Main* M = US47Main::Get(W);
 if (!M || !M->World || !M->Player || !M->Story) return;
 struct FState {
  TWeakObjectPtr<AS47Web> World;
  double BeginAt = 0, CameraAt = 0, LastAt = 0, FirstSampleAt = 0, LastSampleAt = 0;
  uint64 FirstFrame = 0, LastFrame = 0;
  int32 Step = -1, Slot = 0;
  bool Shot = false, Done = false;
  TArray<int32> Selection;
  TArray<float> Frames, Game, Draw, Gpu;
 };
 static FState S;
 const double Now = FPlatformTime::Seconds();
 if (S.World != W) {
  S = FState(); S.World = W; S.BeginAt = S.LastAt = Now;
  IFileManager::Get().MakeDirectory(*Directory, true);
  FString Selected;
  if (!FParse::Value(FCommandLine::Get(), TEXT("S47GraphicsViews="), Selected))
   FParse::Value(FCommandLine::Get(), TEXT("S47GraphicsView="), Selected);
  if (Selected.IsEmpty()) {
   for (int32 I = 0; I < int32(UE_ARRAY_COUNT(Cameras)); ++I) S.Selection.Add(I);
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
  S.Done = true;
  UE_LOG(LogTemp, Display, TEXT("S47: graphics review complete; captured=%d clock=%s"), S.Selection.Num(), *Clock);
  return;
 }
 const int32 Next = S.Selection[S.Slot];
 const FCamera& V = Cameras[Next];
 if (Next != S.Step) {
  S.Step = Next; S.CameraAt = Now; S.Shot = false;
  S.Frames.Reset(); S.Game.Reset(); S.Draw.Reset(); S.Gpu.Reset();
  S.FirstFrame = S.LastFrame = 0;
  M->World->Enter(V.Area);
  if (FString(V.Name) == TEXT("cab")) { M->World->PlaceTruck(TEXT("saro")); M->World->Roam(TEXT("saro")); M->World->HoldTruck(1); }
  if (V.Lab >= 0) {
   W->SetDoorOpen(TEXT("lab"), V.Lab == 0, true);
   W->Light(TEXT("lab.normal")).SetIntensity(V.Lab == 0 ? 1 : 0);
   W->Light(TEXT("lab.safelight")).SetIntensity(V.Lab != 0 ? 1 : 0);
   W->Light(TEXT("lab.enlarger")).SetIntensity(V.Lab == 2 ? 1 : 0);
  }
  UE_LOG(LogTemp, Display, TEXT("S47: graphics camera %s area=%s x=%.5f z=%.5f yaw=%.7f pitch=%.5f clock=%s"), V.Name, V.Area, V.X, V.Z, V.Yaw, V.Pitch, *Clock);
 }
 if (FString(V.Name) != TEXT("cab")) { M->Player->Place(V.X, V.Z, V.Yaw); M->Player->Pitch = V.Pitch; }
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

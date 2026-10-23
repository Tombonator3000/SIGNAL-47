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
void Tick(AS47Web* W, float Dt)
{
 FString Directory;
 if (!FParse::Value(FCommandLine::Get(), TEXT("S47GraphicsReview="), Directory) || UGameplayStatics::IsGamePaused(W)) return;
 US47Main* M = US47Main::Get(W);
 if (!M || !M->World || !M->Player || !M->Story) return;
 struct FState {
  TWeakObjectPtr<AS47Web> World;
  double Time = 0, CameraTime = 0; int32 Step = -1; bool Shot = false, Done = false;
  TArray<float> Frames, Game, Draw, Gpu;
 };
 static FState S;
 if (S.World != W) { S = FState(); S.World = W; }
 if (S.Done) return;
 S.Time += Dt;
 float Warmup = 20.f, Hold = 7.f;
 FParse::Value(FCommandLine::Get(), TEXT("S47GraphicsWarmup="), Warmup);
 FParse::Value(FCommandLine::Get(), TEXT("S47GraphicsHold="), Hold);
 Hold = FMath::Max(4.f, Hold);
 if (S.Time < Warmup) return;
 FString Single;
 FParse::Value(FCommandLine::Get(), TEXT("S47GraphicsView="), Single);
 int32 Next = FMath::Max(0, int32((S.Time - Warmup) / Hold));
 if (!Single.IsEmpty()) {
  if (Next > 0) Next = UE_ARRAY_COUNT(Cameras);
  else { Next = UE_ARRAY_COUNT(Cameras); for (int32 I = 0; I < int32(UE_ARRAY_COUNT(Cameras)); ++I) if (Single == Cameras[I].Name) Next = I; }
 }
 if (Next >= int32(UE_ARRAY_COUNT(Cameras))) { S.Done = true; UE_LOG(LogTemp, Display, TEXT("S47: graphics review complete")); return; }
 const FCamera& V = Cameras[Next];
 M->Story->SetPaused(true);
 FString Clock = TEXT("02:15"); FParse::Value(FCommandLine::Get(), TEXT("S47GraphicsClock="), Clock);
 TArray<FString> Parts; Clock.ParseIntoArray(Parts, TEXT(":"));
 if (Parts.Num() >= 2) M->Story->SetClock(FCString::Atod(*Parts[0]) * 3600 + FCString::Atod(*Parts[1]) * 60);
 if (Next != S.Step) {
  S.Step = Next; S.CameraTime = S.Time; S.Shot = false; S.Frames.Reset(); S.Game.Reset(); S.Draw.Reset(); S.Gpu.Reset();
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
 const double Since = S.Time - S.CameraTime;
 if (Since > Hold * 0.5 && !S.Shot) {
  S.Frames.Add(Dt * 1000); S.Game.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));
  S.Draw.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime)); S.Gpu.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));
 }
 if (Since < Hold - 0.3 || S.Shot || S.Frames.IsEmpty()) return;
 S.Shot = true;
 auto Mean = [](const TArray<float>& A) { double N = 0; for (float X : A) N += X; return N / A.Num(); };
 TArray<float> Ordered = S.Frames; Ordered.Sort();
 const float P95 = Ordered[FMath::Min(Ordered.Num() - 1, int32(Ordered.Num() * .95))];
 const FString Row = FString::Printf(TEXT("%s,%s,%d,%.4f,%.4f,%.4f,%.4f,%.4f\n"), V.Name, *Clock, S.Frames.Num(), Mean(S.Frames), P95, Mean(S.Game), Mean(S.Draw), Mean(S.Gpu));
 IFileManager::Get().MakeDirectory(*Directory, true);
 FFileHelper::SaveStringToFile(Row, *(Directory / TEXT("frames.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
 FScreenshotRequest::RequestScreenshot(Directory / (FString(V.Name) + TEXT(".png")), true, false);
 UE_LOG(LogTemp, Display, TEXT("S47: graphics sample %s"), *Row.TrimEnd());
}
}

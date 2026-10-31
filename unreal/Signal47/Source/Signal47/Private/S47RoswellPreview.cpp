#include "S47RoswellPreview.h"
#include "S47Glimpses.h"
#include "S47Flow.h"
#include "S47Flashlight.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/PoseableMeshComponent.h"
#include "Containers/Ticker.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

// Drives the ordinary Glimpses instance and its actual imported skin. There is
// no second display alien or alternate model created for this camera fixture.
struct FS47RoswellPreviewDriver
{
 double T=0, Settle=0, Deadline=0;
 int32 Shot=0;
 bool bPlaced=false, bStarted=false, bBaseline=false, bSkinRendered=false, bFullBody=false;
 bool bHadLight=false, bLightOn=false;
 FString Shots, Pending;
 FVector Eye=FVector::ZeroVector, Target=FVector::ZeroVector;

 bool Step(US47Main* M, double Dt)
 {
  if (!M || !M->Web || !M->Web->IsLoaded() || !M->Flow || !M->Story || !M->World ||
   !M->Player || !M->Night || !M->Night->Glimpses || S47Startup::Blocking() ||
   M->Flow->bStarting || M->World->bBusy || M->Mode!=US47Main::EMode::Play) return true;
  auto& G=*M->Night->Glimpses;
  const auto Restore=[&]
  {
   G.Hide(); G.bPreviewDriver=false;
   if (bPlaced && M->Night->Flashlight)
   { M->Night->Flashlight->bHave=bHadLight; M->Night->Flashlight->bOn=bLightOn; }
  };
  const auto Fail=[&](const TCHAR* Why)
  { UE_LOG(LogTemp,Error,TEXT("S47: Roswell preview FAIL: %s"),Why); Restore(); return false; };
  if (!bPlaced)
  {
   FString UserDir; FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);
   FPaths::NormalizeDirectoryName(UserDir);
   if (!M->Flow->bDevRun || M->Flow->CaseId!=0 || !UserDir.StartsWith(TEXT("/tmp/")) ||
    !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),FPaths::ConvertRelativePathToFull(UserDir)) || Shots.IsEmpty())
    return Fail(TEXT("requires isolated Case0, /tmp UserDir and S47RoswellShots"));
   if (M->World->Area!=TEXT("saro") || M->World->bDriving)
    return Fail(TEXT("start with S47Evening=parking in SARO"));
   if (!bBaseline && !G.HasNativeBody()) return Fail(TEXT("actual Roswell skeletal asset unavailable"));
   if (M->Night->Flashlight)
   { bHadLight=M->Night->Flashlight->bHave; bLightOn=M->Night->Flashlight->bOn; }
   G.bPreviewDriver=true; G.Hide();
   // The small head is seen through an actual pane, not overlaid in front of
   // the window. Find a normal floor position whose line clears the consoles.
   Target=FVector(-4.5,S47Roswell::ProbeM,-5.25);
   const FVector2D Feet[]={FVector2D(-5.6,-1.5),FVector2D(-2.8,-1.5),FVector2D(-4.5,-.3),FVector2D(-3.4,-2.)};
   bool Found=false;
   for (const auto& P:Feet)
   {
    if (!M->Player->Walkable(P.X,P.Y)) continue;
    M->Player->Place(P.X,P.Y,0);
    Eye=FVector(P.X,M->Player->FloorY+M->Player->Eye,P.Y);
    if (G.ClearLine(Eye,Target)) { Found=true; break; }
   }
   if (!Found) return Fail(TEXT("no clear player-height window view past the actual consoles"));
   const FVector D=Target-Eye;
   M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);
   M->Player->Pitch=FMath::Atan2(D.Y,FMath::Sqrt(D.X*D.X+D.Z*D.Z));
   M->Story->SetClock(22*3600);
   if (M->UI) M->UI->ClearToasts();
   IFileManager::Get().MakeDirectory(*Shots,true);
   bPlaced=true;
   UE_LOG(LogTemp,Display,TEXT("S47: Roswell preview FIXTURE camera=%s target=%s native=%d bones=%d; ordinary Glimpses body, skin/window fixture only"),
    *Eye.ToCompactString(),*Target.ToCompactString(),G.HasNativeBody()?1:0,G.Body.Names.Num());
   return true;
  }
  Settle+=FMath::Min(Dt,.1);
  if (Settle<2.) return true;
  if (!Pending.IsEmpty())
  {
   if (!IFileManager::Get().FileExists(*Pending))
   {
    if (FPlatformTime::Seconds()>Deadline) return Fail(TEXT("screenshot was not written"));
    return true;
   }
   UE_LOG(LogTemp,Display,TEXT("S47: Roswell preview wrote %s"),*Pending);
   Pending.Reset(); ++Shot;
   if (bBaseline || Shot>=5)
   {
    UE_LOG(LogTemp,Display,TEXT("S47: Roswell preview COMPLETE %d images; native=%d rendered=%d, camera fixture not a natural encounter/playthrough"),
     Shot,G.HasNativeBody()?1:0,bSkinRendered?1:0);
    Restore(); return false;
   }
   if (Shot==4)
   {
    // Finish with the whole actual body, face and planted feet outside the
    // corner, using another real floor position rather than a model showroom.
    G.Hide();
    if (!M->Player->Walkable(6.95,-8.35)) return Fail(TEXT("full-body view is outside walkable ground"));
    M->Player->Place(6.95,-8.35,0);
    Eye=FVector(6.95,M->Player->FloorY+M->Player->Eye,-8.35);
    Target=FVector(6.95,US47Night::SaroFloorAt(M->Web.Get(),6.95,-5.25)+.6,-5.25);
    const FVector D=Target-Eye;
    M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);
    M->Player->Pitch=FMath::Atan2(D.Y,FMath::Sqrt(D.X*D.X+D.Z*D.Z));
    bFullBody=true; bStarted=false; T=0; Settle=0;
    // The actual player's flashlight lights only this close full-body view.
    // Window frames above retain their ordinary night lighting.
    if (M->Night->Flashlight) { M->Night->Flashlight->bHave=true; M->Night->Flashlight->bOn=true; }
    UE_LOG(LogTemp,Display,TEXT("S47: Roswell preview full-body camera=%s target=%s"),*Eye.ToCompactString(),*Target.ToCompactString());
    return true;
   }
  }
  if (!bBaseline && !bStarted)
  {
   const auto S=M->Night->GlimpseIn(true);
   if (!G.Now(S,bFullBody?TEXT("control-corner"):TEXT("control-window-west"))) return Fail(TEXT("normal Glimpse placement rejected the view"));
   bStarted=true;
  }
  static const double At[]={.15,2.,2.9,4.2,2.};
  static const TCHAR* Names[]={TEXT("crouched"),TEXT("peek"),TEXT("retreat"),TEXT("covered"),TEXT("full-body")};
  if (!bBaseline && T<At[Shot])
  {
   const double Step=FMath::Min(FMath::Min(Dt,.05),At[Shot]-T);
   G.UpdateRoswell(Step,M->Night->GlimpseIn(true)); T+=Step;
   return true;
  }
  const FString Name=bBaseline?TEXT("legacy-window-baseline"):FString(TEXT("roswell-"))+Names[Shot];
  Pending=Shots/(Name+TEXT(".png"));
  if (IFileManager::Get().FileExists(*Pending)) return Fail(TEXT("refusing to overwrite a preview image"));
  auto* Skin=G.Body.Mesh.Get();
  bSkinRendered|=G.Body.Rendered();
  const FVector Head=Skin?Skin->GetBoneLocationByName(TEXT("head"),EBoneSpaces::WorldSpace):FVector::ZeroVector;
  const FVector LeftFoot=Skin?Skin->GetBoneLocationByName(TEXT("foot_l"),EBoneSpaces::WorldSpace):FVector::ZeroVector;
  const auto Sight=G.Sight(M->Night->GlimpseIn(true).Cam,G.BasePos,S47Roswell::HeadM);
  FString BlockedBy;
  const bool Clear=G.ClearLine(M->Night->GlimpseIn(true).Cam.Pos,G.BasePos+FVector(0,S47Roswell::ProbeM,0),&BlockedBy);
  UE_LOG(LogTemp,Display,TEXT("S47: Roswell preview sight inView=%d off=%.3f clear=%d blockedBy=%s noticed=%.3f withdraw=%.3f flashlight=%.3f"),
   Sight.bInView?1:0,Sight.Off,Clear?1:0,*BlockedBy,G.Noticed,G.Withdraw,M->Night->Flashlight?M->Night->Flashlight->Intensity():0.);
  UE_LOG(LogTemp,Display,TEXT("S47: Roswell preview shot=%s t=%.3f exposed=%.3f shown=%d rendered=%d headUE=%s leftFootUE=%s"),
   *Name,T,G.ExposureNow,G.Visible()?1:0,G.Body.Rendered()?1:0,*Head.ToCompactString(),*LeftFoot.ToCompactString());
  FScreenshotRequest::RequestScreenshot(Pending,true,false);
  Deadline=FPlatformTime::Seconds()+15.;
  return true;
 }
};

void S47RoswellPreview::Start(US47Main* M)
{
 if (!M || !FParse::Param(FCommandLine::Get(),TEXT("S47RoswellPreview"))) return;
 const auto P=MakeShared<FS47RoswellPreviewDriver>();
 FParse::Value(FCommandLine::Get(),TEXT("S47RoswellShots="),P->Shots);
 P->bBaseline=FParse::Param(FCommandLine::Get(),TEXT("S47RoswellOff"));
 const TWeakObjectPtr<US47Main> Weak=M;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,P](float Dt)
 { return Weak.IsValid() && P->Step(Weak.Get(),Dt); }));
}

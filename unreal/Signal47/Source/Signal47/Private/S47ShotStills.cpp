#include "S47ShotStills.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "S47CameraMoment.h"
#include "S47DirectorCam.h"
#include "S47EventPlaces.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47Json.h"
#include "S47Main.h"
#include "S47OpenLand.h"
#include "S47Relief.h"
#include "S47Script.h"
#include "S47ScriptPlayer.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "SS47ScriptEditor.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47ShotStills,Log,All);

// DIRECTOR-MODE-01 stills (S47ShotStills.h): pictures and a sheet of one script's shots and trigger, through the director's camera.

namespace S47ShotStills
{
double FGround::Clear() const
{
 double C=bOnLand?OverLand:1e9;
 if(Below.Blocked())C=FMath::Min(C,Below.Dist);
 return C;
}

FString FileName(int32 ShotNumber,int32 Frame)
{
 return FString::Printf(TEXT("shot%02d-"),FMath::Max(0,ShotNumber))+FString::Chr(TCHAR('a'+FMath::Clamp(Frame,0,25)))+TEXT(".png");
}

TArray<double> FrameTimes(const FS47ShotSpec& S)
{
 if(!S.bMove||!(S.Secs>0))return {0.};
 return {0.,S.Secs*.5,S.Secs};
}

double TopHeight(double HalfExtent,double FovDeg)
{
 const double Half=FMath::DegreesToRadians(FMath::Clamp(FovDeg,1.,170.))*.5;
 return FMath::Max(MinTopM,FMath::Max(0.,HalfExtent)*Margin/FMath::Tan(Half));
}

FVector EyeBack(const FVector& C,double Yaw,double Pitch,double Dist)
{
 const FVector Fwd(-FMath::Sin(Yaw)*FMath::Cos(Pitch),FMath::Sin(Pitch),-FMath::Cos(Yaw)*FMath::Cos(Pitch));
 return C-Fwd*Dist;
}

TArray<FString> Warnings(const FShotSheet& S)
{
 TArray<FString> Out;
 if(!S.bPlays)Out.Add(TEXT("the game would not play this shot: ")+S.Why);
 if(!S.bTraced)return Out;
 const auto CheckLook=[&Out](const FHit& H,double Dist,const TCHAR* Who)
 {
  if(H.Blocked()&&Dist>0&&H.Dist<BlockShare*Dist)
  {
   Out.Add(FString::Printf(TEXT("from %s the look is blocked by %s at %.1f m of %.1f m (%.0f %% of the way)"),Who,*H.What,H.Dist,Dist,100.*H.Dist/Dist));
  }
 };
 const auto CheckLow=[&Out](const FGround& G,const TCHAR* Who)
 {
  if(G.Under())Out.Add(FString::Printf(TEXT("%s is under the ground (%.2f m)"),Who,G.OverLand));
  else if(G.Clear()<LowEyeM)Out.Add(FString::Printf(TEXT("%s is only %.2f m over the ground"),Who,G.Clear()));
 };
 CheckLook(S.Look,S.LookDist,TEXT("the first eye"));
 CheckLow(S.EyeG,TEXT("the first eye"));
 if(S.bHasEye2)
 {
  CheckLook(S.Look2,S.LookDist2,TEXT("the second eye"));
  CheckLow(S.Eye2G,TEXT("the second eye"));
  if(S.Path.Blocked())Out.Add(FString::Printf(TEXT("the path from the first eye to the second passes through %s at %.1f m"),*S.Path.What,S.Path.Dist));
 }
 if(S.TargetG.Under())Out.Add(FString::Printf(TEXT("the point it looks at is under the ground (%.2f m)"),S.TargetG.OverLand));
 return Out;
}
}

namespace S47ShotStillsLocal
{
 using S47ShotStills::FHit;
 using S47ShotStills::FGround;

 FString Picture(const TArray<FString>& Files,const TArray<bool>& Written,int32 I)
 {
  if(!Files.IsValidIndex(I))return FString();
  return Files[I]+(Written.IsValidIndex(I)&&!Written[I]?TEXT(" (not written)"):TEXT(""));
 }

 FString LookSentence(const TCHAR* Who,const FHit& H,double Dist)
 {
  if(!H.Blocked())return FString::Printf(TEXT("From %s to the point it looks at: %.1f m, nothing on the line."),Who,Dist);
  const double Share=Dist>0?100.*H.Dist/Dist:0.;
  if(H.Dist>=S47ShotStills::BlockShare*Dist)return FString::Printf(TEXT("From %s to the point it looks at: %.1f m, in view (the first thing on the line is %s at %.1f m, %.0f %% of the way, at or next to the point)."),Who,Dist,*H.What,H.Dist,Share);
  return FString::Printf(TEXT("From %s to the point it looks at: %.1f m, blocked by %s at %.1f m (%.0f %% of the way)."),Who,Dist,*H.What,H.Dist,Share);
 }

 FString GroundSentence(const TCHAR* Who,const FGround& G)
 {
  FString Out=G.bOnLand?FString::Printf(TEXT("%s is %.2f m %s the land"),Who,FMath::Abs(G.OverLand),G.OverLand<0?TEXT("under"):TEXT("over")):FString::Printf(TEXT("%s is not over the land of this area"),Who);
  Out+=G.Below.Blocked()?FString::Printf(TEXT("; straight below it %s at %.2f m"),*G.Below.What,G.Below.Dist):FString(TEXT("; nothing straight below it within 500 m"));
  Out+=G.Above.Blocked()?FString::Printf(TEXT("; straight above it %s at %.1f m"),*G.Above.What,G.Above.Dist):FString(TEXT("; nothing above it within 300 m"));
  return Out+TEXT(".");
 }

 S47J::FObj HitJson(const FHit& H)
 {
  if(!H.Blocked())return S47J::FObj();
  S47J::FObj O=S47J::New();
  S47J::Set(O,TEXT("dist"),H.Dist);S47J::Set(O,TEXT("what"),H.What);
  return O;
 }
 S47J::FObj GroundJson(const FGround& G)
 {
  S47J::FObj O=S47J::New();
  S47J::Set(O,TEXT("onLand"),G.bOnLand);S47J::Set(O,TEXT("overLand"),G.OverLand);S47J::Set(O,TEXT("under"),G.Under());
  S47J::Set(O,TEXT("below"),HitJson(G.Below));S47J::Set(O,TEXT("above"),HitJson(G.Above));
  return O;
 }
 TArray<double> Xyz(const FVector& V){return {V.X,V.Y,V.Z};}
}

namespace S47ShotStills
{
FString SheetText(const FSheet& S)
{
 using namespace S47ShotStillsLocal;
 FString T;
 T+=FString::Printf(TEXT("# Shot stills: %s\n\n"),*S.Id);
 T+=FString::Printf(TEXT("- Script: %s\n- Read from: %s\n- Clock: %s\n"),*S.Name,*S.Source,*S.Clock);
 if(!S.Lint.IsEmpty())T+=FString::Printf(TEXT("- Lint: %s (the game will not play this script as it is)\n"),*S.Lint);
 T+=TEXT("- Points are scene metres (x east, y up, z south); distances in metres.\n");
 if(S.Shots.IsEmpty())T+=TEXT("\nThe script has no shots.\n");
 for(const FShotSheet& Sh:S.Shots)
 {
  T+=FString::Printf(TEXT("\n## Shot %d (action %d)\n\n"),Sh.Number,Sh.Action+1);
  T+=FString::Printf(TEXT("- Starts %.1f s into the script, held %.1f s, lens %.0f to %.0f degrees, %s.\n"),Sh.At,Sh.Secs,Sh.Fov,Sh.Fov2,
   Sh.bMoves?(Sh.bHasEye2?TEXT("moves from the first eye to the second"):TEXT("the lens changes, the eye stays")):TEXT("holds its pose"));
  if(Sh.Files.Num()==1)T+=FString::Printf(TEXT("- Picture: %s (the pose it holds).\n"),*Picture(Sh.Files,Sh.Written,0));
  else if(Sh.Files.Num()>=3)T+=FString::Printf(TEXT("- Pictures: %s (first frame), %s (middle), %s (last frame).\n"),*Picture(Sh.Files,Sh.Written,0),*Picture(Sh.Files,Sh.Written,1),*Picture(Sh.Files,Sh.Written,2));
  else T+=TEXT("- No pictures (no eye or no point to look at).\n");
  if(Sh.bTraced)
  {
   T+=FString::Printf(TEXT("- The first eye is at (%.1f, %.1f, %.1f), %.1f m from the player's eye at the start; it looks at (%.1f, %.1f, %.1f).\n"),Sh.Eye.X,Sh.Eye.Y,Sh.Eye.Z,Sh.FromPlayer,Sh.Target.X,Sh.Target.Y,Sh.Target.Z);
   T+=TEXT("- ")+LookSentence(TEXT("the first eye"),Sh.Look,Sh.LookDist)+TEXT("\n");
   T+=TEXT("- ")+GroundSentence(TEXT("The first eye"),Sh.EyeG)+TEXT("\n");
   T+=TEXT("- ")+GroundSentence(TEXT("The point it looks at"),Sh.TargetG)+TEXT("\n");
   if(Sh.bHasEye2)
   {
    T+=FString::Printf(TEXT("- The second eye is at (%.1f, %.1f, %.1f), %.1f m from the first.\n"),Sh.Eye2.X,Sh.Eye2.Y,Sh.Eye2.Z,FVector::Dist(Sh.Eye,Sh.Eye2));
    T+=TEXT("- ")+LookSentence(TEXT("the second eye"),Sh.Look2,Sh.LookDist2)+TEXT("\n");
    T+=TEXT("- ")+GroundSentence(TEXT("The second eye"),Sh.Eye2G)+TEXT("\n");
    T+=Sh.Path.Blocked()?FString::Printf(TEXT("- The path from the first eye to the second passes through %s at %.1f m.\n"),*Sh.Path.What,Sh.Path.Dist):FString(TEXT("- The path from the first eye to the second is clear.\n"));
   }
  }
  else if(!Sh.Why.IsEmpty())T+=TEXT("- No sight report: ")+Sh.Why+TEXT(".\n");
  for(const FString& Wn:Sh.Warnings)T+=TEXT("- WARNING: ")+Wn+TEXT("\n");
 }
 const FTriggerSheet& Tr=S.Trigger;
 T+=TEXT("\n## Trigger\n\n");
 if(Tr.Where.IsEmpty())T+=TEXT("- No place and no point: the trigger is anywhere.\n");
 else if(!Tr.bHas)T+=FString::Printf(TEXT("- %s: not found, no pictures.\n"),*Tr.Where);
 else
 {
  T+=FString::Printf(TEXT("- Where: %s, road-local (%.1f, %.1f), %.1f m up; scene (%.1f, %.1f, %.1f).\n"),*Tr.Where,Tr.X,Tr.Z,Tr.Up,Tr.Middle.X,Tr.Middle.Y,Tr.Middle.Z);
  T+=Tr.Shape==TEXT("box")?FString::Printf(TEXT("- A box %.1f by %.1f m"),Tr.HalfX*2,Tr.HalfZ*2):FString::Printf(TEXT("- A circle of %.1f m"),Tr.Radius);
  T+=Tr.Height>0?FString::Printf(TEXT(", %.1f m high"),Tr.Height):FString(TEXT(", any height"));
  T+=FString::Printf(TEXT(", mode %s.\n"),*Tr.Mode);
  T+=FString::Printf(TEXT("- The land at its middle is at %.2f m%s; %s.\n"),Tr.GroundY,Tr.Hill>0?*FString::Printf(TEXT(" (a hill of the range adds %.1f m)"),Tr.Hill):TEXT(""),
   Tr.bWalkable?TEXT("its middle is on land one can walk"):TEXT("its middle is NOT on land one can walk (past the rim, or a hill too steep or too high)"));
  if(Tr.OnTop.Blocked())T+=FString::Printf(TEXT("- From above, its middle is on %s, %.2f m over the land.\n"),*Tr.OnTop.What,Tr.OnTop.Dist);
  else T+=TEXT("- From above, nothing solid at its middle.\n");
  if(Tr.NearPlaceM>=0)T+=FString::Printf(TEXT("- Nearest named place: %s, %.0f m from its middle.\n"),*Tr.NearPlace,Tr.NearPlaceM);
  if(Tr.NearShotM>=0)T+=FString::Printf(TEXT("- Nearest shot eye: shot %d, %.0f m from its middle.\n"),Tr.NearShot,Tr.NearShotM);
  if(Tr.Files.Num()>=2)T+=FString::Printf(TEXT("- Pictures (the director's drawing on): %s (from %.0f m straight above), %s (from %.0f m, 35 degrees up, south of it).\n"),*Picture(Tr.Files,Tr.Written,0),Tr.TopM,*Picture(Tr.Files,Tr.Written,1),Tr.SideM);
 }
 return T;
}

FString SheetJson(const FSheet& S)
{
 using namespace S47ShotStillsLocal;
 S47J::FObj O=S47J::New();
 S47J::Set(O,TEXT("id"),S.Id);S47J::Set(O,TEXT("name"),S.Name);S47J::Set(O,TEXT("source"),S.Source);S47J::Set(O,TEXT("clock"),S.Clock);
 if(!S.Lint.IsEmpty())S47J::Set(O,TEXT("lint"),S.Lint);
 TArray<TSharedPtr<FJsonValue>> Shots;
 for(const FShotSheet& Sh:S.Shots)
 {
  S47J::FObj J=S47J::New();
  S47J::Set(J,TEXT("number"),Sh.Number);S47J::Set(J,TEXT("action"),Sh.Action);
  S47J::Set(J,TEXT("at"),Sh.At);S47J::Set(J,TEXT("secs"),Sh.Secs);S47J::Set(J,TEXT("fov"),Sh.Fov);S47J::Set(J,TEXT("fov2"),Sh.Fov2);
  S47J::Set(J,TEXT("plays"),Sh.bPlays);if(!Sh.Why.IsEmpty())S47J::Set(J,TEXT("why"),Sh.Why);
  S47J::Set(J,TEXT("moves"),Sh.bMoves);S47J::Set(J,TEXT("hasEye2"),Sh.bHasEye2);
  S47J::Set(J,TEXT("files"),Sh.Files);
  TArray<FString> Missing;for(int32 I=0;I<Sh.Files.Num();++I)if(Sh.Written.IsValidIndex(I)&&!Sh.Written[I])Missing.Add(Sh.Files[I]);
  S47J::Set(J,TEXT("notWritten"),Missing);
  S47J::Set(J,TEXT("traced"),Sh.bTraced);
  if(Sh.bTraced)
  {
   S47J::Set(J,TEXT("eye"),Xyz(Sh.Eye));S47J::Set(J,TEXT("target"),Xyz(Sh.Target));S47J::Set(J,TEXT("fromPlayer"),Sh.FromPlayer);
   S47J::Set(J,TEXT("lookDist"),Sh.LookDist);S47J::Set(J,TEXT("look"),HitJson(Sh.Look));
   S47J::Set(J,TEXT("eyeGround"),GroundJson(Sh.EyeG));S47J::Set(J,TEXT("targetGround"),GroundJson(Sh.TargetG));
   if(Sh.bHasEye2)
   {
    S47J::Set(J,TEXT("eye2"),Xyz(Sh.Eye2));S47J::Set(J,TEXT("lookDist2"),Sh.LookDist2);S47J::Set(J,TEXT("look2"),HitJson(Sh.Look2));
    S47J::Set(J,TEXT("eye2Ground"),GroundJson(Sh.Eye2G));S47J::Set(J,TEXT("path"),HitJson(Sh.Path));
   }
  }
  S47J::Set(J,TEXT("warnings"),Sh.Warnings);
  Shots.Add(MakeShared<FJsonValueObject>(J));
 }
 O->SetArrayField(TEXT("shots"),Shots);
 const FTriggerSheet& Tr=S.Trigger;
 S47J::FObj J=S47J::New();
 S47J::Set(J,TEXT("has"),Tr.bHas);S47J::Set(J,TEXT("where"),Tr.Where);
 if(Tr.bHas)
 {
  S47J::Set(J,TEXT("shape"),Tr.Shape);S47J::Set(J,TEXT("mode"),Tr.Mode);
  S47J::Set(J,TEXT("road"),TArray<double>{Tr.X,Tr.Z,Tr.Up});S47J::Set(J,TEXT("middle"),Xyz(Tr.Middle));
  S47J::Set(J,TEXT("halfX"),Tr.HalfX);S47J::Set(J,TEXT("halfZ"),Tr.HalfZ);S47J::Set(J,TEXT("radius"),Tr.Radius);S47J::Set(J,TEXT("height"),Tr.Height);S47J::Set(J,TEXT("extent"),Tr.Extent);
  S47J::Set(J,TEXT("groundY"),Tr.GroundY);S47J::Set(J,TEXT("hill"),Tr.Hill);S47J::Set(J,TEXT("walkable"),Tr.bWalkable);S47J::Set(J,TEXT("onTop"),HitJson(Tr.OnTop));
  if(Tr.NearPlaceM>=0){S47J::Set(J,TEXT("nearPlace"),Tr.NearPlace);S47J::Set(J,TEXT("nearPlaceM"),Tr.NearPlaceM);}
  if(Tr.NearShotM>=0){S47J::Set(J,TEXT("nearShot"),Tr.NearShot);S47J::Set(J,TEXT("nearShotM"),Tr.NearShotM);}
  S47J::Set(J,TEXT("topM"),Tr.TopM);S47J::Set(J,TEXT("sideM"),Tr.SideM);S47J::Set(J,TEXT("files"),Tr.Files);
  TArray<FString> Missing;for(int32 I=0;I<Tr.Files.Num();++I)if(Tr.Written.IsValidIndex(I)&&!Tr.Written[I])Missing.Add(Tr.Files[I]);
  S47J::Set(J,TEXT("notWritten"),Missing);
 }
 S47J::Set(O,TEXT("trigger"),J);
 return S47J::Write(O,true);
}

bool SelfTest(TArray<FString>& Lines)
{
 bool bAll=true;
 const auto Check=[&](bool bOk,const FString& What){bAll&=bOk;Lines.Add(FString::Printf(TEXT("%s STILLS: %s"),bOk?TEXT("PASS"):TEXT("FAIL"),*What));};
 Check(FileName(1,0)==TEXT("shot01-a.png")&&FileName(12,2)==TEXT("shot12-c.png")&&FileName(3,1)==TEXT("shot03-b.png"),TEXT("pictures are named shot<NN>-a/b/c.png by the shot's number"));
 {
  FS47ShotSpec Still;Still.Secs=6;Still.bMove=false;
  FS47ShotSpec Moving=Still;Moving.bMove=true;Moving.Secs=8;
  const TArray<double> A=FrameTimes(Still),B=FrameTimes(Moving);
  Check(A.Num()==1&&A[0]==0,TEXT("a shot that holds its pose gets one picture, at its first frame"));
  Check(B.Num()==3&&B[0]==0&&B[1]==4&&B[2]==8,TEXT("a shot that moves (or changes its lens) gets its first, middle and last frame"));
 }
 {
  const double Small=TopHeight(5,60),Big=TopHeight(100,60);
  Check(Small==MinTopM,TEXT("a small volume is seen from 40 m at least"));
  Check(FMath::IsNearlyEqual(Big*FMath::Tan(FMath::DegreesToRadians(30.)),130.,1e-6),FString::Printf(TEXT("a 100 m volume from %.0f m: 30 %% more than it in view"),Big));
  Check(TopHeight(100,30)>Big,TEXT("a narrower lens goes higher"));
 }
 {
  const FVector C(10,2,-30);
  const FVector Eye=EyeBack(C,.4,-.6,50);
  double Yaw=0,Pitch=0;S47CameraMoment::LookAt(Eye,C,Yaw,Pitch);
  Check(FMath::IsNearlyEqual(FVector::Dist(Eye,C),50.,1e-9)&&FMath::IsNearlyEqual(Yaw,.4,1e-9)&&FMath::IsNearlyEqual(Pitch,-.6,1e-9),TEXT("the eye put back along the view looks at the point with that yaw and pitch"));
  const FVector South=EyeBack(C,0,-.6,50);
  Check(South.Z>C.Z&&South.Y>C.Y,TEXT("facing north and down, the eye is south of the point and above it"));
 }
 {
  FShotSheet S;S.bTraced=true;S.LookDist=40;S.EyeG.bOnLand=true;S.EyeG.OverLand=1.8;S.TargetG.bOnLand=true;S.TargetG.OverLand=0;
  S.Look.Dist=39.5;S.Look.What=TEXT("Ground");
  Check(Warnings(S).IsEmpty(),TEXT("a clear shot, its look ending on the ground at the point, has no warnings"));
  FShotSheet B=S;B.Look.Dist=20;B.Look.What=TEXT("Barn");
  const TArray<FString> BW=Warnings(B);
  Check(BW.Num()==1&&BW[0].Contains(TEXT("blocked"))&&BW[0].Contains(TEXT("Barn")),TEXT("the look blocked half way is a warning naming what"));
  FShotSheet U=S;U.EyeG.OverLand=-.5;
  Check(Warnings(U).Num()==1&&Warnings(U)[0].Contains(TEXT("under the ground")),TEXT("an eye under the ground is a warning"));
  FShotSheet L=S;L.EyeG.Below.Dist=.2;L.EyeG.Below.What=TEXT("Rock");
  Check(Warnings(L).Num()==1&&Warnings(L)[0].Contains(TEXT("only 0.20 m")),TEXT("an eye less than 0.3 m over what is below it is a warning"));
  FShotSheet P=S;P.bHasEye2=true;P.LookDist2=40;P.Eye2G=S.EyeG;P.Path.Dist=3;P.Path.What=TEXT("Fence");
  Check(Warnings(P).Num()==1&&Warnings(P)[0].Contains(TEXT("path")),TEXT("the path between the eyes through something is a warning"));
  FShotSheet N;N.bPlays=false;N.Why=TEXT("no eye or no point");
  Check(Warnings(N).Num()==1&&Warnings(N)[0].Contains(TEXT("would not play")),TEXT("a shot the game would not play is a warning, with no sight report"));
  FShotSheet T=S;T.TargetG.OverLand=-2;
  Check(Warnings(T).Num()==1&&Warnings(T)[0].Contains(TEXT("looks at is under")),TEXT("a look-at under the ground is a warning"));
 }
 {
  FSheet Sh;Sh.Id=TEXT("test_stills");Sh.Name=TEXT("Test");Sh.Clock=TEXT("22:00");
  FShotSheet S;S.Number=1;S.bTraced=true;S.LookDist=40;S.Look.Dist=10;S.Look.What=TEXT("Barn");S.Files={FileName(1,0)};S.Written={true};
  S.Warnings=Warnings(S);
  Sh.Shots.Add(S);
  Sh.Trigger.bHas=true;Sh.Trigger.Where=TEXT("point (1.0, 2.0)");Sh.Trigger.Shape=TEXT("circle");Sh.Trigger.Mode=TEXT("enter");Sh.Trigger.Files={TEXT("trigger-top.png"),TEXT("trigger-side.png")};Sh.Trigger.Written={true,false};
  const FString Text=SheetText(Sh);
  Check(Text.Contains(TEXT("# Shot stills: test_stills"))&&Text.Contains(TEXT("## Shot 1"))&&Text.Contains(TEXT("shot01-a.png"))&&Text.Contains(TEXT("- WARNING: "))&&Text.Contains(TEXT("## Trigger"))&&Text.Contains(TEXT("trigger-side.png (not written)")),TEXT("the sheet has the script, each shot with its pictures and warnings, and the trigger"));
  const S47J::FObj J=S47J::Read(SheetJson(Sh));
  Check(J.IsValid()&&S47J::Str(J,TEXT("id"))==Sh.Id&&S47J::Arr(J,TEXT("shots")).Num()==1&&S47J::Bool(S47J::Obj(J,TEXT("trigger")),TEXT("has")),TEXT("sheet.json reads back with the same facts"));
 }
 return bAll;
}
}

// ---- the run ----

namespace S47ShotStillsLocal
{
 using S47ShotStills::FSheet;
 using S47ShotStills::FShotSheet;
 using S47ShotStills::FTriggerSheet;

 // One picture: where the camera goes, and whether the director's drawing is on.
 struct FPic { FString File; FVector Eye=FVector::ZeroVector; double Yaw=0,Pitch=0,Fov=60; bool bOverlay=false; int32 Shot=INDEX_NONE,Frame=0; };
 struct FRun
 {
  int32 Stage=0,Phase=0,PhaseFrames=0,Pic=0,Written=0;
  double T=0,At=0,PhaseAt=0,NextTry=0,WantHour=22;
  bool bDone=false,bRead=false,bFovSaved=false,bDraftSwapped=false;
  FString Dir,Arg;
  FS47Script Script;
  FSheet Sheet;
  TArray<FS47ShotSpec> Specs; TArray<bool> HasSpec; // by the sheet's shots
  TArray<FPic> Pics;
  // what is put back on leaving
  FS47Script Draft; bool bHadDraft=false,bWasDirty=false; int32 Selected=INDEX_NONE;
  float Fov0=70.f;
  FVector PlayerEye=FVector::ZeroVector;
 };
 FRun& Run(){static FRun R;return R;}

 constexpr double Overhead=1.5; // the top picture's pitch: as straight down as the director's camera goes (S47DirectorCam::PitchLimit)
 constexpr double SidePitch=-35.*UE_DOUBLE_PI/180.,OverviewFov=60;

 FString NameOf(const FHitResult& H)
 {
  const AActor* A=H.GetActor();
  const UPrimitiveComponent* C=H.GetComponent();
  FString Out=A?A->GetName():FString(TEXT("something"));
  if(C)Out+=TEXT(" / ")+C->GetName();
  if(const UStaticMeshComponent* SM=Cast<UStaticMeshComponent>(C))if(const UStaticMesh* Mesh=SM->GetStaticMesh())Out+=TEXT(" (")+Mesh->GetName()+TEXT(")");
  return Out;
 }

 // The first thing on the line from A to B (scene metres), the player's own actor left out.
 FHit Trace(UWorld* World,const FVector& A,const FVector& B,const AActor* Ignore)
 {
  FHit Out;
  if(!World||FVector::Dist(A,B)<1e-3)return Out;
  FHitResult H;
  FCollisionQueryParams Q(SCENE_QUERY_STAT(S47StillsTrace),true);
  if(Ignore)Q.AddIgnoredActor(Ignore);
  if(World->LineTraceSingleByChannel(H,S47Three::ToUE(A),S47Three::ToUE(B),ECC_Visibility,Q)){Out.Dist=FVector::Dist(A,S47Three::ToThree(H.ImpactPoint));Out.What=NameOf(H);}
  return Out;
 }

 // The land along the line, as the game's floor has it (S47FarmWorld::FloorRoad, the range's hills included: those have no collision):
 // the first sample more than 5 cm under it, every 2 m or so, the ends left out.
 FHit LandOnLine(US47Main* M,const FVector& A,const FVector& B)
 {
  FHit Out;
  const double Len=FVector::Dist(A,B);
  const int32 N=FMath::Clamp(int32(Len/2.),2,400);
  for(int32 K=1;K<N;++K)
  {
   FS47ScriptPoint P;
   if(FS47ScriptPlayer::RoadOfWorld(M,FMath::Lerp(A,B,double(K)/N),P)&&P.Up<-.05){Out.Dist=Len*K/N;Out.What=TEXT("the land (or a hill of the range)");return Out;}
  }
  return Out;
 }

 FHit Nearer(const FHit& A,const FHit& B)
 {
  if(!A.Blocked())return B;
  if(!B.Blocked())return A;
  return A.Dist<=B.Dist?A:B;
 }

 // What is on the line from A to B, from either end (a trace that starts inside something does not see it), and the land.
 FHit Line(US47Main* M,UWorld* World,const FVector& A,const FVector& B,const AActor* Ignore)
 {
  FHit Back=Trace(World,B,A,Ignore);
  if(Back.Blocked())Back.Dist=FVector::Dist(A,B)-Back.Dist;
  return Nearer(Nearer(Trace(World,A,B,Ignore),Back),LandOnLine(M,A,B));
 }

 FGround GroundAt(US47Main* M,UWorld* World,const FVector& P,const AActor* Ignore)
 {
  FGround G;
  FS47ScriptPoint R;
  G.bOnLand=FS47ScriptPlayer::RoadOfWorld(M,P,R);
  if(G.bOnLand)G.OverLand=R.Up;
  G.Below=Trace(World,P,P-FVector(0,500,0),Ignore);
  G.Above=Trace(World,P+FVector(0,300,0),P,Ignore);
  if(G.Above.Blocked())G.Above.Dist=300-G.Above.Dist;
  return G;
 }

 void Restore(US47Main* M)
 {
  FRun& R=Run();
  if(S47DirectorCam::On(M))S47DirectorCam::Leave(M);
  S47DirectorCam::SetOverlay(true);
  if(R.bFovSaved)if(AS47Walker* W=M?M->Player.Get():nullptr)W->Fov=R.Fov0;
  R.bFovSaved=false;
  if(R.bDraftSwapped)
  {
   S47ScriptEditor::Draft()=R.Draft;S47ScriptEditor::HaveDraft()=R.bHadDraft;S47ScriptEditor::Dirty()=R.bWasDirty;
   S47ScriptEditor::SelectShot(R.Selected);
   R.bDraftSwapped=false;
  }
 }

 bool Finish(US47Main* M,bool bGood,const FString& Why)
 {
  FRun& R=Run();
  R.bDone=true;
  Restore(M);
  if(bGood){UE_LOG(LogS47ShotStills,Display,TEXT("S47: shot stills COMPLETE PASS (%d pictures)"),R.Written);}
  else{UE_LOG(LogS47ShotStills,Display,TEXT("S47: shot stills FAIL: %s"),*Why);}
  FPlatformMisc::RequestExitWithStatus(false,bGood?0:1);
  return true;
 }

 bool ReadFile(const FString& Path,FS47Script& Out,FString& Why)
 {
  FString Text,Said;
  if(!FFileHelper::LoadFileToString(Text,*Path)){Why=TEXT("cannot read ")+Path;return false;}
  if(!S47Script::Read(Text,Out,Said)){Why=Path+TEXT(" does not parse: ")+Said;return false;}
  Out.File=Path;
  return true;
 }

 // The script by id from the script player's library (what the editor shows), else its file in the folders the player reads; or a
 // path to a .json.
 bool Load(FS47ScriptPlayer* P,const FString& Arg,FS47Script& Out,FString& Lint,FString& Why)
 {
  const bool bPath=Arg.EndsWith(TEXT(".json"))||Arg.Contains(TEXT("/"));
  FString Path=bPath?Arg:FString();
  if(!bPath)
  {
   if(const FS47Script* Found=P->Find(Arg))
   {
    Out=*Found;
    if(const FString* Bad=P->Refused().Find(Arg))Lint=*Bad;
    return true;
   }
   for(const FString& D:FS47ScriptPlayer::ReadDirs())if(FPaths::FileExists(D/Arg+TEXT(".json")))Path=D/Arg+TEXT(".json");
   if(Path.IsEmpty()){Why=FString::Printf(TEXT("no script %s in the library or in %s"),*Arg,*FString::Join(FS47ScriptPlayer::ReadDirs(),TEXT(", ")));return false;}
  }
  if(!ReadFile(Path,Out,Why))return false;
  const TArray<FString> Errors=S47Script::Lint(Out);
  if(Errors.Num())Lint=Errors[0];
  return true;
 }

 // The shots as the script player would start them (FS47ScriptPlayer::Fire, FS47ShotCamera::Start), and their pictures.
 void PlanShots(US47Main* M,FRun& R)
 {
  int32 Number=0;
  for(int32 I=0;I<R.Script.Actions.Num();++I)
  {
   const FS47ScriptAction& A=R.Script.Actions[I];
   if(A.Do!=ES47ScriptAct::Shot)continue;
   FShotSheet Sh;
   Sh.Number=++Number;Sh.Action=I;Sh.At=A.At;Sh.Secs=A.Secs;Sh.Fov=A.Fov;Sh.Fov2=A.Fov2>0?A.Fov2:A.Fov;Sh.bHasEye2=A.Path.Num()>1;
   FS47ShotSpec Spec;
   const bool bSpec=!A.Path.IsEmpty()&&A.Point.bSet;
   if(!bSpec){Sh.bPlays=false;Sh.Why=TEXT("no eye or no point (the script player skips it)");}
   else
   {
    Spec.EyeA=FS47ScriptPlayer::WorldOfRoad(M,A.Path[0]);Spec.EyeB=Sh.bHasEye2?FS47ScriptPlayer::WorldOfRoad(M,A.Path[1]):Spec.EyeA;Spec.Target=FS47ScriptPlayer::WorldOfRoad(M,A.Point);
    Spec.FovA=A.Fov;Spec.FovB=Sh.Fov2;
    Spec.Secs=FMath::Min(A.Secs,S47Shot::MaxSecs);Spec.In=A.In;Spec.Out=A.Out;
    Spec.bMove=Sh.bHasEye2||Spec.FovB!=Spec.FovA;
    FString Why;
    if(!S47Shot::Sane(Spec,Why)){Sh.bPlays=false;Sh.Why=Why;}
    Sh.bMoves=Spec.bMove;Sh.Eye=Spec.EyeA;Sh.Eye2=Spec.EyeB;Sh.Target=Spec.Target;
    const TArray<double> Times=S47ShotStills::FrameTimes(Spec);
    for(int32 K=0;K<Times.Num();++K)
    {
     const FS47ArrayShot::FPose Pose=S47Shot::PoseAt(Spec,Times[K]);
     FPic Pc;Pc.File=S47ShotStills::FileName(Sh.Number,K);Pc.Eye=Pose.Eye;Pc.Yaw=Pose.Yaw;Pc.Pitch=Pose.Pitch;Pc.Fov=Pose.Fov;Pc.bOverlay=false;Pc.Shot=R.Sheet.Shots.Num();Pc.Frame=K;
     R.Pics.Add(Pc);
     Sh.Files.Add(Pc.File);Sh.Written.Add(false);
    }
   }
   R.Sheet.Shots.Add(Sh);R.Specs.Add(Spec);R.HasSpec.Add(bSpec);
  }
 }

 const TCHAR* ModeName(ES47ScriptMode Mo){return Mo==ES47ScriptMode::Look?TEXT("look"):Mo==ES47ScriptMode::Leave?TEXT("leave"):TEXT("enter");}

 // The trigger's facts, and its two overview pictures.
 void PlanTrigger(US47Main* M,FRun& R,UWorld* World,const AActor* Ignore)
 {
  const FS47ScriptTrigger& Tg=R.Script.Trigger;
  FTriggerSheet& Tr=R.Sheet.Trigger;
  if(!Tg.HasWhere())return;
  const FS47EventPlaces* Places=M->Freeform?M->Freeform->EventPlaces.Get():nullptr;
  FS47ScriptPoint C;
  if(Tg.Point.bSet){C=Tg.Point;Tr.Where=FString::Printf(TEXT("point (%.1f, %.1f)"),C.X,C.Z);}
  else
  {
   const FS47EventPlace* Pl=Places?Places->Find(Tg.Place):nullptr;
   Tr.Where=TEXT("place ")+Tg.Place;
   if(!Pl)return;
   Tr.Where+=TEXT(" (")+Pl->Title+TEXT(")");
   C.X=Pl->X;C.Z=Pl->Z;C.Up=0;C.bSet=true;
  }
  Tr.bHas=true;
  Tr.X=C.X;Tr.Z=C.Z;Tr.Up=C.Up;
  Tr.Shape=Tg.Shape==ES47ScriptShape::Box?TEXT("box"):TEXT("circle");
  Tr.Mode=ModeName(Tg.Mode);
  Tr.HalfX=Tg.HalfX;Tr.HalfZ=Tg.HalfZ;Tr.Radius=Tg.Radius;Tr.Height=Tg.Height;
  Tr.Extent=Tg.Shape==ES47ScriptShape::Box?FMath::Sqrt(Tg.HalfX*Tg.HalfX+Tg.HalfZ*Tg.HalfZ):Tg.Radius;
  Tr.Middle=FS47ScriptPlayer::WorldOfRoad(M,C);
  Tr.GroundY=S47FarmWorld::FloorRoad(C.X,C.Z);
  Tr.Hill=S47Relief::HillLift(C.X,C.Z);
  Tr.bWalkable=S47OpenLand::InRim(C.X,C.Z)&&!S47Relief::HillStops(C.X,C.Z,.5);
  Tr.OnTop=Trace(World,Tr.Middle+FVector(0,300,0),Tr.Middle-FVector(0,C.Up+2,0),Ignore);
  if(Tr.OnTop.Blocked())Tr.OnTop.Dist=Tr.Middle.Y+300-Tr.OnTop.Dist-Tr.GroundY;
  if(Places)if(const FS47EventPlace* Near=Places->NearestTo(FVector2D(C.X,C.Z),1e7))
  {
   Tr.NearPlace=Near->Id;Tr.NearPlaceM=FVector2D::Distance(FVector2D(Near->X,Near->Z),FVector2D(C.X,C.Z));
  }
  for(int32 I=0;I<R.Sheet.Shots.Num();++I)if(R.HasSpec[I])
  {
   const double D=FMath::Min(FVector::Dist(R.Specs[I].EyeA,Tr.Middle),FVector::Dist(R.Specs[I].EyeB,Tr.Middle));
   if(Tr.NearShotM<0||D<Tr.NearShotM){Tr.NearShotM=D;Tr.NearShot=R.Sheet.Shots[I].Number;}
  }
  // From straight above (as near as the camera goes) and from the south, 35 degrees up, the whole volume and 30 % more in view.
  const double Tall=FMath::Max(0.,Tg.Height);
  Tr.TopM=S47ShotStills::TopHeight(Tr.Extent,OverviewFov)+Tall;
  Tr.SideM=S47ShotStills::TopHeight(FMath::Max(Tr.Extent,Tall*.5),OverviewFov);
  FPic Top;Top.File=TEXT("trigger-top.png");Top.Yaw=0;Top.Pitch=-Overhead;Top.Fov=OverviewFov;Top.bOverlay=true;
  Top.Eye=S47ShotStills::EyeBack(Tr.Middle,Top.Yaw,Top.Pitch,Tr.TopM/FMath::Sin(Overhead));
  FPic Side=Top;Side.File=TEXT("trigger-side.png");Side.Pitch=SidePitch;
  Side.Eye=S47ShotStills::EyeBack(Tr.Middle+FVector(0,Tall*.5,0),Side.Yaw,Side.Pitch,Tr.SideM);
  R.Pics.Add(Top);R.Pics.Add(Side);
  Tr.Files={Top.File,Side.File};Tr.Written={false,false};
 }

 // The sight report of one shot, from line traces only.
 void TraceShot(US47Main* M,FRun& R,int32 I,UWorld* World,const AActor* Ignore)
 {
  FShotSheet& Sh=R.Sheet.Shots[I];
  if(Sh.bTraced||!R.HasSpec[I])return;
  const FS47ShotSpec& Sp=R.Specs[I];
  Sh.LookDist=FVector::Dist(Sp.EyeA,Sp.Target);
  Sh.Look=Line(M,World,Sp.EyeA,Sp.Target,Ignore);
  Sh.EyeG=GroundAt(M,World,Sp.EyeA,Ignore);
  Sh.TargetG=GroundAt(M,World,Sp.Target,Ignore);
  if(Sh.bHasEye2)
  {
   Sh.LookDist2=FVector::Dist(Sp.EyeB,Sp.Target);
   Sh.Look2=Line(M,World,Sp.EyeB,Sp.Target,Ignore);
   Sh.Eye2G=GroundAt(M,World,Sp.EyeB,Ignore);
   Sh.Path=Line(M,World,Sp.EyeA,Sp.EyeB,Ignore);
  }
  Sh.FromPlayer=FVector::Dist(Sp.EyeA,R.PlayerEye);
  Sh.bTraced=true;
 }

 bool WriteSheet(FRun& R,FString& Why)
 {
  const bool bMd=FFileHelper::SaveStringToFile(S47ShotStills::SheetText(R.Sheet),*(R.Dir/TEXT("sheet.md")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
  const bool bJson=FFileHelper::SaveStringToFile(S47ShotStills::SheetJson(R.Sheet),*(R.Dir/TEXT("sheet.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
  if(!bMd||!bJson){Why=TEXT("cannot write sheet.md or sheet.json in ")+R.Dir;return false;}
  return true;
 }
}

namespace S47ShotStills
{
bool Wanted()
{
 static const bool bWanted=[]{FString D;return FParse::Value(FCommandLine::Get(),TEXT("S47ShotStills="),D,false)&&!D.IsEmpty();}();
 return bWanted;
}

bool Step(US47Main* M,double Dt)
{
 using namespace S47ShotStillsLocal;
 FRun& R=Run();
 if(R.bDone)return true;
 R.T+=FMath::Max(0.,Dt);
 ++R.PhaseFrames;
 AS47Walker* W=M?M->Player.Get():nullptr;
 FS47ScriptPlayer* P=FS47ScriptPlayer::Of(M);
 UWorld* World=M&&M->Web?M->Web->GetWorld():nullptr;
 if(R.Stage==0)
 {
  if(!R.bRead)
  {
   R.bRead=true;
   FParse::Value(FCommandLine::Get(),TEXT("S47ShotStills="),R.Dir,false);
   FParse::Value(FCommandLine::Get(),TEXT("S47ShotStillsScript="),R.Arg,false);
   FString Clock=TEXT("22:00");
   FParse::Value(FCommandLine::Get(),TEXT("S47ShotStillsClock="),Clock);
   R.WantHour=S47Script::ParseHour(Clock);
   if(R.Arg.IsEmpty())return Finish(M,false,TEXT("no -S47ShotStillsScript=<script id or .json path>"));
   if(R.WantHour<0||R.WantHour>=24)return Finish(M,false,TEXT("-S47ShotStillsClock= is not a time of day (HH:MM): ")+Clock);
   R.Dir=FPaths::ConvertRelativePathToFull(R.Dir);
   if(!IFileManager::Get().MakeDirectory(*R.Dir,true))return Finish(M,false,TEXT("cannot make the output folder ")+R.Dir);
   UE_LOG(LogS47ShotStills,Display,TEXT("S47: shot stills of %s at %s into %s"),*R.Arg,*Clock,*R.Dir);
  }
  // The Freeform session up and the land loaded, the player on foot; settled for a few seconds, at most two minutes for it.
  const bool bReady=W&&P&&World&&M->Freeform&&M->Freeform->Active()&&M->Mode==US47Main::EMode::Play&&M->Web->IsLoaded();
  if(!bReady||R.T<8)
  {
   if(R.T>120)return Finish(M,false,TEXT("no Freeform session with a player within two minutes (add -S47Freeform)"));
   return false;
  }
  if(!FS47ScriptPlayer::DevAllowed(M))return Finish(M,false,TEXT("not a developer's run (add -S47Director)"));
  FString Why;
  if(!Load(P,R.Arg,R.Script,R.Sheet.Lint,Why))return Finish(M,false,Why);
  R.Sheet.Id=R.Script.Id;R.Sheet.Name=R.Script.Name;R.Sheet.Source=R.Script.File.IsEmpty()?R.Arg:R.Script.File;
  const bool bShots=R.Script.Actions.ContainsByPredicate([](const FS47ScriptAction& X){return X.Do==ES47ScriptAct::Shot;});
  if(!bShots&&!R.Script.Trigger.HasWhere())return Finish(M,false,FString::Printf(TEXT("%s has neither a shot nor a trigger point or place"),*R.Script.Id));
  // The script is the editor's draft meanwhile, so that director mode draws it (its own draft is put back after).
  R.Draft=S47ScriptEditor::Draft();R.bHadDraft=S47ScriptEditor::HaveDraft();R.bWasDirty=S47ScriptEditor::Dirty();R.Selected=S47ScriptEditor::SelectedShot();
  R.bDraftSwapped=true;
  FS47Script Drawn=R.Script;
  if(!Drawn.Trigger.Point.bSet&&!Drawn.Trigger.Place.IsEmpty()&&M->Freeform->EventPlaces)
  {
   // (director mode draws a trigger by its point: a place's middle as one)
   if(const FS47EventPlace* Pl=M->Freeform->EventPlaces->Find(Drawn.Trigger.Place)){Drawn.Trigger.Point.X=Pl->X;Drawn.Trigger.Point.Z=Pl->Z;Drawn.Trigger.Point.Up=0;Drawn.Trigger.Point.bSet=true;Drawn.Trigger.Place.Reset();}
  }
  S47ScriptEditor::Draft()=Drawn;S47ScriptEditor::HaveDraft()=true;S47ScriptEditor::Dirty()=false;S47ScriptEditor::SelectShot(INDEX_NONE);
  R.Stage=1;R.At=R.T;R.NextTry=R.T;
  return false;
 }
 if(!W||!P||!World)return Finish(M,false,TEXT("the walker, the script player or the world went"));
 if(R.Stage==1)
 {
  // Director mode on (the night held, no script, no dice); tried again for a minute while something else holds the night.
  if(!S47DirectorCam::On(M))
  {
   if(R.T<R.NextTry)return false;
   FString Why;
   if(!S47DirectorCam::Toggle(M,Why)||!S47DirectorCam::On(M))
   {
    if(R.T-R.At>60)return Finish(M,false,TEXT("director mode not entered within a minute: ")+Why);
    R.NextTry=R.T+2;
    return false;
   }
  }
  R.Fov0=W->Fov;R.bFovSaved=true;
  R.PlayerEye=FVector(W->Pos.X,W->FloorY+W->Eye,W->Pos.Z);
  S47DirectorCam::SetOverlay(false);
  S47DirectorCam::SetPose(M,S47Three::ToThree(W->GetActorLocation()),W->Yaw,W->Pitch,0);
  // The clock forward to the hour asked for, as the developer tools push it (never back).
  FS47FreeformSession& F=*M->Freeform;
  const double Push=FMath::Fmod(R.WantHour*3600.-FMath::Fmod(F.Clock(),86400.)+86400.,86400.);
  if(Push>30&&!F.TestAdvance(Push)){UE_LOG(LogS47ShotStills,Display,TEXT("S47: shot stills warning: the clock was not pushed"));}
  R.Sheet.Clock=S47Script::HourText(FMath::Fmod(F.Clock(),86400.)/3600.);
  PlanShots(M,R);
  PlanTrigger(M,R,World,W);
  if(R.Pics.IsEmpty())return Finish(M,false,TEXT("nothing to take: no shot has an eye and a point, and the trigger's place is not on the list"));
  UE_LOG(LogS47ShotStills,Display,TEXT("S47: shot stills: director mode on, clock %s, %d shots, %d pictures to take"),*R.Sheet.Clock,R.Sheet.Shots.Num(),R.Pics.Num());
  R.Stage=2;R.Phase=0;R.Pic=0;
  return false;
 }
 if(R.Stage==2)
 {
  if(!S47DirectorCam::On(M))return Finish(M,false,TEXT("director mode went off (see S47: director mode ended by itself)"));
  if(R.Pic>=R.Pics.Num()){R.Stage=3;return false;}
  const FPic& Pc=R.Pics[R.Pic];
  const FString Path=R.Dir/Pc.File;
  if(R.Phase==0)
  {
   // the camera to the picture's pose; no picture asked for on this frame
   S47DirectorCam::SetOverlay(Pc.bOverlay);
   if(!S47DirectorCam::SetPose(M,Pc.Eye,Pc.Yaw,Pc.Pitch,Pc.Fov))return Finish(M,false,TEXT("the director's camera would not take a pose"));
   R.Phase=1;R.PhaseAt=R.T;R.PhaseFrames=0;
   return false;
  }
  if(R.Phase==1)
  {
   // a moment for the streaming and the exposure: longer for the first picture (the clock has just been pushed) and the overviews
   W->Fov=float(Pc.Fov);
   const double Need=R.Pic==0?4.:Pc.bOverlay?2.:1.2;
   if(R.T-R.PhaseAt<Need||R.PhaseFrames<20)return false;
   R.Phase=2;
   return false;
  }
  if(R.Phase==2)
  {
   // the picture asked for (one at a time); the camera stays where it is until it is on disk
   if(FScreenshotRequest::IsScreenshotRequested())return false;
   if(Pc.Shot!=INDEX_NONE)TraceShot(M,R,Pc.Shot,World,W);
   IFileManager::Get().Delete(*Path,false,true,true);
   FScreenshotRequest::RequestScreenshot(Path,false,false);
   R.Phase=3;R.PhaseAt=R.T;R.PhaseFrames=0;
   return false;
  }
  // Phase 3: on disk, or two seconds; the camera moves on the next frame
  const bool bThere=IFileManager::Get().FileExists(*Path);
  if(R.PhaseFrames<2||(!bThere&&R.T-R.PhaseAt<2))return false;
  if(Pc.Shot!=INDEX_NONE){if(R.Sheet.Shots[Pc.Shot].Written.IsValidIndex(Pc.Frame))R.Sheet.Shots[Pc.Shot].Written[Pc.Frame]=bThere;}
  else{const int32 K=Pc.File==TEXT("trigger-top.png")?0:1;if(R.Sheet.Trigger.Written.IsValidIndex(K))R.Sheet.Trigger.Written[K]=bThere;}
  R.Written+=bThere?1:0;
  UE_LOG(LogS47ShotStills,Display,TEXT("S47: shot stills: %s %s"),*Pc.File,bThere?TEXT("written"):TEXT("NOT written within 2 s"));
  ++R.Pic;R.Phase=0;
  return false;
 }
 // Stage 3: the warnings, the sheet, and everything put back.
 int32 Warned=0;
 for(FShotSheet& Sh:R.Sheet.Shots)
 {
  Sh.Warnings=S47ShotStills::Warnings(Sh);
  for(const FString& Wn:Sh.Warnings){++Warned;UE_LOG(LogS47ShotStills,Display,TEXT("S47: shot stills warning: shot %d: %s"),Sh.Number,*Wn);}
 }
 if(R.Sheet.Trigger.bHas&&!R.Sheet.Trigger.bWalkable){++Warned;UE_LOG(LogS47ShotStills,Display,TEXT("S47: shot stills warning: the trigger's middle is not on land one can walk"));}
 if(!R.Sheet.Trigger.Where.IsEmpty()&&!R.Sheet.Trigger.bHas){++Warned;UE_LOG(LogS47ShotStills,Display,TEXT("S47: shot stills warning: the trigger's %s is not found"),*R.Sheet.Trigger.Where);}
 FString Why;
 if(!WriteSheet(R,Why))return Finish(M,false,Why);
 UE_LOG(LogS47ShotStills,Display,TEXT("S47: shot stills: %s and sheet.json written, %d warnings"),*(R.Dir/TEXT("sheet.md")),Warned);
 if(R.Written<R.Pics.Num())return Finish(M,false,FString::Printf(TEXT("%d of %d pictures not written (see sheet.md)"),R.Pics.Num()-R.Written,R.Pics.Num()));
 return Finish(M,true,FString());
}
}

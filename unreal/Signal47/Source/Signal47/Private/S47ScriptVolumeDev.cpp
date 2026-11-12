#include "S47ScriptVolumeDev.h"
#include "S47DevEvents.h"
#include "S47FarmWorld.h"
#include "S47Main.h"
#include "S47OpenLand.h"
#include "S47Script.h"
#include "S47ScriptPlayer.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "HAL/PlatformMisc.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47ScriptVolume,Log,All);

// DIRECTOR-MODE-01 (slice 3): the trigger volumes' in-game check (S47ScriptVolumeDev.h).

namespace S47ScriptVolumeDevLocal
{
 const TCHAR* const DevGroup=TEXT("Director volume");
 const TCHAR* const DevId=TEXT("dev_director_volume");
 // A script that would only say a word: no figure, no sky, so a hint at any hour; never stored, never started here.
 FS47Script Probe(ES47ScriptShape Shape,ES47ScriptMode Mode,const FS47ScriptPoint& At)
 {
  FS47Script S;S.Id=DevId;S.Name=TEXT("Director volume check");S.bEnabled=true;
  FS47ScriptTrigger& T=S.Trigger;
  T.Point=At;T.Point.bSet=true;T.Shape=Shape;T.Mode=Mode;
  T.HalfX=4;T.HalfZ=3;T.Yaw=.4;T.Radius=6;T.Degrees=12;T.LookRange=60;
  FS47ScriptAction A;A.Do=ES47ScriptAct::Toast;A.At=0;A.Text=TEXT("Developer check.");A.Secs=3;
  S.Actions.Add(A);
  return S;
 }
}

FString S47ScriptVolumeDev::Check(US47Main* M)
{
 using namespace S47ScriptVolumeDevLocal;
 FS47ScriptPlayer* P=FS47ScriptPlayer::Of(M);
 const AS47Walker* W=M?M->Player.Get():nullptr;
 if(!P)return TEXT("no script player here (not in Freeform?)");
 if(!W||!M->Web)return TEXT("no walker or no world");
 FS47ScriptPoint Here;
 if(!P->HereRoad(Here))return TEXT("no player on the land");
 const FString SaveBefore=S47J::Write(P->Save());
 const bool bRanBefore=P->Running();
 // The real session's moment; the night's lock is left out (it is a check of the volumes, not of the night's other events).
 FS47ScriptNow Base=P->Now();
 if(!Base.bHasRoad)return TEXT("Now has no player on the land (not in Freeform?)");
 Base.bBigEvent=false;Base.BigWhy.Reset();
 const FS47ScriptMemory Fresh;
 const auto Ask=[P,&Base,&Fresh](const FS47Script& S,FString& Why){return S47Script::Ready(S,P->NowFor(S,Base),Fresh,Why);};
 FString Why;
 // (a) a turned box around where the player stands starts; one 100 m east does not.
 const FS47Script Box=Probe(ES47ScriptShape::Box,ES47ScriptMode::Enter,Here);
 if(!S47Script::Lint(Box).IsEmpty())return TEXT("(a) the box does not keep to the lint: ")+S47Script::Lint(Box)[0];
 if(!Ask(Box,Why))return TEXT("(a) a box around the player does not start: ")+Why;
 FS47ScriptPoint Away=Here;Away.X+=100;
 if(Ask(Probe(ES47ScriptShape::Box,ES47ScriptMode::Enter,Away),Why))return TEXT("(b) a box 100 m away starts");
 // (c) a look trigger 20 m along the middle of the view starts; one 20 m behind does not. The points are where the view's line is,
 // with their height over the ground there (WorldOf puts them back on the same line).
 const FVector Eye(W->Pos.X,W->FloorY+W->Eye,W->Pos.Z);
 const FVector Fwd(-FMath::Sin(W->Yaw)*FMath::Cos(W->Pitch),FMath::Sin(W->Pitch),-FMath::Cos(W->Yaw)*FMath::Cos(W->Pitch));
 const auto RoadAt=[M](const FVector& At,FS47ScriptPoint& Out)
 {
  double X=0,Z=0;
  if(!S47OpenLand::RoadOf(M->Web->Area(),At.X,At.Z,X,Z))return false;
  Out=FS47ScriptPoint();Out.X=X;Out.Z=Z;Out.Up=At.Y-S47FarmWorld::FloorRoad(X,Z);Out.bSet=true;
  return true;
 };
 FS47ScriptPoint Ahead,Behind;
 if(!RoadAt(Eye+Fwd*20.,Ahead)||!RoadAt(Eye-Fwd*20.,Behind))return TEXT("(c) the points ahead and behind are not on the land");
 if(!Ask(Probe(ES47ScriptShape::Circle,ES47ScriptMode::Look,Ahead),Why))return TEXT("(c) a look trigger straight ahead does not start: ")+Why;
 if(Ask(Probe(ES47ScriptShape::Circle,ES47ScriptMode::Look,Behind),Why))return TEXT("(c) a look trigger behind the player starts");
 // (d) a leave trigger around the player does not start while they stand in it, whether or not it has seen them inside; moved 100 m
 // out after having been in, it would (Ready is pure: the moment is moved by hand, nothing in the world).
 const FS47Script Leave=Probe(ES47ScriptShape::Box,ES47ScriptMode::Leave,Here);
 if(Ask(Leave,Why))return TEXT("(d) a leave trigger starts with the player inside, never seen in");
 FS47ScriptNow In=P->NowFor(Leave,Base);In.bWasInside=true;
 if(S47Script::Ready(Leave,In,Fresh,Why))return TEXT("(d) a leave trigger starts with the player still inside");
 FS47ScriptNow Gone=In;Gone.Road.X+=100;
 if(!S47Script::Ready(Leave,Gone,Fresh,Why))return TEXT("(d) a leave trigger does not start once the player is out: ")+Why;
 // (e) nothing played, previewed or kept.
 if(P->Running()!=bRanBefore||P->PlayedCount(DevId)!=0||S47J::Write(P->Save())!=SaveBefore)return TEXT("(e) the check played something or changed the save");
 return FString();
}

void S47ScriptVolumeDev::AddDevLine(US47Main* M)
{
 using namespace S47ScriptVolumeDevLocal;
 if(!M||!M->DevEvents||!FS47ScriptPlayer::DevAllowed(M))return;
 if(M->DevEvents->Events().ContainsByPredicate([](const FS47DevEvents::FEvent& E){return E.Group==DevGroup;}))return;
 M->DevEvents->AddFreeform(DevGroup,TEXT("Check the trigger volumes here"),TEXT("DIRECTOR-MODE-01: a box, a look and a leave around where you stand; plays nothing"),[M]
 {
  const FString Fail=Check(M);
  UE_LOG(LogS47ScriptVolume,Display,TEXT("S47: developer event director volume: %s%s"),Fail.IsEmpty()?TEXT("PASS"):TEXT("FAIL "),*Fail);
  if(M->UI)M->UI->Toast(Fail.IsEmpty()?FString(TEXT("Developer event: trigger volumes hold here.")):TEXT("Developer event: volume check failed: ")+Fail,4);
 });
}

bool S47ScriptVolumeDev::Contract(US47Main* M,double Dt)
{
 struct FRun{double T=0;bool bDone=false;};
 static FRun R;
 if(R.bDone)return true;
 // (R is static: used here, not captured)
 const auto Finish=[](bool bGood,const FString& What)
 {
  R.bDone=true;
  if(bGood){UE_LOG(LogS47ScriptVolume,Display,TEXT("S47: director volume contract COMPLETE PASS"));}
  else{UE_LOG(LogS47ScriptVolume,Display,TEXT("S47: director volume contract FAIL %s"),*What);}
  FPlatformMisc::RequestExitWithStatus(false,bGood?0:1);
  return true;
 };
 R.T+=FMath::Max(0.,Dt);
 // The night settled and the player on foot, no script playing; at most a minute for it.
 const FS47ScriptPlayer* P=FS47ScriptPlayer::Of(M);
 const AS47Walker* W=M?M->Player.Get():nullptr;
 FS47ScriptPoint Here;
 const bool bReady=P&&W&&M->Web&&(!M->World||!M->World->bDriving)&&!P->Running()&&P->HereRoad(Here);
 if(!bReady||R.T<6)
 {
  if(R.T>60)return Finish(false,!P||!W?FString(TEXT("no walker or no script player (not in Freeform?)")):FString(TEXT("at the wheel, a script playing, or no player on the land")));
  return false;
 }
 const FString Fail=Check(M);
 return Finish(Fail.IsEmpty(),Fail);
}

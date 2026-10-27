// Explicit isolated test: normal walker movement/collision via TestMove, API ray picks, no human-input claim.
#include "S47Web.h"
#include "S47MotelV2.h"
#include "S47Main.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Inter.h"
#include "S47Flow.h"
#include "S47Story.h"
#include "S47Startup.h"
#include "S47Three.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47MotelRoute,Log,All);
void AS47Web::BeginMotelV2Route()
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47MotelV2Route")))return;
 struct FPoint {double X,Z;const TCHAR* Proxy;const TCHAR* Id;};
 static const FPoint Route[]={
  {-44.8,674.1,TEXT("register"),TEXT("court:register")},
  {-44.8,673.0,nullptr,nullptr},{-46.1,673.0,TEXT("keyBoard"),TEXT("court:keyBoard")},
  {-45.4,673.0,TEXT("officePhone"),TEXT("court:officePhone")},
  // The fixed-open office leaf spans x[-44.86,-43.7], z[674.81,674.90].
  // Walk around its west tip through the existing aisle beside the counter; never disable its collider.
  {-44.8,673.0,nullptr,nullptr},{-45.5,674.1,nullptr,nullptr},
  {-45.5,675.8,TEXT("officeTv"),TEXT("court:officeTv")},
  {-45.5,674.1,nullptr,nullptr},{-44.8,674.1,nullptr,nullptr},
  {-42,674.1,nullptr,nullptr},{-42,671.1,nullptr,nullptr},
  {-42.3,671.1,nullptr,nullptr},{-42.3,670.55,nullptr,nullptr},
  {-53.2,670.55,nullptr,nullptr},{-53.2,680.6,nullptr,nullptr},{-55.5,680.6,nullptr,nullptr},
  {-55.5,667.3,nullptr,nullptr},{-55.5,680.6,nullptr,nullptr},{-53.2,680.6,nullptr,nullptr},
  {-53.2,670.55,nullptr,nullptr},{-42.3,670.55,nullptr,nullptr},
  {-42.3,671.1,nullptr,nullptr},{-42,671.1,nullptr,nullptr},{-42,674.1,nullptr,nullptr},
  // Keep the walker's radius clear of the newspaper box near the office and the vending machine opposite.
  {-42.25,674.1,nullptr,nullptr},{-42.25,705,TEXT("room6Door"),TEXT("room6Door")}
 };
 struct FState {bool Started=false,Done=false;int32 Index=0,Probe=0,Picks=0;double Since=0,Began=0,Distance=0;FVector Last=FVector::ZeroVector,ProbeStart=FVector::ZeroVector;FS47Rect Box;};
 const auto S=MakeShared<FState>();const TWeakObjectPtr<AS47Web> Self=this;FTimerHandle Timer;
 GetWorldTimerManager().SetTimer(Timer,[Self,S]
 {
  if(S->Done)return;auto* W=Self.Get();auto* M=W?US47Main::Get(W):nullptr;
  if(!M||!M->World||!M->Flow||!M->Player||!M->Inter||!M->Story||M->Flow->bStarting||S47Startup::Blocking()||M->World->bBusy||M->Mode!=US47Main::EMode::Play)return;
  auto* P=M->Player.Get();const double Now=FPlatformTime::Seconds();
  auto Fail=[&](const FString& Why){P->TestMove.Reset();P->bTestRun=false;S->Done=true;UE_LOG(LogS47MotelRoute,Error,TEXT("S47: motel V2 route FAIL point=%d probe=%d at %.3f %.3f: %s"),S->Index,S->Probe,P->Pos.X,P->Pos.Z,*Why);};
  if(!S->Started)
  {
   FString UserDir;
   if(!M->Flow->bDevRun||M->Flow->CaseId!=0||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||!FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),FPaths::ConvertRelativePathToFull(UserDir)))
   {Fail(TEXT("requires dev run, no case and explicit isolated UserDir"));return;}
   if(!W->MotelV2||M->World->bDriving){Fail(TEXT("requires installed motel V2 and on-foot start"));return;}
   TArray<FString> Lines;if(!W->MotelV2->Audit(Lines)){Fail(TEXT("installation audit"));return;}
   M->World->Enter(TEXT("saro"));P->Place(Route[0].X,Route[0].Z,0);P->Pitch=-.06;
   S->Started=true;S->Since=S->Began=Now;S->Last=P->Pos;
   UE_LOG(LogS47MotelRoute,Display,TEXT("S47: motel V2 route START: one API placement; continuous normal walker movement via TestMove; API interaction ray picks; isolated collision fixtures after route"));return;
  }
  if(Now-S->Began>240){Fail(TEXT("overall timeout"));return;}
  const double Step=FVector2D(P->Pos.X-S->Last.X,P->Pos.Z-S->Last.Z).Size();if(Step<1)S->Distance+=Step;S->Last=P->Pos;
  if(S->Index<UE_ARRAY_COUNT(Route))
  {
   const FPoint& Goal=Route[S->Index];const double Dx=Goal.X-P->Pos.X,Dz=Goal.Z-P->Pos.Z;
   const double Distance=FMath::Sqrt(Dx*Dx+Dz*Dz);
   if(Distance<.08)
   {
    P->TestMove.Reset();
    if(Goal.Proxy)
    {
     const int32 Node=W->Handle(FString(TEXT("world.motel.proxies."))+Goal.Proxy);
     if(!W->Node(Node)||!M->Inter->Get(Goal.Id)){Fail(FString(TEXT("missing interaction "))+Goal.Id);return;}
     const FVector Eye=S47Three::ToUE(P->Pos.X,P->FloorY+P->Eye,P->Pos.Z),Target=W->WorldUE(Node);
     const FString Hit=M->Inter->Pick(Eye,Target-Eye,3.6);
     if(Hit!=Goal.Id){Fail(FString::Printf(TEXT("ray pick %s returned '%s'"),Goal.Id,*Hit));return;}
     ++S->Picks;UE_LOG(LogS47MotelRoute,Display,TEXT("S47: motel V2 route interaction PASS %s via normal range/label/raycast"),Goal.Id);
    }
    UE_LOG(LogS47MotelRoute,Display,TEXT("S47: motel V2 route waypoint %d/%d PASS at %.3f %.3f floor %.3f"),S->Index+1,int32(UE_ARRAY_COUNT(Route)),P->Pos.X,P->Pos.Z,P->FloorY);
    ++S->Index;S->Since=Now;return;
   }
   if(Now-S->Since>22){Fail(TEXT("waypoint stalled; do not bypass collision"));return;}
   // Analog approach slows inside50cm so the8cm waypoint tolerance does not cut a tight corner.
   P->Yaw=FMath::Atan2(-Dx,-Dz);P->TestMove=FVector2D(0,-FMath::Clamp(Distance/.5,0.,1.));P->bTestRun=false;return;
  }
  // Independent collision fixtures use the same normal walker after one explicit placement each.
  if(S->Probe<6)
  {
   const int32 Kind=S->Probe/2;
   if(S->Probe%2==0)
   {
    bool Found=false;
    for(const auto& B:W->CollidersMutable(TEXT("saro")))
    {
     const bool Match=Kind==0?(FMath::Abs(B.MinX+64.96)<.02&&FMath::Abs(B.MaxX+64.84)<.02):Kind==1?(FMath::Abs(B.MinX+61)<.02&&FMath::Abs(B.MaxX+56.8)<.02):B.Id==TEXT("prop:motelCar1");
     if(B.bEnabled&&Match){S->Box=B;Found=true;break;}
    }
    if(!Found){Fail(TEXT("collision fixture rectangle missing"));return;}
    const double Z=Kind==0?674.9:(S->Box.MinZ+S->Box.MaxZ)*.5;
    P->Place(S->Box.MaxX+.8,Z,UE_DOUBLE_PI/2);S->ProbeStart=P->Pos;S->Last=P->Pos;S->Since=Now;P->TestMove=FVector2D(0,-1);++S->Probe;return;
   }
   if(Now-S->Since<2)return;
   P->TestMove.Reset();const double Moved=S->ProbeStart.X-P->Pos.X;
   if(Moved<.1||Moved>1.1||P->Pos.X<S->Box.MaxX+P->Radius-.05){Fail(TEXT("normal walker crossed blocker or did not approach it"));return;}
   UE_LOG(LogS47MotelRoute,Display,TEXT("S47: motel V2 route blocker PASS %s moved=%.3f edge=%.3f"),Kind==0?TEXT("pool fence"):Kind==1?TEXT("pool water boundary"):TEXT("original car"),Moved,P->Pos.X);
   ++S->Probe;return;
  }
  P->TestMove.Reset();P->bTestRun=false;TArray<FString> Lines;
  if(!W->MotelV2->Audit(Lines)||S->Picks!=5){Fail(TEXT("final installation/interactions audit"));return;}
  S->Done=true;UE_LOG(LogS47MotelRoute,Display,TEXT("S47: motel V2 route PASS waypoints=%d interactions=%d/5 blockers=3/3 walked=%.2fm elapsed=%.1fs; controller/API evidence, human input unverified"),int32(UE_ARRAY_COUNT(Route)),S->Picks,S->Distance,Now-S->Began);
 },.05f,true);
}

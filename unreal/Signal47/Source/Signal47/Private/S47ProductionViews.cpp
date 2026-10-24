// Explicit isolated camera fixtures. These do not claim walking, controller or driving proof.
#include "S47Web.h"
#include "S47SaroSite.h"
#include "S47Fleet.h"
#include "S47TownBuildings.h"
#include "S47HighwayPower.h"
#include "S47Main.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Flow.h"
#include "S47Story.h"
#include "S47Startup.h"
#include "S47UI.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"
DEFINE_LOG_CATEGORY_STATIC(LogS47ProductionViews,Log,All);
void AS47Web::BeginProductionViews()
{
 if(FParse::Param(FCommandLine::Get(),TEXT("S47ProductionCab")))
 {
  struct FCab {bool Done=false;double Since=0;FVector Start=FVector::ZeroVector;};const auto P=MakeShared<FCab>();const TWeakObjectPtr<AS47Web> Self=this;FTimerHandle Timer;
  GetWorldTimerManager().SetTimer(Timer,[Self,P]{
   if(P->Done)return;auto* M=Self.IsValid()?US47Main::Get(Self.Get()):nullptr;
   if(!M||!M->World||!M->Flow||!M->Player||M->Flow->bStarting||M->World->bBusy||!M->World->bDriving||M->Mode!=US47Main::EMode::Play)return;
   auto* T=M->World->CabTruck();auto* D=M->World->CabDrive();if(!T||!D)return;
   FString UserDir;if(!M->Flow->bDevRun||M->Flow->CaseId!=0||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||!FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),FPaths::ConvertRelativePathToFull(UserDir)))return;
   D->SetLook(-.08,-.32);const double Now=FPlatformTime::Seconds();if(!P->Since){P->Since=Now;P->Start=T->Position;return;}if(Now-P->Since<4)return;
   TArray<FString> Lines;bool Ok=T->AuditProductionArt(Lines)&&FVector::Dist(T->Position,P->Start)>.5;
   for(const auto& Line:Lines)UE_LOG(LogS47ProductionViews,Display,TEXT("S47: cab %s"),*Line);
   UE_LOG(LogS47ProductionViews,Display,TEXT("S47: production cab %s distance=%.3fm speed=%.3fm/s; normal drive controller and fitted cockpit"),Ok?TEXT("PASS"):TEXT("FAIL"),FVector::Dist(T->Position,P->Start),M->World->DrivingSpeed());P->Done=true;
  },.1f,true);
 }
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47ProductionViews")))return;
 struct FView {const TCHAR* Name;const TCHAR* Area;FVector Foot,Target;};
 static const FView Views[]={
  {TEXT("saro-east-wing"),TEXT("saro"),{18,0,24},{29.6,1.5,5.55}},
  {TEXT("saro-staff-cars"),TEXT("saro"),{21,0,18},{29,.5,9.8}},
  {TEXT("saro-utilities"),TEXT("saro"),{-15,0,27},{2,4,17}},
  {TEXT("saro-service"),TEXT("saro"),{4,0,13.5},{8.9,.9,8}},
  {TEXT("sheriff"),TEXT("diner"),{7989,0,2110},{7981,8.8,2107.3}},
  {TEXT("laundry"),TEXT("diner"),{7989,0,2130},{7981,9.5,2132}},
  {TEXT("cinema"),TEXT("diner"),{7991,0,2157},{7983,11.0,2162}},
  {TEXT("feed"),TEXT("diner"),{7992,0,2195},{7981,12.1,2202}},
  {TEXT("church"),TEXT("diner"),{8014,0,2089},{8024,9,2095.5}},
  {TEXT("motel-court"),TEXT("saro"),{-37.5,0,694},{-43,0,698}},
  {TEXT("motel-office"),TEXT("saro"),{-42,0,673},{-46.5,-.7,674.7}},
  {TEXT("room6"),TEXT("room6"),{-1.4,0,-7997.7},{1,1,-8001.4}},
  {TEXT("room6-bath"),TEXT("room6"),{-1.05,0,-8003.55},{-.3,1,-8004.95}}
 };
 struct FState {int32 Index=0;bool Placed=false,Done=false,bFloorOverride=false;double Since=0;FString Dir,Suite;TFunction<double(double,double)> PreviousFloor;};
 const auto State=MakeShared<FState>();FParse::Value(FCommandLine::Get(),TEXT("S47ProductionShots="),State->Dir);FParse::Value(FCommandLine::Get(),TEXT("S47ProductionSuite="),State->Suite);
 const TWeakObjectPtr<AS47Web> Self=this;FTimerHandle Timer;
 GetWorldTimerManager().SetTimer(Timer,[Self,State]{
  if(State->Done)return;AS47Web* W=Self.Get();auto* M=W?US47Main::Get(W):nullptr;
  if(!M||!M->Flow||!M->Story||!M->World||!M->Player||S47Startup::Blocking()||M->Flow->bStarting||M->World->bBusy||M->Mode!=US47Main::EMode::Play)return;
  auto RestoreFixtureFloor=[State,M]{if(State->bFloorOverride){M->Player->Floor=MoveTemp(State->PreviousFloor);State->bFloorOverride=false;}};
  auto Fail=[State,RestoreFixtureFloor](const TCHAR* Why){RestoreFixtureFloor();State->Done=true;UE_LOG(LogS47ProductionViews,Error,TEXT("S47: production views FAIL %s"),Why);};
  FString UserDir;if(!M->Flow->bDevRun||M->Flow->CaseId!=0||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||!FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),FPaths::ConvertRelativePathToFull(UserDir))||State->Dir.IsEmpty()){Fail(TEXT("requires isolated unsaved profile"));return;}
  while(State->Index<UE_ARRAY_COUNT(Views)*3)
  {
   const int32 I=State->Index%UE_ARRAY_COUNT(Views);const bool Saro=I<4;
   if((State->Suite==TEXT("saro")&&!Saro)||(State->Suite==TEXT("town")&&Saro)){++State->Index;continue;}break;
  }
  if(State->Index>=UE_ARRAY_COUNT(Views)*3){RestoreFixtureFloor();State->Done=true;UE_LOG(LogS47ProductionViews,Display,TEXT("S47: production views PASS; static camera fixtures, input and interactions separate"));return;}
  const int32 I=State->Index%UE_ARRAY_COUNT(Views),Light=State->Index/UE_ARRAY_COUNT(Views);const auto& V=Views[I];const double Now=FPlatformTime::Seconds();
  if(!State->Placed)
  {
   if(M->World->bDriving){Fail(TEXT("fixture starts outside vehicle"));return;}
   RestoreFixtureFloor();
   M->Story->SetCinematic(true);M->World->Enter(V.Area);
   if(FString(V.Area)==TEXT("room6"))
   {
    // A review jump skips the normal door's arrival placement. Enter clears Player.floor;
    // a trace from the previous motel floor (-2.099m) starts below Room6's authored floor0.
    // Scope that known floor to this static fixture; do not invoke chapter arrival callbacks.
    State->PreviousFloor=MoveTemp(M->Player->Floor);State->bFloorOverride=true;
    M->Player->Floor=[](double,double){return 0.;};
   }
   static const double Clock[]={-12.*3600,-5.5*3600,-2.*3600};M->Story->SetClock(Clock[Light]);
   M->Player->Place(V.Foot.X,V.Foot.Z,0);
   if(FString(V.Area)==TEXT("room6")&&FMath::Abs(M->Player->FloorY)>.0001){Fail(TEXT("Room6 fixture must stand on authored floor0"));return;}
   const FVector Eye(V.Foot.X,M->Player->FloorY+M->Player->Eye,V.Foot.Z),D=V.Target-Eye;
   M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FMath::Sqrt(D.X*D.X+D.Z*D.Z));
   State->Since=Now;State->Placed=true;return;
  }
  if(Now-State->Since<2.5)return;
  if(FString(V.Area)==TEXT("room6")&&FMath::Abs(M->Player->FloorY)>.0001){Fail(TEXT("Room6 fixture floor drift"));return;}
  TArray<FString> Lines;bool Ok=true;
  if(I<4){Ok=W->SaroSite&&W->SaroSite->Audit(Lines)&&W->FleetParked&&W->FleetParked->Audit(Lines);const auto P=S47HighwayPower::Status(W);Ok&=P.bReady&&P.SaroTotalWithin100m==3;
   if(I==3)Ok&=M->World->SaroTruck()&&M->World->SaroTruck()->AuditProductionArt(Lines); }
  else Ok=W->TownBuildings&&W->TownBuildings->Audit(Lines);
  for(const auto& Line:Lines)UE_LOG(LogS47ProductionViews,Display,TEXT("S47: production %s"),*Line);
  if(!Ok){Fail(TEXT("native installation audit"));return;}
  static const TCHAR* Lights[]={TEXT("day"),TEXT("dusk"),TEXT("night")};const FString Name=FString(Lights[Light])+TEXT("-")+V.Name,File=State->Dir/(Name+TEXT(".png"));
  IFileManager::Get().MakeDirectory(*State->Dir,true);if(IFileManager::Get().FileExists(*File)){Fail(TEXT("refusing evidence overwrite"));return;}
  if(M->UI)M->UI->ClearToasts();FScreenshotRequest::RequestScreenshot(File,true,false);
  UE_LOG(LogS47ProductionViews,Display,TEXT("S47: production view %s foot=%.3f,%.3f,%.3f eye=%.3f"),*Name,M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z,M->Player->FloorY+M->Player->Eye);
  ++State->Index;State->Placed=false;
 },.1f,true);
}

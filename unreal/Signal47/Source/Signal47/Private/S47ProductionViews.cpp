// Explicit isolated camera fixtures. These do not claim walking, controller or driving proof.
#include "S47Web.h"
#include "S47SaroSite.h"
#include "S47Fleet.h"
#include "S47TownBuildings.h"
#include "S47MotelV2.h"
#include "S47Office.h"
#include "S47HighwayPower.h"
#include "S47Main.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Flow.h"
#include "S47Story.h"
#include "S47Startup.h"
#include "S47UI.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"
DEFINE_LOG_CATEGORY_STATIC(LogS47ProductionViews,Log,All);
namespace
{
 // A screenshot request is asynchronous. File existence alone can observe a partial PNG;
 // require its signature and complete terminal IEND before changing the review scene.
 bool ProductionPngWritten(const FString& Path)
 {
  TUniquePtr<IFileHandle> File(FPlatformFileManager::Get().GetPlatformFile().OpenRead(*Path,true));
  if(!File || File->Size()<20)return false;
  static const uint8 Signature[]={137,80,78,71,13,10,26,10};
  static const uint8 End[]={0,0,0,0,73,69,78,68,174,66,96,130};
  uint8 Header[8],Tail[12];
  return File->Read(Header,sizeof(Header)) && File->Seek(File->Size()-sizeof(Tail)) && File->Read(Tail,sizeof(Tail))
   && FMemory::Memcmp(Header,Signature,sizeof(Header))==0 && FMemory::Memcmp(Tail,End,sizeof(Tail))==0;
 }
}
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
  {TEXT("room6-bath"),TEXT("room6"),{-1.05,0,-8003.55},{-.3,1,-8004.95}},
  {TEXT("field-hut"),TEXT("station01"),{-7,0,8008},{-11,1.35,8003.3}},
  {TEXT("field-interior"),TEXT("station01"),{-10.6,0,8003.7},{-11.5,1.1,8001.9}},
  {TEXT("field-transit"),TEXT("station01"),{5,0,7998},{3,1.1,7996}},
  {TEXT("field-phone"),TEXT("station01"),{-11.5,0,8003.4},{-12.85,1.6,8002.7}},
  {TEXT("archive-shelves"),TEXT("saro"),{-3.2,0,7.3},{-5.4,1.1,8.9}},
  {TEXT("archive-recorder"),TEXT("saro"),{-1.9,0,9.3},{-1.1,1.5,10.22}},
  {TEXT("motel-pool-gate"),TEXT("saro"),{-53.2,0,680.6},{-59,-2.1,674}},
  {TEXT("motel-pool-north"),TEXT("saro"),{-57.8,0,666.4},{-59,-2.1,674.5}},
  {TEXT("motel-reception-tv"),TEXT("saro"),{-45.2,0,675.8},{-47.77,-.37,676.85}},
  {TEXT("motel-arrival"),TEXT("saro"),{-30.8,0,669.7},{-45.1,-.4,683}},
  // ENV-03: the crossing's flasher, a long-lens look up from the shoulder and the whole span.
  {TEXT("signal-close"),TEXT("diner"),{8004.2,0,2006},{8000.5,10.85,2000}},
  {TEXT("signal-span"),TEXT("diner"),{8012,0,1995},{8000.5,10.6,2002}},
  {TEXT("signal-close-later"),TEXT("diner"),{8004.2,0,2006},{8000.5,10.85,2000}},
  {TEXT("motel-phone-wall"),TEXT("saro"),{-44.6,0,668.7},{-44.6,-1.2,671.25}},
  {TEXT("motel-pool-exterior"),TEXT("saro"),{-49,0,668.8},{-53.55,-1.95,665.55}},
  {TEXT("homes-h01"),TEXT("diner"),{8011,0,2133},{8024,8.8142,2133}},
  {TEXT("homes-h02"),TEXT("diner"),{8011,0,2156.5},{8024.5,9.5031,2156.5}},
  {TEXT("homes-h03"),TEXT("diner"),{8012,0,2185},{8025,10.31875,2185}},
  {TEXT("homes-h04"),TEXT("diner"),{7994,0,2241},{7981,12.76855,2241}},
  {TEXT("homes-sign-south"),TEXT("diner"),{8001.8,0,2335},{8008.6,14.7472,2315}},
  {TEXT("homes-sign-north"),TEXT("diner"),{8001.8,0,1865},{7991.4,4.3622,1895}},
  // ART-01: Dale's desk papers in Operations, from the chair side of the desk.
  {TEXT("office-desk"),TEXT("saro"),{.30,0,8.70},{.25,.80,9.50}},
  {TEXT("office-desk-left"),TEXT("saro"),{-.12,0,9.0},{-.12,.81,9.55}},
  {TEXT("office-desk-right"),TEXT("saro"),{.75,0,9.0},{.75,.81,9.60}},
  {TEXT("office-contact-close"),TEXT("saro"),{.76,0,8.98},{.76,.81,9.42}},
  // ART01: legal exterior cameras keep the complete 2:3 print inside the glass.
  {TEXT("cinema-poster-left"),TEXT("diner"),{7988.7,0,2169},{7985.163,10.497393,2169}},
  {TEXT("cinema-poster-right"),TEXT("diner"),{7988.7,0,2155},{7985.163,10.497393,2155}}
 };
 struct FState {int32 Index=0,LastLight=INDEX_NONE;bool Placed=false,Done=false,bFloorOverride=false,bExit=false,bHadCinematic=false,bPreviousCinematic=false;double Since=0,Hold=2.5,RequestedAt=0;float PrevFov=-1;FString Dir,Suite,PendingFile;TFunction<double(double,double)> PreviousFloor;FTimerHandle Timer;};
 const auto State=MakeShared<FState>();FParse::Value(FCommandLine::Get(),TEXT("S47ProductionShots="),State->Dir);FParse::Value(FCommandLine::Get(),TEXT("S47ProductionSuite="),State->Suite);
 State->bExit=FParse::Param(FCommandLine::Get(),TEXT("S47ProductionExit"));
 const TWeakObjectPtr<AS47Web> Self=this;
 GetWorldTimerManager().SetTimer(State->Timer,[Self,State]{
  if(State->Done)return;AS47Web* W=Self.Get();auto* M=W?US47Main::Get(W):nullptr;
  if(!M||!M->Flow||!M->Story||!M->World||!M->Player)return;
  auto RestoreFixtureFloor=[State,M]{if(State->bFloorOverride){M->Player->Floor=MoveTemp(State->PreviousFloor);State->bFloorOverride=false;}if(State->PrevFov>0){M->Player->Fov=State->PrevFov;State->PrevFov=-1;}};
  auto Finish=[State,W,M,RestoreFixtureFloor](bool Pass,const TCHAR* Why)
  {
   RestoreFixtureFloor();if(State->bHadCinematic)M->Story->SetCinematic(State->bPreviousCinematic);
   State->Done=true;W->GetWorldTimerManager().ClearTimer(State->Timer);
   if(Pass){UE_LOG(LogS47ProductionViews,Display,TEXT("S47: production views PASS; static camera fixtures, input and interactions separate"));}
   else {UE_LOG(LogS47ProductionViews,Error,TEXT("S47: production views FAIL %s"),Why);}
   if(State->bExit){if(Pass)FPlatformMisc::RequestExit(false);else FPlatformMisc::RequestExitWithStatus(false,1);}
  };
  auto Fail=[Finish](const TCHAR* Why){Finish(false,Why);};
  if(!State->PendingFile.IsEmpty())
  {
   if(ProductionPngWritten(State->PendingFile))
   {
    UE_LOG(LogS47ProductionViews,Display,TEXT("S47: production screenshot written %s"),*State->PendingFile);
    State->PendingFile.Empty();++State->Index;State->Placed=false;
   }
   else if(FPlatformTime::Seconds()-State->RequestedAt>15.)Fail(TEXT("requested PNG was not completely written within 15 seconds"));
   return;
  }
  if(S47Startup::Blocking()||M->Flow->bStarting||M->World->bBusy||M->Mode!=US47Main::EMode::Play)return;
  FString UserDir;if(!M->Flow->bDevRun||M->Flow->CaseId!=0||!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir)||!FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),FPaths::ConvertRelativePathToFull(UserDir))||State->Dir.IsEmpty()){Fail(TEXT("requires isolated unsaved profile"));return;}
  while(State->Index<UE_ARRAY_COUNT(Views)*3)
  {
   const int32 I=State->Index%UE_ARRAY_COUNT(Views);const bool Saro=I<4;
   const FString Name(Views[I].Name);const bool Motel=Name.StartsWith(TEXT("motel-")),Signal=Name.StartsWith(TEXT("signal-")),Homes=Name.StartsWith(TEXT("homes-")),Office=Name.StartsWith(TEXT("office-")),Cinema=Name==TEXT("cinema")||Name.StartsWith(TEXT("cinema-poster-"));
   if((State->Suite==TEXT("saro")&&!Saro)||(State->Suite==TEXT("town")&&(I<4||I>8))
    ||(State->Suite==TEXT("motel")&&!Motel)||(State->Suite==TEXT("signal")&&!Signal)||(State->Suite==TEXT("homes")&&!Homes)||(State->Suite==TEXT("documents")&&!Office)||(State->Suite==TEXT("cinema-posters")&&!Cinema)||(State->Suite==TEXT("room6")&&(I<11||I>12))
    ||(State->Suite==TEXT("field")&&(I<13||I>16))||(State->Suite==TEXT("archive")&&(I<17||I>18))
    ||(State->Suite.IsEmpty()&&I>12)){++State->Index;continue;}break;
  }
  // This branch is reached only after the final PendingFile has a complete PNG terminator.
  if(State->Index>=UE_ARRAY_COUNT(Views)*3){Finish(true,TEXT("complete"));return;}
  const int32 I=State->Index%UE_ARRAY_COUNT(Views),Light=State->Index/UE_ARRAY_COUNT(Views);const auto& V=Views[I];const double Now=FPlatformTime::Seconds();
  const bool bMotelView=FString(V.Name).StartsWith(TEXT("motel-")),bSignalView=FString(V.Name).StartsWith(TEXT("signal-"));
  if(!State->Placed)
  {
   if(M->World->bDriving){Fail(TEXT("fixture starts outside vehicle"));return;}
   RestoreFixtureFloor();
   const bool AreaChanged=M->World->Area!=V.Area,LightChanged=State->LastLight!=Light;
   if(!State->bHadCinematic){State->bPreviousCinematic=M->Story->IsCinematic();State->bHadCinematic=true;}
   M->Story->SetCinematic(true);if(AreaChanged)M->World->Enter(V.Area);
   if(FString(V.Area)==TEXT("room6"))
   {
    // A review jump skips the normal door's arrival placement. Enter clears Player.floor;
    // a trace from the previous motel floor (-2.099m) starts below Room6's authored floor0.
    // Scope that known floor to this static fixture; do not invoke chapter arrival callbacks.
    State->PreviousFloor=MoveTemp(M->Player->Floor);State->bFloorOverride=true;
    M->Player->Floor=[](double,double){return 0.;};
   }
   static const double Clock[]={-12.*3600,-5.5*3600,-2.*3600};
   if(LightChanged){M->Story->SetClock(Clock[Light]);State->LastLight=Light;}
   const FString ViewName(V.Name);
   if(ViewName.StartsWith(TEXT("signal-"))){State->PrevFov=M->Player->Fov;M->Player->Fov=ViewName==TEXT("signal-span")?55.f:22.f;}
   else if(ViewName.StartsWith(TEXT("cinema-poster-"))){State->PrevFov=M->Player->Fov;M->Player->Fov=50.f;}
   else if(ViewName.StartsWith(TEXT("office-desk-"))||ViewName==TEXT("office-contact-close")){State->PrevFov=M->Player->Fov;M->Player->Fov=ViewName==TEXT("office-contact-close")?28.f:50.f;}
   else if(ViewName.StartsWith(TEXT("homes-"))){State->PrevFov=M->Player->Fov;M->Player->Fov=ViewName.StartsWith(TEXT("homes-sign-"))?40.f:55.f;}
   M->Player->Place(V.Foot.X,V.Foot.Z,0);
   if(FString(V.Area)==TEXT("room6")&&FMath::Abs(M->Player->FloorY)>.0001){Fail(TEXT("Room6 fixture must stand on authored floor0"));return;}
   const FVector Eye(V.Foot.X,M->Player->FloorY+M->Player->Eye,V.Foot.Z),D=V.Target-Eye;
   M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FMath::Sqrt(D.X*D.X+D.Z*D.Z));
   State->Hold=AreaChanged||LightChanged?5.:2.5;State->Since=Now;State->Placed=true;return;
  }
  if(Now-State->Since<State->Hold)return;
  if(FString(V.Area)==TEXT("room6")&&FMath::Abs(M->Player->FloorY)>.0001){Fail(TEXT("Room6 fixture floor drift"));return;}
  TArray<FString> Lines;bool Ok=true;
  if(FString(V.Name).StartsWith(TEXT("office-")))Ok=M->Office&&M->Office->AuditDocumentArt(Lines);
  else if(State->Suite==TEXT("cinema-posters")){Ok=W->TownBuildings&&W->TownBuildings->Audit(Lines)&&W->AuditCinemaPosters(Lines);}
  else if(I<4){Ok=W->SaroSite&&W->SaroSite->Audit(Lines)&&W->FleetParked&&W->FleetParked->Audit(Lines);const auto P=S47HighwayPower::Status(W);Ok&=P.bReady&&P.SaroTotalWithin100m==3;
   if(I==3)Ok&=M->World->SaroTruck()&&M->World->SaroTruck()->AuditProductionArt(Lines); }
  else if(I<13||I>=19) Ok=W->TownBuildings&&W->TownBuildings->Audit(Lines);
  else { Ok=W->FieldModelComponents.Num()==29 && W->FieldModelBindings.Num()==2 && W->FieldModelRetired.Num()>0;
   for(const auto& C:W->FieldModelComponents)Ok&=C.IsValid();
   for(const auto& C:W->FieldModelRetired)Ok&=C.IsValid()&&C->bHiddenInGame;
   Lines.Add(FString::Printf(TEXT("field kit %s: %d parts, %d live bindings, %d old leaves hidden"),Ok?TEXT("PASS"):TEXT("FAIL"),W->FieldModelComponents.Num(),W->FieldModelBindings.Num(),W->FieldModelRetired.Num())); }
  if(FString(V.Name).StartsWith(TEXT("motel-"))||I==11||I==12) Ok&=W->MotelV2&&W->MotelV2->Audit(Lines);
  if(FString(V.Name).StartsWith(TEXT("homes-")))Ok&=W->AuditCalicheHomes(Lines);
  for(const auto& Line:Lines)UE_LOG(LogS47ProductionViews,Display,TEXT("S47: production %s"),*Line);
  if(!Ok){Fail(TEXT("native installation audit"));return;}
  static const TCHAR* Lights[]={TEXT("day"),TEXT("dusk"),TEXT("night")};const FString Name=FString(Lights[Light])+TEXT("-")+V.Name,File=State->Dir/(Name+TEXT(".png"));
  IFileManager::Get().MakeDirectory(*State->Dir,true);if(IFileManager::Get().FileExists(*File)){Fail(TEXT("refusing evidence overwrite"));return;}
  if(M->UI)M->UI->ClearToasts();FScreenshotRequest::RequestScreenshot(File,true,false);
  UE_LOG(LogS47ProductionViews,Display,TEXT("S47: production view %s foot=%.3f,%.3f,%.3f eye=%.3f"),*Name,M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z,M->Player->FloorY+M->Player->Eye);
  State->PendingFile=File;State->RequestedAt=FPlatformTime::Seconds();
 },.1f,true);
}

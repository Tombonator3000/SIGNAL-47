// Opt-in SC09 API contract. Chapter jump and individual stand points are declared fixtures.
// By default both transfers use ordinary World trips with the existing autopilot, no driving teleport.
#include "S47Chapter3.h"
#include "S47Chapter1.h"
#include "S47Chapter4.h"
#include "S47Flow.h"
#include "S47HUD.h"
#include "S47Image.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Saves.h"
#include "S47Startup.h"
#include "S47Station.h"
#include "S47Substation.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Base64.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

struct FS47SC09Test
{
 US47Chapter3* C = nullptr;
 bool bDone = false, bTravelFixture = false, bWasDriving = false, bAimed = false;
 int32 Stage = 0, Checks = 0, Loads = 0, Candidate = 0;
 double T = 0, Since = 3, OdoStart = 0, OutMetres = 0, BackMetres = 0;
 FString Root, PendingItem;
 FString SaveWaitCohort;
 double SaveWaitAt = -1;
 TArray<TSharedPtr<FJsonValue>> Events;

 US47Main* Main() const { return C->M; }
 void Next(int32 S, double Wait = .35) { Stage = S; Since = T + Wait; }
 void Result(bool bPass, const FString& Why)
 {
  if (bDone) return;
  bDone = true;
  auto* M = Main(); auto O = S47J::New();
  S47J::Set(O,TEXT("status"),bPass?TEXT("PASS"):TEXT("FAIL")); S47J::Set(O,TEXT("error"),Why);
  S47J::Set(O,TEXT("scenario"),TEXT("SC09")); S47J::Set(O,TEXT("checks"),Checks); S47J::Set(O,TEXT("restores"),Loads);
  S47J::Set(O,TEXT("elapsed"),T); S47J::Set(O,TEXT("stage"),Stage); S47J::Set(O,TEXT("chapter3"),C->SaveState());
  S47J::Set(O,TEXT("clock"),M->Story->Clock()); S47J::Set(O,TEXT("outboundMetres"),OutMetres); S47J::Set(O,TEXT("returnMetres"),BackMetres);
  S47J::Set(O,TEXT("travelFixture"),bTravelFixture); O->SetArrayField(TEXT("events"),Events);
  S47J::Set(O,TEXT("limits"),TArray<FString>{TEXT("Chapter3 jump is a developer prerequisite fixture, not an SC01-SC08 playthrough."),
   TEXT("Individual standing/camera positions and Slate button simulation are API fixtures, not human input or a complete on-foot route."),
   bTravelFixture?TEXT("Both travel endpoints are explicit developer fixtures; full driving is not claimed."):TEXT("Ordinary World trips use API autopilot; no human driving or real-time duration claim."),
   TEXT("Nora is written dialogue. No recording-heard, human listening, FPS, full matrix, installation or release claim.")});
  const bool Written = !Root.IsEmpty() && FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("result.json")));
  if (bPass && Written) { UE_LOG(LogTemp,Display,TEXT("S47: spine contract sc09 PASS COMPLETE: %d checks, %d restores; evidence=%s"),Checks,Loads,*Root); FPlatformMisc::RequestExit(false); }
  else { UE_LOG(LogTemp,Error,TEXT("S47: spine contract FAIL sc09 stage %d: %s; evidence=%s written=%d"),Stage,*Why,*Root,Written); FPlatformMisc::RequestExitWithStatus(false,1); }
 }
 bool Check(bool Good, const FString& What)
 {
  if (!Good) { Result(false,What); return false; }
  ++Checks; UE_LOG(LogTemp,Display,TEXT("S47: spine contract sc09 PASS: %s"),*What); return true;
 }
 void Aim(const FVector& At)
 {
  auto* P = Main()->Player.Get(); const FVector D = At - S47Three::ToThree(P->EyeUE());
  P->Yaw = FMath::Atan2(-D.X,-D.Z); P->Pitch = FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
 }
 FVector Target(const FString& Id) const
 {
  const auto* I = Main()->Inter->Get(Id);
  FS47Obj Object = I ? I->Object : FS47Obj();
  if (!Object) if (const auto* Item = Main()->Web->Item(Id)) Object = Main()->Web->ObjAt(Item->Node);
  if (!Object) return FVector(0,-1e6,0);
  const FBox B = Object.WorldBox(); return B.IsValid ? B.GetCenter() : Object.WorldPosition();
 }
 bool Stand(const FString& Id)
 {
  auto* M = Main(); const FVector At = Target(Id);
  if (At.Y < -1e5) { Result(false,TEXT("missing ordinary interactable ")+Id); return false; }
  const FS47Area Hut = C->Site()->HutBounds();
  int32 Legal = 0;
  for (double R : {1.1,1.5,1.9}) for (int32 K=0;K<24;++K)
  {
   const double X=At.X+R*FMath::Cos(K*PI/12), Z=At.Z+R*FMath::Sin(K*PI/12);
   if (!M->Player->Walkable(X,Z)) continue;
   if (C->Area()==TEXT("station01") && Hut.Contains(At.X,At.Z)!=Hut.Contains(X,Z)) continue;
   if (Legal++ != Candidate) continue;
   M->Player->Place(X,Z,0); PendingItem=Id;bAimed=false;
   UE_LOG(LogTemp,Display,TEXT("S47: sc09 standing-position API fixture %s at %.3f,%.3f"),*Id,X,Z);
   return true;
  }
  Result(false,TEXT("no legal stand point for ")+Id); return false;
 }
 bool Use()
 {
  auto* M=Main(); const FVector At=Target(PendingItem);
  const FVector Eye=M->Player->EyeUE(),Forward=M->Player->ForwardUE(),End=S47Three::ToUE(At);
  const FString Picked=M->Inter->Pick(Eye,Forward),Focused=M->Inter->Update(Eye,Forward);
  auto E=S47J::New(); S47J::Set(E,TEXT("item"),PendingItem); S47J::Set(E,TEXT("picked"),Picked);
  S47J::Set(E,TEXT("standThree"),TArray<double>{M->Player->Pos.X,M->Player->Pos.Y,M->Player->Pos.Z}); Events.Add(MakeShared<FJsonValueObject>(E));
  const auto* Interaction=M->Inter->Get(PendingItem);FS47Obj TargetObject=Interaction?Interaction->Object:FS47Obj();
  if(!TargetObject)if(const auto* Item=M->Web->Item(PendingItem))TargetObject=M->Web->ObjAt(Item->Node);
  FHitResult Hit;
  const bool HasHit=M->Web->GetWorld()->LineTraceSingleByChannel(Hit,Eye,End,ECC_Visibility);
  const bool HitTarget=HasHit&&TargetObject&&M->Web->NodeOf(Hit.GetComponent())==TargetObject.I;
  // The transit's own large query proxy is a valid target surface, not a wall
  // hiding it. Independent opaque geometry still rejects the sight line.
  const bool Obstructed=HasHit&&!HitTarget&&Hit.Distance<FVector::Dist(Eye,End)-35.;
  S47J::Set(E,TEXT("opaqueObstruction"),Obstructed);
  S47J::Set(E,TEXT("visibilityHitIsTarget"),HitTarget);S47J::Set(E,TEXT("focused"),Focused);
  S47J::Set(E,TEXT("cameraForwardDotTarget"),FVector::DotProduct(Forward,(End-Eye).GetSafeNormal()));
  S47J::Set(E,TEXT("visibilityHit"),Hit.GetComponent()?Hit.GetComponent()->GetPathName():FString());
  if(Picked!=PendingItem||Focused!=PendingItem||Obstructed||FVector::DotProduct(Forward,(End-Eye).GetSafeNormal())<.995)
  { UE_LOG(LogTemp,Display,TEXT("S47: sc09 stand candidate %d to %s picked '%s'; obstruction=%d"),Candidate,*PendingItem,*Picked,Obstructed); return false; }
  if (!Check(bool(M->Player->OnUse),TEXT("actual aimed unobstructed interaction ray: ")+PendingItem)) return false;
  M->Player->OnUse(Picked); return true;
 }
 bool UsePair(int32 Start, const TCHAR* Id, int32 Then, bool Close=true)
 {
  if (Stage==Start) { if (Stand(Id)) Next(Start+1); return true; }
  if (Stage==Start+1)
  {
   if(!bAimed){Aim(Target(PendingItem));bAimed=true;Since=T+.15;return true;}
   if (Use()) {Candidate=0;if (Close) Main()->UI->Close(); Next(Then);}
   else if (!bDone) {++Candidate;Next(Start,.1);}
   return true;
  }
  return false;
 }
 static bool HasText(const TSharedRef<SWidget>& Widget,const FString& Label)
 {
  if (Widget->GetTypeAsString()==TEXT("STextBlock") && StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString()==Label) return true;
  auto* Children=Widget->GetChildren();
  for (int32 I=0;Children && I<Children->Num();++I) if (HasText(Children->GetChildAt(I),Label)) return true;
  return false;
 }
 static void Buttons(const TSharedRef<SWidget>& Widget,const FString& Label,TArray<TSharedRef<SButton>>& Found)
 {
  if (!Widget->GetVisibility().IsVisible()) return;
  if (Widget->GetTypeAsString()==TEXT("SButton") && HasText(Widget,Label)) Found.Add(StaticCastSharedRef<SButton>(Widget));
  auto* Children=Widget->GetChildren();
  for (int32 I=0;Children && I<Children->Num();++I) Buttons(Children->GetChildAt(I),Label,Found);
 }
 bool Click(const FString& Label)
 {
  const auto Widget=Main()->Hud?Main()->Hud->ModalWidget():nullptr; TArray<TSharedRef<SButton>> Found;
  if (Widget) Buttons(Widget.ToSharedRef(),Label,Found);
  if (!Check(Found.Num()==1 && Found[0]->IsEnabled(),TEXT("actual unique Slate action: ")+Label)) return false;
#if !UE_BUILD_SHIPPING
  Found[0]->SimulateClick(); return true;
#else
  Result(false,TEXT("SC09 Slate contract requires a development build")); return false;
#endif
 }
 bool Restore(const FString& Name)
 {
  auto* M=Main();
  const FString Block=M->Flow->GameSaveBlock();
  const bool Stable=Block.IsEmpty()&&!M->UI->Modal()&&!M->Story->IsCinematic()&&!M->World->bBusy;
  if(!Stable)
  {
   // Arrival's actual 0.4 s parking hold outlives the chapter's arrival flag.
   // Respect the production save gate rather than taking a snapshot through it.
   if(SaveWaitCohort!=Name){SaveWaitCohort=Name;SaveWaitAt=T;}
   if(T-SaveWaitAt>8)Result(false,FString::Printf(TEXT("save boundary did not settle: %s; block=%s modal=%d cinematic=%d busy=%d"),*Name,*Block,M->UI->Modal(),M->Story->IsCinematic(),M->World->bBusy));
   return false;
  }
  SaveWaitCohort.Reset();SaveWaitAt=-1;
  if (!Check(Stable,TEXT("ordinary stable save boundary: ")+Name)) return false;
  const auto Snapshot=M->Flow->GameState(), Expected=C->SaveState();
  const auto Pose=S47J::Obj(Snapshot,TEXT("pose")); const double Clock=M->Story->Clock(); const FString Area=C->Area();
  FS47SaveStore Writer(Root/Name); Writer.Init(); FS47SaveCard Card; Card.Chapter=TEXT("SC09 isolated contract");Card.Clock=M->Story->ClockText(true);Card.Place=Area;
  FString Error; const auto Meta=Writer.Save(1,TEXT("manual"),0,Snapshot,Card,Error);
  if (!Meta) { Result(false,TEXT("isolated disk write: ")+Error);return false; }
  FS47SaveStore Reader(Root/Name);Reader.Init();const auto Disk=Reader.Load(Meta->Id);
  if (!Check(Disk.IsValid()&&S47J::Write(S47J::Obj(S47J::Obj(Disk,TEXT("case")),TEXT("ch3")))==S47J::Write(Expected),TEXT("fresh disk reader preserves complete SC09 state/images: ")+Name))return false;
  if (!FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(Root/(Name+TEXT(".loaded.json"))))) {Result(false,TEXT("write actual disk evidence"));return false;}
  US47Flow::FStart Start;Start.State=Disk;Start.CaseId=0;Start.bDev=true;Start.Playtime=M->Playtime;M->Flow->StartGame(Start);++Loads;
  auto E=S47J::New(); S47J::Set(E,TEXT("restore"),Name);S47J::Set(E,TEXT("expected"),Expected);S47J::Set(E,TEXT("actual"),C->SaveState());
  S47J::Set(E,TEXT("expectedClock"),Clock);S47J::Set(E,TEXT("actualClock"),M->Story->Clock());Events.Add(MakeShared<FJsonValueObject>(E));
  S47J::Set(E,TEXT("expectedArea"),Area);S47J::Set(E,TEXT("actualArea"),C->Area());S47J::Set(E,TEXT("expectedPose"),Pose);
  S47J::Set(E,TEXT("actualPose"),S47J::Obj(M->Flow->GameState(),TEXT("pose")));
  return Check(S47J::Write(C->SaveState())==S47J::Write(Expected)&&M->Story->Clock()==Clock&&C->Area()==Area
   &&M->Flow->CaseId==0&&M->Flow->bDevRun&&Pose
   &&FMath::Abs(M->Player->Pos.X-S47J::Num(Pose,TEXT("x")))<.02&&FMath::Abs(M->Player->Pos.Z-S47J::Num(Pose,TEXT("z")))<.02,
   TEXT("full Flow restore retains facts/negatives/clock/area/pose: ")+Name);
 }
 bool Photo(const TOptional<FS47Photo>& P,const TCHAR* Name,int32 Targets)
 {
  if (!Check(P.IsSet()&&P->Targets.Num()>=Targets&&P->Url.StartsWith(TEXT("data:image/jpeg;base64,")),FString(TEXT("actual field exposure metadata: "))+Name))return false;
  UTexture2D* Image=S47Image::FromDataUrl(P->Url);
  if (!Check(Image&&Image->GetSizeX()==US47FieldCamera::PRINT_W&&Image->GetSizeY()==US47FieldCamera::PRINT_H,FString(TEXT("actual decoded field photograph dimensions: "))+Name))return false;
  TArray<uint8> Bytes;
  return Check(FBase64::Decode(P->Url.Mid(P->Url.Find(TEXT(","))+1),Bytes)&&FFileHelper::SaveArrayToFile(Bytes,*(Root/(FString(Name)+TEXT(".jpg")))),TEXT("retain actual captured field frame"));
 }
 void Step(double Dt)
 {
  auto* M=Main();T+=Dt;
  if (T>1000) {Result(false,TEXT("1000 s bounded SC09 contract timeout"));return;}
  if (!S47Startup::Ready()||S47Startup::Blocking()||M->Flow->bStarting||T<Since)return;
  if (Stage==0)
  {
   FString User,Jump;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);FParse::Value(FCommandLine::Get(),TEXT("S47Jump="),Jump);
   FString Profile=FPaths::ConvertRelativePathToFull(User);Profile.RemoveFromEnd(TEXT("/"));
   const FString Saved=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
   if(!User.StartsWith(TEXT("/tmp/S47-"))||User.Contains(TEXT(".."))||!Saved.StartsWith(Profile+TEXT("/"))) {Result(false,TEXT("explicit isolated /tmp/S47- profile and actual Saved containment required"));return;}
   Root=Saved/TEXT("SpineContract/sc09")/FGuid::NewGuid().ToString();
   IFileManager::Get().MakeDirectory(*Root,true);
   if (!Check(User.StartsWith(TEXT("/tmp/S47-"))&&M->Flow->bDevRun&&M->Flow->CaseId==0&&Jump==TEXT("chapter3")&&C->bActive
    &&C->S.SurveyVersion==1&&M->Story->bEveningTimeline&&C->Site(),TEXT("isolated developer case; explicit chapter3 prerequisite fixture; new SC09 route")))return;
   bTravelFixture=FParse::Param(FCommandLine::Get(),TEXT("S47SC09TravelFixture"));
   M->Flow->bAbductOn=false; // Deterministic optional-random-event setting, never a progression fact.
   if (!Check(!C->S.bP06&&!C->S.bP09&&!C->S.F3&&!C->S.F4&&!C->bRinging&&!C->S.bCalled
    &&M->Story->Clock()>=S47Timeline::Chapter3(true),TEXT("04:25 opening has no forged field findings, frames or Nora call")))return;
   if (!Restore(TEXT("before-station")))return;
   if (bTravelFixture) {M->World->PlaceTruck(TEXT("station01"));M->World->Enter(TEXT("station01"));M->World->PlaceAtStation();Next(2,1);return;}
   OdoStart=M->World->Odometer();M->World->Autopilot=24;const auto Trip=C->TruckTrip(TEXT("saro"));
   if (!Check(Trip.IsSet(),TEXT("ordinary outbound truck choice exists")))return;Trip->Use();Next(1,0);return;
  }
  if (Stage==1)
  {
   bWasDriving|=M->World->bDriving;
   if (M->World->bDriving||M->World->bBusy||C->Area()!=TEXT("station01"))return;
   OutMetres=(M->World->Odometer()-OdoStart)*1609.344;
   if (!Check(bWasDriving&&OutMetres>10,TEXT("ordinary autopilot World trip actually arrives at STATION01 without teleport")))return;
   Next(2);return;
  }
  if (Stage==2)
  {
   if (!Check(C->S.bArrived&&C->S.Stage==TEXT("station")&&!C->bRinging,TEXT("real arrival opens field work, field phone remains quiet")))return;
   if (!Restore(TEXT("at-station")))return;Next(10);return;
  }
  if(UsePair(10,TEXT("s1footing"),12)||UsePair(12,TEXT("s1markerA"),14)||UsePair(14,TEXT("s1markerB"),16)||UsePair(16,TEXT("s1transit"),18))return;
  if(Stage==18)
  {
   if (!Check(C->S.Seen.bFooting&&C->S.Seen.bMarkerA&&C->S.Seen.bMarkerB&&C->S.bReadE09a,TEXT("real footing, moved A, fixed B and E09A read independently")))return;
   const auto EarlyMarker=C->Book->Record(TEXT("P06"));
   if (!Check(EarlyMarker&&!EarlyMarker->bOk&&!C->S.bP06,TEXT("P06 cannot be recorded without FRAME03")))return;
   const FS47Pose P=C->Site()->PhotoSpot(TEXT("marker"));
   if (!Check(P.bValid&&M->Player->Walkable(P.X,P.Z),TEXT("declared FRAME03 camera stand is walkable")))return;
   M->Player->Place(P.X,P.Z,P.Yaw);Next(19);return;
  }
  if(Stage==19){Aim(C->Site()->MarkerTarget());if(!M->FieldCamera->bRaised)C->Ch1()->ToggleCamera();Next(20,.5);return;}
  if(Stage==20)
  {
   if(!Check(C->Check03().bOk,TEXT("actual viewfinder frames empty footing and post with clear line")))return;
   C->Ch1()->Shutter();if(!Photo(C->S.F3,TEXT("frame03"),2))return;Next(21,1);return;
  }
  if(Stage==21)
  {
   C->Book->Connect(TEXT("e09a.b"),TEXT("frame03"),false);C->Book->Connect(TEXT("e09a.a"),TEXT("frame03"),false);
   const auto R=C->Book->Record(TEXT("P06"));if(!Check(R&&R->bOk&&C->S.bP06&&C->S.Wrong06>0,TEXT("wrong fixed marker rejected; actual board link records A")))return;
   if(!Restore(TEXT("after-frame03")))return;Next(22);return;
  }
  if(UsePair(22,TEXT("s1lamp"),24,false))return;
  if(Stage==24)
  {
   if(!Click(TEXT("OBSERVE NULL TEST"))||!Check(!C->S.bP07,TEXT("uncovered lamp does not qualify as null test")))return;
   if(!Click(TEXT("COVER LAMP"))||!Click(TEXT("OBSERVE NULL TEST")))return;
   if(!Check(C->S.bP07&&C->S.bLampObserved&&C->Site()->bLampCovered,TEXT("actual hood/control and local null observation recorded")))return;
   M->UI->Close();Next(26);return;
  }
  if(UsePair(26,TEXT("s1cable"),28))return;
  if(Stage==28)
  {
   if(!Check(C->S.bCableInspected&&!C->S.bP08,TEXT("physical two clean cable faces inspected; FRAME04 still required")))return;
   const FS47Pose P=C->Site()->PhotoSpot(TEXT("cable"));if(!Check(P.bValid&&M->Player->Walkable(P.X,P.Z),TEXT("FRAME04 stand is walkable")))return;
   M->Player->Place(P.X,P.Z,P.Yaw);Next(29);return;
  }
  if(Stage==29){Aim(C->Site()->CableTarget());if(!M->FieldCamera->bRaised)C->Ch1()->ToggleCamera();Next(30,.5);return;}
  if(Stage==30)
  {
   if(!Check(C->Check04().bOk,TEXT("actual cut faces in near viewfinder")))return;
   C->Ch1()->Shutter();if(!Photo(C->S.F4,TEXT("frame04"),1))return;Next(31,1);return;
  }
  if(Stage==31)
  {
   C->Book->Connect(TEXT("frame04"),TEXT("P07"),false);const auto R=C->Book->Record(TEXT("P08"));
   if(!Check(R&&R->bOk&&C->S.bP08&&!C->S.bP09&&!C->bRinging,TEXT("cable/null evidence supports deliberate cut without early Nora")))return;
   if(!Restore(TEXT("after-cable")))return;Next(32);return;
  }
  if(UsePair(32,TEXT("s1timing"),34)||UsePair(34,TEXT("s1receiver"),36))return;
  if(Stage==36)
  {
   if(!C->S.bHeard||M->Story->IsCinematic())return;
   if(!Check(C->S.bReadE10&&C->DocTiming().Page.Contains(TEXT("T. Vega")),TEXT("E10 and actual timed receiver fragment identify T.Vega; no final observation invented")))return;
   C->Book->Connect(TEXT("hut.rx"),TEXT("e10.carrier"),false);const auto R=C->Book->Record(TEXT("P09"));
   if(!Check(R&&R->bOk&&C->S.Stage==TEXT("return")&&!C->S.bCalled&&!C->bRinging,TEXT("actual P09 opens return while Nora and field-call abduction remain untriggered")))return;
   if(!Restore(TEXT("return-with-negatives")))return;
   if(bTravelFixture)
   {
    M->World->PlaceTruck(TEXT("saro"));M->World->Enter(TEXT("saro"));
    const FVector Start=M->Web->SpawnThree();
    if(!Check(M->Player->Walkable(Start.X,Start.Z),TEXT("explicit return-endpoint fixture uses a legal SARO stand")))return;
    M->Player->Place(Start.X,Start.Z,Start.Y);C->ArrivedSaro();Next(40,1);return;
   }
   OdoStart=M->World->Odometer();bWasDriving=false;M->World->Autopilot=24;const auto Trip=C->TruckTrip(TEXT("station01"));
   if(!Check(Trip.IsSet(),TEXT("ordinary return trip offered before Nora")))return;Trip->Use();Next(38,0);return;
  }
  if(Stage==38)
  {
   bWasDriving|=M->World->bDriving;
   if(M->World->bDriving||M->World->bBusy||C->Area()!=TEXT("saro"))return;
   BackMetres=(M->World->Odometer()-OdoStart)*1609.344;
   if(!Check(bWasDriving&&BackMetres>10,TEXT("ordinary World return actually reaches SARO with both negatives")))return;Next(40);return;
  }
  if(Stage==40)
  {
   if(!Check(C->S.Stage==TEXT("develop")&&C->S.Dev==0&&!C->bRinging&&!C->S.bCalled,TEXT("04:40 return waits for both field prints")))return;
   M->Web->SetDoorOpen(TEXT("lab"),false,true); // declared access fixture; paper/fog actions below remain production controls.
   if(!Restore(TEXT("before-developing")))return;Next(42);return;
  }
  if(UsePair(42,TEXT("wetBench"),44,false))return;
  if(Stage==44){if(!Click(TEXT("LOAD SEALED FILM INTO TANK")))return;Next(45,.2);return;}
  if(Stage==45)
  {
   if(!Check(!C->SaveBlock().IsEmpty(),TEXT("transient development cannot be saved")))return;
   M->Web->SetDoorOpen(TEXT("lab"),true,true);Next(46,.4);return;
  }
  if(Stage==46)
  {
   if(!Check(C->S.Dev==0&&C->S.Fogged==1&&C->Busy.IsEmpty()&&!C->bRinging&&C->S.F3&&C->S.F4,TEXT("opening actual lab door fogs only paper, retaining both negatives")))return;
   M->Web->SetDoorOpen(TEXT("lab"),false,true);if(!Restore(TEXT("fogged-paper-negatives-retained")))return;Next(48);return;
  }
  if(UsePair(48,TEXT("wetBench"),50,false))return;
  if(Stage==50){if(!Click(TEXT("LOAD SEALED FILM INTO TANK")))return;Next(51,2.6);return;}
  if(Stage==51){if(!Check(C->S.Dev==1&&!C->bRinging,TEXT("fresh contact prints from original negatives, no Nora")))return;if(!Restore(TEXT("contact-not-fixed")))return;Next(52);return;}
  if(UsePair(52,TEXT("wetBench"),54,false))return;
  if(Stage==54){if(!Click(TEXT("TRANSFER CONTACT PRINT TO FIXER")))return;Next(55,1.9);return;}
  if(Stage==55){if(!Check(C->S.Dev==2&&!C->bRinging&&!C->S.bCalled,TEXT("both fixed prints still must be collected before Nora")))return;if(!Restore(TEXT("fixed-not-collected")))return;Next(56);return;}
  if(UsePair(56,TEXT("wetBench"),58,false))return;
  if(Stage==58)
  {
   if(!Click(TEXT("COLLECT AND EXAMINE PRINT"))||!Check(C->S.Dev==3&&C->FieldCopiesSecured()&&!C->bRinging&&!C->S.bCalled,TEXT("both actual frame copies secured; no early phone before closing prints")))return;
   M->UI->Close();Next(59);return;
  }
  if(Stage==59)
  {
   if(!Check(C->S.Stage==TEXT("nora")&&!C->S.bCalled,TEXT("closing secured field prints queues Nora, no SC10 credit")))return;
   if(!Restore(TEXT("both-copies-before-nora")))return;Next(60,3);return;
  }
  if(Stage==60){if(!Check(C->bRinging&&M->Story->Label(TEXT("phone"))==TEXT("Answer Nora Vega"),TEXT("only now the ordinary SARO phone rings for Nora")))return;Next(62);return;}
  if(UsePair(62,TEXT("phone"),64,false))return;
  if(Stage==64)
  {
   const auto Widget=M->Hud->ModalWidget();
   if(!Check(Widget&&M->UI->Modal()&&!C->S.bCalled&&HasText(Widget.ToSharedRef(),TEXT("Nora Vega / SARO telephone")),TEXT("normal phone ray opens written Nora conversation; not yet acknowledged")))return;
   M->UI->Close();
   if(!Check(C->S.bCalled&&C->S.Stage==TEXT("complete")&&C->S.NoraAt&&M->Story->Clock()>=4*3600+45*60,TEXT("closing actual Nora conversation completes SC09 at 04:45 or later")))return;
   if(!Restore(TEXT("nora-complete-before-sc10")))return;Next(66,2.5);return;
  }
  if(Stage==66)
  {
   if(!M->Story->Begun.Contains(TEXT("ch4"))||M->Story->IsCinematic())return;
   if(!Check(M->Story->Clock()>=S47Timeline::Chapter4(true)&&C->S.bCalled&&C->S.Dev==3,TEXT("ordinary chapter card enters SC10 at 04:50 with both original field images")))return;
   if(!Check(!M->Substation||!M->Substation->FollowupVerifiedProof(),TEXT("optional T02 verification was not required for SC09/SC10 progression")))return;
   if(!Restore(TEXT("sc10-boundary")))return;Result(true,FString());return;
  }
 }
};

void US47Chapter3::ControlSC09Test(double Dt)
{
 if(!SC09Test) {SC09Test=MakeShared<FS47SC09Test>();SC09Test->C=this;}
 const auto KeepAlive=SC09Test;
 if(KeepAlive->bDone||!M||!M->Flow||!M->Story||!M->World||!M->Player||!M->UI||!M->Inter||!M->Web||!M->FieldCamera)return;
 KeepAlive->Step(Dt);
}

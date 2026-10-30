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
#include "S47Spine.h"
#include "S47ThreatNavigation.h"
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
#include "Misc/SecureHash.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
 S47J::FObj SC09File(const FString& Path)
 {
  TArray<uint8> Bytes; if(!FFileHelper::LoadFileToArray(Bytes,*Path))return nullptr;
  FSHAHash Hash; FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash.Hash); auto O=S47J::New();
  S47J::Set(O,TEXT("path"),Path);S47J::Set(O,TEXT("bytes"),Bytes.Num());S47J::Set(O,TEXT("sha1"),Hash.ToString());return O;
 }
 bool SC09Same(const S47J::FObj& A,const S47J::FObj& B)
 {return A.IsValid()==B.IsValid()&&(!A||S47J::Write(A)==S47J::Write(B));}
 bool SC09Copy(const FString& From,const FString& To)
 {TArray<uint8> Bytes;return FFileHelper::LoadFileToArray(Bytes,*From)&&FFileHelper::SaveArrayToFile(Bytes,*To);}
}

struct FS47SC09Test
{
 US47Chapter3* C = nullptr;
 bool bDone = false, bTravelFixture = false, bWasDriving = false, bAimed = false, bActual = false;
 int32 Stage = 0, Checks = 0, Loads = 0, Candidate = 0, WalkCursor = 0, WalkSamples = 0, Uses = 0, TitleContinues = 0;
 double T = 0, Since = 3, OdoStart = 0, OutMetres = 0, BackMetres = 0;
 FString Root, PendingItem, ChainId;
 S47J::FObj Prerequisite, PriorState;
 TMap<FString,FString> PriorDocs;
 TArray<FVector2D> Route;
 FVector LastWalk = FVector::ZeroVector;
 double Walked = 0, WalkMark = 0;
 FString SaveWaitCohort;
 double SaveWaitAt = -1, ReturnWaitAt = -1;
 TArray<TSharedPtr<FJsonValue>> Events;

 US47Main* Main() const { return C->M; }
 void Next(int32 S, double Wait = .35) { Stage = S; Since = T + Wait; }
 void Result(bool bPass, const FString& Why)
 {
  if (bDone) return;
  bDone = true;
  if(Main()&&Main()->Player){Main()->Player->TestMove.Reset();Main()->Player->bTestRun=false;}
  auto* M = Main(); auto O = S47J::New();
  S47J::Set(O,TEXT("status"),bPass?TEXT("PASS"):TEXT("FAIL")); S47J::Set(O,TEXT("error"),Why);
  S47J::Set(O,TEXT("scenario"),TEXT("SC09")); S47J::Set(O,TEXT("checks"),Checks); S47J::Set(O,TEXT("restores"),Loads);
  S47J::Set(O,TEXT("elapsed"),T); S47J::Set(O,TEXT("stage"),Stage); S47J::Set(O,TEXT("chapter3"),C->SaveState());
  S47J::Set(O,TEXT("clock"),M->Story->Clock()); S47J::Set(O,TEXT("outboundMetres"),OutMetres); S47J::Set(O,TEXT("returnMetres"),BackMetres);
  S47J::Set(O,TEXT("travelFixture"),bTravelFixture); O->SetArrayField(TEXT("events"),Events);
  S47J::Set(O,TEXT("mode"),bActual?TEXT("actual-checkpoint"):TEXT("named-chapter3-fixture"));
  S47J::Set(O,TEXT("source_method"),bActual?TEXT("Actual hash-bound SC08 PASS/full disk; ordinary isolated case1 manual/fresh-reader/title/Continue. Existing visibility-graph planner with native Walker/Resolve, actual camera Pick/focus/HUD OnUse, real field frames/null test/darkroom retry/Nora and normal World/autopilot trips."):TEXT("Named chapter3 prerequisite/standing fixtures; ordinary World trips unless S47SC09TravelFixture is explicitly selected."));
  S47J::Set(O,TEXT("chain_id"),ChainId);S47J::Set(O,TEXT("prerequisite"),Prerequisite);
  S47J::Set(O,TEXT("checkpoint_file"),Root.IsEmpty()?nullptr:SC09File(Root/TEXT("sc09-complete.disk.json")));
  S47J::Set(O,TEXT("final_state"),M->Flow->GameState());S47J::Set(O,TEXT("normal_title_continues"),TitleContinues);
  S47J::Set(O,TEXT("native_uses"),Uses);S47J::Set(O,TEXT("walked_metres"),Walked);S47J::Set(O,TEXT("walk_samples"),WalkSamples);
  S47J::Set(O,TEXT("limits"),TArray<FString>{bActual?TEXT("SC01-SC08 are consumed from the exact named SC08 PASS/checkpoint bytes, not replayed or fabricated here."):TEXT("Chapter3 jump is a developer prerequisite fixture, not an SC01-SC08 playthrough."),
   bActual?TEXT("Native Walker/Resolve approaches and actual camera rays; offered Slate actions and API autopilot are not human input."):TEXT("Individual standing/camera positions and Slate button simulation are API fixtures, not human input or a complete on-foot route."),
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
 TMap<FString,FString> Documents() const
 {
  TMap<FString,FString> Out;
  for(const auto& V:S47J::Arr(Main()->Story->SaveRoutine(),TEXT("docs")))if(V&&V->Type==EJson::Object)
  {const auto D=V->AsObject();Out.Add(S47J::Str(D,TEXT("id")),S47J::Write(D));}return Out;
 }
 bool Papers(const TMap<FString,FString>& ExpectedDocs,const FString& Context)
 {
  const auto Actual=Documents();
  for(const auto& D:ExpectedDocs)if(!Check(Actual.Contains(D.Key)&&Actual[D.Key]==D.Value,Context+TEXT(" complete document/image/stamp/sheets: ")+D.Key))return false;
  return true;
 }
 bool Segment(const FVector2D& A,const FVector2D& B) const
 {
  const auto* Player=Main()->Player.Get();const double Radius=Player->Radius;const FVector2D Delta=B-A;
  for(const auto& Box:Main()->Web->Colliders())
  {
   if(!Box.bEnabled||(Box.Id==TEXT("truck")&&Player->TruckBox&&Player->TruckBox->Area==Main()->World->Area)
    ||(Box.Id==TEXT("personalCar")&&Player->PersonalCarBox&&Player->PersonalCarBox->Area==Main()->World->Area))continue;
   double Lo=0,Hi=1;auto Clip=[&](double Pos,double D,double Low,double High){if(FMath::Abs(D)<1e-12)return Pos<Low||Pos>High;double U=(Low-Pos)/D,V=(High-Pos)/D;if(U>V)Swap(U,V);Lo=FMath::Max(Lo,U);Hi=FMath::Min(Hi,V);return Lo>Hi;};
   if(!Clip(A.X,Delta.X,Box.MinX-Radius,Box.MaxX+Radius)&&!Clip(A.Y,Delta.Y,Box.MinZ-Radius,Box.MaxZ+Radius))return false;
  }
  auto InCar=[&](const FVector2D& P){if(!Player->PersonalCarBox||Player->PersonalCarBox->Area!=Main()->World->Area)return false;const auto& Box=*Player->PersonalCarBox;
   const double Cs=FMath::Cos(Box.Heading),Sn=FMath::Sin(Box.Heading),X=P.X-Box.X,Z=P.Y-Box.Z;
   const double Ox=FMath::Max(0.,FMath::Abs(X*Cs-Z*Sn)-Box.HalfW),Oz=FMath::Max(0.,FMath::Abs(X*Sn+Z*Cs)-Box.HalfL);return Ox*Ox+Oz*Oz<Radius*Radius;};
  const int32 Count=FMath::Max(1,FMath::CeilToInt(FVector2D::Distance(A,B)/.04));
  for(int32 I=0;I<=Count;++I){const auto P=FMath::Lerp(A,B,double(I)/Count);if(!Player->Walkable(P.X,P.Y)||(Player->TruckBox&&Player->TruckBox->Area==Main()->World->Area&&Player->InTruck(P.X,P.Y))||InCar(P))return false;}return true;
 }
 TArray<FVector2D> Plan(const FVector2D& Goal) const
 {
  auto* M=Main();TArray<FS47Rect> Blocks=M->Web->Colliders();
  Blocks.RemoveAll([M](const FS47Rect& B){return (B.Id==TEXT("truck")&&M->Player->TruckBox&&M->Player->TruckBox->Area==M->World->Area)
   ||(B.Id==TEXT("personalCar")&&M->Player->PersonalCarBox&&M->Player->PersonalCarBox->Area==M->World->Area);});
  const TArray<FVector2D> Seeds=M->World->Area==TEXT("station01")
   ?TArray<FVector2D>{{-22.3,8027.1},{-22.3,8025.2},{-22.3,8023.5},{-10,8005.2},{-10,8003.7},{-10.8,8003.2}}
   :TArray<FVector2D>{{9.95,3.7},{9.95,1.6},{7.55,1.6},{6.45,1.6},{5.5,1.6},{9.7,-.1},{12.1,-.1},{12.1,-1.6},{12.1,-3.8},{14,-3.8},{14,-5.5}};
  return FS47ThreatNavigation::FindPath({M->Player->Pos.X,M->Player->Pos.Z},Goal,Seeds,Blocks,M->Player->Radius,
   [this](const FVector2D& A,const FVector2D& B){return Segment(A,B);});
 }
 void Move(TArray<FVector2D> Points)
 {
  Route=MoveTemp(Points);WalkCursor=0;LastWalk=Main()->Player->Pos;WalkMark=T;
  auto E=S47J::New();S47J::Set(E,TEXT("native_route_to"),PendingItem);TArray<TSharedPtr<FJsonValue>> PointsJson;
  for(const auto& P:Route)PointsJson.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(P.X),MakeShared<FJsonValueNumber>(P.Y)}));
  E->SetArrayField(TEXT("route"),PointsJson);Events.Add(MakeShared<FJsonValueObject>(E));
 }
 bool Walk()
 {
  auto* P=Main()->Player.Get();const double Distance=FVector2D(P->Pos.X-LastWalk.X,P->Pos.Z-LastWalk.Z).Size();
  if(Distance>.65||!P->Walkable(P->Pos.X,P->Pos.Z)){Result(false,TEXT("native Walker discontinuity/non-walkable point; no placement recovery"));return false;}
  Walked+=Distance;++WalkSamples;LastWalk=P->Pos;if(Distance>.002)WalkMark=T;
  if(!Route.IsValidIndex(WalkCursor)){P->TestMove.Reset();P->bTestRun=false;return true;}
  if(T-WalkMark>20){Result(false,TEXT("native Walker stalled approaching ")+PendingItem);return false;}
  const FVector2D Here(P->Pos.X,P->Pos.Z),D=Route[WalkCursor]-Here;
  // Existing navigation corners have 2cm spare clearance; reach them before turning.
  if(D.Size()<.012){++WalkCursor;P->TestMove.Reset();return WalkCursor==Route.Num();}
  if(!Segment(Here,Route[WalkCursor])){Result(false,TEXT("actual route became blocked; no pose/door recovery"));return false;}
  P->Yaw=FMath::Atan2(-D.X,-D.Y);P->Pitch=-.05;P->TestMove=FVector2D(0,-FMath::Min(1.,D.Size()/.3));P->bTestRun=false;return false;
 }
 bool WalkTo(const FS47Pose& P,const FString& Label)
 {
  if(!Check(P.bValid&&Main()->Player->Walkable(P.X,P.Z),TEXT("actual authored camera spot is walkable: ")+Label))return false;
  PendingItem=Label;auto Points=Plan({P.X,P.Z});if(!Check(!Points.IsEmpty(),TEXT("existing body-clear route to camera spot: ")+Label))return false;
  Move(MoveTemp(Points));return true;
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
  if(bActual)
  {
   if(M->FieldCamera->bRaised)C->Ch1()->ToggleCamera();
   const auto* I=M->Inter->Get(Id);const auto* Exported=M->Web->Item(Id);
   const double Range=I&&I->Range>0?I->Range:Exported&&Exported->Range>0?Exported->Range:2.4;
   int32 Visible=0;
   for(double R:{.7,1.1,1.5,1.9})for(int32 K=0;K<24;++K)
   {
    const double X=At.X+R*FMath::Cos(K*PI/12),Z=At.Z+R*FMath::Sin(K*PI/12);
    if(!M->Player->Walkable(X,Z))continue;
    // Operate the lab door from the side actually reached; never walk out to shut oneself out.
    if(Id==TEXT("labDoor")&&(M->Player->Pos.Z-At.Z)*(Z-At.Z)<0)continue;
    if(C->Area()==TEXT("station01")&&Id!=TEXT("s1hutDoor")&&Hut.Contains(At.X,At.Z)!=Hut.Contains(X,Z))continue;
    double Floor=M->Player->Floor?M->Player->Floor(X,Z):M->Player->FloorY;if(!FMath::IsFinite(Floor))Floor=M->Player->FloorY;
    const FVector Eye(X,Floor+M->Player->Eye,Z),D=At-Eye;
    if(D.Size()>Range-.04||M->Inter->Pick(S47Three::ToUE(Eye),S47Three::DirToUE(D.GetSafeNormal()))!=Id)continue;
    if(Visible++<Candidate)continue;
    auto Points=Plan({X,Z});if(Points.IsEmpty())continue;
    PendingItem=Id;bAimed=false;Move(MoveTemp(Points));return true;
   }
   Result(false,TEXT("no body-clear native route and actual item ray: ")+Id);return false;
  }
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
  const int32 HitNode=HasHit?M->Web->NodeOf(Hit.GetComponent()):INDEX_NONE;
  bool HitTarget=HasHit&&TargetObject&&HitNode==TargetObject.I;
  if(bActual&&TargetObject)for(int32 Node=HitNode,Depth=0;Node>=0&&Depth<64;++Depth)
  {if(Node==TargetObject.I){HitTarget=true;break;}const auto* Part=M->Web->Node(Node);if(!Part)break;Node=Part->Parent;}
  // The transit's own large query proxy is a valid target surface, not a wall
  // hiding it. Independent opaque geometry still rejects the sight line.
  const bool Obstructed=HasHit&&!HitTarget&&Hit.Distance<FVector::Dist(Eye,End)-35.;
  S47J::Set(E,TEXT("opaqueObstruction"),Obstructed);
  S47J::Set(E,TEXT("visibilityHitIsTarget"),HitTarget);S47J::Set(E,TEXT("focused"),Focused);
  S47J::Set(E,TEXT("visibilityHitNode"),HitNode);S47J::Set(E,TEXT("targetNode"),TargetObject?TargetObject.I:INDEX_NONE);
  S47J::Set(E,TEXT("cameraForwardDotTarget"),FVector::DotProduct(Forward,(End-Eye).GetSafeNormal()));
  S47J::Set(E,TEXT("visibilityHit"),Hit.GetComponent()?Hit.GetComponent()->GetPathName():FString());
  if(Picked!=PendingItem||Focused!=PendingItem||Obstructed||FVector::DotProduct(Forward,(End-Eye).GetSafeNormal())<.995)
  { UE_LOG(LogTemp,Display,TEXT("S47: sc09 stand candidate %d to %s picked '%s'; obstruction=%d"),Candidate,*PendingItem,*Picked,Obstructed); return false; }
  if (!Check(bool(M->Player->OnUse),TEXT("actual aimed unobstructed interaction ray: ")+PendingItem)) return false;
  M->Player->OnUse(Picked);++Uses; return true;
 }
 bool UsePair(int32 Start, const TCHAR* Id, int32 Then, bool Close=true)
 {
  if (Stage==Start) { if (Stand(Id)) Next(Start+1); return true; }
  if (Stage==Start+1)
  {
   if(bActual&&!Walk())return true;
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
  if(bActual)
  {
   if(!Check(M->Flow->CaseId==1&&!M->Flow->bDevRun,TEXT("actual isolated case1 manual boundary: ")+Name)||!Papers(PriorDocs,Name))return false;
   const auto Before=S47J::Clone(M->Flow->GameState());const auto ExpectedDocs=Documents();FString Error;
   const auto Meta=M->Flow->WriteSave(TEXT("manual"),0,Error);FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();const auto Disk=Meta?Reader.Load(Meta->Id):nullptr;
   if(!Check(Disk&&SC09Same(Disk,Before),TEXT("ordinary manual save and fresh complete disk reader: ")+Name+Error))return false;
   if(!Check(FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(Root/(Name+TEXT(".loaded.json")))),TEXT("raw full disk cohort: ")+Name))return false;
   if(Name==TEXT("sc09-complete")&&!Check(FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(Root/TEXT("sc09-complete.disk.json"))),TEXT("stable actual SC09 output checkpoint before SC10 actions")))return false;
   M->Flow->ToTitle();FS47SaveStore TitleReader(M->Flow->Saves.Dir);TitleReader.Init();const auto Selected=TitleReader.ContinueSave(),FlowSelected=M->Flow->Saves.ContinueSave();
   const auto Chosen=Selected?TitleReader.Load(Selected->Id):nullptr;
   if(!Check(M->Mode==US47Main::EMode::Title&&Selected&&FlowSelected&&Selected->Id==FlowSelected->Id&&SC09Same(Chosen,Disk),TEXT("ordinary title and independent Continue select the complete saved cohort: ")+Name))return false;
   if(!Check(M->Flow->OpenTestView(TEXT("continue")),TEXT("ordinary title Continue loads actual disk choice: ")+Name))return false;
   ++Loads;++TitleContinues;
   const auto Now=M->Flow->GameState(),BeforeCase=S47J::Obj(Before,TEXT("case")),NowCase=S47J::Obj(Now,TEXT("case")),Pose=S47J::Obj(Before,TEXT("pose"));
   auto Row=S47J::New();S47J::Set(Row,TEXT("restore"),Name);S47J::Set(Row,TEXT("expected_full_state"),Before);S47J::Set(Row,TEXT("actual_full_state"),Now);Events.Add(MakeShared<FJsonValueObject>(Row));
   for(const TCHAR* Key:{TEXT("s"),TEXT("ch2"),TEXT("ch3"),TEXT("ch4")})if(!Check(SC09Same(S47J::Obj(NowCase,Key),S47J::Obj(BeforeCase,Key)),FString(TEXT("full Flow retained prior/current chapter bytes "))+Key+TEXT(" / ")+Name))return false;
   for(const TCHAR* Key:{TEXT("doors"),TEXT("office"),TEXT("spine"),TEXT("horror"),TEXT("substation"),TEXT("truck"),TEXT("odo")})if(!Check(SC09Same(S47J::Obj(Now,Key),S47J::Obj(Before,Key)),FString(TEXT("full Flow retained world/source state "))+Key+TEXT(" / ")+Name))return false;
   LastWalk=M->Player->Pos;
   return Check(M->Flow->CaseId==1&&!M->Flow->bDevRun&&M->World->Area==S47J::Str(Before,TEXT("area"))&&Pose
    &&M->Story->Clock()>=S47J::Num(BeforeCase,TEXT("clock"))&&M->Story->Clock()-S47J::Num(BeforeCase,TEXT("clock"))<3
    &&FMath::Abs(M->Player->Pos.X-S47J::Num(Pose,TEXT("x")))<.03&&FMath::Abs(M->Player->Pos.Z-S47J::Num(Pose,TEXT("z")))<.03
    &&FMath::Abs(M->Player->Yaw-S47J::Num(Pose,TEXT("yaw")))<.001&&FMath::Abs(M->Player->Pitch-S47J::Num(Pose,TEXT("pitch")))<.001,
    TEXT("ordinary case1 Continue retains actual pose/clock/area: ")+Name)&&Papers(ExpectedDocs,Name);
  }
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
   FString User,Jump,Input;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);FParse::Value(FCommandLine::Get(),TEXT("S47Jump="),Jump);
   bActual=FParse::Value(FCommandLine::Get(),TEXT("S47SC08Checkpoint="),Input);
   FString Profile=FPaths::ConvertRelativePathToFull(User);Profile.RemoveFromEnd(TEXT("/"));
   const FString Saved=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
   if(!User.StartsWith(TEXT("/tmp/S47-"))||User.Contains(TEXT(".."))||!Saved.StartsWith(Profile+TEXT("/"))) {Result(false,TEXT("explicit isolated /tmp/S47- profile and actual Saved containment required"));return;}
   Root=Saved/TEXT("SpineContract/sc09")/FGuid::NewGuid().ToString();
   IFileManager::Get().MakeDirectory(*Root,true);
   if(bActual)
   {
    if(!Check(Jump.IsEmpty()&&!FParse::Param(FCommandLine::Get(),TEXT("S47SC09TravelFixture"))&&M->Flow->CaseId==0&&M->Flow->bDevRun&&M->Flow->Saves.List().IsEmpty()
     &&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),Profile),TEXT("fresh isolated case0 before actual checkpoint import; no chapter jump/travel fixture")))return;
    for(const TCHAR* Flag:{TEXT("S47Drive"),TEXT("S47Use"),TEXT("S47Press"),TEXT("S47Open"),TEXT("S47WalkFrom"),TEXT("S47Walk"),TEXT("S47Face"),TEXT("S47Auto"),TEXT("Clock"),TEXT("S47Clock"),TEXT("S47SpineStart"),TEXT("S47EveningRoute"),TEXT("S47ProductionViews"),TEXT("S47Ch3Drive"),TEXT("S47HorrorContract"),TEXT("S47NightSequenceContract")})
    {FString Value;if(FParse::Param(FCommandLine::Get(),Flag)||FParse::Value(FCommandLine::Get(),*(FString(Flag)+TEXT("=")),Value)){Result(false,TEXT("competing world/fixture driver: ")+FString(Flag));return;}}
    const FString ReceiptPath=FPaths::GetPath(Input)/TEXT("result.json");FString Text,ReceiptText;
    const bool Read=!FPaths::IsRelative(Input)&&FPaths::GetCleanFilename(Input)==TEXT("sc08-complete.disk.json")&&FFileHelper::LoadFileToString(Text,*Input)&&FFileHelper::LoadFileToString(ReceiptText,*ReceiptPath);
    PriorState=Read?S47J::Read(Text):nullptr;const auto Receipt=Read?S47J::Read(ReceiptText):nullptr;
    const auto File=SC09File(Input),Certified=S47J::Obj(Receipt,TEXT("checkpoint_file"));
    if(!Check(PriorState&&Receipt&&File&&Certified&&S47J::Str(Receipt,TEXT("status"))==TEXT("PASS")&&S47J::Str(Receipt,TEXT("scenario"))==TEXT("SC08")
     &&S47J::Str(Certified,TEXT("path"))==Input&&S47J::Num(Certified,TEXT("bytes"))==S47J::Num(File,TEXT("bytes"))&&S47J::Str(Certified,TEXT("sha1"))==S47J::Str(File,TEXT("sha1")),TEXT("actual SC08 PASS pins this exact absolute checkpoint path/bytes/SHA1")))return;
    ChainId=S47J::Str(Receipt,TEXT("chain_id"));const auto Case=S47J::Obj(PriorState,TEXT("case")),Spine=S47J::Obj(PriorState,TEXT("spine")),Act=S47J::Obj(Spine,TEXT("sc08d")),Ch3=S47J::Obj(Case,TEXT("ch3"));
    const auto Ch1=S47J::Obj(Case,TEXT("s")),Ch2=S47J::Obj(Case,TEXT("ch2")),Office=S47J::Obj(PriorState,TEXT("office"));
    if(!Check(!ChainId.IsEmpty()&&S47J::Int(PriorState,TEXT("timelineVersion"))==2&&(S47J::Int(Spine,TEXT("v"))==1||S47J::Int(Spine,TEXT("v"))==2)
     &&S47J::Int(Spine,TEXT("sc08"))==2&&S47J::Bool(Act,TEXT("escaped"))&&S47J::Num(Act,TEXT("escapedAt"))>0
     &&S47J::Int(Act,TEXT("plays"))>=2&&S47J::Bool(Act,TEXT("labelRead"))&&S47J::Bool(Act,TEXT("isolated"))&&S47J::Bool(Act,TEXT("referenceRecorded"))&&S47J::Bool(Act,TEXT("compared"))
     &&S47J::Arr(Act,TEXT("samples")).Num()>=3&&S47J::Arr(Act,TEXT("pointings")).Num()==27
     &&S47J::Bool(Ch1,TEXT("filed"))&&S47J::Int(Ch1,TEXT("dev1"))==3&&S47J::Int(Ch1,TEXT("dev2"))==3&&S47J::Has(Ch1,TEXT("f1"))&&S47J::Has(Ch1,TEXT("f2"))
     &&S47J::Int(Ch2,TEXT("sc07Version"))==1&&S47J::Bool(Ch2,TEXT("binderCompared"))&&S47J::Bool(Ch2,TEXT("p04"))&&S47J::Bool(Ch2,TEXT("p05"))&&S47J::Bool(Ch2,TEXT("called"))&&S47J::Bool(Office,TEXT("departed"))
     &&S47J::Str(Ch3,TEXT("stage"))==TEXT("to-truck")&&S47J::Int(Ch3,TEXT("surveyVersion"))==1&&!S47J::Has(Ch3,TEXT("f3"))&&!S47J::Has(Ch3,TEXT("f4"))&&!S47J::Bool(Ch3,TEXT("called"))
     &&!S47J::Bool(Ch3,TEXT("arrived"))&&!S47J::Bool(Ch3,TEXT("p06"))&&!S47J::Bool(Ch3,TEXT("p07"))&&!S47J::Bool(Ch3,TEXT("p08"))&&!S47J::Bool(Ch3,TEXT("p09"))
     &&S47J::Num(Case,TEXT("clock"))>=S47Timeline::Chapter3(true)&&S47J::Num(Case,TEXT("clock"))<S47Timeline::Chapter3(true)+120,TEXT("real escaped SC08/026 and untouched04:25 SC09 boundary; spine1/2 allowed, no source fabrication")))return;
    if(!Check(SC09Copy(Input,Root/TEXT("input-sc08.disk.json"))&&SC09Copy(ReceiptPath,Root/TEXT("input-sc08.result.json")),TEXT("byte-identical predecessor checkpoint and PASS receipt retained")))return;
    FString CopiedData,CopiedReceipt;const auto Copied=SC09File(Root/TEXT("input-sc08.disk.json"));
    if(!Check(Copied&&S47J::Str(Copied,TEXT("sha1"))==S47J::Str(File,TEXT("sha1"))&&S47J::Num(Copied,TEXT("bytes"))==S47J::Num(File,TEXT("bytes"))&&SC09Same(SC09File(Input),File)
     &&FFileHelper::LoadFileToString(CopiedData,*(Root/TEXT("input-sc08.disk.json")))&&FFileHelper::LoadFileToString(CopiedReceipt,*(Root/TEXT("input-sc08.result.json")))&&SC09Same(S47J::Read(CopiedData),PriorState)&&SC09Same(S47J::Read(CopiedReceipt),Receipt),TEXT("copied bytes rehash and parse as the exact certified input; no unbound read/copy race")))return;
    Prerequisite=S47J::New();S47J::Set(Prerequisite,TEXT("checkpoint"),File);S47J::Set(Prerequisite,TEXT("result"),SC09File(ReceiptPath));
    US47Flow::FStart Start;Start.State=PriorState;Start.CaseId=1;Start.bDev=false;M->Flow->StartGame(Start);Next(100,.5);return;
   }
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
  if(Stage==100)
  {
   if(!Check(M->Mode==US47Main::EMode::Play&&M->Flow->CaseId==1&&!M->Flow->bDevRun&&C->bActive&&C->S.SurveyVersion==1&&C->S.Stage==TEXT("to-truck")&&C->Site()&&M->Story->bEveningTimeline,
    TEXT("ordinary full Flow restored the actual SC08 boundary into isolated case1")))return;
   const auto Now=M->Flow->GameState();for(const TCHAR* Key:{TEXT("s"),TEXT("ch2"),TEXT("ch3")})if(!Check(SC09Same(S47J::Obj(S47J::Obj(Now,TEXT("case")),Key),S47J::Obj(S47J::Obj(PriorState,TEXT("case")),Key)),FString(TEXT("predecessor actual chapter sources unchanged: "))+Key))return;
   for(const TCHAR* Id:{TEXT("obs_runbinder"),TEXT("EV_DALE_INDEX"),TEXT("EV_DALE_NOTE"),TEXT("e07"),TEXT("e06"),TEXT("e08"),TEXT("p04"),TEXT("p05"),TEXT("frame01"),TEXT("frame02"),TEXT("witness-reel"),TEXT("sc08_reel_cues"),TEXT("EV_DALE_ARRAY_SERVICE"),TEXT("sc08_comparison")})
   {
    const auto* Doc=M->Story->Docs().FindByPredicate([Id](const FS47Doc& D){return D.Id==Id;});
    if(!Check(Doc&&(!Doc->Page.IsEmpty()||!Doc->Transcript.IsEmpty()||!Doc->Image.IsEmpty()),FString(TEXT("actual inherited source present, no substitute: "))+Id))return;
   }
   PriorDocs=Documents();if(!Restore(TEXT("actual-sc08-input")))return;Next(101);return;
  }
  if(Stage==101)
  {
   if(!Check(!C->S.F3&&!C->S.F4&&!C->S.bCalled&&!C->bRinging,TEXT("actual SC09 opening still has no field exposure or Nora credit")))return;
   if(!Restore(TEXT("before-station")))return;OdoStart=M->World->Odometer();bWasDriving=false;M->World->Autopilot=24;Next(M->Web->IsDoorOpen(TEXT("doorEast"))?104:102);return;
  }
  if(UsePair(102,TEXT("doorEast"),104)||UsePair(104,TEXT("truck"),1))return;
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
   if(bActual){if(!WalkTo(P,TEXT("FRAME03 camera")))return;}else M->Player->Place(P.X,P.Z,P.Yaw);Next(19);return;
  }
  if(Stage==19){if(bActual&&!Walk())return;Aim(C->Site()->MarkerTarget());if(!M->FieldCamera->bRaised)C->Ch1()->ToggleCamera();Next(20,.5);return;}
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
   if(bActual){if(!WalkTo(P,TEXT("FRAME04 camera")))return;}else M->Player->Place(P.X,P.Z,P.Yaw);Next(29);return;
  }
  if(Stage==29){if(bActual&&!Walk())return;Aim(C->Site()->CableTarget());if(!M->FieldCamera->bRaised)C->Ch1()->ToggleCamera();Next(30,.5);return;}
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
   if(bActual){Next(360);return;}
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
  if(UsePair(360,TEXT("s1hutDoor"),362))return;
  if(Stage==362){OdoStart=M->World->Odometer();bWasDriving=false;M->World->Autopilot=24;Next(364);return;}
  if(UsePair(364,TEXT("s1truck"),38))return;
  if(Stage==38)
  {
   bWasDriving|=M->World->bDriving;
   if(M->World->bDriving||M->World->bBusy||C->Area()!=TEXT("saro"))return;
   // Park releases the ordinary arrival callback after its real 0.4-second hold.
   // The truck being stopped is not yet the chapter's arrival transaction.
   if(ReturnWaitAt<0)ReturnWaitAt=T;
   if(C->S.Stage!=TEXT("develop"))
   {
    if(T-ReturnWaitAt>8)Result(false,TEXT("ordinary SARO parking did not deliver the chapter arrival callback"));
    return;
   }
   BackMetres=(M->World->Odometer()-OdoStart)*1609.344;
   if(!Check(bWasDriving&&BackMetres>10,TEXT("ordinary World return actually reaches SARO with both negatives after its arrival transaction")))return;Next(40);return;
  }
  if(Stage==40)
  {
   if(!Check(C->S.Stage==TEXT("develop")&&C->S.Dev==0&&!C->bRinging&&!C->S.bCalled,TEXT("04:40 return waits for both field prints")))return;
   if(bActual){if(!Restore(TEXT("before-developing")))return;Next(M->Web->IsDoorOpen(TEXT("labDoor"))?402:400);return;}
   M->Web->SetDoorOpen(TEXT("lab"),false,true); // declared access fixture; paper/fog actions below remain production controls.
   if(!Restore(TEXT("before-developing")))return;Next(42);return;
  }
  if(UsePair(400,TEXT("labDoor"),402)||UsePair(402,TEXT("wetBench"),404,false))return;
  if(Stage==404){M->UI->Close(true);Next(406);return;}
  if(UsePair(406,TEXT("labDoor"),42))return;
  if(UsePair(42,TEXT("wetBench"),44,false))return;
  if(Stage==44){if(!Click(TEXT("LOAD SEALED FILM INTO TANK")))return;Next(45,.2);return;}
  if(Stage==45)
  {
   if(!Check(!C->SaveBlock().IsEmpty(),TEXT("transient development cannot be saved")))return;
   if(bActual){M->UI->Close(true);Next(450);return;}
   M->Web->SetDoorOpen(TEXT("lab"),true,true);
   if(!Check(M->Web->DoorStates().FindRef(TEXT("lab"))&&M->Web->IsDoorOpen(TEXT("labDoor")),TEXT("actual darkroom door is open by saved id and production interaction id")))return;
   Next(46,.4);return;
  }
  if(UsePair(450,TEXT("labDoor"),46))return;
  if(Stage==46)
  {
   if(!Check(C->S.Dev==0&&C->S.Fogged==1&&C->Busy.IsEmpty()&&!C->bRinging&&C->S.F3&&C->S.F4,FString::Printf(TEXT("opening actual lab door fogs only paper, retaining both negatives: dev=%d fogged=%d busy=%s"),C->S.Dev,C->S.Fogged,*C->Busy)))return;
   if(bActual){if(!Restore(TEXT("fogged-paper-negatives-retained")))return;Next(460);return;}
   M->Web->SetDoorOpen(TEXT("lab"),false,true);if(!Restore(TEXT("fogged-paper-negatives-retained")))return;Next(48);return;
  }
  if(UsePair(460,TEXT("labDoor"),48))return;
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
  if(Stage==60){if(!Check(C->bRinging&&M->Story->Label(TEXT("phone"))==TEXT("Answer Nora Vega"),TEXT("only now the ordinary SARO phone rings for Nora")))return;Next(bActual?600:62);return;}
  if(UsePair(600,TEXT("labDoor"),602))return;
  if(Stage==602){Next(M->Web->IsDoorOpen(TEXT("doorEast"))?62:604);return;}
  if(UsePair(604,TEXT("doorEast"),62))return;
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
   if(!bActual&&!Check(!M->Substation||!M->Substation->FollowupVerifiedProof(),TEXT("optional T02 verification was not required for SC09/SC10 progression")))return;
   if(bActual)
   {
    const auto State=M->Flow->GameState();const auto Ch4=S47J::Obj(S47J::Obj(State,TEXT("case")),TEXT("ch4"));
    if(!Check(S47J::Str(State,TEXT("checkpoint"))==TEXT("chapter4")&&S47J::Str(Ch4,TEXT("stage"))==TEXT("to-motel")&&!S47J::Bool(Ch4,TEXT("arrived"))&&!S47J::Bool(Ch4,TEXT("greeted")),TEXT("natural C4 boundary has no SC10 interaction/travel credit")))return;
    if(!Restore(TEXT("sc09-complete")))return;
    if(!Check(TitleContinues==Loads&&TitleContinues>=10&&Uses>=16&&Walked>40&&WalkSamples>100&&OutMetres>100&&BackMetres>100,TEXT("actual checkpoint chain, native walks/rays, ordinary trips and case1/title cohorts executed")))return;
    Result(true,FString());return;
   }
   if(!Restore(TEXT("sc10-boundary")))return;
   if(!Check(SC09Copy(Root/TEXT("sc10-boundary.loaded.json"),Root/TEXT("sc09-complete.disk.json")),TEXT("fixture output is explicitly tagged and retains the actual final disk bytes")))return;Result(true,FString());return;
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

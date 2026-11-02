#include "S47DaleDocumentsTest.h"
#include "S47Chapter2.h"
#include "S47Evening.h"
#include "S47Flow.h"
#include "S47HUD.h"
#include "S47Inter.h"
#include "S47Three.h"
#include "SS47Widgets.h"
#include "S47Main.h"
#include "S47Office.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "UnrealClient.h"
#include "Widgets/Text/STextBlock.h"
#include "Algo/Reverse.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47DaleDocs, Log, All);

namespace
{
 struct FDeskPaper { const TCHAR* Use; const TCHAR* Doc; const TCHAR* Flag; double X, Z; };
 // All approaches remain in the real room, south of deskFoot z9.31-radius.27
 // and west of the rotated source chairFoot minX .708089-radius.27.
 // No placement follows entry in the document-only contract.
 const FDeskPaper DeskPapers[] = {
  {TEXT("office:desk"), TEXT("EV_DALE_INDEX"), TEXT("deskSeen"), .35, 8.85},
  {TEXT("office:index"), TEXT("EV_DALE_INDEX"), TEXT("indexRead"), .02, 8.96},
  {TEXT("office:clippings"), TEXT("EV_DALE_CLIPPINGS"), TEXT("clippingsRead"), .34, 9.01},
  {TEXT("office:note"), TEXT("EV_DALE_NOTE"), TEXT("noteRead"), .08, 8.85},
  {TEXT("office:contact"), TEXT("EV_DALE_CONTACT"), TEXT("contactRead"), .35, 8.92},
  {TEXT("office:folder"), TEXT("EV_DALE_INDEX"), TEXT("fieldSheet"), .08, 8.98}
 };
 void ModalText(const TSharedRef<SWidget>& Widget, TArray<FString>& Text)
 {
  if (Widget->GetTypeAsString() == TEXT("STextBlock")) Text.Add(StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString());
  FChildren* Children = Widget->GetChildren();
  for (int32 Index = 0; Children && Index < Children->Num(); ++Index) ModalText(Children->GetChildAt(Index), Text);
 }
}

FS47DaleDocumentsTest::FS47DaleDocumentsTest(US47Main* InM) : M(InM)
{
 bCollision = FParse::Param(FCommandLine::Get(), TEXT("S47DaleCollisionContract"));
 bEnabled = bCollision || FParse::Param(FCommandLine::Get(), TEXT("S47DaleDocumentsContract"));
}
void FS47DaleDocumentsTest::Fail(const FString& Why)
{
 if (bDone) return; bDone = true;
 if (M && M->Player) M->Player->TestMove.Reset();
 UE_LOG(LogS47DaleDocs, Error, TEXT("S47: Dale documents FAIL stage=%d checks=%d: %s"), Stage, Checks, *Why);
 if (!Root.IsEmpty()) { auto R = S47J::New(); S47J::Set(R,TEXT("status"),TEXT("FAIL")); S47J::Set(R,TEXT("reason"),Why); S47J::Set(R,TEXT("stage"),Stage); S47J::Set(R,TEXT("checks"),Checks); R->SetArrayField(TEXT("physicalDeskPapers"),PhysicalPapers); R->SetArrayField(TEXT("collisionMoves"),CollisionMoves); S47J::Set(R,TEXT("closedDoorProbe"),CollisionDoorProbe); FFileHelper::SaveStringToFile(S47J::Write(R,true), *(Root / TEXT("result.json"))); }
 FPlatformMisc::RequestExitWithStatus(false, 1);
}
bool FS47DaleDocumentsTest::Check(bool Good, const FString& Why)
{
 ++Checks; if (!Good) { Fail(Why); return false; }
 UE_LOG(LogS47DaleDocs, Display, TEXT("S47: Dale documents PASS: %s"), *Why); return true;
}
void FS47DaleDocumentsTest::Next(int32 Value, double Wait) { Stage = Value; Hold = Wait; }
void FS47DaleDocumentsTest::Shot(const FString& Name) { FScreenshotRequest::RequestScreenshot(Root / (Name + TEXT(".png")), true, false); }
bool FS47DaleDocumentsTest::OfficeFacts(const S47J::FObj& Office, int32 Completed, const FString& Context)
{
 for (int32 Index = 0; Index < UE_ARRAY_COUNT(DeskPapers); ++Index)
 {
  const TCHAR* Flag = DeskPapers[Index].Flag;
  if (!Check(Office && Office->HasTypedField<EJson::Boolean>(Flag) && S47J::Bool(Office,Flag) == (Index < Completed),
             Context + TEXT(" actual ") + Flag)) return false;
 }
 return Completed < UE_ARRAY_COUNT(DeskPapers) || Check(Office->HasTypedField<EJson::Number>(TEXT("fieldSheetAt"))
  && FMath::IsFinite(S47J::Num(Office,TEXT("fieldSheetAt"))), Context + TEXT(" actual finite field-sheet acquisition clock"));
}
bool FS47DaleDocumentsTest::AuditArt(const FString& Context)
{
 TArray<FString> Lines; const bool Good = M->Office->AuditDocumentArt(Lines);
 auto Evidence = S47J::New(); S47J::Set(Evidence,TEXT("context"),Context); S47J::Set(Evidence,TEXT("lines"),Lines);
 S47J::Set(Evidence,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));
 if (!Check(FFileHelper::SaveStringToFile(S47J::Write(Evidence,true),*(Root/(Context+TEXT(".art.json")))),TEXT("write actual art audit: ")+Context)) return false;
 if (!Check(Good,TEXT("actual six bound document surfaces: ")+Context)) return false;
 for (int32 Index = 0; Index < UE_ARRAY_COUNT(DeskPapers); ++Index)
 {
  const FS47Obj Obj = M->Office->Object(DeskPapers[Index].Use); const FBox Bounds = Obj.WorldBox();
  const FS47Interactable* Item = M->Inter->Get(DeskPapers[Index].Use);
  if (!Check(Obj && Bounds.IsValid && Item && Item->Object.I == Obj.I,TEXT("original physical paper/interaction identity: ")+FString(DeskPapers[Index].Use))) return false;
  if (!PaperNodes.IsValidIndex(Index)) { PaperNodes.Add(Obj.I); PaperBounds.Add(Bounds); }
  else if (!Check(PaperNodes[Index] == Obj.I && PaperBounds[Index].Min.Equals(Bounds.Min,1e-6)
                 && PaperBounds[Index].Max.Equals(Bounds.Max,1e-6),TEXT("unchanged actual paper bounds/node: ")+FString(DeskPapers[Index].Use))) return false;
 }
 ++ArtAudits; return true;
}
bool FS47DaleDocumentsTest::FinishRestore()
{
 const auto Restored = M->Office->Save(); const auto Disk = S47J::Obj(RestoreReceipt,TEXT("diskOffice"));
 S47J::Set(RestoreReceipt,TEXT("restoredOffice"),Restored);
 if (!OfficeFacts(Restored,UE_ARRAY_COUNT(DeskPapers),TEXT("restored ")+PendingRestore)
     || !Check(S47J::Num(Restored,TEXT("fieldSheetAt")) == S47J::Num(Disk,TEXT("fieldSheetAt")),TEXT("full Flow retains real field-sheet acquisition time: ")+PendingRestore)
     || !AuditArt(TEXT("after-")+PendingRestore)) return false;
 if (!Check(FFileHelper::SaveStringToFile(S47J::Write(RestoreReceipt,true),*(Root/(PendingRestore+TEXT(".office.json")))),TEXT("write source/disk/restored office facts: ")+PendingRestore)) return false;
 PendingRestore.Reset(); RestoreReceipt.Reset(); return true;
}
bool FS47DaleDocumentsTest::StoreRestore(const FString& Name, const S47J::FObj& Snapshot)
{
 if (!AuditArt(TEXT("before-")+Name)) return false;
 const auto SourceOffice = S47J::Obj(Snapshot,TEXT("office")), LiveOffice = M->Office->Save();
 if (!OfficeFacts(SourceOffice,UE_ARRAY_COUNT(DeskPapers),TEXT("source snapshot ")+Name)
     || !OfficeFacts(LiveOffice,UE_ARRAY_COUNT(DeskPapers),TEXT("live source ")+Name)) return false;
 FS47SaveStore Writer(Root / Name); Writer.Init(); FS47SaveCard Card; Card.Chapter = TEXT("Dale documents contract"); Card.Place = TEXT("Operations"); Card.Clock = M->Story->ClockText(true);
 FString Error; const auto Meta = Writer.Save(1, TEXT("manual"), 0, S47J::Clone(Snapshot), Card, Error);
 if (!Meta) { Fail(TEXT("isolated Writer failed: ") + Error); return false; }
 FS47SaveStore Reader(Root / Name); Reader.Init(); const auto Disk = Reader.Load(Meta->Id);
 if (!Check(Disk.IsValid(), TEXT("fresh isolated disk reader: ") + Name)) return false;
 const auto DiskOffice = S47J::Obj(Disk,TEXT("office"));
 if (!OfficeFacts(DiskOffice,UE_ARRAY_COUNT(DeskPapers),TEXT("actual disk ")+Name)
     || !Check(S47J::Num(DiskOffice,TEXT("fieldSheetAt")) == S47J::Num(SourceOffice,TEXT("fieldSheetAt")),TEXT("disk preserves actual field-sheet acquisition time: ")+Name)) return false;
 RestoreReceipt = S47J::New(); S47J::Set(RestoreReceipt,TEXT("cohort"),Name); S47J::Set(RestoreReceipt,TEXT("sourceOffice"),SourceOffice);
 S47J::Set(RestoreReceipt,TEXT("liveSourceOffice"),LiveOffice); S47J::Set(RestoreReceipt,TEXT("diskOffice"),DiskOffice); PendingRestore = Name;
 FFileHelper::SaveStringToFile(S47J::Write(Disk,true), *(Root / (Name + TEXT(".loaded.json")))); ++Stores;
 US47Flow::FStart Start; Start.State = Disk; Start.CaseId = 0; Start.bDev = true; Start.Playtime = M->Playtime; M->Flow->StartGame(Start);
 return Check(M->Flow->bDevRun && M->Flow->CaseId == 0 && M->Office->WasUnlocked(), TEXT("full Flow restore keeps actual office access: ") + Name);
}
bool FS47DaleDocumentsTest::CollisionClear(const FVector& At) const
{
 const auto* P=M->Player.Get();const double R=P->Radius,E=1e-7;
 if(!P->Walkable(At.X,At.Z))return false;
 for(const FS47Rect& C:M->Web->Colliders())
 {
  if(!C.bEnabled)continue;
  if(C.Id==TEXT("truck")&&P->TruckBox&&P->TruckBox->Area==M->Web->Area())continue;
  if(C.Id==TEXT("personalCar")&&P->PersonalCarBox&&P->PersonalCarBox->Area==M->Web->Area())continue;
  if(At.X+R>C.MinX+E&&At.X-R<C.MaxX-E&&At.Z+R>C.MinZ+E&&At.Z-R<C.MaxZ-E)return false;
 }
 // Independent endpoint geometry oracle, not a copy of the movement solver.
 for(const auto* Box:{&P->TruckBox,&P->PersonalCarBox})if(Box->IsSet()&&Box->GetValue().Area==M->Web->Area())
 {
  const auto& B=Box->GetValue();const double C=FMath::Cos(B.Heading),S=FMath::Sin(B.Heading);
  const double X=(At.X-B.X)*C-(At.Z-B.Z)*S,Z=(At.X-B.X)*S+(At.Z-B.Z)*C;
  const double DX=FMath::Max(0.,FMath::Abs(X)-B.HalfW),DZ=FMath::Max(0.,FMath::Abs(Z)-B.HalfL);
  if(DX*DX+DZ*DZ<FMath::Square(R-E))return false;
 }
 return true;
}
bool FS47DaleDocumentsTest::ProbeCollisionMoves(bool Airborne)
{
 auto* P=M->Player.Get();auto& Rows=M->Web->CollidersMutable(M->Web->Area());
 const FS47Rect* Chair=Rows.FindByPredicate([](const FS47Rect& C){return C.Id==TEXT("office:chairFoot")&&C.bEnabled;});
 const FS47Rect* Desk=Rows.FindByPredicate([](const FS47Rect& C){return C.Id==TEXT("office:deskFoot")&&C.bEnabled;});
 const FS47Rect* Wall=Rows.FindByPredicate([](const FS47Rect& C){return C.Id==TEXT("office:researchWall")&&C.bEnabled;});
 if(!Check(Chair&&Desk&&Wall&&Chair->MaxX+P->Radius>Wall->MinX-P->Radius&&Chair->MaxZ+P->Radius>Desk->MinZ-P->Radius,
           TEXT("actual rotated chair envelope overlaps expanded desk and research wall")))return false;
 struct FProbe {FVector From;FVector2D Delta;};
 // Explicit reproduction fixtures from the measured source chair. Expected results
 // are free-space/bounded-motion invariants, never a duplicated clipping algorithm.
 const FProbe Probes[]={{{.39,0,8.22},{.05048395947625578,.0044167741440745215}},
                       {{.30,0,7.95},{.17420385183546452,.9879591378218471}}};
 for(int32 I=0;I<UE_ARRAY_COUNT(Probes);++I)
 {
  const auto& Q=Probes[I];FVector Forward;
  for(int32 Order=0;Order<2;++Order)
  {
   P->Pos=Q.From;
   if(!Check(CollisionClear(P->Pos)&&P->InAir()==Airborne,TEXT("collision fixture begins clear in the requested ground/air state")))return false;
   if(Order)Algo::Reverse(Rows);
   P->MoveBy(Q.Delta.X,Q.Delta.Y);
   if(Order)Algo::Reverse(Rows);
   const FVector After=P->Pos;
   auto R=S47J::New();S47J::Set(R,TEXT("airborne"),Airborne);S47J::Set(R,TEXT("probe"),I);S47J::Set(R,TEXT("reversedColliders"),Order!=0);
   S47J::Set(R,TEXT("from"),TArray<double>{Q.From.X,Q.From.Z});S47J::Set(R,TEXT("requested"),TArray<double>{Q.Delta.X,Q.Delta.Y});S47J::Set(R,TEXT("actual"),TArray<double>{After.X,After.Z});CollisionMoves.Add(MakeShared<FJsonValueObject>(R));
   if(!Check(CollisionClear(After),TEXT("actual movement endpoint clears every active relevant collider and stays walkable")))return false;
   const double E=1e-6;
   if(!Check(After.X>=FMath::Min(Q.From.X,Q.From.X+Q.Delta.X)-E&&After.X<=FMath::Max(Q.From.X,Q.From.X+Q.Delta.X)+E
             &&After.Z>=FMath::Min(Q.From.Z,Q.From.Z+Q.Delta.Y)-E&&After.Z<=FMath::Max(Q.From.Z,Q.From.Z+Q.Delta.Y)+E,
             TEXT("normal/large step never reverses or adds unrequested axis travel")))return false;
   if(!Order)Forward=After;
   else if(!Check(After.Equals(Forward,1e-6),TEXT("reversing actual collider order leaves the movement endpoint unchanged")))return false;
   const FVector BeforeRetreat=P->Pos;P->MoveBy(-.1,-.1);
   if(!Check(CollisionClear(P->Pos)&&FVector2D(P->Pos.X-BeforeRetreat.X,P->Pos.Z-BeforeRetreat.Z).Size()>.1,
             TEXT("player can immediately move away from chair contact")))return false;
  }
 }
 P->Pos=FVector(.20,0,8.65);return true;
}
bool FS47DaleDocumentsTest::CheckCollisionRestore(bool Recovered)
{
 const FVector At=M->Player->Pos;const auto Pose=S47J::Obj(CollisionSnapshot,TEXT("pose"));
 if(!Check(CollisionClear(At)&&M->Office->Contains(At),TEXT("full disk/Flow restore produces a clear position in the same office")))return false;
 if(!Check(Recovered?(FVector::DistSquared(At,CollisionRestoredFrom)>.0001&&FVector::DistSquared(At,CollisionRestoredFrom)<=FMath::Square(1.25)+1e-6):At.Equals(CollisionRestoredFrom,1e-9),
           Recovered?TEXT("overlapping current-layout save recovers only within the bounded local office"):TEXT("valid current-layout saved position is unchanged")))return false;
 if(!Check(FMath::IsNearlyEqual(M->Player->Yaw,S47J::Num(Pose,TEXT("yaw")),1e-9)&&FMath::IsNearlyEqual(M->Player->Pitch,S47J::Num(Pose,TEXT("pitch")),1e-9),TEXT("disk/Flow collision recovery preserves saved yaw and pitch")))return false;
 const auto Saved=S47J::Obj(CollisionSnapshot,TEXT("office")),Live=M->Office->Save();
 for(const TCHAR* Key:{TEXT("key"),TEXT("unlocked"),TEXT("everOpened"),TEXT("entered"),TEXT("doorInspected"),TEXT("doorOpen"),TEXT("drawerOpen"),TEXT("routeRequired"),TEXT("deskSeen"),TEXT("indexRead"),TEXT("clippingsRead"),TEXT("noteRead"),TEXT("contactRead"),TEXT("fieldSheet"),TEXT("hasFieldSheet"),TEXT("departed")})
  if(!Check(S47J::Bool(Saved,Key)==S47J::Bool(Live,Key),FString(TEXT("collision restore preserves actual office fact "))+Key))return false;
 if(!Check(S47J::Num(Live,TEXT("interiorLayout"))==2&&M->Office->HasFieldSheet()&&!M->Evening->Pack.Flag(TEXT("vhs_seen"))&&!M->Evening->Pack.Flag(TEXT("trafo_anomaly_checked")),TEXT("current layout and real field-sheet progression retained without completing unrelated work")))return false;
 const FVector Before=M->Player->Pos;M->Player->MoveBy(0,-.2);
 return Check(CollisionClear(M->Player->Pos)&&FVector::Dist(Before,M->Player->Pos)>.15,TEXT("restored player can walk away without another recovery"));
}
void FS47DaleDocumentsTest::CollisionStep()
{
 auto* P=M->Player.Get();
 if(Stage==200)
 {
  P->TestMove.Reset();P->Place(.20,8.65,-1.655);P->Pitch=-.6;
  Next(2001,.2);return;
 }
 if(Stage==2001)
 {
  Shot(TEXT("collision-chair-before-jump"));Next(2002,.2);return;
 }
 if(Stage==2002)
 {
  if(!ProbeCollisionMoves(false))return;
  P->Jump();
  if(!Check(P->InAir(),TEXT("ordinary Jump starts in the real office")))return;
  Next(201,.15);return;
 }
 if(Stage==201)
 {
  if(!Check(P->InAir()&&P->Lift>.02,TEXT("native Tick advances the office jump above the floor"))||!ProbeCollisionMoves(true))return;
  Shot(TEXT("collision-chair-airborne"));Next(202,1.);return;
 }
 if(Stage==202)
 {
  if(!Check(!P->InAir()&&CollisionClear(P->Pos),TEXT("ordinary landing leaves player clear and mobile")))return;
  // One synthetic rotated-vehicle regression, kept synchronous and restored before
  // any Tick. The rendered Jeep is not moved and this is not a driving claim.
  auto& Rows=M->Web->CollidersMutable(M->Web->Area());auto& Zones=M->Web->ZonesMutable(M->Web->Area());
  const auto OldRows=Rows,OldZones=Zones;const auto Truck=P->TruckBox,Car=P->PersonalCarBox;const FVector Before=P->Pos;
  {
   ON_SCOPE_EXIT {Rows=OldRows;Zones=OldZones;P->TruckBox=Truck;P->PersonalCarBox=Car;P->Pos=Before;};
   Rows={{-2.2,2.2,5.8,10.2,TEXT("truck"),true}};Zones={{-5,5,3,13,TEXT("collision-test-ground"),true}};
   AS47Walker::FTruckBox B;B.Area=M->Web->Area();B.X=0;B.Z=8;B.Heading=UE_DOUBLE_PI/4;B.HalfW=.4;B.HalfL=2;P->TruckBox=B;P->PersonalCarBox.Reset();
   P->Pos=FVector(1.8,0,6.2);P->MoveBy(-.2,0);
   if(!Check(P->Pos.Equals(FVector(1.6,0,6.2),1e-6)&&CollisionClear(P->Pos),TEXT("rotated vehicle leaves its empty AABB corner walkable")))return;
   P->Pos=FVector(-3,0,8);P->MoveBy(6,0);
   if(!Check(P->Pos.X>-3&&P->Pos.X<0&&CollisionClear(P->Pos),TEXT("large step stops before the actual rotated vehicle instead of tunnelling")))return;
  }
  const FS47Rect* ContactChair=Rows.FindByPredicate([](const FS47Rect& C){return C.Id==TEXT("office:chairFoot")&&C.bEnabled;});
  if(!Check(ContactChair!=nullptr,TEXT("valid current-save contact fixture has the actual chair envelope")))return;
  // Exactly the expanded contact face, without an artificial epsilon: adding
  // Radius back can round one ULP inside MinX and must not trigger recovery.
  P->Pos=FVector(ContactChair->MinX-P->Radius,0,(ContactChair->MinZ+ContactChair->MaxZ)*.5);
  if(!Check(CollisionClear(P->Pos),TEXT("exact chair contact is a valid modern saved position")))return;
  P->Yaw=-1.655;P->Pitch=-.6;CollisionRestoredFrom=P->Pos;CollisionSnapshot=S47J::Clone(M->Flow->GameState());
  if(!Check(S47J::Num(S47J::Obj(CollisionSnapshot,TEXT("office")),TEXT("interiorLayout"))==2,TEXT("valid save fixture uses the current interior layout"))||!StoreRestore(TEXT("collision-modern-valid"),CollisionSnapshot))return;
  Next(203,.6);return;
 }
 if(Stage==203)
 {
  if(!CheckCollisionRestore(false))return;
  const auto& Rows=M->Web->Colliders();const FS47Rect* Chair=Rows.FindByPredicate([](const FS47Rect& C){return C.Id==TEXT("office:chairFoot");});
  const FS47Rect* Wall=Rows.FindByPredicate([](const FS47Rect& C){return C.Id==TEXT("office:researchWall");});
  if(!Check(Chair&&Wall,TEXT("recovery fixture takes bounds from actual chair and research wall")))return;
  CollisionRestoredFrom=FVector((Wall->MinX-P->Radius+FMath::Min(Chair->MaxX+P->Radius,Wall->MaxX+P->Radius))*.5,0,(Chair->MinZ+Chair->MaxZ)*.5);
  if(!Check(!CollisionClear(CollisionRestoredFrom),TEXT("modern saved-pose fixture genuinely overlaps today's colliders")))return;
  FVector Outside(10,0,10);const FVector OutsideBefore=Outside;
  if(!Check(!M->Office->MigrateRestoredPose(Outside,P->Radius)&&Outside.Equals(OutsideBefore,0),TEXT("modern office recovery leaves an unrelated outside pose unchanged")))return;
  {
   auto& Mutable=M->Web->CollidersMutable(M->Web->Area());const int32 Count=Mutable.Num();
   ON_SCOPE_EXIT {Mutable.SetNum(Count);};
   // No candidate can be valid in this explicit failure fixture. It must not
   // teleport or downgrade layout2 to the broad legacy migration on next save.
   Mutable.Add({-10,10,0,20,TEXT("collision-test-no-clear-recovery"),true});
   FVector Blocked=CollisionRestoredFrom;
   if(!Check(!M->Office->MigrateRestoredPose(Blocked,P->Radius)&&Blocked.Equals(CollisionRestoredFrom,0)
             &&S47J::Num(M->Office->Save(),TEXT("interiorLayout"))==2,TEXT("failed modern recovery preserves position and current layout without legacy fallback")))return;
  }
  CollisionSnapshot=S47J::Clone(M->Flow->GameState());const auto Pose=S47J::Obj(CollisionSnapshot,TEXT("pose"));
  S47J::Set(Pose,TEXT("x"),CollisionRestoredFrom.X);S47J::Set(Pose,TEXT("z"),CollisionRestoredFrom.Z);
  if(!StoreRestore(TEXT("collision-modern-overlap"),CollisionSnapshot))return;
  Next(204,.6);return;
 }
 if(Stage==204)
 {
  if(!CheckCollisionRestore(true))return;
  Shot(TEXT("collision-chair-after-continue"));
  if(!Check(M->Office->Use(TEXT("office:door")),TEXT("normal office latch closes from a clear room position")))return;
  Next(205,.8);return;
 }
 if(Stage==205)
 {
  if(!Check(!M->Office->DoorOpen(),TEXT("closed office door remains closed before crossing test")))return;
  P->Place(.45,6.2,0);const FVector Before=P->Pos,Requested=Before+FVector(0,0,1.4);P->MoveBy(0,1.4);
  CollisionDoorProbe=S47J::New();const auto D=CollisionDoorProbe;const double R=P->Radius,E=1e-7;
  S47J::Set(D,TEXT("before"),TArray<double>{Before.X,Before.Z});S47J::Set(D,TEXT("requested"),TArray<double>{Requested.X,Requested.Z});S47J::Set(D,TEXT("actual"),TArray<double>{P->Pos.X,P->Pos.Z});
  S47J::Set(D,TEXT("radius"),R);S47J::Set(D,TEXT("beforeWalkable"),P->Walkable(Before.X,Before.Z));S47J::Set(D,TEXT("requestedWalkable"),P->Walkable(Requested.X,Requested.Z));S47J::Set(D,TEXT("actualWalkable"),P->Walkable(P->Pos.X,P->Pos.Z));S47J::Set(D,TEXT("actualClear"),CollisionClear(P->Pos));S47J::Set(D,TEXT("doorOpen"),M->Office->DoorOpen());
  TArray<FString> Overlaps,ContainingZones;TArray<TSharedPtr<FJsonValue>> OfficeZones,ColliderBounds;
  for(const FS47Rect& C:M->Web->Colliders())if(C.bEnabled)
  {
   const bool Hit=P->Pos.X+R>C.MinX+E&&P->Pos.X-R<C.MaxX-E&&P->Pos.Z+R>C.MinZ+E&&P->Pos.Z-R<C.MaxZ-E;
   if(Hit)Overlaps.Add(C.Id);
   if(Hit||C.Id==TEXT("office:doorCollider")){auto B=S47J::New();S47J::Set(B,TEXT("id"),C.Id);S47J::Set(B,TEXT("expandedBounds"),TArray<double>{C.MinX-R,C.MaxX+R,C.MinZ-R,C.MaxZ+R});ColliderBounds.Add(MakeShared<FJsonValueObject>(B));}
  }
  for(const FS47Rect& Z:M->Web->Zones())
  {
   if(Z.bEnabled&&P->Pos.X>=Z.MinX+R-1e-6&&P->Pos.X<=Z.MaxX-R+1e-6&&P->Pos.Z>=Z.MinZ+R-1e-6&&P->Pos.Z<=Z.MaxZ-R+1e-6)ContainingZones.Add(Z.Id);
   if(Z.Id.StartsWith(TEXT("office:"))){auto B=S47J::New();S47J::Set(B,TEXT("id"),Z.Id);S47J::Set(B,TEXT("enabled"),Z.bEnabled);S47J::Set(B,TEXT("centreBounds"),TArray<double>{Z.MinX+R,Z.MaxX-R,Z.MinZ+R,Z.MaxZ-R});OfficeZones.Add(MakeShared<FJsonValueObject>(B));}
  }
  S47J::Set(D,TEXT("overlappingColliderIds"),Overlaps);S47J::Set(D,TEXT("containingZoneIds"),ContainingZones);D->SetArrayField(TEXT("officeZones"),OfficeZones);D->SetArrayField(TEXT("contactOrOverlappingColliders"),ColliderBounds);
  UE_LOG(LogS47DaleDocs,Display,TEXT("S47: closed office crossing actual geometry: %s"),*S47J::Write(D,false));
  if(!Check(CollisionClear(P->Pos)&&P->Pos.Z<6.9,TEXT("closed office door still blocks the explicit crossing fixture")))return;
  if(!Check(M->Office->Use(TEXT("office:door")),TEXT("normal latch reopens the same office door")))return;
  Next(206,.8);return;
 }
 if(Stage==206)
 {
  P->Place(.45,6.2,0);P->MoveBy(0,1.4);
  if(!Check(M->Office->DoorOpen()&&CollisionClear(P->Pos)&&P->Pos.Z>7.5,TEXT("reopened office door admits the same crossing without collider bypass")))return;
  if(!Check(Stores==2&&PhysicalPapers.Num()==6&&CollisionMoves.Num()==8,TEXT("complete bounded collision cohort: six physical papers, eight motion probes and two disk/Flow restores")))return;
  auto R=S47J::New();S47J::Set(R,TEXT("status"),TEXT("PASS"));S47J::Set(R,TEXT("checks"),Checks);S47J::Set(R,TEXT("freshDiskFlowRestores"),Stores);R->SetArrayField(TEXT("collisionMoves"),CollisionMoves);R->SetArrayField(TEXT("physicalDeskPapers"),PhysicalPapers);S47J::Set(R,TEXT("closedDoorProbe"),CollisionDoorProbe);
  S47J::Set(R,TEXT("scope"),TEXT("COLLISION-01: original key/door/native entry and six physical paper rays; explicit ground/air movement fixtures call production MoveBy with all-collider endpoint/bounded-motion/order oracles; actual Jump/Tick/landing; one temporary rotated-vehicle geometry fixture; two isolated Writer/fresh-reader/full-Flow saves (valid and overlapping current layout), preserved office facts/view, local recovery and retreat; closed/reopened door. Fixture placement is explicit; no OS-input, exact user-save reproduction or full-matrix claim."));
  if(!FFileHelper::SaveStringToFile(S47J::Write(R,true),*(Root/TEXT("result.json")))){Fail(TEXT("collision result write failed"));return;}
  bDone=true;UE_LOG(LogS47DaleDocs,Display,TEXT("S47: Dale collision contract PASS checks=%d diskRestores=%d root=%s"),Checks,Stores,*Root);FPlatformMisc::RequestExit(false);return;
 }
}
void FS47DaleDocumentsTest::Step(double Dt)
{
 if (!bEnabled || bDone || !M || !M->Office || !M->Evening || !M->Flow || !M->Story || !M->World || !M->Player || !M->UI || !M->Inter) return;
 Elapsed += FMath::Clamp(Dt,0.,.2);
 if (Elapsed > 180) { Fail(TEXT("180 s timeout")); return; }
 if (M->Flow->bStarting || M->World->bBusy) return;
 if (Hold > 0) { Hold -= Dt; return; }
 if (!M->Flow->bDevRun || M->Flow->CaseId != 0) { Fail(TEXT("developer case zero required; no player save may be changed")); return; }
 if (!PendingRestore.IsEmpty() && !FinishRestore()) return;
 if(bCollision&&Stage>=200){CollisionStep();return;}
 auto Document = [this]() -> TSharedPtr<SS47Document>
 {
  const TSharedPtr<SWidget> Widget=M->Hud?M->Hud->ModalWidget():nullptr;
  return Widget.IsValid()&&Widget->GetTypeAsString()==TEXT("SS47Document")?StaticCastSharedPtr<SS47Document>(Widget):nullptr;
 };
 auto SendKey = [this](const FKey& Key, bool bRepeat = false)
 {
  if (!FSlateApplication::IsInitialized()) { Fail(TEXT("Slate keyboard routing is unavailable")); return false; }
  return Check(FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(Key,FModifierKeysState(),0,bRepeat,0,0)), TEXT("actual focused Slate document handles key ")+Key.GetFName().ToString());
 };
 if (Stage == 0)
 {
  FString UserDir, Jump; FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir); FParse::Value(FCommandLine::Get(), TEXT("S47Jump="), Jump);
  UserDir = FPaths::ConvertRelativePathToFull(UserDir); FPaths::CollapseRelativeDirectories(UserDir);
  if (!(UserDir.StartsWith(TEXT("/tmp/")) || UserDir.StartsWith(TEXT("/var/tmp/"))) || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()), UserDir) || Jump != TEXT("chapter2"))
  { Fail(TEXT("explicit isolated /tmp UserDir and S47Jump=chapter2 required")); return; }
  for (const TCHAR* Flag : {TEXT("S47T01Contract"),TEXT("S47T02Contract"),TEXT("S47T02DriveContract"),TEXT("S47T02UnresolvedContract"),TEXT("S47SmokeTest")})
   if (FParse::Param(FCommandLine::Get(),Flag)) { Fail(TEXT("do not combine independent live drivers")); return; }
  if(bCollision)for(const TCHAR* Flag:{TEXT("S47Walk"),TEXT("S47Hop"),TEXT("S47Drive")})
  {FString Value;if(FParse::Param(FCommandLine::Get(),Flag)||FParse::Value(FCommandLine::Get(),*(FString(Flag)+TEXT("=")),Value)){Fail(TEXT("collision contract cannot share another movement driver"));return;}}
  Root = FPaths::ProjectSavedDir() / (bCollision?TEXT("DaleCollisionContract"):TEXT("DaleDocumentsContract")) / FGuid::NewGuid().ToString(EGuidFormats::Digits); IFileManager::Get().MakeDirectory(*Root,true);
  auto* C2 = Cast<US47Chapter2>(M->Story->Chapter(TEXT("ch2"))); if (!C2) { Fail(TEXT("chapter-two fixture missing")); return; }
  M->UI->Close(true); M->Story->SetCinematic(false); M->Story->ClearTimers();
  C2->bRinging=false; C2->S.bAnswered=true; C2->S.bDoor=true; C2->S.bEntered=true; C2->S.Stage=TEXT("records");
  M->Player->Place(.45,5.75,0);
  if (!Check(M->Office->Ready() && !M->Office->HasKey() && !M->Office->RouteRequired(),TEXT("legacy chapter fixture has physical locked office, optional new route"))) return;
  if (!Check(!M->Office->ReadLibraryDocument(TEXT("EV_DALE_SKYLOG")),TEXT("locked/outside office cannot read a registered private document"))) return;
  if (!Check(M->Office->Use(TEXT("office:drawer")),TEXT("actual service drawer begins opening"))) return;
  Next(1,.65); return;
 }
 if (Stage == 1)
 {
  if (!Check(M->Office->Use(TEXT("office:key")) && M->Office->HasKey() && M->Office->Use(TEXT("office:door")),TEXT("actual spare and office latch used"))) return;
  Next(2,.8); return;
 }
 if (Stage == 2)
 {
  if (!Check(M->Office->DoorOpen(),TEXT("physical office door finishes opening"))) return;
  Points = {FVector(.45,0,6.2),FVector(.45,0,6.6),FVector(.45,0,7.0),FVector(.45,0,7.4),FVector(.30,0,8.1),FVector(.20,0,8.65)};
  WalkIndex=0; LastWalk=M->Player->Pos; LastProgress=Elapsed; Next(3,0); return;
 }
 if (Stage == 3)
 {
  const FVector At=M->Player->Pos; const double Moved=FVector2D(At.X-LastWalk.X,At.Z-LastWalk.Z).Size();
  if (Moved > .5) { Fail(TEXT("office entry jumped beyond a native walking step")); return; }
  WalkMetres+=Moved; LastWalk=At; if(Moved>.002)LastProgress=Elapsed;
  if(WalkIndex<Points.Num())
  {
   const FVector D=Points[WalkIndex]-At;const double Distance=FVector2D(D.X,D.Z).Size();
   if(Distance<.11){++WalkIndex;M->Player->TestMove.Reset();return;}
   if(Elapsed-LastProgress>4){Fail(TEXT("native office walk blocked"));return;}
   M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=-.12;M->Player->TestMove=FVector2D(0,-FMath::Min(1.,Distance*2));return;
  }
  M->Player->TestMove.Reset();
  if(!Check(M->Office->Contains(At)&&WalkMetres>2.5,TEXT("native Walker/Resolve enters real room without placement")))return;
  if(!AuditArt(TEXT("entered-before-desk-papers")) || !OfficeFacts(M->Office->Save(),0,TEXT("before physical desk reading")))return;
  DeskPaperIndex=0;DeskWalkStart=WalkMetres;LastProgress=Elapsed;Next(30,0);return;
 }
 if(Stage==30)
 {
  const FVector At=M->Player->Pos;const double Moved=FVector2D(At.X-LastWalk.X,At.Z-LastWalk.Z).Size();
  if(Moved>.5){Fail(TEXT("desk-paper walk jumped beyond a native walking step"));return;}
  WalkMetres+=Moved;LastWalk=At;if(Moved>.002)LastProgress=Elapsed;
  const FDeskPaper& Paper=DeskPapers[DeskPaperIndex];const FVector Target(Paper.X,At.Y,Paper.Z);
  if(!CollisionClear(Target)){Fail(FString(TEXT("paper approach overlaps active geometry: "))+Paper.Use);return;}
  const FVector D=Target-At;
  const double Distance=FVector2D(D.X,D.Z).Size();
  if(Distance>=.08)
  {
   if(Elapsed-LastProgress>4){Fail(FString(TEXT("native desk-paper approach blocked: "))+Paper.Use);return;}
   M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=-.12;M->Player->TestMove=FVector2D(0,-FMath::Min(1.,Distance*2));return;
  }
  M->Player->TestMove.Reset();
  if(!Check(M->Office->Contains(At)&&CollisionClear(At)&&WalkMetres-DeskWalkStart>.08,
            FString(TEXT("native Walker reaches separate desk-paper approach: "))+Paper.Use))return;
  const FS47Obj Obj=M->Office->Object(Paper.Use);const FBox Bounds=Obj.WorldBox();
  if(!Check(Obj&&Bounds.IsValid,FString(TEXT("actual target paper exists: "))+Paper.Use))return;
  const FVector Aim=Bounds.GetCenter()-S47Three::ToThree(M->Player->EyeUE());
  M->Player->Yaw=FMath::Atan2(-Aim.X,-Aim.Z);M->Player->Pitch=FMath::Atan2(Aim.Y,FVector2D(Aim.X,Aim.Z).Size());
  Next(31,.25);return;
 }
 if(Stage==31)
 {
  const FDeskPaper& Paper=DeskPapers[DeskPaperIndex];const FVector Eye=M->Player->EyeUE(),Forward=M->Player->ForwardUE();
  const FString Picked=M->Inter->Update(Eye,Forward);
  PaperReceipt=S47J::New();S47J::Set(PaperReceipt,TEXT("interaction"),Paper.Use);S47J::Set(PaperReceipt,TEXT("expectedDocument"),Paper.Doc);
  S47J::Set(PaperReceipt,TEXT("picked"),Picked);S47J::Set(PaperReceipt,TEXT("playerFocused"),M->Player->FocusedId());
  S47J::Set(PaperReceipt,TEXT("positionThree"),TArray<double>{M->Player->Pos.X,M->Player->Pos.Y,M->Player->Pos.Z});
  S47J::Set(PaperReceipt,TEXT("eyeUE"),TArray<double>{Eye.X,Eye.Y,Eye.Z});S47J::Set(PaperReceipt,TEXT("forwardUE"),TArray<double>{Forward.X,Forward.Y,Forward.Z});
  S47J::Set(PaperReceipt,TEXT("approachWalkMetres"),WalkMetres-DeskWalkStart);
  const bool Good=Picked==Paper.Use&&M->Player->FocusedId()==Paper.Use&&!M->UI->Modal()&&bool(M->Player->OnUse);
  if(!Good){PhysicalPapers.Add(MakeShared<FJsonValueObject>(PaperReceipt));Fail(FString::Printf(TEXT("actual paper camera Pick/Focus expected %s, picked '%s', focus '%s'"),Paper.Use,*Picked,*M->Player->FocusedId()));return;}
  if(!Check(true,FString(TEXT("actual camera ray and focused normal Use: "))+Paper.Use))return;
  M->Player->OnUse(Picked);Next(32,.4);return;
 }
 if(Stage==32)
 {
  const FDeskPaper& Paper=DeskPapers[DeskPaperIndex];const auto Widget=Document();
  const FS47Doc* Retained=M->Story->Docs().FindByPredicate([&](const FS47Doc& Doc){return Doc.Id==Paper.Doc;});
  const FS47EveningDocument* Source=M->Evening->Pack.FindDocument(Paper.Doc);
  if(!Check(Widget.IsValid()&&M->UI->Modal()&&Retained&&Source&&Retained->Title==Source->Title
            &&Retained->Page==FString::Join(Source->Body,TEXT("\n\n"))&&!Retained->Page.IsEmpty(),
            FString(TEXT("normal Use opens actual registered document modal: "))+Paper.Use))return;
  TArray<FString> Text;ModalText(Widget.ToSharedRef(),Text);TArray<FString> Lines;Retained->Page.ParseIntoArray(Lines,TEXT("\n"),false);
  bool bBody=true;int32 BodyLines=0;
  for(const FString& Line:Lines)
  {
   if(Line.TrimStartAndEnd().IsEmpty()||Line.TrimStart().StartsWith(TEXT("- - -")))continue;
   ++BodyLines;bBody&=Text.Contains(Line.StartsWith(TEXT("~ "))?Line.Mid(2):Line);
  }
  if(!Check(Text.Contains(Retained->Title)&&bBody&&BodyLines>0,FString(TEXT("actual Slate title and all paper body lines match document: "))+Paper.Doc))return;
  if(!OfficeFacts(M->Office->Save(),DeskPaperIndex+1,FString(TEXT("after actual Use "))+Paper.Use))return;
  S47J::Set(PaperReceipt,TEXT("modalTitle"),Retained->Title);S47J::Set(PaperReceipt,TEXT("actualSlateBodyLines"),BodyLines);
  S47J::Set(PaperReceipt,TEXT("officeAfterUse"),M->Office->Save());S47J::Set(PaperReceipt,TEXT("status"),TEXT("PASS"));PhysicalPapers.Add(MakeShared<FJsonValueObject>(PaperReceipt));
  Shot(FString::Printf(TEXT("desk-paper-%02d"),DeskPaperIndex));Next(33,.35);return;
 }
 if(Stage==33)
 {
  if(!SendKey(EKeys::Escape))return;Next(34,.2);return;
 }
 if(Stage==34)
 {
  if(!Check(!M->UI->Modal(),TEXT("focused Escape closes physical desk-paper modal")))return;
  ++DeskPaperIndex;
  if(DeskPaperIndex<UE_ARRAY_COUNT(DeskPapers)){DeskWalkStart=WalkMetres;LastWalk=M->Player->Pos;LastProgress=Elapsed;Next(30,0);return;}
  if(!Check(M->Office->HasFieldSheet(),TEXT("physical sixth paper retains real T02 field sheet")))return;
  Next(bCollision?200:35,0);return;
 }
 if(Stage==35)
 {
  for(const TCHAR* Id : {TEXT("office:private"),TEXT("office:arrayService")})
  {
   const FS47Obj Obj=M->Office->Object(Id);const FVector Eye=M->Player->EyeUE();const FVector Aim=(S47Three::ToUE(Obj.WorldBox().GetCenter())-Eye).GetSafeNormal();
   if(!Check(M->Inter->Pick(Eye,Aim)==Id,FString(TEXT("separate physical paper is independently ray-pickable: "))+Id))return;
  }
  if(!Check(M->Office->Use(TEXT("office:private"))&&M->UI->Modal(),TEXT("real private folder opens its document list")))return;
  for(const FString& Id:FS47Office::LibraryDocumentIds()) if(!Check(!M->Office->DocumentRead(Id),TEXT("browsing leaves unread: ")+Id))return;
  Shot(TEXT("private-folder-unread"));Next(4,.5);return;
 }
 if(Stage==4)
 {
  M->UI->Close(true);
  if(!StoreRestore(TEXT("unread-menu"),M->Flow->GameState()))return;
  Next(5,.6);return;
 }
 if(Stage==5)
 {
  if(!Check(M->Office->HasFieldSheet()&&!M->Evening->Pack.Flag(TEXT("vhs_seen"))&&!M->Office->DocumentRead(TEXT("EV_DALE_SKYLOG")),TEXT("Continue preserves field route without reading lore or film")))return;
  if(!Check(M->Office->ReadLibraryDocument(TEXT("EV_DALE_SKYLOG"))&&M->UI->Modal()&&M->Evening->Pack.Flag(TEXT("office_skylog_read")),TEXT("opening actual sky log commits just that read")))return;
  Next(6,.6);return;
 }
 if(Stage==6)
 {
  const TSharedPtr<SWidget> Modal=M->Hud?M->Hud->ModalWidget():nullptr;
  if(!Check(Modal.IsValid()&&Modal->GetTypeAsString()==TEXT("SS47Document")&&StaticCastSharedPtr<SS47Document>(Modal)->DocumentTextScale()==1.f,TEXT("actual normal document widget uses scale one")))return;
  Shot(TEXT("skylog-normal"));Next(7,.4);return;
 }
 if(Stage==7)
 {
  M->UI->Close(true); ActualRead=S47J::Clone(M->Flow->GameState());
  if(!StoreRestore(TEXT("partial-one-read"),ActualRead))return;
  Next(8,.6);return;
 }
 if(Stage==8)
 {
  if(!Check(M->Office->DocumentRead(TEXT("EV_DALE_SKYLOG"))&&!M->Office->DocumentRead(TEXT("EV_DALE_WITNESS"))&&M->Evening->Pack.Flag(TEXT("office_skylog_read"))&&!M->Evening->Pack.Flag(TEXT("office_witness_read")),TEXT("full Continue preserves one read and leaves the next page unread")))return;
  FS47Doc Empty;Empty.Id=TEXT("EV_DALE_SKYLOG");Empty.Title=TEXT("Isolated empty retained-copy fixture");M->Story->AddDoc(Empty);
  if(!Check(M->Office->ReadLibraryDocument(TEXT("EV_DALE_SKYLOG")),TEXT("actual private-page opening replaces an isolated blank retained copy")))return;
  const auto* Recovered=M->Story->Docs().FindByPredicate([](const FS47Doc& D){return D.Id==TEXT("EV_DALE_SKYLOG");});
  const auto* Source=M->Evening->Pack.FindDocument(TEXT("EV_DALE_SKYLOG"));
  if(!Check(Recovered&&Source&&Recovered->Page==FString::Join(Source->Body,TEXT("\n\n"))&&Recovered->Title==Source->Title,TEXT("read credit follows actual registered text, never the blank notebook-copy fixture")))return;
  M->UI->Close(true);
  auto Legacy=S47J::Clone(ActualRead);S47J::Obj(Legacy,TEXT("office"))->RemoveField(TEXT("readDocuments"));
  if(!StoreRestore(TEXT("legacy-no-read-list"),Legacy))return;
  Next(9,.6);return;
 }
 if(Stage==9)
 {
  if(!Check(!M->Office->DocumentRead(TEXT("EV_DALE_SKYLOG"))&&!M->Evening->Pack.Flag(TEXT("office_skylog_read"))&&M->Office->HasFieldSheet(),TEXT("legacy optional-read omission removes read credit without revoking T02 sheet")))return;
  if(!StoreRestore(TEXT("genuine-partial-restored"),ActualRead))return;
  ReadIndex=0;Next(10,.6);return;
 }
 if(Stage==10)
 {
  const auto& Ids=FS47Office::LibraryDocumentIds();
  if(ReadIndex>=Ids.Num()){Next(14);return;}
  M->UI->Close(true);M->UI->bLargeText=true;
  if(!Check(M->Office->ReadLibraryDocument(Ids[ReadIndex])&&M->UI->Modal(),TEXT("actual scaled document opened: ")+Ids[ReadIndex]))return;
  const TSharedPtr<SWidget> Modal=M->Hud?M->Hud->ModalWidget():nullptr;
  if(!Check(Modal.IsValid()&&Modal->GetTypeAsString()==TEXT("SS47Document")&&FMath::IsNearlyEqual(StaticCastSharedPtr<SS47Document>(Modal)->DocumentTextScale(),1.35f),TEXT("actual document widget carries larger text scale: ")+Ids[ReadIndex]))return;
  const auto* Doc=M->Story->Docs().FindByPredicate([&](const FS47Doc& D){return D.Id==Ids[ReadIndex];});
  if(!Check(Doc&&Doc->Page.Len()>180&&!Doc->Transcript.IsEmpty()&&Doc->Image.IsEmpty(),TEXT("real text page retained, without invented photograph: ")+Ids[ReadIndex]))return;
  Next(11,.6);return;
 }
 if(Stage==11){Shot(FString::Printf(TEXT("document-%02d-large"),ReadIndex));Next(12,.4);return;}
 if(Stage==12){ScrollKeys=0;Next(120);return;}
 if(Stage==120)
 {
  const auto W=Document();if(!Check(W.IsValid(),TEXT("keyboard scrolling targets the actual document widget")))return;
  PreviousScroll=W->DocumentScrollOffset();
  if(++ScrollKeys>24){Fail(TEXT("PageDown failed to reach the actual end within 24 keys"));return;}
  if(!SendKey(EKeys::PageDown))return;++TotalScrollKeys;Next(121,.2);return;
 }
 if(Stage==121)
 {
  const auto W=Document();if(!W.IsValid()){Fail(TEXT("document disappeared while scrolling"));return;}
  const float Offset=W->DocumentScrollOffset(),End=W->DocumentScrollEnd();
  if(!Check(Offset>=PreviousScroll-.5f&&Offset<=End+1.f,TEXT("native scroll stays monotonic and within actual content bounds")))return;
  if(Offset+1.f<End)
  {
   if(Offset<=PreviousScroll+.01f){Fail(TEXT("PageDown was handled without advancing the long paper"));return;}
   Next(120,.05);return;
  }
  if(!Check(FMath::Abs(Offset-End)<=1.f,TEXT("focused keyboard reaches last paragraph at actual scroll end")))return;
  Shot(FString::Printf(TEXT("document-%02d-large-bottom"),ReadIndex));Next(122,.4);return;
 }
 if(Stage==122)
 {
  const auto W=Document();if(!W.IsValid()){Fail(TEXT("document missing before PageUp"));return;}
  PreviousScroll=W->DocumentScrollOffset();
  // Held/repeated PageDown at an already visible end must not store invisible
  // overshoot that would swallow the first reverse key.
  for(int32 Repeat=0;Repeat<3;++Repeat){if(!SendKey(EKeys::PageDown,true))return;++TotalScrollKeys;}
  Next(1221,.2);return;
 }
 if(Stage==1221)
 {
  const auto W=Document();
  if(!Check(W.IsValid()&&FMath::Abs(W->DocumentScrollOffset()-PreviousScroll)<=1.f,TEXT("repeated PageDown remains at the actual last paragraph")))return;
  if(!SendKey(EKeys::PageUp))return;++TotalScrollKeys;Next(123,.2);return;
 }
 if(Stage==123)
 {
  const auto W=Document();if(!Check(W.IsValid()&&(PreviousScroll<=1.f?W->DocumentScrollOffset()<=1.f:W->DocumentScrollOffset()<PreviousScroll-.5f),TEXT("PageUp returns through the same real scroll surface")))return;
  if(!SendKey(EKeys::Escape))return;Next(124,.2);return;
 }
 if(Stage==124)
 {
  if(!Check(!M->UI->Modal(),TEXT("focused Escape closes the document without a mouse")))return;
  if(bLegacyTyped){Next(141);return;}
  ++ReadIndex;Next(10);return;
 }
 if(Stage==14)
 {
  M->UI->Close(true);
  for(const FString& Id:FS47Office::LibraryDocumentIds())if(!Check(M->Office->DocumentRead(Id),TEXT("independent page read retained: ")+Id))return;
  if(!Check(!M->Evening->Pack.Flag(TEXT("vhs_seen"))&&!M->Evening->Pack.Flag(TEXT("trafo_anomaly_checked"))&&M->Office->HasFieldSheet(),TEXT("all optional reading completes neither film nor array/T02 experiment")))return;
  if(!Check(M->Player->Walkable(-8,-74)&&M->Player->Walkable(18,-74)&&M->Player->Walkable(-1,-74),TEXT("ordinary A/B/reference coordinates lie on native walkable Row One")))return;
  const auto* Existing=M->Story->Docs().FindByPredicate([](const FS47Doc& D){return D.Id==TEXT("EV_SERVICE_LOG");});
  if(!Check(Existing&&Existing->Kind==TEXT("typed"),TEXT("existing ordinary typed service-log document is retained")))return;
  M->UI->bLargeText=true;M->UI->Document(*Existing);bLegacyTyped=true;ReadIndex=7;Next(140,.6);return;
 }
 if(Stage==140)
 {
  const auto W=Document();
  if(!Check(W.IsValid()&&FMath::IsNearlyEqual(W->DocumentTextScale(),1.35f),TEXT("ordinary existing typed document uses actual large-text widget scale")))return;
  Shot(TEXT("service-log-large-top"));Next(12,.4);return;
 }
 if(Stage==141)
 {
  if(!StoreRestore(TEXT("all-documents-read"),M->Flow->GameState()))return;
  Next(15,.6);return;
 }
 if(Stage==15)
 {
  for(const FString& Id:FS47Office::LibraryDocumentIds())if(!Check(M->Office->DocumentRead(Id),TEXT("full Flow restores actually read page: ")+Id))return;
  if(!Check(!M->Office->ReadLibraryDocument(TEXT("VHS_DALE_FIELD_01")),TEXT("unproduced film cannot be read as an optional paper")))return;
  if(!Check(DeskPaperIndex==UE_ARRAY_COUNT(DeskPapers)&&PhysicalPapers.Num()==6&&Stores==5&&ArtAudits==11,
            TEXT("six physical papers and art/facts through the original five disk/Flow cohorts")))return;
  auto R=S47J::New();S47J::Set(R,TEXT("status"),TEXT("PASS"));S47J::Set(R,TEXT("checks"),Checks);S47J::Set(R,TEXT("freshDiskFlowRestores"),Stores);S47J::Set(R,TEXT("nativeOfficeWalkMetres"),WalkMetres);S47J::Set(R,TEXT("actualSlateScrollKeys"),TotalScrollKeys);
  R->SetArrayField(TEXT("physicalDeskPapers"),PhysicalPapers);S47J::Set(R,TEXT("documentArtAudits"),ArtAudits);
  S47J::Set(R,TEXT("scope"),TEXT("API-driven native Walker entry and six distinct desk approaches; actual camera ray/Focus and ordinary Player OnUse for all six art papers; registered document and actual Slate title/body; focused PageDown/PageUp/Escape through private-library content end and existing typed document at large scale; original five isolated Writer/fresh-reader/full-Flow cohorts with source/disk/restored office facts and actual art audits. Initial chapter prerequisites and corridor start are explicit fixtures; no human-input, listening, FPS, VHS, photos, ARRAY-01 encounter, full-matrix, installation or release claim."));
  if(!FFileHelper::SaveStringToFile(S47J::Write(R,true),*(Root/TEXT("result.json")))){Fail(TEXT("final document result could not be written"));return;}bDone=true;
  UE_LOG(LogS47DaleDocs,Display,TEXT("S47: Dale documents contract PASS checks=%d diskRestores=%d root=%s"),Checks,Stores,*Root);FPlatformMisc::RequestExit(false);
 }
}

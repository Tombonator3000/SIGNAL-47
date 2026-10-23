#include "S47Evening.h"

#include "S47EveningVisual.h"
#include "S47FieldCamera.h"
#include "S47Flashlight.h"
#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47RecordedDialogue.h"
#include "S47Story.h"
#include "S47Takeaway.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

// Fixture/camera setup and conversation choices use the API. Every food, phone and
// document interaction below first resolves the normal physical pick ray, then calls
// the same walker OnUse -> HUD -> Evening -> Interaction chain as the use key.
struct FS47Evening::FTakeawayTest
{
 bool bEnabled = false, bDone = false, bCameraHave = false, bLightHave = false, bLightOn = false, bEatPlaced = false, bMealReloaded = false;
 int32 Stage = 0, TraceAttempts = 0, Uses = 0, Restores = 0, ExtraSpot = 0;
 double Elapsed = 0, Since = 0, Hold = 2, LastClock = 0, MealClock = 0;
 FString Shots, TargetId;
 FVector TargetPoint = FVector::ZeroVector;
 TArray<FString> Captures;
};

void FS47Evening::ControlTakeawayTest(double Dt)
{
 if (!TakeawayTest)
 {
  TakeawayTest = MakeShared<FTakeawayTest>();
  TakeawayTest->bEnabled = FParse::Param(FCommandLine::Get(), TEXT("S47EveningTakeaway"));
  if (!TakeawayTest->bEnabled) return;
  TakeawayTest->bEatPlaced = FParse::Param(FCommandLine::Get(), TEXT("S47TakeawayEatPlaced"));
  FParse::Value(FCommandLine::Get(), TEXT("S47EveningShots="), TakeawayTest->Shots);
  if (!TakeawayTest->Shots.IsEmpty()) IFileManager::Get().MakeDirectory(*TakeawayTest->Shots, true);
 }
 FTakeawayTest& T = *TakeawayTest;
 if (!T.bEnabled || T.bDone) return;
 const auto Fail = [this, &T](const FString& Why)
 {
  T.bDone = true;
  if (M && M->Player) M->Player->TestMove.Reset();
  UE_LOG(LogTemp, Error, TEXT("S47: takeaway FAIL stage %d at %.3f %.3f: %s"), T.Stage, M && M->Player ? M->Player->Pos.X : 0, M && M->Player ? M->Player->Pos.Z : 0, *Why);
  FPlatformMisc::RequestExit(false);
 };
 if (!M || !M->Player || !M->World || !M->Story || !M->Flow || !M->Web || !M->Inter || !M->UI || !Takeaway) { Fail(TEXT("missing runtime dependency")); return; }
 if (!M->Flow->bDevRun || M->Flow->CaseId != 0) { Fail(TEXT("requires an isolated developer run without S47Case")); return; }
 T.Elapsed += Dt;
 if (T.Elapsed > 240) { Fail(TEXT("240 s control timeout")); return; }
 if (M->World->bBusy) return;
 if (T.Stage >= 10 && M->Story->Clock() + 0.001 < T.LastClock) { Fail(TEXT("clock moved backwards after arrival")); return; }
 T.LastClock = M->Story->Clock();
 if (T.Hold > 0) { T.Hold -= Dt; return; }
 const auto Stage = [&T](int32 Next, double Hold = 0.25)
 {
  T.Stage = Next; T.Since = T.Elapsed; T.Hold = Hold;
  UE_LOG(LogTemp, Display, TEXT("S47: takeaway stage %d at %.2f s"), Next, T.Elapsed);
 };
 const auto Shot = [this, &T](const FString& Name)
 {
  if (T.Shots.IsEmpty()) return;
  const FString Path = T.Shots / (TEXT("takeaway-") + Name + TEXT(".png")); T.Captures.AddUnique(Path);
  M->Story->After(0.1, [Path] { FScreenshotRequest::RequestScreenshot(Path, true, false); });
 };
 const auto Roundtrip = [this, &T, &Fail](const FString& Name)
 {
  const bool Held = Takeaway->Carried(), Eaten = Takeaway->Eaten(); const FString Spot = Takeaway->PlacedSpot();
  const FString Pending = Pack.GetPendingChoice(), Scene = Pack.CurrentScene() ? Pack.CurrentScene()->Id : FString();
  const int32 Cursor = Pack.NextLine(), Docs = Pack.Documents().Num(); const FVector Before = M->Player->Pos;
  const double Clock = M->Story->Clock(); const bool Bought = Pack.Flag(TEXT("food_bought")), Taken = Pack.Flag(TEXT("food_taken")), FoodEaten = Pack.Flag(TEXT("food_eaten"));
  const FString StoreDir = FPaths::ProjectSavedDir() / TEXT("TakeawayControls") / Name;
  FS47SaveStore Writer(StoreDir); Writer.Init(); FS47SaveCard Card;
  Card.Chapter = TEXT("Takeaway physical interaction control"); Card.Place = M->World->Area; Card.Clock = M->Story->ClockText(false);
  FString Error; const auto Meta = Writer.Save(1, TEXT("manual"), 0, M->Flow->GameState(), Card, Error);
  if (!Meta) { Fail(TEXT("disk save ") + Name + TEXT(": ") + Error); return false; }
  FS47SaveStore Reader(StoreDir); Reader.Init(); const auto Saved = Reader.Load(Meta->Id);
  if (!Saved) { Fail(TEXT("fresh disk load failed: ") + Name); return false; }
  if (!T.Shots.IsEmpty()) FFileHelper::SaveStringToFile(S47J::Write(Saved, true), *(T.Shots / (TEXT("takeaway-") + Name + TEXT(".disk-save.json"))));
  US47Flow::FStart Restore; Restore.State = Saved; Restore.CaseId = 0; Restore.bDev = true; Restore.Playtime = M->Playtime;
  M->Flow->StartGame(Restore);
  const FString RestoredScene = Pack.CurrentScene() ? Pack.CurrentScene()->Id : FString();
  if (Takeaway->Carried() != Held || Takeaway->Eaten() != Eaten || Takeaway->PlacedSpot() != Spot || !M->Player->Pos.Equals(Before, 0.015) || M->Story->Clock() != Clock || Pack.GetPendingChoice() != Pending || RestoredScene != Scene || Pack.NextLine() != Cursor || Pack.Documents().Num() != Docs || Pack.Flag(TEXT("food_bought")) != Bought || Pack.Flag(TEXT("food_taken")) != Taken || Pack.Flag(TEXT("food_eaten")) != FoodEaten)
  {
   Fail(FString::Printf(TEXT("disk/full-world restore changed %s: held %d/%d eaten %d/%d spot '%s'/'%s' pose %.3f,%.3f/%.3f,%.3f clock %.9f/%.9f pending '%s'/'%s' scene '%s'/'%s' cursor %d/%d"), *Name, Held, Takeaway->Carried(), Eaten, Takeaway->Eaten(), *Spot, *Takeaway->PlacedSpot(), Before.X, Before.Z, M->Player->Pos.X, M->Player->Pos.Z, Clock, M->Story->Clock(), *Pending, *Pack.GetPendingChoice(), *Scene, *RestoredScene, Cursor, Pack.NextLine())); return false;
  }
  ++T.Restores;
  UE_LOG(LogTemp, Display, TEXT("S47: takeaway PASS disk/fresh-store/full-world restore %s: one portion, exact location, flags, pending reply, cursor, documents, pose and clock retained"), *Name);
  return true;
 };
 const auto Prepare = [this, &T, &Fail](const FString& Id)
 {
  FS47Obj Obj;
  if (const auto* I = M->Inter->Get(Id)) Obj = I->Object;
  else if (const auto* I = M->Web->Item(Id)) Obj = M->Web->ObjAt(I->Node);
  if (!Obj.IsValid() || M->Inter->LabelOf(Id).IsEmpty()) { Fail(TEXT("target has no active physical interaction: ") + Id); return false; }
  const FBox Bounds = Obj.WorldBox(); const FVector Target = Bounds.IsValid ? Bounds.GetCenter() : Obj.WorldPosition();
  // Camera fixtures stand on normal walkable, collider-clear ground around the actual
  // physical target. A successful ray is found before asking the live camera to use it.
  for (double Radius : {0.95, 1.2, 1.5, 1.8}) for (int32 K = 0; K < 24; ++K)
  {
   const double Angle = K * UE_DOUBLE_PI / 12;
   const FVector P(Target.X + FMath::Cos(Angle) * Radius, M->Player->FloorY, Target.Z + FMath::Sin(Angle) * Radius);
   if (!M->Player->Walkable(P.X, P.Z)) continue;
   bool Blocked = false;
   for (const auto& C : M->Web->Colliders()) if (C.bEnabled && P.X + M->Player->Radius > C.MinX && P.X - M->Player->Radius < C.MaxX && P.Z + M->Player->Radius > C.MinZ && P.Z - M->Player->Radius < C.MaxZ) { Blocked = true; break; }
   if (Blocked) continue;
   const FVector Eye = P + FVector(0, M->Player->Eye, 0), D = Target - Eye;
   if (M->Inter->Pick(S47Three::ToUE(Eye), S47Three::ToUE(D).GetSafeNormal()) != Id) continue;
   M->Player->Place(P.X, P.Z, FMath::Atan2(-D.X, -D.Z));
   M->Player->Pitch = FMath::Atan2(D.Y, FMath::Sqrt(D.X * D.X + D.Z * D.Z));
   T.TargetId = Id; T.TargetPoint = Target; T.TraceAttempts = 0;
   return true;
  }
  Fail(TEXT("no walkable, collider-clear physical pick setup for target: ") + Id); return false;
 };
 const auto UsePicked = [this, &T, &Fail]()
 {
  const FString Picked = M->Inter->Pick(M->Player->EyeUE(), M->Player->ForwardUE());
  if (Picked != T.TargetId || M->Player->FocusedId() != T.TargetId)
  {
   if (++T.TraceAttempts < 4)
   {
    const FVector Eye = S47Three::ToThree(M->Player->EyeUE()), D = T.TargetPoint - Eye;
    M->Player->Yaw = FMath::Atan2(-D.X, -D.Z); M->Player->Pitch = FMath::Atan2(D.Y, FMath::Sqrt(D.X * D.X + D.Z * D.Z)); T.Hold = 0.2; return false;
   }
   Fail(FString::Printf(TEXT("live physical ray/focus missed %s: pick '%s', focus '%s', label '%s'"), *T.TargetId, *Picked, *M->Player->FocusedId(), *M->Inter->LabelOf(T.TargetId))); return false;
  }
  if (M->ModalOpen() || !M->Player->OnUse) { Fail(TEXT("normal use chain unavailable: ") + T.TargetId); return false; }
  M->Player->OnUse(Picked); ++T.Uses;
  UE_LOG(LogTemp, Display, TEXT("S47: takeaway PASS actual ray and focus -> normal walker/HUD/use/interaction chain: %s"), *Picked);
  return true;
 };
 const auto FinishLeaf = [this]()
 {
  if (Voice) Voice->Stop(); M->UI->Close(true); bDialogueOpen = false;
  while (Pack.CurrentLine()) Pack.Advance(); Changed();
 };
 const auto CheckOnePortion = [this, &Fail](const FString& Name)
 {
  if ((Takeaway->HeldVisible() && Takeaway->PlacedVisible()) || (Takeaway->Eaten() && (Takeaway->Carried() || !Takeaway->PlacedSpot().IsEmpty() || Takeaway->HeldVisible() || Takeaway->PlacedVisible()))) { Fail(TEXT("duplicate or reappeared portion: ") + Name); return false; }
  return true;
 };
 if (!CheckOnePortion(TEXT("per-frame invariant"))) return;
 switch (T.Stage)
 {
 case 0:
  if (!Commute()) { Fail(TEXT("requires -S47Evening=burger opening fixture")); return; }
  for (const FString& Id : TArray<FString>{TEXT("diner"), TEXT("desk"), TEXT("annex"), TEXT("hood"), TEXT("passenger"), TEXT("bench")})
  {
   if (!Takeaway->SpotIds().Contains(Id) || Takeaway->SpotArea(Id).IsEmpty()) { Fail(TEXT("authored placement registry missing: ") + Id); return; }
   for (const FString& Action : TArray<FString>{TEXT("place"), TEXT("pickup"), TEXT("eat")})
   {
    const auto* Target = M->Inter->Get(TEXT("takeaway:") + Action + TEXT(":") + Id);
    if (!Target || !Target->Object.IsValid()) { Fail(TEXT("authored placement target missing: ") + Action + TEXT(":") + Id); return; }
   }
  }
  if (Voice) Voice->Stop(); M->UI->Close(true); bDialogueOpen = false;
  Pack.Reset(); BootstrapTest(TEXT("order"));
  UE_LOG(LogTemp, Display, TEXT("S47: takeaway START: authored paid-order fixture, physical pick/use, optional single portion, all six placement spots, persistence and suppression controls; meal from %s, setup/API dialogue is not human input"), T.bEatPlaced ? TEXT("placed portion with committed mid-fade disk restore") : TEXT("held portion with uninterrupted short fade"));
  Stage(1); return;
 case 1:
  if (!Pack.CurrentScene() || Pack.CurrentScene()->Id != TEXT("I04_BURGER") || Pack.NextLine() != 0 || Pack.GetPendingChoice().IsEmpty() || !Pack.Flag(TEXT("food_bought")) || Pack.Flag(TEXT("food_taken"))) { Fail(TEXT("paid pending reply fixture missing")); return; }
  if (!Roundtrip(TEXT("paid-pending"))) return;
  Stage(2, 0.5); return;
 case 2:
  FinishLeaf();
  if (!Pack.Select(TEXT("I04_TAKE"))) { Fail(TEXT("authored takeaway choice unavailable")); return; }
  FinishLeaf(); AfterDialogue(); Stage(3, 0.5); return;
 case 3:
  if (!Takeaway->Carried() || Takeaway->Eaten() || !Pack.Flag(TEXT("food_taken")) || !Takeaway->HeldVisible()) { Fail(TEXT("completed take response did not create one carried visible bag")); return; }
  if (!Roundtrip(TEXT("held-diner"))) return; Shot(TEXT("held-diner"));
  Stage(4, 0.5); return;
 case 4: if (Prepare(TEXT("takeaway:place:diner"))) Stage(5); return;
 case 5: if (UsePicked()) Stage(6); return;
 case 6:
  if (Takeaway->Carried() || Takeaway->PlacedSpot() != TEXT("diner") || !Takeaway->PlacedVisible()) { Fail(TEXT("diner set-down did not retain one visible placed portion")); return; }
  if (!Roundtrip(TEXT("placed-diner"))) return; Shot(TEXT("diner-set-down"));
  Stage(7, 0.5); return;
 case 7: if (Prepare(TEXT("takeaway:pickup:diner"))) Stage(8); return;
 case 8: if (UsePicked()) Stage(148); return;
 case 148:
  if (!Takeaway->Carried()) { Fail(TEXT("diner pickup did not return the same portion to hand")); return; }
  if (Prepare(TEXT("takeaway:place:bench"))) Stage(149); return;
 case 149: if (UsePicked()) Stage(150); return;
 case 150:
  if (Takeaway->Carried() || Takeaway->PlacedSpot() != TEXT("bench") || !Takeaway->PlacedVisible()) { Fail(TEXT("authored diner bench placement failed")); return; }
  if (!Roundtrip(TEXT("placed-bench"))) return;
  Stage(151, 0.5); return;
 case 151: if (Prepare(TEXT("takeaway:pickup:bench"))) Stage(152); return;
 case 152: if (UsePicked()) Stage(153); return;
 case 153:
  if (!Takeaway->Carried() || !Takeaway->PlacedSpot().IsEmpty()) { Fail(TEXT("diner bench pickup failed to return the same portion")); return; }
  Stage(9); return;
 case 9:
  if (!Takeaway->Carried()) { Fail(TEXT("diner pickup did not return the same portion to hand")); return; }
  BootstrapTest(TEXT("parking")); Stage(10, 1); return;
 case 10:
  if (Takeaway->CanEat()) { Fail(TEXT("SARO meal must wait until the phone handover is complete")); return; }
  if (Prepare(TEXT("phone"))) Stage(11); return;
 case 11: if (UsePicked()) Stage(12); return;
 case 12:
  if (!M->ModalOpen() || !Takeaway->Carried() || Takeaway->HeldVisible() || !Pack.CurrentScene() || Pack.CurrentScene()->Id != TEXT("D01_HANDOVER")) { Fail(TEXT("carried bag blocked ordinary phone use or obscured its modal")); return; }
  Shot(TEXT("phone-with-bag-stowed")); Stage(13); return;
 case 13:
  FinishLeaf();
  if (const auto* Scene = Pack.CurrentScene())
  {
   const FString Id = Scene->Id == TEXT("D01_HANDOVER") ? TEXT("D01_CH_FOOD") : Scene->Id == TEXT("D01_FOOD") ? TEXT("D01_CH_FOOD_BRIEF") : FString();
   if (Id.IsEmpty() || !Pack.Select(Id)) { Fail(TEXT("unexpected handover choice")); return; }
   Stage(13, 0.05); return;
  }
  AfterDialogue();
  if (!Pack.Flag(TEXT("handover_done")) || Opening() || !Takeaway->Carried()) { Fail(TEXT("optional meal gated handover or consumed the bag")); return; }
  Stage(14, 0.5); return;
 case 14: if (Prepare(TEXT("workOrder"))) Stage(15); return;
 case 15: if (UsePicked()) Stage(16); return;
 case 16:
  if (!M->ModalOpen() || Takeaway->HeldVisible() || !Takeaway->Carried()) { Fail(TEXT("bag blocked existing document trace/use or remained visible over document")); return; }
  Shot(TEXT("document-with-bag-stowed")); Stage(17); return;
 case 17: M->UI->Close(true); Stage(18); return;
 case 18: if (Prepare(TEXT("takeaway:place:desk"))) Stage(19); return;
 case 19: if (UsePicked()) Stage(20); return;
 case 20:
  if (Takeaway->PlacedSpot() != TEXT("desk") || Takeaway->Carried() || !Takeaway->PlacedVisible()) { Fail(TEXT("SARO desk placement failed")); return; }
  if (!Roundtrip(TEXT("placed-desk"))) return; Shot(TEXT("desk-set-down"));
  Stage(21, 0.5); return;
 case 21: if (Prepare(TEXT("takeaway:pickup:desk"))) Stage(22); return;
 case 22: if (UsePicked()) Stage(23); return;
 case 23:
  M->Web->SetDoorOpen(TEXT("doorSouth"), true);
  if (Prepare(TEXT("takeaway:place:annex"))) Stage(24); return;
 case 24: if (UsePicked()) Stage(25); return;
 case 25:
  if (Takeaway->PlacedSpot() != TEXT("annex") || Takeaway->Carried() || !Takeaway->PlacedVisible()) { Fail(TEXT("SARO annex placement failed")); return; }
  if (!Roundtrip(TEXT("placed-annex"))) return; Shot(TEXT("annex-set-down"));
  Stage(26, 0.5); return;
 case 26: if (Prepare(TEXT("takeaway:pickup:annex"))) Stage(27); return;
 case 27: if (UsePicked()) Stage(28); return;
 case 28:
  if (!Takeaway->Carried() || !Takeaway->HeldVisible() || !M->FieldCamera || !M->Night || !M->Night->Flashlight) { Fail(TEXT("held portion or suppression dependencies unavailable")); return; }
  T.bCameraHave = M->FieldCamera->bHave; M->FieldCamera->bHave = true; M->FieldCamera->Raise(true); Stage(29); return;
 case 29:
  if (!Takeaway->Carried() || Takeaway->HeldVisible()) { Fail(TEXT("field camera did not suppress held food presentation")); return; }
  Shot(TEXT("camera-bag-stowed")); Stage(129); return;
 case 129:
  M->FieldCamera->Raise(false); M->FieldCamera->bHave = T.bCameraHave;
  T.bLightHave = M->Night->Flashlight->bHave; T.bLightOn = M->Night->Flashlight->bOn;
  M->Night->Flashlight->bHave = true; M->Night->Flashlight->bOn = true; Stage(30); return;
 case 30:
  if (!Takeaway->Carried() || Takeaway->HeldVisible()) { Fail(TEXT("flashlight did not suppress held food presentation")); return; }
  Shot(TEXT("flashlight-bag-stowed")); Stage(130); return;
 case 130:
  M->Night->Flashlight->bHave = T.bLightHave; M->Night->Flashlight->bOn = T.bLightOn;
  M->Flow->OpenNotebook(); Stage(31); return;
 case 31:
  if (!M->ModalOpen() || !Takeaway->Carried() || Takeaway->HeldVisible()) { Fail(TEXT("notebook did not suppress held food presentation")); return; }
  Shot(TEXT("notebook-bag-stowed")); Stage(32); return;
 case 32: M->UI->Close(true); Stage(33); return;
 case 33:
  if (!Takeaway->Carried() || !Takeaway->HeldVisible()) { Fail(TEXT("closing camera/light/notebook did not restore the same held portion")); return; }
  if (!Roundtrip(TEXT("held-saro"))) return;
  Stage(140, 0.5); return;
 case 140:
  if (T.ExtraSpot >= 2) { Stage(34); return; }
  if (Prepare(TEXT("takeaway:place:") + TArray<FString>{TEXT("hood"), TEXT("passenger")}[T.ExtraSpot])) Stage(141); return;
 case 141: if (UsePicked()) Stage(142); return;
 case 142:
  {
   const FString Spot = TArray<FString>{TEXT("hood"), TEXT("passenger")}[T.ExtraSpot];
   if (Takeaway->Carried() || Takeaway->PlacedSpot() != Spot || !Takeaway->PlacedVisible()) { Fail(TEXT("authored flat-surface placement failed: ") + Spot); return; }
   if (!Roundtrip(TEXT("placed-") + Spot)) return;
   Stage(143, 0.5); return;
  }
 case 143:
  if (Prepare(TEXT("takeaway:pickup:") + TArray<FString>{TEXT("hood"), TEXT("passenger")}[T.ExtraSpot])) Stage(144); return;
 case 144: if (UsePicked()) Stage(145); return;
 case 145:
  if (!Takeaway->Carried() || !Takeaway->PlacedSpot().IsEmpty()) { Fail(TEXT("flat-surface pickup failed to return the same portion")); return; }
  ++T.ExtraSpot; Stage(140); return;
 case 34:
  if (T.bEatPlaced) { if (Prepare(TEXT("takeaway:place:annex"))) Stage(134); return; }
  if (Prepare(TEXT("takeaway:eat:annex"))) Stage(35); return;
 case 134: if (UsePicked()) Stage(135); return;
 case 135:
  if (Takeaway->Carried() || Takeaway->PlacedSpot() != TEXT("annex")) { Fail(TEXT("placed-meal variant did not put the existing portion on its authored surface")); return; }
  if (Prepare(TEXT("takeaway:eat:annex"))) Stage(35); return;
 case 35:
  if (Takeaway->Carried() == T.bEatPlaced || (T.bEatPlaced && Takeaway->PlacedSpot() != TEXT("annex"))) { Fail(TEXT("meal source does not match held/placed test variant")); return; }
  T.MealClock = M->Story->Clock();
  if (UsePicked())
  {
   if (!Takeaway->Eaten() || !Pack.Flag(TEXT("food_eaten")) || Takeaway->Carried() || FMath::Abs(M->Story->Clock() - T.MealClock - 720) > 0.001) { Fail(TEXT("meal did not atomically consume one portion and skip exactly twelve minutes")); return; }
   if (T.bEatPlaced)
   {
    if (!Takeaway->MealBusy()) { Fail(TEXT("mid-meal save did not start during the short fade")); return; }
    if (!Roundtrip(TEXT("meal-committed-mid-fade"))) return;
    T.bMealReloaded = true;
   }
   Stage(36);
  }
  return;
 case 36:
  if (!Takeaway->Eaten() || !Pack.Flag(TEXT("food_eaten")) || Takeaway->Carried() || M->Story->Clock() - T.MealClock < 720) { Fail(TEXT("committed meal or its clock was rolled back during the fade")); return; }
  if (Takeaway->MealBusy())
  {
   if (T.Elapsed - T.Since > 8) Fail(TEXT("compressed meal fade did not finish within eight real seconds"));
   return;
  }
  if (!Takeaway->TraceVisible() || Takeaway->HeldVisible() || Takeaway->PlacedVisible()) { Fail(TEXT("finished meal must leave one wrapper/cup trace and no available portion")); return; }
  UE_LOG(LogTemp, Display, TEXT("S47: takeaway PASS optional %s meal through actual napkin target: one atomic twelve-minute skip, %.3f total story seconds forward, short fade complete, one aftermath trace, mid-fade restore %d"), T.bEatPlaced ? TEXT("placed") : TEXT("held"), M->Story->Clock() - T.MealClock, T.bMealReloaded);
  if (!Roundtrip(TEXT("eaten-annex"))) return;
  Stage(37, 0.5); return;
 case 37:
  if (!Takeaway->Eaten() || !Takeaway->TraceVisible() || Takeaway->Carried() || Takeaway->HeldVisible() || Takeaway->PlacedVisible() || !M->Inter->LabelOf(TEXT("takeaway:eat:annex")).IsEmpty() || !M->Inter->LabelOf(TEXT("takeaway:pickup:annex")).IsEmpty()) { Fail(TEXT("eaten save recreated food or offered duplicate meal/pickup")); return; }
  {
   const double Before = M->Story->Clock();
   if (Takeaway->Eat(TEXT("annex")) || M->Story->Clock() != Before) { Fail(TEXT("repeated meal after disk restore consumed or advanced time again")); return; }
  }
  for (const FString& Id : TArray<FString>{TEXT("EV_HANDOVER"), TEXT("EV_SERVICE_LOG"), TEXT("EV_RADIO_SCHEDULE")})
  {
   int32 Count = 0; for (const auto& Doc : M->Story->Docs()) if (Doc.Id == Id) ++Count;
   if (Count != 1) { Fail(TEXT("optional food duplicated or lost handover paper: ") + Id); return; }
  }
  if (!Pack.Flag(TEXT("handover_done")) || Opening() || M->Story->bOpeningHeld) { Fail(TEXT("optional meal changed the completed handover/story gate")); return; }
  Shot(TEXT("eaten-annex-trace")); Stage(38, 0.5); return;
 case 38:
  for (const FString& Path : T.Captures) if (IFileManager::Get().FileSize(*Path) < 1000) { Fail(TEXT("screenshot missing/empty: ") + Path); return; }
  if (T.Restores != 10 + int32(T.bEatPlaced) || T.Uses != 15 + int32(T.bEatPlaced) || T.bMealReloaded != T.bEatPlaced) { Fail(FString::Printf(TEXT("control count mismatch: restores %d/%d physical uses %d/%d mid-fade restore %d/%d"), T.Restores, 10 + int32(T.bEatPlaced), T.Uses, 15 + int32(T.bEatPlaced), T.bMealReloaded, T.bEatPlaced)); return; }
  T.bDone = true;
  UE_LOG(LogTemp, Display, TEXT("S47: takeaway PASS COMPLETE: physical diner/desk/annex/hood/passenger/bench set-down and pickup, ordinary phone and document use unobstructed, %s meal optional with exactly one twelve-minute skip, %d disk/fresh-store/full-world restores including paid reply/held/all placements/eaten, camera/light/notebook suppression and no duplicate portion; human input UNVERIFIED"), T.bEatPlaced ? TEXT("placed with committed mid-fade restore") : TEXT("held with uninterrupted fade"), T.Restores);
  FPlatformMisc::RequestExit(false); return;
 default: Fail(TEXT("unknown takeaway control stage")); return;
 }
}

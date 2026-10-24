// Original Blender diner, applied as one complete replacement after Props placement.
#include "S47Web.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Startup.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47MesaDiner, Log, All);

namespace
{
 const FVector Origin(7979.95, 4.309006389843889, 1999.95);
 const TCHAR* Parts[] = {TEXT("Shell"),TEXT("Glass"),TEXT("Door"),TEXT("Interior"),TEXT("Booths"),TEXT("Counter"),TEXT("Kitchen"),TEXT("Fixtures"),TEXT("Truck"),TEXT("Sign"),TEXT("Parking")};
 const TCHAR* Proxies[] = {TEXT("menu"),TEXT("door"),TEXT("window"),TEXT("counter"),TEXT("coffee"),TEXT("booth"),TEXT("jukebox"),TEXT("clipping"),TEXT("payphone"),TEXT("waitress"),TEXT("driver"),TEXT("sign"),TEXT("rig")};
 // Removed dressing and four old sign feet. The new sign has one actual central footing.
 // The replacement bench, walls, furniture, rig and movable player truck retain their boxes.
 const double Obsolete[][4] = {{3.04,3.46,1.22,1.68},{2.16,2.96,7.83,8.77},{3,3.6,-1.75,-1.15},
  {-6.15,-.65,10.7,12.7},{-5.997541916668,-.002458083333,-12.7711575662533,-10.4288424337467},
  {-7.5,-6.9,9.45,10.05},{-6.8,-6.2,9.5,10.1},{-8.9,-8.3,7.9,8.5},
  {12.8,13.2,-12.17,-11.77},{12.8,13.2,-8.23,-7.83},{7.33,7.73,-21.2,-20.8},{11.27,11.67,-21.2,-20.8}};
 bool ObsoleteRect(double A,double B,double C,double D)
 {
  for (const auto& Q:Obsolete) if (FMath::Abs(A-Origin.X-Q[0])<.001 && FMath::Abs(B-Origin.X-Q[1])<.001 && FMath::Abs(C-Origin.Z-Q[2])<.001 && FMath::Abs(D-Origin.Z-Q[3])<.001) return true;
  return false;
 }
}

void AS47Web::BuildMesaDiner()
{
 bMesaDinerReady = false;
 const FS47Obj Root = Obj(TEXT("world.diner.group"));
 TArray<int32> New;
 for (int32 I=0;I<Nodes.Num();++I) if (Nodes[I].Name.StartsWith(TEXT("prop:mesa"))) New.Add(I);
 auto Reject = [&](const TCHAR* Reason) {
  for (int32 I:New) { SetVisible(I,false); if (UPrimitiveComponent* C=Cast<UPrimitiveComponent>(Nodes[I].Comp.Get())) C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
  UE_LOG(LogS47MesaDiner,Display,TEXT("S47: Mesa Diner original retained: %s"),Reason);
  if (FParse::Param(FCommandLine::Get(),TEXT("S47MesaDinerAudit"))) UE_LOG(LogS47MesaDiner,Error,TEXT("S47: Mesa Diner audit FAIL: %s"),Reason);
 };
 if (FParse::Param(FCommandLine::Get(),TEXT("S47LegacyMesaDiner"))) { Reject(TEXT("explicit legacy mode")); return; }
 if (!Root || New.Num()!=UE_ARRAY_COUNT(Parts)) { Reject(TEXT("complete 11-part kit absent")); return; }
 for (const TCHAR* Part:Parts)
 {
  const FS47Obj O=Root.Find(FString(TEXT("prop:mesa"))+Part);
  const FS47WebNode* N=Node(O.I); UStaticMeshComponent* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
  const FString Name=FString(TEXT("SM_Mesa_"))+Part;
  if (!C || !C->GetStaticMesh() || C->GetStaticMesh()->GetName()!=Name || C->GetNumMaterials()==0) { Reject(TEXT("missing/incorrect native part")); return; }
  for (UMaterialInterface* Material:C->GetMaterials()) if (!Material || !Material->GetName().StartsWith(TEXT("M_Mesa_"))) { Reject(TEXT("missing/incorrect native material")); return; }
  const FVector Offset=FString(Part)==TEXT("Door")?FVector(1.9,0,.58):FVector::ZeroVector;
  if (!S47Three::ToThree(C->GetComponentLocation()).Equals(Origin+Offset,.0001) || !C->GetComponentScale().Equals(FVector::OneVector,.00001) || !C->GetComponentQuat().Equals(FQuat::Identity,.00001)) { Reject(TEXT("native pivot, rotation or scale does not match the preserved diner")); return; }
 }
 for (const TCHAR* Id:Proxies) if (!Obj(FString(TEXT("world.diner.proxies."))+Id)) { Reject(TEXT("original story proxy missing")); return; }
 for (const TCHAR* Id:{TEXT("coffeePot"),TEXT("cup"),TEXT("regularCup"),TEXT("clipping"),TEXT("payphoneHandset"),TEXT("waitress"),TEXT("waitressHead"),TEXT("driver"),TEXT("driverHead")})
  if (!Obj(FString(TEXT("world.diner.objs."))+Id)) { Reject(TEXT("original dynamic object missing")); return; }
 if (!Obj(TEXT("world.diner.shell")) || !Obj(TEXT("world.diner.objs.door"))) { Reject(TEXT("original geometry hierarchy missing")); return; }
 const FBox Box=Root.Find(TEXT("prop:mesaDoor")).WorldBox();
 if (!Box.IsValid || Box.Min.Z-Origin.Z<.5 || Box.Max.Z-Origin.Z>1.0 || Box.Min.X-Origin.X<1.8 || Box.Max.X-Origin.X>3.2) { Reject(TEXT("door is not baked open clear of the original doorway")); return; }
 const FBox SignBox=Root.Find(TEXT("prop:mesaSign")).WorldBox();
 if (!SignBox.IsValid || SignBox.GetSize().Y<3 || SignBox.GetSize().Z<1 ||
  FMath::Abs(SignBox.GetCenter().X-Origin.X-2.65)>.05 || FMath::Abs(SignBox.GetCenter().Z-Origin.Z-6.8)>.1 ||
  FMath::Abs(SignBox.Min.Y-Origin.Y)>.01) { Reject(TEXT("new physical sign is not at the locked facade anchor")); return; }
 FS47Rect SignFoot; SignFoot.Id=TEXT("mesaDiner:signFoot"); SignFoot.MinX=Origin.X+2.275; SignFoot.MaxX=Origin.X+3.025; SignFoot.MinZ=Origin.Z+6.39; SignFoot.MaxZ=Origin.Z+7.21;
 // Everything above is read-only. No original disappears until the complete native kit is usable.
 auto Hide=[this](FS47Obj O) { O.SetVisible(false); O.Traverse([this](FS47Obj Child) { if (const FS47WebNode* N=Node(Child.I)) if (UPrimitiveComponent* C=Cast<UPrimitiveComponent>(N->Comp.Get())) C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }); };
 Hide(Obj(TEXT("world.diner.shell"))); Hide(Obj(TEXT("world.diner.objs.door")));
 for (const TCHAR* Name:{TEXT("clutter:diner:near"),TEXT("wear:near")}) if (const FS47Obj O=Root.Find(Name)) Hide(O);
 for (const TCHAR* Name:{TEXT("world.diner.models.models[0]"),TEXT("world.diner.models.models[1]")}) if (const FS47Obj O=Obj(Name)) Hide(O);
 int32 Removed=0;
 for (const TCHAR* Walk:{TEXT("road"),TEXT("diner")}) for (FS47Rect& R:CollidersMutable(Walk))
  if (ObsoleteRect(R.MinX,R.MaxX,R.MinZ,R.MaxZ)) { R.bEnabled=false; ++Removed; }
 const TSharedPtr<FJsonObject>* Legs=nullptr;
 if (DriveData().IsValid() && DriveData()->TryGetObjectField(TEXT("legs"),Legs)) for (const TCHAR* Leg:{TEXT("road"),TEXT("roswell")})
 {
  const TSharedPtr<FJsonObject>* L=nullptr; const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
  if (!(*Legs)->TryGetObjectField(Leg,L) || !(*L)->TryGetArrayField(TEXT("obstacles"),Rows)) continue;
  TArray<TSharedPtr<FJsonValue>> Keep;
  for (const auto& V:*Rows) { const auto& Q=V->AsArray(); if (Q.Num()!=4 || !ObsoleteRect(Q[0]->AsNumber(),Q[1]->AsNumber(),Q[2]->AsNumber(),Q[3]->AsNumber())) Keep.Add(V); }
  TArray<TSharedPtr<FJsonValue>> Foot={MakeShared<FJsonValueNumber>(SignFoot.MinX),MakeShared<FJsonValueNumber>(SignFoot.MaxX),MakeShared<FJsonValueNumber>(SignFoot.MinZ),MakeShared<FJsonValueNumber>(SignFoot.MaxZ)};
  Keep.Add(MakeShared<FJsonValueArray>(Foot));
  (*L)->SetArrayField(TEXT("obstacles"),Keep);
 }
 // Glass does not become a floor or an invisible use-trace blocker. Other new pieces answer
 // only the existing floor/visibility channel; every original interaction keeps its own proxy.
 for (int32 I:New) if (UPrimitiveComponent* C=Cast<UPrimitiveComponent>(Nodes[I].Comp.Get()))
 {
  C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Ignore);
  if (Nodes[I].Name==TEXT("prop:mesaGlass")) C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 }
 // The baked-open leaf is off the doorway centreline. Its actual footprint remains physical.
 FS47Rect Leaf; Leaf.Id=TEXT("mesaDiner:openDoor"); Leaf.MinX=Box.Min.X; Leaf.MaxX=Box.Max.X; Leaf.MinZ=Box.Min.Z; Leaf.MaxZ=Box.Max.Z;
 for (const TCHAR* Walk:{TEXT("road"),TEXT("diner")}) { CollidersMutable(Walk).Add(Leaf); CollidersMutable(Walk).Add(SignFoot); }
 // Preserve the existing sign ID/action, but put its query surface over the actual new sign.
 // The old short 1.3m hit area would leave the new taller neon impossible to examine by aiming.
 FS47Obj SignProxy=Obj(TEXT("world.diner.proxies.sign"));
 SignProxy.SetPosition(FVector(SignBox.Max.X-Origin.X+.025,SignBox.GetCenter().Y-Origin.Y,SignBox.GetCenter().Z-Origin.Z));
 SignProxy.SetScale(FVector(.08/.17,SignBox.GetSize().Y/1.3,SignBox.GetSize().Z/5.9));
 bMesaDinerReady=true;
 UE_LOG(LogS47MesaDiner,Display,TEXT("S47: Mesa Diner ready: 11 native parts, 13 original proxies and 9 dynamic objects retained; %d obsolete walk boxes disabled; open door footprint %.3f %.3f %.3f %.3f"),Removed,Leaf.MinX,Leaf.MaxX,Leaf.MinZ,Leaf.MaxZ);
 if (!FParse::Param(FCommandLine::Get(),TEXT("S47MesaDinerAudit"))) return;
 struct FProbe { int32 Stage=0; double T=0,At=0; bool Done=false; };
 const TSharedRef<FProbe> P=MakeShared<FProbe>(); FTimerHandle Timer; TWeakObjectPtr<AS47Web> Self=this;
 GetWorldTimerManager().SetTimer(Timer,[Self,P,Leaf] {
  if (P->Done) return;
  AS47Web* W=Self.Get(); US47Main* M=W?US47Main::Get(W):nullptr;
  if (!W || !M || !M->Player || !M->World || !M->Inter || S47Startup::Blocking()) return;
  P->T+=.1;
  // The timer owns this capture. Clearing that timer while executing destroys P's only
  // owning reference underneath the callback. A done guard leaves it inert until world teardown.
  auto Finish=[&](bool Pass,const TCHAR* Why) { P->Done=true; M->Player->TestMove.Reset(); UE_LOG(LogS47MesaDiner,Display,TEXT("S47: Mesa Diner audit %s: %s"),Pass?TEXT("PASS"):TEXT("FAIL"),Why); };
  if (P->T>12) { Finish(false,TEXT("walking or initialization timeout")); return; }
  if (P->Stage==0) {
   if (P->T<2 || M->Mode!=US47Main::EMode::Play) return;
   M->World->Enter(TEXT("diner")); M->Player->Place(Origin.X+3.1,Origin.Z,UE_DOUBLE_PI/2); M->Player->Pitch=-.06; M->Player->bTestRun=false; P->At=P->T; P->Stage=1; return;
  }
  if (P->Stage==1) {
   if (P->T-P->At<.2) return;
   const FVector Target=W->Obj(TEXT("world.diner.proxies.door")).WorldPosition();
   const FVector Eye=M->Player->EyeUE();
   if (M->Inter->Pick(Eye,S47Three::ToUE(Target)-Eye)!=TEXT("diner:door")) { Finish(false,TEXT("normal outside eye cannot pick original door proxy")); return; }
   M->Player->TestMove=FVector2D(0,-1); P->At=P->T; P->Stage=2; return;
  }
  if (P->Stage==2 || P->Stage==3 || P->Stage==4) {
   FHitResult Hit; const FVector V=M->Player->Pos;
   FCollisionQueryParams Query(SCENE_QUERY_STAT(S47MesaDinerFloor),true,M->Player);
   Query.bReturnFaceIndex=true;
   const bool FloorHit=W->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(FVector(V.X,M->Player->FloorY+.25,V.Z)),S47Three::ToUE(FVector(V.X,M->Player->FloorY-4,V.Z)),ECC_Visibility,Query);
   const double HitY=FloorHit?S47Three::ToThree(Hit.ImpactPoint).Y:-9999;
   if (!FloorHit || FMath::Abs(HitY-M->Player->FloorY)>.1) {
    UPrimitiveComponent* C=Hit.GetComponent(); int32 Section=INDEX_NONE;
    UMaterialInterface* Material=C?C->GetMaterialFromCollisionFaceIndex(Hit.FaceIndex,Section):nullptr;
    UStaticMeshComponent* Static=Cast<UStaticMeshComponent>(C);
    const int32 NodeIndex=W->NodeOf(C); const FS47WebNode* N=W->Node(NodeIndex);
    FString Tags; if (C) for (FName Tag:C->ComponentTags) Tags+=Tag.ToString()+TEXT("|");
    UE_LOG(LogS47MesaDiner,Error,TEXT("S47: Mesa Diner floor detail stage=%d t=%.3f posXZ=%.6f,%.6f localXZ=%.6f,%.6f floorY=%.6f callbackFloorY=%.6f hit=%d hitY=%.6f delta=%.6f actor=%s component=%s node=%s mesh=%s face=%d section=%d material=%s tags=%s traceComplex=1"),
     P->Stage,P->T,V.X,V.Z,V.X-Origin.X,V.Z-Origin.Z,M->Player->FloorY,M->Player->Floor?M->Player->Floor(V.X,V.Z):-9999,FloorHit,HitY,HitY-M->Player->FloorY,
     *GetNameSafe(Hit.GetActor()),*GetNameSafe(C),N?*N->Name:TEXT("none"),Static?*GetNameSafe(Static->GetStaticMesh()):TEXT("none"),Hit.FaceIndex,Section,*GetNameSafe(Material),*Tags);
    Finish(false,TEXT("normal walking lost its actual visible floor")); return;
   }
   if (P->Stage==4) {
    if (P->T-P->At<.8) return;
    if (FMath::Abs(V.Z-(Leaf.MinZ-M->Player->Radius))>.04) { Finish(false,TEXT("normal walking passed through the physical open door leaf")); return; }
    M->Player->Place(Origin.X+3.1,Origin.Z,UE_DOUBLE_PI/2);
    Finish(true,TEXT("11 native meshes, preserved proxies/dynamics, actual door+counter+coffee picks, visible floor, normal walk in/out and actual open-leaf stop")); return;
   }
   if (M->Player->Speed<=0) { Finish(false,TEXT("walker has no normal walking speed")); return; }
   if (P->T-P->At<5.7/M->Player->Speed) return;
   if (FMath::Abs(V.Z-Origin.Z)>.04) { Finish(false,TEXT("door crossing deflected from the clear opening")); return; }
   if (P->Stage==2) {
    if (V.X<Origin.X-2.85 || V.X>Origin.X-2.0) { Finish(false,TEXT("normal walking could not enter the customer aisle")); return; }
    for (const TCHAR* Id:{TEXT("counter"),TEXT("coffee")}) {
     const FVector Target=W->Obj(FString(TEXT("world.diner.proxies."))+Id).WorldPosition(); const FVector Eye=M->Player->EyeUE();
     if (M->Inter->Pick(Eye,S47Three::ToUE(Target)-Eye)!=FString(TEXT("diner:"))+Id) { Finish(false,TEXT("normal inside eye cannot pick retained counter/coffee")); return; }
    }
    M->Player->TestMove=FVector2D(0,1); P->At=P->T; P->Stage=3; return;
   }
   if (V.X<Origin.X+2.8 || V.X>Origin.X+3.5) { Finish(false,TEXT("normal walking could not leave through the same doorway")); return; }
   M->Player->Place(Origin.X+2.7,Origin.Z,0); M->Player->TestMove=FVector2D(0,1); P->At=P->T; P->Stage=4; return;
  }
 },.1f,true);
}

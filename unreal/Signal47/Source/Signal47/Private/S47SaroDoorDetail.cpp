#include "S47SaroDoorDetail.h"

#include "S47Three.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace S47SaroDoorDetail
{
namespace
{
 constexpr double DepthScale = .045 / .055;
 const FString Office = TEXT("/Game/Signal47/Art/Office/");
 struct FSpec { const TCHAR* Id; const TCHAR* Title; bool bArchive; double ClosedYaw, FrameDepthSign; bool bAlongZ; };
 const FSpec Specs[] = {
  {TEXT("records"), TEXT("RECORDS"), true, PI, 1, false},
  {TEXT("south"), TEXT("CONTROL"), false, PI, 1, false},
  {TEXT("lab"), TEXT("PHOTO LAB"), false, PI*.5, -1, true}
 };
 struct FOld
 {
  int32 Node = -1; bool bVis = false;
  TWeakObjectPtr<UStaticMeshComponent> Comp;
 };
 struct FDoor
 {
  FString Id, Inter, Title; bool bArchive = false;
  int32 Hinge = -1, Parent = -1, Proxy = -1, LeafNode = -1, FrameNode = -1;
  double Width = 0, Height = 0, Bottom = 0;
  FTransform LeafLocal, FrameLocal;
  FBox OldBounds = FBox(ForceInit);
  TArray<FOld> Old;
  TWeakObjectPtr<UStaticMeshComponent> Leaf, Frame;
  TArray<TWeakObjectPtr<UTextRenderComponent>> Labels;
 };
 struct FKit
 {
  UStaticMesh* Leaf = nullptr; UStaticMesh* Service = nullptr; UStaticMesh* Archive = nullptr;
  TMap<FName, UMaterialInterface*> Materials;
 };
 struct FInstalled { TArray<FDoor> Doors; FKit Kit; };
 // Actor-owned registered components retain their assets. The audit registry is weak.
 TMap<TWeakObjectPtr<AS47Web>, FInstalled> Installed;

 FTransform Three(const FVector& P, double Yaw, const FVector& Scale)
 {
  double M[16]; S47Three::Compose(P, S47Three::Euler(0, Yaw, 0), Scale, M);
  return S47Three::MatToUE(M);
 }
 bool Fail(FString& Why, const FString& Detail) { Why = Detail; return false; }
 bool Matrix(const TSharedPtr<FJsonObject>& J, const TCHAR* Key, FTransform& Out)
 {
  const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
  if (!J->TryGetArrayField(Key, A) || A->Num() != 16) return false;
  TArray<double> Values; for (const auto& V : *A) Values.Add(V->AsNumber());
  Out = S47Three::MatToUE(Values); return true;
 }
 bool Bindings(UStaticMesh* Mesh, FKit& Kit, FString& Why)
 {
  if (!Mesh || Mesh->GetStaticMaterials().IsEmpty()) return Fail(Why, TEXT("missing typed native mesh/material slots"));
  TSet<FName> Seen;
  for (const FStaticMaterial& Slot : Mesh->GetStaticMaterials())
  {
   const FName Name = Slot.MaterialSlotName;
   if (!Name.ToString().StartsWith(TEXT("M_Office_")) || Seen.Contains(Name))
    return Fail(Why, TEXT("unexpected or repeated Office slot: ") + Name.ToString());
   Seen.Add(Name);
   UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr,
    *(Office + TEXT("Materials/") + Name.ToString() + TEXT(".") + Name.ToString()));
   if (!Material || Slot.MaterialInterface != Material)
    return Fail(Why, TEXT("slot does not bind the current native Office material: ") + Name.ToString());
   Kit.Materials.Add(Name, Material);
  }
  return true;
 }
 bool LoadKit(FKit& Kit, FString& Why)
 {
  Kit.Leaf = LoadObject<UStaticMesh>(nullptr, *(Office + TEXT("SM_Office_Door.SM_Office_Door")));
  Kit.Service = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Signal47/Art/Doors/SM_DoorFrame_Service.SM_DoorFrame_Service"));
  Kit.Archive = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Signal47/Art/Doors/SM_DoorFrame_Archive.SM_DoorFrame_Archive"));
  if (!Bindings(Kit.Leaf, Kit, Why) || !Bindings(Kit.Service, Kit, Why) || !Bindings(Kit.Archive, Kit, Why)) return false;
  const FBox Leaf = Kit.Leaf->GetBoundingBox();
  if (!FMath::IsNearlyEqual(Leaf.GetSize().Z, 207., .2) ||
      FMath::Abs(Leaf.GetCenter().X) > 1 || FMath::Abs(Leaf.GetCenter().Z) > .1 ||
      !Kit.Materials.Contains(TEXT("M_Office_Walnut")) || !Kit.Materials.Contains(TEXT("M_Office_CabinetEnamel")))
   return Fail(Why, TEXT("Office leaf metric/pivot/material contract differs"));
  const FBox Service = Kit.Service->GetBoundingBox(), Archive = Kit.Archive->GetBoundingBox();
  if (!Service.Min.Equals(FVector(-5.2,-10.4,-.4), .2) || !Service.Max.Equals(FVector(105.2,10.4,213.4), .2) ||
      !Archive.Min.Equals(FVector(-7.9,-11.3,-.4), .2) || !Archive.Max.Equals(FVector(107.9,11.3,216.1), .2))
   return Fail(Why, TEXT("authored frame origin/profile bounds differ"));
  return true;
 }
 FBox RelativeBounds(UStaticMeshComponent* Comp, USceneComponent* Hinge)
 {
  FBox Result(ForceInit);
  const FBox Box = Comp->GetStaticMesh()->GetBoundingBox();
  const FTransform Local = Comp->GetComponentTransform().GetRelativeTransform(Hinge->GetComponentTransform());
  for (int32 I = 0; I < 8; ++I)
   Result += S47Three::ToThree(Local.TransformPosition(FVector(
    I & 1 ? Box.Max.X : Box.Min.X, I & 2 ? Box.Max.Y : Box.Min.Y, I & 4 ? Box.Max.Z : Box.Min.Z)));
  return Result;
 }
 bool Prepare(AS47Web* W, const FSpec& Spec, const TSharedPtr<FJsonObject>& J, FDoor& D, FString& Why)
 {
  const TArray<TSharedPtr<FJsonValue>>* Moved = nullptr;
  if (!J->TryGetArrayField(TEXT("moved"), Moved) || Moved->Num() != 1) return Fail(Why, TEXT("expected one original moving hinge"));
  const auto Move = (*Moved)[0]->AsObject();
  if (!Move.IsValid()) return Fail(Why, TEXT("invalid moved metadata"));
  D.Id = Spec.Id; D.Title = Spec.Title; D.bArchive = Spec.bArchive;
  D.Inter = J->GetStringField(TEXT("inter")); D.Hinge = int32(Move->GetNumberField(TEXT("node")));
  D.Proxy = int32(J->GetNumberField(TEXT("proxy")));
  const FS47WebNode* HingeNode = W->Node(D.Hinge);
  const FS47WebNode* Proxy = W->Node(D.Proxy);
  const FS47WebItem* Item = W->Item(D.Inter);
  if (!HingeNode || !HingeNode->Comp.IsValid() || !Proxy || !Item || Item->Node != D.Proxy || !W->IsDoor(D.Inter))
   return Fail(Why, TEXT("original hinge/proxy/item identity mismatch"));
  D.Parent = HingeNode->Parent;
  const FS47WebNode* Parent = W->Node(D.Parent);
  if (!Parent || !Parent->Comp.IsValid()) return Fail(Why, TEXT("missing fixed hinge parent"));
  FTransform Shut, Open;
  if (!Matrix(Move, TEXT("shut"), Shut) || !Matrix(Move, TEXT("open"), Open) ||
      !Shut.GetRotation().Equals(FQuat::Identity, 1e-5) || !Shut.GetScale3D().Equals(FVector::OneVector, 1e-5))
   return Fail(Why, TEXT("closed hinge basis changed"));
  // Only the original two renderer children: one leaf and its handle. Never
  // inspect/retire the photolab parent, which contains unrelated room geometry.
  if (HingeNode->Children.Num() != 2) return Fail(Why, TEXT("unexpected children below original hinge"));
  double Largest = 0;
  for (int32 I : HingeNode->Children)
  {
   const FS47WebNode* N = W->Node(I);
   UStaticMeshComponent* C = N ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
   if (!C || !C->GetStaticMesh() || C->bHiddenInGame || C->GetCollisionEnabled() != ECollisionEnabled::QueryOnly ||
       C->GetCollisionResponseToChannel(ECC_GameTraceChannel1) != ECR_Block || W->UsableAt(C) != D.Inter)
    return Fail(Why, TEXT("original renderer/query proxy contract differs"));
   D.Old.Add({I, N->bVis, C});
   const FBox B = RelativeBounds(C, HingeNode->Comp.Get());
   const double Area = FMath::Max(B.GetSize().X,B.GetSize().Z) * B.GetSize().Y;
   if (Area > Largest) { Largest = Area; D.OldBounds = B; }
  }
  const FBox& B = D.OldBounds;
  D.Width = Spec.bAlongZ ? B.GetSize().Z : B.GetSize().X;
  D.Height = B.GetSize().Y; D.Bottom = B.Min.Y;
  const double Near = Spec.bAlongZ ? B.Max.Z : B.Max.X;
  const double CenterAlong = Spec.bAlongZ ? B.GetCenter().Z : B.GetCenter().X;
  const double Thickness = Spec.bAlongZ ? B.GetSize().X : B.GetSize().Z;
  if (D.Width < .85 || D.Width > 1.05 || D.Height < 2 || D.Height > 2.15 ||
      !FMath::IsNearlyEqual(Thickness, .045, .003) || FMath::Abs(Near) > .035 ||
      FMath::Abs(D.Bottom) > .005 || CenterAlong > -.4)
   return Fail(Why, TEXT("actual native leaf bounds do not match the original closed doorway: ") + B.ToString());
  const FVector Test = S47Three::ToUE(B.GetCenter());
  const FVector ClosedPoint = Shut.TransformPosition(Test);
  const FVector MidPoint = FTransform(FQuat::Slerp(Shut.GetRotation(), Open.GetRotation(), .5),
   Shut.GetTranslation(), FVector::OneVector).TransformPosition(Test);
  const double Motion = Spec.bAlongZ ? MidPoint.X-ClosedPoint.X : MidPoint.Y-ClosedPoint.Y;
  if (Motion < 20) return Fail(Why, TEXT("original opening direction changed"));
  D.LeafLocal = Three(FVector(B.GetCenter().X, D.Bottom+.012+D.Height*.5, B.GetCenter().Z), Spec.ClosedYaw,
    FVector(D.Width, D.Height/2.07, DepthScale));
  // Closed native X runs along the measured old leaf. The fixed stop faces away
  // from the original opening motion (+Z for records/control, +X for lab).
  const FVector FrameOrigin = Spec.bAlongZ ? FVector(B.GetCenter().X,D.Bottom,Near) : FVector(Near,D.Bottom,B.GetCenter().Z);
  const FTransform FrameAtHinge = Three(FrameOrigin, Spec.ClosedYaw,
    FVector(D.Width,D.Height/2.07, Spec.FrameDepthSign));
  D.FrameLocal = FrameAtHinge * Shut;
  return true;
 }
 void SetMaterials(UStaticMeshComponent* C, FKit& Kit, bool bServiceLeaf)
 {
  const auto& Slots = C->GetStaticMesh()->GetStaticMaterials();
  for (int32 I = 0; I < Slots.Num(); ++I)
  {
   const FName Name = bServiceLeaf && Slots[I].MaterialSlotName == TEXT("M_Office_Walnut") ?
    FName(TEXT("M_Office_CabinetEnamel")) : Slots[I].MaterialSlotName;
   C->SetMaterial(I, Kit.Materials.FindChecked(Name));
  }
 }
 UStaticMeshComponent* AddMesh(AS47Web* W, FKit& Kit, const FString& Name, UStaticMesh* Mesh,
                             int32 Parent, const FTransform& Local, bool bServiceLeaf, int32& Node)
 {
  UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, *Name);
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(W->Node(Parent)->Comp.Get());
  C->SetStaticMesh(Mesh); C->SetRelativeTransform(Local); C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->SetGenerateOverlapEvents(false); C->SetCastShadow(true); SetMaterials(C, Kit, bServiceLeaf);
  W->AddInstanceComponent(C); C->RegisterComponent(); Node = W->AdoptNode(C, Parent, Name); return C;
 }
 bool CurrentMaterials(const UStaticMeshComponent* C, const FKit& Kit, bool bServiceLeaf)
 {
  if (!C || !C->GetStaticMesh()) return false;
  const auto& Slots = C->GetStaticMesh()->GetStaticMaterials();
  if (C->GetNumMaterials() != Slots.Num()) return false;
  for (int32 I = 0; I < Slots.Num(); ++I)
  {
   FName Name = Slots[I].MaterialSlotName;
   if (bServiceLeaf && Name == TEXT("M_Office_Walnut")) Name = TEXT("M_Office_CabinetEnamel");
   UMaterialInterface* const* Expected = Kit.Materials.Find(Name);
   if (!Expected || C->GetMaterial(I) != *Expected) return false;
  }
  return true;
 }
 FString Semantics(AS47Web* W)
 {
  FString S;
  for (const FS47Rect& R : W->ZonesMutable(TEXT("saro")))
   S += FString::Printf(TEXT("Z%s %.9f %.9f %.9f %.9f %d;"), *R.Id,R.MinX,R.MaxX,R.MinZ,R.MaxZ,R.bEnabled);
  for (const FS47Rect& R : W->CollidersMutable(TEXT("saro")))
   S += FString::Printf(TEXT("C%s %.9f %.9f %.9f %.9f %d;"), *R.Id,R.MinX,R.MaxX,R.MinZ,R.MaxZ,R.bEnabled);
  const auto States = W->DoorStates();
  for (const FSpec& Spec : Specs) S += FString::Printf(TEXT("D%s=%d;"), Spec.Id, States.FindRef(Spec.Id));
  return S;
 }
}

bool Build(AS47Web* W)
{
 if (!W || !W->IsLoaded() || FParse::Param(FCommandLine::Get(), TEXT("S47SaroDoorDetailOff")) ||
     FParse::Param(FCommandLine::Get(), TEXT("S47PropsOff"))) return false;
 if (Installed.Contains(W)) return true;
 FInstalled Candidate; FString Why, Text;
 TSharedPtr<FJsonObject> Manifest;
 if (!LoadKit(Candidate.Kit, Why) || !FFileHelper::LoadFileToString(Text,
     *(FPaths::ProjectContentDir()/TEXT("WebData/manifest.json"))) ||
     !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Manifest) || !Manifest.IsValid())
 {
  UE_LOG(LogTemp, Warning, TEXT("S47 door detail skipped; originals retained: %s"), *Why); return false;
 }
 const TArray<TSharedPtr<FJsonValue>>* Doors = nullptr;
 if (!Manifest->TryGetArrayField(TEXT("doors"), Doors)) return false;
 for (const FSpec& Spec : Specs)
 {
  TSharedPtr<FJsonObject> Match; int32 Count = 0;
  for (const auto& V : *Doors) if (V->AsObject().IsValid() && V->AsObject()->GetStringField(TEXT("id")) == Spec.Id)
  { Match = V->AsObject(); ++Count; }
  FDoor D;
  if (Count != 1 || !Prepare(W, Spec, Match, D, Why))
  {
   UE_LOG(LogTemp, Warning, TEXT("S47 door detail skipped; all originals retained, %s: %s"), Spec.Id, *Why); return false;
  }
  Candidate.Doors.Add(MoveTemp(D));
 }
 const FString Before = Semantics(W);
 // Every asset, material and all three original door contracts passed first.
 for (FDoor& D : Candidate.Doors)
 {
  const FString Name = TEXT("saroDoorDetail:") + D.Id;
  D.Leaf = AddMesh(W, Candidate.Kit, Name+TEXT(":leaf"), Candidate.Kit.Leaf, D.Hinge, D.LeafLocal, !D.bArchive, D.LeafNode);
  D.Frame = AddMesh(W, Candidate.Kit, Name+TEXT(":frame"), D.bArchive ? Candidate.Kit.Archive : Candidate.Kit.Service,
                    D.Parent, D.FrameLocal, false, D.FrameNode);
  for (int32 Side : {-1, 1})
  {
   UTextRenderComponent* C = NewObject<UTextRenderComponent>(W, *(Name+FString::Printf(TEXT(":label%d"),Side)));
   C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(D.Leaf.Get());
   C->SetText(FText::FromString(D.Title)); C->SetWorldSize(D.bArchive ? 3.8f : 3.5f);
   C->SetHorizontalAlignment(EHTA_Center); C->SetVerticalAlignment(EVRTA_TextCenter);
   C->SetTextRenderColor(FColor(28,35,29)); C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   C->SetRelativeLocation(FVector(0,Side*4.6,61.5)); C->SetRelativeRotation(FRotator(0,Side*90,0));
   W->AddInstanceComponent(C); C->RegisterComponent(); W->AdoptNode(C, D.LeafNode, Name+TEXT(":label"));
   D.Labels.Add(C);
  }
 }
 for (FDoor& D : Candidate.Doors) for (const FOld& Old : D.Old)
  Old.Comp->SetHiddenInGame(true, false); // QueryOnly/use and every original node visibility flag stay intact.
 const bool bSame = Before == Semantics(W);
 if (!bSame)
 {
  // No story setter is called here. Still retain the old renderers if a future
  // change accidentally mutates state while adding the visual children.
  for (FDoor& D : Candidate.Doors)
  {
   for (const FOld& Old : D.Old) Old.Comp->SetHiddenInGame(false,false);
   D.Leaf->SetHiddenInGame(true,true); D.Frame->SetHiddenInGame(true,true);
  }
  UE_LOG(LogTemp, Error, TEXT("S47 door detail state guard failed; original renderers restored")); return false;
 }
 Installed.Add(W, MoveTemp(Candidate));
 UE_LOG(LogTemp, Display, TEXT("S47 door detail installed: three native leaves/frames, six English plaques; original state/zones/colliders %s"), bSame?TEXT("UNCHANGED"):TEXT("FAIL"));
 return bSame;
}

bool Audit(AS47Web* W, TArray<FString>& Lines)
{
 const FInstalled* Have = W ? Installed.Find(W) : nullptr;
 if (!Have) { Lines.Add(TEXT("SARO native door detail FAIL: no installed candidate; original doors retained")); return false; }
 bool All = true;
 for (const FDoor& D : Have->Doors)
 {
  const FS47WebNode* H = W->Node(D.Hinge); const FS47WebNode* P = W->Node(D.Parent);
  const FS47WebItem* Item = W->Item(D.Inter);
  bool Pass = H && P && H->Parent == D.Parent && W->Node(D.Proxy) && Item && Item->Node == D.Proxy && W->IsDoor(D.Inter);
  Pass &= D.Leaf.IsValid() && D.Frame.IsValid();
  if (H && P && D.Leaf.IsValid() && D.Frame.IsValid())
  {
   Pass &= D.Leaf->GetAttachParent() == H->Comp.Get() && D.Frame->GetAttachParent() == P->Comp.Get() &&
    D.Leaf->GetRelativeTransform().Equals(D.LeafLocal,.001) && D.Frame->GetRelativeTransform().Equals(D.FrameLocal,.001) &&
    D.Leaf->GetStaticMesh() == Have->Kit.Leaf && D.Frame->GetStaticMesh() == (D.bArchive?Have->Kit.Archive:Have->Kit.Service) &&
    !D.Leaf->bHiddenInGame && !D.Frame->bHiddenInGame &&
    D.Leaf->IsVisible() == W->ObjAt(D.LeafNode).Shown() && D.Frame->IsVisible() == W->ObjAt(D.FrameNode).Shown() &&
    D.Leaf->GetCollisionEnabled() == ECollisionEnabled::NoCollision && D.Frame->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
    CurrentMaterials(D.Leaf.Get(),Have->Kit,!D.bArchive) && CurrentMaterials(D.Frame.Get(),Have->Kit,false);
  }
  for (const FOld& Old : D.Old)
  {
   const FS47WebNode* N = W->Node(Old.Node);
   Pass &= Old.Comp.IsValid() && N && N->Parent == D.Hinge && N->Comp.Get() == Old.Comp.Get() && N->bVis == Old.bVis;
   if (Old.Comp.IsValid()) Pass &= Old.Comp->bHiddenInGame && Old.Comp->GetCollisionEnabled() == ECollisionEnabled::QueryOnly &&
    Old.Comp->GetCollisionResponseToChannel(ECC_GameTraceChannel1) == ECR_Block && W->UsableAt(Old.Comp.Get()) == D.Inter;
  }
  Pass &= D.Labels.Num() == 2;
  for (const auto& C : D.Labels) Pass &= C.IsValid() && C->GetAttachParent() == D.Leaf.Get() &&
   C->Text.ToString() == D.Title && !C->bHiddenInGame && C->GetCollisionEnabled() == ECollisionEnabled::NoCollision;
  Lines.Add(FString::Printf(TEXT("SARO native door %s %s: hinge=%d fixedParent=%d proxy=%d item=%s state=%s; measured leaf=%s; native slots/renderer/local transforms/English plaques/query proxy checked"),
   *D.Id, Pass?TEXT("PASS"):TEXT("FAIL"), D.Hinge,D.Parent,D.Proxy,*D.Inter,W->IsDoorOpen(D.Inter)?TEXT("open"):TEXT("closed"),*D.OldBounds.ToString()));
  if (H && H->Comp.IsValid() && D.Leaf.IsValid() && D.Frame.IsValid())
  {
   Lines.Add(FString::Printf(TEXT("SARO native door %s actual hinge-local leaf bounds=%s; fixed-parent frame transform=%s; visible leaf/frame=%d/%d"),
    *D.Id,*RelativeBounds(D.Leaf.Get(),H->Comp.Get()).ToString(),*D.Frame->GetRelativeTransform().ToString(),D.Leaf->IsVisible(),D.Frame->IsVisible()));
   for (int32 I = 0; I < D.Leaf->GetNumMaterials(); ++I)
    Lines.Add(FString::Printf(TEXT("SARO native door %s leaf slot %s = %s"),*D.Id,
     *D.Leaf->GetStaticMesh()->GetStaticMaterials()[I].MaterialSlotName.ToString(),*GetPathNameSafe(D.Leaf->GetMaterial(I))));
  }
  All &= Pass;
 }
 Lines.Add(TEXT("SARO native doors: swinging, both-side use, existing-save restore and rendered fit require runtime verification"));
 return All;
}
}

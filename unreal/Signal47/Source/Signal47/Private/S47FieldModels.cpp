// Unreal-only authored field investigation kit. Story anchors and colliders remain authoritative.
#include "S47Web.h"
#include "S47Three.h"
#include "S47Glow.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47FieldModels, Log, All);
namespace
{
 FVector FieldVec(const TArray<TSharedPtr<FJsonValue>>& A)
 { return A.Num() == 3 ? FVector(A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber()) : FVector::ZeroVector; }
}

void AS47Web::BuildFieldModels()
{
 if (FParse::Param(FCommandLine::Get(), TEXT("S47FieldModelsOff")) || FParse::Param(FCommandLine::Get(), TEXT("S47PropsOff"))) return;
 FString Text; TSharedPtr<FJsonObject> Assembly;
 if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("FieldModels/assembly.json"))) ||
     !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Assembly) || !Assembly.IsValid())
 { UE_LOG(LogS47FieldModels, Warning, TEXT("S47: field models metadata absent; originals retained")); return; }
 auto Reject = [](const FString& Why) { UE_LOG(LogS47FieldModels, Error, TEXT("S47: field models rejected: %s; originals retained"), *Why); };
 if (Assembly->GetIntegerField(TEXT("version")) != 1) { Reject(TEXT("assembly version")); return; }
 struct FReady { UStaticMesh* Mesh; int32 Parent; FVector At; bool Local; FString Name, Use; double Yaw; };
 struct FCut { UStaticMeshComponent* Component; UStaticMesh* Mesh; TArray<int32> Slots; bool Hide; };
 TArray<FReady> Ready; TArray<FCut> Cuts;
 const auto& Rows = Assembly->GetArrayField(TEXT("parts"));
 if (Rows.Num() != Assembly->GetIntegerField(TEXT("part_count")) || Rows.Num() < 10) { Reject(TEXT("part count")); return; }
 // Validate the whole kit before changing a single original component.
 for (const auto& Value : Rows)
 {
  const auto R = Value->AsObject(); const FString Name = R->GetStringField(TEXT("name"));
  auto* Mesh = LoadObject<UStaticMesh>(nullptr, *R->GetStringField(TEXT("asset")));
  const int32 Parent = Handle(R->GetStringField(TEXT("parent_handle")));
  if (!Mesh || !Node(Parent) || !Node(Parent)->Comp.IsValid() || Mesh->GetNumLODs() != 3) { Reject(Name + TEXT(" asset/parent/LODs")); return; }
  const auto& Tris = R->GetArrayField(TEXT("lod_triangles"));
  for (int32 L = 0; L < 3; ++L) if (Tris.Num() != 3 || Mesh->GetNumTriangles(L) != int32(Tris[L]->AsNumber())) { Reject(Name + TEXT(" geometry")); return; }
  const auto& B = R->GetArrayField(TEXT("bounds_local_m"));
  if (B.Num() != 2) { Reject(Name + TEXT(" bounds schema")); return; }
  const FVector Lo = FieldVec(B[0]->AsArray()), Hi = FieldVec(B[1]->AsArray());
  const FVector Center((Lo.X+Hi.X)*50, -(Lo.Y+Hi.Y)*50, (Lo.Z+Hi.Z)*50);
  if (!Mesh->GetBounds().Origin.Equals(Center, .4) || !Mesh->GetBounds().BoxExtent.Equals((Hi-Lo)*50, .4)) { Reject(Name + TEXT(" metric pivot")); return; }
  const auto& Mats = R->GetArrayField(TEXT("materials"));
  if (Mats.Num() != Mesh->GetStaticMaterials().Num()) { Reject(Name + TEXT(" material count")); return; }
  for (int32 K = 0; K < Mats.Num(); ++K) if (!Mesh->GetMaterial(K) || Mesh->GetMaterial(K)->GetName() != Mats[K]->AsString()) { Reject(Name + TEXT(" material binding")); return; }
  Ready.Add({Mesh, Parent, FieldVec(R->GetArrayField(TEXT("at"))), R->GetBoolField(TEXT("local")), Name, R->GetStringField(TEXT("use")), R->GetNumberField(TEXT("yaw"))});
 }
 for (const auto& Value : Assembly->GetArrayField(TEXT("patches")))
 {
  const auto R = Value->AsObject(); const int32 I = R->GetIntegerField(TEXT("node")); const auto* N = Node(I);
  auto* C = N ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
  if (!C || !C->GetStaticMesh() || N->Geo != R->GetIntegerField(TEXT("geo")) || N->Parent != R->GetIntegerField(TEXT("parent")) ||
      C->GetStaticMesh()->GetNumTriangles(0) != R->GetIntegerField(TEXT("tris"))) { Reject(FString::Printf(TEXT("predecessor node %d"), I)); return; }
  FCut Cut{C, nullptr, {}, R->GetBoolField(TEXT("hidden"))};
  if (!Cut.Hide)
  {
   Cut.Mesh = LoadObject<UStaticMesh>(nullptr, *R->GetStringField(TEXT("asset")));
   if (!Cut.Mesh || Cut.Mesh->GetNumTriangles(0) != R->GetIntegerField(TEXT("resultTris"))) { Reject(TEXT("trim geometry")); return; }
   for (const auto& Slot : R->GetArrayField(TEXT("originalMaterialIndices")))
   { const int32 K = int32(Slot->AsNumber()); if (K < 0 || K >= C->GetNumMaterials()) { Reject(TEXT("trim slot source")); return; } Cut.Slots.Add(K); }
   if (Cut.Slots.Num() != Cut.Mesh->GetStaticMaterials().Num()) { Reject(TEXT("trim slot count")); return; }
  }
  Cuts.Add(MoveTemp(Cut));
 }
 // Bind only controlled instrument paint, never whole body materials or photographic evidence.
 const auto& Bindings = Assembly->GetArrayField(TEXT("bindings"));
 for (const auto& Value : Bindings)
 {
  const auto R = Value->AsObject(); const FString Part = R->GetStringField(TEXT("part"));
  const auto* P = Ready.FindByPredicate([&](const FReady& X) { return X.Name == Part; });
  const int32 Slot = R->GetIntegerField(TEXT("slot"));
  const FString Kind = R->GetStringField(TEXT("kind")), HandleName = R->GetStringField(TEXT("handle"));
  const bool ValidSource = Kind == TEXT("material") ? HandleMat(HandleName) >= 0 : Handle(HandleName) >= 0;
  if (!P || Slot < 0 || Slot >= P->Mesh->GetStaticMaterials().Num() || !ValidSource) { Reject(Part + TEXT(" live binding")); return; }
 }
 TMap<FString,UStaticMeshComponent*> Made;
 for (const auto& P : Ready)
 {
  auto* C = NewObject<UStaticMeshComponent>(this, *P.Name); C->SetMobility(EComponentMobility::Movable); C->SetStaticMesh(P.Mesh);
  C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore);
  C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
  if (!P.Use.IsEmpty()) C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
  C->SetCastShadow(true); C->RegisterComponent();
  C->AttachToComponent(Node(P.Parent)->Comp.Get(), P.Local ? FAttachmentTransformRules::KeepRelativeTransform : FAttachmentTransformRules::KeepWorldTransform);
  if (P.Local) { C->SetRelativeLocation(S47Three::ToUE(P.At)); C->SetRelativeRotation(FRotator(0,-P.Yaw,0)); }
  else { C->SetWorldLocation(S47Three::ToUE(P.At)); C->SetWorldRotation(FRotator(0,-P.Yaw,0)); }
  const int32 I = AdoptNode(C, P.Parent, P.Name); if (!P.Use.IsEmpty()) MarkUsable(I, P.Use);
  Made.Add(P.Name,C); FieldModelComponents.Add(C);
 }
 for (const FCut& Cut : Cuts)
 {
  if (Cut.Hide) { Cut.Component->SetHiddenInGame(true,false); Cut.Component->SetCastShadow(false); FieldModelRetired.Add(Cut.Component); continue; }
  TArray<UMaterialInterface*> Original; for (int32 K=0; K<Cut.Component->GetNumMaterials(); ++K) Original.Add(Cut.Component->GetMaterial(K));
  Cut.Component->SetStaticMesh(Cut.Mesh); Cut.Component->EmptyOverrideMaterials();
  for (int32 K=0; K<Cut.Slots.Num(); ++K) Cut.Component->SetMaterial(K, Original[Cut.Slots[K]]);
 }
 for (const auto& Value : Bindings)
 {
  const auto R=Value->AsObject(); auto* C=Made[R->GetStringField(TEXT("part"))]; const int32 Slot=R->GetIntegerField(TEXT("slot"));
  auto* Mid=UMaterialInstanceDynamic::Create(C->GetMaterial(Slot),this); C->SetMaterial(Slot,Mid); KeepMids.Add(Mid);
  const bool Shared=R->GetStringField(TEXT("kind"))==TEXT("material"); const FString H=R->GetStringField(TEXT("handle"));
  FieldModelBindings.Add({Mid,Shared ? HandleMat(H) : Handle(H),Shared,float(R->GetNumberField(TEXT("emission")))});
 }
 // The old metre-wide glow cards cover the authored door from normal player height.
 // Keep their existing switch/colour controls, with a physical bulb-sized footprint.
 if (auto* G=Glow(Handle(TEXT("world.site.glow.points"))))
  for (int32 I=0; I<G->Num(); ++I) G->SetSize(I,FMath::Min(G->Size[I],.08f));
 UpdateFieldModels();
 UE_LOG(LogS47FieldModels,Display,TEXT("S47: field models ready: %d native parts, %d precise patches, %d live bindings; original story anchors retained"),Ready.Num(),Cuts.Num(),FieldModelBindings.Num());
}

void AS47Web::UpdateFieldModels()
{
 for (const auto& C:FieldModelRetired) if (C.IsValid()) C->SetHiddenInGame(true,false);
 for (const auto& B:FieldModelBindings)
 {
  if (!B.Mid.IsValid()) continue;
  UMaterialInstanceDynamic* Source=nullptr;
  if (B.Shared) Source=SharedMid(B.Source);
  else { const auto Mids=NodeMids(B.Source); if (!Mids.IsEmpty()) Source=Mids[0]; }
  if (!Source) continue;
  FLinearColor Color=FLinearColor::White;
  if (Source->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Color")),Color))
  { B.Mid->SetVectorParameterValue(TEXT("Color"),Color); B.Mid->SetVectorParameterValue(TEXT("TextureTint"),Color); B.Mid->SetVectorParameterValue(TEXT("EmissiveFactor"),Color*B.Emission); }
 }
}

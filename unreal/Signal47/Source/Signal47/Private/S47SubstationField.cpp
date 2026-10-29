// FIX-07: original native field kit for the T02 reference/test hardware and the apron grade.
// The earlier engine-cube T02 meshes stay as hidden, unchanged interaction/collision/occluder
// proxies, so every T01/T02 trace, collider and saved-photo feature keeps its exact geometry.
#include "S47Substation.h"
#include "S47Main.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "StaticMeshResources.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SubstationField, Log, All);

namespace
{
 // Same placement as SubstationDetail: Blender site metres (x,y,z) -> world Three (7890-x,-1.10+z,536.20+y).
 const FTransform FieldTransform(FRotator(0, 180, 0), FVector(789000, 53620, -110), FVector::OneVector);
 struct FFieldPart { const TCHAR* Name; int32 Triangles[3]; };
 #include "S47SubstationFieldData.inl"
}

bool FS47Substation::FieldInstalled() const { return FieldParts == UE_ARRAY_COUNT(FieldPartsData); }

void FS47Substation::InstallField()
{
 FieldParts = 0;
 if (!M || !M->Web || FParse::Param(FCommandLine::Get(), TEXT("S47SubstationFieldOff")) || FParse::Param(FCommandLine::Get(), TEXT("S47PropsOff"))) return;
 const FS47WebNode* Parent = M->Web->Node(Root.I);
 if (!Parent || !Parent->Comp.IsValid()) return;
 TArray<UStaticMesh*> Meshes;
 for (const FFieldPart& Part : FieldPartsData)
 {
  UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/SubstationField/%s.%s"), Part.Name, Part.Name));
  bool Ok = Mesh && Mesh->GetRenderData() && Mesh->GetRenderData()->LODResources.Num() == 3;
  for (int32 L = 0; Ok && L < 3; ++L) Ok = Mesh->GetNumTriangles(L) == Part.Triangles[L];
  for (int32 S = 0; Ok && S < Mesh->GetStaticMaterials().Num(); ++S) Ok = Mesh->GetStaticMaterials()[S].MaterialInterface != nullptr;
  if (!Ok)
  {
   // An absent or stale kit keeps the earlier visible T02 meshes; nothing is half-replaced.
   UE_LOG(LogS47SubstationField, Warning, TEXT("S47: substation field kit skipped: invalid native asset %s; earlier T02 meshes stay visible"), Part.Name);
   return;
  }
  Meshes.Add(Mesh);
 }
 for (int32 I = 0; I < Meshes.Num(); ++I)
 {
  UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(M->Web, *FString::Printf(TEXT("S47SubstationField_%s"), FieldPartsData[I].Name));
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(Parent->Comp.Get()); C->SetStaticMesh(Meshes[I]);
  // Decorative: no walk, use, photo-occluder or navigation change. Proxies keep that authority.
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetGenerateOverlapEvents(false); C->SetCanEverAffectNavigation(false);
  C->SetCastShadow(true); C->RegisterComponent(); C->SetWorldTransform(FieldTransform);
  M->Web->AdoptNode(C, Root.I, FString(TEXT("substationField:")) + FieldPartsData[I].Name);
  ++FieldParts;
 }
 // Retire only the visible earlier engine cubes now represented by the kit. Their nodes stay
 // shown (Shown() is node based), so FollowupOccluders, use traces and colliders are unchanged.
 int32 Hidden = 0;
 for (const FS47Obj& O : FollowupVisuals)
 {
  const FS47WebNode* N = O ? M->Web->Node(O.I) : nullptr;
  UPrimitiveComponent* C = N ? Cast<UPrimitiveComponent>(N->Comp.Get()) : nullptr;
  if (!C) continue;
  for (const TCHAR* Name : FieldRetired)
   if (N->Name == Name) { C->SetHiddenInGame(true); C->SetCastShadow(false); ++Hidden; }
 }
 // Small interactive parts remain the live animated/emissive objects, in period finishes.
 for (const FS47Obj& O : FollowupVisuals)
 {
  const FS47WebNode* N = O ? M->Web->Node(O.I) : nullptr;
  UStaticMeshComponent* C = N ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
  UMaterialInstanceDynamic* D = C ? Cast<UMaterialInstanceDynamic>(C->GetMaterial(0)) : nullptr;
  if (!D) continue;
  if (N->Name == TEXT("T02IsolationHandle")) D->SetVectorParameterValue(TEXT("Color"), FLinearColor(.020f, .018f, .016f));
  else if (N->Name == TEXT("T02ReportPlate")) D->SetVectorParameterValue(TEXT("Color"), FLinearColor(.34f, .34f, .31f));
  else if (N->Name == TEXT("T02AuxiliaryHandle")) D->SetVectorParameterValue(TEXT("Color"), FLinearColor(.42f, .16f, .02f));
 }
 UE_LOG(LogS47SubstationField, Display, TEXT("S47: substation field kit installed parts=%d retiredCubes=%d/%d; T02 proxies, colliders and occluders unchanged"), FieldParts, Hidden, int32(UE_ARRAY_COUNT(FieldRetired)));
}

void FS47Substation::ProbeGround()
{
 // -S47SubstationGroundProbe=<file>: measured downward visibility hits and the walker floor
 // around the yard front. Authoring input for Tools/substationfield; no gameplay effect.
 FString Out;
 if (!FParse::Value(FCommandLine::Get(), TEXT("S47SubstationGroundProbe="), Out) || !M || !M->Web || !M->Web->GetWorld()) return;
 FString Json = TEXT("{\"space\":\"world Three metres\",\"rows\":[\n");
 bool First = true;
 for (double Z = 516.0; Z <= 531.01; Z += .25)
  for (double X = 7874.0; X <= 7902.01; X += .25)
  {
   const double Floor = M->Player && M->Player->LandFloor ? M->Player->LandFloor(X, Z) : NAN;
   FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SubstationGroundProbe), true);
   if (M->Player) Q.AddIgnoredActor(M->Player);
   FString Hits;
   for (int32 K = 0; K < 8; ++K)
   {
    FHitResult H;
    if (!M->Web->GetWorld()->LineTraceSingleByChannel(H, S47Three::ToUE(FVector(X, 4, Z)), S47Three::ToUE(FVector(X, -6, Z)), ECC_Visibility, Q)) break;
    const UPrimitiveComponent* C = H.GetComponent();
    const int32 Node = C ? M->Web->NodeOf(C) : -1;
    const FString Name = Node >= 0 ? M->Web->Node(Node)->Name : C ? C->GetName() : FString(TEXT("?"));
    Hits += FString::Printf(TEXT("%s[%.4f,\"%s\"]"), Hits.IsEmpty() ? TEXT("") : TEXT(","), H.ImpactPoint.Z / 100.0, *Name.ReplaceCharWithEscapedChar());
    if (!C) break;
    Q.AddIgnoredComponent(C);
   }
   Json += FString::Printf(TEXT("%s[%.2f,%.2f,%.4f,[%s]]"), First ? TEXT("") : TEXT(",\n"), X, Z, Floor, *Hits);
   First = false;
  }
 Json += TEXT("\n]}\n");
 FFileHelper::SaveStringToFile(Json, *Out);
 UE_LOG(LogS47SubstationField, Display, TEXT("S47: substation ground probe PASS written %s"), *Out);
}

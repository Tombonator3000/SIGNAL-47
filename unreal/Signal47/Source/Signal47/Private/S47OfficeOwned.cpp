#include "S47OfficeOwned.h"

#include "S47Three.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshResources.h"

namespace S47OfficeOwned
{
namespace
{
 const FString AssetRoot(TEXT("/Game/Licensed/SaroInteriors/"));
 const FString Prefix(TEXT("officeOwned:"));
 struct FAsset
 {
  const TCHAR* Name; const TCHAR* Material; int32 Triangles;
  FVector MinCm, MaxCm;
 };
 // Geometry metadata only, measured from the historical source FBX. Rejected
 // r3 PBR is not accepted here. Root separately binds the r4 import/fresh reader.
 const FAsset Assets[] = {
  {TEXT("SM_InteriorOwned_SignalCable"),TEXT("M_InteriorOwned_SignalCable"),14386,
   FVector(-19.619182,-24.127245,0),FVector(19.619183,24.127245,5.556832)},
  {TEXT("SM_InteriorOwned_ArchiveBox"),TEXT("M_InteriorOwned_ArchiveBox"),660,
   FVector(-21.248490,-17.048046,0),FVector(21.248490,17.048049,25.016913)},
  {TEXT("SM_InteriorOwned_ArchiveBoxBody"),TEXT("M_InteriorOwned_ArchiveBox"),515,
   FVector(-20.562333,-16.256168,0),FVector(20.706266,16.465151,24.816093)},
  {TEXT("SM_InteriorOwned_ArchiveBoxLid"),TEXT("M_InteriorOwned_ArchiveBox"),145,
   FVector(-21.248490,-17.048046,21.416675),FVector(21.248490,17.048049,25.016913)}
 };
 const TCHAR* Ids[] = {TEXT("signalCable"),TEXT("deskArchiveBox"),TEXT("openArchiveBody"),TEXT("openArchiveLid")};
 const TCHAR* PaperNames[] = {TEXT("office:fieldFolder"),TEXT("office:workingIndex"),TEXT("office:clippings"),
  TEXT("office:foldedNote"),TEXT("office:contactCard"),TEXT("office:deskForms"),TEXT("office:privateFolder"),TEXT("office:arrayServiceSheet")};
 struct FPaper { FString Name; int32 Node=-1; FBox Bounds=FBox(ForceInit); };
 struct FInstance { int32 Node=-1; TWeakObjectPtr<UStaticMeshComponent> Component; FTransform Wanted; };
 struct FInstalled { int32 Parent=-1; TArray<FInstance> Parts; TArray<FPaper> Papers; };
 TMap<TWeakObjectPtr<AS47Web>,FInstalled> Installed;

 bool Fail(FString& Why, const FString& Message) { Why=Message; return false; }
 FString ObjectPath(const FString& Package) { return Package+TEXT(".")+FPackageName::GetShortName(Package); }
 FTransform Three(const FVector& P, double X=0)
 {
  double M[16]; S47Three::Compose(P,S47Three::Euler(X,0,0),FVector::OneVector,M); return S47Three::MatToUE(M);
 }
 FVector Position(int32 Index)
 {
  if (Index==0) return FVector(-.47,.175,7.75); // Lower shelf top .1735m, 1.5mm clearance.
  if (Index==1) return FVector(.37,.008,9.83); // Inside the actual desk kneehole, before its rear panel.
  if (Index==2) return FVector(-.60,.008,8.70);
  // The removable lid keeps its original shared mesh origin. Its actual rear
  // low point is propped on the body's rear top rim, rather than rotated about
  // the floor origin. No hinge hardware or interactive state is invented.
  const FVector SourceContact(0,.214739099,.170266151), BodyContact(-.604,.256160928,8.864651513);
  return BodyContact-S47Three::Euler(FMath::DegreesToRadians(75.),0,0).RotateVector(SourceContact);
 }
 FTransform Wanted(int32 Index) { return Three(Position(Index),Index==3?FMath::DegreesToRadians(75.):0); }

 bool MaterialMaps(UMaterialInterface* Material, const FAsset& Spec, FString& Why)
 {
  const FString MaterialPackage=AssetRoot+TEXT("Materials/")+Spec.Material;
  if (!Material || !Cast<UMaterial>(Material) || Material->GetPathName()!=ObjectPath(MaterialPackage) ||
      Material->GetBlendMode()!=BLEND_Opaque || Material->IsTwoSided())
   return Fail(Why,FString(Spec.Name)+TEXT(" original opaque native material identity differs"));
  FLinearColor Color, Emissive; float Strength=0;
  if (!Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Color")),Color) || !Color.Equals(FLinearColor::White,.0001f) ||
      !Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveFactor")),Emissive) ||
      FMath::Max3(FMath::Abs(Emissive.R),FMath::Abs(Emissive.G),FMath::Abs(Emissive.B))>.0001f ||
      !Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("NormalStrength")),Strength) || !FMath::IsNearlyEqual(Strength,1.f,.0001f))
   return Fail(Why,FString(Spec.Name)+TEXT(" neutral colour, nonemissive or full-strength normal parameters differ"));
  TArray<UTexture*> Used; Material->GetUsedTextures(Used);
  TSet<UTexture*> Unique; for (UTexture* T:Used) if (T) Unique.Add(T);
  if (Unique.Num()!=4) return Fail(Why,FString(Spec.Name)+FString::Printf(TEXT(" uses %d distinct textures, expected four native PBR maps"),Unique.Num()));
  for (const TCHAR* Role:{TEXT("base_color"),TEXT("roughness"),TEXT("metallic"),TEXT("normal")})
  {
   const FString Name=FString(TEXT("T_"))+Spec.Material+TEXT("_")+Role;
   const FString Path=ObjectPath(AssetRoot+TEXT("Materials/Textures/")+Name);
   UTexture2D* Found=nullptr;
   for (UTexture* T:Unique) if (T->GetPathName()==Path) Found=Cast<UTexture2D>(T);
   if (!Found || Found->GetSizeX()<256 || Found->GetSizeY()<256 || Found->SRGB!=(FString(Role)==TEXT("base_color")))
    return Fail(Why,FString(Spec.Name)+TEXT(" missing, unready or incorrectly colour-spaced used map ")+Role+TEXT(" at ")+Path);
   const bool Normal=FString(Role)==TEXT("normal");
   if (Found->CompressionSettings!=(Normal?TC_Normalmap:TC_Default))
    return Fail(Why,FString(Spec.Name)+TEXT(" unexpected compression on used ")+Role+TEXT(" map"));
#if WITH_EDITORONLY_DATA
   if (Normal && Found->bFlipGreenChannel) return Fail(Why,FString(Spec.Name)+TEXT(" normal map has unexpected green-channel inversion"));
#endif
  }
  return true;
 }
 bool AssetValid(UStaticMesh* Mesh, const FAsset& Spec, FString& Why)
 {
  if (!Mesh || Mesh->GetPathName()!=ObjectPath(AssetRoot+Spec.Name) || !Mesh->GetRenderData() ||
      Mesh->GetRenderData()->LODResources.Num()!=3 || Mesh->GetStaticMaterials().Num()!=1 || !Mesh->bSupportRayTracing)
   return Fail(Why,FString(Spec.Name)+TEXT(" missing typed private mesh, three native LODs, single slot or RT support"));
  if (Mesh->GetNumTriangles(0)!=Spec.Triangles || Mesh->GetNumTriangles(1)>=Spec.Triangles ||
      Mesh->GetNumTriangles(1)<=Mesh->GetNumTriangles(2) || Mesh->GetNumTriangles(2)<=0)
   return Fail(Why,FString(Spec.Name)+TEXT(" native triangle identity or decreasing LODs differ"));
  const FBox Bounds=Mesh->GetBoundingBox();
  if (!Bounds.IsValid || !Bounds.Min.Equals(Spec.MinCm,.2) || !Bounds.Max.Equals(Spec.MaxCm,.2))
   return Fail(Why,FString(Spec.Name)+TEXT(" native metric bounds or shared source origin differ"));
  const FStaticMaterial& Slot=Mesh->GetStaticMaterials()[0];
  if (Slot.MaterialSlotName.ToString()!=Spec.Material) return Fail(Why,FString(Spec.Name)+TEXT(" original material slot name differs"));
  if (!MaterialMaps(Slot.MaterialInterface,Spec,Why)) return false;
  const UBodySetup* Body=Mesh->GetBodySetup();
  if (!Body || Body->DefaultInstance.GetCollisionEnabled()!=ECollisionEnabled::NoCollision || Body->DefaultInstance.GetCollisionProfileName()!=TEXT("NoCollision"))
   return Fail(Why,FString(Spec.Name)+TEXT(" native decorative NoCollision default differs"));
  for (const FStaticMeshLODResources& LOD:Mesh->GetRenderData()->LODResources)
  {
   if (LOD.Sections.Num()!=1 || LOD.Sections[0].MaterialIndex!=0 || LOD.Sections[0].bEnableCollision ||
       LOD.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords()<1)
    return Fail(Why,FString(Spec.Name)+TEXT(" actual render slot, collision section or atlas UV differs"));
  }
  return true;
 }
 bool LoadAssets(TArray<UStaticMesh*>& Meshes, FString& Why)
 {
  for (const FAsset& Spec:Assets)
  {
   const FString Package=AssetRoot+Spec.Name;
   // Optional licensed content must not emit failed LoadObject errors on a public clone.
   if (!FPackageName::DoesPackageExist(Package)) return Fail(Why,TEXT("private native asset absent: ")+Package);
   UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*ObjectPath(Package));
   if (!AssetValid(Mesh,Spec,Why)) return false;
   Meshes.Add(Mesh);
  }
  return true;
 }
 bool Papers(const FS47Obj& Root, TArray<FPaper>& Out, FString& Why)
 {
  for (const TCHAR* Name:PaperNames)
  {
   const FS47Obj Obj=Root.Find(Name); const FBox Bounds=Obj?Obj.WorldBox():FBox(ForceInit);
   if (!Obj || !Bounds.IsValid) return Fail(Why,TEXT("original live paper absent: ")+FString(Name));
   Out.Add({Name,Obj.I,Bounds});
  }
  return true;
 }
}

bool Build(AS47Web* W)
{
 if (!W || !W->IsLoaded() || FParse::Param(FCommandLine::Get(),TEXT("S47OfficeOwnedOff")) ||
     FParse::Param(FCommandLine::Get(),TEXT("S47PropsOff"))) return false;
 if (Installed.Contains(W)) { TArray<FString> Lines; return Audit(W,Lines); }
 const FS47Obj Annex=W->Obj(TEXT("annex.group")), Root=Annex?Annex.Find(TEXT("operations:room")):FS47Obj();
 const FS47WebNode* Parent=Root?W->Node(Root.I):nullptr;
 if (!Parent || !Parent->Comp.IsValid() || !W->NodeWorldThree(Root.I).Equals(FTransform::Identity,.0001) ||
     !Parent->Comp->GetComponentTransform().Equals(FTransform::Identity,.001)) return false;
 USceneComponent* ParentComponent=Parent->Comp.Get(); // AdoptNode may reallocate W's node array.
 for (int32 I=0;I<W->PartCount();++I) if (const FS47WebNode* N=W->Node(I);N && N->Name.StartsWith(Prefix))
 { UE_LOG(LogTemp,Warning,TEXT("S47: owned Office props skipped: partial or unregistered existing component set")); return false; }
 FInstalled Candidate; Candidate.Parent=Root.I; TArray<UStaticMesh*> Meshes; FString Why;
 if (!LoadAssets(Meshes,Why) || !Papers(Root,Candidate.Papers,Why))
 { UE_LOG(LogTemp,Warning,TEXT("S47: owned Office props skipped: %s; baseline retained"),*Why); return false; }
 double LowestPaper=100;
 for (const FPaper& P:Candidate.Papers) LowestPaper=FMath::Min(LowestPaper,P.Bounds.Min.Y);
 for (int32 I=0;I<Meshes.Num();++I)
  if (S47Three::ToThree(Meshes[I]->GetBoundingBox().TransformBy(Wanted(I)).Max).Y>=LowestPaper-.02)
  { UE_LOG(LogTemp,Warning,TEXT("S47: owned Office props skipped: source prop exceeds protected paper underside")); return false; }
 // All four assets, their original materials/maps and eight paper surfaces have
 // passed before any new component is created. Never hide the baseline furniture.
 for (int32 I=0;I<Meshes.Num();++I)
 {
  const FString Name=Prefix+Ids[I]; UStaticMeshComponent* C=NewObject<UStaticMeshComponent>(W,*Name);
  C->SetupAttachment(ParentComponent); C->SetMobility(EComponentMobility::Movable); C->SetStaticMesh(Meshes[I]);
  C->SetCollisionProfileName(TEXT("NoCollision")); C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCollisionResponseToAllChannels(ECR_Ignore);
  C->SetCastShadow(true); W->AddInstanceComponent(C); C->RegisterComponent(); C->SetWorldTransform(Wanted(I));
  const int32 Node=W->AdoptNode(C,Root.I,Name);
  // AdoptNode initially records position only; set the tipped lid's genuine Three
  // rotation too so component and named scene-node transforms agree in audits.
  W->SetPosition(Node,Position(I)); W->SetRotation(Node,I==3?FMath::DegreesToRadians(75.):0,0,0);
  Candidate.Parts.Add({Node,C,Wanted(I)});
 }
 Installed.Add(W,MoveTemp(Candidate));
 UE_LOG(LogTemp,Display,TEXT("S47: owned Office props placed: cable on lower trolley shelf, closed desk box, open body and supported removable lid; native bindings only, bake and visual acceptance separate"));
 TArray<FString> Lines;
 if (Audit(W,Lines)) return true;
 // A post-placement programming/binding error must still retain the baseline.
 // Adopted nodes hold weak components and never become gameplay handles.
 if (FInstalled* Set=Installed.Find(W)) for (FInstance& Part:Set->Parts) if (UStaticMeshComponent* C=Part.Component.Get()) C->DestroyComponent();
 Installed.Remove(W);
 for (const FString& Line:Lines) UE_LOG(LogTemp,Warning,TEXT("S47: %s"),*Line);
 return false;
}

bool Audit(AS47Web* W, TArray<FString>& Lines)
{
 const FInstalled* Set=W?Installed.Find(W):nullptr;
 if (!Set || Set->Parts.Num()!=4)
 {
  TArray<UStaticMesh*> Check; FString Missing;
  if (W && !LoadAssets(Check,Missing)) Lines.Add(TEXT("Owned Office props FAIL: ")+Missing);
  else Lines.Add(TEXT("Owned Office props FAIL: complete private native set not installed; baseline may be retained"));
  return false;
 }
 TArray<UStaticMesh*> Meshes; FString Why;
 if (!LoadAssets(Meshes,Why)) { Lines.Add(TEXT("Owned Office props FAIL: ")+Why); return false; }
 bool Good=true; const FS47WebNode* Parent=W->Node(Set->Parent);
 if (!Parent || !Parent->Comp.IsValid() || Parent->Name!=TEXT("operations:room"))
 { Lines.Add(TEXT("Owned Office props FAIL: original Operations parent identity differs")); return false; }
 for (int32 I=0;I<Set->Parts.Num();++I)
 {
  const FInstance& P=Set->Parts[I]; UStaticMeshComponent* C=P.Component.Get(); const FS47WebNode* N=W->Node(P.Node);
  const bool Bound=C && N && N->Comp.Get()==C && N->Parent==Set->Parent && N->Name==Prefix+Ids[I] &&
   C->GetAttachParent()==Parent->Comp.Get() && C->GetStaticMesh()==Meshes[I] && C->GetNumOverrideMaterials()==0 &&
   C->GetMaterial(0)==Meshes[I]->GetStaticMaterials()[0].MaterialInterface && C->GetCollisionEnabled()==ECollisionEnabled::NoCollision &&
   C->GetCollisionProfileName()==TEXT("NoCollision") && C->GetComponentTransform().Equals(P.Wanted,.001) &&
   W->NodeWorldThree(P.Node).Equals(FTransform(S47Three::Euler(I==3?FMath::DegreesToRadians(75.):0,0,0),Position(I),FVector::OneVector),.00001) && W->UsableAt(C).IsEmpty();
  Good &= Bound;
  Lines.Add(FString::Printf(TEXT("Owned Office %s %s: original mesh/material, four used PBR maps, parent, shared-origin transform, NoCollision and no interaction override"),Ids[I],Bound?TEXT("PASS"):TEXT("FAIL")));
 }
 for (const FPaper& P:Set->Papers)
 {
  const FS47WebNode* N=W->Node(P.Node); const FBox Actual=N?W->ObjAt(P.Node).WorldBox():FBox(ForceInit);
  const bool Same=N && N->Name==P.Name && Actual.IsValid && Actual.Min.Equals(P.Bounds.Min,.0001) && Actual.Max.Equals(P.Bounds.Max,.0001);
  Good &= Same; Lines.Add(TEXT("Owned Office protected paper ")+P.Name+(Same?TEXT(" PASS: original node and bounds unchanged"):TEXT(" FAIL: original node or bounds differs")));
 }
 Lines.Add(TEXT("Owned Office audit scope: native types, geometry metadata, original material/texture identities and transforms. Bake pixels, appearance and gameplay collision route are separate acceptance."));
 return Good;
}
}

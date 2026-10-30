#include "S47PeriodFixtures.h"

#include "S47Main.h"
#include "S47SaroLegacyCleanup.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/RectLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47PeriodFixtures, Log, All);

namespace S47PeriodFixtures
{
namespace
{
 using S47SaroLegacyCleanup::FEdit;
 struct FFixture
 {
  TWeakObjectPtr<UStaticMeshComponent> Body, Lens;
  TWeakObjectPtr<UMaterialInstanceDynamic> Glow;
  TWeakObjectPtr<URectLightComponent> Driver;
  int32 Parent=-1, OriginalTube=-1;
  FVector DriverAt=FVector::ZeroVector;
  float ReferenceLumens=36.f;
 };
 struct FState
 {
  TWeakObjectPtr<AS47Web> Web;
  TArray<FFixture> Fixtures;
  bool bCorridor=false, bControl=false, bReported=false;
  double Retry=0;
 };
 TMap<TWeakObjectPtr<US47Main>,FState> States;

 UStaticMesh* Asset(const TCHAR* Name)
 {
  const FString Package=FString(TEXT("/Game/Licensed/Saro1986/"))+Name;
  if (!FPackageName::DoesPackageExist(Package)) return nullptr;
  return LoadObject<UStaticMesh>(nullptr,*(Package+TEXT(".")+Name),nullptr,LOAD_NoWarn|LOAD_Quiet);
 }

 FEdit Cut(const FString& Id,const FVector& Min,const FVector& Max,const TCHAR* Slot=nullptr)
 {
  FEdit E; E.Id=Id; E.RootPath=TEXT("room.group"); E.GeometryIds={54};
  E.ThreeBounds=FBox(Min,Max); E.MaxTriangles=2000;
  if (Slot) E.MaterialSlotNames.Add(FName(Slot));
  return E;
 }

 // Every box is one measured authored part, with its exact material slot. The
 // ceiling's T-grid shares steel but lies outside the side-rail/endcap boxes.
 TArray<FEdit> CorridorCuts()
 {
  TArray<FEdit> Edits;
  for (int32 I=0; I<2; ++I)
  {
   const double X=I==0?-3.7:.2, Z=5.7;
   const FString Prefix=FString::Printf(TEXT("tripo-fixture-corridor-%d-"),I);
   Edits.Add(Cut(Prefix+TEXT("back"),FVector(X-.690,2.790,Z-.230),FVector(X+.690,2.800,Z+.230),TEXT("M_Office_CabinetEnamel")));
   for (int32 Sign:{-1,1})
   {
    const double End=X+Sign*.657, Edge=Z+Sign*.190;
    Edits.Add(Cut(Prefix+FString::Printf(TEXT("end%d"),Sign),FVector(End-.0225,2.710,Z-.2025),FVector(End+.0225,2.800,Z+.2025),TEXT("M_Office_CreamEnamel")));
    Edits.Add(Cut(Prefix+FString::Printf(TEXT("rail%d"),Sign),FVector(X-.645,2.7225,Edge-.0125),FVector(X+.645,2.7875,Edge+.0125),TEXT("M_Office_CabinetEnamel")));
    const double LensZ=Z+Sign*.085;
    Edits.Add(Cut(Prefix+FString::Printf(TEXT("lens%d"),Sign),FVector(X-.620,2.706,LensZ-.0725),FVector(X+.620,2.724,LensZ+.0725),I==0?TEXT("M_Office_CorridorTubeWest"):TEXT("M_Office_CorridorTubeEast")));
    const double Screw=X+Sign*.59;
    Edits.Add(Cut(Prefix+FString::Printf(TEXT("screw%d"),Sign),FVector(Screw-.010,2.7065,Z-.010),FVector(Screw+.010,2.7095,Z+.010),TEXT("M_Office_BrushedMetal")));
   }
  }
  return Edits;
 }

 UStaticMeshComponent* Part(AS47Web* W,UStaticMesh* Mesh,int32 Parent,const FString& Name,
                            const FVector& Top,const FVector& Scale,bool Shadow)
 {
  const auto* N=W->Node(Parent);
  if (!N || !N->Comp.IsValid()) return nullptr;
  auto* C=NewObject<UStaticMeshComponent>(W,*Name);
  C->SetupAttachment(N->Comp.Get()); C->SetMobility(EComponentMobility::Movable); C->SetStaticMesh(Mesh);
  C->SetCollisionProfileName(TEXT("NoCollision")); C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->SetGenerateOverlapEvents(false); C->SetCanEverAffectNavigation(false); C->SetCastShadow(Shadow);
  W->AddInstanceComponent(C); C->RegisterComponent();
  C->SetWorldTransform(FTransform(FQuat::Identity,S47Three::ToUE(Top),Scale));
  W->AdoptNode(C,Parent,Name);
  return C;
 }

 void Add(FState& S,AS47Web* W,UStaticMesh* Body,UStaticMesh* Lens,int32 Parent,
          const FString& Name,const FVector& Top,const FVector& Scale,int32 Tube,bool Corridor)
 {
  FFixture F; F.Parent=Parent; F.OriginalTube=Tube; F.ReferenceLumens=Corridor?32.f:36.f;
  F.Body=Part(W,Body,Parent,Name+TEXT("Body"),Top,Scale,true);
  F.Lens=Part(W,Lens,Parent,Name+TEXT("Lens"),Top,Scale,false);
  if (F.Lens.IsValid())
  {
   auto* D=UMaterialInstanceDynamic::Create(F.Lens->GetMaterial(0),W);
   F.Lens->SetMaterial(0,D); D->SetVectorParameterValue(TEXT("EmissiveFactor"),FLinearColor::Black); F.Glow=D;
  }
  F.DriverAt=S47Three::ToUE(FVector(Top.X,Corridor?2.696:2.910,Top.Z));
  S.Fixtures.Add(F);
 }

 bool BuildCorridor(FState& S,AS47Web* W,UStaticMesh* Body,UStaticMesh* Lens)
 {
  const FS47Obj Annex=W->Obj(TEXT("annex.group"));
  const FS47Obj Room=Annex?Annex.Find(TEXT("office:authoredRoom")):FS47Obj();
  const auto* N=W->Node(Room.I);
  auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
  const TCHAR* CpuPath=TEXT("/Game/Signal47/Art/Saro1986/SM_Saro1986_OfficeRoomCpu");
  if (!C || !FPackageName::DoesPackageExist(CpuPath)) return false;
  auto* Cpu=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Signal47/Art/Saro1986/SM_Saro1986_OfficeRoomCpu.SM_Saro1986_OfficeRoomCpu"));
  if (!Cpu || Cpu->IsCompiling()) return false;
  UStaticMesh* Original=C->GetStaticMesh();
  TArray<UMaterialInterface*> Materials=C->GetMaterials();
  C->SetStaticMesh(Cpu);
  for (int32 I=0; I<Materials.Num(); ++I) C->SetMaterial(I,Materials[I]);
  FString Why;
  if (!S47SaroLegacyCleanup::ApplyToComponent(W,C,CorridorCuts(),&Why))
  {
   C->SetStaticMesh(Original);
   for (int32 I=0; I<Materials.Num(); ++I) C->SetMaterial(I,Materials[I]);
   if (!S.bReported) UE_LOG(LogS47PeriodFixtures,Warning,TEXT("S47: Tripo corridor fixtures wait: %s"),*Why);
   S.bReported=true; return false;
  }
  for (int32 I=0; I<2; ++I)
   Add(S,W,Body,Lens,Annex.I,FString::Printf(TEXT("saro:tripoCorridor%d"),I),
       FVector(I==0?-3.7:.2,2.800,5.7),FVector(1.34/1.30,.405/.36,.100/.080),-1,true);
  UE_LOG(LogS47PeriodFixtures,Display,TEXT("S47: Tripo H3.1 corridor fixtures placed; original two electrical drivers retained"));
  return true;
 }

 bool BuildControl(FState& S,AS47Web* W,UStaticMesh* Body,UStaticMesh* Lens)
 {
  const FS47Obj Room=W->Obj(TEXT("room.group"));
  if (!Room) return false;
  const FVector2D Positions[]={{-3.5,-1.8},{0,-1.8},{3.5,-1.8},{-3.5,1.8},{0,1.8},{3.5,1.8}};
  TArray<FEdit> Edits;
  for (int32 I=0; I<6; ++I)
  {
   const auto P=Positions[I];
   FEdit E=Cut(FString::Printf(TEXT("tripo-control-housing%d"),I),
               FVector(P.X-.650,2.940,P.Y-.180),FVector(P.X+.650,3.000,P.Y+.180));
   E.MinTriangles=12; E.MaxTriangles=12; Edits.Add(E);
  }
  FString Why;
  if (!S47SaroLegacyCleanup::Apply(W,Edits,&Why))
  {
   if (!S.bReported) UE_LOG(LogS47PeriodFixtures,Warning,TEXT("S47: Tripo control fixtures wait: %s"),*Why);
   S.bReported=true; return false;
  }
  for (int32 I=0; I<6; ++I)
   Add(S,W,Body,Lens,Room.I,FString::Printf(TEXT("saro:tripoControl%d"),I),
       FVector(Positions[I].X,2.997,Positions[I].Y),FVector::OneVector,177+I,false);
  UE_LOG(LogS47PeriodFixtures,Display,TEXT("S47: six Tripo H3.1 control fixtures placed; original four powered and two unpowered lamps retained"));
  return true;
 }
}

void Update(US47Main* M,double Dt)
{
 if (!M || FParse::Param(FCommandLine::Get(),TEXT("S47Saro1986Off")) ||
     FParse::Param(FCommandLine::Get(),TEXT("S47PeriodFixturesOff"))) return;
 for (auto It=States.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
 AS47Web* W=M->Web.Get(); if (!W || !W->IsLoaded()) return;
 FState& S=States.FindOrAdd(TWeakObjectPtr<US47Main>(M));
 if (S.Web.Get()!=W) { S=FState(); S.Web=W; }
 S.Retry-=Dt;
 if ((!S.bCorridor || !S.bControl) && S.Retry<=0)
 {
  S.Retry=2;
  UStaticMesh* Body=Asset(TEXT("SM_Saro_FluorescentBody"));
  UStaticMesh* Lens=Asset(TEXT("SM_Saro_FluorescentLens"));
  if (Body && Lens && !Body->IsCompiling() && !Lens->IsCompiling())
  {
   if (!S.bCorridor) S.bCorridor=BuildCorridor(S,W,Body,Lens);
   if (!S.bControl) S.bControl=BuildControl(S,W,Body,Lens);
  }
 }
 TArray<URectLightComponent*> Lights;
 for (FFixture& F:S.Fixtures)
 {
  if (!F.Driver.IsValid())
  {
   if (Lights.IsEmpty()) W->GetComponents(Lights);
   for (auto* L:Lights)
    if (L && L->GetComponentLocation().Equals(F.DriverAt,.02) &&
        FMath::IsNearlyEqual(L->SourceWidth,F.ReferenceLumens==32.f?124.f:120.f,.01f) &&
        FMath::IsNearlyEqual(L->SourceHeight,F.ReferenceLumens==32.f?30.f:26.f,.01f)) F.Driver=L;
  }
  // Render visibility is independent of the original node's bVis, which still
  // drives electrical state, story flicker, and the real source rect light.
  if (const auto* Old=W->Node(F.OriginalTube)) if (auto* C=Cast<UPrimitiveComponent>(Old->Comp.Get()))
  { C->SetHiddenInGame(true); C->SetVisibility(false); C->SetCastShadow(false); }
  const bool Visible=M->Mode==US47Main::EMode::Play && M->World && M->World->Area==TEXT("saro") &&
      W->ObjAt(F.Parent).Shown() && (F.ReferenceLumens!=32.f || W->Obj(TEXT("annex.interior")).Shown());
  if (F.Body.IsValid()) F.Body->SetVisibility(Visible);
  if (F.Lens.IsValid()) F.Lens->SetVisibility(Visible);
  auto* Driver=F.Driver.Get();
  const float K=Visible && Driver && Driver->IsVisible()?FMath::Max(0.f,Driver->Intensity/F.ReferenceLumens):0.f;
  if (F.Glow.IsValid()) F.Glow->SetVectorParameterValue(TEXT("EmissiveFactor"),Driver?Driver->GetLightColor()*(1.4f*K):FLinearColor::Black);
 }
}
}

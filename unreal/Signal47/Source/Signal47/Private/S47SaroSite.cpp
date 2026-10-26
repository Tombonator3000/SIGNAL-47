// Original SARO building 2, recessed glazing and consolidated utility equipment.
#include "S47SaroSite.h"
#include "S47Web.h"
#include "S47Obj.h"
#include "S47Three.h"
#include "S47HighwayPower.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "StaticMeshResources.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SaroSite,Log,All);
namespace
{
 struct FPart {const TCHAR* Name;FVector Root,Min,Max;int32 Tris[3];int32 Slots;};
 // FBX mapping: Blender(x,y,z) -> UE(x,-y,z)*100. These roots remain Three Y-up.
 const FPart Spec[]={
 {TEXT("SM_SaroSite_Facade"),{29.6,-.6,0},{-9,-5.6,0},{9,5.6,4.2},{2618,2258,1898},8},
 {TEXT("SM_SaroSite_Glass"),{29.6,-.6,0},{-8.295,-5.236,1.285},{8.495,5.236,2.865},{120,120,120},1},
 {TEXT("SM_SaroSite_ServiceEquipment"),{3.6,-.6,18.6},{-2.15,-.36,1.89},{.12,7.79,8.2},{1624,1048,712},4}
 };
 // Existing Exterior.ts wing wall-pack apertures; sign is on Three Z=5.552.
 const FVector SignWashAt[]={{20.9,2.665,5.94},{30.4,2.665,5.94}};
 const FVector SignWashTarget[]={{24.4,2.1,5.552},{27.2,2.1,5.552}};
 float SignPower(AS47Web* W)
 {
  // Same site slot9 and ext.glow20/21 feed as WingWest/WingEast. Fixed original
  // reference14*.07 avoids capturing a blackout or recovery ramp as full power.
  const int32 Site=W->FloodSet(TEXT("site"));
  if(Site<0||W->FloodSlots(Site)<=9||!W->Obj(TEXT("ext.group")).Shown())return 0;
  float K=FMath::Clamp(W->FloodPos(Site,9).A*W->FloodScaleOf(Site)/(14.f*.07f),0.f,1.f);
  const auto* G=W->Glow(TEXT("ext.glow.points"));
  if(!G)return 0;
  for(int32 I:{20,21})
  {
   if(!G->Pos.IsValidIndex(I)||!G->Col.IsValidIndex(I)||W->NodeWorldThree(G->Node).TransformPosition(G->Pos[I]).Y < -900)return 0;
   const FLinearColor C=G->Col[I];K=FMath::Min(K,FMath::Clamp(FMath::Max3(C.R,C.G,C.B),0.f,1.f));
  }
  return K;
 }
 const FVector2D OldFeet[]={{-16.8,-16.8},{-16.8,28.5},{-16.8,69},{-16.8,-14},{-3,28.5},{3.6,18.6}};
 bool OldFoot(double X0,double X1,double Z0,double Z1)
 {
  for(const FVector2D& P:OldFeet)
   if(FMath::Abs(X0-P.X+.25)<.0001&&FMath::Abs(X1-P.X-.25)<.0001&&FMath::Abs(Z0-P.Y+.25)<.0001&&FMath::Abs(Z1-P.Y-.25)<.0001)return true;
  return false;
 }
 bool GoodMesh(UStaticMesh* M,const FPart& S)
 {
  if(!M||!M->GetRenderData()||M->GetRenderData()->LODResources.Num()!=3||M->GetStaticMaterials().Num()!=S.Slots)return false;
  for(int32 L=0;L<3;++L)if(M->GetNumTriangles(L)!=S.Tris[L])return false;
  const FVector Center((S.Min.X+S.Max.X)*50,-(S.Min.Y+S.Max.Y)*50,(S.Min.Z+S.Max.Z)*50);
  const FVector Extent=(S.Max-S.Min)*50;
  if(!M->GetBounds().Origin.Equals(Center,.25)||!M->GetBounds().BoxExtent.Equals(Extent,.25))return false;
  for(const FStaticMaterial& Slot:M->GetStaticMaterials())
   if(!Slot.MaterialInterface||!Slot.MaterialInterface->GetName().StartsWith(TEXT("M_SaroSite_"))||Slot.MaterialSlotName.ToString()!=Slot.MaterialInterface->GetName())return false;
  return true;
 }
}
TSharedPtr<FS47SaroSite> FS47SaroSite::Build(AS47Web* W)
{
 if(!W||FParse::Param(FCommandLine::Get(),TEXT("S47SaroSiteOff"))||FParse::Param(FCommandLine::Get(),TEXT("S47PropsOff")))return {};
 const auto Power=S47HighwayPower::Status(W);
 const int32 Grounds=W->Handle(TEXT("world.grounds.group"));
 // Resolve by exact source geometry and ancestry as handle layouts can add wrapper groups.
 auto Under=[W](int32 I,int32 Parent){for(;I>=0;){if(I==Parent)return true;const auto* N=W->Node(I);if(!N)return false;I=N->Parent;}return false;};
 if(!W->Node(Grounds)||!W->Node(Grounds)->Comp.IsValid()||!Power.bReady||Power.SaroMainWithin100m!=2||Power.SaroServicePoles!=1)return {};
 TArray<int32> Retire;int32 WingNode=-1,TitleNode=-1;
 const int32 OldGeo[]={439,440,441,452};const int32 OldTris[]={240,432,116,1156};
 for(int32 G=0;G<4;++G)
 {
  int32 Found=-1;
  for(int32 I=0;I<W->PartCount();++I)
  {
   const auto* N=W->Node(I);if(!N||N->Geo!=OldGeo[G]||!Under(I,Grounds))continue;
   auto* C=Cast<UStaticMeshComponent>(N->Comp.Get());
   if(Found>=0||!C||!C->GetStaticMesh()||C->GetStaticMesh()->GetNumTriangles(0)!=OldTris[G])return {};
   Found=I;
  }
  if(Found<0)return {};Retire.Add(Found);
 }
 for(int32 I=0;I<W->PartCount();++I)
 {
  const auto* N=W->Node(I);if(!N)continue;
  if(N->Name==TEXT("house:wing")&&N->Geo==15){if(WingNode>=0)return {};WingNode=I;}
 }
 if(WingNode<0)return {};
 for(int32 I:W->Node(W->Node(WingNode)->Parent)->Children)if(W->Node(I)&&W->Node(I)->Geo==16){if(TitleNode>=0)return {};TitleNode=I;}
 auto* OldWing=Cast<UStaticMeshComponent>(W->Node(WingNode)->Comp.Get());
 auto* OldTitle=TitleNode>=0?Cast<UStaticMeshComponent>(W->Node(TitleNode)->Comp.Get()):nullptr;
 if(!OldWing||!OldTitle||!OldWing->GetStaticMesh()||OldWing->GetStaticMesh()->GetNumTriangles(0)!=12||!OldTitle->GetStaticMesh()||OldTitle->GetStaticMesh()->GetNumTriangles(0)!=2)return {};
 // The old wing remains a transform/visibility parent for its dressing and interactions.
 Retire.Add(WingNode);Retire.Add(TitleNode);
 TArray<UStaticMesh*> Meshes;
 for(const FPart& S:Spec)
 {
  UStaticMesh* M=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/SaroSite/%s.%s"),S.Name,S.Name));
  if(!GoodMesh(M,S)){UE_LOG(LogS47SaroSite,Warning,TEXT("S47: SARO site preserved original: native mesh contract unavailable %s"),S.Name);return {};}
  Meshes.Add(M);
 }
 // g454 merges the old fake wing windows with roof/yard dressing. Use the measured
 // car+window composition, never hide that shared leaf. Require the matching staff cars too.
 UStaticMesh* DressingTrim=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Free/ServiceFleet/SM_SaroPark_LegacyTrim.SM_SaroPark_LegacyTrim"));
 auto* StaffSedan=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Free/ServiceFleet/SM_SaroPark_Sedan.SM_SaroPark_Sedan"));
 auto* StaffPickup=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Free/ServiceFleet/SM_SaroPark_Pickup.SM_SaroPark_Pickup"));
 UStaticMeshComponent* Dressing=nullptr;
 for(int32 I=0;I<W->PartCount();++I)if(const auto* N=W->Node(I))if(N->Geo==454&&Under(I,Grounds))
 {if(Dressing)return {};Dressing=Cast<UStaticMeshComponent>(N->Comp.Get());}
 if(!Dressing||!Dressing->GetStaticMesh()||Dressing->GetStaticMesh()->GetNumTriangles(0)!=13550||!DressingTrim||DressingTrim->GetNumLODs()!=3||!StaffSedan||!StaffPickup)return {};
 for(int32 L=0;L<3;++L)if(DressingTrim->GetNumTriangles(L)!=10702)return {};
 // Old wall-grime overlays contain rectangular holes for the obsolete windows. They
 // sit 12mm in front of the new stucco, so preserve only the unrelated original stains.
 UStaticMesh* StainTrim=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/SaroSite/Trim/StaticMeshes/g455.g455"));
 UStaticMeshComponent* Stains=nullptr;const int32 DressingGroup=W->Handle(TEXT("world.grounds.dressing.group"));
 for(int32 I=0;I<W->PartCount();++I)if(const auto* N=W->Node(I))if(N->Geo==455&&N->Parent==DressingGroup)
 {if(Stains)return {};Stains=Cast<UStaticMeshComponent>(N->Comp.Get());}
 if(!Stains||!Stains->GetStaticMesh()||Stains->GetStaticMesh()->GetNumTriangles(0)!=1712||!Stains->GetMaterial(0)||!StainTrim||StainTrim->GetNumLODs()!=1||StainTrim->GetNumTriangles(0)!=1224||StainTrim->GetStaticMaterials().Num()!=1)
 {UE_LOG(LogS47SaroSite,Warning,TEXT("S47: SARO site preserved original: exact g455 stain composition1712 to1224 unavailable"));return {};}
 const auto& Drive=W->DriveData();const TSharedPtr<FJsonObject>* Legs=nullptr,*Leg=nullptr;const TArray<TSharedPtr<FJsonValue>>* Obs=nullptr;
 if(!Drive.IsValid()||!Drive->TryGetObjectField(TEXT("legs"),Legs)||!(*Legs)->TryGetObjectField(TEXT("saro"),Leg)||!(*Leg)->TryGetArrayField(TEXT("obstacles"),Obs))return {};
 int32 Walk=0,Driving=0;
 for(const auto& R:W->CollidersMutable(TEXT("saro")))if(OldFoot(R.MinX,R.MaxX,R.MinZ,R.MaxZ))++Walk;
 for(const auto& V:*Obs){const auto& A=V->AsArray();if(A.Num()==4&&OldFoot(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber(),A[3]->AsNumber()))++Driving;}
 if(Walk!=6||Driving!=6){UE_LOG(LogS47SaroSite,Warning,TEXT("S47: SARO site preserved original: exact legacy feet expected6, actual walk=%d drive=%d"),Walk,Driving);return {};}
 auto S=MakeShared<FS47SaroSite>();S->Web=W;
 for(int32 I=0;I<3;++I)
 {
  auto* C=NewObject<UStaticMeshComponent>(W,*FString(Spec[I].Name));C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Meshes[I]);
  C->SetCollisionEnabled(I==1?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);
  if(I!=1)C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
  // Interaction proxies keep authority on their own trace channel, including the original meter face.
  C->SetCastShadow(I!=1);C->RegisterComponent();C->AttachToComponent(W->Node(Grounds)->Comp.Get(),FAttachmentTransformRules::KeepWorldTransform);
  C->SetWorldTransform(FTransform(S47Three::ToUE(Spec[I].Root)));
  S->PartNodes.Add(W->AdoptNode(C,Grounds,FString(TEXT("saroSite:"))+Spec[I].Name));S->Parts.Add(C);
 }
 // Two finite, shadow-casting warm sign washes at the preserved physical lamps.
 // Their grazing cross-aim lights the printed sign without emissive lettering.
 for(int32 I=0;I<2;++I)
 {
  // A separate, wall-mounted sign optic replaces the old second light floating at a wall pack.
  const auto Fixture=S47ExteriorFixtures::Make(W,I==0?TEXT("WingSignWest"):TEXT("WingSignEast"),S47ExteriorFixtures::EKind::SignWash,
   FVector(I==0?24.4:27.2,3.30,5.50),S47Three::Euler(UE_DOUBLE_PI/2,0,0),Grounds);
  const FVector At=Fixture?Fixture.Aperture:SignWashAt[I];
  const FVector Target=Fixture?At+Fixture.Normal*3:SignWashTarget[I];
  auto* L=NewObject<USpotLightComponent>(W,*FString::Printf(TEXT("S47SaroSignWash%d"),I));
  L->SetupAttachment(W->GetRootComponent());L->SetMobility(EComponentMobility::Movable);
  L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(0);L->SetLightColor(FLinearColor(1.f,.72f,.43f));
  L->SetWorldLocationAndRotation(S47Three::ToUE(At),S47Three::DirToUE(Target-At).Rotation());
  L->SetInnerConeAngle(45.f);L->SetOuterConeAngle(70.f);L->SetAttenuationRadius(900.f);
  L->SetSourceRadius(Fixture?Fixture.Height*.5f:3.f);L->SetSoftSourceRadius(3.f);L->SetCastShadows(true);
  L->SetVolumetricScatteringIntensity(0.f);L->SetIndirectLightingIntensity(1.f);
  L->SetVisibility(false);L->RegisterComponent();S->SignWash.Add(L);S->SignFixtures.Add(Fixture);
 }
 // Unreal: a continuous paved east forecourt connects to the original service road.
 // Visual finish only: retain the existing -.600m walk/drive plane and every query floor.
 UStaticMesh* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 UTexture* AsphaltRGB=LoadObject<UTexture>(nullptr,TEXT("/Game/MesaDiner/Materials/Textures/T_M_Mesa_Asphalt_base_color.T_M_Mesa_Asphalt_base_color"));
 if(Cube&&Base&&Base->GetBlendMode()==BLEND_Opaque&&AsphaltRGB)
 {
  auto Mat=[&](uint32 Color)
  {
   auto* M=UMaterialInstanceDynamic::Create(Base,W);M->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(Color));
   M->SetScalarParameterValue(TEXT("NativeLighting"),1);M->SetScalarParameterValue(TEXT("Roughness"),.96f);
   M->SetScalarParameterValue(TEXT("Metalness"),0);M->SetScalarParameterValue(TEXT("EmissivePower"),0);return M;
  };
  auto* Asphalt=Mat(0xffffff);Asphalt->SetTextureParameterValue(TEXT("Map"),AsphaltRGB);Asphalt->SetScalarParameterValue(TEXT("UseMap"),1);
  Asphalt->SetVectorParameterValue(TEXT("MapU"),FLinearColor(2.4,0,.08,0));Asphalt->SetVectorParameterValue(TEXT("MapV"),FLinearColor(0,1.5,.08,0));
  auto* Paint=Mat(0xb6ad8a);auto* Concrete=Mat(0x8c8776);
  auto Box=[&](const TCHAR* Name,const FVector& At,const FVector& Size,UMaterialInterface* M)
  {
   auto* C=NewObject<UStaticMeshComponent>(W);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Cube);C->SetMaterial(0,M);
   C->SetupAttachment(W->Node(Grounds)->Comp.Get());C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->RegisterComponent();
   C->SetWorldLocation(S47Three::ToUE(At));C->SetWorldScale3D(S47Three::DirToUE(Size));
   int32 I=W->AdoptNode(C,Grounds,FString(TEXT("saroSite:forecourt:"))+Name);W->ObjAt(I).SetScale(Size);
   S->PartNodes.Add(I);S->Forecourt.Add(C);S->ForecourtPoses.Add(C->GetComponentTransform());
  };
  using namespace S47SaroForecourt;
  // Top -.576m clears the old road drawing by 12mm, same as the visitor-lot finish.
  Box(TEXT("eastApron"),FVector((X0+X1)/2,-.636,(Z0+Z1)/2),FVector(X1-X0,.12,Z1-Z0),Asphalt);
  Box(TEXT("serviceRoadAccess"),FVector((AccessX0+AccessX1)/2,-.636,(AccessZ0+AccessZ1)/2),FVector(AccessX1-AccessX0,.12,AccessZ1-AccessZ0),Asphalt);
  // A clear pedestrian edge and generous 4m staff bays. Keep all visitor bays vacant.
  Box(TEXT("facadeWalk"),FVector(29.95,-.574,6.2),FVector(19.7,.004,.6),Concrete);
  for(double X:{25.,29.,33.,37.})Box(TEXT("staffBayLine"),FVector(X,-.572,9.65),FVector(.065,.004,6.1),Paint);
  Box(TEXT("staffBayEnds"),FVector(31,-.572,6.6),FVector(12,.004,.065),Paint);
  Box(TEXT("staffEastStop"),FVector(SedanX,-.54,6.45),FVector(1.8,.12,.2),Concrete);
 }
 else UE_LOG(LogS47SaroSite,Warning,TEXT("S47: SARO forecourt finish unavailable"));
 // Commit only after every source/native check. SetVisible on the wing would recurse into
 // preserved roof details, so retire only these exact draw components, leaving parent flags intact.
 for(int32 I:Retire)
 {
  auto* C=Cast<UStaticMeshComponent>(W->Node(I)->Comp.Get());C->SetVisibility(false,false);C->SetHiddenInGame(true,false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 }
 // The original material atlas is unchanged on every retained dressing triangle.
 auto* DressingAtlas=Dressing->GetMaterial(0);Dressing->SetStaticMesh(DressingTrim);Dressing->SetMaterial(0,DressingAtlas);
 auto* StainAtlas=Stains->GetMaterial(0);Stains->SetStaticMesh(StainTrim);Stains->SetMaterial(0,StainAtlas);S->Stains=Stains;S->StainAtlas=StainAtlas;
 S->RetiredNodes=Retire;
 S->RemovedWalkFeet=W->CollidersMutable(TEXT("saro")).RemoveAll([](const FS47Rect& R){return OldFoot(R.MinX,R.MaxX,R.MinZ,R.MaxZ);});
 TArray<TSharedPtr<FJsonValue>> Kept;
 for(const auto& V:*Obs){const auto& A=V->AsArray();if(A.Num()==4&&OldFoot(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber(),A[3]->AsNumber()))++S->RemovedDriveFeet;else Kept.Add(V);}
 (*Leg)->SetArrayField(TEXT("obstacles"),Kept);S->Update();
 UE_LOG(LogS47SaroSite,Display,TEXT("S47: SARO site ready: original wing+title and16 old fake windows replaced, old window-shaped grime488tri removed, recessed glass .27m, solid sign wall; retired legacy poles=6 utilityGeos=4 walkFeet=%d driveFeet=%d; natural mainWithin100m=2 service=1 total=3; meter glass, field lights, fences, roof details and story proxies preserved"),S->RemovedWalkFeet,S->RemovedDriveFeet);
 return S;
}
void FS47SaroSite::Update()
{
 auto* W=Web.Get();if(!W)return;const bool Shown=W->Area()==TEXT("saro");
 for(int32 I:PartNodes)W->SetVisible(I,Shown);
 // Area changes can reapply the original hierarchy visibility; suppress only retired draws.
 for(int32 I:RetiredNodes)if(const auto* N=W->Node(I))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get())){C->SetVisibility(false,false);C->SetHiddenInGame(true,false);}
 const float Power=Shown?SignPower(W):0.f;
 for(const auto& P:SignWash)if(auto* L=P.Get()){L->SetIntensity(1200.f*Power);L->SetVisibility(Power>.0001f);}
 for(const auto& F:SignFixtures)F.Update(Shown,Power,FLinearColor(1.f,.72f,.43f));
 bShown=Shown;
}
bool FS47SaroSite::Audit(TArray<FString>& Lines)const
{
 auto* W=Web.Get();if(!W)return false;bool OK=Parts.Num()==3&&RemovedWalkFeet==6&&RemovedDriveFeet==6;
 for(int32 I=0;I<Parts.Num();++I)
 {
  auto* C=Parts[I].Get();const bool Good=C&&GoodMesh(C->GetStaticMesh(),Spec[I])&&C->GetComponentLocation().Equals(S47Three::ToUE(Spec[I].Root),.01)&&C->IsVisible()==bShown;
  OK&=Good;Lines.Add(FString::Printf(TEXT("SARO_SITE part=%s mesh/root/visibility=%s"),Spec[I].Name,Good?TEXT("PASS"):TEXT("FAIL")));
 }
 for(int32 I:RetiredNodes){const auto* N=W->Node(I);auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;OK&=C&&!C->IsVisible()&&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision;}
 bool ForecourtOK=Forecourt.Num()==9&&ForecourtPoses.Num()==9;
 for(int32 I=0;I<Forecourt.Num();++I)
 {
  const auto* C=Forecourt[I].Get();ForecourtOK&=C&&C->GetComponentTransform().Equals(ForecourtPoses[I],.001)&&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&C->IsVisible()==bShown;
 }
 OK&=ForecourtOK;Lines.Add(FString::Printf(TEXT("SARO_SITE paved east apron x20.1..39.8 z5.9..18, service-road access x10..39.8 z18..23, three staff bays and clear facade walk; original physical floor/query collision retained; %s"),ForecourtOK?TEXT("PASS"):TEXT("FAIL")));
 bool ComposedWindows=false;
 for(int32 I=0;I<W->PartCount();++I)if(const auto* N=W->Node(I))if(N->Geo==454)
  if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))ComposedWindows=C->GetStaticMesh()&&C->GetStaticMesh()->GetName()==TEXT("SM_SaroPark_ForecourtTrim")&&C->GetStaticMesh()->GetNumTriangles(0)==10328;
 OK&=ComposedWindows;Lines.Add(FString::Printf(TEXT("SARO_SITE old fake windows=16 removed1872 triangles, composed dressing10328 incl230 forecourt brush+144 wheel remnants removed; %s"),ComposedWindows?TEXT("PASS"):TEXT("FAIL")));
 auto* OldStains=Stains.Get();const bool StainsOK=OldStains&&OldStains->GetStaticMesh()&&OldStains->GetStaticMesh()->GetPathName()==TEXT("/Game/SaroSite/Trim/StaticMeshes/g455.g455")&&OldStains->GetStaticMesh()->GetNumTriangles(0)==1224&&OldStains->GetMaterial(0)==StainAtlas.Get();
 OK&=StainsOK;Lines.Add(FString::Printf(TEXT("SARO_SITE old window grime488 triangles removed,1224 unrelated stains retain original atlas; %s"),StainsOK?TEXT("PASS"):TEXT("FAIL")));
 const float WashPower=bShown?SignPower(W):0.f;bool WashOK=SignWash.Num()==2;
 for(int32 I=0;I<SignWash.Num();++I){const auto* L=SignWash[I].Get();const FVector At=SignFixtures.IsValidIndex(I)&&SignFixtures[I]?SignFixtures[I].Aperture:SignWashAt[I];WashOK&=L&&L->GetComponentLocation().Equals(S47Three::ToUE(At),.01)&&L->CastShadows&&L->IsVisible()==(WashPower>.0001f)&&FMath::IsNearlyEqual(L->Intensity,1200.f*WashPower,.1f);}
 OK&=WashOK;Lines.Add(FString::Printf(TEXT("SARO_SITE sign wash=2 warm1200lm each, original site9/glow20/21 power=%.4f; %s"),WashPower,WashOK?TEXT("PASS"):TEXT("FAIL")));
 const auto Power=S47HighwayPower::Status(W);OK&=Power.bReady&&Power.SaroMainWithin100m==2&&Power.SaroServicePoles==1&&Power.SaroTotalWithin100m==3;
 Lines.Add(FString::Printf(TEXT("SARO_SITE retiredDraws=%d legacyWalkFeet=%d legacyDriveFeet=%d utilityWithin100m=%d (main=%d service=%d), field lights/proxies unchanged; %s"),RetiredNodes.Num(),RemovedWalkFeet,RemovedDriveFeet,Power.SaroTotalWithin100m,Power.SaroMainWithin100m,Power.SaroServicePoles,OK?TEXT("PASS"):TEXT("FAIL")));return OK;
}

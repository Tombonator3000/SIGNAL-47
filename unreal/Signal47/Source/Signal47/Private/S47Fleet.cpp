#include "S47Fleet.h"
#include "S47TripoFleet.h"
#include "S47Truck.h"
#include "S47Three.h"
#include "S47SaroSite.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StaticMeshResources.h"
#include "RenderUtils.h"
DEFINE_LOG_CATEGORY_STATIC(LogS47Fleet,Log,All);
namespace
{
 UStaticMesh* Asset(const TCHAR* Name)
 {
  const FString N(Name);const bool Own=N==TEXT("SM_Saro07_Cabin")||N==TEXT("SM_Saro07_Branding")||N==TEXT("SM_Saro07_HeadLens")||N==TEXT("SM_Saro07_TailLens");
  const FString P=FString(Own?TEXT("/Game/SaroFleet/"):TEXT("/Game/Free/ServiceFleet/"))+Name+TEXT(".")+Name;
  return LoadObject<UStaticMesh>(nullptr,*P);
 }
 UStaticMeshComponent* Part(AS47Web* W,UStaticMesh* Mesh,int32 Parent,const FString& Name,const FVector& At=FVector::ZeroVector,double H=0)
 {
  const FS47WebNode* N=W->Node(Parent);if(!N||!N->Comp.IsValid())return nullptr;
  auto* C=NewObject<UStaticMeshComponent>(W);C->SetStaticMesh(Mesh);C->SetupAttachment(N->Comp.Get());
  C->SetMobility(EComponentMobility::Movable);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(true);C->RegisterComponent();
  double E[16];S47Three::Compose(At,S47Three::Euler(0,H,0),FVector::OneVector,E);C->SetRelativeTransform(S47Three::MatToUE(E));
  const int32 I=W->AdoptNode(C,Parent,Name);if(I<0){C->DestroyComponent();return nullptr;}return C;
 }
 bool Within(AS47Web* W,int32 I,int32 Root)
 {
  for(;I>=0;){if(I==Root)return true;const FS47WebNode* N=W->Node(I);if(!N)return false;I=N->Parent;}return false;
 }

 UStaticMesh* ForecourtTrim(AS47Web* W,UStaticMesh* Original)
 {
  if(!Original||!Original->bAllowCPUAccess||!Original->GetRenderData()||Original->GetRenderData()->LODResources.Num()!=3)return nullptr;
  const FStaticMeshLODResources& Src=Original->GetRenderData()->LODResources[0];
  const FStaticMeshVertexBuffers& B=Src.VertexBuffers;
  if(Src.GetNumTriangles()!=10702||!B.PositionVertexBuffer.GetVertexData()||
     !B.StaticMeshVertexBuffer.GetTangentData()||!B.StaticMeshVertexBuffer.GetTexCoordData()||Src.IndexBuffer.GetIndexDataSize()==0)return nullptr;
  auto Plant=[&](uint32 I)
  {
   const FVector P=S47Three::ToThree(FVector(B.PositionVertexBuffer.VertexPosition(I)));
   const FVector2f UV=B.StaticMeshVertexBuffer.GetVertexUV(I,0);
   // Exact brushDry/grassDry constant palette swatches from weathering.ts.
   // Accept the source V convention or the FBX V flip; exact230 guard below.
   // The measured front clumps overlap the paving edge by up to 15cm: remove whole
   // authored triangles in this narrow seam only, preserving all plinth/roof/props.
   return P.X>=19.8&&P.X<=40&&P.Y>=-1&&P.Y<=.1&&P.Z>=5.6&&P.Z<=6.6&&
    (FMath::Abs(UV.Y-.1875f)<.0001f||FMath::Abs(UV.Y-.8125f)<.0001f)&&(FMath::Abs(UV.X-.5625f)<.0001f||FMath::Abs(UV.X-.6875f)<.0001f);
  };
  auto CarRemnant=[&](uint32 I)
  {
   const FVector P=S47Three::ToThree(FVector(B.PositionVertexBuffer.VertexPosition(I)));
   const FVector2f UV=B.StaticMeshVertexBuffer.GetVertexUV(I,0);
   const bool InCar=P.Y<-.1&&((P.X>=25.8&&P.X<=28.2&&P.Z>=6.6&&P.Z<=12.6)||
    (P.X>=29.8&&P.X<=32.2&&P.Z>=6.55&&P.Z<=13.25));
   // Original y>-.57 car trim missed bottom-crossing wheel/well triangles.
   // Only the two exact dark swatches; retain the concrete wheel stops.
   const bool Rubber=FMath::Abs(UV.X-.9375f)<.0001f&&(FMath::Abs(UV.Y-.9375f)<.0001f||FMath::Abs(UV.Y-.0625f)<.0001f);
   const bool Well=FMath::Abs(UV.X-.3125f)<.0001f&&(FMath::Abs(UV.Y-.1875f)<.0001f||FMath::Abs(UV.Y-.8125f)<.0001f);
   return InCar&&(Rubber||Well);
  };
  TArray<uint32> Old,Keep;Src.IndexBuffer.GetCopy(Old);Keep.Reserve(Old.Num());
  TArray<FStaticMeshSection> Sections;
  int32 Removed=0,RemovedPlants=0,RemovedCars=0;
  for(const FStaticMeshSection& Section:Src.Sections)
  {
   FStaticMeshSection Copy=Section;Copy.FirstIndex=Keep.Num();Copy.NumTriangles=0;
   for(uint32 I=Section.FirstIndex;I<Section.FirstIndex+Section.NumTriangles*3;I+=3)
   {
    if(I+2>=uint32(Old.Num()))return nullptr;
    const uint32 A=Old[I],Bidx=Old[I+1],Cidx=Old[I+2];
    if(FMath::Max3(A,Bidx,Cidx)>=B.PositionVertexBuffer.GetNumVertices())return nullptr;
    if(Plant(A)&&Plant(Bidx)&&Plant(Cidx)) {++Removed;++RemovedPlants;continue;}
    if(CarRemnant(A)&&CarRemnant(Bidx)&&CarRemnant(Cidx)) {++Removed;++RemovedCars;continue;}
    Keep.Append({A,Bidx,Cidx});++Copy.NumTriangles;
   }
   if(Copy.NumTriangles)Sections.Add(Copy);
  }
  if(RemovedPlants!=230||RemovedCars!=144||Removed!=374||Keep.Num()!=10328*3||Keep.Num()+Removed*3!=Old.Num())return nullptr;
  // Preserve every other dressing triangle, normal, UV and slot; imported asset is immutable.
  // The imported asset itself is immutable and remains available for atomic fallback.
  UStaticMesh* Cut=NewObject<UStaticMesh>(W,TEXT("SM_SaroPark_ForecourtTrim"),RF_Transient);
  Cut->bAllowCPUAccess=true;Cut->NeverStream=true;Cut->GetStaticMaterials()=Original->GetStaticMaterials();
  // This dressing clone keeps native ray tracing with correctly initialised render geometry.
  Cut->bSupportRayTracing=true;
  auto Data=MakeUnique<FStaticMeshRenderData>();Data->AllocateLODResources(1);
  Data->Bounds=Original->GetRenderData()->Bounds;Data->ScreenSize[0].Default=1;
  Data->NumInlinedLODs=1;Data->CurrentFirstLODIdx=0;Data->bReadyForStreaming=false;
  FStaticMeshLODResources& Dst=Data->LODResources[0];Dst.Sections.Append(Sections);
  Dst.VertexBuffers.PositionVertexBuffer.Init(B.PositionVertexBuffer,true);
  auto& Tangents=Dst.VertexBuffers.StaticMeshVertexBuffer;
  Tangents.SetUseHighPrecisionTangentBasis(B.StaticMeshVertexBuffer.GetUseHighPrecisionTangentBasis());
  Tangents.SetUseFullPrecisionUVs(B.StaticMeshVertexBuffer.GetUseFullPrecisionUVs());
  Tangents.Init(B.StaticMeshVertexBuffer.GetNumVertices(),B.StaticMeshVertexBuffer.GetNumTexCoords(),true);
  for(uint32 I=0;I<B.StaticMeshVertexBuffer.GetNumVertices();++I)
  {
   Tangents.SetVertexTangents(I,FVector3f(B.StaticMeshVertexBuffer.VertexTangentX(I)),B.StaticMeshVertexBuffer.VertexTangentY(I),FVector3f(B.StaticMeshVertexBuffer.VertexTangentZ(I)));
   for(uint32 U=0;U<B.StaticMeshVertexBuffer.GetNumTexCoords();++U)Tangents.SetVertexUV(I,U,B.StaticMeshVertexBuffer.GetVertexUV(I,U));
  }
  const uint32 Colors=B.ColorVertexBuffer.GetNumVertices();
  if(Colors)
  {
   if(!B.ColorVertexBuffer.GetVertexData())return nullptr;
   Dst.VertexBuffers.ColorVertexBuffer.Init(Colors,true);
   for(uint32 I=0;I<Colors;++I)Dst.VertexBuffers.ColorVertexBuffer.VertexColor(I)=B.ColorVertexBuffer.VertexColor(I);
  }
  Dst.bHasColorVertexData=Colors>0;Dst.bBuffersInlined=true;Dst.bIsOptionalLOD=false;
  Dst.SourceMeshBounds=Data->Bounds;Dst.MaxDeviation=0;Dst.IndexBuffer.SetIndices(Keep,EIndexBufferStride::Force32Bit);
  // Same runtime RT-proxy construction policy as UStaticMesh::BuildFromMeshDescriptions.
  // If enabled, InitResources builds its geometry from these indices without renderer overrides.
  bool bInitRayTracing=Cut->bSupportRayTracing;
#if !WITH_EDITOR
  bInitRayTracing=bInitRayTracing&&IsRayTracingAllowed();
#endif
  if(bInitRayTracing)Data->InitializeRayTracingRepresentationFromRenderingLODs();
  Cut->SetRenderData(MoveTemp(Data));Cut->SetExtendedBounds(Original->GetBounds());
  Cut->InitResources();return Cut;
 }
}
TUniquePtr<FS47Fleet> FS47Fleet::Install(AS47Web* W,FS47Truck& T)
{
 if(!W||!T.Group||!T.Body||!T.Shell||!T.Cab)return {};
 if(auto Jeep=FS47TripoFleet::InstallJeep(W,T))return Jeep;
 const TCHAR* Names[]={TEXT("SM_Saro07_Body"),TEXT("SM_Saro07_Shell"),TEXT("SM_Saro07_Glass"),TEXT("SM_Saro07_Cabin"),TEXT("SM_Saro07_Branding"),TEXT("SM_Saro07_HeadLens"),TEXT("SM_Saro07_TailLens"),TEXT("SM_Saro07_WheelFL"),TEXT("SM_Saro07_WheelFR"),TEXT("SM_Saro07_WheelRL"),TEXT("SM_Saro07_WheelRR")};
 TArray<UStaticMesh*> Meshes;for(const TCHAR* Name:Names){auto* A=Asset(Name);if(!A){UE_LOG(LogS47Fleet,Warning,TEXT("S47: fleet unavailable %s; complete legacy truck retained"),Name);return {};}Meshes.Add(A);}
 // Preserve the original live cluster/radio faces and all moving instrument assemblies.
 TSet<int32> Keep;
 for(const TCHAR* Key:{TEXT("needles"),TEXT("keys"),TEXT("wheelSpin"),TEXT("compass.card")})
 {
  const FS47Obj O=W->Obj(T.Path+TEXT(".")+Key);if(!O){UE_LOG(LogS47Fleet,Error,TEXT("S47: fleet missing live anchor %s"),Key);return {};}
  const int32 I=FString(Key)==TEXT("needles")||FString(Key)==TEXT("compass.card")?O.Parent().I:O.I;Keep.Add(I);
 }
 const int32 Gauge=W->Material(T.Path+TEXT(".m.gauge")).Id;
 T.Cab.Traverse([&](FS47Obj O){const auto* N=W->Node(O.I);if(N&&N->Mats.Contains(Gauge))Keep.Add(O.I);});
 // Collect original meshes BEFORE adopting replacement nodes so the new art is never hidden.
 TArray<int32> Old;
 for(FS47Obj R:{T.Body,T.Shell,T.Cab,W->Obj(T.Path+TEXT(".wheels"))})R.Traverse([&](FS47Obj O)
 {
  const auto* N=W->Node(O.I);if(!N||!N->Comp.IsValid()||!N->Comp->IsA<UStaticMeshComponent>())return;
  bool Retain=false;for(int32 K:Keep)Retain|=Within(W,O.I,K);if(!Retain)Old.AddUnique(O.I);
 });
 auto F=MakeUnique<FS47Fleet>();F->Web=W;F->RetiredNodes=Old;TArray<UStaticMeshComponent*> Made;
 auto Add=[&](int32 Mesh,int32 Parent,const FString& S,const FVector& P=FVector::ZeroVector)
 {auto* C=Part(W,Meshes[Mesh],Parent,TEXT("fleet:")+T.Path+TEXT(":")+S,P);if(C)Made.Add(C);return C;};
 Add(0,T.Body.I,TEXT("body"));Add(1,T.Shell.I,TEXT("shell"));Add(2,T.Shell.I,TEXT("glass"));
 Add(3,T.Shell.I,TEXT("parked-cabin"));Add(2,T.Cab.I,TEXT("driver-glass"));Add(3,T.Cab.I,TEXT("driver-cabin"));Add(4,T.Body.I,TEXT("markings"));
 auto* Head=Add(5,T.Body.I,TEXT("head-lenses"));auto* Tail=Add(6,T.Body.I,TEXT("tail-lenses"));
 // Contract centres match the inspected C10 axle positions; original suspension input is retained.
 const FVector At[]={FVector(-.8003,.4198,-1.3427),FVector(.8143,.4237,-1.3394),FVector(-.8058,.3730,1.5063),FVector(.8087,.3767,1.5097)};
 for(int32 I=0;I<4;++I)
 {
  // Source C10 front tyres were 45mm above the source road. Put the real tyre bottom
  // on the controller ground; each authored radius also sets its own rolling speed.
  const double Radius=Meshes[7+I]->GetBounds().BoxExtent.Z/100.;FVector Centre=At[I];Centre.Y=Radius;
  auto* C=Add(7+I,T.Body.I,FString::Printf(TEXT("wheel%d"),I),Centre);F->WheelParts.Add(C);F->WheelAt.Add(Centre);F->WheelRadius.Add(Radius);F->ExpectedWheels.Add(C?C->GetRelativeTransform():FTransform::Identity);
 }
 if(Made.Num()!=13){for(auto* C:Made)C->DestroyComponent();return {};}
 for(auto Pair:{TPair<UStaticMeshComponent*,TArray<TWeakObjectPtr<UMaterialInstanceDynamic>>*>(Head,&F->HeadMids),TPair<UStaticMeshComponent*,TArray<TWeakObjectPtr<UMaterialInstanceDynamic>>*>(Tail,&F->TailMids)})
  for(int32 I=0;I<Pair.Key->GetNumMaterials();++I)if(auto* Mid=Pair.Key->CreateDynamicMaterialInstance(I))Pair.Value->Add(Mid);
 for(int32 I:Old)W->ObjAt(I).SetVisible(false);
 for(auto* C:Made)F->NativeParts.Add(C);
 F->bReady=true;UE_LOG(LogS47Fleet,Display,TEXT("S47: production SARO07 ready %s; C10+original fitted cab; %d legacy meshes replaced, live instrument anchors retained"),*T.Path,Old.Num());return F;
}
void FS47Fleet::Wheels(double Travel,double Steer,const double Drops[4])
{
 for(int32 I=0;I<WheelParts.Num();++I)if(auto* C=WheelParts[I].Get())
 {
  double E[16];const FVector P=WheelAt[I]+FVector(0,Drops[I],0);
  S47Three::Compose(P,S47Three::Euler(Travel/FMath::Max(WheelRadius[I],.05),I<2?-Steer:0,0,TEXT("YXZ")),FVector::OneVector,E);ExpectedWheels[I]=S47Three::MatToUE(E);C->SetRelativeTransform(ExpectedWheels[I]);
 }
}
void FS47Fleet::Lenses(const FLinearColor& Color,double Head,double Tail)
{
 ExpectedHead=Color*float(Head*1.6);ExpectedTail=FLinearColor(1,.025,.01)*float(Tail);
 if(bTripoJeep)
 {
  for(auto& P:HeadMids)if(auto* M=P.Get()){M->SetVectorParameterValue(TEXT("S47Headlamp"),ExpectedHead);M->SetVectorParameterValue(TEXT("S47Taillamp"),ExpectedTail);}return;
 }
 for(auto& P:HeadMids)if(auto* M=P.Get())M->SetVectorParameterValue(TEXT("EmissiveFactor"),ExpectedHead);
 for(auto& P:TailMids)if(auto* M=P.Get())M->SetVectorParameterValue(TEXT("EmissiveFactor"),ExpectedTail);
}
TSharedPtr<FS47FleetParked> FS47FleetParked::Build(AS47Web* W,bool bSiteReady)
{
 if(!W||!bSiteReady)return {};auto* Sedan=Asset(TEXT("SM_SaroPark_Sedan"));auto* Pickup=Asset(TEXT("SM_SaroPark_Pickup"));auto* Trim=Asset(TEXT("SM_SaroPark_LegacyTrim"));
 const int32 Parent=W->Handle(TEXT("world.grounds.group"));UStaticMeshComponent* Old=nullptr;
 for(int32 I=0;I<W->PartCount();++I){const auto* N=W->Node(I);if(N&&N->Geo==454&&Within(W,I,Parent))Old=Cast<UStaticMeshComponent>(N->Comp.Get());}
 if(!Sedan||!Pickup||!Trim||!Old||Parent<0){UE_LOG(LogS47Fleet,Warning,TEXT("S47: parked fleet unavailable; complete legacy staff corner retained"));return {};}
 // Move the red sedan to the east staff bay, keeping every west visitor bay free.
 // Only its exact original walking footprint moves with it; no stale invisible car remains.
 int32 SedanFeet=0;
 for(const auto& R:W->CollidersMutable(TEXT("saro")))
  if(FMath::IsNearlyEqual(R.MinX,26.,.0001)&&FMath::IsNearlyEqual(R.MaxX,28.,.0001)&&FMath::IsNearlyEqual(R.MinZ,6.85,.0001)&&FMath::IsNearlyEqual(R.MaxZ,12.35,.0001))++SedanFeet;
 if(SedanFeet!=1){UE_LOG(LogS47Fleet,Warning,TEXT("S47: staff fleet unchanged: exact sedan foot unavailable"));return {};}
 UStaticMesh* PavedTrim=ForecourtTrim(W,Trim);
 if(!PavedTrim){UE_LOG(LogS47Fleet,Error,TEXT("S47: forecourt trim requires exact10702 source with230 brush+144 wheel remnants and CPU access"));return {};}
 auto F=MakeShared<FS47FleetParked>();F->Web=W;auto* A=Part(W,Sedan,Parent,TEXT("fleet:staff-sedan"),FVector(S47SaroForecourt::SedanX,-.576,9.6));auto* B=Part(W,Pickup,Parent,TEXT("fleet:staff-pickup"),FVector(31,-.576,9.9),.04);
 if(!A||!B){if(A)A->DestroyComponent();if(B)B->DestroyComponent();return {};}
 for(auto& R:W->CollidersMutable(TEXT("saro")))
  if(FMath::IsNearlyEqual(R.MinX,26.,.0001)&&FMath::IsNearlyEqual(R.MaxX,28.,.0001)&&FMath::IsNearlyEqual(R.MinZ,6.85,.0001)&&FMath::IsNearlyEqual(R.MaxZ,12.35,.0001))
  {R.MinX+=S47SaroForecourt::SedanX-27.;R.MaxX+=S47SaroForecourt::SedanX-27.;}
 // This composed trim removes only the two old vehicle bodies and the sixteen replaced wing windows.
 // Reuse the exact original atlas; unrelated footprint colliders stay byte-for-byte in place.
 auto* OriginalMaterial=Old->GetMaterial(0);Old->SetStaticMesh(PavedTrim);Old->SetMaterial(0,OriginalMaterial);
 F->Components={A,B};F->TrimmedDressing=Old;F->DressingMaterial=OriginalMaterial;F->bReady=true;UE_LOG(LogS47Fleet,Display,TEXT("S47: staff fleet ready: two period vehicles on paved staff bays, sedan east35 and matching foot moved, visitors free"));return F;
}

bool FS47Fleet::Audit(TArray<FString>& Lines)const
{
 bool Ok=true;auto Check=[&](bool B,const FString& N){Ok&=B;Lines.Add(FString(B?TEXT("PASS fleet: "):TEXT("FAIL fleet: "))+N);};
 if(bTripoJeep)
 {
  Check(bReady&&NativeParts.Num()==1,TEXT("original textured Tripo Jeep bound atomically"));
  for(auto& Part:NativeParts){auto* C=Part.Get();Check(C&&C->GetStaticMesh()&&C->GetStaticMesh()->GetNumLODs()==3&&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("three source LODs; transport collision remains explicit"));}
  Check(WheelParts.IsEmpty(),TEXT("no manufactured wheel parts for a source with fused wheels"));
  Check(!HeadMids.IsEmpty(),TEXT("source lens mask has switched head/tail material parameters"));
  for(auto& P:HeadMids)Check(P.IsValid()&&P->K2_GetVectorParameterValue(TEXT("S47Headlamp")).Equals(ExpectedHead,.0001f)&&P->K2_GetVectorParameterValue(TEXT("S47Taillamp")).Equals(ExpectedTail,.0001f),TEXT("actual inspected source lenses follow both switches"));
  return Ok;
 }
 Check(bReady&&NativeParts.Num()==13,TEXT("complete fitted C10 body/shell/glass/cabin/lenses/four wheels"));
 for(int32 I=0;I<NativeParts.Num();++I)
 {
  auto* C=NativeParts[I].Get();bool Materials=C&&C->GetStaticMesh();if(C)for(int32 K=0;K<C->GetNumMaterials();++K)Materials&=C->GetMaterial(K)!=nullptr;
  Check(Materials&&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision,FString::Printf(TEXT("part %d actual mesh/materials; existing gameplay collision retained"),I));
 }
 if(auto* W=Web.Get())for(int32 I:RetiredNodes)Check(!W->ObjAt(I).Visible(),FString::Printf(TEXT("legacy body/cab node%d retired"),I));else Check(false,TEXT("live Web reference"));
 Check(WheelParts.Num()==4&&WheelRadius.Num()==4,TEXT("four authored wheels"));
 for(int32 I=0;I<WheelParts.Num();++I)
 {
  auto* C=WheelParts[I].Get();Check(C&&C->GetRelativeTransform().Equals(ExpectedWheels[I],.001),FString::Printf(TEXT("wheel%d actual rolling/steering/suspension pose"),I));
  Check(WheelRadius[I]>.30&&WheelRadius[I]<.50&&FMath::IsNearlyEqual(WheelAt[I].Y,WheelRadius[I],.0001),FString::Printf(TEXT("wheel%d real tyre radius and grounded neutral position"),I));
 }
 Check(!HeadMids.IsEmpty()&&!TailMids.IsEmpty(),TEXT("physical lens material bindings"));
 for(auto& P:HeadMids)Check(P.IsValid()&&P->K2_GetVectorParameterValue(TEXT("EmissiveFactor")).Equals(ExpectedHead,.0001f),TEXT("actual sealed-beam lens follows headlight state"));
 for(auto& P:TailMids)Check(P.IsValid()&&P->K2_GetVectorParameterValue(TEXT("EmissiveFactor")).Equals(ExpectedTail,.0001f),TEXT("actual tail lens follows lamp/brake state"));
 return Ok;
}
bool FS47FleetParked::Audit(TArray<FString>& Lines)const
{
 bool Ok=true;auto Check=[&](bool B,const FString& N){Ok&=B;Lines.Add(FString(B?TEXT("PASS staff fleet: "):TEXT("FAIL staff fleet: "))+N);};
 Check(bReady&&Components.Num()==2,TEXT("two production vehicles"));
 const FVector At[]={FVector(S47SaroForecourt::SedanX,-.576,9.6),FVector(31,-.576,9.9)};
 const TCHAR* Names[]={TEXT("SM_SaroPark_Sedan"),TEXT("SM_SaroPark_Pickup")};
 for(int32 I=0;I<Components.Num();++I)
 {
  auto* C=Components[I].Get();double E[16];S47Three::Compose(At[I],S47Three::Euler(0,I==0?0:.04,0),FVector::OneVector,E);
  Check(C&&C->GetStaticMesh()&&C->GetStaticMesh()->GetName()==Names[I]&&C->GetRelativeTransform().Equals(S47Three::MatToUE(E),.001),FString::Printf(TEXT("car%d correct imported body, staff bay and paved grounded height"),I));
 }
 int32 NewFeet=0,OldFeet=0;
 if(auto* W=Web.Get())for(const auto& R:W->CollidersMutable(TEXT("saro")))
 {
  if(FMath::IsNearlyEqual(R.MinX,34.,.0001)&&FMath::IsNearlyEqual(R.MaxX,36.,.0001)&&FMath::IsNearlyEqual(R.MinZ,6.85,.0001)&&FMath::IsNearlyEqual(R.MaxZ,12.35,.0001))++NewFeet;
  if(FMath::IsNearlyEqual(R.MinX,26.,.0001)&&FMath::IsNearlyEqual(R.MaxX,28.,.0001)&&FMath::IsNearlyEqual(R.MinZ,6.85,.0001)&&FMath::IsNearlyEqual(R.MaxZ,12.35,.0001))++OldFeet;
 }
 Check(NewFeet==1&&OldFeet==0,TEXT("sedan actual foot follows east staff bay, old position clear; all visitor bays free"));
 auto* D=TrimmedDressing.Get();Check(D&&D->GetStaticMesh()&&D->GetStaticMesh()->GetName()==TEXT("SM_SaroPark_ForecourtTrim")&&D->GetStaticMesh()->GetNumTriangles(0)==10328&&D->GetMaterial(0)==DressingMaterial.Get(),TEXT("exact old-car/window trim plus230 front brush+144 old wheel triangles removed,10328 unrelated triangles retain original dressing atlas"));return Ok;
}

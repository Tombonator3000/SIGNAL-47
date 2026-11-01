// One detailed roadside line, shared in the canonical road frame. Unreal only.
#include "S47HighwayPower.h"
#include "S47Web.h"
#include "S47Geo.h"
#include "S47RoadShape.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "StaticMeshResources.h"
#include "RenderUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47HighwayPower, Log, All);
namespace
{
 TMap<TWeakObjectPtr<const AS47Web>, S47HighwayPower::FStatus> Results;
 const FVector RoadOrigin(8000,0,0);
 constexpr double PoleSink=.4, Top=9.75, Foot=.24;
 const TCHAR* Pole1=TEXT("/Game/Free/Roadside/telephone_poles/StaticMeshes/telephone_poles_Small_Pole1.telephone_poles_Small_Pole1");
 const TCHAR* Pole2=TEXT("/Game/Free/Roadside/telephone_poles/StaticMeshes/telephone_poles_Small_Pole2.telephone_poles_Small_Pole2");
 struct FWirePatch { UStaticMeshComponent* Comp=nullptr; UStaticMesh* Mesh=nullptr; int32 Removed=0, Kept=0; };

 bool Overhead(const FVector3f& UE)
 {
  // g684/g918 vertices are both in road-local metres before their different parents.
  const FVector P=S47Three::ToThree(FVector(UE));
  return P.Y-S47RoadTerrain::SurfaceY(P.X,P.Z)>6.0;
 }
 bool MakeWirePatch(AS47Web* W,UStaticMeshComponent* C,int32 Expected,FWirePatch& Out)
 {
  UStaticMesh* Original=C?C->GetStaticMesh():nullptr;
  if(!Original||!Original->bAllowCPUAccess||!Original->GetRenderData()||Original->GetRenderData()->LODResources.Num()!=1)return false;
  const FStaticMeshLODResources& Src=Original->GetRenderData()->LODResources[0];
  const FStaticMeshVertexBuffers& B=Src.VertexBuffers;
  if(Src.GetNumTriangles()!=Expected||!B.PositionVertexBuffer.GetVertexData()||
     !B.StaticMeshVertexBuffer.GetTangentData()||!B.StaticMeshVertexBuffer.GetTexCoordData()||Src.IndexBuffer.GetIndexDataSize()==0)return false;
  TArray<uint32> Old,Keep;Src.IndexBuffer.GetCopy(Old);Keep.Reserve(Old.Num());
  TArray<FStaticMeshSection> Sections;
  int32 Removed=0;
  for(const FStaticMeshSection& Section:Src.Sections)
  {
   FStaticMeshSection Copy=Section;Copy.FirstIndex=Keep.Num();Copy.NumTriangles=0;
   for(uint32 I=Section.FirstIndex;I<Section.FirstIndex+Section.NumTriangles*3;I+=3)
   {
    if(I+2>=uint32(Old.Num()))return false;
    const uint32 A=Old[I],Bidx=Old[I+1],Cidx=Old[I+2];
    if(FMath::Max3(A,Bidx,Cidx)>=B.PositionVertexBuffer.GetNumVertices())return false;
    if(Overhead(B.PositionVertexBuffer.VertexPosition(A))&&Overhead(B.PositionVertexBuffer.VertexPosition(Bidx))&&Overhead(B.PositionVertexBuffer.VertexPosition(Cidx))) {++Removed;continue;}
    Keep.Append({A,Bidx,Cidx});++Copy.NumTriangles;
   }
   if(Copy.NumTriangles)Sections.Add(Copy);
  }
  if(Removed==0||Keep.Num()==0||Keep.Num()+Removed*3!=Old.Num())return false;
  // Copy every original vertex, tangent, UV, color and slot. Only overhead indices differ.
  // The imported asset itself is immutable and remains available for atomic fallback.
  UStaticMesh* Cut=NewObject<UStaticMesh>(W,NAME_None,RF_Transient);
  Cut->bAllowCPUAccess=true;Cut->NeverStream=true;Cut->GetStaticMaterials()=Original->GetStaticMaterials();
  // Only these two 6mm non-shadow fence-wire clones omit duplicate always-resident RT BLAS.
  // Their raster geometry stays exact; all new power poles/cables retain their original RT policy.
  Cut->bSupportRayTracing=false;
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
   if(!B.ColorVertexBuffer.GetVertexData())return false;
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
  Out={C,Cut,Removed,Keep.Num()/3};return true;
 }

 FVector At(double X,double Z) {return FVector(X,S47RoadTerrain::SurfaceY(X,Z),Z);}
 bool OldFoot(const FS47Rect& R,const TArray<FVector>& Feet,const FVector& Offset)
 {
  if(R.Id.StartsWith(TEXT("highway:power")))return true;
  const double X=(R.MinX+R.MaxX)*.5-Offset.X,Z=(R.MinZ+R.MaxZ)*.5-Offset.Z;
  const double Sx=R.MaxX-R.MinX,Sz=R.MaxZ-R.MinZ;
  if(Sx<.35||Sx>.65||Sz<.35||Sz>.65)return false;
  for(const FVector& P:Feet)if(FMath::Abs(P.X-X)<.025&&FMath::Abs(P.Z-Z)<.025)return true;
  return false;
 }
}
S47HighwayPower::FStatus S47HighwayPower::Status(const AS47Web* W) {return Results.FindRef(W);}

void AS47Web::BuildHighwayPower()
{
 if(FParse::Param(FCommandLine::Get(),TEXT("S47HighwayPowerOff"))||FParse::Param(FCommandLine::Get(),TEXT("S47PropsOff")))return;
 if(S47HighwayPower::Status(this).bReady)return;
 S47HighwayPower::FStatus State;
 const int32 Parent=Handle(TEXT("world.legs.road.area.group"));
 UStaticMesh* MainMesh=LoadObject<UStaticMesh>(nullptr,Pole1);
 UStaticMesh* SpurMesh=LoadObject<UStaticMesh>(nullptr,Pole2);
 UStaticMesh* Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 UMaterialInterface* WireBase=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 if(!Nodes.IsValidIndex(Parent)||!MainMesh||!SpurMesh||!Cylinder||!WireBase)return;
 TArray<int32> Hide;
 TArray<FWirePatch> Patches;
 UInstancedStaticMeshComponent* OutsideWires=nullptr;TArray<int32> OutsidePower;
 int32 OldMainNodes=0;
 TArray<FVector> OldFeet;
 for(int32 I=0;I<Nodes.Num();++I)
 {
  const FS47WebNode& N=Nodes[I];
  if(N.Geo==666||N.Geo==877)
  {
   UInstancedStaticMeshComponent* C=Cast<UInstancedStaticMeshComponent>(N.Comp.Get());
   const int32 Expected=N.Geo==666?19:125;
   if(!C||C->GetInstanceCount()!=Expected||!C->GetStaticMesh()||C->GetStaticMesh()->GetNumTriangles(0)!=156)return;
   for(int32 K=0;K<C->GetInstanceCount();++K)
   {
    FTransform T;C->GetInstanceTransform(K,T,false);FVector P=S47Three::ToThree(T.GetLocation());
    // Source instance transforms are road-local for both copies, independent of parent.
    if(!OldFeet.ContainsByPredicate([&](const FVector& V){return FMath::Abs(V.X-P.X)<.01&&FMath::Abs(V.Z-P.Z)<.01;}))OldFeet.Add(P);
   }
   Hide.Add(I);++OldMainNodes;
  }
  if(N.Geo==684||N.Geo==918)
  {
   FWirePatch P;
   if(!MakeWirePatch(this,Cast<UStaticMeshComponent>(N.Comp.Get()),N.Geo==684?7776:46992,P))
   { UE_LOG(LogS47HighwayPower,Warning,TEXT("S47: highway power preserved legacy line: CPU-ready exact wire mesh g%d required"),N.Geo);return; }
   Patches.Add(P);
  }
  if(N.Name==TEXT("props:powerline")||N.Name==TEXT("props:spur"))Hide.Add(I);
  if(N.Name==TEXT("props:powerline_wires"))
  {
   // BuildRows shares ONE wire ISM across this file, including both ranch fences.
   OutsideWires=Cast<UInstancedStaticMeshComponent>(N.Comp.Get());if(!OutsideWires)return;
   for(int32 K=0;K<OutsideWires->GetInstanceCount();++K)
   {
    FTransform T;OutsideWires->GetInstanceTransform(K,T,true);
    const FVector Site=S47Three::ToThree(T.GetLocation());
    if(Site.Y-S47RoadTerrain::SurfaceY(Site.X+24,Site.Z-300)>6)OutsidePower.Add(K);
   }
   // The obsolete decorative fence rows may be absent. Accept the exact
   // power-only batch too; never retire arbitrary low wires as overhead cable.
   const int32 FenceSegments=OutsideWires->GetInstanceCount()-OutsidePower.Num();
   if(OutsidePower.Num()<500||OutsidePower.Num()>1000||(FenceSegments!=0&&FenceSegments<500))return;
  }
 }
 if(OldMainNodes!=2||Patches.Num()!=2)return;
 // New junctions replace the closest ordinary pole, rather than adding a crowded second row.
 TArray<double> Zs;
 // Wider spans reduce both poles and the four twelve-segment cable runs.
 // Keep the installed SARO source position and all authored service junctions.
 const double SaroSourceZ=-280+(S47RoadShape::Hash(-280,1)-.5)*4;
 for(int32 Z=-1900;Z<=3400;Z+=65)Zs.Add(Z+(S47RoadShape::Hash(Z,1)-.5)*4);
 for(double Z:{SaroSourceZ,190.0,380.0,622.0})
 {
  int32 Closest=0;for(int32 K=1;K<Zs.Num();++K)if(FMath::Abs(Zs[K]-Z)<FMath::Abs(Zs[Closest]-Z))Closest=K;
  Zs[Closest]=Z;
 }
 Zs.Sort();
 TArray<FVector> Main;for(double Z:Zs)Main.Add(At(15,Z));
 int32 SaroSource=0;
 for(int32 K=1;K<Main.Num();++K)if(FMath::Abs(Main[K].Z+281.4)<FMath::Abs(Main[SaroSource].Z+281.4))SaroSource=K;
 // Terminate the highway line before the antenna field. The previous north extension
 // crossed actual S02/S11 reflector triangles; the field uses its existing buried cable.
 Main.RemoveAt(0,SaroSource);SaroSource=0;
 // This one natural-spaced highway stem stands on SARO's authored planar visitor ground.
 Main[SaroSource].Y=-.6;
 TArray<TArray<FVector>> Spurs;
 Spurs.Add({Main[SaroSource],FVector(27.6,-.6,-281.4)}); // one necessary SARO transformer pole
 Spurs.Add({At(15,190),At(75,190.72),At(135,191.44),At(195,192.16),At(255,192.88),At(315,193.60),At(375,194.32),At(435,195.04)});
 Spurs.Add({At(15,380),At(-13,380)}); // Motor Court service drop, clear of the highway
 const bool bSubstation=Nodes.ContainsByPredicate([](const FS47WebNode& N){return N.Name==TEXT("prop:substationSitePole_C");});
 if(bSubstation)Spurs.Add({At(15,622),At(-25,622),At(-65,622)});

 // Prepare every component before changing any old visibility or movement data.
 TArray<UHierarchicalInstancedStaticMeshComponent*> Created;
 auto Batch=[&](UStaticMesh* Mesh,const TCHAR* Name,bool bWire)->UHierarchicalInstancedStaticMeshComponent*
 {
  auto* C=NewObject<UHierarchicalInstancedStaticMeshComponent>(this,Name);
  C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);C->SetCastShadow(!bWire);
  C->SetCollisionEnabled(bWire?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryOnly);
  C->SetCollisionResponseToAllChannels(ECR_Ignore);if(!bWire)C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
  C->SetCullDistances(140000,170000);C->bAutoRebuildTreeOnInstanceChanges=false;
  C->RegisterComponent();C->AttachToComponent(Nodes[Parent].Comp.Get(),FAttachmentTransformRules::KeepWorldTransform);
  C->SetWorldTransform(FTransform(S47Three::ToUE(RoadOrigin)));Created.Add(C);return C;
 };
 auto* Poles=Batch(MainMesh,TEXT("HighwayPowerMain"),false);
 auto* Branch=Batch(SpurMesh,TEXT("HighwayPowerSpurs"),false);
 auto* Wires=Batch(Cylinder,TEXT("HighwayPowerWires"),true);
 UMaterialInstanceDynamic* WireMat=UMaterialInstanceDynamic::Create(WireBase,this);
 WireMat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.016f,.015f,.013f));Wires->SetMaterial(0,WireMat);
 TArray<FVector> Feet;
 auto AddPole=[&](UHierarchicalInstancedStaticMeshComponent* C,const FVector& P,double Yaw)
 {
  C->AddInstance(FTransform(FRotator(0,Yaw,0),S47Three::ToUE(P-FVector(0,PoleSink,0)),FVector::OneVector),false);Feet.Add(P);
 };
 for(const FVector& P:Main)AddPole(Poles,P,0);
 for(const TArray<FVector>& S:Spurs)for(int32 K=1;K<S.Num();++K)AddPole(Branch,S[K],90);
 auto Span=[&](const FVector& A,const FVector& B,double Sag)
 {
  FVector Prev=S47Three::ToUE(A);
  for(int32 K=1;K<=12;++K)
  {
   const double T=K/12.0;FVector P=FMath::Lerp(A,B,T);P.Y-=4*Sag*T*(1-T);
   const FVector Next=S47Three::ToUE(P),D=Next-Prev;
   Wires->AddInstance(FTransform(FRotationMatrix::MakeFromZ(D.GetSafeNormal()).ToQuat(),(Next+Prev)*.5,FVector(.022,.022,D.Size()/100.0)),false);
   Prev=Next;++State.WireSegments;
  }
 };
 for(int32 K=0;K+1<Main.Num();++K)
  for(double X:{-1.95,-.95,.95,1.95})Span(Main[K]+FVector(X,Top,0),Main[K+1]+FVector(X,Top,0),.85);
 for(int32 Si=0;Si<Spurs.Num();++Si)
 {
  const TArray<FVector>& S=Spurs[Si];const bool Sub=bSubstation&&Si==Spurs.Num()-1;
  const TArray<double> Ports=Sub?TArray<double>{-1.95,0,1.95}:TArray<double>{-1.95,1.95};
  for(int32 K=0;K+1<S.Num();++K)
   for(int32 Phase=0;Phase<Ports.Num();++Phase)
   {
    // A branch turns off the real main crossarm, not an unattached point beside it.
    const double MainPort=Sub&&Phase==1?.95:Ports[Phase];
    const FVector A=S[K]+(K==0?FVector(MainPort,Top,0):FVector(0,Top,Ports[Phase]));
    Span(A,S[K+1]+FVector(0,Top,Ports[Phase]),.7);
   }
 }
 // Two short primary jumpers and a three-wire service to the existing annex weatherhead.
 const FVector Transformer(27.6,-.6,-281.4);
 Span(Transformer+FVector(0,Top,-1.95),Transformer+FVector(-.53,8.20,.16),.12);
 Span(Transformer+FVector(0,Top,1.95),Transformer+FVector(-.19,8.20,-.16),.12);
 for(double D:{-.06,0.,.06})Span(Transformer+FVector(-.36,7.70,D*3),FVector(25.6,3.55,-289.14+D),.28);
 // Preserve the original authored transformer branch; terminate its three phases exactly at C.
 if(bSubstation)
  for(int32 Phase=0;Phase<3;++Phase)
  {
   const double EndX=(Phase-1)*1.04,StartZ=(Phase-1)*1.95;
   Span(At(-65,622)+FVector(0,Top,StartZ),FVector(-110-EndX,7.798202,622),.75);
  }
 // The existing motel roof point is authoritative (Roadside.cpp RoofPoint), a two-wire drop.
 const auto& M=S47Geo::MOTEL_AT();
 for(double Offset:{-.12,.12})Span(At(-13,380)+FVector(0,Top,Offset),FVector(-46.5+M.X+24,M.Y+3.01,31+M.Z-300+Offset),.35);
 for(auto* C:Created) { C->BuildTreeIfOutdated(false,true); }
 // All source checks succeeded; commit meshes, components and stem collisions together.
 for(const FWirePatch& P:Patches)
 {
  P.Mesh->InitResources();TArray<UMaterialInterface*> Materials=P.Comp->GetMaterials();
  P.Comp->SetStaticMesh(P.Mesh);P.Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  for(int32 I=0;I<Materials.Num();++I)P.Comp->SetMaterial(I,Materials[I]);
  State.RemovedOverheadTriangles+=P.Removed;State.PreservedWireTriangles+=P.Kept;
 }
 if(OutsideWires)
 {
  State.RemovedOutsidePowerSegments=OutsidePower.Num();
  State.PreservedOutsideFenceSegments=OutsideWires->GetInstanceCount()-OutsidePower.Num();
  OutsideWires->RemoveInstances(OutsidePower);
 }
 for(int32 I:Hide) {SetVisible(I,false);if(auto* C=Cast<UPrimitiveComponent>(Nodes[I].Comp.Get()))C->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
 AdoptNode(Poles,Parent,TEXT("highway:power:main"));AdoptNode(Branch,Parent,TEXT("highway:power:spurs"));AdoptNode(Wires,Parent,TEXT("highway:power:wires"));
 // Match exact old small stem footprints, not signs, fences, trees, wreck or whole row AABBs.
 for(const FString& Area:{FString(TEXT("saro")),FString(TEXT("road"))})
 {
  const FVector Offset=Area==TEXT("saro")?FVector(-24,0,300):RoadOrigin;
  TArray<FS47Rect>& Cs=CollidersMutable(Area);
  State.RemovedWalkBlocks+=Cs.RemoveAll([&](const FS47Rect& R){return OldFoot(R,OldFeet,Offset);});
  for(const FVector& P:Feet)
  {
   if(Area==TEXT("saro")&&(P.Z<-316||P.Z>485))continue;
   FS47Rect R;R.MinX=P.X+Offset.X-Foot;R.MaxX=P.X+Offset.X+Foot;R.MinZ=P.Z+Offset.Z-Foot;R.MaxZ=P.Z+Offset.Z+Foot;R.Id=TEXT("highway:power:stem");Cs.Add(R);
  }
  const TSharedPtr<FJsonObject>* Legs=nullptr,*Leg=nullptr;const TArray<TSharedPtr<FJsonValue>>* Obstacles=nullptr;
  if(!Drive.IsValid()||!Drive->TryGetObjectField(TEXT("legs"),Legs)||!(*Legs)->TryGetObjectField(Area,Leg)||!(*Leg)->TryGetArrayField(TEXT("obstacles"),Obstacles))continue;
  TArray<TSharedPtr<FJsonValue>> Kept;
  for(const auto& Value:*Obstacles)
  {
   const auto& A=Value->AsArray();bool Old=false;
   if(A.Num()==3&&FMath::Abs(A[2]->AsNumber()-.3)<.001)
    Old=OldFeet.ContainsByPredicate([&](const FVector& P){return FMath::Abs(P.X+Offset.X-A[0]->AsNumber())<.025&&FMath::Abs(P.Z+Offset.Z-A[1]->AsNumber())<.025;});
   if(A.Num()==4)
   {
    FS47Rect R;R.MinX=A[0]->AsNumber();R.MaxX=A[1]->AsNumber();R.MinZ=A[2]->AsNumber();R.MaxZ=A[3]->AsNumber();Old=OldFoot(R,OldFeet,Offset);
   }
   if(Old)++State.RemovedDriveBlocks;else Kept.Add(Value);
  }
  for(const FVector& P:Feet)
  {
   if(Area==TEXT("saro")&&(P.Z<-316||P.Z>485))continue;
   Kept.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(P.X+Offset.X),MakeShared<FJsonValueNumber>(P.Z+Offset.Z),MakeShared<FJsonValueNumber>(Foot)}));
  }
  (*Leg)->SetArrayField(TEXT("obstacles"),Kept);
 }
 State.bReady=true;State.MainPoles=Main.Num();State.SpurPoles=Feet.Num()-Main.Num();
 for(const FVector& P:Main)if(FMath::Square(P.X-24)+FMath::Square(P.Z+300)<=10000)++State.SaroMainWithin100m;
 State.SaroServicePoles=1;State.SaroTotalWithin100m=State.SaroMainWithin100m+1;Results.Add(this,State);
 UE_LOG(LogS47HighwayPower,Display,TEXT("S47: SARO utility layout mainWithin100m=%d service=%d totalWithin100m=%d naturalSourceSite=-9,%.6f sourceGround=-.600 serviceSite=3.6,18.6; legacy Grounds stems retired only by complete SaroSite kit"),State.SaroMainWithin100m,State.SaroServicePoles,State.SaroTotalWithin100m,Main[SaroSource].Z+300);
 UE_LOG(LogS47HighwayPower,Display,TEXT("S47: highway power ready main=%d spur=%d wireSegments=%d oldOverheadTriangles=%d preservedFenceTriangles=%d removedWalkStems=%d removedDriveStems=%d outsidePowerSegments=%d outsideFenceSegments=%d; canonical roadX15, wreck/fences/substation preserved"),State.MainPoles,State.SpurPoles,State.WireSegments,State.RemovedOverheadTriangles,State.PreservedWireTriangles,State.RemovedWalkBlocks,State.RemovedDriveBlocks,State.RemovedOutsidePowerSegments,State.PreservedOutsideFenceSegments);
}

// Original SARO building 2, recessed glazing and consolidated utility equipment.
#include "S47SaroSite.h"
#include "S47Web.h"
#include "S47Obj.h"
#include "S47Three.h"
#include "S47HighwayPower.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
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
 // Commit only after every source/native check. SetVisible on the wing would recurse into
 // preserved roof details, so retire only these exact draw components, leaving parent flags intact.
 for(int32 I:Retire)
 {
  auto* C=Cast<UStaticMeshComponent>(W->Node(I)->Comp.Get());C->SetVisibility(false,false);C->SetHiddenInGame(true,false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 }
 S->RetiredNodes=Retire;
 S->RemovedWalkFeet=W->CollidersMutable(TEXT("saro")).RemoveAll([](const FS47Rect& R){return OldFoot(R.MinX,R.MaxX,R.MinZ,R.MaxZ);});
 TArray<TSharedPtr<FJsonValue>> Kept;
 for(const auto& V:*Obs){const auto& A=V->AsArray();if(A.Num()==4&&OldFoot(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber(),A[3]->AsNumber()))++S->RemovedDriveFeet;else Kept.Add(V);}
 (*Leg)->SetArrayField(TEXT("obstacles"),Kept);S->Update();
 UE_LOG(LogS47SaroSite,Display,TEXT("S47: SARO site ready: original wing+title replaced, recessed glass .27m, solid sign wall; retired legacy poles=6 utilityGeos=4 walkFeet=%d driveFeet=%d; natural mainWithin100m=2 service=1 total=3; meter glass, field lights, fences, roof details and story proxies preserved"),S->RemovedWalkFeet,S->RemovedDriveFeet);
 return S;
}
void FS47SaroSite::Update()
{
 auto* W=Web.Get();if(!W)return;const bool Shown=W->Area()==TEXT("saro");
 for(int32 I:PartNodes)W->SetVisible(I,Shown);
 // Area changes can reapply the original hierarchy visibility; suppress only retired draws.
 for(int32 I:RetiredNodes)if(const auto* N=W->Node(I))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get())){C->SetVisibility(false,false);C->SetHiddenInGame(true,false);}
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
 const auto Power=S47HighwayPower::Status(W);OK&=Power.bReady&&Power.SaroMainWithin100m==2&&Power.SaroServicePoles==1&&Power.SaroTotalWithin100m==3;
 Lines.Add(FString::Printf(TEXT("SARO_SITE retired=6 legacyWalkFeet=%d legacyDriveFeet=%d utilityWithin100m=%d (main=%d service=%d), field lights/proxies unchanged; %s"),RemovedWalkFeet,RemovedDriveFeet,Power.SaroTotalWithin100m,Power.SaroMainWithin100m,Power.SaroServicePoles,OK?TEXT("PASS"):TEXT("FAIL")));return OK;
}

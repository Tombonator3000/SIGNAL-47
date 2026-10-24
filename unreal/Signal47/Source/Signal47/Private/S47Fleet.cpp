#include "S47Fleet.h"
#include "S47Truck.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
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

}
TUniquePtr<FS47Fleet> FS47Fleet::Install(AS47Web* W,FS47Truck& T)
{
 if(!W||!T.Group||!T.Body||!T.Shell||!T.Cab)return {};
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
 for(auto& P:HeadMids)if(auto* M=P.Get())M->SetVectorParameterValue(TEXT("EmissiveFactor"),ExpectedHead);
 for(auto& P:TailMids)if(auto* M=P.Get())M->SetVectorParameterValue(TEXT("EmissiveFactor"),ExpectedTail);
}
TSharedPtr<FS47FleetParked> FS47FleetParked::Build(AS47Web* W,bool bSiteReady)
{
 if(!W||!bSiteReady)return {};auto* Sedan=Asset(TEXT("SM_SaroPark_Sedan"));auto* Pickup=Asset(TEXT("SM_SaroPark_Pickup"));auto* Trim=Asset(TEXT("SM_SaroPark_LegacyTrim"));
 const int32 Parent=W->Handle(TEXT("world.grounds.group"));UStaticMeshComponent* Old=nullptr;
 for(int32 I=0;I<W->PartCount();++I){const auto* N=W->Node(I);if(N&&N->Geo==454&&Within(W,I,Parent))Old=Cast<UStaticMeshComponent>(N->Comp.Get());}
 if(!Sedan||!Pickup||!Trim||!Old||Parent<0){UE_LOG(LogS47Fleet,Warning,TEXT("S47: parked fleet unavailable; complete legacy staff corner retained"));return {};}
 auto F=MakeShared<FS47FleetParked>();auto* A=Part(W,Sedan,Parent,TEXT("fleet:staff-sedan"),FVector(27,-.60,9.6));auto* B=Part(W,Pickup,Parent,TEXT("fleet:staff-pickup"),FVector(31,-.60,9.9),.04);
 if(!A||!B){if(A)A->DestroyComponent();if(B)B->DestroyComponent();return {};}
 // This composed trim removes only the two old vehicle bodies and the sixteen replaced wing windows.
 // Reuse the exact original atlas material and retain all existing footprint colliders.
 auto* OriginalMaterial=Old->GetMaterial(0);Old->SetStaticMesh(Trim);Old->SetMaterial(0,OriginalMaterial);
 F->Components={A,B};F->TrimmedDressing=Old;F->DressingMaterial=OriginalMaterial;F->bReady=true;UE_LOG(LogS47Fleet,Display,TEXT("S47: staff fleet ready: two period vehicles, original poses/colliders and other dressing retained"));return F;
}

bool FS47Fleet::Audit(TArray<FString>& Lines)const
{
 bool Ok=true;auto Check=[&](bool B,const FString& N){Ok&=B;Lines.Add(FString(B?TEXT("PASS fleet: "):TEXT("FAIL fleet: "))+N);};
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
 const FVector At[]={FVector(27,-.60,9.6),FVector(31,-.60,9.9)};
 const TCHAR* Names[]={TEXT("SM_SaroPark_Sedan"),TEXT("SM_SaroPark_Pickup")};
 for(int32 I=0;I<Components.Num();++I)
 {
  auto* C=Components[I].Get();double E[16];S47Three::Compose(At[I],S47Three::Euler(0,I==0?0:.04,0),FVector::OneVector,E);
  Check(C&&C->GetStaticMesh()&&C->GetStaticMesh()->GetName()==Names[I]&&C->GetRelativeTransform().Equals(S47Three::MatToUE(E),.001),FString::Printf(TEXT("car%d correct imported body, original pose and grounded height"),I));
 }
 auto* D=TrimmedDressing.Get();Check(D&&D->GetStaticMesh()&&D->GetStaticMesh()->GetName()==TEXT("SM_SaroPark_LegacyTrim")&&D->GetMaterial(0)==DressingMaterial.Get(),TEXT("exact old-car trim installed with original dressing atlas"));return Ok;
}

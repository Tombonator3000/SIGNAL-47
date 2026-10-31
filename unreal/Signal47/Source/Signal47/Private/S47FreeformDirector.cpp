#include "S47FreeformDirector.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Main.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Night.h"
#include "S47Glimpses.h"
#include "S47RoswellWorld.h"
#include "S47Audio.h"
#include "S47UI.h"
#include "S47Inter.h"
#include "S47Walker.h"
#include "S47Story.h"
#include "S47Album.h"
#include "S47Three.h"
#include "S47Docs.h"
#include "S47Flow.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "../../../../Narrative/Freeform/generated/FreeformEvents.inl"

DEFINE_LOG_CATEGORY_STATIC(LogS47FreeformDirector,Log,All);
namespace
{
 struct FEvent { FString Id,Family,Action,Finding; int32 Day=1; bool bNight=false;double Duration=60; };
 const TArray<FEvent>& Catalog()
 {
  static const TArray<FEvent> Data=[]
  {
   TArray<FEvent> Out;auto Root=S47J::Read(FreeformEventsJson);
   for(const auto& Value:S47J::Arr(Root,TEXT("events")))
   {const auto O=Value->AsObject();Out.Add({S47J::Str(O,TEXT("id")),S47J::Str(O,TEXT("family")),S47J::Str(O,TEXT("action")),S47J::Str(O,TEXT("finding")),S47J::Int(O,TEXT("day"),1),S47J::Bool(O,TEXT("night")),S47J::Num(O,TEXT("duration"),60)});}
   return Out;
  }();return Data;
 }
 const FEvent* Definition(const FString& Id){return Catalog().FindByPredicate([&](const FEvent& D){return D.Id==Id;});}
 bool Instrument(const FString& Id){const auto* D=Definition(Id);return D && D->Family==TEXT("instrument");}
 FVector ReadVector(const S47J::FObj& O,const TCHAR* Key,const FVector& Default=FVector::ZeroVector)
 {const auto A=S47J::Nums(O,Key);if(A.Num()!=3)return Default;for(double V:A)if(!FMath::IsFinite(V)||FMath::Abs(V)>1e6)return Default;return FVector(A[0],A[1],A[2]);}
}

struct FS47FreeformDirector::FEffects
{
 US47Main* M=nullptr;
 FS47RoswellWorld Query;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Parts;
 TArray<TWeakObjectPtr<UPointLightComponent>> Lights;
 FS47Obj Sample,Car;
 TSharedPtr<uint8> Life=MakeShared<uint8>(0);
 bool bBuilt=false;
 FVector HullCentreOffsetUE=FVector::ZeroVector;
 double HullLampDrop=.25;
 ~FEffects(){Life.Reset();for(auto& P:Parts)if(P.IsValid())P->DestroyComponent();for(auto& L:Lights)if(L.IsValid())L->DestroyComponent();}
 UStaticMeshComponent* Mesh(const FString& Name,const TCHAR* Asset,const FVector& At,const FVector& Scale,uint32 Color,bool bOriginal=false)
 {
  if(!M||!M->Web)return nullptr;auto* W=M->Web.Get();auto* Mesh=LoadObject<UStaticMesh>(nullptr,Asset);if(!Mesh)return nullptr;
  auto* C=NewObject<UStaticMeshComponent>(W,*Name);C->SetupAttachment(W->GetRootComponent());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);
  if(!bOriginal)if(auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque")))
  {auto* Mat=UMaterialInstanceDynamic::Create(Base,W);Mat->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(Color));Mat->SetScalarParameterValue(TEXT("Roughness"),.6);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);Mat->SetScalarParameterValue(TEXT("EmissivePower"),0);C->SetMaterial(0,Mat);}
  C->RegisterComponent();C->SetWorldLocation(S47Three::ToUE(At));C->SetWorldScale3D(FVector(Scale.X,Scale.Z,Scale.Y));C->SetVisibility(false);Parts.Add(C);return C;
 }
 void Build(FS47FreeformDirector* Owner)
 {
  if(bBuilt||!M||!M->Web||!M->Web->IsLoaded()||!M->Inter)return;Query.Init(M);
  // Reuse Tom's imported saucer when locally available. Optional licensed art
  // is never required for compiling or running the public-source fallback.
  const TCHAR* SaucerPath=TEXT("/Game/Licensed/TripoFleet/ufo_saucer/SM_TripoFleet_Saucer.SM_TripoFleet_Saucer");
  UStaticMeshComponent* Hull=nullptr;
  if(FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(SaucerPath))))
   Hull=Mesh(TEXT("FreeformUfoHull"),SaucerPath,FVector(5,35,-80),FVector(1),0xffffff,true);
  if(Hull)
  {
   const FBoxSphereBounds B=Hull->GetStaticMesh()->GetBounds();
   const double Width=FMath::Max(1.,2.*double(FMath::Max(B.BoxExtent.X,B.BoxExtent.Y)));
   const double Scale=800./Width; // native centimetres: eight metres across
   Hull->SetWorldScale3D(FVector(Scale));HullCentreOffsetUE=FVector(B.Origin)*Scale;
   HullLampDrop=FMath::Clamp(double(B.BoxExtent.Z)*Scale*.0075+.05,.25,1.2);
   UE_LOG(LogS47FreeformDirector,Display,TEXT("S47: Freeform close-pass native saucer width=%.2fm scale=%.5f; original PBR retained"),Width*Scale/100.,Scale);
  }
  else
   Mesh(TEXT("FreeformUfoHull"),TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),FVector(5,35,-80),FVector(8,.4,8),0x393c3c);
  for(int32 I=0;I<3;++I)
  {
   auto* C=Mesh(FString::Printf(TEXT("FreeformUfoLamp%d"),I),TEXT("/Engine/BasicShapes/Sphere.Sphere"),FVector(5,35,-80),FVector(.35),0xc7dbbc);
   if(C)if(auto* Mat=Cast<UMaterialInstanceDynamic>(C->GetMaterial(0)))Mat->SetScalarParameterValue(TEXT("EmissivePower"),6.f);
   auto* L=NewObject<UPointLightComponent>(M->Web.Get());L->SetupAttachment(M->Web->GetRootComponent());L->SetMobility(EComponentMobility::Movable);L->SetIntensity(0);L->SetAttenuationRadius(900);L->SetCastShadows(false);L->SetLightColor(FLinearColor(.65,.85,.72));L->RegisterComponent();Lights.Add(L);
  }
  auto Register=[&](UStaticMeshComponent* C,const TCHAR* Name,const TCHAR* Id,FS47Obj& Obj)
  {
   if(!C)return;const int32 Parent=M->Web->Handle(TEXT("world.grounds.group"));Obj=M->Web->ObjAt(M->Web->AdoptNode(C,Parent,Name));M->Web->MarkUsable(Obj.I,Id);
   const TWeakPtr<uint8> Guard=Life;FS47Interactable It;It.Id=Id;It.Object=Obj;It.Range=2.4;
   It.Label=[Owner,Guard,Key=FString(Id)]{return Guard.IsValid()?Owner->Label(Key):FString();};
   It.Use=[Owner,Guard,Key=FString(Id)]{if(Guard.IsValid())Owner->Use(Key);};
   M->Inter->Add(It);
  };
  Register(Mesh(TEXT("FreeformTrace"),TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),FVector(12,.025,-12),FVector(.34,.035,.34),0x9f9c7b),TEXT("FreeformTrace"),TEXT("ff:event-sample"),Sample);
  auto* Sedan=Mesh(TEXT("FreeformEmptyCar"),TEXT("/Game/Free/ServiceFleet/SM_SaroPark_Sedan.SM_SaroPark_Sedan"),FVector(15,0,7),FVector(1),0xffffff,true);
  if(Sedan)
  {
   // Imported dressing may retain an authored mesh offset. Place its actual bounds on the bay.
   const FBoxSphereBounds B=Sedan->GetStaticMesh()->GetBounds();const double Scale=420./FMath::Max(1.,double(FMath::Max(B.BoxExtent.X,B.BoxExtent.Y)*2));
   Sedan->SetWorldScale3D(FVector(Scale));const FVector Offset(B.Origin.X,B.Origin.Y,B.Origin.Z-B.BoxExtent.Z);
   Sedan->SetWorldLocation(S47Three::ToUE(FVector(15,0,7))-Offset*Scale);
  }
  Register(Sedan,TEXT("FreeformEmptyCar"),TEXT("ff:event-check"),Car);
  bBuilt=true;
 }
 void Hide()
 {for(auto& P:Parts)if(P.IsValid()){P->SetVisibility(false);P->SetCollisionEnabled(ECollisionEnabled::NoCollision);}for(auto& L:Lights)if(L.IsValid())L->SetIntensity(0);}
 void Physical(FS47Obj O,bool bVisible)
 {
  O.SetVisible(bVisible);if(M&&M->Web)if(const auto* N=M->Web->Node(O.I))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))
  {C->SetCollisionEnabled(bVisible?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);}
 }
 bool Visible(const FVector& At)const
 {
  if(!M||!M->Night||!M->Web||!M->Web->GetWorld())return false;const auto Cam=M->Night->GlimpseIn(true).Cam;const FVector To=At-Cam.Pos;
  if(To.Size()<.01||FVector::DotProduct(To.GetSafeNormal(),Cam.Forward)<.72)return false;
  FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(FreeformEventSight),true);if(M->Player)Params.AddIgnoredActor(M->Player);
  const bool Blocked=M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(Cam.Pos),S47Three::ToUE(At),ECC_Visibility,Params);
  if(!Blocked)return true;for(const auto& P:Parts)if(P.Get()==Hit.Component.Get())return true;return false;
 }
};

FS47FreeformDirector::FS47FreeformDirector(US47Main* Main):Effects(MakeUnique<FEffects>()),M(Main){Effects->M=Main;Random.Initialize(4701986);}
FS47FreeformDirector::~FS47FreeformDirector(){Stop();}
void FS47FreeformDirector::Begin(const S47J::FObj& O)
{
 Stop();bEnabled=true;bEffectsApplied=false;Event.Empty();PhotoId.Empty();Scene=TEXT("saro");Stage=Variant=SeenDay=MinorLeft=InvestLeft=ThreatRollDay=MajorRollDay=LastThreatDay=0;LastMajorDay=-3;
 Age=ActiveTime=0;NextAt=90;RestUntil=SoundAt=0;LastPhotoAt=-10;LastDays.Reset();bSeen=bPhoto=bControlled=bCompared=false;Anchor=FVector::ZeroVector;Random.Initialize(M && M->Flow && !M->Flow->bDevRun ? int32(FDateTime::UtcNow().GetTicks() ^ FPlatformTime::Cycles64()) : 4701986);
 if(O)
 {
  if(S47J::Num(O,TEXT("v"))!=1){bEnabled=false;return;}
  Event=S47J::Str(O,TEXT("event"));if(!Event.IsEmpty()&&!Definition(Event)){bEnabled=false;return;}
  PhotoId=S47J::Str(O,TEXT("photo"));Scene=S47J::Str(O,TEXT("scene"),TEXT("saro"));Stage=FMath::Clamp(S47J::Int(O,TEXT("stage")),0,3);Variant=FMath::Clamp(S47J::Int(O,TEXT("variant")),0,2);
  SeenDay=S47J::Int(O,TEXT("seenDay"));MinorLeft=FMath::Clamp(S47J::Int(O,TEXT("minorLeft")),0,4);InvestLeft=FMath::Clamp(S47J::Int(O,TEXT("investLeft")),0,2);ThreatRollDay=S47J::Int(O,TEXT("threatRollDay"));MajorRollDay=S47J::Int(O,TEXT("majorRollDay"));LastThreatDay=S47J::Int(O,TEXT("lastThreatDay"));LastMajorDay=S47J::Int(O,TEXT("lastMajorDay"),-3);
  auto Time=[&](const TCHAR* K,double Max=1e9){const double V=S47J::Num(O,K);return FMath::IsFinite(V)?FMath::Clamp(V,0.,Max):0.;};
  Age=Time(TEXT("age"),1000);ActiveTime=Time(TEXT("activeTime"));NextAt=Time(TEXT("nextAt"));RestUntil=Time(TEXT("restUntil"));SoundAt=Time(TEXT("soundAt"));
  // Logical event effects and native presentation have different lifetimes.
  // An older v1 save with elapsed time has already applied its one-shot effects.
  bEffectsApplied=S47J::Bool(O,TEXT("effectsApplied"),Age>0||Stage>0);
  bSeen=S47J::Bool(O,TEXT("seen"));bPhoto=S47J::Bool(O,TEXT("photographed"));bControlled=S47J::Bool(O,TEXT("controlled"));bCompared=S47J::Bool(O,TEXT("compared"));Anchor=ReadVector(O,TEXT("anchor"));
  Random.Initialize(S47J::Int(O,TEXT("random"),4701986));
  if(const auto H=S47J::Obj(O,TEXT("history")))for(const auto& Pair:H->Values)if(Definition(Pair.Key))LastDays.Add(Pair.Key,int32(Pair.Value->AsNumber()));
  GroupRestore=S47J::Clone(S47J::Obj(O,TEXT("group")));
 }
 bApplied=false;bChoiceStored=!Event.IsEmpty();
}
void FS47FreeformDirector::Stop()
{
 if(Effects)Effects->Hide();if(M&&M->Audio){M->Audio->Stop(TEXT("freeformEvent"));M->Audio->HoldRoomTone(1);}
 if(M&&M->Night&&M->Night->Glimpses)M->Night->Glimpses->DevClear();
 bEnabled=bApplied=false;GroupRestore.Reset();
}
S47J::FObj FS47FreeformDirector::Save()const
{
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("event"),Event);S47J::Set(O,TEXT("photo"),PhotoId);S47J::Set(O,TEXT("scene"),Scene);S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("variant"),Variant);
 S47J::Set(O,TEXT("seenDay"),SeenDay);S47J::Set(O,TEXT("minorLeft"),MinorLeft);S47J::Set(O,TEXT("investLeft"),InvestLeft);S47J::Set(O,TEXT("threatRollDay"),ThreatRollDay);S47J::Set(O,TEXT("majorRollDay"),MajorRollDay);S47J::Set(O,TEXT("lastThreatDay"),LastThreatDay);S47J::Set(O,TEXT("lastMajorDay"),LastMajorDay);
 S47J::Set(O,TEXT("age"),Age);S47J::Set(O,TEXT("activeTime"),ActiveTime);S47J::Set(O,TEXT("nextAt"),NextAt);S47J::Set(O,TEXT("restUntil"),RestUntil);S47J::Set(O,TEXT("soundAt"),SoundAt);S47J::Set(O,TEXT("random"),Random.GetCurrentSeed());
 S47J::Set(O,TEXT("effectsApplied"),bEffectsApplied);
 S47J::Set(O,TEXT("seen"),bSeen);S47J::Set(O,TEXT("photographed"),bPhoto);S47J::Set(O,TEXT("controlled"),bControlled);S47J::Set(O,TEXT("compared"),bCompared);S47J::Set(O,TEXT("anchor"),TArray<double>{Anchor.X,Anchor.Y,Anchor.Z});
 auto H=S47J::New();for(const auto& Pair:LastDays)S47J::Set(H,*Pair.Key,Pair.Value);S47J::Set(O,TEXT("history"),H);
 if(GroupRestore)S47J::Set(O,TEXT("group"),GroupRestore);else if(M&&M->Night&&M->Night->Glimpses)S47J::Set(O,TEXT("group"),M->Night->Glimpses->SaveControlled());
 return O;
}
bool FS47FreeformDirector::Busy()const{return bEnabled&&!Event.IsEmpty();}
bool FS47FreeformDirector::Night()const
 {if(!M||!M->Freeform)return false;const double H=FMath::Fmod(M->Freeform->Clock(),86400.)/3600.;return H>=19||H<6;}
bool FS47FreeformDirector::Near(const FVector& At,double Range)const
 {return M&&M->Player&&FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(At.X,At.Z))<=Range;}
bool FS47FreeformDirector::InView(const FVector& At,double Range)const
 {return M&&M->Night&&FVector::Dist(M->Night->GlimpseIn(true).Cam.Pos,At)<=Range&&Effects->Visible(At);}
void FS47FreeformDirector::Changed(){if(M)M->RequestAutosave();}
TArray<FString> FS47FreeformDirector::EventIds(){TArray<FString> Out;for(const auto& D:Catalog())Out.Add(D.Id);return Out;}
bool FS47FreeformDirector::Eligible(const FString& Id,bool Forced)const
{
 const auto* D=Definition(Id);if(!bEnabled||!D||!M||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!Event.IsEmpty()||M->Freeform->Held())return false;
 // The initial catalogue is anchored to the actual SARO yard and apparatus.
 if(M->Web->Area()!=TEXT("saro"))return false;
 if(!Forced)
 {
  // An instrumental anomaly needs an actually completed normal reference first.
  if(Instrument(Id)&&!M->Freeform->Jobs().ContainsByPredicate([](const FS47FreeformSession::FJob& J){return J.Source==0&&J.Stage==FS47FreeformSession::EJobStage::Filed;}))return false;
  if(Id==TEXT("unsigned_telex")&&M->Freeform->FindingCount()==0)return false;
  if(M->Freeform->Day()<D->Day||(D->bNight&&!Night())||ActiveTime<RestUntil)return false;
  if(const int32* Last=LastDays.Find(Id))if(M->Freeform->Day()-*Last<3)return false;
  if(D->Family==TEXT("threat")&&(LastThreatDay==M->Freeform->Day()||M->Freeform->Clock()<86400.+19*3600.))return false;
  if(D->Family==TEXT("major")&&(M->Freeform->Day()-LastMajorDay<3||M->Freeform->Clock()<3*86400.+19*3600.))return false;
 }
 if((Id==TEXT("observer")||Id==TEXT("service_presence"))&&(!M->Night||!M->Night->Glimpses||!M->Night->Glimpses->HasNativeBody()||!M->Night->Glimpses->Individuals().IsEmpty()))return false;
 if(Id==TEXT("s03_failure")&&(!M->Freeform->Field||M->Freeform->Field->HasArrayFault()||M->Freeform->PowerFault()))return false;
 if(Id==TEXT("feeder_fault")&&M->Freeform->PowerFault())return false;
 if(Id==TEXT("empty_car")&&(!Effects->Car||InView(FVector(15,1,7))))return false;
 if(Id==TEXT("landing_trace")&&InView(FVector(12,.1,-12),40))return false;
 return true;
}
bool FS47FreeformDirector::Start(const FString& Id,bool Forced)
{
 Effects->Build(this);if(!Eligible(Id,Forced))return false;
 Event=Id;Age=SoundAt=0;Stage=0;Variant=Random.RandRange(0,2);bSeen=bPhoto=bControlled=bCompared=false;PhotoId.Empty();bApplied=bEffectsApplied=bChoiceStored=false;Scene=M->Web->Area();
 Anchor=Id==TEXT("empty_car")?FVector(15,1,7):Id==TEXT("landing_trace")?FVector(12,.08,-12):Id==TEXT("quiet_zone")?FVector(12,1.1,-17):Id==TEXT("roof_steps")?FVector(-1.5,3.5,-1):FVector(11.9,1,-11);
 if(Id==TEXT("response_lights")||Id==TEXT("flyover")||Id==TEXT("close_pass"))Anchor=FVector(-30,Id==TEXT("close_pass")?28:55,-110);
 LastDays.Add(Id,M->Freeform->Day());if(const auto* D=Definition(Id)){if(D->Family==TEXT("threat"))LastThreatDay=M->Freeform->Day();if(D->Family==TEXT("major"))LastMajorDay=M->Freeform->Day();}
 // Save the random choice and phase before applying any perceptible effect.
 Changed();UE_LOG(LogS47FreeformDirector,Display,TEXT("S47: Freeform event %s variant%d day%d"),*Event,Variant,M->Freeform->Day());return true;
}
bool FS47FreeformDirector::Force(const FString& Id)
{ return M&&M->Flow&&M->Flow->bDevRun&&Start(Id,true); }
bool FS47FreeformDirector::CancelForTest()
{if(!M||!M->Flow||!M->Flow->bDevRun)return false;Finish(false);return true;}
void FS47FreeformDirector::Apply()
{
 if(bApplied||Event.IsEmpty()||!M||!M->Freeform)return;
 if(!bChoiceStored)
 {
  if(!M->Flow||M->Flow->bStarting||M->Flow->bRestoringNow)return;
  if(!M->Flow->bDevRun)
  {
   FString Reason;
   if(!M->Flow->AutosaveNow(&Reason))
   {
    // Nothing belonging to this event has touched the world yet. Fail quiet.
    UE_LOG(LogS47FreeformDirector,Warning,TEXT("S47: Freeform pending event cancelled before effects: %s"),*Reason);
    Event.Empty();Stage=0;NextAt=ActiveTime+120;Changed();
    if(M->UI)M->UI->Toast(TEXT("The pending field incident was suspended because the case could not be saved. ")+Reason,6);
    return;
   }
  }
  bChoiceStored=true;
 }
 bApplied=true;
 if(!bEffectsApplied)
 {
  // These mutate persistent gameplay state once. Recreating a saved incident's
  // presentation must never break a repaired feeder/relay, re-arm a cleared
  // anomaly or replay its initial warning/telex sound.
  bEffectsApplied=true;
  if(Instrument(Event)){M->Freeform->SetPhenomenon(Event);M->Freeform->SetAnomaly(true);}
  if(Event==TEXT("s03_failure")&&M->Freeform->Field)
  {
   M->Freeform->Field->TriggerServiceFault();
   if(!M->Freeform->Field->HasArrayFault()){Finish(false);return;}
  }
  if(Event==TEXT("feeder_fault"))M->Freeform->SetPowerFault(true);
  if(Event==TEXT("service_presence")&&Stage==0)
  {if(M->Audio)M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.6f,.At=Anchor});if(M->UI)M->UI->Toast(TEXT("A heavy scrape beyond the service door. Something moves against the yard fence."),5);}
  if(Event==TEXT("unsigned_telex")&&M->Audio)M->Audio->Sfx(TEXT("printer"),FS47Sound{.Gain=.25f,.At=FVector(-4,1,2)});
  Changed();
 }
 // Rehydrate only presentation. Existing encounter brains, field originals,
 // component fault states and the next scheduled sound come from their saves.
 if(Event==TEXT("landing_trace"))Effects->Physical(Effects->Sample,true);
 if(Event==TEXT("empty_car"))Effects->Physical(Effects->Car,true);
 Draw();
}
void FS47FreeformDirector::Draw()
{
 if(!Effects->bBuilt)return;
 const bool Sky=Event==TEXT("response_lights")||Event==TEXT("flyover")||Event==TEXT("close_pass");
 for(int32 I=0;I<4&&I<Effects->Parts.Num();++I)if(auto* C=Effects->Parts[I].Get())
 {
  const bool Hull=I==0;C->SetVisibility(Sky&&(!Hull||Event==TEXT("close_pass")));
  const double Side=Event==TEXT("response_lights")?0:Age*(Event==TEXT("close_pass")?2.3:5.);
  const FVector Centre=Anchor+FVector(Side,FMath::Sin(Age*.2)*.6,0);
  // The close-pass hull and its lamps share one transform. Three bounded
  // underside positions stay within the eight-metre hull instead of trailing
  // a displaced hull ten metres to the side of an unrelated light line.
  const FVector AttachedLamps[]={FVector(-2.55,-Effects->HullLampDrop,-1.35),FVector(2.55,-Effects->HullLampDrop,-1.35),FVector(0,-Effects->HullLampDrop,2.7)};
  const FVector Offset=Hull?FVector::ZeroVector:Event==TEXT("close_pass")?AttachedLamps[I-1]:FVector((I-2)*5.,(FMath::Sin(Age*.2+I)-FMath::Sin(Age*.2))*.6,0);
  const FVector At=Centre+Offset;
  C->SetWorldLocation(S47Three::ToUE(At)-(Hull?Effects->HullCentreOffsetUE:FVector::ZeroVector));
  if(!Hull)if(auto* Mat=Cast<UMaterialInstanceDynamic>(C->GetMaterial(0)))
  {
   // Slow independent lamps; honour calm-flash preference by keeping a steady output.
   const float On=M&&M->Flow&&M->Flow->bCalmFlash?1.f:float(.65+.35*FMath::Sin(Age*(bControlled?1.1:.6)+I*2.));
   Mat->SetScalarParameterValue(TEXT("EmissivePower"),5*On);
  }
  if(!Hull&&Effects->Lights.IsValidIndex(I-1))if(auto* L=Effects->Lights[I-1].Get())
  {L->SetWorldLocation(S47Three::ToUE(At));L->SetIntensity(Event==TEXT("close_pass")?80.f:0.f);}
 }
}
void FS47FreeformDirector::StepGroup(double Dt)
{
 if(!M||!M->Night||!M->Night->Glimpses)return;auto G=M->Night->Glimpses;
 if(GroupRestore){G->RestoreControlled(GroupRestore);GroupRestore.Reset();}
 auto Input=M->Night->GlimpseIn(true);Input.bOk=true;Input.bHeld=!M->Freeform||M->Freeform->Held();Input.May=false;Input.Scene=M->Web?M->Web->Area():FString();
 G->ControlledStep(Dt,Input);
}
void FS47FreeformDirector::Step(double Dt)
{
 if(!bEnabled||!M||!M->Freeform||!M->Freeform->Active()||!FMath::IsFinite(Dt)||Dt<=0)return;
 Effects->Build(this);StepGroup(Dt);if(M->Freeform->Held())return;
 Dt=FMath::Min(Dt,.25);ActiveTime+=Dt;
 const int32 Day=M->Freeform->Day();
 if(SeenDay!=Day){SeenDay=Day;MinorLeft=Random.RandRange(2,4);InvestLeft=Random.RandRange(0,2);NextAt=FMath::Max(ActiveTime+60,RestUntil);Changed();}
 if(Event.IsEmpty())
 {
  if(ActiveTime<NextAt||ActiveTime<RestUntil)return;NextAt=ActiveTime+Random.FRandRange(240,420);
  if(Night()&&Day>=2&&ThreatRollDay!=Day)
  {ThreatRollDay=Day;const bool Chosen=Random.FRand()<.35;Changed();if(Chosen&&Start(TEXT("service_presence"),false))return;}
  if(Night()&&Day>=4&&MajorRollDay!=Day)
  {MajorRollDay=Day;const bool Chosen=Random.FRand()<.10;Changed();if(Chosen&&Start(TEXT("close_pass"),false))return;}
  TArray<FString> Choices;
  for(const auto& D:Catalog())if(D.Family!=TEXT("threat")&&D.Family!=TEXT("major")&&Eligible(D.Id,false))
  {const bool Minor=D.Family==TEXT("minor")||D.Family==TEXT("maintenance");if((Minor&&MinorLeft>0)||(!Minor&&InvestLeft>0))Choices.Add(D.Id);}
  if(Choices.IsEmpty()){Changed();return;}
  const FString Id=Choices[Random.RandRange(0,Choices.Num()-1)];
  if(Start(Id,false)){const auto* D=Definition(Id);if(D->Family==TEXT("minor")||D->Family==TEXT("maintenance"))--MinorLeft;else --InvestLeft;Changed();}return;
 }
 Apply();if(!bApplied||Event.IsEmpty())return;Age+=Dt;Draw();const auto* Def=Definition(Event);if(!Def){Finish(false);return;}
 // Hardware can be repaired at a separate physical site (the feeder is on the
 // road). Complete against its saved authority before applying scene-local rules.
 if(Event==TEXT("s03_failure")&&M->Freeform->Field&&!M->Freeform->Field->HasArrayFault()){Finish(true);return;}
 if(Event==TEXT("feeder_fault")&&!M->Freeform->PowerFault()){Finish(true);return;}
 if(Scene!=M->Web->Area())
 {if(Event==TEXT("service_presence"))Finish(true,TEXT("The encounter ended when the player left the site."));return;}
 if(Event==TEXT("observer")||Event==TEXT("service_presence"))
 {
  auto G=M->Night?M->Night->Glimpses:TSharedPtr<FS47Glimpses>();if(!G)return;
  if(Stage==0&&Age>=6)
  {
   const auto Mode=Event==TEXT("service_presence")?FS47RoswellBrain::EMode::Hunt:FS47RoswellBrain::EMode::Curious;
   if(G->ControlledSpawn(1,Mode,false)>0){Stage=1;Changed();}
   else if(Age>35)Finish(false);return;
  }
  const auto Views=G->Individuals();
  for(const auto& I:Views)
  {
   bSeen|=InView(I.Eye,35);
   if(Event==TEXT("service_presence")&&Stage==1&&I.State==FS47RoswellBrain::EState::Contact&&!(M->World&&M->World->bDriving))
   {const FString Why=TEXT("The service-yard presence reached you. The secured work survives; the encounter is over.");Finish(false);if(M->Night&&M->Night->Glimpses)M->Night->Glimpses->DevClear();M->Freeform->Recover(Why);return;}
  }
  const bool Safe=M->Web->Rect(TEXT("room.bounds")).Contains(M->Player->Pos)&&!M->Web->IsDoorOpen(TEXT("doorSouth"))&&!M->Web->IsDoorOpen(TEXT("doorEast"));
  if(Stage==1&&((Event==TEXT("service_presence")&&Safe&&Age>15)||Age>Def->Duration))
  {G->ControlledRelease();Stage=2;Changed();}
  if(Stage==2&&(Views.IsEmpty()||Age>Def->Duration+30))
  {
   // End pressure after a bounded withdrawal even if the player keeps watching.
   // Finish releases the body to its normal hidden retirement; it never removes it in view.
   Finish(Event==TEXT("service_presence")||bPhoto);return;
  }
 }
 if((Event==TEXT("roof_steps")||Event==TEXT("quiet_zone")||Event==TEXT("flyover")||Event==TEXT("close_pass"))&&Age>=SoundAt)
 {
  SoundAt=Age+(Event==TEXT("roof_steps")?Random.FRandRange(2.5,5):Random.FRandRange(5,8));
  if(M->Audio)
  {
   FS47Sound O;O.At=Event==TEXT("roof_steps")?Anchor+FVector(FMath::Sin(Age)*2,0,0):Anchor+FVector(Age*2,0,0);O.Gain=Event==TEXT("roof_steps")?.45f:.2f;O.Lp=Event==TEXT("roof_steps")?1800:500;
   M->Audio->Sfx(Event==TEXT("roof_steps")?TEXT("stepConcrete0"):TEXT("thudSoft"),O);
  }
 }
 if(Event==TEXT("quiet_zone")&&M->Audio)
 {const bool Inside=Near(Anchor,5);M->Audio->HoldRoomTone(Inside?.15f:1.f);if(Stage==0&&Inside){Stage=1;Changed();}else if(Stage==1&&!Inside){Stage=2;Changed();}}
 if(Event==TEXT("roof_steps")&&Age>=6&&Near(FVector(9,0,0),8)&&!M->Web->Rect(TEXT("room.bounds")).Contains(M->Player->Pos))
 {bSeen=true;Stage=1;}
 if(Age>Def->Duration&&Event!=TEXT("observer")&&Event!=TEXT("service_presence"))
 {
  if(Def->Action==TEXT("service")||Def->Action==TEXT("feeder")){Finish(false);return;} // leave actual unresolved hardware state intact
  if(Event==TEXT("roof_steps")){Finish(bSeen);return;}
  Finish(bPhoto||bCompared||(Event==TEXT("quiet_zone")&&Stage==2));
 }
}
void FS47FreeformDirector::Finish(bool Evidence,const FString& Text)
{
 if(Event.IsEmpty())return;const auto* D=Definition(Event);const bool Threat=D&&D->Family==TEXT("threat");
 if(Evidence&&M&&M->Freeform&&D)
 {
  FString Record=Text.IsEmpty()?D->Finding:Text;
  Record+=FString::Printf(TEXT("\nFIELD POSITION: %.2f, %.2f, %.2f m. VARIANT %d."),Anchor.X,Anchor.Y,Anchor.Z,Variant);
  if(!PhotoId.IsEmpty())Record+=TEXT("\nRETAINED ORIGINAL PHOTOGRAPH: ")+PhotoId+TEXT(". The photograph remains in this case's album.");
  const FString Key=M->Freeform->HasFinding(Event)?Event+FString::Printf(TEXT(".day%d"),M->Freeform->Day()):Event;
  M->Freeform->RecordEvidence(Key,Record);
 }
 if(M&&M->Night&&M->Night->Glimpses&&(Event==TEXT("observer")||Threat))M->Night->Glimpses->ControlledRelease();
 if(Instrument(Event)&&M&&M->Freeform){M->Freeform->SetAnomaly(false);M->Freeform->SetPhenomenon(FString());}
 Effects->Hide();if(M&&M->Audio){M->Audio->Stop(TEXT("freeformEvent"));M->Audio->HoldRoomTone(1);}
 UE_LOG(LogS47FreeformDirector,Display,TEXT("S47: Freeform event %s ended evidence=%d"),*Event,Evidence);
 if(Threat)RestUntil=ActiveTime+480;else RestUntil=FMath::Max(RestUntil,ActiveTime+45);
 Event.Empty();Stage=0;bApplied=bEffectsApplied=false;NextAt=FMath::Max(NextAt,RestUntil);Changed();
}
FString FS47FreeformDirector::Label(const FString& Id)const
{
 if(!bEnabled||Event.IsEmpty())return {};
 if(Id==TEXT("ff:event-check")&&Event==TEXT("empty_car"))return TEXT("Inspect the unoccupied car and dashboard clock");
 if(Id==TEXT("ff:event-sample")&&Event==TEXT("landing_trace"))return bPhoto?TEXT("Inspect and retain the loose fragment"):TEXT("Photograph the marked surface before moving anything");
 return {};
}
bool FS47FreeformDirector::Use(const FString& Id)
{
 if(!bEnabled||Event.IsEmpty()||!M||!M->Freeform)return false;
 if(Id==TEXT("ff:control")){bControlled=true;Changed();return false;}
 if(Id==TEXT("ff:compare")&&Instrument(Event))
 {if(bControlled){bCompared=true;Finish(true);}return false;}
 if(Id==TEXT("ff:archive")&&Event==TEXT("response_lights")&&bControlled){Finish(true);return false;}
 if(Id==TEXT("ff:telex")&&Event==TEXT("unsigned_telex"))
 {
  FS47Doc Doc;Doc.Id=TEXT("ff_unsigned_telex_")+FString::FromInt(M->Freeform->Day());Doc.Kind=TEXT("telex");Doc.Title=TEXT("Unsigned line / retained original");Doc.Stamp=M->Story?M->Story->ClockText(true):FString();
  Doc.Page=TEXT("RECEIVED / NO VERIFIED SENDER\n\nYOUR PREVIOUS REGISTERED INTERVAL IS STILL PRESENT. RECHECK THE LOCAL TIMING PATH.\n\nThe header carries no originating station. Compare against the original observation; this message is a new source, not a correction to it.");
  if(M->Story)M->Story->AddDoc(Doc);if(M->UI)M->UI->Document(Doc);Finish(true);return true;
 }
 if(Id==TEXT("ff:event-check")&&Event==TEXT("empty_car")&&Near(FVector(15,0,7),4))
 {Finish(true);if(M->UI)M->UI->Toast(TEXT("Bonnet warm. Ignition off. The dashboard clock is six minutes behind the retained base reference. No driver in sight."),6);return true;}
 if(Id==TEXT("ff:event-sample")&&Event==TEXT("landing_trace")&&Near(Anchor,2.7))
 {if(!bPhoto){if(M->UI)M->UI->Toast(TEXT("Keep its position as evidence first: photograph the marked surface before lifting the fragment."),4);return true;}M->Freeform->AddItem(TEXT("fragment"),1);Finish(true);return true;}
 return false;
}
void FS47FreeformDirector::Photo(const FS47AlbumShot& Shot)
{
 if(!bEnabled||Event.IsEmpty()||Shot.Id.IsEmpty()||Shot.bUnavailable||ActiveTime-LastPhotoAt<.3)return;
 const FVector PhotoAt=(Event==TEXT("flyover")||Event==TEXT("close_pass"))?Anchor+FVector(Age*(Event==TEXT("close_pass")?2.3:5.),0,0):Anchor;
 bool Visible=InView(PhotoAt,180);
 if(Event==TEXT("observer")||Event==TEXT("service_presence"))
 {Visible=false;if(M&&M->Night&&M->Night->Glimpses)for(const auto& I:M->Night->Glimpses->Individuals())Visible|=InView(I.Eye,40);}
 if(!Visible)return;LastPhotoAt=ActiveTime;bSeen=bPhoto=true;PhotoId=Shot.Id;Changed();
 if(M&&M->UI)M->UI->Toast(TEXT("The photograph is retained with this field observation. It documents what was visible, not its origin."),4);
}
FString FS47FreeformDirector::ObservationNote()const
{
 if(Event==TEXT("wrong_source"))return TEXT("CONTROL NOTE: extra component persists off the expected source centre. Compare independent antenna groups.");
 if(Event==TEXT("time_disagreement"))return TEXT("CONTROL NOTE: receiver and independent timing path disagree by 3.2 s. Retain both timestamps.");
 if(Event==TEXT("duplicate_print"))return TEXT("PROVENANCE NOTE: a second spool impression has a different interval label. The retained original and source recording are unchanged.");
 if(Event==TEXT("extra_pulse"))return TEXT("CONTROL NOTE: faint extra pulses occur outside the expected period. Test a separate internal reference.");
 return {};
}
FString FS47FreeformDirector::Status()const
 {return Event.IsEmpty()?FString(TEXT("No active field incident")):FString::Printf(TEXT("%s / phase %d / %.0f s"),*Event,Stage,Age);}
bool FS47FreeformDirector::SelfTest(TArray<FString>& Lines)
{
 bool Ok=true;auto Check=[&](bool B,const TCHAR* S){Ok&=B;Lines.Add(FString(B?TEXT("PASS "):TEXT("FAIL "))+TEXT("freeform director: ")+S);};
 Check(Catalog().Num()==16,TEXT("sixteen authored event definitions are embedded in packaged runtime"));TSet<FString> Ids;
 for(const auto& D:Catalog()){Check(!Ids.Contains(D.Id)&&!D.Finding.IsEmpty()&&D.Day>=1&&D.Duration>0,TEXT("event ID, finding and finite duration"));Ids.Add(D.Id);}
 FS47FreeformDirector D(nullptr);D.Begin(nullptr);auto Saved=D.Save();FS47FreeformDirector R(nullptr);R.Begin(Saved);
 Check(S47J::Write(Saved)==S47J::Write(R.Save()),TEXT("idle director seed, budgets and schedule round-trip without reroll"));
 auto Active=S47J::Clone(Saved);S47J::Set(Active,TEXT("event"),TEXT("extra_pulse"));S47J::Set(Active,TEXT("age"),12.5);S47J::Set(Active,TEXT("controlled"),true);R.Begin(Active);
 Check(R.ActiveEvent()==TEXT("extra_pulse")&&R.EventAge()==12.5&&R.Busy(),TEXT("pending incident restores its selected identity, control and phase"));
 auto Applied=S47J::Clone(Active);S47J::Set(Applied,TEXT("event"),TEXT("feeder_fault"));S47J::Set(Applied,TEXT("effectsApplied"),true);R.Begin(Applied);
 Check(S47J::Bool(R.Save(),TEXT("effectsApplied"))&&R.EventId()==TEXT("feeder_fault"),TEXT("saved applied hardware incident keeps its consumed-effect fact across presentation restore"));
 Applied->RemoveField(TEXT("effectsApplied"));R.Begin(Applied);
 Check(S47J::Bool(R.Save(),TEXT("effectsApplied")),TEXT("older elapsed v1 incident is treated as already applied instead of retriggering hardware"));
 auto Future=S47J::New();S47J::Set(Future,TEXT("v"),2);R.Begin(Future);Check(!R.Busy(),TEXT("unknown revision never starts an invented event"));
 FS47RoswellBrain Brain;Brain.Start(FS47RoswellBrain::EMode::Curious,FVector(2,0,3),.4,77);Brain.bHasPlayerMemory=true;Brain.LastKnownPlayer=FVector(8,0,9);auto B=Brain.Save();FS47RoswellBrain Copy;
 Check(Copy.Restore(B)&&Copy.Pos==Brain.Pos&&Copy.LastKnownPlayer==Brain.LastKnownPlayer&&Copy.bHasPlayerMemory,TEXT("active native observer keeps identity-space position and measured player memory"));
 return Ok;
}

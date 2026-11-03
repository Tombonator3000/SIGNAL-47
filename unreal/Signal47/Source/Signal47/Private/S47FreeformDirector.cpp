#include "S47FreeformDirector.h"
#include "S47UfoPresentation.h"
#include "S47HighwayUfo.h"
#include "S47Freeform.h"
#include "S47RoomLights.h"
#include "Misc/CommandLine.h"
#include "S47FreeformWorld.h"
#include "S47Main.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Night.h"
#include "S47SkyVisitors.h"
#include "S47Glimpses.h"
#include "S47Audio.h"
#include "S47UI.h"
#include "S47Inter.h"
#include "S47Walker.h"
#include "S47Story.h"
#include "S47Album.h"
#include "S47FieldCamera.h"
#include "S47Image.h"
#include "S47Three.h"
#include "S47Docs.h"
#include "S47Flow.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "CoreGlobals.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/GarbageCollection.h"
#include "../../../../Narrative/Freeform/generated/FreeformEvents.inl"

DEFINE_LOG_CATEGORY_STATIC(LogS47FreeformDirector,Log,All);
namespace
{
 // The original west staff bay was cleared when the normal sedan moved to
 // east35 (S47FleetParked). Its paved surface is -0.576 m, matching both native cars.
 const FVector EmptyCarFoot(27,-.576,9.8);
 const FVector EmptyCarReference=EmptyCarFoot+FVector(0,.9,0);
 FVector ClosePassAt(const FVector& Origin,double Age)
 {
  const double Near=FMath::Clamp(1.-FMath::Abs(Age-30.)/16.,0.,1.);
  return Origin+FVector(Age*2.3,-8.*Near+FMath::Max(0.,Age-48.)*2.,0);
 }
 struct FEvent { FString Id,Family,Action,Finding; int32 Day=1; bool bNight=false;double Duration=60; };
 const TArray<FEvent>& Catalog()
 {
  static const TArray<FEvent> Data=[]
  {
   TArray<FEvent> Out;auto Root=S47J::Read(FreeformEventsJson);
   for(const auto& Value:S47J::Arr(Root,TEXT("events")))
   {const auto O=Value->AsObject();Out.Add({S47J::Str(O,TEXT("id")),S47J::Str(O,TEXT("family")),S47J::Str(O,TEXT("action")),S47J::Str(O,TEXT("finding")),S47J::Int(O,TEXT("day"),1),S47J::Bool(O,TEXT("night")),S47J::Num(O,TEXT("duration"),60)});}
   Out.Add({TEXT("roomlight_return"),TEXT("minor"),TEXT("roomlight"),TEXT("A previously switched-off room light was on at return. The original switch operation and a physical readback are retained; no operator identified."),1,false,180});
   Out.Add({TEXT("feeder_presence"),TEXT("investigation"),TEXT("photo"),TEXT("A figure was photographed near the remote feeder. Its activity and any connection to equipment faults remain unconfirmed."),2,true,85});
   Out.Add({TEXT("array_presence"),TEXT("investigation"),TEXT("photo"),TEXT("A figure was photographed near the array service area. Check the physical cabinet and retained service log; the photograph alone does not establish interference."),3,true,85});
   return Out;
  }();return Data;
 }
 FString LightStamp(double Clock)
 {
  const int32 Day=FMath::Max(1,int32(FMath::FloorToDouble((Clock-28800.)/86400.))+1);
  const int32 Seconds=int32(FMath::Fmod(FMath::Max(0.,Clock),86400.));
  return FString::Printf(TEXT("DAY %d / %02d:%02d:%02d"),Day,Seconds/3600,(Seconds/60)%60,Seconds%60);
 }
 const FEvent* Definition(const FString& Id){return Catalog().FindByPredicate([&](const FEvent& D){return D.Id==Id;});}
 bool Instrument(const FString& Id){const auto* D=Definition(Id);return D && D->Family==TEXT("instrument");}
 FVector ReadVector(const S47J::FObj& O,const TCHAR* Key,const FVector& Default=FVector::ZeroVector)
 {const auto A=S47J::Nums(O,Key);if(A.Num()!=3)return Default;for(double V:A)if(!FMath::IsFinite(V)||FMath::Abs(V)>1e6)return Default;return FVector(A[0],A[1],A[2]);}
}

struct FS47FreeformDirector::FEffects
{
 US47Main* M=nullptr;
 TUniquePtr<FS47UfoPresentation> Ufo;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Parts;
 TArray<TWeakObjectPtr<UPointLightComponent>> Lights;
 FS47Obj Sample,Car;
 TSharedPtr<uint8> Life=MakeShared<uint8>(0);
 bool bBuilt=false;
 FVector HullCentreOffsetUE=FVector::ZeroVector;
 double HullLampDrop=.25;
 bool CanTouchWorld()const
 {
  if(IsEngineExitRequested()||IsGarbageCollecting()||!IsValid(M)||M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed))return false;
  AS47Web* Web=M->Web.Get();
  return IsValid(Web)&&!Web->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&!Web->IsActorBeingDestroyed()&&IsValid(Web->GetWorld())&&!Web->GetWorld()->bIsTearingDown;
 }
 ~FEffects()
 {
  Life.Reset();
  // Native components belong to the actor. During purge its maps and component
  // ownership may already be gone; Unreal performs their remaining destruction.
  if(!CanTouchWorld())return;
  auto Live=[](const UObject* O){return IsValid(O)&&!O->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed);};
  for(auto& P:Parts)if(Live(P.Get()))P->DestroyComponent();
  for(auto& L:Lights)if(Live(L.Get()))L->DestroyComponent();
 }
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
  if(bBuilt||!CanTouchWorld()||!M->Web->IsLoaded()||!M->Inter)return;
  Ufo=MakeUnique<FS47UfoPresentation>(M->Web.Get());
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
   if(!C)return;const int32 Parent=M->Web->Handle(TEXT("world.grounds.group"));Obj=M->Web->ObjAt(M->Web->AdoptNode(C,Parent,Name));
   // AdoptNode applies the default visible node flag. Retire the node itself,
   // not only its component, until this particular incident presents it.
   Obj.SetVisible(false);C->SetVisibility(false,false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->Web->MarkUsable(Obj.I,Id);
   const TWeakPtr<uint8> Guard=Life;FS47Interactable It;It.Id=Id;It.Object=Obj;It.Range=2.4;
   It.Label=[Owner,Guard,Key=FString(Id)]{return Guard.IsValid()?Owner->Label(Key):FString();};
   It.Use=[Owner,Guard,Key=FString(Id)]{if(Guard.IsValid())Owner->Use(Key);};
   M->Inter->Add(It);
  };
  Register(Mesh(TEXT("FreeformTrace"),TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),FVector(12,.025,-12),FVector(.34,.035,.34),0x9f9c7b),TEXT("FreeformTrace"),TEXT("ff:event-sample"),Sample);
  auto* Sedan=Mesh(TEXT("FreeformEmptyCar"),TEXT("/Game/Free/ServiceFleet/SM_SaroPark_Sedan.SM_SaroPark_Sedan"),EmptyCarFoot,FVector(1),0xffffff,true);
  if(Sedan)
  {
   // Imported dressing may retain an authored mesh offset. Place its actual bounds on the bay.
   const FBoxSphereBounds B=Sedan->GetStaticMesh()->GetBounds();const double Scale=420./FMath::Max(1.,double(FMath::Max(B.BoxExtent.X,B.BoxExtent.Y)*2));
   Sedan->SetWorldScale3D(FVector(Scale));const FVector Offset(B.Origin.X,B.Origin.Y,B.Origin.Z-B.BoxExtent.Z);
   Sedan->SetWorldLocation(S47Three::ToUE(EmptyCarFoot)-Offset*Scale);
  }
  Register(Sedan,TEXT("FreeformEmptyCar"),TEXT("ff:event-check"),Car);
  bBuilt=true;
 }
 void Hide()
 {
  if(!CanTouchWorld())return;
  if(Ufo)Ufo->Hide();
  for(const FS47Obj O:{Sample,Car})if(O.W==M->Web.Get()&&O)M->Web->SetVisible(O.I,false);
  for(auto& P:Parts)if(P.IsValid()){P->SetVisibility(false,false);P->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
  for(auto& L:Lights)if(L.IsValid())L->SetIntensity(0);
 }
 void Physical(FS47Obj O,bool bVisible)
 {
  if(!CanTouchWorld()||O.W!=M->Web.Get()||!O)return;
  // Unconditional node update repairs stale presentation too. A hidden scene
  // or ancestor still wins; component visibility never bypasses its parent.
  M->Web->SetVisible(O.I,bVisible);
  const bool Presented=bVisible&&O.Shown()&&M->Web->Area()==TEXT("saro");
  if(const auto* N=M->Web->Node(O.I))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))
  {C->SetVisibility(Presented,false);C->SetCollisionEnabled(Presented?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);}
 }
 bool PartPresented(int32 Index)const
 {
  if(!CanTouchWorld()||M->Web->Area()!=TEXT("saro")||!Parts.IsValidIndex(Index))return false;
  const auto* C=Parts[Index].Get();
  return IsValid(C)&&C->IsRegistered()&&C->IsVisible()&&!C->bHiddenInGame&&C->GetOwner()&&!C->GetOwner()->IsHidden();
 }
 UStaticMeshComponent* PresentedComponent(FS47Obj O)const
 {
  if(!CanTouchWorld()||O.W!=M->Web.Get()||!O||!O.Shown()||M->Web->Area()!=TEXT("saro"))return nullptr;
  if(const auto* N=M->Web->Node(O.I))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))
   if(IsValid(C)&&C->IsRegistered()&&C->IsVisible()&&!C->bHiddenInGame&&C->GetOwner()&&!C->GetOwner()->IsHidden())return C;
  return nullptr;
 }
 bool Visible(const FVector& At,const UPrimitiveComponent* Target=nullptr)const
 {
  if(!CanTouchWorld()||!IsValid(M->Night))return false;const auto Cam=M->Night->GlimpseIn(true).Cam;const FVector To=At-Cam.Pos;
  if(To.Size()<.01||FVector::DotProduct(To.GetSafeNormal(),Cam.Forward)<.72)return false;
  FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(FreeformEventSight),true);if(M->Player)Params.AddIgnoredActor(M->Player);
  const bool Blocked=M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(Cam.Pos),S47Three::ToUE(At),ECC_Visibility,Params);
  if(!Blocked)return true;if(Target)return Hit.Component.Get()==Target;for(const auto& P:Parts)if(P.Get()==Hit.Component.Get()&&P->IsVisible())return true;return false;
 }
};

FS47FreeformDirector::FS47FreeformDirector(US47Main* Main):Effects(MakeUnique<FEffects>()),M(Main){Effects->M=Main;Random.Initialize(4701986);}
FS47FreeformDirector::~FS47FreeformDirector(){Stop();}
void FS47FreeformDirector::Begin(const S47J::FObj& O)
{
 Stop();ReactiveRoom.Empty();ReactiveSerial=0;ReactiveAge=ReactiveOffClock=ReactiveOnClock=0;bReactiveLeft=bReactiveApplied=bReactiveInspected=false;bEnabled=true;bEffectsApplied=false;Event.Empty();PhotoId.Empty();Scene=TEXT("saro");Stage=Variant=SeenDay=MinorLeft=InvestLeft=ThreatRollDay=MajorRollDay=LastThreatDay=0;LastMajorDay=-3;
 PendingThreatNight=PendingMajorNight=0;IncidentSerial=0;IncidentId.Empty();TaggedShot.Empty();TaggedQuality=0;Claims.Reset();FieldObservations.Reset();
 Age=ActiveTime=0;NextAt=90;RestUntil=SoundAt=0;LastPhotoAt=-10;LastDays.Reset();bSeen=bPhoto=bControlled=bCompared=false;Anchor=FVector::ZeroVector;Random.Initialize(M && M->Flow && !M->Flow->bDevRun ? int32(FDateTime::UtcNow().GetTicks() ^ FPlatformTime::Cycles64()) : 4701986);
 if(O)
 {
  if(S47J::Num(O,TEXT("v"))!=1){bEnabled=false;return;}
  Event=S47J::Str(O,TEXT("event"));if(!Event.IsEmpty()&&!Definition(Event)){bEnabled=false;return;}
  IncidentSerial=FMath::Clamp(S47J::Int(O,TEXT("incidentSerial")),0,1000000000);
  IncidentId=S47J::Str(O,TEXT("incidentId"));
  for(const auto& V:S47J::Arr(O,TEXT("photoClaims")))
  {
   const S47J::FObj* C=nullptr;if(!V.IsValid()||!V->TryGetObject(C)||!C)continue;
   FPhotoClaim P;P.Id=S47J::Str(*C,TEXT("id"));P.PhotoId=S47J::Str(*C,TEXT("photo"));P.Title=S47J::Str(*C,TEXT("title"));P.bPaid=S47J::Bool(*C,TEXT("paid"));
   P.Quality=S47J::Num(*C,TEXT("quality"));
   if(P.Id.IsEmpty()||P.PhotoId.IsEmpty()||!FMath::IsFinite(P.Quality)||P.Quality<.025||P.Quality>1||Claims.ContainsByPredicate([&](const FPhotoClaim& Q){return Q.Id==P.Id;}))continue;
   Claims.Add(P);
  }
  for(const auto& V:S47J::Arr(O,TEXT("observations")))
  {
   const S47J::FObj* Entry=nullptr;FS47FreeformObservation Observation;
   if(V.IsValid()&&V->TryGetObject(Entry)&&Entry&&FS47FreeformObservation::Read(*Entry,Observation)&&!FieldObservations.ContainsByPredicate([&](const FS47FreeformObservation& P){return P.Id==Observation.Id;}))FieldObservations.Add(Observation);
  }
  PhotoId=S47J::Str(O,TEXT("photo"));Scene=S47J::Str(O,TEXT("scene"),TEXT("saro"));Stage=FMath::Clamp(S47J::Int(O,TEXT("stage")),0,3);Variant=FMath::Clamp(S47J::Int(O,TEXT("variant")),0,2);
  SeenDay=S47J::Int(O,TEXT("seenDay"));MinorLeft=FMath::Clamp(S47J::Int(O,TEXT("minorLeft")),0,4);InvestLeft=FMath::Clamp(S47J::Int(O,TEXT("investLeft")),0,2);ThreatRollDay=S47J::Int(O,TEXT("threatRollDay"));MajorRollDay=S47J::Int(O,TEXT("majorRollDay"));LastThreatDay=S47J::Int(O,TEXT("lastThreatDay"));LastMajorDay=S47J::Int(O,TEXT("lastMajorDay"),-3);
  // Old saves know that a roll was spent, but not whether an unavailable
  // event was chosen. Never reroll it or invent a pending event on migration.
  PendingThreatNight=FMath::Max(0,S47J::Int(O,TEXT("pendingThreatNight")));
  PendingMajorNight=FMath::Max(0,S47J::Int(O,TEXT("pendingMajorNight")));
  if(PendingThreatNight!=ThreatRollDay||Event==TEXT("service_presence"))PendingThreatNight=0;
  if(PendingMajorNight!=MajorRollDay||Event==TEXT("close_pass"))PendingMajorNight=0;
  auto Time=[&](const TCHAR* K,double Max=1e9){const double V=S47J::Num(O,K);return FMath::IsFinite(V)?FMath::Clamp(V,0.,Max):0.;};
  Age=Time(TEXT("age"),1000);ActiveTime=Time(TEXT("activeTime"));NextAt=Time(TEXT("nextAt"));RestUntil=Time(TEXT("restUntil"));SoundAt=Time(TEXT("soundAt"));
  // Logical event effects and native presentation have different lifetimes.
  // An older v1 save with elapsed time has already applied its one-shot effects.
  bEffectsApplied=S47J::Bool(O,TEXT("effectsApplied"),Age>0||Stage>0);
  bSeen=S47J::Bool(O,TEXT("seen"));bPhoto=S47J::Bool(O,TEXT("photographed"));bControlled=S47J::Bool(O,TEXT("controlled"));bCompared=S47J::Bool(O,TEXT("compared"));Anchor=ReadVector(O,TEXT("anchor"));
  Random.Initialize(S47J::Int(O,TEXT("random"),4701986));
  if(const auto H=S47J::Obj(O,TEXT("history")))for(const auto& Pair:H->Values)
  {
   const FString Key(*Pair.Key);double SavedDay=0;
   if(Definition(Key)&&Pair.Value.IsValid()&&Pair.Value->TryGetNumber(SavedDay)&&FMath::IsFinite(SavedDay))
    LastDays.Add(Key,int32(FMath::Clamp(SavedDay,0.,1000000.)));
  }
  if(const auto R=S47J::Obj(O,TEXT("reactiveLight")))
  {
   ReactiveRoom=S47J::Str(R,TEXT("room"));
   if(ReactiveRoom!=TEXT("control")&&ReactiveRoom!=TEXT("corridor")&&ReactiveRoom!=TEXT("records")&&ReactiveRoom!=TEXT("office"))ReactiveRoom.Empty();
   auto Value=[&](const TCHAR* Key,double Max){const double N=S47J::Num(R,Key);return FMath::IsFinite(N)?FMath::Clamp(N,0.,Max):0.;};
   ReactiveSerial=int32(Value(TEXT("serial"),1000000000.));ReactiveAge=Value(TEXT("age"),1e9);ReactiveOffClock=Value(TEXT("offClock"),3153600000.);ReactiveOnClock=Value(TEXT("onClock"),3153600000.);
   bReactiveLeft=S47J::Bool(R,TEXT("left"));bReactiveApplied=S47J::Bool(R,TEXT("applied"));bReactiveInspected=S47J::Bool(R,TEXT("inspected"));
   if(ReactiveSerial<=0)ReactiveRoom.Empty();
  }
  GroupRestore=S47J::Clone(S47J::Obj(O,TEXT("group")));
 }
 // References follow the corrected authored bay and the actual remote feeder;
 // these are incident locations, not survey measurements inferred from a log.
 if(Event==TEXT("empty_car"))Anchor=EmptyCarReference;
 if(Event==TEXT("feeder_fault")&&M&&M->Freeform&&M->Freeform->Field)Anchor=M->Freeform->Field->InteractionPoint(TEXT("ff:feeder"));
 if(!Event.IsEmpty()&&IncidentId.IsEmpty()){IncidentSerial=FMath::Max(1,IncidentSerial);IncidentId=FString::Printf(TEXT("%s.legacy.%d"),*Event,IncidentSerial);}
 bApplied=false;bChoiceStored=!Event.IsEmpty();
 if(M&&M->Freeform)ExpireNightChoices(M->Freeform->Day(),Night());
 if(M&&M->SkyVisitors)M->SkyVisitors->BeginFreeform(S47J::Obj(O,TEXT("highSky")));
}
void FS47FreeformDirector::Stop()
{
 bEnabled=bApplied=false;GroupRestore.Reset();
 if(IsEngineExitRequested()||IsGarbageCollecting()||!IsValid(M)||M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed))return;
 if(IsValid(M)&&M->SkyVisitors)M->SkyVisitors->StopFreeform();
 if(!Effects||!Effects->CanTouchWorld())return;
 Effects->Hide();
 auto Live=[](const UObject* O){return IsValid(O)&&!O->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed);};
 if(Live(M->Audio.Get())){M->Audio->Stop(TEXT("freeformEvent"));M->Audio->HoldRoomTone(1);}
 if(Live(M->Night.Get())&&M->Night->Glimpses)M->Night->Glimpses->DevClear();
}
S47J::FObj FS47FreeformDirector::Save()const
{
 auto O=S47J::New();if(M&&M->SkyVisitors)S47J::Set(O,TEXT("highSky"),M->SkyVisitors->SaveFreeform());S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("event"),Event);S47J::Set(O,TEXT("photo"),PhotoId);S47J::Set(O,TEXT("scene"),Scene);S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("variant"),Variant);
 S47J::Set(O,TEXT("seenDay"),SeenDay);S47J::Set(O,TEXT("minorLeft"),MinorLeft);S47J::Set(O,TEXT("investLeft"),InvestLeft);S47J::Set(O,TEXT("threatRollDay"),ThreatRollDay);S47J::Set(O,TEXT("majorRollDay"),MajorRollDay);S47J::Set(O,TEXT("lastThreatDay"),LastThreatDay);S47J::Set(O,TEXT("lastMajorDay"),LastMajorDay);
 S47J::Set(O,TEXT("pendingThreatNight"),PendingThreatNight);S47J::Set(O,TEXT("pendingMajorNight"),PendingMajorNight);
 S47J::Set(O,TEXT("age"),Age);S47J::Set(O,TEXT("activeTime"),ActiveTime);S47J::Set(O,TEXT("nextAt"),NextAt);S47J::Set(O,TEXT("restUntil"),RestUntil);S47J::Set(O,TEXT("soundAt"),SoundAt);S47J::Set(O,TEXT("random"),Random.GetCurrentSeed());
 S47J::Set(O,TEXT("effectsApplied"),bEffectsApplied);
 S47J::Set(O,TEXT("seen"),bSeen);S47J::Set(O,TEXT("photographed"),bPhoto);S47J::Set(O,TEXT("controlled"),bControlled);S47J::Set(O,TEXT("compared"),bCompared);S47J::Set(O,TEXT("anchor"),TArray<double>{Anchor.X,Anchor.Y,Anchor.Z});
 S47J::Set(O,TEXT("incidentSerial"),IncidentSerial);S47J::Set(O,TEXT("incidentId"),IncidentId);
 TArray<TSharedPtr<FJsonValue>> PaidPhotos;
 for(const auto& P:Claims){auto C=S47J::New();S47J::Set(C,TEXT("id"),P.Id);S47J::Set(C,TEXT("photo"),P.PhotoId);S47J::Set(C,TEXT("title"),P.Title);S47J::Set(C,TEXT("quality"),P.Quality);S47J::Set(C,TEXT("paid"),P.bPaid);PaidPhotos.Add(MakeShared<FJsonValueObject>(C));}
 O->SetArrayField(TEXT("photoClaims"),PaidPhotos);
 TArray<TSharedPtr<FJsonValue>> Observed;
 for(const auto& P:FieldObservations)Observed.Add(MakeShared<FJsonValueObject>(P.Save()));
 O->SetArrayField(TEXT("observations"),Observed);
 auto H=S47J::New();for(const auto& Pair:LastDays)S47J::Set(H,*Pair.Key,Pair.Value);S47J::Set(O,TEXT("history"),H);
 auto R=S47J::New();S47J::Set(R,TEXT("room"),ReactiveRoom);S47J::Set(R,TEXT("serial"),ReactiveSerial);S47J::Set(R,TEXT("age"),ReactiveAge);S47J::Set(R,TEXT("offClock"),ReactiveOffClock);S47J::Set(R,TEXT("onClock"),ReactiveOnClock);S47J::Set(R,TEXT("left"),bReactiveLeft);S47J::Set(R,TEXT("applied"),bReactiveApplied);S47J::Set(R,TEXT("inspected"),bReactiveInspected);S47J::Set(O,TEXT("reactiveLight"),R);
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
 if(M&&M->HighwayUfo&&M->HighwayUfo->Busy())return false;
 const auto* D=Definition(Id);if(!bEnabled||!D||!M||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!Event.IsEmpty()||M->Freeform->Held())return false;
 // The initial catalogue is anchored to the actual SARO yard and apparatus.
 if(Id==TEXT("feeder_presence"))
 {if(M->Web->Area()!=TEXT("road")||!M->Freeform->Field||!Near(M->Freeform->Field->InteractionPoint(TEXT("ff:feeder")),12))return false;}
 else if(M->Web->Area()!=TEXT("saro"))return false;
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
 if(Id==TEXT("array_presence")&&(!M->Freeform->Field||M->Freeform->Field->HasArrayFault()||!Near(M->Freeform->Field->InteractionPoint(TEXT("ff:service")),12)))return false;
 if(Id==TEXT("roomlight_return")&&!ReactiveReady())return false;
 if((Id==TEXT("observer")||Id==TEXT("service_presence")||Id==TEXT("feeder_presence")||Id==TEXT("array_presence"))&&(!M->Night||!M->Night->Glimpses||!M->Night->Glimpses->HasNativeBody()||!M->Night->Glimpses->Individuals().IsEmpty()))return false;
 if(Id==TEXT("s03_failure")&&(!M->Freeform->Field||M->Freeform->Field->HasArrayFault()||M->Freeform->PowerFault()))return false;
 if(Id==TEXT("feeder_fault")&&M->Freeform->PowerFault())return false;
 if(Id==TEXT("empty_car")&&(!Effects->Car||InView(EmptyCarReference)))return false;
 if(Id==TEXT("landing_trace")&&InView(FVector(12,.1,-12),40))return false;
 return true;
}
bool FS47FreeformDirector::Start(const FString& Id,bool Forced)
{
 Effects->Build(this);if(!Eligible(Id,Forced))return false;
 IncidentSerial=FMath::Min(1000000000,IncidentSerial+1);IncidentId=FString::Printf(TEXT("%s.day%d.%d"),*Id,M->Freeform->Day(),IncidentSerial);TaggedShot.Empty();TaggedQuality=0;
 Event=Id;Age=SoundAt=0;Stage=0;Variant=Random.RandRange(0,2);bSeen=bPhoto=bControlled=bCompared=false;PhotoId.Empty();bApplied=bEffectsApplied=bChoiceStored=false;Scene=M->Web->Area();
 Anchor=Id==TEXT("empty_car")?EmptyCarReference:Id==TEXT("landing_trace")?FVector(12,.08,-12):Id==TEXT("quiet_zone")?FVector(12,1.1,-17):Id==TEXT("roof_steps")?FVector(-1.5,3.5,-1):FVector(11.9,1,-11);
 if(Id==TEXT("response_lights")||Id==TEXT("flyover")||Id==TEXT("close_pass"))Anchor=FVector(-30,Id==TEXT("close_pass")?28:55,-110);
 // Store one world-space route origin; it never follows the player camera.
 if(Id==TEXT("close_pass")&&M->Player)Anchor=M->Player->Pos+FVector(-70,24,-38);
 if((Id==TEXT("feeder_fault")||Id==TEXT("feeder_presence"))&&M->Freeform->Field)Anchor=M->Freeform->Field->InteractionPoint(TEXT("ff:feeder"));
 if(Id==TEXT("array_presence"))Anchor=M->Freeform->Field->InteractionPoint(TEXT("ff:service"));
 if(Id==TEXT("roomlight_return"))Anchor=M->RoomLights->InteractionPoint(ReactiveRoom);
 LastDays.Add(Id,M->Freeform->Day());if(const auto* D=Definition(Id)){if(D->Family==TEXT("threat"))LastThreatDay=M->Freeform->Day();if(D->Family==TEXT("major"))LastMajorDay=M->Freeform->Day();}
 // Save the random choice and phase before applying any perceptible effect.
 Changed();UE_LOG(LogS47FreeformDirector,Display,TEXT("S47: Freeform event %s variant%d day%d"),*Event,Variant,M->Freeform->Day());return true;
}
bool FS47FreeformDirector::Force(const FString& Id)
{
 const bool Good=M&&M->Flow&&M->Flow->bDevRun&&Start(Id,true);
 if(!Good&&Id==TEXT("roomlight_return")&&FParse::Param(FCommandLine::Get(),TEXT("S47ReactiveLightTest")))
 {
  const FVector At=M&&M->RoomLights?M->RoomLights->InteractionPoint(ReactiveRoom):FVector::ZeroVector;
  const FVector Pos=M&&M->Player?M->Player->Pos:FVector::ZeroVector;
  const FVector Cam=M&&M->Night?M->Night->GlimpseIn(true).Cam.Pos:FVector::ZeroVector;
  const FVector Forward=M&&M->Night?M->Night->GlimpseIn(true).Cam.Forward:FVector::ZeroVector;
  UE_LOG(LogS47FreeformDirector,Display,TEXT("S47 reactive Force rejected: enabled=%d definition=%d valid=%d serial=%d inputSerial=%d room=%s age=%.6f left=%d applied=%d near25=%d distance=%.6f inView120=%d inRoom=%d power=%d held=%d active=%d event=%s area=%s loaded=%d ready=%d eligible=%d player=%s camera=%s forward=%s"),bEnabled,Definition(Id)!=nullptr,ReactiveValid(),ReactiveSerial,M&&M->RoomLights?M->RoomLights->InputSerial():-1,*ReactiveRoom,ReactiveAge,bReactiveLeft,bReactiveApplied,Near(At,25),FVector2D::Distance({Pos.X,Pos.Z},{At.X,At.Z}),InView(At,120),M&&M->RoomLights?M->RoomLights->PlayerInRoom(ReactiveRoom):false,M&&M->Freeform?M->Freeform->HasPowerForReactiveLight():false,M&&M->Freeform?M->Freeform->Held():true,M&&M->Freeform?M->Freeform->Active():false,*Event,M&&M->Web?*M->Web->Area():TEXT("missing"),M&&M->Web?M->Web->IsLoaded():false,ReactiveReady(),Eligible(Id,true),*Pos.ToString(),*Cam.ToString(),*Forward.ToString());
 }
 return Good;
}
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
  if(Event==TEXT("roomlight_return"))
  {
   if(!ReactiveReady()||!M->RoomLights->SetEventLight(ReactiveRoom,ReactiveSerial)){Finish(false);return;}
   bReactiveApplied=true;ReactiveOnClock=M->Freeform->Clock();
   M->Freeform->RecordEvidence(FString::Printf(TEXT("roomlight-original-%d"),ReactiveSerial),FString::Printf(TEXT("LOCAL SWITCH ENTRY / %s / %s\nWall switch set to OFF. This is the last manual operation recorded before the later circuit readback."),*ReactiveRoom,*LightStamp(ReactiveOffClock)));
  }
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
 Draw();
}
void FS47FreeformDirector::Draw()
{
 if(!Effects->bBuilt||!Effects->CanTouchWorld())return;
 // Scene visibility can change after the one-shot Apply (including a save
 // restored on the road). Reconcile both native render and query collision
 // whenever the active incident is presented; no gameplay effect is replayed.
 if(Event==TEXT("landing_trace"))Effects->Physical(Effects->Sample,true);
 if(Event==TEXT("empty_car"))Effects->Physical(Effects->Car,true);
 const bool Sky=Event==TEXT("response_lights")||Event==TEXT("flyover");
 if(Effects->Ufo)
 {
  if(Event==TEXT("close_pass")&&M->Web->Area()==Scene)
  {
   FS47UfoPresentation::FFrame F;
   const double Near=FMath::Clamp(1.-FMath::Abs(Age-30.)/16.,0.,1.);
   F.Position=ClosePassAt(Anchor,Age);
   F.GroundTarget=FVector(F.Position.X,US47Night::SaroFloorAt(M->Web,F.Position.X,F.Position.Z),F.Position.Z);
   F.Forward=FVector(1,0,0);F.Seconds=Age;F.Width=8;F.Beam=.4+.55*Near;F.NearWhite=Near*.7;
   F.Opacity=FMath::Min(FMath::Clamp(Age/2.,0.,1.),FMath::Clamp((65.-Age)/5.,0.,1.));
   F.bCalm=M->Flow&&M->Flow->bCalmFlash;Effects->Ufo->Show(F);
  }
  else Effects->Ufo->Hide();
 }
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
   Mat->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(I==2?0xffe9df:0xff4936));
   Mat->SetScalarParameterValue(TEXT("EmissivePower"),5*On);
  }
  if(!Hull&&Effects->Lights.IsValidIndex(I-1))if(auto* L=Effects->Lights[I-1].Get())
  {L->SetWorldLocation(S47Three::ToUE(At));L->SetIntensity(0.f);}
 }
}
void FS47FreeformDirector::RoomLightInput(const FString& Room,int32 Serial,bool Off)
{
 if(!bEnabled||!M||!M->IsFreeform()||!M->Freeform||!M->Freeform->Active()||!M->RoomLights||Serial!=M->RoomLights->InputSerial()||M->RoomLights->ManualEnabled(Room)==Off)return;
 if(Event==TEXT("roomlight_return"))Finish(false);
 ReactiveRoom=Off?Room:FString();ReactiveSerial=Off?Serial:0;ReactiveAge=0;
 ReactiveOffClock=M->Freeform->Clock();ReactiveOnClock=0;bReactiveLeft=bReactiveApplied=bReactiveInspected=false;Changed();
}
bool FS47FreeformDirector::ReactiveValid() const
{
 return M&&M->IsFreeform()&&M->RoomLights&&!ReactiveRoom.IsEmpty()&&ReactiveSerial>0&&M->RoomLights->InputSerial()==ReactiveSerial&&!M->RoomLights->ManualEnabled(ReactiveRoom);
}
bool FS47FreeformDirector::ReactiveReady() const
{
 return ReactiveValid()&&!bReactiveApplied&&bReactiveLeft&&ReactiveAge>=60&&M->Night&&!Near(M->RoomLights->InteractionPoint(ReactiveRoom),25)&&M->Freeform&&M->Freeform->HasPowerForReactiveLight()&&!M->RoomLights->PlayerInRoom(ReactiveRoom)&&!InView(M->RoomLights->InteractionPoint(ReactiveRoom),120);
}
void FS47FreeformDirector::StepReactive(double Dt)
{
 if(ReactiveRoom.IsEmpty())return;
 if(!ReactiveValid()||(bReactiveApplied&&!M->RoomLights->EventLightMatches(ReactiveRoom,ReactiveSerial)))
 {
  if(Event==TEXT("roomlight_return"))Finish(false);
  ReactiveRoom.Empty();ReactiveSerial=0;Changed();return;
 }
 if(!bReactiveApplied)
 {
  ReactiveAge+=Dt;
  // A turn away is insufficient: the player must physically leave the room.
  if(!bReactiveLeft&&!M->RoomLights->PlayerInRoom(ReactiveRoom)){bReactiveLeft=true;Changed();}
 }
}
FString FS47FreeformDirector::InspectRoomLight(const FString& Room)
{
 if(!bEnabled||!ReactiveValid()||Room!=ReactiveRoom||!bReactiveApplied||!M->RoomLights->EventLightMatches(Room,ReactiveSerial)||!M->Freeform||M->Freeform->Held()||!M->Freeform->HasPowerForReactiveLight()||!M->RoomLights->PlayerInRoom(Room)||!M->Player||!M->Inter)return {};
 const FString Id=TEXT("roomlight:")+Room;
 if(!Near(M->RoomLights->InteractionPoint(Room),2.2)||M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE())!=Id)return {};
 const FString Text=FString::Printf(TEXT("\nRETAINED CHECK / OFF entry: %s. The wall switch remains OFF, but the ceiling light is ON. Supply is present; no intervening switch operation is recorded. Both entries are in the case archive."),*LightStamp(ReactiveOffClock));
 if(!bReactiveInspected)
 {
  bReactiveInspected=true;
  M->Freeform->RecordEvidence(FString::Printf(TEXT("roomlight-control-%d"),ReactiveSerial),FString::Printf(TEXT("LOCAL CIRCUIT READBACK / %s / %s\nORIGINAL: roomlight-original-%d\n%s"),*Room,*LightStamp(M->Freeform->Clock()),ReactiveSerial,*Text));
  if(Event==TEXT("roomlight_return"))Finish(true,Text);else Changed();
 }
 return Text;
}
void FS47FreeformDirector::StepGroup(double Dt)
{
 if(!M||!M->Night||!M->Night->Glimpses)return;auto G=M->Night->Glimpses;
 if(GroupRestore){G->RestoreControlled(GroupRestore);GroupRestore.Reset();}
 auto Input=M->Night->GlimpseIn(true);Input.bOk=true;Input.bHeld=!M->Freeform||M->Freeform->Held();Input.May=false;Input.Scene=M->Web?M->Web->Area():FString();
 G->ControlledStep(Dt,Input);
}
bool FS47FreeformDirector::RollNightChoice(int32 Day,int32 FirstDay,float Chance,int32& RolledDay,int32& PendingNight)
{
 if(Day<FirstDay||RolledDay>=Day)return false;
 RolledDay=Day;PendingNight=Random.FRand()<Chance?Day:0;return true;
}
bool FS47FreeformDirector::ExpireNightChoices(int32 Day,bool bNight)
{
 bool ChangedChoice=false;
 for(int32* Pending:{&PendingThreatNight,&PendingMajorNight})
  if(*Pending>0&&(!bNight||*Pending!=Day)){*Pending=0;ChangedChoice=true;}
 return ChangedChoice;
}
bool FS47FreeformDirector::StartPendingNightChoice(const FString& Id,int32 Day,int32& PendingNight)
{
 // Selection survives a trip away, a busy native actor or a cooldown. All
 // normal gates still apply, and retries only happen at scheduler intervals.
 if(PendingNight!=Day||!Eligible(Id,false)||!Start(Id,false))return false;
 PendingNight=0;Changed();return true;
}
void FS47FreeformDirector::Step(double Dt)
{
 if(!bEnabled||!M||!M->Freeform||!M->Freeform->Active()||!FMath::IsFinite(Dt)||Dt<=0)return;
 Effects->Build(this);StepGroup(Dt);if(M->Freeform->Held())return;
 Dt=FMath::Min(Dt,.25);ActiveTime+=Dt;StepReactive(Dt);
 if(M->SkyVisitors)M->SkyVisitors->Step(Dt);
 const int32 Day=M->Freeform->Day();
 const bool bNight=Night();if(ExpireNightChoices(Day,bNight))Changed();
 if(SeenDay!=Day){SeenDay=Day;MinorLeft=Random.RandRange(2,4);InvestLeft=Random.RandRange(0,2);NextAt=FMath::Max(ActiveTime+60,RestUntil);Changed();}
 if(Event.IsEmpty())
 {
  if(ActiveTime<NextAt||ActiveTime<RestUntil)return;NextAt=ActiveTime+Random.FRandRange(240,420);
  if(bNight)
  {
   if(RollNightChoice(Day,2,.35f,ThreatRollDay,PendingThreatNight))Changed();
   if(StartPendingNightChoice(TEXT("service_presence"),Day,PendingThreatNight))return;
   if(RollNightChoice(Day,4,.10f,MajorRollDay,PendingMajorNight))Changed();
   if(StartPendingNightChoice(TEXT("close_pass"),Day,PendingMajorNight))return;
  }
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
 {if(Event==TEXT("service_presence")||Event==TEXT("feeder_presence")||Event==TEXT("array_presence")||Event==TEXT("close_pass")||Event==TEXT("flyover"))Finish(bPhoto,TEXT("The encounter ended when the player left the site. Any retained photograph records the observation, not its origin."));return;}
 if(Event==TEXT("observer")||Event==TEXT("service_presence")||Event==TEXT("feeder_presence")||Event==TEXT("array_presence"))
 {
  auto G=M->Night?M->Night->Glimpses:TSharedPtr<FS47Glimpses>();if(!G)return;
  if(Stage==0&&Age>=6)
  {
   if((Event==TEXT("feeder_presence")||Event==TEXT("array_presence"))&&!Near(Anchor,12)){Finish(false);return;}
   const auto Mode=Event==TEXT("service_presence")?FS47RoswellBrain::EMode::Hunt:FS47RoswellBrain::EMode::Curious;
   if(G->ControlledSpawn(1,Mode,false)>0){Stage=1;Changed();}
   else if(Age>35)Finish(false);return;
  }
  const auto Views=G->Individuals();
  for(const auto& I:Views)
  {
   const bool Visible=I.bRendered&&InView(I.Eye,35);
   bSeen|=Visible;
   if(Event==TEXT("observer")&&Visible)ObserveVisitor(I.Eye);
   if(Event==TEXT("array_presence")&&Stage==1&&!bControlled&&I.bRendered&&FVector::Dist(I.Pos,Anchor)<2.5&&M->Freeform->Field)
   {
    // A fault is only applied after a real native actor reaches the physical cabinet.
    // Presence plus a fault is a lead, not proof of intent or origin.
    bControlled=true;M->Freeform->Field->TriggerServiceFault();Changed();
   }
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
   FS47Sound O;O.At=Event==TEXT("roof_steps")?Anchor+FVector(FMath::Sin(Age)*2,0,0):Event==TEXT("close_pass")?ClosePassAt(Anchor,Age):Anchor+FVector(Age*2,0,0);O.Gain=Event==TEXT("roof_steps")?.45f:.2f;O.Lp=Event==TEXT("roof_steps")?1800:500;
   M->Audio->Sfx(Event==TEXT("roof_steps")?TEXT("stepConcrete0"):TEXT("thudSoft"),O);
   // A heard lead is tied to an emitted nearby impact, never to a timer alone.
   if(Event==TEXT("roof_steps")&&Near(O.At.GetValue(),12))ObserveVisitor(O.At.GetValue(),true);
  }
 }
 if(Event==TEXT("landing_trace")&&Effects->PresentedComponent(Effects->Sample)&&InView(Anchor,8))ObserveVisitor(Anchor);
 if(Event==TEXT("quiet_zone")&&M->Audio)
 {const bool Inside=Near(Anchor,5);M->Audio->HoldRoomTone(Inside?.15f:1.f);if(Stage==0&&Inside){Stage=1;Changed();}else if(Stage==1&&!Inside){Stage=2;Changed();}}
 if(Event==TEXT("roof_steps")&&Age>=6&&Near(FVector(9,0,0),8)&&!M->Web->Rect(TEXT("room.bounds")).Contains(M->Player->Pos))
 {bSeen=true;Stage=1;}
 if(Age>Def->Duration&&Event!=TEXT("observer")&&Event!=TEXT("service_presence")&&Event!=TEXT("feeder_presence")&&Event!=TEXT("array_presence"))
 {
  if(Def->Action==TEXT("service")||Def->Action==TEXT("feeder")){Finish(false);return;} // leave actual unresolved hardware state intact
  if(Event==TEXT("roof_steps")){Finish(bSeen);return;}
  Finish(bPhoto||bCompared||(Event==TEXT("quiet_zone")&&Stage==2));
 }
}
void FS47FreeformDirector::RetainObservation(const FS47FreeformObservation& Observation)
{
 if(!Observation.Valid())return;
 auto* Existing=FieldObservations.FindByPredicate([&](const FS47FreeformObservation& P){return P.Id==Observation.Id;});
 if(Existing)
 {
  // Preserve the first measured date/place/bearing. A later photograph is a supplement.
  if(!Existing->PhotoId.IsEmpty()||Observation.PhotoId.IsEmpty())return;
  Existing->PhotoId=Observation.PhotoId;
  if(M&&M->Freeform)M->Freeform->RecordEvidence(TEXT("observation-photo.")+Existing->Id,Existing->Text());
 }
 else
 {
  FieldObservations.Add(Observation);
  if(M&&M->Freeform)M->Freeform->RecordEvidence(TEXT("observation.")+Observation.Id,Observation.Text());
 }
 Changed();
}
void FS47FreeformDirector::ObserveVisitor(const FVector& At,bool Heard)
{
 if(!M||!M->Freeform||!M->Player||IncidentId.IsEmpty()||(Event!=TEXT("roof_steps")&&Event!=TEXT("observer")&&Event!=TEXT("landing_trace")))return;
 FS47FreeformObservation O;O.Id=IncidentId;O.Event=Event;O.Day=M->Freeform->Day();O.Clock=M->Freeform->Clock();O.bHeard=Heard;
 O.Bearing=FS47FreeformObservation::Direction(M->Player->Pos,At);O.PhotoId=PhotoId;
 static const TMap<FString,FString> Places=[]
 {
  TMap<FString,FString> Out;
  for(const auto& Value:S47J::Arr(S47J::Read(FreeformEventsJson),TEXT("events")))
  {const auto D=Value->AsObject();const FString Place=S47J::Str(D,TEXT("observationPlace"));if(!Place.IsEmpty())Out.Add(S47J::Str(D,TEXT("id")),Place);}
  return Out;
 }();
 if(const FString* Place=Places.Find(Event))O.Place=*Place;
 RetainObservation(O);
}
void FS47FreeformDirector::Finish(bool Evidence,const FString& Text)
{
 if(Event.IsEmpty())return;const auto* D=Definition(Event);const bool Threat=D&&D->Family==TEXT("threat");
 if(Evidence&&M&&M->Freeform&&D)
 {
  FString Record=Text.IsEmpty()?D->Finding:Text;
  Record+=FString::Printf(TEXT("\nINCIDENT REFERENCE: %.2f, %.2f, %.2f m."),Anchor.X,Anchor.Y,Anchor.Z);
  if(!PhotoId.IsEmpty())Record+=TEXT("\nRETAINED ORIGINAL PHOTOGRAPH: ")+PhotoId+TEXT(". The photograph remains in this case's album.");
  const FString Key=M->Freeform->HasFinding(Event)?Event+FString::Printf(TEXT(".day%d"),M->Freeform->Day()):Event;
  M->Freeform->RecordEvidence(Key,Record);
 }
 if(M&&M->Night&&M->Night->Glimpses&&(Event==TEXT("observer")||Event==TEXT("feeder_presence")||Event==TEXT("array_presence")||Threat))M->Night->Glimpses->ControlledRelease();
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
 if(Id==TEXT("ff:event-check")&&Event==TEXT("empty_car")&&Near(EmptyCarReference,4))
 {Finish(true);if(M->UI)M->UI->Toast(TEXT("Bonnet warm. Ignition off. The dashboard clock is six minutes behind the retained base reference. No driver in sight."),6);return true;}
 if(Id==TEXT("ff:event-sample")&&Event==TEXT("landing_trace")&&Near(Anchor,2.7))
 {if(!bPhoto){if(M->UI)M->UI->Toast(TEXT("Keep its position as evidence first: photograph the marked surface before lifting the fragment."),4);return true;}M->Freeform->AddItem(TEXT("fragment"),1);Finish(true);return true;}
 return false;
}
bool FS47FreeformDirector::PhysicalObjectPresented()const
{
 if(!bEnabled||!Effects)return false;
 if(Event==TEXT("close_pass"))return M&&M->Web&&M->Web->Area()==Scene&&Effects->Ufo&&Effects->Ufo->Presented();
 if(Event==TEXT("response_lights")||Event==TEXT("flyover")||Event==TEXT("close_pass"))
  return Effects->PartPresented(1)&&Effects->PartPresented(2)&&Effects->PartPresented(3)&&(Event!=TEXT("close_pass")||Effects->PartPresented(0));
 const FS47Obj Object=Event==TEXT("landing_trace")?Effects->Sample:Event==TEXT("empty_car")?Effects->Car:FS47Obj();
 const auto* C=Effects->PresentedComponent(Object);
 return C&&(C->GetCollisionEnabled()==ECollisionEnabled::QueryOnly||C->GetCollisionEnabled()==ECollisionEnabled::QueryAndPhysics)&&C->GetCollisionResponseToChannel(ECC_Visibility)==ECR_Block;
}
namespace
{
 // Sample the central half of the native component's projected bounds. The outer
 // rectangle includes background around the silhouette; it must not decide exposure.
 double SubjectMean(const TArray<FColor>& Pixels,int32 W,int32 H,const FBox2D& Region)
 {
  if(!Region.bIsValid||W<=0||H<=0||Pixels.Num()!=W*H)return -1;
  const FVector2D Margin=(Region.Max-Region.Min)*.25;
  const FVector2D A=Region.Min+Margin,B=Region.Max-Margin;
  if(!FMath::IsFinite(A.X)||!FMath::IsFinite(A.Y)||!FMath::IsFinite(B.X)||!FMath::IsFinite(B.Y))return -1;
  const int32 X0=FMath::Clamp(FMath::CeilToInt(A.X*W),0,W),X1=FMath::Clamp(FMath::FloorToInt(B.X*W),0,W);
  const int32 Y0=FMath::Clamp(FMath::CeilToInt(A.Y*H),0,H),Y1=FMath::Clamp(FMath::FloorToInt(B.Y*H),0,H);
  if(X1-X0<2||Y1-Y0<2)return -1;
  double Sum=0;for(int32 Y=Y0;Y<Y1;++Y)for(int32 X=X0;X<X1;++X){const auto& C=Pixels[Y*W+X];Sum+=.3*C.R+.59*C.G+.11*C.B;}
  return Sum/((X1-X0)*(Y1-Y0));
 }
}
void FS47FreeformDirector::TagPhoto(FS47AlbumShot& Shot)
{
 TaggedShot.Empty();TaggedIncident.Empty();TaggedTitle.Empty();bTaggedLamps=false;TaggedQuality=0;TaggedRegion=FBox2D(ForceInit);PhotoSubjectMean=-1;
 if(!bEnabled||Shot.Id.IsEmpty()||Shot.bUnavailable||!M||!M->FieldCamera||!M->Web)return;
 const FString RoadIncident=M->HighwayUfo?M->HighwayUfo->PhotoIncidentId():FString();
 const bool RoadPhoto=!RoadIncident.IsEmpty();
 if(!RoadPhoto&&(Event.IsEmpty()||IncidentId.IsEmpty()||M->Web->Area()!=Scene))return;
 auto* Camera=M->FieldCamera.Get();
 auto Qualify=[&](const FVector& At,double Height,const UPrimitiveComponent* Target,double Minimum=.025)->double
 {
  const auto P=Camera->Project(At),Top=Camera->Project(At+FVector(0,Height*.5,0)),Bottom=Camera->Project(At-FVector(0,Height*.5,0));
  if(!P.bFront||!Top.bFront||!Bottom.bFront||P.U<.06||P.U>.94||Top.V<.04||Bottom.V>.96)return 0;
  const double Size=FMath::Abs(Bottom.V-Top.V);if(Size<Minimum)return 0;
  FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(FreeformPhotoSight),true);if(M->Player)Params.AddIgnoredActor(M->Player);
  const bool Blocked=RoadPhoto?!M->HighwayUfo->PhotoLineClear(Camera->PhotoLoc,S47Three::ToUE(At),Target):M->Web->GetWorld()->LineTraceSingleByChannel(Hit,Camera->PhotoLoc,S47Three::ToUE(At),ECC_Visibility,Params);
  if(Blocked&&Hit.Component.Get()!=Target)return 0;
  if(Target&&Size>=TaggedQuality)
  {
   FBox2D Bounds(ForceInit);const FBox Box=Target->Bounds.GetBox();bool Front=true;
   for(int32 I=0;I<8;++I)
   {
    const FVector Corner((I&1)?Box.Max.X:Box.Min.X,(I&2)?Box.Max.Y:Box.Min.Y,(I&4)?Box.Max.Z:Box.Min.Z);
    const auto Q=Camera->Project(S47Three::ToThree(Corner));Front&=Q.bFront;Bounds+=FVector2D(Q.U,Q.V);
   }
   TaggedRegion=Front?Bounds:FBox2D(ForceInit);
  }
  return FMath::Clamp(Size,0.,1.);
 };
 if(!RoadPhoto&&(Event==TEXT("observer")||Event==TEXT("service_presence")||Event==TEXT("feeder_presence")||Event==TEXT("array_presence")))
 {
  if(M->Night&&M->Night->Glimpses)for(const auto& I:M->Night->Glimpses->Individuals())if(I.bRendered&&M->Night->Glimpses->ControlledPhotoBody(I.Id))
   TaggedQuality=FMath::Max(TaggedQuality,Qualify(I.Eye-FVector(0,.5,0),1.2,M->Night->Glimpses->ControlledPhotoBody(I.Id)));
 }
 else if(!RoadPhoto&&Event==TEXT("landing_trace"))
 {
  if(const auto* C=Effects->PresentedComponent(Effects->Sample))TaggedQuality=Qualify(Anchor,.35,C);
 }
 else if(RoadPhoto||Event==TEXT("flyover")||Event==TEXT("close_pass")||Event==TEXT("response_lights"))
 {
  if(RoadPhoto||PhysicalObjectPresented())
  {
   double Left=1,Right=0,Top=1,Bottom=0;int32 VisibleLights=0;
   TArray<UStaticMeshComponent*> PhotoParts;
   if(RoadPhoto)PhotoParts=M->HighwayUfo->PhotoParts();
   else if(Event==TEXT("close_pass")&&Effects->Ufo)PhotoParts=Effects->Ufo->PhotoParts();
   else for(int32 I=0;I<4&&I<Effects->Parts.Num();++I)PhotoParts.Add(Effects->Parts[I].Get());
   for(int32 Index=0;Index<PhotoParts.Num();++Index)if(const auto* C=PhotoParts[Index])if(C->IsVisible())
   {
    const FVector At=S47Three::ToThree(C->Bounds.Origin);const double Q=Qualify(At,FMath::Max(.1,double(C->Bounds.BoxExtent.Z)*.02),C,Index==0?.025:.001);
    if(Q<=0)continue;if(Index==0){TaggedQuality=FMath::Max(TaggedQuality,Q);continue;}
    const auto P=Camera->Project(At);Left=FMath::Min(Left,P.U);Right=FMath::Max(Right,P.U);Top=FMath::Min(Top,P.V);Bottom=FMath::Max(Bottom,P.V);++VisibleLights;
   }
   // Three actual lamps must be in the captured frame and individually unobstructed.
   if(VisibleLights==3)TaggedQuality=FMath::Max(TaggedQuality,FMath::Clamp(FVector2D(Right-Left,Bottom-Top).Size(),0.,1.));
  }
 }
 if(TaggedQuality<.025)return;
 TaggedShot=Shot.Id;TaggedIncident=RoadPhoto?RoadIncident:IncidentId;
 TaggedTitle=RoadPhoto?TEXT("highway light"):Event.Replace(TEXT("_"),TEXT(" "));
 bTaggedLamps=RoadPhoto||Event==TEXT("flyover")||Event==TEXT("close_pass")||Event==TEXT("response_lights");
 Shot.Targets.Add({TEXT("freeform:")+TaggedIncident,FVector2D(.5,.5)});
}
void FS47FreeformDirector::Photo(const FS47AlbumShot& Shot)
{
 if(TaggedShot!=Shot.Id||TaggedQuality<.025||Shot.bUnavailable||Shot.Url.IsEmpty()||TaggedIncident.IsEmpty())return;
 // Read the exact retained image, not a later capture or a whole-frame brightness
 // proxy. Emissive airborne lamps retain their existing three-visible-lamp rule.
 const bool Lamps=bTaggedLamps;
 if(!Lamps)
 {
  TArray<FColor> Pixels;int32 W=0,H=0;
  const double Mean=S47Image::DecodePixels(Shot.Url,Pixels,W,H)?SubjectMean(Pixels,W,H,TaggedRegion):-1;
  PhotoSubjectMean=Mean;
  if(Mean<15){TaggedShot.Empty();if(M&&M->UI)M->UI->Toast(TEXT("The photographed subject is too dark or too small to register. Retain it as a lead; seek a clearer, better-lit view."),4);return;}
 }

 TaggedShot.Empty();LastPhotoAt=ActiveTime;
 if(TaggedIncident==IncidentId&&!Event.IsEmpty())
 {
  bSeen=bPhoto=true;PhotoId=Shot.Id;FVector ObservedAt=Anchor;
  if(Event==TEXT("observer")&&M&&M->Night&&M->Night->Glimpses)
   for(const auto& I:M->Night->Glimpses->Individuals())if(I.bRendered){ObservedAt=I.Eye;break;}
  ObserveVisitor(ObservedAt);
 }
 auto* Existing=Claims.FindByPredicate([&](const FPhotoClaim& C){return C.Id==TaggedIncident;});
 if(!Existing){FPhotoClaim C;C.Id=TaggedIncident;C.PhotoId=Shot.Id;C.Title=TaggedTitle;C.Quality=TaggedQuality;Claims.Add(C);}
 else if(!Existing->bPaid&&TaggedQuality>Existing->Quality){Existing->PhotoId=Shot.Id;Existing->Quality=TaggedQuality;}
 Changed();if(M&&M->UI)M->UI->Toast(Existing&&Existing->bPaid?TEXT("This incident is already registered. The new photograph remains in the album; no additional credit is due."):TEXT("Field photograph retained. Register it at OPS for $20 documentation credit, once per incident. Origin remains unconfirmed."),5);
}
bool FS47FreeformDirector::RegisterPhotoClaim(const FString& Id)
{
 if(!M||!M->Freeform||!M->Freeform->CanRegisterPhotoEvidence()||!M->Night||!M->Night->Album)return false;
 auto* C=Claims.FindByPredicate([&](const FPhotoClaim& P){return P.Id==Id;});if(!C||C->bPaid)return false;
 const auto* Shot=M->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& P){return P.Id==C->PhotoId;});
 if(!Shot||Shot->bUnavailable||Shot->Url.IsEmpty()||!Shot->Targets.ContainsByPredicate([&](const TPair<FString,FVector2D>& P){return P.Key==TEXT("freeform:")+C->Id;}))return false;
 if (!Shot->IsDeveloped())
 {
  if (M->UI) M->UI->Toast(TEXT("PHOTO LAB: Develop the photograph before submitting it"), 5);
  return false;
 }
 C->bPaid=true;M->Freeform->Earn(C->Reward);
 M->Freeform->RecordEvidence(TEXT("photo-registration.")+C->Id,FString::Printf(TEXT("OPS FIELD PHOTOGRAPH RECEIPT\nIncident: %s\nRetained original: %s\nDocumentation credit: $%d (applied to outstanding supply credit first).\nThe captured subject met framing and visibility checks. Registration documents the observation; its origin and intent remain unconfirmed. This incident cannot be paid twice."),*C->Title,*C->PhotoId,C->Reward));Changed();
 if(M->UI)M->UI->Toast(TEXT("OPS registered the field photograph: $20 documentation credit. This records an observation, not a confirmed origin."),5);
 return true;
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
 {
  constexpr int32 W=100,H=100;const FBox2D Region(FVector2D(.4,.4),FVector2D(.6,.6));
  TArray<FColor> P;P.Init(FColor(3,3,3),W*H);
  for(int32 Y=40;Y<60;++Y)for(int32 X=40;X<60;++X)P[Y*W+X]=FColor(100,100,100);
  Check(SubjectMean(P,W,H,Region)>=15,TEXT("illuminated subject qualifies against dark night background"));
  P.Init(FColor(180,180,180),W*H);for(int32 Y=40;Y<60;++Y)for(int32 X=40;X<60;++X)P[Y*W+X]=FColor(3,3,3);
  Check(SubjectMean(P,W,H,Region)<15,TEXT("bright surroundings do not qualify an unlit subject"));
  P.Init(FColor(180,180,180),W*H);for(int32 Y=45;Y<55;++Y)for(int32 X=45;X<55;++X)P[Y*W+X]=FColor(3,3,3);
  Check(SubjectMean(P,W,H,Region)<15,TEXT("bright margins inside component bounds cannot mask a dark subject centre"));
  Check(SubjectMean(P,W,H,FBox2D(ForceInit))<0&&SubjectMean(P,W,H,FBox2D(FVector2D(.5,.5),FVector2D(.501,.501)))<0,TEXT("missing or subpixel subject region cannot qualify"));
 }
 Check(Catalog().Num()==19&&Definition(TEXT("roomlight_return")),TEXT("authored catalogue, action-bound room light and two physical service-site encounters are embedded in runtime"));TSet<FString> Ids;
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
 auto Light=S47J::Clone(Saved);auto LightState=S47J::New();
 S47J::Set(LightState,TEXT("room"),TEXT("control"));S47J::Set(LightState,TEXT("serial"),7);S47J::Set(LightState,TEXT("age"),84.5);S47J::Set(LightState,TEXT("offClock"),40000.);S47J::Set(LightState,TEXT("onClock"),42000.);S47J::Set(LightState,TEXT("left"),true);S47J::Set(LightState,TEXT("applied"),true);S47J::Set(LightState,TEXT("inspected"),false);S47J::Set(Light,TEXT("reactiveLight"),LightState);S47J::Set(Light,TEXT("event"),TEXT("roomlight_return"));S47J::Set(Light,TEXT("effectsApplied"),true);
 R.Begin(S47J::Read(S47J::Write(Light)));
 Check(R.ReactiveRoom==TEXT("control")&&R.ReactiveSerial==7&&R.ReactiveAge==84.5&&R.bReactiveLeft&&R.bReactiveApplied&&!R.bReactiveInspected&&R.bEffectsApplied,TEXT("Reactive light candidate, actual input identity, departure, age and applied-once effect survive JSON Continue"));
 auto RestoredLight=S47J::Read(S47J::Write(R.Save()));FS47FreeformDirector LightCopy(nullptr);LightCopy.Begin(RestoredLight);
 Check(LightCopy.ReactiveOffClock==40000&&LightCopy.ReactiveOnClock==42000&&LightCopy.bReactiveApplied&&!LightCopy.ReactiveReady(),TEXT("Applied light restore retains original clocks and cannot rearm or fabricate a live player action"));
 LightCopy.RoomLightInput(TEXT("records"),8,true);Check(LightCopy.ReactiveSerial==7,TEXT("A missing Freeform world cannot manufacture a manual OFF candidate"));
 R.Begin(Saved);Check(R.ReactiveRoom.IsEmpty()&&!R.bReactiveApplied,TEXT("Legacy Director save does not infer an incident from a room preference"));
 FS47FreeformDirector Schedule(nullptr);Schedule.Begin(nullptr);
 Check(!Schedule.RollNightChoice(1,2,1.f,Schedule.ThreatRollDay,Schedule.PendingThreatNight)&&Schedule.ThreatRollDay==0,TEXT("The first night cannot consume a future threat roll"));
 Schedule.RollNightChoice(2,2,1.f,Schedule.ThreatRollDay,Schedule.PendingThreatNight);const int32 ChosenSeed=Schedule.Random.GetCurrentSeed();
 Check(Schedule.PendingThreatNight==2&&!Schedule.StartPendingNightChoice(TEXT("service_presence"),2,Schedule.PendingThreatNight)&&Schedule.PendingThreatNight==2,TEXT("A chosen threat waits when a live eligible location is unavailable instead of losing its outcome"));
 FS47FreeformDirector PendingCopy(nullptr);PendingCopy.Begin(S47J::Read(S47J::Write(Schedule.Save())));
 Check(PendingCopy.PendingThreatNight==2&&!PendingCopy.RollNightChoice(2,2,0.f,PendingCopy.ThreatRollDay,PendingCopy.PendingThreatNight)&&PendingCopy.Random.GetCurrentSeed()==ChosenSeed&&PendingCopy.PendingThreatNight==2,TEXT("Continue and repeated scheduler attempts preserve the selected night outcome without another random draw"));
 Check(PendingCopy.ExpireNightChoices(2,false)&&PendingCopy.PendingThreatNight==0&&!PendingCopy.RollNightChoice(2,2,1.f,PendingCopy.ThreatRollDay,PendingCopy.PendingThreatNight)&&PendingCopy.Random.GetCurrentSeed()==ChosenSeed,TEXT("Sunrise discards an unstarted threat without rerolling the spent night"));
 Schedule.RollNightChoice(3,2,0.f,Schedule.ThreatRollDay,Schedule.PendingThreatNight);const int32 QuietSeed=Schedule.Random.GetCurrentSeed();
 Check(Schedule.PendingThreatNight==0&&!Schedule.RollNightChoice(3,2,1.f,Schedule.ThreatRollDay,Schedule.PendingThreatNight)&&Schedule.Random.GetCurrentSeed()==QuietSeed,TEXT("A quiet-night result stays quiet across retries rather than becoming a guaranteed encounter"));
 Schedule.RollNightChoice(4,4,1.f,Schedule.MajorRollDay,Schedule.PendingMajorNight);
 PendingCopy.Begin(S47J::Read(S47J::Write(Schedule.Save())));
 Check(PendingCopy.PendingMajorNight==4&&PendingCopy.ExpireNightChoices(5,true)&&PendingCopy.PendingMajorNight==0,TEXT("Major-event choices serialize independently and never accumulate across later nights"));
 auto OldSchedule=Schedule.Save();OldSchedule->RemoveField(TEXT("pendingThreatNight"));OldSchedule->RemoveField(TEXT("pendingMajorNight"));
 PendingCopy.Begin(OldSchedule);const int32 LegacySeed=PendingCopy.Random.GetCurrentSeed();
 Check(PendingCopy.PendingThreatNight==0&&PendingCopy.PendingMajorNight==0&&!PendingCopy.RollNightChoice(4,4,1.f,PendingCopy.MajorRollDay,PendingCopy.PendingMajorNight)&&PendingCopy.Random.GetCurrentSeed()==LegacySeed,TEXT("A spent legacy roll is retained without inventing or rerolling its unknown outcome"));
 FS47FreeformDirector Photos(nullptr);Photos.Begin(nullptr);Photos.IncidentSerial=7;Photos.IncidentId=TEXT("observer.day3.7");
 FPhotoClaim Claim;Claim.Id=Photos.IncidentId;Claim.PhotoId=TEXT("album7");Claim.Title=TEXT("observer");Claim.Quality=.18;Photos.Claims.Add(Claim);
 FS47FreeformDirector PhotoCopy(nullptr);PhotoCopy.Begin(S47J::Read(S47J::Write(Photos.Save())));
 Check(PhotoCopy.Claims.Num()==1&&!PhotoCopy.Claims[0].bPaid&&PhotoCopy.Claims[0].Reward==20&&PhotoCopy.IncidentSerial==7,TEXT("Unregistered photo evidence retains identity, quality and fixed credit after incident and Continue"));
 PhotoCopy.Claims[0].bPaid=true;Photos.Begin(PhotoCopy.Save());
 Check(Photos.Claims.Num()==1&&Photos.Claims[0].bPaid&&!Photos.RegisterPhotoClaim(Claim.Id),TEXT("Paid photographic incident persists and cannot be registered without physical powered OPS"));
 auto Duplicate=PhotoCopy.Save();auto Entries=S47J::Arr(Duplicate,TEXT("photoClaims"));const auto DuplicateEntry=Entries[0];Entries.Add(DuplicateEntry);Duplicate->SetArrayField(TEXT("photoClaims"),Entries);Photos.Begin(Duplicate);
 Check(Photos.Claims.Num()==1&&Photos.Claims[0].bPaid,TEXT("Duplicate serialized photo incident cannot create a second claim"));
 auto LegacyPhoto=S47J::Clone(Active);LegacyPhoto->RemoveField(TEXT("incidentId"));LegacyPhoto->RemoveField(TEXT("photoClaims"));Photos.Begin(LegacyPhoto);const FString LegacyIncident=Photos.IncidentId;Photos.Begin(Photos.Save());
 Check(Photos.Claims.IsEmpty()&&!LegacyIncident.IsEmpty()&&Photos.IncidentId==LegacyIncident,TEXT("Legacy active incident gets stable identity without inventing paid photographs"));
 FS47FreeformDirector Facts(nullptr);Facts.Begin(nullptr);
 FS47FreeformObservation Fact;Fact.Id=TEXT("roof_steps.day2.3");Fact.Event=TEXT("roof_steps");Fact.Place=TEXT("SARO roof / service yard");Fact.Bearing=TEXT("NW");Fact.Day=2;Fact.Clock=160000;Fact.bHeard=true;
 Facts.RetainObservation(Fact);Fact.Day=4;Fact.Clock=300000;Fact.PhotoId=TEXT("photo-original");Facts.RetainObservation(Fact);Facts.RetainObservation(Fact);
 FS47FreeformDirector FactsCopy(nullptr);FactsCopy.Begin(S47J::Read(S47J::Write(Facts.Save())));
 Check(FactsCopy.Observations().Num()==1&&FactsCopy.Observations()[0].Day==2&&FactsCopy.Observations()[0].Clock==160000&&FactsCopy.Observations()[0].PhotoId==TEXT("photo-original")&&FactsCopy.Observations()[0].bHeard,TEXT("Observed facts retain first date, bearing and heard provenance across duplicate/photo updates and Continue"));
 Fact.Id=TEXT("invalid");Fact.Clock=-1;Facts.RetainObservation(Fact);
 Check(Facts.Observations().Num()==1,TEXT("Invalid observation time cannot create a case lead"));
 FactsCopy.Begin(Saved);Check(FactsCopy.Observations().IsEmpty(),TEXT("Legacy saves invent no observations from old event flags"));
 Check(FS47FreeformObservation::Direction(FVector::ZeroVector,FVector(0,0,-10))==TEXT("N")&&FS47FreeformObservation::Direction(FVector::ZeroVector,FVector(10,0,0))==TEXT("E")&&FS47FreeformObservation::Direction(FVector::ZeroVector,FVector(-10,0,-10))==TEXT("NW"),TEXT("Coarse bearings follow world north and are observer-relative"));
 FS47RoswellBrain Brain;Brain.Start(FS47RoswellBrain::EMode::Curious,FVector(2,0,3),.4,77);Brain.bHasPlayerMemory=true;Brain.LastKnownPlayer=FVector(8,0,9);auto B=Brain.Save();FS47RoswellBrain Copy;
 Check(Copy.Restore(B)&&Copy.Pos==Brain.Pos&&Copy.LastKnownPlayer==Brain.LastKnownPlayer&&Copy.bHasPlayerMemory,TEXT("active native observer keeps identity-space position and measured player memory"));
 return Ok;
}

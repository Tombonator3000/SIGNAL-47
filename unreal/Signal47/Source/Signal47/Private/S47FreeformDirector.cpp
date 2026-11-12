#include "S47FreeformDirector.h"
#include "S47DirectorCam.h"
#include <limits>
#include "S47SmallHints.h"
#include "S47Rhythm.h"
#include "S47Taken.h"
#include "S47FieldRadio.h"
#include "S47FarmSite.h"
#include "S47EventPlaces.h"
#include "S47OpenLand.h"
#include "S47Visitors.h"
#include "S47NightRhythm.h"
#include "S47IndoorDread.h"
#include "S47Inter.h"
#include "S47UfoPresentation.h"
#include "S47HighwayUfo.h"
#include "S47Freeform.h"
#include "S47Happenings.h"
#include "S47EventScript.h"
#include "S47WeekCase.h"
#include "S47StoryDirector.h"
#include "S47CaseFieldTests.h"
#include "S47RoomLights.h"
#include "Misc/CommandLine.h"
#include "S47FreeformWorld.h"
#include "S47Main.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Night.h"
#include "S47SkyVisitors.h"
#include "S47Glimpses.h"
#include "S47Aftermath.h"
#include "S47Audio.h"
#include "S47UI.h"
#include "S47Inter.h"
#include "S47Walker.h"
#include "S47Story.h"
#include "S47Album.h"
#include "S47Witness.h"
#include "S47FieldCamera.h"
#include "S47Image.h"
#include "S47Three.h"
#include "S47Docs.h"
#include "S47Flow.h"
#include "S47SaroSave.h"
#include "S47ScriptPlayer.h"
#include "S47Fx.h"
#include "NiagaraComponent.h"
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
 const FS47FreeformDirector::FPhotoClaim* SelectPhotoReturnPlan(
  const TArray<FS47FreeformDirector::FPhotoClaim>& Claims,const FString& Focus,
  TFunctionRef<bool(const FS47FreeformDirector::FPhotoClaim&)> Available)
 {
  const FS47FreeformDirector::FPhotoClaim* First=nullptr;
  for(const auto& C:Claims)
  {
   if(C.ControlPlannedAt<0||C.ControlOriginalId.IsEmpty()||!C.ControlPhotoId.IsEmpty()||!Available(C))continue;
   if(C.Id==Focus)return &C;
   if(!First)First=&C;
  }
  return First;
 }

 // The previous staff bay stays clear after the sedan moves west to x24.6.
 // Retain this existing event anchor, outside the new east-wing garage approach.
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
   Out.Add({TEXT("feeder_presence"),TEXT("investigation"),TEXT("photo"),TEXT("A figure on film out by the remote feeder on the county road."),2,true,85});
   Out.Add({TEXT("array_presence"),TEXT("investigation"),TEXT("photo"),TEXT("A figure on film by the array service cabinet. The cabinet and the service log are worth a look."),3,true,85});
   return Out;
  }();return Data;
 }
 FString LightStamp(double Clock)
 {
  const int32 Day=FMath::Max(1,int32(FMath::FloorToDouble((Clock-28800.)/86400.))+1);
  const int32 Seconds=int32(FMath::Fmod(FMath::Max(0.,Clock),86400.));
  return FString::Printf(TEXT("DAY %d / %02d:%02d:%02d"),Day,Seconds/3600,(Seconds/60)%60,Seconds%60);
 }

 FString LocalNoiseEquipmentHint(const FS47FieldLogger& L,const FS47FreeformSession::FJob* J,const FS47Profile* P)
 {
  FString Text=TEXT("\n")+L.NextAction();
  const bool UnhandledTape=!L.Tapes.IsEmpty()&&!L.Tapes.Last().bCompared;
  if(L.Site<0&&!L.bRecording&&!UnhandledTape&&L.Tapes.Num()<256)
   Text+=TEXT("\nFor your next new check, try SITE A on the generator service path. Placement starts two hours; sleeping does not supply observations.");
  if(L.bRecording&&!L.Tapes.IsEmpty())
  {
   const auto& T=L.Tapes.Last();Text+=FString::Printf(TEXT("\nEXISTING RECORDING / %.3f MHz. Elapsed interval: %s to %s. Sampled %.0f s; missed %.0f s. Planned finish: %s. This recording is separate from the saved idea; overlap still needs actual SARO acquisition."),T.Frequency,*LightStamp(T.Start),*LightStamp(T.End),T.Seconds,T.GapSeconds,*LightStamp(T.Start+FS47FieldLogger::Window));
  }
  using E=FS47FreeformSession::EJobStage;
  if(J&&P&&!J->bLegacyTrace&&!J->bDuplicateRejected)
  {
   if(J->Stage==E::Chart)return Text+TEXT("\nSARO / Collect the finished recording at the printer. A finished acquisition cannot add new overlap.");
   if(J->Stage==E::New||J->Stage==E::Pointed||J->Stage==E::Integrating)
   {
    Text+=FString::Printf(TEXT("\nSELECTED SARO CARD / %s / %.3f MHz."),*J->Id,P->F);
    if(L.bRecording)
    {
     if(FMath::Abs(L.Frequency-P->F)>.001)return Text+TEXT(" Frequencies differ. Select a matching SARO card if available, or prepare another logger run later; do not retune a deployed logger.");
     return Text+(J->Stage==E::Integrating?TEXT(" Frequencies match. SARO acquisition is running; let both recordings finish, then retrieve the logger."):TEXT(" Frequencies match. Point and tune SARO, then start its recording while the logger runs."));
    }
    if(J->Stage==E::Integrating)return Text+TEXT(" SARO acquisition is already running. Finish this observation; prepare a separate overlapping run when the logger is ready.");
    if(UnhandledTape||L.Site>=0||L.Tapes.Num()>=256)return Text+TEXT(" Handle the existing logger and original as directed above before preparing another run.");
    return Text+TEXT(" At CALIBRATION / REPAIR, tune the returned logger to this card. Place it before starting the SARO recording.");
   }
  }
  return Text+TEXT("\nOPS / DAILY WORK: select an unfinished observation. When none remain, the optional reference check provides a new SARO original.");
 }
 // M7 (B2): what is out there is heard where the player is, not told. A line goes with the sound only within reach and
 // with nothing solid between (a shut door is solid); some only to a player who is outside. Anything else gets the sound alone.
 bool SaysAloud(double Distance,double Range,bool bClear,bool bIndoors,bool bOutsideOnly)
 {return FMath::IsFinite(Distance)&&Distance<=Range&&bClear&&!(bOutsideOnly&&bIndoors);}
 bool PlayerIndoors(const US47Main* M){return M&&M->Freeform&&M->Freeform->Taken&&M->Freeform->Taken->Indoors();}
 // Line of hearing from the player's eye to the place: open, or the first thing in the way is the place itself (the fence it is at).
 bool ClearTo(const US47Main* M,const FVector& At)
 {
  if(!M||!M->Night||!M->Web||!M->Web->GetWorld())return false;
  const FVector Eye=M->Night->GlimpseIn(true).Cam.Pos;
  FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(FreeformEventHearing),true);if(M->Player)Params.AddIgnoredActor(M->Player);
  if(!M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(Eye),S47Three::ToUE(At),ECC_Visibility,Params))return true;
  return FVector::Dist(S47Three::ToThree(Hit.ImpactPoint),At)<2.5;
 }
 bool HeardHere(const US47Main* M,const FVector& At,double Range,bool bOutsideOnly)
 {
  if(!M||!M->Player)return false;
  const double Distance=FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(At.X,At.Z));
  return Distance<=Range&&SaysAloud(Distance,Range,ClearTo(M,At),PlayerIndoors(M),bOutsideOnly);
 }
 // Who rings the lights in, and how it is put, by the week: the same words every night for a month were a template (M7).
 FString LightsTip(int32 Week,const FString& Caller,const FString& Place)
 {
  switch(FMath::Clamp(Week,0,3))
  {
   case 0: return FString::Printf(TEXT("%s reports lights low over %s."),*Caller,*Place);
   case 1: return FString::Printf(TEXT("%s says there are lights over %s again, low and slow, for a few minutes now."),*Caller,*Place);
   case 2: return FString::Printf(TEXT("%s calls about lights over %s, and says somebody else on the road saw them too."),*Caller,*Place);
   default: return FString::Printf(TEXT("%s is on the line about %s. Lights, low, and closer to the road than before."),*Caller,*Place);
  }
 }
 FString HourText(double Clock){const int32 S=int32(FMath::Fmod(FMath::Max(0.,Clock),86400.));return FString::Printf(TEXT("%02d:%02d"),S/3600,(S/60)%60);}
 // The antenna a visit was over: the one in S-03's place in their flight, wherever on the array the flight was put (OrbSite).
 bool OrbDish(const US47Main* M,const FVector& Anchor,FString& OutDish,FVector& OutAt)
 {
  if(!M||!M->Web||M->Web->DishCount()<1)return false;
  const auto& S03=FS47DuskOrb::Stops()[0];
  const FVector2D Want(Anchor.X+S03.X-FS47DuskOrb::HomeX,Anchor.Z+S03.Z-FS47DuskOrb::HomeZ);
  int32 Best=INDEX_NONE;double Near=12;
  for(int32 I=0;I<M->Web->DishCount();++I){const double D=FVector2D::Distance(FVector2D(M->Web->DishX(I),M->Web->DishZ(I)),Want);if(D<Near){Near=D;Best=I;}}
  if(Best==INDEX_NONE)return false;
  const double X=M->Web->DishX(Best),Z=M->Web->DishZ(Best);
  OutDish=M->Web->DishId(Best);OutAt=FVector(X,US47Night::SaroFloorAt(M->Web,X,Z)+1.,Z);
  return !OutDish.IsEmpty();
 }
 // Found by going there: a sound at the thing, what is seen there, and a record another system reads. The orb's goes with the
 // night's records as a burst on the array (FS47Happenings::IsNightFinding, the visitors' case, the night log's instruments);
 // the fence and the gate end in _visit, the visitors' case's own (FS47FreeformCases::LineForFinding).
 void FoundLeft(US47Main* M,const FS47DuskOrb::FLeft& L)
 {
  if(!M||!M->Freeform)return;
  const int32 Day=M->Freeform->Day();
  if(M->Audio)M->Audio->Sfx(L.Kind==TEXT("orb")?TEXT("click"):TEXT("metalTap"),FS47Sound{.Gain=.45f,.At=L.At});
  FString Seen,Key,Record;
  if(L.Kind==TEXT("orb"))
  {
   Seen=FString::Printf(TEXT("The strip recorder in the junction cabinet at %s has a step in it."),*L.Dish);
   Key=FString::Printf(TEXT("array_burst_d%d_%s"),Day,*L.Dish);
   Record=FString::Printf(TEXT("At %s the strip recorder in the junction cabinet at %s stepped up off its noise floor and stayed up until %s. The antennas either side of it stayed flat. Nothing at the cabinet was switched."),*HourText(L.Began),*L.Dish,*HourText(L.Clock));
  }
  else if(L.Kind==TEXT("fence"))
  {
   Seen=TEXT("Two posts of the yard fence lean outward here. The wire between them is rubbed bright.");
   Key=FString::Printf(TEXT("array_fence_d%d_visit"),Day);
   Record=TEXT("Two posts of the yard fence toward the array lean outward, and the wire between them is rubbed bright at about chest height. The ground under it is soft and shows no prints.");
  }
  else
  {
   Seen=TEXT("The lower rail of the service gate is bowed inward.");
   Key=FString::Printf(TEXT("service_gate_d%d_visit"),Day);
   Record=TEXT("The lower rail of the service gate is bowed inward a hand's width, at about knee height. It was straight on the last round. Scrape marks on the concrete inside the gate.");
  }
  if(M->UI)M->UI->Toast(Seen,6);
  M->Freeform->RecordEvidence(Key,Record);
  UE_LOG(LogS47FreeformDirector,Display,TEXT("S47: found what the night left: %s %s (%s)"),*L.Kind,*L.Dish,*Key);
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
 TArray<TWeakObjectPtr<UStaticMeshComponent>> FarmExtra;
 TArray<FVector> LampScale;
 TArray<TWeakObjectPtr<UPointLightComponent>> Lights;
 // The fragment has one for each ground it can lie on: SARO's, and the road's for the rest of the land (Tom, 8 November).
 FS47Obj Sample,SampleRoad,Car;
 TWeakObjectPtr<UStaticMeshComponent> Orb;
 // The mesh stays as the physical record for sight and photographs; these are what is seen.
 TWeakObjectPtr<UNiagaraComponent> OrbFx[FS47DuskOrb::Count];
 // Niagara does not reach the film's scene capture: the plain spheres draw for that one frame.
 TWeakObjectPtr<UStaticMeshComponent> OrbFilm[FS47DuskOrb::Count];
 TWeakObjectPtr<UPointLightComponent> OrbLight[FS47DuskOrb::Count];
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
  for(int32 I=0;I<5;++I)if(auto* C=Mesh(FString::Printf(TEXT("FreeformFarmLight%d"),I),TEXT("/Engine/BasicShapes/Sphere.Sphere"),FVector(84,20,492),FVector(3.5),0xffe9df))FarmExtra.Add(C);
  Orb=Mesh(TEXT("FreeformDuskOrb"),TEXT("/Engine/BasicShapes/Sphere.Sphere"),FVector(-3.25,2,-47),FVector(.85),0xffe7bd);
  if(Orb.IsValid())if(auto* Mat=Cast<UMaterialInstanceDynamic>(Orb->GetMaterial(0))){Mat->SetVectorParameterValue(TEXT("Emissive"),S47Three::Hex(0xffe7bd));Mat->SetScalarParameterValue(TEXT("EmissivePower"),4.f);}
  for(int32 I=0;I<FS47DuskOrb::Count;++I)
  {
   auto* Root=M->Web->GetRootComponent();
   auto* Fx=S47Fx::Make(M->Web.Get(),Root,TEXT("/Game/Signal47/FX/NS_S47_Orb.NS_S47_Orb"),*FString::Printf(TEXT("FreeformOrbFx%d"),I));
   if(!Fx)continue;
   OrbFx[I]=Fx;Fx->SetWorldRotation(FRotator::ZeroRotator);Fx->SetWorldScale3D(FVector::OneVector);
   auto* L=NewObject<UPointLightComponent>(M->Web.Get(),*FString::Printf(TEXT("FreeformOrbLight%d"),I));L->SetupAttachment(Root);L->SetMobility(EComponentMobility::Movable);
   L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(0);L->SetAttenuationRadius(1700);L->SetSourceRadius(30);L->SetCastShadows(false);L->SetLightColor(S47Three::Hex(0xffc98a));L->SetVisibility(false);L->RegisterComponent();
   OrbLight[I]=L;
   // With a real effect the sphere no longer draws, but it still answers sight and photo queries.
   auto* Film=I==0?Orb.Get():Mesh(FString::Printf(TEXT("FreeformDuskOrbFilm%d"),I),TEXT("/Engine/BasicShapes/Sphere.Sphere"),FVector(-3.25,2,-47),FVector(.6),0xffe7bd);
   if(Film)
   {
    if(auto* Mat=Cast<UMaterialInstanceDynamic>(Film->GetMaterial(0))){Mat->SetVectorParameterValue(TEXT("Emissive"),S47Three::Hex(0xffe7bd));Mat->SetScalarParameterValue(TEXT("EmissivePower"),14.f);}
    Film->SetRenderInMainPass(false);Film->SetRenderInDepthPass(false);Film->SetCastShadow(false);Film->SetCollisionEnabled(ECollisionEnabled::NoCollision);OrbFilm[I]=Film;
   }
  }
  if(M->FieldCamera)
  {
   const TWeakPtr<uint8> Alive=Life;
   M->FieldCamera->OnExposure=[this,Alive](bool bOn){if(Alive.IsValid())for(const auto& W:OrbFilm)if(auto* C=W.Get())C->SetRenderInMainPass(bOn);};
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
  // The same fragment on the road's ground, for marked ground anywhere else on the land. One interactable: its ray finds whichever is shown.
  if(auto* RoadTrace=Mesh(TEXT("FreeformTraceRoad"),TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),FVector(12,.025,-12),FVector(.34,.035,.34),0x9f9c7b))
  {
   const int32 RoadRoot=M->Web->Handle(TEXT("world.legs.road.area.group"));
   if(RoadRoot>=0)
   {
    SampleRoad=M->Web->ObjAt(M->Web->AdoptNode(RoadTrace,RoadRoot,TEXT("FreeformTraceRoad")));
    SampleRoad.SetVisible(false);RoadTrace->SetVisibility(false,false);RoadTrace->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->Web->MarkUsable(SampleRoad.I,TEXT("ff:event-sample"));
   }
  }
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
  for(int32 I=0;I<FS47DuskOrb::Count;++I){if(auto* Fx=OrbFx[I].Get())Fx->DeactivateImmediate();if(auto* L=OrbLight[I].Get())L->SetVisibility(false);}
  for(const FS47Obj O:{Sample,SampleRoad,Car})if(O.W==M->Web.Get()&&O)M->Web->SetVisible(O.I,false);
  for(auto& P:Parts)if(P.IsValid()){P->SetVisibility(false,false);P->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
  for(auto& L:Lights)if(L.IsValid())L->SetIntensity(0);
 }
 // Shown only where its own ground is: the road's fragment in town and on the land, the rest at SARO.
 bool IsRoadSample(FS47Obj O)const{return SampleRoad&&O.W==SampleRoad.W&&O.I==SampleRoad.I;}
 bool OwnArea(FS47Obj O)const
 {
  const FString Here=M->Web->Area();
  return IsRoadSample(O)?Here==TEXT("road")||Here==TEXT("diner"):Here==TEXT("saro");
 }
 FS47Obj SampleFor(const FString& Area)const{return (Area==TEXT("road")||Area==TEXT("diner"))&&SampleRoad?SampleRoad:Sample;}
 // Both fragments where it lies, on whatever is under it there.
 void PlaceSample(const FVector& At)
 {
  if(!CanTouchWorld())return;
  FVector Foot=At;FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(FreeformTraceGround),true);if(M->Player)Query.AddIgnoredActor(M->Player);
  if(M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(At+FVector(0,20,0)),S47Three::ToUE(At-FVector(0,20,0)),ECC_Visibility,Query))Foot.Y=S47Three::ToThree(Hit.ImpactPoint).Y;
  for(const FS47Obj O:{Sample,SampleRoad})if(O.W==M->Web.Get()&&O)if(const auto* N=M->Web->Node(O.I))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))C->SetWorldLocation(S47Three::ToUE(Foot+FVector(0,.025,0)));
 }
 void Physical(FS47Obj O,bool bVisible)
 {
  if(!CanTouchWorld()||O.W!=M->Web.Get()||!O)return;
  // Unconditional node update repairs stale presentation too. A hidden scene
  // or ancestor still wins; component visibility never bypasses its parent.
  M->Web->SetVisible(O.I,bVisible);
  const bool Presented=bVisible&&O.Shown()&&OwnArea(O);
  if(const auto* N=M->Web->Node(O.I))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))
  {C->SetVisibility(Presented,false);C->SetCollisionEnabled(Presented?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);}
 }
 bool PartPresented(int32 Index)const
 {
  const FString Here=CanTouchWorld()?M->Web->Area():FString();
  if((Here!=TEXT("saro")&&Here!=TEXT("road")&&Here!=TEXT("diner"))||!Parts.IsValidIndex(Index))return false;
  const auto* C=Parts[Index].Get();
  return IsValid(C)&&C->IsRegistered()&&C->IsVisible()&&!C->bHiddenInGame&&C->GetOwner()&&!C->GetOwner()->IsHidden();
 }
 UStaticMeshComponent* PresentedComponent(FS47Obj O)const
 {
  if(!CanTouchWorld()||O.W!=M->Web.Get()||!O||!O.Shown()||!OwnArea(O))return nullptr;
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
 Stop();DuskOrb.Begin(S47J::Obj(O,TEXT("duskOrb")));bCarLight=bCarTaken=false;CarStage=0;CarLightClock=-1;bForcedEvent=bOwedArray=bOwedFeeder=false;SkyLeft=0;ReactiveRoom.Empty();ReactiveSerial=0;ReactiveAge=ReactiveOffClock=ReactiveOnClock=0;bReactiveLeft=bReactiveApplied=bReactiveInspected=false;bEnabled=true;bEffectsApplied=false;bEveningTold=false;Event.Empty();PhotoId.Empty();PlaceId.Empty();NextPlace.Empty();bTracePlaced=false;Scene=TEXT("saro");DangerNight=DangerCount=SlotNight=PeekNight=0;PointNight=PointCount=0;Stage=Variant=SeenDay=MinorLeft=MinorBudget=InvestLeft=ThreatRollDay=MajorRollDay=LastThreatDay=0;LastMajorDay=-3;
 MysteryRollDay=PendingMysteryNight=MysterySeen=0;MysteryClue=-1;MysteryAfter=MysteryClearAt=MysteryAge=0;
 PendingThreatNight=PendingMajorNight=LightRollDay=PendingLightNight=0;LightAfter=0;ObserverRollDay=PendingObserverNight=SiteRollDay=PendingSiteNight=0;SiteCheckAt=0;IncidentSerial=0;IncidentId.Empty();TaggedShot.Empty();TaggedQuality=0;Claims.Reset();PhotoControlFocus.Empty();FieldObservations.Reset();
 Age=ActiveTime=0;NextAt=90;RestUntil=SoundAt=0;bWasFirstShift=false;LastPhotoAt=-10;LastDays.Reset();bSeen=bPhoto=bControlled=bCompared=false;Anchor=FVector::ZeroVector;Random.Initialize(M && M->Flow && !M->Flow->bDevRun ? int32(FDateTime::UtcNow().GetTicks() ^ FPlatformTime::Cycles64()) : 4701986);
 if(O)
 {
  if(S47J::Num(O,TEXT("v"))!=1){bEnabled=false;return;}
  Event=S47J::Str(O,TEXT("event"));if(!Event.IsEmpty()&&!Definition(Event)){bEnabled=false;return;}
  // A light under way does not come back with a Continue: the car does, and whatever was seen stands.
  bCarLight=Event==TEXT("empty_car")&&S47J::Bool(O,TEXT("carLight"));CarLightClock=bCarLight?S47J::Num(O,TEXT("carLightClock"),-1):-1;CarStage=CarLightClock>=0?2:0;
  IncidentSerial=FMath::Clamp(S47J::Int(O,TEXT("incidentSerial")),0,1000000000);
  IncidentId=S47J::Str(O,TEXT("incidentId"));
  PhotoControlFocus=S47J::Str(O,TEXT("photoControlFocus")).Left(200);
  for(const auto& V:S47J::Arr(O,TEXT("photoClaims")))
  {
   const S47J::FObj* C=nullptr;if(!V.IsValid()||!V->TryGetObject(C)||!C)continue;
   FPhotoClaim P;P.Id=S47J::Str(*C,TEXT("id"));P.PhotoId=S47J::Str(*C,TEXT("photo"));P.Title=S47J::Str(*C,TEXT("title"));P.bPaid=S47J::Bool(*C,TEXT("paid"));
   P.Quality=S47J::Num(*C,TEXT("quality"));P.Reward=PhotoReward(P.Id,P.Quality);
   P.LocalNoiseOriginalId=S47J::Str(*C,TEXT("localNoiseOriginal")).Left(128);P.LocalNoisePlannedAt=S47J::Num(*C,TEXT("localNoisePlannedAt"),-1);
   if(!FMath::IsFinite(P.LocalNoisePlannedAt)||P.LocalNoisePlannedAt<0||P.LocalNoiseOriginalId.IsEmpty()){P.LocalNoisePlannedAt=-1;P.LocalNoiseOriginalId.Empty();}
   P.ControlOriginalId=S47J::Str(*C,TEXT("controlOriginal"));P.ControlPhotoId=S47J::Str(*C,TEXT("controlPhoto"));P.ControlPlannedAt=S47J::Num(*C,TEXT("controlPlannedAt"),-1);
   if(!FMath::IsFinite(P.ControlPlannedAt)||P.ControlPlannedAt<0||P.ControlOriginalId.IsEmpty()){P.ControlPlannedAt=-1;P.ControlOriginalId.Empty();P.ControlPhotoId.Empty();}
   if(P.Id.IsEmpty()||P.PhotoId.IsEmpty()||!FMath::IsFinite(P.Quality)||P.Quality<.025||P.Quality>1||Claims.ContainsByPredicate([&](const FPhotoClaim& Q){return Q.Id==P.Id;}))continue;
   Claims.Add(P);
  }
  for(const auto& V:S47J::Arr(O,TEXT("observations")))
  {
   const S47J::FObj* Entry=nullptr;FS47FreeformObservation Observation;
   if(V.IsValid()&&V->TryGetObject(Entry)&&Entry&&FS47FreeformObservation::Read(*Entry,Observation)&&!FieldObservations.ContainsByPredicate([&](const FS47FreeformObservation& P){return P.Id==Observation.Id;}))FieldObservations.Add(Observation);
  }
  LightPlace=S47J::Str(O,TEXT("lightPlace"));PlaceId=S47J::Str(O,TEXT("place")).Left(64);PhotoId=S47J::Str(O,TEXT("photo"));Scene=S47J::Str(O,TEXT("scene"),TEXT("saro"));Stage=FMath::Clamp(S47J::Int(O,TEXT("stage")),0,3);Variant=FMath::Clamp(S47J::Int(O,TEXT("variant")),0,2);
  // Older saves have none of these: no pot until the next day, no finding owed.
  SkyLeft=FMath::Clamp(S47J::Int(O,TEXT("skyLeft")),0,1);bOwedArray=S47J::Bool(O,TEXT("owedArray"));bOwedFeeder=S47J::Bool(O,TEXT("owedFeeder"));bForcedEvent=S47J::Bool(O,TEXT("forced"));
  SeenDay=S47J::Int(O,TEXT("seenDay"));DangerNight=FMath::Max(0,S47J::Int(O,TEXT("dangerNight")));DangerCount=FMath::Max(0,S47J::Int(O,TEXT("danger")));SlotNight=FMath::Max(0,S47J::Int(O,TEXT("slotNight")));PeekNight=FMath::Max(0,S47J::Int(O,TEXT("peekNight")));PointNight=FMath::Max(0,S47J::Int(O,TEXT("pointNight")));PointCount=FMath::Clamp(S47J::Int(O,TEXT("pointOut")),0,9);MinorLeft=FMath::Clamp(S47J::Int(O,TEXT("minorLeft")),0,4);MinorBudget=FMath::Clamp(S47J::Int(O,TEXT("minorBudget"),MinorLeft),MinorLeft,4);InvestLeft=FMath::Clamp(S47J::Int(O,TEXT("investLeft")),0,2);ThreatRollDay=S47J::Int(O,TEXT("threatRollDay"));MajorRollDay=S47J::Int(O,TEXT("majorRollDay"));LastThreatDay=S47J::Int(O,TEXT("lastThreatDay"));LastMajorDay=S47J::Int(O,TEXT("lastMajorDay"),-3);
  // Old saves know that a roll was spent, but not whether an unavailable
  // event was chosen. Never reroll it or invent a pending event on migration.
  PendingThreatNight=FMath::Max(0,S47J::Int(O,TEXT("pendingThreatNight")));
  LightRollDay=FMath::Max(0,S47J::Int(O,TEXT("lightRollDay")));PendingLightNight=FMath::Max(0,S47J::Int(O,TEXT("pendingLightNight")));
  // An older save has no hour for them: they come as soon as they can, as before.
  LightAfter=FMath::Clamp(S47J::Num(O,TEXT("lightAfter")),0.,26.5);
  // Older saves: a night not yet drawn, nothing waiting.
  ObserverRollDay=FMath::Max(0,S47J::Int(O,TEXT("observerRollDay")));PendingObserverNight=FMath::Max(0,S47J::Int(O,TEXT("pendingObserverNight")));
  SiteRollDay=FMath::Max(0,S47J::Int(O,TEXT("siteRollDay")));PendingSiteNight=FMath::Max(0,S47J::Int(O,TEXT("pendingSiteNight")));
  PendingMajorNight=FMath::Max(0,S47J::Int(O,TEXT("pendingMajorNight")));
  // Older saves have none: a mystery still to come, nothing waiting.
  if(const auto Y=S47J::Obj(O,TEXT("mystery")))
  {
   const auto Finite=[&Y](const TCHAR* K,double Max){const double V=S47J::Num(Y,K);return FMath::IsFinite(V)?FMath::Clamp(V,0.,Max):0.;};
   MysteryRollDay=FMath::Max(0,S47J::Int(Y,TEXT("rollDay")));PendingMysteryNight=FMath::Max(0,S47J::Int(Y,TEXT("pendingNight")));
   MysterySeen=FMath::Clamp(S47J::Int(Y,TEXT("seen")),0,(1<<FS47FreeformCases::ClueCount)-1);MysteryClue=FMath::Clamp(S47J::Int(Y,TEXT("clue"),-1),-1,FS47FreeformCases::ClueCount-1);
   MysteryAfter=Finite(TEXT("after"),26.5);MysteryClearAt=Finite(TEXT("clearAt"),3153600000.);MysteryAge=Finite(TEXT("age"),MysteryNoticeSeconds);
  }
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
 auto O=S47J::New();S47J::Set(O,TEXT("duskOrb"),DuskOrb.Save());if(M&&M->SkyVisitors)S47J::Set(O,TEXT("highSky"),M->SkyVisitors->SaveFreeform());S47J::Set(O,TEXT("v"),1);if(!LightPlace.IsEmpty())S47J::Set(O,TEXT("lightPlace"),LightPlace);if(!PlaceId.IsEmpty())S47J::Set(O,TEXT("place"),PlaceId);S47J::Set(O,TEXT("event"),Event);S47J::Set(O,TEXT("photo"),PhotoId);S47J::Set(O,TEXT("scene"),Scene);S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("variant"),Variant);
 S47J::Set(O,TEXT("seenDay"),SeenDay);if(DangerNight>0){S47J::Set(O,TEXT("dangerNight"),DangerNight);S47J::Set(O,TEXT("danger"),DangerCount);}if(SlotNight>0)S47J::Set(O,TEXT("slotNight"),SlotNight);if(PeekNight>0)S47J::Set(O,TEXT("peekNight"),PeekNight);if(PointNight>0){S47J::Set(O,TEXT("pointNight"),PointNight);S47J::Set(O,TEXT("pointOut"),PointCount);}S47J::Set(O,TEXT("minorLeft"),MinorLeft);S47J::Set(O,TEXT("minorBudget"),MinorBudget);S47J::Set(O,TEXT("investLeft"),InvestLeft);S47J::Set(O,TEXT("threatRollDay"),ThreatRollDay);S47J::Set(O,TEXT("majorRollDay"),MajorRollDay);S47J::Set(O,TEXT("lastThreatDay"),LastThreatDay);S47J::Set(O,TEXT("lastMajorDay"),LastMajorDay);
 S47J::Set(O,TEXT("lightRollDay"),LightRollDay);S47J::Set(O,TEXT("pendingLightNight"),PendingLightNight);if(LightAfter>0)S47J::Set(O,TEXT("lightAfter"),LightAfter);if(ObserverRollDay>0)S47J::Set(O,TEXT("observerRollDay"),ObserverRollDay);if(PendingObserverNight>0)S47J::Set(O,TEXT("pendingObserverNight"),PendingObserverNight);if(SiteRollDay>0)S47J::Set(O,TEXT("siteRollDay"),SiteRollDay);if(PendingSiteNight>0)S47J::Set(O,TEXT("pendingSiteNight"),PendingSiteNight);S47J::Set(O,TEXT("pendingThreatNight"),PendingThreatNight);S47J::Set(O,TEXT("pendingMajorNight"),PendingMajorNight);
 if(bCarLight){S47J::Set(O,TEXT("carLight"),true);if(CarLightClock>=0)S47J::Set(O,TEXT("carLightClock"),CarLightClock);}
 if(SkyLeft>0)S47J::Set(O,TEXT("skyLeft"),SkyLeft);if(bOwedArray)S47J::Set(O,TEXT("owedArray"),true);if(bOwedFeeder)S47J::Set(O,TEXT("owedFeeder"),true);if(bForcedEvent)S47J::Set(O,TEXT("forced"),true);
 if(MysteryRollDay>0)
 {
  auto Y=S47J::New();S47J::Set(Y,TEXT("rollDay"),MysteryRollDay);S47J::Set(Y,TEXT("pendingNight"),PendingMysteryNight);S47J::Set(Y,TEXT("seen"),MysterySeen);S47J::Set(Y,TEXT("clue"),MysteryClue);
  S47J::Set(Y,TEXT("after"),MysteryAfter);S47J::Set(Y,TEXT("clearAt"),MysteryClearAt);S47J::Set(Y,TEXT("age"),MysteryAge);S47J::Set(O,TEXT("mystery"),Y);
 }
 S47J::Set(O,TEXT("age"),Age);S47J::Set(O,TEXT("activeTime"),ActiveTime);S47J::Set(O,TEXT("nextAt"),NextAt);S47J::Set(O,TEXT("restUntil"),RestUntil);S47J::Set(O,TEXT("soundAt"),SoundAt);S47J::Set(O,TEXT("random"),Random.GetCurrentSeed());
 S47J::Set(O,TEXT("effectsApplied"),bEffectsApplied);
 S47J::Set(O,TEXT("seen"),bSeen);S47J::Set(O,TEXT("photographed"),bPhoto);S47J::Set(O,TEXT("controlled"),bControlled);S47J::Set(O,TEXT("compared"),bCompared);S47J::Set(O,TEXT("anchor"),TArray<double>{Anchor.X,Anchor.Y,Anchor.Z});
 S47J::Set(O,TEXT("incidentSerial"),IncidentSerial);S47J::Set(O,TEXT("incidentId"),IncidentId);
 TArray<TSharedPtr<FJsonValue>> PaidPhotos;
 for(const auto& P:Claims){auto C=S47J::New();S47J::Set(C,TEXT("id"),P.Id);S47J::Set(C,TEXT("photo"),P.PhotoId);S47J::Set(C,TEXT("title"),P.Title);S47J::Set(C,TEXT("quality"),P.Quality);S47J::Set(C,TEXT("paid"),P.bPaid);if(P.LocalNoisePlannedAt>=0&&!P.LocalNoiseOriginalId.IsEmpty()){S47J::Set(C,TEXT("localNoiseOriginal"),P.LocalNoiseOriginalId);S47J::Set(C,TEXT("localNoisePlannedAt"),P.LocalNoisePlannedAt);}if(P.ControlPlannedAt>=0&&!P.ControlOriginalId.IsEmpty()){S47J::Set(C,TEXT("controlOriginal"),P.ControlOriginalId);S47J::Set(C,TEXT("controlPlannedAt"),P.ControlPlannedAt);if(!P.ControlPhotoId.IsEmpty())S47J::Set(C,TEXT("controlPhoto"),P.ControlPhotoId);}PaidPhotos.Add(MakeShared<FJsonValueObject>(C));}
 O->SetArrayField(TEXT("photoClaims"),PaidPhotos);
 if(!PhotoControlFocus.IsEmpty())S47J::Set(O,TEXT("photoControlFocus"),PhotoControlFocus);
 TArray<TSharedPtr<FJsonValue>> Observed;
 for(const auto& P:FieldObservations)Observed.Add(MakeShared<FJsonValueObject>(P.Save()));
 O->SetArrayField(TEXT("observations"),Observed);
 auto H=S47J::New();for(const auto& Pair:LastDays)S47J::Set(H,*Pair.Key,Pair.Value);S47J::Set(O,TEXT("history"),H);
 auto R=S47J::New();S47J::Set(R,TEXT("room"),ReactiveRoom);S47J::Set(R,TEXT("serial"),ReactiveSerial);S47J::Set(R,TEXT("age"),ReactiveAge);S47J::Set(R,TEXT("offClock"),ReactiveOffClock);S47J::Set(R,TEXT("onClock"),ReactiveOnClock);S47J::Set(R,TEXT("left"),bReactiveLeft);S47J::Set(R,TEXT("applied"),bReactiveApplied);S47J::Set(R,TEXT("inspected"),bReactiveInspected);S47J::Set(O,TEXT("reactiveLight"),R);
 if(GroupRestore)S47J::Set(O,TEXT("group"),GroupRestore);else if(M&&M->Night&&M->Night->Glimpses)S47J::Set(O,TEXT("group"),M->Night->Glimpses->SaveControlled());
 return O;
}
bool FS47FreeformDirector::Busy()const{return bEnabled&&!Event.IsEmpty();}
bool FS47FreeformDirector::LeavesSkyFree()const
{
 if(!Busy())return true;
 if(Instrument(Event)||S47SmallHints::Is(Event))return true;
 if(Event==TEXT("empty_car"))return !(bCarLight&&CarStage==1);
 return Event==TEXT("s03_failure")||Event==TEXT("feeder_fault")||Event==TEXT("landing_trace")||Event==TEXT("unsigned_telex")||Event==TEXT("roomlight_return");
}
int32 FS47FreeformDirector::CoolFor(const FString& Id,int32 Last)const
{
 // Always the same answer for one event's one last day. The small things may come back the next night, some the same one.
 int32 Min=2,Max=4;
 if(M&&M->Rhythm)if(const FS47RhythmEvent* E=M->Rhythm->Data().Find(FName(*Id)))if(E->CoolMin>=0){Min=E->CoolMin;Max=FMath::Max(E->CoolMin,E->CoolMax);}
 return FS47Rhythm::CoolDays(Id,Last,Min,Max);
}
bool FS47FreeformDirector::Night()const
 {if(!M||!M->Freeform)return false;const double H=FMath::Fmod(M->Freeform->Clock(),86400.)/3600.;return H>=19||H<6;}
bool FS47FreeformDirector::Near(const FVector& At,double Range)const
 {return M&&M->Player&&FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(At.X,At.Z))<=Range;}
bool FS47FreeformDirector::InView(const FVector& At,double Range)const
 {return M&&M->Night&&FVector::Dist(M->Night->GlimpseIn(true).Cam.Pos,At)<=Range&&Effects->Visible(At);}
void FS47FreeformDirector::Changed(){if(M)M->RequestAutosave();}
TArray<FString> FS47FreeformDirector::EventIds(){TArray<FString> Out;for(const auto& D:Catalog())Out.Add(D.Id);return Out;}
FVector FS47FreeformDirector::ClosePassNow() const{return ClosePassAt(Anchor,Age);}
namespace
{
 const FS47EventPlaces* PlacesOf(const US47Main* M){return M&&M->Freeform?M->Freeform->EventPlaces.Get():nullptr;}
 // F10's "the draw, as in play": where Force alone keeps a fixture's old spot.
 const TCHAR* const DrawHere=TEXT("*");
}
FVector FS47FreeformDirector::LightPlacePoint(const FString& Place)const
{
 // One list for the whole land (S47EventPlaces): the farm is a place on it like any other (Tom, 8 November).
 const FS47EventPlaces* Places=PlacesOf(M);const FS47EventPlace* P=Places?Places->Find(Place):nullptr;
 if(!P)return FVector::ZeroVector;
 // A place with a front of its own (a door, a find) is measured off it, as it always was; the rest off the land itself.
 if(!P->Prop.IsEmpty()&&M->Inter)if(auto* It=M->Inter->Get(P->Prop))if(It->Object)return It->Object.WorldBox().GetCenter();
 FVector At;Places->AtNow(Place,At);
 return At;
}
FString FS47FreeformDirector::LightPlaceTitle(const FString& Place)const
{
 const FS47EventPlaces* Places=PlacesOf(M);
 if(const FS47EventPlace* P=Places?Places->Find(Place):nullptr)return P->Title;
 if(Place==TEXT("farm"))return TEXT("the Alderete place");
 return TEXT("town");
}
FString FS47FreeformDirector::PickLightPlace(const FString& Id,bool bFixture)const
{
 const FS47EventPlaces* Places=PlacesOf(M);if(!Places)return FString();
 // Seen from here (farm_lights): a few hundred metres off, across the ground the player is on. Heard of (town_lights): somewhere
 // off that ground, a drive away, and now and then a long way off. From the game's own seed, the night and the incident count (all saved): loading again before
 // they come gives the same place, not a new throw (GAME_BIBLE: "Områdebytte eller gjeninnlasting skal ikke produsere nye kast").
 FS47PlaceNeed Need;Need.Kind=Id;Need.Any=ES47PlaceTrait::Open|ES47PlaceTrait::Town|ES47PlaceTrait::Farm|ES47PlaceTrait::Array|ES47PlaceTrait::Lot;
 if(Id==TEXT("farm_lights")){Need.MinM=250;Need.MaxM=1600;Need.FarShare=0;Need.bSameArea=true;}
 else if(bFixture){Need.FarShare=0;Need.bSameArea=true;} // a test's own street: seen from where it stands, as before the list
 else{Need.MinM=150;Need.FarShare=.35f;Need.bOtherArea=true;}
 return Places->Pick(Need,uint32(IncidentSerial)).Id;
}
bool FS47FreeformDirector::PickTrace(FString& OutPlace,FVector& OutAt,bool bForced)const
{
 const FS47EventPlaces* Places=PlacesOf(M);if(!Places||!M->Web)return false;
 FString Want=NextPlace==DrawHere?FString():NextPlace;
 // A fixture started by Force alone keeps the east yard it was written for.
 if(Want.IsEmpty()&&bForced&&NextPlace.IsEmpty()&&M->Web->Area()==TEXT("saro"))Want=TEXT("saro_yard");
 if(Want.IsEmpty())
 {
  // Open ground within reach of where the player is, on the ground they are on: found by walking, or told of on the set.
  FS47PlaceNeed Need;Need.Kind=TEXT("landing_trace");Need.All=ES47PlaceTrait::Open;Need.MaxM=700;Need.FarShare=0;Need.bSameArea=true;
  Want=Places->Pick(Need,uint32(IncidentSerial)).Id;
 }
 FVector At;
 if(Want.IsEmpty()||!Places->AtNow(Want,At))return false;
 // A few metres off the place's own middle, never under whatever names it. Fixed by the game and the incident, not the frame.
 const FS47EventPlace* P=Places->Find(Want);
 const int32 Seed=M->Freeform?M->Freeform->Seed():47;
 const double Turn=FS47NightRhythm::Roll(Seed,IncidentSerial+1,0x54524143u)*2*UE_DOUBLE_PI,Off=P?FMath::Min(P->Radius*.4,10.):3.;
 OutPlace=Want;OutAt=At+FVector(FMath::Cos(Turn)*Off,.08,FMath::Sin(Turn)*Off);
 return true;
}
FString FS47FreeformDirector::PreviewPlace(const FString& Id)const
{
 if(Id==TEXT("farm_lights")||Id==TEXT("town_lights"))return PickLightPlace(Id);
 FString Place;FVector At;
 if(Id==TEXT("landing_trace")&&PickTrace(Place,At))return Place;
 return FString();
}
void FS47FreeformDirector::OrbSite(int32 Seed,FVector2D& OutShift,FString& OutPair)const
{
 // The flight is written round S-03 with S-02 beside it. Somewhere else on the array it is the same flight round another antenna;
 // where a second stands about where S-02 stands from S-03, that one is named with it, and both have something to look over.
 OutShift=FVector2D::ZeroVector;
 if(!M||!M->Web||M->Web->DishCount()<3)return;
 const AS47Web& W=*M->Web;
 int32 S02=INDEX_NONE,S03=INDEX_NONE;
 for(int32 I=0;I<W.DishCount();++I){if(W.DishId(I)==TEXT("S-02"))S02=I;if(W.DishId(I)==TEXT("S-03"))S03=I;}
 if(S02==INDEX_NONE||S03==INDEX_NONE)return;
 const FVector2D From(W.DishX(S03),W.DishZ(S03)),Beside=FVector2D(W.DishX(S02),W.DishZ(S02))-From;
 TArray<TPair<int32,int32>> Good,Any;
 for(int32 I=0;I<W.DishCount();++I)
 {
  if(I==S03)continue;
  const FVector2D At(W.DishX(I),W.DishZ(I));int32 Partner=INDEX_NONE;double Best=8;
  for(int32 J=0;J<W.DishCount();++J)if(J!=I){const double D=FVector2D::Distance(FVector2D(W.DishX(J),W.DishZ(J)),At+Beside);if(D<Best){Best=D;Partner=J;}}
  (Partner!=INDEX_NONE?Good:Any).Add(TPair<int32,int32>(I,Partner));
 }
 // Any antenna on the array, not only where a second stands just so: on this array that is one pair, and every visit came to it.
 TArray<TPair<int32,int32>> From2=Good;From2.Append(Any);
 if(From2.IsEmpty())return;
 // The game's own seed with the visit's: another game sends them elsewhere, and a Continue keeps them where they are.
 const int32 Game=M->Freeform?M->Freeform->Seed():47;
 const TPair<int32,int32>& Pick=From2[FMath::Min(From2.Num()-1,int32(FS47NightRhythm::Roll(Game,Seed,0x4f524253u)*From2.Num()))];
 OutShift=FVector2D(W.DishX(Pick.Key),W.DishZ(Pick.Key))-From;
 OutPair=Pick.Value!=INDEX_NONE?FString::Printf(TEXT("%s and %s"),*W.DishId(Pick.Value),*W.DishId(Pick.Key)):FString::Printf(TEXT("the antennas around %s"),*W.DishId(Pick.Key));
}
bool FS47FreeformDirector::ForceLights(const FString& Place)
{
 if(LightPlacePoint(Place).IsZero())return false;
 NextLightPlace=Place;
 if(Force(TEXT("town_lights")))return true;
 NextLightPlace.Empty();return false;
}
bool FS47FreeformDirector::ForceAt(const FString& Id,const FString& Place)
{
 NextPlace=Place.IsEmpty()?FString(DrawHere):Place;
 if(Id==TEXT("farm_lights")||Id==TEXT("town_lights"))NextLightPlace=Place;
 const bool bGood=Force(Id);
 NextPlace.Empty();NextLightPlace.Empty();
 return bGood;
}
bool FS47FreeformDirector::Eligible(const FString& Id,bool Forced,double At)const
{
 if(M&&M->HighwayUfo&&M->HighwayUfo->Busy())return false;
 // Nothing else starts on top of a visit or a taking.
 if(M&&M->Freeform&&((M->Freeform->Taken&&M->Freeform->Taken->Busy())||(M->Freeform->FarmSite&&M->Freeform->FarmSite->VisitWantsGround())))return false;
 // WEEKLY-CASE-20261109: nor on top of the week's case on the ground (no presence, so no small bodies out of the glimpses either).
 if(FS47WeekCase::Holds(M))return false;
 // STORY-ON-FREEFORM-01 (step 4): nor while a Story night's script runs (false in any other night).
 if(FS47StoryDirector::Holds(M))return false;
 // DIRECTOR-MODE-01: nor while the developer flies the director's camera.
 if(S47DirectorCam::On(M))return false;
 const auto* D=Definition(Id);if(!bEnabled||!D||!M||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!Event.IsEmpty()||M->Freeform->Held())return false;
 // CRIT9-20261109: from ten to two the night is the week's case's: no threat, major or presence on the ground before it begins.
 if(!Forced&&FS47WeekCase::HoldsFarm(M))
 {
  const FS47RhythmEvent* E=M->Rhythm?M->Rhythm->Data().Find(FName(*Id)):nullptr;
  if(D->Family==TEXT("threat")||D->Family==TEXT("major")||(E&&E->bGround&&E->Level>=ES47RhythmLevel::Presence))return false;
 }
 // The initial catalogue is anchored to the actual SARO yard and apparatus.
 if(Id==TEXT("feeder_presence"))
 {if(M->Web->Area()!=TEXT("road")||!M->Freeform->Field||!Near(M->Freeform->Field->InteractionPoint(TEXT("ff:feeder")),12))return false;}
 else if(Id==TEXT("town_lights")||Id==TEXT("farm_lights"))
 {
  // Heard about from SARO too: the point is to drive there. Seen from wherever the player is on the land, the farm among the rest.
  if((M->Web->Area()!=TEXT("road")&&M->Web->Area()!=TEXT("diner")&&M->Web->Area()!=TEXT("saro"))||(NextLightPlace.IsEmpty()&&PickLightPlace(Id,Forced&&NextPlace.IsEmpty()).IsEmpty()))return false;
 }
 else if(Id==TEXT("landing_trace"))
 {if(M->Web->Area()!=TEXT("road")&&M->Web->Area()!=TEXT("diner")&&M->Web->Area()!=TEXT("saro"))return false;}
 else if(M->Web->Area()!=TEXT("saro")&&!(S47SmallHints::Is(Id)&&S47SmallHints::Anywhere(Id)))return false;
 if(!Forced)
 {
  // Every event begins with a save, and there is no saving at the wheel. What was drawn waits until the player has parked.
  if(!M->Flow||!M->Flow->FreeformSaveBlock().IsEmpty())return false;
  if(M->Freeform->InTutorial())return false; // first shift: one clean routine job
  const double Clock=At>=0?At:M->Freeform->Clock();
  const int32 Today=At>=0?FMath::Max(1,int32(FMath::FloorToDouble((At-8*3600.)/86400.))+1):M->Freeform->Day();
  const double Hour=FMath::Fmod(Clock,86400.)/3600.;const bool bDark=Hour>=19||Hour<6;
  // An instrumental anomaly needs an actually completed normal reference first.
  if(Instrument(Id)&&!M->Freeform->Jobs().ContainsByPredicate([](const FS47FreeformSession::FJob& J){return J.Source==0&&J.Stage==FS47FreeformSession::EJobStage::Filed;}))return false;
  if(Id==TEXT("unsigned_telex")&&M->Freeform->FindingCount()==0)return false;
  // Noticed at all from where the player is, and the world as it needs it (FS47SmallHints).
  if(S47SmallHints::Is(Id)&&M->Player&&!S47SmallHints::Fits(M,Id,M->Player->Pos,[this](const FVector& At){return InView(At);}))return false;
  if(Today<D->Day||(D->bNight&&!bDark)||(At<0&&(ActiveTime<RestUntil||bTestHush)))return false;
  // The evening is a normal job: hints, sounds, lights in the sky. What the rhythm file calls full (TIERS-20261109: a body, and the
  // close pass, a hull beside the player) waits for their hours, which never begin before the full tier (S47NightTiers).
  const bool bFull=M->Rhythm?M->Rhythm->TierOf(FName(*Id))==ES47Allowed::Full:(Id==TEXT("observer")||Id==TEXT("service_presence")||Id==TEXT("feeder_presence")||Id==TEXT("array_presence")||Id==TEXT("close_pass"));
  if(bFull&&!M->Freeform->TheirHoursNow(Clock))return false;
  if(Id!=TEXT("dusk_orb"))if(const int32* Last=LastDays.Find(Id))if(Today-*Last<CoolFor(Id,*Last))return false;
  // PT-FEEDER-20261109: the big night's night is its own, and a report is the night's one pointing-out.
  if(Id==TEXT("town_lights")||Id==TEXT("farm_lights"))
  {
   const int32 LightNight=FS47FreeformSession::NightOf(Clock);
   if(!LightsMayCome(Id,M->Freeform->Happenings&&M->Freeform->Happenings->BigOwns(LightNight),MayPointOut(LightNight)))return false;
  }
  if(D->Family==TEXT("threat")&&(LastThreatDay==Today||Clock<86400.+19*3600.))return false;
  // Asked at the moment it would start: one dangerous meeting a night, across every system (Tom, 7 November).
  if(At<0&&D->Family==TEXT("threat")&&M->Rhythm&&!M->Rhythm->Allow(FName(*Id)))return false;
  if(D->Family==TEXT("major")&&(Today-LastMajorDay<3||Clock<3*86400.+19*3600.))return false;
  // FEEDER-LEAD-20261110: the lead is drawn from 08:00 (DrawLead), so a fault never starts on a night that has one, or one the big night owns.
  if(Id==TEXT("feeder_fault")&&M->Freeform->Happenings)
  {
   const int32 FaultNight=FS47FreeformSession::NightOf(Clock);FString LeadEvent,LeadTitle;double LeadX=0,LeadZ=0;bool bLeadAhead=false;
   if(!FeederMayFail(M->Freeform->Happenings->BigOwns(FaultNight)||M->Freeform->Happenings->OutTonight(FaultNight,LeadEvent,LeadTitle,LeadX,LeadZ,bLeadAhead)))return false;
  }
 }
 if(Id==TEXT("roof_steps")&&M->Freeform->CaseField&&M->Freeform->CaseField->OwnsRoofWindow())return false;
 // The inspector's letter says that was the last loose sheet, and so it was.
 if(Id==TEXT("roof_steps")){const auto* Visitors=M->Freeform->CaseFile().Get(2);if(Visitors&&Visitors->bConcluded&&Visitors->Conclusion==0)return false;}
 if(Id==TEXT("array_presence")&&(!M->Freeform->Field||M->Freeform->Field->HasArrayFault()||!Near(M->Freeform->Field->InteractionPoint(TEXT("ff:service")),12)))return false;
 if(Id==TEXT("roomlight_return")&&!ReactiveReady())return false;
 if((Id==TEXT("observer")||Id==TEXT("service_presence")||Id==TEXT("feeder_presence")||Id==TEXT("array_presence"))&&(!M->Night||!M->Night->Glimpses||!M->Night->Glimpses->HasNativeBody()||!M->Night->Glimpses->Individuals().IsEmpty()))return false;
 if(Id==TEXT("s03_failure")&&(!M->Freeform->Field||M->Freeform->Field->HasArrayFault()||M->Freeform->PowerFault()))return false;
 if(Id==TEXT("feeder_fault")&&M->Freeform->PowerFault())return false;
 if(Id==TEXT("empty_car")&&(!Effects->Car||InView(EmptyCarReference)))return false;
 if(Id==TEXT("landing_trace")){FString Place;FVector Trace;if(!PickTrace(Place,Trace,Forced)||InView(Trace,40))return false;}
 return true;
}
bool FS47FreeformDirector::Start(const FString& Id,bool Forced)
{
 Effects->Build(this);if(!Eligible(Id,Forced))return false;
 IncidentSerial=FMath::Min(1000000000,IncidentSerial+1);IncidentId=FString::Printf(TEXT("%s.day%d.%d"),*Id,M->Freeform->Day(),IncidentSerial);TaggedShot.Empty();TaggedQuality=0;
 for(int32& Logged:OrbStopLogged)Logged=-1;
 bForcedEvent=Forced;OrbFleeAge=-1;Event=Id;Age=SoundAt=0;Stage=0;Variant=Random.RandRange(0,2);bSeen=bPhoto=bControlled=bCompared=false;PhotoId.Empty();bApplied=bEffectsApplied=bChoiceStored=false;Scene=M->Web->Area();
 bCarTaken=false;CarStage=0;CarLightClock=-1;
 // FIX-20261110 (Tom, point 2: the generator's use, tied to what happens here): what crosses over SARO takes the mains first. The
 // event waits out the sag (Step), which is the operator's time to get to the standby generator; then it comes, in the dark or not.
 SkyPrelude=0;bSkyCut=false;
 if((Id==TEXT("flyover")||Id==TEXT("close_pass"))&&(bSkyCutNext||(!Forced&&!M->Freeform->InTutorial()))&&Scene==TEXT("saro")&&!M->Freeform->PowerFault()&&!M->Freeform->SkyCutLive())
 {
  const auto* D=Definition(Id);const double Sag=bSkyCutNext?SkyCutSag:45,After=bSkyCutNext?SkyCutAfter:75;
  M->Freeform->SkyCutBegin(Sag,(D?D->Duration:40)+After);
  if(M->Freeform->SkySagging()){SkyPrelude=Sag;bSkyCut=true;}
 }
 bSkyCutNext=false;
 bCarLight=Id==TEXT("empty_car")&&M->Freeform->TheirHoursNow(M->Freeform->Clock())&&(CarDice>=0?CarDice:(Forced||(M->Flow&&M->Flow->bDevRun))?1.f:FMath::FRand())<.35f;
 Anchor=Id==TEXT("empty_car")?EmptyCarReference:Id==TEXT("quiet_zone")?FVector(12,1.1,-17):Id==TEXT("roof_steps")?FVector(-1.5,3.5,-1):FVector(11.9,1,-11);
 PlaceId.Empty();bTracePlaced=false;
 FS47EventPlaces* Places=M->Freeform->EventPlaces.Get();
 if(Id==TEXT("landing_trace"))
 {
  FString Place;FVector Trace;
  if(PickTrace(Place,Trace,Forced))
  {
   Anchor=Trace;PlaceId=Place;
   if(Places){const FS47EventPlace* P=Places->Find(Place);Places->Used(Place,Id);FS47EventPlaces::LogPick(Id,P,FString::Printf(TEXT("%.0f m from the player"),P?Places->DistanceFromPlayer(*P):-1.));}
  }
  else Anchor=FVector(12,.08,-12);
 }
 if(Id==TEXT("farm_lights")||Id==TEXT("town_lights"))
 {
  // Over a place from the list: the farm, a building in Caliche, the motel, SARO itself, a wash out on the land (Tom, 8 November).
  // A fixture started by Force alone keeps what it was written for: the farm from SARO, lights it can see from a street in town.
  // F10 (ForceAt) and the night draw from the whole list.
  const bool bFixture=Forced&&NextPlace.IsEmpty()&&NextLightPlace.IsEmpty();
  const bool bFarmFixture=bFixture&&Id==TEXT("farm_lights")&&M->Web->Area()==TEXT("saro");
  LightPlace=!NextLightPlace.IsEmpty()?NextLightPlace:bFarmFixture?FString(TEXT("farm")):PickLightPlace(Id,bFixture);NextLightPlace.Empty();PlaceId=LightPlace;
  const FS47EventPlace* P=Places?Places->Find(LightPlace):nullptr;
  Anchor=LightPlacePoint(LightPlace)+FVector(0,P?P->Up:28,0);
  if(P)Scene=FS47EventPlaces::AreaOf(*P);
  // Out of this area, or a long way across it: a drive, so they last that much longer before they are gone.
  const double Away=P&&Places?Places->DistanceFromPlayer(*P):0;
  if(!SameScene())Age=-170;
  else if(Away>700)Age=-FMath::Min(170.,(Away-700)/5.);
  if(Places&&P){Places->Used(LightPlace,Id);FS47EventPlaces::LogPick(Id,P,FString::Printf(TEXT("%.0f m from the player, %s"),Away,SameScene()?TEXT("on this ground"):TEXT("a drive away")));}
 }
 if(Id==TEXT("dusk_orb"))
 {
  // ARRAY-ORBS (Tom, 8 November): the first time where they have always come, in front, so that they are seen; after that
  // anywhere over the array, by the visit's own seed. A fixture started by Force alone keeps the first spot.
  const bool bHome=!NextPlace.IsEmpty()?(NextPlace==TEXT("home")):(DuskOrb.VisitsSoFar()==0||Forced);
  DuskOrb.Started(M->Freeform->Clock());
  FVector2D Shift=FVector2D::ZeroVector;OrbPair=TEXT("S-02 and S-03");
  if(!bHome)OrbSite(DuskOrb.VisitSeed(),Shift,OrbPair);
  const double X=FS47DuskOrb::HomeX+Shift.X,Z=FS47DuskOrb::HomeZ+Shift.Y;
  Anchor=FVector(X,US47Night::SaroFloorAt(M->Web,X,Z)+FS47DuskOrb::HomeUp,Z);
  UE_LOG(LogS47FreeformDirector,Display,TEXT("S47: dusk orb visit: %d of %d lights, %.0f s, flight seed %d, between %s (%.0f %.0f)%s"),DuskOrb.VisitLights(),FS47DuskOrb::Count,DuskOrb.VisitSeconds(),DuskOrb.VisitSeed(),*OrbPair,X,Z,bHome?TEXT(", the first spot"):TEXT(""));
  if(Places)FS47EventPlaces::LogPick(Id,Places->Find(TEXT("array")),FString::Printf(TEXT("over %s"),*OrbPair));
 }
 if(S47SmallHints::Is(Id))Anchor=S47SmallHints::AnchorOf(M,Id);
 if(Id==TEXT("response_lights")||Id==TEXT("flyover")||Id==TEXT("close_pass"))Anchor=FVector(-30,Id==TEXT("close_pass")?28:55,-110);
 // Store one world-space route origin; it never follows the player camera.
 if(Id==TEXT("close_pass")&&M->Player)Anchor=M->Player->Pos+FVector(-70,24,-38);
 if((Id==TEXT("feeder_fault")||Id==TEXT("feeder_presence"))&&M->Freeform->Field)Anchor=M->Freeform->Field->InteractionPoint(TEXT("ff:feeder"));
 if(Id==TEXT("array_presence"))Anchor=M->Freeform->Field->InteractionPoint(TEXT("ff:service"));
 if(Id==TEXT("roomlight_return"))Anchor=M->RoomLights->InteractionPoint(ReactiveRoom);
 if(M->Rhythm)M->Rhythm->Began(FName(*Id));
 // PT-FEEDER-20261109: the report sends the player out to the place: tonight's one pointing-out (LightsMayCome).
 if(Id==TEXT("town_lights")&&!Forced)NotePointOut(FS47FreeformSession::NightOf(M->Freeform->Clock()));
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
bool FS47FreeformDirector::StartForStory(const FString& Id,FString& Why)
{
 if(!FS47StoryDirector::Of(M)){Why=TEXT("not a Story night");return false;}
 if(!Definition(Id)){Why=TEXT("no event ")+Id;return false;}
 if(!Event.IsEmpty()){Why=TEXT("event ")+Event+TEXT(" is under way");return false;}
 if(!Start(Id,true)){Why=TEXT("the world cannot start it now (area, power, or a panel)");return false;}
 UE_LOG(LogS47FreeformDirector,Display,TEXT("S47: Freeform event %s started for the story"),*Id);
 return true;
}
bool FS47FreeformDirector::TestBegin(const FString& Id)
{
 if(!M||!M->Flow||!M->Flow->bDevRun||!Start(Id,true))return false;
 bForcedEvent=false;return true;
}
bool FS47FreeformDirector::CancelForTest()
{if(!M||!M->Flow||!M->Flow->bDevRun)return false;Finish(false);return true;}
void FS47FreeformDirector::Apply()
{
 if(bApplied||Event.IsEmpty()||!M||!M->Freeform)return;
 if(!bChoiceStored)
 {
  if(!M->Flow||M->Flow->bStarting||M->Flow->bRestoringNow)return;
  // Chosen a moment before the player turned the key: it waits for the key to come out. A developer run asks this too.
  if(!bForcedEvent&&!M->Flow->FreeformSaveBlock().IsEmpty())return;
  // M7-E: away from SARO no routine save is written (S47SaroSave::StationBlock). The event goes ahead; the save is asked for
  // and waits for the station, so a contact out there goes back to the last log filed.
  const bool bAway=!M->Flow->bDevRun&&!bTestSaveFails&&M->Flow->ManualSaveBlock()==S47SaroSave::StationOnly();
  if(bAway)M->Flow->RequestAutosave(false);
  else if(!M->Flow->bDevRun||bTestSaveFails)
  {
   FString Reason;
   if(bTestSaveFails||!M->Flow->AutosaveNow(&Reason))
   {
    // Nothing belonging to this event has touched the world yet, tip included: for the player it never happened.
    UE_LOG(LogS47FreeformDirector,Warning,TEXT("S47: Freeform pending event cancelled before effects: %s"),*Reason);
    Event.Empty();Stage=0;NextAt=ActiveTime+120;Changed();
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
  // Only now is the choice on disk: a tip that goes out here always has an event behind it.
  if(Event==TEXT("farm_lights")||Event==TEXT("town_lights"))
  {
   // Who rings it in follows where it is, not which of the two it was.
   const FS47EventPlaces* Places=PlacesOf(M);const FS47EventPlace* P=Places?Places->Find(LightPlace):nullptr;
   const FString Region=P?P->Region:FString(TEXT("land"));
   const bool bMotel=Region==TEXT("motel");
   const FString Caller=bMotel?TEXT("The night clerk at the Sierra Motor Court"):Region==TEXT("caliche")||Region==TEXT("diner")?TEXT("Somebody in town"):Region==TEXT("saro")?TEXT("A trucker on the highway"):Region==TEXT("farm")?TEXT("A caller on the county road"):TEXT("A rancher on the county road");
   const FString Tip=LightsTip((M->Freeform->Day()-1)/7,Caller,bMotel?FString(TEXT("the lot")):LightPlaceTitle(LightPlace));
   FS47FieldRadio::Report(M,Tip); // CB-WALKIE-20261108: the CB at hand, else Dale passes it on over the walkie
  }
  // Woken for this, or simply there: a figure out at the array makes itself known at least this much.
  if(Event==TEXT("observer"))
  {
   // The sound always, where it is. The words only to someone outside and in earshot; nobody near, and the fence keeps it for the morning.
   if(M->Audio)M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.35f,.At=Anchor});
   if(HeardHere(M,Anchor,25,true)){if(M->UI)M->UI->Toast(TEXT("Something shifts out along the array fence. Then nothing."),5);}
   else DuskOrb.LeaveFor(TEXT("fence"),FString(),Anchor,M->Freeform->Clock());
  }
  if(Event==TEXT("roomlight_return"))
  {
   if(!ReactiveReady()||!M->RoomLights->SetEventLight(ReactiveRoom,ReactiveSerial)){Finish(false);return;}
   bReactiveApplied=true;ReactiveOnClock=M->Freeform->Clock();
   M->Freeform->RecordEvidence(FString::Printf(TEXT("roomlight-original-%d"),ReactiveSerial),FString::Printf(TEXT("LOCAL SWITCH ENTRY / %s / %s\nWall switch set to OFF. This is the last manual operation recorded before the later circuit readback."),*ReactiveRoom,*LightStamp(ReactiveOffClock)));
  }
  if(Event==TEXT("dusk_orb"))
  {
   if(M->Audio&&Near(FVector(-3,1,1),18)){FS47Sound O;O.Gain=.12f;O.At=FVector(-3,1,1);O.Dest=TEXT("radio");const auto H=M->Audio->Sfx(TEXT("alienRadio"),O);if(H&&M->Audio->Ctx)H.Src->Stop(M->Audio->Ctx->CurrentTime()+1.2);}
   // No line here (M7): the one line about them comes the first time they are close or in sight (Step), and never again.
  }
  // Marked ground somewhere the player is not looking: somebody saw it come down, and says so.
  if(Event==TEXT("landing_trace")&&!PlaceId.IsEmpty()&&!Near(Anchor,60))
  {
   const FString Tip=FString::Printf(TEXT("A rancher says something came down near %s a while ago, a flash and no sound. He would rather not go and look."),*LightPlaceTitle(PlaceId));
   FS47FieldRadio::Report(M,Tip); // CB-WALKIE-20261108: the CB at hand, else Dale passes it on over the walkie
  }
  if(Instrument(Event)){M->Freeform->SetPhenomenon(Event);M->Freeform->SetAnomaly(true);}
  if(Event==TEXT("s03_failure")&&M->Freeform->Field)
  {
   M->Freeform->Field->TriggerServiceFault();
   if(!M->Freeform->Field->HasArrayFault()){Finish(false);return;}
   // N1110: the quiet, the relay and the answering gear are its script's; the old start was silent, so nothing stands in for it
   FS47EventCtx Fault;S47EventScript::Play(M,TEXT("s03_failure"),Fault);
  }
  // N1110: the steps above the station: the night goes still first, so the code holds the first step back while the script plays (the old timer if it cannot)
  if(Event==TEXT("roof_steps"))
  {
   FS47EventCtx Roof;Roof.Points.Add(TEXT("roof"),Anchor);
   if(S47EventScript::Play(M,TEXT("roof_steps"),Roof))SoundAt=3.5;
  }
  if(Event==TEXT("feeder_fault"))M->Freeform->SetPowerFault(true);
  if(Event==TEXT("service_presence")&&Stage==0)
  {
   if(M->Audio)M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.6f,.At=Anchor});
   if(HeardHere(M,Anchor,20,false)){if(M->UI)M->UI->Toast(TEXT("A heavy scrape beyond the service door. Something moves against the yard fence."),5);}
   else DuskOrb.LeaveFor(TEXT("gate"),FString(),Anchor,M->Freeform->Clock());
  }
  if(Event==TEXT("unsigned_telex")&&M->Audio)M->Audio->Sfx(TEXT("printer"),FS47Sound{.Gain=.25f,.At=FVector(-4,1,2)});
  if(S47SmallHints::Is(Event))S47SmallHints::Play(M,Event,Anchor);
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
 if(Event==TEXT("landing_trace"))
 {
  if(!bTracePlaced){Effects->PlaceSample(Anchor);bTracePlaced=true;}
  const FS47Obj Here=Effects->SampleFor(Scene);
  Effects->Physical(Effects->IsRoadSample(Here)?Effects->Sample:Effects->SampleRoad,false);Effects->Physical(Here,!TraceRaked());
 }
 if(Event==TEXT("empty_car"))Effects->Physical(Effects->Car,true);
 if(auto* Orb=Effects->Orb.Get())
 {
  const int32 OrbSeed=DuskOrb.VisitSeed();const double OrbSeconds=DuskOrb.VisitSeconds();
  const double V=Event==TEXT("dusk_orb")?FS47DuskOrb::Visibility(Age,OrbSeconds):0;
  const bool Here=V>0&&M->Web->Area()==Scene;
  Orb->SetVisibility(Here);Orb->SetWorldScale3D(FVector(.85*V));
  if(V>0)Orb->SetWorldLocation(S47Three::ToUE(OrbAt()+FVector(0,OrbFleeAge>=0?FS47DuskOrb::FleeRise(OrbFleeAge):0.,0)));
  const bool Low=M->Flow&&M->Flow->Graphics.Effects==0;
  for(int32 I=0;I<FS47DuskOrb::Count;++I)
  {
   auto* Fx=Effects->OrbFx[I].Get();auto* L=Effects->OrbLight[I].Get();if(!Fx||!L)continue;
   const double Own=FS47DuskOrb::Presence(Age,I,OrbSeed,OrbSeconds);
   const bool Use=Here&&I<DuskOrb.VisitLights()&&Own>0&&(!Low||I==0);
   auto* Film=Effects->OrbFilm[I].Get();
   if(!Use){if(Fx->IsActive())Fx->DeactivateImmediate();L->SetVisibility(false);if(Film&&I>0)Film->SetVisibility(false,false);continue;}
   if(!Fx->IsActive())Fx->Activate(true);
   Fx->SetVisibility(OrbLightProbe==0);
   // A startled orb goes first; the others follow a beat apart.
   const double Flee=OrbFleeAge>=0?FMath::Max(0.,OrbFleeAge-I*.12):-1;
   const FVector At=OrbAt(I)+FVector(0,Flee>=0?FS47DuskOrb::FleeRise(Flee):0.,0);
   const double Looking=Flee>=0?0.:FS47DuskOrb::ScanLevel(Age,I,OrbSeed);
   const double Pulse=.86+.14*FMath::Sin(Age*3.1+I)*FMath::Sin(Age*1.3+I*.7);
   const double Show=Own*(I==0?1.:.7)*(Flee>=0?FMath::Clamp(1.-Flee/FS47DuskOrb::FleeSeconds,0.,1.):1.);
   Fx->SetWorldLocation(S47Three::ToUE(At));
   if(Film&&I>0){Film->SetWorldLocation(S47Three::ToUE(At));Film->SetWorldScale3D(FVector(.6*Show));Film->SetVisibility(Show>0,false);}
   Fx->SetVariableFloat(TEXT("User.Phase"),float(Age+I*2.));Fx->SetVariableFloat(TEXT("User.Brightness"),float(Show*(1.+.3*Looking)));Fx->SetVariableFloat(TEXT("User.Size"),I==0?90.f:62.f);
   L->SetWorldLocation(S47Three::ToUE(At+(Flee>=0?FVector::ZeroVector:FS47DuskOrb::LightOffset(Age,I,OrbSeed))));L->SetIntensity(float((Flee>=0?2400.:900.)*Show*Pulse*(1.+.6*Looking)));L->SetVisibility(Show>0&&OrbLightProbe!=2);
  }
 }
 const bool Farm=Event==TEXT("farm_lights")||Event==TEXT("town_lights");
 const bool Sky=Event==TEXT("response_lights")||Event==TEXT("flyover")||(Farm&&SameScene());
 // Sized for how they are seen: half a kilometre off at the farm, a street or two away in town.
 const double FarmScale=Event==TEXT("farm_lights")?3.5:1.1;
 // Eight lights, wide enough to read as many from half a kilometre. They drift, never answer the viewer.
 // Once someone comes close they go straight up, staggered, and are out of sight within seconds.
 auto FarmAt=[&](int32 I){const double T=Age*.05+I*1.7;const double Up=OrbFleeAge>=0?FMath::Square(FMath::Max(0.,OrbFleeAge-I*.1))*40:0;return Anchor+FVector(0,Up,0)+FVector((I<3?(I-1)*22.:(I-5)*11.)+FMath::Sin(T)*8,(I%3)*5.-8+FMath::Sin(Age*.13+I)*4,FMath::Cos(T*.7)*14);};
 const float FarmFade=float(FMath::Min(FMath::Clamp(Age/12.,0.,1.),FMath::Clamp((240.-Age)/20.,0.,1.)));
 for(int32 I=0;I<Effects->FarmExtra.Num();++I)if(auto* C=Effects->FarmExtra[I].Get())
 {
  C->SetVisibility(Sky&&Farm);if(!(Sky&&Farm))continue;
  C->SetWorldScale3D(FVector(FarmScale));
  C->SetWorldLocation(S47Three::ToUE(FarmAt(I+3)));
  if(auto* Mat=Cast<UMaterialInstanceDynamic>(C->GetMaterial(0))){const FLinearColor Glow=S47Three::Hex(I%2?0xffe9df:0xffd9a8);Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor::Black);Mat->SetVectorParameterValue(TEXT("Emissive"),Glow*FarmFade*float(70+25*FMath::Sin(Age*.7+I)));Mat->SetScalarParameterValue(TEXT("EmissivePower"),1);}
 }
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
  while(Effects->LampScale.Num()<=I)Effects->LampScale.Add(Effects->Parts[Effects->LampScale.Num()].IsValid()?Effects->Parts[Effects->LampScale.Num()]->GetComponentScale():FVector::OneVector);
  if(!Hull)C->SetWorldScale3D(Farm?FVector(FarmScale):Effects->LampScale[I]);
  const double Side=Event==TEXT("response_lights")?0:Age*(Event==TEXT("close_pass")?2.3:5.);
  const FVector Centre=Anchor+FVector(Side,FMath::Sin(Age*.2)*.6,0);
  // The close-pass hull and its lamps share one transform. Three bounded
  // underside positions stay within the eight-metre hull instead of trailing
  // a displaced hull ten metres to the side of an unrelated light line.
  const FVector AttachedLamps[]={FVector(-2.55,-Effects->HullLampDrop,-1.35),FVector(2.55,-Effects->HullLampDrop,-1.35),FVector(0,-Effects->HullLampDrop,2.7)};
  const FVector Offset=Hull?FVector::ZeroVector:Event==TEXT("close_pass")?AttachedLamps[I-1]:FVector((I-2)*5.,(FMath::Sin(Age*.2+I)-FMath::Sin(Age*.2))*.6,0);
  const FVector At=Farm&&!Hull?FarmAt(I-1):Centre+Offset;
  C->SetWorldLocation(S47Three::ToUE(At)-(Hull?Effects->HullCentreOffsetUE:FVector::ZeroVector));
  if(!Hull)if(auto* Mat=Cast<UMaterialInstanceDynamic>(C->GetMaterial(0)))
  {
   // Slow independent lamps; honour calm-flash preference by keeping a steady output.
   const float On=M&&M->Flow&&M->Flow->bCalmFlash?1.f:float(.65+.35*FMath::Sin(Age*(bControlled?1.1:.6)+I*2.));
   Mat->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(I==2?0xffe9df:0xff4936));
   Mat->SetScalarParameterValue(TEXT("EmissivePower"),Farm?1.f:5*On);
   Mat->SetVectorParameterValue(TEXT("Emissive"),Farm?S47Three::Hex(0xffe9df)*FarmFade*80*On:S47Three::Hex(I==2?0xffe9df:0xff4936)); // Without this colour the power lights nothing.
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
bool FS47FreeformDirector::SameScene() const
{
 if(!M||!M->Web)return false;
 const FString Here=M->Web->Area();
 // On foot in town is the diner's area and in the truck it is the road's: one street either way.
 const auto Town=[](const FString& A){return A==TEXT("road")||A==TEXT("diner");};
 return Here==Scene||(Town(Here)&&Town(Scene));
}
bool FS47FreeformDirector::TraceRaked() const
{
 const FS47FreeformSession* F=M&&M->Freeform?M->Freeform.Get():nullptr;
 return F&&Event==TEXT("landing_trace")&&!PlaceId.IsEmpty()&&F->Visitors&&F->EventPlaces&&F->Visitors->Cleared(PlaceId,F->EventPlaces->LastNight(PlaceId));
}
FString FS47FreeformDirector::TipLine() const
{
 if(Event==TEXT("farm_lights")||Event==TEXT("town_lights"))return FString(TEXT("RADIO TIP: LIGHTS REPORTED LOW OVER "))+LightPlaceTitle(LightPlace).ToUpper()+TEXT(". TAKE THE CAMERA AND GO LOOK.");
 return FString();
}
void FS47FreeformDirector::TestLightsTonight(){if(M&&M->Freeform){SeenDay=M->Freeform->Day();LightRollDay=ThreatRollDay=MajorRollDay=SeenDay;PendingThreatNight=PendingMajorNight=0;PendingLightNight=SeenDay;LightAfter=0;NextAt=0;RestUntil=0;}}
void FS47FreeformDirector::RestFor(double Seconds){RestUntil=FMath::Max(RestUntil,ActiveTime+Seconds);}
void FS47FreeformDirector::NoteDanger()
{
 if(!M||!M->Freeform)return;
 const int32 Night=FS47FreeformSession::NightOf(M->Freeform->Clock());
 if(DangerNight!=Night){DangerNight=Night;DangerCount=0;}
 ++DangerCount;Changed();
}
int32 FS47FreeformDirector::DangerTonight() const{return M&&M->Freeform&&DangerNight==FS47FreeformSession::NightOf(M->Freeform->Clock())?DangerCount:0;}
int32 FS47FreeformDirector::PointOutLimit() const{return M&&M->Rhythm?M->Rhythm->Data().PointOutPerNight:1;}
void FS47FreeformDirector::NotePointOut(int32 Night)
{
 if(Night<=0)return;
 if(PointNight!=Night){PointNight=Night;PointCount=0;}
 ++PointCount;
 UE_LOG(LogS47FreeformDirector,Display,TEXT("S47: director: pointed out at the land, night %d (%d of %d)"),Night,PointCount,PointOutLimit());
 if(M)Changed();
}
bool FS47FreeformDirector::TierAllowsAt(ES47Allowed Need,double Clock,int32 Night){return S47NightTiers::Allows(Need,FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.,Night);}
bool FS47FreeformDirector::MayPointAt(const FString& EventId,double Clock,int32 Night) const
{
 const FS47RhythmEvent* E=M&&M->Rhythm?M->Rhythm->Data().Find(FName(*EventId)):nullptr;
 return TierAllowsAt(E?E->Tier:ES47Allowed::Full,Clock,Night);
}
void FS47FreeformDirector::FillRhythm(FS47RhythmNow& Out) const
{
 Out.RestLeft=FMath::Max(0.,RestUntil-ActiveTime);Out.MinorLeft=MinorLeft;Out.InvestLeft=InvestLeft;Out.SkyLeft=SkyLeft;Out.DangerTonight=DangerTonight();
 for(const auto& Pair:LastDays)Out.LastDays.Add(FName(*Pair.Key),Pair.Value);
 const auto Drew=[&Out](const TCHAR* Key,int32 Rolled,int32 Pending){if(Rolled>=Out.Day)Out.Draws.Add(Key,Pending==Out.Day?1:0);};
 Drew(TEXT("pendingThreatNight"),ThreatRollDay,PendingThreatNight);Drew(TEXT("pendingMajorNight"),MajorRollDay,PendingMajorNight);Drew(TEXT("pendingLightNight"),LightRollDay,PendingLightNight);Drew(TEXT("pendingObserverNight"),ObserverRollDay,PendingObserverNight);Drew(TEXT("pendingSiteNight"),SiteRollDay,PendingSiteNight);
 if(!Event.IsEmpty())Out.Busy=FName(*Event);
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
 for(int32* Pending:{&PendingThreatNight,&PendingMajorNight,&PendingLightNight,&PendingObserverNight,&PendingSiteNight,&PendingMysteryNight})
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
void FS47FreeformDirector::RollTonight(int32 Day)
{
 // A calm night draws nothing at all. The rest grow slowly with the nights played; lights stay as they were.
 const bool bCalm=M&&M->Freeform&&M->Freeform->QuietNight(Day);
 const double Growing=M&&M->Freeform&&M->Freeform->RulesLive()?FS47NightRhythm::Threat(Day):1.;
 const auto Chance=[this,bCalm](float Usual){return TestDice>=0?TestDice:bCalm?0.f:Usual;};
 const auto Per=[this](const TCHAR* Id,float Usual){return M&&M->Rhythm?M->Rhythm->PerNight(Id,Usual):Usual;};
 bool bAny=RollNightChoice(Day,2,Chance(Per(TEXT("service_presence"),.35f)*float(Growing)),ThreatRollDay,PendingThreatNight);
 bAny|=RollNightChoice(Day,4,Chance(Per(TEXT("close_pass"),.10f)*float(Growing)),MajorRollDay,PendingMajorNight);
 if(RollNightChoice(Day,2,TestLightAfter>=0?1.f:Chance(Per(TEXT("farm_lights"),.45f)),LightRollDay,PendingLightNight))
 {
  // Some nights while there is still work on, most nights late, when going out to them can come to something. Never
  // later than half past two, so that there is a night left to go out in.
  if(PendingLightNight==Day)LightAfter=TestLightAfter>=0?TestLightAfter:Random.FRand()<.35f?Random.FRandRange(21.f,23.5f):Random.FRandRange(24.5f,26.5f);
  bAny=true;
 }
 // Not an investigation among others, spent out of the day's budget: its own night, as the lights have. A figure two
 // nights running would be a different game, so it keeps the usual three days.
 const int32* Figure=LastDays.Find(TEXT("observer"));
 if(!Figure||Day-*Figure>=3)bAny|=RollNightChoice(Day,1,Chance(Per(TEXT("observer"),.5f)*float(Growing)),ObserverRollDay,PendingObserverNight);
 bAny|=RollNightChoice(Day,2,Chance(Per(TEXT("feeder_presence"),.4f)*float(Growing)),SiteRollDay,PendingSiteNight);
 // Its own dice, so that the night's other draws are not moved by it. Over once its case has opened.
 const auto* MysteryCase=M&&M->Freeform?M->Freeform->CaseFile().Get(FS47FreeformCases::MysteryLine):nullptr;
 if(MysteryCase&&!MysteryCase->bOpen&&(MysteryDice>=0||(M->Flow&&!M->Flow->bDevRun))&&RollNightChoice(Day,2,MysteryDice>=0?MysteryDice:bCalm?0.f:Per(TEXT("mystery"),.3f),MysteryRollDay,PendingMysteryNight))
 {
  // As the lights have an hour: some nights while there is still work on, some late.
  if(PendingMysteryNight==Day)MysteryAfter=Random.FRandRange(20.f,26.5f);
  bAny=true;
 }
 if(bAny)Changed();
}
bool FS47FreeformDirector::DrawForNight(int32 Night,double At)
{
 if(!bEnabled||!Event.IsEmpty())return false;
 // M7: the first big night wakes whoever sleeps at the station or out on the land on its night, without the dice below
 // (FS47Happenings::BigWakes). It comes at its own hour, after the waking or at once if that is past.
 if(M&&M->Freeform&&M->Freeform->Happenings&&M->Freeform->Happenings->BigWakes(Night))
 {
  UE_LOG(LogS47FreeformDirector,Display,TEXT("S47: a rest on night %d is broken for the big night"),Night);
  return true;
 }
 RollTonight(Night);
 // Only for what would really start at that hour, and can be heard or read when it does. The feeder and the array wait
 // for the player to go there: no reason to wake anyone.
 const bool bLights=PendingLightNight==Night&&(Eligible(TEXT("farm_lights"),false,At)||Eligible(TEXT("town_lights"),false,At));
 const bool bDue=bLights||(PendingThreatNight==Night&&Eligible(TEXT("service_presence"),false,At))||(PendingMajorNight==Night&&Eligible(TEXT("close_pass"),false,At))||(PendingObserverNight==Night&&Eligible(TEXT("observer"),false,At));
 // With the rule of no two nights running this comes to about two nights in five (a model of the night's dice: .85 gives
 // 40 percent). The rest is a night slept through, whatever was drawn: it runs out unseen in the morning.
 if(!bDue||(TestDice<0&&Random.FRand()>=.85f))return false;
 // The lights' own hour must not leave the sleeper awake with nothing to hear: they are there by two.
 if(bLights)LightAfter=FMath::Min(LightAfter,26.);
 NextAt=ActiveTime+8;RestUntil=FMath::Min(RestUntil,ActiveTime+8);Changed();
 return true;
}
FString FS47FreeformDirector::RestBlock() const
{
 if(!bEnabled||!M||!M->Freeform)return FString();
 const auto& Session=*M->Freeform;
 if(M->HighwayUfo&&M->HighwayUfo->Busy())return TEXT("Not with that light still over the road.");
 if(Session.Taken&&Session.Taken->Busy())return TEXT("Not now.");
 if(Session.FarmSite&&Session.FarmSite->VisitWantsGround())return TEXT("Something is out here with you. Get away from it before you think about sleep.");
 if(Session.IndoorDread&&Session.IndoorDread->PeekActive())return TEXT("Something is looking in. You are not lying down with that there.");
 if(Event==TEXT("service_presence"))return TEXT("Something is moving along the service yard fence. Get inside and shut the doors first.");
 if(Event==TEXT("close_pass"))return TEXT("Something is passing low over the station. Nobody sleeps through that.");
 if(Event==TEXT("observer")||Event==TEXT("feeder_presence")||Event==TEXT("array_presence"))return TEXT("There is a figure out by the antennas. Not while it is still there.");
 // A night event's danger, once met (the light over the truck, the woman by the road, the voice): stress reads this too.
 if(Session.Happenings){const FString Night=Session.Happenings->RestBlock();if(!Night.IsEmpty())return Night;}
 // CRIT6-20261109: a director script under way: a rest would jump the clock past its timeline with its figure still out.
 if(FS47ScriptPlayer::Busy(M))return TEXT("Not now.");
 // WEEKLY-CASE-20261109: the week's case on the ground.
 if(const FString Week=FS47WeekCase::RestBlockOf(M);!Week.IsEmpty())return Week;
 return FString();
}
void FS47FreeformDirector::EndForRest()
{
 if(!bEnabled||Event.IsEmpty()||!RestBlock().IsEmpty())return;
 if(Event==TEXT("s03_failure")&&bEffectsApplied)bOwedArray=true;
 if(Event==TEXT("feeder_fault")&&bEffectsApplied)bOwedFeeder=true;
 Finish(bPhoto,TEXT("I turned in before it was over."));
}
FString FS47FreeformDirector::EveningLightsAt(double Clock,int32 NightVisits,double NightDue)
{
 // D45-20261110: only on the first night, from 19:00, before its first visit has started and while it can still come (its hour's window).
 if(!FMath::IsFinite(Clock)||Clock<0||FS47FreeformSession::NightOf(Clock)!=1||NightVisits>0||!FMath::IsFinite(NightDue)||NightDue<0)return FString();
 const double Hour=FMath::Fmod(Clock,86400.)/3600.;
 if(Hour<19||Clock>NightDue+3600.||NightDue-Clock>6*3600.)return FString();
 return TEXT("YARD / Lights show up over the dishes most evenings around nine. Watch from the yard, and stay well back.");
}
FString FS47FreeformDirector::EveningLightsLine() const
{
 if(!M||!M->Freeform||!M->Freeform->Active()||!M->Freeform->RulesLive()||M->Freeform->InTutorial())return FString();
 return EveningLightsAt(M->Freeform->Clock(),DuskOrb.NightVisitCount(),DuskOrb.NextNightClock());
}
FString FS47FreeformDirector::ReportedLine() const
{
 FString Where;
 if(Event==TEXT("farm_lights")||Event==TEXT("town_lights"))Where=LightPlaceTitle(LightPlace);
 else if(Event==TEXT("landing_trace")&&!PlaceId.IsEmpty()&&!Near(Anchor,60))return FString::Printf(TEXT("Reported: something came down near %s"),*LightPlaceTitle(PlaceId));
 else if(M&&M->Freeform&&M->Freeform->FarmSite&&M->Freeform->FarmSite->Armed())Where=M->Freeform->FarmSite->PlaceTitle();
 return Where.IsEmpty()?FString():FString::Printf(TEXT("Reported: lights at %s (until noon)"),*Where);
}
bool FS47FreeformDirector::ReportDone() const
{
 if(Event==TEXT("landing_trace")&&!PlaceId.IsEmpty()&&!Near(Anchor,60))return false;
 const FS47FarmSite* Site=M&&M->Freeform?M->Freeform->FarmSite.Get():nullptr;
 return ReportDoneWith(Event==TEXT("farm_lights")||Event==TEXT("town_lights"),Site&&Site->Armed(),Site&&Site->Checked());
}
FString FS47FreeformDirector::MysteryTrailHint() const
{
 for(const TPair<FString,bool>& Row:MysteryTrailRows())if(!Row.Value)return Row.Key;
 return FString();
}
TArray<TPair<FString,bool>> FS47FreeformDirector::MysteryTrailRows() const
{
 // (the trail opens with the first clue found, so one row is always ticked; the words stay the same done or not, so the note's key holds)
 TArray<TPair<FString,bool>> Out;
 if(MysteryClearAt<=0)return Out;
 Out.Add({TEXT("WORTH A LOOK / The remote feeder on the county road, after midnight. Check it before noon."),(MysterySeen&2)!=0}); // D45-20261110: says when, not a riddle
 Out.Add({TEXT("WORTH A LOOK / The receiver chart at the rack. Check it before noon."),(MysterySeen&1)!=0});
 return Out;
}
void FS47FreeformDirector::StepOwed()
{
 if(!M||!M->Freeform)return;
 const auto Mended=[this](bool& bOwed,bool bStill,const TCHAR* Id)
 {
  if(!bOwed||bStill)return;
  bOwed=false;const auto* Def=Definition(Id);if(!Def)return;
  const FString Key=M->Freeform->HasFinding(Id)?FString(Id)+FString::Printf(TEXT(".day%d"),M->Freeform->Day()):FString(Id);
  M->Freeform->RecordEvidence(Key,Def->Finding);Changed();
  if(FCString::Strcmp(Id,TEXT("s03_failure"))==0){FS47EventCtx Back;S47EventScript::Play(M,TEXT("s03_failure_mended"),Back);} // N1110: the finding stays the code's
 };
 Mended(bOwedArray,M->Freeform->Field&&M->Freeform->Field->HasArrayFault(),TEXT("s03_failure"));
 Mended(bOwedFeeder,M->Freeform->PowerFault(),TEXT("feeder_fault"));
}
int32 FS47FreeformDirector::MysteryClueHere() const
{
 if(!M||!M->Web||!M->Freeform||!M->Player||!M->World||M->World->bDriving)return -1;
 const FString Here=M->Web->Area();
 // At the station, not out at the farm, which shares its area.
 if(Here==TEXT("saro"))return Near(FVector(-3,1,1),18)?0:-1; // in earshot of the rack that makes the sound
 // The evening keeps to small hints: a kick of a meter. Nothing is there to be found at the feeder before midnight.
 const double Hour=FMath::Fmod(M->Freeform->Clock(),86400.)/3600.;
 if(Here==TEXT("road")&&Hour<12&&M->Freeform->Field&&Near(M->Freeform->Field->InteractionPoint(TEXT("ff:feeder")),12))return 1;
 return -1;
}
bool FS47FreeformDirector::MysteryQuiet() const
{
 if(!M||!M->Freeform||!Event.IsEmpty()||ActiveTime<RestUntil||bTestHush||M->Freeform->InTutorial())return false;
 if(M->HighwayUfo&&M->HighwayUfo->Busy())return false;
 const auto& Session=*M->Freeform;
 if((Session.Taken&&Session.Taken->Busy())||(Session.FarmSite&&Session.FarmSite->VisitWantsGround())||(Session.IndoorDread&&Session.IndoorDread->PeekActive()))return false;
 return !M->Night||!M->Night->Glimpses||M->Night->Glimpses->Individuals().IsEmpty();
}
void FS47FreeformDirector::NoticeMystery(int32 Clue,bool bFirst)
{
 // The same sound and the same words under every cause: nothing here depends on what it was.
 if(Clue==0)
 {
  if(bFirst&&M->Audio&&Near(FVector(-3,1,1),18)){FS47Sound O;O.Gain=.12f;O.At=FVector(-3,1,1);O.Dest=TEXT("radio");const auto H=M->Audio->Sfx(TEXT("alienRadio"),O);if(H&&M->Audio->Ctx)H.Src->Stop(M->Audio->Ctx->CurrentTime()+1.2);}
  if(M->UI)M->UI->Toast(bFirst?TEXT("The receiver meter kicks and settles. A second of hash from the rack."):TEXT("The receiver chart has a burst on it from the same night. You keep the strip."),5);
  return;
 }
 if(M->Audio&&M->Freeform->Field)M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.25f,.At=M->Freeform->Field->InteractionPoint(TEXT("ff:feeder"))});
 if(M->UI)M->UI->Toast(TEXT("The feeder panel buzzes once as you come up to it. You look it over."),5);
}
void FS47FreeformDirector::StepMystery(double Dt)
{
 const FS47FreeformCases::FMysteryDef* Y=FS47FreeformCases::Def(FS47FreeformCases::MysteryLine).Mystery;if(!Y)return;
 auto& Session=*M->Freeform;const double Now=Session.Clock();
 if(MysteryClue>=0)
 {
  // The notice is out. A Continue in here picks up the count with no second sound.
  MysteryAge+=Dt;if(MysteryAge<MysteryNoticeSeconds)return;
  const int32 Found=MysteryClue;MysteryClue=-1;MysteryAge=0;MysterySeen|=1<<Found;
  if(MysteryClearAt<=0)MysteryClearAt=(FMath::FloorToDouble((Now-12*3600.)/86400.)+1)*86400.+12*3600.; // the next noon
  Session.RecordEvidence(Y->Clues[Found].Finding,Session.CaseFile().ClueText(FS47FreeformCases::MysteryLine,Found));
  Changed();return;
 }
 if(MysteryClearAt>0&&Now>=MysteryClearAt){MysteryClearAt=0;Changed();return;}
 const double Hour=FMath::Fmod(Now,86400.)/3600.;
 const bool bDrawn=PendingMysteryNight>0&&PendingMysteryNight==Session.Day()&&(Hour<12?Hour+24:Hour)>=MysteryAfter;
 if(!bDrawn&&MysteryClearAt<=0)return;
 const int32 Here=MysteryClueHere();
 if(Here<0||(MysterySeen&(1<<Here))||!MysteryQuiet())return;
 const bool bFirst=MysterySeen==0;
 MysteryClue=Here;MysteryAge=0;PendingMysteryNight=0;Changed();
 NoticeMystery(Here,bFirst);
}
void FS47FreeformDirector::Step(double Dt)
{
 if(!bEnabled&&bTestKeepGroup&&M&&M->Freeform&&M->Freeform->Active()&&FMath::IsFinite(Dt)&&Dt>0){Effects->Build(this);StepGroup(Dt);return;}
 if(!bEnabled||!M||!M->Freeform||!M->Freeform->Active()||!FMath::IsFinite(Dt)||Dt<=0)return;
 Effects->Build(this);StepGroup(Dt);if(M->Freeform->Held())return;
 Dt=FMath::Min(Dt,.25);ActiveTime+=Dt*TestScale;StepReactive(Dt);StepOwed();
 if(M->SkyVisitors)M->SkyVisitors->Step(Dt);
 const int32 Day=M->Freeform->Day();
 const bool bNight=Night();if(ExpireNightChoices(Day,bNight))Changed();
 if(SeenDay!=Day){SeenDay=Day;{const FS47RhythmData Plain;const FS47RhythmData& R=M->Rhythm?M->Rhythm->Data():Plain;MinorLeft=MinorBudget=FMath::Min(Random.RandRange(R.MinorMin,R.MinorMax),4);InvestLeft=FMath::Min(Random.RandRange(R.InvestMin,R.InvestMax),2);}SkyLeft=(Random.GetCurrentSeed()>>3)&1;NextAt=FMath::Max(ActiveTime+60,RestUntil);Changed();}
 // The first shift's end is not a cue: what comes first after the routine job waits an ordinary interval from then, so that it does
 // not land in the same second as the payment, and does not race whatever the shift's last minute held (the normal journey's
 // S-03). A run whose fixture has just reset the scheduler (NextAt 0, TestLightsTonight) or a developer run is left as it is.
 {
  const bool bFirstShift=M->Freeform->InTutorial();
  if(bWasFirstShift&&!bFirstShift&&NextAt>0&&M->Flow&&!M->Flow->bDevRun)
  {
   const FS47RhythmData Plain;const FS47RhythmData& R=M->Rhythm?M->Rhythm->Data():Plain;
   NextAt=FMath::Max(NextAt,ActiveTime+R.TickMin);Changed();
   UE_LOG(LogS47FreeformDirector,Display,TEXT("S47: first shift over: the first draw waits until %.0f s (now %.0f)"),NextAt,ActiveTime);
  }
  bWasFirstShift=bFirstShift;
 }
 if(DuskOrb.Update(M->Freeform->Clock(),M->Freeform->InTutorial()))Changed();
 // D45-20261110 (D45-26): the first evening's appointment, said once as it goes on the note.
 if(!bEveningTold){const FString Watch=EveningLightsLine();if(!Watch.IsEmpty()){bEveningTold=true;if(M->UI)M->UI->Toast(Watch,8);UE_LOG(LogS47FreeformDirector,Display,TEXT("S47: the first evening's appointment on the note: the lights around nine"));}}
 // What a night left out there waits for the player to come to it, on foot, by the next morning; then it is gone.
 {
  const double Clock=M->Freeform->Clock();
  if(DuskOrb.ExpireLeft(Clock))Changed();
  if(!DuskOrb.Left().IsEmpty()&&M->Player&&M->Web&&M->Web->Area()==TEXT("saro")&&!(M->World&&M->World->bDriving))
  {
   const int32 Here=DuskOrb.LeftNear(Clock,FVector2D(M->Player->Pos.X,M->Player->Pos.Z));
   if(Here>=0){const FS47DuskOrb::FLeft L=DuskOrb.Left()[Here];DuskOrb.Found(Here);FoundLeft(M,L);Changed();}
  }
 }
 if(Event==TEXT("roof_steps")&&M->Freeform->CaseField&&M->Freeform->CaseField->OwnsRoofWindow()){Finish(false);return;}
 StepMystery(Dt);
 if(Event.IsEmpty())
 {
  if(!M->Freeform->InTutorial()&&DuskOrb.Ready(M->Freeform->Clock())&&Start(TEXT("dusk_orb"),false))return;
  // Twelve metres on one scheduler tick would almost never be met: this looks every second, for the player's arrival.
  if(PendingSiteNight==Day&&bNight&&ActiveTime>=SiteCheckAt)
  {
   SiteCheckAt=ActiveTime+TestScale;
   if(StartPendingNightChoice(TEXT("feeder_presence"),Day,PendingSiteNight)||StartPendingNightChoice(TEXT("array_presence"),Day,PendingSiteNight))return;
  }
  // Not only at dusk: the low lights keep their own hour in the dark. The same event, the same path, the same photograph.
  if(!M->Freeform->InTutorial()&&(bTestOrbNight||(M->Flow&&!M->Flow->bDevRun&&!M->Freeform->QuietNight(Day)))&&DuskOrb.ReadyNight(M->Freeform->Clock())&&Start(TEXT("dusk_orb"),false))
  {UE_LOG(LogTemp,Display,TEXT("S47: dusk orb night visit %d starts at %.2f h, from the schedule"),DuskOrb.NightVisitCount(),FMath::Fmod(M->Freeform->Clock(),86400.)/3600.);return;}
  if(ActiveTime<NextAt||ActiveTime<RestUntil)return;{const FS47RhythmData Plain;const FS47RhythmData& R=M->Rhythm?M->Rhythm->Data():Plain;NextAt=ActiveTime+Random.FRandRange(float(R.TickMin),float(R.TickMax));}
  if(bNight)
  {
   RollTonight(Day);
   // M7 (B1): on SARO at night the shared danger slot can be taken by something at the service yard (the window face waits 9 minutes for it)
   if(M->Rhythm&&M->Rhythm->DangerSlotOpen()&&PendingThreatNight!=Day&&M->Web&&M->Web->Area()==TEXT("saro"))PendingThreatNight=Day;
   if(StartPendingNightChoice(TEXT("service_presence"),Day,PendingThreatNight))return;
   if(StartPendingNightChoice(TEXT("close_pass"),Day,PendingMajorNight))return;
   if(StartPendingNightChoice(TEXT("observer"),Day,PendingObserverNight))return;
   // Lights are a night's own draw, not left to what the day's instruments use up. Seen from where the player is:
   // over the farm or in town, from SARO; in town, from the road.
   const double LightHour=FMath::Fmod(M->Freeform->Clock(),86400.)/3600.;
   if(PendingLightNight==Day&&(LightHour<12?LightHour+24:LightHour)>=LightAfter)
   {
    // Seen from here or heard of: either, wherever the player is on the land.
    const FString First=FMath::RandBool()?TEXT("farm_lights"):TEXT("town_lights");
    const FString Other=First==TEXT("farm_lights")?TEXT("town_lights"):TEXT("farm_lights");
    if(StartPendingNightChoice(First,Day,PendingLightNight)||StartPendingNightChoice(Other,Day,PendingLightNight))return;
   }
  }
  TArray<FString> Choices;
  // The small things are spread over the dark, not spent in its first hours: a night should feel as if something is going on all through it.
  // A developer run's fixtures keep the fixed night they were written for: the same list, in the same order, spent as it comes.
  const bool bLiveNight=M->Freeform->RulesLive();
  const bool bMinorDue=!bLiveNight||!bNight||S47SmallHints::PaceAllows(MinorBudget-MinorLeft,MinorBudget,FMath::Fmod(M->Freeform->Clock(),86400.)/3600.,FMath::FRand());
  // A calm night (FS47NightRhythm::Quiet) has no bodies, no lights and nothing that leads anywhere, but it is not an empty
  // one: the small deniable things still come (Tom, 7 November: "more must happen"; krit9 night 5 had none at all).
  for(const auto& D:Catalog())if(D.Id!=TEXT("dusk_orb")&&D.Id!=TEXT("farm_lights")&&D.Id!=TEXT("town_lights")&&D.Id!=TEXT("observer")&&D.Id!=TEXT("feeder_presence")&&D.Id!=TEXT("array_presence")&&D.Family!=TEXT("threat")&&D.Family!=TEXT("major")&&(!D.bNight||!bNight||!M->Freeform->QuietNight(Day)||S47SmallHints::Is(D.Id))&&(bLiveNight||!S47SmallHints::Is(D.Id))&&Eligible(D.Id,false))
  {const bool Minor=D.Family==TEXT("minor")||D.Family==TEXT("maintenance");const bool bFar=D.Id==TEXT("response_lights")||D.Id==TEXT("quiet_zone");if((Minor&&MinorLeft>0&&bMinorDue)||(bFar&&SkyLeft>0)||(!Minor&&!bFar&&InvestLeft>0))Choices.Add(D.Id);}
  if(Choices.IsEmpty()){Changed();return;}
  const FString Id=Choices[Random.RandRange(0,Choices.Num()-1)];
  if(Start(Id,false)){const auto* D=Definition(Id);if(D->Family==TEXT("minor")||D->Family==TEXT("maintenance"))--MinorLeft;else if(Id==TEXT("response_lights")||Id==TEXT("quiet_zone"))--SkyLeft;else --InvestLeft;Changed();}return;
 }
 if(SkyPrelude>0){SkyPrelude-=Dt;if(SkyPrelude>0&&M->Freeform->SkySagging())return;SkyPrelude=0;}
 Apply();if(!bApplied||Event.IsEmpty())return;Age+=Dt*TestPace;Draw();const auto* Def=Definition(Event);if(!Def){Finish(false);return;}
 // MILITARY-20261108: they got to the marked ground first. Nothing left to photograph or pick up; their raked circle where it lay.
 if(TraceRaked())
 {
  double RoadX=0,RoadZ=0;
  if(S47OpenLand::RoadOf(Scene==TEXT("diner")?FString(TEXT("road")):Scene,Anchor.X,Anchor.Z,RoadX,RoadZ))M->Freeform->Visitors->RakeAt(PlaceId,M->Freeform->EventPlaces->LastNight(PlaceId),RoadX,RoadZ);
  UE_LOG(LogS47FreeformDirector,Display,TEXT("S47: landing_trace at %s gone over by the military: marks and fragment gone"),*PlaceId);
  Finish(false);return;
 }
 // Hardware can be repaired at a separate physical site (the feeder is on the
 // road). Complete against its saved authority before applying scene-local rules.
 if(Event==TEXT("s03_failure")&&M->Freeform->Field&&!M->Freeform->Field->HasArrayFault()){Finish(true);return;}
 if(Event==TEXT("feeder_fault")&&!M->Freeform->PowerFault()){Finish(true);return;}
 // A fault is the equipment's from here on, however long it stands: the night goes on around it. Its finding is written
 // when it is mended (StepOwed), and Rest or a drive to Caliche has nothing to do with it.
 if((Event==TEXT("s03_failure")||Event==TEXT("feeder_fault"))&&!bForcedEvent)
 {
  (Event==TEXT("s03_failure")?bOwedArray:bOwedFeeder)=true;
  Finish(false);return;
 }
 // Nothing else outlives the player leaving: an event held open out of sight would close the night to everything else.
 if(!SameScene()&&Event!=TEXT("farm_lights")&&Event!=TEXT("town_lights"))
 {if(!bForcedEvent||Event==TEXT("service_presence")||Event==TEXT("feeder_presence")||Event==TEXT("array_presence")||Event==TEXT("close_pass")||Event==TEXT("flyover")||Event==TEXT("dusk_orb"))Finish(bPhoto,TEXT("I left before it was over."));return;}
 if(Event==TEXT("empty_car")&&bCarLight&&StepCarLight(Def->Duration))return;
 if((Event==TEXT("farm_lights")||Event==TEXT("town_lights"))&&M->Player)
 {
  const double FarmFleeRange=Event==TEXT("farm_lights")?130:70;static constexpr double FarmFleeSeconds=4;
  if(OrbFleeAge>=0){OrbFleeAge+=Dt;if(OrbFleeAge>=FarmFleeSeconds){Finish(bPhoto,FString::Printf(TEXT("Several lights hung low over %s, then went straight up as I came near."),*LightPlaceTitle(LightPlace)));return;}}
  // Seen first: however close the tip finds the player, the lights have time to come up.
  else if(Age>=14&&SameScene()&&FVector2D(M->Player->Pos.X-Anchor.X,M->Player->Pos.Z-Anchor.Z).Size()<FarmFleeRange)
  {OrbFleeAge=0;UE_LOG(LogTemp,Display,TEXT("S47: farm lights left at %.1f s"),Age);}
 }
 if(Event==TEXT("dusk_orb")&&M->Player)
 {
  if(OrbFleeAge<0)for(int32 I=0;I<DuskOrb.VisitLights();++I)
  {
   const int32 StopNow=OrbStopNow(I);if(StopNow==OrbStopLogged[I])continue;
   OrbStopLogged[I]=StopNow;if(StopNow>=0){UE_LOG(LogTemp,Display,TEXT("S47: dusk orb %d stops at %s at %.1f s"),I,FS47DuskOrb::Stops()[StopNow].Name,Age);}
  }
  // The one line about them, the first time one is close or in sight: not from a room it cannot be seen from, and never again.
  if(OrbFleeAge<0&&!DuskOrb.Told()&&!DuskOrb.WasPhotographed())for(int32 I=0;I<DuskOrb.VisitLights();++I)
  {
   if(FS47DuskOrb::Presence(Age,I,DuskOrb.VisitSeed(),DuskOrb.VisitSeconds())<=.5)continue;
   const FVector At=OrbAt(I);
   if(!InView(At,150)&&!SaysAloud(FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(At.X,At.Z)),25,true,PlayerIndoors(M),true))continue;
   DuskOrb.Tell();Changed();
   if(M->UI)M->UI->Toast(FString::Printf(TEXT("A low light moves between %s. Worth a photograph."),OrbPair.IsEmpty()?TEXT("the antennas"):*OrbPair),6);
   break;
  }
  if(OrbFleeAge>=0)
  {
   OrbFleeAge+=Dt;
   if(OrbFleeAge>=FS47DuskOrb::FleeSeconds+.3){Finish(bPhoto,TEXT("The lights rose straight up the moment I came close."));return;}
  }
  else for(int32 I=0;I<DuskOrb.VisitLights();++I)if(FS47DuskOrb::Presence(Age,I,DuskOrb.VisitSeed(),DuskOrb.VisitSeconds())>.5&&FVector::Dist(M->Player->Pos,OrbAt(I))<FS47DuskOrb::FleeRange)
  {
   OrbFleeAge=0;UE_LOG(LogTemp,Display,TEXT("S47: dusk orb %d startled at %.1f s"),I,Age);
   if(M->Audio){FS47Sound O;O.Gain=.2f;O.At=OrbAt(I);O.Dest=TEXT("radio");const auto H=M->Audio->Sfx(TEXT("alienRadio"),O);if(H&&M->Audio->Ctx)H.Src->Stop(M->Audio->Ctx->CurrentTime()+.8);}
   break;
  }
 }
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
    bControlled=true;M->Freeform->Field->TriggerServiceFault(false,M->Freeform->Field->ServiceDishId());Changed();
   }
   if(Event==TEXT("service_presence")&&Stage==1&&I.State==FS47RoswellBrain::EState::Contact&&!(M->World&&M->World->bDriving))
   {const FString Why=TEXT("Something in the service yard got to me. I do not remember coming inside.");Finish(false);if(M->Night&&M->Night->Glimpses)M->Night->Glimpses->DevClear();M->Freeform->Recover(Why);return;}
  }
  const bool Safe=M->Web->Rect(TEXT("room.bounds")).Contains(M->Player->Pos)&&!M->Web->IsDoorOpen(TEXT("doorSouth"))&&!M->Web->IsDoorOpen(TEXT("doorEast"));
  if(Stage==1&&((Event==TEXT("service_presence")&&Safe&&Age>15)||Age>Def->Duration))
  {G->ControlledRelease();Stage=2;Changed();}
  if(Stage==2&&(Views.IsEmpty()||Age>Def->Duration+30))
  {
   // End pressure after a bounded withdrawal even if the player keeps watching.
   // Finish releases the body to its normal hidden retirement; it never removes it in view.
   // Seen is enough for the watcher at the base: a case about visitors cannot hang on having a camera to hand.
   Finish(Event==TEXT("service_presence")||bPhoto||(Event==TEXT("observer")&&bSeen));return;
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
 if(Event==TEXT("landing_trace")&&Effects->PresentedComponent(Effects->SampleFor(Scene))&&InView(Anchor,8))ObserveVisitor(Anchor);
 if(Event==TEXT("quiet_zone")&&M->Audio)
 {const bool Inside=Near(Anchor,5);M->Audio->HoldRoomTone(Inside?.15f:1.f);if(Stage==0&&Inside){Stage=1;Changed();}else if(Stage==1&&!Inside){Stage=2;Changed();}}
 if(Event==TEXT("roof_steps")&&Age>=6&&Near(FVector(9,0,0),8)&&!M->Web->Rect(TEXT("room.bounds")).Contains(M->Player->Pos))
 {
  // N1110: once, when the player first goes out under the steps; the script is the yard's quiet and the steps going on (nothing if it cannot play)
  if(Stage==0){FS47EventCtx Yard;Yard.Points.Add(TEXT("roof"),Anchor);S47EventScript::Play(M,TEXT("roof_steps_yard"),Yard);}
  bSeen=true;Stage=1;
 }
 // The low lights' visits are of uneven length, written with each one.
 if(Age>(Event==TEXT("dusk_orb")?FMath::Min(Def->Duration,DuskOrb.VisitSeconds()):Def->Duration)&&Event!=TEXT("observer")&&Event!=TEXT("service_presence")&&Event!=TEXT("feeder_presence")&&Event!=TEXT("array_presence"))
 {
  if(Def->Action==TEXT("service")||Def->Action==TEXT("feeder")){Finish(false);return;} // leave actual unresolved hardware state intact
  if(Event==TEXT("roof_steps")){if(bSeen){FS47EventCtx Over;S47EventScript::Play(M,TEXT("roof_steps_end"),Over);}Finish(bSeen);return;} // N1110: the finding is Finish's, key unchanged
  Finish(bPhoto||bCompared||(Event==TEXT("quiet_zone")&&Stage==2));
 }
}
// True when this has decided what the event does this step. Nobody near and it is only a parked car, as on any other night.
bool FS47FreeformDirector::StepCarLight(double Duration)
{
 const auto Tk=M->Freeform->Taken;if(!Tk)return false;
 if(CarStage==0)
 {
  const bool bVisit=M->Freeform->FarmSite&&M->Freeform->FarmSite->VisitWantsGround();
  if(!M->Freeform->TheirHoursNow(M->Freeform->Clock())||!Near(EmptyCarReference,32)||bVisit||M->ModalOpen())return false;
  // BeginOver asks the night's rules for "taken" (one dangerous meeting a night, the first day free): refused, it is a parked car.
  if(!Tk->BeginOver(ES47Taker::Roswell,TEXT("the car in the staff bay"),EmptyCarFoot))return false;
  CarStage=1;CarLightClock=M->Freeform->Clock();Changed();return true;
 }
 if(CarStage==1)
 {
  // The time to inspect it afterwards is not used up while the light stands there.
  Age=FMath::Min(Age,Duration-60);
  if(Tk->Stage()==FS47Taken::EStage::Lift||Tk->Stage()==FS47Taken::EStage::Away)bCarTaken=true;
  if(Tk->Busy())return true;
  // Taken: FS47Taken wrote what they can check, and the car is no longer the story.
  if(bCarTaken){Finish(false);return true;}
  CarStage=2;Changed();return true;
 }
 if(Age>Duration)Finish(bPhoto,TEXT("A light stood over the empty car in the staff bay, then went out. I did not go up to the car afterward."));
 return true;
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
 if((Event==TEXT("farm_lights")||Event==TEXT("town_lights"))&&M&&M->Freeform&&M->Freeform->FarmSite)
 {
  // Their marks where they were, at whichever place that was: the farm's pasture as any other.
  const FS47EventPlaces* Places=PlacesOf(M);const FS47EventPlace* P=Places?Places->Find(LightPlace):nullptr;
  M->Freeform->FarmSite->Arm(M->Freeform->Clock(),LightPlace,LightPlaceTitle(LightPlace),P?FS47EventPlaces::AreaOf(*P):Scene,LightPlacePoint(LightPlace));
 }
 // After the first photograph a visit pays nothing more: what it gives is what it leaves at the antenna it was over (M7).
 if(Event==TEXT("dusk_orb")&&DuskOrb.VisitAfterPhoto()&&M&&M->Freeform)
 {FString Dish;FVector At;if(OrbDish(M,Anchor,Dish,At))DuskOrb.LeaveFor(TEXT("orb"),Dish,At,M->Freeform->Clock(),DuskOrb.VisitClock());}
 // FIX-20261110: the standby generator carried the station through it: the line recorder kept writing, and that strip is a record
 // of its own, seen or not, photographed or not. In the dark there is nothing but what the operator saw.
 if(bSkyCut&&M&&M->Freeform&&M->Freeform->SkyRode())
 {
  const int32 Sec=int32(FMath::Fmod(FMath::Max(0.,M->Freeform->Clock()),86400.));
  const FString Key=M->Freeform->HasFinding(TEXT("mains-ride"))?FString::Printf(TEXT("mains-ride.day%d"),M->Freeform->Day()):FString(TEXT("mains-ride"));
  M->Freeform->RecordEvidence(Key,FString::Printf(TEXT("Mains failure under a clear sky, %02d:%02d. The standby generator carried the station and the line recorder kept writing: the feeder voltage sags in steps for most of a minute before the light crosses, and the mains are gone while it is overhead. No fault found on the feeder. Strip kept."),Sec/3600,(Sec/60)%60));
 }
 bSkyCut=false;SkyPrelude=0;
 if(Evidence&&M&&M->Freeform&&D)
 {
  FString Record=Text.IsEmpty()?D->Finding:Text;
  if(!PhotoId.IsEmpty())Record+=TEXT("\nThe original photograph is in the album.");
  const FString Key=M->Freeform->HasFinding(Event)?Event+FString::Printf(TEXT(".day%d"),M->Freeform->Day()):Event;
  M->Freeform->RecordEvidence(Key,Record);
 }
 if(M&&M->Night&&M->Night->Glimpses&&(Event==TEXT("observer")||Event==TEXT("feeder_presence")||Event==TEXT("array_presence")||Threat))M->Night->Glimpses->ControlledRelease();
 if(Instrument(Event)&&M&&M->Freeform){M->Freeform->SetAnomaly(false);M->Freeform->SetPhenomenon(FString());}
 bCarLight=bCarTaken=false;CarStage=0;CarLightClock=-1;
 Effects->Hide();if(M&&M->Audio){M->Audio->Stop(TEXT("freeformEvent"));M->Audio->HoldRoomTone(1);}
 UE_LOG(LogS47FreeformDirector,Display,TEXT("S47: Freeform event %s ended evidence=%d"),*Event,Evidence);
 {const FS47RhythmData Plain;const FS47RhythmData& R=M&&M->Rhythm?M->Rhythm->Data():Plain;RestUntil=FMath::Max(RestUntil,ActiveTime+(Threat?R.RestDanger:R.RestOther));}
 Event.Empty();PlaceId.Empty();Stage=0;bApplied=bEffectsApplied=false;NextAt=FMath::Max(NextAt,RestUntil);Changed();
}
FString FS47FreeformDirector::Label(const FString& Id)const
{
 if(!bEnabled||Event.IsEmpty())return {};
 if(Id==TEXT("ff:event-check")&&Event==TEXT("empty_car")&&!(bCarLight&&CarStage==1))return TEXT("Inspect the unoccupied car and dashboard clock");
 if(Id==TEXT("ff:event-sample")&&Event==TEXT("landing_trace")&&!TraceRaked())return bPhoto?TEXT("Pick up the loose fragment"):TEXT("Photograph the marked surface before moving anything");
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
  Doc.Page=TEXT("RECEIVED / NO VERIFIED SENDER\n\nTO SARO NIGHT OPERATOR. YOUR LAST LOGGED OBSERVATION IS ON FILE HERE. NO FURTHER REPORT NEEDED.\n\nThe answerback is blank. The line went dead as soon as it finished printing.");
  if(M->Story)M->Story->AddDoc(Doc);if(M->UI)M->UI->Document(Doc);Finish(true);return true;
 }
 if(Id==TEXT("ff:event-check")&&Event==TEXT("empty_car")&&bCarLight&&CarStage==1)return true;
 // Gone up to it afterwards: not what the ordinary night's car shows.
 if(Id==TEXT("ff:event-check")&&Event==TEXT("empty_car")&&Near(EmptyCarReference,4)&&bCarLight&&CarStage==2)
 {
  const FString Stopped=FS47FreeformCases::Clock(FMath::Fmod(CarLightClock,86400.));
  Finish(true,FString::Printf(TEXT("The empty car in the staff bay, after a light stood over it. Roof warm, hood cold, ignition off, dashboard clock stopped at %s."),*Stopped));
  if(M->UI)M->UI->Toast(FString::Printf(TEXT("Roof warm. Hood cold. Ignition off. The dashboard clock has stopped at %s. No driver in sight."),*Stopped),6);
  return true;
 }
 if(Id==TEXT("ff:event-check")&&Event==TEXT("empty_car")&&Near(EmptyCarReference,4))
 {Finish(true);if(M->UI)M->UI->Toast(TEXT("Hood warm. Ignition off. The dashboard clock is six minutes behind the station clock. No driver in sight."),6);return true;}
 if(Id==TEXT("ff:event-sample")&&Event==TEXT("landing_trace")&&!TraceRaked()&&Near(Anchor,2.7))
 {if(!bPhoto){if(M->UI)M->UI->Toast(TEXT("Photograph it where it lies before you pick it up."),4);return true;}M->Freeform->AddItem(TEXT("fragment"),1);Finish(true);return true;}
 return false;
}
void FS47FreeformDirector::OrbLightCount(int32& On,int32& Shadowed,float& Lumens,float& Radius)const
{
 On=Shadowed=0;Lumens=Radius=0;if(!Effects)return;
 for(const auto& W:Effects->OrbLight)if(const auto* L=W.Get())if(L->IsVisible())
 {++On;Shadowed+=L->CastShadows?1:0;Lumens=FMath::Max(Lumens,L->Intensity);Radius=FMath::Max(Radius,L->AttenuationRadius);}
}
FVector FS47FreeformDirector::OrbRecordAt()const
{
 const auto* C=Effects?Effects->Orb.Get():nullptr;return C?S47Three::ToThree(C->Bounds.Origin):FVector::ZeroVector;
}
bool FS47FreeformDirector::PhysicalObjectPresented()const
{
 if(!bEnabled||!Effects)return false;
 if(Event==TEXT("dusk_orb")){const auto* C=Effects->Orb.Get();return M&&M->Web&&M->Web->Area()==Scene&&IsValid(C)&&C->IsRegistered()&&C->IsVisible()&&!C->bHiddenInGame;}
 if(Event==TEXT("close_pass"))return M&&M->Web&&M->Web->Area()==Scene&&Effects->Ufo&&Effects->Ufo->Presented();
 if(Event==TEXT("response_lights")||Event==TEXT("flyover")||Event==TEXT("close_pass")||Event==TEXT("farm_lights")||Event==TEXT("town_lights"))
  return Effects->PartPresented(1)&&Effects->PartPresented(2)&&Effects->PartPresented(3)&&(Event!=TEXT("close_pass")||Effects->PartPresented(0));
 const FS47Obj Object=Event==TEXT("landing_trace")?Effects->SampleFor(Scene):Event==TEXT("empty_car")?Effects->Car:FS47Obj();
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
// D48-20261110: the table of what is strange enough to pay. Base is dollars before quality; the week's side cap (35) still rules the sum.
// Rarer and stranger pays more: lights in the high sky 14, ground evidence 12 to 16, the Director's own incidents 20, a figure or a
// shade 20 to 28, the small grey and the aftermath 24, the pale and the tall one in the open 30 to 32, the big night 35.
// Ordinary things are not in it and pay nothing: dust devil, rabbits, cattle, tracks of the truck, the hitchhiker, cars, humour,
// lit windows, the balloon.
FString FS47FreeformDirector::StrangeGroupOf(const FString& K)
{
 if(K==TEXT("glimpse_nordic"))return TEXT("nordic");
 if(K==TEXT("glimpse_cosmic"))return TEXT("cosmic");
 if(K==TEXT("glimpse_tall"))return TEXT("tallfigure");
 if(K==TEXT("glimpse_shadow"))return TEXT("shadow");
 if(K==TEXT("glimpse_crawler"))return TEXT("crawler");
 if(K==TEXT("shade"))return TEXT("shade");
 if(K==TEXT("carcass")||K==TEXT("deadcow"))return TEXT("cattle");
 if(K==TEXT("ring")||K==TEXT("embers"))return TEXT("scorch");
 if(K==TEXT("roadtracks_low")||K==TEXT("roadtracks_tall"))return TEXT("prints");
 if(K==TEXT("bigglow_column")||K==TEXT("bigglow_fall")||K==TEXT("bigglow_rise"))return TEXT("bigglow");
 if(K==TEXT("bigmark_column")||K==TEXT("bigmark_fall")||K==TEXT("bigmark_rise"))return TEXT("bigmark");
 return FString();
}
int32 FS47FreeformDirector::StrangeBase(const FString& G)
{
 static const TPair<const TCHAR*,int32> Table[]={
  {TEXT("sky"),14},{TEXT("marks"),12},{TEXT("prints"),12},{TEXT("scorch"),14},{TEXT("cattle"),16},{TEXT("bigmark"),28},{TEXT("bigglow"),35},
  {TEXT("shade"),20},{TEXT("shadow"),22},{TEXT("tallfigure"),26},{TEXT("crawler"),24},{TEXT("nordic"),28},{TEXT("cosmic"),30},
  {TEXT("roswell"),24},{TEXT("pale"),30},{TEXT("tall"),32},{TEXT("aftermath"),24},{TEXT("cowlift"),34}};
 for(const auto& T:Table)if(G==T.Key)return T.Value;
 return 0;
}
namespace
{
 FString StrangeTitleOf(const FString& G)
 {
  static const TPair<const TCHAR*,const TCHAR*> Names[]={
   {TEXT("nordic"),TEXT("Nordic figure")},{TEXT("cosmic"),TEXT("Cosmic figure")},{TEXT("tallfigure"),TEXT("tall figure")},{TEXT("shadow"),TEXT("shadow figure")},
   {TEXT("crawler"),TEXT("low crawler")},{TEXT("shade"),TEXT("figure on the height")},{TEXT("cattle"),TEXT("dead cow")},{TEXT("scorch"),TEXT("scorched ring")},
   {TEXT("prints"),TEXT("prints on the shoulder")},{TEXT("cowlift"),TEXT("cow in the light")},{TEXT("bigglow"),TEXT("big light")},{TEXT("bigmark"),TEXT("big night marks")}};
  for(const auto& N:Names)if(G==N.Key)return N.Value;
  return G;
 }
}
int32 FS47FreeformDirector::PhotoReward(const FString& ClaimId,double Quality)
{
 if(!ClaimId.StartsWith(TEXT("strange.")))return 20;
 FString G=ClaimId.Mid(8);int32 Dot=INDEX_NONE;if(G.FindChar('.',Dot))G.LeftInline(Dot);
 const int32 Base=StrangeBase(G);if(Base<=0||!FMath::IsFinite(Quality))return 0;
 return FMath::Max(1,FMath::RoundToInt(Base*FMath::Lerp(.6,1.25,FMath::Clamp(Quality/.25,0.,1.))));
}
void FS47FreeformDirector::TagPhoto(FS47AlbumShot& Shot)
{
 TaggedShot.Empty();TaggedIncident.Empty();TaggedTitle.Empty();bTaggedLamps=false;TaggedQuality=0;TaggedRegion=FBox2D(ForceInit);PhotoSubjectMean=-1;
 if(!bEnabled||Shot.Id.IsEmpty()||Shot.bUnavailable||!M||!M->FieldCamera||!M->Web)return;
 // This hook is invoked only after a real scene capture. Controls work after
 // the encounter ends, independently of new-incident photographic claims.
 if(Shot.Pose.IsSet()&&M->Night&&M->Night->Album&&FMath::IsFinite(Shot.Clock))for(const auto& C:Claims)
 {
  if(C.ControlPlannedAt<0||!C.ControlPhotoId.IsEmpty()||Shot.Id==C.ControlOriginalId||Shot.Clock<=C.ControlPlannedAt||Event==TEXT("dusk_orb"))continue;
  const auto* Original=M->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& S){return S.Id==C.ControlOriginalId;});
  if(Original&&Original->Pose.IsSet()&&Original->Pose->Matches(Shot.Pose.GetValue()))Shot.Targets.Add({TEXT("freeform-control:")+C.Id,FVector2D(.5,.5)});
 }
 const FString RoadIncident=M->HighwayUfo?M->HighwayUfo->PhotoIncidentId():FString();
 const bool RoadPhoto=!RoadIncident.IsEmpty();
 // D48-20261110: without an incident of the Director's own in this scene the frame is still looked at for the strange things
 // that are out (below), not dropped.
 const bool bOwn=RoadPhoto||!(Event.IsEmpty()||IncidentId.IsEmpty()||!SameScene());
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
 if(!bOwn){}
 else if(!RoadPhoto&&(Event==TEXT("observer")||Event==TEXT("service_presence")||Event==TEXT("feeder_presence")||Event==TEXT("array_presence")))
 {
  if(M->Night&&M->Night->Glimpses)for(const auto& I:M->Night->Glimpses->Individuals())if(I.bRendered&&M->Night->Glimpses->ControlledPhotoBody(I.Id))
   TaggedQuality=FMath::Max(TaggedQuality,Qualify(I.Eye-FVector(0,.5,0),1.2,M->Night->Glimpses->ControlledPhotoBody(I.Id)));
 }
 else if(!RoadPhoto&&Event==TEXT("dusk_orb")&&!DuskOrb.WasPhotographed()&&PhysicalObjectPresented())
 {
  if(const auto* C=Effects->Orb.Get())TaggedQuality=Qualify(S47Three::ToThree(C->Bounds.Origin),double(C->Bounds.BoxExtent.Z)*.02,C);
 }
 else if(!RoadPhoto&&Event==TEXT("landing_trace"))
 {
  if(const auto* C=Effects->PresentedComponent(Effects->SampleFor(Scene)))TaggedQuality=Qualify(Anchor,.35,C);
 }
 else if(!RoadPhoto&&Event==TEXT("empty_car")&&bCarLight&&CarStage>=1)
 {
  // From a distance, with the light on it: on any other night the car alone is no incident to register.
  if(const auto* C=Effects->PresentedComponent(Effects->Car))TaggedQuality=Qualify(Anchor,1.4,C);
 }
 else if(RoadPhoto||Event==TEXT("flyover")||Event==TEXT("close_pass")||Event==TEXT("response_lights")||Event==TEXT("farm_lights")||Event==TEXT("town_lights"))
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

 // D48-20261110 (Tom: every photo of aliens, events and strange things pays, if it is good enough). When no incident of the
 // Director's own is in the frame, the strange things out there are: each by the same rules (in the frame, large enough, in the
 // clear, lit enough to read in Photo), one claim a group a night, the best-paying one in the frame.
 FString StrangeKey,StrangeTitle;bool bStrangeLamps=false;
 if(TaggedQuality<.025&&M->Freeform)
 {
  const int32 NightNo=FS47FreeformSession::NightOf(M->Freeform->Clock());
  double BestQ=0;int32 BestPay=0;FBox2D BestRegion(ForceInit);
  auto Offer=[&](const FString& Group,const FString& Title,bool bLamps,double Q,const FBox2D& Region,const FString& Extra=FString())
  {
   if(Q<.025)return;
   const FString Key=FString::Printf(TEXT("strange.%s.n%d%s"),*Group,NightNo,*Extra);
   const int32 Pay=PhotoReward(Key,Q);
   // A group already registered this night does not offer again; one still waiting offers its better frame.
   const auto* Have=Claims.FindByPredicate([&](const FPhotoClaim& C){return C.Id==Key;});
   if(Pay<=0||(Have&&Have->bPaid))return;
   if(Pay>BestPay||(Pay==BestPay&&Q>BestQ)){BestPay=Pay;BestQ=Q;BestRegion=Region;StrangeKey=Key;StrangeTitle=Title;bStrangeLamps=bLamps;}
  };
  // A single native body: Qualify sets the region of its bounds in TaggedRegion, read and cleared here for each.
  auto Body=[&](const FString& Group,const FString& Title,const FVector& At,double Height,const UPrimitiveComponent* C)
  {
   TaggedQuality=0;TaggedRegion=FBox2D(ForceInit);
   const double Q=C?Qualify(At,Height,C):0.;
   Offer(Group,Title,false,Q,TaggedRegion);
   TaggedQuality=0;TaggedRegion=FBox2D(ForceInit);
  };
  // Boxes of drawn props (UE centimetres), any of whose parts may stand in the line.
  auto Boxed=[&](const FString& Group,const FString& Title,const FBox& Box,const TArray<const UPrimitiveComponent*>& Parts,bool bLamps)
  {
   if(!Box.IsValid)return;
   const double Height=FMath::Max(.1,Box.GetSize().Z*.01);
   // Flat things on the ground are looked at from just above it, or the ground itself would be in the way.
   const FVector At=S47Three::ToThree(Box.GetCenter())+FVector(0,FMath::Max(0.,.35-Height*.5),0);
   const auto P=Camera->Project(At),Top=Camera->Project(At+FVector(0,Height*.5,0)),Bottom=Camera->Project(At-FVector(0,Height*.5,0));
   if(!P.bFront||!Top.bFront||!Bottom.bFront||P.U<.06||P.U>.94||Top.V<.04||Bottom.V>.96)return;
   FBox2D Bounds(ForceInit);bool Front=true;
   for(int32 I=0;I<8;++I)
   {
    const FVector Corner((I&1)?Box.Max.X:Box.Min.X,(I&2)?Box.Max.Y:Box.Min.Y,(I&4)?Box.Max.Z:Box.Min.Z);
    const auto Q=Camera->Project(S47Three::ToThree(Corner));Front&=Q.bFront;Bounds+=FVector2D(Q.U,Q.V);
   }
   if(!Front||!Bounds.bIsValid)return;
   // How large it is in the frame: its taller side, a ground mark counted by its width.
   const double Size=FMath::Clamp(FMath::Max(Bounds.Max.X-Bounds.Min.X,Bounds.Max.Y-Bounds.Min.Y),0.,1.);
   if(Size<.025)return;
   FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(FreeformPhotoSight),true);if(M->Player)Params.AddIgnoredActor(M->Player);
   if(M->Web->GetWorld()->LineTraceSingleByChannel(Hit,Camera->PhotoLoc,S47Three::ToUE(At),ECC_Visibility,Params)&&!Parts.Contains(Hit.Component.Get()))return;
   Offer(Group,Title,bLamps,Size,Bounds);
  };
  // The small ones: the glimpses' bodies, wherever they came from (the base, the cabinets, a site's cover, a window).
  if(M->Night&&M->Night->Glimpses)for(const auto& I:M->Night->Glimpses->Individuals())if(I.bRendered)
   Body(TEXT("roswell"),TEXT("grey visitor"),I.Eye-FVector(0,.5,0),1.2,M->Night->Glimpses->ControlledPhotoBody(I.Id));
  // The crab and the tall one in the open: their own skinned bodies (S47FarmSite).
  if(M->Freeform->FarmSite)if(const auto Eye=M->Freeform->FarmSite->VisitBodyEye())if(const UPrimitiveComponent* Mesh=M->Freeform->FarmSite->VisitPhotoBody())
  {
   const bool bCrab=M->Freeform->FarmSite->VisitKind()==1;
   Body(bCrab?TEXT("pale"):TEXT("tall"),bCrab?TEXT("pale visitor"):TEXT("tall visitor"),*Eye-FVector(0,bCrab?.35:.95,0),bCrab?.55:1.9,Mesh);
  }
  // PHOTO-02 (N1110): a cow lifted into the air. It pays only with the lift really up there to be seen: the craft and the beam drawn, the
  // cow clear of the grass, and the cow herself inside the frame. The craft is the body that must stand in the clear (Qualify).
  if(M->Freeform->Aftermath&&M->Freeform->Aftermath->CowLifting().Visible())
  {
   const FS47CowLift& Raised=M->Freeform->Aftermath->CowLifting();
   const auto RaisedAt=Camera->Project(Raised.CowNow());
   if(RaisedAt.bFront&&RaisedAt.U>.06&&RaisedAt.U<.94&&RaisedAt.V>.04&&RaisedAt.V<.96)Body(TEXT("cowlift"),TEXT("cow in the light"),Raised.CraftAt(),3.,Raised.Hull());
  }
  // Drawn props of the happenings: the figures on the road and the height, the dead cow, a landing's ring, prints, the big night.
  if(M->Freeform->Happenings)
  {
   TArray<FS47Happenings::FPhotoSubject> Props;M->Freeform->Happenings->PhotoSubjects(Props);
   for(const auto& P:Props)
   {
    const FString Group=StrangeGroupOf(P.Kind);if(Group.IsEmpty())continue;
    const bool bLamps=Group==TEXT("bigglow");
    Boxed(Group,StrangeTitleOf(Group),P.Box,P.Parts,bLamps);
   }
  }
  // The marks a landing left at a place (the pressed ground, the scorch): one group a night and place.
  if(M->Freeform->FarmSite)
  {
   const TArray<UPrimitiveComponent*> Marks=M->Freeform->FarmSite->PhotoMarks();
   FBox Box(ForceInit);TArray<const UPrimitiveComponent*> Parts;
   for(const UPrimitiveComponent* C:Marks){Box+=C->Bounds.GetBox();Parts.Add(C);}
   if(Parts.Num())Boxed(TEXT("marks"),TEXT("marks in the ground"),Box,Parts,false);
  }
  // The lights in the high sky (a formation, the lone track): the points drawn, each in the clear of roofs and mesas; three in the
  // frame make a formation, fewer a track.
  if(M->SkyVisitors)
  {
   const FVector Eye=S47Three::ToThree(Camera->PhotoLoc);int32 Seen=0;double L=1,R=0,T=1,B=0;
   for(const FVector& At:M->SkyVisitors->ShownPoints())
   {
    const auto P=Camera->Project(At);
    if(!P.bFront||P.U<.06||P.U>.94||P.V<.04||P.V>.96||!M->SkyVisitors->ClearLine(Eye,At))continue;
    ++Seen;L=FMath::Min(L,P.U);R=FMath::Max(R,P.U);T=FMath::Min(T,P.V);B=FMath::Max(B,P.V);
   }
   if(Seen>0)Offer(TEXT("sky"),Seen>=3?TEXT("lights in formation"):TEXT("light in the sky"),true,Seen>=3?FMath::Clamp(FVector2D(R-L,B-T).Size(),.05,1.):.03,FBox2D(ForceInit));
  }
  // What the album itself named in the frame as not to be there (its own size and distance rules): the one in the SARO jacket,
  // the ring of frost. The whole middle of the frame must be readable.
  if(Shot.bOdd&&(Shot.Text.Contains(TEXT("SARO jacket"))||Shot.Text.Contains(TEXT("ring of flattened grass"))))
   Offer(TEXT("aftermath"),TEXT("what was left out there"),false,.08,FBox2D(FVector2D(.2,.2),FVector2D(.8,.8)));
  TaggedQuality=BestQ;TaggedRegion=BestRegion;
 }
 if(TaggedQuality<.025)return;
 TaggedShot=Shot.Id;TaggedIncident=!StrangeKey.IsEmpty()?StrangeKey:RoadPhoto?RoadIncident:IncidentId;
 TaggedTitle=!StrangeKey.IsEmpty()?StrangeTitle:RoadPhoto?FString(TEXT("highway light")):Event.Replace(TEXT("_"),TEXT(" "));
 bTaggedLamps=bStrangeLamps||(StrangeKey.IsEmpty()&&(RoadPhoto||Event==TEXT("flyover")||Event==TEXT("close_pass")||Event==TEXT("response_lights")||Event==TEXT("farm_lights")||Event==TEXT("town_lights")));
 Shot.Targets.Add({TEXT("freeform:")+TaggedIncident,FVector2D(.5,.5)});
}
void FS47FreeformDirector::Photo(const FS47AlbumShot& Shot)
{
 // A body in the frame, however the picture comes out: the stores' offer of better film waits on this and nothing else.
 if(M&&M->Freeform&&M->Freeform->FarmSite&&!Shot.bUnavailable&&M->Freeform->FarmSite->VisitBodyDistance()>=0&&M->Freeform->FarmSite->VisitBodyDistance()<60&&!M->Freeform->HasFinding(TEXT("body_photo")))
  M->Freeform->RecordEvidence(TEXT("body_photo"),TEXT("A frame on the roll with something standing in it. It is on the film, whatever it comes out as."));
 if(TaggedShot!=Shot.Id||TaggedQuality<.025||Shot.bUnavailable||Shot.Url.IsEmpty()||TaggedIncident.IsEmpty())return;
 // Read the exact retained image, not a later capture or a whole-frame brightness
 // proxy. Emissive airborne lamps retain their existing three-visible-lamp rule.
 const bool Lamps=bTaggedLamps;
 if(!Lamps)
 {
  TArray<FColor> Pixels;int32 W=0,H=0;
  const double Mean=S47Image::DecodePixels(Shot.Url,Pixels,W,H)?SubjectMean(Pixels,W,H,TaggedRegion):-1;
  PhotoSubjectMean=Mean;
  if(Mean<(M->Freeform&&M->Freeform->HasKit(TEXT("film"))?8:15)){TaggedShot.Empty();if(M&&M->UI)M->UI->Toast(TEXT("Too dark or too small to make out. Try a closer, better-lit frame."),4);return;}
 }

 TaggedShot.Empty();LastPhotoAt=ActiveTime;
 if(TaggedIncident==IncidentId&&!Event.IsEmpty())
 {
  bSeen=bPhoto=true;PhotoId=Shot.Id;if(Event==TEXT("dusk_orb"))DuskOrb.Photographed(M&&M->Freeform?M->Freeform->Clock():-1.);FVector ObservedAt=Anchor;
  if(Event==TEXT("observer")&&M&&M->Night&&M->Night->Glimpses)
   for(const auto& I:M->Night->Glimpses->Individuals())if(I.bRendered){ObservedAt=I.Eye;break;}
  ObserveVisitor(ObservedAt);
 }
 auto* Existing=Claims.FindByPredicate([&](const FPhotoClaim& C){return C.Id==TaggedIncident;});
 if(!Existing){FPhotoClaim C;C.Id=TaggedIncident;C.PhotoId=Shot.Id;C.Title=TaggedTitle;C.Quality=TaggedQuality;C.Reward=PhotoReward(C.Id,C.Quality);Claims.Add(C);Existing=&Claims.Last();}
 else if(!Existing->bPaid&&TaggedQuality>Existing->Quality){Existing->PhotoId=Shot.Id;Existing->Quality=TaggedQuality;Existing->Reward=PhotoReward(Existing->Id,Existing->Quality);}
 Changed();if(M&&M->UI)M->UI->Toast(Existing->bPaid?FString(TEXT("Already registered. The new photograph stays in the album; no extra credit.")):FString::Printf(TEXT("Photograph taken. Develop it in the PHOTO LAB, then register it at OPS for $%d."),Existing->Reward),5);
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
 // M7 (Tom, 9 November): the documentation credit is field-work money: inside the week's cap (FS47FreeformSession::EarnSide)
 C->Reward=PhotoReward(C->Id,C->Quality);C->bPaid=true;const int32 Credit=M->Freeform->EarnSide(C->Reward+(M->Freeform->HasKit(TEXT("film"))?5:0),TEXT("photo credit"));
 FS47Witness::PayPhoto(M,*Shot); // N1110 PHOTO-03: a statement of the same place, backed by this photograph, pays a small bonus once
 M->Freeform->RecordEvidence(TEXT("photo-registration.")+C->Id,FString::Printf(TEXT("OPS FIELD PHOTOGRAPH RECEIPT\nIncident: %s\nRetained original: %s\nField allowance: $%d (applied to outstanding supply credit first).\nThe subject is in frame and visible. Paid once per incident."),*C->Title,*C->PhotoId,Credit));Changed();
 if(M->UI)M->UI->Toast(Credit>0?FString::Printf(TEXT("OPS registered the field photograph: $%d field allowance."),Credit):FString(TEXT("OPS registered the field photograph. This week's field allowance is used up: no payment.")),5);
 return true;
}
bool FS47FreeformDirector::RegisterFieldRecord(const FString& Id)
{
 if(Claims.ContainsByPredicate([&](const FPhotoClaim& P){return P.Id==Id;}))return RegisterPhotoClaim(Id);
 if(!M||!M->Freeform||!M->Freeform->CanRegisterPhotoEvidence()||!M->Night||!M->Night->Album)return false;
 const auto* Shot=M->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& P){return P.Id==Id;});
 if(!Shot||Shot->bUnavailable||Shot->Url.IsEmpty()||!Shot->Enlarged.IsEmpty())return false;
 if(!Shot->IsDeveloped()){if(M->UI)M->UI->Toast(TEXT("PHOTO LAB: Develop the photograph before submitting it"),5);return false;}
 const int32 Paid=M->Freeform->RegisterNightPhoto(Shot->Id,Shot->Clock,true,Shot->Text);
 if(Paid<0)return false;
 FS47Witness::PayPhoto(M,*Shot); // N1110 PHOTO-03
 Changed();
 if(M->UI)M->UI->Toast(Paid>0?FString::Printf(TEXT("OPS registered the field record: $%d field allowance."),Paid):FString(TEXT("OPS registered the field record. This week's field allowance is used up: no payment.")),5);
 return true;
}
// Public declaration: bool PlanPhotoControl(const FString& Id);
bool FS47FreeformDirector::PlanPhotoControl(const FString& Id)
{
 if(!M||!M->Freeform||!M->Freeform->CanRegisterPhotoEvidence()||!M->Night||!M->Night->Album||!Id.StartsWith(TEXT("dusk_orb.")))return false;
 auto* C=Claims.FindByPredicate([&](const FPhotoClaim& P){return P.Id==Id;});if(!C||C->ControlPlannedAt>=0)return false;
 const auto* Shot=M->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& P){return P.Id==C->PhotoId;});
 if(!Shot||Shot->bUnavailable||Shot->Url.IsEmpty()||!Shot->IsDeveloped()||!Shot->Pose.IsSet()||!Shot->Pose->Valid()||!Shot->Targets.ContainsByPredicate([&](const TPair<FString,FVector2D>& P){return P.Key==TEXT("freeform:")+Id;}))return false;
 C->ControlOriginalId=Shot->Id;C->ControlPlannedAt=M->Freeform->Clock();
 M->Freeform->RecordEvidence(TEXT("orb-return-plan.")+Id,FString::Printf(TEXT("PLANNED SAME-PLACE PHOTOGRAPH / %s\nFrozen original: %s / exposure clock %.3f s\n%s\nWhen this passage ends, take a new photograph from the same place, develop it and compare both originals at OPS. The plan itself pays nothing."),*Id,*Shot->Id,Shot->Clock,*PhotoControlHint(Id)));
 Changed();return true;
}
bool FS47FreeformDirector::PlanLocalNoiseCheck(const FString& Id)
{
 if(!M||!M->Freeform||!M->Freeform->CanRegisterPhotoEvidence()||!M->Night||!M->Night->Album||!Id.StartsWith(TEXT("dusk_orb.")))return false;
 auto* C=Claims.FindByPredicate([&](const FPhotoClaim& P){return P.Id==Id;});if(!C||C->LocalNoisePlannedAt>=0)return false;
 const auto* Shot=M->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& S){return S.Id==C->PhotoId;});
 if(!Shot||Shot->bUnavailable||Shot->Url.IsEmpty()||!Shot->IsDeveloped()||!Shot->Targets.ContainsByPredicate([&](const TPair<FString,FVector2D>& T){return T.Key==TEXT("freeform:")+Id;}))return false;
 C->LocalNoiseOriginalId=Shot->Id;C->LocalNoisePlannedAt=M->Freeform->Clock();
 const FString Original=Shot->Id,When=LightStamp(Shot->Clock); // copy before journal callbacks
 M->Freeform->RecordEvidence(TEXT("photo-local-noise-plan.")+Id,TEXT("OPTIONAL PLAN / Later local-noise check\nPhotograph ")+Original+TEXT(" / ")+When+TEXT("\nUse a loan logger during a new SARO recording. The plan itself measures nothing and does not open the loan. The return photograph is a separate check."));
 Changed();return true;
}
FString FS47FreeformDirector::LocalNoisePlanHint(const FString& Id) const
{
 if(!M||!M->Freeform||!M->Night||!M->Night->Album)return {};
 const auto* C=Claims.FindByPredicate([&](const FPhotoClaim& P){return P.Id==Id;});if(!C||C->LocalNoisePlannedAt<0)return {};
 const auto* Photo=M->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& S){return S.Id==C->LocalNoiseOriginalId;});
 FString Text=TEXT("OPTIONAL / LATER LOCAL-NOISE CHECK\n");
 Text+=Photo&&!Photo->bUnavailable&&!Photo->Url.IsEmpty()&&Photo->IsDeveloped()?TEXT("Photograph ")+Photo->Id+TEXT(" / ")+LightStamp(Photo->Clock):TEXT("The reference photograph is unavailable. Your saved idea remains; no comparison with that image is claimed.");
 Text+=TEXT("\nA later recording can describe local electrical noise. It cannot tell us what caused this earlier light. You can still repeat the photograph without the logger.");
 const auto* Field=M->Freeform->Field.Get();
 if(!Field||!Field->LoggerAccess())return Text+TEXT("\nLOAN ACCESS / Reach a supported conclusion in THE REPEATED INTERVAL before borrowing the logger. Keeping this idea does not unlock the loan.");
 return Text+LocalNoiseEquipmentHint(Field->FieldLogger(),M->Freeform->CurrentJob(),M->Freeform->ConsoleProfile());
}
FString FS47FreeformDirector::PhotoControlHint(const FString& Id) const
{
 if(!M||!M->Night||!M->Night->Album)return {};
 const auto* C=Claims.FindByPredicate([&](const FPhotoClaim& P){return P.Id==Id;});if(!C)return {};
 const FString OriginalId=C->ControlOriginalId.IsEmpty()?C->PhotoId:C->ControlOriginalId;
 const auto* S=M->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& P){return P.Id==OriginalId;});
 if(!S||S->bUnavailable||S->Url.IsEmpty())return TEXT("Frozen original unavailable. Existing records remain retained; no verified comparison can be registered.");
 if(!S->Pose.IsSet()||!S->Pose->Valid())return TEXT("Original camera position was not retained. The image can still be viewed and registered; a same-place comparison cannot be verified.");
 const auto& P=S->Pose.GetValue();const FVector Reference(4.647,0,-39.4);
 const double Metres=FVector2D::Distance({Reference.X,Reference.Z},{P.Eye.X,P.Eye.Z});
 const FString Direction=FS47FreeformObservation::Direction(Reference,P.Eye),Facing=FS47FreeformObservation::Direction(FVector::ZeroVector,P.Forward*10.);
 const double Pitch=FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(P.Forward.Y,-1.,1.)));
 FString Hint=FString::Printf(TEXT("RETURN / From S-03 CABLE JUNCTION, walk about %.0f m %s. Face %s, camera %.0f degrees %s. Match the original antenna silhouettes; a new frame must be within 1.5 m and 5 degrees of the original view."),Metres,*Direction,*Facing,FMath::Abs(Pitch),Pitch<0?TEXT("down"):TEXT("up"));
 if(M->Player&&M->Web&&M->Web->Area()==P.Scene)Hint+=FString::Printf(TEXT(" Your current distance from the original viewpoint: %.0f m."),FVector2D::Distance({M->Player->Pos.X,M->Player->Pos.Z},{P.Eye.X,P.Eye.Z}));
 return Hint;
}
bool FS47FreeformDirector::PhotoControlOriginalAvailable(const FPhotoClaim& C) const
{
 if(!M||!M->Night||!M->Night->Album)return false;
 const auto* S=M->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& P){return P.Id==C.ControlOriginalId;});
 return S&&!S->bUnavailable&&!S->Url.IsEmpty()&&S->IsDeveloped()&&S->Pose.IsSet()&&S->Pose->Valid();
}
FString FS47FreeformDirector::PhotoControlFocusId() const
{
 const auto* C=SelectPhotoReturnPlan(Claims,PhotoControlFocus,[this](const FPhotoClaim& P){return PhotoControlOriginalAvailable(P);});
 return C?C->Id:FString();
}
bool FS47FreeformDirector::FollowPhotoControl(const FString& Id)
{
 if(!M||!M->Freeform||!M->Freeform->CanRegisterPhotoEvidence())return false;
 const auto* C=SelectPhotoReturnPlan(Claims,Id,[this](const FPhotoClaim& P){return PhotoControlOriginalAvailable(P);});
 if(!C||C->Id!=Id)return false;
 if(PhotoControlFocus!=Id){PhotoControlFocus=Id;Changed();}
 return true;
}
FString FS47FreeformDirector::PhotoControlFieldHint(bool& bReady) const
{
 bReady=false;
 if(!M||!M->Freeform||!M->Freeform->Active()||!M->Night||!M->Night->Album||!M->Web||!M->Player)return {};
 const auto& Shots=M->Night->Album->Shots();
 const auto* C=SelectPhotoReturnPlan(Claims,PhotoControlFocus,[this](const FPhotoClaim& P){return PhotoControlOriginalAvailable(P);});
 const bool Pending=Claims.ContainsByPredicate([](const FPhotoClaim& P){return P.ControlPlannedAt>=0&&P.ControlPhotoId.IsEmpty();});
 if(!C)return Pending?FString(TEXT("CONTROL / ORIGINAL VIEW UNAVAILABLE; RECORDS RETAINED")):FString();
 const auto* Original=Shots.FindByPredicate([&](const FS47AlbumShot& P){return P.Id==C->ControlOriginalId;});
 auto ControlFor=[&](const FS47AlbumShot& S){return !S.bUnavailable&&!S.Url.IsEmpty()&&S.Targets.ContainsByPredicate([&](const TPair<FString,FVector2D>& T){return T.Key==TEXT("freeform-control:")+C->Id;});};
 if(Shots.ContainsByPredicate([&](const FS47AlbumShot& S){return ControlFor(S)&&S.IsDeveloped();}))return TEXT("OPS / COMPARE DEVELOPED ORIGINALS");
 if(Shots.ContainsByPredicate(ControlFor))return TEXT("PHOTO LAB / DEVELOP CONTROL ORIGINAL");
 const auto& Target=Original->Pose.GetValue();
 if(M->Web->Area()!=Target.Scene)return TEXT("RETURN TO SARO / ORIGINAL VIEWPOINT");
 const bool Raised=M->FieldCamera&&M->FieldCamera->bRaised;
 FS47PhotoPose Live;Live.Scene=M->Web->Area();Live.Fov=US47FieldCamera::PHOTO_FOV;
 // The viewfinder calls this immediately after Sync. A lowered-camera notebook
 // hint uses the current walker, never the last cached photograph-camera pose.
 Live.Eye=S47Three::ToThree(Raised?M->FieldCamera->PhotoLoc:M->Player->EyeUE());
 Live.Forward=S47Three::ToThree(Raised?M->FieldCamera->PhotoRot.Vector():M->Player->GetActorRotation().Vector()).GetSafeNormal();
 if(Event==TEXT("dusk_orb"))return TEXT("CONTROL / WAIT FOR PASSAGE TO END");
 const double Distance=FVector::Dist(Live.Eye,Target.Eye);
 if(Distance>1.5)return FString::Printf(TEXT("CONTROL POSITION / %.1f m %s"),Distance,*FS47FreeformObservation::Direction(Live.Eye,Target.Eye));
 if(!Raised)return TEXT("POSITION REACHED / RAISE CAMERA TO ALIGN");
 bReady=Target.Matches(Live);
 if(bReady)return TEXT("CONTROL VIEW MATCHED / TAKE NEW FRAME");
 const double A=FMath::Atan2(-Target.Forward.X,-Target.Forward.Z)-FMath::Atan2(-Live.Forward.X,-Live.Forward.Z);
 const double Yaw=FMath::RadiansToDegrees(FMath::Atan2(FMath::Sin(A),FMath::Cos(A)));
 const double Pitch=FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Target.Forward.Y,-1.,1.))-FMath::Asin(FMath::Clamp(Live.Forward.Y,-1.,1.)));
 if(FMath::Abs(Live.Fov-Target.Fov)>1)return TEXT("CONTROL / MATCH THE ORIGINAL LENS");
 return FString::Printf(TEXT("MATCH VIEW / %s %.0f DEG / %s %.0f DEG"),Yaw>=0?TEXT("LEFT"):TEXT("RIGHT"),FMath::Abs(Yaw),Pitch>=0?TEXT("UP"):TEXT("DOWN"),FMath::Abs(Pitch));
}
bool FS47FreeformDirector::CanRegisterPhotoControl(const FString& Id,const FString& ControlPhotoId) const
{
 if(!M||!M->Freeform||!M->Freeform->CanRegisterPhotoEvidence()||!M->Night||!M->Night->Album)return false;
 auto* C=Claims.FindByPredicate([&](const FPhotoClaim& P){return P.Id==Id;});if(!C||C->ControlPlannedAt<0||!C->ControlPhotoId.IsEmpty()||ControlPhotoId==C->ControlOriginalId)return false;
 const auto& Shots=M->Night->Album->Shots();const auto* Original=Shots.FindByPredicate([&](const FS47AlbumShot& S){return S.Id==C->ControlOriginalId;});const auto* Control=Shots.FindByPredicate([&](const FS47AlbumShot& S){return S.Id==ControlPhotoId;});
 auto Usable=[](const FS47AlbumShot* S){return S&&!S->bUnavailable&&!S->Url.IsEmpty()&&S->IsDeveloped()&&S->Pose.IsSet()&&S->Pose->Valid();};
 if(!Usable(Original)||!Usable(Control)||!FMath::IsFinite(Control->Clock)||Control->Clock<=C->ControlPlannedAt||!Original->Pose->Matches(Control->Pose.GetValue())||!Control->Targets.ContainsByPredicate([&](const TPair<FString,FVector2D>& T){return T.Key==TEXT("freeform-control:")+Id;}))return false;
 return true;
}
bool FS47FreeformDirector::RegisterPhotoControl(const FString& Id,const FString& ControlPhotoId)
{
 if(!CanRegisterPhotoControl(Id,ControlPhotoId))return false;
 auto* C=Claims.FindByPredicate([&](const FPhotoClaim& P){return P.Id==Id;});
 const auto& Shots=M->Night->Album->Shots();const auto* Original=Shots.FindByPredicate([&](const FS47AlbumShot& S){return S.Id==C->ControlOriginalId;});const auto* Control=Shots.FindByPredicate([&](const FS47AlbumShot& S){return S.Id==ControlPhotoId;});
 C->ControlPhotoId=Control->Id;
 M->Freeform->RecordEvidence(TEXT("orb-photo-comparison.")+Id,FString::Printf(TEXT("RETAINED PHOTOGRAPHIC COMPARISON / %s\nFirst original: %s / exposure clock %.3f s\nLater control: %s / exposure clock %.3f s\nBoth developed originals are retained. The later view matches the original scene, position (within 1.5 m), direction (within 5 degrees) and lens. Lighting and elapsed time can differ. No additional payment."),*Id,*Original->Id,Original->Clock,*Control->Id,Control->Clock));
 Changed();return true;
}

FString FS47FreeformDirector::ObservationNote()const
{
 if(Event==TEXT("wrong_source"))return TEXT("CONTROL NOTE: extra component persists off the expected source center. Compare independent antenna groups.");
 if(Event==TEXT("time_disagreement"))return TEXT("CONTROL NOTE: receiver and independent timing path disagree by 3.2 s. Retain both timestamps.");
 if(Event==TEXT("duplicate_print"))return TEXT("PROVENANCE NOTE: a second spool impression has a different run label. The retained original and source recording are unchanged.");
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
 Check(Catalog().Num()==22+S47SmallHints::Ids().Num()&&Definition(TEXT("roomlight_return"))&&Definition(TEXT("farm_lights"))&&Definition(TEXT("town_lights")),TEXT("authored catalogue, action-bound room light and two physical service-site encounters are embedded in runtime"));TSet<FString> Ids;
 for(const auto& D:Catalog()){Check(!Ids.Contains(D.Id)&&!D.Finding.IsEmpty()&&D.Day>=1&&D.Duration>0,TEXT("event ID, finding and finite duration"));Ids.Add(D.Id);}
 FS47FreeformDirector D(nullptr);D.Begin(nullptr);auto Saved=D.Save();FS47FreeformDirector R(nullptr);R.Begin(Saved);
 Check(S47J::Write(Saved)==S47J::Write(R.Save()),TEXT("idle director seed, budgets and schedule round-trip without reroll"));
 {
  // Critic round 5: the rhythm's danger slot and the peek, taken on night 12, come back with a load; an older save, or a new game, has neither.
  FS47FreeformDirector Took(nullptr);Took.Begin(Saved);Took.SlotNight=12;Took.PeekNight=12;
  FS47FreeformDirector Back(nullptr);Back.Begin(S47J::Read(S47J::Write(Took.Save())));
  FS47FreeformDirector Before(nullptr);Before.Begin(Saved);
  Check(Back.SlotNight==12&&Back.PeekNight==12&&Before.SlotNight==0&&Before.PeekNight==0&&!S47J::Write(Saved).Contains(TEXT("slotNight")),TEXT("the night's danger slot and the peek survive a load, and a save from before has neither"));
 }
 {
  // LOOP-1-20261109: one pointing-out a night (the bearing slip), held over a Continue; the next night has its own. The tiers: what
  // is only a hint may be pointed at in the evening, a full meeting's place not before two.
  FS47FreeformDirector Point(nullptr);Point.Begin(Saved);
  const bool bFirst=Point.PointOutLimit()==1&&Point.MayPointOut(4);Point.NotePointOut(4);
  FS47FreeformDirector Back(nullptr);Back.Begin(S47J::Read(S47J::Write(Point.Save())));
  const double Evening=3*86400.+21*3600.,Late=4*86400.+3*3600.;
  Check(bFirst&&!Point.MayPointOut(4)&&!Back.MayPointOut(4)&&Back.MayPointOut(5)&&!S47J::Write(Saved).Contains(TEXT("pointNight"))
   &&TierAllowsAt(ES47Allowed::Hint,Evening,4)&&!TierAllowsAt(ES47Allowed::Full,Evening,4)&&!TierAllowsAt(ES47Allowed::Glimpse,Evening,4)&&TierAllowsAt(ES47Allowed::Full,Late,4)&&!Point.MayPointAt(TEXT("no_such_event"),Evening,4),
   TEXT("at most one pointing-out a night, kept over a Continue and new the next night; before midnight only a hint may be pointed at"));
 }
 {
  // LOOP-4-20261109: the array's burst and a landing the array logs take the night's one pointing-out too (FS47Happenings::
  // TakePointOut): after a bearing slip the burst does not point; a burst first leaves no slip that night; the next night has its own.
  FS47FreeformDirector Night(nullptr);Night.Begin(Saved);
  Night.NotePointOut(7);
  const bool bAfterSlip=!FS47Happenings::TakePointOut(&Night,7);
  const bool bBurst=FS47Happenings::TakePointOut(&Night,8)&&!Night.MayPointOut(8)&&!FS47Happenings::TakePointOut(&Night,8);
  Check(bAfterSlip&&bBurst&&FS47Happenings::TakePointOut(&Night,9)&&FS47Happenings::TakePointOut(nullptr,9),TEXT("the array's burst and a logged landing count against the night's one pointing-out, as the bearing slip does"));
 }
 {
  // D45-20261110 (D45-26): the first evening's appointment. On the first night from 19:00 until the night's first visit has come (or
  // its window has gone); never before 19:00, never on another night, never once a visit has been; plain words, no Story.
  const double Visit=21.25*3600.;
  const FString Line=EveningLightsAt(19*3600.,0,Visit);
  Check(!Line.IsEmpty()&&Line.Contains(TEXT("around nine"))&&!EveningLightsAt(21.5*3600.,0,Visit).IsEmpty()&&EveningLightsAt(18.9*3600.,0,Visit).IsEmpty()&&EveningLightsAt(22.3*3600.,0,Visit).IsEmpty()
   &&EveningLightsAt(20*3600.,1,Visit).IsEmpty()&&EveningLightsAt(86400.+20*3600.,0,86400.+Visit).IsEmpty()&&EveningLightsAt(20*3600.,0,-1).IsEmpty()&&S47Script::LintText(Line).IsEmpty()&&!Line.Contains(TEXT("47")),
   TEXT("the first evening's note says the lights come over the dishes around nine, from 19:00 until the first visit, and only then"));
 }
 {
  // PT-FEEDER-20261109 (d37: lights at the feeder at 03:22, the big night at 03:35, the site's visit refused for the big night's slot):
  // the big night and the lights' report never share a night. Planned for night 2: night 2 is its own before it comes and after,
  // nights 1 and 3 are not; slept through or come on 3, that night. A report is the night's one pointing-out: not after a slip.
  using H=FS47Happenings;
  const bool bOwns=H::BigOwnsNight(2,2,0,true)&&!H::BigOwnsNight(1,2,0,true)&&!H::BigOwnsNight(3,2,0,true)&&H::BigOwnsNight(2,2,2,true)&&!H::BigOwnsNight(3,2,2,true)
   &&H::BigOwnsNight(3,2,3,true)&&!H::BigOwnsNight(2,2,0,false)&&!H::BigOwnsNight(2,2,-1,true)&&!H::BigOwnsNight(0,0,0,true);
  bool bApart=true;
  for(const TCHAR* L:{TEXT("town_lights"),TEXT("farm_lights")})for(const bool bPoint:{true,false})bApart&=!LightsMayCome(L,true,bPoint);
  FS47FreeformDirector Rep(nullptr);Rep.Begin(Saved);
  const bool bFree=LightsMayCome(TEXT("town_lights"),false,Rep.MayPointOut(5));Rep.NotePointOut(5);
  const bool bSpent=!LightsMayCome(TEXT("town_lights"),false,Rep.MayPointOut(5))&&LightsMayCome(TEXT("farm_lights"),false,Rep.MayPointOut(5))&&LightsMayCome(TEXT("town_lights"),false,Rep.MayPointOut(6));
  Check(bOwns&&bApart&&bFree&&bSpent,TEXT("the big night and the lights' report never overlap: the big night owns its night, before and after it comes, and a report is the night's one pointing-out"));
  Check(FeederMayFail(false)&&!FeederMayFail(true),TEXT("FEEDER-LEAD-20261110: no feeder fault starts on a night with a lead drawn or owned by the big night"));
 }
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
 Check(!S47J::Obj(Saved,TEXT("mystery")),TEXT("An idle director writes nothing for the night's mystery"));
 FS47FreeformDirector MysteryModel(nullptr);MysteryModel.Begin(nullptr);MysteryModel.RollNightChoice(2,2,1.f,MysteryModel.MysteryRollDay,MysteryModel.PendingMysteryNight);
 MysteryModel.MysteryAfter=23.5;MysteryModel.MysterySeen=1;MysteryModel.MysteryClue=1;MysteryModel.MysteryAge=2.5;MysteryModel.MysteryClearAt=200000;
 FS47FreeformDirector MysteryCopy(nullptr);MysteryCopy.Begin(S47J::Read(S47J::Write(MysteryModel.Save())));
 Check(MysteryCopy.MysteryRollDay==2&&MysteryCopy.PendingMysteryNight==2&&MysteryCopy.MysteryAfter==23.5&&MysteryCopy.MysterySeen==1&&MysteryCopy.MysteryNoticeClue()==1&&MysteryCopy.MysteryNoticeAge()==2.5&&MysteryCopy.MysteryTrailOpen(),TEXT("The night's mystery, its hour, what has been seen and a notice under way survive Continue"));
 {const TArray<TPair<FString,bool>> Rows=MysteryCopy.MysteryTrailRows();
  Check(Rows.Num()==2&&Rows[0].Key.Contains(TEXT("remote feeder"))&&!Rows[0].Value&&Rows[1].Value&&MysteryCopy.MysteryTrailHint()==Rows[0].Key,TEXT("PT-NOTEBOOK: the trail's rows: the chart seen is ticked off, the feeder still open"));
  FS47FreeformDirector Both(nullptr);Both.Begin(nullptr);Both.MysterySeen=3;Both.MysteryClearAt=200000;
  Check(Both.MysteryTrailRows().Num()==2&&Both.MysteryTrailRows()[0].Value&&Both.MysteryTrailRows()[1].Value&&Both.MysteryTrailHint().IsEmpty(),TEXT("PT-NOTEBOOK: the feeder visited stays on the note ticked off until the trail clears, not gone"));
  Both.MysteryClearAt=0;Check(Both.MysteryTrailRows().IsEmpty(),TEXT("PT-NOTEBOOK: the trail's rows go with the trail at noon"));}
 Check(MysteryCopy.ExpireNightChoices(2,false)&&MysteryCopy.PendingMysteryNight==0&&MysteryCopy.MysteryTrailOpen()&&MysteryCopy.MysteryRollDay==2,TEXT("Morning ends a drawn mystery not yet met, and does not touch a trail already found"));
 MysteryCopy.Begin(Saved);Check(MysteryCopy.MysteryRollDay==0&&MysteryCopy.MysteryNoticeClue()==-1&&!MysteryCopy.MysteryTrailOpen(),TEXT("A save from before the mystery has it still to come"));
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

 // D48-20261110: the table of what is photographed -> what it pays. Pure: not native image or render qualification.
 {
  const TCHAR* const Strange[]={TEXT("glimpse_nordic"),TEXT("glimpse_cosmic"),TEXT("glimpse_tall"),TEXT("glimpse_shadow"),TEXT("glimpse_crawler"),TEXT("shade"),TEXT("carcass"),TEXT("deadcow"),
   TEXT("ring"),TEXT("embers"),TEXT("roadtracks_low"),TEXT("roadtracks_tall"),TEXT("bigglow_column"),TEXT("bigglow_fall"),TEXT("bigglow_rise"),TEXT("bigmark_column"),TEXT("bigmark_fall"),TEXT("bigmark_rise")};
  bool bAllPay=true;for(const TCHAR* K:Strange){const FString G=StrangeGroupOf(K);bAllPay&=!G.IsEmpty()&&StrangeBase(G)>0&&PhotoReward(TEXT("strange.")+G+TEXT(".n1"),.1)>0;}
  Check(bAllPay,TEXT("D48: every strange prop kind (figures, dead cow, ring, prints, the big night) maps to a group that pays when photographed"));
  const TCHAR* const Ordinary[]={TEXT("devil"),TEXT("rabbits"),TEXT("cattle"),TEXT("tracks"),TEXT("hitcher"),TEXT("pickup"),TEXT("rv"),TEXT("truck"),TEXT("darkcar"),TEXT("hairy"),TEXT("singer"),TEXT("zet"),TEXT("window"),TEXT("litwin"),TEXT("balloon"),TEXT("")};
  bool bNonePay=true;for(const TCHAR* K:Ordinary){const FString G=StrangeGroupOf(K);bNonePay&=G.IsEmpty()&&PhotoReward(TEXT("strange.")+G+TEXT(".n1"),.5)==0;}
  Check(bNonePay&&PhotoReward(TEXT("strange.balloon.n2"),.5)==0&&PhotoReward(TEXT("strange.dust_devil.n2"),.5)==0,TEXT("D48: dust devil, animals, cars, people, humour, lit windows and the balloon pay nothing"));
  const TCHAR* const Groups[]={TEXT("sky"),TEXT("marks"),TEXT("prints"),TEXT("scorch"),TEXT("cattle"),TEXT("bigmark"),TEXT("bigglow"),TEXT("shade"),TEXT("shadow"),TEXT("tallfigure"),TEXT("crawler"),TEXT("nordic"),TEXT("cosmic"),TEXT("roswell"),TEXT("pale"),TEXT("tall"),TEXT("aftermath"),TEXT("cowlift")};
  bool bScale=true;for(const TCHAR* G:Groups)
  {
   const FString Id=FString(TEXT("strange."))+G+TEXT(".n4");const int32 Base=StrangeBase(G),Low=PhotoReward(Id,.03),High=PhotoReward(Id,.4);
   bScale&=Base>0&&Low>=FMath::RoundToInt(Base*.6)-1&&Low<High&&High==FMath::RoundToInt(Base*1.25)&&High<=44;
  }
  Check(bScale,TEXT("D48: every group pays 0.6 to 1.25 of its base by quality (just legible to large in frame)"));
  Check(StrangeBase(TEXT("tall"))>=StrangeBase(TEXT("pale"))&&StrangeBase(TEXT("pale"))>StrangeBase(TEXT("roswell"))&&StrangeBase(TEXT("roswell"))>StrangeBase(TEXT("shade"))&&StrangeBase(TEXT("shade"))>StrangeBase(TEXT("sky"))&&StrangeBase(TEXT("bigglow"))>=StrangeBase(TEXT("tall")),TEXT("D48: the rarer and stranger, the more it pays: lights in the sky least, then a figure, the small grey, the pale and the tall one, the big night most"));
  Check(PhotoReward(TEXT("observer.day3.7"),.03)==20&&PhotoReward(TEXT("dusk_orb.n2"),.9)==20&&PhotoReward(TEXT("strange.tall.n1"),std::numeric_limits<double>::quiet_NaN())==0,TEXT("D48: the Director's own incidents still pay their fixed 20; a strange claim with no usable quality pays nothing"));
  FS47FreeformDirector Wonder(nullptr);Wonder.Begin(nullptr);
  FPhotoClaim A;A.Id=TEXT("strange.tall.n2");A.PhotoId=TEXT("album9");A.Title=TEXT("tall visitor");A.Quality=.2;Wonder.Claims.Add(A);
  FS47FreeformDirector StrangeCopy(nullptr);StrangeCopy.Begin(S47J::Read(S47J::Write(Wonder.Save())));
  Check(StrangeCopy.Claims.Num()==1&&StrangeCopy.Claims[0].Reward==PhotoReward(A.Id,.2)&&StrangeCopy.Claims[0].Reward>StrangeCopy.PhotoReward(TEXT("strange.roswell.n2"),.2),TEXT("D48: a strange claim's pay follows its quality after Continue without a new save field; one claim a group a night keeps its id"));
 }
 // Pure selection/serialization fixtures: not native image/render qualification.
 FS47FreeformDirector FocusModel(nullptr);FocusModel.Begin(nullptr);
 FPhotoClaim FA;FA.Id=TEXT("dusk_orb.focus-a");FA.PhotoId=FA.ControlOriginalId=TEXT("original-a");FA.Quality=.5;FA.ControlPlannedAt=100;FA.bPaid=true;
 FPhotoClaim FB=FA;FB.Id=TEXT("dusk_orb.focus-b");FB.PhotoId=FB.ControlOriginalId=TEXT("original-b");FB.ControlPlannedAt=200;
 FocusModel.Claims={FA,FB};const auto BeforeFocus=FocusModel.Save();
 auto AllAvailable=[](const FPhotoClaim&){return true;};
 auto Selected=[&](const FS47FreeformDirector& D){const auto* C=SelectPhotoReturnPlan(D.Claims,D.PhotoControlFocus,AllAvailable);return C?C->Id:FString();};
 Check(Selected(FocusModel)==FA.Id,TEXT("legacy photo focus defaults to first pending available return plan"));
 FocusModel.PhotoControlFocus=FB.Id;
 Check(Selected(FocusModel)==FB.Id,TEXT("second pending photo plan can guide without completing the oldest"));
 FS47FreeformDirector FocusCopy(nullptr);FocusCopy.Begin(S47J::Clone(FocusModel.Save()));
 Check(FocusCopy.PhotoControlFocus==FB.Id&&Selected(FocusCopy)==FB.Id&&FocusCopy.Claims.Num()==2&&FocusCopy.Claims[0].bPaid&&FocusCopy.Claims[1].bPaid,TEXT("selected return-plan ID survives Continue with both paid original records"));
 auto AfterFocus=FocusCopy.Save();BeforeFocus->RemoveField(TEXT("photoControlFocus"));AfterFocus->RemoveField(TEXT("photoControlFocus"));
 Check(FJsonValue::CompareEqual(FJsonValueObject(BeforeFocus),FJsonValueObject(AfterFocus)),TEXT("photo focus selection preserves all non-focus history and payment state"));
 FocusCopy.PhotoControlFocus=TEXT("missing-plan");Check(Selected(FocusCopy)==FA.Id,TEXT("missing saved focus safely falls back to first pending plan"));
 FocusCopy.PhotoControlFocus=FB.Id;const auto* AvailableFallback=SelectPhotoReturnPlan(FocusCopy.Claims,FB.Id,[&](const FPhotoClaim& C){return C.Id!=FB.Id;});
 Check(AvailableFallback&&AvailableFallback->Id==FA.Id,TEXT("unavailable focused original falls back without erasing the planned record"));
 FocusCopy.Claims[1].ControlPhotoId=TEXT("retained-control-b");
 Check(Selected(FocusCopy)==FA.Id&&FocusCopy.Claims.Num()==2&&FocusCopy.Claims[1].ControlPhotoId==TEXT("retained-control-b")&&FocusCopy.Claims[1].bPaid,TEXT("completed focused plan yields guidance to pending plan and retains completed history"));
 FocusCopy.Claims[0].ControlPhotoId=TEXT("retained-control-a");Check(Selected(FocusCopy).IsEmpty()&&FocusCopy.Claims.Num()==2,TEXT("all completed return plans stop field guidance without deleting original pairs"));
 auto NoiseState=FocusModel.Save();auto NoiseClaims=S47J::Arr(NoiseState,TEXT("photoClaims"));
 if(NoiseClaims.Num())
 {
  S47J::Set(NoiseClaims[0]->AsObject(),TEXT("localNoiseOriginal"),TEXT("frozen-noise-reference"));S47J::Set(NoiseClaims[0]->AsObject(),TEXT("localNoisePlannedAt"),123.);S47J::Set(NoiseClaims[0]->AsObject(),TEXT("photo"),TEXT("newer-best-claim-image"));
  FS47FreeformDirector NoiseCopy(nullptr);NoiseCopy.Begin(NoiseState);const auto Round=S47J::Arr(NoiseCopy.Save(),TEXT("photoClaims"));
  Check(Round.Num()==NoiseClaims.Num()&&NoiseCopy.Claims[0].LocalNoiseOriginalId==TEXT("frozen-noise-reference")&&NoiseCopy.Claims[0].LocalNoisePlannedAt==123.&&NoiseCopy.Claims[0].PhotoId==TEXT("newer-best-claim-image")&&NoiseCopy.Claims[1].LocalNoisePlannedAt<0&&NoiseCopy.Claims[0].bPaid==FocusModel.Claims[0].bPaid,TEXT("optional local-noise idea preserves frozen source and plan time without changing payment"));
 }

 FS47FieldLogger HintLogger;HintLogger.Site=1;HintLogger.bRecording=true;HintLogger.Frequency=1420.11;FS47FieldLogger::FTape HT;HT.Start=100;HT.End=120;HT.Frequency=1420.11;HT.Site=1;HintLogger.Tapes.Add(HT);
 FS47FreeformSession::FJob HJ;HJ.Id=TEXT("hint-job");HJ.Stage=FS47FreeformSession::EJobStage::Chart;FS47Profile HP{};HP.F=1420.11;
 const auto ChartHint=LocalNoiseEquipmentHint(HintLogger,&HJ,&HP);
 Check(ChartHint.Contains(TEXT("SITE B"))&&ChartHint.Contains(TEXT("Elapsed interval"))&&ChartHint.Contains(TEXT("Planned finish"))&&ChartHint.Contains(TEXT("printer"))&&!ChartHint.Contains(TEXT("separate later window"))&&!ChartHint.Contains(TEXT("try SITE A")),TEXT("existing B recording has observed/expected times, never claims later-photo simultaneity, and finished SARO card goes to printer"));
 HJ.Stage=FS47FreeformSession::EJobStage::Integrating;const auto ActiveHint=LocalNoiseEquipmentHint(HintLogger,&HJ,&HP);
 Check(ActiveHint.Contains(TEXT("Frequencies match"))&&ActiveHint.Contains(TEXT("acquisition is running"))&&!ActiveHint.Contains(TEXT("tune the returned logger")),TEXT("active matching acquisition never directs player to retune deployed logger"));
 HP.F=1420.2;Check(LocalNoiseEquipmentHint(HintLogger,&HJ,&HP).Contains(TEXT("Frequencies differ")),TEXT("frequency mismatch offers no fabricated acquisition overlap"));
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
 {
  // M7 (B2): the photo lab is about ten metres from the service door's anchor, through walls, indoors. No line from there,
  // for the figure, the scrape or the lights; outside in earshot with nothing between, the line goes with the sound.
  const double Lab=FVector2D::Distance(FVector2D(17.25,-1.9),FVector2D(11.9,-11));
  Check(!SaysAloud(Lab,25,false,true,true)&&!SaysAloud(Lab,20,false,true,false)&&!SaysAloud(Lab,25,false,true,true),TEXT("from inside the photo lab neither the figure at the fence, the scrape at the service door nor the lights come with a line"));
  Check(SaysAloud(8,25,true,false,true)&&SaysAloud(8,20,true,false,false)&&SaysAloud(8,20,true,true,false)&&!SaysAloud(8,25,true,true,true)&&!SaysAloud(30,25,true,false,true),TEXT("near, outside and with nothing between, the line goes with the sound; the figure's only to someone outside"));
  const FString W1=LightsTip(0,TEXT("A rancher on the county road"),TEXT("the Baca lease")),W4=LightsTip(3,TEXT("A rancher on the county road"),TEXT("the Baca lease"));
  bool Differ=true;for(int32 Wa=0;Wa<4;++Wa)for(int32 Wb=Wa+1;Wb<4;++Wb)Differ&=LightsTip(Wa,TEXT("x"),TEXT("y"))!=LightsTip(Wb,TEXT("x"),TEXT("y"));
  Check(W1==TEXT("A rancher on the county road reports lights low over the Baca lease.")&&W4.Contains(TEXT("the Baca lease"))&&W4.StartsWith(TEXT("A rancher"))&&Differ&&LightsTip(9,TEXT("x"),TEXT("y"))==LightsTip(3,TEXT("x"),TEXT("y")),TEXT("the lights on the radio are put differently each week, by the same callers, about the same places"));
  Check(HourText(2*86400.+20.5*3600.)==TEXT("20:30")&&HourText(-5)==TEXT("00:00"),TEXT("what is left at an antenna is dated by the hours of the visit"));
 }
 Ok=FS47DuskOrb::SelfTest(Lines)&&Ok;
 return Ok;
}

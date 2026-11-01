#include "S47FieldRadio.h"
#include "S47Main.h"
#include "S47Inter.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47Story.h"
#include "S47World.h"
#include "S47Flow.h"
#include "S47UI.h"
#include "S47Night.h"
#include "S47Glimpses.h"
#include "S47Spine.h"
#include "S47Horror.h"
#include "S47HorrorThreat.h"
#include "S47DevEvents.h"
#include "S47Three.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

namespace { const TCHAR* Pickup=TEXT("saroFieldRadio"); }

FS47FieldRadio::~FS47FieldRadio()
{
 Sound.Reset();
 for(auto C:Copies) if(C.IsValid()) C->DestroyComponent();
 if(Hit.IsValid()) Hit->DestroyComponent();
}
void FS47FieldRadio::Begin(const S47J::FObj& Saved)
{
 Stop();
 bUnsupported=Saved && S47J::Num(Saved,TEXT("v"))!=1;
 Unknown=bUnsupported?S47J::Clone(Saved):nullptr;
 bOwned=!bUnsupported && S47J::Bool(Saved,TEXT("owned"));
 bAvailable=true;
}
void FS47FieldRadio::Stop()
{
 Sound.Reset(); LastLevel=QuietFor=SeenFor=ThreatFor=0; LastCue=-1; bAvailable=false;
 if(Root) Root.SetVisible(false);
 if(Hit.IsValid()) Hit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
S47J::FObj FS47FieldRadio::Save() const
{
 if(bUnsupported) return S47J::Clone(Unknown);
 auto O=S47J::New(); S47J::Set(O,TEXT("v"),1); S47J::Set(O,TEXT("owned"),bOwned); return O;
}
bool FS47FieldRadio::Build()
{
 if(Root) return true;
 if(!M || !M->Web || !M->Web->IsLoaded() || !M->Inter) return false;
 AS47Web* W=M->Web.Get();
 const FS47Obj Room=W->Obj(TEXT("room.group")), Source=W->Obj(TEXT("world.room6.objs.walkie"));
 if(!Room || !Source) return false;
 const FS47WebNode* Parent=W->Node(Room.I);
 const FS47WebNode* Original=W->Node(Source.I);
 if(!Parent || !Original || !Parent->Comp.IsValid() || !Original->Comp.IsValid()) return false;
 const auto SourceChildren=Original->Children;
 // The three already-exported own meshes are shared, not moved or edited.
 TArray<UStaticMeshComponent*> Parts;
 for(int32 Id:SourceChildren)
  if(const auto* N=W->Node(Id);N && N->Comp.IsValid())
   if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get());C && C->GetStaticMesh()) Parts.Add(C);
 if(Parts.Num()!=3) return false;
 auto* Box=NewObject<UBoxComponent>(W,TEXT("SaroFieldRadio"));
 Box->SetupAttachment(Parent->Comp.Get()); Box->SetMobility(EComponentMobility::Movable);
 Box->SetBoxExtent(FVector(6,18,6)); Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 Box->SetCollisionResponseToAllChannels(ECR_Ignore); Box->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
 Box->SetGenerateOverlapEvents(false); Box->SetCanEverAffectNavigation(false);
 W->AddInstanceComponent(Box); Box->RegisterComponent();
 Root=W->ObjAt(W->AdoptNode(Box,Room.I,TEXT("saro:fieldRadio"))); Hit=Box;
 Root.SetPosition(FVector(3.18,.840,2.70)); Root.SetRotation(FVector(0,.2,0));
 for(int32 I=0;I<Parts.Num();++I)
 {
  auto* C=NewObject<UStaticMeshComponent>(W,*FString::Printf(TEXT("SaroFieldRadioPart%d"),I));
  C->SetupAttachment(Box); C->SetMobility(EComponentMobility::Movable); C->SetStaticMesh(Parts[I]->GetStaticMesh());
  for(int32 K=0;K<Parts[I]->GetNumMaterials();++K) C->SetMaterial(K,Parts[I]->GetMaterial(K));
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCollisionResponseToAllChannels(ECR_Ignore);
  C->SetCanEverAffectNavigation(false); C->SetCastShadow(true);
  W->AddInstanceComponent(C); C->RegisterComponent();
  // AdoptNode composes identity rotation/scale: restore the complete source pose afterwards.
  const int32 Id=W->AdoptNode(C,Root.I,FString::Printf(TEXT("saro:fieldRadio:%d"),I));
  const FS47Obj Old=W->ObjAt(SourceChildren[I]); FS47Obj Copy=W->ObjAt(Id);
  Copy.SetPosition(Old.Position()+FVector(0,-.06,0)); Copy.SetQuaternion(Old.Quaternion()); Copy.SetScale(Old.Scale());
  Copies.Add(C);
 }
 TWeakObjectPtr<US47Main> Weak=M;
 FS47Interactable It; It.Id=Pickup; It.Object=Root; It.Range=2.2;
 It.Label=[Weak]{return Weak.IsValid() && Weak->FieldRadio && !Weak->FieldRadio->Owned()?TEXT("TAKE FIELD RADIO"):FString();};
 It.Use=[Weak]{if(Weak.IsValid() && Weak->FieldRadio) Weak->FieldRadio->Take();};
 It.Examine=[] {return TEXT("SARO field set. Battery fitted, channel marked. Small enough to keep with me.");};
 M->Inter->Add(It);
 for(auto C:Copies) if(C.IsValid()) C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 return true;
}
void FS47FieldRadio::Take()
{
 if(bOwned || !bAvailable || bUnsupported || !Root || !Root.Shown() || !M || !M->Player) return;
 if(FVector::Distance(M->Player->Pos,Root.WorldPosition())>2.4) return;
 bOwned=true; Root.SetVisible(false);
 if(Hit.IsValid()) Hit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 if(M->UI) M->UI->Toast(TEXT("FIELD RADIO TAKEN // I'll keep it with me."),3);
 M->RequestAutosave();
}
double FS47FieldRadio::Proximity(const FVector& Eye,const FPresence& In)
{
 if(!In.bActive) return 0;
 const double D=FVector::Distance(Eye,In.At);
 // Broad, smooth uncertainty, not a rangefinder. Quiet above eighteen metres.
 const double K=FMath::Clamp((18.-D)/15.,0.,1.);
 return K*K*(3.-2.*K);
}
bool FS47FieldRadio::InView(const FVector& At) const
{
 if(!M || !M->Player || !M->Web) return false;
 const FVector From=M->Player->EyeUE(), To=S47Three::ToUE(At);
 auto* PC=M->Web->GetWorld()->GetFirstPlayerController(); FVector2D Screen; int32 Width=0,Height=0;
 if(!PC || !PC->ProjectWorldLocationToScreen(To,Screen)) return false;
 PC->GetViewportSize(Width,Height);
 if(Width<=0 || Height<=0 || Screen.X<0 || Screen.Y<0 || Screen.X>Width || Screen.Y>Height) return false;
 FHitResult H; FCollisionQueryParams Q(SCENE_QUERY_STAT(S47FieldRadioSight),true);
 // A hit at the creature itself is allowed; a nearer opaque wall is not.
 return !M->Web->GetWorld()->LineTraceSingleByChannel(H,From,To,ECC_Visibility,Q) || H.Distance>FVector::Distance(From,To)-45.;
}
TArray<FS47FieldRadio::FPresence> FS47FieldRadio::Nearby() const
{
 TArray<FPresence> Out;
 auto Add=[&](const FVector& At,bool Hunt){Out.Add({At,true,InView(At),Hunt});};
 if(M->Night && M->Night->Glimpses)
 {
  const auto G=M->Night->Glimpses;
  for(const auto& A:G->Individuals())
   Add(A.Eye,A.State==FS47RoswellBrain::EState::Hunt || A.State==FS47RoswellBrain::EState::Contact);
  if(G->Visible() && G->Individuals().IsEmpty()) Add(G->Position()+FVector(0,.85,0),false);
 }
 if(M->Spine)
 {
  const auto& A=M->Spine->Secure();
  if(A.BodyShown() && A.Body()!=FS47SC04::EBody::Away)
   Add(A.BodyHead(),A.Body()==FS47SC04::EBody::Chase || A.Body()==FS47SC04::EBody::Grab);
  const auto& B=M->Spine->Searched();
  if(B.BodyShown() && B.Body()!=FS47SC11::EBody::Away)
   Add(B.BodyHead(),B.Body()==FS47SC11::EBody::Chase || B.Body()==FS47SC11::EBody::Grab);
 }
 if(M->Horror && M->Horror->Threat)
 {
  const auto& A=*M->Horror->Threat;
  if(A.Active() && A.Appeared() && !A.Gone() && !A.Withdrawn())
   Add(A.EyePosition(),A.State()==FS47HorrorThreat::EState::Chase || A.State()==FS47HorrorThreat::EState::Capture);
 }
 if(M->DevEvents) if(auto At=M->DevEvents->PalePresence()) Add(*At,false);
 return Out;
}
void FS47FieldRadio::Step(double Dt)
{
 if(!M) return;
 Build();
 const bool Play=bAvailable && !bUnsupported && M->Mode==US47Main::EMode::Play;
 const bool Shown=Play && !bOwned && M->Web && M->Web->Area()==TEXT("saro");
 if(Root) Root.SetVisible(Shown);
 if(Hit.IsValid()) Hit->SetCollisionEnabled(Shown?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
 const bool Held=!Play || M->bPausedByMenu || M->ModalOpen() || !M->Story || M->Story->IsPaused() || M->Story->IsCinematic();
 Sound.Hold(Held);
 if(Held || !M->Player) return;
 const FVector Eye=S47Three::ToThree(M->Player->EyeUE());
 double Level=0; bool Visible=false,Hunting=false;
 for(const auto& A:Nearby())
 {
  const double K=Proximity(Eye,A); Level=FMath::Max(Level,K);
  Visible|=K>.04 && A.bVisible; Hunting|=K>.04 && A.bHunting;
 }
 LastLevel=Level;
 // Require stable changes and a genuine quiet gap before another presence cue.
 QuietFor=Level<.015?QuietFor+Dt:0;
 SeenFor=Visible?SeenFor+Dt:0; ThreatFor=Level>.04?ThreatFor+Dt:0;
 if(QuietFor>8) LastCue=-1;
 int32 Want=Hunting?2:SeenFor>.25?1:ThreatFor>1.1?0:-1;
 if(Want>LastCue && Want>=0 && Sound.Cue(M->Audio,FS47EncounterSound::EKind(Want),{},FMath::Max(.3,Level))) LastCue=Want;
 Sound.UpdateRadio(M->Audio,bOwned?Level:0,{},false);
 ControlTest(Dt);
}
void FS47FieldRadio::ControlTest(double Dt)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47FieldRadioContract")) || TestStage<0) return;
 TestT+=Dt;
 if(TestT<2 || !Ready()) return;
 auto Check=[&](bool B,const TCHAR* Text){UE_LOG(LogTemp,Display,TEXT("S47: field radio %s: %s"),B?TEXT("PASS"):TEXT("FAIL"),Text);if(!B){TestStage=-1;FPlatformMisc::RequestExitWithStatus(false,1);}return B;};
 if(TestStage==0)
 {
  if(!Check(!Owned() && Copies.Num()==3 && Root.Shown(),TEXT("three source-mesh copies are on the SARO desk"))) return;
  const auto* Old=M->Web->Node(M->Web->Obj(TEXT("world.room6.objs.walkie")).I);
  if(!Check(Old && Old->Children.Num()==3,TEXT("Room 6 original remains separate and complete")))return;
  if(!Check(LastLevel==0,TEXT("quiet room does not warn about dormant or future enemies")))return;
  M->Player->Place(3.18,3.7,0); M->Player->Pitch=-.65;
  FString Shot;
  if(FParse::Value(FCommandLine::Get(),TEXT("S47FieldRadioShot="),Shot))
   FScreenshotRequest::RequestScreenshot(Shot,true,false);
  TestStage=1; TestT=0; return;
 }
 if(TestStage==1)
 {
  if(TestT<3) return;
  const FString Pick=M->Inter->Pick(M->Player->EyeUE(),(S47Three::ToUE(Root.WorldPosition())-M->Player->EyeUE()).GetSafeNormal());
  if(!Check(Pick==Pickup,TEXT("ordinary interaction trace reaches the desk radio")))return;
  M->Inter->UseId(Pick);
  if(!Check(Owned() && !Root.Shown() && Hit->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("normal pickup removes visible body and query collision")))return;
  const auto Saved=M->Flow->GameState();
  if(!Check(S47J::Bool(S47J::Obj(Saved,TEXT("fieldRadio")),TEXT("owned")),TEXT("normal Flow snapshot contains ownership")))return;
  Begin(S47J::Obj(Saved,TEXT("fieldRadio")));
  if(!Check(Owned(),TEXT("Continue restores ownership without respawning a second radio")))return;
  Begin(nullptr);
  if(!Check(!Owned(),TEXT("older case without radio field can still collect the radio")))return;
  Begin(S47J::Obj(Saved,TEXT("fieldRadio")));
  if(!Check(M->Night && M->Night->Glimpses && M->Night->Glimpses->DevSpawn(1,FS47RoswellBrain::EMode::Stand)==1,TEXT("explicit fixture creates one actual nearby Roswell")))return;
  M->Player->Yaw+=PI; M->Player->Pitch=0;
  TestStage=2;TestT=0;return;
 }
 if(TestStage==2)
 {
  if(TestT<2) return;
  const auto Actors=Nearby();
  if(!Check(Actors.Num()>0 && LastLevel>.04,TEXT("active creature behind the player produces carried interference")))return;
  bool Seen=false;for(const auto& A:Actors)Seen|=A.bVisible;
  if(!Check(!Seen,TEXT("off-screen presence is not mislabelled as visual discovery")))return;
  M->Night->Glimpses->DevClear(); TestStage=3;TestT=0;return;
 }
 if(TestStage==3)
 {
  if(TestT<1) return;
  if(!Check(LastLevel==0,TEXT("clearing the actual creature returns radio interference to zero")))return;
  FString Tour;
  if(FParse::Value(FCommandLine::Get(),TEXT("S47EncounterTour="),Tour))
   if(!Check(FS47EncounterSound::RenderTour(Tour),TEXT("six-second original sound preview written")))return;
  UE_LOG(LogTemp,Display,TEXT("S47: field radio PASS complete")); TestStage=-1; FPlatformMisc::RequestExitWithStatus(false,0);
 }
}
bool FS47FieldRadio::SelfTest(TArray<FString>& Lines)
{
 bool Ok=true;
 auto C=[&](bool B,const TCHAR* Text){Lines.Add(FString::Printf(TEXT("%s field radio: %s"),B?TEXT("ok"):TEXT("FAIL"),Text));Ok&=B;};
 FPresence A; A.At=FVector(0,0,4);
 C(Proximity(FVector::ZeroVector,A)==0,TEXT("dormant/future creatures never trigger interference"));
 A.bActive=true;
 const double Near=Proximity(FVector::ZeroVector,A); A.At.Z=12;
 C(Near>Proximity(FVector::ZeroVector,A) && Near<1,TEXT("smooth proximity strengthens towards a real creature"));
 A.At.Z=19; C(Proximity(FVector::ZeroVector,A)==0,TEXT("distant presence is silent"));
 FS47FieldRadio R(nullptr); R.Begin(nullptr); C(!R.Owned(),TEXT("old and new cases leave pickup available"));
 auto S=R.Save();S47J::Set(S,TEXT("owned"),true);R.Begin(S);C(R.Owned(),TEXT("ownership survives serialize/restore"));
 S47J::Set(S,TEXT("v"),99); R.Begin(S); C(!R.Owned() && S47J::Write(R.Save())==S47J::Write(S),TEXT("unknown future version is preserved and inactive"));
 return Ok;
}

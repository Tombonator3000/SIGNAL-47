#include "S47FieldRadio.h"
#include "S47Freeform.h"
#include "S47GearFlicker.h"
#include "S47FarmSite.h"
#include "S47IndoorDread.h"
#include "S47Taken.h"
#include "S47Main.h"
#include "S47Audio.h"
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
#include "S47CB.h"
#include "S47HudGauges.h"
#include "S47Keys.h"
#include "S47Predecessor.h"
#include "S47TripoNew.h"
#include "S47Props.h"
#include "S47DaleVoice.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/Parse.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

namespace { const TCHAR* Pickup=TEXT("saroFieldRadio"); }

// WALKIE-USE-20261108 and CB-WALKIE-20261108: the carried set's own sounds, apart from the CB's horn voice and long hiss tails
// (S47CB.cpp): a two-tone call when Dale hails, a dry key click with a short burst of squelch, and a roger beep at the end of each
// over. Made here from the game's own noise and tones; nothing recorded.
namespace S47FieldRadioSet
{
 constexpr double HailSeconds=18,HailAgainSeconds=8,MessageSeconds=3.42;
 void Squelch(US47Audio* Audio,double After,double Seconds,double Gain)
 {
  if(!Audio||!Audio->Ctx.IsValid()||!Audio->SfxBus.IsValid())return;
  const TSharedPtr<FS47AudioBuffer> Hiss=Audio->NoiseBuffer();if(!Hiss.IsValid())return;
  FS47AudioContext::FBatch Batch(*Audio->Ctx);
  const double From=Audio->Ctx->CurrentTime()+After;
  auto Src=Audio->Ctx->CreateBufferSource();Src->Buffer=Hiss;
  auto Hp=Audio->Ctx->CreateBiquadFilter();Hp->SetType(TEXT("highpass"));Hp->Frequency.SetValue(900);
  auto Lp=Audio->Ctx->CreateBiquadFilter();Lp->SetType(TEXT("lowpass"));Lp->Frequency.SetValue(3600);
  auto Amp=Audio->Ctx->CreateGain();
  Amp->Gain.SetValueAtTime(0,From);Amp->Gain.LinearRampToValueAtTime(Gain,From+.012);Amp->Gain.SetValueAtTime(Gain,From+Seconds*.6);Amp->Gain.LinearRampToValueAtTime(0,From+Seconds);
  Src->Connect(Hp);Hp->Connect(Lp);Lp->Connect(Amp);Amp->Connect(Audio->SfxBus);
  Src->Start(From,FMath::FRand()*.5);Src->Stop(From+Seconds+.05);
 }
 void Call(US47Audio* Audio)
 {
  if(!Audio)return;
  for(const double ToneAt:{0.,.34}){Audio->Beep(1450.f,.07f,.05f,float(ToneAt));Audio->Beep(1900.f,.07f,.05f,float(ToneAt+.1));}
 }
 void Roger(US47Audio* Audio,double After)
 {
  if(!Audio)return;
  Audio->Beep(1750.f,.05f,.04f,float(After),TEXT("sine"));Audio->Beep(1320.f,.06f,.04f,float(After+.06),TEXT("sine"));
 }
 void Click(US47Audio* Audio){if(Audio)Audio->Sfx(TEXT("click"),FS47Sound{.Gain=.3f});}
 // M7-WALKIE-20261109: the same two-tone call from the set lying on the desk, placed there and muffled by its own small speaker
 // face down on the wood, with a breath of squelch after it. Heard through the room's walls as anything placed is.
 void DeskCall(US47Audio* Audio,const FVector& At)
 {
  if(!Audio||!Audio->Ctx.IsValid()||!Audio->SfxBus.IsValid())return;
  const TSharedPtr<FS47AudioNode> Out=Audio->Spatial(At,1.5f);if(!Out.IsValid())return;
  FS47AudioContext::FBatch Batch(*Audio->Ctx);
  const double From=Audio->Ctx->CurrentTime()+.05;
  auto Lp=Audio->Ctx->CreateBiquadFilter();Lp->SetType(TEXT("lowpass"));Lp->Frequency.SetValue(1700);Lp->Connect(Out);
  for(const double ToneAt:{0.,.34})for(const double Hz:{1450.,1900.})
  {
   const double T=From+ToneAt+(Hz>1500?.1:0);
   auto O=Audio->Ctx->CreateOscillator();O->SetType(TEXT("square"));O->Frequency.SetValue(Hz);
   auto G=Audio->Ctx->CreateGain();G->Gain.SetValueAtTime(0,T);G->Gain.LinearRampToValueAtTime(.09,T+.005);G->Gain.SetValueAtTime(.09,T+.07);G->Gain.LinearRampToValueAtTime(0,T+.09);
   O->Connect(G);G->Connect(Lp);O->Start(T);O->Stop(T+.12);
  }
  if(const TSharedPtr<FS47AudioBuffer> Hiss=Audio->NoiseBuffer();Hiss.IsValid())
  {
   const double T=From+.62;
   auto Src=Audio->Ctx->CreateBufferSource();Src->Buffer=Hiss;
   auto Hp=Audio->Ctx->CreateBiquadFilter();Hp->SetType(TEXT("highpass"));Hp->Frequency.SetValue(900);
   auto G=Audio->Ctx->CreateGain();G->Gain.SetValueAtTime(0,T);G->Gain.LinearRampToValueAtTime(.05,T+.015);G->Gain.SetValueAtTime(.05,T+.25);G->Gain.LinearRampToValueAtTime(0,T+.45);
   Src->Connect(Hp);Hp->Connect(G);G->Connect(Lp);Src->Start(T,FMath::FRand()*.5);Src->Stop(T+.5);
  }
 }
}

double FS47FieldRadio::ChannelMHz(int32 Channel){return 49.830+.015*(FMath::Clamp(Channel,1,Channels)-1);}
FString FS47FieldRadio::Head(int32 Channel,const FString& Who)
{
 const FString SetName=FString::Printf(TEXT("WALKIE %d"),FMath::Clamp(Channel,1,Channels));
 return Who.IsEmpty()?SetName:SetName+TEXT(", ")+Who;
}
FString FS47FieldRadio::ChannelLine() const
{
 const FString Freq=FString::Printf(TEXT("channel %d, %.3f"),Chan,ChannelMHz(Chan));
 if(Chan==1)return Freq+TEXT(". SARO's own, and Dale's.");
 if(Chan==2)return Freq+TEXT(". The district's service band.");
 return Freq+TEXT(". Nobody's.");
}

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
 bLeftOut=!bUnsupported && S47J::Bool(Saved,TEXT("leftOut"));
 // The channel it was left on (WALKIE-USE-20261108); 1 for a save from before there were others.
 Chan=Saved && !bUnsupported?FMath::Clamp(S47J::Int(Saved,TEXT("ch"),1),1,Channels):1;
 bAvailable=true;
}
void FS47FieldRadio::Stop()
{
 Sound.Reset(); LastLevel=QuietFor=SeenFor=ThreatFor=LastRadio=CrackleIn=GearNoise=OrdHiss=OrdCrackle=RealNear=0; MusicNow=EMusicTier::Place; LastCue=-1; StrengthNow=StrengthWant=1; StrengthIn=0; SinceNoise=SinceCrackle=1e9; CrackleRun=0; bAvailable=false; LastSeen=LastHunt=false; SinceMessage=1e9;
 // Nothing half said carries over to another game or the title.
 if(Voice)Voice->Stop(); bCaption=false;
 HailLeft=HailAgainIn=TxLeft=0; ThenIn=-1; HailCount=0; Then=nullptr; HailAnswer=nullptr; HailMissed=nullptr; SinceLoud=SinceTouch=1e9; HeldTips.Reset(); bBandPending=false;
 if(Root) Root.SetVisible(false);
 if(Hit.IsValid()) Hit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
S47J::FObj FS47FieldRadio::Save() const
{
 if(bUnsupported) return S47J::Clone(Unknown);
 auto O=S47J::New(); S47J::Set(O,TEXT("v"),1); S47J::Set(O,TEXT("owned"),bOwned); if(bLeftOut) S47J::Set(O,TEXT("leftOut"),true);
 if(Chan!=1) S47J::Set(O,TEXT("ch"),Chan);
 return O;
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
 // TRIPO-NEW (Tom's walkie of 9 November): his own model on the desk when this build has it, standing on its foot with its face to
 // the room (-Z); else the three web parts as before. -S47TripoNewOff keeps the old one, for a picture to compare.
 // PT-WALKIE-20261109 (Tom): the one on the desk is his better walkie (S47TripoNew::DeskWalkie, SM_Tripo_Walkie), as it was first;
 // "pakke tripo3d-1" is a Geiger counter (S47Happenings' "geiger"). Prepared standing .34 m with its face toward -Y in Blender (Three
 // +Z): turned half round to face the room, at its own height from its bounds.
 bTripoWalkie=false;
 if(const S47TripoNew::FModel* Own=S47TripoNew::DeskWalkie();Own&&S47TripoNew::Exists(Own->Path)&&!FParse::Param(FCommandLine::Get(),TEXT("S47TripoNewOff")))
  if(UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,Own->Path))
  {
   const double TallCm=Mesh->GetBounds().BoxExtent.Z*2.;
   const double Scale=TallCm>1.?FMath::Clamp(S47TripoNew::DeskWalkieHeightM*100./TallCm,.2,2.):1.;
   auto* C=NewObject<UStaticMeshComponent>(W,TEXT("SaroFieldRadioTripo"));
   C->SetupAttachment(Box); C->SetMobility(EComponentMobility::Movable); C->SetStaticMesh(Mesh);
   C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCollisionResponseToAllChannels(ECR_Ignore);
   C->SetCanEverAffectNavigation(false); C->SetCastShadow(true);
   W->AddInstanceComponent(C); C->RegisterComponent();
   FS47Obj Copy=W->ObjAt(W->AdoptNode(C,Root.I,TEXT("saro:fieldRadio:tripo")));
   Copy.SetPosition(FVector(0,-.06,0)); Copy.SetRotation(FVector(0,PI,0)); Copy.SetScale(FVector(Scale));
   Copies.Add(C); bTripoWalkie=true;
   UE_LOG(LogTemp,Display,TEXT("S47: field radio: Tom's Tripo walkie on the desk (%s, %.0f cm as made, scale %.2f)"),Own->Path,TallCm,Scale);
  }
 for(int32 I=0;I<Parts.Num()&&!bTripoWalkie;++I)
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
 It.Label=[Weak]{return Weak.IsValid() && Weak->FieldRadio && !Weak->FieldRadio->Owned() && !Weak->FieldRadio->LeftOut()?TEXT("TAKE FIELD RADIO"):FString();};
 It.Use=[Weak]{if(Weak.IsValid() && Weak->FieldRadio) Weak->FieldRadio->Take();};
 It.Examine=[] {return TEXT("SARO field set. Battery in, channel marked. Small enough to keep on me.");};
 M->Inter->Add(It);
 for(auto C:Copies) if(C.IsValid()) C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 return true;
}
// FIX-20261110 (Tom, point 8): where the set lies when nobody carries it. Looked for once a second (the props are placed after
// the web, and a Story game after a Freeform one has it back on the desk); the place is the dresser's own top, from its bounds.
void FS47FieldRadio::Rest(double Dt)
{
 HallRetry-=Dt;if(!Root||HallRetry>0||!M)return;
 HallRetry=1;
 const UStaticMeshComponent* Dresser=M->IsFreeform()?AS47Props::PlacedMesh(TEXT("hallDresser")):nullptr;
 const bool bWant=Dresser!=nullptr;
 if(bWant==bHall)return;
 bHall=bWant;
 if(bHall)
 {
  const FBox B=Dresser->Bounds.GetBox();const FVector C=S47Three::ToThree(B.GetCenter());
  Root.SetPosition(FVector(C.X+.34,B.Max.Z/100.+.06,C.Z-.03));Root.SetRotation(FVector(0,PI-.25,0)); // its face to the corridor (+Z)
  UE_LOG(LogTemp,Display,TEXT("S47: field radio: on the hall dresser (%.2f %.2f %.2f)"),C.X+.34,B.Max.Z/100.+.06,C.Z-.03);
 }
 else{Root.SetPosition(FVector(3.18,.840,2.70));Root.SetRotation(FVector(0,.2,0));}
}
bool FS47FieldRadio::Message(const FString& Text,bool bDeskToo)
{
 // NIGHT-20261110: a set never taken off the SARO desk is still on, and is heard in the building (the big night's Dale, play n18).
 if(!M||bLeftOut||(!bOwned&&!(bDeskToo&&OnDesk()&&InSaroBuilding(M))))return false;
 // CB-WALKIE-20261108: the walkie is Dale's. His come on channel 1 only, under the set's own caption head; a voice that is too
 // broken to make out comes on whatever channel it is on.
 FString Who,Said=Text;
 if(Text.StartsWith(TEXT("DALE: "))){if(Chan!=1)return false;Who=TEXT("DALE");Said=Text.Mid(6);}
 ++MessageCount; SinceMessage=0; SinceLoud=0;
 // DALE-WIRE-20261109: what he said, whole, is what his takes are looked up by.
 const FString Whole=Said;
 // GEAR-FLICKER-20261108: near something, words of his are lost in the noise, and the set crackles over them.
 if(!Who.IsEmpty()&&M->IsFreeform()&&M->Gear.IsValid())
 {
  Said=M->Gear->Broken(Said,MessageCount);
  if(Said!=Whole&&M->Audio&&Sound.Crackle(M->Audio,FMath::Max(.4,GearNoise)))++Crackled;
 }
 // DALE-WIRE-20261109: his own voice when the bank has a take for it (or for each of his parts of it, one after another; a part with
 // a place or a bearing in it stays text). The set's synthetic chatter only when nothing of his is played. The crackle above lies
 // over his words where they break up; the squelch opens and shuts round him and the roger beep comes after his last take.
 bool bVoiced=false;
 FS47DaleVoice::FPlan Plan;
 if(!Who.IsEmpty()&&M->IsFreeform()&&M->Audio)
 {
  if(!Voice)Voice=MakeShared<FS47DaleVoice>(M->Audio,TEXT("walkie"));
  // A caption of his still waiting for its first word is not lost under this one.
  if(bCaption){Voice->Stop();StepCaption();}
  Plan=Voice->PlanWalkie(Whole);
  bVoiced=Plan.Voiced()>0&&Voice->Play(Plan);
 }
 const bool bHeard=bVoiced||(M->Audio&&Sound.Message(M->Audio));
 if(bVoiced){S47FieldRadioSet::Squelch(M->Audio,0,.16,.08);S47FieldRadioSet::Squelch(M->Audio,Plan.Seconds+.02,.22,.1);}
 if(bHeard)S47FieldRadioSet::Roger(M->Audio,bVoiced?Plan.Seconds+.05:S47FieldRadioSet::MessageSeconds);
 // Anything but Dale is the set itself breaking up: it is only heard (the broken voice and the squelch), never told in words.
 // A caption is for words somebody says.
 if(M->UI&&!Who.IsEmpty())
 {
  const FString Line=Head(Chan,Who)+TEXT(": ")+Said;
  const float Reading=FMath::Clamp(3.f+Said.Len()/18.f,6.f,11.f);
  // With his voice: up at his first word, down at his last (takes.json), on the sound's clock. Words of it that are only text
  // (a report he passes on) keep the time to read them.
  if(bVoiced){bCaption=true;bCaptionAll=Plan.All();CaptionLine=Line;CaptionStart=Plan.SpeechStart;CaptionEnd=Plan.SpeechEnd;CaptionReading=Reading;StepCaption();}
  else{bCaption=false;M->UI->Toast(Line,Reading);}
 }
 UE_LOG(LogTemp,Display,TEXT("S47: walkie message on channel %d, sound %d, voice %d of %d: %s"),Chan,bHeard,bVoiced?Plan.Voiced():0,Plan.Parts.Num(),*Text);
 return true;
}
bool FS47FieldRadio::DaleSpeaking() const{return Voice&&Voice->Playing();}
bool FS47FieldRadio::DaleOnAir(const US47Main* M){return M&&M->FieldRadio.IsValid()&&M->FieldRadio->DaleSpeaking();}
void FS47FieldRadio::StepCaption()
{
 if(!bCaption)return;
 if(!M||!M->UI){bCaption=false;return;}
 const double At=Voice?Voice->Clock():-1.;
 // Before his first word, wait for it; cut off before it (another line, the channel turned), it is shown now for its reading time.
 if(At>=0&&At<CaptionStart)return;
 bCaption=false;
 const TFunction<double()> Clock=Voice&&At>=0?Voice->CaptionClock():TFunction<double()>();
 const double For=!Clock?CaptionReading:FMath::Max(.01,FMath::Max(CaptionEnd-At,bCaptionAll?0.:CaptionReading-(At-CaptionStart)));
 M->UI->Toast(CaptionLine,float(For),Clock);
}

// ---------- the player's own hand on it (WALKIE-USE-20261108) ----------
void FS47FieldRadio::BindKeys()
{
 if(!M)return;
 bKeysBound=true;
 TWeakObjectPtr<US47Main> WeakMain=M;
 M->OnAction(TEXT("walkie"),[WeakMain]{return WeakMain.IsValid()&&WeakMain->FieldRadio.IsValid()&&WeakMain->FieldRadio->PressTalk();});
 M->OnAction(TEXT("channel"),[WeakMain]
 {
  if(!WeakMain.IsValid()||!WeakMain->FieldRadio.IsValid()||WeakMain->Mode!=US47Main::EMode::Play||WeakMain->ModalOpen())return false;
  WeakMain->FieldRadio->NextChannel();return true;
 });
}
void FS47FieldRadio::NextChannel()
{
 if(!bOwned||bLeftOut){if(M&&M->UI)M->UI->Toast(TEXT("No walkie on you to turn."),3);return;}
 Chan=Chan%Channels+1;SinceTouch=0;
 // DALE-WIRE-20261109: his voice is on channel 1, and goes with the turn of the knob.
 if(Voice)Voice->Stop();
 // A look at the service band still to come goes with the turn of the knob; an answer still to come does not.
 if(bBandPending){Then=nullptr;ThenIn=-1;bBandPending=false;}
 if(!M)return;
 if(M->Audio){S47FieldRadioSet::Click(M->Audio);S47FieldRadioSet::Squelch(M->Audio,.05,.18,.08);}
 if(M->UI)M->UI->Toast(Head(Chan)+TEXT(": ")+ChannelLine(),4);
 UE_LOG(LogTemp,Display,TEXT("S47: walkie turned to channel %d"),Chan);
 // The service band: anything on it, out here? A moment, as the squelch settles.
 if(Chan==2&&ThenIn<0&&M->IsFreeform()&&M->Freeform&&M->Freeform->Predecessor)
 {
  Then=[this]
  {
   if(!M||Chan!=2||!M->Freeform||!M->Freeform->Predecessor)return;
   const FString OnBand=M->Freeform->Predecessor->ServiceBand();
   if(!OnBand.IsEmpty())Carrier(OnBand);
  };
  ThenIn=1.4;bBandPending=true;
 }
}
void FS47FieldRadio::KeyUp(const FString& Says)
{
 TxLeft=1.5;SinceTouch=0;SinceLoud=0;++MessageCount;
 if(M&&M->Audio){S47FieldRadioSet::Click(M->Audio);S47FieldRadioSet::Squelch(M->Audio,1.36,.22,.12);S47FieldRadioSet::Roger(M->Audio,1.3);}
 if(M&&M->UI)M->UI->Toast(FString(TEXT("YOU: "))+Says,2.6f);
 UE_LOG(LogTemp,Display,TEXT("S47: walkie keyed on channel %d"),Chan);
}
bool FS47FieldRadio::PressTalk()
{
 if(!M||M->Mode!=US47Main::EMode::Play||M->bPausedByMenu||M->ModalOpen())return false;
 if(M->Story&&(M->Story->IsPaused()||M->Story->IsCinematic()))return false;
 if(!bOwned||bLeftOut)
 {
  if(M->UI)M->UI->Toast(bLeftOut?TEXT("The walkie isn't on you. It's out there somewhere."):*FString::Printf(TEXT("No walkie on you. SARO's field set is %s."),Where()),4);
  return true;
 }
 if(HailLeft>0){Answer();return true;}
 // (keyed before the look at the service band: the call itself listens there)
 if(bBandPending){Then=nullptr;ThenIn=-1;bBandPending=false;}
 // Already on the air, or waiting for what comes back.
 if(TxLeft>0||ThenIn>=0)return true;
 KeyUp(Chan==1?TEXT("SARO night to Dale. Come in."):TEXT("Anybody on this channel?"));
 Then=[this]{Reply();};ThenIn=FMath::FRandRange(1.9f,3.f);
 return true;
}
FS47FieldRadio::EBack FS47FieldRadio::BackFor(bool bSomethingNear,float Roll)
{
 // Neither is sure: a far car on 49 MHz sounds like something under the hiss, and something near does not always answer.
 const float Voice=bSomethingNear?.7f:.2f;
 return FMath::Clamp(Roll,0.f,1.f)<Voice?EBack::Voice:EBack::Squelch;
}
void FS47FieldRadio::Carrier(const FString& OnBand)
{
 // An open carrier: the squelch held open, a thin steady whine under it. What the player makes of it is the band's own reading.
 if(M&&M->Audio){S47FieldRadioSet::Squelch(M->Audio,0,2.4,.05);M->Audio->Beep(1020.f,2.2f,.010f,.05f,TEXT("sine"));}
 if(M&&M->UI)M->UI->Toast(Head(2)+TEXT(": ")+OnBand,8);
 SinceTouch=0;
}
void FS47FieldRadio::Reply()
{
 if(!M||!bOwned||bLeftOut)return;
 SinceTouch=0;
 // Something near is breaking up the set: what comes back is not Dale. Only heard, never told; and not always.
 if(RealNear>.05)
 {
  if(BackFor(true,FMath::FRand())==EBack::Voice)
  {
   ++MessageCount;SinceLoud=0;
   if(M->Audio)Sound.Message(M->Audio);
  }
  else if(M->Audio)S47FieldRadioSet::Squelch(M->Audio,0,.5,.1);
  UE_LOG(LogTemp,Display,TEXT("S47: walkie called on channel %d with a presence near: not Dale"),Chan);
  return;
 }
 FS47Predecessor* Dale=M->IsFreeform()&&M->Freeform?M->Freeform->Predecessor.Get():nullptr;
 if(Chan==1&&Dale)
 {
  const FString Back=Dale->AnswerWalkie();
  if(!Back.IsEmpty()&&Message(Back))return;
 }
 if(Chan==2&&Dale)
 {
  const FString OnBand=Dale->ServiceBand();
  if(!OnBand.IsEmpty()){Carrier(OnBand);return;}
 }
 // Nobody: the squelch opens on nothing and shuts again; now and then a far voice on the channel that is nobody's business.
 if(BackFor(false,FMath::FRand())==EBack::Voice&&M->Audio&&Sound.Message(M->Audio)){++MessageCount;SinceLoud=0;}
 else if(M->Audio)S47FieldRadioSet::Squelch(M->Audio,0,.5,.1);
 UE_LOG(LogTemp,Display,TEXT("S47: walkie called on channel %d: nobody"),Chan);
}
bool FS47FieldRadio::Hail(const FHail& Words,TFunction<void()> OnAnswer,TFunction<void()> OnMissed)
{
 if(!bOwned){DeskCall(Words);return false;}
 // Not while the player is on the air, or waiting for what comes back: that call gets its answer first.
 // (Nor over his own voice still on the set, DALE-WIRE-20261109: he finishes first.)
 if(!M||!bOwned||bLeftOut||Chan!=1||HailLeft>0||TxLeft>0||ThenIn>=0||DaleSpeaking()||Words.First.IsEmpty())return false;
 HailWords=Words;HailAnswer=MoveTemp(OnAnswer);HailMissed=MoveTemp(OnMissed);
 HailLeft=S47FieldRadioSet::HailSeconds;HailAgainIn=S47FieldRadioSet::HailAgainSeconds;HailCount=1;SinceTouch=0;
 if(M->Audio)S47FieldRadioSet::Call(M->Audio);
 Message(FString(TEXT("DALE: "))+Words.First);
 // The first few times, what answers it.
 if(HintsShown<3&&M->UI){++HintsShown;M->UI->Toast(FString::Printf(TEXT("%s answers the walkie."),*S47Keys::Label(TEXT("walkie"))),5);}
 UE_LOG(LogTemp,Display,TEXT("S47: walkie: Dale hails on channel 1"));
 return true;
}
void FS47FieldRadio::DeskCall(const FHail& Words)
{
 // Nothing on the desk to call from: the set is out where the player was taken, or there is no hail at all.
 if(bLeftOut||Words.First.IsEmpty())return;
 const int32 Day=M&&M->IsFreeform()&&M->Freeform?M->Freeform->Day():0;
 if(DeskDay==Day)return;
 if(M)
 {
  // Only where it can be heard: the set is built on the desk and the player is within earshot of it.
  if(!Root||!M->Player)return;
  const FVector At=Root.WorldPosition();
  if(FVector::Distance(S47Three::ToThree(M->Player->EyeUE()),At)>DeskHearing)return;
  if(M->Audio)S47FieldRadioSet::DeskCall(M->Audio,At);
  if(DeskHints<3&&M->UI){++DeskHints;M->UI->Toast(bHall?TEXT("Something on the hall dresser gives two beeps and a hiss."):TEXT("Something on the control room desk gives two beeps and a hiss."),5);}
 }
 DeskDay=Day;++DeskCallCount;
 UE_LOG(LogTemp,Display,TEXT("S47: walkie: Dale's hail heard from the desk, day %d"),Day);
}
void FS47FieldRadio::Answer()
{
 const TFunction<void()> Said=MoveTemp(HailAnswer);
 HailLeft=HailAgainIn=0;HailAnswer=nullptr;HailMissed=nullptr;
 KeyUp(TEXT("Go ahead, Dale."));
 Then=Said;ThenIn=1.6;
 UE_LOG(LogTemp,Display,TEXT("S47: walkie: Dale's hail answered"));
}
void FS47FieldRadio::EndHail(bool bHeardGiveUp)
{
 const TFunction<void()> Missed=MoveTemp(HailMissed);
 HailLeft=HailAgainIn=0;HailAnswer=nullptr;HailMissed=nullptr;
 if(bHeardGiveUp&&!HailWords.GiveUp.IsEmpty())Message(FString(TEXT("DALE: "))+HailWords.GiveUp);
 UE_LOG(LogTemp,Display,TEXT("S47: walkie: Dale's hail %s"),bHeardGiveUp?TEXT("went unanswered"):TEXT("was not heard"));
 if(Missed)Missed();
}
void FS47FieldRadio::StepHand(double Dt)
{
 SinceLoud+=Dt;SinceTouch+=Dt;TxLeft=FMath::Max(0.,TxLeft-Dt);
 if(!bKeysBound)BindKeys();
 if(HailLeft>0)
 {
  // The set gone from hand, or turned off his channel: he is not heard, and he does not know why.
  if(!bOwned||bLeftOut||Chan!=1)EndHail(false);
  else
  {
   HailLeft-=Dt;HailAgainIn-=Dt;
   if(HailCount<2&&HailAgainIn<=0&&!HailWords.Again.IsEmpty()){++HailCount;if(M&&M->Audio)S47FieldRadioSet::Call(M->Audio);Message(FString(TEXT("DALE: "))+HailWords.Again);}
   if(HailLeft<=0)EndHail(true);
  }
 }
 if(ThenIn>=0)
 {
  ThenIn-=Dt;
  if(ThenIn<0){const TFunction<void()> Do=MoveTemp(Then);Then=nullptr;bBandPending=false;if(Do)Do();}
 }
 // A report that waited: said by Dale on the walkie once he is free and the set is on his channel, or let go after a while.
 for(FHeldTip& Held:HeldTips)Held.Age+=Dt;
 HeldTips.RemoveAll([](const FHeldTip& Held){return Held.Age>HeldTipSeconds;});
 if(!HeldTips.IsEmpty()&&ThenIn<0&&TxLeft<=0&&Deliver(HeldTips[0].Tip))HeldTips.RemoveAt(0);
}

// ---------- the CB and the walkie apart (CB-WALKIE-20261108) ----------
bool FS47FieldRadio::CbAtHand(const US47Main* M)
{
 // CB-TRUCK-20261109: under the dash, and only with the player in the seat.
 return M&&CbHeardAt(M->World&&M->World->bDriving,false,false);
}
bool FS47FieldRadio::InSaroBuilding(const US47Main* M)
{
 if(!M||(M->World&&M->World->bDriving))return false;
 if(!M->Web||!M->Player||M->Web->Area()!=TEXT("saro"))return false;
 for(const TCHAR* ZoneName:{TEXT("room.bounds"),TEXT("annex.zone.southDoor"),TEXT("annex.zone.corridor"),TEXT("annex.zone.recordsDoor"),TEXT("annex.zone.records")})
  if(M->Web->Rect(ZoneName).Contains(M->Player->Pos))return true;
 return false;
}
bool FS47FieldRadio::CbSay(US47Main* M,const TArray<FString>& Lines,const FString& Voice,TFunction<void()> OnBegun)
{
 // CB-TRUCK-20261109: nothing is heard on a CB the player is not sitting at.
 if(!M||Lines.IsEmpty()||!CbAtHand(M))return false;
 if(!M->CB.IsValid()&&IsValid(M->Audio))M->CB=MakeShared<FS47CB>(M->Audio);
 if(!M->CB.IsValid()||M->CB->bBusy)return false;
 // MSG-ONE-20261109: not over Dale's voice on the walkie; the caller tries again (as when the CB is busy).
 if(DaleOnAir(M))return false;
 TArray<FS47CBLine> Call;
 for(const FString& Said:Lines){FS47CBLine Each;Each.Text=Said;Each.Who=Voice;Each.By=TEXT("CB, a caller");Call.Add(Each);}
 // In the cab, from the set under the dash (FS47CB::Call: unset is in the cab with the player).
 TOptional<FVector> From;
 TWeakObjectPtr<US47Main> WeakMain=M;
 const TSharedRef<bool> bFirst=MakeShared<bool>(true);
 const double Took=M->CB->Call(Call,From,[WeakMain,OnBegun,bFirst](const FS47CBLine& Line,double Secs)
 {
  if(*bFirst){*bFirst=false;if(OnBegun)OnBegun();}
  if(WeakMain.IsValid()&&WeakMain->UI)WeakMain->UI->Toast(TEXT("CB: ")+Line.Text,float(Secs)+1.5f);
 });
 return Took>0;
}
void FS47FieldRadio::Report(US47Main* M,const FString& Tip)
{
 if(!M||Tip.IsEmpty())return;
 if(M->FieldRadio.IsValid()&&M->FieldRadio->Deliver(Tip))return;
 // Nobody to hear it on yet: it waits for Dale on the walkie, free and on his channel.
 if(!M->FieldRadio.IsValid())return;
 auto& Held=M->FieldRadio->HeldTips;
 if(Held.Num()>=3)Held.RemoveAt(0);
 Held.Add({Tip,0.});
 UE_LOG(LogTemp,Display,TEXT("S47: report waits for a CB or the walkie: %s"),*Tip);
}
bool FS47FieldRadio::Deliver(const FString& Tip)
{
 if(!M||M->ModalOpen())return false;
 // The county has rung it through to Dale's bench, and he passes it on in his own words, on his channel. A summary like that is
 // never read out on the CB (the CB is the callers' own voices, FS47Happenings::Cb): while Dale is busy or the walkie is off his
 // channel, it waits for him.
 FS47Predecessor* Dale=M->IsFreeform()&&M->Freeform?M->Freeform->Predecessor.Get():nullptr;
 const FString Relay=Dale?Dale->RelayLine(Tip):FString();
 // (Nor over his own voice still on the set: DALE-WIRE-20261109.)
 if(Relay.IsEmpty()||HailLeft>0||DaleSpeaking()||!Message(Relay))return false;
 UE_LOG(LogTemp,Display,TEXT("S47: report passed on by Dale over the walkie: %s"),*Tip);
 return true;
}

void FS47FieldRadio::Talk(const FVector& At){if(M&&M->Audio)Sound.Talk(M->Audio,At);}

void FS47FieldRadio::Take()
{
 if(bOwned || bLeftOut || !bAvailable || bUnsupported || !Root || !Root.Shown() || !M || !M->Player) return;
 if(FVector::Distance(M->Player->Pos,Root.WorldPosition())>2.4) return;
 bOwned=true; Root.SetVisible(false);
 if(Hit.IsValid()) Hit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 if(M->UI) M->UI->Toast(FString::Printf(TEXT("FIELD RADIO TAKEN // I'll keep it with me. %s talks on it, %s turns the channel."),*S47Keys::Label(TEXT("walkie")),*S47Keys::Label(TEXT("channel"))),5);
 M->RequestAutosave();
}
double FS47FieldRadio::Proximity(const FVector& Eye,const FPresence& In,double Reach)
{
 if(!In.bActive) return 0;
 const double D=FVector::Distance(Eye,In.At);
 // Broad, smooth uncertainty, not a rangefinder. Full three metres off, quiet at its reach.
 const double K=FMath::Clamp((Reach-D)/(Reach-3.),0.,1.);
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
 auto Add=[&](const FVector& At,bool Hunt,bool Shown=true){Out.Add({At,true,Shown && InView(At),Hunt});};
 if(M->Night && M->Night->Glimpses)
 {
  const auto G=M->Night->Glimpses;
  for(const auto& A:G->Individuals())
   Add(A.Eye,A.State==FS47RoswellBrain::EState::Hunt || A.State==FS47RoswellBrain::EState::Contact,A.bRendered);
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
 // D45-20261110: F10's look-only walks are heard as any body is (they never moved the music before).
 if(M->DevEvents) for(const FVector& GaitHead:M->DevEvents->GaitPresence()) Add(GaitHead,false);
 if(M->IsFreeform()&&M->Freeform)
 {
  if(M->Freeform->FarmSite)
  {
   // D45-20261110: hunting only while it walks at the player near by; standing off it was the chase's music at 34 m.
   if(auto At=M->Freeform->FarmSite->VisitBodyEye()) Add(*At,M->Freeform->FarmSite->VisitBodyComing());
   // No body yet: it breaks up on the place itself. Heard, never seen.
   if(auto At=M->Freeform->FarmSite->VisitWarningAt()) Add(*At,false,false);
  }
  if(M->Freeform->Taken) if(auto At=M->Freeform->Taken->RadioAt()) Add(*At,false,false);
  if(M->Freeform->IndoorDread) if(auto At=M->Freeform->IndoorDread->PeekEye()) Add(*At,false);
 }
 return Out;
}
double FS47FieldRadio::ObservedThreat(const FVector& Eye,const TArray<FPresence>& Visible)
{
 double Level=0;
 for(const auto& A:Visible)
  if(A.bActive && A.bVisible && !A.At.ContainsNaN() && FVector::Distance(Eye,A.At)<35)
   Level=FMath::Max(Level,A.bHunting?1.:.55);
 return Level;
}
double FS47FieldRadio::ObservedThreat() const
{
 return M && M->Player?ObservedThreat(S47Three::ToThree(M->Player->EyeUE()),Nearby()):0;
}
FS47FieldRadio::EMusicTier FS47FieldRadio::MusicTierFor(double Heard,double Level,double Seen,bool bHunt,bool bWasNear)
{
 // D45-20261110: what the player can tell, in order: coming at them, in view or close, heard on the set, nothing.
 if(Seen>=1 || bHunt) return EMusicTier::Action;
 if(Seen>0 || Level>.04) return EMusicTier::Tension;
 return Heard>=(bWasNear?ApproachOut:ApproachIn)?EMusicTier::Eerie:EMusicTier::Place;
}
const TCHAR* FS47FieldRadio::MoodOf(EMusicTier In)
{
 return In==EMusicTier::Action?TEXT("action"):In==EMusicTier::Tension?TEXT("tension"):In==EMusicTier::Eerie?TEXT("eerie"):TEXT("");
}
FString FS47FieldRadio::MusicMood(double& Severity) const
{
 // D45-20261110: steps on the roof and the lamp failing: the take owns the room, the place's piece goes in two seconds (urgent).
 if(M && M->IsFreeform() && M->Freeform && M->Freeform->Taken && M->Freeform->Taken->Stage()==FS47Taken::EStage::Dread)
 {Severity=1; return TEXT("tension");}
 // Eerie stays under the Score's urgent .75: it crossfades in four seconds, never cuts.
 Severity=MusicNow==EMusicTier::Action?1.:MusicNow==EMusicTier::Tension?.55:MusicNow==EMusicTier::Eerie?FMath::Clamp(RealNear,.1,.7):0.;
 return MoodOf(MusicNow);
}
bool FS47FieldRadio::Scare(int32 Kind,double Strength,bool bAudition)
{
 return M && M->Audio && Kind>=0 && Kind<4 && Sound.Scare(M->Audio,FS47EncounterSound::EScare(Kind),Strength,bAudition);
}
void FS47FieldRadio::Step(double Dt)
{
 if(!M) return;
 SinceMessage+=Dt;
 StepCaption();
 Build();Rest(Dt);
 const bool Play=bAvailable && !bUnsupported && M->Mode==US47Main::EMode::Play;
 const bool Shown=Play && !bOwned && !bLeftOut && M->Web && M->Web->Area()==TEXT("saro");
 if(Root) Root.SetVisible(Shown);
 if(Hit.IsValid()) Hit->SetCollisionEnabled(Shown?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
 const bool Held=!Play || M->bPausedByMenu || (M->ModalOpen() && !(M->Audio && M->Audio->ReceiverMonitorOn())) || !M->Story || M->Story->IsPaused() || M->Story->IsCinematic();
 Sound.Hold(Held);
 if(Held || !M->Player) return;
 StepHand(Dt);
 const FVector Eye=S47Three::ToThree(M->Player->EyeUE());
 double Level=0,Heard=0; bool Visible=false,Hunting=false;
 // The cues and the music keep to the short range. In Freeform the set itself hears them from further off.
 const double Reach=M->IsFreeform()?FreeformReach:18.;
 const TArray<FPresence> Near=Nearby();
 for(const auto& A:Near)
 {
  const double K=Proximity(Eye,A); Level=FMath::Max(Level,K);
  Heard=FMath::Max(Heard,Proximity(Eye,A,Reach));
  Visible|=K>.04 && A.bVisible; Hunting|=K>.04 && A.bHunting;
 }
 // Freeform: the set hears everything near the way it hears a body (GEAR-FLICKER-20261108), not only the bodies above; and what
 // any set hears out here (a spell of open squelch), so that the hiss alone says nothing sure.
 if(M->IsFreeform()) Heard=FMath::Max(Heard,GearNoise);
 RealNear=Heard;
 if(M->IsFreeform()) Heard=FMath::Max(Heard,OrdHiss);
 LastLevel=Level; LastSeen=Visible; LastHunt=Hunting;
 // Require stable changes and a genuine quiet gap before another presence cue.
 QuietFor=Level<.015?QuietFor+Dt:0;
 SeenFor=Visible?SeenFor+Dt:0; ThreatFor=Level>.04?ThreatFor+Dt:0;
 if(QuietFor>8) LastCue=-1;
 int32 Want=Hunting?2:SeenFor>.25?1:ThreatFor>1.1?0:-1;
 if(!M->ModalOpen() && Want>LastCue && Want>=0 && Sound.Cue(M->Audio,FS47EncounterSound::EKind(Want),{},FMath::Max(.3,Level))) LastCue=Want;
 // DREAD-20261110: the ground under a meeting. Near (the cues' 18 m) it comes up with the nearness; one in view further off
 // (35 m, as the music counts it) has it too. It sinks away by itself when they are gone.
 {
  const double Far=ObservedThreat(Eye,Near);
  // D45-20261110: in Freeform the ground starts early, with what the set hears (to .45 of it), before the cues' 18 m.
  double Low=0,High=0; FS47EncounterSound::DreadFor(Level,Visible||Far>0,Hunting||Far>=1,Low,High,M->IsFreeform()?RealNear:0.);
  MusicNow=MusicTierFor(M->IsFreeform()?RealNear:0.,Level,Far,Hunting,MusicNow!=EMusicTier::Place);
  if(AuditionFor>0){AuditionFor-=Dt;Low=FMath::Max(Low,AuditionLow);High=FMath::Max(High,AuditionHigh);}
  Sound.Dread(M->Audio,Low,High);
 }
 // One shared interference voice: carried set, active monitor or powered AM set.
 // The historical Room 6 radio and its authored 4/7 sequence are untouched.
 TOptional<FVector> Speaker;
 bool Listening=bOwned || (M->Audio && M->Audio->ReceiverMonitorOn());
 if(!Listening && M->Audio && M->Audio->RadioOn())
 { Listening=true; Speaker=M->Audio->AmRadioPosition(); }
 LastRadio=Listening?Heard:0;
 Sound.UpdateRadio(M->Audio,LastRadio,Speaker,false);
 // Freeform: it crackles, seldom at the edge of its reach, near enough all the time close by; and of itself, now and then, far
 // from the mast, in low ground, in the wind or with a far car keyed up on the channel.
 const double Ordinary=Listening&&M->IsFreeform()?OrdCrackle:0.;
 bool bCrackled=false;
 if(M->IsFreeform()&&(LastRadio>.01||Ordinary>.01))
 {
  CrackleIn-=Dt;
  if(CrackleIn<=0)
  {
   const double RealGap=LastRadio>.01?FMath::Lerp(2.6,.09,FMath::Sqrt(LastRadio)):1e9;
   const double OrdinaryGap=Ordinary>.01?FMath::Lerp(14.,2.,FMath::Clamp(Ordinary/.7,0.,1.)):1e9;
   CrackleIn=FMath::Min(RealGap,OrdinaryGap)*FMath::FRandRange(.55f,1.45f);
   if(Sound.Crackle(M->Audio,FMath::Max(LastRadio,Ordinary*.6))){++Crackled;bCrackled=true;}
  }
 }
 else CrackleIn=0;
 // A spell of breaking up, for the night's log and the notebook: the hiss up, or the crackle coming thick (three in a few seconds;
 // a crackle now and then far from the mast is not one). What comes within eight seconds of the last is the same spell.
 SinceNoise+=Dt;SinceCrackle+=Dt;
 if(bCrackled){if(SinceCrackle>6)CrackleRun=0;++CrackleRun;SinceCrackle=0;}
 if(bOwned&&!bLeftOut&&(LastRadio>.25||CrackleRun>=3))
 {
  if(SinceNoise>8)++DropCount;
  SinceNoise=0;
  if(CrackleRun>=3&&SinceCrackle>6)CrackleRun=0;
 }
 // The HUD's wave, when the line is up of its own accord: what the set hears now, wandering a little, never a steady reading.
 if((StrengthIn-=Dt)<=0)
 {
  StrengthIn=FMath::FRandRange(.6f,2.4f);
  StrengthWant=FMath::Clamp(1.-.6*LastRadio-.5*Ordinary-FMath::FRandRange(0.f,.25f),.15,1.);
 }
 StrengthNow+=(StrengthWant-StrengthNow)*FMath::Min(1.,Dt*2.5);
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
  if(!Check(!Owned() && (Copies.Num()==3||(bTripoWalkie&&Copies.Num()==1)) && Root.Shown(),TEXT("three source-mesh copies, or Tom's Tripo walkie, are on the SARO desk"))) return;
  const auto* Old=M->Web->Node(M->Web->Obj(TEXT("world.room6.objs.walkie")).I);
  if(!Check(Old && Old->Children.Num()==3,TEXT("Room 6 original remains separate and complete")))return;
  if(!Check(LastLevel==0,TEXT("quiet room does not warn about dormant or future enemies")))return;
  {const FVector R=Root.WorldPosition();M->Player->Place(R.X,R.Z+1.,0);} M->Player->Pitch=-.65; // the desk, or the hall dresser (FIX-20261110)
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
 C(Proximity(FVector::ZeroVector,A,FreeformReach)>0,TEXT("the Freeform set hears it from further off"));
 {
  double Last=-1;bool Rising=true;
  for(double D=FreeformReach;D>=3;D-=3){A.At.Z=D;const double K=Proximity(FVector::ZeroVector,A,FreeformReach);Rising&=K>=Last;Last=K;}
  C(Rising&&Last==1,TEXT("stronger all the way in, and full close by"));
  A.At.Z=FreeformReach;C(Proximity(FVector::ZeroVector,A,FreeformReach)==0,TEXT("silent at its reach"));
 }
 A.At.Z=4; A.bHunting=true;
 C(ObservedThreat(FVector::ZeroVector,{A})==0,TEXT("off-screen threat does not reveal itself through music"));
 A.bVisible=true;
 C(ObservedThreat(FVector::ZeroVector,{A})==1,TEXT("observed nearby chase requests action music"));
 A.bHunting=false;
 C(ObservedThreat(FVector::ZeroVector,{A})==.55,TEXT("observed presence requests tension rather than action"));
 {
  // D45-20261110: the music's four tiers, from what the player can tell only.
  FPresence T; T.bActive=true; T.At=FVector(0,0,40);
  const double At40=Proximity(FVector::ZeroVector,T,FreeformReach); T.At.Z=58;
  const double At58=Proximity(FVector::ZeroVector,T,FreeformReach); T.At.Z=40;
  C(MusicTierFor(0,0,0,false,false)==EMusicTier::Place && MoodOf(EMusicTier::Place)[0]==0,TEXT("nothing near: the place's own music"));
  C(At58==0 && MusicTierFor(At58,0,0,false,false)==EMusicTier::Place,TEXT("the tall one 58 m off, past the set's reach, changes nothing"));
  C(MusicTierFor(At40,Proximity(FVector::ZeroVector,T),0,false,false)==EMusicTier::Eerie && FCString::Strcmp(MoodOf(EMusicTier::Eerie),TEXT("eerie"))==0,
   TEXT("something heard on the set 40 m off takes the town's piece to eerie"));
  C(MusicTierFor(.04,0,0,false,true)==EMusicTier::Eerie && MusicTierFor(.04,0,0,false,false)==EMusicTier::Place && MusicTierFor(.02,0,0,false,true)==EMusicTier::Place,
   TEXT("in at .05, out only under .025: no chatter at the edge"));
  T.bHunting=true;
  C(ObservedThreat(FVector::ZeroVector,{T})==0 && MusicTierFor(At40,0,ObservedThreat(FVector::ZeroVector,{T}),false,false)==EMusicTier::Eerie,
   TEXT("one hunting unseen 40 m off is only heard: eerie, never the chase"));
  T.bHunting=false; T.bVisible=true; T.At.Z=34;
  C(MusicTierFor(.3,0,ObservedThreat(FVector::ZeroVector,{T}),false,true)==EMusicTier::Tension && FCString::Strcmp(MoodOf(EMusicTier::Tension),TEXT("tension"))==0,
   TEXT("one standing in view 34 m off is tension, not the chase"));
  C(MusicTierFor(.6,.3,0,false,true)==EMusicTier::Tension,TEXT("close enough for the cues, unseen: tension"));
  T.bHunting=true;
  C(MusicTierFor(.3,0,ObservedThreat(FVector::ZeroVector,{T}),false,true)==EMusicTier::Action && MusicTierFor(.9,.5,0,true,true)==EMusicTier::Action
   && FCString::Strcmp(MoodOf(EMusicTier::Action),TEXT("action"))==0,TEXT("one coming at them, seen or close by: action"));
  FS47FieldRadio Q(nullptr); double Sev=-1;
  C(Q.MusicMood(Sev).IsEmpty() && Sev==0 && Q.Approach()==0,TEXT("outside a game the set asks nothing of the music"));
 }
 FS47FieldRadio R(nullptr); R.Begin(nullptr); C(!R.Owned(),TEXT("old and new cases leave pickup available"));
 auto S=R.Save();S47J::Set(S,TEXT("owned"),true);R.Begin(S);C(R.Owned(),TEXT("ownership survives serialize/restore"));
 S47J::Set(S,TEXT("v"),99); R.Begin(S); C(!R.Owned() && S47J::Write(R.Save())==S47J::Write(S),TEXT("unknown future version is preserved and inactive"));
 {
  FS47FieldRadio Q(nullptr); Q.TestOwn();
  C(!Q.InUse() && Q.Strength()==1 && Q.PresenceLevel()==0 && !Q.PresenceSeen() && !Q.PresenceHunting(),TEXT("a carried set that has heard nothing is not in use on the HUD, at full strength"));
  // M2: breaking up never puts the line on the HUD; only words, a call or the player's own hand do.
  Q.SetGearNoise(1);Q.SetOrdinaryNoise(1,1);Q.LastRadio=1;
  C(!Q.InUse()&&Q.OrdinaryNoiseNow()==1&&Q.GearNoiseNow()==1,TEXT("a set breaking up with something near is heard, not shown: no HUD line for it"));
 }
 {
  int32 NearVoice=0,NobodyVoice=0;
  for(int32 I=0;I<100;++I){NearVoice+=BackFor(true,I/100.f)==EBack::Voice?1:0;NobodyVoice+=BackFor(false,I/100.f)==EBack::Voice?1:0;}
  C(NearVoice>50&&NearVoice<100&&NobodyVoice>0&&NobodyVoice<40,TEXT("what comes back is not sure either way: mostly a voice under the hiss with something near, now and then with nobody"));
 }
 {
  // WALKIE-USE-20261108: the channels, what the set is on, and what it says it is.
  FS47FieldRadio W(nullptr);W.Begin(nullptr);
  C(W.Channel()==1&&FMath::IsNearlyEqual(ChannelMHz(1),49.830)&&FMath::IsNearlyEqual(ChannelMHz(2),49.845)&&FMath::IsNearlyEqual(ChannelMHz(3),49.860),TEXT("three channels, 1 first: 49.830, 49.845, 49.860"));
  W.NextChannel();C(W.Channel()==1,TEXT("no set in hand: nothing to turn"));
  W.TestOwn();W.NextChannel();
  C(W.Channel()==2&&W.InUse()&&S47J::Int(W.Save(),TEXT("ch"))==2,TEXT("turned to 2: it shows on the HUD and is saved"));
  FS47FieldRadio Back(nullptr);Back.Begin(W.Save());C(Back.Channel()==2&&Back.Owned(),TEXT("the channel survives a Continue"));
  W.NextChannel();W.NextChannel();C(W.Channel()==1&&!W.Save()->HasField(TEXT("ch")),TEXT("round to 1 again, and the save is as it was before channels"));
  C(!W.Message(TEXT("DALE: Dale out."))&&!W.Hail(FHail{TEXT("Come in."),TEXT(""),TEXT("")},nullptr,nullptr)&&!W.LoudNow()&&!W.Hailing(),TEXT("nothing to say it with outside a game: no hail, no message, no noise"));
  C(Head(1,TEXT("DALE"))==TEXT("WALKIE 1, DALE")&&Head(3)==TEXT("WALKIE 3")&&Head(9)==TEXT("WALKIE 3"),TEXT("the caption's head names the set, its channel and who speaks"));
  {
   // M7-WALKIE-20261109: no set in hand yet: the hail is heard from the desk once, and is still no hail
   FS47FieldRadio D(nullptr);D.Begin(nullptr);
   const FHail Words{TEXT("SARO, Dale. Pick up."),TEXT(""),TEXT("")};
   const bool bFirst=D.Hail(Words,nullptr,nullptr);const bool bSecond=D.Hail(Words,nullptr,nullptr);
   C(!bFirst&&!bSecond&&D.DeskCalls()==1&&!D.Hailing(),TEXT("a hail with the walkie still on the desk is heard from the desk once, and the line waits for the slip"));
   FS47FieldRadio L(nullptr);L.Begin(nullptr);L.Lose();L.Hail(Words,nullptr,nullptr);
   FS47FieldRadio H(nullptr);H.Begin(nullptr);H.TestOwn();H.Hail(Words,nullptr,nullptr);
   C(L.DeskCalls()==0&&H.DeskCalls()==0,TEXT("nothing from the desk once the set is out in the field, or in the player's hand"));
  }
  Report(nullptr,TEXT("A rancher reports lights."));
  C(!CbAtHand(nullptr)&&!CbSay(nullptr,{TEXT("Breaker.")},TEXT("man"))&&W.TipsWaiting()==0,TEXT("no game: no CB at hand, and nothing held for one"));
  // CB-TRUCK-20261109 (Tom, 9 November): the CB is the truck's, under the dash: heard and used only sitting in it.
  C(CbHeardAt(true,false,false)&&CbHeardAt(true,false,true)&&!CbHeardAt(false,true,false)&&!CbHeardAt(false,true,true)&&!CbHeardAt(false,false,true)&&!CbHeardAt(false,false,false),
   TEXT("the CB works only in the truck: not in the SARO building, not on foot, not with the walkie in hand"));
  C(S47HudGauges::RadioKind(true,CbHeardAt(false,true,false),false,false,false,0)!=1&&S47HudGauges::RadioKind(true,CbHeardAt(true,false,false),false,false,false,0)==1,
   TEXT("the HUD's CB 19 line shows a call only in the truck"));
 }
 return Ok;
}

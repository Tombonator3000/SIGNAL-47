#include "S47FreeformAudioDesk.h"
#include "S47Script.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Observing.h"
#include "S47Story.h"
#include "S47Main.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Inter.h"
#include "S47UI.h"
#include "S47Three.h"
#include "S47Audio.h"
#include "S47ReceiverSound.h"
#include "S47ConsoleKit.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47UiSound.h"

namespace
{
 const TCHAR* Actions[]={TEXT("original"),TEXT("control"),TEXT("play"),TEXT("rewind"),TEXT("mark_start"),TEXT("mark_end"),TEXT("retain"),TEXT("ref_next"),TEXT("ref_play"),TEXT("ref_stop")};
 const TCHAR* Captions[]={TEXT("ORIGINAL"),TEXT("CONTROL"),TEXT("PLAY / STOP"),TEXT("REWIND"),TEXT("MARK IN"),TEXT("MARK OUT"),TEXT("RETAIN"),TEXT("REF NEXT"),TEXT("REF PLAY"),TEXT("REF STOP")};
 const TCHAR* StationId=TEXT("ff:audio:station");
 // These two tests still aim at the physical keys; players use the panel on the same actions.
 bool AimedKeyTest(){return FParse::Param(FCommandLine::Get(),TEXT("S47FreeformEvidenceTest"))||FParse::Param(FCommandLine::Get(),TEXT("S47FreeformInWorldTest"));}
 int32 ActionIndex(const FString& Id)
 {for(int32 I=0;I<UE_ARRAY_COUNT(Actions);++I)if(Id==FString(TEXT("ff:audio:"))+Actions[I])return I;return INDEX_NONE;}
}
FS47FreeformAudioDesk::~FS47FreeformAudioDesk(){Stop();}
const TCHAR* FS47FreeformAudioDesk::SourceName(int32 Source,bool bPlain){return Source==0?TEXT("ORIGINAL"):bPlain?TEXT("REFERENCE"):TEXT("CONTROL");}
bool FS47FreeformAudioDesk::PlainNow()const
{
 if(!M||!M->IsFreeform()||!M->Freeform)return false;
 const auto* J=M->Freeform->CurrentJob();return J&&FS47FreeformSession::PlainAnomaly(*J);
}
bool FS47FreeformAudioDesk::SelfTest(TArray<FString>& Lines)
{
 // review 14 Nov (B3): one name for the station's tape in each flow, on the key, the readout and the toast that sends the player to it
 const bool bNames=FString(SourceName(0,true))==TEXT("ORIGINAL")&&FString(SourceName(0,false))==TEXT("ORIGINAL")&&FString(SourceName(1,true))==TEXT("REFERENCE")&&FString(SourceName(1,false))==TEXT("CONTROL");
 const bool bKeys=FString(Captions[0])==SourceName(0,false)&&FString(Captions[1])==SourceName(1,false)&&FString(Actions[1])==TEXT("control");
 const FString Toast=FS47Observing::DeckToast();
 const bool bToast=Toast.Contains(FString(TEXT("ORIGINAL / "))+SourceName(1,false))&&Toast.Contains(TEXT("wall table"))&&!Toast.Contains(TEXT("telephone"))&&!Toast.Contains(TEXT("REFERENCE"));
 Lines.Add(FString::Printf(TEXT("%s audio desk: the station's tape is REFERENCE in Freeform's plain anomaly flow and CONTROL in Story's, on the key (built as %s, renamed per flow), the readout and Story's toast (\"%s\")"),bNames&&bKeys&&bToast?TEXT("PASS"):TEXT("FAIL"),Captions[1],*Toast));
 return bNames&&bKeys&&bToast;
}
TSharedPtr<FS47FreeformAudioDesk> FS47FreeformAudioDesk::Shared(US47Main* Main)
{
 static TWeakObjectPtr<US47Main> Owner;static TWeakPtr<FS47FreeformAudioDesk> Desk;
 if(Owner.Get()==Main)if(const auto Existing=Desk.Pin())return Existing;
 auto Result=MakeShared<FS47FreeformAudioDesk>(Main);Owner=Main;Desk=Result;return Result;
}
bool FS47FreeformAudioDesk::Live()const
{
 return !IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&Scene.IsValid()&&IsValid(M->Web)&&Scene.Get()==M->Web;
}
bool FS47FreeformAudioDesk::Ready()const
{
 if(!bActive||!bBuilt||!Live())return false;
 return M->IsFreeform()?M->Freeform&&M->Freeform->Active():M->Observing&&M->Observing->WorkEnabled()&&M->Observing->Active();
}
bool FS47FreeformAudioDesk::Powered()const
{
 if(!Ready())return false;
 return M->IsFreeform()?(!M->Freeform->PowerFault()||(M->Freeform->Field&&M->Freeform->Field->HasBackupPower())):M->Story&&M->Story->BankIn()&&!M->Story->FeederOutage();
}
bool FS47FreeformAudioDesk::ListeningPosition()const
{
 if(!Powered()||!IsValid(M->Player)||!IsValid(M->World)||M->Web->Area()!=TEXT("saro")||M->World->Area!=TEXT("saro")||M->World->bDriving||M->World->bBusy)return false;
 return FVector2D(M->Player->Pos.X-Anchor.X,M->Player->Pos.Z-Anchor.Z).Size()<=2.4&&FMath::Abs(M->Player->FloorY-Anchor.Y)<2.;
}
FVector FS47FreeformAudioDesk::InteractionPoint(const FString& Id)const
{
 if(!Ready())return FVector::ZeroVector;
 for(const auto& C:Controls)if(C.Id==Id)return C.Object.WorldPosition();return Anchor;
}
void FS47FreeformAudioDesk::Show(bool bShow)
{
 if(!Live())return;
 const bool Free=M->IsFreeform();const bool Keys=bShow&&(!Free||AimedKeyTest()),Sign=bShow&&Free;
 for(const auto& C:Controls)if(C.Mesh.IsValid()){C.Mesh->SetVisibility(Keys);C.Mesh->SetHiddenInGame(!Keys);C.Mesh->SetCollisionEnabled(Keys?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);}
 for(const auto& T:Texts)if(T.IsValid()){T->SetVisibility(Keys);T->SetHiddenInGame(!Keys);}
 for(const auto& T:SignTexts)if(T.IsValid()){T->SetVisibility(Sign);T->SetHiddenInGame(!Sign);}
 if(SignBoard.IsValid()){SignBoard->SetVisibility(Sign);SignBoard->SetHiddenInGame(!Sign);}
}
void FS47FreeformAudioDesk::Begin()
{
 Stop();if(!IsValid(M)||!IsValid(M->Web))return;
 if(Scene.Get()!=M->Web){bBuilt=false;Controls.Reset();Texts.Reset();SignTexts.Reset();SignBoard.Reset();Readout.Reset();Deck={};StackHome.Reset();}
 Scene=M->Web;Life=MakeShared<uint8>(0);bActive=true;LastReadout.Empty();
 if(Build()){BindControls();Show(Ready());}
 PlaceStack(M->IsFreeform());
}
void FS47FreeformAudioDesk::PlaceStack(bool bTable)
{
 if(!Live())return; // nothing to put back while the engine is shutting down
 const int32 Kit=FS47ConsoleKit::Find(M->Web);const auto* N=Kit>=0?M->Web->Node(Kit):nullptr;USceneComponent* C=N?N->Comp.Get():nullptr;if(!C)return;
 if(!StackHome.IsSet())StackHome=C->GetComponentTransform();
 C->SetMobility(EComponentMobility::Movable);
 if(!bTable){C->SetWorldTransform(*StackHome);return;}
 // Half a turn about the analyzer's foot on the console, to its place on the table facing the room.
 const FVector From=S47Three::ToUE(FVector(2.95,.78,-3.95)),To=S47Three::ToUE(FVector(-.75,.737,4.1));
 const FQuat Turn(FVector::UpVector,UE_DOUBLE_PI);
 FTransform T=*StackHome;T.SetLocation(To+Turn.RotateVector(StackHome->GetLocation()-From));T.SetRotation(Turn*StackHome->GetRotation());
 C->SetWorldTransform(T);
}
void FS47FreeformAudioDesk::Stop()
{
 if(Reference)Reference->Stop();Reference.Reset();bReferenceView=false;
 bActive=false;Life.Reset();Show(false);PlaceStack(false);
}
bool FS47FreeformAudioDesk::Build()
{
 if(!Live())return false;if(bBuilt)return true;
 Deck=M->Web->Obj(TEXT("room.group")).Find(TEXT("prop:reelDeck"));
 if(!Deck)return false;
 const auto* N=M->Web->Node(Deck.I);if(!N||!N->Comp.IsValid())return false;
 const FBox B=Deck.WorldBox();if(!B.IsValid)return false;
 auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));if(!Cube||!Base)return false;
 // The existing deck faces into the room (-Z in the SARO scene). Controls sit
 // just above its lower fascia, sized from its actual imported world bounds.
 const double Width=FMath::Clamp(B.GetSize().X,.38,.68),X=B.GetCenter().X;
 const double Y=B.Min.Y+.065,Z=B.Min.Z-.018;
 Anchor=FVector(X,Y,Z);
 auto Text=[&](const FString& Name,const FString& Value,const FVector& At,double Size)
 {
  auto* T=NewObject<UTextRenderComponent>(M->Web,*Name);T->SetupAttachment(N->Comp.Get());T->SetMobility(EComponentMobility::Movable);T->SetText(FText::FromString(Value));
  T->SetHorizontalAlignment(EHTA_Center);T->SetVerticalAlignment(EVRTA_TextCenter);T->SetWorldSize(Size);T->SetTextRenderColor(FColor(212,214,176));T->SetCollisionEnabled(ECollisionEnabled::NoCollision);T->RegisterComponent();
  T->SetWorldLocation(S47Three::ToUE(At));T->SetWorldRotation(FRotator(0,-90,0));Texts.Add(T);return T;
 };
 for(int32 I=0;I<UE_ARRAY_COUNT(Actions);++I)
 {
  const int32 Col=I<4?I:I<7?I-4:I-7,Count=I<4?4:3;
  const FVector At(X+Width*((Col+.5)/Count-.5),Y+(I<4?.065:I<7?0:.13),Z);
  auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web);Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.12,.15,.13));Mat->SetScalarParameterValue(TEXT("Roughness"),.65);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
  auto* C=NewObject<UStaticMeshComponent>(M->Web,*FString::Printf(TEXT("FreeformTapeKey%d"),I));C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(N->Comp.Get());C->SetStaticMesh(Cube);C->SetMaterial(0,Mat);
  C->SetWorldLocation(S47Three::ToUE(At));C->SetWorldScale3D(S47Three::DirToUE(FVector(Width/Count*.91,.048,.027)));C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);C->SetCastShadow(false);C->RegisterComponent();
  const FString Id=FString(TEXT("ff:audio:"))+Actions[I];const FS47Obj O=M->Web->ObjAt(M->Web->AdoptNode(C,Deck.I,Id));Controls.Add({Id,O,C,Mat});
  Text(FString::Printf(TEXT("FreeformTapeLabel%d"),I),Captions[I],At+FVector(0,0,-.016),1.1);
 }
 Readout=Text(TEXT("FreeformTapeReadout"),TEXT("SARO / ORIGINAL - INTERNAL REFERENCE"),FVector(X,Y+.172,Z-.002),1.15);
 // A sign on the wall above the deck, readable from across the room.
 const FVector SignAt(B.GetCenter().X-.26,B.Max.Y+.30,B.Max.Z-.01); // clear of the notice board beside it
 auto* SignMat=UMaterialInstanceDynamic::Create(Base,M->Web);SignMat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.05,.07,.06));SignMat->SetScalarParameterValue(TEXT("Roughness"),.7);SignMat->SetScalarParameterValue(TEXT("NativeLighting"),1);
 auto* Board=NewObject<UStaticMeshComponent>(M->Web,TEXT("FreeformTapeSign"));Board->SetMobility(EComponentMobility::Movable);Board->SetupAttachment(N->Comp.Get());Board->SetStaticMesh(Cube);Board->SetMaterial(0,SignMat);Board->SetCollisionEnabled(ECollisionEnabled::NoCollision);Board->RegisterComponent();
 Board->SetWorldLocation(S47Three::ToUE(SignAt));Board->SetWorldScale3D(S47Three::DirToUE(FVector(.58,.17,.012)));SignBoard=Board;
 auto SignLine=[&](const TCHAR* Name,const TCHAR* Value,double Up,double Size)
 {
  auto* T=Text(Name,Value,SignAt+FVector(0,Up,-.012),Size);T->SetTextRenderColor(FColor(240,172,62));Texts.Pop();SignTexts.Add(T);
 };
 SignLine(TEXT("FreeformTapeSign1"),TEXT("REEL DECK"),.028,7.);
 SignLine(TEXT("FreeformTapeSign2"),TEXT("COMPARE RECORDINGS"),-.05,3.8);
 bBuilt=true;return true;
}
void FS47FreeformAudioDesk::BindControls()
{
 if(!Live()||!IsValid(M->Inter))return;
 const TWeakPtr<uint8> Guard=Life;const TWeakObjectPtr<US47Main> Owner=M;
 for(const auto& C:Controls)
 {
  const FString Id=C.Id;FS47Interactable I;I.Id=Id;I.Object=C.Object;I.Range=2.4;
  I.Label=[this,Guard,Owner,Id]{return !IsEngineExitRequested()&&Guard.IsValid()&&Owner.IsValid()?Label(Id):FString();};
  I.Use=[this,Guard,Owner,Id]{if(!IsEngineExitRequested()&&Guard.IsValid()&&Owner.IsValid())Use(Id);};
  I.Examine=[this,Guard,Owner,Id]{return !IsEngineExitRequested()&&Guard.IsValid()&&Owner.IsValid()?Label(Id):FString();};
  M->Inter->Add(I);M->Web->MarkUsable(C.Object.I,Id);
 }
 if(M->IsFreeform()&&!AimedKeyTest())
 {
  const FString Id=StationId;FS47Interactable I;I.Id=Id;I.Object=Deck;I.Range=2.4;
  I.Label=[this,Guard,Owner,Id]{return !IsEngineExitRequested()&&Guard.IsValid()&&Owner.IsValid()?Label(Id):FString();};
  I.Use=[this,Guard,Owner,Id]{if(!IsEngineExitRequested()&&Guard.IsValid()&&Owner.IsValid())Use(Id);};
  // D45-20261110: what the deck is for, said when it is looked at, now that its panel keeps to its keys and displays
  I.Examine=[this,Guard,Owner]{return !IsEngineExitRequested()&&Guard.IsValid()&&Owner.IsValid()&&Ready()?FString(TEXT("A reel-to-reel deck for two tapes of one job: the ORIGINAL the antennas recorded, and the CONTROL, the station's own reference with the antennas out. A sound on both comes from the equipment. A sound only on the original came from outside.")):FString();};
  M->Inter->Add(I);M->Web->MarkUsable(Deck.I,Id);
 }
}
FString FS47FreeformAudioDesk::ReferenceLabel()const{return Reference?Reference->Label():FString(TEXT("Press NEXT to load a reference recording."));}
bool FS47FreeformAudioDesk::ReferencePlaying()const{return Reference&&Reference->Playing();}
int32 FS47FreeformAudioDesk::ReferenceIndex()const{return Reference?Reference->Index():-1;}
FString FS47FreeformAudioDesk::Label(const FString& Id)const
{
 if(Id==StationId)return !Ready()||!M->IsFreeform()?FString():Powered()?FString(TEXT("Reel deck / compare recordings")):FString(TEXT("Reel deck unpowered / restore feeder or generator"));
 const int32 I=ActionIndex(Id);if(!Ready()||I==INDEX_NONE)return FString();
 if(!Powered())return TEXT("Tape comparator unpowered / restore feeder or generator");
 if(I==7)return TEXT("Select next listening reference");
 if(I==8)return Reference&&Reference->Playing()?TEXT("Stop reference playback"):TEXT("Play selected listening reference");
 if(I==9)return TEXT("Stop reference playback");
 if(I==2)return (M->IsFreeform()?M->Freeform->AudioPlaying():M->Observing->AudioPlaying())?TEXT("Stop tape playback"):TEXT("Play selected recording");
 if(I==1&&PlainNow())return TEXT("Select the station reference"); // review 14 Nov (B3)
 static const TCHAR* Labels[]={TEXT("Select original recording"),TEXT("Select independent control"),TEXT("Play selected recording"),TEXT("Rewind selected tape"),TEXT("Mark start on the original"),TEXT("Mark end on the original"),TEXT("Retain measured comparison / then register at OPS")};
 return Labels[I];
}
bool FS47FreeformAudioDesk::Use(const FString& Id)
{
 if(Id==StationId){if(!Ready()||!M->IsFreeform())return false;M->Freeform->OpenTapePanel();return true;}
 const int32 I=ActionIndex(Id);if(I==INDEX_NONE||!Ready())return false;
 S47UiSound::Retag(M->Audio,I==4||I==5?S47UiSound::EKind::Mark:I==6?S47UiSound::EKind::Retain:S47UiSound::EKind::Transport); // FIX10-7: the panel's key just pressed, named
 if(!ListeningPosition()){if(IsValid(M->UI))M->UI->Toast(TEXT("Use the powered tape comparator at the radio table."),3);return true;}
 const bool Free=M->IsFreeform();bool Accepted=true;
 if(I>=7)
 {
  // Reference playback has its own transport; comparison cursors, marks, heard
  // coverage and retained conclusions remain owned by the original/control deck.
  if(Free){if(M->Freeform->AudioPlaying()||M->Freeform->AudioPreparing())M->Freeform->AudioAction(TEXT("pause"));}
  else if(M->Observing->AudioPlaying())M->Observing->AudioAction(TEXT("pause"));
  if(!Reference&&M->Audio)Reference=MakeShared<FS47ReceiverSound>(M->Audio,M->Audio->Spatial(Anchor+FVector(0,.12,0),.65));
  bReferenceView=true;
  if(Reference)
  {
   if(I==7)Reference->Next();else if(I==9)Reference->Stop();else Accepted=Reference->Toggle();
   if(IsValid(M->UI))M->UI->Toast(Reference->Label(),4);
  }
  return true;
 }
 if(Reference)Reference->Stop();bReferenceView=false;
 if(Free)M->Freeform->OpenAudio();else M->Observing->OpenAudioComparison();
 const bool Playing=Free?(M->Freeform->AudioPlaying()||M->Freeform->AudioPreparing()):M->Observing->AudioPlaying();
 const FString Action=I==2?(Playing?TEXT("pause"):TEXT("play")):I==6?TEXT("confirm"):Actions[I];
 if(I==3&&Free)M->Freeform->SeekAudio(0);
 else Accepted=Free?M->Freeform->AudioAction(Action):M->Observing->AudioAction(I==3?TEXT("rewind"):Action);
 if(IsValid(M->UI))
 {
  const FString Feedback=Free?M->Freeform->FeedbackText():M->Observing->AudioFeedbackText();
  if(!Accepted)M->UI->Toast(Feedback.IsEmpty()?(Free?TEXT("Nothing unusual on tape yet. An unusual take is recorded at the receiver first."):TEXT("Retain an anomalous original and complete its independent control before comparing.")):Feedback,4);
  // (Freeform's RETAIN says its one next step itself, FS47FreeformSession::AudioAction: Tom, 9 November, the case opens at OPS)
 }
 return true;
}
void FS47FreeformAudioDesk::Step(double Dt)
{
 if(!bActive||!Live()){if(Reference)Reference->Stop();return;}
 if(!bBuilt){if(!Build())return;BindControls();}
 const bool EvidencePlaying=M->IsFreeform()?M->Freeform&&M->Freeform->AudioPlaying():M->Observing&&M->Observing->AudioPlaying();
 if(Reference)Reference->Step(ListeningPosition()&&!EvidencePlaying&&!M->bPausedByMenu&&(!M->ModalOpen()||(M->IsFreeform()&&M->Freeform&&M->Freeform->TapePanelOpen()))&&M->Story&&!M->Story->IsCinematic(),M->Audio&&M->Audio->Ctx?M->Audio->Ctx->CurrentTime():0);
 const bool Shown=Ready()&&M->Web->Area()==TEXT("saro");Show(Shown);if(!Shown)return;
 int32 Source=0;double Cursor=0,HeardO=0,HeardC=0,MarkIn=-1,MarkOut=-1,Control=0;bool Playing=false,Reviewed=false,Original=false;
 if(M->IsFreeform())
 {
  const auto& F=*M->Freeform;const auto* J=F.CurrentJob();Source=F.AudioSource();Cursor=F.AudioPosition();Playing=F.AudioPlaying();
  if(J){HeardO=J->OriginalCovered;HeardC=J->ControlCovered;MarkIn=J->MarkStart;MarkOut=J->MarkEnd;Control=J->ControlAt;Reviewed=J->bAudioReviewed;Original=J->bAnomaly&&J->Stage>=FS47FreeformSession::EJobStage::Chart;} // D45-20261110: on tape from the end of the take
 }
 else
 {
  const auto& O=*M->Observing;Source=O.AudioSource();Cursor=O.AudioPosition();Playing=O.AudioPlaying();Reviewed=O.AudioReviewed();HeardO=O.AudioHeard(0);HeardC=O.AudioHeard(1);MarkIn=O.AudioMarkedStart();MarkOut=O.AudioMarkedEnd();Control=O.ControlElapsed();Original=O.LoopStage()>=FS47Observing::ELoopStage::Rack;
 }
 FString Line=Powered()?FString::Printf(TEXT("%s / %s / %s s\nHEARD TAKE %s/18  REF %s/8\nMARK %s - %s / %s"),SourceName(Source,false),Playing&&ListeningPosition()?TEXT("PLAY"):TEXT("STOP"),*S47Script::TapeSeconds(Cursor,1,true,true),*S47Script::TapeSeconds(HeardO,1,true),*S47Script::TapeSeconds(HeardC,1,true),*(MarkIn<0?FString(TEXT("--.-")):S47Script::TapeSeconds(MarkIn)),*(MarkOut<0?FString(TEXT("--.-")):S47Script::TapeSeconds(MarkOut)),Reviewed?TEXT("RETAINED"):TEXT("COMPARE")):TEXT("NO POWER\nRESTORE FEEDER OR GENERATOR");
 if(Powered()&&!Original)Line=TEXT("NO RETAINED ANOMALOUS ORIGINAL\nCOLLECT ITS CHART AT THE PRINTER\nTHEN RECORD A RACK CONTROL");
 else if(Powered()&&Control<8)Line=FString::Printf(TEXT("INDEPENDENT CONTROL REQUIRED\nUSE RECEIVER RACK / %s OF 8 s\nORIGINAL WILL REMAIN INTACT"),*S47Script::TapeSeconds(Control,1,true));
 if(Powered()&&M->IsFreeform()&&M->Freeform)
 {
  // GAMEPLAY-LOOP-01: the extra-tick take is on the deck from the end of the take; one listen confirms it, no control asked for
  const auto* J=M->Freeform->CurrentJob();
  if(!J||!FS47FreeformSession::PlainAnomaly(*J)||J->Stage<FS47FreeformSession::EJobStage::Chart)Line=M->Freeform->NextAction();
  else if(FS47FreeformSession::RetainWaits(*J))Line=TEXT("TAKE NOT RETAINED YET\nRECEIVER (CENTRE PC), RECORD TAB\nRETAIN TAKE, THEN PLAY IT HERE"); // RX-ANOMALY-FLOW-20261113
  else Line=FString::Printf(TEXT("%s / %s / %s s\nHEARD %s/18  REF %s/8\n%s"),SourceName(Source,true),Playing&&ListeningPosition()?TEXT("PLAY"):TEXT("STOP"),*S47Script::TapeSeconds(Cursor,1,true,true),*S47Script::TapeSeconds(HeardO,1,true),*S47Script::TapeSeconds(HeardC,1,true),Reviewed?TEXT("ANOMALY CONFIRMED / TO OPS"):TEXT("PLAY, MARK THE TICKS, RETAIN"));
  if(J&&FS47FreeformSession::PlainAnomaly(*J)&&J->bLegacyTrace)Line+=TEXT("\nLEGACY AUDIO RECONSTRUCTION");
  const FString CaptureStatus=M->Freeform->AudioCaptureStatus();if(!CaptureStatus.IsEmpty())Line+=TEXT("\n")+CaptureStatus;

 }
 if(Powered()&&bReferenceView&&Reference)Line=Reference->Label();
 if(Powered()&&M->IsFreeform()&&M->Freeform&&M->Freeform->AudioPreparing())Line+=TEXT("\nLOADING TAPE");
 if(Line!=LastReadout&&Readout.IsValid()){Readout->SetText(FText::FromString(Line));LastReadout=Line;}
 // review 14 Nov (B3): the second key's caption names the station's tape as the readout does (REFERENCE in the plain anomaly flow)
 {const FString Key=SourceName(1,PlainNow());if(Texts.IsValidIndex(1)&&Texts[1].IsValid()&&Texts[1]->Text.ToString()!=Key)Texts[1]->SetText(FText::FromString(Key));}
 for(int32 I=0;I<Controls.Num();++I)if(Controls[I].Material.IsValid())
 {
  const bool Lit=Powered()&&((I==0&&Source==0)||(I==1&&Source==1)||(I==2&&Playing&&ListeningPosition())||(I==4&&MarkIn>=0)||(I==5&&MarkOut>=0)||(I==6&&Reviewed)||(I==8&&Reference&&Reference->Playing()));
  Controls[I].Material->SetVectorParameterValue(TEXT("Color"),Lit?FLinearColor(.24,.55,.30):FLinearColor(.12,.15,.13));
 }
}

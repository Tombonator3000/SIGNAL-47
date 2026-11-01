#include "S47FreeformAudioDesk.h"
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
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
 const TCHAR* Actions[]={TEXT("original"),TEXT("control"),TEXT("play"),TEXT("rewind"),TEXT("mark_start"),TEXT("mark_end"),TEXT("retain")};
 const TCHAR* Captions[]={TEXT("ORIGINAL"),TEXT("CONTROL"),TEXT("PLAY / STOP"),TEXT("REWIND"),TEXT("MARK IN"),TEXT("MARK OUT"),TEXT("RETAIN")};
 int32 ActionIndex(const FString& Id)
 {for(int32 I=0;I<7;++I)if(Id==FString(TEXT("ff:audio:"))+Actions[I])return I;return INDEX_NONE;}
}
FS47FreeformAudioDesk::~FS47FreeformAudioDesk(){Stop();}
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
 for(const auto& C:Controls)if(C.Mesh.IsValid()){C.Mesh->SetVisibility(bShow);C.Mesh->SetHiddenInGame(!bShow);C.Mesh->SetCollisionEnabled(bShow?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);}
 for(const auto& T:Texts)if(T.IsValid()){T->SetVisibility(bShow);T->SetHiddenInGame(!bShow);}
}
void FS47FreeformAudioDesk::Begin()
{
 Stop();if(!IsValid(M)||!IsValid(M->Web))return;
 if(Scene.Get()!=M->Web){bBuilt=false;Controls.Reset();Texts.Reset();Readout.Reset();Deck={};}
 Scene=M->Web;Life=MakeShared<uint8>(0);bActive=true;LastReadout.Empty();
 if(Build()){BindControls();Show(Ready());}
}
void FS47FreeformAudioDesk::Stop()
{
 bActive=false;Life.Reset();Show(false);
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
 for(int32 I=0;I<7;++I)
 {
  const int32 Col=I<4?I:I-4,Count=I<4?4:3;
  const FVector At(X+Width*((Col+.5)/Count-.5),Y+(I<4?.065:0),Z);
  auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web);Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.12,.15,.13));Mat->SetScalarParameterValue(TEXT("Roughness"),.65);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
  auto* C=NewObject<UStaticMeshComponent>(M->Web,*FString::Printf(TEXT("FreeformTapeKey%d"),I));C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(N->Comp.Get());C->SetStaticMesh(Cube);C->SetMaterial(0,Mat);
  C->SetWorldLocation(S47Three::ToUE(At));C->SetWorldScale3D(S47Three::DirToUE(FVector(Width/Count*.91,.048,.027)));C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);C->SetCastShadow(false);C->RegisterComponent();
  const FString Id=FString(TEXT("ff:audio:"))+Actions[I];const FS47Obj O=M->Web->ObjAt(M->Web->AdoptNode(C,Deck.I,Id));Controls.Add({Id,O,C,Mat});
  Text(FString::Printf(TEXT("FreeformTapeLabel%d"),I),Captions[I],At+FVector(0,0,-.016),1.1);
 }
 Readout=Text(TEXT("FreeformTapeReadout"),TEXT("SARO / ORIGINAL - INTERNAL REFERENCE"),FVector(X,Y+.172,Z-.002),1.15);
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
}
FString FS47FreeformAudioDesk::Label(const FString& Id)const
{
 const int32 I=ActionIndex(Id);if(!Ready()||I==INDEX_NONE)return FString();
 if(!Powered())return TEXT("Tape comparator unpowered / restore feeder or generator");
 if(I==2)return (M->IsFreeform()?M->Freeform->AudioPlaying():M->Observing->AudioPlaying())?TEXT("Stop tape playback"):TEXT("Play selected recording");
 static const TCHAR* Labels[]={TEXT("Select original recording"),TEXT("Select independent control"),TEXT("Play selected recording"),TEXT("Rewind selected tape"),TEXT("Mark start on the original"),TEXT("Mark end on the original"),TEXT("Retain measured comparison / then register at OPS")};
 return Labels[I];
}
bool FS47FreeformAudioDesk::Use(const FString& Id)
{
 const int32 I=ActionIndex(Id);if(I==INDEX_NONE||!Ready())return false;
 if(!ListeningPosition()){if(IsValid(M->UI))M->UI->Toast(TEXT("Use the powered tape comparator at the radio table."),3);return true;}
 const bool Free=M->IsFreeform();bool Accepted=true;
 if(Free)M->Freeform->OpenAudio();else M->Observing->OpenAudioComparison();
 const bool Playing=Free?M->Freeform->AudioPlaying():M->Observing->AudioPlaying();
 const FString Action=I==2?(Playing?TEXT("pause"):TEXT("play")):I==6?TEXT("confirm"):Actions[I];
 if(I==3&&Free)M->Freeform->SeekAudio(0);
 else Accepted=Free?M->Freeform->AudioAction(Action):M->Observing->AudioAction(I==3?TEXT("rewind"):Action);
 if(IsValid(M->UI))
 {
  const FString Feedback=Free?M->Freeform->FeedbackText():M->Observing->AudioFeedbackText();
  if(!Accepted)M->UI->Toast(Feedback.IsEmpty()?TEXT("Retain an anomalous original and complete its independent control before comparing."):Feedback,4);
  else if(I==6&&Free)M->UI->Toast(TEXT("Comparison retained. Register the supported conclusion at OPS."),4);
 }
 return true;
}
void FS47FreeformAudioDesk::Step(double Dt)
{
 if(!bActive||!Live())return;
 if(!bBuilt){if(!Build())return;BindControls();}
 const bool Shown=Ready()&&M->Web->Area()==TEXT("saro");Show(Shown);if(!Shown)return;
 int32 Source=0;double Cursor=0,HeardO=0,HeardC=0,MarkIn=-1,MarkOut=-1,Control=0;bool Playing=false,Reviewed=false,Original=false;
 if(M->IsFreeform())
 {
  const auto& F=*M->Freeform;const auto* J=F.CurrentJob();Source=F.AudioSource();Cursor=F.AudioPosition();Playing=F.AudioPlaying();
  if(J){HeardO=J->OriginalCovered;HeardC=J->ControlCovered;MarkIn=J->MarkStart;MarkOut=J->MarkEnd;Control=J->ControlAt;Reviewed=J->bAudioReviewed;Original=J->bAnomaly&&J->Stage>=FS47FreeformSession::EJobStage::Recorded;}
 }
 else
 {
  const auto& O=*M->Observing;Source=O.AudioSource();Cursor=O.AudioPosition();Playing=O.AudioPlaying();Reviewed=O.AudioReviewed();HeardO=O.AudioHeard(0);HeardC=O.AudioHeard(1);MarkIn=O.AudioMarkedStart();MarkOut=O.AudioMarkedEnd();Control=O.ControlElapsed();Original=O.LoopStage()>=FS47Observing::ELoopStage::Rack;
 }
 FString Line=Powered()?FString::Printf(TEXT("%s / %s / %04.1f s\nHEARD O %.1f/18  C %.1f/8\nMARK %.1f - %.1f / %s"),Source==0?TEXT("ORIGINAL"):TEXT("CONTROL"),Playing&&ListeningPosition()?TEXT("PLAY"):TEXT("STOP"),Cursor,HeardO,HeardC,MarkIn,MarkOut,Reviewed?TEXT("RETAINED"):TEXT("COMPARE")):TEXT("NO POWER\nRESTORE FEEDER OR GENERATOR");
 if(Powered()&&!Original)Line=TEXT("NO RETAINED ANOMALOUS ORIGINAL\nCOLLECT ITS CHART AT THE PRINTER\nTHEN RECORD A RACK CONTROL");
 else if(Powered()&&Control<8)Line=FString::Printf(TEXT("INDEPENDENT CONTROL REQUIRED\nUSE RECEIVER RACK / %.1f OF 8 s\nORIGINAL WILL REMAIN INTACT"),Control);
 if(Line!=LastReadout&&Readout.IsValid()){Readout->SetText(FText::FromString(Line));LastReadout=Line;}
 for(int32 I=0;I<Controls.Num();++I)if(Controls[I].Material.IsValid())
 {
  const bool Lit=Powered()&&((I==0&&Source==0)||(I==1&&Source==1)||(I==2&&Playing&&ListeningPosition())||(I==4&&MarkIn>=0)||(I==5&&MarkOut>=0)||(I==6&&Reviewed));
  Controls[I].Material->SetVectorParameterValue(TEXT("Color"),Lit?FLinearColor(.24,.55,.30):FLinearColor(.12,.15,.13));
 }
}

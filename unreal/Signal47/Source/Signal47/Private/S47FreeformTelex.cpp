// TELEX-20261109 (Tom, 9 November, DECISIONS "Saker og OPS"): the control room's telex as paper. What comes back to SARO prints on
// the machine with its motor and type bars, the paper grows out of the top, and the player tears it off there (FS47FreeformSession,
// FTelexSheet in S47Freeform.h). The printing follows the printer's own rule (StepPrinter): it feeds while there is power and the game
// runs, and is heard only at SARO.
#include "S47Freeform.h"
#include "S47Audio.h"
#include "S47Docs.h"
#include "S47Main.h"
#include "S47NightLog.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47WardReport.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47FreeformTelex, Log, All);

namespace S47FreeformTelexLocal
{
 // The machine is three's group "room.objs.telex" (the web version's ControlRoom: a cabinet under a head whose top is 0.88 m up, the
 // keyboard toward -z, the roll behind it). The paper comes out of the top of the head on the operator's side and leans back a
 // little. SEE IN THE GAME: the front offset and the lean are from the web version's measures, not measured in Unreal.
 constexpr double Top=.88,Front=-.05,Lean=.3,Wide=.21,Thin=.003,Least=.03,Most=.42;
 // The type bars' pace, characters a second, as Hondo's telex prints (S47SaroDark), and a carriage return every so many.
 constexpr double Cps=10.5;constexpr int32 LineChars=34;
 bool Valid(const FS47FreeformSession::FTelexSheet& S){return !S.DocId.IsEmpty()&&S.DocId.Len()<=96&&!S.Page.IsEmpty()&&S.Page.Len()<=6000&&S.Title.Len()<=200&&S.Stamp.Len()<=96;}
 // The machine in three's metres, for its sound: the web version's place when the scene has no such group.
 FVector MachineAt(US47Main* M)
 {
  if(M&&IsValid(M->Web.Get()))if(const FS47Obj O=M->Web->Obj(TEXT("room.objs.telex"));O.IsValid()){const FVector P=O.WorldPosition();return FVector(P.X,.9,P.Z);}
  return FVector(-4.2,.9,4.18);
 }
}

bool FS47FreeformSession::QueueTelex(const FString& DocId,const FString& Title,const FString& Stamp,const FString& Page)
{
 FTelexSheet S;S.DocId=DocId;S.Title=Title.IsEmpty()?FString(TEXT("Telex")):Title;S.Stamp=Stamp;S.Page=Page;S.Day=FMath::Max(1,DayNumber);
 if(!S47FreeformTelexLocal::Valid(S)||TelexSheets.Num()>=MostTelex||OnTelex(DocId)||InTelexTray(DocId))return false;
 TelexSheets.Add(S);
 UE_LOG(LogS47FreeformTelex,Display,TEXT("S47: telex: %s queued, %d on the machine"),*DocId,TelexSheets.Num());
 Changed();return true;
}
bool FS47FreeformSession::OnTelex(const FString& DocId)const{return TelexSheets.ContainsByPredicate([&DocId](const FTelexSheet& S){return S.DocId==DocId;});}
bool FS47FreeformSession::InTelexTray(const FString& DocId)const{return TelexTorn.ContainsByPredicate([&DocId](const FTelexSheet& S){return S.DocId==DocId;});}
bool FS47FreeformSession::TelexReady()const{return !TelexSheets.IsEmpty()&&TelexSheets[0].Feed>=TelexSeconds;}
FString FS47FreeformSession::TelexLine()const
{
 // Tom's words on the HUD (DECISIONS): "TELEX: A reply is waiting".
 if(TelexSheets.IsEmpty())return FString();
 return TelexSheets.Num()==1?FString(TEXT("TELEX: A reply is waiting")):FString::Printf(TEXT("TELEX: %d papers are waiting"),TelexSheets.Num());
}
bool FS47FreeformSession::TearOffTelex(FS47Doc* Out)
{
 if(!TelexReady())return false;
 // The roll is torn once: everything printed comes off together, each paper its own document; the first is the one read now.
 TArray<FS47Doc> Papers;
 while(TelexReady())
 {
  FTelexSheet S=TelexSheets[0];TelexSheets.RemoveAt(0);
  S.Day=FMath::Max(1,DayNumber);S.Feed=TelexSeconds;
  TelexTorn.Add(S);
  FS47Doc Doc;Doc.Id=S.DocId;Doc.Title=S.Title;Doc.Kind=TEXT("telex");Doc.Stamp=S.Stamp;Doc.Page=S.Page;Doc.Transcript=S.Page;
  Papers.Add(Doc);
  if(M&&M->Story)M->Story->AddDoc(Doc);
  // What the paper answers, its owner knows: Dr. Ward's line goes on the night log or the weekly report it is about.
  if(S.DocId.StartsWith(TEXT("telex-nightlog-"))&&NightLog)NightLog->Received(FCString::Atoi(*S.DocId.Mid(15)));
  if(S.DocId.StartsWith(TEXT("telex-wardreport-"))&&WardReport)WardReport->Received(FCString::Atoi(*S.DocId.Mid(17)));
  UE_LOG(LogS47FreeformTelex,Display,TEXT("S47: telex: %s torn off, into the notebook and the OPS tray"),*S.DocId);
 }
 while(TelexTorn.Num()>MostTray)TelexTorn.RemoveAt(0);
 TelexStrikes=0;
 if(Out)*Out=Papers[0];
 if(M&&IsValid(M->Audio))M->Audio->Sfx(TEXT("click"),FS47Sound{.Gain=.4f,.At=S47FreeformTelexLocal::MachineAt(M)});
 if(M&&M->UI)M->UI->Document(Papers[0]);
 Feedback=Papers.Num()==1?FString::Printf(TEXT("Torn off the telex: %s. It is in your notebook and in the tray at OPS."),*Papers[0].Title)
  :FString::Printf(TEXT("Torn off the telex: %d papers. They are in your notebook and in the tray at OPS."),Papers.Num());
 Changed(Feedback);
 StepTelex(0); // the paper on the machine as it is now
 return true;
}
void FS47FreeformSession::StepTelex(double Dt)
{
 using namespace S47FreeformTelexLocal;
 // The first paper not yet printed feeds; the others wait behind it, as on the roll.
 FTelexSheet* Printing=TelexSheets.FindByPredicate([](const FTelexSheet& S){return S.Feed<TelexSeconds;});
 const bool bRunning=Printing&&HasPower()&&!Held();
 const bool bOnSite=M&&IsValid(M->Web.Get())&&M->Web->IsLoaded()&&M->Web->Area()==TEXT("saro")&&M->World&&M->World->Area==TEXT("saro");
 const FVector At3=MachineAt(M);
 if(bRunning)
 {
  const double Before=Printing->Feed;
  Printing->Feed=FMath::Min(TelexSeconds,Printing->Feed+FMath::Max(0.,Dt));
  // The type bars at the machine's pace; after a long frame (or a Continue) only the last few, never a burst.
  const int32 Strikes=int32(Printing->Feed*Cps);
  if(Strikes-TelexStrikes>3)TelexStrikes=Strikes-1;
  for(;TelexStrikes<Strikes;++TelexStrikes)if(bOnSite&&IsValid(M->Audio))M->Audio->TypeBar(At3,TelexStrikes%LineChars==LineChars-1);
  if(Before<TelexSeconds&&Printing->Feed>=TelexSeconds)
  {
   TelexStrikes=0;
   Changed(TelexLine()+TEXT(". Tear it off at the telex in the control room."));
  }
 }
 const bool bMotor=bRunning&&bOnSite&&M&&IsValid(M->Audio);
 if(bMotor!=bTelexMotor)
 {
  bTelexMotor=bMotor;
  if(M&&IsValid(M->Audio))M->Audio->TelexMotor(bMotor?TOptional<FVector>(At3):TOptional<FVector>());
 }
 // The paper: built at the machine the first time there is any, then only shown, hidden and lengthened.
 if(!M||!IsValid(M->Web.Get())||!M->Web->IsLoaded())return;
 const int32 Node=M->Web->Handle(TEXT("room.objs.telex"));
 if(!(TelexScene.Get()==M->Web.Get()&&TelexPaper.IsValid()))
 {
  TelexPaper.Reset();
  const int32 Root=M->Web->Handle(TEXT("room.group"));
  if(TelexSheets.IsEmpty()||Node<0||Root<0)return; // nothing to show, or no telex in this scene
  auto* Box=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
  auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));if(!Box||!Base)return;
  auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web.Get());
  // the roll's paper, yellowish (the web version's telexPaperTex)
  Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.86f,.82f,.66f));Mat->SetScalarParameterValue(TEXT("Roughness"),.9f);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
  auto* C=NewObject<UStaticMeshComponent>(M->Web.Get(),MakeUniqueObjectName(M->Web.Get(),UStaticMeshComponent::StaticClass(),TEXT("ff_telexPaper")));
  C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(M->Web->GetRootComponent());C->SetStaticMesh(Box);C->SetMaterial(0,Mat);C->SetCastShadow(false);
  // Only to be seen: the telex's own box keeps answering the use trace.
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->RegisterComponent();
  if(M->Web->AdoptNode(C,Root,TEXT("ff:telexPaper"))<0){C->DestroyComponent();return;}
  TelexPaper=C;TelexScene=M->Web;
 }
 UStaticMeshComponent* C=TelexPaper.Get();
 if(TelexSheets.IsEmpty()||Node<0){if(!C->bHiddenInGame)C->SetHiddenInGame(true);return;}
 // As long as what has printed: the first paper's share of its print, a little more for each one printed under it.
 int32 Printed=0;double Share=0;
 for(const FTelexSheet& S:TelexSheets){if(S.Feed>=TelexSeconds)++Printed;else{Share=S.Feed/TelexSeconds;break;}}
 const double Length=FMath::Min(Most,Least+.3*(Printed>0?1.:Share)+.05*FMath::Max(0,Printed-1)+(Printed>0?.05*Share:0.));
 const FTransform Machine=M->Web->NodeWorldThree(Node);
 const FQuat Tilt=S47Three::Euler(Lean,0,0);
 const FVector Up=Tilt.RotateVector(FVector(0,1,0));
 const FVector Centre=Machine.TransformPosition(FVector(0,Top,Front)+Up*(Length*.5));
 double E[16];S47Three::Compose(Centre,Machine.GetRotation()*Tilt,FVector(Wide,Length,Thin),E);
 C->SetWorldTransform(S47Three::MatToUE(E));
 if(C->bHiddenInGame)C->SetHiddenInGame(false);
}
void FS47FreeformSession::StopTelex()
{
 const bool bAlive=!IsEngineExitRequested()&&!IsGarbageCollecting();
 if(bTelexMotor){bTelexMotor=false;if(bAlive&&IsValid(M)&&IsValid(M->Audio))M->Audio->TelexMotor(TOptional<FVector>());}
 if(bAlive&&TelexPaper.IsValid())TelexPaper->SetHiddenInGame(true);
 TelexStrikes=0;
}
void FS47FreeformSession::LoadTelex(const S47J::FObj& Saved)
{
 TelexSheets.Reset();TelexTorn.Reset();TelexStrikes=0;
 const S47J::FObj T=S47J::Obj(Saved,TEXT("telex"));if(!T)return;
 const auto Read=[](const TSharedPtr<FJsonValue>& V,FTelexSheet& S)
 {
  if(!V||V->Type!=EJson::Object)return false;const S47J::FObj O=V->AsObject();if(!O)return false;
  S.DocId=S47J::Str(O,TEXT("doc"));S.Title=S47J::Str(O,TEXT("title"));S.Stamp=S47J::Str(O,TEXT("stamp"));S.Page=S47J::Str(O,TEXT("page"));
  const double Day=S47J::Num(O,TEXT("day"),1),Feed=S47J::Num(O,TEXT("feed"));
  S.Day=FMath::IsFinite(Day)?int32(FMath::Clamp(Day,1.,36500.)):1;S.Feed=FMath::IsFinite(Feed)?FMath::Clamp(Feed,0.,TelexSeconds):0.;
  return S47FreeformTelexLocal::Valid(S);
 };
 for(const auto& V:S47J::Arr(T,TEXT("sheets"))){FTelexSheet S;if(Read(V,S)&&TelexSheets.Num()<MostTelex&&!OnTelex(S.DocId))TelexSheets.Add(S);}
 for(const auto& V:S47J::Arr(T,TEXT("tray"))){FTelexSheet S;if(Read(V,S)&&!InTelexTray(S.DocId)&&!OnTelex(S.DocId))TelexTorn.Add(S);}
 while(TelexTorn.Num()>MostTray)TelexTorn.RemoveAt(0);
 // A paper half printed goes on where it was, without the strikes it already had.
 if(const FTelexSheet* P=TelexSheets.FindByPredicate([](const FTelexSheet& S){return S.Feed<TelexSeconds;}))TelexStrikes=int32(P->Feed*S47FreeformTelexLocal::Cps);
}
void FS47FreeformSession::SaveTelex(const S47J::FObj& Out)const
{
 if(!Out||(TelexSheets.IsEmpty()&&TelexTorn.IsEmpty()))return; // an older save, or nothing yet: nothing written
 const auto Write=[](const TArray<FTelexSheet>& List)
 {
  TArray<TSharedPtr<FJsonValue>> A;
  for(const FTelexSheet& S:List)
  {
   auto X=S47J::New();S47J::Set(X,TEXT("doc"),S.DocId);S47J::Set(X,TEXT("title"),S.Title);S47J::Set(X,TEXT("stamp"),S.Stamp);S47J::Set(X,TEXT("page"),S.Page);
   S47J::Set(X,TEXT("day"),S.Day);S47J::Set(X,TEXT("feed"),S.Feed);A.Add(MakeShared<FJsonValueObject>(X));
  }
  return A;
 };
 auto T=S47J::New();T->SetArrayField(TEXT("sheets"),Write(TelexSheets));T->SetArrayField(TEXT("tray"),Write(TelexTorn));
 S47J::Set(Out,TEXT("telex"),T);
}

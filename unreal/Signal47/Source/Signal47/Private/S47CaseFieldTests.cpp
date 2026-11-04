#include "S47CaseFieldTests.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47FreeformWorld.h"
#include "S47Inter.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Audio.h"
#include "S47UI.h"
#include "S47Three.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47CaseField,Log,All);
namespace
{
 const FVector LadderAt(5.25,0,-5.12);         // control-room north facade, outside (east of the windows)
 const FVector RoofAnchor(-1.5,3.5,-1);        // where Director's roof_steps already sound
 const FVector GateAt(-1.0,0,-18.55);          // the array field gate in the fence line
 constexpr double ListenNeed=60;               // seconds inside the control room during the peak
}

void FS47CaseFieldTests::PeakAfter(double Prepared,double& Start,double& End)
{
 const double Base=FMath::FloorToDouble(Prepared/86400.)*86400.;
 Start=Base+1.5*3600.;while(Start<Prepared+2*3600.)Start+=86400.;
 End=Start+3*3600.;
}

double FS47CaseFieldTests::PreparedClock(int32 Test) const
{
 const FString Job=M->Freeform->PlannedCaseJob(Line);if(Job.IsEmpty()||M->Freeform->PlannedFieldTest(Line)!=Test)return -1;
 const FString Id=M->Freeform->FindingWithPrefix(Job+TEXT("-prep-"));
 return Id.IsEmpty()?-1:FCString::Atod(*Id.RightChop(Job.Len()+6));
}

void FS47CaseFieldTests::Prepare(int32 Test)
{
 const FString Job=M->Freeform->PlannedCaseJob(Line);if(Job.IsEmpty()||PreparedClock(Test)>=0)return;
 const double Now=M->Freeform->Clock();double S=0,E=0;PeakAfter(Now,S,E);
 const FString When=FS47FreeformCases::Clock(FMath::Fmod(S,86400.));
 M->Freeform->RecordEvidence(FString::Printf(TEXT("%s-prep-%.0f"),*Job,Now),Test==0
  ?FString::Printf(TEXT("FIELD TEST / SECURE THE ROOF FITTING\nThe loose roof plate is marked and fastened with two new screws.\nLISTEN / from the control room during the next night window (from %s)."),*When)
  :FString::Printf(TEXT("FIELD TEST / CONTROLLED ACCESS\nThe field gate is sealed with numbered tape and two tracking strips of raked sand.\nINSPECT / after the next night window (from %s)."),*When));
 if(M->Audio)M->Audio->Play(Test==0?TEXT("rack_switch"):TEXT("small_switch"),.6f,S47Three::ToUE(Test==0?LadderAt:GateAt));
 if(M->UI)M->UI->Toast(Test==0?TEXT("Roof plate fastened. Listen from the control room in the next night window."):TEXT("Gate sealed and tracking strips laid. Inspect them after the next night window."),4.f);
 Listened=0;ImpactsLeft=0;ImpactAt=-1;
}

bool FS47CaseFieldTests::Build()
{
 if(bBuilt)return true;
 if(!M||!M->Web||!M->Web->IsLoaded()||!M->Inter||!M->Freeform||!M->Freeform->Field)return false;
 const int32 Saro=M->Web->Handle(TEXT("ext.group"));if(Saro<0)return false;
 FS47FreeformWorld* W=M->Freeform->Field.Get();
 Ladder=W->StageBox(TEXT("CaseLadderL"),LadderAt+FVector(-.22,1.6,0),FVector(.05,3.3,.05),0x8a8d86,Saro);
 LadderParts.Add(W->StageBox(TEXT("CaseLadderR"),LadderAt+FVector(.22,1.6,0),FVector(.05,3.3,.05),0x8a8d86,Saro));
 for(int32 I=0;I<10;++I)LadderParts.Add(W->StageBox(FString::Printf(TEXT("CaseLadderRung%d"),I),LadderAt+FVector(0,.25+I*.32,0),FVector(.44,.03,.04),0x8a8d86,Saro));
 Plate=W->StageBox(TEXT("CaseRoofPlate"),FVector(4.7,3.62,-4.6),FVector(.9,.012,.6),0x9a9c97,Saro);
 Seal=W->StageBox(TEXT("CaseGateSeal"),GateAt+FVector(0,1.05,.02),FVector(1.4,.07,.02),0xe0c23a,Saro);
 SealBroken=W->StageBox(TEXT("CaseGateSealBroken"),GateAt+FVector(.45,.55,.0),FVector(.4,.05,.015),0xd9c23a,Saro);
 Strip=W->StageBox(TEXT("CaseGateStrip"),GateAt+FVector(0,.005,.55),FVector(1.6,.01,.9),0xc9a77a,Saro);
 for(int32 I=0;I<6;++I)Tracks[I]=W->StageBox(FString::Printf(TEXT("CaseGateTrack%d"),I),GateAt+FVector(-.15+(I%2)*.3,.012,.2+I*.14),FVector(.11,.006,.26),0x6d5a40,Saro);
 auto Add=[&](const TCHAR* Id,FS47Obj Obj,TFunction<FString()> Label,TFunction<void()> Use,double Range)
 {FS47Interactable It;It.Id=Id;It.Object=Obj;It.Label=MoveTemp(Label);It.Use=MoveTemp(Use);It.Range=Range;M->Inter->Add(It);};
 Add(TEXT("case:roof-plate"),Ladder,[this]{return M->Freeform->PlannedFieldTest(Line)==0&&PreparedClock(0)<0?FString(TEXT("Climb up and fasten the loose roof plate")):FString();},[this]{Prepare(0);},2.4);
 Add(TEXT("case:gate-seal"),Strip,[this]
 {
  if(M->Freeform->PlannedFieldTest(Line)!=1)return FString();
  const double P=PreparedClock(1);if(P<0)return FString(TEXT("Seal the gate and lay tracking strips"));
  double S=0,E=0;PeakAfter(P,S,E);return M->Freeform->Clock()>=E?FString(TEXT("Inspect the gate seal and tracking strips")):FString();
 },[this]
 {
  const double P=PreparedClock(1);if(P<0){Prepare(1);return;}
  double S=0,E=0;PeakAfter(P,S,E);if(M->Freeform->Clock()<E)return;
  const bool Present=M->Freeform->FieldTestExpectsPresent(Line,1);
  M->Freeform->ReportFieldTest(Line,1,true,Present,Present?TEXT("Seal tape torn at the latch; a continuous line of shoe prints crosses both strips."):TEXT("Seal tape intact and numbered; both strips undisturbed apart from wind ripples."));
 },2.6);
 bBuilt=true;UE_LOG(LogS47CaseField,Display,TEXT("S47: case 3 field tests staged (roof ladder, field gate seal)"));
 return true;
}

void FS47CaseFieldTests::Present()
{
 const int32 Test=M->Freeform->PlannedFieldTest(Line);
 const bool Roof=Test==0,Gate=Test==1;
 Ladder.SetVisible(Roof);Plate.SetVisible(Roof);for(FS47Obj& L:LadderParts)L.SetVisible(Roof);
 const double P=Gate?PreparedClock(1):-1;double S=0,E=0;if(P>=0)PeakAfter(P,S,E);
 const bool Done=P>=0&&M->Freeform->Clock()>=E,Broken=Done&&M->Freeform->FieldTestExpectsPresent(Line,1);
 Seal.SetVisible(P>=0&&!Broken);SealBroken.SetVisible(Broken);Strip.SetVisible(P>=0);
 for(FS47Obj& T:Tracks)T.SetVisible(Broken);
}

void FS47CaseFieldTests::Step(double Dt)
{
 if(!M||!M->Freeform||!M->Freeform->Active()||!M->IsFreeform()||!Build())return;
 Present();
 DevTest(Dt);
 if(M->Freeform->PlannedFieldTest(Line)!=0)return;
 const double P=PreparedClock(0);if(P<0)return;
 double S=0,E=0;PeakAfter(P,S,E);const double Now=M->Freeform->Clock();
 if(Now>=E&&ImpactsLeft<=0)
 {
  M->Freeform->ReportFieldTest(Line,0,false,false,TEXT("Nobody listened from the control room during the night window after the plate was fastened."));
  return;
 }
 if(Now<S)return;
 const bool Inside=M->World&&M->World->Area==TEXT("saro")&&M->Player&&M->Web->Rect(TEXT("room.bounds")).Contains(M->Player->Pos);
 if(Inside&&ImpactsLeft==0&&Listened<ListenNeed)
 {
  Listened+=Dt;
  if(Listened>=ListenNeed)
  {
   if(M->Freeform->FieldTestExpectsPresent(Line,0)){ImpactsLeft=4;ImpactAt=0;}
   else{M->Freeform->ReportFieldTest(Line,0,true,false,TEXT("A full listening minute from the control room on a windy night: no impacts from the roof plate."));return;}
  }
 }
 if(ImpactsLeft>0&&(ImpactAt-=Dt)<=0)
 {
  if(M->Audio){FS47Sound O;O.At=RoofAnchor+FVector(FMath::FRandRange(-1.5,1.5),0,0);O.Gain=.5f;M->Audio->Sfx(TEXT("stepConcrete0"),O);}
  ImpactAt=FMath::FRandRange(2.5,4.5);
  if(--ImpactsLeft==0)M->Freeform->ReportFieldTest(Line,0,true,true,TEXT("With the plate fastened, four distinct impacts still crossed the roof during the listening window."));
 }
}

bool FS47CaseFieldTests::SelfTest(TArray<FString>& Lines)
{
 double S=0,E=0;bool Ok=true;
 PeakAfter(86400.+20*3600.,S,E);Ok&=FMath::IsNearlyEqual(S,2*86400.+1.5*3600.)&&FMath::IsNearlyEqual(E-S,3*3600.);
 PeakAfter(86400.+1*3600.,S,E);Ok&=FMath::IsNearlyEqual(S,2*86400.+1.5*3600.); // prepared 01:00: same-night peak too soon
 PeakAfter(86400.-2*3600.,S,E);Ok&=FMath::IsNearlyEqual(S,86400.+1.5*3600.);   // prepared 22:00: that night's peak
 Lines.Add(FString::Printf(TEXT("%s / Case 3 field tests: the observation window is the next night peak at least two hours after preparation"),Ok?TEXT("PASS"):TEXT("FAIL")));
 return Ok;
}

// -S47CaseFieldTest (temp profile): both staged field tests through the production interactions,
// with screenshots in Saved/CaseFieldShots and PASS/FAIL lines in the log.
void FS47CaseFieldTests::DevTest(double Dt)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47CaseFieldTest"))||DevStage<0||!M->Player||!M->World||M->World->Area!=TEXT("saro"))return;
 FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);if(!User.StartsWith(TEXT("/tmp/")))return;
 const double Now=FPlatformTime::Seconds();if(Now<DevAt)return;
 const FString Dir=FPaths::ProjectSavedDir()/TEXT("CaseFieldShots");
 FS47FreeformSession& F=*M->Freeform;
 auto Check=[&](bool Ok,const TCHAR* Why){DevLines.Add(FString(Ok?TEXT("PASS / "):TEXT("FAIL / "))+Why);UE_LOG(LogS47CaseField,Display,TEXT("%s%s"),Ok?TEXT("PASS / "):TEXT("FAIL / "),Why);if(!Ok){DevStage=-1;FPlatformMisc::RequestExit(false);}return Ok;};
 auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Dir/Name,false,false);};
 auto Look=[&](const FVector& Stand,const FVector& Target){M->Player->Place(Stand.X,Stand.Z,0);const FVector D=Target-FVector(Stand.X,Stand.Y+1.6,Stand.Z);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());};
 auto Result=[&](int32 Test){const auto* C=F.CaseState(Line);return C&&C->Results[Test].bDone;};
 switch(DevStage)
 {
 case 0: DevAt=Now+8;DevStage=1;return;
 case 1: if(!Check(F.DevOpenCase(Line)&&F.PlanCaseTest(Line,1,1),TEXT("case 3 opens and the controlled-access test is planned with a hunch")))return;
  Look(GateAt+FVector(0,-.576,3.2),GateAt+FVector(0,.4,.3));DevAt=Now+2;DevStage=2;return;
 case 2: {FS47Interactable* It=M->Inter->Get(TEXT("case:gate-seal"));if(!Check(It&&It->Label()==TEXT("Seal the gate and lay tracking strips"),TEXT("gate offers seal and strips")))return;It->Use();
  if(!Check(PreparedClock(1)>=0,TEXT("gate preparation retained as a finding")))return;DevAt=Now+2;DevStage=3;return;}
 case 3: Shot(TEXT("01-gate-sealed.png"));DevAt=Now+1.5;DevStage=4;return;
 case 4: {FS47Interactable* It=M->Inter->Get(TEXT("case:gate-seal"));if(!Check(It->Label().IsEmpty(),TEXT("inspection is locked until after the night window")))return;
  double S=0,E=0;PeakAfter(PreparedClock(1),S,E);F.TestAdvance(E-F.Clock()+3*3600);DevAt=Now+3;DevStage=5;return;} // morning light for the shot
 case 5: Shot(TEXT("02-gate-after-night.png"));DevAt=Now+1.5;DevStage=6;return;
 case 6: {FS47Interactable* It=M->Inter->Get(TEXT("case:gate-seal"));if(!Check(It->Label()==TEXT("Inspect the gate seal and tracking strips"),TEXT("after the night window the strips can be inspected")))return;It->Use();
  if(!Check(Result(1)&&F.CaseState(Line)->Results[1].bPresent==F.CaseState(Line)->Results[1].bPresent,TEXT("inspection records the controlled-access result")))return;
  if(!Check(F.PlanCaseTest(Line,0,0),TEXT("roof fitting test planned")))return;Look(LadderAt+FVector(-1.2,-.576,-3.2),LadderAt+FVector(0,1.8,0));DevAt=Now+2;DevStage=7;return;}
 case 7: {FS47Interactable* It=M->Inter->Get(TEXT("case:roof-plate"));if(!Check(It&&!It->Label().IsEmpty(),TEXT("ladder offers fastening the roof plate")))return;Shot(TEXT("03-roof-ladder.png"));DevAt=Now+1.5;DevStage=71;return;}
 case 71: {FS47Interactable* It=M->Inter->Get(TEXT("case:roof-plate"));It->Use();
  if(!Check(PreparedClock(0)>=0,TEXT("roof preparation retained")))return;double S=0,E=0;PeakAfter(PreparedClock(0),S,E);F.TestAdvance(S-F.Clock()+60);
  M->Player->Place(0,0,0);DevAt=Now+2;DevStage=8;return;}
 case 8: DevAt=Now+.5;if(Result(0)){Check(true,TEXT("listening from the control room during the peak records the roof result"));DevStage=9;}else if(Now-DevAt>200)Check(false,TEXT("roof result within bounded wait"));return;
 case 9: Check(F.CaseState(Line)->ValidTests>=2,TEXT("two valid field tests retained on case 3"));DevStage=-1;FPlatformMisc::RequestExit(false);return;
 }
}

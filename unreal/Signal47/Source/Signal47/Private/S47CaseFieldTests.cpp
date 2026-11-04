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
#include "S47Wind.h"
#include "S47Three.h"
#include "Components/PrimitiveComponent.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "UObject/GarbageCollection.h"

namespace
{
 const FVector LadderAt(5.25,0,-5.12),RoofAnchor(-1.5,3.5,-1),GateAt(-1.0,0,-20.0);
 constexpr double ListenNeed=60;
 FString When(double Clock)
 {
  const int32 Day=FMath::Max(1,int32(FMath::FloorToDouble((Clock-8*3600)/86400.))+1);
  return FString::Printf(TEXT("SHIFT DAY %d / %s"),Day,*FS47FreeformCases::Clock(FMath::Fmod(Clock,86400.)));
 }
}
FS47CaseFieldTests::~FS47CaseFieldTests(){Stop();Life.Reset();}
void FS47CaseFieldTests::PeakAfter(double Prepared,double& Start,double& End)
{
 const double Day=FMath::FloorToDouble(Prepared/86400.);
 Start=Day*86400.+1.5*3600.;if(Start<Prepared+2*3600.)Start+=86400.;End=Start+3*3600.;
}
double FS47CaseFieldTests::Overlap(double From,double To,double Start,double End)
{return FMath::Max(0.,FMath::Min(To,End)-FMath::Max(From,Start));}
bool FS47CaseFieldTests::ComparableWind(double Reference,const FVector2D& Direction,double Strength,const FVector2D& Current)
{
 return FMath::IsFinite(Reference)&&FMath::IsFinite(Strength)&&Reference>=0
  &&Strength>=FMath::Max(.035,Reference*.8)&&FVector2D::DotProduct(Direction.GetSafeNormal(),Current.GetSafeNormal())>=.70710678;
}
int32 FS47CaseFieldTests::PlannedTest()const
{return M&&M->Freeform&&M->Freeform->Active()?M->Freeform->PlannedFieldTest(Line):-1;}
FString FS47CaseFieldTests::PlannedJob()const
{
 const auto* C=M&&M->Freeform?M->Freeform->CaseFile().Get(Line):nullptr;
 return C&&PlannedTest()>=0?C->PlannedJob:FString();
}
bool FS47CaseFieldTests::Prepared()const
{return bActive&&R.Prepared>=0&&R.Test==PlannedTest()&&R.Job==PlannedJob();}
void FS47CaseFieldTests::ResetPlan(){R=FRun();Present();}
void FS47CaseFieldTests::SyncPlan()
{
 if(R.Test!=PlannedTest()||R.Job!=PlannedJob()){R=FRun();R.Test=PlannedTest();R.Job=PlannedJob();}
}
bool FS47CaseFieldTests::Window(double& Start,double& End)const
{
 const int32 Test=PlannedTest();if(Test<0||Test>1)return false;
 if(Prepared()){Start=R.Start;End=R.End;}else PeakAfter(M->Freeform->Clock(),Start,End);
 return true;
}
bool FS47CaseFieldTests::OwnsRoofWindow()const
{return Prepared()&&R.Test==0&&M->Freeform->Clock()>=R.Start&&M->Freeform->Clock()<R.End;}
FString FS47CaseFieldTests::Objective()const
{
 const int32 Test=PlannedTest();if(Test<0||Test>1)return FString();
 double Start=0,End=0;Window(Start,End);
 const FString Times=When(Start)+TEXT(" to ")+When(End)+TEXT(" (after midnight)");
 if(!Prepared())return (Test==0?FString(TEXT("SARO NORTH FACADE: FASTEN THE ROOF PLATE AT THE LADDER. ")):FString(TEXT("ARRAY SERVICE GATE: SEAL THE LATCH AND LAY THE TRACKING STRIPS. ")))
  +TEXT("If prepared now: ")+Times+TEXT(". Preparation needs two hours before the window; late preparation moves it to the following night.");
 const double Now=M->Freeform->Clock();
 if(Now<R.Start)return FString(Test==0?TEXT("CONTROL ROOM: RETURN TO LISTEN UNDER THE FASTENED ROOF. "):TEXT("SERVICE GATE CONTROL IS SET; INSPECT AFTER THE WINDOW. "))+Times+TEXT(". A time jump through the window supplies no observation.");
 if(Test==1)return Now<R.End?TEXT("SEALED SERVICE GATE: CONTROL WINDOW RUNNING / ")+Times+TEXT(". Leave tape and strips undisturbed; do other work without sleeping through the window.")
  :TEXT("ARRAY SERVICE GATE: INSPECT THE TAPE AND TRACKING STRIPS. ")+(R.Gaps>0||R.bGateTampered?FString(TEXT("Control incomplete: unobserved time or operator disturbance; inspection records no negative result.")):FString(TEXT("The active window is complete.")));
 if(Now>=R.End)return TEXT("ROOF CONTROL INCOMPLETE: THE LISTENING WINDOW ENDED. Plan the roof test again; no silence or cause is inferred from missed time.");
 const auto& Wind=S47Wind::Wind();
 return FString::Printf(TEXT("CONTROL ROOM: LISTEN / %.0f OF 60 SECONDS. %s Window ends %s."),R.Listened,
  ComparableWind(R.WindReference,R.WindDirection,Wind.Strength,Wind.Dir)?TEXT("Wind matches the preparation reference. Stay inside."):TEXT("Wait for wind matching the reference ribbon; listening is paused."),*FS47FreeformCases::Clock(R.End));
}
bool FS47CaseFieldTests::Near(const FVector& At,double Range)const
{
 return M&&M->Mode==US47Main::EMode::Play&&!M->bPausedByMenu&&M->Player&&M->Web&&M->World&&!M->World->bDriving&&M->Web->Area()==TEXT("saro")
  &&FVector2D(M->Player->Pos.X-At.X,M->Player->Pos.Z-At.Z).Size()<=Range&&FMath::Abs(M->Player->FloorY-At.Y)<2.;
}
void FS47CaseFieldTests::Changed(const FString& Message)
{if(M){if(!Message.IsEmpty()&&M->UI)M->UI->Toast(Message,6);M->RequestAutosave();}Present();}
bool FS47CaseFieldTests::Prepare(int32 Test)
{
 if(!bActive||Test<0||Test>1||Test!=PlannedTest()||!Near(Test==0?LadderAt:GateAt,2.6))return false;
 SyncPlan();if(Prepared())return false;
 if(Test==1)M->Web->SetDoorOpen(TEXT("gate"),false,true); // sealing includes closing the existing physical leaf
 R.Prepared=M->Freeform->Clock();PeakAfter(R.Prepared,R.Start,R.End);R.LastClock=R.Prepared;
 const auto& Wind=S47Wind::Wind();R.WindReference=Wind.Strength;R.WindDirection=Wind.Dir;
 R.LastPlayer=FVector2D(M->Player->Pos.X,M->Player->Pos.Z);R.bHavePlayer=true;
 if(M->Audio)M->Audio->Play(Test==0?TEXT("rack_switch"):TEXT("small_switch"),.6f,S47Three::ToUE(Test==0?LadderAt:GateAt));
 Changed(Objective());return true;
}
bool FS47CaseFieldTests::InspectGate()
{
 if(!Prepared()||R.Test!=1||M->Freeform->Clock()<R.End||!Near(GateAt,2.6))return false;
 const bool Valid=R.Coverage>=R.End-R.Start-.01&&R.Gaps<.001&&!R.bGateTampered;
 const FString Evidence=!Valid?TEXT("The sealed-gate control lacks a complete observed world window, or the operator crossed the strip. No negative result is established; repeat the control."):
  R.bGateCrossed?TEXT("After a fully sampled control window: numbered seal torn at the latch and a series of dark crossing marks disturbs the strips on both sides of the gate. The physical disturbance occurred during the active window."):
  TEXT("After a fully sampled control window: numbered seal intact and both strips undisturbed. This excludes a crossing through this controlled gate in this window only.");
 const bool Result=M->Freeform->ReportFieldTest(Line,1,Valid,R.bGateCrossed,Evidence);if(Result)Changed();return Result;
}
bool FS47CaseFieldTests::SceneIntact() const
{
 if(!IsValid(M)||!IsValid(M->Web))return false;
 for(auto O:Objects)
 {
  if(O.W!=M->Web.Get())return false;const auto* N=M->Web->Node(O.I);const auto* C=Components.Find(O.I);
  if(!N||!C||!C->IsValid()||N->Comp.Get()!=C->Get())return false;
 }
 return !Objects.IsEmpty();
}
void FS47CaseFieldTests::ForgetScene()
{
 // Rebuilds can reuse a node index. Never hide or rotate through an old handle.
 bBuilt=false;Objects.Reset();LadderParts.Reset();Components.Reset();
 Ladder=Plate=Seal=SealBroken=Strip=StripBack=Ribbon=FS47Obj();for(auto& O:Tracks)O=FS47Obj();
}
bool FS47CaseFieldTests::Build()
{
 if(bBuilt){if(SceneIntact())return true;ForgetScene();}
 if(!M||!M->Web||!M->Web->IsLoaded()||M->Web->Area()!=TEXT("saro")||!M->Inter||!M->Freeform||!M->Freeform->Field)return false;
 const int32 Parent=M->Web->Handle(TEXT("ext.group"));if(Parent<0)return false;
 auto Box=[&](const TCHAR* Name,const FVector& At,const FVector& Size,uint32 Colour)
 {FS47Obj O=M->Freeform->Field->StageBox(Name,At,Size,Colour,Parent);if(O){Objects.Add(O);if(const auto* N=M->Web->Node(O.I))Components.Add(O.I,N->Comp);}return O;};
 Ladder=Box(TEXT("CaseLadderL"),LadderAt+FVector(-.22,1.6,0),FVector(.05,3.3,.05),0x8a8d86);
 LadderParts.Add(Box(TEXT("CaseLadderR"),LadderAt+FVector(.22,1.6,0),FVector(.05,3.3,.05),0x8a8d86));
 for(int32 I=0;I<10;++I)LadderParts.Add(Box(*FString::Printf(TEXT("CaseLadderRung%d"),I),LadderAt+FVector(0,.25+I*.32,0),FVector(.44,.03,.04),0x8a8d86));
 Plate=Box(TEXT("CaseRoofPlate"),FVector(4.7,3.62,-4.6),FVector(.9,.012,.6),0x9a9c97);
 Ribbon=Box(TEXT("CaseWindReferenceRibbon"),LadderAt+FVector(.3,2.4,0),FVector(.03,.05,.45),0xd9b950);
 Seal=Box(TEXT("CaseGateSeal"),GateAt+FVector(0,1.05,.02),FVector(1.4,.07,.02),0xe0c23a);
 SealBroken=Box(TEXT("CaseGateSealBroken"),GateAt+FVector(.45,.55,0),FVector(.4,.05,.015),0xd9c23a);
 // Measure the visible native walkway, not just the authored terrain floor.
 // Ignore this control's own geometry so repeated surface samples cannot stack it upward.
 auto Surface=[&](double X,double Z)
 {
  double Y=M->Player&&M->Player->Floor?M->Player->Floor(X,Z):-.6;if(!FMath::IsFinite(Y))Y=-.6;
  FCollisionQueryParams Query(SCENE_QUERY_STAT(S47CaseGateSurface),true,M->Player);
  for(const auto& O:Objects)if(const auto* N=M->Web->Node(O.I))if(auto* P=Cast<UPrimitiveComponent>(N->Comp.Get()))Query.AddIgnoredComponent(P);
  FHitResult Hit;
  if(M->Web->GetWorld()&&M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(X,Y+1.,Z),S47Three::ToUE(X,Y-1.,Z),ECC_Visibility,Query))Y=FMath::Max(Y,Hit.ImpactPoint.Z/100.);
  return Y;
 };
 double StripTop[2];
 for(int32 Side=0;Side<2;++Side)
 {
  const double Z=GateAt.Z+(Side==0?.65:-.65);double Y=Surface(GateAt.X,Z);
  for(double XOffset:{-.7,.7})for(double ZOffset:{-.28,.28})Y=FMath::Max(Y,Surface(GateAt.X+XOffset,Z+ZOffset));
  auto O=Box(Side==0?TEXT("CaseGateStrip"):TEXT("CaseGateStripBack"),FVector(GateAt.X,Y+.008,Z),FVector(1.6,.012,.6),0xc9a77a);
  if(Side==0)Strip=O;else StripBack=O;StripTop[Side]=Y+.014;
  UE_LOG(LogTemp,Display,TEXT("S47 gate strip side=%d z=%.3f native surface=%.3f"),Side,Z,Y);
 }
 for(int32 I=0;I<6;++I)
 {
  const int32 Side=I<3?0:1;const double X=GateAt.X+((I%2)? .16:-.16),Z=GateAt.Z+(Side==0?1:-1)*(.4+(I%3)*.23);
  const double Y=FMath::Max(Surface(X,Z),StripTop[Side]);
  Tracks[I]=Box(*FString::Printf(TEXT("CaseGateTrack%d"),I),FVector(X,Y+.005,Z),FVector(.11,.006,.16),0x57452f);
 }
 const TWeakPtr<uint8> Guard=Life;
 auto Add=[&](const TCHAR* Id,FS47Obj O,int32 Test)
 {
  FS47Interactable I;I.Id=Id;I.Object=O;I.Range=2.6;
  I.Label=[this,Guard,Test]{if(!Guard.IsValid()||!bActive||PlannedTest()!=Test)return FString();if(!Prepared())return FString(Test==0?TEXT("Fasten the roof plate from the ladder / retain wind reference"):TEXT("Seal the gate and lay tracking strips"));return FString(Test==1&&M->Freeform->Clock()>=R.End?TEXT("Inspect numbered gate seal and tracking strips"):TEXT("Inspect prepared field control / observation window"));};
  I.Use=[this,Guard,Test]{if(!Guard.IsValid()||!bActive)return;if(!Prepared())Prepare(Test);else if(Test==1&&M->Freeform->Clock()>=R.End)InspectGate();else if(Near(Test==0?LadderAt:GateAt,2.6))Changed(Objective());};M->Inter->Add(I);
 };
 Add(TEXT("case:roof-plate"),Ladder,0);Add(TEXT("case:gate-seal"),Strip,1);
 for(auto O:LadderParts)if(O)M->Web->MarkUsable(O.I,TEXT("case:roof-plate"));
 if(StripBack)M->Web->MarkUsable(StripBack.I,TEXT("case:gate-seal"));
 if(Seal)M->Web->MarkUsable(Seal.I,TEXT("case:gate-seal"));if(SealBroken)M->Web->MarkUsable(SealBroken.I,TEXT("case:gate-seal"));
 bBuilt=Ladder&&Plate&&Strip&&StripBack&&Seal;Present();return bBuilt;
}
void FS47CaseFieldTests::Present()
{
 if(!bBuilt||IsEngineExitRequested()||IsGarbageCollecting()||!IsValid(M)||!IsValid(M->Web))return;
 if(!SceneIntact()){ForgetScene();return;}
 auto Show=[&](FS47Obj O,bool Yes)
 {if(!O)return;O.SetVisible(Yes);if(const auto* N=O.W->Node(O.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))if(IsValid(C))C->SetCollisionEnabled(Yes?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);};
 const bool Here=bActive&&M->IsFreeform()&&M->Web->Area()==TEXT("saro");
 const bool Roof=Here&&PlannedTest()==0,Gate=Here&&PlannedTest()==1;
 Show(Ladder,Roof);Show(Plate,Roof);Show(Ribbon,Roof);for(auto O:LadderParts)Show(O,Roof);
 if(Roof&&Plate)Plate.SetRotZ(Prepared()?0:.055);
 if(Roof&&Ribbon){const auto& W=S47Wind::Wind();Ribbon.SetRotY(FMath::Atan2(W.Dir.X,W.Dir.Y));Ribbon.SetRotX(-.5+W.Strength*6+FMath::Sin(W.Time*3)*W.Strength);}
 Show(Strip,Gate);Show(StripBack,Gate);Show(Seal,Gate&&Prepared()&&!R.bGateCrossed&&!R.bGateTampered);Show(SealBroken,Gate&&Prepared()&&(R.bGateCrossed||R.bGateTampered));
 for(auto O:Tracks)Show(O,Gate&&Prepared()&&R.bGateCrossed);
}
void FS47CaseFieldTests::Stop(){bActive=false;Present();}
void FS47CaseFieldTests::AdvanceUnobservedTo(double Clock)
{
 if(!Prepared()||!FMath::IsFinite(Clock)||Clock<=R.LastClock)return;
 R.Gaps+=Overlap(R.LastClock,Clock,R.Start,R.End);R.LastClock=Clock;R.bHavePlayer=false;
}
void FS47CaseFieldTests::Step(double Dt)
{
 if(!bActive||!M||!M->Freeform||!M->Freeform->Active()||!FMath::IsFinite(Dt)||Dt<=0)return;
 SyncPlan();Build();Present();if(!Prepared())return;
 const double Now=M->Freeform->Clock(),Delta=Now-R.LastClock,From=R.LastClock;
 if(Delta<0){R.LastClock=Now;R.bHavePlayer=false;return;}
 if(Delta>3.||M->Freeform->Held()){AdvanceUnobservedTo(Now);return;}
 if(Delta<=0)return;R.LastClock=Now;
 const double Covered=Overlap(From,Now,R.Start,R.End);
 if(R.Test==1)
 {
  R.Coverage+=Covered;
  if(Covered>0&&M->Web&&M->Web->IsDoorOpen(TEXT("fieldGate")))R.bGateTampered=true;
  const double EventAt=R.Start+(R.End-R.Start)*.5;
  if(Covered>0&&From<EventAt&&Now>=EventAt&&M->Freeform->FieldTestExpectsPresent(Line,1))
  {R.bGateCrossed=true;if(M->Audio&&Near(GateAt,15))M->Audio->Play(TEXT("small_switch"),.4f,S47Three::ToUE(GateAt));Changed();}
  if(M->Player&&M->Web&&M->Web->Area()==TEXT("saro"))
  {
   const FVector2D P(M->Player->Pos.X,M->Player->Pos.Z);
   if(Covered>0&&R.bHavePlayer&&FMath::Abs(P.X-GateAt.X)<1.0&&FMath::Abs(R.LastPlayer.X-GateAt.X)<1.0
    &&(P.Y-GateAt.Z)*(R.LastPlayer.Y-GateAt.Z)<0)R.bGateTampered=true;
   R.LastPlayer=P;R.bHavePlayer=true;
  }else R.bHavePlayer=false;
  return;
 }
 if(Now>=R.End)
 {
  M->Freeform->ReportFieldTest(Line,0,false,false,TEXT("The listening window ended before a full minute with comparable wind and all audible events was observed. Missed time, silence outside the window and a sleeping operator prove nothing."));Changed();return;
 }
 const bool Inside=M->Player&&M->World&&!M->World->bDriving&&M->Web&&M->Web->Area()==TEXT("saro")&&M->Web->Rect(TEXT("room.bounds")).Contains(M->Player->Pos);
 const auto& Wind=S47Wind::Wind();
 if(Covered<=0||!Inside||!M->Audio||!M->Audio->Ctx||!ComparableWind(R.WindReference,R.WindDirection,Wind.Strength,Wind.Dir))return;
 R.Listened=FMath::Min(ListenNeed,R.Listened+FMath::Min(Dt,.1)*Covered/Delta);
 const bool Impacts=M->Freeform->FieldTestExpectsPresent(Line,0);
 if(Impacts&&R.Impacts<4&&R.Listened>=(R.Impacts+1)*15.)
 {
  FS47Sound Sound;Sound.At=RoofAnchor+FVector((R.Impacts-1.5)*.6,0,0);Sound.Gain=.5f;Sound.Lp=1800;
  if(M->Audio->Sfx(TEXT("stepConcrete0"),Sound).IsValid()){++R.Impacts;Changed();}
 }
 if(R.Listened>=ListenNeed&&(!Impacts||R.Impacts==4))
 {
  const FString Evidence=Impacts?TEXT("Roof plate physically fastened; a minute observed from the control room in wind at least comparable to the retained foliage-sway reference and within 45 degrees of its direction. Four spatial impacts were heard after fastening."):
   TEXT("Roof plate physically fastened; a minute observed from the control room in wind at least comparable to the retained foliage-sway reference and within 45 degrees of its direction. No impacts occurred under this controlled condition. This is not a wind-speed measurement.");
  M->Freeform->ReportFieldTest(Line,0,true,Impacts,Evidence);Changed();
 }
}
S47J::FObj FS47CaseFieldTests::Save()const
{
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("job"),R.Job);S47J::Set(O,TEXT("test"),R.Test);
 S47J::Set(O,TEXT("prepared"),R.Prepared);S47J::Set(O,TEXT("start"),R.Start);S47J::Set(O,TEXT("end"),R.End);S47J::Set(O,TEXT("lastClock"),R.LastClock);
 S47J::Set(O,TEXT("listened"),R.Listened);S47J::Set(O,TEXT("coverage"),R.Coverage);S47J::Set(O,TEXT("gaps"),R.Gaps);S47J::Set(O,TEXT("impacts"),R.Impacts);
 S47J::Set(O,TEXT("wind"),R.WindReference);S47J::Set(O,TEXT("windDir"),TArray<double>{R.WindDirection.X,R.WindDirection.Y});
 S47J::Set(O,TEXT("gateCrossed"),R.bGateCrossed);S47J::Set(O,TEXT("gateTampered"),R.bGateTampered);return O;
}
void FS47CaseFieldTests::Load(const S47J::FObj& O)
{
 Stop();R=FRun();bActive=true;if(!O||S47J::Int(O,TEXT("v"))!=1)return;
 auto N=[&](const TCHAR* Key,double Min,double Max,double Default=0.){const double V=S47J::Num(O,Key,Default);return FMath::IsFinite(V)?FMath::Clamp(V,Min,Max):Default;};
 R.Job=S47J::Str(O,TEXT("job")).Left(128);R.Test=int32(N(TEXT("test"),-1,1,-1));R.Prepared=N(TEXT("prepared"),-1,3153600000.,-1);
 if(R.Job.IsEmpty()||R.Test<0||R.Prepared<0){R=FRun();return;}
 PeakAfter(R.Prepared,R.Start,R.End);R.LastClock=N(TEXT("lastClock"),R.Prepared,3153600000.,R.Prepared);
 R.Listened=N(TEXT("listened"),0,ListenNeed);R.Coverage=N(TEXT("coverage"),0,R.End-R.Start);R.Gaps=N(TEXT("gaps"),0,R.End-R.Start);R.Impacts=int32(N(TEXT("impacts"),0,4));
 R.WindReference=N(TEXT("wind"),0,.2);const auto D=S47J::Nums(O,TEXT("windDir"));if(D.Num()==2&&FMath::IsFinite(D[0])&&FMath::IsFinite(D[1]))R.WindDirection=FVector2D(D[0],D[1]).GetSafeNormal();
 R.bGateCrossed=S47J::Bool(O,TEXT("gateCrossed"));R.bGateTampered=S47J::Bool(O,TEXT("gateTampered"));
 // First restored frame cannot infer whether a player crossed a gate while loading.
 R.bHavePlayer=false;
}
bool FS47CaseFieldTests::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;auto C=[&](bool Good,const TCHAR* Text){OK&=Good;Lines.Add(FString(Good?TEXT("PASS "):TEXT("FAIL "))+TEXT("case field: ")+Text);};
 double Start=0,End=0;PeakAfter(86400.+3600.,Start,End);C(Start==2*86400.+5400&&End-Start==10800,TEXT("late preparation explicitly moves control to the following night"));
 PeakAfter(22*3600.,Start,End);C(Start==86400.+5400,TEXT("evening preparation keeps the next after-midnight window"));
 C(Overlap(Start-300,Start-1,Start,End)==0&&Overlap(Start-300,End+100,Start,End)==10800,TEXT("pre-window waiting has no gap; sleeping through entire window has full gap"));
 C(ComparableWind(.06,FVector2D(1,0),.05,FVector2D(1,0))&&!ComparableWind(.06,FVector2D(1,0),.02,FVector2D(1,0))&&!ComparableWind(.06,FVector2D(1,0),.07,FVector2D(-1,0)),TEXT("weak or contrary wind cannot establish a negative roof control"));
 FS47CaseFieldTests A(nullptr);A.R.Job=TEXT("field-case3");A.R.Test=0;A.R.Prepared=22*3600.;PeakAfter(A.R.Prepared,A.R.Start,A.R.End);A.R.LastClock=A.R.Start+100;A.R.Listened=23.5;A.R.Impacts=1;A.R.WindReference=.065;A.R.WindDirection=FVector2D(.8,.6);A.R.Gaps=30;
 FS47CaseFieldTests B(nullptr);B.Load(S47J::Read(S47J::Write(A.Save())));
 C(B.R.Job==A.R.Job&&B.R.Test==0&&B.R.Prepared==A.R.Prepared&&B.R.Start==A.R.Start&&B.R.Listened==23.5&&B.R.Impacts==1&&B.R.Gaps==30&&B.R.WindDirection.Equals(A.R.WindDirection,.0001),TEXT("JSON Continue retains physical preparation, wind reference, listening, emitted impacts and gaps"));
 B.ResetPlan();C(B.R.Prepared<0&&B.R.Listened==0,TEXT("new or cancelled plan cannot borrow an earlier control"));return OK;
}

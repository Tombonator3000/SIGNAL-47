#include "S47SaroWork.h"
#include "S47Main.h"
#include "S47Web.h"
#include "S47Story.h"
#include "S47Audio.h"
#include "S47UI.h"
#include "S47Inter.h"
#include "S47Walker.h"
#include "S47Three.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

namespace
{
 const TCHAR* ServiceId = TEXT("obs:service");
 const FVector PanelLocal(0, .62, .29);
 const FVector2D Cross[] = { {0,.5}, {0,-.5}, {.5,0}, {-.5,0}, {0,0} };
 constexpr double Tolerance = .1, SettlingSeconds = 1., SampleSeconds = 2.;
 bool Close(double Az, double El, double ToAz, double ToEl)
 {
  return FMath::Abs(FMath::FindDeltaAngleDegrees(Az, ToAz)) <= Tolerance && FMath::Abs(El-ToEl) <= Tolerance;
 }
}

FS47SaroWork::~FS47SaroWork() { Stop(); }

void FS47SaroWork::Begin(const S47J::FObj& Saved, bool bEnable)
{
 Stop();
 bEnabled = bEnable; ContextRun = 0; CommandRun = -1; CrossIndex = 0;
 Service = EService::NotDue; Motion = EMotion::None; bCross = false;
 TargetAz = TargetEl = Settle = Sample = ServiceAz = ServiceEl = 0;
 SavedAngles.Reset(); Commands.Reset(); bRestore = false;
 if (Saved && S47J::Int(Saved,TEXT("v"))!=1) { bEnabled=false; DrawPanel(); return; }
 if (Saved && S47J::Int(Saved, TEXT("v")) == 1 && bEnable)
 {
  Service = EService(FMath::Clamp(S47J::Int(Saved,TEXT("service")),0,int32(EService::Complete)));
  Motion = EMotion(FMath::Clamp(S47J::Int(Saved,TEXT("motion")),0,int32(EMotion::Complete)));
  CommandRun = FMath::Clamp(S47J::Int(Saved,TEXT("run"),-1),-1,3);
  TargetAz = S47J::Num(Saved,TEXT("az")); TargetEl = S47J::Num(Saved,TEXT("el"));
  Settle = FMath::Clamp(S47J::Num(Saved,TEXT("settle")),0.,SettlingSeconds);
  Sample = FMath::Clamp(S47J::Num(Saved,TEXT("sample")),0.,SampleSeconds);
  CrossIndex = FMath::Clamp(S47J::Int(Saved,TEXT("crossPoint")),0,4);
  bCross = S47J::Bool(Saved,TEXT("cross"));
  ServiceAz = S47J::Num(Saved,TEXT("serviceAz")); ServiceEl = S47J::Num(Saved,TEXT("serviceEl"));
  SavedAngles = S47J::Nums(Saved,TEXT("angles")); Commands = S47J::Strs(Saved,TEXT("commands"));
  bRestore = true;
 }
 DrawPanel();
}

void FS47SaroWork::Stop()
{
 if (M && M->Web)
 {
  M->Web->SetWorkMotionPaused(false);
  const int32 I = S03(); if (I >= 0) M->Web->SetDishRemoteAvailable(I,true);
 }
 Sound(false); bEnabled = false;
 DrawPanel();
}

S47J::FObj FS47SaroWork::Save() const
{
 auto O = S47J::New(); S47J::Set(O,TEXT("v"),1);
 S47J::Set(O,TEXT("service"),int32(Service)); S47J::Set(O,TEXT("motion"),int32(Motion));
 S47J::Set(O,TEXT("run"),CommandRun); S47J::Set(O,TEXT("az"),TargetAz); S47J::Set(O,TEXT("el"),TargetEl);
 S47J::Set(O,TEXT("settle"),Settle); S47J::Set(O,TEXT("sample"),Sample);
 S47J::Set(O,TEXT("crossPoint"),CrossIndex); S47J::Set(O,TEXT("cross"),bCross);
 S47J::Set(O,TEXT("serviceAz"),ServiceAz); S47J::Set(O,TEXT("serviceEl"),ServiceEl);
 TArray<double> Angles = SavedAngles;
 if (!bRestore && M && M->Web && M->Web->IsLoaded())
 {
  Angles.Reset();
  for (int32 I=0; I<M->Web->DishCount(); ++I)
  {
   Angles.Append({ M->Web->DishAz(I), M->Web->DishEl(I), M->Web->DishTargetAz(I), M->Web->DishTargetEl(I) });
  }
 }
 S47J::Set(O,TEXT("angles"),Angles); S47J::Set(O,TEXT("commands"),Commands);
 return O;
}

int32 FS47SaroWork::S03() const
{
 if (M && M->Web) for (int32 I=0;I<M->Web->DishCount();++I) if (M->Web->DishId(I)==TEXT("S-03")) return I;
 return INDEX_NONE;
}

void FS47SaroWork::ApplyRestore()
{
 if (!bRestore || !M || !M->Web || !M->Web->IsLoaded()) return;
 AS47Web* W=M->Web; bRestore=false;
 if (SavedAngles.Num()==W->DishCount()*4)
 {
  for (int32 I=0;I<W->DishCount();++I)
  {
   W->SnapDish(I,SavedAngles[I*4],SavedAngles[I*4+1]);
   W->PointDish(I,SavedAngles[I*4+2],SavedAngles[I*4+3],9,true);
  }
 }
 const int32 I=S03();
 if (I>=0 && ServiceRequired())
 {
  W->SetDishRemoteAvailable(I,false);
  if (Service==EService::TestingOut) W->PointDish(I,ServiceAz+5,ServiceEl,2.2,true);
  if (Service==EService::TestingBack) W->PointDish(I,ServiceAz,ServiceEl,2.2,true);
 }
 DrawPanel();
}

void FS47SaroWork::SetContext(int32 Run, bool bServiceDue)
{
 if (!bEnabled) return;
 ContextRun=Run;
 if (bServiceDue && Run==1 && Service==EService::NotDue)
 {
  Service=EService::Fault; Motion=EMotion::None; CommandRun=-1; Settle=Sample=0;
  if (M && M->Web && S03()>=0) M->Web->SetDishRemoteAvailable(S03(),false);
  Changed(TEXT("S-03 REMOTE LINK / HEALTH POLL FAILED. Read its status, then check the local cabinet in the east service yard."));
 }
 if (CommandRun>=0 && Run!=CommandRun && !ServiceRequired()) { Motion=EMotion::None; CommandRun=-1; Settle=Sample=0; }
 DrawPanel();
}

bool FS47SaroWork::CommandObservation(int32 Run, double Az, double El, bool bIsCross)
{
 if (!bEnabled || Run!=ContextRun || !M || !M->Web || !M->Story || M->Story->FeederOutage() || ServiceRequired()) return false;
 if (!FMath::IsFinite(Az) || !FMath::IsFinite(El)) return false;
 ApplyRestore();
 TargetAz=Az; TargetEl=El; CommandRun=Run; bCross=bIsCross; CrossIndex=0; Settle=Sample=0;
 Motion=EMotion::Slewing;
 M->Web->PointDishes(Az,El,9);
 Commands.Add(FString::Printf(TEXT("%s  OBS %d  CMD AZ %.1f EL %.1f"),*M->Story->ClockText(true),Run+1,Az,El));
 while (Commands.Num()>16) Commands.RemoveAt(0);
 Changed(TEXT("Array command sent. Watch the dishes turn; acquisition waits for the actual encoder readings."));
 return true;
}

bool FS47SaroWork::OnTarget(bool bOnlyS03) const
{
 if (!M || !M->Web || M->Web->DishCount()!=27) return false;
 AS47Web* W=M->Web;
 if (bOnlyS03)
 {
  const int32 I=S03();
  return I>=0 && Close(W->DishAz(I),W->DishEl(I),ServiceAz+(Service==EService::TestingOut?5.:0.),ServiceEl);
 }
 const FVector2D Offset=Motion==EMotion::Scanning?Cross[CrossIndex]:FVector2D::ZeroVector;
 for (int32 I=0;I<W->DishCount();++I)
 {
  if (!W->DishRemoteAvailable(I) || !Close(W->DishAz(I),W->DishEl(I),TargetAz+Offset.X,TargetEl+Offset.Y)) return false;
 }
 return true;
}

bool FS47SaroWork::CanMeasure() const
{
 if (!bEnabled || ServiceRequired() || (M && M->Story && M->Story->FeederOutage())) return false;
 return (Motion==EMotion::Ready || Motion==EMotion::Complete || (Motion==EMotion::Scanning && Settle>=SettlingSeconds)) && OnTarget();
}

void FS47SaroWork::BeginScan()
{
 if (!Ready() || !OnTarget() || ServiceRequired()) return;
 if (!bCross) { Motion=EMotion::Complete; Changed(); return; }
 Motion=EMotion::Scanning; CrossIndex=0; PointCross(); Changed();
}

void FS47SaroWork::PointCross()
{
 Settle=Sample=0;
 if (M && M->Web) M->Web->PointDishes(TargetAz+Cross[CrossIndex].X,TargetEl+Cross[CrossIndex].Y,2.2);
}

void FS47SaroWork::Step(double Dt, bool bPause)
{
 if (!bEnabled || !M || !M->Web) return;
 BuildPanel(); ApplyRestore();
 const bool bHold=bPause || !FMath::IsFinite(Dt) || Dt<=0 || (M->Story && M->Story->FeederOutage());
 M->Web->SetWorkMotionPaused(bHold);
 if (bHold) { Sound(false); return; }
 Dt=FMath::Min(Dt,.1);
 if (Service==EService::TestingOut || Service==EService::TestingBack)
 {
  Settle=OnTarget(true) ? Settle+Dt : 0;
  if (Settle>=SettlingSeconds)
  {
   if (Service==EService::TestingOut)
   {
    Service=EService::TestingBack; Settle=0;
    M->Web->PointDish(S03(),ServiceAz,ServiceEl,2.2,true);
   }
   else { Service=EService::Tested; Settle=0; Changed(TEXT("S-03 local test passed: five degrees out and back. Close the local test by restoring REMOTE.")); }
  }
 }
 else if (Motion==EMotion::Slewing)
 {
  Settle=OnTarget() ? Settle+Dt : 0;
  if (Settle>=SettlingSeconds) { Settle=SettlingSeconds; Motion=EMotion::Ready; Changed(TEXT("27 / 27 ON TARGET. The receiver can begin this observation.")); }
 }
 else if (Motion==EMotion::Scanning)
 {
  if (!OnTarget()) Settle=0;
  else if (Settle<SettlingSeconds) Settle=FMath::Min(SettlingSeconds,Settle+Dt);
  else
  {
   Sample+=Dt;
   if (Sample>=SampleSeconds)
   {
    if (CrossIndex<4) { ++CrossIndex; PointCross(); }
    else { Motion=EMotion::Complete; Changed(); }
   }
  }
 }
 Sound(M->Web->DishesMoving()); DrawPanel();
}

void FS47SaroWork::Sound(bool bMoving)
{
 if (!M || !M->Audio) { bMotorSound=false; return; }
 if (bMoving && !bMotorSound) { M->Audio->StartMotors(); bMotorSound=true; }
 else if (!bMoving && bMotorSound) { M->Audio->StopMotors(); bMotorSound=false; }
}

FVector FS47SaroWork::ServicePoint() const
{
 if (M && M->Web)
 {
  const FS47Obj Cabinet=M->Web->Obj(TEXT("yard.objs.motorBus"));
  if (Cabinet) return Cabinet.LocalToWorld(PanelLocal+FVector(0,0,.06));
 }
 return FVector(11.95,.62,-11);
}

FString FS47SaroWork::Label(const FString& Id) const
{
 if (!bEnabled || Id!=ServiceId || !ServiceRequired()) return FString();
 switch(Service)
 {
  case EService::Fault: return TEXT("S-03: switch from REMOTE to LOCAL SERVICE");
  case EService::Local: return TEXT("Open the S-03 service panel");
  case EService::OpenLoose: return TEXT("Seat the loose drive-command connector");
  case EService::OpenSeated: return TEXT("Close the service panel over the seated connector");
  case EService::Closed: return TEXT("Run the local five-degree movement test");
  case EService::TestingOut: case EService::TestingBack: return TEXT("Local test running: watch S-03 turn and return");
  case EService::Tested: return TEXT("Restore S-03 REMOTE control");
  default: return FString();
 }
}

bool FS47SaroWork::Use(const FString& Id)
{
 if (Id!=ServiceId || !bEnabled) return false;
 if (!ServiceRequired()) return false;
 if (M && M->Player && FVector2D(M->Player->Pos.X-ServicePoint().X,M->Player->Pos.Z-ServicePoint().Z).Size()>2.7) return true;
 if (!M || !M->Web || S03()<0) return true;
 FString Message;
 switch(Service)
 {
  case EService::Fault: Service=EService::Local; Message=TEXT("LOCAL SERVICE selected. Remote movement is isolated. Open the service panel."); break;
  case EService::Local: Service=EService::OpenLoose; Message=TEXT("The drive-command plug is loose in its socket. Seat it before testing the drive."); break;
  case EService::OpenLoose: Service=EService::OpenSeated; Message=TEXT("Connector seated. Close the panel before running the local movement test."); break;
  case EService::OpenSeated: Service=EService::Closed; Message=TEXT("Panel secured. Run the local five-degree test and watch S-03."); break;
  case EService::Closed:
   ServiceAz=M->Web->DishAz(S03()); ServiceEl=M->Web->DishEl(S03()); Settle=0;
   Service=EService::TestingOut; M->Web->PointDish(S03(),ServiceAz+5,ServiceEl,2.2,true);
   Message=TEXT("LOCAL TEST: five degrees clockwise, then back. Observe the dish and its encoder."); break;
  case EService::Tested:
   Service=EService::Complete; M->Web->SetDishRemoteAvailable(S03(),true); Motion=EMotion::None; CommandRun=-1;
   Message=TEXT("S-03 REMOTE restored. Return to the tracking terminal and send the pulsar pointing command.");
   Commands.Add(TEXT("S-03 SERVICE / COMMAND CONNECTOR SEATED / LOCAL +5 DEG AND RETURN PASS / REMOTE RESTORED"));
   if (M->Story) M->Story->Note(M->Story->ClockText(true)+TEXT(". S-03 drive-command connector seated. Local movement and encoder agreement verified; remote control restored. This service record precedes the pulsar observation."));
   break;
  default: Message=TEXT("The local movement test is still running. Watch the dish complete its outward and return movements."); break;
 }
 if (M->Audio) M->Audio->Play(TEXT("rack_switch"),.45f,ServicePoint());
 DrawPanel(); Changed(Message); return true;
}

FString FS47SaroWork::Objective() const
{
 if (ServiceRequired()) return TEXT("S-03 SERVICE // ")+Label(ServiceId).ToUpper();
 if (Motion==EMotion::None) return TEXT("TRACKING TERMINAL // SELECT THE SCHEDULED SOURCE AND POINT THE ARRAY");
 if (Motion==EMotion::Slewing) return TEXT("ARRAY MOVING // WAIT FOR 27 ANTENNAS ON TARGET, THEN USE THE RECEIVER");
 if (Motion==EMotion::Ready) return TEXT("RECEIVER // ARRAY ON TARGET. SET THE RECEIVER AND START THE OBSERVATION");
 return FString();
}

FString FS47SaroWork::StateText() const
{
 if (ServiceRequired()) return Service==EService::Fault ? FString(TEXT("S-03 REMOTE LINK HEALTH POLL FAILED / LOCAL SERVICE REQUIRED")) : FString(TEXT("S-03 LOCAL SERVICE IN PROGRESS / REMOTE ISOLATED"));
 switch(Motion)
 {
  case EMotion::None:return TEXT("NO POINTING COMMAND");
  case EMotion::Slewing:return Settle>0 ? TEXT("ON TARGET / STABILISING") : TEXT("ARRAY SLEWING");
  case EMotion::Ready:return TEXT("27 / 27 ON TARGET / READY FOR ACQUISITION");
  case EMotion::Scanning:return FString::Printf(TEXT("POINTING CROSS %d / 5 / %s"),CrossIndex+1,Settle>=SettlingSeconds?TEXT("SAMPLING"):TEXT("MOVING / SETTLING"));
  default:return TEXT("OBSERVATION POINTING COMPLETE");
 }
}

TArray<FString> FS47SaroWork::StatusRows() const
{
 TArray<FString> Rows;
 if (!M || !M->Web) return Rows;
 AS47Web* W=M->Web;
 for(int32 I=0;I<W->DishCount();++I)
 {
  const FString State=!W->DishRemoteAvailable(I) ? FString(TEXT("LOCAL / UNAVAILABLE")) : W->DishMoving(I)?FString(TEXT("MOVING")):FString(TEXT("HOLD"));
  Rows.Add(FString::Printf(TEXT("%s  ACT %05.1f/%04.1f  CMD %05.1f/%04.1f  %s"),*W->DishId(I),W->DishAz(I),W->DishEl(I),W->DishTargetAz(I),W->DishTargetEl(I),*State));
 }
 for(const FString& Log:Commands) Rows.Add(Log);
 return Rows;
}

void FS47SaroWork::Changed(const FString& Text)
{
 if (!M) return;
 if (!Text.IsEmpty() && M->UI) M->UI->Toast(Text,5);
 M->RequestAutosave();
}

void FS47SaroWork::BuildPanel()
{
 if (bBuilt || !M || !M->Web || !M->Inter || !M->Web->IsLoaded()) return;
 AS47Web* W=M->Web;
 const FS47Obj Cabinet=W->Obj(TEXT("yard.objs.motorBus"));
 if (!Cabinet) return;
 const auto* N=W->Node(Cabinet.I);
 auto* Parent=N?N->Comp.Get():nullptr;
 auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 if (!Parent || !Cube || !Base) return;
 auto Part=[&](const TCHAR* Name,const FVector& At,const FVector& Scale,uint32 Colour)
 {
  auto* Mat=UMaterialInstanceDynamic::Create(Base,W);
  Mat->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(Colour));
  Mat->SetScalarParameterValue(TEXT("Roughness"),.72f);
  Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
  Mat->SetScalarParameterValue(TEXT("EmissivePower"),0);
  auto* C=NewObject<UStaticMeshComponent>(W,Name);
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(Parent); C->SetStaticMesh(Cube); C->SetMaterial(0,Mat);
  C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore);
  C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block); C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
  C->RegisterComponent(); C->SetWorldLocation(S47Three::ToUE(Cabinet.LocalToWorld(At)));
  FS47Obj O=W->ObjAt(W->AdoptNode(C,Cabinet.I,Name)); O.SetPosition(At); O.SetScale(Scale);
  return O;
 };
 // The original cabinet and inspection screen stay in place. A service hatch is fitted below it.
 Panel=Part(TEXT("ObsS03ServicePanel"),PanelLocal,FVector(.50,.34,.025),0x252c28);
 Lid=Part(TEXT("ObsS03ServiceLid"),PanelLocal+FVector(0,0,.045),FVector(.50,.34,.025),0x596158);
 Connector=Part(TEXT("ObsS03CommandConnector"),PanelLocal+FVector(.07,0,.045),FVector(.14,.07,.06),0xc5a04e);
 Selector=Part(TEXT("ObsS03LocalSelector"),PanelLocal+FVector(-.17,-.1,.075),FVector(.055,.1,.035),0xab4031);
 bBuilt=true;
 const TWeakPtr<uint8> Guard=Life;
 FS47Interactable It; It.Id=ServiceId; It.Object=Panel; It.Range=2.4;
 It.Label=[this,Guard] {return Guard.IsValid()?Label(ServiceId):FString();};
 It.Use=[this,Guard] {if(Guard.IsValid())Use(ServiceId);};
 It.Examine=[this,Guard] {return Guard.IsValid()?FString(TEXT("S-03 local service hatch beneath the original motor-controller screen. Red LOCAL selector, hinged cover and a keyed drive-command connector. Open only after selecting local service.")):FString();};
 M->Inter->Add(It);
 for(const FS47Obj& O:{Lid,Connector,Selector}) W->MarkUsable(O.I,ServiceId);
 DrawPanel();
}

void FS47SaroWork::DrawPanel()
{
 if (!bBuilt) return;
 const bool bOpen=bEnabled && (Service==EService::OpenLoose || Service==EService::OpenSeated);
 // A completed repair leaves its fitted hatch in place after Observing releases ownership.
 const bool bVisible=bEnabled || Service==EService::Complete;
 for(FS47Obj O:{Panel,Lid,Connector,Selector}) if(O)
 {
  O.SetVisible(bVisible);
  if (const auto* N=O.W->Node(O.I)) if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))
   C->SetCollisionEnabled(bEnabled && ServiceRequired()?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
 }
 if (!bVisible) return;
 // Sliding/folding cover exposes the plug; its loose state protrudes from the keyed socket.
 Lid.SetPosition(PanelLocal+(bOpen?FVector(-.31,0,.10):FVector(0,0,.045)));
 Lid.SetRotY(bOpen?-1.35:0);
 Connector.SetPosition(PanelLocal+FVector(.07,0,Service==EService::OpenLoose?.11:.035));
 Connector.SetRotZ(Service==EService::OpenLoose?.24:0);
 Selector.SetRotZ(Service==EService::NotDue || Service==EService::Fault || Service==EService::Complete?0:UE_DOUBLE_PI*.5);
}

bool FS47SaroWork::SelfTest(TArray<FString>& Lines)
{
 bool bOk=true;
 auto Check=[&](bool b,const TCHAR* Text){Lines.Add(FString::Printf(TEXT("%s saro work: %s"),b?TEXT("PASS"):TEXT("FAIL"),Text));bOk&=b;};
 Check(Close(359.98,32,.02,32.04) && !Close(26,32,26.2,32) && !Close(26,32,26,32.2),TEXT("both physical axes and wrapped azimuth determine target tolerance"));
 FS47SaroWork Disabled(nullptr); Disabled.Begin({},false); Disabled.SetContext(1,true);
 Check(!Disabled.Enabled() && !Disabled.ServiceRequired() && Disabled.Label(ServiceId).IsEmpty(),TEXT("legacy disabled helper creates no service gate"));
 bool Retained=true;
 for(int32 I=0;I<=int32(EService::Complete);++I)
 {
  auto O=S47J::New();S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("service"),I);
  S47J::Set(O,TEXT("motion"),int32(EMotion::Scanning));S47J::Set(O,TEXT("run"),0);
  S47J::Set(O,TEXT("crossPoint"),3);S47J::Set(O,TEXT("settle"),.45);S47J::Set(O,TEXT("sample"),1.2);
  S47J::Set(O,TEXT("angles"),TArray<double>{70,25,70.5,25});
  FS47SaroWork A(nullptr); A.Begin(O,true);
  FS47SaroWork B(nullptr); B.Begin(S47J::Read(S47J::Write(A.Save())),true);
  Retained &= int32(B.ServiceStage())==I && B.CrossPoint()==3 && B.CommandedRun()==0 &&
   FMath::IsNearlyEqual(S47J::Num(B.Save(),TEXT("settle")),.45) && S47J::Nums(B.Save(),TEXT("angles")).Num()==4;
 }
 Check(Retained,TEXT("every service state, unfinished cross dwell and physical angle snapshot survive serialization"));
 FS47SaroWork Fresh(nullptr);Fresh.Begin({},true);Fresh.SetContext(1,true);
 Check(Fresh.ServiceStage()==EService::Fault && !Fresh.Ready() && !Fresh.CanMeasure() && !Fresh.ScanComplete(),TEXT("service fault blocks acquisition rather than completing on a timer"));
 auto Future=S47J::New();S47J::Set(Future,TEXT("v"),2);
 FS47SaroWork Unknown(nullptr);Unknown.Begin(Future,true);
 Check(!Unknown.Enabled(),TEXT("unknown work versions do not invent new tasks"));
 Fresh.Stop();Check(!Fresh.Enabled() && Fresh.Label(ServiceId).IsEmpty(),TEXT("story takeover disables the physical service interaction"));
 return bOk;
}

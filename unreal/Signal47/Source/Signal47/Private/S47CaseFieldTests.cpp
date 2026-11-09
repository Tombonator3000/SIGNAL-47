#include "S47CaseFieldTests.h"
#include "S47Main.h"
#include "S47Night.h"
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
#include "S47SaroClip.h"
#include "Components/PrimitiveComponent.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "UObject/GarbageCollection.h"

namespace
{
 // T16-11 (Tom: "there is no ladder to climb up on the roof with"): a fixed steel ladder on the control room's north wall, there at
 // SARO all along in Freeform, not only once the roof control is planned (before, it was hidden until then). It stands on the ground
 // measured under its feet (the old one began at the floor slab's height, 0, and hung in the air wherever the yard is lower; the walk's
 // fallback for the yard is -0.6; Build logs what it measured), its rails go a metre past the
 // roof's edge, its rung line is 18 cm off the wall's face (the north slab's outer face, z -4.8 in S47SaroClip's table) on four
 // standoffs that end on that face, and it is east of the north window band (x -5.4..5.4) and short of the corner (6.3): the old one
 // stood in front of the window. The plate lies on the roof's edge by its top, where the hands reach from the ladder.
 constexpr double WallFace=-4.8,WindowEnd=5.4,CornerX=6.3,RoofY=3.6;
 const FVector LadderAt(5.85,0,-4.98),PlateAt(5.5,3.62,-4.6),RoofAnchor(-1.5,3.5,-1),GateAt(-1.0,0,-20.0);
 constexpr uint32 LadderColour=0xa3a6a0; // galvanised steel, lighter than the wall so it reads at dusk
 struct FLadderPart{FString Name;FVector At,Size;};
 // The ladder's boxes (three metres, centres and sizes) for its feet on the ground at Ground.
 TArray<FLadderPart> LadderShape(double Ground)
 {
  TArray<FLadderPart> Out;
  const double Top=RoofY+1.0,Length=Top-Ground;
  for(const double Side:{-1.,1.})Out.Add({Side<0?TEXT("CaseLadderL"):TEXT("CaseLadderR"),FVector(LadderAt.X+Side*.22,Ground+Length*.5,LadderAt.Z),FVector(.06,Length,.06)});
  int32 Rung=0;
  for(double Y=Ground+.3;Y<RoofY+.2;Y+=.3)Out.Add({FString::Printf(TEXT("CaseLadderRung%d"),Rung++),FVector(LadderAt.X,Y,LadderAt.Z),FVector(.44,.035,.035)});
  // from the rails' backs to the wall's face, low and high
  const double Back=LadderAt.Z+.03,Depth=WallFace-Back;
  int32 Stay=0;
  for(const double Side:{-1.,1.})for(const double Y:{Ground+.7,RoofY-.35})
   Out.Add({FString::Printf(TEXT("CaseLadderStandoff%d"),Stay++),FVector(LadderAt.X+Side*.22,Y,Back+Depth*.5),FVector(.05,.05,Depth)});
  return Out;
 }
 // The second logger's field post, out on the east array edge; on whatever the ground is there.
 FVector PostPoint(const US47Main* Main){return FVector(30,Main?US47Night::SaroFloorAt(Main->Web.Get(),30,-26):0.,-26);}
 constexpr double ListenNeed=60;
 // The ladder climb (seconds): walk to the foot, climb, work at the top, climb down, hand the view back.
 constexpr double ClimbToFoot=.7,ClimbUp=2.1,ClimbWork=1.5,ClimbDown=1.7,ClimbBack=.6;
 constexpr double ClimbWorkAt=ClimbToFoot+ClimbUp+.6; // the plate is fastened mid-work
 constexpr double ClimbEnd=ClimbToFoot+ClimbUp+ClimbWork+ClimbDown+ClimbBack;
 constexpr double ClimbTop=RoofY+.5;                    // the eye at the top: half a metre over the roof's edge and the plate
 double Smooth(double X){X=FMath::Clamp(X,0.,1.);return X*X*(3-2*X);}
 FString When(double Clock)
 {
  const int32 Day=FMath::Max(1,int32(FMath::FloorToDouble((Clock-8*3600)/86400.))+1);
  return FString::Printf(TEXT("SHIFT DAY %d / %s"),Day,*FS47FreeformCases::Clock(FMath::Fmod(Clock,86400.)));
 }
}
FS47CaseFieldTests::~FS47CaseFieldTests(){EndClimb();Stop();Life.Reset();}
void FS47CaseFieldTests::StartClimb()
{
 AS47Walker* W=M?M->Player.Get():nullptr;if(!W||ClimbT>=0)return;
 ClimbFrom=FVector(W->Pos.X,W->FloorY+W->Eye,W->Pos.Z);ClimbYaw=W->Yaw;ClimbPitch=W->Pitch;ClimbRung=0;ClimbT=0;
 if(!bClimbHooked||ClimbWalker.Get()!=W)
 {
  const TWeakPtr<uint8> Guard=Life;TFunction<bool(double,const FVector2D&,const FVector2D&,FS47View&)> Prev=W->CameraOverride;
  W->CameraOverride=[this,Guard,Prev](double Dt,const FVector2D& Move,const FVector2D& LookIn,FS47View& Out)
  {
   if(Guard.IsValid()&&ClimbFrame(Dt,Out))return true;
   return Prev?Prev(Dt,Move,LookIn,Out):false;
  };
  bClimbHooked=true;
 }
 ClimbWalker=W;
 UE_LOG(LogTemp,Display,TEXT("S47: roof plate: the ladder climb begins from the eye at %.2f %.2f %.2f"),ClimbFrom.X,ClimbFrom.Y,ClimbFrom.Z);
 Present();
}
void FS47CaseFieldTests::EndClimb()
{
 if(ClimbT>=0){ClimbT=-1;Present();} // the hook stays and answers false
}
bool FS47CaseFieldTests::ClimbFrame(double Dt,FS47View& Out)
{
 if(ClimbT<0)return false;
 AS47Walker* W=ClimbWalker.Get();
 // Anything that takes the player off the ladder's side ends it at once: the preparation is already in the plan.
 if(!W||!M||M->Mode!=US47Main::EMode::Play||!M->World||M->World->bDriving||!M->Web||M->Web->Area()!=TEXT("saro")){EndClimb();return false;}
 ClimbT+=FMath::Clamp(Dt,0.,.1);
 if(ClimbT>=ClimbEnd){EndClimb();return false;}
 // The climber stands on the north side of the ladder (the wall is south of it) and faces the wall.
 const double RoofSide=LadderAt.Z-.42,Rail=LadderAt.X;
 const FVector Foot(Rail,ClimbFrom.Y,RoofSide);
 const FVector Top(Rail,ClimbTop,RoofSide);
 const double FacingYaw=PI; // towards +z, the wall
 const double UpPitch=.38,PlatePitch=FMath::Atan2(PlateAt.Y-Top.Y,FVector2D(PlateAt.X-Top.X,PlateAt.Z-Top.Z).Size());
 const double PlateYaw=FMath::Atan2(-(PlateAt.X-Top.X),-(PlateAt.Z-Top.Z));
 const double T=ClimbT;
 FVector Eye=Foot;double Yaw=FacingYaw,Pitch=UpPitch;
 auto Mix=[](double A,double B,double K){return A+(B-A)*K;};
 auto Turn=[](double From,double To,double K){double D=FMath::UnwindRadians(To-From);return From+D*K;};
 if(T<ClimbToFoot)
 {
  const double K=Smooth(T/ClimbToFoot);
  Eye=FMath::Lerp(ClimbFrom,Foot,K);Yaw=Turn(ClimbYaw,FacingYaw,K);Pitch=Mix(ClimbPitch,UpPitch,K);
 }
 else if(T<ClimbToFoot+ClimbUp)
 {
  const double K=(T-ClimbToFoot)/ClimbUp;
  Eye=FMath::Lerp(Foot,Top,K)+FVector(0,.012*FMath::Sin(K*40.),0);Pitch=Mix(UpPitch,UpPitch*.6,K);
 }
 else if(T<ClimbToFoot+ClimbUp+ClimbWork)
 {
  const double K=Smooth((T-ClimbToFoot-ClimbUp)/.5);
  Eye=Top;Yaw=Turn(FacingYaw,PlateYaw,K);Pitch=Mix(UpPitch*.6,PlatePitch,K);
 }
 else if(T<ClimbToFoot+ClimbUp+ClimbWork+ClimbDown)
 {
  const double K=(T-ClimbToFoot-ClimbUp-ClimbWork)/ClimbDown;
  const double Look=Smooth(K*3.);
  Eye=FMath::Lerp(Top,Foot,K)+FVector(0,.012*FMath::Sin(K*40.),0);Yaw=Turn(PlateYaw,FacingYaw,Look);Pitch=Mix(PlatePitch,UpPitch*.6,Look);
 }
 else
 {
  const double K=Smooth((T-ClimbToFoot-ClimbUp-ClimbWork-ClimbDown)/ClimbBack);
  Eye=FMath::Lerp(Foot,ClimbFrom,K);Yaw=Turn(FacingYaw,ClimbYaw,K);Pitch=Mix(UpPitch*.6,ClimbPitch,K);
 }
 // a hand on a rung every .32 m of the climb, up and down
 if(T>=ClimbToFoot&&T<ClimbToFoot+ClimbUp+ClimbWork+ClimbDown&&M->Audio)
 {
  const double Rung=FMath::Abs(Eye.Y-Foot.Y)/.32;
  if(FMath::FloorToDouble(Rung)!=FMath::FloorToDouble(ClimbRung)&&(T<ClimbToFoot+ClimbUp||T>ClimbToFoot+ClimbUp+ClimbWork))
   M->Audio->Play(TEXT("door_metal"),.12f,S47Three::ToUE(FVector(Rail,Eye.Y-.9,LadderAt.Z)),1.6f);
  ClimbRung=Rung;
 }
 if(ClimbT>=ClimbWorkAt-Dt&&ClimbT<ClimbWorkAt+.2)Present(); // the plate goes flat
 Out.Eye=Eye;Out.Yaw=Yaw;Out.Pitch=Pitch;Out.Roll=0;
 return true;
}
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
 const int32 Test=PlannedTest();if(Test<0||Test>2)return false;
 if(Prepared()){Start=R.Start;End=R.End;}else PeakAfter(M->Freeform->Clock(),Start,End);
 return true;
}
bool FS47CaseFieldTests::OwnsRoofWindow()const
{return Prepared()&&R.Test==0&&M->Freeform->Clock()>=R.Start&&M->Freeform->Clock()<R.End;}
FString FS47CaseFieldTests::Objective()const
{
 const int32 Test=PlannedTest();if(Test<0||Test>2)return FString();
 double Start=0,End=0;Window(Start,End);
 const FString Times=When(Start)+TEXT(" to ")+When(End)+TEXT(" (after midnight)");
 if(Test==2)
 {
  if(!Prepared())return TEXT("FIELD POST, EAST ARRAY EDGE: SET THE CONTACT PICKUPS. If set now: ")+Times+TEXT(". They need two hours before the window; set later, it moves to the following night.");
  const double At=M->Freeform->Clock();
  if(At<R.End)return TEXT("FIELD POST SET / ")+Times+TEXT(". Do other work; a window slept through records nothing.");
  return R.Gaps>0?FString(TEXT("FIELD POST: COLLECT THE PICKUPS. Part of the window went by unattended, so this records no comparison.")):FString(TEXT("FIELD POST, EAST ARRAY EDGE: COLLECT THE PICKUPS AND READ THEM."));
 }
 if(!Prepared())return (Test==0?FString(TEXT("SARO NORTH FACADE: CLIMB THE LADDER BY THE CONTROL ROOM WALL AND FASTEN THE ROOF PLATE. ")):FString(TEXT("ARRAY SERVICE GATE: SEAL THE LATCH AND LAY THE TRACKING STRIPS. ")))
  +TEXT("If prepared now: ")+Times+TEXT(". Preparation needs two hours before the window; late preparation moves it to the following night.");
 if(Test==1&&R.bGateTampered)return TEXT("GATE CONTROL DISTURBED / The numbered seal or strips were disturbed after preparation. Closing the gate does not reseal it. OPS / CASE BOARD: cancel this plan and prepare a new control, or inspect after the window to retain the incomplete result.");
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
 if(!bActive||Test<0||Test>2||Test!=PlannedTest()||!Near(Test==0?LadderAt:Test==2?PostPoint(M):GateAt,2.6))return false;
 SyncPlan();if(Prepared())return false;
 if(Test==1)M->Web->SetDoorOpen(TEXT("gate"),false,true); // sealing includes closing the existing physical leaf
 R.Prepared=M->Freeform->Clock();PeakAfter(R.Prepared,R.Start,R.End);R.LastClock=R.Prepared;
 const auto& Wind=S47Wind::Wind();R.WindReference=Wind.Strength;R.WindDirection=Wind.Dir;
 R.LastPlayer=FVector2D(M->Player->Pos.X,M->Player->Pos.Z);R.bHavePlayer=true;
 if(M->Audio)M->Audio->Play(Test==0?TEXT("rack_switch"):TEXT("small_switch"),.6f,S47Three::ToUE(Test==0?LadderAt:Test==2?PostPoint(M):GateAt));
 Changed(Objective());return true;
}
bool FS47CaseFieldTests::ReadPost()
{
 if(!Prepared()||R.Test!=2||M->Freeform->Clock()<R.End||!Near(PostPoint(M),2.6))return false;
 const bool Valid=R.Coverage>=R.End-R.Start-.01&&R.Gaps<.001;
 const bool bBoth=M->Freeform->FieldTestExpectsPresent(Line,2);
 const FString Evidence=!Valid?TEXT("Part of the window went by unattended. The pickups' roll is not continuous, so this establishes nothing; repeat the control."):
  bBoth?TEXT("Both pickups' rolls for the full window: the roof and the field post moved at the same minute, three times, a hundred and forty yards apart. Nothing on the road or the wind record accounts for that minute."):
  TEXT("Both pickups' rolls for the full window: the roof moved and the field post did not, at any minute. This excludes a simultaneous disturbance in this window only.");
 const bool Result=M->Freeform->ReportFieldTest(Line,2,Valid,bBoth,Evidence);if(Result)Changed();return Result;
}
bool FS47CaseFieldTests::InspectGate()
{
 if(!Prepared()||R.Test!=1||M->Freeform->Clock()<R.End||!Near(GateAt,2.6))return false;
 MonitorGateIntegrity();
 const bool Valid=R.Coverage>=R.End-R.Start-.01&&R.Gaps<.001&&!R.bGateTampered;
 const FString Evidence=!Valid?TEXT("The sealed-gate control lacks a complete observed world window, or its seal/strips were disturbed between preparation and inspection. No negative result is established; repeat the control."):
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
 Ladder=Plate=Seal=SealBroken=Strip=StripBack=Ribbon=Post=FS47Obj();for(auto& O:Tracks)O=FS47Obj();
}
bool FS47CaseFieldTests::Build()
{
 if(bBuilt){if(SceneIntact())return true;ForgetScene();}
 if(!M||!M->Web||!M->Web->IsLoaded()||M->Web->Area()!=TEXT("saro")||!M->Inter||!M->Freeform||!M->Freeform->Field)return false;
 const int32 Parent=M->Web->Handle(TEXT("ext.group"));if(Parent<0)return false;
 auto Box=[&](const TCHAR* Name,const FVector& At,const FVector& Size,uint32 Colour)
 {FS47Obj O=M->Freeform->Field->StageBox(Name,At,Size,Colour,Parent);if(O){Objects.Add(O);if(const auto* N=M->Web->Node(O.I))Components.Add(O.I,N->Comp);}return O;};
 {
  // the ground under both feet, as it is drawn: the walk's floor, then a ray down onto whatever is visibly there
  auto Ground=[&](double X,double Z)
  {
   double Y=M->Player&&M->Player->Floor?M->Player->Floor(X,Z):US47Night::SaroFloorAt(M->Web.Get(),X,Z);if(!FMath::IsFinite(Y))Y=-.6;
   FCollisionQueryParams Query(SCENE_QUERY_STAT(S47CaseLadderFoot),true,M->Player);
   FHitResult Hit;
   if(M->Web->GetWorld()&&M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(X,Y+1.2,Z),S47Three::ToUE(X,Y-1.2,Z),ECC_Visibility,Query))Y=Hit.ImpactPoint.Z/100.;
   return Y;
  };
  const double Foot=FMath::Max(Ground(LadderAt.X-.22,LadderAt.Z),Ground(LadderAt.X+.22,LadderAt.Z));
  UE_LOG(LogTemp,Display,TEXT("S47: roof ladder: feet on the ground at %.2f, rails to %.2f, at x %.2f z %.2f"),Foot,RoofY+1.,LadderAt.X,LadderAt.Z);
  for(const FLadderPart& Part:LadderShape(Foot))
  {
   const FS47Obj O=Box(*Part.Name,Part.At,Part.Size,LadderColour);
   if(!Ladder)Ladder=O;else LadderParts.Add(O);
  }
 }
 Plate=Box(TEXT("CaseRoofPlate"),PlateAt,FVector(.9,.012,.6),0x9a9c97);
 Ribbon=Box(TEXT("CaseWindReferenceRibbon"),LadderAt+FVector(.265,2.4,-.2),FVector(.03,.05,.45),0xd9b950); // tied to the east rail, out from the wall
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
 Post=Box(TEXT("CaseFieldPost"),PostPoint(M)+FVector(0,.6,0),FVector(.14,1.2,.14),0x8a7a5e);
 const TWeakPtr<uint8> Guard=Life;
 auto Add=[&](const TCHAR* Id,FS47Obj O,int32 Test)
 {
  FS47Interactable I;I.Id=Id;I.Object=O;I.Range=2.6;
  I.Label=[this,Guard,Test]{if(!Guard.IsValid()||!bActive||PlannedTest()!=Test)return FString();if(!Prepared())return FString(Test==0?TEXT("Climb the ladder and fasten the roof plate / retain wind reference"):TEXT("Seal the gate and lay tracking strips"));return FString(Test==1&&M->Freeform->Clock()>=R.End?TEXT("Inspect numbered gate seal and tracking strips"):TEXT("Inspect prepared field control / observation window"));};
  I.Use=[this,Guard,Test]{if(!Guard.IsValid()||!bActive)return;if(!Prepared()){if(Prepare(Test)&&Test==0)StartClimb();}else if(Test==1&&M->Freeform->Clock()>=R.End)InspectGate();else if(Near(Test==0?LadderAt:GateAt,2.6))Changed(Objective());};M->Inter->Add(I);
 };
 Add(TEXT("case:roof-plate"),Ladder,0);Add(TEXT("case:gate-seal"),Strip,1);
 {
  FS47Interactable I;I.Id=TEXT("case:field-post");I.Object=Post;I.Range=2.6;
  I.Label=[this,Guard]{if(!Guard.IsValid()||!bActive||PlannedTest()!=2)return FString();if(!Prepared())return FString(TEXT("Set the contact pickups on this post"));return FString(M->Freeform->Clock()>=R.End?TEXT("Collect the pickups and read the rolls"):TEXT("Inspect the pickups / observation window"));};
  I.Use=[this,Guard]{if(!Guard.IsValid()||!bActive)return;if(!Prepared())Prepare(2);else if(M->Freeform->Clock()>=R.End)ReadPost();else if(Near(PostPoint(M),2.6))Changed(Objective());};M->Inter->Add(I);
 }
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
 // the ladder is the building's, there whatever is planned; the plate and the ribbon belong to the roof control
 Show(Ladder,Here);Show(Plate,Roof);Show(Ribbon,Roof);for(auto O:LadderParts)Show(O,Here);
 if(Roof&&Plate)Plate.SetRotZ(Prepared()&&!(ClimbT>=0&&ClimbT<ClimbWorkAt)?0:.055); // the plate lies flat when the hands are on it
 if(Roof&&Ribbon){const auto& W=S47Wind::Wind();Ribbon.SetRotY(FMath::Atan2(W.Dir.X,W.Dir.Y));Ribbon.SetRotX(-.5+W.Strength*6+FMath::Sin(W.Time*3)*W.Strength);}
 Show(Strip,Gate);Show(StripBack,Gate);Show(Seal,Gate&&Prepared()&&!R.bGateCrossed&&!R.bGateTampered);Show(SealBroken,Gate&&Prepared()&&(R.bGateCrossed||R.bGateTampered));
 for(auto O:Tracks)Show(O,Gate&&Prepared()&&R.bGateCrossed);
 Show(Post,Here&&PlannedTest()==2);
}
void FS47CaseFieldTests::Stop(){bActive=false;Present();}
void FS47CaseFieldTests::AdvanceUnobservedTo(double Clock)
{
 if(!Prepared()||!FMath::IsFinite(Clock)||Clock<=R.LastClock)return;
 MonitorGateIntegrity();
 R.Gaps+=Overlap(R.LastClock,Clock,R.Start,R.End);R.LastClock=Clock;R.bHavePlayer=false;
}
void FS47CaseFieldTests::MonitorGateIntegrity()
{
 if(!Prepared()||R.Test!=1||!M||!M->Web)return;
 const bool Was=R.bGateTampered;
 // A physical seal exists from preparation until inspection, not only while
 // samples count toward the timed window. Closing the leaf cannot repair it.
 if(M->Web->IsDoorOpen(TEXT("fieldGate")))R.bGateTampered=true;
 if(M->Player&&M->Web->Area()==TEXT("saro"))
 {
  const FVector2D P(M->Player->Pos.X,M->Player->Pos.Z);
  if(R.bHavePlayer&&FMath::Abs(P.X-GateAt.X)<1.0&&FMath::Abs(R.LastPlayer.X-GateAt.X)<1.0
   &&(P.Y-GateAt.Z)*(R.LastPlayer.Y-GateAt.Z)<0)R.bGateTampered=true;
  R.LastPlayer=P;R.bHavePlayer=true;
 }else R.bHavePlayer=false;
 if(!Was&&R.bGateTampered)Changed(Objective());
}
void FS47CaseFieldTests::Step(double Dt)
{
 if(!bActive||!M||!M->Freeform||!M->Freeform->Active()||!FMath::IsFinite(Dt)||Dt<=0)return;
 SyncPlan();Build();Present();if(!Prepared())return;
 MonitorGateIntegrity();
 const double Now=M->Freeform->Clock(),Delta=Now-R.LastClock,From=R.LastClock;
 if(Delta<0){R.LastClock=Now;R.bHavePlayer=false;return;}
 if(Delta>3.||M->Freeform->Held()){AdvanceUnobservedTo(Now);return;}
 if(Delta<=0)return;R.LastClock=Now;
 const double Covered=Overlap(From,Now,R.Start,R.End);
 if(R.Test==2){R.Coverage+=Covered;return;}
 if(R.Test==1)
 {
  R.Coverage+=Covered;
  const double EventAt=R.Start+(R.End-R.Start)*.5;
  if(Covered>0&&From<EventAt&&Now>=EventAt&&M->Freeform->FieldTestExpectsPresent(Line,1))
  {R.bGateCrossed=true;if(M->Audio&&Near(GateAt,15))M->Audio->Play(TEXT("small_switch"),.4f,S47Three::ToUE(GateAt));Changed();}
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
 R.Job=S47J::Str(O,TEXT("job")).Left(128);R.Test=int32(N(TEXT("test"),-1,2,-1));R.Prepared=N(TEXT("prepared"),-1,3153600000.,-1);
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
 B.ResetPlan();C(B.R.Prepared<0&&B.R.Listened==0,TEXT("new or cancelled plan cannot borrow an earlier control"));
 // T16-11: the roof ladder against SARO's wall table (S47SaroClip), on the yard's ground and on the slab's
 TArray<TPair<FString,FBox>> Walls;S47SaroClip::Walls(Walls);
 for(const double Ground:{-.6,0.})
 {
  const TArray<FLadderPart> Parts=LadderShape(Ground);
  FString Into;int32 Rails=0,Stays=0;double Lowest=1e9,Highest=-1e9,West=1e9,East=-1e9,Gap=0,LastRung=-1e9,WallMost=-1e9;
  for(const FLadderPart& P:Parts)
  {
   const FBox Box(P.At-P.Size*.5,P.At+P.Size*.5);
   for(const auto& W:Walls)
   {
    const FVector Lo=Box.Min.ComponentMax(W.Value.Min),Hi=Box.Max.ComponentMin(W.Value.Max);
    if(Hi.X-Lo.X>.005&&Hi.Y-Lo.Y>.005&&Hi.Z-Lo.Z>.005)Into=P.Name+TEXT(" in ")+W.Key;
   }
   West=FMath::Min(West,Box.Min.X);East=FMath::Max(East,Box.Max.X);
   if(P.Name.StartsWith(TEXT("CaseLadderRung"))){if(LastRung>-1e8)Gap=FMath::Max(Gap,P.At.Y-LastRung);LastRung=P.At.Y;}
   else if(P.Name.StartsWith(TEXT("CaseLadderStandoff"))){++Stays;WallMost=FMath::Max(WallMost,Box.Max.Z);}
   else{++Rails;Lowest=FMath::Min(Lowest,Box.Min.Y);Highest=FMath::Max(Highest,Box.Max.Y);}
  }
  C(Into.IsEmpty(),*FString::Printf(TEXT("roof ladder (ground %.1f): no part of it in a wall%s%s"),Ground,Into.IsEmpty()?TEXT(""):TEXT(": "),*Into));
  C(Rails==2&&FMath::Abs(Lowest-Ground)<.01&&Highest>=RoofY+.9,*FString::Printf(TEXT("roof ladder (ground %.1f): both rails stand on the ground and go %.2f m past the roof's edge"),Ground,Highest-RoofY));
  C(Stays==4&&FMath::Abs(WallMost-WallFace)<.01,TEXT("roof ladder: its four standoffs end on the north wall's face, so it is fixed to the wall and not hanging off it"));
  C(West>WindowEnd&&East<CornerX&&Gap>0&&Gap<=.31&&LastRung>RoofY-.35,TEXT("roof ladder: east of the window band and short of the corner, a rung every 30 cm up to the roof"));
 }
 const FVector TopEye(LadderAt.X,ClimbTop,LadderAt.Z-.42);
 C(FVector2D(PlateAt.X-TopEye.X,PlateAt.Z-TopEye.Z).Size()<1.&&ClimbTop>PlateAt.Y&&ClimbTop<RoofY+1.,TEXT("roof ladder: from its top the eye is over the roof's edge and the plate is within reach"));
 return OK;
}

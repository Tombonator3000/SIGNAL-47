#include "S47SkyVisitors.h"
#include "S47Rhythm.h"

#include "S47Chapter6.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47FreeformObservation.h"
#include "S47FarmSite.h"
#include "S47HighwayUfo.h"
#include "S47IndoorDread.h"
#include "S47Sky.h"
#include "S47Taken.h"
#include "S47WorldLife.h"
#include "S47Evening.h"
#include "S47FieldCamera.h"
#include "S47Flow.h"
#include "S47Geo.h"
#include "S47Image.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/MaterialBillboardComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Base64.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SkyVisitors, Log, All);

namespace
{
 bool StrictBool(const S47J::FObj& O, const TCHAR* K, bool& Out)
 {
  const auto V = O.IsValid() ? O->TryGetField(K) : nullptr;
  return V.IsValid() && V->Type == EJson::Boolean && V->TryGetBool(Out);
 }
 bool StrictNumber(const S47J::FObj& O, const TCHAR* K, double& Out)
 {
  const auto V = O.IsValid() ? O->TryGetField(K) : nullptr;
  return V.IsValid() && V->Type == EJson::Number && V->TryGetNumber(Out) && FMath::IsFinite(Out);
 }
 bool TraceSkySight(US47Main* M, const FVector& Eye, const FVector& At, FString* Detail = nullptr)
 {
  if (!M || !M->Web || !M->Web->GetWorld()) { if (Detail) *Detail = TEXT("no live world"); return false; }
  FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SkyVisitorSight), true);
  const FVector From = S47Three::ToUE(Eye), End = S47Three::ToUE(At);
  for (int32 I = 0; I < 32; ++I)
  {
   FHitResult H;
   if (!M->Web->GetWorld()->LineTraceSingleByChannel(H, From, End, ECC_Visibility, Q))
   { if (Detail) *Detail = FString::Printf(TEXT("clear after %d ignored components"), I); return true; }
   const auto* C = H.GetComponent();
   if (!C) { if (Detail) *Detail = TEXT("blocking hit has no component"); return false; }
   const int32 NodeIndex = M->Web->NodeOf(C);
   const auto* Node = M->Web->Node(NodeIndex);
   bool Solid = C->IsVisible();
   if (Solid && M->Web->ItemOf(C) && Node && Node->Name.StartsWith(TEXT("proxy"))) Solid = false;
   const auto* Mat = C->GetMaterial(FMath::Max<int32>(0, H.ElementIndex));
   float Opacity = 1;
   if (Mat)
   {
    const auto Blend = Mat->GetBlendMode();
    Mat->GetScalarParameterValue(TEXT("Opacity"), Opacity);
    if (Blend == BLEND_Additive || (Blend == BLEND_Translucent && Opacity < .5f)) Solid = false;
   }
   if (Solid)
   {
    if (Detail) *Detail = FString::Printf(TEXT("actor=%s component=%s hiddenInGame=%d node=%d:%s material=%s blend=%d opacity=%.3f hit=%s distance=%.3fm ignored=%d"),
     *GetNameSafe(H.GetActor()), *C->GetPathName(), C->bHiddenInGame, NodeIndex, Node ? *Node->Name : TEXT("none"), *GetNameSafe(Mat),
     Mat ? int32(Mat->GetBlendMode()) : -1, Opacity, *S47Three::ToThree(H.ImpactPoint).ToString(), H.Distance / 100., I);
    return false;
   }
   Q.AddIgnoredComponent(C);
  }
  if (Detail) *Detail = TEXT("32 ignored components exhausted the sight trace");
  return false;
 }
}

double FS47SkyVisitorState::Duration(int32 S)
{
 const double Durations[5] = {80, 120, 150, 160, 240};
 return S >= 1 && S <= 5 ? Durations[S - 1] : 0;
}
int32 FS47SkyVisitorState::Count(int32 S) { const int32 N[5] = {1, 2, 4, 6, 9}; return S >= 1 && S <= 5 ? N[S - 1] : 0; }
int32 FS47SkyVisitorState::RequiredVisible(int32 S) { const int32 N[5] = {1, 2, 2, 3, 4}; return S >= 1 && S <= 5 ? N[S - 1] : MAX_int32; }
int32 FS47SkyVisitorState::ScheduledStage(double Clock, int32 Chapter, bool bHandoverDone)
{
 if (!FMath::IsFinite(Clock)) return 0;
 const double Night = S47Timeline::Night(Clock);
 // The evening has one light only, first due at 21:30. Observing's short clock
 // jumps past that time may start the opportunity, but cannot replace it with
 // the old 20:00/22:00 multi-light tracks. Later groups follow actual chapters.
 if (Night < 0) return bHandoverDone && Night >= S47Timeline::Sky01 ? 1 : 0;
 if (Chapter >= 5) return 5;
 if (Chapter >= 3) return 4;
 if (Chapter >= 2) return 3;
 if (Chapter >= 1) return 2;
 return bHandoverDone ? 1 : 0;
}

void FS47SkyVisitorState::Begin(const S47J::FObj& Saved, bool bNewNight)
{
 *this = FS47SkyVisitorState();
 if (!Saved.IsValid()) { bEnabled = bNewNight; return; }
 double Version = 0, Phase = 0, Time = 0;
 bool Enabled = false, Second = false;
 if (!StrictNumber(Saved, TEXT("v"), Version) || Version != 1 ||
  !StrictNumber(Saved, TEXT("stage"), Phase) || Phase < 0 || Phase > 5 || Phase != FMath::FloorToDouble(Phase) ||
  !StrictNumber(Saved, TEXT("elapsed"), Time) || Time < 0 || Time > Duration(int32(Phase)) ||
  !StrictBool(Saved, TEXT("enabled"), Enabled) || !StrictBool(Saved, TEXT("secondEarlyPass"), Second)) return;
 const auto Stages = S47J::Arr(Saved, TEXT("stages"));
 if (Stages.Num() != 5) return;
 FS47SkyVisitorState Restored;
 Restored.bEnabled = Enabled; Restored.Stage = int32(Phase); Restored.Elapsed = Time; Restored.bSecondEarlyPass = Second;
 for (int32 I = 0; I < 5; ++I)
 {
  if (!Stages[I].IsValid() || Stages[I]->Type != EJson::Object) return;
  const auto O = Stages[I]->AsObject();
  if (!StrictBool(O, TEXT("occurred"), Restored.Occurred[I]) ||
   !StrictBool(O, TEXT("observed"), Restored.Observed[I]) ||
   !StrictBool(O, TEXT("completed"), Restored.Completed[I])) return;
  if ((Restored.Observed[I] || Restored.Completed[I]) && !Restored.Occurred[I]) return;
  if (I + 1 > Restored.Stage && Restored.Occurred[I]) return;
 }
 if (Restored.Stage && (!Restored.Occurred[Restored.Stage - 1] ||
   (Restored.Completed[Restored.Stage - 1] != (Time >= Duration(Restored.Stage))))) return;
 if (Second && !Restored.Occurred[0]) return;
 *this = Restored; // Pending gaze dwell is deliberately not a saved observation.
}

bool FS47SkyVisitorState::Advance(int32 Requested, double Dt, bool bHeld)
{
 if (!bEnabled || bHeld || !FMath::IsFinite(Dt) || Dt <= 0) return false;
 Requested = FMath::Clamp(Requested, 0, 5);
 bool Changed = false;
 if (Requested > Stage)
 {
  // Optional sky opportunities can be missed. A later chapter never replays all
  // old tracks, nor marks them observed, after a clock jump or a time away.
  Stage = Requested; Elapsed = 0; Looking = 0; Occurred[Stage - 1] = true; Changed = true;
 }
 if (!Stage || Completed[Stage - 1]) return Changed;
 Elapsed = FMath::Min(Duration(Stage), Elapsed + Dt);
 if (Elapsed >= Duration(Stage)) { Completed[Stage - 1] = true; Looking = 0; Changed = true; }
 return Changed;
}
bool FS47SkyVisitorState::Observe(int32 Visible, double Dt)
{
 if (!bEnabled || Stage < 1 || Completed[Stage - 1] || Observed[Stage - 1] || !FMath::IsFinite(Dt) || Dt <= 0) return false;
 Looking = Visible >= RequiredVisible(Stage) ? Looking + Dt : 0;
 if (Looking < .65) return false;
 Observed[Stage - 1] = true; Looking = 0; return true;
}
bool FS47SkyVisitorState::OfferEarlyPass()
{
 if (!bEnabled || Stage != 1 || !Completed[0] || Observed[0] || bSecondEarlyPass) return false;
 bSecondEarlyPass = true; Completed[0] = false; Elapsed = Looking = 0; return true;
}
S47J::FObj FS47SkyVisitorState::Save() const
{
 const auto O = S47J::New();
 S47J::Set(O, TEXT("v"), 1); S47J::Set(O, TEXT("enabled"), bEnabled);
 S47J::Set(O, TEXT("stage"), Stage); S47J::Set(O, TEXT("elapsed"), Elapsed);
 S47J::Set(O, TEXT("secondEarlyPass"), bSecondEarlyPass);
 TArray<TSharedPtr<FJsonValue>> Stages;
 for (int32 I = 0; I < 5; ++I)
 {
  const auto P = S47J::New(); S47J::Set(P, TEXT("occurred"), Occurred[I]);
  S47J::Set(P, TEXT("observed"), Observed[I]); S47J::Set(P, TEXT("completed"), Completed[I]);
  Stages.Add(MakeShared<FJsonValueObject>(P));
 }
 O->SetArrayField(TEXT("stages"), Stages); return O;
}

FVector FS47SkyVisitorState::MapPosition(int32 S, int32 I, double Seconds)
{
 const double P = FMath::Clamp(Seconds / FMath::Max(1., Duration(S)), 0., 1.);
 // Road-map coordinates: the middle of this slow crossing is due north of
 // SARO's walk gate (site x=-1, z=-20). It is a distant light, never a figure.
 if (S == 1) return FVector(23 - 520 + 1040 * P, 330 + 22 * FMath::Sin(P * PI), -1180 + 80 * P);
 if (S == 2) return I == 0 ? FVector(-510 - 650 * P, 650 + 70 * P, 1410 + 360 * P)
  : FVector(-430 + 1100 * P, 760 + 70 * P, 1320 - 520 * P);
 if (S == 3)
 {
  static const FVector Starts[] = {{-760,800,-1000},{220,950,-1300},{-950,760,900},{900,900,1200}};
  const FVector A = Starts[FMath::Clamp(I, 0, 3)];
  return I % 2 ? A + FVector(650 * P, -40 * P, 310 * P) : A;
 }
 if (S == 4) return FVector(-700 + I * 105 + 800 * P, 670 + (I % 2) * 55, -1050 - I * 90 - 410 * P);
 const int32 Group = FMath::Clamp(I / 3, 0, 2), Member = I % 3;
 return FVector(-1000 + Group * 900 + Member * 75 + (Group - 1) * 440 * P,
  750 + Group * 80 + Member * 22, -850 - Member * 130 - 800 * P);
}

namespace
{
 // Minutes of the clock to the next pass, and the order they come in. At night the clock runs about 15 to 1: a pass every six minutes or so.
 double FormationGap(int32 Pass){static const double Minutes[7]={95,80,105,75,100,85,110};return Minutes[FMath::Abs(Pass)%7]*60.;}
 ES47SkyForm FormationAt(int32 Pass,bool bEvening)
 {
  // The evening is a normal job first: a row or a loose handful that could still be something else. The rest after midnight.
  static const ES47SkyForm Late[10]={ES47SkyForm::V,ES47SkyForm::Scatter,ES47SkyForm::Triangle,ES47SkyForm::Line,ES47SkyForm::Circle,ES47SkyForm::V,ES47SkyForm::Triangle,ES47SkyForm::Scatter,ES47SkyForm::Circle,ES47SkyForm::Line};
  return bEvening?(FMath::Abs(Pass)%2?ES47SkyForm::Scatter:ES47SkyForm::Line):Late[FMath::Abs(Pass)%10];
 }
 int32 FormationLights(ES47SkyForm Form,bool bEvening)
 {
  switch(Form)
  {
   case ES47SkyForm::V: return 7;
   case ES47SkyForm::Line: return bEvening?3:5;
   case ES47SkyForm::Triangle: return 6;
   case ES47SkyForm::Circle: return 8;
   default: return bEvening?4:6;
  }
 }
 double FormationHour(double Clock){const double Hour=FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;return Hour;}
}
const TCHAR* FS47SkyFormationState::Name(ES47SkyForm InForm)
{
 switch(InForm)
 {
  case ES47SkyForm::V: return TEXT("V");
  case ES47SkyForm::Line: return TEXT("line");
  case ES47SkyForm::Triangle: return TEXT("triangle");
  case ES47SkyForm::Circle: return TEXT("circle");
  default: return TEXT("scattered");
 }
}
bool FS47SkyFormationState::Open(double Clock)
{
 if(!FMath::IsFinite(Clock)||Clock<0)return false;
 const double Hour=FormationHour(Clock);
 // Saved clocks start on day one at eight: the first evening is all "normal job", so its first possible pass is after midnight.
 if(Clock<86400.+.5*3600.)return false;
 return Hour>=21.5||Hour<5.5;
}
double FS47SkyFormationState::NextOpening(double Clock)
{
 if(Open(Clock))return Clock;
 const double Start=FMath::FloorToDouble(Clock/86400.)*86400.;
 // First night: half past midnight. Afterwards today's half past nine, or tomorrow's if it has gone by.
 if(Clock<86400.+.5*3600.)return 86400.+.5*3600.;
 return Start+21.5*3600.;
}
bool FS47SkyFormationState::Update(double Clock)
{
 if(!FMath::IsFinite(Clock)||Clock<0)return false;
 if(NextClock<0||Clock>NextClock+LateBy){NextClock=NextOpening(Clock);return true;}
 return false;
}
void FS47SkyFormationState::StartNatural(double Clock,double X,double Z)
{
 const double Hour=FormationHour(Clock);const bool bEvening=Hour>=12;
 const int32 N=Passes;Passes=FMath::Min(1000000,Passes+1);
 Form=FormationAt(N,bEvening);Lights=FormationLights(Form,bEvening);
 bActive=true;bDev=false;Age=From=0;LeaveAt=-1;Looking=0;
 bFast=!bEvening&&N%3==2;bVanish=!bEvening&&!bFast&&N%4==1;
 // Each from another quarter and at its own distance: round the sky by the golden angle, off to either side of whoever is out there.
 Heading=FMath::Fmod(N*2.399963,2.*UE_DOUBLE_PI);AnchorX=X;AnchorZ=Z;
 Lateral=(N%2?-1.:1.)*(2000.+175.*((N*37)%5));Height=1500.+150.*((N*11)%5);
 const double After=Clock+FormationGap(N);
 NextClock=Open(After)?After:NextOpening(After)+((N*7)%4)*600.;
}
void FS47SkyFormationState::StartDev(ES47SkyForm InForm,int32 Pace,double X,double Z,double InHeading,double InLateral,double InHeight)
{
 Form=InForm;Lights=FormationLights(InForm,false);bActive=bDev=true;bFast=Pace==1;bVanish=Pace==2;LeaveAt=-1;Looking=0;
 // Already in the view: a slow pass starts part of the way along its crossing, and comes up from nothing where it is.
 Age=From=Seconds()*(bFast?.33:.25);
 AnchorX=X;AnchorZ=Z;Heading=InHeading;Lateral=InLateral;Height=InHeight;
}
bool FS47SkyFormationState::Finished()const
{
 if(!bActive)return false;
 if(LeaveAt>=0&&Age>=LeaveAt+1.6)return true;
 if(bVanish&&Age>=Seconds()*.6+Lights*.4+.3)return true;
 return Age>=Seconds();
}
FVector2D FS47SkyFormationState::Slot(ES47SkyForm InForm,int32 Light,int32 Count,double T)
{
 // Metres ahead along its way, and to its side. Wide enough to read as a shape from below at three or four kilometres.
 const double Mid=(Count-1)*.5;
 switch(InForm)
 {
  case ES47SkyForm::V:
  {
   if(Light==0)return FVector2D(0,0);
   const int32 Back=(Light+1)/2;
   return FVector2D(-Back*135.,(Light%2?1.:-1.)*Back*105.);
  }
  case ES47SkyForm::Line: return FVector2D(-(Light-Mid)*170.,(Light-Mid)*120.);
  case ES47SkyForm::Triangle:
  {
   static const double Rows[6][2]={{0,0},{-180,-125},{-180,125},{-360,-250},{-360,0},{-360,250}};
   return FVector2D(Rows[Light%6][0]+180.,Rows[Light%6][1]);
  }
  case ES47SkyForm::Circle:
  {
   const double A=2.*UE_DOUBLE_PI*Light/FMath::Max(1,Count)+.2*T;
   return FVector2D(290.*FMath::Cos(A),290.*FMath::Sin(A));
  }
  default:
  {
   static const double Loose[8][2]={{120,-340},{-260,-90},{310,160},{-90,330},{-380,260},{40,60},{230,-120},{-210,-300}};
   const double* L=Loose[Light%8];
   return FVector2D(L[0]+45.*FMath::Sin(T*.11+Light*1.7),L[1]+45.*FMath::Cos(T*.09+Light*2.3));
  }
 }
}
FVector FS47SkyFormationState::MapPosition(int32 Light)const
{
 const double P=FMath::Clamp(Age/Seconds(),0.,1.),Along=bFast?22000.:7000.;
 const FVector Ahead(FMath::Sin(Heading),0,FMath::Cos(Heading)),Side(FMath::Cos(Heading),0,-FMath::Sin(Heading));
 const FVector2D O=Slot(Form,Light,Lights,Age);
 // The plane of it leans towards whoever is under it, so a shape is a shape from the ground and not a line along the horizon.
 const double Lift=O.Y*(Lateral>=0?1.:-1.)*1.1+(Form==ES47SkyForm::Scatter?80.*FMath::Sin(Light*2.1):0.);
 return FVector(AnchorX,Height+Lift,AnchorZ)+Ahead*((P-.5)*Along+O.X)+Side*(Lateral+O.Y);
}
double FS47SkyFormationState::Level(int32 Light)const
{
 double L=FMath::Clamp((Age-From)/3.,0.,1.)*FMath::Clamp((Seconds()-Age)/4.,0.,1.);
 // Going out where they are, one after another and quickly.
 if(bVanish)L*=FMath::Clamp((Seconds()*.6+Light*.4-Age)/.25,0.,1.);
 if(LeaveAt>=0)L*=FMath::Clamp(1.-(Age-LeaveAt)/1.5,0.,1.);
 return L*(.82+.18*FMath::Sin(Age*(2.1+Light*.37)+Light*1.9));
}
void FS47SkyFormationState::Begin(const S47J::FObj& O)
{
 *this={};
 if(!O)return;
 const double Next=S47J::Num(O,TEXT("next"),-1);if(FMath::IsFinite(Next)&&Next>=0)NextClock=Next;
 Passes=FMath::Clamp(S47J::Int(O,TEXT("passes")),0,1000000);
 const auto Pass=S47J::Obj(O,TEXT("pass"));if(!Pass)return;
 const int32 SavedForm=S47J::Int(Pass,TEXT("form"),-1),SavedLights=S47J::Int(Pass,TEXT("lights"));
 const double Numbers[7]={S47J::Num(Pass,TEXT("age")),S47J::Num(Pass,TEXT("leave"),-1),S47J::Num(Pass,TEXT("heading")),S47J::Num(Pass,TEXT("x")),S47J::Num(Pass,TEXT("z")),S47J::Num(Pass,TEXT("side")),S47J::Num(Pass,TEXT("up"))};
 for(const double V:Numbers)if(!FMath::IsFinite(V)||FMath::Abs(V)>1e6)return;
 if(SavedForm<0||SavedForm>=int32(ES47SkyForm::Num)||SavedLights<1||SavedLights>MaxLights||Numbers[0]<0)return;
 Form=ES47SkyForm(SavedForm);Lights=SavedLights;bFast=S47J::Bool(Pass,TEXT("fast"));bVanish=S47J::Bool(Pass,TEXT("vanish"));
 Age=Numbers[0];LeaveAt=Numbers[1];Heading=Numbers[2];AnchorX=Numbers[3];AnchorZ=Numbers[4];Lateral=Numbers[5];Height=Numbers[6];
 bActive=Age<Seconds();
}
S47J::FObj FS47SkyFormationState::Save()const
{
 auto O=S47J::New();S47J::Set(O,TEXT("next"),NextClock);S47J::Set(O,TEXT("passes"),Passes);
 // A developer's pass is not the night's.
 if(bActive&&!bDev)
 {
  auto Pass=S47J::New();S47J::Set(Pass,TEXT("form"),int32(Form));S47J::Set(Pass,TEXT("lights"),Lights);S47J::Set(Pass,TEXT("fast"),bFast);S47J::Set(Pass,TEXT("vanish"),bVanish);
  S47J::Set(Pass,TEXT("age"),Age);S47J::Set(Pass,TEXT("leave"),LeaveAt);S47J::Set(Pass,TEXT("heading"),Heading);S47J::Set(Pass,TEXT("x"),AnchorX);S47J::Set(Pass,TEXT("z"),AnchorZ);S47J::Set(Pass,TEXT("side"),Lateral);S47J::Set(Pass,TEXT("up"),Height);
  S47J::Set(O,TEXT("pass"),Pass);
 }
 return O;
}
void FS47FreeformSkyState::Begin(const S47J::FObj& O,int32 NewSeed)
{
 *this={};Seed=NewSeed;
 if(!O||S47J::Int(O,TEXT("v"))!=1)return;
 const double SavedStart=S47J::Num(O,TEXT("start")),SavedElapsed=S47J::Num(O,TEXT("elapsed"));
 const int32 SavedDay=S47J::Int(O,TEXT("day")),SavedPattern=S47J::Int(O,TEXT("pattern"));
 if(!FMath::IsFinite(SavedStart)||!FMath::IsFinite(SavedElapsed)||SavedStart<0||SavedDay<0||SavedDay>1000000||SavedPattern<0||SavedPattern>3)return;
 Day=SavedDay;Pattern=SavedPattern;Start=SavedStart;Elapsed=FMath::Clamp(SavedElapsed,-86400.,Duration);
 Seed=S47J::Int(O,TEXT("seed"),NewSeed);bChosen=S47J::Bool(O,TEXT("chosen"));
 for(const auto& Place:S47J::Strs(O,TEXT("observedPlaces")))
  if(Place==TEXT("saro")||Place==TEXT("road")||Place==TEXT("station01")||Place==TEXT("diner")||Place==TEXT("roswell"))ObservedPlaces.AddUnique(Place);
}
bool FS47FreeformSkyState::Advance(int32 CurrentDay,double Clock)
{
 if(CurrentDay<1||!FMath::IsFinite(Clock)||Clock<0)return false;
 bool Changed=false;
 if(CurrentDay>Day)
 {
  Day=CurrentDay;FRandomStream R(Seed);
  bChosen=Day>=2&&R.FRand()<.22f;
  Pattern=R.RandRange(0,3);Start=(Day-1)*86400.+21*3600.+R.FRand()*6*3600.;Seed=R.GetCurrentSeed();
  ObservedPlaces.Reset();Looking=0;Changed=true;
 }
 const bool Was=Active();
 // At night 11 in-world hours take 45 active minutes. No camera-relative time.
 Elapsed=FMath::Clamp((Clock-Start)/(11.*3600./2700.),-86400.,Duration);
 return Changed||Was!=Active();
}
FVector FS47FreeformSkyState::Position(int32 Light)const
{
 double P=FMath::Clamp(Elapsed/Duration,0.,1.);
 // Four restrained tracks: crossing, a pause, separating points, return arc.
 if(Pattern==1)P=P<.35?P:P<.65?.35:.35+(P-.65)*(.65/.35);
 if(Pattern==3)P=.5-.5*FMath::Cos(P*2*PI);
 const double Split=Pattern==2&&Light==1?FMath::Max(0.,Elapsed-100)*9.:0;
 return FVector(-5800.+11600.*P,5600.+400.*FMath::Sin(P*PI)+Split,-6500.+700.*P+Split);
}
S47J::FObj FS47FreeformSkyState::Save()const
{
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("day"),Day);S47J::Set(O,TEXT("pattern"),Pattern);
 S47J::Set(O,TEXT("seed"),Seed);S47J::Set(O,TEXT("start"),Start);S47J::Set(O,TEXT("elapsed"),Elapsed);
 S47J::Set(O,TEXT("chosen"),bChosen);S47J::Set(O,TEXT("observedPlaces"),ObservedPlaces);return O;
}
void FS47SkyVisitors::BeginFreeform(const S47J::FObj& Saved)
{
 bFreeform=true;ClearPreview();Hide();
 FreeformState.Begin(Saved,M&&M->Flow&&M->Flow->bDevRun?4701986:int32(FDateTime::UtcNow().GetTicks()^FPlatformTime::Cycles64()));
 Formation.Begin(Saved?S47J::Obj(Saved,TEXT("formation")):S47J::FObj());
}
S47J::FObj FS47SkyVisitors::SaveFreeform()const
{
 auto O=FreeformState.Save();S47J::Set(O,TEXT("formation"),Formation.Save());return O;
}
void FS47SkyVisitors::StopFreeform()
{
 bFreeform=false;FreeformState.Looking=0;Formation.bActive=false;
 if(!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed))Hide();
}
void FS47SkyVisitors::StepFreeform(double Dt)
{
 auto* Session=M?M->Freeform.Get():nullptr;
 if(!bFreeform||!Session||!Session->Active()||!M->World||!M->Player||!M->Web||M->Mode!=US47Main::EMode::Play){Hide();return;}
 if(Session->Held()){FreeformState.Looking=0;return;}
 if(FreeformState.Advance(Session->Day(),Session->Clock()))M->RequestAutosave();
 const FString Area=M->World->Area;
 const bool Outdoor=Area==TEXT("saro")||Area==TEXT("road")||Area==TEXT("station01")||Area==TEXT("diner")||Area==TEXT("roswell");
 StepFormation(Dt);
 // F10's preview draws over everything here: this branch used to return before ever reading it, so in Freeform it showed nothing.
 if(Preview.bEnabled&&Area!=PreviewArea)ClearPreview();
 if(Preview.bEnabled)
 {
  Preview.Advance(Preview.Stage,Dt,false);
  if(Preview.Completed[Preview.Stage-1])ClearPreview();
  else if(Outdoor){double Fade=0;int32 Count=0;Build();DrawTrack(Preview,PreviewShift,DevBoost(),Fade,Count);FreeformState.Looking=0;return;}
 }
 // These nine points are one set: while a formation is up the single high track keeps its time and is not drawn.
 if(Formation.bActive){if(Outdoor)DrawFormation(Dt,Area);else{HidePoints();Formation.Looking=0;}FreeformState.Looking=0;return;}
 // Its time keeps going, and when the rules say it does not go under something else it is simply not drawn for that part.
 if(!FreeformState.Active()||!Outdoor||(M->Rhythm&&!M->Rhythm->Allow(TEXT("sky_high")))){Hide();FreeformState.Looking=0;return;}
 Build();if(!bBuilt){FreeformState.Looking=0;return;}
 const auto Cam=FS47Cam::Of(M->Player,30000);
 const double Fade=FMath::Clamp(FreeformState.Elapsed/4.,0.,1.)*FMath::Clamp((FS47FreeformSkyState::Duration-FreeformState.Elapsed)/8.,0.,1.);
 int32 Seen=0;FVector SeenAt=FVector::ZeroVector;
 for(int32 I=0;I<Points.Num();++I)
 {
  auto* B=Points[I].Get();if(!B)continue;
  const bool Live=I<FreeformState.Count()&&Fade>0;
  B->SetVisibility(Live);if(!Live||B->Elements.IsEmpty())continue;
  const FVector At=WorldPosition(FreeformState.Position(I),Area);
  const bool ClearWeather=M->Web->CloudSightClear(S47Three::ToUE(Cam.Pos),S47Three::ToUE(At));
  B->SetVisibility(ClearWeather);
  B->SetWorldLocation(S47Three::ToUE(At));
  const float Half=float(5.*FMath::Tan(FMath::DegreesToRadians(Cam.Fov)/2)*FVector::Dist(Cam.Pos,At)/900.*100.);
  B->Elements[0].BaseSizeX=B->Elements[0].BaseSizeY=Half;
  if(auto* D=Cast<UMaterialInstanceDynamic>(B->Elements[0].Material))D->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(0xe3eeed)*float(Fade*3.));
  B->MarkRenderStateDirty();
  const FVector P=Cam.Project(At);
  if(ClearWeather&&Fade>=.6&&P.Z<1&&FMath::Abs(P.X)<.94&&FMath::Abs(P.Y)<.94&&ClearLine(Cam.Pos,At))
  {++Seen;SeenAt=At;}
 }
 if(FreeformState.ObservedPlaces.Contains(Area)||!FMath::IsFinite(Dt)||Dt<=0)return;
 FreeformState.Looking=Seen>0?FreeformState.Looking+FMath::Min(Dt,.2):0;
 if(FreeformState.Looking<.8||!Session->Director)return;
 FreeformState.ObservedPlaces.Add(Area);FreeformState.Looking=0;
 FS47FreeformObservation O;
 O.Id=FString::Printf(TEXT("high-sky.%d.%s"),FreeformState.Day,*Area);O.Event=TEXT("high_sky_track");O.Day=Session->Day();
 O.Clock=Session->Clock();
 FVector CompassDelta=SeenAt-Cam.Pos;
 if(Area==TEXT("station01"))CompassDelta=FVector(CompassDelta.Z,CompassDelta.Y,-CompassDelta.X);
 O.Bearing=FS47FreeformObservation::Direction(FVector::ZeroVector,CompassDelta);
 O.Place=Area==TEXT("saro")?TEXT("SARO / above the distant mesa"):Area==TEXT("road")?TEXT("Highway / distant horizon"):Area==TEXT("station01")?TEXT("Station 01 / distant horizon"):TEXT("Caliche / distant horizon");
 Session->Director->RetainObservation(O);M->RequestAutosave();
}

FS47SkyVisitors::FS47SkyVisitors(US47Main* InM) : M(InM)
{
 FParse::Value(FCommandLine::Get(), TEXT("S47SkyVisit="), TestStage);
 TestStage = FMath::Clamp(TestStage, 0, 5);
 bContract = FParse::Param(FCommandLine::Get(), TEXT("S47SkyVisitContract"));
 FParse::Value(FCommandLine::Get(), TEXT("S47SkyVisitProof="), ContractRoot);
}
FS47SkyVisitors::~FS47SkyVisitors()
{
 for (auto& W : Points) if (W.IsValid()) W->DestroyComponent();
}
void FS47SkyVisitors::Begin(const S47J::FObj& Saved, bool bNewNight)
{
 ClearPreview();
 State.Begin(Saved, bNewNight); ObservationStep = 0; Hide();
 if (TestStage && !Saved.IsValid()) State.Begin(nullptr, true);
 if (State.bEnabled) Build();
}
void FS47SkyVisitors::Build()
{
 if (bBuilt || !M || !M->Web) return;
 auto* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebGlow.M_WebGlow"));
 if (!Glow) return;
 for (int32 I = 0; I < 9; ++I)
 {
  auto* B = NewObject<UMaterialBillboardComponent>(M->Web, *FString::Printf(TEXT("SkyVisitor%d"), I));
  B->SetupAttachment(M->Web->GetRootComponent()); B->SetMobility(EComponentMobility::Movable);
  auto* D = UMaterialInstanceDynamic::Create(Glow, M->Web);
  D->SetVectorParameterValue(TEXT("Color"), FLinearColor::Black);
  B->AddElement(D, nullptr, false, 50, 50, nullptr);
  B->SetCollisionEnabled(ECollisionEnabled::NoCollision); B->SetCastShadow(false);
  B->RegisterComponent(); B->SetVisibility(false); Points.Add(B);
 }
 bBuilt = true;
}
void FS47SkyVisitors::HidePoints()
{
 for (auto& B : Points) if (B.IsValid()) B->SetVisibility(false);
}
void FS47SkyVisitors::Hide()
{
 State.Looking = 0; ObservationStep = 0;
 HidePoints();
}
void FS47SkyVisitors::ClearPreview()
{
 if (!Preview.bEnabled) return;
 Preview = {}; PreviewArea.Reset();
 HidePoints();
}
void FS47SkyVisitors::DevStage(int32 Stage)
{
 ClearPreview();
 Stage = FMath::Clamp(Stage, 0, 5);
 if (!Stage || !M || !M->World || M->Mode != US47Main::EMode::Play) return;
 DevFormationStop();
 Preview.Begin(nullptr, true);
 Preview.Stage = Stage; Preview.Occurred[Stage - 1] = true;
 PreviewArea = M->World->Area;
 // The natural light is no longer on screen: a partial gaze cannot bridge a preview.
 State.Looking = 0; ObservationStep = 0;
 PreviewShift = FVector::ZeroVector;
 // Story's tracks are over SARO's north, where its nights are watched from and its contract holds them. Freeform can be anywhere:
 // there the middle of the track is brought to high up ahead of the player, or asking for it from elsewhere would show empty sky.
 if (bFreeform && M->Player)
 {
  const int32 N = FS47SkyVisitorState::Count(Stage);
  FVector Mean = FVector::ZeroVector;
  for (int32 I = 0; I < N; ++I) Mean += WorldPosition(FS47SkyVisitorState::MapPosition(Stage, I, FS47SkyVisitorState::Duration(Stage) * .25), PreviewArea);
  // The view's own direction: at the wheel the walker's yaw is not where the driver looks.
  const FS47Cam Cam = FS47Cam::Of(M->Player, 30000);
  const FVector Flat = FVector(Cam.Forward.X, 0, Cam.Forward.Z).GetSafeNormal();
  const FVector Ahead = Flat.IsNearlyZero() ? FVector(-FMath::Sin(M->Player->Yaw), 0, -FMath::Cos(M->Player->Yaw)) : Flat;
  PreviewShift = Cam.Pos + Ahead * 1500. + FVector(0, 420, 0) - Mean / FMath::Max(1, N);
 }
 HidePoints();
 UE_LOG(LogS47SkyVisitors, Display, TEXT("S47: sky preview stage %d in %s, freeform %d, %d lights, moved %s"), Stage, *PreviewArea, bFreeform ? 1 : 0, FS47SkyVisitorState::Count(Stage), *PreviewShift.ToCompactString());
}
double FS47SkyVisitors::DevBoost() const
{
 // Asked for in the day: a point of light against the bright sky would look as if nothing happened.
 const double Day = M && M->Life && M->Life->Sky ? FMath::Clamp(M->Life->Sky->Daylight(), 0., 1.) : 0.;
 return 1. + 14. * Day;
}
bool FS47SkyVisitors::DevFormation(ES47SkyForm InForm, int32 Pace)
{
 if (!M || !M->World || !M->Player || !M->Web || M->Mode != US47Main::EMode::Play) return false;
 const FString Area = M->World->Area;
 if (Area != TEXT("saro") && Area != TEXT("road") && Area != TEXT("station01") && Area != TEXT("diner") && Area != TEXT("roswell")) return false;
 ClearPreview();
 // Across the view, in the map's own terms, so it reads the same from the station's turned ground.
 const FS47Cam Cam = FS47Cam::Of(M->Player, 30000);
 const FVector Flat = FVector(Cam.Forward.X, 0, Cam.Forward.Z).GetSafeNormal();
 const FVector Ahead = Flat.IsNearlyZero() ? FVector(-FMath::Sin(M->Player->Yaw), 0, -FMath::Cos(M->Player->Yaw)) : Flat;
 const FVector Here = MapPoint(Cam.Pos, Area);
 const FVector Out = (MapPoint(Cam.Pos + Ahead * 100., Area) - Here).GetSafeNormal();
 // Side (cos h, -sin h) is out in front of the player, and the way along it crosses from one hand to the other.
 Formation.StartDev(InForm, Pace, Here.X, Here.Z, FMath::Atan2(-Out.Z, Out.X), 2600., 950.);
 UE_LOG(LogS47SkyVisitors, Display, TEXT("S47: sky formation %s, developer, %s%s, %d lights, in %s"), FS47SkyFormationState::Name(InForm), Formation.bFast ? TEXT("fast") : TEXT("slow"), Formation.bVanish ? TEXT(", goes out") : TEXT(""), Formation.Lights, *Area);
 return true;
}
void FS47SkyVisitors::DevFormationStop()
{
 if (Formation.bActive && Formation.bDev) { Formation.bActive = false; HidePoints(); }
}

bool FS47SkyVisitors::DevHighTrack()
{
 auto* Session = M ? M->Freeform.Get() : nullptr;
 if (!bFreeform || !Session || !Session->Active()) return false;
 ClearPreview(); DevFormationStop();
 // Its day first, so that the next Advance does not draw over what is set here; Elapsed follows from Start on the next step.
 FreeformState.Advance(Session->Day(), Session->Clock());
 FreeformState.bChosen = true; FreeformState.Start = Session->Clock(); FreeformState.Elapsed = 0; FreeformState.ObservedPlaces.Reset();
 return true;
}
TArray<FVector> FS47SkyVisitors::ShownPoints() const
{
 TArray<FVector> Out;
 for (const auto& W : Points) if (const auto* B = W.Get()) if (B->IsRegistered() && B->IsVisible()) Out.Add(S47Three::ToThree(B->GetComponentLocation()));
 return Out;
}
FVector FS47SkyVisitors::MapPoint(const FVector& W, const FString& Area)
{
 if (Area == TEXT("saro")) return W + FVector(S47Geo::SARO_IN_ROAD.X, 0, S47Geo::SARO_IN_ROAD.Z);
 if (Area == TEXT("station01")) { const auto R = S47Geo::StationToRoad(W.X, W.Z - 8000); return FVector(R.X, W.Y - S47Geo::STATION_DY(), R.Z); }
 return W - FVector(8000, 0, 0);
}
bool FS47SkyVisitors::FormationQuiet() const
{
 const auto* Session = M ? M->Freeform.Get() : nullptr;
 if (!Session) return false;
 // The sky is the background slot (director.json): it runs under a ground event with nothing up there. Held to every
 // Director event, a parked car or a fragment in the yard took the night's formations with it (krit9, 8 November).
 if (Session->Director && !Session->Director->LeavesSkyFree()) return false;
 if (M->HighwayUfo && M->HighwayUfo->Busy()) return false;
 if (Session->Taken && Session->Taken->Busy()) return false;
 if (Session->FarmSite && Session->FarmSite->VisitWantsGround()) return false;
 if (Session->IndoorDread && Session->IndoorDread->PeekActive()) return false;
 if (M->Rhythm && !M->Rhythm->Allow(TEXT("sky_formation"))) return false;
 return true;
}
void FS47SkyVisitors::StepFormation(double Dt)
{
 auto* Session = M->Freeform.Get();
 const double Clock = Session->Clock();
 const bool bWasWaiting = Formation.NextClock >= 0 && Clock > Formation.NextClock;
 if (Formation.Update(Clock))
 {
  if (bWasWaiting) UE_LOG(LogS47SkyVisitors, Display, TEXT("S47: sky formation not shown: the night had other things at its hour; next at %.2f h"), FMath::Fmod(Formation.NextClock, 86400.) / 3600.);
  M->RequestAutosave();
 }
 const bool bQuiet = FormationQuiet();
 if (Formation.Due(Clock) && bQuiet && !Session->InTutorial() && !Preview.bEnabled)
 {
  const FVector Here = MapPoint(M->Player->Pos, M->World->Area);
  Formation.StartNatural(Clock, Here.X, Here.Z);
  UE_LOG(LogS47SkyVisitors, Display, TEXT("S47: sky formation %d: %s, %d lights, %s%s, from the schedule at %.2f h on day %d; next at %.2f h"), Formation.Passes, FS47SkyFormationState::Name(Formation.Form), Formation.Lights, Formation.bFast ? TEXT("fast") : TEXT("slow"), Formation.bVanish ? TEXT(", goes out") : TEXT(""), FMath::Fmod(Clock, 86400.) / 3600., Session->Day(), FMath::Fmod(Formation.NextClock, 86400.) / 3600.);
  M->RequestAutosave();
 }
 if (!Formation.bActive) return;
 Formation.Age += FMath::Max(0., Dt);
 // Whatever begins out here has the night to itself: they go, and do not stand over it.
 if (!Formation.bDev && !bQuiet && Formation.LeaveAt < 0)
 {
  Formation.LeaveAt = Formation.Age;
  UE_LOG(LogS47SkyVisitors, Display, TEXT("S47: sky formation leaves at %.1f s: something else has started"), Formation.Age);
 }
 if (Formation.Finished())
 {
  UE_LOG(LogS47SkyVisitors, Display, TEXT("S47: sky formation %s over after %.1f s%s"), FS47SkyFormationState::Name(Formation.Form), Formation.Age, Formation.bDev ? TEXT(" (developer)") : TEXT(""));
  const bool bWasDev = Formation.bDev;
  Formation.bActive = Formation.bDev = false; HidePoints();
  if (!bWasDev) M->RequestAutosave();
 }
}
void FS47SkyVisitors::DrawFormation(double Dt, const FString& Area)
{
 Build();
 if (!bBuilt || !M->Web) return;
 const FS47Cam Cam = FS47Cam::Of(M->Player, 30000);
 FVector2D Size(1600, 900);
 if (M->Web->GetWorld() && M->Web->GetWorld()->GetGameViewport()) M->Web->GetWorld()->GetGameViewport()->GetViewportSize(Size);
 const double DiscPixels = (Formation.bDev ? 10. : 8.5) * FMath::Max(1., Size.Y) / 900.;
 const double Boost = Formation.bDev ? DevBoost() : 1.;
 int32 Seen = 0; FVector SeenAt = FVector::ZeroVector;
 for (int32 I = 0; I < Points.Num(); ++I)
 {
  auto* B = Points[I].Get(); if (!B) continue;
  const double Level = I < Formation.Lights ? Formation.Level(I) : 0.;
  if (Level <= 0 || B->Elements.IsEmpty()) { B->SetVisibility(false); continue; }
  const FVector At = WorldPosition(Formation.MapPosition(I), Area);
  // A developer asked to see it; the night's own stay behind the weather as the single track does.
  const bool bClearWeather = Formation.bDev || M->Web->CloudSightClear(S47Three::ToUE(Cam.Pos), S47Three::ToUE(At));
  B->SetVisibility(bClearWeather); if (!bClearWeather) continue;
  B->SetWorldLocation(S47Three::ToUE(At));
  const float Half = float(DiscPixels * FMath::Tan(FMath::DegreesToRadians(Cam.Fov) / 2) * FVector::Dist(Cam.Pos, At) / FMath::Max(1., Size.Y) * 100);
  B->Elements[0].BaseSizeX = B->Elements[0].BaseSizeY = Half;
  if (auto* D = Cast<UMaterialInstanceDynamic>(B->Elements[0].Material)) D->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(I % 3 == 2 ? 0xffdfb4 : 0xe3f2ff) * float(Level * 4. * Boost));
  B->MarkRenderStateDirty();
  const FVector P = Cam.Project(At);
  if (Level >= .5 && P.Z < 1 && FMath::Abs(P.X) < .94 && FMath::Abs(P.Y) < .94 && ClearLine(Cam.Pos, At)) { ++Seen; SeenAt = At; }
 }
 // As the high track's: one retained observation a night from each place, however many pass. The same event, the same place words.
 if (Formation.bDev || !FMath::IsFinite(Dt) || Dt <= 0) { Formation.Looking = 0; return; }
 auto* Session = M->Freeform.Get();
 Formation.Looking = Seen >= 2 ? Formation.Looking + FMath::Min(Dt, .2) : 0;
 if (Formation.Looking < .8 || !Session || !Session->Director) return;
 Formation.Looking = -1e9; // once for this pass
 FS47FreeformObservation O;
 O.Id = FString::Printf(TEXT("high-sky-form.%d.%s"), Session->Day(), *Area); O.Event = TEXT("high_sky_track"); O.Day = Session->Day(); O.Clock = Session->Clock();
 FVector CompassDelta = SeenAt - Cam.Pos;
 if (Area == TEXT("station01")) CompassDelta = FVector(CompassDelta.Z, CompassDelta.Y, -CompassDelta.X);
 O.Bearing = FS47FreeformObservation::Direction(FVector::ZeroVector, CompassDelta);
 O.Place = Area == TEXT("saro") ? TEXT("SARO / above the distant mesa") : Area == TEXT("road") ? TEXT("Highway / distant horizon") : Area == TEXT("station01") ? TEXT("Station 01 / distant horizon") : TEXT("Caliche / distant horizon");
 Session->Director->RetainObservation(O); M->RequestAutosave();
 UE_LOG(LogS47SkyVisitors, Display, TEXT("S47: sky formation seen: %d lights in view with clear sight, bearing %s"), Seen, *O.Bearing);
}

FVector FS47SkyVisitors::WorldPosition(const FVector& P, const FString& Area)
{
 if (Area == TEXT("saro")) return P - FVector(S47Geo::SARO_IN_ROAD.X, 0, S47Geo::SARO_IN_ROAD.Z);
 if (Area == TEXT("station01"))
 {
  const auto Q = S47Geo::RoadToStation(P.X, P.Z); return FVector(Q.X, P.Y + S47Geo::STATION_DY(), Q.Z + 8000);
 }
 return P + FVector(8000, 0, 0); // road, diner and Roswell share the same geographic frame.
}
bool FS47SkyVisitors::ClearLine(const FVector& Eye, const FVector& At) const
{
 return TraceSkySight(M, Eye, At);
}
int32 FS47SkyVisitors::RequestedStage() const
{
 if (TestStage) return TestStage;
 if (!M || !M->Story || !M->Story->bEveningTimeline) return 0;
 const FString& C = M->Story->ChapterKey;
 const int32 Chapter = C.StartsWith(TEXT("ch")) ? FCString::Atoi(*C.Mid(2)) : 0;
 return FS47SkyVisitorState::ScheduledStage(M->Story->Clock(), Chapter,
  M->Evening && M->Evening->Pack.Flag(TEXT("handover_done")));
}
void FS47SkyVisitors::RecordObservation(int32 Visible)
{
 static const TCHAR* Notes[] = {
  TEXT("One pale light above the mesa."),
  TEXT("Two lights visible together in the sky."),
  TEXT("More than one light in the sky, in separate parts of the view."),
  TEXT("A row of lights above the field."),
  TEXT("Separate groups of lights over the horizon.")};
 FString Text = Notes[State.Stage - 1];
 if (State.Stage == 5 && State.Observed[0]) Text += TEXT(" More lights than the first one I saw.");
 if (M->Story) M->Story->Note(Text);
 UE_LOG(LogS47SkyVisitors, Display, TEXT("S47: SKY%02d observed: %d world lights in view with clear sight"), State.Stage, Visible);
 M->RequestAutosave();
}
void FS47SkyVisitors::Step(double Dt)
{
 if(bFreeform&&M&&M->Freeform&&M->Freeform->Active()){StepFreeform(Dt);return;}
 ControlTest(Dt);
 if (!M || !M->Story || !M->Player || !M->World || M->Mode != US47Main::EMode::Play)
 { ClearPreview(); Hide(); return; }
 if (Preview.bEnabled && (M->World->Area != PreviewArea || M->World->Area == TEXT("room6"))) ClearPreview();
 // F10's formation in a Story night: nothing natural comes in formation there.
 if (Formation.bActive && Formation.bDev)
 {
  const bool bHoldDev = M->bPausedByMenu || M->ModalOpen() || M->Story->IsCinematic();
  if (!bHoldDev) Formation.Age += FMath::Max(0., Dt);
  if (Formation.Finished()) DevFormationStop(); else { DrawFormation(Dt, M->World->Area); return; }
 }
 if (!State.bEnabled && !Preview.bEnabled) { Hide(); return; }
 // The authored final journey owns its sky. Keep an earlier visitor track and its
 // observation clock untouched while C6 is active, including the empty-car pullout.
 if (const auto* Finale = Cast<US47Chapter6>(M->Story->Chapter(TEXT("ch6"))); Finale && Finale->bActive)
 { ClearPreview(); Hide(); return; }
 const bool Held = M->bPausedByMenu || M->ModalOpen() || M->Story->IsCinematic();
 if (State.Advance(RequestedStage(), Dt, Held)) M->RequestAutosave();
 // An early miss gets one further natural opportunity on the first field trip.
 // Loading, opening a door and simply walking outside never restart a track.
 if (!Held && M->Evening && M->Evening->ServicePending() && M->World->bDriving &&
  M->World->Area == TEXT("road") && State.OfferEarlyPass()) M->RequestAutosave();
 // Natural time follows its existing rules even while an F10 preview is drawn.
 // Preview time has the same holds, but cannot autosave or grant observation credit.
 if (Preview.bEnabled)
 {
  Preview.Advance(Preview.Stage, Dt, Held);
  if (Preview.Completed[Preview.Stage - 1]) ClearPreview();
 }
 const FS47SkyVisitorState& VisibleState = ActiveState();
 if (!VisibleState.Stage || VisibleState.Completed[VisibleState.Stage - 1] || M->World->Area == TEXT("room6")) { Hide(); return; }
 Build();
 if (!bBuilt || Points.Num() != 9) { Hide(); return; }
 const FS47Cam Cam = FS47Cam::Of(M->Player, 12000);
 double Fade = 0; int32 Count = 0;
 DrawTrack(VisibleState, FVector::ZeroVector, 1., Fade, Count);
 if (Preview.bEnabled || Held || Dt <= 0) { State.Looking = 0; ObservationStep = 0; return; }
 ObservationStep += Dt;
 if (ObservationStep < .12) return;
 const double Sample = FMath::Min(ObservationStep, .2); ObservationStep = 0;
 int32 Visible = 0;
 if (Fade >= .6)
  for (int32 I = 0; I < Count; ++I)
  {
   const auto* B = Points[I].Get();
   if (!B || !B->IsRegistered() || !B->IsVisible() || B->Elements.IsEmpty() || !B->Elements[0].Material) continue;
   const FVector At = WorldPosition(FS47SkyVisitorState::MapPosition(State.Stage, I, State.Elapsed), M->World->Area);
   const FVector P = Cam.Project(At);
   if (P.Z < 1 && FMath::Abs(P.X) < .94 && FMath::Abs(P.Y) < .94 && ClearLine(Cam.Pos, At)) ++Visible;
  }
 if (State.Observe(Visible, Sample)) RecordObservation(Visible);
}

void FS47SkyVisitors::DrawTrack(const FS47SkyVisitorState& Track, const FVector& Shift, double Boost, double& Fade, int32& Count)
{
 Fade = 0; Count = 0;
 if (!bBuilt || Points.Num() != 9) return;
 const FS47Cam Cam = FS47Cam::Of(M->Player, 12000);
 FVector2D Size(1600, 900);
 if (M->Web && M->Web->GetWorld() && M->Web->GetWorld()->GetGameViewport()) M->Web->GetWorld()->GetGameViewport()->GetViewportSize(Size);
 Fade = FMath::Clamp(Track.Elapsed / 3, 0., 1.) * FMath::Clamp((FS47SkyVisitorState::Duration(Track.Stage) - Track.Elapsed) / 8, 0., 1.);
 Count = FS47SkyVisitorState::Count(Track.Stage);
 // Keep SKY01's original six-pixel introduction. Later groups get a larger
 // angular footprint, referenced to 900p so higher resolutions do not hide them.
 // Only the sprite changes: the world paths, sight and observation rules remain.
 static const double LaterDiscPixels[] = {10., 11., 12., 14.};
 const double DiscPixels = Track.Stage == 1 ? 6.4
  : LaterDiscPixels[FMath::Clamp(Track.Stage - 2, 0, 3)] * FMath::Max(1., Size.Y) / 900.;
 for (int32 I = 0; I < Points.Num(); ++I)
 {
  auto* B = Points[I].Get(); if (!B) continue;
  B->SetVisibility(I < Count && Fade > 0);
  if (I >= Count || B->Elements.IsEmpty()) continue;
  const FVector At = WorldPosition(FS47SkyVisitorState::MapPosition(Track.Stage, I, Track.Elapsed), M->World->Area) + Shift;
  B->SetWorldLocation(S47Three::ToUE(At));
  // World location and depth remain physical; only the distant glow keeps size.
  const float Half = float(DiscPixels * FMath::Tan(FMath::DegreesToRadians(Cam.Fov) / 2) * FVector::Dist(Cam.Pos, At) / FMath::Max(1., Size.Y) * 100);
  B->Elements[0].BaseSizeX = B->Elements[0].BaseSizeY = Half;
  if (auto* D = Cast<UMaterialInstanceDynamic>(B->Elements[0].Material))
   D->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(I % 3 == 2 ? 0xffdfb4 : 0xe3f2ff) * float(Fade * 4.0 * Boost));
  B->MarkRenderStateDirty();
 }
}

bool FS47SkyVisitors::Check(bool Good, const FString& What)
{
 ++ContractChecks;
 UE_LOG(LogS47SkyVisitors, Display, TEXT("S47: sky contract %s: %s"), Good ? TEXT("PASS") : TEXT("FAIL"), *What);
 if (Good) return true;
 bContractDone = true;
 auto O = S47J::New(); S47J::Set(O,TEXT("status"),TEXT("FAIL")); S47J::Set(O,TEXT("reason"),What);
 S47J::Set(O,TEXT("phase"),ContractPhase); S47J::Set(O,TEXT("checks"),ContractChecks);
 if (!ContractRoot.IsEmpty()) FFileHelper::SaveStringToFile(S47J::Write(O,true), *(ContractRoot / TEXT("result.json")));
 FPlatformMisc::RequestExitWithStatus(false, 1); return false;
}
void FS47SkyVisitors::ControlTest(double Dt)
{
 if (!bContract || bContractDone || !M || !M->Flow || !M->World || !M->Player || !M->Story) return;
 ContractTime += Dt;
 if (ContractTime > 100) { Check(false,TEXT("world sight/save contract timed out")); return; }
 if (!ContractPhase)
 {
  FString User;
  FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
  if (ContractRoot.IsEmpty() || !ContractRoot.StartsWith(TEXT("/tmp/")) || !User.StartsWith(TEXT("/tmp/")) || M->Flow->CaseId != 0)
  { bContractDone = true; UE_LOG(LogS47SkyVisitors,Error,TEXT("S47: sky contract requires developer case zero and /tmp proof/UserDir")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
  IFileManager::Get().MakeDirectory(*ContractRoot,true);
  M->World->Enter(TEXT("saro")); M->Player->Place(-1,-18.5,0); M->Player->Pitch = -.5;
  State.Begin(nullptr,true); TestStage = 1; ContractPhase = 1; ContractSince = ContractTime;
  return;
 }
 const double Since = ContractTime - ContractSince;
 if (ContractPhase == 1)
 {
  if (Since < 4) return;
  if (!Check(State.Occurred[0] && !State.Observed[0],TEXT("early light moves in the world while looking down without journal credit"))) return;
  const FVector Target = WorldPosition(FS47SkyVisitorState::MapPosition(1,0,State.Elapsed),TEXT("saro"));
  if (!Check(!ClearLine(FVector(0,1.6,0),Target),TEXT("actual control-room roof or wall occludes the sky sight ray"))) return;
  HeldTime = State.Elapsed; M->bPausedByMenu = true; ContractPhase = 2; ContractSince = ContractTime; return;
 }
 if (ContractPhase == 2)
 {
  if (Since < .8) return;
  if (!Check(State.Elapsed == HeldTime && !State.Observed[0],TEXT("menu pause freezes actual world trajectory and observation"))) return;
  M->bPausedByMenu = false;
  const auto Full = M->Flow->GameState();
  if (!Check(FFileHelper::SaveStringToFile(S47J::Write(Full,true),*(ContractRoot/TEXT("midpass.json"))),TEXT("write real full-case snapshot before seeing the light"))) return;
  FString Bytes; FFileHelper::LoadFileToString(Bytes,*(ContractRoot/TEXT("midpass.json")));
  const auto Saved = S47J::Read(Bytes);
  if (!Check(Saved.IsValid(),TEXT("fresh reader parses the on-disk case"))) return;
  US47Flow::FStart Start; Start.State = Saved; Start.bDev = true; Start.CaseId = 0;
  M->Flow->StartGame(Start);
  if (!Check(State.Elapsed == HeldTime && State.Occurred[0] && !State.Observed[0],TEXT("full Flow restore retains midpass time and unseen status"))) return;
  M->World->Enter(TEXT("saro")); M->Player->Place(-1,-18.5,0);
  ContractPhase = 3; ContractSince = ContractTime;
 }
 // This test follows the physical targets using the ordinary Walker view. Production
 // code never changes the camera or follows player position with the light path.
 FVector Center = FVector::ZeroVector;
 const int32 N = FS47SkyVisitorState::Count(TestStage);
 const int32 AimN = TestStage == 3 ? 2 : N;
 for (int32 I = 0; I < AimN; ++I) Center += WorldPosition(FS47SkyVisitorState::MapPosition(TestStage,I,State.Elapsed),TEXT("saro"));
 Center /= FMath::Max(1,AimN);
 const FVector D = Center - S47Three::ToThree(M->Player->EyeUE());
 M->Player->Yaw = FMath::Atan2(-D.X,-D.Z); M->Player->Pitch = FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
 if (ContractPhase == 3 && Since >= 2)
 {
  // One diagnostic sample after the ordinary Walker has applied the test aim.
  // The close gate fixture deliberately catches the opaque crossrail. Give its
  // screenshot a frame before moving the positive camera fixture farther back.
  ContractPhase = 4;
  const FS47Cam Cam = FS47Cam::Of(M->Player,12000);
  const FVector At = WorldPosition(FS47SkyVisitorState::MapPosition(TestStage,0,State.Elapsed),M->World->Area);
  const FVector Projected = Cam.Project(At);
  FString Sight; const bool Clear = TraceSkySight(M,Cam.Pos,At,&Sight);
  const bool Modal = M->ModalOpen(), Cinematic = M->Story->IsCinematic();
  const auto* Point = Points.IsValidIndex(0) ? Points[0].Get() : nullptr;
  const bool PointVisible = Point && Point->IsVisible(), Registered = Point && Point->IsRegistered();
  const bool Material = Point && !Point->Elements.IsEmpty() && Point->Elements[0].Material;
  const FString ActualPoint = Point ? S47Three::ToThree(Point->GetComponentLocation()).ToString() : TEXT("none");
  UE_LOG(LogS47SkyVisitors,Display,TEXT("S47: sky phase3 diagnostic stage=%d requested=%d elapsed=%.6f looking=%.6f observed=%d completed=%d held=%d modal=%d menu=%d storyPaused=%d cinematic=%d driving=%d area=%s eye=%s forward=%s target=%s projection=%s actorRotation=%s yaw=%.6f pitch=%.6f built=%d pointVisible=%d registered=%d material=%d actualPoint=%s clear=%d trace=%s"),
   State.Stage,RequestedStage(),State.Elapsed,State.Looking,State.Stage > 0 && State.Observed[State.Stage-1],State.Stage > 0 && State.Completed[State.Stage-1],
   M->bPausedByMenu || Modal || Cinematic,Modal,M->bPausedByMenu,M->Story->IsPaused(),Cinematic,M->World->bDriving,*M->World->Area,
   *Cam.Pos.ToString(),*Cam.Forward.ToString(),*At.ToString(),*Projected.ToString(),*M->Player->GetActorRotation().ToString(),
   M->Player->Yaw,M->Player->Pitch,bBuilt,PointVisible,Registered,Material,*ActualPoint,Clear,*Sight);
  auto Diagnostic = S47J::New();
  S47J::Set(Diagnostic,TEXT("stage"),State.Stage); S47J::Set(Diagnostic,TEXT("elapsed"),State.Elapsed);
  S47J::Set(Diagnostic,TEXT("held"),M->bPausedByMenu || Modal || Cinematic); S47J::Set(Diagnostic,TEXT("modal"),Modal);
  S47J::Set(Diagnostic,TEXT("menuPaused"),M->bPausedByMenu); S47J::Set(Diagnostic,TEXT("storyPaused"),M->Story->IsPaused());
  S47J::Set(Diagnostic,TEXT("cinematic"),Cinematic); S47J::Set(Diagnostic,TEXT("area"),M->World->Area);
  S47J::Set(Diagnostic,TEXT("eye"),Cam.Pos.ToString()); S47J::Set(Diagnostic,TEXT("forward"),Cam.Forward.ToString());
  S47J::Set(Diagnostic,TEXT("target"),At.ToString()); S47J::Set(Diagnostic,TEXT("projection"),Projected.ToString());
  S47J::Set(Diagnostic,TEXT("pointVisible"),PointVisible); S47J::Set(Diagnostic,TEXT("registered"),Registered);
  S47J::Set(Diagnostic,TEXT("material"),Material); S47J::Set(Diagnostic,TEXT("actualPoint"),ActualPoint);
  S47J::Set(Diagnostic,TEXT("clearLine"),Clear); S47J::Set(Diagnostic,TEXT("trace"),Sight);
  S47J::Set(Diagnostic,TEXT("skyVisitors"),State.Save());
  FFileHelper::SaveStringToFile(S47J::Write(Diagnostic,true),*(ContractRoot/TEXT("phase3-diagnostic.json")));
  FScreenshotRequest::RequestScreenshot(ContractRoot/TEXT("phase3-diagnostic.png"),false,false);
  if (!Check(FVector2D(Cam.Pos.X+1,Cam.Pos.Z+18.5).Size() < .03,
   TEXT("near-gate negative fixture uses the actual Walker eye after the restore has settled"))) return;
  if (!Check(!Clear && !State.Observed[0],TEXT("actual opaque gate rail occludes the near-gate sky fixture without observation credit"))) return;
  return;
 }
 if (ContractPhase == 4)
 {
  if (Since < 2.15) return;
  if (!Check(M->Player->Walkable(-1,-15),TEXT("positive sky camera-position fixture is a walkable five-metre setback from the gate"))) return;
  M->Player->Place(-1,-15,0);
  UE_LOG(LogS47SkyVisitors,Display,TEXT("S47: sky positive sight camera-position fixture at x=-1 z=-15; authored light path and physical occlusion unchanged"));
  ContractPhase = 5; ContractSince = ContractTime;
  return; // The ordinary Walker tick applies this eye position and the subsequent aim.
 }
 if (State.Stage == TestStage && State.Observed[TestStage - 1] && Since > 4)
 {
  if (!Check(true,FString::Printf(TEXT("SKY%02d seen through actual projected and unoccluded sight"),TestStage))) return;
  if (TestStage == 1)
  {
   // Compare two real film captures at the same pose/seed. Only the world light
   // is hidden for the reference. Journal state is never used as pixel proof.
   if (!Check(M->FieldCamera != nullptr,TEXT("field camera exists for the optional gate photograph"))) return;
   M->FieldCamera->Sync();
   const FVector At = WorldPosition(FS47SkyVisitorState::MapPosition(1,0,State.Elapsed),TEXT("saro"));
   const FS47Projection P = M->FieldCamera->Project(At);
   if (!Check(P.bFront && P.U > .03 && P.U < .97 && P.V > .03 && P.V < .97,TEXT("SKY01 projects inside the actual field camera from the gate"))) return;
   FS47SnapOptions O; O.W = 960; O.Seed = 47;
   const FS47Snap With = M->FieldCamera->Snap(O);
   TArray<bool> Visible;
   for (auto& B : Points) { Visible.Add(B.IsValid() && B->IsVisible()); if (B.IsValid()) B->SetVisibility(false); }
   const FS47Snap Without = M->FieldCamera->Snap(O);
   for (int32 I = 0; I < Points.Num(); ++I) if (Points[I].IsValid()) Points[I]->SetVisibility(Visible[I]);
   if (!Check(With.bRendered && Without.bRendered && With.Px.Num() == Without.Px.Num(),TEXT("two native scene captures render for the photograph comparison"))) return;
   const int32 X = FMath::RoundToInt(P.U * With.W), Y = FMath::RoundToInt(P.V * With.H);
   int32 Pixels = 0, Peak = 0;
   for (int32 V = FMath::Max(0,Y-12); V <= FMath::Min(With.H-1,Y+12); ++V)
    for (int32 U = FMath::Max(0,X-12); U <= FMath::Min(With.W-1,X+12); ++U)
    {
     const FColor A = With.Px[V*With.W+U], B = Without.Px[V*With.W+U];
     const int32 Delta = int32(A.R)+A.G+A.B-int32(B.R)-B.G-B.B;
     Peak = FMath::Max(Peak,Delta); if (Delta >= 20) ++Pixels;
    }
   if (!Check(Pixels >= 2,FString::Printf(TEXT("SKY01 contributes actual photograph pixels near its projection (%d pixels, peak delta %d)"),Pixels,Peak))) return;
   const FString Url = S47Image::ToDataUrl(With.Px,With.W,With.H,false);
   int32 Comma = INDEX_NONE; TArray<uint8> Png;
   if (!Check(Url.FindChar(TEXT(','),Comma) && FBase64::Decode(Url.Mid(Comma+1),Png) &&
    FFileHelper::SaveArrayToFile(Png,*(ContractRoot/TEXT("sky01-field-photo.png"))),TEXT("preserve the native field-camera photograph as PNG"))) return;
  }
  FScreenshotRequest::RequestScreenshot(ContractRoot/FString::Printf(TEXT("sky%02d.png"),TestStage),false,false);
  const auto Snapshot = S47J::Clone(State.Save()); FS47SkyVisitorState Restored; Restored.Begin(Snapshot,false);
  if (!Check(Restored.Observed[TestStage-1] && Restored.Stage == TestStage && Restored.Elapsed == State.Elapsed,TEXT("phase position and actual observation survive serialization"))) return;
  if (TestStage < 5)
  {
   ++TestStage; ContractSince = ContractTime; return;
  }
  bContractDone = true;
  auto O = S47J::New(); S47J::Set(O,TEXT("status"),TEXT("PASS")); S47J::Set(O,TEXT("checks"),ContractChecks);
  S47J::Set(O,TEXT("elapsed"),ContractTime); S47J::Set(O,TEXT("skyVisitors"),State.Save());
  FFileHelper::SaveStringToFile(S47J::Write(O,true),*(ContractRoot/TEXT("result.json")));
  UE_LOG(LogS47SkyVisitors,Display,TEXT("S47: sky contract PASS complete (%d checks)"),ContractChecks);
  M->Story->After(1., [] { FPlatformMisc::RequestExit(false); });
 }
}

bool FS47SkyVisitors::SelfTest(TArray<FString>& Lines)
{
 bool Good = true;
 auto Check = [&](bool B, const TCHAR* Text) { Lines.Add(FString::Printf(TEXT("%s sky visitors: %s"), B ? TEXT("ok") : TEXT("FAIL"), Text)); Good &= B; };
 Check(FS47SkyVisitorState::ScheduledStage(21*3600+29*60,0,true) == 0 &&
  FS47SkyVisitorState::ScheduledStage(21*3600+30*60,0,true) == 1 &&
  FS47SkyVisitorState::ScheduledStage(-2*3600-30*60,0,true) == 1,
  TEXT("first evening light starts at 21:30 on either clock representation"));
 Check(FS47SkyVisitorState::ScheduledStage(18*3600,0,true) == 0 &&
  FS47SkyVisitorState::ScheduledStage(21*3600+30*60,0,false) == 0,
  TEXT("handover alone cannot start a light and unfinished handover has none"));
 Check(FS47SkyVisitorState::ScheduledStage(22*3600+5*60,0,true) == 1 &&
  FS47SkyVisitorState::ScheduledStage(22*3600+50*60,2,true) == 1,
  TEXT("Observing clock jumps retain the first single-light opportunity without early groups"));
 Check(FS47SkyVisitorState::ScheduledStage(2*3600,1,true) == 2 &&
  FS47SkyVisitorState::ScheduledStage(2*3600+15*60,2,true) == 3 &&
  FS47SkyVisitorState::ScheduledStage(3*3600,3,true) == 4 &&
  FS47SkyVisitorState::ScheduledStage(4*3600,5,true) == 5,
  TEXT("later world formations follow actual night chapters"));
 FS47SkyVisitorState S; S.Begin(nullptr, false);
 Check(!S.bEnabled && !S.Advance(5, 2, false), TEXT("legacy cases gain no retroactive sky sequence"));
 S.Begin(nullptr,true);
 S.Advance(FS47SkyVisitorState::ScheduledStage(20*3600+45*60,0,true),1,false);
 Check(S.Stage == 0 && !S.Occurred[0],TEXT("the 20:45 observing run is too early for SKY01"));
 S.Advance(FS47SkyVisitorState::ScheduledStage(22*3600+5*60,0,true),1,false);
 Check(S.Stage == 1 && S.Occurred[0] && !S.Observed[0] && !S.Occurred[1],
  TEXT("a short clock jump across 21:30 starts one optional unseen light, not a later formation"));
 S.Begin(nullptr, true); S.Advance(1, 5, false);
 Check(S.Stage == 1 && S.Occurred[0] && !S.Observed[0] && !S.Completed[0], TEXT("occurrence does not imply observation or completion"));
 const double Before = S.Elapsed; S.Advance(5, 20, true);
 Check(S.Stage == 1 && S.Elapsed == Before, TEXT("pause and modal cannot advance the global phase"));
 S.Observe(0, 20); Check(!S.Observed[0], TEXT("offscreen and occluded light gives no observation"));
 S.Observe(1, .4); S.Observe(0, .1); S.Observe(1, .4);
 Check(!S.Observed[0], TEXT("discontinuous glimpses do not manufacture a sustained view"));
 S.Looking = 0; const auto Early = S.Save(); FS47SkyVisitorState R; R.Begin(S47J::Clone(Early), false);
 Check(R.bEnabled && R.Stage == 1 && R.Elapsed == Before && !R.Observed[0], TEXT("Continue retains the exact route time without resetting the pass"));
 R.Advance(1, 100, false); Check(R.Completed[0] && !R.Observed[0] && R.OfferEarlyPass(), TEXT("missed first pass gets one field-trip opportunity"));
 R.Advance(1, 100, false); Check(!R.OfferEarlyPass(), TEXT("second early pass cannot become a doorway replay"));
 S.Looking = 0; Check(S.Observe(1, .7) && !S.Observe(1, 1), TEXT("actual sustained sight records exactly once"));
 S.Advance(4, 1, false); Check(S.Stage == 4 && !S.Occurred[1] && !S.Observed[2], TEXT("time loss does not queue or fabricate missed stages"));
 Check(!S.Completed[0], TEXT("interrupted earlier trajectory is not marked naturally complete"));
 Check(!S.Observe(2, 1) && S.Observe(3, .7), TEXT("formation claim needs multiple simultaneous visible lights"));
 S.Advance(1, 2, false); Check(S.Stage == 4, TEXT("old clock or chapter cannot rewind the phase"));
 R.Begin(S47J::Clone(S.Save()), false); Check(R.bEnabled && R.Stage == 4 && R.Observed[3], TEXT("observed phase survives a serialized restore"));
 auto Bad = S47J::Clone(S.Save()); Bad->SetBoolField(TEXT("stage"), true); R.Begin(Bad, true);
 Check(!R.bEnabled, TEXT("corrupt bool-to-number phase is rejected atomically"));
 Bad = S47J::Clone(S.Save()); S47J::Set(Bad, TEXT("elapsed"), -1.); R.Begin(Bad, true);
 Check(!R.bEnabled, TEXT("negative or invalid track time grants no sequence"));
 for (int32 P = 1; P <= 5; ++P)
 {
  const FVector A = FS47SkyVisitorState::MapPosition(P, 0, 0), B = FS47SkyVisitorState::MapPosition(P, 0, 40);
  Check(!A.Equals(B, .1) || P == 3, TEXT("overflight is a world trajectory independent of player movement"));
  Check(FS47SkyVisitorState::Count(P) >= FS47SkyVisitorState::RequiredVisible(P), TEXT("each authored sight threshold is physically attainable"));
 }
 const FVector P(55, 700, 1200), Road = WorldPosition(P, TEXT("road")), Site = WorldPosition(P, TEXT("saro"));
 Check(Road.Equals(P + FVector(8000,0,0), 1e-9) && (Site + FVector(24,0,-300)).Equals(P,1e-9), TEXT("SARO and road draw the same geographic path in their distinct world frames"));
 const FVector GateLight = WorldPosition(FS47SkyVisitorState::MapPosition(1,0,FS47SkyVisitorState::Duration(1)/2),TEXT("saro"));
 Check(FMath::Abs(GateLight.X+1) < 1e-9 && GateLight.Y > 300 && GateLight.Z < -600,
  TEXT("single world light crosses the northern sky above the SARO gate sector"));
 FS47FreeformSkyState F;F.Begin(nullptr,4701986);F.Advance(1,8*3600.);
 Check(!F.bChosen&&!F.Active(),TEXT("Freeform first workday has no forced sky encounter"));
 const int32 SpentSeed=F.Seed;F.Advance(1,22*3600.);
 Check(F.Seed==SpentSeed&&!F.bChosen,TEXT("a quiet night cannot reroll while waiting or looking away"));
 F.Day=2;F.bChosen=true;F.Start=86400.+22*3600.;F.Pattern=2;
 F.Advance(2,F.Start+30.*(11.*3600./2700.));const FVector First=F.Position(0);
 FS47FreeformSkyState Copy;Copy.Begin(S47J::Read(S47J::Write(F.Save())),0);
 Check(Copy.Active()&&Copy.Position(0).Equals(First,1e-6)&&Copy.Seed==F.Seed,TEXT("Freeform midpass JSON restores exact world position and spent random draw"));
 Copy.Advance(2,Copy.Start+120.*(11.*3600./2700.));
 Check(!Copy.Position(0).Equals(First,.1)&&Copy.Count()==2&&!Copy.Position(1).Equals(Copy.Position(0),.1)&&Copy.ObservedPlaces.IsEmpty(),TEXT("unseen tracks continue and separate without inventing observations"));
 Copy.ObservedPlaces.Add(TEXT("saro"));F.Begin(Copy.Save(),0);
 Check(F.ObservedPlaces.Contains(TEXT("saro")),TEXT("a retained location cannot grant duplicate observation on Continue"));
 F.Advance(2,F.Start+6*3600.);
 Check(!F.Active(),TEXT("sleep or missed time expires a passage without a catch-up replay"));
 FS47FreeformSkyState Quiet;Quiet.Begin(nullptr,4701986);int32 Chosen=0;
 for(int32 Day=2;Day<102;++Day){Quiet.Advance(Day,(Day-1)*86400.+8*3600.);Chosen+=Quiet.bChosen;}
 Check(Chosen>0&&Chosen<40,TEXT("seeded Freeform opportunities remain sparse with many quiet nights"));
 // Formations: what each one is, when they come, what stops them.
 using FForm=FS47SkyFormationState;
 const auto Ahead=[](ES47SkyForm Form,int32 I,int32 N){return FForm::Slot(Form,I,N,0).X;};
 const auto Across=[](ES47SkyForm Form,int32 I,int32 N){return FForm::Slot(Form,I,N,0).Y;};
 bool Vee=true;for(int32 I=1;I<7;++I)Vee&=Ahead(ES47SkyForm::V,I,7)<Ahead(ES47SkyForm::V,0,7);
 Vee&=FMath::IsNearlyEqual(Across(ES47SkyForm::V,1,7),-Across(ES47SkyForm::V,2,7))&&FMath::IsNearlyEqual(Across(ES47SkyForm::V,5,7),-Across(ES47SkyForm::V,6,7))&&FMath::Abs(Across(ES47SkyForm::V,5,7))>FMath::Abs(Across(ES47SkyForm::V,1,7));
 Check(Vee,TEXT("the V has its point ahead and its two arms spreading back"));
 bool Row=true;for(int32 I=2;I<5;++I)Row&=FMath::IsNearlyEqual((Ahead(ES47SkyForm::Line,I,5)-Ahead(ES47SkyForm::Line,0,5))*(Across(ES47SkyForm::Line,1,5)-Across(ES47SkyForm::Line,0,5)),(Ahead(ES47SkyForm::Line,1,5)-Ahead(ES47SkyForm::Line,0,5))*(Across(ES47SkyForm::Line,I,5)-Across(ES47SkyForm::Line,0,5)),1.);
 Check(Row,TEXT("the line is straight"));
 bool Ring=true;for(int32 I=0;I<8;++I)Ring&=FMath::IsNearlyEqual(FForm::Slot(ES47SkyForm::Circle,I,8,40).Size(),290.,.01);
 Check(Ring&&!FForm::Slot(ES47SkyForm::Circle,0,8,0).Equals(FForm::Slot(ES47SkyForm::Circle,0,8,40),1.),TEXT("the circle holds its radius and turns"));
 Check(Ahead(ES47SkyForm::Triangle,0,6)>Ahead(ES47SkyForm::Triangle,1,6)&&Ahead(ES47SkyForm::Triangle,1,6)>Ahead(ES47SkyForm::Triangle,3,6)&&FMath::Abs(Across(ES47SkyForm::Triangle,5,6))>FMath::Abs(Across(ES47SkyForm::Triangle,2,6)),TEXT("the triangle widens row by row"));
 bool Apart=true;for(int32 I=0;I<8;++I)for(int32 J=I+1;J<8;++J)Apart&=FVector2D::Distance(FForm::Slot(ES47SkyForm::Scatter,I,8,0),FForm::Slot(ES47SkyForm::Scatter,J,8,0))>100;
 Check(Apart,TEXT("the scattered ones keep apart, no two reading as one"));
 FForm Fm;Fm.Begin(nullptr);
 Check(!FForm::Open(9*3600.)&&!FForm::Open(22*3600.)&&FForm::Open(25*3600.)&&!FForm::Open(32*3600.)&&FForm::Open(86400.+22*3600.)&&!FForm::Open(86400.+21*3600.),TEXT("formations keep to the night, and on the first night to after midnight"));
 // Three whole nights at the clock's own pace, nothing else going on.
 TArray<double> Starts;bool EveningQuiet=true,FitsPoints=true,High=true;TSet<int32> Forms;int32 Fast=0,Vanish=0;
 for(double Clock=9*3600.;Clock<3*86400.+7*3600.;Clock+=30)
 {
  Fm.Update(Clock);
  if(!Fm.Due(Clock))continue;
  const double Hour=FMath::Fmod(Clock,86400.)/3600.;
  Fm.StartNatural(Clock,0,0);Starts.Add(Clock);Forms.Add(int32(Fm.Form));Fast+=Fm.bFast?1:0;Vanish+=Fm.bVanish?1:0;FitsPoints&=Fm.Lights>=3&&Fm.Lights<=FForm::MaxLights;
  if(Hour>=12)EveningQuiet&=(Fm.Form==ES47SkyForm::Line||Fm.Form==ES47SkyForm::Scatter)&&!Fm.bFast&&!Fm.bVanish;
  Fm.Age=Fm.Seconds()*.5;for(int32 I=0;I<Fm.Lights;++I)High&=Fm.MapPosition(I).Y>1000&&Fm.MapPosition(I).Y<4000&&FVector2D(Fm.MapPosition(I).X,Fm.MapPosition(I).Z).Size()>1000;
  Fm.bActive=false;
 }
 bool Even=Starts.Num()>=10;for(int32 I=1;I<Starts.Num();++I){const double Gap=Starts[I]-Starts[I-1];Even&=Gap>=74*60.&&(Gap<=111*60.||Gap>6*3600.);}
 Lines.Add(FString::Printf(TEXT("sky visitors: %d formations in three nights, %d kinds, %d fast, %d going out"),Starts.Num(),Forms.Num(),Fast,Vanish));
 Check(Even&&Forms.Num()==5&&Fast>0&&Vanish>0&&FitsPoints,TEXT("formations come at regular intervals through the night, all five kinds, some fast and some going out"));
 Check(EveningQuiet&&High,TEXT("before midnight only a row or a loose handful; always high, never over the watcher's head"));
 FForm Late;Late.Begin(nullptr);Late.Update(25*3600.);const double Asked=Late.NextClock;
 Check(!Late.Due(Asked-1)&&Late.Due(Asked)&&Late.Due(Asked+FForm::LateBy)&&!Late.Update(Asked+FForm::LateBy)&&Late.Update(Asked+FForm::LateBy+1)&&!Late.bActive&&Late.Passes==0,TEXT("a pass missed or kept waiting too long is gone, never made up for"));
 Late.StartNatural(86400.+23*3600.,5,6);Late.Age=30;FForm Back;Back.Begin(S47J::Read(S47J::Write(Late.Save())));
 Check(Back.bActive&&Back.Form==Late.Form&&Back.Passes==1&&Back.NextClock==Late.NextClock&&Back.MapPosition(1).Equals(Late.MapPosition(1),.001),TEXT("a pass in the sky and the schedule survive Continue"));
 Late.bDev=true;Back.Begin(S47J::Read(S47J::Write(Late.Save())));Check(!Back.bActive&&Back.Passes==1,TEXT("a developer's pass is not saved"));
 FForm Gone;Gone.StartDev(ES47SkyForm::V,2,0,0,0,2600,950);Gone.Age=Gone.Seconds()*.6-1;const double LitAt=Gone.Level(0);Gone.Age=Gone.Seconds()*.6+7*.4;
 Check(LitAt>.5&&Gone.Level(0)==0&&Gone.Level(6)<=1&&!Gone.Finished(),TEXT("going out is one light after another, in place"));
 Gone.Age=Gone.Seconds()*.6+7*.4+.4;Check(Gone.Finished(),TEXT("and then the pass is over"));
 FForm Off;Off.StartDev(ES47SkyForm::Line,0,0,0,0,2600,950);Off.LeaveAt=Off.Age;const double Now=Off.Age;Off.Age=Now+1.7;
 Check(Off.Level(0)==0&&Off.Finished(),TEXT("something else starting sends them off within two seconds"));
 FForm Cross;Cross.StartDev(ES47SkyForm::V,0,0,0,0,2600,950);Cross.Age=Cross.Seconds()*.5;
 Check(Cross.MapPosition(0).X>2000&&FMath::Abs(Cross.MapPosition(0).Z)<1,TEXT("a developer's pass crosses in front of whoever asked"));
 Cross=FForm();Cross.StartDev(ES47SkyForm::Circle,1,0,0,0,2600,950);Check(Cross.bFast&&Cross.Lights==8&&Cross.Level(0)==0,TEXT("each formation and pace can be asked for, and it comes up from nothing"));
 for(const TCHAR* Area:{TEXT("saro"),TEXT("road"),TEXT("station01"),TEXT("diner")})
 {const FVector Map(300,1500,-700);Check(MapPoint(WorldPosition(Map,Area),Area).Equals(Map,1e-6),TEXT("the player's place goes back onto the map from any outdoor area"));}
 return Good;
}

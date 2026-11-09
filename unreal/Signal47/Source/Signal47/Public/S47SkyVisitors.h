#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Seen.h"

class US47Main;
class UMaterialBillboardComponent;

// Unreal addition: optional world-space overflights, independent of the comet,
// meteors, star hole and the old three lights along B. No mission or photo quota.
struct FS47SkyVisitorState
{
 bool bEnabled = false;
 int32 Stage = 0;
 double Elapsed = 0, Looking = 0;
 bool Occurred[5] = {}, Observed[5] = {}, Completed[5] = {};
 bool bSecondEarlyPass = false;
 void Begin(const S47J::FObj& Saved, bool bNewNight);
 bool Advance(int32 RequestedStage, double Dt, bool bHeld);
 bool Observe(int32 Visible, double Dt);
 bool OfferEarlyPass();
 S47J::FObj Save() const;
 static double Duration(int32 Stage);
 static int32 Count(int32 Stage);
 static int32 RequiredVisible(int32 Stage);
 static int32 ScheduledStage(double Clock, int32 Chapter, bool bHandoverDone);
 static FVector MapPosition(int32 Stage, int32 Light, double Seconds);
};

// A separate Freeform opportunity: one seeded draw per workday, never a replay
// queue. Absolute session time makes unseen passages and sleep expire naturally.
struct FS47FreeformSkyState
{
 int32 Day=0,Pattern=0,Seed=4701986;
 double Start=0,Elapsed=0,Looking=0;
 bool bChosen=false;
 TArray<FString> ObservedPlaces;
 static constexpr double Duration=240.;
 void Begin(const S47J::FObj& Saved,int32 NewSeed);
 bool Advance(int32 CurrentDay,double Clock);
 bool Active() const { return bChosen&&Elapsed>=0&&Elapsed<Duration; }
 int32 Count() const { return Pattern==2&&Elapsed>100?2:1; }
 FVector Position(int32 Light) const;
 S47J::FObj Save() const;
};

// Tom, 7 November: they come in different formations at regular intervals through the night. High bright points that pass,
// slowly or fast, and now and then go out where they are. Nothing but lights: no hull, no sound, nothing on the ground, and the
// hours and rules for a body are not touched. The schedule is written out, not drawn, so it survives Continue like the low lights'.
enum class ES47SkyForm : uint8 { V, Line, Triangle, Circle, Scatter, Num };
struct FS47SkyFormationState
{
 static constexpr int32 MaxLights=8;
 static constexpr double LateBy=45*60.; // of the clock: still comes this late for something else to finish, then is gone
 double NextClock=-1;
 int32 Passes=0;
 bool bActive=false,bDev=false,bFast=false,bVanish=false;
 ES47SkyForm Form=ES47SkyForm::V;
 int32 Lights=0;
 double Age=0,From=0,LeaveAt=-1,Heading=0,AnchorX=0,AnchorZ=0,Lateral=2400,Height=1700,Looking=0;
 // In a real game's rules (FS47SkyVisitors::FormationTonight): the night these were counted for, and how many it has shown.
 int32 TonightNight=0,TonightShown=0;
 void Begin(const S47J::FObj& Saved);
 S47J::FObj Save() const;
 // True when the schedule moved (something to save). A pass slept through or missed is rescheduled, never made up for.
 bool Update(double Clock);
 bool Due(double Clock) const { return !bActive&&NextClock>=0&&Clock>=NextClock&&Clock<=NextClock+LateBy; }
 void StartNatural(double Clock,double X,double Z);
 void StartDev(ES47SkyForm InForm,int32 Pace,double X,double Z,double InHeading,double InLateral,double InHeight);
 double Seconds() const { return bFast?24.:95.; }
 bool Finished() const;
 // Map metres (S47SkyVisitors::WorldPosition): a pure function of the saved pass and its age, never of the view.
 FVector MapPosition(int32 Light) const;
 // 0 to 1: its fades, its going out, the leaving when something else starts, and its shimmer.
 double Level(int32 Light) const;
 static FVector2D Slot(ES47SkyForm InForm,int32 Light,int32 Count,double Seconds);
 static const TCHAR* Name(ES47SkyForm InForm);
 // From about half past nine, and on the first night only after midnight; over by half past five.
 static bool Open(double Clock);
 static double NextOpening(double Clock);
};

// Tom, 9 November: the lights a little larger, and twinkling as a far plane or a bright star does low in the night sky: quick and
// uneven in strength, and now and then a touch red, green or blue (chromatic scintillation), most near the horizon and little high
// up. Only the sprite's colour and size: nothing is lit, nothing saved, the paths, sight and observation rules are as they were.
struct FS47SkyGlint
{
 // On every light's sprite, as against before 9 November.
 static double SizeScale,Twinkle,Chroma;
 // F10's comparison: drawn as before 9 November (old size, steady) for a few seconds of real time, then by itself as now again.
 static double PlainUntil;
 static void ShowPlain(double Seconds);
 static bool Plain();
 static constexpr double DefaultSize=1.7,DefaultTwinkle=1.,DefaultChroma=1.;
 // 0 to 1: how much air the light is seen through, by its height above the horizontal (radians).
 static double Strength(double Elevation);
 // Gains on red, green and blue (one on average; their sum is the brightness), and the sprite's size factor. A pure function of
 // time, the light's own number and its height: two lights, or two moments, never twinkle alike.
 static FVector Gains(double Time,int32 Light,double Elevation,double* Size=nullptr);
 // -S47SkyGlint=size,twinkle,chroma in a developer's run, to set them from the command line without a build.
 static void ReadCommandLine();
};

class FS47SkyVisitors
{
public:
 explicit FS47SkyVisitors(US47Main* InM);
 ~FS47SkyVisitors();
 void Begin(const S47J::FObj& Saved = nullptr, bool bNewNight = false);
 void Step(double Dt);
 void BeginFreeform(const S47J::FObj& Saved);
 void StopFreeform();
 S47J::FObj SaveFreeform() const;
 S47J::FObj Save() const { return State.Save(); }
 const FS47SkyVisitorState& Data() const { return State; }
 static FVector WorldPosition(const FVector& MapPoint, const FString& Area);
 // Export meshes use a dedicated visibility trace. Transparent glass and hidden
 // area copies do not conceal a light; an opaque roof, wall or mesa does.
 bool ClearLine(const FVector& Eye, const FVector& At) const;
 static bool SelfTest(TArray<FString>& Lines);
 // F10 is an ephemeral visual preview, separate from the CLI natural-state contract.
 // Repeating a stage restarts only the preview; zero returns to the natural sky.
 void DevStage(int32 Stage);
 // F10 too: one formation now, ahead of the player and high enough to read. Pace 0 slow, 1 fast, 2 slow and it goes out.
 // Nothing saved, no observation, the night's own schedule untouched. False: nowhere to show it from here.
 // Lateral and Height (map metres) place it lower or higher in the view: the default is about twenty degrees up.
 bool DevFormation(ES47SkyForm Form,int32 Pace,double Lateral=2600.,double Height=950.);
 void DevFormationStop();
 // The lone high track, the night's 22 percent, now and from where you stand. Only time is asked of it. False: not outdoors in Freeform.
 bool DevHighTrack();
 bool HighTrackOn() const { return FreeformState.Active(); }
 const FS47SkyFormationState& FormationData() const { return Formation; }
 // What is actually drawn: the registered, visible points' places, in three's metres.
 TArray<FVector> ShownPoints() const;
 // The same points' sprite colours as drawn this frame, for F10's measurements.
 TArray<FLinearColor> ShownColors() const;
 void TestFormationAge(double Age) { Formation.Age=Age; }
 void TestFormationQuiet(double Clock) { if(!Formation.bDev)Formation.bActive=false; Formation.NextClock=Clock+20*3600.; HidePoints(); }
 static FVector MapPoint(const FVector& World, const FString& Area);
private:
 US47Main* M = nullptr;
 FS47SkyVisitorState State;
 FS47FreeformSkyState FreeformState;
 bool bFreeform=false;
 void StepFreeform(double Dt);
 FS47SkyVisitorState Preview;
 FString PreviewArea;
 FVector PreviewShift=FVector::ZeroVector; // Freeform: the preview's track, moved to where this player can see it
 FS47SkyFormationState Formation;
 void StepFormation(double Dt);
 bool FormationTonight(double Clock);
 void DrawFormation(double Dt,const FString& Area);
 bool FormationQuiet() const;
 double DevBoost() const;
 // One stage's track on the points. Fade and Count for the observation that follows it in Story.
 void DrawTrack(const FS47SkyVisitorState& Track,const FVector& Shift,double Boost,double& Fade,int32& Count);
 TArray<TWeakObjectPtr<UMaterialBillboardComponent>> Points;
 bool bBuilt = false;
 double ObservationStep = 0;
 int32 TestStage = 0;
 bool bContract = false, bContractDone = false;
 int32 ContractPhase = 0, ContractChecks = 0;
 double ContractTime = 0, ContractSince = 0, HeldTime = 0;
 FString ContractRoot;
 void Build();
 const FS47SkyVisitorState& ActiveState() const { return Preview.bEnabled ? Preview : State; }
 void HidePoints();
 void ClearPreview();
 void Hide();
 int32 RequestedStage() const;
 void RecordObservation(int32 Visible);
 void ControlTest(double Dt);
 bool Check(bool Good, const FString& What);
};

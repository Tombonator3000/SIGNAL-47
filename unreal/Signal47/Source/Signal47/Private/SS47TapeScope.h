#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

// The reel deck's instrument display: the whole tape as a signal trace with its time axis,
// marks and tape head, over a segmented band display that follows playback. Picture only;
// the recording and its evidence are owned by the Freeform session.
class SS47TapeScope : public SLeafWidget
{
public:
 SLATE_BEGIN_ARGS(SS47TapeScope){}
  SLATE_ATTRIBUTE(double,Cursor)
  SLATE_ATTRIBUTE(double,Length)
  SLATE_ATTRIBUTE(double,MarkIn)
  SLATE_ATTRIBUTE(double,MarkOut)
  SLATE_ATTRIBUTE(bool,Control)
  SLATE_ATTRIBUTE(bool,Playing)
  SLATE_ATTRIBUTE(TArray<double>,Ticks)
  SLATE_ATTRIBUTE(bool,CanMark)
  SLATE_ARGUMENT(TFunction<void(double)>,OnMark)
  // D45-20261110 (Tom: every take looked the same; a recording should be seen on the receiver too). Seed: the take's own look
  // (the same tape always draws the same trace, another take another one). Recording: the pen writes the trace up to the cursor
  // and nothing ahead of it. Retained: the moment it turns true the band display runs once. Period: the source's regular pulse
  // (0 none), drawn where it is heard. Phosphor: the receiver's green glass instead of the deck's olive. Caption: the line over it.
  SLATE_ATTRIBUTE(uint32,Seed)
  SLATE_ATTRIBUTE(bool,Recording)
  SLATE_ATTRIBUTE(bool,Retained)
  SLATE_ATTRIBUTE(double,Period)
  SLATE_ATTRIBUTE(FString,Caption)
  SLATE_ARGUMENT(bool,Phosphor)
 SLATE_END_ARGS()
 void Construct(const FArguments& Args);
 virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(640,430);}
 // D45-20261110: what one take looks like on tape, made once from its seed (pure: the self-test compares two of them).
 struct FTakeLook
 {
  uint32 Key=0;float Noise=.08f,Grain=.5f,HumHz=.4f,HumAmp=.03f,HumPhase=0,PulseH=.2f,PulseW=.04f,BandMid=3,BandWide=1.9f;
  TArray<FVector2f> Drops;TArray<float> TickH;
 };
 static FTakeLook MakeLook(uint32 InSeed,int32 TickCount);
 static bool SelfTest(TArray<FString>& Lines);
 virtual int32 OnPaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Cull,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled) const override;
 virtual FReply OnMouseButtonDown(const FGeometry& G,const FPointerEvent& E) override;
 virtual FReply OnMouseMove(const FGeometry& G,const FPointerEvent& E) override;
 virtual void OnMouseLeave(const FPointerEvent& E) override;
 virtual FCursorReply OnCursorQuery(const FGeometry& G,const FPointerEvent& E) const override;
 // Where a tape second lies inside a widget of this size (the trace's own scale).
 FVector2f LocalAt(double Seconds,const FVector2f& Size) const;
private:
 bool TimeAt(const FGeometry& G,const FVector2D& Screen,double& Seconds) const;
 double Hover=-1;
 TAttribute<bool> CanMark;
 TFunction<void(double)> OnMark;
 TAttribute<double> Cursor,Length,MarkIn,MarkOut;
 TAttribute<bool> Control,Playing;
 TAttribute<TArray<double>> Ticks;
 // D45-20261110: the take's look, the band display's live state (peak-hold dots, the pass of this playback) and the retain run
 TAttribute<uint32> Seed;TAttribute<bool> Recording,Retained;TAttribute<double> Period;TAttribute<FString> Caption;bool bPhosphor=false;
 mutable FTakeLook Look;mutable bool bLook=false,bWasRun=false,bWasRetained=false,bPrimed=false;
 mutable float PeakHold[7]={0,0,0,0,0,0,0};mutable double LastPaint=0,RetainAt=-10;mutable uint32 Pass=0;
};

// FIX-20261110 (Tom: the reel needs more effects and animation). The deck's transport as a picture:
// two reels whose tape packs follow the cursor, the tape over the head, two needles and the counter.
// A rewind is seen: the picture winds to the new position, it does not jump. Picture only.
class SS47TapeReels : public SLeafWidget
{
public:
 SLATE_BEGIN_ARGS(SS47TapeReels){}
  SLATE_ATTRIBUTE(double,Cursor)
  SLATE_ATTRIBUTE(double,Length)
  SLATE_ATTRIBUTE(bool,Playing)
  SLATE_ATTRIBUTE(bool,Loading)
  SLATE_ATTRIBUTE(bool,Control)
  SLATE_ATTRIBUTE(TArray<double>,Ticks)
  // D45-20261110: as SS47TapeScope (Seed, Recording: the REC lamp and the tape being written, Retained, Period, Phosphor), the
  // take's label on the transport (Caption) and the memory marks shown on the counter's window (MEMO IN / OUT, -1 none).
  SLATE_ATTRIBUTE(uint32,Seed)
  SLATE_ATTRIBUTE(bool,Recording)
  SLATE_ATTRIBUTE(bool,Retained)
  SLATE_ATTRIBUTE(double,Period)
  SLATE_ATTRIBUTE(FString,Caption)
  SLATE_ATTRIBUTE(double,MarkIn)
  SLATE_ATTRIBUTE(double,MarkOut)
  SLATE_ARGUMENT(bool,Phosphor)
 SLATE_END_ARGS()
 void Construct(const FArguments& Args);
 virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(420,168);}
 virtual int32 OnPaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Cull,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled) const override;
private:
 TAttribute<double> Cursor,Length;
 TAttribute<bool> Playing,Loading,Control;
 TAttribute<TArray<double>> Ticks;
 // what the picture shows (winds toward the real cursor), the reels' angles, the needles and their peak lamps
 mutable double Shown=-1,Last=0,AngleL=0,AngleR=0,Needle[2]={0,0},Peak[2]={0,0},PeakAt[2]={-10,-10},HeadGlow=0;
 // D45-20261110: see the arguments; ThreadAt: a tape change threads the new tape in; RetainAt: the counter's RETAINED blink
 TAttribute<uint32> Seed;TAttribute<bool> Recording,Retained;TAttribute<double> Period,MarkIn,MarkOut;TAttribute<FString> Caption;bool bPhosphor=false;
 mutable double ThreadAt=-10,RetainAt=-10;mutable bool bWasControl=false,bWasRetained=false,bPrimed=false;
};

// The reel deck panel fills the view away from the workstation glass, so nothing around it can
// be clicked: it holds the keyboard itself and leaves on the same keys as the workstation screens.
class SS47TapePanel : public SCompoundWidget
{
public:
 SLATE_BEGIN_ARGS(SS47TapePanel){}
  SLATE_DEFAULT_SLOT(FArguments,Content)
  SLATE_ARGUMENT(TFunction<void()>,OnLeave)
 SLATE_END_ARGS()
 void Construct(const FArguments& Args);
 virtual bool SupportsKeyboardFocus() const override{return true;}
 virtual void Tick(const FGeometry& G,const double Time,const float Dt) override;
 virtual FReply OnKeyDown(const FGeometry& G,const FKeyEvent& E) override;
private:
 TFunction<void()> OnLeave;
};

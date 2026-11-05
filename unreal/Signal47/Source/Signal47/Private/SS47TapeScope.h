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
 SLATE_END_ARGS()
 void Construct(const FArguments& Args);
 virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(640,430);}
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

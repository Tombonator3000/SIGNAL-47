#pragma once
#include "CoreMinimal.h"
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
 SLATE_END_ARGS()
 void Construct(const FArguments& Args);
 virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(640,430);}
 virtual int32 OnPaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Cull,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled) const override;
private:
 TAttribute<double> Cursor,Length,MarkIn,MarkOut;
 TAttribute<bool> Control,Playing;
 TAttribute<TArray<double>> Ticks;
};

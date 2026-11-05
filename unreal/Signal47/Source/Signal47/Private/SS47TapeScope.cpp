#include "SS47TapeScope.h"
#include "SS47Widgets.h"
#include "S47Keys.h"
#include "S47Theme.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
 const FLinearColor Trace(.47f,.95f,.55f),Dim(.13f,.34f,.17f),Grid(.05f,.15f,.08f),Amber(1.f,.66f,.2f),Ghost(.03f,.016f,.005f);
 double Hash(double N){const double V=FMath::Sin(N*12.9898)*43758.5453;return V-FMath::FloorToDouble(V);}
 const float TraceLeft=34,TraceRightGap=22,TraceTop=26;
 float TraceHeight(float SizeY){return SizeY*.5f;}
}
void SS47TapeScope::Construct(const FArguments& A)
{
 Cursor=A._Cursor;Length=A._Length;MarkIn=A._MarkIn;MarkOut=A._MarkOut;Control=A._Control;Playing=A._Playing;Ticks=A._Ticks;CanMark=A._CanMark;OnMark=A._OnMark;
 SetCanTick(false);ForceVolatile(true);
}
int32 SS47TapeScope::OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const
{
 const FVector2f S(G.GetLocalSize());if(S.X<50||S.Y<50)return Layer;
 const FSlateBrush* White=FCoreStyle::Get().GetBrush("WhiteBrush");
 auto Box=[&](float X,float Y,float W,float H,const FLinearColor& C){FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2f(W,H),FSlateLayoutTransform(FVector2f(X,Y))),White,ESlateDrawEffect::None,C);};
 auto Line=[&](const TArray<FVector2f>& P,const FLinearColor& C,float Thick){FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,Thick);};
 auto Label=[&](const FString& T,float X,float Y,const FLinearColor& C,int32 Size=15){FSlateDrawElement::MakeText(Out,Layer,G.ToPaintGeometry(FVector2f(200,24),FSlateLayoutTransform(FVector2f(X,Y))),T,S47Style::Mono(Size),ESlateDrawEffect::None,C);};
 const double L=FMath::Max(1.,Length.Get(18.)),At=FMath::Clamp(Cursor.Get(0.),0.,L);const TArray<double> Marks=Ticks.Get(TArray<double>());const bool Run=Playing.Get(false);
 // Trace: the whole tape, left to right.
 const float Left=TraceLeft,Right=S.X-TraceRightGap,Top=TraceTop,TraceH=TraceHeight(S.Y),Base=Top+TraceH*.78f,W=Right-Left;
 auto Px=[&](double T){return Left+float(T/L)*W;};
 Label(Control.Get(false)?TEXT("NOW LOADED: CONTROL / THE STATION'S OWN REFERENCE"):TEXT("NOW LOADED: ORIGINAL / WHAT THE ANTENNAS RECORDED"),Left,0,Trace,20);
 if(CanMark.Get(false))Label(TEXT("CLICK TO MARK"),Right-150,0,Amber,20);
 const double In=MarkIn.Get(-1.),OutMark=MarkOut.Get(-1.);
 if(In>=0)Box(Px(In),Top,FMath::Max(2.f,(OutMark>In?Px(OutMark):Px(At))-Px(In)),TraceH,FLinearColor(.45f,.27f,.05f,.45f));
 for(int32 I=0;I<=int32(L);I+=2)
 {
  Line({FVector2f(Px(I),Top),FVector2f(Px(I),Top+TraceH)},Grid,1);
  Label(FString::FromInt(I),Px(I)-5,Top+TraceH+2,Trace,17);
 }
 Line({FVector2f(Left,Base),FVector2f(Right,Base)},Grid,1);
 const int32 N=FMath::Max(2,int32(W/2));TArray<FVector2f> Heard,Ahead;Heard.Reserve(N);Ahead.Reserve(N);
 for(int32 I=0;I<=N;++I)
 {
  const double T=L*I/N;double Rise=(Hash(I+L*7)-.5)*TraceH*.09+(Hash(I*.37+3)-.5)*TraceH*.05;
  for(double Tick:Marks){const double D=FMath::Abs(T-Tick);if(D<.09)Rise+=TraceH*.62*(1-D/.09);}
  (T<=At?Heard:Ahead).Add(FVector2f(Left+W*I/N,Base-float(Rise)));
 }
 if(Heard.Num())Ahead.Insert(Heard.Last(),0);
 if(Ahead.Num()>1)Line(Ahead,Dim,1);if(Heard.Num()>1)Line(Heard,Trace,1.4f);
 Line({FVector2f(Px(At),Top-3),FVector2f(Px(At),Top+TraceH+3)},Amber,2);
 Label(TEXT("s"),Right+1,Top+TraceH+2,Trace,17);
 if(Hover>=0&&CanMark.Get(false))
 {
  Line({FVector2f(Px(Hover),Top),FVector2f(Px(Hover),Top+TraceH)},Amber*FLinearColor(1,1,1,.55f),1);
  Label(FString::Printf(TEXT("%.1f s"),Hover),FMath::Min(Px(Hover)+6,Right-70),Top+4,Amber,17);
 }
 // Bands: follow the tape while it plays; a passing pulse lifts the middle of the band.
 static const TCHAR* Bands[]={TEXT("125"),TEXT("250"),TEXT("500"),TEXT("1k"),TEXT("2k"),TEXT("4k"),TEXT("8k")};
 const int32 Count=UE_ARRAY_COUNT(Bands),Segments=11;const float BarTop=Top+TraceH+30,BarBottom=S.Y-26,SegH=(BarBottom-BarTop)/Segments,Slot=W/Count;
 double Pulse=0;if(Run)for(double Tick:Marks){const double D=At-Tick;if(D>=0&&D<.35)Pulse=FMath::Max(Pulse,1-D/.35);}
 const int32 Step=FMath::FloorToInt32(At*14);
 for(int32 B=0;B<Count;++B)
 {
  const double Shape=FMath::Exp(-FMath::Square((B-3)/1.9));
  const double Level=Run?FMath::Clamp(Shape*(.28+.26*Hash(Step*.71+B*3.1))+Pulse*.62*Shape,0.,1.):0.;
  const int32 Lit=FMath::RoundToInt32(Level*Segments);const float X=Left+Slot*B+Slot*.17f,BarW=Slot*.66f;
  for(int32 I=0;I<Segments;++I)Box(X,BarBottom-SegH*(I+1)+1,BarW,FMath::Max(2.f,SegH-3),I<Lit?Amber:Ghost);
  Label(Bands[B],X+BarW*.5f-14,BarBottom+1,Trace,17);
 }
 return Layer;
}
FVector2f SS47TapeScope::LocalAt(double Seconds,const FVector2f& Size) const
{
 const float W=Size.X-TraceRightGap-TraceLeft;
 return FVector2f(TraceLeft+float(Seconds/FMath::Max(1.,Length.Get(18.)))*W,TraceTop+TraceHeight(Size.Y)*.5f);
}
bool SS47TapeScope::TimeAt(const FGeometry& G,const FVector2D& Screen,double& Seconds) const
{
 const FVector2f S(G.GetLocalSize());const FVector2f P(G.AbsoluteToLocal(Screen));
 const float W=S.X-TraceRightGap-TraceLeft;
 // The numbers under the trace count as the trace; the band display below does not.
 if(W<=0||P.Y<TraceTop-3||P.Y>TraceTop+TraceHeight(S.Y)+24)return false;
 Seconds=FMath::Clamp(double(P.X-TraceLeft)/W,0.,1.)*FMath::Max(1.,Length.Get(18.));
 return true;
}
FReply SS47TapeScope::OnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{
 double At=0;
 if(E.GetEffectingButton()!=EKeys::LeftMouseButton||!CanMark.Get(false)||!OnMark||!TimeAt(G,E.GetScreenSpacePosition(),At))return FReply::Unhandled();
 OnMark(At);
 return FReply::Handled();
}
FReply SS47TapeScope::OnMouseMove(const FGeometry& G,const FPointerEvent& E)
{
 double At=0;Hover=TimeAt(G,E.GetScreenSpacePosition(),At)?At:-1;
 return FReply::Unhandled();
}
void SS47TapeScope::OnMouseLeave(const FPointerEvent& E){SLeafWidget::OnMouseLeave(E);Hover=-1;}
FCursorReply SS47TapeScope::OnCursorQuery(const FGeometry& G,const FPointerEvent& E) const
{
 double At=0;
 return CanMark.Get(false)&&TimeAt(G,E.GetScreenSpacePosition(),At)?FCursorReply::Cursor(EMouseCursor::Hand):FCursorReply::Unhandled();
}
void SS47TapePanel::Construct(const FArguments& A)
{
 OnLeave=A._OnLeave;
 ChildSlot[A._Content.Widget];
}
void SS47TapePanel::Tick(const FGeometry& G,const double Time,const float Dt)
{
 SCompoundWidget::Tick(G,Time,Dt);
 S47Theme::KeepFocus(SharedThis(this));
}
FReply SS47TapePanel::OnKeyDown(const FGeometry&,const FKeyEvent& E)
{
 const FKey K=E.GetKey();
 // A held use key is what opened the panel: only a fresh press closes it.
 const bool Use=!E.IsRepeat()&&!K.IsGamepadKey()&&S47Keys::Is(TEXT("use"),K)&&K!=EKeys::Enter&&K!=EKeys::SpaceBar;
 if(K!=EKeys::Escape&&K!=EKeys::Gamepad_FaceButton_Right&&!Use)return FReply::Unhandled();
 if(OnLeave)OnLeave();
 return FReply::Handled();
}

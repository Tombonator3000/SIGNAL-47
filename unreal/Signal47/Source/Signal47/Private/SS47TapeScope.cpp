#include "SS47TapeScope.h"
#include "SS47Widgets.h"
#include "S47Keys.h"
#include "S47Kit.h"
#include "S47Theme.h"
#include "Math/RandomStream.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
 // Sits on the reel deck's dark olive panel; signal stays a muted green, labels are cream.
 // D45-20261110 (Tom: a 1980s high-tech deck, and a recording seen on the receiver): one palette per widget, the deck's olive or
 // the receiver's green glass (Phosphor), with black display windows, the tape's own colours and the REC red.
 struct FPal{FLinearColor Trace,Dim,Grid,Amber,Ghost,Cream,Window,Rec,Tape,TapeEdge,Oxide;};
 const FPal& Pal(bool bPhosphor)
 {
  static const FPal Reel{FLinearColor(.38f,.69f,.28f),FLinearColor(.13f,.2f,.09f),FLinearColor(.07f,.08f,.04f),FLinearColor(.87f,.43f,.05f),FLinearColor(.05f,.055f,.028f),FLinearColor(.87f,.8f,.62f),
   S47Kit::Css(0x0b0c08),S47Kit::Css(0xff5a44),FLinearColor(.16f,.10f,.05f),FLinearColor(.34f,.22f,.10f),FLinearColor(.42f,.27f,.12f)};
  static const FPal Glass{S47Kit::Css(0x8cffa4),S47Kit::Css(0x1d4a2b),S47Kit::Css(0x0c2a16),S47Kit::Css(0xffb84a),S47Kit::Css(0x0a1f10),S47Kit::Css(0x8cffa4,.85f),
   S47Kit::Css(0x010603),S47Kit::Css(0xff5a44),S47Kit::Css(0x0c2a16),S47Kit::Css(0x3f9a5a),S47Kit::Css(0x3f9a5a)};
  return bPhosphor?Glass:Reel;
 }
 FLinearColor Lift(const FLinearColor& C,float K){return FLinearColor(C.R*K,C.G*K,C.B*K,C.A);}
 FLinearColor Fade(const FLinearColor& C,float A){return FLinearColor(C.R,C.G,C.B,C.A*A);}
 double Hash(double N){const double V=FMath::Sin(N*12.9898)*43758.5453;return V-FMath::FloorToDouble(V);}
 // D45-20261110: a black display window let into the faceplate: a thin cream bevel and the glass's own reflection along the top.
 void DrawWindow(FSlateWindowElementList& Out,int32 Layer,const FGeometry& G,const FPal& Pc,float X,float Y,float W,float H)
 {
  const FSlateBrush* White=FCoreStyle::Get().GetBrush("WhiteBrush");
  FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2f(W,H),FSlateLayoutTransform(FVector2f(X,Y))),White,ESlateDrawEffect::None,Pc.Window);
  const TArray<FVector2f> Edge={FVector2f(X,Y),FVector2f(X+W,Y),FVector2f(X+W,Y+H),FVector2f(X,Y+H),FVector2f(X,Y)};
  FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),Edge,ESlateDrawEffect::None,Fade(Pc.Cream,.35f),true,1);
  FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2f(FMath::Max(1.f,W-6),2),FSlateLayoutTransform(FVector2f(X+3,Y+2))),White,ESlateDrawEffect::None,FLinearColor(1,1,1,.05f));
 }
 // Below this height the display is the receiver's strip: the trace alone, no band display, small labels.
 constexpr float StripBelow=220;
 const float TraceLeft=34;
 float TraceTopAt(float SizeY){return SizeY<StripBelow?24.f:38.f;}
 float TraceHeight(float SizeY){return SizeY<StripBelow?FMath::Max(10.f,SizeY-46):FMath::Max(10.f,SizeY-64);}
 // D45-20261110: on the deck the band display stands beside the trace, as on the decks of the time; the strip has the width to itself.
 float TraceRightAt(const FVector2f& Size){return Size.Y<StripBelow?Size.X-14:Size.X*.74f-22;}
}
SS47TapeScope::FTakeLook SS47TapeScope::MakeLook(uint32 InSeed,int32 TickCount)
{
 // D45-20261110: hiss, hum and grain of this tape, the height of the regular pulse, a dropout or two, how the bands sit and how
 // tall each extra tick stands (always above the pulse: the reading must stay plain).
 FRandomStream R(int32(InSeed&0x7fffffffu));FTakeLook Made;Made.Key=InSeed;
 Made.Noise=float(R.FRandRange(.05,.12));Made.Grain=float(R.FRandRange(.2,.9));
 Made.HumHz=float(R.FRandRange(.15,.9));Made.HumAmp=float(R.FRandRange(.015,.06));Made.HumPhase=float(R.FRandRange(0.,2.*UE_DOUBLE_PI));
 Made.PulseH=float(R.FRandRange(.16,.30));Made.PulseW=float(R.FRandRange(.03,.06));
 Made.BandMid=float(R.FRandRange(2.2,3.9));Made.BandWide=float(R.FRandRange(1.4,2.4));
 for(int32 D=R.RandRange(0,2);D>0;--D){const float From=float(R.FRandRange(.5,17.));Made.Drops.Add(FVector2f(From,From+float(R.FRandRange(.08,.3))));}
 for(int32 K=0;K<FMath::Max(1,TickCount);++K)Made.TickH.Add(float(R.FRandRange(.48,.74)));
 return Made;
}
bool SS47TapeScope::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;auto Check=[&](bool bPass,const TCHAR* Name){OK&=bPass;Lines.Add(FString(bPass?TEXT("PASS "):TEXT("FAIL "))+Name);};
 auto Same=[](const FTakeLook& X,const FTakeLook& Y){return X.Key==Y.Key&&X.Noise==Y.Noise&&X.HumHz==Y.HumHz&&X.HumPhase==Y.HumPhase&&X.BandMid==Y.BandMid&&X.PulseH==Y.PulseH&&X.TickH==Y.TickH&&X.Drops==Y.Drops;};
 const FTakeLook LookA=MakeLook(4711,11),LookAgain=MakeLook(4711,11),LookB=MakeLook(4712,11);
 Check(Same(LookA,LookAgain)&&!Same(LookA,LookB)&&LookA.TickH.Num()==11,TEXT("D45 reel: the same tape always draws the same trace, another take another one"));
 bool Clear=true;
 for(uint32 K=0;K<64;++K){const FTakeLook Each=MakeLook(K*2654435761u,11);for(const float H:Each.TickH)Clear&=H>=.48f&&H<=.74f&&H>Each.PulseH+.15f;Clear&=Each.Noise<=.12f&&Each.PulseH<=.30f&&Each.Drops.Num()<=2;}
 Check(Clear,TEXT("D45 reel: an extra tick always stands well above the pulsar's own pulse and the hiss"));
 return OK;
}
void SS47TapeScope::Construct(const FArguments& A)
{
 Cursor=A._Cursor;Length=A._Length;MarkIn=A._MarkIn;MarkOut=A._MarkOut;Control=A._Control;Playing=A._Playing;Ticks=A._Ticks;CanMark=A._CanMark;OnMark=A._OnMark;
 Seed=A._Seed;Recording=A._Recording;Retained=A._Retained;Period=A._Period;Caption=A._Caption;bPhosphor=A._Phosphor;
 // D45-20261110: each display starts its live parts somewhere of its own, so two playbacks never breathe alike
 Pass=FPlatformTime::Cycles();
 SetCanTick(false);ForceVolatile(true);
}
int32 SS47TapeScope::OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const
{
 const FVector2f S(G.GetLocalSize());if(S.X<50||S.Y<50)return Layer;
 const FPal& Pc=Pal(bPhosphor);
 const FSlateBrush* White=FCoreStyle::Get().GetBrush("WhiteBrush");
 auto Box=[&](float X,float Y,float W,float H,const FLinearColor& C){FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2f(W,H),FSlateLayoutTransform(FVector2f(X,Y))),White,ESlateDrawEffect::None,C);};
 auto Line=[&](const TArray<FVector2f>& P,const FLinearColor& C,float Thick){FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,Thick);};
 auto Label=[&](const FString& T,float X,float Y,const FLinearColor& C,int32 Size=15){FSlateDrawElement::MakeText(Out,Layer,G.ToPaintGeometry(FVector2f(400,24),FSlateLayoutTransform(FVector2f(X,Y))),T,S47Style::Mono(Size),ESlateDrawEffect::None,C);};
 const double L=FMath::Max(1.,Length.Get(18.)),At=FMath::Clamp(Cursor.Get(0.),0.,L);const TArray<double> Marks=Ticks.Get(TArray<double>());const bool Run=Playing.Get(false);
 // D45-20261110: the take's own look (made again only when the tape changes); a new playback is a new pass for what is live.
 const bool Rec=Recording.Get(false),Strip=S.Y<StripBelow;const double Every=Period.Get(0.);
 const uint32 Want=Seed.Get(0u);if(!bLook||Look.Key!=Want||Look.TickH.Num()!=FMath::Max(1,Marks.Num())){Look=MakeLook(Want,Marks.Num());bLook=true;}
 const double Now=FPlatformTime::Seconds(),Dt=FMath::Clamp(Now-LastPaint,0.,.1);LastPaint=Now;
 if(Run&&!bWasRun)++Pass;bWasRun=Run;
 const bool bKept=Retained.Get(false);if(bPrimed&&bKept&&!bWasRetained)RetainAt=Now;bWasRetained=bKept;bPrimed=true;
 const double SF=double(Look.Key%9973u);
 // Trace: the whole tape, left to right, in its black window.
 const float Left=TraceLeft,Right=TraceRightAt(S),Top=TraceTopAt(S.Y),TraceH=TraceHeight(S.Y),Base=Top+TraceH*.78f,W=Right-Left;
 auto Px=[&](double T){return Left+float(T/L)*W;};
 const int32 HeadSize=Strip?15:20,AxisSize=Strip?13:17;
 const FString Head=Caption.Get(FString());
 Label(!Head.IsEmpty()?Head:Control.Get(false)?FString(TEXT("CONTROL / THE STATION'S OWN REFERENCE, ANTENNAS OUT")):FString(TEXT("ORIGINAL / WHAT THE ANTENNAS RECORDED")),Left-26,0,Pc.Cream,HeadSize);
 if(CanMark.Get(false))Label(TEXT("CLICK TO MARK"),Right-150,0,Pc.Amber,HeadSize);
 DrawWindow(Out,Layer,G,Pc,Left-26,Top-12,W+40,TraceH+12+AxisSize+8);
 const double In=MarkIn.Get(-1.),OutMark=MarkOut.Get(-1.);
 if(In>=0)Box(Px(In),Top,FMath::Max(2.f,(OutMark>In?Px(OutMark):Px(At))-Px(In)),TraceH,FLinearColor(.45f,.27f,.05f,.45f));
 const int32 Every2=L>24?5:2;
 for(int32 I=0;I<=int32(L);I+=Every2)
 {
  Line({FVector2f(Px(I),Top),FVector2f(Px(I),Top+TraceH)},Pc.Grid,1);
  Label(FString::FromInt(I),Px(I)-5,Top+TraceH+2,Pc.Cream,AxisSize);
 }
 Line({FVector2f(Left,Base),FVector2f(Right,Base)},Pc.Grid,1);
 const int32 N=FMath::Max(2,int32(W/2));Heard.Reset();Ahead.Reset();Heard.Reserve(N);Ahead.Reserve(N);
 for(int32 I=0;I<=N;++I)
 {
  const double T=L*I/N;
  // D45-20261110: hiss, grain and hum of this take (a dropout takes the hiss, never a pulse), the source's regular pulse where it
  // is heard, and the extra ticks standing above it.
  double Rise=(Hash(FMath::FloorToDouble(T*55)+SF*.37)-.5)*TraceH*Look.Noise*2+(Hash(T*Look.Grain*31+SF*.13)-.5)*TraceH*.04+FMath::Sin(2*UE_DOUBLE_PI*Look.HumHz*T+Look.HumPhase)*TraceH*Look.HumAmp;
  for(const FVector2f& Dr:Look.Drops)if(T>=Dr.X&&T<=Dr.Y)Rise*=.15;
  if(Every>.2)for(double P=Every;P<L;P+=Every){const double D=FMath::Abs(T-P);if(D<Look.PulseW)Rise+=TraceH*Look.PulseH*(1-D/Look.PulseW);}
  for(int32 K=0;K<Marks.Num();++K){const double D=FMath::Abs(T-Marks[K]);if(D<.09)Rise+=TraceH*Look.TickH[K%Look.TickH.Num()]*(1-D/.09);}
  (T<=At?Heard:Ahead).Add(FVector2f(Left+W*I/N,Base-float(Rise)));
 }
 // Recording: the pen has written up to the cursor and nothing lies ahead of it yet.
 if(!Rec){if(Heard.Num())Ahead.Insert(Heard.Last(),0);if(Ahead.Num()>1)Line(Ahead,Pc.Dim,1);}
 if(Heard.Num()>1)Line(Heard,Pc.Trace,1.4f);
 if(Rec&&Heard.Num()){const FVector2f Pen=Heard.Last();Box(Pen.X-6,Pen.Y-6,12,12,Fade(Pc.Rec,.22f));Box(Pen.X-1.5f,Pen.Y-1.5f,3,3,Pc.Trace);}
 Line({FVector2f(Px(At),Top-3),FVector2f(Px(At),Top+TraceH+3)},Rec?Pc.Rec:Pc.Amber,2);
 Label(TEXT("s"),Right+1,Top+TraceH+2,Pc.Cream,AxisSize);
 if(Hover>=0&&CanMark.Get(false))
 {
  Line({FVector2f(Px(Hover),Top),FVector2f(Px(Hover),Top+TraceH)},Pc.Amber*FLinearColor(1,1,1,.55f),1);
  Label(FString::Printf(TEXT("%.1f s"),Hover),FMath::Min(Px(Hover)+6,Right-70),Top+4,Pc.Amber,17);
 }
 if(Strip)return Layer;
 // D45-20261110: the band display, an LED analyzer beside the trace with a peak-hold dot per band that falls back slowly. It follows
 // the tape while it plays: a passing extra tick lifts it hard, the regular pulse a little. RETAIN runs it once, in this take's order.
 static const TCHAR* Bands[]={TEXT("125"),TEXT("250"),TEXT("500"),TEXT("1k"),TEXT("2k"),TEXT("4k"),TEXT("8k")};
 const int32 Count=UE_ARRAY_COUNT(Bands),Segments=12;
 const float AX=Right+26,AW=S.X-2-AX;if(AW<80)return Layer;
 Label(TEXT("LEVEL"),AX+6,0,Pc.Cream,HeadSize);
 DrawWindow(Out,Layer,G,Pc,AX,Top-12,AW,S.Y-Top+10);
 const float BarTop=Top,BarBottom=S.Y-24,SegH=(BarBottom-BarTop)/Segments,Slot=(AW-12)/Count;
 double Hit=0;if(Run)for(double Tick:Marks){const double D=At-Tick;if(D>=0&&D<.35)Hit=FMath::Max(Hit,1-D/.35);}
 double Beat=0;if(Run&&Every>.2&&At>=Every){const double Since=At-FMath::FloorToDouble(At/Every)*Every;if(Since<.3)Beat=1-Since/.3;}
 const int32 Step=FMath::FloorToInt32(At*14);const double Sweep=Now-RetainAt;const uint32 Order=Look.Key%3u;
 for(int32 B=0;B<Count;++B)
 {
  const double Shape=FMath::Exp(-FMath::Square((B-Look.BandMid)/Look.BandWide));
  double Level=Run?FMath::Clamp(Shape*(.24+.26*Hash(Step*.71+B*3.1+SF+double(Pass%997u)*17.3))+.06*FMath::Sin(Now*(.7+.3*B)+Look.HumPhase+B)+Hit*.62*Shape+Beat*.35*Shape,0.,1.):0.;
  if(Sweep>=0&&Sweep<.8)
  {
   const double Delay=.06*(Order==0?B:Order==1?FMath::Abs(B-3)*2:FMath::FloorToDouble(Hash(B*7.7+SF)*6.99));
   Level=FMath::Max(Level,FMath::Sin(UE_DOUBLE_PI*FMath::Clamp((Sweep-Delay)/.4,0.,1.)));
  }
  PeakHold[B]=FMath::Max(float(Level),PeakHold[B]-float(Dt)*6.f/Segments);
  const int32 Lit=FMath::RoundToInt32(Level*Segments),Held=FMath::Clamp(FMath::RoundToInt32(PeakHold[B]*Segments)-1,0,Segments-1);
  const float X=AX+6+Slot*B+Slot*.15f,BarW=Slot*.7f;
  for(int32 I=0;I<Segments;++I){const FLinearColor Seg=I>=Segments-2?Pc.Rec:I>=Segments-5?Pc.Amber:Pc.Trace;Box(X,BarBottom-SegH*(I+1)+1,BarW,FMath::Max(2.f,SegH-3),I<Lit?Seg:Pc.Ghost);}
  if(PeakHold[B]>.05f)Box(X,BarBottom-SegH*(Held+1)+1,BarW,FMath::Max(2.f,SegH-3),Held>=Segments-2?Pc.Rec:Pc.Amber);
  Label(Bands[B],X+BarW*.5f-10,BarBottom+3,Pc.Cream,15);
 }
 return Layer;
}
FVector2f SS47TapeScope::LocalAt(double Seconds,const FVector2f& Size) const
{
 const float W=TraceRightAt(Size)-TraceLeft;
 return FVector2f(TraceLeft+float(Seconds/FMath::Max(1.,Length.Get(18.)))*W,TraceTopAt(Size.Y)+TraceHeight(Size.Y)*.5f);
}
bool SS47TapeScope::TimeAt(const FGeometry& G,const FVector2D& Screen,double& Seconds) const
{
 const FVector2f S(G.GetLocalSize());const FVector2f P(G.AbsoluteToLocal(Screen));
 const float Right=TraceRightAt(S),W=Right-TraceLeft,Top=TraceTopAt(S.Y);
 // The numbers under the trace count as the trace; the band display beside it does not.
 if(W<=0||P.Y<Top-3||P.Y>Top+TraceHeight(S.Y)+24||P.X>Right+12)return false;
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
void SS47TapeReels::Construct(const FArguments& A)
{
 Cursor=A._Cursor;Length=A._Length;Playing=A._Playing;Loading=A._Loading;Control=A._Control;Ticks=A._Ticks;
 Seed=A._Seed;Recording=A._Recording;Retained=A._Retained;Period=A._Period;Caption=A._Caption;MarkIn=A._MarkIn;MarkOut=A._MarkOut;bPhosphor=A._Phosphor;
 SetCanTick(false);ForceVolatile(true);SetVisibility(EVisibility::HitTestInvisible);
}
int32 SS47TapeReels::OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const
{
 const FVector2f S(G.GetLocalSize());if(S.X<200||S.Y<90)return Layer;
 const FPal& Pc=Pal(bPhosphor);
 const FSlateBrush* White=FCoreStyle::Get().GetBrush("WhiteBrush");
 auto Box=[&](float X,float Y,float W,float H,const FLinearColor& C){FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2f(W,H),FSlateLayoutTransform(FVector2f(X,Y))),White,ESlateDrawEffect::None,C);};
 auto Line=[&](const TArray<FVector2f>& P,const FLinearColor& C,float Thick){FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,Thick);};
 auto Label=[&](const FString& T,float X,float Y,const FLinearColor& C,int32 Size=15){FSlateDrawElement::MakeText(Out,Layer,G.ToPaintGeometry(FVector2f(320,26),FSlateLayoutTransform(FVector2f(X,Y))),T,S47Style::Mono(Size),ESlateDrawEffect::None,C);};
 auto Ring=[&](const FVector2f& C,float R,const FLinearColor& Col,float Thick)
 {
  const int32 N=FMath::Clamp(int32(R*.9f),14,48);RingPts.Reset();
  for(int32 I=0;I<=N;++I){const float A=2*PI*I/N;RingPts.Add(C+FVector2f(FMath::Cos(A),FMath::Sin(A))*R);}
  Line(RingPts,Col,Thick);
 };
 const double Now=FPlatformTime::Seconds(),Dt=FMath::Clamp(Now-Last,0.,.1);Last=Now;
 const double L=FMath::Max(1.,Length.Get(18.)),At=FMath::Clamp(Cursor.Get(0.),0.,L);const bool Run=Playing.Get(false),Load=Loading.Get(false);
 // D45-20261110: recording (REC), the take's seed, a tape change threaded in, the RETAINED blink; Small: the receiver's strip.
 const bool Rec=Recording.Get(false),Ctl=Control.Get(false),Small=S.Y<150;const uint32 Key=Seed.Get(0u);const double Every=Period.Get(0.);
 if(bPrimed&&Ctl!=bWasControl)ThreadAt=Now;bWasControl=Ctl;
 const bool bKept=Retained.Get(false);if(bPrimed&&bKept&&!bWasRetained)RetainAt=Now;bWasRetained=bKept;bPrimed=true;
 // The picture winds to where the tape is: at playing speed it simply follows, a rewind or a jump is run at winding speed.
 if(Shown<0||Shown>L)Shown=At;
 const double Before=Shown,Gap=At-Shown;const bool Wind=FMath::Abs(Gap)>.35;
 Shown=Wind?Shown+FMath::Sign(Gap)*FMath::Min(FMath::Abs(Gap),Dt*26.):At;
 const double Moved=Shown-Before,U=Shown/L;
 // Tape packs: the supply reel empties as the take-up reel fills (area, not radius, is what moves). The control is a short reel.
 const float R=FMath::Min(S.Y*.5f-12,S.X*.17f),Hub=R*.34f;
 auto Pack=[&](double Full){return Hub+4+float(FMath::Sqrt(FMath::Clamp(Full,0.,1.)))*(R-Hub-8);};
 const double Spool=Ctl?.55:1.;
 const float PackL=Pack(Spool*(1-U*.82)),PackR=Pack(Spool*(.18+U*.82));
 AngleL+=Moved*38./FMath::Max(8.f,PackL);AngleR+=Moved*38./FMath::Max(8.f,PackR);
 const FVector2f CL(R+8,R+8),CR(S.X-R-8,R+8);
 auto Reel=[&](const FVector2f& C,float PackR2,double Angle)
 {
  for(float Rr=Hub+3;Rr<PackR2;Rr+=2.5f)Ring(C,Rr,Pc.Tape,2.6f);      // the wound tape
  Ring(C,PackR2,Pc.TapeEdge,1.5f);
  Ring(C,R,Pc.Cream*FLinearColor(1,1,1,.75f),1.6f);Ring(C,Hub,Pc.Cream,1.6f);Ring(C,Hub*.34f,Pc.Cream,1.4f);
  for(int32 K=0;K<3;++K)
  {
   const float A=float(Angle)+2*PI*K/3;const FVector2f D(FMath::Cos(A),FMath::Sin(A)),N(-D.Y,D.X);
   // three windows in the flange, turning with the reel
   Line({C+D*(Hub+5)+N*5,C+D*(R-6)+N*9,C+D*(R-6)-N*9,C+D*(Hub+5)-N*5,C+D*(Hub+5)+N*5},Pc.Cream*FLinearColor(1,1,1,.55f),1.2f);
   Line({C+D*(Hub*.34f),C+D*Hub},Pc.Cream,1.4f);
  }
 };
 Reel(CL,PackL,AngleL);Reel(CR,PackR,AngleR);
 // Tape path: off the supply pack, round two guides, across the head, onto the take-up pack. A new tape is threaded in.
 const float HeadY=S.Y-20,Mid=S.X*.5f;const FVector2f G1(CL.X+R*.9f,HeadY),G2(CR.X-R*.9f,HeadY);
 TArray<FVector2f> Path={CL+FVector2f(-PackL*.2f,PackL*.98f),G1,G2,CR+FVector2f(PackR*.2f,PackR*.98f)};
 if(const float Th=float(FMath::Clamp((Now-ThreadAt)/.6,0.,1.));Th<1)
 {
  float Total=0;for(int32 K=1;K<Path.Num();++K)Total+=FVector2f::Distance(Path[K-1],Path[K]);
  float Rest=Total*Th;TArray<FVector2f> Cut={Path[0]};
  for(int32 K=1;K<Path.Num()&&Rest>0;++K){const float Seg=FVector2f::Distance(Path[K-1],Path[K]);Cut.Add(Seg<=Rest?Path[K]:Path[K-1]+(Path[K]-Path[K-1])*(Rest/FMath::Max(Seg,.001f)));Rest-=Seg;}
  Path=MoveTemp(Cut);
 }
 if(Path.Num()>1)Line(Path,Pc.Oxide,2);
 Ring(G1,4,Pc.Cream,1.4f);Ring(G2,4,Pc.Cream,1.4f);
 HeadGlow=FMath::FInterpTo(float(HeadGlow),Run||Rec?1.f:Wind?.35f:0.f,float(Dt),9.f);
 Box(Mid-13,HeadY-4,26,14,Pc.Grid);Box(Mid-11,HeadY-2,22,10,FMath::Lerp(Pc.Dim,Rec?Pc.Rec:Pc.Amber,float(HeadGlow)));
 if(HeadGlow>.05)Box(Mid-22,HeadY+11,44,2,(Rec?Pc.Rec:Pc.Amber)*FLinearColor(1,1,1,.5f*float(HeadGlow)));
 // Needles: what passes the head now. A tick on the tape kicks them, the regular pulse nudges them; the peak lamp holds a moment.
 // D45-20261110: each tape has its own slow wow on the needles.
 const double Wow=2.4+3.*double(Key%89u)/89.;
 double Level=0;if(Run)
 {
  Level=.30+.16*Hash(FMath::FloorToDouble(Now*17)+double(Key%977u))+.07*FMath::Sin(Now*9.)+.02*FMath::Sin(Now*Wow);
  for(double Tick:Ticks.Get(TArray<double>())){const double D=At-Tick;if(D>=0&&D<.3)Level=FMath::Max(Level,.96-D*1.5);}
  if(Every>.2&&At>=Every){const double Since=At-FMath::FloorToDouble(At/Every)*Every;if(Since<.25)Level=FMath::Max(Level,.62-Since*1.5);}
  if(Ctl)Level*=.62;
 }
 else if(Wind)Level=.12+.08*Hash(FMath::FloorToDouble(Now*30));
 const float Mh=Small?40.f:58.f,My=Small?4.f:8.f;
 const float Mw=FMath::Min(92.f,(CR.X-CL.X-2*R-26)*.5f);
 for(int32 Ch=0;Ch<2&&Mw>40;++Ch)
 {
  const double Want=FMath::Clamp(Level*(Ch?.93:1.)+(Run?.04*Hash(Now*13+Ch*5):0),0.,1.);
  Needle[Ch]+=(Want-Needle[Ch])*FMath::Min(1.,Dt*(Want>Needle[Ch]?22.:6.)); // quick up, slow back, as a real meter
  if(Needle[Ch]>.8){Peak[Ch]=1;PeakAt[Ch]=Now;}else if(Now-PeakAt[Ch]>.7)Peak[Ch]=0;
  const float X=Mid+(Ch?5:-Mw-5);const FVector2f Pivot(X+Mw*.5f,My+Mh+10);
  DrawWindow(Out,Layer,G,Pc,X,My,Mw,Mh);
  for(int32 K=0;K<=10;++K)
  {
   const float A=FMath::DegreesToRadians(-48.f+96.f*K/10.f);const FVector2f D(FMath::Sin(A),-FMath::Cos(A));
   Line({Pivot+D*(Mh-2),Pivot+D*(Mh+(K%5?3:7))},K>=8?Pc.Amber:Pc.Cream,K>=8?2.f:1.2f);
  }
  const float A=FMath::DegreesToRadians(-48.f+96.f*float(Needle[Ch]));const FVector2f D(FMath::Sin(A),-FMath::Cos(A));
  Line({Pivot+D*14,Pivot+D*(Mh-4)},Pc.Cream,1.8f);
  Label(Ch?TEXT("VU R"):TEXT("VU L"),X+4,My+Mh-(Small?17.f:20.f),Pc.Cream*FLinearColor(1,1,1,.7f),Small?11:13);
  Box(X+Mw-13,My+5,8,8,Peak[Ch]>0?Pc.Amber:Pc.Ghost);
 }
 // The counter, in its own window with the unlit segments showing through, and the transport's state.
 const bool Blink=FMath::Fmod(Now,1.)<.5;
 const TCHAR* State=Rec?TEXT("REC"):Load?TEXT("LOADING"):Wind?(Gap<0?TEXT("<< REWIND"):TEXT(">> WIND")):Run?TEXT("> PLAY"):TEXT("STOP");
 const FString Count=FString::Printf(TEXT("%04d"),FMath::Clamp(int32(Shown*10+.5),0,9999));
 const float Cw=Small?80.f:96.f,Chh=Small?24.f:32.f,Cx=Mid-Cw*.5f,Cy=My+Mh+(Small?10.f:16.f);const int32 Digit=Small?18:24;const float Pitch=Small?17.f:21.f;
 DrawWindow(Out,Layer,G,Pc,Cx,Cy,Cw,Chh);
 if(Now-RetainAt<1.&&FMath::Fmod(Now-RetainAt,.5)<.3)Label(TEXT("RETAINED"),Cx+5,Cy+(Small?3.f:5.f),Pc.Amber,Small?15:19);
 else for(int32 K=0;K<4;++K){const float Dx=Cx+7+K*Pitch;Label(TEXT("8"),Dx,Cy+1,Lift(Pc.Ghost,2.2f),Digit);Label(Count.Mid(K,1),Dx,Cy+1,K==3?Pc.Amber:Pc.Cream,Digit);}
 const float Sx=Mid+Cw*.5f+10;
 // FIX10-10: the state text must not run over the take-up reel; where there is not room for the words, the short form stands.
 const float GlyphW=Small?9.f:10.5f,Room=CR.X-R-4-Sx-(Rec?16.f:0.f);
 const FString Shown2=FString(State).Len()*GlyphW<=Room?FString(State):Rec?FString(TEXT("REC")):Load?FString(TEXT("LOAD")):Wind?(Gap<0?FString(TEXT("<<")):FString(TEXT(">>"))):Run?FString(TEXT(">")):FString(TEXT("STOP"));
 if(Rec){Box(Sx,Cy+(Small?7.f:10.f),10,10,Blink?Pc.Rec:Pc.Ghost);Label(Shown2,Sx+16,Cy+(Small?3.f:5.f),Pc.Rec,Small?15:17);}
 else Label(Shown2,Sx,Cy+(Small?3.f:5.f),Run||Wind||Load?Pc.Amber:Pc.Cream*FLinearColor(1,1,1,.7f),Small?15:17);
 if(Mid-Cw*.5f-118>CL.X+R+6)Label(FString::Printf(TEXT("%04.1f / %.0f s"),At,L),Mid-Cw*.5f-118,Cy+(Small?3.f:5.f),Pc.Cream,Small?15:17);
 if(Small)return Layer;
 // The deck's memory marks on the counter's window, the take's label, and the faceplate: four screws and the engraving.
 const double In=MarkIn.Get(-1.),OutAt=MarkOut.Get(-1.);
 if(!bPhosphor)Label(FString::Printf(TEXT("MEMO IN %s  OUT %s"),In<0?TEXT("--.-"):*FString::Printf(TEXT("%04.1f"),In),OutAt<0?TEXT("--.-"):*FString::Printf(TEXT("%04.1f"),OutAt)),Mid-92,Cy+Chh+3,In>=0||OutAt>=0?Pc.Amber:Pc.Cream*FLinearColor(1,1,1,.55f),15);
 const FString Take=Caption.Get(FString());if(!Take.IsEmpty())Label(Take,Mid-float(Take.Len())*3.6f,Cy+Chh+21,Pc.Cream*FLinearColor(1,1,1,.8f),15);
 for(const FVector2f& Screw:{FVector2f(6,6),FVector2f(S.X-6,6),FVector2f(6,S.Y-6),FVector2f(S.X-6,S.Y-6)}){Ring(Screw,3,Pc.Cream*FLinearColor(1,1,1,.4f),1.f);Line({Screw-FVector2f(2,2),Screw+FVector2f(2,2)},Pc.Cream*FLinearColor(1,1,1,.4f),1.f);}
 Label(bPhosphor?TEXT("RX BANK 3 / TAPE TRANSPORT"):TEXT("SARO / RX-3 TAPE COMPARATOR"),CL.X+R+18,8,Pc.Cream*FLinearColor(1,1,1,.4f),14);
 return Layer;
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

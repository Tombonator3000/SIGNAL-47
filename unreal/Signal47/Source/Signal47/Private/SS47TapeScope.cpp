#include "SS47TapeScope.h"
#include "SS47Widgets.h"
#include "S47Keys.h"
#include "S47Theme.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
 // Sits on the reel deck's dark olive panel; signal stays a muted green, labels are cream.
 const FLinearColor Trace(.38f,.69f,.28f),Dim(.13f,.2f,.09f),Grid(.07f,.08f,.04f),Amber(.87f,.43f,.05f),Ghost(.05f,.055f,.028f),Cream(.87f,.8f,.62f);
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
 Label(Control.Get(false)?TEXT("NOW LOADED: CONTROL / THE STATION'S OWN REFERENCE"):TEXT("NOW LOADED: ORIGINAL / WHAT THE ANTENNAS RECORDED"),Left,0,Cream,20);
 if(CanMark.Get(false))Label(TEXT("CLICK TO MARK"),Right-150,0,Amber,20);
 const double In=MarkIn.Get(-1.),OutMark=MarkOut.Get(-1.);
 if(In>=0)Box(Px(In),Top,FMath::Max(2.f,(OutMark>In?Px(OutMark):Px(At))-Px(In)),TraceH,FLinearColor(.45f,.27f,.05f,.45f));
 for(int32 I=0;I<=int32(L);I+=2)
 {
  Line({FVector2f(Px(I),Top),FVector2f(Px(I),Top+TraceH)},Grid,1);
  Label(FString::FromInt(I),Px(I)-5,Top+TraceH+2,Cream,17);
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
 Label(TEXT("s"),Right+1,Top+TraceH+2,Cream,17);
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
  Label(Bands[B],X+BarW*.5f-14,BarBottom+1,Cream,17);
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
void SS47TapeReels::Construct(const FArguments& A)
{
 Cursor=A._Cursor;Length=A._Length;Playing=A._Playing;Loading=A._Loading;Control=A._Control;Ticks=A._Ticks;
 SetCanTick(false);ForceVolatile(true);SetVisibility(EVisibility::HitTestInvisible);
}
int32 SS47TapeReels::OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const
{
 const FVector2f S(G.GetLocalSize());if(S.X<200||S.Y<90)return Layer;
 const FSlateBrush* White=FCoreStyle::Get().GetBrush("WhiteBrush");
 auto Box=[&](float X,float Y,float W,float H,const FLinearColor& C){FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2f(W,H),FSlateLayoutTransform(FVector2f(X,Y))),White,ESlateDrawEffect::None,C);};
 auto Line=[&](const TArray<FVector2f>& P,const FLinearColor& C,float Thick){FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,Thick);};
 auto Label=[&](const FString& T,float X,float Y,const FLinearColor& C,int32 Size=15){FSlateDrawElement::MakeText(Out,Layer,G.ToPaintGeometry(FVector2f(220,26),FSlateLayoutTransform(FVector2f(X,Y))),T,S47Style::Mono(Size),ESlateDrawEffect::None,C);};
 auto Ring=[&](const FVector2f& C,float R,const FLinearColor& Col,float Thick)
 {
  const int32 N=FMath::Clamp(int32(R*.9f),14,48);TArray<FVector2f> P;P.Reserve(N+1);
  for(int32 I=0;I<=N;++I){const float A=2*PI*I/N;P.Add(C+FVector2f(FMath::Cos(A),FMath::Sin(A))*R);}
  Line(P,Col,Thick);
 };
 const double Now=FPlatformTime::Seconds(),Dt=FMath::Clamp(Now-Last,0.,.1);Last=Now;
 const double L=FMath::Max(1.,Length.Get(18.)),At=FMath::Clamp(Cursor.Get(0.),0.,L);const bool Run=Playing.Get(false),Load=Loading.Get(false);
 // The picture winds to where the tape is: at playing speed it simply follows, a rewind or a jump is run at winding speed.
 if(Shown<0||Shown>L)Shown=At;
 const double Before=Shown,Gap=At-Shown;const bool Wind=FMath::Abs(Gap)>.35;
 Shown=Wind?Shown+FMath::Sign(Gap)*FMath::Min(FMath::Abs(Gap),Dt*26.):At;
 const double Moved=Shown-Before,U=Shown/L;
 // Tape packs: the supply reel empties as the take-up reel fills (area, not radius, is what moves).
 const float R=FMath::Min(S.Y*.5f-12,S.X*.17f),Hub=R*.34f;
 auto Pack=[&](double Full){return Hub+4+float(FMath::Sqrt(FMath::Clamp(Full,0.,1.)))*(R-Hub-8);};
 const float PackL=Pack(1-U*.82),PackR=Pack(.18+U*.82);
 AngleL+=Moved*38./FMath::Max(8.f,PackL);AngleR+=Moved*38./FMath::Max(8.f,PackR);
 const FVector2f CL(R+8,R+8),CR(S.X-R-8,R+8);
 auto Reel=[&](const FVector2f& C,float PackR2,double Angle)
 {
  for(float Rr=Hub+3;Rr<PackR2;Rr+=2.5f)Ring(C,Rr,FLinearColor(.16f,.10f,.05f),2.6f);      // the wound tape, brown oxide
  Ring(C,PackR2,FLinearColor(.34f,.22f,.10f),1.5f);
  Ring(C,R,Cream*FLinearColor(1,1,1,.75f),1.6f);Ring(C,Hub,Cream,1.6f);Ring(C,Hub*.34f,Cream,1.4f);
  for(int32 K=0;K<3;++K)
  {
   const float A=float(Angle)+2*PI*K/3;const FVector2f D(FMath::Cos(A),FMath::Sin(A)),N(-D.Y,D.X);
   // three windows in the flange, turning with the reel
   Line({C+D*(Hub+5)+N*5,C+D*(R-6)+N*9,C+D*(R-6)-N*9,C+D*(Hub+5)-N*5,C+D*(Hub+5)+N*5},Cream*FLinearColor(1,1,1,.55f),1.2f);
   Line({C+D*(Hub*.34f),C+D*Hub},Cream,1.4f);
  }
 };
 Reel(CL,PackL,AngleL);Reel(CR,PackR,AngleR);
 // Tape path: off the supply pack, round two guides, across the head, onto the take-up pack.
 const float HeadY=S.Y-20,Mid=S.X*.5f;const FVector2f G1(CL.X+R*.9f,HeadY),G2(CR.X-R*.9f,HeadY);
 const FLinearColor Oxide(.42f,.27f,.12f);
 Line({CL+FVector2f(-PackL*.2f,PackL*.98f),G1,G2,CR+FVector2f(PackR*.2f,PackR*.98f)},Oxide,2);
 Ring(G1,4,Cream,1.4f);Ring(G2,4,Cream,1.4f);
 HeadGlow=FMath::FInterpTo(float(HeadGlow),Run?1.f:Wind?.35f:0.f,float(Dt),9.f);
 Box(Mid-13,HeadY-4,26,14,FLinearColor(.07f,.08f,.04f));Box(Mid-11,HeadY-2,22,10,FMath::Lerp(Dim,Amber,float(HeadGlow)));
 if(HeadGlow>.05)Box(Mid-22,HeadY+11,44,2,Amber*FLinearColor(1,1,1,.5f*float(HeadGlow)));
 // Needles: what passes the head now. A tick on the tape kicks them; the peak lamp holds a moment.
 double Level=0;if(Run)
 {
  Level=.30+.16*Hash(FMath::FloorToDouble(Now*17))+.07*FMath::Sin(Now*9.);
  for(double Tick:Ticks.Get(TArray<double>())){const double D=At-Tick;if(D>=0&&D<.3)Level=FMath::Max(Level,.96-D*1.5);}
  if(Control.Get(false))Level*=.62;
 }
 else if(Wind)Level=.12+.08*Hash(FMath::FloorToDouble(Now*30));
 const float Mw=FMath::Min(92.f,(CR.X-CL.X-2*R-26)*.5f),Mh=58,My=8;
 for(int32 Ch=0;Ch<2&&Mw>40;++Ch)
 {
  const double Want=FMath::Clamp(Level*(Ch?.93:1.)+(Run?.04*Hash(Now*13+Ch*5):0),0.,1.);
  Needle[Ch]+=(Want-Needle[Ch])*FMath::Min(1.,Dt*(Want>Needle[Ch]?22.:6.)); // quick up, slow back, as a real meter
  if(Needle[Ch]>.8){Peak[Ch]=1;PeakAt[Ch]=Now;}else if(Now-PeakAt[Ch]>.7)Peak[Ch]=0;
  const float X=Mid+(Ch?5:-Mw-5);const FVector2f Pivot(X+Mw*.5f,My+Mh+10);
  Line({FVector2f(X,My),FVector2f(X+Mw,My),FVector2f(X+Mw,My+Mh),FVector2f(X,My+Mh),FVector2f(X,My)},Cream*FLinearColor(1,1,1,.7f),1.2f);
  for(int32 K=0;K<=10;++K)
  {
   const float A=FMath::DegreesToRadians(-48.f+96.f*K/10.f);const FVector2f D(FMath::Sin(A),-FMath::Cos(A));
   Line({Pivot+D*(Mh-2),Pivot+D*(Mh+(K%5?3:7))},K>=8?Amber:Cream,K>=8?2.f:1.2f);
  }
  const float A=FMath::DegreesToRadians(-48.f+96.f*float(Needle[Ch]));const FVector2f D(FMath::Sin(A),-FMath::Cos(A));
  Line({Pivot+D*14,Pivot+D*(Mh-4)},Cream,1.8f);
  Label(Ch?TEXT("VU R"):TEXT("VU L"),X+4,My+Mh-20,Cream*FLinearColor(1,1,1,.7f),13);
  Box(X+Mw-13,My+5,8,8,Peak[Ch]>0?Amber:Ghost);
 }
 // The counter and the transport's state.
 const TCHAR* State=Load?TEXT("LOADING"):Wind?(Gap<0?TEXT("<< REWIND"):TEXT(">> WIND")):Run?TEXT("> PLAY"):TEXT("STOP");
 const FString Count=FString::Printf(TEXT("%04d"),FMath::Clamp(int32(Shown*10+.5),0,9999));
 const float Cw=88,Cx=Mid-Cw*.5f,Cy=My+Mh+16;
 Box(Cx,Cy,Cw,30,FLinearColor(.02f,.022f,.012f));
 for(int32 K=0;K<4;++K){Label(Count.Mid(K,1),Cx+7+K*20,Cy+2,K==3?Amber:Cream,22);if(K)Box(Cx+3+K*20,Cy+2,1,26,Ghost);}
 Label(State,Mid+Cw*.5f+10,Cy+5,Run||Wind||Load?Amber:Cream*FLinearColor(1,1,1,.7f),17);
 Label(FString::Printf(TEXT("%04.1f / %.0f s"),At,L),Mid-Cw*.5f-118,Cy+5,Cream,17);
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

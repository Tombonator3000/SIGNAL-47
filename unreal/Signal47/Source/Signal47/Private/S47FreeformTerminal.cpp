#include "S47FreeformTerminal.h"
#include "S47Kit.h"
#include "SS47Widgets.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include <limits>

namespace S47FreeformTerminal
{
 // Phosphor palette (mockups): bright green text and lines, amber selection/action,
 // near-black ink on filled amber/green, red only for an isolated element.
 // FReelLook swaps these while one panel is built; widgets copy what they need at that moment.
 FLinearColor Green=S47Kit::Css(0x8cffa4),Amber=S47Kit::Css(0xffb84a),Line=S47Kit::Css(0x3f9a5a),Dim=S47Kit::Css(0x1d4a2b),Black=S47Kit::Css(0x020a05);
 const FLinearColor Ink=S47Kit::Css(0x000000),Red=S47Kit::Css(0xff5a44);
 static bool bReel=false;
 FReelLook::FReelLook()
 {
  bReel=true;Green=S47Kit::Css(0xf1e8cf);Amber=S47Kit::Css(0xf0b040);Line=S47Kit::Css(0xd9d0b4);Dim=S47Kit::Css(0x62664a);Black=S47Kit::Css(0x24271b);
 }
 FReelLook::~FReelLook()
 {
  bReel=false;Green=S47Kit::Css(0x8cffa4);Amber=S47Kit::Css(0xffb84a);Line=S47Kit::Css(0x3f9a5a);Dim=S47Kit::Css(0x1d4a2b);Black=S47Kit::Css(0x020a05);
 }
 namespace
 {
  TFunction<FString()> ClockSource;
  FString StationSource; // CASES-OPS-20261109 (SetStation)
  double Now(){return FPlatformTime::Seconds();}
  // FIX-20261110 (Tom: more effects, feedback and sound on the screens; the look stays).
  // The operator's hand on the station: every control reports its kind here and the owner's sink
  // gives it a sound (D45-20261110: the glass no longer flashes for it; Tom kept the sounds).
  TFunction<void(int32)> SoundSink;TFunction<float()> JamSource;
  double KeyAt=-10,HoverAt=-10,PowerAt=-1e9;
  // D45-20261110: the tracking plot's search tone and how near the pointer is to the source (the owner's), and the screen the
  // next frame is for (its hum bar keeps the phase of the idle picture on the same monitor); -1 for none.
  TFunction<void(float)> ProbeSink;TFunction<double(FVector2D)> NearSource;int32 HumScreen=-1;
  TFunction<bool()> CalmSource; // RX-TABS-DUE-20261113 (SetCalm)
  void Key(int32 Kind)
  {
   const double T=Now();if(Kind==3&&T-KeyAt<.07)return; // a dragged knob ticks, it does not buzz
   KeyAt=T;if(SoundSink)SoundSink(Kind);
  }
  void Hover(){const double T=Now();if(T-KeyAt<.2||T-HoverAt<.06)return;HoverAt=T;if(SoundSink)SoundSink(4);}
  float Pulse(double Speed=3.){return .55f+.45f*float(FMath::Sin(Now()*Speed));}
  const FSlateBrush* Fill(){static const FSlateColorBrush B(FLinearColor::White);return &B;}
  const FSlateBrush* Outline(){static const FSlateRoundedBoxBrush B(FLinearColor::Transparent,2.f,FLinearColor::White,2.f);return &B;}
  const FSlateBrush* Solid(){static const FSlateRoundedBoxBrush B(FLinearColor::White,2.f,FLinearColor::White,2.f);return &B;}
  FButtonStyle MakeStyle(FLinearColor Normal,FLinearColor Hover,FLinearColor Pressed,FLinearColor Edge,float EdgeWidth)
  {
   return FButtonStyle()
    .SetNormal(FSlateRoundedBoxBrush(Normal,2.f,Edge,EdgeWidth)).SetHovered(FSlateRoundedBoxBrush(Hover,2.f,Edge,EdgeWidth)).SetPressed(FSlateRoundedBoxBrush(Pressed,2.f,Edge,EdgeWidth))
    .SetDisabled(FSlateRoundedBoxBrush(FLinearColor::Transparent,2.f,Dim,EdgeWidth)).SetNormalPadding(FMargin(12,8)).SetPressedPadding(FMargin(12,9,12,7));
  }
  const FButtonStyle& ReelOutline(){static const FButtonStyle S=MakeStyle(S47Kit::Css(0x24271b),S47Kit::Css(0x3a3e2b),S47Kit::Css(0x4b5038),S47Kit::Css(0xf1e8cf),2.f);return S;}
  const FButtonStyle& ReelPrimary(){static const FButtonStyle S=MakeStyle(S47Kit::Css(0x24271b),S47Kit::Css(0x423617),S47Kit::Css(0x57471c),S47Kit::Css(0xf0b040),3.f);return S;}
  const FButtonStyle& ReelFilled(){static const FButtonStyle S=MakeStyle(S47Kit::Css(0xa4d890),S47Kit::Css(0xb8e6a6),S47Kit::Css(0x90c67c),S47Kit::Css(0xa4d890),2.f);return S;}
  const FButtonStyle& OutlineStyle(){if(bReel)return ReelOutline();static const FButtonStyle S=MakeStyle(Black,S47Kit::Css(0x0e3519),S47Kit::Css(0x1b5a2c),Green,2.f);return S;}
  const FButtonStyle& PrimaryStyle(){if(bReel)return ReelPrimary();static const FButtonStyle S=MakeStyle(Black,S47Kit::Css(0x2a1c05),S47Kit::Css(0x4a3008),Amber,3.f);return S;}
  // Every style that reads the palette has one copy per look; a single static would keep whichever look asked first for the whole session.
  const FButtonStyle& ReelSelected(){static const FButtonStyle S=MakeStyle(S47Kit::Css(0xf0b040),S47Kit::Css(0xffcc73),S47Kit::Css(0xe09a30),S47Kit::Css(0xf0b040),2.f);return S;}
  const FButtonStyle& FilledStyle(){if(bReel)return ReelFilled();static const FButtonStyle S=MakeStyle(Green,S47Kit::Css(0xb4ffc4),S47Kit::Css(0x6fdc88),Green,2.f);return S;}
  const FButtonStyle& PlainStyle(){static const FButtonStyle S=MakeStyle(FLinearColor::Transparent,S47Kit::Css(0x0e3519),S47Kit::Css(0x1b5a2c),FLinearColor::Transparent,0.f);return S;}
  const FButtonStyle& SelectedStyle(){if(bReel)return ReelSelected();static const FButtonStyle S=MakeStyle(Amber,S47Kit::Css(0xffcc73),S47Kit::Css(0xe09a30),Amber,2.f);return S;}
  FW Rule(FLinearColor C=Line,float H=2){return SNew(SBox).HeightOverride(H)[SNew(SBorder).BorderImage(Fill()).BorderBackgroundColor(C)];}
  FW Label(TAttribute<FString> V,int32 Size,FLinearColor C)
  {return SNew(STextBlock).Text_Lambda([V]{return FText::FromString(V.Get());}).Font(S47Style::Mono(Size)).ColorAndOpacity(C);}
  void Text2(FSlateWindowElementList& Out,int32 L,const FGeometry& G,const FString& S,float X,float Y,int32 Size,FLinearColor C)
  {FSlateDrawElement::MakeText(Out,L,G.ToPaintGeometry(FVector2f(400.f,float(Size)+6),FSlateLayoutTransform(FVector2f(X,Y))),S,S47Style::Mono(Size),ESlateDrawEffect::None,C);}
  void Lines(FSlateWindowElementList& Out,int32 L,const FGeometry& G,const TArray<FVector2f>& P,FLinearColor C,float W=1.5f)
  {FSlateDrawElement::MakeLines(Out,L,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,W);}
  void Dashed(FSlateWindowElementList& Out,int32 L,const FGeometry& G,FVector2f A,FVector2f B,FLinearColor C,float Dash=7,float W=1.5f)
  {
   const float Len=FVector2f::Distance(A,B);if(Len<1)return;const FVector2f D=(B-A)/Len;
   for(float T=0;T<Len;T+=Dash*2)Lines(Out,L,G,{A+D*T,A+D*FMath::Min(Len,T+Dash)},C,W);
  }
  void Rect(FSlateWindowElementList& Out,int32 L,const FGeometry& G,FVector2f At,FVector2f Size,FLinearColor C,bool bSolid)
  {
   if(bSolid){FSlateDrawElement::MakeBox(Out,L,G.ToPaintGeometry(Size,FSlateLayoutTransform(At)),Fill(),ESlateDrawEffect::None,C);return;}
   Lines(Out,L,G,{At,At+FVector2f(Size.X,0),At+Size,At+FVector2f(0,Size.Y),At},C,2.f);
  }
  const FSliderStyle& KnobStyle()
  {
   static const FSliderStyle Style=[]
   {
    FSliderStyle S=FCoreStyle::Get().GetWidgetStyle<FSliderStyle>("Slider");
    const FSlateColorBrush Bar(FLinearColor::White);const FSlateRoundedBoxBrush Thumb(FLinearColor::White,3.f,FVector2f(18,26));
    S.SetNormalBarImage(Bar).SetHoveredBarImage(Bar).SetDisabledBarImage(Bar);
    S.SetNormalThumbImage(Thumb).SetHoveredThumbImage(Thumb).SetDisabledThumbImage(Thumb);
    S.SetBarThickness(3.f);return S;
   }();return Style;
  }
 }

 FW Text(TAttribute<FString> Value,TAttribute<int32> Tone)
 {return SNew(STextBlock).Text_Lambda([Value]{return FText::FromString(Value.Get());}).Font(S47Style::Mono(24)).ColorAndOpacity_Lambda([Tone,AmberNow=Amber,GreenNow=Green]{return FSlateColor(Tone.Get()<0?AmberNow:GreenNow);}).AutoWrapText(true).AddMetaData(MakeShared<FS47WrapMark>()).Margin(FMargin(0,5));}
 FW Text(const FString& Value,int32 Tone){return Text(TAttribute<FString>(Value),TAttribute<int32>(Tone));}
 FW Paper(const FString& Value){return Text(Value);}
 FW Small(const FString& Value){return SNew(STextBlock).Text(FText::FromString(Value)).Font(S47Style::Mono(20)).ColorAndOpacity(Green*.8f).AutoWrapText(true).AddMetaData(MakeShared<FS47WrapMark>()).Margin(FMargin(0,6));}
 FW ButtonLive(TAttribute<FString> Text,TFunction<void()> Action,bool Primary,TAttribute<bool> Enabled)
 {
  return SNew(SButton).ButtonStyle(Primary?&PrimaryStyle():&OutlineStyle()).ContentPadding(FMargin(12,7)).IsEnabled(Enabled).OnHovered_Lambda([]{Hover();}).OnClicked_Lambda([Action,Primary]{Key(Primary?1:0);if(Action)Action();return FReply::Handled();})
  [SNew(STextBlock).Text_Lambda([Text]{return FText::FromString(Text.Get());}).Font(S47Style::Mono(24)).ColorAndOpacity_Lambda([Primary,Enabled,AmberNow=Amber,GreenNow=Green,DimNow=Dim]{return FSlateColor(!Enabled.Get()?DimNow*1.6f:Primary?AmberNow:GreenNow);}).AutoWrapText(true).AddMetaData(MakeShared<FS47WrapMark>())];
 }
 FW Toggle(const FString& Text,TAttribute<bool> On,TFunction<void()> Action,TAttribute<bool> Enabled)
 {
  const TAttribute<EVisibility> Off=TAttribute<EVisibility>::CreateLambda([On]{return On.Get()?EVisibility::Collapsed:EVisibility::Visible;});
  const TAttribute<EVisibility> Lit=TAttribute<EVisibility>::CreateLambda([On]{return On.Get()?EVisibility::Visible:EVisibility::Collapsed;});
  return SNew(SOverlay)
   +SOverlay::Slot()[SNew(SBox).Visibility(Off)[Button(Text,Action,false,Enabled)]]
   +SOverlay::Slot()[SNew(SBox).Visibility(Lit)
    [SNew(SButton).ButtonStyle(bReel?&ReelFilled():&FilledStyle()).ContentPadding(FMargin(12,7)).IsEnabled(Enabled).OnHovered_Lambda([]{Hover();}).OnClicked_Lambda([Action]{Key(0);if(Action)Action();return FReply::Handled();})
     [SNew(STextBlock).Text(FText::FromString(Text)).Font(S47Style::Mono(24)).ColorAndOpacity(Ink)]]];
 }
 FW Filled(const FString& Text,TFunction<void()> Action,TAttribute<bool> Enabled)
 {
  return SNew(SButton).ButtonStyle(&SelectedStyle()).ContentPadding(FMargin(12,7)).HAlign(HAlign_Center).IsEnabled(Enabled).OnHovered_Lambda([]{Hover();}).OnClicked_Lambda([Action]{Key(1);if(Action)Action();return FReply::Handled();})
   [SNew(STextBlock).Text(FText::FromString(Text)).Font(S47Style::Mono(26)).ColorAndOpacity_Lambda([Enabled,DimNow=Dim]{return FSlateColor(Enabled.Get()?Ink:DimNow*1.6f);})];
 }
 FW Button(const FString& Text,TFunction<void()> Action,bool Primary,TAttribute<bool> Enabled){return ButtonLive(TAttribute<FString>(Text),Action,Primary,Enabled);}
 FW Pair(FW Left,FW Right)
 {return SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(1).Padding(0,0,12,0)[Left]+SHorizontalBox::Slot().FillWidth(1)[Right];}
 FW Control(const FString& Name,TAttribute<FString> Shown,float Min,float Max,float Step,TAttribute<float> Value,TFunction<void(float)> OnChange)
 {
  auto Nudge=[Value,OnChange,Min,Max](float By){Key(3);if(OnChange)OnChange(FMath::Clamp(Value.Get()+By,Min,Max));return FReply::Handled();};
  auto Glyph=[](const TCHAR* Glyph){return SNew(STextBlock).Text(FText::FromString(Glyph)).Font(S47Style::Mono(30)).ColorAndOpacity(Green).Justification(ETextJustify::Center);};
  // One SVerticalBox per control holding its name and one slider: the evidence journey
  // locates the real receiver controls by exactly this structure.
  return SNew(SBorder).BorderImage(Outline()).BorderBackgroundColor(Line).Padding(FMargin(12,5))
  [SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()
    [SNew(SHorizontalBox)
     +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text(FText::FromString(Name)).Font(S47Style::Mono(26)).ColorAndOpacity(Green)]
     +SHorizontalBox::Slot().AutoWidth().Padding(10,0,0,0)[SNew(STextBlock).Text_Lambda([Shown]{return FText::FromString(Shown.Get());}).Font(S47Style::Mono(28)).ColorAndOpacity(Amber)]]
   +SVerticalBox::Slot().AutoHeight().Padding(0,4,0,1)
    [SNew(SHorizontalBox)
     +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(44).HeightOverride(40)[SNew(SButton).ButtonStyle(&OutlineStyle()).ContentPadding(0).HAlign(HAlign_Center).VAlign(VAlign_Center).OnClicked_Lambda([Nudge,Step]{return Nudge(-Step);})[Glyph(TEXT("-"))]]]
     +SHorizontalBox::Slot().FillWidth(1).Padding(10,0)
      [SNew(SBox).HeightOverride(40).VAlign(VAlign_Fill)
       [SNew(SSlider).Style(&KnobStyle()).MinValue(Min).MaxValue(Max).StepSize(Step).MouseUsesStep(true).Value(Value)
        .SliderBarColor(Line).SliderHandleColor(Amber).OnValueChanged_Lambda([OnChange](float V){Key(3);if(OnChange)OnChange(V);})]]
     +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(44).HeightOverride(40)[SNew(SButton).ButtonStyle(&OutlineStyle()).ContentPadding(0).HAlign(HAlign_Center).VAlign(VAlign_Center).OnClicked_Lambda([Nudge,Step]{return Nudge(Step);})[Glyph(TEXT("+"))]]]]];
 }
 FW Knob(const FString& Name,TAttribute<FString> Shown,float Min,float Max,float Step,TAttribute<float> Value,TFunction<void(float)> OnChange)
 {return Control(Name,Shown,Min,Max,Step,Value,OnChange);}
 void SetClock(TFunction<FString()> Clock){ClockSource=MoveTemp(Clock);}
 void SetStation(const FString& Station){StationSource=Station;}
 void SetSound(TFunction<void(int32)> Sink){SoundSink=MoveTemp(Sink);}
 void SetInterference(TFunction<float()> Level){JamSource=MoveTemp(Level);}
 void PowerOn(){PowerAt=Now();}
 double PowerAge(){return Now()-PowerAt;}
 void SetHumScreen(int32 Screen){HumScreen=Screen;}
 void SetProbeSound(TFunction<void(float)> Sink){ProbeSink=MoveTemp(Sink);}
 void SetNearness(TFunction<double(FVector2D)> Near){NearSource=MoveTemp(Near);}
 float Interference(){return JamSource?FMath::Clamp(JamSource(),0.f,1.f):0.f;}
 // D45-20261110 (Tom: the band was too big and always there, and the picture jumped on every menu; a real CRT does not). A real
 // 1986 set holds a still picture. Now and then a hum bar, the mains beating against the field rate, drifts up the tube: soft,
 // wide and barely there, with no edge and no line. One chance in each 90 seconds per screen, taken two times in five, so
 // a pass every four minutes or so, eleven seconds long, fading in and out. Tune here by eye: Slot, Chance, Peak.
 FHum HumBar(double T,int32 Seed,float ScreenH)
 {
  constexpr double Slot=90,Pass=11,Chance=.4;constexpr float Peak=.05f; // D45 critic: .028 summed to about .02 on the glass, which nobody sees
  if(!FMath::IsFinite(T)||!(ScreenH>0))return FHum();
  auto Rnd=[](double X){const double V=FMath::Sin(X*12.9898)*43758.5453;return V-FMath::FloorToDouble(V);};
  const int32 Sd=FMath::Max(0,Seed);const double At=T+Sd*.37*Slot,N=FMath::FloorToDouble(At/Slot);
  if(Rnd(N*1.7+Sd*3.1)>Chance)return FHum();
  const double In=At-N*Slot-Rnd(N+Sd*5.3)*(Slot-Pass);
  if(In<0||In>Pass)return FHum();
  const double U=In/Pass;FHum B;
  B.Y=ScreenH*float(1.1-1.3*U);B.H=ScreenH*.22f;B.A=Peak*float(FMath::Sin(UE_DOUBLE_PI*U)); // up the tube, in and out with no pop
  return B;
 }
 void PaintHum(FSlateWindowElementList& Out,int32 Layer,const FGeometry& G,const FVector2f& Size,int32 Seed,const FLinearColor& Phosphor)
 {
  const FHum B=HumBar(Now(),Seed,Size.Y);if(B.A<=0)return;
  constexpr int32 Bands=12; // thin strips, strongest in the middle and nothing at the rims: no edge to see
  for(int32 K=0;K<Bands;++K)
  {
   FLinearColor C=Phosphor;C.A=B.A*FMath::Sin(UE_PI*(K+.5f)/Bands);
   FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2f(Size.X,B.H/Bands),FSlateLayoutTransform(FVector2f(0,B.Y-B.H*.5f+B.H*K/Bands))),Fill(),ESlateDrawEffect::None,C);
  }
 }
 // The set coming on (PowerOn, only when the supply comes back): a line across the middle that opens into the picture.
 void PaintPowerOn(FSlateWindowElementList& Out,int32 Layer,const FGeometry& G,const FVector2f& Size,const FLinearColor& Phosphor)
 {
  const double On=PowerAge();if(On<0||On>=.55)return;
  auto Box=[&](FVector2f At,FVector2f Wh,FLinearColor C,int32 Lr){if(Wh.X>0&&Wh.Y>0)FSlateDrawElement::MakeBox(Out,Lr,G.ToPaintGeometry(Wh,FSlateLayoutTransform(At)),Fill(),ESlateDrawEffect::None,C);};
  FLinearColor Beam=Phosphor;Beam.A=.95f;FLinearColor Glow=Phosphor;Glow.A=.10f*float(1-On/.55);
  const float Open=float(FMath::Clamp((On-.12)/.3,0.,1.));const float Half=Size.Y*.5f*Open*Open*(3-2*Open),Mid=Size.Y*.5f;
  Box({0.f,0.f},{Size.X,Mid-Half-1},FLinearColor::Black,Layer);Box({0.f,Mid+Half+1},{Size.X,Size.Y-Mid-Half-1},FLinearColor::Black,Layer);
  const float Wd=Size.X*float(FMath::Clamp(On/.12,0.,1.));
  if(Open<1)Box({(Size.X-Wd)*.5f,Mid-Half-2},{Wd,3.f},Beam,Layer+1);
  if(Open>0&&Open<1)Box({0.f,Mid+Half-1},{Size.X,3.f},Beam,Layer+1);
  Box({0.f,0.f},Size,Glow,Layer+1);
 }
 void SetCalm(TFunction<bool()> Calm){CalmSource=MoveTemp(Calm);}
 bool CalmNow(){return CalmSource&&CalmSource();}
 float DueLevel(double Seconds,bool bCalm)
 {
  if(bCalm||!FMath::IsFinite(Seconds))return 1.f;
  return .5f-.5f*float(FMath::Cos(Seconds*2.*UE_DOUBLE_PI)); // 1 Hz, soft at both ends
 }
 FW DueRing(FW Inner,TFunction<bool()> Due)
 {
  const FLinearColor AmberNow=Amber;
  auto Level=[Due]{return Due&&Due()?DueLevel(Now(),CalmNow()):-1.f;};
  return SNew(SOverlay)
   +SOverlay::Slot()[Inner]
   +SOverlay::Slot()[SNew(SBorder).Visibility(EVisibility::HitTestInvisible).BorderImage(Fill()).BorderBackgroundColor_Lambda([Level,AmberNow]{const float L=Level();FLinearColor C=AmberNow;C.A=L<0?0.f:.16f*L;return FSlateColor(C);})]
   +SOverlay::Slot()[SNew(SBorder).Visibility(EVisibility::HitTestInvisible).BorderImage(Outline()).BorderBackgroundColor_Lambda([Level,AmberNow]{const float L=Level();FLinearColor C=FMath::Lerp(AmberNow,S47Kit::Css(0xffe2b0),L<0?0.f:L);C.A=L<0?0.f:.35f+.65f*L;return FSlateColor(C);})];
 }
 bool SelfTest(TArray<FString>& Report)
 {
  bool bDueOk=true;
  { // RX-TABS-DUE-20261113: the due cue breathes at 1 Hz, softly, never more than three changes a second; calm holds it lit
   float Lo=1,Hi=0,Step=0,Was=DueLevel(100.,false);int32 Turns=0,Dir=0;bool bCalm=true;
   for(int32 I=1;I<=1200;++I)
   {
    const double T=100.+I/120.;const float L=DueLevel(T,false);Lo=FMath::Min(Lo,L);Hi=FMath::Max(Hi,L);Step=FMath::Max(Step,FMath::Abs(L-Was));
    const int32 D=L>Was?1:L<Was?-1:Dir;if(Dir!=0&&D!=Dir)++Turns;Dir=D;Was=L;bCalm&=DueLevel(T,true)==1.f;
   }
   const bool bBreath=Lo<.02f&&Hi>.98f&&Step<.03f&&Turns<=3*10&&Turns>=15;
   Report.Add(FString::Printf(TEXT("%s freeform terminal: the due tab breathes 0..1 (%.2f..%.2f), largest step %.3f at 120 Hz, %d turns in 10 s (at most 3 a second)"),bBreath?TEXT("ok"):TEXT("FAIL"),Lo,Hi,Step,Turns));
   const bool bHeld=bCalm&&DueLevel(std::numeric_limits<double>::quiet_NaN(),false)==1.f;
   Report.Add(FString::Printf(TEXT("%s freeform terminal: with calm flash the due tab does not pulse, it is held lit with its marker"),bHeld?TEXT("ok"):TEXT("FAIL")));
   bDueOk=bBreath&&bHeld;
  }
  // D45-20261110: the hum bar is rare, faint and smooth on every screen, and it does come now and then
  bool bOk=true;
  for(int32 Sd=0;Sd<4;++Sd)
  {
   int32 Off=0,Steps=0,Seen=0;float Most=0,Jump=0,Was=HumBar(1000,Sd,900).A;
   for(int32 I=0;I<6000;++I,++Steps){const float A=HumBar(1000+I*.1,Sd,900).A;Off+=A<=0?1:0;Most=FMath::Max(Most,A);Jump=FMath::Max(Jump,FMath::Abs(A-Was));Was=A;}
   for(int32 I=0;I<3600;++I)Seen+=HumBar(5000+I,Sd,900).A>0?1:0;
   const bool bScreen=Off*2>=Steps&&Most<=.06f&&Jump<.008f&&Seen>0&&Seen*2<3600;
   Report.Add(FString::Printf(TEXT("%s freeform terminal: screen %d hum bar off %d of %d steps, strongest %.3f, largest step %.4f, on %d s of an hour"),bScreen?TEXT("ok"):TEXT("FAIL"),Sd,Off,Steps,Most,Jump,Seen));
   bOk&=bScreen;
  }
  const bool bSafe=HumBar(std::numeric_limits<double>::quiet_NaN(),0,900).A==0&&HumBar(1000,0,0).A==0;
  Report.Add(FString::Printf(TEXT("%s freeform terminal: no hum bar for a clock that is not a number or a screen with no height"),bSafe?TEXT("ok"):TEXT("FAIL")));
  return bOk&&bSafe&&bDueOk;
 }
 // What a tube does that a drawing of one does not: the picture opens from a line when the set comes on, a faint hum bar drifts
 // through it now and then, and what is on the air tonight tears lines out of it. A page change is just the new page, as on a real
 // terminal (D45-20261110: no wipe, no whole-tube flash on a key; the key's own sound and the button's pressed look answer it).
 // Drawn over the page and under the glass's own scan lines; it takes no clicks and moves no layout.
 class SGlassFx:public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SGlassFx){} SLATE_END_ARGS()
  int32 Seed=7;TFunction<float()> Jam;FLinearColor GreenNow,AmberNow,BlackNow;
  void Construct(const FArguments&){GreenNow=Green;AmberNow=Amber;BlackNow=Black;ForceVolatile(true);SetVisibility(EVisibility::HitTestInvisible);}
  virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(1,1);}
  static float Hash(double X){const double V=FMath::Sin(X*12.9898)*43758.5453;return float(V-FMath::FloorToDouble(V));}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 L,const FWidgetStyle&,bool)const override
  {
   const FVector2f S(G.GetLocalSize());const double T=Now();
   auto Box=[&](float Y,float H,FLinearColor C,int32 Layer){if(H>0)FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2f(S.X,H),FSlateLayoutTransform(FVector2f(0,Y))),Fill(),ESlateDrawEffect::None,C);};
   auto Tint=[](FLinearColor C,float A){C.A=A;return C;};
   PaintHum(Out,L,G,S,Seed,GreenNow);
   // On the air: lines drop out, a band of snow crosses, the frame dims on a bad beat.
   if(const float J=Jam?FMath::Clamp(Jam(),0.f,1.f):0.f;J>.03f)
   {
    const double Beat=FMath::FloorToDouble(T*17);
    const int32 Bars=1+int32(J*9);
    for(int32 I=0;I<Bars;++I)
    {
     if(Hash(Beat+I*7.31)>J*.9f)continue;
     const float Y=S.Y*Hash(Beat*1.7+I*3.1),H=2+26*Hash(Beat*.37+I)*J;
     Box(Y,H,Hash(Beat+I)<.6f?Tint(BlackNow,.85f):Tint(GreenNow,.10f+.25f*J),L+4);
    }
    const float Snow=S.Y*float(FMath::Fmod(T*.61,1.));
    for(int32 I=0;I<int32(60*J);++I)
    {
     const float X=S.X*Hash(Beat*2.3+I*1.37),Y=Snow+60*Hash(Beat+I*5.9)-30;
     FSlateDrawElement::MakeBox(Out,L+4,G.ToPaintGeometry(FVector2f(6+18*Hash(I+Beat),2),FSlateLayoutTransform(FVector2f(X,Y))),Fill(),ESlateDrawEffect::None,Tint(GreenNow,.25f+.5f*J));
    }
    if(Hash(Beat*.91)<J*.35f)Box(0,S.Y,Tint(FLinearColor::Black,.25f+.4f*J),L+4);
   }
   PaintPowerOn(Out,L+5,G,S,GreenNow);
   return L+7;
  }
 };
 FW Frame(const FString& Title,FW Body,TSharedPtr<SWidget> Footer)
 {
  // One header row as in the mockups: station | screen title | day and clock.
  const TFunction<FString()> Clock=ClockSource;
  const FString Station=StationSource.IsEmpty()?FString(TEXT("SARO / LOCAL WORKSTATION")):StationSource;
  auto Bar=[]{return SNew(STextBlock).Text(FText::FromString(TEXT("|"))).Font(S47Style::Mono(26)).ColorAndOpacity(Green);};
  // The glass is 1200 wide; the frame's padding takes 24 of it (FS47WrapMark).
  S47Kit::FirstFrameWrap(Body,1176.f);
  const auto Fx=SNew(SGlassFx);Fx->Seed=HumScreen>=0?HumScreen:7;Fx->Jam=JamSource;
  // D45-20261110: the same page built again (a tab, a pick, a live refresh) stays where the operator had scrolled it
  static TWeakPtr<SScrollBox> LastScroll;static FString LastScrollTitle;
  const TSharedPtr<SScrollBox> Was=LastScroll.Pin();const float Keep=Was.IsValid()&&Title==LastScrollTitle?Was->GetScrollOffset():0.f;
  const TSharedRef<SScrollBox> Scroll=SNew(SScrollBox);Scroll->AddSlot()[Body];if(Keep>0)Scroll->SetScrollOffset(Keep);
  LastScroll=Scroll;LastScrollTitle=Title;
  const TFunction<float()> Jam=bReel?TFunction<float()>():JamSource; // the reel deck is a panel of lamps, not a tube
  // On a bad beat the whole picture loses its hold and jumps sideways a few lines' worth.
  const auto Tear=TAttribute<TOptional<FSlateRenderTransform>>::CreateLambda([Jam]
  {
   const float J=Jam?FMath::Clamp(Jam(),0.f,1.f):0.f;if(J<.15f)return TOptional<FSlateRenderTransform>();
   const double Beat=FMath::FloorToDouble(Now()*13);const float H=SGlassFx::Hash(Beat*3.3);
   return H<J*.4f?TOptional<FSlateRenderTransform>(FSlateRenderTransform(FVector2f((SGlassFx::Hash(Beat)-.5f)*36*J,0))):TOptional<FSlateRenderTransform>();
  });
  return SNew(SOverlay)
  +SOverlay::Slot()
  [SNew(SBorder).BorderImage(Fill()).BorderBackgroundColor(Black).Padding(FMargin(12,6)).RenderTransform(Tear)
  [SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()
    [SNew(SHorizontalBox)
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Label(Station,22,Green)]
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14,0)[Bar()]
     +SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Center).VAlign(VAlign_Center)
      [SNew(SHorizontalBox)
       +SHorizontalBox::Slot().AutoWidth()[Label(Title,30,Amber)]
       // A slow block cursor: the station is live even while nothing is selected.
       +SHorizontalBox::Slot().AutoWidth().Padding(4,0,0,0)[SNew(STextBlock).Text(FText::FromString(TEXT("_"))).Font(S47Style::Mono(30)).ColorAndOpacity_Lambda([AmberNow=Amber]{return FSlateColor(AmberNow*(FMath::Fmod(Now(),1.1)<.6?1.f:0.f));})]]
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14,0)[Bar()]
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Label(TAttribute<FString>::CreateLambda([Clock]{return Clock?Clock():FString(TEXT("SIGNAL / 47"));}),24,Green)]]
   +SVerticalBox::Slot().AutoHeight().Padding(0,4,0,8)[Rule(Green)]
   +SVerticalBox::Slot().FillHeight(1)[Scroll]
   // Below the scroll area, so the way out stays in view however long the page is.
   +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)[Footer.IsValid()?Footer.ToSharedRef():SNullWidget::NullWidget]]]
  +SOverlay::Slot()[bReel?S47Kit::FW(SNullWidget::NullWidget):S47Kit::FW(Fx)];
 }
 FW Nav(const TArray<FNav>& Items)
 {
  TSharedRef<SVerticalBox> Box=SNew(SVerticalBox);
  for(const FNav& I:Items)
  {
   const bool Sel=I.bSelected,Attention=I.bAttention;const TFunction<void()> Act=I.Action;
   Box->AddSlot().AutoHeight()
    [SNew(SButton).ButtonStyle(Sel?&SelectedStyle():&PlainStyle()).ContentPadding(FMargin(12,8)).OnHovered_Lambda([]{Hover();}).OnClicked_Lambda([Act]{Key(2);if(Act)Act();return FReply::Handled();})
     [SNew(SHorizontalBox)
      +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text(FText::FromString(I.Label)).Font(S47Style::Mono(26)).ColorAndOpacity(Sel?Ink:Green)]
      +SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(Attention?TEXT("*"):TEXT(""))).Font(S47Style::Mono(26)).ColorAndOpacity_Lambda([Sel]{return FSlateColor((Sel?Ink:Amber)*(CalmNow()?1.f:Pulse(5)));})]]]; // RX-TABS-DUE-20261113: held when calm
   Box->AddSlot().AutoHeight().Padding(8,0)[Rule(Dim)];
  }
  return Box;
 }
 FW Tabs(const TArray<FNav>& Items)
 {
  TSharedRef<SHorizontalBox> Box=SNew(SHorizontalBox);
  for(const FNav& I:Items)
  {
   const bool Sel=I.bSelected;const TFunction<void()> Act=I.Action;
   // RX-TABS-DUE-20261113: the due tab (never the selected one). The marker has its own text block and a fixed place, so the label
   // reads as before (the tests find tabs by it) and nothing moves when a tab becomes due.
   const TFunction<bool()> Due=Sel?TFunction<bool()>():I.Due;
   auto Level=[Due]{return Due&&Due()?DueLevel(Now(),CalmNow()):-1.f;};
   const FLinearColor GreenNow=Green,AmberNow=Amber;
   const TSharedRef<SWidget> TabKey=SNew(SButton).ButtonStyle(Sel?&FilledStyle():&PlainStyle()).ContentPadding(FMargin(0,2,16,2)).OnHovered_Lambda([]{Hover();}).OnClicked_Lambda([Act]{Key(2);if(Act)Act();return FReply::Handled();})
     [SNew(SHorizontalBox)
      +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(16)[SNew(STextBlock).Text_Lambda([Level]{return FText::FromString(Level()<0?TEXT(""):TEXT("*"));}).Font(S47Style::Mono(26)).ColorAndOpacity(AmberNow)]]
      +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(I.Label)).Font(S47Style::Mono(26))
       .ColorAndOpacity_Lambda([Sel,Level,GreenNow,AmberNow]{if(Sel)return FSlateColor(Ink);const float L=Level();return FSlateColor(L<0?GreenNow:FMath::Lerp(GreenNow,AmberNow,.35f+.65f*L));})]];
   Box->AddSlot().AutoWidth().Padding(0,0,10,0)
    [SNew(SOverlay)
     +SOverlay::Slot()[TabKey]
     +SOverlay::Slot()[SNew(SBorder).Visibility(EVisibility::HitTestInvisible).BorderImage(Outline()).BorderBackgroundColor_Lambda([Level,AmberNow]{const float L=Level();FLinearColor C=AmberNow;C.A=L<0?0.f:.25f+.75f*L;return FSlateColor(C);})]];
  }
  return SNew(SVerticalBox)+SVerticalBox::Slot().AutoHeight()[Box]+SVerticalBox::Slot().AutoHeight().Padding(0,3,0,6)[Rule()];
 }
 FW Panel(const FString& Title,FW Body,bool bFilledHeader)
 {
  TSharedRef<SVerticalBox> Box=SNew(SVerticalBox);
  if(!Title.IsEmpty())
  {
   if(bFilledHeader)Box->AddSlot().AutoHeight()[SNew(SBorder).BorderImage(Fill()).BorderBackgroundColor(Green).Padding(FMargin(10,2))[SNew(STextBlock).Text(FText::FromString(Title)).Font(S47Style::Mono(26)).ColorAndOpacity(Ink)]];
   else
   {
    Box->AddSlot().AutoHeight().Padding(10,6,10,4)[SNew(STextBlock).Text(FText::FromString(Title)).Font(S47Style::Mono(30)).ColorAndOpacity(Green)];
    Box->AddSlot().AutoHeight().Padding(10,0)[Rule()];
   }
  }
  Box->AddSlot().AutoHeight().Padding(12,8)[Body];
  return SNew(SBorder).BorderImage(Outline()).BorderBackgroundColor(Line).Padding(2)[Box];
 }
 FW Heading(const FString& V,int32 Size){return SNew(STextBlock).Text(FText::FromString(V)).Font(S47Style::Mono(Size)).ColorAndOpacity(Amber).AutoWrapText(true).AddMetaData(MakeShared<FS47WrapMark>());}
 FW Banner(TAttribute<FString> V,TAttribute<bool> Urgent)
 {
  return SNew(SBorder).BorderImage(Outline()).BorderBackgroundColor_Lambda([Urgent]{return FSlateColor(Amber*(Urgent.Get()?Pulse(4):1.f));}).Padding(FMargin(14,8))
   [SNew(STextBlock).Text_Lambda([V]{return FText::FromString(V.Get());}).Font(S47Style::Mono(28)).ColorAndOpacity(Amber).Justification(ETextJustify::Center).AutoWrapText(true).AddMetaData(MakeShared<FS47WrapMark>())];
 }
 FW Big(TAttribute<FString> V,TFunction<void()> Action,int32 Kind,TAttribute<bool> Enabled)
 {
  const FButtonStyle* S=Kind==2?&FilledStyle():Kind==1?&PrimaryStyle():&OutlineStyle();
  return SNew(SButton).ButtonStyle(S).ContentPadding(FMargin(16,9)).HAlign(HAlign_Center).IsEnabled(Enabled).OnHovered_Lambda([]{Hover();}).OnClicked_Lambda([Action,Kind]{Key(Kind?1:0);if(Action)Action();return FReply::Handled();})
   [SNew(STextBlock).Text_Lambda([V]{return FText::FromString(V.Get());}).Font(S47Style::Mono(28)).Justification(ETextJustify::Center).AutoWrapText(true).AddMetaData(MakeShared<FS47WrapMark>())
    .ColorAndOpacity_Lambda([Kind,Enabled]{return FSlateColor(!Enabled.Get()?Dim*1.8f:Kind==2?Ink:Kind==1?Amber:Green);})];
 }
 FW Picture(UTexture2D* Image,float Width,const FString& Caption,bool bSelected,TFunction<void()> Action,bool bDue)
 {
  const TSharedPtr<FSlateBrush> B=S47Kit::BrushFor(Image);
  const float H=B.IsValid()?Width*B->ImageSize.Y/FMath::Max(1.,B->ImageSize.X):Width*.72f;
  const FW Face=B.IsValid()?FW(SNew(SImage).Image(B.Get()).ColorAndOpacity(FLinearColor(.46f,1.f,.60f)))
   :FW(SNew(SBorder).BorderImage(Outline()).BorderBackgroundColor(Dim*1.6f).HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("NO IMAGE"))).Font(S47Style::Mono(Width>200?24:16)).ColorAndOpacity(Dim*2.2f)]);
  TSharedRef<SVerticalBox> Col=SNew(SVerticalBox)+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(SBox).WidthOverride(Width).HeightOverride(H)[Face]];
  if(!Caption.IsEmpty())Col->AddSlot().AutoHeight().Padding(0,2,0,0)[SNew(STextBlock).Text(FText::FromString(Caption)).Font(S47Style::Mono(18)).ColorAndOpacity(bDue||bSelected?Amber:Green).Justification(ETextJustify::Center)];
  if(!Action)return Col;
  return SNew(SButton).ButtonStyle(bSelected?&PrimaryStyle():&OutlineStyle()).ContentPadding(FMargin(4,4)).HAlign(HAlign_Center).OnHovered_Lambda([]{Hover();}).OnClicked_Lambda([Action]{Key(2);Action();return FReply::Handled();})[Col];
 }
 FW Table(const TArray<FString>& Head,const TArray<float>& Widths,const TArray<FRow>& Rows,int32 Size)
 {
  auto Cells=[&Widths](const TArray<FString>& Values,FLinearColor C,int32 Size,const FString& Badge)
  {
   TSharedRef<SHorizontalBox> H=SNew(SHorizontalBox);
   for(int32 I=0;I<Values.Num();++I)H->AddSlot().FillWidth(Widths.IsValidIndex(I)?Widths[I]:1.f).Padding(6,0)[SNew(STextBlock).Text(FText::FromString(Values[I])).Font(S47Style::Mono(Size)).ColorAndOpacity(C)];
   if(!Badge.IsEmpty())H->AddSlot().AutoWidth().Padding(6,0)[SNew(SBorder).BorderImage(Outline()).BorderBackgroundColor(C).Padding(FMargin(6,0))[SNew(STextBlock).Text(FText::FromString(Badge)).Font(S47Style::Mono(Size-4)).ColorAndOpacity(C)]];
   return H;
  };
  TSharedRef<SVerticalBox> Box=SNew(SVerticalBox);
  if(!Head.IsEmpty()){Box->AddSlot().AutoHeight().Padding(12,6)[Cells(Head,Green,22,FString())];Box->AddSlot().AutoHeight()[Rule()];} // D48-20261110
  for(const FRow& R:Rows)
  {
   const TFunction<void()> Act=R.Action;
   Box->AddSlot().AutoHeight().Padding(0,2)
    [SNew(SButton).ButtonStyle(R.bSelected?&SelectedStyle():&PlainStyle()).ContentPadding(FMargin(6,7)).OnHovered_Lambda([]{Hover();}).OnClicked_Lambda([Act]{Key(2);if(Act)Act();return FReply::Handled();})[Cells(R.Cells,R.bSelected?Ink:Green,Size,R.Badge)]];
   Box->AddSlot().AutoHeight()[Rule(Dim)];
  }
  return SNew(SBorder).BorderImage(Outline()).BorderBackgroundColor(Line).Padding(2)[Box];
 }
 FW Value(const FString& Name,TAttribute<FString> Shown,bool bAmber)
 {
  return SNew(SHorizontalBox)
   +SHorizontalBox::Slot().FillWidth(1).Padding(0,3)[SNew(STextBlock).Text(FText::FromString(Name)).Font(S47Style::Mono(26)).ColorAndOpacity(Green)]
   +SHorizontalBox::Slot().AutoWidth().Padding(12,3,0,3)[SNew(STextBlock).Text_Lambda([Shown]{return FText::FromString(Shown.Get());}).Font(S47Style::Mono(26)).ColorAndOpacity(bAmber?Amber:Green)];
 }
 class SBar:public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SBar){} SLATE_ATTRIBUTE(float,Amount) SLATE_END_ARGS()
  void Construct(const FArguments& A){Amount=A._Amount;LineNow=Line;DoneNow=bReel?S47Kit::Css(0xa4d890):Green;ActiveNow=Amber;ForceVolatile(true);}
  virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(200,12);}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 L,const FWidgetStyle&,bool)const override
  {
   const float V=Amount.Get();if(V<0)return L;const FVector2f S(G.GetLocalSize());
   Rect(Out,L,G,FVector2f(0,0),S,LineNow,false);Rect(Out,L+1,G,FVector2f(2,2),FVector2f((S.X-4)*FMath::Clamp(V,0.f,1.f),S.Y-4),V>=.999f?DoneNow:ActiveNow,true);return L+2;
  }
 private:TAttribute<float> Amount;FLinearColor LineNow,DoneNow,ActiveNow; // copied when built, so a reel deck keeps its own colours
 };
 FW Meter(const FString& Name,TAttribute<FString> Shown,TAttribute<float> FillAmount)
 {
  return SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()
    [SNew(SHorizontalBox)
     +SHorizontalBox::Slot().FillWidth(1)[Label(Name,24,Green)]
     +SHorizontalBox::Slot().AutoWidth().Padding(12,0,0,0)[Label(Shown,24,Amber)]]
   +SVerticalBox::Slot().AutoHeight().Padding(0,2,0,6)[SNew(SBar).Amount(FillAmount)];
 }
 FW Display(const FString& Name,TAttribute<FString> Value,TAttribute<FString> State)
 {
  return SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()
    [SNew(SHorizontalBox)
     +SHorizontalBox::Slot().FillWidth(1)[Label(Name,26,Green)]
     +SHorizontalBox::Slot().AutoWidth().Padding(12,0,0,0)[Label(State,26,Amber)]]
   +SVerticalBox::Slot().AutoHeight()[Label(Value,44,Amber)];
 }
 FW Readout(const FString& Name,TAttribute<FString> Shown,TAttribute<float> FillAmount)
 {
  return SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Name)).Font(S47Style::Mono(26)).ColorAndOpacity(Green)]
   +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text_Lambda([Shown]{return FText::FromString(Shown.Get());}).Font(S47Style::Mono(34)).ColorAndOpacity(Amber)]
   +SVerticalBox::Slot().AutoHeight().Padding(0,2)[SNew(SBar).Amount(FillAmount)];
 }
 FW Columns(const TArray<TPair<float,FW>>& Cols,float Gap)
 {
  TSharedRef<SHorizontalBox> H=SNew(SHorizontalBox);
  float Sum=0;for(const auto& C:Cols)Sum+=FMath::Max(0.f,C.Key);
  for(int32 I=0;I<Cols.Num();++I)H->AddSlot().FillWidth(Cols[I].Key).Padding(I?Gap*.5f:0.f,0,I+1<Cols.Num()?Gap*.5f:0.f,0)[S47Kit::Share(Cols[I].Value,Sum>0?Cols[I].Key/Sum:1.f)];
  return H;
 }

 FW Split(const TArray<TPair<float,FW>>& Cols)
 {
  TSharedRef<SHorizontalBox> H=SNew(SHorizontalBox);
  float Sum=0;for(const auto& C:Cols)Sum+=FMath::Max(0.f,C.Key);
  for(int32 I=0;I<Cols.Num();++I)
  {
   if(I)H->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(2)[SNew(SBorder).BorderImage(Fill()).BorderBackgroundColor(Line)]];
   H->AddSlot().FillWidth(Cols[I].Key).Padding(14,6).VAlign(VAlign_Center)[S47Kit::Share(Cols[I].Value,Sum>0?Cols[I].Key/Sum:1.f)];
  }
  return SNew(SBorder).BorderImage(Outline()).BorderBackgroundColor(Line).Padding(2)[H];
 }
 FW TitleRow(const FString& Left,TAttribute<FString> Right)
 {
  return SNew(SHorizontalBox)
   +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Left)).Font(S47Style::Mono(26)).ColorAndOpacity(Green)]
   +SHorizontalBox::Slot().FillWidth(1)
   +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text_Lambda([Right]{return FText::FromString(Right.Get());}).Font(S47Style::Mono(20)).ColorAndOpacity(Green)];
 }
 // --- Live plots ---
 class SSpectrum:public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SSpectrum){} SLATE_END_ARGS()
  TFunction<FS47Rx()> Rx;TFunction<FS47Profile()> Card;TFunction<double()> Q;
  void Construct(const FArguments&){ForceVolatile(true);}
  virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(1100,250);}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 L,const FWidgetStyle&,bool)const override
  {
   const FVector2f S(G.GetLocalSize());const float X0=66,Y0=42,W=S.X-X0-10,H=S.Y-Y0-28;
   const FS47Rx R=Rx?Rx():FS47Rx();const FS47Profile P=Card?Card():R.Profile();const double Quality=R.bPowered&&Q?FMath::Clamp(Q(),0.,1.):0.;
   auto Fx=[&](double F){return X0+float((F-S47Signal::FMin)/(S47Signal::FMax-S47Signal::FMin))*W;};
   // dB grid and labels, frequency ticks every 0.1 MHz, labels every 0.2 MHz.
   for(int32 I=0;I<=4;++I){const float Y=Y0+H*I/4;Lines(Out,L,G,{{X0,Y},{X0+W,Y}},Dim,1);Text2(Out,L,G,FString::Printf(TEXT("%d"),-110-5*I),4,Y-11,20,Green*.85f);}
   Text2(Out,L,G,TEXT("dB"),18,Y0-34,20,Green*.85f);
   for(int32 I=0;I<=12;++I)
   {
    const float X=X0+W*I/12;Lines(Out,L,G,{{X,Y0},{X,Y0+H}},Dim,1);
    if(I%2==0)Text2(Out,L,G,FString::Printf(TEXT("%.3f"),S47Signal::FMin+.1*I),X-(I==12?62:30),Y0+H+5,16,Green*.85f);
   }
   Lines(Out,L,G,{{X0,Y0},{X0,Y0+H},{X0+W,Y0+H}},Line,1.5f);
   if(!R.bPowered){Text2(Out,L+2,G,TEXT("NO POWER / RECEIVER OFF"),X0+W*.36f,Y0+H*.4f,28,Amber);return L+3;}
   const double T=FPlatformTime::Seconds();const float Tune=Fx(R.Frequency),Peak=Fx(P.F);
   const float Half=FMath::Max(2.f,float(R.Bandwidth/1000/(S47Signal::FMax-S47Signal::FMin)*W*.5));
   const float B0=FMath::Max(X0,Tune-Half),B1=FMath::Min(X0+W,Tune+Half);
   if(B1>B0)FSlateDrawElement::MakeBox(Out,L+1,G.ToPaintGeometry(FVector2f(B1-B0,H),FSlateLayoutTransform(FVector2f(B0,Y0))),Fill(),ESlateDrawEffect::None,FLinearColor(.1f,.5f,.2f,.12f));
   TArray<FVector2f> Trace;Trace.Reserve(361);
   for(int32 I=0;I<=360;++I)
   {
    const double X=X0+W*I/360.,Noise=(FMath::Sin(X*.17+T*4)*2+FMath::Sin(X*.71+T*7)*3+FMath::Sin(I*12.9898+T*11)*1.5)*(R.Bandwidth/35+.2)*H/220;
    const double Amp=(10+120*Quality*R.Gain/100)*H/200;
    const double Y=Y0+H*.80-Noise-Amp*FMath::Exp(-FMath::Square((X-Peak)/FMath::Max(1.,6*W/512.)))-10*H/200*FMath::Exp(-FMath::Square((X-Fx(1420.406))/FMath::Max(1.,40*W/512.)));
    Trace.Add(FVector2f(float(X),FMath::Clamp(float(Y),Y0+2,Y0+H-2)));
   }
   Lines(Out,L+1,G,Trace,FLinearColor(.25f,.8f,.35f,.18f),4);Lines(Out,L+2,G,Trace,Green,1.6f);
   const bool Close=FMath::Abs(Tune-Peak)<150;
   if(Peak>=X0&&Peak<=X0+W)
   {
    Dashed(Out,L+2,G,{Peak,Y0},{Peak,Y0+H},Green,6,2);
    const float LX=Close&&Peak<Tune?Peak-110:Peak+8;
    Text2(Out,L+2,G,TEXT("TARGET"),LX,Y0-42,20,Green);Text2(Out,L+2,G,FString::Printf(TEXT("%.3f"),P.F),LX,Y0-22,20,Green);
   }
   if(Tune>=X0&&Tune<=X0+W)
   {
    Lines(Out,L+3,G,{{Tune,Y0},{Tune,Y0+H}},Amber,2.5f);
    const float LX=Close&&Peak>=Tune?Tune-110:Tune+8;
    Text2(Out,L+3,G,TEXT("TUNED"),LX,Y0-42,20,Amber);Text2(Out,L+3,G,FString::Printf(TEXT("%.3f"),R.Frequency),LX,Y0-22,20,Amber);
   }
   return L+4;
  }
 };
 FW Spectrum(TFunction<FS47Rx()> Receiver,TFunction<FS47Profile()> Card,TFunction<double()> Quality)
 {const auto W=SNew(SSpectrum);W->Rx=Receiver;W->Card=Card;W->Q=Quality;return SNew(SBox).HeightOverride(170)[W];}


 // D48-20261110 (D46-14, Tom: TUNE must have something to do): the spectrum zoomed on the target. The window is a few times the card's
 // tolerance wide, so turning the frequency dial one step moves the amber marker visibly; the peak rises as the carrier lock does.
 class STuneZoom:public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(STuneZoom){} SLATE_END_ARGS()
  TFunction<FS47Rx()> Rx;TFunction<FS47Profile()> Card;TFunction<double()> Q;
  void Construct(const FArguments&){ForceVolatile(true);}
  virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(1100,190);}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 L,const FWidgetStyle&,bool)const override
  {
   const FVector2f S(G.GetLocalSize());const float X0=60,Y0=26,W=S.X-X0-10,H=S.Y-Y0-28;
   const FS47Rx R=Rx?Rx():FS47Rx();const FS47Profile P=Card?Card():R.Profile();const double Quality=R.bPowered&&Q?FMath::Clamp(Q(),0.,1.):0.;
   const double Half=FMath::Max(.04,P.FTol*5),Lo=P.F-Half;
   auto Fx=[&](double F){return X0+float((F-Lo)/(2*Half))*W;};
   for(int32 I=0;I<=3;++I){const float Y=Y0+H*I/3;Lines(Out,L,G,{{X0,Y},{X0+W,Y}},Dim,1);}
   for(int32 I=0;I<=8;++I)
   {
    const float X=X0+W*I/8;Lines(Out,L,G,{{X,Y0},{X,Y0+H}},I==4?Line:Dim,1);
    if(I%2==0)Text2(Out,L,G,FString::Printf(TEXT("%.3f"),Lo+2*Half*I/8),X-(I==8?62:I==0?0:30),Y0+H+5,16,Green*.85f);
   }
   Lines(Out,L,G,{{X0,Y0},{X0,Y0+H},{X0+W,Y0+H}},Line,1.5f);
   Text2(Out,L,G,TEXT("ZOOM"),6,Y0-6,18,Green*.85f);
   if(!R.bPowered){Text2(Out,L+2,G,TEXT("NO POWER / RECEIVER OFF"),X0+W*.36f,Y0+H*.4f,28,Amber);return L+3;}
   const double T=FPlatformTime::Seconds();const float Peak=Fx(P.F),Tune=Fx(R.Frequency);
   const float Sigma=FMath::Max(3.f,float(P.FTol/(2*Half)*W*.8));
   const float Bw=FMath::Clamp(float(R.Bandwidth/1000./(2*Half)*W),2.f,W);
   const float B0=FMath::Clamp(Tune-Bw*.5f,X0,X0+W),B1=FMath::Clamp(Tune+Bw*.5f,X0,X0+W);
   if(B1>B0)FSlateDrawElement::MakeBox(Out,L+1,G.ToPaintGeometry(FVector2f(B1-B0,H),FSlateLayoutTransform(FVector2f(B0,Y0))),Fill(),ESlateDrawEffect::None,FLinearColor(.1f,.5f,.2f,.12f));
   TArray<FVector2f> Trace;Trace.Reserve(241);
   for(int32 I=0;I<=240;++I)
   {
    const double X=X0+W*I/240.,Noise=(FMath::Sin(X*.31+T*5)*2+FMath::Sin(X*.97+T*9)*3+FMath::Sin(I*12.9898+T*13)*1.5)*(R.Bandwidth/35+.2)*H/220;
    // The peak is what the dishes hear, so it follows the lock; a dial far off it hardly shows, one on it stands up.
    const double Amp=(8+130*Quality*(.35+.65*R.Gain/100))*H/200;
    const double Y=Y0+H*.82-Noise-Amp*FMath::Exp(-FMath::Square((X-Peak)/Sigma));
    Trace.Add(FVector2f(float(X),FMath::Clamp(float(Y),Y0+2,Y0+H-2)));
   }
   Lines(Out,L+1,G,Trace,FLinearColor(.25f,.8f,.35f,.18f),4);Lines(Out,L+2,G,Trace,Green,1.6f);
   Dashed(Out,L+2,G,{Peak,Y0},{Peak,Y0+H},Green,6,2);Text2(Out,L+2,G,TEXT("TARGET"),FMath::Min(Peak+8,X0+W-90),Y0-24,18,Green);
   const bool In=Tune>=X0&&Tune<=X0+W;
   if(In)
   {
    Lines(Out,L+3,G,{{Tune,Y0},{Tune,Y0+H}},Amber,2.5f);
    Text2(Out,L+3,G,FString::Printf(TEXT("TUNED %.3f"),R.Frequency),FMath::Clamp(Tune+8,X0,X0+W-140),Y0+H-22,18,Amber);
   }
   else Text2(Out,L+3,G,R.Frequency<Lo?TEXT("<< TUNED BELOW THIS WINDOW"):TEXT("TUNED ABOVE THIS WINDOW >>"),R.Frequency<Lo?X0+8:X0+W-330,Y0+8,20,Amber);
   return L+4;
  }
 };
 FW TuneZoom(TFunction<FS47Rx()> Receiver,TFunction<FS47Profile()> Card,TFunction<double()> Quality,float Height)
 {const auto W=SNew(STuneZoom);W->Rx=Receiver;W->Card=Card;W->Q=Quality;return SNew(SBox).HeightOverride(Height)[W];}
 // D48-20261110 (D46-14): a grid of tube-green cells, time across and low frequency at the foot. Brightest cells go amber.
 class SSpectrogram:public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SSpectrogram){} SLATE_END_ARGS()
  TFunction<float(float,float)> Sample;TFunction<bool()> Has;
  void Construct(const FArguments&){ForceVolatile(true);}
  virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(560,120);}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 L,const FWidgetStyle&,bool)const override
  {
   const FVector2f S(G.GetLocalSize());Rect(Out,L,G,FVector2f(0,0),S,Line,false);
   if(!Has||!Has()||!Sample){Text2(Out,L+1,G,TEXT("NOTHING RECORDED YET"),S.X*.5f-130,S.Y*.5f-12,22,Amber);return L+2;}
   constexpr int32 Cols=56,Rows=12;const float Cw=(S.X-4)/Cols,Ch=(S.Y-4)/Rows;
   for(int32 C=0;C<Cols;++C)for(int32 Rw=0;Rw<Rows;++Rw)
   {
    const float V=FMath::Clamp(Sample((C+.5f)/Cols,1.f-(Rw+.5f)/Rows),0.f,1.f);if(V<.05f)continue;
    const FLinearColor Base=V>.85f?Amber:Green;
    FSlateDrawElement::MakeBox(Out,L+1,G.ToPaintGeometry(FVector2f(Cw+.5f,Ch+.5f),FSlateLayoutTransform(FVector2f(2+C*Cw,2+Rw*Ch))),Fill(),ESlateDrawEffect::None,FLinearColor(Base.R,Base.G,Base.B,.15f+.85f*V));
   }
   return L+2;
  }
 };
 FW Spectrogram(TFunction<float(float,float)> Sample,TFunction<bool()> Has,float Height)
 {const auto W=SNew(SSpectrogram);W->Sample=Sample;W->Has=Has;return SNew(SBox).HeightOverride(Height)[W];}

 class SSkyPlot:public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SSkyPlot){} SLATE_END_ARGS()
  TFunction<FVector2D()> Target,Current;FString Source;
  // Freeform: the marker the player drags. Without these the plot only shows the target.
  TFunction<void(FVector2D)> OnAim;TFunction<bool()> OnTarget;
  mutable FVector2D Shown=FVector2D(-1,-1);mutable double LastPaint=0;
  // D45-20261110 (Tom: no green flash while hunting with the mouse; a pointer and a sound that say you are getting closer).
  // Over the plot the tube draws its own cursor: a wide, thin frame far from the source that closes into a small bright square
  // as the pointer nears it, amber on it, while a quiet search tick quickens and rises. Freeform only (OnAim set).
  FVector2f HoverLocal=FVector2f::ZeroVector;FVector2D HoverAzEl=FVector2D::ZeroVector;double MovedAt=-10;mutable double NextProbe=0;
  void Construct(const FArguments&){ForceVolatile(true);}
  virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(560,360);}
  FVector2D AzElAt(const FGeometry& G,const FVector2D& Screen)const
  {
   const FVector2f S(G.GetLocalSize());const float R=FMath::Min(S.X*.5f-26,S.Y-96);const FVector2f C(S.X*.5f,S.Y-44);
   const FVector2f V=FVector2f(G.AbsoluteToLocal(Screen))-C;const double Rr=FMath::Min(double(V.Size()),double(R));
   const double Th=V.Y>0?(V.X>0?0.:PI):FMath::Atan2(double(-V.Y),double(V.X));
   return FVector2D(180.*(1-Th/PI),90.*(1-Rr/FMath::Max(1.,double(R))));
  }
  void AimFrom(const FGeometry& G,const FVector2D& Screen)const{OnAim(AzElAt(G,Screen));}
  void Track(const FGeometry& G,const FVector2D& Screen){HoverLocal=FVector2f(G.AbsoluteToLocal(Screen));HoverAzEl=AzElAt(G,Screen);MovedAt=Now();}
  float Near()const{return NearSource?FMath::Clamp(float(NearSource(HoverAzEl)),0.f,1.f):0.f;}
  virtual FReply OnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)override
  {if(!OnAim||E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();Key(3);Track(G,E.GetScreenSpacePosition());AimFrom(G,E.GetScreenSpacePosition());return FReply::Handled();}
  // A drag is heard through the search tick, never flashed and never keyed on every move.
  virtual FReply OnMouseMove(const FGeometry& G,const FPointerEvent& E)override
  {
   if(!OnAim)return FReply::Unhandled();Track(G,E.GetScreenSpacePosition());
   if(!E.IsMouseButtonDown(EKeys::LeftMouseButton))return FReply::Unhandled();AimFrom(G,E.GetScreenSpacePosition());return FReply::Handled();
  }
  virtual FCursorReply OnCursorQuery(const FGeometry&,const FPointerEvent&)const override
  {return OnAim&&IsHovered()?FCursorReply::Cursor(EMouseCursor::None):FCursorReply::Unhandled();}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 L,const FWidgetStyle&,bool)const override
  {
   // Half-sky plot: zenith at the centre of the base line, horizon on the rim,
   // azimuth 0 to 180 degrees around the arc, elevation rings at 0, 30 and 60 degrees.
   const FVector2f S(G.GetLocalSize());const float R=FMath::Min(S.X*.5f-26,S.Y-96);const FVector2f C(S.X*.5f,S.Y-44);
   auto At=[&](FVector2D AzEl){const double Az=FMath::Clamp(AzEl.X,0.,180.),El=FMath::Clamp(AzEl.Y,0.,90.);const double Th=PI*(1-Az/180.),Rr=R*(90-El)/90;return C+FVector2f(float(FMath::Cos(Th)*Rr),float(-FMath::Sin(Th)*Rr));};
   for(const double El:{0.,30.,60.})
   {
    TArray<FVector2f> Arc;for(int32 I=0;I<=60;++I)Arc.Add(At({180.*I/60,El}));Lines(Out,L,G,Arc,El==0?Line:Dim,El==0?2.f:1.f);
    const FVector2f P=At({90,El});Text2(Out,L,G,FString::Printf(TEXT("%d"),int32(El)),P.X+6,P.Y-2,18,Green*.7f);
   }
   for(int32 Az=0;Az<=180;Az+=30)
   {
    const FVector2f P=At({double(Az),0});Lines(Out,L,G,{C,P},Dim,1);const FVector2f D=(P-C).GetSafeNormal();
    Text2(Out,L,G,FString::Printf(TEXT("%d"),Az),P.X+D.X*18-12,P.Y+D.Y*18-10,18,Green*.85f);
   }
   Lines(Out,L,G,{C-FVector2f(R+8,0),C+FVector2f(R+8,0)},Line,2);
   Text2(Out,L,G,TEXT("AZ"),C.X+R-10,C.Y+12,18,Green*.85f);Text2(Out,L,G,TEXT("ZENITH"),C.X-30,C.Y+12,18,Green*.7f);
   const double T=FPlatformTime::Seconds();const double Dt=LastPaint>0?FMath::Clamp(T-LastPaint,0.,.25):.25;LastPaint=T;
   const FVector2D Want=Current?Current():FVector2D(0,0),Aim=Target?Target():FVector2D(0,0);
   if(Shown.X<0)Shown=Want;Shown+=(Want-Shown)*FMath::Min(1.,Dt*8.);
   const FVector2f Tp=At(Aim),Cp=At(Shown);
   if(FVector2D::Distance(Shown,Aim)>.3)Dashed(Out,L+1,G,Cp,Tp,Amber*.6f,5,1.5f);
   if(OnTarget&&OnTarget())
   {
    const float Ring=16+10*Pulse(3);TArray<FVector2f> Circle;
    for(int32 I=0;I<=32;++I){const float A=2*PI*I/32;Circle.Add(Tp+FVector2f(FMath::Cos(A),FMath::Sin(A))*Ring);}
    Lines(Out,L+2,G,Circle,Amber,2.f);
   }
   const float Arm=9+3*Pulse(4);
   Lines(Out,L+2,G,{Tp-FVector2f(Arm,0),Tp+FVector2f(Arm,0)},Amber,2.5f);Lines(Out,L+2,G,{Tp-FVector2f(0,Arm),Tp+FVector2f(0,Arm)},Amber,2.5f);
   Rect(Out,L+2,G,Tp-FVector2f(4,4),FVector2f(8,8),Amber,false);
   Rect(Out,L+3,G,Cp-FVector2f(8,8),FVector2f(16,16),Green,false);Lines(Out,L+3,G,{Cp-FVector2f(5,5),Cp+FVector2f(5,5)},Green,1.5f);
   // D45-20261110: the operator's own cursor and the search tick, while the pointer is on the plot
   if(OnAim&&IsHovered()&&MovedAt>0) // (not before the pointer has moved: HoverLocal is still the corner)
   {
    const float N=Near();const bool bOn=N>=1;const FLinearColor Col=bOn?Amber:Green*FMath::Lerp(.65f,1.f,N);
    const float Gap=FMath::Lerp(34.f,7.f,N*N),Leg=FMath::Lerp(9.f,5.f,N),W=FMath::Lerp(1.5f,2.6f,N);const FVector2f P=HoverLocal;
    for(const FVector2f Q:{FVector2f(-1,-1),FVector2f(1,-1),FVector2f(1,1),FVector2f(-1,1)})
    {const FVector2f K=P+Q*Gap;Lines(Out,L+4,G,{K-FVector2f(Q.X*Leg,0),K,K-FVector2f(0,Q.Y*Leg)},Col,W);}
    Rect(Out,L+4,G,P-FVector2f(1.5f,1.5f),FVector2f(3,3),Col,true);
    // slow ticks far off, a fast chatter close in; only while the hand moves, so a resting pointer is quiet
    if(T>NextProbe+1)NextProbe=T;
    if(T-MovedAt<.6&&ProbeSink&&T>=NextProbe){ProbeSink(N);NextProbe=T+FMath::Lerp(.46,.07,FMath::Pow(double(N),1.6));}
   }
   Lines(Out,L+1,G,{{10,14},{26,14}},Amber,2.5f);Lines(Out,L+1,G,{{18,6},{18,22}},Amber,2.5f);Text2(Out,L+1,G,OnAim?FString(TEXT("YOUR MARKER")):TEXT("TARGET ")+Source.Left(12),34,3,18,Green);
   Rect(Out,L+1,G,FVector2f(12,32),FVector2f(12,12),Green,false);Text2(Out,L+1,G,TEXT("ARRAY (CURRENT)"),34,28,18,Green);
   return L+5;
  }
 };
 FW SkyPlot(TFunction<FVector2D()> Target,TFunction<FVector2D()> Current,const FString& Source,TFunction<void(FVector2D)> OnAim,TFunction<bool()> OnTarget)
 {const auto W=SNew(SSkyPlot);W->Target=Target;W->Current=Current;W->Source=Source;W->OnAim=OnAim;W->OnTarget=OnTarget;return SNew(SBox).HeightOverride(400)[W];}

 class SFivePoint:public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SFivePoint){} SLATE_END_ARGS()
  TFunction<int32()> Done;TFunction<bool()> Running,Required;
  void Construct(const FArguments&){ForceVolatile(true);}
  virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(300,230);}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 L,const FWidgetStyle&,bool)const override
  {
   const FVector2f S(G.GetLocalSize());const FVector2f C(S.X*.5f,S.Y*.5f+4);const float Dx=S.X*.36f,Dy=S.Y*.33f;
   const bool Need=!Required||Required();const int32 N=Done?FMath::Clamp(Done(),0,5):0;const bool Run=Running&&Running();
   // Measurement order of the session's calibration cross: north, south, east, west, centre.
   const FVector2f P[5]={C+FVector2f(0,-Dy),C+FVector2f(0,Dy),C+FVector2f(Dx,0),C+FVector2f(-Dx,0),C};
   const TCHAR* Names[5]={TEXT("NORTH"),TEXT("SOUTH"),TEXT("EAST"),TEXT("WEST"),TEXT("CENTRE")};
   const FLinearColor Base=Need?Green:Dim*1.6f;
   Dashed(Out,L,G,P[3],P[2],Base*.7f,6,1.5f);Dashed(Out,L,G,P[0],P[1],Base*.7f,6,1.5f);
   for(int32 I=0;I<5;++I)
   {
    const bool Filled=I<N,Active=Run&&I==N;const float Sz=Active?16+4*Pulse(6):16;
    Rect(Out,L+1,G,P[I]-FVector2f(Sz*.5f,Sz*.5f),FVector2f(Sz,Sz),Filled||Active?Amber:Base,Filled);
    const float Ly=I==0?P[I].Y-34:P[I].Y+12;const float Lx=P[I].X-float(FCString::Strlen(Names[I]))*5;
    Text2(Out,L+1,G,Names[I],Lx,Ly,18,Base);
   }
   if(!Need)Text2(Out,L+2,G,TEXT("NOT REQUIRED"),C.X-60,2,18,Amber*.8f);
   return L+3;
  }
 };
 FW FivePoint(TFunction<int32()> Done,TFunction<bool()> Running,TFunction<bool()> Required)
 {const auto W=SNew(SFivePoint);W->Done=Done;W->Running=Running;W->Required=Required;return SNew(SBox).HeightOverride(230)[W];}

 class SAntennaCell:public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SAntennaCell){} SLATE_END_ARGS()
  TFunction<FString()> Label;TFunction<int32()> State;TFunction<void()> Click;
  void Construct(const FArguments&){ForceVolatile(true);}
  virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(64,64);}
  virtual FReply OnMouseButtonDown(const FGeometry&,const FPointerEvent& E)override{if(E.GetEffectingButton()==EKeys::LeftMouseButton&&Click){Key(0);Click();return FReply::Handled();}return FReply::Unhandled();}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 L,const FWidgetStyle&,bool)const override
  {
   const FVector2f S(G.GetLocalSize());const int32 St=State?State():0;
   const FLinearColor C=St==1?Amber:St==2?Red:St==3?Green*Pulse(7):Green;const bool Filled=St==1||St==2;
   Rect(Out,L,G,FVector2f(2,2),S-FVector2f(4,4),C,Filled);if(Filled)Rect(Out,L+1,G,FVector2f(2,2),S-FVector2f(4,4),C,false);
   const FString T=Label?Label():FString();const float Tw=float(T.Len())*12.f;
   Text2(Out,L+2,G,T,(S.X-Tw)*.5f,(S.Y-30)*.5f,26,Filled?Ink:C);
   return L+3;
  }
 };
 FW AntennaGrid(int32 Count,TFunction<FString(int32)> LabelOf,TFunction<int32(int32)> State,TFunction<void(int32)> Pick)
 {
  TSharedRef<SUniformGridPanel> Grid=SNew(SUniformGridPanel).SlotPadding(FMargin(4));
  for(int32 I=0;I<Count;++I)
  {
   const auto Cell=SNew(SAntennaCell);Cell->Label=[LabelOf,I]{return LabelOf?LabelOf(I):FString::Printf(TEXT("S%02d"),I+1);};Cell->State=[State,I]{return State?State(I):0;};
   Cell->Click=[Pick,I]{if(Pick)Pick(I);};
   Grid->AddSlot(I%9,I/9).HAlign(HAlign_Fill).VAlign(VAlign_Fill)[SNew(SBox).MinDesiredWidth(64).HeightOverride(66)[Cell]];
  }
  return Grid;
 }
 // D45-20261110: the reel deck's faceplate (see the header). The header row is Frame's own, kept apart so that the glass's frame
 // can change without the deck; the body sits in a clipping box of the faceplate's height and never scrolls.
 FW Faceplate(const FString& Title,FW Body,TSharedPtr<SWidget> Footer)
 {
  const TFunction<FString()> Clock=ClockSource;
  const FString Station=StationSource.IsEmpty()?FString(TEXT("SARO / LOCAL WORKSTATION")):StationSource;
  auto Bar=[]{return SNew(STextBlock).Text(FText::FromString(TEXT("|"))).Font(S47Style::Mono(26)).ColorAndOpacity(Green);};
  S47Kit::FirstFrameWrap(Body,1176.f);
  return SNew(SBorder).BorderImage(Fill()).BorderBackgroundColor(Black).Padding(FMargin(12,6))
  [SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()
    [SNew(SHorizontalBox)
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Label(Station,22,Green)]
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14,0)[Bar()]
     +SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Center).VAlign(VAlign_Center)[Label(Title,30,Amber)]
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14,0)[Bar()]
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Label(TAttribute<FString>::CreateLambda([Clock]{return Clock?Clock():FString(TEXT("SIGNAL / 47"));}),24,Green)]]
   +SVerticalBox::Slot().AutoHeight().Padding(0,4,0,8)[Rule(Green)]
   +SVerticalBox::Slot().FillHeight(1)[SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)[Body]]
   +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)[Footer.IsValid()?Footer.ToSharedRef():SNullWidget::NullWidget]];
 }
 FW Lamp(TAttribute<FLinearColor> Lit,float Size)
 {return SNew(SBox).WidthOverride(Size).HeightOverride(Size)[SNew(SBorder).BorderImage(Solid()).BorderBackgroundColor_Lambda([Lit]{return FSlateColor(Lit.Get());})];}
 FW Note(TAttribute<FString> Value,int32 Size,TAttribute<FLinearColor> Colour,bool bWrap)
 {
  const TSharedRef<STextBlock> Block=SNew(STextBlock).Text_Lambda([Value]{return FText::FromString(Value.Get());}).Font(S47Style::Mono(Size)).ColorAndOpacity_Lambda([Colour]{return FSlateColor(Colour.Get());}).AutoWrapText(bWrap);
  if(bWrap)Block->AddMetadata(MakeShared<FS47WrapMark>());
  return Block;
 }
}

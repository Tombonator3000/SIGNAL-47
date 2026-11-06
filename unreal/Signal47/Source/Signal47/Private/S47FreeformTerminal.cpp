#include "S47FreeformTerminal.h"
#include "S47Kit.h"
#include "SS47Widgets.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

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
  double Now(){return FPlatformTime::Seconds();}
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
  const FButtonStyle& FilledStyle(){static const FButtonStyle S=MakeStyle(Green,S47Kit::Css(0xb4ffc4),S47Kit::Css(0x6fdc88),Green,2.f);return S;}
  const FButtonStyle& PlainStyle(){static const FButtonStyle S=MakeStyle(FLinearColor::Transparent,S47Kit::Css(0x0e3519),S47Kit::Css(0x1b5a2c),FLinearColor::Transparent,0.f);return S;}
  const FButtonStyle& SelectedStyle(){static const FButtonStyle S=MakeStyle(Amber,S47Kit::Css(0xffcc73),S47Kit::Css(0xe09a30),Amber,2.f);return S;}
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
 {return SNew(STextBlock).Text_Lambda([Value]{return FText::FromString(Value.Get());}).Font(S47Style::Mono(24)).ColorAndOpacity_Lambda([Tone,AmberNow=Amber,GreenNow=Green]{return FSlateColor(Tone.Get()<0?AmberNow:GreenNow);}).AutoWrapText(true).Margin(FMargin(0,5));}
 FW Text(const FString& Value,int32 Tone){return Text(TAttribute<FString>(Value),TAttribute<int32>(Tone));}
 FW Paper(const FString& Value){return Text(Value);}
 FW Small(const FString& Value){return SNew(STextBlock).Text(FText::FromString(Value)).Font(S47Style::Mono(20)).ColorAndOpacity(Green*.8f).AutoWrapText(true).Margin(FMargin(0,6));}
 FW ButtonLive(TAttribute<FString> Text,TFunction<void()> Action,bool Primary,TAttribute<bool> Enabled)
 {
  return SNew(SButton).ButtonStyle(Primary?&PrimaryStyle():&OutlineStyle()).ContentPadding(FMargin(12,7)).IsEnabled(Enabled).OnClicked_Lambda([Action]{if(Action)Action();return FReply::Handled();})
  [SNew(STextBlock).Text_Lambda([Text]{return FText::FromString(Text.Get());}).Font(S47Style::Mono(24)).ColorAndOpacity_Lambda([Primary,Enabled,AmberNow=Amber,GreenNow=Green,DimNow=Dim]{return FSlateColor(!Enabled.Get()?DimNow*1.6f:Primary?AmberNow:GreenNow);}).AutoWrapText(true)];
 }
 FW Toggle(const FString& Text,TAttribute<bool> On,TFunction<void()> Action,TAttribute<bool> Enabled)
 {
  const TAttribute<EVisibility> Off=TAttribute<EVisibility>::CreateLambda([On]{return On.Get()?EVisibility::Collapsed:EVisibility::Visible;});
  const TAttribute<EVisibility> Lit=TAttribute<EVisibility>::CreateLambda([On]{return On.Get()?EVisibility::Visible:EVisibility::Collapsed;});
  return SNew(SOverlay)
   +SOverlay::Slot()[SNew(SBox).Visibility(Off)[Button(Text,Action,false,Enabled)]]
   +SOverlay::Slot()[SNew(SBox).Visibility(Lit)
    [SNew(SButton).ButtonStyle(bReel?&ReelFilled():&FilledStyle()).ContentPadding(FMargin(12,7)).IsEnabled(Enabled).OnClicked_Lambda([Action]{if(Action)Action();return FReply::Handled();})
     [SNew(STextBlock).Text(FText::FromString(Text)).Font(S47Style::Mono(24)).ColorAndOpacity(Ink)]]];
 }
 FW Filled(const FString& Text,TFunction<void()> Action,TAttribute<bool> Enabled)
 {
  return SNew(SButton).ButtonStyle(&SelectedStyle()).ContentPadding(FMargin(12,7)).HAlign(HAlign_Center).IsEnabled(Enabled).OnClicked_Lambda([Action]{if(Action)Action();return FReply::Handled();})
   [SNew(STextBlock).Text(FText::FromString(Text)).Font(S47Style::Mono(26)).ColorAndOpacity_Lambda([Enabled,DimNow=Dim]{return FSlateColor(Enabled.Get()?Ink:DimNow*1.6f);})];
 }
 FW Button(const FString& Text,TFunction<void()> Action,bool Primary,TAttribute<bool> Enabled){return ButtonLive(TAttribute<FString>(Text),Action,Primary,Enabled);}
 FW Pair(FW Left,FW Right)
 {return SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(1).Padding(0,0,12,0)[Left]+SHorizontalBox::Slot().FillWidth(1)[Right];}
 FW Control(const FString& Name,TAttribute<FString> Shown,float Min,float Max,float Step,TAttribute<float> Value,TFunction<void(float)> OnChange)
 {
  auto Nudge=[Value,OnChange,Min,Max](float By){if(OnChange)OnChange(FMath::Clamp(Value.Get()+By,Min,Max));return FReply::Handled();};
  auto Key=[](const TCHAR* Glyph){return SNew(STextBlock).Text(FText::FromString(Glyph)).Font(S47Style::Mono(30)).ColorAndOpacity(Green).Justification(ETextJustify::Center);};
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
     +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(44).HeightOverride(40)[SNew(SButton).ButtonStyle(&OutlineStyle()).ContentPadding(0).HAlign(HAlign_Center).VAlign(VAlign_Center).OnClicked_Lambda([Nudge,Step]{return Nudge(-Step);})[Key(TEXT("-"))]]]
     +SHorizontalBox::Slot().FillWidth(1).Padding(10,0)
      [SNew(SBox).HeightOverride(40).VAlign(VAlign_Fill)
       [SNew(SSlider).Style(&KnobStyle()).MinValue(Min).MaxValue(Max).StepSize(Step).MouseUsesStep(true).Value(Value)
        .SliderBarColor(Line).SliderHandleColor(Amber).OnValueChanged_Lambda([OnChange](float V){if(OnChange)OnChange(V);})]]
     +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(44).HeightOverride(40)[SNew(SButton).ButtonStyle(&OutlineStyle()).ContentPadding(0).HAlign(HAlign_Center).VAlign(VAlign_Center).OnClicked_Lambda([Nudge,Step]{return Nudge(Step);})[Key(TEXT("+"))]]]]];
 }
 FW Knob(const FString& Name,TAttribute<FString> Shown,float Min,float Max,float Step,TAttribute<float> Value,TFunction<void(float)> OnChange)
 {return Control(Name,Shown,Min,Max,Step,Value,OnChange);}
 void SetClock(TFunction<FString()> Clock){ClockSource=MoveTemp(Clock);}
 FW Frame(const FString& Title,FW Body,TSharedPtr<SWidget> Footer)
 {
  // One header row as in the mockups: station | screen title | day and clock.
  const TFunction<FString()> Clock=ClockSource;
  auto Bar=[]{return SNew(STextBlock).Text(FText::FromString(TEXT("|"))).Font(S47Style::Mono(26)).ColorAndOpacity(Green);};
  return SNew(SBorder).BorderImage(Fill()).BorderBackgroundColor(Black).Padding(FMargin(12,6))
  [SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()
    [SNew(SHorizontalBox)
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Label(FString(TEXT("SARO / LOCAL WORKSTATION")),22,Green)]
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14,0)[Bar()]
     +SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Center).VAlign(VAlign_Center)
      [SNew(SHorizontalBox)
       +SHorizontalBox::Slot().AutoWidth()[Label(Title,30,Amber)]
       // A slow block cursor: the station is live even while nothing is selected.
       +SHorizontalBox::Slot().AutoWidth().Padding(4,0,0,0)[SNew(STextBlock).Text(FText::FromString(TEXT("_"))).Font(S47Style::Mono(30)).ColorAndOpacity_Lambda([AmberNow=Amber]{return FSlateColor(AmberNow*(FMath::Fmod(Now(),1.1)<.6?1.f:0.f));})]]
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14,0)[Bar()]
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Label(TAttribute<FString>::CreateLambda([Clock]{return Clock?Clock():FString(TEXT("SIGNAL / 47"));}),24,Green)]]
   +SVerticalBox::Slot().AutoHeight().Padding(0,4,0,8)[Rule(Green)]
   +SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot()[Body]]
   // Below the scroll area, so the way out stays in view however long the page is.
   +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)[Footer.IsValid()?Footer.ToSharedRef():SNullWidget::NullWidget]];
 }
 FW Nav(const TArray<FNav>& Items)
 {
  TSharedRef<SVerticalBox> Box=SNew(SVerticalBox);
  for(const FNav& I:Items)
  {
   const bool Sel=I.bSelected,Attention=I.bAttention;const TFunction<void()> Act=I.Action;
   Box->AddSlot().AutoHeight()
    [SNew(SButton).ButtonStyle(Sel?&SelectedStyle():&PlainStyle()).ContentPadding(FMargin(12,8)).OnClicked_Lambda([Act]{if(Act)Act();return FReply::Handled();})
     [SNew(SHorizontalBox)
      +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text(FText::FromString(I.Label)).Font(S47Style::Mono(26)).ColorAndOpacity(Sel?Ink:Green)]
      +SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(Attention?TEXT("*"):TEXT(""))).Font(S47Style::Mono(26)).ColorAndOpacity_Lambda([Sel]{return FSlateColor((Sel?Ink:Amber)*Pulse(5));})]]];
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
   Box->AddSlot().AutoWidth().Padding(0,0,10,0)
    [SNew(SButton).ButtonStyle(Sel?&FilledStyle():&PlainStyle()).ContentPadding(FMargin(16,2)).OnClicked_Lambda([Act]{if(Act)Act();return FReply::Handled();})
     [SNew(STextBlock).Text(FText::FromString(I.Label)).Font(S47Style::Mono(26)).ColorAndOpacity(Sel?Ink:Green)]];
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
 FW Heading(const FString& V,int32 Size){return SNew(STextBlock).Text(FText::FromString(V)).Font(S47Style::Mono(Size)).ColorAndOpacity(Amber).AutoWrapText(true);}
 FW Banner(TAttribute<FString> V,TAttribute<bool> Urgent)
 {
  return SNew(SBorder).BorderImage(Outline()).BorderBackgroundColor_Lambda([Urgent]{return FSlateColor(Amber*(Urgent.Get()?Pulse(4):1.f));}).Padding(FMargin(14,8))
   [SNew(STextBlock).Text_Lambda([V]{return FText::FromString(V.Get());}).Font(S47Style::Mono(28)).ColorAndOpacity(Amber).Justification(ETextJustify::Center).AutoWrapText(true)];
 }
 FW Big(TAttribute<FString> V,TFunction<void()> Action,int32 Kind,TAttribute<bool> Enabled)
 {
  const FButtonStyle* S=Kind==2?&FilledStyle():Kind==1?&PrimaryStyle():&OutlineStyle();
  return SNew(SButton).ButtonStyle(S).ContentPadding(FMargin(16,9)).HAlign(HAlign_Center).IsEnabled(Enabled).OnClicked_Lambda([Action]{if(Action)Action();return FReply::Handled();})
   [SNew(STextBlock).Text_Lambda([V]{return FText::FromString(V.Get());}).Font(S47Style::Mono(28)).Justification(ETextJustify::Center).AutoWrapText(true)
    .ColorAndOpacity_Lambda([Kind,Enabled]{return FSlateColor(!Enabled.Get()?Dim*1.8f:Kind==2?Ink:Kind==1?Amber:Green);})];
 }
 FW Table(const TArray<FString>& Head,const TArray<float>& Widths,const TArray<FRow>& Rows)
 {
  auto Cells=[&Widths](const TArray<FString>& Values,FLinearColor C,int32 Size,const FString& Badge)
  {
   TSharedRef<SHorizontalBox> H=SNew(SHorizontalBox);
   for(int32 I=0;I<Values.Num();++I)H->AddSlot().FillWidth(Widths.IsValidIndex(I)?Widths[I]:1.f).Padding(6,0)[SNew(STextBlock).Text(FText::FromString(Values[I])).Font(S47Style::Mono(Size)).ColorAndOpacity(C)];
   if(!Badge.IsEmpty())H->AddSlot().AutoWidth().Padding(6,0)[SNew(SBorder).BorderImage(Outline()).BorderBackgroundColor(C).Padding(FMargin(6,0))[SNew(STextBlock).Text(FText::FromString(Badge)).Font(S47Style::Mono(Size-4)).ColorAndOpacity(C)]];
   return H;
  };
  TSharedRef<SVerticalBox> Box=SNew(SVerticalBox);
  Box->AddSlot().AutoHeight().Padding(12,6)[Cells(Head,Green,22,FString())];
  Box->AddSlot().AutoHeight()[Rule()];
  for(const FRow& R:Rows)
  {
   const TFunction<void()> Act=R.Action;
   Box->AddSlot().AutoHeight().Padding(0,2)
    [SNew(SButton).ButtonStyle(R.bSelected?&SelectedStyle():&PlainStyle()).ContentPadding(FMargin(6,7)).OnClicked_Lambda([Act]{if(Act)Act();return FReply::Handled();})[Cells(R.Cells,R.bSelected?Ink:Green,23,R.Badge)]];
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
  void Construct(const FArguments& A){Amount=A._Amount;ForceVolatile(true);}
  virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(200,12);}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 L,const FWidgetStyle&,bool)const override
  {
   const float V=Amount.Get();if(V<0)return L;const FVector2f S(G.GetLocalSize());
   Rect(Out,L,G,FVector2f(0,0),S,Line,false);Rect(Out,L+1,G,FVector2f(2,2),FVector2f((S.X-4)*FMath::Clamp(V,0.f,1.f),S.Y-4),V>=.999f?Green:Amber,true);return L+2;
  }
 private:TAttribute<float> Amount;
 };
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
  for(int32 I=0;I<Cols.Num();++I)H->AddSlot().FillWidth(Cols[I].Key).Padding(I?Gap*.5f:0.f,0,I+1<Cols.Num()?Gap*.5f:0.f,0)[Cols[I].Value];
  return H;
 }

 FW Split(const TArray<TPair<float,FW>>& Cols)
 {
  TSharedRef<SHorizontalBox> H=SNew(SHorizontalBox);
  for(int32 I=0;I<Cols.Num();++I)
  {
   if(I)H->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(2)[SNew(SBorder).BorderImage(Fill()).BorderBackgroundColor(Line)]];
   H->AddSlot().FillWidth(Cols[I].Key).Padding(14,6).VAlign(VAlign_Center)[Cols[I].Value];
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

 class SSkyPlot:public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SSkyPlot){} SLATE_END_ARGS()
  TFunction<FVector2D()> Target,Current;FString Source;
  mutable FVector2D Shown=FVector2D(-1,-1);mutable double LastPaint=0;
  void Construct(const FArguments&){ForceVolatile(true);}
  virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(560,360);}
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
   const float Arm=9+3*Pulse(4);
   Lines(Out,L+2,G,{Tp-FVector2f(Arm,0),Tp+FVector2f(Arm,0)},Amber,2.5f);Lines(Out,L+2,G,{Tp-FVector2f(0,Arm),Tp+FVector2f(0,Arm)},Amber,2.5f);
   Rect(Out,L+2,G,Tp-FVector2f(4,4),FVector2f(8,8),Amber,false);
   Rect(Out,L+3,G,Cp-FVector2f(8,8),FVector2f(16,16),Green,false);Lines(Out,L+3,G,{Cp-FVector2f(5,5),Cp+FVector2f(5,5)},Green,1.5f);
   Lines(Out,L+1,G,{{10,14},{26,14}},Amber,2.5f);Lines(Out,L+1,G,{{18,6},{18,22}},Amber,2.5f);Text2(Out,L+1,G,TEXT("TARGET ")+Source.Left(12),34,3,18,Green);
   Rect(Out,L+1,G,FVector2f(12,32),FVector2f(12,12),Green,false);Text2(Out,L+1,G,TEXT("ARRAY (CURRENT)"),34,28,18,Green);
   return L+4;
  }
 };
 FW SkyPlot(TFunction<FVector2D()> Target,TFunction<FVector2D()> Current,const FString& Source)
 {const auto W=SNew(SSkyPlot);W->Target=Target;W->Current=Current;W->Source=Source;return SNew(SBox).HeightOverride(400)[W];}

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
  virtual FReply OnMouseButtonDown(const FGeometry&,const FPointerEvent& E)override{if(E.GetEffectingButton()==EKeys::LeftMouseButton&&Click){Click();return FReply::Handled();}return FReply::Unhandled();}
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
}

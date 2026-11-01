#include "S47FreeformTerminal.h"
#include "S47Kit.h"
#include "SS47Widgets.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace S47FreeformTerminal
{
 const FLinearColor Green=S47Kit::Css(0x8cffa4),Amber=S47Kit::Css(0xffd27a);
 const FButtonStyle& ButtonStyle()
 {
  static const FButtonStyle S=FButtonStyle().SetNormal(FSlateColorBrush(S47Kit::Css(0x071a0d))).SetHovered(FSlateColorBrush(S47Kit::Css(0x193d24))).SetPressed(FSlateColorBrush(S47Kit::Css(0x285934))).SetNormalPadding(FMargin(8,6)).SetPressedPadding(FMargin(8,6));return S;
 }
 const FSliderStyle& KnobStyle()
 {
  static const FSliderStyle Style=[]
  {
   FSliderStyle S=FCoreStyle::Get().GetWidgetStyle<FSliderStyle>("Slider");
   const FSlateColorBrush Bar(FLinearColor::White);
   const FSlateRoundedBoxBrush Thumb(FLinearColor::White,8.f,FVector2f(16,16));
   S.SetNormalBarImage(Bar).SetHoveredBarImage(Bar).SetDisabledBarImage(Bar);
   S.SetNormalThumbImage(Thumb).SetHoveredThumbImage(Thumb).SetDisabledThumbImage(Thumb);
   S.SetBarThickness(4.f);return S;
  }();return Style;
 }
 FW Text(TAttribute<FString> Value,TAttribute<int32> Tone)
 {return SNew(STextBlock).Text_Lambda([Value]{return FText::FromString(Value.Get());}).Font(S47Style::Mono(24)).ColorAndOpacity_Lambda([Tone]{return FSlateColor(Tone.Get()<0?Amber:Green);}).AutoWrapText(true).Margin(FMargin(0,5));}
 FW Text(const FString& Value,int32 Tone){return Text(TAttribute<FString>(Value),TAttribute<int32>(Tone));}
 FW Paper(const FString& Value){return Text(Value);}
 FW Small(const FString& Value){return SNew(STextBlock).Text(FText::FromString(Value)).Font(S47Style::Mono(20)).ColorAndOpacity(Green*.8f).AutoWrapText(true).Margin(FMargin(0,7));}
 FW ButtonLive(TAttribute<FString> Label,TFunction<void()> Action,bool Primary,TAttribute<bool> Enabled)
 {
  return SNew(SButton).ButtonStyle(&ButtonStyle()).ContentPadding(FMargin(10,7)).IsEnabled(Enabled).OnClicked_Lambda([Action]{if(Action)Action();return FReply::Handled();})
  [SNew(STextBlock).Text_Lambda([Label]{return FText::FromString(Label.Get());}).Font(S47Style::Mono(24)).ColorAndOpacity(Primary?Amber:Green).AutoWrapText(true)];
 }
 FW Button(const FString& Label,TFunction<void()> Action,bool Primary,TAttribute<bool> Enabled){return ButtonLive(TAttribute<FString>(Label),Action,Primary,Enabled);}
 FW Pair(FW Left,FW Right)
 {return SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(1).Padding(0,0,12,0)[Left]+SHorizontalBox::Slot().FillWidth(1)[Right];}
 FW Knob(const FString& Label,TAttribute<FString> Shown,float Min,float Max,float Step,TAttribute<float> Value,TFunction<void(float)> OnChange)
 {
  return SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()
    [SNew(SHorizontalBox)
     +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text(FText::FromString(Label)).Font(S47Style::Mono(22)).ColorAndOpacity(Green)]
     +SHorizontalBox::Slot().AutoWidth().Padding(10,0)[SNew(STextBlock).Text_Lambda([Shown]{return FText::FromString(Shown.Get());}).Font(S47Style::Mono(24)).ColorAndOpacity(Amber)]]
   +SVerticalBox::Slot().AutoHeight().Padding(0,2,0,4)
    [SNew(SBox).HeightOverride(28).VAlign(VAlign_Center)
     [SNew(SSlider).Style(&KnobStyle()).MinValue(Min).MaxValue(Max).StepSize(Step).MouseUsesStep(true).Value(Value)
      .SliderBarColor(Green*.45f).SliderHandleColor(Amber).OnValueChanged_Lambda([OnChange](float V){if(OnChange)OnChange(V);})]];
 }
 FW Frame(const FString& Title,FW Body)
 {
  static const FSlateColorBrush Black(S47Kit::Css(0x020a05));
  return SNew(SBorder).BorderImage(&Black).Padding(16)
  [SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Title)).Font(S47Style::Mono(30)).ColorAndOpacity(Amber)]
   +SVerticalBox::Slot().AutoHeight().Padding(0,1,0,4)[SNew(STextBlock).Text(FText::FromString(TEXT("SARO / LOCAL WORKSTATION / 1986"))).Font(S47Style::Mono(16)).ColorAndOpacity(Green*.8f)]
   +SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot()[Body]]];
 }
}

#include "S47FreeformTerminal.h"
#include "S47Kit.h"
#include "SS47Widgets.h"
#include "Brushes/SlateColorBrush.h"
#include "Widgets/Input/SButton.h"
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
 FW Frame(const FString& Title,FW Body)
 {
  static const FSlateColorBrush Black(S47Kit::Css(0x020a05));
  return SNew(SBorder).BorderImage(&Black).Padding(24)
  [SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Title)).Font(S47Style::Mono(30)).ColorAndOpacity(Amber)]
   +SVerticalBox::Slot().AutoHeight().Padding(0,10)[Small(TEXT("SARO / LOCAL WORKSTATION / 1986"))]
   +SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot()[Body]]
   +SVerticalBox::Slot().AutoHeight()[Small(TEXT("MOUSE / SELECT     SCROLL / PAGE     ESC / STEP BACK"))]];
 }
}

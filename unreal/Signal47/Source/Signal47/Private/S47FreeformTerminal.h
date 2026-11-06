#pragma once
#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "S47Rx.h"
// SARO 1986 workstation kit, drawn on the physical CRT glass (1200 x 900 Slate units).
// Look follows Tom's five screen mockups of 3 November 2026: green phosphor on black,
// amber for the selected item and the next action, outlined panels, live plots.
namespace S47FreeformTerminal
{
 using FW=TSharedRef<SWidget>;
 FW Text(const FString& Value,int32 Tone=0);
 FW Text(TAttribute<FString> Value,TAttribute<int32> Tone);
 FW Paper(const FString& Value);
 FW Small(const FString& Value);
 FW Button(const FString& Label,TFunction<void()> Action,bool Primary=false,TAttribute<bool> Enabled=true);
 FW ButtonLive(TAttribute<FString> Label,TFunction<void()> Action,bool Primary=false,TAttribute<bool> Enabled=true);
 FW Pair(FW Left,FW Right);
 FW Knob(const FString& Label,TAttribute<FString> Shown,float Min,float Max,float Step,TAttribute<float> Value,TFunction<void(float)> OnChange);
 FW Frame(const FString& Title,FW Body,TSharedPtr<SWidget> Footer=nullptr);
 // The reel deck's own look (Tom's reference, 5 Nov): dark olive, cream text, amber action. Widgets
 // built while one of these lives keep it; the green workstation screens are untouched.
 struct FReelLook { FReelLook(); ~FReelLook(); };
 FW Toggle(const FString& Text,TAttribute<bool> On,TFunction<void()> Action,TAttribute<bool> Enabled=true); // filled when on
 FW Filled(const FString& Text,TFunction<void()> Action,TAttribute<bool> Enabled=true); // the one thing to do
 // The right-hand header clock ("DAY 01 | 19:10"). The owner clears it when it stops.
 void SetClock(TFunction<FString()> Clock);

 // --- Workstation layout (UI-03) ---
 struct FNav { FString Label; bool bSelected=false; TFunction<void()> Action; bool bAttention=false; };
 FW Nav(const TArray<FNav>& Items);                 // vertical menu, selected item filled amber
 FW Tabs(const TArray<FNav>& Items);                // horizontal tabs, selected filled green
 FW Panel(const FString& Title,FW Body,bool bFilledHeader=false);
 FW Heading(const FString& Value,int32 Size=30);    // amber section title
 FW Banner(TAttribute<FString> Value,TAttribute<bool> Urgent=true); // amber framed status line, pulses when urgent
 // Kind: 0 outline green, 1 primary amber outline, 2 filled green
 FW Big(TAttribute<FString> Label,TFunction<void()> Action,int32 Kind=0,TAttribute<bool> Enabled=true);
 struct FRow { TArray<FString> Cells; bool bSelected=false; TFunction<void()> Action; FString Badge; };
 FW Table(const TArray<FString>& Head,const TArray<float>& Widths,const TArray<FRow>& Rows);
 FW Value(const FString& Label,TAttribute<FString> Shown,bool bAmber=false); // label left, value right
 FW Readout(const FString& Label,TAttribute<FString> Shown,TAttribute<float> Fill=-1.f); // big number, optional bar
 FW Control(const FString& Label,TAttribute<FString> Shown,float Min,float Max,float Step,TAttribute<float> Value,TFunction<void(float)> OnChange);
 FW Spectrum(TFunction<FS47Rx()> Receiver,TFunction<FS47Profile()> Card,TFunction<double()> Quality);
 FW SkyPlot(TFunction<FVector2D()> Target,TFunction<FVector2D()> Current,const FString& Source,TFunction<void(FVector2D)> OnAim=nullptr,TFunction<bool()> OnTarget=nullptr);
 FW FivePoint(TFunction<int32()> Done,TFunction<bool()> Running,TFunction<bool()> Required);
 // State per element: 0 ready, 1 selected, 2 isolated/fault, 3 moving
 FW AntennaGrid(int32 Count,TFunction<FString(int32)> Label,TFunction<int32(int32)> State,TFunction<void(int32)> Pick);
 FW Columns(const TArray<TPair<float,FW>>& Columns,float Gap=14);
 FW Split(const TArray<TPair<float,FW>>& Columns);   // one outlined strip, vertical rules between columns
 FW TitleRow(const FString& Left,TAttribute<FString> Right);
}

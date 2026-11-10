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
 // D45-20261110 (Tom: the reel deck must fit one screen without scrolling): Frame's header and footer on a fixed faceplate, the
 // body clipped to its place instead of scrolled. The owner gives it its size (1200 x 900 in a scale box). No tube effects.
 FW Faceplate(const FString& Title,FW Body,TSharedPtr<SWidget> Footer=nullptr);
 // D45-20261110: the faceplate's small parts: a square lamp (its colour live), and a live line of text at a size and colour of its own
 // (bWrap: wraps to its column; the owner gives it the height it may take).
 FW Lamp(TAttribute<FLinearColor> Lit,float Size=12);
 FW Note(TAttribute<FString> Value,int32 Size,TAttribute<FLinearColor> Colour,bool bWrap=true);
 FW Toggle(const FString& Text,TAttribute<bool> On,TFunction<void()> Action,TAttribute<bool> Enabled=true); // filled when on
 FW Filled(const FString& Text,TFunction<void()> Action,TAttribute<bool> Enabled=true); // the one thing to do
 // The right-hand header clock ("DAY 01 | 19:10"). The owner clears it when it stops.
 void SetClock(TFunction<FString()> Clock);
 // CASES-OPS-20261109: the left-hand header ("SARO OPS V4.2  TERM 1  OPR NIGHT", as Story's operations system heads its screen).
 // Empty: the old "SARO / LOCAL WORKSTATION". The owner clears it with the clock.
 void SetStation(const FString& Station);
 // FIX-20261110: the operator's feedback and the tube's own life (the look itself is unchanged).
 // Sound: one sink for every control on the station. Kind 0 key, 1 the amber/primary action,
 // 2 a menu line or tab, 3 a knob step, 4 the cursor arriving on a control.
 void SetSound(TFunction<void(int32 Kind)> Sink);
 // 0..1, what is on the air around the station: lines drop out and the picture tears with it.
 void SetInterference(TFunction<float()> Level);
 float Interference();
 // The set was just switched on: the open terminal and the idle pictures open from a line. D45-20261110: only when the supply
 // comes back (FS47FreeformSession::Step), not each time the operator sits down.
 void PowerOn();
 double PowerAge(); // seconds since PowerOn
 // D45-20261110 (Tom: the band was too big and always there; not like a real CRT): the tube's one rare, faint, soft hum bar,
 // shared by the open terminal and the idle picture on the monitor across the room (SS47Crt). Seed: the screen (0 array status,
 // 1 receiver, 2 tracking, 3 OPS), so both pictures of one monitor agree. A is 0 while there is none, which is most of the time.
 struct FHum { float Y=0,H=0,A=0; };
 FHum HumBar(double T,int32 Seed,float ScreenH);
 void PaintHum(FSlateWindowElementList& Out,int32 Layer,const FGeometry& G,const FVector2f& Size,int32 Seed,const FLinearColor& Phosphor);
 void PaintPowerOn(FSlateWindowElementList& Out,int32 Layer,const FGeometry& G,const FVector2f& Size,const FLinearColor& Phosphor);
 void SetHumScreen(int32 Screen); // the screen the next Frame is for; -1 none. The owner clears it with the clock.
 // D45-2: the tracking plot's search tick (Near 0 far off to 1 on the source) and how near a plot position is to the source. The
 // owner gives both while its terminal is open and clears them when it closes.
 void SetProbeSound(TFunction<void(float Near)> Sink);
 void SetNearness(TFunction<double(FVector2D AzEl)> Near);
 bool SelfTest(TArray<FString>& Report);

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
 FW Meter(const FString& Label,TAttribute<FString> Shown,TAttribute<float> Fill); // one line (name left, value right) over a thin progress bar
 FW Display(const FString& Label,TAttribute<FString> Value,TAttribute<FString> State); // name and state on one line, the value large beneath
 FW Control(const FString& Label,TAttribute<FString> Shown,float Min,float Max,float Step,TAttribute<float> Value,TFunction<void(float)> OnChange);
 FW Spectrum(TFunction<FS47Rx()> Receiver,TFunction<FS47Profile()> Card,TFunction<double()> Quality);
 FW SkyPlot(TFunction<FVector2D()> Target,TFunction<FVector2D()> Current,const FString& Source,TFunction<void(FVector2D)> OnAim=nullptr,TFunction<bool()> OnTarget=nullptr);
 FW FivePoint(TFunction<int32()> Done,TFunction<bool()> Running,TFunction<bool()> Required);
 // State per element: 0 ready, 1 selected, 2 isolated/fault, 3 moving
 FW AntennaGrid(int32 Count,TFunction<FString(int32)> Label,TFunction<int32(int32)> State,TFunction<void(int32)> Pick);
 FW Columns(const TArray<TPair<float,FW>>& Columns,float Gap=14);
 FW Split(const TArray<TPair<float,FW>>& Columns);   // one outlined strip, vertical rules between columns
 FW TitleRow(const FString& Left,TAttribute<FString> Right);
 // FIX-20261110: a frame off the video digitizer, in the tube's own phosphor (no image: nothing to read yet). With an action it
 // is a key, framed amber when it is the one selected.
 FW Picture(class UTexture2D* Image,float Width,const FString& Caption=FString(),bool bSelected=false,TFunction<void()> Action=nullptr,bool bDue=false);
}

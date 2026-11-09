#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "S47Script.h"
class US47Main;
class SBox;
class FS47ScriptPlayer;

// DIRECTOR-TOOL-20261109: the director's panel, behind F10 > Director scripts > Script editor, in a developer's run only
// (FS47ScriptPlayer::DevAllowed). The scripts on the left; on the right the one being edited: its name, on or off, how often, the
// trigger (days, nights, hours, a place from the list or a point where you stand or look, conditions) and the actions on their
// timeline, added from a row of buttons. SAVE writes it (after the lint), PREVIEW plays it now, STOP ends it. What is being edited
// stays when the panel is closed, so one can walk to the next point and open it again. HUD D's colours (S47HudGauges::Palette).
namespace S47ScriptEditor
{
 void Open(US47Main* M);
}

class SS47ScriptEditor : public SCompoundWidget
{
public:
 SLATE_BEGIN_ARGS(SS47ScriptEditor) : _Main(nullptr) {}
  SLATE_ARGUMENT(US47Main*, Main)
 SLATE_END_ARGS()
 void Construct(const FArguments& InArgs);
 virtual bool SupportsKeyboardFocus() const override { return true; }
 virtual FReply OnKeyDown(const FGeometry& G, const FKeyEvent& E) override;
private:
 US47Main* M = nullptr;
 TSharedPtr<SBox> ListSlot, BodySlot;
 FString Status;
 bool bStatusBad = false;
 FS47ScriptPlayer* Player() const;
 void Rebuild();
 TSharedRef<SWidget> BuildList();
 TSharedRef<SWidget> BuildBody();
 TSharedRef<SWidget> BuildTrigger();
 TSharedRef<SWidget> BuildCond(int32 I);
 TSharedRef<SWidget> BuildAction(int32 I);
 TSharedRef<SWidget> BuildAdd();
 TSharedRef<SWidget> BuildBar();
 // The pieces, in HUD D's colours.
 TSharedRef<SWidget> Btn(const FString& Text, TFunction<void()> Fn, bool bStrong = false);
 TSharedRef<SWidget> Words(const FString& Text, float Size = 20, int32 Tone = 0); // 0 body, 1 accent, 2 dim, 3 warning
 TSharedRef<SWidget> Field(const FString& Hint, const FString& Value, TFunction<void(const FString&)> Set, float Width = 160);
 TSharedRef<SWidget> Num(const FString& Hint, double Value, TFunction<void(double)> Set, float Width = 70);
 TSharedRef<SWidget> Cycle(const FString& Prefix, const TArray<FString>& Options, const FString& Value, TFunction<void(const FString&)> Set);
 // A point's buttons: where I stand, where I look, and what it is now.
 TSharedRef<SWidget> PointRow(const FString& Label, TFunction<FS47ScriptPoint*()> Get);
 void Select(const FString& Id);
 void NewScript();
 void SaveIt();
 void PreviewIt();
 void StopIt();
 void CloseIt();
 void Say(const FString& Text, bool bBad = false);
 void Changed(); // the draft has changed in shape: build again
};

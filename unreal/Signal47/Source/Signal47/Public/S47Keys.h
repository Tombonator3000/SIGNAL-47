#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class APlayerController;
class FJsonObject;

// The game's keys in one place: every action the player can take (the web version's controls
// list, ui/UI.ts CONTROLS) with up to two keys each, rebindable in Settings > Controls and kept
// with the other settings (settings.json, "keys"). The walker, the truck and the keys the modules
// answer (US47Main::OnAction) read their keys from here.
//
// Fixed, and not offered for another action: Escape (pause and every menu's way back) and the
// left mouse button (a click uses, as E does). The walk keys drive the truck too (throttle,
// brake and steering), as in the web version.
namespace S47Keys
{
	struct FAction
	{
		FName Id;
		FString Label;          // "Walk forward"
		FString Group;          // "On foot", "In the truck", "Panels"
		FString Help;           // what it does (the settings' description)
		FKey Default[2];
		bool bFixedFirst = false; // the first key cannot be changed (Escape for the pause)
	};

	// every action, in the order the settings list them
	SIGNAL47_API const TArray<FAction>& Actions();
	SIGNAL47_API const FAction* Find(FName Id);

	// ---------- reading ----------
	SIGNAL47_API FKey Key(FName Action, int32 Slot);                   // EKeys::Invalid when the slot is empty
	SIGNAL47_API TArray<FKey, TInlineAllocator<2>> KeysOf(FName Action);
	SIGNAL47_API bool Is(FName Action, const FKey& K);                  // K is one of the action's keys
	SIGNAL47_API bool Down(const APlayerController* PC, FName Action);   // held now
	SIGNAL47_API bool Pressed(const APlayerController* PC, FName Action); // pressed this frame
	// names for the screen: Short is a keycap's ("E", "SPACE", "RMB"); Name is for a sentence ("E",
	// "Space", "Right mouse"). Label is the action's first key (or the second when the first is empty).
	SIGNAL47_API FString Short(const FKey& K);
	SIGNAL47_API FString Name(const FKey& K);
	SIGNAL47_API FString Label(FName Action);   // Short of its key, "" when it has none
	SIGNAL47_API FString Said(FName Action);    // Name of its key, for a hint

	// ---------- changing ----------
	enum class EBind : uint8 { Done, Swapped, Same, Reserved, Fixed };
	// Bind K to Action's Slot. A key another action has moves there in exchange (Swapped: Other is
	// that action, which now has Action's old key in its place); Escape, the left mouse button and the
	// gamepad are refused (Reserved), and a fixed slot is not changed (Fixed).
	SIGNAL47_API EBind Bind(FName Action, int32 Slot, const FKey& K, FName* Other = nullptr);
	SIGNAL47_API void Clear(FName Action, int32 Slot);
	SIGNAL47_API void Reset(FName Action);                               // its defaults (taken from any other action that has them)
	SIGNAL47_API void ResetAll();
	SIGNAL47_API bool IsDefault(FName Action);
	SIGNAL47_API bool Bindable(const FKey& K);                          // a key that can be bound at all

	// ---------- kept with the settings ----------
	SIGNAL47_API TSharedPtr<FJsonObject> Save();                        // { action: ["W", "Up"] } (FKey names, "" for none)
	SIGNAL47_API void Load(const TSharedPtr<FJsonObject>& O);           // unknown or broken entries keep their defaults

	SIGNAL47_API bool SelfTest(TArray<FString>& Out);
	// tests (-S47Press=Down,Down,Enter,G): the keys pressed one after another, Every seconds apart, as
	// Slate would have them from the keyboard (to the menu that has the focus); "Wait" presses nothing,
	// "Click:x:y" moves the mouse to a point of the game's picture (its pixels) and clicks there
	SIGNAL47_API void TestPresses(const FString& Csv, float Every = 0.5f);
}

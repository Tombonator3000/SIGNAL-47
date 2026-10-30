#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"

class US47UI;
class SWidget;

// ui/MapOverlay.ts: All Night's map puzzle (KAPITLER.md, P13): SARO 07's service map on the booth
// table and E09A traced on onion-skin over it. The tracing is dragged with the mouse; it settles on
// STATION 01 when its transit comes within a few millimetres of the station. Then C runs east across
// the map, and the player reads off where it meets the old road.
namespace S47MapOverlay
{
	struct FChoice { FString Id, Label; };

	// MapOverlayOpts
	struct FOpts
	{
		FVector2D At = FVector2D::ZeroVector;          // where the tracing's transit lies on the map now
		FString Solved;                                // the recorded finding, once there is one ('' null)
		FString Question;
		bool bHandover = false; // optional SC12-only annotation; the legacy map remains unchanged
		TArray<FChoice> Choices;
		TFunction<void(double X, double Y, bool bAligned)> OnMove;
		TFunction<FS47Result(const FString& Id)> OnChoose;
		TFunction<void()> OnClose;
	};

	// mapOverlay(ui, o): the panel, opened with UI->Open, and given back as the page's element is
	SIGNAL47_API TSharedRef<SWidget> MapOverlay(US47UI* UI, const FOpts& O);

	// the drag's rules without a screen: where a press takes the sheet, the clamp, the snap
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Input/Reply.h"
#include "S47Walker.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Widgets/SCompoundWidget.h"

class AS47Web;
class AS47Walker;
class FSlateVirtualUserHandle;
class FWidgetPath;
class FWidgetRenderer;
class SVirtualWindow;
class UTextureRenderTarget2D;

// Using one of the station's screens (the receiver console on the centre CRT, the operations
// terminal on the west desk's): the camera goes up to the glass in 0.6 s, the screen's picture is
// drawn on the CRT in the room (its material shows a render target this draws into, at 1280 x 960)
// and the mouse and the keys go to it. The pointer's ray from the camera meets the glass, and the
// point it meets is where the screen's widgets are told the pointer is: they live in a window of
// their own that Slate knows (a virtual window) and get their events as a virtual user's, as
// Unreal drives its widget components. Esc, a click off the glass (and E where nothing is typed)
// step back; the modal closes when the camera is back. Without the room (no such screen in the
// export, no walker) the content stands over the game in the web version's beige bezel instead.
// (THE BLACK MAP draws its room's screens into render targets the same way, BlackMapRoom.cpp.)
class SS47ScreenView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47ScreenView) : _Screen(1), _Web(nullptr), _Walker(nullptr), _bUseLeaves(true) {}
		SLATE_ARGUMENT(int32, Screen)             // AS47Web's CRTs: 0 left, 1 centre, 2 right, 3 ops
		SLATE_ARGUMENT(AS47Web*, Web)
		SLATE_ARGUMENT(AS47Walker*, Walker)
		SLATE_ARGUMENT(bool, bUseLeaves)          // E steps back too (not where E is typed)
		SLATE_ARGUMENT(TFunction<void()>, OnLeft) // the camera is back: close the modal
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	virtual ~SS47ScreenView() override;

	/** What the screen shows, and the widget that has the keys first. */
	void SetScreen(const TSharedRef<SWidget>& Content, const TSharedPtr<SWidget>& KeysTo);
	/** Step back from the screen (the content's own close). */
	void Leave();
	bool OnGlass() const { return bWorld; }
 uint64 RenderedFrames() const { return DrawFrames; }
	/** The camera's turn towards the glass, the glass's pixels and the steps' easing. */
	static bool SelfTest(TArray<FString>& Lines);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

	static constexpr int32 PicW = 1280, PicH = 960; // the picture's pixels (4:3, as the glass)
	static constexpr float Units = 1200.f;          // its width in Slate units (S47Theme's 1000 mockup px)

private:
	struct FEye { FVector Eye = FVector::ZeroVector; double Yaw = 0, Pitch = 0; };
	uint64 DrawFrames=0;
 int32 Screen = 1;
	TWeakObjectPtr<AS47Web> Web;
	TWeakObjectPtr<AS47Walker> Walker;
	bool bUseLeaves = true, bWorld = false, bLeaving = false, bLeft = false, bStarted = false, bForwarding = false, bKeysLogged = false;
	TFunction<void()> OnLeft;
	// the glass: its centre, the way it faces (to the eye), its right and up as the picture runs, its size (cm)
	FVector Centre = FVector::ZeroVector, Normal = FVector::ForwardVector, Right = FVector::RightVector, Up = FVector::UpVector;
	FVector2D Size = FVector2D(37, 28);
	FEye From, To;
	float T = 0.f; // 0 the walker's eye, 1 at the glass
	TFunction<bool(double, const FVector2D&, const FVector2D&, FS47View&)> Before; // the walker's override before this
	TSharedPtr<SVirtualWindow> Window;
	TSharedPtr<SWidget> Content;
	TWeakPtr<SWidget> KeysTo;
	TSharedPtr<FSlateVirtualUserHandle> User;
	FWidgetRenderer* Renderer = nullptr;
	TStrongObjectPtr<UTextureRenderTarget2D> Target;
	FVector2D Last = FVector2D(-1, -1);
	TSet<FKey> Held;

	FEye Now() const;
	void Finish();
	bool KeysInside() const;
	void FocusInside();
	/** The pointer on the glass, in the picture's pixels; false off the glass (or no ray). */
	bool ToPicture(const FGeometry& G, const FVector2D& ScreenPos, FVector2D& Px) const;
	FWidgetPath PathAt(const FVector2D& Px) const;
	FReply Route(const FGeometry& G, const FPointerEvent& E, int32 Kind);
};

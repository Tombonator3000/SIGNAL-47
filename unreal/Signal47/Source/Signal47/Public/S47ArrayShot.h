#pragma once

#include "CoreMinimal.h"

class US47Main;
class AS47Walker;
struct FS47View;

// Unreal only: the web version has no such shot (Prologue.ts has only its gentle look assist). Tom,
// 22 October 2026, after playing the Unreal build: "kamera må zoome til vinduet automatisk så spiller
// garantert ser de flytte seg og situasjonen blir mer dramatisk". So when every dish of the array turns
// together at 02:15 (the prologue's 'turning'), the camera is taken, over a little more than a second,
// to the control room's window and looks out at the array, closing in slowly (the field of view narrows
// as the closer look's does, and the eye eases towards the glass), holds while the dishes swing round,
// and gives the view back as smoothly. Away from the control room the eye stays where the player is and
// only turns to the array. The player is never moved: the walker's camera is held (its CameraOverride,
// as the cab's is), and the look assist goes on under it, so the view comes back facing the array.
// Positions in three's terms (metres, y up), as the walker's.
class SIGNAL47_API FS47ArrayShot : public TSharedFromThis<FS47ArrayShot>
{
public:
	explicit FS47ArrayShot(US47Main* InM) : M(InM) {}

	/** The dishes begin to turn: take the camera. */
	void Begin();
	/** They have come round: give the view back (over BLEND_OUT seconds). */
	void Release();
	/** At once, the walker's own view again (a jump, a restart). */
	void Stop();
	/** Each frame from the story, outside the walker's own frame: puts the walker's camera back in its
	 *  hands once the shot is over (not from inside the override, which is still running then). */
	void Tick();
	bool IsOn() const { return Stage != EStage::Off; }

	/** The pose math and the turn's timing, without the world (-S47SelfTest). */
	static bool SelfTest(TArray<FString>& Lines);

	/** Where it looks: the array's middle, as the look assist (Prologue.update's target). */
	static const FVector TARGET;
	/** The eye at the window, and where it has eased to by the end of the push: over the console desk
	 *  between the left and the middle screens, in the middle of a pane (mullions at x -1.8 and 0). */
	static const FVector WINDOW_EYE0, WINDOW_EYE1;
	/** Seconds: in, the push-in (about the dishes' turn), out; the longest it holds without a Release. */
	static constexpr double BLEND_IN = 1.3, PUSH = 14, BLEND_OUT = 1.4, MAX_HOLD = 45;
	/** How far the field of view narrows by the end of the push: at the window, and turning in place. */
	static constexpr double NARROW_WINDOW = 0.4, NARROW_TURN = 0.3;
	/** The dishes' slew in this scene (degrees a second; elevation at 0.6 of it, Dish.update): the web
	 *  version's 2.2 (Prologue.turnDishes) made twice as quick, on the same request ("arrays må gå litt
	 *  kjappere så det ikke blir så tregt i scenen"): about 13 s from the survey to 026 / 32 instead of 26. */
	static constexpr double WEB_SLEW = 2.2, TURN_SLEW = 2 * WEB_SLEW;

	struct FPose { FVector Eye = FVector::ZeroVector; double Yaw = 0, Pitch = 0, Roll = 0, Fov = 70; };
	/** The shot itself at T seconds in, given where the player's eye is and its field of view. */
	static FPose ShotPose(bool bWindow, double T, const FVector& LiveEye, double Fov0);
	/** From a to b by k (0..1, eased by the caller): the eye along a low arc, the angles the short way. */
	static FPose Blend(const FPose& A, const FPose& B, double K, double Arc);
	/** Whether a point is in the control room (ControlRoom.ts bounds). */
	static bool InControlRoom(const FVector& P);

private:
	enum class EStage : uint8 { Off, In, Out };
	US47Main* M = nullptr;
	EStage Stage = EStage::Off;
	bool bWindow = true;
	double T = 0, TOut = 0, KOut = 0, Fov0 = 70;
	TWeakObjectPtr<AS47Walker> Hooked;
	TFunction<bool(double, const FVector2D&, const FVector2D&, FS47View&)> Before;

	/** The walker's frame while the shot holds it (false: the walker's own, or the cab's). */
	bool Frame(double Dt, FS47View& Out);
	FPose Live() const;
	void Unhook();
};

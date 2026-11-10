#pragma once

#include "CoreMinimal.h"

class UMaterialParameterCollection;
class UWorld;

// D45-20261110 (D45-19): the BODYCAM 1986 picture (S47Picture mode 5). A 1986 field camera on a chest
// harness taping to VHS: the walker's camera sits a little below and ahead of the eye and looks a little
// down, it sways with the body when Head motion and bumps is on, and the tone pass (film_tone.hlsl) bends
// the picture through a wide lens and burns in the date and the unit line. Everything here is a pure step:
// the walker feeds it each frame and places its camera from FOut; US47WorldLife writes FOut's vectors
// and the stamp into MPC_Body beside the wind's values. Nothing of it is saved. In every other picture mode
// the offset is exactly zero and the field of view factor exactly one.
struct SIGNAL47_API FS47Bodycam
{
	// the chest mount (metres, degrees), the wide lens and how fast the mount gives way
	static constexpr double Drop = 0.20, Ahead = 0.08, TiltDeg = 3.0, FovWide = 1.12, FadeSecs = 0.2;
	static constexpr float Bend = 0.14f, Fringe = 1.6f, Vignette = 0.30f;

	struct FIn
	{
		bool bOn = false;         // BODYCAM 1986 chosen, its shader present and the strength above zero
		bool bHeadMotion = true;  // Settings: Head motion and bumps (all sway, none of the static mount)
		bool bCalm = false;       // Slow fades or Still shots: no pan skew, no gain pumping
		bool bRaised = false;     // the field camera at the eye: the frame must equal the photograph
		bool bScreen = false;     // a world PC screen in use: its clicks are deprojected through a plain lens
		bool bHeld = false;       // the camera held by something else (the truck's cab, a screen): no offset now
		bool bLamp = false;       // the flashlight on: its toggles pump the gain
		bool bRun = false, bStep = false;
		double Dt = 0;
		double Turn = 0, Tilt = 0; // the look this frame, radians: right and up positive
		double Strafe = 0;         // sideways keys, -1 left .. 1 right
		double Bob = 0;            // the walker's bob phase
	};
	struct FOut
	{
		double Mount = 0;   // how much of the chest offset and its sway is applied now, 0..1
		double Gate = 0;    // how much of the lens the shader shows, 0..1
		FVector Offset = FVector::ZeroVector; // body terms, metres: X right, Y up, Z ahead (before Mount)
		double DownDeg = 0, YawDeg = 0, RollDeg = 0; // camera turn on the mount (Unreal yaw: right positive)
		float Fov = 1.f;    // times the walker's field of view
		FLinearColor Lens = FLinearColor(0, 0, 0, 0); // BodyLens: bend, fringe px, gate, vignette
		FLinearColor Cam = FLinearColor(0, 0, 0, 0);  // BodyCam: gain kick, turn rate, tilt rate (rad/s), stamp alpha (the writer's)
	};

	FOut Step(const FIn& In);
	const FOut& Last() const { return Out; }

	// BodyStamp for the shader: x month*100+day, y two-digit year, z hour(1-12)*10000+min*100+sec, w PM.
	static FLinearColor Stamp(int32 Month, int32 Day, int32 Year, double SecondsOfDay);
	// What the shader draws from that vector: "04/28/86  11:58:07 PM" (and "SARO FIELD CAM 2" under it).
	static FString StampText(const FLinearColor& Packed);
	// MPC_Body, made by import_web.py build_picture with the new M_WebTone; null with older content.
	static UMaterialParameterCollection* Collection();
	static bool HasShader();
	// The three vectors into MPC_Body; nothing is written while they stay as they were.
	static void Write(UWorld* World, const FLinearColor& Lens, const FLinearColor& Cam, const FLinearColor& StampV);
	static bool SelfTest(TArray<FString>& Lines);

private:
	FOut Out;
	double MountK = 0, GateK = 0, Lag = 0, Roll = 0, StepT = -1, Breath = 0, Kick = 0, TurnRate = 0, TiltRate = 0;
	bool bLampKnown = false, bLampWas = false;
};

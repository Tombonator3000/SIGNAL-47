#include "S47Bodycam.h"

#include "Engine/World.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "UObject/WeakObjectPtrTemplates.h"

// D45-20261110 (D45-19): see S47Bodycam.h.

FS47Bodycam::FOut FS47Bodycam::Step(const FIn& In)
{
	const double Dt = FMath::Clamp(In.Dt, 0.0, 0.25);
	const double Fade = Dt / FadeSecs;
	auto Toward = [Fade](double From, double To) { return From < To ? FMath::Min(To, From + Fade) : FMath::Max(To, From - Fade); };
	// The mount is held where it is while something else holds the camera: a screen starts from (and
	// returns to) the eye the walker had, so handing the camera over and back never jumps.
	if (!In.bHeld || !In.bOn) MountK = Toward(MountK, In.bOn && !In.bRaised && !In.bHeld ? 1.0 : 0.0);
	GateK = Toward(GateK, In.bOn && !In.bRaised && !In.bScreen && !In.bHeld ? 1.0 : 0.0);
	// the look's rates, a little smoothed (mouse deltas are uneven frame to frame)
	const double WantTurn = Dt > 1e-6 ? FMath::Clamp(In.Turn / Dt, -6.0, 6.0) : 0.0;
	const double WantTilt = Dt > 1e-6 ? FMath::Clamp(In.Tilt / Dt, -6.0, 6.0) : 0.0;
	const double Smooth = FMath::Min(1.0, Dt / 0.06);
	TurnRate += (WantTurn - TurnRate) * Smooth;
	TiltRate += (WantTilt - TiltRate) * Smooth;
	// the tube's gain pumps when the flashlight goes on (it closes down) or off (it opens up)
	if (!bLampKnown) { bLampKnown = true; bLampWas = In.bLamp; }
	else if (In.bLamp != bLampWas) { bLampWas = In.bLamp; Kick = In.bLamp ? -0.6 : 0.6; }
	Kick *= FMath::Exp(-Dt / 0.6);
	if (FMath::Abs(Kick) < 1e-3) Kick = 0;
	// The body's sway, only with Head motion and bumps on: lag behind a turn (about 90 ms, at most
	// 4 degrees), a lean when stepping sideways, a nod on each step and slow breathing; more at a run.
	const bool bSway = In.bHeadMotion;
	const double RunK = In.bRun ? 1.7 : 1.0;
	double Nod = 0;
	if (bSway)
	{
		const double MaxLag = FMath::DegreesToRadians(4.0);
		Lag += (FMath::Clamp(-TurnRate * 0.09, -MaxLag, MaxLag) - Lag) * FMath::Min(1.0, Dt / 0.09);
		Roll += (FMath::Clamp(In.Strafe, -1.0, 1.0) * 0.8 * RunK - Roll) * FMath::Min(1.0, Dt / 0.15);
		if (In.bStep) StepT = 0;
		else if (StepT >= 0) { StepT += Dt; if (StepT > 0.3) StepT = -1; }
		Breath = FMath::Fmod(Breath + Dt * 2.0 * UE_DOUBLE_PI * 0.25, 2.0 * UE_DOUBLE_PI);
		Nod = StepT >= 0 ? FMath::Sin(UE_DOUBLE_PI * StepT / 0.3) : 0.0;
	}
	else { Lag = Roll = Breath = 0; StepT = -1; }
	Out.Mount = In.bHeld ? 0.0 : MountK;
	Out.Gate = GateK;
	Out.Offset = FVector(0.0, -Drop + (bSway ? 0.008 * FMath::Sin(In.Bob) * RunK + 0.004 * FMath::Sin(Breath) : 0.0), Ahead);
	Out.DownDeg = TiltDeg + (bSway ? 0.3 * Nod * RunK + 0.12 * FMath::Sin(Breath + 0.6) : 0.0);
	Out.YawDeg = bSway ? FMath::RadiansToDegrees(Lag) : 0.0;
	Out.RollDeg = bSway ? Roll : 0.0;
	Out.Fov = GateK > 0 ? float(1.0 + (FovWide - 1.0) * GateK) : 1.f;
	Out.Lens = In.bOn ? FLinearColor(Bend, Fringe, float(GateK), Vignette) : FLinearColor(0, 0, 0, 0);
	Out.Cam = In.bOn && !In.bCalm ? FLinearColor(float(Kick), float(TurnRate), float(TiltRate), 0.f) : FLinearColor(0, 0, 0, 0);
	return Out;
}

FLinearColor FS47Bodycam::Stamp(int32 Month, int32 Day, int32 Year, double SecondsOfDay)
{
	const double Sod = FMath::IsFinite(SecondsOfDay) ? FMath::Fmod(FMath::Fmod(SecondsOfDay, 86400.0) + 86400.0, 86400.0) : 0.0;
	const int32 T = FMath::Clamp(int32(FMath::FloorToDouble(Sod)), 0, 86399);
	const int32 H24 = T / 3600, Min = T / 60 % 60, Sec = T % 60;
	const int32 H12 = H24 % 12 == 0 ? 12 : H24 % 12;
	return FLinearColor(float(FMath::Clamp(Month, 1, 12) * 100 + FMath::Clamp(Day, 1, 31)), float(((Year % 100) + 100) % 100), float(H12 * 10000 + Min * 100 + Sec), H24 >= 12 ? 1.f : 0.f);
}

FString FS47Bodycam::StampText(const FLinearColor& Packed)
{
	// the shader's own reading of the vector (dateChar in film_tone.hlsl)
	const int32 Md = int32(Packed.R + .5f), Yy = int32(Packed.G + .5f) % 100, T = int32(Packed.B + .5f);
	return FString::Printf(TEXT("%02d/%02d/%02d  %02d:%02d:%02d %s"), Md / 100, Md % 100, Yy, T / 10000, T / 100 % 100, T % 100, Packed.A > .5f ? TEXT("PM") : TEXT("AM"));
}

UMaterialParameterCollection* FS47Bodycam::Collection()
{
	static TWeakObjectPtr<UMaterialParameterCollection> Found;
	static bool bMissing = false;
	if (Found.IsValid()) return Found.Get();
	if (bMissing) return nullptr;
	// Older generated content has no MPC_Body: the bodycam choice then keeps the VHS look (S47Picture::ShaderMode).
	Found = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Web/Base/MPC_Body.MPC_Body"), {}, LOAD_NoWarn | LOAD_Quiet);
	bMissing = !Found.IsValid();
	return Found.Get();
}

bool FS47Bodycam::HasShader()
{
	const UMaterialParameterCollection* Mpc = Collection();
	return Mpc && Mpc->GetVectorParameterByName(TEXT("BodyLens")) && Mpc->GetVectorParameterByName(TEXT("BodyCam")) && Mpc->GetVectorParameterByName(TEXT("BodyStamp"));
}

void FS47Bodycam::Write(UWorld* World, const FLinearColor& Lens, const FLinearColor& Cam, const FLinearColor& StampV)
{
	static TWeakObjectPtr<UWorld> LastWorld;
	static FLinearColor LastLens(0, 0, 0, 0), LastCam(0, 0, 0, 0), LastStamp(0, 0, 0, 0);
	if (!World) return;
	if (LastWorld.Get() == World && Lens == LastLens && Cam == LastCam && StampV == LastStamp) return;
	UMaterialParameterCollectionInstance* I = Collection() ? World->GetParameterCollectionInstance(Collection()) : nullptr;
	if (!I) return;
	I->SetVectorParameterValue(TEXT("BodyLens"), Lens);
	I->SetVectorParameterValue(TEXT("BodyCam"), Cam);
	I->SetVectorParameterValue(TEXT("BodyStamp"), StampV);
	LastWorld = World; LastLens = Lens; LastCam = Cam; LastStamp = StampV;
}

bool FS47Bodycam::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bPass, const TCHAR* What, const FString& Got = FString())
	{
		Lines.Add(bPass ? FString::Printf(TEXT("ok  bodycam: %s"), What) : FString::Printf(TEXT("FAIL bodycam: %s %s"), What, *Got));
		bOk &= bPass;
	};
	// the burned-in stamp
	const FString Late = StampText(Stamp(4, 28, 1986, 23 * 3600 + 58 * 60 + 7));
	Check(Late == TEXT("04/28/86  11:58:07 PM"), TEXT("stamp before midnight, period character-generator style"), Late);
	const FString After = StampText(Stamp(4, 29, 1986, 86400 + 12 * 60 + 5));
	Check(After == TEXT("04/29/86  12:12:05 AM"), TEXT("stamp after midnight reads twelve, AM"), After);
	Check(StampText(Stamp(5, 3, 1986, 12 * 3600)) == TEXT("05/03/86  12:00:00 PM") && StampText(Stamp(12, 31, 1986, 3600 + 5 * 60 + 9)) == TEXT("12/31/86  01:05:09 AM"),
		TEXT("noon, and a one-digit hour with its zero"));
	bool bRuns = true;
	for (int32 S = 23 * 3600 + 59 * 60 + 50; S < 86400 + 10; ++S)
	{
		bRuns &= StampText(Stamp(4, 28, 1986, S)) != StampText(Stamp(4, 28, 1986, S + 1));
	}
	Check(bRuns, TEXT("the stamp follows the clock second by second through midnight (never freezes)"));
	// the mount and the lens
	auto Run = [](FS47Bodycam& B, FIn In, double Secs)
	{
		FOut O;
		for (double T = 0; T < Secs - 1e-9; T += 1.0 / 60.0) { In.Dt = 1.0 / 60.0; O = B.Step(In); }
		return O;
	};
	FIn Off; Off.bOn = false;
	FS47Bodycam B0;
	const FOut O0 = Run(B0, Off, 0.5);
	Check(O0.Mount == 0.0 && O0.Gate == 0.0 && O0.Fov == 1.f && O0.Lens == FLinearColor(0, 0, 0, 0) && O0.Cam == FLinearColor(0, 0, 0, 0),
		TEXT("other picture modes: no offset, the field of view exactly as before, no lens"));
	FIn On; On.bOn = true; On.bHeadMotion = false;
	FS47Bodycam B1;
	FIn Half = On; Half.Dt = 0.1;
	const FOut OHalf = B1.Step(Half);
	Check(FMath::IsNearlyEqual(OHalf.Mount, 0.5, 1e-6) && FMath::IsNearlyEqual(OHalf.Gate, 0.5, 1e-6), TEXT("the mount and lens fade in over 0.2 s"));
	const FOut O1 = Run(B1, On, 0.5);
	Check(O1.Mount == 1.0 && O1.Gate == 1.0 && FMath::IsNearlyEqual(O1.Fov, float(FovWide)) && O1.Offset == FVector(0, -Drop, Ahead) && O1.DownDeg == TiltDeg && O1.YawDeg == 0 && O1.RollDeg == 0,
		TEXT("chest mount: 20 cm down, 8 cm ahead, 3 degrees down, wider view; head motion off holds it still"));
	FIn Raised = On; Raised.bRaised = true;
	const FOut ORaised = Run(B1, Raised, 0.25);
	Check(ORaised.Mount == 0.0 && ORaised.Gate == 0.0 && ORaised.Fov == 1.f && ORaised.Lens.B == 0.f, TEXT("raising the field camera returns the eye and a plain lens (frame = photograph)"));
	FS47Bodycam B2;
	Run(B2, On, 0.5);
	FIn Screen = On; Screen.bScreen = true; Screen.bHeld = true;
	const FOut OScreen = Run(B2, Screen, 0.5);
	Check(OScreen.Mount == 0.0 && OScreen.Gate == 0.0 && OScreen.Fov == 1.f, TEXT("a world screen in use: no offset and a plain lens for its clicks"));
	FIn Back = On; Back.Dt = 1.0 / 60.0;
	const FOut OBack = B2.Step(Back);
	Check(OBack.Mount == 1.0, TEXT("back from a screen the camera is where the screen view returned it (no jump)"));
	FIn Truck = On; Truck.bHeld = true;
	const FOut OTruck = Run(B2, Truck, 0.5);
	Check(OTruck.Mount == 0.0 && OTruck.Gate == 0.0, TEXT("the truck's cab holds the camera: no offset, no lens"));
	// the sway, only with Head motion and bumps
	FIn Turning = On; Turning.Turn = 3.0 / 60.0; Turning.Strafe = 1; Turning.bStep = true; Turning.Bob = 1.0;
	FS47Bodycam B3;
	const FOut OStill = Run(B3, Turning, 0.5);
	Check(OStill.YawDeg == 0 && OStill.RollDeg == 0 && OStill.Offset == FVector(0, -Drop, Ahead) && OStill.DownDeg == TiltDeg, TEXT("head motion off: no sway at all"));
	Turning.bHeadMotion = true;
	FS47Bodycam B4;
	const FOut OSway = Run(B4, Turning, 0.5);
	Check(OSway.YawDeg < 0 && OSway.YawDeg >= -4.0 - 1e-6 && OSway.RollDeg > 0 && OSway.RollDeg <= 0.8 + 1e-6, TEXT("head motion on: the camera lags a turn (at most 4 degrees) and leans with a sidestep"));
	// calm and the gain
	FIn Lamp = On; Lamp.bLamp = false; Lamp.Turn = 3.0 / 60.0;
	FS47Bodycam B5;
	Run(B5, Lamp, 0.2);
	Lamp.bLamp = true;
	const FOut OKick = Run(B5, Lamp, 1.0 / 60.0);
	Check(OKick.Cam.R < 0.f && OKick.Cam.G > 0.f, TEXT("the flashlight's switch pumps the gain; a turn reaches the shader"));
	Lamp.bCalm = true; Lamp.bLamp = false;
	const FOut OCalm = Run(B5, Lamp, 1.0 / 60.0);
	Check(OCalm.Cam == FLinearColor(0, 0, 0, 0) && OCalm.Lens.B > 0.f, TEXT("Slow fades or Still shots: no skew, smear or gain pumping; the lens stays"));
	return bOk;
}

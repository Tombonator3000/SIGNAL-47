#include "S47ArrayShot.h"

#include "S47Flow.h"
#include "S47Main.h"
#include "S47Walker.h"
#include "S47World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47ArrayShot, Log, All);

// (Unreal only, on Tom's request: see S47ArrayShot.h. Nothing here is in the web version.)
const FVector FS47ArrayShot::TARGET(-6, 9, -70);
const FVector FS47ArrayShot::WINDOW_EYE0(-0.9, 1.66, -3.55);
const FVector FS47ArrayShot::WINDOW_EYE1(-0.9, 1.7, -3.92);

namespace
{
	double Ease(double X) { X = FMath::Clamp(X, 0.0, 1.0); return X * X * (3 - 2 * X); }
	double Wrap(double A) { return FMath::Atan2(FMath::Sin(A), FMath::Cos(A)); }
}

bool FS47ArrayShot::InControlRoom(const FVector& P)
{
	// ControlRoom.ts bounds (minX -5.75, maxX 5.75, minZ -4.22, maxZ 4.22)
	return P.X > -5.75 && P.X < 5.75 && P.Z > -4.22 && P.Z < 4.22;
}

FS47ArrayShot::FPose FS47ArrayShot::ShotPose(bool bAtWindow, double T, const FVector& LiveEye, double InFov0)
{
	const double Push = Ease(T / PUSH);
	FPose P;
	P.Eye = bAtWindow ? FMath::Lerp(WINDOW_EYE0, WINDOW_EYE1, Push) : LiveEye;
	// held, not on rails: a slow breath in the eye and the angles
	P.Eye.Y += 0.005 * FMath::Sin(T * 0.9);
	const FVector D = TARGET - P.Eye;
	P.Yaw = FMath::Atan2(-D.X, -D.Z) + 0.0022 * FMath::Sin(T * 0.53 + 0.4);
	P.Pitch = FMath::Atan2(D.Y, FMath::Sqrt(D.X * D.X + D.Z * D.Z)) + 0.0016 * FMath::Sin(T * 0.71);
	P.Roll = 0.003 * FMath::Sin(T * 0.37);
	P.Fov = InFov0 * (1 - (bAtWindow ? NARROW_WINDOW : NARROW_TURN) * Push);
	return P;
}

FS47ArrayShot::FPose FS47ArrayShot::Blend(const FPose& A, const FPose& B, double K, double Arc)
{
	FPose P;
	P.Eye = FMath::Lerp(A.Eye, B.Eye, K) + FVector(0, Arc * FMath::Sin(UE_DOUBLE_PI * K), 0);
	P.Yaw = A.Yaw + Wrap(B.Yaw - A.Yaw) * K;
	P.Pitch = FMath::Lerp(A.Pitch, B.Pitch, K);
	P.Roll = FMath::Lerp(A.Roll, B.Roll, K);
	P.Fov = FMath::Lerp(A.Fov, B.Fov, K);
	return P;
}

FS47ArrayShot::FPose FS47ArrayShot::Live() const
{
	// the walker's own view, as its frame places it (without the bob and the shake: it stands still)
	FPose P;
	if (const AS47Walker* W = Hooked.Get())
	{
		P.Eye = FVector(W->Pos.X, W->FloorY + W->Eye, W->Pos.Z);
		P.Yaw = W->Yaw; P.Pitch = W->Pitch;
	}
	P.Fov = Fov0;
	return P;
}

void FS47ArrayShot::Begin()
{
	AS47Walker* W = M ? M->Player.Get() : nullptr;
	if (!W) return;
	Unhook();
	Fov0 = M->Flow ? M->Flow->BaseFov : W->Fov;
	bWindow = InControlRoom(W->Pos) && (!M->World || M->World->Area == TEXT("saro"));
	T = 0; TOut = 0; KOut = 0; Stage = EStage::In;
	// the walker's camera, held as the cab holds it; whatever held it before has it back after
	Before = W->CameraOverride;
	TFunction<bool(double, const FVector2D&, const FVector2D&, FS47View&)> Prev = Before;
	TWeakPtr<FS47ArrayShot> Wk = AsShared();
	W->CameraOverride = [Wk, Prev](double Dt, const FVector2D& Move, const FVector2D& LookIn, FS47View& Out)
	{
		if (const TSharedPtr<FS47ArrayShot> S = Wk.Pin()) if (S->Frame(Dt, Out)) return true;
		return Prev ? Prev(Dt, Move, LookIn, Out) : false;
	};
	Hooked = W;
	UE_LOG(LogS47ArrayShot, Display, TEXT("S47: array shot: %s"), bWindow ? TEXT("to the window") : TEXT("turning in place"));
}

void FS47ArrayShot::Release()
{
	if (Stage != EStage::In) return;
	// from wherever the way in has got to (all the way, as a rule)
	KOut = Ease(T / BLEND_IN);
	Stage = EStage::Out; TOut = 0;
	UE_LOG(LogS47ArrayShot, Display, TEXT("S47: array shot: the view given back, %.1f s in"), T);
}

void FS47ArrayShot::Stop()
{
	if (Stage != EStage::Off) { if (AS47Walker* W = Hooked.Get()) W->Fov = float(Fov0); }
	Stage = EStage::Off;
	Unhook();
}

void FS47ArrayShot::Tick()
{
	if (Stage == EStage::Off && Hooked.IsValid()) Unhook();
}

void FS47ArrayShot::Unhook()
{
	if (AS47Walker* W = Hooked.Get()) W->CameraOverride = Before;
	Hooked.Reset();
	Before = nullptr;
}

bool FS47ArrayShot::Frame(double Dt, FS47View& Out)
{
	AS47Walker* W = Hooked.Get();
	if (Stage == EStage::Off || !W) return false;
	T += Dt;
	if (Stage == EStage::In && T > MAX_HOLD) Release();
	double K;
	if (Stage == EStage::In) K = Ease(T / BLEND_IN);
	else
	{
		TOut += Dt;
		if (TOut >= BLEND_OUT)
		{
			// the walker's own from this frame on (Tick puts its camera back in its hands)
			Stage = EStage::Off;
			W->Fov = float(Fov0);
			UE_LOG(LogS47ArrayShot, Display, TEXT("S47: array shot: over"));
			return false;
		}
		K = KOut * (1 - Ease(TOut / BLEND_OUT));
	}
	const FPose L = Live();
	const FPose P = Blend(L, ShotPose(bWindow, T, L.Eye, Fov0), K, bWindow ? 0.12 : 0.0);
	Out.Eye = P.Eye; Out.Yaw = P.Yaw; Out.Pitch = P.Pitch; Out.Roll = P.Roll;
	W->Fov = float(P.Fov);
	return true;
}

bool FS47ArrayShot::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s array shot: %s"), bCond ? TEXT("PASS") : TEXT("FAIL"), *What)); bOk &= bCond; };
	auto Fwd = [](const FPose& P) { return FVector(-FMath::Sin(P.Yaw) * FMath::Cos(P.Pitch), FMath::Sin(P.Pitch), -FMath::Cos(P.Yaw) * FMath::Cos(P.Pitch)); };
	// at the window, all the way in: the eye eased to the glass, the field of view narrowed, looking at the array
	const FPose S = ShotPose(true, PUSH, FVector(3.35, 1.62, 2.75), 70);
	Check(FVector::Dist(S.Eye, WINDOW_EYE1) < 0.01, TEXT("the eye at the window by the end of the push"));
	Check(FMath::IsNearlyEqual(S.Fov, 70 * (1 - NARROW_WINDOW), 1e-6), FString::Printf(TEXT("the field of view narrowed to %.1f"), S.Fov));
	const double Off = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Fwd(S), (TARGET - S.Eye).GetSafeNormal()), -1.0, 1.0)));
	Check(Off < 0.5, FString::Printf(TEXT("looking at the array (%.2f degrees off)"), Off));
	// the line to the array goes out through a pane: between the mullions at x -1.8 and 0, under the head and over the sill
	for (const FVector& E : { WINDOW_EYE0, WINDOW_EYE1 })
	{
		const FVector D = TARGET - E;
		const double U = (-4.56 - E.Z) / D.Z;
		const FVector G = E + D * U;
		Check(G.X > -1.75 && G.X < -0.05 && G.Y > 1.0 && G.Y < 2.7, FString::Printf(TEXT("through the glass at x %.2f, y %.2f"), G.X, G.Y));
	}
	// turning in place: the eye stays where the player is
	const FPose Tn = ShotPose(false, 3, FVector(20, 1.62, -12), 70);
	Check(FMath::Abs(Tn.Eye.X - 20) < 1e-9 && FMath::Abs(Tn.Eye.Z + 12) < 1e-9 && Tn.Fov < 70, TEXT("away from the room only the view turns"));
	// the blend: its ends, and the angles the short way round
	FPose A, B; A.Yaw = UE_DOUBLE_PI - 0.1; B.Yaw = -UE_DOUBLE_PI + 0.1; A.Fov = 70; B.Fov = 40; B.Eye = FVector(1, 2, 3);
	const FPose H = Blend(A, B, 0.5, 0.12);
	Check(FMath::Abs(Wrap(H.Yaw - UE_DOUBLE_PI)) < 1e-6 && FMath::IsNearlyEqual(H.Fov, 55.0) && FMath::IsNearlyEqual(H.Eye.Y, 1.12), TEXT("the blend turns the short way, along a low arc"));
	Check(FVector::Dist(Blend(A, B, 1, 0.12).Eye, B.Eye) < 1e-9 && Blend(A, B, 0, 0.12).Fov == 70, TEXT("the blend's ends are its poses"));
	Check(InControlRoom(FVector(-0.9, 1.6, -3.5)) && InControlRoom(FVector(3.35, 1.6, 2.75)) && !InControlRoom(FVector(11.2, 1.6, -11)), TEXT("the control room's bounds"));
	// the dishes' turn in this scene: from the survey (az 83, el 48) to 026 / 32, twice the web version's slew
	auto Secs = [](double Slew) { return FMath::Max((83.0 - 26.0) / Slew, (48.0 - 32.0) / (Slew * 0.6)); };
	Check(FMath::IsNearlyEqual(Secs(TURN_SLEW), Secs(WEB_SLEW) / 2) && Secs(TURN_SLEW) < PUSH, FString::Printf(TEXT("the turn takes %.1f s (the web version's %.1f), inside the push"), Secs(TURN_SLEW), Secs(WEB_SLEW)));
	return bOk;
}

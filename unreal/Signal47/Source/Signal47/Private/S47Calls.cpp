// core/calls.ts
#include "S47Calls.h"

#include "S47Audio.h"

namespace
{
	const TArray<FString> CALLS = { TEXT("alienVoice"), TEXT("alienRadio"), TEXT("alienCalls"), TEXT("alienSong"), TEXT("alienTemple") };
	const TCHAR* RARE = TEXT("alienHorn");
	double Rnd(double A, double B) { return A + FMath::FRand() * (B - A); }
}

FS47NightCalls::FS47NightCalls(US47Audio* InAudio) : Audio(InAudio)
{
	Next = Rnd(45, 100);
}

void FS47NightCalls::Update(double Dt, const FS47CallsWhen& W)
{
	auto Flicker = [&W](double K) { if (W.Flicker) W.Flicker(K); };
	// the stutter fades out after a call
	if (Stutter > 0) { Stutter = FMath::Max(0.0, Stutter - Dt * 0.35); Flicker(Stutter); }
	if (!W.bOk) { if (Lead > 0) { Lead = 0; Flicker(0); Stutter = 0; } return; }
	if (Lead > 0)
	{
		Lead -= Dt;
		Flicker(FMath::Min(1.0, 0.4 + (1.8 - Lead) * 0.4));
		if (Lead <= 0) Call(W.Cam);
		return;
	}
	// (the night's curve says when: in its build and its peak, never in the quiet after)
	if (W.May == TOptional<bool>(false) && !bForced) return;
	Next -= Dt * W.Pace.Get(1);
	if (Next > 0) return;
	bForced = false;
	// first the quiet: the crickets and the wind drop away, the light starts to stutter
	Lead = 1.8;
	if (W.Hush) W.Hush(10);
	Next = Rnd(90, 220);
}

void FS47NightCalls::Call(const FS47Cam& Cam)
{
	US47Audio* A = Audio.Get();
	if (!A) return;
	const FString Tone = A->Or(TEXT("call_tone"));
	TArray<FString> Calls = CALLS;
	if (!Tone.IsEmpty()) Calls.Add(Tone);
	FString Name = FMath::FRand() < 0.12 ? FString(RARE) : Calls[FMath::Min(Calls.Num() - 1, int32(FMath::FRand() * Calls.Num()))];
	if (Name == Before) Name = Calls[(Calls.IndexOfByKey(Name) + 1) % Calls.Num()]; // (indexOf -1 for the horn: the first)
	Before = Name;
	// behind the player or off to one side, out of the beam, and far
	const FVector F = Cam.Forward;
	const double Yaw = FMath::Atan2(F.X, F.Z);
	const double Off = (FMath::FRand() < 0.5 ? -1 : 1) * Rnd(1.6, UE_DOUBLE_PI);   // 90 to 180 degrees round
	const double Dist = Name == RARE ? Rnd(90, 140) : Rnd(35, 110);
	const FVector At = Cam.Pos + FVector(FMath::Sin(Yaw + Off) * Dist, Rnd(-0.5, 4), FMath::Cos(Yaw + Off) * Dist);
	const TSharedPtr<FS47AudioNode> Pan = A->Spatial(At, 28);
	if (!Pan.IsValid()) return;
	FS47Sound O;
	O.DestNode = Pan; O.Gain = Name == RARE ? 0.55f : 0.75f; O.Rate = float(Rnd(0.86, 1.04)); O.Lp = float(FMath::Clamp(4200 - Dist * 22, 1100.0, 3600.0));
	A->Sfx(Name, O);
	Stutter = 1;
	Count++;
	Last = FLast{ Name, At, Dist, FMath::Abs(Off) };
}

bool FS47NightCalls::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s calls: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	FS47NightCalls C(nullptr);
	double Hushed = 0, Flick = 0;
	FS47CallsWhen W;
	W.bOk = true; W.Hush = [&Hushed](double S) { Hushed = S; }; W.Flicker = [&Flick](double K) { Flick = K; };
	W.May = false;
	C.Update(200, W);
	Check(Hushed == 0 && C.Next > 0, TEXT("nothing while the curve says no"));
	C.Now();
	C.Update(0.016, W);
	Check(Hushed == 10 && C.Next >= 90 && C.Next <= 220, TEXT("asked for: the night goes quiet for 10 s first"));
	C.Update(0.9, W);
	Check(FMath::IsNearlyEqual(Flick, 0.4 + 0.9 * 0.4, 1e-9), TEXT("the flashlight stutters more and more before the call"));
	W.bOk = false;
	C.Update(0.1, W);
	Check(Flick == 0, TEXT("out of the night (indoors, the light off): the stutter stops"));
	return bOk;
}

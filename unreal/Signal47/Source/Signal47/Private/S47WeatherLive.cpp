#include "S47WeatherLive.h"

#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "S47Audio.h"
#include "S47Drive.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47Sky.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47WebAudio.h"
#include "S47World.h"
#include "S47WorldLife.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47WeatherLive, Log, All);

namespace S47WeatherLiveLocal
{
	// how fast what is shown follows the weather (seconds): it changes over the clock's minutes anyway; this smooths F10 and the doors
	constexpr double EaseSeconds = 6, DoorSeconds = 1.5, JumpSeconds = 600;
	// the most each adds to the native fog out in the open: the light mist of F10's "Fog: light" is .04 (about 100 m)
	constexpr double SandFog = .022, HazeFog = .010, RainFog = .003;
	// indoors most of it stays: the fog is the whole world's, and the land outside the control room's big windows must stay brown in the
	// sand (M5 critic 3; a small room shows almost no haze at that density)
	constexpr double IndoorShare = .65;
}

void FS47WeatherLive::Shown(const S47Weather::FNow& Now, double Daylight, double InOutdoors, S47Weather::FLive& Out)
{
	using namespace S47WeatherLiveLocal;
	Out.bOn = true;
	Out.Wind = Now.Wind; Out.Mph = Now.Mph; Out.Dust = Now.Dust; Out.Rain = Now.Rain; Out.Haze = Now.Haze; Out.Wet = Now.Wet;
	Out.CloudBias = Now.CloudBias; Out.Storm = Now.Storm; Out.DirX = Now.DirX; Out.DirZ = Now.DirZ;
	const double Day = FMath::Clamp(Daylight, 0.0, 1.0), Open = FMath::Clamp(InOutdoors, 0.0, 1.0);
	// how thick the air is where the player is: the sand most, the morning's haze, a little in a shower
	Out.FogAdd = FMath::Min(MaxFog, (SandFog * Now.Dust + HazeFog * Now.Haze + RainFog * Now.Rain) * FMath::Lerp(IndoorShare, 1.0, Open));
	// its colour: tan sand by day and a dark brown by night, the haze pale
	const double SandK = FMath::Clamp(Now.Dust * 1.3, 0.0, 1.0), HazeK = FMath::Clamp(Now.Haze, 0.0, 1.0) * (1 - SandK);
	const float Lit = float(FMath::Lerp(.05, 1.0, Day));
	const FLinearColor Sand = FLinearColor(.52f, .40f, .26f) * Lit, Pale = FLinearColor(.58f, .60f, .63f) * Lit;
	Out.Tint = SandK + HazeK > 1e-4 ? (Sand * float(SandK) + Pale * float(HazeK)) * float(1 / (SandK + HazeK)) : Sand;
	Out.TintK = FMath::Clamp(.85 * SandK + .6 * HazeK, 0.0, .9);
	// the far land fades: the mesas go first in the sand
	Out.HorizonK = FMath::Clamp(.55 * Now.Dust + .3 * Now.Haze + .08 * Now.Rain, 0.0, .6);
}

void FS47WeatherLive::Ease(S47Weather::FNow& Cur, const S47Weather::FNow& Want, double K)
{
	const double A = FMath::Clamp(K, 0.0, 1.0);
	for (const TPair<double*, double> P : { TPair<double*, double>(&Cur.Wind, Want.Wind), TPair<double*, double>(&Cur.Mph, Want.Mph), TPair<double*, double>(&Cur.Dust, Want.Dust),
		TPair<double*, double>(&Cur.Rain, Want.Rain), TPair<double*, double>(&Cur.Haze, Want.Haze), TPair<double*, double>(&Cur.Wet, Want.Wet),
		TPair<double*, double>(&Cur.CloudBias, Want.CloudBias), TPair<double*, double>(&Cur.Storm, Want.Storm) })
		*P.Key += (P.Value - *P.Key) * A;
	FVector2D Dir(Cur.DirX + (Want.DirX - Cur.DirX) * A, Cur.DirZ + (Want.DirZ - Cur.DirZ) * A);
	Dir = Dir.SizeSquared() > 1e-8 ? Dir.GetSafeNormal() : FVector2D(Want.DirX, Want.DirZ);
	Cur.DirX = Dir.X; Cur.DirZ = Dir.Y;
	Cur.bThunder = Want.bThunder;
}

void FS47WeatherLive::Override(EDev Kind, S47Weather::FNow& Now)
{
	switch (Kind)
	{
	case EDev::Sand: Now.Wind = FMath::Max(Now.Wind, .8); Now.Mph = FMath::Max(Now.Mph, 30.0); Now.Dust = 1; Now.Storm = FMath::Max(Now.Storm, .8); Now.CloudBias = -.12; Now.Rain = 0; Now.Wet = 0; Now.Haze = 0; break;
	case EDev::Shower: Now.Rain = 1; Now.Wet = 1; Now.CloudBias = .24; Now.bThunder = true; Now.Dust = 0; break;
	case EDev::Haze: Now.Haze = 1; Now.Dust = 0; Now.Wind = 0; Now.Mph = 4; break;
	case EDev::Calm: Now = S47Weather::FNow(); break;
	default: break;
	}
}

void FS47WeatherLive::Dev(EDev Kind, double Seconds)
{
	DevWhat = Kind; DevLeft = Kind == EDev::None ? 0 : FMath::Max(0.0, Seconds);
	bHave = false; // shown at once, not eased in
	UE_LOG(LogS47WeatherLive, Display, TEXT("S47: weather: developer %d for %.0f s"), int32(Kind), DevLeft);
}

void FS47WeatherLive::Stop()
{
	S47Weather::Live() = S47Weather::FLive();
	bHave = false; LastClock = -1; DevLeft = 0; DevWhat = EDev::None;
	if (Spray.IsValid() && M && M->Audio && M->Audio->Ctx) { FS47AudioContext::FBatch Batch(*M->Audio->Ctx); Spray->Gain.SetTargetAtTime(0, M->Audio->Ctx->CurrentTime(), 0.3); }
	if (M && M->Life && M->Life->Sky.IsValid()) M->Life->Sky->DayThunder = 0;
	if (FogApplied > 0 && M && M->Web) M->Web->RefreshFog();
	FogApplied = -1; TintApplied = -1;
}

void FS47WeatherLive::Step(double Dt)
{
	using namespace S47WeatherLiveLocal;
	FS47FreeformSession* F = M ? M->Freeform.Get() : nullptr;
	if (!bCmdChecked) { bCmdChecked = true; bCmd = FParse::Param(FCommandLine::Get(), TEXT("S47Weather")); }
	if (!M || !F || !M->IsFreeform() || !F->Active() || M->Mode != US47Main::EMode::Play || !(F->RulesLive() || bCmd || bForceLive || DevLeft > 0))
	{
		if (S47Weather::Live().bOn || bHave) Stop();
		return;
	}
	const double Clock = F->Clock();
	CalDay = int32(FMath::FloorToDouble(Clock / 86400.0)) + 1;
	S47Weather::FNow Want = S47Weather::Now(F->Seed(), CalDay, FMath::Fmod(Clock, 86400.0));
	if (DevLeft > 0) { DevLeft = FMath::Max(0.0, DevLeft - FMath::Max(0.0, Dt)); Override(DevWhat, Want); }
	// a rest or a test that moves the clock: the weather of the new hour at once
	const bool bJump = !bHave || FMath::Abs(Clock - LastClock) > JumpSeconds;
	if (bJump) Cur = Want;
	else Ease(Cur, Want, 1 - FMath::Exp(-FMath::Max(0.0, Dt) / EaseSeconds));
	LastClock = Clock; bHave = true;
	// out in the open or in the cab it is all there; indoors most of it, seen through the windows (IndoorShare)
	const double OpenWant = M->Space == TEXT("yard") || (M->World && M->World->bDriving) ? 1.0 : 0.0;
	Outdoors = bJump ? OpenWant : Outdoors + (OpenWant - Outdoors) * (1 - FMath::Exp(-FMath::Max(0.0, Dt) / DoorSeconds));
	const double Daylight = M->Life && M->Life->Sky.IsValid() ? M->Life->Sky->Daylight() : 1.0;
	S47Weather::FLive& L = S47Weather::Live();
	Shown(Cur, Daylight, Outdoors, L);
	if (M->Life && M->Life->Sky.IsValid()) M->Life->Sky->DayThunder = Cur.bThunder ? 1.0 : 0.0;
	// the native fog is set when its colour changes; the weather tells it when the air does
	if (M->Web && (FMath::Abs(L.FogAdd - FogApplied) > .0004 || FMath::Abs(L.TintK - TintApplied) > .01))
	{
		FogApplied = L.FogAdd; TintApplied = L.TintK;
		M->Web->RefreshFog();
	}
	StepWet();
	if (LoggedDay != CalDay)
	{
		LoggedDay = CalDay;
		UE_LOG(LogS47WeatherLive, Display, TEXT("S47: weather day %d: %s"), CalDay, *S47Weather::Words(S47Weather::Day(F->Seed(), CalDay)));
	}
}

void FS47WeatherLive::StepWet()
{
	// M5 critic 3: after a shower the dirt is slick (FS47TruckCare takes the grip); here it is heard, a hiss of wet grit off the tyres
	// with the speed, and the first slide on it says why, once a day
	const S47Weather::FLive& L = S47Weather::Live();
	FS47DriveController* D = M->World && M->World->bDriving ? M->World->CabDrive() : nullptr;
	const bool bLoose = D && D->Surface.Kind != ES47SurfaceKind::Asphalt;
	const double Wet = L.bOn && bLoose ? FMath::Clamp(L.Wet, 0.0, 1.0) : 0.0;
	const double Want = Wet > 0.12 ? 0.055 * Wet * FMath::Clamp(FMath::Abs(D->Speed) / 12.0, 0.0, 1.0) : 0.0;
	US47Audio* Audio = M->Audio.Get();
	if (Audio && Audio->Ctx && Audio->SfxBus && Audio->NoiseBuffer())
	{
		const TSharedRef<FS47AudioContext> Ctx = Audio->Ctx.ToSharedRef();
		FS47AudioContext::FBatch Batch(*Ctx);
		if (!Spray.IsValid() && Want > 0)
		{
			auto Src = Ctx->CreateBufferSource(); Src->Buffer = Audio->NoiseBuffer(); Src->bLoop = true;
			auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Q.SetValue(0.6); Bp->Frequency.SetValue(2300);
			Spray = Ctx->CreateGain(); Spray->Gain.SetValue(0);
			Src->Connect(Bp); Bp->Connect(Spray); Spray->Connect(Audio->SfxBus); Src->Start(Ctx->CurrentTime(), FMath::FRand());
		}
		if (Spray.IsValid()) Spray->Gain.SetTargetAtTime(Want, Ctx->CurrentTime(), 0.25);
	}
	if (D && D->bHandling && Wet > 0.25 && D->Slip() > 0.25 && ToldWet != CalDay && M->UI)
	{
		ToldWet = CalDay;
		M->UI->Toast(TEXT("The back end steps out. The shower has turned the dirt to grease."), 4);
	}
}

FString FS47WeatherLive::Describe() const
{
	const S47Weather::FLive& L = S47Weather::Live();
	if (!L.bOn) return TEXT("no weather on the world (not a live Freeform game: F10 or -S47Weather turns it on)");
	static const TCHAR* const Kinds[] = { TEXT("the day's own"), TEXT("sand (F10)"), TEXT("shower (F10)"), TEXT("haze (F10)"), TEXT("calm (F10)") };
	return FString::Printf(TEXT("day %d, %s: wind %.0f mph (%.2f), sand %.2f, rain %.2f, wet %.2f, haze %.2f, cloud %+.2f, night storm %.2f; fog +%.4f, tint %.2f, horizon %.2f"),
		CalDay, Kinds[int32(DevNow())], L.Mph, L.Wind, L.Dust, L.Rain, L.Wet, L.Haze, L.CloudBias, L.Storm, L.FogAdd, L.TintK, L.HorizonK);
}

bool FS47WeatherLive::SelfTest(TArray<FString>& Lines)
{
	bool OK = true;
	const auto Check = [&](bool B, const FString& Why) { OK &= B; Lines.Add(FString::Printf(TEXT("%s weather live: %s"), B ? TEXT("PASS") : TEXT("FAIL"), *Why)); };
	S47Weather::FLive Out;
	Shown(S47Weather::FNow(), 1, 1, Out);
	Check(Out.bOn && Out.FogAdd == 0 && Out.TintK == 0 && Out.HorizonK == 0, TEXT("a plain day adds nothing to the air"));
	S47Weather::FNow Sand; Sand.Dust = 1; Sand.Wind = 1; Sand.Mph = 34;
	S47Weather::FLive Open, In, Night;
	Shown(Sand, 1, 1, Open); Shown(Sand, 1, 0, In); Shown(Sand, 0, 1, Night);
	Check(Open.FogAdd > .015 && Open.FogAdd <= MaxFog && In.FogAdd < Open.FogAdd && In.FogAdd > Open.FogAdd * .5, TEXT("sand thickens the air out in the open, and the land outside the windows stays in it from indoors, never past a light mist"));
	Check(Open.Tint.R > Open.Tint.G && Open.Tint.G > Open.Tint.B && Open.TintK > .7 && Night.Tint.R < Open.Tint.R * .2 && Open.HorizonK > .4, TEXT("sand is tan by day and dark at night, and the far land fades in it"));
	S47Weather::FNow Haze; Haze.Haze = 1; S47Weather::FLive Hz;
	Shown(Haze, 1, 1, Hz);
	Check(Hz.FogAdd > 0 && Hz.FogAdd < Open.FogAdd && FMath::Abs(Hz.Tint.R - Hz.Tint.B) < .1f, TEXT("the morning haze is thinner than sand, and pale"));
	{
		// over whole days of a game nothing is ever thicker than the light mist, and something shows on some of them
		double Most = 0; int32 Shown_ = 0;
		for (int32 Num = 1; Num <= 30; ++Num) for (double H = 0; H < 24; H += .5)
		{
			S47Weather::FLive Each;
			Shown(S47Weather::Now(470047, Num, H * 3600), 1, 1, Each);
			Most = FMath::Max(Most, Each.FogAdd); Shown_ += Each.FogAdd > .005 ? 1 : 0;
		}
		Check(Most <= MaxFog && Shown_ > 0, FString::Printf(TEXT("thirty days of seed 470047: the thickest air +%.4f, thick at %d half hours"), Most, Shown_));
	}
	{
		S47Weather::FNow Cur, Want; Want.Dust = 1; Want.DirX = 0; Want.DirZ = 1; Cur.DirX = 1; Cur.DirZ = 0;
		Ease(Cur, Want, .5);
		const double Half = Cur.Dust;
		Ease(Cur, Want, 1);
		Check(FMath::IsNearlyEqual(Half, .5) && Cur.Dust == 1 && FMath::IsNearlyEqual(FVector2D(Cur.DirX, Cur.DirZ).Size(), 1.0, 1e-6) && FMath::IsNearlyEqual(Cur.DirZ, 1.0, 1e-6), TEXT("what is shown eases to the weather and turns with the wind"));
	}
	{
		S47Weather::FNow N; Override(EDev::Sand, N);
		S47Weather::FNow R; Override(EDev::Shower, R);
		S47Weather::FNow C = N; Override(EDev::Calm, C);
		Check(N.Dust == 1 && N.Wind >= .8 && R.Rain == 1 && R.Wet == 1 && R.bThunder && C.Dust == 0 && C.Wind == 0, TEXT("F10's sand, shower and calm"));
	}
	return OK;
}

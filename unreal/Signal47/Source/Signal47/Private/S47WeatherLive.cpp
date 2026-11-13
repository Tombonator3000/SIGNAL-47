#include "S47WeatherLive.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "S47Audio.h"
#include "S47Drive.h"
#include "S47Freeform.h"
#include "S47FreeformTerrain.h"
#include "S47Main.h"
#include "S47Sky.h"
#include "S47Thoughts.h"
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
	Out.CloudBias = Now.CloudBias; Out.Storm = Now.Storm; Out.DirX = Now.DirX; Out.DirZ = Now.DirZ; Out.ShowerH = Now.ShowerH;
	Out.Lightning = Now.Lightning; Out.LightningAz = Now.LightningAz; // D46-20261110: the night's lightning, for FS47Sky
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
	Cur.ShowerH = Want.ShowerH; // (a clock, not a quantity: the cloud's walk is already smooth)
	Cur.Lightning = Want.Lightning; Cur.LightningAz = Want.LightningAz; // (D46-20261110: read a flash at a time, nothing to smooth)
}

void FS47WeatherLive::Override(EDev Kind, S47Weather::FNow& Now, double ShowerHours)
{
	switch (Kind)
	{
	case EDev::Sand: Now.Wind = FMath::Max(Now.Wind, .8); Now.Mph = FMath::Max(Now.Mph, 30.0); Now.Dust = 1; Now.Storm = FMath::Max(Now.Storm, .8); Now.CloudBias = -.12; Now.Rain = 0; Now.Wet = 0; Now.Haze = 0; break;
	case EDev::Shower:
	{
		// SHOWER-LOOK-20261109: the shower's own course, as a shower day has it, so the rain stops and the dirt dries after it
		double Rain = 0, Wet = 0, Cloud = 0; bool bThunder = false;
		S47Weather::ShowerCourse(ShowerHours, true, Rain, Wet, Cloud, bThunder);
		Now.Rain = Rain; Now.Wet = Wet; Now.CloudBias = FMath::Max(Now.CloudBias, Cloud); Now.bThunder = bThunder; Now.ShowerH = ShowerHours; Now.Dust = 0;
		break;
	}
	case EDev::Haze: Now.Haze = 1; Now.Dust = 0; Now.Wind = 0; Now.Mph = 4; break;
	case EDev::Calm: Now = S47Weather::FNow(); Now.Lightning = 0; break;
	case EDev::Lightning: Now.Lightning = 2.2; Now.Dust = 0; Now.Rain = 0; break; // D46-20261110
	default: break;
	}
}

void FS47WeatherLive::Dev(EDev Kind, double Seconds, double ShowerHours)
{
	DevWhat = Kind; DevLeft = Kind == EDev::None ? 0 : FMath::Max(0.0, Seconds);
	DevShowerH = FMath::IsFinite(ShowerHours) ? ShowerHours : DevShowerHours; DevShowerFrom = -1; // its course starts on the next frame's clock
	bHave = false; // shown at once, not eased in
	UE_LOG(LogS47WeatherLive, Display, TEXT("S47: weather: developer %d for %.0f s"), int32(Kind), DevLeft);
}

void FS47WeatherLive::Stop()
{
	S47Weather::Live() = S47Weather::FLive();
	bHave = false; LastClock = -1; DevLeft = 0; DevWhat = EDev::None; DevShowerFrom = -1;
	ApplyGroundWet(0);
	if (Spray.IsValid() && M && M->Audio && M->Audio->Ctx) { FS47AudioContext::FBatch Batch(*M->Audio->Ctx); Spray->Gain.SetTargetAtTime(0, M->Audio->Ctx->CurrentTime(), 0.3); }
	if (M && M->Life && M->Life->Sky.IsValid()) M->Life->Sky->DayThunder = 0;
	if (FogApplied > 0 && M && M->Web) M->Web->RefreshFog();
	FogApplied = -1; TintApplied = -1;
	OutFor = 0; // (ThoughtNight stays: a panel opened does not give the night its thought again)
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
	if (DevLeft > 0)
	{
		DevLeft = FMath::Max(0.0, DevLeft - FMath::Max(0.0, Dt));
		// the shower's course runs on the game's clock from where F10 put it (26 game seconds to one by day)
		if (DevShowerFrom < 0) DevShowerFrom = Clock - DevShowerH * 3600.0;
		Override(DevWhat, Want, (Clock - DevShowerFrom) / 3600.0);
	}
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
	ApplyGroundWet(GroundWet(L));
	StepNightThought(Dt);
	if (LoggedDay != CalDay)
	{
		LoggedDay = CalDay;
		UE_LOG(LogS47WeatherLive, Display, TEXT("S47: weather day %d: %s"), CalDay, *S47Weather::Words(S47Weather::Day(F->Seed(), CalDay)));
	}
}

void FS47WeatherLive::StepNightThought(double Dt)
{
	// M5 (13 November): the moon and the blowing sand change how far the small ones see the player (S47Weather::SightScale, through
	// FS47RoswellGroup); so that it is noticed, once a night, half a minute out on foot in the dark, the player's own thought says what
	// kind of night it is for being seen. Nothing on an ordinary night, nothing in a developer run.
	FS47FreeformSession* F = M ? M->Freeform.Get() : nullptr;
	const bool bOnFoot = F && F->RulesLive() && M->Web && M->Space == TEXT("yard") && !(M->World && M->World->bDriving);
	const double Daylight = M && M->Life && M->Life->Sky.IsValid() ? M->Life->Sky->Daylight() : 1.0;
	if (!bOnFoot || Daylight > .05) { OutFor = 0; return; }
	OutFor += FMath::Max(0.0, Dt);
	if (OutFor < 25) return;
	const int32 Night = int32(FMath::FloorToDouble((F->Clock() - 12 * 3600.0) / 86400.0));
	if (Night == ThoughtNight) return;
	const S47Weather::FLive& L = S47Weather::Live();
	const TCHAR* Key = S47Weather::NightThoughtKey(M->Web->MoonLightNow(), L.bOn ? L.Dust : 0.0);
	if (!*Key) return;
	ThoughtNight = Night;
	S47Thoughts::Later(Key, 1.5);
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

void FS47WeatherLive::ApplyGroundWet(double Wet)
{
	// SHOWER-LOOK-20261109: the ground darkens where the shower has been and dries again with the dirt's own Wet (the material's
	// patches dry first on the rises, last in the washes). Written when it changes, and again when the ground's instance is new.
	// A ground material without WetK (not yet imported) takes no harm: the parameter is simply not there.
	if (!M || !M->Web) return;
	UMaterialInstanceDynamic* Ground = S47FreeformTerrain::GroundMid(M->Web);
	const bool bNew = Ground && WetSeen.Get() != Ground;
	if (!bNew && WetApplied >= 0 && FMath::Abs(Wet - WetApplied) < .004 && !(Wet == 0 && WetApplied > 0)) return;
	if (!Ground && WetApplied < 0) return;
	WetSeen = Ground; WetApplied = Wet;
	for (UMaterialInstanceDynamic* Mid : { Ground, S47FreeformTerrain::CorridorMid(M->Web), S47FreeformTerrain::RangeMid(M->Web) })
		if (Mid) Mid->SetScalarParameterValue(TEXT("WetK"), float(Wet));
	// (the road edge copies the ground's parameters, WetK with them: S47RoadEdge EdgeTone)
}

FString FS47WeatherLive::Describe() const
{
	const S47Weather::FLive& L = S47Weather::Live();
	if (!L.bOn) return TEXT("no weather on the world (not a live Freeform game: F10 or -S47Weather turns it on)");
	static const TCHAR* const Kinds[] = { TEXT("the day's own"), TEXT("sand (F10)"), TEXT("shower (F10)"), TEXT("haze (F10)"), TEXT("calm (F10)"), TEXT("lightning (F10)") };
	return FString::Printf(TEXT("day %d, %s: wind %.0f mph (%.2f), sand %.2f, rain %.2f, wet %.2f, haze %.2f, cloud %+.2f, night storm %.2f, lightning %.2f at %.0f deg; fog +%.4f, tint %.2f, horizon %.2f"),
		CalDay, Kinds[int32(DevNow())], L.Mph, L.Wind, L.Dust, L.Rain, L.Wet, L.Haze, L.CloudBias, L.Storm, L.Lightning, L.LightningAz, L.FogAdd, L.TintK, L.HorizonK);
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
	{
		// D46-20261110 (D46-4): the sky's lightning follows the night's weather: none on a quiet night, the sky's own far-off flicker
		// at 1, a storm on the horizon flashing far more often where the weather stands it, with thunder; no weather, the sky's own
		const S47Weather::FLive Was = S47Weather::Live();
		struct FCount { int32 Flashes = 0, Thunder = 0, Near = 0; };
		const auto TenMinutes = [](bool bOn, double Lightning, double Az)
		{
			S47Weather::FLive& L = S47Weather::Live();
			L = S47Weather::FLive(); L.bOn = bOn; L.Lightning = Lightning; L.LightningAz = Az;
			FS47Sky Sky; FCount C; bool bWas = false;
			Sky.OnThunder = [&C](double, double) { ++C.Thunder; };
			for (int32 Fr = 0; Fr < 30 * 600; ++Fr)
			{
				Sky.Update(1 / 30.0, Fr / 30.0);
				const bool bLit = Sky.Flash > 0;
				if (bLit && !bWas)
				{
					++C.Flashes;
					const double FlashAz = FMath::RadiansToDegrees(FMath::Atan2(Sky.FlashDir.X, -Sky.FlashDir.Z));
					C.Near += FMath::Abs(FMath::FindDeltaAngleDegrees(FlashAz, Az)) < 15 ? 1 : 0;
				}
				bWas = bLit;
			}
			return C;
		};
		const FCount Quiet = TenMinutes(true, 0, 225), Far = TenMinutes(true, 1, 225), Storm = TenMinutes(true, 2.2, 250), Own = TenMinutes(false, 0, 225);
		S47Weather::Live() = Was;
		Check(Quiet.Flashes == 0 && Quiet.Thunder == 0 && Far.Flashes >= 12 && Far.Thunder > 0 && Storm.Flashes > 2 * Far.Flashes && Storm.Near * 10 >= Storm.Flashes * 9 && Storm.Thunder > Far.Thunder && Own.Flashes >= 12,
			FString::Printf(TEXT("ten minutes of night: quiet %d flashes, far off %d, a storm on the horizon %d (%d where it stands, %d thunder), no weather %d"), Quiet.Flashes, Far.Flashes, Storm.Flashes, Storm.Near, Storm.Thunder, Own.Flashes));
		S47Weather::FNow Lit; Override(EDev::Lightning, Lit); S47Weather::FNow Still; Override(EDev::Calm, Still);
		S47Weather::FLive LitShown; Shown(Lit, 0, 1, LitShown);
		Check(Lit.Lightning >= FS47Sky::StormRate && Still.Lightning < FS47Sky::QuietRate && LitShown.Lightning == Lit.Lightning && LitShown.LightningAz == Lit.LightningAz, TEXT("F10's lightning is a storm on the horizon, its calm a quiet night, and the world shows the weather's lightning"));
	}
	{
		// M5 sweep 2: the sky's veil (FS47Sky): brown and thick in the sand by day, grey under a shower, nothing on a plain day
		FLinearColor SandC, RainC, PlainC; double SandA = 0, RainA = 0, PlainA = 1, NightA = 0; FLinearColor NightC;
		FS47Sky::VeilOf(1, 0, 1, SandC, SandA); FS47Sky::VeilOf(0, 1, 1, RainC, RainA); FS47Sky::VeilOf(0, 0, 1, PlainC, PlainA); FS47Sky::VeilOf(1, 0, 0, NightC, NightA);
		Check(SandA > .4 && SandC.R > SandC.G && SandC.G > SandC.B && RainA > .5 && RainC.B >= RainC.R && RainC.R < .35f && PlainA == 0 && NightA > .1 && NightC.R < .1f,
			TEXT("the sky's veil: brown in the sand by day (dark at night), a dark grey under a shower, none on a plain day"));
	}
	{
		// M5 sweep 3: the shower's own cloud (FS47Sky::CellPixels): dark and solid high in the middle, darker at its base than its top,
		// virga hanging under it that thins downward, and nothing at the foot, the top row or the sides (no edge of a card shows)
		const int32 W = 128, H = 64;
		TArray<FColor> Px; FS47Sky::CellPixels(W, H, Px);
		const auto At = [&Px, W](double U, double V) { return Px[FMath::Clamp(int32(V * 63), 0, 63) * W + FMath::Clamp(int32(U * 127), 0, 127)]; };
		uint8 Edges = 0; double VirgaHigh = 0, VirgaLow = 0;
		for (int32 X = 0; X < W; ++X) Edges = FMath::Max3(Edges, Px[X].A, Px[(H - 1) * W + X].A);
		for (int32 Y = 0; Y < H; ++Y) Edges = FMath::Max3(Edges, Px[Y * W].A, Px[Y * W + W - 1].A);
		for (int32 X = W / 4; X < 3 * W / 4; ++X) { VirgaHigh += Px[int32(.66 * (H - 1)) * W + X].A; VirgaLow += Px[int32(.84 * (H - 1)) * W + X].A; }
		Check(Px.Num() == W * H && At(.5, .4).A > 150 && At(.5, .54).R < At(.5, .3).R && Edges < 8 && VirgaHigh > 2 * VirgaLow && VirgaHigh > 4 * (W / 2),
			TEXT("the shower's cloud: dark and solid in the middle, darkest at the base, virga thinning under it, no edge of the card shows"));
	}
	{
		// SHOWER-LOOK-20261109: the base is soft and torn, not a ruler's line (from solid to clear over several rows, at a height that
		// wanders across the cloud), each of the three cards is its own cloud with no edge of its own, and they are not one picture
		const int32 W = 192, H = 96;
		bool bNoEdges = true; int32 Rows = 0, Spread = 0, Same = 0;
		TArray<FColor> First;
		for (int32 Var = 0; Var < FS47Sky::CellCards; ++Var)
		{
			TArray<FColor> Px; FS47Sky::CellPixels(W, H, Px, Var);
			uint8 Edges = 0;
			for (int32 X = 0; X < W; ++X) Edges = FMath::Max3(Edges, Px[X].A, Px[(H - 1) * W + X].A);
			for (int32 Y = 0; Y < H; ++Y) Edges = FMath::Max3(Edges, Px[Y * W].A, Px[Y * W + W - 1].A);
			bNoEdges &= Edges < 8;
			if (Var == 0)
			{
				First = Px;
				int32 Low = H, High = 0;
				for (int32 X = W / 4; X < 3 * W / 4; X += 4)
				{
					// down from the middle of the cloud: the last solid row and the first thin one under it
					int32 Solid = -1, Thin = -1;
					for (int32 Y = int32(.4 * H); Y < H; ++Y) { if (Px[Y * W + X].A > 210) Solid = Y; else if (Solid >= 0 && Px[Y * W + X].A < 120) { Thin = Y; break; } }
					if (Solid < 0 || Thin < 0) continue;
					Rows += Thin - Solid; ++Spread; Low = FMath::Min(Low, Solid); High = FMath::Max(High, Solid);
				}
				Spread = Spread > 0 ? High - Low : 0;
				Rows = Spread > 0 ? Rows : 0;
			}
			else
				for (int32 I = 0; I < Px.Num(); I += 7) Same += Px[I] == First[I] && Px[I].A > 20 && Px[I].A < 235; // (the solid body and the clear sky are alike in any cloud; the shape is in between)
		}
		Check(bNoEdges && Rows > 0 && Spread >= 4 && Same < (W * H / 7) * 2 / 3,
			FString::Printf(TEXT("the shower's three cards: a soft, torn base (its height wanders %d rows of %d), no card's edge shows, each its own cloud (edges %s, rows %d, same %d of %d)"), Spread, H, bNoEdges ? TEXT("none") : TEXT("SEEN"), Rows, Same, (W * H / 7) * 2 / 3));
	}
	{
		// the shower walks past with the wind: upwind before, beside the land while it rains, downwind after, gone in the end
		FVector2D Before, During, After, Gone, Plain; double ShowB = 0, ShowD = 0, ShowA = 0, ShowG = 1, ShowP = 0;
		const double Dx = .6, Dz = -.8;
		FS47Sky::CellPlace(Dx, Dz, -.6, Before, ShowB); FS47Sky::CellPlace(Dx, Dz, .4, During, ShowD);
		FS47Sky::CellPlace(Dx, Dz, 1.2, After, ShowA); FS47Sky::CellPlace(Dx, Dz, 3.5, Gone, ShowG);
		FS47Sky::CellPlace(Dx, Dz, S47Weather::NoShower, Plain, ShowP);
		const FVector2D Wind(Dx, Dz);
		Check(FVector2D::DotProduct(Before, Wind) < -5000 && During.Size() < 4000 && During.Size() > 2000 && FVector2D::DotProduct(After, Wind) > 5000 && ShowB > .9 && ShowD == 1 && ShowA > .9
			&& ShowG < .05 && FMath::IsNearlyEqual(Plain.Size(), FS47Sky::CellDistance, 1.0) && FVector2D::DotProduct(Plain, Wind) < 0 && ShowP == 1,
			TEXT("the shower's cloud: upwind before it rains, about 3 km beside the land while it rains, downwind after, gone over the horizon three hours on"));
	}
	{
		// F10's shower runs the shower's own course on the clock, and the ground shows the slick dirt's own wet, drying over the hours after
		S47Weather::FNow Under, Just, TwoAfter, Over;
		Override(EDev::Shower, Under, .5); Override(EDev::Shower, Just, 1.2); Override(EDev::Shower, TwoAfter, 3.2); Override(EDev::Shower, Over, 4.2);
		S47Weather::FLive LU, LJ, LT, LO, Off;
		Shown(Under, 1, 1, LU); Shown(Just, 1, 1, LJ); Shown(TwoAfter, 1, 1, LT); Shown(Over, 1, 1, LO);
		Check(LU.Rain == 1 && GroundWet(LU) == 1 && LJ.Rain == 0 && GroundWet(LJ) == 1 && GroundWet(LT) > .15 && GroundWet(LT) < .6 && GroundWet(LO) == 0 && GroundWet(Off) == 0
			&& LT.ShowerH == 3.2 && Under.bThunder && !TwoAfter.bThunder,
			FString::Printf(TEXT("F10's shower: rain and wet under it, no rain and wet just after, the ground %.2f wet two hours after the rain, dry by four, none with no weather"), GroundWet(LT)));
		// and a real shower day the same, on its own clock
		for (int32 TrySeed = 1; TrySeed < 60; ++TrySeed) for (int32 Num = 3; Num < 20; ++Num)
		{
			const S47Weather::FDay D = S47Weather::Day(TrySeed, Num);
			if (!D.bShower) continue;
			const S47Weather::FNow A = S47Weather::Now(TrySeed, Num, (D.ShowerAt + 3.2) * 3600.0);
			Check(FMath::IsNearlyEqual(A.ShowerH, 3.2, 1e-6) && FMath::IsNearlyEqual(A.Wet, TwoAfter.Wet, 1e-9), TEXT("a shower day's wet two hours after the rain is F10's"));
			TrySeed = 60; break;
		}
	}
	return OK;
}

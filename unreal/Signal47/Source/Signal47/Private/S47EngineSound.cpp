#include "S47EngineSound.h"

#include "S47Audio.h"

namespace
{
	constexpr double MASTER = 0.33;
	/** The rpm each recording was made at (measured from their spectra, then by ear in the game). */
	constexpr double IDLE_RPM = 600, DRIVE_RPM = 1800;
	/** The recordings' level against the old engine's (they are quieter as files). */
	constexpr double REC = 3.7;
	/** The fixed "cab" low-pass after the engine's own (Hz). */
	constexpr double CAB_LP = 1100;
	double Smooth(double X, double A, double B) { const double T = FMath::Min(1.0, FMath::Max(0.0, (X - A) / (B - A))); return T * T * (3 - 2 * T); }
	// the loops' keys in US47Audio
	const TCHAR* IDLE_KEY = TEXT("truck:idle");
	const TCHAR* DRIVE_KEY = TEXT("truck:drive");
}

// ---------- FS47AudioParam: the Web Audio spec's automation ----------

void FS47AudioParam::Add(const FEvent& E)
{
	// placed after the events at the same time, before those after it
	int32 I = 0;
	while (I < Events.Num() && Events[I].Time <= E.Time) ++I;
	Events.Insert(E, I);
}

void FS47AudioParam::CancelScheduledValues(double T)
{
	Events.RemoveAll([T](const FEvent& E) { return E.Time >= T; });
}

double FS47AudioParam::Value(double T) const
{
	double V = BaseV, Vt = BaseT;
	for (int32 i = 0; i < Events.Num(); ++i)
	{
		const FEvent& E = Events[i];
		if (IsRamp(E))
		{
			// a ramp from the previous event's time and value to its own
			const double T0 = Vt, V0 = V;
			if (T < E.Time)
			{
				if (T <= T0) return V0;
				const double F = (T - T0) / (E.Time - T0);
				if (E.Kind == EKind::Linear) return V0 + (E.V - V0) * F;
				// (exponential: held where the two values have different signs or one is 0)
				return V0 * E.V > 0 ? V0 * FMath::Pow(E.V / V0, F) : V0;
			}
			V = E.V; Vt = E.Time;
			continue;
		}
		if (E.Time > T) return V;
		if (E.Kind == EKind::Set) { V = E.V; Vt = E.Time; continue; }
		// a target: an exponential approach from the value it begins at, until the next event; a
		// ramp after it starts where it began
		const FEvent* Nx = Events.IsValidIndex(i + 1) ? &Events[i + 1] : nullptr;
		if (Nx && IsRamp(*Nx)) { Vt = E.Time; continue; }
		if (!Nx || T < Nx->Time) return E.V + (V - E.V) * FMath::Exp(-(T - E.Time) / E.Tau);
		V = E.V + (V - E.V) * FMath::Exp(-(Nx->Time - E.Time) / E.Tau); Vt = Nx->Time;
	}
	return V;
}

void FS47AudioParam::Prune(double T)
{
	while (Events.Num() >= 2 && Events[1].Time <= T)
	{
		// the state the first event leaves for the second, as Value's loop has it
		const FEvent E = Events[0];
		if (E.Kind != EKind::Target) { BaseV = E.V; BaseT = E.Time; }
		else if (IsRamp(Events[1])) BaseT = E.Time;
		else { BaseV = E.V + (BaseV - E.V) * FMath::Exp(-(Events[1].Time - E.Time) / E.Tau); BaseT = Events[1].Time; }
		Events.RemoveAt(0);
	}
}

// ---------- the engine ----------

FS47EngineSound::FS47EngineSound(US47Audio* InAudio) : Audio(InAudio) {}

FS47EngineSound::~FS47EngineSound()
{
	if (Audio && bLoops) { Audio->Stop(IDLE_KEY, 0.1f); Audio->Stop(DRIVE_KEY, 0.1f); }
}

double FS47EngineSound::Master() const { return bHushed ? 0 : MASTER; }
double FS47EngineSound::Now() const { return FPlatformTime::Seconds(); }

void FS47EngineSound::EngineNodes()
{
	// the engine itself: the two recordings, each looped (the web version starts each somewhere in
	// itself; here they start at their heads)
	const double T = Now();
	Bus = FS47AudioParam(bEng ? Bus.Value(T) : 0.0);
	Lp = FS47AudioParam(300);
	Gi = FS47AudioParam(REC);
	Gd = FS47AudioParam(0);
	IdleRate = FS47AudioParam(1);
	DriveRate = FS47AudioParam(1);
	Band = FS47AudioParam(120);
	Noise = FS47AudioParam(0.03);
	if (Audio && !bLoops)
	{
		// (started just audible, so the mixer keeps them playing until their level comes up)
		FS47Sound S; S.Gain = 0.0001f; S.bLoop = true;
		Audio->LoopSfx(IDLE_KEY, TEXT("engine_idle"), S);
		Audio->LoopSfx(DRIVE_KEY, TEXT("engine_drive"), S);
		bLoops = true;
	}
	StopAt = -1;
	bEng = true;
}

void FS47EngineSound::Build()
{
	EngineNodes();
	// tyres and wind: less muffled than the engine (US47Audio::EngineTyres)
	bTyres = true;
}

double FS47EngineSound::Once(const TCHAR* Clip, double Gain)
{
	// a recording played once, through the cab
	if (!Audio) return 0;
	const double D = Audio->Duration(Clip);
	if (D <= 0) return 0;
	Audio->Sfx(Clip, [&] { FS47Sound S; S.Gain = float(Gain * Master()); return S; }());
	return D;
}

void FS47EngineSound::Start()
{
	const bool bFresh = !bEng;
	if (bFresh) Build();
	bRunning = true; bStalled = false;
	const double T = Now();
	Out.CancelScheduledValues(T);
	Out.SetTargetAtTime(Master(), T, 0.25);
	Set(650, 0);
	// the key: it turns over and catches, and the idle comes up under it
	if (bFresh && Once(TEXT("engine_start"), 1.1) > 0 && bEng)
	{
		Bus.CancelScheduledValues(T); Bus.SetValueAtTime(0, T); Bus.SetValueAtTime(0, T + 1.3); Bus.LinearRampToValueAtTime(Level, T + 2.6);
	}
}

void FS47EngineSound::Stop()
{
	if (!bEng) return;
	const double T = Now();
	// switched off, not dead already: the shut-off, then quiet
	const double Off = bRunning && !bStalled ? Once(TEXT("engine_off"), 1.0) : 0;
	bRunning = false;
	Bus.SetTargetAtTime(0, T, 0.06);
	Out.CancelScheduledValues(T);
	Out.SetTargetAtTime(0, T + Off, 0.15);
	StopAt = T + Off + 1;
	bEng = false; bTyres = false;
	if (Audio) Audio->EngineStop(float(Off + 1));
}

void FS47EngineSound::Set(double InRpm, double InLoad)
{
	if (!bEng || !bRunning || bStalled) return;
	const double T = Now();
	Rpm = InRpm; Load = InLoad;
	const double F = FMath::Max(InRpm, 300.0) / 60 * 4; // a V8 fires four times a turn
	const double L = FMath::Min(1.0, FMath::Max(0.0, InLoad));
	const double R = FMath::Max(InRpm, 300.0), X = Smooth(R, 900, 1500);
	IdleRate.SetTargetAtTime(FMath::Min(1.9, FMath::Max(0.6, R / IDLE_RPM)), T, 0.05);
	DriveRate.SetTargetAtTime(FMath::Min(2.1, FMath::Max(0.55, R / DRIVE_RPM)), T, 0.05);
	Gi.SetTargetAtTime(REC * FMath::Cos(X * UE_DOUBLE_PI / 2), T, 0.08);
	Gd.SetTargetAtTime(REC * FMath::Sin(X * UE_DOUBLE_PI / 2) * (0.85 + 0.3 * L), T, 0.08);
	Lp.SetTargetAtTime(700 + FMath::Min(1.0, R / 4000) * 900 + L * 1200, T, 0.08);
	Band.SetTargetAtTime(F * 2.5, T, 0.08);
	Noise.SetTargetAtTime(0.05 + 0.16 * L, T, 0.1);
	Level = 0.1 + 0.07 * L + 0.04 * FMath::Min(1.0, InRpm / 3500);
	Bus.SetTargetAtTime(Level, T, 0.1);
}

void FS47EngineSound::Stall()
{
	if (!bEng || bStalled) return;
	bStalled = true;
	const double T = Now(), L = Level;
	// the recordings wind down like a tape slowing
	for (FS47AudioParam* P : { &IdleRate, &DriveRate })
	{
		const double V = P->Value(T);
		P->CancelScheduledValues(T); P->SetValueAtTime(V, T); P->ExponentialRampToValueAtTime(0.25, T + 1.2);
	}
	Bus.CancelScheduledValues(T); Bus.SetValueAtTime(L, T);
	const double Chugs[6][2] = { { 0.08, 1.25 }, { 0.3, 0.45 }, { 0.42, 0.85 }, { 0.72, 0.25 }, { 0.84, 0.45 }, { 1.3, 0 } };
	for (const auto& C : Chugs) Bus.LinearRampToValueAtTime(L * C[1], T + C[0]);
	Noise.SetTargetAtTime(0, T + 0.2, 0.2);
}

void FS47EngineSound::Hush(bool bOn)
{
	if (bOn == bHushed) return;
	bHushed = bOn;
	const double T = Now();
	Out.CancelScheduledValues(T);
	Out.SetTargetAtTime(bOn ? 0 : bEng ? MASTER : 0, T, bOn ? 0.012 : 0.3);
}

void FS47EngineSound::Still()
{
	if (!bEng) return;
	bStalled = true;
	const double T = Now();
	Bus.CancelScheduledValues(T); Bus.SetValueAtTime(0, T);
	Noise.CancelScheduledValues(T); Noise.SetValueAtTime(0, T);
	if (Audio && bTyres) Audio->EngineTyres(TEXT(""), 0.f);
}

void FS47EngineSound::Cough(double K)
{
	if (!bEng || bStalled) return;
	const double T = Now(), L = Level;
	Bus.CancelScheduledValues(T); Bus.SetValueAtTime(L, T);
	Bus.LinearRampToValueAtTime(L * (1 - 0.75 * K), T + 0.025);
	Bus.LinearRampToValueAtTime(L * 1.15, T + 0.11);
	Bus.LinearRampToValueAtTime(L, T + 0.2);
	// the dull pop in the exhaust (band-passed brown noise)
	if (Audio) Audio->EngineCough(float(K));
}

void FS47EngineSound::Crank(double Secs)
{
	if (!bEng) Build();
	const double T = Now();
	Out.CancelScheduledValues(T);
	Out.SetTargetAtTime(Master(), T, 0.05);
	// a whirring motor that labours on each compression, about four and a half a second
	if (Audio) Audio->EngineCrank(float(Secs));
}

void FS47EngineSound::Solenoid(const TArray<double>& Secs, double K)
{
	const double T = Now();
	Out.CancelScheduledValues(T);
	Out.SetTargetAtTime(Master(), T, 0.02);
	TArray<float> S;
	for (double V : Secs) S.Add(float(V));
	if (Audio) Audio->EngineSolenoid(S, float(K));
}

void FS47EngineSound::Restart()
{
	if (!bEng) { Start(); return; }
	bStalled = false; bRunning = true;
	const double T = Now();
	IdleRate.CancelScheduledValues(T); DriveRate.CancelScheduledValues(T);
	Bus.CancelScheduledValues(T);
	Bus.SetValueAtTime(0, T);
	Bus.LinearRampToValueAtTime(Level * 1.6, T + 0.15);
	Bus.SetTargetAtTime(Level, T + 0.3, 0.3);
	Out.CancelScheduledValues(T);
	Out.SetTargetAtTime(Master(), T, 0.05);
	Set(1400, 0.4);
}

void FS47EngineSound::Tyres(ES47SurfaceKind Surface, double Speed)
{
	if (!bTyres || !bRunning) return;
	if (Audio) Audio->EngineTyres(S47SurfaceName(Surface), float(FMath::Abs(Speed)));
}

void FS47EngineSound::Rattle(double Speed) { if (Audio) Audio->EngineRattle(float(Speed)); }
void FS47EngineSound::Thump(double Speed) { if (Audio) Audio->EngineThump(float(Speed)); }

void FS47EngineSound::Tick()
{
	const double T = Now();
	for (FS47AudioParam* P : { &Out, &Bus, &Gi, &Gd, &IdleRate, &DriveRate, &Lp, &Band, &Noise }) P->Prune(T);
	if (!bLoops || !Audio) return;
	if (StopAt >= 0 && T >= StopAt)
	{
		Audio->Stop(IDLE_KEY, 0.05f);
		Audio->Stop(DRIVE_KEY, 0.05f);
		bLoops = false; StopAt = -1;
		return;
	}
	const double O = Out.Value(T), B = Bus.Value(T);
	// the engine's low-pass and the cab's after it, as one filter (the lower of the two)
	const float Cut = float(FMath::Min(Lp.Value(T), CAB_LP));
	Audio->LoopLevel(IDLE_KEY, float(O * B * Gi.Value(T)), float(IdleRate.Value(T)), Cut);
	Audio->LoopLevel(DRIVE_KEY, float(O * B * Gd.Value(T)), float(DriveRate.Value(T)), Cut);
	// the band-passed rumble under the engine, at the bus's level
	Audio->EngineBus(float(O * B), float(Noise.Value(T)), float(Band.Value(T)));
}

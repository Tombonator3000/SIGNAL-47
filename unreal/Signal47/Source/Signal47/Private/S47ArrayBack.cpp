// story/arrayBack.ts
#include "S47ArrayBack.h"

#include "S47Audio.h"
#include "S47DishArray.h"
#include "S47Docs.h"
#include "S47WebAudio.h"

using namespace S47ArrayBack;

namespace
{
	const double ARM = 0.7;                                       // from the command to the first contactor
	const double STAGGER = 1.2;                                   // the contactors out across the field
	struct { double Az, El, AccAz, AccEl; } const RATE = { 2.2, 1.32, 1.0, 0.7 };   // 02:15's pace (Prologue.turnDishes, slew 2.2)
	const double SYNC[2] = { 6, 34 };                             // the lamps slide into one blink, seconds into the hold
	const double HUSH = 37;                                       // and the crickets stop
	// the four and the seven: dark first, the flashes, a gap, dark again before they turn
	const double DARK0 = 1.6, SLOT = 0.58, ON = 0.3, GAP = 0.9, TAIL = 1.0;
	const double SEVEN = DARK0 + 4 * SLOT + GAP;
	const double SETTLE = 2.6;                                    // home: the crickets back, then it is over
	// a flash: brighter, and flared to a halo wider than the dish, so that from behind it (from SARO's
	// side, where the reflectors hide the lamps on their feeds) the rim stands out red against it
	const double FLASH = 1.9, FLARE = 1.5, STEADY = 1.0;
	const TCHAR* WORDS[] = { TEXT("no"), TEXT("one"), TEXT("two"), TEXT("three"), TEXT("four"), TEXT("five"), TEXT("six"), TEXT("seven"), TEXT("eight"), TEXT("nine"), TEXT("ten"), TEXT("eleven"), TEXT("twelve") };
	const TCHAR* ORD[] = { TEXT(""), TEXT("first"), TEXT("second"), TEXT("third"), TEXT("fourth"), TEXT("fifth"), TEXT("sixth"), TEXT("seventh"), TEXT("eighth"), TEXT("ninth"), TEXT("tenth") };
	FString Ordinal(int32 N) { return N >= 0 && N < 11 ? FString(ORD[N]) : FString::Printf(TEXT("%dth"), N); }

	double Wrap180(double A) { return FMath::Fmod(A + 540, 360.0) - 180; }
	double WrapPi(double A) { return FMath::Atan2(FMath::Sin(A), FMath::Cos(A)); }
	double Rnd(double A, double B) { return A + FMath::FRand() * (B - A); }
	double JsRound(double V) { return FMath::FloorToDouble(V + 0.5); }
	/** One axis of a drive: up to vmax, gathering speed at acc, braking harder, stopping on the target. */
	double Axis(double V, double Err, double VMax, double Acc, double Dt)
	{
		const double Want = FMath::Sign(Err) * FMath::Min(VMax, FMath::Sqrt(2 * Acc * FMath::Abs(Err)) * 0.92);
		const bool bFaster = FMath::Abs(Want) > FMath::Abs(V) && Want * V >= 0;
		const double Lim = (bFaster ? 1 : 1.8) * Acc * Dt;
		return V + FMath::Clamp(Want - V, -Lim, Lim);
	}
	FString Clk(double C, bool bSecs = true) { return S47Text::ClockText(C, bSecs); }
}

double S47ArrayBack::SIGNAL() { return SEVEN + 7 * SLOT + TAIL; }

FString S47ArrayBack::Times(int32 N)
{
	if (N == 1) return TEXT("once");
	if (N == 2) return TEXT("twice");
	return FString::Printf(TEXT("%s times"), N >= 0 && N < 13 ? WORDS[N] : *FString::FromInt(N));
}

S47ArrayBack::FLamps S47ArrayBack::LampsAt(double Ts)
{
	if (Ts < DARK0) return { false, -1 };
	const double A = (Ts - DARK0) / SLOT;
	if (A < 4) return { FMath::Fmod(Ts - DARK0, SLOT) < ON, int32(FMath::FloorToDouble(A)) };
	if (Ts < SEVEN) return { false, 3 };
	const double B = (Ts - SEVEN) / SLOT;
	if (B < 7) return { FMath::Fmod(Ts - SEVEN, SLOT) < ON, 4 + int32(FMath::FloorToDouble(B)) };
	return { false, 10 };
}

// ---------- the case's JSON ----------

S47J::FObj FS47ArrayTry::ToJson() const
{
	const S47J::FObj O = S47J::New();
	S47J::Set(O, TEXT("cmd"), Cmd);
	if (On) S47J::Set(O, TEXT("on"), *On);
	if (Lost) S47J::Set(O, TEXT("lost"), *Lost);
	if (Turn) S47J::Set(O, TEXT("turn"), *Turn);
	if (Home) S47J::Set(O, TEXT("home"), *Home);
	return O;
}

TOptional<FS47ArrayTry> FS47ArrayTry::Copy(const TSharedPtr<FJsonValue>& V)
{
	const TSharedPtr<FJsonObject>* O = nullptr;
	if (!V.IsValid() || !V->TryGetObject(O) || !O || !O->IsValid()) return {};
	double Cmd = 0;
	if (!(*O)->TryGetNumberField(TEXT("cmd"), Cmd) || !FMath::IsFinite(Cmd)) return {};
	FS47ArrayTry Out; Out.Cmd = Cmd;
	auto Opt = [&](const TCHAR* K, TOptional<double>& To) { double X = 0; if ((*O)->TryGetNumberField(K, X) && FMath::IsFinite(X)) To = X; };
	Opt(TEXT("on"), Out.On); Opt(TEXT("lost"), Out.Lost); Opt(TEXT("turn"), Out.Turn); Opt(TEXT("home"), Out.Home);
	return Out;
}

S47J::FObj FS47ArraySave::ToJson() const
{
	const S47J::FObj O = S47J::New();
	TArray<TSharedPtr<FJsonValue>> A;
	for (const FS47ArrayTry& R : Runs) A.Add(MakeShared<FJsonValueObject>(R.ToJson()));
	O->SetArrayField(TEXT("runs"), A);
	if (Run) O->SetObjectField(TEXT("run"), Run->ToJson());
	return O;
}

TOptional<FS47ArraySave> FS47ArraySave::Copy(const S47J::FObj& O)
{
	const TArray<TSharedPtr<FJsonValue>>* Runs = nullptr;
	if (!O.IsValid() || !O->TryGetArrayField(TEXT("runs"), Runs) || !Runs) return {};
	FS47ArraySave Out;
	for (const TSharedPtr<FJsonValue>& V : *Runs) if (const TOptional<FS47ArrayTry> T = FS47ArrayTry::Copy(V)) Out.Runs.Add(*T);
	Out.Run = FS47ArrayTry::Copy(O->TryGetField(TEXT("run")));
	return Out;
}

// ---------- ArrayBack ----------

FS47ArrayBack::FS47ArrayBack(FS47ArrayDeps InD) : D(MoveTemp(InD)) {}

FS47ArraySave FS47ArrayBack::Kept() const
{
	const TOptional<FS47ArraySave> V = D.Get ? D.Get() : TOptional<FS47ArraySave>();
	return V ? *V : FS47ArraySave();
}

int32 FS47ArrayBack::Count() const { const TOptional<FS47ArraySave> V = D.Get ? D.Get() : TOptional<FS47ArraySave>(); return V ? V->Runs.Num() : 0; }
TArray<FS47ArrayTry> FS47ArrayBack::Runs() const { return Kept().Runs; }

const FS47Dish* FS47ArrayBack::Dish(int32 I) const { return D.Dishes && D.Dishes->Dishes.IsValidIndex(I) ? &D.Dishes->Dishes[I] : nullptr; }
void FS47ArrayBack::HoldBeacons(TOptional<double> Level, double Flare) const { if (D.Dishes) D.Dishes->HoldBeacons(Level, Flare); }
void FS47ArrayBack::BeaconPhase(int32 I, double P) const { if (D.Dishes) D.Dishes->BeaconPhase(I, P); }

TOptional<FS47ArrayStatus> FS47ArrayBack::Status() const
{
	if (Stage == TEXT("armed") || Stage == TEXT("slew")) return FS47ArrayStatus{ TEXT("SLEWING TO SCHEDULE"), TEXT("S-01 TO S-27  CMD  AZ 042 EL 48"), false };
	if (Stage == TEXT("hold")) return FS47ArrayStatus{ TEXT("ON SCHEDULE"), FString::Printf(TEXT("ON SCHEDULE  00:00:%s"), *S47Text::PadStart(FString::FromInt(int32(FMath::Min(HOLD, FMath::FloorToDouble(T)))), 2, TEXT('0'))), false };
	if (Stage == TEXT("signal")) return FS47ArrayStatus{ TEXT("ON SCHEDULE"), FString::Printf(TEXT("ON SCHEDULE  00:00:%d"), int32(HOLD)), true };
	if (Stage == TEXT("back")) return FS47ArrayStatus{ TEXT("NO SCHEDULE ENTRY"), TEXT("MOTION  NO CONTROL COMMAND"), true };
	if (Stage == TEXT("settle")) return FS47ArrayStatus{ TEXT("NO SCHEDULE ENTRY"), TEXT("AZ 026 EL 32  NO CONTROL COMMAND"), true };
	return {};
}

FString FS47ArrayBack::StatusText() const
{
	if (Stage == TEXT("armed") || Stage == TEXT("slew")) return TEXT("S-01 to S-27: slewing to the schedule. Az 042, el 48.");
	if (Stage == TEXT("hold") || Stage == TEXT("signal")) return FString::Printf(TEXT("S-01 to S-27 on the schedule. Az 042, el 48, for %d seconds."), int32(FMath::Min(HOLD, FMath::FloorToDouble(T))));
	if (Stage == TEXT("back") || Stage == TEXT("settle")) return TEXT("S-01 to S-27: moving. No control command. No drive current.");
	return FString();
}

// ---------- the console (story/nightshift.ts) ----------

TOptional<FString> FS47ArrayBack::Command(const FString& Input, const FS47ArrayGate& Gate)
{
	// input.trim().toUpperCase().replace(/\s+/g, ' ')
	FString C;
	bool bSpace = false;
	for (const TCHAR Ch : Input.TrimStartAndEnd().ToUpper())
	{
		if (FChar::IsWhitespace(Ch)) { bSpace = true; continue; }
		if (bSpace && !C.IsEmpty()) C.AppendChar(' ');
		bSpace = false;
		C.AppendChar(Ch);
	}
	// /^(ARRAY RESUME( SCHEDULE| ALL)?|ARRAY RESUME\/SCHEDULE|RESUME ARRAY|RESUME SCHEDULE)$/
	static const TArray<FString> OK = { TEXT("ARRAY RESUME"), TEXT("ARRAY RESUME SCHEDULE"), TEXT("ARRAY RESUME ALL"), TEXT("ARRAY RESUME/SCHEDULE"), TEXT("RESUME ARRAY"), TEXT("RESUME SCHEDULE") };
	if (!OK.Contains(C)) return {};
	if (Busy()) return FString(TEXT("%ARRAY-W-INPROG, schedule resume in progress"));
	if (Gate == TEXT("early")) return FString(TEXT("ARRAY ON SURVEY TRACK.\nNOTHING TO RESUME."));
	if (Gate == TEXT("release")) return FString(TEXT("%ARRAY-E-HELD, array held, no schedule entry\n-ARRAY-I-SUPV, release by supervisor only"));
	if (Gate == TEXT("moving")) return FString(TEXT("%ARRAY-E-DRVMOTION, drives in motion\n-ARRAY-I-NOCMD, no command"));
	// (tipped down on 026 by the archive's phone, story/archiveDishes.ts: they do not answer)
	if (Gate == TEXT("tipped")) return FString(TEXT("%ARRAY-E-DRVNORESP, drives not responding\n-ARRAY-I-POS, S-01 TO S-27  AZ 026  EL 02\n-ARRAY-I-NOCMD, no command"));
	if (Gate == TEXT("busy")) return FString(TEXT("%ARRAY-W-LINKBUSY, array link busy\n-ARRAY-I-RETRY, retry"));
	const TArray<FS47ArrayTry> Was = Kept().Runs;
	const TOptional<FS47ArrayTry> Last = Was.Num() ? TOptional<FS47ArrayTry>(Was.Last()) : TOptional<FS47ArrayTry>();
	Begin();
	const FString Head = Last && Last->Lost ? FString::Printf(TEXT("%%ARRAY-W-LEFTSCHED, left schedule %s\n-ARRAY-I-NOCMD, no command\n"), *Clk(*Last->Lost)) : FString();
	return Head + TEXT("ARRAY RESUME\nSCHEDULE  RUN860413_2341\nPOINTING  AZ 042  EL 48\nDRIVES    S-01 TO S-27  ENABLED");
}

TOptional<FS47ArrayBack::FHistory> FS47ArrayBack::History() const
{
	const TArray<FS47ArrayTry> R = Kept().Runs;
	if (!R.Num()) return {};
	TArray<FString> Rows;
	for (int32 I = FMath::Max(0, R.Num() - 4); I < R.Num(); ++I)
	{
		const FS47ArrayTry& Tr = R[I];
		Rows.Add(FString::Printf(TEXT("%s  CMD RESUME   AZ 042 EL 48"), *Clk(Tr.Cmd)));
		if (Tr.On) Rows.Add(FString::Printf(TEXT("%s  ON SCHEDULE  AZ 042 EL 48"), *Clk(*Tr.On)));
		const TOptional<double> Back = Tr.Home ? Tr.Home : Tr.Turn ? Tr.Turn : Tr.Lost;
		if (Back) Rows.Add(FString::Printf(TEXT("%s  NO COMMAND   AZ 026 EL 32"), *Clk(*Back)));
	}
	return FHistory{ R.Num(), FString::Printf(TEXT("SINCE %s:\n%s"), *Clk(R[0].Cmd, false), *FString::Join(Rows, TEXT("\n"))) };
}

// ---------- a try ----------

void FS47ArrayBack::Begin()
{
	Stage = TEXT("armed"); T = 0; bCut = false; bHushed = false; bMotors = false;
	Got = FGot(); Flashes = 0; bLit = false;
	Said = FSaid();
	const double C = JsRound(Clock());
	Run = FS47ArrayTry(); Run.Cmd = C;
	Log(TEXT("ARRAY RESUME  RUN860413_2341  OPERATOR REYES"), C);
	// out along the cable trench from the control room: the near rows first, the far ones last
	Rigs.Reset();
	const int32 N = D.Dishes ? D.Dishes->Dishes.Num() : 0;
	double Far = 0;
	for (int32 I = 0; I < N; ++I) Far = FMath::Max(Far, FMath::Sqrt(FMath::Square(Dish(I)->X) + FMath::Square(Dish(I)->Z)));
	for (int32 I = 0; I < N; ++I)
	{
		FRig R; R.I = I; R.Az = D.Dishes->CurAz(I); R.El = D.Dishes->CurEl(I);
		R.At = ARM + STAGGER * FMath::Sqrt(FMath::Square(Dish(I)->X) + FMath::Square(Dish(I)->Z)) / FMath::Max(1e-9, Far) + Rnd(0, 0.12);
		Rigs.Add(R);
	}
	Lamps();
	Keep();
}

void FS47ArrayBack::Lamps()
{
	// the lamps' places, the first time they are needed
	const int32 N = D.Dishes ? D.Dishes->Dishes.Num() : 0;
	if (Ph.Num() == N) return;
	Ph.Reset();
	for (int32 I = 0; I < N; ++I) Ph.Add(Dish(I)->Phase);
}

void FS47ArrayBack::Keep()
{
	FS47ArraySave V = Kept();
	V.Run = Run;
	if (D.Set) D.Set(V);
}

void FS47ArrayBack::Update(double Dt, const FS47ArrayIn& S)
{
	if (Stage == TEXT("idle")) { Drift(Dt, S); return; }
	// away from SARO: nobody there to see it go back
	if (!S.bSaro) { Away(); return; }
	// the warning of the time away: the array leaves the schedule for it
	if (S.bWarning && !bCut && Stage != TEXT("settle")) Leave();
	T += Dt;
	if (Stage == TEXT("armed")) { if (T >= ARM) { Stage = TEXT("slew"); T = ARM; Loud(S.Eye); } }
	else if (Stage == TEXT("slew")) Slew(Dt, S);
	else if (Stage == TEXT("hold")) Hold(Dt, S);
	else if (Stage == TEXT("signal")) Signal(S);
	else if (Stage == TEXT("back")) Back(Dt, S);
	else if (Stage == TEXT("settle")) { if (T >= SETTLE) End(true); }
}

void FS47ArrayBack::Loud(const FVector& Eye)
{
	// the few heard close by: the nearest to the player now
	TArray<FRig*> By;
	for (FRig& R : Rigs) By.Add(&R);
	auto H = [this, &Eye](const FRig* R) { return FMath::Sqrt(FMath::Square(Dish(R->I)->X - Eye.X) + FMath::Square(Dish(R->I)->Z - Eye.Z)); };
	By.StableSort([&](const FRig& A, const FRig& B) { return H(&A) < H(&B); });
	for (int32 K = 0; K < By.Num() && K < 5; ++K) if (H(By[K]) < 160) By[K]->bLoud = true;
}

void FS47ArrayBack::Slew(double Dt, const FS47ArrayIn& S)
{
	US47Audio* A = D.Audio.Get();
	bool bAll = true;
	for (FRig& R : Rigs)
	{
		if (!R.bMine) continue;
		if (T < R.At) { bAll = false; continue; }
		if (!R.bOn)
		{
			R.bOn = true;
			if (!bMotors)
			{
				bMotors = true;
				if (A) A->StartMotors();
				if (Count() == 0)
				{
					if (S.bOutside) Toast(TEXT("Across the field the drives start up, one after another."), 3.2);
					else if (S.bRoom) Toast(TEXT("Through the windows the dishes are turning, all of them, to the schedule."), 3.4);
				}
			}
			if (R.bLoud) Clunk(R.I, true, S.bCalm);
		}
		if (!Drive(R, SCHED_AZ, SCHED_EL, Dt)) bAll = false;
	}
	if (bAll) OnSchedule(S);
}

bool FS47ArrayBack::Drive(FRig& R, double ToAz, double ToEl, double Dt)
{
	// turn a dish towards a pointing at the drives' pace; true when it is on it, stopped
	const double EAz = Wrap180(ToAz - R.Az), EEl = ToEl - R.El;
	R.VAz = Axis(R.VAz, EAz, RATE.Az, RATE.AccAz, Dt);
	R.VEl = Axis(R.VEl, EEl, RATE.El, RATE.AccEl, Dt);
	if (FMath::Abs(EAz) <= FMath::Abs(R.VAz) * Dt + 0.02 && FMath::Abs(R.VAz) < 1.2) { R.Az = ToAz; R.VAz = 0; } else R.Az += R.VAz * Dt;
	if (FMath::Abs(EEl) <= FMath::Abs(R.VEl) * Dt + 0.02 && FMath::Abs(R.VEl) < 1.2) { R.El = ToEl; R.VEl = 0; } else R.El += R.VEl * Dt;
	if (D.Dishes)
	{
		D.Dishes->Snap(R.I, R.Az, R.El);
		D.Dishes->Dishes[R.I].Spin = FMath::Abs(R.VAz) + FMath::Abs(R.VEl);
	}
	return R.Az == ToAz && R.El == ToEl && !R.VAz && !R.VEl;
}

void FS47ArrayBack::OnSchedule(const FS47ArrayIn& S)
{
	US47Audio* A = D.Audio.Get();
	Stage = TEXT("hold"); T = 0;
	Run.On = JsRound(Clock());
	Log(TEXT("ARRAY  S-01 TO S-27  ON SCHEDULE  AZ 042 EL 48"), *Run.On);
	if (bMotors) { bMotors = false; if (A) A->StopMotors(); }
	for (FRig& R : Rigs) { if (D.Dishes) D.Dishes->Dishes[R.I].Spin = 0; if (R.bLoud && R.bMine) Clunk(R.I, false, S.bCalm, Rnd(0, 0.5)); }
	// the one they slide to: the nearest lamp's
	Lamps();
	const FRig* Near = Rigs.FindByPredicate([](const FRig& R) { return R.bLoud; });
	if (!Near && Rigs.Num()) Near = &Rigs[0];
	if (Near && Ph.IsValidIndex(Near->I)) Common = Ph[Near->I];
	if (Count() == 0) Toast(TEXT("All twenty-seven on the schedule. Az 042, el 48."), 3.4);
	Keep();
}

void FS47ArrayBack::Hold(double Dt, const FS47ArrayIn& S)
{
	// the red lamps on the feeds, each its own way, slide into one blink across the field
	if (T > SYNC[0])
	{
		const double K = UE_DOUBLE_PI / (SYNC[1] - SYNC[0]);
		for (const FRig& R : Rigs)
		{
			if (!R.bMine) continue;
			Ph[R.I] += FMath::Clamp(WrapPi(Common - Ph[R.I]), -K * Dt, K * Dt);
			BeaconPhase(R.I, Ph[R.I]);
		}
	}
	if (!Said.bSync && T > 20 && T < HOLD - 6 && Sees(S))
	{
		Said.bSync = true;
		if (Count() < 2) Toast(TEXT("The red lamps on the feeds are blinking together now. All of them."), 3.6);
	}
	if (!bHushed && T >= HUSH) { bHushed = true; if (D.Hush) D.Hush(180, 0.02, 1.4, 2.5); }
	if (T >= HOLD)
	{
		Stage = TEXT("signal"); T = 0; bLit = false;
		HoldBeacons(S.bCalm ? 0.22 : 0);
		// and the floods out among them, with every lamp on the posts
		Field(S.bCalm ? 0.35 : 0);
		// (the console has the lamps as it has the signal: the binder's printouts say the same)
		// (stamped 47 s after ON SCHEDULE, which is when it is: the frame may be a little after)
		Run.Lost = (Run.On ? *Run.On : JsRound(Clock() - HOLD)) + HOLD;
		Log(TEXT("ARRAY  BEACONS S-01 TO S-27  PULSE GROUP 4 / 7"), *Run.Lost);
		Keep();
	}
}

void FS47ArrayBack::Signal(const FS47ArrayIn& S)
{
	US47Audio* A = D.Audio.Get();
	const FLamps L = LampsAt(T);
	const double Dark = S.bCalm ? 0.22 : 0;
	if (L.bOn != bLit)
	{
		bLit = L.bOn;
		HoldBeacons(L.bOn ? (S.bCalm ? 1.3 : FLASH) : Dark, L.bOn ? (S.bCalm ? 0.6 : FLARE) : 0);
		// the near dishes lit red by their own lamps (not with calm flashes: the floods stay low)
		if (!S.bCalm) Field(L.bOn ? 0.42 : 0, L.bOn);
		if (L.bOn)
		{
			Flashes++;
			// the status console in the control room, with them; out of doors, with each flash, the
			// deep thump the signal made in the receiver at 02:14 (core/signalVoice.ts), low and far
			if (S.bRoom) { if (A) A->Beep(Flashes <= 4 ? 880.f : 988.f, 0.16f, 0.05f, 0.f, TEXT("square")); Got.bBeeps = true; }
			else if (S.bOutside) Thump(S.bCalm ? 0.12 : 0.26);
		}
	}
	if (Sees(S)) Got.bLamps = true;
	if (T >= SIGNAL()) Turn(S);
}

void FS47ArrayBack::Turn(const FS47ArrayIn& S)
{
	// after the seventh: back to 026, together and without a sound
	Stage = TEXT("back"); T = 0; bLit = false;
	const double C = JsRound(Clock());
	Run.Turn = C;
	Log(TEXT("ARRAY  S-01 TO S-27  MOTION  NO COMMAND"), C);
	Log(TEXT("ARRAY  DRIVE CURRENT 0.0 A"), C);
	HoldBeacons(STEADY);
	Field({});
	for (FRig& R : Rigs) if (R.bMine) { if (D.Dishes) D.Dishes->Dishes[R.I].bBack = true; R.VAz = R.VEl = 0; }
	const bool bAgain = Count() > 0;
	if (Got.bLamps) Toast(bAgain ? TEXT("Four. Then seven.") : TEXT("The lights in the field went out. Every red lamp in the array, all at once: four flashes. Then seven."), 4);
	else if (S.bRoom) Toast(TEXT("The status console beeps. Four. Then seven."), 3.2);
	else if (S.bOutside) Toast(TEXT("Out in the array the red lamps are flashing, all of them. Four. Then seven."), 3.4);
	Keep();
}

void FS47ArrayBack::Back(double Dt, const FS47ArrayIn& S)
{
	bool bAll = true;
	for (FRig& R : Rigs)
	{
		if (!R.bMine) continue;
		// (the warning's dishes have it now: it is theirs from where it is)
		if (S.Taken && S.Taken->Contains(Dish(R.I)->Id)) { R.bMine = false; if (D.Dishes) { D.Dishes->Dishes[R.I].bBack = false; D.Dishes->Dishes[R.I].Spin = 0; } continue; }
		if (!Drive(R, HOME_AZ, HOME_EL, Dt)) bAll = false;
	}
	if (T > 0.6 && Sees(S)) Got.bBack = true;
	if (!Said.bTurn && T > 1.6)
	{
		Said.bTurn = true;
		if (bCut) { /* the warning has the night: nothing said */ }
		// (watched from the control room's windows as well: the console's hum went with them out to
		// the schedule, and there is none now)
		// (from the truck nothing is heard of them over the engine either way)
		else if (Got.bBack) Toast(Count() ? (S.bCab ? FString(TEXT("Back to 026.")) : FString(TEXT("Back to 026. Not a sound."))) : FString::Printf(TEXT("They are turning back. All twenty-seven at once%s."), S.bCab ? TEXT("") : TEXT(", and not a sound")), 3.6);
		else if (S.bRoom) Toast(Count() ? TEXT("On the status screen they are moving back. No command.") : TEXT("On the status screen all twenty-seven are moving back. No command. No drive current."), 3.6);
	}
	if (bAll) Home(S);
}

void FS47ArrayBack::Home(const FS47ArrayIn& S)
{
	US47Audio* A = D.Audio.Get();
	TArray<FRig*> Mine;
	for (FRig& R : Rigs) if (R.bMine) Mine.Add(&R);
	for (FRig* R : Mine) { if (D.Dishes) { D.Dishes->Snap(R->I, HOME_AZ, HOME_EL); D.Dishes->Dishes[R->I].bBack = false; D.Dishes->Dishes[R->I].Spin = 0; } }
	Run.Home = JsRound(Clock());
	Log(FString::Printf(TEXT("ARRAY  %d DRIVES  AZ 026 EL 32  NO COMMAND"), Mine.Num()), *Run.Home);
	if (!bCut)
	{
		HoldBeacons({});
		for (FRig* R : Mine) { Ph[R->I] = Common; BeaconPhase(R->I, Common); }
		// every brake at once, out among them; in the control room the console's chime of 02:15
		FRig* const* Near = Mine.FindByPredicate([](const FRig* R) { return R->bLoud; });
		if (Near && S.bOutside && A)
		{
			const FS47Dish* Dn = Dish((*Near)->I);
			const TSharedPtr<FS47AudioNode> Sp = A->Spatial(FVector(Dn->X, 6 * Dn->Scale, Dn->Z), 9);
			if (Sp.IsValid())
			{
				const double Brakes[3][3] = { { 0.42, 0.5, 0 }, { 0.47, 0.38, 0.012 }, { 0.38, 0.32, 0.03 } };
				for (const auto& B : Brakes)
				{
					FS47Sound O; O.DestNode = Sp; O.Gain = float(B[1] * (S.bCalm ? 0.6 : 1)); O.Rate = float(B[0]); O.Lp = 900; O.When = float(B[2]);
					A->Sfx(A->Or(TEXT("door_heavy"), TEXT("doorMetal")), O);
				}
			}
		}
		if (S.bRoom && A) { A->Beep(1760, 0.18f, 0.06f); A->Beep(1760, 0.18f, 0.06f, 0.3f); A->Beep(2350, 0.4f, 0.06f, 0.6f); }
		if (Got.bBack) Toast(TEXT("Az 026, el 32. All twenty-seven. Nobody sent a command."), 3.6);
		else if (S.bOutside) Toast(TEXT("Out in the array, every brake at once."), 3);
		else if (S.bRoom) Toast(TEXT("The console chimes, as it did at 02:15. Az 026, el 32, all twenty-seven."), 3.6);
	}
	Stage = TEXT("settle"); T = 0;
	Keep();
}

void FS47ArrayBack::End(bool bSeen)
{
	// it is over: kept, written down, and S-03's log has it
	US47Audio* A = D.Audio.Get();
	if (bHushed && !bCut && D.Hush) D.Hush(0, 1, 0.4, 2.5);
	bHushed = false;
	if (bMotors) { bMotors = false; if (A) A->StopMotors(); }
	FS47ArraySave V = Kept();
	const FS47ArrayTry R = Run;
	const int32 N = V.Runs.Num() + 1;
	FS47ArraySave Out; Out.Runs = V.Runs; Out.Runs.Add(R);
	if (D.Set) D.Set(Out);
	const FString At = Clk(R.Cmd, false);
	if (bCut) Note(FString::Printf(TEXT("%s. ARRAY RESUME. The array went to the schedule, and when the warning came it left it by itself. 026 again, el 32. Nobody sent a command."), *At));
	else if (!bSeen || !(Got.bLamps || Got.bBeeps || Got.bBack)) Note(FString::Printf(TEXT("%s. ARRAY RESUME. The console log has the array on the schedule for 47 seconds, then back on 026 by itself. Nobody sent a command."), *At));
	else if (N == 1)
	{
		const TCHAR* Saw = Got.bLamps ? TEXT("every red lamp in the array blinked four times, then seven, all together") : Got.bBeeps ? TEXT("the status console beeped four times, then seven") : TEXT("the lamps out in the array blinked four, then seven");
		Note(FString::Printf(TEXT("%s. ARRAY RESUME. All twenty-seven went to the schedule, az 042, el 48, and stayed there 47 seconds. Then %s, and they turned back to 026 by themselves, all at once. No motors. No drive current. Nobody sent a command."), *At, Saw));
	}
	else if (N == 2) Note(FString::Printf(TEXT("%s. ARRAY RESUME again. 042 for 47 seconds, four and seven on the lamps, and back to 026 by themselves. Twice now."), *At));
	else Note(FString::Printf(TEXT("%s. The %s time. The same: 47 seconds on the schedule, four, seven, 026."), *At, *Ordinal(N)));
	Rigs.Reset(); Stage = TEXT("idle"); T = 0; bCut = false;
	bDrifting = true;
	if (D.S03) D.S03();
	if (D.Save) D.Save();
}

void FS47ArrayBack::Leave()
{
	// the warning of the time away came: the dishes it takes are its; the rest turn back at once,
	// with no four and seven (the warning has the night)
	US47Audio* A = D.Audio.Get();
	bCut = true;
	HoldBeacons({});
	Field({});
	if (bMotors) { bMotors = false; if (A) A->StopMotors(); }
	if (Stage == TEXT("settle")) return;
	const double C = JsRound(Clock());
	if (!Run.On) Run.On = C;
	if (!Run.Lost) Run.Lost = C;
	if (!Run.Turn) { Run.Turn = C; Log(TEXT("ARRAY  S-01 TO S-27  MOTION  NO COMMAND"), C); }
	if (Stage != TEXT("back")) for (FRig& R : Rigs) if (R.bMine) { if (D.Dishes) D.Dishes->Dishes[R.I].bBack = true; R.VAz = R.VEl = 0; R.bOn = true; }
	Stage = TEXT("back"); T = 0; Said.bTurn = true;
	// (the crickets are the warning's now; this lets go of nothing when it ends)
	bHushed = false;
	Keep();
}

void FS47ArrayBack::Away()
{
	// out of SARO's area in the middle of it: home at once, unseen, and the log has the rest
	const double C = JsRound(Clock());
	FS47ArrayTry& R = Run;
	if (!R.On) { R.On = C; Log(TEXT("ARRAY  S-01 TO S-27  ON SCHEDULE  AZ 042 EL 48"), C); }
	if (!R.Lost) { R.Lost = FMath::Max(C, *R.On + HOLD); Log(TEXT("ARRAY  BEACONS S-01 TO S-27  PULSE GROUP 4 / 7"), *R.Lost); }
	if (!R.Turn) { R.Turn = FMath::Max(C, *R.Lost + JsRound(SIGNAL())); Log(TEXT("ARRAY  S-01 TO S-27  MOTION  NO COMMAND"), *R.Turn); Log(TEXT("ARRAY  DRIVE CURRENT 0.0 A"), *R.Turn); }
	int32 Mine = 0;
	for (const FRig& X : Rigs) if (X.bMine) { Mine++; if (D.Dishes) { D.Dishes->Snap(X.I, HOME_AZ, HOME_EL); D.Dishes->Dishes[X.I].bBack = false; D.Dishes->Dishes[X.I].Spin = 0; } }
	if (!R.Home) { R.Home = FMath::Max(C, *R.Turn + 13); Log(FString::Printf(TEXT("ARRAY  %d DRIVES  AZ 026 EL 32  NO COMMAND"), Mine), *R.Home); }
	HoldBeacons({});
	Field({});
	End(false);
}

void FS47ArrayBack::Drift(double Dt, const FS47ArrayIn& S)
{
	// idle: the lamps that went into one blink go back to their own, slowly (not while the dishes
	// that follow have some of them)
	if (!bDrifting || !D.Dishes) return;
	if (!S.bFollowIdle) { bDrifting = false; Lamps(); for (int32 I = 0; I < Ph.Num(); ++I) Ph[I] = Dish(I)->Phase; return; }
	bool bLeft = false;
	for (int32 I = 0; I < D.Dishes->Dishes.Num() && I < Ph.Num(); ++I)
	{
		const double E = WrapPi(Dish(I)->Phase - Ph[I]);
		if (FMath::Abs(E) > 0.004) { bLeft = true; Ph[I] += FMath::Clamp(E, -0.035 * Dt, 0.035 * Dt); BeaconPhase(I, Ph[I]); }
	}
	if (!bLeft) bDrifting = false;
}

bool FS47ArrayBack::Sees(const FS47ArrayIn& S) const
{
	// where the field can be seen from (out of doors, the control room's windows, the truck's
	// windscreen at SARO), and some of it on the screen
	return (S.bOutside || S.bRoom || S.bCab) && InView(S.Cam);
}

bool FS47ArrayBack::InView(const FS47Cam& Cam) const
{
	// some of the array's lamps on the screen, near enough
	if (S47Facing(Cam, Centre(), 700, 55)) return true;
	if (D.Dishes) for (const FS47Dish& Dd : D.Dishes->Dishes) if (S47Facing(Cam, FVector(Dd.X, 16 * Dd.Scale, Dd.Z), 500, 45)) return true;
	return false;
}

void FS47ArrayBack::Thump(double Level)
{
	// one pulse of the four and seven, heard out in the open: a sine falling from 118 to 41 Hz and
	// an octave over it, as the signal's pulse is drawn (SignalVoice.thump), without its clank
	US47Audio* A = D.Audio.Get();
	if (!A || !A->Ctx.IsValid()) return;
	const TSharedRef<FS47AudioContext> Ctx = A->Ctx.ToSharedRef();
	FS47AudioContext::FBatch B(*Ctx);
	const double T0 = Ctx->CurrentTime() + 0.005;
	const TSharedRef<FS47GainNode> Env = Ctx->CreateGain();
	Env->Gain.SetValueAtTime(0, T0); Env->Gain.LinearRampToValueAtTime(Level, T0 + 0.008); Env->Gain.ExponentialRampToValueAtTime(0.001, T0 + 0.42);
	Env->Connect(A->SfxBus);
	const double Parts[2][3] = { { 118, 41, 1 }, { 236, 82, 0.3 } };
	for (const auto& P : Parts)
	{
		const TSharedRef<FS47OscillatorNode> O = Ctx->CreateOscillator();
		O->Frequency.SetValueAtTime(P[0], T0); O->Frequency.ExponentialRampToValueAtTime(P[1], T0 + 0.18);
		const TSharedRef<FS47GainNode> Og = Ctx->CreateGain(); Og->Gain.SetValue(P[2]);
		O->Connect(Og); Og->Connect(Env);
		O->Start(T0); O->Stop(T0 + 0.45);
	}
	TSharedPtr<FS47GainNode> E = Env;
	Ctx->SetTimeout(0.9, [E] { E->Disconnect(); });
}

void FS47ArrayBack::Clunk(int32 I, bool bStart, bool bCalm, double When)
{
	// a contactor and the brake at a dish's foot: letting go (start) or setting (stop)
	US47Audio* A = D.Audio.Get();
	const FS47Dish* Dd = Dish(I);
	if (!A || !Dd) return;
	const TSharedPtr<FS47AudioNode> Sp = A->Spatial(FVector(Dd->X, 6 * Dd->Scale, Dd->Z), 6);
	if (!Sp.IsValid()) return;
	const double K = bCalm ? 0.6 : 1;
	const FString Sw = A->Or(TEXT("rack_switch"), TEXT("switch")), Brake = A->Or(TEXT("door_heavy"), TEXT("doorMetal"));
	auto Play = [&](const FString& Name, double Gain, double Rate, double Lp, double W)
	{
		FS47Sound O; O.DestNode = Sp; O.Gain = float(Gain); O.Rate = float(Rate); O.Lp = float(Lp); O.When = float(W);
		A->Sfx(Name, O);
	};
	if (bStart) { Play(Sw, 0.45 * K, 0.62, 0, When); Play(Brake, 0.34 * K, 0.5, 1300, When + 0.09); }
	else { Play(Brake, 0.3 * K, 0.45, 900, When); Play(Sw, 0.32 * K, 0.55, 0, When + 0.06); }
}

// ---------- the case ----------

void FS47ArrayBack::Reset()
{
	US47Audio* A = D.Audio.Get();
	if (bMotors && A) A->StopMotors();
	if (bHushed && !bCut && D.Hush) D.Hush(0, 1, 0.3, 1);
	bMotors = bHushed = bCut = bDrifting = false;
	HoldBeacons({});
	Field({});
	for (const FRig& R : Rigs) if (D.Dishes) { D.Dishes->Dishes[R.I].bBack = false; D.Dishes->Dishes[R.I].Spin = 0; }
	Rigs.Reset(); Stage = TEXT("idle"); T = 0; bLit = false;
	// (the lamps each in its own blink again, at once)
	if (Ph.Num() && D.Dishes) for (int32 I = 0; I < D.Dishes->Dishes.Num(); ++I) BeaconPhase(I, Dish(I)->Phase);
	Ph.Reset();
}

void FS47ArrayBack::Restore()
{
	Reset();
	const TOptional<FS47ArraySave> V = D.Get ? D.Get() : TOptional<FS47ArraySave>();
	if (!V || !V->Run) return;
	// (kept part way: the log has what came before the save; this writes the rest at its own times)
	const FS47ArrayTry& R = *V->Run;
	const double On = R.On ? *R.On : R.Cmd + 14, Lost = R.Lost ? *R.Lost : On + HOLD, Tn = R.Turn ? *R.Turn : Lost + JsRound(SIGNAL()), Hm = R.Home ? *R.Home : Tn + 13;
	if (!R.On) Log(TEXT("ARRAY  S-01 TO S-27  ON SCHEDULE  AZ 042 EL 48"), On);
	if (!R.Lost) Log(TEXT("ARRAY  BEACONS S-01 TO S-27  PULSE GROUP 4 / 7"), Lost);
	if (!R.Turn) { Log(TEXT("ARRAY  S-01 TO S-27  MOTION  NO COMMAND"), Tn); Log(TEXT("ARRAY  DRIVE CURRENT 0.0 A"), Tn); }
	if (!R.Home) Log(TEXT("ARRAY  27 DRIVES  AZ 026 EL 32  NO COMMAND"), Hm);
	FS47ArraySave Out; Out.Runs = V->Runs;
	FS47ArrayTry Done; Done.Cmd = R.Cmd; Done.On = On; Done.Lost = Lost; Done.Turn = Tn; Done.Home = Hm;
	Out.Runs.Add(Done);
	if (D.Set) D.Set(Out);
	if (D.Dishes) for (int32 I = 0; I < D.Dishes->Dishes.Num(); ++I) { D.Dishes->Snap(I, HOME_AZ, HOME_EL); D.Dishes->Dishes[I].bBack = false; D.Dishes->Dishes[I].Spin = 0; }
	Note(FString::Printf(TEXT("%s. ARRAY RESUME. The console log has the array on the schedule for 47 seconds, then back on 026 by itself. Nobody sent a command."), *Clk(R.Cmd, false)));
	if (D.S03) D.S03();
}

bool FS47ArrayBack::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s array back: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// the lamps' four and seven
	int32 Ons = 0; bool bWas = false;
	for (double Ts = 0; Ts < SIGNAL(); Ts += 0.01) { const FLamps L = LampsAt(Ts); if (L.bOn && !bWas) Ons++; bWas = L.bOn; }
	Check(Ons == 11 && !LampsAt(0.5).bOn && LampsAt(DARK0 + 0.1).N == 0 && LampsAt(SEVEN + 0.1).N == 4, TEXT("four flashes, a gap, seven"));
	Check(Times(1) == TEXT("once") && Times(2) == TEXT("twice") && Times(3) == TEXT("three times") && Times(14) == TEXT("14 times"), TEXT("times in words"));
	// the console's command, without the world (a store in memory, no dishes)
	TOptional<FS47ArraySave> Store;
	TArray<TPair<double, FString>> Logs;
	TArray<FString> Notes;
	double Now = 3 * 3600;
	FS47ArrayDeps Dp;
	Dp.Get = [&Store] { return Store; };
	Dp.Set = [&Store](const FS47ArraySave& V) { Store = V; };
	Dp.Clock = [&Now] { return Now; };
	Dp.Log = [&Logs](const FString& S, double At) { Logs.Add({ At, S }); };
	Dp.Note = [&Notes](const FString& S) { Notes.Add(S); };
	FS47ArrayBack Ab(Dp);
	Check(!Ab.Command(TEXT("SHOW LOG"), FString()).IsSet(), TEXT("not one of its commands: unset"));
	Check(Ab.Command(TEXT("  array   resume "), TEXT("release")).GetValue().StartsWith(TEXT("%ARRAY-E-HELD")) && !Ab.Busy(), TEXT("held for the supervisor before Ward has asked"));
	const TOptional<FString> Go = Ab.Command(TEXT("ARRAY RESUME"), FString());
	Check(Go && Go->StartsWith(TEXT("ARRAY RESUME\nSCHEDULE  RUN860413_2341")) && Ab.Busy() && Store && Store->Run && Store->Run->Cmd == Now, TEXT("ARRAY RESUME: armed, the try kept as it goes"));
	Check(Ab.Command(TEXT("RESUME ARRAY"), FString()).GetValue() == TEXT("%ARRAY-W-INPROG, schedule resume in progress"), TEXT("in progress"));
	// away from SARO in the middle of it: home at once, the log has the rest, counted
	FS47ArrayIn S; S.bSaro = false;
	Ab.Update(0.1, S);
	Check(!Ab.Busy() && Ab.Count() == 1 && Store && !Store->Run && Store->Runs[0].Home.IsSet() && Notes.Num() == 1 && Notes[0].Contains(TEXT("The console log has the array on the schedule")),
		TEXT("gone from SARO: over, counted, the log written"));
	Check(Logs.ContainsByPredicate([](const TPair<double, FString>& L) { return L.Value == TEXT("ARRAY  DRIVE CURRENT 0.0 A"); }), TEXT("DRIVE CURRENT 0.0 A in the log"));
	const TOptional<FHistory> H = Ab.History();
	Check(H && H->Cmds == 1 && H->Text.StartsWith(TEXT("SINCE 03:00:\n03:00:00  CMD RESUME   AZ 042 EL 48")), TEXT("S-03's history"));
	// the next command says where it left the schedule
	Now += 600;
	const TOptional<FString> Again = Ab.Command(TEXT("ARRAY RESUME"), FString());
	Check(Again && Again->StartsWith(TEXT("%ARRAY-W-LEFTSCHED, left schedule 03:00:")), TEXT("the second time: LEFTSCHED first"));
	// a case kept in the middle of it comes back over
	Ab.Restore();
	Check(Store && !Store->Run && Store->Runs.Num() == 2 && Store->Runs[1].Home.IsSet(), TEXT("restore: a try kept part way is finished and counted"));
	// the JSON round trip
	const TOptional<FS47ArraySave> Back = FS47ArraySave::Copy(Store->ToJson());
	Check(Back && Back->Runs.Num() == 2 && Back->Runs[1].Turn == Store->Runs[1].Turn, TEXT("kept with the case and read back"));
	return bOk;
}

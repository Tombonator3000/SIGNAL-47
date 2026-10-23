// story/curve.ts
#include "S47Curve.h"
#include "S47Timeline.h"

namespace
{
	const double PLAIN = 600;                                  // ten minutes of plain night after the prologue
	const double REST[2] = { 480, 720 };                       // after a peak: 8 to 12 minutes
	const double EBB[2] = { 180, 240 };                        // after a peak that never came to anything
	const double SHORT = 150;                                  // a night put back from a case without a curve
	const double BUILD[2] = { 390, 170 };                      // the build's length at 02:16 and at 05:29 (play seconds)
	const double PEAK[2] = { 300, 240 };                       // how long the peak waits, out in the night, for something to land
	const double HURRY = 40;                                   // a waiting 47 here: the rest of the build in at most this (play seconds)
	const double LINGER = 5;                                   // and staying by it this long, the rest lets go after its first EBB[0] s
	const double NIGHT0 = 2 * 3600 + 16 * 60, NIGHT1 = 5 * 3600 + 29 * 60;
	const TCHAR* CHAPTERS[] = { TEXT("ch1"), TEXT("ch2"), TEXT("ch3"), TEXT("ch4"), TEXT("ch5") };
	bool IsChapter(const FString& C) { for (const TCHAR* K : CHAPTERS) if (C == K) return true; return false; }
	/** How far into the build each kind may start (glimpses and the time away only at the peak). */
	double FROM(const FString& Kind)
	{
		static const TMap<FString, double> F = { { TEXT("sign"), 0.12 }, { TEXT("call"), 0.3 }, { TEXT("cutout"), 0.12 }, { TEXT("post"), 0.2 }, { TEXT("event"), 0.45 },
			{ TEXT("long"), 0.6 }, { TEXT("scare"), 0.5 }, { TEXT("glimpse"), 9 }, { TEXT("abduct"), 9 } };
		const double* V = F.Find(Kind);
		return V ? *V : 9;
	}
	/** The cutout keeps out of the end of the build: never there at the peak's moment. */
	const double CUTOUT_UNTIL = 0.78;
	/** The peaks that are heard or felt (they count when they come). */
	bool Felt(const FString& Kind, const FString& What)
	{
		static const TMap<FString, TArray<FString>> F = {
			{ TEXT("abduction"), { TEXT("warning") } }, { TEXT("roof"), { TEXT("steps") } }, { TEXT("scare"), { TEXT("silence") } },
			{ TEXT("event"), { TEXT("bulletin"), TEXT("phone"), TEXT("phone2"), TEXT("tv"), TEXT("lamps"), TEXT("reel"), TEXT("blackout"), TEXT("officephone"), TEXT("array"), TEXT("archive"), TEXT("reel047"), TEXT("truck7"), TEXT("truck7m") } },
		};
		const TArray<FString>* L = F.Find(Kind);
		return L && L->Contains(What);
	}
	/** Peaks the player can make again and again (the array put back on the schedule): each lands the
	 *  curve, but one that comes in a rest does not start the rest over. */
	bool Again(const FString& Key) { return Key == TEXT("event:array"); }
	/** Those that count only once they were seen (story/seen.ts, looking); unset: not one of them. */
	TOptional<bool> Seen(const FString& Kind, const FString& What)
	{
		if (Kind == TEXT("glimpse")) return What != TEXT("on film");
		if (Kind == TEXT("face")) return true;
		if (Kind == TEXT("event")) return What == TEXT("lights");
		// (the recording walking where the player was taken, story/recording.ts)
		if (Kind == TEXT("scare")) return What.StartsWith(TEXT("walk "));
		return {};
	}
	/** The 47-second ones (the per-chapter limit). */
	bool IsLong(const FString& Key)
	{
		return Key == TEXT("scare:silence") || Key == TEXT("event:bulletin") || Key == TEXT("event:phone") || Key == TEXT("event:tv") || Key == TEXT("event:blackout") || Key == TEXT("event:archive");
	}

	double Rnd(double A, double B) { return A + FMath::FRand() * (B - A); }
	double Lerp(double A, double B, double K) { return A + (B - A) * K; }
	double Clamp01(double V) { return FMath::Min(1.0, FMath::Max(0.0, V)); }
	double Smooth(double A, double B, double V) { const double K = Clamp01((V - A) / (B - A)); return K * K * (3 - 2 * K); }
	double Round(double V) { return FMath::FloorToDouble(V + 0.5); } // Math.round
}

bool FS47NightCurve::May(const FString& Kind) const
{
	if (bFree) return true;
	if (bHeld || bBusy) return false;
	if (Kind == TEXT("long") && (Long.Contains(Chapter) || bKeep)) return false;
	// the peak this is hurrying to is kept for the 47 that waits here: nothing else takes it
	if (bPulled && Kind != PullKind && Kind != TEXT("sign")) return false;
	const double S = Strength;
	if (Kind == TEXT("call") && Took.Call >= (S > 0.45 ? 2 : 1)) return false;
	if (Kind == TEXT("event") && Took.Event >= (S > 0.5 ? 2 : 1)) return false;
	if (Phase == TEXT("build"))
	{
		if (Level < FROM(Kind)) return false;
		if (Kind == TEXT("cutout")) return Level < CUTOUT_UNTIL && Took.Cutout < (S > 0.6 ? 2 : 1);
		if (Kind == TEXT("post")) return Level < 0.85;
		return true;
	}
	if (Phase == TEXT("peak")) return Kind != TEXT("cutout") && Kind != TEXT("post");
	return false;
}

bool FS47NightCurve::CutoutKeep() const
{
	if (bFree) return true;
	return Phase == TEXT("build") && Level < CUTOUT_UNTIL && !bBusy;
}

double FS47NightCurve::Pace(const FString& Kind) const
{
	if (bFree) return 1;
	if (Phase == TEXT("peak")) return Kind == TEXT("glimpse") ? 3.5 : 1.5;
	if (Phase == TEXT("build")) return Kind == TEXT("cutout") ? 3 : 1 + 1.2 * Level;
	return 1;
}

double FS47NightCurve::Thin() const
{
	if (bFree) return 0;
	if (Phase == TEXT("build")) return Smooth(0.08, 0.85, Level) * Lerp(0.6, 0.9, Strength);
	if (Phase == TEXT("peak")) return Lerp(0.7, 0.95, Strength);
	return 0;
}

void FS47NightCurve::Started(const FString& Kind)
{
	if (Kind == TEXT("call")) Took.Call++;
	else if (Kind == TEXT("cutout")) Took.Cutout++;
	else if (Kind == TEXT("event")) Took.Event++;
}

void FS47NightCurve::Update(double Dt, const FS47CurveIn& S)
{
	Chapter = S.Chapter;
	bHeld = S.bHeld; bBusy = S.bBusy; bKeep = S.bKeep;
	bSign = false;
	// (not for a chapter that has had its extra 47, or keeps it for the story's: then there is
	// nothing here to hurry for)
	const bool bLong = S.bWaiting && !Long.Contains(S.Chapter) && !bKeep;
	bPulled = !bFree && (bLong || S.bWaitingScare);
	PullKind = bLong ? TEXT("long") : TEXT("scare");
	if (!IsChapter(S.Chapter)) { if (Phase != TEXT("off")) Stop(); return; }
	if (Phase == TEXT("off")) Begin();
	const double Start = S.bEveningTimeline ? 16 * 60 : NIGHT0;
	const double Now = S.bEveningTimeline ? S47Timeline::Night(S.Clock) : S47Timeline::Day(S.Clock);
	Strength = Clamp01((Now - Start) / (NIGHT1 - Start));
	ReadLog(S.Clock);
	if (S.bHeld) return;
	Step(Dt, S.bExposed, S.bBusy, S.bPeaking);
}

void FS47NightCurve::Step(double Dt, bool bExposed, bool bInBusy, bool bPeaking)
{
	PullT = bPulled ? PullT + Dt : 0;
	if (Phase == TEXT("plain"))
	{
		if ((T += Dt) >= Len) To(TEXT("build"));
	}
	else if (Phase == TEXT("build"))
	{
		// indoors the night goes on building, but slowly: it is out there it is felt; where a 47
		// waits for the player, what is left of it goes in at most HURRY seconds, indoors too
		const double P = bExposed ? 1 : 0.35;
		T += Dt * (bPulled ? FMath::Max(P, Len / HURRY) : P);
		Level = Clamp01(T / Len);
		Signs(Dt, bExposed, bInBusy);
		if (Level >= 1) To(TEXT("peak"));
	}
	else if (Phase == TEXT("peak"))
	{
		Level = 1;
		Signs(Dt, bExposed, bInBusy);
		// the peak waits while the player is somewhere it cannot reach them, or while something runs,
		// or while the 47 it is kept for waits for them here
		if (bExposed && !bInBusy && !bPulled && (T += Dt) >= Len) { bEbb = true; To(TEXT("rest")); }
	}
	else if (Phase == TEXT("rest"))
	{
		Level = FMath::Max(0.0, Level - Dt / 20);
		// (staying by a 47 that waits there: only the breath after the peak, then the hurried build)
		if (!bPeaking && ((T += Dt) >= Len || (bPulled && PullT >= LINGER && T >= EBB[0]))) To(TEXT("build"));
	}
}

void FS47NightCurve::Signs(double Dt, bool bExposed, bool bInBusy)
{
	// the small signs: now and then, closer together as the build goes on and towards morning; not
	// while one of the night's things is going (one at a time)
	if (!bExposed || bInBusy || Level < FROM(TEXT("sign"))) return;
	if ((SignT -= Dt) > 0) return;
	bSign = true;
	SignT = Rnd(28, 60) / (0.55 + Level) * (1 - 0.35 * Strength);
}

void FS47NightCurve::To(const FString& InPhase, TOptional<double> InLen)
{
	Phase = InPhase; T = 0;
	const double S = Strength;
	if (InPhase == TEXT("plain")) { Len = InLen.Get(PLAIN); Level = 0; }
	else if (InPhase == TEXT("build"))
	{
		Len = InLen ? *InLen : Lerp(BUILD[0], BUILD[1], S) * Rnd(0.85, 1.15);
		Level = 0; bEbb = false; Builds++;
		Took.Call = Took.Cutout = Took.Event = 0;
		SignT = Rnd(20, 45);
	}
	else if (InPhase == TEXT("peak")) { Len = InLen ? *InLen : Lerp(PEAK[0], PEAK[1], S); Level = 1; }
	else if (InPhase == TEXT("rest"))
	{
		// 8 to 12 minutes, the shorter towards morning; a peak that never landed lets go for less
		Len = InLen ? *InLen : (bEbb ? Rnd(EBB[0], EBB[1]) : FMath::Min(REST[1], FMath::Max(REST[0], Lerp(REST[1], REST[0], S) + Rnd(-40, 40))));
	}
}

void FS47NightCurve::ReadLog(double Clock)
{
	// what the night did since the last look (story/seen.ts): a peak the player got lands the curve
	const TArray<FS47SeenRef>& All = Log.Entries;
	const int32 Fresh = FMath::Min(All.Num(), Log.Added - Read);
	Read = Log.Added;
	for (int32 I = Fresh > 0 ? All.Num() - Fresh : All.Num(); I < All.Num(); ++I)
	{
		const FS47SeenRef E = All[I];
		if (E->Kind == TEXT("call")) { Took.Call++; continue; }
		const TOptional<bool> S = Seen(E->Kind, E->What);
		if (Felt(E->Kind, E->What)) Land(*E, Clock);
		else if ((E->Kind == TEXT("event") && !Seen(TEXT("event"), E->What).GetValue()) || (E->Kind == TEXT("scare") && E->What == TEXT("car lights"))) Took.Event++;
		else if (S && *S) { if (E->Looking.Get(false)) Land(*E, Clock); else Pending.Add(E); }
	}
	// a glimpse counts once it was looked at; if its watch ends unseen it never does
	if (Pending.Num())
	{
		TArray<FS47SeenRef> Keep;
		for (const FS47SeenRef& E : Pending)
		{
			if (E->Looking.Get(false)) { Land(*E, Clock); continue; }
			if (E->Kind == TEXT("face") || Log.Watching(E)) Keep.Add(E);
		}
		Pending = MoveTemp(Keep);
	}
}

void FS47NightCurve::Land(const FS47SeenEntry& E, double Clock)
{
	const FString Key = E.Kind + TEXT(":") + E.What;
	if (IsLong(Key) && !Chapter.IsEmpty()) Long.Add(Chapter);
	Landed = FLanded{ E.Kind, E.What, Round(Clock) };
	if (Again(Key) && Phase == TEXT("rest")) return;
	Cycle++;
	bEbb = false;
	To(TEXT("rest"));
}

void FS47NightCurve::Begin()
{
	// the free night begins (chapter one after the prologue), or a night put back goes on
	const TOptional<FSave> R = Restored; Restored.Reset();
	Read = Log.Added; Pending.Reset();
	if (R)
	{
		To(R->Phase == TEXT("off") ? FString(TEXT("plain")) : R->Phase, R->Len > 0 ? TOptional<double>(R->Len) : TOptional<double>());
		T = FMath::Min(R->T, Len); bEbb = R->bEbb;
		if (Phase == TEXT("build")) Level = Clamp01(T / Len);
		if (Phase == TEXT("peak")) Level = 1;
		return;
	}
	if (Resume == TEXT("short")) To(TEXT("rest"), SHORT);
	else To(TEXT("plain"));
}

void FS47NightCurve::Stop()
{
	Phase = TEXT("off"); T = 0; Len = 0; Level = 0; Pending.Reset(); bPulled = false; PullT = 0;
}

S47J::FObj FS47NightCurve::Data() const
{
	const S47J::FObj O = S47J::New();
	S47J::Set(O, TEXT("phase"), Phase);
	S47J::Set(O, TEXT("t"), Round(T));
	S47J::Set(O, TEXT("len"), Round(Len));
	S47J::Set(O, TEXT("cycle"), Cycle);
	S47J::Set(O, TEXT("long"), Long.Array());
	if (bEbb) S47J::Set(O, TEXT("ebb"), true); // (ebb || undefined)
	return O;
}

void FS47NightCurve::Restore(const S47J::FObj& Save, bool bStarted)
{
	// a save opened (its curve, or none: an older case goes on after a short quiet), or a new night
	// (null: the plain ten minutes when chapter one begins); a save from before the curve began (the
	// prologue, from 02:13) has it 'off': its night still gets the plain ten minutes
	Stop(); Cycle = 0; Long.Reset(); Landed.Reset(); bEbb = false;
	Took = FTook();
	Restored.Reset(); Resume = bStarted ? TEXT("short") : TEXT("plain");
	const FString P = S47J::Str(Save, TEXT("phase"));
	if (Save.IsValid() && (P == TEXT("off") || P == TEXT("plain") || P == TEXT("build") || P == TEXT("peak") || P == TEXT("rest")))
	{
		Resume = TEXT("plain");
		Cycle = int32(S47J::Num(Save, TEXT("cycle"), 0));
		for (const FString& C : S47J::Strs(Save, TEXT("long"))) if (IsChapter(C)) Long.Add(C);
		if (P != TEXT("off")) Restored = FSave{ P, S47J::Num(Save, TEXT("t"), 0), S47J::Num(Save, TEXT("len"), 0), S47J::Bool(Save, TEXT("ebb")) };
	}
	Read = Log.Added;
}

bool FS47NightCurve::Force(const FString& InPhase, double InLevel)
{
	if (Phase == TEXT("off") && !IsChapter(Chapter)) return false;
	To(InPhase);
	if (InPhase == TEXT("build")) { T = Clamp01(InLevel) * Len; Level = Clamp01(InLevel); }
	return true;
}

void FS47NightCurve::Advance(double Secs)
{
	for (double L = Secs; L > 0; L -= 1) Step(FMath::Min(1.0, L), true, false, false);
	bSign = false;
}

FString FS47NightCurve::Text() const
{
	if (Phase == TEXT("off")) return FString::Printf(TEXT("curve off%s"), bFree ? TEXT(" (free)") : TEXT(""));
	const TArray<FString> L = Long.Array();
	return FString::Printf(TEXT("curve %s %.2f %ds c%d s%.2f%s%s%s%s"), *Phase, Level, Left(), Cycle, Strength, bPulled ? TEXT(" pull") : TEXT(""),
		L.Num() ? *(TEXT(" 47:") + FString::Join(L, TEXT(","))) : TEXT(""), Landed ? *(TEXT(" last ") + Landed->What.Left(10)) : TEXT(""), bFree ? TEXT(" free") : TEXT(""));
}

// ---------- self test ----------

bool FS47NightCurve::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s curve: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	FS47SeenLog Log;
	FS47NightCurve C(Log);
	FS47CurveIn In;
	In.Chapter = TEXT("prologue"); In.Clock = 2 * 3600 + 14 * 60; In.bExposed = true;
	C.Update(1, In);
	Check(C.Phase == TEXT("off") && !C.May(TEXT("call")), TEXT("nothing in the prologue"));
	In.Chapter = TEXT("ch1"); In.Clock = 2 * 3600 + 16 * 60;
	C.Update(1, In);
	Check(C.Phase == TEXT("plain") && C.Len == 600 && C.T == 1 && !C.May(TEXT("sign")) && C.Strength == 0, TEXT("ten minutes of plain night from 02:16"));
	C.Update(600, In);
	Check(C.Phase == TEXT("build") && C.Builds == 1 && C.Len >= 390 * 0.85 - 1e-9 && C.Len <= 390 * 1.15 + 1e-9, TEXT("then the build, 390 s give or take 15 percent"));
	C.Force(TEXT("build"), 0.5);
	Check(C.May(TEXT("call")) && C.May(TEXT("event")) && !C.May(TEXT("long")) && !C.May(TEXT("glimpse")) && C.May(TEXT("cutout")), TEXT("half way: calls, events and the cutout, no 47 and no glimpse"));
	Check(FMath::IsNearlyEqual(C.Pace(TEXT("call")), 1.6) && C.Pace(TEXT("cutout")) == 3, TEXT("the build's pace"));
	In.bBusy = true; C.Update(0.01, In);
	Check(!C.May(TEXT("call")), TEXT("one at a time: nothing while something is going"));
	In.bBusy = false; C.Update(0.01, In);
	C.Started(TEXT("call"));
	Check(!C.May(TEXT("call")), TEXT("one call in a cycle early in the night"));
	C.Force(TEXT("peak"));
	Check(C.May(TEXT("glimpse")) && C.May(TEXT("long")) && !C.May(TEXT("cutout")) && C.Pace(TEXT("glimpse")) == 3.5 && FMath::IsNearlyEqual(C.Thin(), 0.7), TEXT("the peak: a glimpse or a 47, no cutout"));
	// a glimpse not looked at does not land the peak; looked at, it does
	auto Entry = [](const TCHAR* Kind, const TCHAR* What) { const FS47SeenRef E = MakeShared<FS47SeenEntry>(); E->Kind = Kind; E->What = What; return E; };
	FS47SeenWatch W; W.Pos = [] { return TOptional<FVector>(FVector(0, 0, 100)); }; W.Secs = 60;
	const FS47SeenRef G = Log.Add(Entry(TEXT("glimpse"), TEXT("S-11")), W);
	C.Update(0.1, In);
	Check(C.Phase == TEXT("peak") && C.Cycle == 0, TEXT("a glimpse unseen does not land"));
	G->Looking = true;
	C.Update(0.1, In);
	Check(C.Phase == TEXT("rest") && C.Cycle == 1 && C.Len >= 480 && C.Len <= 720 && C.Landed && C.Landed->What == TEXT("S-11"), TEXT("looked at, it lands: the rest"));
	Check(!C.May(TEXT("call")) && !C.May(TEXT("glimpse")) && C.Thin() == 0, TEXT("nothing in the rest"));
	// a 47 lands the peak and takes the chapter's one extra
	C.Force(TEXT("peak"));
	Log.Add(Entry(TEXT("event"), TEXT("bulletin")));
	C.Update(0.1, In);
	Check(C.Phase == TEXT("rest") && C.Long.Contains(TEXT("ch1")) && !C.Data()->HasField(TEXT("ebb")), TEXT("the bulletin is chapter one's 47"));
	C.Force(TEXT("peak"));
	Check(!C.May(TEXT("long")) && C.May(TEXT("glimpse")), TEXT("no second 47 in the chapter"));
	// a peak that never lands ebbs out after its wait
	C.Update(301, In);
	Check(C.Phase == TEXT("rest") && C.bEbb && C.Len >= 180 && C.Len <= 240, TEXT("a peak that never came ebbs"));
	// the save and back
	const S47J::FObj D = C.Data();
	FS47NightCurve C2(Log);
	C2.Restore(S47J::Read(S47J::Write(D)), true);
	C2.Update(0.5, In);
	Check(C2.Phase == TEXT("rest") && C2.bEbb && C2.Cycle == 2 && C2.Long.Contains(TEXT("ch1")) && FMath::IsNearlyEqual(C2.T, 0.5, 1e-9) && FMath::IsNearlyEqual(C2.Len, Round(C.Len)), TEXT("a save and back"));
	FS47NightCurve C3(Log);
	C3.Restore(nullptr, true);
	C3.Update(0.1, In);
	Check(C3.Phase == TEXT("rest") && C3.Len == 150, TEXT("a case without a curve goes on after a short quiet"));
	In.Clock = 3 * 3600 + 52 * 60 + 30;
	C3.Update(0, In);
	Check(FMath::IsNearlyEqual(C3.Strength, 0.5), TEXT("the strength is half way at 03:52:30"));
	In.Chapter = TEXT("ch6");
	C3.Update(0.1, In);
	Check(C3.Phase == TEXT("off"), TEXT("chapter six stops it"));
	return bOk;
}

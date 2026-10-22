// story/nightEvents.ts
#include "S47NightEvents.h"

#include "S47Docs.h"
#include "S47WebAudio.h"

namespace
{
	double Rnd(double A, double B) { return A + FMath::FRand() * (B - A); }
	const TCHAR* ALL[] = { TEXT("steps"), TEXT("knocks"), TEXT("radio"), TEXT("bulletin"), TEXT("phone"), TEXT("door"), TEXT("silence") };
}

const TArray<FS47BulletinLine>& S47Bulletin()
{
	static const TArray<FS47BulletinLine> B = {
		{ TEXT("We interrupt this program. This is a local news bulletin."), TEXT("news"), TEXT("a bulletin") },
		{ TEXT("Roswell, New Mexico. Tuesday, July eighth, nineteen forty-seven."), TEXT("news"), TEXT("a bulletin") },
		{ TEXT("A flying disc is reported recovered on a ranch outside town. That is the wording on the notice."), TEXT("news"), TEXT("a bulletin") },
		{ TEXT("There is no description of the object. The space for the name of the person reporting it is blank."), TEXT("news"), TEXT("a bulletin") },
		{ TEXT("We have received the same line twice. We are reading it just as it came through."), TEXT("news"), TEXT("a bulletin") },
		{ TEXT("Please stand by. We will return to our regular program."), TEXT("news"), TEXT("a bulletin") },
	};
	return B;
}

FS47NightEvents::FS47NightEvents(FS47NightDeps InD) : D(MoveTemp(InD))
{
	Wait = Rnd(90, 160);
}

bool FS47NightEvents::Busy() const { return Pending.IsSet() || StepsLeft.Num() > 0 || IsSilent(); }

bool FS47NightEvents::Running() const
{
	return (Pending && Pending->Id != TEXT("radio") && Pending->Id != TEXT("door")) || StepsLeft.Num() > 0 || IsSilent();
}

int32 FS47NightEvents::Count(const FString& Id) const
{
	int32 N = 0;
	if (D.Seen) for (const FString& K : D.Seen->Keys) if (K.StartsWith(TEXT("ne:") + Id)) N++;
	return N;
}

void FS47NightEvents::Done(const FString& Id, double Clock)
{
	if (D.Seen) D.Seen->Add(FString::Printf(TEXT("ne:%s%d"), *Id, Count(Id) + 1));
	Last = FLast{ Id, Clock };
	Wait = Rnd(130, 260);
}

bool FS47NightEvents::Can(const FString& Id, const FS47NightIn& S) const
{
	const TOptional<FS47NightIn::FFoot>& F = S.Foot;
	if (Id == TEXT("steps")) return F && F->bOutside && Count(TEXT("steps")) < 3 && Still > 0.25 && Still < 1 && Walked > 8;
	if (Id == TEXT("knocks")) return S.bDriving && Count(TEXT("knocks")) < 1;
	if (Id == TEXT("radio")) return F && S.Truck && !S.Truck->bRadioOn && Count(TEXT("radio")) < 1 && RadioNear > 14 && RadioNear < 40;
	if (Id == TEXT("bulletin")) return S.bRadio1420 && Count(TEXT("bulletin")) < 1;
	if (Id == TEXT("phone")) return F && F->bOutside && S.Saro && FVector::Dist(F->Pos, S.Saro->Phone) < 30 && Count(TEXT("phone")) < 2;
	if (Id == TEXT("door")) return F && F->bOutside && S.Saro && !S.Saro->bDoorOpen && FVector::Dist(F->Pos, S.Saro->Door) > 25 && !InView(S.Cam, S.Saro->Door) && Count(TEXT("door")) < 1;
	if (Id == TEXT("silence")) return F && F->bOutside && S.bSilenceFree && Count(TEXT("silence")) < 1;
	return false;
}

bool FS47NightEvents::InView(const FS47Cam& Cam, const FVector& At)
{
	const FVector N = Cam.Project(At);
	return N.Z < 1 && FMath::Abs(N.X) < 1.05 && FMath::Abs(N.Y) < 1.05;
}

void FS47NightEvents::Update(double Dt, const FS47NightIn& S)
{
	const TOptional<FS47NightIn::FFoot>& F = S.Foot;
	if (F && F->bMoving) { Walked += Dt; Still = 0; } else { Still += Dt; if (Still > 1.5) Walked = 0; }
	if (F && S.Truck) RadioNear = FVector::Dist(F->Pos, S.Truck->Pos);
	StepsBehind(Dt, S);
	// something started that ends with a note: when it has been seen or heard through
	if (Pending)
	{
		FPending& P = *Pending;
		if (P.Id == TEXT("bulletin") && D.BulletinProgress)
		{
			const TOptional<FS47ArchiveProgress> Progress = D.BulletinProgress();
			if (!Progress || !P.ArchiveId || Progress->Id != *P.ArchiveId || Progress->State == TEXT("cancelled"))
			{
				Pending.Reset(); Wait = Rnd(130, 260); return;
			}
			// A menu pause suspends both clocks; it does not make an otherwise heard recording
			// incomplete. Story/receiver interruptions still invalidate the whole finding.
			US47Audio* A = D.Audio.Get();
			const bool bSuspended = A && A->Ctx.IsValid() && A->Ctx->State() == TEXT("suspended");
			if ((!S.bOk && !bSuspended) || (D.BulletinHeard && !D.BulletinHeard())) P.Heard = false;
			P.T = FMath::Max(0.0, Progress->Duration - Progress->Elapsed);
			if (Progress->State == TEXT("complete") && P.T <= 0)
			{
				const FPending Was = P;
				Pending.Reset();
				if (Was.Heard != TOptional<bool>(false)) Finish(Was.Id, S);
				else Wait = Rnd(130, 260);
			}
			return;
		}
		if (P.Id == TEXT("bulletin") && (!S.bOk || (D.BulletinHeard && !D.BulletinHeard()))) P.Heard = false;
		P.T -= Dt;
		if (P.T <= 0 && (!P.Check || P.Check()))
		{
			const FPending Was = P;
			Pending.Reset();
			if (Was.Heard != TOptional<bool>(false)) Finish(Was.Id, S);
			else Wait = Rnd(130, 260);
		}
		return;
	}
	if (!S.bOk || (S.May == TOptional<bool>(false) && S.MayLong == TOptional<bool>(false))) return;
	Wait -= Dt * S.Pace.Get(1);
	if (Wait > 0) return;
	// (the night's curve: the 47-second ones are a peak, the rest come in its build)
	TArray<FString> CanNow;
	for (const TCHAR* Id : ALL)
	{
		const bool bLong = FCString::Strcmp(Id, TEXT("bulletin")) == 0 || FCString::Strcmp(Id, TEXT("silence")) == 0;
		if (Can(Id, S) && (bLong ? S.MayLong != TOptional<bool>(false) : S.May != TOptional<bool>(false))) CanNow.Add(Id);
	}
	if (!CanNow.Num()) return;
	Start(CanNow[FMath::Min(CanNow.Num() - 1, int32(FMath::FRand() * CanNow.Num()))], S);
}

bool FS47NightEvents::Now(const FString& Id, const FS47NightIn& S)
{
	if (Busy()) return false;
	if (!Can(Id, S) && Id != TEXT("steps")) return false;
	return Start(Id, S);
}

bool FS47NightEvents::Start(const FString& Id, const FS47NightIn& S)
{
	US47Audio* A = D.Audio.Get();
	const FS47Cam& Cam = S.Cam;
	if (Id == TEXT("steps"))
	{
		// behind, where the player came from: on the same ground, slower than theirs
		const FVector Back = FVector(-Cam.Forward.X, 0, -Cam.Forward.Z).GetSafeNormal();
		StepAt = Cam.Pos + Back * Rnd(3.5, 5.5);
		StepAt.Y = Cam.Pos.Y - 1.5;
		const int32 N = FMath::FRand() < 0.5 ? 2 : 3;
		StepsLeft.Reset();
		for (int32 I = 0; I < N; ++I) StepsLeft.Add(0.35 + I * Rnd(0.55, 0.7));
		Pending = FPending{ Id, StepsLeft[N - 1] + 1.5 };
	}
	else if (Id == TEXT("knocks"))
	{
		for (const float W : { 0.f, 0.95f, 1.9f })
		{
			FS47Sound O; O.Gain = 1.1f; O.Rate = 0.62f; O.Lp = 900; O.When = W;
			if (A) A->Sfx(TEXT("thudSoft"), O);
		}
		Pending = FPending{ Id, 3.5 };
	}
	else if (Id == TEXT("radio"))
	{
		if (D.SetRadio) D.SetRadio(TEXT("far"));
		// written down once the player is by the truck and has heard it
		Pending = FPending{ Id, 1, [this] { return RadioNear < 7; } };
	}
	else if (Id == TEXT("bulletin"))
	{
		const double Len = D.Bulletin ? D.Bulletin() : 0;
		if (Len <= 0) return false;
		FPending P{ Id, Len };
		P.Heard = true;
		if (D.BulletinProgress) { if (const TOptional<FS47ArchiveProgress> G = D.BulletinProgress()) P.ArchiveId = G->Id; }
		Pending = P;
	}
	else if (Id == TEXT("phone"))
	{
		if (S.Saro && A)
		{
			FS47Sound O; O.Gain = 1.0f; O.At = S.Saro->Phone;
			const FS47Play H = A->Sfx(TEXT("phoneRing"), O);
			if (H && A->Ctx.IsValid())
			{
				const double T = A->Ctx->CurrentTime();
				H.Gain->Gain.SetValueAtTime(1, T + 1.7);
				H.Gain->Gain.LinearRampToValueAtTime(0, T + 1.85);
				H.Src->Stop(T + 1.9);
			}
		}
		Pending = FPending{ Id, 2.5 };
	}
	else if (Id == TEXT("door"))
	{
		if (D.OpenDoor) D.OpenDoor();
		DoorAt = S.Saro ? S.Saro->Door : DoorAt;
		Pending = FPending{ Id, 1, [this] { return bDoorSeen; } };
	}
	else if (Id == TEXT("silence"))
	{
		if (D.Silence) D.Silence();
		Pending = FPending{ Id, 0 };
	}
	else return false;
	Last = FLast{ Id, S.Clock };
	return true;
}

void FS47NightEvents::StepsBehind(double Dt, const FS47NightIn& S)
{
	// the door: noticed once the player is back by it and sees it open
	if (Pending && Pending->Id == TEXT("door") && S.Foot && S.Saro) bDoorSeen = S.Saro->bDoorOpen && FVector::Dist(S.Foot->Pos, DoorAt) < 10 && InView(S.Cam, DoorAt);
	if (!StepsLeft.Num()) return;
	// walking again: whatever it was stops with the player
	if (S.Foot && S.Foot->bMoving) { StepsLeft.Reset(); return; }
	for (double& L : StepsLeft) L -= Dt;
	while (StepsLeft.Num() && StepsLeft[0] <= 0)
	{
		StepsLeft.RemoveAt(0);
		if (!D.Step) continue;
		const US47Audio::FStepPick St = D.Step();
		FS47Sound O; O.Gain = St.Gain * 1.2f; O.Rate = 0.86f + FMath::FRand() * 0.06f; O.At = StepAt; O.Lp = 3200;
		if (US47Audio* A = D.Audio.Get()) A->Sfx(St.Name, O);
	}
}

void FS47NightEvents::Finish(const FString& Id, const FS47NightIn& S)
{
	const FString T = S47Text::ClockText(S.Clock, false), Place = D.Place ? D.Place() : FString();
	const bool bFirst = Count(Id) == 0;
	Done(Id, S.Clock);
	auto Note = [this](const FString& Text) { if (D.Note) D.Note(Text); };
	if (Id == TEXT("steps"))
	{
		Note(bFirst ? FString::Printf(TEXT("%s. %s. I stopped, and the footsteps behind me went on two or three more and stopped. Nobody there."), *T, *Place)
			: FString::Printf(TEXT("%s. %s. The steps behind me again when I stopped."), *T, *Place));
	}
	else if (Id == TEXT("knocks")) Note(FString::Printf(TEXT("%s. Driving. Three knocks on the roof of the cab, slow, from above. Nothing on the road behind."), *T));
	else if (Id == TEXT("radio")) Note(FString::Printf(TEXT("%s. The truck's radio was on when I came back to it, on 1090. I had left it off."), *T));
	else if (Id == TEXT("bulletin")) Note(FString::Printf(TEXT("%s. AM 1420, a news bulletin: a flying disc found on a ranch near Roswell, the eighth of July. 1947. Then the Night Line again, as if nothing."), *T));
	else if (Id == TEXT("phone")) Note(FString::Printf(TEXT("%s. %s. The phone in the control room rang once, and stopped. Nobody else is in the building."), *T, *Place));
	else if (Id == TEXT("door")) Note(FString::Printf(TEXT("%s. The service door stood open. I had shut it."), *T));
	// (silence: core/roadside.ts writes it down when the 47 s are over)
}

void FS47NightEvents::Reset()
{
	Wait = Rnd(90, 160); Pending.Reset(); StepsLeft.Reset(); Walked = 0; Still = 0; bDoorSeen = false; RadioNear = 1e9;
}

// ---------- self test ----------

bool FS47NightEvents::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s night events: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	FS47TownSeen Seen;
	TArray<FString> Notes;
	FS47NightDeps Deps;
	Deps.Seen = &Seen;
	Deps.Note = [&Notes](const FString& T) { Notes.Add(T); };
	Deps.Place = [] { return FString(TEXT("Service yard")); };
	Deps.Step = [] { US47Audio::FStepPick P; P.Name = TEXT("stepConcrete0"); P.Gain = 0.5f; return P; };
	FS47NightEvents E(Deps);
	FS47NightIn S;
	S.bOk = true; S.Clock = 3 * 3600 + 5 * 60;
	S.Cam.Pos = FVector(5, 1.02, -8);
	// walking 9 s, then standing a moment: the steps may come
	S.Foot = FS47NightIn::FFoot{ FVector(5, -0.6, -8), true, true };
	for (int32 I = 0; I < 90; ++I) E.Update(0.1, S);
	S.Foot->bMoving = false;
	E.Update(0.3, S);
	Check(E.Can(TEXT("steps"), S) && !E.Can(TEXT("knocks"), S) && !E.Can(TEXT("silence"), S), TEXT("standing after a walk outside: the steps behind, nothing else"));
	Check(E.Now(TEXT("steps"), S) && E.Running() && FMath::IsNearlyEqual(E.StepsAt().Y, 1.02 - 1.5) && FVector::Dist2D(FVector(E.StepsAt().X, E.StepsAt().Z, 0), FVector(5, -8, 0)) >= 3.5 - 1e-9
		&& E.StepsAt().Z > -8, TEXT("the steps behind the player, 3.5 to 5.5 m back, 1.5 m under the eye"));
	for (int32 I = 0; I < 60; ++I) E.Update(0.1, S);
	Check(!E.Busy() && Seen.Contains(TEXT("ne:steps1")) && Notes.Num() == 1 && Notes[0] == TEXT("03:05. Service yard. I stopped, and the footsteps behind me went on two or three more and stopped. Nobody there."),
		TEXT("written down: ne:steps1 and the note"));
	const bool bAgain = E.Now(TEXT("steps"), S);
	E.Update(6, S);
	Check(bAgain && Notes.Last() == TEXT("03:05. Service yard. The steps behind me again when I stopped.") && Seen.Contains(TEXT("ne:steps2")), TEXT("the second time, again"));
	// driving: the knocks, once
	S.Foot.Reset(); S.bDriving = true;
	Check(E.Now(TEXT("knocks"), S) && E.Running(), TEXT("the knocks on the cab roof"));
	E.Update(3.6, S);
	Check(Seen.Contains(TEXT("ne:knocks1")) && !E.Can(TEXT("knocks"), S) && Notes.Last().StartsWith(TEXT("03:05. Driving. Three knocks")), TEXT("the knocks once a night"));
	// the door: left open while the player is away, found by them
	S.bDriving = false;
	S.Foot = FS47NightIn::FFoot{ FVector(-20, -0.6, 30), true, false };
	S.Cam.Pos = FVector(-20, 1.02, 30); S.Cam.Forward = FVector(0, 0, 1);
	S.Saro = FS47NightIn::FSaro{ FVector(0, 1, 0), FVector(6, 1, -1), false };
	Check(E.Now(TEXT("door"), S) && !E.Running() && E.Busy(), TEXT("the door: busy, not running (it waits to be come back to)"));
	S.Saro->bDoorOpen = true;
	E.Update(2, S);
	Check(E.Busy() && !Seen.Contains(TEXT("ne:door1")), TEXT("not written until seen"));
	S.Foot->Pos = FVector(6, -0.6, 6); S.Cam.Pos = FVector(6, 1.02, 6); S.Cam.Forward = FVector(0, 0, -1);
	E.Update(0.1, S); E.Update(0.1, S);
	Check(Seen.Contains(TEXT("ne:door1")) && Notes.Last() == TEXT("03:05. The service door stood open. I had shut it."), TEXT("the door found open"));
	// the curve says no: nothing starts on its own
	E.Reset();
	S.May = false; S.MayLong = false;
	for (int32 I = 0; I < 400; ++I) E.Update(1, S);
	Check(!E.Busy(), TEXT("nothing when the curve says no"));
	Check(S47Bulletin().Num() == 6 && S47Bulletin()[1].Text == TEXT("Roswell, New Mexico. Tuesday, July eighth, nineteen forty-seven."), TEXT("the bulletin's lines"));
	return bOk;
}

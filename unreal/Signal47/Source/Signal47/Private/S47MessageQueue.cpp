#include "S47MessageQueue.h"
#include "S47GameStyle.h"

// MSG-ONE-20261109: see the header.

FString FS47MessageQueue::Speaker(const FString& Text)
{
	FString Who, Line;
	S47Game::SplitCaption(Text, Who, Line);
	if (Who.IsEmpty()) return FString();
	// the heads somebody speaks under; a sign or a station's name before a colon ("CALICHE GAS STATION: ", "PHOTO LAB: ") is not one
	static const TCHAR* const Heads[] = { TEXT("WALKIE"), TEXT("DALE"), TEXT("CB"), TEXT("RADIO"), TEXT("KOB"), TEXT("1420"), TEXT("WARD"), TEXT("DR. WARD"),
		TEXT("NORA"), TEXT("N. VEGA"), TEXT("THE LINE"), TEXT("YOU"), TEXT("TOMÁS"), TEXT("TOMAS"), TEXT("EARL") };
	const FString Up = Who.ToUpper();
	for (const TCHAR* H : Heads)
	{
		const int32 N = FCString::Strlen(H);
		if (Up.StartsWith(H) && (Up.Len() == N || !FChar::IsAlnum(Up[N]))) return Who;
	}
	// anyone else whose words are quoted ("THE WAITRESS: \"...\"")
	const FString Said = Line.TrimStart();
	if (Said.StartsWith(TEXT("\"")) || Said.StartsWith(TEXT("“"))) return Who;
	return FString();
}

FS47MessageQueue::EPri FS47MessageQueue::Classify(const FString& Text, bool bSound)
{
	if (Text.StartsWith(TEXT("COUNTER / "))) return EPri::Danger;
	const FString Who = Speaker(Text).ToUpper();
	if (!Who.IsEmpty())
	{
		// "WALKIE 1, DALE", "DALE": his; any other speaker is the CB's, the radio's or the telephone's
		TArray<FString> Words;
		Who.Replace(TEXT(","), TEXT(" ")).ParseIntoArrayWS(Words);
		return Words.Contains(TEXT("DALE")) ? EPri::Dale : EPri::Radio;
	}
	return bSound ? EPri::Radio : EPri::Hint;
}

int32 FS47MessageQueue::Push(const FString& Text, float Secs, EPri Pri, TFunction<double()> Clock, TFunction<bool()> Valid)
{
	if (Text.TrimStartAndEnd().IsEmpty()) return 0;
	const ELane L = LaneOf(Pri);
	// the same line twice is noise: on screen already, it stays as it is; waiting, the two are one (the higher priority, the longer
	// time, the newer clock and test)
	for (const TOptional<FItem>& C : Current) if (C.IsSet() && C->Text == Text) { ++Merged; return C->Id; }
	for (FItem& W : Queue)
	{
		if (W.Text != Text) continue;
		++Merged;
		W.Pri = FMath::Min(W.Pri, Pri);
		W.Secs = FMath::Max(W.Secs, Secs);
		if (Clock) { W.Clock = MoveTemp(Clock); W.SoundEnd = W.Clock() + Secs; }
		if (Valid) W.Valid = MoveTemp(Valid);
		return W.Id;
	}
	FItem I;
	I.Id = NextId++;
	I.Text = Text;
	I.Secs = FMath::Max(.1f, Secs);
	I.Pri = Pri;
	I.Clock = MoveTemp(Clock);
	I.Valid = MoveTemp(Valid);
	I.Arrived = T;
	I.SoundEnd = I.Clock ? I.Clock() + I.Secs : -1.;
	// The same speaker going on (the CB call's next line, the radio host's): the new line takes the old one's place now, if the old
	// one has been read a moment; one speaker, one caption, in step with the voice.
	if (L == ELane::Speech && Current[0].IsSet() && Current[0]->Pri == Pri && T - Current[0]->ShownAt >= MinShow)
	{
		const FString Who = Speaker(Text);
		if (!Who.IsEmpty() && Who == Speaker(Current[0]->Text)) { Current[0].Reset(); FreeAt[0] = T; bChanged = true; }
	}
	// Danger does not wait for a caption of less weight: that one steps back into the queue with what it has left, and comes after.
	if (L == ELane::Speech && Pri == EPri::Danger && Current[0].IsSet() && Current[0]->Pri != EPri::Danger)
	{
		FItem Back = Current[0].GetValue();
		Current[0].Reset();
		if (!Back.Clock) Back.Secs = float(FMath::Max(0., Back.Until - T));
		if (Back.Clock || Back.Secs >= MinShow) Queue.Add(MoveTemp(Back));
		FreeAt[0] = T;
		bChanged = true;
	}
	Queue.Add(MoveTemp(I));
	return NextId - 1;
}

bool FS47MessageQueue::Over(const FItem& I) const
{
	return (I.Clock ? I.Clock() : T) >= I.Until;
}

bool FS47MessageQueue::Stale(const FItem& I) const
{
	if (I.Age > MaxWait) return true;
	if (I.Valid && !I.Valid()) return true;
	// timed by its sound, and the sound is (all but) over: the words have gone by
	if (I.Clock && I.SoundEnd >= 0 && I.Clock() > I.SoundEnd - MinShow * .5) return true;
	return false;
}

int32 FS47MessageQueue::Waiting(ELane L) const
{
	int32 N = 0;
	for (const FItem& I : Queue) N += I.Lane() == L;
	return N;
}

void FS47MessageQueue::Clear()
{
	Queue.Empty();
	for (TOptional<FItem>& C : Current) C.Reset();
	FreeAt[0] = FreeAt[1] = T;
	bChanged = true;
}

bool FS47MessageQueue::Step(double Now, double PlayDt)
{
	T = Now;
	bool bDid = bChanged;
	bChanged = false;
	for (FItem& I : Queue) I.Age += PlayDt;
	const int32 Before = Queue.Num();
	Queue.RemoveAll([this](const FItem& I) { return Stale(I); });
	Dropped += Before - Queue.Num();
	for (int32 L = 0; L < 2; ++L)
	{
		TOptional<FItem>& C = Current[L];
		if (C.IsSet() && (Over(C.GetValue()) || (C->Valid && !C->Valid())))
		{
			C.Reset();
			FreeAt[L] = T + Gap;
			bDid = true;
		}
		if (C.IsSet() || T < FreeAt[L]) continue;
		// the next: the most pressing, and among equals the first to come
		int32 Best = INDEX_NONE;
		for (int32 K = 0; K < Queue.Num(); ++K)
		{
			const FItem& I = Queue[K];
			if (int32(I.Lane()) != L) continue;
			if (L == int32(ELane::Speech) && Hold && Hold(I)) continue;
			if (Best == INDEX_NONE || I.Pri < Queue[Best].Pri || (I.Pri == Queue[Best].Pri && I.Arrived < Queue[Best].Arrived)) Best = K;
		}
		if (Best == INDEX_NONE) continue;
		FItem I = MoveTemp(Queue[Best]);
		Queue.RemoveAt(Best);
		// a line timed by its sound runs out with it; any other has its whole time from now
		I.Until = I.Clock ? FMath::Max(I.SoundEnd, I.Clock() + MinShow) : T + I.Secs;
		I.ShownAt = T;
		C = MoveTemp(I);
		bDid = true;
	}
	return bDid;
}

bool FS47MessageQueue::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bPass, const TCHAR* Name) { Lines.Add(FString::Printf(TEXT("%s message queue: %s"), bPass ? TEXT("ok  ") : TEXT("FAIL"), Name)); bOk &= bPass; };
	auto Shows = [](const FS47MessageQueue& Q, ELane L) { const FItem* I = Q.Showing(L); return I ? I->Text : FString(); };
	using E = EPri;
	Check(Classify(TEXT("WALKIE 1, DALE: Come in.")) == E::Dale && Classify(TEXT("DALE: You there?")) == E::Dale && Classify(TEXT("CB: Breaker one-nine.")) == E::Radio
		&& Classify(TEXT("RADIO, a boy: \"It had a tail.\"")) == E::Radio && Classify(TEXT("N. VEGA: \"Sierra Motor Court.\"")) == E::Radio && Classify(TEXT("YOU: SARO, come in.")) == E::Radio
		&& Classify(TEXT("COUNTER / Pegged.")) == E::Danger && Classify(TEXT("CALICHE GAS STATION: south past the motel.")) == E::Hint && Classify(TEXT("PHOTO LAB: The print is dry.")) == E::Hint
		&& Classify(TEXT("Locked, with a night window and a bell.")) == E::Hint && Classify(TEXT("Something faint."), true) == E::Radio,
		TEXT("Dale, the CB, the radio and the telephone are speech; a sign's head or a plain line is a hint; the Geiger counter is danger"));
	{
		// two at once: one, then the other, never both
		FS47MessageQueue Q;
		Q.Push(TEXT("WALKIE 1, DALE: Where are you?"), 4.f, E::Dale);
		Q.Push(TEXT("CB: Breaker one-nine, anybody out by the mesa?"), 3.f, E::Radio);
		Q.Step(0, 0);
		const bool bFirst = Shows(Q, ELane::Speech).StartsWith(TEXT("WALKIE")) && Q.Waiting(ELane::Speech) == 1;
		Q.Step(2, 2);
		const bool bStill = Shows(Q, ELane::Speech).StartsWith(TEXT("WALKIE"));
		Q.Step(4.1, 2.1);
		const bool bGap = Shows(Q, ELane::Speech).IsEmpty();
		Q.Step(4.5, .4);
		const bool bSecond = Shows(Q, ELane::Speech).StartsWith(TEXT("CB")) && Q.Waiting() == 0;
		Q.Step(7.6, 3.1);
		Check(bFirst && bStill && bGap && bSecond && Shows(Q, ELane::Speech).IsEmpty(), TEXT("two lines at once are shown one after the other, the second when the first has had its time"));
		FS47MessageQueue C;
		C.Push(TEXT("CB: Breaker one-nine."), 3.f, E::Radio);
		C.Step(0, 0);
		C.Push(TEXT("CB: Too soon to cut."), 3.f, E::Radio);
		C.Step(1.5, 1.5);
		const bool bKept = Shows(C, ELane::Speech) == TEXT("CB: Breaker one-nine.") && C.Waiting() == 1;
		C.Push(TEXT("CB: Anybody out by the mesa?"), 3.f, E::Radio);
		C.Step(1.6, .1);
		const bool bNext = Shows(C, ELane::Speech) == TEXT("CB: Too soon to cut.") && C.Waiting() == 1;
		C.Step(5.0, 3.4);
		C.Step(5.4, .4);
		Check(bKept && bNext && Shows(C, ELane::Speech) == TEXT("CB: Anybody out by the mesa?") && C.Waiting() == 0,
			TEXT("the same speaker's next line takes the place of the one before, once that has been read a moment, never two at once"));
	}
	{
		// danger before the CB, waiting or showing
		FS47MessageQueue Q;
		Q.Push(TEXT("CB: You hear that?"), 3.f, E::Radio);
		Q.Push(TEXT("COUNTER / It's climbing."), 3.f, E::Danger);
		Q.Step(0, 0);
		const bool bWaiting = Shows(Q, ELane::Speech) == TEXT("COUNTER / It's climbing.");
		FS47MessageQueue R;
		R.Push(TEXT("CB: Long story, this one, about my cousin's dog."), 8.f, E::Radio);
		R.Step(0, 0);
		R.Step(2, 2);
		R.Push(TEXT("COUNTER / Pegged."), 3.f, E::Danger);
		R.Step(2.1, .1);
		const bool bCut = Shows(R, ELane::Speech) == TEXT("COUNTER / Pegged.") && R.Waiting(ELane::Speech) == 1;
		R.Step(5.2, 3.1);
		R.Step(5.6, .4);
		const FItem* Back = R.Showing(ELane::Speech);
		Check(bWaiting && bCut && Back && Back->Text.StartsWith(TEXT("CB: Long")) && FMath::IsNearlyEqual(Back->Until - 5.6, 6., .01),
			TEXT("danger goes before the CB, and the CB line it cut comes back after with the time it had left"));
	}
	{
		// out of date: waited too long, no longer holds, or its sound is over
		FS47MessageQueue Q;
		bool bInTruck = true;
		double Sound = 0;
		Q.Push(TEXT("WALKIE 1, DALE: A long one."), 100.f, E::Dale);
		Q.Push(TEXT("CB: Only in the cab."), 3.f, E::Radio, {}, [&bInTruck] { return bInTruck; });
		Q.Push(TEXT("RADIO: \"Night Line.\""), 4.f, E::Radio, [&Sound] { return Sound; });
		Q.Push(TEXT("Something written on the wall."), 3.f, E::Hint);
		Q.Step(0, 0);
		Q.Step(1, 1);
		bInTruck = false; Sound = 5;
		Q.Step(2, 1);
		const bool bGoneTwo = Q.Waiting(ELane::Speech) == 0 && Q.Dropped == 2;
		Q.Push(TEXT("CB: Later."), 3.f, E::Radio);
		Q.Step(40, 38);
		const bool bWaits = Q.Waiting(ELane::Speech) == 1;
		Q.Step(70, 30);
		Check(bGoneTwo && bWaits && Q.Waiting(ELane::Speech) == 0 && Q.Dropped == 3 && Shows(Q, ELane::Speech).StartsWith(TEXT("WALKIE")),
			TEXT("a line is let go when it no longer holds (the CB out of the truck), when its sound is over, or after a minute of waiting"));
		FS47MessageQueue P;
		P.Push(TEXT("WALKIE 1, DALE: A long one."), 100.f, E::Dale);
		P.Push(TEXT("CB: Waits through the pause."), 3.f, E::Radio);
		P.Step(0, 0);
		P.Step(90, 0);
		Check(P.Waiting(ELane::Speech) == 1, TEXT("a line does not grow old while the game is paused or a panel is open"));
	}
	{
		// duplicates, the corner lane, and Dale's voice holding the others
		FS47MessageQueue Q;
		Q.Push(TEXT("CB: Same again."), 3.f, E::Radio);
		Q.Push(TEXT("CB: Same again."), 5.f, E::Radio);
		Q.Push(TEXT("The door is locked."), 3.f, E::Hint);
		Q.Push(TEXT("The door is locked."), 3.f, E::Hint);
		Q.Step(0, 0);
		Q.Push(TEXT("CB: Same again."), 3.f, E::Radio);
		const FItem* Cb = Q.Showing(ELane::Speech);
		Check(Q.Waiting() == 0 && Q.Merged == 3 && Cb && FMath::IsNearlyEqual(Cb->Until, 5.) && Shows(Q, ELane::Hint) == TEXT("The door is locked."),
			TEXT("the same line twice is one; a plain line has its own lane beside the caption"));
		FS47MessageQueue H;
		bool bDale = true;
		H.Hold = [&bDale](const FItem& I) { return bDale && I.Pri > EPri::Dale; };
		H.Push(TEXT("CB: Wait for him."), 3.f, E::Radio);
		H.Step(0, 0);
		const bool bHeld = Shows(H, ELane::Speech).IsEmpty() && H.Waiting() == 1;
		bDale = false;
		H.Step(.5, .5);
		Check(bHeld && Shows(H, ELane::Speech).StartsWith(TEXT("CB")), TEXT("while Dale's voice is on the set nobody else's caption starts"));
	}
	bOk &= S47TruckHint::SelfTest(Lines);
	return bOk;
}

bool S47TruckHint::Due(FState& S, const TArray<FString>& Now)
{
	bool bSay = !S.bIntro;
	S.bIntro = true;
	for (const FString& Each : Now) bSay |= !S.Shown.Contains(Each);
	S.Shown.Empty();
	S.Shown.Append(Now);
	return bSay;
}

bool S47TruckHint::SelfTest(TArray<FString>& Lines)
{
	FState S;
	const TArray<FString> None, Tank = { TEXT("tank") }, TankAndParcel = { TEXT("tank"), TEXT("parcel") };
	const bool bFirst = Due(S, None), bAgain = Due(S, None);
	const bool bTank = Due(S, Tank), bTankAgain = Due(S, Tank), bParcel = Due(S, TankAndParcel), bLess = Due(S, Tank);
	const bool bFilled = Due(S, None), bEmptyAgain = Due(S, Tank);
	const bool bOk = bFirst && !bAgain && bTank && !bTankAgain && bParcel && !bLess && !bFilled && bEmptyAgain;
	Lines.Add(FString::Printf(TEXT("%s truck hint: once the first time in, then once for each new situation (the tank empty, a parcel aboard), again only when it comes back"), bOk ? TEXT("ok  ") : TEXT("FAIL")));
	return bOk;
}

// story/stationTaken.ts: at STATION 01 everyone goes
#include "S47StationTaken.h"

#include "S47Audio.h"
#include "S47Docs.h"
#include "S47Main.h"
#include "S47Station.h"
#include "S47Story.h"
#include "S47UI.h"

void US47StationTaken::Init(US47Main* InM, TFunction<US47Station*()> InSite)
{
	M = InM;
	Site = MoveTemp(InSite);
}

// ---------- deps ----------

void US47StationTaken::Toast(const FString& Text, float Secs)
{
	if (M && M->UI) M->UI->Toast(Text, Secs);
	else TestSaid.Add(Text);
}

void US47StationTaken::Note(const FString& Text)
{
	if (M && M->Story) M->Story->Note(Text);
	else TestSaid.Add(Text);
}

void US47StationTaken::Play(const FString& Name, float Gain, float Rate, float When, const TOptional<FVector>& At)
{
	if (!M || !M->Audio || Name.IsEmpty()) return;
	FS47Sound O;
	O.Gain = Gain; O.Rate = Rate; O.When = When; O.At = At;
	M->Audio->Sfx(Name, O);
}

// ---------- the warning at the station ----------

void US47StationTaken::Begin(double Clock)
{
	State = TEXT("shut"); T = 0; DoorK = 0; bLit = true; bLamps = true; bKeysGone = true;
	TriedTruck = 0; TriedDoor = 0; From = Clock; WasStage = TEXT("warning"); Last.Reset();
}

void US47StationTaken::Update(double Dt, const FIn& S)
{
	const FString Was = WasStage;
	WasStage = S.Stage;
	// up again: the keys by the hand, and it written down
	if (bPending && S.Stage == TEXT("calm") && Was == TEXT("back"))
	{
		bPending = false;
		Toast(TEXT("The truck's keys are lying in the frost by your hand."), 4);
		Note(FString::Printf(TEXT("%s. STATION 01, on the way out from the hut after N. Vega's call. The hut door swung shut behind me by itself, and the light inside went out. The keys were not in the truck. There was nowhere to go. When I came to, the keys were in the frost beside me, and the door stood open again with the light on."), *S47Text::ClockText(From, false)));
	}
	if (State != TEXT("shut")) return;
	US47Station* St = Site ? Site() : nullptr;
	// the warning passed after all (Missing time turned off on the way, the night left): put back
	if (S.Stage == TEXT("calm") && Was == TEXT("warning")) { PutBack(); Last = TEXT("passed"); return; }
	// taken: while it is black, the door open again and the lights on (the keys will be by the hand)
	if (S.Stage == TEXT("black") || S.Stage == TEXT("back")) { PutBack(); Last = TEXT("taken"); bPending = true; return; }
	if (S.Stage != TEXT("warning") && S.Stage != TEXT("light")) return;
	T += Dt;
	if (!St) return;
	// the door swings shut behind the player, latches; the light inside goes out
	if (T >= DOOR_AT && DoorK < 1)
	{
		if (DoorK == 0)
		{
			const TOptional<FVector> At = DoorAt(St);
			US47Audio* A = M ? M->Audio.Get() : nullptr;
			Play(FMath::FRand() < 0.5f ? TEXT("doorCreak0") : TEXT("doorCreak1"), 0.2f, 0.82f, 0.f, At);
			Play(A ? A->Or(TEXT("door_light"), TEXT("doorClose")) : FString(), 0.55f, 1.f, float(DOOR_T * 0.82), At);
			Play(A ? A->Or(TEXT("door_latch"), TEXT("switch")) : FString(), 0.4f, 0.9f, float(DOOR_T * 0.82 + 0.12), At);
		}
		DoorK = FMath::Min(1.0, DoorK + Dt / DOOR_T);
		St->SetHutDoor(DoorK);
		if (DoorK >= 1) Toast(TEXT("Behind you, the hut door swings shut by itself, and latches."), 3.2f);
	}
	if (bLit && T >= DOOR_AT + DOOR_T + LIGHT_AT - 1.15)
	{
		bLit = false;
		St->SetHutLight(false);
		Play(TEXT("switch"), 0.18f, 0.7f, 0.f, DoorAt(St));
	}
	// :40, with the flashlight: the lamps outside, all of them
	if (bLamps && S.WarnT.Get(0) >= BEAT_DARK) { bLamps = false; St->SetOutLamps(false); }
}

void US47StationTaken::TryTruck()
{
	++TriedTruck;
	Play(TEXT("doorCreak1"), 0.14f, 1.25f, 0.f, {});
	Toast(TriedTruck == 1 ? TEXT("The keys are not in it. They are always in it.") : TEXT("No keys. Not in the ignition, not on the seat, not on the floor."), 3.6f);
}

void US47StationTaken::TryDoor()
{
	++TriedDoor;
	US47Station* St = Site ? Site() : nullptr;
	US47Audio* A = M ? M->Audio.Get() : nullptr;
	Play(A ? A->Or(TEXT("door_latch"), TEXT("switch")) : FString(), 0.5f, 0.75f, 0.f, St ? DoorAt(St) : TOptional<FVector>());
	Toast(TriedDoor == 1 ? TEXT("The latch gives. The door does not. It has not been locked in years.") : TEXT("It does not move. Not even in the frame."), 3.4f);
}

TOptional<FVector> US47StationTaken::DoorAt(US47Station* S) const
{
	const FS47Obj P = S ? S->Proxy(TEXT("hutDoor")) : FS47Obj();
	return P ? TOptional<FVector>(P.WorldPosition()) : TOptional<FVector>();
}

void US47StationTaken::PutBack()
{
	// The station as it always is: the door open into the room, the lights on, the keys in the truck.
	if (US47Station* St = Site ? Site() : nullptr) { St->SetHutDoor(0); St->SetHutLight(true); St->SetOutLamps(true); }
	State = TEXT("idle"); bKeysGone = false; DoorK = 0; bLit = true; bLamps = true;
}

void US47StationTaken::Reset()
{
	if (State == TEXT("shut")) PutBack();
	State = TEXT("idle"); WasStage = TEXT("calm"); Last.Reset(); bPending = false;
}

// ---------- self test ----------

bool US47StationTaken::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s station taken: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	Check(FMath::IsNearlyEqual(LAST, 4 * 3600 + 16 * 60 + 10.0), FString::Printf(TEXT("the last clock it may begin at is 04:16:10 (%s)"), *S47Text::ClockText(LAST)));
	US47StationTaken* S = NewObject<US47StationTaken>(GetTransientPackage());
	S->Begin(4 * 3600 + 2 * 60);
	Check(S->State == TEXT("shut") && S->bKeysGone && !S->HutShut() && S->TruckExamine().Contains(TEXT("ignition is empty")), TEXT("begun: the keys gone, the door not shut yet"));
	// no station (a world without it): the warning's time still runs, nothing is moved
	S->Update(0.5, { TEXT("warning"), 0.5, 0 });
	Check(S->T == 0.5 && S->DoorK == 0, TEXT("without a site only the time runs"));
	S->TryTruck(); S->TryTruck(); S->TryDoor();
	Check(S->TestSaid.Num() == 3 && S->TestSaid[0].StartsWith(TEXT("The keys are not in it")) && S->TestSaid[1].StartsWith(TEXT("No keys")) && S->TestSaid[2].StartsWith(TEXT("The latch gives")), TEXT("trying the truck and the door"));
	// taken: put back while it is black, and the keys and the note once up again
	S->Update(0.1, { TEXT("black"), {}, 0 });
	Check(S->State == TEXT("idle") && !S->bKeysGone && S->Last == TEXT("taken"), TEXT("taken: put back while it is black"));
	S->Update(0.1, { TEXT("back"), {}, 0 });
	S->Update(0.1, { TEXT("calm"), {}, 0 });
	Check(S->TestSaid.Num() == 5 && S->TestSaid[3].Contains(TEXT("frost by your hand")) && S->TestSaid[4].StartsWith(TEXT("04:02. STATION 01")), TEXT("up again: the keys by the hand, and the note"));
	// passed: the warning that ends without the time away
	S->Begin(4 * 3600);
	S->Update(0.1, { TEXT("calm"), {}, 0 });
	Check(S->State == TEXT("idle") && S->Last == TEXT("passed"), TEXT("the warning passed after all"));
	S->MarkAsGarbage();
	return bOk;
}

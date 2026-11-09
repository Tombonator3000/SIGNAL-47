#include "S47MotelNight.h"
#include "S47Audio.h"
#include "S47EventPlaces.h"
#include "S47Freeform.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47NightRhythm.h"
#include "S47SaroSave.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47MotelNight, Log, All);

FS47MotelNight::EKind FS47MotelNight::KindOf(int32 Seed, int32 Night, bool bCanSteal)
{
	// About half the nights are only the highway and the ice machine.
	const float R = FS47NightRhythm::Roll(Seed, Night, 0x4d4f5445u);
	if (R < .50f) return EKind::Quiet;
	if (R < .64f) return EKind::Handle;
	if (R < .76f) return EKind::Idling;
	if (R < .86f) return EKind::Phone;
	if (R < .94f) return bCanSteal ? EKind::Theft : EKind::Quiet;
	return EKind::Party;
}

bool FS47MotelNight::CanSteal() const
{
	// What rides in the truck out on the land (FS47TruckCare takes them from there): the spare wheel and the cans of water.
	return M && M->Freeform && (M->Freeform->InventoryCount(TEXT("spare")) > 0 || M->Freeform->InventoryCount(TEXT("water")) > 0);
}

bool FS47MotelNight::SleptTheNight(double SleptFrom, double Until, double Morning)
{
	const double Midnight = FMath::FloorToDouble((Morning - 8 * 3600.) / 86400.) * 86400.;
	return SleptFrom < 0 || (SleptFrom < Midnight + 4 * 3600. && Until > Midnight + .5 * 3600.);
}

FS47MotelNight::EKind FS47MotelNight::Tonight(int32 Night) const
{
	if (ForcedKind >= 0 && ForcedKind <= int32(EKind::Party)) return EKind(ForcedKind);
	return KindOf(M && M->Freeform ? M->Freeform->Seed() : 47, Night, CanSteal());
}

double FS47MotelNight::SleepUntil(double Morning) const
{
	if (!M || !M->Freeform) return Morning;
	const int32 Night = FS47FreeformSession::NightOf(M->Freeform->Clock());
	if (!Wakes(Tonight(Night))) return Morning;
	// Between half past two and four, by the seed: the hour of the night's own.
	const double Midnight = FMath::FloorToDouble((Morning - 8 * 3600.) / 86400.) * 86400.;
	const double At = Midnight + (2.5 + 1.5 * FS47NightRhythm::Roll(M->Freeform->Seed(), Night, 0x484f5552u)) * 3600.;
	return At > M->Freeform->Clock() + 1800 && At < Morning ? At : Morning;
}

void FS47MotelNight::AfterRest(double Morning, double SleptFrom)
{
	if (!M || !M->Freeform || !M->UI) return;
	FS47FreeformSession& F = *M->Freeform;
	bUnfiled = true;
	const int32 Night = FS47FreeformSession::NightOf(F.Clock() - 3600);
	if (Night == LastNight && F.Clock() >= Morning - 1) return; // the second half of a woken night: nothing more
	// A room taken after four (in from a night out on the land) or for an hour in the day: the night was not slept here, and has none.
	const bool bNight = SleptTheNight(SleptFrom, F.Clock(), Morning);
	if (bNight) LastNight = Night;
	const EKind K = bNight ? Tonight(Night) : EKind::Quiet;
	ForcedKind = -1;
	const int32 Secs = int32(FMath::Fmod(F.Clock(), 86400.));
	const FString Time = FString::Printf(TEXT("%d:%02d"), Secs / 3600, (Secs / 60) % 60);
	const FVector Door = M->Player ? FVector(M->Player->Pos.X + 1.5, M->Player->FloorY + 1, M->Player->Pos.Z) : FVector::ZeroVector;
	FString Text, Record, Title = TEXT("SIERRA MOTOR COURT / THE NIGHT");
	switch (K)
	{
	case EKind::Handle:
		Text = FString::Printf(TEXT("%s. The door handle turns. Slowly, all the way one way and all the way back, and lets go. When you look, the walkway is empty and the lot is still. There is a smell like a struck match."), *Time);
		Record = TEXT("Sierra Motor Court: the door handle of the room tried in the night; nobody outside. A smell like a struck match.");
		if (M->Audio) M->Audio->Play(TEXT("door_latch"), .6f, Door);
		break;
	case EKind::Idling:
		Text = FString::Printf(TEXT("%s. Headlights on the curtain, stopped. An engine idling outside your door. You count twenty minutes. Then it pulls away without its lights on."), *Time);
		Record = TEXT("Sierra Motor Court: a vehicle idling outside the room for twenty minutes in the night, its lights on the door; it left dark.");
		if (M->Audio) M->Audio->Play(TEXT("engineIdle"), .5f, Door + FVector(0, 0, 6));
		break;
	case EKind::Phone:
		Text = FString::Printf(TEXT("%s. The phone on the nightstand rings once. When you pick it up there is only a carrier, and very far off something like the array's tone. The desk says no call came through."), *Time);
		Record = TEXT("Sierra Motor Court: the room phone rang once in the night; an open carrier on the line; the desk had put no call through.");
		if (M->Audio) M->Audio->Play(TEXT("phoneRing"), .55f, Door);
		break;
	case EKind::Theft:
	{
		// From the truck, what the truck carries: the spare off its bracket first, else a can of water out of the bed.
		const bool bSpare = F.InventoryCount(TEXT("spare")) > 0;
		F.Consume(bSpare ? TEXT("spare") : TEXT("water"));
		Text = bSpare ? TEXT("In the morning the truck's tailgate is down and the spare wheel is gone off its bracket. Nothing else is touched, and nobody at the desk saw a thing.")
			: TEXT("In the morning the truck's tailgate is down and a can of radiator water is gone out of the bed. Nothing else is touched, and nobody at the desk saw a thing.");
		Record = bSpare ? TEXT("Sierra Motor Court: the truck gone through in the night; the spare wheel taken off its bracket, nothing else.")
			: TEXT("Sierra Motor Court: the truck gone through in the night; a can of radiator water taken from the bed, nothing else.");
		break;
	}
	case EKind::Party:
		Text = TEXT("A party next door until three: a radio, laughing, somebody crying in the bathroom. The highway after that. You slept, some.");
		break;
	default:
		Text = bNight ? TEXT("Trucks on the highway all night, the ice machine, a television through the wall until two. You slept, mostly.")
			: FString::Printf(TEXT("%s. You sleep a few hours with the curtains drawn against the light. Trucks on the highway, a maid's cart on the walkway. Nothing else."), *Time);
		break;
	}
	TArray<TSharedRef<SWidget>> Rows;
	Rows.Add(S47Kit::Text(Text));
	if (!Record.IsEmpty())
	{
		const FString Id = FString::Printf(TEXT("motel_%d"), Night);
		if (!F.HasFinding(Id)) F.RegisterFinding(Id, Record);
		Rows.Add(S47Kit::Small(TEXT("Kept as a field record.")));
	}
	Rows.Add(S47Kit::Small(TEXT("Nothing is saved at the motel. The game saves again once you are back at SARO.")));
	if (Wakes(K) && F.Clock() < Morning - 600)
		Rows.Add(S47Kit::Button(TEXT("Back to sleep until morning"), [this, Morning]
		{
			if (!M || !M->Freeform) return;
			// The room is already paid for tonight: nothing is charged again (and nothing handed back that a requisition could swallow).
			if (M->UI) M->UI->Close(true);
			if (!M->Freeform->RestAt(Morning, FS47FreeformSession::ERest::Motel, true) && M->UI) M->UI->Toast(TEXT("You lie awake until it is light."), 4);
		}, true));
	Rows.Add(S47Kit::Button(Wakes(K) ? TEXT("Stay up") : TEXT("Check out"), [this] { if (M && M->UI) M->UI->Close(); }));
	M->UI->Open(S47Kit::Shell(M->UI, Title, S47Kit::Col(Rows), 780));
	UE_LOG(LogS47MotelNight, Display, TEXT("S47: motel night %d: kind %d at %s"), Night, int32(K), *Time);
}

void FS47MotelNight::Step(double Dt)
{
	if (!bUnfiled || !M || !M->Web || !M->Player) return;
	StepIn -= Dt;
	if (StepIn > 0) return;
	StepIn = 1;
	// Back on SARO's ground (or in the building): the motel night is behind, the next departure files as ever.
	if (S47SaroSave::InStation(M) || (M->Web->Area() == TEXT("saro") && S47SaroSave::OnStationGround(M->Player->Pos.X, M->Player->Pos.Z)))
	{
		bUnfiled = false;
		UE_LOG(LogS47MotelNight, Display, TEXT("S47: motel night behind: back at SARO"));
	}
}

void FS47MotelNight::Begin(const S47J::FObj& Saved)
{
	// A Continue comes from a save, and no save is written during a motel night: it always starts filed.
	bUnfiled = false; ForcedKind = -1; StepIn = 0;
	LastNight = Saved.IsValid() ? FMath::Clamp(S47J::Int(Saved, TEXT("lastNight")), 0, 1000000) : 0;
}

S47J::FObj FS47MotelNight::Save() const
{
	auto O = S47J::New();
	if (LastNight) S47J::Set(O, TEXT("lastNight"), LastNight);
	return O;
}

bool FS47MotelNight::SelfTest(TArray<FString>& Lines)
{
	bool bAll = true;
	const auto Check = [&](bool bOk, const FString& What) { bAll &= bOk; Lines.Add(FString(bOk ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("motel night: ") + What); };
	int32 Counts[6] = {}; bool bNoTheft = true, bSame = true;
	for (int32 N = 1; N <= 400; ++N)
	{
		const EKind K = KindOf(47, N, true); ++Counts[int32(K)];
		bNoTheft &= KindOf(47, N, false) != EKind::Theft;
		bSame &= KindOf(47, N, true) == K;
	}
	Check(bSame && Counts[0] > 150 && Counts[0] < 260 && Counts[1] > 20 && Counts[3] > 15 && Counts[4] > 10 && Counts[5] > 5 && bNoTheft,
		FString::Printf(TEXT("of 400 nights %d quiet, %d the handle, %d a car idling, %d the phone, %d a theft, %d a party; nothing stolen from an empty truck"), Counts[0], Counts[1], Counts[2], Counts[3], Counts[4], Counts[5]));
	Check(Wakes(EKind::Handle) && Wakes(EKind::Phone) && !Wakes(EKind::Quiet) && !Wakes(EKind::Theft), TEXT("the handle, the car and the phone wake you at their hour; a theft is found in the morning"));
	{
		// Night 5 runs to 08:00 on calendar day 5: a room from 22:00 has the night; one from 05:00 or for an hour at 07:00 has none.
		const double Morning = 5 * 86400. + 8 * 3600.;
		Check(SleptTheNight(4 * 86400. + 22 * 3600., Morning, Morning) && SleptTheNight(5 * 86400. + 1 * 3600., Morning, Morning) && !SleptTheNight(5 * 86400. + 5 * 3600., Morning, Morning) && !SleptTheNight(5 * 86400. + 7 * 3600., Morning, Morning),
			TEXT("a nap at the motel after four in the morning has no night in it: no knock, no theft, no record"));
	}
	FS47MotelNight A(nullptr); A.LastNight = 6; FS47MotelNight B(nullptr); B.Begin(S47J::Read(S47J::Write(A.Save())));
	Check(B.LastNight == 6 && !B.Unfiled(), TEXT("a Continue starts with the motel night filed: no save was written during one"));
	return bAll;
}

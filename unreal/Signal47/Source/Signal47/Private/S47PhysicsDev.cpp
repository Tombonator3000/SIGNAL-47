#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47Fatigue.h"
#include "S47Freeform.h"
#include "S47Glimpses.h"
#include "S47Handling.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Noise.h"
#include "S47RoswellGroup.h"
#include "S47SaroCot.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"

// F10, M5 (PHYSICS-20261108, SARO-COT): a stone off the ground, a hard throw at the station's outside wall and one at the control
// room's wall (it has to stay on this side), the crate in both arms and heaved, the carton set down on the garage bench, a small one
// drawn off by a thrown stone, and the cot: where it stands and why there, a night on it, and walking into it. Each line starts with
// the loose things back in their places. Nothing here is saved. -S47DevSweep=/tmp/dir -S47DevSweepGroup="Physics (M5)" runs the
// group in turn, a picture after each line.
void FS47DevFreeform::AddPhysics(FS47DevEvents& E)
{
	static const TCHAR* const Group = TEXT("Physics (M5)");
	using EKind = FS47Handling::EKind;
	const auto Hands = [this]() -> FS47Handling* { FS47FreeformSession* F = Session(); return F ? F->Handling.Get() : nullptr; };
	const auto CotOf = [this]() -> FS47SaroCot* { FS47FreeformSession* F = Session(); return F ? F->Cot.Get() : nullptr; };
	// At SARO at this hour and place, the loose things back where they live, then Then.
	const auto At = [this, Hands](double Hour, double X, double Z, TFunction<void(FS47Handling*)> Then)
	{
		FStage S; S.From = Hour; S.To = Hour + 1.5; S.Area = TEXT("saro"); S.Where = [X, Z] { return FVector2D(X, Z); };
		Go(S, [this, Hands, Then]
		{
			FS47Handling* H = Hands();
			if (!H) { Tell(false, TEXT("Freeform only")); return; }
			H->Begin();
			if (M->Player) M->Player->SetCrouch(false);
			Then(H);
		});
	};
	const auto Feet = [this]() { return M->Player ? FVector(M->Player->Pos.X, M->Player->FloorY, M->Player->Pos.Z) : FVector::ZeroVector; };
	// A stone in the hand, there and then.
	const auto StoneInHand = [this, Feet](FS47Handling* H)
	{
		const int32 I = H->Spawn(EKind::Stone, Feet() + FVector(.2, 0, .2), 0, M->Web->Area(), true);
		return I >= 0 && H->PickUp(I);
	};
	// When nothing is in the air any more.
	const auto Landed = [](FS47Handling* H) { if (!H) return true; for (int32 I = 0; I < H->Num(); ++I) if (H->View(I).bFlying) return false; return true; };
	const auto Find = [](FS47Handling* H, EKind Kind) { for (int32 I = 0; I < H->Num(); ++I) if (H->View(I).Kind == Kind && H->View(I).bResting && H->View(I).Area == TEXT("saro")) return I; return -1; };

	E.AddFreeform(Group, TEXT("Pick a stone off the ground"), TEXT("10:00 in the yard by the fire exit's ramp, looking at the dirt a step ahead: a stone comes up into the right hand"), [this, At]
	{
		At(10, -10.6, 9.6, [this](FS47Handling* H)
		{
			AimAt(FVector(M->Player->Pos.X - 1.4, M->Player->FloorY, M->Player->Pos.Z + .3));
			After(.4, [this, H]
			{
				const bool bGot = H->PickStone();
				Tell(bGot, bGot ? FString::Printf(TEXT("A stone in the hand (%s). Turn: it hangs behind the view a moment. Tap the throw key to set it down, hold it to throw."), *H->HeldName())
				                : FString(TEXT("no stone: the view is not on bare outdoor ground within reach")), 8);
			});
		});
	});
	E.AddFreeform(Group, TEXT("Throw a stone hard at the station wall"), TEXT("10:00 in the yard, three metres from the records room's outside wall: a full throw; it must bounce back on this side and be heard"), [this, At, StoneInHand, Landed, Hands]
	{
		At(10, -9.4, 8.7, [this, StoneInHand, Landed, Hands](FS47Handling* H)
		{
			AimAt(FVector(-6.2, M->Player->FloorY + 1.5, 8.7));
			if (!StoneInHand(H)) { Tell(false, TEXT("no stone in the hand")); return; }
			After(.5, [this, H, Landed, Hands]
			{
				const int32 Stone = H->HeldIndex();
				const int32 Before = H->ImpactCount();
				H->ThrowNow(1);
				WaitFor([Landed, Hands] { return Landed(Hands()); }, 7, [this, H, Stone, Before]
				{
					const FVector P = H->View(Stone).At;
					const bool bThisSide = P.X < -6.2;
					Tell(bThisSide && H->ImpactCount() > Before, FString::Printf(TEXT("%s; it lies at x %.2f (the wall's outside face is at -6.3): %s. Last impact: %s."),
						bThisSide ? TEXT("Stayed outside") : TEXT("WENT THROUGH"), P.X, H->ImpactCount() > Before ? TEXT("it hit") : TEXT("no impact"), *H->LastImpact()), 10);
				}, [this] { Tell(false, TEXT("still in the air after seven seconds")); });
			});
		});
	});
	E.AddFreeform(Group, TEXT("Throw a stone at the control room wall"), TEXT("10:00 in the control room, a full throw at the south wall: it must stay in the room"), [this, At, StoneInHand, Landed, Hands]
	{
		At(10, .4, .6, [this, StoneInHand, Landed, Hands](FS47Handling* H)
		{
			AimAt(FVector(.4, M->Player->FloorY + 1.3, 4.7));
			if (!StoneInHand(H)) { Tell(false, TEXT("no stone in the hand")); return; }
			After(.5, [this, H, Landed, Hands]
			{
				const int32 Stone = H->HeldIndex();
				H->ThrowNow(1);
				WaitFor([Landed, Hands] { return Landed(Hands()); }, 7, [this, H, Stone]
				{
					const FVector P = H->View(Stone).At;
					// the walls stand some half a metre outside the walk zone (room.bounds); a stone on the radio table by the south wall is in the room
					const bool bIn = M->Web->Rect(TEXT("room.bounds")).Contains(P, .55);
					Tell(bIn, FString::Printf(TEXT("%s, at %.2f %.2f %.2f after %d impacts. Last: %s."), bIn ? TEXT("Still in the control room") : TEXT("OUT OF THE ROOM"), P.X, P.Y, P.Z, H->View(Stone).Impacts, *H->LastImpact()), 10);
				}, [this] { Tell(false, TEXT("still in the air after seven seconds")); });
			});
		});
	});
	E.AddFreeform(Group, TEXT("Carry the crate"), TEXT("10:00 in the garage: the wooden crate in both arms, low; walking a step, it lags and the walk is slower"), [this, At, Find]
	{
		At(10, 36.5, 1.5, [this, Find](FS47Handling* H)
		{
			const int32 Crate = Find(H, EKind::Crate);
			if (Crate < 0) { Tell(false, TEXT("no crate in the garage")); return; }
			AimAt(H->View(Crate).At);
			After(.3, [this, H, Crate]
			{
				const double Walk = M->Player->Speed;
				if (!H->PickUp(Crate)) { Tell(false, TEXT("the crate could not be picked up from here")); return; }
				M->Player->Yaw += 1.2; // turn away with it: it comes round behind the view
				M->Player->TestMove = FVector2D(0, -1);
				After(1.0, [this, H, Walk]
				{
					M->Player->TestMove.Reset();
					Tell(H->Holding(), FString::Printf(TEXT("The crate in both arms; the walk %.2f m/s against %.2f (x%.2f); it follows at %.1f/s, a stone at %.1f/s."), M->Player->Speed, Walk, FS47Handling::WalkScale(FS47Handling::Def(EKind::Crate).Mass),
						FS47Handling::FollowRate(FS47Handling::Def(EKind::Crate).Mass), FS47Handling::FollowRate(FS47Handling::Def(EKind::Stone).Mass)), 9);
				});
			});
		});
	});
	E.AddFreeform(Group, TEXT("Heave the crate"), TEXT("the same crate given a full throw: a crate goes a step or so, lands with a thump on the concrete, and is heard"), [this, At, Find, Landed, Hands]
	{
		At(10, 36.5, 1.5, [this, Find, Landed, Hands](FS47Handling* H)
		{
			const int32 Crate = Find(H, EKind::Crate);
			if (Crate < 0) { Tell(false, TEXT("no crate in the garage")); return; }
			AimAt(H->View(Crate).At);
			After(.3, [this, H, Crate, Landed, Hands]
			{
				if (!H->PickUp(Crate)) { Tell(false, TEXT("the crate could not be picked up from here")); return; }
				M->Player->Yaw += 3.1; // into the open bay
				After(.8, [this, H, Crate, Landed, Hands]
				{
					const FVector From = H->View(Crate).At;
					H->ThrowNow(1);
					WaitFor([Landed, Hands] { return Landed(Hands()); }, 6, [this, H, Crate, From]
					{
						const double Went = FVector::Dist2D(FVector(From.X, From.Z, 0), FVector(H->View(Crate).At.X, H->View(Crate).At.Z, 0));
						const auto Heard = S47Noise::Last();
						Tell(Went < 2.2 && H->View(Crate).bResting, FString::Printf(TEXT("Heaved %.2f m. %s. Heard to %.0f m."), Went, *H->LastImpact(), Heard ? S47Noise::Reach(Heard->Loudness) : 0.0), 9);
					}, [this] { Tell(false, TEXT("still moving after six seconds")); });
				});
			});
		});
	});
	E.AddFreeform(Group, TEXT("Set the carton down"), TEXT("10:00 in the garage: the cardboard box set down upright where you look (the mechanic's bench if its top answers, else the floor in front of it), not dropped"), [this, At, Find]
	{
		At(10, 33.4, 1.7, [this, Find](FS47Handling* H)
		{
			const int32 Box = Find(H, EKind::Box);
			if (Box < 0) { Tell(false, TEXT("no carton in the garage")); return; }
			AimAt(H->View(Box).At);
			After(.3, [this, H, Box]
			{
				if (!H->PickUp(Box)) { Tell(false, TEXT("the carton could not be picked up from here")); return; }
				AimAt(FVector(32.0, M->Player->FloorY + .86, .75));
				After(1.0, [this, H, Box]
				{
					const bool bPut = H->Put();
					const FVector P = H->View(Box).At;
					const double Up = P.Y - M->Player->FloorY;
					Tell(bPut && H->View(Box).bResting, FString::Printf(TEXT("%s; its middle %.2f m over the floor (%s)."), bPut ? TEXT("Set down") : TEXT("No room"), Up, Up > .6 ? TEXT("on the bench") : TEXT("on the floor")), 8);
				});
			});
		});
	});
	E.AddFreeform(Group, TEXT("Set a stone down on the radio table"), TEXT("10:00 in the control room: a stone in the hand set down on the radio table by the south wall, steady, where the view meets it"), [this, At, StoneInHand]
	{
		At(10, .2, 2.9, [this, StoneInHand](FS47Handling* H)
		{
			if (!StoneInHand(H)) { Tell(false, TEXT("no stone in the hand")); return; }
			const int32 Stone = H->HeldIndex();
			AimAt(FVector(.15, M->Player->FloorY + .78, 4.2));
			After(.8, [this, H, Stone]
			{
				const bool bPut = H->Put();
				const double Up = H->View(Stone).At.Y - M->Player->FloorY;
				Tell(bPut && Up > .6 && H->View(Stone).bResting, FString::Printf(TEXT("%s; %.2f m over the floor (%s)."), bPut ? TEXT("Set down") : TEXT("No room"), Up, Up > .6 ? TEXT("on the table") : TEXT("on the floor")), 8);
			});
		});
	});
	E.AddFreeform(Group, TEXT("Draw a small one off with a stone"), TEXT("23:30 in the yard, low and still, lamp off: one of the small ones comes; a stone thrown off to the side, and it goes to where it landed"), [this, At, StoneInHand, Landed, Hands]
	{
		At(23.5, -12.5, 14, [this, StoneInHand, Landed, Hands](FS47Handling* H)
		{
			if (!DevRun()) { Tell(false, TEXT("this needs a developer run, where nothing is saved")); return; }
			const auto G = M->Night ? M->Night->Glimpses : TSharedPtr<FS47Glimpses>();
			if (!G || !G->HasNativeBody()) { Tell(false, TEXT("the Roswell body is not in this build")); return; }
			M->Player->SetCrouch(true);
			if (G->DevSpawn(1, FS47RoswellBrain::EMode::Curious) < 1) { Tell(false, TEXT("no small one could come here")); return; }
			// STONE-LURE-20261109: it comes up a step or three in front of the view, and one that is stood over (under three metres)
			// or watched while it sees you bolts and hides (FS47RoswellBrain), and a stone does not draw one out of that. Stood there
			// looking at it, the line failed every time. So back off to some seven metres from it first, low and looking away, as a
			// player who has seen it and keeps still: then turn only to aim, and away again as it is thrown.
			if (const auto Came = G->Individuals(); !Came.IsEmpty())
			{
				const FVector Body = Came[0].Pos;
				const FVector Back = FVector(M->Player->Pos.X - Body.X, 0, M->Player->Pos.Z - Body.Z).GetSafeNormal();
				for (const double Off : { 7.0, 6.5, 6.0, 5.5, 5.0 })
				{
					const double X = Body.X + Back.X * Off, Z = Body.Z + Back.Z * Off;
					if (Back.IsNearlyZero() || !M->Player->Walkable(X, Z)) continue;
					M->Player->Place(X, Z, FMath::Atan2(-Back.X, -Back.Z));
					M->Player->SetCrouch(true);
					break;
				}
			}
			After(1.5, [this, H, G, StoneInHand, Landed, Hands]
			{
				const auto Bodies = G->Individuals();
				if (Bodies.IsEmpty()) { Tell(false, TEXT("it went")); return; }
				const FVector Body = Bodies[0].Pos, Me = FVector(M->Player->Pos.X, M->Player->FloorY, M->Player->Pos.Z);
				// to land a few metres off to the side of it, beyond it: thrown up at about twenty degrees, as hard as that distance takes
				FVector ToBody(Body.X - Me.X, 0, Body.Z - Me.Z); ToBody = ToBody.GetSafeNormal();
				const FVector Side(-ToBody.Z, 0, ToBody.X);
				const FVector Aim = Body + Side * 3.0 + ToBody * 1.5;
				AimAt(FVector(Aim.X, Aim.Y, Aim.Z));
				const double Pitch = .244, Rise = FMath::Atan2(FMath::Sin(Pitch) + .1, FMath::Cos(Pitch));
				const double Dist = FVector::Dist2D(FVector(Me.X, Me.Z, 0), FVector(Aim.X, Aim.Z, 0)), Drop = M->Player->FloorY + M->Player->Eye - .62 * M->Player->Sneak() - .21 - Aim.Y;
				const double V = FMath::Sqrt(9.8 * Dist * Dist / FMath::Max(.1, 2 * FMath::Square(FMath::Cos(Rise)) * (Dist * FMath::Tan(Rise) + Drop)));
				const double Arm = FS47Handling::ThrowSpeed(FS47Handling::Def(EKind::Stone).Mass, 1);
				const double Charge = FMath::Clamp((V / Arm - .22) / .78, 0.0, 1.0);
				M->Player->Pitch = Pitch;
				if (!StoneInHand(H)) { Tell(false, TEXT("no stone in the hand")); return; }
				After(.4, [this, H, G, Charge, Landed, Hands]
				{
					const int32 Stone = H->HeldIndex();
					const auto AtThrow = G->Individuals();
					const FVector Was = AtThrow.IsEmpty() ? FVector::ZeroVector : AtThrow[0].Pos;
					const uint32 Since = S47Noise::Last() ? S47Noise::Last()->Serial : 0;
					H->ThrowNow(Charge);
					// and look away at once, still low: one that sees you watching it bolts (FS47RoswellBrain: watched and alarmed)
					M->Player->Yaw += UE_DOUBLE_PI;
					// where it first came down, loudest: that is where it is heard, and the stone can roll on a few metres from there
					const TSharedRef<TOptional<S47Noise::FNoise>> Loudest = MakeShared<TOptional<S47Noise::FNoise>>();
					WaitFor([Landed, Hands, Since, Loudest]
					{
						if (const auto N = S47Noise::Last(); N && N->Serial > Since && (!Loudest->IsSet() || N->Loudness > (*Loudest)->Loudness)) *Loudest = N;
						return Landed(Hands());
					}, 5, [this, H, G, Stone, Was, Loudest]
					{
						const FVector Fell = H->View(Stone).At;
						const FVector Hit = Loudest->IsSet() ? (*Loudest)->At : Fell;
						const double From = FVector::Dist2D(FVector(Was.X, Was.Z, 0), FVector(Fell.X, Fell.Z, 0));
						After(2.5, [this, G, Fell, Hit, From, H, Was]
						{
							const auto Now = G->Individuals();
							if (Now.IsEmpty()) { Tell(false, TEXT("it went before it could be seen to hear")); return; }
							const FVector Me(M->Player->Pos.X, M->Player->FloorY, M->Player->Pos.Z);
							const double To = FVector::Dist2D(FVector(Now[0].Pos.X, Now[0].Pos.Z, 0), FVector(Fell.X, Fell.Z, 0));
							const double ToMe = FVector::Dist2D(FVector(Now[0].Pos.X, Now[0].Pos.Z, 0), FVector(Me.X, Me.Z, 0));
							const bool bInReach = From <= S47Noise::Reach(H->ThrowLoudness()) + .5;
							// what it now believes: that you are where the stone came down (where it first hit, or where it lies)
							const auto Flat = [](const FVector& A, const FVector& B) { return FVector::Dist2D(FVector(A.X, A.Z, 0), FVector(B.X, B.Z, 0)); };
							const double Belief = Now[0].bMemory ? FMath::Min(Flat(Now[0].Memory, Fell), Flat(Now[0].Memory, Hit)) : -1;
							const bool bScared = Now[0].State == FS47RoswellBrain::EState::Flee || Now[0].State == FS47RoswellBrain::EState::Hide;
							const bool bDrawn = bInReach && !bScared && ((Belief >= 0 && Belief < 2.0) || To < From - 1.0);
							Tell(bDrawn, FString::Printf(TEXT("The stone fell %.1f m from where it stood, first down %.1f m from it (%s). It now takes you to be %s; it is %.1f m from the stone and %.1f m from you, %s."), From, Flat(Was, Hit), *H->LastImpact(),
								Belief < 0 ? TEXT("nowhere it knows") : *FString::Printf(TEXT("%.1f m from the stone"), Belief), To, ToMe, FS47RoswellBrain::StateName(Now[0].State)), 10);
						});
					}, [this] { Tell(false, TEXT("the stone was still in the air")); });
				});
			});
		});
	});
	// ---- the cot ----
	const auto ByCot = [this, CotOf](double Hour, TFunction<void(FS47SaroCot&)> Then)
	{
		FS47SaroCot* C = CotOf();
		if (!C) { Tell(false, TEXT("Freeform only")); return; }
		if (!C->Built()) { Tell(false, FString(TEXT("no cot: ")) + FString::Join(C->Checks(), TEXT(" / "))); return; }
		const FVector P = C->Center();
		const FVector2D Stand = C->AlongZ() ? FVector2D(P.X + 1.35, P.Z + .9) : FVector2D(P.X + .9, P.Z + 1.2);
		FStage S; S.From = Hour; S.To = Hour + 1.5; S.Area = TEXT("saro"); S.Where = [Stand] { return Stand; };
		Go(S, [this, CotOf, Then] { if (FS47SaroCot* Cot = CotOf()) { AimAt(Cot->Center() + FVector(0, .25, 0)); Then(*Cot); } });
	};
	E.AddFreeform(Group, TEXT("The cot at SARO"), TEXT("where the cot stands, and what was measured at each place it could have gone"), [this, ByCot]
	{
		ByCot(10, [this](FS47SaroCot& C) { Tell(true, FString::Printf(TEXT("The cot: %s. %s"), *C.Place(), *FString::Join(C.Checks(), TEXT(" / "))), 12); });
	});
	E.AddFreeform(Group, TEXT("Sleep on the cot until morning"), TEXT("23:00 by the cot: lie down until 08:00, the same rest as OPS; the screen goes dark and comes back"), [this, ByCot]
	{
		ByCot(23, [this](FS47SaroCot& C)
		{
			FS47FreeformSession* F = Session();
			const double Before = F->Clock();
			const bool bSlept = C.Sleep(0);
			const int32 Secs = int32(FMath::Fmod(F->Clock(), 86400.0));
			Tell(bSlept, FString::Printf(TEXT("%s %.1f hours: now %02d:%02d. %s"), bSlept ? TEXT("Slept") : TEXT("Refused after"), (F->Clock() - Before) / 3600.0, Secs / 3600, (Secs / 60) % 60, F->Fatigue ? *F->Fatigue->Describe() : TEXT("")), 9);
		});
	});
	E.AddFreeform(Group, TEXT("Walk into the cot"), TEXT("walk straight at the cot from its side: the walk stops at its edge"), [this, ByCot]
	{
		ByCot(10, [this](FS47SaroCot& C)
		{
			const FVector P = C.Center();
			const double Half = (C.AlongZ() ? FS47SaroCot::Width : FS47SaroCot::Length) / 2;
			M->Player->Place(P.X + Half + .9, P.Z, 0);
			for (int32 I = 0; I < 40; ++I) M->Player->MoveBy(-.05, 0);
			const double Edge = P.X + Half + M->Player->Radius;
			const bool bStopped = M->Player->Pos.X >= Edge - .03;
			AimAt(P);
			Tell(bStopped, FString::Printf(TEXT("%s at x %.2f; the cot's edge plus the player's width is at %.2f."), bStopped ? TEXT("Stopped") : TEXT("WALKED INTO IT"), M->Player->Pos.X, Edge), 8);
		});
	});
}

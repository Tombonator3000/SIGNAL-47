#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47Drive.h"
#include "S47Freeform.h"
#include "S47HighwayUfo.h"
#include "S47Main.h"
#include "S47OpenLand.h"
#include "S47ReliefProbe.h"
#include "S47RoadTerrain.h"
#include "S47SkyVisitors.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Reach, Log, All);

// F10, T16 (Tom's test, 7 November 2026): how far one gets from the farm, eight ways on foot and eight at the wheel, and what stops
// it (T16-01: "kommer bare litt rundt gården"); the sky's formations and its preview from the yard at night (T16-05); the highway's
// encounter going north and going south, asked for and by the night's own rule (T16-06); and the Pale One (T16-07). The sky and the
// Pale One lines press the menu's own lines, the ones Tom pressed, and then read what is drawn and whether it is on the screen. Each
// reach line leaves you standing where it stopped, looking on, so the picture shows what stops one there. Nothing here is saved.
// -S47DevSweep=/tmp/dir -S47DevSweepGroup="Reach and skies (T16)" runs the group in turn, a picture after each line.
namespace
{
	// The farm's yard, road-local (S47ReliefProbe's own start), and SARO's yard in SARO's terms.
	constexpr double FarmX = 90, FarmZ = 197;
	const FVector2D SaroYard(7, 19);

	// How many of these three's-metre points are on the screen now.
	int32 OnScreen(US47Main* M, const TArray<FVector>& Points)
	{
		UWorld* W = M && M->Web ? M->Web->GetWorld() : nullptr;
		APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
		if (!PC) return -1;
		int32 X = 0, Y = 0;
		PC->GetViewportSize(X, Y);
		int32 Seen = 0;
		for (const FVector& P : Points)
		{
			FVector2D Screen;
			if (PC->ProjectWorldLocationToScreen(S47Three::ToUE(P), Screen) && Screen.X >= 0 && Screen.Y >= 0 && Screen.X <= X && Screen.Y <= Y) ++Seen;
		}
		return Seen;
	}
}

void FS47DevFreeform::AddReach(FS47DevEvents& E)
{
	static const TCHAR* const Group = TEXT("Reach and skies (T16)");
	// The menu's own line, by its group and label, as a click on it.
	const auto Press = [this](const TCHAR* InGroup, const FString& Label) -> bool
	{
		if (!Menu) return false;
		for (const auto& Ev : Menu->Events()) if (Ev.Group == InGroup && Ev.Label == Label && Ev.Fn) { Ev.Fn(); return true; }
		return false;
	};

	// ---- T16-01: from the farm, every way, on foot and at the wheel ----
	for (const bool bDrive : { false, true }) for (int32 Dir = 0; Dir < 8; ++Dir)
	{
		E.AddFreeform(Group, FString::Printf(TEXT("Reach %s from the farm: %s"), bDrive ? TEXT("at the wheel") : TEXT("on foot"), S47ReliefProbe::ReachDir(Dir)),
			bDrive ? TEXT("11:00, straight out until the truck stops; you stand where it did") : TEXT("11:00, straight out, round what stands in the way, until the land stops you; you stand there"),
			[this, bDrive, Dir]
		{
			FStage S; S.From = 11; S.To = 15; S.Area = TEXT("saro");
			S.Where = [] { const FVector At = S47OpenLand::FromRoad(TEXT("saro"), FarmX, FarmZ); return FVector2D(At.X, At.Z); };
			Go(S, [this, bDrive, Dir]
			{
				if (!M->World || !M->Player) { Tell(false, TEXT("no player")); return; }
				if (M->World->bDriving) M->World->StopDriving();
				const S47ReliefProbe::FReach R = S47ReliefProbe::Reach(M, TEXT("farm"), TEXT("saro"), FarmX, FarmZ, Dir, bDrive, true);
				UE_LOG(LogS47Reach, Display, TEXT("S47: T16 %s %s"), R.bNatural ? TEXT("PASS") : TEXT("FAIL"), *R.Line);
				// Stand a few metres short of where it stopped, on foot in that place's area, and look on the way it was going.
				const FString Area = R.EndArea == TEXT("road") ? FString(TEXT("diner")) : R.EndArea;
				if (!Area.IsEmpty() && M->World->Area != Area) M->World->Enter(Area);
				const double Back = bDrive ? 6 : 2;
				const FVector At = S47OpenLand::FromRoad(M->World->Area, R.EndX - R.Dx * Back, R.EndZ - R.Dz * Back);
				M->Player->Place(At.X, At.Z, 0);
				const FVector Ahead = S47OpenLand::FromRoad(M->World->Area, R.EndX + R.Dx * 60, R.EndZ + R.Dz * 60);
				After(.4, [this, Ahead, R, Dir]
				{
					if (M->Player) AimAt(FVector(Ahead.X, M->Player->FloorY + M->Player->Eye + 4, Ahead.Z));
					After(1.2, [this, R, Dir] { Tell(R.bNatural, FString::Printf(TEXT("%.0f m %s, stopped by %s; %d things gone round (log: LogS47Reach)"), R.Metres, S47ReliefProbe::ReachDir(Dir), *R.Why.Left(90), R.Round), 10); });
				});
			});
		});
	}

	// ---- T16-05: the sky from SARO's yard at night, by the menu's own lines ----
	const auto SkyLine = [this, Press](const TCHAR* InGroup, const FString& Label, double Wait)
	{
		FStage S; S.From = 22; S.To = 23.5; S.Area = TEXT("saro"); S.Where = [] { return SaroYard; }; S.Yaw = UE_DOUBLE_PI; S.Pitch = .35;
		Go(S, [this, Press, InGroup, Label, Wait]
		{
			if (!Press(InGroup, Label)) { Tell(false, FString::Printf(TEXT("no line \"%s\" in %s"), *Label, InGroup)); return; }
			After(Wait, [this, Label]
			{
				const TArray<FVector> Shown = M->SkyVisitors ? M->SkyVisitors->ShownPoints() : TArray<FVector>();
				const int32 Seen = OnScreen(M, Shown);
				FString Where;
				for (int32 I = 0; I < FMath::Min(3, Shown.Num()); ++I) Where += FString::Printf(TEXT(" %.0f,%.0f,%.0f"), Shown[I].X, Shown[I].Y, Shown[I].Z);
				const FString Line = FString::Printf(TEXT("%s: %d lights drawn, %d on the screen (first at%s)"), *Label, Shown.Num(), Seen, *Where);
				UE_LOG(LogS47Reach, Display, TEXT("S47: T16 %s %s"), Seen > 0 ? TEXT("PASS") : TEXT("FAIL"), *Line);
				Tell(Seen > 0, Line, 8);
				// Gone again once the picture is taken, so the next line starts from the night's own sky.
				After(3, [this] { if (M->SkyVisitors) { M->SkyVisitors->DevStage(0); M->SkyVisitors->DevFormationStop(); } });
			});
		});
	};
	for (int32 F = 0; F < int32(ES47SkyForm::Num); ++F)
	{
		const FString Label = FString::Printf(TEXT("Formation: %s"), FS47SkyFormationState::Name(ES47SkyForm(F)));
		E.AddFreeform(Group, FString::Printf(TEXT("Sky: %s"), *Label), TEXT("22:00, SARO's yard: the menu's own line, then what is drawn and on the screen"), [SkyLine, Label] { SkyLine(TEXT("UFO formations"), Label, 3); });
	}
	for (const int32 Stage : { 1, 3, 5 })
	{
		const FString Label = FString::Printf(TEXT("Preview sky lights, stage %d"), Stage);
		E.AddFreeform(Group, FString::Printf(TEXT("Sky: %s"), *Label), TEXT("22:00, SARO's yard: the menu's own line, then what is drawn and on the screen"), [SkyLine, Label] { SkyLine(TEXT("Sky"), Label, 2.5); });
	}
	E.AddFreeform(Group, TEXT("Sky: the night's own formations, 21:00 to 03:00"), TEXT("SARO's yard; the clock runs on by the hour and the schedule says what came"), [this]
	{
		FStage S; S.From = 21; S.To = 21.2; S.Area = TEXT("saro"); S.Where = [] { return SaroYard; };
		Go(S, [this]
		{
			if (!M->SkyVisitors) { Tell(false, TEXT("no sky")); return; }
			M->SkyVisitors->DevStage(0);
			M->SkyVisitors->DevFormationStop();
			// What the night's own schedule sends, read each half hour from the state it keeps (the log has S47SkyVisitors' own lines).
			TSharedRef<int32> Came = MakeShared<int32>(0), Asked = MakeShared<int32>(0);
			const int32 PassesBefore = M->SkyVisitors->FormationData().Passes;
			TSharedRef<TFunction<void(int32)>> Next = MakeShared<TFunction<void(int32)>>();
			*Next = [this, Came, Asked, Next, PassesBefore](int32 Half)
			{
				if (Half >= 12)
				{
					const int32 Started = M->SkyVisitors ? M->SkyVisitors->FormationData().Passes - PassesBefore : 0;
					const FString Line = FString::Printf(TEXT("from 21:00 to 03:00 (clock jumped half an hour at a time; developer rules, so every scheduled pass is let through): %d passes started by the schedule, one up in %d of %d half hours; S47SkyVisitors' log lines say which and when"), Started, *Came, *Asked);
					UE_LOG(LogS47Reach, Display, TEXT("S47: T16 %s %s"), Started > 0 ? TEXT("PASS") : TEXT("FAIL"), *Line);
					Tell(Started > 0, Line, 8);
					return;
				}
				if (auto* F = Session()) F->TestAdvance(1800);
				After(1.5, [this, Came, Asked, Next, Half]
				{
					++*Asked;
					if (M->SkyVisitors && M->SkyVisitors->FormationData().bActive) ++*Came;
					(*Next)(Half + 1);
				});
			};
			(*Next)(0);
		});
	});

	// ---- T16-06: the highway, both ways ----
	for (const bool bNorth : { true, false })
	{
		E.AddFreeform(Group, FString::Printf(TEXT("Highway %s: asked for"), bNorth ? TEXT("north") : TEXT("south")), TEXT("03:12, in the truck at speed; the encounter on the way you drive"), [this, bNorth]
		{
			StartHighway(bNorth, false);
			WaitFor([this] { return M->HighwayUfo && M->HighwayUfo->Busy(); }, 20, [this, bNorth]
			{
				UE_LOG(LogS47Reach, Display, TEXT("S47: T16 PASS highway %s: started (S47HighwayUfo's line says which way)"), bNorth ? TEXT("north") : TEXT("south"));
				After(3, [this, bNorth] { Tell(true, FString::Printf(TEXT("highway encounter on, driving %s"), bNorth ? TEXT("north") : TEXT("south")), 8); });
			}, [this, bNorth]
			{
				UE_LOG(LogS47Reach, Display, TEXT("S47: T16 FAIL highway %s: not started in 20 s"), bNorth ? TEXT("north") : TEXT("south"));
				Tell(false, FString::Printf(TEXT("highway %s: not started in 20 s"), bNorth ? TEXT("north") : TEXT("south")));
			});
		});
		E.AddFreeform(Group, FString::Printf(TEXT("Highway %s: by the night's own rule"), bNorth ? TEXT("north") : TEXT("south")), TEXT("03:12, the truck held at 14 m/s; nothing forced"), [this, bNorth]
		{
			auto* F = Session();
			if (!F || !M->HighwayUfo || !M->World) { Tell(false, TEXT("Freeform only")); return; }
			ClearRunning();
			if (M->World->bDriving) M->World->StopDriving();
			ToHour(3.2, 5.5);
			M->HighwayUfo->Begin(nullptr);
			// From the town's edge north-bound, from the motel south-bound: the stretch where it may start lies just ahead.
			const double X = bNorth ? 1.8 : -1.8, Z = bNorth ? 1700. : 600.;
			FS47TripOpts O; O.AtPos = FVector(8000 + X, S47RoadTerrain::SurfaceY(X, Z), Z); O.AtHeading = bNorth ? 0. : UE_DOUBLE_PI; O.Leg = TEXT("road");
			M->World->StartTrip(TEXT("road"), TEXT("diner"), O);
			M->World->TestInput = [this] { FS47DriveIn I; auto* D = M && M->World ? M->World->CabDrive() : nullptr; if (D) I.Throttle = D->Speed < 14 ? 1 : 0; return I; };
			WaitFor([this] { return M->HighwayUfo && M->HighwayUfo->Busy(); }, 40, [this, bNorth]
			{
				if (M->World) M->World->TestInput = nullptr;
				UE_LOG(LogS47Reach, Display, TEXT("S47: T16 PASS highway %s by itself"), bNorth ? TEXT("north") : TEXT("south"));
				After(3, [this, bNorth] { Tell(true, FString::Printf(TEXT("highway encounter came by itself, driving %s"), bNorth ? TEXT("north") : TEXT("south")), 8); });
			}, [this, bNorth]
			{
				if (M->World) M->World->TestInput = nullptr;
				UE_LOG(LogS47Reach, Display, TEXT("S47: T16 FAIL highway %s by itself: not in 40 s"), bNorth ? TEXT("north") : TEXT("south"));
				Tell(false, FString::Printf(TEXT("highway %s: did not come by itself in 40 s"), bNorth ? TEXT("north") : TEXT("south")));
			});
		});
	}

	// ---- T16-07: the Pale One, by the menu's own lines, by day and by night ----
	for (const double Hour : { 11., 22. })
	{
		E.AddFreeform(Group, FString::Printf(TEXT("Pale One, here, %02.0f:00"), Hour), TEXT("SARO's yard: the menu's own line, then whether it stands drawn on the screen"), [this, Press, Hour]
		{
			FStage S; S.From = Hour; S.To = Hour + 1.5; S.Area = TEXT("saro"); S.Where = [] { return SaroYard; }; S.Yaw = UE_DOUBLE_PI;
			Go(S, [this, Press, Hour]
			{
				if (!Press(TEXT("Aliens"), TEXT("Pale One, here"))) { Tell(false, TEXT("no line \"Pale One, here\" in Aliens")); return; }
				After(1.5, [this, Hour]
				{
					const TOptional<FVector> At = Menu ? Menu->PalePresence() : TOptional<FVector>();
					const int32 Seen = At ? OnScreen(M, { *At }) : 0;
					const FString Line = At ? FString::Printf(TEXT("Pale One at %02.0f:00: drawn at %s, %s"), Hour, *At->ToCompactString(), Seen > 0 ? TEXT("on the screen") : TEXT("not on the screen"))
						: FString::Printf(TEXT("Pale One at %02.0f:00: not drawn"), Hour);
					UE_LOG(LogS47Reach, Display, TEXT("S47: T16 %s %s"), Seen > 0 ? TEXT("PASS") : TEXT("FAIL"), *Line);
					Tell(Seen > 0, Line, 8);
				});
			});
		});
	}
	E.AddFreeform(Group, TEXT("Pale One: walks up from afar"), TEXT("SARO's yard, 22:00: the menu's own line; 30 seconds"), [this, Press]
	{
		FStage S; S.From = 22; S.To = 23.5; S.Area = TEXT("saro"); S.Where = [] { return SaroYard; }; S.Yaw = UE_DOUBLE_PI;
		Go(S, [this, Press]
		{
			if (!Press(TEXT("Aliens"), TEXT("Pale One: walks up from afar"))) { Tell(false, TEXT("no line \"Pale One: walks up from afar\" in Aliens")); return; }
			After(3, [this] { Tell(true, TEXT("Pale One walking up (the picture shows whether it is seen)"), 8); });
		});
	});
}

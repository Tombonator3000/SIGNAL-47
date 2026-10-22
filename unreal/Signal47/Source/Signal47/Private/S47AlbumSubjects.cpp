// story/albumSubjects.ts
#include "S47AlbumSubjects.h"
#include "S47TownLayer.h"
#include "S47Town.h"
#include "S47Cutouts.h"
#include "S47NinthModel.h"

#include "S47Geo.h"
#include "S47Main.h"
#include "S47Truck.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"

namespace
{
	// [object key, name, size m, farthest m, weight, things that count as one]
	struct FThing { const TCHAR* Key; const TCHAR* Name; double R, Max, W; const TCHAR* Group; };
	const FThing ROOM[] = {
		{ TEXT("rack"), TEXT("the receivers"), 0.9, 11, 1, nullptr }, { TEXT("phone"), TEXT("the phone"), 0.2, 6, 1, nullptr }, { TEXT("mug"), TEXT("my coffee"), 0.12, 3, 0.8, nullptr },
		{ TEXT("coffeePot"), TEXT("the coffee pot"), 0.2, 5, 0.8, nullptr }, { TEXT("printer"), TEXT("the printer"), 0.4, 8, 0.8, nullptr }, { TEXT("clock"), TEXT("the clock"), 0.25, 10, 1, nullptr },
		{ TEXT("opsTerminal"), TEXT("the tracking terminal"), 0.4, 8, 1, nullptr }, { TEXT("halley"), TEXT("the Halley poster"), 0.6, 10, 1.1, nullptr }, { TEXT("decoder"), TEXT("the signal processor"), 0.5, 8, 1, nullptr },
		{ TEXT("map"), TEXT("the map on the wall"), 0.8, 10, 0.9, nullptr }, { TEXT("telex"), TEXT("the telex"), 0.4, 8, 0.8, nullptr }, { TEXT("radio"), TEXT("the radio"), 0.25, 6, 0.8, nullptr },
	};
	const FThing ANNEX[] = { { TEXT("vending"), TEXT("the vending machine"), 0.8, 12, 1, nullptr }, { TEXT("fieldBoxModel"), TEXT("the field box"), 0.4, 6, 1, nullptr }, { TEXT("recPhoneModel"), TEXT("the phone in the records room"), 0.2, 6, 1, nullptr } };
	const FThing YARD_THINGS[] = { { TEXT("motorBus"), TEXT("the S-03 motor cabinet"), 0.8, 25, 1, nullptr }, { TEXT("vane"), TEXT("B-12"), 0.7, 45, 1.2, nullptr }, { TEXT("wetBench"), TEXT("the wet bench"), 1.2, 8, 1, nullptr }, { TEXT("r07"), TEXT("R-07"), 0.4, 25, 1, nullptr } };
	const FThing STATION[] = {
		{ TEXT("hutDoor"), TEXT("the field hut"), 3, 260, 1.3, nullptr }, { TEXT("transit"), TEXT("the transit"), 0.6, 30, 1.1, nullptr }, { TEXT("markerA"), TEXT("fixed point A"), 0.4, 60, 1.3, nullptr },
		{ TEXT("markerB"), TEXT("fixed point B"), 0.4, 60, 1.2, nullptr }, { TEXT("stakesC"), TEXT("the stakes along C"), 3, 80, 1.2, nullptr }, { TEXT("cable"), TEXT("the cut cable"), 0.3, 12, 1.5, nullptr },
		{ TEXT("lamp"), TEXT("the field lamp"), 0.5, 60, 0.9, nullptr }, { TEXT("gate"), TEXT("the gate"), 2.4, 120, 0.9, nullptr }, { TEXT("receiver"), TEXT("the receiver"), 0.4, 6, 1.1, nullptr },
		{ TEXT("fieldPhone"), TEXT("the field telephone"), 0.3, 6, 1, nullptr }, { TEXT("diagram"), TEXT("the diagram on the wall"), 0.4, 5, 0.9, nullptr },
	};
	const FThing ROOM6[] = { { TEXT("tv"), TEXT("the television"), 0.5, 8, 1, nullptr }, { TEXT("walkie"), TEXT("the walkie-talkie"), 0.2, 4, 1.1, nullptr }, { TEXT("photo"), TEXT("the photograph by the bed"), 0.2, 4, 1, nullptr },
		{ TEXT("bed"), TEXT("the bed"), 1, 8, 0.6, nullptr }, { TEXT("phone"), TEXT("the phone"), 0.2, 5, 0.8, nullptr } };
	const FThing DINER[] = {
		{ TEXT("sign"), TEXT("the Mesa Diner sign"), 3, 900, 2.5, TEXT("diner") }, { TEXT("door"), TEXT("Mesa Diner"), 7, 500, 1.4, TEXT("diner") }, { TEXT("waitress"), TEXT("the waitress"), 0.4, 15, 2.5, nullptr },
		{ TEXT("driver"), TEXT("the driver"), 0.4, 15, 2.5, nullptr }, { TEXT("counter"), TEXT("the counter"), 2, 12, 0.7, nullptr }, { TEXT("jukebox"), TEXT("the jukebox"), 0.6, 12, 1, nullptr },
		{ TEXT("booth"), TEXT("the booth"), 0.9, 10, 0.7, nullptr }, { TEXT("clipping"), TEXT("the clipping on the wall"), 0.4, 5, 1.1, nullptr }, { TEXT("payphone"), TEXT("the payphone"), 0.3, 8, 0.9, nullptr },
		{ TEXT("coffee"), TEXT("the coffee"), 0.2, 3, 0.8, nullptr }, { TEXT("menu"), TEXT("the menu"), 0.6, 6, 0.8, nullptr }, { TEXT("rig"), TEXT("the rig on the lot"), 6, 200, 1.2, nullptr },
	};
	// world/geo.ts MOTEL_GLOWS[0]: the sign's red, in the numbers the motel was built in (before MOTEL_AT)
	const double MOTEL_SIGN[3] = { -30.4, 5.8, 30.2 };
}

namespace S47AlbumSubjects
{
	const TCHAR* CARDBOARD() { return TEXT("The cardboard alien. Who keeps moving it?"); }
	const TCHAR* JACKET() { return TEXT("Someone in a SARO jacket, standing out there. I was the only one on shift."); }
	const TCHAR* NINTH_PRINT() { return TEXT("The ninth one from the pasture, inside the fence. Not a drop of blood."); }

	TArray<FS47Subject> AlbumSubjects(const FS47SubjectDeps& D)
	{
		TArray<FS47Subject> Out;
		US47Main* M = D.M;
		AS47Web* Web = M ? M->Web.Get() : nullptr;
		if (!Web) return Out;
		/** Where a shown object is (world terms), raised by dy; unset when it or a parent is hidden. */
		auto P = [](const FS47Obj& O, double Dy = 0) -> TOptional<FVector>
		{
			if (!O || !O.Shown()) return {};
			FVector W = O.WorldPosition(); W.Y += Dy;
			return W;
		};
		auto Add = [&Out](const FString& Name, const TArray<TOptional<FVector>>& At, TFunctionRef<void(FS47Subject&)> O)
		{
			FS47Subject S;
			S.Name = Name;
			for (const TOptional<FVector>& Pt : At) if (Pt) S.At.Add(*Pt);
			if (!S.At.Num()) return;
			O(S);
			Out.Add(S);
		};
		auto Things = [&](const FString& Prefix, const FThing* List, int32 N)
		{
			for (int32 I = 0; I < N; ++I)
			{
				const FThing& T = List[I];
				Add(T.Name, { P(Web->Obj(Prefix + T.Key)) }, [&T](FS47Subject& S) { S.R = T.R; S.Max = T.Max; S.W = T.W; if (T.Group) S.Group = T.Group; });
			}
		};
		const FS47Cam Camera = FS47Cam::Of(M->Player);
		const FVector Cam = Camera.Pos;
		const bool bOpen = D.Outdoors && D.Outdoors(), bNight = D.Night && D.Night();
		US47World* World = M->World;
		const FString Area = World ? World->Area : Web->Area();

		// anywhere: the truck where it stands
		if (FS47Truck* Tr = World ? World->TruckHere() : nullptr) Add(TEXT("the truck"), { P(Tr->Group, 1.1) }, [](FS47Subject& S) { S.R = 2.6; S.Max = 140; S.W = 1.2; });

		if (Area == TEXT("saro"))
		{
			// the array, and the near antennas by their numbers (only the most prominent is named)
			TArray<TOptional<FVector>> All;
			for (int32 I = 0; I < Web->DishCount(); ++I) All.Add(FVector(Web->DishX(I), 9 * Web->DishScale(I), Web->DishZ(I)));
			Add(TEXT("the antennas"), All, [](FS47Subject& S) { S.R = 7; S.Max = 900; S.W = 1.2; S.Group = TEXT("array"); });
			for (int32 I = 0; I < Web->DishCount(); ++I)
			{
				const FVector Pt(Web->DishX(I), 9 * Web->DishScale(I), Web->DishZ(I));
				const double Sc = Web->DishScale(I);
				if (FVector::Dist(Pt, Cam) < 60) Add(Web->DishId(I), { Pt }, [Sc](FS47Subject& S) { S.R = 6 * Sc; S.Max = 60; S.W = 1.25; S.Group = TEXT("array"); });
			}
			if (bOpen)
			{
				const FS47Area B = Web->Rect(TEXT("room.bounds")), Lab = Web->Rect(TEXT("yard.zone.lab"));
				Add(TEXT("the control room"), { FVector((B.MinX + B.MaxX) / 2, 2.6, (B.MinZ + B.MaxZ) / 2) }, [](FS47Subject& S) { S.R = 6; S.Max = 260; });
				if (Lab.bValid) Add(TEXT("the photo lab"), { FVector((Lab.MinX + Lab.MaxX) / 2, 2, (Lab.MinZ + Lab.MaxZ) / 2) }, [](FS47Subject& S) { S.R = 3.5; S.Max = 120; });
			}
			Things(TEXT("yard.objs."), YARD_THINGS, UE_ARRAY_COUNT(YARD_THINGS));
			Add(TEXT("the RX consoles"), { P(Web->Obj(TEXT("room.crtLeft.group")), 0.3), P(Web->Obj(TEXT("room.crtCenter.group")), 0.3), P(Web->Obj(TEXT("room.crtRight.group")), 0.3) },
				[](FS47Subject& S) { S.R = 0.45; S.Max = 12; S.W = 1.3; });
			Things(TEXT("room.objs."), ROOM, UE_ARRAY_COUNT(ROOM));
			Things(TEXT("annex.objs."), ANNEX, UE_ARRAY_COUNT(ANNEX));
			// Sierra Motor Court down the highway: its sign from all over the site, the rooms nearer
			const S47Geo::FAt& At = S47Geo::MOTEL_AT();
			Add(TEXT("the Sierra Motor Court sign"), { FVector(MOTEL_SIGN[0] + At.X, MOTEL_SIGN[1] + At.Y, MOTEL_SIGN[2] + At.Z) }, [](FS47Subject& S) { S.R = 3.5; S.Max = 1600; S.W = 3; S.Group = TEXT("motel"); });
			Add(TEXT("Sierra Motor Court"), { FVector(-40.8 + At.X, 2.2 + At.Y, 47 + At.Z) }, [](FS47Subject& S) { S.R = 14; S.Max = 500; S.W = 1.5; S.Group = TEXT("motel"); });
		}
		else if (Area == TEXT("station01"))
		{
			Things(TEXT("world.site.proxies."), STATION, UE_ARRAY_COUNT(STATION));
			if (D.M->TownLayer && D.M->TownLayer->NinthModel->bNinthDown) Add(TEXT("the dead cow"), { P(Web->Obj(TEXT("world.site.proxies.ninthBody"))), P(Web->Obj(TEXT("world.site.proxies.ninthHead"))) }, [](FS47Subject& S) { S.R = 1.3; S.Max = 45; S.W = 3; S.Odd = NINTH_PRINT(); });
		}
		else if (Area == TEXT("room6"))
		{
			Add(TEXT("N. Vega"), { P(Web->Obj(TEXT("world.room6.objs.noraHead"))) }, [](FS47Subject& S) { S.R = 0.35; S.Max = 8; S.W = 2.5; });
			Things(TEXT("world.room6.proxies."), ROOM6, UE_ARRAY_COUNT(ROOM6));
		}
		else if (Area == TEXT("diner") || Area == TEXT("road"))
		{
			Things(TEXT("world.diner.proxies."), DINER, UE_ARRAY_COUNT(DINER));
			const double Day = FMath::Fmod(FMath::Fmod(D.Clock ? D.Clock() : 0, 86400.0) + 86400.0, 86400.0);
			const bool bOnAir = Day < 4 * 3600 || Day > 20 * 3600;
			// (world/Town.ts's people and the deputy's car as they are now: not ported yet; nobody in)
			const FS47Town* TownState = D.M->TownLayer ? D.M->TownLayer->Town.Get() : nullptr;
			const FThing Town[] = {
				{ TEXT("pumps"), TEXT("the gas station"), 4, 250, 1.2, nullptr }, { TEXT("gasWindow"), TEXT("the attendant behind the night window"), 0.8, 30, 2, nullptr },
				{ TEXT("sheriffWindow"), TownState && TownState->bDeputyIn ? TEXT("the deputy at his desk") : TEXT("the sheriff's substation"), 2, 150, TownState && TownState->bDeputyIn ? 1.8 : 1.1, nullptr },
				{ TEXT("laundromat"), TEXT("the laundromat"), 3, 150, 1, nullptr }, { TEXT("cinema"), TEXT("The Plaza"), 4, 250, 1.2, TEXT("plaza") }, { TEXT("marquee"), TEXT("the marquee"), 2, 250, 1.1, TEXT("plaza") },
				{ TEXT("studio"), bOnAir ? TEXT("Earl at the microphone") : TEXT("the studio"), 1.5, 40, bOnAir ? 1.8 : 1.0, TEXT("1420") }, { TEXT("amDoor"), TEXT("1420 AM"), 1.5, 120, 1, TEXT("1420") },
				{ TEXT("souvenir"), TEXT("the souvenir shop"), 1.5, 120, 1.1, nullptr }, { TEXT("letters"), TEXT("the church sign"), 1.2, 80, 0.9, TEXT("church") }, { TEXT("churchDoor"), TEXT("the church"), 3, 250, 1, TEXT("church") },
				{ TEXT("feed"), TEXT("the feed store"), 3, 150, 0.9, nullptr }, { TEXT("payphone"), TEXT("the payphone"), 0.3, 12, 0.9, nullptr }, { TEXT("tv1"), TEXT("a lit window"), 0.8, 40, 0.6, nullptr },
				{ TEXT("tv2"), TEXT("a lit window"), 0.8, 40, 0.6, nullptr }, { TEXT("kitchen"), TEXT("a lit window"), 0.8, 40, 0.5, nullptr },
			};
			if (Web->Obj(TEXT("world.town.group"))) Things(TEXT("world.town.proxies."), Town, UE_ARRAY_COUNT(Town));
			if (TownState)
			{
				TArray<FThing> More;
				if (TownState->bWomanIn) More.Add({ TEXT("woman"), TEXT("the woman with the radio"), .5, 25, 2.2, nullptr });
				if (TownState->bSmokerIn) More.Add({ TEXT("smoker"), TEXT("the man on the stoop"), .5, 30, 2.2, nullptr });
				if (TownState->bPaperIn) More.Add({ TEXT("paper"), TEXT("the newspapers"), .3, 8, .9, nullptr });
				if (TownState->bDeputyIn || TownState->bCarOut) More.Add({ TEXT("patrolCar"), TownState->bCarOut ? TEXT("the deputy's car with its door open") : TEXT("the patrol car"), 2.4, 120, TownState->bCarOut ? 2.4 : 1.1, nullptr });
				Things(TEXT("world.town.proxies."), More.GetData(), More.Num());
				if (TownState->bCutoutOut) Add(TEXT("the cardboard alien on the shoulder"), { TownState->Group.LocalToWorld(TownState->CutoutAt) + FVector(0, .6, 0) }, [](FS47Subject& S) { S.R = .6; S.Max = 120; S.W = 2.5; });
			}
		}

		// the sky at night: Halley's comet until it sets, and the Milky Way looking up
		if (bNight && bOpen)
		{
			// (the comet is up from the evening until it sets a little after three: Sky.setCometClock;
			// the sky follows the eye and is turned with the area's sky, World.skyYaw)
			const double S = FMath::Fmod(FMath::Fmod(D.Clock ? D.Clock() : 0, 86400.0) + 86400.0, 86400.0);
			const FS47Obj Comet = Web->Obj(TEXT("sky.comet"));
			if (Comet && (S > 20 * 3600 || S < 3 * 3600 + 5 * 60))
			{
				const FVector L = Comet.Position();
				const double Yaw = World ? World->SkyYaw() : 0;
				const FVector Rot(L.X * FMath::Cos(Yaw) + L.Z * FMath::Sin(Yaw), L.Y, -L.X * FMath::Sin(Yaw) + L.Z * FMath::Cos(Yaw));
				Add(TEXT("Halley's comet, low in the south"), { Cam + Rot }, [](FS47Subject& Sj) { Sj.Ang = 1.4; Sj.W = 1.6; Sj.Max = 1e6; Sj.bSky = true; });
			}
			const FVector F = Camera.Forward;
			if (F.Y > 0.45) Add(TEXT("the Milky Way"), { Cam + F * 900 }, [](FS47Subject& Sj) { Sj.Ang = 18; Sj.W = 0.35; Sj.Max = 1e6; Sj.bSky = true; });
			// (no stars at 026 where the dishes point: story/starHole.ts, not ported yet)
		}

		// what should not be there
		const FS47Obj G = S47SceneObject(Web, TEXT("glimpse"));
		// (the figures only when they are more than a speck on the print: core/album.ts, Subject.h)
		if (G && G.Visible()) Add(TEXT("someone"), { P(G, 0.7), P(G, 1.05), P(G, 0.3) }, [](FS47Subject& S) { S.R = 0.3; S.Max = 330; S.Odd = FS47Album::NOBODY; S.H = 1.2; });
		if (D.M->TownLayer && !D.M->TownLayer->Cutouts->Up.IsEmpty()) Add(TEXT("the cardboard alien"), { D.M->TownLayer->Cutouts->Position() + FVector(0, .8, 0) }, [](FS47Subject& S) { S.R = .5; S.Max = 70; S.Odd = CARDBOARD(); S.H = 1.5; });
		for (int32 I = 0; I < Web->PartCount(); ++I)
		{
			const FS47WebNode* N = Web->Node(I);
			if (!N || N->Parent >= 0 || !N->bVis) continue;
			if (N->Name == TEXT("abduct:record")) Add(TEXT("someone in a SARO jacket"), { P(FS47Obj(Web, I), 1.1) }, [](FS47Subject& S) { S.R = 0.5; S.Max = 400; S.Odd = JACKET(); S.H = 1.75; });
			if (N->Name == TEXT("abduct:ring")) Add(TEXT("a ring of flattened grass, white with frost"), { P(FS47Obj(Web, I)) }, [](FS47Subject& S) { S.R = 2.7; S.Max = 50; S.W = 2; });
		}
		return Out;
	}
}

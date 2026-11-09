#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47Drive.h"
#include "S47EventPlaces.h"
#include "S47FieldLife.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47Geo.h"
#include "S47Inter.h"
#include "S47Legs.h"
#include "S47Main.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47RadioTips.h"
#include "S47TownSites.h"
#include "S47TownUse.h"
#include "S47Townsfolk.h"
#include "S47Visitors.h"
#include "S47MotelNight.h"
#include "S47Places.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47Predecessor.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "SS47FieldMap.h"
#include "Containers/Ticker.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47TownDev, Log, All);

// F10, M4 (NEW-PLACES-20261108, TOWN-USE-20261108): the temporary boxes from where a player would first see them, the town on the
// survey sheet, whether they hold the walker and the truck; and each counter in Caliche, the tip to 1420 and what comes back, the tour,
// a lure's morning. Nothing here is saved. -S47DevSweep=/tmp/dir -S47DevSweepGroup="Town (M4)" runs the group in turn, a picture each.
namespace S47TownDevLocal
{
	const TCHAR* const Group = TEXT("Town (M4)");
	// The walker's yaw that faces from A toward B (0 faces north, a quarter turn faces west), the road's land unturned.
	double YawToward(const FVector2D& From, const FVector2D& To)
	{
		const FVector2D D = (To - From).GetSafeNormal();
		return FMath::Atan2(-D.X, -D.Y);
	}
	// Where to stand to see each site whole: off its front, on the side a driver comes from, road-local.
	struct FView { const TCHAR* Id; double X, Z; };
	const FView Views[] = { { TEXT("trailers"), 10, 1862 }, { TEXT("school"), -6, 2205 }, { TEXT("grange"), 6, 2300 } };
}

void FS47DevFreeform::AddTown(FS47DevEvents& E)
{
	using namespace S47TownDevLocal;
	// Stand on the road's land (the diner's area walks it) at a road-local point, facing a road-local point, by day.
	const auto StandAt = [this](const FVector2D& Road, const FVector2D& Look, double From, double To, TFunction<void()> Then)
	{
		const FVector Origin = S47Legs::ORIGINS(TEXT("diner"));
		FStage Stage; Stage.Area = TEXT("diner"); Stage.From = From; Stage.To = To;
		Stage.Where = [Road, Origin] { return FVector2D(Origin.X + Road.X, Origin.Z + Road.Y); };
		Stage.Yaw = YawToward(Road, Look); Stage.Pitch = -.04;
		Go(Stage, Then);
	};
	const auto Pictured = [this](const TCHAR* Kind, float Delay = 1.2f)
	{
		if (SweepDir.IsEmpty()) return;
		const FString Path = SweepDir / FString::Printf(TEXT("%s-%03d.png"), Kind, SweepIndex);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Path](float) { FScreenshotRequest::RequestScreenshot(Path, true, false); return false; }), Delay);
	};
	for (const FView& V : Views)
	{
		const S47TownSites::FSite* Site = S47TownSites::Find(V.Id);
		if (!Site) continue;
		const FString Id = V.Id, Sign = Site->Sign;
		const FVector2D Eye(V.X, V.Z), Middle(Site->X, Site->Z);
		E.AddFreeform(Group, FString::Printf(TEXT("Temp box: %s"), *Sign), TEXT("by day, from the highway where a driver first sees it; says whether it is up, seated and in the walker's way"), [this, Id, Sign, Eye, Middle, StandAt]
		{
			StandAt(Eye, Middle, 10, 16, [this, Id, Sign]
			{
				FS47FreeformSession* F = Session();
				FS47TownSites* T = F ? F->TownSites.Get() : nullptr;
				if (!T || !T->Built()) { Tell(false, TEXT("the temporary boxes are not built (Freeform only)")); return; }
				Tell(true, FString::Printf(TEXT("%s: %d parts and boards in all, %d colliders up, seated on the walker's floor: %s, %d plants hidden under the boxes"), *Sign, T->PartCount(), T->ColliderCount(), T->Seated(Id) ? TEXT("yes") : TEXT("not yet"), T->HiddenPlants()), 9);
			});
		});
	}
	E.AddFreeform(Group, TEXT("Temp boxes: the town on the survey sheet"), TEXT("stands in the middle of Caliche and opens the map close in: the three boxes in ink with their names"), [this, StandAt, Pictured]
	{
		StandAt(FVector2D(0, 2090), FVector2D(0, 2200), 10, 16, [this, Pictured]
		{
			S47FieldMap::DevScale(1.25);
			if (M && M->UI) { M->UI->Close(true); M->UI->NotebookAt(TEXT("map")); }
			if (M && M->Flow) M->Flow->OpenNotebook();
			Pictured(TEXT("townmap"));
			Tell(true, TEXT("The survey sheet over Caliche: Trailer Park, School and Grange Hall, each (temp)."), 6);
		});
	});
	E.AddFreeform(Group, TEXT("Temp boxes: do they hold the walker and the truck?"), TEXT("inside each solid box: a collider in the walker's list and a box in the truck's"), [this]
	{
		FS47FreeformSession* F = Session();
		if (!F || !M || !M->Web || !M->World) { Tell(false, TEXT("Freeform only")); return; }
		const FVector Road = S47Legs::ORIGINS(TEXT("road"));
		IS47DriveArea* Drive = M->World->LegArea(TEXT("road"));
		int32 Walk = 0, Truck = 0, All = 0;
		for (const FBox2D& B : S47TownSites::SolidFootprints())
		{
			++All;
			const FVector2D C = B.GetCenter() + FVector2D(Road.X, Road.Z);
			Walk += M->Web->CollidersMutable(TEXT("road")).ContainsByPredicate([&C](const FS47Rect& R) { return R.bEnabled && C.X >= R.MinX && C.X <= R.MaxX && C.Y >= R.MinZ && C.Y <= R.MaxZ; }) ? 1 : 0;
			Truck += Drive && Drive->Obstacles.ContainsByPredicate([&C](const FS47Obstacle& O) { return !O.bCircle && C.X >= O.MinX && C.X <= O.MaxX && C.Y >= O.MinZ && C.Y <= O.MaxZ; }) ? 1 : 0;
		}
		const bool bOk = All > 0 && Walk == All && Truck == All;
		UE_LOG(LogS47TownDev, Display, TEXT("S47: temp boxes hold: walker %d of %d, truck %d of %d"), Walk, All, Truck, All);
		Tell(bOk, FString::Printf(TEXT("Walker held at %d of %d boxes, the truck at %d of %d"), Walk, All, Truck, All), 7);
	});
	// ---- the counters ----
	const auto Counter = [this, StandAt, Pictured](const FVector2D& Road, const FVector2D& Look, double Hour, TFunction<void(FS47TownUse&)> Open)
	{
		StandAt(Road, Look, Hour, Hour + 1, [this, Open, Pictured]
		{
			FS47FreeformSession* F = Session();
			if (!F || !F->TownUse) { Tell(false, TEXT("Freeform only")); return; }
			Open(*F->TownUse);
			Pictured(TEXT("counter"), 1.5f);
			bLastOk = true; LastText = TEXT("a counter in Caliche");
		});
	};
	// Road-local: the station's lobby x 20 to 27.3, z 2030.3 to 2032.9; the Feed & Seed's room x -31 to -15; the souvenir shop x 20.5 to 29.5.
	E.AddFreeform(Group, TEXT("1420 AM: the lobby board (tips, parts, Earl's notes)"), TEXT("noon, in the lobby, facing the cork board"), [Counter]
	{
		Counter(FVector2D(22.0, 2031.6), FVector2D(27.3, 2031.6), 12, [](FS47TownUse& T) { T.OpenRadio(false); });
	});
	E.AddFreeform(Group, TEXT("1420 AM: the request line from SARO's phone"), TEXT("the same tip line by telephone, wherever you are"), [this, Pictured]
	{
		FS47FreeformSession* F = Session();
		if (!F || !F->TownUse) { Tell(false, TEXT("Freeform only")); return; }
		F->TownUse->OpenRadio(true);
		Pictured(TEXT("phone"), 1.5f);
		Tell(true, TEXT("Earl on the request line."), 4);
	});
	E.AddFreeform(Group, TEXT("1420 AM: a tip given, read on the air and called back now"), TEXT("gives a tip for the first place you have found (else the windmill), then Earl reads it and a caller rings back, whatever the radio"), [this]
	{
		FS47FreeformSession* F = Session();
		if (!F || !F->Tips || !F->Places) { Tell(false, TEXT("Freeform only")); return; }
		FString Place = TEXT("windmill");
		for (const FS47Places::FDef& D : FS47Places::Defs()) if (F->Places->Found(D.Id) && !F->Tips->Gave(D.Id)) { Place = D.Id; break; }
		const bool bGiven = F->Tips->Gave(Place) || F->Tips->Give(Place);
		const FString Read = F->Tips->DevNext(), Back = F->Tips->DevNext();
		const bool bOk = bGiven && !Read.IsEmpty() && !Back.IsEmpty();
		if (M && M->UI) M->UI->Toast(Read + TEXT("\n") + Back, 12);
		Tell(bOk, bOk ? TEXT("Read and called back: the place it points at is on the map as heard of.") : TEXT("no tip could be given today (one a day), or it had already been called back"), 8);
	});
	E.AddFreeform(Group, TEXT("Feed & Seed: the counter"), TEXT("ten in the morning, inside the door, facing the price board"), [Counter]
	{
		Counter(FVector2D(-17.1, 2202.2), FVector2D(-16.1, 2198.6), 10, [](FS47TownUse& T) { T.OpenFeed(); });
	});
	E.AddFreeform(Group, TEXT("Feed & Seed: stuck in sand, out with the come-along"), TEXT("gives a tow rope and come-along, sticks the truck and digs it out: 10 minutes instead of 25"), [this]
	{
		FS47FreeformSession* F = Session();
		if (!F || !F->Care) { Tell(false, TEXT("Freeform only")); return; }
		if (F->InventoryCount(TEXT("rope")) <= 0) F->AddItem(TEXT("rope"), 1);
		F->Care->TestStick();
		const double Before = F->Clock();
		const bool bOut = F->Care->DigOut();
		const double Minutes = (F->Clock() - Before) / 60.;
		Tell(bOut && FMath::IsNearlyEqual(Minutes, FS47TruckCare::RopeDigMinutes, 1.), FString::Printf(TEXT("Dug out in %.0f minutes with the come-along"), Minutes), 6);
	});
	E.AddFreeform(Group, TEXT("Souvenirs: the counter (map, tour, film, prints)"), TEXT("eleven in the morning, inside the shop, facing the counter"), [Counter]
	{
		Counter(FVector2D(21.15, 2063.6), FVector2D(22.2, 2064.5), 11, [](FS47TownUse& T) { T.OpenSouvenir(); });
	});
	E.AddFreeform(Group, TEXT("Souvenirs: the tour now"), TEXT("two hours in his station wagon; a place you had not heard of goes on the map"), [this]
	{
		FS47FreeformSession* F = Session();
		if (!F || !F->TownUse) { Tell(false, TEXT("Freeform only")); return; }
		const FString Said = F->TownUse->DevTour();
		Tell(!Said.IsEmpty(), Said.IsEmpty() ? FString(TEXT("the tour would not go")) : Said, 6);
	});
	for (const bool bShy : { false, true })
	{
		E.AddFreeform(Group, bShy ? TEXT("Lure: salt at the windmill, and something came by in the night") : TEXT("Lure: salt at the windmill, a quiet night"),
			bShy ? TEXT("the salt untouched, the tracks turning back short of it; kept as a field record") : TEXT("licked, and the tracks of whatever came to it"), [this, bShy]
		{
			FS47FreeformSession* F = Session();
			if (!F || !F->TownUse) { Tell(false, TEXT("Freeform only")); return; }
			FString Said;
			const bool bOk = F->TownUse->DevLureNight(TEXT("windmill"), bShy, Said);
			Tell(bOk, bOk ? Said : FString(TEXT("the lure could not be set out")), 9);
		});
	}
	// What the wares put out in the world (critics, round 3: not only text).
	E.AddFreeform(Group, TEXT("Lure: the salt block at the windmill by morning, in the world"), TEXT("salt set out last night at the windmill: the block licked down and hoof prints right up to it, seen from a few steps off"), [this, StandAt, Pictured]
	{
		FS47FreeformSession* F = Session();
		double X = 0, Z = 0;
		if (!F || !F->TownUse || !F->Places || !F->Places->Spot(TEXT("windmill"), X, Z)) { Tell(false, TEXT("Freeform only")); return; }
		FString Said;
		if (!F->TownUse->DevLureNight(TEXT("windmill"), false, Said)) { Tell(false, TEXT("the lure could not be set out")); return; }
		StandAt(FVector2D(X + 7, Z + 6), FVector2D(X + 3, Z + 2), 10, 11, [this, Pictured] { Pictured(TEXT("lure"), 1.5f); bLastOk = true; LastText = TEXT("the salt block, licked"); });
	});
	E.AddFreeform(Group, TEXT("Souvenirs: your picture in his window"), TEXT("eleven in the morning on the sidewalk: the framed print of lights over the flats inside the shop window, once he has bought a strange one"), [this, StandAt, Pictured]
	{
		FS47FreeformSession* F = Session();
		if (!F || !F->TownUse) { Tell(false, TEXT("Freeform only")); return; }
		F->TownUse->Note(TEXT("sold_odd"));
		StandAt(FVector2D(17.6, 2062.4), FVector2D(20.7, 2062.95), 11, 12, [this, Pictured] { Pictured(TEXT("window"), 1.5f); bLastOk = true; LastText = TEXT("the picture in the window"); });
	});
	// ---- TOWNSFOLK-20261108: the regulars ----
	const TCHAR* const Folk = TEXT("Townsfolk (M4)");
	const auto TalkTo = [this, StandAt, Pictured](const FString& Who, const FVector2D& Eye, const FVector2D& Look, double Hour)
	{
		StandAt(Eye, Look, Hour, Hour + 1, [this, Who, Pictured]
		{
			FS47FreeformSession* F = Session();
			if (!F || !F->Townsfolk || !F->Predecessor) { Tell(false, TEXT("Freeform only")); return; }
			if (Who == TEXT("waitress")) F->Townsfolk->OpenDiner(); else F->Predecessor->Talk(Who);
			Pictured(TEXT("folk"), 1.5f);
			bLastOk = true; LastText = TEXT("a talk in Caliche");
		});
	};
	E.AddFreeform(Folk, TEXT("Townsfolk: the deputy at the substation"), TEXT("ten in the morning; what he remembers of you, or what is going round"), [TalkTo] { TalkTo(TEXT("deputy"), FVector2D(-8, 2112), FVector2D(-16, 2109), 10); });
	E.AddFreeform(Folk, TEXT("Townsfolk: the waitress at the Mesa"), TEXT("noon at the counter"), [TalkTo] { TalkTo(TEXT("waitress"), FVector2D(-10, 2000), FVector2D(-22, 2000), 12); });
	E.AddFreeform(Folk, TEXT("Townsfolk: the attendant at the night window"), TEXT("ten at night"), [TalkTo] { TalkTo(TEXT("attendant"), FVector2D(-12, 2072), FVector2D(-17, 2072), 22); });
	E.AddFreeform(Folk, TEXT("Townsfolk: what each of them could say (log)"), TEXT("every line each of the four has for you now, newest first, in the log"), [this]
	{
		FS47FreeformSession* F = Session();
		if (!F || !F->Townsfolk) { Tell(false, TEXT("Freeform only")); return; }
		int32 N = 0;
		for (const FString& Who : FS47Townsfolk::People()) for (const FString& Line : F->Townsfolk->Candidates(Who)) { UE_LOG(LogS47TownDev, Display, TEXT("S47: townsfolk %s could say %s"), *Who, *Line); ++N; }
		Tell(N >= 4, FString::Printf(TEXT("%d lines in the log for the four of them"), N), 5);
	});
	for (const bool bDeputy : { true, false })
	{
		E.AddFreeform(Folk, bDeputy ? TEXT("Townsfolk: the deputy goes missing") : TEXT("Townsfolk: the souvenir man goes missing"),
			bDeputy ? TEXT("from tonight: the substation dark and no car out front; the attendant talks of nothing else") : TEXT("ten at night: nobody on the stoop; the waitress talks of nothing else"), [this, bDeputy, StandAt, Pictured]
		{
			FS47FreeformSession* F = Session();
			if (!F || !F->Townsfolk) { Tell(false, TEXT("Freeform only")); return; }
			F->Townsfolk->DevReturnAll();
			const FString Who = bDeputy ? TEXT("deputy") : TEXT("souvenir_man");
			if (!F->Townsfolk->Vanish(Who, FS47Townsfolk::WorksAt(Who))) { Tell(false, TEXT("could not take them")); return; }
			const FVector2D Eye = bDeputy ? FVector2D(-6, 2114) : FVector2D(8, 2066), Look = bDeputy ? FVector2D(-16, 2106) : FVector2D(20.5, 2066);
			StandAt(Eye, Look, bDeputy ? 10 : 22, bDeputy ? 11 : 23, [this, bDeputy, Pictured]
			{
				After(1.0, [this, bDeputy, Pictured]
				{
					FS47FreeformSession* G = Session();
					const FS47Town* Town = M && M->TownLayer && M->TownLayer->Town ? M->TownLayer->Town.Get() : nullptr;
					const bool bGone = Town && (bDeputy ? !Town->bDeputyIn : !Town->bSmokerIn);
					Pictured(TEXT("gone"), .2f);
					if (G && G->Townsfolk) { if (bDeputy && G->Predecessor) G->Predecessor->Talk(TEXT("attendant")); else G->Townsfolk->OpenDiner(); }
					Pictured(TEXT("gonetalk"), 1.6f);
					Tell(bGone, bGone ? FString(TEXT("Gone from the post; the others talk of it (a field record is kept), and 1420's callers will ask.")) : FString(TEXT("the figure is still at the post")), 7);
				});
			});
		});
	}
	E.AddFreeform(Folk, TEXT("Townsfolk: everyone comes back"), TEXT("whoever is gone is back today, and will not say where"), [this]
	{
		FS47FreeformSession* F = Session();
		if (!F || !F->Townsfolk) { Tell(false, TEXT("Freeform only")); return; }
		F->Townsfolk->DevReturnAll();
		Tell(true, TEXT("All four at their posts."), 4);
	});
	// ---- NEIGHBOR, ENTHUSIASTS, MILITARY ----
	const TCHAR* const Out = TEXT("Visitors (M4)");
	E.AddFreeform(Out, TEXT("Visitors: Eloy Baca on his porch"), TEXT("ten in the morning at the Baca place under the east rim; today's story, kept as an unconfirmed record"), [this, StandAt, Pictured]
	{
		StandAt(FVector2D(812, 1362), FVector2D(812, 1354), 10, 11, [this, Pictured]
		{
			FS47FreeformSession* F = Session();
			if (!F || !F->Visitors) { Tell(false, TEXT("Freeform only")); return; }
			F->Visitors->OpenBaca(); Pictured(TEXT("baca"), 1.5f); bLastOk = true; LastText = TEXT("Baca's story");
		});
	});
	E.AddFreeform(Out, TEXT("Visitors: Baca after a night's event (his version)"), TEXT("lights over the church last night, then his porch: he may put it somewhere else, or on another night"), [this]
	{
		FS47FreeformSession* F = Session();
		if (!F || !F->Visitors || !F->EventPlaces) { Tell(false, TEXT("Freeform only")); return; }
		F->EventPlaces->Used(TEXT("church"), TEXT("dev_lights"));
		const FS47Visitors::FTale T = F->Visitors->Tale(F->Day());
		Tell(!T.Text.IsEmpty(), FString::Printf(TEXT("%s (%s)"), *T.Text, T.bTrue ? TEXT("as it was") : TEXT("not as it was")), 9);
	});
	E.AddFreeform(Out, TEXT("Visitors: Baca gone"), TEXT("the night's events take him; his door stands open"), [this, StandAt, Pictured]
	{
		FS47FreeformSession* F = Session();
		if (!F || !F->Visitors) { Tell(false, TEXT("Freeform only")); return; }
		F->Visitors->VanishBaca();
		StandAt(FVector2D(812, 1362), FVector2D(812, 1354), 10, 11, [this, Pictured] { if (FS47FreeformSession* G = Session()) if (G->Visitors) G->Visitors->OpenBaca(); Pictured(TEXT("bacagone"), 1.5f); bLastOk = true; LastText = TEXT("Baca gone"); });
	});
	E.AddFreeform(Out, TEXT("Visitors: the Sky Watch van at the Mesa"), TEXT("noon, the van on the lot north of the diner, and what they want"), [this, StandAt, Pictured]
	{
		FS47FreeformSession* F = Session();
		if (!F || !F->Visitors) { Tell(false, TEXT("Freeform only")); return; }
		F->Visitors->DevVan();
		StandAt(FVector2D(-32, 1990), FVector2D(-40, 1982), 12, 13, [this, Pictured]
		{
			Pictured(TEXT("vanworld"), .8f);
			After(2.0, [this, Pictured] { if (FS47FreeformSession* G = Session()) if (G->Visitors) G->Visitors->OpenVan(); Pictured(TEXT("van"), 1.5f); });
			bLastOk = true; LastText = TEXT("the Sky Watch");
		});
	});
	// Stand at a road-local point in the area that owns it, facing another, at an hour; then Then.
	const auto StandIn = [this](const FString& Area, const FVector2D& Eye, const FVector2D& Look, double From, double Pitch, TFunction<void()> Then)
	{
		FStage Stage; Stage.Area = Area; Stage.From = From; Stage.To = From + 1;
		const FVector At = S47OpenLand::FromRoad(Area, Eye.X, Eye.Y);
		Stage.Where = [At] { return FVector2D(At.X, At.Z); };
		Stage.Yaw = S47TownDevLocal::YawToward(Eye, Look); Stage.Pitch = Pitch;
		Go(Stage, Then);
	};
	E.AddFreeform(Out, TEXT("Visitors: the military's morning at the farm"), TEXT("first light at the Alderete place, ten metres from where the two black sedans park at its way in: the helicopter over it, the cars, then they drive off as you stand there"), [this, StandIn, Pictured]
	{
		FS47FreeformSession* F = Session();
		const FS47EventPlace* P = F && F->EventPlaces ? F->EventPlaces->Find(TEXT("farm")) : nullptr;
		if (!F || !F->Visitors || !P) { Tell(false, TEXT("Freeform only")); return; }
		// Where the cars will stand (at the farm's way in, clear of its buildings), and a place ten metres off them on the far side from
		// the pasture: the cars fill the near half of the picture, the pasture and the helicopter over it the far half.
		FVector2D A, B;
		if (!F->Visitors->CarsSpot(TEXT("farm"), A, B)) { A = FVector2D(P->EntryX + 9, P->EntryZ + 6); B = A + FVector2D(3.4, 0); }
		const FVector2D Cars = (A + B) * .5, Place(P->X, P->Z);
		const FVector2D Away = (Cars - Place).GetSafeNormal();
		const FVector2D Eye = Cars + Away * 10 + FVector2D(-Away.Y, Away.X) * 2.5;
		UE_LOG(LogS47TownDev, Display, TEXT("S47: F10 military: cars at road %.1f %.1f and %.1f %.1f, eye %.1f %.1f"), A.X, A.Y, B.X, B.Y, Eye.X, Eye.Y);
		StandIn(FS47EventPlaces::AreaOf(*P), Eye, Cars, 6, .14, [this, Pictured]
		{
			FS47FreeformSession* G = Session();
			if (!G || !G->Visitors || !G->Visitors->DevMilitary(TEXT("farm"), true)) { Tell(false, TEXT("no such place")); return; }
			// The helicopter comes over the place about 40 s in; the cars stand from 20 s; held for the pictures until about 56 s.
			After(39, [this, Pictured] { Pictured(TEXT("milheli"), .2f); });
			After(50, [this, Pictured] { Pictured(TEXT("milcars"), .2f); });
			After(59.5, [this, Pictured] { Pictured(TEXT("milgo"), .2f); });
			bLastOk = true; LastText = TEXT("the military's morning");
		});
	});
	E.AddFreeform(Out, TEXT("Visitors: the ground they went over"), TEXT("eleven in the morning at the farm, hours after they left: the raked circle and the tire tracks where the marks were"), [this, StandIn, Pictured]
	{
		FS47FreeformSession* F = Session();
		const FS47EventPlace* P = F && F->EventPlaces ? F->EventPlaces->Find(TEXT("farm")) : nullptr;
		if (!F || !F->Visitors || !P) { Tell(false, TEXT("Freeform only")); return; }
		const FVector2D Place(P->X, P->Z), Entry(P->EntryX, P->EntryZ);
		const FVector2D Eye = Place + (Entry - Place).GetSafeNormal() * 16;
		StandIn(FS47EventPlaces::AreaOf(*P), Eye, Place, 11, -.22, [this, Pictured]
		{
			FS47FreeformSession* G = Session();
			if (!G || !G->Visitors || !G->Visitors->DevRaked(TEXT("farm"))) { Tell(false, TEXT("no such place")); return; }
			Pictured(TEXT("raked"), 1.5f);
			Tell(G->Visitors->Cleared(TEXT("farm"), FS47FreeformSession::NightOf(G->Clock())), TEXT("Raked smooth in a wide circle, tire tracks across it toward the way in."), 6);
		});
	});
	E.AddFreeform(Out, TEXT("Visitors: the Sky Watch at SARO's gate"), TEXT("half past nine the evening they were let in: their van on the visitors' lot, the dome light on, and what they do"), [this, StandIn, Pictured]
	{
		FS47FreeformSession* F = Session();
		if (!F || !F->Visitors) { Tell(false, TEXT("Freeform only")); return; }
		F->Visitors->DevVanGate();
		// The visitors' lot by SARO's gate (SARO's own x -18 to -8, z 10 to 20): from the south side of it, looking north.
		const FVector2D Lot(-13 + S47Geo::SARO_IN_ROAD.X, 15.5 + S47Geo::SARO_IN_ROAD.Z);
		StandIn(TEXT("saro"), Lot + FVector2D(4, 12), Lot, 21.5, -.04, [this, Pictured]
		{
			Pictured(TEXT("vangate"), 2.f);
			After(3.0, [this, Pictured] { if (M && M->Inter) M->Inter->UseId(TEXT("ff:visit:van_saro")); Pictured(TEXT("vangatetalk"), 1.2f); });
			bLastOk = true; LastText = TEXT("the Sky Watch at the gate");
		});
	});
	E.AddFreeform(Out, TEXT("Visitors: the Sky Watch van, empty, out where they went"), TEXT("by day at the windmill on the Baca lease: the van standing empty, the side door open, a flashlight on the ground"), [this, StandIn, Pictured]
	{
		FS47FreeformSession* F = Session();
		if (!F || !F->Visitors || !F->Visitors->DevVanEmpty(TEXT("windmill"))) { Tell(false, TEXT("Freeform only")); return; }
		FString Area; FVector2D At; bool bOpen = false;
		if (!F->Visitors->VanWhere(Area, At, bOpen)) { Tell(false, TEXT("the van is not anywhere")); return; }
		// From its open side (east), a little behind.
		StandIn(Area, At + FVector2D(8, 5), At, 11, -.05, [this, Pictured]
		{
			Pictured(TEXT("vanempty"), 2.f);
			After(3.0, [this, Pictured] { if (M && M->Inter) M->Inter->UseId(TEXT("ff:visit:van")); Pictured(TEXT("vanemptytalk"), 1.2f); });
			bLastOk = true; LastText = TEXT("the empty van");
		});
	});
	// ---- MOTEL-SLEEP-20261108 ----
	const TCHAR* const Motel = TEXT("Motel (M4)");
	static const TCHAR* const KindNames[] = { TEXT("a quiet night"), TEXT("the door handle tried"), TEXT("a car idling outside"), TEXT("the phone rings once"), TEXT("the truck gone through"), TEXT("a party next door") };
	for (int32 K = 0; K < 6; ++K)
	{
		E.AddFreeform(Motel, FString::Printf(TEXT("Motel: a night with %s"), KindNames[K]), TEXT("a room at the Sierra Motor Court from 22:00; what the night is, at its hour"), [this, K, Pictured]
		{
			FS47FreeformSession* F = Session();
			if (!F || !F->MotelNight || !F->Places) { Tell(false, TEXT("Freeform only")); return; }
			if (K == 4 && F->InventoryCount(TEXT("spare")) <= 0 && F->InventoryCount(TEXT("water")) <= 0) F->AddItem(TEXT("water"), 1);
			FStage Stage; Stage.From = 22; Stage.To = 23; Stage.Area = TEXT("saro");
			double X = 0, Z = 0; F->Places->Spot(TEXT("motel"), X, Z);
			Stage.Where = [X, Z] { return FVector2D(X + 1.5, Z); };
			Go(Stage, [this, K, Pictured]
			{
				FS47FreeformSession* G = Session();
				if (!G || !G->MotelNight) return;
				if (G->Budget() < G->MotelPrice()) G->Earn(G->MotelPrice());
				G->MotelNight->DevKind(K);
				const double Next = (FMath::FloorToDouble((G->Clock() - 8 * 3600.) / 86400.) + 1) * 86400. + 8 * 3600.;
				const double Until = G->MotelNight->SleepUntil(Next);
				const bool bSlept = G->RestAt(Until, FS47FreeformSession::ERest::Motel);
				if (bSlept) G->MotelNight->AfterRest(Next);
				Pictured(TEXT("motel"), 1.2f);
				Tell(bSlept && G->MotelNight->Unfiled(), bSlept ? FString::Printf(TEXT("Woken at %s; the night is not filed until SARO"), *FString::Printf(TEXT("%02d:%02d"), int32(FMath::Fmod(G->Clock(), 86400.)) / 3600, (int32(FMath::Fmod(G->Clock(), 86400.)) / 60) % 60)) : FString(TEXT("the rest was refused")), 7);
			});
		});
	}
}

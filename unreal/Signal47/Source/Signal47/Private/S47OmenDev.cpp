#include "S47DevFreeform.h"
#include "S47Web.h"
#include "S47Animals.h"
#include "S47DevEvents.h"
#include "S47EventPlaces.h"
#include "S47FarmWorld.h"
#include "S47Flashlight.h"
#include "S47Freeform.h"
#include "S47GearFlicker.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Omen.h"
#include "S47OpenLand.h"
#include "S47Seen.h"
#include "S47Soundscape.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47World.h"

// F10, M2 (GEAR-FLICKER, SOUNDSCAPE, ANIMALS, 8 November): each from its own line, with the hour and the place set so that it can be
// seen and heard, and something real-looking (FS47Omen::DevPresence) for them to react to. Nothing here is saved; Stop everything
// clears the made-up presence and the silence. -S47DevSweep=/tmp/dir -S47DevSweepGroup="Animals (M2)" runs a group in turn.
namespace S47OmenDevLocal
{
	// A place of the catalogue: the area that owns its ground and a spot off it, in that area's terms.
	bool Spot(US47Main* M, const TCHAR* Id, double OffX, double OffZ, FString& OutArea, FVector2D& OutAt)
	{
		if (!M || !M->Freeform || !M->Freeform->EventPlaces) return false;
		const FS47EventPlace* Pl = M->Freeform->EventPlaces->Find(Id);
		if (!Pl) return false;
		OutArea = FS47EventPlaces::AreaOf(*Pl);
		const FVector Here = S47OpenLand::FromRoad(OutArea, Pl->X + OffX, Pl->Z + OffZ);
		OutAt = FVector2D(Here.X, Here.Z);
		return true;
	}
	// A road-local point in the area the player is in, on the ground and a little over it.
	FVector Land(US47Main* M, double RoadX, double RoadZ, double Up)
	{
		FVector Here = S47OpenLand::FromRoad(M && M->Web ? M->Web->Area() : FString(TEXT("road")), RoadX, RoadZ);
		Here.Y = S47FarmWorld::FloorRoad(RoadX, RoadZ) + Up;
		return Here;
	}
	void LampOn(US47Main* M)
	{
		if (M && M->Night && M->Night->Flashlight && M->Night->Flashlight->bHave) M->Night->Flashlight->bOn = true;
	}
}

void FS47DevFreeform::AddOmens(FS47DevEvents& E)
{
	static const TCHAR* const GearGroup = TEXT("Gear near things (M2)");
	static const TCHAR* const SoundGroup = TEXT("Night sounds (M2)");
	static const TCHAR* const AnimalGroup = TEXT("Animals (M2)");
	// On foot at a place of the catalogue, at night, looking along +Look degrees; then Then.
	const auto At = [this](const TCHAR* Id, double OffX, double OffZ, TFunction<void()> Then)
	{
		FStage Stage; Stage.From = 23; Stage.To = 4.5;
		FString Area; FVector2D Where;
		if (!S47OmenDevLocal::Spot(M, Id, OffX, OffZ, Area, Where)) { Tell(false, TEXT("the land's places are not there in this run")); return; }
		Stage.Area = Area; Stage.Where = [Where] { return Where; };
		Go(Stage, Then);
	};
	// Something made up, Dist ahead of the eyes and Side to the right, as dangerous as Severity, for Seconds.
	const auto Presence = [this](double Dist, double Side, double Severity, double Seconds, bool bComing)
	{
		if (!M->Omen || !M->Player) return false;
		const FS47Cam Cam = FS47Cam::Of(M->Player);
		FVector Ahead = Cam.Forward; Ahead.Y = 0; Ahead = Ahead.GetSafeNormal();
		FVector Lateral = Cam.Right; Lateral.Y = 0; Lateral = Lateral.GetSafeNormal();
		M->Omen->DevPresence(S47Three::ToThree(M->Player->EyeUE()) + Ahead * Dist + Lateral * Side, Severity, Seconds, bComing);
		return true;
	};

	// ---- the gear ----
	E.AddFreeform(GearGroup, TEXT("Something 60 m off in the dark"), TEXT("23:00, open land by the Baca windmill, lamp on; walk toward it and it gets worse"), [this, At, Presence]
	{
		At(TEXT("windmill"), -40, 0, [this, Presence]
		{
			S47OmenDevLocal::LampOn(M);
			Presence(60, 0, 0.8, 45, false);
			Tell(true, TEXT("Something stands 60 m ahead for 45 seconds. The lamp sags and blinks, the walkie hisses; nearer is worse."), 6);
		});
	});
	E.AddFreeform(GearGroup, TEXT("Something 15 m off"), TEXT("23:00, open land; the lamp goes out for moments, the walkie breaks up, the watch jumps"), [this, At, Presence]
	{
		At(TEXT("windmill"), -40, 0, [this, Presence]
		{
			S47OmenDevLocal::LampOn(M);
			Presence(15, 0, 0.9, 25, false);
			Tell(true, TEXT("Something 15 m ahead for 25 seconds: the close band."), 5);
		});
	});
	E.AddFreeform(GearGroup, TEXT("Something over the road ahead (in the truck)"), TEXT("as you are, driving: the lights dim and stutter, the engine coughs, the radio fuzzes, the dash clock jumps"), [this, Presence]
	{
		if (!M->World || !M->World->bDriving) { Tell(false, TEXT("drive the truck first")); return; }
		Presence(45, 0, 0.95, 30, false);
		Tell(true, TEXT("Something 45 m ahead of the truck for 30 seconds."), 5);
	});
	E.AddFreeform(GearGroup, TEXT("What the gear reads now"), TEXT("the level, its band (0 nothing to 4 on you) and what is out there"), [this]
	{
		if (!M->Gear || !M->Omen) { Tell(false, TEXT("Freeform only")); return; }
		FString Seen;
		for (const FS47OmenSource& Src : M->Omen->Sources()) Seen += (Seen.IsEmpty() ? TEXT("") : TEXT(", ")) + Src.Id;
		Tell(true, FString::Printf(TEXT("Gear %.2f, band %d; coughs %d, clock jumps %d; out there: %s"), M->Gear->Level(), FS47GearFlicker::Band(M->Gear->Level()),
			M->Gear->Coughs(), M->Gear->ClockJumps(), Seen.IsEmpty() ? TEXT("nothing") : *Seen), 7);
	});

	// ---- the night's sounds ----
	E.AddFreeform(SoundGroup, TEXT("Something comes: the night stops first"), TEXT("23:00, open land; six seconds of its first sign, 80 m off, then it is there for half a minute"), [this, At, Presence]
	{
		At(TEXT("windmill"), -40, 0, [this, Presence]
		{
			// a moment for the night to be heard before it stops
			After(8, [this, Presence] { Presence(80, 30, 0.85, 40, true); Tell(true, TEXT("Something is coming, 80 m off: everything out here stops at once."), 5); });
			Tell(true, TEXT("Listen to the night for eight seconds."), 4);
		});
	});
	E.AddFreeform(SoundGroup, TEXT("The silence alone (30 s)"), TEXT("as you are, outdoors at night: everything stops at once, then comes back a cricket at a time"), [this]
	{
		if (!M->Soundscape) { Tell(false, TEXT("Freeform only")); return; }
		M->Soundscape->DevHush(30);
		Tell(true, TEXT("Thirty seconds of silence, as before something comes."), 4);
	});
	struct FKind { const TCHAR* Kind; const TCHAR* Label; const TCHAR* Note; };
	static const FKind SoundKinds[] = {
		{ TEXT("windmill"), TEXT("A windmill creaks"), TEXT("the nearest of the two, within two kilometres") },
		{ TEXT("wires"), TEXT("Wind in the wires"), TEXT("the power line within 400 m, for 25 seconds") },
		{ TEXT("cattle"), TEXT("Cattle lowing far off"), TEXT("from the nearest pen or trough") },
		{ TEXT("coyote"), TEXT("Coyotes from a wash"), TEXT("the washes and open land 220 m to 1.5 km off") },
		{ TEXT("train"), TEXT("A train on the line to Roswell"), TEXT("always from the east") },
		{ TEXT("owl"), TEXT("An owl by the buildings"), TEXT("the farm, the town or the station within 650 m") } };
	for (const FKind& Entry : SoundKinds)
	{
		const FString Kind = Entry.Kind;
		E.AddFreeform(SoundGroup, Entry.Label, Entry.Note, [this, Kind]
		{
			FString Why;
			const bool bOk = M->Soundscape && M->Soundscape->DevPlay(Kind, Why);
			Tell(bOk, Why.IsEmpty() ? FString(TEXT("Freeform only")) : Why, 5);
		});
	}
	E.AddFreeform(SoundGroup, TEXT("Go: under the wires at the county road turnoff"), TEXT("23:00, 8 m from the highway's line"), [this, At]
	{
		At(TEXT("road:turnoff"), -5, 0, [this] { Tell(true, TEXT("Under the line by the turnoff. The wires moan with the wind."), 4); });
	});
	E.AddFreeform(SoundGroup, TEXT("Go: by the Alderete windmill"), TEXT("23:00, 25 m west of the farm's windmill"), [this, At]
	{
		At(TEXT("farm"), 61, -26, [this] { Tell(true, TEXT("By the farm's windmill. Its head turns and creaks; the dog and the cattle are near."), 4); });
	});

	// ---- the animals ----
	E.AddFreeform(AnimalGroup, TEXT("Quail go up from something behind you"), TEXT("23:00, open land; a covey 14 m ahead, then something 55 m behind: they fly away from it"), [this, At]
	{
		At(TEXT("windmill"), -40, 0, [this] { FString Why; const bool bOk = M->Animals && M->Animals->DevCovey(Why, true); Tell(bOk, Why, 7); });
	});
	E.AddFreeform(AnimalGroup, TEXT("Quail: walk into them"), TEXT("23:00, open land; a covey 14 m ahead and nothing else; walk on, or keep low and slow"), [this, At]
	{
		At(TEXT("windmill"), -40, 0, [this] { FString Why; const bool bOk = M->Animals && M->Animals->DevCovey(Why, false); Tell(bOk, Why, 7); });
	});
	E.AddFreeform(AnimalGroup, TEXT("A coyote stops and stares at something"), TEXT("23:00, open land, lamp on; it crosses 32 m ahead, stares at something 70 m off it, then runs"), [this, At]
	{
		At(TEXT("windmill"), -40, 0, [this] { S47OmenDevLocal::LampOn(M); FString Why; const bool bOk = M->Animals && M->Animals->DevCoyote(Why, true); Tell(bOk, Why, 7); });
	});
	E.AddFreeform(AnimalGroup, TEXT("A coyote crossing in the lamp"), TEXT("23:00, open land, lamp on; nothing near it: its eyes shine back when it looks round"), [this, At]
	{
		At(TEXT("windmill"), -40, 0, [this] { S47OmenDevLocal::LampOn(M); FString Why; const bool bOk = M->Animals && M->Animals->DevCoyote(Why, false); Tell(bOk, Why, 7); });
	});
	E.AddFreeform(AnimalGroup, TEXT("The Alderete dog stops barking"), TEXT("23:00, in the farm yard; it barks at you, then something comes near it"), [this, At]
	{
		At(TEXT("farm"), 38, -27, [this] { FString Why; const bool bOk = M->Animals && M->Animals->DevDog(Why); Tell(bOk, Why, 7); });
	});
	E.AddFreeform(AnimalGroup, TEXT("The cattle walk off together"), TEXT("23:00, west of the pen; something 150 m east of them: all walk the same way, away"), [this, At]
	{
		At(TEXT("farm"), -35, 0, [this]
		{
			if (!M->Omen) return;
			M->Omen->DevPresence(S47OmenDevLocal::Land(M, 205, 240, 2), 0.65, 40, false);
			Tell(true, TEXT("Something stands 150 m east of the pen for 40 seconds."), 5);
		});
	});
	E.AddFreeform(AnimalGroup, TEXT("The cattle run"), TEXT("23:00, west of the pen; something right at them: they scatter and bellow"), [this, At]
	{
		At(TEXT("farm"), -35, 0, [this]
		{
			if (!M->Omen) return;
			M->Omen->DevPresence(S47OmenDevLocal::Land(M, 85, 240, 2), 0.9, 25, false);
			Tell(true, TEXT("Something stands at the east fence of the pen for 25 seconds."), 5);
		});
	});
	E.AddFreeform(AnimalGroup, TEXT("What the animals have done"), TEXT("this run: quail up, with nobody near, coyote stares, barks, a dog gone quiet"), [this]
	{
		if (!M->Animals) { Tell(false, TEXT("Freeform only")); return; }
		const FS47Animals& An = *M->Animals;
		Tell(true, FString::Printf(TEXT("Quail up %d (%d from something), coyote stares %d, barks %d, dogs gone quiet %d; a covey here %d, a coyote out %d"),
			An.Flushes(), An.FlushesFromDanger(), An.Stares(), An.DogBarks(), An.DogStops(), An.CoveyHere() ? 1 : 0, An.CoyoteOut() ? 1 : 0), 7);
	});
}

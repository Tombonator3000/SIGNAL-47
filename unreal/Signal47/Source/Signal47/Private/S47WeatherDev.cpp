#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47Fatigue.h"
#include "S47Flashlight.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Sky.h"
#include "S47Weather.h"
#include "S47WeatherLive.h"
#include "S47Walker.h"

// F10, M5 (WEATHER-20261108, FATIGUE-20261108): the day's weather as the paper has it, each kind of weather now over the day's own
// (out on the open land by the Baca windmill, at an hour it belongs to), the next real sand day or shower, and the hours awake. Nothing
// here is saved; a weather set here lasts a few minutes, and Stop everything gives the day's own back.
void FS47DevFreeform::AddWeather(FS47DevEvents& E)
{
	static const TCHAR* const Group = TEXT("Weather and sleep (M5)");
	const auto Weather = [this]() -> FS47WeatherLive* { FS47FreeformSession* F = Session(); return F ? F->Weather.Get() : nullptr; };
	const auto Body = [this]() -> FS47Fatigue* { FS47FreeformSession* F = Session(); return F ? F->Fatigue.Get() : nullptr; };
	// Out on the land at an hour this weather has, then the weather for this long.
	// (ShowerH: for a shower, how far into its course: S47Weather::ShowerCourse; Up: how far up the view looks, metres in a hundred)
	const auto Out = [this, Weather](double From, double To, FS47WeatherLive::EDev Kind, double Seconds, bool bLamp, const FString& Say,
		double ShowerH = FS47WeatherLive::DevShowerHours, double Up = 21)
	{
		ToLandPlace(TEXT("windmill"), -40, From, To, [this, Weather, Kind, Seconds, bLamp, Say, ShowerH, Up](double, double)
		{
			FS47WeatherLive* Wx = Weather();
			if (!Wx) { Tell(false, TEXT("Freeform only")); return; }
			Wx->Dev(Kind, Seconds, ShowerH);
			if (bLamp && M->Night && M->Night->Flashlight && M->Night->Flashlight->bHave) M->Night->Flashlight->bOn = true;
			// M5 sweep 3, SHOWER-LOOK-20261109: the view turned to where the shower's cloud stands at that point of its course
			// (FS47Sky::CellPlace: beside the land while it rains, downwind after), its cloud, virga and the ground before it in the picture
			if (Kind == FS47WeatherLive::EDev::Shower && M->Player)
				if (const FS47FreeformSession* F = Session())
				{
					const S47Weather::FNow N = S47Weather::Now(F->Seed(), int32(FMath::FloorToDouble(F->Clock() / 86400.0)) + 1, FMath::Fmod(F->Clock(), 86400.0));
					FVector2D Off; double Show = 0;
					FS47Sky::CellPlace(N.DirX, N.DirZ, FMath::Min(ShowerH, 1.6), Off, Show);
					const FVector2D Look = Off.SizeSquared() > 1e-6 ? Off.GetSafeNormal() : FVector2D(-.6, .8);
					AimAt(FVector(M->Player->Pos.X + Look.X * 100, M->Player->FloorY + M->Player->Eye + Up, M->Player->Pos.Z + Look.Y * 100));
				}
			Tell(true, Say, 8);
		});
	};
	E.AddFreeform(Group, TEXT("Today's weather, as the paper has it"), TEXT("the day's own weather in words, and what is on the world now"), [this, Weather]
	{
		FS47FreeformSession* F = Session(); FS47WeatherLive* Wx = Weather();
		if (!F || !Wx) { Tell(false, TEXT("Freeform only")); return; }
		const int32 Cal = int32(FMath::FloorToDouble(F->Clock() / 86400.0)) + 1;
		Tell(true, FString::Printf(TEXT("Day %d: %s  Now: %s"), Cal, *S47Weather::Words(S47Weather::Day(F->Seed(), Cal)), *Wx->Describe()), 12);
	});
	E.AddFreeform(Group, TEXT("Blowing sand, afternoon"), TEXT("16:00, open land by the windmill: sand in the air, the far land brown, the brush whipping, the wind roaring; four minutes"),
		[Out] { Out(16, 18, FS47WeatherLive::EDev::Sand, 240, false, TEXT("Blowing sand for four minutes. Look at the far land, walk, get in the truck and drive: the sand drifts onto the track and the wind pushes the truck.")); });
	E.AddFreeform(Group, TEXT("Blowing sand at night, lamp on"), TEXT("23:00, open land: sand streaming through the lamp's beam and the headlights; the crickets gone under the wind; four minutes"),
		[Out] { Out(23, 4.5, FS47WeatherLive::EDev::Sand, 240, true, TEXT("Sand at night for four minutes: through the lamp and the headlights. Listen: the wind covers the crickets and your steps.")); });
	// SHOWER-LOOK-20261109: the shower runs its own course on the clock from where F10 puts it (about 26 game minutes a minute by day):
	// the rain for a minute or two, then the wet dirt drying over the next hours, as the paper's shower does
	E.AddFreeform(Group, TEXT("A stray shower"), TEXT("17:00, open land, looking at the shower beside the land: its dark cloud and virga, the ground going dark in patches and drops, drops falling past and ticking, thunder far off; it passes and the dirt dries over the next hours (slick while it is wet)"),
		[Out] { Out(17, 19.5, FS47WeatherLive::EDev::Shower, 600, false, TEXT("A stray shower, half an hour into it: the cloud beside you with its virga, the ground darkening, a few drops, thunder far off. It passes in a minute or two; the dirt stays slick while it dries.")); });
	E.AddFreeform(Group, TEXT("A stray shower: just after"), TEXT("17:00, open land, looking after the shower as it goes downwind: the rain over, the ground dark and wet, the dirt slick"),
		[Out] { Out(17, 19.5, FS47WeatherLive::EDev::Shower, 600, false, TEXT("Just after a stray shower: the cloud going off downwind, the ground still dark and wet. Drive on the dirt: less grip, the tyres hiss."), 1.2, 9); });
	E.AddFreeform(Group, TEXT("A stray shower: two hours after"), TEXT("17:00, open land: two hours after the rain, the ground drying, wet still in the low patches and the washes, the rises dry"),
		[Out] { Out(17, 19.5, FS47WeatherLive::EDev::Shower, 400, false, TEXT("Two hours after a stray shower: the ground drying from the rises, still dark in the low patches. Dry in another two hours."), 3.2, -6); });
	E.AddFreeform(Group, TEXT("Morning haze"), TEXT("06:30, open land: the low haze, the far land pale; four minutes"),
		[Out] { Out(6.5, 8.5, FS47WeatherLive::EDev::Haze, 240, false, TEXT("Haze for four minutes, as on a still morning or the morning after a shower.")); });
	E.AddFreeform(Group, TEXT("The day's own weather again"), TEXT("whatever F10 set is gone; in a developer run the day's weather stays on the world until the run ends"), [this, Weather]
	{
		FS47WeatherLive* Wx = Weather();
		if (!Wx) { Tell(false, TEXT("Freeform only")); return; }
		Wx->Dev(FS47WeatherLive::EDev::None, 0); Wx->DevLive(true);
		Tell(true, TEXT("The day's own weather, as the paper has it."), 5);
	});
	// The next real one: the clock goes on to it, and the day's own weather shows it, as the paper prints it.
	const auto Next = [this, Weather](bool bSand)
	{
		FS47FreeformSession* F = Session(); FS47WeatherLive* Wx = Weather();
		if (!F || !Wx) { Tell(false, TEXT("Freeform only")); return; }
		const int32 Today = int32(FMath::FloorToDouble(F->Clock() / 86400.0)) + 1;
		for (int32 Day = Today; Day < Today + 40; ++Day)
		{
			const S47Weather::FDay D = S47Weather::Day(F->Seed(), Day);
			if (bSand ? !D.bDust : !D.bShower) continue;
			const double Hour = bSand ? 16.0 : D.ShowerAt + 0.1;
			const double Target = (Day - 1) * 86400.0 + Hour * 3600.0;
			if (Target <= F->Clock() + 60) continue;
			ClearRunning();
			Wx->Dev(FS47WeatherLive::EDev::None, 0); Wx->DevLive(true);
			F->TestAdvance(Target - F->Clock());
			Tell(true, FString::Printf(TEXT("Day %d at %02d:%02d, the day's own weather. The paper says: %s"), Day, int32(Hour), int32(FMath::Fmod(Hour, 1.0) * 60), *S47Weather::Words(D)), 12);
			return;
		}
		Tell(false, bSand ? TEXT("no sand day in the next forty days of this game") : TEXT("no shower in the next forty days of this game"));
	};
	E.AddFreeform(Group, TEXT("Go to the next sand day"), TEXT("the clock goes on to 16:00 on the next day of this game that has blowing sand; the weather is the day's own"), [Next] { Next(true); });
	E.AddFreeform(Group, TEXT("Go to the next shower"), TEXT("the clock goes on to the next stray shower of this game; the weather is the day's own"), [Next] { Next(false); });
	// ---- the hours awake ----
	const auto Awake = [this, Body](double Hours, const TCHAR* Say)
	{
		FS47Fatigue* B = Body();
		if (!B) { Tell(false, TEXT("Freeform only")); return; }
		B->DevSet(Hours);
		Tell(true, FString::Printf(TEXT("%s (%s)"), Say, *B->Describe()), 7);
	};
	E.AddFreeform(Group, TEXT("Tired: twenty hours awake"), TEXT("now and then the eyelids grow heavy for a few seconds, with a yawn; the strain comes faster; a run winds you and eases off"), [Awake] { Awake(20, TEXT("Twenty hours awake.")); });
	E.AddFreeform(Group, TEXT("Worn out: twenty-eight hours awake"), TEXT("as tired as it gets: heavy spells more often, the eyes shutting a moment in them; winded, the breath is heard; coffee at the pot or the Mesa helps"), [Awake] { Awake(28, TEXT("Twenty-eight hours awake.")); });
	E.AddFreeform(Group, TEXT("Rested"), TEXT("felt in this run too: nothing shows"), [Awake] { Awake(0, TEXT("Rested.")); });
	E.AddFreeform(Group, TEXT("The hours awake as the game has them"), TEXT("what F10 set is gone; a developer run feels none of it"), [this, Body]
	{
		FS47Fatigue* B = Body();
		if (!B) { Tell(false, TEXT("Freeform only")); return; }
		B->DevSet(-1);
		Tell(true, B->Describe(), 6);
	});
	E.AddFreeform(Group, TEXT("What the body says now"), TEXT("hours awake, coffee, how tired, the run's wind, and what it does to the run and the strain"), [this, Body]
	{
		FS47Fatigue* B = Body();
		if (!B) { Tell(false, TEXT("Freeform only")); return; }
		Tell(true, B->Describe(), 8);
	});
}

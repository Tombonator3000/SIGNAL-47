#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47EventPlaces.h"
#include "S47FieldLife.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47NightLog.h"
#include "S47OpenLand.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "SS47FieldMap.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47FieldMapDev, Log, All);

// F10, M3 (MAP-20261108, NIGHTLOG-20261108): the survey sheet's accuracy, place by place, and the night's typed log. Nothing here is
// saved. -S47DevSweep=/tmp/dir -S47DevSweepGroup="Map and night log (M3)" runs the group in turn, a picture after each line.
//
// The probe: stand on a place from the catalogue (S47EventPlaces), in the area that owns its ground, facing a set way; then the red
// arrow must be within 3 m of the place and its heading within 2 degrees of the way the walker faces, worked out the other way round
// (S47OpenLand::Turned, not the sheet's own two-point sum). Every area's terms are crossed: SARO's, the road's, the diner's and
// STATION 01's, which are turned a quarter round.
namespace S47FieldMapDevLocal
{
	constexpr double ProbeMetres = 3, ProbeDegrees = 2;
	const TCHAR* const MapGroup = TEXT("Map and night log (M3)");
	struct FProbeLine { const TCHAR* Id; double Turns; };
	// A place in each area and each kind of ground, facing north, east, south and west in turn.
	const FProbeLine ProbeLines[] = {
		{ TEXT("saro"), 0 }, { TEXT("saro_yard"), 1 }, { TEXT("array"), 2 }, { TEXT("farm"), 3 }, { TEXT("oldhouse"), 0 }, { TEXT("motel"), 1 },
		{ TEXT("diner"), 2 }, { TEXT("gas"), 3 }, { TEXT("church"), 0 }, { TEXT("cinema"), 1 }, { TEXT("station01"), 2 }, { TEXT("windmill"), 3 },
		{ TEXT("road:pullout_north"), 0 }, { TEXT("land:wash_west"), 1 } };
}

void FS47DevFreeform::AddMapLog(FS47DevEvents& E)
{
	using namespace S47FieldMapDevLocal;
	// On the place, in its own area, facing Turns quarter turns east of north on the land; then Then with what the sheet should say.
	const auto Stand = [this](const FString& Id, double Turns, TFunction<void(const FVector2D&, double)> Then)
	{
		FS47FreeformSession* Fs = Session();
		const FS47EventPlace* Pl = Fs && Fs->EventPlaces ? Fs->EventPlaces->Find(Id) : nullptr;
		if (!Pl) { Tell(false, FString::Printf(TEXT("%s is not on the list of places"), *Id)); return; }
		FString Area = FS47EventPlaces::AreaOf(*Pl);
		// On foot in Caliche is the diner's area, as the game has it.
		if (Area == TEXT("road") && (Pl->Region == TEXT("caliche") || Pl->Region == TEXT("diner"))) Area = TEXT("diner");
		const FVector At = S47OpenLand::FromRoad(Area, Pl->X, Pl->Z);
		// Facing Turns quarter turns east of north on the land, in the area's own yaw.
		const double RoadYaw = -Turns * HALF_PI;
		FStage Stage; Stage.Area = Area; Stage.Where = [At] { return FVector2D(At.X, At.Z); };
		Stage.Yaw = S47OpenLand::Turned(TEXT("road"), Area, RoadYaw);
		const FVector2D Want(Pl->X, Pl->Z);
		const double WantHeading = FMath::Fmod(Turns * 90.0 + 360.0, 360.0);
		Go(Stage, [Then, Want, WantHeading] { Then(Want, WantHeading); });
	};
	// What the sheet says against what it should. True: within 3 m and 2 degrees.
	const auto Gauge = [this](const FString& Id, const FVector2D& Want, double WantHeading, FString& Said) -> bool
	{
		double X = 0, Z = 0, Hd = 0; FString Area; bool bDriving = false;
		const bool bOn = S47FieldMap::Me(M, X, Z, Hd, Area, bDriving);
		const double Off = bOn ? FVector2D::Distance(FVector2D(X, Z), Want) : -1;
		const double Turn = bOn ? FMath::Abs(FMath::Fmod(Hd - WantHeading + 540.0, 360.0) - 180.0) : 999;
		const bool bOk = bOn && Off <= ProbeMetres && Turn <= ProbeDegrees;
		const FVector Stood = M && M->Player ? M->Player->Pos : FVector::ZeroVector;
		UE_LOG(LogS47FieldMapDev, Display, TEXT("S47: map probe %s %s: area %s, standing %.1f %.1f there, place %.1f %.1f, arrow %.1f %.1f, off %.2f m, heading %.1f for %.1f"),
			*Id, bOk ? TEXT("PASS") : TEXT("FAIL"), *Area, Stood.X, Stood.Z, Want.X, Want.Y, X, Z, Off, Hd, WantHeading);
		Said = FString::Printf(TEXT("Map at %s: the arrow is %.1f m off, heading %03.0f for %03.0f"), *Id, Off, Hd, WantHeading);
		return bOk;
	};
	const auto Sheet = [this](double Scale)
	{
		S47FieldMap::DevScale(Scale);
		if (M && M->UI) { M->UI->Close(true); M->UI->NotebookAt(TEXT("map")); }
		if (M && M->Flow) M->Flow->OpenNotebook();
	};
	for (const FProbeLine& Line : ProbeLines)
	{
		const FString Id = Line.Id;
		const double Turns = Line.Turns;
		E.AddFreeform(MapGroup, FString::Printf(TEXT("Map: stand at %s, the sheet close up"), Line.Id), TEXT("puts you on the place, checks the red arrow against it (3 m, 2 degrees) and opens the map on you"), [this, Id, Turns, Stand, Gauge, Sheet]
		{
			Stand(Id, Turns, [this, Id, Gauge, Sheet](const FVector2D& Want, double WantHeading)
			{
				FString Said;
				const bool bOk = Gauge(Id, Want, WantHeading, Said);
				Sheet(3.5);
				Tell(bOk, Said);
			});
		});
	}
	E.AddFreeform(MapGroup, TEXT("Map: every place on the list, one after another (log)"), TEXT("each place in the catalogue in turn, the arrow checked at each; one line in the log at the end"), [this, Stand, Gauge]
	{
		const TArray<FString> Every = FS47EventPlaces::Ids();
		const TSharedRef<int32> Good = MakeShared<int32>(0);
		const TSharedRef<TArray<FString>> Bad = MakeShared<TArray<FString>>();
		const TSharedRef<TFunction<void(int32)>> Next = MakeShared<TFunction<void(int32)>>();
		*Next = [this, Every, Good, Bad, Next, Stand, Gauge](int32 I)
		{
			if (I >= Every.Num())
			{
				UE_LOG(LogS47FieldMapDev, Display, TEXT("S47: map probes: %d of %d within %.0f m and %.0f degrees%s%s"), *Good, Every.Num(), ProbeMetres, ProbeDegrees, Bad->Num() ? TEXT("; off: ") : TEXT(""), *FString::Join(*Bad, TEXT(", ")));
				Tell(*Good == Every.Num(), FString::Printf(TEXT("Map: %d of %d places within %.0f m"), *Good, Every.Num(), ProbeMetres));
				// Let go of itself, once it is no longer running.
				After(0, [Next] { *Next = nullptr; });
				return;
			}
			const FString Id = Every[I];
			Stand(Id, double(I % 4), [this, Id, I, Good, Bad, Next, Gauge](const FVector2D& Want, double WantHeading)
			{
				FString Said;
				if (Gauge(Id, Want, WantHeading, Said)) ++*Good; else Bad->Add(Id);
				After(.2, [Next, I] { if (*Next) (*Next)(I + 1); });
			});
		};
		(*Next)(0);
	});
	E.AddFreeform(MapGroup, TEXT("Map: SARO, all the way in"), TEXT("on the station's yard, the sheet at its closest"), [this, Stand, Sheet]
	{
		Stand(TEXT("saro_yard"), 1, [this, Sheet](const FVector2D&, double) { Sheet(9); Tell(true, TEXT("Map at SARO, all the way in")); });
	});
	E.AddFreeform(MapGroup, TEXT("Map: SARO, all the way out"), TEXT("on the station's yard, the whole sheet"), [this, Stand, Sheet]
	{
		Stand(TEXT("saro_yard"), 1, [this, Sheet](const FVector2D&, double) { Sheet(.1); Tell(true, TEXT("Map at SARO, all the way out")); });
	});
	E.AddFreeform(MapGroup, TEXT("Map: the whole sheet"), TEXT("the map zoomed all the way out, on you"), [Sheet] { Sheet(.1); });
	E.AddFreeform(MapGroup, TEXT("Map: the station, the farm and the motel at once"), TEXT("from the county road turnoff, the scale a driver reads"), [this, Stand, Sheet]
	{
		Stand(TEXT("road:turnoff"), 2, [this, Sheet](const FVector2D&, double) { Sheet(.25); Tell(true, TEXT("Map at about a mile and a half across")); });
	});

	// ---- the night's log ----
	E.AddFreeform(MapGroup, TEXT("Night log: a night with things in it, through to 06:00"), TEXT("22:00 at the yard: a light seen, a flat tire, 95 minutes nobody can account for, a rest to six; then the sheet"), [this]
	{
		// A night of its own: the next day's, so that no line before has typed it already.
		FStage Stage; Stage.From = 22; Stage.To = 23.5; Stage.Area = TEXT("saro"); Stage.Day = Session() ? Session()->Day() + 1 : 0;
		Stage.Where = [] { const FVector At = S47OpenLand::FromRoad(TEXT("saro"), 36, -312); return FVector2D(At.X, At.Z); };
		Go(Stage, [this]
		{
			FS47FreeformSession* Fs = Session();
			if (!Fs || !Fs->NightLog) { Tell(false, TEXT("no night log in this run")); return; }
			// The sheet keeps no first shift; a developer run may still be in it.
			Fs->TestEndFirstShift();
			After(.4, [this]
			{
				FS47FreeformSession* Fs1 = Session(); if (!Fs1) return;
				Fs1->RecordEvidence(FString::Printf(TEXT("nightlog_check_d%d"), Fs1->Day()), TEXT("At 22:41 a light stood low over the flat north of the station for about a minute, then went out. Nothing on the radio."));
				if (Fs1->Care) Fs1->Care->TestFlat();
				After(.4, [this]
				{
					FS47FreeformSession* Fs2 = Session(); if (!Fs2 || !Fs2->NightLog) return;
					Fs2->NightLog->DevLostTime(95);
					After(.4, [this]
					{
						FS47FreeformSession* Fs3 = Session(); if (!Fs3 || !Fs3->NightLog) return;
						const double Six = FS47NightLog::SixOf(FS47FreeformSession::NightOf(Fs3->Clock()));
						Fs3->TestAdvance(FMath::Max(60.0, Six + 120 - Fs3->Clock()));
						After(.6, [this]
						{
							FS47FreeformSession* Fs4 = Session(); if (!Fs4 || !Fs4->NightLog) return;
							if (Fs4->Care) Fs4->Care->Clear();
							const bool bOk = Fs4->NightLog->DevOpenLast();
							Tell(bOk, bOk ? TEXT("The 06:00 night log after a night with things in it") : TEXT("no sheet was typed at 06:00"));
						});
					});
				});
			});
		});
	});
	E.AddFreeform(MapGroup, TEXT("Night log: type tonight's sheet now"), TEXT("night if it is not; the sheet with what tonight has so far, opened"), [this]
	{
		FStage Stage; Stage.From = 23; Stage.To = 5.5;
		Go(Stage, [this]
		{
			FS47FreeformSession* Fs = Session();
			const bool bOk = Fs && Fs->NightLog && Fs->NightLog->DevTypeNow();
			Tell(bOk, bOk ? TEXT("Tonight's night log, typed now") : TEXT("no night log in this run"));
		});
	});
	E.AddFreeform(MapGroup, TEXT("Night log: telex the last sheet to Dr. Ward, on to 08:40"), TEXT("sent from OPS, initialed by her office after it opens; the sheet with her line on it"), [this]
	{
		FS47FreeformSession* Fs = Session();
		if (!Fs || !Fs->NightLog || Fs->NightLog->Sheets().IsEmpty()) { Tell(false, TEXT("no night log typed yet: run one of the lines above first")); return; }
		Fs->NightLog->Send(Fs->NightLog->Sheets().Last().Night);
		FStage Stage; Stage.From = 8.67; Stage.To = 12;
		Go(Stage, [this]
		{
			After(.6, [this]
			{
				FS47FreeformSession* Fs1 = Session();
				const bool bOk = Fs1 && Fs1->NightLog && Fs1->NightLog->Sheets().Num() && Fs1->NightLog->Sheets().Last().Read >= 0 && Fs1->NightLog->DevOpenLast();
				Tell(bOk, bOk ? FString::Printf(TEXT("Initialed by Dr. Ward's office: %d"), Fs1->NightLog->ReadCount()) : FString(TEXT("not initialed")));
			});
		});
	});
}

#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47EventPlaces.h"
#include "S47FieldLife.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Main.h"
#include "S47NightLog.h"
#include "S47Keys.h"
#include "S47FarmWorld.h"
#include "S47Omen.h"
#include "S47OpenLand.h"
#include "S47Story.h"
#include "S47Tape.h"
#include "S47Places.h"
#include "S47PinBoard.h"
#include "S47BoardRules.h"
#include "S47Docs.h"
#include "S47RoadShape.h"
#include "S47World.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "SS47FieldMap.h"
#include "Containers/Ticker.h"
#include "UnrealClient.h"

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
	// The sweep's own picture is the world without the UI: a sweep also takes one with it, once the sheet has been drawn.
	// Delay: after a line's last step the sweep moves on in half a second, so a picture taken as it ends is taken sooner.
	const auto Pictured = [this](const TCHAR* Kind, float Delay = 1.2f)
	{
		if (SweepDir.IsEmpty()) return;
		const FString Path = SweepDir / FString::Printf(TEXT("%s-%03d.png"), Kind, SweepIndex);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Path](float) { FScreenshotRequest::RequestScreenshot(Path, true, false); return false; }), Delay);
	};
	const auto Sheet = [this, Pictured](double Scale)
	{
		S47FieldMap::DevScale(Scale);
		if (M && M->UI) { M->UI->Close(true); M->UI->NotebookAt(TEXT("map")); }
		if (M && M->Flow) M->Flow->OpenNotebook();
		Pictured(TEXT("map"));
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
	E.AddFreeform(MapGroup, TEXT("Map: at the wheel, southbound on the highway"), TEXT("a free drive from the road's start, the truck driving itself; the arrow must be the cab, in the southbound lane, heading south"), [this, Sheet]
	{
		if (!M || !M->World) { Tell(false, TEXT("no world")); return; }
		M->World->FreeDrive(TEXT("road"));
		WaitFor([this] { return M && M->World && M->World->bDriving && M->World->CabTruck(); }, 8, [this, Sheet]
		{
			M->World->Autopilot = 14.0;
			After(2.5, [this, Sheet]
			{
				double X = 0, Z = 0, Hd = 0; FString Area; bool bDriving = false;
				const bool bOn = S47FieldMap::Me(M, X, Z, Hd, Area, bDriving);
				// The road's own lanes: the highway runs north and south along x 0, the southbound lane at x -1.8 (S47RoadShape::START).
				const double Lane = FMath::Abs(X - S47RoadShape::START.X), Turn = FMath::Abs(FMath::Fmod(Hd - 180.0 + 540.0, 360.0) - 180.0);
				const bool bOk = bOn && bDriving && Lane <= 2.5 && Turn <= 10;
				UE_LOG(LogS47FieldMapDev, Display, TEXT("S47: map probe driving %s: area %s, arrow %.1f %.1f, %.1f m off the southbound lane, heading %.1f (south 180)"), bOk ? TEXT("PASS") : TEXT("FAIL"), *Area, X, Z, Lane, Hd);
				if (M->World) M->World->Autopilot.Reset();
				Sheet(3);
				Tell(bOk, FString::Printf(TEXT("Map at the wheel: %.1f m off the southbound lane, heading %03.0f"), Lane, Hd));
			});
		}, [this] { Tell(false, TEXT("the free drive did not start")); });
	});
	E.AddFreeform(MapGroup, TEXT("Map: a pencil mark of your own, by the keys"), TEXT("at the windmill the sheet opens, X is pressed as from the keyboard: a numbered mark goes down where the sheet is centred, and is kept"), [this, Stand, Sheet]
	{
		Stand(TEXT("windmill"), 1, [this, Sheet](const FVector2D&, double)
		{
			FS47FreeformSession* Fs = Session();
			const int32 Before = Fs && Fs->Places ? Fs->Places->MapMarks().Num() : -1;
			Sheet(4);
			// The notebook holds the menu's clock while it is open: the keys and the count go on the core ticker.
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float) { S47Keys::TestPresses(TEXT("X"), .25f); return false; }), .5f);
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this, Before](float)
			{
				FS47FreeformSession* Fs1 = Session();
				const int32 After1 = Fs1 && Fs1->Places ? Fs1->Places->MapMarks().Num() : -1;
				UE_LOG(LogS47FieldMapDev, Display, TEXT("S47: map mark by the keys %s: %d marks before, %d after"), After1 == Before + 1 ? TEXT("PASS") : TEXT("FAIL"), Before, After1);
				Tell(After1 == Before + 1, FString::Printf(TEXT("Map mark by the keys: %d before, %d after"), Before, After1));
				return false;
			}), 1.6f);
		});
	});
	// Work and leads on the sheet (Tom, 8 November): a dish down, the feeder out, places heard of and one looked into, then the sheet.
	const auto Leads = [this]
	{
		FS47FreeformSession* Fs = Session(); if (!Fs) return;
		if (Fs->Field && !Fs->Field->HasArrayFault()) Fs->Field->TriggerServiceFault(false, TEXT("S-05"));
		Fs->SetPowerFault(true);
		if (Fs->Places) { Fs->Places->Hear(TEXT("windmill")); Fs->Places->Hear(TEXT("tank")); Fs->Places->Hear(TEXT("cairn")); }
		if (!Fs->HasFinding(TEXT("place_cairn"))) Fs->RecordEvidence(TEXT("place_cairn"), TEXT("A stack of stones with a post in the top, the kind a survey party leaves."));
	};
	E.AddFreeform(MapGroup, TEXT("Map: work and leads written in, close up at SARO"), TEXT("a dish down, the feeder out, places heard of; the dishes' numbers; the sheet close in at the array"), [this, Stand, Sheet, Leads]
	{
		Leads();
		Stand(TEXT("saro_yard"), 0, [this, Sheet](const FVector2D&, double) { Sheet(1.1); Tell(true, TEXT("Map: work and leads, close up")); });
	});
	E.AddFreeform(MapGroup, TEXT("Map: work and leads written in, the land around"), TEXT("the same, at the scale a driver reads"), [this, Stand, Sheet, Leads]
	{
		Leads();
		Stand(TEXT("saro_yard"), 0, [this, Sheet](const FVector2D&, double) { Sheet(.22); Tell(true, TEXT("Map: work and leads, the land around")); });
	});
	E.AddFreeform(MapGroup, TEXT("Map: the whole sheet"), TEXT("the map zoomed all the way out, on you"), [Sheet] { Sheet(.1); });
	E.AddFreeform(MapGroup, TEXT("Map: the station, the farm and the motel at once"), TEXT("from the county road turnoff, the scale a driver reads"), [this, Stand, Sheet]
	{
		Stand(TEXT("road:turnoff"), 2, [this, Sheet](const FVector2D&, double) { Sheet(.25); Tell(true, TEXT("Map at about a mile and a half across")); });
	});

	// ---- the field recorder (TAPE-20261108) ----
	static const TCHAR* const TapeGroup = TEXT("Tape (M3)");
	// In the control room by the desk, facing it.
	const auto AtDesk = [this](TFunction<void()> Then)
	{
		FStage Stage; Stage.Area = TEXT("saro"); Stage.Where = [] { return FVector2D(-2.2, -2.6); }; Stage.Yaw = 0.9;
		Go(Stage, MoveTemp(Then));
	};
	E.AddFreeform(TapeGroup, TEXT("Tape: record at the windmill with something out there, play it back on the desk"), TEXT("23:00 at the windmill with the recorder; something stands 170 m off, too far to feel; 14 s on tape; then on the desk at SARO it is there"), [this, AtDesk, Pictured]
	{
		FS47FreeformSession* Fs = Session();
		const FS47EventPlace* Mill = Fs && Fs->EventPlaces ? Fs->EventPlaces->Find(TEXT("windmill")) : nullptr;
		// (The first line of a sweep starts on the first frame, before the omen exists: it is asked for when the place is reached.)
		if (!Fs || !Fs->Tape || !Mill || !M) { Tell(false, TEXT("no recorder or windmill in this run")); return; }
		Fs->Tape->DevGive();
		const FString Area = FS47EventPlaces::AreaOf(*Mill);
		const FVector Stood = S47OpenLand::FromRoad(Area, Mill->X, Mill->Z);
		FVector Far = S47OpenLand::FromRoad(Area, Mill->X + 170, Mill->Z);
		Far.Y = S47FarmWorld::FloorRoad(Mill->X + 170, Mill->Z) + 1.5;
		FStage Stage; Stage.From = 23; Stage.To = 4.5; Stage.Area = Area; Stage.Where = [Stood] { return FVector2D(Stood.X, Stood.Z); };
		Go(Stage, [this, Far, AtDesk, Pictured]
		{
			FS47FreeformSession* Fs1 = Session(); if (!Fs1 || !Fs1->Tape || !M) return;
			if (!M->Omen) { Tell(false, TEXT("no omen in this run")); return; }
			M->Omen->DevPresence(Far, .6, 40, false);
			Fs1->Tape->ToggleRecord();
			After(14, [this, AtDesk, Pictured]
			{
				FS47FreeformSession* Fs2 = Session(); if (!Fs2 || !Fs2->Tape) return;
				Fs2->Tape->ToggleRecord();
				if (M && M->Omen) M->Omen->DevClear();
				const bool bCaught = Fs2->Tape->Takes().Num() && !Fs2->Tape->Takes().Last().Odd.IsEmpty();
				AtDesk([this, bCaught, Pictured]
				{
					FS47FreeformSession* Fs3 = Session(); if (!Fs3 || !Fs3->Tape || Fs3->Tape->Takes().IsEmpty()) { Tell(false, TEXT("no tape was made")); return; }
					Fs3->Tape->UseDesk(); // the recorder back on the desk
					Fs3->Tape->PlayTake(Fs3->Tape->Takes().Num() - 1);
					const float Len = Fs3->Tape->Takes().Last().Seconds;
					After(Len + 1.5, [this, bCaught, Pictured]
					{
						FS47FreeformSession* Fs4 = Session(); if (!Fs4 || !Fs4->Tape || Fs4->Tape->Takes().IsEmpty()) return;
						const FS47Tape::FTake& Tk = Fs4->Tape->Takes().Last();
						const bool bOk = bCaught && Tk.bOddHeard && Fs4->HasFinding(FString::Printf(TEXT("tape_%d"), Tk.N));
						if (M && M->Story && M->UI) for (const FS47Doc& D : M->Story->Docs()) if (D.Id == FString::Printf(TEXT("freeform-tape_%d"), Tk.N)) { M->UI->Document(D); break; }
						Pictured(TEXT("tape"), .3f);
						Tell(bOk, FString::Printf(TEXT("Tape %d: %d sounds, %s at %.1f s, %s"), Tk.N, Tk.Sounds.Num(), Tk.Odd.IsEmpty() ? TEXT("nothing else") : *Tk.Odd, Tk.OddAt, Tk.bOddHeard ? TEXT("heard on the desk and kept") : TEXT("not heard")));
					});
				});
			});
		});
	});
	E.AddFreeform(TapeGroup, TEXT("Tape: the desk at SARO"), TEXT("the field recorder's panel, with its cassettes"), [this, AtDesk, Pictured]
	{
		AtDesk([this, Pictured]
		{
			FS47FreeformSession* Fs = Session(); if (!Fs || !Fs->Tape) { Tell(false, TEXT("no recorder in this run")); return; }
			if (Fs->Tape->Carried()) Fs->Tape->UseDesk();
			Fs->Tape->UseDesk();
			Pictured(TEXT("tape"));
			Tell(true, FString::Printf(TEXT("The desk: %d cassettes"), Fs->Tape->Takes().Num()));
		});
	});

	// ---- the case board (CASEBOARD-20261108) ----
	static const TCHAR* const BoardGroup = TEXT("Case board (M3)");
	E.AddFreeform(BoardGroup, TEXT("Case board: field records brought back and put up, with threads"), TEXT("two records at the windmill, one behind the church on the same line of inquiry, a Ledger letter naming the windmill pinned from the tray; back at SARO they go up; one thread of your own; the board"), [this, Stand, AtDesk, Pictured]
	{
		Stand(TEXT("windmill"), 3, [this, Stand, AtDesk, Pictured](const FVector2D&, double)
		{
			FS47FreeformSession* Fs = Session(); if (!Fs || !Fs->PinBoard) { Tell(false, TEXT("no case board in this run")); return; }
			const int32 Dn = Fs->Day();
			Fs->RecordEvidence(FString::Printf(TEXT("observer.day%d"), Dn), TEXT("F10: small prints in a ring by the windmill, nothing leading in or out."));
			After(.4, [this, Stand, AtDesk, Pictured, Dn]
			{
				FS47FreeformSession* Fs1 = Session(); if (!Fs1) return;
				Fs1->RecordEvidence(FString::Printf(TEXT("pinboard_mill_d%d"), Dn), TEXT("F10: the brake wire on the windmill, cut and twisted back on itself."));
				Stand(TEXT("church"), 0, [this, AtDesk, Pictured, Dn](const FVector2D&, double)
				{
					FS47FreeformSession* Fs2 = Session(); if (!Fs2 || !M || !M->Story) return;
					Fs2->RecordEvidence(FString::Printf(TEXT("landing_trace.day%d"), Dn), TEXT("F10: a ring of pressed-down ground behind the church, thirty feet across."));
					FS47Doc Paper; Paper.Id = FString::Printf(TEXT("dev-ledger-d%d"), Dn); Paper.Kind = TEXT("typed"); Paper.Stamp = TEXT("THE LEDGER");
					Paper.Title = TEXT("The Caliche County Ledger, letters");
					Paper.Page = TEXT("LIGHTS AGAIN\nEditor: What sat over the windmill on the Baca lease the other night was not a plane. I would like somebody to tell me what it wasn't. / Baca lease");
					Paper.Transcript = Paper.Page;
					M->Story->AddDoc(Paper);
					const bool bClip = Fs2->PinBoard && Fs2->PinBoard->DevClip(Paper.Id);
					AtDesk([this, Pictured, Dn, bClip]
					{
						After(1.6, [this, Pictured, Dn, bClip]
						{
							FS47FreeformSession* Fs3 = Session(); if (!Fs3 || !Fs3->PinBoard || !M) return;
							FS47PinBoard& Pb = *Fs3->PinBoard;
							const FString Mine = FString::Printf(TEXT("ff:f:pinboard_mill_d%d"), Dn), Church = FString::Printf(TEXT("ff:f:landing_trace.day%d"), Dn);
							if (Pb.Book()) Pb.Book()->Connect(Mine, Church, false); // a second time would untie it
							const int32 Station = Pb.StationThreads().Num();
							if (M->Board.Open) M->Board.Open(FString(), FString());
							Pictured(TEXT("board"), 1.5f);
							const bool bOk = Pb.AtSaro() && Pb.UpCount() >= 4 && Station >= 2 && Pb.ThreadCount() >= 1 && bClip;
							Tell(bOk, FString::Printf(TEXT("On the board: %d up, %d station threads, %d of your own, clipping %s"), Pb.UpCount(), Station, Pb.ThreadCount(), bClip ? TEXT("up") : TEXT("refused")));
						});
					});
				});
			});
		});
	});
	E.AddFreeform(BoardGroup, TEXT("Case board: the tray of papers"), TEXT("the papers that are not field material, by the board; one names a place and can be pinned"), [this, AtDesk, Pictured]
	{
		AtDesk([this, Pictured]
		{
			FS47FreeformSession* Fs = Session(); if (!Fs || !Fs->PinBoard) { Tell(false, TEXT("no case board in this run")); return; }
			Fs->PinBoard->OpenTray();
			Pictured(TEXT("board"), 1.f);
			Tell(true, FString::Printf(TEXT("The tray: %d papers"), Fs->PinBoard->Papers().Num()));
		});
	});

	// ---- the night's log ----
	E.AddFreeform(MapGroup, TEXT("Night log: a night with things in it, through to 06:00"), TEXT("22:00 at the yard: a light seen, a flat tire, 95 minutes nobody can account for, a rest to six; then the sheet"), [this, Pictured]
	{
		// A night of its own: the next day's, so that no line before has typed it already.
		FStage Stage; Stage.From = 22; Stage.To = 23.5; Stage.Area = TEXT("saro"); Stage.Day = Session() ? Session()->Day() + 1 : 0;
		Stage.Where = [] { const FVector At = S47OpenLand::FromRoad(TEXT("saro"), 36, -312); return FVector2D(At.X, At.Z); };
		Go(Stage, [this, Pictured]
		{
			FS47FreeformSession* Fs = Session();
			if (!Fs || !Fs->NightLog) { Tell(false, TEXT("no night log in this run")); return; }
			// The sheet keeps no first shift; a developer run may still be in it.
			Fs->TestEndFirstShift();
			After(.4, [this, Pictured]
			{
				FS47FreeformSession* Fs1 = Session(); if (!Fs1) return;
				Fs1->RecordEvidence(FString::Printf(TEXT("nightlog_check_d%d"), Fs1->Day()), TEXT("At 22:41 a light stood low over the flat north of the station for about a minute, then went out. Nothing on the radio."));
				if (Fs1->Care) Fs1->Care->TestFlat();
				After(.4, [this, Pictured]
				{
					FS47FreeformSession* Fs2 = Session(); if (!Fs2 || !Fs2->NightLog) return;
					Fs2->NightLog->DevLostTime(95);
					After(.4, [this, Pictured]
					{
						FS47FreeformSession* Fs3 = Session(); if (!Fs3 || !Fs3->NightLog) return;
						const double Six = FS47NightLog::SixOf(FS47FreeformSession::NightOf(Fs3->Clock()));
						Fs3->TestAdvance(FMath::Max(60.0, Six + 120 - Fs3->Clock()));
						After(.6, [this, Pictured]
						{
							FS47FreeformSession* Fs4 = Session(); if (!Fs4 || !Fs4->NightLog) return;
							if (Fs4->Care) Fs4->Care->Clear();
							const bool bOk = Fs4->NightLog->DevOpenLast();
							Pictured(TEXT("log"));
							Tell(bOk, bOk ? TEXT("The 06:00 night log after a night with things in it") : TEXT("no sheet was typed at 06:00"));
						});
					});
				});
			});
		});
	});
	E.AddFreeform(MapGroup, TEXT("Night log: type tonight's sheet now"), TEXT("night if it is not; the sheet with what tonight has so far, opened"), [this, Pictured]
	{
		FStage Stage; Stage.From = 23; Stage.To = 5.5;
		Go(Stage, [this, Pictured]
		{
			FS47FreeformSession* Fs = Session();
			const bool bOk = Fs && Fs->NightLog && Fs->NightLog->DevTypeNow();
			Pictured(TEXT("log"));
			Tell(bOk, bOk ? TEXT("Tonight's night log, typed now") : TEXT("no night log in this run"));
		});
	});
	E.AddFreeform(MapGroup, TEXT("Night log: telex the last sheet to Dr. Ward, on to 08:40"), TEXT("sent from OPS, initialed by her office after it opens; the sheet with her line on it"), [this, Pictured]
	{
		FS47FreeformSession* Fs = Session();
		if (!Fs || !Fs->NightLog || Fs->NightLog->Sheets().IsEmpty()) { Tell(false, TEXT("no night log typed yet: run one of the lines above first")); return; }
		Fs->NightLog->Send(Fs->NightLog->Sheets().Last().Night);
		FStage Stage; Stage.From = 8.67; Stage.To = 12;
		Go(Stage, [this, Pictured]
		{
			After(.6, [this, Pictured]
			{
				FS47FreeformSession* Fs1 = Session();
				const bool bOk = Fs1 && Fs1->NightLog && Fs1->NightLog->Sheets().Num() && Fs1->NightLog->Sheets().Last().Read >= 0 && Fs1->NightLog->DevOpenLast();
				Pictured(TEXT("log"));
				Tell(bOk, bOk ? FString::Printf(TEXT("Initialed by Dr. Ward's office: %d"), Fs1->NightLog->ReadCount()) : FString(TEXT("not initialed")));
			});
		});
	});
}

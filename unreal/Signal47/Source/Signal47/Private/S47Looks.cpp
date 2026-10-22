// story/looks.ts
#include "S47Looks.h"

#include "S47Inter.h"
#include "S47Web.h"

namespace
{
	/** String(((Math.round(n) % 360) + 360) % 360).padStart(3, '0') */
	FString Pad(double N)
	{
		const int32 R = int32(FMath::FloorToDouble(N + 0.5));
		return FString::Printf(TEXT("%03d"), ((R % 360) + 360) % 360);
	}

	/** A few things said about an antenna, chosen by its number. */
	const TCHAR* DISH[] = {
		TEXT("A white dish on a concrete pedestal, the red lamp blinking on the feed legs."),
		TEXT("Rust runs down the pedestal from every bolt. Two of the reflector panels are newer than the rest."),
		TEXT("The drive motors tick as it tracks. There is frost on the lower rim."),
		TEXT("A ladder up the pedestal, chained at the bottom. Its red lamp blinks out of step with the next one."),
		TEXT("Bird droppings down one side of the feed housing. The number is stencilled on the pedestal twice."),
	};
}

FString S47Looks::DishSays(const FString& Id, int32 Index, double CurAz, double CurEl, bool bMoving, bool bRogue, bool bBack)
{
	const FString Line = Id == TEXT("S-03") ? FString(TEXT("S-03, at the end of the east walk, its motor cabinet on the apron.")) : FString(DISH[Index % UE_ARRAY_COUNT(DISH)]);
	const FString Az = Pad(CurAz);
	const int32 El = int32(FMath::FloorToDouble(CurEl + 0.5));
	// back off the schedule by itself (story/arrayBack.ts): no motor
	if (bBack) return FString::Printf(TEXT("%s. It is turning back towards 026, and there is no sound from it at all. Az %s, el %d."), *Id, *Az, El);
	if (bMoving) return FString::Printf(TEXT("%s. It is turning, the drive motors whining. Az %s, el %d."), *Id, *Az, El);
	// come down out of 026 by itself (story/dishesFollow.ts), standing a moment where it turned to
	if (bRogue) return FString::Printf(TEXT("%s. It has come down out of 026 and stopped. Az %s, el %d. Nobody sent a command."), *Id, *Az, El);
	// after 02:13 every dish in the array stands at 026, and nobody sent a command
	if (Az == TEXT("026")) return FString::Printf(TEXT("%s. Az 026, el %d. Every dish in the array points the same way."), *Id, El);
	return FString::Printf(TEXT("%s. %s Az %s, el %d."), *Id, *Line, *Az, El);
}

void S47Looks::BindLooks(const FS47LooksDeps& D)
{
	US47Inter* Inter = D.Inter.Get();
	AS47Web* Web = D.Web.Get();
	if (!Inter || !Web) return;
	auto Looks = [Inter](const FString& Id, const FString& Text, double Range = -1) { FS47Look L; L.Say = [Text] { return Text; }; L.Range = Range; Inter->Looks.Add(Id, L); };
	// an invisible box only for a closer look (made by the web version in SARO's outdoor group; the
	// export has it by its id)
	auto Box = [Inter, Web](const FString& Id, TFunction<FString()> Text, double Range = 14)
	{
		const FS47WebItem* It = Web->Item(TEXT("look:") + Id);
		if (!It) return;
		FS47Interactable I;
		I.Id = TEXT("look:") + Id; I.Object = Web->ObjAt(It->Node); I.Label = [] { return FString(); }; I.Use = [] {}; I.bLook = true; I.Examine = MoveTemp(Text); I.ExamineRange = Range;
		Inter->Add(I);
	};
	auto Say = [](const FString& Text) { return TFunction<FString()>([Text] { return Text; }); };

	// ---------- the antennas: the pedestal, and round the dish whichever way it points ----------
	for (int32 i = 0; i < Web->DishCount(); ++i)
	{
		const FString Id = Web->DishId(i);
		TWeakObjectPtr<AS47Web> W = Web;
		TFunction<bool(int32)> Rogue = D.Rogue, Back = D.Back;
		Box(Id, [W, Id, i, Rogue, Back]
		{
			if (!W.IsValid()) return FString();
			return DishSays(Id, i, W->DishAz(i), W->DishEl(i), W->DishMoving(i), Rogue && Rogue(i), Back && Back(i));
		}, 60);
	}

	// ---------- signs round SARO (ServiceYard.ts, Grounds.ts, Exterior.ts) ----------
	Box(TEXT("yardSign"), Say(TEXT("SERVICE YARD. AUTHORIZED PERSONNEL ONLY. The enamel has chipped away round the screws.")));
	Box(TEXT("routeSign"), Say(TEXT("EAST WALK. B-12: NORTH OF S-03. Lettered by hand, under varnish gone yellow.")));
	Box(TEXT("b12Sign"), Say(TEXT("B-12, OPTICAL REFERENCE. A typed card behind plexiglass gone cloudy.")));
	Box(TEXT("labSign"), Say(TEXT("PHOTOGRAPHIC LAB. PROCESS / EXAMINE / FILE. Under it, in marker: SHUT THE DOOR WHEN THE RED LIGHT IS ON.")));
	Box(TEXT("siteSign"), Say(TEXT("SARO. SOUTHWEST ASTRONOMICAL RESEARCH OBSERVATORY. AUTHORIZED VEHICLES ONLY. NO VISITORS AFTER DARK. Sun-bleached on the side facing the highway.")), 20);
	Box(TEXT("quietSign"), Say(TEXT("RADIO QUIET ZONE. NO TRANSMITTERS BEYOND THIS SIGN. The CB under the truck's dash is meant to stay off past here.")), 20);
	// the stop sign where the service road meets the highway
	Box(TEXT("stopSign"), Say(TEXT("STOP. The highway is empty both ways, as far as the dark goes.")), 20);
	Box(TEXT("wingLetters"), Say(TEXT("SARO, and under it SOUTHWEST ASTRONOMICAL RESEARCH OBSERVATORY. One bolt in the last word has rusted through.")), 30);
	// ---------- down the highway to the motel: where drive/roadProps.ts signs() puts them ----------
	Box(TEXT("speedSign"), Say(TEXT("SPEED LIMIT 55. Pocked with birdshot along the bottom edge.")), 24);
	Box(TEXT("mileSign"), Say(TEXT("MILE 112. A green plate on a steel post, the white numbers gone dull.")), 24);
	Box(TEXT("roswellSign"), Say(TEXT("ROSWELL 64. Sixty-four miles of nothing, and then Roswell.")), 24);
	Box(TEXT("stationSign"), Say(TEXT("STATION 01. U.S. SURVEY. NO THROUGH ROAD. Old enamel on wooden posts, and the survey track going off behind it.")), 24);

	// ---------- Sierra Motor Court (MotelFront.ts; chapter four binds the office and the doors) ----------
	auto Motel = [&](const FString& Id, const FString& Text, double Range = 10)
	{
		Looks(TEXT("court:") + Id, Text);
		// its own hit area until chapter four binds the proxy for use (then the text above answers)
		const FS47WebItem* It = Web->Item(TEXT("look:motel.") + Id);
		if (!It || Inter->Get(TEXT("court:") + Id)) return;
		FS47Interactable I;
		I.Id = TEXT("look:motel.") + Id; I.Object = Web->ObjAt(It->Node); I.Label = [] { return FString(); }; I.Use = [] {}; I.bLook = true; I.Examine = Say(Text); I.ExamineRange = Range;
		Inter->Add(I);
	};
	Motel(TEXT("officeDoor"), TEXT("OFFICE, in gold letters on the glass, half of them flaked away. A bell on the counter inside."));
	Motel(TEXT("otherDoors"), TEXT("A brass number on the door, the curtains drawn. Dark behind them."));
	Motel(TEXT("iceMachine"), TEXT("ICE. It rattles, and drops a load into the bin for nobody."));
	Motel(TEXT("car"), TEXT("A dusty sedan nosed in by the office, dew on the glass. It has not been driven tonight."));
	Motel(TEXT("pool"), TEXT("The pool behind its fence, closed for the season. Leaves turn slowly on the water."), 16);
	// (inside the office: from close by, not through its walls)
	Motel(TEXT("register"), TEXT("A ledger in green cloth, open at April. The same name, all the way down the page."), 3.5);
	Motel(TEXT("keyBoard"), TEXT("Brass tags worn smooth by thumbs. One hook is empty."), 3.5);
	Motel(TEXT("officePhone"), TEXT("An oak wall telephone with a crank, older than anything else in the office."), 3.5);
	Motel(TEXT("envelope"), TEXT("T. VEGA, C/O SIERRA MOTOR COURT. A three-cent stamp, and a wax seal nobody has broken."), 3.5);
	Motel(TEXT("message"), TEXT("Pencil on the back of a bill, pressed hard enough to dent the paper."), 3.5);
	Looks(TEXT("room6Door"), TEXT("A brass 6 on the door, and a light behind the curtains."));
	if (const FS47WebItem* It = Web->Item(TEXT("look:motel.room6Door")); It && !Inter->Get(TEXT("room6Door")))
	{
		FS47Interactable I;
		I.Id = TEXT("look:motel.room6Door"); I.Object = Web->ObjAt(It->Node); I.Label = [] { return FString(); }; I.Use = [] {}; I.bLook = true;
		I.Examine = Say(TEXT("A brass 6 on the door, and a light behind the curtains.")); I.ExamineRange = 10;
		Inter->Add(I);
	}
	for (const TCHAR* Face : { TEXT("Sierra Motor Court"), TEXT("Sierra Motor Court (back)") })
	{
		Box(Face, Say(TEXT("Sierra in red script, MOTOR COURT in cyan, a yellow star on top. CLEAN ROOMS. CABLE TV. VACANCY. The neon hums.")), 40);
	}

	// ---------- room 6 (Chapter4.ts) ----------
	Looks(TEXT("r6:bed"), TEXT("A chenille spread pulled tight. The pillow has not been touched."));
	Looks(TEXT("r6:lamp"), TEXT("A ceramic lamp with a paper shade gone brown on the side nearest the bulb."));
	Looks(TEXT("r6:tv"), TEXT("A portable set on the dresser, rabbit ears bent to find Roswell."));
	Looks(TEXT("r6:window"), TEXT("The Sierra sign through a gap in the curtains, red and cyan, and the highway."));
	Looks(TEXT("r6:phone"), TEXT("A beige phone. The cord runs to a wall jack marked OFFICE."));
	Looks(TEXT("r6:bathroom"), TEXT("Shut. A tap drips behind it, steady as a clock."));
	Looks(TEXT("r6:walkie"), TEXT("A field radio in olive drab, its whip aerial folded down. The battery door is taped shut."));
	Looks(TEXT("r6:photo"), TEXT("A snapshot in a dime-store frame, turned a little towards the window."));
	Looks(TEXT("r6:door"), TEXT("The chain is off. The door to the walk, and the cold behind it."));
	Looks(TEXT("r6:table"), TEXT("A table by the window: the papers, a cup gone cold, her reading glasses on their cord."));

	// ---------- the control room: the papers on the desks, and the rest (Prologue.ts, Decoder.ts) ----------
	Looks(TEXT("logbook"), TEXT("Dale's shift log, open at tonight. His writing leans harder to the right as the page goes on."));
	Looks(TEXT("workOrder"), TEXT("The night work order, typed, initialled D. A coffee ring on one corner."));
	Looks(TEXT("excBinder"), TEXT("A grey ring binder, SOLVER EXCEPTIONS on the spine, fat with fan-fold."));
	Looks(TEXT("serviceRecord"), TEXT("RX bank 3's service record, in three different hands. The last is Dale's."));
	Looks(TEXT("telex"), TEXT("The telex roll, curling off the machine and onto the floor."));
	Looks(TEXT("rfiCard"), TEXT("An index card taped to the console. Dale's block capitals."));
	Looks(TEXT("halley"), TEXT("The comet over a desert, in a cheap frame. Somebody has pencilled in the night it came closest: April 11."));
	Looks(TEXT("opsTerminal"), TEXT("The operations terminal. Green letters, and a cursor waiting."));
	Looks(TEXT("map"), TEXT("A road map of New Mexico, the folds worn white. The pencil ring round Roswell has been gone over twice."));
	Looks(TEXT("clock"), TEXT("An electric wall clock with a sweep hand. It hums, and the hand never stops."));
	Looks(TEXT("printer"), TEXT("Dot matrix. The fan-fold feeds up out of a box under the table."));
	Looks(TEXT("phone"), TEXT("A beige desk phone, two lines. The supervisor line has a light of its own."));
	Looks(TEXT("rack"), TEXT("RX bank 3. The rack is warm to the touch and smells of dust burning off."));
	Looks(TEXT("crtCenter"), TEXT("The receiver console, its screen in a hood of black card against the glare."));
	Looks(TEXT("crtLeft"), TEXT("The status terminal: one line for every dish, S-01 to S-27."));
	Looks(TEXT("crtRight"), TEXT("The tracking terminal. The keys in the middle are worn blank."));
	Looks(TEXT("coffeePot"), TEXT("A drip coffee maker on a hot plate, the glass stained brown to the line."));
	Looks(TEXT("mug"), TEXT("SARO, in brown on white. Chipped at the handle."));
	Looks(TEXT("radio"), TEXT("A transistor radio on the shelf, its aerial bent where somebody caught it."));
	Looks(TEXT("fieldCamera"), TEXT("The SARO field camera in its dented case, a sheet film holder beside it."));
	Looks(TEXT("decoder"), TEXT("The signal processor: a tape deck and two spools behind smoked plastic."));
	// ---------- SARO's doors (Doors.ts) ----------
	Looks(TEXT("doorEast"), TEXT("The service door. A push bar, and a card: KEEP CLOSED. DUST."));
	Looks(TEXT("doorSouth"), TEXT("The corridor door, a square of wired glass at head height."));
	Looks(TEXT("labDoor"), TEXT("The lab door. A red bulb over it for when the safelight is on."));
	Looks(TEXT("exitDoor"), TEXT("FIRE EXIT, stencilled in red on grey steel."));
	Looks(TEXT("recordsDoor"), TEXT("RECORDS, on a card in a brass holder."));
	// ---------- the yard and the records room (Chapter1.ts, Chapter2.ts) ----------
	Looks(TEXT("motorBus"), TEXT("S-03's motor bus cabinet. A sticker on the door: DO NOT RESET WITHOUT A LOG ENTRY."));
	Looks(TEXT("wetBench"), TEXT("Three trays, tongs, brown bottles in a row, and the smell of fixer."));
	Looks(TEXT("archive"), TEXT("The archive bench: a light box, a loupe on a string, a print tray."));
	Looks(TEXT("b12Control"), TEXT("B-12's local control, a grey box on a post with its key on a chain."));
	Looks(TEXT("vane"), TEXT("A plate on a post with one ivory stripe, turned to face the apron."));
	Looks(TEXT("records"), TEXT("The night report on a clipboard: observation, method, conclusion."));
	Looks(TEXT("r07"), TEXT("Three black bars on white, the paint crazed by the sun."));
	Looks(TEXT("fieldBox"), TEXT("A records box, STATION 01 1947 in grease pencil. The lid is soft with handling."));
	Looks(TEXT("binder"), TEXT("A binder of service copies, the spine cracked white."));
	Looks(TEXT("lineage"), TEXT("A long drawer of index cards with a brass pull: MAINTENANCE."));
	Looks(TEXT("workTable"), TEXT("A big table under a strip light, scored all over with a craft knife."));
	Looks(TEXT("recPhone"), TEXT("A wall phone, beige, the cord stretched out long."));
	Looks(TEXT("vending"), TEXT("Cola, orange soda, peanut butter crackers. The light inside flickers."));
	Looks(TEXT("officeDoor"), TEXT("A frosted glass door, OPERATIONS in black. Dark behind it."));
	Looks(TEXT("restroomDoor"), TEXT("MEN and WOMEN. The fan inside runs all night."));

	// ---------- STATION 01 (Chapter3.ts) ----------
	Looks(TEXT("s1transit"), TEXT("A brass transit on a concrete pier, its cover off. The bubble still settles."));
	Looks(TEXT("s1footing"), TEXT("Grit in the bolt holes. Nothing has stood on it for a long time."));
	Looks(TEXT("s1markerA"), TEXT("A triangle outlined on a plate, and a bar under it."));
	Looks(TEXT("s1markerB"), TEXT("A vane plate on a post, still set square in its footing."));
	Looks(TEXT("s1stakes"), TEXT("Wooden stakes going east into the dark, grey with age."), 8);
	Looks(TEXT("s1diagram"), TEXT("A survey diagram in a frame, the glass fly-spotted."));
	Looks(TEXT("s1lamp"), TEXT("A field lamp on a stand, the glass sooted on one side."));
	Looks(TEXT("s1cable"), TEXT("A field cable in the dirt, the insulation cracked by the sun."));
	Looks(TEXT("s1hutDoor"), TEXT("Boards gone silver, and a door that has not been locked in years."));
	Looks(TEXT("s1receiver"), TEXT("A valve receiver on the bench, older than anything at SARO."));
	Looks(TEXT("s1timing"), TEXT("A timing log on a clip: pencil, columns of seconds."));
	Looks(TEXT("s1phone"), TEXT("A field telephone with a crank. Its line leaves through the wall."));
	Looks(TEXT("s1gate"), TEXT("The vehicle gate, chained and padlocked."), 10);

	// ---------- Mesa Diner (Chapter5.ts) ----------
	Looks(TEXT("diner:waitress"), TEXT("A pencil behind her ear. She looks at the window each time a truck might pass."));
	Looks(TEXT("diner:driver"), TEXT("A driver in a quilted vest, eating slowly. A feed store's name on his cap."));
	Looks(TEXT("diner:coffee"), TEXT("The pot on the warmer, a ring of old coffee baked round the rim."));
	Looks(TEXT("diner:counter"), TEXT("A radio on the shelf behind the counter, between the pie dome and the register."));
	Looks(TEXT("diner:clipping"), TEXT("Yellow newsprint behind a strip of tape, the date cut off."));
	Looks(TEXT("diner:payphone"), TEXT("A payphone in a niche. Numbers written all over the wall round it."));
	Looks(TEXT("diner:booth"), TEXT("Red vinyl, split and taped. The window beside it is cold."));
	Looks(TEXT("diner:menu"), TEXT("White letters pushed into a felt board. The P in PIE is upside down."));
	Looks(TEXT("diner:jukebox"), TEXT("OUT OF ORDER, in ballpoint behind the glass. A record still on the arm."));
	Looks(TEXT("diner:window"), TEXT("The lot, the truck, the highway, and past it the dark."));
	Looks(TEXT("diner:door"), TEXT("MESA DINER, OPEN ALL NIGHT, on the glass. The closer hisses."));
	Looks(TEXT("diner:sign"), TEXT("MESA DINER. One letter of the neon buzzes, goes out, and buzzes."), 14);
	Looks(TEXT("diner:rig"), TEXT("A refrigerated trailer, the unit running. Frost on the doors at the back."), 12);

	// ---------- Caliche, what had nothing to say yet (caliche.ts) ----------
	Looks(TEXT("town:bench"), TEXT("A bench under the window, the paint worn through where somebody sits."), 6);
	Looks(TEXT("town:gasSign"), TEXT("GAS. DIESEL. The tubes in one side of the sign are dark."), 12);
	Looks(TEXT("town:feed"), TEXT("FEED & SEED. Moths round the bulb over the door, and sacks on the dock."), 10);
	Looks(TEXT("town:amDoor"), TEXT("1420 AM. STUDIO IN USE, PLEASE WAIT. The buzzer button is worn white."), 6);
	Looks(TEXT("town:souvenirSign"), TEXT("SAUCER CRASH 1947. TOURS. ASK INSIDE. Under the paint, an older word: BAIT."), 10);
	Looks(TEXT("town:souvenirDoor"), TEXT("CLOSED. The hours on the card have faded white."), 6);
	Looks(TEXT("town:letters"), TEXT("THE HEAVENS DECLARE THE GLORY OF GOD. PS 19:1. One letter hangs crooked."), 10);
	Looks(TEXT("town:churchDoor"), TEXT("SUNDAY 10 AM. ALL WELCOME. The doors are new; the step is worn hollow."), 8);
	Looks(TEXT("town:tv2"), TEXT("Snow, then the test pattern, then snow. Nobody in the room."), 9);
	Looks(TEXT("town:kitchen"), TEXT("A kitchen light behind a curtain, and a shadow that does not move."), 7);
}

bool S47Looks::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s looks: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	Check(DishSays(TEXT("S-07"), 6, 26.2, 31.6, false, false, false) == TEXT("S-07. Az 026, el 32. Every dish in the array points the same way."), TEXT("after 02:13, every dish at 026"));
	Check(DishSays(TEXT("S-03"), 2, 18, 48, false, false, false) == TEXT("S-03. S-03, at the end of the east walk, its motor cabinet on the apron. Az 018, el 48."), TEXT("S-03 has a line of its own"));
	Check(DishSays(TEXT("S-02"), 1, -0.4, 48, false, false, false) == TEXT("S-02. Rust runs down the pedestal from every bolt. Two of the reflector panels are newer than the rest. Az 000, el 48."), TEXT("the others by their number, the azimuth padded"));
	Check(DishSays(TEXT("S-01"), 0, 26, 2, false, true, false).Contains(TEXT("It has come down out of 026 and stopped. Az 026, el 2.")) && DishSays(TEXT("S-01"), 0, 40, 20, true, true, true).Contains(TEXT("turning back towards 026")), TEXT("rogue, and back without a sound"));
	return bOk;
}

// story/Chapter3.ts: chapter three, "The Survey Station"
#include "S47Chapter3.h"
#include "S47Chapter4.h"

#include "Misc/Base64.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "S47Audio.h"
#include "S47Chapter2.h"
#include "S47Drawings.h"
#include "S47HUD.h"
#include "S47Image.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Panels.h"
#include "S47PhoneScripts.h"
#include "S47Station.h"
#include "S47StationTaken.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Ch3, Log, All);

namespace
{
	// ---------- the sources and replies (Unity Station26 text) ----------
	const TCHAR* TRANSIT = TEXT("FIELD TRANSIT / STATION 01 / E09A\n\n1947 / A: triangle with bar; mounted in the concrete footing on the transit sight line. B: fixed comparison marker to the west. C: closing sight line, shown in the original arrangement.\n\nCompare the empty footing, present markers and this diagram. Photograph both the footing and the marked post before recording which position changed.");
	const TCHAR* TIMING_1947 = TEXT("FIELD TIMING RECORD / STATION 01 / E10\n\n02:17:00 / Carrier ceased. Recorded voice: ‘Reference west. No. East. Hold the last reading.’\n02:17:47 / Local relay impact and reference motion observed.\nFinal observation / Not entered.");
	const TCHAR* TIMING_1986 = TEXT("NEW RECEIVER OBSERVATION / 1986\nThe received fragment repeats the same correction, ‘west’ to ‘east,’ in the same place. The original field record identifies the speaker as T. Vega.");

	// SUPPORTED
	const TCHAR* SUPPORTED_MARKER = TEXT("P06 SUPPORTED / The fixed survey marker A has moved relative to the transit record. The original footing and fixed B marker establish the comparison; this record does not identify who moved A.");
	const TCHAR* SUPPORTED_LAMP = TEXT("NULL TEST RECORDED / The local source is screened. This control result does not complete the cable reconstruction.");
	const TCHAR* SUPPORTED_CABLE = TEXT("P08 SUPPORTED / The cable was deliberately cut. The null test separates the local lamp from the retained reference.");
	const TCHAR* SUPPORTED_TIMING = TEXT("P09 SUPPORTED / The historic relay observation followed carrier loss by 47 seconds. The 1986 fragment repeats T. Vega's same correction and adds no final observation.");

	// Three levels, as in chapters one and two: a question, a pointer, the answer. (HINTS)
	const TArray<FString> HINTS_MARKER = { TEXT("Which fixed point is no longer where E09A puts it?"), TEXT("E09A puts A in the concrete footing on the transit sight line. Is the footing still occupied?"), TEXT("The footing is empty, and the post with the triangle and bar stands off the line. Tie A on E09A to FRAME 03.") };
	const TArray<FString> HINTS_LAMP = { TEXT("A null test changes one local condition and looks again."), TEXT("The field lamp has a tin hood. Cover it, then observe."), TEXT("Stand by the lamp, cover it, and choose OBSERVE NULL TEST.") };
	const TArray<FString> HINTS_CABLE = { TEXT("Look at the ends of the cable, not at the reference."), TEXT("Old cable frays and corrodes. These faces are clean and opposite each other. And the lamp, covered, changed nothing."), TEXT("Tie FRAME 04, the cut, to the null test at the lamp, P07.") };
	const TArray<FString> HINTS_TIMING = { TEXT("Read the 1947 log in the hut and listen to the receiver."), TEXT("The carrier ceased at 02:17:00. What was observed 47 seconds later?"), TEXT("Tie the receiver to the line in E10 with the same words.") };

	// ---------- the case board ----------
	// What the station's records and frames show together (story/boardRules.ts). The replies to
	// threads that contradict are the Unity ones where Unity had that wrong answer.
	const TMap<FString, FS47BookNote>& NOTES()
	{
		static const TMap<FString, FS47BookNote> N = {
			{ TEXT("n.a"), { TEXT("A is gone from its footing on the transit sight line. The post with its mark, the triangle and bar, stands off the line."), TEXT("P06"), false } },
			{ TEXT("n.cut"), { TEXT("Two clean faces, opposite each other, and a lamp that changed nothing when it was covered. The cable was cut on purpose."), TEXT("P08"), false } },
			{ TEXT("n.same"), { TEXT("Tonight's fragment is T. Vega's correction from the 1947 log, word for word. The relay struck 47 seconds after it, at 02:17:47."), TEXT("P09"), false } },
		};
		return N;
	}
	const TCHAR* NOT_THE_LAMP = TEXT("The lamp and the cable do not identify the moved fixed point. Match the outlined triangle and short bar, then record A.");
	const TCHAR* NOT_WHO = TEXT("The faces show that the cable was cut on purpose. They do not show who cut it, or why.");
	FS47BookRule Rule(const TCHAR* A, const TCHAR* B, const FString& Note, const FString& Text, const FString& Wrong = FString())
	{
		FS47BookRule R;
		R.A = A; R.B = B; R.Note = Note; R.Text = Text; R.Wrong = Wrong;
		return R;
	}
	TArray<FS47BookRule> RULES()
	{
		const TMap<FString, FS47BookNote>& N = NOTES();
		return {
			Rule(TEXT("e09a.a"), TEXT("frame03"), TEXT("n.a"), N[TEXT("n.a")].Text),
			Rule(TEXT("e09a.b"), TEXT("frame03"), FString(), TEXT("B is still in its footing west of the transit. The post in FRAME 03 carries the triangle with the bar."), TEXT("P06")),
			Rule(TEXT("e09a.c"), TEXT("frame03"), FString(), TEXT("C is a sight line, not a marker; the stakes still run east. Which reference had the triangle with the bar?"), TEXT("P06")),
			Rule(TEXT("P07"), TEXT("frame03"), FString(), NOT_THE_LAMP, TEXT("P06")),
			Rule(TEXT("e09a.a"), TEXT("frame04"), FString(), NOT_THE_LAMP, TEXT("P06")),
			Rule(TEXT("frame04"), TEXT("P07"), TEXT("n.cut"), N[TEXT("n.cut")].Text),
			Rule(TEXT("frame04"), TEXT("e07.sig"), FString(), NOT_WHO, TEXT("P08")),
			Rule(TEXT("frame04"), TEXT("e06.sig"), FString(), NOT_WHO, TEXT("P08")),
			Rule(TEXT("frame04"), TEXT("e07.head"), FString(), TEXT("The cut faces are opposed and clean. A cable from 1947 can wear through, but age and weather do not leave a break like that."), TEXT("P08")),
			Rule(TEXT("hut.rx"), TEXT("e10.carrier"), TEXT("n.same"), N[TEXT("n.same")].Text),
			Rule(TEXT("hut.rx"), TEXT("e10.relay"), FString(), TEXT("The voice is not on that line. The matched reading arrives at the 47-second relay impact: 02:17:47. Do not infer a new event from the clock alone."), TEXT("P09")),
			Rule(TEXT("hut.rx"), TEXT("e10.final"), FString(), TEXT("The fragment is the correction, not a final observation. It adds nothing after it, and the log leaves that line empty."), TEXT("P09")),
			// threads that hold without answering anything
			Rule(TEXT("e09a.c"), TEXT("e07.abc"), FString(), TEXT("The same C: the closing sight line, on the transit plan and in the 1947 record.")),
			Rule(TEXT("s1diagram"), TEXT("e06.omit"), FString(), TEXT("The framed copy in the hut still has C. Only the service copy left it out.")),
		};
	}

	constexpr double TANK_TIME = 2.2, FIX_TIME = 1.5;
	const TCHAR* RING = TEXT("ring3");
	const TCHAR* TRUCK_EXAMINE = TEXT("SARO 07: a white pickup with tired paint and a toolbox in the bed. The keys are in it, like always.");

	FString ClockText(double Sec, bool bSeconds = true) { return S47Text::ClockText(Sec, bSeconds); }

	FS47Sound Snd(float Gain, float Rate = 1.f, TOptional<FVector> At = {})
	{
		FS47Sound O;
		O.Gain = Gain; O.Rate = Rate; O.At = At;
		return O;
	}

	// the chapter in play counts as Prologue.at('chN'): ch1 < ch2 < ... < ch6
	bool AtChapter(const AS47Story* H, int32 N)
	{
		return H && H->ChapterKey.StartsWith(TEXT("ch")) && FCString::Atoi(*H->ChapterKey.Mid(2)) >= N;
	}
}

// ---------- the saved state ----------

bool& FS47Ch3State::SeenOf(const FString& What)
{
	if (What == TEXT("footing")) return Seen.bFooting;
	if (What == TEXT("markerA")) return Seen.bMarkerA;
	if (What == TEXT("markerB")) return Seen.bMarkerB;
	if (What == TEXT("stakes")) return Seen.bStakes;
	if (What == TEXT("diagram")) return Seen.bDiagram;
	return Seen.bHut;
}

S47J::FObj FS47Ch3State::Save() const
{
	using namespace S47J;
	FObj O = Extra.IsValid() ? Clone(Extra) : New();
	Set(O, TEXT("v"), 1);
	Set(O, TEXT("stage"), Stage);
	Set(O, TEXT("arrived"), bArrived);
	Set(O, TEXT("readE09a"), bReadE09a); Set(O, TEXT("readE10"), bReadE10); Set(O, TEXT("heard"), bHeard);
	FObj Sn = Obj(O, TEXT("seen"));
	if (!Sn.IsValid()) Sn = New();
	Set(Sn, TEXT("footing"), Seen.bFooting); Set(Sn, TEXT("markerA"), Seen.bMarkerA); Set(Sn, TEXT("markerB"), Seen.bMarkerB);
	Set(Sn, TEXT("stakes"), Seen.bStakes); Set(Sn, TEXT("diagram"), Seen.bDiagram); Set(Sn, TEXT("hut"), Seen.bHut);
	Set(O, TEXT("seen"), Sn);
	Set(O, TEXT("lampCovered"), bLampCovered); Set(O, TEXT("lampObserved"), bLampObserved); Set(O, TEXT("cableInspected"), bCableInspected);
	if (F3) Set(O, TEXT("f3"), F3->ToJson()); else O->RemoveField(TEXT("f3"));
	if (F4) Set(O, TEXT("f4"), F4->ToJson()); else O->RemoveField(TEXT("f4"));
	Set(O, TEXT("p06"), bP06); Set(O, TEXT("p07"), bP07); Set(O, TEXT("p08"), bP08); Set(O, TEXT("p09"), bP09);
	Set(O, TEXT("wrong06"), Wrong06); Set(O, TEXT("wrong08"), Wrong08); Set(O, TEXT("wrong09"), Wrong09); Set(O, TEXT("rejected"), Rejected);
	Set(O, TEXT("called"), bCalled);
	Set(O, TEXT("dev"), Dev);
	if (Links) Set(O, TEXT("links"), *Links); else O->RemoveField(TEXT("links"));
	if (Fb.IsValid()) O->SetField(TEXT("fb"), Fb); else O->RemoveField(TEXT("fb"));
	return O;
}

FS47Ch3State FS47Ch3State::Load(const S47J::FObj& Saved)
{
	using namespace S47J;
	FS47Ch3State C;
	if (!Saved.IsValid()) return C;
	C.Extra = Clone(Saved);
	C.Stage = Str(Saved, TEXT("stage"), C.Stage);
	C.bArrived = Bool(Saved, TEXT("arrived"));
	C.bReadE09a = Bool(Saved, TEXT("readE09a")); C.bReadE10 = Bool(Saved, TEXT("readE10")); C.bHeard = Bool(Saved, TEXT("heard"));
	const FObj Sn = Obj(Saved, TEXT("seen"));
	C.Seen.bFooting = Bool(Sn, TEXT("footing")); C.Seen.bMarkerA = Bool(Sn, TEXT("markerA")); C.Seen.bMarkerB = Bool(Sn, TEXT("markerB"));
	C.Seen.bStakes = Bool(Sn, TEXT("stakes")); C.Seen.bDiagram = Bool(Sn, TEXT("diagram")); C.Seen.bHut = Bool(Sn, TEXT("hut"));
	C.bLampCovered = Bool(Saved, TEXT("lampCovered")); C.bLampObserved = Bool(Saved, TEXT("lampObserved")); C.bCableInspected = Bool(Saved, TEXT("cableInspected"));
	C.F3 = FS47Photo::FromJson(Obj(Saved, TEXT("f3")));
	C.F4 = FS47Photo::FromJson(Obj(Saved, TEXT("f4")));
	C.bP06 = Bool(Saved, TEXT("p06")); C.bP07 = Bool(Saved, TEXT("p07")); C.bP08 = Bool(Saved, TEXT("p08")); C.bP09 = Bool(Saved, TEXT("p09"));
	C.Wrong06 = Int(Saved, TEXT("wrong06")); C.Wrong08 = Int(Saved, TEXT("wrong08")); C.Wrong09 = Int(Saved, TEXT("wrong09")); C.Rejected = Int(Saved, TEXT("rejected"));
	C.bCalled = Bool(Saved, TEXT("called"));
	C.Dev = Int(Saved, TEXT("dev"));
	if (Saved->HasTypedField<EJson::Array>(TEXT("links"))) C.Links = Strs(Saved, TEXT("links"));
	if (Has(Saved, TEXT("fb"))) C.Fb = Saved->TryGetField(TEXT("fb"));
	return C;
}

// ---------- deps ----------

US47UI* US47Chapter3::Ui() const { return M ? M->UI.Get() : nullptr; }
US47Audio* US47Chapter3::Au() const { return M ? M->Audio.Get() : nullptr; }
US47FieldCamera* US47Chapter3::Fcam() const { return M ? M->FieldCamera.Get() : nullptr; }
US47Chapter1* US47Chapter3::Ch1() const { return Host ? Cast<US47Chapter1>(Host->Chapter(TEXT("ch1"))) : nullptr; }
void US47Chapter3::Toast(const FString& Text, float Secs) const { if (US47UI* U = Ui()) U->Toast(Text, Secs); }
void US47Chapter3::Note(const FString& Text) { if (Host) Host->Note(Text); else if (!TestNotes.Contains(Text)) TestNotes.Add(Text); }
void US47Chapter3::After(double Secs, TFunction<void()> Fn, const FString& Tag) { if (Host) Host->After(Secs, MoveTemp(Fn), Tag); }
double US47Chapter3::Gt() const { return Host ? Host->Gt() : 0.0; }
FVector US47Chapter3::PlayerPos() const { return M && M->Player ? M->Player->Pos : FVector(0, 0, -1e6); }
void US47Chapter3::Save() { if (M) M->RequestAutosave(false); }   // deps.save: saveCase(composeCase(...))
void US47Chapter3::Milestone() { if (Host && Host->OnCheckpoint) Host->OnCheckpoint(TEXT("chapter3")); }
void US47Chapter3::BoardRefresh() { if (M && M->Board.Refresh) M->Board.Refresh(); }
void US47Chapter3::BoardOpen() { if (M && M->Board.Open) M->Board.Open(TEXT("STATION 01"), TEXT("ch3")); }
FString US47Chapter3::Time() const { return ClockText(Host ? Host->Clock() : 0.0, false); }

bool US47Chapter3::File(const FS47Doc& Doc)
{
	if (!Host) return true;
	TArray<FS47Doc>& Docs = Host->DocsMutable();
	const int32 I = Docs.IndexOfByPredicate([&](const FS47Doc& X) { return X.Id == Doc.Id; });
	if (I >= 0) { Docs[I] = Doc; return false; }
	Docs.Add(Doc);
	return true;
}

double US47Chapter3::Dist(const FVector& P) const
{
	const FVector Q = PlayerPos();
	return FMath::Sqrt(FMath::Square(P.X - Q.X) + FMath::Square(P.Z - Q.Z));
}

// world.area: the area shown (AS47Web holds it; World.enter and a restored save both set it there)
FString US47Chapter3::Area() const { return M && M->Web ? M->Web->Area() : FString(TEXT("saro")); }
US47Station* US47Chapter3::Site() const { return Station && Station->IsBuilt() ? Station.Get() : nullptr; }
bool US47Chapter3::Driving() const { return M && M->World && M->World->bDriving; }

// ---------- registration (the constructor, and main.ts's part) ----------

US47Chapter3* US47Chapter3::Register(US47Chapter3* C)
{
	if (!C) return C;
	C->BuildBook();
	TWeakObjectPtr<US47Chapter3> W = C;
	US47Main* M = C->M;
	// main.ts: the world's station (world.site, built from the export) and its hooks
	C->Station = NewObject<US47Station>(C);
	C->Station->Init(M);
	// main.ts every frame: world.setStoryPlaces({ stationOpen: game.at('ch3') }): the walk gap
	C->Station->StationOpen = [W] { return W.IsValid() && AtChapter(W->Host, 3); };
	// at STATION 01 after N. Vega's call, everyone goes (story/stationTaken.ts): the hut door shut by
	// itself, the truck without its keys
	C->StationTaken = NewObject<US47StationTaken>(C);
	C->StationTaken->Init(M, [W] { return W.IsValid() ? W->Site() : nullptr; });
	{
		FTaken T;
		T.Shut = [W] { return W.IsValid() && W->StationTaken && W->StationTaken->HutShut(); };
		T.TryDoor = [W] { if (W.IsValid() && W->StationTaken) W->StationTaken->TryDoor(); };
		T.Truck = [W] { return W.IsValid() && W->StationTaken ? W->StationTaken->TruckExamine() : FString(); };
		C->Taken = T;
	}
	if (M && M->Web && M->Inter)
	{
		// ch3.bindSaroTruck(yard.objs.truck)
		C->BindSaroTruck(M->Web->Obj(TEXT("yard.objs.truck")));
		// world.onStationLoaded: the station is in the export from the start, so its things are bound now
		if (US47Station* Site = C->Site())
		{
			C->BindSite(Site);
			// the walk gap before chapter three: chained (Station01.setWalkGate, World.setStoryPlaces)
			if (const FS47Obj Gate = Site->Proxy(TEXT("walkGate")))
			{
				FS47Interactable It;
				It.Id = TEXT("s1walkGate"); It.Object = Gate; It.Range = 2.6;
				It.Label = [W] { return W.IsValid() && W->Area() == TEXT("station01") && !AtChapter(W->Host, 3) ? FString(TEXT("Walk gate")) : FString(); };
				It.Use = [W] { if (W.IsValid()) W->Toast(TEXT("Chained, with a padlock. N. Vega holds the keys out here."), 3.4f); };
				It.Examine = [W] { return W.IsValid() && W->Area() == TEXT("station01") && !AtChapter(W->Host, 3) ? FString(TEXT("A chain through the walk gap, and a padlock a good deal newer than the fence.")) : FString(); };
				M->Inter->Add(It);
			}
			// (the drawer under the bench, s1drawer: story/fieldBook.ts; the ninth cow, s1ninth:*:
			// story/ninthCow.ts; neither is in this version)
		}
	}
	if (M && M->World)
	{
		M->World->OnArriveStation = [W] { if (W.IsValid()) W->ArrivedStation(); };
		M->World->OnArriveSaro = [W] { if (W.IsValid()) W->ArrivedSaro(); };
		// main.ts world.truckChoice: the night's own drive if a chapter has one from here, otherwise
		// one of the player's own; not while the shift's first hours are still running (the World's
		// own choice, which stays behind this one for the later chapters and the player's drives)
		TFunction<US47World::FChoice(const FString&)> Prev = M->World->TruckChoice;
		M->World->TruckChoice = [W, Prev, M](const FString& Place) -> US47World::FChoice
		{
			if (!W.IsValid() || (M->Story && !M->Story->At(ES47Phase::Ch1))) return Prev ? Prev(Place) : US47World::FChoice();
			// (at the station while everyone goes: the keys are not in it; story/stationTaken.ts)
			if (Place == TEXT("station01") && W->StationTaken && W->StationTaken->bKeysGone)
			{
				const TOptional<FTrip> T = W->TruckTrip(Place);
				return { T ? T->Label : FString(TEXT("Drive")), [W] { if (W.IsValid() && W->StationTaken) W->StationTaken->TryTruck(); } };
			}
			if (const TOptional<FTrip> T = W->TruckTrip(Place)) return { T->Label, T->Use };
			// ?? game.ch4.truckTrip(world.truckAt) ?? game.ch5.truckTrip(place) ?? { label: 'Drive', use: () => world.roam(place) }
			return Prev ? Prev(Place) : US47World::FChoice();
		};
	}
	if (C->Host) C->Host->Jumps.Add(TEXT("chapter3"), [W] { if (W.IsValid()) W->JumpChapter3(); });
	// -S47Ch3Break (with -S47Jump=chapter2): chapter two ends three seconds in, and its card leads here
	if (FParse::Param(FCommandLine::Get(), TEXT("S47Ch3Break")) && M && M->Web)
	{
		FTimerHandle Th;
		M->Web->GetWorldTimerManager().SetTimer(Th, [W]
		{
			US47Chapter2* C2 = W.IsValid() && W->Host ? Cast<US47Chapter2>(W->Host->Chapter(TEXT("ch2"))) : nullptr;
			if (C2 && C2->OnEnd) { UE_LOG(LogS47Ch3, Display, TEXT("S47: chapter two ends (test)")); C2->OnEnd(); }
		}, 3.0f, false);
	}
	// main.ts ch3.onEnd: the prints are filed, and N. Vega is waiting across the road. Chapter four is
	// not ported yet: until it is, the night ends on a card that says so, as chapter two's did.
	C->OnEnd = [W]
	{
		if (!W.IsValid() || !W->Host) return;
		AS47Story* H = W->Host;
		if (H->Chapter(TEXT("ch4")))
		{
			// (the integrator puts chapter four's own begin here: Prologue.beginChapter4(null))
			H->ChapterBreak({ TEXT("THE SURVEY STATION // P06 TO P09 RECORDED"),
				TEXT("Fixed point A was moved. The cable was cut on purpose. The receiver in the hut still carries T. Vega's correction. N. Vega is waiting in room 6, across the road."),
				TEXT("CHAPTER FOUR"), TEXT("ROOM 6"), [H] { if (US47Chapter4* C4 = Cast<US47Chapter4>(H->Chapter(TEXT("ch4")))) C4->BeginChapter4(nullptr); else H->BeginChapter(TEXT("ch4")); } });
			return;
		}
		if (W->Ui()) W->Ui()->Fade(true, FString());
		H->After(1.6, [W]
		{
			if (!W.IsValid()) return;
			W->Host->SetCinematic(true);
			if (W->M && W->M->Hud) W->M->Hud->EndCard({ TEXT("CHAPTER 3: THE SURVEY STATION"), TEXT(""), TEXT("Chapter 4, Room 6, is not in the Unreal version yet."), TEXT("The web version plays on from here.") });
		});
	};
	return C;
}

void US47Chapter3::BeginDestroy()
{
	if (M && Book) M->Books.Remove(Book.Get());
	Super::BeginDestroy();
}

void US47Chapter3::BuildBook()
{
	FS47BookSpec O;
	O.Key = TEXT("ch3");
	// /^(e09a|e10)\./ or one of the station's own
	O.Owns = [](const FString& N)
	{
		if (N.StartsWith(TEXT("e09a."), ESearchCase::CaseSensitive) || N.StartsWith(TEXT("e10."), ESearchCase::CaseSensitive)) return true;
		for (const TCHAR* K : { TEXT("frame03"), TEXT("frame04"), TEXT("hut.rx"), TEXT("P06"), TEXT("P07"), TEXT("P08"), TEXT("P09") }) if (N.Equals(K, ESearchCase::CaseSensitive)) return true;
		return false;
	};
	O.Notes = NOTES();
	O.Rules = RULES();
	{
		FS47BookQuestion Q;
		Q.Id = TEXT("P06"); Q.Stamp = TEXT("P06"); Q.Text = TEXT("Which fixed point is no longer where E09A puts it?"); Q.Slots = 1; Q.Hints = HINTS_MARKER; Q.X = 14; Q.Y = 470; Q.W = 290;
		Q.Open = [this] { return bStarted; };
		Q.Done = [this] { return S.bP06 ? FString(SUPPORTED_MARKER) : FString(); };
		Q.Record = [this] { return SubmitMarker(TEXT("a")); };
		O.Questions.Add(Q);
	}
	{
		FS47BookQuestion Q;
		Q.Id = TEXT("P07"); Q.Stamp = TEXT("P07"); Q.Text = TEXT("Does the local lamp make the mark?"); Q.Slots = 0; Q.Hints = HINTS_LAMP; Q.X = 600; Q.Y = 14; Q.W = 270;
		Q.Elsewhere = TEXT("Made at the field lamp, not here: cover it, then observe a null test.");
		Q.Open = [this] { return bStarted; };
		Q.Done = [this] { return S.bP07 ? FString(SUPPORTED_LAMP) : FString(); };
		O.Questions.Add(Q);
	}
	{
		FS47BookQuestion Q;
		Q.Id = TEXT("P08"); Q.Stamp = TEXT("P08"); Q.Text = TEXT("What does the cable break show?"); Q.Slots = 1; Q.Hints = HINTS_CABLE; Q.X = 600; Q.Y = 222; Q.W = 270;
		Q.Open = [this] { return bStarted; };
		Q.Done = [this] { return S.bP08 ? FString(SUPPORTED_CABLE) : FString(); };
		Q.Record = [this] { return SubmitCable(TEXT("deliberate-cut")); };
		O.Questions.Add(Q);
	}
	{
		FS47BookQuestion Q;
		Q.Id = TEXT("P09"); Q.Stamp = TEXT("P09"); Q.Text = TEXT("Match the receiver fragment to the 1947 log. What does T. Vega's correction lead up to?"); Q.Slots = 1; Q.Hints = HINTS_TIMING; Q.X = 600; Q.Y = 444; Q.W = 270;
		Q.Open = [this] { return bStarted; };
		Q.Done = [this] { return S.bP09 ? FString(SUPPORTED_TIMING) : FString(); };
		Q.Record = [this] { return SubmitTiming(TEXT("02:17:47")); };
		O.Questions.Add(Q);
	}
	O.Links = [this]() -> TArray<FString>& { if (!S.Links) S.Links.Emplace(); return *S.Links; };
	O.Wrong = [this](const FString& Q, const FS47BookRule&) -> TOptional<FS47Result>
	{
		if (Q == TEXT("P06")) S.Wrong06++; else if (Q == TEXT("P08")) S.Wrong08++; else if (Q == TEXT("P09")) S.Wrong09++;
		return {};
	};
	O.Save = [this] { Save(); };
	O.Cards = [this] { return BoardCards(); };
	Book = MakeUnique<FS47RuleBook>(MoveTemp(O));
}

// The station's records and the two field frames, as they lie on the board. A frame is on the
// roll, sealed, until it is developed at SARO; what is in it is known all the same.
TArray<FS47BoardCard> US47Chapter3::BoardCards()
{
	if (!bStarted) return {};
	const bool bDev = S.Dev >= 3;
	auto Line = [](const TCHAR* Id, const FString& Text) { FS47BoardLine L; L.Id = Id; L.Text = Text; return L; };
	auto Frame = [bDev](const TOptional<FS47Photo>& F, const TCHAR* Id, const TCHAR* Title, const FString& What, const TCHAR* Where, double X, double Y)
	{
		FS47BoardCard C;
		C.Id = Id; C.Doc = Id; C.Stamp = FString(Id) == TEXT("frame03") ? TEXT("FRAME 03") : TEXT("FRAME 04");
		C.Title = bDev ? Title : TEXT("ON THE FIELD ROLL"); C.Kind = bDev ? TEXT("photo") : TEXT("sealed");
		C.bHave = F.IsSet(); C.Where = Where;
		if (bDev && F) C.Image = F->Url;
		C.Text = bDev ? What : What + TEXT(" Sealed; it is developed at SARO.");
		C.X = X; C.Y = Y; C.W = 250;
		return C;
	};
	TArray<FS47BoardCard> Out;
	{
		FS47BoardCard C;
		C.Id = TEXT("e09a"); C.Doc = TEXT("e09a"); C.Stamp = TEXT("E09A"); C.Title = TEXT("TRANSIT RECORD"); C.bHave = S.bReadE09a;
		C.Where = TEXT("Behind glass in a steel frame on the west face of the transit pier.");
		C.Image = TransitImage(); C.Full = TRANSIT; C.X = 14; C.Y = 14; C.W = 290;
		C.Lines = {
			Line(TEXT("e09a.a"), TEXT("A: triangle with bar, mounted in the concrete footing on the transit sight line.")),
			Line(TEXT("e09a.b"), TEXT("B: fixed comparison marker to the west.")),
			Line(TEXT("e09a.c"), TEXT("C: closing sight line, shown in the original arrangement.")) };
		Out.Add(C);
	}
	Out.Add(Frame(S.F3, TEXT("frame03"), TEXT("FIXED POINT A"), TEXT("The empty footing on the transit sight line, and the post with the triangle and bar standing off it."),
		TEXT("Expose it from near the transit: the empty footing and the marked post in one frame."), 320, 14));
	Out.Add(Frame(S.F4, TEXT("frame04"), TEXT("CABLE BREAK"), TEXT("The cable south of the transit pier, cut: two clean faces, opposite each other."),
		TEXT("Inspect the cable break, then expose a near frame of the cut."), 320, 300));
	{
		FS47BoardCard C;
		C.Id = TEXT("e10"); C.Doc = TEXT("e10"); C.Stamp = TEXT("E10"); C.Title = TEXT("TIMING RECORD, 1947"); C.bHave = S.bReadE10;
		C.Where = TEXT("The 1947 ledger is on the workbench in the hut.");
		C.Full = S.bHeard ? FString(TIMING_1947) + TEXT("\n\n") + TIMING_1986 : FString(TIMING_1947); C.X = 890; C.Y = 14; C.W = 276;
		C.Lines = {
			Line(TEXT("e10.carrier"), TEXT("02:17:00 / Carrier ceased. Recorded voice: ‘Reference west. No. East. Hold the last reading.’")),
			Line(TEXT("e10.relay"), TEXT("02:17:47 / Local relay impact and reference motion observed.")),
			Line(TEXT("e10.final"), TEXT("Final observation / Not entered.")) };
		Out.Add(C);
	}
	{
		FS47BoardCard C;
		C.Id = TEXT("hut.rx"); C.Stamp = TEXT("RECEIVER"); C.Title = TEXT("IN THE HUT, TONIGHT"); C.Kind = TEXT("thing"); C.bHave = S.bHeard;
		C.Where = TEXT("The receiver on the hut workbench. Switch it on and listen.");
		C.Text = TEXT("A carrier, very weak, and under it a man's voice, far away: \"Reference west. No. East. Hold the last reading.\" Then only the carrier.");
		C.X = 890; C.Y = 330; C.W = 276;
		Out.Add(C);
	}
	return Out;
}

// ---------- lifecycle ----------

void US47Chapter3::Begin(const S47J::FObj& Saved)
{
	Reset();
	bActive = true; bStarted = true;
	S = Saved.IsValid() ? FS47Ch3State::Load(Saved) : FS47Ch3State();
	// (Prologue.ts books(): the board reads the books of the chapters that have started)
	if (M && Book) M->Books.AddUnique(Book.Get());
	// chapter one's camera and wet bench answer to this chapter now
	if (US47Chapter1* C1 = Ch1())
	{
		FS47FieldShots F;
		F.FrameNo = [this] { return FrameNo(); };
		F.Check = [this] { return Check(); };
		F.Expose = [this] { Expose(); };
		F.Rejected = [this] { S.Rejected++; };
		C1->Field = F;
		C1->Lab = [this] { return UseBench(); };
	}
	ApplyWorld();
	if (!Saved.IsValid())
	{
		Note(FString::Printf(TEXT("%s. N. Vega wants me at the cut cable at STATION 01 before I bring her anything. The service truck is on the pad below the east landing. The keys are always in it."), *Time()));
		Toast(TEXT("The service truck is parked on the pad below the east landing."), 4);
	}
	Save();
	static bool bDriven = false;
	FString Stop;
	if (!bDriven && FParse::Value(FCommandLine::Get(), TEXT("S47Ch3Drive="), Stop)) { bDriven = true; TestDrive(Stop); }
}

void US47Chapter3::Reset()
{
	US47Audio* A = Au();
	bActive = false; bStarted = false; bRinging = false;
	S = FS47Ch3State(); Busy.Reset(); PanelRefresh = nullptr; CallT = -1; bRadioOn = false;
	if (US47Chapter1* C1 = Ch1()) { C1->Field.Reset(); C1->Lab = nullptr; }
	if (A && A->Ctx.IsValid()) { A->Stop(RING); A->Radio({}); }
	if (US47Station* Site = this->Site())
	{
		Site->SetLampCovered(false); Site->SetReceiver(false);
		FS47Obj Hs = Site->Obj(TEXT("fieldPhoneHandset"));
		if (Hs) { Hs.SetVisible(true); Hs.SetRotZ(0); }
	}
	if (M && Book) M->Books.Remove(Book.Get());
	// main.ts resets its stationTaken when a night starts or a save is opened (startGame), the
	// times the host resets the chapters (Prologue.reset) and begins this one again
	if (StationTaken) StationTaken->Reset();
}

S47J::FObj US47Chapter3::SaveState() const
{
	return bStarted ? S.Save() : nullptr;
}

// Puts the world in the state of S, after begin or once the station has loaded.
void US47Chapter3::ApplyWorld()
{
	if (US47Station* Site = this->Site()) { Site->SetLampCovered(S.bLampCovered); Site->SetReceiver(S.bHeard); }
	if (S.bReadE09a) File(DocTransit());
	if (S.bReadE10) File(DocTiming());
	if (S.bP06) File(DocFinding(TEXT("p06")));
	if (S.bP07) File(DocFinding(TEXT("p07")));
	if (S.bP08) File(DocFinding(TEXT("p08")));
	if (S.bP09) File(DocFinding(TEXT("p09")));
	if (S.Dev >= 3 && S.F3 && S.F4)
	{
		if (US47Chapter1* C1 = Ch1()) { C1->HangPrint(2, *S.F3); C1->HangPrint(3, *S.F4); }
		File(DocPhoto(*S.F3)); File(DocPhoto(*S.F4));
	}
	if (S.Stage == TEXT("call") && !S.bCalled) CallT = Gt() + 2.5; // it rings again
}

// ---------- saving ----------

FString US47Chapter3::SaveBlock() const
{
	if (Driving()) return TEXT("Not while driving. The game saves when you get there.");
	if (Busy == TEXT("tank")) return TEXT("Not while the film is in the tank.");
	return FString();
}

// ---------- objective and notebook ----------

FString US47Chapter3::Objective() const
{
	const FString& St = S.Stage;
	const FString Ar = Area();
	if (St == TEXT("to-truck"))
	{
		if (!Driving()) return Ar == TEXT("diner") ? TEXT("STATION 01 // BACK IN THE TRUCK, THEN RIGHT ONTO THE SURVEY TRACK") : TEXT("SERVICE YARD // TAKE THE SARO TRUCK TO STATION 01");
		// one line the whole way: a change of line where the truck goes from one area into the next
		// would give the change away (World.ts)
		return TEXT("STATION 01 // SOUTH ON THE HIGHWAY, RIGHT ONTO THE SURVEY TRACK TO THE GATE");
	}
	if (St == TEXT("station"))
	{
		if (!S.bP06 && !S.bP07) return TEXT("STATION 01 // DOCUMENT THE MOVED MARKER OR RUN A LOCAL LAMP NULL TEST");
		if (!S.bP06) return S.F3 ? TEXT("STATION 01 // ON THE CASE BOARD: WHICH FIXED MARKER MOVED (P06)") : TEXT("STATION 01 // EXPOSE FRAME 03 AND RECORD WHICH FIXED MARKER MOVED");
		if (!S.bP07) return TEXT("STATION 01 // COVER THE LOCAL LAMP AND OBSERVE A NORMAL NULL TEST");
		if (!S.bP08) return S.F4 ? TEXT("STATION 01 // ON THE CASE BOARD: WHAT THE CABLE BREAK SHOWS (P08)") : TEXT("STATION 01 // INSPECT THE CABLE BREAK AND EXPOSE FRAME 04");
		return TEXT("STATION 01 // MATCH THE 1947 AND 1986 TIMING RECORDS ON THE CASE BOARD");
	}
	if (St == TEXT("call")) return bRinging ? TEXT("STATION 01 // THE FIELD TELEPHONE IS RINGING") : TEXT("STATION 01 // FIELD RECORD COMPLETE");
	if (St == TEXT("return"))
	{
		if (!Driving()) return TEXT("STATION 01 // DRIVE BACK TO SARO WITH THE FIELD ROLL");
		return TEXT("SARO // THE TRACK TO THE HIGHWAY, NORTH, UP THE RAMP TO THE TRUCK PAD");
	}
	if (St == TEXT("develop"))
	{
		if (S.Dev == 0) return TEXT("SARO PHOTOLAB // DEVELOP FRAMES 03 AND 04 AT THE WET BENCH");
		if (S.Dev == 1) return TEXT("SARO PHOTOLAB // TRANSFER THE FIELD PRINTS TO THE FIXER");
		return TEXT("SARO PHOTOLAB // COLLECT THE FIELD PRINTS");
	}
	if (St == TEXT("complete")) return TEXT("ROOM 6 // SIERRA MOTOR COURT / NEXT AREA NOT YET PLAYABLE");
	return FString();
}

TArray<FS47Task> US47Chapter3::Tasks() const
{
	TArray<FS47Task> T = { { TEXT("Take the service truck to STATION 01"), S.bArrived } };
	if (S.bArrived) T.Append({
		{ TEXT("Read the transit record (E09A)"), S.bReadE09a },
		{ TEXT("Expose FRAME 03: the empty footing and the marked post"), S.F3.IsSet() },
		{ TEXT("Record which fixed marker moved (P06)"), S.bP06 },
		{ TEXT("Cover the field lamp and observe the null test (P07)"), S.bP07 },
		{ TEXT("Inspect the cable cut and expose FRAME 04"), S.bCableInspected && S.F4.IsSet() },
		{ TEXT("Record what the cable break shows (P08)"), S.bP08 },
		{ TEXT("Listen to the receiver in the hut"), S.bHeard },
		{ TEXT("Read the 1947 timing log (E10)"), S.bReadE10 },
		{ TEXT("Match the two timing records (P09)"), S.bP09 },
	});
	// once the player has been in at the bench: the drawer under it, which nothing asks for
	if (S.bHeard || S.bReadE10) T.Add({ TEXT("The drawer under the hut bench (optional)"), S.Fb.IsValid() });
	if (S.bP09) T.Add({ TEXT("Answer the field telephone"), S.bCalled });
	if (S.bCalled) T.Append({
		{ TEXT("Drive back to SARO"), S.Stage == TEXT("develop") || S.Stage == TEXT("complete") },
		{ TEXT("Develop FRAMES 03 and 04 in the photo lab"), S.Dev >= 3 },
	});
	return T;
}

// ---------- the truck ----------

TOptional<US47Chapter3::FTrip> US47Chapter3::TruckTrip(const FString& Place)
{
	(void)Place;
	if (!bActive) return {};
	if (S.Stage == TEXT("to-truck")) return FTrip{ TEXT("Drive to STATION 01"), [this] { if (M && M->World) M->World->DriveOut(); } };
	if (S.Stage == TEXT("return")) return FTrip{ TEXT("Drive back to SARO"), [this] { if (M && M->World) M->World->DriveBack(); } };
	return {};
}

void US47Chapter3::BindSaroTruck(const FS47Obj& Object)
{
	if (!M || !M->Inter) return;
	FS47Interactable It;
	It.Id = TEXT("truck"); It.Object = Object; It.Range = 3.2;
	It.Label = [this] { return M && M->World && M->World->TruckChoice ? M->World->TruckChoice(TEXT("saro")).Label : FString(); };
	It.Use = [this] { if (M && M->World && M->World->TruckChoice) M->World->TruckChoice(TEXT("saro")).Use(); };
	It.Examine = [] { return FString(TRUCK_EXAMINE); };
	M->Inter->Add(It);
}

void US47Chapter3::ArrivedStation()
{
	if (S.bArrived) return;
	S.bArrived = true;
	if (S.Stage == TEXT("to-truck")) S.Stage = TEXT("station");
	Note(FString::Printf(TEXT("%s. STATION 01. A stucco hut, a transit on a concrete pier, marker posts, and a cable on the ground. The vehicle gate is chained; the walk gate beside it is open."), *Time()));
	Toast(TEXT("STATION 01. The transit stands on its pier inside the fence."), 4);
	Milestone();
}

void US47Chapter3::ArrivedSaro()
{
	if (S.Stage != TEXT("return")) return;
	S.Stage = TEXT("develop");
	Note(FString::Printf(TEXT("%s. Back at SARO with the field roll: FRAME 03 and FRAME 04, still sealed. The photo lab is east of the walk."), *Time()));
	Toast(TEXT("Back at SARO. The film goes to the wet bench in the photo lab."), 4);
	Milestone();
}

// ---------- the station ----------

void US47Chapter3::BindSite(US47Station* Site)
{
	if (!bBound && Site && M && M->Inter)
	{
		bBound = true;
		US47Inter* Inter = M->Inter;
		auto On = [this] { return bActive && Area() == TEXT("station01"); };
		auto Add = [Inter, Site, On](const TCHAR* Id, const TCHAR* Key, TFunction<FString()> Label, TFunction<void()> Use, double Range = 2.6)
		{
			const FS47Obj O = Site->Proxy(Key);
			if (!O) return;
			FS47Interactable It;
			It.Id = Id; It.Object = O; It.Range = Range;
			It.Label = [On, Label] { return On() ? Label() : FString(); };
			It.Use = MoveTemp(Use);
			Inter->Add(It);
		};
		Add(TEXT("s1transit"), TEXT("transit"), [this] { return FString(S.bReadE09a ? TEXT("Lay out the field record") : TEXT("Read the transit record (E09A)")); }, [this] { UseTransit(); }, 2.8);
		Add(TEXT("s1footing"), TEXT("footing"), [] { return FString(TEXT("Concrete footing")); }, [this] { Look(TEXT("footing")); });
		Add(TEXT("s1markerA"), TEXT("markerA"), [] { return FString(TEXT("Marker post")); }, [this] { Look(TEXT("markerA")); }, 3.2);
		Add(TEXT("s1markerB"), TEXT("markerB"), [] { return FString(TEXT("Comparison marker")); }, [this] { Look(TEXT("markerB")); }, 3.2);
		Add(TEXT("s1stakes"), TEXT("stakesC"), [] { return FString(TEXT("Survey stakes")); }, [this] { Look(TEXT("stakes")); }, 3.5);
		Add(TEXT("s1diagram"), TEXT("diagram"), [] { return FString(TEXT("Framed diagram")); }, [this] { Look(TEXT("diagram")); });
		Add(TEXT("s1lamp"), TEXT("lamp"), [this] { return FString(S.bLampCovered ? TEXT("Field lamp (covered)") : TEXT("Field lamp")); }, [this] { OpenLamp(); });
		Add(TEXT("s1cable"), TEXT("cable"), [this] { return FString(S.bCableInspected ? TEXT("Cable break") : TEXT("Inspect the cable")); }, [this] { UseCable(); }, 2.4);
		Add(TEXT("s1hutDoor"), TEXT("hutDoor"), [this] { return FString(Taken && Taken->Shut && Taken->Shut() ? TEXT("Hut door") : TEXT("Field hut")); },
			[this] { if (Taken && Taken->Shut && Taken->Shut()) { if (Taken->TryDoor) Taken->TryDoor(); } else Look(TEXT("hut")); });
		Add(TEXT("s1receiver"), TEXT("receiver"), [this] { return FString(S.bHeard ? TEXT("Receiver") : TEXT("Switch on the receiver")); }, [this] { UseReceiver(); });
		Add(TEXT("s1timing"), TEXT("timingLog"), [this] { return FString(S.bReadE10 ? TEXT("Lay out the field record") : TEXT("Read the timing log (E10)")); }, [this] { UseTimingLog(); });
		Add(TEXT("s1phone"), TEXT("fieldPhone"), [this] { return FString(bRinging ? TEXT("Answer the field telephone") : TEXT("Field telephone")); }, [this] { UsePhone(); });
		Add(TEXT("s1gate"), TEXT("gate"), [] { return FString(TEXT("Vehicle gate")); }, [this] { Toast(TEXT("Chained and padlocked. N. Vega holds the key. The walk gate beside it is open."), 3.6f); });
		// the truck outside the gate, any time it stands there (with the truck somewhere else,
		// World.parkAt has put its handle out of the way: nothing to use)
		const FS47Obj Spot = Site->Proxy(TEXT("truckSpot"));
		if (Spot)
		{
			FS47Interactable It;
			It.Id = TEXT("s1truck"); It.Object = Spot; It.Range = 3.2;
			It.Label = [this, Spot] { return Area() == TEXT("station01") && Spot.Position().Y > -100 && M->World && M->World->TruckChoice ? M->World->TruckChoice(TEXT("station01")).Label : FString(); };
			It.Use = [this] { if (M->World && M->World->TruckChoice) M->World->TruckChoice(TEXT("station01")).Use(); };
			It.Examine = [this, Spot]
			{
				if (!(Area() == TEXT("station01") && Spot.Position().Y > -100)) return FString();
				const FString T = Taken && Taken->Truck ? Taken->Truck() : FString();
				return T.IsEmpty() ? FString(TRUCK_EXAMINE) : T;
			};
			Inter->Add(It);
		}
	}
	if (bActive) ApplyWorld();
}

void US47Chapter3::Look(const FString& What)
{
	const bool bFirst = !S.SeenOf(What);
	S.SeenOf(What) = true;
	if (What == TEXT("footing"))
	{
		Toast(TEXT("A square concrete footing on the transit sight line. Four rusted anchor bolts, and nothing on them."), 4);
		if (bFirst) Note(TEXT("The footing on the transit sight line is empty: four rusted anchor bolts in a square of paler concrete."));
	}
	else if (What == TEXT("markerA"))
	{
		Toast(TEXT("A marker post with an outlined triangle and a short bar beneath it. It stands a few metres off the sight line."), 4.2f);
		if (bFirst) Note(TEXT("A post carrying the outlined triangle with a short bar stands a few metres off the transit sight line, away from the empty footing."));
	}
	else if (What == TEXT("markerB"))
	{
		Toast(TEXT("B, the comparison marker west of the transit: a vertical vane plate on a post, still set in its footing."), 4);
		if (bFirst) Note(TEXT("B, the comparison marker west of the transit, is still in its footing."));
	}
	else if (What == TEXT("stakes"))
	{
		Toast(TEXT("Old wooden survey stakes run east into the dark, out past the fence. C, the closing sight line."), 4);
		if (bFirst) Note(TEXT("Weathered survey stakes run east past the fence: C, the closing sight line, as it was laid out in 1947."));
	}
	else if (What == TEXT("diagram"))
	{
		if (US47UI* U = Ui()) U->Document(DocDiagram(), [] {});
		File(DocDiagram());
		if (bFirst) Note(TEXT("A framed copy of the 1947 arrangement hangs in the hut: A, B and C, signed N. Vega."));
	}
	else if (What == TEXT("hut"))
	{
		Toast(TEXT("The field hut. Workbench, receiver, ledger, a telephone on the wall. The door has not been locked in years."), 4);
	}
	if (bFirst) Save();
}

FString US47Chapter3::TransitImage()
{
	if (Images.Transit.IsEmpty())
	{
		US47Station* St = Site();
		if (!St) return S47Drawings::TransitRecord(FVector2D(0, -8), FVector2D(-9, 0), FVector2D(12, 0)); // drawn again once the station is built
		auto Rel = [St](const TCHAR* K, const FVector2D& Fallback)
		{
			const FS47Obj T = St->Proxy(TEXT("transit")), O = St->Proxy(K);
			if (!T || !O) return Fallback;
			const FVector A = T.WorldPosition(), B = O.WorldPosition();
			return FVector2D(B.X - A.X, B.Z - A.Z);
		};
		const FVector2D C = Rel(TEXT("stakesC"), FVector2D(12, 0));
		Images.Transit = S47Drawings::TransitRecord(Rel(TEXT("footing"), FVector2D(0, -8)), Rel(TEXT("markerB"), FVector2D(-9, 0)), C);
	}
	return Images.Transit;
}

// Read once, the transit record is on the board with the rest; after that the transit is where
// the field record is laid out.
void US47Chapter3::UseTransit()
{
	if (S.bReadE09a) { BoardOpen(); return; }
	S.bReadE09a = true;
	File(DocTransit());
	Note(TEXT("E09A, the transit record: A sat in the concrete footing on the transit sight line. B is the fixed comparison marker to the west. C is the closing sight line."));
	Save();
	if (US47UI* U = Ui()) U->Document(DocTransit(), [this] { Toast(TEXT("E09A is filed, and pinned on the case board with the station's other records."), 3.6f); });
}

void US47Chapter3::UseCable()
{
	if (!S.bCableInspected)
	{
		const FS47Result R = Inspect();
		Toast(R.bOk ? FString(TEXT("Two clean faces, opposite each other, copper bright where the sheath ends. Take a near frame of them.")) : R.Text, 4);
		return;
	}
	Toast(S.F4 ? TEXT("The cut cable. FRAME 04 has it, close up.") : TEXT("Two clean faces, opposite each other. Take a near frame of the cut."), 3.4f);
}

void US47Chapter3::UseReceiver()
{
	US47Audio* A = Au();
	US47Station* St = Site();
	const FS47Obj Rx = St ? St->Proxy(TEXT("receiver")) : FS47Obj();
	const TOptional<FVector> At = Rx ? TOptional<FVector>(Pos(Rx)) : TOptional<FVector>();
	if (S.bHeard) { Toast(TEXT("The carrier, steady under the static. The voice does not come back."), 3.4f); return; }
	if (St) St->SetReceiver(true);
	if (At && A) { A->Radio(*At); bRadioOn = true; }
	if (A) A->Sfx(TEXT("click"), Snd(0.5f, 1.f, At));
	if (Host) Host->SetCinematic(true);
	auto Cap = [this](double T, const FString& Text, float Secs) { After(T, [this, Text, Secs] { Toast(Text, Secs); }); };
	Cap(0.4, TEXT("The dial warms to amber. A carrier comes up out of the static, very weak."), 3.4f);
	FS47PhoneCue Line;
	Line.By = TEXT("THE LINE"); Line.Text = TEXT("Reference west. No. East. Hold the last reading.");
	const TOptional<FS47CallTimeline> Recorded = A ? A->PhoneCall({ Line }, 4.0f) : TOptional<FS47CallTimeline>();
	if (!Recorded) After(4.0, [this] { if (US47Audio* A2 = Au()) A2->RadioVoice(4.6f); });
	Cap(4.0, TEXT("Under the carrier, a man's voice, far away: \"Reference west. No. East. Hold the last reading.\""), Recorded && Recorded->Cues.Num() ? Recorded->Cues[0].Duration : 5.2f);
	After(FMath::Max(9.4, Recorded ? double(Recorded->Duration) : 0.0), [this]
	{
		if (Host) Host->SetCinematic(false);
		S.bHeard = true;
		Note(TEXT("The receiver in the hut, on the carrier: a man's voice, far away. \"Reference west. No. East. Hold the last reading.\" Then only the carrier."));
		Toast(TEXT("Then only the carrier."), 2.6f);
		if (S.bReadE10) File(DocTiming());   // the 1986 line goes under the 1947 one
		BoardRefresh();
		Save();
	});
}

void US47Chapter3::UseTimingLog()
{
	if (S.bReadE10) { BoardOpen(); return; }
	S.bReadE10 = true;
	File(DocTiming());
	Note(TEXT("E10, the 1947 timing log: 02:17:00 carrier ceased, a recorded voice. 02:17:47 relay impact and reference motion. Final observation not entered."));
	Save();
	if (US47UI* U = Ui()) U->Document(DocTiming(), [this] { Toast(TEXT("E10 is filed, and pinned on the case board."), 3); });
}

// ---------- P07 at the field lamp ----------

void US47Chapter3::OpenLamp()
{
	S47Panels::FNullTest O;
	O.Status = [this] { return S.bP09 ? FString(TEXT("P06 TO P09 RECORDED / Field timing linked / Return to SARO")) : Objective(); };
	O.State = [this] { S47Panels::FNullTestState St; St.bCovered = S.bLampCovered; St.bObserved = S.bLampObserved; St.bP07 = S.bP07; return St; };
	O.Supported = SUPPORTED_LAMP;
	O.OnLamp = [this](bool bCover) { SetLamp(bCover); };
	O.OnObserve = [this] { return ObserveLamp(); };
	O.Hints = HINTS_LAMP;
	PanelRefresh = S47Panels::NullTest(Ui(), O).Refresh;
}

bool US47Chapter3::Near(const FString& Key, double R) const
{
	const US47Station* St = Site();
	const FS47Obj O = St ? St->Proxy(Key) : FS47Obj();
	return O && Dist(Pos(O)) <= R;
}

FS47Result US47Chapter3::SubmitMarker(const FString& C)
{
	if (!S.bReadE09a) return { false, TEXT("READ E09A / TRANSIT RECORD BEFORE RECORDING THE MARKER FINDING.") };
	if (!S.F3) return { false, TEXT("FRAME 03 FIRST / Expose the empty footing and the marked post together, then record which position changed.") };
	if (C != TEXT("a")) { S.Wrong06++; Save(); return { false, TEXT("The lamp and the cable do not identify the moved fixed point. Match the outlined triangle and short bar, then record A.") }; }
	if (!S.bP06)
	{
		S.bP06 = true;
		File(DocFinding(TEXT("p06")));
		Note(TEXT("P06: fixed point A has moved relative to the transit record. B has not. The record does not say who moved A."));
		if (US47Audio* A = Au()) A->Sfx(TEXT("click"), Snd(0.5f));
		Milestone();
	}
	return { true, SUPPORTED_MARKER };
}

void US47Chapter3::SetLamp(bool bCover)
{
	S.bLampCovered = bCover;
	US47Station* St = Site();
	if (St) St->SetLampCovered(bCover);
	const FS47Obj O = St ? St->Proxy(TEXT("lamp")) : FS47Obj();
	if (US47Audio* A = Au()) A->Sfx(TEXT("switch"), Snd(0.35f, 1.3f, O ? TOptional<FVector>(Pos(O)) : TOptional<FVector>()));
	Toast(bCover ? TEXT("LOCAL LAMP COVERED // NULL TEST READY") : TEXT("LOCAL LAMP EXPOSED"), 2.6f);
	Save();
}

FS47Result US47Chapter3::ObserveLamp()
{
	if (!Near(TEXT("lamp"), 4)) return { false, TEXT("MOVE TO THE LOCAL LAMP BEFORE OBSERVING ITS NULL TEST.") };
	if (!S.bLampCovered) return { false, TEXT("COVER THE LOCAL LAMP BEFORE OBSERVING A NULL TEST.") };
	S.bLampObserved = true;
	if (!S.bP07)
	{
		S.bP07 = true;
		File(DocFinding(TEXT("p07")));
		Note(TEXT("P07: with the field lamp covered, the local null test is normal. The lamp is a local source, nothing more."));
		Toast(TEXT("Null test recorded. Uncover the lamp again when you need its light."), 4);
		BoardRefresh();
		Milestone();
	}
	return { true, SUPPORTED_LAMP };
}

FS47Result US47Chapter3::Inspect()
{
	if (!Near(TEXT("cable"), 3)) return { false, TEXT("MOVE TO THE CABLE CUT BEFORE INSPECTING ITS FACES.") };
	if (!S.bCableInspected)
	{
		S.bCableInspected = true;
		Note(TEXT("The cable runs from the junction box on the hut to the transit pier. South of the pier it is cut: two clean faces, opposite each other, copper bright where the sheath ends."));
		Save();
	}
	return { true, TEXT("CABLE INSPECTED / Follow the sheath to the opposed cut faces and take the near frame.") };
}

FS47Result US47Chapter3::SubmitCable(const FString& C)
{
	if (!S.bP06 || !S.bP07) return { false, TEXT("P08 REQUIRES BOTH THE MARKER FINDING AND THE LOCAL LAMP NULL TEST.") };
	if (!S.bCableInspected || !S.F4) return { false, TEXT("FRAME 04 FIRST / Inspect the cut faces and expose a near frame of the break.") };
	if (C != TEXT("deliberate-cut"))
	{
		S.Wrong08++; Save();
		return { false, C == TEXT("author-guilt")
			? TEXT("The faces show that the cable was cut on purpose. They do not show who cut it, or why.")
			: TEXT("The cut faces are opposed and clean. A lamp fault or ordinary drift does not account for that physical break.") };
	}
	if (!S.bP08)
	{
		S.bP08 = true;
		File(DocFinding(TEXT("p08")));
		Note(TEXT("P08: the cable was cut on purpose. A cut shows how, not who or why."));
		if (US47Audio* A = Au()) A->Sfx(TEXT("click"), Snd(0.5f));
		Milestone();
	}
	return { true, SUPPORTED_CABLE };
}

FS47Result US47Chapter3::SubmitTiming(const FString& C)
{
	if (!S.bP08) return { false, TEXT("RECORD THE MARKER AND CABLE FINDINGS BEFORE MATCHING THE TIMING LOG.") };
	if (!S.bReadE10) return { false, TEXT("READ E10 / HISTORICAL TIMING RECORD BEFORE SUBMITTING A TIME.") };
	if (!S.bHeard) return { false, TEXT("LISTEN TO THE HUT RECEIVER BEFORE MATCHING THE TWO RECORDS.") };
	if (C != TEXT("02:17:47")) { S.Wrong09++; Save(); return { false, TEXT("The matched reading arrives at the 47-second relay impact: 02:17:47. Do not infer a new event from the clock alone.") }; }
	if (!S.bP09)
	{
		S.bP09 = true;
		S.Stage = TEXT("call");
		File(DocFinding(TEXT("p09")));
		Note(TEXT("P09: the carrier ceased at 02:17:00 and the relay impact came 47 seconds later, at 02:17:47. Tonight's fragment repeats T. Vega's correction and adds nothing after it."));
		if (US47Audio* A = Au()) A->Sfx(TEXT("click"), Snd(0.5f));
		CallT = Gt() + 4.0;
		Milestone();
	}
	return { true, SUPPORTED_TIMING };
}

// ---------- Nora on the field telephone ----------

void US47Chapter3::Ring()
{
	US47Station* St = Site();
	const FS47Obj O = St ? St->Proxy(TEXT("fieldPhone")) : FS47Obj();
	if (!O || Area() != TEXT("station01")) { CallT = Gt() + 3; return; }
	bRinging = true;
	FS47Sound Snd_;
	Snd_.Gain = 0.9f; Snd_.Rate = 0.82f; Snd_.At = Pos(O);
	if (US47Audio* A = Au()) A->LoopSfx(RING, TEXT("phoneRing"), Snd_);
	After(0.8, [this] { Toast(TEXT("The field telephone on the hut wall is ringing."), 3.4f); });
}

void US47Chapter3::UsePhone()
{
	US47Audio* A = Au();
	if (!bRinging)
	{
		Toast(S.bCalled ? TEXT("The line is open, and quiet. She has said what she wanted to say.") : TEXT("A field telephone with a crank, on a line that leaves the hut through the wall. Nobody answers it."), 3.6f);
		return;
	}
	bRinging = false;
	if (A) A->Stop(RING, 0.02f);
	US47Station* St = Site();
	FS47Obj Handset = St ? St->Obj(TEXT("fieldPhoneHandset")) : FS47Obj();
	if (Handset) Handset.SetVisible(false);
	if (Host) Host->SetCinematic(true);
	const TOptional<FS47CallTimeline> Spoken = A ? A->PhoneCall(S47PhoneScripts::FieldNora()) : TOptional<FS47CallTimeline>();
	if (Spoken)
	{
		for (const FS47CallCue& Cue : Spoken->Cues) After(Cue.At, [this, Cue] { Toast(FString::Printf(TEXT("%s: \"%s\""), *Cue.By, *Cue.Text), FMath::Max(2.f, Cue.Duration)); });
	}
	else
	{
		if (A) A->CallLine(18.4f);
		auto Cap = [this](double T, const FString& Text, float Secs) { After(T, [this, Text, Secs] { Toast(Text, Secs); }); };
		Cap(0.3, TEXT("N. VEGA: \"So you went out there.\""), 2.6f);
		Cap(3.0, TEXT("N. VEGA: \"This line still rings in the motel office. It has since 1947. Nobody ever took it down.\""), 4.6f);
		Cap(7.8, TEXT("N. VEGA: \"You have seen the cable now. It did not break by itself.\""), 3.8f);
		Cap(11.8, TEXT("N. VEGA: \"Room 6. Bring the original record and your film, developed. Then I will tell you the rest.\""), 5.0f);
	}
	After(Spoken ? Spoken->Duration + 0.1 : 17.0, [this, Handset]
	{
		if (Host) Host->SetCinematic(false);
		if (Handset) FS47Obj(Handset).SetVisible(true);
		S.bCalled = true; S.Stage = TEXT("return");
		Note(FString::Printf(TEXT("%s. N. Vega called the field telephone. The line still rings in the motel office. She wants the original record and my film, developed, in room 6."), *Time()));
		Toast(TEXT("The line goes quiet. The truck is by the gate."), 3.4f);
		Milestone();
	});
}

// ---------- the field camera ----------

FString US47Chapter3::FrameNo() const
{
	if (S.F3 && S.F4) return TEXT("FRAME 05");
	if (!S.F4 && (S.F3 || Dist(Target(TEXT("cable"))) < 6)) return TEXT("FRAME 04");
	return TEXT("FRAME 03");
}

FVector US47Chapter3::Target(const FString& Which) const
{
	const US47Station* St = Site();
	if (!St) return FVector(0, -999, 0);
	return Which == TEXT("marker") ? St->MarkerTarget() : St->CableTarget();
}

// bNear (chapter one's ShotCheck): within the transit sight line or at the cable break, a miss is
// a refused case frame and never a free one (core/album.ts)
FS47ShotCheck US47Chapter3::Check03()
{
	US47FieldCamera* F = Fcam();
	US47Station* St = Site();
	const FVector T = Target(TEXT("marker"));
	const double D = Dist(T);
	const bool bNear = D <= 16;
	if (!S.bReadE09a) return { TEXT("READ THE TRANSIT RECORD E09A FIRST"), false, bNear };
	if (D > 16) return { TEXT("MOVE CLOSER TO THE TRANSIT SIGHT LINE"), false };
	if (D < 2.2) return { TEXT("STEP BACK // THE FOOTING AND THE POST IN ONE FRAME"), false, bNear };
	const FS47Obj Fo = St ? St->Proxy(TEXT("footing")) : FS47Obj(), Ma = St ? St->Proxy(TEXT("markerA")) : FS47Obj();
	const FVector Fp = Fo ? Pos(Fo) : T, Mp = Ma ? Pos(Ma) : T;
	if (!F || !F->InFrame(T, 0.18, 0.82, 0.15, 0.85) || !F->InFrame(Fp, 0.04, 0.96, 0.06, 0.94) || !F->InFrame(Mp, 0.04, 0.96, 0.06, 0.94))
		return { TEXT("FRAME THE EMPTY FOOTING AND THE MARKED POST TOGETHER"), false, bNear };
	if (St && !F->ClearLine(T, St->Occluders)) return { TEXT("VIEW OBSTRUCTED // FIND A CLEAR LINE OF SIGHT"), false, bNear };
	return { TEXT("FOOTING AND MARKED POST IN FRAME // READY"), true };
}

FS47ShotCheck US47Chapter3::Check04()
{
	US47FieldCamera* F = Fcam();
	const FVector T = Target(TEXT("cable"));
	const bool bNear = Dist(T) <= 3.2;
	if (!S.bCableInspected) return { TEXT("INSPECT THE CABLE CUT FIRST"), false, bNear };
	if (!bNear) return { TEXT("MOVE CLOSER TO THE CABLE BREAK"), false };
	if (!F || !F->InFrame(T, 0.22, 0.78, 0.18, 0.82)) return { TEXT("FRAME THE CUT CABLE ENDS"), false, bNear };
	return { TEXT("CUT FACES IN FRAME // READY"), true };
}

FS47ShotCheck US47Chapter3::Check()
{
	if (Area() != TEXT("station01") || !Site())
	{
		return { S.F3 && S.F4 ? TEXT("FIELD ROLL EXPOSED // DEVELOP IT AT THE WET BENCH") : TEXT("THE FIELD ROLL IS FOR STATION 01"), false };
	}
	if (S.F3 && S.F4) return { TEXT("FRAMES 03 AND 04 EXPOSED // DEVELOP AT SARO"), false };
	const TOptional<FS47ShotCheck> C4 = S.F4 ? TOptional<FS47ShotCheck>() : TOptional<FS47ShotCheck>(Check04());
	if (C4 && C4->bOk) return *C4;
	const TOptional<FS47ShotCheck> C3 = S.F3 ? TOptional<FS47ShotCheck>() : TOptional<FS47ShotCheck>(Check03());
	if (C3 && C3->bOk) return *C3;
	const bool bNear = ((C4 && C4->bNear) || (C3 && C3->bNear)) && !(FreeShot && FreeShot());
	FS47ShotCheck R = C4 && (!C3 || Dist(Target(TEXT("cable"))) < 6) ? *C4 : *C3;
	R.bNear = bNear;
	return R;
}

void US47Chapter3::Expose()
{
	US47FieldCamera* F = Fcam();
	US47Station* St = Site();
	if (!F || !St) return;
	const bool bCable = !S.F4 && Check04().bOk;
	const double Clock = Host ? Host->Clock() : 0.0;
	F->Sync();
	TArray<TPair<FString, FVector2D>> Targets;
	const TArray<FString> Keys = bCable ? TArray<FString>{ TEXT("cable") } : TArray<FString>{ TEXT("footing"), TEXT("markerA") };
	for (const FString& K : Keys)
	{
		const FS47Obj O = St->Proxy(K);
		const TOptional<FVector> P = K == TEXT("cable") ? TOptional<FVector>(Target(TEXT("cable"))) : O ? TOptional<FVector>(Pos(O)) : TOptional<FVector>();
		if (!P) continue;
		const FS47Projection R = F->Project(*P);
		if (R.bFront) Targets.Add({ K, FVector2D(R.U, R.V) });
	}
	// out here at night the camera needs its flash; nearer subjects get less of it
	const FVector Subject = bCable ? Target(TEXT("cable")) : Target(TEXT("marker"));
	const double Flash = FMath::Min(18.0, FMath::Max(3.0, Dist(Subject) * 2.4));
	// out on C's ground, in the picture, the third one (story/theThird.ts): in it from the moment it
	// is exposed, and nowhere but on the film
	const TOptional<FS47FilmShot> Th = !bCable && Third ? Third(F) : TOptional<FS47FilmShot>();
	if (Th) { Targets.Add({ TEXT("band"), FVector2D(Th->U, Th->V) }); if (Th->bOnC) Targets.Add({ TEXT("bandC"), FVector2D(Th->U, Th->V) }); }
	TWeakObjectPtr<US47Station> Ws = St;
	TWeakObjectPtr<US47FieldCamera> Wf = F;
	FS47ExposeOptions O;
	O.Before = [Ws, Wf, Flash, Th] { if (Ws.IsValid() && Wf.IsValid()) Ws->SetFlash(S47Three::ToThree(Wf->PhotoLoc), Flash); if (Th && Th->Before) Th->Before(); };
	O.After = [Ws, Th] { if (Ws.IsValid()) Ws->SetFlash({}); if (Th && Th->After) Th->After(); };
	if (Th && Th->Post) O.Post = Th->Post;
	O.Restore = [] {}; // this.d.view.restore(): the screen was never changed (the photograph is a capture of its own)
	O.Caption = bCable ? TEXT("STATION 01 // CABLE BREAK") : TEXT("STATION 01 // FIXED POINT A");
	O.Time = ClockText(Clock);
	O.Seed = FMath::FloorToDouble(Clock * 1000);
	TArray<FColor> Print;
	F->Expose(O, Print);
	// (this.d.view.draw(): the next frame draws as usual)
	FS47Photo Photo;
	Photo.Id = bCable ? TEXT("frame04") : TEXT("frame03");
	Photo.Subject = bCable ? TEXT("STATION 01 / cable break") : TEXT("STATION 01 / fixed point A");
	Photo.Method = TEXT("field");
	Photo.Clock = Clock;
	Photo.Url = S47Image::ToDataUrl(Print, US47FieldCamera::PRINT_W, US47FieldCamera::PRINT_H, true, 86);
	Photo.Targets = Targets;
	UE_LOG(LogS47Ch3, Display, TEXT("S47: %s exposed, %d targets, %d kB"), *Photo.Id, Targets.Num(), Photo.Url.Len() / 1024);
	if (US47Audio* A = Au()) A->Shutter();
	if (US47UI* U = Ui()) U->ShutterFlash();
	if (bCable)
	{
		S.F4 = Photo;
		Note(TEXT("Frame 04 exposed: the cable break, close up. Sealed film; it is developed at SARO."));
		Toast(TEXT("Frame 04 exposed. The roll is developed at SARO."), 3.5f);
	}
	else
	{
		S.F3 = Photo;
		Note(TEXT("Frame 03 exposed: the empty footing and the marked post in one frame. Sealed film; it is developed at SARO."));
		Toast(TEXT("Frame 03 exposed. The roll is developed at SARO."), 3.5f);
	}
	if (PanelRefresh) PanelRefresh();
	BoardRefresh();
	Save();
}

// ---------- the wet bench at SARO ----------

bool US47Chapter3::UseBench()
{
	if (!bActive) return false;
	US47UI* U = Ui();
	if (S.Stage != TEXT("develop") || !S.F3 || !S.F4)
	{
		S47Panels::WetBench(U, FString(), 0, S.Dev >= 3
			? TEXT("The tank is rinsed and empty.\nFrames 03 and 04 hang on the drying line and are filed in the case.")
			// Tom, 18 October: "how do you fill the tank?" Say where the film comes from.
			: !S.F3 || !S.F4
				? TEXT("The tank is empty. Nothing to develop yet.\nFRAMES 03 and 04 are taken at STATION 01: drive out in the truck on the pad, photograph the site, and bring the roll back here.")
				: TEXT("The tank is empty.\nThe field roll for STATION 01 is still in the camera. Bring it back to SARO in the truck."), [] {});
		return true;
	}
	if (Busy == TEXT("tank")) { Toast(TEXT("The tank is sealed. Give it a moment.")); return true; }
	if (!Busy.IsEmpty()) return true;
	const int32 Step = S.Dev == 0 ? 0 : S.Dev == 1 ? 2 : 3;
	S47Panels::WetBench(U, TEXT("FRAMES 03 + 04 / STATION 01 FIELD ROLL"), Step, FString(), [this]
	{
		US47Audio* A = Au();
		if (S.Dev == 0)
		{
			Busy = TEXT("tank");
			if (A) A->PourThree();
			Toast(TEXT("Field roll in the sealed tank."), 3);
			After(TANK_TIME, [this]
			{
				Busy.Reset(); S.Dev = 1; Save();
				if (US47Audio* A2 = Au()) A2->Beep(1500, 0.12f, 0.05f);
				Toast(TEXT("Contact prints ready. Transfer them to the fixer."), 3);
			}, TEXT("lab3"));
		}
		else if (S.Dev == 1)
		{
			Busy = TEXT("fix");
			if (A) A->PourThree();
			if (US47Chapter1* C1 = Ch1(); C1 && S.F3) C1->FixPrint(*S.F3);
			Toast(TEXT("Prints in the fixer. The images are becoming stable."), 3);
			After(FIX_TIME, [this]
			{
				Busy.Reset(); S.Dev = 2; Save();
				if (US47Audio* A2 = Au()) A2->Beep(1500, 0.12f, 0.05f);
				Toast(TEXT("Prints fixed. Collect them at the bench."), 3);
			}, TEXT("lab3"));
		}
		else Collect();
	});
	return true;
}

void US47Chapter3::Collect()
{
	if (!S.F3 || !S.F4) return;
	if (US47Chapter1* C1 = Ch1())
	{
		C1->ClearWetPrint();
		C1->HangPrint(2, *S.F3); C1->HangPrint(3, *S.F4);
	}
	if (US47Audio* A = Au()) A->Sfx(TEXT("thudSoft"), Snd(0.3f, 1.5f));
	S.Dev = 3;
	File(DocPhoto(*S.F3)); File(DocPhoto(*S.F4));
	Note(TEXT("Filed: PHOTO 03, the empty footing and the moved marker A. PHOTO 04, the cable break."));
	Save();
	S47Panels::FFieldPrints O;
	O.Prints = { { S.F3->Url, TEXT("FRAME 03 / FIXED POINT A") }, { S.F4->Url, TEXT("FRAME 04 / CABLE BREAK") } };
	O.Text = TEXT("The footing without its marker, and the marker standing where nobody recorded it. The cable, cut clean. Both prints go into the case with the original record, for room 6.");
	O.OnClose = [this] { Finish(); };
	O.OnLook = [this](int32 I, double U, double V) { if (I == 0 && BandInLook(U, V) && OnBand) OnBand(); };
	S47Panels::FieldPrints(Ui(), O);
}

// FRAME 03 looked at closer (three times, round where it was clicked: Panels.fieldPrints): the band
// is well inside what is shown.
bool US47Chapter3::BandInLook(double U, double V) const
{
	const TPair<FString, FVector2D>* B = S.F3 ? S.F3->Targets.FindByPredicate([](const TPair<FString, FVector2D>& T) { return T.Key == TEXT("band"); }) : nullptr;
	if (!B) return false;
	// in the print's terms: the window shown is a third of it, round the point clicked
	auto P = [](double Pu, double Pv) { return FVector2D((US47FieldCamera::PRINT_X + Pu * US47FieldCamera::PHOTO_W) / US47FieldCamera::PRINT_W, (US47FieldCamera::PRINT_Y + Pv * US47FieldCamera::PHOTO_H) / US47FieldCamera::PRINT_H); };
	const FVector2D C = P(U, V), Bp = P(B->Value.X, B->Value.Y);
	auto Mid = [](double X) { return (1 + 4 * X) / 6; };
	return FMath::Abs(Bp.X - Mid(C.X)) <= 0.14 && FMath::Abs(Bp.Y - Mid(C.Y)) <= 0.14;
}

void US47Chapter3::Finish()
{
	if (S.Stage == TEXT("complete")) return;
	S.Stage = TEXT("complete");
	Note(TEXT("FIELD RECORD FILED: STATION 01, P06 to P09, with frames 03 and 04. Next: room 6 at Sierra Motor Court, with the original 1947 record and the prints."));
	Milestone();
	TWeakObjectPtr<US47Chapter3> W = this;
	After(0.8, [W] { if (W.IsValid() && W->OnEnd) W->OnEnd(); });
}

// ---------- documents ----------

FS47Doc US47Chapter3::DocTransit()
{
	FS47Doc D;
	D.Id = TEXT("e09a"); D.Title = TEXT("E09A / Field transit record"); D.Kind = TEXT("typed"); D.Stamp = TEXT("E09A"); D.Page = TRANSIT; D.Image = TransitImage();
	D.Transcript = TEXT("From the steel frame on the transit pier. A plan of the 1947 set-up: A in its footing on the transit sight line, B to the west, C running off the sheet.");
	return D;
}

FS47Doc US47Chapter3::DocTiming() const
{
	FS47Doc D;
	D.Id = TEXT("e10"); D.Title = TEXT("E10 / Field timing record, 1947"); D.Kind = TEXT("typed"); D.Stamp = TEXT("E10");
	D.Page = S.bHeard ? FString(TIMING_1947) + TEXT("\n\n") + TIMING_1986 : FString(TIMING_1947);
	D.Transcript = TEXT("The ledger on the hut workbench. Times to the second, in pencil. The last line was never filled in.");
	return D;
}

FS47Doc US47Chapter3::DocDiagram() const
{
	FS47Doc D;
	D.Id = TEXT("s1diagram"); D.Title = TEXT("STATION 01 / Arrangement on the hut wall"); D.Kind = TEXT("typed"); D.Stamp = TEXT("1947");
	D.Page = TEXT("A: fixed survey point. B: comparison vane. C: closing sight line.\n\nThe framed copy of the original arrangement, signed N. Vega.");
	D.Image = S47Drawings::Arrangement(true);
	D.Transcript = TEXT("Behind dusty glass in the hut: the same arrangement as the original 1947 record, with all three references, A, B and C.");
	return D;
}

FS47Doc US47Chapter3::DocFinding(const FString& K) const
{
	FS47Doc D;
	D.Id = K; D.Kind = TEXT("typed"); D.Stamp = K.ToUpper();
	if (K == TEXT("p06")) { D.Title = TEXT("P06 / The moved fixed point"); D.Page = SUPPORTED_MARKER; D.Transcript = TEXT("Your finding at the transit. A is not where E09A puts it. Who moved it is not in the record."); }
	else if (K == TEXT("p07")) { D.Title = TEXT("P07 / Local lamp null test"); D.Page = SUPPORTED_LAMP; D.Transcript = TEXT("An ordinary control: the field lamp covered, the local condition normal."); }
	else if (K == TEXT("p08")) { D.Title = TEXT("P08 / The cable break"); D.Page = SUPPORTED_CABLE; D.Transcript = TEXT("The cable from the hut to the pier was cut on purpose. The cut does not say by whom."); }
	else { D.Title = TEXT("P09 / Two timing records"); D.Page = SUPPORTED_TIMING; D.Transcript = TEXT("Carrier lost at 02:17:00, relay impact at 02:17:47. The voice tonight is the same correction as in 1947."); }
	return D;
}

FS47Doc US47Chapter3::DocPhoto(const FS47Photo& P) const
{
	const bool bA = P.Id == TEXT("frame03");
	FS47Doc D;
	D.Id = P.Id; D.Kind = TEXT("photo"); D.Image = P.Url; D.Page = FString();
	D.Title = bA ? TEXT("PHOTO 03 / Fixed point A") : TEXT("PHOTO 04 / Cable break");
	D.Transcript = bA
		? FString::Printf(TEXT("Exposed at STATION 01 at %s. The empty footing on the transit sight line, and the marker post with the triangle and bar standing off it."), *ClockText(P.Clock))
		: FString::Printf(TEXT("Exposed at STATION 01 at %s. The cable south of the transit pier, cut clean, the two faces opposite each other."), *ClockText(P.Clock));
	return D;
}

// ---------- per frame ----------

void US47Chapter3::Update(double Dt, double T)
{
	(void)Dt;
	if (!bActive) return;
	US47Station* St = Site();
	if (FS47Obj Handset = St ? St->Obj(TEXT("fieldPhoneHandset")) : FS47Obj())
	{
		Handset.SetRotZ(bRinging ? FMath::Sin(T * 70) * 0.05 * (FMath::Sin(T * 2.2) > -0.2 ? 1 : 0) : 0);
	}
	// the receiver keeps its carrier once it is on, but only out at the station
	const bool bWant = S.bHeard && Area() == TEXT("station01");
	if (bWant != bRadioOn)
	{
		bRadioOn = bWant;
		const FS47Obj O = St ? St->Proxy(TEXT("receiver")) : FS47Obj();
		if (US47Audio* A = Au()) A->Radio(bWant && O ? TOptional<FVector>(Pos(O)) : TOptional<FVector>());
	}
	if (CallT >= 0 && Gt() >= CallT && !(Host && Host->IsCinematic()) && !(Ui() && Ui()->Modal())) { CallT = -1; Ring(); }
	if (Area() == TEXT("station01") && !Driving() && !S.bArrived) ArrivedStation();
	if (US47UI* U = Ui()) U->Objective(Objective());
}

// ---------- the host (Prologue.ts) ----------

void US47Chapter3::BeginChapter3(const S47J::FObj& SavedCase)
{
	// Prologue.beginChapter3: after the call to the key holder, from the records room out to the
	// service truck. (setPhase('ch3'), the cinematic and the timers are the host's BeginChapter's,
	// and so is onCheckpoint('chapter3'); lookAssist is the prologue's own and is 0 by now.)
	if (!Host) return;
	Host->SetCinematic(false);
	Host->ClearTimers();
	const S47J::FObj Ch3 = S47J::Obj(SavedCase, TEXT("ch3"));
	if (Ch3.IsValid())
	{
		Host->NotesMutable() = S47J::Strs(SavedCase, TEXT("notes"));
		Host->SetClock(S47J::Num(SavedCase, TEXT("clock"), Host->Clock()));
	}
	else Host->SetClock(FMath::Max(Host->Clock(), 3 * 3600 + 40 * 60.0));
	if (US47UI* U = Ui()) U->Fade(false);
	Host->BeginChapter(TEXT("ch3"), Ch3);
}

void US47Chapter3::JumpChapter3()
{
	// Prologue.jumpChapter3: chapter three from a checkpoint, or for testing straight after chapter
	// two (without photographs) when there is no saved case.
	if (!Host) return;
	const S47J::FObj Saved = Host->LoadCase ? Host->LoadCase() : S47J::FObj();
	Host->AfterPrologue(Saved);
	S47J::FObj Done = Saved;
	if (!(Saved.IsValid() && S47J::Bool(S47J::Obj(Saved, TEXT("ch2")), TEXT("called"))))
	{
		const S47J::FObj Ch2 = S47J::New();
		S47J::Set(Ch2, TEXT("v"), 1); S47J::Set(Ch2, TEXT("stage"), TEXT("complete")); S47J::Set(Ch2, TEXT("answered"), true); S47J::Set(Ch2, TEXT("door"), true); S47J::Set(Ch2, TEXT("entered"), true);
		const S47J::FObj Read = S47J::New();
		S47J::Set(Read, TEXT("original"), true); S47J::Set(Read, TEXT("amended"), true); S47J::Set(Read, TEXT("lineage"), true); S47J::Set(Read, TEXT("index"), true);
		S47J::Set(Ch2, TEXT("read"), Read);
		S47J::Set(Ch2, TEXT("p04"), true); S47J::Set(Ch2, TEXT("p05"), true);
		S47J::Set(Ch2, TEXT("finding"), TEXT("omitted-c")); S47J::Set(Ch2, TEXT("destination"), TEXT("old-survey-station")); S47J::Set(Ch2, TEXT("surveyId"), TEXT("STATION 01")); S47J::Set(Ch2, TEXT("marker"), TEXT("triangle-bar"));
		S47J::Set(Ch2, TEXT("wrong04"), 0); S47J::Set(Ch2, TEXT("wrong05"), 0); S47J::Set(Ch2, TEXT("called"), true);
		const S47J::FObj Ch1 = S47J::New();
		S47J::Set(Ch1, TEXT("v"), 1); S47J::Set(Ch1, TEXT("stage"), TEXT("complete")); S47J::Set(Ch1, TEXT("eventClock"), Host->EventClockSec());
		S47J::Set(Ch1, TEXT("camera"), true); S47J::Set(Ch1, TEXT("door"), true); S47J::Set(Ch1, TEXT("labDoor"), true); S47J::Set(Ch1, TEXT("log"), true); S47J::Set(Ch1, TEXT("returned"), true);
		S47J::Set(Ch1, TEXT("dev1"), 3); S47J::Set(Ch1, TEXT("dev2"), 3); S47J::Set(Ch1, TEXT("ref"), true); S47J::Set(Ch1, TEXT("hyp"), TEXT("light")); S47J::Set(Ch1, TEXT("method"), TEXT("passive"));
		S47J::Set(Ch1, TEXT("observed"), true); S47J::Set(Ch1, TEXT("concluded"), true); S47J::Set(Ch1, TEXT("filed"), true);
		S47J::Set(Ch1, TEXT("wrongRef"), 0); S47J::Set(Ch1, TEXT("wrongHyp"), 0); S47J::Set(Ch1, TEXT("wrongConcl"), 0); S47J::Set(Ch1, TEXT("rejected"), 0);
		Done = S47J::New();
		S47J::Set(Done, TEXT("s"), Ch1);
		S47J::Set(Done, TEXT("ch2"), Ch2);
		S47J::Set(Done, TEXT("notes"), Host->Notes());
		S47J::Set(Done, TEXT("clock"), Host->EventClockSec() + 80 * 60);
	}
	// beginChapter1(done): chapter one's Begin takes the whole case (its notes, its clock, its state)
	if (Host->Chapter(TEXT("ch1"))) Host->BeginChapter(TEXT("ch1"), Done);
	// beginChapter2(done)
	if (US47Chapter2* C2 = Cast<US47Chapter2>(Host->Chapter(TEXT("ch2")))) C2->BeginChapter2(Done);
	BeginChapter3(S47J::Has(Done, TEXT("ch3")) ? Done : S47J::FObj());
}

// ---------- the test drive ----------

void US47Chapter3::TestDrive(const FString& Stop)
{
	// tools/chapter3.py's moves, done from here: the station (driven to with -S47Drive, or put there),
	// E09A at the transit, FRAME 03 from its photo spot, P06 on the board, the null test at the lamp,
	// the cable and FRAME 04, P08, the ledger and the receiver, P09, N. Vega on the field telephone,
	// back at SARO, the wet bench and the field prints. Each stop holds the game there.
	if (!M || !M->Player || !M->Inter || !Host || !M->World) return;
	TWeakObjectPtr<US47Chapter3> W = this;
	FString DriveArg;
	const bool bDriven = FParse::Value(FCommandLine::Get(), TEXT("S47Drive="), DriveArg);
	FString DumpDir;
	FParse::Value(FCommandLine::Get(), TEXT("S47Ch3Dump="), DumpDir);
	auto Say = [](const FString& T) { UE_LOG(LogS47Ch3, Display, TEXT("S47: ch3 drive: %s"), *T); };
	// the player at (x, z), looking at t (as the tests' place and pitch: atan2(ty - 1.62, d))
	auto Aim = [W](double Px, double Pz, const FVector& T)
	{
		AS47Walker* P = W->M->Player;
		P->Place(Px, Pz, FMath::Atan2(-(T.X - Px), -(T.Z - Pz)));
		P->Pitch = FMath::Atan2(T.Y - 1.62, FMath::Sqrt(FMath::Square(T.X - Px) + FMath::Square(T.Z - Pz)));
	};
	// d metres from a proxy, on walkable ground, facing it (tools/chapter3.py STAND)
	auto Stand = [W, Aim](const TCHAR* Key, double D, double Dy) -> bool
	{
		US47Station* St = W->Site();
		const FS47Obj O = St ? St->Proxy(Key) : FS47Obj();
		if (!O) return false;
		const FVector T = O.WorldPosition() + FVector(0, Dy, 0);
		const FS47Area Hut = St->HutBounds();
		const bool bTIn = Hut.Contains(T.X, T.Z);
		const FS47Pose A = St->Arrive();
		const double Base = FMath::Atan2(A.X - T.X, A.Z - T.Z);
		for (int32 i = 0; i < 24; ++i)
		{
			const double Ang = Base + (i % 2 ? 1 : -1) * FMath::CeilToDouble(i / 2.0) * PI / 12;
			for (const double K : { 1.0, 0.8, 1.25, 0.6 })
			{
				const double X = T.X + FMath::Sin(Ang) * D * K, Z = T.Z + FMath::Cos(Ang) * D * K;
				if (!W->M->Player->Walkable(X, Z) || Hut.Contains(X, Z) != bTIn) continue;
				Aim(X, Z, T);
				return true;
			}
		}
		return false;
	};
	auto Dump = [DumpDir](const FString& Name, const FString& Url)
	{
		TArray<uint8> Bytes;
		if (!DumpDir.IsEmpty() && FBase64::Decode(Url.Mid(Url.Find(TEXT(",")) + 1), Bytes)) FFileHelper::SaveArrayToFile(Bytes, *(DumpDir / Name));
	};
	auto Close = [W] { if (W->Ui()) W->Ui()->Close(); };

	struct FStep { FString Name; double Wait; TFunction<bool()> Do; };
	TSharedRef<TArray<FStep>> Steps = MakeShared<TArray<FStep>>();
	auto Add = [&Steps](const TCHAR* Name, double Wait, TFunction<bool()> Do) { Steps->Add({ Name, Wait, MoveTemp(Do) }); };

	Add(TEXT("arrive"), 1.0, [W, bDriven, Say]
	{
		if (bDriven) return W->S.bArrived && !W->Driving();   // (waits for the drive to end at the gate)
		// put there: the truck on the gravel by the gate and the player out of it (World.placeAtStation)
		W->M->World->PlaceTruck(TEXT("station01"));
		W->M->World->Enter(TEXT("station01"));
		W->M->World->PlaceAtStation();
		Say(TEXT("put at the station"));
		return true;
	});
	Add(TEXT("hut"), 1.0, [W, Say, Aim]
	{
		const FVector P = W->M->Player->Pos;
		Aim(P.X, P.Z, W->Site() ? W->Site()->Point(TEXT("hutDoor")) : FVector::ZeroVector);
		// (the walk gap beside the vehicle gate is open from chapter three on: Station01.setWalkGate)
		Say(FString::Printf(TEXT("at the hut, stage %s, arrived %d, area %s, walk gap %s, objective %s"), *W->S.Stage, W->S.bArrived, *W->Area(),
			W->M->Player->Walkable(-22.3, 8026.0) ? TEXT("open") : TEXT("chained"), *W->Objective()));
		return true;
	});
	Add(TEXT("transit"), 1.0, [W, Say, Stand, Close, Dump, Stop]
	{
		const bool bStood = Stand(TEXT("transit"), 1.6, 0);
		W->M->Inter->UseId(TEXT("s1transit"));
		Say(FString::Printf(TEXT("transit stood %d, read E09A %d, label now %s"), bStood, W->S.bReadE09a, *W->M->Inter->LabelOf(TEXT("s1transit"))));
		Dump(TEXT("transit.png"), W->TransitImage());
		if (Stop != TEXT("transit")) Close();
		return true;
	});
	Add(TEXT("frame03aim"), 1.0, [W, Say, Aim]
	{
		US47Station* St = W->Site();
		if (!St) return true;
		const FS47Pose P = St->PhotoSpot(TEXT("marker"));
		Aim(P.X, P.Z, St->MarkerTarget());
		if (US47Chapter1* C1 = W->Ch1()) C1->ToggleCamera();
		Say(FString::Printf(TEXT("camera raised at FRAME 03's spot (%.2f %.2f)"), P.X, P.Z));
		return true;
	});
	Add(TEXT("frame03"), 1.5, [W, Say, Dump]
	{
		Say(TEXT("before the shutter: ") + W->Check().Text);
		if (US47Chapter1* C1 = W->Ch1()) C1->Shutter();
		Say(FString::Printf(TEXT("frame 03 %s, %d targets"), W->S.F3 ? TEXT("exposed") : TEXT("refused"), W->S.F3 ? W->S.F3->Targets.Num() : 0));
		if (W->S.F3) Dump(TEXT("frame03.jpg"), W->S.F3->Url);
		return true;
	});
	Add(TEXT("p06"), 1.0, [W, Say]
	{
		FS47RuleBook& B = *W->Book;
		const TOptional<FS47Result> R1 = B.Connect(TEXT("e09a.b"), TEXT("frame03"), false), R2 = B.Connect(TEXT("e09a.a"), TEXT("frame03"), false);
		const TOptional<FS47Result> R3 = B.Record(TEXT("P06"));
		Say(FString::Printf(TEXT("P06: [%s] [%s] [%s], wrong06 %d, p06 %d"), R1 ? *R1->Text : TEXT("-"), R2 ? *R2->Text : TEXT("-"), R3 ? *R3->Text : TEXT("-"), W->S.Wrong06, W->S.bP06));
		return true;
	});
	Add(TEXT("lamp"), 1.0, [W, Say, Stand, Close, Stop]
	{
		Stand(TEXT("lamp"), 1.4, 0);
		W->M->Inter->UseId(TEXT("s1lamp"));
		const FS47Result A = W->ObserveLamp();
		W->SetLamp(true);
		if (W->PanelRefresh) W->PanelRefresh();
		const FS47Result B = W->ObserveLamp();
		if (W->PanelRefresh) W->PanelRefresh();
		Say(FString::Printf(TEXT("null test: [%s] then [%s], p07 %d, hood %s"), *A.Text, *B.Text, W->S.bP07, W->Site() && W->Site()->bLampCovered ? TEXT("down") : TEXT("up")));
		if (Stop != TEXT("lamp")) { Close(); W->SetLamp(false); }
		return true;
	});
	Add(TEXT("frame04aim"), 1.0, [W, Say, Stand, Aim]
	{
		Stand(TEXT("cable"), 1.3, -0.3);
		W->M->Inter->UseId(TEXT("s1cable"));
		US47Station* St = W->Site();
		if (!St) return true;
		const FS47Pose P = St->PhotoSpot(TEXT("cable"));
		Aim(P.X, P.Z, St->CableTarget());
		if (US47Chapter1* C1 = W->Ch1()) C1->ToggleCamera();
		Say(FString::Printf(TEXT("cable inspected %d, camera raised at FRAME 04's spot"), W->S.bCableInspected));
		return true;
	});
	Add(TEXT("frame04"), 1.5, [W, Say, Dump]
	{
		Say(TEXT("before the shutter: ") + W->Check().Text);
		if (US47Chapter1* C1 = W->Ch1()) C1->Shutter();
		Say(FString::Printf(TEXT("frame 04 %s, %d targets"), W->S.F4 ? TEXT("exposed") : TEXT("refused"), W->S.F4 ? W->S.F4->Targets.Num() : 0));
		if (W->S.F4) Dump(TEXT("frame04.jpg"), W->S.F4->Url);
		return true;
	});
	Add(TEXT("p08"), 1.0, [W, Say]
	{
		FS47RuleBook& B = *W->Book;
		const TOptional<FS47Result> R1 = B.Connect(TEXT("frame04"), TEXT("e07.sig"), false), R2 = B.Connect(TEXT("frame04"), TEXT("P07"), false);
		const TOptional<FS47Result> R3 = B.Record(TEXT("P08"));
		Say(FString::Printf(TEXT("P08: [%s] [%s] [%s], wrong08 %d, p08 %d"), R1 ? *R1->Text : TEXT("-"), R2 ? *R2->Text : TEXT("-"), R3 ? *R3->Text : TEXT("-"), W->S.Wrong08, W->S.bP08));
		return true;
	});
	Add(TEXT("board"), 0.5, [W, Say, Close, Stop]
	{
		W->BoardOpen();
		Say(FString::Printf(TEXT("the board opened, %d cards of this chapter's"), W->BoardCards().Num()));
		if (Stop != TEXT("board")) Close();
		return true;
	});
	Add(TEXT("e10"), 1.0, [W, Say, Stand, Close, Stop]
	{
		Stand(TEXT("timingLog"), 1.2, 0);
		W->M->Inter->UseId(TEXT("s1timing"));
		Say(FString::Printf(TEXT("E10 read %d"), W->S.bReadE10));
		if (Stop != TEXT("e10")) Close();
		return true;
	});
	Add(TEXT("receiver"), 1.0, [W, Say, Stand]
	{
		Stand(TEXT("receiver"), 1.2, 0);
		W->M->Inter->UseId(TEXT("s1receiver"));
		Say(TEXT("the receiver switched on"));
		return true;
	});
	Add(TEXT("p09"), 11.0, [W, Say]
	{
		FS47RuleBook& B = *W->Book;
		const TOptional<FS47Result> R1 = B.Connect(TEXT("hut.rx"), TEXT("e10.relay"), false), R2 = B.Connect(TEXT("hut.rx"), TEXT("e10.carrier"), false);
		const TOptional<FS47Result> R3 = B.Record(TEXT("P09"));
		Say(FString::Printf(TEXT("heard %d; P09: [%s] [%s] [%s], stage %s"), W->S.bHeard, R1 ? *R1->Text : TEXT("-"), R2 ? *R2->Text : TEXT("-"), R3 ? *R3->Text : TEXT("-"), *W->S.Stage));
		return true;
	});
	Add(TEXT("ring"), 1.0, [W, Say, Stand]
	{
		if (!W->bRinging) return false;   // (it rings four seconds after P09)
		Stand(TEXT("fieldPhone"), 1.1, 0);
		Say(FString::Printf(TEXT("the field telephone rings: %s"), *W->Objective()));
		return true;
	});
	Add(TEXT("phone"), 1.0, [W, Say]
	{
		W->M->Inter->UseId(TEXT("s1phone"));
		Say(TEXT("answered"));
		return true;
	});
	Add(TEXT("called"), 19.0, [W, Say]
	{
		Say(FString::Printf(TEXT("called %d, stage %s, objective %s, truck: %s"), W->S.bCalled, *W->S.Stage, *W->Objective(), *W->M->Inter->LabelOf(TEXT("s1truck"))));
		return true;
	});
	Add(TEXT("back"), 1.0, [W, Say, Aim]
	{
		// (the drive back is the drive helper's: here the truck is put on its pad and the player by it)
		W->M->World->PlaceTruck(TEXT("saro"));
		W->M->World->Enter(TEXT("saro"));
		W->ArrivedSaro();
		Aim(15.1, -3.9, FVector(15.1, 1.0, -5.4));
		Say(FString::Printf(TEXT("back at SARO: stage %s, objective %s"), *W->S.Stage, *W->Objective()));
		return true;
	});
	Add(TEXT("bench"), 1.0, [W, Say, Close, Stop]
	{
		W->M->Inter->UseId(TEXT("wetBench"));
		Say(TEXT("the wet bench"));
		if (Stop != TEXT("bench")) Close();
		return true;
	});
	Add(TEXT("wet"), 0.5, [W, Say]
	{
		W->S.Dev = 1;
		if (US47Chapter1* C1 = W->Ch1(); C1 && W->S.F3) C1->FixPrint(*W->S.F3);
		Say(TEXT("the field prints in the fixer"));
		return true;
	});
	Add(TEXT("prints"), 2.0, [W, Say, Close, Stop]
	{
		W->S.Dev = 2;
		W->Collect();
		Say(FString::Printf(TEXT("collected: dev %d, docs frame03 %d"), W->S.Dev, W->Host->Docs().ContainsByPredicate([](const FS47Doc& D) { return D.Id == TEXT("frame03"); })));
		// (game time stands while the prints are open: closing them files them, and the chapter ends)
		if (Stop != TEXT("prints")) Close();
		return true;
	});
	Add(TEXT("end"), 1.0, [W, Say]
	{
		// (chapter four's card, or until it is ported the end card, comes 2.4 s after the prints are filed)
		Say(FString::Printf(TEXT("stage %s"), *W->S.Stage));
		return true;
	});

	// one step after the other on game time; a step that is not ready is asked again a second later
	TSharedRef<TFunction<void(int32)>> Run = MakeShared<TFunction<void(int32)>>();
	*Run = [W, Steps, Stop, Say, Run](int32 I)
	{
		if (!W.IsValid() || !Steps->IsValidIndex(I)) return;
		const FStep& Sp = (*Steps)[I];
		W->Host->After(Sp.Wait, [W, Steps, Stop, Say, Run, I]
		{
			if (!W.IsValid()) return;
			const FStep& Sp2 = (*Steps)[I];
			if (!Sp2.Do())
			{
				// not yet: the same step again, once a second
				(*Steps)[I].Wait = 1.0;
				(*Run)(I);
				return;
			}
			if (Sp2.Name == Stop) { Say(TEXT("held at ") + Stop); return; }
			(*Run)(I + 1);
		});
	};
	(*Run)(0);
}

// ---------- self test ----------

bool US47Chapter3::SelfTest(TArray<FString>& Lines)
{
	bool bOk = US47StationTaken::SelfTest(Lines);
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s chapter three: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };

	// the board as tools/chapter3.py plays it (no world: no host, no screen, no sound)
	US47Chapter3* C = NewObject<US47Chapter3>(GetTransientPackage());
	C->BuildBook();
	C->bStarted = true; C->bActive = true;
	FS47RuleBook& B = *C->Book;
	auto Thread = [&B](const TCHAR* A, const TCHAR* Bb) { const TOptional<FS47Result> R = B.Connect(A, Bb, false); return R ? R->Text : FString(TEXT("(not this chapter's)")); };
	Check(C->Objective() == TEXT("SERVICE YARD // TAKE THE SARO TRUCK TO STATION 01") && C->Tasks().Num() == 1, TEXT("a fresh chapter: to the truck"));
	Check(C->TruckTrip(TEXT("saro")) && C->TruckTrip(TEXT("saro"))->Label == TEXT("Drive to STATION 01"), TEXT("the truck offers the drive out"));
	Check(C->Check().Text == TEXT("THE FIELD ROLL IS FOR STATION 01") && !C->Check().bOk, TEXT("the field roll away from the station"));
	C->S.Stage = TEXT("station"); C->S.bArrived = true;
	Check(!C->TruckTrip(TEXT("station01")) && C->Objective() == TEXT("STATION 01 // DOCUMENT THE MOVED MARKER OR RUN A LOCAL LAMP NULL TEST") && C->Tasks().Num() == 10, TEXT("at the station: the objective and ten tasks"));
	// P06
	FString R = Thread(TEXT("e09a.b"), TEXT("frame03"));
	Check(R.Contains(TEXT("B is still in its footing")) && C->S.Wrong06 == 1, TEXT("P06: B against FRAME 03 is answered and counts"));
	R = Thread(TEXT("e09a.a"), TEXT("frame03"));
	Check(R.Contains(TEXT("RECORD it")) && !C->S.bP06, TEXT("P06: A on E09A tied to FRAME 03"));
	Check(B.Record(TEXT("P06"))->Text.StartsWith(TEXT("READ E09A")), TEXT("P06 waits for E09A"));
	C->S.bReadE09a = true;
	Check(B.Record(TEXT("P06"))->Text.StartsWith(TEXT("FRAME 03 FIRST")), TEXT("P06 waits for FRAME 03"));
	FS47Photo P3; P3.Id = TEXT("frame03"); P3.Subject = TEXT("STATION 01 / fixed point A"); P3.Method = TEXT("field"); P3.Clock = 4 * 3600 + 5 * 60; P3.Url = TEXT("data:image/jpeg;base64,AAAA");
	P3.Targets = { { TEXT("footing"), FVector2D(0.4, 0.6) }, { TEXT("markerA"), FVector2D(0.6, 0.55) }, { TEXT("band"), FVector2D(0.5, 0.5) } };
	C->S.F3 = P3;
	Check(C->Objective() == TEXT("STATION 01 // DOCUMENT THE MOVED MARKER OR RUN A LOCAL LAMP NULL TEST"), TEXT("objective with FRAME 03 exposed, before P06 and P07"));
	TOptional<FS47Result> Rec = B.Record(TEXT("P06"));
	Check(Rec && Rec->bOk && C->S.bP06 && Rec->Text.StartsWith(TEXT("P06 SUPPORTED")), TEXT("P06 recorded: A moved"));
	Check(C->Objective() == TEXT("STATION 01 // COVER THE LOCAL LAMP AND OBSERVE A NORMAL NULL TEST"), TEXT("objective: the null test"));
	// P08 before P07, and its wrong threads
	R = Thread(TEXT("frame04"), TEXT("e07.sig"));
	Check(R.Contains(TEXT("do not show who cut it")) && C->S.Wrong08 == 1, TEXT("P08: a motive from the signature is answered"));
	R = Thread(TEXT("frame04"), TEXT("e07.head"));
	Check(R.Contains(TEXT("opposed and clean")) && C->S.Wrong08 == 2, TEXT("P08: age and weather are answered with the Unity words"));
	R = Thread(TEXT("frame04"), TEXT("P07"));
	Check(R.Contains(TEXT("RECORD it")), TEXT("P08: FRAME 04 tied to the null test"));
	Check(B.Record(TEXT("P08"))->Text.StartsWith(TEXT("P08 REQUIRES BOTH")), TEXT("P08 waits for P07"));
	C->S.bP07 = true; C->S.bLampObserved = true;
	Check(B.Record(TEXT("P08"))->Text.StartsWith(TEXT("FRAME 04 FIRST")), TEXT("P08 waits for FRAME 04"));
	FS47Photo P4 = P3; P4.Id = TEXT("frame04"); P4.Targets = { { TEXT("cable"), FVector2D(0.5, 0.5) } };
	C->S.bCableInspected = true; C->S.F4 = P4;
	Check(C->FrameNo() == TEXT("FRAME 05"), TEXT("both field frames exposed: FRAME 05 next"));
	Rec = B.Record(TEXT("P08"));
	Check(Rec && Rec->bOk && C->S.bP08, TEXT("P08 recorded: a deliberate cut"));
	Check(C->SubmitCable(TEXT("weathering")).Text.Contains(TEXT("lamp fault or ordinary drift")) && C->S.Wrong08 == 3, TEXT("the field record's P08 replies"));
	// P09
	R = Thread(TEXT("hut.rx"), TEXT("e10.relay"));
	Check(R.Contains(TEXT("47-second relay impact")) && C->S.Wrong09 == 1, TEXT("P09: the relay line is answered with the Unity reply"));
	R = Thread(TEXT("hut.rx"), TEXT("e10.carrier"));
	Check(R.Contains(TEXT("RECORD it")), TEXT("P09: the receiver tied to the same words in E10"));
	Check(B.Record(TEXT("P09"))->Text.StartsWith(TEXT("READ E10")), TEXT("P09 waits for E10"));
	C->S.bReadE10 = true;
	Check(B.Record(TEXT("P09"))->Text.StartsWith(TEXT("LISTEN TO THE HUT RECEIVER")), TEXT("P09 waits for the receiver"));
	C->S.bHeard = true;
	Rec = B.Record(TEXT("P09"));
	Check(Rec && Rec->bOk && C->S.bP09 && C->S.Stage == TEXT("call") && C->CallT == 4.0, TEXT("P09 recorded: 02:17:47, the field telephone to ring in four seconds"));
	Check(C->Objective() == TEXT("STATION 01 // FIELD RECORD COMPLETE") && C->Tasks().Num() == 12, TEXT("objective and tasks after P09"));
	// threads that hold without answering anything, and others' threads
	Check(Thread(TEXT("e09a.c"), TEXT("e07.abc")).StartsWith(TEXT("The same C")) && Thread(TEXT("s1diagram"), TEXT("e06.omit")).Contains(TEXT("framed copy")), TEXT("the thin threads to chapter two's records"));
	Check(Thread(TEXT("e07.abc"), TEXT("e06.omit")) == TEXT("(not this chapter's)"), TEXT("chapter two's own threads are not this book's"));
	Check(C->S.Links && C->S.Links->Num() == 5 && C->S.Links->Contains(TEXT("e10.carrier|hut.rx")), TEXT("five threads kept, sorted pairs"));
	// the cards
	TArray<FS47BoardCard> Cards = C->BoardCards();
	Check(Cards.Num() == 5 && Cards[1].Kind == TEXT("sealed") && Cards[1].Title == TEXT("ON THE FIELD ROLL") && Cards[1].Text.EndsWith(TEXT("Sealed; it is developed at SARO.")) && Cards[3].Full.Contains(TEXT("NEW RECEIVER OBSERVATION / 1986")) && Cards[4].Id == TEXT("hut.rx"),
		TEXT("five cards; the frames sealed on the roll; E10 with the 1986 line"));
	C->S.Dev = 3;
	Cards = C->BoardCards();
	Check(Cards[1].Kind == TEXT("photo") && Cards[1].Title == TEXT("FIXED POINT A") && Cards[1].Image == P3.Url, TEXT("developed, the frames are photographs"));
	// the bench, the band on FRAME 03, the save block, the way back
	Check(C->BandInLook(0.5, 0.5) && !C->BandInLook(0.0, 0.0), TEXT("the band within FRAME 03 looked at closer"));
	C->Busy = TEXT("tank");
	Check(C->SaveBlock() == TEXT("Not while the film is in the tank."), TEXT("no saving while the film is in the tank"));
	C->Busy.Reset();
	C->S.Stage = TEXT("return");
	Check(C->TruckTrip(TEXT("station01")) && C->TruckTrip(TEXT("station01"))->Label == TEXT("Drive back to SARO") && C->Objective() == TEXT("STATION 01 // DRIVE BACK TO SARO WITH THE FIELD ROLL"), TEXT("the truck offers the way back"));
	C->ArrivedSaro();
	Check(C->S.Stage == TEXT("develop") && C->Objective() == TEXT("SARO PHOTOLAB // COLLECT THE FIELD PRINTS"), TEXT("back at SARO with the field roll"));
	// the saved state, with the TypeScript's field names
	const FString Json = S47J::Write(C->S.Save());
	Check(Json.StartsWith(TEXT("{\"v\":1,\"stage\":\"develop\",\"arrived\":true,\"readE09a\":true,\"readE10\":true,\"heard\":true,\"seen\":{\"footing\":false,\"markerA\":false,\"markerB\":false,\"stakes\":false,\"diagram\":false,\"hut\":false},\"lampCovered\":false,\"lampObserved\":true,\"cableInspected\":true,\"f3\":{"))
		&& Json.Contains(TEXT("\"p06\":true,\"p07\":true,\"p08\":true,\"p09\":true,\"wrong06\":1,\"wrong08\":3,\"wrong09\":1,\"rejected\":0,\"called\":false,\"dev\":3,\"links\":[")) && !Json.Contains(TEXT("\"fb\"")),
		TEXT("Ch3State in the web version's JSON"));
	const FS47Ch3State L = FS47Ch3State::Load(S47J::Read(TEXT("{\"v\":1,\"stage\":\"station\",\"arrived\":true,\"seen\":{\"footing\":true,\"extra\":1},\"p06\":true,\"wrong06\":2,\"fb\":{\"at\":15000,\"times\":{},\"had\":[],\"told\":[],\"links\":[]},\"later\":7}")));
	const S47J::FObj Ls = L.Save();
	Check(L.Stage == TEXT("station") && L.Seen.bFooting && !L.Seen.bHut && L.bP06 && L.Wrong06 == 2 && !L.F3 && !L.Links && L.Fb.IsValid() && L.Dev == 0
		&& S47J::Num(Ls, TEXT("later")) == 7 && S47J::Num(S47J::Obj(Ls, TEXT("fb")), TEXT("at")) == 15000 && S47J::Num(S47J::Obj(Ls, TEXT("seen")), TEXT("extra")) == 1 && S47J::Has(S47J::Obj(Ls, TEXT("seen")), TEXT("diagram")),
		TEXT("an older save merges with a fresh state; the field book and unknown fields stay"));
	// a save without threads gets its recorded findings' threads back
	C->S.Links.Reset();
	B.Backfill();
	Check(C->S.Links && C->S.Links->Num() == 3, TEXT("backfill restores the three findings' threads"));
	C->MarkAsGarbage();
	return bOk;
}

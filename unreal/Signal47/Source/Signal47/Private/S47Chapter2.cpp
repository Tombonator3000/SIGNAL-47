#include "S47Chapter2.h"

#include "Internationalization/Regex.h"
#include "S47Annex.h"
#include "S47Audio.h"
#include "S47Chapter3.h"
#include "S47Drawings2.h"
#include "S47HUD.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Panels2.h"
#include "S47PhoneScripts.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Ch2, Log, All);

namespace
{
	// ---------- the sources (Unity WorldCase22 text) ----------
	const TCHAR* ORIGINAL = TEXT("FIELD SURVEY RECORD / STATION 01 / 1947\n\nThe reference remained visible in the plate after the lamp circuit was opened. A second plate was exposed with the local lamp isolated. The retained mark did not follow the new position of the physical vane.\n\nThe arrangement used three references: A, the fixed survey point; B, the optical comparison vane; and C, the closing sight line. The run was interrupted before the final observation was entered. Preserve the original plates and the complete arrangement drawing.\n\nN. VEGA / FIELD TECHNICIAN");
	const TCHAR* AMENDED = TEXT("FIELD SURVEY RECORD / STATION 01 / 1947\nAMENDED COPY\n\nThe additional mark is attributed to a fault in plate development. The lamp and comparison vane are sufficient to describe the test. The incomplete closing sight line has been omitted from the service copy.\n\nNo repeat observation is required. Retain the amended copy with the routine reference records.\n\nN. VEGA / FIELD TECHNICIAN");
	const TCHAR* LINEAGE = TEXT("SARO / REFERENCE LINEAGE / B-12\n\nThe B-12 comparison vane retains the fixed-point survey identified as STATION 01. Fixed-point register mark: outlined triangle with a short horizontal bar beneath it. The archive sleeve contains the dated plate and fixed-point sheet used for this index. Match the survey identifier and fixed-point mark before selecting the field destination.\n\nSITE REGISTER: STATION 01 / OLD SURVEY STATION. The matching archive sleeve contains the field access sheet.");
	const TCHAR* INDEX = TEXT("PLATE / 1947 / STATION 01\n\nFixed-point mark: outlined triangle with a short horizontal bar beneath it.\nArrangement: A - B - C.\nService index: B-12.\nField destination: OLD SURVEY STATION.\n\nRETURN CHECK: Match both STATION 01 and the fixed-point mark. A similar triangle without the bar is a general elevation symbol and is not a matching survey reference.");
	const TCHAR* ACCESS = TEXT("FIELD ACCESS / OLD SURVEY STATION\nSURVEY: STATION 01\n\nACCESS ROAD UNSURFACED. GATE LOCKED.\n\nKEY HOLDER:  N. VEGA\n             SIERRA MOTOR COURT\n             (505) 555-0119\n\nDO NOT DISTURB THE FIXED-POINT MARKS.");
	const TCHAR* PHONE = TEXT("5055550119");
	// How long the line rings for her before it stops, and how long after that it rings again (with
	// the player in the control room, or in the corridor just outside it).
	constexpr double BACK_RING = 24, BACK_AGAIN = 40;

	const TCHAR* SUPPORTED_04 = TEXT("SUPPORTED / E07 records A, B and C and a retained mark with the lamp isolated. E06 omits C, the closing sight line, and calls the mark a development fault. The report was changed; motive remains unknown.");
	const TCHAR* SUPPORTED_05 = TEXT("SUPPORTED / B-12 retains STATION 01. The archive sleeve matches its ID and triangle-with-bar fixed-point mark. OLD SURVEY STATION is the supported destination. Field access prepared. The matching sleeve also holds the field access sheet.");

	// Design bible section 08 (three levels) and the facilitator hints from the P04/P05 paper test,
	// plus a fourth that names the threads, held back until the player has seen the third and still
	// missed twice (SPILLDESIGN.md: hints never spell the pair too early).
	const TArray<FString> HINTS_04 = { TEXT("What is different between the two versions?"), TEXT("Which field is in the original and missing from the service copy? Compare the arrangement, not only the conclusion."), TEXT("Put the original plan beside the amended copy and find the connection that is missing."),
		TEXT("Thread the three references in E07 to the omitted sight line in E06, and the retained mark in E07 to the development fault in E06.") };
	const TArray<FString> HINTS_05 = { TEXT("Where does today's B-12 reference come from?"), TEXT("Find both the site ID and the reference mark in the lineage card and in the sleeve."), TEXT("Match STATION 01 and the triangle with a bar, then choose the place the sleeve names."),
		TEXT("Thread STATION 01 on the B-12 card to STATION 01 on the sleeve, the two triangles with the bar to each other, and the two place names.") };

	// ---------- the evidence board ----------
	// The lines that matter on each record. A thread between two lines that say something about
	// each other holds; what they show together goes into its question by itself (P04: the omitted
	// C and the explained-away mark; P05: the survey, the mark and the place). Some threads hold
	// without answering anything: a note on the board, or a thin thread. Threads that contradict
	// get the Unity replies and count as wrong, as before. The rules are worked by FS47RuleBook
	// (story/boardRules.ts); the board is the night's case board (story/caseBoard.ts).
	const TMap<FString, FS47BookNote>& NOTES()
	{
		static const TMap<FString, FS47BookNote> N = {
			{ TEXT("n.c"), { TEXT("C, the closing sight line, is in the original. The service copy leaves it out."), TEXT("P04"), false } },
			{ TEXT("n.mark"), { TEXT("The original kept the mark with the lamp isolated. The copy calls it a development fault."), TEXT("P04"), false } },
			{ TEXT("n.sig"), { TEXT("The same name signs both versions. It says who changed the record, not why."), FString(), false } },
			{ TEXT("n.id"), { TEXT("Same survey: STATION 01 on the B-12 card and on the 1947 sleeve."), TEXT("P05"), false } },
			{ TEXT("n.mark2"), { TEXT("Same fixed-point mark: the outlined triangle with the bar."), TEXT("P05"), false } },
			{ TEXT("n.dest"), { TEXT("Both name the same place: OLD SURVEY STATION."), TEXT("P05"), false } },
		};
		return N;
	}
	const TCHAR* ELEVATION = TEXT("A triangle without the bar is only an elevation symbol. The matching fixed-point mark has a short bar beneath the outlined triangle.");
	// -39 against a year from the archive holds, as a thin thread with nothing added: the player can
	// do the sum or not (HISTORIE.md: -39 LY is 39 years, and nobody says so).
	const TCHAR* LY = TEXT("-39 on tonight's printout. 1947 on the record.");
	const TCHAR* DEVELOPMENT = TEXT("Compare the arrangement, not just the amended explanation. Which reference appears in the original and is omitted from the service copy?");
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
			Rule(TEXT("e07.abc"), TEXT("e06.omit"), TEXT("n.c"), N[TEXT("n.c")].Text),
			Rule(TEXT("e07.mark"), TEXT("e06.fault"), TEXT("n.mark"), N[TEXT("n.mark")].Text),
			Rule(TEXT("e07.sig"), TEXT("e06.sig"), TEXT("n.sig"), N[TEXT("n.sig")].Text),
			Rule(TEXT("b12.id"), TEXT("e08.id"), TEXT("n.id"), N[TEXT("n.id")].Text),
			Rule(TEXT("b12.mark"), TEXT("e08.mark"), TEXT("n.mark2"), N[TEXT("n.mark2")].Text),
			Rule(TEXT("b12.site"), TEXT("e08.dest"), TEXT("n.dest"), N[TEXT("n.dest")].Text),
			Rule(TEXT("e07.head"), TEXT("e08.id"), FString(), TEXT("The sleeve belongs with the 1947 record: same survey, same year.")),
			Rule(TEXT("e07.head"), TEXT("b12.id"), FString(), TEXT("Today's B-12 vane keeps the survey from the 1947 record.")),
			Rule(TEXT("e06.fault"), TEXT("e07.abc"), FString(), DEVELOPMENT, TEXT("P04")),
			Rule(TEXT("e06.fault"), TEXT("e07.head"), FString(), DEVELOPMENT, TEXT("P04")),
			Rule(TEXT("e06.fault"), TEXT("e07.sig"), FString(), TEXT("The shared signature identifies the report. It does not establish the author's motive or responsibility. Which reference changes between the two versions?"), TEXT("P04")),
			Rule(TEXT("b12.mark"), TEXT("e08.elev"), FString(), ELEVATION, TEXT("P05")),
			Rule(TEXT("rx.dist"), TEXT("e08.id"), FString(), LY),
			Rule(TEXT("rx.dist"), TEXT("e07.head"), FString(), LY),
		};
	}
	// /^(b12|e08|rx)\./
	bool IsP05Pin(const FString& X) { return X.StartsWith(TEXT("b12.")) || X.StartsWith(TEXT("e08.")) || X.StartsWith(TEXT("rx.")); }
	FString QuestionOf(const FString& A, const FString& B) { return IsP05Pin(A) || IsP05Pin(B) ? TEXT("P05") : TEXT("P04"); }

	// WHERE: Record<P.SourceKey, string>
	const TCHAR* WHERE_ORIGINAL = TEXT("The 1947 field records are in a box on the west shelves, marked STATION 01.");
	const TCHAR* WHERE_AMENDED = TEXT("The service copies are in a binder on the bookcase by the south wall.");
	const TCHAR* WHERE_LINEAGE = TEXT("The maintenance lineage cards are in the card index on the east wall.");
	const TCHAR* WHERE_INDEX = TEXT("The archive sleeve is in the same box as the 1947 field record.");

	// Where Ward's call-back may begin besides the control room: its south door and the corridor
	// outside it, so that it reaches a player on the way to the records room where she said (it is
	// answered at the desk). Not the records room itself, where the steps on the roof want their
	// quiet (story/roofSteps.ts).
	const TCHAR* EARSHOT[] = { TEXT("southDoor"), TEXT("corridor") };

	// `${cue.by}: "${cue.text}"`
	FString Caption(const FString& By, const FString& Text) { return FString::Printf(TEXT("%s: \"%s\""), *By, *Text); }
}

// ---------- the saved state ----------

S47J::FObj FS47Ch2State::Save() const
{
	S47J::FObj O = Extra.IsValid() ? S47J::Clone(Extra) : S47J::New();
	S47J::Set(O, TEXT("v"), 1);
	S47J::Set(O, TEXT("stage"), Stage);
	S47J::Set(O, TEXT("answered"), bAnswered);
	S47J::Set(O, TEXT("door"), bDoor);
	S47J::Set(O, TEXT("entered"), bEntered);
	S47J::FObj R = S47J::New();
	S47J::Set(R, TEXT("original"), Read.bOriginal);
	S47J::Set(R, TEXT("amended"), Read.bAmended);
	S47J::Set(R, TEXT("lineage"), Read.bLineage);
	S47J::Set(R, TEXT("index"), Read.bIndex);
	S47J::Set(O, TEXT("read"), R);
	S47J::Set(O, TEXT("p04"), bP04);
	S47J::Set(O, TEXT("p05"), bP05);
	S47J::Set(O, TEXT("finding"), Finding);
	S47J::Set(O, TEXT("destination"), Destination);
	S47J::Set(O, TEXT("surveyId"), SurveyId);
	S47J::Set(O, TEXT("marker"), Marker);
	S47J::Set(O, TEXT("wrong04"), Wrong04);
	S47J::Set(O, TEXT("wrong05"), Wrong05);
	S47J::Set(O, TEXT("called"), bCalled);
	S47J::Set(O, TEXT("links"), Links);
	if (ArrayAsk) S47J::Set(O, TEXT("arrayAsk"), *ArrayAsk);
	return O;
}

FS47Ch2State FS47Ch2State::Load(const S47J::FObj& Saved)
{
	FS47Ch2State C;
	if (!Saved.IsValid()) return C;
	C.Extra = S47J::Clone(Saved);
	C.Stage = S47J::Str(Saved, TEXT("stage"), C.Stage);
	C.bAnswered = S47J::Bool(Saved, TEXT("answered"));
	C.bDoor = S47J::Bool(Saved, TEXT("door"));
	C.bEntered = S47J::Bool(Saved, TEXT("entered"));
	const S47J::FObj R = S47J::Obj(Saved, TEXT("read"));
	C.Read.bOriginal = S47J::Bool(R, TEXT("original"));
	C.Read.bAmended = S47J::Bool(R, TEXT("amended"));
	C.Read.bLineage = S47J::Bool(R, TEXT("lineage"));
	C.Read.bIndex = S47J::Bool(R, TEXT("index"));
	C.bP04 = S47J::Bool(Saved, TEXT("p04"));
	C.bP05 = S47J::Bool(Saved, TEXT("p05"));
	C.Finding = S47J::Str(Saved, TEXT("finding"));
	C.Destination = S47J::Str(Saved, TEXT("destination"));
	C.SurveyId = S47J::Str(Saved, TEXT("surveyId"));
	C.Marker = S47J::Str(Saved, TEXT("marker"));
	C.Wrong04 = S47J::Int(Saved, TEXT("wrong04"));
	C.Wrong05 = S47J::Int(Saved, TEXT("wrong05"));
	C.bCalled = S47J::Bool(Saved, TEXT("called"));
	C.Links = S47J::Strs(Saved, TEXT("links"));
	if (S47J::Has(Saved, TEXT("arrayAsk"))) C.ArrayAsk = S47J::Bool(Saved, TEXT("arrayAsk"));
	return C;
}

// ---------- registration (the constructor) ----------

US47Chapter2* US47Chapter2::Register(US47Chapter2* C)
{
	if (!C) return C;
	// the archive wing (main.ts builds it with the world; the deps hand it to the chapter)
	C->Annex = NewObject<US47Annex>(C);
	C->Annex->Init(C->M);
	C->Interactables();
	C->BuildBook();
	if (C->M)
	{
		// (one chapter two at a time: a book left by an earlier start of the game goes)
		C->M->Books.RemoveAll([](const FS47RuleBook* B) { return B && B->Key() == TEXT("ch2"); });
		C->M->Books.Add(C->Book.Get());
	}
	TWeakObjectPtr<US47Chapter2> W = C;
	if (C->Host) C->Host->Jumps.Add(TEXT("chapter2"), [W] { if (W.IsValid()) W->JumpChapter2(); });
	// main.ts ch2.onEnd: the key holder sends you to the station first. (If the steps on the records
	// room roof are still going, the web version waits for them, story/roofSteps.ts; they are not in
	// this version.)
	C->OnEnd = [W]
	{
		if (!W.IsValid() || !W->Host) return;
		AS47Story* H = W->Host;
		H->ChapterBreak({ TEXT("THE AMENDED RECORD // P04 AND P05 RECORDED"),
			TEXT("The 1947 record was changed: the service copy leaves out C. B-12 still carries STATION 01, and the key holder wants you at the cut cable before you come to her."),
			TEXT("CHAPTER THREE"), TEXT("THE SURVEY STATION"), [H] { if (US47Chapter3* C3 = Cast<US47Chapter3>(H->Chapter(TEXT("ch3")))) C3->BeginChapter3(nullptr); } });
	};
	return C;
}

void US47Chapter2::BuildBook()
{
	FS47BookSpec O;
	O.Key = TEXT("ch2");
	O.Owns = [](const FString& N) { return N.StartsWith(TEXT("e07.")) || N.StartsWith(TEXT("e06.")) || N.StartsWith(TEXT("b12.")) || N.StartsWith(TEXT("e08.")) || N.StartsWith(TEXT("rx.")) || N == TEXT("P04") || N == TEXT("P05"); };
	O.Notes = NOTES();
	O.Rules = RULES();
	FS47BookQuestion P04;
	P04.Id = TEXT("P04"); P04.Stamp = TEXT("P04"); P04.Text = TEXT("What changed in the 1947 record after the run?"); P04.Slots = 2; P04.Hints = HINTS_04;
	P04.X = 14; P04.Y = 452; P04.W = 272;
	P04.Open = [this] { return bStarted; };
	P04.Done = [this] { return S.bP04 ? FString(SUPPORTED_04) : FString(); };
	P04.Record = [this] { return Compare(TEXT("omitted-c")); };
	FS47BookQuestion P05;
	P05.Id = TEXT("P05"); P05.Stamp = TEXT("P05"); P05.Text = TEXT("Where does the B-12 reference lead in the field?"); P05.Slots = 3; P05.Hints = HINTS_05;
	P05.X = 898; P05.Y = 14; P05.W = 266;
	P05.Open = [this] { return bStarted; };
	P05.Done = [this] { return S.bP05 ? FString(SUPPORTED_05) : FString(); };
	P05.Record = [this] { return Route(TEXT("old-survey-station"), TEXT("STATION 01"), TEXT("triangle-bar")); };
	O.Questions = { P04, P05 };
	O.Links = [this]() -> TArray<FString>& { return S.Links; };
	O.Wrong = [this](const FString& Q, const FS47BookRule&) -> TOptional<FS47Result>
	{
		if (Q == TEXT("P04")) S.Wrong04++; else S.Wrong05++;
		return {};
	};
	O.Save = [this] { Save(); };
	O.Cards = [this] { return BoardCards(); };
	O.QuestionOf = [](const FString& A, const FString& B) { return QuestionOf(A, B); };
	O.Empty = TEXT("Those two lines do not say anything about each other.");
	// the first board kept threads to the question cards and its notes: /(^|\|)(P0|n\.)/
	O.Legacy = [](const FString& K)
	{
		TArray<FString> Parts;
		K.ParseIntoArray(Parts, TEXT("|"), false);
		for (const FString& P : Parts) if (P.StartsWith(TEXT("P0")) || P.StartsWith(TEXT("n."))) return true;
		return false;
	};
	Book = MakeUnique<FS47RuleBook>(MoveTemp(O));
}

void US47Chapter2::BeginDestroy()
{
	if (M && Book) M->Books.Remove(Book.Get());
	Super::BeginDestroy();
}

// ---------- lifecycle ----------

void US47Chapter2::Begin(const S47J::FObj& Saved)
{
	const bool bHold = bHoldRing;   // (the card's: main.ts, chapterBreak)
	Reset();
	bHoldRing = bHold;
	bActive = true; bStarted = true;
	S = Saved.IsValid() ? FS47Ch2State::Load(Saved) : FS47Ch2State();
	ApplyWorld();
	if (S.Stage == TEXT("ward-call")) Ring();
	// a case kept between her two calls: she calls back once the player is in the control room
	else if (S.bAnswered && !S.ArrayAsk.Get(false)) BackT = 6;
	Save();
}

void US47Chapter2::Reset()
{
	bActive = false; bStarted = false; bRinging = false; bHoldRing = false;
	bRingBack = false; BackT = -1; BackRing = 0;
	S = FS47Ch2State();
	if (M && M->Audio) M->Audio->Stop(TEXT("ring2"));
}

S47J::FObj US47Chapter2::SaveState() const
{
	return bStarted ? S.Save() : nullptr;
}

void US47Chapter2::ApplyWorld()
{
	// a case from before the doors were free: doors.set('south', true, true). (AS47Web opens a door
	// by toggling it, which swings it and sounds as the player opening it would.)
	if (S.bDoor && M && M->Web) M->Web->SetDoorOpen(TEXT("doorSouth"), true, true);
	if (S.Read.bOriginal) File(DocOriginal());
	if (S.Read.bIndex) File(DocIndex());
	if (S.Read.bAmended) File(DocAmended());
	if (S.Read.bLineage) File(DocLineage());
	if (S.bP04) File(DocFinding04());
	if (S.bP05) { File(DocFinding05()); File(DocAccess()); File(DocMap()); }
}

void US47Chapter2::File(const FS47Doc& Doc)
{
	// file(doc): replaces the one with the same id, or adds it
	if (Host) Host->AddDoc(Doc);
}

void US47Chapter2::Save() { if (OnSave) OnSave(); }

FString US47Chapter2::Time() const { return S47Text::ClockText(Host ? Host->Clock() : 0, false); }

void US47Chapter2::Toast(const FString& Text, float Secs) const { if (M && M->UI) M->UI->Toast(Text, Secs); }

void US47Chapter2::Note(const FString& Text) const { if (Host) Host->Note(Text); }

void US47Chapter2::Click(const TOptional<FVector>& At) const
{
	// audio.play('click', { gain: 0.5, at })
	if (!M || !M->Audio) return;
	FS47Sound O;
	O.Gain = 0.5f;
	O.At = At;
	M->Audio->Sfx(TEXT("click"), O);
}

FVector US47Chapter2::PhonePos() const
{
	// this.pos(room.objs.phone)
	return M && M->Web ? M->Web->Obj(TEXT("room.objs.phone")).WorldPosition() : FVector::ZeroVector;
}

// ---------- objective and notebook ----------

FString US47Chapter2::Objective() const
{
	if (S.Stage == TEXT("ward-call")) return TEXT("CONTROL ROOM // THE SUPERVISOR LINE IS RINGING");
	if (S.Stage == TEXT("records"))
	{
		if (!S.bEntered) return TEXT("SARO RECORDS // SOUTH CORRIDOR: FIND WHERE THE B-12 REFERENCE CAME FROM");
		if (!S.bP04) return TEXT("SARO RECORDS // COMPARE THE ORIGINAL AND AMENDED FIELD RECORDS ON THE CASE BOARD");
		return TEXT("SARO RECORDS // ON THE CASE BOARD: MATCH THE B-12 LINEAGE TO THE ARCHIVE INDEX");
	}
	if (S.Stage == TEXT("call-nora")) return TEXT("STATION 01 // CALL THE KEY HOLDER FROM THE RECORDS ROOM PHONE");
	if (S.Stage == TEXT("complete")) return TEXT("STATION 01 // FIELD ACCESS PREPARED");
	return FString();
}

TArray<FS47Task> US47Chapter2::Tasks() const
{
	TArray<FS47Task> T = {
		{ TEXT("Answer the supervisor's call"), S.bAnswered },
		{ TEXT("Find the reference record (south corridor)"), S.bEntered },
	};
	if (S.bEntered) T.Append({
		{ TEXT("Read the 1947 field record (E07)"), S.Read.bOriginal },
		{ TEXT("Read the amended service copy (E06)"), S.Read.bAmended },
		{ TEXT("Read the B-12 reference lineage card"), S.Read.bLineage },
		{ TEXT("Read the archive sleeve index (E08)"), S.Read.bIndex },
		{ TEXT("Record what changed in 1947, on the case board (P04)"), S.bP04 },
		{ TEXT("Record a supported field destination, on the case board (P05)"), S.bP05 },
	});
	if (S.bP05) T.Add({ TEXT("Call the key holder for STATION 01"), S.bCalled });
	if (S.ArrayAsk.Get(false)) T.Add({ TEXT("Put the array back on the schedule (operations terminal)"), ArrayTried ? ArrayTried() : false });
	return T;
}

// ---------- Ward ----------

void US47Chapter2::Ring()
{
	bRinging = true;
	if (bHoldRing) return;
	RingOn();
	if (Host) Host->After(1.0, [this] { RingSaid(); });
}

bool US47Chapter2::RingOn()
{
	// audio.loop('ring2', 'phoneRing', { gain: 1.0, at: this.pos(room.objs.phone) })
	if (!M || !M->Audio) return false;
	FS47Sound O;
	O.Gain = 1.0f;
	O.At = PhonePos();
	M->Audio->LoopSfx(TEXT("ring2"), TEXT("phoneRing"), O);
	return true;
}

void US47Chapter2::RingSaid()
{
	if (bRinging && !bRingBack) Toast(TEXT("The phone again. The supervisor's line."), 3);
}

bool US47Chapter2::RingOut()
{
	// the held ring, now: the supervisor's line in the room, ringing (the card's title is up); false
	// when there is nothing to ring (the chapter is over, or no sound)
	if (!bHoldRing) return false;
	bHoldRing = false;
	if (!bActive || !bRinging || bRingBack) return false;
	return RingOn();
}

bool US47Chapter2::RingAgain()
{
	return bActive && bRinging && !bRingBack && !bHoldRing ? RingOn() : false;
}

void US47Chapter2::RingShown()
{
	if (Host) Host->After(1.0, [this] { RingSaid(); });
}

FString US47Chapter2::PhoneLabel() const
{
	if (bRinging) return TEXT("Answer the phone");
	if (PhoneHook.Label) { const FString L = PhoneHook.Label(); if (!L.IsEmpty()) return L; }
	return S.bCalled || S.bAnswered ? TEXT("Telephone") : FString();
}

void US47Chapter2::UsePhone()
{
	if (!M || !Host) return;
	US47Audio* Audio = M->Audio;
	if (!bRinging && PhoneHook.Use && PhoneHook.Use(TEXT("desk"))) return;
	if (!bRinging) { Toast(S.bAnswered ? TEXT("Ward said what she had to say. The records room is down the south corridor.") : TEXT("Internal line and one outside line.")); return; }
	if (bRingBack) { AnswerBack(); return; }
	bRinging = false;
	if (Audio) Audio->Stop(TEXT("ring2"), 0.02f);
	if (M->Web) M->Web->SetHandsetVisible(false);
	Host->SetCinematic(true);
	const TOptional<FS47CallTimeline> Spoken = Audio ? Audio->PhoneCall(S47PhoneScripts::Ward()) : TOptional<FS47CallTimeline>();
	if (Spoken)
	{
		for (const FS47CallCue& Cue : Spoken->Cues) Host->After(Cue.At, [this, Cue] { Toast(Caption(Cue.By, Cue.Text), FMath::Max(2.f, Cue.Duration)); });
	}
	else
	{
		if (Audio) Audio->CallLine(10.2f);
		auto Cap = [this](double At, const FString& Text, float Secs) { Host->After(At, [this, Text, Secs] { Toast(Text, Secs); }); };
		Cap(0.3, TEXT("WARD: \"That is enough to hold the morning series. It is not enough to tell us why it happened.\""), 4.6f);
		Cap(5.0, TEXT("WARD: \"Check the reference record. If the instrument inherited the fault, the record may tell us from where.\""), 5.0f);
	}
	Host->After(Spoken ? Spoken->Duration + 0.1 : 10.3, [this]
	{
		if (M->Web) M->Web->SetHandsetVisible(true);
		Host->SetCinematic(false);
		S.bAnswered = true;
		S.Stage = TEXT("records");
		Note(FString::Printf(TEXT("%s. Dr. Ward read the B-12 report and is holding the morning series. Check the reference record: where did the B-12 reference come from?"), *Time()));
		Toast(TEXT("Ward hangs up. The records room is down the south corridor."), 3.6f);
		if (!S.ArrayAsk.Get(false)) BackT = 3;
		Save();
	});
}

// ---------- Ward again: the array (story/arrayBack.ts) ----------

void US47Chapter2::CallBack(bool bHere)
{
	bRinging = true; bRingBack = true; BackRing = 0; BackT = -1;
	RingOn();
	Toast(bHere ? TEXT("The supervisor's line again.") : TEXT("In the control room, the supervisor's line is ringing again."), 3.4f);
}

void US47Chapter2::AnswerBack()
{
	US47Audio* Audio = M->Audio;
	bRinging = false; bRingBack = false;
	if (Audio) Audio->Stop(TEXT("ring2"), 0.02f);
	if (M->Web) M->Web->SetHandsetVisible(false);
	Host->SetCinematic(true);
	const TArray<FS47PhoneCue>& WardArray = S47PhoneScripts::WardArray();
	const TOptional<FS47CallTimeline> Spoken = Audio ? Audio->PhoneCall(WardArray) : TOptional<FS47CallTimeline>();
	if (Spoken)
	{
		for (const FS47CallCue& Cue : Spoken->Cues) Host->After(Cue.At, [this, Cue] { Toast(Caption(Cue.By, Cue.Text), FMath::Max(2.f, Cue.Duration)); });
	}
	else
	{
		if (Audio) Audio->CallLine(7.6f);
		const FString A = Caption(TEXT("WARD"), WardArray[0].Text), B = Caption(TEXT("WARD"), WardArray[1].Text);
		Host->After(0.3, [this, A] { Toast(A, 2.4f); });
		Host->After(2.8, [this, B] { Toast(B, 4.6f); });
	}
	Host->After(Spoken ? Spoken->Duration + 0.1 : 7.7, [this]
	{
		if (M->Web) M->Web->SetHandsetVisible(true);
		Host->SetCinematic(false);
		S.ArrayAsk = true;
		Note(FString::Printf(TEXT("%s. Ward called back. \"Put the array back on the schedule and tell me it stays there.\" The operations terminal on the west desk: ARRAY RESUME."), *Time()));
		Toast(TEXT("She hangs up. The operations terminal is on the west desk."), 3.4f);
		Save();
	});
}

// ---------- the corridor door ----------
// The doors are free (world/Doors.ts); the case remembers that this one was opened.

void US47Chapter2::DoorChanged(const FString& Id, bool bOpen)
{
	if (!bActive || Id != TEXT("south") || !bOpen || S.bDoor) return;
	S.bDoor = true;
	Save();
}

// ---------- the records room ----------

void US47Chapter2::Interactables()
{
	if (!M || !M->Inter || !Annex) return;
	auto Add = [this](const TCHAR* Id, TFunction<FString()> Label, TFunction<void()> Use)
	{
		FS47Interactable It;
		It.Id = Id;
		It.Object = Annex->Obj(Id);
		It.Label = MoveTemp(Label);
		It.Use = MoveTemp(Use);
		M->Inter->Add(It);
	};
	auto On = [this] { return bActive; };
	Add(TEXT("fieldBox"), [On] { return On() ? FString(TEXT("STATION 01 field records, 1947")) : FString(); }, [this] { TakeFieldBox(); });
	Add(TEXT("binder"), [On] { return On() ? FString(TEXT("Service copies, reference records")) : FString(); }, [this] { TakeBinder(); });
	Add(TEXT("lineage"), [On] { return On() ? FString(TEXT("Maintenance card index")) : FString(); }, [this] { TakeLineage(); });
	Add(TEXT("workTable"), [this, On] { return On() ? FString(AnyRead() ? TEXT("Lay out the records") : TEXT("Work table")) : FString(); }, [this] { OpenTable(); });
	Add(TEXT("recPhone"), [this, On]
	{
		if (!On()) return FString();
		if (PhoneHook.Label) { const FString L = PhoneHook.Label(); if (!L.IsEmpty()) return L; }
		return FString(S.Stage == TEXT("call-nora") ? TEXT("Call the key holder") : TEXT("Wall phone"));
	}, [this] { if (!(PhoneHook.Use && PhoneHook.Use(TEXT("room")))) UseRecPhone(); });
	Add(TEXT("vending"), [On] { return On() ? FString(TEXT("Vending machine")) : FString(); }, [this] { Toast(TEXT("Cola, orange soda, peanut butter crackers. Somebody's dime is stuck in the coin return.")); });
	Add(TEXT("officeDoor"), [On] { return On() ? FString(TEXT("Operations office")) : FString(); }, [this] { Toast(TEXT("Locked. Dale took the key home.")); });
	Add(TEXT("restroomDoor"), [On] { return On() ? FString(TEXT("Restrooms")) : FString(); }, [this] { Toast(TEXT("Not now.")); });
}

bool US47Chapter2::AnyRead() const
{
	const FS47Ch2State::FRead& R = S.Read;
	return R.bOriginal || R.bAmended || R.bLineage || R.bIndex;
}

void US47Chapter2::TakeFieldBox()
{
	const bool bFirst = !S.Read.bOriginal;
	S.Read.bOriginal = true;
	File(DocOriginal());
	if (!M || !M->UI) return;
	M->UI->Document(DocOriginal(), [this, bFirst]
	{
		if (!S.Read.bIndex)
		{
			S.Read.bIndex = true;
			File(DocIndex());
			Toast(TEXT("The same box holds the archive sleeve for the 1947 plate: an index card and a fixed-point sheet. Filed in the case."), 4.4f);
		}
		if (bFirst) Note(FString::Printf(TEXT("%s. The STATION 01 box: the original 1947 field record, signed N. Vega, and the archive sleeve for the plate."), *Time()));
		Save();
	});
}

void US47Chapter2::TakeBinder()
{
	const bool bFirst = !S.Read.bAmended;
	S.Read.bAmended = true;
	File(DocAmended());
	if (!M || !M->UI) return;
	M->UI->Document(DocAmended(), [this, bFirst]
	{
		if (bFirst) Note(FString::Printf(TEXT("%s. The service copy of the same 1947 record. Same title, same signature. It calls the mark a development fault."), *Time()));
		Save();
	});
}

void US47Chapter2::TakeLineage()
{
	const bool bFirst = !S.Read.bLineage;
	S.Read.bLineage = true;
	File(DocLineage());
	if (!M || !M->UI) return;
	M->UI->Document(DocLineage(), [this, bFirst]
	{
		if (bFirst) Note(FString::Printf(TEXT("%s. Maintenance card for B-12: the vane keeps an older survey, STATION 01."), *Time()));
		Save();
	});
}

void US47Chapter2::OpenBoard()
{
	if (M && M->Board.Open) M->Board.Open(FString(), FString());
}

void US47Chapter2::OpenTable()
{
	if (!AnyRead()) { Toast(TEXT("Nothing on the table yet. The records are on the shelves, the bookcase and in the card index."), 4); return; }
	if (M && M->Board.Open) { M->Board.Open(TEXT("SARO ARCHIVE"), TEXT("ch2")); return; }
	// No case board in this version yet: the archive table the chapter had before it (Chapter2.ts
	// openTable before 46ad542), with the same compare() and route() behind its buttons.
	if (!M || !M->UI) return;
	FImages& Im = Images;
	if (Im.Original.IsEmpty()) Im.Original = S47Drawings2::Arrangement(true);
	if (Im.Amended.IsEmpty()) Im.Amended = S47Drawings2::Arrangement(false);
	if (Im.Index.IsEmpty()) Im.Index = S47Drawings2::SleeveMark();
	if (Im.Icons.Num() == 0) for (const TCHAR* K : { TEXT("triangle-bar"), TEXT("triangle"), TEXT("three-bars") }) Im.Icons.Add(K, S47Drawings2::MarkIcon(K));
	const FS47Ch2State::FRead& R = S.Read;
	S47Panels2::FArchive O;
	O.Page = S.bP05 ? TEXT("route") : S.bP04 ? (R.bLineage && R.bIndex ? TEXT("route") : R.bLineage ? TEXT("index") : TEXT("lineage"))
		: R.bOriginal && R.bAmended ? TEXT("compare") : R.bOriginal ? TEXT("original") : R.bAmended ? TEXT("amended") : R.bLineage ? TEXT("lineage") : TEXT("index");
	O.Sources.Add(TEXT("original"), { R.bOriginal, TEXT("ORIGINAL FIELD PROTOCOL / E07"), ORIGINAL, Im.Original, WHERE_ORIGINAL });
	O.Sources.Add(TEXT("amended"), { R.bAmended, TEXT("AMENDED SERVICE COPY / E06"), AMENDED, Im.Amended, WHERE_AMENDED });
	O.Sources.Add(TEXT("lineage"), { R.bLineage, TEXT("MODERN MAINTENANCE CARD / B-12"), LINEAGE, FString(), WHERE_LINEAGE });
	O.Sources.Add(TEXT("index"), { R.bIndex, TEXT("ARCHIVE SLEEVE / E08 INDEX"), INDEX, Im.Index, WHERE_INDEX });
	O.Status = [this]
	{
		return FString(S.bP05 ? TEXT("P04 RECORDED  /  P05 RECORDED  /  Sources remain available for review")
			: S.bP04 ? TEXT("P04 RECORDED  /  P05: Trace the reference from B-12 to a field destination")
			: TEXT("P04: Establish what changed  /  P05: Identify a supported field destination"));
	};
	O.P04 = [this] { return S.bP04; };
	O.P05 = [this] { return S.bP05; };
	O.Supported04 = SUPPORTED_04; O.Supported05 = SUPPORTED_05;
	O.OnCompare = [this](const FString& C) { return Compare(C); };
	O.OnRoute = [this](const FString& Dest, const FString& Survey, const FString& Marker) { return Route(Dest, Survey, Marker); };
	O.Hints04 = HINTS_04; O.Hints05 = HINTS_05;
	O.Icons = Im.Icons;
	S47Panels2::Archive(M->UI, O);
}

// The five records on the board, with the lines that matter on each. Each picture on its own:
// reading E07, E06 or E08 on the shelf already drew that one, and the mark icons were then never
// drawn, which left P05 an empty page.
TArray<FS47BoardCard> US47Chapter2::BoardCards()
{
	if (!bStarted) return {};
	FImages& Im = Images;
	if (Im.Original.IsEmpty()) Im.Original = S47Drawings2::Arrangement(true);
	if (Im.Amended.IsEmpty()) Im.Amended = S47Drawings2::Arrangement(false);
	if (Im.Index.IsEmpty()) Im.Index = S47Drawings2::SleeveMark();
	if (Im.Icons.Num() == 0) for (const TCHAR* K : { TEXT("triangle-bar"), TEXT("triangle"), TEXT("three-bars") }) Im.Icons.Add(K, S47Drawings2::MarkIcon(K));
	auto Card = [](const TCHAR* Id, const TCHAR* Doc, const TCHAR* Stamp, const TCHAR* Title, bool bHave, const FString& Where, const FString& Image, const FString& Full, double X, double Y, double W, TArray<FS47BoardLine> Lines)
	{
		FS47BoardCard C;
		C.Id = Id; C.Doc = Doc; C.Stamp = Stamp; C.Title = Title; C.bHave = bHave; C.Where = Where; C.Image = Image; C.Full = Full;
		C.X = X; C.Y = Y; C.W = W;
		C.Lines = MoveTemp(Lines);
		return C;
	};
	auto Line = [](const TCHAR* Id, const TCHAR* Text, const FString& Icon = FString()) { return FS47BoardLine{ Id, Text, Icon }; };
	return {
		Card(TEXT("e07"), TEXT("e07"), TEXT("E07"), TEXT("ORIGINAL FIELD RECORD"), S.Read.bOriginal, WHERE_ORIGINAL, Im.Original, ORIGINAL, 14, 14, 272, {
			Line(TEXT("e07.head"), TEXT("FIELD SURVEY RECORD / STATION 01 / 1947")),
			Line(TEXT("e07.mark"), TEXT("The retained mark did not follow the new position of the physical vane.")),
			Line(TEXT("e07.abc"), TEXT("Three references: A, the fixed survey point; B, the optical comparison vane; C, the closing sight line.")),
			Line(TEXT("e07.sig"), TEXT("N. VEGA / FIELD TECHNICIAN")) }),
		Card(TEXT("e06"), TEXT("e06"), TEXT("E06"), TEXT("AMENDED SERVICE COPY"), S.Read.bAmended, WHERE_AMENDED, Im.Amended, AMENDED, 298, 14, 272, {
			Line(TEXT("e06.fault"), TEXT("The additional mark is attributed to a fault in plate development.")),
			Line(TEXT("e06.omit"), TEXT("The incomplete closing sight line has been omitted from the service copy.")),
			Line(TEXT("e06.repeat"), TEXT("No repeat observation is required.")),
			Line(TEXT("e06.sig"), TEXT("N. VEGA / FIELD TECHNICIAN")) }),
		Card(TEXT("rx"), TEXT("printout"), TEXT("RX"), TEXT("PRINTOUT, 02:14"), true, FString(), FString(), FString(), 898, 640, 266, {
			Line(TEXT("rx.dist"), TEXT("SOURCE DISTANCE:  -39 LY")) }),
		Card(TEXT("b12"), TEXT("b12lineage"), TEXT("B-12"), TEXT("LINEAGE CARD"), S.Read.bLineage, WHERE_LINEAGE, FString(), LINEAGE, 600, 14, 278, {
			Line(TEXT("b12.id"), TEXT("The B-12 vane retains the fixed-point survey identified as STATION 01.")),
			Line(TEXT("b12.mark"), TEXT("Register mark: outlined triangle with a short bar beneath it."), Im.Icons.FindRef(TEXT("triangle-bar"))),
			Line(TEXT("b12.site"), TEXT("SITE REGISTER: STATION 01 / OLD SURVEY STATION.")) }),
		Card(TEXT("e08"), TEXT("e08"), TEXT("E08"), TEXT("ARCHIVE SLEEVE"), S.Read.bIndex, WHERE_INDEX, Im.Index, INDEX, 600, 274, 278, {
			Line(TEXT("e08.id"), TEXT("PLATE / 1947 / STATION 01")),
			Line(TEXT("e08.mark"), TEXT("Fixed-point mark: outlined triangle with a short bar beneath it."), Im.Icons.FindRef(TEXT("triangle-bar"))),
			Line(TEXT("e08.dest"), TEXT("Field destination: OLD SURVEY STATION.")),
			Line(TEXT("e08.elev"), TEXT("A similar triangle without the bar is a general elevation symbol."), Im.Icons.FindRef(TEXT("triangle"))) }),
	};
}

// P04: the Unity rules and replies.
FS47Result US47Chapter2::Compare(const FString& Finding)
{
	if (!S.Read.bOriginal || !S.Read.bAmended) return { false, TEXT("Read both the original protocol and amended copy before recording a comparison.") };
	if (Finding != TEXT("omitted-c"))
	{
		S.Wrong04++; Save();
		return { false, Finding == TEXT("author-guilt")
			? TEXT("The shared signature identifies the report. It does not establish the author's motive or responsibility. Which reference changes between the two versions?")
			: TEXT("Compare the arrangement, not just the amended explanation. Which reference appears in the original and is omitted from the service copy?") };
	}
	if (!S.bP04)
	{
		S.bP04 = true; S.Finding = Finding;
		File(DocFinding04());
		Note(FString::Printf(TEXT("%s. P04: the 1947 record was changed after the run. The service copy leaves out C, the closing sight line, and calls the retained mark a development fault. That is a changed report, not a motive."), *Time()));
		Click();
		Save();
	}
	return { true, SUPPORTED_04 };
}

// P05: the Unity rules and replies, checked in the same order.
FS47Result US47Chapter2::Route(const FString& Destination, const FString& SurveyId, const FString& Marker)
{
	if (!S.bP04) return { false, TEXT("Record what changed in the two field records before preparing the field destination.") };
	if (!S.Read.bLineage || !S.Read.bIndex) return { false, TEXT("Read both the B-12 lineage card and archive sleeve index. A place name alone does not establish the connection.") };
	auto Wrong = [this](const TCHAR* Text) { S.Wrong05++; Save(); return FS47Result{ false, Text }; };
	if (SurveyId != TEXT("STATION 01")) return Wrong(TEXT("Match the survey ID in both sources. -39 LY is not a survey ID."));
	if (Marker != TEXT("triangle-bar")) return Wrong(TEXT("The matching fixed-point mark has a short bar beneath the outlined triangle. A triangle without the bar is only an elevation symbol."));
	if (Destination != TEXT("old-survey-station")) return Wrong(TEXT("The B-12 service lineage and matching archive sleeve identify OLD SURVEY STATION. The documents do not establish another destination."));
	if (!S.bP05)
	{
		S.bP05 = true; S.Destination = Destination; S.SurveyId = SurveyId; S.Marker = Marker;
		S.Stage = TEXT("call-nora");
		File(DocFinding05()); File(DocAccess()); File(DocMap());
		Note(FString::Printf(TEXT("%s. P05: B-12 keeps the STATION 01 survey. The sleeve matches the ID and the triangle with a bar. Destination: OLD SURVEY STATION."), *Time()));
		Note(TEXT("Field access sheet: key holder N. VEGA, SIERRA MOTOR COURT, (505) 555-0119. The same name signs both versions of the 1947 record."));
		Click();
		Toast(TEXT("Field access sheet filed. The key holder has a phone number."), 4);
		Save();
	}
	return { true, SUPPORTED_05 };
}

// ---------- N. Vega ----------

void US47Chapter2::UseRecPhone()
{
	if (!M || !Host) return;
	US47Audio* Audio = M->Audio;
	if (S.Stage != TEXT("call-nora"))
	{
		Toast(S.bCalled ? TEXT("You could call back. You would get the same answer.") : TEXT("Wall phone. Internal line and one outside line."));
		return;
	}
	Host->SetCinematic(true);
	// (Ward's call-back left ringing in the control room stops as the line here is picked up)
	if (bRingBack) { bRinging = false; bRingBack = false; BackRing = 0; if (Audio) Audio->Stop(TEXT("ring2"), 0.3f); }
	Click(Annex ? Annex->Obj(TEXT("recPhone")).WorldPosition() : FVector::ZeroVector);
	Toast(TEXT("(505) 555-0119."), 2.4f);
	const float Dial = Audio ? Audio->Dial(PHONE) : 1.5f;
	const float Answered = (Audio ? Audio->Ringback(2, Dial + 0.5f) : Dial + 0.5f + 2 * 3.2f) + 0.2f;
	TFunction<void()> Finish = [this]
	{
		Host->SetCinematic(false);
		S.bCalled = true; S.Stage = TEXT("complete");
		// (the chapter ends: Ward does not call back about the array now; the console still has it)
		if (bRingBack) { bRinging = false; bRingBack = false; if (M->Audio) M->Audio->Stop(TEXT("ring2"), 0.3f); }
		BackT = -1;
		File(DocMap());
		Note(FString::Printf(TEXT("%s. Called the key holder. N. Vega, Sierra Motor Court. She wants me at the cut cable at STATION 01 before I come to her, and she wants the originals, not copies."), *Time()));
		Toast(TEXT("She hangs up before you can ask anything else."), 3.4f);
		Save();
		Host->After(3.6, [this] { if (OnEnd) OnEnd(); });
	};
	auto Cap = [this](double At, const FString& Text, float Secs) { Host->After(At, [this, Text, Secs] { Toast(Text, Secs); }); };
	TFunction<void()> BeginCall = [this, Finish, Cap]
	{
		const TOptional<FS47CallTimeline> Spoken = M->Audio ? M->Audio->PhoneCall(S47PhoneScripts::Nora()) : TOptional<FS47CallTimeline>();
		if (Spoken)
		{
			for (const FS47CallCue& Cue : Spoken->Cues) Host->After(Cue.At, [this, Cue] { Toast(Cue.bAside ? Cue.Text : Caption(Cue.By, Cue.Text), FMath::Max(2.f, Cue.Duration)); });
			Host->After(Spoken->Duration + 0.1, Finish);
			return;
		}
		if (M->Audio) M->Audio->CallLine(13.6f);
		Cap(0.4, TEXT("N. VEGA: \"Sierra Motor Court.\""), 2.4f);
		Cap(2.8, TEXT("You give your name and SARO, and read her the survey number from the sleeve."), 3.4f);
		Cap(6.2, TEXT("N. VEGA: \"Station One. Nobody has asked me for that key in a long time.\""), 3.6f);
		Cap(9.6, TEXT("N. VEGA: \"Don't send me copies. Go out to the station first and look at the cable, where it was cut. Then bring me the originals.\""), 4.4f);
		Host->After(13.8, Finish);
	};
	if (!Audio || Audio->bKinskiCalls || Audio->bCharacterVoices) Host->After(Answered, BeginCall);
	else
	{
		const double Talk = 13.6;
		Host->After(Answered, [this, Talk] { if (M->Audio) M->Audio->CallLine(float(Talk)); });
		Cap(Answered + 0.4, TEXT("N. VEGA: \"Sierra Motor Court.\""), 2.4f);
		Cap(Answered + 2.8, TEXT("You give your name and SARO, and read her the survey number from the sleeve."), 3.4f);
		Cap(Answered + 6.2, TEXT("N. VEGA: \"Station One. Nobody has asked me for that key in a long time.\""), 3.6f);
		Cap(Answered + 9.6, TEXT("N. VEGA: \"Don't send me copies. Go out to the station first and look at the cable, where it was cut. Then bring me the originals.\""), 4.4f);
		Host->After(Answered + Talk + 0.2, Finish);
	}
}

// ---------- documents ----------

FS47Doc US47Chapter2::DocOriginal()
{
	FS47Doc D;
	D.Id = TEXT("e07"); D.Title = TEXT("E07 / Field survey record, 1947"); D.Kind = TEXT("typed"); D.Stamp = TEXT("E07"); D.Page = ORIGINAL;
	if (Images.Original.IsEmpty()) Images.Original = S47Drawings2::Arrangement(true);
	D.Image = Images.Original;
	D.Transcript = TEXT("The original field protocol from the STATION 01 box, signed N. Vega. The arrangement drawing is clipped to it: A, the fixed survey point; B, the comparison vane; C, the closing sight line.");
	return D;
}

FS47Doc US47Chapter2::DocAmended()
{
	FS47Doc D;
	D.Id = TEXT("e06"); D.Title = TEXT("E06 / Amended service copy, 1947"); D.Kind = TEXT("typed"); D.Stamp = TEXT("E06"); D.Page = AMENDED;
	if (Images.Amended.IsEmpty()) Images.Amended = S47Drawings2::Arrangement(false);
	D.Image = Images.Amended;
	D.Transcript = TEXT("The service copy of the same record, filed with the routine reference records. Same title, same signature. Its own arrangement drawing is clipped to the back.");
	return D;
}

FS47Doc US47Chapter2::DocLineage() const
{
	FS47Doc D;
	D.Id = TEXT("b12lineage"); D.Title = TEXT("B-12 / Reference lineage card"); D.Kind = TEXT("typed"); D.Stamp = TEXT("B-12"); D.Page = LINEAGE;
	D.Transcript = TEXT("A card from the maintenance index. It ties today's B-12 vane to an older survey, and names the place in the site register.");
	return D;
}

FS47Doc US47Chapter2::DocIndex()
{
	FS47Doc D;
	D.Id = TEXT("e08"); D.Title = TEXT("E08 / Archive sleeve index"); D.Kind = TEXT("typed"); D.Stamp = TEXT("E08"); D.Page = INDEX;
	if (Images.Index.IsEmpty()) Images.Index = S47Drawings2::SleeveMark();
	D.Image = Images.Index;
	D.Transcript = TEXT("From the same box: the archive sleeve for the 1947 plate, with its index card and the fixed-point sheet. This is the index, not a photograph you took.");
	return D;
}

FS47Doc US47Chapter2::DocFinding04() const
{
	FS47Doc D;
	D.Id = TEXT("p04"); D.Title = TEXT("P04 / The amended record"); D.Kind = TEXT("typed"); D.Stamp = TEXT("P04"); D.Page = SUPPORTED_04;
	D.Transcript = TEXT("Your finding at the records table. It establishes that the report was changed. It does not establish why, or who cut anything.");
	return D;
}

FS47Doc US47Chapter2::DocFinding05() const
{
	FS47Doc D;
	D.Id = TEXT("p05"); D.Title = TEXT("P05 / Field destination"); D.Kind = TEXT("typed"); D.Stamp = TEXT("P05"); D.Page = SUPPORTED_05;
	D.Transcript = TEXT("Your second finding: the reference in today's B-12 leads to the old survey station, by survey ID and fixed-point mark together.");
	return D;
}

FS47Doc US47Chapter2::DocAccess() const
{
	FS47Doc D;
	D.Id = TEXT("access"); D.Title = TEXT("STATION 01 / Field access sheet"); D.Kind = TEXT("typed"); D.Stamp = TEXT("ACCESS"); D.Page = ACCESS;
	D.Transcript = TEXT("Folded into the matching sleeve. The key holder for the old station is listed by initial and surname, at the motel down the road: N. Vega. The same name signs both versions of the 1947 record.");
	return D;
}

FS47Doc US47Chapter2::DocMap() const
{
	FS47Doc D;
	D.Id = TEXT("casemap"); D.Title = TEXT("Case map / SARO and field sites"); D.Kind = TEXT("map"); D.Page = FString();
	D.Image = S47Drawings2::CaseMap(S.bP05, S.bCalled);
	D.Transcript = S.bCalled
		? TEXT("Sketched from the site register. SARO, the old survey station along a dashed road, and now the motel where the key holder answered. Not to scale.")
		: TEXT("Sketched from the site register. SARO, and the old survey station along a dashed road. Not to scale.");
	return D;
}

// ---------- per frame ----------

void US47Chapter2::Update(double Dt, double T)
{
	if (!bActive || !M || !Host) return;
	const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
	const FS47Area Z = Annex ? Annex->Zone(TEXT("records")) : FS47Area();
	if (!S.bEntered && Z.Contains(P))
	{
		S.bEntered = true;
		Note(FString::Printf(TEXT("%s. The records room. Steel shelves, three filing cabinets, a card index for maintenance. A box marked STATION 01 sticks out of the west shelves."), *Time()));
		Toast(TEXT("SARO records room."), 2.4f);
		Save();
	}
	// Ward calls back about the array a few seconds after her first call, with the player in the
	// control room and nothing else going (or just outside it, in the corridor); let ring, it stops,
	// and she tries again later
	const bool bHere = M->Web && M->Web->Rect(TEXT("room.bounds")).Contains(P);
	bool bHeard = bHere;
	for (const TCHAR* K : EARSHOT) if (!bHeard && Annex && Annex->Zone(K).Contains(P)) bHeard = true;
	if (bRingBack && bRinging && (BackRing += Dt) > BACK_RING)
	{
		bRinging = false; bRingBack = false; BackT = BACK_AGAIN;
		if (M->Audio) M->Audio->Stop(TEXT("ring2"), 0.3f);
		if (bHeard) Toast(TEXT("The ringing stops."), 2.4f);
	}
	else if (!bRinging && BackT >= 0 && !S.ArrayAsk.Get(false) && !Host->IsCinematic() && S.Stage != TEXT("complete"))
	{
		if (BackT > 0) BackT = FMath::Max(0.0, BackT - Dt);
		else if (bHeard && (Quiet ? Quiet() : true)) CallBack(bHere);
	}
	if (M->UI) M->UI->Objective(Objective());
}

// ---------- the host (Prologue.ts, main.ts) ----------

void US47Chapter2::BeginChapter2(const S47J::FObj& SavedCase)
{
	// Prologue.beginChapter2: after the B-12 report, back at the desk, the supervisor's line rings.
	// (setPhase('ch2') is the host's chapter key, set by BeginChapter; lookAssist is the prologue's
	// own and is already 0 by now)
	if (!Host) return;
	Host->SetCinematic(false);
	Host->ClearTimers();
	const S47J::FObj Ch2 = S47J::Obj(SavedCase, TEXT("ch2"));
	if (Ch2.IsValid())
	{
		Host->NotesMutable() = S47J::Strs(SavedCase, TEXT("notes"));
		Host->SetClock(S47J::Num(SavedCase, TEXT("clock"), Host->Clock()));
	}
	else
	{
		Host->SetClock(FMath::Max(Host->Clock(), Host->EventClockSec() + 40 * 60));
		if (M && M->Player) M->Player->Place(3.35, 1.45, 2.75); // at the supervisor desk, facing the phone
	}
	if (M && M->UI) M->UI->Fade(false);
	Host->BeginChapter(TEXT("ch2"), Ch2);
	if (Host->OnCheckpoint) Host->OnCheckpoint(TEXT("chapter2"));
}

void US47Chapter2::JumpChapter2()
{
	// Prologue.jumpChapter2: chapter two from a checkpoint, or for testing straight after a
	// finished chapter one (without photographs) when there is no saved case.
	if (!Host) return;
	const S47J::FObj Saved = Host->LoadCase ? Host->LoadCase() : S47J::FObj();
	Host->AfterPrologue(Saved);
	S47J::FObj Done = Saved;
	if (!(Saved.IsValid() && S47J::Bool(S47J::Obj(Saved, TEXT("s")), TEXT("filed"))))
	{
		const S47J::FObj Ch1 = S47J::New();
		S47J::Set(Ch1, TEXT("v"), 1); S47J::Set(Ch1, TEXT("stage"), TEXT("complete")); S47J::Set(Ch1, TEXT("eventClock"), Host->EventClockSec());
		S47J::Set(Ch1, TEXT("camera"), true); S47J::Set(Ch1, TEXT("door"), true); S47J::Set(Ch1, TEXT("labDoor"), true); S47J::Set(Ch1, TEXT("log"), true); S47J::Set(Ch1, TEXT("returned"), true);
		S47J::Set(Ch1, TEXT("dev1"), 3); S47J::Set(Ch1, TEXT("dev2"), 3); S47J::Set(Ch1, TEXT("ref"), true); S47J::Set(Ch1, TEXT("hyp"), TEXT("light")); S47J::Set(Ch1, TEXT("method"), TEXT("passive"));
		S47J::Set(Ch1, TEXT("observed"), true); S47J::Set(Ch1, TEXT("concluded"), true); S47J::Set(Ch1, TEXT("filed"), true);
		S47J::Set(Ch1, TEXT("wrongRef"), 0); S47J::Set(Ch1, TEXT("wrongHyp"), 0); S47J::Set(Ch1, TEXT("wrongConcl"), 0); S47J::Set(Ch1, TEXT("rejected"), 0);
		Done = S47J::New();
		S47J::Set(Done, TEXT("s"), Ch1);
		S47J::Set(Done, TEXT("notes"), Host->Notes());
		S47J::Set(Done, TEXT("clock"), Host->EventClockSec() + 35 * 60);
	}
	// Prologue.beginChapter1(done), the host's part, when chapter one is in this version (when it
	// is not, its part is skipped: chapter two begins straight after the prologue). The integrator
	// may put chapter one's own begin here instead.
	if (Host->Chapter(TEXT("ch1")))
	{
		Reset(); // this.ch2.reset(): a new or restored chapter one; chapter two begins after it
		Host->SetCinematic(false);
		Host->ClearTimers();
		Host->NotesMutable() = S47J::Strs(Done, TEXT("notes"));
		Host->SetClock(S47J::Num(Done, TEXT("clock"), Host->Clock()));
		if (M && M->UI) M->UI->Fade(false);
		Host->BeginChapter(TEXT("ch1"), S47J::Obj(Done, TEXT("s")));
		if (Host->OnCheckpoint) Host->OnCheckpoint(TEXT("chapter1"));
	}
	BeginChapter2(S47J::Has(Done, TEXT("ch2")) ? Done : S47J::FObj());
}

void US47Chapter2::AfterChapterOne(const FString& Method)
{
	// main.ts ch1.onEnd: Ward has the report, and the night goes on in the records room. (If SARO
	// is going dark just then, the web version waits for the light and the telex, story/saroDark.ts;
	// that is not in this version.) The card's ring (ring.hold/out/again/shown) is bHoldRing,
	// RingOut, RingAgain and RingShown; AS47Story::ChapterBreak has no ring yet, so the hold is not
	// asked for and the phone rings as soon as the chapter begins behind the black.
	if (!Host) return;
	TWeakObjectPtr<US47Chapter2> W = this;
	Host->ChapterBreak({ TEXT("LOCAL INCIDENT S-03 / B-12 // FILED"),
		FString::Printf(TEXT("%s The eye saw one reference; the film kept two. One line on the receiver print is still unexplained: -39 LY."),
			Method == TEXT("passive") ? TEXT("The shielded lamp did not explain it.") : TEXT("Simple encoder drift did not explain it.")),
		TEXT("CHAPTER TWO"), TEXT("THE AMENDED RECORD"), [W] { if (W.IsValid()) W->BeginChapter2(S47J::FObj()); } });
}

// ---------- self test ----------

bool US47Chapter2::SelfTest(TArray<FString>& Lines)
{
	bool bOk = S47PhoneScripts::SelfTest(Lines);
	bOk &= S47Drawings2::SelfTest(Lines);
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s chapter two: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };

	// The board as tools/chapter2.py plays it (no world: no host, no screen, no sound).
	US47Chapter2* C = NewObject<US47Chapter2>(GetTransientPackage());
	C->BuildBook();
	C->bStarted = true; C->bActive = true;
	FS47RuleBook& B = *C->Book;
	auto Thread = [&B](const TCHAR* A, const TCHAR* Bb) { const TOptional<FS47Result> R = B.Connect(A, Bb, false); return R ? R->Text : FString(TEXT("(not this chapter's)")); };
	C->S.Read.bLineage = true;
	Check(C->Objective() == TEXT("CONTROL ROOM // THE SUPERVISOR LINE IS RINGING"), TEXT("objective while the phone rings"));
	Check(Thread(TEXT("b12.id"), TEXT("rx.dist")).Contains(TEXT("do not say anything")) && !C->S.bP04, TEXT("lines that say nothing about each other"));
	Check(!B.Record(TEXT("P04"))->bOk && B.Questions()[0].Slots == 2 && B.Questions()[1].Slots == 3, TEXT("P04 waits with two empty slots, P05 with three"));
	C->S.Read.bOriginal = C->S.Read.bIndex = C->S.Read.bAmended = true;
	FString R = Thread(TEXT("e07.sig"), TEXT("e06.sig"));
	Check(R.Contains(TEXT("who changed the record")) && B.Notes().Num() == 1 && B.Filled(TEXT("P04")).Num() == 0, TEXT("signature to signature: a note on the board, not on P04"));
	R = Thread(TEXT("e06.fault"), TEXT("e07.sig"));
	Check(R.Contains(TEXT("shared signature")) && C->S.Wrong04 == 1, TEXT("P04: the signature is not a motive (Unity reply)"));
	R = Thread(TEXT("e06.fault"), TEXT("e07.abc"));
	Check(R.Contains(TEXT("Compare the arrangement")) && C->S.Wrong04 == 2, TEXT("P04: the development explanation is not the comparison"));
	R = Thread(TEXT("e07.head"), TEXT("e06.repeat"));
	Check(R.Contains(TEXT("do not say anything")) && C->S.Links.Num() == 1 && C->S.Wrong04 == 2, TEXT("an empty thread falls slack and counts for nothing"));
	TArray<FString> Hints;
	for (int32 i = 0; i < 4; ++i) Hints.Add(*B.Hint(TEXT("P04")));
	Check(Hints[0].StartsWith(TEXT("What is different")) && Hints[2] == Hints[3] && Hints[3].Contains(TEXT("Put the original plan")), TEXT("P04 hints: three levels, the fourth held back"));
	for (int32 i = 0; i < 2; ++i) Thread(TEXT("e07.head"), TEXT("e06.repeat"));
	Check(B.Hint(TEXT("P04"))->Contains(TEXT("Thread the three references")), TEXT("the fourth hint after two more misses"));
	R = Thread(TEXT("e07.abc"), TEXT("e06.omit"));
	Check(R.Contains(TEXT("leaves it out")) && R.Contains(TEXT("(P04: 1 of 2)")), TEXT("C goes into P04 by itself"));
	R = Thread(TEXT("e07.mark"), TEXT("e06.fault"));
	Check(R.Contains(TEXT("RECORD it")) && !C->S.bP04, TEXT("the retained mark and the development fault fill P04"));
	const TOptional<FS47Result> Rec4 = B.Record(TEXT("P04"));
	Check(Rec4 && Rec4->bOk && C->S.bP04 && C->S.Wrong04 == 2 && B.Questions()[0].Done.StartsWith(TEXT("SUPPORTED")), TEXT("P04 recorded: C was omitted"));
	C->S.Stage = TEXT("records"); C->S.bEntered = true;
	Check(C->Objective() == TEXT("SARO RECORDS // ON THE CASE BOARD: MATCH THE B-12 LINEAGE TO THE ARCHIVE INDEX") && C->Tasks().Num() == 8, TEXT("objective and tasks after P04"));
	R = Thread(TEXT("rx.dist"), TEXT("e08.id"));
	Check(R.Contains(TEXT("1947 on the record")) && C->S.Wrong05 == 0 && C->S.Links.Contains(TEXT("e08.id|rx.dist")), TEXT("-39 against the 1947 sleeve holds as a thin thread"));
	R = Thread(TEXT("b12.mark"), TEXT("e08.elev"));
	Check(R.Contains(TEXT("only an elevation symbol")) && C->S.Wrong05 == 1, TEXT("P05: a triangle without the bar is refused"));
	R = Thread(TEXT("b12.id"), TEXT("e08.id"));
	Check(R.Contains(TEXT("Same survey")) && R.Contains(TEXT("(P05: 1 of 3)")), TEXT("STATION 01 on the card and on the sleeve"));
	R = Thread(TEXT("b12.mark"), TEXT("e08.mark"));
	Check(R.Contains(TEXT("Same fixed-point mark")) && !B.Record(TEXT("P05"))->bOk, TEXT("the two triangles with the bar"));
	R = Thread(TEXT("b12.site"), TEXT("e08.dest"));
	Check(R.Contains(TEXT("RECORD it")), TEXT("the two place names fill P05"));
	const TOptional<FS47Result> Rec5 = B.Record(TEXT("P05"));
	Check(Rec5 && Rec5->bOk && C->S.bP05 && C->S.Stage == TEXT("call-nora") && C->S.Wrong05 == 1 && C->S.Marker == TEXT("triangle-bar"), TEXT("P05 recorded: OLD SURVEY STATION"));
	Check(C->Objective() == TEXT("STATION 01 // CALL THE KEY HOLDER FROM THE RECORDS ROOM PHONE") && C->Tasks().Num() == 9, TEXT("objective and tasks after P05"));
	// a save from before the board: P04 and P05 recorded, the first board's pins
	C->S.Links = { TEXT("P04|n.c"), TEXT("P05|n.id") };
	B.Backfill();
	bool bNoP0 = true;
	for (const FString& K : C->S.Links) if (K.Contains(TEXT("P0"))) bNoP0 = false;
	Check(C->S.Links.Num() == 5 && bNoP0, TEXT("an older save gets its five threads back"));
	// the old panel's replies (Compare and Route behind its buttons)
	Check(C->Compare(TEXT("author-guilt")).Text.StartsWith(TEXT("The shared signature")) && C->S.Wrong04 == 3, TEXT("the panel's P04 replies"));
	Check(C->Route(TEXT("old-survey-station"), TEXT("-39 LY AS A YEAR CODE"), TEXT("triangle-bar")).Text == TEXT("Match the survey ID in both sources. -39 LY is not a survey ID.") && C->S.Wrong05 == 2, TEXT("the panel's P05 replies, in the TypeScript's order"));
	// the cards
	const TArray<FS47BoardCard> Cards = B.Cards();
	Check(Cards.Num() == 5 && Cards[2].Id == TEXT("rx") && Cards[2].Doc == TEXT("printout") && Cards[3].Lines.Num() == 3 && Cards[4].Lines.Num() == 4, TEXT("five records on the board, with their lines"));
	// the saved state, with the TypeScript's field names
	C->S.ArrayAsk = true;
	const S47J::FObj O = C->S.Save();
	const FString Json = S47J::Write(O);
	Check(Json.StartsWith(TEXT("{\"v\":1,\"stage\":\"call-nora\",\"answered\":false,\"door\":false,\"entered\":true,\"read\":{\"original\":true,\"amended\":true,\"lineage\":true,\"index\":true},\"p04\":true,\"p05\":true,\"finding\":\"omitted-c\",\"destination\":\"old-survey-station\",\"surveyId\":\"STATION 01\",\"marker\":\"triangle-bar\",\"wrong04\":3,\"wrong05\":2,\"called\":false,\"links\":[")) && Json.EndsWith(TEXT("],\"arrayAsk\":true}")),
		TEXT("Ch2State in the web version's JSON"));
	const S47J::FObj Old = S47J::Read(TEXT("{\"v\":1,\"stage\":\"records\",\"answered\":true,\"door\":true,\"entered\":false,\"read\":{\"original\":true},\"p04\":false,\"p05\":false,\"finding\":\"\",\"destination\":\"\",\"surveyId\":\"\",\"marker\":\"\",\"wrong04\":1,\"wrong05\":0,\"called\":false,\"later\":7}"));
	const FS47Ch2State L = FS47Ch2State::Load(Old);
	Check(L.Stage == TEXT("records") && L.bAnswered && L.Read.bOriginal && !L.Read.bIndex && L.Wrong04 == 1 && !L.ArrayAsk && L.Links.Num() == 0
		&& S47J::Num(L.Save(), TEXT("later")) == 7 && !S47J::Has(L.Save(), TEXT("arrayAsk")), TEXT("an older save: missing fields fresh, unknown ones kept"));
	C->MarkAsGarbage();
	return bOk;
}


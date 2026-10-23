#include "S47Chapter4.h"
#include "S47Chapter3.h"

#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "S47Audio.h"
#include "S47CB.h"
#include "S47Canvas.h"
#include "S47Chapter1.h"
#include "S47Chapter2.h"
#include "S47Chapter5.h"
#include "S47HUD.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Motel.h"
#include "S47NpcDialogue.h"
#include "S47Room6.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Walkie.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Ch4, Log, All);

namespace
{
	// ---------- papers (design bible story material, A5 to A7) ----------
	const TCHAR* CARD = TEXT("T. VEGA / LAST OBSERVATION\n\nOpening the lamp circuit removes the source, but not the retained reference. Masking the closing sight line prevents an organised return. These are different results.\n\nThe final reading must be taken from outside the marked reference surface. A completed observation may leave its own reference behind. Do not connect a fresh observer to a run merely to finish an old one.");
	const TCHAR* LETTER = TEXT("Nora,\n\nI left the spare key in the mug because you said I would lose it in the sand. You were right about the reference moving. I checked it twice before I wrote it down.\n\nWhen we finish, I owe you breakfast and an apology for calling your first plate a mistake.\n\nT.");
	const TCHAR* CORRECTION = TEXT("CORRECTION TO THE AMENDED FIELD RECORD\n\nI removed the closing reference from the service copy. I attributed the additional mark to development because I believed the complete arrangement would be repeated if the original finding remained on the record.\n\nThe interruption was deliberate. Tomás Vega was present in the active reference when I broke the connection. He was not accounted for after the interruption. I cannot replace that fact with an instrument fault.\n\nNORA VEGA / 1986");

	const TCHAR* SUPPORTED_P10 = TEXT("P10 SUPPORTED / Tonight's B-12 control print and the 1947 plate carry the same feature: a reference that stays on the film after its local source is taken away. N. Vega knows it from her own plate in 1947.");
	const TCHAR* SUPPORTED_P11 = TEXT("P11 SUPPORTED / N. Vega removed C from the service copy and blamed development because she believed the complete record would let someone repeat the arrangement. She cut the cable herself while T. Vega stood inside the active reference. The record does not say what became of him.");
	const TCHAR* SUPPORTED_P12 = TEXT("P12 SUPPORTED / The return line repeats T. Vega's 1947 correction word for word. A new question gets the same fragment back, not an answer. It is a retained reference, not a conversation.");
	FString Supported(const FString& K) { return K == TEXT("p10") ? SUPPORTED_P10 : K == TEXT("p11") ? SUPPORTED_P11 : SUPPORTED_P12; }

	const TCHAR* N = TEXT("N. VEGA");
	const TCHAR* YOU = TEXT("YOU");
	const TCHAR* LINE = TEXT("THE LINE");
	using FLine = S47Panels::FTalkLine;
	FLine Nv(const FString& Text) { return { N, Text }; }
	FLine You(const FString& Text) { return { YOU, Text }; }
	FLine Aside(const FString& Text) { return { FString(), Text }; }

	// The evidence that can go on the table for P10.
	struct FChipDef { const TCHAR* Id; const TCHAR* Label; };
	const FChipDef CHIPS[] = {
		{ TEXT("frame01"), TEXT("PHOTO 01 / S-03 print") },
		{ TEXT("frame02"), TEXT("PHOTO 02 / B-12 control") },
		{ TEXT("e07"), TEXT("E07 / The 1947 original") },
		{ TEXT("e06"), TEXT("E06 / The amended copy") },
		{ TEXT("frame03"), TEXT("PHOTO 03 / Fixed point A") },
		{ TEXT("frame04"), TEXT("PHOTO 04 / Cable break") },
	};

	// ---------- the case board ----------
	// What her papers and the return line show together with what was found before tonight
	// (story/boardRules.ts). Her papers are on the board once she has put them on the table.
	const TMap<FString, FS47BookNote>& NOTES()
	{
		static const TMap<FString, FS47BookNote> M = {
			{ TEXT("n.why"), { TEXT("She left C out on purpose. She believed the complete record would let someone connect the arrangement again."), TEXT("P11"), false } },
			{ TEXT("n.cut4"), { TEXT("The omission and the cut had the same purpose. She broke the connection herself, with Tomás inside the reference."), TEXT("P11"), false } },
			{ TEXT("n.line"), { TEXT("The line gives back the 1947 correction, word for word, to a new question. It repeats; it does not answer."), TEXT("P12"), false } },
			{ TEXT("n.key"), { TEXT("Her question had an answer: his letter says the key was in the mug. The line did not give it."), FString(), false } },
		};
		return M;
	}
	const TCHAR* NOT_HIM = TEXT("His letter says he checked the reference twice. His readings were not the problem.");
	FS47BookRule Rule(const TCHAR* A, const TCHAR* B, const FString& Note, const FString& Text, const FString& Wrong = FString())
	{
		FS47BookRule R;
		R.A = A; R.B = B; R.Note = Note; R.Text = Text; R.Wrong = Wrong;
		return R;
	}
	TArray<FS47BookRule> RULES()
	{
		const TMap<FString, FS47BookNote>& Nt = NOTES();
		return {
			Rule(TEXT("e12.why"), TEXT("P04"), TEXT("n.why"), Nt[TEXT("n.why")].Text),
			Rule(TEXT("e12.cut"), TEXT("P08"), TEXT("n.cut4"), Nt[TEXT("n.cut4")].Text),
			Rule(TEXT("e12.dev"), TEXT("e06.fault"), FString(), TEXT("Her own correction says the mark was not a development fault. She wrote that it was one."), TEXT("P11")),
			Rule(TEXT("e13.twice"), TEXT("e12.why"), FString(), NOT_HIM, TEXT("P11")),
			Rule(TEXT("e13.twice"), TEXT("P04"), FString(), NOT_HIM, TEXT("P11")),
			Rule(TEXT("r6.line"), TEXT("e10.carrier"), TEXT("n.line"), Nt[TEXT("n.line")].Text),
			Rule(TEXT("r6.line"), TEXT("hut.rx"), TEXT("n.line"), Nt[TEXT("n.line")].Text),
			Rule(TEXT("r6.line"), TEXT("e13.key"), TEXT("n.key"), Nt[TEXT("n.key")].Text),
			Rule(TEXT("r6.line"), TEXT("e13.twice"), FString(), TEXT("He did not answer the question. It is the same fragment as in the 1947 timing log."), TEXT("P12")),
			// threads that hold without answering anything
			Rule(TEXT("e11.mask"), TEXT("e06.omit"), FString(), TEXT("Tomás wrote it first: masking the closing sight line prevents an organised return. Then she left C out of the copy.")),
			Rule(TEXT("e12.cut"), TEXT("frame04"), FString(), TEXT("The cut in FRAME 04 is hers.")),
		};
	}
	const TArray<FString> HINTS_11 = { TEXT("Why would anyone leave one line out of a record?"), TEXT("Her correction says why she removed C. What else did she do that night, for the same reason?"), TEXT("Tie her reason in E12 to what changed in the record, P04, and the deliberate interruption in E12 to the cut cable, P08.") };
	const TArray<FString> HINTS_12 = { TEXT("What did the line say, and where have you heard it before?"), TEXT("Compare it with the 1947 timing log, or with the receiver in the hut."), TEXT("Tie the return line to the recorded voice in E10.") };

	FS47Sound Snd(float Gain, float Rate = 1.f, const TOptional<FVector>& At = {})
	{
		FS47Sound O;
		O.Gain = Gain; O.Rate = Rate; O.At = At;
		return O;
	}
}

// ---------- the saved state ----------

S47J::FObj FS47Ch4State::Save() const
{
	S47J::FObj O = Extra.IsValid() ? S47J::Clone(Extra) : S47J::New();
	S47J::Set(O, TEXT("v"), 1);
	S47J::Set(O, TEXT("stage"), Stage);
	S47J::Set(O, TEXT("exitOpen"), bExitOpen);
	S47J::Set(O, TEXT("arrived"), bArrived);
	S47J::Set(O, TEXT("greeted"), bGreeted);
	S47J::Set(O, TEXT("placed"), Placed);
	S47J::Set(O, TEXT("shown"), bShown);
	S47J::Set(O, TEXT("p10"), bP10);
	S47J::Set(O, TEXT("wrong10"), Wrong10);
	S47J::FObj T = S47J::New();
	S47J::Set(T, TEXT("record"), Topics.bRecord);
	S47J::Set(T, TEXT("tomas"), Topics.bTomas);
	S47J::Set(T, TEXT("cable"), Topics.bCable);
	S47J::Set(O, TEXT("topics"), T);
	S47J::SetOrNull(O, TEXT("tone"), Tone);
	S47J::Set(O, TEXT("p11"), bP11);
	S47J::Set(O, TEXT("wrong11"), Wrong11);
	S47J::Set(O, TEXT("line"), bLine);
	S47J::Set(O, TEXT("p12"), bP12);
	S47J::Set(O, TEXT("wrong12"), Wrong12);
	S47J::Set(O, TEXT("told"), bTold);
	S47J::FObj R = S47J::New();
	S47J::Set(R, TEXT("card"), Read.bCard);
	S47J::Set(R, TEXT("letter"), Read.bLetter);
	S47J::Set(R, TEXT("correction"), Read.bCorrection);
	S47J::Set(O, TEXT("read"), R);
	S47J::FObj Sn = S47J::New();
	S47J::Set(Sn, TEXT("photo"), Seen.bPhoto);
	S47J::Set(Sn, TEXT("window"), Seen.bWindow);
	S47J::Set(O, TEXT("seen"), Sn);
	if (Cb) S47J::Set(O, TEXT("cb"), *Cb);
	if (Links) S47J::Set(O, TEXT("links"), *Links);
	if (Mark) S47J::Set(O, TEXT("mark"), *Mark);
	if (Down3) S47J::Set(O, TEXT("down3"), *Down3);
	return O;
}

FS47Ch4State FS47Ch4State::Load(const S47J::FObj& Saved)
{
	FS47Ch4State C;
	if (!Saved.IsValid()) return C;
	C.Extra = S47J::Clone(Saved);
	C.Stage = S47J::Str(Saved, TEXT("stage"), C.Stage);
	C.bExitOpen = S47J::Bool(Saved, TEXT("exitOpen"));
	C.bArrived = S47J::Bool(Saved, TEXT("arrived"));
	C.bGreeted = S47J::Bool(Saved, TEXT("greeted"));
	C.Placed = S47J::Strs(Saved, TEXT("placed"));
	C.bShown = S47J::Bool(Saved, TEXT("shown"));
	C.bP10 = S47J::Bool(Saved, TEXT("p10"));
	C.Wrong10 = S47J::Int(Saved, TEXT("wrong10"));
	const S47J::FObj T = S47J::Obj(Saved, TEXT("topics"));
	C.Topics.bRecord = S47J::Bool(T, TEXT("record"));
	C.Topics.bTomas = S47J::Bool(T, TEXT("tomas"));
	C.Topics.bCable = S47J::Bool(T, TEXT("cable"));
	C.Tone = S47J::Str(Saved, TEXT("tone"));
	C.bP11 = S47J::Bool(Saved, TEXT("p11"));
	C.Wrong11 = S47J::Int(Saved, TEXT("wrong11"));
	C.bLine = S47J::Bool(Saved, TEXT("line"));
	C.bP12 = S47J::Bool(Saved, TEXT("p12"));
	C.Wrong12 = S47J::Int(Saved, TEXT("wrong12"));
	C.bTold = S47J::Bool(Saved, TEXT("told"));
	const S47J::FObj R = S47J::Obj(Saved, TEXT("read"));
	C.Read.bCard = S47J::Bool(R, TEXT("card"));
	C.Read.bLetter = S47J::Bool(R, TEXT("letter"));
	C.Read.bCorrection = S47J::Bool(R, TEXT("correction"));
	const S47J::FObj Sn = S47J::Obj(Saved, TEXT("seen"));
	C.Seen.bPhoto = S47J::Bool(Sn, TEXT("photo"));
	C.Seen.bWindow = S47J::Bool(Sn, TEXT("window"));
	if (S47J::Has(Saved, TEXT("cb"))) C.Cb = S47J::Bool(Saved, TEXT("cb"));
	if (S47J::Has(Saved, TEXT("links"))) C.Links = S47J::Strs(Saved, TEXT("links"));
	if (S47J::Has(Saved, TEXT("mark"))) C.Mark = S47J::Bool(Saved, TEXT("mark"));
	if (S47J::Has(Saved, TEXT("down3"))) C.Down3 = S47J::Bool(Saved, TEXT("down3"));
	return C;
}

// ---------- registration (the constructor, and main.ts's part) ----------

US47Chapter4* US47Chapter4::Register()
{
	BuildBook();
	if (!M || !Host) return this;
	// (one chapter four at a time: a book left by an earlier start of the game goes)
	M->Books.RemoveAll([](const FS47RuleBook* B) { return B && B->Key() == TEXT("ch4"); });
	M->Books.Add(Book.Get());
	// World.ts: room 6 and the motel's front (MotelFront through Crossing's CourtSite), and the brick
	Room = NewObject<US47Room6>(this);
	Room->Init(M);
	Court = NewObject<US47Motel>(this);
	Court->Init(M);
	// main.ts: the CB under the truck's dash, and the night's character voices (ui.npcDialogue)
	if (!M->CB.IsValid()) M->CB = MakeShared<FS47CB>(M->Audio);
	if (M->UI && !M->UI->NpcDialogue.IsValid())
	{
		M->UI->NpcDialogue = MakeShared<FS47NpcDialogue>(M->Audio);
		M->UI->NpcDialogue->SetEnabled(M->Audio ? M->Audio->bCharacterVoices : true);
		// ui.onCharacterStart: audio.stopPhoneCalls(); cb.stop() (stopNarration(npcDialogue), the readers and
		// Earl, is added by main.ts's wiring of the readers, S47Flow)
		US47Main* Mn = M;
		M->UI->NpcDialogue->OnCharacterStart = [Mn] { if (Mn->Audio) Mn->Audio->StopPhoneCalls(); if (Mn->CB) Mn->CB->Stop(); };
	}
	// audio.onPhoneCall: stopNarration() (the character voices among them) and cb.stop()
	if (M->Audio && !M->Audio->OnPhoneCall)
	{
		US47Main* Mn = M;
		M->Audio->OnPhoneCall = [Mn] { if (Mn->UI && Mn->UI->NpcDialogue) Mn->UI->NpcDialogue->Stop(); if (Mn->CB) Mn->CB->Stop(); };
	}
	TWeakObjectPtr<US47Chapter4> W = this;
	// main.ts motel.cb: (not in the middle of the time away, in the truck or out of it, nor as the truck
	// comes onto B at the turn-off: story/abduction.ts and story/truckTaken.ts are not in this version)
	Cb = [W](const TArray<FS47CBLine>& L, TFunction<void()> OnCbEnd)
	{
		if (!W.IsValid() || !W->M || !W->M->CB) return false;
		US47Main* Mn = W->M;
		return Mn->CB->Call(L, {}, [Mn](const FS47CBLine& Ln, double Duration)
		{
			if (Mn->Mode == US47Main::EMode::Play && !(Mn->Story && Mn->Story->IsCinematic()) && Mn->UI) Mn->UI->Toast(FString::Printf(TEXT("CB, %s: \"%s\""), *Ln.By, *Ln.Text), float(Duration));
		}, OnCbEnd) > 0;
	};
	// main.ts: world.onRoom6Loaded = ch4.bindRoom (the room is exported with the rest: it is loaded
	// from the start), onEnterRoom6, onLeaveRoom6
	BindRoom();
	if (M->World)
	{
		M->World->OnEnterRoom6 = [W] { if (W.IsValid()) W->EnteredRoom(); };
		M->World->OnLeaveRoom6 = [W] { if (W.IsValid()) W->LeftRoom(); };
		// main.ts truckChoice: ch3.truckTrip(place) ?? ch4.truckTrip(world.truckAt) ?? ch5 ... ?? the
		// player's own drive. (In front of what the world had: before chapter one nothing of chapter
		// four is active, so the prologue's refusal still comes first.)
		TFunction<US47World::FChoice(const FString&)> Prev = M->World->TruckChoice;
		M->World->TruckChoice = [W, Prev](const FString& Place) -> US47World::FChoice
		{
			if (W.IsValid() && W->M && W->M->World)
			{
				if (const TOptional<FTrip> T = W->TruckTrip(W->M->World->TruckAt)) return { T->Label, T->Use };
			}
			return Prev ? Prev(Place) : US47World::FChoice();
		};
	}
	// Prologue.jump('chapter4')
	Host->Jumps.Add(TEXT("chapter4"), [W] { if (W.IsValid()) W->JumpChapter4(); });
	// main.ts ch4.onEnd: she has told you where C goes, and it is five in the morning.
	OnEnd = [W]
	{
		if (!W.IsValid() || !W->Host) return;
		AS47Story* H = W->Host;
		H->ChapterBreak({ TEXT("ROOM 6 // P10 TO P12 RECORDED"),
			TEXT("She cut the cable in 1947 with her brother standing on C, and he was not there afterwards. C crosses the old road to Roswell. The truck is on its pad."),
			TEXT("CHAPTER FIVE"), TEXT("ALL NIGHT"), [H] { if (US47Chapter5* C5 = Cast<US47Chapter5>(H->Chapter(TEXT("ch5")))) C5->BeginChapter5(nullptr); else H->BeginChapter(TEXT("ch5")); } });
	};
	return this;
}

void US47Chapter4::BuildBook()
{
	FS47BookSpec O;
	O.Key = TEXT("ch4");
	O.Owns = [](const FString& Nd) { return Nd.StartsWith(TEXT("e11.")) || Nd.StartsWith(TEXT("e12.")) || Nd.StartsWith(TEXT("e13.")) || Nd == TEXT("r6.line") || Nd == TEXT("P10") || Nd == TEXT("P11") || Nd == TEXT("P12"); };
	O.Notes = NOTES();
	O.Rules = RULES();
	FS47BookQuestion P10;
	P10.Id = TEXT("P10"); P10.Stamp = TEXT("P10"); P10.Text = TEXT("Do tonight's prints carry the same mark as 1947?"); P10.Slots = 0; P10.X = 900; P10.Y = 14; P10.W = 266;
	P10.Hints = { TEXT("She asked for three things on her table."), TEXT("The two prints from B-12 and the original 1947 record, the one with her name on it."), TEXT("Put PHOTO 01, PHOTO 02 and E07 on her table, then tell her which source you took away.") };
	P10.Elsewhere = TEXT("Answered at her table in room 6: put the prints and the 1947 original in front of her.");
	P10.Open = [this] { return bStarted && S.bArrived; };
	P10.Done = [this] { return S.bP10 ? FString(SUPPORTED_P10) : FString(); };
	FS47BookQuestion P11;
	P11.Id = TEXT("P11"); P11.Stamp = TEXT("P11"); P11.Text = TEXT("Why did N. Vega rewrite the 1947 finding?"); P11.Slots = 2; P11.Hints = HINTS_11; P11.X = 610; P11.Y = 300; P11.W = 270;
	P11.Open = [this] { return bStarted && S.bP10; };
	P11.Done = [this] { return S.bP11 ? FString(SUPPORTED_P11) : FString(); };
	P11.Record = [this] { return RecordP11(); };
	FS47BookQuestion P12;
	P12.Id = TEXT("P12"); P12.Stamp = TEXT("P12"); P12.Text = TEXT("What does the return line show?"); P12.Slots = 1; P12.Hints = HINTS_12; P12.X = 900; P12.Y = 300; P12.W = 266;
	P12.Open = [this] { return bStarted && (S.bLine || S.bP12); };
	P12.Done = [this] { return S.bP12 ? FString(SUPPORTED_P12) : FString(); };
	P12.Record = [this] { return RecordP12(); };
	O.Questions = { P10, P11, P12 };
	O.Links = [this]() -> TArray<FString>& { if (!S.Links) S.Links.Emplace(); return *S.Links; }; // (this.s.links ??= [])
	O.Wrong = [this](const FString& Q, const FS47BookRule&) -> TOptional<FS47Result>
	{
		if (Q == TEXT("P11")) S.Wrong11++; else if (Q == TEXT("P12")) S.Wrong12++;
		return {};
	};
	O.Save = [this] { Save(); };
	O.Cards = [this] { return BoardCards(); };
	// P10 is made in the talk: the board draws what it rests on
	O.Auto = [this]() -> TArray<TPair<FString, FString>>
	{
		if (!S.bP10) return {};
		return { { TEXT("P10"), TEXT("frame02") }, { TEXT("P10"), TEXT("e07.mark") } };
	};
	Book = MakeUnique<FS47RuleBook>(MoveTemp(O));
}

void US47Chapter4::BeginDestroy()
{
	if (M && Book) M->Books.Remove(Book.Get());
	Super::BeginDestroy();
}

// Her papers on the board, once she has put them on the table, and the return line once it has
// been heard. Nothing of hers lies there before she gives it.
TArray<FS47BoardCard> US47Chapter4::BoardCards() const
{
	if (!bStarted) return {};
	auto Card = [](const TCHAR* Id, const TCHAR* Doc, const TCHAR* Stamp, const TCHAR* Title, bool bHave, const FString& Full, double X, double Y, double W, TArray<FS47BoardLine> L)
	{
		FS47BoardCard C;
		C.Id = Id; C.Doc = Doc; C.Stamp = Stamp; C.Title = Title; C.bHave = bHave; C.Full = Full;
		C.X = X; C.Y = Y; C.W = W;
		C.Lines = MoveTemp(L);
		return C;
	};
	auto Ln = [](const TCHAR* Id, const TCHAR* Text) { return FS47BoardLine{ Id, Text, FString() }; };
	TArray<FS47BoardCard> Cards = {
		Card(TEXT("e12"), TEXT("e12"), TEXT("E12"), TEXT("HER CORRECTION"), S.Topics.bRecord, CORRECTION, 14, 14, 300, {
			Ln(TEXT("e12.why"), TEXT("I removed the closing reference from the service copy because I believed the complete arrangement would be repeated if the original finding remained on the record.")),
			Ln(TEXT("e12.dev"), TEXT("I attributed the additional mark to development.")),
			Ln(TEXT("e12.cut"), TEXT("The interruption was deliberate. Tomás Vega was present in the active reference when I broke the connection.")) }),
		Card(TEXT("e13"), TEXT("e13"), TEXT("E13"), TEXT("A LETTER FROM T. VEGA"), S.Topics.bTomas, LETTER, 330, 14, 266, {
			Ln(TEXT("e13.key"), TEXT("I left the spare key in the mug because you said I would lose it in the sand.")),
			Ln(TEXT("e13.twice"), TEXT("You were right about the reference moving. I checked it twice before I wrote it down.")) }),
		Card(TEXT("e11"), TEXT("e11"), TEXT("E11"), TEXT("HIS LAST FIELD CARD"), S.Topics.bCable, CARD, 330, 330, 266, {
			Ln(TEXT("e11.mask"), TEXT("Masking the closing sight line prevents an organised return.")),
			Ln(TEXT("e11.fresh"), TEXT("Do not connect a fresh observer to a run merely to finish an old one.")) }),
	};
	FS47BoardCard Line;
	Line.Id = TEXT("r6.line"); Line.Stamp = TEXT("THE LINE"); Line.Title = TEXT("ROOM 6, TONIGHT"); Line.Kind = TEXT("thing"); Line.bHave = S.bLine;
	Line.X = 610; Line.Y = 14; Line.W = 270;
	Line.Text = TEXT("She asked it: \"Tomás. Where did you leave the spare key?\" The line said: \"Reference west. No. East. Hold the last reading.\"");
	Cards.Add(Line);
	return Cards.FilterByPredicate([](const FS47BoardCard& C) { return C.bHave; });
}

// ---------- lifecycle ----------

void US47Chapter4::Begin(const S47J::FObj& Saved)
{
	Reset();
	bActive = true; bStarted = true;
	S = Saved.IsValid() ? FS47Ch4State::Load(Saved) : FS47Ch4State();
	// the field roll is developed and filed; the camera has nothing left to do tonight
	if (US47Chapter1* C1 = Host ? Cast<US47Chapter1>(Host->Chapter(TEXT("ch1"))) : nullptr)
	{
		FS47FieldShots F;
		F.FrameNo = [] { return FString(TEXT("FRAME 05")); };
		F.Check = [] { FS47ShotCheck C; C.Text = TEXT("THE FIELD ROLL IS DEVELOPED AND FILED"); C.bOk = false; return C; };
		F.Expose = [] {};
		F.Rejected = [] {};
		C1->Field = F;
	}
	BindCourt();
	ApplyWorld();
	if (!Saved.IsValid())
	{
		Note(FString::Printf(TEXT("%s. N. Vega is in room 6 at Sierra Motor Court, 650 metres down the highway south of SARO, on the right. She wants the original record and the prints. The truck is on its pad; on foot it is the fire exit at the west end of the south corridor, and the shoulder."), *Time()));
		Toast(TEXT("Room 6 is at Sierra Motor Court, down the highway south. Take the truck, or walk the shoulder from the fire exit."), 4.6f);
	}
	Save();
	static bool bDriven = false;
	FString Stop;
	if (!bDriven && FParse::Value(FCommandLine::Get(), TEXT("S47Ch4Drive="), Stop)) { bDriven = true; TestDrive(Stop); }
}

void US47Chapter4::Reset()
{
	bActive = false; bStarted = false; AfterText.Reset();
	S = FS47Ch4State(); Lines.Reset(); Mode = TEXT("menu"); Reply.Reset(); Panel.Reset();
	if (Court)
	{
		Court->Brick().SetVisible(false);
		Court->SetRoom6Light(true);
	}
	SetLeaf(0);
	if (Room)
	{
		for (const TCHAR* K : { TEXT("fieldCard"), TEXT("letter"), TEXT("correction") }) Room->Obj(K).SetVisible(false);
		Room->SetLamp(true);
	}
}

S47J::FObj US47Chapter4::SaveState() const
{
	return bStarted ? S.Save() : nullptr;
}

// Puts the world in the state of this.s (after begin, and when room 6 has loaded).
void US47Chapter4::ApplyWorld()
{
	if (S.bExitOpen && M && M->Web) M->Web->SetDoorById(TEXT("exit"), true, true); // a case from before the doors were saved
	if (Court) Court->Brick().SetVisible(ExitOpen());
	SetLeaf(S.Stage == TEXT("complete") ? 0 : 0.35);   // her door stands ajar while she waits
	if (Room)
	{
		Room->Obj(TEXT("correction")).SetVisible(S.Topics.bRecord);
		Room->Obj(TEXT("letter")).SetVisible(S.Topics.bTomas);
		Room->Obj(TEXT("fieldCard")).SetVisible(S.Topics.bCable);
	}
	if (S.Topics.bRecord) File(DocCorrection());
	if (S.Topics.bTomas) File(DocLetter());
	if (S.Topics.bCable) File(DocCard());
	if (S.bP10) File(DocFinding(TEXT("p10")));
	if (S.bP11) File(DocFinding(TEXT("p11")));
	if (S.bP12) File(DocFinding(TEXT("p12")));
}

void US47Chapter4::Save() { if (OnSave) OnSave(); }

FString US47Chapter4::Time() const { return S47Text::ClockText(Host ? Host->Clock() : 0, false); }

bool US47Chapter4::File(const FS47Doc& Doc)
{
	// file(doc): replaces the one with the same id (false), or adds it (true)
	if (!Host) { if (TestDocs.Contains(Doc.Id)) return false; TestDocs.Add(Doc.Id); return true; }
	const bool bNew = !Host->Docs().ContainsByPredicate([&](const FS47Doc& D) { return D.Id == Doc.Id; });
	Host->AddDoc(Doc);
	return bNew;
}

void US47Chapter4::Note(const FString& Text) { if (Host) Host->Note(Text); else if (!TestNotes.Contains(Text)) TestNotes.Add(Text); }
void US47Chapter4::Toast(const FString& Text, float Secs) const { if (M && M->UI) M->UI->Toast(Text, Secs); }
void US47Chapter4::Milestone() const { if (Host && Host->OnCheckpoint) Host->OnCheckpoint(TEXT("chapter4")); }
FString US47Chapter4::Area() const { return M && M->World ? M->World->Area : FString(TEXT("saro")); }
bool US47Chapter4::InRoom() const { return Area() == TEXT("room6"); }
bool US47Chapter4::ExitOpen() const { return M && M->Web ? M->Web->DoorStates().FindRef(TEXT("exit")) : S.bExitOpen; }

const US47Chapter1* US47Chapter4::Ch1() const { return Host ? Cast<US47Chapter1>(Host->Chapter(TEXT("ch1"))) : nullptr; }
const US47Chapter2* US47Chapter4::Ch2() const { return Host ? Cast<US47Chapter2>(Host->Chapter(TEXT("ch2"))) : nullptr; }
S47J::FObj US47Chapter4::Ch3() const
{
	// chapter three's state, read as it saves it: its class's SaveState() once it is in this version
	// (and has begun), else the case chapter four began with
	if (const US47Chapter* C3 = Host ? Host->Chapter(TEXT("ch3")) : nullptr; C3 && Host->Begun.Contains(TEXT("ch3"))) return C3->SaveState();
	return S47J::Obj(BeganCase, TEXT("ch3"));
}

// ---------- objective and notebook ----------

FString US47Chapter4::Objective() const
{
	const bool bRoom = InRoom();
	if (S.Stage == TEXT("to-motel"))
	{
		if (!ExitOpen() && Area() == TEXT("saro")) return TEXT("SIERRA MOTOR COURT // ROOM 6, DOWN THE HIGHWAY SOUTH. THE TRUCK, OR THE FIRE EXIT AT THE END OF THE SOUTH CORRIDOR");
		return TEXT("SIERRA MOTOR COURT // DOWN THE HIGHWAY TO ROOM 6, THE LIT SIGN ON THE RIGHT");
	}
	if (S.Stage == TEXT("room"))
	{
		if (!bRoom) return TEXT("SIERRA MOTOR COURT // N. VEGA IS STILL WAITING IN ROOM 6");
		if (!S.bGreeted) return TEXT("ROOM 6 // N. VEGA IS WAITING AT THE TABLE");
		if (!S.bP10) return TEXT("ROOM 6 // PUT THE PRINTS AND THE 1947 RECORD ON THE TABLE (P10)");
		if (!S.bP11 || !S.bP12)
		{
			const int32 Left = int32(!S.Topics.bRecord) + int32(!S.Topics.bTomas) + int32(!S.Topics.bCable);
			if (Left) return FString::Printf(TEXT("ROOM 6 // ASK HER ABOUT THE RECORD, T. VEGA AND THE CABLE (%d OF 3)%s"), 3 - Left, S.bP12 ? TEXT("") : TEXT(". AND THE PHONE"));
			if (!S.bP11) return TEXT("ROOM 6 // ON THE CASE BOARD: WHY SHE REWROTE THE FINDING (P11)");
			return S.bLine ? TEXT("ROOM 6 // ON THE CASE BOARD: WHAT THE RETURN LINE SHOWS (P12)") : TEXT("ROOM 6 // ASK ABOUT THE PHONE ON THE NIGHTSTAND (P12)");
		}
		return TEXT("ROOM 6 // ASK HER WHERE C WENT");
	}
	if (S.Stage == TEXT("leave")) return bRoom ? TEXT("ROOM 6 // THE ROAD SOUTH IS NEXT. LEAVE ROOM 6") : TEXT("SIERRA MOTOR COURT // THE ROAD SOUTH IS NEXT");
	if (S.Stage == TEXT("complete")) return TEXT("ALL NIGHT // NEXT PART NOT YET PLAYABLE");
	return FString();
}

TArray<FS47Task> US47Chapter4::Tasks() const
{
	TArray<FS47Task> T = { { TEXT("Go to room 6 at Sierra Motor Court"), S.bArrived } };
	if (S.bArrived) T.Append({
		{ TEXT("Put the prints and the 1947 record on the table (P10)"), S.bP10 },
		{ TEXT("Ask about the amended record"), S.Topics.bRecord },
		{ TEXT("Ask who T. Vega was"), S.Topics.bTomas },
		{ TEXT("Show her the cable break"), S.Topics.bCable },
		{ TEXT("Work out why she rewrote the finding, on the case board (P11)"), S.bP11 },
		{ TEXT("Ask about the phone on the nightstand (P12)"), S.bP12 },
	});
	if (S.bP10 && S.bP11 && S.bP12) T.Add({ TEXT("Ask where C went"), S.bTold });
	return T;
}

// The door of room 6 on the motel's front (MotelFront's hinged leaf): 0 shut, 1 open.
void US47Chapter4::SetLeaf(double K)
{
	if (!Court) return;
	FS47Obj Leaf = Court->Obj(TEXT("room6DoorLeaf"));
	if (Leaf) Leaf.SetRotY(K * UE_DOUBLE_PI / 2);
}

// ---------- the truck ----------

TOptional<US47Chapter4::FTrip> US47Chapter4::TruckTrip(const FString& At) const
{
	if (!bActive || S.Stage != TEXT("to-motel") || At == TEXT("motel")) return {};
	TWeakObjectPtr<US47World> Wd = M ? M->World.Get() : nullptr;
	return FTrip{ TEXT("Drive to Sierra Motor Court"), [Wd] { if (Wd.IsValid()) Wd->DriveToMotel([] {}); } };
}

// ---------- the fire exit ----------
// The doors are free (world/Doors.ts). Opened in this chapter, a brick props the fire exit.

void US47Chapter4::DoorChanged(const FString& Id, bool bOpen)
{
	if (Id != TEXT("exit")) return;
	if (Court) Court->Brick().SetVisible(bOpen);
	if (!bActive || !bOpen || S.bExitOpen) return;
	S.bExitOpen = true;
	Toast(TEXT("The alarm on the push bar was disconnected years ago. A brick on the step holds the door."), 4.2f);
	Save();
}

// The motel's front: room 6 (and, with MotelFront.ts, the office).
void US47Chapter4::BindCourt()
{
	if (bCourtBound || !M || !M->Inter || !Court) return;
	bCourtBound = true;
	US47Inter* Inter = M->Inter;
	TWeakObjectPtr<US47Chapter4> W = this;
	auto On = [W] { return W.IsValid() && W->Area() == TEXT("saro"); };
	if (const FS47Obj Px = Court->Proxy(TEXT("room6Door")))
	{
		FS47Interactable It;
		It.Id = TEXT("room6Door"); It.Object = Px; It.Range = 2.6;
		It.Label = [W, On] { return On() ? FString(W->bActive && W->S.Stage != TEXT("complete") ? TEXT("Room 6") : TEXT("Room 6, Sierra Motor Court")) : FString(); };
		It.Use = [W]
		{
			if (!W.IsValid()) return;
			if (!W->bActive) { W->Toast(TEXT("Room 6. There is a light behind the curtains."), 3); return; }
			if (W->S.Stage == TEXT("complete")) { W->Toast(TEXT("The light is still on. She said what she had to say."), 3.2f); return; }
			W->SetLeaf(1);
			if (W->M->World) W->M->World->GoRoom6();
		};
		Inter->Add(It);
	}
	auto Extra = [&](const TCHAR* Id, const FString& Label, const FString& Text)
	{
		const FS47Obj Px = Court->Proxy(Id);
		if (!Px) return;
		FS47Interactable It;
		It.Id = FString(TEXT("court:")) + Id; It.Object = Px;
		It.Label = [On, Label] { return On() ? Label : FString(); };
		It.Use = [W, Text] { if (W.IsValid()) W->Toast(Text, 3.6f); };
		Inter->Add(It);
	};
	Extra(TEXT("officeDoor"), TEXT("Motel office"), TEXT("The office of Sierra Motor Court. The night bell is on the counter."));
	Extra(TEXT("register"), TEXT("Guest register"), TEXT("A register in her handwriting. Room 6: N. Vega, every night since the spring."));
	Extra(TEXT("keyBoard"), TEXT("Key board"), TEXT("Ten hooks with brass tags. The hook for 6 is empty."));
	// the crank telephone: while it rings or is off the hook it is story/officePhone.ts's (main.ts)
	if (const FS47Obj Px = Court->Proxy(TEXT("officePhone")))
	{
		FS47Interactable It;
		It.Id = TEXT("court:officePhone"); It.Object = Px;
		It.Label = [W, On]
		{
			if (!On()) return FString();
			const FString L = W->OfficePhone && W->OfficePhone->Label ? W->OfficePhone->Label() : FString();
			return L.IsEmpty() ? FString(TEXT("Field telephone")) : L;
		};
		It.Use = [W]
		{
			if (!W.IsValid()) return;
			if (W->OfficePhone && W->OfficePhone->Use && W->OfficePhone->Use()) return;
			W->Toast(TEXT("An oak wall telephone with a crank. The cable runs out through the back wall and on to STATION 01."), 3.6f);
		};
		Inter->Add(It);
	}
	// (the envelope on the counter is the field book's, any time of the night: story/fieldBook.ts)
	if (const FS47Obj Px = Court->Proxy(TEXT("message")))
	{
		FS47Interactable It;
		It.Id = TEXT("court:message"); It.Object = Px;
		It.Label = [On] { return On() ? FString(TEXT("A note on the counter")) : FString(); };
		It.Use = [W]
		{
			if (!W.IsValid()) return;
			W->Toast(TEXT("In pencil, on the back of a bill: \"Room 6. The door is open. N.\""), 4);
			const bool bHave = W->Host && W->Host->Notes().ContainsByPredicate([](const FString& X) { return X.Contains(TEXT("the back of a bill")); });
			if (W->bActive && !bHave) W->Note(TEXT("A note on the office counter, in pencil on the back of a bill: \"Room 6. The door is open. N.\""));
		};
		Inter->Add(It);
	}
	Extra(TEXT("otherDoors"), TEXT("Motel room"), TEXT("Dark. Nobody else is staying tonight."));
	Extra(TEXT("iceMachine"), TEXT("Ice machine"), TEXT("It hums. Nobody has opened it in a while."));
	Extra(TEXT("car"), TEXT("Parked car"), TEXT("A dusty sedan with New Mexico plates. Her car, by the look of the survey stakes on the back seat."));
	Extra(TEXT("pool"), TEXT("Pool"), TEXT("Empty, with a fence round it and leaves in the deep end."));
}

// ---------- room 6 ----------
// Room 6 loads the first time the player goes in; its interactables are added then. (Here it is
// loaded from the start: Register binds it once.)

void US47Chapter4::BindRoom()
{
	if (!bRoomBound && M && M->Inter && Room)
	{
		bRoomBound = true;
		US47Inter* Inter = M->Inter;
		TWeakObjectPtr<US47Chapter4> W = this;
		auto On = [W] { return W.IsValid() && W->bActive && W->InRoom(); };
		auto Add = [&](const TCHAR* Id, const TCHAR* Key, TFunction<FString()> Label, TFunction<void()> Use, double Range = 2.4)
		{
			const FS47Obj Px = Room->Proxy(Key);
			if (!Px) return;
			FS47Interactable It;
			It.Id = FString(TEXT("r6:")) + Id; It.Object = Px; It.Range = Range;
			It.Label = [On, Label] { return On() ? Label() : FString(); };
			It.Use = MoveTemp(Use);
			Inter->Add(It);
		};
		auto Str = [](const TCHAR* S) { return [S] { return FString(S); }; };
		Add(TEXT("nora"), TEXT("nora"), Str(TEXT("Talk to N. Vega")), [W] { if (W.IsValid()) W->OpenTalk(); }, 2.8);
		Add(TEXT("table"), TEXT("table"), Str(TEXT("The table")), [W] { if (W.IsValid()) W->OpenTalk(); }, 2.4);
		Add(TEXT("card"), TEXT("fieldCard"), [W] { return W->S.Topics.bCable ? FString(TEXT("E11 / T. Vega's field card")) : FString(); }, [W] { if (W.IsValid()) W->ReadPaper(TEXT("card")); });
		Add(TEXT("letter"), TEXT("letter"), [W] { return W->S.Topics.bTomas ? FString(TEXT("E13 / The letter")) : FString(); }, [W] { if (W.IsValid()) W->ReadPaper(TEXT("letter")); });
		Add(TEXT("correction"), TEXT("correction"), [W] { return W->S.Topics.bRecord ? FString(TEXT("E12 / Her correction")) : FString(); }, [W] { if (W.IsValid()) W->ReadPaper(TEXT("correction")); });
		Add(TEXT("photo"), TEXT("photo"), Str(TEXT("Framed snapshot")), [W] { if (W.IsValid()) W->Look(TEXT("photo")); });
		Add(TEXT("window"), TEXT("window"), Str(TEXT("Window")), [W] { if (W.IsValid()) W->Look(TEXT("window")); });
		Add(TEXT("phone"), TEXT("phone"), Str(TEXT("Phone on the nightstand")), [W] { if (W.IsValid()) W->UsePhone(); });
		Add(TEXT("tv"), TEXT("tv"), [W] { return FString(W->Room && W->Room->bTvOn ? TEXT("Turn the television off") : TEXT("Turn the television on")); }, [W] { if (W.IsValid()) W->UseTv(); });
		Add(TEXT("bed"), TEXT("bed"), Str(TEXT("Bed")), [W] { if (W.IsValid()) W->Toast(TEXT("Made, and not slept in."), 2.6f); });
		Add(TEXT("lamp"), TEXT("lamp"), Str(TEXT("Table lamp")), [W]
		{
			if (!W.IsValid() || !W->Room) return;
			W->Room->SetLamp(!W->Room->bLampOn);
			if (US47Audio* A = W->M->Audio) A->Sfx(A->Or(TEXT("small_switch"), TEXT("switch")), Snd(0.3f, 1.4f));
		});
		Add(TEXT("bathroom"), TEXT("bathroom"), Str(TEXT("Bathroom door")), [W] { if (W.IsValid()) W->Toast(TEXT("Shut. A tap drips behind it."), 2.6f); });
		Add(TEXT("walkie"), TEXT("walkie"), [W] { return W->WalkieLabel(); }, [W] { if (W.IsValid()) W->UseWalkie(); });
		Add(TEXT("door"), TEXT("door"), [W] { return FString(W->S.Stage == TEXT("leave") ? TEXT("Leave room 6") : TEXT("Door to the walk")); }, [W] { if (W.IsValid()) W->UseDoor(); }, 2.6);
	}
	if (bActive) ApplyWorld();
}

void US47Chapter4::UseTv()
{
	if (!Room) return;
	bTvTouched = true;
	Room->SetTv(!Room->bTvOn);
	if (US47Audio* A = M ? M->Audio.Get() : nullptr) A->Sfx(A->Or(TEXT("small_switch"), TEXT("switch")), Snd(0.35f, 0.8f));
	TvSound();
	if (Room->bTvOn && !bTvSeen) { bTvSeen = true; Toast(Room->HasProgramme() ? TEXT("A strange late-night broadcast. Visitors from the stars, playing to an empty room.") : TEXT("Snow on every channel. The stations signed off at one."), 3.2f); }
	else if (!Room->bTvOn) NoraToast(TEXT("N. VEGA: \"Leave it. The quiet is worse.\""), 3);
}

void US47Chapter4::TvSound()
{
	US47Audio* A = M ? M->Audio.Get() : nullptr;
	if (!A) return;
	const FS47Obj Tv = Room ? Room->Obj(TEXT("tv")) : FS47Obj();
	// (with the local broadcast staged, the set plays its sound instead: US47Room6::Broadcast)
	A->TvHiss(Room && Tv && Room->bTvOn && InRoom() && !Room->HasProgrammeSound() ? TOptional<FVector>(Tv.WorldPosition()) : TOptional<FVector>());
}

// Called by World when the player comes in through the door.
void US47Chapter4::EnteredRoom()
{
	if (!bActive) return;
	if (Room && !bTvTouched) Room->SetTv(true);
	TvSound();
	if (!S.bArrived)
	{
		S.bArrived = true;
		if (S.Stage == TEXT("to-motel")) S.Stage = TEXT("room");
		Note(FString::Printf(TEXT("%s. Room 6. N. Vega in the chair by the window, a shoebox of papers on the table. She has been waiting a long time; she does not say so."), *Time()));
		Toast(TEXT("N. Vega is sitting by the window."), 3.4f);
		Milestone();
	}
}

// ---------- Tomás' field radio (KAPITLER.md, Room 6; core/walkie.ts) ----------
// It plays the road an hour before it happens: an engine dying, the hiss gone, relay clicks in four
// and seven, someone breathing. Nobody says so. It has no battery.

FString US47Chapter4::WalkieLabel() const
{
	if (WalkieState == TEXT("playing") || WalkieState == TEXT("sending")) return FString();
	if (WalkieState == TEXT("on")) return bWalkiePressed ? TEXT("Turn the field radio off") : TEXT("Press to talk");
	return TEXT("Field radio, 1947");
}

void US47Chapter4::UseWalkie()
{
	US47Audio* A = M ? M->Audio.Get() : nullptr;
	const FS47Obj O = Room ? Room->Obj(TEXT("walkie")) : FS47Obj();
	if (!O || !A || !A->Ctx.IsValid()) { Toast(TEXT("The switch is stiff. Nothing."), 2.4f); return; }
	if (WalkieState == TEXT("on") && !bWalkiePressed) { WalkiePress(); return; }
	if (WalkieState == TEXT("on")) { WalkieOff(); A->Sfx(TEXT("click"), Snd(0.35f, 0.8f, O.WorldPosition())); return; }
	FVector At = O.WorldPosition();
	At.Y = 0.95;
	const TSharedPtr<FS47AudioNode> Dest = A->Spatial(At, 0.7f);
	const TSharedPtr<FS47AudioBuffer> Noise = A->NoiseBuffer();
	if (!Dest.IsValid() || !Noise.IsValid()) return;
	A->Sfx(TEXT("click"), Snd(0.45f, 0.7f, At));
	if (Walkie) Walkie->Stop();
	Walkie = MakeShared<FS47WalkieSound>(A->Ctx.ToSharedRef(), Dest, Noise);
	WalkieState = TEXT("playing"); bWalkiePressed = false;
	if (!bWalkieHeard) Toast(TEXT("The switch is stiff. There is no battery in it."), 3);
	const double Secs = Walkie->Start();
	TWeakObjectPtr<US47Chapter4> W = this;
	if (Host) Host->After(Secs, [W]
	{
		if (!W.IsValid() || W->WalkieState != TEXT("playing")) return;
		W->WalkieState = TEXT("on");
		if (!W->bWalkieHeard)
		{
			W->bWalkieHeard = true;
			W->NoraToast(TEXT("N. VEGA: \"I took the battery out in 1947.\""), 3.4f);
			W->Note(TEXT("Tomás' field radio, on her table. No battery. An engine far off, coughing, then stopping. Silence. Clicks, four and seven. Someone breathing."));
		}
	});
}

void US47Chapter4::WalkiePress()
{
	if (!Walkie) return;
	WalkieState = TEXT("sending"); bWalkiePressed = true;
	if (US47Audio* A = M ? M->Audio.Get() : nullptr) A->Sfx(TEXT("click"), Snd(0.5f, 1.2f));
	const double Secs = Walkie->PressToTalk();
	TWeakObjectPtr<US47Chapter4> W = this;
	if (Host) Host->After(Secs, [W]
	{
		if (!W.IsValid() || W->WalkieState != TEXT("sending")) return;
		W->WalkieState = TEXT("on");
		if (!W->bWalkieSent)
		{
			W->bWalkieSent = true;
			W->NoraToast(TEXT("N. VEGA: \"He pressed it too. Twice.\""), 3.2f);
			W->Note(TEXT("I pressed to talk once. Four clicks and seven came back."));
		}
	});
}

void US47Chapter4::WalkieOff()
{
	if (Walkie) Walkie->Stop();
	WalkieState = TEXT("off"); bWalkiePressed = false;
}

// Called by World when the player has stepped out of room 6.
void US47Chapter4::LeftRoom()
{
	if (US47Audio* A = M ? M->Audio.Get() : nullptr) A->TvHiss({});
	WalkieOff();
	if (!bActive) return;
	SetLeaf(S.Stage == TEXT("leave") ? 0 : 0.35);
	if (S.Stage == TEXT("leave")) Finish();
}

void US47Chapter4::UseDoor()
{
	if (S.Stage != TEXT("leave") && S.bArrived && !S.bTold) NoraToast(TEXT("N. VEGA: \"Go if you need to. I will be here.\""), 3.2f);
	if (M && M->World) M->World->LeaveRoom6();
}

void US47Chapter4::Look(const FString& What)
{
	bool& bSeen = What == TEXT("photo") ? S.Seen.bPhoto : S.Seen.bWindow;
	const bool bFirst = !bSeen;
	bSeen = true;
	if (What == TEXT("photo"))
	{
		Toast(TEXT("Two people at a transit on a concrete pier, squinting into the sun. On the back, in pencil: N. and T., STATION 01, June 1947."), 5);
		if (bFirst) Note(TEXT("A framed snapshot on the dresser: N. and T. Vega at the transit at STATION 01, June 1947."));
	}
	else Toast(TEXT("Through the gap in the curtains: the Sierra sign, red and cyan, and far up the road the lights of SARO."), 4);
	if (bFirst) Save();
}

void US47Chapter4::ReadPaper(const FString& Which)
{
	const FS47Doc Doc = Which == TEXT("card") ? DocCard() : Which == TEXT("letter") ? DocLetter() : DocCorrection();
	bool& bRead = Which == TEXT("card") ? S.Read.bCard : Which == TEXT("letter") ? S.Read.bLetter : S.Read.bCorrection;
	if (!bRead) { bRead = true; Save(); }
	if (M && M->UI) M->UI->Document(Doc, [] {});
}

void US47Chapter4::UsePhone()
{
	if (!S.bP10) { Toast(TEXT("A beige phone. The cord runs to a wall jack marked OFFICE."), 3.2f); return; }
	if (S.bP12) { Toast(TEXT("The handset is back on the cradle. The carrier is still there, if you listen for it."), 3.2f); return; }
	OpenTalk();
	Choose(TEXT("line"));
	// (as in the web version, the panel opened above is not drawn again for the line: it shows the
	// lines from before until the next click)
}

// ---------- the conversation ----------

void US47Chapter4::NoraToast(const FString& Text, float Seconds)
{
	Toast(Text, Seconds);
	if (M && M->UI && M->UI->NpcDialogue) M->UI->NpcDialogue->SpeakNora(Text);
}

void US47Chapter4::OpenTalk()
{
	if (WalkieState != TEXT("off")) WalkieOff();   // it keeps quiet while she talks
	if (!S.bGreeted)
	{
		S.bGreeted = true;
		Mode = TEXT("table");
		Lines = {
			Nv(TEXT("So you came down the road.")),
			Nv(TEXT("Do not sit yet. Put them on the table first. The two prints from B-12, and the record from 1947. The one with my name on it.")),
		};
		Save();
	}
	else if (S.bP10 && !S.Mark.Get(false) && Taken && Taken() && (Mode == TEXT("menu") || Mode == TEXT("mark")))
	{
		// taken since, or the chair never sat in: she sees it as you come to the table
		MarkTalk(true);
	}
	else if (Mode == TEXT("menu") && !Lines.Num())
	{
		Lines = { Aside(S.bTold ? TEXT("She is looking out at the road.") : TEXT("She waits for you to go on.")) };
	}
	Reply.Reset();
	if (!M || !M->UI) return;
	TWeakObjectPtr<US47Chapter4> W = this;
	S47Panels::FTalk O;
	O.Title = TEXT("ROOM 6 / N. VEGA");
	O.Character = TEXT("bacall");
	O.View = [W] { return W.IsValid() ? W->View() : S47Panels::FTalkView(); };
	O.OnChip = [W](const FString& Id) { if (W.IsValid()) W->ToggleChip(Id); };
	O.OnTable = [W] { if (W.IsValid()) W->ShowEvidence(); };
	O.OnChoice = [W](const FString& Id) { if (W.IsValid()) W->Choose(Id); };
	O.OnRead = [W](const FString& Id) { if (W.IsValid()) W->ReadPaper(Id); };
	O.OnClose = [W] { if (!W.IsValid()) return; if (W->M->Audio) W->M->Audio->StopPhoneCalls(); W->Panel.Reset(); };
	Panel = S47Panels::Talk(M->UI, O);
}

bool US47Chapter4::Have(const FString& Id) const
{
	// ch1.s.filed, ch2.s.read.original / amended, ch3.s.dev >= 3 (the classes where they are; chapter
	// three, and any of them without a host, from the case it began with)
	const US47Chapter1* C1 = Ch1();
	const US47Chapter2* C2 = Ch2();
	if (Id == TEXT("frame01") || Id == TEXT("frame02")) return C1 ? C1->S.bFiled : S47J::Bool(S47J::Obj(BeganCase, TEXT("s")), TEXT("filed"));
	if (Id == TEXT("e07")) return C2 ? C2->S.Read.bOriginal : S47J::Bool(S47J::Obj(S47J::Obj(BeganCase, TEXT("ch2")), TEXT("read")), TEXT("original"));
	if (Id == TEXT("e06")) return C2 ? C2->S.Read.bAmended : S47J::Bool(S47J::Obj(S47J::Obj(BeganCase, TEXT("ch2")), TEXT("read")), TEXT("amended"));
	if (Id == TEXT("frame03") || Id == TEXT("frame04")) return S47J::Num(Ch3(), TEXT("dev")) >= 3;
	return false;
}

FString US47Chapter4::Image(const FString& Id) const
{
	// { frame01: ch1.s.f1?.url, frame02: ch1.s.f2?.url, frame03: ch3.s.f3?.url, frame04: ch3.s.f4?.url }
	if (Id == TEXT("frame01") || Id == TEXT("frame02"))
	{
		if (const US47Chapter1* C1 = Ch1())
		{
			const TOptional<FS47Photo>& P = Id == TEXT("frame01") ? C1->S.F1 : C1->S.F2;
			return P ? P->Url : FString();
		}
		return S47J::Str(S47J::Obj(S47J::Obj(BeganCase, TEXT("s")), Id == TEXT("frame01") ? TEXT("f1") : TEXT("f2")), TEXT("url"));
	}
	if (Id == TEXT("frame03")) return S47J::Str(S47J::Obj(Ch3(), TEXT("f3")), TEXT("url"));
	if (Id == TEXT("frame04")) return S47J::Str(S47J::Obj(Ch3(), TEXT("f4")), TEXT("url"));
	return FString();
}

S47Panels::FTalkView US47Chapter4::View() const
{
	S47Panels::FTalkView V;
	V.Status = Objective(); V.Lines = Lines; V.Reply = Reply;
	if (!AfterText.IsEmpty()) V.After = Aside(AfterText);
	if (S.Topics.bRecord) V.Given.Add({ TEXT("correction"), TEXT("E12 / Her correction") });
	if (S.Topics.bTomas) V.Given.Add({ TEXT("letter"), TEXT("E13 / The letter") });
	if (S.Topics.bCable) V.Given.Add({ TEXT("card"), TEXT("E11 / The field card") });
	if (Mode == TEXT("table"))
	{
		S47Panels::FTalkTable T;
		T.Prompt = TEXT("Put the evidence on the table.");
		for (const FChipDef& C : CHIPS) T.Chips.Add({ C.Id, C.Label, Image(C.Id), Have(C.Id), S.Placed.Contains(C.Id) });
		T.Submit = TEXT("SHOW HER");
		V.Table = T;
	}
	else if (Mode == TEXT("control"))
	{
		V.Question = TEXT("Which source did you take away before the control exposure?");
		V.Choices = {
			{ TEXT("lamp"), TEXT("The work lamp. I shielded it, and the vane motor was isolated.") },
			{ TEXT("none"), TEXT("None. I drove the vane to 042 and left the lamp on.") },
			{ TEXT("receiver"), TEXT("The receiver bank. I switched it off.") },
		};
	}
	else if (Mode == TEXT("mark"))
	{
		V.Choices = { { TEXT("sit"), TEXT("Sit down") } };
	}
	else if (Mode == TEXT("tone"))
	{
		V.Question = TEXT("How do you ask her?");
		V.Choices = {
			{ TEXT("mild"), TEXT("\"Why did you change it?\"") },
			{ TEXT("critical"), TEXT("\"You falsified the record.\"") },
		};
	}
	else if (Mode == TEXT("menu"))
	{
		if (!S.Topics.bRecord) V.Choices.Add({ TEXT("record"), TEXT("Ask about the amended record") });
		if (!S.Topics.bTomas) V.Choices.Add({ TEXT("tomas"), TEXT("Ask who T. Vega was") });
		if (!S.Topics.bCable) V.Choices.Add({ TEXT("cable"), TEXT("Show her PHOTO 04, the cable break"), !Have(TEXT("frame04")) });
		if (!S.bLine) V.Choices.Add({ TEXT("line"), TEXT("Ask about the phone on the nightstand") });
		{
			// P11 once all three parts are on the table, P12 once the line has been heard
			TArray<FString> Open;
			if (S.Topics.bRecord && S.Topics.bTomas && S.Topics.bCable && !S.bP11) Open.Add(TEXT("P11"));
			if (S.bLine && !S.bP12) Open.Add(TEXT("P12"));
			if (Open.Num()) V.Choices.Add({ TEXT("board"), FString::Printf(TEXT("Lay it out on the case board (%s)"), *FString::Join(Open, TEXT(", "))) });
		}
		if (S.bP10 && S.bP11 && S.bP12 && !S.bTold) V.Choices.Add({ TEXT("end"), TEXT("Ask where C went") });
		V.Choices.Add({ TEXT("bye"), S.bTold ? TEXT("Leave her be") : TEXT("Step back from the table") });
	}
	return V;
}

void US47Chapter4::ToggleChip(const FString& Id)
{
	if (!Have(Id)) return;
	TArray<FString>& P = S.Placed;
	const int32 I = P.IndexOfByKey(Id);
	if (I >= 0) P.RemoveAt(I); else P.Add(Id);
	Reply.Reset();
}

void US47Chapter4::ShowEvidence()
{
	const TArray<FString>& P = S.Placed;
	const bool bPrints = P.Contains(TEXT("frame01")) && P.Contains(TEXT("frame02"));
	if (!bPrints || !P.Contains(TEXT("e07")))
	{
		S.Wrong10++;
		Reply = FS47Result{ false, !bPrints ? TEXT("N. VEGA: \"Both prints from B-12. The first one, and the control.\"") : TEXT("N. VEGA: \"And the original. The one from the field box, not the service copy.\"") };
		Save();
		return;
	}
	S.bShown = true;
	Mode = TEXT("control");
	Reply.Reset();
	Lines = { Aside(TEXT("She lays the two prints side by side and puts the 1947 record under them.")) };
	if (P.Contains(TEXT("e06"))) Lines.Add(Nv(TEXT("The amended copy you can keep. I know what it says. I wrote it.")));
	Lines.Add(Nv(TEXT("I know that second line.")));
	Lines.Add(Nv(TEXT("Tell me which source you took away before you made the control.")));
	if (P.Contains(TEXT("frame03"))) FaceDown(TEXT("table"));
	Save();
}

// The third one (story/theThird.ts): when FRAME 03 has the band on it, she sees it the first time the
// print is in front of her, on the table or under PHOTO 04 from the same roll, and turns it face down.
// Not a word: told under her lines a moment after them (TalkView.after), never voiced or read with
// them, so her recorded lines keep their voice; and kept in the notebook. (A toast would be under the
// talk, not over it.)
void US47Chapter4::FaceDown(const FString& How)
{
	if (S.Down3.Get(false) || !S47J::Has(S47J::Obj(S47J::Obj(Ch3(), TEXT("f3")), TEXT("targets")), TEXT("band"))) return;
	S.Down3 = true;
	AfterText = How == TEXT("table")
		? TEXT("Her hand stops on PHOTO 03. She looks at it for a long moment, then turns it face down on the table and keeps her hand flat on it. She does not say anything.")
		: TEXT("PHOTO 03 is under it, from the same roll. She slides it out, looks at it for a long moment, and turns it face down on the table. She does not say anything.");
	Note(How == TEXT("table")
		? FString::Printf(TEXT("%s. N. Vega turned PHOTO 03 face down on her table and kept her hand on it. She did not say a word about it."), *Time())
		: FString::Printf(TEXT("%s. N. Vega turned PHOTO 03 face down on her table. It was under PHOTO 04, from the same roll. She did not say a word about it."), *Time()));
}

void US47Chapter4::Choose(const FString& Id)
{
	if (M && M->Audio) M->Audio->StopPhoneCalls();
	if (M && M->UI && M->UI->NpcDialogue) M->UI->NpcDialogue->Stop();
	Reply.Reset();
	AfterText.Reset();
	if (Mode == TEXT("control")) { AnswerControl(Id); return; }
	if (Mode == TEXT("tone")) { AnswerTone(Id); return; }
	if (Mode == TEXT("mark")) { MarkTalk(false); return; }
	if (Id == TEXT("record")) { Mode = TEXT("tone"); Lines = { You(TEXT("About the amended copy.")), Aside(TEXT("She does not look away from it.")) }; return; }
	if (Id == TEXT("tomas")) { TopicTomas(); return; }
	if (Id == TEXT("cable")) { TopicCable(); return; }
	if (Id == TEXT("line")) { TopicLine(); return; }
	if (Id == TEXT("board")) { if (M && M->Board.Open) M->Board.Open(TEXT("ROOM 6"), TEXT("ch4")); return; }
	if (Id == TEXT("end")) { Ending(); return; }
	if (Id == TEXT("bye")) { if (M && M->UI) M->UI->Close(); return; }
}

void US47Chapter4::AnswerControl(const FString& Id)
{
	const US47Chapter1* C1 = Ch1();
	const FString Method = C1 ? C1->S.Method : S47J::Str(S47J::Obj(BeganCase, TEXT("s")), TEXT("method"));
	const bool bRight = Method == TEXT("passive") ? Id == TEXT("lamp") : Method == TEXT("active") ? Id == TEXT("none") : Id != TEXT("receiver");
	if (!bRight)
	{
		S.Wrong10++;
		Reply = FS47Result{ false, Id == TEXT("receiver")
			? TEXT("N. VEGA: \"The receiver has nothing to do with the film. Look at how you made the second print.\"")
			: Method == TEXT("active")
				? TEXT("N. VEGA: \"That is not what your print says. The lamp was on when you took it.\"")
				: TEXT("N. VEGA: \"That is not what your print says. Your lamp was shielded.\"") };
		Save();
		return;
	}
	S.bP10 = true;
	// (taken tonight: she has a chair out for you, and then she sees it: markTalk)
	Mode = Taken && Taken() && !S.Mark.Get(false) ? TEXT("mark") : TEXT("menu");
	Lines = {
		Id == TEXT("lamp") ? Nv(TEXT("The lamp. That is what I did in 1947. I opened the lamp circuit, and the mark stayed on the plate.")) : Nv(TEXT("You moved it, and the second line did not move with it. Then it was not the drive.")),
		Nv(TEXT("Then you have what I had. A reference that stays on the film after its source is gone.")),
		Aside(TEXT("P10 recorded. She pushes a chair out for you with her foot.")),
	};
	File(DocFinding(TEXT("p10")));
	Note(TEXT("P10: tonight's control print and her 1947 plate carry the same mark, a reference that stays on the film after its source is taken away. She recognised it at once."));
	if (M && M->Audio) M->Audio->Sfx(TEXT("click"), Snd(0.5f));
	Milestone();
}

void US47Chapter4::AnswerTone(const FString& InTone)
{
	S.Tone = InTone;
	S.Topics.bRecord = true;
	Mode = TEXT("menu");
	Lines = {
		You(InTone == TEXT("mild") ? TEXT("Why did you change it?") : TEXT("You falsified the record.")),
		InTone == TEXT("mild")
			? Nv(TEXT("I thought leaving out one line would stop someone connecting it again. I was wrong about what a missing line could hide."))
			: Nv(TEXT("Yes. I changed it. I called it a development fault because I could not write what was left after the cable was cut.")),
		Nv(TEXT("I wrote that the lamp remained. It was easier than writing that he did.")),
		Nv(TEXT("I typed this in the office tonight, before you came. I have not signed it yet.")),
		Aside(TEXT("She signs it and lays it on the table, on top of the amended copy. E12 filed.")),
	};
	Give(TEXT("correction"));
	Note(FString::Printf(TEXT("N. Vega on the amended record: she left out C so that nobody would connect the arrangement again, and blamed development. She signed a correction tonight (E12). %s"),
		InTone == TEXT("critical") ? TEXT("I put it to her as falsification. She did not argue.") : TEXT("")).TrimEnd());
}

// The mark (FORSLAG.md, 20 October, item 9 C): after a time away it is on one side of the face, and
// nobody has said so. She sees it in her lamp. New lines, not recorded: their own exchange, so the
// recorded ones around them keep their voice (core/npcDialogue.ts).
// Tomás in June: his field book has the night (story/fieldBook.ts, Jun 27).
void US47Chapter4::MarkTalk(bool bLater)
{
	S.Mark = true;
	Mode = TEXT("menu");
	// (PHOTO 03 face down was told under the lines before these: not under these)
	AfterText.Reset();
	Lines = {
		Aside(bLater ? TEXT("She looks up when you come to the table, and does not look away. She tips the shade of the lamp towards you.")
			: TEXT("You sit. She does not. She tips the shade of the lamp towards you and looks at the side of your face.")),
		Nv(TEXT("You have been out on the line.")),
		Nv(TEXT("One side of your face.")),
		Aside(TEXT("You put a hand to it. The skin there is hot and tight, as after a day in the sun. You had not felt it until now.")),
		Nv(TEXT("Tomás came back from it like that in June.")),
		Aside(TEXT("She lets the shade go back.")),
	};
	Note(FString::Printf(TEXT("%s. N. Vega, under her lamp in room 6: \"You have been out on the line. One side of your face.\" The skin on that side is hot and tight, like sunburn. I had not felt it. \"Tomás came back from it like that in June.\"%s"),
		*Time(), S47J::Has(Ch3(), TEXT("fb")) ? TEXT(" His field book has it: June 27, 1947.") : TEXT("")));
	Save();
}

void US47Chapter4::TopicTomas()
{
	S.Topics.bTomas = true;
	Lines = {
		You(TEXT("Who was T. Vega?")),
		Nv(TEXT("Tomás. My brother. He was nineteen that summer, the field assistant, and very sure of himself.")),
		Nv(TEXT("He has been nineteen for thirty-nine years.")),
		Nv(TEXT("He read the last observation into the field line. You heard him tonight, on the receiver in the hut.")),
		Nv(TEXT("He wrote me this the day before. I have never shown it to anyone.")),
		Aside(TEXT("She puts a folded letter on the table. E13 filed.")),
	};
	Give(TEXT("letter"));
	Note(TEXT("T. Vega was Tomás, her brother, the field assistant in 1947. Nineteen. His voice is the one on the receiver. She gave me his letter (E13)."));
}

void US47Chapter4::TopicCable()
{
	S.Topics.bCable = true;
	Lines = {
		Aside(TEXT("You put PHOTO 04 in front of her.")),
		Nv(TEXT("Yes. I cut it.")),
		Nv(TEXT("The lamp was off. The reference stayed anyway. Tomás was out on C with the last reading, and the arrangement was answering him.")),
		Nv(TEXT("I thought if I broke the connection it would stop. It stopped. He was not there afterwards.")),
		Nv(TEXT("This was in his coat. He knew more than I did.")),
		Aside(TEXT("She puts a field card on the table. E11 filed.")),
	};
	FaceDown(TEXT("roll"));
	Give(TEXT("card"));
	Note(TEXT("She cut the cable in 1947, with Tomás standing on C inside the active reference. Afterwards he was not there. She gave me his last field card (E11)."));
}

void US47Chapter4::Give(const FString& Which)
{
	if (Room) Room->Obj(Which == TEXT("card") ? FString(TEXT("fieldCard")) : Which).SetVisible(true);
	File(Which == TEXT("card") ? DocCard() : Which == TEXT("letter") ? DocLetter() : DocCorrection());
	if (M && M->Audio) M->Audio->Sfx(TEXT("thudSoft"), Snd(0.18f, 1.8f));
	Milestone();
}

// P11, recorded on the case board once both notes are on it. Her answer is waiting in the talk when
// the player turns back to her.
FS47Result US47Chapter4::RecordP11()
{
	if (!(S.Topics.bRecord && S.Topics.bTomas && S.Topics.bCable)) return { false, TEXT("Hear her out first: the amended record, T. Vega and the cable.") };
	if (S.bP11) return { true, SUPPORTED_P11 };
	S.bP11 = true;
	Mode = TEXT("menu");
	Lines = {
		You(TEXT("You thought the whole record would bring it back.")),
		Nv(TEXT("Write it like that. Not cleaner than it was.")),
		Aside(TEXT("P11 recorded.")),
	};
	File(DocFinding(TEXT("p11")));
	Note(TEXT("P11: she rewrote the finding because she believed the complete record would let someone repeat the arrangement. She cut the cable herself, with Tomás inside the reference. Nobody knows what became of him."));
	if (M && M->Audio) M->Audio->Sfx(TEXT("click"), Snd(0.5f));
	Milestone();
	return { true, SUPPORTED_P11 };
}

// P12: the phone on the nightstand is on the old field line.
void US47Chapter4::TopicLine()
{
	US47Audio* A = M ? M->Audio.Get() : nullptr;
	if (S.bLine) { Mode = TEXT("menu"); Lines = { Aside(TEXT("The handset is back on the cradle.")) }; return; }
	const FS47Obj Ph = Room ? Room->Proxy(TEXT("phone")) : FS47Obj();
	const TOptional<FVector> At = Ph ? TOptional<FVector>(Pos(Ph)) : TOptional<FVector>();
	Lines = {
		You(TEXT("Where does that phone go?")),
		Nv(TEXT("To the office. At night the office line is patched through to this room, and the other end of it is the field hut. It has been since 1947.")),
		Nv(TEXT("You left the receiver on out there. Listen.")),
		Aside(TEXT("She lifts the handset and holds it between you. A carrier, very faint, under the hiss.")),
		Nv(TEXT("Tomás. Where did you leave the spare key?")),
		{ LINE, TEXT("Reference west. No. East. Hold the last reading.") },
		Nv(TEXT("That is what he said then. Not an answer to what I asked now.")),
		Aside(TEXT("She puts the handset down.")),
	};
	S.bLine = true;
	Mode = TEXT("menu");
	// Character mode plays Nora and the recorded Price line through the panel's single actor-aware
	// queue. The legacy whole-exchange phone voice remains optional.
	TOptional<FS47CallTimeline> Spoken;
	if (A && !A->bCharacterVoices)
	{
		TArray<FS47PhoneCue> Cues;
		for (const FLine& L : Lines) if (!L.Who.IsEmpty()) { FS47PhoneCue C; C.By = L.Who; C.Text = L.Text; Cues.Add(C); }
		Spoken = A->PhoneCall(Cues);
	}
	if (A && !A->bCharacterVoices && !Spoken && At)
	{
		A->Sfx(TEXT("click"), Snd(0.5f, 1.f, *At));
		A->Radio(*At, 0.08f);
		A->RadioVoice(4.4f, 3.6f);
		TWeakObjectPtr<US47Audio> Wa = A;
		const FVector P = *At;
		if (Host) Host->After(9.5, [Wa, P] { if (!Wa.IsValid()) return; Wa->Radio({}); Wa->Sfx(TEXT("click"), Snd(0.45f, 1.f, P)); });
	}
	Note(TEXT("The phone in room 6 is on the old field line to the hut. She asked it: \"Tomás. Where did you leave the spare key?\" The line said: \"Reference west. No. East. Hold the last reading.\" The same words as in 1947."));
	Toast(TEXT("The same words as on the receiver in the hut."), 3);
	Save();
}

// P12, recorded on the case board.
FS47Result US47Chapter4::RecordP12()
{
	if (!S.bLine) return { false, TEXT("Ask her about the phone on the nightstand first.") };
	if (S.bP12) return { true, SUPPORTED_P12 };
	S.bP12 = true;
	Mode = TEXT("menu");
	Lines = {
		Nv(TEXT("No. It is not him. It is what he left.")),
		Aside(TEXT("P12 recorded.")),
	};
	File(DocFinding(TEXT("p12")));
	Note(TEXT("P12: the return line repeats the 1947 correction unchanged. A new question gets the same fragment. It is a retained reference, not a conversation."));
	if (M && M->Audio) M->Audio->Sfx(TEXT("click"), Snd(0.5f));
	Milestone();
	return { true, SUPPORTED_P12 };
}

void US47Chapter4::Ending()
{
	S.bTold = true;
	S.Stage = TEXT("leave");
	Lines = {
		You(TEXT("Where did C go?")),
		Nv(TEXT("East from the station, over the fence, and on across the old highway to Roswell. There is a survey bolt in the shoulder where it crosses.")),
		Nv(TEXT("Tomás walked out to the closing point with the last reading.")),
		Nv(TEXT("If you want to know whether the line still holds, drive the road.")),
		Nv(TEXT("Do not stop on the line.")),
	};
	Note(FString::Printf(TEXT("%s. C ran east from STATION 01 and crosses the old highway to Roswell at a survey bolt in the shoulder. Tomás walked out to the closing point with the last reading. \"If you want to know whether the line still holds, drive the road. Do not stop on the line.\""), *Time()));
	Milestone();
}

void US47Chapter4::Finish()
{
	if (S.Stage == TEXT("complete")) return;
	S.Stage = TEXT("complete");
	Note(TEXT("ROOM 6: P10 to P12 recorded, with her correction (E12), Tomás' letter (E13) and his last field card (E11). Next: the road south, where C crosses it."));
	Milestone();
	TWeakObjectPtr<US47Chapter4> W = this;
	if (Host) Host->After(1.2, [W] { if (W.IsValid() && W->OnEnd) W->OnEnd(); });
}

// ---------- documents ----------

FS47Doc US47Chapter4::DocCard() const
{
	FS47Doc D;
	D.Id = TEXT("e11"); D.Title = TEXT("E11 / T. Vega, last observation card"); D.Kind = TEXT("hand"); D.Stamp = TEXT("E11"); D.Page = CARD;
	D.Paper = TEXT("cardField"); D.PaperRatio = 1.6; // artUrl('cardField')
	D.Transcript = TEXT("A field card in pencil, from Tomás Vega's coat. She has kept it for thirty-nine years.\n\nThe first half separates opening the lamp from masking C. The second half says how the last reading must be taken, and who should not take it.");
	return D;
}

FS47Doc US47Chapter4::DocLetter() const
{
	FS47Doc D;
	D.Id = TEXT("e13"); D.Title = TEXT("E13 / A letter from T. Vega, 1947"); D.Kind = TEXT("hand"); D.Stamp = TEXT("E13"); D.Page = LETTER;
	D.Paper = TEXT("letterPaper"); D.PaperRatio = 0.75; // artUrl('letterPaper')
	D.Transcript = TEXT("Tomás to his sister, the day before the last observation. No instrument language. The spare key is the key to the field hut.");
	return D;
}

FS47Doc US47Chapter4::DocCorrection() const
{
	FS47Doc D;
	D.Id = TEXT("e12"); D.Title = TEXT("E12 / Correction to the amended record"); D.Kind = TEXT("typed"); D.Stamp = TEXT("E12"); D.Page = CORRECTION;
	D.Transcript = TEXT("Typed on the office machine earlier tonight and signed at the table, in front of you. It is attached to the amended copy. It does not replace it.");
	return D;
}

FS47Doc US47Chapter4::DocFinding(const FString& K) const
{
	FS47Doc D;
	D.Id = K; D.Kind = TEXT("typed"); D.Stamp = K.ToUpper(); D.Page = Supported(K);
	if (K == TEXT("p10")) { D.Title = TEXT("P10 / The same mark"); D.Transcript = TEXT("Tonight's two B-12 prints beside the 1947 original. The control did not remove the second reference, then or now."); }
	else if (K == TEXT("p11")) { D.Title = TEXT("P11 / Why the record was changed"); D.Transcript = TEXT("The omission of C and the cut cable had the same purpose. Neither says what became of Tomás."); }
	else { D.Title = TEXT("P12 / The return line"); D.Transcript = TEXT("A new question on the old field line gets the 1947 fragment back, unchanged."); }
	return D;
}

// ---------- per frame ----------

void US47Chapter4::Update(double Dt, double T)
{
	(void)T;
	if (!bActive || !M) return;
	// on the way down the highway in the truck, Nora calls it on the CB (Tom's improvements, 16
	// October: the CB did nothing). Once; it is in the notebook after.
	const TOptional<double> Driven = M->World ? M->World->TripSecs() : TOptional<double>();
	if (S.Stage == TEXT("to-motel") && !S.Cb.Get(false) && Driven && *Driven > 9 && Cb)
	{
		const FString By = TEXT("N. VEGA");
		TWeakObjectPtr<US47Chapter4> W = this;
		const bool bOk = Cb({
			{ TEXT("SARO seven, this is Sierra. You hear me?"), TEXT("woman"), By },
			{ TEXT("Pull in at the north end, by the office. Not down past the rooms."), TEXT("woman"), By },
			{ TEXT("Room six. The door is open. Bring the prints."), TEXT("woman"), By },
		}, [W]
		{
			if (!W.IsValid()) return;
			W->Note(FString::Printf(TEXT("%s. N. Vega on the truck's CB, on the way down the highway: pull in by the office, room six, the door is open, bring the prints."), *W->Time()));
			W->Save();
		});
		if (bOk) { S.Cb = true; UE_LOG(LogS47Ch4, Display, TEXT("S47: ch4: N. Vega calls the truck on the CB")); }
	}
	// N. Vega turns her head to whoever is in the room with her
	FS47Obj Head = Room ? Room->Obj(TEXT("noraHead")) : FS47Obj();
	const FS47Obj Parent = Head ? Head.Parent() : FS47Obj();
	if (Head && InRoom() && Parent && M->Player)
	{
		FVector At = M->Player->Pos;
		At.Y = 1.2;
		const FVector P = Parent.WorldToLocal(At);
		const FVector Hp = Head.Position();
		const double Want = FMath::Clamp(FMath::Atan2(-(P.X - Hp.X), -(P.Z - Hp.Z)), -0.9, 0.9);
		HeadYaw += (Want - HeadYaw) * FMath::Min(1.0, Dt * 3);
		Head.SetRotY(HeadYaw);
	}
	if (M->UI) M->UI->Objective(Objective());
}

// ---------- the host (Prologue.ts) ----------

void US47Chapter4::BeginChapter4(const S47J::FObj& SavedCase)
{
	// Prologue.beginChapter4: after the field prints, down the highway to room 6 at Sierra Motor Court.
	// (ch3.active = false: its field roll is filed; the truck and the station stay as they are. The host's
	// BeginChapter makes the other chapters stop leading. setPhase('ch4') is the host's chapter key.)
	if (!Host) return;
	if (SavedCase.IsValid()) BeganCase = SavedCase;
	Host->SetCinematic(false);
	Host->ClearTimers();
	const S47J::FObj Ch4 = S47J::Obj(SavedCase, TEXT("ch4"));
	if (Ch4.IsValid())
	{
		Host->NotesMutable() = S47J::Strs(SavedCase, TEXT("notes"));
		Host->SetClock(S47J::Num(SavedCase, TEXT("clock"), Host->Clock()));
	}
	else Host->SetClock(FMath::Max(Host->Clock(), S47Timeline::Chapter4(Host->bEveningTimeline)));
	if (M && M->UI) M->UI->Fade(false);
	Host->BeginChapter(TEXT("ch4"), Ch4); // (ch4.begin, then onCheckpoint('chapter4'))
}

void US47Chapter4::JumpChapter4()
{
	// Prologue.jumpChapter4: chapter four from a checkpoint, or for testing straight after chapter three
	// (without photographs) when there is no saved case.
	if (!Host) return;
	const S47J::FObj Saved = Host->LoadCase ? Host->LoadCase() : S47J::FObj();
	Host->AfterPrologue(Saved);
	S47J::FObj Done = Saved;
	if (!(Saved.IsValid() && S47J::Str(S47J::Obj(Saved, TEXT("ch3")), TEXT("stage")) == TEXT("complete")))
	{
		const S47J::FObj J1 = S47J::New();
		S47J::Set(J1, TEXT("v"), 1); S47J::Set(J1, TEXT("stage"), TEXT("complete")); S47J::Set(J1, TEXT("eventClock"), Host->EventClockSec());
		S47J::Set(J1, TEXT("camera"), true); S47J::Set(J1, TEXT("door"), true); S47J::Set(J1, TEXT("labDoor"), true); S47J::Set(J1, TEXT("log"), true); S47J::Set(J1, TEXT("returned"), true);
		S47J::Set(J1, TEXT("dev1"), 3); S47J::Set(J1, TEXT("dev2"), 3); S47J::Set(J1, TEXT("ref"), true); S47J::Set(J1, TEXT("hyp"), TEXT("light")); S47J::Set(J1, TEXT("method"), TEXT("passive"));
		S47J::Set(J1, TEXT("observed"), true); S47J::Set(J1, TEXT("concluded"), true); S47J::Set(J1, TEXT("filed"), true);
		S47J::Set(J1, TEXT("wrongRef"), 0); S47J::Set(J1, TEXT("wrongHyp"), 0); S47J::Set(J1, TEXT("wrongConcl"), 0); S47J::Set(J1, TEXT("rejected"), 0);
		const S47J::FObj J2 = S47J::New();
		S47J::Set(J2, TEXT("v"), 1); S47J::Set(J2, TEXT("stage"), TEXT("complete")); S47J::Set(J2, TEXT("answered"), true); S47J::Set(J2, TEXT("door"), true); S47J::Set(J2, TEXT("entered"), true);
		const S47J::FObj Read = S47J::New();
		for (const TCHAR* K : { TEXT("original"), TEXT("amended"), TEXT("lineage"), TEXT("index") }) S47J::Set(Read, K, true);
		S47J::Set(J2, TEXT("read"), Read);
		S47J::Set(J2, TEXT("p04"), true); S47J::Set(J2, TEXT("p05"), true);
		S47J::Set(J2, TEXT("finding"), TEXT("omitted-c")); S47J::Set(J2, TEXT("destination"), TEXT("old-survey-station")); S47J::Set(J2, TEXT("surveyId"), TEXT("STATION 01")); S47J::Set(J2, TEXT("marker"), TEXT("triangle-bar"));
		S47J::Set(J2, TEXT("wrong04"), 0); S47J::Set(J2, TEXT("wrong05"), 0); S47J::Set(J2, TEXT("called"), true);
		const S47J::FObj J3 = S47J::New();
		S47J::Set(J3, TEXT("v"), 1); S47J::Set(J3, TEXT("stage"), TEXT("complete")); S47J::Set(J3, TEXT("arrived"), true); S47J::Set(J3, TEXT("readE09a"), true); S47J::Set(J3, TEXT("readE10"), true); S47J::Set(J3, TEXT("heard"), true);
		const S47J::FObj Seen = S47J::New();
		for (const TCHAR* K : { TEXT("footing"), TEXT("markerA"), TEXT("markerB"), TEXT("stakes"), TEXT("diagram"), TEXT("hut") }) S47J::Set(Seen, K, true);
		S47J::Set(J3, TEXT("seen"), Seen);
		S47J::Set(J3, TEXT("lampCovered"), false); S47J::Set(J3, TEXT("lampObserved"), true); S47J::Set(J3, TEXT("cableInspected"), true);
		S47J::Set(J3, TEXT("p06"), true); S47J::Set(J3, TEXT("p07"), true); S47J::Set(J3, TEXT("p08"), true); S47J::Set(J3, TEXT("p09"), true);
		S47J::Set(J3, TEXT("wrong06"), 0); S47J::Set(J3, TEXT("wrong08"), 0); S47J::Set(J3, TEXT("wrong09"), 0); S47J::Set(J3, TEXT("rejected"), 0); S47J::Set(J3, TEXT("called"), true); S47J::Set(J3, TEXT("dev"), 3);
		Done = S47J::New();
		S47J::Set(Done, TEXT("s"), J1);
		S47J::Set(Done, TEXT("ch2"), J2);
		S47J::Set(Done, TEXT("ch3"), J3);
		S47J::Set(Done, TEXT("notes"), Host->Notes());
		S47J::Set(Done, TEXT("clock"), 4 * 3600.0 + 32 * 60);
	}
	BeganCase = Done;
	// Prologue.beginChapter1(done): chapter one's Begin takes the whole case (notes, clock, its state)
	if (Host->Chapter(TEXT("ch1"))) Host->BeginChapter(TEXT("ch1"), Done);
	// Prologue.beginChapter2(done)
	if (US47Chapter2* C2 = Cast<US47Chapter2>(Host->Chapter(TEXT("ch2")))) C2->BeginChapter2(Done);
	// Prologue.beginChapter3(done), the host's part, when chapter three is in this version (the
	// integrator may put chapter three's own begin here): its phase, no cinematic, no timers, the notes
	// and the clock of the case, ch3.begin(done.ch3)
	if (Host->Chapter(TEXT("ch3")))
	{
		const S47J::FObj J3 = S47J::Obj(Done, TEXT("ch3"));
		Host->SetCinematic(false);
		Host->ClearTimers();
		if (J3.IsValid())
		{
			Host->NotesMutable() = S47J::Strs(Done, TEXT("notes"));
			Host->SetClock(S47J::Num(Done, TEXT("clock"), Host->Clock()));
		}
		else Host->SetClock(FMath::Max(Host->Clock(), S47Timeline::Chapter3(Host->bEveningTimeline)));
		if (M && M->UI) M->UI->Fade(false);
		// Prologue.beginChapter3 (its own: the clock to 03:40, the station bound)
		if (US47Chapter3* C3 = Cast<US47Chapter3>(Host->Chapter(TEXT("ch3")))) C3->BeginChapter3(Done);
		else Host->BeginChapter(TEXT("ch3"), J3);
	}
	BeginChapter4(S47J::Has(Done, TEXT("ch4")) ? Done : S47J::FObj());
}

// ---------- -S47Ch4Drive ----------

void US47Chapter4::TestDrive(const FString& Stop)
{
	// tools/chapter4.py's moves, done from here: into room 6, to her table, the evidence, the control,
	// the line, the three parts and the board, where C goes, out. Each stop holds the game there. (On
	// the world's timers: game time stands while the talk is open.)
	if (!M || !M->Web || !M->Player) return;
	TWeakObjectPtr<US47Chapter4> W = this;
	static const TArray<FString> Order = { TEXT("room"), TEXT("talk"), TEXT("table"), TEXT("p10"), TEXT("line"), TEXT("end"), TEXT("leave") };
	const int32 Upto = Stop == TEXT("walkie") || Stop == TEXT("tv") || Stop == TEXT("tvnear") ? 0 : Order.IndexOfByKey(Stop);
	auto Say = [](const FString& T) { UE_LOG(LogS47Ch4, Display, TEXT("S47: ch4 drive: %s"), *T); };
	auto Later = [W](float Secs, TFunction<void()> Fn)
	{
		if (!W.IsValid()) return;
		FTimerHandle H;
		W->M->Web->GetWorldTimerManager().SetTimer(H, FTimerDelegate::CreateLambda([W, Fn] { if (W.IsValid()) Fn(); }), Secs, false);
	};
	auto Refresh = [W] { if (W.IsValid() && W->Panel && W->Panel->Refresh) W->Panel->Refresh(); };
	Later(0.6f, [W, Say] { W->SetLeaf(1); if (W->M->World) W->M->World->GoRoom6(); Say(TEXT("through the door of room 6")); });
	Later(1.8f, [W, Say, Upto, Stop]
	{
		// the talk anchor, facing her
		const US47Room6::FAnchor A = W->Room ? W->Room->Anchor(TEXT("talk")) : US47Room6::FAnchor();
		const FVector Nora = W->Room ? W->Room->Proxy(TEXT("nora")).WorldPosition() : FVector::ZeroVector;
		W->M->Player->Place(A.X, A.Z, FMath::Atan2(-(Nora.X - A.X), -(Nora.Z - A.Z)));
		W->M->Player->Pitch = FMath::Atan2(Nora.Y + 0.25 - 1.62, FMath::Sqrt(FMath::Square(Nora.X - A.X) + FMath::Square(Nora.Z - A.Z)));
		Say(FString::Printf(TEXT("room: area %s, arrived %d, stage %s, tv %d"), *W->Area(), W->S.bArrived, *W->S.Stage, W->Room && W->Room->bTvOn));
		if (Stop == TEXT("tv") || Stop == TEXT("tvnear"))
		{
			// the set on the dresser, snow on every channel
			const FVector Tv = W->Room ? W->Room->Proxy(TEXT("tv")).WorldPosition() : FVector::ZeroVector;
			W->M->Player->Yaw = FMath::Atan2(-(Tv.X - A.X), -(Tv.Z - A.Z));
			W->M->Player->Pitch = FMath::Atan2(Tv.Y - 1.62, FMath::Sqrt(FMath::Square(Tv.X - A.X) + FMath::Square(Tv.Z - A.Z)));
			// tvnear: straight in front of the screen, a metre off (its picture's shape)
			US47Room6::FAnchor Stand = A;
			const FS47Obj Set = W->Room ? W->Room->Obj(TEXT("tv")) : FS47Obj();
			const FVector Screen = Set ? Set.LocalToWorld(FVector(-0.055, 0.29, 0.279)) : Tv;
			if (Stop == TEXT("tvnear") && Set)
			{
				const FVector Out = Set.WorldDirection();
				Stand.X = Screen.X + Out.X * 1.0; Stand.Z = Screen.Z + Out.Z * 1.0;
				W->M->Player->Yaw = FMath::Atan2(Out.X, Out.Z);
			}
			// (held there: a stray mouse on the test machine must not turn the view off the set)
			const double Yaw = W->M->Player->Yaw, Dist = FMath::Sqrt(FMath::Square(Screen.X - Stand.X) + FMath::Square(Screen.Z - Stand.Z));
			FTimerHandle Hold;
			W->M->Web->GetWorldTimerManager().SetTimer(Hold, FTimerDelegate::CreateLambda([W, Stand, Yaw, Screen, Dist]
			{
				if (!W.IsValid() || !W->M->Player || !W->InRoom()) return;
				AS47Walker* P = W->M->Player; P->Place(Stand.X, Stand.Z, Yaw); P->Pitch = FMath::Atan2(Screen.Y - P->FloorY - P->Eye, Dist);
			}), 0.25f, true);
			// the broadcast, when staged: its picture and sound some seconds apart
			for (const float At : { 3.f, 9.f })
			{
				FTimerHandle H;
				W->M->Web->GetWorldTimerManager().SetTimer(H, FTimerDelegate::CreateLambda([W, Say]
				{
					if (W.IsValid() && W->Room && W->Room->HasProgramme()) Say(TEXT("tv: ") + W->Room->ProgrammeState());
				}), At, false);
			}
			return;
		}
		if (Stop == TEXT("walkie"))
		{
			const FVector Wk = W->Room ? W->Room->Proxy(TEXT("walkie")).WorldPosition() : FVector::ZeroVector;
			W->M->Player->Pitch = FMath::Atan2(Wk.Y - 1.62, FMath::Sqrt(FMath::Square(Wk.X - A.X) + FMath::Square(Wk.Z - A.Z)));
			W->M->Player->Yaw = FMath::Atan2(-(Wk.X - A.X), -(Wk.Z - A.Z));
			W->UseWalkie();
			Say(FString::Printf(TEXT("walkie: %s, label '%s'"), *W->WalkieState, *W->WalkieLabel()));
			// 17.2 s on: Nora took the battery out; press to talk, and four and seven come back
			FTimerHandle H1, H2;
			W->M->Web->GetWorldTimerManager().SetTimer(H1, FTimerDelegate::CreateLambda([W, Say]
			{
				if (!W.IsValid()) return;
				Say(FString::Printf(TEXT("walkie: %s, label '%s', heard %d"), *W->WalkieState, *W->WalkieLabel(), W->bWalkieHeard));
				W->UseWalkie();
			}), 18.5f, false);
			W->M->Web->GetWorldTimerManager().SetTimer(H2, FTimerDelegate::CreateLambda([W, Say]
			{
				if (!W.IsValid()) return;
				Say(FString::Printf(TEXT("walkie: %s, label '%s', sent %d"), *W->WalkieState, *W->WalkieLabel(), W->bWalkieSent));
			}), 27.f, false);
			return;
		}
		if (Upto >= 1) { W->OpenTalk(); Say(FString::Printf(TEXT("talk: greeted %d, mode %s"), W->S.bGreeted, *W->Mode)); }
	});
	if (Upto < 2) return;
	Later(3.0f, [W, Say, Refresh, Upto]
	{
		// P10: one paper is not enough; then the prints, the original and the amended copy
		W->ToggleChip(TEXT("e07")); W->ShowEvidence();
		const FString Wrong = W->Reply ? W->Reply->Text : FString();
		for (const TCHAR* C : { TEXT("frame01"), TEXT("frame02"), TEXT("e06") }) W->ToggleChip(C);
		W->ShowEvidence();
		Refresh();
		Say(FString::Printf(TEXT("table: wrong '%s', shown %d, mode %s"), *Wrong, W->S.bShown, *W->Mode));
		if (Upto < 3) return;
		const FString Method = W->Ch1() ? W->Ch1()->S.Method : FString(TEXT("passive"));
		W->Choose(Method == TEXT("active") ? TEXT("none") : TEXT("lamp"));
		Refresh();
		Say(FString::Printf(TEXT("p10: %d, lines %d"), W->S.bP10, W->Lines.Num()));
		if (Upto < 4) return;
		W->Choose(TEXT("line"));
		Refresh();
		Say(FString::Printf(TEXT("line: %d, %s"), W->S.bLine, W->Lines.Num() > 5 ? *W->Lines[5].Text : TEXT("")));
		if (Upto < 5) return;
		W->Choose(TEXT("tomas"));
		W->Choose(TEXT("record")); W->Choose(TEXT("critical"));
		W->Choose(TEXT("cable"));
		FS47RuleBook& B = *W->Book;
		const TOptional<FS47Result> R1 = B.Connect(TEXT("r6.line"), TEXT("e10.carrier"), false);
		const TOptional<FS47Result> R2 = B.Record(TEXT("P12"));
		B.Connect(TEXT("e12.why"), TEXT("P04"), false);
		B.Connect(TEXT("e12.cut"), TEXT("P08"), false);
		const TOptional<FS47Result> R3 = B.Record(TEXT("P11"));
		W->Choose(TEXT("end"));
		Refresh();
		Say(FString::Printf(TEXT("end: p11 %d, p12 %d, told %d, stage %s (%s / %s / %s)"), W->S.bP11, W->S.bP12, W->S.bTold, *W->S.Stage,
			R1 ? *R1->Text : TEXT("-"), R2 ? *R2->Text.Left(20) : TEXT("-"), R3 ? *R3->Text.Left(20) : TEXT("-")));
		if (Upto < 6) return;
		if (W->M->UI) W->M->UI->Close();
		W->UseDoor();
		Say(TEXT("leave: out of room 6"));
	});
}

// ---------- self test ----------

bool US47Chapter4::SelfTest(TArray<FString>& Out)
{
	bool bOk = S47Walkie::SelfTest(Out);
	bOk &= FS47CB::SelfTest(Out);
	auto Check = [&](bool bCond, const FString& What) { Out.Add(FString::Printf(TEXT("%s chapter four: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };

	// tools/chapter4.py's conversation, without the world (no host, no screen, no sound): the chapters
	// before are read from the case it began with, as jumpChapter4 writes it (passive control)
	US47Chapter4* C = NewObject<US47Chapter4>(GetTransientPackage());
	C->BuildBook();
	C->BeganCase = S47J::Read(TEXT("{\"s\":{\"stage\":\"complete\",\"filed\":true,\"method\":\"passive\"},\"ch2\":{\"read\":{\"original\":true,\"amended\":true}},\"ch3\":{\"stage\":\"complete\",\"dev\":3}}"));
	C->bStarted = true; C->bActive = true;
	Check(C->Objective() == TEXT("SIERRA MOTOR COURT // ROOM 6, DOWN THE HIGHWAY SOUTH. THE TRUCK, OR THE FIRE EXIT AT THE END OF THE SOUTH CORRIDOR") && C->Tasks().Num() == 1, TEXT("to the motel: the truck, or the fire exit"));
	Check(C->TruckTrip(TEXT("saro")).IsSet() && C->TruckTrip(TEXT("saro"))->Label == TEXT("Drive to Sierra Motor Court") && !C->TruckTrip(TEXT("motel")).IsSet(), TEXT("the truck's trip, not from the motel's lot"));
	C->EnteredRoom();
	Check(C->S.bArrived && C->S.Stage == TEXT("room") && C->Tasks().Num() == 7 && C->TestNotes.Num() == 1 && C->TruckTrip(TEXT("saro")).IsSet() == false, TEXT("into room 6"));
	C->OpenTalk();
	Check(C->S.bGreeted && C->Mode == TEXT("table") && C->Lines.Num() == 2 && C->Lines[0].Text == TEXT("So you came down the road."), TEXT("she speaks first"));
	S47Panels::FTalkView V = C->View();
	Check(V.Table && V.Table->Chips.Num() == 6 && V.Table->Chips[4].bHave && V.Table->Submit == TEXT("SHOW HER") && V.Status == TEXT("SIERRA MOTOR COURT // N. VEGA IS STILL WAITING IN ROOM 6"), TEXT("the table and its chips"));
	C->ToggleChip(TEXT("e07")); C->ShowEvidence();
	Check(C->Reply && C->Reply->Text.Contains(TEXT("Both prints")) && !C->S.bP10 && C->S.Wrong10 == 1, TEXT("P10: one paper is not enough"));
	for (const TCHAR* K : { TEXT("frame01"), TEXT("frame02"), TEXT("e06") }) C->ToggleChip(K);
	C->ShowEvidence();
	Check(C->S.bShown && C->Mode == TEXT("control") && C->Lines.Num() == 4 && C->Lines[1].Text.Contains(TEXT("I wrote it")), TEXT("P10: the prints and the original; she knows the amended copy"));
	C->Choose(TEXT("none"));
	Check(C->Reply && C->Reply->Text.Contains(TEXT("Your lamp was shielded")) && !C->S.bP10, TEXT("P10: a control that does not match the print is answered"));
	C->Choose(TEXT("lamp"));
	Check(C->S.bP10 && C->TestDocs.Contains(TEXT("p10")) && C->Lines[0].Text.Contains(TEXT("That is what I did in 1947")) && C->Mode == TEXT("menu"), TEXT("P10 recorded"));
	C->Choose(TEXT("tomas"));
	Check(C->S.Topics.bTomas && C->TestDocs.Contains(TEXT("e13")) && C->Lines[2].Text.Contains(TEXT("thirty-nine years")), TEXT("T. Vega: his letter (E13)"));
	C->Choose(TEXT("record"));
	V = C->View();
	Check(V.Choices.Num() == 2 && V.Choices[0].Id == TEXT("mild") && V.Choices[1].Id == TEXT("critical") && V.Question == TEXT("How do you ask her?"), TEXT("the amended record: a mild or a hard question"));
	C->Choose(TEXT("critical"));
	Check(C->S.Topics.bRecord && C->S.Tone == TEXT("critical") && C->TestDocs.Contains(TEXT("e12")) && C->Lines[1].Text.StartsWith(TEXT("Yes. I changed it")), TEXT("the amended record: her signed correction (E12)"));
	V = C->View();
	Check(!V.Choices.ContainsByPredicate([](const S47Panels::FTalkChoice& X) { return X.Id == TEXT("board"); }), TEXT("P11 waits for all three parts"));
	C->Choose(TEXT("line"));
	Check(C->S.bLine && C->Lines[5].Who == TEXT("THE LINE") && C->Lines[4].Text == TEXT("Tomás. Where did you leave the spare key?"), TEXT("P12: a new question on the old field line, the 1947 fragment back"));
	V = C->View();
	const S47Panels::FTalkChoice* Board = V.Choices.FindByPredicate([](const S47Panels::FTalkChoice& X) { return X.Id == TEXT("board"); });
	Check(Board && Board->Label == TEXT("Lay it out on the case board (P12)"), TEXT("the talk offers the case board for P12"));
	FS47RuleBook& B = *C->Book;
	auto Thread = [&B](const TCHAR* A, const TCHAR* Bb) { const TOptional<FS47Result> R = B.Connect(A, Bb, false); return R ? R->Text : FString(TEXT("(not this chapter's)")); };
	FString R = Thread(TEXT("r6.line"), TEXT("e13.key"));
	Check(R.Contains(TEXT("in the mug")) && !C->S.bP12 && B.Notes().Num() == 1, TEXT("P12: the line against his letter: it did not give the answer"));
	R = Thread(TEXT("r6.line"), TEXT("e10.carrier"));
	Check(R.Contains(TEXT("RECORD it")), TEXT("P12: the return line tied to the 1947 voice in E10"));
	const TOptional<FS47Result> Rec12 = B.Record(TEXT("P12"));
	Check(Rec12 && Rec12->bOk && C->S.bP12 && C->TestDocs.Contains(TEXT("p12")) && C->Lines[0].Text.Contains(TEXT("It is not him")), TEXT("P12 recorded; back at the table she says what the line is"));
	C->Choose(TEXT("cable"));
	Check(C->S.Topics.bCable && C->TestDocs.Contains(TEXT("e11")) && C->Lines[1].Text == TEXT("Yes. I cut it."), TEXT("the cable: she cut it; his last field card (E11)"));
	R = Thread(TEXT("e12.dev"), TEXT("e06.fault"));
	Check(R.Contains(TEXT("not a development fault")) && !C->S.bP11 && C->S.Wrong11 == 1, TEXT("P11: a wrong motive is answered"));
	Thread(TEXT("e12.why"), TEXT("P04"));
	R = Thread(TEXT("e12.cut"), TEXT("P08"));
	Check(R.Contains(TEXT("RECORD it")), TEXT("P11: her reason tied to P04, the deliberate interruption to the cut cable, P08"));
	const TOptional<FS47Result> Rec11 = B.Record(TEXT("P11"));
	Check(Rec11 && Rec11->bOk && C->S.bP11 && C->Lines[1].Text.Contains(TEXT("Not cleaner than it was")), TEXT("P11 recorded"));
	Check(C->Objective() == TEXT("SIERRA MOTOR COURT // N. VEGA IS STILL WAITING IN ROOM 6") && C->Tasks().Num() == 8 && !C->Tasks().Last().bDone, TEXT("objective (outside the room: no world here) and tasks: where C went"));
	const TArray<FS47BoardThread> Th = B.Threads();
	Check(B.Cards().Num() == 4 && Th.Num() == 6 && Th.Last().bAuto && Th.Last().B == TEXT("e07.mark"), TEXT("her papers and the line on the board, P10 resting on what it was made of"));
	V = C->View();
	Check(V.Given.Num() == 3 && V.Given[0].Id == TEXT("correction") && V.Choices.Last().Label == TEXT("Step back from the table"), TEXT("on the table: her three papers"));
	C->Choose(TEXT("end"));
	Check(C->S.bTold && C->S.Stage == TEXT("leave") && C->Lines.Last().Text == TEXT("Do not stop on the line.") && C->View().Choices.Last().Label == TEXT("Leave her be"), TEXT("where C goes: across the highway to Roswell"));
	// the walkie's label as the radio goes (no sound here)
	Check(C->WalkieLabel() == TEXT("Field radio, 1947"), TEXT("the field radio from 1947 lies on the table"));
	C->WalkieState = TEXT("playing");
	Check(C->WalkieLabel().IsEmpty(), TEXT("while it plays it has nothing to offer"));
	C->WalkieState = TEXT("on");
	Check(C->WalkieLabel() == TEXT("Press to talk"), TEXT("then it can be pressed to talk"));
	C->bWalkiePressed = true;
	Check(C->WalkieLabel() == TEXT("Turn the field radio off"), TEXT("and it can be switched off"));
	// PHOTO 03 with the band on it, under PHOTO 04: face down, once, told under her lines
	US47Chapter4* D = NewObject<US47Chapter4>(GetTransientPackage());
	D->BuildBook();
	D->BeganCase = S47J::Read(TEXT("{\"s\":{\"filed\":true,\"method\":\"active\"},\"ch2\":{\"read\":{\"original\":true}},\"ch3\":{\"dev\":3,\"f3\":{\"url\":\"\",\"targets\":{\"band\":[0.5,0.4]}},\"fb\":{}}}"));
	D->bStarted = true; D->bActive = true; D->S.bArrived = true; D->S.bP10 = true;
	D->Choose(TEXT("cable"));
	Check(D->S.Down3.Get(false) && D->View().After && D->View().After->Text.StartsWith(TEXT("PHOTO 03 is under it")), TEXT("PHOTO 03 face down, told under her lines"));
	D->Choose(TEXT("tomas"));
	Check(!D->View().After, TEXT("and gone from under the next lines"));
	D->Taken = [] { return true; };
	D->OpenTalk(); // greeted: the table first
	D->Mode = TEXT("menu");
	D->OpenTalk();
	Check(D->S.Mark.Get(false) && D->Lines[1].Text == TEXT("You have been out on the line.") && D->TestNotes.Last().EndsWith(TEXT("His field book has it: June 27, 1947.")), TEXT("taken tonight: she sees it on the face, once"));
	D->S.Placed = { TEXT("frame01"), TEXT("frame02"), TEXT("e07") };
	D->Mode = TEXT("control"); D->S.bP10 = false;
	D->Choose(TEXT("lamp"));
	Check(D->Reply && D->Reply->Text.Contains(TEXT("The lamp was on when you took it")), TEXT("an active control: the lamp was on"));
	// the saved state, with the TypeScript's field names
	const FString Json = S47J::Write(C->S.Save());
	Check(Json.StartsWith(TEXT("{\"v\":1,\"stage\":\"leave\",\"exitOpen\":false,\"arrived\":true,\"greeted\":true,\"placed\":[\"e07\",\"frame01\",\"frame02\",\"e06\"],\"shown\":true,\"p10\":true,\"wrong10\":2,\"topics\":{\"record\":true,\"tomas\":true,\"cable\":true},\"tone\":\"critical\",\"p11\":true,\"wrong11\":1,\"line\":true,\"p12\":true,\"wrong12\":0,\"told\":true,\"read\":{\"card\":false,\"letter\":false,\"correction\":false},\"seen\":{\"photo\":false,\"window\":false},\"links\":["))
		&& !Json.Contains(TEXT("\"cb\"")) && !Json.Contains(TEXT("\"mark\"")), TEXT("Ch4State in the web version's JSON"));
	const S47J::FObj Old = S47J::Read(TEXT("{\"v\":1,\"stage\":\"room\",\"exitOpen\":true,\"arrived\":true,\"greeted\":true,\"placed\":[\"frame01\"],\"shown\":false,\"p10\":false,\"wrong10\":0,\"topics\":{\"tomas\":true},\"tone\":null,\"p11\":false,\"wrong11\":0,\"line\":false,\"p12\":false,\"wrong12\":0,\"told\":false,\"read\":{\"letter\":true},\"seen\":{},\"later\":3}"));
	const FS47Ch4State L = FS47Ch4State::Load(Old);
	const FString Again = S47J::Write(L.Save());
	Check(L.Stage == TEXT("room") && L.bExitOpen && L.Topics.bTomas && !L.Topics.bRecord && L.Tone.IsEmpty() && L.Read.bLetter && !L.Read.bCard && !L.Cb && !L.Links
		&& Again.Contains(TEXT("\"tone\":null")) && Again.Contains(TEXT("\"later\":3")) && Again.Contains(TEXT("\"seen\":{\"photo\":false,\"window\":false}")), TEXT("an older save: missing fields fresh, unknown ones kept"));
	// the CB call on the way, only with the truck driven 9 s (no world: no trip, no call)
	US47Chapter4* E = NewObject<US47Chapter4>(GetTransientPackage());
	E->BuildBook(); E->bStarted = true; E->bActive = true;
	int32 Calls = 0;
	E->Cb = [&Calls](const TArray<FS47CBLine>& L3, TFunction<void()>) { ++Calls; return L3.Num() == 3; };
	E->Update(0.1, 0);
	Check(Calls == 0 && !E->S.Cb, TEXT("no CB call while nobody drives"));
	// the talk panel's helpers
	FString Quote;
	Check(FS47NpcDialogue::NoraQuote(TEXT("N. VEGA: \"Leave it. The quiet is worse.\""), Quote) && Quote == TEXT("Leave it. The quiet is worse.") && !FS47NpcDialogue::NoraQuote(TEXT("Snow on every channel."), Quote), TEXT("speakNora takes her quotes only"));
	const TArray<FS47DialogueLine> Spoken = FS47NpcDialogue::CharacterLines(TEXT("bacall"), { { TEXT("YOU"), TEXT("a") }, { TEXT("N. VEGA"), TEXT("b") }, { TEXT(""), TEXT("c") }, { TEXT("THE LINE"), TEXT("d") } });
	Check(Spoken.Num() == 2 && Spoken[1].Who == TEXT("THE LINE"), TEXT("characterLines: hers and the line's"));
	Check(S47Panels::PortraitOf(TEXT(" n. vega ")) == TEXT("portrait_nora") && S47Panels::PortraitOf(TEXT("THE LINE")).IsEmpty() && S47Panels::PortraitOf(TEXT("EARL")) == TEXT("portrait_host"), TEXT("portraits by the speaker's name"));
	TArray<FColor> Snow;
	int32 K = 0;
	US47Room6::DrawSnow(Snow, 2.0, [&K] { return (K++ % 7) / 7.0; });
	// band = (2 * 9) % 72 = 18: rows 14 to 22 are lifted by 40
	Check(Snow.Num() == 96 * 72 && Snow[18 * 96 + 1].B == 40 && Snow[0].B == 0 && Snow[1].R == FS47Canvas::Clamp8(215.0 / 7 * 0.92), TEXT("the television's snow and its rolling band"));
	C->MarkAsGarbage(); D->MarkAsGarbage(); E->MarkAsGarbage();
	return bOk;
}

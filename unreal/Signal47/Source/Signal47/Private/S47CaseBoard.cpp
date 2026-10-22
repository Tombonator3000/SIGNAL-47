#include "S47CaseBoard.h"
#include "S47Keys.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IInputProcessor.h"
#include "Internationalization/Regex.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47UI.h"
#include "SS47Board.h"

namespace
{
	// caseBoard.ts: six patches, three across and two down. The later row has room for the field book.
	constexpr double RW = 1180, RH = 760, GAP = 60, Margin = 40, LABEL = 40, MORE = 560;
	const TCHAR* const RegionIds[] = { TEXT("ns"), TEXT("ch1"), TEXT("ch2"), TEXT("ch3"), TEXT("ch4"), TEXT("ch5") };
	const TCHAR* const RegionLabels[] = { TEXT("NIGHT SHIFT"), TEXT("1  THE SECOND EXPOSURE"), TEXT("2  THE AMENDED RECORD"), TEXT("3  THE SURVEY STATION"), TEXT("4  ROOM 6"), TEXT("5  ALL NIGHT") };

	int32 RegionIndex(const FString& Id)
	{
		for (int32 I = 0; I < UE_ARRAY_COUNT(RegionIds); ++I) if (Id.Equals(RegionIds[I], ESearchCase::CaseSensitive)) return I;
		return -1;
	}

	const TMap<FString, FString> Group = {
		{ TEXT("log"), TEXT("ns") }, { TEXT("workorder"), TEXT("ns") }, { TEXT("printout"), TEXT("ns") }, { TEXT("binder"), TEXT("ns") },
		{ TEXT("service"), TEXT("ns") }, { TEXT("rfiCard"), TEXT("ns") }, { TEXT("halley"), TEXT("ns") }, { TEXT("telex"), TEXT("ns") },
		{ TEXT("s03log"), TEXT("ch1") }, { TEXT("refsheet"), TEXT("ch1") }, { TEXT("frame01"), TEXT("ch1") }, { TEXT("frame02"), TEXT("ch1") },
		{ TEXT("observation"), TEXT("ch1") }, { TEXT("finding"), TEXT("ch1") }, { TEXT("access"), TEXT("ch2") }, { TEXT("casemap"), TEXT("ch2") },
		{ TEXT("s1diagram"), TEXT("ch3") }, { TEXT("e14"), TEXT("ch5") }, { TEXT("e15"), TEXT("ch5") }, { TEXT("env4"), TEXT("ch4") },
	};

	// UI-only Slate input never reaches PlayerController's polled OnKey handlers. Forward B
	// from the notebook only; the board handles its own modal keys and walking still uses OnKey.
	class FS47BoardNotebookKeys : public IInputProcessor
	{
	public:
		explicit FS47BoardNotebookKeys(TFunction<bool()> InFn) : Fn(MoveTemp(InFn)) {}
		virtual void Tick(float, FSlateApplication&, TSharedRef<ICursor>) override {}
		virtual bool HandleKeyDownEvent(FSlateApplication&, const FKeyEvent& E) override
		{
			return S47Keys::Is(TEXT("board"), E.GetKey()) && !E.IsRepeat() && Fn && Fn();
		}
	private:
		TFunction<bool()> Fn;
	};

	FString ReplacePattern(const FString& Text, const TCHAR* Pattern, const FString& With)
	{
		FRegexMatcher Match(FRegexPattern(Pattern), Text);
		FString Out;
		int32 Last = 0;
		while (Match.FindNext())
		{
			Out += Text.Mid(Last, Match.GetMatchBeginning() - Last) + With;
			Last = Match.GetMatchEnding();
		}
		return Out + Text.Mid(Last);
	}
}

void US47CaseBoard::Init(US47Main* InM)
{
	M = InM;
	H.Docs = [this] { return M && M->Story ? M->Story->Docs() : TArray<FS47Doc>(); };
	H.Books = [this] { return M ? M->Books : TArray<FS47RuleBook*>(); };
	H.Region = [this]
	{
		// Prologue.boardRegion(): chapter six still uses chapter five's patch.
		const FString Key = M && M->Story ? M->Story->ChapterKey : FString();
		return Key == TEXT("ch6") ? FString(TEXT("ch5")) : RegionIndex(Key) >= 0 ? Key : FString(TEXT("ns"));
	};
	// The host represents chapter progression with ChapterKey (its old prologue phase stays put).
	H.Save = [this] { if (H.Region && H.Region() != TEXT("ns") && OnSave) OnSave(); };
	if (!M) return;
	TWeakObjectPtr<US47CaseBoard> Self = this;
	M->Board.Open = [Self](const FString& Place, const FString& Region) { if (Self.IsValid()) Self->Open(Place, Region); };
	M->Board.Refresh = [Self] { if (Self.IsValid()) Self->Refresh(); };
	// main.ts input.onKey(KeyB), including closing the board and opening it from the notebook.
	TFunction<bool()> KeyB = [Self]
	{
		if (!Self.IsValid() || !Self->M || !Self->M->UI || Self->M->Mode != US47Main::EMode::Play) return false;
		US47Main* Main = Self->M;
		if (Main->UI->Modal())
		{
			if (Self->View.IsValid()) { Main->UI->Close(); return true; }
			// Slate's focused widget and its parents stand in for the DOM notebook query.
			TSharedPtr<SWidget> Focus = FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetUserFocusedWidget(0) : nullptr;
			for (; Focus.IsValid(); Focus = Focus->GetParentWidget())
				if (Focus->GetTypeAsString() == TEXT("SS47Notebook")) { Self->Open(); return true; }
			return false;
		}
		if (!Main->Story || !Main->Story->IsCinematic()) { Self->Open(); return true; }
		return false;
	};
	M->OnAction(TEXT("board"), KeyB, true);
	if (M->UI && FSlateApplication::IsInitialized())
	{
		NotebookKeys = MakeShared<FS47BoardNotebookKeys>([Self, KeyB]
		{
			return Self.IsValid() && Self->M && Self->M->UI && Self->M->UI->Modal() && !Self->View.IsValid() && KeyB();
		});
		FSlateApplication::Get().RegisterInputPreProcessor(NotebookKeys);
	}
}

void US47CaseBoard::BeginDestroy()
{
	if (NotebookKeys && FSlateApplication::IsInitialized()) FSlateApplication::Get().UnregisterInputPreProcessor(NotebookKeys);
	NotebookKeys.Reset();
	Super::BeginDestroy();
}

TArray<FS47RuleBook*> US47CaseBoard::Books() const { return H.Books ? H.Books() : TArray<FS47RuleBook*>(); }

FS47BoardCard US47CaseBoard::DocCard(const FS47Doc& D, const FString& Region)
{
	// caseBoard.ts docCard(): stamp, title, first transcript line and the complete paper behind TEXT.
	const TArray<FString> Parts = S47Text::Split(D.Title, TEXT(" / "));
	const FString Short = !D.Stamp.IsEmpty() && D.Stamp.Len() <= 10 ? D.Stamp : Parts.Num() > 1 ? Parts[0] : FString();
	TArray<FString> Tail = Parts;
	if (Tail.Num() > 1) Tail.RemoveAt(0);
	const FString Title = FString::Join(Tail, TEXT(" / ")).ToUpper();
	const FString First = ReplacePattern(S47Text::Split(D.Transcript, TEXT("\n"))[0], TEXT("\\s+"), TEXT(" ")).TrimStartAndEnd();
	const FString Line = First.Len() > 120 ? ReplacePattern(First.Left(117), TEXT("[\\s.,;:]+\\S*$"), FString()) + TEXT("...") : First;
	const FString Kind = D.Kind == TEXT("photo") ? TEXT("photo") : D.Kind == TEXT("map") ? TEXT("map") : D.Id == TEXT("finding") ? TEXT("finding") : TEXT("paper");
	FString Page;
	if (D.Kind != TEXT("photo") && D.Kind != TEXT("map"))
	{
		if (!D.Sheets.IsEmpty())
		{
			TArray<FString> Pages;
			for (const FS47DocSheet& Sheet : D.Sheets) Pages.Add(Sheet.Page);
			Page = FString::Join(Pages, TEXT("\n\n"));
		}
		else Page = D.Page;
		TArray<FString> PageLines = S47Text::Split(Page, TEXT("\n"));
		for (FString& L : PageLines) if (L.StartsWith(TEXT("~ "))) L.RightChopInline(2);
		Page = FString::Join(PageLines, TEXT("\n"));
	}
	FRegexMatcher Paragraph(FRegexPattern(TEXT("\\n\\s*\\n")), D.Transcript);
	FS47BoardCard C;
	C.Id = D.Id; C.Doc = D.Id; C.Stamp = Short; C.Title = Title.Equals(Short, ESearchCase::CaseSensitive) ? FString() : Title;
	C.Kind = Kind; C.bHave = true; C.Region = Region; C.Image = D.Image; C.Text = Line;
	C.ReadingText = Paragraph.FindNext() ? D.Transcript.Left(Paragraph.GetMatchBeginning()) : D.Transcript;
	C.Full = Page + (!Page.IsEmpty() && !D.Transcript.IsEmpty() ? TEXT("\n\n") : TEXT("")) + D.Transcript;
	return C;
}

TArray<FS47BoardCard> US47CaseBoard::Cards()
{
	TArray<FS47BoardCard> Out;
	for (const FS47RuleBook* B : Books()) for (FS47BoardCard C : B->Cards())
	{
		if (C.Region.IsEmpty()) C.Region = B->Key();
		Out.Add(MoveTemp(C));
	}
	TSet<FString> Claimed;
	for (const FS47BoardCard& C : Out) { Claimed.Add(C.Id); Claimed.Add(C.Doc); }
	// RuleBook exposes question specs by id. Its current doc-backed question (P03/finding)
	// remains visible once filed; an all-question accessor is needed for future hidden doc questions.
	for (const FS47RuleBook* B : Books()) for (const FS47BoardQuestion& Q : B->Questions())
		if (const FS47BookQuestion* Spec = B->Question(Q.Id); Spec && !Spec->Doc.IsEmpty()) Claimed.Add(Spec->Doc);
	const FString Current = H.Region ? H.Region() : FString(TEXT("ns"));
	const int32 Now = RegionIndex(Current);
	const TArray<FS47Doc> Docs = H.Docs ? H.Docs() : TArray<FS47Doc>();
	const FRegexPattern Finding(TEXT("^p\\d\\d$"));
	for (const FS47Doc& D : Docs)
	{
		FRegexMatcher IsFinding(Finding, D.Id);
		if (IsFinding.FindNext() || Claimed.Contains(D.Id)) continue;
		const FString* G = Group.Find(D.Id);
		FString Region;
		if (G && !Seen.Contains(D.Id) && RegionIndex(*G) <= Now) Region = *G;
		else
		{
			if (!Seen.Contains(D.Id)) Seen.Add(D.Id, Current);
			Region = Seen[D.Id];
		}
		Out.Add(DocCard(D, Region));
	}
	CardPins.Reset();
	for (const FS47BoardCard& C : Out) if (C.bHave && !C.Lines.IsEmpty()) CardPins.Add(C.Id, C.Lines[0].Id);
	return Out;
}

TArray<US47CaseBoard::FPin> US47CaseBoard::Pins()
{
	TArray<FPin> Out;
	for (const FS47BoardCard& C : Cards())
	{
		if (!C.bHave) continue;
		if (!C.Lines.IsEmpty()) for (const FS47BoardLine& L : C.Lines) Out.Add({ L.Id, C.Id });
		else Out.Add({ C.Id, C.Id });
	}
	return Out;
}

bool US47CaseBoard::Rests(const FString& Pin) const
{
	for (const FS47RuleBook* B : Books()) if (B->Rests(Pin)) return true;
	return false;
}

FString US47CaseBoard::CardOf(const FString& Node, const TArray<FS47BoardCard>& BoardCards)
{
	for (const FS47BoardCard& C : BoardCards)
		if (C.Id == Node || C.Lines.ContainsByPredicate([&](const FS47BoardLine& L) { return L.Id == Node; })) return C.Id;
	return Node;
}

FVector2D US47CaseBoard::SizeAt(int32 Now)
{
	const int32 Cols = FMath::Min(3, Now + 1), Rows = Now >= 3 ? 2 : 1;
	return { 2 * Margin + Cols * RW + (Cols - 1) * GAP, 2 * Margin + Rows * (RH + LABEL) + (Rows - 1) * GAP + (Rows > 1 ? MORE : 0) };
}

FString US47CaseBoard::Status() const
{
	TArray<FString> OpenStamps;
	int32 Done = 0;
	for (const FS47RuleBook* B : Books()) for (const FS47BoardQuestion& Q : B->Questions())
		if (Q.Done.IsEmpty()) OpenStamps.Add(Q.Stamp); else ++Done;
	return (OpenStamps.IsEmpty() ? FString(TEXT("NOTHING OPEN")) : TEXT("OPEN: ") + FString::Join(OpenStamps, TEXT(", ")))
		+ (Done ? FString::Printf(TEXT("  /  RECORDED: %d"), Done) : FString());
}

TArray<FS47BoardRegion> US47CaseBoard::Regions(int32 Now) const
{
	TArray<FS47BoardRegion> Out;
	for (int32 I = 0; I < UE_ARRAY_COUNT(RegionIds); ++I)
		Out.Add({ RegionIds[I], I <= Now ? RegionLabels[I] : TEXT(""), Margin + (I % 3) * (RW + GAP), Margin + LABEL + (I / 3) * (RH + GAP + LABEL), RW, RH + (I >= 3 ? MORE : 0) });
	return Out;
}

TArray<FS47BoardQuestion> US47CaseBoard::Questions() const
{
	TArray<FS47BoardQuestion> Out;
	for (const FS47RuleBook* B : Books()) for (FS47BoardQuestion Q : B->Questions()) { Q.Region = B->Key(); Out.Add(MoveTemp(Q)); }
	return Out;
}

TArray<FS47BoardNote> US47CaseBoard::Notes() const
{
	TArray<FS47BoardNote> Out;
	for (const FS47RuleBook* B : Books()) for (FS47BoardNote N : B->Notes()) { N.Region = B->Key(); Out.Add(MoveTemp(N)); }
	return Out;
}

TArray<FS47BoardThread> US47CaseBoard::Threads() const
{
	TArray<FS47BoardThread> Out;
	for (const FS47RuleBook* B : Books()) for (FS47BoardThread T : B->Threads())
	{
		if (const FString* P = CardPins.Find(T.A)) T.A = *P;
		if (const FString* P = CardPins.Find(T.B)) T.B = *P;
		Out.Add(MoveTemp(T));
	}
	return Out;
}

FS47Result US47CaseBoard::Connect(const FString& A, const FString& B)
{
	if (A == B) return { false, TEXT("Pull the thread to another piece of evidence.") };
	const TArray<FS47BoardCard> BoardCards = Cards();
	const bool bSame = CardOf(A, BoardCards) == CardOf(B, BoardCards);
	const TArray<FS47RuleBook*> All = Books();
	for (int32 I = All.Num() - 1; I >= 0; --I)
		if (const TOptional<FS47Result> R = All[I]->Connect(A, B, bSame)) return *R;
	return { false, bSame ? S47Board::SAME : S47Board::EMPTY };
}

FS47Result US47CaseBoard::Record(const FString& Q)
{
	for (FS47RuleBook* B : Books()) if (const TOptional<FS47Result> R = B->Record(Q)) return *R;
	return { false, TEXT("Nothing to record.") };
}

FString US47CaseBoard::Hint(const FString& Q)
{
	for (FS47RuleBook* B : Books()) if (const TOptional<FString> R = B->Hint(Q); R && !R->IsEmpty()) return *R;
	return FString();
}

void US47CaseBoard::Open(const FString& Place, const FString& Region)
{
	if (!M || !M->UI) return;
	// Close the old modal before assigning View: HUD::SetModal calls the former close callback.
	M->UI->Close();
	for (FS47RuleBook* B : Books()) B->Backfill();
	bLayoutMoved = false;
	const FString Current = H.Region ? H.Region() : FString(TEXT("ns"));
	const int32 Now = FMath::Max(0, RegionIndex(Current));
	FS47BoardSpec O;
	O.Title = Place.IsEmpty() ? TEXT("CASE BOARD") : TEXT("CASE BOARD / ") + Place;
	O.Size = SizeAt(Now); O.Focus = Region.IsEmpty() ? Current : Region;
	O.Status = [this] { return Status(); };
	O.Regions = [this, Now] { return Regions(Now); };
	O.Cards = [this] { return Cards(); };
	O.Questions = [this] { return Questions(); };
	O.Notes = [this] { return Notes(); };
	O.Threads = [this] { return Threads(); };
	O.Connect = [this](const FString& A, const FString& B) { return Connect(A, B); };
	O.Record = [this](const FString& Q) { return Record(Q); };
	O.Hint = [this](const FString& Q) { return Hint(Q); };
	O.LayoutGet = [this](const FString& Id) { return GetPosition(Id); };
	O.LayoutSet = [this](const FString& Id, double X, double Y) { SetPosition(Id, X, Y); };
	O.OnClose = [this] { View.Reset(); if (bLayoutMoved && H.Save) H.Save(); };
	View = SNew(SS47Board).UI(M->UI).Spec(O);
	M->UI->Open(View.ToSharedRef(), O.OnClose);
	if (OnOpen) OnOpen(View.ToSharedRef());
}

void US47CaseBoard::Refresh() { if (View.IsValid() && M && M->UI && M->UI->Modal()) View->Render(); }

S47J::FObj US47CaseBoard::Save() const
{
	S47J::FObj Out = S47J::New(), Positions = S47J::New();
	for (const auto& P : Pos) S47J::Set(Positions, *P.Key, TArray<double>{ P.Value.X, P.Value.Y });
	S47J::Set(Out, TEXT("pos"), Positions);
	return Out;
}

void US47CaseBoard::Load(const S47J::FObj& Saved)
{
	Pos.Reset();
	const S47J::FObj Positions = S47J::Obj(Saved, TEXT("pos"));
	if (Positions) for (const auto& P : Positions->Values)
	{
		const TArray<TSharedPtr<FJsonValue>> XY = S47J::Arr(Positions, *P.Key);
		double X = 0, Y = 0;
		if (XY.Num() >= 2 && XY[0]->TryGetNumber(X) && XY[1]->TryGetNumber(Y) && FMath::IsFinite(X) && FMath::IsFinite(Y)) Pos.Add(FString(*P.Key), FVector2D(X, Y));
	}
	bLayoutMoved = false;
}

TOptional<FVector2D> US47CaseBoard::GetPosition(const FString& Id) const
{
	if (const FVector2D* P = Pos.Find(Id)) return *P;
	return {};
}

void US47CaseBoard::SetPosition(const FString& Id, double X, double Y)
{
	// JavaScript Math.round rounds a half towards +infinity, including negative coordinates.
	Pos.Add(Id, FVector2D(FMath::FloorToDouble(X + 0.5), FMath::FloorToDouble(Y + 0.5)));
	bLayoutMoved = true;
}

bool US47CaseBoard::SelfTest(TArray<FString>& Lines)
{
	// caseBoard.ts: two chapters share a rule; the later one answers first. No world is needed.
	bool bOk = true;
	auto Check = [&](bool bCondition, const TCHAR* What)
	{
		Lines.Add(FString::Printf(TEXT("%s caseBoard: %s"), bCondition ? TEXT("ok") : TEXT("FAIL"), What));
		bOk &= bCondition;
	};
	US47Main* Main = NewObject<US47Main>();
	US47CaseBoard* Board = NewObject<US47CaseBoard>(Main);
	Board->Init(Main);
	Check(bool(Main->Board.Open) && bool(Main->Board.Refresh) && Main->Keys.Num() == 1 && Main->Keys[0].Action == FName(TEXT("board")), TEXT("main hooks and B registered"));
	TArray<FString> OldLinks, NewLinks;
	FString OldDone, NewDone, Now = TEXT("ch1");
	int32 OldSaves = 0, NewSaves = 0;
	FS47BoardCard A; A.Id = TEXT("record"); A.Doc = TEXT("claimed"); A.bHave = true;
	A.Lines = { { TEXT("a"), TEXT("First line"), FString() }, { TEXT("c"), TEXT("Second line"), FString() } };
	FS47BoardCard B; B.Id = TEXT("b"); B.bHave = true;
	FS47BoardCard Away; Away.Id = TEXT("away");
	FS47BookSpec Old;
	Old.Key = TEXT("ch1");
	Old.Owns = [](const FString& N) { return N == TEXT("a") || N == TEXT("b") || N == TEXT("c"); };
	Old.Cards = [=] { return TArray<FS47BoardCard>{ A, B, Away }; };
	Old.Links = [&]() -> TArray<FString>& { return OldLinks; };
	Old.Save = [&] { ++OldSaves; };
	Old.Notes.Add(TEXT("n1"), { TEXT("Earlier note."), TEXT("P01"), false });
	Old.Rules = { { TEXT("a"), TEXT("b"), TEXT("n1"), TEXT("Earlier."), FString(), FString(), nullptr } };
	FS47BookQuestion Q;
	Q.Id = TEXT("P01"); Q.Stamp = TEXT("P01"); Q.Text = TEXT("First?"); Q.Doc = TEXT("finding"); Q.Slots = 1;
	Q.Hints = { TEXT("Earlier hint.") }; Q.Open = [] { return true; }; Q.Done = [&] { return OldDone; };
	Q.Record = [&] { OldDone = TEXT("Earlier finding."); return FS47Result{ true, TEXT("Earlier recorded.") }; };
	Old.Questions = { Q };
	Old.Auto = [] { return TArray<TPair<FString, FString>>{ { TEXT("record"), TEXT("b") } }; };
	FS47BookSpec Latest = Old;
	Latest.Key = TEXT("ch2"); Latest.Cards = nullptr;
	Latest.Links = [&]() -> TArray<FString>& { return NewLinks; };
	Latest.Save = [&] { ++NewSaves; };
	Latest.Notes.Reset();
	Latest.Notes.Add(TEXT("n2"), { TEXT("Latest note."), TEXT("P02"), false });
	Latest.Notes.Add(TEXT("loose"), { TEXT("Pencil word."), FString(), true });
	Latest.Rules = {
		{ TEXT("a"), TEXT("b"), TEXT("n2"), TEXT("Latest."), FString(), FString(), nullptr },
		{ TEXT("b"), TEXT("c"), TEXT("loose"), TEXT("On the thread."), FString(), FString(), nullptr },
	};
	Q.Id = TEXT("P02"); Q.Stamp = TEXT("P02"); Q.Doc.Reset(); Q.Done = [&] { return NewDone; };
	Q.Hints = { TEXT("Latest hint.") };
	Q.Record = [&] { NewDone = TEXT("Latest finding."); return FS47Result{ true, TEXT("Latest recorded.") }; };
	Latest.Questions = { Q }; Latest.Auto = nullptr;
	FS47RuleBook Earlier(Old), Later(Latest);
	Main->Books = { &Earlier, &Later };
	Board->H.Region = [&] { return Now; };
	int32 LayoutSaves = 0;
	Board->OnSave = [&] { ++LayoutSaves; };
	Now = TEXT("ns"); Board->H.Save();
	Check(LayoutSaves == 0, TEXT("layout does not save during prologue"));
	Now = TEXT("ch1"); Board->H.Save();
	Check(LayoutSaves == 1, TEXT("layout save hook uses chapter progression"));
	TArray<FS47Doc> Docs;
	for (const TCHAR* Id : { TEXT("claimed"), TEXT("finding"), TEXT("p01"), TEXT("log"), TEXT("env4"), TEXT("unknown") })
	{
		FS47Doc D; D.Id = Id; D.Title = TEXT("ID / A record"); D.Transcript = TEXT("First line\n\nSecond paragraph"); Docs.Add(D);
	}
	Board->H.Docs = [&] { return Docs; };
	TArray<FS47BoardCard> BoardCards = Board->Cards();
	Check(BoardCards.Num() == 6 && !BoardCards.ContainsByPredicate([](const FS47BoardCard& C) { return C.Id == TEXT("claimed") || C.Id == TEXT("finding") || C.Id == TEXT("p01"); }), TEXT("books and unclaimed journal docs, findings and question docs excluded"));
	Check(BoardCards[0].Region == TEXT("ch1") && BoardCards[3].Region == TEXT("ns"), TEXT("book default region and known journal group"));
	Check(BoardCards[4].Region == TEXT("ch1"), TEXT("later envelope stays where first seen"));
	Now = TEXT("ch4");
	Check(Board->Cards()[4].Region == TEXT("ch1") && Board->Cards()[5].Region == TEXT("ch1"), TEXT("seen region retained across chapters"));
	const TArray<FPin> BoardPins = Board->Pins();
	Check(BoardPins.Num() == 6 && BoardPins[0].Pin == TEXT("a") && BoardPins[1].Pin == TEXT("c") && BoardPins[1].Card == TEXT("record") && !BoardPins.ContainsByPredicate([](const FPin& P) { return P.Pin == TEXT("away"); }), TEXT("line pins, plain card pins, absent cards omitted"));
	Check(Board->Status() == TEXT("OPEN: P01, P02"), TEXT("open status in chapter order"));
	Check(Board->Rests(TEXT("a")) && !Board->Rests(TEXT("unknown")), TEXT("open questions rest on pins across books"));
	Check(Board->Connect(TEXT("a"), TEXT("a")).Text == TEXT("Pull the thread to another piece of evidence."), TEXT("identical pin reply"));
	Check(Board->Connect(TEXT("a"), TEXT("c")).Text == S47Board::SAME && OldLinks.IsEmpty() && NewLinks.IsEmpty(), TEXT("same-card detection uses line pins"));
	Check(Board->Connect(TEXT("x"), TEXT("y")).Text == S47Board::EMPTY, TEXT("unclaimed thread reply"));
	Check(!Board->Record(TEXT("P02")).bOk && Board->Record(TEXT("missing")).Text == TEXT("Nothing to record."), TEXT("record dispatch, incomplete and missing question"));
	Check(Board->Connect(TEXT("a"), TEXT("b")).Text == TEXT("Latest. P02 has what it needs: RECORD it.") && OldLinks.IsEmpty() && NewLinks.Num() == 1 && OldSaves == 0 && NewSaves == 1, TEXT("latest chapter dispatch and its save only"));
	Check(Board->Questions().Num() == 2 && Board->Questions()[1].Region == TEXT("ch2") && Board->Questions()[1].Filled == TArray<FString>{ TEXT("Latest note.") }, TEXT("questions, slots and book region aggregated"));
	Check(Board->Record(TEXT("P02")).Text == TEXT("Latest recorded.") && NewDone == TEXT("Latest finding.") && OldDone.IsEmpty(), TEXT("record dispatched to owning book"));
	Check(Board->Status() == TEXT("OPEN: P01  /  RECORDED: 1"), TEXT("mixed recorded and open status"));
	Check(Board->Hint(TEXT("P01")) == TEXT("Earlier hint.") && Board->Hint(TEXT("P02")) == TEXT("Latest hint.") && Board->Hint(TEXT("missing")).IsEmpty(), TEXT("hint dispatch"));
	Check(Board->Connect(TEXT("b"), TEXT("c")).bOk && Board->Notes().Num() == 1 && Board->Notes()[0].bHand && Board->Notes()[0].Region == TEXT("ch2"), TEXT("standalone pencil notes aggregated"));
	const TArray<FS47BoardThread> BoardThreads = Board->Threads();
	Check(BoardThreads.Num() == 3 && BoardThreads[0].A == TEXT("a") && BoardThreads[0].bAuto, TEXT("whole-card legacy thread lands on first line pin"));
	Main->Books = { &Earlier };
	Check(Board->Connect(TEXT("b"), TEXT("a")).bOk && OldLinks.Num() == 1, TEXT("earlier chapter dispatch when later book absent"));
	Check(Board->Record(TEXT("P01")).bOk && Board->Status() == TEXT("NOTHING OPEN  /  RECORDED: 1"), TEXT("all recorded status"));
	OldLinks.Reset(); Earlier.Backfill();
	Check(OldLinks.Num() == 1, TEXT("old findings backfill before board opens"));
	Main->Books.Reset();
	Check(Board->Status() == TEXT("NOTHING OPEN") && Board->Questions().IsEmpty() && Board->Notes().IsEmpty() && Board->Threads().IsEmpty(), TEXT("board works without any chapter books"));
	Check(SizeAt(0) == FVector2D(1260, 880) && SizeAt(1) == FVector2D(2500, 880) && SizeAt(2) == FVector2D(3740, 880) && SizeAt(3) == FVector2D(3740, 2300), TEXT("board grows by reached regions"));
	const TArray<FS47BoardRegion> Patches = Board->Regions(2);
	Check(Patches.Num() == 6 && Patches[0].X == 40 && Patches[0].Y == 80 && Patches[3].X == 40 && Patches[3].Y == 940 && Patches[3].H == 1320 && Patches[3].Label.IsEmpty() && Patches[2].Label == TEXT("2  THE AMENDED RECORD"), TEXT("region rects, lower row extension and unreached labels"));
	Board->SetPosition(TEXT("record"), 123.5, 99.4);
	Board->SetPosition(TEXT("note"), -2.5, -3.6);
	Check(Board->GetPosition(TEXT("record")) == TOptional<FVector2D>(FVector2D(124, 99)) && Board->GetPosition(TEXT("note")) == TOptional<FVector2D>(FVector2D(-2, -4)) && Board->bLayoutMoved, TEXT("layout rounding matches JavaScript including negative halves"));
	const S47J::FObj Saved = Board->Save();
	Check(S47J::Nums(S47J::Obj(Saved, TEXT("pos")), TEXT("record")) == TArray<double>{ 124, 99 }, TEXT("save is board.pos id -> [x,y]"));
	US47CaseBoard* Restored = NewObject<US47CaseBoard>();
	Restored->Load(S47J::Read(S47J::Write(Saved)));
	Check(Restored->GetPosition(TEXT("record")) == Board->GetPosition(TEXT("record")) && Restored->GetPosition(TEXT("note")) == Board->GetPosition(TEXT("note")) && !Restored->GetPosition(TEXT("absent")) && !Restored->bLayoutMoved, TEXT("layout JSON round trip"));
	Restored->SetPosition(TEXT("record"), 7, 8);
	Check(S47J::Nums(S47J::Obj(Saved, TEXT("pos")), TEXT("record"))[0] == 124, TEXT("save does not alias live layout"));
	Restored->Load(S47J::Read(TEXT("{\"pos\":{\"short\":[1],\"bad\":[1,\"a\",2],\"ok\":[3,4]}}")));
	Check(!Restored->GetPosition(TEXT("record")) && !Restored->GetPosition(TEXT("short")) && !Restored->GetPosition(TEXT("bad")) && Restored->GetPosition(TEXT("ok")) == TOptional<FVector2D>(FVector2D(3, 4)), TEXT("load replaces layout and ignores malformed pairs"));
	Restored->Load(nullptr);
	Check(S47J::Obj(Restored->Save(), TEXT("pos"))->Values.IsEmpty(), TEXT("old save without board resets layout"));
	FS47Doc D; D.Id = TEXT("paper"); D.Title = TEXT("ID / A / B"); D.Stamp = TEXT("STAMP"); D.Page = TEXT("typed\n~ pencil"); D.Transcript = TEXT("First  line\ncontinued\n \nSecond");
	FS47BoardCard Doc = DocCard(D, TEXT("ns"));
	Check(Doc.Stamp == TEXT("STAMP") && Doc.Title == TEXT("A / B") && Doc.Text == TEXT("First line") && Doc.ReadingText == TEXT("First  line\ncontinued") && Doc.Full == TEXT("typed\npencil\n\nFirst  line\ncontinued\n \nSecond"), TEXT("docCard title, excerpt, paragraph, pencil and full text"));
	D.Stamp = TEXT("LONGER THAN TEN"); D.Kind = TEXT("photo"); D.Image = TEXT("data:image/png;base64,AA==");
	Doc = DocCard(D, TEXT("ch1"));
	Check(Doc.Kind == TEXT("photo") && Doc.Stamp == TEXT("ID") && Doc.Full == D.Transcript && Doc.Image == D.Image, TEXT("photo cards keep URL and transcript without page"));
	D.Kind = TEXT("map"); Check(DocCard(D, TEXT("ns")).Kind == TEXT("map"), TEXT("map kind"));
	D.Kind = TEXT("typed"); D.Id = TEXT("finding"); D.Sheets = { { TEXT("one"), FString(), FString(), FString() }, { TEXT("~ two"), FString(), FString(), FString() } };
	Check(DocCard(D, TEXT("ch1")).Kind == TEXT("finding") && DocCard(D, TEXT("ch1")).Full.StartsWith(TEXT("one\n\ntwo\n\n")), TEXT("finding kind and multi-page text"));
	D.Sheets.Reset(); D.Transcript = FString::ChrN(112, TEXT('a')) + TEXT(" wordlong") + FString::ChrN(20, TEXT('b'));
	Check(DocCard(D, TEXT("ns")).Text == FString::ChrN(112, TEXT('a')) + TEXT("..."), TEXT("long first line shortened on last word boundary"));
	D.Title = TEXT("SAME"); D.Stamp = TEXT("SAME"); D.Transcript.Reset();
	Check(DocCard(D, TEXT("ns")).Title.IsEmpty() && DocCard(D, TEXT("ns")).Text.IsEmpty(), TEXT("repeated stamp suppressed and empty transcript supported"));
	bOk &= SS47Board::SelfTest(Lines);
	return bOk;
}

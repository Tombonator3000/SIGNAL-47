// story/sameThread.ts
#include "S47SameThread.h"

#include "Containers/Ticker.h"
#include "S47Docs.h"
#include "SS47Board.h"

const TCHAR* S47Same::SAME_WORD = TEXT("same");
const TCHAR* S47Same::SAME_SAY = TEXT("I don't remember drawing that.");

const TArray<FString>& S47Same::SAME_ENDS()
{
	static const TArray<FString> E = { TEXT("e11.fresh"), TEXT("e12.cut"), TEXT("s03log.heading"), TEXT("frame02.extra"), TEXT("frame01.extra"), TEXT("rx.dist"), TEXT("fb"), TEXT("@away"), TEXT("printout"), TEXT("frame02"), TEXT("frame01") };
	return E;
}

namespace
{
	const TCHAR* KEY = TEXT("same:");
	const TCHAR* SEEN = TEXT("same:seen");
}

FS47SameThread::FS47SameThread(FS47SameDeps InD) : D(MoveTemp(InD))
{
	MakeBook();
}

void FS47SameThread::MakeBook()
{
	FS47BookSpec O;
	O.Key = TEXT("same");
	O.Owns = [](const FString&) { return false; };
	O.Notes = Notes; O.Rules = Rules;
	O.Links = [this]() -> TArray<FString>& { return Links; };
	O.Wrong = [](const FString&, const FS47BookRule&) { return TOptional<FS47Result>(); };
	O.Save = [this] { if (D.Save) D.Save(); };
	O.Cards = [] { return TArray<FS47BoardCard>(); };
	Book = MakeUnique<FS47RuleBook>(MoveTemp(O));
	if (OnBook) OnBook();
}

void FS47SameThread::Update(int32 Count)
{
	while (UpTo < Count)
	{
		UpTo++;
		bool bHad = D.Seen && D.Seen->Contains(FString::Printf(TEXT("%s%d"), KEY, UpTo));
		const FString Tied = FString::Printf(TEXT("%s%d="), KEY, UpTo);
		if (D.Seen) for (const FString& K : D.Seen->Keys) bHad |= K.StartsWith(Tied);
		if (bHad) continue;
		Tie(UpTo);
	}
}

void FS47SameThread::Tie(int32 N)
{
	// ties the thread for the time away n, if the board has two ends for it (else none, for good)
	const TArray<TPair<FString, FString>> Pins = D.Pins ? D.Pins() : TArray<TPair<FString, FString>>();
	auto Has = [&Pins](const FString& P) { return Pins.ContainsByPredicate([&P](const TPair<FString, FString>& X) { return X.Key == P; }); };
	TSet<FString> Used;
	for (const FThread& T : Threads) { Used.Add(T.A); Used.Add(T.B); }
	const FString Away = FString::Printf(TEXT("away%d"), N);
	const FString A = Has(Away) ? Away : FString();
	FString B;
	for (const FString& Want : S47Same::SAME_ENDS())
	{
		FString P = Want;
		if (Want == TEXT("@away"))
		{
			P.Empty();
			for (int32 I = 0; I < N - 1; ++I) { const FString X = FString::Printf(TEXT("away%d"), N - 1 - I); if (Has(X) && !Used.Contains(X)) { P = X; break; } }
		}
		if (P.IsEmpty() || !Has(P) || Used.Contains(P) || P == A || (D.Rests && D.Rests(P))) continue;
		B = P; break;
	}
	if (A.IsEmpty() || B.IsEmpty()) { if (D.Seen) D.Seen->Add(FString::Printf(TEXT("%s%d"), KEY, N)); if (D.Save) D.Save(); return; }
	if (D.Seen) D.Seen->Add(FString::Printf(TEXT("%s%d=%s|%s"), KEY, N, *A, *B));
	Add(N, A, B);
	MakeBook();
	if (D.Save) D.Save();
}

void FS47SameThread::Add(int32 N, const FString& A, const FString& B)
{
	const FString Id = FString::Printf(TEXT("same%d"), N);
	FS47BookNote Nt; Nt.Text = S47Same::SAME_WORD; Nt.bHand = true;
	Notes.Add(Id, Nt);
	FS47BookRule R; R.A = A; R.B = B; R.Note = Id; R.Text = TEXT("Those two are already connected.");
	Rules.Add(R);
	Links.Add(S47Board::Pair(A, B));
	Threads.Add({ N, A, B });
}

void FS47SameThread::Opened(const TSharedRef<SS47Board>& View, TFunction<bool()> Connected)
{
	if (Noticed() || !Threads.Num()) return;
	const FThread T = Threads[0];
	if (!View->Frame({ T.A, T.B, FString::Printf(TEXT("same%d"), T.N) })) return;
	if (D.Seen) D.Seen->Add(SEEN);
	// (setTimeout, 1100 ms: real time, the board open over the game)
	TWeakPtr<SS47Board> W = View;
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([W, Connected](float)
	{
		const TSharedPtr<SS47Board> V = W.Pin();
		if (V.IsValid() && (!Connected || Connected())) V->Say(FString(S47Same::SAME_SAY));
		return false;
	}), 1.1f);
	const TArray<TPair<FString, FString>> Cards = D.Pins ? D.Pins() : TArray<TPair<FString, FString>>();
	auto Name = [&Cards](const FString& P) { const TPair<FString, FString>* C = Cards.FindByPredicate([&P](const TPair<FString, FString>& X) { return X.Key == P; }); return C ? C->Value : P; };
	if (D.Note) D.Note(FString::Printf(TEXT("%s. On the case board, a thread I did not tie: from the photograph I did not take to %s. On it, in pencil, in my own hand: \"%s\". %s"),
		*S47Text::ClockText(D.Clock ? D.Clock() : 0, false), *EndName(Name(T.B)), S47Same::SAME_WORD, S47Same::SAME_SAY));
	if (D.Saw) D.Saw();
	if (D.Save) D.Save();
}

FString FS47SameThread::EndName(const FString& Card)
{
	if (Card.StartsWith(TEXT("away"))) return TEXT("the one from the time before");
	static const TMap<FString, FString> N = { { TEXT("e11"), TEXT("his last field card") }, { TEXT("e12"), TEXT("her correction") }, { TEXT("s03log"), TEXT("S-03's motor bus, 026") },
		{ TEXT("frame01"), TEXT("FRAME 01") }, { TEXT("frame02"), TEXT("FRAME 02") }, { TEXT("rx"), TEXT("the printout, -39 LY") }, { TEXT("printout"), TEXT("the printout, -39 LY") }, { TEXT("fb"), TEXT("T. Vega's field book") } };
	const FString* F = N.Find(Card);
	return F ? *F : FString(TEXT("another card"));
}

void FS47SameThread::Reset(int32 Count)
{
	Threads.Reset(); Rules.Reset(); Links.Reset(); Notes.Reset();
	if (D.Seen)
	{
		for (const FString& K : D.Seen->Keys)
		{
			// /^same:(\d+)=([^|]+)\|(.+)$/
			if (!K.StartsWith(KEY)) continue;
			const FString Rest = K.Mid(5);
			int32 Eq = -1, Bar = -1;
			if (!Rest.FindChar(TEXT('='), Eq) || Eq == 0) continue;
			bool bDigits = true;
			for (int32 i = 0; i < Eq; ++i) bDigits &= FChar::IsDigit(Rest[i]);
			if (!bDigits) continue;
			const FString Ends = Rest.Mid(Eq + 1);
			if (!Ends.FindChar(TEXT('|'), Bar) || Bar == 0 || Bar == Ends.Len() - 1) continue;
			Add(FCString::Atoi(*Rest.Left(Eq)), Ends.Left(Bar), Ends.Mid(Bar + 1));
		}
	}
	Threads.StableSort([](const FThread& X, const FThread& Y) { return X.N < Y.N; });
	UpTo = Count;
	MakeBook();
}

bool FS47SameThread::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s same thread: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	FS47TownSeen Seen;
	TArray<TPair<FString, FString>> Pins = { { TEXT("printout"), TEXT("printout") }, { TEXT("rx.dist"), TEXT("rx") } };
	TArray<FString> Notes;
	FS47SameDeps Dp;
	Dp.Seen = &Seen;
	Dp.Pins = [&Pins] { return Pins; };
	Dp.Rests = [](const FString& P) { return P == TEXT("rx.dist"); };
	Dp.Note = [&Notes](const FString& N) { Notes.Add(N); };
	FS47SameThread S(Dp);
	S.Reset(0);
	// the first time away, its photograph not on the board: none, for good
	S.Update(1);
	Check(S.Threads.Num() == 0 && Seen.Contains(TEXT("same:1")), TEXT("no photograph from above: no thread, for good"));
	// the second, with it: to the first end the board has that no open question rests on
	Pins.Add({ TEXT("away2"), TEXT("away2") });
	S.Update(2);
	Check(S.Threads.Num() == 1 && S.Threads[0].A == TEXT("away2") && S.Threads[0].B == TEXT("printout") && Seen.Contains(TEXT("same:2=away2|printout")), TEXT("tied from away2 to the printout, past the pin a question rests on"));
	const TArray<FS47BoardThread> Th = S.Book->Threads();
	const TArray<FS47BoardNote> No = S.Book->Notes();
	Check(Th.Num() == 1 && No.Num() == 1 && No[0].Text == TEXT("same") && No[0].bHand, TEXT("on the board, with \"same\" in the player's hand"));
	// a case opened: the threads as it has them
	FS47SameThread R(Dp);
	R.Reset(2);
	Check(R.Threads.Num() == 1 && R.Threads[0].N == 2 && R.Threads[0].B == TEXT("printout"), TEXT("a case opened keeps its threads"));
	Check(EndName(TEXT("printout")) == TEXT("the printout, -39 LY") && EndName(TEXT("away1")) == TEXT("the one from the time before"), TEXT("the other end, in Reyes's words"));
	return bOk;
}

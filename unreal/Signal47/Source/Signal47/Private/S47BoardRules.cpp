#include "S47BoardRules.h"

const TCHAR* S47Board::SAME = TEXT("Both lines are on the same record. Compare it with another one.");
const TCHAR* S47Board::EMPTY = TEXT("Those two do not say anything about each other.");

FString S47Board::Pair(const FString& A, const FString& B)
{
	// [a, b].sort().join('|'): JavaScript's default sort compares UTF-16 code units
	return A.Compare(B, ESearchCase::CaseSensitive) <= 0 ? A + TEXT("|") + B : B + TEXT("|") + A;
}

const FS47BookRule* FS47RuleBook::RuleOf(const FString& K) const
{
	return O.Rules.FindByPredicate([&](const FS47BookRule& R) { return S47Board::Pair(R.A, R.B) == K; });
}

const FS47BookQuestion* FS47RuleBook::Question(const FString& Id) const
{
	return O.Questions.FindByPredicate([&](const FS47BookQuestion& Q) { return Q.Id == Id; });
}

FString FS47RuleBook::QuestionOf(const FString& A, const FString& B) const
{
	if (O.QuestionOf) return O.QuestionOf(A, B);
	const FS47BookRule* R = RuleOf(S47Board::Pair(A, B));
	FString Q;
	if (R && !R->Wrong.IsEmpty()) Q = R->Wrong;
	else if (R && !R->Note.IsEmpty()) { if (const FS47BookNote* N = O.Notes.Find(R->Note)) Q = N->Q; }
	if (!Q.IsEmpty()) return Q;
	for (const FS47BookQuestion& X : O.Questions) if (X.Open && X.Open() && (!X.Done || X.Done().IsEmpty())) return X.Id;
	return FString();
}

FS47Result FS47RuleBook::Miss(const FString& A, const FString& B, const FS47Result& R)
{
	const FString Q = QuestionOf(A, B);
	if (!Q.IsEmpty()) MissSince.FindOrAdd(Q) += 1;
	return R;
}

TOptional<FS47Result> FS47RuleBook::Connect(const FString& A, const FString& B, bool bSameCard)
{
	const FString K = S47Board::Pair(A, B);
	const FS47BookRule* R = RuleOf(K);
	if (!R && !(Owns(A) && Owns(B))) return {};
	if (bSameCard) return Miss(A, B, { false, S47Board::SAME });
	TArray<FString>& Links = O.Links();
	if (Links.Contains(K)) return FS47Result{ true, TEXT("Those two are already connected.") };
	if (!R) return Miss(A, B, { false, O.Empty.IsEmpty() ? FString(S47Board::EMPTY) : O.Empty });
	if (!R->Wrong.IsEmpty())
	{
		const TOptional<FS47Result> Reply = O.Wrong ? O.Wrong(R->Wrong, *R) : TOptional<FS47Result>();
		if (O.Save) O.Save();
		if (Reply && Reply->bOk) return *Reply;
		return Miss(A, B, Reply ? *Reply : FS47Result{ false, R->Now() });
	}
	FString Q;
	if (!R->Note.IsEmpty()) { if (const FS47BookNote* N = O.Notes.Find(R->Note)) Q = N->Q; }
	if (!R->Group.IsEmpty())
	{
		const FS47BookQuestion* QQ = Q.IsEmpty() ? nullptr : Question(Q);
		const FString Done = QQ && QQ->Done ? QQ->Done() : FString();
		if (!Done.IsEmpty()) return FS47Result{ true, Done };
		for (int32 i = Links.Num() - 1; i >= 0; --i)
		{
			const FS47BookRule* L = RuleOf(Links[i]);
			if (L && L->Group == R->Group) Links.RemoveAt(i);
		}
	}
	Links.Add(K);
	if (O.Save) O.Save();
	const FString RText = R->Now();
	if (Q.IsEmpty()) return FS47Result{ true, RText };
	const int32 NFilled = Filled(Q).Num();
	const FS47BookQuestion* QQ = Question(Q);
	const int32 Total = QQ ? QQ->Slots : NFilled;
	return FS47Result{ true, NFilled >= Total ? FString::Printf(TEXT("%s %s has what it needs: RECORD it."), *RText, *Q) : FString::Printf(TEXT("%s (%s: %d of %d)"), *RText, *Q, NFilled, Total) };
}

TArray<FString> FS47RuleBook::Filled(const FString& Q) const
{
	TArray<FString> Ids;
	for (const FString& K : O.Links())
	{
		const FS47BookRule* R = RuleOf(K);
		if (!R || R->Note.IsEmpty()) continue;
		const FS47BookNote* N = O.Notes.Find(R->Note);
		if (N && N->Q == Q && !Ids.Contains(R->Note)) Ids.Add(R->Note);
	}
	TArray<FString> Out;
	for (const FString& Id : Ids) Out.Add(O.Notes[Id].Now());
	return Out;
}

TArray<FS47BoardThread> FS47RuleBook::Threads() const
{
	TArray<FS47BoardThread> Out;
	for (const FString& K : O.Links())
	{
		if (const FS47BookRule* R = RuleOf(K)) Out.Add({ R->A, R->B, R->Note.IsEmpty(), false });
	}
	if (O.Auto) for (const TPair<FString, FString>& T : O.Auto()) Out.Add({ T.Key, T.Value, true, true });
	return Out;
}

TArray<FS47BoardNote> FS47RuleBook::Notes() const
{
	TArray<FS47BoardNote> Out;
	for (const FString& K : O.Links())
	{
		const FS47BookRule* R = RuleOf(K);
		if (!R || R->Note.IsEmpty()) continue;
		const FS47BookNote* N = O.Notes.Find(R->Note);
		if (!N || !N->Q.IsEmpty() || Out.ContainsByPredicate([&](const FS47BoardNote& X) { return X.Id == R->Note; })) continue;
		FS47BoardNote BN;
		BN.Id = R->Note; BN.Text = N->Now(); BN.FromA = R->A; BN.FromB = R->B; BN.bHand = N->bHand;
		Out.Add(BN);
	}
	return Out;
}

TArray<FS47BoardQuestion> FS47RuleBook::Questions() const
{
	TArray<FS47BoardQuestion> Out;
	for (const FS47BookQuestion& Q : O.Questions)
	{
		if (!Q.Open || !Q.Open()) continue;
		FS47BoardQuestion B;
		B.Id = Q.Id; B.Stamp = Q.Stamp; B.Text = Q.Text; B.Slots = Q.Slots; B.Filled = Filled(Q.Id);
		B.Done = Q.Done ? Q.Done() : FString(); B.Elsewhere = Q.ElsewhereNow(); B.X = Q.X; B.Y = Q.Y; B.W = Q.W;
		Out.Add(B);
	}
	return Out;
}

TOptional<FS47Result> FS47RuleBook::Record(const FString& Id)
{
	const FS47BookQuestion* Q = Question(Id);
	if (!Q) return {};
	if (!Q->Record) return FS47Result{ false, Q->ElsewhereNow().IsEmpty() ? FString(TEXT("This one is not recorded on the board.")) : Q->ElsewhereNow() };
	const int32 Have = Filled(Id).Num();
	if (Have < Q->Slots) return FS47Result{ false, FString::Printf(TEXT("%s needs %d more on it first."), *Id, Q->Slots - Have) };
	return Q->Record();
}

TOptional<FString> FS47RuleBook::Hint(const FString& Id)
{
	// Three hint levels per question; a fourth, if there is one, names the threads and only comes
	// after the third has been read and two more threads have missed.
	const FS47BookQuestion* Q = Question(Id);
	if (!Q || Q->Hints.Num() == 0) return {};
	const TArray<FString>& List = Q->Hints;
	const int32 N = HintSeen.FindRef(Id);
	if (N >= 3 && MissSince.FindRef(Id) < 2) return List[FMath::Min(2, List.Num() - 1)];
	HintSeen.Add(Id, FMath::Min(N + 1, List.Num()));
	if (HintSeen[Id] == 3) MissSince.Add(Id, 0);
	return List[FMath::Min(N, List.Num() - 1)];
}

void FS47RuleBook::Backfill()
{
	// Saves from before the board recorded findings without threads: lay the threads that fill
	// each recorded question out again.
	TArray<FString>& Links = O.Links();
	if (O.Legacy) for (int32 i = Links.Num() - 1; i >= 0; --i) if (O.Legacy(Links[i])) Links.RemoveAt(i);
	for (const FS47BookQuestion& Q : O.Questions)
	{
		if (!Q.Slots || !Q.Done || Q.Done().IsEmpty()) continue;
		TSet<FString> Have;
		for (const FString& K : Links) if (const FS47BookRule* R = RuleOf(K); R && !R->Note.IsEmpty()) Have.Add(R->Note);
		for (const auto& KV : O.Notes)
		{
			if (KV.Value.Q != Q.Id || Have.Contains(KV.Key)) continue;
			const FS47BookRule* R = O.Rules.FindByPredicate([&](const FS47BookRule& X) { return X.Note == KV.Key && X.Wrong.IsEmpty() && (X.Restore ? X.Restore() : true); });
			if (R) { Links.Add(S47Board::Pair(R->A, R->B)); Have.Add(KV.Key); }
		}
	}
}

bool FS47RuleBook::Rests(const FString& Pin) const
{
	for (const FS47BookRule& R : O.Rules)
	{
		if (R.A != Pin && R.B != Pin) continue;
		FString Q = R.Wrong;
		if (Q.IsEmpty() && !R.Note.IsEmpty()) if (const FS47BookNote* N = O.Notes.Find(R.Note)) Q = N->Q;
		const FS47BookQuestion* QQ = Q.IsEmpty() ? nullptr : Question(Q);
		if (QQ && QQ->Open && QQ->Open() && (!QQ->Done || QQ->Done().IsEmpty())) return true;
	}
	return false;
}

bool FS47RuleBook::SelfTest(TArray<FString>& Lines)
{
	// a small book: two notes for P01 (2 slots), a wrong rule, a group
	TArray<FString> Links;
	FString Recorded;
	FS47BookSpec S;
	S.Key = TEXT("t");
	S.Owns = [](const FString& N) { return N.StartsWith(TEXT("t.")); };
	S.Notes.Add(TEXT("n1"), { TEXT("One."), TEXT("P01"), false });
	S.Notes.Add(TEXT("n2"), { TEXT("Two."), TEXT("P01"), false });
	S.Rules = {
		{ TEXT("t.a"), TEXT("t.b"), TEXT("n1"), TEXT("A and B."), FString(), FString(), nullptr },
		{ TEXT("t.c"), TEXT("t.b"), TEXT("n2"), TEXT("B and C."), FString(), FString(), nullptr },
		{ TEXT("t.a"), TEXT("t.d"), FString(), TEXT("Not so."), TEXT("P01"), FString(), nullptr },
	};
	FS47BookQuestion Q;
	Q.Id = TEXT("P01"); Q.Stamp = TEXT("P01"); Q.Text = TEXT("?"); Q.Slots = 2; Q.Hints = { TEXT("h1"), TEXT("h2"), TEXT("h3"), TEXT("h4") };
	Q.Open = [] { return true; };
	Q.Done = [&Recorded] { return Recorded; };
	Q.Record = [&Recorded] { Recorded = TEXT("Done."); return FS47Result{ true, TEXT("Recorded.") }; };
	S.Questions = { Q };
	S.Links = [&Links]() -> TArray<FString>& { return Links; };
	S.Save = [] {};
	FS47RuleBook B(S);
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s board: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bCond; };
	Check(!B.Connect(TEXT("x.a"), TEXT("x.b"), false).IsSet(), TEXT("not this chapter's pins"));
	Check(B.Connect(TEXT("t.a"), TEXT("t.b"), true)->Text == S47Board::SAME, TEXT("same card"));
	Check(B.Connect(TEXT("t.b"), TEXT("t.a"), false)->Text == TEXT("A and B. (P01: 1 of 2)"), TEXT("first note"));
	Check(B.Connect(TEXT("t.a"), TEXT("t.b"), false)->Text == TEXT("Those two are already connected."), TEXT("already"));
	Check(!B.Connect(TEXT("t.a"), TEXT("t.d"), false)->bOk, TEXT("wrong rule"));
	Check(B.Record(TEXT("P01"))->Text == TEXT("P01 needs 1 more on it first."), TEXT("record too early"));
	Check(B.Connect(TEXT("t.c"), TEXT("t.b"), false)->Text == TEXT("B and C. P01 has what it needs: RECORD it."), TEXT("second note"));
	Check(B.Filled(TEXT("P01")).Num() == 2 && B.Threads().Num() == 2, TEXT("filled and threads"));
	Check(B.Record(TEXT("P01"))->bOk && B.Questions()[0].Done == TEXT("Done."), TEXT("record"));
	Check(*B.Hint(TEXT("P01")) == TEXT("h1") && *B.Hint(TEXT("P01")) == TEXT("h2") && *B.Hint(TEXT("P01")) == TEXT("h3") && *B.Hint(TEXT("P01")) == TEXT("h3"), TEXT("hints wait for misses"));
	Check(S47Board::Pair(TEXT("b"), TEXT("a")) == TEXT("a|b") && S47Board::Pair(TEXT("B"), TEXT("a")) == TEXT("B|a"), TEXT("pair sorts as JavaScript"));
	return bOk;
}

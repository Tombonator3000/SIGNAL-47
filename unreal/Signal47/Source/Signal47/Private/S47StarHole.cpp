// story/starHole.ts: no stars where the dishes point
#include "S47StarHole.h"

#include "S47Docs.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47StarHole, Log, All);

const TCHAR* FS47StarHole::HOLE_PRINT = TEXT("No stars where the dishes point.");
const TCHAR* FS47StarHole::HOLE_TOAST = TEXT("There are no stars where the dishes point.");

namespace
{
	const TCHAR* HOLE_LOOK = TEXT("No stars in it. Not one, and the sky behind it is the same colour as the sky round it.");
	const FString KEY = TEXT("sky:hole"), LINK = TEXT("sh:");
	const TArray<FString> CHAPTERS = { TEXT("ch1"), TEXT("ch2"), TEXT("ch3"), TEXT("ch4"), TEXT("ch5") };
	double JsRound(double V) { return FMath::FloorToDouble(V + 0.5); }
	double AngleDeg(const FVector& A, const FVector& B)
	{
		const double D = FMath::Sqrt(A.SizeSquared() * B.SizeSquared());
		return FMath::RadiansToDegrees(D == 0 ? UE_DOUBLE_HALF_PI : FMath::Acos(FMath::Clamp(FVector::DotProduct(A, B) / D, -1.0, 1.0)));
	}

	// ---------- the board ----------
	TMap<FString, FS47BookNote> NOTES()
	{
		TMap<FString, FS47BookNote> M;
		FS47BookNote A; A.Text = TEXT("S-03 has read 026 since the turn, with no command. All twenty-seven are pointed at the one place in the sky with no stars in it."); M.Add(TEXT("n.hole.s03"), A);
		FS47BookNote B; B.Text = TEXT("The herd stands facing 026 with their heads up. The way the dishes point."); M.Add(TEXT("n.hole.cows"), B);
		return M;
	}
	TArray<FS47BookRule> RULES()
	{
		const TMap<FString, FS47BookNote> N = NOTES();
		TArray<FS47BookRule> R;
		FS47BookRule A; A.A = TEXT("hole.sky"); A.B = TEXT("s03log.heading"); A.Note = TEXT("n.hole.s03"); A.Text = N[TEXT("n.hole.s03")].Text; R.Add(A);
		// tonight's herd, from T. Vega's field book when it is on the board (story/fieldBook.ts)
		FS47BookRule B; B.A = TEXT("hole.sky"); B.B = TEXT("tn.cows"); B.Note = TEXT("n.hole.cows"); B.Text = N[TEXT("n.hole.cows")].Text; R.Add(B);
		return R;
	}
}

double FS47StarHole::At(double Clock, TOptional<double> From)
{
	const double S = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
	if (S > 12 * 3600 || !From || !FMath::IsFinite(*From)) return 0;
	const double K = FMath::Clamp((S - *From) / STAR_HOLE.Fade, 0.0, 1.0);
	return K * K * (3 - 2 * K);
}

FVector FS47StarHole::SkyDir(double Az, double El)
{
	const double A = FMath::DegreesToRadians(Az), E = FMath::DegreesToRadians(El);
	return FVector(FMath::Sin(A) * FMath::Cos(E), FMath::Sin(E), -FMath::Cos(A) * FMath::Cos(E));
}

FS47StarHole::FS47StarHole(FS47StarHoleDeps InD) : D(MoveTemp(InD))
{
	MakeBook();
}

void FS47StarHole::MakeBook()
{
	// (its card and its notes lie in the patch of cork of the part of the night it was noticed in: the
	// book's key is asked each time in the web version, a getter; here the book is made again when it changes)
	BookKey = Region();
	FS47BookSpec O;
	O.Key = BookKey;
	O.Owns = [](const FString& N) { return N == TEXT("hole") || N.StartsWith(TEXT("hole.")); };
	O.Notes = NOTES(); O.Rules = RULES();
	O.Links = [this]() -> TArray<FString>& { return Links; };
	O.Wrong = [](const FString&, const FS47BookRule&) { return TOptional<FS47Result>(); };
	O.Save = [this] { KeepLinks(); if (D.Save) D.Save(); };
	O.Cards = [this] { return Cards(); };
	O.Empty = TEXT("Not that one. What else tonight is on 026?");
	// (the one before stays alive: the board may still hold it until the books are gathered again)
	if (Book) Retired.Add(MoveTemp(Book));
	Book = MakeUnique<FS47RuleBook>(MoveTemp(O));
}

bool FS47StarHole::Noticed() const { return D.Seen && D.Seen->Contains(KEY); }

TOptional<double> FS47StarHole::When() const
{
	if (D.Seen) for (const FString& K : D.Seen->Keys) if (K.StartsWith(KEY + TEXT("@"))) return FCString::Atod(*K.Mid(KEY.Len() + 1));
	return {};
}

FString FS47StarHole::PrintId() const
{
	if (D.Seen) for (const FString& K : D.Seen->Keys) if (K.StartsWith(KEY + TEXT(":print@"))) return K.Mid(KEY.Len() + 7);
	return FString();
}

FString FS47StarHole::Region() const
{
	if (D.Seen) for (const FString& K : D.Seen->Keys) if (K.StartsWith(KEY + TEXT("/"))) { const FString C = K.Mid(KEY.Len() + 1); if (CHAPTERS.Contains(C)) return C; }
	return TEXT("ch1");
}

TArray<FS47RuleBook*> FS47StarHole::Books()
{
	if (!Noticed()) return {};
	if (Region() != BookKey) MakeBook();
	return { Book.Get() };
}

void FS47StarHole::Update(double Dt, const FS47StarHoleIn& S)
{
	if (Noticed() || bOff || !S.bOk || !S.bFree || !S.bOutdoors || !S.bNight || S.Hole < 0.9) { Look = 0; return; }
	const double Off = AngleDeg(S.Cam.Forward, S.Dir);
	const bool bZoom = S.bZoom && Off < NOTICE.ZoomDeg;
	// (a moment's slip off it does not start the look over)
	Look = bZoom || Off < NOTICE.Deg ? Look + Dt * (bZoom ? NOTICE.Secs / NOTICE.ZoomSecs : 1) : FMath::Max(0.0, Look - Dt * 2);
	if (Look < NOTICE.Secs) return;
	if (S.Clear && !S.Clear()) { Look = NOTICE.Secs * 0.5; return; }
	Notice(false);
}

FString FS47StarHole::Examine(const FS47StarHoleIn& S) const
{
	if (!S.bOutdoors || !S.bNight || S.Hole < 0.5) return FString();
	const double Off = AngleDeg(S.Cam.Forward, S.Dir);
	return Off < NOTICE.ZoomDeg && (!S.Clear || S.Clear()) ? FString(HOLE_LOOK) : FString();
}

void FS47StarHole::Photographed(const FString& ShotId, bool bFree)
{
	if (PrintId().IsEmpty() && D.Seen) D.Seen->Add(KEY + TEXT(":print@") + ShotId);
	if (!Noticed() && !bOff && bFree) Notice(true);
	else { if (D.Refresh) D.Refresh(); if (D.Save) D.Save(); }
}

void FS47StarHole::Notice(bool bViaPrint)
{
	if (!D.Seen) return;
	const double Clock = D.Clock ? D.Clock() : 0;
	const FString T = S47Text::ClockText(Clock, false);
	D.Seen->Add(KEY);
	D.Seen->Add(FString::Printf(TEXT("%s@%d"), *KEY, int32(JsRound(Clock))));
	const FString Ch = D.Chapter ? D.Chapter() : FString();
	if (CHAPTERS.Contains(Ch)) D.Seen->Add(KEY + TEXT("/") + Ch);
	Look = 0;
	if (D.Toast) D.Toast(bViaPrint ? FString(TEXT("In the viewfinder, up where the dishes point: no stars.")) : FString(HOLE_TOAST), 5);
	if (D.Note) D.Note(FString::Printf(TEXT("%s. There are no stars where the dishes point. Az 026, el 32: a round patch about eight degrees across, nearly a fist at arm's length, with nothing in it. Not a cloud. The sky is the same colour all the way through it. Only the stars are gone, as if something were standing in front of them."), *T));
	if (D.Saw) D.Saw();
	if (D.Refresh) D.Refresh();
	if (D.Save) D.Save();
	UE_LOG(LogS47StarHole, Display, TEXT("S47: no stars at 026: noticed%s"), bViaPrint ? TEXT(" (on the print)") : TEXT(""));
}

TArray<FS47BoardCard> FS47StarHole::Cards() const
{
	if (!Noticed()) return {};
	const TOptional<double> W = When();
	const FString Id = PrintId(), Url = !Id.IsEmpty() && D.Print ? D.Print(Id) : FString();
	FS47BoardCard C;
	C.Id = TEXT("hole"); C.Stamp = TEXT("AZ 026 EL 32"); C.Title = TEXT("NO STARS AT 026"); C.Kind = Url.IsEmpty() ? TEXT("thing") : TEXT("photo"); C.bHave = true; C.W = Url.IsEmpty() ? 300 : 260;
	C.Image = Url;
	C.Text = (W ? FString::Printf(TEXT("%s, looking"), *S47Text::ClockText(*W, false)) : FString(TEXT("Looking"))) + TEXT(" up along the dishes: a round patch of sky with no stars in it.");
	FS47BoardLine L; L.Id = TEXT("hole.sky"); L.Text = TEXT("Az 026, el 32. Eight degrees across, no stars, the sky the same colour through it.");
	C.Lines = { L };
	return { C };
}

void FS47StarHole::KeepLinks()
{
	if (!D.Seen) return;
	for (const FString& K : D.Seen->Array()) if (K.StartsWith(LINK)) D.Seen->Remove(K);
	for (const FString& K : Links) D.Seen->Add(LINK + K);
}

void FS47StarHole::Reset()
{
	Look = 0;
	Links.Reset();
	if (D.Seen) for (const FString& K : D.Seen->Array())
	{
		if (!K.StartsWith(LINK)) continue;
		const FString Pk = K.Mid(LINK.Len());
		if (RULES().ContainsByPredicate([&Pk](const FS47BookRule& R) { return S47Board::Pair(R.A, R.B) == Pk; })) Links.Add(Pk);
	}
	MakeBook();
}

void FS47StarHole::Forget()
{
	if (D.Seen) for (const FString& K : D.Seen->Array())
	{
		if (K == KEY || K.StartsWith(KEY + TEXT("@")) || K.StartsWith(KEY + TEXT("/")) || K.StartsWith(KEY + TEXT(":")) || K.StartsWith(LINK)) D.Seen->Remove(K);
	}
	Reset();
}

bool FS47StarHole::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s star hole: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	const FVector H = SkyDir(26, 32);
	Check(FVector::Dist(H, FVector(0.3717598164443862, 0.5299192642332049, -0.7622205798007392)) < 1e-9, TEXT("where the dishes point, as the web world's sky has it"));
	const double From = 2 * 3600 + 15 * 60 + 12 + STAR_HOLE.After;
	Check(At(From - 1, From) == 0 && At(From + 50, From) == 0.5 && At(From + 100, From) == 1 && At(20 * 3600, From) == 0 && At(3 * 3600, {}) == 0, TEXT("it comes in over 100 s after 02:15:22"));
	FS47TownSeen Seen;
	TArray<FString> Notes;
	int32 Saws = 0;
	FS47StarHoleDeps D;
	D.Seen = &Seen; D.Note = [&Notes](const FString& T) { Notes.Add(T); }; D.Clock = [] { return 3 * 3600.0; }; D.Chapter = [] { return FString(TEXT("ch2")); }; D.Saw = [&Saws] { Saws++; };
	FS47StarHole S(D);
	FS47StarHoleIn In;
	In.bOk = true; In.bFree = true; In.bOutdoors = true; In.bNight = true; In.Hole = 1; In.Dir = H;
	In.Cam.Forward = H;
	for (int32 i = 0; i < 29; ++i) S.Update(0.1, In);
	Check(!S.Noticed() && S.Look > 2.8, TEXT("looked at, not yet three seconds"));
	S.Update(0.2, In);
	Check(S.Noticed() && Seen.Contains(TEXT("sky:hole/ch2")) && S.When() == 10800.0 && Saws == 1 && Notes.Num() == 1 && S.Books().Num() == 1 && S.Book->Key() == TEXT("ch2"), TEXT("noticed: the notebook, the log, its card on chapter two's cork"));
	Check(S.Examine(In) == HOLE_LOOK, TEXT("a closer look at it"));
	S.Forget();
	Check(!S.Noticed() && Seen.Num() == 0, TEXT("forgotten"));
	return bOk;
}

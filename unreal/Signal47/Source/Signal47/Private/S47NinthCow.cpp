// story/ninthCow.ts
#include "S47NinthCow.h"

#include "S47Docs.h"
#include "S47NinthModel.h"

#include <cmath>

const TCHAR* S47Ninth::NINTH_TAG = TEXT("ALDERETE 25");
const TCHAR* S47Ninth::NINTH_PRINT = TEXT("The ninth one from the pasture, inside the fence. Not a drop of blood.");

namespace
{
	constexpr double QUIET = 26, LOOK = 0.5, CLOSE = 3.4, TICK_R = 45, TICK_SAID = 28;
	const TCHAR* LINK = TEXT("nl:");
	const TCHAR* PARTS[3] = { TEXT("middle"), TEXT("head"), TEXT("body") };
}

FS47NinthCow::FS47NinthCow(FS47NinthDeps InD) : D(MoveTemp(InD))
{
	FS47BookNote Ring; Ring.Text = TEXT("Her tag is the herd's. She is the one from the ring of frost in the pasture, six hundred metres east, and nothing walked her here.");
	Notes.Add(TEXT("n.nine.ring"), Ring);
	FS47BookNote Fb; Fb.Text = TEXT("Jul 3, 1947: nine head. Jul 4: eight. He counted them too, and never wrote down where the ninth went.");
	Notes.Add(TEXT("n.nine.fb"), Fb);
	auto Rule = [&](const TCHAR* A, const TCHAR* B, const TCHAR* Note)
	{
		FS47BookRule R; R.A = A; R.B = B; R.Note = Note; R.Text = Notes[Note].Text; Rules.Add(R);
	};
	Rule(TEXT("nine.tag"), TEXT("nine.ring"), TEXT("n.nine.ring"));
	Rule(TEXT("nine.tag"), TEXT("tn.cows"), TEXT("n.nine.ring"));
	Rule(TEXT("nine.frost"), TEXT("fb.cows"), TEXT("n.nine.fb"));
	Rule(TEXT("nine.tag"), TEXT("fb.cows"), TEXT("n.nine.fb"));
	FS47BookSpec O;
	O.Key = TEXT("ch3");
	O.Owns = [](const FString& N) { return N == TEXT("nine") || N.StartsWith(TEXT("nine.")); };
	O.Notes = Notes; O.Rules = Rules;
	O.Links = [this]() -> TArray<FString>& { return Links; };
	O.Wrong = [](const FString&, const FS47BookRule&) { return TOptional<FS47Result>(); };
	O.Save = [this] { KeepLinks(); if (D.Save) D.Save(); };
	O.Cards = [this] { return Cards(); };
	O.Empty = TEXT("Not that one. What on her says where she came from?");
	Book = MakeUnique<FS47RuleBook>(MoveTemp(O));
}

bool FS47NinthCow::Down() const { return Has(TEXT("nine:down")); }
bool FS47NinthCow::Found() const { return Has(TEXT("nine:found")); }
bool FS47NinthCow::Close() const { return Has(TEXT("nine:close")); }
TArray<FS47RuleBook*> FS47NinthCow::Books() const { return Found() ? TArray<FS47RuleBook*>{ Book.Get() } : TArray<FS47RuleBook*>(); }

void FS47NinthCow::Update(double Dt, const FS47NinthIn& S)
{
	Quiet = FMath::Max(0.0, Quiet - Dt);
	FS47NinthModel* Site = D.Site ? D.Site() : nullptr;
	if (!Site) return;
	if (Down() != Site->bNinthDown) Site->SetNinth(Down());
	bInHut = S.bInHut;
	if (!Down()) { bCanFind = false; PutDown(*Site, S); return; }
	bCanFind = S.Area == TEXT("station01") && !S.bDriving && S.bOk && S.bFree && !S.bInHut;
	if (!bCanFind) { Look = 0; return; }
	const FVector Mid = Site->NinthAt(TEXT("middle")), Cam = S.Cam.Pos;
	const double Dist = FVector::Dist(Cam, Mid);
	if (Found())
	{
		bool bFacing = false;
		for (const TCHAR* Part : PARTS) if (S47Facing(S.Cam, Site->NinthAt(Part), 8, 40)) { bFacing = true; break; }
		if (!Close() && FMath::Min(FVector::Dist(Cam, Site->NinthAt(TEXT("head"))), FVector::Dist(Cam, Site->NinthAt(TEXT("body")))) < CLOSE
			&& bFacing && InSight(*Site, Cam)) CloseTo();
		Look = 0; return;
	}
	if (Dist < TICK_R) Ticking(Dt, *Site, Dist);
	const bool bSee = (Dist < 7 ? S47Facing(S.Cam, Mid, 7, 50) : S.bLight && Dist < 24 && S47Facing(S.Cam, Mid, 24, 22)) && InSight(*Site, Cam);
	Look = bSee ? Look + Dt : 0;
	if (Look >= LOOK) Find();
}

void FS47NinthCow::Ticking(double Dt, FS47NinthModel& Site, double Dist)
{
	TickT -= Dt;
	if (TickT > 0) return;
	TickT = FMath::FRand() < 0.25 ? 0.15 + FMath::FRand() * 0.1 : 1.3 + FMath::FRand() * 2.6;
	if (D.Tick) D.Tick(Site.NinthAt(TEXT("body")));
	if (Dist >= TICK_SAID) return;
	Ticks++;
	if (Ticks == 3 && !Has(TEXT("nine:tick")))
	{
		if (D.Seen) D.Seen->Add(TEXT("nine:tick"));
		if (D.Toast) D.Toast(TEXT("Somewhere in the dark inside the fence, a small, sharp tick. Then another, the way an engine ticks as it cools."), 5);
	}
}

bool FS47NinthCow::InSight(const FS47NinthModel& Site, const FVector& From) const
{
	if (Site.Occluders.IsEmpty()) return true;
	for (const TCHAR* Part : PARTS)
	{
		const FVector To = Site.NinthAt(Part);
		const double Len = FVector::Dist(To, From);
		if (Len < 0.4) return true;
		if (!Site.Occluded(From, To, 0.05, Len - 0.3)) return true;
	}
	return false;
}

void FS47NinthCow::PutDown(FS47NinthModel& Site, const FS47NinthIn& S)
{
	if (bOff) return;
	if (!(bForce || S.Chapter == TEXT("ch3")) || S.Area != TEXT("station01") || !S.bFree || !S.bOk) return;
	if (!Unseen(Site, S)) return;
	if (!D.TakeNinth || !D.TakeNinth()) return;
	bForce = false;
	if (D.Seen) D.Seen->Add(TEXT("nine:down"));
	Site.SetNinth(true);
	if (D.Save) D.Save();
}

bool FS47NinthCow::Unseen(const FS47NinthModel& Site, const FS47NinthIn& S) const
{
	if (S.bInHut) return true;
	const FVector Mid = Site.NinthAt(TEXT("middle")), Cam = S.Cam.Pos;
	const double Dist = FVector::Dist(Cam, Mid);
	if (Dist > 110) return true;
	FVector F = S.Cam.Forward, To = Mid - Cam; F.Y = 0; To.Y = 0;
	const double Den = F.Size() * To.Size();
	// three's Vector3.angleTo gives PI/2 when either vector is zero.
	const double Angle = Den == 0 ? UE_DOUBLE_PI / 2 : FMath::Acos(FMath::Clamp(FVector::DotProduct(F, To) / Den, -1.0, 1.0));
	return Dist > 6 && Angle > FMath::DegreesToRadians(78.0);
}

void FS47NinthCow::Find()
{
	if (Found()) return;
	const FString T = S47Text::ClockText(Clock(), false);
	if (D.Seen) { D.Seen->Add(TEXT("nine:found")); D.Seen->Add(FString::Printf(TEXT("mark:ninth@%.0f"), std::floor(Clock() + 0.5))); }
	Look = 0; Quiet = QUIET;
	if (D.Hush) D.Hush(QUIET);
	const bool bTicked = Ticks >= 2 || Has(TEXT("nine:tick"));
	if (D.Toast) D.Toast(FString(TEXT("Inside the fence, on the path out to the B vane: a cow, on her side in a patch of frost. She does not move.")) + (bTicked ? TEXT(" The ticking has stopped.") : TEXT("")), 5.5);
	const FString Ring = Has(TEXT("ev:cows")) ? TEXT(" She is the one missing from the ring of frost in the pasture.") : Has(TEXT("ev:cows1")) ? TEXT(" There were nine in the pasture by the highway tonight.") : TEXT("");
	if (D.Note) D.Note(T + TEXT(". STATION 01, inside the fence: a Hereford cow, dead on her side on B, between the transit and the comparison vane, her head to 026, in a ring of frost.")
		+ (bTicked ? TEXT(" Something out here was ticking like an engine cooling. It stopped when I found her.") : TEXT("")) + TEXT(" The vehicle gate is chained.") + Ring);
	if (D.Refresh) D.Refresh();
	if (D.Save) D.Save();
}

void FS47NinthCow::CloseTo()
{
	if (D.Seen) D.Seen->Add(TEXT("nine:close"));
	if (Quiet < 10) { Quiet = 10; if (D.Hush) D.Hush(10); }
	if (D.Toast) D.Toast(TEXT("No blood. Not on her, not in the frost under her. The jaw bare to the bone, the tongue gone, a round piece of her side cut out clean. The edges are dark and hard."), 7);
	if (D.Note) D.Note(S47Text::ClockText(Clock(), false) + TEXT(". Close to: no blood anywhere, on her or under her. Cut out clean: an oval of the jaw to the bone, the tongue, a round piece of her side, the edges dark and hard. No tracks in the frost round her."));
	if (D.Save) D.Save();
}

FString FS47NinthCow::Examine(const FString& Part, const FS47Cam& Cam)
{
	FS47NinthModel* Site = D.Site ? D.Site() : nullptr;
	if (!Site || !Site->bNinthDown) return FString();
	if (bInHut || !InSight(*Site, Cam.Pos)) return FString();
	if (!Found()) { if (!bCanFind) return FString(); Find(); }
	const double Dist = FVector::Dist(Cam.Pos, Site->NinthAt(Part == TEXT("head") ? TEXT("head") : TEXT("middle")));
	if (Dist > 7) return Part == TEXT("frost") ? TEXT("White frost on the ground, in a ring round something lying there.") : TEXT("A cow, on her side in the frost. She does not move. Nothing near her moves.");
	if (Part == TEXT("head"))
	{
		if (!Has(TEXT("nine:tag"))) TagNote();
		return FString::Printf(TEXT("Frost on her open eye. An oval of hide is gone from the jaw, down to the bone, and the mouth hangs open on nothing: the tongue is gone. A yellow tag in her ear: %s."), S47Ninth::NINTH_TAG);
	}
	if (Part == TEXT("body")) return TEXT("A round piece cut out of her side, clean as a chassis punch. The edges are dark and hard, as if burnt. Not a drop of blood, on her or under her.");
	return TEXT("The frost round her is unbroken. No tracks in it, hers or anyone's. The grass under her is not pressed down.");
}

void FS47NinthCow::TagNote()
{
	if (D.Seen) D.Seen->Add(TEXT("nine:tag"));
	const FString Ring = Has(TEXT("ev:cows")) ? TEXT(" She is the ninth.") : TEXT("");
	if (D.Note) D.Note(FString::Printf(TEXT("%s. Her ear tag: %s, yellow. Alderete runs the herd in the pasture by the highway, six hundred metres east of here.%s"), *S47Text::ClockText(Clock(), false), S47Ninth::NINTH_TAG, *Ring));
	if (D.Save) D.Save();
}

FString FS47NinthCow::Label() const
{
	const FS47NinthModel* Site = D.Site ? D.Site() : nullptr;
	return Site && Site->bNinthDown && Found() ? TEXT("Dead cow") : FString();
}

void FS47NinthCow::Use()
{
	if (Has(TEXT("nine:touch"))) { if (D.Toast) D.Toast(TEXT("Still cold. The frost on her has not changed."), 3); return; }
	if (D.Seen) D.Seen->Add(TEXT("nine:touch"));
	if (D.Toast) D.Toast(TEXT("You put your hand on her flank. Cold, colder than the ground, and hard. The frost does not melt under your hand. Over her a smell like the racks at SARO after an arc: hot metal."), 6.5);
	if (D.Note) D.Note(S47Text::ClockText(Clock(), false) + TEXT(". She is cold through, colder than the ground, and the frost on her does not melt under my hand. A smell over her like the racks after an arc. Hot metal, out here."));
	if (D.Save) D.Save();
}

FString FS47NinthCow::When() const
{
	if (D.Seen) for (const FString& K : D.Seen->Keys) if (K.StartsWith(TEXT("mark:ninth@"))) return S47Text::ClockText(FCString::Atod(*K.Mid(11)), false);
	return FString();
}

TArray<FS47BoardCard> FS47NinthCow::Cards() const
{
	if (!Found()) return {};
	const FString T = When(); const bool bRing = Has(TEXT("ev:cows"));
	FS47BoardCard Cow;
	Cow.Id = TEXT("nine"); Cow.Stamp = TEXT("STATION 01"); Cow.Title = TEXT("THE NINTH, ON B"); Cow.Kind = TEXT("thing"); Cow.bHave = true; Cow.X = 790; Cow.Y = 1020; Cow.W = 360;
	Cow.Text = FString(TEXT("Inside the fence at STATION 01")) + (T.IsEmpty() ? FString() : TEXT(", ") + T) + TEXT(": a cow from the pasture, dead on her side in the frost. No blood.");
	Cow.Lines = {
		{ TEXT("nine.tag"), FString::Printf(TEXT("Her ear tag: %s, yellow. The herd by the highway."), S47Ninth::NINTH_TAG), FString() },
		{ TEXT("nine.cut"), TEXT("Cut out clean: the jaw to the bone, the tongue, a piece of her side. No blood."), FString() },
		{ TEXT("nine.frost"), TEXT("On B, her head to 026, in a ring of frost. No tracks to her."), FString() },
	};
	FS47BoardCard Ring;
	Ring.Id = TEXT("nine.ring"); Ring.Stamp = TEXT("PASTURE"); Ring.Title = TEXT("THE RING OF FROST"); Ring.Kind = TEXT("thing"); Ring.bHave = bRing; Ring.X = 1000; Ring.Y = 790; Ring.W = 170;
	Ring.Where = bRing ? TEXT("") : TEXT("The pasture by the highway, where she stood.");
	Ring.Text = TEXT("The herd facing 026. Eight. Where the ninth stood, flattened grass in a ring, white with frost.");
	return { Cow, Ring };
}

void FS47NinthCow::KeepLinks()
{
	if (!D.Seen) return;
	for (const FString& K : D.Seen->Array()) if (K.StartsWith(LINK)) D.Seen->Remove(K);
	for (const FString& K : Links) D.Seen->Add(FString(LINK) + K);
}

void FS47NinthCow::Reset()
{
	Quiet = 0; Look = 0; bForce = false; TickT = 2; Ticks = 0;
	Links.Empty();
	if (D.Seen) for (const FString& K : D.Seen->Keys) if (K.StartsWith(LINK))
	{
		const FString Pair = K.Mid(FCString::Strlen(LINK));
		if (Rules.ContainsByPredicate([&Pair](const FS47BookRule& R) { return S47Board::Pair(R.A, R.B) == Pair; })) Links.Add(Pair);
	}
	if (D.Site) if (FS47NinthModel* Site = D.Site()) Site->SetNinth(Down());
}

bool FS47NinthCow::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool b, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s ninth cow: %s"), b ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= b; };
	FS47TownSeen Seen; FS47NinthModel Site(nullptr);
	int32 Saved = 0, Noted = 0; double Hushed = 0;
	FS47NinthDeps D; D.Seen = &Seen; D.Site = [&] { return &Site; }; D.TakeNinth = [] { return true; };
	D.Clock = [] { return 3 * 3600 + 47; }; D.Hush = [&](double Secs) { Hushed = Secs; }; D.Save = [&] { Saved++; }; D.Note = [&](const FString&) { Noted++; };
	FS47NinthCow N(MoveTemp(D));
	Check(N.State() == TEXT("not yet") && N.Books().IsEmpty(), TEXT("not present before the night puts her down"));
	FS47NinthIn S; S.bOk = true; S.bFree = true; S.Chapter = TEXT("ch3"); S.Area = TEXT("station01"); S.bInHut = true;
	N.Update(0.1, S);
	Check(N.Down() && Site.bNinthDown && Saved == 1 && !N.Found(), TEXT("put down unseen in the hut, not found through its walls"));
	S.bInHut = false; S.Cam.Pos = FVector(0, 0, 5); S.Cam.Forward = FVector(0, 0, -1);
	N.Update(0.49, S); Check(!N.Found(), TEXT("looking 0.49 seconds is not enough"));
	N.Update(0.01, S); Check(N.Found() && N.Quiet == 26 && Hushed == 26 && N.Books().Num() == 1, TEXT("half-second find, exact 26 seconds of quiet and board card"));
	S.Cam.Pos = FVector(0, 0, 2); N.Update(25, S);
	Check(N.Close() && N.Quiet == 10 && Hushed == 10, TEXT("close detail extends the remaining quiet to ten seconds"));
	Check(N.Examine(TEXT("head"), S.Cam).Contains(TEXT("ALDERETE 25")) && Seen.Contains(TEXT("nine:tag")), TEXT("closer look reads the same ranch and missing tag"));
	N.Use(); const int32 Once = Noted; N.Use(); Check(Noted == Once && Seen.Contains(TEXT("nine:touch")), TEXT("touch is recorded once"));
	Seen.Add(TEXT("nl:fb.cows|nine.tag")); Seen.Add(TEXT("nl:invalid|pair")); N.Reset();
	Check(N.Quiet == 0 && N.Links.Num() == 1 && N.Down() && N.Found(), TEXT("restore keeps known board threads and findings, clears transient quiet"));
	const TArray<FS47BoardCard> Cards = N.Book->Cards();
	Check(Cards.Num() == 2 && Cards[0].X.GetValue() == 790 && !Cards[1].bHave && Cards[0].Lines.Num() == 3, TEXT("same two cards, pins, coordinates and uncollected ring"));
	return bOk;
}

#include "S47HudGauges.h"
#include "S47Json.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include <limits>

namespace
{
	// a steady little pseudo-random number from 0 to 1 for a whole number (no state, the same every time)
	double Hash01(int32 N)
	{
		uint32 X = uint32(N) * 2654435761u;
		X ^= X >> 15; X *= 2246822519u; X ^= X >> 13;
		return double(X & 0xffffu) / 65535.0;
	}
}

int32 S47HudGauges::Strokes(double Level)
{
	if (!FMath::IsFinite(Level) || Level <= 0) return 0;
	return FMath::Clamp(FMath::CeilToInt(Level * BatteryStrokes - 1e-9), 1, BatteryStrokes);
}

bool S47HudGauges::BatteryShown(bool bDrains, bool bHave, bool bOn, double Level, double Low)
{
	return bDrains && bHave && (bOn || Level < Low);
}

FString S47HudGauges::CbLine() { return TEXT("CB 19"); }
FString S47HudGauges::WalkieLine(int32 Channel, const FString& Who)
{
	const FString Line = FString::Printf(TEXT("WALKIE %d"), FMath::Clamp(Channel, 1, 3));
	const FString Name = Who.TrimStartAndEnd().ToUpper();
	return Name.IsEmpty() ? Line : Line + TEXT(" \u00B7 ") + Name;
}

bool S47HudGauges::WalkieSpeaker(const FString& Head, int32& Channel, FString& Who)
{
	const FString HeadUp = Head.TrimStartAndEnd().ToUpper();
	if (!HeadUp.StartsWith(TEXT("WALKIE "))) return false;
	FString Set, Name;
	if (!HeadUp.Split(TEXT(","), &Set, &Name)) return false;
	Name.TrimStartAndEndInline();
	const FString Digits = Set.Mid(7).TrimStartAndEnd();
	if (Name.IsEmpty() || Digits.IsEmpty() || !Digits.IsNumeric()) return false;
	Channel = FMath::Clamp(FCString::Atoi(*Digits), 1, 3);
	Who = Name;
	return true;
}

int32 S47HudGauges::RadioKind(bool bCbBusy, bool bCbAtHand, bool bWalkieOwned, bool bHailing, bool bKeyed, double SpeechLeft)
{
	if (bCbBusy && bCbAtHand) return 1;
	if (bWalkieOwned && (bHailing || bKeyed || (FMath::IsFinite(SpeechLeft) && SpeechLeft > 0))) return 2;
	return 0;
}

FString S47HudGauges::WalkieWho(bool bHailing, bool bKeyed, const FString& Speaker)
{
	if (bKeyed) return TEXT("YOU");
	if (bHailing) return TEXT("DALE");
	return Speaker;
}

double S47HudGauges::CbStrength(double T)
{
	return FMath::Clamp(0.82 + 0.1 * FMath::Sin(T * 1.7) + 0.05 * FMath::Sin(T * 4.3), 0.0, 1.0);
}

double S47HudGauges::Wave(double X, double Phase, double Strength)
{
	const double K = FMath::Clamp(Strength, 0.0, 1.0);
	const double Ends = FMath::Sin(UE_DOUBLE_PI * FMath::Clamp(X, 0.0, 1.0));
	const double Clean = FMath::Sin(X * UE_DOUBLE_PI * 2 * 2.5 - Phase);
	// the weaker the signal, the more the line is torn: a different jitter every tenth of the phase
	const double Torn = (Hash01(FMath::FloorToInt(X * 18) + 131 * FMath::FloorToInt(Phase * 2.5)) - 0.5) * 2;
	return FMath::Clamp((Clean * (0.25 + 0.75 * K) + Torn * (1 - K) * 0.6) * Ends * (0.2 + 0.8 * K), -1.0, 1.0);
}

bool S47HudGauges::FiledRefused(const FString& Stamp)
{
	return Stamp.Contains(TEXT("NOT FILED"));
}

FString S47HudGauges::FiledLine(const FString& Clock)
{
	const FString C = Clock.TrimStartAndEnd();
	return C.IsEmpty() ? FString(TEXT("LOG FILED")) : TEXT("LOG FILED  ") + C;
}

double S47HudGauges::FiledAlpha(double T)
{
	if (!FMath::IsFinite(T) || T < 0 || T >= FiledFor) return 0;
	if (T < 0.25) return T / 0.25;
	return FMath::Clamp((FiledFor - T) / 0.8, 0.0, 1.0);
}

// ---------- HUD D (HUD-D-20261109) ----------

const S47HudGauges::FHudPalette& S47HudGauges::Palette(EHudColor Color)
{
	// V4 (Tom, 8 November): the clock and the battery one teal VFD, the battery dimmed to the clock; low in the game's Neon at full
	// strength; the radio line amber. The others are one colour throughout, each with a low colour well away from it.
	static const FHudPalette Palettes[HudColors] = {
		{ TEXT("teal"), TEXT("Station teal"), TEXT("The station's own: the clock and the battery in a cool teal, like the console's vacuum-fluorescent displays, and the radio line in amber. A low battery turns red."),
			0x78ffde, 150.f / 255.f, 14.f / 255.f, 0x78ffde, 0.5f, 0.55f, 0.14f, 0xff5a45, 0.7f, 1.f, 0.2f, 0xffb052, 225.f / 255.f, 215.f / 255.f },
		{ TEXT("amber"), TEXT("Amber"), TEXT("Everything in warm amber, the clock too, like an old amber terminal. A low battery turns a hard red."),
			0xffb052, 150.f / 255.f, 14.f / 255.f, 0xffb052, 0.6f, 0.7f, 0.18f, 0xff3a2e, 0.7f, 1.f, 0.2f, 0xffb052, 225.f / 255.f, 215.f / 255.f },
		{ TEXT("green"), TEXT("Phosphor green"), TEXT("Everything in the green of a phosphor screen. A low battery turns red."),
			0x8cffa4, 150.f / 255.f, 14.f / 255.f, 0x8cffa4, 0.5f, 0.55f, 0.14f, 0xff5a45, 0.7f, 1.f, 0.2f, 0x8cffa4, 215.f / 255.f, 205.f / 255.f },
		{ TEXT("white"), TEXT("Paper white"), TEXT("Everything in a soft paper white, the plainest to read. A low battery turns red."),
			0xebe3cf, 130.f / 255.f, 12.f / 255.f, 0xebe3cf, 0.45f, 0.5f, 0.12f, 0xff5a45, 0.7f, 1.f, 0.2f, 0xebe3cf, 205.f / 255.f, 195.f / 255.f },
		{ TEXT("red"), TEXT("Red night"), TEXT("Everything in a deep red that leaves your eyes used to the dark, as an observer's red lamp does. A low battery turns pale yellow, so it still stands out."),
			0xff3a2e, 170.f / 255.f, 16.f / 255.f, 0xff3a2e, 0.55f, 0.65f, 0.16f, 0xffd27a, 0.7f, 1.f, 0.2f, 0xff3a2e, 225.f / 255.f, 215.f / 255.f },
	};
	const int32 I = int32(Color);
	return Palettes[I >= 0 && I < HudColors ? I : 0];
}

namespace S47HudGaugesChoice
{
	// the chosen palette, for the session (US47Flow sets it from the store and from Settings)
	static S47HudGauges::EHudColor& Chosen() { static S47HudGauges::EHudColor Value = S47HudGauges::EHudColor::StationTeal; return Value; }
}

const S47HudGauges::FHudPalette& S47HudGauges::Palette() { return Palette(S47HudGaugesChoice::Chosen()); }
S47HudGauges::EHudColor S47HudGauges::HudColor() { return S47HudGaugesChoice::Chosen(); }
void S47HudGauges::SetHudColor(EHudColor Color) { const int32 I = int32(Color); S47HudGaugesChoice::Chosen() = I >= 0 && I < HudColors ? Color : EHudColor::StationTeal; }
FString S47HudGauges::HudColorId(EHudColor Color) { return Palette(Color).Id; }

S47HudGauges::EHudColor S47HudGauges::HudColorFrom(const FString& Id)
{
	const FString Want = Id.TrimStartAndEnd().ToLower();
	for (int32 I = 0; I < HudColors; ++I)
		if (Want == Palette(EHudColor(I)).Id) return EHudColor(I);
	return EHudColor::StationTeal;
}

double S47HudGauges::ColorDistance(uint32 A, uint32 B)
{
	double Sum = 0;
	for (int32 Shift : { 16, 8, 0 })
	{
		const double Step = (double((A >> Shift) & 255u) - double((B >> Shift) & 255u)) / 255.0;
		Sum += Step * Step;
	}
	return FMath::Sqrt(Sum);
}

S47HudGauges::FClockFace S47HudGauges::ClockFace(const FString& Watch)
{
	FClockFace Face;
	const FString W = Watch.TrimStartAndEnd();
	if (W == TEXT("88:88")) { Face.bValid = Face.bAll = true; return Face; }
	TArray<FString> Pieces;
	W.ParseIntoArray(Pieces, TEXT(":"), false);
	if (Pieces.Num() < 2 || Pieces.Num() > 3) return Face;
	int32 Nums[3] = { 0, 0, -1 };
	for (int32 I = 0; I < Pieces.Num(); ++I)
	{
		if (Pieces[I].IsEmpty() || Pieces[I].Len() > 2) return Face;
		for (const TCHAR Ch : Pieces[I]) if (!FChar::IsDigit(Ch)) return Face;
		Nums[I] = FCString::Atoi(*Pieces[I]);
	}
	if (Nums[0] > 23 || Nums[1] > 59 || Nums[2] > 59 || (Pieces.Num() == 3 && Pieces[2].Len() != 2) || Pieces[1].Len() != 2) return Face;
	Face.bValid = true;
	Face.bPm = Nums[0] >= 12;
	Face.Hour = Nums[0] % 12 == 0 ? 12 : Nums[0] % 12;
	Face.Minute = Nums[1];
	Face.Second = Nums[2];
	return Face;
}

uint8 S47HudGauges::Segments(int32 Digit)
{
	static const uint8 Lit[10] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f };
	return Digit >= 0 && Digit <= 9 ? Lit[Digit] : 0;
}

double S47HudGauges::NoteAlpha(double T)
{
	if (!FMath::IsFinite(T) || T < 0) return 0;
	if (T < NoteIn) return FMath::SmoothStep(0.0, 1.0, T / NoteIn);
	if (T < NoteHold) return 1;
	if (T < NoteHold + NoteFade) return 1.0 - FMath::SmoothStep(0.0, 1.0, (T - NoteHold) / NoteFade);
	return 0;
}

FString S47HudGauges::Handwritten(const FString& Text, bool bTitle)
{
	int32 Upper = 0, Lower = 0;
	for (const TCHAR Ch : Text) { Upper += FChar::IsUpper(Ch) ? 1 : 0; Lower += FChar::IsLower(Ch) ? 1 : 0; }
	if (Lower > Upper) return Text;
	// the capitals the hand keeps (the station's, the units, the bands) and the names it writes with a capital
	struct FKept { const TCHAR* Up; const TCHAR* As; };
	static const FKept Kept[] = {
		{ TEXT("SARO"), TEXT("SARO") }, { TEXT("CB"), TEXT("CB") }, { TEXT("MHZ"), TEXT("MHz") }, { TEXT("KHZ"), TEXT("kHz") }, { TEXT("GHZ"), TEXT("GHz") },
		{ TEXT("AM"), TEXT("AM") }, { TEXT("FM"), TEXT("FM") }, { TEXT("UFO"), TEXT("UFO") }, { TEXT("VLA"), TEXT("VLA") }, { TEXT("TV"), TEXT("TV") },
		{ TEXT("USA"), TEXT("USA") }, { TEXT("OK"), TEXT("OK") }, { TEXT("I"), TEXT("I") }, { TEXT("KOB"), TEXT("KOB") }, { TEXT("NASA"), TEXT("NASA") },
		{ TEXT("FAA"), TEXT("FAA") }, { TEXT("HQ"), TEXT("HQ") }, { TEXT("DALE"), TEXT("Dale") }, { TEXT("WARD"), TEXT("Ward") }, { TEXT("EARL"), TEXT("Earl") },
		{ TEXT("NORA"), TEXT("Nora") }, { TEXT("VEGA"), TEXT("Vega") }, { TEXT("CALICHE"), TEXT("Caliche") }, { TEXT("ROSWELL"), TEXT("Roswell") },
		{ TEXT("SOCORRO"), TEXT("Socorro") }, { TEXT("MAGDALENA"), TEXT("Magdalena") }, { TEXT("ALBUQUERQUE"), TEXT("Albuquerque") }, { TEXT("DATIL"), TEXT("Datil") },
		{ TEXT("MEXICO"), TEXT("Mexico") }, { TEXT("APRIL"), TEXT("April") },
		// M7-HUD-20261109: the room's own capitals, and the doctor's title
		{ TEXT("OPS"), TEXT("OPS") }, { TEXT("PC"), TEXT("PC") }, { TEXT("DR"), TEXT("Dr") },
	};
	static const TCHAR* Small[] = { TEXT("a"), TEXT("an"), TEXT("and"), TEXT("the"), TEXT("of"), TEXT("to"), TEXT("in"), TEXT("on"), TEXT("at"), TEXT("by"), TEXT("for"), TEXT("or") };
	FString Out;
	Out.Reserve(Text.Len());
	bool bSentence = true, bFirstWord = true;
	int32 I = 0;
	while (I < Text.Len())
	{
		const TCHAR Ch = Text[I];
		if (!FChar::IsAlnum(Ch))
		{
			Out.AppendChar(Ch);
			if (Ch == TEXT('.') || Ch == TEXT('!') || Ch == TEXT('?')) bSentence = true;
			++I;
			continue;
		}
		int32 J = I;
		bool bFigure = false;
		while (J < Text.Len() && FChar::IsAlnum(Text[J])) { bFigure |= FChar::IsDigit(Text[J]); ++J; }
		const FString Word = Text.Mid(I, J - I);
		const bool bAfterApostrophe = I > 0 && Text[I - 1] == TEXT('\'');
		FString Written;
		// (an antenna's or a tape's name, "S-13", "C-60": its letters stay as they are)
		const bool bLabel = Word.Len() <= 2 && J + 1 < Text.Len() && Text[J] == TEXT('-') && FChar::IsDigit(Text[J + 1]);
		if (bFigure || bLabel) Written = Word;
		else
		{
			const FString Up = Word.ToUpper();
			for (const FKept& K : Kept) if (Up == K.Up) { Written = K.As; break; }
			if (Written.IsEmpty())
			{
				Written = Word.ToLower();
				bool bSmall = false;
				for (const TCHAR* S : Small) bSmall |= Written == S;
				const bool bCapital = !bAfterApostrophe && ((bTitle && (bFirstWord || !bSmall)) || bSentence);
				if (bCapital) Written[0] = FChar::ToUpper(Written[0]);
			}
		}
		Out += Written;
		bSentence = false;
		bFirstWord = false;
		I = J;
	}
	return Out;
}

FString S47HudGauges::CardLine(const FString& Task, int32 Most)
{
	FString T = Task.TrimStartAndEnd();
	if (T.IsEmpty()) return T;
	Most = FMath::Max(Most, 12);
	// the screens and tables as the room's labels have them, without the way to them
	static const TCHAR* Shorter[][2] = {
		{ TEXT("TRACKING (right-hand PC)"), TEXT("TRACKING PC") }, { TEXT("RECEIVER (centre PC)"), TEXT("RECEIVER PC") },
		{ TEXT("OPS (corner PC)"), TEXT("OPS") }, { TEXT("REEL DECK (wall table)"), TEXT("REEL DECK") },
		{ TEXT("ARRAY STATUS (left-hand PC)"), TEXT("ARRAY STATUS") }, { TEXT(" (corner PC)"), TEXT("") }, { TEXT(" (right-hand PC)"), TEXT("") },
		{ TEXT(" (centre PC)"), TEXT("") }, { TEXT(" (left-hand PC)"), TEXT("") }, { TEXT(" (wall table)"), TEXT("") },
	};
	for (const auto& S : Shorter) T.ReplaceInline(S[0], S[1], ESearchCase::CaseSensitive);
	while (T.ReplaceInline(TEXT("  "), TEXT(" ")) > 0) {}
	FString Head, Body;
	if (!T.Split(TEXT(" // "), &Head, &Body))
	{
		// the place: what comes before the first " / " or ": ", if it comes early enough to be one
		const int32 Slash = T.Find(TEXT(" / ")), Colon = T.Find(TEXT(": "));
		int32 Cut = INDEX_NONE, Skip = 0;
		if (Slash >= 0 && (Colon < 0 || Slash < Colon)) { Cut = Slash; Skip = 3; }
		else if (Colon >= 0) { Cut = Colon; Skip = 2; }
		if (Cut > 0 && Cut <= 28) { Head = T.Left(Cut).TrimStartAndEnd(); Body = T.Mid(Cut + Skip).TrimStartAndEnd(); }
		else Body = T;
	}
	Head.TrimStartAndEndInline();
	Body.TrimStartAndEndInline();
	if (Head.Len() > 28) { Body = Head + TEXT(": ") + Body; Head.Reset(); }
	// a step of something ("FIRST SHIFT 2/5") with a place of its own under it: the place takes the head, with the step's count,
	// when the whole would not fit ("OPS 1/5 // OPEN DAILY WORK ...")
	if (!Head.IsEmpty() && Head.Len() + 4 + Body.Len() > Most)
	{
		int32 Space = INDEX_NONE;
		const FString Count = Head.FindLastChar(TEXT(' '), Space) ? Head.Mid(Space + 1) : FString();
		const int32 Colon = Body.Find(TEXT(": "));
		if (Count.Contains(TEXT("/")) && Count.Len() <= 5 && FChar::IsDigit(Count[0]) && Colon > 0 && Colon <= 20 && !Body.Left(Colon).Contains(TEXT(".")))
		{
			Head = Body.Left(Colon) + TEXT(" ") + Count;
			Body = Body.Mid(Colon + 2).TrimStartAndEnd();
		}
	}
	const FString Joint = Head.IsEmpty() ? FString() : Head + TEXT(" // ");
	const int32 Room = Most - Joint.Len();
	// the task's sentences (and its clauses after a semicolon), a doctor's "DR." not counted as an end
	TArray<FString> Said;
	{
		FString Piece;
		for (int32 I = 0; I < Body.Len(); ++I)
		{
			const TCHAR Ch = Body[I];
			Piece.AppendChar(Ch);
			const bool bEnd = (Ch == TEXT('.') || Ch == TEXT(';') || Ch == TEXT('!') || Ch == TEXT('?')) && (I + 1 == Body.Len() || Body[I + 1] == TEXT(' '));
			const FString Lower = Piece.ToLower();
			const bool bTitle = Ch == TEXT('.') && (Lower.EndsWith(TEXT(" dr.")) || Lower == TEXT("dr.") || Lower.EndsWith(TEXT(" mr.")) || Lower.EndsWith(TEXT(" st.")));
			if (bEnd && !bTitle)
			{
				FString Done = Piece.TrimStartAndEnd();
				if (Done.EndsWith(TEXT(";"))) { Done.LeftChopInline(1); Done += TEXT("."); }
				if (!Done.IsEmpty()) Said.Add(Done);
				Piece.Reset();
			}
		}
		Piece.TrimStartAndEndInline();
		if (!Piece.IsEmpty()) Said.Add(Piece);
	}
	// a sentence cut at a word to fit In characters, and saying so
	auto Cut = [](const FString& S, int32 In)
	{
		FString C = S.Left(FMath::Max(0, In - 3));
		int32 Space = INDEX_NONE;
		if (C.Len() < S.Len() && C.FindLastChar(TEXT(' '), Space) && Space > 0) C.LeftInline(Space);
		while (!C.IsEmpty() && (C.EndsWith(TEXT(",")) || C.EndsWith(TEXT(":")) || C.EndsWith(TEXT(";")) || C.EndsWith(TEXT(" ")))) C.LeftChopInline(1);
		return C + TEXT("...");
	};
	FString Out;
	for (const FString& S : Said)
	{
		const FString Try = Out.IsEmpty() ? S : Out + TEXT(" ") + S;
		if (Try.Len() <= Room) { Out = Try; continue; }
		// one sentence too long on its own; or, with room left, the start of the next (the thing to do after "TANK EMPTY.")
		if (Out.IsEmpty()) Out = Cut(S, Room);
		else if (Room - Out.Len() - 1 >= 20) Out += TEXT(" ") + Cut(S, Room - Out.Len() - 1);
		break;
	}
	if (Out.IsEmpty()) return Head.Left(Most);
	return Joint + Out;
}

FString S47HudGauges::CardWith(const FString& Line, std::initializer_list<FString> Tails, int32 Most)
{
	FString Out = CardLine(Line, Most);
	for (const FString& Tail : Tails)
	{
		if (Tail.IsEmpty()) continue;
		if (Out.Len() + Tail.Len() <= Most) Out += Tail;
	}
	return Out;
}

double S47HudGauges::BackingStrength(double Daylight)
{
	const double Lit = FMath::IsFinite(Daylight) ? FMath::Clamp(Daylight, 0.0, 1.0) : 0.0;
	return 0.15 + 0.85 * FMath::SmoothStep(0.05, 0.6, Lit);
}

// ---------- the dark edge (VIGNETTE-STEPS-20261109) ----------

namespace
{
	// how far the rim's corners are rounded: 2 a circle, larger nearer the old square corner
	constexpr double RimCornerPower = 3.0;
	// vertices across each side's reach: at 1920 pixels and the widest reach, one every 18 pixels or so
	constexpr int32 RimSteps = 40, LidSteps = 40;
	constexpr double RimMost = 0.95;

	bool Usable(double X) { return FMath::IsFinite(X); }

	// The axis' stops: the edge, through the reach, and the same from the far side. Sorted, without doubles.
	void Stops(double Reach, int32 Steps, TArray<double>& Out)
	{
		Out.Reset();
		const double R = FMath::Clamp(Reach, 0.0, 0.5);
		for (int32 I = 0; I <= Steps; ++I) { Out.Add(R * I / Steps); Out.Add(1.0 - R * I / Steps); }
		Out.Sort();
		TArray<double> Kept;
		for (const double S : Out) if (Kept.IsEmpty() || S - Kept.Last() > 1e-6) Kept.Add(S);
		Out = MoveTemp(Kept);
	}

	// Two triangles over a cell, split along the diagonal whose ends are most alike, so the split follows the shade.
	void Cell(S47HudGauges::FShadeMesh& M, uint32 A, uint32 B, uint32 C, uint32 D)
	{
		// A B
		// C D
		constexpr float Seen = 0.5f / 255.f;
		if (M.Alpha[A] < Seen && M.Alpha[B] < Seen && M.Alpha[C] < Seen && M.Alpha[D] < Seen) return;
		if (FMath::Abs(M.Alpha[A] - M.Alpha[D]) <= FMath::Abs(M.Alpha[B] - M.Alpha[C])) M.Index.Append({ A, B, D, A, D, C });
		else M.Index.Append({ A, B, C, B, D, C });
	}
}

double S47HudGauges::RimShade(double U, double V, double Reach, double Alpha)
{
	if (!Usable(U) || !Usable(V) || !Usable(Reach) || !Usable(Alpha) || Reach <= 0.0 || Alpha <= 0.0) return 0.0;
	const double A = FMath::Min(Alpha, RimMost);
	// nearness to each edge, 1 at the edge and 0 at the reach, in each side's own share as the old frames were
	const double Wx = FMath::Clamp(1.0 - FMath::Min(U, 1.0 - U) / Reach, 0.0, 1.0);
	const double Wy = FMath::Clamp(1.0 - FMath::Min(V, 1.0 - V) / Reach, 0.0, 1.0);
	const double C = FMath::Min(1.0, FMath::Pow(FMath::Pow(Wx, RimCornerPower) + FMath::Pow(Wy, RimCornerPower), 1.0 / RimCornerPower));
	// in softly from the inner end, still rising at the edge
	const double E = C * C * (2.0 - C);
	// the old frames laid N shades of 1-(1-A)^(1/N) over each other: 1-(1-A)^(share of the frames a point was inside), here without steps
	return 1.0 - FMath::Pow(1.0 - A, E);
}

double S47HudGauges::LidShade(double Y, double Deep, double Edge)
{
	if (!Usable(Y) || !Usable(Deep) || !Usable(Edge) || Deep <= 0.0 || Edge <= 0.0) return 0.0;
	const double C = FMath::Clamp(1.0 - Y / Deep, 0.0, 1.0);
	return 1.0 - FMath::Pow(1.0 - FMath::Min(Edge, 0.999), C);
}

void S47HudGauges::RimMesh(const FVector2f& Size, double Reach, double Alpha, FShadeMesh& Out)
{
	Out.Reset();
	if (Size.X <= 0.f || Size.Y <= 0.f || !Usable(Reach) || !Usable(Alpha) || Reach <= 0.0 || Alpha <= 0.0) return;
	TArray<double> Su, Sv;
	Stops(Reach, RimSteps, Su);
	Stops(Reach, RimSteps, Sv);
	const int32 Nu = Su.Num(), Nv = Sv.Num();
	Out.At.Reserve(Nu * Nv); Out.Alpha.Reserve(Nu * Nv);
	for (int32 J = 0; J < Nv; ++J)
		for (int32 I = 0; I < Nu; ++I)
		{
			Out.At.Add(FVector2f(float(Su[I] * Size.X), float(Sv[J] * Size.Y)));
			Out.Alpha.Add(float(RimShade(Su[I], Sv[J], Reach, Alpha)));
		}
	for (int32 J = 0; J + 1 < Nv; ++J)
		for (int32 I = 0; I + 1 < Nu; ++I)
			Cell(Out, uint32(J * Nu + I), uint32(J * Nu + I + 1), uint32((J + 1) * Nu + I), uint32((J + 1) * Nu + I + 1));
}

void S47HudGauges::LidMesh(const FVector2f& Size, double Deep, double Edge, FShadeMesh& Out)
{
	Out.Reset();
	if (Size.X <= 0.f || Size.Y <= 0.f || !Usable(Deep) || !Usable(Edge) || Deep <= 0.0 || Edge <= 0.0) return;
	const double D = FMath::Min(Deep, double(Size.Y));
	// one strip from each edge: a row of two vertices at each step, the shade the same all along it
	for (const bool bTop : { true, false })
	{
		const uint32 First = uint32(Out.At.Num());
		for (int32 K = 0; K <= LidSteps; ++K)
		{
			const double Y = D * K / LidSteps;
			const float At = float(bTop ? Y : Size.Y - Y), A = float(LidShade(Y, D, Edge));
			Out.At.Add(FVector2f(0.f, At)); Out.Alpha.Add(A);
			Out.At.Add(FVector2f(Size.X, At)); Out.Alpha.Add(A);
		}
		for (int32 K = 0; K < LidSteps; ++K)
		{
			const uint32 R = First + uint32(2 * K);
			Cell(Out, R, R + 1, R + 2, R + 3);
		}
	}
}

bool S47HudGauges::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s hud gauges: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bCond; };
	Check(Strokes(1) == 5 && Strokes(0.81) == 5 && Strokes(0.8) == 4 && Strokes(0.21) == 2 && Strokes(0.2) == 1 && Strokes(0.0001) == 1 && Strokes(0) == 0,
		TEXT("five strokes: full shows five, anything left shows one, empty none"));
	Check(Strokes(-1) == 0 && Strokes(2) == 5 && Strokes(std::numeric_limits<double>::quiet_NaN()) == 0, TEXT("a level out of range or not a number still gives a stroke count in 0 to 5"));
	bool bSteps = true; int32 Last = 0;
	for (int32 I = 0; I <= 100; ++I) { const int32 N = Strokes(I / 100.0); bSteps &= N >= Last; Last = N; }
	Check(bSteps, TEXT("the strokes only ever grow with the level"));
	Check(BatteryShown(true, true, true, 1.0, 0.2) && BatteryShown(true, true, false, 0.1, 0.2) && !BatteryShown(true, true, false, 0.8, 0.2),
		TEXT("shown while the lamp is on, or when it is low; hidden when it is off and well charged"));
	Check(!BatteryShown(false, true, true, 0.1, 0.2) && !BatteryShown(true, false, true, 0.1, 0.2), TEXT("not shown in the Story, nor before the lamp is taken"));
	Check(CbLine() == TEXT("CB 19") && WalkieLine(1, TEXT("Dale")) == FString(TEXT("WALKIE 1 \u00B7 DALE")) && WalkieLine(2) == TEXT("WALKIE 2") && WalkieLine(7) == WalkieLine(3),
		TEXT("the radio line names the set, its channel and who is sending"));
	// HUD-D-20261109: one radio colour in every palette, so the CB and the walkie are told apart by their words alone
	bool bApart = true;
	for (const FString& Who : { FString(), FString(TEXT("DALE")), FString(TEXT("YOU")) })
		for (int32 Ch = 1; Ch <= 3; ++Ch)
		{
			const FString W = WalkieLine(Ch, Who);
			bApart &= W.StartsWith(TEXT("WALKIE ")) && !W.Contains(TEXT("CB")) && !CbLine().Contains(TEXT("WALKIE")) && W != CbLine();
		}
	Check(bApart, TEXT("the CB and the walkie are told apart by their words: CB 19 against WALKIE and its channel, never by colour alone"));
	int32 Ch = 0; FString Who;
	Check(WalkieSpeaker(TEXT("WALKIE 1, DALE"), Ch, Who) && Ch == 1 && Who == TEXT("DALE") && !WalkieSpeaker(TEXT("WALKIE 2"), Ch, Who) && !WalkieSpeaker(TEXT("CB, Breaker"), Ch, Who)
		&& !WalkieSpeaker(TEXT("WARD"), Ch, Who), TEXT("a walkie caption names who is speaking; the set's own head and the CB's do not"));
	Check(RadioKind(false, false, true, false, false, 0) == 0 && RadioKind(false, true, true, false, false, 0) == 0 && RadioKind(true, false, true, false, false, 0) == 0
		&& RadioKind(false, false, false, true, true, 5) == 0 && RadioKind(false, false, true, false, false, -1) == 0
		&& RadioKind(false, false, true, false, false, std::numeric_limits<double>::quiet_NaN()) == 0,
		TEXT("the walkie line is hidden while nobody is sending: carrying the set, a CB out of reach or no set at all show nothing"));
	Check(RadioKind(true, true, true, true, false, 3) == 1 && RadioKind(false, false, true, true, false, 0) == 2 && RadioKind(false, false, true, false, true, 0) == 2
		&& RadioKind(false, false, true, false, false, 2.5) == 2, TEXT("the line shows while a CB call is on, Dale hails, the player is keyed or words are coming"));
	Check(WalkieWho(false, true, TEXT("DALE")) == TEXT("YOU") && WalkieWho(true, false, FString()) == TEXT("DALE") && WalkieWho(false, false, TEXT("DALE")) == TEXT("DALE"),
		TEXT("the walkie line names the one sending"));
	bool bCb = true;
	for (int32 I = 0; I < 200; ++I) { const double S = CbStrength(I * 0.37); bCb &= S > 0.6 && S <= 1.0; }
	Check(bCb, TEXT("the CB's signal during a call stays strong and steady"));
	bool bEnds = true, bBounded = true;
	for (double K : { 0.0, 0.3, 1.0 })
		for (int32 P = 0; P < 40; ++P)
		{
			bEnds &= FMath::Abs(Wave(0, P * 0.3, K)) < 1e-9 && FMath::Abs(Wave(1, P * 0.3, K)) < 1e-9;
			for (int32 I = 0; I <= 24; ++I) { const double V = Wave(I / 24.0, P * 0.3, K); bBounded &= V >= -1.0 && V <= 1.0; }
		}
	Check(bEnds && bBounded, TEXT("the wave starts and ends on the line and stays within it"));
	auto Swing = [](double K) { double M = 0; for (int32 P = 0; P < 40; ++P) for (int32 I = 0; I <= 48; ++I) M = FMath::Max(M, FMath::Abs(Wave(I / 48.0, P * 0.3, K))); return M; };
	Check(Swing(1.0) > Swing(0.5) && Swing(0.5) > Swing(0.1) && Swing(1.0) > 0.9, TEXT("a stronger signal swings wider"));
	Check(FiledLine(TEXT("08:27")) == TEXT("LOG FILED  08:27") && FiledLine(FString()) == TEXT("LOG FILED"), TEXT("the departure stamp says LOG FILED and the save's clock"));
	Check(FiledRefused(TEXT("LOG NOT FILED")) && !FiledRefused(FiledLine(TEXT("08:27"))) && !FiledRefused(FiledLine(FString())), TEXT("a departure that was not filed has a stamp of its own colour, LOG FILED does not"));
	Check(FiledAlpha(-1) == 0 && FiledAlpha(0) == 0 && FMath::IsNearlyEqual(FiledAlpha(0.125), 0.5, 1e-9) && FiledAlpha(1) == 1 && FMath::IsNearlyEqual(FiledAlpha(FiledFor - 0.4), 0.5, 1e-9) && FiledAlpha(FiledFor) == 0
		&& FiledAlpha(std::numeric_limits<double>::quiet_NaN()) == 0, TEXT("the stamp comes in, holds and is gone after a few seconds"));

	// HUD-D-20261109: the palettes, the choice and its keeping
	bool bPalettes = true, bLowApart = true;
	TSet<FString> PaletteIds;
	for (int32 I = 0; I < HudColors; ++I)
	{
		const FHudPalette& Pal = Palette(EHudColor(I));
		PaletteIds.Add(Pal.Id);
		bPalettes &= HudColorFrom(Pal.Id) == EHudColor(I) && FCString::Strlen(Pal.Label) > 2 && FCString::Strlen(Pal.Help) > 20
			&& Pal.ClockOn > Pal.ClockGhost && Pal.BatteryOn > Pal.BatteryGhost && Pal.RadioText > 0.5f && Pal.RadioWave > 0.5f;
		// the low battery always stands apart: a colour well away from the battery's own, and at full strength
		bLowApart &= ColorDistance(Pal.Low, Pal.Battery) >= 0.4 && ColorDistance(Pal.Low, Pal.Clock) >= 0.4 && Pal.LowOn >= 1.f && Pal.LowOn > Pal.BatteryOn
			&& Pal.LowFrame > Pal.BatteryFrame && Pal.LowGhost >= Pal.BatteryGhost;
	}
	Check(bPalettes && PaletteIds.Num() == HudColors, TEXT("five HUD palettes, each with its own id, name and words, read back from its id"));
	Check(bLowApart, TEXT("in every palette a low battery stands apart from the battery and the clock, at full strength"));
	const FHudPalette& Teal = Palette(EHudColor::StationTeal);
	Check(Teal.Clock == 0x78ffde && Teal.Battery == 0x78ffde && Teal.Low == 0xff5a45 && Teal.Radio == 0xffb052 && FMath::IsNearlyEqual(Teal.ClockOn, 150.f / 255.f)
		&& FMath::IsNearlyEqual(Teal.ClockGhost, 14.f / 255.f) && FMath::IsNearlyEqual(Teal.BatteryOn, 0.55f) && FMath::IsNearlyEqual(Teal.BatteryFrame, 0.5f)
		&& FMath::IsNearlyEqual(Teal.LowFrame, 0.7f) && FMath::IsNearlyEqual(Teal.RadioText, 225.f / 255.f) && FMath::IsNearlyEqual(Teal.RadioWave, 215.f / 255.f)
		&& FString(Teal.Label) == TEXT("Station teal"), TEXT("Station teal is V4 as Tom chose it"));
	const FHudPalette& AmberPal = Palette(EHudColor::Amber);
	Check(AmberPal.Clock == AmberPal.Battery && AmberPal.Clock == AmberPal.Radio && AmberPal.Clock == 0xffb052, TEXT("Amber is amber throughout, the clock too"));
	Check(HudColorFrom(TEXT("nonsense")) == EHudColor::StationTeal && HudColorFrom(FString()) == EHudColor::StationTeal && HudColorFrom(TEXT(" RED ")) == EHudColor::RedNight,
		TEXT("an unknown or missing choice is Station teal"));
	{
		const EHudColor Was = HudColor();
		SetHudColor(EHudColor::PhosphorGreen);
		const bool bSet = HudColor() == EHudColor::PhosphorGreen && Palette().Clock == Palette(EHudColor::PhosphorGreen).Clock;
		SetHudColor(EHudColor(200));
		const bool bBad = HudColor() == EHudColor::StationTeal;
		// kept as the settings are: a string in the store's JSON, written out and read back
		bool bKept = true;
		for (int32 I = 0; I < HudColors; ++I)
		{
			const S47J::FObj Saved = S47J::New();
			S47J::Set(Saved, TEXT("hudColor"), HudColorId(EHudColor(I)));
			FString Written;
			const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Written);
			FJsonSerializer::Serialize(Saved.ToSharedRef(), Writer);
			S47J::FObj Back;
			FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Written), Back);
			bKept &= HudColorFrom(S47J::Str(Back, TEXT("hudColor"))) == EHudColor(I);
		}
		SetHudColor(Was);
		Check(bSet && bBad && bKept, TEXT("the HUD colour is set at once, a bad one falls back, and the choice is kept and read back from the store"));
	}

	// the segment clock: 12 hours with AM and PM
	const FClockFace Late = ClockFace(TEXT("23:41")), Small = ClockFace(TEXT("01:47")), Noon = ClockFace(TEXT("12:05")), Midnight = ClockFace(TEXT("00:12"));
	Check(Late.bValid && Late.Hour == 11 && Late.Minute == 41 && Late.bPm && Late.Second < 0 && Small.Hour == 1 && !Small.bPm && Noon.Hour == 12 && Noon.bPm
		&& Midnight.Hour == 12 && !Midnight.bPm, TEXT("the clock reads 12 hours with AM and PM, never 24"));
	const FClockFace Secs = ClockFace(TEXT("02:13:07")), All = ClockFace(TEXT("88:88"));
	Check(Secs.bValid && Secs.Second == 7 && Secs.Hour == 2 && All.bValid && All.bAll && !ClockFace(TEXT("")).bValid && !ClockFace(TEXT("LATE AFTERNOON")).bValid
		&& !ClockFace(TEXT("25:00")).bValid && !ClockFace(TEXT("9:7")).bValid, TEXT("the seconds of a warning, the jumping 88:88, and words that are no time"));
	Check(Segments(8) == 0x7f && Segments(1) == 0x06 && Segments(0) == 0x3f && Segments(7) == 0x07 && Segments(-1) == 0 && Segments(10) == 0,
		TEXT("each figure lights its own segments"));

	// the task's cassette card: in, six seconds, out over one
	bool bHold = true, bFalls = true;
	for (double T = NoteIn; T < NoteHold; T += 0.25) bHold &= NoteAlpha(T) == 1.0;
	for (double T = NoteHold; T < NoteHold + NoteFade + 0.5; T += 0.05) bFalls &= NoteAlpha(T + 0.05) <= NoteAlpha(T);
	Check(NoteAlpha(0) == 0 && NoteAlpha(-1) == 0 && NoteAlpha(NoteIn * 0.5) > 0 && bHold && FMath::IsNearlyEqual(NoteAlpha(NoteHold + NoteFade * 0.5), 0.5, 1e-9)
		&& NoteAlpha(NoteHold + NoteFade) == 0 && NoteAlpha(60) == 0 && NoteAlpha(std::numeric_limits<double>::quiet_NaN()) == 0 && bFalls,
		TEXT("the task's card stands six seconds after the task changes and fades out over one"));
	Check(Handwritten(TEXT("CALICHE JUNCTION"), true) == TEXT("Caliche Junction") && Handwritten(TEXT("CONTROL ROOM"), true) == TEXT("Control Room")
		&& Handwritten(TEXT("SARO"), true) == TEXT("SARO") && Handwritten(TEXT("KEEP THE COMPLETED FEEDER RECORD. RETURN TO THE SERVICE TRUCK"), false) == TEXT("Keep the completed feeder record. Return to the service truck")
		&& Handwritten(TEXT("TRACK THE UNLOGGED RESIDUAL NEAR 1420.4 MHz"), false) == TEXT("Track the unlogged residual near 1420.4 MHz")
		&& Handwritten(TEXT("back to the truck - on foot"), false) == TEXT("back to the truck - on foot") && Handwritten(TEXT("CALL DALE'S DESK"), false) == TEXT("Call Dale's desk")
		&& Handwritten(TEXT("ROAD TO THE MESA"), true) == TEXT("Road to the Mesa"),
		TEXT("the task is written by hand: the place in title case, the task as a sentence, the station's capitals and the figures kept"));
	// M7-HUD-20261109: OPS and PC as the room has them, an antenna's name as it is painted, the doctor's title
	Check(Handwritten(TEXT("OPS"), true) == TEXT("OPS") && Handwritten(TEXT("TRACKING PC"), true) == TEXT("Tracking PC")
		&& Handwritten(TEXT("REPAIR THE WEST BRANCH AT ANTENNA S-13"), false) == TEXT("Repair the west branch at antenna S-13")
		&& Handwritten(TEXT("A LETTER FROM DR. WARD'S OFFICE"), false) == TEXT("A letter from Dr. Ward's office") && Handwritten(TEXT("DALE'S DESK"), true) == TEXT("Dale's Desk"),
		TEXT("OPS and PC stay in capitals on the card, S-13 keeps its letter, Dr. Ward his title"));
	{
		const FString Long = TEXT("OPS (corner PC), DAILY WORK: SELECT ONE OF 2 UNFINISHED OBSERVATIONS; TRACK, TUNE AND REGISTER THE NEXT RESULT. COMPLETED RESULTS ARE IN EARLIER WORK. STATION GOAL: ARRAY 12 OF 27.");
		const TCHAR* Tasks[] = {
			TEXT("TRACKING (right-hand PC): POINT THE ARRAY TO THE SELECTED SOURCE"),
			TEXT("RECEIVER (centre PC): KEEP IT TUNED; COLLECT THE ORIGINAL AT THE PRINTER"),
			TEXT("FIRST SHIFT 2/5 / TRACKING (right-hand PC): POINT THE ARRAY TO THE SELECTED SOURCE"),
			TEXT("SERVICE TRUCK: TANK EMPTY. GET OUT AND USE THE TRUCK TO CALL ROADSIDE RETURN"),
			TEXT("S-03 CABLE JUNCTION / LOCAL BOX: INSPECT, ISOLATE AND OPEN THE RELAY HATCH"),
			TEXT("EAST-WING GARAGE: THE ROLL-UP DOOR IS DOWN. PRESS THE GREEN UP BUTTON BY THE DOOR (OR USE THE HAND CHAIN), THEN TAKE THE PARCEL TO THE RECEIVING TABLE"),
			TEXT("MAILBOX / A letter from Dr. Ward's office is in the box at the road."),
			TEXT("PAUSE MENU: LOAD A SAVED CASE TO CONTINUE AFTER CONTACT"),
			TEXT("ANOTHERVERYLONGWORDWITHOUTANYSPACEATALLTHATGOESONANDONANDONFORMORETHANSEVENTYCHARACTERSINALL"),
		};
		bool bFits = CardLine(Long).Len() <= CardMost;
		for (const TCHAR* Task : Tasks) bFits &= CardLine(Task).Len() <= CardMost && !CardLine(Task).IsEmpty();
		Check(bFits && CardLine(FString()).IsEmpty(), TEXT("every task fits the card in seventy characters, however long it is"));
		Check(CardLine(Tasks[0]) == TEXT("TRACKING PC // POINT THE ARRAY TO THE SELECTED SOURCE") && CardLine(Tasks[1]) == TEXT("RECEIVER PC // KEEP IT TUNED. COLLECT THE ORIGINAL AT THE PRINTER")
			&& CardLine(Tasks[3]).StartsWith(TEXT("SERVICE TRUCK // TANK EMPTY. GET OUT AND USE THE TRUCK")) && CardLine(Tasks[3]).EndsWith(TEXT("..."))
			&& CardLine(Tasks[6]) == TEXT("MAILBOX // A letter from Dr. Ward's office is in the box at the road.")
			&& CardLine(TEXT("FRAME 01 // INSPECT THE PRINT")) == TEXT("FRAME 01 // INSPECT THE PRINT"),
			TEXT("the card names the place and then the task in whole sentences; a doctor's title is no end of one"));
		Check(CardLine(Tasks[2]) == TEXT("FIRST SHIFT 2/5 // TRACKING PC: POINT THE ARRAY TO THE SELECTED SOURCE")
			&& CardLine(TEXT("FIRST SHIFT 1/5 / OPS (corner PC): OPEN DAILY WORK AND ACCEPT YOUR FIRST JOB")) == TEXT("FIRST SHIFT 1/5 // OPS: OPEN DAILY WORK AND ACCEPT YOUR FIRST JOB")
			&& CardLine(TEXT("FIRST SHIFT 3/5 / RECEIVER (centre PC): TUNE THE CARD AND PRESS START RECORDING")) == TEXT("RECEIVER PC 3/5 // TUNE THE CARD AND PRESS START RECORDING") && CardLine(Tasks[5]) == TEXT("EAST-WING GARAGE // THE ROLL-UP DOOR IS DOWN. PRESS THE GREEN UP...")
			&& CardLine(Tasks[8]).EndsWith(TEXT("...")), TEXT("a sentence too long for the card is cut at a word and says so; the rest waits in the notebook and the pause menu"));
		{
			// Critic round 5: the first shift's card is never cut off. Its steps as FS47FreeformSession says them (TutorialObjective,
			// JobNextAction), and the printer stays on it.
			const TCHAR* Shift[] = {
				TEXT("FIRST SHIFT 1/5 / OPS (corner PC): OPEN DAILY WORK AND ACCEPT YOUR FIRST JOB"),
				TEXT("FIRST SHIFT 2/5 / TRACKING (right-hand PC): POINT THE ARRAY TO THE SELECTED SOURCE"),
				TEXT("FIRST SHIFT 3/5 / RECEIVER (centre PC): TUNE THE CARD AND PRESS START RECORDING"),
				TEXT("FIRST SHIFT 3/5 / TRACKING (right-hand PC): WAIT FOR ENCODER AGREEMENT, THEN USE RECEIVER (centre PC)"),
				TEXT("FIRST SHIFT 4/5 / RECEIVER (centre PC): KEEP IT TUNED; COLLECT THE ORIGINAL AT THE PRINTER"),
				TEXT("FIRST SHIFT 4/5 / PRINTER: RETAIN THE ORIGINAL OBSERVATION CHART"),
				TEXT("FIRST SHIFT 5/5 / OPS (corner PC): REGISTER THE ROUTINE RESULT"),
				TEXT("FIRST SHIFT 6/6 / PRINTER: COLLECT THE LABELLED REGISTERED PACKET"),
				TEXT("FIRST SHIFT 6/6 / RECORDS ROOM: PLACE THE ORIGINAL PACKET IN THE BINDER"),
			};
			bool bWhole = true;
			FString Cut;
			for (const TCHAR* Step : Shift) { const FString Shown = CardLine(Step); if (Shown.EndsWith(TEXT("...")) || Shown.Len() > CardMost) { bWhole = false; Cut = Shown; } }
			Check(bWhole && CardLine(Shift[4]).EndsWith(TEXT("AT THE PRINTER")), *(FString(TEXT("the first shift's card says each step whole, the printer too ")) + Cut));
		}
		Check(CardWith(TEXT("OPS // 2 OBSERVATIONS WAITING."), { FString(TEXT(" WALKIE: ON THE DESK.")), FString(TEXT(" ARRAY 12 OF 27.")) }) == TEXT("OPS // 2 OBSERVATIONS WAITING. WALKIE: ON THE DESK. ARRAY 12 OF 27.")
			&& CardWith(TEXT("ANTENNA S-13 // REPAIR IT."), { FString(TEXT(" THIS ONE IS FAR TOO LONG TO FIT ON THE CARD AT ALL, REALLY NOT.")), FString(TEXT(" ARRAY 12/27.")) }) == TEXT("ANTENNA S-13 // REPAIR IT. ARRAY 12/27."),
			TEXT("more is said after the task only while the card has room, a tail too long left out"));
	}
	Check(BackingStrength(0) <= 0.15 && BackingStrength(0.05) <= 0.15 && BackingStrength(1) == 1.0 && BackingStrength(0.6) == 1.0 && BackingStrength(0.3) > BackingStrength(0.1)
		&& BackingStrength(std::numeric_limits<double>::quiet_NaN()) <= 0.15 && BackingStrength(-2) <= 0.15 && BackingStrength(5) == 1.0,
		TEXT("the dark rim under the clock and the battery is faint at night and full against a bright day sky"));
	// VIGNETTE-STEPS-20261109: the strain's rim and the lids are smooth shades, not frames on frames. A step is a change of more
	// than 8 of 255 from one pixel to the next on a 1920 x 1080 view; the old 14 frames at 0.7 jumped 21 of 255 at each edge.
	{
		constexpr double Step = 8.0 / 255.0;
		constexpr double W = 1920.0, H = 1080.0;
		bool bRimSmooth = true, bRimRising = true, bRimEnds = true;
		FString Worst;
		for (const double Reach : { 0.1, 0.22, 0.38 })
			for (const double A : { 0.3, 0.7, 0.95 })
			{
				// along the middle of the left side, the edge to the reach and on into the clear middle
				double Was = RimShade(0.0, 0.5, Reach, A), Most = 0;
				bRimEnds &= FMath::IsNearlyEqual(Was, FMath::Min(A, 0.95), 1e-6) && RimShade(Reach, 0.5, Reach, A) < 1e-9 && RimShade(0.5, 0.5, Reach, A) == 0.0;
				for (int32 X = 1; X <= int32(W / 2); ++X)
				{
					const double S = RimShade(X / W, 0.5, Reach, A);
					bRimRising &= S <= Was + 1e-12;
					Most = FMath::Max(Most, FMath::Abs(Was - S));
					Was = S;
				}
				// and down from a corner, where the old frames met in a hard diagonal
				Was = RimShade(0.0, 0.0, Reach, A);
				for (int32 K = 1; K <= int32(H / 2); ++K)
				{
					const double S = RimShade(K / W, K / H, Reach, A);
					bRimRising &= S <= Was + 1e-12;
					Most = FMath::Max(Most, FMath::Abs(Was - S));
					Was = S;
				}
				if (Most > Step) { bRimSmooth = false; Worst = FString::Printf(TEXT(" (reach %.2f, alpha %.2f: %.1f of 255)"), Reach, A, Most * 255); }
			}
		Check(bRimSmooth && bRimRising && bRimEnds, *(FString(TEXT("the strain's rim darkens smoothly toward the edge, as dark there as before and clear past its reach")) + Worst));
		// the old frames, for the measure: the same test finds their steps
		{
			constexpr int32 Bands = 14;
			const double A = 0.7, Reach = 0.3, Each = 1.0 - FMath::Pow(1.0 - A, 1.0 / Bands);
			auto Old = [&](double U) { int32 K = 0; for (int32 I = 1; I <= Bands; ++I) K += U < Reach * I / Bands ? 1 : 0; return 1.0 - FMath::Pow(1.0 - Each, K); };
			double Most = 0;
			for (int32 X = 1; X <= int32(W / 2); ++X) Most = FMath::Max(Most, FMath::Abs(Old((X - 1) / W) - Old(X / W)));
			Check(Most > Step, TEXT("the measure catches the old nested frames' steps"));
		}
		// the mesh: one element, its vertices on the shade, fine enough that the blend between them hides them
		FShadeMesh Mesh;
		RimMesh(FVector2f(float(W), float(H)), 0.38, 0.95, Mesh);
		bool bMesh = Mesh.At.Num() == Mesh.Alpha.Num() && Mesh.Index.Num() > 0 && Mesh.Index.Num() % 3 == 0 && Mesh.At.Num() < 65536;
		for (const uint32 I : Mesh.Index) bMesh &= I < uint32(Mesh.At.Num());
		bool bOn = true, bFine = true;
		double Gap = 0;
		for (int32 I = 0; I < Mesh.At.Num(); ++I)
		{
			bOn &= FMath::IsNearlyEqual(double(Mesh.Alpha[I]), RimShade(Mesh.At[I].X / W, Mesh.At[I].Y / H, 0.38, 0.95), 1e-5);
			// the next vertex along the row, if there is one: how big a change the card blends over how many pixels
			if (I + 1 < Mesh.At.Num() && Mesh.At[I + 1].Y == Mesh.At[I].Y && Mesh.At[I + 1].X > Mesh.At[I].X)
			{
				const double Dx = Mesh.At[I + 1].X - Mesh.At[I].X, Da = FMath::Abs(Mesh.Alpha[I + 1] - Mesh.Alpha[I]);
				Gap = FMath::Max(Gap, Da);
				bFine &= Da / Dx <= Step;
			}
		}
		Check(bMesh && bOn && bFine && Gap < 0.12, *FString::Printf(TEXT("the rim is one mesh of %d vertices, drawn as one element, %.1f of 255 at most between neighbours"), Mesh.At.Num(), Gap * 255));
		RimMesh(FVector2f(float(W), float(H)), 0.0, 0.7, Mesh);
		const bool bNoReach = Mesh.Index.IsEmpty();
		RimMesh(FVector2f(float(W), float(H)), 0.3, 0.0, Mesh);
		Check(bNoReach && Mesh.Index.IsEmpty() && RimShade(0, 0, std::numeric_limits<double>::quiet_NaN(), 0.7) == 0.0, TEXT("no rim, no mesh: nothing is drawn while calm"));
		// the lids: darkest at the screen's edge, smooth to their reach, the same as the old 12 bands at the edge
		bool bLids = true;
		for (const double Lids : { 0.3, 0.6, 1.0 })
		{
			const double Edge = FMath::Lerp(0.8, 0.985, Lids), Deep = H * 0.5 * FMath::Min(1.0, Lids * 1.08);
			double Was = LidShade(0, Deep, Edge);
			bLids &= FMath::IsNearlyEqual(Was, Edge, 1e-6) && LidShade(Deep, Deep, Edge) < 1e-9;
			for (int32 Y = 1; Y <= int32(Deep); ++Y) { const double S = LidShade(Y, Deep, Edge); bLids &= S <= Was + 1e-12 && Was - S <= Step; Was = S; }
		}
		LidMesh(FVector2f(float(W), float(H)), H * 0.5, 0.985, Mesh);
		bLids &= Mesh.At.Num() == 2 * 2 * (LidSteps + 1) && Mesh.Index.Num() > 0 && Mesh.Index.Num() % 3 == 0;
		Check(bLids, TEXT("the heavy lids are one smooth shade each from the top and the bottom, as dark at the edge as before"));
	}
	return bOk;
}

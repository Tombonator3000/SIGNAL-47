#include "S47Sky.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "S47FreeformTerrain.h"
#include "S47Fx.h"
#include "S47Image.h"
#include "S47Three.h"
#include "S47Weather.h"
#include "S47Web.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Sky, Log, All);

namespace
{
	constexpr double DEG = UE_DOUBLE_PI / 180;
	// (measured as the chord between two directions: Sky.ts chord)
	double Chord(double Deg) { return 2 * FMath::Sin(Deg * DEG / 2); }
	double Clamp01(double V) { return FMath::Clamp(V, 0.0, 1.0); }
	double Smooth01(double V) { const double K = Clamp01(V); return K * K * (3 - 2 * K); }
	double Rand() { return FMath::FRand(); }
	FLinearColor V4(const FVector& V) { return FLinearColor(float(V.X), float(V.Y), float(V.Z), 0.f); }
	// PT-MOON-20261109: the textured moon's light (M_S47_Moon's Color, times its albedo: the highlands 1, the seas about .4)
	TAutoConsoleVariable<float> CvMoonGain(TEXT("s47.moon.Gain"), .7f, TEXT("The textured moon's brightness at night (the painted disc of 8 November was 4: one white blot)."));
	TAutoConsoleVariable<float> CvMoonDayGain(TEXT("s47.moon.DayGain"), .55f, TEXT("The textured moon's brightness by day, over the sky."));
	TAutoConsoleVariable<float> CvMoonEarth(TEXT("s47.moon.Earth"), .035f, TEXT("Earthshine on the moon's dark side at a new crescent, of its lit side."));

	UStaticMeshComponent* Part(AS47Web* W, USceneComponent* Under, UStaticMesh* Mesh, UMaterialInterface* Mat, int32 SortPriority)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W);
		C->SetMobility(EComponentMobility::Movable);
		C->SetupAttachment(Under);
		C->SetStaticMesh(Mesh);
		if (Mat) C->SetMaterial(0, Mat);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->bAffectDistanceFieldLighting = false;
		C->SetVisibleInRayTracing(false);
		C->SetTranslucentSortPriority(SortPriority);
		C->RegisterComponent();
		return C;
	}
}

FS47Sky::FDevMoon FS47Sky::DevMoon;

FVector S47SkyDir(double Az, double El)
{
	// skyDir: v.set(sin(az) cos(el), sin(el), -cos(az) cos(el))
	return FVector(FMath::Sin(Az * DEG) * FMath::Cos(El * DEG), FMath::Sin(El * DEG), -FMath::Cos(Az * DEG) * FMath::Cos(El * DEG));
}

double S47CalicheVisualWeight(const FVector& EyeThree)
{
	static const bool bLegacy = FParse::Param(FCommandLine::Get(), TEXT("S47LegacyCalicheConcept")) || FParse::Param(FCommandLine::Get(), TEXT("S47LegacyCalicheStreet")) || FParse::Param(FCommandLine::Get(), TEXT("S47PropsOff"));
	if (bLegacy) return 0;
	const double D = FVector2D(EyeThree.X - 8000, EyeThree.Z - 2050).Size();
	// Nature extends through the town outskirts. Keep its authored escarpments
	// fully active there, so the old mesa ring cannot reappear through them.
	static const bool bNature = !FParse::Param(FCommandLine::Get(), TEXT("S47CalicheNatureOff")) &&
		FPaths::FileExists(FPaths::ProjectContentDir() / TEXT("CalicheStreet/Nature/nature-placement.json"));
	return bNature ? 1 - Smooth01((D - 420) / 480) : 1 - Smooth01((D - 120) / 480);
}

double S47StarHoleAt(double Clock, double From)
{
	const double S = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
	if (S > 12 * 3600 || !FMath::IsFinite(From)) return 0;
	const double K = Clamp01((S - From) / STAR_HOLE.Fade);
	return K * K * (3 - 2 * K);
}

FS47Sky::FS47Sky()
{
	HoleR = FVector2D(Chord(STAR_HOLE.Inner), Chord(STAR_HOLE.Outer));
	ShowerRadiant = S47SkyDir(200, 60);
	PassageReset(false);
}

// ---------- Sky.ts ----------

void FS47Sky::MeteorNow()
{
	FMet& M = Met;
	if (M.T >= 0) return;
	// somewhere between 25 and 70 degrees up, any direction
	const double Az = Rand() * UE_DOUBLE_PI * 2, El = (25 + Rand() * 45) * UE_DOUBLE_PI / 180;
	M.Start = FVector(FMath::Sin(Az) * FMath::Cos(El), FMath::Sin(El), -FMath::Cos(Az) * FMath::Cos(El));
	// across the sky and mostly downwards, as they seem to fall towards the horizon
	const FVector Down = (FVector(0, -1, 0) + M.Start * M.Start.Y).GetSafeNormal();
	// (three's crossVectors, in its right-handed terms: the formula is the same)
	const FVector Side = FVector::CrossProduct(M.Start, Down).GetSafeNormal();
	M.Tan = (Down * (0.6 + Rand() * 0.6) + Side * ((Rand() * 2 - 1) * 1.2)).GetSafeNormal();
	M.Len = (8 + Rand() * 10) * UE_DOUBLE_PI / 180;
	M.Dur = 0.45 + Rand() * 0.5;
	// now and then a bright one
	M.Bright = Rand() < 0.15 ? 2.6 : 1.1 + Rand() * 0.6;
	M.T = 0;
	Meteors++;
}

void FS47Sky::MeteorFrom(const FVector& Dir)
{
	// (as meteorNow lays out its way, from a start the test chose)
	FMet& M = Met;
	if (M.T < 0) return;
	M.Start = Dir.GetSafeNormal();
	const FVector Down = (FVector(0, -1, 0) + M.Start * M.Start.Y).GetSafeNormal();
	const FVector Side = FVector::CrossProduct(M.Start, Down).GetSafeNormal();
	M.Tan = (Down * 0.9 + Side * 0.3).GetSafeNormal();
}

void FS47Sky::MeteorStep(double Dt)
{
	FMet& M = Met;
	M.Next -= Dt;
	// only while it is night: none in the dawn
	// (FX-01: none of the rare ones while the passage by the hole is drawn, nor for a minute after a seen one)
	QuietLeft = FMath::Max(0.0, QuietLeft - FMath::Max(0.0, Dt));
	const bool bQuiet = PassageActive() || QuietLeft > 0;
	if (M.Next <= 0) { M.Next = 18 + Rand() * 22; if (Daylight() < 0.3 && !bQuiet) MeteorNow(); }
	if (M.T < 0) { bMetVisible = false; return; }
	M.T += Dt;
	const double TAIL = 0.2;                                   // the trail lingers a moment after the head is gone
	if (M.T > M.Dur + TAIL) { M.T = -1; bMetVisible = false; return; }
	const double P = FMath::Min(1.0, M.T / M.Dur);
	Head = M.Len * P;
	Back = FMath::Max(0.0, Head - M.Len * 0.65) + M.Len * 0.65 * FMath::Max(0.0, M.T - M.Dur) / TAIL;
	const double Fade = FMath::Min(1.0, M.T / 0.08) * (M.T > M.Dur ? 0.6 * (1 - (M.T - M.Dur) / TAIL) : 1 - 0.4 * P);
	MetI = M.Bright * Fade * (1 - Daylight());
	// (the strip's corners along the great circle from the start, and the way across it, are laid out
	// from these by its material, M_WebMeteor: the loop over MET_N is its vertex shader)
	bMetVisible = true;
}

// ---------- FX-01: the meteor passage by the star hole ----------

namespace
{
	double PassAngleDeg(const FVector& A, const FVector& B) { return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(A, B), -1.0, 1.0))); }
	/** A radians along a streak's great circle from its start (as M_WebMeteor lays the strip out). */
	FVector PassAlong(const FS47Sky::FPassMet& P, double A) { return P.Start * FMath::Cos(A) + P.Tan * FMath::Sin(A); }
	FVector PassHoleCentre() { return S47SkyDir(STAR_HOLE.Az, STAR_HOLE.El); }

	/** Against STAR_HOLE in flight (seconds into the streak): head in, wholly hidden (first and last), head out again. */
	bool PassMaskProfileOf(const FS47Sky::FPassMet& P, double& Enter, double& Hidden, double& HiddenEnd, double& Emerge)
	{
		Enter = Hidden = HiddenEnd = Emerge = -1;
		const FVector H = PassHoleCentre();
		for (int32 I = 0; I <= 2000; ++I)
		{
			// the flight only: once the head has stopped the tail merely closes up behind it
			const double T = P.Dur * I / 2000;
			double Hd, Bk, Fd;
			if (!FS47Sky::PassageLayout(P, T, Hd, Bk, Fd)) continue;
			const double HeadOff = PassAngleDeg(PassAlong(P, Hd), H);
			if (Enter < 0 && HeadOff < STAR_HOLE.Inner) Enter = T;
			// both ends of the drawn segment inside the inner rim: all of it is (the cap is convex)
			const bool bHidden = Enter >= 0 && Hd > Bk && HeadOff < STAR_HOLE.Inner && PassAngleDeg(PassAlong(P, Bk), H) < STAR_HOLE.Inner;
			if (bHidden && Emerge < 0) { if (Hidden < 0) Hidden = T; HiddenEnd = T; }
			if (Hidden >= 0 && Emerge < 0 && HeadOff > STAR_HOLE.Outer) Emerge = T;
		}
		return Enter >= 0 && Hidden >= Enter && Emerge > Hidden;
	}
}

const TArray<FS47Sky::FPassMet>& FS47Sky::PassageSchedule()
{
	static const TArray<FPassMet> Schedule = []
	{
		// Seconds into the passage: a few, building over five seconds, a long dense middle, thinning out
		// over the last ten. Every few seconds one crosses behind the field. Each streak gets its strip of
		// the pool here, once, so two drawn at the same time never share one; Low keeps a subset of them,
		// at most PASSAGE_LOW_HEADS at once (crossers first), High all of them, at most PASSAGE_SLOTS.
		uint32 Seed = 4747;
		auto R = [&Seed] { Seed = Seed * 1664525u + 1013904223u; return double(Seed >> 8) / double(1 << 24); };
		const FVector H = PassHoleCentre();
		const FVector Up = (FVector(0, 1, 0) - H * H.Y).GetSafeNormal(), Rt = FVector::CrossProduct(H, Up).GetSafeNormal();
		auto Around = [&](double AngDeg, double Pa) { const double A = AngDeg * DEG; return (H * FMath::Cos(A) + (Up * FMath::Cos(Pa) + Rt * FMath::Sin(Pa)) * FMath::Sin(A)).GetSafeNormal(); };
		// kept above 18 degrees (clear of the dishes and the horizon) and below 78 (not overhead)
		auto InSky = [](const FPassMet& P)
		{
			for (int32 K = 0; K <= 48; ++K) { const double Y = PassAlong(P, P.Len * K / 48).Y; if (Y < FMath::Sin(18 * DEG) || Y > FMath::Sin(78 * DEG)) return false; }
			return true;
		};
		// From near the field, 10 to 22 degrees out, away from it and a little down, as if from a radiant there; never into it.
		auto Radiant = [&](FPassMet& P)
		{
			for (int32 Try = 0; Try < 200; ++Try)
			{
				P.Start = Around(10 + R() * 12, R() * UE_DOUBLE_PI * 2);
				const FVector Away = (P.Start * FVector::DotProduct(P.Start, H) - H).GetSafeNormal();
				const FVector Down = (FVector(0, -1, 0) + P.Start * P.Start.Y).GetSafeNormal();
				const FVector Side = FVector::CrossProduct(P.Start, Down).GetSafeNormal();
				P.Tan = Away * 0.8 + Down * 0.45 + Side * ((R() * 2 - 1) * 0.25);
				P.Tan = (P.Tan - P.Start * FVector::DotProduct(P.Tan, P.Start)).GetSafeNormal();
				P.Len = (8 + R() * 7) * DEG;
				P.Dur = 0.4 + R() * 0.45;
				P.Trail = .65; P.bCross = false;
				P.Bright = R() < .1 ? 2.3 : 1.0 + R() * 0.6;
				bool bGood = InSky(P);
				for (int32 K = 0; K <= 24 && bGood; ++K) bGood = PassAngleDeg(PassAlong(P, P.Len * K / 24), H) >= STAR_HOLE.Outer + .8;
				if (bGood) return true;
			}
			return false;
		};
		// Across: from 10 to 13 degrees out, aimed to pass at most 2.5 degrees beside the centre, on to 5 degrees
		// beyond the outer rim. Its shorter drawn trail fits wholly inside the inner rim for a moment.
		auto Cross = [&](FPassMet& P)
		{
			for (int32 Try = 0; Try < 400; ++Try)
			{
				const double D0 = 10 + R() * 3, Miss = (R() * 2 - 1) * 2.5;
				P.Start = Around(D0, R() * UE_DOUBLE_PI * 2);
				const FVector To = (H - P.Start * FVector::DotProduct(P.Start, H)).GetSafeNormal();
				const FVector Perp = FVector::CrossProduct(P.Start, To).GetSafeNormal();
				// the right spherical triangle start / nearest point / centre: sin(miss) = sin(d0) sin(beta)
				const double Beta = FMath::Asin(FMath::Clamp(FMath::Sin(Miss * DEG) / FMath::Sin(D0 * DEG), -1.0, 1.0));
				P.Tan = (To * FMath::Cos(Beta) + Perp * FMath::Sin(Beta)).GetSafeNormal();
				P.Len = (FMath::Sqrt(D0 * D0 - Miss * Miss) + FMath::Sqrt(FMath::Square(STAR_HOLE.Outer + 5) - Miss * Miss) + R() * 3) * DEG;
				// about as fast across the sky as the rare shooting star (17 to 23 degrees a second)
				P.Dur = FMath::RadiansToDegrees(P.Len) / (17 + R() * 6);
				P.Trail = .3; P.bCross = true;
				P.Bright = 1.5 + R() * 0.7;
				double E, Hd, He, Ex;
				if (InSky(P) && PassAngleDeg(P.Start, H) >= STAR_HOLE.Outer + 1.5 && PassMaskProfileOf(P, E, Hd, He, Ex)) return true;
			}
			return false;
		};
		TArray<FPassMet> Out;
		double SlotFree[PASSAGE_SLOTS], LowFree[PASSAGE_LOW_HEADS];
		for (double& F : SlotFree) F = -1;
		for (double& F : LowFree) F = -1;
		double T = .25, NextCross = 1.2;
		while (T < PASSAGE_LEN)
		{
			const double Rate = .8 + 4.2 * Smooth01(T / 5) * (1 - Smooth01((T - 25) / 9));
			FPassMet P;
			if (T >= NextCross ? Cross(P) : Radiant(P))
			{
				int32 Slot = INDEX_NONE;
				for (int32 K = 0; K < PASSAGE_SLOTS && Slot == INDEX_NONE; ++K) if (SlotFree[K] < T) Slot = K;
				if (Slot != INDEX_NONE && T + P.Dur <= PASSAGE_LEN)
				{
					P.At = T; P.Slot = Slot; SlotFree[Slot] = T + P.Dur + PASSAGE_TAIL;
					int32 Lane = INDEX_NONE;
					for (int32 K = 0; K < PASSAGE_LOW_HEADS && Lane == INDEX_NONE; ++K) if (LowFree[K] < T) Lane = K;
					P.bLow = Lane != INDEX_NONE && (P.bCross || R() < .42);
					if (P.bLow) LowFree[Lane] = T + P.Dur + PASSAGE_TAIL;
					if (P.bCross) NextCross = T + 2.2 + R() * 1.6;
					Out.Add(P);
				}
			}
			T += FMath::Max(.04, -FMath::Loge(FMath::Max(1e-6, 1 - R())) / Rate);
		}
		return Out;
	}();
	return Schedule;
}

bool FS47Sky::PassageLayout(const FPassMet& P, double T, double& OutHead, double& OutBack, double& OutFade)
{
	// (meteorStep's own layout and fade, for each streak of the passage; the trail its own share of the way)
	if (!(T >= 0) || T > P.Dur + PASSAGE_TAIL) return false;
	const double K = FMath::Min(1.0, T / P.Dur), Tr = P.Len * P.Trail;
	OutHead = P.Len * K;
	OutBack = FMath::Max(0.0, OutHead - Tr) + Tr * FMath::Max(0.0, T - P.Dur) / PASSAGE_TAIL;
	OutFade = FMath::Min(1.0, T / 0.08) * (T > P.Dur ? 0.6 * (1 - (T - P.Dur) / PASSAGE_TAIL) : 1 - 0.4 * K);
	return true;
}

bool FS47Sky::PassageMaskProfile(const FPassMet& P, double& OutEnter, double& OutHidden, double& OutEmerge)
{
	double HiddenEnd;
	return PassMaskProfileOf(P, OutEnter, OutHidden, HiddenEnd, OutEmerge);
}

TArray<double> FS47Sky::PassageHiddenMoments(bool bLow)
{
	TArray<double> Out;
	for (const FPassMet& P : PassageSchedule())
	{
		double E, Hd, He, Ex;
		if (P.bCross && (!bLow || P.bLow) && PassMaskProfileOf(P, E, Hd, He, Ex)) Out.Add(P.At + (Hd + He) / 2);
	}
	return Out;
}

namespace
{
	/** Per tier (0 High, 1 Low): streaks, most drawn at once (sampled every 10 ms), crossers. Computed once. */
	struct FPassStats { int32 Count[2] = {}, Heads[2] = {}, Cross[2] = {}; };
	const FPassStats& PassStats()
	{
		static const FPassStats S = []
		{
			FPassStats Out;
			const TArray<FS47Sky::FPassMet>& Sch = FS47Sky::PassageSchedule();
			for (int32 Tier = 0; Tier < 2; ++Tier)
			{
				for (const FS47Sky::FPassMet& P : Sch) if (!Tier || P.bLow) { ++Out.Count[Tier]; Out.Cross[Tier] += P.bCross; }
				for (int32 Ms = 0; Ms <= int32((FS47Sky::PASSAGE_LEN + FS47Sky::PASSAGE_TAIL) * 100); ++Ms)
				{
					int32 N = 0;
					for (const FS47Sky::FPassMet& P : Sch) { double H, B, F; N += (!Tier || P.bLow) && FS47Sky::PassageLayout(P, Ms / 100.0 - P.At, H, B, F); }
					Out.Heads[Tier] = FMath::Max(Out.Heads[Tier], N);
				}
			}
			return Out;
		}();
		return S;
	}
}

int32 FS47Sky::PassageCount(bool bLow) { return PassStats().Count[bLow ? 1 : 0]; }
int32 FS47Sky::PassageMaxHeads(bool bLow) { return PassStats().Heads[bLow ? 1 : 0]; }
int32 FS47Sky::PassageCrossers(bool bLow) { return PassStats().Cross[bLow ? 1 : 0]; }

void FS47Sky::PassageReset(bool bDone)
{
	bPassageDone = bDone; PassageT = -1; PassageHigh = -1; QuietLeft = 0;
	PassageHeads = PassageStarted = PassageMasked = PassageHidden = 0;
	PassageSeen.Init(false, PassageSchedule().Num());
}

void FS47Sky::SetPassageTime(double Elapsed, bool bDone)
{
	const TArray<FPassMet>& Sch = PassageSchedule();
	if (PassageSeen.Num() != Sch.Num()) PassageSeen.Init(false, Sch.Num());
	if (bDone && !bPassageDone && PassageStarted > 0)
	{
		// a seen passage: the rare shooting stars keep away a minute longer
		QuietLeft = PASSAGE_QUIET;
		if (bLive) UE_LOG(LogS47Sky, Display, TEXT("S47: meteor passage spent (%d of %d streaks seen, %s); it does not play again"), PassageStarted, PassageCount(bPassageLow), bPassageLow ? TEXT("Low") : TEXT("High"));
	}
	bPassageDone = bDone;
	PassageHeads = PassageMasked = PassageHidden = 0;
	// Hidden (indoors, away, travel, a cinematic) or spent: nothing drawn; what was begun stays begun.
	if (bDone || !FMath::IsFinite(Elapsed) || Elapsed < 0) { PassageT = -1; return; }
	// Never back: a resumed passage continues where it was, an earlier time cannot replay any streak.
	PassageHigh = FMath::Max(PassageHigh, FMath::Min(Elapsed, PASSAGE_LEN + PASSAGE_TAIL));
	PassageT = PassageHigh;
	for (int32 I = 0; I < Sch.Num(); ++I)
	{
		const FPassMet& P = Sch[I];
		double Hd, Bk, Fd;
		if ((bPassageLow && !P.bLow) || !PassageLayout(P, PassageT - P.At, Hd, Bk, Fd)) continue;
		++PassageHeads;
		// against the sky's own field (the same HoleDir / HoleR chords the material masks with)
		double Near = 4;
		for (int32 K = 0; K <= 16; ++K) Near = FMath::Min(Near, (PassAlong(P, Bk + (Hd - Bk) * K / 16) - HoleDir).Size());
		PassageMasked += Near < HoleR.Y;
		PassageHidden += Hd > Bk && (PassAlong(P, Hd) - HoleDir).Size() < HoleR.X && (PassAlong(P, Bk) - HoleDir).Size() < HoleR.X;
		if (!PassageSeen[I])
		{
			PassageSeen[I] = true; ++PassageStarted;
			if (bLive && PassageStarted == 1) UE_LOG(LogS47Sky, Display, TEXT("S47: meteor passage begins at %.1f s (%s, %d streaks, %d across the field)"), PassageT, bPassageLow ? TEXT("Low") : TEXT("High"), PassageCount(bPassageLow), PassageCrossers(bPassageLow));
		}
	}
}

// ---------- the developer menu's meteor shower ----------

void FS47Sky::ShowerClear() { for (FShowerMet& S : Shower) S.T = -1; }

void FS47Sky::ShowerBegin()
{
	ShowerClear();
	// somewhere high, any direction: the streaks fan out from it
	ShowerRadiant = S47SkyDir(Rand() * 360, 50 + Rand() * 20);
}

bool FS47Sky::ShowerMeteor(double Ahead)
{
	int32 Free = INDEX_NONE;
	for (int32 K = 0; K < SHOWER_SLOTS && Free == INDEX_NONE; ++K) if (Shower[K].T < 0) Free = K;
	if (Free == INDEX_NONE) return false;
	FPassMet& P = Shower[Free].P;
	const FVector Rd = ShowerRadiant.GetSafeNormal();
	const FVector Up = (FVector(0, 1, 0) - Rd * Rd.Y).GetSafeNormal(), Rt = FVector::CrossProduct(Rd, Up).GetSafeNormal();
	for (int32 Try = 0; Try < 40; ++Try)
	{
		// 6 to 40 degrees from the radiant, away from it and a little down; begun above 20 degrees
		const double Ang = (6 + Rand() * 34) * DEG, Pa = Rand() * UE_DOUBLE_PI * 2;
		P.Start = (Rd * FMath::Cos(Ang) + (Up * FMath::Cos(Pa) + Rt * FMath::Sin(Pa)) * FMath::Sin(Ang)).GetSafeNormal();
		const FVector Away = (P.Start * FVector::DotProduct(P.Start, Rd) - Rd).GetSafeNormal();
		const FVector Down = (FVector(0, -1, 0) + P.Start * P.Start.Y).GetSafeNormal();
		const FVector Side = FVector::CrossProduct(P.Start, Down).GetSafeNormal();
		P.Tan = Away + Down * 0.25 + Side * ((Rand() * 2 - 1) * 0.2);
		P.Tan = (P.Tan - P.Start * FVector::DotProduct(P.Tan, P.Start)).GetSafeNormal();
		if (P.Start.Y >= FMath::Sin(20 * DEG)) break;
	}
	// as meteorNow draws the rare one
	P.Len = (8 + Rand() * 10) * DEG;
	P.Dur = 0.45 + Rand() * 0.5;
	P.Bright = Rand() < 0.15 ? 2.6 : 1.1 + Rand() * 0.6;
	P.Trail = .65; P.At = 0; P.bCross = false; P.bLow = true; P.Slot = Free;
	Shower[Free].T = FMath::Clamp(Ahead, 0.0, P.Dur);
	Shower[Free].bFresh = true; // DevEvents already supplied the age within this frame.
	++ShowerStarted; ++Meteors;
	return true;
}

void FS47Sky::ShowerStep(double Dt)
{
	for (FShowerMet& S : Shower)
	{
		if (S.T < 0) continue;
		// DevEvents spawns before Life.Update. Ahead is the time since the emission within
		// this frame, so only existing streaks receive the full frame duration.
		if (!S.bFresh && !bShowerHeld && Dt > 0) S.T += Dt;
		S.bFresh = false;
		if (S.T > S.P.Dur + PASSAGE_TAIL) S.T = -1;
	}
}

int32 FS47Sky::ShowerLive() const
{
	int32 N = 0;
	for (const FShowerMet& S : Shower) N += S.T >= 0;
	return N;
}

int32 FS47Sky::ShowerSlotsVisible() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<UStaticMeshComponent>& C : ShowerComps) N += C.IsValid() && C->IsVisible();
	return N;
}

int32 FS47Sky::PassageSlotsVisible() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<UStaticMeshComponent>& C : PassComps) N += C.IsValid() && C->IsVisible();
	return N;
}

double FS47Sky::PassageLongest() const
{
	double Top = -1;
	if (!PassageActive()) return Top;
	for (const FPassMet& P : PassageSchedule())
	{
		const double T = PassageT - P.At;
		if ((!bPassageLow || P.bLow) && T >= 0 && T <= P.Dur) Top = FMath::Max(Top, T / P.Dur);
	}
	return Top;
}

FString FS47Sky::PassageDebug() const
{
	for (const TWeakObjectPtr<UStaticMeshComponent>& C : PassComps)
		if (C.IsValid() && C->IsVisible())
			return FString::Printf(TEXT("%s bounds origin %s radius %.1f boundsScale %.1f rendered %d"), *C->GetName(), *C->Bounds.Origin.ToString(), C->Bounds.SphereRadius, C->BoundsScale, C->WasRecentlyRendered(.2f));
	return TEXT("no strip drawn");
}

bool FS47Sky::PassageSelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What)
	{
		Lines.Add((bCond ? TEXT("ok   meteor passage: ") : TEXT("FAIL meteor passage: ")) + What); bOk &= bCond;
		UE_LOG(LogS47Sky, Display, TEXT("S47: %s"), *Lines.Last());
	};
	const TArray<FPassMet>& Sch = PassageSchedule();
	const FVector H = PassHoleCentre();
	{
		// the one field: the sky's own mask (stars, every strip, the clouds' clear direction) is the shared spec
		const FS47Sky Fresh;
		Check(STAR_HOLE.Az == 26 && STAR_HOLE.El == 32 && STAR_HOLE.Inner == 6 && STAR_HOLE.Outer == 8 && Fresh.HoleDir.Equals(H, 1e-12)
			&& FMath::IsNearlyEqual(Fresh.HoleR.X, Chord(STAR_HOLE.Inner), 1e-12) && FMath::IsNearlyEqual(Fresh.HoleR.Y, Chord(STAR_HOLE.Outer), 1e-12)
			&& !Fresh.PassageActive() && Fresh.PassageT < 0 && Fresh.PassageStarted == 0 && !Fresh.bPassageDone,
			TEXT("one shared field at 026 / 32 (rims 6 / 8 degrees) masks the stars and every strip; a new sky draws no passage"));
	}
	bool bUp = true, bClear = true, bInside = true, bSorted = true, bCross = true, bSlots = true;
	double MinOff = 180;
	TArray<TArray<const FPassMet*>> BySlot; BySlot.SetNum(PASSAGE_SLOTS);
	for (int32 I = 0; I < Sch.Num(); ++I)
	{
		const FPassMet& P = Sch[I];
		for (int32 K = 0; K <= 48; ++K)
		{
			const FVector D = PassAlong(P, P.Len * K / 48);
			bUp &= D.Y >= FMath::Sin(17.9 * DEG) && D.Y <= FMath::Sin(78.1 * DEG);
			if (!P.bCross) { const double Off = PassAngleDeg(D, H); MinOff = FMath::Min(MinOff, Off); bClear &= Off >= STAR_HOLE.Outer + .7; }
		}
		bInside &= P.At >= 0 && P.At + P.Dur <= PASSAGE_LEN && P.Dur >= .4 && P.Dur <= 2 && FMath::Abs(FVector::DotProduct(P.Start, P.Tan)) < 1e-9 && P.Trail > 0 && P.Trail < 1;
		bSorted &= I == 0 || Sch[I - 1].At < P.At;
		if (P.bCross)
		{
			double E, Hd, Ex;
			bCross &= PassageMaskProfile(P, E, Hd, Ex) && PassAngleDeg(P.Start, H) >= STAR_HOLE.Outer + 1.4 && E <= Hd && Ex < P.Dur;
		}
		if (P.Slot >= 0 && P.Slot < PASSAGE_SLOTS) BySlot[P.Slot].Add(&P); else bSlots = false;
	}
	for (const TArray<const FPassMet*>& On : BySlot)
		for (int32 J = 1; J < On.Num(); ++J) bSlots &= On[J]->At > On[J - 1]->At + On[J - 1]->Dur + PASSAGE_TAIL;
	const int32 NH = PassageCount(false), NL = PassageCount(true);
	Check(NH >= 80 && NL >= 30 && NL < NH, FString::Printf(TEXT("a dense passage: %d streaks on High, %d of them on Low"), NH, NL));
	Check(PassageMaxHeads(false) >= 6 && PassageMaxHeads(false) <= PASSAGE_SLOTS && PassageMaxHeads(true) >= 3 && PassageMaxHeads(true) <= PASSAGE_LOW_HEADS,
		FString::Printf(TEXT("up to %d drawn at once on High (6 to %d strips), %d on Low (3 to %d)"), PassageMaxHeads(false), PASSAGE_SLOTS, PassageMaxHeads(true), PASSAGE_LOW_HEADS));
	Check(PassageCrossers(false) >= 6 && PassageCrossers(true) >= 3 && bCross,
		FString::Printf(TEXT("%d cross behind the field on High, %d on Low: in from beyond the outer rim, head and whole trail gone inside the inner one, the head out beyond the outer rim again in flight"), PassageCrossers(false), PassageCrossers(true)));
	Check(bClear && bUp, FString::Printf(TEXT("the others radiate from beside it, never nearer than %.1f degrees (outer rim %.0f); all stay 18 to 78 degrees up"), MinOff, STAR_HOLE.Outer));
	Check(bInside && bSorted && bSlots, TEXT("every streak inside the 36 s, in order, and no two drawn at once share a strip of the pool"));
	{
		// dense for its whole length, not one burst
		bool bSpread = Sch.Num() > 0 && Sch.Last().At >= 30;
		for (double W0 = 0; W0 < 32; W0 += 4) { int32 N = 0; for (const FPassMet& P : Sch) N += P.At >= W0 && P.At < W0 + 4; bSpread &= N >= 2; }
		for (double W0 = 0; W0 < 30; W0 += 6) { int32 N = 0; for (const FPassMet& P : Sch) N += P.bLow && P.At >= W0 && P.At < W0 + 6; bSpread &= N >= 1; }
		Check(bSpread, TEXT("streaks all through it: some every 4 s on High and every 6 s on Low, the last ones after 30 s"));
	}
	// Life's presentation time: the same value holds, hidden keeps the progress, nothing runs back or replays.
	double Busy = 1; int32 BusyHeads = 0;
	for (int32 Ms = 100; Ms <= 3000; ++Ms)
	{
		int32 N = 0;
		for (const FPassMet& P : Sch) { double A, B, F; N += PassageLayout(P, Ms / 100.0 - P.At, A, B, F); }
		if (N > BusyHeads) { BusyHeads = N; Busy = Ms / 100.0; }
	}
	FS47Sky S;
	S.SetPassageTime(-1);
	const bool bBefore = !S.PassageActive() && S.PassageHeads == 0 && S.PassageStarted == 0 && !S.bPassageDone;
	S.SetPassageTime(Busy); const int32 Heads = S.PassageHeads, Started = S.PassageStarted;
	S.SetPassageTime(Busy);
	const bool bHeld = S.PassageActive() && Heads >= 3 && S.PassageT == Busy && S.PassageHeads == Heads && S.PassageStarted == Started && Started == Heads;
	S.SetPassageTime(-1);
	const bool bHidden = !S.PassageActive() && S.PassageHeads == 0 && S.PassageStarted == Started && !S.bPassageDone;
	S.SetPassageTime(Busy);
	const bool bResumed = S.PassageT == Busy && S.PassageHeads == Heads && S.PassageStarted == Started;
	S.SetPassageTime(Busy * .5);
	const bool bNoBack = S.PassageT == Busy && S.PassageHeads == Heads && S.PassageStarted == Started;
	S.HoleFrom = 2 * 3600 + 15 * 60 + 12 + STAR_HOLE.After;
	S.SetClock(S.HoleFrom + 3600); S.SetClock(14 * 3600); S.SetClock(S.HoleFrom - 30);
	const bool bClockFree = S.PassageT == Busy && S.PassageHeads == Heads && !S.bPassageDone;
	Check(bBefore && bHeld && bHidden && bResumed && bNoBack && bClockFree,
		FString::Printf(TEXT("held at %.2f s with %d drawn; hidden keeps it begun, not spent; resumed the same, never back; the clock neither starts nor spends it"), Busy, Heads));
	for (const bool bLowTier : { false, true })
	{
		FS47Sky F; F.bPassageLow = bLowTier;
		int32 Top = 0, Masked = 0, Hidden = 0;
		for (int32 Ms = 0; Ms <= int32((PASSAGE_LEN + PASSAGE_TAIL) * 100); ++Ms)
		{
			F.SetPassageTime(Ms / 100.0);
			Top = FMath::Max(Top, F.PassageHeads); Masked = FMath::Max(Masked, F.PassageMasked); Hidden = FMath::Max(Hidden, F.PassageHidden);
		}
		const bool bAll = F.PassageStarted == PassageCount(bLowTier) && Top == PassageMaxHeads(bLowTier) && Top <= (bLowTier ? PASSAGE_LOW_HEADS : PASSAGE_SLOTS) && Hidden >= 1 && Masked >= Hidden;
		F.SetPassageTime(-1, true);
		const bool bSpent = F.bPassageDone && !F.PassageActive() && F.PassageHeads == 0;
		F.SetPassageTime(5, true);
		const bool bStays = F.bPassageDone && F.PassageHeads == 0 && F.PassageT < 0 && F.PassageStarted == PassageCount(bLowTier);
		Check(bAll && bSpent && bStays, FString::Printf(TEXT("%s played through: all %d streaks once, %d at most at once, %d reaching into the field and %d wholly behind it at once; spent by Life, it draws nothing again"),
			bLowTier ? TEXT("Low") : TEXT("High"), F.PassageStarted, Top, Masked, Hidden));
	}
	{
		bool bMoments = true; int32 N = 0;
		for (const bool bLowTier : { false, true })
			for (const double At : PassageHiddenMoments(bLowTier))
			{
				FS47Sky F; F.bPassageLow = bLowTier; F.SetPassageTime(At);
				bMoments &= F.PassageHidden >= 1 && F.PassageMasked >= F.PassageHidden && F.PassageHeads >= F.PassageMasked; ++N;
			}
		Check(bMoments && N == PassageCrossers(false) + PassageCrossers(true), FString::Printf(TEXT("at each of the %d hidden moments a crossing streak lies wholly behind the shared mask"), N));
	}
	FS47Sky Ld; Ld.PassageReset(false); Ld.SetPassageTime(Busy);
	Check(Ld.PassageActive() && Ld.PassageStarted == Ld.PassageHeads && Ld.PassageHeads == Heads, TEXT("a load mid-way resumes at Life's saved time, with no backlog of the streaks before it"));
	FS47Sky Dn; Dn.PassageReset(true); Dn.SetPassageTime(-1, true);
	const bool bLoadedSpent = !Dn.PassageActive() && Dn.PassageHeads == 0 && Dn.bPassageDone;
	Dn.PassageReset(false); Dn.SetPassageTime(Busy);
	Check(bLoadedSpent && !Dn.bPassageDone && Dn.PassageHeads == Heads, TEXT("a saved spent passage stays spent after a load; only a genuine reset (a new night) clears it"));
	{
		// the rare shooting stars: as before, but none for a minute after a passage that was seen
		const double Night = 2 * 3600 + 20 * 60;
		FS47Sky Q; Q.SetClock(Night); Q.SetPassageTime(Busy); Q.SetPassageTime(-1, true); Q.Update(11, 11);
		FS47Sky O; O.SetClock(Night); O.Update(11, 11);
		FS47Sky U; U.SetClock(Night); U.PassageReset(true); U.SetPassageTime(-1, true); U.Update(11, 11);
		Check(Q.Meteors == 0 && O.Meteors == 1 && U.Meteors == 1, TEXT("the rare shooting star still comes at night, but keeps away for a minute after a seen passage"));
	}
	{
		// the developer shower: several on strips of their own at once, frozen when held, gone in time
		FS47Sky W; W.ShowerBegin();
		const bool bThree = W.ShowerMeteor(0) && W.ShowerMeteor(.1) && W.ShowerMeteor(.2) && W.ShowerLive() == 3;
		int32 More = 0;
		while (More < 100 && W.ShowerMeteor()) ++More;
		const bool bFull = W.ShowerLive() == SHOWER_SLOTS && More == SHOWER_SLOTS - 3 && !W.ShowerMeteor();
		bool bShape = true;
		for (int32 K = 0; K < SHOWER_SLOTS; ++K) bShape &= W.Shower[K].P.Slot == K && FMath::Abs(FVector::DotProduct(W.Shower[K].P.Start, W.Shower[K].P.Tan)) < 1e-9;
		W.bShowerHeld = true; W.Update(5, 5);
		const bool bHeldShower = W.ShowerLive() == SHOWER_SLOTS;
		W.bShowerHeld = false; W.Update(2, 7);
		const bool bGone = W.ShowerLive() == 0;
		FS47Sky Frame; Frame.ShowerBegin(); Frame.ShowerMeteor(.18);
		Frame.Update(.30, .30);
		const bool bOneAdvance = FMath::IsNearlyEqual(Frame.Shower[0].T, .18, 1e-12);
		Frame.Update(.10, .40);
		Check(bOneAdvance && FMath::IsNearlyEqual(Frame.Shower[0].T, .28, 1e-12),
			TEXT("developer shower: emission 0.12 s into a 0.30 s frame is aged 0.18 once, then advances normally"));
		FS47Sky C; C.ShowerBegin();
		double Next = 0; int32 Top = 0;
		for (int32 Fr = 0; Fr < 40 * 60; ++Fr)
		{
			Next -= 1 / 60.0;
			if (Next <= 0) { C.ShowerMeteor(); Next += SHOWER_GAP_MIN + (SHOWER_GAP_MAX - SHOWER_GAP_MIN) * Rand(); }
			C.Update(1 / 60.0, Fr / 60.0);
			Top = FMath::Max(Top, C.ShowerLive());
		}
		Check(bThree && bFull && bShape && bHeldShower && bGone && Top >= 3 && C.ShowerStarted >= 90,
			FString::Printf(TEXT("developer shower: %d strips, %d streaks in 40 s, up to %d crossing at once; held it freezes, then they finish"), SHOWER_SLOTS, C.ShowerStarted, Top));
	}
	return bOk;
}

void FS47Sky::SetClock(double Clock, TOptional<double> SunElevDeg)
{
	const double S = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
	PureSkyWeight = PureSkyWeightAt(Clock);
	const double K = S > 12 * 3600 ? 0 : Clamp01((S - (4 * 3600 + 40 * 60)) / (50 * 60));
	const double D = FMath::Pow(K, 1.6) * DawnLift;
	Dawn = D;
	// Rendering-only daytime plateau. Never raise Dawn: chapters and anomaly gates retain
	// the original 04:40-05:40 curve and per-area lift. Both joins have zero slope.
	Midday = Smooth01((S - (5 * 3600 + 40 * 60)) / (20 * 60)) *
		(1 - Smooth01((S - 17 * 3600) / 3600));
	// EveningShift extends the existing dawn palette backwards. Keep this separate from Dawn:
	// anomaly/story gates query the morning curve, while drawing and native light use Daylight().
	// Art-directed MST reference: sunset 18:27, civil twilight 18:53, last blue hour 19:07.
	// Keep the sunlit palette through shift arrival. The old full-night-to-dawn shader
	// otherwise reads 18:00 as dark twilight despite the sun still being 4.53 degrees up.
	const double EveningK = S > 12 * 3600 ? Clamp01(((19 * 3600 + 7 * 60) - S) / (67 * 60)) : 0;
	Evening = FMath::Pow(EveningK * EveningK * (3 - 2 * EveningK), 1.4);
	// from 02:15, where the dishes point (it fades with the stars at dawn, by itself)
	HoleK = S47StarHoleAt(Clock, HoleFrom);
	// (FX-01: the passage's time is Life's, SetPassageTime; the clock never starts or spends it)
	CloudCover = CloudCoverAt(Clock); Cirrus = CirrusAt(Clock);
	// (a chapter that sets the sun's height has its own light on the ground, approved as it is)
	Twilight = SunElevDeg.IsSet() ? 0 : TwilightAt(Clock);
	const FLinearColor A(0.043f, 0.059f, 0.098f), B(0.19f, 0.2f, 0.25f);
	FogColor = A + (B - A) * float(D * 0.8);
	// the sun: azimuth 79 degrees, rising about 0.18 degrees a minute; its upper limb reaches
	// the horizon at 05:30, so the glow shows a few minutes before
	const double El = (SunElevDeg.IsSet() ? *SunElevDeg : 0.33 + (S - (5 * 3600 + 30 * 60)) / 60 * 0.18) * UE_DOUBLE_PI / 180, Az = 79 * UE_DOUBLE_PI / 180;
	SunDir = FVector(FMath::Sin(Az) * FMath::Cos(El), FMath::Sin(El), -FMath::Cos(Az) * FMath::Cos(El));
	Sun = S > 12 * 3600 ? 0 : SunElevDeg.IsSet() ? 1 : Clamp01((S - (5 * 3600 + 26 * 60)) / 240) * DawnLift;
	if (Evening > 0)
	{
		const double Sunset = 18 * 3600 + 27 * 60;
		const double EveningEl = -0.33 + (Sunset - S) / 60 * 0.18;
		// Western sun: never mirror the old eastern sunrise or drag it round after dusk.
		SunDir = S47SkyDir(281, EveningEl);
		Sun = Clamp01((EveningEl + 0.55) / 1.5);
		// Warm dust at sunset, fading back to the original cold night air at 19:07.
		const float Amber = float(4 * Evening * (1 - Evening));
		const FLinearColor Haze = FMath::Lerp(FLinearColor(.34f, .26f, .24f), FLinearColor(.48f, .25f, .12f), Amber);
		FogColor = A + (Haze - A) * float(Evening * 0.8);
	}
	if (Midday > 0)
	{
		// A modest April/New Mexico solar approximation: north maps to -Z in three,
		// with solar noon at 12:00. Unlike the sunrise extrapolation, it crosses the
		// southern sky and cannot turn underground after noon. This is art calibration,
		// not a replacement for the authored 18:27 sunset or the finale's elevation.
		if (!SunElevDeg.IsSet())
		{
			const double Latitude = 33.4 * DEG, Declination = 9.1 * DEG;
			const double HourAngle = (S - 12 * 3600) * UE_DOUBLE_PI / (12 * 3600);
			const double East = -FMath::Cos(Declination) * FMath::Sin(HourAngle);
			const double Up = FMath::Sin(Latitude) * FMath::Sin(Declination) + FMath::Cos(Latitude) * FMath::Cos(Declination) * FMath::Cos(HourAngle);
			const double North = FMath::Cos(Latitude) * FMath::Sin(Declination) - FMath::Sin(Latitude) * FMath::Cos(Declination) * FMath::Cos(HourAngle);
			const FVector DaySun(East, Up, -North);
			SunDir = (SunDir * (1 - Midday) + DaySun * Midday).GetSafeNormal();
			Sun = Sun + (1 - Sun) * Midday;
		}
		// The dome daylight and horizon band's Day texture receive Daylight()==1. Cool the shared
		// haze too; this blend is zero throughout the documented dawn and evening fades.
		FogColor = FogColor + (FLinearColor(0.30f, 0.40f, 0.55f) - FogColor) * float(Midday);
	}
}

double FS47Sky::PureSkyWeightAt(double Clock)
{
	const double S = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
	// Independent of Dawn/Evening and story gates. Fade out before the night stars return.
	return Smooth01((S - 6 * 3600) / 3600) * (1 - Smooth01((S - 18 * 3600) / (53 * 60)));
}

double FS47Sky::TwilightAt(double Clock)
{
	const double S = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
	// Sunset 18:27 and sunrise 05:30. The sky stays bright for half an hour past the sun, and the ground under it is plain to see. Up as the
	// low sun stops reaching the ground, whole at 18:30, falling with the sky's own brightness (the land should not outshine it: 18:40 read about
	// 60 in the picture, 18:46 about 35, 19:04 about 20), and exactly nothing from 19:30, so that the night is what it was.
	if (S > 12 * 3600) return Smooth01((S - (17 * 3600 + 55 * 60)) / (35 * 60)) * FMath::Square(1 - Smooth01((S - (18 * 3600 + 30 * 60)) / (60 * 60)));
	// The same at dawn, mirrored about sunrise, and given over to the day as the sun climbs and Midday comes in (05:40 to 06:00).
	return FMath::Square(Smooth01((S - (4 * 3600 + 27 * 60)) / (60 * 60))) * (1 - Smooth01((S - (5 * 3600 + 35 * 60)) / (30 * 60)));
}

double FS47Sky::CloudCoverAt(double Clock)
{
	const double H = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0) / 3600;
	// A slow, deterministic variation over the day: broken alto/stratocumulus, 40-70 %.
	const double Day = 0.55 + 0.11 * FMath::Sin(H * 0.83 + 0.6) + 0.04 * FMath::Sin(H * 2.3 + 1.1);
	// A few scattered clouds that now and then cross the moon; never a closed overcast.
	const double Night = 0.11 + 0.04 * FMath::Sin(H * UE_DOUBLE_PI / 6 + 0.4); // 24 h periodic: no jump at midnight
	const double K = H >= 12 ? 1 - Smooth01((H - 18.8) / 2.2) : Smooth01((H - 4.5) / 2.0);
	// The dusk concept: a broad broken layer over the commute and the handover (about 17:40-19:00).
	const double Dusk = FMath::Exp(-FMath::Square((H - 18.35) / 0.55));
	return FMath::Min(0.7, Night + (Day + 0.16 * Dusk - Night) * K);
}

double FS47Sky::CirrusAt(double Clock)
{
	const double H = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0) / 3600;
	const double K = H >= 12 ? 1 - Smooth01((H - 19.0) / 2.5) : Smooth01((H - 4.2) / 2.0);
	return 0.12 + 0.36 * K + 0.08 * FMath::Sin(H * UE_DOUBLE_PI / 12 + 2.0);
}

void FS47Sky::SetCometClock(double Clock)
{
	const double S = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
	bComet = S > 20 * 3600 || S < 3 * 3600 + 5 * 60;
}

void FS47Sky::SetMoon(bool bShow, double Az, double El, double Lit, double SunAz, double SunEl)
{
	bMoon = bShow; MoonAz = Az; MoonEl = El; MoonLit = FMath::Clamp(Lit, 0.0, 1.0); MoonSunAz = SunAz; MoonSunEl = SunEl;
	MoonStarDim = bShow ? .5 * MoonLit * MoonLit * FMath::SmoothStep(-2.0, 15.0, El) : 0.0;
}

bool FS47Sky::MoonWorld(FVector& Out) const
{
	if (!MoonComp.IsValid() || !MoonComp->IsVisible()) return false;
	Out = MoonComp->GetComponentLocation();
	return true;
}

bool FS47Sky::MoonDrawsTexture() const
{
	return MoonComp.IsValid() && MoonComp->IsVisible() && MoonTexMid.IsValid() && MoonComp->GetMaterial(0) == MoonTexMid.Get();
}

FString FS47Sky::MoonDrawn() const
{
	if (!MoonComp.IsValid() || !MoonComp->IsVisible()) return TEXT("hidden");
	const UMaterialInterface* On = MoonComp->GetMaterial(0);
	if (MoonMid.IsValid() && On == MoonMid.Get()) return FString::Printf(TEXT("painted disc (M_Web_Unlit_Add, phase painted %.2f)"), MoonPainted);
	if (!MoonTexMid.IsValid() || On != MoonTexMid.Get()) return FString::Printf(TEXT("other material %s"), On ? *On->GetPathName() : TEXT("(none)"));
	// the MID that is on the plane, as it is set this frame (S47Sky::Draw): the light, the sun's side, the map
	const UMaterialInstanceDynamic* T = MoonTexMid.Get();
	FLinearColor Color = FLinearColor::Black, SunL = FLinearColor::Black;
	float Flip = 0, Earth = 0, OccLit = 0, OccDark = 0;
	T->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Color);
	T->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("SunL")), SunL);
	T->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Flip")), Flip);
	T->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Earth")), Earth);
	T->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("OccLit")), OccLit);
	T->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("OccDark")), OccDark);
	UTexture* Map = nullptr;
	T->GetTextureParameterValue(FHashedMaterialParameterInfo(TEXT("Map")), Map);
	FString MapNote = TEXT("Map missing");
	if (UTexture2D* Tex = Cast<UTexture2D>(Map))
		MapNote = FString::Printf(TEXT("Map %s %dx%d, mips %d resident %d, group %d, %s"), *Tex->GetName(), Tex->GetSizeX(), Tex->GetSizeY(),
			Tex->GetNumMips(), Tex->GetNumResidentMips(), int32(Tex->LODGroup), Tex->NeverStream ? TEXT("never streamed") : TEXT("streamed"));
	else if (Map) MapNote = FString::Printf(TEXT("Map %s (not a 2D texture)"), *Map->GetName());
	return FString::Printf(TEXT("M_S47_Moon (%s; Color %.2f %.2f %.2f, SunL %.2f %.2f %.2f, Flip %+.0f, Earth %.3f, OccLit %.2f, OccDark %.2f)"),
		*MapNote, Color.R, Color.G, Color.B, SunL.R, SunL.G, SunL.B, Flip, Earth, OccLit, OccDark);
}

void FS47Sky::PaintMoon()
{
	// The disc lit on its +U side, as much as its phase: the terminator is an ellipse across it. A little earthshine on the rest,
	// and a few darker seas, the same every night.
	constexpr int32 Size = 128;
	const double K = 1 - 2 * MoonLit; // 1 new, 0 half, -1 full
	TArray<FColor> Px; Px.SetNumZeroed(Size * Size);
	for (int32 Y = 0; Y < Size; ++Y) for (int32 X = 0; X < Size; ++X)
	{
		const double U = (X + .5) / Size * 2 - 1, V = (Y + .5) / Size * 2 - 1, R2 = U * U + V * V;
		if (R2 > 1) continue;
		const double Soft = FMath::Clamp((1 - FMath::Sqrt(R2)) * Size * .5, 0.0, 1.0); // a soft rim, one pixel
		const double Term = K * FMath::Sqrt(FMath::Max(0.0, 1 - V * V));
		const double Lit = FMath::Clamp((U - Term) * Size * .25 + .5, 0.0, 1.0);
		const double Sea = .82 + .18 * FMath::Clamp(FMath::Sin(U * 5.1 + 1.3) * FMath::Cos(V * 4.3 - .4) * 1.4 + FMath::Sin(U * 11.7 - V * 9.2) * .35, -1.0, 1.0);
		const double Shine = (Lit * Sea + (1 - Lit) * .035) * Soft;
		const uint8 G = uint8(FMath::Clamp(FMath::RoundToInt(Shine * 255), 0, 255));
		Px[Y * Size + X] = FColor(G, uint8(G * .97), uint8(G * .9), G);
	}
	if (UTexture2D* Disc = S47Image::FromPixels(Px, Size, Size); Disc && MoonMid.IsValid())
	{
		MoonMid->SetScalarParameterValue(TEXT("UseMap"), 1.f);
		MoonMid->SetTextureParameterValue(TEXT("Map"), Disc);
	}
	MoonPainted = MoonLit;
}

void FS47Sky::Update(double Dt, double T)
{
	Time = T;
	MeteorStep(Dt);
	ShowerStep(Dt);
	// Sunlit daytime and the friendly arrival have no forced night storm or thunder.
	if (Evening > 0 || Midday > 0)
	{
		Flash = 0; FlashT = 0; Restrikes = 0;
		// (WEATHER-20261108: a shower's afternoon has thunder a long way off, heard and not seen)
		if (DayThunder > 0 && (DayThunderIn -= Dt) <= 0)
		{
			DayThunderIn = 20 + Rand() * 45;
			if (OnThunder) OnThunder(0.5 + Rand() * 2, (0.25 + Rand() * 0.35) * FMath::Min(1.0, DayThunder));
		}
		return;
	}
	NextFlash -= Dt;
	const double Rate = LightningRate();
	if (NextFlash <= 0 && Rate < QuietRate)
	{
		// D46-20261110: a quiet night, nothing on the horizon (looked at again a little later, should the night change)
		NextFlash = 10; Restrikes = 0;
	}
	else if (NextFlash <= 0 && Restrikes > 0)
	{
		// a storm's flash strikes again a moment later, in the same place, its thunder already on its way
		--Restrikes; FlashT = 1; FlashPeak = .7 + .3 * Rand();
		NextFlash = Restrikes > 0 ? .25 + Rand() * .45 : (14 + Rand() * 26) / FMath::Pow(Rate, 1.5);
	}
	else if (NextFlash <= 0)
	{
		FlashT = 1;
		if (Rate >= StormRate)
		{
			// the storm on the horizon, where the weather has it: often, bright, two or three strokes, thunder sooner and louder
			const double A = FMath::DegreesToRadians(S47Weather::Live().LightningAz) + (Rand() - .5) * .4;
			FlashDir = FVector(FMath::Sin(A), 0.03 + Rand() * 0.03, -FMath::Cos(A)).GetSafeNormal();
			FlashPeak = 1; Restrikes = Rand() < .6 ? 1 + int32(Rand() * 2) : 0;
			NextFlash = Restrikes > 0 ? .25 + Rand() * .45 : (14 + Rand() * 26) / FMath::Pow(Rate, 1.5);
			if (OnThunder) OnThunder(2.5 + Rand() * 3.5, FMath::Min(1.0, 0.55 + Rand() * 0.45));
		}
		else
		{
			// the sky's own far-off flicker (Rate 1: as it always was); fainter and rarer on a night with little of it
			NextFlash = (14 + Rand() * 26) / FMath::Pow(FMath::Max(Rate, QuietRate), 1.5);
			const double A = -0.2 + Rand() * 1.6;
			FlashDir = FVector(FMath::Sin(A), 0.04, -FMath::Cos(A)).GetSafeNormal();
			FlashPeak = FMath::Min(1.0, Rate);
			const double Delay = 3 + Rand() * 5, Loud = (0.4 + Rand() * 0.6) * FMath::Min(1.0, Rate);
			if (OnThunder) OnThunder(Delay, Loud);
		}
	}
	if (FlashT > 0)
	{
		FlashT = FMath::Max(0.0, FlashT - Dt * 2.2);
		const double Flick = FlashT > 0.55 ? (FMath::Sin(T * 90) > 0 ? 1 : 0.3) : FlashT;
		Flash = Flick * FlashT * FlashPeak;
	}
	else Flash = 0;
}

double FS47Sky::LightningRate()
{
	const S47Weather::FLive& Wx = S47Weather::Live();
	return Wx.bOn ? FMath::Clamp(Wx.Lightning, 0.0, 2.5) : 1.0;
}

// ---------- Unreal ----------

bool FS47Sky::Build(AS47Web* W)
{
	Web = W;
	const TSharedPtr<FJsonObject>& L = W ? W->LifeData() : nullptr;
	const TSharedPtr<FJsonObject>* SkyJ = nullptr;
	if (!L.IsValid() || !L->TryGetObjectField(TEXT("sky"), SkyJ)) return false;
	UMaterialInterface* DomeBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebSkyDome.M_WebSkyDome"));
	// A locally calibrated sky keeps the existing night/story uniforms and the old import as fallback.
	if (UMaterialInterface* Concept = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/CalicheStreet/Atmosphere/M_Caliche_Sky.M_Caliche_Sky"))) DomeBase = Concept;
	UMaterialInterface* StarsBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebStars.M_WebStars"));
	if (UMaterialInterface* Concept = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/CalicheStreet/Atmosphere/M_Caliche_Stars.M_Caliche_Stars"))) StarsBase = Concept;
	UMaterialInterface* MeteorBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebMeteor.M_WebMeteor"));
	UMaterialInterface* BandBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebSkyBand.M_WebSkyBand"));
	if (UMaterialInterface* Concept = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/CalicheStreet/Atmosphere/M_Caliche_Horizon.M_Caliche_Horizon"))) BandBase = Concept;
	if (!DomeBase || !StarsBase || !MeteorBase || !BandBase)
	{
		UE_LOG(LogS47Sky, Warning, TEXT("S47: the live sky's materials are not imported (Tools/webexport/import.sh): the sky stays the export's picture"));
		return false;
	}
	const TSharedPtr<FJsonObject>& S = *SkyJ;
	auto Num = [&S](const TCHAR* K, double D) { double V = D; S->TryGetNumberField(K, V); return V; };
	auto Tex = [](int32 Id) { return Id >= 0 ? LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Web/Tex/T%d.T%d"), Id, Id)) : nullptr; };
	GroupNode = int32(Num(TEXT("group"), -1));
	const FS47WebNode* G = W->Node(GroupNode);
	if (!G || !G->Comp.IsValid()) return false;
	ConceptBackdropNode = -1;
	for (int32 I = 0; I < W->PartCount(); ++I)
		if (const FS47WebNode* N = W->Node(I); N && N->Name == TEXT("prop:streetGroundMesas"))
			if (UStaticMeshComponent* C = Cast<UStaticMeshComponent>(N->Comp.Get()); C && C->GetStaticMesh()) ConceptBackdropNode = I;
	// Original horizon.ts meshes have baked night vertex colours. They stay black at
	// noon; the authored local escarpments replace them only while actually visible.
	ConceptLegacyMesas.Reset(); bConceptLegacyMesasHidden = false;
	for (int32 I = 0; I < W->PartCount(); ++I)
		if (const FS47WebNode* N = W->Node(I); N && N->Geo >= 0 &&
			(N->Name == TEXT("mesas:near") || N->Name == TEXT("mesas")))
			ConceptLegacyMesas.Emplace(I, W->IsVisible(I));
	const TArray<TSharedPtr<FJsonValue>>* HS = nullptr;
	if (S->TryGetArrayField(TEXT("holeSky"), HS) && HS->Num() >= 3) HoleSky = FLinearColor((*HS)[0]->AsNumber(), (*HS)[1]->AsNumber(), (*HS)[2]->AsNumber());
	auto Under = [W](int32 Node) -> USceneComponent* { const FS47WebNode* N = W->Node(Node); return N ? N->Comp.Get() : nullptr; };
	// the dome: Sky.ts's fragment shader with the painted Milky Way (milkyWay, its stars taken out)
	if (const FS47WebNode* N = W->Node(int32(Num(TEXT("dome"), -1))))
	{
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(DomeBase, W);
		if (UTexture2D* P = Tex(int32(Num(TEXT("panorama"), -1)))) D->SetTextureParameterValue(TEXT("Pano"), P);
		D->SetVectorParameterValue(TEXT("HoleSky"), HoleSky);
		DomeMid.Reset(D);
		DomeComp = Part(W, N->Comp.Get(), W->GeoMesh(N->Geo), D, 0);
		// A separate material preserves the original night asset and capture shader byte-for-byte.
		// A partial import or the comparison flag keeps the original sky and cloud values all day.
		PureSkyMid.Reset();
		if (!FParse::Param(FCommandLine::Get(), TEXT("S47NoDaySky")))
		{
			UMaterialInterface* PureBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Sky/PureSky/M_S47_PureSky.M_S47_PureSky"));
			UTexture2D* DayTex = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Sky/PureSky/T_S47_DaySky.T_S47_DaySky"));
			UTexture* BoundTex = nullptr; float Weight = 0, Yaw = 0;
			if (PureBase && DayTex && PureBase->GetTextureParameterValue(FMaterialParameterInfo(TEXT("PureSkyTexture")), BoundTex) && BoundTex == DayTex &&
				PureBase->GetScalarParameterValue(FMaterialParameterInfo(TEXT("PureSkyWeight")), Weight) &&
				PureBase->GetScalarParameterValue(FMaterialParameterInfo(TEXT("PureSkyYaw")), Yaw))
			{
				UMaterialInstanceDynamic* Pure = UMaterialInstanceDynamic::Create(PureBase, W);
				if (UTexture2D* P = Tex(int32(Num(TEXT("panorama"), -1)))) Pure->SetTextureParameterValue(TEXT("Pano"), P);
				Pure->SetVectorParameterValue(TEXT("HoleSky"), HoleSky);
				Pure->SetScalarParameterValue(TEXT("PureSkyWeight"), 0.f);
				// PureSkyYaw and radiance calibration belong to the imported asset, not world/story yaw.
				PureSkyMid.Reset(Pure);
			}
			UE_LOG(LogS47Sky, Display, TEXT("S47: Pure Sky %s; original night material retained"), PureSkyMid.IsValid() ? TEXT("ready") : TEXT("unavailable, legacy fallback"));
		}
	}
	// the stars: a quad each, turned to the eye (custom data: direction, colour, size, phase)
	const int32 StarsNode = int32(Num(TEXT("stars"), -1));
	const TArray<float>* Pts = W->PointsOf(StarsNode);
	const TArray<float>* Ph = W->PointsPhOf(StarsNode);
	if (Pts && Pts->Num() >= 7)
	{
		UInstancedStaticMeshComponent* I = NewObject<UInstancedStaticMeshComponent>(W);
		I->SetMobility(EComponentMobility::Movable);
		I->SetupAttachment(Under(StarsNode));
		I->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(StarsBase, W);
		StarsMid = D;
		I->SetMaterial(0, D);
		I->NumCustomDataFloats = 8;
		I->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		I->SetCastShadow(false);
		I->bAffectDistanceFieldLighting = false;
		I->SetVisibleInRayTracing(false);
		I->SetTranslucentSortPriority(-4);
		const int32 N = Pts->Num() / 7;
		TArray<FTransform> T;
		T.Reserve(N);
		for (int32 k = 0; k < N; ++k)
		{
			const float* P = Pts->GetData() + k * 7;
			T.Add(FTransform(FQuat::Identity, S47Three::ToUE(P[0], P[1], P[2]), FVector(0.001)));
		}
		I->AddInstances(T, false);
		for (int32 k = 0; k < N; ++k)
		{
			const float* P = Pts->GetData() + k * 7;
			const FVector Dir = FVector(P[0], P[1], P[2]).GetSafeNormal();
			const TArray<float> Cd = { float(Dir.X), float(Dir.Y), float(Dir.Z), P[3], P[4], P[5], P[6], Ph && Ph->IsValidIndex(k) ? (*Ph)[k] : 0.f };
			I->SetCustomData(k, Cd, false);
		}
		I->RegisterComponent();
		StarsComp = I;
	}
	// the comet (Halley's, April 1986: its own material from the export)
	if (const FS47WebNode* N = W->Node(int32(Num(TEXT("comet"), -1))))
	{
		CometComp = Part(W, N->Comp.Get(), W->GeoMesh(N->Geo), N->Mats.Num() ? W->MatAsset(N->Mats[0]) : nullptr, -3);
	}
	// the moon (MOON-20261108): a disc turned to the eye, its phase painted, in the sky's own frame at the stars' distance
	if (UMaterialInterface* MoonBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Unlit_Add.M_Web_Unlit_Add")))
	{
		if (Pts && Pts->Num() >= 3) StarRadius = FMath::Max(50.0, FVector((*Pts)[0], (*Pts)[1], (*Pts)[2]).Size());
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(MoonBase, W);
		D->SetVectorParameterValue(TEXT("MapU"), FLinearColor(1, 0, 0, 0));
		D->SetVectorParameterValue(TEXT("MapV"), FLinearColor(0, 1, 0, 0));
		MoonMid.Reset(D);
		// PT-MOON-20261109 (Tom: "månen trenger tekstur", a flat white disc): NASA's near side in its own material, the phase shaded from
		// the sun, hidden by the clouds in front of it (Tools/sky/import_moon.py). The painted disc stays the fallback without it.
		bMoonTex = false;
		// Not in Git until import_moon.sh has run: look for the package first, so a build without it says nothing and asks the disk once.
		UMaterialInterface* TexBase = FPackageName::DoesPackageExist(TEXT("/Game/Sky/M_S47_Moon"))
			? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Sky/M_S47_Moon.M_S47_Moon"), nullptr, LOAD_NoWarn | LOAD_Quiet) : nullptr;
		if (TexBase)
			if (UMaterialInstanceDynamic* T = UMaterialInstanceDynamic::Create(TexBase, W)) { MoonTexMid.Reset(T); bMoonTex = true; }
		UE_LOG(LogS47Sky, Display, TEXT("S47: moon %s"), bMoonTex ? TEXT("textured (M_S47_Moon)") : TEXT("painted disc (M_S47_Moon missing: Tools/sky/import_moon.sh)"));
		MoonComp = Part(W, Under(GroupNode), LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")), bMoonTex ? MoonTexMid.Get() : D, -3);
		if (MoonComp.IsValid()) MoonComp->SetVisibility(false);
		MoonPainted = -1;
	}
	// the shooting star's strip
	if (const FS47WebNode* N = W->Node(int32(Num(TEXT("meteor"), -1))))
	{
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(MeteorBase, W);
		MeteorMid = D;
		MeteorComp = Part(W, N->Comp.Get(), W->GeoMesh(N->Geo), D, -2);
		if (MeteorComp.IsValid()) MeteorComp->SetVisibility(false);
		// FX-01: the passage's own small pool of the same strip (-S47FxOff: none, the clock logic stays)
		PassComps.Reset(); PassMids.Reset();
		if (!S47Fx::Off())
			for (int32 K = 0; K < PASSAGE_SLOTS; ++K)
			{
				UMaterialInstanceDynamic* PD = UMaterialInstanceDynamic::Create(MeteorBase, W);
				UStaticMeshComponent* C = Part(W, N->Comp.Get(), W->GeoMesh(N->Geo), PD, -2);
				if (!C) continue;
				C->SetVisibility(false);
				S47Fx::Precache(C);
				PassComps.Add(C); PassMids.Add(PD);
			}
		// the developer menu's shower: its own pool, so several of its streaks are drawn at once
		ShowerComps.Reset(); ShowerMids.Reset();
		for (int32 K = 0; K < SHOWER_SLOTS; ++K)
		{
			UMaterialInstanceDynamic* SD = UMaterialInstanceDynamic::Create(MeteorBase, W);
			UStaticMeshComponent* C = Part(W, N->Comp.Get(), W->GeoMesh(N->Geo), SD, -2);
			if (!C) continue;
			C->SetVisibility(false);
			S47Fx::Precache(C);
			ShowerComps.Add(C); ShowerMids.Add(SD);
		}
	}
	// the painted horizon (backdrop.ts), over all of them
	if (const FS47WebNode* N = W->Node(int32(Num(TEXT("band"), -1))))
	{
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(BandBase, W);
		if (UTexture2D* T = Tex(int32(Num(TEXT("night"), -1)))) D->SetTextureParameterValue(TEXT("Night"), T);
		if (UTexture2D* T = Tex(int32(Num(TEXT("day"), -1)))) D->SetTextureParameterValue(TEXT("Day"), T);
		D->SetScalarParameterValue(TEXT("Texel"), float(Num(TEXT("texel"), 1.0 / 128)));
		BandMid = D;
		BandComp = Part(W, N->Comp.Get(), W->GeoMesh(N->Geo), D, -1);
	}
	bLive = DomeComp.IsValid() && DomeComp->GetStaticMesh() != nullptr;
	if (bLive && W->SkySphereComponent()) W->SkySphereComponent()->SetVisibility(false);
	UE_LOG(LogS47Sky, Display, TEXT("S47: live sky %s: dome %s, %d stars, comet %s, shooting star %s, band %s"), bLive ? TEXT("on") : TEXT("off"),
		DomeComp.IsValid() ? TEXT("yes") : TEXT("no"), StarsComp.IsValid() ? StarsComp->GetInstanceCount() : 0, CometComp.IsValid() ? TEXT("yes") : TEXT("no"),
		MeteorComp.IsValid() ? TEXT("yes") : TEXT("no"), BandComp.IsValid() ? TEXT("yes") : TEXT("no"));
	return bLive;
}

void FS47Sky::VeilOf(double Sand, double Shower, double Day, FLinearColor& OutColor, double& OutOpacity)
{
	const double S = FMath::Clamp(FMath::IsFinite(Sand) ? Sand : 0.0, 0.0, 1.0), R = FMath::Clamp(FMath::IsFinite(Shower) ? Shower : 0.0, 0.0, 1.0);
	const double D = FMath::Clamp(FMath::IsFinite(Day) ? Day : 1.0, 0.0, 1.0);
	// the sand: a tan-brown veil by day, a dark brown one at night that hides the fainter stars; a shower's cloud: grey, by day
	// (M5 sweep 3: .42 of a light grey only greyed the blue; now darker than the photograph's sky, and more of it)
	const double SandA = 0.55 * S * (0.35 + 0.65 * D), RainA = 0.6 * R * D;
	OutOpacity = 1.0 - (1.0 - SandA) * (1.0 - RainA);
	const FLinearColor SandC = FMath::Lerp(FLinearColor(.06f, .04f, .025f), FLinearColor(.66f, .50f, .32f), float(D));
	const FLinearColor RainC = FLinearColor(.30f, .32f, .36f) * float(FMath::Lerp(0.15, 1.0, D));
	OutColor = SandA + RainA > 1e-6 ? (SandC * float(SandA) + RainC * float(RainA)) * float(1.0 / (SandA + RainA)) : SandC;
}

namespace
{
	// a few octaves of the engine's noise, about -1 to 1
	double CellFbm(double X, double Y, int32 Octaves)
	{
		double Sum = 0, Amp = .5, Freq = 1;
		for (int32 O = 0; O < Octaves; ++O) { Sum += Amp * FMath::PerlinNoise2D(FVector2D(X * Freq, Y * Freq)); Amp *= .5; Freq *= 2.03; }
		return Sum * 1.6;
	}
	// a rain shaft under a card's base: where (0 to 1 across), how wide, how much
	struct FCellShaft { double Mid, Width, Strength; };
}

void FS47Sky::CellPixels(int32 Wd, int32 Ht, TArray<FColor>& Out, int32 Variant)
{
	Out.SetNumZeroed(FMath::Max(0, Wd) * FMath::Max(0, Ht));
	if (Wd < 2 || Ht < 2) return;
	const auto Smooth = [](double A, double B, double X) { return Smooth01((X - A) / FMath::Max(1e-6, B - A)); };
	// SHOWER-LOOK-20261109: each card its own cloud. The cell has four shafts, the mass behind two broad pale ones, the piece in front
	// two narrow ones
	static const FCellShaft Shafts0[] = { { .30, .045, .85 }, { .46, .07, 1 }, { .61, .05, .8 }, { .73, .03, .5 } };
	static const FCellShaft Shafts1[] = { { .38, .08, .7 }, { .58, .06, .55 } };
	static const FCellShaft Shafts2[] = { { .44, .05, 1 }, { .60, .035, .7 } };
	const int32 Kind = FMath::Clamp(Variant, 0, 2);
	const TArrayView<const FCellShaft> Shafts = Kind == 0 ? MakeArrayView(Shafts0) : Kind == 1 ? MakeArrayView(Shafts1) : MakeArrayView(Shafts2);
	const double Seed = 17.31 * Kind;
	for (int32 X = 0; X < Wd; ++X)
	{
		const double U = double(X) / (Wd - 1), Edge = 1 - FMath::Abs(2 * U - 1);
		// the top: towers higher in the middle, lumpy; the base soft and ragged, sagging under each shaft where the rain falls out of it
		const double Top = FMath::Max(.05, .27 - .2 * Smooth(0, .8, Edge) + .09 * FMath::PerlinNoise1D(float(U * 4.3 + Seed)) + .04 * FMath::PerlinNoise1D(float(U * 11.3 + Seed)));
		double Sag = 0;
		for (const FCellShaft& Sh : Shafts) Sag += .075 * Sh.Strength * FMath::Exp(-FMath::Square((U - Sh.Mid) / (Sh.Width * 1.6)));
		const double Base = .56 + .045 * FMath::PerlinNoise1D(float(U * 3.7 + Seed + 3.1)) + .025 * FMath::PerlinNoise1D(float(U * 9.1 + Seed)) + .01 * FMath::PerlinNoise1D(float(U * 23.0 + Seed)) + Sag;
		for (int32 Y = 0; Y < Ht; ++Y)
		{
			const double V = double(Y) / (Ht - 1);
			const double Mottle = .82 + .18 * (.5 + .5 * CellFbm(U * 8.0 + Seed, V * 5.0, 4));
			// cauliflower at the top, tatters at the base: the edges are torn by the noise in two directions, a band 8 % deep, not a line
			const double TopV = V + .035 * CellFbm(U * 26.0 + Seed, V * 18.0, 3);
			const double BaseV = V + .055 * CellFbm(U * 21.0 + Seed + 5.7, V * 26.0, 3);
			const double Cloud = Smooth(Top, Top + .10, TopV) * (1 - Smooth(Base - .055, Base + .035, BaseV)) * Smooth(0, .35, Edge) * (.9 + .1 * Mottle); // (solid: the sky photograph behind must not show through it)
			// the virga: fibres hanging from the base in its shafts, curling back with the wind the further they fall (the wind is less
			// under the cloud), each drying at its own height well before the ground
			const double Below = V - Base;
			double Virga = 0;
			if (Below > -.03)
			{
				for (int32 K = 0; K < Shafts.Num(); ++K)
				{
					const FCellShaft& Sh = Shafts[K];
					const double Len = FMath::Min(.93 - Base, .2 + .16 * (.5 + .5 * FMath::PerlinNoise1D(float(K * 3.7 + Seed + 1.3))));
					if (Len <= 0) continue;
					const double Fall = FMath::Max(0.0, Below);
					const double Us = U - .05 * Fall - .5 * Fall * Fall;
					const double Shape = FMath::Exp(-FMath::Square((Us - Sh.Mid) / (Sh.Width * (1 + 1.2 * Fall)))); // (spreading as it falls)
					const double Fibre = FMath::Square(FMath::Clamp(.5 + .8 * FMath::PerlinNoise2D(FVector2D(Us * 70.0 + Seed, Fall * 2.5 + K)), 0.0, 1.0));
					const double Fade = FMath::Pow(FMath::Clamp(1 - Fall / Len, 0.0, 1.0), 1.25) * Smooth(-.03, .02, Below);
					Virga += .7 * Sh.Strength * Shape * (.45 + .55 * Fibre) * Fade;
				}
				Virga = FMath::Clamp(Virga, 0.0, 1.0) * Smooth(.1, .45, Edge);
			}
			// the card itself never shows: everything fades out over the outer few percent of the picture (self test: no card's edge shows)
			const double Ux = (X + .5) / Wd, Vy = (Y + .5) / Ht;
			const double Border = Smooth(0, .07, FMath::Min(Ux, 1 - Ux)) * Smooth(0, .07, FMath::Min(Vy, 1 - Vy));
			const double A = FMath::Clamp(1 - (1 - .98 * Cloud) * (1 - Virga), 0.0, 1.0) * Border;
			// sunlit at the top, dark and heavy at the base; the rain a lighter grey than the base it falls from
			const double CloudLum = FMath::Lerp(.95, .44, Smooth(.12, Base, V)) * (.9 + .1 * Mottle); // (not from Top: its noise would stripe the cloud)
			const double VirgaLum = FMath::Lerp(.58, .74, Smooth(0, .25, Below));
			const double Lum = A > 1e-4 ? (CloudLum * .98 * Cloud + VirgaLum * Virga) / FMath::Max(1e-4, .98 * Cloud + Virga) : 0.0;
			const uint8 G = uint8(FMath::Clamp(FMath::RoundToInt(255 * Lum), 0, 255));
			Out[Y * Wd + X] = FColor(G, G, uint8(FMath::Min(255, G + 8)), uint8(FMath::Clamp(FMath::RoundToInt(255 * A), 0, 255)));
		}
	}
}

void FS47Sky::CellPlace(double DirX, double DirZ, double ShowerH, FVector2D& OutOffset, double& OutShow)
{
	FVector2D Dir(FMath::IsFinite(DirX) ? DirX : 0, FMath::IsFinite(DirZ) ? DirZ : 0);
	Dir = Dir.SizeSquared() > 1e-6 ? Dir.GetSafeNormal() : FVector2D(.6, -.8);
	OutShow = 1;
	if (!FMath::IsFinite(ShowerH) || ShowerH <= S47Weather::NoShower + 1) { OutOffset = -Dir * CellDistance; return; }
	// it rains on the land from a quarter of an hour before ShowerH 0 to about 1.1 (S47Weather::ShowerCourse): nearest at .4, to the side
	const double Upwind = CellSpeed * (.4 - ShowerH);
	OutOffset = -Dir * Upwind + FVector2D(-Dir.Y, Dir.X) * CellAside;
	const double Far = OutOffset.Size();
	OutShow = 1 - Smooth01((Far - (CellFar - 3000)) / 5000);
	if (Far > CellFar) OutOffset *= CellFar / Far;
}

void FS47Sky::DrawCell(AS47Web* W, const FVector& Eye, double Rain, double Day)
{
	const S47Weather::FLive& Wx = S47Weather::Live();
	FVector2D Off; double Show = 0;
	CellPlace(Wx.DirX, Wx.DirZ, Wx.ShowerH, Off, Show);
	const double K = Wx.bOn ? Clamp01(Rain) * Smooth01((Day - .1) / .5) * Show : 0.0;
	if (K < .02)
	{
		for (const TWeakObjectPtr<UStaticMeshComponent>& C : CellComp) if (C.IsValid() && C->IsVisible()) C->SetVisibility(false);
		return;
	}
	if (!CellComp[0].IsValid())
	{
		if (bCellTried || !W) return;
		bCellTried = true;
		UStaticMesh* Card = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Unlit_Trans_2S.M_Web_Unlit_Trans_2S"));
		if (!Card || !Base) return;
		// drawn far to near: the mass behind, the cell, the piece in front, all over the veil, which would wash them to its grey
		static const int32 Sort[CellCards] = { 2, 1, 3 };
		for (int32 I = 0; I < CellCards; ++I)
		{
			TArray<FColor> Px; CellPixels(384, 192, Px, I);
			UTexture2D* Tex = S47Image::FromPixels(Px, 384, 192);
			if (!Tex) return;
			Tex->AddressX = TA_Clamp; Tex->AddressY = TA_Clamp; Tex->UpdateResource();
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, *FString::Printf(TEXT("S47ShowerCell%d"), I));
			C->SetupAttachment(W->GetRootComponent());
			C->SetMobility(EComponentMobility::Movable);
			C->SetStaticMesh(Card);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(false);
			C->bAffectDistanceFieldLighting = false;
			C->bVisibleInReflectionCaptures = false;
			C->bVisibleInRealTimeSkyCaptures = false;
			C->SetVisibleInRayTracing(false);
			C->SetTranslucentSortPriority(Sort[I]);
			CellMid[I].Reset(UMaterialInstanceDynamic::Create(Base, W));
			CellMid[I]->SetTextureParameterValue(TEXT("Map"), Tex);
			CellMid[I]->SetScalarParameterValue(TEXT("UseMap"), 1.f);
			// the engine's plane has the picture's top row at its -Y edge (as S47Figures' cards have it), stood up below
			CellMid[I]->SetVectorParameterValue(TEXT("MapU"), FLinearColor(1, 0, 0, 0));
			CellMid[I]->SetVectorParameterValue(TEXT("MapV"), FLinearColor(0, 1, 0, 0));
			C->SetMaterial(0, CellMid[I].Get());
			C->RegisterComponent();
			CellComp[I] = C;
		}
		UE_LOG(LogS47Sky, Display, TEXT("S47: sky: the shower's cloud built (%d cards)"), CellCards);
	}
	// laid out from the cell: Ahead along the eye's line to it (metres), Aside across it (shares of the cell's width), the card's size
	// against the cell's, how light and how solid. Nearer, the cell is drawn a little narrower, so that it never fills the whole view.
	const double Dist = FMath::Max(1000.0, Off.Size());
	const FVector2D Fwd = Off / Dist, Right(-Fwd.Y, Fwd.X);
	const double Width = FMath::Clamp(2.4 * Dist, 9000.0, CellWidth);
	struct FLayer { double Ahead, Aside, Size, Tone, Solid; };
	const FLayer Layers[CellCards] = { { 0, 0, 1, 1, 1 }, { 3500, .3, .85, 1.18, .8 }, { -FMath::Min(1800.0, .35 * Dist), -.32, .5, .82, 1 } };
	const float Lit = float(FMath::Lerp(0.25, 1.0, Clamp01(Day)));
	for (int32 I = 0; I < CellCards; ++I)
	{
		UStaticMeshComponent* C = CellComp[I].Get();
		UMaterialInstanceDynamic* D = CellMid[I].Get();
		if (!C || !D) continue;
		const FLayer& L = Layers[I];
		D->SetVectorParameterValue(TEXT("Color"), FLinearColor(.26f, .27f, .30f) * Lit * float(L.Tone));
		D->SetScalarParameterValue(TEXT("Opacity"), float(FMath::Min(1.0, 1.1 * K) * L.Solid));
		const FVector2D At2 = Off + Fwd * L.Ahead + Right * (L.Aside * Width);
		const double CardW = Width * L.Size, CardH = CellHeight * (.6 + .4 * L.Size);
		// the foot of every card at the eye's own height, so a smaller card is a lower cloud
		const FVector At = Eye + FVector(At2.X, CardH / 2 - 2, At2.Y);
		// turned to the eye, the picture's top (the plane's -Y) up
		const FVector2D ToEye = (-At2).GetSafeNormal();
		const FVector Facing = S47Three::DirToUE(FVector(ToEye.X, 0, ToEye.Y));
		C->SetWorldLocationAndRotation(S47Three::ToUE(At), FRotationMatrix::MakeFromZY(Facing, FVector(0, 0, -1)).Rotator());
		C->SetWorldScale3D(FVector(CardW, CardH, 1)); // the plane is a metre square in centimetres
		if (!C->IsVisible()) C->SetVisibility(true);
	}
}

void FS47Sky::DrawVeil(AS47Web* W, const FVector& Eye)
{
	const S47Weather::FLive& Wx = S47Weather::Live();
	FLinearColor Col; double A = 0;
	const double Rain = Wx.bOn ? Clamp01(FMath::Max(Wx.Rain, (Wx.CloudBias - .06) / .18)) : 0.0;
	DrawCell(W, Eye, Rain, Daylight());
	if (Wx.bOn) VeilOf(Wx.Dust * 1.2, Rain, Daylight(), Col, A);
	if (A < 0.01)
	{
		if (VeilComp.IsValid() && VeilComp->IsVisible()) VeilComp->SetVisibility(false);
		return;
	}
	if (!VeilComp.IsValid())
	{
		if (bVeilTried || !W) return;
		bVeilTried = true;
		UStaticMesh* Ball = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Unlit_Trans_2S.M_Web_Unlit_Trans_2S"));
		if (!Ball || !Base) return;
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, TEXT("S47WeatherVeil"));
		C->SetupAttachment(W->GetRootComponent());
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Ball);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->bAffectDistanceFieldLighting = false;
		C->bVisibleInReflectionCaptures = false;
		C->bVisibleInRealTimeSkyCaptures = false;
		C->SetVisibleInRayTracing(false);
		C->SetWorldScale3D(FVector(36000.f)); // 18 km round the eye (the dome is 30 km out): the far land beyond it goes into the sand too
		VeilMid.Reset(UMaterialInstanceDynamic::Create(Base, W));
		C->SetMaterial(0, VeilMid.Get());
		C->RegisterComponent();
		VeilComp = C;
		UE_LOG(LogTemp, Display, TEXT("S47: sky: the weather's veil built"));
	}
	if (UMaterialInstanceDynamic* D = VeilMid.Get())
	{
		D->SetVectorParameterValue(TEXT("Color"), Col);
		D->SetScalarParameterValue(TEXT("Opacity"), float(A));
	}
	VeilComp->SetWorldLocation(S47Three::ToUE(Eye));
	if (!VeilComp->IsVisible()) VeilComp->SetVisibility(true);
}

void FS47Sky::Draw(const FVector& Eye, double Yaw)
{
	AS47Web* W = Web.Get();
	if (!bLive || !W) return;
	// The daytime photograph contains grey overcast clouds. Hand its radiance to
	// the existing amber western horizon / cool zenith recipe as the sun lowers.
	// This is a sky-only blend, never an orange postprocess on the player or night.
	const float GoldenSky = float(Clamp01(Evening * 4) * (1 - Smooth01((SunDir.Y - .035) / .145)));
	const float DayWeight = PureSkyMid.IsValid() ? float(PureSkyWeight) * (1.f - .88f * GoldenSky) : 0.f;
	UMaterialInstanceDynamic* ActiveDome = DayWeight > 0.f ? PureSkyMid.Get() : DomeMid.Get();
	if (DomeComp.IsValid() && DomeComp->GetMaterial(0) != ActiveDome) DomeComp->SetMaterial(0, ActiveDome);
	// M5 critic 3: blowing sand (S47Weather::Live, Freeform only) browns the low sky toward the sun, as the dusk's dusty grade does, and
	// dims the sun's disc; a shower's cloud dims it too
	const S47Weather::FLive& Wx = S47Weather::Live();
	const double SandSky = Wx.bOn ? FMath::Clamp(Wx.Dust * 1.2, 0.0, 1.0) * Midday : 0.0;
	const double SunVeil = Wx.bOn ? (1.0 - 0.55 * FMath::Clamp(Wx.Dust * 1.2, 0.0, 1.0)) * (1.0 - 0.4 * FMath::Clamp(Wx.Rain + FMath::Max(0.0, Wx.CloudBias - .06) / .18, 0.0, 1.0)) : 1.0;
	if (PureSkyMid.IsValid())
	{
		PureSkyMid->SetScalarParameterValue(TEXT("PureSkyWeight"), DayWeight);
		PureSkyMid->SetScalarParameterValue(TEXT("SunsetStrength"), FMath::Max(GoldenSky, float(0.6 * SandSky)));
	}
	// sky.group.rotation.y = world.skyYaw; group.position.copy(follow); ten times as far (SKY_SCALE)
	W->SetLocal(GroupNode, Eye, S47Three::Euler(0, Yaw, 0), FVector(SKY_SCALE));
	DrawVeil(W, Eye);
	const FLinearColor Holes = V4(HoleDir), HoleRad(float(HoleR.X), float(HoleR.Y), 0.f, 0.f);
	auto Common = [&](UMaterialInstanceDynamic* D)
	{
		if (!D) return;
		D->SetScalarParameterValue(TEXT("Dawn"), float(Daylight()));
		D->SetVectorParameterValue(TEXT("HoleDir"), Holes);
		D->SetVectorParameterValue(TEXT("HoleR"), HoleRad);
		D->SetScalarParameterValue(TEXT("Hole"), float(HoleK));
		D->SetScalarParameterValue(TEXT("ConceptBlend"), FMath::Max(float(S47CalicheVisualWeight(W->HighwayMapEye(Eye))), GoldenSky));
	};
	auto Sunny = [&](UMaterialInstanceDynamic* D)
	{
		if (!D) return;
		D->SetScalarParameterValue(TEXT("Yaw"), float(Yaw));
		D->SetScalarParameterValue(TEXT("Dawn"), float(Daylight()));
		D->SetScalarParameterValue(TEXT("Sun"), float(Sun * SunVeil));
		D->SetScalarParameterValue(TEXT("ConceptBlend"), FMath::Max(float(S47CalicheVisualWeight(W->HighwayMapEye(Eye))), GoldenSky));
		D->SetVectorParameterValue(TEXT("SunDir"), V4(SunDir));
		for (int32 k = 0; k < 3; ++k) D->SetVectorParameterValue(*FString::Printf(TEXT("Glow%d"), k), FLinearColor(float(Glow[k].X), float(Glow[k].Y), float(Glow[k].Z), float(Glow[k].W)));
	};
	for (UMaterialInstanceDynamic* D : { DomeMid.Get(), PureSkyMid.Get() }) if (D)
	{
		Sunny(D); Common(D);
		D->SetScalarParameterValue(TEXT("Flash"), float(Flash));
		D->SetScalarParameterValue(TEXT("ConceptBlend"), FMath::Max(float(S47CalicheVisualWeight(W->HighwayMapEye(Eye))), GoldenSky));
		D->SetScalarParameterValue(TEXT("Midday"), float(Midday));
		D->SetVectorParameterValue(TEXT("FlashDir"), V4(FlashDir));
	}
	Common(StarsMid.Get());
	if (UMaterialInstanceDynamic* D = StarsMid.Get())
	{
		D->SetScalarParameterValue(TEXT("Dawn"), float(FMath::Max3(StarDimming(), FMath::Clamp(FinaleStarDim, 0., 1.), MoonStarDim)));
		D->SetScalarParameterValue(TEXT("PR"), float(FMath::Clamp(FinaleStarScale, .2, 1.)));
	}
	// Keep native capability separate from this private material's fallback-weather blend.
	// Low has no native layer: raising Vol toward 1 fades its painted cloud alternative.
	const float WeatherWeight = float(FMath::Clamp(FinaleWeatherWeight, 0., 1.));
	const float NativeVol = W->CloudsActive() ? 1.f : 0.f;
	const float Vol = FMath::Max(NativeVol, 1.f - WeatherWeight);
	for (UMaterialInstanceDynamic* D : { DomeMid.Get(), PureSkyMid.Get() }) if (D)
	{
		D->SetScalarParameterValue(TEXT("VolClouds"), Vol);
		// The painted cirrus above the layer, drifting slowly with the same west-southwest wind.
		// Gate by actual capability too: Low's synthetic Vol blend must not wake new cirrus mid-fade.
		D->SetScalarParameterValue(TEXT("Cirrus"), float(Cirrus) * WeatherWeight * NativeVol);
		D->SetVectorParameterValue(TEXT("CirrusDrift"), FLinearColor(float(FMath::Fmod(Time * 0.0011, 1000.0)), float(FMath::Fmod(Time * 0.0004, 1000.0)), 0.f, 0.f));
	}
	if (UMaterialInstanceDynamic* D = StarsMid.Get()) D->SetScalarParameterValue(TEXT("VolClouds"), Vol);
	{
		// Sky terms -> three's world: the group turns by Yaw about +Y (three's rotation.y).
		auto Turn = [Yaw](const FVector& V) { const double C = FMath::Cos(Yaw), S = FMath::Sin(Yaw); return FVector(V.X * C + V.Z * S, V.Y, -V.X * S + V.Z * C); };
		// Story beats keep open sky: the star hole where the dishes point from 02:15, and Halley's comet.
		const FVector Hole = Turn(HoleDir.GetSafeNormal());
		FVector Comet = FVector(0, 0, 1); double CometK = 0;
		if (CometComp.IsValid() && CometComp->IsVisible())
		{
			const FVector D = (CometComp->Bounds.Origin - S47Three::ToUE(Eye)).GetSafeNormal();
			Comet = FVector(D.X, D.Z, D.Y); CometK = 1 - Daylight();
		}
		// By day the same slot keeps a low sun open where the player stands (late afternoon commute,
		// sunrise): its light still reaches the road under the layer; shadows still drift elsewhere.
		if (CometK < 0.05 && Sun > 0 && SunDir.Y > -0.02 && SunDir.Y < 0.4)
		{
			Comet = SunDir.GetSafeNormal(); CometK = 0.85 * (1 - FMath::Clamp((SunDir.Y - 0.25) / 0.15, 0.0, 1.0));
		}
		W->SetClouds(float(CloudCover), float(Cirrus), FVector4(Hole.X, Hole.Y, Hole.Z, HoleK), FVector4(Comet.X, Comet.Y, Comet.Z, CometK),
			float(Flash), Turn(FlashDir), Time, DayWeight, WeatherWeight);
	}
	if (CometComp.IsValid() && CometComp->IsVisible() != bComet) CometComp->SetVisibility(bComet);
	// The moon: in the sky's own frame (the group turns with the area's north), turned to the eye, its lit side to the sun.
	if (MoonComp.IsValid())
	{
		const bool bShow = bMoon && MoonEl > -1 && MoonLit > .01;
		if (MoonComp->IsVisible() != bShow) MoonComp->SetVisibility(bShow);
		if (bShow)
		{
			const bool bTex = bMoonTex && MoonTexMid.IsValid() && !DevMoon.bOld;
			UMaterialInterface* Want = bTex ? static_cast<UMaterialInterface*>(MoonTexMid.Get()) : static_cast<UMaterialInterface*>(MoonMid.Get());
			if (Want && MoonComp->GetMaterial(0) != Want) MoonComp->SetMaterial(0, Want);
			const FVector Dir = S47SkyDir(MoonAz, MoonEl);
			const FVector SunAt = S47SkyDir(MoonSunAz, MoonSunEl);
			FVector Lit = SunAt - Dir * FVector::DotProduct(SunAt, Dir);
			if (Lit.SizeSquared() < 1e-6) Lit = FVector(0, 1, 0) - Dir * Dir.Y;
			const FVector Facing = S47Three::ToUE(-Dir).GetSafeNormal(), Toward = S47Three::ToUE(Lit).GetSafeNormal();
			MoonComp->SetRelativeLocation(S47Three::ToUE(Dir * StarRadius * .97));
			MoonComp->SetRelativeScale3D(FVector(StarRadius * .036)); // about two degrees across: larger than life, as a painter would
			const double Day = Clamp01(Daylight());
			if (!bTex)
			{
				if (FMath::Abs(MoonLit - MoonPainted) > .01) PaintMoon();
				MoonComp->SetRelativeRotation(FRotationMatrix::MakeFromZX(Facing, Toward).Rotator());
				if (MoonMid.IsValid()) MoonMid->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, .97f, .9f) * float(FMath::Lerp(4.0, .5, Day)));
			}
			else
			{
				// PT-MOON-20261109: the moon's north toward the celestial pole (the valley at 33.3 N), so its seas stand as they are seen and do
				// not turn with the sun; the sun's side comes in the material from SunL instead.
				FVector North = S47SkyDir(0, 33.3);
				North -= Dir * FVector::DotProduct(North, Dir);
				const FVector Up = S47Three::ToUE(North).GetSafeNormal();
				MoonComp->SetRelativeRotation(FRotationMatrix::MakeFromZX(Facing, Up).Rotator());
				// Which of the plane's sides the eye has on its right (Unreal: right = up x forward), read in the world, whatever the frames do.
				const FTransform& X = MoonComp->GetComponentTransform();
				const FVector FwdW = (MoonComp->GetComponentLocation() - S47Three::ToUE(Eye)).GetSafeNormal();
				const FVector UpW = X.TransformVector(FVector::XAxisVector).GetSafeNormal(), YW = X.TransformVector(FVector::YAxisVector).GetSafeNormal();
				const double Flip = FVector::DotProduct(FVector::CrossProduct(UpW, FwdW), YW) >= 0 ? 1.0 : -1.0;
				// The sun in the disc's frame (right, up, toward the eye): its side from Toward, how far round from the phase (cos = 2 lit - 1).
				const FVector Right = FVector::CrossProduct(Facing, Up).GetSafeNormal() * Flip;
				const double CosP = FMath::Clamp(2 * MoonLit - 1, -1.0, 1.0), SinP = FMath::Sqrt(FMath::Max(0.0, 1 - CosP * CosP));
				const FVector SunL(SinP * FVector::DotProduct(Toward, Right), SinP * FVector::DotProduct(Toward, Up), CosP);
				const double Night = 1 - Day;
				const double Gain = FMath::Lerp(double(CvMoonGain.GetValueOnGameThread()), double(CvMoonDayGain.GetValueOnGameThread()), Day);
				UMaterialInstanceDynamic* T = MoonTexMid.Get();
				T->SetScalarParameterValue(TEXT("Flip"), float(Flip));
				T->SetVectorParameterValue(TEXT("SunL"), FLinearColor(float(SunL.X), float(SunL.Y), float(SunL.Z), 0.f));
				T->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, .97f, .91f) * float(Gain));
				// earthshine: most on a thin crescent, none to speak of past the quarter, and lost in daylight
				T->SetScalarParameterValue(TEXT("Earth"), float(CvMoonEarth.GetValueOnGameThread() * FMath::Pow(1 - MoonLit, 2.0) * Night));
				// the disc hides the stars behind it at night, dark side too; by day the sky's blue lies over it
				// NIGHT-20261110 MOON-DARK: only partly at night (was .92), so a crescent's dark side is no blacker than the sky around it
				T->SetScalarParameterValue(TEXT("OccDark"), float(FMath::Lerp(.4, 0.0, Day)));
				T->SetScalarParameterValue(TEXT("OccLit"), float(FMath::Lerp(1.0, .35, Day)));
			}
		}
	}
	if (MeteorComp.IsValid() && MeteorComp->IsVisible() != bMetVisible) MeteorComp->SetVisibility(bMetVisible);
	if (UMaterialInstanceDynamic* D = MeteorMid.Get(); D && bMetVisible)
	{
		Common(D);
		D->SetScalarParameterValue(TEXT("Yaw"), float(Yaw));
		D->SetScalarParameterValue(TEXT("Scale"), float(SKY_SCALE * 100));
		D->SetScalarParameterValue(TEXT("Back"), float(Back));
		D->SetScalarParameterValue(TEXT("Head"), float(Head));
		D->SetVectorParameterValue(TEXT("Start"), V4(Met.Start));
		D->SetVectorParameterValue(TEXT("Way"), V4(Met.Tan));
		D->SetScalarParameterValue(TEXT("I"), float(MetI));
	}
	// One streak on one strip of a pool: the same parameters, shader and shared hole mask as the rare one.
	auto Strip = [&](UMaterialInstanceDynamic* D, const FPassMet& P, double Hd, double Bk, double Intensity)
	{
		Common(D);
		D->SetScalarParameterValue(TEXT("Yaw"), float(Yaw));
		D->SetScalarParameterValue(TEXT("Scale"), float(SKY_SCALE * 100));
		D->SetScalarParameterValue(TEXT("Back"), float(Bk));
		D->SetScalarParameterValue(TEXT("Head"), float(Hd));
		D->SetVectorParameterValue(TEXT("Start"), V4(P.Start));
		D->SetVectorParameterValue(TEXT("Way"), V4(P.Tan));
		D->SetScalarParameterValue(TEXT("I"), float(Intensity));
	};
	{
		// FX-01: the passage's streaks, each on the strip the schedule gave it (never two at once on one)
		bool Vis[PASSAGE_SLOTS] = {};
		const TArray<FPassMet>& Sch = PassageSchedule();
		if (PassageActive() && !bPassageHiddenForTest)
			for (const FPassMet& P : Sch)
			{
				double Hd, Bk, Fd;
				UMaterialInstanceDynamic* D = PassMids.IsValidIndex(P.Slot) ? PassMids[P.Slot].Get() : nullptr;
				if (!D || Vis[P.Slot] || (bPassageLow && !P.bLow) || !PassageLayout(P, PassageT - P.At, Hd, Bk, Fd)) continue;
				Strip(D, P, Hd, Bk, PASSAGE_GAIN * P.Bright * Fd * (1 - Daylight()));
				Vis[P.Slot] = true;
			}
		for (int32 K = 0; K < PassComps.Num() && K < PASSAGE_SLOTS; ++K)
			if (UStaticMeshComponent* C = PassComps[K].Get(); C && C->IsVisible() != Vis[K]) C->SetVisibility(Vis[K]);
	}
	for (int32 K = 0; K < ShowerComps.Num() && K < SHOWER_SLOTS; ++K)
	{
		// the developer shower: each live streak on its own strip, frozen while it is held
		double Hd, Bk, Fd;
		UMaterialInstanceDynamic* D = ShowerMids.IsValidIndex(K) ? ShowerMids[K].Get() : nullptr;
		const bool bVis = D && Shower[K].T >= 0 && PassageLayout(Shower[K].P, Shower[K].T, Hd, Bk, Fd);
		if (bVis) Strip(D, Shower[K].P, Hd, Bk, Shower[K].P.Bright * Fd * (1 - Daylight()));
		if (UStaticMeshComponent* C = ShowerComps[K].Get(); C && C->IsVisible() != bVis) C->SetVisibility(bVis);
	}
	const FS47WebNode* BackdropNode = W->Node(ConceptBackdropNode);
	const UStaticMeshComponent* Backdrop = BackdropNode ? Cast<UStaticMeshComponent>(BackdropNode->Comp.Get()) : nullptr;
	const bool bBackdropReady = Backdrop && Backdrop->GetStaticMesh() && Backdrop->IsVisible() && !Backdrop->bHiddenInGame;
	const bool bReplaceLegacy = bBackdropReady && (W->HighwayLandscapeReady() || S47CalicheVisualWeight(W->HighwayMapEye(Eye)) > .999);
	if (bReplaceLegacy != bConceptLegacyMesasHidden)
	{
		for (const TPair<int32, bool>& Entry : ConceptLegacyMesas) W->SetVisible(Entry.Key, Entry.Value && !bReplaceLegacy);
		bConceptLegacyMesasHidden = bReplaceLegacy;
		UE_LOG(LogS47Sky, Display, TEXT("S47: Caliche legacy night mesas %s: %d original visibility flags preserved"),
			bReplaceLegacy ? TEXT("replaced") : TEXT("restored"), ConceptLegacyMesas.Num());
	}
	if (UMaterialInstanceDynamic* D = BandMid.Get())
	{
		Sunny(D);
		// A partial import must keep the painted horizon, even with the new sky material present.
		D->SetScalarParameterValue(TEXT("BackdropReady"), bBackdropReady ? 1.f : 0.f);
	}
	// The painted horizon's flat dark mesas (backdrop.ts) only fade near town, by ConceptBlend. With Freeform's real range up
	// they would stand black behind it everywhere else.
	const bool bBandNow = BandVisible && !S47FreeformTerrain::RangeMid(W);
	if (BandComp.IsValid() && BandComp->IsVisible() != bBandNow) BandComp->SetVisibility(bBandNow);
}

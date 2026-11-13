#include "S47Soundscape.h"

#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "S47Ambience.h"
#include "S47Audio.h"
#include "S47EventPlaces.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47Geo.h"
#include "S47Herd.h"
#include "S47Main.h"
#include "S47Omen.h"
#include "S47OpenLand.h"
#include "S47Three.h"
#include "S47TownLayer.h"
#include "S47Walker.h"
#include "S47WebAudio.h"
#include "S47Web.h"
#include "S47World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Soundscape, Log, All);

namespace S47SoundscapeLocal
{
	// How far each carries on a still desert night, road-local metres.
	// CATTLE-FARM-ONLY-20261113: the far lowing carried 1100 m from three spots (two of them with no cattle), through a panner whose
	// inverse fall (ref 80, rolloff .5) still left an eighth of it at 1100 m: heard over nearly the whole land. Now the farm's pen only,
	// faded to nothing by CattleFar on top of the panner's fall; at SARO a whisper (CattleAtSaro of the gain) while the herd is uneasy.
	constexpr double MillReach = 240, WireReach = 70, CattleNear = 180, CattleFar = 420, CattleRef = 40, OwlReach = 650, CoyoteNear = 220, CoyoteFar = 1500;
	constexpr double CattleAtSaro = .25, CattleSaroReach = 160, CattleUneasy = .05;
	// The onset is as dangerous as FS47Omen's; the hush holds this long after the last of it, and our own voices wait after.
	constexpr double HushSeverity = 0.35, HushHoldSeconds = 2.0, AfterHushSeconds = 25, BreathSeconds = 28;
	// The power line along the highway and its spurs (S47HighwayPower), road-local.
	struct FWireSpan { double X0, Z0, X1, Z1; };
	const FWireSpan WireSpans[] = { { 15, -280, 15, 3400 }, { 15, 190, 435, 195 }, { 15, 380, -13, 380 }, { 15, 622, -65, 622 } };
	// The line to Roswell runs north and south well east of the land.
	constexpr double RailX = 3600, RailSpread = 2400;
	double Wrap(double Hour) { return FMath::Fmod(FMath::Fmod(Hour, 24.0) + 24.0, 24.0); }
	bool Between(double Hour, double From, double To) { return From <= To ? Hour >= From && Hour < To : Hour >= From || Hour < To; }
}

// ---------- the rules ----------

double FS47Soundscape::Activity(const FString& Kind, double Hour)
{
	using S47SoundscapeLocal::Between;
	const double Hr = S47SoundscapeLocal::Wrap(Hour);
	if (Kind == TEXT("coyote"))
	{
		if (Between(Hr, 7, 18)) return 0;
		if (Between(Hr, 18, 20)) return 0.6;
		if (Between(Hr, 20, 24)) return 1.0;
		if (Between(Hr, 0, 3)) return 0.8;
		if (Between(Hr, 3, 5.5)) return 1.5;   // the chorus before dawn
		return 0.7;
	}
	if (Kind == TEXT("owl")) return Between(Hr, 6.5, 18.5) ? 0 : Between(Hr, 18.5, 23) ? 0.8 : 1.2;
	if (Kind == TEXT("cattle")) return Between(Hr, 19, 23) ? 1.4 : Between(Hr, 23, 4) ? 0.6 : Between(Hr, 4, 6.5) ? 1.0 : 0.8;
	if (Kind == TEXT("windmill")) return Between(Hr, 9, 18) ? 1.2 : Between(Hr, 18, 22) ? 1.0 : 0.75;
	if (Kind == TEXT("wires")) return Between(Hr, 9, 18) ? 1.0 : 0.8;
	if (Kind == TEXT("train")) return 1.0;
	return 0;
}

double FS47Soundscape::HushReach(double SourceReach)
{
	return FMath::Max(FS47Omen::MinSense, SourceReach * FS47Omen::SenseWiden);
}

bool FS47Soundscape::NearestWire(const FVector2D& RoadAt, double Within, FVector2D& OutRoad, double& OutDist)
{
	double Best = TNumericLimits<double>::Max();
	for (const S47SoundscapeLocal::FWireSpan& Span : S47SoundscapeLocal::WireSpans)
	{
		const FVector2D From(Span.X0, Span.Z0), To(Span.X1, Span.Z1), Along = To - From;
		const double Len2 = Along.SizeSquared();
		const double U = Len2 > 0 ? FMath::Clamp(FVector2D::DotProduct(RoadAt - From, Along) / Len2, 0.0, 1.0) : 0.0;
		const FVector2D On = From + Along * U;
		const double Dist = FVector2D::Distance(RoadAt, On);
		if (Dist < Best) { Best = Dist; OutRoad = On; }
	}
	OutDist = Best;
	return Best <= Within;
}

FVector2D FS47Soundscape::TrainFrom(const FVector2D& RoadAt, float Roll)
{
	return FVector2D(S47SoundscapeLocal::RailX, RoadAt.Y + (FMath::Clamp(double(Roll), 0.0, 1.0) - 0.5) * S47SoundscapeLocal::RailSpread);
}

const TArray<FVector2D>& FS47Soundscape::Mills()
{
	// The farm's (S47FarmWindmill MillAt) and the one on the Baca lease (S47EventPlaces "windmill").
	static const TArray<FVector2D> Spots = { FVector2D(146, 214), FVector2D(170, 720) };
	return Spots;
}

const TArray<FVector2D>& FS47Soundscape::Cattle()
{
	// The farm's pen (the corral, road x 30..80, z 215..265: US47World::ReconcileFarm), where the herd is (FS47Herd). CATTLE-FARM-ONLY-20261113:
	// the salt lick (240, 1740) and the fence past the turnoff (110, 1590) had lowing and no cattle; they are gone.
	static const TArray<FVector2D> Spots = { FVector2D(55, 240) };
	return Spots;
}

double FS47Soundscape::CattleHeard(const FVector2D& RoadAt, bool bHerdUneasy)
{
	using namespace S47SoundscapeLocal;
	double Best = 1e12;
	for (const FVector2D& Spot : Cattle()) Best = FMath::Min(Best, FVector2D::Distance(RoadAt, Spot));
	// (inside CattleNear the near ones are FS47FarmNight's, from the cows themselves)
	if (Best > CattleNear && Best < CattleFar) return FMath::Square(1.0 - (Best - CattleNear) / (CattleFar - CattleNear));
	const FVector2D Saro(S47Geo::SARO_IN_ROAD.X, S47Geo::SARO_IN_ROAD.Z);
	if (bHerdUneasy && FVector2D::Distance(RoadAt, Saro) < CattleSaroReach) return CattleAtSaro;
	return 0;
}

// ---------- the world ----------

bool FS47Soundscape::Road(FVector2D& Out) const
{
	if (!M || !M->Player || !M->Web) return false;
	double Rx = 0, Rz = 0;
	if (!S47OpenLand::RoadOf(M->Web->Area(), M->Player->Pos.X, M->Player->Pos.Z, Rx, Rz)) return false;
	Out = FVector2D(Rx, Rz);
	return true;
}

FVector FS47Soundscape::Here(const FVector2D& RoadAt, double Up) const
{
	const FString Area = M && M->Web ? M->Web->Area() : FString(TEXT("road"));
	FVector Spot = S47OpenLand::FromRoad(Area, RoadAt.X, RoadAt.Y);
	double Floor = S47FarmWorld::FloorRoad(RoadAt.X, RoadAt.Y);
	if (Area == TEXT("station01")) Floor += S47Geo::STATION_DY();
	Spot.Y = Floor + Up;
	return Spot;
}

double FS47Soundscape::HourNow() const
{
	return M && M->Freeform ? FMath::Fmod(M->Freeform->Clock(), 86400.0) / 3600.0 : 0.0;
}

bool FS47Soundscape::Place(const FString& Kind, const FVector& Listener, FVector& Out, bool bForce) const
{
	(void)Listener;
	FVector2D From;
	if (!M || !M->Freeform || !Road(From)) return false;
	const double Hour = HourNow();
	// a dog lives somewhere and is FS47Animals'; it never barks from nowhere
	if (Kind == TEXT("dog")) return false;
	if (Kind == TEXT("train")) { Out = Here(TrainFrom(From, FMath::FRand()), 0); return true; }
	const FS47EventPlaces* Places = M->Freeform->EventPlaces.Get();
	if (!Places) return false;
	if (Kind == TEXT("coyote"))
	{
		if (!bForce && FMath::FRand() > Activity(Kind, Hour)) return false;
		// a wash or the open land a long way off: the washes first, and nearer more than further
		TArray<FVector2D> Spots; TArray<double> Weights; double Sum = 0;
		for (const FS47EventPlace& Pl : Places->All())
		{
			if (!Pl.HasAny(ES47PlaceTrait::Wash | ES47PlaceTrait::Far) || Pl.HasAny(ES47PlaceTrait::Town | ES47PlaceTrait::Indoors)) continue;
			const double Dist = FVector2D::Distance(From, FVector2D(Pl.X, Pl.Z));
			if (Dist < S47SoundscapeLocal::CoyoteNear || Dist > S47SoundscapeLocal::CoyoteFar) continue;
			const double Weight = (Pl.Has(ES47PlaceTrait::Wash) ? 2.0 : 1.0) * (S47SoundscapeLocal::CoyoteFar - Dist) / S47SoundscapeLocal::CoyoteFar;
			Spots.Add(FVector2D(Pl.X, Pl.Z) + FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)) * Pl.Radius);
			Weights.Add(Weight); Sum += Weight;
		}
		if (Spots.IsEmpty() || Sum <= 0) return false;
		double Roll = FMath::FRand() * Sum;
		for (int32 I = 0; I < Spots.Num(); ++I) { Roll -= Weights[I]; if (Roll <= 0 || I == Spots.Num() - 1) { Out = Here(Spots[I], 1); return true; } }
		return false;
	}
	if (Kind == TEXT("owl"))
	{
		if (!bForce && Activity(Kind, Hour) <= 0) return false;
		const FS47EventPlace* Near = Places->NearestTo(From, S47SoundscapeLocal::OwlReach, ES47PlaceTrait::None);
		if (!Near || !Near->HasAny(ES47PlaceTrait::Farm | ES47PlaceTrait::Town | ES47PlaceTrait::Saro | ES47PlaceTrait::People)) return false;
		const double Angle = FMath::FRand() * UE_DOUBLE_PI * 2, Away = FMath::FRandRange(20.f, 60.f);
		Out = Here(FVector2D(Near->X + FMath::Cos(Angle) * Away, Near->Z + FMath::Sin(Angle) * Away), 6);
		return true;
	}
	return false;
}

bool FS47Soundscape::Moo(const FVector& Listener, const FVector& Where, double Gain)
{
	if (!M || !M->Audio || !M->Audio->Night) return false;
	if (!bMoosTried)
	{
		// The same cattle as FS47FarmNight's (BigSoundBank, CC0, FREE_ASSET_CATALOG); without the files there is no far lowing.
		bMoosTried = true;
		for (int32 I = 0; I < 3; ++I)
		{
			TArray<uint8> Bytes;
			if (FFileHelper::LoadFileToArray(Bytes, *(FPaths::ProjectContentDir() / FString::Printf(TEXT("Audio/Raw/sound_design/cow_moo_%d.wav"), I))))
				if (TSharedPtr<FS47AudioBuffer> Rec = S47Wa::DecodeWav(Bytes)) Moos.Add(Rec);
		}
	}
	if (Moos.IsEmpty()) return false;
	return M->Audio->Night->PlayBufferAt(Moos[FMath::RandRange(0, Moos.Num() - 1)], Listener, Where, Gain, FMath::FRandRange(0.9f, 1.06f), S47SoundscapeLocal::CattleRef).IsValid();
}

// ---------- in play ----------

void FS47Soundscape::Hook()
{
	if (bHooked || !M || !M->Audio || !M->Audio->Night) return;
	FS47Ambience& Night = *M->Audio->Night;
	// (the night's own quiet spells stay in: a lull in the crickets is ordinary out here, and a silence is no sure sign, M2 critic)
	TWeakPtr<FS47Soundscape> Weak = AsShared();
	Night.PlaceFor = [Weak](const FString& Kind, const FVector& Listener, FVector& Out)
	{
		const TSharedPtr<FS47Soundscape> Self = Weak.Pin();
		return Self.IsValid() && Self->Place(Kind, Listener, Out, false);
	};
	bHooked = true;
}

void FS47Soundscape::Unhook()
{
	if (M && M->Audio && M->Audio->Night)
	{
		FS47Ambience& Night = *M->Audio->Night;
		if (bHooked) { Night.bQuietSpells = true; Night.PlaceFor = nullptr; }
		if (Night.WiresLevel() > 0) Night.Wires(FVector::ZeroVector, 0);
	}
	bHooked = false;
}

void FS47Soundscape::Reset()
{
	Unhook();
	bHush = false; HushHold = HushAgain = AfterHush = DevHushLeft = DevWiresLeft = 0;
	Due.Reset();
}

void FS47Soundscape::StepHush(double Dt)
{
	FS47Ambience& Night = *M->Audio->Night;
	bool bWant = DevHushLeft > 0, bReal = DevHushLeft > 0;
	FVector From = FVector::ZeroVector;
	DevHushLeft = FMath::Max(0.0, DevHushLeft - Dt);
	const FVector Eye = M->Player ? S47Three::ToThree(M->Player->EyeUE()) : FVector::ZeroVector;
	if (M->Omen && M->Player)
	{
		for (const FS47OmenSource& Src : M->Omen->Sources())
		{
			if (Src.bResidual || Src.Severity < S47SoundscapeLocal::HushSeverity || M->Omen->Misses(Src.Id, ES47OmenChannel::Hush)) continue;
			const double Flat = FVector2D::Distance(FVector2D(Eye.X, Eye.Z), FVector2D(Src.At.X, Src.At.Z));
			if (Flat < HushReach(Src.Reach)) { bWant = true; bReal = true; From = Src.At; break; }
		}
	}
	// A gust: the crickets stop for it all the same, at once, and from the way the wind comes (FS47Omen's ordinary causes).
	double GustFrom = 0;
	const double Gust = M->Omen && !bWant ? M->Omen->Ordinary(ES47Ordinary::Gust, &GustFrom) : 0.0;
	if (Gust > 0)
	{
		bWant = true;
		From = Eye + FVector(FMath::Sin(GustFrom), 0, -FMath::Cos(GustFrom)) * 120.0;
	}
	if (bWant) HushHold = S47SoundscapeLocal::HushHoldSeconds;
	if (bWant && !bHush)
	{
		// At once: not a fade. That is the warning, or the wind.
		bHush = true; ++HushCount; HushAgain = 1.2;
		if (bReal) ++RealHushCount;
		Night.Hush(4, 0.0, 0.07, 4);
		Night.Wires(FVector::ZeroVector, 0);
		// a strong gust is heard coming through; a weak one is not, and then it is a silence like any other
		if (!bReal && Gust > 0.65 && M->Audio) M->Audio->Sfx(TEXT("gust"), FS47Sound{ .Gain = float(0.25 + 0.3 * Gust), .Lp = 1400.f });
		if (M->Omen && M->Space == TEXT("yard")) M->Omen->Felt(ES47OmenChannel::Hush, TEXT("the crickets stopped all at once"), bReal, From, true);
		if (bLog)
		{
			UE_LOG(LogS47Soundscape, Display, TEXT("S47: soundscape hush %d (%s)"), HushCount, bReal ? TEXT("something real") : TEXT("a gust"));
		}
	}
	else if (bHush && bWant)
	{
		if ((HushAgain -= Dt) <= 0) { HushAgain = 1.2; Night.Hush(4, 0.0, 0.2, 4); }
	}
	else if (bHush && (HushHold -= Dt) <= 0)
	{
		// It has gone: the night comes back slowly, a cricket at a time, and the far-off ones wait a while longer.
		bHush = false; AfterHush = S47SoundscapeLocal::AfterHushSeconds;
		Night.Breath(S47SoundscapeLocal::BreathSeconds);
		if (bLog)
		{
			UE_LOG(LogS47Soundscape, Display, TEXT("S47: soundscape back"));
		}
	}
}

void FS47Soundscape::StepVoices(double Dt, double Hour)
{
	using namespace S47SoundscapeLocal;
	FS47Ambience& Night = *M->Audio->Night;
	FVector2D From;
	if (!Road(From)) { Night.Wires(FVector::ZeroVector, 0); return; }
	const FVector Ear = S47Three::ToThree(M->Player->EyeUE());
	const bool bQuiet = bHush || AfterHush > 0;
	// the wires: a held sound under the line, loudest right under it
	{
		FVector2D On; double Dist = 0;
		const bool bNear = NearestWire(From, DevWiresLeft > 0 ? 400.0 : WireReach, On, Dist);
		const double Want = bQuiet || !bNear ? 0.0 : DevWiresLeft > 0 ? 1.0 : FMath::Pow(1.0 - Dist / WireReach, 1.5) * Activity(TEXT("wires"), Hour);
		Night.Wires(Here(On, 8.5), Want);
		if (Want > 0.05) Counts.FindOrAdd(TEXT("wires_frames"))++;
	}
	if (bQuiet) return;
	// indoors and in the cab the far ones are not worth a voice (the walls and the engine take them anyway)
	if (M->Space != TEXT("yard")) return;
	// the windmills
	{
		const FVector2D* Mill = nullptr; double Best = MillReach;
		for (const FVector2D& Spot : Mills()) { const double Dist = FVector2D::Distance(From, Spot); if (Dist < Best) { Best = Dist; Mill = &Spot; } }
		double& Next = Due.FindOrAdd(TEXT("windmill"));
		if (Mill && (Next -= Dt) <= 0)
		{
			Next = FMath::FRandRange(5.f, 13.f) / FMath::Max(0.2, Activity(TEXT("windmill"), Hour));
			// Tom 13 November (d67): the creak carried far too far (600 m, never under a quarter strength). It is the farm's own sound:
			// gone by MillReach, and falling off fast on the way there.
			Night.MillCreak(Ear, Here(*Mill, 9.5), FMath::Clamp(FMath::Square(1.0 - Best / MillReach), 0.04, 1.0));
			Counts.FindOrAdd(TEXT("windmill"))++;
		}
	}
	// cattle lowing some way off the farm (the near ones are FS47FarmNight's); at SARO only a whisper of an uneasy herd (CATTLE-FARM-ONLY-20261113)
	{
		const FS47Herd* Herd = M->TownLayer && !M->TownLayer->Herds.IsEmpty() ? M->TownLayer->Herds[0].Get() : nullptr;
		const bool bUneasy = Herd && Herd->AlarmLevel() > CattleUneasy;
		const double Heard = CattleHeard(From, bUneasy);
		double& Next = Due.FindOrAdd(TEXT("cattle"));
		if (Heard > 0 && (Next -= Dt) <= 0)
		{
			// (an uneasy herd calls more, the hour or not)
			Next = bUneasy ? FMath::FRandRange(12.f, 30.f) : FMath::FRandRange(35.f, 90.f) / FMath::Max(0.2, Activity(TEXT("cattle"), Hour));
			if (Moo(Ear, Here(Cattle()[0] + FVector2D(FMath::FRandRange(-15.f, 15.f), FMath::FRandRange(-15.f, 15.f)), 1.4), 0.9 * Heard)) Counts.FindOrAdd(TEXT("cattle"))++;
		}
	}
}

void FS47Soundscape::Step(double Dt)
{
	if (!bLogChecked) { bLogChecked = true; bLog = FParse::Param(FCommandLine::Get(), TEXT("S47SoundscapeLog")); }
	if (!M || !M->IsFreeform() || M->Mode != US47Main::EMode::Play || !M->Audio || !M->Audio->Night || !M->Player || !M->Freeform)
	{
		if (bHooked || bHush) Reset();
		return;
	}
	Hook();
	if (M->bPausedByMenu || M->ModalOpen() || !FMath::IsFinite(Dt) || Dt <= 0) return;
	AfterHush = FMath::Max(0.0, AfterHush - Dt);
	DevWiresLeft = FMath::Max(0.0, DevWiresLeft - Dt);
	StepHush(Dt);
	StepVoices(Dt, HourNow());
}

bool FS47Soundscape::DevPlay(const FString& Kind, FString& Why)
{
	FVector2D From;
	if (!M || !M->IsFreeform() || !M->Audio || !M->Audio->Night || !M->Player) { Why = TEXT("Freeform only, with the night's sound running"); return false; }
	if (!Road(From)) { Why = TEXT("not on the land here"); return false; }
	if (bHush) { Why = TEXT("the night is hushed: something is near"); return false; }
	Hook();
	FS47Ambience& Night = *M->Audio->Night;
	const FVector Ear = S47Three::ToThree(M->Player->EyeUE());
	auto Nearest = [&From](const TArray<FVector2D>& Spots, double& OutDist) { const FVector2D* Found = nullptr; OutDist = 1e12; for (const FVector2D& Spot : Spots) { const double Gap = FVector2D::Distance(From, Spot); if (Gap < OutDist) { OutDist = Gap; Found = &Spot; } } return Found; };
	double Dist = 0;
	if (Kind == TEXT("windmill"))
	{
		const FVector2D* Mill = Nearest(Mills(), Dist);
		if (!Mill || Dist > 2000) { Why = TEXT("no windmill within two kilometres"); return false; }
		Night.MillCreak(Ear, Here(*Mill, 9.5), 1.0); Counts.FindOrAdd(Kind)++;
		Why = FString::Printf(TEXT("the windmill %.0f m off"), Dist); return true;
	}
	if (Kind == TEXT("cattle"))
	{
		const FVector2D* Herd = Nearest(Cattle(), Dist);
		if (!Herd || !Moo(Ear, Here(*Herd, 1.4), 0.9)) { Why = TEXT("no cattle recordings in this build"); return false; }
		Counts.FindOrAdd(Kind)++; Why = FString::Printf(TEXT("cattle %.0f m off"), Dist); return true;
	}
	if (Kind == TEXT("wires"))
	{
		FVector2D On;
		if (!NearestWire(From, 400, On, Dist)) { Why = TEXT("no power line within 400 m"); return false; }
		DevWiresLeft = 25; Why = FString::Printf(TEXT("the line %.0f m off, for 25 seconds"), Dist); return true;
	}
	FVector Where;
	if (Kind == TEXT("coyote"))
	{
		if (!Place(Kind, Ear, Where, true)) { Why = TEXT("no wash or open land between 220 m and 1.5 km"); return false; }
		Night.CoyotesFrom(Ear, Where); Counts.FindOrAdd(Kind)++;
		Why = FString::Printf(TEXT("coyotes %.0f m off"), FVector2D::Distance(FVector2D(Ear.X, Ear.Z), FVector2D(Where.X, Where.Z))); return true;
	}
	if (Kind == TEXT("owl"))
	{
		if (!Place(Kind, Ear, Where, true) || !Night.PlayAt(TEXT("owl"), Ear, Where, 0.55, FMath::FRandRange(0.92f, 1.04f), 60).IsValid()) { Why = TEXT("no barn, town or station within 650 m"); return false; }
		Counts.FindOrAdd(Kind)++; Why = TEXT("an owl by the nearest buildings"); return true;
	}
	if (Kind == TEXT("train"))
	{
		if (!Place(Kind, Ear, Where, true)) { Why = TEXT("not on the land here"); return false; }
		Night.TrainAt(Ear, Where); Counts.FindOrAdd(Kind)++;
		Why = TEXT("a freight on the line to Roswell, east of the land"); return true;
	}
	Why = TEXT("not a kind of night sound");
	return false;
}

bool FS47Soundscape::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s soundscape: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bCond; };
	Check(Activity(TEXT("coyote"), 12) == 0 && Activity(TEXT("coyote"), 4) > Activity(TEXT("coyote"), 22) && Activity(TEXT("coyote"), 22) > 0, TEXT("no coyotes at noon; most in the hour before dawn"));
	Check(Activity(TEXT("owl"), 14) == 0 && Activity(TEXT("owl"), 2) > 0 && Activity(TEXT("cattle"), 20) > Activity(TEXT("cattle"), 1), TEXT("owls keep to the dark; the cattle low most at dusk"));
	Check(Activity(TEXT("train"), 3) > 0 && Activity(TEXT("train"), 15) > 0 && Activity(TEXT("nothing"), 2) == 0, TEXT("a train any hour; nothing that is not a kind"));
	Check(Activity(TEXT("coyote"), 28) == Activity(TEXT("coyote"), 4) && Activity(TEXT("owl"), -2) == Activity(TEXT("owl"), 22), TEXT("the hours wrap the day"));
	FVector2D On; double Dist = 0;
	Check(NearestWire(FVector2D(25, 1000), 70, On, Dist) && FMath::IsNearlyEqual(On.X, 15.0) && FMath::IsNearlyEqual(Dist, 10.0), TEXT("the main line runs along the highway"));
	Check(NearestWire(FVector2D(200, 200), 70, On, Dist) && Dist < 9 && On.X > 190 && On.X < 210, TEXT("the farm's spur runs east to the farm"));
	Check(!NearestWire(FVector2D(-600, 1300), 70, On, Dist) && Dist > 500, TEXT("out at the mine there is no line to hear"));
	Check(!NearestWire(FVector2D(15, -1000), 70, On, Dist), TEXT("the line ends before the antenna field"));
	const FVector2D TrainA = TrainFrom(FVector2D(0, 500), 0.f), TrainB = TrainFrom(FVector2D(-2000, 500), 1.f);
	Check(TrainA.X > 3000 && TrainB.X > 3000 && TrainA.Y < 500 && TrainB.Y > 500, TEXT("the train is always east of the land, up or down the line"));
	Check(HushReach(70) >= 80 && HushReach(260) > HushReach(70) && HushReach(0) == FS47Omen::MinSense, TEXT("the hush reaches as far as the night's own sense of what comes"));
	Check(Mills().Num() == 2 && Cattle().Num() == 1, TEXT("two windmills and one place with cattle on the land: the farm's pen"));
	{
		// CATTLE-FARM-ONLY-20261113: on and round the farm, faded out by 420 m; at SARO only a whisper of an uneasy herd; nowhere else.
		const FVector2D Pen = Cattle()[0], Saro(S47Geo::SARO_IN_ROAD.X, S47Geo::SARO_IN_ROAD.Z);
		const double Near = CattleHeard(Pen + FVector2D(200, 0), false), Mid = CattleHeard(Pen + FVector2D(0, 320), false);
		Check(Near > .8 && Mid > 0 && Mid < Near && CattleHeard(Pen + FVector2D(0, 430), false) == 0 && CattleHeard(Pen + FVector2D(0, 430), true) == 0,
			TEXT("cattle: the far lowing fades from the farm and is gone past 420 m"));
		const double AtSaro = CattleHeard(Saro, true);
		Check(CattleHeard(Saro, false) == 0 && AtSaro > 0 && AtSaro < .3 && CattleHeard(Saro + FVector2D(0, -200), true) == 0,
			TEXT("cattle: at SARO only a whisper, and only while the herd is uneasy"));
		bool bNone = true;
		for (const FVector2D& Far : { FVector2D(240, 1740), FVector2D(110, 1590), FVector2D(0, 1000), FVector2D(0, 2000), FVector2D(-600, 1300), FVector2D(20, -900) })
			bNone &= CattleHeard(Far, true) == 0;
		Check(bNone, TEXT("cattle: not on the highway, in Caliche, by the old troughs, at the mine or the antenna field"));
	}
	FS47Soundscape Bare(nullptr); Bare.Step(1); Bare.Reset();
	Check(!Bare.Hushed() && Bare.Hushes() == 0, TEXT("without a game nothing is hushed"));
	return bOk;
}

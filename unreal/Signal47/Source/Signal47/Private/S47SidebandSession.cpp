// SIDEBAND-20261113: the session. Reads the game's own signals (never a hidden schedule: what the player can perceive), glides the
// four bands, runs FLARES' events and rules, and is FS47EncounterSound's sink for the ground and the hit (one owner per job).
#include "S47Sideband.h"

#include "S47Ambience.h"
#include "S47Audio.h"
#include "S47CB.h"
#include "S47Fatigue.h"
#include "S47FieldRadio.h"
#include "S47Freeform.h"
#include "S47GearFlicker.h"
#include "S47Hide.h"
#include "S47Main.h"
#include "S47NightTiers.h"
#include "S47Omen.h"
#include "S47RxSearch.h"
#include "S47Score.h"
#include "S47Stress.h"
#include "S47Taken.h"
#include "S47Walker.h"
#include "S47Weather.h"
#include "S47World.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SidebandSession, Log, All);

namespace S47SbSession
{
	using S47Sideband::ETier;
	using S47Sideband::EMode;
	// A curve through (seconds, value) points, a smoothstep glide between them (the prototype's env()).
	struct FPoint { double T, V; };
	double SbEnv(const TArrayView<const FPoint>& P, double At)
	{
		if (At <= P[0].T) return P[0].V;
		for (int32 I = 0; I + 1 < P.Num(); ++I)
		{
			if (At > P[I + 1].T) continue;
			double U = FMath::Clamp((At - P[I].T) / FMath::Max(1e-9, P[I + 1].T - P[I].T), 0., 1.);
			U = U * U * (3. - 2. * U);
			return P[I].V + (P[I + 1].V - P[I].V) * U;
		}
		return P.Last().V;
	}
	// The night scene of the prototypes (colour2/make_flares.py scene(); the 36 dB duck is the moment's own automation).
	const FPoint SceneLevel[] = { { 0, -5 }, { 20, -5 }, { 44, -1.5 }, { 49.3, -1.5 }, { 53.5, 0 }, { 60, -1 }, { 80, -3 } };
	const FPoint SceneWorn[] = { { 0, 0 }, { 20, 0 }, { 30, .3 }, { 44, .9 }, { 46.2, .9 }, { 49, .5 }, { 60, .35 }, { 80, .15 } };
	const FPoint SceneBroken[] = { { 0, 0 }, { 39.5, 0 }, { 45.7, .68 }, { 46.6, .5 }, { 48.8, 0 }, { 80, 0 } };
	const FPoint SceneFar[] = { { 0, 0 }, { 48.6, 0 }, { 52, 1 }, { 58, 1 }, { 80, .2 } };
	const FPoint SceneNear[] = { { 0, 0 }, { 28, .05 }, { 40, .5 }, { 46, 1 }, { 48, 1 }, { 62, .2 }, { 80, .2 } };
	constexpr double SceneHit = 46., SceneBedFrom = 28., SceneBedTo = 63., SceneEnd = 80.;
	constexpr int32 SceneStinger = 2, SceneBed = 2; // full_glass_low over bed_cold_tone
	// -S47SidebandRender=<dir> [-S47SidebandRenderSecs=<n>] [-S47SidebandScene=<s>] [-S47SidebandSceneLen=<s>] [-S47SidebandPiece=<p>]:
	// keep the bus output of a run as a WAVE file, and start the audition scene this many seconds after play begins (time for the
	// world to settle). SIDEBAND-20261113 (d69: an 80 s request recorded 10 s of the scene, the first 8 s had no music and a dev event
	// stopped it at 58 s): with a scene the recording starts with the scene and runs its whole length (SceneLen, default the scene's
	// 80 s and 4 s of tail), and the piece (default s47_cold_dish, the prototype's) owns the score from the scene's first second to
	// the recording's end (FS47Score::Own: through the stems when they may play; nothing else stops it meanwhile).
	struct FAsked { FString Dir, Piece; double Secs = 90, SceneAfter = -1, SceneLen = -1; bool bRead = false, bDone = false; };
	FAsked& Asked()
	{
		static FAsked A;
		if (!A.bRead)
		{
			A.bRead = true;
			FParse::Value(FCommandLine::Get(), TEXT("S47SidebandRender="), A.Dir);
			FParse::Value(FCommandLine::Get(), TEXT("S47SidebandRenderSecs="), A.Secs);
			FParse::Value(FCommandLine::Get(), TEXT("S47SidebandScene="), A.SceneAfter);
			FParse::Value(FCommandLine::Get(), TEXT("S47SidebandSceneLen="), A.SceneLen);
			FParse::Value(FCommandLine::Get(), TEXT("S47SidebandPiece="), A.Piece);
			if (A.SceneAfter >= 0 && A.Piece.IsEmpty()) A.Piece = TEXT("s47_cold_dish");
		}
		return A;
	}
}

FS47Sideband::FS47Sideband(US47Main* InM, const TSharedRef<FS47AudioContext>& InCtx)
	: Bus(MakeShared<FS47SidebandBus>(InCtx)), M(InM), Ctx(InCtx)
{
	// the bands' own glides, seconds for the whole range: up, down
	Glide[0] = { 5., 6. }; Glide[1] = { 2.5, 5. }; Glide[2] = { 5., 6. }; Glide[3] = { 3.4, 6. };
}

FS47Sideband::~FS47Sideband()
{
	*bAlive = false;
}

S47Sideband::ETier FS47Sideband::TierAt(double Clock, int32 Night)
{
	const ES47Allowed A = S47NightTiers::AtClock(Clock, Night);
	return A == ES47Allowed::Full ? S47Sideband::ETier::Full : A == ES47Allowed::Glimpse ? S47Sideband::ETier::Glimpse : S47Sideband::ETier::Hint;
}

// 0 by a building, 1 out on open land. SARO: from 60 m off the station, all of it by 220 m (where FS47FarmSite counts a walker as
// out on foot); the road is open land; the town and the other places are among buildings.
double FS47Sideband::OpenFor(const FString& Area, double FromStation)
{
	if (Area == TEXT("saro")) return FMath::Clamp((FromStation - 60.) / 160., 0., 1.);
	if (Area == TEXT("road")) return 1.;
	return .25;
}

// What a moment may be: a light or a figure first seen far off is never more than a glimpse's; the rest is what the clock allows.
S47Sideband::ETier FS47Sideband::MomentTier(int32 Scare, S47Sideband::ETier Clock)
{
	if (Scare == 3 && Clock == S47Sideband::ETier::Full) return S47Sideband::ETier::Glimpse;
	return Clock;
}

// A voice or a signal sound is audible now: the walkie, the phone, the receiver (its monitor, Tom's search sound, the reference
// recording), a reel, the CB, the AM radio. SIDEBAND touches none of them; its noise layers close and a soft stinger gives way.
bool FS47Sideband::SpeechNow(const US47Main* InM)
{
	if (!InM) return false;
	const US47Audio* A = InM->Audio;
	if (A && (A->PhonePlaying() || A->ReceiverMonitorOn() || A->SoundMixPlaying(TEXT("receiver_reference")) || (A->RadioOn() && InM->Space != TEXT("yard")))) return true;
	if (InM->bRadioHeard) return true;
	if (InM->FieldRadio && (InM->FieldRadio->DaleSpeaking() || InM->FieldRadio->Transmitting() || InM->FieldRadio->Hailing())) return true;
	if (InM->CB && InM->CB->bBusy) return true;
	if (InM->IsFreeform() && InM->Freeform)
	{
		if (InM->Freeform->AudioPlaying()) return true;
		if (InM->Freeform->RxSearch && InM->Freeform->RxSearch->Voices() > 0) return true;
	}
	return false;
}

bool FS47Sideband::CanOwn() const
{
	return M && M->IsFreeform() && M->Freeform && Reacts != S47Sideband::EMode::Off && Bus->LoadFlares();
}

S47Sideband::FDrive FS47Sideband::Read() const
{
	S47Sideband::FDrive D;
	D.Reacts = Reacts;
	D.AfterEncounter = AfterEncounter; D.AfterTaken = AfterTaken; D.HintNudge = HintNudge;
	if (!M || !M->IsFreeform() || !M->Freeform || M->Mode != US47Main::EMode::Play) return D;
	const FS47FreeformSession& F = *M->Freeform;
	D.bFreeform = true;
	const double Clock = F.Clock();
	D.Tier = TierAt(Clock, FS47FreeformSession::NightOf(Clock));
	D.NightHour = S47NightTiers::NightHour(FMath::Fmod(FMath::Max(0., Clock), 86400.) / 3600.);
	D.Fatigue = F.Fatigue ? F.Fatigue->Level() : 0.;
	D.Stress = M->Stress ? M->Stress->Level() : 0.;
	D.Omen = M->Omen ? M->Omen->Level() : 0.;
	D.Gear = M->Gear ? M->Gear->Level() : 0.;
	if (M->FieldRadio)
	{
		D.Approach = M->FieldRadio->Approach(); D.Presence = M->FieldRadio->PresenceLevel();
		D.bSeen = M->FieldRadio->PresenceSeen(); D.bHunt = M->FieldRadio->PresenceHunting();
	}
	const FString Area = M->World ? M->World->Area : FString(TEXT("saro"));
	const bool bDriving = M->World && M->World->bDriving;
	const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
	const bool bMotel = M->World && Area == TEXT("saro") && M->World->AtMotel(P);
	D.bPowerOut = Area == TEXT("saro") && !bMotel && !F.HasPowerForReactiveLight();
	D.bOutdoors = M->Space == TEXT("yard") && !bDriving;
	D.Open = OpenFor(Area, FVector2D(P.X, P.Z).Size());
	D.Wind = S47Weather::Live().bOn ? S47Weather::Live().Wind : 0.;
	D.bSaroIndoors = Area == TEXT("saro") && !bMotel && !bDriving && M->Space == TEXT("room");
	D.bHiding = F.Hide && F.Hide->Hidden();
	D.Hush = M->Audio && M->Audio->Night ? 1. - M->Audio->Night->Openness() : 0.;
	return D;
}

double FS47Sideband::StandBackFor(bool bPlacePiece, double Dread, double Heard) const
{
	// The bought ground was the music under a meeting, and the score stood back to a third for it. FLARES' bed is carved to sit
	// under the score instead (8 to 18 LU under it), so an adaptive cue keeps its level; a place's own piece still goes all but
	// under (Tom, d45: the town's piece must not play on while it comes), by the bed's nearness as it did by the ground's.
	if (!bOwnsGround) return FS47Score::DreadDuck(bPlacePiece, Dread, Heard);
	return bPlacePiece ? FS47Score::DreadDuck(true, FMath::Max(Dread, GroundNow), Heard) : 1.;
}

bool FS47Sideband::Ground(double Low, double High, bool bHeld)
{
	if (!CanOwn()) return false;
	bOwnsGround = true; bGroundFrame = true; bGroundHeld = bHeld;
	const double N = FMath::Max(FMath::IsFinite(Low) ? Low : 0., FMath::IsFinite(High) ? High : 0.);
	GroundWant = bHeld ? 0. : FMath::Clamp(N, 0., 1.);
	if (High > 0) GroundSeen = 1;
	return true;
}

bool FS47Sideband::MomentFor(int32 Scare, double Strength, bool bAudition)
{
	(void)Strength;
	if (bAudition || !CanOwn() || SceneRunning()) return false;
	const double Clock = M->Freeform->Clock();
	const S47Sideband::ETier Tier = MomentTier(Scare, TierAt(Clock, FS47FreeformSession::NightOf(Clock)));
	FString Why;
	FireMoment(Tier, false, -1, Why);
	LastWhy = Why;
	UE_LOG(LogS47SidebandSession, Display, TEXT("S47: FLARES moment (kind %d, %s): %s"), Scare, S47Sideband::TierName(Tier), *Why);
	return true; // taken, with or without a stinger: no bought hit follows, and nothing is queued
}

// The event's one moment. In the full tier the hit, the duck and FAR come whether a stinger sounds or not.
bool FS47Sideband::FireMoment(S47Sideband::ETier Tier, bool bForce, int32 ForceIndex, FString& Why)
{
	using S47Sideband::ETier;
	// (a chain that was out takes a tenth of a second to come across: the moment waits for it, so that no front is lost)
	const bool bCold = !Bus->Warm();
	Bus->Wake();
	const double When = Ctx->CurrentTime() + (bCold ? .12 : .02);
	int32 Pick = ForceIndex; bool bSoft = Stingers == S47Sideband::EMode::Soft;
	if (!bForce)
	{
		if (!Rules.InEvent()) { Rules.Begin(T, Tier); EventQuiet = 0; }
		Rules.Cap(Tier);
		if (Rules.MomentSpent()) { Why = TEXT("this event has had its moment"); return false; }
		Rules.Setting = Stingers;
		Pick = Rules.Moment(T, SpeechNow(M), bSoft, Why, [this](int32 I) { return Bus->HasStinger(I); });
		Tier = Rules.EventTier();
	}
	bool bPlayed = false;
	if (Pick >= 0)
	{
		if (bForce) Bus->EndStinger();
		bPlayed = Bus->Stinger(Pick, Tier, bSoft, When);
		if (bPlayed) Why = FString::Printf(TEXT("%s%s"), S47Sideband::Stingers()[Pick].Name, bSoft ? TEXT(" (soft)") : TEXT(""));
	}
	if (Tier == ETier::Full)
	{
		// with a stinger the hit keeps its crunch but not its bursts; the soft setting has no bursts at all
		Bus->Hit(When, !bPlayed && Reacts == S47Sideband::EMode::On);
		Bus->MomentDuck(When);
		Bus->BedMoment(When);
		FarRise = 2.6; // FAR comes up from 2.6 s, the score comes back inside it
		BedOutAt = T + 13.; // and the bed is gone by 13 s
	}
	else if (Tier == ETier::Hint) HintNudge = 1.;
	return bPlayed;
}

void FS47Sideband::Event(double Dt, bool bActive, S47Sideband::ETier Clock)
{
	if (bActive)
	{
		EventQuiet = 0;
		if (!Rules.InEvent())
		{
			const int32 Bed = Rules.Begin(T, Clock);
			BedOutAt = -1;
			UE_LOG(LogS47SidebandSession, Display, TEXT("S47: FLARES event begins (%s), bed %s"), S47Sideband::TierName(Clock), Bed >= 0 ? S47Sideband::Beds()[Bed] : TEXT("none"));
		}
	}
	else if (Rules.InEvent())
	{
		EventQuiet += Dt;
		if (EventQuiet > 10. && !Bus->StingerSounding())
		{
			// Something was in view and no full moment came of it: still, afterwards, a while of FAR.
			if (GroundSeen > 0 && FarRise < 0 && AfterEncounter < .7 && Rules.EventTier() == S47Sideband::ETier::Full) { AfterEncounter = .7; AfterHold = 4.; }
			Rules.End(); GroundSeen = 0;
		}
	}
}

void FS47Sideband::Audition(int32 Band, double Value, double Seconds)
{
	if (Band < 0 || Band > 3) return;
	AuditionV[Band] = Value; AuditionLeft[Band] = Value < 0 ? 0. : FMath::Max(0., Seconds);
}

bool FS47Sideband::AuditionFlare(S47Sideband::ETier Tier, FString& What)
{
	if (!Bus->LoadFlares()) { What = TEXT("FLARES' baked sounds are not in this build (Content/Audio/Raw/media/sideband)"); return false; }
	// the tier's stingers in turn
	static int32 Turn[4] = { 0, 0, 0, 0 };
	const TArray<S47Sideband::FStinger>& All = S47Sideband::Stingers();
	TArray<int32, TInlineAllocator<8>> Pool;
	for (int32 I = 0; I < All.Num(); ++I) if (All[I].Tier == Tier && Bus->HasStinger(I)) Pool.Add(I);
	if (Pool.IsEmpty()) { What = TEXT("no stinger of that tier is in this build"); return false; }
	const int32 Pick = Pool[Turn[int32(Tier)]++ % Pool.Num()];
	FireMoment(Tier, true, Pick, What);
	return true;
}

void FS47Sideband::StartScene()
{
	SceneT = 0; bSceneHit = false; bSceneBed = false;
	Bus->LoadFlares();
}

void FS47Sideband::StepScene(double Dt)
{
	using namespace S47SbSession;
	SceneT += Dt;
	if (SceneT >= SceneEnd)
	{
		SceneT = -1; Bus->Bed(-1, 0, false); Bus->StandBack(StandWant);
		return;
	}
	Glide[0].Now = SbEnv(SceneWorn, SceneT); Glide[1].Now = SbEnv(SceneBroken, SceneT); Glide[2].Now = 0; Glide[3].Now = SbEnv(SceneFar, SceneT);
	Bus->StandBack(FMath::Pow(10., SbEnv(SceneLevel, SceneT) / 20.));
	const bool bBed = SceneT >= SceneBedFrom && SceneT < SceneBedTo && Bus->HasBed(SceneBed);
	Bus->Bed(bBed ? SceneBed : -1, SbEnv(SceneNear, SceneT), bBed);
	if (!bSceneHit && SceneT >= SceneHit)
	{
		bSceneHit = true;
		const double When = Ctx->CurrentTime() + (Bus->Warm() ? .02 : .12);
		Bus->Wake();
		const bool bPlayed = Stingers != S47Sideband::EMode::Off && Bus->Stinger(SceneStinger, S47Sideband::ETier::Full, Stingers == S47Sideband::EMode::Soft, When);
		Bus->Hit(When, !bPlayed); Bus->MomentDuck(When); Bus->BedMoment(When);
	}
}

bool FS47Sideband::Record(double Seconds, const FString& Path)
{
	if (bRecordAsked) return false;
	Bus->Out->Arm(Seconds); RecordPath = Path; bRecordAsked = true;
	return true;
}

bool FS47Sideband::WriteWav(const FString& Path, const TArray<float>& Stereo, int32 Rate)
{
	TArray<uint8> Bytes; Bytes.Reserve(44 + Stereo.Num() * 2);
	const auto Tag = [&Bytes](const char* S) { Bytes.Append(reinterpret_cast<const uint8*>(S), 4); };
	const auto Put32 = [&Bytes](uint32 V) { for (int32 I = 0; I < 4; ++I) Bytes.Add(uint8(V >> (8 * I))); };
	const auto Put16 = [&Bytes](uint16 V) { Bytes.Add(uint8(V)); Bytes.Add(uint8(V >> 8)); };
	const uint32 Data = uint32(Stereo.Num()) * 2;
	Tag("RIFF"); Put32(36 + Data); Tag("WAVE"); Tag("fmt "); Put32(16); Put16(1); Put16(2); Put32(uint32(Rate)); Put32(uint32(Rate) * 4); Put16(4); Put16(16);
	Tag("data"); Put32(Data);
	for (float V : Stereo) Put16(uint16(int16(FMath::RoundToInt(FMath::Clamp(FMath::IsFinite(V) ? V : 0.f, -1.f, 1.f) * 32767.f))));
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
	return FFileHelper::SaveArrayToFile(Bytes, *Path);
}

FString FS47Sideband::Describe() const
{
	const FS47SidebandBands& B = Bus->Bands();
	return FString::Printf(TEXT("worn %.2f  broken %.2f  wide %.2f  far %.2f  tension %.2f  %s%s%s, %d FLARES sounds, chain %s"),
		B.Worn, B.Broken, B.Wide, B.Far, TensionNow, Rules.InEvent() ? TEXT("event ") : TEXT("no event"), Rules.InEvent() ? S47Sideband::TierName(Rules.EventTier()) : TEXT(""),
		bOwnsGround ? TEXT(", FLARES has the ground") : TEXT(""), Bus->FlaresLoaded(), Bus->Engaged() ? TEXT("in") : TEXT("out"));
}

void FS47Sideband::Step(double Dt)
{
	using namespace S47SbSession;
	Dt = FMath::IsFinite(Dt) ? FMath::Clamp(Dt, 0., .25) : 0.;
	T += Dt;
	const bool bPlay = M && M->Mode == US47Main::EMode::Play;
	// FS47EncounterSound asks through its sink: the sink lives as long as this session does
	if (M && M->FieldRadio && M->FieldRadio.Get() != SinkOn)
	{
		SinkOn = M->FieldRadio.Get();
		FS47EncounterSound::FSink Sink;
		Sink.Ground = [this, Alive = bAlive](double Low, double High, bool bHeld) { return *Alive && Ground(Low, High, bHeld); };
		Sink.Moment = [this, Alive = bAlive](int32 Scare, double Strength, bool bAudition) { return *Alive && MomentFor(Scare, Strength, bAudition); };
		M->FieldRadio->SetEncounterSink(MoveTemp(Sink));
	}
	// (the ground is asked for each frame it is wanted: a frame without the question is a frame without the ground)
	if (!bGroundFrame) { bOwnsGround = false; GroundWant = 0; }
	bGroundFrame = false;

	// the envelopes the drivers read
	if (FarRise >= 0) { FarRise -= Dt; if (FarRise < 0) { AfterEncounter = 1.; AfterHold = 8.; } }
	if (AfterHold > 0) AfterHold -= Dt; else AfterEncounter = FMath::Max(0., AfterEncounter - Dt / 45.);
	const bool bTaken = M && M->IsFreeform() && M->Freeform && M->Freeform->Taken && M->Freeform->Taken->Busy();
	if (bWasTaken && !bTaken) AfterTaken = 1.;
	else AfterTaken = FMath::Max(0., AfterTaken - Dt / 90.);
	bWasTaken = bTaken;
	HintNudge = FMath::Max(0., HintNudge - Dt / 20.);

	const S47Sideband::FDrive D = Read();
	Wanted = S47Sideband::Drive(D);
	// (the chain is made early in a Freeform session that reacts, not at the moment something first comes near)
	if (D.bFreeform && Reacts != S47Sideband::EMode::Off && T > 2. && !bPrebuilt) { bPrebuilt = true; Bus->Warm0(); }
	TensionNow = Wanted.Tension;
	// the omen silence: while the night's sounds have dropped and nothing is seen, the bed keeps out of the first six seconds
	HushFor = D.Hush > .5 && !D.bSeen && !D.bHunt ? HushFor + Dt : 0.;

	// the command line's recording and scene
	FAsked& Ask = Asked();
	if (!Ask.bDone && bPlay && (!Ask.Dir.IsEmpty() || Ask.SceneAfter >= 0))
	{
		Ask.bDone = true;
		if (!Ask.Dir.IsEmpty() && Ask.SceneAfter < 0) Record(Ask.Secs, Ask.Dir / TEXT("sideband_bus.wav"));
		if (Ask.SceneAfter >= 0) SceneDelay = Ask.SceneAfter;
	}
	if (SceneDelay >= 0)
	{
		SceneDelay -= Dt;
		if (SceneDelay < 0)
		{
			// the piece first (it starts in a twentieth of a second), then the recording and the scene on the same frame
			if (!Ask.Piece.IsEmpty() && M && M->Score) bScoreOwned = M->Score->Own(Ask.Piece);
			const double Len = Ask.SceneLen > 0 ? Ask.SceneLen : S47SbSession::SceneEnd + 4.;
			if (!Ask.Dir.IsEmpty()) Record(Len, Ask.Dir / TEXT("sideband_bus.wav"));
			UE_LOG(LogS47SidebandSession, Display, TEXT("S47: SIDEBAND scene from the command line: %.0f s%s, piece %s"), Len, Ask.Dir.IsEmpty() ? TEXT("") : TEXT(" recorded"), Ask.Piece.IsEmpty() ? TEXT("(what plays)") : *Ask.Piece);
			StartScene();
			if (Ask.Dir.IsEmpty()) OwnUntil = T + Len;
		}
	}
	if (bScoreOwned && ((OwnUntil >= 0 && T >= OwnUntil) || (OwnUntil < 0 && !bRecordAsked)))
	{
		bScoreOwned = false; OwnUntil = -1;
		if (M && M->Score) M->Score->Own(FString());
	}

	if (SceneRunning()) StepScene(Dt);
	else
	{
		const double Want[4] = { Wanted.Bands.Worn, Wanted.Bands.Broken, Wanted.Bands.Wide, Wanted.Bands.Far };
		for (int32 I = 0; I < 4; ++I)
		{
			double V = Want[I];
			if (AuditionLeft[I] > 0) { AuditionLeft[I] -= Dt; V = AuditionV[I]; }
			Glide[I].Step(Dt, V);
		}
		Bus->StandBack(StandWant);
		// FLARES' bed: only inside an event, never in a hint, capped in a glimpse, and held back under the omen silence
		const bool bActive = bOwnsGround && bPlay && GroundWant > .02;
		Event(Dt, bActive, D.Tier);
		const int32 Bed = Rules.EventBed();
		const double Cap = Rules.EventTier() == S47Sideband::ETier::Glimpse ? FMath::Min(Wanted.BedCap, S47Sideband::GlimpseBedCap) : Rules.EventTier() == S47Sideband::ETier::Full ? Wanted.BedCap : 0.;
		const double Near = HushFor > 0 && HushFor < 6. ? 0. : FMath::Min(GroundWant, Cap);
		GroundNow = bActive ? Near : 0.;
		Bus->Bed(Bed, Near, bActive && Bed >= 0 && Near > .01 && !(BedOutAt >= 0 && T >= BedOutAt));
	}
	Bus->Set({ Glide[0].Now, Glide[1].Now, Glide[2].Now, Glide[3].Now });
	Bus->NoiseOpen(!SpeechNow(M));
	if (M && M->Score)
	{
		Bus->Hold(M->Score->MusicHeld());
		const FString File = M->Score->PlayingFile();
		if (!File.IsEmpty()) Bus->FarTrim(S47Sideband::FarTrimDb(File));
	}
	Bus->Step(Dt);

	if (bRecordAsked && Bus->Out->Full())
	{
		bRecordAsked = false;
		const TArray<float> Got = Bus->Out->Take();
		const bool bOk = WriteWav(RecordPath, Got, int32(Ctx->SampleRate));
		UE_LOG(LogS47SidebandSession, Display, TEXT("S47: SIDEBAND bus recording %s: %s (%.1f s)"), bOk ? TEXT("written") : TEXT("FAILED"), *RecordPath, Got.Num() / 2. / Ctx->SampleRate);
	}
}

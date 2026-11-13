#pragma once

#include "CoreMinimal.h"
#include "S47WebAudio.h"
#include "Math/RandomStream.h"

class US47Main;

// SIDEBAND-20261113 (Tom, 13 November: «Likte alt. Bygg Sideband og Flares.»). The game's music system, in three parts:
//   the colour bus   four bands on the SCORE's bus only, each one intensity 0 to 1: WORN (late, tired, something is off), BROKEN
//                    (it is close, the gear is failing; the gentle one), WIDE (open land, wind) and FAR (after)
//   FLARES           a horror bed that exists only inside an event, and at most one stinger per event, chosen by the time tier
//   channels         the nine own mixes as stems on their bar grid (FS47Score, S47ScoreStems.cpp)
// Built from the two offline prototypes Tom heard (Docs/Design/Audio/SIDEBAND_20261113.md has every chain and number): only the
// nodes of S47WebAudio.h. Speech, the walkie, the phone, the receiver's signal sounds and the AM radio never pass any of it: it
// sits between the score's decks and the score's own bus.
//
//   decks -> FS47Score::Mix -> Head (the score standing back) -> Moment (the 36 dB duck after a close sight)
//         -> [ Thru | Pre -> WORN -> BROKEN -(+ FLARES)-> WIDE -> FAR -> Post ] -> Out -> FS47Score::Bus (MusicDuck) -> Audio->Music
//
// With all four bands at zero and no flare sounding the chain is not connected at all (Thru alone: three gains at one), so the
// idle cost is nil and the score is untouched to the sample.
struct FS47SidebandBands
{
	double Worn = 0, Broken = 0, Wide = 0, Far = 0;
	bool Any() const { return Worn > 0 || Broken > 0 || Wide > 0 || Far > 0; }
};

namespace S47Sideband
{
	// The game's time tiers as FLARES counts them (S47NightTiers: 18 to 24 deniable hints, 00 to 02 distant glimpses, from 02 full).
	enum class ETier : uint8 { None, Hint, Glimpse, Full };
	// The two settings: "Music reacts" on / soft / off, and "Stingers" full / soft / off (Full is On's number).
	enum class EMode : uint8 { On, Soft, Off };
	SIGNAL47_API EMode ModeFrom(const FString& Id);        // "on"/"full", "soft", "off"; anything else: On
	SIGNAL47_API const TCHAR* ReactsId(EMode In);          // "on" | "soft" | "off"
	SIGNAL47_API const TCHAR* StingersId(EMode In);        // "full" | "soft" | "off"
	SIGNAL47_API const TCHAR* TierName(ETier In);

	// ---------- the drivers: game state in, four band targets out (pure) ----------
	struct FDrive
	{
		bool bFreeform = false;      // nothing outside Freeform: Story keeps the score it had
		EMode Reacts = EMode::On;
		ETier Tier = ETier::Hint;    // by the clock and the week (S47NightTiers::AtClock)
		double NightHour = 20;       // 12 to 36, past midnight counted on (S47NightTiers::NightHour)
		double Fatigue = 0;          // FS47Fatigue::Level
		double Stress = 0;           // FS47Stress::Level
		double Omen = 0;             // FS47Omen::Level: what is out there near the player, as the gear feels it
		double Approach = 0;         // FS47FieldRadio::Approach: how near anything real is as the set hears it (48 m)
		double Presence = 0;         // FS47FieldRadio::PresenceLevel: the cues' 18 m
		bool bSeen = false, bHunt = false;
		double Gear = 0;             // FS47GearFlicker::Level
		bool bPowerOut = false;      // SARO dark: no mains and no generator
		bool bOutdoors = false;      // M->Space == "yard", on foot
		double Open = 0;             // 0 by a building, 1 out on open land
		double Wind = 0;             // S47Weather::Live().Wind
		bool bSaroIndoors = false;   // under SARO's roof
		bool bHiding = false;        // FS47Hide::Hidden
		double AfterEncounter = 0;   // 1 just after a meeting, gone in about a minute (the session's envelope)
		double AfterTaken = 0;       // 1 as the player comes to after lost time, gone in about a minute and a half
		double HintNudge = 0;        // 1 just after a deniable hint, gone in 20 s
		double Hush = 0;             // 1 - FS47Ambience::Openness: the night's sounds have dropped
	};
	struct FWant
	{
		FS47SidebandBands Bands;
		double Tension = 0;          // 0 to 1, for the stems
		double BedCap = 1;           // the most the FLARES bed may open (the tier's cap, and the omen silence's)
		bool bRoutine = false;       // indoors at SARO with nothing about: dry
	};
	SIGNAL47_API FWant Drive(const FDrive& In);
	// Caps, for the tests and the docs.
	inline constexpr double WornCap = .4, BrokenHintCap = .12, BrokenGlimpseCap = .35, BrokenFullCap = .68, WideCap = .8;
	inline constexpr double SoftWorn = .2, SoftBroken = .3, SoftWide = .4, SoftFar = .6;
	inline constexpr double HushBedCap = .12, HushBrokenCap = .25, GlimpseBedCap = .35;

	// A band's own slew: the whole range in Up seconds on the way up and Down on the way down (2 to 6 s; the hit is automation).
	struct FGlide
	{
		double Up = 3, Down = 5, Now = 0;
		double Step(double Dt, double Want);
	};

	// ---------- FLARES' rules (pure; colour2/flares_rules.py) ----------
	struct FStinger { const TCHAR* Name; ETier Tier; double TrimDb; };
	SIGNAL47_API const TArray<FStinger>& Stingers(); // the nine
	SIGNAL47_API const TArray<const TCHAR*>& Beds(); // the three
	inline constexpr double CooldownHint = 90, CooldownGlimpse = 120, CooldownFull = 180, FullGap = 300;
	inline constexpr int32 FullRun = 2; // of any three full events in a row at most this many get a stinger
	class SIGNAL47_API FRules
	{
	public:
		explicit FRules(int32 Seed = 47) : Rng(Seed) {}
		EMode Setting = EMode::On;
		// An event begins. Returns its bed (index into Beds(), -1: none in this tier): never the last event's.
		int32 Begin(double T, ETier Tier);
		// The event's one moment. Returns the stinger (index into Stingers(), -1: none) and whether it plays soft. bSpeech: a
		// voice or a signal sound is audible. Why: a word for the log. Have: unset, or says which stingers' files are there.
		int32 Moment(double T, bool bSpeech, bool& bSoft, FString& Why, TFunctionRef<bool(int32)> Have);
		int32 Moment(double T, bool bSpeech, bool& bSoft, FString& Why) { return Moment(T, bSpeech, bSoft, Why, [](int32) { return true; }); }
		void End() { bEvent = false; }
		// The event's moment may be no more than this (a light far off in the full hours is still a glimpse's).
		void Cap(ETier Max) { if (bEvent && uint8(Tier) > uint8(Max)) Tier = Max; }
		bool InEvent() const { return bEvent; }
		ETier EventTier() const { return bEvent ? Tier : ETier::None; }
		bool MomentSpent() const { return bEvent && bSpent; }
		int32 EventBed() const { return bEvent ? Bed : -1; }
	private:
		FRandomStream Rng;
		bool bEvent = false, bSpent = false;
		ETier Tier = ETier::None, LastTier = ETier::None;
		int32 Bed = -1, LastBed = -1, LastStinger = -1, LastOfTier[4] = { -1, -1, -1, -1 };
		double LastTime = -1e9, LastFullTime = -1e9;
		TArray<bool> FullHistory;
		int32 Decide(double T, bool bSpeech, bool& bSoft, FString& Why, TFunctionRef<bool(int32)> Have);
	};
	// The tier's chain for a stinger (colour2 NOTES, "The tiers"): level on the baked buffer in dB, lowpass, direct and room send.
	struct FTierMix { double LevelDb, Lowpass, Direct, Send; };
	SIGNAL47_API FTierMix TierMix(ETier Tier, bool bSoft, double TrimDb);
	// The bed's level (dB on the baked loop) and lowpass (Hz) for a nearness 0 to 1.
	SIGNAL47_API void BedFor(double Near, double& LevelDb, double& Lowpass);
	// The wet trim of FAR for a piece of the score (dB; a held pad rings up more than a plucked guitar). By the file's name.
	SIGNAL47_API double FarTrimDb(const FString& File);
}

// A graph node that passes its input on and keeps a copy (dev: -S47SidebandRender, F10 "record").
class SIGNAL47_API FS47SidebandTap : public FS47AudioNode
{
public:
	explicit FS47SidebandTap(const TSharedRef<FS47AudioContext>& InCtx) : FS47AudioNode(InCtx) {}
	void Arm(double Seconds);                 // start keeping (stereo, interleaved), for this long
	bool Armed() const { return bArmed; }
	bool Full() const;                        // the time asked for is in
	TArray<float> Take();                     // what was kept; keeping stops
protected:
	virtual void Process(int64 Quantum) override;
private:
	TArray<float> Kept;
	int32 Want = 0;
	bool bArmed = false;
};

// The colour bus and FLARES' two voices: the graph. Knows nothing of the game.
class SIGNAL47_API FS47SidebandBus
{
public:
	explicit FS47SidebandBus(const TSharedRef<FS47AudioContext>& InCtx);
	~FS47SidebandBus();
	FS47SidebandBus(const FS47SidebandBus&) = delete;
	FS47SidebandBus& operator=(const FS47SidebandBus&) = delete;

	TSharedPtr<FS47GainNode> Head;       // in: the score's decks
	TSharedPtr<FS47SidebandTap> Out;     // out: to the score's bus

	// The four bands as they are to sound now (already glided by the caller); the graph follows in a tenth of a second.
	void Set(const FS47SidebandBands& In);
	const FS47SidebandBands& Bands() const { return Now; }
	// Each frame: brings the chain in and out, and the room with it.
	void Step(double Dt);
	// The score standing back (0 to 1, the place's piece under the ground), on the score only: FLARES is not ducked by it.
	void StandBack(double InLevel);
	// Noise layers (WORN's hiss and crackle, BROKEN's static) closed while a voice or a signal sound is audible.
	void NoiseOpen(bool bOpen);
	// The score is held (a listening hold, the take): the room's return is closed, so no tail rings on over it.
	void Hold(bool bHeld);
	// FAR's wet trim for the piece that plays (dB).
	void FarTrim(double Db);
	// BROKEN's hit at When (context time): the push to full, the static and the soft dip; with bBurst also the bursts of drive
	// and level (left out when a stinger lands in the same instant and is the loud part).
	void Hit(double When, bool bBurst);
	// The full tier's habit after a close sight: 0.2 s after When the score falls 36 dB in 0.1 s, stays 3 s, is back in 4.2 s.
	void MomentDuck(double When);

	// ---------- FLARES' voices ----------
	// The baked forms, read at first use from Content/Audio/Raw/media/sideband/<name>.wav. False: none of them is there.
	bool LoadFlares();
	bool HasStinger(int32 Index) const { return StingerBuf.IsValidIndex(Index) && StingerBuf[Index].IsValid(); }
	bool HasBed(int32 Index) const { return BedBuf.IsValidIndex(Index) && BedBuf[Index].IsValid(); }
	int32 FlaresLoaded() const;
	// The bed: Index into S47Sideband::Beds() (-1: none), how near (0 to 1) and whether the event is on. It fades itself.
	void Bed(int32 Index, double Near, bool bOn);
	bool BedSounding() const { return BedSrc.IsValid(); }
	double BedNear() const { return BedSrc.IsValid() && bBedOn ? BedNow : 0.; }
	// The bed's part in the full moment: down 20 dB by 0.6 s, back to about half with the score, gone by 13 s.
	void BedMoment(double When);
	// One stinger at When through the tier's chain. False: its file is not there, or one is sounding.
	bool Stinger(int32 Index, S47Sideband::ETier Tier, bool bSoft, double When);
	bool StingerSounding() const;

	bool Engaged() const { return bEngaged; }
	// The chain is in and across: a sound started now is heard from its first frame. Wake brings it in (a tenth of a second).
	bool Warm() const;
	void Wake();
	bool RoomEngaged() const { return bRoom; }
	int32 Nodes() const { return NodeCount; }
	// For the tests: stand-in buffers instead of the files.
	void TestFlares(const TSharedPtr<FS47AudioBuffer>& InStinger, const TSharedPtr<FS47AudioBuffer>& InBed);
	static constexpr double Level = .43;        // the decks' usual gain: the chain works at the staged level (1 / this before it)
	static constexpr double Tail = 14.;         // seconds at zero before the chain is let go (the room's own tail is 6.5 s)
	static constexpr double Latency = .012;     // WORN's delay

private:
	friend class FS47Sideband;
	TSharedRef<FS47AudioContext> Ctx;
	struct FAim { FS47AudioParam* P = nullptr; TFunction<double(double)> Fn; };
	TArray<FAim> WornAims, BrokenAims, WideAims, FarAims;
	TArray<TSharedPtr<FS47AudioNode>> Keep;         // every node of the chain (sources and modulators have no other owner)
	TArray<TSharedPtr<FS47ScheduledSourceNode>> Sources;
	TSharedPtr<FS47GainNode> Moment, Thru, Pre, ChainOut;
	TSharedPtr<FS47GainNode> WornNoise, BrokenNoise, FlareSum, RoomIn, Return, FarOut, Ones;
	TSharedPtr<FS47GainNode> HitDrive, HitLevel, HitStatic, HitHole;
	TSharedPtr<FS47AudioBuffer> OnesBuf;
	FS47SidebandBands Now, Pushed;
	bool bBuilt = false, bEngaged = false, bRoom = false, bNoise = true, bHeld = false;
	double IdleFor = 0, RoomIdleFor = 0, HitUntil = 0, RoomUntil = 0, EngagedAt = 0, LetGoAt = -1;
	double TrimDb = -6.5, StaticK = 1, StandNow = 1;
	int32 NodeCount = 0;
	TArray<float> HoleCurve;
	// FLARES
	TArray<TSharedPtr<FS47AudioBuffer>> StingerBuf, BedBuf;
	bool bFlaresTried = false;
	TSharedPtr<FS47BufferSourceNode> BedSrc, StingSrc;
	TSharedPtr<FS47GainNode> BedGain, BedDuck, StingGain;
	TSharedPtr<FS47BiquadFilterNode> BedLp;
	TArray<TSharedPtr<FS47AudioNode>> StingNodes;
	int32 BedIndex = -1;
	bool bBedOn = false;
	double BedNow = 0, BedOffFor = 0, StingEnd = 0;

	template <typename T> TSharedRef<T> Add(TSharedRef<T> Node) { Keep.Add(Node); ++NodeCount; return Node; }
	TSharedRef<FS47GainNode> G(double V);
	TSharedRef<FS47BiquadFilterNode> Bq(const TCHAR* Type, double Hz, double Q, double GainDb = 0);
	TSharedRef<FS47OscillatorNode> Osc(double Hz, const TCHAR* Type = TEXT("sine"));
	TSharedRef<FS47BufferSourceNode> Loop(const TSharedPtr<FS47AudioBuffer>& B, double Offset = 0);
	TSharedRef<FS47GainNode> Mono(const TSharedPtr<FS47AudioNode>& From);
	void Build();
	void BuildWorn(const TSharedPtr<FS47AudioNode>& In, const TSharedPtr<FS47AudioBuffer>& Hiss);
	void BuildBroken(const TSharedPtr<FS47AudioBuffer>& Hiss);
	void BuildWide();
	void BuildFar();
	TSharedPtr<FS47GainNode> WornOut, BrokenOut, WideOut;
	void Engage();
	void LetGo();
	void Room(bool bOn);
	void Push(TArray<FAim>& Aims, double V, double At);
	void StopBed();
	void EndStinger();
};

// The session: reads the game, drives the bus, runs FLARES' rules, and stands in for FS47EncounterSound's bought ground and hit
// while it can (S47SidebandSession.cpp).
class SIGNAL47_API FS47Sideband
{
public:
	FS47Sideband(US47Main* InM, const TSharedRef<FS47AudioContext>& InCtx);
	~FS47Sideband();
	TSharedRef<FS47SidebandBus> Bus;
	S47Sideband::FRules Rules;
	S47Sideband::EMode Reacts = S47Sideband::EMode::On, Stingers = S47Sideband::EMode::On;

	void Step(double Dt);
	// What the game is asked for this frame (after the drivers, before the glides).
	const S47Sideband::FWant& Want() const { return Wanted; }
	double Tension() const { return TensionNow; }
	// ---------- FS47EncounterSound's sink (one owner per job) ----------
	// The ground: true when FLARES has it (the files are there, the setting is not off, Freeform): the caller keeps its own beds down.
	bool Ground(double Low, double High, bool bHeld);
	// A moment (FS47EncounterSound::EScare by number: 0 seen, 1 hunted, 2 a light near, 3 a light far off). True: FLARES took
	// it, with or without a stinger, and no bought hit follows.
	bool MomentFor(int32 Scare, double Strength, bool bAudition);
	bool GroundOwned() const { return bOwnsGround; }
	double GroundNear() const { return GroundNow; }
	// What the score should stand back to, from FS47Score::DreadDuck's two cases; and S47Main hands the result in each frame.
	double StandBackFor(bool bPlacePiece, double Dread, double Heard) const;
	void SetStandBack(double InLevel) { StandWant = FMath::IsFinite(InLevel) ? FMath::Clamp(InLevel, 0., 1.) : 1.; }

	// ---------- dev (F10, S47SidebandDev.cpp) ----------
	// One band held at a value for some seconds (Band 0 to 3; Value under 0: let go).
	void Audition(int32 Band, double Value, double Seconds = 20);
	// A flare of this tier now, whatever the rules say. False: the files are not there.
	bool AuditionFlare(S47Sideband::ETier Tier, FString& What);
	// The prototype's 75 s scene against whatever the score plays (the desk, the walk out, the sight, after).
	void StartScene();
	bool SceneRunning() const { return SceneT >= 0; }
	// Keep the bus output for this long and write it as a WAVE file. False: already recording.
	bool Record(double Seconds, const FString& Path);
	FString Describe() const;

	static bool SelfTest(TArray<FString>& Lines);
	static bool WriteWav(const FString& Path, const TArray<float>& Stereo, int32 Rate = 48000);
	// Pure pieces of Step, for the tests.
	static S47Sideband::ETier TierAt(double Clock, int32 Night);
	static double OpenFor(const FString& Area, double FromStation);
	static S47Sideband::ETier MomentTier(int32 Scare, S47Sideband::ETier Clock);
	static bool SpeechNow(const US47Main* M);

private:
	US47Main* M = nullptr;
	TSharedRef<FS47AudioContext> Ctx;
	S47Sideband::FWant Wanted;
	S47Sideband::FGlide Glide[4];
	double T = 0, TensionNow = 0, GroundNow = 0, GroundWant = 0, GroundSeen = 0, EventQuiet = 0, HushFor = 0;
	double AfterEncounter = 0, AfterHold = 0, AfterTaken = 0, HintNudge = 0, FarRise = -1;
	double BedOutAt = -1; // after a full moment the bed dies away and stays out for the rest of that event
	bool bOwnsGround = false, bWasTaken = false, bGroundFrame = false, bGroundHeld = false;
	double AuditionV[4] = { -1, -1, -1, -1 }, AuditionLeft[4] = { 0, 0, 0, 0 };
	double SceneT = -1, SceneDelay = -1, StandWant = 1; bool bSceneHit = false, bSceneBed = false;
	TSharedRef<bool> bAlive = MakeShared<bool>(true);
	const void* SinkOn = nullptr; // the FS47FieldRadio whose encounter sound asks this session
	FString RecordPath; bool bRecordAsked = false;
	FString LastWhy;
	void Event(double Dt, bool bActive, S47Sideband::ETier Clock);
	bool FireMoment(S47Sideband::ETier Tier, bool bForce, int32 ForceIndex, FString& Why);
	void StepScene(double Dt);
	S47Sideband::FDrive Read() const;
	bool CanOwn() const;
};

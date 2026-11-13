#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class FS47MediaSource;
class FS47GainNode;
class FS47EveningVisual;

// STORY-DRIVE-OWN-CAR-20261113 (Tom 13 Nov: «I Story har spilleren en egen bil som man kjører til SARO og man hører på en radio»).
// A Story night's drive to SARO is the old Story's again: the player's own Fairheaven SW '84 station wagon (FS47EveningVisual: its own
// body, interior, hinged driver door and driver eye; US47World::BeginCommute borrows the SARO truck's drive and cab, hidden by
// FS47Truck::SetPersonalVehicle, never its odometer, fuel or wear), up the highway through Caliche from 17:20, the Mesa Diner stop
// optional, parked in SARO's visitor lot (US47World::FinishCommute: the wagon stays there, SARO 07 waits on its pad for the night).
//
// The wagon's radio is 1420's local evening host, as the old drive had it (FS47Evening::RadioStep): the story's radio scenes (I01_DRIVE,
// I10_BACK_ON_ROAD, played by FS47StoryScene) are voiced from their recorded takes on the radio bus (channel car-radio), the line as a
// subtitle; under them a music bed. The bed is Tom's own "Intro Drive" (IntroBed, from the git-ignored licensed folder when it is there),
// once in full from the start of the drive, the host held for its first IntroLead seconds and then heard over it, ducked; then the old bed (LoopBed) looped, paused
// while the host speaks. Without the Intro Drive file the old bed plays throughout. The radio key turns it on and off; the general car
// radio (US47Main::CarRadio) is silent while this one owns the dial.
//
// Story nights only: everything here is reached through FS47StoryDirector::Of (a Story night's active director); Freeform never has one.
// The save is the director's "car": {v:1, phase:"drive"|"parked", at, area, x,y,z,h (the wagon while driven or left on the way),
// px,py,pz,ph (where it is parked at SARO), radioOn, introDone, introAt, bedAt}. No "car" (older saves): no wagon, as before.
class SIGNAL47_API FS47StoryDrive
{
public:
	// The beds: named so another track can replace either later.
	static constexpr const TCHAR* IntroBed = TEXT("signal47_intro_drive");   // Content/Licensed/Raw (index key lic:signal47_intro_drive)
	static constexpr const TCHAR* LoopBed = TEXT("radio/bama_country.mp3");  // the old evening bed (Content/Audio/Raw/media)
	static constexpr double LoopGain = 0.23;   // the old bed's level on the radio bus (FS47Evening::RadioStep)
	static constexpr double IntroGain = 0.08;  // Tom's tracks are mastered louder (the menu's own track: 0.2 against the old 0.6)
	static constexpr double IntroDuck = 0.4;   // the intro's level under the host's voice, as a share of IntroGain
	static constexpr double IntroLead = 20;    // the track is 3:06: the host leaves its opening alone for this long, then talks over it ducked
	static constexpr double IntroTail = 4;     // the host may come in over the intro's last seconds, never over its opening
	static constexpr int32 Dial = 1420;

	enum class EPhase : uint8 { None, Drive, Parked };

	explicit FS47StoryDrive(US47Main* InMain);
	~FS47StoryDrive();
	// The drive of a Story night's active director, or null (no Story night: Freeform, the old Story, the title).
	static FS47StoryDrive* Of(const US47Main* Main);

	// A new Story night: into the wagon at the old drive start (US47World::BeginCommute). False when the world cannot start it.
	bool BeginNew();
	// Continue: the director's "car" (null: none, an older save).
	void Load(const S47J::FObj& Saved);
	S47J::FObj Save() const;
	// Hides the wagon and lets go of the radio and the truck rig (the director's Stop: the night's end, another start).
	void Stop();
	// Each tick of the director. bHostOn: a radio scene of the drive is under way (FS47StoryScene::CarRadio).
	void Tick(double Dt, bool bHostOn);

	EPhase GetPhase() const { return Phase; }
	bool Driving() const { return Phase == EPhase::Drive; }

	// ---- the wagon's radio ----
	// The drive owns the dial: in (or by) the wagon on the drive. The general car radio is silent meanwhile.
	bool OwnsRadio() const;
	bool RadioOn() const { return bRadioOn; }
	// At the wheel, radio on, no panel up: the host and the bed are heard.
	bool Audible() const;
	// The host may speak now: the Intro Drive is past its first IntroLead seconds (or its last IntroTail, for a short one), or there is none.
	bool HostMayTalk() const;
	// The radio key on the drive (US47Main::TurnRadio).
	void ToggleRadio();
	// The truck's choice where the wagon stands during the drive (US47World::TruckChoice): "Drive your car to SARO".
	static bool Choice(US47Main* Main, FString& OutLabel, TFunction<void()>& OutUse);

	// The Intro Drive file, or empty when it is not in this build.
	static FString IntroFile();
	// Whether the host waits for the intro (pure; the self-test's).
	static bool IntroHolds(bool bIntro, bool bIntroDone, double At, double Len) { return bIntro && !bIntroDone && At < FMath::Min(IntroLead, Len - IntroTail); }
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	EPhase Phase = EPhase::None;
	bool bRadioOn = true, bIntroDone = false, bNoIntro = false;
	double IntroAt = 0, BedAt = 0;
	TSharedPtr<FS47MediaSource> Intro, Bed;
	TSharedPtr<FS47GainNode> IntroLevel, BedLevel;
	FS47EveningVisual* Visual() const;
	void Arrive();
	void StepRadio(bool bHostOn);
	void StopBeds();
	// -S47StoryDriveTest: the live check's lines, once each (S47: story drive test PASS/FAIL ...).
	TSet<FString> Told;
	void Tell(bool bPass, const FString& What);
};

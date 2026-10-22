#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"
#include "S47Chapter.h"
#include "S47TheThird.h"
#include "S47Chapter1.h"
#include "S47Docs.h"
#include "S47FieldCamera.h"
#include "S47Json.h"
#include "S47Chapter3.generated.h"

class US47Station;
class US47StationTaken;
class US47Chapter1;

// story/Chapter3.ts Ch3State: the saved state of chapter three, written and read with the
// TypeScript's own field names ({ v: 1, stage, arrived, readE09a, readE10, heard, seen: { footing,
// markerA, markerB, stakes, diagram, hut }, lampCovered, lampObserved, cableInspected, f3?, f4?,
// p06, p07, p08, p09, wrong06, wrong08, wrong09, rejected, called, dev, links?, fb? }).
struct SIGNAL47_API FS47Ch3State
{
	FString Stage = TEXT("to-truck");  // Stage3: 'to-truck' | 'station' | 'call' | 'return' | 'develop' | 'complete'
	bool bArrived = false;
	bool bReadE09a = false, bReadE10 = false, bHeard = false;
	struct FSeen { bool bFooting = false, bMarkerA = false, bMarkerB = false, bStakes = false, bDiagram = false, bHut = false; };
	FSeen Seen;
	bool bLampCovered = false, bLampObserved = false, bCableInspected = false;
	TOptional<FS47Photo> F3, F4;
	bool bP06 = false, bP07 = false, bP08 = false, bP09 = false;
	int32 Wrong06 = 0, Wrong08 = 0, Wrong09 = 0, Rejected = 0;
	bool bCalled = false;
	int32 Dev = 0;                      // the field roll: 0 sealed, 1 developed, 2 print in fixer, 3 collected
	TOptional<TArray<FString>> Links;   // this chapter's threads on the case board, 'a|b' sorted
	// T. Vega's field book from the drawer in the hut, once read (story/fieldBook.ts FbSave): kept
	// as the save had it, as the field book is not in this version
	TSharedPtr<FJsonValue> Fb;
	S47J::FObj Extra;                   // a saved state as it came, so fields this version does not know stay in it

	bool& SeenOf(const FString& What);  // s.seen[what]
	S47J::FObj Save() const;
	// { ...fresh(), ...saved, seen: { ...fresh().seen, ...saved.seen } }
	static FS47Ch3State Load(const S47J::FObj& Saved);
};

// story/theThird.ts FilmShot: the third one, for one exposure of FRAME 03 (S47TheThird.h)

// story/Chapter3.ts, chapter three, "The Survey Station" (K3 in the design bible). Ported from the
// Unity field pass (Station26): P06, which fixed point moved, with FRAME 03; P07, the local lamp
// null test; P08, the deliberate cable cut, with FRAME 04; P09, the 1947 timing log against
// tonight's receiver. P06 and P07 go in either order, P08 needs both and its own photograph, P09
// needs P08. P07 is made at the lamp; P06, P08 and P09 are made on the case board, by tying the
// records and the frames together (SPILLDESIGN.md: "Kapittel 3 på bordet").
// New in the web version: the drive out in the SARO service truck, Nora's call on the hut's field
// telephone after P09, and the drive back to develop both frames at the SARO wet bench.
//
// The host side (Prologue.ts beginChapter3 and jumpChapter3, main.ts's hooks for the chapter, its
// ch3.onEnd and the station from world/Station01.ts) is here too, as BeginChapter3, JumpChapter3
// and Register.
UCLASS()
class SIGNAL47_API US47Chapter3 : public US47Chapter
{
	GENERATED_BODY()

public:
	// The constructor's work (the rule book) and main.ts's for chapter three: the station
	// (world.site), ch3.bindSaroTruck(yard.objs.truck), world.onStationLoaded (bindSite and the walk
	// gate), world.onArriveStation / onArriveSaro, world.truckChoice, the stationTaken hooks,
	// ch3.onEnd; and Prologue.ts's jump('chapter3'). S47Chapters.cpp: US47Chapter3::Register(Make<US47Chapter3>(Host, M)).
	static US47Chapter3* Register(US47Chapter3* C);
	virtual void BeginDestroy() override;

	// ---------- lifecycle ----------
	virtual void Begin(const S47J::FObj& Saved) override; // begin(saved: Ch3State | null)
	virtual void Reset() override;
	virtual S47J::FObj SaveState() const override;        // this.s once started (Prologue.composeCase: ch3), else null
	virtual void Update(double Dt, double T) override;
	virtual FString Objective() const override;
	virtual TArray<FS47Task> Tasks() const override;
	virtual FString SaveBlock() const override;
	virtual FString Key() const override { return TEXT("ch3"); }

	// (Chapter3.active is US47Chapter::bActive: begun, until reset or chapter four begins)
	bool bStarted = false;
	bool bRinging = false;
	FS47Ch3State S;
	TFunction<void()> OnEnd;   // main.ts ch3.onEnd (Register sets it)
	// At the station after the call, everyone goes (story/stationTaken.ts, main.ts): the hut door
	// shut by itself and what trying it does, and the truck without its keys ('' from Truck: null).
	struct FTaken { TFunction<bool()> Shut; TFunction<void()> TryDoor; TFunction<FString()> Truck; };
	TOptional<FTaken> Taken;
	// Something of the night's is in the middle of the viewfinder, nearer than the case's subjects
	// (the ninth cow lying by B's vane, story/ninthCow.ts; main.ts): the shutter takes a free frame of
	// it rather than refusing a case frame. Not ported: unset.
	TFunction<bool()> FreeShot;
	// The third one on FRAME 03 (story/theThird.ts; main.ts): where it stands for this exposure, out on
	// C's ground in the picture, or unset. Not ported: unset.
	TFunction<TOptional<FS47FilmShot>(US47FieldCamera* Cam)> Third;
	// The band on FRAME 03 has been looked at closely (main.ts writes it down, once). Not ported: unset.
	TFunction<void()> OnBand;
	// P06 to P09 on the case board: the station's records and frames, the questions, the rules.
	TUniquePtr<FS47RuleBook> Book;
	// world.site (world/Station01.ts): made here, as main.ts's World makes it; the travel deps hand it on
	UPROPERTY() TObjectPtr<US47Station> Station;
	// main.ts's stationTaken (story/stationTaken.ts)
	UPROPERTY() TObjectPtr<US47StationTaken> StationTaken;

	// ---------- the truck ----------
	struct FTrip { FString Label; TFunction<void()> Use; };
	// This chapter's drive from wherever the truck stands: out to STATION 01, and back to SARO with
	// the field roll (main.ts asks it first, World.truckChoice).
	TOptional<FTrip> TruckTrip(const FString& Place);
	void BindSaroTruck(const FS47Obj& Object);
	// Called by main when the drive reaches the station gate (and when a save there is restored).
	void ArrivedStation();
	// Called by main when the drive back ends at SARO.
	void ArrivedSaro();

	// ---------- the station ----------
	// The station's interactables are added once, the first time (main.ts world.onStationLoaded).
	void BindSite(US47Station* Site);
	// Why the shutter would (not) fire right now. Chapter one asks this while it runs.
	FS47ShotCheck Check();

	// ---------- Travel (main.ts: the areas, the truck and the drive) ----------
	FString Area() const;       // world.area
	US47Station* Site() const;  // world.site (null when the export has no station)
	bool Driving() const;       // world.driving

	// ---------- the host (Prologue.ts) ----------
	void BeginChapter3(const S47J::FObj& SavedCase);  // Prologue.beginChapter3
	void JumpChapter3();                              // Prologue.jumpChapter3

	// The rules, the replies, the saved state and the drawings, without the world.
	static bool SelfTest(TArray<FString>& Lines);
	// -S47Ch3Drive=<stop>: plays the chapter at the station by itself up to a stop and holds there
	// (hut, transit, frame03, lamp, frame04, board, receiver, phone, bench, prints), as
	// tools/chapter3.py plays the web version. With -S47Drive the truck is driven there first.
	// (-S47Ch3Break with -S47Jump=chapter2: chapter two's end and its card into this chapter.)
	void TestDrive(const FString& Stop);

private:
	bool bBound = false;
	FString Busy;                       // 'tank' | 'fix' | '' (null)
	TFunction<void()> PanelRefresh;     // the null test's panel, while it is the last one opened
	struct FImages { FString Transit; } Images;
	double CallT = -1;
	bool bRadioOn = false;
	TArray<FString> TestNotes;          // notes when there is no host (the self test)

	// deps
	class US47UI* Ui() const;
	class US47Audio* Au() const;
	US47FieldCamera* Fcam() const;
	US47Chapter1* Ch1() const;
	void Toast(const FString& Text, float Secs = 3.2f) const;
	void Note(const FString& Text);
	void After(double Secs, TFunction<void()> Fn, const FString& Tag = FString());
	double Gt() const;
	FVector PlayerPos() const;
	void Save();
	void Milestone();
	void BoardRefresh();
	void BoardOpen();
	FString Time() const;
	bool File(const FS47Doc& Doc);
	FVector Pos(const FS47Obj& O) const { return O.WorldPosition(); }
	double Dist(const FVector& P) const;

	void BuildBook();
	TArray<FS47BoardCard> BoardCards();
	void ApplyWorld();

	// the station
	void Look(const FString& What);
	FString TransitImage();
	void UseTransit();
	void UseCable();
	void UseReceiver();
	void UseTimingLog();
	void OpenLamp();
	bool Near(const FString& Key, double R) const;
	FS47Result SubmitMarker(const FString& C);
	void SetLamp(bool bCover);
	FS47Result ObserveLamp();
	FS47Result Inspect();
	FS47Result SubmitCable(const FString& C);
	FS47Result SubmitTiming(const FString& C);

	// Nora on the field telephone
	void Ring();
	void UsePhone();

	// the field camera
	FString FrameNo() const;
	FVector Target(const FString& Which) const;
	FS47ShotCheck Check03();
	FS47ShotCheck Check04();
	void Expose();

	// the wet bench at SARO
	bool UseBench();
	void Collect();
	bool BandInLook(double U, double V) const;
	void Finish();

	// documents
	FS47Doc DocTransit();
	FS47Doc DocTiming() const;
	FS47Doc DocDiagram() const;
	FS47Doc DocFinding(const FString& K) const;
	FS47Doc DocPhoto(const FS47Photo& P) const;
};

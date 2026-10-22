#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"
#include "S47Chapter.h"
#include "S47Docs.h"
#include "S47Json.h"
#include "S47Obj.h"
#include "S47Panels.h"
#include "S47Chapter5.generated.h"

class FS47Diner;

// story/Chapter5.ts Ch5State: the saved state of chapter five, written and read with the
// TypeScript's own field names ({ v: 1, stage, bell, file, arrived, coffee, lights, clipped, aunt,
// radio, road, october, confirmed, ward, regular?, arrayTold?, tracing: [x, y] | null, p13, wrong13 }),
// merged over a fresh state as the TypeScript does ({ ...fresh(), ...saved }).
struct SIGNAL47_API FS47Ch5State
{
	FString Stage = TEXT("to-truck");   // Stage5: 'to-truck' | 'diner' | 'road' | 'complete'
	bool bBell = false, bFile = false;  // the terminal chimed; the 05:29 file was read
	bool bArrived = false;
	bool bCoffee = false, bLights = false, bClipped = false, bAunt = false, bRadio = false;
	bool bRoad = false, bOctober = false, bConfirmed = false;
	bool bWard = false;
	/** HENDELSER.md, event 11: the cup poured for the regular nobody sees (added 17 October; older saves
	 *  do not have it, and that is no; fresh() has it, so a begun chapter always does). */
	bool bRegular = false;
	/** Ward has the count of the array's tries (story/arrayBack.ts; older saves do not have it). */
	TOptional<bool> ArrayTold;
	TOptional<FVector2D> Tracing;       // where the tracing lies on the map (null: not laid out yet)
	bool bP13 = false; int32 Wrong13 = 0;
	S47J::FObj Extra;                   // a saved state as it came, so fields this version does not know stay in it

	S47J::FObj Save() const;
	static FS47Ch5State Load(const S47J::FObj& Saved);
};

// story/Chapter5.ts, chapter five, "All Night" (KAPITLER.md). 05:00 to 05:20. Out of room 6 to the
// truck, on the motel's lot or back up the highway on SARO's pad; past the control room the operations
// terminal chimes: a new file, opened at 05:29, half an hour from now. Then the diner where the old
// road to Roswell leaves the highway: coffee, a waitress who has watched the lights over the mesa since
// she was eleven, a driver whose engine died on the old road last October, the 1947 clipping, the
// radio, and a payphone to Ward. At a booth the service map and E09A on tracing paper show where C
// crosses the old road (P13). The driver confirms it without knowing he does. The chapter ends at the
// truck; chapter six takes the road.
//
// The host side (Prologue.ts beginChapter5 and jumpChapter5, main.ts's allNight deps, ch5.onEnd,
// driverMore, arrayCount, world.onDinerLoaded and onDinerTruck) is here too, as BeginChapter5,
// JumpChapter5 and Register.
UCLASS()
class SIGNAL47_API US47Chapter5 : public US47Chapter
{
	GENERATED_BODY()

public:
	// the constructor's work (the rule book) and main.ts's for chapter five: the truck's choice, the
	// diner's things (world.onDinerLoaded = ch5.bindDiner) and the truck on its lot (onDinerTruck),
	// driverMore, arrayCount, Host->Jumps["chapter5"], OnEnd. S47Chapters.cpp: Make<US47Chapter5>(Host, M)->Register()
	US47Chapter5* Register();
	virtual void BeginDestroy() override;

	// ---------- lifecycle ----------
	virtual void Begin(const S47J::FObj& Saved) override;
	virtual void Reset() override;
	virtual S47J::FObj SaveState() const override; // this.s once started (Prologue.composeCase: ch5), else null
	virtual void Update(double Dt, double T) override;
	virtual FString Objective() const override;
	virtual TArray<FS47Task> Tasks() const override;
	virtual FString SaveBlock() const override { return FString(); } // saveBlock(): null
	virtual FString Key() const override { return TEXT("ch5"); }

	// (Chapter5.active is US47Chapter::bActive: begun, until reset or chapter six begins)
	bool bStarted = false;
	FS47Ch5State S;
	TFunction<void()> OnEnd;          // main.ts ch5.onEnd (Register sets it)
	TFunction<void()> OnSave;         // Chapter5Deps.save: the host composes the case and main.ts autosaves
	/** More for the driver at the counter (story/lostNights.ts: his watch, and the tow ticket from
	 *  Caliche), set by main.ts; without it he is as he was. (Register sets it to M->Away->LostNights.) */
	struct FDriverMore
	{
		TFunction<FString()> Examine;                                            // '' null
		TFunction<TArray<S47Panels::FTalkLine>()> Open;
		TFunction<TArray<S47Panels::FTalkChoice>(bool bOctober)> Choices;
		TFunction<TOptional<TArray<S47Panels::FTalkLine>>(const FString& Id)> Choose;
	};
	TOptional<FDriverMore> DriverMore;
	/** How many times tonight the array was put back on the schedule and went back by itself
	 *  (story/arrayBack.ts, set by main.ts): Ward has the count. Unset: null. */
	TFunction<int32()> ArrayCount;
	/** P13 on the case board. It is found with the maps on a table in the diner, not with threads; the
	 *  board shows it, tied to the map and to C on the transit record. */
	TUniquePtr<FS47RuleBook> Book;

	// ---------- SARO: the terminal and the truck ----------
	/** The operations terminal ran a command (story/Prologue.ts openOps; AS47Story::OpsRan). */
	void TerminalCommand(const FString& Cmd);
	/** This chapter's drive from wherever the truck stands: to the diner, and from its lot out on the
	 *  old road once C is found (main.ts asks it, World.truckChoice). */
	struct FTrip { FString Label; TFunction<void()> Use; };
	TOptional<FTrip> TruckTrip(const FString& Place);
	/** Called when the truck is left on the diner's lot (and when a save there is restored). */
	void ArrivedDiner();

	// ---------- the diner's things ----------
	/** Every hit area in the diner has a label whenever the player is there (world.onDinerLoaded). */
	void BindDiner(FS47Diner& Diner);
	/** The truck on the diner's lot (World gives it a hit area: world.onDinerTruck). */
	void BindDinerTruck(const FS47Obj& Object);
	FS47Result Answer(const FString& Id);

	// ---------- the host (Prologue.ts) ----------
	void BeginChapter5(const S47J::FObj& SavedCase);  // Prologue.beginChapter5
	void JumpChapter5();                              // Prologue.jumpChapter5

	// The talks, the map's answers, the saved state and the drawings' numbers, without the world.
	static bool SelfTest(TArray<FString>& Out);
	// -S47Ch5Drive=<stop>: plays the chapter in the diner by itself up to a stop and holds there for the
	// screenshot (counter, waitress, clipping, map, aligned, p13, truck; road: the truck used, onto the
	// old road), as tools/chapter5.py plays it.
	void TestDrive(const FString& Stop);

private:
	TArray<S47Panels::FTalkLine> Lines;
	FString Who;                      // 'waitress' | 'driver' | 'ward' | '' (null)
	TOptional<S47Panels::FTalkHandle> Panel;
	bool bDinerBound = false;
	FString ClippingUrl, MapUrl;      // '' null
	bool bLeaving = false;
	bool bRadioBusy = false;
	TArray<FString> NoLinks;          // the book's links: () => [] (a fresh empty list each time)
	TArray<FString> TestNotes, TestDocs; // without a host (the self test)

	void BuildBook();
	void ApplyWorld();
	void Save();
	FString Time() const;
	bool InDiner() const;
	FString TruckWhere() const;
	bool File(const FS47Doc& Doc);
	void Note(const FString& Text);
	void Toast(const FString& Text, float Secs) const;
	void Milestone() const;
	FString Area() const;
	bool Driving() const;
	FString TruckAt() const;
	FS47Diner* Diner() const;

	FS47Obj RegularCup() const;
	void UseCoffee();
	void Pour();
	void UseRadio();
	void ReadClipping();
	void UsePayphone();

	void OpenTalk(const FString& InWho);
	S47Panels::FTalkView View() const;
	void Choose(const FString& Id);

	void OpenMap();
	bool Aligned() const;
	void Finish();

	FS47Doc DocMap();
	FS47Doc DocClipping();
	FS47Doc DocFinding() const;
};

#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"

// Saved games, ported from web/src/core/saves.ts (the save contract in the design bible, part 13)
// and web/src/story/state.ts (what a save holds).
//
// Three cases. Each case keeps three autosaves that take turns and three manual saves. A save is
// the game state as JSON plus a little card for the menus (chapter, place, clock, play time, a
// small picture). Photographs are kept once, in a store of their own under a key made from their
// content, and saves point at them with 'idb:<key>', so an autosave never copies a 200 kB print.
//
// Where the web version has IndexedDB (and localStorage as a fallback), this keeps files under
// Saved/SaveGames: save-<case>-<kind>-<slot>.json (the record: the card's fields and 'state', the
// GameState exactly as the web version writes it), photos/<key>.txt (one photograph's data URL)
// and active.json (s47.activeCase). The references keep the web version's 'idb:' prefix, so a save
// written by either reads the same. The web version's move of its very first save format
// (migrateOldSave) has nothing to move here and is left out.
namespace S47Saves
{
	constexpr int32 CASES = 3, AUTO = 3, MANUAL = 3;
	SIGNAL47_API FString SaveId(int32 CaseId, const FString& Kind, int32 Slot); // `${caseId}:${kind}:${slot}`
	SIGNAL47_API FString PhotoKey(const FString& Url);                          // 'p' + cyrb53(url) in base 36
	SIGNAL47_API double NowMs();                                                // Date.now()
	// Checks the store against the web version's rules (tools/saves.cjs) in a scratch folder.
	SIGNAL47_API bool SelfTest(TArray<FString>& Out);
}

// SaveMeta: what the menus show of a save
struct SIGNAL47_API FS47SaveMeta
{
	FString Id;               // `${caseId}:${kind}:${slot}`
	int32 CaseId = 0;         // 1..CASES
	FString Kind;             // 'auto' | 'manual'
	int32 Slot = 0;           // 0..AUTO-1 or 0..MANUAL-1
	double SavedAt = 0;       // real time, ms
	double Playtime = 0;      // seconds of active play in this case
	FString Chapter;          // "Chapter 2: The Amended Record"
	FString Place;            // "Records room"
	FString Clock;            // the night's clock, "03:12"
	FString Thumb;            // a JPEG data URL; empty: null
	TArray<FString> Photos;   // photograph keys this save points at
	int32 Version = 2;

	void Write(const S47J::FObj& O) const;
	static FS47SaveMeta Read(const S47J::FObj& O);
};

// SaveCard: the card's part the game hands in with a save
struct FS47SaveCard
{
	FString Chapter, Place, Clock;
	double Playtime = 0;
	FString Thumb;
};

// story/state.ts: the chapter titles of the checkpoints the first web version knew, and the names
// of a GameState's fields (v 2):
//   v, checkpoint ('residual' | 'chapter1' ...), case (SavedCase | null), area, pose {x, z, yaw, pitch} | null,
//   doors {id: open}, flashlight, truck, truckPose, abduct, town, album, motelTv, seen, curve, odo
namespace S47State
{
	SIGNAL47_API FString CheckpointTitle(const FString& Checkpoint); // CHAPTERS[checkpoint], or empty
}

// SaveStore: the saves of all three cases and their photographs.
class SIGNAL47_API FS47SaveStore
{
public:
	explicit FS47SaveStore(const FString& InDir = FString());

	// the web version opens its store while the title is up (ready); here the files are read at once
	void Init();
	bool bFallback = false; // true when the photo store could not be used (the menus say so)
	FString Dir;

	// ---------- reading ----------
	TArray<FS47SaveMeta> List(int32 CaseId = 0) const;            // newest first; 0: every case
	TOptional<FS47SaveMeta> Latest(int32 CaseId = 0) const;
	TArray<int32> UsedCases() const;
	int32 FreeCase() const;                                       // 0 when every case is taken
	int32 Active() const;                                         // the case played last, 0 for none
	void SetActive(int32 CaseId);
	// the save Continue opens: the newest in the case played last, else the newest of all
	TOptional<FS47SaveMeta> ContinueSave() const;
	S47J::FObj Load(const FString& Id);                           // the GameState, or null

	// ---------- writing ----------
	// Write a save. Slot -1 ('rotate') picks the autosave to replace. Returns the new card when it
	// is safely stored, or unset with the reason in OutError (what the web version throws).
	TOptional<FS47SaveMeta> Save(int32 CaseId, const FString& Kind, int32 Slot, const S47J::FObj& State, const FS47SaveCard& Card, FString& OutError);
	bool Remove(const FString& Id, FString& OutError);
	bool RemoveCase(int32 CaseId, FString& OutError);
	// The next autosave of this case starts a new generation (a chapter or an area begins).
	void NewGeneration(int32 CaseId) { Generation.Add(CaseId); }

	// what the tests change: the clock (Date.now) and the stand-in print
	TFunction<double()> Now;
	TFunction<FString()> MissingPrint;
	// tests: the next N photograph reads fail (a failed transaction)
	int32 FailReads = 0;

private:
	TMap<FString, FS47SaveMeta> Metas;
	TSet<FString> Stored;                                  // photograph keys known to be in the store
	struct FUnread { FString Shown, Ref; };
	TMap<FString, FUnread> Unread;                         // photo id -> stand-in shown, reference kept
	TSet<int32> Generation;

	int32 NextAuto(int32 CaseId);
	TOptional<FS47SaveMeta> WriteRecord(int32 CaseId, const FString& Kind, int32 Slot, const S47J::FObj& State, const FS47SaveCard& Card, FString& OutError);
	void Sweep();
	FString SavePath(const FString& Id) const;
	FString PhotoPath(const FString& Key) const;
	FString ActivePath() const;
	TOptional<FString> ReadPhoto(const FString& Key);
	bool WriteFile(const FString& Path, const FString& Text) const;
};

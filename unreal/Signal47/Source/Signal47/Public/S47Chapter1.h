#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"
#include "S47Chapter.h"
#include "S47Docs.h"
#include "S47FieldCamera.h"
#include "S47Chapter1.generated.h"

class US47Yard;
class US47UI;
class US47Audio;
class UTexture2D;
struct FS47Viewfinder;

// story/Chapter1.ts CaseState: the saved case of chapter one, field for field (SaveState writes
// the same JSON the web version's serialize() does: v, stage, eventClock, camera, door, labDoor,
// log, returned, f1?, f2?, dev1, dev2, mark1?, ref, hyp, method, observed, mark2?, concluded,
// filed, wrongRef, wrongHyp, wrongConcl, rejected, sheet?, links?).
struct SIGNAL47_API FS47CaseState
{
	int32 V = 1;
	// Opt-in SC06 route. Missing field is the unchanged legacy case.
	int32 SequenceVersion = 0;
	TArray<FS47Doc> Sources;
	FString Stage = TEXT("collect-camera"); // collect-camera | motor-log | first-exposure | develop-first | interpret-first | choose-control | observe-control | second-exposure | develop-second | compare-exposures | file-report | complete
	double EventClock = 0;
	bool bCamera = false, bDoor = false, bLabDoor = false, bLog = false, bReturned = false;
	TOptional<FS47Photo> F1, F2;
	int32 Dev1 = 0, Dev2 = 0;          // 0 sealed, 2 print in fixer, 3 collected
	// (Unreal only, on Tom's request: the darkroom wants its door shut) prints of frame 01 and frame 02
	// fogged by light through the lab door; the negative is kept, so the next print comes from it.
	// Saved as fog1/fog2 when there were any.
	int32 Fog1 = 0, Fog2 = 0;
	TOptional<FVector2D> Mark1;
	bool bRef = false;
	FString Hyp;                       // 'light' | 'drift' | '' (null)
	FString Method;                    // 'passive' | 'active' | '' (null)
	bool bObserved = false;
	TOptional<FVector2D> Mark2;
	bool bConcluded = false, bFiled = false;
	int32 WrongRef = 0, WrongHyp = 0, WrongConcl = 0, Rejected = 0;
	TOptional<bool> Sheet;             // the installation sheet has been read at the archive
	TOptional<TArray<FString>> Links;  // chapter one's threads, as sorted pin pairs

	static FS47CaseState Fresh(double EventClock);
	TSharedPtr<FJsonObject> ToJson() const;
	// { ...fresh(eventClock), ...saved }
	static FS47CaseState FromJson(const TSharedPtr<FJsonObject>& Saved, double EventClock);
};

// Later chapters take over the camera and the wet bench while they run (chapter three's field
// frames): the viewfinder, the shutter rules and the bench go to them. bNear: the player is at a
// case frame's subject, so a miss there is a refused case frame and never a free one.
struct FS47ShotCheck { FString Text; bool bOk = false; bool bNear = false; };
struct FS47FieldShots
{
	TFunction<FString()> FrameNo;
	TFunction<FS47ShotCheck()> Check;
	TFunction<void()> Expose;
	TFunction<void()> Rejected;
};
// The free frames (core/album.ts): anything, anywhere, once the camera is had.
struct FS47FreeShots { TFunction<int32()> Left; TFunction<void()> Expose; };
// S-03's local controller log since 02:15 (story/arrayBack.ts)
struct FS47S03Since { int32 Cmds = 0; FString Text; };

// Chapter one, "The Second Exposure", from web/src/story/Chapter1.ts. Ported from the Unity
// chapter (Chapter09 and Visual10 passes): S-03 motor log, field camera, photo lab, B-12 control
// test with a passive or active method, two exposures and a bounded local report.
// Web adaptations: the array lies north of the yard, the photo lab sits east of the walk, the
// three-bar marker is called R-07 (S-07 is an antenna here), and the motor bus header reads SARO
// ARRAY.
UCLASS()
class SIGNAL47_API US47Chapter1 : public US47Chapter
{
	GENERATED_BODY()

public:
	// What the TypeScript constructor does (the interactables and the rule book), with what
	// main.ts does for chapter one (the field camera, the yard, the keys) and Prologue.ts's
	// jump('chapter1'). Called once from S47RegisterChapters.
	US47Chapter1* Register();
	virtual void BeginDestroy() override;

	// ---------- US47Chapter ----------
	// Prologue.beginChapter1(saved): Saved is the saved case (story/state.ts SavedCase: s, notes,
	// clock ...), or null for a new chapter; a bare CaseState is taken as well.
	virtual void Begin(const S47J::FObj& Saved) override;
	virtual void Reset() override;
	virtual S47J::FObj SaveState() const override;
	virtual FString SaveBlock() const override;
	virtual void Update(double Dt, double T) override;
	virtual FString Objective() const override;
	virtual TArray<FS47Task> Tasks() const override;
	virtual void DoorChanged(const FString& Id, bool bOpen) override;
	virtual FString Key() const override { return TEXT("ch1"); }

	// Chapter1.begin(saved, eventClock)
	void BeginCase(const S47J::FObj& Saved, double InEventClock);

	// Chapter1.active (US47Chapter::bActive is the host's: which chapter leads; chapter one stays
	// active, its yard and lab with it, through the chapters after it)
	bool Active = false;
	bool Started() const { return Active; }
	FS47CaseState S;
	TFunction<void(const FString& Method)> OnEnd;
	TOptional<FS47FieldShots> Field;            // set by a later chapter while it runs
	TOptional<FS47FreeShots> Free;              // the album's free frames (main.ts)
	TFunction<bool()> Lab;                      // a later chapter's use of the wet bench; true when handled
	TFunction<TOptional<FS47S03Since>()> S03Since;
	TFunction<bool()> Dark;                     // SARO has no power (story/saroDark.ts)
	TFunction<void()> SaveHook;                 // Chapter1Deps.save: the host composes the case and main.ts autosaves

	// ---------- the camera (main.ts input: C, Space, E) ----------
	void ToggleCamera();
	void Shutter();
	void RestoreEarlyCamera();
	bool EarlyCamera() const;

	// For chapter three's field roll: the same tray, the same fade-in, the same drying line.
	void FixPrint(const FS47Photo& Photo);
	void ClearWetPrint();
	void HangPrint(int32 I, const FS47Photo& Photo);
	// S-03's log has more in it (story/arrayBack.ts): the notebook's copy as the cabinet has it now.
	void RefileS03();

	FS47RuleBook* Book() const { return BookPtr.Get(); }
	US47Yard* GetYard() const { return Yard; }
	US47FieldCamera* Fcam() const;

	static bool SelfTest(TArray<FString>& Lines);
	// -S47Ch1Drive=<stop>: plays the chapter's camera and lab by itself up to a stop and holds there
	// for the screenshot (viewfinder, photo, bench, wet, lab, print, reference, b12, report, compare, board), as tools/chapter1.py drives the web version.
	// -S47Ch1Dump=<dir> also writes the motor bus screen, the drawings and frame 01's print there.
	void TestDrive(const FString& Stop);

private:
	friend class FS47SC06Contract;
	UPROPERTY() TObjectPtr<US47Yard> Yard;
	UPROPERTY() TMap<FString, TObjectPtr<UTexture2D>> Images;
	TUniquePtr<FS47RuleBook> BookPtr;
	struct FVaneAnim { double From = 0, To = 0, T0 = 0; };
	TOptional<FVaneAnim> AnimVane;
	TOptional<double> AnimFix;
	FString Busy;            // a timed step at the wet bench: 'tank' | 'fix' | ''
	bool bSettled = false;
	FS47ShotCheck Status;
	double BusT = 0;
	bool bEastOpen = false, bLabOpen = false; // the doors as last seen (Doors.onChange)
	// (Unreal only) the darkroom: a step's generation (a fogged print stops its timers), which frame is
	// in the process, the lab's lamps as they were before the safelight took over
	int32 DevGen = 0;
	bool bDarkroom = false;
	bool bPrintingLight = false;
	TMap<int32, float> LampBase;
	TArray<FString> TestNotes; // notes when there is no host (the self test)

	// deps
	US47UI* Ui() const;
	US47Audio* Au() const;
	bool Touch() const { return false; } // isTouch(): no touch input in this version
	void Toast(const FString& T, float Secs = 3.2f) const;
	void NoteIt(const FString& T);
	void After(double Secs, TFunction<void()> Fn, const FString& Tag = FString());
	double Gt() const;
	bool InDark(const FString& Text) const;

	void BuildBook();
	void Interactables();
	void ApplyWorld();
	void KeepSources();
	void ShowCamera(bool bOn);
	void Save();
	bool File(const FS47Doc& Doc);
	void StageTo(const FString& St);

	// wet bench
	struct FDevFrame { int32 Which = 1; FS47Photo Photo; };
	TOptional<FDevFrame> DevFrame() const;
	void UseWetBench();
	FString BenchLabel() const;
	// (Unreal only) a print is in the process, from the tank until it is fixed: light ruins it
	bool Developing() const;
	// (Unreal only) the lab door opened on a print in the process: the print fogs, the negative stays
	void Fog();
	// (Unreal only) the lab's white lamps out and the safelights up while a print is in the process
	void Darkroom(bool bOn);
	UTexture2D* FoggedTex(const FS47Photo& Photo);
	void SetDev(int32 Which, int32 V);
	void Collect(int32 Which, const FS47Photo& Photo);
	UTexture2D* TexOf(const FS47Photo& Photo);
	void SetMap(const FS47Obj& Mesh, UTexture2D* Tex, double Color);
	void ShowWetPrint(const FS47Photo& Photo);

	// the case board
	TArray<FS47BoardCard> BoardCards() const;

	// prints and the archive bench
	void OpenPrint1();
	bool Near(const FS47Photo& Photo, double U, double V, double R) const;
	void UseArchive();
	void OpenReference();
	void OpenCompare();
	FS47Result MatchReference(const FString& C);
	FS47Result ChooseHypothesis(const FString& C);
	FS47Result Conclude(const FString& C);
	FString LampReply() const;

	// B-12
	void UseB12();

	// camera
	FString FrameNo() const;
	FS47ShotCheck Check() const;
	FVector S03Centre() const;
	void FreeLine(FS47Viewfinder& V) const;

	// report and ending
	FString ReportText() const;
	void FileReport();

	// documents
	FS47Doc DocS03() const;
	FS47Doc DocSheet() const;
	FS47Doc DocPhoto(const FS47Photo& P) const;
	FString ObservationText() const;
	FS47Doc DocObservation() const;
	FS47Doc DocFinding() const;

public:
	// world/Dish.ts DishArray.feedPosition for an antenna at (x, z) of that scale, pointing so
	static FVector FeedPosition(double X, double Z, double Scale, double Az, double El);
};

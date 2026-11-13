#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"

// EVIDENCE-DOSSIER-20261113 (-> M3, M4; Tom 13 November), slice 1. See Docs/Design/Freeform/EVIDENCE_SPECIES_20261113.md.
// What the player logs out on the land gathers into one file for each unknown thing, so that over the month they see for themselves
// that there is more than one, and what tells them apart. Pure: no world, no session, no Slate. The session (S47FreeformDossier.cpp)
// puts a tag on a find where the game already knows which one it was; the notebook's THEM page and the records binder's UNKNOWNS
// divider draw Files().
//
// The game never says what any of them is. A file is UNKNOWN 1 to 5, numbered in the order the player first logged them, so the
// number says nothing of which it is; the ids below ("roswell", "pale", ...) are the code's working names and are never shown.
// Tom's decisions of 13 November: the road's crawler is the low one and the road's tall figure the tall one (one file each); the
// silver man and the one that shines have files that stop at MARKS and can never be filed complete; the shade never has a file.
//
// Six kinds of evidence, one box each: SHAPE (a photograph, or seen on a site visit), TAPE, GROUND, LAMP, GEIGER, ANIMALS. A file's stage:
//   OUTLINE  one kind of evidence
//   MARKS    three kinds, from at least two nights
//   HABITS   four kinds with LAMP among them, from at least three nights (never the two that stop at MARKS)
// Saved as "dossier": { "order": ["pale","roswell"], "tags": { "<finding key>": { "k": "pale", "t": "ground", "d": 9 } } }.
// LOOP-DOSSIER-GIVES-20261113: a tag may also have "h" (the hour) and "x", "z" (where); and the dossier "extra": { "pale": 9 } (the night
// its MARKS bearing is for) and "spent": ["pale"]. All optional: a save without them reads as before (no hours, no map mark, no extra).
namespace S47Dossier
{
 enum class EKind : int32 { Small=0, Low, Tall, Silver, Shining, Count, None=-1 };
 enum class EType : int32 { Shape=0, Tape, Ground, Lamp, Geiger, Animals, Count, None=-1 };
 enum class EStage : int32 { NoFile=0, Outline, Marks, Habits };
 // A find held against a file's box: the file has nothing in that box yet (Unknown), what it has is the same (Fits), or it differs.
 enum class EFit : int32 { Unknown=0, Fits, Contradicts };

 // The save's ids, and back (None for anything else).
 SIGNAL47_API const TCHAR* KindId(EKind Kind);
 SIGNAL47_API const TCHAR* TypeId(EType Type);
 SIGNAL47_API EKind KindOfId(const FString& Id);
 SIGNAL47_API EType TypeOfId(const FString& Id);
 // Which one a source already knows it was. A photograph's group (FS47FreeformDirector's "strange.<group>.n<night>": roswell, pale
 // and crawler, tall and tallfigure, nordic, cosmic; None for the shade, the shadow and everything that is not one of them); a site
 // visit's VisitWho (0, 1, 2).
 SIGNAL47_API EKind KindOfGroup(const FString& Group);
 SIGNAL47_API EKind KindOfVisit(int32 VisitWho);
 // A photograph claim's id into its kind and night; false when it is not one of the five.
 SIGNAL47_API bool ClaimKind(const FString& ClaimId,EKind& OutKind,int32& OutNight);
 SIGNAL47_API bool PhotoKey(const FString& TagKey); // the tag is a photograph (its key is the claim's id)

 // The table: what each kind leaves in each box, as an internal word (empty: that box gives nothing for this one), and the
 // player's own pencil for it. No two kinds have the same row; the three that come near differ in every box.
 SIGNAL47_API const TCHAR* Signature(EKind Kind,EType Type);
 SIGNAL47_API const TCHAR* Note(EKind Kind,EType Type);
 SIGNAL47_API const TCHAR* TypeLabel(EType Type);   // "SHAPE"
 SIGNAL47_API const TCHAR* StageWord(EStage Stage); // "OUTLINE"; empty for no file
 SIGNAL47_API bool StopsAtMarks(EKind Kind);        // the silver man and the one that shines
 SIGNAL47_API EStage StageFor(EKind Kind,int32 Types,int32 Nights,bool bLamp);
 // How a file is kept from here, in the player's pencil: what the next stage takes. Never what the file holds.
 SIGNAL47_API FString NextWords(EKind Kind,EStage Stage);
 // The stamp when evidence is logged: the file's number and the kind of evidence, never which one it is.
 SIGNAL47_API FString Stamp(int32 Number,EType Type,bool bPhoto,bool bNewFile,EStage Reached);

 // LOOP-DOSSIER-GIVES-20261113 (-> M3, M4; Tom 13 November, after the critics: "a thing is done when the player notices it and it has a
 // consequence"). What reaching each stage gives, with what the game has already; never what the file holds, never what it is:
 //   OUTLINE  where the file was first logged goes on the field map in pencil, its title beside it (FS47Dossier::FirstWhere)
 //   MARKS    the array may point out once more than the night allows: one more bearing slip, once, by the slip's own target order,
 //            never on the big night (FS47Places::AfterRegistered; FS47Dossier::Grant, ExtraOpen, Spend)
 //   HABITS   tonight's note carries the hours the file has been logged at so far (HoursLine)
 // Gave: the one line under the stage on THEM and on the binder's sheet. bMapped: it is on the map; Extra: 0 none kept for it (a file at
 // MARKS from a save that had no record of it), 1 still to come, 2 printed; bHours: some hours are kept for it.
 SIGNAL47_API FString Gave(EKind Kind,EStage Stage,bool bMapped,int32 Extra,bool bHours);
 // The extra slip's own words on the printer and its toast, and tonight's note while it is still to come. Never which file gave it.
 SIGNAL47_API FString ExtraSlipWords();
 SIGNAL47_API FString ExtraToast();
 SIGNAL47_API FString ExtraTaskLine();
 // "UNKNOWN 2 / Logged so far at 23:00, 01:00, 03:00." Hours 0 to 23 in the night's order (from 08:00), each once; empty for none.
 SIGNAL47_API FString HoursLine(int32 Number,const TArray<int32>& Hours);
 // The night a stage reached at Clock gives its extra to (FS47FreeformSession::NightOf's nights): that night, or from five in the
 // morning the next one, so that a file brought to MARKS at dawn or in daylight gives it to the coming night.
 SIGNAL47_API int32 ExtraNightFor(double Clock);

 // ---------- a file as the notebook and the binder show it ----------
 struct FBox
 {
  FString Label,How,Note; // How: "on film", "seen", "noted"; Note: the pencil for it. Both empty while the box is.
  int32 Day=0;            // the first night it was logged
  bool bFilled=false;
 };
 struct FFile
 {
  int32 Number=0,Nights=0,Opened=0,Latest=0; // Opened, Latest: the first and the last night something went in
  FString Title,Stage,Next;                  // "UNKNOWN 2", "MARKS", how it is kept from here
  bool bAtMarks=false;
  FString Gave;                              // LOOP-DOSSIER-GIVES-20261113: what reaching the stage gave, one line
  TArray<FBox> Boxes;                        // six, in the table's order
  TArray<FString> Photos;                    // the photograph claims in it (their ids, for the session to name the prints by)
 };
 // The file typed for the binder: the stage, the six boxes, the photographs as the session names them, the pencil notes.
 SIGNAL47_API FString Sheet(const FFile& File,const TArray<FString>& PhotoLines);
 // Every word the dossier can show, and what is wrong with any of it (empty: nothing): Freeform's spoiler limits
 // (S47Script::LintText), no word for what they are (S47Marginalia::NamesKind), none of Freeform's banned words.
 SIGNAL47_API TArray<FString> Words();
 SIGNAL47_API FString Lint();
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}

class SIGNAL47_API FS47Dossier
{
public:
 // LOOP-DOSSIER-GIVES-20261113: Hour, the whole hour of the clock it was logged at (0 to 23; -1: not kept, as in a save from before),
 // and where the player stood (road-local x east, z south; bWhere false: not kept, or logged at a bench and not where it was got).
 struct FTag { FString Key; S47Dossier::EKind Kind=S47Dossier::EKind::None; S47Dossier::EType Type=S47Dossier::EType::None; int32 Day=1; int32 Hour=-1; bool bWhere=false; double X=0,Z=0; };
 // What an older save kept, for Rebuild: a finding's key, its text and its day.
 struct FOld { FString Key,Text; int32 Day=0; };

 void Reset(){Sequence.Reset();Kept.Reset();Given.Reset();Spent.Reset();}
 bool IsEmpty() const{return Kept.IsEmpty();}
 // A tag on a find. False when the key has one already, when the kind has no file (the shade), or when that box gives nothing for
 // this one. The first tag of a kind opens its file and gives it the next number.
 bool Tag(const FString& Key,S47Dossier::EKind Kind,S47Dossier::EType Type,int32 Day,int32 Hour=-1,const FVector2D* Where=nullptr);
 bool HasTag(const FString& Key) const;
 const TArray<S47Dossier::EKind>& Order() const{return Sequence;}
 const TArray<FTag>& Tags() const{return Kept;}
 int32 NumberOf(S47Dossier::EKind Kind) const; // 1 to 5; 0: no file
 bool Has(S47Dossier::EKind Kind,S47Dossier::EType Type) const;
 int32 TypesOf(S47Dossier::EKind Kind) const;
 int32 NightsOf(S47Dossier::EKind Kind) const;
 S47Dossier::EStage StageOf(S47Dossier::EKind Kind) const;
 int32 AtMarks() const; // the files at MARKS or further: Dr. Ward's line
 // A find of FindKind's held against File's box of that type, from the player's own tags only: a box the player has not filled
 // answers Unknown whatever the table says.
 S47Dossier::EFit Fit(S47Dossier::EKind File,S47Dossier::EType Type,S47Dossier::EKind FindKind) const;
 TArray<S47Dossier::FFile> Files() const; // in the order of their numbers
 // LOOP-DOSSIER-GIVES-20261113. Where the file was first logged out where it was got: the earliest tag (by day, then as logged) that
 // kept a place. The hours it has been logged at, each once, in the night's order.
 bool FirstWhere(S47Dossier::EKind Kind,double& OutX,double& OutZ) const;
 TArray<int32> HoursOf(S47Dossier::EKind Kind) const;
 // MARKS's extra bearing: given once to a file, for a night (Grant: false when it has had one); open on that night and any after until
 // it is printed (Spend, once), so a night without a discrepancy to register, or the big night, does not lose it. ExtraState: 0 none,
 // 1 open, 2 printed. GrantMissing: a file at MARKS or past it with no extra kept (a save from before) gets one from Night.
 bool Grant(S47Dossier::EKind Kind,int32 Night);
 bool ExtraOpen(int32 Night) const;
 bool Spend(int32 Night);
 int32 ExtraState(S47Dossier::EKind Kind) const;
 int32 GrantMissing(int32 Night);

 S47J::FObj Save() const;
 // An object that is not a dossier, or a tag that is not sound, is left out; a tag whose kind has no number gets the next one.
 void Load(const S47J::FObj& Saved);
 // A save from before the dossier: tags from what it kept, where the key or the record's own words say which one it was. The
 // photograph claims ("strange.<group>.n<night>", and their receipts among the findings), "rule-still-held", the prints on the
 // shoulder ("road_tracks_d<night>", by the record's words). "rule-pale-broken" and a site visit's page do not say which one it
 // was and are left alone. In order of day, then key, so the numbers come out the same every time. Returns the tags made.
 int32 Rebuild(const TArray<FOld>& Findings,const TArray<FString>& Claims);

private:
 TArray<S47Dossier::EKind> Sequence;
 TArray<FTag> Kept;
 TMap<int32,int32> Given; // by the kind's number: the night each file's extra bearing is for
 TSet<int32> Spent;       // and the ones printed
};

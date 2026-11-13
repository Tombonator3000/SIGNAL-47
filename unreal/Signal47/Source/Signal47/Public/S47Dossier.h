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
// Six kinds of evidence, one box each: SHAPE (a photograph, or seen close), TAPE, GROUND, LAMP, GEIGER, ANIMALS. A file's stage:
//   OUTLINE  one kind of evidence
//   MARKS    three kinds, from at least two nights
//   HABITS   four kinds with LAMP among them, from at least three nights (never the two that stop at MARKS)
// Saved as "dossier": { "order": ["pale","roswell"], "tags": { "<finding key>": { "k": "pale", "t": "ground", "d": 9 } } }.
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

 // ---------- a file as the notebook and the binder show it ----------
 struct FBox
 {
  FString Label,How,Note; // How: "on film", "seen close", "noted"; Note: the pencil for it. Both empty while the box is.
  int32 Day=0;            // the first night it was logged
  bool bFilled=false;
 };
 struct FFile
 {
  int32 Number=0,Nights=0,Opened=0,Latest=0; // Opened, Latest: the first and the last night something went in
  FString Title,Stage,Next;                  // "UNKNOWN 2", "MARKS", how it is kept from here
  bool bAtMarks=false;
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
 struct FTag { FString Key; S47Dossier::EKind Kind=S47Dossier::EKind::None; S47Dossier::EType Type=S47Dossier::EType::None; int32 Day=1; };
 // What an older save kept, for Rebuild: a finding's key, its text and its day.
 struct FOld { FString Key,Text; int32 Day=0; };

 void Reset(){Sequence.Reset();Kept.Reset();}
 bool IsEmpty() const{return Kept.IsEmpty();}
 // A tag on a find. False when the key has one already, when the kind has no file (the shade), or when that box gives nothing for
 // this one. The first tag of a kind opens its file and gives it the next number.
 bool Tag(const FString& Key,S47Dossier::EKind Kind,S47Dossier::EType Type,int32 Day);
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
};

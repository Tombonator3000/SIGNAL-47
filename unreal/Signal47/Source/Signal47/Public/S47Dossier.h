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
//
// EVIDENCE-DOSSIER-20261113 slice 2 (14 November). A file is no longer one per kind: it is what the player could tie together. A find
// goes into a file that already has the same thing in the same box (the same notes on a tape, the same prints), or that has something
// logged the same night where it was got (within NearM; a find kept with no place, of the same kind, the same night); the lowest such
// file. None: it opens a file of its own, also when the game knows another file is the same thing (a tape of something never
// photographed). When one find ties two files at once, the game does not merge them: it keeps them side by side ("same") and THEM
// shows where their boxes agree. A find that ties by place and night into a file whose box holds something else strikes the old pencil
// there through. The stage is the file's own; HABITS waits for the first week of the rhythm's curve that has danger in it (HabitsFrom,
// FS47RhythmData's weeks; 1 in a pure test). More saved, all optional: "files": the kind that opened each file, in number order (only
// once a kind has more than one); a tag's "f": its file's number (only when it is not the first file of its own kind); "same": the
// pairs kept side by side, flat ([1,3, 2,5]). A save from slice 1 has none of them and reads as it was.
// REVIEW (14 November): by place and night alone a find ties only to a file opened by one of the same reach (the far two, or the
// near three); a file's stage never drops once reached ("reached": by file, saved only where its tags alone would say less).
//
// EVIDENCE-DOSSIER-20261113 slice 3 (14 November). The player's own working name for a file (prefilled with a plain suggestion from
// what it holds; written at the binder in the records room), shown beside "UNKNOWN n" wherever the file is named. Two files filed as
// one at the binder (any two; he can be wrong): the lower number survives with the boxes of both, the other number is gone from the
// lists; it can be taken out again until the next 08:00 (the night it was filed, FS47FreeformSession::NightOf). A file filed as one
// takes its stage from what in it is consistent: a box where the two held different things counts for nothing, and the pencil there
// is struck as any contradiction is. Only here can a stage drop. A file is FILED once it has a name and stands at HABITS; Dr. Ward
// counts the near three filed each in a file of its own (correctly kept apart or correctly filed as one): one point for two, two for
// all three (Tom, 13 November); a wrong one filed as one gives none until it is taken out again (REVIEW 14 Nov: that file; a kind's own file elsewhere still counts). Saved, all
// optional: "names": { "<number>": { "n": "...", "d": day } }; "merged": [ { "into": n, "from": n, "night": k, "was": stage,
// "name": "...", "nameDay": d, "had": b } ]; "paid": [numbers]. A save from slices 1 and 2 has neither and reads as it was.
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
 // Latest: the last night anything went in; HabitsFrom: the first night HABITS may be had (slice 2; 1: no such wait).
 SIGNAL47_API EStage StageFor(EKind Kind,int32 Types,int32 Nights,bool bLamp,int32 Latest=1<<30,int32 HabitsFrom=1);
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

 // ---------- slice 2 (14 November): the signatures in the channels that were the same for all ----------
 // The kind a sign out there belongs to, where its source knows (FS47Omen's ids): "visit" (the site visit's VisitWho), "body:N" (the
 // small ones' brain), "door" and "peek" (the small ones at the house). None for everything else (the Director's figures, lights).
 SIGNAL47_API EKind KindOfOmen(const FString& SourceId,int32 VisitWho);
 // The odd-sound generator's kind for a dossier kind (S47AlienKinds::Render), and back; the field tape keeps the id ("oddKind").
 SIGNAL47_API int32 AlienKindOf(EKind Kind); // ES47AlienKind as an int; -1 none
 // What the counter makes of each, against the plain reading of the same ground (S47Happenings::DoseAt): a little more, nothing, a
 // clear climb. 1 where the kind is not known; 0 for the far two (they leave nothing for it).
 SIGNAL47_API float GeigerFactor(EKind Kind);
 // How close a counter has to be held, and for how long out and ticking, before its reading is the file's (S47Happenings::StepGeiger).
 inline constexpr double GeigerNearM=12,GeigerHoldS=3;
 // Two finds were got together: the same night, and within this many metres where both kept a place.
 inline constexpr double NearM=250;
 // The pacing check (M7, Tom 13 November): the first nights each source can give anything, as director.json has them, and the
 // first night with danger in the curve. Earliest: in the best case for the player (every source that may come does, and he ties
 // all of a kind into one file), the first night a kind can be at Stage; 0: not by Last.
 struct FGates
 {
  int32 Visit=2;      // site_visit, field_visit: the three that come near (shape seen, tape, ground, lamp, counter, animals)
  int32 Glimpse=3;    // road_glimpse: the low one's and the tall one's shape and prints on the shoulder
  int32 Sky=2;        // horizon_figure: a photograph of the far two
  int32 Take=1;       // the unusual take on the receiver: the far two's tape
  int32 Figure=10;    // figure_near: the far two's answer to the lamp
  int32 RuleBreak=4;  // FS47RuleBreak::FromNight: the low one's answer to standing still, after the small ones' rule was learned
  int32 HabitsFrom=8; // the curve's first week with danger in it
 };
 SIGNAL47_API int32 Earliest(EKind Kind,EStage Stage,const FGates& Gates,int32 Last=30);
 // Felt, not fired: the moments a source tags. The counter out (held up or standing up on its own), within GeigerNearM of what it
 // reads, for GeigerHoldS; the receiver's take heard through the middle of its signal (S47AlienKinds' window).
 SIGNAL47_API bool GeigerReads(bool bOut,double Metres,double HeldFor);
 SIGNAL47_API bool TakeHeard(double Cursor);

 // ---------- a file as the notebook and the binder show it ----------
 struct FBox
 {
  FString Label,How,Note; // How: "on film", "seen", "noted"; Note: the pencil for it. Both empty while the box is.
  int32 Day=0;            // the first night it was logged
  bool bFilled=false;
  FString Sign;           // slice 2: the table's word for what is in it now (never shown)
  TArray<FString> Struck; // slice 2: the earlier pencil there that a later find contradicted, struck through, oldest first
 };
 struct FFile
 {
  int32 Number=0,Nights=0,Opened=0,Latest=0; // Opened, Latest: the first and the last night something went in
  FString Title,Stage,Next;                  // "UNKNOWN 2", "MARKS", how it is kept from here
  bool bAtMarks=false;
  FString Gave;                              // LOOP-DOSSIER-GIVES-20261113: what reaching the stage gave, one line
  TArray<FBox> Boxes;                        // six, in the table's order
  TArray<FString> Photos;                    // the photograph claims in it (their ids, for the session to name the prints by)
  TArray<int32> Beside;                      // slice 2: the files a find showed to be kept beside this one (their numbers)
  // slice 3: the player's name (empty: none) and the title with it ("UNKNOWN 2 (Bare feet)"); FILED (named, at HABITS); the numbers
  // filed in it as one, with the night each was; the hours it was logged at (the HABITS sketch)
  FString Name,Named;
  bool bFiled=false;
  TArray<int32> Merged,MergedNight,Hours;
 };
 // Slice 2: two files side by side, box by box: 0 one of them has nothing there, 1 they agree, 2 they differ.
 SIGNAL47_API TArray<int32> Agreement(const FFile& A,const FFile& B);
 // What is read aloud of a pair, and the binder's line for it.
 SIGNAL47_API FString BesideWords(const FFile& A,const FFile& B);
 SIGNAL47_API FString BesideLine(int32 Number);
 // REVIEW (14 Nov): a night (or a count of nights) as the dossier prints it: stepped past any number whose digits read 47 (47 shows
 // 48, 147 148, 470 to 479 480), as the cassettes are counted (S47Tape) and the clock is shown (S47Script::PlayerClock). Only what
 // is shown moves.
 SIGNAL47_API int32 ShownNight(int32 Night);
 // The file typed for the binder: the stage, the six boxes, the photographs as the session names them, the pencil notes.
 SIGNAL47_API FString Sheet(const FFile& File,const TArray<FString>& PhotoLines);
 // Every word the dossier can show, and what is wrong with any of it (empty: nothing): Freeform's spoiler limits
 // (S47Script::LintText), no word for what they are (S47Marginalia::NamesKind), none of Freeform's banned words.
 SIGNAL47_API TArray<FString> Words();
 SIGNAL47_API FString Lint();
 SIGNAL47_API FString LintWords(const TArray<FString>& Said); // the same checks on any words (slice 2: the sources' new text)
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);

 // ---------- slice 3 (14 November): names, files filed as one, Dr. Ward's count, the habits as thoughts, the sketches ----------
 inline constexpr int32 NameMost=24;
 // The player's working name as typed: trimmed, the spaces inside made single; letters, spaces and , . - ' only (no digits: nothing
 // can print a year or a clock); 1 to NameMost characters; and the design's word check (LintWords: Freeform's limits, no word for
 // what they are). False with Why, in the binder's own words, when it is not taken.
 SIGNAL47_API bool CleanName(const FString& Typed,FString& Out,FString& Why);
 // The suggestion the name field is filled with: plain working words from what the file holds now (its SHAPE, and its GROUND with it;
 // else its first filled box), never a name the game has for what it is. Empty for a file with nothing in it.
 SIGNAL47_API FString Suggest(const FFile& File);
 SIGNAL47_API TArray<FString> SuggestWords(); // every word a suggestion is made of (the lint)
 SIGNAL47_API FString Titled(int32 Number,const FString& Name); // "UNKNOWN 2", or "UNKNOWN 2 (Bare feet)"
 SIGNAL47_API FString StampOf(const FString& Title,EType Type,bool bPhoto,bool bNewFile,EStage Reached); // Stamp, by the file's title
 SIGNAL47_API FString HoursLineOf(const FString& Title,const TArray<int32>& Hours);                    // HoursLine, by the title
 // Dr. Ward's review: one point for two of the near three filed, two for all three.
 SIGNAL47_API int32 WardPoints(int32 KindsFiled);
 // The pencil sketch per stage on THEM: OUTLINE a rough build from the SHAPE box (three generic ones for the near three; the far two a
 // point of light and a bright outline; None while nothing has been seen), MARKS the prints from the GROUND box (0 none, 1 small
 // and bare, 2 in fours, 3 long and a stride apart), HABITS a clock face with File.Hours shaded.
 enum class ESketch : int32 { None=0, Small, Low, Tall, Point, Outline };
 SIGNAL47_API ESketch SketchOf(const FFile& File);
 SIGNAL47_API int32 PrintsOf(const FFile& File);
 // HABITS as the player's own thoughts (S47Thoughts, once a night a file): the hour coming up on a night out (HabitHourAhead seconds
 // before a dark hour the file was logged at), and the lamp raised within HabitLampNearM of one whose answer to the lamp the file holds.
 inline constexpr double HabitHourAhead=1200,HabitLampNearM=40;
 SIGNAL47_API bool HourNear(double Clock,int32 Hour);
 SIGNAL47_API FString HabitHourThought(int32 Hour);
 SIGNAL47_API FString HabitLampThought(const FString& LampSign); // empty: no habit line for that answer (the far two)
 // The binder's words for it (the UNKNOWNS rows, the sheet's lines, the slip a filing leaves; all in Lint).
 SIGNAL47_API FString NameRow();                         // the line that writes the name in the field on the file
 SIGNAL47_API FString MergeRow(int32 With,bool bBeside); // "FILE AS ONE WITH UNKNOWN 3"
 SIGNAL47_API FString TakeOutRow(int32 From);            // "TAKE UNKNOWN 3 OUT AGAIN"
 SIGNAL47_API FString NameSlip(const FString& Title);
 SIGNAL47_API FString MergeSlip(int32 Into,int32 From);
 SIGNAL47_API FString TakeOutSlip(int32 Into,int32 From);
 SIGNAL47_API FString AwayWords();                       // the reading copy: where naming and filing as one are done
 SIGNAL47_API FString NameHint();                        // the empty field's hint
 SIGNAL47_API FString MergedLine(const TArray<int32>& Numbers); // THEM: "Filed in it as one: UNKNOWN 3."; empty for none
}

class SIGNAL47_API FS47Dossier
{
public:
 // LOOP-DOSSIER-GIVES-20261113: Hour, the whole hour of the clock it was logged at (0 to 23; -1: not kept, as in a save from before),
 // and where the player stood (road-local x east, z south; bWhere false: not kept, or logged at a bench and not where it was got).
 // Slice 2: File, the file it went into (0-based: its number less one).
 struct FTag { FString Key; S47Dossier::EKind Kind=S47Dossier::EKind::None; S47Dossier::EType Type=S47Dossier::EType::None; int32 Day=1; int32 Hour=-1; bool bWhere=false; double X=0,Z=0; int32 File=0; };
 // What an older save kept, for Rebuild: a finding's key, its text and its day.
 struct FOld { FString Key,Text; int32 Day=0; };

 void Reset(){Sequence.Reset();Kept.Reset();Given.Reset();Spent.Reset();Same.Reset();Reached.Reset();Merges.Reset();Names.Reset();PaidFiles.Reset();}
 bool IsEmpty() const{return Kept.IsEmpty();}
 // A tag on a find. False when the key has one already, when the kind has no file (the shade), or when that box gives nothing for
 // this one. Slice 2: it goes into the file it ties to (see the head of this file), else it opens the next.
 bool Tag(const FString& Key,S47Dossier::EKind Kind,S47Dossier::EType Type,int32 Day,int32 Hour=-1,const FVector2D* Where=nullptr);
 bool HasTag(const FString& Key) const;
 // The kind that opened each file, in number order (a kind may open more than one).
 const TArray<S47Dossier::EKind>& Order() const{return Sequence;}
 const TArray<FTag>& Tags() const{return Kept;}
 int32 NumberOf(S47Dossier::EKind Kind) const; // the first file that kind opened, 1 up; 0: none
 int32 FileOf(const FString& Key) const;       // the number of the file a find went into; 0: not tagged
 int32 FilesOf(S47Dossier::EKind Kind) const;  // how many files that kind opened
 S47Dossier::EStage StageOfFile(int32 Number) const;
 // The whole of what the player has of one kind, whichever files it is in (the dev menu and the pacing; never shown as such).
 bool Has(S47Dossier::EKind Kind,S47Dossier::EType Type) const;
 int32 TypesOf(S47Dossier::EKind Kind) const;
 int32 NightsOf(S47Dossier::EKind Kind) const;
 S47Dossier::EStage StageOf(S47Dossier::EKind Kind) const;
 int32 AtMarks() const; // the files at MARKS or further: Dr. Ward's line
 // Slice 2: the pairs kept side by side (their numbers, the lower first), and the first night HABITS may be had (not saved).
 // Slice 3: as the files stand (a pair one of which is filed in the other is no pair any more).
 TArray<FIntPoint> Beside() const;
 void SetHabitsFrom(int32 Night){HabitsFrom=FMath::Max(1,Night);}
 int32 HabitsFromNight() const{return HabitsFrom;}
 // A find of FindKind's held against File's box of that type, from the player's own tags only: a box the player has not filled
 // answers Unknown whatever the table says.
 S47Dossier::EFit Fit(S47Dossier::EKind File,S47Dossier::EType Type,S47Dossier::EKind FindKind) const;
 TArray<S47Dossier::FFile> Files() const; // in the order of their numbers
 // LOOP-DOSSIER-GIVES-20261113. Where the file was first logged out where it was got: the earliest tag (by day, then as logged) that
 // kept a place. The hours it has been logged at, each once, in the night's order.
 bool FirstWhere(S47Dossier::EKind Kind,double& OutX,double& OutZ) const;
 TArray<int32> HoursOf(S47Dossier::EKind Kind) const;
 bool FirstWhereOfFile(int32 Number,double& OutX,double& OutZ) const;
 TArray<int32> HoursOfFile(int32 Number) const;
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
 // Slice 2: each kind's finds go into one file, as slice 1 had them (nothing in an older save says what the player could tie).
 int32 Rebuild(const TArray<FOld>& Findings,const TArray<FString>& Claims);

 // ---------- slice 3 (14 November) ----------
 // A file as it stands: a number that is not filed in another. Every per-number call above answers for files as they stand only
 // (StageOfFile NoFile, FirstWhereOfFile false, HoursOfFile empty for a number filed in another), and FileOf gives the survivor.
 bool IsFile(int32 Number) const;
 int32 SurvivorOf(int32 Number) const; // the file it is in now (itself if it stands); 0: no such number
 // The player's name, already clean (S47Dossier::CleanName); Day: the night it was written. False for no file as it stands.
 bool SetName(int32 Number,const FString& Name,int32 Day);
 FString NameOf(int32 Number) const;
 FString TitleOf(int32 Number) const;
 // File two as one (Night: the night it is done). The lower number survives with what both held; it keeps its name, or takes the
 // other's when it had none. False when either is not a file as it stands, or they are the same one.
 bool Merge(int32 A,int32 B,int32 Night);
 // Taken out again, as it was, only on the night it was filed as one (until the next 08:00).
 bool CanTakeOut(int32 Into,int32 From,int32 Night) const;
 bool TakeOut(int32 Into,int32 From,int32 Night);
 // What the game knows and never shows: the kind a file stands for (REVIEW 14 Nov: for each number in it, what most of its own finds
 // are of, so that a find that joined it by place and night does not change it; None when two numbers filed in it as one by the
 // player are of different kinds, or it holds nothing); a file filed as one that holds more than one (WrongMerge).
 S47Dossier::EKind KindOfFile(int32 Number) const;
 bool WrongMerge(int32 Number) const;
 bool Filed(int32 Number) const; // named, and at HABITS
 // Dr. Ward's count: of the near three, each with a file FILED that stands for it (KindOfFile); a wrong one filed as one counts for
 // nothing itself, and holds back no kind that has a file of its own elsewhere (REVIEW 14 Nov).
 int32 KindsFiled() const;
 bool Paid(int32 Number) const; // the field allowance for a file FILED, once (FS47FreeformSession::DossierPayFiled)
 void MarkPaid(int32 Number);
 // The habits as thoughts, once a night a file (Came: by number, the night its thought came). The file at HABITS whose hour is
 // coming up (OutHour), or whose lamp box holds what Near does in the lamp; 0: none.
 int32 HabitHourFile(double Clock,const TMap<int32,int32>& Came,int32 Night,int32& OutHour) const;
 int32 HabitLampFile(S47Dossier::EKind Near,const TMap<int32,int32>& Came,int32 Night) const;

private:
 TArray<S47Dossier::EKind> Sequence;
 TArray<FTag> Kept;
 TArray<FIntPoint> Same;
 int32 HabitsFrom=1;
 int32 FirstFileOf(S47Dossier::EKind Kind) const{return Sequence.IndexOfByKey(Kind);}
 bool Put(FTag&& One,int32 File); // into that file (INDEX_NONE: a new one); false when there is no room
 void Link(int32 A,int32 B);      // 0-based
 FString BoxSign(int32 File,S47Dossier::EType Type) const; // what is in that box of that file now; empty: nothing
 S47Dossier::EStage StageIn(int32 File) const;
 S47Dossier::EStage StageNow(int32 File) const; // from its tags alone; StageIn never goes below what Reached keeps
 TArray<int32> Reached; // REVIEW (14 Nov): by file, the highest stage it has had (saved as "reached" only where its tags alone say less)
 TMap<int32,int32> Given; // by the kind's number: the night each file's extra bearing is for
 TSet<int32> Spent;       // and the ones printed
 // slice 3: by file (0-based); a merge, Into and From 0-based, with what the survivor was before it (for taking it out again)
 struct FNamed { FString Text; int32 Day=0; };
 struct FMerge { int32 Into=0,From=0,Night=0,ReachedWas=0; FNamed NameWas; bool bHadName=false; };
 TMap<int32,FNamed> Names;
 TArray<FMerge> Merges;
 TSet<int32> PaidFiles;           // the files whose allowance for being FILED is paid (saved as "paid", their numbers)
 int32 Root(int32 File) const;    // the file it is in now, 0-based
 bool IsIn(const FTag& One,int32 File) const{return Root(One.File)==File;}
 bool HasMembers(int32 File) const{return Merges.ContainsByPredicate([File](const FMerge& One){return One.Into==File;});}
 bool Apply(const FMerge& One);   // the merge itself (Merge, and Load as saved)
 S47Dossier::EKind PartKind(int32 Part) const; // REVIEW (14 Nov): what most of the finds under that number (0-based) are of
 S47Dossier::EStage StageConsistent(int32 File) const; // a file filed as one: from the boxes where all in it agree
};

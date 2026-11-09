#pragma once
#include "CoreMinimal.h"

// Everything Freeform says about the operator before you, and about Ward's review, kept here so it can be read through in one go.
// The line it keeps to (Tom, 7 November): only the outside. That Reyes had the nights, was gone from 14 April, that Mara Ortiz is gone
// too, that the originals are gone, and that something does not add up. FS47Predecessor::SelfTest refuses words from the far side.
namespace S47PredecessorText
{
 struct FDocDef{const TCHAR* Id;const TCHAR* Title;const TCHAR* Kind;const TCHAR* Stamp;const TCHAR* Page;const TCHAR* Summary;const TCHAR* Next;};
 // A SARO survey marker Reyes left beside a place. Offset in metres from the place's own centre, x east, z south.
 struct FTagDef{const TCHAR* Place;const TCHAR* Doc;double OffX,OffZ;};
 // Filed among the sheets of a case: it comes to light when that case opens.
 struct FCaseNote{const TCHAR* CaseTitle;const TCHAR* Doc;};
 struct FSay{const TCHAR* Who;const TCHAR* Line;};
 struct FTopic{const TCHAR* Id;const TCHAR* Label;const TCHAR* Needs;const TCHAR* Hears;const TCHAR* Note;TArray<FSay> Says;};
 struct FPerson{const TCHAR* Id;const TCHAR* Title;TArray<FSay> First;TArray<FTopic> Topics;};
 const TArray<FDocDef>& Docs();
 const FDocDef* Doc(const FString& Id);
 const TArray<FTagDef>& Tags();
 const TArray<FCaseNote>& CaseNotes();
 const TArray<FPerson>& People();
 // What Dale says over the walkie, unasked. Each once, at most one a day. Needs as for a topic; empty: from the day given.
 struct FBark{const TCHAR* Id;const TCHAR* Needs;int32 FromDay;const TCHAR* Line;};
 const TArray<FBark>& Barks();
 // WALKIE-USE-20261108: what Dale says on the walkie when the player calls him, his hail when he calls the player, and how he passes
 // on what the county rang through to his bench (CB-WALKIE-20261108). Short and angry, his own words: fiction, nobody's quotes.
 struct FWalkieLine{const TCHAR* Id;const TCHAR* Line;};
 const TArray<FWalkieLine>& WalkieLines();
 // The line with that id; empty if there is none.
 FString WalkieLine(const TCHAR* Id);
 const FPerson* Person(const FString& Id);
 FString Letter(bool bOlderGame,const FString& Dated,const FString& Assess);
 // Ward's Monday sheet. Her remark follows what the assessment will go by (FS47Predecessor::Points), not the count of cards, and it
 // names the hours in the log nobody can account for.
 // Land: kinds of record kept from the land at night (FS47FreeformSession::NightRecordKinds), which count too.
 // M7: bWouldStay: the assessment today would keep the station (FS47Predecessor::Decide), the only time she praises the week.
 // Levers: her word on each other thing she reads (Levers below). Reprieved: the times taken at "Not yet" (FS47Predecessor's
 // TakenAtReprieve). From three stretches of lost time she says what she has already done about it only once "Not yet" answered
 // that count; noticed and not yet acted on, only that she meant it.
 FString WeekNote(int32 Week,int32 Filed,int32 Concluded,int32 Dishes,int32 Clean,int32 Arrears,int32 DaysLeft,const FString& Assess,int32 Points,int32 PointsNeeded,int32 Taken,int32 Land=0,bool bWouldStay=false,const FString& Levers=FString(),int32 Reprieved=0);
 // From this much carried on the sheet she names its limit (FS47FreeformSession::ArrearsCap) in her own words.
 constexpr int32 CarriedWarn=40;
 // One sentence of hers per thing the assessment reads besides the numbers on the sheet, no figures but money: the night sheets,
 // the tapes, the weekly reports, the photographs, a filing with nothing behind it, and the limit of the carried line.
 FString Levers(bool bLogsCount,bool bTapesCount,int32 TrustPoints,bool bPhotosCount,int32 Guessed,int32 Arrears,int32 Cap);
 // The Monday sheet after KEPT OPEN: shorter, and nothing to win.
 FString KeptNote(int32 Week,int32 Filed,int32 Dishes,int32 Arrears,const FString& FiscalEnd,int32 Taken,const FString& Levers);
 // What the Stays letter says of what was found (M7): wagon 23, the deputy's file, the routed pay, Ortiz's thermos sent on. Only the
 // outside; empty if none.
 FString Followed(bool bWagon,bool bDeputy,bool bPayReturned,bool bThermos=false);
 FString Stays(const FString& Dated,int32 Filed,int32 Concluded,int32 Dishes,int32 Arrears,bool bKnowsMuch,const FString& FoundOut=FString());
 // Day 22: the county's inspector writes ahead of the assessment, about the one thing he will look at first.
 // Weakest: 0 cases, 1 the array, 2 the work, 3 the carried line, 4 readable records, 5 nothing he can find.
 FString Inspector(const FString& Dated,const FString& Assess,int32 Weakest);
 FString KnowsEnough(const FString& Dated,int32 Filed,int32 Concluded,int32 Dishes,int32 Arrears,const FString& CloseOn);
 FString WhatYouKnow();
 FString ClosedOriginals(const FString& Dated,int32 Filed,int32 Concluded,int32 Dishes,int32 Arrears,int32 Originals,int32 Photos,const FString& CloseOn);
 FString ClosedNothing(const FString& Dated,int32 Filed,int32 Concluded,int32 Dishes,int32 Arrears,const FString& CloseOn);
 // After the second stretch of lost time: one more, and somebody else has the nights.
 FString TakenWarning(const FString& Dated,int32 Taken);
 // The morning the closing order takes effect.
 FString ClosedNow(const FString& Dated);
 FString NextOperator(const FString& Dated,const FString& YourLast,int32 Originals,int32 Photos,int32 Filed);
}

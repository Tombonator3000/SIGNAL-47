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
 const FPerson* Person(const FString& Id);
 FString Letter(bool bOlderGame,const FString& Dated,const FString& Assess);
 FString WeekNote(int32 Week,int32 Filed,int32 Dishes,int32 Arrears,int32 DaysLeft,const FString& Assess,double Pace);
 FString Stays(const FString& Dated,int32 Filed,int32 Concluded,int32 Dishes,int32 Arrears,bool bKnowsMuch);
 FString KnowsEnough(const FString& Dated,int32 Filed,int32 Concluded,int32 Dishes,int32 Arrears);
 FString WhatYouKnow();
 FString ClosedOriginals(const FString& Dated,int32 Filed,int32 Concluded,int32 Dishes,int32 Arrears,int32 Originals,int32 Photos);
 FString ClosedNothing(const FString& Dated,int32 Filed,int32 Concluded,int32 Dishes,int32 Arrears);
 FString NextOperator(const FString& Dated,const FString& YourLast,int32 Originals,int32 Photos,int32 Filed);
 const TCHAR* const WardClose=TEXT("June 30");
}

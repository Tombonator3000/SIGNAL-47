#pragma once
#include "CoreMinimal.h"

// RECORDS-BINDER (-> M3, Tom 12 November: "redesign the records binder with better logic and UI/UX").
// The ring binder in SARO's records room, as a model: what is filed behind which divider, in what order, what each page says and
// which page it belongs with. Pure: no world, no session, no Slate. FS47FreeformSession::BinderSource hands over plain copies of
// its own records (the jobs with an original, the findings, the cases, the station's files) and Build files them. Nothing here is
// invented and nothing here is saved; SS47Binder (SS47Binder.h) draws it and FS47FreeformSession::OpenBinder opens it.
//
// The dividers:
//   INDEX          the front sheet: a packet waiting, one line for each divider, the last pages filed
//   TODAY          what went in today: the packet to file, originals filed today, records and cases of today, originals still at OPS
//   CASES          one sheet for each case, open or closed, with its pages (originals, test recordings, leads) listed under it
//   DAILY WORK     every original by day; FILED once its packet is in the binder, HELD AT OPS until then
//   FIELD + BENCH  what was retained in the field, on the station round and at the workshop bench, by day
//   STATION FILES  the station's own papers as the duty officer releases them; read only
//
// One page belongs to one divider (its home). TODAY and a case's list point at pages that live behind other dividers; ENTER on such
// a line turns to the page's own divider, and ENTER on a page that belongs to a case turns to the case.
namespace S47Binder
{
 enum ESection : int32 { SecIndex=0, SecToday, SecCases, SecDaily, SecField, SecStation, SecCount };

 // ---------- what the session hands over ----------
 struct FJobIn
 {
  FString Id,Source,Showed,Original,TestName; // Showed: the registered result in the station's words; Original: the chart's own page
  int32 Day=1,CaseLine=-1,Paid=-1,FiledDay=0; // FiledDay 0: no stamp kept (filed before the stamp existed, or by a fixture)
  double Scheduled=-1,FiledClock=-1,ControlAt=0,MarkStart=-1,MarkEnd=-1;
  bool bFiled=false,bPacket=false,bControl=false,bAnomaly=false;
 };
 struct FFindingIn { FString Key,Text; int32 Day=0; };
 struct FCaseIn
 {
  int32 Line=0,OpenDay=0,ConcludeDay=0,Folded=0;double OpenClock=0;
  FString Title,Kind,Question,Head,Next,Reply;
  bool bConcluded=false,bAssessment=false,bReplied=false;
  TArray<FString> Tests,Leads; // Tests: one line for each result registered; Leads: the keys of the findings kept as its leads
 };
 struct FFileIn { FString Key,Title,Text; int32 Day=1; bool bSeen=false; };
 struct FSource
 {
  int32 Today=1;FDateTime Day1=FDateTime(1986,4,28);
  bool bAtBinder=false; // the operator stands at the binder itself: a packet can go in
  FString ToFileId;     // the job whose packet the next filing puts in (FS47FreeformSession::FilePacket's choice); empty for none
  TArray<FJobIn> Jobs;TArray<FFindingIn> Findings;TArray<FCaseIn> Cases;TArray<FFileIn> Files;
  int32 FilesHeld=0; // station files the duty officer has not released yet
  FString Older;     // the binder from before, on the same shelf: its label, empty for none
 };

 // ---------- the binder ----------
 struct FEntry
 {
  FString Key;                 // "job:<id>", "rec:<finding key>", "case:<line>", "file:<key>"
  int32 Home=SecDaily;            // the divider it is filed behind
  int32 Day=0,Rank=0,Seq=0;    // Day: the day it belongs to (0: undated). Within a divider: higher Rank first, then lower Seq
  int32 TodayGroup=-1;         // on TODAY: 0 to file, 1 filed today, 2 retained today, 3 still at OPS; -1 not there
  FString Kind,Ref,DayLine,When,What,Showed,Status,StatusLine; // Status: a word or two for a list; StatusLine: the page's own line
  double FiledAt=-1;           // a filed original: when it went in, for LAST FILED (its day alone when no stamp was kept)
  int32 Tone=0;                // 1 in the binder, 0 plain, -1 waits for the operator
  bool bNew=false,bToFile=false;
  TArray<FString> Pages;       // the sheet, a page at a time; never empty
  FString Link,LinkLabel;      // the page it belongs with (a case), and the words for turning to it
  TArray<FString> Children;    // a case's pages: keys of entries behind other dividers
  FString Stamp;               // the rubber stamp on a filed page; empty when it is not in the binder
 };
 // A line of a divider's list. A heading (bHead) is not chosen. Id is unique within the list.
 struct FRow
 {
  FString Id,Left,Mid,Right; // Left and Right on the first line, Mid under them
  FString Jump,Act;          // ENTER: turn to this entry (Jump), with the words for it (Act); "FILE THE PACKET" on the packet in hand
  int32 Entry=-1,Indent=0,Goto=-1,Tone=0; // Goto: ENTER turns to this divider
  bool bHead=false,bFile=false,bOlder=false,bNew=false;
 };
 struct FDivider { FString Tab,Title,Empty,Summary; TArray<int32> Entries; int32 New=0; };
 struct FModel
 {
  TArray<FEntry> Entries;FDivider Dividers[SecCount];
  int32 ToFile=-1;        // the entry of the packet in hand, -1 for none
  bool bAtBinder=false;
  FString Date,Older,Front,Held;
  TArray<int32> Latest;   // LAST FILED: the originals in the binder, newest first, five at most
  int32 Find(const FString& Key) const;
  // The divider's list as it stands with this line chosen (a case shows its pages only while it, or one of them, is chosen).
  TArray<FRow> Rows(int32 Section,const FString& PickId) const;
  // Where the binder opens: TODAY with a packet in hand, else the index.
  int32 Land() const { return ToFile>=0?int32(SecToday):int32(SecIndex); }
 };
 // Where the binder is open. The session keeps it between visits; never saved.
 struct FView { int32 Tab=-1,Page=0; FString Pick,Note,NoteKey; };

 SIGNAL47_API FModel Build(const FSource& In);
 // A text a page at a time: Wide characters to a line, Lines lines to a page, as near as a typed line can be counted. The words
 // are kept as they are; a paragraph goes whole to the next page when it fits there, and is cut at a space when it is longer than a page.
 SIGNAL47_API TArray<FString> Paginate(const FString& Text,int32 Wide,int32 Lines);
 SIGNAL47_API FString DateOf(const FSource& In,int32 Day); // "MON APR 28"; "UNDATED" for day 0
 // A finding's place: the workshop bench or the field; and whether it is one that still asks a question.
 SIGNAL47_API bool BenchKey(const FString& Key);
 SIGNAL47_API bool OpenQuestionKey(const FString& Key);
 SIGNAL47_API FString JobKey(const FString& Id);
 SIGNAL47_API FString RecordKey(const FString& FindingKey);
 SIGNAL47_API FString CaseKey(int32 Line);
 SIGNAL47_API FString FileKey(const FString& Key);
 // The binder's own words keep to Freeform's spoiler limits (S47Script::LintText). Empty when all is well.
 SIGNAL47_API FString Lint();
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
 // The page a sheet is cut to (SS47Binder's right-hand page at 1080p).
 constexpr int32 SheetWide=54,SheetLines=15;
}

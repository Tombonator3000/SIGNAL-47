#pragma once
#include "CoreMinimal.h"
#include "Widgets/SWidget.h"

// THE CALICHE LEDGER, the valley's weekly paper, in the box at the road on Thursdays (MAILBOX-20261108; Tom: «Lag lokal avis,
// randomisert innhold, nyheter og vær som forholder seg mer eller mindre til spillets vær»). Caliche is a town in Socorro County, as
// the sheriff's letters and the district payroll have it; the paper is the town's. A front page that tells what the station
// registered in the nights (a finding with its place and its night, FS47Mail::WeekFacts), at the place it was, written the way a
// small paper writes it: careful, second-hand, with no answer in it; what the front has no room for goes in the sheriff's log. The
// rest is drawn with the game's seed from large pots: county news, the sheriff's log, the week's weather from S47Weather (what the
// sky shows), the sky's almanac, ads that can carry a coupon for the Caliche counter, the motel or the catalog, and letters and
// classifieds, some of which point at a place out on the land. An item with a date is printed only before it (or after it, for what
// came of it); items that tell one story share a thread, one to a paper, and nothing is printed twice. Front is kept in the save as
// it was printed, with the templates it used, so the next weeks use others; everything else follows from the seed, the issue's
// number and its date, so a Continue shows the same paper. All names are made up; no brands, no real films. S47Ledger::SelfTest
// keeps it so.
namespace S47Ledger
{
 // What a week can leave that the town might hear of, most newsworthy first. Called: the player called the missing man's truck in.
 enum class ETopic:uint8{Called,Missing,Carcass,Cattle,Landing,Lights,LitHouse,Stalled,EmptyRig,Camper,Walker,Outage,Fled,ClockFast,Rumor,Helped,Count};
 // A finding as the paper hears of it. Night: "Tuesday night". Place: how people would say where ("the church"); PlaceId: a place on
 // the land (FS47Places) or empty. Id: the finding it came from.
 struct FFact{ETopic Topic=ETopic::Lights;int32 Day=0;FString Night,Place,PlaceId,Id;};
 // Lead: what it points the player at: a place on the land (FS47Places), "wagon" (wagon 23), or nothing; since D45-24 the player
 // pencils it onto the map by clicking the story (FS47Mail::MarkLead), reading alone marks nothing. Key: the template
 // it was written from (kept, so the next weeks use another). FactId: the finding it tells, if any.
 struct FStory{FString Head,Body,Lead,Key,FactId;};
 struct FAd{FString Name,Body,Coupon;};
 struct FNote{FString Head,Body,Lead;};
 // A line of the sheriff's log from the week's findings: how many days before the paper, and what it says.
 struct FLogLine{int32 Ago=0;FString Text;};
 struct FIssue
 {
  int32 No=0,Day=0;FDateTime Date;
  TArray<FStory> Front;
  TArray<FLogLine> Log;
  TArray<FString> Locals,Sheriff,Week;
  TArray<FAd> Ads;
  TArray<FNote> Letters;
  FString Showing,LastWeek,Almanac;
  FString Photo; // D46-1-20261110: the front page's photo from the world (PhotoFromPixels); empty: the drawn picture
 };
 // What an ad's coupon gives, until the day before the next paper. What: a supply at the Caliche counter (FS47FreeformWorld's id),
 // "motel", "post" (postage on a mail order), or "mail:<id>" (that line of the catalog).
 struct FCoupon{const TCHAR* Id;const TCHAR* What;int32 Price;const TCHAR* Line;};
 const TArray<FCoupon>& Coupons();
 const FCoupon* Coupon(const FString& Id);
 // The front page, two stories: the week's two most newsworthy findings, each at its own place, or the county's own news where the
 // week left too little. The first issue always carries the search for the two who went missing on April 14. A week that points at
 // nothing gets talk about Unknown (a place on the land nobody has told the player of; empty when there is none). Used: the
 // templates earlier papers printed, with the number of the paper; an unused one is taken first, else the one used longest ago.
 TArray<FStory> Front(int32 Seed,int32 No,const FDateTime& Date,const TArray<FFact>& Facts,const TMap<FString,int32>& Used,const FString& UnknownId,const FString& UnknownTitle);
 // The findings the front had no room for, as the sheriff's log puts them (one to a topic and a place, at most Most), each with
 // how many days before the paper (OnDay) it was.
 TArray<FLogLine> LogLines(const TArray<FFact>& Facts,const TArray<FStory>& Front,int32 OnDay,int32 Most);
 // Everything but the front page and the log's findings, from the seed and the issue's number and date.
 void Fill(FIssue& Issue,int32 Seed,bool bHelped);
 // D45-24-20261110 (Tom: «skal se ut som en ekte avis; kanskje kan man trykke på en artikkel med merkelig info og få stedet på
 // kartet»). What the page asks of the one who reads it: whether a lead is on the player's map already (what the map knows: heard
 // of or found, so nothing new is saved), and to pencil one onto it. Empty: the stories cannot be clicked (the tests' pages).
 struct FLeadHooks{TFunction<bool(const FString&)> Marked;TFunction<void(const FString& Lead,const FString& Head)> Mark;};
 // An article that points somewhere: its lead, its head, and the sheet it is printed on (0 the front, 1 inside).
 struct FLeadRef{FString Lead,Head;int32 Sheet=0;};
 // The paper has two sheets: the front page (the two stories, a photo, the county briefs) and the inside (the sheriff's log, the
 // letters and classifieds, the weather, the almanac, the picture show and the ads).
 constexpr int32 Sheets=2;
 // How the paper names a lead: the place's own title ("the windmill on the Baca lease"), the district's wagon for "wagon", empty
 // for anything the map cannot show.
 FString LeadTitle(const FString& Lead);
 // Every article of the issue with a lead the map can show, front page first, each lead once to a sheet.
 TArray<FLeadRef> LeadsOf(const FIssue& Issue);
 // D46-1-20261110 (Tom: «tar et svarthvitt bilde fra verdenen som passer til artikkelen»): the front page's photo, made in the
 // world at the lead story's place and kept with the issue as a small grey JPEG data URL (about 320x170, 5 to 12 KB). Any frame
 // (BGRA, top row first) to that, empty if it cannot be made; and whether a kept URL is one the paper will print.
 constexpr int32 PhotoStoreW=320,PhotoStoreH=170,PhotoQuality=60,PhotoMaxChars=32000;
 FString PhotoFromPixels(const TArray<FColor>& Px,int32 W,int32 H);
 bool PhotoValid(const FString& Url);
 // The paper as a page: newsprint, a nameplate, six columns, a photo on the front, the ads inside. Its coupons are clipped from
 // under it (FS47Mail). Sheet: 0 the front, 1 inside. An article with a lead can be clicked to pencil it onto the map (Hooks).
 TSharedRef<SWidget> Page(const FIssue& Issue,int32 Sheet=0,const FLeadHooks& Hooks=FLeadHooks());
 // The same as plain text, for the notebook.
 FString PlainText(const FIssue& Issue);
 FString DateLine(const FDateTime& Date);   // "THURSDAY, MAY 1, 1986"
 // The words nothing in the post may say: Story's far side (Freeform's spoiler line), brand names, British spellings and words, and
 // a sex or a first name for Reyes. False, with the word, when Text has one. Shared with FS47Mail::SelfTest.
 bool Clean(const FString& Text,FString& Word);
 TArray<FString> AllText();
 bool SelfTest(TArray<FString>& Lines);
}

// The pots (S47LedgerText.cpp). {place} is where, {night} when ("Tuesday night"); both are filled in from the week's findings.
// First and Last: the window an item may be printed in, as month*100+day (0: open at that end). Thread: items that tell one story
// (the truck stop, the graduation), never two on one paper.
namespace S47LedgerText
{
 struct FItemDef{const TCHAR* Text;const TCHAR* Thread;int32 First,Last;};
 struct FAdDef{const TCHAR* Name;const TCHAR* Body;const TCHAR* Coupon;int32 First,Last;};
 struct FLetterDef{const TCHAR* Head;const TCHAR* Body;const TCHAR* Lead;const TCHAR* Thread;int32 First,Last;};
 struct FFrontDef{S47Ledger::ETopic Topic;const TCHAR* Head;const TCHAR* Body;const TCHAR* Thread;int32 First,Last;};
 struct FLogDef{S47Ledger::ETopic Topic;const TCHAR* Line;};
 const TArray<FItemDef>& Locals();
 const TArray<const TCHAR*>& Sheriff();
 const TArray<FAdDef>& Ads();
 const TArray<FLetterDef>& Letters();
 const TArray<const TCHAR*>& Films();
 const TArray<FFrontDef>& Fronts();
 // The county's own news for a week that left little: the search first, the review of the station second, then the rest.
 const TArray<FFrontDef>& Quiet();
 // The sheriff's log's way of putting a finding the front had no room for.
 const TArray<FLogDef>& Logs();
 // The letter of thanks when the player pulled a driver out (cb_helped).
 const FLetterDef& Thanks();
 // Whether an item with this window may be printed on this date.
 bool InWindow(int32 First,int32 Last,const FDateTime& Date);
}

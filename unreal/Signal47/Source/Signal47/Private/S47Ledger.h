#pragma once
#include "CoreMinimal.h"
#include "Widgets/SWidget.h"

// THE CALICHE COUNTY LEDGER, the valley's weekly paper, in the box at the road on Thursdays (MAILBOX-20261108; Tom: «Lag lokal avis,
// randomisert innhold, nyheter og vær som forholder seg mer eller mindre til spillets vær»). A front page that remembers what the
// nights left (lights over a place, cattle, the missing man, rumors), written the way a small paper writes it: careful, second-hand,
// with no answer in it. The rest is drawn with the game's seed from large pots, so no item comes round twice in a month: county
// news, the sheriff's log, the week's weather from S47Weather (the same weather the game keeps, with a forecaster's errors), the
// sky's almanac, ads that can carry a coupon for the Caliche counter, the motel or the catalog, and letters and classifieds, some of
// which point at a place out on the land. Front is kept in the save as it was printed; everything else follows from the seed and the
// issue's number, so a Continue shows the same paper. All names are made up; no brands, no real films. S47Ledger::SelfTest keeps it so.
namespace S47Ledger
{
 // What a week can leave that the town might hear of, most newsworthy first. Called: the player called the missing man's truck in.
 enum class ETopic:uint8{Called,Missing,Carcass,Cattle,Landing,Lights,Rumor,Stalled,Outage,EmptyRig,Camper,Walker,Helped,Count};
 // Night: "Tuesday night". Place: how people would say where ("the church"); PlaceId: a place on the land (FS47Places) or empty.
 struct FFact{ETopic Topic=ETopic::Lights;int32 Day=0;FString Night,Place,PlaceId;};
 // Lead: what reading it points the player at: a place on the land (FS47Places), "wagon" (wagon 23), or nothing.
 struct FStory{FString Head,Body,Lead;};
 struct FAd{FString Name,Body,Coupon;};
 struct FNote{FString Head,Body,Lead;};
 struct FIssue
 {
  int32 No=0,Day=0;FDateTime Date;
  TArray<FStory> Front;
  TArray<FString> Locals,Sheriff,Week;
  TArray<FAd> Ads;
  TArray<FNote> Letters;
  FString Showing,LastWeek,Almanac;
 };
 // What an ad's coupon gives, until the day before the next paper. What: a supply at the Caliche counter (FS47FreeformWorld's id),
 // "motel", "post" (postage on a mail order), or "mail:<id>" (that line of the catalog).
 struct FCoupon{const TCHAR* Id;const TCHAR* What;int32 Price;const TCHAR* Line;};
 const TArray<FCoupon>& Coupons();
 const FCoupon* Coupon(const FString& Id);
 // The front page, two stories: the week's two most newsworthy facts, or the county's own news where the week left too little. The
 // first issue always carries the search for the two who went missing on April 14. A week that points at nothing gets a rumor about
 // Unknown (a place on the land nobody has told the player of; empty when there is none).
 TArray<FStory> Front(int32 Seed,int32 No,const TArray<FFact>& Facts,const FString& UnknownId,const FString& UnknownTitle);
 // Everything but the front page, from the seed and the issue's number and date.
 void Fill(FIssue& Issue,int32 Seed,bool bHelped);
 // The paper as a page: newsprint, a masthead, columns, the ads. Its coupons are clipped from under it (FS47Mail).
 TSharedRef<SWidget> Page(const FIssue& Issue);
 // The same as plain text, for the notebook.
 FString PlainText(const FIssue& Issue);
 FString DateLine(const FDateTime& Date);   // "THURSDAY, MAY 1, 1986"
 // The words nothing in the post may say: Story's far side (Freeform's spoiler line), brand names, British spellings, and a sex or a
 // first name for Reyes. False, with the word, when Text has one. Shared with FS47Mail::SelfTest.
 bool Clean(const FString& Text,FString& Word);
 TArray<FString> AllText();
 bool SelfTest(TArray<FString>& Lines);
}

// The pots (S47LedgerText.cpp). {place} is where, {night} when ("Tuesday night"); both are filled in from the week's facts.
namespace S47LedgerText
{
 struct FAdDef{const TCHAR* Name;const TCHAR* Body;const TCHAR* Coupon;};
 struct FLetterDef{const TCHAR* Head;const TCHAR* Body;const TCHAR* Lead;};
 struct FFrontDef{S47Ledger::ETopic Topic;const TCHAR* Head;const TCHAR* Body;};
 const TArray<const TCHAR*>& Locals();
 const TArray<const TCHAR*>& Sheriff();
 const TArray<FAdDef>& Ads();
 const TArray<FLetterDef>& Letters();
 const TArray<const TCHAR*>& Films();
 const TArray<FFrontDef>& Fronts();
 // The county's own news for a week that left little: the search first, the review of the station second, then the rest.
 const TArray<FFrontDef>& Quiet();
 // The letter of thanks when the player pulled a driver out (cb_helped).
 const FLetterDef& Thanks();
}

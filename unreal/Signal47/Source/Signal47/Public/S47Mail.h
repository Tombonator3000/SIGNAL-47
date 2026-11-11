#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class FS47MailBoxModel;
class FS47MailCarModel;
namespace S47Ledger{struct FIssue;struct FFact;struct FLeadRef;}

// MAILBOX-20261108 (Tom: «Inkluder post funksjon til SARO og lag postboks utenfor SARO»). Freeform only. A rural box on a post where
// the service road meets the highway (S47MailBox.cpp), and the post that comes to it: the carrier once a day, Monday to Saturday,
// between noon and half past one, in a small white mail truck up the highway from the south (FS47MailCarModel). When the player is
// at the station it is seen and heard on the road: it pulls over and stops at the box when it has something to leave or to take,
// its roof light flashing and the lid clanking, and goes on north; with nothing for SARO it drives by. A round the player was not
// there for (away, asleep, a long rest) is made without the car. The red flag is the house's, as in the whole country: up when an
// order waits in the box for the carrier, down when the carrier has taken it. The carrier never puts it up, so the player knows
// there is post by the truck stopping at the box, by the flag gone down, by a parcel sitting on top of the box, or by going to look.
// E at the box takes it out. What comes:
//   ward      Dr. Ward's letter, her Monday sheets, her warning about the lost hours and the assessment (FS47Predecessor::Send).
//             Taken out, they go to the OPS tray as before, where the review and its tests look for them.
// What is taken out stays on the box's list as NOT READ YET until it is read (PT-MAIL-20261109, Tom: the letters he took out with
// the catalog and the parcel were never seen again), and the list of things to do and the parcel's note say where it is.
//   ledger    THE CALICHE LEDGER on Thursdays (S47Ledger): what the station registered in the nights, at its place, on the front page,
//             coupons, places to look (D45-24: the reader clicks a story to pencil its place onto the map).
//   catalog   Western Surplus & Radio Supply's catalog, in the box on the first day: parts and gear by mail, cheaper than Caliche,
//             ordered with a money order left in the box with the flag up; the carrier takes it, and the parcel comes one to three
//             rounds later, on top of the box. Lifted down, a parcel is in both hands, for GARAGE RECEIVING like any other.
//   bill      the weekly statement after each review, and the county's bill for a tow: what was charged, on paper.
//   nobody    twice in a month or so, something with no stamp and no sender: a blank sheet, a postcard, or a county road map with a
//             place circled in pencil, which goes on the player's map. It explains nothing.
// What it opens for the others: the station's stock and kit (parcels), money (orders, coupons at the counter and the motel), places
// on the map and wagon 23 (the paper, the map), and Ward's papers for the review. All of it is saved under "mail" and survives a
// Continue; a save from before it finds the catalog in the box. Developer runs have no post unless F10 or a test asks.
class FS47Mail
{
public:
 explicit FS47Mail(US47Main* Main);
 ~FS47Mail();
 void Begin(const S47J::FObj& Saved,bool bSavedGame);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Live() const;

 // One thing for the box. Kind: "ward", "ledger", "catalog", "parcel", "bill", "nobody". Id: unique in the game and never posted
 // twice ("ward:letter"). Ref: the tray id, the issue, the finding that holds a bill's text. Place: the place a map circles. Day:
 // the day it is in the box.
 struct FPiece{FString Id,Kind,Ref,Text,Place;int32 Day=0;TMap<FString,int32> Goods;};
 // Into the box with the next round (bNow: in it already). False: posted before, or there is no post.
 bool Post(const FPiece& Piece,bool bNow=false);
 // Dr. Ward's paper with this tray id (FS47Predecessor). False: there is no post in this game; it goes to the tray instead.
 bool PostWard(const FString& TrayId,bool bNow);
 // The statement of a weekly review (its finding, FS47FreeformSession::StepWeeklyReview), and the county's bill for a tow.
 void PostWeekStatement(int32 Week);
 void PostTowBill(int32 Fee,bool bPaidInCash);
 // D48-20261110: something of the station's left in the box for the carrier that is no order: the balloon's instrument box in its
 // mailing bag (FS47Balloon::Mail). Id: once in a game. What: the line the box's panel and the task list say of it. Rounds: the
 // carrier's rounds after it is taken until Reply comes back to the box. A reply of kind "letter" has its title in Ref and, with
 // "dollars" in its Goods, a check in it, cashed to the budget when it is taken out. The flag goes up for it as for an order.
 // False: posted before, or no reply to come.
 bool PostOut(const FString& Id,const FString& What,int32 InRounds,const FPiece& Reply);
 bool OutWaiting(const FString& Id) const;
 bool Holds(const FString& Id) const;
 bool Coming(const FString& Id) const;
 bool Ever(const FString& Id) const{return Posted.Contains(Id);}
 int32 InBox() const{return Box.Num();}
 // The house's red flag: up while an order waits in the box for the carrier, and only then.
 bool FlagUp() const{return bFlag;}
 // Anything in the box (what the flag never says).
 bool MailIn() const{return !Box.IsEmpty();}
 bool ParcelOnBox() const;
 // Taken out of the box and not read yet, oldest first, the catalog last; and a line that says where they are (empty: none).
 TArray<FPiece> UnreadPieces() const;
 FString UnreadNote() const;
 int32 RoundsMade() const{return Rounds;}
 int32 IssuesPrinted() const{return Issues.Num();}
 // The carrier's car on the road past SARO now, and standing at the box.
 bool CarrierDriving() const{return Drive.bOn;}
 bool CarrierStopped() const{return Drive.bOn&&Drive.bStopped;}

 // The carrier: no round on Sundays or on Memorial Day. The round's world clock on a shift day (FS47FreeformSession::Clock): when
 // the car reaches the box.
 static bool RoundOn(const FDateTime& Date);
 static double RoundClock(int32 Seed,int32 ShiftDay);
 // Rounds after the carrier takes an order until the parcel comes: one to three.
 static int32 ShipRounds(int32 Seed,int32 Serial);

 // The box: E on it.
 void OpenBox();
 void OpenCatalog();
 // Sheet: 0 the front page, 1 inside ("Turn the page" goes between them).
 void OpenLedger(int32 No,int32 Sheet=0);
 // D45-24-20261110 (Tom: «trykke på en artikkel med merkelig info og få stedet på kartet»): the reader circles an article and its
 // lead goes on the map as a place heard of (FS47Places::Hear; the wagon by FS47Predecessor::HearOf), with a word to say so.
 // Reading a paper marks nothing by itself any more. True: it was not on the map before. No: the issue (0 for F10's paper).
 bool MarkLead(int32 No,const FString& Lead,const FString& Head);
 // Whether a lead is on the map already, heard of or found: what the page rings in pencil. Nothing of its own is saved.
 bool LeadMarked(const FString& Lead) const;
 // The articles of a printed paper that point somewhere, with their sheets (the tests read them).
 TArray<S47Ledger::FLeadRef> IssueLeads(int32 No) const;
 // What the Caliche counter charges for a supply with this week's coupon clipped (Base without one), and the motel's room.
 int32 CounterPrice(const FString& Item,int32 Base) const;
 int32 MotelPrice(int32 Base) const;
 FString TaskLine() const;
 // Where a player stands to use the box: SARO's area, three-space metres.
 bool WorldSpot(FString& OutArea,double& OutX,double& OutZ) const;

 // F10 and the tests. Nothing here is saved in a developer run.
 void TestOn();
 // The carrier's round now, whatever the hour, made at once (the car is shown coming up to the box after it). Why: what it left.
 bool TestRound(FString& Why);
 // The carrier's car now, from down the highway, as at its hour: it stops at the box if it has something to leave or take, and the
 // round is made when it gets there. False: not in SARO's area.
 bool TestCarrier();
 void TestFlag(bool bUp);
 // As E at the box, without walking there: the box's panel opens.
 bool TestFetch();
 // This week's paper as it would be printed now, from what the station registered (not kept).
 void TestLedger();
 // A parcel of two dry cells on the box now, as if it had been ordered.
 void TestParcel();
 // Something with no sender in the box now: 0 the blank sheet, 1 the map, 2 the postcard.
 void TestNobody(int32 Variant);
 bool TestOrder(const TMap<FString,int32>& Lines,FString& Why);
 // The first parcel on the box lifted down, as its button does. False: none, or the hands are full.
 bool TestLift();
 int32 OrdersOut() const{return Orders.Num();}
 int32 CouponCount() const{return Coupons.Num();}
 const TArray<FPiece>& Contents() const{return Box;}
 // What the station's nights registered, each with the place it was at and its day, as the paper uses them.
 struct FSeen{FString Id,Place,PlaceId;int32 Day=0;};
 const TArray<FSeen>& Noted() const{return Seen;}
 // The facts the paper would print from now, for the week from FromDay to ToDay (the tests read them).
 TArray<S47Ledger::FFact> WeekFacts(int32 FromDay,int32 ToDay) const;
 // The text of a printed paper (empty: no such issue).
 FString IssueText(int32 No) const;
 static bool SelfTest(TArray<FString>& Lines);

 // The catalog (Western Surplus & Radio Supply): a line's goods (a supply of FS47FreeformWorld, or "kit:<id>" for FS47Openings),
 // what it is, and its price by mail.
 struct FLine{const TCHAR* Id;const TCHAR* Title;int32 Price;};
 static const TArray<FLine>& Catalog();
 static constexpr int32 Postage=2,MostPerLine=6,MostPerOrder=12;
private:
 US47Main* M=nullptr;
 TSharedPtr<uint8> Life;
 TUniquePtr<FS47MailBoxModel> Model;
 TUniquePtr<FS47MailCarModel> Car;
 struct FCarSound;
 TUniquePtr<FCarSound> CarSound;
 // The car on the road: its round's day, whether it makes the round when it gets there (a test's show does not), whether it stops.
 struct FDrive{bool bOn=false,bApply=false,bStops=false,bForceStop=false,bStopped=false,bDone=false,bClank1=false,bClank2=false;int32 OnDay=0;double Z=0,X=0,Speed=0,Wait=0,Started=0,StopAt=0,XAtStop=0;};
 FDrive Drive;
 bool bTest=false,bFlag=false,bCatalog=false,bBound=false,bPrimed=false;
 TArray<FPiece> Box,Pending,Fetched;
 TSet<FString> Posted,ReadIssues;
 // Taken out of the box and not yet read (ids of Fetched): Dr. Ward's is read once it has left the OPS tray.
 TSet<FString> Unread;
 // An order: written and left in the box (Picked 0), or picked up by the carrier on that day and due at the round on or after Due.
 struct FOrder{int32 Serial=0,Paid=0,Picked=0,Due=0;TMap<FString,int32> Goods;};
 TArray<FOrder> Orders;
 struct FClip{FString Id,What;int32 Price=0,Until=0;};
 // D48-20261110: left in the box for the carrier (PostOut): taken on the next round, its reply then pending for its due day.
 struct FOut{FString Id,What;int32 Rounds=1;FPiece Reply;};
 TArray<FOut> Outbound;
 TArray<FClip> Coupons;
 // A printed paper: its front page as printed and the templates it used, the sheriff's lines from the week's findings, and whether
 // the trucker's thanks was in it. Photo (D46-1-20261110): the front page's picture of the lead's place from the world, a small grey
 // JPEG data URL; empty until the world could be photographed by day (then the drawn picture is printed).
 struct FKept{int32 No=0,Day=0;bool bThanks=false;TArray<FString> Heads,Bodies,Leads,Keys,Log;TArray<int32> LogAgo;FString Photo;};
 TArray<FKept> Issues;
 TArray<FSeen> Seen;
 int32 SeenCount=-1;
 int32 LastRound=0,Rounds=0,NextSerial=1,NobodySent=0;
 TMap<FString,int32> Cart; // the order form being filled in: never saved
 void Round(int32 OnDay);
 bool RoundBrings(int32 OnDay) const;
 bool StartCar(int32 OnDay,bool bApply,double FromZ,bool bForceStop);
 void StepCar(double Dt);
 void PlaceCar();
 void ApplyRound();
 void EndCar();
 void CarEngine(float Rate,float Gain);
 void Clank(float Gain);
 void NoteFindings();
 bool PlaceOf(const FString& Id,const FString& Text,bool bHere,FString& OutPlace,FString& OutId) const;
 void PrintLedger(int32 OnDay);
 // D46-1-20261110: photograph the lead story's place for the front page: by day, from a few tens of metres, with nothing film-only
 // shown. False (and nothing kept) when it cannot be done now; PhotoTries counts the attempts that rendered and failed.
 bool TakePhoto(FKept& K);
 double PhotoWait=0;
 TMap<int32,int32> PhotoTries;
 bool IssueOf(int32 No,S47Ledger::FIssue& Out) const;
 // The paper's panel: a sheet of the issue, the lead buttons for keyboard and pad, the coupons inside, and the page turn.
 void ShowLedger(const S47Ledger::FIssue& Issue,int32 Sheet,const FString& Title,int32 No);
 int32 NobodyDay(int32 Which) const;
 // Everything but a parcel out of the box onto the desk, not read yet (the pure part of TakeOut): letters first, the catalog last.
 TArray<FPiece> Unbox();
 bool IsUnread(const FPiece& Piece) const;
 void MarkRead(const FString& Id);
 TArray<FPiece> TakeOut();
 void ReadPiece(const FPiece& Piece);
 void FileDoc(const FPiece& Piece);
 bool LiftParcel(const FString& Id);
 void Clip(const FString& CouponId,int32 Until);
 bool Clipped(const FString& CouponId) const;
 int32 LinePrice(const FLine& L) const;
 int32 OrderTotal(const TMap<FString,int32>& Lines,int32& OutPostage) const;
 bool Offered(const FLine& L) const;
 bool WriteOrder(const TMap<FString,int32>& Lines,FString& Why);
 void TakeOrderBack();
 FString DayText(int32 ShiftDay) const;
 FString PieceTitle(const FPiece& Piece) const;
 FString PieceText(const FPiece& Piece) const;
 static FString NobodyText(int32 Variant,const FString& Place);
 static FString SlipText(int32 Serial,const TMap<FString,int32>& Goods,int32 Paid);
 static FString TowText(const FString& Date,int32 Fee,bool bPaidInCash);
 static TArray<FString> AllText();
 bool Build();
};

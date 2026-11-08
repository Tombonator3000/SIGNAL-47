#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class FS47MailBoxModel;
namespace S47Ledger{struct FIssue;struct FFact;}

// MAILBOX-20261108 (Tom: «Inkluder post funksjon til SARO og lag postboks utenfor SARO»). Freeform only. A rural box on a post where
// the service road meets the highway (S47MailBox.cpp), and the post that comes to it: the carrier once a day, Monday to Saturday,
// between noon and half past one, heard on the highway when the player is at the station; the red flag up when there is something in
// the box, seen from the road and from the yard. E at the box takes it out. What comes:
//   ward      Dr. Ward's letter, her Monday sheets, her warning about the lost hours and the assessment (FS47Predecessor::Send).
//             Taken out, they go to the OPS tray as before, where the review and its tests look for them.
//   ledger    THE CALICHE COUNTY LEDGER on Thursdays (S47Ledger): the nights on its front page, coupons, places to look.
//   catalog   Western Surplus & Radio Supply's catalog, in the box on the first day: parts and gear by mail, cheaper than Caliche,
//             ordered with a money order left in the box with the flag up; the carrier takes it, and the parcel comes one to three
//             rounds later. Lifted out, a parcel is in both hands, for GARAGE RECEIVING like any other (FS47FreeformWorld).
//   bill      the weekly statement after each review, and the county's bill for a tow: what was charged, on paper.
//   nobody    twice in a month or so, something with no stamp and no sender. It explains nothing.
// What it opens for the others: the station's stock and kit (parcels), money (orders, coupons at the counter and the motel), places
// on the map and wagon 23 (the paper), and Ward's papers for the review. All of it is saved under "mail" and survives a Continue;
// a save from before it finds the catalog in the box and the flag up. Developer runs have no post unless F10 or a test asks.
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
 // twice ("ward:letter"). Ref: the tray id, the issue, the finding that holds a bill's text. Day: the day it is in the box.
 struct FPiece{FString Id,Kind,Ref,Text;int32 Day=0;TMap<FString,int32> Goods;};
 // Into the box with the next round (bNow: in it already, flag up). False: posted before, or there is no post.
 bool Post(const FPiece& Piece,bool bNow=false);
 // Dr. Ward's paper with this tray id (FS47Predecessor). False: there is no post in this game; it goes to the tray instead.
 bool PostWard(const FString& TrayId,bool bNow);
 // The statement of a weekly review (its finding, FS47FreeformSession::StepWeeklyReview), and the county's bill for a tow.
 void PostWeekStatement(int32 Week);
 void PostTowBill(int32 Fee,bool bPaidInCash);
 bool Holds(const FString& Id) const;
 bool Coming(const FString& Id) const;
 bool Ever(const FString& Id) const{return Posted.Contains(Id);}
 int32 InBox() const{return Box.Num();}
 bool FlagUp() const{return bFlag;}
 int32 RoundsMade() const{return Rounds;}
 int32 IssuesPrinted() const{return Issues.Num();}

 // The carrier: no round on Sundays or on Memorial Day. The round's world clock on a shift day (FS47FreeformSession::Clock).
 static bool RoundOn(const FDateTime& Date);
 static double RoundClock(int32 Seed,int32 ShiftDay);
 // Rounds after the carrier takes an order until the parcel comes: one to three.
 static int32 ShipRounds(int32 Seed,int32 Serial);

 // The box: E on it.
 void OpenBox();
 void OpenCatalog();
 void OpenLedger(int32 No);
 // What the Caliche counter charges for a supply with this week's coupon clipped (Base without one), and the motel's room.
 int32 CounterPrice(const FString& Item,int32 Base) const;
 int32 MotelPrice(int32 Base) const;
 FString TaskLine() const;
 // Where a player stands to use the box: SARO's area, three-space metres.
 bool WorldSpot(FString& OutArea,double& OutX,double& OutZ) const;

 // F10 and the tests. Nothing here is saved in a developer run.
 void TestOn();
 // The carrier's round now, whatever the hour. Why: what it left, or why not.
 bool TestRound(FString& Why);
 void TestFlag(bool bUp);
 // As E at the box, without walking there: the box's panel opens.
 bool TestFetch();
 // This week's paper as it would be printed now, from what the game holds (not kept).
 void TestLedger();
 // A parcel of two dry cells in the box now, as if it had been ordered.
 void TestParcel();
 // Something with no sender in the box now: 0 the blank sheet, 1 the photograph, 2 the postcard.
 void TestNobody(int32 Variant);
 bool TestOrder(const TMap<FString,int32>& Lines,FString& Why);
 // The first parcel in the box lifted out, as its button does. False: none, or the hands are full.
 bool TestLift();
 int32 OrdersOut() const{return Orders.Num();}
 int32 CouponCount() const{return Coupons.Num();}
 const TArray<FPiece>& Contents() const{return Box;}
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
 bool bTest=false,bFlag=false,bCatalog=false,bBound=false;
 TArray<FPiece> Box,Pending,Fetched;
 TSet<FString> Posted,ReadIssues;
 // An order: written and left in the box (Picked 0), or picked up by the carrier on that day and due at the round on or after Due.
 struct FOrder{int32 Serial=0,Paid=0,Picked=0,Due=0;TMap<FString,int32> Goods;};
 TArray<FOrder> Orders;
 struct FClip{FString Id,What;int32 Price=0,Until=0;};
 TArray<FClip> Coupons;
 // A printed paper: its front page as printed, and whether the trucker's thanks was in it.
 struct FKept{int32 No=0,Day=0;bool bThanks=false;TArray<FString> Heads,Bodies,Leads;};
 TArray<FKept> Issues;
 int32 LastRound=0,Rounds=0,NextSerial=1,NobodySent=0;
 TMap<FString,int32> Cart; // the order form being filled in: never saved
 void Round(int32 OnDay,bool bHere);
 void PrintLedger(int32 OnDay);
 bool IssueOf(int32 No,S47Ledger::FIssue& Out) const;
 TArray<S47Ledger::FFact> WeekFacts(int32 FromDay,int32 ToDay) const;
 int32 NobodyDay(int32 Which) const;
 void Truck(bool bStops);
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
 static FString NobodyText(int32 Variant,const FString& Stamped);
 static FString SlipText(int32 Serial,const TMap<FString,int32>& Goods,int32 Paid);
 static TArray<FString> AllText();
 bool Build();
};

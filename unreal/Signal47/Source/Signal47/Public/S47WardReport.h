#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class SWidget;

// WARD-REPORT-20261108 (Tom, 8 November: "ukerapport til Ward der spilleren velger hva som rapporteres; påvirker tillit og vurdering,
// og løgn kan slå tilbake"). After each weekly review a report to Dr. Ward is due at OPS. It is made from the week's night logs
// (FS47NightLog's sheets keep their facts): what was seen and heard, time nobody can account for, equipment, work, and the account.
// The player ticks what goes in. Where a part is left out entirely, the report can say it was in order instead, and that is a lie.
//
// Not a second night log: the sheets are typed by themselves and say what happened; the report is the player's, and says what
// they choose. What Ward already holds decides what she can see through: a night log telexed to her shows a left-out light or a
// "no time lost" for what it is when the report is read; the county's statement shows a false account at the next review; the
// array's own record a false "work on schedule" when a dish was down; time and equipment may come out later, or not. A report sent
// whole raises her trust; a lie found out lowers it twice as far; a week with no report lowers it a little. Trust counts in her
// assessment (FS47Predecessor). She never says what anything was. Freeform only, saved under "wardReport".
class FS47WardReport
{
public:
	explicit FS47WardReport(US47Main* InMain) : M(InMain) {}
	void Begin(const S47J::FObj& Saved);
	void Stop();
	void Step(double Dt);
	S47J::FObj Save() const;
	void OpsRows(TArray<TSharedRef<SWidget>>& Rows);
	void Open(int32 Week);
	bool Send(int32 Week);
	// TELEX-20261109: her office's reply ("telex-wardreport-W") and her query about a report come back on the telex in the control
	// room; torn off there (FS47FreeformSession::TearOffTelex), the reply goes on the report in the notebook too.
	void Received(int32 Week);
	int32 Trust() const { return TrustNow; }
	// M7: what the assessment counts. A report sent whole that her office has not read yet counts as read (the week 4 report
	// comes due on day 29 and is often telexed after office hours, the evening before she assesses).
	int32 TrustAtReview() const;
	int32 SentCount() const;
	int32 CaughtCount() const;

	enum class ECat : uint8 { Seen, Time, Gear, Work, Money, Count };
	struct FItem { ECat Cat = ECat::Seen; int32 Night = 0; double Clock = 0; FString Text; bool bOn = true; };
	struct FReport
	{
		int32 Week = 0, First = 0, Last = 0; // the nights it covers
		double Due = 0, Sent = -1, Read = -1;
		TArray<FItem> Items;
		uint8 Routine = 0;   // parts written as "in order" (bit per ECat) though something happened
		FString Page, Reply;
		int32 Delta = 0;     // what reading it did to her trust
	};
	struct FLie { int32 Week = 0; ECat Cat = ECat::Time; double CatchAt = -1; bool bCaught = false; FString Proof; };
	const TArray<FReport>& Reports() const { return Kept; }
	static int32 TrustAtReview(int32 TrustThen, const TArray<FReport>& Reports);

	// ---------- the pure rules (tested) ----------
	static const TCHAR* CatTitle(ECat C);
	static const TCHAR* RoutineLine(ECat C); // nullptr: this part cannot be written away (what was seen is left out or not)
	// The typed report, from the ticked items and the parts written as in order.
	static FString Type(const FReport& R, const TFunction<FString(int32)>& DateOf);
	// The lies in a sent report: each part written as in order that had something in it.
	static TArray<ECat> LiesIn(const FReport& R);
	static int32 LeftOut(const FReport& R);
	// What trust does to her assessment: a little either way.
	static int32 PointsFor(int32 Trust) { return Trust >= 4 ? 2 : Trust >= 2 ? 1 : Trust <= -4 ? -2 : Trust <= -2 ? -1 : 0; }
	static constexpr int32 MostTrust = 6, MostItems = 8;
	static bool SelfTest(TArray<FString>& Lines);

	// F10
	bool DevDue();
	// The newest unsent report: leave out every item of a part and write it as in order. Its week, or 0.
	int32 DevLie(ECat Part);
	void DevCatchNow();
private:
	US47Main* M = nullptr;
	TArray<FReport> Kept;
	TArray<FLie> Lies;
	int32 TrustNow = 0, LastWeek = -1;
	double Look = 0;
	TSharedPtr<uint8> Life;
	FReport* Find(int32 Week);
	void Make(int32 Week);
	void Receive(double ClockNow);
	void File(const FReport& R) const;
	void Query(const FLie& L);
	FString DateOf(int32 Night) const;
};

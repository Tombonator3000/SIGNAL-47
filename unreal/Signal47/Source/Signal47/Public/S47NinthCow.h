#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"
#include "S47Seen.h"

class FS47NinthModel;

namespace S47Ninth
{
	extern SIGNAL47_API const TCHAR* NINTH_TAG;
	extern SIGNAL47_API const TCHAR* NINTH_PRINT;
}

struct FS47NinthDeps
{
	FS47TownSeen* Seen = nullptr;
	TFunction<FS47NinthModel*()> Site;
	TFunction<bool()> TakeNinth;
	TFunction<void(const FString&)> Note;
	TFunction<void(const FString&, double)> Toast;
	TFunction<void(double)> Hush;
	TFunction<void(const FVector&)> Tick;
	TFunction<double()> Clock;
	TFunction<void()> Save, Refresh;
};

struct FS47NinthIn
{
	bool bOk = false, bFree = false;
	FString Chapter, Area;
	bool bDriving = false, bInHut = false;
	FS47Cam Cam;
	bool bLight = false;
};

// story/ninthCow.ts: the found thing on B inside STATION 01's chained fence, and its board book.
class SIGNAL47_API FS47NinthCow
{
public:
	explicit FS47NinthCow(FS47NinthDeps InD);
	TUniquePtr<FS47RuleBook> Book;
	double Quiet = 0;
	bool bForce = false, bOff = false;
	bool Down() const;
	bool Found() const;
	bool Close() const;
	FString State() const { return Found() ? TEXT("found") : Down() ? TEXT("down") : TEXT("not yet"); }
	TArray<FS47RuleBook*> Books() const;
	void Update(double Dt, const FS47NinthIn& S);
	FString Examine(const FString& Part, const FS47Cam& Cam);
	FString Label() const;
	void Use();
	void Reset();
	void Now() { if (!Down()) bForce = true; }
	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47NinthDeps D;
	double Look = 0;
	TArray<FString> Links;
	bool bInHut = false;
	double TickT = 2;
	int32 Ticks = 0;
	bool bCanFind = false;
	TMap<FString, FS47BookNote> Notes;
	TArray<FS47BookRule> Rules;
	void Ticking(double Dt, FS47NinthModel& Site, double Dist);
	bool InSight(const FS47NinthModel& Site, const FVector& From) const;
	void PutDown(FS47NinthModel& Site, const FS47NinthIn& S);
	bool Unseen(const FS47NinthModel& Site, const FS47NinthIn& S) const;
	void Find();
	void CloseTo();
	void TagNote();
	FString When() const;
	TArray<FS47BoardCard> Cards() const;
	void KeepLinks();
	double Clock() const { return D.Clock ? D.Clock() : 0; }
	bool Has(const TCHAR* Key) const { return D.Seen && D.Seen->Contains(Key); }
};

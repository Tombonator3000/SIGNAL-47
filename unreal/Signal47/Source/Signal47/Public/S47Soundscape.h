#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

class US47Main;
class FS47AudioBuffer;

// SOUNDSCAPE-20261108 (Tom, 8 November): the night as heard, Freeform only. Far, clear sounds from real places, by the place
// and the hour, all through the night's own path (FS47Ambience), so that walls muffle them and a hush stops them:
//
//   the windmills    the farm's and the one on the Baca lease creak as their heads turn, and the pump rod knocks in the tower
//   the wires        the wind in the power line along the highway and its spurs, a low moan and a thin whistle, loudest under them
//   cattle           lowing from the pens and the troughs out on the land, most at dusk
//   coyotes          from the washes and the open land, never at noon, most in the hour before dawn
//   a train          on the line to Roswell, always from the east, miles off
//   owls             where there are barns and trees: the farm, the town, the station
//
// Dogs are FS47Animals' (a dog lives somewhere, and stops). The Story's night is untouched.
//
// A sudden silence is the warning (GAME_BIBLE, Lydens roller): when something real begins to come near (FS47Omen: the first
// sign of a figure, the steps before the light, the quiet before a visit, a craft on its approach), everything out here stops at
// once, seconds before it is there, and stays stopped while it is near. After, the crickets come back one at a time. But the
// crickets stop for a gust too (FS47Omen's ordinary causes), now and then the night misses what comes (FS47Omen::Misses), and the
// night's own lulls are left in: a silence is a reason to listen, never a sure answer.
class SIGNAL47_API FS47Soundscape : public TSharedFromThis<FS47Soundscape>
{
public:
	explicit FS47Soundscape(US47Main* Main) : M(Main) {}

	/** Every frame after FS47Omen. */
	void Step(double Dt);
	/** Out of Freeform: the night is the Story's again. */
	void Reset();

	bool Hushed() const { return bHush; }
	int32 Hushes() const { return HushCount; }
	/** Of those, the ones with something real behind them (the rest were the wind). */
	int32 RealHushes() const { return RealHushCount; }
	int32 Played(const FString& Kind) const { return Counts.FindRef(Kind); }

	/** F10: one of them now, from where it would come from here: windmill, wires, cattle, coyote, train, owl. Why: what stopped it. */
	bool DevPlay(const FString& Kind, FString& Why);
	/** F10: the night goes quiet now, as before something comes, for this long; then it comes back. */
	void DevHush(double Seconds) { DevHushLeft = FMath::Max(0.0, Seconds); }

	// ---------- the pure rules (tested) ----------
	/** How busy a kind is at this hour of the day, 0 for none. */
	static double Activity(const FString& Kind, double Hour);
	/** How far the hush reaches for a source of this reach (the same as FS47Omen's onset). */
	static double HushReach(double SourceReach);
	/** The nearest point of the power line and its spurs, road-local. False beyond Within. */
	static bool NearestWire(const FVector2D& Road, double Within, FVector2D& OutRoad, double& OutDist);
	/** Where the horn comes from on the line to Roswell, road-local: always east of the land. */
	static FVector2D TrainFrom(const FVector2D& Road, float Roll);
	/** The windmills and the cattle, road-local. */
	static const TArray<FVector2D>& Mills();
	static const TArray<FVector2D>& Cattle();
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	bool bHooked = false, bHush = false, bLog = false, bLogChecked = false;
	double HushHold = 0, HushAgain = 0, AfterHush = 0, DevHushLeft = 0, DevWiresLeft = 0;
	int32 HushCount = 0, RealHushCount = 0;
	TMap<FString, double> Due;
	TMap<FString, int32> Counts;
	TArray<TSharedPtr<FS47AudioBuffer>> Moos;
	bool bMoosTried = false;
	void Hook();
	void Unhook();
	/** The Ambience's own far-off ones, placed. bForce: past the hour (F10). */
	bool Place(const FString& Kind, const FVector& Listener, FVector& Out, bool bForce) const;
	bool Road(FVector2D& Out) const;
	FVector Here(const FVector2D& RoadAt, double Up) const;
	double HourNow() const;
	void StepHush(double Dt);
	void StepVoices(double Dt, double Hour);
	bool Moo(const FVector& Listener, const FVector& Where, double Gain);
};

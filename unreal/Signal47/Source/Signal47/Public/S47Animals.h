#pragma once

#include "CoreMinimal.h"

class US47Main;
class AS47Web;
class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class FS47BufferSourceNode;

// ANIMALS-20261108 (Tom, 8 November): animals as warning, Freeform only. They react to what is really near (FS47Omen) and to the
// player, and to nothing else, so that they can be read like a radar:
//
//   quail     a covey sits in the scrub near where the player walks. Come too close and it goes up from under the feet, away from
//             them. When something comes near it goes up with nobody close, and flies away from it: the way they go says where.
//   a coyote  trots across the open land at night, now and then, and looks round at a lamp (its eyes shine). When something comes
//             near it stops dead and stares at it, a few seconds, then runs the other way.
//   the dog   at the farm and in Caliche, barks at night at the player coming and at the coyotes; when something comes near the dog
//             it stops in the middle of a bark, whines once, and stays quiet until long after it has gone.
//   cattle    the Alderete herd (S47Herd) walks off together, all the same way, from something some way off; close by they scatter
//             and bellow (FS47FarmNight reads HerdFear).
//
// Made of the engine's cube and sphere with the game's lit material (no animal models in the project); small and dark at night,
// the eyes the one thing that shows.
class SIGNAL47_API FS47Animals
{
public:
	explicit FS47Animals(US47Main* Main) : M(Main) {}
	~FS47Animals();

	/** Every frame after FS47Omen (game time). */
	void Step(double Dt);
	/** Out of Freeform, another area, a save opened: all of them gone from view. */
	void Reset();

	/** What the herd feels of anything near it now (0 to 1, S47Herd's alarm: below S47Herd::SCATTER they walk off together, above
	 *  it they run), and from where, three-space in the herd's area. */
	double HerdFear(const FVector& HerdMiddle, FVector& OutFrom) const;

	// ---------- tests, F10 ----------
	int32 Flushes() const { return FlushCount; }
	int32 FlushesFromDanger() const { return DangerFlushes; }
	int32 Stares() const { return StareCount; }
	int32 DogBarks() const { return BarkCount; }
	int32 DogStops() const { return StopCount; }
	bool CoyoteOut() const { return Coyote.Mode != ECoyote::None; }
	bool CoveyHere() const { return Covey.Mode == ECovey::Sitting; }
	/** F10: a covey here, 14 m ahead; with bThreat, then something behind the player, so it goes up away from it. Why: what stopped it. */
	bool DevCovey(FString& Why, bool bThreat = true);
	/** F10: a coyote crossing 32 m ahead; with bThreat, then something to one side, so it stops and stares at it. */
	bool DevCoyote(FString& Why, bool bThreat = true);
	/** F10: the nearest dog barks now; then something comes near it, so it stops. */
	bool DevDog(FString& Why);

	// ---------- the pure rules (tested) ----------
	/** Flat, away from From, from At. */
	static FVector FleeDir(const FVector& From, const FVector& At);
	/** How close the player may come before the covey goes up: nearer keeping low and slow, further running. */
	static double FlushRange(double Sneak, bool bRunning);
	/** What a creature at a place feels of a sense level: 0 nothing, 1 uneasy (the herd walks off, the coyote stops), 2 afraid
	 *  (it runs, the covey goes up, the dog stops). */
	static int32 Fear(double Sense);
	/** S47Herd's alarm for a sense level: nothing, a walk together below SCATTER, a run above it. */
	static double HerdLevel(double Sense);
	/** How long the dog keeps quiet after the last of it. */
	static double DogQuiet(double Sense);
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	TWeakObjectPtr<AS47Web> Scene;
	TWeakObjectPtr<USceneComponent> Root;
	TWeakObjectPtr<UMaterialInstanceDynamic> Feather, Fur, EyeGlow;
	bool bBuilt = false, bLog = false, bLogChecked = false;
	FString LastArea;
	int32 FlushCount = 0, DangerFlushes = 0, StareCount = 0, BarkCount = 0, StopCount = 0;

	// the covey
	enum class ECovey : uint8 { None, Sitting, Flying, Gone };
	static constexpr int32 Quail = 9;
	struct FBird { TWeakObjectPtr<UStaticMeshComponent> Body, WingL, WingR; FVector From = FVector::ZeroVector, Dir = FVector::ForwardVector; double Speed = 12, Lift = 2, Phase = 0; };
	struct FCovey
	{
		ECovey Mode = ECovey::None;
		FVector At = FVector::ZeroVector;
		double Age = 0, Wait = 20;
		FBird Birds[Quail];
	} Covey;

	// the coyote
	enum class ECoyote : uint8 { None, Trot, Stare, Bolt, Look };
	struct FCoyote
	{
		ECoyote Mode = ECoyote::None;
		FVector At = FVector::ZeroVector, Dir = FVector::ForwardVector, LookAt = FVector::ZeroVector;
		double Age = 0, Left = 0, Wait = 90, Gait = 0, Ground = 0, GroundIn = 0, Yaw = 0, HeadYaw = 0, Shine = 0;
		bool bLooked = false;
		TWeakObjectPtr<UStaticMeshComponent> Torso, Neck, Head, Snout, EarL, EarR, Tail, EyeL, EyeR, Legs[4];
	} Coyote;

	// the dogs: where they live, road-local, and what each is doing
	struct FDog { FVector2D Home = FVector2D::ZeroVector; double Next = 20, Quiet = 0, BarkIn = 0; int32 Left = 0; bool bStopped = false, bFrantic = false; TSharedPtr<FS47BufferSourceNode> Playing; };
	TArray<FDog> Dogs;

	// F10: what it is to react to once it is in place
	double DevCoveyIn = -1, DevCoyoteIn = -1, DevDogIn = -1;
	FVector DevDogAt = FVector::ZeroVector;

	bool Build();
	void Show(UStaticMeshComponent* Part, bool bOn) const;
	FVector LastFeet = FVector::ZeroVector;
	double FeetSpeed = 0, DangerSpawnIn = 0;
	int32 LastCoyoteCalls = 0;
	void Put(UStaticMeshComponent* Part, const FVector& At, const FRotator& Rot, const FVector& Size) const;
	double GroundAt(const FVector& At) const;
	FVector EyeNow() const;
	bool Outdoors() const;
	bool NightNow() const;
	double Hour() const;
	bool RoadOf(const FVector& At, FVector2D& Out) const;
	FVector FromRoad(const FVector2D& RoadAt, double Up) const;

	void StepCovey(double Dt);
	void PlaceCovey(const FVector& Near, const FVector& Ahead);
	void Flush(const FVector& From, bool bDanger);
	void PoseBirds();

	void StepCoyote(double Dt);
	bool StartCrossing(bool bAhead, double Dist);
	void PoseCoyote(double Dt);
	double ShineOf(const FVector& EyeAt, const FVector& Facing) const;

	void StepDogs(double Dt);
	void Bark(FDog& Dog, bool bFrantic);
};

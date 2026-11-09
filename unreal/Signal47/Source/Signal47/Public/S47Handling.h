#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

class US47Main;
class UStaticMeshComponent;
class UMaterialInterface;
class UPrimitiveComponent;

// PHYSICS-20261108 (Tom, 8 November): picking up, carrying and throwing loose things in Freeform. The parcel from the shop keeps
// its own carry (FS47FreeformCarry); this is everything else: a stone off the ground wherever the player stands outdoors, and the
// loose things at SARO (a coffee can, a cardboard box, a wooden crate in the garage, a few stones by the door).
//
// What the player notices: a thing in the hand hangs behind it as the view turns and the feet move, a light one quickly, a heavy one
// slowly and low in both arms, and a heavy one slows the walk. The throw key held draws the arm back; let go, it is thrown as hard as
// it was held long (a crate only heaved a step or two). Tapped, or Use with nothing else in view, the thing is set down steady and
// upright on what is in front of you, floor or table. Each impact sounds by what hit what and how hard (existing recordings: gravel,
// concrete and wood steps, the metal door, the soft thud), and nothing flies through a wall: each move is a swept box against the
// world, and the walk zones stand in for a wall the sweep did not see.
//
// The consequence: an impact loud enough is posted to S47Noise where it happened, and what listens hears it there (the small ones
// go to look; a visit turns to it), so a stone thrown off to the side draws them away from you. Nothing of it is saved: the things
// at SARO are back in their places when a save is opened, and a stone taken off the ground is only a stone.
//
// STONE-POCKET-20261109 (Tom, 9 November): a jacket pocket for three stones. The throw key tapped with a stone in the hand puts it
// in the pocket (looking at bare ground, the next one comes up off it at once); empty-handed, the key brings one out of the pocket
// before it looks for one on the ground. The player has it without a number on the screen: the stones knock together, quietly, when
// running or jumping with them, and the notebook's kit has a line for them ("Two stones in your pocket."). Not saved: the pocket is
// empty when a save is opened and when a new day begins.
class SIGNAL47_API FS47Handling
{
public:
	enum class EKind : uint8 { Stone, Can, Box, Crate, Count };
	enum class ESurface : uint8 { Dirt, Hard, Wood, Metal, Soft };
	struct FKindDef
	{
		const TCHAR* Id; const TCHAR* Name;
		double Mass;          // kg
		FVector Half;         // the box round it, half extents, centimetres, in its own (Unreal) axes
		uint32 Colour; float Rough;
		double Bounce, Grip;  // restitution, and how much of the sliding speed a contact takes
		double Loud;          // times a stone's loudness at the same speed
		bool bBlocksSight;    // big enough to stand in a line of sight (and in a thrown stone's way)
	};
	static const FKindDef& Def(EKind K);

	explicit FS47Handling(US47Main* Main) : M(Main) {}
	~FS47Handling();

	void Begin();
	void Stop();
	void Step(double Dt);

	bool Holding() const { return Held >= 0; }
	bool Charging() const { return bDown && Held >= 0 && !bPressTaken; }
	FString HeldName() const;
	/** The prompt with nothing else in view while something is held (E sets it down; the walker offers "phys:put"). */
	FString PutLabel() const;
	bool Put();

	// ---------- the pure rules (tested) ----------
	/** How much of a full throw, 0 to 1, after the key was held this long (a tap is none). */
	static double ChargeOf(double HeldSeconds);
	/** Metres a second out of the hand: a stone from a lob to some fourteen, a crate only heaved. */
	static double ThrowSpeed(double Mass, double Charge);
	/** How quickly the thing in the hand follows it (the spring's rate, 1/s): fast for a stone, slow for a crate. */
	static double FollowRate(double Mass);
	/** The walk with it in the arms, times the usual. */
	static double WalkScale(double Mass);
	/** How loud an impact is for whatever listens (S47Noise; heard out to ten metres times this). */
	static double Loudness(EKind Kind, ESurface Surface, double ImpactMps);
	static const TCHAR* SurfaceName(ESurface S);
	/** Drawn and solid: a hidden proxy or a see-through light shaft is in the Visibility channel too, and is no wall or floor. */
	static bool Solid(const UPrimitiveComponent* C);
	static bool SelfTest(TArray<FString>& Lines);

	// ---------- the pocket (STONE-POCKET-20261109) ----------
	static constexpr int32 PocketSize = 3;
	/** What the throw key does pressed with nothing in the hand. */
	enum class EEmpty : uint8 { PickUpInView, FromPocket, OffTheGround };
	static EEmpty EmptyHanded(bool bThingInView, int32 InPocket);
	/** What a tap of the throw key does with this in the hand (held longer, it is thrown as ever). */
	enum class ETap : uint8 { IntoPocket, SetDown };
	static ETap Tapped(EKind InHand, int32 InPocket);
	/** The kit's line for it, in words: "Two stones in your pocket."; empty for none. */
	static FString PocketLine(int32 InPocket);
	/** Whether the stones knock together now: some in the pocket, and running (faster than the walk by a quarter) or in the air. */
	static bool Rattles(int32 InPocket, double Mps, double WalkMps, bool bInAir);
	int32 PocketCount() const { return PocketItems.Num(); }
	/** The stone in the hand into the pocket, if there is room. */
	bool PocketHeld();
	/** A stone out of the pocket into the empty hand. */
	bool TakeOut();

	// ---------- for F10 and the tests ----------
	struct FView { EKind Kind = EKind::Stone; FString Id, Area; FVector At = FVector::ZeroVector; bool bHeld = false, bFlying = false, bResting = true; int32 Impacts = 0; double Speed = 0; };
	int32 Num() const { return Items.Num(); }
	FView View(int32 Index) const;
	/** A thing of this kind at At (three-space, its bottom on the ground there), in this area; its index. */
	int32 Spawn(EKind Kind, const FVector& At, double YawRad, const FString& Area, bool bLoose);
	bool PickUp(int32 Index);
	/** A stone off the ground in view, if the player stands outdoors looking at bare ground within reach. */
	bool PickStone();
	/** Throw what is held now, this much of a full throw, along the view. */
	bool ThrowNow(double Charge);
	int32 HeldIndex() const { return Held; }
	int32 IndexOf(const FString& Id) const;
	/** The loudest impact since the last throw (or the last one, between throws), in words. */
	FString LastImpact() const { return LastHit; }
	/** How loud that loudest impact was (S47Noise's scale); negative before any. */
	double ThrowLoudness() const { return ThrowLoud; }
	int32 ImpactCount() const { return Hits; }

private:
	enum class EState : uint8 { Rest, Held, Fly };
	struct FItem
	{
		EKind Kind = EKind::Stone;
		FString Id, Area;
		TWeakObjectPtr<UStaticMeshComponent> Comp;
		TArray<TWeakObjectPtr<UStaticMeshComponent>> Parts;
		int32 Node = -1;
		EState State = EState::Rest;
		FVector Loc = FVector::ZeroVector, Vel = FVector::ZeroVector, Spin = FVector::ZeroVector; // Unreal: cm, cm/s, rad/s
		FVector Half = FVector(5, 5, 5); // its own box (a stone each its own size), cm
		FQuat Q = FQuat::Identity;
		FVector HomeLoc = FVector::ZeroVector; FQuat HomeQ = FQuat::Identity;
		double Air = 0, LastHitAt = -10, Far = 0, Age = 0;
		int32 Impacts = 0;
		bool bLoose = false, bWasWalkable = true;
	};
	US47Main* M = nullptr;
	TSharedPtr<uint8> Life = MakeShared<uint8>(0);
	TArray<FItem> Items;
	int32 Held = -1, Hits = 0, StoneCount = 0;
	bool bBuilt = false, bDown = false, bPressTaken = false, bOwnSpeed = false, bPutAdded = false;
	double DownFor = 0, Clock = 0, BaseSpeed = 1.9, AppliedSpeed = 1.9, ThrowLoud = -1;
	FString LastHit;
	UMaterialInterface* Base = nullptr;
	// the pocket: the stones in it, by index (hidden, in no area); the walk measured for their knocking
	TArray<int32> PocketItems;
	int32 PocketDay = -1, Knocks = 0;
	bool bFromPocket = false, bPocketPos = false, bPocketAir = false;
	double KnockDist = 0;
	FVector PocketPos = FVector::ZeroVector;
	void EmptyPocket();
	void StepPocket(double Dt);

	bool Live() const;
	void Build();
	void AddUse(int32 Index);
	void Show(FItem& It) const;
	void Collide(FItem& It, bool bWorld) const;
	void Apply(FItem& It) const;
	void StepHeld(FItem& It, double Dt);
	void StepFly(FItem& It, double Dt);
	void Settle(FItem& It, const FVector& SurfaceNormal);
	void Impact(FItem& It, const FHitResult* Hit, const FVector& Normal, double Mps);
	ESurface SurfaceOf(const FHitResult* Hit, const FVector& AtUE, const FVector& Normal) const;
	double FloorUnder(const FVector& AtUE) const; // cm, Unreal Z; NaN: none known
	double HalfUp(const FItem& It, const FVector& Up = FVector::UpVector) const;
	bool Walkable(const FVector& AtUE) const;
	FCollisionQueryParams Query(const FItem* Self) const;
	bool Trace(FHitResult& H, const FVector& A, const FVector& B, FCollisionQueryParams P) const;
	bool Sweep(FHitResult& H, const FVector& A, const FVector& B, const FQuat& Q, const struct FCollisionShape& S, FCollisionQueryParams P) const;
	void Release(bool bRestoreSpeed);
};

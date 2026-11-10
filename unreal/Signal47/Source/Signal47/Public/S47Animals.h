#pragma once

#include "CoreMinimal.h"

class US47Main;
class AS47Web;
class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class FS47BufferSourceNode;
class FS47AudioBuffer;
struct FReferenceSkeleton;

// ANIMALS-20261108 (Tom, 8 November): animals as warning, Freeform only. They react to what is really near (FS47Omen), to the
// player, and to what animals react to anyway (a gust, a sound nobody else heard: FS47Omen's ordinary causes), so that a careful
// player reads them as a hint and never as an answer:
//
//   quail     a covey sits in the scrub near where the player walks. Come too close and it goes up from under the feet, away from
//             them. When something comes near it goes up with nobody close, and flies away from it: the way they go says where.
//             A gust can put them up too.
//   a coyote  is out on the open land at night now and then (every ten to twenty-five minutes, drawn from the seed), off to one
//             side and some 18 to 30 m away, trotting its own way, stopping to nose the ground; the lamp or the headlights find it
//             and its eyes shine when it looks round. When something comes near, the one that is out stops where it is, glances
//             at the light, huffs once and stares off toward it (give or take), then runs the other way. Now and then one stops
//             and stares at nothing.
//   the dog   at the farm and in Caliche, barks at night at the player coming and at the coyotes; when something comes near the dog
//             it stops in the middle of a bark, whines once, and stays quiet until long after it has gone. Sometimes it just quits.
//   cattle    the Alderete herd (S47Herd) walks off together, all the same way, from something some way off; close by they scatter
//             and bellow (FS47FarmNight reads HerdFear).
//
// The coyote and the dog are the rigged CC0 models (Tools/animals, /Game/Free/Animals); the quail, and the coyote where a build has
// no model, are the engine's cube and sphere with the game's lit material. Each reacts on FS47Omen's Animals reader, which now and
// then misses a real source (FS47Omen::Misses).
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
	/** Of those, the ones at nothing (FS47Omen's ordinary causes), and the coveys a gust put up. */
	int32 StaresAtNothing() const { return OrdinaryStares; }
	int32 FlushesFromWind() const { return OrdinaryFlushes; }
	int32 DogBarks() const { return BarkCount; }
	int32 DogStops() const { return StopCount; }
	bool CoyoteOut() const { return Coyote.Mode != ECoyote::None; }
	bool CoveyHere() const { return Covey.Mode == ECovey::Sitting; }
	/** The coyote and the dog are the rigged models (Tools/animals), not the boxes. */
	bool RiggedCoyote() const { return CoyoteRig.bReady; }
	bool RiggedDog() const { return DogRig.bReady; }
	/** F10: with bThreat a covey 14 m ahead, then something behind the player, so it goes up away from it; without, a covey right
	 *  in front of the player's feet (and nothing made up near), so it goes up from them. Why: what stopped it. */
	bool DevCovey(FString& Why, bool bThreat = true);
	/** F10: a coyote crossing 32 m ahead; with bThreat, then something to one side, so it stops and stares at it. */
	bool DevCoyote(FString& Why, bool bThreat = true);
	/** F10: the nearest dog barks now; then something comes near it, so it stops. */
	bool DevDog(FString& Why);
	/** N1110 (ANIMALS-20261108): a warning a few seconds before an event, from FS47AnimalWarn. Out on foot only. bBirds: a covey goes
	 *  up out of the brush near the player all at once, away from Threat (three-space, a point in the way of what comes); else
	 *  (night only, off the towns and the buildings) a coyote 18 to 30 m off stops and stares at the player for Lead seconds, then
	 *  runs. False with Why: no cover or room here, one is out already, not outdoors. */
	bool Warn(bool bBirds, const FVector& Threat, double Lead, FString& Why);

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
	/** A rig's bone, as the glTF names it ("FF.L") or as Unreal imported it ("FF_L"): every spelling to look for, in turn. */
	static TArray<FName> BoneSpellings(const TCHAR* Bone);
	/** The head and the four paws of a quadruped rig (the IK feet, else the lower legs), in component space of the ref pose. */
	static bool FindRigBones(const FReferenceSkeleton& Ref, FVector& OutHead, FVector OutPaws[4], FName& OutHeadName);
	/** Where a coyote stares for something at Source: toward it, off by up to SpreadDeg either way (Roll 0 to 1). */
	static FVector StareToward(const FVector& CoyoteAt, const FVector& Source, double SpreadDeg, float Roll);
	/** Seconds until the next coyote is out, drawn from the seed and the night and how many have been out: ten to twenty-five minutes. */
	static double CoyoteWait(int32 Seed, int32 Night, int32 Count);
	/** Where a coyote is first seen on foot: off to one side of where the player looks (50 to 130 degrees), 18 to 30 m out. */
	static FVector CoyoteStart(const FVector& Eye, const FVector& Forward, float RollSide, float RollAngle, float RollDist);
	static constexpr double StareSpread = 25;
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	TWeakObjectPtr<AS47Web> Scene;
	TWeakObjectPtr<USceneComponent> Root;
	TWeakObjectPtr<UMaterialInstanceDynamic> Feather, Fur, EyeGlow;
	bool bBuilt = false, bLog = false, bLogChecked = false;
	FString LastArea;
	int32 FlushCount = 0, DangerFlushes = 0, StareCount = 0, BarkCount = 0, StopCount = 0, OrdinaryStares = 0, OrdinaryFlushes = 0, CoyotesOut = 0;

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
	enum class ECoyote : uint8 { None, Trot, Stare, Bolt, Look, Sniff };
	struct FCoyote
	{
		ECoyote Mode = ECoyote::None;
		FVector At = FVector::ZeroVector, Dir = FVector::ForwardVector, LookAt = FVector::ZeroVector, StareAt = FVector::ZeroVector;
		double Age = 0, Left = 0, Wait = -1, Gait = 0, Ground = 0, GroundIn = 0, Yaw = 0, HeadYaw = 0, Shine = 0, Glance = 0, SniffIn = 8, Out = 0;
		bool bLooked = false, bAtNothing = false;
		TWeakObjectPtr<UStaticMeshComponent> Torso, Neck, Head, Snout, EarL, EarR, Tail, EyeL, EyeR, Legs[4];
	} Coyote;

	// the dogs: where they live, road-local, and what each is doing
	struct FDog { FVector2D Home = FVector2D::ZeroVector; double Next = 20, Quiet = 0, BarkIn = 0; int32 Left = 0; bool bStopped = false, bFrantic = false; FVector Feared = FVector::ZeroVector; TSharedPtr<FS47BufferSourceNode> Playing; };
	TArray<FDog> Dogs;

	// The rigged models (Tools/animals, CC0 from Quaternius, imported to /Game/Free/Animals): a coyote and a ranch dog that walk,
	// run and stand. Without them the coyote is the boxes above and the dog is only heard. Each is scaled to its height and turned
	// by its own skeleton (Head ahead of Body), whatever axes the import gave it.
	struct FRig
	{
		FString Animal;
		TWeakObjectPtr<class USkeletalMeshComponent> Mesh;
		FString Playing;
		double Scale = 1, YawOffset = 0, Lift = 0, Rate = 1;
		// from the model's own frame (whatever the import's axes) to standing up, facing +X; and its feet, in that frame
		FQuat Fix = FQuat::Identity;
		FVector Feet = FVector::ZeroVector;
		FName Head;
		bool bReady = false, bTried = false;
		// PT-FARM-GROUND-20261109: its own eyes (a copy of the model's own eye material: the game's lit material has no skeletal
		// usage and drew the whole animal in the engine's grey), and what BuildRig found, for F10 and the log
		TWeakObjectPtr<UMaterialInstanceDynamic> Eye;
		FString Note;
	};
	FRig CoyoteRig, DogRig;
	int32 DogShown = INDEX_NONE;
	double DogYaw = 0;
	/** A rig's coat, pale, nose: base colour and roughness for a copy of the slot's own (glTF) material. */
	struct FCoat { FLinearColor Colour = FLinearColor::Gray; float Rough = .9f; };
	bool BuildRig(FRig& Rig, const TCHAR* Animal, double HeightCm, USceneComponent* Holder, const FCoat& Coat, const FCoat& Pale, const FCoat& Nose);
	/** The coyote's eye shine on the boxes' eyes and on each rig's own eyes. */
	void SetEyeShine(double Shine);
	/** The skeleton and the mesh agree: the paws at the foot of the mesh and the head inside it, along the skeleton's up. False
	 *  (with Why) when the import put the mesh in another frame from its bones, as the Quaternius glTF can: skinned, it then
	 *  sweeps through the ground. Pure, for the self-test. */
	static bool RigFramesAgree(double PawsUp, double HeadUp, double Sole, double Tall, FString& Why);
public:
	/** F10/log: how each rig was built (rigged, or why not), and whether its materials can draw on a skeletal mesh. */
	FString RigReport() const;
private:
	void PlayRig(FRig& Rig, const TCHAR* Clip, double Pace);
	void PlaceRig(FRig& Rig, const FVector& FeetThree, double YawDeg);
	void StepDogBody(double Dt);

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
	enum class EFlush : uint8 { Player, Danger, Wind };
	void Flush(const FVector& From, EFlush Why);
	void PoseBirds();

	void StepCoyote(double Dt);
	bool StartCrossing(bool bAhead, double Dist);
	bool StartOut(double StareFor = 0);
	void BeginStare(const FVector& Toward, bool bAtNothing);
	void PoseCoyote(double Dt);
	double ShineOf(const FVector& EyeAt, const FVector& Facing) const;
	// the coyote's one huff as it stops: made here once, from the game's own noise, and played from where it stands
	TSharedPtr<FS47AudioBuffer> HuffRec;
	void Huff(const FVector& At);

	void StepDogs(double Dt);
	void Bark(FDog& Dog, bool bFrantic);
};

// One believable person: an assembled MetaHuman driven natively (no Animation Blueprint of ours).
// The body carries everything: idle, gestures, look-at, and the face control curves. The
// MetaHuman face copies body pose and curves (ABP_Face) and its post-process runs RigLogic.
#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/ActorComponent.h"
#include "S47Person.generated.h"

class UAnimSequence;
class USkeletalMeshComponent;

// Game-thread state handed to the animation worker once per update.
struct FS47PersonPose
{
	const UAnimSequence* Idle = nullptr;    // body loop, breathing included
	float IdleTime = 0;
	const UAnimSequence* Gesture = nullptr; // optional full-body clip over the idle
	float GestureTime = 0, GestureWeight = 0;
	const UAnimSequence* IdleFace = nullptr; // quiet facial micro-movement loop (curves only)
	float IdleFaceTime = 0;
	const UAnimSequence* Talk = nullptr;     // audio-driven take (curves only)
	float TalkTime = 0, TalkWeight = 0;
	// Degrees in her own frame: yaw positive to her left, pitch positive up.
	float LookYaw = 0, LookPitch = 0;        // spine to head share of the gaze
	float EyeYaw = 0, EyePitch = 0;          // the rest, carried by the eyes
	float Blink = 0;                         // 0 open, 1 closed
	float Breath = 0;                        // -1..1, a slow chest swell under the idle
};

struct FS47PersonAnimProxy : public FAnimInstanceProxy
{
	using FAnimInstanceProxy::FAnimInstanceProxy;
	FS47PersonPose In;
	virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;
private:
	void Aim(FPoseContext& Output) const;
	void Face(FPoseContext& Output) const;
};

UCLASS(Transient, NotBlueprintable)
class SIGNAL47_API US47PersonAnim : public UAnimInstance
{
	GENERATED_BODY()
public:
	FS47PersonPose Pose;
	// Keeps every sequence the worker samples alive.
	UPROPERTY() TArray<TObjectPtr<UAnimSequence>> Held;
protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FS47PersonAnimProxy(this); }
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override { delete Proxy; }
};

// Behaviour on top of an assembled MetaHuman actor: attention, gaze, blinking, speech.
UCLASS()
class SIGNAL47_API US47PersonComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	US47PersonComponent();
	// Clip id and seconds into it while this person's voice is audible, on the audio clock.
	TFunction<bool(FString& Clip, double& Seconds)> Speech;
	// Where the listener's eyes are (world), when there is one worth looking at.
	TFunction<TOptional<FVector>()> Listener;
	// Places she looks at when nobody holds her attention (world).
	TArray<FVector> IdleSpots;
	// False hides her and pauses the behaviour (the place she belongs to is not shown).
	TFunction<bool()> Present;
	FString TalkPath;          // object path with {take}, e.g. .../FaceAnims/AS_{take}.AS_{take}
	bool Setup(UAnimSequence* InIdle, UAnimSequence* InIdleFace);
	FString Describe() const;
	virtual void TickComponent(float Dt, ELevelTick Tick, FActorComponentTickFunction* Fn) override;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;
	UPROPERTY() TObjectPtr<US47PersonAnim> Anim;
private:
	UPROPERTY() TMap<FString, TObjectPtr<UAnimSequence>> Takes;
	UAnimSequence* Take(const FString& Clip);
	FVector2D Gaze = FVector2D::ZeroVector, GazeVel = FVector2D::ZeroVector; // smoothed yaw/pitch
	FVector2D Saccade = FVector2D::ZeroVector;
	double Clock = 0, NextBlink = 1.5, BlinkAt = -1, NextSpot = 0, NextSaccade = 0;
	int32 Spot = INDEX_NONE;
	bool bSecondBlink = false;
	FString LastClip;
	FVector2D Aim(const FVector& World) const;
};

class AS47Web;
// Mesa Diner: replaces the original waitress figure when the local MetaHuman assets exist.
namespace S47Waitress { void Attach(AS47Web* W); }

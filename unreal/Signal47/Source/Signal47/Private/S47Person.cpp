// A native MetaHuman driver: one proxy on the body samples idle, gesture and face takes, turns
// spine to head toward what she looks at, and writes the face rig's control curves. ABP_Face
// copies body pose and curves; its post-process (RigLogic, head-movement IK) does the rest.
#include "S47Person.h"
#include "AnimationRuntime.h"
#include "BonePose.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Person, Log, All);
namespace S47PersonReview { void Tick(US47PersonComponent* Person, float Dt); }

namespace
{
 // Share of the gaze turned by each bone, spine to head (yaw sums to 1, pitch stays high).
 struct FShare { const TCHAR* Bone; float Yaw, Pitch, Breath; };
 const FShare Chain[] = {{TEXT("spine_03"),.06f,0,.25f},{TEXT("spine_04"),.10f,0,.35f},{TEXT("spine_05"),.14f,.05f,.40f},
  {TEXT("neck_01"),.22f,.25f,0},{TEXT("neck_02"),.22f,.30f,0},{TEXT("head"),.26f,.40f,0}};
 constexpr float BreathDegrees = .55f;
 // Degrees of eye turn at curve value 1 (MetaHuman eye rig, approximately).
 constexpr float EyeYawRange = 32, EyePitchRange = 24;
 const FName LookLeft[2] = {TEXT("CTRL_expressions_eyeLookLeftL"), TEXT("CTRL_expressions_eyeLookLeftR")};
 const FName LookRight[2] = {TEXT("CTRL_expressions_eyeLookRightL"), TEXT("CTRL_expressions_eyeLookRightR")};
 const FName LookUp[2] = {TEXT("CTRL_expressions_eyeLookUpL"), TEXT("CTRL_expressions_eyeLookUpR")};
 const FName LookDown[2] = {TEXT("CTRL_expressions_eyeLookDownL"), TEXT("CTRL_expressions_eyeLookDownR")};
 const FName Blinks[2] = {TEXT("CTRL_expressions_eyeBlinkL"), TEXT("CTRL_expressions_eyeBlinkR")};

 FAnimExtractContext At(float Time, bool bLoop) { return FAnimExtractContext(double(Time), false, FDeltaTimeRecord(), bLoop); }
 float LoopTime(float Time, const UAnimSequence* Sequence)
 {
  const float Length = Sequence ? Sequence->GetPlayLength() : 0.f;
  return Length > SMALL_NUMBER ? FMath::Fmod(FMath::Max(0.f, Time), Length) : 0.f;
 }

 // A turn in her own frame: pitch toward Up first, then yaw toward Left, both about her chest.
 FQuat Turn(const FVector& Forward, const FVector& Left, float YawDeg, float PitchDeg)
 {
  const float Y = FMath::DegreesToRadians(YawDeg), P = FMath::DegreesToRadians(PitchDeg);
  const FQuat Pitch = FQuat::FindBetweenNormals(Forward, (Forward * FMath::Cos(P) + FVector::UpVector * FMath::Sin(P)).GetSafeNormal());
  const FQuat Yaw = FQuat::FindBetweenNormals(Forward, (Forward * FMath::Cos(Y) + Left * FMath::Sin(Y)).GetSafeNormal());
  return Yaw * Pitch;
 }
}

void FS47PersonAnimProxy::PreUpdate(UAnimInstance* Instance, float DeltaSeconds)
{
 FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
 In = CastChecked<US47PersonAnim>(Instance)->Pose;
}

bool FS47PersonAnimProxy::Evaluate(FPoseContext& Output)
{
 FAnimationPoseData Out(Output);
 if (In.Idle) In.Idle->GetAnimationPose(Out, At(In.IdleTime, true));
 else Output.ResetToRefPose();
 if (In.Gesture && In.GestureWeight > ZERO_ANIMWEIGHT_THRESH)
 {
  FPoseContext Gesture(Output);
  FAnimationPoseData GestureData(Gesture);
  In.Gesture->GetAnimationPose(GestureData, At(In.GestureTime, false));
  FAnimationRuntime::BlendTwoPosesTogetherInPlace(Out, GestureData, 1.f - In.GestureWeight);
 }
 Aim(Output);
 Face(Output);
 return true;
}

void FS47PersonAnimProxy::Aim(FPoseContext& Output) const
{
 const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
 const FReferenceSkeleton& Ref = Bones.GetReferenceSkeleton();
 auto Index = [&](const TCHAR* Name)
 {
  const int32 Mesh = Ref.FindBoneIndex(FName(Name));
  return Mesh == INDEX_NONE ? FCompactPoseBoneIndex(INDEX_NONE) : Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Mesh));
 };
 const FCompactPoseBoneIndex ThighL = Index(TEXT("thigh_l")), ThighR = Index(TEXT("thigh_r"));
 if (ThighL == INDEX_NONE || ThighR == INDEX_NONE) return;
 FCSPose<FCompactPose> Space; Space.InitPose(Output.Pose); // a copy: the original pose stays readable while we edit
 // Her frame from the hips, which the look never moves: up, her left, and the facing between.
 FVector Left = Space.GetComponentSpaceTransform(ThighL).GetLocation() - Space.GetComponentSpaceTransform(ThighR).GetLocation();
 Left = (Left - FVector::UpVector * (Left | FVector::UpVector)).GetSafeNormal();
 if (Left.IsNearlyZero()) return;
 const FVector Forward = FVector::UpVector ^ Left;
 for (const FShare& S : Chain)
 {
  const FCompactPoseBoneIndex B = Index(S.Bone); if (B == INDEX_NONE) continue;
  const FCompactPoseBoneIndex P = Bones.GetParentBoneIndex(B); if (P == INDEX_NONE) continue;
  const FQuat Parent = Space.GetComponentSpaceTransform(P).GetRotation();
  const FQuat Delta = Turn(Forward, Left, In.LookYaw * S.Yaw, In.LookPitch * S.Pitch + In.Breath * BreathDegrees * S.Breath);
  FTransform& Local = Output.Pose[B];
  Local.SetRotation((Parent.Inverse() * Delta * Parent * Local.GetRotation()).GetNormalized());
 }
}

void FS47PersonAnimProxy::Face(FPoseContext& Output) const
{
 // Quiet face under the spoken take, cross-faded by weight.
 FBlendedCurve Mix;
 auto Add = [&](const UAnimSequence* Seq, float Time, float Weight, bool bLoop)
 {
  if (!Seq || Weight <= ZERO_ANIMWEIGHT_THRESH) return;
  FBlendedCurve C; Seq->EvaluateCurveData(C, At(Time, bLoop));
  Mix.Accumulate(C, Weight);
 };
 Add(In.IdleFace, In.IdleFaceTime, 1.f - In.TalkWeight, true);
 Add(In.Talk, In.TalkTime, In.TalkWeight, false);
 Output.Curve.Combine(Mix);
 // The eyes carry what the neck does not; a talker still looks at whoever she talks to.
 auto Pair = [&](const FName (&Names)[2], float V) { V = FMath::Clamp(V, 0.f, 1.f); Output.Curve.Set(Names[0], V); Output.Curve.Set(Names[1], V); };
 Pair(LookLeft, In.EyeYaw / EyeYawRange);
 Pair(LookRight, -In.EyeYaw / EyeYawRange);
 Pair(LookUp, In.EyePitch / EyePitchRange);
 Pair(LookDown, -In.EyePitch / EyePitchRange);
 if (In.Blink > 0) for (const FName& N : Blinks) Output.Curve.Set(N, FMath::Max(Output.Curve.Get(N), In.Blink));
}

US47PersonComponent::US47PersonComponent()
{
 PrimaryComponentTick.bCanEverTick = true;
 PrimaryComponentTick.TickGroup = TG_PrePhysics; // before the body's animation update
}

bool US47PersonComponent::Setup(UAnimSequence* InIdle, UAnimSequence* InIdleFace)
{
 for (USkeletalMeshComponent* C : TInlineComponentArray<USkeletalMeshComponent*>(GetOwner()))
  if (C->GetName() == TEXT("Body")) Body = C;
 if (!Body || !Body->GetSkeletalMeshAsset()) { UE_LOG(LogS47Person, Warning, TEXT("S47: person %s has no Body mesh"), *GetNameSafe(GetOwner())); return false; }
 Body->SetAnimInstanceClass(US47PersonAnim::StaticClass());
 Anim = Cast<US47PersonAnim>(Body->GetAnimInstance());
 if (!Anim) { UE_LOG(LogS47Person, Warning, TEXT("S47: person %s body refused the native animation"), *GetNameSafe(GetOwner())); return false; }
 Anim->Pose.Idle = InIdle; Anim->Pose.IdleFace = InIdleFace;
 if (InIdle) Anim->Held.Add(InIdle);
 if (InIdleFace) Anim->Held.Add(InIdleFace);
 // Nobody needs her to animate when nobody can see her; animation then stays cheap.
 Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
 Body->AddTickPrerequisiteComponent(this);
 NextBlink = FMath::FRandRange(.8, 3.0);
 UE_LOG(LogS47Person, Display, TEXT("S47: person ready: %s"), *Describe());
 return true;
}

FString US47PersonComponent::Describe() const
{
 TArray<FString> Parts;
 for (const UActorComponent* C : GetOwner()->GetComponents())
  if (const USkeletalMeshComponent* S = Cast<USkeletalMeshComponent>(C))
   Parts.Add(FString::Printf(TEXT("%s=%s/%s"), *S->GetName(), *GetNameSafe(S->GetSkeletalMeshAsset()), *GetNameSafe(S->GetAnimInstance())));
 return FString::Printf(TEXT("%s idle=%s face=%s [%s]"), *GetNameSafe(GetOwner()), Anim ? *GetNameSafe(Anim->Pose.Idle) : TEXT("-"),
  Anim ? *GetNameSafe(Anim->Pose.IdleFace) : TEXT("-"), *FString::Join(Parts, TEXT(", ")));
}

UAnimSequence* US47PersonComponent::Take(const FString& Clip)
{
 if (const TObjectPtr<UAnimSequence>* Found = Takes.Find(Clip)) return *Found;
 UAnimSequence* Seq = TalkPath.IsEmpty() ? nullptr : LoadObject<UAnimSequence>(nullptr, *TalkPath.Replace(TEXT("{take}"), *Clip), nullptr, LOAD_NoWarn | LOAD_Quiet);
 Takes.Add(Clip, Seq);
 if (Seq && Anim) Anim->Held.Add(Seq);
 if (!Seq) UE_LOG(LogS47Person, Display, TEXT("S47: person %s has no face take for %s; voice plays without lip sync"), *GetNameSafe(GetOwner()), *Clip);
 return Seq;
}

FVector2D US47PersonComponent::Aim(const FVector& World) const
{
 // Measured from the hips (unmoved by the look itself), from the head's actual place.
 const FVector Head = Body->GetSocketLocation(TEXT("head"));
 FVector Left = Body->GetSocketLocation(TEXT("thigh_l")) - Body->GetSocketLocation(TEXT("thigh_r"));
 Left = (Left - FVector::UpVector * (Left | FVector::UpVector)).GetSafeNormal();
 const FVector Forward = FVector::UpVector ^ Left;
 const FVector Dir = (World - Head).GetSafeNormal();
 return FVector2D(FMath::RadiansToDegrees(FMath::Atan2(Dir | Left, Dir | Forward)), FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Dir | FVector::UpVector, -1.0, 1.0))));
}

void US47PersonComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* Fn)
{
 Super::TickComponent(Dt, TickType, Fn);
 if (!Anim || !Body) return;
 const bool bShown = !Present || Present();
 if (GetOwner()->IsHidden() == bShown) GetOwner()->SetActorHiddenInGame(!bShown);
 if (!bShown) return;
 Clock += Dt;
 FS47PersonPose& P = Anim->Pose;
 // FAnimExtractContext::bLooping does not wrap pose or curve sample time in UE 5.8.
 P.IdleTime = LoopTime(P.IdleTime + Dt, P.Idle);
 P.IdleFaceTime = LoopTime(P.IdleFaceTime + Dt, P.IdleFace);
 // Speech follows the voice's own clock, so pause and reload stay in sync with the audio.
 FString Clip; double Seconds = 0;
 UAnimSequence* Talk = Speech && Speech(Clip, Seconds) ? Take(Clip) : nullptr;
 if (Talk) { P.Talk = Talk; P.TalkTime = float(FMath::Max(0.0, Seconds)); }
 const bool bTalking = Talk && Seconds < Talk->GetPlayLength();
 P.TalkWeight = FMath::FInterpConstantTo(P.TalkWeight, bTalking ? 1.f : 0.f, Dt, 6.f);
 if (P.TalkWeight <= 0) P.Talk = nullptr;
 if (Clip != LastClip && !Clip.IsEmpty()) NextBlink = FMath::Min(NextBlink, Clock + .08); // a new line often starts with a blink
 LastClip = Clip;
 // Attention: the listener when there is one, otherwise her own small round of places.
 FVector2D Want = FVector2D::ZeroVector;
 if (const TOptional<FVector> Who = Listener ? Listener() : TOptional<FVector>()) Want = Aim(*Who);
 else if (IdleSpots.Num())
 {
  if (Clock >= NextSpot) { Spot = FMath::RandRange(-1, IdleSpots.Num() - 1); NextSpot = Clock + FMath::FRandRange(2.5, 7.0); }
  if (IdleSpots.IsValidIndex(Spot)) Want = Aim(IdleSpots[Spot]);
 }
 Want = FVector2D(FMath::Clamp(Want.X, -80.0, 80.0), FMath::Clamp(Want.Y, -35.0, 25.0));
 if (FMath::Abs(Want.X - Gaze.X) > 25) NextBlink = FMath::Min(NextBlink, Clock + .04); // big gaze shifts carry a blink
 // The head follows on a critically damped spring; the eyes get there first.
 const double K = 26.0, C = 2.0 * FMath::Sqrt(K);
 // Keep the damped spring stable through loading hitches or a slow frame.
 double Remaining = FMath::Clamp(double(Dt), 0.0, .25);
 while (Remaining > UE_SMALL_NUMBER)
 {
  const double Step = FMath::Min(Remaining, 1.0 / 60.0);
  GazeVel += ((Want - Gaze) * K - GazeVel * C) * Step; Gaze += GazeVel * Step;
  Remaining -= Step;
 }
 if (Clock >= NextSaccade) { Saccade = FVector2D(FMath::FRandRange(-1.4, 1.4), FMath::FRandRange(-.9, .9)); NextSaccade = Clock + FMath::FRandRange(.35, 1.8); }
 P.LookYaw = float(FMath::Clamp(Gaze.X, -55.0, 55.0)); P.LookPitch = float(FMath::Clamp(Gaze.Y * .8, -24.0, 16.0));
 P.EyeYaw = float(FMath::Clamp(Want.X - P.LookYaw + Saccade.X, -28.0, 28.0));
 P.EyePitch = float(FMath::Clamp(Want.Y - P.LookPitch + Saccade.Y, -20.0, 18.0));
 // Blinks: 0.17 s, every two to six seconds, now and then a double.
 if (BlinkAt < 0 && Clock >= NextBlink) BlinkAt = Clock;
 P.Blink = 0;
 if (BlinkAt >= 0)
 {
  const double U = (Clock - BlinkAt) / .17;
  if (U < 1) P.Blink = float(FMath::Sin(U * UE_DOUBLE_PI));
  else { BlinkAt = -1; bSecondBlink = !bSecondBlink && FMath::FRand() < .12f; NextBlink = Clock + (bSecondBlink ? .1 : FMath::FRandRange(2.0, 6.0)); }
 }
 // A slow breath under the idle; shallower while speaking.
 P.Breath = float(FMath::Sin(Clock * UE_DOUBLE_TWO_PI / 4.3)) * (bTalking ? .35f : 1.f);
 S47PersonReview::Tick(this, Dt);
}

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "S47Obj.h"
#include "S47Json.h"
#include "S47FreeformCarry.generated.h"
class US47Main;
class UBoxComponent;
class UStaticMeshComponent;

// One bounded package contact receiver, owned by the existing world actor.
UCLASS()
class US47FreeformCarryContact : public UActorComponent
{
 GENERATED_BODY()
public:
 TFunction<void(const FVector&,const FHitResult&)> Notify;
 UFUNCTION() void Hit(UPrimitiveComponent* Body,AActor* Other,UPrimitiveComponent* Part,FVector Impulse,const FHitResult& Contact);
};

// A single bought package. The transaction and item identity remain in FreeformWorld.
class FS47FreeformCarry
{
public:
 explicit FS47FreeformCarry(US47Main* Main);
 ~FS47FreeformCarry();
 void Configure(FS47Obj Saro,FS47Obj Road);
 void Restore(const S47J::FObj& Saved,const FVector& LegacyDropped);
 S47J::FObj Save() const;
 void Stop();
 void Step(double Dt,int32 State,const FString& Item,const FVector& Anchor);
 bool Release(bool bDrop);
 FVector Position() const;
 bool Simulating() const;
 bool Blocked() const { return bBlocked; }
 int32 Impacts() const { return ImpactCount; }
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 TSharedPtr<uint8> Life=MakeShared<uint8>(0);
 FS47Obj SaroBody,RoadBody;
 TWeakObjectPtr<UBoxComponent> Support;
 TWeakObjectPtr<US47FreeformCarryContact> Receiver;
 int32 LastState=-1,ImpactCount=0;
 FString Scene,ItemId;
 FTransform Pose=FTransform::Identity;
 FVector Velocity=FVector::ZeroVector,Angular=FVector::ZeroVector;
 FVector LastSafe=FVector::ZeroVector,SupportPoint=FVector::ZeroVector,SupportNormal=FVector::UpVector;
 bool bHavePose=false,bDynamic=false,bPaused=false,bBlocked=false,bBodyBound=false,bSleep=false,bOwnSpeed=false;
 double ActiveAge=0,LastImpact=-10,LastSave=-10;
 float BaseSpeed=1.9f,AppliedSpeed=1.9f;
 UStaticMeshComponent* Body() const;
 FString CurrentScene() const;
 bool Live() const;
 FCollisionQueryParams Query() const;
 FVector Extent() const;
 void Place(const FVector& P,const FQuat& Q);
 void Freeze(bool bKeepVelocity);
 void Resume();
 bool Ground(const FVector& P,FHitResult& Hit) const;
 FVector Clearance(const FVector& From,const FVector& To,const FQuat& Q,bool& bHit) const;
 void SetShown(bool bShown);
 void Contact(const FVector& Impulse,const FHitResult& Hit);
 void ReleaseSpeed();
};

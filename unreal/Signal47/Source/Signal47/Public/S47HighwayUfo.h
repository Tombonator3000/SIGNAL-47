#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"

class US47Main;
class UStaticMeshComponent;
class UPrimitiveComponent;

// Unreal addition: a bounded encounter on the existing highway. Its own route
// never drives, stalls, relocates or captures the player's vehicle.
class SIGNAL47_API FS47HighwayUfo
{
public:
 explicit FS47HighwayUfo(US47Main* Main);
 ~FS47HighwayUfo();
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 S47J::FObj Readback() const;
 // Includes a short, saved quiet interval after the encounter.
 bool Busy() const;
 bool Force(bool bFollowPlayer=false);
 bool CancelForTest();
 FString StageName() const;
 double EventAge() const { return Age; }
 FVector UfoPosition() const { return UfoAt; }
 FVector CarPosition() const { return CarAt; }
 FString PhotoIncidentId() const;
 TArray<UStaticMeshComponent*> PhotoParts() const;
 // UE world centimetres, matching the field camera capture coordinates.
 bool PhotoLineClear(const FVector& FromUE,const FVector& AtUE,const UPrimitiveComponent* Subject) const;
 static bool SelfTest(TArray<FString>& Lines);

private:
 struct FVisual;
 TUniquePtr<FVisual> Visual;
 US47Main* M=nullptr;
 bool bEnabled=false,bActive=false,bFollow=false,bSeen=false,bSawCar=false,bSawPass=false;
 bool bLogged=false,bEscaped=false,bCarRetained=false,bResumePending=false;
 int32 LastNight=-1000,Serial=0;
 double Age=0,Rest=0,StartZ=0,Sign=1,CarSpeed=0,HiddenFor=0,SoundAt=0;
 FVector CarAt=FVector::ZeroVector,UfoAt=FVector::ZeroVector;
 FVector PassStart=FVector::ZeroVector,PassTarget=FVector::ZeroVector;
 FString Outcome;
 mutable FString UfoSightBlocker,CarSightBlocker,FollowSightBlocker,PhotoSightBlocker;
 bool CanStart(bool bForced) const;
 bool Start(bool bForced,bool bFollowPlayer);
 bool Held() const;
 bool RoadVisible() const;
 int32 NightId() const;
 double Clock() const;
 FVector PlayerAt() const;
 bool ClearSight(const FVector& Eye,const FVector& At,const UPrimitiveComponent* Target=nullptr,FString* BlockedBy=nullptr) const;
 bool Visible(const FVector& At,const UPrimitiveComponent* Target=nullptr) const;
 bool ClearToPlayer() const;
 void Draw();
 void Finish(const FString& Why);
 void Note();
 void Changed() const;
 void Advance(double Dt);
};

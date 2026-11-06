#pragma once
#include "CoreMinimal.h"
class US47Main;
class AS47Web;
class UStaticMeshComponent;
class USpotLightComponent;
class UMaterialInstanceDynamic;

// Freeform only: what makes the way out to the farm findable and the farm itself audible. A street lamp at the
// turn-off from the highway, a lamp over the substation's sign, and the farm's own weather and small noises.
class FS47FarmNight
{
public:
 explicit FS47FarmNight(US47Main* Main):M(Main){}
 void Stop();
 void Step(double Dt);
 bool LampsOn() const{return bOn;}
 int32 FarmSounds() const{return Sounds;}
 static FVector EntryLampRoad();
 static FVector TrafoLampWorld();
 static FVector SignLampWorld();
 static FVector SignWorld();
private:
 struct FLamp{TArray<TWeakObjectPtr<UStaticMeshComponent>> Parts;TArray<FVector> Offsets;TWeakObjectPtr<UMaterialInstanceDynamic> Head;TWeakObjectPtr<USpotLightComponent> Light;bool bRoadLocal=false;FVector At=FVector::ZeroVector,Aim=FVector::ZeroVector;float Candela=0;};
 US47Main* M=nullptr;
 TWeakObjectPtr<AS47Web> Scene;
 TArray<FLamp> Lamps;
 FString LastArea;
 double SoundIn=4;
 int32 Sounds=0;
 TArray<TWeakObjectPtr<class ULocalFogVolumeComponent>> Hazes;
 void BuildHaze();
 void StepCattle(double Dt,const FString& Area);
 double MooIn=9;
 int32 StareCow=-1;
 int32 Moos=0;
public:
 int32 MooCount() const{return Moos;}
 FVector TestAlarmAt=FVector::ZeroVector;
private:
 bool bBuilt=false,bOn=false,bWind=false;
 bool Build();
 void Place(const FString& Area);
 void SetOn(bool bNow,const FString& Area);
 void StepSound(double Dt,bool bNight,const FString& Area);
};

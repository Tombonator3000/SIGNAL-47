#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
class US47Main;
class AS47Web;
class UStaticMeshComponent;
class ULocalFogVolumeComponent;

// Freeform only: what the lights leave behind until the next morning, at the farm or a place in town. Pressed
// ground, a low local haze and an occasional sound from that spot; looking closer keeps a field record.
class FS47FarmSite
{
public:
 explicit FS47FarmSite(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 void ArmFarm(double Clock);
 // Finds open ground beside Point, so the marks are never inside the building they name.
 void Arm(double Clock,const FString& InPlace,const FString& InTitle,const FString& InArea,const FVector& Point);
 bool Use();
 FString Label() const;
 bool Armed() const{return ClearAt>0;}
 FVector Centre() const{return Middle;}
 FString FindingId() const{return Place==TEXT("farm")?FString(TEXT("farm_trace")):TEXT("lights_trace_")+Place;}
 FString PlaceId() const{return Place;}
private:
 US47Main* M=nullptr;
 TWeakObjectPtr<AS47Web> Scene;
 TSharedPtr<uint8> Life;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Marks,Spots;
 TWeakObjectPtr<ULocalFogVolumeComponent> Haze;
 FS47Obj SpotSaro,SpotRoad;
 FString Place,Title,AreaId;
 FVector Middle=FVector::ZeroVector;
 double ClearAt=0,SoundIn=0;
 bool bBuilt=false,bActive=false,bBound=false,bShown=false;
 bool Build();
 void Show(bool bShow);
 void Layout();
 double GroundAt(double X,double Y,double Z) const;
 bool Here() const;
};

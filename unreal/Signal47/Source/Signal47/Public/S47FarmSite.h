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
 // Whether a body is coming or here at this site, so that spawning may use the open ground round the player.
 bool VisitWantsGround() const{return Visit>=1&&Visit<=3;}
 int32 VisitStage() const{return Visit;}
 void TestVisit(){Visit=1;VisitAge=0;bVisitSeen=false;}
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
 // A visit: 0 may come, 1 the warning, 2 it sneaks toward the player, 3 it hunts, 4 over for this site.
 int32 Visit=4;
 double VisitAge=0,VisitDwell=0;
 bool bVisitSeen=false;
 void StepVisit(double Dt,double Dist);
 void EndVisit(bool bContact);
 bool bBuilt=false,bActive=false,bBound=false,bShown=false;
 bool Build();
 void Show(bool bShow);
 void Layout();
 double GroundAt(double X,double Y,double Z) const;
 bool Here() const;
};

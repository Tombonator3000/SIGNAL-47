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
 void TestVisit(int32 Who=0){Visit=1;VisitAge=0;bVisitSeen=false;bVisitTaken=false;VisitWho=Who;}
 double VisitBodyDistance() const;
 TOptional<FVector> VisitBodyEye() const{return bBodyOut?TOptional<FVector>(BodyAt+FVector(0,VisitWho==1?.6:1.9,0)):TOptional<FVector>();}
 // After being taken, no new visit before this clock.
 void SetCalm(double Until){CalmUntil=Until;}
 void TestReadyVisit(){Visit=0;VisitDwell=0;}
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
 bool bVisitSeen=false,bVisitTaken=false,bVisitDread=false;
 double CalmUntil=0;
 int32 VisitWho=0; // 0 Roswell, 1 crab, 2 tall
 // Crab and tall are moved here; the small ones have their own brain.
 TSharedPtr<struct FS47PaleBody> VisitBody;
 FVector BodyAt=FVector::ZeroVector;
 double BodyYaw=0,BodyClock=0;
 bool bBodyOut=false;
 bool PutBody(const FVector& Near,double Dist);
 void StepBody(double Dt,double Speed);
 void HideBody();
 void StepVisit(double Dt,double Dist);
 void EndVisit(bool bTaken);
 bool bBuilt=false,bActive=false,bBound=false,bShown=false;
 bool Build();
 void Show(bool bShow);
 void Layout();
 double GroundAt(double X,double Y,double Z) const;
 bool Here() const;
};

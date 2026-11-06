#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
class US47Main;
class AS47Web;
class UStaticMeshComponent;
class ULocalFogVolumeComponent;

// Freeform only: what the lights over the farm leave behind until the next morning. Pressed ground in the
// pasture, a low local haze and an occasional sound from that spot; looking closer keeps a field record.
class FS47FarmSite
{
public:
 explicit FS47FarmSite(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 void Arm(double Clock);
 bool Use();
 FString Label() const;
 bool Armed() const{return ClearAt>0;}
 FVector Centre() const;
 static const TCHAR* FindingId(){return TEXT("farm_trace");}
private:
 US47Main* M=nullptr;
 TWeakObjectPtr<AS47Web> Scene;
 TSharedPtr<uint8> Life;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Marks;
 TWeakObjectPtr<ULocalFogVolumeComponent> Haze;
 FS47Obj Spot;
 double ClearAt=0,SoundIn=0;
 bool bBuilt=false,bActive=false,bBound=false,bShown=false;
 bool Build();
 void Show(bool bShow);
};

#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47EncounterSound.h"
#include "S47Obj.h"

class US47Main;
class UBoxComponent;

// SARO's carried field set. Room 6's batteryless historical radio remains independent.
class FS47FieldRadio
{
public:
 explicit FS47FieldRadio(US47Main* Main):M(Main) {}
 ~FS47FieldRadio();
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Owned() const { return bOwned; }
 bool Ready() const { return Root.IsValid(); }
 void Take();
 struct FPresence { FVector At=FVector::ZeroVector; bool bActive=false,bVisible=false,bHunting=false; };
 static double Proximity(const FVector& Eye,const FPresence& In);
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 FS47Obj Root;
 TWeakObjectPtr<UBoxComponent> Hit;
 TArray<TWeakObjectPtr<class UStaticMeshComponent>> Copies;
 FS47EncounterSound Sound;
 bool bOwned=false,bUnsupported=false,bAvailable=false;
 S47J::FObj Unknown;
 double LastLevel=0,QuietFor=0,SeenFor=0,ThreatFor=0,TestT=0;
 int32 LastCue=-1,TestStage=0;
 bool Build();
 TArray<FPresence> Nearby() const;
 bool InView(const FVector& At) const;
 void ControlTest(double Dt);
};

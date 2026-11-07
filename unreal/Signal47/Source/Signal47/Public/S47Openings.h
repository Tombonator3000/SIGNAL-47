#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
class US47Main;
class AS47Web;
class USpotLightComponent;
class UPointLightComponent;

// Freeform only: what the station can get hold of as it goes on (Tom, 7 November). Each stands at OPS with its price and what it
// waits for, is saved by name, and an older save simply has none. Prices are low against $60 a day, on purpose.
struct FS47Opening
{
 const TCHAR* Id;const TCHAR* Title;int32 Price;const TCHAR* Waits;const TCHAR* What;
};
struct SIGNAL47_API FS47Openings
{
 static const TArray<FS47Opening>& Defs();
 static const FS47Opening* Def(const FString& Id);
 static bool SelfTest(TArray<FString>& Lines);
};

// The spotlight on the truck's roof. It shows what is out there, and it shows the truck. F in the cab.
class FS47TruckLamp
{
public:
 explicit FS47TruckLamp(US47Main* Main):M(Main){}
 void Stop();
 void Step(double Dt);
 bool Toggle();
 bool On() const{return bOn;}
 bool LitTonight(int32 Night) const{return LitNight==Night;}
private:
 US47Main* M=nullptr;
 TWeakObjectPtr<USpotLightComponent> Lamp;
 TWeakObjectPtr<AS47Web> Scene;
 bool bOn=false;
 int32 LitNight=0;
};

// Security lights and a klaxon in the yard. Goes off at what comes near the station; what comes near can put it out, and then it
// wants a fuse at its box. It tells the player something is there. It stops nothing.
class FS47YardAlarm
{
public:
 explicit FS47YardAlarm(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Dead() const{return bDead;}
 bool Ringing() const{return RingLeft>0;}
 static constexpr double Reach=48,RingSeconds=25;
 static constexpr float KnockedOutChance=.3f;
 // Test: as if something had just come into reach, with the given draw for whether it puts the alarm out.
 void TestTrip(float Roll){TestRoll=Roll;bTestTrip=true;}
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 TSharedPtr<uint8> Life;
 TWeakObjectPtr<AS47Web> Scene;
 TArray<TWeakObjectPtr<UPointLightComponent>> Lamps;
 FS47Obj Box;
 bool bBuilt=false,bDead=false,bNear=false,bTestTrip=false;
 float TestRoll=-1;
 double RingLeft=0,HornIn=0;
 bool Build();
 bool BodyNear() const;
 void Show();
 void Mend();
};

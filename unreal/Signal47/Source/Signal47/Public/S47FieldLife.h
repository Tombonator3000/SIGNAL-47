#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
class US47Main;
class AS47Web;
class UStaticMeshComponent;

// Freeform only. The ordinary things that make a day and a drive something: the truck wants water and can bog down or go flat,
// a trip camera can be left at a place overnight, a round of the station gives the day a shape and a log, and now and then
// somebody's car stands empty on the shoulder. Nobody starves and nothing here can leave the player stuck: the jack, the
// spade and the county's roadside service are always there.
class FS47TruckCare
{
public:
 explicit FS47TruckCare(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Stuck() const{return bStuck;}
 bool Flat() const{return bFlat;}
 double Water() const{return WaterLeft;}
 bool Hot() const{return WaterLeft<=0;}
 bool NeedsRescue() const{return bStuck||bFlat||Hot();}
 FString StatusLine() const;
 // Each takes game time and says so. The spade and jack ride in the truck.
 bool DigOut();
 bool ChangeWheel();
 bool TopUp();
 void Clear(); // Roadside service leaves the truck as it should be.
 static constexpr double WaterRange=36000; // metres on a full radiator
 static constexpr double DigMinutes=25,WheelMinutes=15;
 // Chance per hundred metres off the road and on it; none on the road for sand.
 static constexpr float SandChance=.02f,FlatOffRoad=.006f,FlatOnRoad=.0004f;
 void TestStick(){bStuck=true;}
 void TestFlat(){bFlat=true;}
 void TestWater(double Value){WaterLeft=Value;}
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 double WaterLeft=1,Metres=0;
 bool bStuck=false,bFlat=false,bWarned=false,bPowerSet=false,bHeldSet=false;
 void Hold();
};

// Built at the bench (FS47Workshop "trip_camera"), left at a place or by the marks, read the next morning. It records what it
// actually covered from where it stood; it is a record of its own, in text, and never adds anything to the album.
class FS47TripCamera
{
public:
 explicit FS47TripCamera(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Out() const{return bOut;}
 bool CanSet() const;
 bool SetHere(const FString& Title,bool bAtMarks);
 static int32 Reading(int32 Seed,int32 Night,bool bAtMarks,bool bCalm);
 static FString ReadingText(int32 Reading,const FString& Title,int32 Seed,int32 Night);
 bool Collect();
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 bool bOut=false,bAtMarksThen=false,bBuilt=false;
 FString Area,Title;
 FVector At=FVector::ZeroVector;
 double SetClock=0;
 TWeakObjectPtr<UStaticMeshComponent> PropSaro,PropRoad;
 FS47Obj ObjSaro,ObjRoad;
 TWeakObjectPtr<AS47Web> Scene;
 TSharedPtr<uint8> Life;
 bool Build();
 void Layout();
};

// A round of the station: three stops a day, drawn for the day, by daylight. Done before noon a coming fault is caught at a cost of
// one relay. A shift log comes in each morning, of what the record says about yesterday.
class FS47DayRound
{
public:
 explicit FS47DayRound(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Step(double Dt);
 S47J::FObj Save() const;
 static constexpr int32 StopCount=3,RoundPay=5;
 static const TArray<const TCHAR*>& AllStops();
 static TArray<FString> StopsFor(int32 Seed,int32 Day);
 static FString StopName(const FString& Id);
 FString TaskLine() const;
 bool DoneToday() const{return DoneDay==CurrentDay();}
 int32 StopsDone() const{return Ticks.Num();}
 static FString LogPage(int32 Day,int32 Filed,int32 Paid,bool bRound,int32 Records,const FString& Trouble);
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 int32 ForDay=0,DoneDay=0,LogDay=0;
 TArray<FString> Ticks;
 int32 CurrentDay() const;
 void WriteLog(int32 Day);
};

// Somebody's pickup on the shoulder with its door open, some mornings: seen, looked into, gone in a day or two. Nobody ever turns
// up for it. Not Ortiz's wagon: that has a story of its own.
class FS47RoadsideCar
{
public:
 explicit FS47RoadsideCar(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Step(double Dt);
 void Stop();
 S47J::FObj Save() const;
 static bool StandsOn(int32 Seed,int32 Day);
 static double RoadZ(int32 Seed,int32 Day);
 static FString Inside(int32 Seed,int32 Day);
 bool Shown() const{return bShown;}
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 TWeakObjectPtr<UStaticMeshComponent> Car;
 TWeakObjectPtr<AS47Web> Scene;
 FS47Obj Obj;
 TSharedPtr<uint8> Life;
 bool bBuilt=false,bShown=false;
 int32 ShownDay=0;
 bool Build();
 void Look();
};

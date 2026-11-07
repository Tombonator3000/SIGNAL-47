#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
#include "S47RoswellWorld.h"

class US47Main;
class AS47Web;
class UStaticMeshComponent;
class FS47IndoorDread;

// Freeform only, and only in their hours: the player is indoors, in the old house or a room in town, and a door near them
// is shut. Something tries the handle and then waits. FF-RESEARCH-07.
//
// While the handle moves, E on that door holds it: it stops and goes away. While it waits, E opens it: an empty threshold
// and one mark. Neither: the door comes open by itself, slowly, something stands there until it is looked at, and goes.
// Nothing here can hurt the player or keep them in, and nothing follows them. At most one in two nights. What it left is a
// mark on the ground that can be looked at, each ending its own, and the record says what was heard or seen and no more.
class FS47DoorVisit
{
public:
 enum class EStage : uint8 { None, Trying, Waiting, Holding, Opening, Standing, Leaving };
 enum class EEnding : uint8 { None, Held, Walked, Opened };
 static constexpr double TrySeconds=8,WaitSeconds=7,HoldSeconds=3.2,OpenSeconds=3.5,ComeChance=.45;
 FS47DoorVisit(US47Main* Main,FS47IndoorDread* InOwner):M(Main),Owner(InOwner){}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 S47J::FObj Save() const;
 // True while one is under way: nothing else indoors starts or moves. Called only when the session is not held.
 bool Step(double Dt);
 // E on a door. True if this visit took it, so the door is not simply opened or shut.
 bool Pressed(const FString& DoorKey);
 // What the prompt says on this door just now; empty: its own label.
 FString LabelFor(const FString& DoorKey) const;
 bool Active() const{return Now!=EStage::None;}
 // At most one in two nights: the same night and the next are out.
 static bool MayCome(int32 Day,int32 LastDay){return LastDay<=0||Day>=LastDay+2;}
 static bool SelfTest(TArray<FString>& Lines);

 // Tests: read only, but for the dice.
 EStage StageNow() const{return Now;}
 double VisitClock() const{return T;}
 FString DoorKeyNow() const{return Target.Key;}
 FVector DoorMiddle() const{return Target.Middle;}
 FVector DoorOutward() const{return Target.Out;}
 bool DoorOpenNow() const;
 int32 LastDayOf() const{return LastDay;}
 int32 RollDayOf() const{return RollDay;}
 bool DrawnTonight() const{return bTonight;}
 bool SawItOpen() const{return bSawOpen;}
 void TestDice(float Roll){TestRoll=Roll;}
 TOptional<FVector> BodyEye() const{return bBodyOut?TOptional<FVector>(BodyFeet+FVector(0,1.05,0)):TOptional<FVector>();}
 int32 MarksOut() const{return TraceAt.Num();}
 EEnding TraceKind() const{return Trace;}
 bool UseMarks();
private:
 struct FTarget
 {
  bool bHouse=false;
  int32 Index=INDEX_NONE; // the old house's door
  FString Name,Key,Title,Area;
  FVector Middle=FVector::ZeroVector,Out=FVector::ZeroVector; // Out: away from the player, as it began
 };
 US47Main* M=nullptr;
 FS47IndoorDread* Owner=nullptr;
 TWeakObjectPtr<AS47Web> Scene;
 TSharedPtr<uint8> Life;
 FS47RoswellWorld Sight;

 // The one under way.
 EStage Now=EStage::None;
 FTarget Target;
 double T=0,AllT=0,SoundIn=0,HushIn=0,Noticed=0,Shake=0;
 int32 Beat=0;
 bool bBodyOut=false,bSawBody=false,bLightComes=false;
 // bSawOpen: the door has been in the player's sight since it moved. bCutShort: something bigger ended it in full view.
 bool bSawOpen=false,bCutShort=false;
 FVector BodyFeet=FVector::ZeroVector;
 double BodyYaw=0;

 // The night's draw for it.
 int32 RollDay=0,LastDay=0;
 int32 LastBefore=0,RetryDay=0; // not saved: only for handing back one nobody saw
 bool bTonight=false;
 double InsideFor=0,WaitFor=0;
 float TestRoll=-1;

 // What it left. Gone by the next noon.
 EEnding Trace=EEnding::None;
 TArray<FVector> TraceAt;
 FString TraceArea,TraceTitle;
 int32 TraceDay=0;
 double TraceUntil=0;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Pieces,Spots;
 FS47Obj SpotSaro,SpotRoad;
 bool bBuilt=false,bShown=false;

 bool Build();
 void ShowMarks(bool bShow);
 void LayoutMarks();
 void PutMarks(EEnding Ending);
 bool MarksHere() const;
 FString MarksLabel() const;
 double GroundNear(const FVector& At) const;

 bool FindDoor(FTarget& Out) const;
 void StepDraw(double Dt);
 void StepVisit(double Dt);
 void BeginOpening();
 void PoseDoor(double K);
 void SetDoorQuietly(bool bOpen);
 void PutBodyOut();
 bool DoorInSight() const;
 void GiveWay();
 void Handle(float Gain);
 void Finish(EEnding Ending);
 void LetGo();
};

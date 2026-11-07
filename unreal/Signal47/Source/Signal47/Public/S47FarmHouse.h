#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"

// Block model of the abandoned farmhouse (Docs/Design/Concepts/Farmhouse, plan v1). Road-local coordinates,
// Freeform only. An older empty house on the property; the Alderete house itself stays lived in.
namespace S47FarmHouse
{
 enum class EMat : uint8 { Wall, Trim, Roof, Floor, Paint };
 struct FPiece { FString Name; FVector Centre, Size; EMat Mat; };
 // Everything visible: walls cut by their doors and windows, floor, ceiling, roof and veranda.
 TArray<FPiece> Pieces();
 // Footprints that block walking: walls, with doors left open and windows closed.
 TArray<FBox2D> Solids();
 bool OnPad(double RoadX,double RoadZ,double Margin=0);
 // Within the outer walls: the pad less its veranda, which has a roof but is outdoors.
 bool Inside(double RoadX,double RoadZ);
 double PadY();
 // The one house's model is imported (T16-15): it is drawn from that, not from Pieces().
 bool HasModel();
 // Road-local points in the plan's own rooms, for tests and later events.
 FVector Point(double PlanX,double PlanY);
 bool SelfTest(TArray<FString>& Lines);
 // Each door opening in the plan: the leaf hangs at From and closes across to To. Road-local.
 struct FDoorPlan { FString Name; FVector From, To; };
 TArray<FDoorPlan> DoorPlans();
}

class US47Main;
class AS47Web;
class UStaticMeshComponent;
// The old house's doors, Freeform only. A closed one stops the player; each keeps its state in the case.
class FS47FarmHouseDoors
{
public:
 explicit FS47FarmHouseDoors(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Use(int32 Index);
 bool IsOpen(int32 Index) const{return Doors.IsValidIndex(Index)&&Doors[Index].bOpen;}
 int32 Num() const{return Doors.Num();}
 int32 IndexOf(const FString& Name) const;
 FString NameOf(int32 Index) const{return Doors.IsValidIndex(Index)?Doors[Index].Name:FString();}
 // Three-space, in the SARO area: the middle of the shut leaf at floor height, and square to it.
 FVector Middle(int32 Index) const;
 FVector Normal(int32 Index) const;
 // A hand on it from the other side (FS47DoorVisit): this fraction of the full swing, drawn over what the door says it is.
 // Negative lets go, and the door goes on from there to where it should be.
 void PoseOver(int32 Index,double K);
 void SetOpenQuietly(int32 Index,bool bOpen);
private:
 struct FDoor{FString Name,Id;FVector Hinge,Along;double Width=1,K=0,Over=-1;bool bOpen=false;TWeakObjectPtr<UStaticMeshComponent> Leaf;FS47Obj Object;FBox2D Closed;};
 US47Main* M=nullptr;
 TWeakObjectPtr<AS47Web> Scene;
 TSharedPtr<uint8> Life;
 TArray<FDoor> Doors;
 bool bBuilt=false,bActive=false;
 bool Build();
 void Pose(FDoor& D);
 void SetCollider(const FDoor& D);
 bool PlayerIn(const FDoor& D) const;
};

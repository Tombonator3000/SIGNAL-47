#pragma once
#include "CoreMinimal.h"
#include "S47Obj.h"
class AS47Web;
class UStaticMeshComponent;
// D45-20261110 (Tom, D45-29): the alien bobblehead Tom downloaded, on the dashboard of the service Jeep: on the soft
// pad left of the middle, turned to the driver. Its big head sits on its spring and wobbles with what the truck does
// (pulling away, braking, the turns, the road's knocks), read from the truck's own pose each frame. The meshes come
// from Tools/alienbubblehead (head and body split there, /Game/Licensed: never in Git). Without them, or without the
// Jeep's authored cab, nothing is added and the truck is as it was. Drawn only: no collision, nothing to use.

/** The head on its spring, pure (the truck and the self-test share it). Angles in radians in the figure's own frame
 *  (three's terms, the figure faces +Z): Nod tips the top of the head towards its face, Lean towards its -X. */
struct SIGNAL47_API FS47BobbleSpring
{
 double Nod=0,NodV=0,Lean=0,LeanV=0;
 /** The largest tilt since the last Settle (the cab test: it moved while driving). */
 double Peak=0;
 /** One step. Force: the specific force on the head in the figure's frame, m/s^2 (gravity less the acceleration):
  *  (0,-9.81,0) when still and level. */
 void Step(double Dt,const FVector& Force);
 /** The base's place (three world, metres) and the figure's world turn, each frame: its acceleration from the
  *  change of its velocity, then Step. A hitch or a jump (a cut, another area) forgets instead of kicking. */
 void Follow(double Dt,const FVector& Base,const FQuat& Figure);
 /** No history: the next Follow gives no kick. */
 void Forget(){bLast=bLastV=false;Acc=FVector::ZeroVector;}
 /** Still and upright, no history. */
 void Settle(){Nod=NodV=Lean=LeanV=Peak=0;Forget();}
 /** A shove (F10), angular speeds in rad/s. */
 void Kick(double ByNod,double ByLean){NodV+=ByNod;LeanV+=ByLean;}
 /** The head's turn on its pivot, in the figure's frame. */
 FQuat Tilt() const;
 static bool SelfTest(TArray<FString>& Lines);
private:
 FVector Last=FVector::ZeroVector,LastV=FVector::ZeroVector,Acc=FVector::ZeroVector;
 bool bLast=false,bLastV=false;
};

class SIGNAL47_API FS47Bobblehead
{
public:
 /** On the Jeep's dash, under the truck's Body node (truck-local, metres), turned to the driver's eye. Null when the
  *  imported meshes are not there. */
 static TUniquePtr<FS47Bobblehead> Install(AS47Web* W,const FS47Obj& Body,const FString& Path,const FVector& Eye);
 /** Where it stands on the pad (truck-local) and its turn about Y so its face looks at Eye. */
 static void Place(const FVector& Eye,FVector& OutAt,double& OutHeading);
 /** Each frame of the truck (FS47Truck::Update), with the truck's pose. Dt 0: no history (parked, a cut). */
 void Update(double Dt,const FVector& TruckPos,const FQuat& TruckQ);
 /** The angles from another copy of the truck (the drive goes on in another area): no kick from the jump. */
 void Adopt(const FS47BobbleSpring& From);
 /** Still and upright (parked). */
 void Settle();
 /** Freeform's truck has it; Story's cab stays as it was (Freeform before Story): hidden there. */
 void SetShown(bool bOn){Figure.SetVisible(bOn);}
 bool Audit(TArray<FString>& Lines,bool bDrivingNow,bool bShownWanted) const;
 FString Describe() const;
 FS47BobbleSpring Spring;
 FVector At=FVector::ZeroVector;
 double Heading=0;
private:
 FS47Obj Figure,Head;
 TWeakObjectPtr<UStaticMeshComponent> FigureMesh,HeadMesh;
};

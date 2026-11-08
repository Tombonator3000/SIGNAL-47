#pragma once
#include "CoreMinimal.h"
#include "S47Obj.h"
class AS47Web;
class USceneComponent;
class UPrimitiveComponent;

// The box at the road (MAILBOX-20261108), built from the engine's cube and cylinder with the project's own materials, as the farm's
// windmill is: no import. A standard rural box of the 1980s (about 19 by 6.5 by 9 inches), galvanized steel gone dull, two beads
// pressed round it, a door that drops on a hinge at the bottom with a thumb latch, on a weathered four-by-four with a mounting board
// and a brace, its bottom 42 inches over the road, the front a hand behind the edge of the pavement as the carrier wants it. Rust at
// the seams and under the latch, soil splashed on the post, a lean and a twist. SARO and a box number on both sides, no makers' names.
// The red flag on the side away from the service road: down along the side, or straight up over the top, where it is seen from the
// highway and from the yard. Three-space: x east, y up, z south; the door faces the highway, west.
class FS47MailBoxModel
{
public:
 // The post's foot on the highway's east shoulder, just south of where the service road comes in (Grounds.ts: the road at z 24.5,
 // the stop sign at 21.1, the ranch fence from z 34.5): clear of the turn, and seen past the annex from the truck pad.
 static constexpr double PostX=-19.45,PostZ=31.0,DesertY=-.6;
 bool Build(AS47Web* Web,int32 Parent);
 bool Built() const;
 // Measured down to the ground there when the land under it is loaded, then it stays put.
 void Seat(AS47Web* Web);
 void SetFlag(bool bUp){FlagTo=bUp?1.f:0.f;}
 bool FlagIsUp() const{return FlagK>.5f;}
 // The flag swings in about half a second.
 void Step(double Dt);
 void Show(bool bOn);
 FS47Obj Pick;
 // Where a player stands to use it, and where a parcel lifted out of it starts (three-space).
 FVector Front() const;
 FVector Lid() const;
 bool Seated() const{return bSeated;}
private:
 TWeakObjectPtr<AS47Web> For;
 TWeakObjectPtr<USceneComponent> Root,FlagPivot;
 TArray<TWeakObjectPtr<UPrimitiveComponent>> Parts;
 TWeakObjectPtr<UPrimitiveComponent> PickPart;
 double Ground=DesertY;
 float FlagK=0,FlagTo=0;
 bool bSeated=false;
 void Place();
};

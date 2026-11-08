#pragma once
#include "CoreMinimal.h"
#include "S47Obj.h"
class AS47Web;
class USceneComponent;
class UPrimitiveComponent;
class UMaterialInstanceDynamic;

// The box at the road (MAILBOX-20261108), built from the engine's cube and cylinder with the project's own materials, as the farm's
// windmill is: no import. A standard rural box of the 1980s (about 19 by 6.5 by 9 inches), galvanized steel gone dull and gray, two
// beads pressed round it, a door that drops on a hinge at the bottom with a thumb latch, on a weathered four-by-four with a mounting
// board and a brace, its bottom 42 inches over the road, the front a hand behind the edge of the pavement as the carrier wants it.
// Thin rust at the seams and a run of it under the latch, road dust on the lower half, scratches by the door, soil splashed on the
// post, a lean and a twist. SARO and BOX 19 painted by hand on both sides, the number on the door, no makers' names.
// The red flag is the house's, as on every rural box in the country: up, there is outgoing mail in the box for the carrier (an order
// to Western Surplus); the carrier takes it and puts the flag down. The carrier never puts it up. Its plate is a big one, as on boxes
// far from the house (about a foot by eight inches), on the side away from the service road: down along the side, or straight up
// over the top, where it shows from the highway and from the truck pad. A parcel too big for the box sits on top of it, as rural
// carriers leave them in dry country. Three-space: x east, y up, z south; the door faces the highway, west.
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
 // A parcel on top of the box, wrapped in brown paper and tied, its label toward the road.
 void SetParcel(bool bOn);
 bool ParcelShown() const{return bParcel;}
 // The flag swings in about half a second.
 void Step(double Dt);
 void Show(bool bOn);
 FS47Obj Pick;
 // Where a player stands to use it, and where a parcel lifted out of it starts (three-space).
 FVector Front() const;
 FVector Lid() const;
 bool Seated() const{return bSeated;}
 double GroundY() const{return Ground;}
private:
 TWeakObjectPtr<AS47Web> For;
 TWeakObjectPtr<USceneComponent> Root,FlagPivot;
 TArray<TWeakObjectPtr<UPrimitiveComponent>> Parts,ParcelParts;
 TWeakObjectPtr<UPrimitiveComponent> PickPart;
 double Ground=DesertY;
 float FlagK=0,FlagTo=0;
 bool bSeated=false,bParcel=false,bShown=true;
 void Place();
};

// The rural carrier's car: a small white right-hand-drive mail truck of the kind the routes ran in the eighties, square-cabbed with a
// short hood, an amber light on the roof that flashes while it stands at a box. Built from the engine's cube and cylinder, no names,
// no badges, no grille that is anybody's. Never a station wagon: the white wagon out on the flat is wagon 23's alone. FS47Mail
// drives it up the highway past SARO at the carrier's hour; here it is only drawn. Three-space, facing north (-z).
class FS47MailCarModel
{
public:
 static constexpr double Length=3.7,Width=1.62;
 bool Build(AS47Web* Web);
 bool Built() const;
 // Where its middle stands on the road (three-space, the ground under it), facing north, nose up by Pitch (radians).
 void Place(const FVector& At,double Pitch);
 void Show(bool bOn);
 void Beacon(bool bOn);
 bool Shown() const{return bShown;}
private:
 TWeakObjectPtr<AS47Web> For;
 TWeakObjectPtr<USceneComponent> Root;
 TWeakObjectPtr<UMaterialInstanceDynamic> BeaconMat;
 bool bShown=false,bBeacon=false;
};

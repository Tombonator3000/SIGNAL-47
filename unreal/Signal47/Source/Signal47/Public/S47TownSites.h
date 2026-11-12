#pragma once
#include "CoreMinimal.h"
#include "S47Drive.h"
#include "S47EventPlaces.h"
class US47Main;
class AS47Web;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class USceneComponent;
class UPointLightComponent;

// M4, NEW-PLACES-20261108 (Tom, 8 November: "Når M4, plasser temp-boks der du mener de forskjellige bygningene bør stå"). Freeform
// only: grey boxes the size the buildings will be, each with its name on a board or a wall and "(TEMP)" after it, so that Tom can
// walk up to them, drive round them and see them on the survey sheet before real models are made. They stop the walker and the
// truck, they are on the place catalogue (S47EventPlaces) so that a night's event can come there, and the sheet draws and names them.
//
// Where they stand is the one table in S47TownSites.cpp (Spots): a row a building, its middle in road-local metres (x east, z south)
// and how far it is turned, degrees clockwise seen from above with north up. Moving a box is changing its row; everything else (the
// walls, the colliders, the place for events, the sheet) follows from it.
namespace S47TownSites
{
	// One box: its middle on the site's own ground (x east, z south before the site is turned), its size, its colour.
	struct FPart
	{
		double X = 0, Z = 0, W = 1, D = 1, H = 1;   // metres: W across x, D across z, H tall
		double Lift = 0;                            // standing this high off the ground (a water tower's tank)
		uint32 Colour = 0xa8a49b;
		bool bSolid = true;                         // stops the walker and the truck, and is drawn on the sheet
		bool bRound = false;                        // a cylinder (the tank, a pole) instead of a box
		FString Use;                                // N1110: what a look at it picks (an interactable's id, FS47Breather's); empty: none
		bool bKeep = false;                         // stays up beside the site's real model (a flagpole); the others are its stand-in
	};
	// Words on a board or a wall: where, which way they face (degrees clockwise from north, before the site is turned), how big.
	struct FWords { FString Text; double X = 0, Z = 0, Up = 2, Facing = 90, Size = 1; bool bLight = false; bool bKeep = false; }; // bLight: pale paint on a dark board; bKeep: as FPart's
	struct FSite
	{
		FString Id;        // "trailers": the place catalogue's id, the colliders' and components' names
		FString Sign;      // "TRAILER PARK (TEMP)"
		FString Title;     // "the trailer park by the highway": for the radio, the log and the catalogue
		FString MapName;   // "Trailer Park (temp)": printed on the survey sheet
		double X = 0, Z = 0, Yaw = 0; // from Spots
		double PlaceX = 0, PlaceZ = 0, EntryX = 0, EntryZ = 0; // road-local, turned with the site: in front of it, and its way in
		int32 NameSide = 1;            // the sheet's name goes east (1) or west (-1) of it
		ES47PlaceTrait Traits = ES47PlaceTrait::None;
		double Radius = 20, Up = 24;
		TArray<FPart> Parts;
		TArray<FWords> Words;
		// N1110: a landmark that is there for good (the Breather), not a temporary box: no "(TEMP)", and not a place for events.
		bool bLandmark = false;
		// SCHOOL-V2 stage 2: the site's real model (a static mesh from Tools/freemodels, /Game/Free; Content/Free is not in Git), stood
		// up when the asset is there. The parts and words without bKeep are hidden while it stands (their footprints still hold the
		// walker and the truck); without the asset the boxes stay as they were. ModelX/Z: the mesh's pivot (the middle of its base) in
		// the site's metres; ModelYaw: turned on top of the site's; ModelLift: how far the pivot goes under the ground (a basement).
		FString Model; double ModelX = 0, ModelZ = 0, ModelYaw = 0, ModelLift = 0;
	};
	// Every site, built once from Spots and the shapes below; fixed after that.
	const TArray<FSite>& Sites();
	const FSite* Find(const FString& Id);
	// A local point of a site (x east, z south before it is turned) on the land, road-local.
	FVector2D ToRoad(const FSite& S, double LocalX, double LocalZ);
	// What stops a walker and the truck, road-local, as boxes square to the land: a part turned at an angle is cut into strips a
	// metre and a half wide that cover it (no gap between them), so the boxes need not stand square. Exact for 0, 90, 180, 270.
	TArray<FBox2D> Footprint(const FSite& S, const FPart& P);
	// All of them, for the survey sheet.
	TArray<FBox2D> SolidFootprints();
	// The catalogue's entries for these sites (S47EventPlaces appends them to its own list); a landmark is not one.
	TArray<FS47EventPlace> EventPlaces();
	bool SelfTest(TArray<FString>& Lines);
}

// The boxes in the world: built in the road's area when Freeform begins, seated on the ground the walker stands on when the player
// comes near, taken out of the walker's and the truck's way again (and hidden) when Freeform stops.
class FS47TownSites
{
public:
	explicit FS47TownSites(US47Main* Main) : M(Main) {}
	void Begin();
	void Stop();
	void Step(double Dt);
	bool Built() const { return bBuilt; }
	// Tests and F10: how many parts and colliders are up, and plants hidden under the boxes.
	int32 PartCount() const { return Parts.Num(); }
	int32 ColliderCount() const;
	int32 HiddenPlants() const { return Hidden.Num(); }
	bool Seated(const FString& Id) const;
	// N1110: the Breather's draft, words and night (S47Breather.h), on the pieces of its row.
	class FS47Breather* Breather() const { return BreatherPart.Get(); }
private:
	TSharedPtr<class FS47Breather> BreatherPart;
	TMap<FString, int32> UseNodes; // a use's id to the first node of a part that carries it
	US47Main* M = nullptr;
	TWeakObjectPtr<AS47Web> Scene;
	// bModel: the site's real model; bByModel: a stand-in hidden for good while the model stands.
	struct FBuiltPart { FString Site; int32 Part = -1; bool bWords = false; int32 Words = -1; bool bModel = false, bByModel = false; TWeakObjectPtr<USceneComponent> Comp; };
	TArray<FBuiltPart> Parts;
	TSet<FString> SeatedSites;
	TArray<FS47Obstacle> Obstacles; // as added to the road's drive area, world terms
	bool bBuilt = false, bActive = false, bDriveAdded = false;
	double SeatIn = 0;
	// The plants may come into the world after the boxes: looked for again a few times while none have been found.
	double PlantIn = 5; int32 PlantTries = 0;
	// Plants (Caliche's own and the route's) that stood where a box now stands: shrunk to nothing, and their transforms kept so that
	// Stop puts them back.
	struct FHidden { TWeakObjectPtr<UInstancedStaticMeshComponent> Comp; int32 Index = -1; FTransform Was; };
	TArray<FHidden> Hidden;
	// The places at night (critics, round 3: the boxes were dead after dark): a porch light at the trailers until midnight, the court's
	// yard light on its water tower and the school's over its door all night; out on a night something big came there (S47Visitors::Big).
	struct FLamp { FString Site; TWeakObjectPtr<UPointLightComponent> L; double From = 19, To = 24; };
	TArray<FLamp> Lamps;
	void BuildLamps();
	void StepLamps();
	bool Build();
	void Seat(const FString& Site, bool bWithFloor);
	void SetColliders(bool bOn);
	void SetDrive(bool bOn);
	void HidePlants(bool bHide);
	void ShowAll(bool bShow);
	double Ground(double RoadX, double RoadZ, bool bWithFloor) const;
};

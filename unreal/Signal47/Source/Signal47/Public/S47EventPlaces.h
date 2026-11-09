#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;

// Freeform only (Tom, 8 November: "hendelser ved gården kan komme hvor som helst i verdenen"). One list of the places a night's
// event can happen at, and one draw that picks among them. The farm is one place on it, the same as the church or a dry wash
// out past the mine. An event says what it needs (a door, a window, open ground, far out); the draw says where, from the
// game's seed, the night and the event, so that loading again gives the same answer.
//
// Positions are road-local metres (x east, z south), the land's one frame (S47OpenLand). Where that is in the area the player
// is in, and which area owns it, is asked of S47OpenLand at the moment it is needed: never stored in a save.

enum class ES47PlaceTrait : uint32
{
 None    =0,
 Indoors =1<<0,  // a room one can stand in, under a roof
 Door    =1<<1,  // a shut door something can try (FS47DoorVisit)
 Window  =1<<2,  // a window or door glass something can look in at (FS47IndoorDread)
 Phone   =1<<3,  // a telephone that can ring
 Road    =1<<4,  // beside a road one drives on
 Open    =1<<5,  // open ground, room for lights to come down and marks to be left
 Far     =1<<6,  // a long way out: a drive, heard of on the CB rather than seen
 People  =1<<7,  // somebody lives or works there, who could call it in
 Farm    =1<<8,  // a ranch yard: stock, dogs, a windmill
 Town    =1<<9,  // Caliche
 Saro    =1<<10, // the station's own ground
 Wash    =1<<11, // beside a dry wash (arroyo)
 Lot     =1<<12, // a parking lot or a pull-off where a car can stand
 Array   =1<<13, // on the array field
};
ENUM_CLASS_FLAGS(ES47PlaceTrait)

struct FS47EventPlace
{
 FString Id;       // "farm", "church", "land:wash_north": stable, saved by name
 FString Title;    // "the church": for radio lines and the log, lower case, with its article
 FString Region;   // "farm", "caliche", "saro", "motel", "diner", "station", "land"
 double X=0,Z=0;   // road-local: on the ground outside, where lights hang and marks are left
 double EntryX=0,EntryZ=0; // road-local: the door or the way in; the same as X,Z where there is none
 FString DoorKey;  // the door's own name where it has one: a CalicheAccess room id ("Church"), "house:<n>" for the old house
 ES47PlaceTrait Traits=ES47PlaceTrait::None;
 double Radius=12; // about how far it spreads, metres
 double Up=24;     // how high lights hang over it, metres over the ground
 FString Prop;     // an interactable at its front ("town:cinema"), measured live where it has one; empty: the land itself
 bool Has(ES47PlaceTrait T) const{return EnumHasAllFlags(Traits,T);}
 bool HasAny(ES47PlaceTrait T) const{return EnumHasAnyFlags(Traits,T);}
};

// What an event asks of a place.
struct FS47PlaceNeed
{
 FString Kind;                         // the event's id ("door_visit", "town_lights"): its own salt, and what "recently" means
 ES47PlaceTrait All=ES47PlaceTrait::None; // every one of these
 ES47PlaceTrait Any=ES47PlaceTrait::None; // at least one of these, when not None
 ES47PlaceTrait Not=ES47PlaceTrait::None; // none of these
 double MinM=0;                        // no nearer the player than this, road-local metres (0: any)
 double MaxM=0;                        // no further than this (0: any)
 float FarShare=.2f;                   // how often it may be somewhere far off, seen or heard of, not where the player is going
 bool bSameArea=false;                 // only where the player is now (its ground is shown): something to be seen from here
 bool bOtherArea=false;                // only off the ground the player is on: heard of, a drive away
 TArray<FString> Only;                 // when not empty: one of these ids
 TArray<FString> Skip;                 // never these
};

struct FS47PlacePick
{
 FString Id;               // empty: nothing on the list fits
 double DistanceM=0;       // from the player at the draw, road-local
 bool bFar=false;          // drawn as one of the far ones
 float Roll=0;             // the draw itself, for the log
 bool IsSet() const{return !Id.IsEmpty();}
};

class FS47EventPlaces
{
public:
 explicit FS47EventPlaces(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Stop(){}
 S47J::FObj Save() const;

 // The whole list. Built once from what the world has (Caliche's rooms, the farm, FS47Places, the stations); fixed after that.
 const TArray<FS47EventPlace>& All() const;
 const FS47EventPlace* Find(const FString& Id) const;
 TArray<const FS47EventPlace*> Matching(const FS47PlaceNeed& Need) const;

 // The draw. Same seed, night, need, salt and history: same answer. Weighted down where something was lately, up where the
 // player is or is heading (their truck's heading, the job they are on), and now and then somewhere far off.
 // Salt: anything else that should part two draws of the same kind on the same night (an incident serial). Does not
 // remember the pick: call Used when the event really starts there.
 FS47PlacePick Pick(const FS47PlaceNeed& Need,uint32 Salt=0) const;
 // Pick as above for a given player position, night and seed: no world needed. For tests and the F10 preview.
 FS47PlacePick PickFor(const FS47PlaceNeed& Need,int32 Seed,int32 Night,const FVector2D& From,const FVector2D& Ahead,uint32 Salt=0) const;

 // Something happened there tonight: the place is weighted down for a few nights, the farm like anywhere else. Saved.
 void Used(const FString& Id,const FString& Kind);
 int32 LastNight(const FString& Id) const; // 0: never
 int32 TimesUsed(const FString& Id) const;
 // What used it last (the event's kind, as Used was told): "site_visit", "landing_trace". Empty: never. (S47Visitors' military)
 FString LastKind(const FString& Id) const{const FUse* U=Uses.Find(Id);return U?U->Kind:FString();}

 // Where it is in the area the player is in now, three-space, on the ground. False when it is in another area: it can still
 // be heard of, or seen as a light from far off (OutAt is then where it would be in that area's terms, Area its name).
 bool AtNow(const FString& Id,FVector& OutAt,FString* OutArea=nullptr) const;
 bool EntryNow(const FString& Id,FVector& OutAt) const;
 static FString AreaOf(const FS47EventPlace& P); // "saro", "road", "station01": the area that owns its ground
 double DistanceFromPlayer(const FS47EventPlace& P) const; // road-local metres, -1 without a player
 // Where the player is, road-local, whatever area they are in. False: no player or no land.
 bool PlayerRoad(FVector2D& Out) const;

 // The place the player is at, or nearest within this many metres that has these traits. Null: none.
 const FS47EventPlace* Nearest(double WithinM,ES47PlaceTrait Need=ES47PlaceTrait::None) const;
 const FS47EventPlace* NearestTo(const FVector2D& Road,double WithinM,ES47PlaceTrait Need=ES47PlaceTrait::None) const;
 // The one whose door this is: a CalicheAccess room id, or "house" for the old house.
 const FS47EventPlace* WithDoor(const FString& DoorKey) const;
 FString TitleOf(const FString& Id) const; // "somewhere out there" for an id it does not know
 // One log line for every event that is given a place: "S47: event place <kind> -> <id> (<title>), <how>".
 static void LogPick(const FString& Kind,const FS47EventPlace* P,const FString& How);

 // F10: an id from the list, or empty for the draw as the game would make it.
 static TArray<FString> Ids();

 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 struct FUse{int32 Night=0;int32 Count=0;FString Kind;};
 TMap<FString,FUse> Uses;
 float Weight(const FS47EventPlace& P,const FS47PlaceNeed& Need,int32 Night,const FVector2D& From,const FVector2D& Ahead,bool bFar) const;
 // Built once from the table, then seated on the land and measured against the world when it can be.
 mutable TArray<FS47EventPlace> List;
 mutable bool bLandSeated=false,bTownMeasured=false,bLiveMeasured=false;
 mutable double NextMeasure=0;
 void Refine() const;
 bool Fits(const FS47EventPlace& P,const FS47PlaceNeed& Need) const;
};

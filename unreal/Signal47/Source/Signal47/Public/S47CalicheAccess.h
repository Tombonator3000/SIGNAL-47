#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class AS47Web;
class UPointLightComponent;
// Temporary playtest admission. Owns only the authored town/homes overlay, never story facts.
namespace S47CalicheAccess
{
 void Init(US47Main* Main);
 void Update();
 void Save(const S47J::FObj& State);
 void Restore(const S47J::FObj& State);
 bool HasDoor(const FString& Id);
 // CHURCH-COLONIAL-20261113: the white board church stands only while the mission church's pack (S47Convent) is not here: never both.
 bool OldChurchStands(bool bConventPresent);
 // Parts of the white board church (shell, glass, interior, fixtures) stood up in this world.
 int32 OldChurchParts();
 // Whether two names (a room's id, its use, town:use, access:id) mean the same room's door.
 bool SameDoor(const FString& A,const FString& B);
 bool UseDoor(const FString& Id);
 bool IsOpen(const FString& Id);
 double FloorAt(double X,double Z); // NaN outside supported rooms and entry ramps.
 // Something at a door, with nobody there: the nearest shut one within reach of this place comes open. Three-space metres.
 bool MoveNearestDoor(const FVector& At,double Range);
 // The rooms' own lamps, 1 as built and 0 out.
 void SetLampLevel(double K);
 // NPC-DAY: one room's lamp (by its id: "Sheriff", "H02"...), 1 as built and 0 out: whether anyone is home. Times the level above.
 void SetRoomLamp(const FString& Id,double K);
 // CALICHE-HOMES: a home's lamp that is not a room's (the trailer, S47CalicheNewHomes) under the same two switches, by its own id
 // ("H08"); and what a home's lamp is at now (the level times whether anyone is home), 0 for none: its windows go by it.
 void AddLamp(const FString& Id,UPointLightComponent* Light,double Lumens);
 double LampLevel(const FString& Id);
 // Test: the middle of a room's floor, by its id or use.
 bool RoomMiddle(const FString& Id,FVector& Out);
 // The room this point is in, by its id. False: none of them.
 bool RoomAt(const FVector& At,FString& OutId);
 // The middle of a room's door at floor height, and square to it.
 bool DoorAt(const FString& Id,FVector& OutMiddle,FVector& OutNormal);
 // Points just outside a room's four walls, a step apart, at floor height: where a window or the door glass can be looked in
 // at from. Three-space metres. Empty: no such room.
 TArray<FVector> OutsideWalls(const FString& Id,double Out=.75,double Step=1.);
 // A hand on it from the other side (FS47DoorVisit): this fraction of the full swing, drawn over what the door says it is.
 // Negative lets go.
 void PoseDoor(const FString& Id,double K);
 void SetOpenQuietly(const FString& Id,bool bOpen);
}

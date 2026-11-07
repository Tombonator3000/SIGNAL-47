#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class AS47Web;
// Temporary playtest admission. Owns only the authored town/homes overlay, never story facts.
namespace S47CalicheAccess
{
 void Init(US47Main* Main);
 void Update();
 void Save(const S47J::FObj& State);
 void Restore(const S47J::FObj& State);
 bool HasDoor(const FString& Id);
 bool UseDoor(const FString& Id);
 bool IsOpen(const FString& Id);
 double FloorAt(double X,double Z); // NaN outside supported rooms and entry ramps.
 // Something at a door, with nobody there: the nearest shut one within reach of this place comes open. Three-space metres.
 bool MoveNearestDoor(const FVector& At,double Range);
 // The rooms' own lamps, 1 as built and 0 out.
 void SetLampLevel(double K);
 // Test: the middle of a room's floor, by its id or use.
 bool RoomMiddle(const FString& Id,FVector& Out);
 // The room this point is in, by its id. False: none of them.
 bool RoomAt(const FVector& At,FString& OutId);
 // The middle of a room's door at floor height, and square to it.
 bool DoorAt(const FString& Id,FVector& OutMiddle,FVector& OutNormal);
 // A hand on it from the other side (FS47DoorVisit): this fraction of the full swing, drawn over what the door says it is.
 // Negative lets go.
 void PoseDoor(const FString& Id,double K);
 void SetOpenQuietly(const FString& Id,bool bOpen);
}

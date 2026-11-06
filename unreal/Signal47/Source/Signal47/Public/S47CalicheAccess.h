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
 // Something at a door, with nobody there: the nearest one within reach of this place moves. Three-space metres.
 bool MoveNearestDoor(const FVector& At,double Range);
 // The rooms' own lamps, 1 as built and 0 out.
 void SetLampLevel(double K);
 // Test: the middle of a room's floor, by its id or use.
 bool RoomMiddle(const FString& Id,FVector& Out);
}

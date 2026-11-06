#pragma once
#include "CoreMinimal.h"

// Block model of the abandoned farmhouse (Docs/Design/Concepts/Farmhouse, plan v1). Road-local coordinates,
// Freeform only. An older empty house on the property; the Alderete house itself stays lived in.
namespace S47FarmHouse
{
 enum class EMat : uint8 { Wall, Trim, Roof, Floor };
 struct FPiece { FString Name; FVector Centre, Size; EMat Mat; };
 // Everything visible: walls cut by their doors and windows, floor, ceiling, roof and veranda.
 TArray<FPiece> Pieces();
 // Footprints that block walking: walls, with doors left open and windows closed.
 TArray<FBox2D> Solids();
 bool OnPad(double RoadX,double RoadZ,double Margin=0);
 double PadY();
 // Road-local points in the plan's own rooms, for tests and later events.
 FVector Point(double PlanX,double PlanY);
 bool SelfTest(TArray<FString>& Lines);
}

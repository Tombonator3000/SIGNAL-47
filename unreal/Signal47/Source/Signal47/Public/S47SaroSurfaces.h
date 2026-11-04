#pragma once
#include "CoreMinimal.h"
class AS47Web;
class UStaticMesh;
class UMaterialInterface;
namespace S47SaroSurfaces
{
 // Single planar finish, with disjoint asphalt/concrete interiors. Gameplay stays at -.600m.
 constexpr double Height=-.576;
 UStaticMesh* Mesh(AS47Web* Web,UMaterialInterface* Material,bool Concrete);
}

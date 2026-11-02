#pragma once
#include "CoreMinimal.h"
class AS47Web;
class UStaticMeshComponent;
class UStaticMesh;
// One existing model at the original gas-station call anchor, with a precise legacy cut.
namespace S47StationPhone
{
 void Register(AS47Web* World,UStaticMeshComponent* Model);
 void Update(AS47Web* World);
 // Accept only this helper's exact installed remainder, never a Geo/count-only substitute.
 bool IsVerifiedRemainder(const AS47Web* World,const UStaticMeshComponent* Component,const UStaticMesh* Source);
 bool Ready(const AS47Web* World);
 int32 RemovedTriangles(const AS47Web* World);
}

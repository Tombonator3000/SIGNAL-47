#pragma once
#include "CoreMinimal.h"

class AS47Web;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// Authored hardware only. Existing native lights retain their feed and gameplay ownership.
namespace S47ExteriorFixtures
{
 enum class EKind { WallPack, Canopy, SignWash };
 struct FFixture
 {
  TWeakObjectPtr<UStaticMeshComponent> Mesh;
  TWeakObjectPtr<UMaterialInstanceDynamic> Lens;
  FVector Aperture = FVector::ZeroVector, Normal = FVector(0,0,1);
  float Width = 0, Height = 0; // centimetres, measured lens aperture
  bool bStreetLamp = false; // a cobra head (S47StreetLamps): its beam keeps the old target over the road, in sodium yellow
  explicit operator bool() const { return Mesh.IsValid() && Lens.IsValid(); }
  void Update(bool Shown,float Power,const FLinearColor& Color) const;
 };
 FFixture Make(AS47Web* W,const TCHAR* Name,EKind Kind,const FVector& Mount,const FQuat& Rotation,int32 Parent);
 TMap<FName,FFixture> ReplaceLegacy(AS47Web* W);
 float SaroPower(AS47Web* W);
}

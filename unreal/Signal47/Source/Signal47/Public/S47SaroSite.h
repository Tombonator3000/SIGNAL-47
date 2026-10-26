#pragma once
#include "CoreMinimal.h"
#include "S47ExteriorFixtures.h"
class AS47Web;
class UStaticMeshComponent;
class UMaterialInterface;
class USpotLightComponent;

// Shared physical footprint, in site-local Three metres. These two rectangles also
// prevent procedural foliage generation on the finish; no world-wide hiding.
namespace S47SaroForecourt
{
 constexpr double X0=20.1,X1=39.8,Z0=5.9,Z1=18.0;
 constexpr double AccessX0=10.0,AccessX1=39.8,AccessZ0=18.0,AccessZ1=23.0;
 constexpr double SedanX=35.0;
}
// Original wing facade, utility equipment and paved staff forecourt. Story/floors retained.
class FS47SaroSite
{
public:
 static TSharedPtr<FS47SaroSite> Build(AS47Web* W);
 void Update();
 bool Audit(TArray<FString>& Lines) const;
private:
 TWeakObjectPtr<AS47Web> Web;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Parts,Forecourt;
 TArray<FTransform> ForecourtPoses;
 TWeakObjectPtr<UStaticMeshComponent> Stains;
 TWeakObjectPtr<UMaterialInterface> StainAtlas;
 TArray<TWeakObjectPtr<USpotLightComponent>> SignWash;
 TArray<S47ExteriorFixtures::FFixture> SignFixtures;
 TArray<int32> PartNodes, RetiredNodes;
 int32 RemovedWalkFeet=0,RemovedDriveFeet=0;
 bool bShown=false;
};

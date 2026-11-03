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
 // EASTWING-01: the staff sedan parks west of the wing's garage apron (roll-up door x 33.2..37.6).
 constexpr double SedanX=24.6;
}
// Original wing facade, utility equipment and paved staff forecourt. Story/floors retained.
class FS47SaroSite
{
public:
 static TSharedPtr<FS47SaroSite> Build(AS47Web* W);
 void Update();
 bool Audit(TArray<FString>& Lines) const;
 // The native east wing (S47EastWing) owns the wing: its facade and glass stay hidden.
 void ReplaceWing(){bWingReplaced=true;Update();}
 bool WingReplaced() const { return bWingReplaced; }
private:
 TWeakObjectPtr<AS47Web> Web;
 bool bWingReplaced=false;
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

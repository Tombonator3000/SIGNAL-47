#pragma once

#include "CoreMinimal.h"

class US47Main;
class UPrimitiveComponent;
class UBoxComponent;
class UStaticMesh;
struct FS47Rect;

// Feet and paths use Three metres, Y up. The roof is a separate supported layer,
// never a destination which a ground agent may reach by climbing an invisible edge.
class SIGNAL47_API FS47RoswellWorld
{
public:
 struct FPoint
 {
  FString Id;
  FVector Pos = FVector::ZeroVector;
  bool bCover = false, bRoof = false, bWindow = false;
 };

 ~FS47RoswellWorld();
 void Init(US47Main* Main);
 // Only the group's own bodies belong here. Doors and structural proxies still block.
 TArray<TWeakObjectPtr<UPrimitiveComponent>> IgnoredComponents;
 static constexpr double Radius = .22, Height = 1.20, MaxStep = .24;

 TOptional<double> Floor(const FVector& NearFoot) const;
 bool IsStandable(const FVector& Foot) const;
 bool MoveAllowed(const FVector& From, const FVector& To) const;
 // Empty means no complete physical route; no partial/direct fallback is supplied.
 TArray<FVector> Path(const FVector& From, const FVector& To) const;
 bool ClearSight(const FVector& Eye, const FVector& Target) const;
 TArray<FPoint> Candidates(const FVector& Near, const FString& Scene, bool bRoof = false) const;

private:
 US47Main* M = nullptr;
 mutable TWeakObjectPtr<UBoxComponent> RoofQuery;
 mutable TWeakObjectPtr<UStaticMesh> VerifiedRoofMesh;
 mutable FTransform VerifiedRoofTransform;
 bool EnsureRoofSupport() const;
 // Cache only inside one synchronous Path call. Physical movement never consults it.
 mutable bool bPlanning = false;
 mutable int32 PlanningQueries = 0;
 mutable bool bPlanningExhausted = false;
 mutable TMap<FIntVector, TOptional<double>> PlanningSupport;
 mutable TMap<FIntVector, bool> PlanningStand;
 bool NativeQueryAllowed() const;
 bool RoofLayer(const FVector& P) const;
 bool InRoof(const FVector& P) const;
 bool GroundAllowed(const FVector& P) const;
 bool BodyClear(const FVector& From, const FVector& To) const;
 TOptional<double> Support(const FVector& P) const;
 double ExpectedGround(double X, double Z, double Fallback) const;
 TArray<FS47Rect> Blocks(bool bRoof) const;
};

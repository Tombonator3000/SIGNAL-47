#pragma once
#include "CoreMinimal.h"
class AS47Web;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/** ENV-03 (Unreal only): Caliche's hanging four-way amber flasher (ArtSource/TrafficSignal).
 *  The head swings a little in the wind about the span-wire clamp; the span stays put. Town.ts's
 *  blink stays the authority: the same flood slot 0, glows 0 and 1 and blink rhythm, but their
 *  places now follow the swinging head, and the native lenses carry the emissive. Installed only
 *  together with the town union trim that removes the old crossing (FS47TownBuildings). */
class FS47TownSignal
{
public:
 /** Validates both native meshes and the live crossing anchors; null keeps the old crossing. */
 static TSharedPtr<FS47TownSignal> Prepare(AS47Web* W);
 /** Creates the components and retires the old blink quads. Call after the trim commit. */
 void Commit(TArray<int32>& Retired);
 void Update();
 bool Audit(TArray<FString>& Lines) const;
 /** Swing about the wire / along it / twist, degrees, at time T and wind (strength, dir). */
 static FVector Swing(double T, double Strength, const FVector2D& Dir);
 FVector LastSwing() const { return Angles; }
private:
 TWeakObjectPtr<AS47Web> Web;
 class UStaticMesh *HeadMesh=nullptr,*SpanMesh=nullptr;
 TWeakObjectPtr<UStaticMeshComponent> Head,Span;
 TWeakObjectPtr<UMaterialInstanceDynamic> Lens;
 int32 Parent=-1,Flood=-1,GlowNodes[2]={-1,-1};
 FVector PivotWorld=FVector::ZeroVector,PivotLocal=FVector::ZeroVector,Angles=FVector::ZeroVector;
 FQuat Last=FQuat::Identity;float LastLevel=-1;
 static FQuat Rotation(const FVector& Deg);
};

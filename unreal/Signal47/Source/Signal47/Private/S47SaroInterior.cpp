#include "S47SaroInterior.h"

#include "S47Web.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace S47SaroInterior
{
namespace
{
 // Annex.ts: north skirting ends at z=4.83; south skirting begins at z=6.575.
 // The original walk rectangle (5.05..6.35) unnecessarily discarded 0.445 m.
 // Leave small measured setbacks from both skirts, without crossing either wall.
 constexpr double CorridorMinX = -5.75, CorridorMaxX = 1.75;
 constexpr double CorridorMinZ = 4.84, CorridorMaxZ = 6.57;
 constexpr double WalkerRadius = .27;
 const TCHAR* CorridorPath = TEXT("annex.zone.corridor");

 bool IsCorridor(const FS47Area& R, double MinZ, double MaxZ)
 {
  return R.bValid && FMath::IsNearlyEqual(R.MinX, CorridorMinX, 1e-6) &&
   FMath::IsNearlyEqual(R.MaxX, CorridorMaxX, 1e-6) &&
   FMath::IsNearlyEqual(R.MinZ, MinZ, 1e-6) && FMath::IsNearlyEqual(R.MaxZ, MaxZ, 1e-6);
 }
}

bool Build(AS47Web* W)
{
 if (!W || !W->IsLoaded() || FParse::Param(FCommandLine::Get(), TEXT("S47SaroInteriorOff")) ||
     FParse::Param(FCommandLine::Get(), TEXT("S47PropsOff"))) return false;
 if (W->Handle(TEXT("annex.group")) < 0 || W->Handle(TEXT("annex.interior")) < 0) return false;
 const FS47Area Old = W->Rect(CorridorPath);
 if (!IsCorridor(Old, 5.05, 6.35) && !IsCorridor(Old, CorridorMinZ, CorridorMaxZ))
 {
  UE_LOG(LogTemp, Warning, TEXT("S47: SARO corridor correction skipped: original bounds differ"));
  return false;
 }
 FS47Area Bounds = Old; Bounds.MinZ = CorridorMinZ; Bounds.MaxZ = CorridorMaxZ;
 // All four exported references name this rectangle. No append/reorder:
 // existing door.Zone indices and closed/open/restore semantics retain their identity.
 if (!W->SetWalkZoneBounds(TEXT("saro"), TEXT("corridor"), Bounds,
     {TEXT("annex.zone.corridor"), TEXT("annex.zones[1]"), TEXT("world.d.player.zones[10]"), TEXT("world.d.saro.zones[10]")}))
 {
  UE_LOG(LogTemp, Warning, TEXT("S47: SARO corridor correction skipped: walk zone/aliases disagree"));
  return false;
 }
 const bool bVending = W->RecessSaroVending(true);
 const bool bLighting = W->RehouseSaroCorridorLights(true);
 if (!bLighting) UE_LOG(LogTemp, Warning, TEXT("S47: native corridor fixture rehouse pending: geometry/slots/drivers differ"));
 if (!bVending)
  UE_LOG(LogTemp, Warning, TEXT("S47: SARO vending relocation skipped: bay/object/item/collider/light contract differs; original retained"));
 UE_LOG(LogTemp, Display, TEXT("S47: SARO corridor candidate walk width=%.3fm centre travel=%.3fm; vending relocation=%s; original use/photo/power identities retained"),
  CorridorMaxZ - CorridorMinZ, CorridorMaxZ - CorridorMinZ - 2 * WalkerRadius, bVending ? TEXT("APPLIED") : TEXT("PENDING"));
 return true;
}

bool ContainsPassingBay(AS47Web* W, const FVector& P)
{
 if (!W || !W->IsLoaded() || W->Area() != TEXT("saro") || P.Y < -2 || P.Y > 4) return false;
 // Validate the actual native zone, never expand the full old corridor rectangle
 // across the unchanged west records wall. Only the new eastern bay is added.
 int32 Bays = 0; bool bBay = false, bThreshold = false;
 for (const FS47Rect& R : W->ZonesMutable(TEXT("saro")))
 {
  if (R.Id == TEXT("office:passingBayZone"))
  {
   ++Bays;
   bBay = R.bEnabled && FMath::IsNearlyEqual(R.MinX, -.82, 1e-6) && FMath::IsNearlyEqual(R.MaxX, 1.77, 1e-6) &&
    FMath::IsNearlyEqual(R.MinZ, 5.70, 1e-6) && FMath::IsNearlyEqual(R.MaxZ, 7.10, 1e-6);
  }
  if (R.Id == TEXT("office:thresholdZone"))
   bThreshold = R.bEnabled && FMath::IsNearlyEqual(R.MinX, -.08, 1e-6) && FMath::IsNearlyEqual(R.MaxX, 1, 1e-6) &&
    FMath::IsNearlyEqual(R.MinZ, 5.70, 1e-6) && FMath::IsNearlyEqual(R.MaxZ, 8.0, 1e-6);
 }
 if (Bays != 1 || !bBay) return false;
 const bool bInBay = P.X >= -.82 && P.X <= 1.77 && P.Z >= CorridorMaxZ && P.Z <= 7.10;
 // The open aperture also bridges the 20cm wall depth before Office::Contains.
 // Keep that genuine room-entry boundary unchanged for bEntered/story facts.
 const bool bInAperture = bThreshold && P.X >= -.08 && P.X <= 1 && P.Z >= 7.10 && P.Z <= 7.30;
 if (!bInBay && !bInAperture) return false;
 for (const FS47Rect& C : W->CollidersMutable(TEXT("saro")))
  if (C.bEnabled && P.X > C.MinX && P.X < C.MaxX && P.Z > C.MinZ && P.Z < C.MaxZ) return false;
 return true;
}

bool Audit(AS47Web* W, TArray<FString>& Lines)
{
 if (!W) return false;
 bool bSemantic = true;
 for (const TCHAR* Path : {CorridorPath, TEXT("annex.zones[1]"), TEXT("world.d.player.zones[10]"), TEXT("world.d.saro.zones[10]")})
  bSemantic &= IsCorridor(W->Rect(Path), CorridorMinZ, CorridorMaxZ);
 int32 Count = 0; bool bWalk = false;
 for (const FS47Rect& R : W->ZonesMutable(TEXT("saro"))) if (R.Id == TEXT("corridor"))
 {
  ++Count;
  bWalk = R.bEnabled && FMath::IsNearlyEqual(R.MinX, CorridorMinX, 1e-6) &&
   FMath::IsNearlyEqual(R.MaxX, CorridorMaxX, 1e-6) && FMath::IsNearlyEqual(R.MinZ, CorridorMinZ, 1e-6) &&
   FMath::IsNearlyEqual(R.MaxZ, CorridorMaxZ, 1e-6);
 }
 const bool bBounds = bSemantic && bWalk && Count == 1;
 Lines.Add(FString::Printf(TEXT("SARO corridor bounds %s: zone+four semantic aliases, width %.3fm, radius %.3fm, centre travel %.3fm"),
  bBounds ? TEXT("PASS") : TEXT("FAIL"), CorridorMaxZ - CorridorMinZ, WalkerRadius, CorridorMaxZ - CorridorMinZ - 2 * WalkerRadius));
 const FS47Obj Machine = W->Obj(TEXT("annex.objs.vending"));
 const bool bMoved = Machine && Machine.WorldPosition().Equals(FVector(1.35, 0, 6.55), .0001) && W->RecessSaroVending(false);
 Lines.Add(FString::Printf(TEXT("SARO vending coupled relocation %s: original group/item/photo, collider+alias, physical screen light and annex pool slot2; north clearance=1.310m, centre travel=.770m"),
  bMoved ? TEXT("PASS") : TEXT("FAIL")));
 const bool bBay = ContainsPassingBay(W, FVector(.47, 0, 6.8)) && !ContainsPassingBay(W, FVector(-1.2, 0, 6.9)) &&
  !ContainsPassingBay(W, FVector(1.35, 0, 6.8));
 Lines.Add(FString::Printf(TEXT("SARO passing bay semantics %s: new clear eastern bay indoors, unchanged records wall and vending volume excluded"), bBay ? TEXT("PASS") : TEXT("FAIL")));
 Lines.Add(TEXT("SARO interior candidate: physical route, existing-save restore and rendered geometry require runtime verification"));
 const bool bLighting = W->RehouseSaroCorridorLights(false);
 Lines.Add(FString::Printf(TEXT("SARO corridor physical lighting %s: two existing tube drivers, original pool suppression/power/flicker retained"),bLighting?TEXT("PASS"):TEXT("FAIL")));
 return bBounds && bMoved && bBay && bLighting;
}
}

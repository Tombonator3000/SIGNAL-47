#include "S47RoswellWorld.h"

#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "StaticMeshResources.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Props.h"
#include "S47ThreatNavigation.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"

namespace
{
 bool Finite(const FVector& P) { return FMath::IsFinite(P.X) && FMath::IsFinite(P.Y) && FMath::IsFinite(P.Z); }
 FVector2D Flat(const FVector& P) { return FVector2D(P.X, P.Z); }
 FIntVector Millimetres(const FVector& P) { return FIntVector(FMath::RoundToInt(P.X * 1000), FMath::RoundToInt(P.Y * 1000), FMath::RoundToInt(P.Z * 1000)); }
 bool RoofDiagnostics() { static const bool On = FParse::Param(FCommandLine::Get(), TEXT("S47RoswellRoofOnly")); return On; }
 bool GroundDiagnostics()
 {
  static const bool On = [] { FString Path; return FParse::Value(FCommandLine::Get(), TEXT("S47RoswellAIContract="), Path); }();
  return On;
 }
 FCollisionQueryParams Query(const TArray<TWeakObjectPtr<UPrimitiveComponent>>& Ignore)
 {
  FCollisionQueryParams Q(SCENE_QUERY_STAT(S47RoswellPhysical), true);
  Q.bFindInitialOverlaps = true;
  for (const auto& P : Ignore) if (const UPrimitiveComponent* C = P.Get()) Q.AddIgnoredComponent(C);
  return Q;
 }
 bool Inside(const FS47Rect& R, const FVector& P, double Pad)
 {
  return R.bEnabled && P.X >= R.MinX - Pad && P.X <= R.MaxX + Pad && P.Z >= R.MinZ - Pad && P.Z <= R.MaxZ + Pad;
 }
 FS47Rect RoswellRect(double X0, double X1, double Z0, double Z1)
 {
  FS47Rect R; R.MinX = X0; R.MaxX = X1; R.MinZ = Z0; R.MaxZ = Z1; return R;
 }
}

FS47RoswellWorld::~FS47RoswellWorld()
{
 if (RoofQuery.IsValid()) RoofQuery->DestroyComponent();
}

void FS47RoswellWorld::Init(US47Main* Main)
{
 if (RoofQuery.IsValid()) RoofQuery->DestroyComponent();
 RoofQuery.Reset(); VerifiedRoofMesh.Reset(); M = Main;
}

bool FS47RoswellWorld::EnsureRoofSupport() const
{
 if (!M || !M->Web || !M->Web->GetWorld()) return false;
 AS47Web* W = M->Web;
 const int32 Id = W->Handle(TEXT("annex.outside"));
 const auto* N = W->Node(Id);
 auto* C = N ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
 UStaticMesh* Mesh = C ? C->GetStaticMesh() : nullptr;
 const bool Visible = N && N->Geo == 313 && N->Name == TEXT("house:annex") && C && C->IsVisible() && W->ObjAt(Id).Shown();
 if (!Visible || !Mesh)
 {
  if (RoofQuery.IsValid()) RoofQuery->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  return false;
 }
 const FTransform Transform = C->GetComponentTransform();
 if (RoofQuery.IsValid() && VerifiedRoofMesh.Get() == Mesh && Transform.Equals(VerifiedRoofTransform, .001))
 {
  RoofQuery->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  return true;
 }
 if (RoofQuery.IsValid()) RoofQuery->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 // The rear-window renderer replaces g313 with a transient cut mesh with no
 // BodySetup. Verify its CURRENT visible top triangles before restoring only
 // the actual slab's query volume; never restore the cut rear wall/window.
 if (!Mesh->bAllowCPUAccess || !Mesh->GetRenderData() || Mesh->GetRenderData()->LODResources.IsEmpty()) return false;
 const auto& LOD = Mesh->GetRenderData()->LODResources[0];
 const auto& Positions = LOD.VertexBuffers.PositionVertexBuffer;
 if (!Positions.GetVertexData() || !LOD.IndexBuffer.GetIndexDataSize()) return false;
 TArray<uint32> Indices; LOD.IndexBuffer.GetCopy(Indices);
 struct FTop { FVector2D A, B, C; };
 TArray<FTop> Top;
 auto Point = [&](uint32 I) { return S47Three::ToThree(Transform.TransformPosition(FVector(Positions.VertexPosition(I)))); };
 for (int32 I = 0; I + 2 < Indices.Num(); I += 3)
 {
  if (FMath::Max3(Indices[I], Indices[I + 1], Indices[I + 2]) >= Positions.GetNumVertices()) return false;
  const FVector A = Point(Indices[I]), B = Point(Indices[I + 1]), D = Point(Indices[I + 2]);
  if (FMath::Abs(A.Y - 3.2) <= .003 && FMath::Abs(B.Y - 3.2) <= .003 && FMath::Abs(D.Y - 3.2) <= .003)
   Top.Add({Flat(A), Flat(B), Flat(D)});
 }
 auto Covered = [&Top](double X, double Z)
 {
  const FVector2D P(X, Z);
  auto Cross = [](const FVector2D& A, const FVector2D& B) { return A.X * B.Y - A.Y * B.X; };
  for (const FTop& T : Top)
  {
   const double D = Cross(T.B - T.A, T.C - T.A);
   if (FMath::Abs(D) < 1e-10) continue;
   const double U = Cross(P - T.A, T.C - T.A) / D, V = Cross(T.B - T.A, P - T.A) / D;
   if (U >= -1e-6 && V >= -1e-6 && U + V <= 1.000001) return true;
  }
  return false;
 };
 // Full footprint including each inset edge, not merely one surviving corner.
 for (double X : {-6.29, -5.7, -2., 1.65, 2.29})
  for (double Z : {4.81, 5.4, 7.6, 10.2, 10.79}) if (!Covered(X, Z)) return false;
 if (!RoofQuery.IsValid())
 {
  auto* Box = NewObject<UBoxComponent>(W, TEXT("RoswellAnnexRoofQuery"));
  W->AddInstanceComponent(Box); Box->SetMobility(EComponentMobility::Movable);
  Box->SetupAttachment(W->GetRootComponent()); Box->SetCanEverAffectNavigation(false);
  Box->SetGenerateOverlapEvents(false); Box->SetCollisionResponseToAllChannels(ECR_Ignore);
  Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); Box->SetHiddenInGame(true);
  Box->SetBoxExtent(S47Three::ToUE(4.3, .2, 3.)); Box->RegisterComponent();
  Box->SetWorldLocation(S47Three::ToUE(-2., 3., 7.8)); RoofQuery = Box;
 }
 VerifiedRoofMesh = Mesh; VerifiedRoofTransform = Transform;
 RoofQuery->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 UE_LOG(LogTemp, Display, TEXT("S47: Roswell Annex roof query restored only for verified visible g313 deck: %s; 25 render-support samples, slab [-6.3,2.3]x[4.8,10.8], Y2.8..3.2"), *Mesh->GetName());
 return true;
}

bool FS47RoswellWorld::NativeQueryAllowed() const
{
 // Count only actual traces/sweeps, not cached support. Movement outside Path
 // is deliberately unlimited and must always query the current doors/geometry.
 if (!bPlanning) return true;
 if (PlanningQueries >= 1500) { bPlanningExhausted = true; return false; }
 ++PlanningQueries;
 return true;
}

bool FS47RoswellWorld::RoofLayer(const FVector& P) const
{
 return M && M->Web && M->Web->Area() == TEXT("saro") && P.Y > 2.6;
}

bool FS47RoswellWorld::InRoof(const FVector& P) const
{
 // Annex.ts / S47RoofSteps: actual slab -6.3..2.3, 4.8..10.8, top 3.2.
 // Keep the entire capsule and its support samples away from the open lip.
 return P.X >= -6.3 + Radius + .06 && P.X <= 2.3 - Radius - .06 &&
  P.Z >= 4.8 + Radius + .06 && P.Z <= 10.8 - Radius - .06 && FMath::Abs(P.Y - 3.2) <= .16;
}

double FS47RoswellWorld::ExpectedGround(double X, double Z, double Fallback) const
{
 if (!M || !M->Player || !M->Web) return Fallback;
 double Y = AS47Props::FloorAt(X, Z);
 if (FMath::IsFinite(Y)) return Y;
 if (M->Player->LandFloor) { Y = M->Player->LandFloor(X, Z); if (FMath::IsFinite(Y)) return Y; }
 if (M->Player->Floor) { Y = M->Player->Floor(X, Z); if (FMath::IsFinite(Y)) return Y; }
 if (M->Web->Area() == TEXT("saro")) return US47Night::SaroFloorAt(M->Web, X, Z);
 return Fallback;
}

TOptional<double> FS47RoswellWorld::Support(const FVector& P) const
{
 if (!Finite(P) || !M || !M->Web || !M->Web->GetWorld()) return {};
 const FIntVector Key = Millimetres(P);
 if (bPlanning) if (const auto* Existing = PlanningSupport.Find(Key)) return *Existing;
 if (!NativeQueryAllowed()) return {};
 FHitResult H;
 auto Q = Query(IgnoredComponents);
 // These three named interaction boxes describe a look target, never a floor.
 for (const TCHAR* Handle : {TEXT("annex.frost.hit.unit"), TEXT("annex.frost.hit.sw"), TEXT("annex.frost.hit.se")})
  if (const auto* N = M->Web->Node(M->Web->Handle(Handle)))
   if (const auto* C = Cast<UPrimitiveComponent>(N->Comp.Get())) Q.AddIgnoredComponent(C);
 TOptional<double> Result;
 // No long trace to a lower storey: a missing roof/floor is a rejected point.
 if (M->Web->GetWorld()->LineTraceSingleByChannel(H, S47Three::ToUE(P + FVector(0, .28, 0)),
  S47Three::ToUE(P - FVector(0, .34, 0)), ECC_Visibility, Q) && H.ImpactNormal.Z >= .68)
  Result = S47Three::ToThree(H.ImpactPoint).Y;
 if (bPlanning) PlanningSupport.Add(Key, Result);
 return Result;
}

TOptional<double> FS47RoswellWorld::Floor(const FVector& NearFoot) const
{
 if (!Finite(NearFoot)) return {};
 const bool bRoof = RoofLayer(NearFoot);
 if (bRoof && !InRoof(NearFoot)) return {};
 if (bRoof && !EnsureRoofSupport()) return {};
 // A native sidewalk/ramp may differ from the legacy area height. Prefer the
 // nearby physical surface; the authored height is a second measured query,
 // never a returned floor when native support is absent.
 auto Y = Support(NearFoot);
 if (!Y.IsSet() && !bRoof)
 {
  FVector P = NearFoot; P.Y = ExpectedGround(P.X, P.Z, P.Y);
  if (FMath::Abs(P.Y - NearFoot.Y) <= .8) Y = Support(P);
 }
 if (!Y.IsSet()) return {};
 if (bRoof && FMath::Abs(Y.GetValue() - 3.2) > .16) return {};
 return Y;
}

TOptional<FVector> FS47RoswellWorld::GroundedStep(const FVector& From, const FVector& Desired) const
{
 if (!Finite(From) || !Finite(Desired) || RoofLayer(From) != RoofLayer(Desired)) return {};
 FVector Planted = Desired;
 const auto Y = Floor(Planted);
 if (!Y.IsSet() || FMath::Abs(Y.GetValue() - From.Y) > MaxStep) return {};
 Planted.Y = Y.GetValue();
 if (!IsStandable(Planted) || !MoveAllowed(From, Planted)) return {};
 return Planted;
}

TArray<FS47Rect> FS47RoswellWorld::Blocks(bool bRoof) const
{
 // Roof equipment is .86m above the slab and cannot be stepped onto.
 if (bRoof) return {RoswellRect(-5.0, -3.4, 8.05, 9.15)};
 if (M && M->Web) return M->Web->Colliders();
 return {};
}

bool FS47RoswellWorld::GroundAllowed(const FVector& P) const
{
 if (!M || !M->Web || !M->Player) return false;
 if (RoofLayer(P)) return InRoof(P);
 // Walkable includes the current area's open terrain, with its real world origin.
 // It is deliberately slightly more conservative than this smaller actor's radius.
 return M->Player->Walkable(P.X, P.Z);
}

bool FS47RoswellWorld::BodyClear(const FVector& From, const FVector& To) const
{
 if (!M || !M->Web || !M->Web->GetWorld()) return false;
 const auto Q = Query(IgnoredComponents);
 FHitResult H;
 // Sole clearance prevents a supporting slab's triangle from becoming a body wall.
 const FVector Lift(0, Height * .5 + .035, 0);
 const auto Capsule = FCollisionShape::MakeCapsule(Radius * 100, Height * 50);
 const bool bGroundQuery = !RoofLayer(From);
 auto Clear = [this, &Q, &H, &Capsule, bGroundQuery](const FVector& A, const FVector& B)
 {
  if (!NativeQueryAllowed()) return false;
  const bool Clear = !M->Web->GetWorld()->SweepSingleByChannel(H, S47Three::ToUE(A), S47Three::ToUE(B),
   FQuat::Identity, ECC_Visibility, Capsule, Q);
  if (!Clear && RoofDiagnostics() && A.Y > 3)
  {
   static int32 Logged = 0;
   if (Logged++ < 30)
   {
    const auto* C = H.GetComponent(); const auto* N = M->Web->Node(M->Web->NodeOf(C));
    UE_LOG(LogTemp, Display, TEXT("S47: Roswell roof capsule blocked A=%s B=%s component=%s node=%s hit=%s normal=%s penetrating=%d"),
     *A.ToCompactString(), *B.ToCompactString(), *GetNameSafe(C), N ? *N->Name : TEXT("none"),
     *S47Three::ToThree(H.ImpactPoint).ToCompactString(), *H.ImpactNormal.ToCompactString(), H.bStartPenetrating ? 1 : 0);
   }
  }
  if (!Clear && bGroundQuery && GroundDiagnostics())
  {
   static int32 Logged = 0;
   if (Logged++ < 64)
   {
    const auto* C = H.GetComponent(); const auto* N = M->Web->Node(M->Web->NodeOf(C));
    UE_LOG(LogTemp, Display, TEXT("S47: Roswell ground capsule blocked planning=%d A=%s B=%s component=%s node=%s hit=%s normal=%s penetrating=%d"),
     bPlanning ? 1 : 0, *A.ToCompactString(), *B.ToCompactString(), *GetNameSafe(C), N ? *N->Name : TEXT("none"),
     *S47Three::ToThree(H.ImpactPoint).ToCompactString(), *H.ImpactNormal.ToCompactString(), H.bStartPenetrating ? 1 : 0);
   }
  }
  return Clear;
 };
 const FVector A = From + Lift, B = To + Lift;
 if (Clear(A, B)) return true;
 if (From.Equals(To, .001) || FMath::Abs(From.Y - To.Y) > MaxStep) return false;
 // A genuine bounded stair step checks the upward, across, and downward legs.
 // The full capsule clears each leg; a wall/door cannot be ignored as a floor hit.
 const FVector Step(0, MaxStep, 0);
 return Clear(A, A + Step) && Clear(A + Step, B + Step) && Clear(B + Step, B);
}

bool FS47RoswellWorld::IsStandable(const FVector& Foot) const
{
 if (!Finite(Foot)) return false;
 const FIntVector Key = Millimetres(Foot);
 if (bPlanning) if (const bool* Existing = PlanningStand.Find(Key)) return *Existing;
 auto Check = [this, &Foot]()
 {
 if (!GroundAllowed(Foot)) return false;
 const auto Y = Floor(Foot);
 if (!Y.IsSet() || FMath::Abs(Y.GetValue() - Foot.Y) > .10) return false;
 for (const FS47Rect& R : Blocks(RoofLayer(Foot))) if (Inside(R, Foot, Radius)) return false;
 // Centre plus four sole samples rejects cracks and unsupported roof/terrain edges.
 for (const FVector& Offset : {FVector(.16, 0, 0), FVector(-.16, 0, 0), FVector(0, 0, .16), FVector(0, 0, -.16)})
 {
  const auto Edge = Support(Foot + Offset);
  if (!Edge.IsSet() || FMath::Abs(Edge.GetValue() - Foot.Y) > MaxStep) return false;
 }
 return BodyClear(Foot, Foot);
 };
 const bool bClear = Check();
 if (bPlanning) PlanningStand.Add(Key, bClear);
 return bClear;
}

bool FS47RoswellWorld::MoveAllowed(const FVector& From, const FVector& To) const
{
 if (!Finite(From) || !Finite(To) || RoofLayer(From) != RoofLayer(To) || !IsStandable(From) || !IsStandable(To)) return false;
 const double Distance = FVector2D::Distance(Flat(From), Flat(To));
 if (Distance > 45) return false;
 const int32 Steps = FMath::Max(1, FMath::CeilToInt(Distance / .20));
 FVector Previous = From;
 const auto Obstacles = Blocks(RoofLayer(From));
 for (int32 I = 1; I <= Steps; ++I)
 {
  FVector Next = FMath::Lerp(From, To, double(I) / Steps);
  const auto Y = Floor(Next);
  if (!Y.IsSet()) return false;
  Next.Y = Y.GetValue();
  if (FMath::Abs(Next.Y - Previous.Y) > MaxStep || !GroundAllowed(Next)) return false;
  for (const FS47Rect& R : Obstacles) if (Inside(R, Next, Radius)) return false;
  if (!BodyClear(Previous, Next)) return false;
  // Support across the whole sole also matters between waypoints, especially at roof lips.
  for (const FVector& Offset : {FVector(.16, 0, 0), FVector(-.16, 0, 0), FVector(0, 0, .16), FVector(0, 0, -.16)})
  {
   const auto Edge = Support(Next + Offset);
   if (!Edge.IsSet() || FMath::Abs(Edge.GetValue() - Next.Y) > MaxStep) return false;
  }
  Previous = Next;
 }
 return true;
}

TArray<FVector> FS47RoswellWorld::Path(const FVector& From, const FVector& To) const
{
 int32 ReturnedPoints = 0;
 bool bCounted = false;
 const TCHAR* Outcome = TEXT("invalid request");
 const TCHAR* Method = TEXT("none");
 ON_SCOPE_EXIT
 {
  if (GroundDiagnostics())
  {
   static int32 Logged = 0;
   if (Logged++ < 96)
    UE_LOG(LogTemp, Display, TEXT("S47: Roswell native path from=%s to=%s roof=%d method=%s outcome=%s points=%d queries=%d exhausted=%d"),
     *From.ToCompactString(), *To.ToCompactString(), RoofLayer(From) ? 1 : 0, Method, Outcome,
     ReturnedPoints, bCounted ? PlanningQueries : 0, bCounted && bPlanningExhausted ? 1 : 0);
  }
 };
 if (!M || !M->Web || !Finite(From) || !Finite(To) || RoofLayer(From) != RoofLayer(To) ||
  FVector2D::Distance(Flat(From), Flat(To)) > 35) return {};
 PlanningSupport.Reset(); PlanningStand.Reset(); PlanningQueries = 0; bPlanningExhausted = false;
 bCounted = true; Outcome = TEXT("endpoint not standable");
 TGuardValue<bool> PlanningScope(bPlanning, true);
 if (!IsStandable(From) || !IsStandable(To)) return {};
 const bool bRoof = RoofLayer(From);
 auto Lift = [this, &From](const FVector2D& P) -> TOptional<FVector>
 {
  FVector V(P.X, From.Y, P.Y);
  const auto Y = Floor(V);
  if (!Y.IsSet()) return {};
  V.Y = Y.GetValue(); return V;
 };
 TArray<FVector2D> Route;
 // The common open-ground/roof case needs no graph or candidate collision pass.
 Method = TEXT("direct");
 if (MoveAllowed(From, To)) Route.Add(Flat(To));
 else
 {
 Outcome = TEXT("native query budget");
 if (bPlanningExhausted) return {};
 Method = TEXT("graph");
 TArray<FVector2D> Seeds;
 for (const FPoint& P : Candidates((From + To) * .5, M->Web->Area(), bRoof)) Seeds.Add(Flat(P.Pos));
 if (bPlanningExhausted) return {};
 Route = FS47ThreatNavigation::FindPath(Flat(From), Flat(To), Seeds, Blocks(bRoof), Radius,
  [this, &Lift](const FVector2D& A, const FVector2D& B)
  {
   const auto X = Lift(A), Y = Lift(B);
   return X.IsSet() && Y.IsSet() && MoveAllowed(X.GetValue(), Y.GetValue());
  });
 }
 Outcome = bPlanningExhausted ? TEXT("native query budget") : TEXT("no complete graph route");
 if (bPlanningExhausted || Route.IsEmpty()) return {};
 Outcome = TEXT("support lost while lifting route");
 TArray<FVector> Result;
 FVector Previous = From;
 // Densify returned 3D path so a raised strip/ramp's height is not interpolated as flat.
 for (const FVector2D& P : Route)
 {
  const auto End = Lift(P);
  if (!End.IsSet()) return {};
  const FVector Begin = Previous;
  const int32 Count = FMath::Max(1, FMath::CeilToInt(FVector2D::Distance(Flat(Begin), P) / .20));
  for (int32 I = 1; I <= Count; ++I)
  {
   const auto Next = Lift(FMath::Lerp(Flat(Begin), P, double(I) / Count));
   if (!Next.IsSet() || FMath::Abs(Next.GetValue().Y - Previous.Y) > MaxStep) return {};
   Result.Add(Next.GetValue()); Previous = Next.GetValue();
  }
 }
 // Keep measured ramps/height transitions but collapse truly linear runs. The brain
 // accepts <=96 points; a 35m flat path must not fail merely because it was sampled.
 TArray<FVector> Compact;
 FVector Anchor = From;
 int32 Begin = 0;
 while (Begin < Result.Num())
 {
  int32 End = Begin;
  for (int32 Candidate = Begin + 1; Candidate < Result.Num(); ++Candidate)
  {
   const FVector D = Result[Candidate] - Anchor;
   const double LengthSq = D.SizeSquared();
   bool bLinear = LengthSq > 1e-9;
   for (int32 J = Begin; bLinear && J < Candidate; ++J)
   {
    const double T = FVector::DotProduct(Result[J] - Anchor, D) / LengthSq;
    bLinear = T >= 0 && T <= 1 && FVector::DistSquared(Result[J], Anchor + D * T) <= .008 * .008;
   }
   if (!bLinear) break;
   End = Candidate;
  }
  Compact.Add(Result[End]); Anchor = Result[End]; Begin = End + 1;
 }
 if (bPlanningExhausted) { Outcome = TEXT("native query budget"); return {}; }
 if (Compact.Num() > 96) { Outcome = TEXT("too many compact waypoints"); return {}; }
 Outcome = TEXT("complete native route"); ReturnedPoints = Compact.Num();
 return Compact;
}

bool FS47RoswellWorld::ClearSight(const FVector& Eye, const FVector& Target) const
{
 if (!Finite(Eye) || !Finite(Target) || !M || !M->Web || !M->Web->GetWorld()) return false;
 auto Q = Query(IgnoredComponents);
 for (int32 I = 0; I < 16; ++I)
 {
  FHitResult H;
  if (!M->Web->GetWorld()->LineTraceSingleByChannel(H, S47Three::ToUE(Eye), S47Three::ToUE(Target), ECC_Visibility, Q)) return true;
  UPrimitiveComponent* C = H.GetComponent();
  if (!C) return false;
  bool bTransparent = false;
  if (const UMaterialInterface* Mat = C->GetMaterial(FMath::Max<int32>(0, H.ElementIndex)))
  {
   float Opacity = 1; Mat->GetScalarParameterValue(TEXT("Opacity"), Opacity);
   bTransparent = Mat->GetBlendMode() == BLEND_Additive || (Mat->GetBlendMode() == BLEND_Translucent && Opacity < .5f);
  }
  // Hidden source/proxy walls remain solid unless deliberately collision-disabled by their owner.
  if (!bTransparent) return false;
  Q.AddIgnoredComponent(C);
 }
 return false;
}

TArray<FS47RoswellWorld::FPoint> FS47RoswellWorld::Candidates(const FVector& Near, const FString& Scene, bool bRoof) const
{
 TArray<FPoint> Out;
 if (!M || !M->Web || Scene != M->Web->Area() || !Finite(Near)) return Out;
 auto Add = [this, &Out, bRoof](const TCHAR* Id, double X, double Y, double Z, bool bCover, bool bWindow)
 {
  FVector P(X, Y, Z);
  const auto FloorY = Floor(P);
  if (bRoof && RoofDiagnostics())
  {
   static TSet<FString> Logged;
   if (!Logged.Contains(Id))
   {
    Logged.Add(Id);
    FHitResult H; const auto Q = Query(IgnoredComponents);
    const bool Hit = M->Web->GetWorld()->LineTraceSingleByChannel(H, S47Three::ToUE(X, 4.6, Z), S47Three::ToUE(X, 2.7, Z), ECC_Visibility, Q);
    const auto* C = H.GetComponent(); const auto* N = M->Web->Node(M->Web->NodeOf(C));
    UE_LOG(LogTemp, Display, TEXT("S47: Roswell roof support id=%s narrow=%d y=%.3f broad=%d broadY=%.3f component=%s node=%s visible=%d normal=%s"),
     Id, FloorY.IsSet() ? 1 : 0, FloorY.Get(0.), Hit ? 1 : 0, S47Three::ToThree(H.ImpactPoint).Y,
     *GetNameSafe(C), N ? *N->Name : TEXT("none"), C && C->IsVisible() ? 1 : 0, *H.ImpactNormal.ToCompactString());
   }
  }
  if (!FloorY.IsSet()) return;
  P.Y = FloorY.GetValue();
  if (IsStandable(P)) Out.Add({Id, P, bCover, bRoof, bWindow});
 };
 if (bRoof)
 {
  if (Scene != TEXT("saro")) return Out;
  // Paths stay on the actual Annex slab, around its raised equipment, above the occupied rooms.
  Add(TEXT("roof-west-north"), -5.7, 3.2, 5.4, true, false);
  Add(TEXT("roof-mid-north"), -2.2, 3.2, 5.4, false, false);
  Add(TEXT("roof-east-north"), 1.65, 3.2, 5.4, true, false);
  Add(TEXT("roof-east-middle"), 1.65, 3.2, 7.6, false, false);
  Add(TEXT("roof-east-south"), 1.65, 3.2, 10.2, true, false);
  Add(TEXT("roof-mid-south"), -2.2, 3.2, 10.2, false, false);
  Add(TEXT("roof-west-south"), -5.7, 3.2, 10.2, true, false);
  Add(TEXT("roof-west-middle"), -5.7, 3.2, 7.5, true, false);
  Add(TEXT("roof-middle"), -1.4, 3.2, 7.6, false, false);
  return Out;
 }
 if (Scene == TEXT("saro") && FMath::Abs(Near.X) < 50 && FMath::Abs(Near.Z) < 40)
 {
  // Native support validates these approaches to the three authored changes
  // between desert (-.6) and the plinth (0). Collider corners alone cannot
  // discover the middle of a ramp; jumping straight over its .6m curb is invalid.
  Add(TEXT("visitor-ramp-low"), -10.95, -.6, 5.7, false, false);
  Add(TEXT("visitor-ramp-mid"), -9.1, -.3, 5.7, false, false);
  Add(TEXT("visitor-ramp-high"), -7.3, 0, 5.7, false, false);
  Add(TEXT("yard-ramp-low"), 9.8, -.6, 16.95, false, false);
  Add(TEXT("yard-ramp-mid"), 9.8, -.3, 14.5, false, false);
  Add(TEXT("yard-ramp-high"), 9.8, 0, 12.1, false, false);
  Add(TEXT("array-steps-low"), 6.7, -.6, -12, false, false);
  Add(TEXT("array-steps-mid"), 7.55, -.3, -12, false, false);
  Add(TEXT("array-steps-high"), 8.4, 0, -12, false, false);
  Add(TEXT("window-west"), -4.5, 0, -5.25, false, true);
  Add(TEXT("window-east"), .9, 0, -5.25, false, true);
  Add(TEXT("control-corner"), 6.95, -.6, -5.25, true, false);
  Add(TEXT("control-east"), 7.3, -.6, -3, false, false);
  Add(TEXT("control-west"), -7.2, -.6, -3.5, true, false);
  Add(TEXT("yard-west"), -7.3, -.6, 8, true, false);
  Add(TEXT("yard-south"), -2.7, -.6, 11.9, true, false);
  Add(TEXT("yard-east"), 4.3, -.6, 8, true, false);
  Add(TEXT("annex-side"), 3.1, -.6, 6.5, true, false);
  Add(TEXT("passing-bay"), 8.3, 0, 5.4, false, false);
  Add(TEXT("wing-corner"), 19.75, -.6, -6.35, true, false);
 }
 // These points use Near's world frame, not SARO-local anchors copied into the road/diner.
 // Native floors, the active walk zones and colliders reject scenery and inaccessible interiors.
 for (int32 Ring = 0; Ring < 3; ++Ring)
  for (int32 I = 0; I < 8; ++I)
  {
   const double Angle = I * UE_PI / 4, R = 2.6 + Ring * 2.9;
   const FString Id = FString::Printf(TEXT("local-%d-%d"), Ring, I);
   Add(*Id, Near.X + FMath::Cos(Angle) * R, Near.Y, Near.Z + FMath::Sin(Angle) * R, false, false);
  }
 return Out;
}

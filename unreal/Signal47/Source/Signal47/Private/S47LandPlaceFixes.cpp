#include "S47LandPlaceFixes.h"
#include "S47Drive.h"
#include "S47Freeform.h"
#include "S47Inter.h"
#include "S47Legs.h"
#include "S47Main.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47PredecessorText.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47Truck.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/PrimitiveComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47LandPlaceFixes, Log, All);

// LAND-PLACES-FIXES-20261114: see the header.
namespace S47LandPlaceFixes
{
namespace Detail
{
 // the shack's fourth basic shape is its window (FS47Places PartsOf: the box, the roof, the door, the window)
 const FSolid SolidTable[] = { { TEXT("shack"), EShape::Box, 3 }, { TEXT("cairn"), EShape::Round, -1 } };
 // The truck's obstacles are never taken out of its list (others hold places in it by index): sent this far off when not wanted.
 constexpr double Away = 1e6;
 constexpr double Walker = .27; // AS47Walker's radius: walk boxes grow by it

 // Whether the place's first part (FS47Places: the one its use is on) is in this scene, and whether it is drawn now (a model that
 // stands in for the shapes hides them, as S47LandWindmill does the windmill's).
 bool Drawn(US47Main* M, const FString& Place, bool& bOutShown)
 {
  if (!M || !M->Inter || !IsValid(M->Web.Get())) return false;
  FS47Interactable* It = M->Inter->Get(FS47Places::UseId(Place));
  if (!It || !It->Object || It->Object.W != M->Web.Get()) return false;
  const FS47WebNode* N = It->Object.W->Node(It->Object.I);
  const UPrimitiveComponent* C = N ? Cast<UPrimitiveComponent>(N->Comp.Get()) : nullptr;
  if (!C) return false;
  bOutShown = C->IsVisible() && !C->bHiddenInGame;
  return true;
 }
 TMap<FString, FContract>& Models() { static TMap<FString, FContract> Each; return Each; }
 // SARO's building, road-local (as FS47Places measures its bearings from: SARO's 9, 0)
 void SaroAt(double& X, double& Z) { X = 0; Z = 0; S47OpenLand::RoadOf(TEXT("saro"), 9, 0, X, Z); }
 // Where the player comes from to the land places: off the highway (its nearest point, road x 0) and from SARO.
 TArray<FVector2D> Approach(double Z)
 {
  double Sx = 0, Sz = 0; SaroAt(Sx, Sz);
  return { FVector2D(0, Z), FVector2D(Sx, Sz) };
 }
 bool SameObstacle(const FS47Obstacle& A, const FS47Obstacle& B)
 {
  if (A.bCircle != B.bCircle) return false;
  if (A.bCircle) return FMath::IsNearlyEqual(A.X, B.X, 1e-6) && FMath::IsNearlyEqual(A.Z, B.Z, 1e-6) && FMath::IsNearlyEqual(A.R, B.R, 1e-6);
  return FMath::IsNearlyEqual(A.MinX, B.MinX, 1e-6) && FMath::IsNearlyEqual(A.MaxX, B.MaxX, 1e-6) && FMath::IsNearlyEqual(A.MinZ, B.MinZ, 1e-6) && FMath::IsNearlyEqual(A.MaxZ, B.MaxZ, 1e-6);
 }
 FS47Obstacle Pushed(FS47Obstacle O, double By) { if (O.bCircle) O.X += By; else { O.MinX += By; O.MaxX += By; } return O; }
 FBox2D Moved(const FBox2D& B, const FVector2D& By) { return FBox2D(B.Min + By, B.Max + By); }
 // Whether the truck takes hold of Want now: not while the truck stands where it would go (a parked truck after the cairn's drift),
 // unless it is already held there.
 bool TakeHold(const FTruckHold& H, const FS47Obstacle& Want, const FVector2D* Truck, double Reach)
 {
  if (H.bHave && !IsAway(H.Written) && SameObstacle(H.Written, Want)) return true;
  return !Truck || TruckClear(Want, *Truck, Reach);
 }

 struct FState
 {
  TWeakObjectPtr<AS47Web> Web;
  double Wait = 0;
  TArray<FHeld> Held;
  TSet<FString> Waiting; // places whose truck obstacle waits for the truck to leave (logged once each)
 };
 FState& State() { static FState S; return S; }

 // The walk boxes of the road's and the diner's lists (the diner's area walks the road's)
 void WalkRows(AS47Web* W, const FString& Place, const FBox2D& R, bool bOn)
 {
  const FString Id = FString::Printf(TEXT("landplace:%s:0"), *Place);
  for (const TCHAR* List : { TEXT("road"), TEXT("diner") })
  {
   TArray<FS47Rect>& Rows = W->CollidersMutable(List);
   FS47Rect* Row = Rows.FindByPredicate([&Id](const FS47Rect& X) { return X.Id == Id; });
   if (!Row) { if (!bOn) continue; FS47Rect New; New.Id = Id; Row = &Rows[Rows.Add(New)]; }
   if (bOn) { Row->MinX = R.Min.X; Row->MaxX = R.Max.X; Row->MinZ = R.Min.Y; Row->MaxZ = R.Max.Y; }
   Row->bEnabled = bOn;
  }
 }
 // CairnRingParts' stones (S47Happenings), as they are laid: radius 2.6, each .22-.27 across, its middle Sz * .32 over the ground
 FVector RingStone(int32 I, double& OutSz)
 {
  const double A = I * 2. * UE_DOUBLE_PI / 8.;
  OutSz = .22 + .05 * FMath::Sin(I * 2.1);
  return FVector(FMath::Cos(A) * 2.6, OutSz * .32, FMath::Sin(A) * 2.6);
 }
}
using namespace Detail;

const FSolid* SolidOf(const FString& Place)
{
 for (const FSolid& S : SolidTable) if (Place == S.Place) return &S;
 return nullptr;
}

bool Basic(const FString& Place, FContract& Out)
{
 const FSolid* S = SolidOf(Place);
 const FS47Places::FDef* D = FS47Places::Def(Place);
 if (!S || !D) return false;
 FContract C; C.Shape = S->Shape; C.Yaw = 0;
 if (!FS47Places::PartOf(D->Kind, 0, C.Centre, C.Size)) return false;
 FVector WinAt, WinSize;
 if (S->WindowPart >= 0 && FS47Places::PartOf(D->Kind, S->WindowPart, WinAt, WinSize) && FaceOfPart(C.Centre, C.Size, WinAt, C.WindowNormal))
 {
  C.bWindow = true; C.Window = WinAt;
 }
 Out = C;
 return true;
}

void SetModel(const FString& Place, const FContract& C) { Models().Add(Place, C); }
void ClearModel(const FString& Place) { Models().Remove(Place); }

bool ContractOf(const FString& Place, bool bBasicShown, FContract& Out)
{
 if (!bBasicShown) if (const FContract* Model = Models().Find(Place)) { Out = *Model; return true; }
 return Basic(Place, Out);
}

FVector2D Turn(const FVector2D& V, double Yaw)
{
 const FVector R = S47Three::Euler(0, Yaw, 0).RotateVector(FVector(V.X, 0, V.Y));
 return FVector2D(R.X, R.Z);
}

FBox2D FootOf(const FContract& C)
{
 const FVector2D Mid(C.Centre.X, C.Centre.Z), Half(C.Size.X * .5, C.Size.Z * .5);
 return FBox2D(Mid - Half, Mid + Half);
}

FBox2D WalkRect(EShape Shape, const FBox2D& Footprint)
{
 if (Shape == EShape::Box) return Footprint;
 // the biggest square-cornered box inside the ellipse: half its axes over the root of two
 const FVector2D C = Footprint.GetCenter(), H = Footprint.GetExtent() / UE_DOUBLE_SQRT_2;
 return FBox2D(C - H, C + H);
}

FS47Obstacle TruckOf(EShape Shape, const FBox2D& Footprint)
{
 if (Shape == EShape::Box) return FS47Obstacle::Box(Footprint.Min.X, Footprint.Max.X, Footprint.Min.Y, Footprint.Max.Y);
 const FVector2D C = Footprint.GetCenter(), E = Footprint.GetExtent();
 return FS47Obstacle::Circle(C.X, C.Y, FMath::Min(E.X, E.Y));
}

FVector2D Toward(const FVector2D& From, TConstArrayView<FVector2D> Targets)
{
 FVector2D Sum = FVector2D::ZeroVector;
 for (const FVector2D& T : Targets) Sum += (T - From).GetSafeNormal();
 return Sum.GetSafeNormal();
}

FVector2D FaceOf(const FVector2D& Dir)
{
 if (Dir.IsNearlyZero()) return FVector2D(0, 1);
 return FMath::Abs(Dir.X) >= FMath::Abs(Dir.Y) ? FVector2D(Dir.X >= 0 ? 1 : -1, 0) : FVector2D(0, Dir.Y >= 0 ? 1 : -1);
}

bool FaceOfPart(const FVector& Centre, const FVector& Size, const FVector& PartAt, FVector2D& OutNormal)
{
 // on a face: its middle within a few centimetres of the face's plane (inside or out), and inside the face's other extent
 const FVector D = PartAt - Centre, E = Size * .5;
 const bool bX = FMath::Abs(FMath::Abs(D.X) - E.X) <= .05 && FMath::Abs(D.Z) < E.Z;
 const bool bZ = FMath::Abs(FMath::Abs(D.Z) - E.Z) <= .05 && FMath::Abs(D.X) < E.X;
 if (bX == bZ) return false;
 OutNormal = bX ? FVector2D(D.X >= 0 ? 1 : -1, 0) : FVector2D(0, D.Z >= 0 ? 1 : -1);
 return true;
}

FVector2D OutsidePoint(EShape Shape, const FBox2D& Footprint, const FVector2D& Dir, double Off)
{
 const FVector2D C = Footprint.GetCenter(), E = Footprint.GetExtent();
 if (Shape == EShape::Round)
 {
  const FVector2D D = Dir.IsNearlyZero() ? FVector2D(0, 1) : Dir.GetSafeNormal();
  return C + D * (FMath::Max(E.X, E.Y) + Off);
 }
 const FVector2D N = FaceOf(Dir);
 return C + FVector2D(N.X * (E.X + Off), N.Y * (E.Y + Off));
}

bool LampOn(const FContract& C, const FVector& Stand, FLampPose& Out)
{
 if (!C.bWindow) return false;
 const FVector2D N = FaceOf(C.WindowNormal);
 const FVector E = C.Size * .5;
 // the window's middle, brought out to the face's plane and the lamp's depth off it (own terms), then turned and set on the ground
 FVector L = C.Window;
 if (N.X != 0) L.X = C.Centre.X + N.X * (E.X + LampOut);
 else L.Z = C.Centre.Z + N.Y * (E.Z + LampOut);
 const FVector2D Flat = Turn(FVector2D(L.X, L.Z), C.Yaw), Out2 = Turn(N, C.Yaw);
 Out.At = Stand + FVector(Flat.X, L.Y, Flat.Y);
 // PoseProp turns the prop's +z by Euler(0, Yaw, 0): to (sin Yaw, 0, cos Yaw)
 Out.Yaw = FMath::Atan2(Out2.X, Out2.Y);
 return true;
}

FVector LightAt(const FContract& C, const FVector& Stand, const FVector2D& Dir, double Off, TFunctionRef<double(double, double)> GroundY)
{
 const FVector2D Own = OutsidePoint(C.Shape, FootOf(C), Turn(Dir, -C.Yaw), Off);
 const FVector2D P = FVector2D(Stand.X, Stand.Z) + Turn(Own, C.Yaw);
 return FVector(P.X, GroundY(P.X, P.Y), P.Y);
}

bool SegmentHits(const FVector& A, const FVector& B, const FBox& Box)
{
 // slabs: the part of A-B inside each pair of planes, all three at once
 double T0 = 0, T1 = 1;
 const FVector D = B - A;
 for (int32 K = 0; K < 3; ++K)
 {
  if (FMath::Abs(D[K]) < 1e-12) { if (A[K] < Box.Min[K] || A[K] > Box.Max[K]) return false; continue; }
  double Ta = (Box.Min[K] - A[K]) / D[K], Tb = (Box.Max[K] - A[K]) / D[K];
  if (Ta > Tb) Swap(Ta, Tb);
  T0 = FMath::Max(T0, Ta); T1 = FMath::Min(T1, Tb);
  if (T0 > T1) return false;
 }
 return true;
}

bool FollowAt(const FString& Id, double& X, double& Z, double TodayX, double TodayZ, double OffX, double OffZ)
{
 if (!FS47Places::Drifts(Id)) return false;
 const double Tx = TodayX + OffX, Tz = TodayZ + OffZ;
 if (FMath::Square(Tx - X) + FMath::Square(Tz - Z) < 1e-4) return false;
 X = Tx; Z = Tz;
 return true;
}

FVector2D StoneAt(const FVector2D& Centre, double Yaw, const FVector& Local)
{
 return Centre + Turn(FVector2D(Local.X, Local.Z), Yaw);
}

bool StoneSeat(const FVector2D& Centre, double Yaw, const FVector& Local, TFunctionRef<bool(double, double, FVector&)> GroundAt, FVector& Out)
{
 const FVector2D On = StoneAt(Centre, Yaw, Local);
 FVector Under;
 if (!GroundAt(On.X, On.Y, Under)) return false;
 Out = Under + FVector(0, Local.Y, 0);
 return true;
}

bool TruckClear(const FS47Obstacle& O, const FVector2D& Truck, double Reach)
{
 if (O.bCircle) return FVector2D::Distance(Truck, FVector2D(O.X, O.Z)) >= O.R + Reach;
 return FMath::Sqrt(FBox2D(FVector2D(O.MinX, O.MinZ), FVector2D(O.MaxX, O.MaxZ)).ComputeSquaredDistanceToPoint(Truck)) >= Reach;
}

bool IsAway(const FS47Obstacle& O) { return (O.bCircle ? O.X : O.MinX) > Away * .5; }

void HoldTruck(TArray<FS47Obstacle>& Obs, FTruckHold& H, const FS47Obstacle* Want)
{
 int32 Found = -1;
 if (H.bHave)
 {
  if (Obs.IsValidIndex(H.At) && SameObstacle(Obs[H.At], H.Written)) Found = H.At;
  else Found = Obs.IndexOfByPredicate([&H](const FS47Obstacle& O) { return SameObstacle(O, H.Written); });
  if (Found < 0) H.bHave = false; // (another list: a new world's legs)
 }
 if (Want)
 {
  if (Found >= 0) Obs[Found] = *Want; else Found = Obs.Add(*Want);
  H.At = Found; H.Written = *Want; H.bHave = true;
 }
 else if (Found >= 0)
 {
  const FS47Obstacle Gone = IsAway(H.Written) ? H.Written : Pushed(H.Written, Away);
  Obs[Found] = Gone; H.At = Found; H.Written = Gone;
 }
}

void LetGo(TArray<FHeld>& Held, TArray<FS47Obstacle>* Obs)
{
 for (FHeld& H : Held)
 {
  if (Obs) HoldTruck(*Obs, H.Truck, nullptr);
  H.bOn = false;
 }
}

bool PlaceToday(const US47Main* M, const FString& Id, double& OutX, double& OutZ)
{
 if (!M || !M->Freeform || !M->Freeform->Places) return false;
 FVector World; bool bSeated = false;
 if (!M->Freeform->Places->Stand(Id, World, bSeated)) return false; // (built only: Spot alone falls back on the plan)
 return M->Freeform->Places->Spot(Id, OutX, OutZ);
}

bool Follow(const US47Main* M, const FString& Id, double& X, double& Z, double OffX, double OffZ)
{
 if (!FS47Places::Drifts(Id)) return false;
 double Tx = 0, Tz = 0;
 if (!PlaceToday(M, Id, Tx, Tz)) return false;
 return FollowAt(Id, X, Z, Tx, Tz, OffX, OffZ);
}

bool LampPose(US47Main* M, const FString& HappeningId, FVector& InOutAt, double& OutYaw)
{
 if (HappeningId != TEXT("shack_lamp") || !M || !M->Freeform || !M->Freeform->Places) return false;
 static const FString Place = TEXT("shack");
 bool bShown = false; FVector Stand; bool bSeated = false; FContract C; FLampPose P;
 if (!Drawn(M, Place, bShown) || !M->Freeform->Places->Stand(Place, Stand, bSeated) || !ContractOf(Place, bShown, C) || !LampOn(C, Stand, P)) return false;
 // Stand is in the road's world, InOutAt in the area's: the lamp's offset from where the shack stands carries over.
 InOutAt += P.At - Stand;
 OutYaw = P.Yaw;
 return true;
}

bool LightPoint(US47Main* M, const FString& Place, FVector& OutAt)
{
 if (!SolidOf(Place) || !M || !M->Freeform || !M->Freeform->Places) return false;
 bool bShown = false; FVector Stand; bool bSeated = false; double X = 0, Z = 0; FContract C;
 if (!Drawn(M, Place, bShown) || !M->Freeform->Places->Stand(Place, Stand, bSeated) || !M->Freeform->Places->Spot(Place, X, Z) || !ContractOf(Place, bShown, C)) return false;
 const TArray<FVector2D> Ways = Approach(Z);
 const FVector Road = S47Legs::ORIGINS(TEXT("road"));
 const bool bFloor = bSeated && M->Player && M->Player->Floor && M->Web && (M->Web->Area() == TEXT("road") || M->Web->Area() == TEXT("diner"));
 // the ground at the point itself (as S47LandWindmill's Ground: the player's floor once measured there, else the land as planned)
 OutAt = LightAt(C, Stand, Toward(FVector2D(X, Z), Ways), Clear, [&](double Wx, double Wz)
 {
  if (bFloor) { const double Y = M->Player->Floor(Wx, Wz); if (FMath::IsFinite(Y)) return Y; }
  return Road.Y + S47RoadTerrain::SurfaceY(X + (Wx - Stand.X), Z + (Wz - Stand.Z));
 });
 return true;
}

void Update(US47Main* M, double Dt)
{
 FState& St = State();
 AS47Web* W = M ? M->Web.Get() : nullptr;
 IS47DriveArea* Leg = M && M->World ? M->World->LegArea(TEXT("road")) : nullptr;
 // A new scene has new walk lists, but the truck's lists are the world's legs and outlive it: what was held there is let go first
 // (S47LandWindmill's way), or the next hold would add a second copy and the old one would stay where the place stood.
 if (St.Web.Get() != W)
 {
  LetGo(St.Held, Leg ? &Leg->Obstacles : nullptr);
  St.Web = W; St.Wait = 0; St.Waiting.Reset();
 }
 if (!IsValid(W) || !W->IsLoaded()) return;
 St.Wait -= Dt; if (St.Wait > 0) return;
 St.Wait = .25;
 const bool bLive = M->IsFreeform() && M->Freeform && M->Freeform->Active() && M->Freeform->Places;
 const bool bRoad = W->Area() == TEXT("road") || W->Area() == TEXT("diner");
 const FS47Truck* Parked = bRoad && M->World ? M->World->TruckHere() : nullptr;
 const FVector2D TruckAt = Parked ? FVector2D(Parked->Position.X, Parked->Position.Z) : FVector2D::ZeroVector;
 const double Reach = Parked ? FVector2D(Parked->HalfLength(), Parked->HalfWidth()).Size() : 0.;
 for (const FSolid& S : SolidTable)
 {
  FHeld* H = St.Held.FindByPredicate([&S](const FHeld& X) { return X.Place == S.Place; });
  bool bShown = false; FVector Stand; bool bSeated = false; FContract C;
  const bool bHave = bLive && Drawn(M, S.Place, bShown) && M->Freeform->Places->Stand(S.Place, Stand, bSeated) && Basic(S.Place, C);
  const bool bOn = bHave && bShown;
  if (!H) { if (!bOn) continue; FHeld New; New.Place = S.Place; H = &St.Held[St.Held.Add(New)]; }
  if (!bOn && !H->bOn) continue;
  // (the basic shapes stand square: their footprint is the contract's own, moved to where the place stands)
  const FBox2D Foot = bOn ? Moved(FootOf(C), FVector2D(Stand.X, Stand.Z)) : FBox2D(FVector2D::ZeroVector, FVector2D::ZeroVector);
  const FBox2D R = WalkRect(S.Shape, Foot);
  WalkRows(W, H->Place, R, bOn);
  if (Leg)
  {
   const FS47Obstacle Want = TruckOf(S.Shape, Foot);
   if (!bOn) HoldTruck(Leg->Obstacles, H->Truck, nullptr);
   else if (TakeHold(H->Truck, Want, Parked ? &TruckAt : nullptr, Reach)) { HoldTruck(Leg->Obstacles, H->Truck, &Want); St.Waiting.Remove(H->Place); }
   else
   {
    // the truck stands where the place now is (left there before the cairn's drift): nothing at the old spot, and nothing over
    // the truck; taken again once it has gone
    HoldTruck(Leg->Obstacles, H->Truck, nullptr);
    if (!St.Waiting.Contains(H->Place)) { St.Waiting.Add(H->Place); UE_LOG(LogS47LandPlaceFixes, Display, TEXT("S47: land place %s: the truck stands where it is now; its obstacle waits until the truck has left"), *H->Place); }
   }
  }
  if (bOn && !H->bLogged)
  {
   H->bLogged = true;
   const FS47Obstacle T = TruckOf(S.Shape, Foot);
   if (T.bCircle) { UE_LOG(LogS47LandPlaceFixes, Display, TEXT("S47: land place %s: a walk box %.2f x %.2f m and a truck circle r %.2f m, inside what is drawn (%.2f x %.2f m)"), *H->Place, R.GetSize().X, R.GetSize().Y, T.R, Foot.GetSize().X, Foot.GetSize().Y); }
   else { UE_LOG(LogS47LandPlaceFixes, Display, TEXT("S47: land place %s: a walk box and a truck box %.2f x %.2f m, as drawn"), *H->Place, R.GetSize().X, R.GetSize().Y); }
  }
  H->bOn = bOn;
 }
}

bool SelfTest(TArray<FString>& Lines)
{
 bool bOk = true;
 const auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s land place fixes: %s"), bCond ? TEXT("PASS") : TEXT("FAIL"), *What)); bOk &= bCond; };
 const FS47Places::FDef* Shack = FS47Places::Def(TEXT("shack"));
 const FS47Places::FDef* Cairn = FS47Places::Def(TEXT("cairn"));
 FContract ShackC, CairnC;
 if (!Shack || !Cairn || !Basic(TEXT("shack"), ShackC) || !Basic(TEXT("cairn"), CairnC))
 {
  Check(false, TEXT("the shack and the cairn are among the places, with their basic shapes"));
  return bOk;
 }
 double Sx = 0, Sz = 0; SaroAt(Sx, Sz);
 const auto Az = [](const FVector2D& V) { return FMath::Fmod(FMath::RadiansToDegrees(FMath::Atan2(V.X, -V.Y)) + 360., 360.); };
 // what FS47Places draws, read from it (not written out here): the shack's box and its window, the cairn's base stone
 FVector BoxAt, BoxSize, WinAt, WinSize, StoneMid, StoneSize;
 const bool bParts = FS47Places::PartOf(Shack->Kind, 0, BoxAt, BoxSize) && FS47Places::PartOf(Shack->Kind, 3, WinAt, WinSize) && FS47Places::PartOf(Cairn->Kind, 0, StoneMid, StoneSize);
 Check(bParts && ShackC.Centre.Equals(BoxAt) && ShackC.Size.Equals(BoxSize) && CairnC.Centre.Equals(StoneMid) && CairnC.Size.Equals(StoneSize) && ShackC.Yaw == 0 && !CairnC.bWindow,
  TEXT("each place's contract is what FS47Places draws: the shack's box and the cairn's base stone, square to the land"));

 // 1. The lamp in the shack's one drawn window, on the outside of the face toward the highway and SARO.
 {
  const FVector2D At(Shack->X, Shack->Z);
  const FVector Stand(At.X, 0, At.Y);
  const FVector2D Dir = Toward(At, Approach(At.Y));
  FLampPose L;
  const bool bLamp = LampOn(ShackC, Stand, L);
  Check(bLamp && ShackC.bWindow && ShackC.WindowNormal.Equals(FaceOf(Dir)) && ShackC.WindowNormal.Equals(FVector2D(-1, 0)),
   FString::Printf(TEXT("the shack's drawn window is in the wall that faces the highway and SARO (west: the highway lies west, SARO at %.0f degrees, the way between at %.0f)"), Az(FVector2D(Sx, Sz) - At), Az(Dir)));
  // one window: no other thin part on a wall that faces away from the road (the old window was on the south wall, the lamp on the west)
  int32 Windows = 0; bool bOther = false;
  for (int32 I = 1; I < 12; ++I)
  {
   FVector PAt, PSize; FVector2D N;
   if (!FS47Places::PartOf(Shack->Kind, I, PAt, PSize)) break;
   if (PSize.Y < 1.5 && FaceOfPart(BoxAt, BoxSize, PAt, N)) { ++Windows; bOther |= !N.Equals(ShackC.WindowNormal); }
  }
  Check(Windows == 1 && !bOther, TEXT("the shack has one window part, and it is where the lamp burns (not on the south wall with the lamp on the west)"));
  const FBox Box(Stand + BoxAt - BoxSize * .5, Stand + BoxAt + BoxSize * .5);
  // the lamp's frame (S47Hap::WindowParts: 1.25 x 1.05, its back .05 behind the lamp's middle, its front .01 behind) covers the dark part
  const double Gap = Box.Min.X - (L.At.X + .05), Out = Box.Min.X - (Stand.X + WinAt.X - WinSize.X * .5);
  const bool bCovers = FMath::Abs(L.At.Y - WinAt.Y) < 1e-9 && FMath::Abs(L.At.Z - (Stand.Z + WinAt.Z)) < 1e-9 && WinSize.Z <= 1.25 && WinSize.Y <= 1.05 && Out <= LampOut - .01;
  Check(bLamp && !Box.IsInsideOrOn(L.At) && Gap >= 0 && Gap <= .01 && bCovers && FMath::IsNearlyEqual(L.Yaw, -UE_DOUBLE_PI / 2),
   FString::Printf(TEXT("the lamp lies outside the solid, flush with the wall (its frame's back %.3f m off it), on the window's middle %.1f m up, the dark window inside its frame, its front turned out"), Gap, WinAt.Y));
  const FVector Aim = L.At + FVector(0, .1, 0);
  const auto Sees = [&Aim, &Box](const FVector& Eye)
  {
   const FVector Line = Aim - Eye; const double Len = Line.Size();
   return !SegmentHits(Eye, Aim - Line / Len * FMath::Min(SightShort, Len * .05), Box) && !SegmentHits(Eye, Aim - Line / Len * .01, Box);
  };
  const FVector Highway(0, 1.62, At.Y), Cab(0, 2.0, At.Y), West(At.X - 300, 1.62, At.Y), Saro(Sx, 1.62, Sz), East(At.X + 300, 1.62, At.Y);
  Check(Sees(Highway) && Sees(Cab) && Sees(West) && Sees(Saro) && !Sees(East),
   TEXT("a line from the highway's nearest point (on foot and from the cab), from 300 m west and from SARO reaches the lamp past the shack; from 300 m east the shack is in the way"));
  // a model that stands in, turned a quarter and of another size: the lamp follows its own window, outside its own solid
  FContract Model = ShackC; Model.Yaw = UE_DOUBLE_PI / 2; Model.Size = FVector(6, 2.8, 4); Model.Centre = FVector(.5, 1.4, 0); Model.Window = FVector(-2.5, 1.5, .8);
  FLampPose Ml; const bool bModel = LampOn(Model, Stand, Ml);
  const FVector2D Back = Turn(FVector2D(Ml.At.X - Stand.X, Ml.At.Z - Stand.Z), -Model.Yaw), Front = Turn(FVector2D(FMath::Sin(Ml.Yaw), FMath::Cos(Ml.Yaw)), -Model.Yaw);
  Check(bModel && FMath::IsNearlyEqual(Back.X, -2.5 - LampOut, 1e-6) && FMath::IsNearlyEqual(Back.Y, .8, 1e-6) && Front.Equals(FVector2D(-1, 0), 1e-6) && FMath::IsNearlyEqual(Ml.At.Y, 1.5, 1e-9),
   TEXT("a turned model of another size: the lamp in that model's window, just out from its wall, its front out of it"));
  // which shapes are shown: a model's contract while the basic ones are hidden, the basic one while they are drawn
  const TMap<FString, FContract> Before = Models();
  SetModel(TEXT("shack"), Model);
  FContract Hidden, Shown;
  const bool bPick = ContractOf(TEXT("shack"), false, Hidden) && ContractOf(TEXT("shack"), true, Shown);
  Models() = Before;
  FContract After; ContractOf(TEXT("shack"), false, After);
  Check(bPick && Hidden.Size.Equals(Model.Size) && Hidden.Yaw == Model.Yaw && Shown.Size.Equals(BoxSize) && Shown.Yaw == 0 && After.Size.Equals(BoxSize),
   TEXT("a model's measures are used while the basic shapes are hidden, the basic shapes' while they are drawn (and with no model, theirs)"));
 }
 // 2. Lights and marks on the ground outside the solid, toward the way the player comes, at the ground's height there.
 {
  struct FOne { const TCHAR* Id; const FContract* C; double X, Z; };
  const FOne Each[] = { { TEXT("shack"), &ShackC, Shack->X, Shack->Z }, { TEXT("cairn"), &CairnC, Cairn->X, Cairn->Z } };
  // a 10 % slope, the place's middle at 3 m
  const auto Slope = [](double Wx, double Wz) { return 3. + .1 * Wx - .04 * Wz; };
  for (const FOne& One : Each)
  {
   const FVector2D At(One.X, One.Z);
   const FVector Stand(At.X, Slope(At.X, At.Y), At.Y);
   const FBox2D Foot = Moved(FootOf(*One.C), At);
   const FVector P = LightAt(*One.C, Stand, Toward(At, Approach(One.Z)), Clear, Slope);
   const FVector2D P2(P.X, P.Z);
   const FBox2D Held = WalkRect(One.C->Shape, Foot).ExpandBy(Walker);
   const double Off = FMath::Sqrt(Foot.ComputeSquaredDistanceToPoint(P2));
   Check(!Foot.IsInside(P2) && !Held.IsInside(P2) && Off >= 1. && FMath::Abs(P.Y - Slope(P.X, P.Z)) < 1e-9 && FMath::Abs(P.Y - Stand.Y) > .1,
    FString::Printf(TEXT("%s: lights and marks on the ground %.2f m outside it (at %.0f degrees from its middle), on the ground there (%.2f m from the middle's height on a 10 %% slope)"), One.Id, Off, Az(P2 - At), P.Y - Stand.Y));
  }
  FVector MidAt, MidSize;
  const bool bMid = FS47Places::PartOf(Cairn->Kind, 1, MidAt, MidSize);
  const FBox2D Base = FootOf(CairnC), Middle(FVector2D(MidAt.X - MidSize.X * .5, MidAt.Z - MidSize.Z * .5), FVector2D(MidAt.X + MidSize.X * .5, MidAt.Z + MidSize.Z * .5));
  Check(bMid && Base.IsInside(Middle.Min) && Base.IsInside(Middle.Max), TEXT("the cairn's base stone is its widest: the middle one stands inside it"));
 }
 // 3. Reyes's tag and the ring follow the cairn as it drifts (FollowAt, what Follow does with the place's today), on days 1 to 30.
 {
  double TagOffX = 0, TagOffZ = 0; bool bTag = false;
  for (const S47PredecessorText::FTagDef& T : S47PredecessorText::Tags()) if (FString(T.Place) == TEXT("cairn")) { TagOffX = T.OffX; TagOffZ = T.OffZ; bTag = true; }
  double Worst = 0, OldWorst = 0; bool bMoved = false, bOnce = true, bStill = true;
  for (const int32 Seed : { 1, 7, 1234, 90210 })
  {
   double TagX = Cairn->X + TagOffX, TagZ = Cairn->Z + TagOffZ, RingX = Cairn->X, RingZ = Cairn->Z, ShackX = Shack->X + 5, ShackZ = Shack->Z + 5;
   for (int32 Day = 1; Day <= 30; ++Day)
   {
    // where FS47Places has the cairn today: where it first stood plus its drift (FS47Places::Step adds the change each day)
    const FVector2D Today = FVector2D(Cairn->X, Cairn->Z) + FS47Places::DriftOf(TEXT("cairn"), Seed, Day);
    FollowAt(TEXT("cairn"), TagX, TagZ, Today.X, Today.Y, TagOffX, TagOffZ);
    bMoved |= FollowAt(TEXT("cairn"), RingX, RingZ, Today.X, Today.Y);
    bOnce &= !FollowAt(TEXT("cairn"), RingX, RingZ, Today.X, Today.Y);
    bStill &= !FollowAt(TEXT("shack"), ShackX, ShackZ, Today.X, Today.Y, 5, 5);
    const FVector2D WantTag = Today + FVector2D(TagOffX, TagOffZ);
    Worst = FMath::Max(Worst, FMath::Max(FVector2D::Distance(FVector2D(TagX, TagZ), WantTag), FVector2D::Distance(FVector2D(RingX, RingZ), Today)));
    OldWorst = FMath::Max(OldWorst, FVector2D::Distance(FVector2D(Cairn->X, Cairn->Z), Today));
   }
  }
  Check(bTag && bMoved && Worst <= .01 && OldWorst > 1, FString::Printf(TEXT("Reyes's tag (at its own offset) and the ring stay with the cairn every day (%.3f m at most; left where they were made they would be %.1f m off)"), Worst, OldWorst));
  Check(bOnce && bStill && FS47Places::Drifts(TEXT("cairn")) && !FS47Places::Drifts(TEXT("shack")) && !FS47Places::Drifts(TEXT("mine")),
   TEXT("a second look the same day moves nothing, and only the places that drift are followed (the shack and the mine never move)"));
 }
 // 4. Each ring stone on the ground under itself (StoneSeat, the cairn ring's own seating), on a sloped ground in another area's terms.
 {
  const double Shift = 137.5; // the area's x is the road's plus this (FS47Happenings::WorldOf in another area)
  const auto Ground = [](double X, double Z) { return 3. + .12 * X - .08 * Z + .05 * FMath::Sin(X * .7); };
  const auto GroundAt = [&Ground, Shift](double X, double Z, FVector& Out) { Out = FVector(X + Shift, Ground(X, Z), Z); return true; };
  const FVector2D Centre(Cairn->X, Cairn->Z); const double Yaw = 1.1;
  double Worst = 0, OldWorst = 0; bool bAll = true;
  for (int32 I = 0; I < 8; ++I)
  {
   double Sz = 0; const FVector Local = RingStone(I, Sz);
   FVector Stone; bAll &= StoneSeat(Centre, Yaw, Local, GroundAt, Stone);
   // measured off where the stone was put, not off what StoneSeat was asked: its foot on the ground under it, 2.6 m from the middle
   const double Under = Ground(Stone.X - Shift, Stone.Z), Foot = Stone.Y - Sz * .35;
   Worst = FMath::Max(Worst, FMath::Abs(Foot - (Under + Local.Y - Sz * .35)) + FMath::Abs(FVector2D::Distance(FVector2D(Stone.X - Shift, Stone.Z), Centre) - 2.6));
   OldWorst = FMath::Max(OldWorst, FMath::Abs(Ground(Centre.X, Centre.Y) - Under));
  }
  Check(bAll && Worst <= 1e-6 && OldWorst > .05, FString::Printf(TEXT("each ring stone sits on the ground under itself on a slope (%.6f m off at most; on the middle's height it was up to %.2f m off)"), Worst, OldWorst));
 }
 // 5. Walk boxes and the truck's obstacles from what FS47Places draws: no more than it; nothing that belongs at the place is inside them.
 {
  const FBox2D ShackFlat = FootOf(ShackC), CairnFlat = FootOf(CairnC);
  const FBox2D SW = WalkRect(ShackC.Shape, ShackFlat), CW = WalkRect(CairnC.Shape, CairnFlat);
  const FS47Obstacle ST = TruckOf(ShackC.Shape, ShackFlat), CT = TruckOf(CairnC.Shape, CairnFlat);
  // the drawn shapes, from PartOf's numbers: the box, and the ellipse of the base stone (x and z radii)
  const auto InBox = [&](const FVector2D& P) { return FMath::Abs(P.X - BoxAt.X) <= BoxSize.X * .5 + 1e-9 && FMath::Abs(P.Y - BoxAt.Z) <= BoxSize.Z * .5 + 1e-9; };
  const auto InStone = [&](const FVector2D& P) { return FMath::Square((P.X - StoneMid.X) / (StoneSize.X * .5)) + FMath::Square((P.Y - StoneMid.Z) / (StoneSize.Z * .5)) <= 1 + 1e-9; };
  bool bBox = true, bStone = true;
  for (int32 I = 0; I <= 10; ++I) for (int32 J = 0; J <= 10; ++J)
  {
   const FVector2D G(FMath::Lerp(-1., 1., I / 10.), FMath::Lerp(-1., 1., J / 10.));
   // the shack's: every point of the drawn box is held, every point a hand outside is not
   const FVector2D InB(BoxAt.X + G.X * BoxSize.X * .5, BoxAt.Z + G.Y * BoxSize.Z * .5), OutB(BoxAt.X + G.X * (BoxSize.X * .5 + .1), BoxAt.Z + G.Y * (BoxSize.Z * .5 + .1));
   bBox &= SW.IsInside(InB) || !InBox(InB) || (I % 10 == 0 || J % 10 == 0); // (the rim itself: FBox2D::IsInside is strict)
   if (I % 10 == 0 || J % 10 == 0) bBox &= !SW.IsInside(OutB) && !TruckClear(ST, InB, .001) && TruckClear(ST, OutB, .05);
   // the cairn's: every point of its walk box is inside the drawn stone
   const FVector2D InW = CW.GetCenter() + G * CW.GetExtent();
   bStone &= InStone(InW);
  }
  for (int32 K = 0; K < 16; ++K) { const double A = K * UE_DOUBLE_PI / 8.; bStone &= InStone(FVector2D(CT.X, CT.Z) + FVector2D(FMath::Cos(A), FMath::Sin(A)) * CT.R); }
  Check(bBox && !ST.bCircle, FString::Printf(TEXT("the shack: its walk box and its truck box are the drawn box (%.1f x %.1f m), no larger"), BoxSize.X, BoxSize.Z));
  Check(bStone && CT.bCircle && CT.R > .6 && StoneMid.Y + StoneSize.Y * .5 > .45,
   FString::Printf(TEXT("the cairn: a walk box %.2f x %.2f m and a truck circle r %.2f m inside the drawn stone (%.2f x %.2f m); higher than 0.45 m and wider than a roadside rock, so the low-rock rule leaves it"), CW.GetSize().X, CW.GetSize().Y, CT.R, StoneSize.X, StoneSize.Z));
  // what must stay out of them: Reyes's tags (and his test's stand 1.6 m south of each), F10's standing point 5 m south, the ring, the lamp
  double Nearest = 1e9;
  const auto Free = [&Nearest](const FBox2D& Box, const FVector2D& Pt, double Size) { const double D = FMath::Sqrt(Box.ExpandBy(Walker).ComputeSquaredDistanceToPoint(Pt)) - Size; Nearest = FMath::Min(Nearest, D); return D > 0; };
  bool bFree = true;
  for (const S47PredecessorText::FTagDef& T : S47PredecessorText::Tags())
  {
   const FString Where = T.Place;
   if (Where == TEXT("shack")) { bFree &= Free(SW, FVector2D(T.OffX, T.OffZ), .5) && Free(SW, FVector2D(T.OffX, T.OffZ + 1.6), 0); }
   if (Where == TEXT("cairn")) { bFree &= Free(CW, FVector2D(T.OffX, T.OffZ), .5) && Free(CW, FVector2D(T.OffX, T.OffZ + 1.6), 0); }
  }
  bFree &= Free(SW, FVector2D(0, 5), 0) && Free(CW, FVector2D(0, 5), 0);
  for (int32 I = 0; I < 8; ++I) { double Sz = 0; const FVector Local = RingStone(I, Sz); bFree &= Free(CW, FVector2D(Local.X, Local.Z), Sz * .6); }
  FLampPose L; LampOn(ShackC, FVector::ZeroVector, L);
  bFree &= !ShackFlat.IsInside(FVector2D(L.At.X, L.At.Z));
  Check(bFree, FString::Printf(TEXT("Reyes's tags, where his test stands to read them, F10's standing point and the ring's stones are clear of the walk boxes with a walker's width (the nearest %.2f m); the lamp is outside the wall"), Nearest));
  // the shack is used from 6 m (FS47Places::Build): a walker held a radius off its wall is well within it. The highway's fence lines are
  // at road x -21 and 21, the wall test drives east at z 720 to road x 150: both places are far off them.
  const double FromFences = FMath::Min(Shack->X - ShackFlat.GetExtent().X - 21., -Cairn->X - CairnFlat.GetExtent().X - 21.);
  Check(Walker + .05 < 6. && FromFences > 100., FString::Printf(TEXT("the shack's use (6 m) is reached from outside its walk box; both stand %.0f m or more off the highway's fence lines, away from the wall test's routes"), FromFences));
  Check(!SolidOf(TEXT("mine")) && SolidOf(TEXT("shack")) && SolidOf(TEXT("cairn")), TEXT("the mine has no solid shape: nothing is added there"));
  // ReconcileFarm's copy of the walk rows into the truck's list (US47World::CopiesToTruck) leaves our rows and the windmill's alone
  const auto Row = [](const TCHAR* Id, double X0, double X1) { FS47Rect R; R.Id = Id; R.MinX = X0; R.MaxX = X1; R.MinZ = 400; R.MaxZ = 404; return R; };
  FS47Rect Off = Row(TEXT("wall:shed"), 300, 304); Off.bEnabled = false;
  Check(!US47World::CopiesToTruck(Row(TEXT("landplace:shack:0"), 300, 304)) && !US47World::CopiesToTruck(Row(TEXT("landplace:cairn:0"), 300, 304)) && !US47World::CopiesToTruck(Row(TEXT("landplace:windmill:3"), 300, 304))
   && !US47World::CopiesToTruck(Row(TEXT("temp:x"), 300, 304)) && !US47World::CopiesToTruck(Off) && US47World::CopiesToTruck(Row(TEXT("wall:shed"), 300, 304)),
   TEXT("the farm's copy of walk rows into the truck's list skips every landplace: row (they put their own in), and still copies an ordinary wall"));
 }
 // 6. The truck's obstacles across a scene change and a drift: one live copy, where the place stands now; none over a parked truck.
 {
  const FS47Obstacle Other = FS47Obstacle::Box(10, 12, 10, 12);
  const FS47Obstacle Was = TruckOf(EShape::Round, Moved(FootOf(CairnC), FVector2D(Cairn->X, Cairn->Z)));
  const FS47Obstacle Now = TruckOf(EShape::Round, Moved(FootOf(CairnC), FVector2D(Cairn->X + 6, Cairn->Z - 3)));
  const auto Live = [](const TArray<FS47Obstacle>& List, const FS47Obstacle& O) { int32 N = 0; for (const FS47Obstacle& X : List) N += SameObstacle(X, O) ? 1 : 0; return N; };
  TArray<FS47Obstacle> Obs = { Other };
  TArray<FHeld> Held; Held.AddDefaulted(); Held[0].Place = TEXT("cairn");
  HoldTruck(Obs, Held[0].Truck, &Was); Held[0].bOn = true;
  LetGo(Held, &Obs); // the scene changes (a save loaded): the legs keep their lists
  HoldTruck(Obs, Held[0].Truck, &Now);
  // what was wrong: the state forgotten, the next hold adds a copy and the old one stays live where the cairn stood
  TArray<FS47Obstacle> Forgot = { Other }; FTruckHold First, Fresh;
  HoldTruck(Forgot, First, &Was); HoldTruck(Forgot, Fresh, &Now);
  Check(Obs.Num() == 2 && SameObstacle(Obs[0], Other) && Live(Obs, Was) == 0 && Live(Obs, Now) == 1 && !Held[0].bOn && Live(Forgot, Was) == 1,
   FString::Printf(TEXT("after a scene change the cairn's truck circle is let go and taken again in its slot: one live, where it stands now (%d obstacles; forgotten it would leave the old one live as well)"), Obs.Num()));
  HoldTruck(Obs, Held[0].Truck, nullptr);
  Check(Obs.Num() == 2 && Live(Obs, Now) == 0 && IsAway(Obs[1]) && SameObstacle(Obs[0], Other), TEXT("let go (Freeform ended), it is sent far off, nothing else in the list touched"));
  // a truck parked 3 m from the cairn's new middle before the drift: no hold until it has gone; 30 m off, or already held: hold
  FTruckHold Empty; const double Reach = FVector2D(2.75, 1.05).Size();
  const FVector2D Near(Now.X + 3, Now.Z), Far(Now.X + 30, Now.Z);
  Check(!TakeHold(Empty, Now, &Near, Reach) && TakeHold(Empty, Now, &Far, Reach) && TakeHold(Empty, Now, nullptr, Reach) && !TruckClear(Now, Near, Reach) && TruckClear(Was, Far + FVector2D(0, 30), Reach),
   TEXT("after the cairn's drift its circle is not put down on a truck parked there; it is taken once the truck has left"));
 }
 return bOk;
}
}

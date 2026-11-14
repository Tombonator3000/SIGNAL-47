#include "S47LandPlaceFixes.h"
#include "S47Drive.h"
#include "S47Freeform.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47PredecessorText.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/PrimitiveComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47LandPlaceFixes, Log, All);

// LAND-PLACES-FIXES-20261114: see the header.
namespace S47LandPlaceFixes
{
namespace Detail
{
 const FSolid SolidTable[] = { { TEXT("shack"), EShape::Box }, { TEXT("cairn"), EShape::Round } };
 // The truck's obstacles are never taken out of its list (others hold places in it by index): sent this far off when not wanted.
 constexpr double Away = 1e6;
 constexpr double Walker = .27; // AS47Walker's radius: walk boxes grow by it

 // The place's first part (FS47Places: the one its use is on): its box in three metres, and whether it is drawn now (a model that
 // stands in for the shapes hides them, as S47LandWindmill does the windmill's).
 bool DrawnBox(US47Main* M, const FString& Place, FBox& OutBox, bool& bOutShown)
 {
  if (!M || !M->Inter || !IsValid(M->Web.Get())) return false;
  FS47Interactable* It = M->Inter->Get(FS47Places::UseId(Place));
  if (!It || !It->Object || It->Object.W != M->Web.Get()) return false;
  const FS47WebNode* N = It->Object.W->Node(It->Object.I);
  const UPrimitiveComponent* C = N ? Cast<UPrimitiveComponent>(N->Comp.Get()) : nullptr;
  if (!C) return false;
  const FBox U = C->Bounds.GetBox();
  if (!U.IsValid) return false;
  OutBox = FBox(FVector(U.Min.X, U.Min.Z, U.Min.Y) / 100., FVector(U.Max.X, U.Max.Z, U.Max.Y) / 100.);
  bOutShown = C->IsVisible() && !C->bHiddenInGame;
  return true;
 }
 FBox2D Flat(const FBox& B) { return FBox2D(FVector2D(B.Min.X, B.Min.Z), FVector2D(B.Max.X, B.Max.Z)); }
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

 struct FHeld
 {
  FString Place;
  bool bOn = false, bTruck = false, bLogged = false;
  int32 TruckAt = -1;
  FS47Obstacle Written; // as it stands in the truck's list now (pushed away or not)
 };
 struct FState
 {
  TWeakObjectPtr<AS47Web> Web;
  double Wait = 0;
  TArray<FHeld> Held;
 };
 FState& State() { static FState S; return S; }

 void Hold(US47Main* M, AS47Web* W, FHeld& H, EShape Shape, const FBox2D& Footprint, bool bOn)
 {
  const FString Id = FString::Printf(TEXT("landplace:%s:0"), *H.Place);
  const FBox2D R = WalkRect(Shape, Footprint);
  for (const TCHAR* List : { TEXT("road"), TEXT("diner") })
  {
   TArray<FS47Rect>& Rows = W->CollidersMutable(List);
   FS47Rect* Row = Rows.FindByPredicate([&Id](const FS47Rect& X) { return X.Id == Id; });
   if (!Row) { if (!bOn) continue; FS47Rect New; New.Id = Id; Row = &Rows[Rows.Add(New)]; }
   if (bOn) { Row->MinX = R.Min.X; Row->MaxX = R.Max.X; Row->MinZ = R.Min.Y; Row->MaxZ = R.Max.Y; }
   Row->bEnabled = bOn;
  }
  IS47DriveArea* Leg = M->World ? M->World->LegArea(TEXT("road")) : nullptr;
  if (Leg)
  {
   TArray<FS47Obstacle>& Obs = Leg->Obstacles;
   int32 At = -1;
   if (H.bTruck)
   {
    if (Obs.IsValidIndex(H.TruckAt) && SameObstacle(Obs[H.TruckAt], H.Written)) At = H.TruckAt;
    else At = Obs.IndexOfByPredicate([&H](const FS47Obstacle& O) { return SameObstacle(O, H.Written); });
   }
   FS47Obstacle Want = bOn ? TruckOf(Shape, Footprint) : H.Written;
   if (!bOn && (Want.bCircle ? Want.X : Want.MinX) < Away * .5) Want = Pushed(Want, Away);
   if (At >= 0) { Obs[At] = Want; H.TruckAt = At; H.Written = Want; }
   else if (bOn) { H.TruckAt = Obs.Add(Want); H.Written = Want; H.bTruck = true; }
  }
  if (bOn && !H.bLogged)
  {
   H.bLogged = true;
   const FS47Obstacle T = TruckOf(Shape, Footprint);
   if (T.bCircle) { UE_LOG(LogS47LandPlaceFixes, Display, TEXT("S47: land place %s: a walk box %.2f x %.2f m and a truck circle r %.2f m, inside what is drawn (%.2f x %.2f m)"), *H.Place, R.GetSize().X, R.GetSize().Y, T.R, Footprint.GetSize().X, Footprint.GetSize().Y); }
   else { UE_LOG(LogS47LandPlaceFixes, Display, TEXT("S47: land place %s: a walk box and a truck box %.2f x %.2f m, as drawn"), *H.Place, R.GetSize().X, R.GetSize().Y); }
  }
  H.bOn = bOn;
 }
}
using namespace Detail;

const FSolid* SolidOf(const FString& Place)
{
 for (const FSolid& S : SolidTable) if (Place == S.Place) return &S;
 return nullptr;
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

FLampPose LampOn(const FBox& Solid, const FVector2D& Normal)
{
 const FVector2D N = FaceOf(Normal);
 const FVector C = Solid.GetCenter(), E = Solid.GetExtent();
 FLampPose P;
 P.At = FVector(C.X + N.X * (E.X + LampOut), Solid.Min.Y + LampUp, C.Z + N.Y * (E.Z + LampOut));
 // PoseProp turns the prop's +z by Euler(0, Yaw, 0): to (sin Yaw, 0, cos Yaw)
 P.Yaw = FMath::Atan2(N.X, N.Y);
 return P;
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

bool FollowTo(double& X, double& Z, double TodayX, double TodayZ, double OffX, double OffZ)
{
 const double Tx = TodayX + OffX, Tz = TodayZ + OffZ;
 if (FMath::Square(Tx - X) + FMath::Square(Tz - Z) < 1e-4) return false;
 X = Tx; Z = Tz;
 return true;
}

FVector2D StoneAt(const FVector2D& Centre, double Yaw, const FVector& Local)
{
 const FVector R = S47Three::Euler(0, Yaw, 0).RotateVector(Local);
 return Centre + FVector2D(R.X, R.Z);
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
 return FollowTo(X, Z, Tx, Tz, OffX, OffZ);
}

bool LampPose(US47Main* M, const FString& HappeningId, FVector& InOutAt, double& OutYaw)
{
 if (HappeningId != TEXT("shack_lamp") || !M || !M->Freeform || !M->Freeform->Places) return false;
 static const FString Place = TEXT("shack");
 FBox Box; bool bShown = false; FVector Stand; bool bSeated = false; double X = 0, Z = 0;
 if (!DrawnBox(M, Place, Box, bShown) || !M->Freeform->Places->Stand(Place, Stand, bSeated) || !M->Freeform->Places->Spot(Place, X, Z)) return false;
 const TArray<FVector2D> Ways = Approach(Z);
 const FLampPose P = LampOn(Box, FaceOf(Toward(FVector2D(X, Z), Ways)));
 // The box is in the road's world, InOutAt in the area's: the lamp's offset from where the shack stands carries over.
 InOutAt += P.At - Stand;
 OutYaw = P.Yaw;
 return true;
}

bool LightPoint(US47Main* M, const FString& Place, FVector& OutAt)
{
 const FSolid* S = SolidOf(Place);
 if (!S || !M || !M->Freeform || !M->Freeform->Places) return false;
 FBox Box; bool bShown = false; FVector Stand; bool bSeated = false; double X = 0, Z = 0;
 if (!DrawnBox(M, Place, Box, bShown) || !M->Freeform->Places->Stand(Place, Stand, bSeated) || !M->Freeform->Places->Spot(Place, X, Z)) return false;
 const TArray<FVector2D> Ways = Approach(Z);
 const FVector2D P = OutsidePoint(S->Shape, Flat(Box), Toward(FVector2D(X, Z), Ways), Clear);
 OutAt = FVector(P.X, Stand.Y, P.Y);
 return true;
}

void Update(US47Main* M, double Dt)
{
 FState& St = State();
 AS47Web* W = M ? M->Web.Get() : nullptr;
 // a new scene has new lists: nothing of ours is in them
 if (St.Web.Get() != W) { St = FState(); St.Web = W; }
 if (!IsValid(W) || !W->IsLoaded()) return;
 St.Wait -= Dt; if (St.Wait > 0) return;
 St.Wait = .25;
 const bool bLive = M->IsFreeform() && M->Freeform && M->Freeform->Active() && M->Freeform->Places;
 for (const FSolid& S : SolidTable)
 {
  FHeld* H = St.Held.FindByPredicate([&S](const FHeld& X) { return X.Place == S.Place; });
  FBox Box; bool bShown = false;
  const bool bHave = bLive && DrawnBox(M, S.Place, Box, bShown);
  const bool bOn = bHave && bShown;
  if (!H) { if (!bOn) continue; FHeld New; New.Place = S.Place; H = &St.Held[St.Held.Add(New)]; }
  if (!bOn && !H->bOn) continue;
  Hold(M, W, *H, S.Shape, bOn ? Flat(Box) : FBox2D(FVector2D::ZeroVector, FVector2D::ZeroVector), bOn);
 }
}

bool SelfTest(TArray<FString>& Lines)
{
 bool bOk = true;
 const auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s land place fixes: %s"), bCond ? TEXT("PASS") : TEXT("FAIL"), *What)); bOk &= bCond; };
 // Today's first parts as FS47Places::PartsOf draws them (place metres, x east, y up, z south): the shack's box and the cairn's base
 // stone, the widest of its three (the middle one, 1 x .9 at .2 .1, and the top one lie inside it).
 const FBox ShackBox(FVector(-2.25, 0, -1.6), FVector(2.25, 2.4, 1.6));
 const FBox CairnBase(FVector(-.75, -.05, -.65), FVector(.75, .65, .65));
 const FBox2D CairnMiddle(FVector2D(.2 - .5, .1 - .45), FVector2D(.2 + .5, .1 + .45));
 const FS47Places::FDef* Shack = FS47Places::Def(TEXT("shack"));
 const FS47Places::FDef* Cairn = FS47Places::Def(TEXT("cairn"));
 if (!Shack || !Cairn) { Check(false, TEXT("the shack and the cairn are among the places")); return bOk; }
 double Sx = 0, Sz = 0; SaroAt(Sx, Sz);
 const auto Az = [](const FVector2D& V) { return FMath::Fmod(FMath::RadiansToDegrees(FMath::Atan2(V.X, -V.Y)) + 360., 360.); };
 const auto Moved = [](const FBox2D& B, const FVector2D& By) { return FBox2D(B.Min + By, B.Max + By); };

 // 1. The lamp in the shack's window, on the outside of the face toward the highway and SARO.
 {
  const FVector2D At(Shack->X, Shack->Z);
  const TArray<FVector2D> Ways = Approach(At.Y);
  const FVector2D Dir = Toward(At, Ways), N = FaceOf(Dir);
  const FBox Box = ShackBox.ShiftBy(FVector(At.X, 0, At.Y));
  const FLampPose L = LampOn(Box, N);
  Check(N.Equals(FVector2D(-1, 0)), FString::Printf(TEXT("the shack's lamp is in its west wall: the highway lies west, SARO at %.0f degrees, the way between at %.0f"), Az(FVector2D(Sx, Sz) - At), Az(Dir)));
  // the frame's back is .05 m behind the lamp's middle (S47Hap::WindowParts): on the wall's outer face, a hair off it
  const double Gap = Box.Min.X - (L.At.X + .05);
  Check(!Box.IsInsideOrOn(L.At) && Gap >= 0 && Gap <= .01 && FMath::Abs(L.At.Y - LampUp) < 1e-9 && FMath::IsNearlyEqual(L.At.Z, At.Y) && FMath::IsNearlyEqual(L.Yaw, -UE_DOUBLE_PI / 2),
   FString::Printf(TEXT("the lamp lies outside the solid, flush with the wall (its frame's back %.3f m off the face), its middle %.1f m up, its front turned out"), Gap, LampUp));
  Check(Box.IsInsideOrOn(FVector(At.X, 1.4, At.Y)), TEXT("(what was wrong: the old lamp at the middle 1.4 m up was inside the box)"));
  const FVector Aim = L.At + FVector(0, .1, 0);
  const auto Sees = [&Aim, &Box](const FVector& Eye)
  {
   const FVector Line = Aim - Eye; const double Len = Line.Size();
   return !SegmentHits(Eye, Aim - Line / Len * FMath::Min(SightShort, Len * .05), Box) && !SegmentHits(Eye, Aim - Line / Len * .01, Box);
  };
  const FVector Highway(0, 1.62, At.Y), Cab(0, 2.0, At.Y), West(At.X - 300, 1.62, At.Y), Saro(Sx, 1.62, Sz), East(At.X + 300, 1.62, At.Y);
  Check(Sees(Highway) && Sees(Cab) && Sees(West) && Sees(Saro) && !Sees(East),
   TEXT("a line from the highway's nearest point (on foot and from the cab), from 300 m west and from SARO reaches the lamp past the shack; from 300 m east the shack is in the way"));
 }
 // 2. Lights and marks on the ground outside the solid, toward the way the player comes.
 {
  struct FOne { const TCHAR* Id; EShape Shape; FBox Local; double X, Z; };
  const FOne Each[] = { { TEXT("shack"), EShape::Box, ShackBox, Shack->X, Shack->Z }, { TEXT("cairn"), EShape::Round, CairnBase, Cairn->X, Cairn->Z } };
  for (const FOne& One : Each)
  {
   const FVector2D At(One.X, One.Z);
   const FBox2D Foot = Moved(Flat(One.Local), At);
   const TArray<FVector2D> Ways = Approach(One.Z);
   const FVector2D P = OutsidePoint(One.Shape, Foot, Toward(At, Ways), Clear);
   const FBox2D Held = WalkRect(One.Shape, Foot).ExpandBy(Walker);
   const double Out = FMath::Sqrt(Foot.ComputeSquaredDistanceToPoint(P));
   Check(!Foot.IsInside(P) && !Held.IsInside(P) && Out >= 1.,
    FString::Printf(TEXT("%s: lights and marks on the ground %.2f m outside it (at %.0f degrees from its middle), not in the middle"), One.Id, Out, Az(P - At)));
  }
  Check(Flat(CairnBase).IsInside(CairnMiddle.Min) && Flat(CairnBase).IsInside(CairnMiddle.Max), TEXT("the cairn's base stone is its widest: the others stand inside it"));
 }
 // 3. Reyes's tag and the ring follow the cairn as it drifts; they stay within 0.5 m of it on days 1, 10 and 30.
 {
  double TagOffX = 0, TagOffZ = 0; bool bTag = false;
  for (const S47PredecessorText::FTagDef& T : S47PredecessorText::Tags()) if (FString(T.Place) == TEXT("cairn")) { TagOffX = T.OffX; TagOffZ = T.OffZ; bTag = true; }
  double Worst = 0, OldWorst = 0; bool bMoved = false;
  for (const int32 Seed : { 1, 7, 1234, 90210 })
  {
   double TagX = Cairn->X + TagOffX, TagZ = Cairn->Z + TagOffZ, RingX = Cairn->X, RingZ = Cairn->Z;
   for (int32 Day = 1; Day <= 30; ++Day)
   {
    const FVector2D Today = FVector2D(Cairn->X, Cairn->Z) + FS47Places::DriftOf(TEXT("cairn"), Seed, Day);
    FollowTo(TagX, TagZ, Today.X, Today.Y, TagOffX, TagOffZ);
    bMoved |= FollowTo(RingX, RingZ, Today.X, Today.Y, 0, 0);
    if (Day == 1 || Day == 10 || Day == 30)
    {
     Worst = FMath::Max(Worst, FMath::Max(FVector2D::Distance(FVector2D(TagX - TagOffX, TagZ - TagOffZ), Today), FVector2D::Distance(FVector2D(RingX, RingZ), Today)));
     OldWorst = FMath::Max(OldWorst, FVector2D::Distance(FVector2D(Cairn->X, Cairn->Z), Today));
    }
   }
  }
  Check(bTag && bMoved && Worst <= .5, FString::Printf(TEXT("Reyes's tag and the ring stay with the cairn on days 1, 10 and 30 (%.2f m at most; left where they were made they would be %.1f m off)"), Worst, OldWorst));
  Check(FS47Places::Drifts(TEXT("cairn")) && !FS47Places::Drifts(TEXT("shack")) && !FS47Places::Drifts(TEXT("mine")), TEXT("only the places that drift are followed (the cairn does; the shack and the mine never move)"));
 }
 // 4. Each ring stone on the ground under itself, on a sloped test ground.
 {
  const auto Ground = [](double X, double Z) { return 3. + .12 * X - .08 * Z + .05 * FMath::Sin(X * .7); };
  const FVector2D C(Cairn->X, Cairn->Z); const double Yaw = 1.1;
  double Worst = 0, OldWorst = 0;
  for (int32 I = 0; I < 8; ++I)
  {
   // as S47Hap::CairnRingParts: radius 2.6, each .22-.27 across, its middle Sz * .32 over the ground, Sz * .7 high
   const double A = I * 2. * UE_DOUBLE_PI / 8., Sz = .22 + .05 * FMath::Sin(I * 2.1);
   const FVector Local(FMath::Cos(A) * 2.6, Sz * .32, FMath::Sin(A) * 2.6);
   const FVector2D P = StoneAt(C, Yaw, Local);
   const double Under = Ground(P.X, P.Y), Bottom = Ground(P.X, P.Y) + Local.Y - Sz * .35, OldBottom = Ground(C.X, C.Y) + Local.Y - Sz * .35;
   Worst = FMath::Max(Worst, FMath::Abs(Bottom - Under) + FMath::Abs(FVector2D::Distance(P, C) - 2.6));
   OldWorst = FMath::Max(OldWorst, FMath::Abs(OldBottom - Under));
  }
  Check(Worst <= .05 && OldWorst > .05, FString::Printf(TEXT("each ring stone sits on the ground under itself on a slope (%.3f m at most; on the middle's height it was %.2f m off)"), Worst, OldWorst));
 }
 // 5. Walk boxes and the truck's obstacles: what is drawn, no more; nothing that belongs at the place is inside them.
 {
  const FBox2D ShackFlat = Flat(ShackBox), CairnFlat = Flat(CairnBase);
  const FBox2D SW = WalkRect(EShape::Box, ShackFlat), CW = WalkRect(EShape::Round, CairnFlat);
  const FS47Obstacle ST = TruckOf(EShape::Box, ShackFlat), CT = TruckOf(EShape::Round, CairnFlat);
  const FVector2D E = CairnFlat.GetExtent();
  bool bInEllipse = true;
  for (const FVector2D& K : { CW.Min, CW.Max, FVector2D(CW.Min.X, CW.Max.Y), FVector2D(CW.Max.X, CW.Min.Y) }) bInEllipse &= FMath::Square(K.X / E.X) + FMath::Square(K.Y / E.Y) <= 1 + 1e-9;
  Check(SW.Min.Equals(ShackFlat.Min) && SW.Max.Equals(ShackFlat.Max) && !ST.bCircle && ST.MinX == ShackFlat.Min.X && ST.MaxX == ShackFlat.Max.X && ST.MinZ == ShackFlat.Min.Y && ST.MaxZ == ShackFlat.Max.Y,
   TEXT("the shack: its walk box and its truck box are the drawn box (4.5 x 3.2 m), no larger"));
  Check(bInEllipse && CT.bCircle && CT.R <= E.GetMin() + 1e-9 && CT.R > .6 && CairnBase.Max.Y > .45,
   FString::Printf(TEXT("the cairn: a walk box %.2f x %.2f m and a truck circle r %.2f m inside the drawn stone (%.2f x %.2f m); higher than 0.45 m and wider than a roadside rock, so the low-rock rule leaves it"), CW.GetSize().X, CW.GetSize().Y, CT.R, CairnFlat.GetSize().X, CairnFlat.GetSize().Y));
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
  for (int32 I = 0; I < 8; ++I) { const double A = I * 2. * UE_DOUBLE_PI / 8.; bFree &= Free(CW, FVector2D(FMath::Cos(A), FMath::Sin(A)) * 2.6, .17); }
  const FLampPose L = LampOn(ShackBox, FVector2D(-1, 0));
  bFree &= !ShackFlat.IsInside(FVector2D(L.At.X, L.At.Z));
  Check(bFree, FString::Printf(TEXT("Reyes's tags, where his test stands to read them, F10's standing point and the ring's stones are clear of the walk boxes with a walker's width (the nearest %.2f m); the lamp is outside the wall"), Nearest));
  // the shack is used from 6 m (FS47Places::Build): a walker held a radius off its wall is well within it. The highway's fence lines are
  // at road x -21 and 21, the wall test drives east at z 720 to road x 150: both places are far off them.
  const double FromFences = FMath::Min(Shack->X - ShackFlat.GetExtent().X - 21., -Cairn->X - CairnFlat.GetExtent().X - 21.);
  Check(Walker + .05 < 6. && FromFences > 100., FString::Printf(TEXT("the shack's use (6 m) is reached from outside its walk box; both stand %.0f m or more off the highway's fence lines, away from the wall test's routes"), FromFences));
  Check(!SolidOf(TEXT("mine")) && SolidOf(TEXT("shack")) && SolidOf(TEXT("cairn")), TEXT("the mine has no solid shape: nothing is added there"));
 }
 return bOk;
}
}

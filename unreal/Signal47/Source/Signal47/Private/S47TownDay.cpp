#include "S47TownDay.h"
#include "S47CalicheAccess.h"
#include "S47Caliche.h"
#include "S47DeputyRound.h"
#include "S47Freeform.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47Props.h"
#include "S47RoadTerrain.h"
#include "S47StoryDirector.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47TownUse.h"
#include "S47Townsfolk.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Weather.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Math/RandomStream.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47TownDay, Log, All);

namespace S47TownDayLocal
{
 using FSpot = S47TownDay::FSpot;
 // ---- where things are (three-space metres; Caliche's own numbers: Content/CalicheAccess/assembly.json for the substation, the
 // souvenir shop and the houses, Config/Props/mesadiner.json and Tools/mesadiner/diner_geometry.py for the diner (world =
 // (7979.95+bx, 4.309+bz, 1999.95-by)), Config/Props/town.json for the gas station (its blocks), FS47Town for the patrol car) ----
 constexpr double DinerFloor = 4.309006389843889;
 constexpr double SitDrop = .42;  // a standing figure lowered behind a table reads as sitting (the figures are single poses)
 constexpr double Speed = 1.25;   // metres a second of real time: an unhurried walk
 struct FNode { const TCHAR* Id; double X, Z; };
 const FNode Nodes[] = {
  // the substation: behind the desk, the middle of the room, inside and outside the door, the foot of the steps, by the car
  { TEXT("OffBack"), 7978.8, 2105.2 }, { TEXT("OffMid"), 7981.8, 2104.6 }, { TEXT("OffIn"), 7982.4, 2102.2 },
  { TEXT("OffDoor"), 7984.0, 2101.6 }, { TEXT("OffOut"), 7985.2, 2101.6 }, { TEXT("OffSteps"), 7986.9, 2101.6 },
  { TEXT("OffCar"), 7986.2, 2103.4 },
  // the west side of the street, clear of the gas station's pumps (its blocks end at x 7990.6) and the parked cars
  { TEXT("StOff"), 7992.5, 2101.6 }, { TEXT("StDiner"), 7992.5, 2003.5 },
  // the Mesa Diner: outside the door, the door (its opening is z 1999.35 to 2000.55), inside, the aisle, the second booth
  { TEXT("DinOut"), 7983.6, 1999.95 }, { TEXT("DinDoor"), 7981.85, 1999.95 }, { TEXT("DinIn"), 7980.4, 1999.95 },
  { TEXT("DinAisle"), 7978.3, 1999.95 }, { TEXT("DinBooth"), 7978.3, 1995.97 },
  // the diner's kitchen, behind the hatch (between the partition and the range), and its back: south of the range, behind the
  // partition's solid part (diner_geometry.py: the hatch is by 1.2 to 3.6, the range bx -7.7 to -6.4 over it), out of the room's sight
  { TEXT("Kitchen"), 7973.8, 1997.2 }, { TEXT("KitSouth"), 7973.8, 1995.6 }, { TEXT("KitBack"), 7972.95, 1993.45 },
  // the gas station: inside and outside its door (the opening z 2066.65 to 2067.75), the forecourt north of the pump island
  { TEXT("GasIn"), 7982.2, 2067.2 }, { TEXT("GasOut"), 7983.9, 2067.2 }, { TEXT("GasFore"), 7991.6, 2064.6 },
  // the souvenir shop, and the walk up the east side to the third house (clear of the church's and the shop's steps)
  { TEXT("SouvIn"), 8021.6, 2066.0 }, { TEXT("SouvDoor"), 8020.5, 2066.8 }, { TEXT("SouvOut"), 8019.4, 2066.8 },
  { TEXT("EastS"), 8016.5, 2068.0 }, { TEXT("EastN"), 8016.5, 2181.6 }, { TEXT("H03Out"), 8024.0, 2181.6 }, { TEXT("H03Door"), 8025.0, 2181.6 },
 };
 const TCHAR* const Edges[][2] = {
  { TEXT("OffBack"), TEXT("OffMid") }, { TEXT("OffMid"), TEXT("OffIn") }, { TEXT("OffIn"), TEXT("OffDoor") }, { TEXT("OffDoor"), TEXT("OffOut") },
  { TEXT("OffOut"), TEXT("OffSteps") }, { TEXT("OffSteps"), TEXT("OffCar") }, { TEXT("OffSteps"), TEXT("StOff") }, { TEXT("StOff"), TEXT("StDiner") },
  { TEXT("StDiner"), TEXT("DinOut") }, { TEXT("DinOut"), TEXT("DinDoor") }, { TEXT("DinDoor"), TEXT("DinIn") }, { TEXT("DinIn"), TEXT("DinAisle") },
  { TEXT("DinAisle"), TEXT("DinBooth") },
  { TEXT("Kitchen"), TEXT("KitSouth") }, { TEXT("KitSouth"), TEXT("KitBack") },
  { TEXT("GasIn"), TEXT("GasOut") }, { TEXT("GasOut"), TEXT("GasFore") },
  { TEXT("SouvIn"), TEXT("SouvDoor") }, { TEXT("SouvDoor"), TEXT("SouvOut") }, { TEXT("SouvOut"), TEXT("EastS") }, { TEXT("EastS"), TEXT("EastN") },
  { TEXT("EastN"), TEXT("H03Out") }, { TEXT("H03Out"), TEXT("H03Door") },
 };
 constexpr double PI_ = UE_DOUBLE_PI;
 const TArray<FSpot> AllSpots = {
  // the sheriff at the substation (desk: the west chair, facing the door; the board, coffee and files along the back)
  // (his chair is x 7979.37 to 7979.83 at the desk's west end, x 7979.725: 5 cm back from its middle, as one sits, 17 cm clear of
  // the desk's edge; sweep of 9 November: at 7979.62 he was in the desk)
  { TEXT("off.desk"), TEXT("office"), 7979.55, 2107.25, PI_ / 2, FSpot::Sit, TEXT("OffBack") },
  { TEXT("off.board"), TEXT("office"), 7978.35, 2105.3, -PI_ / 2, FSpot::Stand, TEXT("OffBack") },
  { TEXT("off.coffee"), TEXT("office"), 7978.25, 2109.9, -PI_ / 2, FSpot::Stand, TEXT("OffBack") },
  { TEXT("off.files"), TEXT("office"), 7977.3, 2102.7, -PI_ / 2, FSpot::Stand, TEXT("OffBack") },
  { TEXT("off.window"), TEXT("office"), 7982.7, 2105.3, PI_ / 2, FSpot::Stand, TEXT("OffMid") },
  { TEXT("off.step"), TEXT("office"), 7984.75, 2102.3, PI_ / 2, FSpot::Stand, TEXT("OffOut") },
  { TEXT("off.car"), TEXT("office"), 7986.2, 2105.0, PI_ / 2, FSpot::Stand, TEXT("OffCar") },
  // lunch at the Mesa: the second booth's seat nearest the door, or standing at the counter between the fourth and the fifth stool
  // (critics, round 8: lowered onto a stool the single pose's legs went through it and the floor; at the counter he stands)
  { TEXT("diner.booth"), TEXT("diner"), 7980.35, 1995.97, 0, FSpot::Sit, TEXT("DinBooth") },
  { TEXT("diner.counter"), TEXT("diner"), 7976.35, 1997.55, -PI_ / 2, FSpot::Stand, TEXT("DinAisle") },
  // home (the second house on the east side), the county's round
  { TEXT("home.sheriff"), TEXT("home"), 8028.25, 2156.5, 0, FSpot::Hidden, TEXT("OffCar") },
  { TEXT("sleep.sheriff"), TEXT("home"), 8028.25, 2156.5, 0, FSpot::Hidden, TEXT("OffCar") },
  { TEXT("patrol"), TEXT("patrol"), 7987.7, 2106.5, 0, FSpot::Away, TEXT("OffCar") },
  // the diner's cook: at the hatch, at the range, in the back; home is the house down the south road
  { TEXT("diner.kitchen"), TEXT("diner"), 7973.8, 1997.55, PI_ / 2, FSpot::Stand, TEXT("Kitchen") },
  { TEXT("diner.range"), TEXT("diner"), 7973.8, 1996.7, -PI_ / 2, FSpot::Stand, TEXT("Kitchen") },
  // (in and out by the kitchen's back, not from beside the range in sight of the room: critics, round 8)
  { TEXT("diner.back"), TEXT("diner"), 7972.95, 1993.45, 0, FSpot::Hidden, TEXT("KitBack") },
  { TEXT("home.cook"), TEXT("home"), 7976.5, 2241.0, 0, FSpot::Hidden, TEXT("KitBack") },
  { TEXT("sleep.cook"), TEXT("home"), 7976.5, 2241.0, 0, FSpot::Hidden, TEXT("KitBack") },
  // the attendant: his counter (where FS47TripoNpcs stands him), the pumps, a smoke by the door, the back room
  { TEXT("gas.counter"), TEXT("gas"), 0, 0, 0, FSpot::Anchor, TEXT("GasIn") },
  { TEXT("gas.pumps"), TEXT("gas"), 7991.3, 2057.5, -PI_ / 2, FSpot::Stand, TEXT("GasFore") },
  { TEXT("gas.smoke"), TEXT("gas"), 7984.4, 2069.6, PI_ / 2, FSpot::Stand, TEXT("GasOut") },
  { TEXT("gas.back"), TEXT("gas"), 7982.2, 2067.2, 0, FSpot::Hidden, TEXT("GasIn") },
  // the souvenir man: his stoop (the old smoker's place), his counter; home is the third house on the east side
  { TEXT("souv.stoop"), TEXT("souvenir"), 0, 0, 0, FSpot::Anchor, TEXT("SouvOut") },
  { TEXT("souv.counter"), TEXT("souvenir"), 8023.3, 2062.4, -PI_ / 2, FSpot::Stand, TEXT("SouvIn") },
  { TEXT("home.souvenir"), TEXT("home"), 8029.5, 2185.0, 0, FSpot::Hidden, TEXT("H03Door") },
  { TEXT("sleep.souvenir"), TEXT("home"), 8029.5, 2185.0, 0, FSpot::Hidden, TEXT("H03Door") },
  // CALICHE-HOMES (Tom, 9 November about 16:40: "Må kanskje et par hus til i Caliche, så folk har steder å bo?"). The ones who had
  // no home: each one's lamp goes by this day, their bodies are their own scenes' (the waitress at the counter, Earl in the studio,
  // the woman on her bench, the regular at the diner), so every spot of theirs is out of the day's drawing, and one way in each.
  // The houses: S47CalicheNewHomes (H05 to H07 are rooms in Content/CalicheAccess/assembly.json, H08 the trailer).
  { TEXT("diner.night"), TEXT("diner"), 7978.3, 1999.95, 0, FSpot::Away, TEXT("DinIn") },
  { TEXT("home.waitress"), TEXT("home"), 8028.25, 2113.0, 0, FSpot::Hidden, TEXT("DinIn") },
  { TEXT("sleep.waitress"), TEXT("home"), 8028.25, 2113.0, 0, FSpot::Hidden, TEXT("DinIn") },
  { TEXT("radio.air"), TEXT("radio"), 8022.8, 2037.55, 0, FSpot::Away, TEXT("StOff") },
  { TEXT("home.earl"), TEXT("home"), 8028.5, 2204.0, 0, FSpot::Hidden, TEXT("StOff") },
  { TEXT("sleep.earl"), TEXT("home"), 8028.5, 2204.0, 0, FSpot::Hidden, TEXT("StOff") },
  { TEXT("bench"), TEXT("bench"), 7986.0, 2130.5, 0, FSpot::Away, TEXT("StDiner") },
  { TEXT("home.bench_woman"), TEXT("home"), 7976.5, 2224.0, 0, FSpot::Hidden, TEXT("StDiner") },
  { TEXT("sleep.bench_woman"), TEXT("home"), 7976.5, 2224.0, 0, FSpot::Hidden, TEXT("StDiner") },
  { TEXT("diner.regular"), TEXT("diner"), 7976.35, 1999.95, 0, FSpot::Away, TEXT("DinOut") },
  { TEXT("home.regular"), TEXT("home"), 7969.0, 2254.5, 0, FSpot::Hidden, TEXT("DinOut") },
  { TEXT("sleep.regular"), TEXT("home"), 7969.0, 2254.5, 0, FSpot::Hidden, TEXT("DinOut") },
 };
 // Whose house is whose (S47CalicheAccess room ids).
 FString HouseOf(const FString& Who)
 {
  return Who == TEXT("deputy") ? TEXT("H02") : Who == TEXT("souvenir_man") ? TEXT("H03") : Who == TEXT("cook") ? TEXT("H04")
   : Who == TEXT("mesa_waitress") ? TEXT("H05") : Who == TEXT("earl") ? TEXT("H06") : Who == TEXT("bench_woman") ? TEXT("H07")
   : Who == TEXT("regular") ? TEXT("H08") : FString();
 }
 // FS47Townsfolk's name for one of the day's people, where it has them ("waitress" is the one at the counter): who can be gone.
 FString FolkOf(const FString& Who) { return Who == TEXT("mesa_waitress") ? FString(TEXT("waitress")) : Who; }
 // FS47TripoNpcs' body for the cook is its spare "waitress" figure; the day calls her "cook" (critics, round 8: under "waitress" she
 // went, and her lamp went out, whenever the waitress at the counter (FS47Townsfolk's "waitress") was gone).
 FString DayKey(const FString& Role) { return Role == TEXT("waitress") ? FString(TEXT("cook")) : Role; }
 int32 NodeIndex(const FString& Id) { for (int32 I = 0; I < UE_ARRAY_COUNT(Nodes); ++I) if (Id == Nodes[I].Id) return I; return -1; }
 FVector2D NodeAt(int32 I) { return FVector2D(Nodes[I].X, Nodes[I].Z); }
 // The shortest way along the known paths, as node indices from A to B (empty: none).
 TArray<int32> Route(int32 A, int32 B)
 {
  const int32 N = UE_ARRAY_COUNT(Nodes);
  TArray<double> D; D.Init(1e30, N); TArray<int32> From; From.Init(-1, N); TArray<bool> Done; Done.Init(false, N);
  if (A < 0 || B < 0) return {};
  D[A] = 0;
  for (int32 K = 0; K < N; ++K)
  {
   int32 U = -1; for (int32 I = 0; I < N; ++I) if (!Done[I] && (U < 0 || D[I] < D[U])) U = I;
   if (U < 0 || D[U] >= 1e29) break;
   Done[U] = true;
   for (const auto& E : Edges)
   {
    const int32 P = NodeIndex(E[0]), Q = NodeIndex(E[1]);
    const int32 V = P == U ? Q : Q == U ? P : -1;
    if (V < 0 || Done[V]) continue;
    const double W = D[U] + FVector2D::Distance(NodeAt(U), NodeAt(V));
    if (W < D[V]) { D[V] = W; From[V] = U; }
   }
  }
  if (D[B] >= 1e29) return {};
  TArray<int32> Out; for (int32 I = B; I >= 0; I = From[I]) Out.Insert(I, 0);
  return Out;
 }
 int32 Nearest(const FVector2D& P)
 {
  int32 Best = -1; double Near = 1e30;
  for (int32 I = 0; I < UE_ARRAY_COUNT(Nodes); ++I) { const double D = FVector2D::Distance(P, NodeAt(I)); if (D < Near) { Near = D; Best = I; } }
  return Best;
 }

 // ---- the plans ----
 struct FAct { const TCHAR* Spot; float Weight, Min, Max; };
 // Fill [From, To) with short stays among Acts, never the same one twice running.
 void Fill(TArray<S47TownDay::FRow>& Rows, double From, double To, FRandomStream& R, std::initializer_list<FAct> Acts)
 {
  double T = From; FString Last;
  float Total = 0; for (const FAct& A : Acts) Total += A.Weight;
  while (T < To - .02)
  {
   const FAct* Pick = nullptr;
   for (int32 Tries = 0; Tries < 6 && !Pick; ++Tries)
   {
    float X = R.FRand() * Total;
    for (const FAct& A : Acts) { X -= A.Weight; if (X <= 0) { Pick = &A; break; } }
    if (!Pick) Pick = &*(Acts.end() - 1);
    if (Last == Pick->Spot) Pick = nullptr;
   }
   if (!Pick) Pick = &*Acts.begin();
   double End = FMath::Min(To, T + Pick->Min + R.FRand() * (Pick->Max - Pick->Min));
   if (To - End < .05) End = To;
   Rows.Add({ T, End, Pick->Spot }); Last = Pick->Spot; T = End;
  }
 }
 void Add(TArray<S47TownDay::FRow>& Rows, double From, double To, const TCHAR* Spot) { if (To > From) Rows.Add({ From, To, Spot }); }
 // Over whatever rows say for [From, To): Spot.
 void Overlay(TArray<S47TownDay::FRow>& Rows, double From, double To, const TCHAR* Spot)
 {
  From = FMath::Max(0., From); To = FMath::Min(24., To);
  if (To <= From) return;
  TArray<S47TownDay::FRow> Out;
  for (const auto& R : Rows)
  {
   if (R.To <= From || R.From >= To) { Out.Add(R); continue; }
   if (R.From < From) Out.Add({ R.From, From, R.Spot });
   if (R.To > To) Out.Add({ To, R.To, R.Spot });
  }
  Out.Add({ From, To, Spot });
  Out.Sort([](const S47TownDay::FRow& A, const S47TownDay::FRow& B) { return A.From < B.From; });
  Rows = MoveTemp(Out);
 }
 uint32 Key(int32 Seed, int32 Day, const FString& Who) { return FCrc::StrCrc32(*Who) ^ (uint32(Seed) * 2654435761u) ^ (uint32(Day + 7) * 40503u) ^ 0x54444159u; }
 // REVIEW (14 Nov): the cook pours through the hatch only once there: at the range or the hatch (a diner spot, not out the back),
 // arrived and not on the way (Counter), and not away
 bool CookPours(const FString& Target, const FString& Arrived, bool bOnTheWay, bool bAway)
 {
  return !bAway && S47TownDay::PlaceOf(Target) == TEXT("diner") && Target != TEXT("diner.back") && Arrived == Target && !bOnTheWay;
 }

 // ---- the running game ----
 struct FWalker
 {
  FString Target, Arrived;          // the spot it is going to, and the one it last reached
  TArray<FVector2D> Path;           // what is left of the way there
  FVector2D At = FVector2D::ZeroVector;
  double Yaw = 0, Age = 0, Phase = 0;
  double Sit = 0;     // 0 standing to 1 seated: lowered and raised over a few tenths of a second, never in one frame
  double Stride = 0;  // metres walked: the steps' rise and fall and the sway go by it, not by the clock
  double Held = 0;    // real seconds a change in sight of the player has been held back (into or out of the car)
  bool bPlaced = false, bHidden = true, bSeated = false;
  FVector Anchor = FVector::ZeroVector; double AnchorYaw = 0; bool bAnchor = false;
 };
 TWeakObjectPtr<US47Main> Main;
 bool bActive = false;
 TMap<FString, FWalker> Walkers;
 TMap<FString, TArray<S47TownDay::FRow>> Plans;
 int32 PlanDay = -1, PlanSeed = 0; double PlanAge = 0, LastClock = -1, Hour = 0;
 TWeakObjectPtr<UStaticMeshComponent> LunchBox; bool bLunchBuilt = false, bLunchOn = false;
 TMap<FString, float> Lamp;
 bool bHeldCar = false;
 // NPC-ANIM: day keys whose body walks on its own clips: whether its sit clip sits, and how far it is lowered then (metres).
 struct FRig { bool bSeatInClip = false; double SitLower = 0; };
 TMap<FString, FRig> Rigged;
 // NPC-DAY-FOLLOW-20261109: whether each body drawn by others is shown now (S47TownDay::Present), and where it is drawn (for Seen).
 TMap<FString, bool> BodyShown;
 struct FBody { const TCHAR* Who; double X, Z; };
 const FBody Bodies[] = { { TEXT("mesa_waitress"), 7974.7, 1999.95 }, { TEXT("earl"), 8022.8, 2037.55 }, { TEXT("bench_woman"), 7986.0, 2130.5 } };

 const FSpot* SpotOf(const FString& Id) { return S47TownDay::Spot(Id); }
 bool Visible(const FSpot* S) { return S && S->Kind != FSpot::Hidden && S->Kind != FSpot::Away; }
 FVector2D Goal(const FWalker& W, const FSpot* S) { return S->Kind == FSpot::Anchor ? FVector2D(W.Anchor.X, W.Anchor.Z) : FVector2D(S->X, S->Z); }
 double GroundAt(US47Main* M, double X, double Z)
 {
  const double Floor = S47CalicheAccess::FloorAt(X, Z);
  if (!FMath::IsNaN(Floor)) return Floor;
  if (X > 7972.0 && X < 7981.8 && Z > 1991.1 && Z < 2008.8) return DinerFloor;
  // The props' own floors (the gas station's shop and its ramp: Config/Props/town.json), as the player's walk has them first
  // (critics, round 8: the attendant walked sunk into the shop's floor between the counter and the door).
  const double Prop = AS47Props::FloorAt(X, Z);
  if (FMath::IsFinite(Prop)) return Prop;
  FVector O = FVector(8000, 0, 0);
  if (M && M->Web) if (const FS47Obj G = M->Web->Obj(TEXT("world.town.group"))) O = G.WorldPosition();
  return O.Y + S47RoadTerrain::SurfaceY(X - O.X, Z - O.Z);
 }
 void Lead(FWalker& W, const FSpot* To, bool bSnap)
 {
  const FSpot* From = SpotOf(W.Arrived);
  const bool bWasWalking = W.Path.Num() > 0;
  W.Path.Reset();
  if (bSnap || !W.bPlaced)
  {
   W.bPlaced = true; W.bHidden = !Visible(To); W.Arrived = W.Target;
   const int32 V = NodeIndex(To->Via);
   W.At = Visible(To) ? Goal(W, To) : V >= 0 ? NodeAt(V) : FVector2D(To->X, To->Z);
   W.Yaw = To->Kind == FSpot::Anchor ? W.AnchorYaw : To->Yaw; W.bSeated = To->Kind == FSpot::Sit; W.Sit = W.bSeated ? 1 : 0;
   return;
  }
  // Unseen to unseen (home to bed, home to the night's round): nothing to walk.
  if (W.bHidden && !Visible(To)) { W.Arrived = W.Target; W.bSeated = false; W.Sit = 0; return; }
  // From a house or a back room: out of its door (where it went in) first.
  int32 Start = -1;
  if (W.bHidden && From) { Start = NodeIndex(From->Via); if (Start >= 0) W.At = NodeAt(Start); W.bHidden = false; }
  else if (From && !bWasWalking) Start = NodeIndex(From->Via);
  if (Start < 0) Start = Nearest(W.At);
  const TArray<int32> Way = Route(Start, NodeIndex(To->Via));
  if (Way.IsEmpty() && Start != NodeIndex(To->Via)) { Lead(W, To, true); return; }
  // Leave a chair or a stool by its own approach, then the way, then the spot itself.
  for (int32 I : Way) W.Path.Add(NodeAt(I));
  if (Visible(To)) W.Path.Add(Goal(W, To));
  W.bSeated = false;
 }
 constexpr double HoldMax = 120; // real seconds: past this a change goes ahead even in sight (a player who never looks away)
 // bHoldOut: the player can see where the walker would go out of sight (the sheriff at his car): stay there, in sight, until not.
 void Walk(FWalker& W, double Dt, bool bHoldOut = false)
 {
  W.Age += Dt;
  const FSpot* To = SpotOf(W.Target);
  // Down into a seat or up out of it over about a third of a second; up first, then away.
  W.Sit = FMath::FInterpConstantTo(W.Sit, W.bSeated ? 1. : 0., Dt, 3.);
  double Left = W.Sit > .001 && !W.bSeated ? 0. : Speed * Dt;
  while (Left > 0 && W.Path.Num())
  {
   const FVector2D D = W.Path[0] - W.At; const double L = D.Size();
   // Turned into the way, not snapped to it: a quarter turn in about a third of a second.
   if (L > .01) { const double Want = FMath::Atan2(D.X, D.Y); W.Yaw += FMath::Clamp(FMath::FindDeltaAngleRadians(W.Yaw, Want), -5. * Dt, 5. * Dt); }
   if (L <= Left)
   {
    W.At = W.Path[0]; W.Path.RemoveAt(0); Left -= L; W.Stride += L;
    // Through a shut door: it opens (the substation's, the shop's), as a hand on it would; nobody shuts it after them.
    for (const TCHAR* Door : { TEXT("OffDoor"), TEXT("SouvDoor") })
     if (FVector2D::Distance(W.At, NodeAt(NodeIndex(Door))) < .01)
     {
      const TCHAR* Room = FCString::Strcmp(Door, TEXT("OffDoor")) == 0 ? TEXT("Sheriff") : TEXT("Souvenir");
      if (S47CalicheAccess::HasDoor(Room) && !S47CalicheAccess::IsOpen(Room)) { S47CalicheAccess::SetOpenQuietly(Room, true); S47CalicheAccess::PoseDoor(Room, -1); }
     }
   }
   else { W.At += D / L * Left; W.Stride += Left; Left = 0; }
  }
  if (W.Path.IsEmpty() && To && W.Arrived != W.Target)
  {
   // Critics, round 8: into the car and gone (or into a house) in front of the player: he waits by it until he is not watched.
   if (bHoldOut && !Visible(To) && W.Held < HoldMax) { W.Held += Dt; return; }
   W.Held = 0;
   W.Arrived = W.Target; W.bHidden = !Visible(To); W.bSeated = To->Kind == FSpot::Sit;
  }
  // Settle facing the spot's way once there.
  if (W.Path.IsEmpty() && To && Visible(To))
  {
   const double Want = To->Kind == FSpot::Anchor ? W.AnchorYaw : To->Yaw;
   const double Diff = FMath::FindDeltaAngleRadians(W.Yaw, Want);
   W.Yaw += FMath::Clamp(Diff, -2.5 * Dt, 2.5 * Dt);
   if (To->Kind == FSpot::Anchor) W.At = FVector2D(W.Anchor.X, W.Anchor.Z);
  }
 }
 bool SheriffSeatedAt(const TCHAR* SpotId)
 {
  const FWalker* W = Walkers.Find(TEXT("deputy"));
  return bActive && W && W->Arrived == SpotId && W->Target == SpotId && W->Path.IsEmpty() && !W->bHidden && !FS47Townsfolk::AwayNow(TEXT("deputy"));
 }
 // Lunch with him in the booth (critics, round 8: it opened his office talk, "He opens the door", "two files on this desk"): a
 // word at the table, today's one thing he has to say (FS47Townsfolk), and the office for the rest.
 void OpenLunch(US47Main* M)
 {
  if (!M || !M->UI) return;
  TArray<S47Kit::FW> Rows;
  Rows.Add(S47Kit::Paper(TEXT("MESA DINER, THE SECOND BOOTH. He doesn't get up. He nods at the seat across from him and goes on with the special.")));
  const FString Said = M->Freeform && M->Freeform->Townsfolk ? M->Freeform->Townsfolk->Gossip(TEXT("deputy")) : FString();
  if (!Said.IsEmpty()) Rows.Add(S47Kit::Text(Said));
  Rows.Add(S47Kit::Text(TEXT("THE DEPUTY: I'm on my lunch. Anything for a file, bring it by the office this afternoon.")));
  const TWeakObjectPtr<US47Main> Weak(M);
  Rows.Add(S47Kit::Button(TEXT("Let him eat"), [Weak] { if (US47Main* Run = Weak.Get()) if (Run->UI) Run->UI->Close(); }));
  M->UI->Open(S47Kit::Shell(M->UI, TEXT("MESA DINER / THE DEPUTY"), S47Kit::Col(Rows), 820));
 }
 // A box round the booth's seat that answers the use trace while the sheriff sits there: talk to him at lunch.
 void BuildLunch(US47Main* M)
 {
  bLunchBuilt = true;
  AS47Web* W = M->Web.Get(); if (!W || !M->Inter) return;
  const int32 Parent = W->Handle(TEXT("world.diner.group"));
  UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
  const FSpot* S = SpotOf(TEXT("diner.booth"));
  if (Parent < 0 || !Cube || !S) return;
  auto* C = NewObject<UStaticMeshComponent>(W, TEXT("TownDayLunchBox"));
  C->SetupAttachment(W->GetRootComponent()); C->SetMobility(EComponentMobility::Movable); C->SetStaticMesh(Cube);
  C->SetHiddenInGame(true); C->SetCastShadow(false);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCollisionResponseToAllChannels(ECR_Ignore);
  C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
  C->RegisterComponent();
  C->SetWorldLocation(S47Three::ToUE(S->X, DinerFloor + .7, S->Z)); C->SetWorldScale3D(FVector(.6, .6, 1.3));
  const int32 Node = W->AdoptNode(C, Parent, TEXT("townday:sheriffLunch"));
  if (Node < 0) { C->DestroyComponent(); return; }
  LunchBox = C;
  FS47Interactable I; I.Id = TEXT("townday:sheriffLunch"); I.Object = W->ObjAt(Node); I.Range = 2.4;
  // (the deputy, as every talk and paper has him; his hat on the hook, since a hat on the seat is the empty patrol car's)
  I.Label = [] { return SheriffSeatedAt(TEXT("diner.booth")) ? FString(TEXT("Talk to the deputy")) : FString(); };
  I.Use = []
  {
   US47Main* Run = Main.Get();
   if (Run && SheriffSeatedAt(TEXT("diner.booth"))) OpenLunch(Run);
  };
  I.Examine = [] { return SheriffSeatedAt(TEXT("diner.booth")) ? FString(TEXT("The deputy, in the second booth, eating the special without looking at it. His hat is on the hook by the door.")) : FString(); };
  M->Inter->Add(I); W->MarkUsable(Node, I.Id);
 }
}

const TArray<FString>& S47TownDay::People()
{
 static const TArray<FString> P = { TEXT("deputy"), TEXT("cook"), TEXT("attendant"), TEXT("souvenir_man"),
  TEXT("mesa_waitress"), TEXT("earl"), TEXT("bench_woman"), TEXT("regular") }; // (CALICHE-HOMES: the last four)
 return P;
}
// NPC-DAY-FOLLOW-20261109 (N1110). Hours to places, and one habit each. The places are the plan's (home, diner, radio, bench, gas)
// or, for the two with no plan, "county road" (Dale) and "baca" (Eloy Baca, the Baca place under the east rim).
const TArray<S47TownDay::FHabit>& S47TownDay::Routine()
{
 static const TArray<FHabit> T = {
  // Dale: never seen (DECISIONS: phone and walkie only). He has no place in the world to be met at, only his voice.
  { TEXT("dale"), 0, 24, TEXT("phone"), TEXT("Never seen him. A voice on the phone and the walkie, from wherever field maintenance is.") },
  // Earl, 1420 AM: the Night Line, midnight to four (the studio's card, S47Timeline::EarlOnDuty); the evening man has the air before.
  { TEXT("earl"), 0, 4, TEXT("radio"), TEXT("The 1420 window is lit and the mast light blinks over the roof. He is on the air.") },
  { TEXT("earl"), 4, 5, TEXT("home"), TEXT("The station goes dark and his car goes down the road home.") },
  { TEXT("earl"), 5, 11, TEXT("home"), TEXT("His car is in the yard of the sixth house, under a blanket of dust. The curtains are shut.") },
  { TEXT("earl"), 11, 24, TEXT("home"), TEXT("His lamp is lit and the radio inside is going: he listens to the evening man and waits for midnight.") },
  // The bench woman: her radio and her blanket (at home instead in a shower or blowing sand: BenchWeather).
  { TEXT("bench_woman"), 13, 18, TEXT("home"), TEXT("Her lamp is lit, and she is hanging wash on the line behind the house.") },
  { TEXT("bench_woman"), 18, 20, TEXT("bench"), TEXT("She sits down on the laundromat bench at six with her radio and her blanket, the same way every evening.") },
  { TEXT("bench_woman"), 20, 24, TEXT("bench"), TEXT("She turns the radio down when somebody walks by, and up again when they have passed.") },
  { TEXT("bench_woman"), 0, 4, TEXT("bench"), TEXT("She is still on the bench, blanket to her chin, the radio turned low.") },
  // Eloy Baca: under the east rim, and in town at noon (EloyAt: every hour from six on has one place).
  { TEXT("eloy"), 6, 8, TEXT("baca"), TEXT("Coffee on the porch at first light, with his hat on the rail.") },
  { TEXT("eloy"), 8, 11.75, TEXT("baca"), TEXT("He is on the porch, looking at the sky as if it owed him something.") },
  { TEXT("eloy"), 11.75, 13.25, TEXT("feed"), TEXT("At noon his truck is gone from the shed: he is at the Feed and Seed counter in town, arguing the price of range cubes.") },
  { TEXT("eloy"), 13.25, 18, TEXT("baca"), TEXT("The truck is in the shed with the hood up, and he is somewhere under it.") },
  { TEXT("eloy"), 18, 19, TEXT("baca"), TEXT("He feeds the dog at the windmill every evening at six, whatever the weather.") },
  { TEXT("eloy"), 19, 22, TEXT("baca"), TEXT("The kitchen lamp is lit at the Baca place, and it goes out at ten.") },
  { TEXT("eloy"), 22, 24, TEXT("baca"), TEXT("The house is dark. He does not come to the door after ten.") },
  // The Mesa waitress, nights, and the day cook.
  { TEXT("mesa_waitress"), 13, 18, TEXT("home"), TEXT("Her lamp is lit in the afternoon, and her shoes are on the step.") },
  { TEXT("mesa_waitress"), 19, 23, TEXT("diner"), TEXT("She wipes the same stretch of counter whenever nobody is talking to her.") },
  { TEXT("mesa_waitress"), 0, 5, TEXT("diner"), TEXT("She tops up the coffee before you ask, and watches the road.") },
  { TEXT("cook"), 6, 14, TEXT("diner"), TEXT("The kitchen light is on at six and the smell of grease is out the door by half past.") },
  { TEXT("cook"), 15, 15.3, TEXT("diner"), TEXT("The cook smokes behind the diner at three, a hand on the wall, for about as long as one cigarette takes.") },
 };
 return T;
}
FString S47TownDay::HabitAt(const FString& Who, double Hour, FString* OutPlace)
{
 for (const FHabit& H : Routine()) if (Who == H.Who && Hour >= H.From && Hour < H.To) { if (OutPlace) *OutPlace = H.Place; return H.Seen; }
 return FString();
}
bool S47TownDay::BodyAt(const FString& Who, const FString& SpotId)
{
 return (Who == TEXT("mesa_waitress") && SpotId == TEXT("diner.night")) || (Who == TEXT("earl") && SpotId == TEXT("radio.air")) || (Who == TEXT("bench_woman") && SpotId == TEXT("bench"));
}
bool S47TownDay::BenchWeather(int32 Seed, int32 Day, double Hour)
{
 // TownDay's calendar day Day is the weather's Day + 1 (FS47WeatherLive: floor(clock / a day) + 1); its small hours are that day's.
 const S47Weather::FNow W = S47Weather::Now(Seed, Day + 1, FMath::Clamp(Hour, 0., 23.999) * 3600.);
 return W.Rain > .15 || W.Dust > .3;
}
FString S47TownDay::EloyAt(double Hour)
{
 FString Place;
 HabitAt(TEXT("eloy"), FMath::Fmod(FMath::Fmod(Hour, 24.) + 24., 24.), &Place);
 return Place.IsEmpty() ? FString(TEXT("baca")) : Place; // (before six he is at home, asleep)
}
bool S47TownDay::Present(const FString& Who, bool& bOut)
{
 using namespace S47TownDayLocal;
 const bool* B = bActive ? BodyShown.Find(Who) : nullptr;
 if (!B) return false;
 bOut = *B;
 return true;
}
FString S47TownDay::Counter()
{
 using namespace S47TownDayLocal;
 bool bWaitress = true;
 if (!bActive || !Present(TEXT("mesa_waitress"), bWaitress)) return TEXT("waitress");
 if (bWaitress) return TEXT("waitress");
 const FWalker* Cook = Walkers.Find(TEXT("cook"));
 // The cook at the range or the hatch pours through it; out the back on her break, or not in yet, nobody does.
 return Cook && CookPours(Cook->Target, Cook->Arrived, !Cook->Path.IsEmpty(), FS47Townsfolk::AwayNow(TEXT("cook"))) ? TEXT("cook") : TEXT("");
}
const TArray<S47TownDay::FSpot>& S47TownDay::Spots() { return S47TownDayLocal::AllSpots; }
FString S47TownDay::HomeOf(const FString& Who) { return S47TownDayLocal::HouseOf(S47TownDayLocal::DayKey(Who)); }
const S47TownDay::FSpot* S47TownDay::Spot(const FString& Id)
{
 for (const FSpot& S : S47TownDayLocal::AllSpots) if (Id == S.Id) return &S;
 return nullptr;
}
FString S47TownDay::PlaceOf(const FString& SpotId) { const FSpot* S = Spot(SpotId); return S ? FString(S->Place) : FString(); }
bool S47TownDay::HomeAwake(const FString& SpotId) { return SpotId.StartsWith(TEXT("home.")); }
FString S47TownDay::SpotIn(const TArray<FRow>& Rows, double H)
{
 for (const FRow& R : Rows) if (H >= R.From && H < R.To) return R.Spot;
 return Rows.Num() ? Rows.Last().Spot : FString();
}

TArray<S47TownDay::FRow> S47TownDay::Plan(int32 Seed, int32 Day, int32 WeekDay, const FString& Who, const FNight& Prev, const FNight& Tonight)
{
 using namespace S47TownDayLocal;
 FRandomStream R(int32(Key(Seed, Day, Who)));
 TArray<FRow> Rows;
 const auto J = [&R](double Lo, double Hi) { return Lo + R.FRand() * (Hi - Lo); };
 if (Who == TEXT("deputy"))
 {
  // A weekday 08 to 17 at the substation; Saturday a half day; Sunday at home.
  const bool bSunday = WeekDay == 6, bSaturday = WeekDay == 5;
  const double Wake = J(6.4, 6.9), Sleep = J(22.25, 23.0);
  if (bSunday)
  {
   Add(Rows, 0, J(7.6, 8.4), TEXT("sleep.sheriff")); Add(Rows, Rows.Last().To, Sleep, TEXT("home.sheriff")); Add(Rows, Sleep, 24, TEXT("sleep.sheriff"));
  }
  else
  {
   const double Arrive = bSaturday ? J(8.8, 9.1) : J(7.72, 7.97), Leave = bSaturday ? J(12.9, 13.2) : J(17.0, 17.25);
   Add(Rows, 0, Wake, TEXT("sleep.sheriff"));
   Add(Rows, Wake, Arrive - .03, TEXT("home.sheriff"));
   Add(Rows, Arrive - .03, Arrive + .03, TEXT("off.car")); // pulls up, gets out
   const auto Office = [&](double From, double To)
   {
    // (a game hour is two and a half real minutes by day: the shortest stay here is about twenty seconds of the player's time)
    Fill(Rows, From, To, R, { { TEXT("off.desk"), 5.f, .4f, .9f }, { TEXT("off.board"), 1.2f, .15f, .3f }, { TEXT("off.coffee"), 1.f, .12f, .25f },
     { TEXT("off.files"), 1.f, .12f, .3f }, { TEXT("off.window"), .8f, .1f, .2f }, { TEXT("off.step"), .7f, .12f, .25f }, { TEXT("off.car"), .35f, .08f, .15f } });
   };
   if (bSaturday) Office(Arrive + .03, Leave - .04);
   else
   {
    // Lunch at the Mesa about twelve; on some afternoons a patrol in the car (the car away with him).
    const double L0 = J(11.85, 12.1), L1 = L0 + J(.7, .9);
    const bool bPatrol = R.FRand() < .5f;
    const double P0 = bPatrol ? J(14.4, 15.4) : Leave, P1 = bPatrol ? FMath::Min(P0 + J(.55, .9), Leave - .3) : Leave;
    Office(Arrive + .03, L0);
    Add(Rows, L0, L1, R.FRand() < .65f ? TEXT("diner.booth") : TEXT("diner.counter"));
    if (bPatrol && P1 > P0 + .2)
    {
     Office(L1, P0 - .03); Add(Rows, P0 - .03, P0, TEXT("off.car")); Add(Rows, P0, P1, TEXT("patrol")); Add(Rows, P1, P1 + .03, TEXT("off.car"));
     Office(P1 + .03, Leave - .04);
    }
    else Office(L1, Leave - .04);
   }
   Add(Rows, Leave - .04, Leave, TEXT("off.car"));
   Add(Rows, Leave, Sleep, TEXT("home.sheriff"));
   Add(Rows, Sleep, 24, TEXT("sleep.sheriff"));
  }
  // The county's round (FS47DeputyRound): out in the car round the hour it is due, the night it comes.
  if (Prev.bRound) Overlay(Rows, Prev.Due - 24 - .5, Prev.Due - 24 + .6, TEXT("patrol"));
  if (Tonight.bRound) Overlay(Rows, Tonight.Due - .5, Tonight.Due + .6, TEXT("patrol"));
 }
 else if (Who == TEXT("cook"))
 {
  // The diner's cook: 06 to 21 every day, a break about 15:00 (N1110: out the back for a cigarette, see Routine).
  const double Arrive = J(5.7, 5.95), Leave = J(20.7, 20.95), B0 = J(14.8, 15.1), Sleep = J(22.3, 23.0);
  Add(Rows, 0, Arrive - .6, TEXT("sleep.cook")); Add(Rows, Arrive - .6, Arrive, TEXT("home.cook"));
  const auto Kitchen = [&](double From, double To)
  { Fill(Rows, From, To, R, { { TEXT("diner.range"), 3.f, .2f, .6f }, { TEXT("diner.kitchen"), 2.f, .1f, .35f } }); };
  Kitchen(Arrive, B0); Add(Rows, B0, B0 + .5, TEXT("diner.back")); Kitchen(B0 + .5, Leave);
  Add(Rows, Leave, Sleep, TEXT("home.cook")); Add(Rows, Sleep, 24, TEXT("sleep.cook"));
 }
 else if (Who == TEXT("attendant"))
 {
  // The town's night man (the night window): always at the station. By day the pumps and the back room now and then.
  Fill(Rows, 0, 7, R, { { TEXT("gas.counter"), 8.f, .5f, 1.2f }, { TEXT("gas.smoke"), 1.f, .08f, .15f } });
  Fill(Rows, 7, 20, R, { { TEXT("gas.counter"), 6.f, .3f, 1.f }, { TEXT("gas.pumps"), 1.2f, .1f, .2f }, { TEXT("gas.smoke"), 1.f, .08f, .15f }, { TEXT("gas.back"), .5f, .1f, .25f } });
  Fill(Rows, 20, 24, R, { { TEXT("gas.counter"), 8.f, .5f, 1.2f }, { TEXT("gas.smoke"), 1.f, .08f, .15f } });
 }
 else if (Who == TEXT("souvenir_man"))
 {
  // The shop's hours (FS47TownUse: 10 to 17), lunch on the stoop, home up the east side.
  const double Arrive = J(9.55, 9.85), Leave = J(17.05, 17.3), Sleep = J(22.5, 23.3), Lunch = J(12.4, 12.7);
  Add(Rows, 0, 7.2, TEXT("sleep.souvenir")); Add(Rows, 7.2, Arrive, TEXT("home.souvenir"));
  const auto Shop = [&](double From, double To)
  { Fill(Rows, From, To, R, { { TEXT("souv.counter"), 4.f, .3f, .8f }, { TEXT("souv.stoop"), 2.f, .12f, .35f } }); };
  Shop(Arrive, Lunch); Add(Rows, Lunch, Lunch + .45, TEXT("souv.stoop")); Shop(Lunch + .45, Leave);
  Add(Rows, Leave, Sleep, TEXT("home.souvenir")); Add(Rows, Sleep, 24, TEXT("sleep.souvenir"));
 }
 // CALICHE-HOMES: the night people sleep into the afternoon and are home, lamp lit, until they go out at dusk.
 else if (Who == TEXT("mesa_waitress"))
 {
  // The Mesa's night counter (FS47Townsfolk has her at the diner in the night's hours): from about 18:50 until after 05.
  const double Off = J(5.1, 5.35), Bed = J(5.9, 6.3), Up = J(13.2, 13.8), On = J(18.75, 18.95);
  Add(Rows, 0, Off, TEXT("diner.night")); Add(Rows, Off, Bed, TEXT("home.waitress")); Add(Rows, Bed, Up, TEXT("sleep.waitress"));
  Add(Rows, Up, On, TEXT("home.waitress")); Add(Rows, On, 24, TEXT("diner.night"));
 }
 else if (Who == TEXT("earl"))
 {
  // 1420 AM's Night Line: on the air from midnight until four (S47Timeline::EarlOnDuty and the studio's card, "MIDNIGHT TO FOUR";
  // the evening man has the air before), home to sleep it off. NPC-DAY-FOLLOW-20261109: no longer "on the air" from seven, which
  // the card, the studio window and his body in the chair (FS47TripoNpcs, by midnight) all said otherwise of.
  const double Off = J(4.0, 4.15), Bed = J(4.6, 5.0), Up = J(11.3, 11.9);
  Add(Rows, 0, Off, TEXT("radio.air")); Add(Rows, Off, Bed, TEXT("home.earl")); Add(Rows, Bed, Up, TEXT("sleep.earl"));
  Add(Rows, Up, 24, TEXT("home.earl"));
 }
 else if (Who == TEXT("bench_woman"))
 {
  // The woman with the radio sits out on the laundromat's bench from six until ten to five (FS47Town: NightPerson), as she always has.
  constexpr double Off = 4. + 50. / 60., On = 18.;
  const double Bed = J(5.3, 5.7), Up = J(12.8, 13.4);
  Add(Rows, 0, Off, TEXT("bench")); Add(Rows, Off, Bed, TEXT("home.bench_woman")); Add(Rows, Bed, Up, TEXT("sleep.bench_woman"));
  Add(Rows, Up, On, TEXT("home.bench_woman")); Add(Rows, On, 24, TEXT("bench"));
  // NPC-DAY-FOLLOW-20261109: in a shower or blowing sand the bench is empty and she is at home, lamp lit, until it passes.
  for (double H = 0; H < 24; H += .25)
   if (BodyAt(Who, SpotIn(Rows, H + .125)) && BenchWeather(Seed, Day, H + .125)) Overlay(Rows, H, H + .25, TEXT("home.bench_woman"));
 }
 else if (Who == TEXT("regular"))
 {
  // The regular ("comes in when the lights are up"): home all day, the Mesa's counter in the evening, home to bed before midnight.
  const double Up = J(8.3, 8.9), In = J(19.9, 20.2), Out = J(23.0, 23.4), Bed = J(23.6, 23.9);
  Add(Rows, 0, Up, TEXT("sleep.regular")); Add(Rows, Up, In, TEXT("home.regular")); Add(Rows, In, Out, TEXT("diner.regular"));
  Add(Rows, Out, Bed, TEXT("home.regular")); Add(Rows, Bed, 24, TEXT("sleep.regular"));
 }
 return Rows;
}

// ---------------------------------------------------------------- the running game

bool S47TownDay::Active() { return S47TownDayLocal::bActive; }

void S47TownDay::Stop()
{
 using namespace S47TownDayLocal;
 US47Main* M = Main.Get();
 if (bActive && M && M->TownLayer && M->TownLayer->Town)
 {
  FS47Town& Town = *M->TownLayer->Town;
  Town.SetCarHeld(false);
  // The man on the stoop as FS47Townsfolk::ShowPosts has him in Freeform without the day (-S47TownDayOff): there unless gone.
  // NPC-DAY-FOLLOW-20261109: and the woman on her bench as Freeform had her without the day (always there).
  if (M->IsFreeform())
  {
   const bool bSmoker = Town.bSmokerIn || !FS47Townsfolk::AwayNow(TEXT("souvenir_man"));
   if (!Town.bWomanIn || bSmoker != Town.bSmokerIn) Town.SetPeople(true, bSmoker);
  }
 }
 for (const auto& Pair : Lamp) S47CalicheAccess::SetRoomLamp(Pair.Key, 1);
 if (auto* C = LunchBox.Get()) C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 bActive = false; bLunchOn = false; Walkers.Reset(); Plans.Reset(); Lamp.Reset(); BodyShown.Reset(); PlanDay = -1; LastClock = -1;
}

void S47TownDay::Step(US47Main* M, double Dt)
{
 using namespace S47TownDayLocal;
 if (FParse::Param(FCommandLine::Get(), TEXT("S47TownDayOff")) || !M || !M->IsFreeform() || !M->Freeform || !M->Freeform->Active() || !M->Web || !M->Web->IsLoaded())
 { if (bActive) Stop(); return; }
 if (Main.Get() != M) { Walkers.Reset(); bLunchBuilt = false; LunchBox.Reset(); }
 Main = M; bActive = true;
 FS47FreeformSession& F = *M->Freeform;
 const double Clock = F.Clock();
 const int32 CalDay = FMath::FloorToInt(Clock / 86400.);
 Hour = FMath::Fmod(Clock, 86400.) / 3600.;
 // The plans: again on a new day, a new game, or every few seconds (the letter that brings the round can come in the day).
 PlanAge -= Dt;
 if (CalDay != PlanDay || F.Seed() != PlanSeed || PlanAge <= 0)
 {
  PlanDay = CalDay; PlanSeed = F.Seed(); PlanAge = 5;
  const int32 WeekDay = int32(F.DateOfDay(CalDay + 1).GetDayOfWeek());
  // The night that ends this morning is the Freeform day CalDay's (days turn at 08:00); tonight's is CalDay+1's.
  const auto Night = [&F](int32 N) { FNight R; R.bRound = N >= 1 && F.DeputyOnRoute() && FS47DeputyRound::Comes(F.Seed(), N, F.SheriffTrust()); R.Due = FS47DeputyRound::DueHour(F.Seed(), N); return R; };
  for (const FString& Who : People()) Plans.Add(Who, Plan(F.Seed(), CalDay, WeekDay, Who, Night(CalDay), Night(CalDay + 1)));
 }
 // Out of sight they are where the plan says: another area, far off, or the clock jumped (a wait, a sleep).
 const bool bJump = LastClock < 0 || FMath::Abs(Clock - LastClock) > 600;
 LastClock = Clock;
 // Critics, round 8: in the car on the main street the area is "road" (FS47FreeformDirector: one street either way), not "diner";
 // every change there snapped him from the office to the booth in sight. Where the eye is: the camera's, on foot or driving.
 const FString Area = M->Web->Area();
 const bool bTown = Area == TEXT("diner") || Area == TEXT("road");
 FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
 FVector2D Fwd = FVector2D::ZeroVector;
 if (M->Player) { P = S47Three::ToThree(M->Player->EyeUE()); const FVector Look = M->Player->ForwardUE(); Fwd = FVector2D(Look.X, Look.Y).GetSafeNormal(); }
 // Whether the player could be watching a spot in town: near enough and roughly that way (or right by it).
 const auto Seen = [&](const FVector2D& At)
 {
  if (!bTown) return false;
  const FVector2D D = At - FVector2D(P.X, P.Z); const double L = D.Size();
  return L < 8 || (L < 120 && FVector2D::DotProduct(D / L, Fwd) > .3);
 };
 const FVector2D CarAt = NodeAt(NodeIndex(TEXT("OffCar")));
 const auto ByCar = [](const FSpot* S) { return S && FCString::Strcmp(S->Via, TEXT("OffCar")) == 0; };
 for (const FString& Who : People())
 {
  FWalker& W = Walkers.FindOrAdd(Who);
  const FString Want = SpotIn(Plans.FindRef(Who), Hour);
  const FSpot* To = SpotOf(Want);
  if (!To) continue;
  if (To->Kind == FSpot::Anchor && !W.bAnchor) continue; // not stood anywhere yet: next frame
  if (Want != W.Target || bJump)
  {
   // Out of the car from home or a patrol: not with the player looking at the car (he and it would come out of nothing).
   const FSpot* From = SpotOf(W.Arrived);
   if (!bJump && W.bPlaced && W.bHidden && Visible(To) && ByCar(From) && Seen(CarAt) && W.Held < HoldMax) { W.Held += Dt; Walk(W, Dt); continue; }
   W.Held = 0;
   const FVector2D G = Visible(To) ? Goal(W, To) : FVector2D(To->X, To->Z);
   const bool bFar = FVector2D::Distance(FVector2D(P.X, P.Z), W.At) > 90 && FVector2D::Distance(FVector2D(P.X, P.Z), G) > 90;
   W.Target = Want;
   Lead(W, To, bJump || !bTown || bFar);
  }
  // Into the car and away: not with the player looking (he waits by it).
  Walk(W, Dt, ByCar(To) && Seen(CarAt));
 }
 // The sheriff's place is the town's: his window lit and his door answered while he is in; the car outside while he is in town.
 if (M->TownLayer && M->TownLayer->Town)
 {
  FS47Town& Town = *M->TownLayer->Town;
  const FWalker* S = Walkers.Find(TEXT("deputy"));
  const bool bGone = FS47Townsfolk::AwayNow(TEXT("deputy"));
  const FString Place = S ? PlaceOf(S->Target) : FString();
  const bool bIn = !bGone && Place == TEXT("office");
  // (and while he is still about on foot: walking to it, or waiting by it until he is not watched)
  const bool bCar = !bGone && S && (Place == TEXT("office") || Place == TEXT("diner") || (!S->bHidden && (S->Path.Num() || S->Arrived != S->Target)));
  if (Town.bDeputyIn != bIn) Town.SetDeputy(bIn);
  if (bHeldCar != (bCar && !bIn)) { bHeldCar = bCar && !bIn; Town.SetCarHeld(bHeldCar); }
  // Critics, round 8: "Man on the stoop" answered at 03:00 and while he stood at his counter. The stoop's label, look and talk
  // (FS47Caliche: Town.bSmokerIn) are there only while the day has him sitting on it.
  const FWalker* Sm = Walkers.Find(TEXT("souvenir_man"));
  const bool bStoop = Sm && !FS47Townsfolk::AwayNow(TEXT("souvenir_man")) && Sm->Target == TEXT("souv.stoop") && Sm->Arrived == Sm->Target && Sm->Path.IsEmpty() && !Sm->bHidden;
  if (Town.bSmokerIn != bStoop) Town.SetPeople(Town.bWomanIn, bStoop);
 }
 // NPC-DAY-FOLLOW-20261109: the bodies drawn by others, by their own plan, never swapped in the player's sight. Not on a Story
 // night (its town is the story's own): there the woman is put back on her bench, as Freeform had her, and nothing is held.
 if (FS47StoryDirector::Of(M))
 {
  if (BodyShown.Num() && M->TownLayer && M->TownLayer->Town && !M->TownLayer->Town->bWomanIn) M->TownLayer->Town->SetPeople(true, M->TownLayer->Town->bSmokerIn);
  BodyShown.Reset();
 }
 else
 {
  for (const FBody& B : Bodies)
  {
   const FString Who = B.Who;
   const bool bWant = BodyAt(Who, SpotIn(Plans.FindRef(Who), Hour)) && !FS47Townsfolk::AwayNow(FolkOf(Who));
   bool* Was = BodyShown.Find(Who);
   if (!Was || bJump) BodyShown.Add(Who, bWant);
   else *Was = HeldShow(*Was, bWant, Seen(FVector2D(B.X, B.Z)));
  }
  if (M->TownLayer && M->TownLayer->Town)
  {
   FS47Town& Town = *M->TownLayer->Town;
   const bool bBench = BodyShown.FindRef(TEXT("bench_woman"));
   if (Town.bWomanIn != bBench) Town.SetPeople(bBench, Town.bSmokerIn);
  }
 }
 // Each house's lamp while its owner is home and awake; the substation's while he is in.
 for (const FString& Who : People())
 {
  const FString House = HouseOf(Who); if (House.IsEmpty()) continue;
  const FWalker* W = Walkers.Find(Who);
  const float K = W && !FS47Townsfolk::AwayNow(FolkOf(Who)) && HomeAwake(W->Target) && W->Arrived == W->Target ? 1.f : 0.f; // (in the house)
  if (Lamp.FindRef(House, -1.f) != K) { Lamp.Add(House, K); S47CalicheAccess::SetRoomLamp(House, K); }
 }
 {
  const FWalker* S = Walkers.Find(TEXT("deputy"));
  const float K = S && PlaceOf(S->Target) == TEXT("office") && !FS47Townsfolk::AwayNow(TEXT("deputy")) ? 1.f : 0.f;
  if (Lamp.FindRef(TEXT("Sheriff"), -1.f) != K) { Lamp.Add(TEXT("Sheriff"), K); S47CalicheAccess::SetRoomLamp(TEXT("Sheriff"), K); }
 }
 // Talk to him in his booth at lunch.
 if (!bLunchBuilt) BuildLunch(M);
 const bool bSeat = SheriffSeatedAt(TEXT("diner.booth"));
 if (bSeat != bLunchOn) { bLunchOn = bSeat; if (auto* C = LunchBox.Get()) C->SetCollisionEnabled(bSeat ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision); }
}

bool S47TownDay::Pose(const FString& InRole, const FVector& DefaultFeet, double DefaultYaw, FVector& OutFeet, double& OutYaw, bool& bOutShow)
{
 using namespace S47TownDayLocal;
 const FString Role = DayKey(InRole);
 if (!bActive || !People().Contains(Role)) return false;
 FWalker& W = Walkers.FindOrAdd(Role);
 W.Anchor = DefaultFeet; W.AnchorYaw = DefaultYaw; W.bAnchor = true;
 if (!W.bPlaced) return false;
 US47Main* M = Main.Get();
 const FSpot* To = SpotOf(W.Target);
 const bool bThere = W.Path.IsEmpty() && To;
 bOutShow = !W.bHidden && !FS47Townsfolk::AwayNow(Role);
 double Y = bThere && To->Kind == FSpot::Anchor ? DefaultFeet.Y : GroundAt(M, W.At.X, W.At.Y);
 // Alive: a slow breath and a little weight shifting standing; lowered behind the table sitting, eased down and up.
 const double T = W.Age + FCrc::StrCrc32(*Role) % 97;
 double Yaw = W.Yaw;
 FVector2D At = W.At;
 // NPC-ANIM: a rigged body's clips step, sway and breathe; its sit clip sits. It gets the path as it is.
 if (const FRig* Rig = Rigged.Find(Role))
 {
  Y -= (Rig->bSeatInClip ? Rig->SitLower : SitDrop) * FMath::SmoothStep(0., 1., W.Sit);
  OutFeet = FVector(At.X, Y, At.Y); OutYaw = Yaw;
  return true;
 }
 if (W.Path.Num())
 {
  // Critics, round 8 (they slid along like statues): a walk read off the distance walked, about 0.7 m a step. The body rises over
  // each planted foot and dips between (two bumps a stride), shifts its weight to the side of the foot it is on (once a stride),
  // and its heading swings a little with the hips. The single pose cannot swing its legs; this is what the body above them does.
  constexpr double StepLength = .68;
  const double Ph = UE_DOUBLE_PI * W.Stride / StepLength;
  const double Rise = FMath::Abs(FMath::Sin(Ph));
  Y += .026 * Rise * Rise; // (never below the ground: the feet are the pose's own)
  const FVector2D Ahead(FMath::Sin(W.Yaw), FMath::Cos(W.Yaw)), Side(Ahead.Y, -Ahead.X);
  At += Side * (.028 * FMath::Sin(Ph));
  Yaw += FMath::DegreesToRadians(2.6) * FMath::Sin(Ph);
 }
 else
 {
  Y += .004 * FMath::Sin(T * 2 * UE_DOUBLE_PI / 4.6);
  Yaw += FMath::DegreesToRadians(1.6) * FMath::Sin(T * 2 * UE_DOUBLE_PI / 9.3) + FMath::DegreesToRadians(.8) * FMath::Sin(T * 2 * UE_DOUBLE_PI / 3.7 + 1.1);
 }
 Y -= SitDrop * FMath::SmoothStep(0., 1., W.Sit);
 OutFeet = FVector(At.X, Y, At.Y); OutYaw = Yaw;
 return true;
}

void S47TownDay::SetRigged(const FString& Role, bool bRigged, bool bSeatInClip, double SitHipsM, double StandHipsM)
{
 using namespace S47TownDayLocal;
 const FString Key = DayKey(Role);
 if (!bRigged) { Rigged.Remove(Key); return; }
 FRig Rig; Rig.bSeatInClip = bSeatInClip;
 // The old figure's hips at a table: standing hips less SitDrop. The clip's are higher; the difference is the seat's.
 if (bSeatInClip && SitHipsM > 0 && StandHipsM > 0) Rig.SitLower = FMath::Clamp(SitHipsM - (StandHipsM - SitDrop), 0., .12);
 Rigged.Add(Key, Rig);
}

bool S47TownDay::SitNow(const FString& Role, double& OutSit)
{
 using namespace S47TownDayLocal;
 const FWalker* W = bActive ? Walkers.Find(DayKey(Role)) : nullptr;
 if (!W || !W->bPlaced) return false;
 OutSit = FMath::SmoothStep(0., 1., W->Sit);
 return true;
}

FString S47TownDay::SpotNow(const FString& Role)
{
 const S47TownDayLocal::FWalker* W = S47TownDayLocal::Walkers.Find(S47TownDayLocal::DayKey(Role));
 return S47TownDayLocal::bActive && W ? W->Target : FString();
}

FString S47TownDay::DevState(US47Main* M)
{
 using namespace S47TownDayLocal;
 if (!bActive) return TEXT("town day: not running (Freeform only)");
 TArray<FString> Parts;
 for (const FString& Who : People())
 {
  const FWalker* W = Walkers.Find(Who); if (!W) continue;
  Parts.Add(FString::Printf(TEXT("%s %s (%s)%s%s%s at %.1f %.1f"), *Who, *W->Target, *PlaceOf(W->Target), W->Path.Num() ? TEXT(" walking") : TEXT(""),
   W->bHidden ? TEXT(" unseen") : TEXT(""), FS47Townsfolk::AwayNow(Who) ? TEXT(" GONE") : TEXT(""), W->At.X, W->At.Y));
 }
 const bool bIn = M && M->TownLayer && M->TownLayer->Town && M->TownLayer->Town->bDeputyIn;
 return FString::Printf(TEXT("%02d:%02d; %s; deputyIn=%d car=%d lunchTalk=%d"), int32(Hour), int32(FMath::Fmod(Hour, 1.) * 60), *FString::Join(Parts, TEXT("; ")), bIn, bHeldCar || bIn, bLunchOn);
}

int32 S47TownDay::MeasureSpots(AS47Web* W, TArray<FString>& Lines)
{
 using namespace S47TownDayLocal;
 if (!W) { Lines.Add(TEXT("FAIL town day spots: no world")); return 1; }
 TArray<FS47Rect> Cs;
 for (const TCHAR* Area : { TEXT("diner"), TEXT("road") }) for (const FS47Rect& R : W->CollidersMutable(Area))
  if (R.bEnabled && !R.Id.StartsWith(TEXT("access:door:")) && R.MaxX - R.MinX < 400 && R.MaxZ - R.MinZ < 400) Cs.Add(R);
 // Boxes of the export's walk list that keep the player out of a place a townsman is meant to be in (sweep of 9 November: five of
 // the "6 in something"). The diner's kitchen behind its partition (web Diner.ts: "rear kitchen/service space is closed behind a
 // partition and hatch"; diner_geometry.py: the partition at bx -5.9 to -5.7, the range's bench at bx -7.7 to -6.4): the cook works
 // there, between the two, and goes out by its back. The second booth's box, seats and table in one: the sheriff sits in it.
 struct FMeant { double MinX, MaxX, MinZ, MaxZ; bool bSitOnly; };
 static const FMeant Meant[] = { { 7971.95, 7974.25, 1991.15, 2008.75, false }, { 7979.68, 7981.62, 1995.66, 1998.14, true } };
 const auto IsMeant = [](const FS47Rect& R, bool bSit)
 {
  for (const FMeant& K : Meant)
   if ((bSit || !K.bSitOnly) && FMath::Abs(R.MinX - K.MinX) < .02 && FMath::Abs(R.MaxX - K.MaxX) < .02 && FMath::Abs(R.MinZ - K.MinZ) < .02 && FMath::Abs(R.MaxZ - K.MaxZ) < .02) return true;
  return false;
 };
 const auto In = [&Cs, &IsMeant](const FVector2D& P, double Pad, bool bSit, FString& Which)
 {
  for (const FS47Rect& R : Cs)
  {
   // A seated spot is in its chair or stool on purpose: small furniture under half a square metre does not count there.
   if (bSit && (R.MaxX - R.MinX) * (R.MaxZ - R.MinZ) < .5) continue;
   if (IsMeant(R, bSit)) continue;
   if (P.X > R.MinX - Pad && P.X < R.MaxX + Pad && P.Y > R.MinZ - Pad && P.Y < R.MaxZ + Pad) { Which = R.Id.IsEmpty() ? TEXT("(unnamed)") : R.Id; return true; }
  }
  return false;
 };
 int32 Bad = 0; TArray<FString> Each;
 for (const FSpot& S : AllSpots)
 {
  if (S.Kind != FSpot::Stand && S.Kind != FSpot::Sit) continue;
  FString Which;
  const bool B = In(FVector2D(S.X, S.Z), .12, S.Kind == FSpot::Sit, Which);
  Bad += B; Each.Add(FString::Printf(TEXT("%s spot %s at %.2f %.2f%s"), B ? TEXT("FAIL") : TEXT("PASS"), S.Id, S.X, S.Z, B ? *(TEXT(" in ") + Which) : TEXT("")));
 }
 for (const auto& E : Edges)
 {
  const FVector2D A = NodeAt(NodeIndex(E[0])), B = NodeAt(NodeIndex(E[1]));
  const int32 N = FMath::Max(1, FMath::CeilToInt(FVector2D::Distance(A, B) / .25));
  FString Which; bool Hit = false;
  for (int32 K = 0; K <= N && !Hit; ++K) Hit = In(FMath::Lerp(A, B, double(K) / N), .05, false, Which);
  Bad += Hit; if (Hit) Each.Add(FString::Printf(TEXT("FAIL path %s to %s crosses %s"), E[0], E[1], *Which));
 }
 Lines.Add(FString::Printf(TEXT("%s town day: %d spots and %d paths against %d colliders, %d in something"), Bad ? TEXT("FAIL") : TEXT("PASS"), AllSpots.Num(), int32(UE_ARRAY_COUNT(Edges)), Cs.Num(), Bad));
 Lines.Append(Each);
 return Bad;
}

bool S47TownDay::SelfTest(TArray<FString>& Lines)
{
 using namespace S47TownDayLocal;
 bool bOk = true;
 const auto Check = [&](bool B, const FString& Why) { bOk &= B; Lines.Add(FString::Printf(TEXT("%s town day: %s"), B ? TEXT("PASS") : TEXT("FAIL"), *Why)); };
 const int32 Seeds[] = { 1, 47, -9, 2024, 77777 };
 bool bOffice10 = true, bHome23 = true, bLunch = true, bEvening = true, bNight = true, bCover = true, bKnown = true, bSunday = true;
 bool bCook = true, bShop = true, bGas = true;
 FString Why;
 for (int32 Seed : Seeds) for (int32 Day = 0; Day < 21; ++Day) for (int32 WeekDay = 0; WeekDay < 7; ++WeekDay)
 {
  const bool bWork = WeekDay < 5;
  for (const FString& Who : People())
  {
   const TArray<FRow> Rows = Plan(Seed, Day, WeekDay, Who);
   double T = 0; for (const FRow& R : Rows) { bCover &= FMath::IsNearlyEqual(R.From, T, 1e-6) && R.To > R.From; bKnown &= Spot(R.Spot) != nullptr; T = R.To; }
   bCover &= FMath::IsNearlyEqual(T, 24., 1e-6);
  }
  const TArray<FRow> S = Plan(Seed, Day, WeekDay, TEXT("deputy"));
  if (bWork && PlaceOf(SpotIn(S, 10.)) != TEXT("office")) { bOffice10 = false; Why = FString::Printf(TEXT("seed %d day %d: %s"), Seed, Day, *SpotIn(S, 10.)); }
  bHome23 &= PlaceOf(SpotIn(S, 23.)) == TEXT("home");
  if (bWork) bLunch &= PlaceOf(SpotIn(S, 12.3)) == TEXT("diner");
  bEvening &= PlaceOf(SpotIn(S, 20.)) == TEXT("home");
  bNight &= PlaceOf(SpotIn(S, 2.)) == TEXT("home");
  if (WeekDay == 6) bSunday &= PlaceOf(SpotIn(S, 10.)) == TEXT("home");
  const TArray<FRow> C = Plan(Seed, Day, WeekDay, TEXT("cook"));
  for (double H : { 7., 12., 19. }) bCook &= PlaceOf(SpotIn(C, H)) == TEXT("diner");
  bCook &= PlaceOf(SpotIn(C, 23.5)) == TEXT("home");
  const TArray<FRow> V = Plan(Seed, Day, WeekDay, TEXT("souvenir_man"));
  for (double H = 10.; H < 17.; H += .25) bShop &= PlaceOf(SpotIn(V, H)) == TEXT("souvenir") && FS47TownUse::OpenAt(TEXT("souvenir"), H);
  bShop &= PlaceOf(SpotIn(V, 20.)) == TEXT("home") && !FS47TownUse::OpenAt(TEXT("souvenir"), 20.);
  const TArray<FRow> G = Plan(Seed, Day, WeekDay, TEXT("attendant"));
  for (double H = 0; H < 24; H += .5) bGas &= PlaceOf(SpotIn(G, H)) == TEXT("gas");
 }
 Check(bCover, TEXT("every plan covers the whole day with known spots, no gaps"));
 Check(bOffice10, TEXT("the sheriff is at the substation at 10:00 on a weekday") + (Why.IsEmpty() ? FString() : TEXT(" (") + Why + TEXT(")")));
 Check(bHome23 && bEvening && bNight, TEXT("and at home at 20:00, 23:00 and 02:00 (no round)"));
 Check(bLunch, TEXT("at the Mesa Diner at 12:18 on a weekday"));
 Check(bSunday, TEXT("at home on a Sunday morning"));
 {
  FNight Prev; Prev.bRound = true; Prev.Due = 25.0; FNight Tonight; Tonight.bRound = true; Tonight.Due = 23.0;
  const TArray<FRow> S = Plan(47, 3, 2, TEXT("deputy"), Prev, Tonight);
  Check(PlaceOf(SpotIn(S, 1.)) == TEXT("patrol") && PlaceOf(SpotIn(S, 23.)) == TEXT("patrol") && PlaceOf(SpotIn(S, 3.)) == TEXT("home"),
   TEXT("out in the car for the county's round, the hour it is due (01:00, 23:00), home again after"));
 }
 Check(bCook, TEXT("the diner's cook in the kitchen at 07, 12 and 19, home at night"));
 Check(bShop, TEXT("the souvenir man in his shop through its hours (FS47TownUse 10 to 17), home at 20"));
 Check(bGas, TEXT("the attendant at the station all day and night (the night window)"));
 {
   // N1110: the table of hours and habits: every row inside the day, the people with a plan where their plan has them.
   bool bRows = Routine().Num() > 0, bPlan = true, bFolks = true; FString Bad;
   TSet<FString> Folks;
   for (const FHabit& H : Routine())
   {
    bRows &= H.From >= 0 && H.To <= 24 && H.To > H.From && FCString::Strlen(H.Seen) > 20 && !FString(H.Seen).Contains(TEXT("taken"));
    Folks.Add(H.Who);
    if (FString(H.Who) == TEXT("dale") || FString(H.Who) == TEXT("eloy")) continue;
    for (int32 Seed : Seeds) for (int32 Day = 0; Day < 14; ++Day)
    {
     if (FString(H.Who) == TEXT("bench_woman") && BenchWeather(Seed, Day, (H.From + H.To) * .5)) continue; // (home out of the weather)
     const FString Place = PlaceOf(SpotIn(Plan(Seed, Day, Day % 7, H.Who), (H.From + H.To) * .5));
     if (Place != H.Place) { bPlan = false; Bad = FString::Printf(TEXT("%s %.2f seed %d day %d: %s"), H.Who, (H.From + H.To) * .5, Seed, Day, *Place); }
    }
   }
   for (const TCHAR* Nm : { TEXT("dale"), TEXT("eloy"), TEXT("earl"), TEXT("bench_woman"), TEXT("mesa_waitress"), TEXT("cook") }) bFolks &= Folks.Contains(Nm);
   Check(bRows && bFolks && bPlan && HabitAt(TEXT("cook"), 15.1).Contains(TEXT("smokes")) && HabitAt(TEXT("eloy"), 18.2).Contains(TEXT("dog")) && HabitAt(TEXT("dale"), 12.).Len() > 0,
    TEXT("hours to places and one noticeable habit each for Dale, Earl, the bench woman, Eloy Baca, the waitress and the cook, following their plans") + (Bad.IsEmpty() ? FString() : TEXT(" (") + Bad + TEXT(")")));
  }
 {
  // NPC-DAY-FOLLOW-20261109 (14 November): a place for every quarter hour from 06 to 24 for the five who had no day; one place at a
  // time (the body drawn where the plan has it is never also at home with the lamp lit); the swap held in sight; Eloy in town only
  // in the Feed & Seed's hours; Earl on the air only in the Night Line's; the bench empty in the weather.
  bool bEvery = true, bOne = true, bEloy = true, bEarl = true, bWeather = true, bDry = false, bWet = false; FString Bad;
  for (int32 Seed : Seeds) for (int32 Day = 0; Day < 14; ++Day)
  {
   for (const TCHAR* Who : { TEXT("earl"), TEXT("bench_woman"), TEXT("mesa_waitress"), TEXT("cook") })
   {
    const TArray<FRow> Rows = Plan(Seed, Day, Day % 7, Who);
    for (double H = 6; H < 24; H += .25)
    {
     const FString S = SpotIn(Rows, H);
     if (PlaceOf(S).IsEmpty()) { bEvery = false; Bad = FString::Printf(TEXT("%s at %.2f"), Who, H); }
     if (BodyAt(Who, S) && (HomeAwake(S) || PlaceOf(S) == TEXT("home"))) bOne = false;
     if (FString(Who) == TEXT("earl")) bEarl &= !BodyAt(Who, S) && !S47Timeline::EarlOnDuty(H * 3600.);
    }
    for (double H = 0; H < 4; H += .25) if (FString(Who) == TEXT("earl")) bEarl &= BodyAt(Who, SpotIn(Rows, H)) && S47Timeline::EarlOnDuty(H * 3600.);
   }
  }
  // The weather: over two months of a few seeds, every bench hour (18:00 to 04:50) with and without rain or sand.
  for (int32 Seed : Seeds) for (int32 Day = 0; Day < 60; ++Day)
  {
   const TArray<FRow> Rows = Plan(Seed, Day, Day % 7, TEXT("bench_woman"));
   for (double H = .125; H < 24; H += .25)
   {
    if (H > 4.75 && H < 18) continue;
    const bool bBad = BenchWeather(Seed, Day, H), bOn = BodyAt(TEXT("bench_woman"), SpotIn(Rows, H));
    (bBad ? bWet : bDry) = true;
    bWeather &= bOn != bBad;
   }
  }
  for (double H = 6; H < 24; H += .25)
  {
   FString Place; HabitAt(TEXT("eloy"), H, &Place); bEvery &= !Place.IsEmpty() && !HabitAt(TEXT("dale"), H).IsEmpty();
   const FString E = EloyAt(H);
   bEloy &= (E == TEXT("baca") || E == TEXT("feed")) && (E != TEXT("feed") || FS47TownUse::OpenAt(TEXT("feed"), H));
  }
  bEloy &= EloyAt(10) == TEXT("baca") && EloyAt(12.5) == TEXT("feed") && EloyAt(23) == TEXT("baca") && EloyAt(3) == TEXT("baca");
  Check(bEvery, TEXT("Earl, the bench woman, the waitress, the cook, Eloy Baca and Dale (a voice) have a place for every quarter hour 06 to 24") + (Bad.IsEmpty() ? FString() : TEXT(" (") + Bad + TEXT(")")));
  // REVIEW (14 Nov, B5): falsifiable: every spot a body can be drawn at, of every spot there is, is no home spot; and the plans do
  // draw each of the two who have a body and do put each at home awake, so the check above has something to hold
  for (const FSpot& Sp : AllSpots) for (const TCHAR* Who : { TEXT("bench_woman"), TEXT("mesa_waitress"), TEXT("earl") })
   if (BodyAt(Who, Sp.Id) && (HomeAwake(Sp.Id) || FString(Sp.Place) == TEXT("home"))) bOne = false;
  for (const TCHAR* Who : { TEXT("bench_woman"), TEXT("mesa_waitress") })
  {
   bool bDrawn = false, bHome = false;
   for (int32 Seed : Seeds) for (int32 Day = 0; Day < 14; ++Day)
   {
    const TArray<FRow> Rows = Plan(Seed, Day, Day % 7, Who);
    for (double H = 0; H < 24; H += .25) { const FString S = SpotIn(Rows, H); bDrawn |= BodyAt(Who, S); bHome |= HomeAwake(S); }
   }
   if (!bDrawn || !bHome) { bOne = false; Bad = FString::Printf(TEXT("%s never %s"), Who, bDrawn ? TEXT("at home awake") : TEXT("drawn")); }
  }
  Check(bOne && bEarl, TEXT("never two places at once: a body drawn where its plan has it is never at home too (and each is both, at their hours); Earl in the studio's chair only midnight to four"));
  // REVIEW (14 Nov): coffee through the hatch only once the cook is there, not while he walks in
  Check(CookPours(TEXT("diner.range"), TEXT("diner.range"), false, false) && !CookPours(TEXT("diner.range"), TEXT("home.cook"), true, false) && !CookPours(TEXT("diner.range"), TEXT("diner.range"), true, false)
   && !CookPours(TEXT("diner.back"), TEXT("diner.back"), false, false) && !CookPours(TEXT("diner.range"), TEXT("diner.range"), false, true),
   TEXT("the cook pours at the hatch only when he has got there: not on the way in, not out the back, not away"));
  Check(bEloy, TEXT("Eloy Baca on his porch in the morning and at the Feed & Seed counter at noon, inside its hours, home again by half past one"));
  Check(bWeather && bDry && bWet, TEXT("the bench woman is on her bench on a dry evening and at home in a shower or blowing sand (both seen in these seeds)"));
  Check(HeldShow(true, false, true) && !HeldShow(false, true, true) && !HeldShow(true, false, false) && HeldShow(false, true, false),
   TEXT("a body is never swapped in the player's sight: seen, it stays as it was; unseen, it goes by the plan"));
  // Story nights: nothing of this runs there (Step, with FS47StoryDirector::Of, resets it), and out of the day nobody is held.
  bool bIgnored = false;
  Check(FString(Bodies[0].Who) == TEXT("mesa_waitress") && (bActive || (!Present(TEXT("earl"), bIgnored) && Counter() == TEXT("waitress"))),
   TEXT("out of a running Freeform day (Story, -S47TownDayOff) the bodies and the Mesa's counter keep their own rule"));
 }
 {
  bool bRoutes = true;
  for (const FString& Who : People())
  {
   TSet<FString> Used;
   for (int32 Day = 0; Day < 14; ++Day) for (const FRow& R : Plan(47, Day, Day % 7, Who)) Used.Add(R.Spot);
   for (const FString& A : Used) for (const FString& B : Used)
   {
    const FSpot* SA = Spot(A); const FSpot* SB = Spot(B);
    bRoutes &= SA && SB && NodeIndex(SA->Via) >= 0 && NodeIndex(SB->Via) >= 0 && Route(NodeIndex(SA->Via), NodeIndex(SB->Via)).Num() > 0;
   }
  }
  bRoutes &= Route(NodeIndex(TEXT("OffBack")), NodeIndex(TEXT("DinBooth"))).Num() > 6 && Route(NodeIndex(TEXT("SouvIn")), NodeIndex(TEXT("H03Door"))).Num() == 7;
  for (const FSpot& S : AllSpots) bRoutes &= NodeIndex(S.Via) >= 0;
  Check(bRoutes, TEXT("a way along the known paths between every two spots of a place, office to booth, shop to home"));
 }
 // The planned spots and the paths against the rooms' furniture and walls (Content/CalicheAccess/assembly.json) and the diner's
 // tables, counter and kitchen (Tools/mesadiner/diner_geometry.py, in world terms).
 {
  struct FBox2 { double X0, X1, Z0, Z1; FString Id; };
  TArray<FBox2> Things, Rooms;
  FString Text; TSharedPtr<FJsonObject> J;
  if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("CalicheAccess/assembly.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) && J)
  {
   for (const auto& V : J->GetArrayField(TEXT("buildings")))
   {
    const auto B = V->AsObject(); const FString Id = B->GetStringField(TEXT("id"));
    const auto& Root = B->GetArrayField(TEXT("root")); const auto& Fp = B->GetArrayField(TEXT("footprint"));
    const double RX = Root[0]->AsNumber(), RZ = Root[2]->AsNumber();
    Rooms.Add({ RX + Fp[0]->AsNumber(), RX + Fp[1]->AsNumber(), RZ - Fp[3]->AsNumber(), RZ - Fp[2]->AsNumber(), Id });
    for (const auto& F : B->GetArrayField(TEXT("furniture"))) { const auto& A = F->AsArray(); Things.Add({ A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber(), A[3]->AsNumber(), Id }); }
   }
  }
  const auto Diner = [&Things](double BX0, double BX1, double BY0, double BY1, const TCHAR* Id) { Things.Add({ 7979.95 + BX0, 7979.95 + BX1, 1999.95 - BY1, 1999.95 - BY0, Id }); };
  for (double Y : { 6.1, 3.05, -3.05, -6.1 }) Diner(-.125, 1.525, Y - .38, Y + .38, TEXT("booth table"));
  Diner(-4.8, -4.2, -5.475, 5.475, TEXT("counter")); Diner(-5.9, -5.7, -8.8, 8.8, TEXT("kitchen partition and hatch")); Diner(-7.7, -6.4, 1.2, 3.6, TEXT("kitchen range"));
  if (Rooms.IsEmpty()) Lines.Add(TEXT("SKIP town day: Content/CalicheAccess/assembly.json not read (the rooms' furniture)"));
  int32 Bad = 0; FString First;
  for (const FSpot& S : AllSpots)
  {
   if (S.Kind != FSpot::Stand && S.Kind != FSpot::Sit) continue;
   const double Pad = S.Kind == FSpot::Sit ? 0. : .12;
   for (const FBox2& B : Things)
   {
    if (S.Kind == FSpot::Sit && (B.X1 - B.X0) * (B.Z1 - B.Z0) < .5) continue; // its own chair or stool
    if (S.X > B.X0 - Pad && S.X < B.X1 + Pad && S.Z > B.Z0 - Pad && S.Z < B.Z1 + Pad) { ++Bad; if (First.IsEmpty()) First = FString::Printf(TEXT("%s in %s's %.2f-%.2f x %.2f-%.2f"), S.Id, *B.Id, B.X0, B.X1, B.Z0, B.Z1); }
   }
   for (const FBox2& R : Rooms)
    if (S.X > R.X0 && S.X < R.X1 && S.Z > R.Z0 && S.Z < R.Z1 && (S.X < R.X0 + .3 || S.X > R.X1 - .3 || S.Z < R.Z0 + .3 || S.Z > R.Z1 - .3))
    { ++Bad; if (First.IsEmpty()) First = FString::Printf(TEXT("%s against %s's wall"), S.Id, *R.Id); }
  }
  for (const auto& E : Edges)
  {
   const FVector2D A = NodeAt(NodeIndex(E[0])), B = NodeAt(NodeIndex(E[1]));
   for (int32 K = 0; K <= 40; ++K)
   {
    const FVector2D P = FMath::Lerp(A, B, K / 40.);
    for (const FBox2& T : Things) if (P.X > T.X0 - .05 && P.X < T.X1 + .05 && P.Y > T.Z0 - .05 && P.Y < T.Z1 + .05)
    { ++Bad; if (First.IsEmpty()) First = FString::Printf(TEXT("path %s to %s through %s's %.2f-%.2f x %.2f-%.2f"), E[0], E[1], *T.Id, T.X0, T.X1, T.Z0, T.Z1); break; }
   }
  }
  // Each spot's last step: from its way in to the spot itself (a seated one ends in its chair).
  for (const FSpot& S : AllSpots)
  {
   if (S.Kind != FSpot::Stand && S.Kind != FSpot::Sit) continue;
   const FVector2D A = NodeAt(NodeIndex(S.Via)), B(S.X, S.Z);
   for (int32 K = 0; K <= 30; ++K)
   {
    const FVector2D P = FMath::Lerp(A, B, K / 30.);
    for (const FBox2& T : Things)
    {
     if (S.Kind == FSpot::Sit && (T.X1 - T.X0) * (T.Z1 - T.Z0) < .5) continue;
     if (P.X > T.X0 && P.X < T.X1 && P.Y > T.Z0 && P.Y < T.Z1) { ++Bad; if (First.IsEmpty()) First = FString::Printf(TEXT("the way in to %s through %s"), S.Id, *T.Id); break; }
    }
   }
  }
  Check(Bad == 0, FString::Printf(TEXT("no planned spot or path in furniture, a table, the counter or a wall (%d furniture boxes, %d rooms)%s"), Things.Num(), Rooms.Num(),
   First.IsEmpty() ? TEXT("") : *(TEXT(": ") + First)));
 }
 return bOk;
}

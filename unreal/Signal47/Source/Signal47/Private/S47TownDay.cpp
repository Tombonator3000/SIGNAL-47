#include "S47TownDay.h"
#include "S47CalicheAccess.h"
#include "S47Caliche.h"
#include "S47DeputyRound.h"
#include "S47Freeform.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47TownUse.h"
#include "S47Townsfolk.h"
#include "S47Walker.h"
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
  // the diner's kitchen, behind the hatch (between the partition and the range)
  { TEXT("Kitchen"), 7973.8, 1997.2 },
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
  { TEXT("GasIn"), TEXT("GasOut") }, { TEXT("GasOut"), TEXT("GasFore") },
  { TEXT("SouvIn"), TEXT("SouvDoor") }, { TEXT("SouvDoor"), TEXT("SouvOut") }, { TEXT("SouvOut"), TEXT("EastS") }, { TEXT("EastS"), TEXT("EastN") },
  { TEXT("EastN"), TEXT("H03Out") }, { TEXT("H03Out"), TEXT("H03Door") },
 };
 constexpr double PI_ = UE_DOUBLE_PI;
 const TArray<FSpot> AllSpots = {
  // the sheriff at the substation (desk: the west chair, facing the door; the board, coffee and files along the back)
  { TEXT("off.desk"), TEXT("office"), 7979.62, 2107.25, PI_ / 2, FSpot::Sit, TEXT("OffBack") },
  { TEXT("off.board"), TEXT("office"), 7978.35, 2105.3, -PI_ / 2, FSpot::Stand, TEXT("OffBack") },
  { TEXT("off.coffee"), TEXT("office"), 7978.25, 2109.9, -PI_ / 2, FSpot::Stand, TEXT("OffBack") },
  { TEXT("off.files"), TEXT("office"), 7977.3, 2102.7, -PI_ / 2, FSpot::Stand, TEXT("OffBack") },
  { TEXT("off.window"), TEXT("office"), 7982.7, 2105.3, PI_ / 2, FSpot::Stand, TEXT("OffMid") },
  { TEXT("off.step"), TEXT("office"), 7984.75, 2102.3, PI_ / 2, FSpot::Stand, TEXT("OffOut") },
  { TEXT("off.car"), TEXT("office"), 7986.2, 2105.0, PI_ / 2, FSpot::Stand, TEXT("OffCar") },
  // lunch at the Mesa: the second booth's seat nearest the door, or the fourth stool
  { TEXT("diner.booth"), TEXT("diner"), 7980.35, 1995.97, 0, FSpot::Sit, TEXT("DinBooth") },
  { TEXT("diner.stool"), TEXT("diner"), 7976.55, 1998.35, -PI_ / 2, FSpot::Sit, TEXT("DinAisle") },
  // home (the second house on the east side), the county's round
  { TEXT("home.sheriff"), TEXT("home"), 8028.25, 2156.5, 0, FSpot::Hidden, TEXT("OffCar") },
  { TEXT("sleep.sheriff"), TEXT("home"), 8028.25, 2156.5, 0, FSpot::Hidden, TEXT("OffCar") },
  { TEXT("patrol"), TEXT("patrol"), 7987.7, 2106.5, 0, FSpot::Away, TEXT("OffCar") },
  // the diner's cook: at the hatch, at the range, in the back; home is the house down the south road
  { TEXT("diner.kitchen"), TEXT("diner"), 7973.8, 1997.55, PI_ / 2, FSpot::Stand, TEXT("Kitchen") },
  { TEXT("diner.range"), TEXT("diner"), 7973.8, 1996.7, -PI_ / 2, FSpot::Stand, TEXT("Kitchen") },
  { TEXT("diner.back"), TEXT("diner"), 7973.8, 1997.2, 0, FSpot::Hidden, TEXT("Kitchen") },
  { TEXT("home.cook"), TEXT("home"), 7976.5, 2241.0, 0, FSpot::Hidden, TEXT("Kitchen") },
  { TEXT("sleep.cook"), TEXT("home"), 7976.5, 2241.0, 0, FSpot::Hidden, TEXT("Kitchen") },
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
 };
 // Whose house is whose (S47CalicheAccess room ids).
 FString HouseOf(const FString& Who)
 {
  return Who == TEXT("deputy") ? TEXT("H02") : Who == TEXT("souvenir_man") ? TEXT("H03") : Who == TEXT("waitress") ? TEXT("H04") : FString();
 }
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

 // ---- the running game ----
 struct FWalker
 {
  FString Target, Arrived;          // the spot it is going to, and the one it last reached
  TArray<FVector2D> Path;           // what is left of the way there
  FVector2D At = FVector2D::ZeroVector;
  double Yaw = 0, Age = 0, Phase = 0;
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

 const FSpot* SpotOf(const FString& Id) { return S47TownDay::Spot(Id); }
 bool Visible(const FSpot* S) { return S && S->Kind != FSpot::Hidden && S->Kind != FSpot::Away; }
 FVector2D Goal(const FWalker& W, const FSpot* S) { return S->Kind == FSpot::Anchor ? FVector2D(W.Anchor.X, W.Anchor.Z) : FVector2D(S->X, S->Z); }
 double GroundAt(US47Main* M, double X, double Z)
 {
  const double Floor = S47CalicheAccess::FloorAt(X, Z);
  if (!FMath::IsNaN(Floor)) return Floor;
  if (X > 7972.0 && X < 7981.8 && Z > 1991.1 && Z < 2008.8) return DinerFloor;
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
   W.Yaw = To->Kind == FSpot::Anchor ? W.AnchorYaw : To->Yaw; W.bSeated = To->Kind == FSpot::Sit;
   return;
  }
  // Unseen to unseen (home to bed, home to the night's round): nothing to walk.
  if (W.bHidden && !Visible(To)) { W.Arrived = W.Target; W.bSeated = false; return; }
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
 void Walk(FWalker& W, double Dt)
 {
  W.Age += Dt;
  const FSpot* To = SpotOf(W.Target);
  double Left = Speed * Dt;
  while (Left > 0 && W.Path.Num())
  {
   const FVector2D D = W.Path[0] - W.At; const double L = D.Size();
   if (L > .01) { const double Want = FMath::Atan2(D.X, D.Y); W.Yaw = Want; }
   if (L <= Left)
   {
    W.At = W.Path[0]; W.Path.RemoveAt(0); Left -= L;
    // Through a shut door: it opens (the substation's, the shop's), as a hand on it would; nobody shuts it after them.
    for (const TCHAR* Door : { TEXT("OffDoor"), TEXT("SouvDoor") })
     if (FVector2D::Distance(W.At, NodeAt(NodeIndex(Door))) < .01)
     {
      const TCHAR* Room = FCString::Strcmp(Door, TEXT("OffDoor")) == 0 ? TEXT("Sheriff") : TEXT("Souvenir");
      if (S47CalicheAccess::HasDoor(Room) && !S47CalicheAccess::IsOpen(Room)) { S47CalicheAccess::SetOpenQuietly(Room, true); S47CalicheAccess::PoseDoor(Room, -1); }
     }
   }
   else { W.At += D / L * Left; Left = 0; }
  }
  if (W.Path.IsEmpty() && To && W.Arrived != W.Target)
  {
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
  I.Label = [] { return SheriffSeatedAt(TEXT("diner.booth")) ? FString(TEXT("Talk to the sheriff")) : FString(); };
  I.Use = []
  {
   US47Main* Run = Main.Get();
   if (Run && SheriffSeatedAt(TEXT("diner.booth")) && Run->TownLayer && Run->TownLayer->Caliche) Run->TownLayer->Caliche->Talk(TEXT("deputy"));
  };
  I.Examine = [] { return SheriffSeatedAt(TEXT("diner.booth")) ? FString(TEXT("The sheriff, hat on the seat beside him, eating the special without looking at it.")) : FString(); };
  M->Inter->Add(I); W->MarkUsable(Node, I.Id);
 }
}

const TArray<FString>& S47TownDay::People()
{
 static const TArray<FString> P = { TEXT("deputy"), TEXT("waitress"), TEXT("attendant"), TEXT("souvenir_man") };
 return P;
}
const TArray<S47TownDay::FSpot>& S47TownDay::Spots() { return S47TownDayLocal::AllSpots; }
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
    Add(Rows, L0, L1, R.FRand() < .65f ? TEXT("diner.booth") : TEXT("diner.stool"));
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
 else if (Who == TEXT("waitress"))
 {
  // The diner's cook: 06 to 21 every day, a break in the afternoon.
  const double Arrive = J(5.7, 5.95), Leave = J(20.7, 20.95), B0 = J(14.3, 14.8), Sleep = J(22.3, 23.0);
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
 return Rows;
}

// ---------------------------------------------------------------- the running game

bool S47TownDay::Active() { return S47TownDayLocal::bActive; }

void S47TownDay::Stop()
{
 using namespace S47TownDayLocal;
 US47Main* M = Main.Get();
 if (bActive && M && M->TownLayer && M->TownLayer->Town) M->TownLayer->Town->SetCarHeld(false);
 for (const auto& Pair : Lamp) S47CalicheAccess::SetRoomLamp(Pair.Key, 1);
 if (auto* C = LunchBox.Get()) C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 bActive = false; bLunchOn = false; Walkers.Reset(); Plans.Reset(); Lamp.Reset(); PlanDay = -1; LastClock = -1;
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
 const bool bTown = M->Web->Area() == TEXT("diner");
 const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
 for (const FString& Who : People())
 {
  FWalker& W = Walkers.FindOrAdd(Who);
  const FString Want = SpotIn(Plans.FindRef(Who), Hour);
  const FSpot* To = SpotOf(Want);
  if (!To) continue;
  if (To->Kind == FSpot::Anchor && !W.bAnchor) continue; // not stood anywhere yet: next frame
  if (Want != W.Target || bJump)
  {
   const FVector2D G = Visible(To) ? Goal(W, To) : FVector2D(To->X, To->Z);
   const bool bFar = FVector2D::Distance(FVector2D(P.X, P.Z), W.At) > 90 && FVector2D::Distance(FVector2D(P.X, P.Z), G) > 90;
   W.Target = Want;
   Lead(W, To, bJump || !bTown || bFar);
  }
  Walk(W, Dt);
 }
 // The sheriff's place is the town's: his window lit and his door answered while he is in; the car outside while he is in town.
 if (M->TownLayer && M->TownLayer->Town)
 {
  FS47Town& Town = *M->TownLayer->Town;
  const FWalker* S = Walkers.Find(TEXT("deputy"));
  const bool bGone = FS47Townsfolk::AwayNow(TEXT("deputy"));
  const FString Place = S ? PlaceOf(S->Target) : FString();
  const bool bIn = !bGone && Place == TEXT("office");
  const bool bCar = !bGone && S && (Place == TEXT("office") || Place == TEXT("diner") || (!S->bHidden && S->Path.Num()));
  if (Town.bDeputyIn != bIn) Town.SetDeputy(bIn);
  if (bHeldCar != (bCar && !bIn)) { bHeldCar = bCar && !bIn; Town.SetCarHeld(bHeldCar); }
 }
 // Each house's lamp while its owner is home and awake; the substation's while he is in.
 for (const FString& Who : People())
 {
  const FString House = HouseOf(Who); if (House.IsEmpty()) continue;
  const FWalker* W = Walkers.Find(Who);
  const float K = W && !FS47Townsfolk::AwayNow(Who) && HomeAwake(W->Target) ? 1.f : 0.f;
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

bool S47TownDay::Pose(const FString& Role, const FVector& DefaultFeet, double DefaultYaw, FVector& OutFeet, double& OutYaw, bool& bOutShow)
{
 using namespace S47TownDayLocal;
 if (!bActive || !People().Contains(Role)) return false;
 FWalker& W = Walkers.FindOrAdd(Role);
 W.Anchor = DefaultFeet; W.AnchorYaw = DefaultYaw; W.bAnchor = true;
 if (!W.bPlaced) return false;
 US47Main* M = Main.Get();
 const FSpot* To = SpotOf(W.Target);
 const bool bThere = W.Path.IsEmpty() && To;
 bOutShow = !W.bHidden && !FS47Townsfolk::AwayNow(Role);
 double Y = bThere && To->Kind == FSpot::Anchor ? DefaultFeet.Y : GroundAt(M, W.At.X, W.At.Y);
 // Alive: a slow breath and a little weight shifting standing, a step's rise and fall walking, lowered behind the table sitting.
 const double T = W.Age + FCrc::StrCrc32(*Role) % 97;
 double Yaw = W.Yaw;
 if (W.Path.Num()) Y += .022 * FMath::Abs(FMath::Sin(T * UE_DOUBLE_PI * 1.85));
 else
 {
  Y += .004 * FMath::Sin(T * 2 * UE_DOUBLE_PI / 4.6);
  Yaw += FMath::DegreesToRadians(1.6) * FMath::Sin(T * 2 * UE_DOUBLE_PI / 9.3) + FMath::DegreesToRadians(.8) * FMath::Sin(T * 2 * UE_DOUBLE_PI / 3.7 + 1.1);
  if (W.bSeated) Y -= SitDrop;
 }
 OutFeet = FVector(W.At.X, Y, W.At.Y); OutYaw = Yaw;
 return true;
}

FString S47TownDay::SpotNow(const FString& Role)
{
 const S47TownDayLocal::FWalker* W = S47TownDayLocal::Walkers.Find(Role);
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
 const auto In = [&Cs](const FVector2D& P, double Pad, bool bSit, FString& Which)
 {
  for (const FS47Rect& R : Cs)
  {
   // A seated spot is in its chair or stool on purpose: small furniture under half a square metre does not count there.
   if (bSit && (R.MaxX - R.MinX) * (R.MaxZ - R.MinZ) < .5) continue;
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
  const TArray<FRow> C = Plan(Seed, Day, WeekDay, TEXT("waitress"));
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

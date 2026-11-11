#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47FarmBuildings.h"
#include "S47FarmHouse.h"
#include "S47FarmLights.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47Happenings.h"
#include "S47Main.h"
#include "S47Omen.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47FarmLightsDev, Log, All);

// F10, "Farm lights (T)" (FARM-LIGHTS, Tom 9 November about 13:30): the barn's swinging bulb, the yard's dusk-to-dawn lamp and the old
// house's lit room, each seen at the hour it matters from where a player stands: in the barn and from the yard, by evening and at night,
// the house from the road side as in Tom's picture, close, from inside and on its ground. Lines that push or start something say so;
// each writes what the lights do to the log (LogS47FarmLightsDev). The farm is walked in SARO's terms. Nothing here is saved.
// -S47DevSweep=/tmp/dir "-S47DevSweepGroup=Farm lights (T)" runs the group in turn, a picture after each line.
void FS47DevFreeform::AddFarmLights(FS47DevEvents& E)
{
 static const TCHAR* const Group = TEXT("Farm lights (T)");
 const auto Saro = [](const FVector& Road) { return S47FarmWorld::RoadToArea(TEXT("saro"), Road); };
 // The barn's own metres (x east, y north) and the house's plan metres, on the ground there, in SARO's terms.
 const auto Barn = [Saro](double X, double Y, double Up)
 {
  const FVector2D P = S47FarmBuildings::LocalToRoad(S47FarmBuildings::Sites()[0], X, Y);
  return Saro(FVector(P.X, S47FarmWorld::FloorRoad(P.X, P.Y) + Up, P.Y));
 };
 const auto Plan = [Saro](double X, double Y, double Up) { const FVector P = S47FarmHouse::Point(X, Y); return Saro(FVector(P.X, S47FarmWorld::FloorRoad(P.X, P.Z) + Up, P.Z)); };
 const auto Say = [this](const FString& What)
 {
  const FString State = S47FarmLights::DevState(M);
  UE_LOG(LogS47FarmLightsDev, Display, TEXT("S47: %s; %s"), *What, *State);
  Tell(true, What + TEXT(". ") + State, 9);
 };
 // Stand at Stand at an hour, look at Look, then do Then (after the scene has settled).
 const auto View = [this, Say](double Hour, TFunction<FVector()> Stand, TFunction<FVector()> Look, const FString& What, TFunction<void()> Then)
 {
  FStage S; S.From = Hour; S.To = Hour + .4; S.Area = TEXT("saro"); S.Where = [Stand] { const FVector P = Stand(); return FVector2D(P.X, P.Z); };
  Go(S, [this, Look, What, Then, Say]
  {
   if (!M->Player) { Tell(false, TEXT("no player")); return; }
   AimAt(Look());
   if (Then) Then();
   After(1.6, [What, Say] { Say(What); });
  });
 };
 const auto Bulb = [] { return S47FarmWorld::RoadToArea(TEXT("saro"), S47FarmLights::BarnPivot() - FVector(0, S47FarmLights::CordLength + .1, 0)); };
 const auto YardLamp = [] { return S47FarmWorld::RoadToArea(TEXT("saro"), S47FarmLights::YardMount() + S47FarmLights::YardOut() * 1.5); };
 const auto Window = [Plan] { return Plan(12, 6.85, 1.5); };

 // ---- the barn ----
 E.AddFreeform(Group, TEXT("Barn: inside at 21:00"), TEXT("in the aisle by the door, looking up the aisle at the bulb"), [View, Barn, Bulb]
 {
  View(21, [Barn] { return Barn(1.3, -4.6, 0); }, Bulb, TEXT("FARM-LIGHTS: the barn's bulb at 21:00, swinging in the draught"), nullptr);
 });
 E.AddFreeform(Group, TEXT("Barn: the bulb knocked (02:00)"), TEXT("in the aisle; the bulb is pushed and swings, the posts' shadows walking the walls"), [this, View, Barn, Bulb]
 {
  View(2, [Barn] { return Barn(1.3, -4.6, 0); }, Bulb, TEXT("FARM-LIGHTS: the bulb knocked at 02:00"), [this] { S47FarmLights::DevKick(M, .10, .18); });
 });
 E.AddFreeform(Group, TEXT("Barn: something at the door (02:00)"), TEXT("in the aisle; something stands 12 m outside the door for 25 s: the bulb flickers and swings"), [this, View, Barn, Bulb]
 {
  View(2, [Barn] { return Barn(1.3, -3.6, 0); }, Bulb, TEXT("FARM-LIGHTS: something at the barn's door"), [this, Barn]
  {
   if (M->Omen) M->Omen->DevPresence(Barn(1.8, -17.5, 1), .8, 25, true);
  });
 });
 E.AddFreeform(Group, TEXT("Barn: at noon, the bulb dark"), TEXT("in the aisle at 12:00"), [View, Barn, Bulb]
 {
  View(12, [Barn] { return Barn(1.3, -4.6, 0); }, Bulb, TEXT("FARM-LIGHTS: the barn at noon"), nullptr);
 });
 E.AddFreeform(Group, TEXT("Barn: from the yard at 22:00"), TEXT("18 m out from the big door, looking in"), [View, Barn, Bulb]
 {
  View(22, [Barn] { return Barn(1.8, -23.5, 0); }, Bulb, TEXT("FARM-LIGHTS: the barn from the yard"), nullptr);
 });

 // ---- the yard ----
 E.AddFreeform(Group, TEXT("Yard: the lamp comes on (18:50)"), TEXT("in the yard just after the photocell trips: blue and dim, white over some minutes"), [View, Barn, YardLamp]
 {
  View(18.83, [Barn] { return Barn(4, -26, 0); }, YardLamp, TEXT("FARM-LIGHTS: the yard lamp warming up"), nullptr);
 });
 E.AddFreeform(Group, TEXT("Yard: at 23:00 from the house"), TEXT("by the old house's yard side, looking across the yard at the barn's lamp"), [View, Plan, YardLamp]
 {
  View(23, [Plan] { return Plan(-4, 5, 0); }, YardLamp, TEXT("FARM-LIGHTS: the yard at 23:00"), nullptr);
 });
 E.AddFreeform(Group, TEXT("Yard: something near, the lamp goes out"), TEXT("23:00 in the yard; something 20 m off for 30 s: the mercury lamp drops out, and comes back slowly after"), [this, View, Barn, YardLamp]
 {
  View(23, [Barn] { return Barn(4, -26, 0); }, YardLamp, TEXT("FARM-LIGHTS: something near the yard lamp"), [this, Barn]
  {
   if (M->Omen) M->Omen->DevPresence(Barn(14, -14, 1), .85, 30, true);
  });
 });

 // ---- the old house ----
 E.AddFreeform(Group, TEXT("House: Tom's view at 21:00"), TEXT("25 m out from the sitting room's window, the road's side"), [View, Plan, Window]
 {
  View(21, [Plan] { return Plan(37, 6.85, 0); }, Window, TEXT("FARM-LIGHTS: the house's window at 21:00, as in Tom's picture"), nullptr);
 });
 E.AddFreeform(Group, TEXT("House: the window close (21:00)"), TEXT("7 m out: the room behind, its blind, the rocking chair against the light"), [View, Plan, Window]
 {
  View(21, [Plan] { return Plan(19, 6.6, 0); }, Window, TEXT("FARM-LIGHTS: the window close"), nullptr);
 });
 E.AddFreeform(Group, TEXT("House: the window's light on the ground"), TEXT("21:00, 6 m off to the side, looking down at the ground under the window"), [View, Plan]
 {
  View(21, [Plan] { return Plan(17, 10.5, 0); }, [Plan] { return Plan(14.2, 6.85, 0); }, TEXT("FARM-LIGHTS: the window's light on the ground"), nullptr);
 });
 E.AddFreeform(Group, TEXT("House: inside the sitting room (21:00)"), TEXT("by the hall door, looking at the window"), [View, Plan, Window]
 {
  View(21, [Plan] { return Plan(7.4, 7.0, 0); }, Window, TEXT("FARM-LIGHTS: the sitting room lit"), nullptr);
 });
 E.AddFreeform(Group, TEXT("House: 23:30, the timer off"), TEXT("Tom's view again: the house dark, nobody lives there"), [View, Plan, Window]
 {
  View(23.5, [Plan] { return Plan(37, 6.85, 0); }, Window, TEXT("FARM-LIGHTS: the house at 23:30"), nullptr);
 });
 E.AddFreeform(Group, TEXT("House: a light where nobody lives (01:30)"), TEXT("70 m off facing it, the night's house_lit started; walk up and it goes out at 18 m"), [this, View, Plan, Window]
 {
  View(1.5, [Plan] { return Plan(82, 6.85, 0); }, Window, TEXT("FARM-LIGHTS: the night's light in the empty house"), [this]
  {
   FString Why;
   FS47FreeformSession* F = Session();
   const bool bOk = F && F->Happenings && F->Happenings->DevStart(TEXT("house_lit"), FString(), Why);
   if (!bOk) Tell(false, Why.IsEmpty() ? FString(TEXT("Freeform only")) : Why);
  });
 });
 E.AddFreeform(Group, TEXT("Farm lights: what each does now"), TEXT("as you are; to the log"), [Say] { Say(TEXT("FARM-LIGHTS: now")); });
}

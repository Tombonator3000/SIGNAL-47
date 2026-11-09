#include "S47DevFreeform.h"
#include "S47CarPaint.h"
#include "S47DevEvents.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47Town.h"
#include "S47TownDay.h"
#include "S47TownLayer.h"
#include "S47Townsfolk.h"
#include "S47Web.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47TownDayDev, Log, All);

// F10, "Town day (T)" (NPC-DAY, Tom 9 November about 13:50): the sheriff at 09, 12, 15, 20 and 02 and the diner at 07, 12 and 19, each
// seen from where a player would stand (the street outside the substation's window, the diner's door, the road by his house), with
// where the day has each of them, whether the town's switches agree (his window and door, the car outside) and, for the sheriff, what
// the day says he should be doing at that hour on this weekday. Then the planned spots and paths against the walk's colliders, and the
// cars' paint (SHERIFF-CAR). Each line writes to the log (LogS47TownDayDev). Nothing here is saved.
void FS47DevFreeform::AddTownDay(FS47DevEvents& E)
{
 static const TCHAR* const Group = TEXT("Town day (T)");
 constexpr double Office = 7.192696381179047, Diner = 4.309006389843889;
 const auto Say = [this](bool bOk, const FString& What)
 {
  const FString State = S47TownDay::DevState(M);
  UE_LOG(LogS47TownDayDev, Display, TEXT("S47: %s %s; %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), *What, *State);
  Tell(bOk, What + TEXT(". ") + State, 12);
 };
 // At Hour, stand at Stand (the diner area's metres), look at Look, then judge with Check after the walkers have settled.
 const auto View = [this, Say](double Hour, FVector2D Stand, FVector Look, const FString& What, TFunction<bool(FString&)> Check)
 {
  FStage S; S.From = Hour; S.To = Hour + .08; S.Area = TEXT("diner"); S.Where = [Stand] { return Stand; };
  Go(S, [this, Look, What, Check, Say]
  {
   if (!M->Player) { Tell(false, TEXT("no player")); return; }
   AimAt(Look);
   After(2.5, [What, Check, Say] { FString Why; const bool bOk = Check ? Check(Why) : true; Say(bOk, What + (Why.IsEmpty() ? FString() : TEXT(" (") + Why + TEXT(")"))); });
  });
 };
 // The sheriff: where the day has him, and the town agreeing (lit window and answered door only while he is in).
 const auto Sheriff = [this](TArray<FString> Places, FString& Why)
 {
  const FString Spot = S47TownDay::SpotNow(TEXT("deputy")), Place = S47TownDay::PlaceOf(Spot);
  const FS47FreeformSession* F = Session();
  const int32 WeekDay = F ? int32(F->DateOfDay(FMath::FloorToInt(F->Clock() / 86400.) + 1).GetDayOfWeek()) : -1;
  const bool bIn = M->TownLayer && M->TownLayer->Town && M->TownLayer->Town->bDeputyIn;
  const bool bGone = FS47Townsfolk::AwayNow(TEXT("deputy"));
  Why = FString::Printf(TEXT("%s, %s, weekday %d%s"), *Spot, *Place, WeekDay, bGone ? TEXT(", gone (Townsfolk)") : TEXT(""));
  // On a weekend the weekday's places are not expected: the line only checks the town agrees.
  const bool bWeekday = WeekDay >= 0 && WeekDay < 5;
  return S47TownDay::Active() && bIn == (!bGone && Place == TEXT("office")) && (bGone || !bWeekday || Places.Contains(Place));
 };
 E.AddFreeform(Group, TEXT("Sheriff at 09:00"), TEXT("from the street, through the substation's window: at his desk or about the office"), [View, Sheriff]
 { View(9, FVector2D(7993, 2106.5), FVector(7980, Office + 1.1, 2107), TEXT("NPC-DAY: the sheriff at 09:00"), [Sheriff](FString& W) { return Sheriff({ TEXT("office") }, W); }); });
 E.AddFreeform(Group, TEXT("Sheriff at 12:00 (lunch)"), TEXT("at the diner's door: in his booth or at the counter, or on his way; his door says BACK SOON"), [View, Sheriff]
 { View(12.15, FVector2D(7984.5, 2001.2), FVector(7980.3, Diner + .9, 1996.6), TEXT("NPC-DAY: the sheriff at lunch"), [Sheriff](FString& W) { return Sheriff({ TEXT("diner"), TEXT("office") }, W); }); });
 E.AddFreeform(Group, TEXT("Sheriff at 15:00"), TEXT("from the street: in the office, or out on patrol with the car (some afternoons)"), [View, Sheriff]
 { View(15, FVector2D(7993, 2104), FVector(7984, Office + 1.0, 2104), TEXT("NPC-DAY: the sheriff at 15:00"), [Sheriff](FString& W) { return Sheriff({ TEXT("office"), TEXT("patrol") }, W); }); });
 E.AddFreeform(Group, TEXT("Sheriff at 20:00"), TEXT("from the road by his house (the second on the east side): home, his lamp lit; the substation dark, no car"), [View, Sheriff]
 { View(20, FVector2D(8013, 2157), FVector(8028.25, 9.3, 2156.5), TEXT("NPC-DAY: the sheriff at 20:00"), [Sheriff](FString& W) { return Sheriff({ TEXT("home") }, W); }); });
 E.AddFreeform(Group, TEXT("Sheriff at 02:00"), TEXT("by his house: asleep, the lamp out; or out on the county's round the night it comes"), [View, Sheriff]
 { View(2, FVector2D(8013, 2157), FVector(8028.25, 9.3, 2156.5), TEXT("NPC-DAY: the sheriff at 02:00"), [Sheriff](FString& W) { return Sheriff({ TEXT("home"), TEXT("patrol") }, W); }); });
 // The diner: the cook in the kitchen through the hatch at 07, 12 and 19 (and the sheriff at lunch).
 const auto Cook = [](FString& Why)
 {
  const FString Spot = S47TownDay::SpotNow(TEXT("waitress"));
  Why = TEXT("cook ") + Spot;
  return S47TownDay::Active() && S47TownDay::PlaceOf(Spot) == TEXT("diner");
 };
 for (const double Hour : { 7., 12., 19. })
 {
  const FString Label = FString::Printf(TEXT("Diner at %02d:00"), int32(Hour));
  E.AddFreeform(Group, Label, TEXT("inside the door, looking at the kitchen's hatch: the cook at the range or the hatch"), [View, Cook, Hour, Label]
  { View(Hour, FVector2D(7979.2, 2001.4), FVector(7973.8, Diner + 1.5, 1997.4), TEXT("NPC-DAY: ") + Label, Cook); });
 }
 E.AddFreeform(Group, TEXT("Spots and paths against the walk"), TEXT("every planned spot and every path between them against the diner's and road's colliders (the log has each)"), [this]
 {
  FStage S; S.From = 11; S.To = 11.1; S.Area = TEXT("diner"); S.Where = [] { return FVector2D(7993, 2104); };
  Go(S, [this]
  {
   After(1.0, [this]
   {
    TArray<FString> Lines; const int32 Bad = S47TownDay::MeasureSpots(M->Web, Lines);
    for (const FString& L : Lines) UE_LOG(LogS47TownDayDev, Display, TEXT("S47: %s"), *L);
    Tell(Bad == 0, Lines.Num() ? Lines[0] : FString(TEXT("nothing measured")), 12);
   });
  });
 });
 E.AddFreeform(Group, TEXT("Cars: paint"), TEXT("SHERIFF-CAR: every car part in the town's own materials, none left grey (the log has each)"), [this]
 {
  FStage S; S.From = 11; S.To = 11.1; S.Area = TEXT("diner"); S.Where = [] { return FVector2D(7993, 2104); };
  Go(S, [this]
  {
   AimAt(FVector(7987.7, Office, 2106.5));
   After(2.5, [this]
   {
    TArray<FString> Lines; const int32 Bad = S47CarPaint::Measure(M->Web, Lines);
    for (const FString& L : Lines) UE_LOG(LogS47TownDayDev, Display, TEXT("S47: %s"), *L);
    Tell(Bad == 0, Lines.Num() ? Lines[0] : FString(TEXT("nothing measured")), 12);
   });
  });
 });
}

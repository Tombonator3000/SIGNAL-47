#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47Fatigue.h"
#include "S47Flashlight.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Openings.h"
#include "S47Outfit.h"
#include "S47World.h"

// F10, M7 (Tom, 9 November): the loop money, motivation, upgrades, going out. What a card pays now and for what, the week's sheet
// as it stands, what the stores list has and what it waits for, the truck as stock and with its upgrades (top speed, the pull from a
// stand, the tank and the parcel, in numbers from the same rules the truck drives by), gear giving out, and the direction finder.
// Each line turns the game's own rules on (TestLiveRules), as a real game has them. -S47DevSweep=/tmp/dir
// -S47DevSweepGroup="Economy (M7)" runs the group in turn, a picture after each line.
void FS47DevFreeform::AddEconomy(FS47DevEvents& E)
{
	static const TCHAR* const Group = TEXT("Economy (M7)");
	E.AddFreeform(Group, TEXT("Money: this week's sheet so far"), TEXT("the budget, the next review and what it will ask, the array's fault tariff an antenna out, the pump's price, side money left, cards tonight"), [this]
	{
		FS47FreeformSession* F = Session();
		if (!F || !F->Field) { Tell(false, TEXT("Freeform only")); return; }
		F->TestLiveRules();
		const int32 Week = (F->Day() - 1) / 7 + 1;
		const int32 Halves = FS47FreeformSession::UpkeepHalves(Week);
		Tell(true, FString::Printf(TEXT("Day %d, week %d. Budget $%d, carried $%d. Next review %s. Fault tariff $%d.%02d an antenna out (%d in service: $%d). Pump $%.2f a unit. Side money left this week $%d of $%d. Cards filed tonight %d. Repairs due $%d."),
			F->Day(), Week, F->Budget(), F->Arrears(), *F->NextReviewText(), Halves / 2, Halves % 2 * 50, F->AvailableDishCount(), F->ReviewPowerNow(), 1.0 / F->Field->FuelPerDollar(),
			F->SideLeftThisWeek(), int32(FS47FreeformSession::SideCap), F->CardsTonight(), F->RepairsDue()), 14);
	});
	E.AddFreeform(Group, TEXT("Money: what a card pays, good and sloppy"), TEXT("the base by age and the bonus for good work: on time, a clean recording, an anomaly controlled"), [this]
	{
		FS47FreeformSession* F = Session();
		if (!F) { Tell(false, TEXT("Freeform only")); return; }
		using S = FS47FreeformSession;
		Tell(true, FString::Printf(TEXT("Base $%d on its day and the next, $%d two days old, $%d three, $%d after. On top: $%d on time, $%d for a recording that reads (quality %.1f or better), $%d for an anomaly carried through its control. Good card $%d, sloppy on time $%d, sloppy late $%d (was $20 flat)."),
			S::PayAfterDays(0), S::PayAfterDays(2), S::PayAfterDays(3), S::PayAfterDays(4), S::OnTimeBonus, S::CleanBonus, S::CleanRecordQuality, S::ControlBonus,
			S::PayAfterDays(0) + S::OnTimeBonus + S::CleanBonus, S::PayAfterDays(0) + S::OnTimeBonus, S::PayAfterDays(3)), 14);
	});
	// D45-20261110: in the bands OPS SUPPLIES shows them (can buy now, coming, in hand), each with its kind
	E.AddFreeform(Group, TEXT("Stores list: can buy now, coming, in hand"), TEXT("each offer in its band on OPS SUPPLIES, with its kind, price and week"), [this]
	{
		FS47FreeformSession* F = Session();
		if (!F) { Tell(false, TEXT("Freeform only")); return; }
		TArray<FString> CanBuy, Coming, Held;
		for (const FS47KitOffer& O : FS47Openings::Defs())
		{
			const FString Waits = F->KitWaits(O.Id);
			const FString Line = FString::Printf(TEXT("%s %s $%d wk%d"), FS47Openings::Kind(O.Id), O.Id, F->KitPrice(O.Id), O.Week);
			(F->HasKit(O.Id) ? Held : Waits.IsEmpty() ? CanBuy : Coming).Add(Line);
		}
		const FString Said = FString::Printf(TEXT("Day %d. CAN BUY NOW: %s. COMING: %s. IN HAND: %s."), F->Day(), CanBuy.IsEmpty() ? TEXT("none") : *FString::Join(CanBuy, TEXT("; ")),
			Coming.IsEmpty() ? TEXT("none") : *FString::Join(Coming, TEXT("; ")), Held.IsEmpty() ? TEXT("none") : *FString::Join(Held, TEXT("; ")));
		UE_LOG(LogTemp, Display, TEXT("S47: F10 economy: stores: %s"), *Said);
		Tell(true, Said, 16);
	});
	E.AddFreeform(Group, TEXT("Truck: as it is now, against stock"), TEXT("top speed and the pull from a stand on the blacktop and on dirt, the tank and range, the parcel"), [this]
	{
		FS47FreeformSession* F = Session();
		if (!F) { Tell(false, TEXT("Freeform only")); return; }
		const FString Said = FS47Outfit::Compare(FS47Outfit::TuneFor(F));
		UE_LOG(LogTemp, Display, TEXT("S47: F10 economy: truck now: %s"), *Said);
		Tell(true, Said, 14);
	});
	E.AddFreeform(Group, TEXT("Truck: fit all four upgrades"), TEXT("tires, bed rack, auxiliary tank and engine rebuild, as if bought; drive it and see"), [this]
	{
		FS47FreeformSession* F = Session();
		if (!F) { Tell(false, TEXT("Freeform only")); return; }
		for (const TCHAR* Id : { TEXT("truck_tires"), TEXT("truck_rack"), TEXT("truck_tank"), TEXT("truck_engine") }) F->TestGiveKit(Id);
		const FString Said = FS47Outfit::Compare(FS47Outfit::TuneFor(F));
		UE_LOG(LogTemp, Display, TEXT("S47: F10 economy: truck upgraded: %s"), *Said);
		Tell(true, TEXT("Fitted. ") + Said, 14);
	});
	E.AddFreeform(Group, TEXT("Truck: back to stock"), TEXT("the four truck upgrades taken off again"), [this]
	{
		FS47FreeformSession* F = Session();
		if (!F) { Tell(false, TEXT("Freeform only")); return; }
		for (const TCHAR* Id : { TEXT("truck_tires"), TEXT("truck_rack"), TEXT("truck_tank"), TEXT("truck_engine") }) F->TestTakeKit(Id);
		Tell(true, TEXT("Stock again. ") + FS47Outfit::Compare(FS47Outfit::TuneFor(F)), 10);
	});
	// D45-20261110 (D45-29): the alien bobblehead on the dash: where it sits, how its head stands, and a shove to see it wobble
	// (from the driver's seat it is on the pad to the right of the compass). Without the imported meshes: says so.
	E.AddFreeform(Group, TEXT("Truck: the dashboard bobblehead"), TEXT("where it sits and how its head stands, and a shove to its head; sit in the truck to see it wobble"), [this]
	{
		FS47Truck* T = M->World ? M->World->TruckHere() : nullptr;
		if (!T && M->World) T = M->World->SaroTruck();
		FS47Bobblehead* B = T ? T->Bobblehead() : nullptr;
		if (!B) { Tell(false, T ? TEXT("No bobblehead on this truck: import it (Tools/alienbubblehead/import_assets.sh), Jeep cab only") : TEXT("No truck here"), 10); return; }
		B->Spring.Kick(2.5, 1.5);
		TArray<FString> Lines;
		const bool bOk = B->Audit(Lines, false, true);
		UE_LOG(LogTemp, Display, TEXT("S47: F10 truck bobblehead: %s %s"), *B->Describe(), *FString::Join(Lines, TEXT("; ")));
		Tell(true, (bOk ? FString() : FString(TEXT("CHECK FAILED (see the log). "))) + B->Describe(), 12);
	});
	E.AddFreeform(Group, TEXT("Gear gives out: the flashlight"), TEXT("as if close to something: the lamp's cell cooks, and $4 goes on the next sheet"), [this]
	{
		FS47FreeformSession* F = Session();
		if (!F) { Tell(false, TEXT("Freeform only")); return; }
		F->TestLiveRules();
		if (M->Night && M->Night->Flashlight) { M->Night->Flashlight->SetBattery(0); M->Night->Flashlight->bOn = false; }
		F->BreakGear(TEXT("flashlight"), 4);
		Tell(true, FString::Printf(TEXT("The flashlight died; repairs due $%d. The next review: %s."), F->RepairsDue(), *F->NextReviewText()), 10);
	});
	E.AddFreeform(Group, TEXT("Direction finder: a bearing now"), TEXT("the loop on the cab, given and read at once (in the cab, at night, with something out there)"), [this]
	{
		FS47FreeformSession* F = Session();
		if (!F || !F->Outfit) { Tell(false, TEXT("Freeform only")); return; }
		F->TestGiveKit(TEXT("rdf"));
		F->Outfit->DevRdfNow();
		Tell(true, M->World && M->World->bDriving ? TEXT("The loop is read now: a bearing comes up if something keys the band within 3 km.") : TEXT("Given. It reads only in the cab, at night."), 8);
	});
}

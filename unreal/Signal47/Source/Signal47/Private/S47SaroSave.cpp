#include "S47SaroSave.h"

#include "S47EastWing.h"
#include "S47FarmSite.h"
#include "S47Freeform.h"
#include "S47Glimpses.h"
#include "S47Happenings.h"
#include "S47HighwayUfo.h"
#include "S47IndoorDread.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Office.h"
#include "S47Rhythm.h"
#include "S47Taken.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"

bool S47SaroSave::InStation(const US47Main* M)
{
	if (!M || !M->Web || !M->Player || M->Web->Area() != TEXT("saro") || (M->World && M->World->bDriving)) return false;
	const FVector P = M->Player->Pos;
	// the control room and the annex (as FS47FieldRadio::CbAtHand and FS47IndoorDread::InSaroBuilding have them), and the photo lab
	for (const TCHAR* Zone : { TEXT("room.bounds"), TEXT("annex.zone.southDoor"), TEXT("annex.zone.corridor"), TEXT("annex.zone.recordsDoor"), TEXT("annex.zone.records"), TEXT("yard.zone.lab") })
	{
		if (M->Web->Rect(Zone).Contains(P)) return true;
	}
	if (M->Office && M->Office->Contains(P)) return true;
	// the east wing's four rooms: test lab, parts storage, calibration and the garage (a vehicle in it is not "inside": see bDriving)
	if (M->EastWing && M->EastWing->Built())
	{
		for (int32 I = 0; I < 4; ++I)
		{
			if (FS47EastWing::Room(I).IsInside(FVector2D(P.X, P.Z))) return true;
		}
	}
	return false;
}

bool S47SaroSave::OnStationGround(double X, double Z)
{
	// 80 m round the middle of the buildings: the pad (x 6.6 to 13), the east wing (to x 38.6), the visitors' lot (x -22, z to 60);
	// the farm (z 510) and the motel (z 650) are far outside it
	return FMath::Square(X - 5.0) + FMath::Square(Z - 10.0) <= FMath::Square(80.0);
}

FString S47SaroSave::StationOnly()
{
	return TEXT("You can only file the log at the station. Get back inside SARO to save.");
}

FString S47SaroSave::Unsaved(const US47Main* M)
{
	const FS47FreeformSession* F = M && M->IsFreeform() && M->Freeform ? M->Freeform.Get() : nullptr;
	if (!F) return FString();
	// FS47FarmSite::Save: "saved in the middle of a field visit ... the visit is over on Continue"; FS47DoorVisit::Save: "one under way is
	// not saved"; a peek is not saved at all; and FS47Happenings resumes none of its dangers. A save in the middle of any of them is a
	// way out of it, so none is written until it is over.
	const bool bVisit = F->FarmSite && F->FarmSite->VisitWantsGround();
	const bool bAtDoor = F->IndoorDread && (F->IndoorDread->DoorActive() || F->IndoorDread->PeekActive());
	const bool bVoice = F->Happenings && !F->Happenings->Ground().IsEmpty() && FS47Happenings::IsDanger(F->Happenings->Ground());
	return bVisit || bAtDoor || bVoice ? FString(TEXT("Not now. Something is out there.")) : FString();
}

FString S47SaroSave::Danger(const US47Main* M)
{
	const FS47FreeformSession* F = M && M->IsFreeform() && M->Freeform ? M->Freeform.Get() : nullptr;
	if (!F) return FString();
	if (F->Dead()) return TEXT("contact");
	if (F->Taken && F->Taken->Busy()) return TEXT("being taken");
	if (F->FarmSite && F->FarmSite->VisitWantsGround()) return TEXT("a visit on the ground");
	if (F->IndoorDread && (F->IndoorDread->DoorActive() || F->IndoorDread->PeekActive())) return TEXT("something at a door or a window");
	if (M->HighwayUfo && M->HighwayUfo->Busy()) return TEXT("the light over the road");
	if (M->Night && M->Night->Glimpses && !M->Night->Glimpses->Individuals().IsEmpty()) return TEXT("bodies out");
	// whatever holds the night's ground (director.json): a presence or a danger, never a light in the sky or a fault in the rack
	if (M->Rhythm)
	{
		const FS47RhythmNow Now = M->Rhythm->ReadNow();
		const FS47RhythmEvent* E = Now.Busy.IsNone() ? nullptr : M->Rhythm->Data().Find(Now.Busy);
		if (E && E->Level >= ES47RhythmLevel::Presence) return Now.Busy.ToString() + TEXT(" under way");
	}
	return FString();
}

// ---------- the rule ----------

void FS47SaroDeparture::Reset()
{
	bArmed = bHaveLast = bSpaced = bWasInside = bWasDriving = false;
	LastArea.Empty(); FromArea.Empty();
	How = EHow::None; Age = 0; Since = 1e9; From = FVector2D::ZeroVector;
}

FS47SaroDeparture::EHow FS47SaroDeparture::Begin(EHow H, const FIn& In)
{
	bArmed = false;
	if (Since < Spacing) { bSpaced = true; return EHow::None; }
	How = H; Age = 0; From = In.At; FromArea = In.Area;
	return H;
}

FS47SaroDeparture::EHow FS47SaroDeparture::Look(const FIn& In)
{
	const double Step = FMath::IsFinite(In.Dt) ? FMath::Max(0.0, In.Dt) : 0.0;
	Since += Step;
	if (Pending()) Age += Step;
	// behind the black of a trip being set up the player still stands where they got in: that is no new stay inside
	if (In.bBusy) return EHow::None;
	const bool bOnFootIn = In.bInside && !In.bDriving;
	const bool bLeftArea = bHaveLast && LastArea == TEXT("saro") && In.Area != TEXT("saro");
	const bool bLeftBuilding = bHaveLast && bWasInside && !bOnFootIn && !In.bDriving;
	const bool bArrived = bHaveLast && bWasDriving && !In.bDriving && In.bTruckAtSaro;
	LastArea = In.Area; bWasInside = bOnFootIn; bWasDriving = In.bDriving; bHaveLast = true;
	if (bOnFootIn)
	{
		// back inside before it could be written: nobody left
		if (Pending()) Drop();
		bArmed = true;
		return EHow::None;
	}
	// parked at SARO: setting off again, or walking on out of the area, will be a departure
	if (bArrived) { bArmed = true; return EHow::None; }
	// (into a vehicle is Drive's: the trip asks before it begins)
	if (!bArmed || In.bDriving) return EHow::None;
	if (bLeftArea) return Begin(EHow::Area, In);
	if (bLeftBuilding) return Begin(EHow::Door, In);
	return EHow::None;
}

FS47SaroDeparture::EHow FS47SaroDeparture::Drive(const FIn& In)
{
	if (Pending()) return How;
	if (!bArmed) return EHow::None;
	return Begin(EHow::Drive, In);
}

bool FS47SaroDeparture::Expired(const FIn& In) const
{
	if (!Pending()) return false;
	return Age > MaxWait || In.bDriving || In.Area != FromArea || FVector2D::Distance(In.At, From) > MaxDrift;
}

void FS47SaroDeparture::Filed() { How = EHow::None; Age = 0; Since = 0; }
void FS47SaroDeparture::Drop() { How = EHow::None; Age = 0; }

const TCHAR* FS47SaroDeparture::Name(EHow H)
{
	switch (H)
	{
	case EHow::Door: return TEXT("out of the building");
	case EHow::Drive: return TEXT("set off in the truck");
	case EHow::Area: return TEXT("into another area");
	default: return TEXT("none");
	}
}

bool FS47SaroDeparture::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s saro save: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bCond; };
	auto At = [](bool bIn, double X, double Z, double Dt, const TCHAR* Area = TEXT("saro"), bool bDrive = false, bool bTruck = false)
	{
		FIn In; In.bInside = bIn; In.bDriving = bDrive; In.bTruckAtSaro = bTruck; In.Area = Area; In.At = FVector2D(X, Z); In.Dt = Dt; return In;
	};
	Check(!S47SaroSave::StationOnly().IsEmpty() && S47SaroSave::StationOnly().Contains(TEXT("station")), TEXT("away from the station the save menu says where the log is filed"));
	Check(S47SaroSave::OnStationGround(9, 8) && S47SaroSave::OnStationGround(35, 0) && S47SaroSave::OnStationGround(-22, 50) && !S47SaroSave::OnStationGround(84, 510) && !S47SaroSave::OnStationGround(-2.5, 650),
		TEXT("the pad, the east wing and the visitors' lot are the station's ground; the farm and the motel are not"));
	{
		FS47SaroDeparture D;
		Check(D.Look(At(false, 7, 19, .1)) == EHow::None && !D.Pending() && !D.Armed(), TEXT("a game begun out in the yard is not a departure"));
		Check(D.Look(At(true, 0, -1, .1)) == EHow::None && D.Armed(), TEXT("inside, a departure is armed"));
		Check(D.Look(At(true, 2, 1, 5)) == EHow::None && !D.Pending(), TEXT("walking about inside is no departure"));
		Check(D.Look(At(false, -7, 5.6, .1)) == EHow::Door && D.Pending() && !D.Armed(), TEXT("out of the building into the yard: one departure"));
		Check(D.Look(At(false, -9, 6, .1)) == EHow::None && D.Pending(), TEXT("and no second one while it waits outside"));
		D.Filed();
		Check(!D.Pending() && D.Look(At(false, -12, 8, 5)) == EHow::None && D.Drive(At(false, 9, 8, 0)) == EHow::None && D.Look(At(false, 9, 8, 1, TEXT("saro"), true)) == EHow::None
			&& D.Look(At(false, 50, 300, 1, TEXT("road"), true)) == EHow::None, TEXT("filed once: the walk to the truck, the drive off and the next area are the same departure"));
		Check(D.Look(At(false, 9, 8, 1, TEXT("saro"), false, true)) == EHow::None && D.Armed() && !D.Pending(), TEXT("parked at SARO again, the next setting off is armed"));
		Check(D.Look(At(true, -5, 5.6, 2)) == EHow::None && D.Armed(), TEXT("and inside it stays armed"));
		Check(D.Look(At(false, -7, 5.6, 2)) == EHow::None && !D.Pending() && !D.Armed() && D.TakeSpaced() && !D.TakeSpaced(), TEXT("a step out within a minute of the last is not filed again"));
		Check(D.Look(At(true, -5, 5.6, 30)) == EHow::None && D.Look(At(true, -5, 5.6, 31)) == EHow::None && D.Look(At(false, -7, 5.6, .1)) == EHow::Door,
			TEXT("a minute after the last, out again is a new departure"));
		Check(D.Look(At(true, -5, 5.6, .1)) == EHow::None && !D.Pending() && D.Armed(), TEXT("back inside before it was written: nobody left"));
	}
	{
		FS47SaroDeparture D;
		D.Look(At(true, 0, 0, .1)); D.Look(At(false, 8, 8, .1));
		Check(!D.Expired(At(false, 10, 10, 1)) && D.Expired(At(false, 10, 10, 1, TEXT("saro"), true)), TEXT("a waiting departure is let go once the player drives"));
		Check(D.Expired(At(false, 80, 80, 1)) && D.Expired(At(false, 10, 10, 1, TEXT("road"))), TEXT("or once they are far off or in another area"));
		for (int32 I = 0; I < 46; ++I) D.Look(At(false, 9, 9, 1));
		Check(D.Pending() && D.Expired(At(false, 9, 9, 0)), TEXT("or after waiting forty-five seconds of play for a safe moment"));
		D.Drop();
		Check(!D.Pending() && D.Look(At(false, 9, 9, 1)) == EHow::None, TEXT("a departure let go is not tried again outside"));
	}
	{
		FS47SaroDeparture D;
		D.Look(At(true, 33, -2, .1));
		Check(D.Drive(At(true, 33, -2, 0)) == EHow::Drive && D.Pending(), TEXT("setting off from inside (the jeep in the garage) is a departure"));
		Check(D.Drive(At(true, 33, -2, 0)) == EHow::Drive, TEXT("a waiting one gets its last chance when the trip begins"));
		D.Drop();
		Check(D.Look(At(false, 33, -2, .1, TEXT("saro"), true)) == EHow::None && !D.Armed() && D.Drive(At(false, 40, 0, 0)) == EHow::None, TEXT("driving off is one departure, not one more at each area on the way"));
		Check(D.Look(At(true, 33, -2, .1, TEXT("saro"), true)) == EHow::None && !D.Armed(), TEXT("at the wheel inside the garage is not inside"));
	}
	{
		// the normal journey, 8 November: the trip set up from the photo lab left the player standing there 0.4 s, which armed it again
		FS47SaroDeparture D;
		D.Look(At(true, 15, -2, .1));
		Check(D.Drive(At(true, 15, -2, 0)) == EHow::Drive, TEXT("setting off from the lab is a departure"));
		D.Filed();
		FIn Setup = At(true, 15, -2, .2); Setup.bBusy = true;
		D.Look(Setup); D.Look(Setup);
		D.Look(At(false, 9, 8, .1, TEXT("saro"), true)); D.Look(At(false, 7990, 2250, 200, TEXT("diner"), true)); D.Look(At(false, 7990, 2250, 1, TEXT("diner")));
		Check(!D.Armed() && D.Drive(At(false, 7990, 2250, 0, TEXT("diner"))) == EHow::None, TEXT("behind the trip's black nothing is armed: setting off back from town is no departure from SARO"));
	}
	{
		FS47SaroDeparture D;
		D.Look(At(false, 300, 200, .1, TEXT("saro"), true));
		Check(D.Look(At(false, 9, 8, .1, TEXT("saro"), false, true)) == EHow::None && D.Armed(), TEXT("arriving by truck at SARO arms a departure without going in"));
		Check(D.Look(At(false, -20, -60, 20)) == EHow::None && D.Drive(At(false, 9, 8, 0)) == EHow::Drive, TEXT("work out at the array, then setting off again: a departure"));
		D.Filed();
		D.Look(At(false, 300, 200, .1, TEXT("saro"), true)); D.Look(At(false, 9, 8, 70, TEXT("saro"), false, true));
		Check(D.Look(At(false, 120, 260, 1)) == EHow::None && D.Look(At(false, 400, 300, 1, TEXT("road"))) == EHow::Area, TEXT("or walking on from there into another area"));
		D.Drop();
		D.Look(At(false, 400, 300, 1, TEXT("road"), true));
		Check(D.Look(At(false, 410, 300, 1, TEXT("road"), false, false)) == EHow::None && !D.Armed(), TEXT("getting out of the truck anywhere else arms nothing"));
	}
	{
		FS47SaroDeparture D;
		D.Look(At(true, 0, 0, .1));
		Check(D.Look(At(false, 7990, 2250, .1, TEXT("diner"))) == EHow::Area, TEXT("straight from the building into another area: a departure"));
		D.Reset();
		Check(!D.Armed() && !D.Pending() && D.Look(At(false, 7990, 2250, .1, TEXT("diner"))) == EHow::None, TEXT("a Continue starts over: nothing armed"));
		D.Look(At(true, 0, 0, .1));
		Check(D.Look(At(false, 0, 0, .1)) == EHow::Door, TEXT("and its first departure is never spaced out"));
	}
	{
		FS47SaroDeparture D;
		D.Look(At(true, 0, -1, .1)); D.Look(At(false, 7, 19, .1)); D.Filed(); D.Look(At(true, 0, -1, .1));
		D.Arm();
		Check(D.Armed() && D.Look(At(false, 7, 19, .1)) == EHow::Door, TEXT("F10's arm: out into the yard is a departure even within the minute"));
	}
	return bOk;
}

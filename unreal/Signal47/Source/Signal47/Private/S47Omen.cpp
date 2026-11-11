#include "S47Omen.h"

#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/Parse.h"
#include "S47DuskOrb.h"
#include "S47EventPlaces.h"
#include "S47FarmSite.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47GearFlicker.h"
#include "S47Glimpses.h"
#include "S47Happenings.h"
#include "S47HighwayUfo.h"
#include "S47IndoorDread.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47OpenLand.h"
#include "S47RoswellBrain.h"
#include "S47RoswellGroup.h"
#include "S47Story.h"
#include "S47Taken.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include <limits>

DEFINE_LOG_CATEGORY_STATIC(LogS47Omen, Log, All);

namespace S47OmenLocal
{
	// Beyond this from the player a source is taken to be in some other area's terms (a site left on other ground), not near.
	constexpr double FarCut = 1500;
	// Only something at least this dangerous is an onset: the small things on a roof do not hush a whole night.
	constexpr double OnsetSeverity = 0.35;
	// What the player felt and what came after it within this long is one thing to write down.
	constexpr double FeltMerge = 20;
	// No more than this many written down in a night: the notebook is not a log of every blink.
	constexpr int32 NotesPerNight = 3;

	double Horizontal(const FVector& From, const FVector& To)
	{
		// Up counts for half: a light high over a place is felt as over it, not as far off.
		const double Dx = From.X - To.X, Dz = From.Z - To.Z, Dy = (From.Y - To.Y) * 0.5;
		return FMath::Sqrt(Dx * Dx + Dz * Dz + Dy * Dy);
	}
	// A steady number from a few whole numbers (no state, the same every time and on every machine).
	uint32 Mix(uint32 X)
	{
		X ^= X >> 16; X *= 0x7feb352du; X ^= X >> 15; X *= 0x846ca68bu; X ^= X >> 16;
		return X;
	}
	double Unit(uint32 X) { return double(Mix(X) & 0xffffffu) / double(0x1000000); }
	uint32 Key(int32 Seed, int64 Slot, uint32 Salt)
	{
		return Mix(uint32(Seed) * 2654435761u ^ Mix(uint32(Slot & 0xffffffff) + 0x9e3779b9u) ^ Mix(uint32(Slot >> 32) ^ Salt));
	}
	// 0 to 2 pi
	double Around(double Bearing) { const double FullTurn = UE_DOUBLE_PI * 2; return FMath::Fmod(FMath::Fmod(Bearing, FullTurn) + FullTurn, FullTurn); }
	bool Sensible(const FString& Watch)
	{
		return Watch.Len() >= 5 && FChar::IsDigit(Watch[0]) && FChar::IsDigit(Watch[1]) && Watch[2] == TEXT(':') && FChar::IsDigit(Watch[3]) && FChar::IsDigit(Watch[4])
			&& FCString::Atoi(*Watch.Left(2)) < 24 && FCString::Atoi(*Watch.Mid(3, 2)) < 60;
	}
}

double FS47Omen::Falloff(double Distance, double Span)
{
	if (!FMath::IsFinite(Distance) || !FMath::IsFinite(Span) || Span <= 0) return 0;
	const double Core = FMath::Min(4.0, Span * 0.1);
	const double Share = FMath::Clamp((Span - FMath::Max(0.0, Distance)) / FMath::Max(0.001, Span - Core), 0.0, 1.0);
	return Share * Share * (3.0 - 2.0 * Share);
}

double FS47Omen::ReachFor(double Severity)
{
	const double Sv = FMath::Clamp(Severity, 0.0, 1.0);
	return 25.0 + 215.0 * Sv * Sv;
}

double FS47Omen::LevelAt(const FVector& Eye, const TArray<FS47OmenSource>& In)
{
	double Level = 0;
	for (const FS47OmenSource& Src : In)
	{
		if (Src.At.ContainsNaN()) continue;
		const double Sev = FMath::Clamp(Src.Severity, 0.0, 1.0) * (Src.bComing ? ComingShare : 1.0);
		Level = FMath::Max(Level, Sev * Falloff(FVector::Distance(Eye, Src.At), Src.Reach));
	}
	return FMath::Clamp(Level, 0.0, 1.0);
}

double FS47Omen::SenseFrom(const FVector& Where, const TArray<FS47OmenSource>& In, double Widen, FVector* OutFrom, bool* bOutComing)
{
	double Best = 0;
	for (const FS47OmenSource& Src : In)
	{
		if (Src.bResidual || Src.At.ContainsNaN()) continue;
		const double Span = FMath::Max(MinSense, Src.Reach * FMath::Max(1.0, Widen));
		const double Got = FMath::Clamp(Src.Severity, 0.0, 1.0) * Falloff(S47OmenLocal::Horizontal(Where, Src.At), Span);
		if (Got > Best)
		{
			Best = Got;
			if (OutFrom) *OutFrom = Src.At;
			if (bOutComing) *bOutComing = Src.bComing;
		}
	}
	return Best;
}

bool FS47Omen::MissesFor(int32 Seed, int32 Night, const FString& SourceId, ES47OmenChannel Channel)
{
	// F10's made-up presence and being taken are felt by everything: one is a test, the other is everything at once. The week's case
	// too (WEEKLY-CASE-20261109, "weekcase:shed"): the yard's lamp and the lantern flicker together, as its beat is written.
	if (SourceId.IsEmpty() || SourceId == TEXT("dev") || SourceId == TEXT("taken") || SourceId.StartsWith(TEXT("weekcase:")) || Channel == ES47OmenChannel::Count) return false;
	const uint32 Salt = FCrc::StrCrc32(*SourceId) ^ (uint32(Channel) + 1u) * 0x85ebca6bu;
	return S47OmenLocal::Unit(S47OmenLocal::Key(Seed, Night, Salt)) < MissShare;
}

int32 FS47Omen::NightNow() const
{
	return M && M->Freeform ? FS47FreeformSession::NightOf(M->Freeform->Clock()) : 0;
}

bool FS47Omen::Misses(const FString& SourceId, ES47OmenChannel Channel) const
{
	return M && M->Freeform && MissesFor(M->Freeform->Seed(), NightNow(), SourceId, Channel);
}

double FS47Omen::SenseAt(const FVector& Where, double Widen, FVector* OutFrom, bool* bOutComing, ES47OmenChannel Channel) const
{
	if (Channel == ES47OmenChannel::Count) return SenseFrom(Where, Now, Widen, OutFrom, bOutComing);
	TArray<FS47OmenSource> Kept;
	for (const FS47OmenSource& Src : Now) if (!Misses(Src.Id, Channel)) Kept.Add(Src);
	return SenseFrom(Where, Kept, Widen, OutFrom, bOutComing);
}

double FS47Omen::LevelFor(ES47OmenChannel Channel) const
{
	const int32 I = int32(Channel);
	return I >= 0 && I < int32(ES47OmenChannel::Count) ? LevelBy[I] : LevelNow;
}

double FS47Omen::WindFrom(int32 Seed, int32 OnDay)
{
	// out of the south-west most days, now and then round to the west or the south
	const double U = S47OmenLocal::Unit(S47OmenLocal::Key(Seed, OnDay, 0x57a1du));
	return S47OmenLocal::Around(FMath::DegreesToRadians(225.0) + (U - 0.5) * 1.8);
}

const TCHAR* FS47Omen::Way(double Bearing)
{
	static const TCHAR* const Ways[] = { TEXT("north"), TEXT("northeast"), TEXT("east"), TEXT("southeast"), TEXT("south"), TEXT("southwest"), TEXT("west"), TEXT("northwest") };
	if (!FMath::IsFinite(Bearing)) return Ways[0];
	return Ways[(FMath::RoundToInt(S47OmenLocal::Around(Bearing) / (UE_DOUBLE_PI / 4)) % 8 + 8) % 8];
}

double FS47Omen::BearingOf(const FVector2D& From, const FVector2D& To)
{
	// x east, z south: north is -z
	const double East = To.X - From.X, North = -(To.Y - From.Y);
	if (FMath::Abs(East) < 1e-9 && FMath::Abs(North) < 1e-9) return 0;
	return S47OmenLocal::Around(FMath::Atan2(East, North));
}

bool FS47Omen::OrdinaryAt(int32 Seed, double Clock, FS47OmenOrdinary& Out)
{
	Out = FS47OmenOrdinary();
	if (!FMath::IsFinite(Clock) || Clock < 0) return false;
	using namespace S47OmenLocal;
	const int64 Slot = int64(FMath::FloorToDouble(Clock / SlotSeconds));
	const double SlotStart = double(Slot) * SlotSeconds;
	const double Hour = FMath::Fmod(SlotStart, 86400.0) / 3600.0;
	const bool bNight = Hour >= 19.0 || Hour < 6.0;
	// About one half hour in two at night has something ordinary in it, one in eight by day.
	if (Unit(Key(Seed, Slot, 0x0d1aau)) >= (bNight ? 0.5 : 0.12)) return false;
	const double Pick = Unit(Key(Seed, Slot, 0x6b1du));
	ES47Ordinary Kind;
	if (bNight) Kind = Pick < 0.3 ? ES47Ordinary::Gust : Pick < 0.5 ? ES47Ordinary::Contact : Pick < 0.7 ? ES47Ordinary::Skip : Pick < 0.85 ? ES47Ordinary::Traffic : ES47Ordinary::Shy;
	else Kind = Pick < 0.5 ? ES47Ordinary::Contact : Pick < 0.8 ? ES47Ordinary::Skip : ES47Ordinary::Traffic;
	const double Lr = Unit(Key(Seed, Slot, 0x1e9u));
	double Len = 0;
	switch (Kind)
	{
	case ES47Ordinary::Gust: Len = FMath::Lerp(4.0, 10.0, Lr); break;
	case ES47Ordinary::Contact: Len = FMath::Lerp(20.0, 60.0, Lr); break;
	case ES47Ordinary::Skip: Len = FMath::Lerp(10.0, 30.0, Lr); break;
	case ES47Ordinary::Traffic: Len = FMath::Lerp(3.0, 6.0, Lr); break;
	case ES47Ordinary::Shy: Len = FMath::Lerp(8.0, 20.0, Lr); break;
	default: return false;
	}
	Out.Kind = Kind;
	Out.Len = Len;
	Out.From = SlotStart + Unit(Key(Seed, Slot, 0xf20u)) * (SlotSeconds - Len);
	Out.Strength = FMath::Lerp(0.3, 1.0, Unit(Key(Seed, Slot, 0x5e7u)));
	const int32 DayN = FMath::Max(1, int32(FMath::FloorToDouble(Clock / 86400.0)) + 1);
	Out.Bearing = Kind == ES47Ordinary::Gust ? Around(WindFrom(Seed, DayN) + (Unit(Key(Seed, Slot, 0xbea1u)) - 0.5) * 0.9)
		: Unit(Key(Seed, Slot, 0xbea2u)) * UE_DOUBLE_PI * 2;
	return true;
}

double FS47Omen::Ordinary(ES47Ordinary Kind, double* OutBearing) const
{
	if (Kind == ES47Ordinary::None) return 0;
	if (DevKindLeft > 0 && DevKind == Kind)
	{
		if (OutBearing) *OutBearing = Drawn.Bearing;
		return 1.0;
	}
	if (!M || !M->Freeform || Drawn.Kind != Kind) return 0;
	const double Clock = M->Freeform->Clock();
	if (Clock < Drawn.From || Clock >= Drawn.From + Drawn.Len) return 0;
	if (OutBearing) *OutBearing = Drawn.Bearing;
	return Drawn.Strength;
}

void FS47Omen::DevOrdinary(ES47Ordinary Kind, double Seconds)
{
	DevKind = Kind; DevKindLeft = FMath::Max(0.0, Seconds);
	if (M && M->Freeform && Kind == ES47Ordinary::Gust) Drawn.Bearing = WindFrom(M->Freeform->Seed(), M->Freeform->Day());
}

void FS47Omen::Post(const FS47OmenSource& Src, double Seconds)
{
	Posted.RemoveAll([&Src](const FPosted& Was) { return Was.Src.Id == Src.Id; });
	if (Src.Id.IsEmpty() || !FMath::IsFinite(Seconds) || Seconds <= 0 || Src.At.ContainsNaN()) return;
	FPosted Fresh; Fresh.Src = Src; Fresh.Left = Seconds;
	if (Fresh.Src.Reach <= 0) Fresh.Src.Reach = ReachFor(Fresh.Src.Severity);
	Posted.Add(Fresh);
	ReadIn = 0;
}

void FS47Omen::DevPresence(const FVector& Where, double Severity, double Seconds, bool bFirst)
{
	DevAt = Where; DevSeverity = FMath::Clamp(Severity, 0.0, 1.0); DevLeft = FMath::Max(0.0, Seconds); DevAge = 0; bDevComing = bFirst;
	ReadIn = 0;
}

void FS47Omen::Reset()
{
	Now.Reset(); Sensed.Reset(); ReadIn = 0; LevelNow = 0; OnsetAge = 1e9; DevLeft = 0; DevAge = 0; Posted.Reset();
	for (double& Lv : LevelBy) Lv = 0;
	Drawn = FS47OmenOrdinary(); DrawnSlot = -1; DevKind = ES47Ordinary::None; DevKindLeft = 0;
	FeltWhat.Reset(); SinceFelt = 1e9; bFeltReal = bFeltWay = false; bModalWas = false; FeltWatch.Reset();
}

void FS47Omen::Gather(TArray<FS47OmenSource>& Out) const
{
	if (!M || !M->IsFreeform() || !M->Freeform || !M->Player || !M->Web) return;
	const FS47FreeformSession& Fs = *M->Freeform;
	const FVector Eye = S47Three::ToThree(M->Player->EyeUE());
	auto Add = [&Out, &Eye](const FString& Name, const FVector& Where, double Sev, double Span, bool bFirst, bool bLeft)
	{
		if (Where.ContainsNaN() || FVector::Distance(Where, Eye) > S47OmenLocal::FarCut || Sev <= 0) return;
		FS47OmenSource Src; Src.Id = Name; Src.At = Where; Src.Severity = Sev; Src.Reach = Span; Src.bComing = bFirst; Src.bResidual = bLeft;
		Out.Add(Src);
	};
	// The Director's event, when it is on the ground the player is on.
	if (Fs.Director)
	{
		const FS47FreeformDirector& Dr = *Fs.Director;
		const FString& Ev = Dr.ActiveEvent();
		if (!Ev.IsEmpty() && Dr.AnchorHere())
		{
			const FVector EvAt = Dr.EventAnchor();
			const FString EvId = TEXT("director:") + Ev;
			const int32 EvStage = Dr.EventStage();
			// the figures: six seconds of nothing at the anchor, then a body (S47FreeformDirector Stage 0 to 1)
			if (Ev == TEXT("service_presence")) Add(EvId, EvAt + FVector(0, 1, 0), 0.9, 70, EvStage == 0, false);
			else if (Ev == TEXT("observer") || Ev == TEXT("feeder_presence") || Ev == TEXT("array_presence")) Add(EvId, EvAt + FVector(0, 1, 0), 0.65, 55, EvStage == 0, false);
			// the craft low over the station: nearest at thirty seconds
			else if (Ev == TEXT("close_pass")) Add(EvId, Dr.ClosePassNow(), 1.0, 260, Dr.EventAge() < 18, false);
			else if (Ev == TEXT("farm_lights") || Ev == TEXT("town_lights") || Ev == TEXT("response_lights")) Add(EvId, EvAt + FVector(0, 20, 0), 0.6, 160, false, false);
			else if (Ev == TEXT("flyover")) Add(EvId, EvAt + FVector(0, 40, 0), 0.4, 200, false, false);
			else if (Ev == TEXT("dusk_orb"))
			{
				const FS47DuskOrb& Orbs = Dr.DuskOrbState();
				for (int32 I = 0; I < Orbs.VisitLights() && I < FS47DuskOrb::Count; ++I)
					if (FS47DuskOrb::Presence(Dr.EventAge(), I, Orbs.VisitSeed(), Orbs.VisitSeconds()) > 0.5)
						Add(FString::Printf(TEXT("orb:%d"), I), Dr.OrbAt(I), 0.45, 45, false, false);
			}
			// something on the roof: a body, but a small thing
			else if (Ev == TEXT("roof_steps")) Add(EvId, EvAt, 0.25, 20, false, false);
		}
	}
	// Being taken: the steps over the roof first, then the light; in it, everything at once.
	if (Fs.Taken && Fs.Taken->Busy())
	{
		const FS47Taken::EStage TakenStage = Fs.Taken->Stage();
		const TOptional<FVector> TakenAt = Fs.Taken->RadioAt();
		if (TakenAt.IsSet()) Add(TEXT("taken"), TakenAt.GetValue(), 1.0, 140, TakenStage == FS47Taken::EStage::Dread, false);
		else Add(TEXT("taken"), Eye + FVector(0, 6, 0), 1.0, 140, false, false);
	}
	// A visit at marks, or out on open ground: the quiet before it, its warning, then the body.
	if (Fs.FarmSite)
	{
		const FS47FarmSite& Farm = *Fs.FarmSite;
		const double Sev = Farm.VisitKind() == 2 ? 0.95 : 0.85;
		if (Farm.VisitWantsGround())
		{
			const TOptional<FVector> Body = Farm.VisitBodyEye();
			const TOptional<FVector> WarnAt = Farm.VisitWarningAt();
			const FVector Where = Body.IsSet() ? Body.GetValue() : WarnAt.IsSet() ? WarnAt.GetValue() : Farm.Centre() + FVector(0, 1, 0);
			Add(TEXT("visit"), Where, Body.IsSet() && Farm.VisitHuntClock() > 0 ? 1.0 : Sev, 70, !Body.IsSet(), false);
		}
		else if (Farm.VisitHushed()) Add(TEXT("visit"), Farm.Centre() + FVector(0, 1, 0), Sev, 70, true, false);
		// marks still standing, with nothing at them: the ground itself, faintly, close by
		else if (Farm.Armed()) Add(TEXT("marks"), Farm.Centre() + FVector(0, 0.5, 0), 0.12, 16, false, true);
	}
	// Indoors: something tries the door; a head at the window.
	if (Fs.IndoorDread)
	{
		const TOptional<FVector> PeekAt = Fs.IndoorDread->PeekEye();
		if (PeekAt.IsSet()) Add(TEXT("peek"), PeekAt.GetValue(), 0.45, 22, false, false);
		FS47DoorVisit& Dv = Fs.IndoorDread->Door();
		if (Dv.Active())
		{
			const TOptional<FVector> Body = Dv.BodyEye();
			Add(TEXT("door"), Body.IsSet() ? Body.GetValue() : Dv.DoorMiddle(), 0.6, 24, Dv.StageNow() == FS47DoorVisit::EStage::Trying, false);
		}
	}
	// Bodies out, whoever sent them: the Director's figures, a visit's, the developer's.
	if (M->Night && M->Night->Glimpses)
	{
		for (const FS47RoswellGroup::FView& View : M->Night->Glimpses->Individuals())
		{
			const bool bHunt = View.State == FS47RoswellBrain::EState::Hunt || View.State == FS47RoswellBrain::EState::Contact || View.State == FS47RoswellBrain::EState::Search;
			Add(FString::Printf(TEXT("body:%d"), View.Id), View.Eye, bHunt ? 1.0 : 0.6, 60, false, false);
		}
	}
	// The light over the highway, on the road's own ground.
	if (M->HighwayUfo && (M->Web->Area() == TEXT("road") || M->Web->Area() == TEXT("diner")))
	{
		const FString UfoStage = M->HighwayUfo->StageName();
		if (UfoStage != TEXT("idle") && UfoStage != TEXT("done")) Add(TEXT("highway"), M->HighwayUfo->UfoPosition(), 1.0, 260, UfoStage == TEXT("approach"), false);
	}
	// The night's own events (S47Happenings), read only: the ones with something real out there.
	if (Fs.Happenings)
	{
		const FS47Happenings& Haps = *Fs.Happenings;
		const FString& OnGround = Haps.Ground();
		FVector HapAt = FVector::ZeroVector;
		if (!OnGround.IsEmpty() && Haps.LiveAt(HapAt))
		{
			double Sev = 0, Span = 0;
			if (OnGround == TEXT("car_dies")) { Sev = 1.0; Span = 220; }
			else if (OnGround == TEXT("landing")) { Sev = 0.85; Span = 320; }
			else if (OnGround == TEXT("car_light")) { Sev = 0.9; Span = 90; }
			else if (OnGround == TEXT("hitchhiker")) { Sev = 0.7; Span = 45; }
			else if (OnGround == TEXT("horizon_figure")) { Sev = 0.5; Span = 200; }
			if (Sev > 0) Add(TEXT("night:") + OnGround, HapAt, Sev, Span, false, false);
		}
		FVector RingAt = FVector::ZeroVector;
		if (Haps.SiteHere(TEXT("landing"), RingAt)) Add(TEXT("ring"), RingAt + FVector(0, 0.5, 0), 0.15, 26, false, true);
	}
	// What other systems posted (their own signs, through the one owner).
	for (const FPosted& Each : Posted) Add(Each.Src.Id, Each.Src.At, Each.Src.Severity, Each.Src.Reach, Each.Src.bComing, Each.Src.bResidual);
	// F10's.
	if (DevLeft > 0) Add(TEXT("dev"), DevAt, DevSeverity, ReachFor(DevSeverity), bDevComing && DevAge < 6, false);
}

void FS47Omen::StepOrdinary()
{
	if (!M || !M->Freeform) return;
	const double Clock = M->Freeform->Clock();
	const int64 Slot = int64(FMath::FloorToDouble(FMath::Max(0.0, Clock) / SlotSeconds));
	if (Slot == DrawnSlot) return;
	DrawnSlot = Slot;
	FS47OmenOrdinary Next;
	if (!OrdinaryAt(M->Freeform->Seed(), Slot * SlotSeconds + 1, Next)) { Drawn = FS47OmenOrdinary(); return; }
	Drawn = Next;
	++OrdinaryCount;
	if (bLog)
	{
		static const TCHAR* const KindNames[] = { TEXT("none"), TEXT("gust"), TEXT("contact"), TEXT("skip"), TEXT("traffic"), TEXT("shy") };
		const double In = Drawn.From - Clock;
		UE_LOG(LogS47Omen, Display, TEXT("S47: omen ordinary %s in %.0f s for %.0f s, strength %.2f, from the %s"), KindNames[FMath::Clamp(int32(Drawn.Kind), 0, 5)], In, Drawn.Len, Drawn.Strength, Way(Drawn.Bearing));
	}
}

void FS47Omen::Felt(ES47OmenChannel Channel, const TCHAR* What, bool bReal, const FVector& From, bool bHasWay)
{
	(void)Channel;
	if (!M || !M->IsFreeform() || !M->Freeform) return;
	++FeltCount; if (bReal) ++FeltRealCount;
	if (bLog)
	{
		UE_LOG(LogS47Omen, Display, TEXT("S47: omen felt %s (%s)"), What ? What : TEXT("something"), bReal ? TEXT("something real") : TEXT("ordinary"));
	}
	// A fresh one: what the watch showed then is what is written down.
	if (SinceFelt > S47OmenLocal::FeltMerge || FeltWhat.IsEmpty())
	{
		FeltWhat.Reset(); bFeltReal = false; bFeltWay = false;
		const FString Watch = M->Story ? M->Story->ClockText(false) : FString();
		FeltWatch = M->Gear.IsValid() ? M->Gear->ClockSays(Watch) : Watch;
	}
	if (What && *What && FeltWhat.Num() < 3) FeltWhat.AddUnique(What);
	bFeltReal |= bReal;
	if (bHasWay && !bFeltWay && !From.ContainsNaN()) { bFeltWay = true; FeltFrom = From; }
	SinceFelt = 0;
}

FString FS47Omen::NoteText(const TArray<FString>& What, const FString& Watch, const FString& Where, const FString& WayWord)
{
	FString Things;
	for (int32 I = 0; I < What.Num(); ++I) Things += (I == 0 ? TEXT("") : I == What.Num() - 1 ? TEXT(" and ") : TEXT(", ")) + What[I];
	if (Things.IsEmpty()) Things = TEXT("something about the night changed");
	const FString Place = Where.IsEmpty() ? FString(TEXT("out on the open land")) : TEXT("near ") + Where;
	FString Out = S47OmenLocal::Sensible(Watch) ? FString::Printf(TEXT("At %s %s, %s."), *Watch.Left(5), *Things, *Place)
		: FString::Printf(TEXT("%s%s, %s. My watch showed %s."), *Things.Left(1).ToUpper(), *Things.Mid(1), *Place, Watch.IsEmpty() ? TEXT("nothing") : *Watch);
	Out += WayWord.IsEmpty() ? FString(TEXT(" Nothing to say which way.")) : FString::Printf(TEXT(" It seemed to be off to the %s."), *WayWord);
	return Out;
}

void FS47Omen::WriteNote()
{
	if (!M || !M->Freeform || !M->Player || !M->Web || FeltWhat.IsEmpty()) return;
	FS47FreeformSession& Fs = *M->Freeform;
	FString Id;
	for (int32 K = 1; K <= S47OmenLocal::NotesPerNight; ++K)
	{
		const FString Try = FString::Printf(TEXT("gearnote_d%d_%d"), NightNow(), K);
		if (!Fs.HasFinding(Try)) { Id = Try; break; }
	}
	if (Id.IsEmpty()) return;
	// Where: the nearest named place, road-local, and which way it seemed (real: toward it, a little off; ordinary: where the
	// gust came from, where the coyote looked). Nothing to say which way unless the night or an animal showed it.
	FString Where, WayWord;
	double Rx = 0, Rz = 0;
	const FString Area = M->Web->Area();
	if (S47OpenLand::RoadOf(Area, M->Player->Pos.X, M->Player->Pos.Z, Rx, Rz))
	{
		const FVector2D Here(Rx, Rz);
		if (Fs.EventPlaces)
			if (const FS47EventPlace* Pl = Fs.EventPlaces->NearestTo(Here, 450, ES47PlaceTrait::None)) Where = Pl->Title;
		double Fx = 0, Fz = 0;
		if (bFeltWay && S47OpenLand::RoadOf(Area, FeltFrom.X, FeltFrom.Z, Fx, Fz))
		{
			const double Off = (S47OmenLocal::Unit(S47OmenLocal::Key(Fs.Seed(), NightNow(), FCrc::StrCrc32(*Id))) - 0.5) * FMath::DegreesToRadians(50.0);
			WayWord = Way(BearingOf(Here, FVector2D(Fx, Fz)) + (bFeltReal ? Off : 0.0));
		}
	}
	Fs.RegisterFinding(Id, NoteText(FeltWhat, FeltWatch, Where, WayWord));
	++NoteCount; ++NotesEver;
	if (NotesEver <= 2 && M->UI) M->UI->Toast(FString::Printf(TEXT("You write down the time: %s."), FeltWatch.IsEmpty() ? TEXT("--:--") : *FeltWatch), 4);
	if (bLog)
	{
		UE_LOG(LogS47Omen, Display, TEXT("S47: omen note %s (%s): %s"), *Id, bFeltReal ? TEXT("something real") : TEXT("ordinary"), *Fs.FindingText(Id));
	}
	FeltWhat.Reset(); SinceFelt = 1e9;
}

void FS47Omen::StepNote()
{
	// The notebook or the map opened soon after the gear acted up, out of the game's own pause: the time goes down.
	const bool bModal = M->ModalOpen() && !M->bPausedByMenu;
	if (bModal && !bModalWas && !FeltWhat.IsEmpty() && SinceFelt < NoteWindow) WriteNote();
	bModalWas = bModal;
}

void FS47Omen::Step(double Dt)
{
	if (!bLogChecked) { bLogChecked = true; bLog = FParse::Param(FCommandLine::Get(), TEXT("S47OmenLog")); }
	if (!M || !M->IsFreeform() || M->Mode != US47Main::EMode::Play || !M->Player)
	{
		if (!Now.IsEmpty() || LevelNow > 0 || !FeltWhat.IsEmpty()) Reset();
		return;
	}
	StepNote();
	if (M->bPausedByMenu || M->ModalOpen() || !FMath::IsFinite(Dt) || Dt <= 0) return;
	if (OnsetAge < 1e8) OnsetAge += Dt;
	if (SinceFelt < 1e8) SinceFelt += Dt;
	if (DevLeft > 0) { DevLeft = FMath::Max(0.0, DevLeft - Dt); DevAge += Dt; }
	if (DevKindLeft > 0) { DevKindLeft = FMath::Max(0.0, DevKindLeft - Dt); if (DevKindLeft <= 0) DevKind = ES47Ordinary::None; }
	for (FPosted& Each : Posted) Each.Left -= Dt;
	Posted.RemoveAll([](const FPosted& Each) { return Each.Left <= 0; });
	StepOrdinary();
	// The night's tally, once a night, for the log of a long run (how many signs, and how many had something real behind them).
	const int32 Nt = NightNow();
	if (Nt != LoggedNight)
	{
		if (bLog && LoggedNight > 0)
		{
			UE_LOG(LogS47Omen, Display, TEXT("S47: omen night %d: onsets %d, ordinary causes drawn %d, signs felt %d (real %d, ordinary %d), notes %d"),
				LoggedNight, OnsetCount, OrdinaryCount, FeltCount, FeltRealCount, FeltCount - FeltRealCount, NoteCount);
		}
		LoggedNight = Nt; OnsetCount = 0; OrdinaryCount = 0; FeltCount = 0; FeltRealCount = 0; NoteCount = 0;
	}
	ReadIn -= Dt;
	if (ReadIn > 0) return;
	ReadIn = 0.25;
	TArray<FS47OmenSource> Fresh;
	Gather(Fresh);
	Now = MoveTemp(Fresh);
	const FVector Eye = S47Three::ToThree(M->Player->EyeUE());
	const double Was = LevelNow;
	LevelNow = LevelAt(Eye, Now);
	for (int32 Ch = 0; Ch < int32(ES47OmenChannel::Count); ++Ch)
	{
		TArray<FS47OmenSource> Kept;
		for (const FS47OmenSource& Src : Now) if (!Misses(Src.Id, ES47OmenChannel(Ch))) Kept.Add(Src);
		LevelBy[Ch] = LevelAt(Eye, Kept);
	}
	// An onset: something real comes within the night's own sense of the player that was not there at the last look.
	TSet<FString> Here;
	for (const FS47OmenSource& Src : Now)
	{
		if (Src.bResidual || Src.Severity < S47OmenLocal::OnsetSeverity) continue;
		const double Span = FMath::Max(MinSense, Src.Reach * SenseWiden);
		if (S47OmenLocal::Horizontal(Eye, Src.At) >= Span) continue;
		Here.Add(Src.Id);
		if (!Sensed.Contains(Src.Id))
		{
			OnsetAge = 0; ++OnsetCount; OnsetAt = Src.At;
			if (bLog)
			{
				UE_LOG(LogS47Omen, Display, TEXT("S47: omen onset %s, severity %.2f, %.0f m, coming %d"), *Src.Id, Src.Severity, FVector::Distance(Eye, Src.At), Src.bComing ? 1 : 0);
			}
		}
	}
	Sensed = MoveTemp(Here);
	if (bLog && int32(Was * 10) != int32(LevelNow * 10))
	{
		UE_LOG(LogS47Omen, Display, TEXT("S47: omen level %.2f from %d sources"), LevelNow, Now.Num());
	}
}

bool FS47Omen::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s omen: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bCond; };
	Check(Falloff(0, 60) == 1 && Falloff(2, 60) == 1 && Falloff(60, 60) == 0 && Falloff(90, 60) == 0, TEXT("full close by, nothing at its reach or beyond"));
	bool bDown = true; double Prev = 1;
	for (double Dist = 0; Dist <= 80; Dist += 0.5) { const double Fell = Falloff(Dist, 60); bDown &= Fell <= Prev + 1e-9; Prev = Fell; }
	Check(bDown && Falloff(30, 60) > 0.3 && Falloff(30, 60) < 0.7, TEXT("it falls smoothly with distance and never rises again"));
	Check(Falloff(10, 0) == 0 && Falloff(TNumericLimits<double>::Max(), 60) == 0 && Falloff(-5, 60) == 1, TEXT("no reach, no feeling; a bad distance is never more than full"));
	Check(ReachFor(0.1) < ReachFor(0.5) && ReachFor(0.5) < ReachFor(1) && ReachFor(1) > 200 && ReachFor(0) >= 25, TEXT("a worse thing carries further"));
	TArray<FS47OmenSource> Empty;
	Check(LevelAt(FVector::ZeroVector, Empty) == 0 && SenseFrom(FVector::ZeroVector, Empty, SenseWiden, nullptr, nullptr) == 0, TEXT("nothing out there, nothing felt"));
	TArray<FS47OmenSource> Two;
	{ FS47OmenSource Src; Src.Id = TEXT("near"); Src.At = FVector(20, 0, 0); Src.Severity = 0.5; Src.Reach = 60; Two.Add(Src); }
	{ FS47OmenSource Src; Src.Id = TEXT("bad"); Src.At = FVector(0, 0, 100); Src.Severity = 1; Src.Reach = 240; Two.Add(Src); }
	const double Lv = LevelAt(FVector::ZeroVector, Two);
	Check(Lv >= 0.5 * Falloff(20, 60) && Lv >= Falloff(100, 240) && Lv <= 1, TEXT("the strongest counts, not the sum"));
	TArray<FS47OmenSource> One;
	{ FS47OmenSource Src; Src.At = FVector(10, 0, 0); Src.Severity = 0.8; Src.Reach = 60; One.Add(Src); }
	const double Full = LevelAt(FVector::ZeroVector, One);
	One[0].bComing = true;
	const double Coming = LevelAt(FVector::ZeroVector, One);
	Check(Coming < Full && Coming > 0, TEXT("a source only coming is felt at the eyes, but less"));
	FVector FromAt = FVector::ZeroVector; bool bFirst = false;
	Check(SenseFrom(FVector::ZeroVector, One, SenseWiden, &FromAt, &bFirst) > Coming && bFirst && FromAt.Equals(FVector(10, 0, 0)), TEXT("the animals and the night feel it fully, and know where it is"));
	One[0].bComing = false; One[0].At = FVector(80, 0, 0);
	Check(LevelAt(FVector::ZeroVector, One) == 0 && SenseFrom(FVector::ZeroVector, One, SenseWiden, nullptr, nullptr) > 0, TEXT("past the lamp's reach the animals still feel it"));
	One[0].bResidual = true;
	Check(SenseFrom(FVector::ZeroVector, One, SenseWiden, nullptr, nullptr) == 0, TEXT("what was left behind is never felt by the animals"));
	TArray<FS47OmenSource> Above;
	{ FS47OmenSource Src; Src.At = FVector(0, 60, 0); Src.Severity = 1; Src.Reach = 120; Above.Add(Src); }
	Check(SenseFrom(FVector::ZeroVector, Above, 1, nullptr, nullptr) > SenseFrom(FVector(60, 0, 0), Above, 1, nullptr, nullptr), TEXT("a light high over a place is felt most under it"));
	{
		FS47Omen Owner(nullptr);
		FS47OmenSource Src; Src.Id = TEXT("night:quiet"); Src.At = FVector(10, 0, 0); Src.Severity = 0.3; Src.bComing = true;
		Owner.Post(Src, 30); Src.Severity = 0.4; Owner.Post(Src, 30);
		Check(Owner.Posted.Num() == 1 && Owner.Posted[0].Src.Severity == 0.4 && Owner.Posted[0].Src.Reach == ReachFor(0.4), TEXT("another system's sign goes through the one owner, the same id replacing itself"));
		Owner.Post(Src, 0);
		Check(Owner.Posted.IsEmpty(), TEXT("and is taken away again"));
	}
	FS47Omen Bare(nullptr); Bare.Step(1); Bare.Reset();
	Check(Bare.Level() == 0 && Bare.Sources().IsEmpty() && Bare.Onsets() == 0 && Bare.LevelFor(ES47OmenChannel::Lamp) == 0 && Bare.Ordinary(ES47Ordinary::Gust) == 0,
		TEXT("without a game nothing is out there, and nothing ordinary either"));

	// ---- the doubt: each reader misses a real source now and then, kept for the night ----
	{
		int32 Missed = 0, Total = 0, AllMissed = 0; bool bSteady = true, bVaries = false;
		static const TCHAR* const Ids[] = { TEXT("director:service_presence"), TEXT("visit"), TEXT("body:1"), TEXT("highway"), TEXT("night:landing"), TEXT("door") };
		for (int32 Nt = 1; Nt <= 60; ++Nt)
			for (const TCHAR* Id : Ids)
			{
				int32 Here = 0;
				for (int32 Ch = 0; Ch < int32(ES47OmenChannel::Count); ++Ch)
				{
					const bool bMiss = MissesFor(470047, Nt, Id, ES47OmenChannel(Ch));
					bSteady &= bMiss == MissesFor(470047, Nt, Id, ES47OmenChannel(Ch));
					bVaries |= bMiss != MissesFor(470047, Nt + 1, Id, ES47OmenChannel(Ch));
					Missed += bMiss ? 1 : 0; Here += bMiss ? 1 : 0; ++Total;
				}
				AllMissed += Here == int32(ES47OmenChannel::Count) ? 1 : 0;
			}
		const double Share = double(Missed) / FMath::Max(1, Total);
		Check(Share > 0.13 && Share < 0.27 && bSteady && bVaries, TEXT("each reader misses about one real source in five, the same way all night, another way the next"));
		Check(AllMissed <= 2, TEXT("a real source almost always shows on something"));
		Check(!MissesFor(470047, 3, TEXT("dev"), ES47OmenChannel::Lamp) && !MissesFor(470047, 3, TEXT("taken"), ES47OmenChannel::Hush) && !MissesFor(1, 1, TEXT("visit"), ES47OmenChannel::Count),
			TEXT("F10's presence and being taken are felt by everything"));
	}
	// ---- the ordinary causes: drawn from the seed, as often as the real ones, more at night ----
	{
		int32 NightCount = 0, DayCount = 0, Gusts = 0, KindCount = 0; bool bSame = true, bInSlot = true, bShort = true;
		TSet<int32> Seen;
		for (int64 Slot = 0; Slot < 48 * 30; ++Slot)
		{
			FS47OmenOrdinary A, B;
			const double At = Slot * SlotSeconds + 7;
			const bool bAny = OrdinaryAt(470047, At, A);
			bSame &= bAny == OrdinaryAt(470047, At, B) && A.Kind == B.Kind && A.From == B.From;
			if (!bAny) continue;
			const double Hour = FMath::Fmod(Slot * SlotSeconds, 86400.0) / 3600.0;
			(Hour >= 19 || Hour < 6 ? NightCount : DayCount)++;
			bInSlot &= A.From >= Slot * SlotSeconds && A.From + A.Len <= (Slot + 1) * SlotSeconds + 1e-6 && A.Strength >= 0.3 && A.Strength <= 1;
			bShort &= A.Len > 0 && A.Len <= 60;
			Gusts += A.Kind == ES47Ordinary::Gust ? 1 : 0;
			Seen.Add(int32(A.Kind));
		}
		KindCount = Seen.Num();
		// thirty days: 22 night slots a day, about half of them; 26 day slots, an eighth
		const double PerNight = NightCount / 30.0, PerDay = DayCount / 30.0;
		Check(bSame && bInSlot && bShort, TEXT("an ordinary cause is drawn the same way every time, inside its half hour, short and of some strength"));
		Check(PerNight > 7 && PerNight < 15 && PerDay > 1 && PerDay < 6 && KindCount == 5 && Gusts > 30, TEXT("about ten ordinary causes a night, a few by day, of every kind"));
		FS47OmenOrdinary Nothing;
		Check(!OrdinaryAt(470047, -5, Nothing) && !OrdinaryAt(470047, std::numeric_limits<double>::quiet_NaN(), Nothing), TEXT("no clock, nothing ordinary"));
		// the gusts come out of the day's wind, give or take
		int32 FromWind = 0, GustN = 0;
		for (int64 Slot = 0; Slot < 48 * 10; ++Slot)
		{
			FS47OmenOrdinary A;
			if (!OrdinaryAt(91, Slot * SlotSeconds + 3, A) || A.Kind != ES47Ordinary::Gust) continue;
			++GustN;
			const double Wind = WindFrom(91, int32(Slot / 48) + 1);
			FromWind += FMath::Abs(FMath::UnwindRadians(A.Bearing - Wind)) < 0.5 ? 1 : 0;
		}
		Check(GustN > 0 && FromWind == GustN, TEXT("a gust comes from about where the day's wind is from"));
	}
	Check(FCString::Strcmp(Way(0), TEXT("north")) == 0 && FCString::Strcmp(Way(UE_DOUBLE_PI / 2), TEXT("east")) == 0 && FCString::Strcmp(Way(-UE_DOUBLE_PI / 4), TEXT("northwest")) == 0
		&& FCString::Strcmp(Way(BearingOf(FVector2D(0, 0), FVector2D(10, 10))), TEXT("southeast")) == 0 && FCString::Strcmp(Way(BearingOf(FVector2D(0, 0), FVector2D(0, -10))), TEXT("north")) == 0,
		TEXT("the compass: x east, z south"));
	{
		const FString Note = NoteText({ TEXT("the lamp cut out"), TEXT("the walkie went to hiss") }, TEXT("23:14"), TEXT("the church"), TEXT("northeast"));
		Check(Note.StartsWith(TEXT("At 23:14 the lamp cut out and the walkie went to hiss, near the church.")) && Note.Contains(TEXT("northeast")), TEXT("the note says when, what, where and which way"));
		const FString Bare2 = NoteText({ TEXT("the crickets stopped all at once") }, TEXT("88:88"), FString(), FString());
		Check(Bare2.StartsWith(TEXT("The crickets stopped all at once, out on the open land.")) && Bare2.Contains(TEXT("My watch showed 88:88")) && Bare2.Contains(TEXT("Nothing to say which way")),
			TEXT("a watch gone wrong is written down as it showed, and no way is guessed at"));
	}
	return bOk;
}

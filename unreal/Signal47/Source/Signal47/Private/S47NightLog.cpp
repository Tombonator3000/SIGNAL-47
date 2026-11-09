#include "S47NightLog.h"
#include "S47Audio.h"
#include "S47Docs.h"
#include "S47EventPlaces.h"
#include "S47FieldLife.h"
#include "S47Flashlight.h"
#include "S47Freeform.h"
#include "S47FreeformTerminal.h"
#include "S47FreeformWorld.h"
#include "S47GameStyle.h"
#include "S47GearFlicker.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Observing.h"
#include "S47Openings.h"
#include "S47Predecessor.h"
#include "S47RadioTips.h"
#include "S47Story.h"
#include "S47Taken.h"
#include "S47UI.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47NightLog, Log, All);

namespace S47NightLogLocal
{
	FString ClockOf(double Clock)
	{
		const double D = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
		return FString::Printf(TEXT("%02d:%02d"), int32(D / 3600) % 24, int32(FMath::Fmod(D, 3600.0) / 60));
	}
	FString Span(double Seconds)
	{
		const int32 Minutes = FMath::Max(1, FMath::RoundToInt(Seconds / 60));
		if (Minutes < 60) return FString::Printf(TEXT("%d min"), Minutes);
		return Minutes % 60 ? FString::Printf(TEXT("%d hr %d min"), Minutes / 60, Minutes % 60) : FString::Printf(TEXT("%d hr"), Minutes / 60);
	}
	// What is not for this sheet: records about the station's own business, Ward's notes, the shift's paperwork, and what being taken
	// writes (the sheet has the time instead).
	bool Skipped(const FString& Key)
	{
		for (const TCHAR* Not : { TEXT("taken-"), TEXT("recovery-"), TEXT("ward-"), TEXT("round-d"), TEXT("assessment"), TEXT("shiftlog"), TEXT("dev_") })
			if (Key.StartsWith(Not)) return true;
		// CASES-OPS-20261109: a case's own records ("case1-test2", its telexes) are not seen or heard; they go under WORK (Watch)
		if (FS47FreeformCases::IsCaseRecord(Key)) return true;
		return Key.EndsWith(TEXT("_needle"));
	}
	// The instruments' own findings go under the night's work, not under what was seen.
	bool Instrument(const FString& Key)
	{
		for (const TCHAR* Kind : { TEXT("extra_pulse"), TEXT("feeder_fault"), TEXT("wrong_source"), TEXT("time_disagreement"), TEXT("duplicate_print"), TEXT("unsigned_telex"), TEXT("bearing_d"), TEXT("array_burst") })
			if (Key.StartsWith(Kind)) return true;
		return false;
	}
	const TCHAR* const DocPrefix = TEXT("nightlog-");
}

// ---------- the words ----------

bool FS47NightLog::CaseLine(const FString& Text)
{
	if (Text.Len() > 6 && Text.StartsWith(TEXT("Case "), ESearchCase::CaseSensitive) && FChar::IsDigit(Text[5])) return true;
	// an older sheet's lines from the field records the case file used to write
	for (const TCHAR* Old : { TEXT("CASE / "), TEXT("CASE CONCLUDED / "), TEXT("ASSESSMENT FILED / ") })
		if (Text.StartsWith(Old, ESearchCase::CaseSensitive)) return true;
	return false;
}

FString FS47NightLog::Lead(const FString& Record, FString& OutAt)
{
	OutAt.Empty();
	FString Words = Record;
	Words.TrimStartAndEndInline();
	// "At 02:14 lights came down ..." / "At 02:14, ..." : the time goes in the margin.
	if (Words.Len() > 9 && Words.StartsWith(TEXT("At ")) && FChar::IsDigit(Words[3]) && FChar::IsDigit(Words[4]) && Words[5] == TEXT(':') && FChar::IsDigit(Words[6]) && FChar::IsDigit(Words[7]))
	{
		OutAt = Words.Mid(3, 5);
		Words = Words.Mid(8);
		Words.RemoveFromStart(TEXT(","));
		Words.TrimStartInline();
		if (Words.Len()) Words[0] = FChar::ToUpper(Words[0]);
	}
	int32 Stop = INDEX_NONE;
	for (int32 I = 0; I + 1 < Words.Len(); ++I) if (Words[I] == TEXT('.') && Words[I + 1] == TEXT(' ')) { Stop = I; break; }
	if (Stop != INDEX_NONE) Words = Words.Left(Stop + 1);
	if (Words.Len() > 110)
	{
		const int32 Space = Words.Left(108).Find(TEXT(" "), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		Words = Words.Left(Space > 40 ? Space : 108) + TEXT("...");
	}
	else if (Words.Len() && Words[Words.Len() - 1] != TEXT('.') && Words[Words.Len() - 1] != TEXT('"') && Words[Words.Len() - 1] != TEXT('?')) Words += TEXT(".");
	return Words;
}

FString FS47NightLog::Type(const FNight& Nt)
{
	FString Page = FString::Printf(TEXT("SARO / NIGHT LOG\nNIGHT OF %s TO %s, 1986\n19:00 TO 06:00, STATION TIME\n"), *Nt.From, *Nt.To);
	const auto Section = [&Nt, &Page](const TCHAR* Head, EPart Part, int32 Most, const TCHAR* Empty, const FString& More)
	{
		Page += FString(TEXT("\n")) + Head + TEXT("\n");
		int32 Shown = 0, Rest = 0;
		for (const FLine& L : Nt.Lines)
		{
			if (L.Part != Part) continue;
			if (Shown >= Most) { ++Rest; continue; }
			Page += (L.At.IsEmpty() ? FString() : L.At + TEXT("  ")) + L.Text + TEXT("\n");
			++Shown;
		}
		if (Rest > 0) Page += FString::Printf(TEXT("And %d more%s.\n"), Rest, *More);
		if (Shown == 0) Page += FString(Empty) + TEXT("\n");
	};
	Section(TEXT("SEEN AND HEARD"), EPart::Seen, 6, TEXT("Nothing out of the ordinary."), TEXT(" in the field records"));
	Section(TEXT("TIME"), EPart::Time, 4, TEXT("Nothing unaccounted for."), FString());
	Section(TEXT("EQUIPMENT"), EPart::Gear, 5, TEXT("No faults."), FString());
	Page += TEXT("\nWORK\n");
	bool bAnyWork = false;
	for (const FLine& L : Nt.Lines) if (L.Part == EPart::Work) { Page += (L.At.IsEmpty() ? FString() : L.At + TEXT("  ")) + L.Text + TEXT("\n"); bAnyWork = true; }
	for (const FString& Job : Nt.Work) { Page += Job + TEXT("\n"); bAnyWork = true; }
	if (!bAnyWork) Page += TEXT("Nothing outstanding.\n");
	// CHAIN-2-20261109: what the night delivered, the same three lines every morning.
	FString Money = Nt.Side > 0 ? FString::Printf(TEXT("Field money tonight: $%d%s."), Nt.Side, Nt.bSameNight ? TEXT(", the same-night rate in it") : TEXT("")) : FString(TEXT("No field money tonight."));
	if (Nt.SideLeft >= 0) Money += FString::Printf(TEXT(" $%d left of the week's $%d."), Nt.SideLeft, int32(FS47FreeformSession::SideCap));
	Page += TEXT("\nDELIVERED\nMONEY   ") + Money + TEXT("\n");
	Page += TEXT("OPENED  ") + (Nt.Opened.IsEmpty() ? FString(TEXT("Nothing new.")) : TEXT("On the stores list now: ") + FString::Join(Nt.Opened, TEXT("; ")) + TEXT(".")) + TEXT("\n");
	Page += TEXT("WARD    ") + (Nt.Ward.IsEmpty() ? FString(TEXT("Nothing from out on the land tonight.")) : Nt.Ward) + TEXT("\n");
	Page += FString::Printf(TEXT("\nACCOUNT\n$%d at dusk, $%d at 06:00. In $%d, out $%d."), Nt.CashFrom, Nt.CashTo, Nt.In, Nt.Out);
	if (Nt.Carried > 0) Page += FString::Printf(TEXT(" Carried on the weekly sheet: $%d."), Nt.Carried);
	Page += TEXT("\n\nTyped at 06:00 for the shift binder.");
	return Page;
}

FString FS47NightLog::RemarkFor(const FNight& Nt)
{
	bool bLost = false, bPoor = false, bGear = false, bSeen = false;
	for (const FLine& L : Nt.Lines)
	{
		bLost |= L.Part == EPart::Time && L.Text.Contains(TEXT("not accounted for"));
		bGear |= L.Part == EPart::Gear;
		bSeen |= L.Part == EPart::Seen;
	}
	for (const FString& Job : Nt.Work) bPoor |= Job.Contains(TEXT("will not read"));
	if (bLost) return TEXT("There is time on this sheet nobody can account for. Check the station clock against WWVB at dusk, and again when you come in.");
	if (bPoor) return TEXT("A recording that will not read is a night's work lost. Retune before you record, not after.");
	if (bGear) return TEXT("Tag whatever failed and leave it on the bench with the time on it.");
	if (bSeen) return TEXT("This is what I want: what, where and when. Keep it that way.");
	return TEXT("A quiet night is worth a sheet too.");
}

// ---------- a game's life ----------

void FS47NightLog::Begin(const S47J::FObj& Saved)
{
	Logs.Reset(); Lines.Reset();
	Night = 0; From = 0; CashFrom = CashIn = CashOut = 0; Flickers = Drifts = Coughs = Jumps = 0;
	bPrimed = false; bDevLost = false; Known.Reset(); SaidKnown.Reset();
	Side = 0; LastSideLeft = -1; OpenAtDusk.Reset(); bOpenKnown = false; // CHAIN-2-20261109
	if (!Saved.IsValid()) return;
	for (const auto& V : S47J::Arr(Saved, TEXT("sheets")))
	{
		const S47J::FObj* O = nullptr;
		if (!V.IsValid() || !V->TryGetObject(O) || !O) continue;
		FSheet Sh;
		Sh.Night = FMath::Clamp(S47J::Int(*O, TEXT("night")), 0, 100000);
		Sh.Title = S47J::Str(*O, TEXT("title")).Left(80);
		Sh.Page = S47J::Str(*O, TEXT("page")).Left(6000);
		Sh.Remark = S47J::Str(*O, TEXT("remark")).Left(400);
		Sh.Typed = S47J::Num(*O, TEXT("typed"));
		Sh.Sent = S47J::Num(*O, TEXT("sent"), -1);
		Sh.Read = S47J::Num(*O, TEXT("read"), -1);
		for (const auto& Fv : S47J::Arr(*O, TEXT("facts")))
		{
			const TArray<TSharedPtr<FJsonValue>>* Four = nullptr;
			if (Sh.Facts.Num() >= 40 || !Fv.IsValid() || !Fv->TryGetArray(Four) || !Four || Four->Num() < 4) continue;
			FLine L; L.Clock = (*Four)[0]->AsNumber(); L.Part = EPart(FMath::Clamp(int32((*Four)[1]->AsNumber()), 0, 3));
			L.At = (*Four)[2]->AsString().Left(80); L.Text = (*Four)[3]->AsString().Left(300);
			if (FMath::IsFinite(L.Clock) && !L.Text.IsEmpty()) Sh.Facts.Add(L);
		}
		if (Sh.Night > 0 && !Sh.Page.IsEmpty() && !Logs.ContainsByPredicate([&Sh](const FSheet& Had) { return Had.Night == Sh.Night; })) Logs.Add(Sh);
	}
	if (const S47J::FObj Tn = S47J::Obj(Saved, TEXT("tonight")); Tn.IsValid())
	{
		Night = FMath::Clamp(S47J::Int(Tn, TEXT("night")), 0, 100000);
		From = S47J::Num(Tn, TEXT("from"));
		CashFrom = S47J::Int(Tn, TEXT("cash")); CashIn = FMath::Max(0, S47J::Int(Tn, TEXT("in"))); CashOut = FMath::Max(0, S47J::Int(Tn, TEXT("out")));
		Flickers = FMath::Max(0, S47J::Int(Tn, TEXT("flickers"))); Drifts = FMath::Max(0, S47J::Int(Tn, TEXT("drifts")));
		Coughs = FMath::Max(0, S47J::Int(Tn, TEXT("coughs"))); Jumps = FMath::Max(0, S47J::Int(Tn, TEXT("jumps")));
		// CHAIN-2-20261109: optional; an older save has neither
		Side = FMath::Clamp(S47J::Int(Tn, TEXT("side")), 0, 10000);
		if (S47J::Has(Tn, TEXT("open"))) { bOpenKnown = true; for (const FString& Id : S47J::Strs(Tn, TEXT("open"))) if (FS47Openings::Def(Id)) OpenAtDusk.Add(Id); }
		for (const auto& V : S47J::Arr(Tn, TEXT("lines")))
		{
			const S47J::FObj* O = nullptr;
			if (!V.IsValid() || !V->TryGetObject(O) || !O) continue;
			FLine L;
			L.Clock = S47J::Num(*O, TEXT("clock"));
			L.Part = EPart(FMath::Clamp(S47J::Int(*O, TEXT("part")), 0, 3));
			L.At = S47J::Str(*O, TEXT("at")).Left(8);
			L.Text = S47J::Str(*O, TEXT("text")).Left(200);
			if (!L.Text.IsEmpty() && Lines.Num() < 60) Lines.Add(L);
		}
		if (Night <= 0) Lines.Reset();
	}
}

S47J::FObj FS47NightLog::Save() const
{
	auto O = S47J::New();
	if (!Logs.IsEmpty())
	{
		TArray<TSharedPtr<FJsonValue>> List;
		for (const FSheet& Sh : Logs)
		{
			auto E = S47J::New();
			S47J::Set(E, TEXT("night"), Sh.Night); S47J::Set(E, TEXT("title"), Sh.Title); S47J::Set(E, TEXT("page"), Sh.Page); S47J::Set(E, TEXT("remark"), Sh.Remark);
			S47J::Set(E, TEXT("typed"), Sh.Typed); S47J::Set(E, TEXT("sent"), Sh.Sent); S47J::Set(E, TEXT("read"), Sh.Read);
			if (Sh.Facts.Num())
			{
				TArray<TSharedPtr<FJsonValue>> Facts;
				for (const FLine& L : Sh.Facts)
					Facts.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{ MakeShared<FJsonValueNumber>(L.Clock), MakeShared<FJsonValueNumber>(int32(L.Part)), MakeShared<FJsonValueString>(L.At), MakeShared<FJsonValueString>(L.Text) }));
				E->SetArrayField(TEXT("facts"), Facts);
			}
			List.Add(MakeShared<FJsonValueObject>(E));
		}
		O->SetArrayField(TEXT("sheets"), List);
	}
	if (Night > 0)
	{
		auto Tn = S47J::New();
		S47J::Set(Tn, TEXT("night"), Night); S47J::Set(Tn, TEXT("from"), From);
		S47J::Set(Tn, TEXT("cash"), CashFrom); S47J::Set(Tn, TEXT("in"), CashIn); S47J::Set(Tn, TEXT("out"), CashOut);
		S47J::Set(Tn, TEXT("flickers"), Flickers); S47J::Set(Tn, TEXT("drifts"), Drifts); S47J::Set(Tn, TEXT("coughs"), Coughs); S47J::Set(Tn, TEXT("jumps"), Jumps);
		S47J::Set(Tn, TEXT("side"), Side); // CHAIN-2-20261109
		if (bOpenKnown) S47J::Set(Tn, TEXT("open"), OpenAtDusk.Array());
		TArray<TSharedPtr<FJsonValue>> List;
		for (const FLine& L : Lines)
		{
			auto E = S47J::New();
			S47J::Set(E, TEXT("clock"), L.Clock); S47J::Set(E, TEXT("part"), int32(L.Part)); S47J::Set(E, TEXT("at"), L.At); S47J::Set(E, TEXT("text"), L.Text);
			List.Add(MakeShared<FJsonValueObject>(E));
		}
		Tn->SetArrayField(TEXT("lines"), List);
		S47J::Set(O, TEXT("tonight"), Tn);
	}
	return O;
}

int32 FS47NightLog::ReadCount() const
{
	int32 Count = 0;
	for (const FSheet& Sh : Logs) Count += Sh.Read >= 0 ? 1 : 0;
	return Count;
}

FS47NightLog::FSheet* FS47NightLog::Find(int32 Nt) { return Logs.FindByPredicate([Nt](const FSheet& Sh) { return Sh.Night == Nt; }); }

void FS47NightLog::Add(double Clock, EPart Part, const FString& Text, const FString& At)
{
	if (Night <= 0 || Text.IsEmpty() || Lines.Num() >= 60) return;
	// At: the time in the margin; empty for the clock now, "-" for none (the line says its own times).
	FLine L; L.Clock = Clock; L.Part = Part; L.Text = Text; L.At = At == TEXT("-") ? FString() : At.IsEmpty() ? S47NightLogLocal::ClockOf(Clock) : At;
	// A record's own time ("02:14") orders the sheet, not when it was noticed by the log: on this night's clock, after midnight the next day.
	if (L.At.Len() == 5 && L.At[2] == TEXT(':'))
	{
		const int32 Hh = FCString::Atoi(*L.At.Left(2)), Mm = FCString::Atoi(*L.At.Mid(3, 2));
		L.Clock = (Night - 1) * 86400.0 + (Hh < 12 ? Hh + 24 : Hh) * 3600.0 + Mm * 60.0;
	}
	Lines.Add(L);
	UE_LOG(LogS47NightLog, Display, TEXT("S47: night log %d: %s %s"), Night, *L.At, *Text);
}

FString FS47NightLog::Place() const
{
	if (!M || !M->Freeform || !M->Freeform->EventPlaces) return FString(TEXT("out on the land"));
	if (const FS47EventPlace* Pl = M->Freeform->EventPlaces->Nearest(900)) return FString(TEXT("near ")) + Pl->Title;
	return FString(TEXT("out on the land"));
}

void FS47NightLog::Prime()
{
	bPrimed = true;
	auto& Fs = *M->Freeform;
	LastCash = Fs.Budget();
	LastFindings = Fs.FindingCount();
	if (M->Story) for (const FS47Doc& D : M->Story->Docs()) if (D.Id.StartsWith(TEXT("freeform-"))) Known.Add(D.Id);
	for (const FString& Key : Fs.FindingIdsFrom(0)) if (FS47FreeformCases::IsCaseRecord(Key)) Known.Add(TEXT("case:") + Key);
	if (Fs.Tips) { SaidKnown = Fs.Tips->Said(); LastSaid = SaidKnown.Num(); }
	if (M->Gear) { BaseFlickers = M->Gear->Flickers(); BaseDrifts = M->Gear->RadioDrifts(); BaseCoughs = M->Gear->Coughs(); BaseJumps = M->Gear->ClockJumps(); }
	bLampDead = M->Night && M->Night->Flashlight && M->Night->Flashlight->Dead();
	if (Fs.Care) { bStuck = Fs.Care->Stuck(); bFlat = Fs.Care->Flat(); bHot = Fs.Care->Hot(); }
	bPowerOut = Fs.PowerFault();
	bArrayOut = Fs.Field && Fs.Field->HasArrayFault();
	bWasTaken = Fs.Taken && Fs.Taken->Busy();
	// CHAIN-2-20261109
	LastSideLeft = Fs.SideLeftThisWeek();
	if (Night > 0 && !bOpenKnown) { OpenNow(OpenAtDusk); bOpenKnown = true; }
}

void FS47NightLog::OpenNow(TSet<FString>& Out) const
{
	Out.Reset();
	if (!M || !M->Freeform) return;
	for (const FS47KitOffer& O : FS47Openings::Defs()) if (M->Freeform->HasKit(O.Id) || M->Freeform->KitWaits(O.Id).IsEmpty()) Out.Add(O.Id);
}

// CHAIN-2-20261109: how tonight counts toward Dr. Ward's assessment: what came in from out on the land, the kinds of night record she
// counts, and whether a case carried through has something from out there in it (FS47Predecessor::Decide wants one).
FString FS47NightLog::WardLine() const
{
	if (!M || !M->Freeform) return FString();
	const auto& Fs = *M->Freeform;
	int32 Kept = 0;
	for (const FString& Key : Fs.FindingIdsFrom(FMath::Max(1, Night))) Kept += FS47FreeformSession::FieldRecordKey(Key) ? 1 : 0;
	FString Said = Kept > 0 ? FString::Printf(TEXT("%d %s from out on the land tonight."), Kept, Kept == 1 ? TEXT("field record") : TEXT("field records")) : FString(TEXT("Nothing from out on the land tonight."));
	if (Fs.Predecessor && Fs.Predecessor->Result() == FS47Predecessor::EOutcome::None)
	{
		const FS47Predecessor::FTally C = Fs.Predecessor->Count();
		Said += FString::Printf(TEXT(" Kinds of night record on file: %d. A case carried through with something from out there: %s."), C.Land, C.Field >= FS47Predecessor::FieldToStay ? TEXT("on file") : TEXT("none yet, and she needs one"));
	}
	return Said;
}

void FS47NightLog::Open(int32 Nt, double At)
{
	Night = Nt; From = At; CashFrom = LastCash; CashIn = CashOut = 0; Flickers = Drifts = Coughs = Jumps = 0;
	Lines.Reset();
	Side = 0; OpenNow(OpenAtDusk); bOpenKnown = true; // CHAIN-2-20261109
	UE_LOG(LogS47NightLog, Display, TEXT("S47: night log %d open from %s, $%d"), Nt, *S47NightLogLocal::ClockOf(At), CashFrom);
}

// What the night does that the player meets: the field records it leaves, the radio, the gear, the truck, the station.
void FS47NightLog::Watch(double ClockNow, bool& bOutLost)
{
	auto& Fs = *M->Freeform;
	if (Fs.FindingCount() != LastFindings && M->Story)
	{
		LastFindings = Fs.FindingCount();
		for (const FS47Doc& D : M->Story->Docs())
		{
			if (!D.Id.StartsWith(TEXT("freeform-")) || Known.Contains(D.Id)) continue;
			Known.Add(D.Id);
			const FString Key = D.Id.Mid(9);
			if (Key.StartsWith(TEXT("taken-")) || Key.StartsWith(TEXT("recovery-"))) bOutLost = true;
			if (Night <= 0 || S47NightLogLocal::Skipped(Key)) continue;
			FString At;
			const FString Text = Lead(Fs.FindingText(Key), At);
			if (Text.IsEmpty()) continue;
			const EPart Part = Key.StartsWith(TEXT("clock_ahead")) ? EPart::Time : (Key.StartsWith(TEXT("car_dies")) || Key.StartsWith(TEXT("car_light"))) ? EPart::Gear : S47NightLogLocal::Instrument(Key) ? EPart::Work : EPart::Seen;
			Add(ClockNow, Part, Text, At);
		}
		// CASES-OPS-20261109: a case's record kept tonight is one line under WORK, never a field record
		for (const FString& Key : Fs.FindingIdsFrom(FMath::Max(1, Night)))
		{
			if (!FS47FreeformCases::IsCaseRecord(Key) || Known.Contains(TEXT("case:") + Key)) continue;
			Known.Add(TEXT("case:") + Key);
			if (Night <= 0) continue;
			const FString Line = Fs.CaseRecordLine(Key);
			if (!Line.IsEmpty()) Add(ClockNow, EPart::Work, Line);
		}
	}
	if (Fs.Tips && Fs.Tips->Said().Num() != LastSaid)
	{
		LastSaid = Fs.Tips->Said().Num();
		for (const FString& Id : Fs.Tips->Said())
		{
			if (SaidKnown.Contains(Id)) continue;
			SaidKnown.Add(Id);
			if (Night <= 0) continue;
			FString Heard;
			if (const auto* Cb = FS47RadioTips::CbLines().FindByPredicate([&Id](const FS47RadioTips::FLine& L) { return Id == L.Id; })) Heard = FString(TEXT("On the CB: \"")) + Cb->Text + TEXT("\"");
			else if (const auto* Am = FS47RadioTips::AmCalls().FindByPredicate([&Id](const FS47RadioTips::FCall& Call) { return Id == Call.Id; })) Heard = FString(TEXT("A caller on 1420 AM: \"")) + Am->Text + TEXT("\"");
			if (Heard.Len() > 120) { const int32 Space = Heard.Left(116).Find(TEXT(" "), ESearchCase::CaseSensitive, ESearchDir::FromEnd); Heard = Heard.Left(Space > 40 ? Space : 116) + TEXT("...\""); }
			Add(ClockNow, EPart::Seen, Heard);
		}
	}
	if (M->Gear)
	{
		Flickers += FMath::Max(0, M->Gear->Flickers() - BaseFlickers); BaseFlickers = M->Gear->Flickers();
		Drifts += FMath::Max(0, M->Gear->RadioDrifts() - BaseDrifts); BaseDrifts = M->Gear->RadioDrifts();
		Coughs += FMath::Max(0, M->Gear->Coughs() - BaseCoughs); BaseCoughs = M->Gear->Coughs();
		Jumps += FMath::Max(0, M->Gear->ClockJumps() - BaseJumps); BaseJumps = M->Gear->ClockJumps();
	}
	const bool bLamp = M->Night && M->Night->Flashlight && M->Night->Flashlight->Dead();
	if (bLamp && !bLampDead) Add(ClockNow, EPart::Gear, TEXT("Flashlight battery ran flat."));
	bLampDead = bLamp;
	if (Fs.Care)
	{
		if (Fs.Care->Stuck() && !bStuck) Add(ClockNow, EPart::Gear, TEXT("Truck bogged down in sand ") + Place() + TEXT("."));
		if (Fs.Care->Flat() && !bFlat) Add(ClockNow, EPart::Gear, TEXT("Flat tire ") + Place() + TEXT("."));
		if (Fs.Care->Hot() && !bHot) Add(ClockNow, EPart::Gear, TEXT("Radiator boiled dry ") + Place() + TEXT("."));
	}
	if (Fs.PowerFault() && !bPowerOut) Add(ClockNow, EPart::Gear, TEXT("Feeder power to the station failed."));
	bPowerOut = Fs.PowerFault();
	const bool bArray = Fs.Field && Fs.Field->HasArrayFault();
	if (bArray && !bArrayOut) Add(ClockNow, EPart::Gear, TEXT("Array drive fault."));
	bArrayOut = bArray;
}

void FS47NightLog::Step(double Dt)
{
	static_cast<void>(Dt);
	if (!M || !M->Freeform || !M->Freeform->Active()) return;
	auto& Fs = *M->Freeform;
	const double ClockNow = Fs.Clock();
	if (!bPrimed) { Prime(); LastClock = ClockNow; }
	// A night the sheet keeps begins at dusk; the first one too, once the first shift is done.
	const int32 Nt = FS47FreeformSession::NightOf(ClockNow);
	if (Night <= 0 && !Fs.InTutorial() && ClockNow >= DuskOf(Nt) && ClockNow < SixOf(Nt) && !Find(Nt)) Open(Nt, FMath::Max(DuskOf(Nt), FMath::Min(LastClock, ClockNow)));
	const bool bTakenNow = Fs.Taken && Fs.Taken->Busy();
	bool bLost = bDevLost || bTakenNow || bWasTaken;
	Watch(ClockNow, bLost);
	// The clock jumped: a rest or a wait, work that takes time, or time nobody can account for.
	const double Jump = ClockNow - LastClock;
	if (Night > 0 && Jump > 5 * 60)
	{
		const double A = FMath::Max(LastClock, From), B = FMath::Min(ClockNow, SixOf(Night));
		const bool bWasStuck = bStuck, bWasFlat = bFlat, bWasHot = bHot;
		const bool bFixed = Fs.Care && ((bWasStuck && !Fs.Care->Stuck()) || (bWasFlat && !Fs.Care->Flat()) || (bWasHot && !Fs.Care->Hot()));
		if (B > A)
		{
			const FString Between = S47NightLogLocal::ClockOf(A) + TEXT(" to ") + S47NightLogLocal::ClockOf(B);
			if (bLost) Add(A, EPart::Time, Between + TEXT(" not accounted for (") + S47NightLogLocal::Span(B - A) + TEXT(")."), TEXT("-"));
			else if (bFixed && Fs.Budget() < LastCash) Add(A, EPart::Gear, FString::Printf(TEXT("County tow for the truck, %s, $%d."), *S47NightLogLocal::Span(B - A), LastCash - Fs.Budget()));
			else if (bFixed && bWasStuck && !Fs.Care->Stuck()) Add(A, EPart::Gear, TEXT("Dug the truck out of the sand (") + S47NightLogLocal::Span(B - A) + TEXT(")."));
			else if (bFixed && bWasFlat && !Fs.Care->Flat()) Add(A, EPart::Gear, TEXT("Changed the wheel (") + S47NightLogLocal::Span(B - A) + TEXT(")."));
			else if (B - A >= 30 * 60) Add(A, EPart::Time, TEXT("Off watch ") + Between + TEXT("."), TEXT("-"));
		}
	}
	if (Fs.Care) { bStuck = Fs.Care->Stuck(); bFlat = Fs.Care->Flat(); bHot = Fs.Care->Hot(); }
	bDevLost = false;
	bWasTaken = bTakenNow;
	// The money, as it moves tonight.
	const int32 Cash = Fs.Budget();
	if (Night > 0) { if (Cash > LastCash) CashIn += Cash - LastCash; else CashOut += LastCash - Cash; }
	LastCash = Cash;
	// CHAIN-2-20261109: field money is what comes off the week's cap (FS47FreeformSession::EarnSide); the cap filling again is no money.
	const int32 SideLeft = Fs.SideLeftThisWeek();
	if (Night > 0 && LastSideLeft >= 0 && SideLeft < LastSideLeft) Side += LastSideLeft - SideLeft;
	LastSideLeft = SideLeft;
	LastClock = ClockNow;
	if (Night > 0 && ClockNow >= SixOf(Night)) Close(ClockNow);
	Receive(ClockNow);
}

FS47NightLog::FNight FS47NightLog::Gather() const
{
	FNight Got;
	Got.Night = Night;
	auto& Fs = *M->Freeform;
	Got.From = S47Game::DateLine(Fs.DateOfDay(Night));
	Got.To = S47Game::DateLine(Fs.DateOfDay(Night + 1));
	Got.CashFrom = CashFrom; Got.CashTo = Fs.Budget(); Got.In = CashIn; Got.Out = CashOut; Got.Carried = Fs.Arrears();
	Got.Lines = Lines;
	Got.Lines.Sort([](const FLine& A, const FLine& B) { return A.Clock < B.Clock; });
	// The gear's own complaints, counted.
	if (Flickers > 0) Got.Lines.Add({ SixOf(Night), EPart::Gear, FString(), Flickers == 1 ? FString(TEXT("Flashlight cut out once.")) : FString::Printf(TEXT("Flashlight cut out %d times."), Flickers) });
	if (Drifts > 0) Got.Lines.Add({ SixOf(Night), EPart::Gear, FString(), Drifts == 1 ? FString(TEXT("Walkie and truck radio lost the channel once.")) : FString::Printf(TEXT("Walkie and truck radio lost the channel %d times."), Drifts) });
	if (Coughs > 0) Got.Lines.Add({ SixOf(Night), EPart::Gear, FString(), Coughs == 1 ? FString(TEXT("Truck engine missed once.")) : FString::Printf(TEXT("Truck engine missed %d times."), Coughs) });
	if (Jumps > 0) Got.Lines.Add({ SixOf(Night), EPart::Time, FString(), Jumps == 1 ? FString(TEXT("Watch and dash clock jumped once, then came right.")) : FString::Printf(TEXT("Watch and dash clock jumped %d times, then came right."), Jumps) });
	// Tonight's work as the record has it.
	int32 Older = 0;
	for (const auto& J : Fs.Jobs())
	{
		if (J.CaseTest >= 0 || J.bDuplicateRejected) continue;
		if (J.Day < Night) { Older += J.Stage != FS47FreeformSession::EJobStage::Filed ? 1 : 0; continue; }
		if (J.Day != Night) continue;
		const FString Name = FString::Printf(TEXT("%s, %s"), *J.Id, FS47Observing::Runs()[FMath::Clamp(J.Source, 0, FS47Observing::Runs().Num() - 1)].Source);
		if (J.Stage == FS47FreeformSession::EJobStage::Filed)
		{
			if (J.RecordingSoundVersion >= 2 && J.RecordingQuality < FS47FreeformSession::PoorRecordQuality) Got.Work.Add(FString::Printf(TEXT("%s: filed, but the recording will not read (quality %.2f)."), *Name, J.RecordingQuality));
			if (J.bDocked) Got.Work.Add(FString::Printf(TEXT("%s: filed at half rate."), *Name));
			if (J.bAnomaly) Got.Work.Add(FString::Printf(TEXT("%s: result off normal, both measurements kept."), *Name));
		}
		else if (J.Stage >= FS47FreeformSession::EJobStage::Recorded) Got.Work.Add(FString::Printf(TEXT("%s: recorded, not filed."), *Name));
		else Got.Work.Add(FString::Printf(TEXT("%s: not recorded."), *Name));
	}
	if (Older > 0) Got.Work.Add(FString::Printf(TEXT("%d older %s still open."), Older, Older == 1 ? TEXT("job") : TEXT("jobs")));
	// CHAIN-2-20261109: DELIVERED
	Got.Side = Side; Got.SideLeft = Fs.SideLeftThisWeek();
	for (const FString& Key : Fs.FindingIdsFrom(FMath::Max(1, Night))) Got.bSameNight |= Key.StartsWith(TEXT("field-registration.")) && Fs.FindingText(Key).Contains(FS47FreeformSession::SameNightMark);
	if (bOpenKnown) for (const FS47KitOffer& O : FS47Openings::Defs())
		if (!OpenAtDusk.Contains(O.Id) && !Fs.HasKit(O.Id) && Fs.KitWaits(O.Id).IsEmpty()) Got.Opened.Add(FString::Printf(TEXT("%s ($%d)"), O.Title, Fs.KitPrice(O.Id)));
	Got.Ward = WardLine();
	return Got;
}

void FS47NightLog::Close(double ClockNow)
{
	if (Night <= 0 || !M || !M->Freeform) return;
	const FNight Done = Gather();
	FSheet Sh;
	Sh.Night = Night;
	Sh.Title = FString::Printf(TEXT("Night log, night of %s"), *Done.From);
	Sh.Page = Type(Done);
	Sh.Remark = RemarkFor(Done);
	Sh.Typed = ClockNow;
	Sh.Facts = Done.Lines;
	for (const FString& Job : Done.Work) Sh.Facts.Add({ SixOf(Night), EPart::Work, FString(), Job });
	if (Sh.Facts.Num() > 40) Sh.Facts.SetNum(40);
	Logs.Add(Sh);
	File(Sh);
	int32 NSeen = 0, NTime = 0, NGear = 0;
	for (const FLine& L : Done.Lines) { NSeen += L.Part == EPart::Seen; NTime += L.Part == EPart::Time; NGear += L.Part == EPart::Gear; }
	UE_LOG(LogS47NightLog, Display, TEXT("S47: night log %d typed: %d seen and heard, %d time, %d equipment, %d work, $%d to $%d"), Night, NSeen, NTime, NGear, Done.Work.Num(), Done.CashFrom, Done.CashTo);
	UE_LOG(LogS47NightLog, Display, TEXT("S47: night log %d page: %s"), Night, *Sh.Page.Replace(TEXT("\n"), TEXT(" | ")));
	if (M->UI && M->Freeform->Clock() < SixOf(Night) + 3 * 3600) M->UI->Toast(TEXT("06:00. The night log is typed: Documents, in your notebook. OPS can telex it to Dr. Ward."), 6);
	Night = 0; Lines.Reset(); Flickers = Drifts = Coughs = Jumps = 0; Side = 0;
	M->RequestAutosave();
}

void FS47NightLog::File(const FSheet& Sh) const
{
	if (!M || !M->Story) return;
	FString Page = Sh.Page;
	// The calendar date of a moment: the clock's day count from midnight (shift days turn at 08:00, dates at midnight).
	const auto Dated = [this](double Clock) { return M->Freeform ? S47Game::DateLine(M->Freeform->DateOfDay(int32(FMath::FloorToDouble(Clock / 86400.0)) + 1)) : FString(); };
	if (Sh.Sent >= 0) Page += FString::Printf(TEXT("\n\nTELEXED TO DR. WARD'S OFFICE %s, %s."), *S47NightLogLocal::ClockOf(Sh.Sent), *Dated(Sh.Sent));
	if (Sh.Read >= 0) Page += FString::Printf(TEXT("\nRECEIVED %s, %s. %s  W."), *S47NightLogLocal::ClockOf(Sh.Read), *Dated(Sh.Read), *Sh.Remark);
	FS47Doc D;
	D.Id = S47NightLogLocal::DocPrefix + FString::FromInt(Sh.Night);
	D.Title = Sh.Title;
	D.Kind = TEXT("typed");
	D.Stamp = Sh.Read >= 0 ? TEXT("INITIALED") : Sh.Sent >= 0 ? TEXT("TELEXED") : TEXT("NIGHT LOG");
	D.Page = Page;
	D.Transcript = Page;
	M->Story->AddDoc(D);
}

bool FS47NightLog::Send(int32 Nt)
{
	FSheet* Sh = Find(Nt);
	if (!Sh || Sh->Sent >= 0 || !M || !M->Freeform) return false;
	Sh->Sent = M->Freeform->Clock();
	File(*Sh);
	if (M->Audio) { M->Audio->Beep(1180.f, .05f, .04f); M->Audio->Beep(1180.f, .05f, .04f, .09f); M->Audio->Beep(1180.f, .05f, .04f, .18f); }
	if (M->UI) M->UI->Toast(FS47Predecessor::WardHours(M->Freeform->Clock()) ? TEXT("Night log telexed. Dr. Ward's office will initial it.") : TEXT("Night log telexed. Dr. Ward's office opens at 08:00."), 5);
	UE_LOG(LogS47NightLog, Display, TEXT("S47: night log %d telexed at %s"), Nt, *S47NightLogLocal::ClockOf(Sh->Sent));
	M->RequestAutosave();
	return true;
}

void FS47NightLog::Receive(double ClockNow)
{
	// Her office reads the telex in office hours, half an hour after it comes in at the soonest.
	if (!FS47Predecessor::WardHours(ClockNow)) return;
	for (FSheet& Sh : Logs)
	{
		if (Sh.Sent < 0 || Sh.Read >= 0 || ClockNow < Sh.Sent + 30 * 60) continue;
		Sh.Read = ClockNow;
		// TELEX-20261109: the receipt prints on the telex; the sheet in the notebook has her line once it is torn off (Received). A full
		// machine files it at once, as before.
		const auto Dated = [this](double Clock) { return M->Freeform ? S47Game::DateLine(M->Freeform->DateOfDay(int32(FMath::FloorToDouble(Clock / 86400.0)) + 1)) : FString(); };
		const FString Page = FString::Printf(TEXT("DR. WARD'S OFFICE\nTO SARO NIGHT OPERATOR\nRE NIGHT LOG, NIGHT OF %s\n\nRECEIVED %s, %s.\n%s\n\nW."),
			*S47Game::DateLine(M->Freeform->DateOfDay(Sh.Night)), *S47NightLogLocal::ClockOf(ClockNow), *Dated(ClockNow), *Sh.Remark);
		if (M->Freeform->QueueTelex(ReceiptId(Sh.Night), FString::Printf(TEXT("Dr. Ward's office: night log of %s"), *S47Game::DateLine(M->Freeform->DateOfDay(Sh.Night))), TEXT("TELEX / INITIALED"), Page))
		{
			if (M->UI) M->UI->Toast(TEXT("In the control room, the telex starts up: Dr. Ward's office."), 5);
		}
		else
		{
			File(Sh);
			if (M->UI) M->UI->Toast(TEXT("Dr. Ward's office initialed your night log. Her note is on it."), 5);
		}
		UE_LOG(LogS47NightLog, Display, TEXT("S47: night log %d initialed at %s: %s (%d initialed)"), Sh.Night, *S47NightLogLocal::ClockOf(ClockNow), *Sh.Remark, ReadCount());
		M->RequestAutosave();
	}
}

void FS47NightLog::Received(int32 Nt)
{
	if (const FSheet* Sh = Find(Nt); Sh && Sh->Read >= 0) File(*Sh);
}

void FS47NightLog::OpsRows(TArray<TSharedRef<SWidget>>& Into)
{
	if (Logs.IsEmpty() || !M || !M->Freeform) return;
	namespace FT = S47FreeformTerminal;
	TArray<S47Kit::FW> Col;
	int32 Shown = 0;
	for (int32 I = Logs.Num() - 1; I >= 0 && Shown < 3; --I)
	{
		const FSheet& Sh = Logs[I];
		if (Sh.Sent >= 0 || M->Freeform->Clock() - Sh.Typed > 7 * 86400.0) continue;
		++Shown;
		const int32 Nt = Sh.Night;
		Col.Add(FT::Button(FString::Printf(TEXT("TELEX TO DR. WARD / NIGHT OF %s"), *S47Game::DateLine(M->Freeform->DateOfDay(Nt))), [this, Nt]
		{
			Send(Nt);
			if (M && M->Freeform) M->Freeform->OpenOperations();
		}, Shown == 1));
	}
	const int32 Got = ReadCount();
	const FSheet* Last = nullptr;
	for (const FSheet& Sh : Logs) if (Sh.Sent >= 0 && (!Last || Sh.Sent > Last->Sent)) Last = &Sh;
	FString Status = FString::Printf(TEXT("Initialed by Dr. Ward's office: %d."), Got);
	// TELEX-20261109: what came back is read off the telex first
	if (Last && Last->Read >= 0 && M->Freeform->OnTelex(ReceiptId(Last->Night))) Status += TEXT(" Her office's receipt for the last is on the telex.");
	else if (Last) Status += Last->Read >= 0 ? FString::Printf(TEXT(" The last came back with a note on it: \"%s\""), *Last->Remark) : FString(TEXT(" The last is waiting for her office (08:00 to 21:00)."));
	Col.Add(FT::Small(Status));
	Into.Add(FT::Panel(TEXT("NIGHT LOG"), S47Kit::Col(Col)));
}

// ---------- the developer's menu ----------

bool FS47NightLog::DevTypeNow()
{
	if (!M || !M->Freeform) return false;
	if (!bPrimed) { Prime(); LastClock = M->Freeform->Clock(); }
	if (Night <= 0)
	{
		const int32 Nt = FS47FreeformSession::NightOf(M->Freeform->Clock());
		if (Find(Nt)) Logs.RemoveAll([Nt](const FSheet& Sh) { return Sh.Night == Nt; });
		Open(Nt, FMath::Min(M->Freeform->Clock(), DuskOf(Nt)));
	}
	Close(M->Freeform->Clock());
	return DevOpenLast();
}

void FS47NightLog::DevLostTime(double Minutes)
{
	if (!M || !M->Freeform) return;
	bDevLost = true;
	M->Freeform->TestAdvance(FMath::Max(6., Minutes) * 60.0);
}

bool FS47NightLog::DevOpenLast() const
{
	if (Logs.IsEmpty() || !M || !M->Story || !M->UI) return false;
	const FString Id = S47NightLogLocal::DocPrefix + FString::FromInt(Logs.Last().Night);
	for (const FS47Doc& D : M->Story->Docs()) if (D.Id == Id) { M->UI->Document(D); return true; }
	return false;
}

// ---------- the self-test ----------

bool FS47NightLog::SelfTest(TArray<FString>& Lines)
{
	bool bAll = true;
	const auto Check = [&](bool bOk, const FString& What) { bAll &= bOk; Lines.Add(FString(bOk ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("night log: ") + What); };
	Check(DuskOf(1) == 19 * 3600. && SixOf(1) == 30 * 3600. && FS47FreeformSession::NightOf(SixOf(3) - 1) == 3 && FS47FreeformSession::NightOf(DuskOf(3)) == 3, TEXT("a night runs from 19:00 to 06:00 and is the shift day it began on"));
	{
		FString At;
		const FString A = Lead(TEXT("At 02:14 lights came down out on the land, about 112 degrees from SARO, give or take 7, and went out on the ground. More after."), At);
		Check(At == TEXT("02:14") && A.StartsWith(TEXT("Lights came down")) && A.EndsWith(TEXT("ground.")) && !A.Contains(TEXT("More after")), TEXT("a record's time goes in the margin and only its first sentence goes on the sheet"));
		FString None;
		const FString B = Lead(FString::ChrN(30, TEXT('a')) + TEXT(" ") + FString::ChrN(200, TEXT('b')), None);
		Check(None.IsEmpty() && B.Len() <= 112 && B.EndsWith(TEXT("...")), TEXT("a long record is cut short"));
	}
	FNight Nt;
	Nt.Night = 4; Nt.From = TEXT("THU MAY 1"); Nt.To = TEXT("FRI MAY 2"); Nt.CashFrom = 142; Nt.CashTo = 167; Nt.In = 40; Nt.Out = 15;
	{
		const FString Quiet = Type(Nt);
		Check(Quiet.Contains(TEXT("NIGHT OF THU MAY 1 TO FRI MAY 2, 1986")) && Quiet.Contains(TEXT("Nothing out of the ordinary.")) && Quiet.Contains(TEXT("No faults.")) && Quiet.Contains(TEXT("$142 at dusk, $167 at 06:00. In $40, out $15.")), TEXT("a quiet night still gets its sheet, with the money"));
		Check(RemarkFor(Nt).Contains(TEXT("quiet night")), TEXT("Ward's line for a quiet night"));
		// CHAIN-2-20261109
		Check(Quiet.Contains(TEXT("\nDELIVERED\nMONEY   No field money tonight.\nOPENED  Nothing new.\nWARD    Nothing from out on the land tonight.\n")) && Quiet.Find(TEXT("DELIVERED")) < Quiet.Find(TEXT("ACCOUNT")), TEXT("CHAIN-2: a quiet night still has its three lines of what it delivered, before the account"));
		FNight Paid = Nt;
		Paid.Side = 12; Paid.SideLeft = 23; Paid.bSameNight = true; Paid.Opened.Add(TEXT("Survey magnetometer, used ($40)")); Paid.Ward = TEXT("1 field record from out on the land tonight.");
		const FString Sheet = Type(Paid);
		Check(Sheet.Contains(TEXT("MONEY   Field money tonight: $12, the same-night rate in it. $23 left of the week's $35.\n")) && Sheet.Contains(TEXT("OPENED  On the stores list now: Survey magnetometer, used ($40).\n")) && Sheet.Contains(TEXT("WARD    1 field record from out on the land tonight.\n")),
			TEXT("CHAIN-2: the money tonight with what is left of the week's cap, what opened on the stores list, and how it counts for Ward"));
	}
	for (int32 I = 0; I < 8; ++I) Nt.Lines.Add({ 23 * 3600. + I * 60, EPart::Seen, FString::Printf(TEXT("%02d:%02d"), 23, I), FString::Printf(TEXT("Thing %d."), I) });
	Nt.Lines.Add({ 26 * 3600., EPart::Time, FString(), TEXT("02:10 to 03:55 not accounted for (1 hr 45 min).") });
	Nt.Lines.Add({ 25 * 3600., EPart::Gear, TEXT("01:00"), TEXT("Flashlight battery ran flat.") });
	Nt.Work.Add(TEXT("J-1, Virgo A: filed, but the recording will not read (quality 0.21)."));
	{
		const FString Busy = Type(Nt);
		Check(Busy.Contains(TEXT("23:05  Thing 5.")) && !Busy.Contains(TEXT("Thing 6.")) && Busy.Contains(TEXT("And 2 more in the field records.")), TEXT("six things seen at most, the rest counted"));
		Check(Busy.Contains(TEXT("not accounted for")) && Busy.Contains(TEXT("01:00  Flashlight battery ran flat.")) && Busy.Contains(TEXT("will not read")), TEXT("time, equipment and work each in their own part"));
		Check(RemarkFor(Nt).Contains(TEXT("WWVB")), TEXT("lost time comes first in Ward's line"));
		// The sheet says what was met, never what it was.
		bool bPlain = true;
		for (const TCHAR* Word : { TEXT("alien"), TEXT("UFO"), TEXT("abduct"), TEXT("Roswell 1947"), TEXT("visitor") }) bPlain &= !Busy.Contains(Word) && !RemarkFor(Nt).Contains(Word);
		Check(bPlain, TEXT("no conclusions on the sheet or in Ward's line"));
	}
	Check(S47NightLogLocal::Skipped(TEXT("taken-3")) && S47NightLogLocal::Skipped(TEXT("place_mine_needle")) && !S47NightLogLocal::Skipped(TEXT("cattle_ring_d4")) && S47NightLogLocal::Instrument(TEXT("bearing_d4")), TEXT("being taken is time, not a record; the instruments' findings are work"));
	{
		// Saved and read back: the sheets with where they have got to, and tonight's lines so far.
		FS47NightLog A(nullptr);
		FSheet Sh; Sh.Night = 3; Sh.Title = TEXT("Night log, night of WED APR 30"); Sh.Page = TEXT("SARO / NIGHT LOG"); Sh.Remark = TEXT("Keep it that way."); Sh.Typed = SixOf(3); Sh.Sent = SixOf(3) + 600; Sh.Read = SixOf(3) + 7200;
		A.Logs.Add(Sh);
		A.Night = 4; A.From = DuskOf(4); A.CashFrom = 90; A.CashIn = 15; A.Flickers = 2;
		A.Lines.Add({ DuskOf(4) + 3600, EPart::Seen, TEXT("20:00"), TEXT("A light low over the flat.") });
		A.Side = 8; A.bOpenKnown = true; A.OpenAtDusk.Add(TEXT("cot")); A.OpenAtDusk.Add(TEXT("flying carpet"));
		FS47NightLog B(nullptr);
		B.Begin(S47J::Read(S47J::Write(A.Save())));
		Check(B.Logs.Num() == 1 && B.ReadCount() == 1 && B.Logs[0].Sent == Sh.Sent && B.Night == 4 && B.CashFrom == 90 && B.CashIn == 15 && B.Flickers == 2 && B.Lines.Num() == 1 && B.Lines[0].At == TEXT("20:00"), TEXT("sheets, what Ward has had, and tonight so far survive a Continue"));
		Check(B.Side == 8 && B.bOpenKnown && B.OpenAtDusk.Num() == 1 && B.OpenAtDusk.Contains(TEXT("cot")), TEXT("CHAIN-2: tonight's field money and the stores list at dusk survive a Continue, nonsense dropped"));
		{
			auto Old = A.Save(); S47J::Obj(Old, TEXT("tonight"))->RemoveField(TEXT("side")); S47J::Obj(Old, TEXT("tonight"))->RemoveField(TEXT("open"));
			FS47NightLog Od(nullptr);
			Od.Begin(S47J::Read(S47J::Write(Old)));
			Check(Od.Night == 4 && Od.Side == 0 && !Od.bOpenKnown, TEXT("CHAIN-2: an older save's night has no field money and reads the stores list again"));
		}
		FS47NightLog Cn(nullptr);
		Cn.Begin(nullptr);
		Check(Cn.Save()->Values.Num() == 0 && Cn.ReadCount() == 0, TEXT("an older save has no sheets and writes nothing"));
	}
	return bAll;
}

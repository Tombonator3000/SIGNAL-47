#include "S47WardReport.h"
#include "S47Docs.h"
#include "S47Freeform.h"
#include "S47FreeformTerminal.h"
#include "S47GameStyle.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47NightLog.h"
#include "S47Predecessor.h"
#include "S47Story.h"
#include "S47UI.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47WardReport, Log, All);

namespace S47WardReportLocal
{
	const TCHAR* const DocPrefix = TEXT("ward-report-");
	FString ClockOf(double Clock)
	{
		const double InDay = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
		return FString::Printf(TEXT("%02d:%02d"), int32(InDay / 3600) % 24, int32(FMath::Fmod(InDay, 3600.0) / 60));
	}
	uint8 Bit(FS47WardReport::ECat Ct) { return uint8(1u << uint8(Ct)); }
	FS47WardReport::ECat CatOf(FS47NightLog::EPart Pp)
	{
		switch (Pp)
		{
		case FS47NightLog::EPart::Time: return FS47WardReport::ECat::Time;
		case FS47NightLog::EPart::Gear: return FS47WardReport::ECat::Gear;
		case FS47NightLog::EPart::Work: return FS47WardReport::ECat::Work;
		default: return FS47WardReport::ECat::Seen;
		}
	}
}

// ---------- the pure rules ----------

const TCHAR* FS47WardReport::CatTitle(ECat Ct)
{
	switch (Ct)
	{
	case ECat::Seen: return TEXT("SEEN AND HEARD");
	case ECat::Time: return TEXT("TIME");
	case ECat::Gear: return TEXT("EQUIPMENT");
	case ECat::Work: return TEXT("WORK");
	default: return TEXT("ACCOUNT");
	}
}

const TCHAR* FS47WardReport::RoutineLine(ECat Ct)
{
	switch (Ct)
	{
	case ECat::Time: return TEXT("No time unaccounted for.");
	case ECat::Gear: return TEXT("Equipment in order.");
	case ECat::Work: return TEXT("Work on schedule.");
	case ECat::Money: return TEXT("Account in order.");
	default: return nullptr;
	}
}

FString FS47WardReport::Type(const FReport& Rep, const TFunction<FString(int32)>& DateOf)
{
	using namespace S47WardReportLocal;
	FString Out = FString::Printf(TEXT("SARO / WEEKLY REPORT TO DR. WARD / WEEK %d\nNights of %s to %s\n"), Rep.Week, *DateOf(Rep.First), *DateOf(Rep.Last));
	for (int32 Ci = 0; Ci < int32(ECat::Count); ++Ci)
	{
		const ECat Ct = ECat(Ci);
		int32 Had = 0;
		TArray<FString> On;
		for (const FItem& It : Rep.Items)
		{
			if (It.Cat != Ct) continue;
			++Had;
			if (It.bOn) On.Add(Ct == ECat::Money ? It.Text : Ct == ECat::Work ? FString::Printf(TEXT("%s  %s"), *DateOf(It.Night), *It.Text) : FString::Printf(TEXT("%s %s  %s"), *DateOf(It.Night), *ClockOf(It.Clock), *It.Text));
		}
		if (On.Num()) Out += FString::Printf(TEXT("\n%s\n%s\n"), CatTitle(Ct), *FString::Join(On, TEXT("\n")));
		else if ((Rep.Routine & Bit(Ct)) && RoutineLine(Ct)) Out += FString::Printf(TEXT("\n%s\n%s\n"), CatTitle(Ct), RoutineLine(Ct));
		else if (Had == 0) Out += FString::Printf(TEXT("\n%s\nNothing to report.\n"), CatTitle(Ct));
		// all of it left out and nothing said: the part is not on the report at all
	}
	return Out + TEXT("\nSARO night operator");
}

TArray<FS47WardReport::ECat> FS47WardReport::LiesIn(const FReport& Rep)
{
	TArray<ECat> Out;
	for (int32 Ci = 0; Ci < int32(ECat::Count); ++Ci)
	{
		const ECat Ct = ECat(Ci);
		if (!(Rep.Routine & S47WardReportLocal::Bit(Ct)) || !RoutineLine(Ct)) continue;
		if (Rep.Items.ContainsByPredicate([Ct](const FItem& It) { return It.Cat == Ct; })) Out.Add(Ct);
	}
	return Out;
}

int32 FS47WardReport::LeftOut(const FReport& Rep)
{
	int32 Nn = 0;
	for (const FItem& It : Rep.Items) Nn += It.bOn ? 0 : 1;
	return Nn;
}

// ---------- a game's life ----------

void FS47WardReport::Begin(const S47J::FObj& Saved)
{
	Stop();
	Life = MakeShared<uint8>(0);
	if (Saved.IsValid())
	{
		TrustNow = FMath::Clamp(S47J::Int(Saved, TEXT("trust")), -MostTrust, MostTrust);
		LastWeek = S47J::Int(Saved, TEXT("lastWeek"), -1);
		for (const auto& V : S47J::Arr(Saved, TEXT("reports")))
		{
			const S47J::FObj* O = nullptr;
			if (Kept.Num() >= 60 || !V.IsValid() || !V->TryGetObject(O) || !O) continue;
			FReport Rep;
			Rep.Week = S47J::Int(*O, TEXT("week")); Rep.First = S47J::Int(*O, TEXT("first")); Rep.Last = S47J::Int(*O, TEXT("last"));
			Rep.Due = S47J::Num(*O, TEXT("due")); Rep.Sent = S47J::Num(*O, TEXT("sent"), -1); Rep.Read = S47J::Num(*O, TEXT("read"), -1);
			Rep.Routine = uint8(FMath::Clamp(S47J::Int(*O, TEXT("routine")), 0, 31)); Rep.Delta = S47J::Int(*O, TEXT("delta"));
			Rep.Page = S47J::Str(*O, TEXT("page")).Left(6000); Rep.Reply = S47J::Str(*O, TEXT("reply")).Left(600);
			for (const auto& Iv : S47J::Arr(*O, TEXT("items")))
			{
				const TArray<TSharedPtr<FJsonValue>>* Five = nullptr;
				if (Rep.Items.Num() >= 50 || !Iv.IsValid() || !Iv->TryGetArray(Five) || !Five || Five->Num() < 5) continue;
				FItem It;
				It.Cat = ECat(FMath::Clamp(int32((*Five)[0]->AsNumber()), 0, int32(ECat::Count) - 1));
				It.Night = int32((*Five)[1]->AsNumber()); It.Clock = (*Five)[2]->AsNumber(); It.Text = (*Five)[3]->AsString().Left(300); It.bOn = (*Five)[4]->AsBool();
				Rep.Items.Add(It);
			}
			if (Rep.Week > 0) Kept.Add(Rep);
		}
		for (const auto& V : S47J::Arr(Saved, TEXT("lies")))
		{
			const S47J::FObj* O = nullptr;
			if (Lies.Num() >= 60 || !V.IsValid() || !V->TryGetObject(O) || !O) continue;
			FLie L;
			L.Week = S47J::Int(*O, TEXT("week")); L.Cat = ECat(FMath::Clamp(S47J::Int(*O, TEXT("cat")), 0, int32(ECat::Count) - 1));
			L.CatchAt = S47J::Num(*O, TEXT("at"), -1); L.bCaught = S47J::Bool(*O, TEXT("caught")); L.Proof = S47J::Str(*O, TEXT("proof")).Left(400);
			Lies.Add(L);
		}
	}
	UE_LOG(LogS47WardReport, Display, TEXT("S47: ward report: %d reports, %d sent, trust %d, %d lies (%d found out)"), Kept.Num(), SentCount(), TrustNow, Lies.Num(), CaughtCount());
}

void FS47WardReport::Stop()
{
	Life.Reset();
	Kept.Reset(); Lies.Reset();
	TrustNow = 0; LastWeek = -1; Look = 0;
}

S47J::FObj FS47WardReport::Save() const
{
	S47J::FObj O = S47J::New();
	if (Kept.IsEmpty() && Lies.IsEmpty() && TrustNow == 0 && LastWeek < 0) return O;
	S47J::Set(O, TEXT("trust"), TrustNow);
	S47J::Set(O, TEXT("lastWeek"), LastWeek);
	TArray<TSharedPtr<FJsonValue>> Reps;
	for (const FReport& Rep : Kept)
	{
		S47J::FObj E = S47J::New();
		S47J::Set(E, TEXT("week"), Rep.Week); S47J::Set(E, TEXT("first"), Rep.First); S47J::Set(E, TEXT("last"), Rep.Last);
		S47J::Set(E, TEXT("due"), Rep.Due); S47J::Set(E, TEXT("sent"), Rep.Sent); S47J::Set(E, TEXT("read"), Rep.Read);
		S47J::Set(E, TEXT("routine"), int32(Rep.Routine)); S47J::Set(E, TEXT("delta"), Rep.Delta);
		S47J::Set(E, TEXT("page"), Rep.Page); S47J::Set(E, TEXT("reply"), Rep.Reply);
		TArray<TSharedPtr<FJsonValue>> ItemArr;
		for (const FItem& It : Rep.Items)
			ItemArr.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{ MakeShared<FJsonValueNumber>(int32(It.Cat)), MakeShared<FJsonValueNumber>(It.Night), MakeShared<FJsonValueNumber>(It.Clock), MakeShared<FJsonValueString>(It.Text), MakeShared<FJsonValueBoolean>(It.bOn) }));
		E->SetArrayField(TEXT("items"), ItemArr);
		Reps.Add(MakeShared<FJsonValueObject>(E));
	}
	O->SetArrayField(TEXT("reports"), Reps);
	TArray<TSharedPtr<FJsonValue>> Ls;
	for (const FLie& L : Lies)
	{
		S47J::FObj E = S47J::New();
		S47J::Set(E, TEXT("week"), L.Week); S47J::Set(E, TEXT("cat"), int32(L.Cat)); S47J::Set(E, TEXT("at"), L.CatchAt); S47J::Set(E, TEXT("caught"), L.bCaught); S47J::Set(E, TEXT("proof"), L.Proof);
		Ls.Add(MakeShared<FJsonValueObject>(E));
	}
	O->SetArrayField(TEXT("lies"), Ls);
	return O;
}

int32 FS47WardReport::SentCount() const { int32 Nn = 0; for (const FReport& Rep : Kept) Nn += Rep.Sent >= 0 ? 1 : 0; return Nn; }
int32 FS47WardReport::CaughtCount() const { int32 Nn = 0; for (const FLie& L : Lies) Nn += L.bCaught ? 1 : 0; return Nn; }

FS47WardReport::FReport* FS47WardReport::Find(int32 Week) { return Kept.FindByPredicate([Week](const FReport& Rep) { return Rep.Week == Week; }); }

FString FS47WardReport::DateOf(int32 Night) const
{
	return M && M->Freeform ? S47Game::DateLine(M->Freeform->DateOfDay(Night)) : FString::Printf(TEXT("NIGHT %d"), Night);
}

void FS47WardReport::Step(double Dt)
{
	if (!M || !M->Freeform || !M->Freeform->Active()) return;
	Look += Dt;
	if (Look < 2.0) return;
	Look = 0;
	const int32 Reviewed = M->Freeform->NextReviewDay() / 7 - 1; // the weekly reviews behind us
	if (LastWeek < 0) LastWeek = Reviewed; // a game from before the report: nothing owed for weeks already played
	while (LastWeek < Reviewed) Make(++LastWeek);
	const double ClockNow = M->Freeform->Clock();
	Receive(ClockNow);
	if (!FS47Predecessor::WardHours(ClockNow)) return;
	for (FLie& L : Lies)
	{
		if (L.bCaught || L.CatchAt < 0 || ClockNow < L.CatchAt) continue;
		const FReport* Rep = Find(L.Week);
		if (!Rep || Rep->Read < 0) continue; // she reads the report before she finds anything in it
		Query(L);
	}
}

void FS47WardReport::Make(int32 Week)
{
	using namespace S47WardReportLocal;
	if (Week <= 0 || !M || !M->Freeform || Find(Week)) return;
	// The week before, never sent: a week she heard nothing from SARO.
	if (FReport* Before = Find(Week - 1); Before && Before->Sent < 0 && Before->Read > -2)
	{
		Before->Read = -2; Before->Delta = -1; Before->Reply = TEXT("Not sent.");
		TrustNow = FMath::Clamp(TrustNow - 1, -MostTrust, MostTrust);
	}
	FReport Rep;
	Rep.Week = Week; Rep.First = (Week - 1) * 7 + 1; Rep.Last = Week * 7; Rep.Due = M->Freeform->Clock();
	int32 PerCat[int32(ECat::Count)] = {};
	if (M->Freeform->NightLog)
		for (const FS47NightLog::FSheet& Sh : M->Freeform->NightLog->Sheets())
		{
			if (Sh.Night < Rep.First || Sh.Night > Rep.Last) continue;
			for (const FS47NightLog::FLine& L : Sh.Facts)
			{
				FItem It; It.Cat = CatOf(L.Part); It.Night = Sh.Night; It.Clock = L.Clock; It.Text = L.Text;
				if (PerCat[int32(It.Cat)]++ < MostItems) Rep.Items.Add(It);
			}
		}
	if (M->Freeform->Arrears() > 0) { FItem It; It.Cat = ECat::Money; It.Night = Rep.Last; It.Text = FString::Printf(TEXT("$%d carried to the next review."), M->Freeform->Arrears()); Rep.Items.Add(It); }
	Kept.Add(Rep);
	UE_LOG(LogS47WardReport, Display, TEXT("S47: ward report: week %d due, %d items from the night logs"), Week, Rep.Items.Num());
	if (M->UI) M->UI->Toast(FString::Printf(TEXT("The weekly report to Dr. Ward is due: week %d, at OPS."), Week), 5);
}

bool FS47WardReport::Send(int32 Week)
{
	using namespace S47WardReportLocal;
	FReport* Rep = Find(Week);
	if (!Rep || Rep->Sent >= 0 || !M || !M->Freeform) return false;
	const double ClockNow = M->Freeform->Clock();
	Rep->Sent = ClockNow;
	Rep->Page = Type(*Rep, [this](int32 Nn) { return DateOf(Nn); });
	File(*Rep);
	// Each lie, and where Dr. Ward could see through it.
	FRandomStream Roll(M->Freeform->Seed() * 31 + Week * 97);
	for (const ECat Ct : LiesIn(*Rep))
	{
		FLie L; L.Week = Week; L.Cat = Ct;
		const FItem* First = Rep->Items.FindByPredicate([Ct](const FItem& It) { return It.Cat == Ct; });
		const FString What = First ? First->Text : FString();
		// A night log she has had already says otherwise.
		const FS47NightLog::FSheet* Told = nullptr;
		if (M->Freeform->NightLog)
			for (const FS47NightLog::FSheet& Sh : M->Freeform->NightLog->Sheets())
				if (Sh.Night >= Rep->First && Sh.Night <= Rep->Last && Sh.Sent >= 0 && Sh.Facts.ContainsByPredicate([Ct](const FS47NightLog::FLine& Ln) { return CatOf(Ln.Part) == Ct; })) { Told = &Sh; break; }
		if (Told) { L.CatchAt = ClockNow; L.Proof = FString::Printf(TEXT("Your night log of %s, which came in on the telex, says: \"%s\""), *DateOf(Told->Night), *What); }
		else if (Ct == ECat::Money) { L.CatchAt = (M->Freeform->NextReviewDay() - 1) * 86400.0 + 9 * 3600.0; L.Proof = FString::Printf(TEXT("The county's statement for the station says: \"%s\""), *What); }
		else if (Ct == ECat::Work && (What.Contains(TEXT("S-")) || What.Contains(TEXT("array")) || What.Contains(TEXT("feeder")))) { L.CatchAt = ClockNow; L.Proof = FString::Printf(TEXT("The array's own record for the week says: \"%s\""), *What); }
		else
		{
			const double Chance = Ct == ECat::Time ? .4 : Ct == ECat::Gear ? .35 : .5;
			if (Roll.FRand() < Chance)
			{
				L.CatchAt = ClockNow + (2 + Roll.RandRange(0, 6)) * 86400.0;
				L.Proof = Ct == ECat::Time ? FString::Printf(TEXT("The station's time clock card for the week shows it: \"%s\""), *What)
					: Ct == ECat::Gear ? FString::Printf(TEXT("The county motor pool's slip for the week says: \"%s\""), *What)
					: FString::Printf(TEXT("The job sheets for the week say: \"%s\""), *What);
			}
		}
		Lies.Add(L);
	}
	UE_LOG(LogS47WardReport, Display, TEXT("S47: ward report: week %d telexed, %d items, %d left out, %d written as in order that were not"), Week, Rep->Items.Num(), LeftOut(*Rep), LiesIn(*Rep).Num());
	if (M->UI) M->UI->Toast(TEXT("Weekly report telexed to Dr. Ward's office. It is read in office hours."), 4.5f);
	M->RequestAutosave();
	return true;
}

void FS47WardReport::Receive(double ClockNow)
{
	using namespace S47WardReportLocal;
	if (!M || !M->Freeform || !FS47Predecessor::WardHours(ClockNow)) return;
	for (FReport& Rep : Kept)
	{
		if (Rep.Sent < 0 || Rep.Read != -1 || ClockNow < Rep.Sent + 30 * 60) continue;
		Rep.Read = ClockNow;
		// What she can hold it against: the night logs she has had for the same week.
		FString Fuller;
		if (M->Freeform->NightLog)
			for (const FItem& It : Rep.Items)
			{
				if (It.bOn || It.Cat != ECat::Seen) continue;
				const FS47NightLog::FSheet* Sh = M->Freeform->NightLog->Sheets().FindByPredicate([&It](const FS47NightLog::FSheet& Sht) { return Sht.Night == It.Night && Sht.Sent >= 0; });
				if (Sh) { Fuller = DateOf(Sh->Night); break; }
			}
		const bool bWhole = LeftOut(Rep) == 0 && LiesIn(Rep).IsEmpty();
		Rep.Delta = bWhole ? 1 : !Fuller.IsEmpty() ? -1 : 0;
		TrustNow = FMath::Clamp(TrustNow + Rep.Delta, -MostTrust, MostTrust);
		const bool bSeen = Rep.Items.ContainsByPredicate([](const FItem& It) { return It.Cat == ECat::Seen && It.bOn; });
		Rep.Reply = bWhole ? FString(TEXT("Received in full. Thank you.")) : !Fuller.IsEmpty() ? FString::Printf(TEXT("Received. Your night log of %s has more in it than this. Send what happened as it happened."), *Fuller) : FString(TEXT("Received."));
		if (bSeen) Rep.Reply += TEXT(" Keep the originals at the station.");
		File(Rep);
		UE_LOG(LogS47WardReport, Display, TEXT("S47: ward report: week %d read at %s, trust %+d to %d: %s"), Rep.Week, *ClockOf(ClockNow), Rep.Delta, TrustNow, *Rep.Reply);
		if (M->UI) M->UI->Toast(TEXT("Dr. Ward's office has read your weekly report. The reply is on it."), 5);
		M->RequestAutosave();
	}
}

void FS47WardReport::Query(const FLie& Ln)
{
	FLie* L = Lies.FindByPredicate([&Ln](const FLie& X) { return &X == &Ln; });
	if (!L || L->bCaught || !M) return;
	L->bCaught = true;
	TrustNow = FMath::Clamp(TrustNow - 2, -MostTrust, MostTrust);
	if (M->Story)
	{
		FS47Doc D;
		D.Id = FString::Printf(TEXT("ward-query-%d-%d"), L->Week, int32(L->Cat));
		D.Title = FString::Printf(TEXT("Dr. Ward, about your report for week %d"), L->Week);
		D.Kind = TEXT("typed"); D.Stamp = TEXT("TELEX");
		D.Page = FString::Printf(TEXT("TO SARO NIGHT OPERATOR\nRE YOUR WEEKLY REPORT, WEEK %d\n\nYour report says: \"%s\"\n%s\n\nI need the station's record as it is, not as it ought to be. Send what happened as it happened.\n\nW."),
			L->Week, RoutineLine(L->Cat) ? RoutineLine(L->Cat) : TEXT(""), *L->Proof);
		D.Transcript = D.Page;
		M->Story->AddDoc(D);
	}
	UE_LOG(LogS47WardReport, Display, TEXT("S47: ward report: week %d, %s found out, trust %d"), L->Week, CatTitle(L->Cat), TrustNow);
	if (M->UI) M->UI->Toast(TEXT("A telex from Dr. Ward's office about your weekly report. Documents, in your notebook."), 6);
	M->RequestAutosave();
}

void FS47WardReport::File(const FReport& Rep) const
{
	using namespace S47WardReportLocal;
	if (!M || !M->Story) return;
	FS47Doc D;
	D.Id = DocPrefix + FString::FromInt(Rep.Week);
	D.Title = FString::Printf(TEXT("Weekly report to Dr. Ward, week %d"), Rep.Week);
	D.Kind = TEXT("typed");
	D.Stamp = Rep.Read >= 0 ? TEXT("READ") : TEXT("TELEXED");
	D.Page = Rep.Page + (Rep.Read >= 0 ? FString::Printf(TEXT("\n\nRECEIVED %s. %s  W."), *ClockOf(Rep.Read), *Rep.Reply) : FString());
	D.Transcript = D.Page;
	M->Story->AddDoc(D);
}

void FS47WardReport::OpsRows(TArray<TSharedRef<SWidget>>& Into)
{
	if (Kept.IsEmpty() || !M || !M->Freeform) return;
	namespace FT = S47FreeformTerminal;
	TArray<S47Kit::FW> Col;
	for (int32 I = Kept.Num() - 1, Shown = 0; I >= 0 && Shown < 2; --I)
	{
		const FReport& Rep = Kept[I];
		if (Rep.Sent >= 0 || Rep.Read == -2) continue;
		++Shown;
		const int32 Wk = Rep.Week;
		Col.Add(FT::Button(FString::Printf(TEXT("WEEKLY REPORT TO DR. WARD / WEEK %d"), Wk), [this, Wk] { Open(Wk); }, Shown == 1));
	}
	const FReport* Last = nullptr;
	for (const FReport& Rep : Kept) if (Rep.Read >= 0 && (!Last || Rep.Read > Last->Read)) Last = &Rep;
	Col.Add(FT::Small(Last ? FString::Printf(TEXT("Week %d came back: \"%s\""), Last->Week, *Last->Reply) : FString(TEXT("Made from the week's night logs. You choose what goes in."))));
	Into.Add(FT::Panel(TEXT("WEEKLY REPORT"), S47Kit::Col(Col)));
}

void FS47WardReport::Open(int32 Week)
{
	using namespace S47WardReportLocal;
	FReport* Rep = Find(Week);
	if (!Rep || !M || !M->UI) return;
	const TWeakPtr<uint8> Guard = Life;
	TArray<S47Kit::FW> Form;
	Form.Add(S47Kit::Small(TEXT("Made from the week's night logs (Documents). Tick what goes in. A part you leave out entirely can be written as in order.")));
	for (int32 Ci = 0; Ci < int32(ECat::Count); ++Ci)
	{
		const ECat Ct = ECat(Ci);
		bool bAny = false, bAllOff = true;
		for (int32 I = 0; I < Rep->Items.Num(); ++I)
		{
			const FItem& It = Rep->Items[I];
			if (It.Cat != Ct) continue;
			if (!bAny) Form.Add(S47Kit::Section(CatTitle(Ct)));
			bAny = true; bAllOff &= !It.bOn;
			const FString Label = FString::Printf(TEXT("%s  %s"), It.bOn ? TEXT("[X]") : TEXT("[  ]"), *(Ct == ECat::Money ? It.Text : Ct == ECat::Work ? FString::Printf(TEXT("%s  %s"), *DateOf(It.Night), *It.Text) : FString::Printf(TEXT("%s %s  %s"), *DateOf(It.Night), *ClockOf(It.Clock), *It.Text)));
			Form.Add(S47Kit::Button(Label, [this, Guard, Week, I]
			{
				FReport* Rp = Guard.IsValid() ? Find(Week) : nullptr;
				if (!Rp || Rp->Sent >= 0 || !Rp->Items.IsValidIndex(I)) return;
				Rp->Items[I].bOn = !Rp->Items[I].bOn;
				if (Rp->Items[I].bOn) Rp->Routine &= ~Bit(Rp->Items[I].Cat);
				Open(Week);
			}, It.bOn));
		}
		if (bAny && bAllOff && RoutineLine(Ct))
		{
			const bool bRoutine = (Rep->Routine & Bit(Ct)) != 0;
			Form.Add(S47Kit::Button(FString::Printf(TEXT("%s  Write: \"%s\""), bRoutine ? TEXT("[X]") : TEXT("[  ]"), RoutineLine(Ct)), [this, Guard, Week, Ct]
			{
				FReport* Rp = Guard.IsValid() ? Find(Week) : nullptr;
				if (!Rp || Rp->Sent >= 0) return;
				Rp->Routine ^= Bit(Ct);
				Open(Week);
			}, bRoutine));
		}
	}
	Form.Add(S47Kit::Paper(Type(*Rep, [this](int32 Nn) { return DateOf(Nn); })));
	if (Rep->Sent < 0)
		Form.Add(S47Kit::Action(TEXT("TELEX TO DR. WARD"), [this, Guard, Week]
		{
			if (!Guard.IsValid() || !M || !M->UI) return;
			Send(Week);
			M->UI->Close();
		}));
	M->UI->Open(S47Kit::Shell(M->UI, FString::Printf(TEXT("WEEKLY REPORT / WEEK %d"), Week), S47Kit::Col(Form), 980));
}

// ---------- the developer's menu ----------

bool FS47WardReport::DevDue()
{
	if (!M || !M->Freeform) return false;
	const int32 Week = (M->Freeform->Day() - 1) / 7 + 1;
	if (const FReport* Was = Find(Week)) { if (Was->Sent >= 0) return false; Kept.RemoveAll([Week](const FReport& X) { return X.Week == Week; }); }
	Make(Week);
	LastWeek = FMath::Max(LastWeek, Week);
	return Find(Week) != nullptr;
}

int32 FS47WardReport::DevLie(ECat Part)
{
	for (int32 I = Kept.Num() - 1; I >= 0; --I)
	{
		FReport& Rep = Kept[I];
		if (Rep.Sent >= 0) continue;
		for (FItem& It : Rep.Items) if (It.Cat == Part) It.bOn = false;
		Rep.Routine |= S47WardReportLocal::Bit(Part);
		return Rep.Week;
	}
	return 0;
}

void FS47WardReport::DevCatchNow()
{
	for (FReport& Rep : Kept) if (Rep.Sent >= 0 && Rep.Read == -1) Rep.Read = Rep.Sent + 1;
	for (int32 I = 0; I < Lies.Num(); ++I) if (!Lies[I].bCaught)
	{
		if (Lies[I].Proof.IsEmpty()) Lies[I].Proof = TEXT("F10: found out now.");
		Query(Lies[I]);
	}
}

// ---------- tests ----------

bool FS47WardReport::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	const auto Check = [&Lines, &bOk](bool bPass, const TCHAR* What)
	{
		Lines.Add(FString::Printf(TEXT("S47: %s ward report: %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), What));
		bOk &= bPass;
	};
	FReport Rep; Rep.Week = 2; Rep.First = 8; Rep.Last = 14;
	const auto Item = [&Rep](ECat Ct, int32 Night, const TCHAR* Text, bool bOn) { FItem It; It.Cat = Ct; It.Night = Night; It.Clock = Night * 86400.0 - 3600; It.Text = Text; It.bOn = bOn; Rep.Items.Add(It); };
	Item(ECat::Seen, 9, TEXT("A light low over the flat north of the station."), true);
	Item(ECat::Time, 10, TEXT("Lost 1 hr 35 min."), false);
	Item(ECat::Gear, 11, TEXT("Flashlight cut out twice."), true);
	const auto Date = [](int32 Nn) { return FString::Printf(TEXT("N%d"), Nn); };
	FString Page = Type(Rep, Date);
	Check(Page.Contains(TEXT("A light low over the flat")) && !Page.Contains(TEXT("Lost 1 hr")) && !Page.Contains(TEXT("\nTIME\n")) && Page.Contains(TEXT("WORK\nNothing to report.")),
		TEXT("what is ticked goes in; a part left out is not on it; a part with nothing in it says so"));
	Check(LeftOut(Rep) == 1 && LiesIn(Rep).IsEmpty(), TEXT("leaving something out is not a lie"));
	Rep.Routine = uint8(1u << uint8(ECat::Time)) | uint8(1u << uint8(ECat::Work));
	Page = Type(Rep, Date);
	const TArray<ECat> Told = LiesIn(Rep);
	Check(Page.Contains(TEXT("No time unaccounted for.")) && Told.Num() == 1 && Told[0] == ECat::Time, TEXT("'in order' over what happened is a lie; over a quiet part it is only true"));
	Check(PointsFor(0) == 0 && PointsFor(2) == 1 && PointsFor(5) == 2 && PointsFor(-2) == -1 && PointsFor(-6) == -2, TEXT("trust counts a little in her assessment, either way"));
	Check(RoutineLine(ECat::Seen) == nullptr, TEXT("what was seen can be left out, never written away"));
	return bOk;
}

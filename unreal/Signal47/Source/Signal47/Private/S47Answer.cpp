#include "S47Answer.h"
#include "S47Audio.h"
#include "S47Docs.h"
#include "S47Freeform.h"
#include "S47GameStyle.h"
#include "S47Main.h"
#include "S47PinBoard.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Web.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Answer, Log, All);

namespace S47AnswerLocal
{
	constexpr double OnPoint = 2.0, Interval = 1.37, SlewAllowance = 240;
	FString ClockOf(double Clock)
	{
		const double InDay = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
		return FString::Printf(TEXT("%02d:%02d:%02d"), int32(InDay / 3600) % 24, int32(FMath::Fmod(InDay, 3600.0) / 60), int32(FMath::Fmod(InDay, 60.0)));
	}
}

// ---------- the pure rules ----------

bool FS47Answer::Answers(int32 Seed, int32 Index, double Held)
{
	FRandomStream Roll(Seed * 7919 + Index * 104729 + 17);
	const double Chance = .45 + .25 * FMath::Clamp((Held - MinHeld) / (45 * 60.0), 0.0, 1.0);
	return Roll.FRand() < Chance;
}

FString FS47Answer::Printout(const FCall& Cl, const FString& Date, bool bOnPoint)
{
	using namespace S47AnswerLocal;
	const int32 Pulses = PulsesFor(Cl.Held);
	FString Strip;
	for (int32 I = 0; I < Pulses; ++I)
	{
		Strip += TEXT("|.");
		if ((I + 1) % 30 == 0 && I + 1 < Pulses) Strip += TEXT("\n");
	}
	return FString::Printf(TEXT("SARO ARRAY / MONITOR CHANNEL / UNSCHEDULED OUTPUT\n%s  %s TO %s MST\nPOSITION AZ %05.1f EL %04.1f  %s\n1420.405 MHZ  BW 2.0 MHZ  INT 1 S\nLEVEL ABOVE NOISE +%.1f DB\nPULSES %d  INTERVAL %.2f S\n%s"),
		*Date, *ClockOf(Cl.At), *ClockOf(Cl.At + Pulses * Interval), Cl.Az, Cl.El, bOnPoint ? TEXT("ARRAY ON POSITION") : TEXT("ARRAY ELSEWHERE / SIDELOBE"), Cl.Level, Pulses, Interval, *Strip);
}

// ---------- a game's life ----------

void FS47Answer::Begin(const S47J::FObj& Saved)
{
	Stop();
	if (!Saved.IsValid()) return;
	for (const auto& V : S47J::Arr(Saved, TEXT("calls")))
	{
		const S47J::FObj* O = nullptr;
		if (Kept.Num() >= 40 || !V.IsValid() || !V->TryGetObject(O) || !O) continue;
		FCall Cl;
		Cl.Az = FMath::Fmod(S47J::Num(*O, TEXT("az")) + 360.0, 360.0); Cl.El = FMath::Clamp(S47J::Num(*O, TEXT("el")), 0.0, 90.0);
		Cl.From = S47J::Num(*O, TEXT("from")); Cl.Held = FMath::Clamp(S47J::Num(*O, TEXT("held")), 0.0, 6 * 3600.0); Cl.Night = S47J::Int(*O, TEXT("night"));
		Cl.bDone = S47J::Bool(*O, TEXT("done")); Cl.bAnswered = S47J::Bool(*O, TEXT("answered")); Cl.bWaiting = S47J::Bool(*O, TEXT("waiting"));
		Cl.At = S47J::Num(*O, TEXT("at")); Cl.Level = S47J::Num(*O, TEXT("level"));
		if (FMath::IsFinite(Cl.From) && FMath::IsFinite(Cl.At)) Kept.Add(Cl);
	}
}

void FS47Answer::Stop()
{
	Kept.Reset();
	bHolding = false; HoldAz = HoldEl = HoldFrom = Look = 0; OnSince = -1;
}

S47J::FObj FS47Answer::Save() const
{
	S47J::FObj O = S47J::New();
	if (Kept.IsEmpty()) return O;
	TArray<TSharedPtr<FJsonValue>> List;
	for (const FCall& Cl : Kept)
	{
		S47J::FObj E = S47J::New();
		S47J::Set(E, TEXT("az"), Cl.Az); S47J::Set(E, TEXT("el"), Cl.El); S47J::Set(E, TEXT("from"), Cl.From); S47J::Set(E, TEXT("held"), Cl.Held);
		S47J::Set(E, TEXT("night"), Cl.Night); S47J::Set(E, TEXT("done"), Cl.bDone); S47J::Set(E, TEXT("answered"), Cl.bAnswered); S47J::Set(E, TEXT("waiting"), Cl.bWaiting);
		S47J::Set(E, TEXT("at"), Cl.At); S47J::Set(E, TEXT("level"), Cl.Level);
		List.Add(MakeShared<FJsonValueObject>(E));
	}
	O->SetArrayField(TEXT("calls"), List);
	return O;
}

FVector2D FS47Answer::ArrayAzEl() const
{
	if (!M || !M->Web) return FVector2D::ZeroVector;
	// The dishes that answer the controller, on the circle: a mean of directions, not of numbers (359 and 1 are 0).
	double Sx = 0, Sy = 0, El = 0; int32 Nd = 0;
	for (int32 I = 0; I < M->Web->DishCount(); ++I)
	{
		if (!M->Web->DishRemoteAvailable(I)) continue;
		const double A = FMath::DegreesToRadians(M->Web->DishAz(I));
		Sx += FMath::Sin(A); Sy += FMath::Cos(A); El += M->Web->DishEl(I); ++Nd;
	}
	if (!Nd) return FVector2D(-999, -999);
	return FVector2D(FMath::Fmod(FMath::RadiansToDegrees(FMath::Atan2(Sx, Sy)) + 360.0, 360.0), El / Nd);
}

bool FS47Answer::Hold(double Az, double El)
{
	if (!M || !M->Web || !M->Freeform || !M->Freeform->HasPowerForReactiveLight() || El < MinEl || El > 90) return false;
	HoldAz = FMath::Fmod(Az + 360.0, 360.0); HoldEl = El; HoldFrom = M->Freeform->Clock(); bHolding = true; OnSince = -1;
	M->Web->PointDishes(HoldAz, HoldEl, 9);
	UE_LOG(LogS47Answer, Display, TEXT("S47: answer: array held at AZ %.1f EL %.1f, off schedule"), HoldAz, HoldEl);
	if (M->UI) M->UI->Toast(FString::Printf(TEXT("TRACKING: Array held at AZ %.0f EL %.0f, off schedule. Nothing is scheduled there. It stays until you point it elsewhere."), HoldAz, HoldEl), 6);
	return true;
}

void FS47Answer::Step(double Dt)
{
	using namespace S47AnswerLocal;
	if (!M || !M->Freeform || !M->Freeform->Active()) return;
	Look += Dt;
	if (Look < 1.0) return;
	Look = 0;
	const double ClockNow = M->Freeform->Clock();
	const FVector2D Pointing = ArrayAzEl();
	const auto On = [&Pointing](double Az, double El) { return FMath::Abs(FMath::FindDeltaAngleDegrees(Pointing.X, Az)) <= OnPoint && FMath::Abs(Pointing.Y - El) <= OnPoint; };
	// The hold: from when the dishes reach the point until they leave it (or never reach it).
	if (bHolding)
	{
		if (On(HoldAz, HoldEl)) { if (OnSince < 0) OnSince = ClockNow; }
		const bool bLeft = OnSince >= 0 ? !On(HoldAz, HoldEl) : ClockNow - HoldFrom > SlewAllowance;
		if (bLeft || !M->Freeform->HasPowerForReactiveLight())
		{
			const double Held = OnSince >= 0 ? ClockNow - OnSince : 0;
			if (Held >= MinHeld && Kept.Num() < 40)
			{
				FCall Cl; Cl.Az = HoldAz; Cl.El = HoldEl; Cl.From = OnSince; Cl.Held = Held; Cl.Night = FS47FreeformSession::NightOf(OnSince);
				Cl.At = OnSince + SiderealDay + Held * .5; // the middle of the hold, a sidereal day on
				Kept.Add(Cl);
				UE_LOG(LogS47Answer, Display, TEXT("S47: answer: held %.0f min at AZ %.1f EL %.1f; the same sky comes round at %s"), Held / 60, HoldAz, HoldEl, *ClockOf(Cl.At));
			}
			bHolding = false; OnSince = -1;
		}
	}
	for (int32 I = 0; I < Kept.Num(); ++I)
	{
		FCall& Cl = Kept[I];
		if (!Cl.bDone && ClockNow >= Cl.At - Window)
		{
			Cl.bDone = true;
			if (Answers(M->Freeform->Seed(), I, Cl.Held)) { Cl.Level = On(Cl.Az, Cl.El) ? 4.1 : .8; Answer(Cl, false); }
			UE_LOG(LogS47Answer, Display, TEXT("S47: answer: the call of night %d at AZ %.1f EL %.1f: %s"), Cl.Night, Cl.Az, Cl.El, Cl.bAnswered ? TEXT("answered") : TEXT("nothing"));
		}
		if (Cl.bWaiting && M->Freeform->PinBoard && M->Freeform->PinBoard->AtSaro()) Deliver(Cl);
	}
}

void FS47Answer::Answer(FCall& Cl, bool bForce)
{
	Cl.bAnswered = true; Cl.bWaiting = true;
	if (bForce && Cl.Level <= 0) Cl.Level = 4.1;
	// Heard in the room if anyone is there: the printer runs by itself.
	if (M && M->Audio && M->Web && M->Freeform && M->Freeform->PinBoard && M->Freeform->PinBoard->AtSaro()) M->Audio->Play(TEXT("printer"), .8f, M->Web->PropLocation(TEXT("printer")));
}

void FS47Answer::Deliver(FCall& Cl)
{
	if (!M || !M->Freeform) return;
	Cl.bWaiting = false;
	const FString Date = S47Game::DateLine(M->Freeform->DateOfDay(FS47FreeformSession::NightOf(Cl.At)));
	const bool bOnPoint = Cl.Level > 2;
	const FString Paper = Printout(Cl, Date, bOnPoint);
	if (M->Story)
	{
		FS47Doc D;
		D.Id = FString::Printf(TEXT("answer-printout-%d"), int32(FMath::FloorToDouble(Cl.At)));
		D.Title = TEXT("Printout, monitor channel, unscheduled");
		D.Kind = TEXT("printout"); D.Stamp = TEXT("MONITOR");
		D.Page = Paper; D.Transcript = Paper;
		M->Story->AddDoc(D);
	}
	const int32 Pulses = PulsesFor(Cl.Held);
	M->Freeform->RecordEvidence(FString::Printf(TEXT("answer_d%d_%d"), FS47FreeformSession::NightOf(Cl.At), int32(FMath::FloorToDouble(Cl.At)) % 100000),
		FString::Printf(TEXT("An unscheduled printout from the monitor channel, %s at %s: %d pulses, %.2f seconds apart, %.1f dB over the noise, from AZ %.0f EL %.0f. The array was held there off schedule the night before, %d minutes. Nothing is scheduled there."),
			*Date, *S47AnswerLocal::ClockOf(Cl.At).Left(5), Pulses, S47AnswerLocal::Interval, Cl.Level, Cl.Az, Cl.El, FMath::FloorToInt(Cl.Held / 60)));
	UE_LOG(LogS47Answer, Display, TEXT("S47: answer: printout delivered, %d pulses at %.1f dB (%s)"), Pulses, Cl.Level, bOnPoint ? TEXT("on position") : TEXT("sidelobe"));
	if (M->UI) M->UI->Toast(TEXT("There is a printout in the printer tray that nobody asked for. Documents, in your notebook."), 6);
	M->RequestAutosave();
}

// ---------- the developer's menu ----------

void FS47Answer::DevCall(double Az, double El, double Held)
{
	if (!M || !M->Freeform) return;
	FCall Cl; Cl.Az = Az; Cl.El = El; Cl.Held = Held; Cl.From = M->Freeform->Clock() - SiderealDay - Held * .5;
	Cl.Night = FS47FreeformSession::NightOf(Cl.From); Cl.At = M->Freeform->Clock() + 2;
	Kept.Add(Cl);
}

bool FS47Answer::DevAnswerNow()
{
	for (FCall& Cl : Kept) if (!Cl.bAnswered) { Cl.bDone = true; Cl.Level = 4.1; Answer(Cl, true); Deliver(Cl); return true; }
	return false;
}

// ---------- tests ----------

bool FS47Answer::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	const auto Check = [&Lines, &bOk](bool bPass, const TCHAR* What)
	{
		Lines.Add(FString::Printf(TEXT("S47: %s answer: %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), What));
		bOk &= bPass;
	};
	Check(PulsesFor(14 * 60) == 15 && PulsesFor(31 * 60 + 20) == 31 && PulsesFor(5 * 3600) == 90, TEXT("as many pulses as the minutes it was held"));
	int32 Short = 0, Long = 0;
	for (int32 Seed = 0; Seed < 400; ++Seed) { Short += Answers(Seed, 0, MinHeld); Long += Answers(Seed, 0, MinHeld + 45 * 60); }
	Check(Short > 140 && Short < 220 && Long > 240 && Long < 320, TEXT("not every time; likelier the longer it was held, never sure"));
	FCall Cl; Cl.Az = 214; Cl.El = 38; Cl.Held = 31 * 60; Cl.At = 3 * 3600 + 8 * 60; Cl.Level = 4.1;
	const FString Pr = Printout(Cl, TEXT("TUE APR 29"), true);
	Check(Pr.Contains(TEXT("AZ 214.0 EL 38.0")) && Pr.Contains(TEXT("PULSES 31")) && Pr.Contains(TEXT("03:08:00")) && !Pr.Contains(TEXT("signal from")), TEXT("the printout gives numbers and nothing about what it is"));
	Check(FMath::IsNearlyEqual(SiderealDay, 86164.09, .01), TEXT("the same sky, four minutes earlier each night"));
	return bOk;
}

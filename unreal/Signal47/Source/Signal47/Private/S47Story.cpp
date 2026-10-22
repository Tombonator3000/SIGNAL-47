#include "S47Story.h"

#include "S47Audio.h"
#include "S47HUD.h"
#include "S47Chapter.h"
#include "S47Chapter2.h"
#include "S47Chapter5.h"
#include "S47Json.h"
#include "S47Main.h"
#include "S47Web.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Story, Log, All);

namespace
{
	// the relay goes first (relayFirst): after the console is left to the sweep, K3 trips; forty
	// seconds later the lights dip (the service record's 40 s) for most of a second; then the skip
	constexpr double RelayFirstSec = 7.0, DipAfter = 40, DipLen = 0.7, DipSkip = 4.6;
}

AS47Story::AS47Story()
{
	PrimaryActorTick.bCanEverTick = true;
	OpsTerminal = MakeUnique<FS47OpsTerminal>([this]
	{
		FS47OpsView V;
		V.Clock = ClockSec;
		V.SolveAt = NS.SolveAt;
		V.Log = NS.Log;
		V.bFuture = ChapterKey == TEXT("ch5"); // future: this.phase === 'ch5' (All Night: RUN860414_0529.DAT)
		return V;
	});
}

void AS47Story::Bind(AS47Web* InSaro, AS47HUD* InHud, US47Audio* InAudio)
{
	Saro = InSaro; Hud = InHud; Audio = InAudio;
}

// ---------- helpers ----------

void AS47Story::After(double Seconds, TFunction<void()> Fn, const FString& Tag)
{
	Timers.Add({ GameTime + Seconds, MoveTemp(Fn), Tag });
}

void AS47Story::Cancel(const FString& Tag)
{
	Timers.RemoveAll([&](const FTimer& T) { return T.Tag == Tag; });
}

void AS47Story::Note(const FString& S)
{
	if (!NotesList.Contains(S)) NotesList.Add(S);
}

void AS47Story::Log(const FString& Text)
{
	NS.Log.Add({ FMath::RoundToDouble(ClockSec), Text });
}

void AS47Story::ConsoleLog(const FString& Text, TOptional<double> At)
{
	// in the night's order: the evening before midnight comes first
	auto Night = [](double V) { const double D = FMath::Fmod(FMath::Fmod(V, 86400.0) + 86400.0, 86400.0); return D >= 12 * 3600 ? D - 86400 : D; };
	const double C = FMath::FloorToDouble(At.Get(ClockSec) + 0.5);
	const int32 I = NS.Log.IndexOfByPredicate([&](const FS47LogLine& L) { return Night(L.Clock) > Night(C); });
	NS.Log.Insert({ C, Text }, I < 0 ? NS.Log.Num() : I);
}

bool AS47Story::RelayClack()
{
	if (bPowered && !NS.bK3Open) { TripK3(false); return true; }
	Log(TEXT("RX BANK 3  K3 OPEN"));
	return false;
}

FVector AS47Story::RackAt() const
{
	return Saro.IsValid() ? Saro.Get()->Obj(TEXT("room.objs.rack")).WorldPosition() : FVector::ZeroVector;
}

FVector AS47Story::TelexAt() const
{
	if (!Saro.IsValid()) return FVector(0, 0.9, 0);
	const FVector P = Saro.Get()->Obj(TEXT("room.objs.telex")).WorldPosition();
	return FVector(P.X, 0.9, P.Z);
}

TArray<FString> AS47Story::TelexRoll() const
{
	TArray<FString> R = S47Docs::TelexEvening();
	if (NS.Hondo) R.Add(S47Docs::TelexHondo(*NS.Hondo));
	return R;
}

void AS47Story::HondoTelex(double Filed, TOptional<double> At)
{
	if (NS.Hondo) return;
	NS.Hondo = FMath::FloorToDouble(Filed + 0.5);
	ConsoleLog(TEXT("TELEX RECEIVED  HONDO VALLEY"), At);
	// (room.setTelexRoll: the roll's paper on the machine is not drawn in this version)
	const int32 I = DocsList.IndexOfByPredicate([](const FS47Doc& D) { return D.Id == TEXT("telex"); });
	if (I >= 0) DocsList[I] = S47Docs::Telex(TelexRoll());
}

void AS47Story::PowerOut(bool bOut)
{
	bDark = bOut;
	if (bRadioOn && Audio.IsValid()) Audio->AmRadio(!bOut, Saro.IsValid() ? Saro->PropLocation(TEXT("radio")) : FVector::ZeroVector);
}

void AS47Story::AddDoc(const FS47Doc& Doc)
{
	const int32 I = DocsList.IndexOfByPredicate([&](const FS47Doc& D) { return D.Id == Doc.Id; });
	if (I < 0) DocsList.Add(Doc); else DocsList[I] = Doc;
}

void AS47Story::FilePaper(const FS47Doc& Doc)
{
	NS.Papers.AddUnique(Doc.Id);
	AddDoc(Doc);
}

void AS47Story::Toast(const FString& S, float Secs)
{
	if (Hud.IsValid()) Hud->Toast(S, Secs);
}

void AS47Story::OpenDoc(const FS47Doc& Doc, TFunction<void()> OnClose)
{
	if (Hud.IsValid()) Hud->ShowDocument(Doc, MoveTemp(OnClose));
}

void AS47Story::PointDishes(double Az, double El, double Slew)
{
	if (Saro.IsValid()) Saro->PointDishes(Az, El, Slew);
}

void AS47Story::SetLeds(bool bOn)
{
	if (Saro.IsValid()) Saro->SetLeds(bOn);
}

FString AS47Story::ChapterTitle() const
{
	// main.ts chapterTitle()
	static const TMap<FString, FString> Titles = {
		{ TEXT("ch1"), TEXT("Chapter 1: The Second Exposure") }, { TEXT("ch2"), TEXT("Chapter 2: The Amended Record") },
		{ TEXT("ch3"), TEXT("Chapter 3: The Survey Station") }, { TEXT("ch4"), TEXT("Chapter 4: Room 6") },
		{ TEXT("ch5"), TEXT("Chapter 5: All Night") }, { TEXT("ch6"), TEXT("Chapter 6: Roswell Road") } };
	if (const FString* T = Titles.Find(ChapterKey)) return *T;
	return TEXT("Prologue: Night Shift");
}

FString AS47Story::SurveyModeText() const
{
	if (At(ES47Phase::Event)) return TEXT("NO SCHEDULE ENTRY");
	if (Rx.Stage >= 2) return TEXT("SURVEY  H-LINE");
	if (Rx.Stage == 1) return TEXT("INTERFERENCE CHECK");
	return TEXT("CALIBRATION");
}

// ---------- notebook ----------

TArray<FS47Task> AS47Story::Tasks() const
{
	if (const US47Chapter* C = Chapter(ChapterKey); C && C->bActive) return C->Tasks();
	TArray<FS47Task> T;
	T.Add({ TEXT("Read Dale's shift log"), bLogRead });
	if (bLogRead || At(ES47Phase::Residual))
	{
		T.Add({ TEXT("Power up RX bank 3"), bPowered });
		T.Add({ TEXT("Calibrate on 1419.900 MHz"), Rx.Stage >= 1 });
		T.Add({ TEXT("Notch the highway relay near 1420.1"), Rx.Stage >= 2 });
		T.Add({ TEXT("Babysit the survey until 06:00"), At(ES47Phase::Residual) });
		T.Add({ TEXT("Coffee (optional)"), Coffee != ECoffee::None });
	}
	if (At(ES47Phase::Residual)) T.Add({ TEXT("Find the residual near 1420.4"), Rx.Stage >= 3 });
	if (Rx.Stage >= 3) T.Add({ TEXT("Run a direction solve on the tracking terminal"), Rx.bSolved });
	if (Rx.bSolved) T.Add({ TEXT("Tear off the printout"), At(ES47Phase::Ringing) });
	if (At(ES47Phase::Ringing)) T.Add({ TEXT("Answer the phone"), bAnswered });
	return T;
}

// ---------- interactables ----------

FString AS47Story::Label(const FString& Id) const
{
	if (Id == TEXT("logbook")) return bLogRead ? TEXT("Read the shift log") : TEXT("Read Dale's shift log");
	if (Id == TEXT("workOrder")) return TEXT("Read the work order");
	if (Id == TEXT("coffeePot")) return At(ES47Phase::Event) ? TEXT("Coffee machine") : Coffee == ECoffee::None ? TEXT("Fill your mug") : TEXT("Coffee machine");
	if (Id == TEXT("mug")) return (Saro.IsValid() && !Saro->IsMugVisible()) ? FString() : Coffee == ECoffee::Poured ? TEXT("Drink coffee") : TEXT("SARO mug");
	if (Id == TEXT("rack")) return NS.bK3Open ? TEXT("Reset RX bank 3") : bPowered ? TEXT("RX bank 3") : TEXT("Power up RX bank 3");
	if (Id == TEXT("crtCenter")) return bCinematic ? FString() : !bPowered ? TEXT("Receiver console") : bAlarmOn ? TEXT("Check the spectrum") : TEXT("Use receiver console");
	if (Id == TEXT("crtLeft")) return TEXT("Status terminal");
	if (Id == TEXT("crtRight")) return CurrentPhase == ES47Phase::Locked ? TEXT("Run direction solve") : CurrentPhase == ES47Phase::Solving ? FString() : TEXT("Tracking terminal");
	if (Id == TEXT("printer"))
	{
		if (CurrentPhase == ES47Phase::Printed) return TEXT("Tear off the printout");
		if (CurrentPhase == ES47Phase::Printing) return FString();
		// this.printerHook?.label() ?? 'Printer'
		const FString Hooked = PrinterHook.Label ? PrinterHook.Label() : FString();
		return Hooked.IsEmpty() ? FString(TEXT("Printer")) : Hooked;
	}
	if (Id == TEXT("phone"))
	{
		// phase 'ch2': this.ch2.phoneLabel()
		if (const US47Chapter2* C2 = ChapterKey == TEXT("ch2") ? Cast<US47Chapter2>(Chapter(TEXT("ch2"))) : nullptr) return C2->PhoneLabel();
		return bRinging ? TEXT("Answer the phone") : CurrentPhase == ES47Phase::Call ? FString() : TEXT("Telephone");
	}
	if (Id == TEXT("clock")) return TEXT("Wall clock");
	if (Id == TEXT("map")) return TEXT("Map of New Mexico");
	if (Id == TEXT("excBinder")) return bCinematic ? FString() : TEXT("Exceptions binder");
	if (Id == TEXT("serviceRecord")) return bCinematic ? FString() : TEXT("Service record");
	if (Id == TEXT("telex")) return bCinematic ? FString() : TEXT("Telex roll");
	if (Id == TEXT("rfiCard")) return bCinematic ? FString() : TEXT("Dale's card");
	if (Id == TEXT("halley")) return bCinematic ? FString() : TEXT("Halley's comet poster");
	if (Id == TEXT("opsTerminal")) return bCinematic ? FString() : TEXT("Operations terminal");
	if (Id == TEXT("jacket")) return (bCinematic || NS.bJacket) ? FString() : TEXT("Your jacket");
	if (Id == TEXT("radio")) return bRadioOn ? TEXT("Turn the radio off") : TEXT("Turn the radio on");
	if (Saro.IsValid() && Saro->IsDoor(Id)) return Saro->DoorLabel(Id);
	return FString();
}

void AS47Story::Use(const FString& Id)
{
	if (bCinematic && !(Saro.IsValid() && Saro->IsDoor(Id))) return;
	UE_LOG(LogS47Story, Display, TEXT("S47: used %s in phase %d"), *Id, (int32)CurrentPhase);
	if (Id == TEXT("logbook")) { OpenDoc(S47Docs::ShiftLog(), [this] { LogClosed(); }); return; }
	if (Id == TEXT("workOrder")) { OpenDoc(S47Docs::WorkOrder(), [this] { WorkOrderClosed(); }); return; }
	if (Id == TEXT("coffeePot"))
	{
		if (Coffee != ECoffee::None || At(ES47Phase::Event)) { Toast(TEXT("The hot plate ticks. It has been on since the day shift.")); return; }
		if (Audio.IsValid()) Audio->Pour(Saro.IsValid() ? Saro->PropLocation(TEXT("coffeePot")) : FVector::ZeroVector);
		Coffee = ECoffee::Poured;
		After(1.5, [this] { if (Saro.IsValid()) { Saro->SetCoffee(true); Saro->SetCoffeeLevel(0.088); } Toast(TEXT("You fill the SARO mug. Fresh-ish, like he said.")); });
		return;
	}
	if (Id == TEXT("mug"))
	{
		if (Coffee == ECoffee::Poured) { Coffee = ECoffee::Drunk; if (Saro.IsValid()) Saro->SetCoffeeLevel(0.05); if (Audio.IsValid()) Audio->Play(TEXT("thud_soft"), 0.25f, Saro.IsValid() ? Saro->PropLocation(TEXT("mug")) : FVector::ZeroVector); Toast(TEXT("Burnt. It helps.")); }
		else if (Coffee == ECoffee::Drunk) Toast(TEXT("Half a cup left. Still burnt."));
		else Toast(TEXT("Empty. The coffee machine is in the corner."));
		return;
	}
	if (Id == TEXT("rack"))
	{
		if (bDark) { Toast(TEXT("The rack is dead. No power anywhere.")); return; }
		if (NS.bK3Open) { ResetK3(); return; }
		if (bPowered) { Toast(TEXT("Bank 3 online. Sixteen green, two amber. Amber is normal, according to Dale.")); return; }
		PowerUp(false);
		return;
	}
	if (Id == TEXT("crtCenter"))
	{
		if (bDark) { Toast(TEXT("Dead screen. No power.")); return; }
		if (!bPowered) { Toast(TEXT("Dead screen. RX bank 3 is still off.")); if (Audio.IsValid()) Audio->Play(TEXT("click"), 0.4f); return; }
		if (NS.bK3Open) { Toast(TEXT("Dead screen. Bank 3 has tripped again. The lever is on the rack.")); if (Audio.IsValid()) Audio->Play(TEXT("click"), 0.4f); return; }
		bAlarmOn = false;
		if (Audio.IsValid()) Audio->Play(TEXT("click"), 0.5f);
		if (Hud.IsValid()) { bConsoleOpen = true; Hud->OpenConsole(); }
		return;
	}
	if (Id == TEXT("crtLeft"))
	{
		if (!bPowered) { Toast(TEXT("No power.")); return; }
		const FString ArrayAt = ArrayText ? ArrayText() : FString();
		if (!ArrayAt.IsEmpty()) Toast(ArrayAt);
		else if (At(ES47Phase::Event)) Toast(TEXT("S-01 to S-27: no control command recorded."));
		else if (Rx.Stage >= 2) Toast(FString::Printf(TEXT("Array on survey schedule. Az %s, el %d. Nothing logged since midnight."), *S47Text::Pad(Saro.IsValid() ? Saro->DishAz(0) : ParkAz, 3), (int32)SurveyEl));
		else Toast(TEXT("Array parked. Receiver needs calibrating before the survey can start."));
		return;
	}
	if (Id == TEXT("crtRight"))
	{
		if (!bPowered) { Toast(TEXT("No power.")); return; }
		if (CurrentPhase == ES47Phase::Locked) { StartSolve(); return; }
		if (Rx.Stage < 3) Toast(TEXT("Direction solve needs a locked source. Nothing worth solving."));
		else if (At(ES47Phase::Countdown)) Toast(TEXT("It is counting from when the line went dead."));
		else Toast(TEXT("Solve finished. The printer has the rest."));
		return;
	}
	if (Id == TEXT("printer"))
	{
		if (CurrentPhase == ES47Phase::Printed) TearPrintout();
		else if (PrinterHook.Use && PrinterHook.Use()) return;
		else Toast(TEXT("Dot matrix printer. A box of fan-fold paper under the table."));
		return;
	}
	if (Id == TEXT("phone"))
	{
		// phase 'ch2': this.ch2.usePhone()
		if (US47Chapter2* C2 = ChapterKey == TEXT("ch2") ? Cast<US47Chapter2>(Chapter(TEXT("ch2"))) : nullptr) { C2->UsePhone(); return; }
		// phases 'ch3' to 'ch5': the supervisor line is quiet, and where to go instead
		if (ChapterKey == TEXT("ch3") || ChapterKey == TEXT("ch4") || ChapterKey == TEXT("ch5"))
		{
			Toast(ChapterKey == TEXT("ch3") ? TEXT("The supervisor line is quiet. Ward wants a field record, not a phone call.")
				: ChapterKey == TEXT("ch4") ? TEXT("The supervisor line is quiet. N. Vega is waiting across the road.")
				: TEXT("The supervisor line is quiet. Ward will be up by now; there is a payphone at the diner."));
			return;
		}
		if (bRinging) { Answer(); return; }
		if (bAnswered) Toast(TEXT("Dead line. Not even a dial tone."));
		else Toast(TEXT("Internal line and one outside line. Nobody calls out here after midnight."));
		return;
	}
	if (Id == TEXT("clock")) { Toast(FString::Printf(TEXT("%s. %s"), *ClockText(false), At(ES47Phase::Residual) ? TEXT("Where did the night go?") : TEXT("Six hours and change to go."))); return; }
	if (Id == TEXT("map")) { Toast(TEXT("SARO is the red X on the plains. Somebody drew a ring around Roswell and a question mark.")); return; }
	if (Id == TEXT("excBinder")) { const FS47Doc D = S47Docs::Binder(); OpenDoc(D, [this, D] { FilePaper(D); }); return; }
	if (Id == TEXT("serviceRecord")) { const FS47Doc D = S47Docs::ServiceRecord(); OpenDoc(D, [this, D] { FilePaper(D); }); return; }
	if (Id == TEXT("telex")) { const FS47Doc D = S47Docs::Telex(TelexRoll()); OpenDoc(D, [this, D] { FilePaper(D); if (NS.Hondo) Note(S47Docs::HondoNote(*NS.Hondo)); }); return; }
	if (Id == TEXT("rfiCard")) { const FS47Doc D = S47Docs::InterferenceCard(Rx.Stage >= 3); OpenDoc(D, [this, D] { FilePaper(D); }); return; }
	if (Id == TEXT("halley")) { const FS47Doc D = S47Docs::HalleyPoster(); OpenDoc(D, [this, D] { FilePaper(D); }); return; }
	if (Id == TEXT("opsTerminal")) { if (bDark) { Toast(TEXT("Dead screen. No power.")); return; } if (Audio.IsValid()) Audio->Play(TEXT("click"), 0.4f); if (Hud.IsValid()) Hud->OpenOps(); return; }
	if (Id == TEXT("jacket")) { Toast(TEXT("SARO issue, with a reflective band all the way round. Too warm for in here."), 3.2f); return; }
	if (Id == TEXT("radio")) { ToggleRadio(); return; }
	if (Saro.IsValid() && Saro->IsDoor(Id)) { Saro->ToggleDoor(Id); return; }
}

void AS47Story::OpsRan(const FString& Cmd, const FString& Result)
{
	OpsLines.Add(TEXT("$ ") + Cmd.ToUpper());
	TArray<FString> Lines;
	Result.ParseIntoArray(Lines, TEXT("\n"), false);
	OpsLines.Append(Lines);
	if (OpsLines.Num() > 9) OpsLines.RemoveAt(0, OpsLines.Num() - 9);
	if (Audio.IsValid()) Audio->Play(TEXT("beep_1400_short"), 0.2f);
	// openOps onCommand: if (this.phase === 'ch5') this.ch5.terminalCommand(c) (the terminal hands it on upper case)
	if (ChapterKey == TEXT("ch5")) if (US47Chapter5* C5 = Cast<US47Chapter5>(Chapter(TEXT("ch5")))) C5->TerminalCommand(Cmd.ToUpper());
}

void AS47Story::ToggleRadio()
{
	if (bDark) { Toast(TEXT("Nothing. The power is out.")); return; }
	bRadioOn = !bRadioOn;
	if (Audio.IsValid())
	{
		Audio->Play(TEXT("click"), 0.35f, FVector::ZeroVector, 1.3f);
		Audio->AmRadio(bRadioOn, Saro.IsValid() ? Saro->PropLocation(TEXT("radio")) : FVector::ZeroVector);
	}
	if (bRadioOn && !bRadioHeard) { bRadioHeard = true; Toast(TEXT("A station a long way off, coming and going in the static."), 3.f); }
}

// ---------- relay K3 and the rack ----------

void AS47Story::ApplyK3(bool bOpen)
{
	Rx.bPowered = !bOpen;
	if (Saro.IsValid())
	{
		Saro->SetLever(!bOpen);
		Saro->SetCrtPowered(1, !bOpen);
		Saro->SetCrtGlow(bOpen ? 0.45f : 0.9f);
	}
	SetLeds(!bOpen);
	if (bOpen && Audio.IsValid()) { Audio->SetCarrier(0, 0, 0); Audio->SignalOff(); }
}

void AS47Story::TripK3(bool bSound)
{
	if (!bPowered || NS.bK3Open) return;
	NS.bK3Open = true;
	Log(TEXT("RX BANK 3  K3 OPEN"));
	if (bSound && Audio.IsValid())
	{
		const FVector At = Saro.IsValid() ? Saro->PropLocation(TEXT("rack")) : FVector::ZeroVector;
		Audio->Play(TEXT("rack_switch"), 1.0f, At, 0.62f);
		Audio->Play(TEXT("thud_soft"), 0.45f, At, 1.4f);
	}
	ApplyK3(true);
}

void AS47Story::ResetK3()
{
	if (bCinematic) return;
	NS.bK3Open = false;
	Log(TEXT("RX BANK 3  K3 CLOSED"));
	const FVector At = Saro.IsValid() ? Saro->PropLocation(TEXT("rack")) : FVector::ZeroVector;
	if (Audio.IsValid()) { Audio->Play(TEXT("rack_switch"), 0.9f, At); Audio->Play(TEXT("thud_soft"), 0.5f, At); }
	ApplyK3(false);
	Toast(TEXT("Bank 3 back on. Sixteen green, two amber."), 2.6f);
}

void AS47Story::PowerUp(bool bInstant)
{
	bPowered = true;
	Rx.bPowered = true;
	if (!bInstant) Log(TEXT("RX BANK 3  K3 CLOSED"));
	if (Saro.IsValid()) Saro->SetLever(true);
	if (Audio.IsValid()) Audio->StartCarrier();
	if (bInstant)
	{
		SetLeds(true);
		if (Saro.IsValid()) { for (int32 i = 0; i < 3; ++i) Saro->SetCrtPowered(i, true); Saro->SetCrtGlow(0.9f); }
		BootT = -10;
		return;
	}
	const FVector At = Saro.IsValid() ? Saro->PropLocation(TEXT("rack")) : FVector::ZeroVector;
	if (Audio.IsValid()) { Audio->Play(TEXT("rack_switch"), 0.9f, At); Audio->Play(TEXT("thud_soft"), 0.5f, At); }
	for (int32 i = 0; i < 18; ++i)
	{
		After(0.3 + i * 0.07, [this, i] { if (Saro.IsValid()) Saro->SetLed(i, true); if (i % 6 == 0 && Audio.IsValid()) Audio->Play(TEXT("beep_900"), 0.15f); });
	}
	After(1.6, [this]
	{
		if (Saro.IsValid()) { for (int32 i = 0; i < 3; ++i) Saro->SetCrtPowered(i, true); Saro->SetCrtGlow(0.9f); }
		BootT = GameTime;
		if (Audio.IsValid()) Audio->Play(TEXT("beep_1500"), 0.25f);
		Toast(TEXT("RX bank 3 hums awake. The console screens warm up."));
	});
	if (!bLogRead) After(4.5, [this] { Toast(TEXT("Dale's shift log is still on the desk.")); });
}

// ---------- the receiver console ----------

void AS47Story::ConsoleAzimuth()
{
	PointDishes(Rx.Azimuth, SurveyEl, 9);
}

void AS47Story::ConsoleAction()
{
	if (!Rx.Pass()) { Rx.Status = TEXT("NO LOCK  /  ADJUST PARAMETERS"); return; }
	StagePassed();
}

void AS47Story::StagePassed()
{
	const int32 S = Rx.Stage;
	Note(S47Signal::Profiles()[FMath::Min(S, 2)].Note);
	Rx.Advance();
	PointDishes(Rx.Azimuth, SurveyEl, 6);
	if (Audio.IsValid()) { Audio->Play(TEXT("beep_1320"), 0.3f); Audio->Play(TEXT("beep_1760"), 0.3f); }
	if (S == 0) Toast(TEXT("Calibrated. Next, the highway relay near 1420.1."));
	else if (S == 1) { SetPhase(ES47Phase::Survey); Toast(TEXT("Notched. The survey sweep takes over from here.")); }
	else if (S == 2)
	{
		SetPhase(ES47Phase::Locked);
		Note(TEXT("02:14. Pulse group: four, a gap, seven. Repeats every 5.2 seconds. Not the relay. Not anything in the schedule."));
		Toast(TEXT("Four pulses. A gap. Seven pulses. Again."), 4.f);
		After(4.5, [this] { if (CurrentPhase == ES47Phase::Locked) Toast(TEXT("The tracking terminal can run a direction solve.")); });
	}
}

void AS47Story::ConsoleClosed()
{
	bConsoleOpen = false;
	if (Rx.Stage == 2 && CurrentPhase == ES47Phase::Survey && !bSkipQueued)
	{
		bSkipQueued = true;
		After(2.5, [this] { Toast(TEXT("Nothing to do now but watch a sweep that takes all night."), 3.5f); });
		After(RelayFirstSec, [this] { RelayFirst(); }, TEXT("skip"));
	}
}

// ---------- beats ----------

void AS47Story::WorkOrderClosed()
{
	if (DocsList.ContainsByPredicate([](const FS47Doc& D) { return D.Id == TEXT("workorder"); })) return;
	DocsList.Add(S47Docs::WorkOrder());
	Note(TEXT("Work order: Reyes on nights, Dr. Ward on call. The morning series at 06:00 waits for a report she can sign."));
}

void AS47Story::LogClosed()
{
	if (bLogRead) return;
	bLogRead = true;
	DocsList.Add(S47Docs::ShiftLog());
	if (CurrentPhase == ES47Phase::Intro) SetPhase(ES47Phase::Shift);
	After(0.6, [this] { Toast(bPowered ? TEXT("Bank 3 is already up. Calibration next.") : TEXT("First the rack. Bank 3 is on the east wall, by the printer.")); });
}

void AS47Story::RelayFirst()
{
	if (CurrentPhase != ES47Phase::Survey) return;
	TripK3();
	After(0.5, [this] { Toast(TEXT("A clack from the rack. Bank 3 has dropped out again."), 3.4f); });
	After(DipAfter, [this] { LightsDip(); }, TEXT("skip"));
}

void AS47Story::LightsDip()
{
	if (CurrentPhase != ES47Phase::Survey) return;
	DipT = DipLen;
	CrtGlitch = DipLen;
	Log(TEXT("LINE POWER  DIP 0.7 S"));
	if (Audio.IsValid())
	{
		Audio->Play(TEXT("thud_soft"), 0.55f, Saro.IsValid() ? Saro->PropLocation(TEXT("rack")) : FVector::ZeroVector, 0.45f);
		Audio->HoldRoomTone(0.2f);
		After(DipLen, [this] { if (Audio.IsValid()) Audio->HoldRoomTone(1.f); });
	}
	After(1.3, [this] { Toast(TEXT("The lights dip. Forty seconds after the relay."), 3.8f); });
	Note(FString::Printf(TEXT("%s. K3 tripped by itself. Forty seconds later the lights dipped. The service record says it does that: it trips before, not after."), *ClockText(false)));
	After(DipSkip, [this] { TimeSkip(); }, TEXT("skip"));
}

void AS47Story::TimeSkip()
{
	// (the bank back on, if the player left it out: somebody threw the lever in the hours between)
	if (NS.bK3Open)
	{
		NS.bK3Open = false;
		NS.Log.Add({ FMath::RoundToDouble(ClockSec) + 95, TEXT("RX BANK 3  K3 CLOSED") });
		ApplyK3(false);
	}
	SetPhase(ES47Phase::Skip);
	bCinematic = true;
	if (Hud.IsValid()) Hud->Fade(true, TEXT("02:13"));
	After(3.6, [this]
	{
		if (Hud.IsValid()) Hud->CloseModal();
		ClockSec = SkipClock;
		Coffee = Coffee == ECoffee::None ? ECoffee::None : ECoffee::Drunk;
		if (Hud.IsValid()) Hud->Place(0.05, -2.2, 0);
		EnterResidual();
		if (Hud.IsValid()) Hud->Fade(false, TEXT(""));
		After(0.9, [this] { bCinematic = false; });
	});
}

void AS47Story::EnterResidual()
{
	SetPhase(ES47Phase::Residual);
	if (Hud.IsValid()) Hud->Checkpoint(TEXT("residual"));
	if (OnCheckpoint) OnCheckpoint(TEXT("residual"));
	After(6, [this]
	{
		bResidualVisible = true;
		Rx.Status = TEXT("UNLOGGED RESIDUAL  /  1420.4 MHz");
		bAlarmOn = true;
		Note(TEXT("02:13:47. Unlogged residual on the spectrum near 1420.4. Survey schedule says nothing should be there."));
		Toast(TEXT("02:13:47. The console is beeping."), 3.5f);
		Alarm();
	});
}

void AS47Story::Alarm()
{
	if (!bAlarmOn) return;
	if (Audio.IsValid()) { Audio->Play(TEXT("beep_1980"), 0.25f); After(0.18, [this] { if (Audio.IsValid()) Audio->Play(TEXT("beep_1980"), 0.25f); }); }
	After(3.2, [this] { Alarm(); }, TEXT("alarm"));
}

void AS47Story::StartSolve()
{
	SetPhase(ES47Phase::Solving);
	SolveT = GameTime;
	for (int32 i = 0; i < 14; ++i) After(i * 0.28, [this, i] { if (Audio.IsValid()) Audio->Play(FString::Printf(TEXT("solve_%d"), i % 4), 0.25f); });
	After(4.2, [this]
	{
		Rx.bSolved = true;
		NS.SolveAt = FMath::RoundToDouble(ClockSec);
		Log(TEXT("SOLVE  RANGE NEGATIVE"));
		Note(TEXT("Direction solve: RA 05h 17m, Dec -05. Somewhere in Orion."));
		if (Audio.IsValid()) Audio->Play(TEXT("beep_1500_long"), 0.3f);
		StartPrint();
	});
}

void AS47Story::StartPrint()
{
	SetPhase(ES47Phase::Printing);
	PrintT = GameTime;
	if (Saro.IsValid()) Saro->SetPrinterPaper(0.001f);
	if (Audio.IsValid()) Audio->Loop(TEXT("printer"), TEXT("printer"), 0.8f, Saro.IsValid() ? Saro->PropLocation(TEXT("printer")) : FVector::ZeroVector);
	After(5.4, [this]
	{
		if (Audio.IsValid()) Audio->Stop(TEXT("printer"), 0.04f);
		SetPhase(ES47Phase::Printed);
		Toast(TEXT("The printer stops."));
	});
}

void AS47Story::TearPrintout()
{
	if (Audio.IsValid()) Audio->Play(TEXT("thud_soft"), 0.35f, Saro.IsValid() ? Saro->PropLocation(TEXT("printer")) : FVector::ZeroVector, 1.6f);
	if (Saro.IsValid()) Saro->SetPrinterPaper(-1.f);
	const FS47Doc Doc = S47Docs::Printout(ClockSec);
	DocsList.Add(Doc);
	SetPhase(ES47Phase::Ringing);
	bRingPending = true;
	OpenDoc(Doc, [this]
	{
		Note(TEXT("Source distance on the solve: -39 light years. A negative distance. The solver cannot produce that."));
		After(3.0, [this] { Ring(); });
	});
}

void AS47Story::Ring()
{
	if (!bRingPending) return;
	bRingPending = false;
	bRinging = true;
	if (Audio.IsValid()) Audio->Loop(TEXT("ring"), TEXT("phone_ring"), 1.0f, Saro.IsValid() ? Saro->PropLocation(TEXT("phone")) : FVector::ZeroVector);
	Toast(TEXT("The phone. At this hour."));
}

void AS47Story::Answer()
{
	bRinging = false;
	if (Audio.IsValid()) { Audio->Stop(TEXT("ring"), 0.02f); Audio->Play(TEXT("click"), 0.7f); }
	bAnswered = true;
	SetPhase(ES47Phase::Call);
	bCinematic = true;
	if (Saro.IsValid()) Saro->SetHandsetVisible(false);
	const double Dur = Audio.IsValid() ? Audio->FutureCall() : 6.0;
	After(0.15, [this] { Toast(TEXT("Static. Then a room. A printer running somewhere in it."), 2.4f); });
	After(2.0, [this] { Toast(TEXT("A deep thud. Somebody catches their breath."), 2.0f); });
	After(2.6, [this] { Toast(TEXT("Something ceramic breaks."), 1.8f); });
	After(Dur + 0.15, [this]
	{
		Toast(TEXT("Click. The line is dead."), 3.f);
		if (Saro.IsValid()) Saro->SetHandsetVisible(true);
		if (Audio.IsValid()) Audio->Play(TEXT("handset_down"), 0.5f, Saro.IsValid() ? Saro->PropLocation(TEXT("phone")) : FVector::ZeroVector);
		bCinematic = false;
		Note(FString::Printf(TEXT("%s. Outside line. No voice. Room tone, a printer, a heavy thud, a gasp, something ceramic breaking. Then nothing."), *ClockText(false)));
		StartCountdown();
	});
}

void AS47Story::StartCountdown()
{
	SetPhase(ES47Phase::Countdown);
	LineDeadT = GameTime;
	Log(TEXT("STATION CLOCK  WWVB REFERENCE LOST"));
	if (bRadioOn && Audio.IsValid()) Audio->AmRadioJam(true);
	After(40, [this] { TripK3(); });
	After(16, [this] { if (Audio.IsValid()) Audio->Play(TEXT("drone_52"), 0.3f); });
	After(31, [this] { if (Audio.IsValid()) Audio->Play(TEXT("printer"), 0.35f, Saro.IsValid() ? Saro->PropLocation(TEXT("printer")) : FVector::ZeroVector, 1.35f); });
	After(47, [this] { Event(); });
}

void AS47Story::Event()
{
	SetPhase(ES47Phase::Event);
	EventClock = ClockSec;
	Log(TEXT("LINE POWER  DIP 1.2 S"));
	Log(TEXT("STATION CLOCK  WWVB REFERENCE RESTORED  HOLDOVER 00:00:47"));
	if (bRadioOn) After(3.5, [this] { if (Audio.IsValid()) Audio->AmRadioJam(false); });
	if (Audio.IsValid()) { Audio->Boom(1.25f); }
	if (Hud.IsValid()) Hud->Shake(1.25f);
	Flicker = 1.8f;
	CrtGlitch = 0.5f;
	if (Saro.IsValid() && Saro->IsMugVisible()) { bMugFalling = true; Saro->DropMug(); }
	After(1.4, [this] { Toast(TEXT("The floor is still shaking."), 2.4f); });
	After(3.0, [this] { TurnDishes(); });
}

void AS47Story::TurnDishes()
{
	SetPhase(ES47Phase::Turning);
	bCinematic = true;
	LookAssist = 1.f;
	if (Audio.IsValid()) Audio->StartMotors();
	Log(TEXT("ARRAY  S-01 TO S-27 MOTION  NO COMMAND"));
	PointDishes(EventAz, EventEl, 2.2);
	Toast(TEXT("Outside, every dish in the array is turning."), 3.5f);
	After(4.5, [this] { Toast(TEXT("Nobody sent a command."), 3.f); });
}

void AS47Story::Arrived()
{
	SetPhase(ES47Phase::End);
	if (Audio.IsValid())
	{
		Audio->StopMotors();
		Audio->Play(TEXT("beep_1760_long"), 0.3f);
		After(0.3, [this] { if (Audio.IsValid()) Audio->Play(TEXT("beep_1760_long"), 0.3f); });
		After(0.6, [this] { if (Audio.IsValid()) Audio->Play(TEXT("beep_2350"), 0.3f); });
	}
	Note(TEXT("Every dish moved to az 026, el 32. Console shows no control command."));
	Note(TEXT("All dishes left their scheduled track and aligned together."));
	After(3.2, [this] { Finish(); });
}

void AS47Story::Finish()
{
	if (Audio.IsValid()) { Audio->SetCarrier(0, 0, 0); Audio->SignalOff(); }
	// main.ts game.onFinish: the night is not over
	if (Chapter(TEXT("ch1")))
	{
		ChapterBreak({ TEXT("NIGHT SHIFT // EVERY DISH AT AZ 026"),
			TEXT("The array moved without a command. The phone played the room before it happened. The night is not over."),
			TEXT("CHAPTER ONE"), TEXT("THE SECOND EXPOSURE"), [this] { BeginChapter(TEXT("ch1")); } });
		return;
	}
	if (Hud.IsValid()) Hud->Fade(true, TEXT(""));
	After(1.6, [this]
	{
		bCinematic = true;
		if (Hud.IsValid()) Hud->EndCard({ TEXT("PROLOGUE: NIGHT SHIFT"), TEXT(""), TEXT("Chapter 1, The Second Exposure, is not in the Unreal version yet."), TEXT("The web version plays on from here.") });
	});
}

// ---------- the chapters ----------

void AS47Story::AfterPrologue(const TSharedPtr<FJsonObject>& Saved)
{
	// Prologue.afterPrologue
	bLogRead = true; bAnswered = true;
	PowerUp(true);
	Rx.Advance(); Rx.Advance(); Rx.Advance();
	Rx.bSolved = true;
	bResidualVisible = true;
	ClockSec = S47J::Has(Saved, TEXT("clock")) ? S47J::Num(Saved, TEXT("clock")) : EventClock + 70;
	DocsList = { S47Docs::ShiftLog(), S47Docs::Printout(EventClock - 60) };
	NotesList.Empty();
	for (const FString& N : { FString(S47Signal::Profiles()[0].Note), FString(S47Signal::Profiles()[1].Note),
		FString(TEXT("02:13:47. Unlogged residual on the spectrum near 1420.4. Survey schedule says nothing should be there.")),
		FString(TEXT("02:14. Pulse group: four, a gap, seven. Repeats every 5.2 seconds. Not the relay. Not anything in the schedule.")),
		FString(TEXT("Direction solve: RA 05h 17m, Dec -05. Somewhere in Orion.")),
		FString(TEXT("Source distance on the solve: -39 light years. A negative distance. The solver cannot produce that.")),
		FString(TEXT("Outside line. No voice. Room tone, a printer, a heavy thud, a gasp, something ceramic breaking. Then nothing.")),
		FString(TEXT("Every dish moved to az 026, el 32. Console shows no control command.")),
		FString(TEXT("All dishes left their scheduled track and aligned together.")) }) Note(N);
	if (Saro.IsValid())
	{
		Saro->SetVisible(Saro->Handle(TEXT("room.mug")), false);
		Saro->SetPrinterPaper(-1.f);
		Saro->SnapDishes(EventAz, EventEl);
	}
	// Night Shift's papers, relay K3 and the console log: as saved, or as the night left them
	const S47J::FObj Ns = S47J::Obj(Saved, TEXT("ns"));
	const double E = S47J::Has(S47J::Obj(Saved, TEXT("s")), TEXT("eventClock")) ? S47J::Num(S47J::Obj(Saved, TEXT("s")), TEXT("eventClock")) : EventClock;
	NS = FS47NightState();
	if (Ns.IsValid())
	{
		NS.Papers = S47J::Strs(Ns, TEXT("papers"));
		NS.bK3Open = S47J::Bool(Ns, TEXT("k3Open"));
		for (const TSharedPtr<FJsonValue>& L : S47J::Arr(Ns, TEXT("log")))
		{
			const TArray<TSharedPtr<FJsonValue>>& A = L->AsArray();
			if (A.Num() >= 2) NS.Log.Add({ A[0]->AsNumber(), A[1]->AsString() });
		}
		if (S47J::Has(Ns, TEXT("solveAt"))) NS.SolveAt = S47J::Num(Ns, TEXT("solveAt"));
		NS.bJacket = S47J::Bool(Ns, TEXT("jacket"));
		// array: copyArraySave(saved.ns.array) (story/arrayBack.ts reads it through its store, which
		// copies it); hondo: Number.isFinite(saved.ns.hondo) ? saved.ns.hondo : undefined
		NS.Array = S47J::Obj(Ns, TEXT("array")).IsValid() ? S47J::Clone(S47J::Obj(Ns, TEXT("array"))) : nullptr;
		double Hondo = 0;
		if (Ns->TryGetNumberField(TEXT("hondo"), Hondo) && FMath::IsFinite(Hondo)) NS.Hondo = Hondo;
	}
	else
	{
		NS.SolveAt = E - 60;
		NS.Log = { { 23 * 3600 + 44 * 60, TEXT("RX BANK 3  K3 CLOSED") }, { E - 60, TEXT("SOLVE  RANGE NEGATIVE") },
			{ E - 47, TEXT("STATION CLOCK  WWVB REFERENCE LOST") }, { E - 7, TEXT("RX BANK 3  K3 OPEN") }, { E, TEXT("LINE POWER  DIP 1.2 S") },
			{ E, TEXT("STATION CLOCK  WWVB REFERENCE RESTORED  HOLDOVER 00:00:47") }, { E + 3, TEXT("ARRAY  S-01 TO S-27 MOTION  NO COMMAND") }, { E + 70, TEXT("RX BANK 3  K3 CLOSED") } };
	}
	// Prologue.paperById
	for (const FString& Id : NS.Papers)
	{
		if (Id == TEXT("binder")) DocsList.Add(S47Docs::Binder());
		else if (Id == TEXT("service")) DocsList.Add(S47Docs::ServiceRecord());
		else if (Id == TEXT("rfiCard")) DocsList.Add(S47Docs::InterferenceCard(Rx.Stage >= 3));
		else if (Id == TEXT("halley")) DocsList.Add(S47Docs::HalleyPoster());
		else if (Id == TEXT("telex")) DocsList.Add(S47Docs::Telex(TelexRoll()));
	}
	ApplyK3(NS.bK3Open);
	// (room.setTelexRoll: the telex is not ported yet)
	if (Saro.IsValid()) Saro->SetJacketVisible(!NS.bJacket);
	CurrentPhase = ES47Phase::End;
}

void AS47Story::DoorChanged(const FString& Id, bool bOpen)
{
	for (US47Chapter* C : Chapters) if (C) C->DoorChanged(Id, bOpen);
}

US47Chapter* AS47Story::Chapter(const FString& Key) const
{
	for (US47Chapter* C : Chapters) if (C && C->Key() == Key) return C;
	return nullptr;
}

void AS47Story::BeginChapter(const FString& Key, const TSharedPtr<FJsonObject>& Saved)
{
	US47Chapter* C = Chapter(Key);
	if (!C) { UE_LOG(LogTemp, Warning, TEXT("S47: no chapter %s"), *Key); return; }
	for (US47Chapter* O : Chapters) if (O && O != C) O->bActive = false;
	ChapterKey = Key;
	Begun.Add(Key);
	// Prologue.beginChapterN: the chapter's phase, no cinematic, nothing from before may fire inside it
	CurrentPhase = ES47Phase::Ch1;
	bCinematic = false;
	LookAssist = 0;
	Timers.Empty();
	C->bActive = true;
	C->Begin(Saved);
	UE_LOG(LogTemp, Display, TEXT("S47: chapter %s begins"), *Key);
	// beginChapterN ends on onCheckpoint('chapterN'), a new autosave generation (not chapter six)
	if (OnCheckpoint && Key != TEXT("ch6")) OnCheckpoint(TEXT("chapter") + Key.Mid(2));
}

void AS47Story::ChapterBreak(const FBreak& B)
{
	bCinematic = true;
	if (Hud.IsValid()) { Hud->CloseModal(); Hud->Fade(true, TEXT("")); }
	After(1.3, [this, B]
	{
		if (B.Begin) B.Begin();
		bCinematic = true;
		if (OnChapterCard) OnChapterCard(B);
		else if (Hud.IsValid()) Hud->Fade(true, B.Next + TEXT("\n") + B.Title);
		// the card stays at least 6.4 s from the break (the narrator may hold it longer)
		After(6.4, [this]
		{
			if (Hud.IsValid()) Hud->Fade(false, TEXT(""));
			bCinematic = false;
		});
	});
}

// ---------- lifecycle ----------

void AS47Story::ResetAll()
{
	Timers.Empty();
	GameTime = 0; ClockSec = StartClock;
	CurrentPhase = ES47Phase::Intro; bCinematic = false;
	bLogRead = false; Coffee = ECoffee::None; bPowered = false; bAnswered = false;
	NotesList.Empty(); DocsList.Empty();
	BootT = SolveT = PrintT = LineDeadT = -1;
	NS = FS47NightState();
	OpsLines.Empty();
	bRadioOn = false;
	bAlarmOn = false; bSkipQueued = false; bRingPending = false; bRinging = false;
	Flicker = 0; CrtGlitch = 0; bMugFalling = false; LookAssist = 0; DipT = 0;
	bDark = false;
	Rx.Reset();
	bResidualVisible = false;
	// Prologue.reset: the chapters too (ch1.reset() ... ch6.reset()); the night is the prologue's again
	for (US47Chapter* C : Chapters) if (C) { C->Reset(); C->bActive = false; }
	ChapterKey = TEXT("prologue");
	Begun.Empty();
	if (Saro.IsValid())
	{
		for (int32 i = 0; i < 3; ++i) Saro->SetCrtPowered(i, false);
		Saro->SetCrtGlow(0.f);
		Saro->SetLightsScale(1.f, 1.f);
		Saro->SetLeds(false);
		Saro->SetLever(false);
		Saro->SetPrinterPaper(-1.f);
		Saro->SetHandsetVisible(true);
		Saro->ResetMug();
		Saro->SetCoffee(false);
		Saro->SetCoffeeLevel(0.088);
		Saro->SetTubes(true);
		Saro->CloseAllDoors();
		Saro->SetJacketVisible(true);
		Saro->SnapDishes(ParkAz, SurveyEl);
	}
	if (Audio.IsValid()) { Audio->Stop(TEXT("ring")); Audio->Stop(TEXT("printer")); Audio->SetCarrier(0, 0, 0); Audio->SignalOff(); Audio->StopMotors(); }
}

void AS47Story::Start()
{
	ResetAll();
	if (Hud.IsValid() && Saro.IsValid()) { const FVector Sp = Saro->SpawnThree(); Hud->Place(Sp.X, Sp.Z, Sp.Y); }
	After(1.2, [this] { Toast(TEXT("23:41. Dale left twenty minutes ago."), 3.4f); });
	After(5.0, [this] { if (!bLogRead) Toast(TEXT("His shift log is on the desk."), 3.4f); });
}

void AS47Story::Jump(const FString& Target)
{
	ResetAll();
	if (const TFunction<void()>* J = Jumps.Find(Target))
	{
		(*J)();
		UE_LOG(LogS47Story, Display, TEXT("S47: jumped to %s"), *Target);
		return;
	}
	static const TCHAR* Steps[] = { TEXT("shift"), TEXT("residual"), TEXT("locked"), TEXT("printed"), TEXT("countdown"), TEXT("event") };
	int32 Idx = -1;
	for (int32 i = 0; i < 6; ++i) if (Target == Steps[i]) Idx = i;
	if (Idx < 0) return;
	bLogRead = true; DocsList = { S47Docs::ShiftLog() }; CurrentPhase = ES47Phase::Shift;
	if (Idx >= 1)
	{
		PowerUp(true);
		NS.Log.Add({ 23 * 3600 + 44 * 60, TEXT("RX BANK 3  K3 CLOSED") });
		Rx.Advance(); Rx.Advance();
		Note(S47Signal::Profiles()[0].Note); Note(S47Signal::Profiles()[1].Note);
		ClockSec = SkipClock;
		PointDishes(Rx.Azimuth, SurveyEl, 99);
		if (Hud.IsValid()) Hud->Place(0.05, -2.2, 0);
		EnterResidual();
		Toast(TEXT("02:13. The survey has been running for hours."), 3.4f);
	}
	if (Idx >= 2)
	{
		Cancel(TEXT("alarm"));
		Timers.Empty();
		bResidualVisible = true;
		Rx.Advance();
		PointDishes(Rx.Azimuth, SurveyEl, 99);
		ClockSec = SkipClock + 40;
		CurrentPhase = ES47Phase::Locked;
	}
	if (Idx >= 3)
	{
		Rx.bSolved = true;
		if (Saro.IsValid()) Saro->SetPrinterPaper(1.f);
		CurrentPhase = ES47Phase::Printed;
		if (Hud.IsValid()) Hud->Place(4.6, -0.4, PI / 2);
	}
	if (Idx >= 4)
	{
		if (Saro.IsValid()) Saro->SetPrinterPaper(-1.f);
		DocsList.Add(S47Docs::Printout(ClockSec));
		bAnswered = true;
		if (Hud.IsValid()) Hud->Place(2.4, 1.6, PI);
		StartCountdown();
		// testing shortcut: the full 47 seconds only run in a real playthrough
		if (Idx == 4) { Timers.RemoveAll([this](const FTimer& T) { return T.At - GameTime >= 40; }); After(4, [this] { Event(); }); }
	}
	if (Idx >= 5) { Timers.Empty(); Event(); }
	UE_LOG(LogS47Story, Display, TEXT("S47: jumped to %s, phase %d"), *Target, (int32)CurrentPhase);
}

// ---------- per frame ----------

void AS47Story::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// main.ts step(): game.update runs only in play (not behind the title, not on the end card)
	if (const US47Main* Main = US47Main::Get(this); Main && Main->Mode != US47Main::EMode::Play) return;
	const double Dt = DeltaSeconds;
	if (!bPaused)
	{
		GameTime += Dt;
		ClockSec += Dt;
		TArray<FTimer> Due;
		for (int32 i = Timers.Num() - 1; i >= 0; --i)
		{
			if (Timers[i].At <= GameTime) { Due.Add(MoveTemp(Timers[i])); Timers.RemoveAt(i); }
		}
		Due.Sort([](const FTimer& A, const FTimer& B) { return A.At < B.At; });
		for (FTimer& T : Due) T.Fn();
	}
	// Prologue.update: the chapters (ch1 runs in every chapter: the viewfinder, the wet print)
	for (US47Chapter* C : Chapters)
	{
		if (!C) continue;
		const bool bCh1Along = C->Key() == TEXT("ch1") && ChapterKey.StartsWith(TEXT("ch"));
		if (C->bActive || bCh1Along) C->Update(bPaused ? 0.0 : Dt, GameTime);
	}
	if (!Saro.IsValid()) return;
	Saro->SetClock(ClockSec);

	// printer paper feeding out
	if (CurrentPhase == ES47Phase::Printing) Saro->SetPrinterPaper(FMath::Clamp(float((GameTime - PrintT) / 5.2), 0.001f, 1.f));
	// phone handset rattle while ringing (this.ringing || this.ch2.ringing)
	const US47Chapter2* C2 = Cast<US47Chapter2>(Chapter(TEXT("ch2")));
	Saro->SetHandsetRattle(bRinging || (C2 && C2->bRinging));

	// receiver tone
	if (bPowered && !NS.bK3Open && Audio.IsValid())
	{
		if (At(ES47Phase::End)) { Audio->SetCarrier(0, 0, 0); Audio->SignalOff(); }
		else if (Rx.Stage >= 3 || (Rx.Stage == 2 && bResidualVisible))
		{
			const double C = Rx.Stage >= 3 ? 1.0 : Rx.Quality(S47Signal::Profiles()[2]);
			Audio->SetCarrier(0, 0, 0);
			Audio->SignalFrame(float(FMath::Max(0.02, C)), bConsoleOpen ? 0.9f : 0.3f);
		}
		else
		{
			const double Q = SurveyOnly() ? 0.05 : Rx.Quality();
			Audio->SetCarrier(bConsoleOpen ? float(0.06 * Q * Q) : 0.f, float((Q - 1) * 0.03), bConsoleOpen ? float(0.03 * (1 - Q) + 0.008) : 0.003f);
		}
	}

	// rack LEDs flicker a little when live
	if (bPowered && !NS.bK3Open)
	{
		LedT -= Dt;
		if (LedT <= 0) { LedT = 0.18f; Saro->FlickerLed(); }
	}
	// light flicker after the boom
	if (Flicker > 0)
	{
		Flicker -= Dt;
		const bool bOn = Flicker <= 0 || FMath::FRand() > 0.45f;
		Saro->SetLightsScale(bOn ? 0.6f + FMath::FRand() * 0.5f : 0.2f / 9.f, 1.f);
		Saro->SetTubes(bOn, bOn ? -1 : 2);
		if (Flicker <= 0) { Saro->SetLightsScale(1.f, 1.f); Saro->SetTubes(true, 3); }
	}
	// the lights dip forty seconds after the relay: the room sags for most of a second
	if (DipT > 0)
	{
		DipT -= Dt;
		const bool bOn = DipT <= 0;
		Saro->SetLightsScale(bOn ? 1.f : 0.24f, bOn ? 1.f : 0.35f);
		Saro->SetTubes(bOn);
	}
	if (CrtGlitch > 0) CrtGlitch -= Dt;

	// dishes arrive
	if (CurrentPhase == ES47Phase::Turning && !Saro->DishesMoving()) Arrived();

	// gentle camera assist so the player sees the array turn
	if (LookAssist > 0 && (CurrentPhase == ES47Phase::Turning || CurrentPhase == ES47Phase::End) && Hud.IsValid())
	{
		Hud->LookTowards(FVector(-6, 9, -70), float(1.0 - FMath::Exp(-Dt * 1.4)));
	}
}

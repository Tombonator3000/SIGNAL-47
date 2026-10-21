// SIGNAL / 47: the papers of Night Shift and the Prologue. See S47Docs.h.

#include "S47Docs.h"

// ---------- time.ts ----------

namespace S47Text
{
	FString PadStart(const FString& S, int32 W, TCHAR Fill)
	{
		if (S.Len() >= W) return S;
		FString Out;
		Out.Reserve(W);
		for (int32 i = S.Len(); i < W; i++) Out.AppendChar(Fill);
		Out += S;
		return Out;
	}

	FString PadEnd(const FString& S, int32 W, TCHAR Fill)
	{
		FString Out = S;
		while (Out.Len() < W) Out.AppendChar(Fill);
		return Out;
	}

	TArray<FString> Split(const FString& S, const FString& Sep)
	{
		TArray<FString> Out;
		int32 Start = 0;
		for (;;)
		{
			const int32 At = S.Find(Sep, ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
			if (At == INDEX_NONE) { Out.Add(S.Mid(Start)); return Out; }
			Out.Add(S.Mid(Start, At - Start));
			Start = At + Sep.Len();
		}
	}

	FString Pad(double N, int32 W)
	{
		const int64 I = static_cast<int64>(FMath::Floor(N));
		return PadStart(FString::Printf(TEXT("%lld"), I), W, TEXT('0'));
	}

	FString AngleText(double V, int32 W)
	{
		const double A = FMath::Abs(V);
		const double T = FMath::Floor(A * 10.0 + 1e-6) / 10.0;
		const int32 Tenth = static_cast<int32>(FMath::Floor(FMath::Fmod(T, 1.0) * 10.0 + 0.5)) % 10;
		return FString::Printf(TEXT("%s%s.%d"), (V < 0 && T > 0) ? TEXT("-") : TEXT(""), *Pad(FMath::Floor(T), W), Tenth);
	}

	FString ClockText(double Sec, bool bWithSeconds)
	{
		const double S = FMath::Fmod(FMath::Fmod(Sec, 86400.0) + 86400.0, 86400.0);
		const double H = FMath::Floor(S / 3600.0);
		const double M = FMath::Fmod(FMath::Floor(S / 60.0), 60.0);
		return bWithSeconds
			? FString::Printf(TEXT("%s:%s:%s"), *Pad(H), *Pad(M), *Pad(FMath::Fmod(S, 60.0)))
			: FString::Printf(TEXT("%s:%s"), *Pad(H), *Pad(M));
	}
}

using namespace S47Text;

// ---------- Prologue.ts ----------

// Dale's shift log, torn out of the night shift logbook.
static const TCHAR* const LOG_PAGE =
	TEXT("23:20  handover\n")
	TEXT("\n")
	TEXT("Kid,\n")
	TEXT("1. RX bank 3 tripped again. Big red lever on the rack by the printer.\n")
	TEXT("2. Calibrate on 1419.900 first. Card is taped to the console.\n")
	TEXT("3. Spike near 1420.1 is the highway relay. Notch it, log it, don't chase it.\n")
	TEXT("4. Then let the survey sweep run. Nothing scheduled till 06:00. Babysit it.\n")
	TEXT("\n")
	TEXT("Coffee is fresh-ish.\n")
	TEXT("Storm out past the Magdalenas. Should stay there.\n")
	TEXT("\n")
	TEXT("Don't break anything.\n")
	TEXT("        D.");
static const TCHAR* const LOG_TRANSCRIPT =
	TEXT("Torn out of the night shift logbook. Dale writes like he is already halfway to his car.\n")
	TEXT("\n")
	TEXT("\"Kid. One: RX bank 3 tripped again. Big red lever on the rack by the printer. Two: calibrate on 1419.900 first. Card is taped to the console. Three: the spike near 1420.1 is the highway relay. Notch it, log it, don't chase it. Four: then let the survey sweep run. Nothing scheduled till 06:00. Babysit it.\"\n")
	TEXT("\n")
	TEXT("\"Coffee is fresh-ish. Storm out past the Magdalenas. Should stay there. Don't break anything. D.\"");

// Under Dale's log: what the night is for. Ward's note is her opening line from the design bible (V1).
static const TCHAR* const WORK_ORDER_PAGE =
	TEXT("SARO / OPERATIONS\n")
	TEXT("NIGHT WORK ORDER   04/13/86\n")
	TEXT("\n")
	TEXT("OPERATOR:  REYES\n")
	TEXT("ON CALL:   DR. E. WARD, SHIFT SUPERVISOR\n")
	TEXT("\n")
	TEXT("1. RESTORE RX BANK 3.\n")
	TEXT("   CALIBRATE ON 1419.900 MHZ.\n")
	TEXT("2. RUN THE SURVEY SWEEP.\n")
	TEXT("   LOG ANYTHING OFF SCHEDULE.\n")
	TEXT("3. MORNING SERIES 06:00.\n")
	TEXT("   IT DOES NOT RUN UNTIL EVERY\n")
	TEXT("   ANOMALY TONIGHT IS DOCUMENTED\n")
	TEXT("   AND SIGNED.");
static const TCHAR* const WORK_ORDER_TRANSCRIPT =
	TEXT("A typed work order with your name on it, initialled in the corner by the shift supervisor.\n")
	TEXT("\n")
	TEXT("Along the bottom, in Ward's handwriting: \"Before the morning series, I need a report I can sign. Start with calibration. Keep the paper if it gives you anything you cannot account for. E.W.\"");

static const TCHAR* const PRINTOUT_TRANSCRIPT =
	TEXT("Fan-fold paper, still warm from the print head.\n")
	TEXT("\n")
	TEXT("Everything above the last lines reads like an ordinary solve: the hydrogen line, a pulse pattern, a position in Orion.\n")
	TEXT("\n")
	TEXT("The distance field is negative. Minus thirty-nine light years. The solver has no way of printing a negative distance. It was never written to.");

// ---------- nightshift.ts ----------

// The binder's sheets: Systems' note, the three printouts the solver made before tonight, and
// the last one.
static const TCHAR* const BINDER_NOTE =
	TEXT("SARO RX/DSP\n")
	TEXT("DIRECTION SOLVE - EXCEPTIONS\n")
	TEXT("FILE COPIES. DO NOT REMOVE.\n")
	TEXT("\n")
	TEXT("FROM SYSTEMS, 03/84:\n")
	TEXT("NEGATIVE RANGES CANNOT BE GENERATED\n")
	TEXT("BY THE SOLVER. RANGE IS COMPUTED FROM\n")
	TEXT("DELAY AND IS UNSIGNED. ANY NEGATIVE\n")
	TEXT("RANGE IS AN INPUT ERROR.\n")
	TEXT("RECLASSIFY AND FILE HERE.\n")
	TEXT("~ Then why does it keep doing it?");
static const TCHAR* const BINDER_PRINT_1 =
	TEXT("DATE    09/22/81   01:52:07\n")
	TEXT("FREQ    1420.405 MHz\n")
	TEXT("PATTERN PULSE GROUP 4 / 7\n")
	TEXT("SOURCE DISTANCE:  -34 LY\n")
	TEXT("** CHECK SOLVE INPUTS **\n")
	TEXT("RECLASSIFIED: INPUT ERROR\n")
	TEXT("~ clock?");
static const TCHAR* const BINDER_PRINT_2 =
	TEXT("DATE    03/02/83   03:10:44\n")
	TEXT("FREQ    1420.405 MHz\n")
	TEXT("PATTERN PULSE GROUP 4 / 7\n")
	TEXT("SOURCE DISTANCE:  -36 LY\n")
	TEXT("** CHECK SOLVE INPUTS **\n")
	TEXT("RECLASSIFIED: INPUT ERROR\n")
	TEXT("~ baseline file reloaded");
static const TCHAR* const BINDER_PRINT_3 =
	TEXT("DATE    10/19/85   23:58:31\n")
	TEXT("FREQ    1420.405 MHz\n")
	TEXT("PATTERN PULSE GROUP 4 / 7\n")
	TEXT("SOURCE DISTANCE:  -38 LY\n")
	TEXT("** CHECK SOLVE INPUTS **\n")
	TEXT("RECLASSIFIED: INPUT ERROR\n")
	TEXT("~ third time. Ask Systems.");
static const TCHAR* const BINDER_1947 =
	TEXT("DATE    07/--/47   02:17:00\n")
	TEXT("FREQ    1420.405 MHz\n")
	TEXT("PATTERN PULSE GROUP 4 / 7\n")
	TEXT("SOURCE DISTANCE:  -00 LY");
static const TCHAR* const RULE = TEXT("\n\n- - - - - - - - - - - - - - - - - -\n");
static const TCHAR* const BINDER_TRANSCRIPT =
	TEXT("A ring binder from the shelf above the printer. The note from Systems is typed. The pencil under it looks like the hand in the shift log.\n")
	TEXT("\n")
	TEXT("Three printouts in the same format as tonight's, from September 1981, March 1983 and October 1985. Each one is stamped as an input error.\n")
	TEXT("\n")
	TEXT("The last sheet is on older, yellowed paper, with no stamp and no pencil. The date field says 1947. SARO's solver did not exist in 1947.");

static const TCHAR* const SERVICE_PAGE =
	TEXT("SARO / MAINTENANCE\n")
	TEXT("RX BANK 3 - SERVICE RECORD\n")
	TEXT("\n")
	TEXT("09/23/81  K3 RELAY REPLACED. TRIPPED\n")
	TEXT("          APPROX 1 MIN BEFORE LINE POWER\n")
	TEXT("          FAILURE 09/22. NO FAULT FOUND.\n")
	TEXT("\n")
	TEXT("03/03/83  K3 REPLACED. TRIPPED BEFORE\n")
	TEXT("          POWER DIP 03/02. RELAY TESTS\n")
	TEXT("          GOOD. REPLACED ANYWAY.\n")
	TEXT("\n")
	TEXT("10/21/85  K3 REPLACED (3RD). TRIPPED\n")
	TEXT("          40 SEC BEFORE OUTAGE 10/19.\n")
	TEXT("          RELAY IS NOT FAULTY.\n")
	TEXT("          IT TRIPS BEFORE, NOT AFTER.\n")
	TEXT("                              M.O.");
static const TCHAR* const SERVICE_TRANSCRIPT =
	TEXT("A clipboard on the rack, under the status lights. Maintenance has replaced relay K3 three times since 1981.\n")
	TEXT("\n")
	TEXT("Each entry is the day after the trip. Each time, the relay tripped first and the power went after.");

static const TCHAR* const RFI_CARD_PAGE =
	TEXT("CALIBRATION\n")
	TEXT("1419.900 MHZ  GAIN 45-65  BW 34-62\n")
	TEXT("\n")
	TEXT("KNOWN INTERFERENCE\n")
	TEXT("1420.110  HIGHWAY MICROWAVE RELAY.\n")
	TEXT("          NOTCH.\n")
	TEXT("1420.6    SATELLITE DOWNLINK,\n")
	TEXT("          PASSES ONLY.\n")
	TEXT("\n")
	TEXT("ANYTHING ELSE: LOG IT.");
static const TCHAR* const RFI_CARD_TRANSCRIPT =
	TEXT("An index card, taped to the console so long ago the tape has gone brown.");
static const TCHAR* const RFI_CARD_LOCKED =
	TEXT("\n\nA pulse pattern of four and seven is not on it.");

static const TCHAR* const HALLEY_PAGE =
	TEXT("HALLEY'S COMET\n")
	TEXT("APRIL 1986\n")
	TEXT("\n")
	TEXT("CLOSEST TO EARTH APRIL 11\n")
	TEXT("LOOK LOW IN THE SOUTH\n")
	TEXT("EARLY APRIL: BEFORE DAWN\n")
	TEXT("AFTER THE 12TH: AROUND MIDNIGHT\n")
	TEXT("GET AWAY FROM TOWN LIGHTS\n")
	TEXT("~ Public line: comet calls go to the planetarium in town. Not us. Not at 3 a.m.  E.W.");
static const TCHAR* const HALLEY_TRANSCRIPT =
	TEXT("A printed poster from the spring, with a little chart of the southern sky and the comet's place for the 5th, 10th, 14th and 20th of April.\n")
	TEXT("\n")
	TEXT("A note is taped in the corner, in the supervisor's hand.");

// The telex roll. Night Shift only has last evening's traffic on it; later chapters add their
// messages to the same roll.
static const TCHAR* const TELEX_WX =
	TEXT("ZCZC WX2250\n")
	TEXT("SARO OPS\n")
	TEXT("WX ADVISORY 2250 MST 13 APR 86\n")
	TEXT("TSTMS W OF MAGDALENA MTNS MOVG NE 15 KT.\n")
	TEXT("DRY LIGHTNING. NO PRECIP E OF RIO GRANDE.\n")
	TEXT("NNNN");
static const TCHAR* const TELEX_NET =
	TEXT("ZCZC NET2300\n")
	TEXT("ALL SITES\n")
	TEXT("NET STATUS 2300 MST\n")
	TEXT("SITE 03 - NORMAL\n")
	TEXT("SITE 11 - NORMAL\n")
	TEXT("SARO - NORMAL\n")
	TEXT("ALL OTHER SITES NORMAL\n")
	TEXT("NNNN");
static const TCHAR* const TELEX_TRANSCRIPT_HONDO =
	TEXT("The roll on the teleprinter in the corner. Above the tear, a weather advisory at 22:50 and the network status at 23:00. Below it, printed when the power came back, one more: Hondo Valley.");
static const TCHAR* const TELEX_TRANSCRIPT_EVENING =
	TEXT("The roll on the teleprinter in the corner, torn off below the last message. A weather advisory at 22:50, and the network status at 23:00.");

static FString Join(const TArray<FString>& Parts, const TCHAR* Sep)
{
	return FString::Join(Parts, Sep);
}

namespace S47Docs
{
	FS47Doc ShiftLog()
	{
		return { TEXT("log"), TEXT("Dale's shift log"), TEXT("hand"), TEXT("NIGHT SHIFT"), LOG_PAGE, LOG_TRANSCRIPT };
	}

	FS47Doc WorkOrder()
	{
		return { TEXT("workorder"), TEXT("Night work order"), TEXT("typed"), TEXT("WORK ORDER"), WORK_ORDER_PAGE, WORK_ORDER_TRANSCRIPT };
	}

	FS47Doc Printout(double Clock)
	{
		const TArray<FString> Lines = {
			TEXT("SARO RX/DSP   DIRECTION SOLVE"),
			TEXT("------------------------------"),
			FString::Printf(TEXT("DATE    04/14/86   %s"), *ClockText(Clock)),
			TEXT("FREQ    1420.405 MHz"),
			TEXT("BW      12 kHz"),
			TEXT("PATTERN PULSE GROUP 4 / 7"),
			TEXT("RA      05h 17m 32s"),
			TEXT("DEC    -05  23' 14\""),
			TEXT("S/N     4.71"),
			TEXT("SOURCE  UNKNOWN"),
			TEXT(""),
			TEXT("SOURCE DISTANCE:  -39 LY"),
			TEXT(""),
			TEXT("** CHECK SOLVE INPUTS **"),
		};
		return { TEXT("printout"), TEXT("Direction solve printout"), TEXT("printout"), TEXT(""), Join(Lines, TEXT("\n")), PRINTOUT_TRANSCRIPT };
	}

	FS47Doc Binder()
	{
		const TArray<FString> Sheets = { BINDER_NOTE, BINDER_PRINT_1, BINDER_PRINT_2, BINDER_PRINT_3, BINDER_1947 };
		return { TEXT("binder"), TEXT("Solver exceptions binder"), TEXT("typed"), TEXT("EXCEPTIONS"), Join(Sheets, RULE), BINDER_TRANSCRIPT };
	}

	FS47Doc ServiceRecord()
	{
		return { TEXT("service"), TEXT("RX bank 3 / Service record"), TEXT("typed"), TEXT("MAINTENANCE"), SERVICE_PAGE, SERVICE_TRANSCRIPT };
	}

	FS47Doc InterferenceCard(bool bLocked)
	{
		FString Transcript = RFI_CARD_TRANSCRIPT;
		if (bLocked) Transcript += RFI_CARD_LOCKED;
		return { TEXT("rfiCard"), TEXT("Dale's card on the console"), TEXT("typed"), TEXT("RX 3"), RFI_CARD_PAGE, Transcript };
	}

	FS47Doc HalleyPoster()
	{
		return { TEXT("halley"), TEXT("Halley's comet poster"), TEXT("typed"), TEXT("1986"), HALLEY_PAGE, HALLEY_TRANSCRIPT };
	}

	TArray<FString> TelexEvening()
	{
		return { TELEX_WX, TELEX_NET };
	}

	FS47Doc Telex(const TArray<FString>& Messages)
	{
		// (last evening's roll keeps its words exactly: the read-aloud voices have them)
		const bool bHondo = Messages.Num() > TelexEvening().Num();
		return { TEXT("telex"), TEXT("Telex roll"), TEXT("typed"), TEXT("TELEX"), Join(Messages, TEXT("\n\n")),
			bHondo ? TELEX_TRANSCRIPT_HONDO : TELEX_TRANSCRIPT_EVENING };
	}

	// Hondo Valley's message, filed at the minute SARO went dark, printed on the roll when the
	// power came back.
	static FString HHMM(double Clock)
	{
		return ClockText(Clock, false).Replace(TEXT(":"), TEXT(""));
	}

	FString TelexHondo(double Filed)
	{
		return FString::Printf(TEXT("ZCZC HVR%s\nSARO OPS\nFM HONDO VALLEY RADIO STN\n%s MST 14 APR 86\nRANGE SOLUTION NEGATIVE. REQUEST PROCEDURE.\nNNNN"),
			*HHMM(Filed), *HHMM(Filed));
	}

	FString HondoNote(double Filed)
	{
		return FString::Printf(TEXT("%s. Telex from Hondo Valley Radio Station: RANGE SOLUTION NEGATIVE. REQUEST PROCEDURE. Somebody else got one tonight, and they are asking us what to do."),
			*ClockText(Filed, false));
	}

	// The roll as it hangs out of the machine: its lines, the newest at the bottom by the platen.
	TArray<FString> TelexLines(const TArray<FString>& Messages)
	{
		return Split(Join(Messages, TEXT("\n\n")), TEXT("\n"));
	}

	// ---------- self test ----------

	static void Check(TArray<FString>& Out, bool& bOk, const TCHAR* Name, const FString& Got, const FString& Want)
	{
		if (Got == Want) { Out.Add(FString::Printf(TEXT("ok  %s"), Name)); return; }
		bOk = false;
		Out.Add(FString::Printf(TEXT("FAIL %s: got %s, want %s"), Name, *Got.ReplaceCharWithEscapedChar(), *Want.ReplaceCharWithEscapedChar()));
	}

	bool SelfTest(TArray<FString>& Out)
	{
		bool bOk = true;
		// time.ts (values from running the TypeScript)
		Check(Out, bOk, TEXT("clockText(23:41:07)"), ClockText(23 * 3600 + 41 * 60 + 7), TEXT("23:41:07"));
		Check(Out, bOk, TEXT("clockText(02:16, no seconds)"), ClockText(2 * 3600 + 16 * 60, false), TEXT("02:16"));
		Check(Out, bOk, TEXT("clockText(-1)"), ClockText(-1), TEXT("23:59:59"));
		Check(Out, bOk, TEXT("angleText(26.0, 3)"), AngleText(26.0, 3), TEXT("026.0"));
		Check(Out, bOk, TEXT("angleText(-0.3)"), AngleText(-0.3), TEXT("-00.3"));
		Check(Out, bOk, TEXT("angleText(-0.04)"), AngleText(-0.04), TEXT("00.0"));
		Check(Out, bOk, TEXT("pad(7)"), Pad(7), TEXT("07"));
		Check(Out, bOk, TEXT("pad(1311, 3)"), Pad(1311, 3), TEXT("1311"));
		Check(Out, bOk, TEXT("padStart(968, 6)"), PadStart(TEXT("968"), 6), TEXT("   968"));
		Check(Out, bOk, TEXT("padEnd(name, 20)"), PadEnd(TEXT("RUN860411_2340.DAT"), 20), TEXT("RUN860411_2340.DAT  "));

		// Prologue
		const FS47Doc Log = ShiftLog();
		Check(Out, bOk, TEXT("LOG first line"), Split(Log.Page, TEXT("\n"))[0], TEXT("23:20  handover"));
		Check(Out, bOk, TEXT("LOG last line"), Split(Log.Page, TEXT("\n")).Last(), TEXT("        D."));
		Check(Out, bOk, TEXT("LOG title"), Log.Title, TEXT("Dale's shift log"));
		const FS47Doc Print = Printout(2 * 3600 + 14 * 60 + 30);
		Check(Out, bOk, TEXT("printout distance"), Print.Page.Contains(TEXT("SOURCE DISTANCE:  -39 LY")) ? TEXT("yes") : TEXT("no"), TEXT("yes"));
		Check(Out, bOk, TEXT("printout date line"), Split(Print.Page, TEXT("\n"))[2], TEXT("DATE    04/14/86   02:14:30"));
		Check(Out, bOk, TEXT("printout DEC line"), Split(Print.Page, TEXT("\n"))[7], TEXT("DEC    -05  23' 14\""));
		Check(Out, bOk, TEXT("printout line count"), FString::FromInt(Split(Print.Page, TEXT("\n")).Num()), TEXT("14"));
		Check(Out, bOk, TEXT("work order first line"), Split(WorkOrder().Page, TEXT("\n"))[0], TEXT("SARO / OPERATIONS"));

		// Night Shift
		const FS47Doc Bind = Binder();
		Check(Out, bOk, TEXT("binder rules"), FString::FromInt(Split(Bind.Page, RULE).Num()), TEXT("5"));
		Check(Out, bOk, TEXT("binder last sheet"), Split(Bind.Page, RULE)[4], BINDER_1947);
		Check(Out, bOk, TEXT("binder line count"), FString::FromInt(Split(Bind.Page, TEXT("\n")).Num()), TEXT("44"));
		Check(Out, bOk, TEXT("service last line"), Split(ServiceRecord().Page, TEXT("\n")).Last(), TEXT("                              M.O."));
		Check(Out, bOk, TEXT("rfi card locked"), InterferenceCard(true).Transcript,
			TEXT("An index card, taped to the console so long ago the tape has gone brown.\n\nA pulse pattern of four and seven is not on it."));
		Check(Out, bOk, TEXT("rfi card open"), InterferenceCard(false).Transcript,
			TEXT("An index card, taped to the console so long ago the tape has gone brown."));
		Check(Out, bOk, TEXT("halley stamp"), HalleyPoster().Stamp, TEXT("1986"));

		// Telex
		const TArray<FString> Evening = TelexEvening();
		Check(Out, bOk, TEXT("telex evening lines"), FString::FromInt(TelexLines(Evening).Num()), TEXT("15"));
		Check(Out, bOk, TEXT("telex evening transcript"), Telex(Evening).Transcript, TELEX_TRANSCRIPT_EVENING);
		TArray<FString> WithHondo = Evening;
		WithHondo.Add(TelexHondo(2 * 3600 + 29 * 60));
		Check(Out, bOk, TEXT("telex hondo transcript"), Telex(WithHondo).Transcript, TELEX_TRANSCRIPT_HONDO);
		Check(Out, bOk, TEXT("telexHondo(02:29)"), TelexHondo(2 * 3600 + 29 * 60),
			TEXT("ZCZC HVR0229\nSARO OPS\nFM HONDO VALLEY RADIO STN\n0229 MST 14 APR 86\nRANGE SOLUTION NEGATIVE. REQUEST PROCEDURE.\nNNNN"));
		Check(Out, bOk, TEXT("HONDO_NOTE(02:29)"), HondoNote(2 * 3600 + 29 * 60),
			TEXT("02:29. Telex from Hondo Valley Radio Station: RANGE SOLUTION NEGATIVE. REQUEST PROCEDURE. Somebody else got one tonight, and they are asking us what to do."));
		Check(Out, bOk, TEXT("telex page join"), Telex(Evening).Page, FString(TELEX_WX) + TEXT("\n\n") + TELEX_NET);
		return bOk;
	}
}

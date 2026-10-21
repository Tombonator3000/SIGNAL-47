// SIGNAL / 47: the operations terminal. See S47Ops.h.

#include "S47Ops.h"
#include "S47Docs.h"

using namespace S47Text;

// Last evening, before the shift: already in the console log when the night begins.
static const FS47LogLine SEEDS[] = {
	{ 22 * 3600 + 50 * 60, TEXT("TELEX RECEIVED  WX") },
	{ 23 * 3600,           TEXT("TELEX RECEIVED  NET STATUS") },
	{ 23 * 3600 + 14 * 60, TEXT("RX BANK 3  K3 OPEN") },
	{ 23 * 3600 + 15 * 60, TEXT("LINE POWER  DIP 0.4 S") },
	{ 23 * 3600 + 41 * 60, TEXT("SURVEY RUN OPENED  RUN860413_2341") },
};
static constexpr int32 NUM_SEEDS = UE_ARRAY_COUNT(SEEDS);

static const TCHAR* const MON[] = { TEXT("JAN"), TEXT("FEB"), TEXT("MAR"), TEXT("APR"), TEXT("MAY"), TEXT("JUN"),
	TEXT("JUL"), TEXT("AUG"), TEXT("SEP"), TEXT("OCT"), TEXT("NOV"), TEXT("DEC") };

static constexpr double START = 23 * 3600 + 41 * 60;   // the survey run opened

static double Wrap(double Clock)
{
	return FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
}

// The evening side of midnight is the 13th, the morning side the 14th.
int32 FS47OpsTerminal::Day(double Clock)
{
	return Wrap(Clock) >= 12 * 3600 ? 13 : 14;
}

FString FS47OpsTerminal::Stamp(double Clock, bool bSecs)
{
	return FString::Printf(TEXT("%d-APR %s"), Day(Clock), *ClockText(Clock, bSecs));
}

// Blocks written to tonight's run so far: one every 9.6 s since 23:41.
int32 FS47OpsTerminal::RunBlocks(double Clock)
{
	const double S = Wrap(Clock);
	const double Since = S >= 12 * 3600 ? S - START : S + 86400 - START;
	return FMath::Max(1, static_cast<int32>(FMath::Floor(Since / 9.6)));
}

// Reel dates: the commissioning series was one reel a night from 25 June 1979, then the
// numbering runs on evenly to reel 1311, mounted the evening of 13 April 1986.
// (The same arithmetic as JavaScript's Date.UTC, in milliseconds.)
FString FS47OpsTerminal::ReelDate(int32 N)
{
	const FDateTime Epoch(1970, 1, 1);
	const double Base = (FDateTime(1979, 6, 25) - Epoch).GetTotalMilliseconds();
	const double Late = (FDateTime(1986, 4, 13) - Epoch).GetTotalMilliseconds();
	const double T = N <= 100
		? Base + (N - 1) * 864e5
		: Base + 99 * 864e5 + (N - 100) * ((Late - Base - 99 * 864e5) / 1211.0);
	const FDateTime D = Epoch + FTimespan(static_cast<int64>(T) * ETimespan::TicksPerMillisecond);
	return FString::Printf(TEXT("%s-%s-%s"), *Pad(D.GetDay()), MON[D.GetMonth() - 1], *FString::FromInt(D.GetYear()).RightChop(2));
}

FS47OpsTerminal::FS47OpsTerminal(TFunction<FS47OpsView()> InView)
	: View(MoveTemp(InView))
{
}

FString FS47OpsTerminal::Header() const
{
	const FS47OpsView V = View();
	return FString::Printf(TEXT("SARO OPERATIONS SYSTEM   V4.2        %d-APR-1986 %s\nTERMINAL 2   OPERATOR REYES"), Day(V.Clock), *ClockText(V.Clock, false));
}

TArray<FString> FS47OpsTerminal::Buttons() const
{
	TArray<FString> Out = { TEXT("DIR SURVEY.RAW"), TEXT("SHOW EXCEPTIONS"), TEXT("SHOW TAPES"), TEXT("SHOW LOG"), TEXT("HELP") };
	if (More.IsSet() && More->Buttons) Out.Append(More->Buttons());
	Out.Append(Extra);
	return Out;
}

// SHOW TAPES (\d{1,4})\s*-\s*(\d{1,4}) on a command whose whitespace is already single spaces.
static bool ParseTapeRange(const FString& Cmd, int32& A, int32& B)
{
	const FString Prefix = TEXT("SHOW TAPES ");
	if (!Cmd.StartsWith(Prefix, ESearchCase::CaseSensitive)) return false;
	int32 I = Prefix.Len();
	auto Digits = [&](int32& Out) -> bool
	{
		int32 Count = 0;
		Out = 0;
		while (I < Cmd.Len() && FChar::IsDigit(Cmd[I]))
		{
			if (++Count > 4) return false;
			Out = Out * 10 + (Cmd[I] - TEXT('0'));
			I++;
		}
		return Count >= 1;
	};
	auto Spaces = [&]() { while (I < Cmd.Len() && Cmd[I] == TEXT(' ')) I++; };
	if (!Digits(A)) return false;
	Spaces();
	if (I >= Cmd.Len() || Cmd[I] != TEXT('-')) return false;
	I++;
	Spaces();
	if (!Digits(B)) return false;
	return I == Cmd.Len();
}

FString FS47OpsTerminal::Run(const FString& Input)
{
	const FS47OpsView V = View();
	const FString Trimmed = Input.TrimStartAndEnd();
	// trim, upper case, any run of whitespace to one space
	FString Cmd;
	bool bSpace = false;
	for (const TCHAR C : Trimmed.ToUpper())
	{
		if (FChar::IsWhitespace(C)) { if (!bSpace) Cmd.AppendChar(TEXT(' ')); bSpace = true; }
		else { Cmd.AppendChar(C); bSpace = false; }
	}
	if (Cmd.IsEmpty()) return TEXT("");
	if (Cmd == TEXT("HELP"))
	{
		TArray<FString> Lines = { TEXT("COMMANDS"), TEXT("  DIR SURVEY.RAW     SURVEY FILES"), TEXT("  TYPE <FILE>        FILE HEADER"),
			TEXT("  SHOW EXCEPTIONS    SOLVER EXCEPTIONS"), TEXT("  SHOW TAPES [A-B]   RAW TAPE CATALOGUE"), TEXT("  SHOW LOG           CONSOLE LOG") };
		if (More.IsSet()) Lines.Append(More->Help);
		Lines.Add(TEXT("  LOGOUT"));
		return FString::Join(Lines, TEXT("\n"));
	}
	if (More.IsSet() && More->Run)
	{
		const TOptional<FString> Answer = More->Run(Cmd);
		if (Answer.IsSet()) return Answer.GetValue();
	}
	if (Cmd == TEXT("DIR") || Cmd == TEXT("DIR SURVEY.RAW") || Cmd == TEXT("DIRECTORY") || Cmd == TEXT("DIRECTORY SURVEY.RAW")) return Dir(V);
	if (Cmd.StartsWith(TEXT("TYPE "), ESearchCase::CaseSensitive)) return Type(V, Cmd.Mid(5).TrimStartAndEnd());
	if (Cmd == TEXT("SHOW EXCEPTIONS") || Cmd == TEXT("SHOW EXCEPTIONS/SOLVER")) return Exceptions(V);
	if (Cmd == TEXT("SHOW LOG") || Cmd == TEXT("SHOW LOG/TODAY")) return Log(V);
	if (Cmd == TEXT("SHOW TAPES"))
	{
		Extra.AddUnique(TEXT("SHOW TAPES 040-050"));
		return TEXT("RAW TAPE CATALOGUE   SARO   1979-1986\nREELS ON FILE: 001-046, 048-1311");
	}
	int32 A = 0, B = 0;
	if (ParseTapeRange(Cmd, A, B)) return Tapes(A, B);
	// the first word of what was typed, as typed (split on a plain space), in upper case
	int32 Space = INDEX_NONE;
	FString First = Trimmed.FindChar(TEXT(' '), Space) ? Trimmed.Left(Space) : Trimmed;
	First.ToUpperInline();
	return FString::Printf(TEXT("%%OPS-W-UNKNOWN, unrecognized command\n \\%s\\"), *First);
}

TArray<FS47OpsTerminal::FFile> FS47OpsTerminal::Files(const FS47OpsView& V) const
{
	TArray<FFile> F = {
		{ TEXT("RUN860411_2340.DAT"), 2210, TEXT("11-APR-1986 23:40") },
		{ TEXT("RUN860412_2338.DAT"), 2214, TEXT("12-APR-1986 23:38") },
		{ TEXT("RUN860413_2341.DAT"), RunBlocks(V.Clock), TEXT("13-APR-1986 23:41") },
	};
	if (V.bFuture) F.Add({ TEXT("RUN860414_0529.DAT"), 0, TEXT("14-APR-1986 05:29") });
	return F;
}

FString FS47OpsTerminal::Dir(const FS47OpsView& V)
{
	const TArray<FFile> F = Files(V);
	TArray<FString> Rows;
	int32 Total = 0;
	for (const FFile& File : F)
	{
		Extra.AddUnique(TEXT("TYPE ") + File.Name);
		Rows.Add(FString::Printf(TEXT("%s%s   %s"), *PadEnd(File.Name, 20), *PadStart(FString::FromInt(File.Blocks), 6), *File.Made));
		Total += File.Blocks;
	}
	return FString::Printf(TEXT("Directory DUA1:[SURVEY.RAW]\n\n%s\n\nTotal of %d files, %d blocks."), *FString::Join(Rows, TEXT("\n")), F.Num(), Total);
}

FString FS47OpsTerminal::Type(const FS47OpsView& V, const FString& Name) const
{
	const TArray<FFile> All = Files(V);
	const FFile* F = All.FindByPredicate([&](const FFile& File) { return File.Name == Name || File.Name == Name + TEXT(".DAT"); });
	if (!F) return FString::Printf(TEXT("%%TYPE-E-OPENIN, error opening %s\n-RMS-E-FNF, file not found"), *Name);
	const FString Id = F->Name.Replace(TEXT(".DAT"), TEXT(""));
	const bool bOpen = F->Name == TEXT("RUN860413_2341.DAT");
	if (F->Name == TEXT("RUN860414_0529.DAT"))
		return FString::Printf(TEXT("SARO SURVEY RUN  %s\nOPENED    %s:00\nOPERATOR  REYES\nRECORDS   0\n\nFILE INCOMPLETE. RECORD NOT CLOSED."), *Id, *F->Made);
	return FString::Printf(TEXT("SARO SURVEY RUN  %s\nOPENED    %s:00\nOPERATOR  %s\nRECORDS   %d\nSTATUS    %s"),
		*Id, *F->Made, bOpen ? TEXT("REYES") : TEXT("DALE"), F->Blocks, bOpen ? TEXT("OPEN") : TEXT("CLOSED 06:00"));
}

FString FS47OpsTerminal::Exceptions(const FS47OpsView& V) const
{
	TArray<FString> Rows = {
		TEXT("22-SEP-81  01:52:07  -34 LY   INPUT ERROR"),
		TEXT("02-MAR-83  03:10:44  -36 LY   INPUT ERROR"),
		TEXT("19-OCT-85  23:58:31  -38 LY   INPUT ERROR"),
	};
	if (V.SolveAt.IsSet()) Rows.Add(FString::Printf(TEXT("14-APR-86  %s  -39 LY   UNCLASSIFIED"), *ClockText(V.SolveAt.GetValue())));
	return FString::Printf(TEXT("DATE       TIME      RANGE    CLASS\n%s\n\n%d entries."), *FString::Join(Rows, TEXT("\n")), Rows.Num());
}

// Reel 47 is not on file. Twenty reels a screen.
FString FS47OpsTerminal::Tapes(int32 A, int32 B) const
{
	if (B < A) Swap(A, B);
	A = FMath::Max(1, A);
	B = FMath::Min(1311, B);
	if (A > 1311) return TEXT("NO REELS IN RANGE.");
	if (B - A > 19) B = A + 19;
	TArray<FString> Rows;
	for (int32 N = A; N <= B; N++)
	{
		if (N == 47) continue;
		Rows.Add(FString::Printf(TEXT("%s   %s  %s"), *Pad(N, 3), *ReelDate(N), N <= 60 ? TEXT("SURVEY / COMMISSIONING") : TEXT("SURVEY")));
	}
	return FString::Printf(TEXT("REEL  DATE       CONTENT\n%s"), *FString::Join(Rows, TEXT("\n")));
}

// Seconds are shown only for tonight's lines after the seeds.
FString FS47OpsTerminal::Log(const FS47OpsView& V) const
{
	TArray<FString> Rows;
	int32 I = 0;
	for (const FS47LogLine& Line : SEEDS) { Rows.Add(PadEnd(Stamp(Line.Clock, I >= NUM_SEEDS), 17) + Line.Text); I++; }
	for (const FS47LogLine& Line : V.Log) { Rows.Add(PadEnd(Stamp(Line.Clock, I >= NUM_SEEDS), 17) + Line.Text); I++; }
	return FString::Join(Rows, TEXT("\n"));
}

// ---------- self test ----------

static void Check(TArray<FString>& Out, bool& bOk, const TCHAR* Name, const FString& Got, const FString& Want)
{
	if (Got == Want) { Out.Add(FString::Printf(TEXT("ok  %s"), Name)); return; }
	bOk = false;
	Out.Add(FString::Printf(TEXT("FAIL %s: got %s, want %s"), Name, *Got.ReplaceCharWithEscapedChar(), *Want.ReplaceCharWithEscapedChar()));
}

bool FS47OpsTerminal::SelfTest(TArray<FString>& Out)
{
	bool bOk = true;
	// Expected values from running web/src/story/nightshift.ts in node.
	FS47OpsView View;
	View.Clock = 2 * 3600 + 16 * 60;
	View.SolveAt = 2 * 3600 + 15 * 60 + 12;
	View.Log = { { 2 * 3600 + 15 * 60 + 12, TEXT("SOLVER  RANGE NEGATIVE") }, { 2 * 3600 + 15 * 60 + 40, TEXT("PRINTER  OUT 1 PAGE") } };
	View.bFuture = false;
	FS47OpsTerminal T([&View]() { return View; });

	Check(Out, bOk, TEXT("HELP"), T.Run(TEXT("HELP")),
		TEXT("COMMANDS\n  DIR SURVEY.RAW     SURVEY FILES\n  TYPE <FILE>        FILE HEADER\n  SHOW EXCEPTIONS    SOLVER EXCEPTIONS\n  SHOW TAPES [A-B]   RAW TAPE CATALOGUE\n  SHOW LOG           CONSOLE LOG\n  LOGOUT"));
	Check(Out, bOk, TEXT("SHOW EXCEPTIONS"), T.Run(TEXT("SHOW EXCEPTIONS")),
		TEXT("DATE       TIME      RANGE    CLASS\n22-SEP-81  01:52:07  -34 LY   INPUT ERROR\n02-MAR-83  03:10:44  -36 LY   INPUT ERROR\n19-OCT-85  23:58:31  -38 LY   INPUT ERROR\n14-APR-86  02:15:12  -39 LY   UNCLASSIFIED\n\n4 entries."));
	Check(Out, bOk, TEXT("SHOW TAPES 040-050"), T.Run(TEXT("SHOW TAPES 040-050")),
		TEXT("REEL  DATE       CONTENT\n040   03-AUG-79  SURVEY / COMMISSIONING\n041   04-AUG-79  SURVEY / COMMISSIONING\n042   05-AUG-79  SURVEY / COMMISSIONING\n043   06-AUG-79  SURVEY / COMMISSIONING\n044   07-AUG-79  SURVEY / COMMISSIONING\n045   08-AUG-79  SURVEY / COMMISSIONING\n046   09-AUG-79  SURVEY / COMMISSIONING\n048   11-AUG-79  SURVEY / COMMISSIONING\n049   12-AUG-79  SURVEY / COMMISSIONING\n050   13-AUG-79  SURVEY / COMMISSIONING"));
	Check(Out, bOk, TEXT("DIR SURVEY.RAW"), T.Run(TEXT("DIR SURVEY.RAW")),
		TEXT("Directory DUA1:[SURVEY.RAW]\n\nRUN860411_2340.DAT    2210   11-APR-1986 23:40\nRUN860412_2338.DAT    2214   12-APR-1986 23:38\nRUN860413_2341.DAT     968   13-APR-1986 23:41\n\nTotal of 3 files, 5392 blocks."));
	Check(Out, bOk, TEXT("SHOW LOG"), T.Run(TEXT("SHOW LOG")),
		TEXT("13-APR 22:50     TELEX RECEIVED  WX\n13-APR 23:00     TELEX RECEIVED  NET STATUS\n13-APR 23:14     RX BANK 3  K3 OPEN\n13-APR 23:15     LINE POWER  DIP 0.4 S\n13-APR 23:41     SURVEY RUN OPENED  RUN860413_2341\n14-APR 02:15:12  SOLVER  RANGE NEGATIVE\n14-APR 02:15:40  PRINTER  OUT 1 PAGE"));
	Check(Out, bOk, TEXT("FOO"), T.Run(TEXT("foo bar")), TEXT("%OPS-W-UNKNOWN, unrecognized command\n \\FOO\\"));
	Check(Out, bOk, TEXT("header"), T.Header(), TEXT("SARO OPERATIONS SYSTEM   V4.2        14-APR-1986 02:16\nTERMINAL 2   OPERATOR REYES"));
	Check(Out, bOk, TEXT("SHOW TAPES 1-1"), T.Run(TEXT("SHOW TAPES 1-1")), TEXT("REEL  DATE       CONTENT\n001   25-JUN-79  SURVEY / COMMISSIONING"));
	Check(Out, bOk, TEXT("SHOW TAPES 47-47"), T.Run(TEXT("SHOW TAPES 47-47")), TEXT("REEL  DATE       CONTENT\n"));
	Check(Out, bOk, TEXT("SHOW TAPES 100-100"), T.Run(TEXT("SHOW TAPES 100-100")), TEXT("REEL  DATE       CONTENT\n100   02-OCT-79  SURVEY"));
	Check(Out, bOk, TEXT("SHOW TAPES 1311-1311"), T.Run(TEXT("SHOW TAPES 1311-1311")), TEXT("REEL  DATE       CONTENT\n1311   13-APR-86  SURVEY"));
	Check(Out, bOk, TEXT("SHOW TAPES 1300-1400"), T.Run(TEXT("SHOW TAPES 1300-1400")),
		TEXT("REEL  DATE       CONTENT\n1300   22-MAR-86  SURVEY\n1301   24-MAR-86  SURVEY\n1302   26-MAR-86  SURVEY\n1303   28-MAR-86  SURVEY\n1304   30-MAR-86  SURVEY\n1305   01-APR-86  SURVEY\n1306   03-APR-86  SURVEY\n1307   05-APR-86  SURVEY\n1308   07-APR-86  SURVEY\n1309   09-APR-86  SURVEY\n1310   11-APR-86  SURVEY\n1311   13-APR-86  SURVEY"));
	Check(Out, bOk, TEXT("SHOW TAPES 2000-2100"), T.Run(TEXT("SHOW TAPES 2000-2100")), TEXT("NO REELS IN RANGE."));
	Check(Out, bOk, TEXT("SHOW TAPES 50 - 40 (swapped)"), T.Run(TEXT("SHOW TAPES 50 - 40")), T.Run(TEXT("SHOW TAPES 040-050")));
	Check(Out, bOk, TEXT("SHOW TAPES"), T.Run(TEXT("SHOW TAPES")), TEXT("RAW TAPE CATALOGUE   SARO   1979-1986\nREELS ON FILE: 001-046, 048-1311"));
	Check(Out, bOk, TEXT("buttons"), FString::Join(T.Buttons(), TEXT("|")),
		TEXT("DIR SURVEY.RAW|SHOW EXCEPTIONS|SHOW TAPES|SHOW LOG|HELP|TYPE RUN860411_2340.DAT|TYPE RUN860412_2338.DAT|TYPE RUN860413_2341.DAT|SHOW TAPES 040-050"));
	Check(Out, bOk, TEXT("TYPE RUN860413_2341"), T.Run(TEXT("TYPE RUN860413_2341")),
		TEXT("SARO SURVEY RUN  RUN860413_2341\nOPENED    13-APR-1986 23:41:00\nOPERATOR  REYES\nRECORDS   968\nSTATUS    OPEN"));
	Check(Out, bOk, TEXT("TYPE NOPE"), T.Run(TEXT("TYPE NOPE")), TEXT("%TYPE-E-OPENIN, error opening NOPE\n-RMS-E-FNF, file not found"));
	Check(Out, bOk, TEXT("TYPE RUN860411_2340.DAT"), T.Run(TEXT("TYPE RUN860411_2340.DAT")),
		TEXT("SARO SURVEY RUN  RUN860411_2340\nOPENED    11-APR-1986 23:40:00\nOPERATOR  DALE\nRECORDS   2210\nSTATUS    CLOSED 06:00"));
	Check(Out, bOk, TEXT("empty"), T.Run(TEXT("   ")), TEXT(""));

	// All Night: the future file is in the directory
	View.bFuture = true;
	View.Clock = 5 * 3600 + 30 * 60;
	Check(Out, bOk, TEXT("DIR future"), T.Run(TEXT("DIR")),
		TEXT("Directory DUA1:[SURVEY.RAW]\n\nRUN860411_2340.DAT    2210   11-APR-1986 23:40\nRUN860412_2338.DAT    2214   12-APR-1986 23:38\nRUN860413_2341.DAT    2181   13-APR-1986 23:41\nRUN860414_0529.DAT       0   14-APR-1986 05:29\n\nTotal of 4 files, 6605 blocks."));
	Check(Out, bOk, TEXT("TYPE RUN860414_0529.DAT"), T.Run(TEXT("TYPE RUN860414_0529.DAT")),
		TEXT("SARO SURVEY RUN  RUN860414_0529\nOPENED    14-APR-1986 05:29:00\nOPERATOR  REYES\nRECORDS   0\n\nFILE INCOMPLETE. RECORD NOT CLOSED."));
	Check(Out, bOk, TEXT("header 05:30"), T.Header(), TEXT("SARO OPERATIONS SYSTEM   V4.2        14-APR-1986 05:30\nTERMINAL 2   OPERATOR REYES"));

	// Before the solve, with nothing logged tonight
	FS47OpsTerminal Early([]() { FS47OpsView E; E.Clock = 0; return E; });
	Check(Out, bOk, TEXT("SHOW EXCEPTIONS (no solve)"), Early.Run(TEXT("SHOW EXCEPTIONS")),
		TEXT("DATE       TIME      RANGE    CLASS\n22-SEP-81  01:52:07  -34 LY   INPUT ERROR\n02-MAR-83  03:10:44  -36 LY   INPUT ERROR\n19-OCT-85  23:58:31  -38 LY   INPUT ERROR\n\n3 entries."));
	Check(Out, bOk, TEXT("SHOW LOG (seeds only)"), Early.Run(TEXT("SHOW LOG")),
		TEXT("13-APR 22:50     TELEX RECEIVED  WX\n13-APR 23:00     TELEX RECEIVED  NET STATUS\n13-APR 23:14     RX BANK 3  K3 OPEN\n13-APR 23:15     LINE POWER  DIP 0.4 S\n13-APR 23:41     SURVEY RUN OPENED  RUN860413_2341"));

	// The array's commands hooked in
	FS47OpsMore MoreCmds;
	MoreCmds.Run = [](const FString& Cmd) { return Cmd == TEXT("ARRAY RESUME") ? TOptional<FString>(FString(TEXT("ARRAY RESUMING."))) : TOptional<FString>(); };
	MoreCmds.Buttons = []() { return TArray<FString>{ TEXT("ARRAY RESUME") }; };
	MoreCmds.Help = { TEXT("  ARRAY RESUME       RESUME THE ARRAY") };
	Early.More = MoreCmds;
	Check(Out, bOk, TEXT("more run"), Early.Run(TEXT("array  resume")), TEXT("ARRAY RESUMING."));
	Check(Out, bOk, TEXT("more buttons"), FString::Join(Early.Buttons(), TEXT("|")), TEXT("DIR SURVEY.RAW|SHOW EXCEPTIONS|SHOW TAPES|SHOW LOG|HELP|ARRAY RESUME"));
	Check(Out, bOk, TEXT("more help"), Early.Run(TEXT("HELP")),
		TEXT("COMMANDS\n  DIR SURVEY.RAW     SURVEY FILES\n  TYPE <FILE>        FILE HEADER\n  SHOW EXCEPTIONS    SOLVER EXCEPTIONS\n  SHOW TAPES [A-B]   RAW TAPE CATALOGUE\n  SHOW LOG           CONSOLE LOG\n  ARRAY RESUME       RESUME THE ARRAY\n  LOGOUT"));
	return bOk;
}

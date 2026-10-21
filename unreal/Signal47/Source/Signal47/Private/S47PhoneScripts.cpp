#include "S47PhoneScripts.h"

// core/phoneScripts.ts, line for line.

namespace
{
	// a spoken line: { by, text }
	FS47PhoneCue Say(const TCHAR* By, const TCHAR* Text) { FS47PhoneCue C; C.By = By; C.Text = Text; return C; }
	// what the player does on the line meanwhile: { text, aside: true, pause }
	FS47PhoneCue Aside(const TCHAR* Text, float Pause) { FS47PhoneCue C; C.Text = Text; C.bAside = true; C.Pause = Pause; return C; }
}

const TArray<FString>& S47PhoneScripts::NoraCbTexts()
{
	static const TArray<FString> T = {
		TEXT("SARO seven, this is Sierra. You hear me?"),
		TEXT("Pull in at the north end, by the office. Not down past the rooms."),
		TEXT("Room six. The door is open. Bring the prints."),
	};
	return T;
}

const TMap<FString, TArray<FS47PhoneCue>>& S47PhoneScripts::PhoneCalls()
{
	static const TMap<FString, TArray<FS47PhoneCue>> Calls = {
		{ TEXT("ward"), {
			Say(TEXT("WARD"), TEXT("That is enough to hold the morning series. It is not enough to tell us why it happened.")),
			Say(TEXT("WARD"), TEXT("Check the reference record. If the instrument inherited the fault, the record may tell us from where.")),
		} },
		{ TEXT("nora"), {
			Say(TEXT("N. VEGA"), TEXT("Sierra Motor Court.")),
			Aside(TEXT("You give your name and SARO, and read her the survey number from the sleeve."), 3.4f),
			Say(TEXT("N. VEGA"), TEXT("Station One. Nobody has asked me for that key in a long time.")),
			Say(TEXT("N. VEGA"), TEXT("Don't send me copies. Go out to the station first and look at the cable, where it was cut. Then bring me the originals.")),
		} },
		{ TEXT("fieldNora"), {
			Say(TEXT("N. VEGA"), TEXT("So you went out there.")),
			Say(TEXT("N. VEGA"), TEXT("This line still rings in the motel office. It has since 1947. Nobody ever took it down.")),
			Say(TEXT("N. VEGA"), TEXT("You have seen the cable now. It did not break by itself.")),
			Say(TEXT("N. VEGA"), TEXT("Room 6. Bring the original record and your film, developed. Then I will tell you the rest.")),
		} },
	};
	return Calls;
}

const TArray<FS47PhoneCue>& S47PhoneScripts::Ward() { return PhoneCalls()[TEXT("ward")]; }
const TArray<FS47PhoneCue>& S47PhoneScripts::Nora() { return PhoneCalls()[TEXT("nora")]; }
const TArray<FS47PhoneCue>& S47PhoneScripts::FieldNora() { return PhoneCalls()[TEXT("fieldNora")]; }

const TArray<FString>& S47PhoneScripts::PhoneTexts()
{
	static const TArray<FString> T = {
		TEXT("That is enough to hold the morning series. It is not enough to tell us why it happened."),
		TEXT("Check the reference record. If the instrument inherited the fault, the record may tell us from where."),
		TEXT("Sierra Motor Court."),
		TEXT("Station One. Nobody has asked me for that key in a long time."),
		TEXT("Don't send me copies. Go out to the station first and look at the cable, where it was cut. Then bring me the originals."),
		TEXT("So you went out there."),
		TEXT("This line still rings in the motel office. It has since 1947. Nobody ever took it down."),
		TEXT("You have seen the cable now. It did not break by itself."),
		TEXT("Room 6. Bring the original record and your film, developed. Then I will tell you the rest."),
		TEXT("Where does that phone go?"),
		TEXT("To the office. At night the office line is patched through to this room, and the other end of it is the field hut. It has been since 1947."),
		TEXT("You left the receiver on out there. Listen."),
		TEXT("Tomás. Where did you leave the spare key?"),
		TEXT("Reference west. No. East. Hold the last reading."),
		TEXT("That is what he said then. Not an answer to what I asked now."),
		TEXT("Ward."),
		TEXT("It is Reyes. I am at the diner on the old road."),
		TEXT("I read the telex. All of it."),
		TEXT("Hondo called. So did Site 11."),
		TEXT("What did they say?"),
		TEXT("Enough. I am not doing this on a pay phone."),
		TEXT("I am checking the old road first."),
		TEXT("Then check it, and come back. I want you on the console before the morning series."),
		TEXT("Six o'clock, Reyes."),
	};
	return T;
}

const TArray<FS47PhoneCue>& S47PhoneScripts::WardArray()
{
	static const TArray<FS47PhoneCue> T = {
		Say(TEXT("WARD"), TEXT("Reyes. One more thing.")),
		Say(TEXT("WARD"), TEXT("Put the array back on the schedule and tell me it stays there.")),
	};
	return T;
}

bool S47PhoneScripts::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s phone scripts: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bCond; };
	Check(PhoneTexts().Num() == 24 && NoraCbTexts().Num() == 3, TEXT("24 recorded phone texts, 3 CB texts"));
	Check(Ward().Num() == 2 && Nora().Num() == 4 && FieldNora().Num() == 4 && WardArray().Num() == 2, TEXT("the calls' lengths"));
	Check(Nora()[1].bAside && Nora()[1].By.IsEmpty() && Nora()[1].Pause.IsSet() && *Nora()[1].Pause == 3.4f, TEXT("the aside in N. Vega's call"));
	// every spoken line of the scripted calls is in the recorded bank; Ward's call-back is not
	bool bAll = true;
	for (const TCHAR* K : { TEXT("ward"), TEXT("nora"), TEXT("fieldNora") })
		for (const FS47PhoneCue& C : PhoneCalls()[K]) if (!C.bAside && !PhoneTexts().Contains(C.Text)) bAll = false;
	Check(bAll, TEXT("the scripted calls are all in PHONE_TEXTS"));
	Check(!PhoneTexts().Contains(WardArray()[0].Text) && !PhoneTexts().Contains(WardArray()[1].Text), TEXT("Ward's call-back has no recording"));
	Check(PhoneTexts()[12] == FString(TEXT("Tom")) + TCHAR(0xE1) + TEXT("s. Where did you leave the spare key?"), TEXT("the accent in Tomas"));
	return bOk;
}

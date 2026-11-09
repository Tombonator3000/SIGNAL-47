// DALE-WIRE-20261109: Dale's own takes in Freeform (S47DaleVoice.h).
#include "S47DaleVoice.h"
#include "S47Audio.h"
#include "S47RecordedDialogue.h"
#include "S47PredecessorText.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	// As FS47RadioVoice keeps a text: runs of whitespace made one space, none at either end.
	FString Single(const FString& Text)
	{
		FString S; bool bSpace = false;
		for (TCHAR C : Text)
		{
			if (FChar::IsWhitespace(C)) { bSpace = !S.IsEmpty(); continue; }
			if (bSpace) S.AppendChar(TEXT(' '));
			S.AppendChar(C); bSpace = false;
		}
		return S;
	}
	void Split(const FString& T, const FS47DaleVoice::FFind& Find, const FString& Finally, const FString& Pre, const FString& Post, TArray<FS47DaleVoice::FPart>& Out, int32 Depth)
	{
		if (T.IsEmpty()) return;
		if (TOptional<FS47RadioClip> Whole = Find ? Find(T) : TOptional<FS47RadioClip>()) { Out.Add({ T, MoveTemp(Whole) }); return; }
		if (Depth < 3)
		{
			// "Finally." before what he had to say (FS47Predecessor::SayBark).
			if (!Finally.IsEmpty() && T.Len() > Finally.Len() + 1 && T.StartsWith(Finally + TEXT(" "), ESearchCase::CaseSensitive))
			{
				Out.Add({ Finally, Find ? Find(Finally) : TOptional<FS47RadioClip>() });
				Split(T.Mid(Finally.Len() + 1), Find, Finally, Pre, Post, Out, Depth + 1);
				return;
			}
			// A stranger's report as he passes it on (FS47Predecessor::RelayLine): his two lines around it.
			if (!Pre.IsEmpty() && !Post.IsEmpty() && T.Len() > Pre.Len() + Post.Len() + 2 && T.StartsWith(Pre + TEXT(" "), ESearchCase::CaseSensitive) && T.EndsWith(TEXT(" ") + Post, ESearchCase::CaseSensitive))
			{
				Out.Add({ Pre, Find ? Find(Pre) : TOptional<FS47RadioClip>() });
				Split(T.Mid(Pre.Len() + 1, T.Len() - Pre.Len() - Post.Len() - 2), Find, Finally, Pre, Post, Out, Depth + 1);
				Out.Add({ Post, Find ? Find(Post) : TOptional<FS47RadioClip>() });
				return;
			}
		}
		Out.Add({ T, {} });
	}
}

int32 FS47DaleVoice::FPlan::Voiced() const
{
	int32 N = 0;
	for (const FPart& P : Parts) N += P.Clip.IsSet() ? 1 : 0;
	return N;
}
FString FS47DaleVoice::FPlan::Text() const
{
	TArray<FString> Words;
	for (const FPart& P : Parts) Words.Add(P.Text);
	return FString::Join(Words, TEXT(" "));
}

FString FS47DaleVoice::Spoken(const FString& Text)
{
	const FString T = Single(Text);
	return T.StartsWith(TEXT("DALE:"), ESearchCase::CaseSensitive) ? Single(T.Mid(5)) : T;
}

void FS47DaleVoice::Time(FPlan& Plan)
{
	Plan.Seconds = Plan.SpeechStart = Plan.SpeechEnd = 0;
	double At = 0; bool bFirst = true;
	for (const FPart& P : Plan.Parts)
	{
		if (!P.Clip) continue;
		const FS47RadioClip& C = *P.Clip;
		if (!bFirst) At += Gap;
		// takes.json's speech window when it is a sane one; else the whole take.
		const bool bMeasured = C.SpeechStart >= 0 && C.SpeechEnd > C.SpeechStart && C.SpeechEnd <= C.Duration + 1e-3;
		if (bFirst) Plan.SpeechStart = At + (bMeasured ? C.SpeechStart : 0);
		Plan.SpeechEnd = At + (bMeasured ? C.SpeechEnd : C.Duration);
		At += C.Duration; bFirst = false;
	}
	Plan.Seconds = At;
}

FS47DaleVoice::FPlan FS47DaleVoice::Walkie(const FString& Text, const FFind& Find, const FString& Finally, const FString& Pre, const FString& Post)
{
	FPlan Plan;
	Split(Spoken(Text), Find, Single(Finally), Single(Pre), Single(Post), Plan.Parts, 0);
	Time(Plan);
	return Plan;
}

FS47DaleVoice::FPlan FS47DaleVoice::Lines(const TArray<FString>& Texts, const FFind& Find)
{
	FPlan Plan;
	for (const FString& Each : Texts)
	{
		const FString T = Spoken(Each);
		if (!T.IsEmpty()) Plan.Parts.Add({ T, Find ? Find(T) : TOptional<FS47RadioClip>() });
	}
	Time(Plan);
	return Plan;
}

FS47DaleVoice::FS47DaleVoice(US47Audio* InAudio, const FString& InChannel) : Audio(InAudio), Channel(InChannel)
{
	const FString Ch = Channel;
	Dialogue = MakeShared<FS47RecordedDialogue>(InAudio, TEXT("dale"),
		// The plan's takes by id, all or nothing (a part without one never reaches here).
		[](const FS47DialogueInput& I, FS47RadioVoice& Bank) -> TOptional<TArray<FS47RadioClip>>
		{
			TArray<FS47RadioClip> Out;
			for (const FS47DialogueLine& L : I.Lines)
			{
				const TOptional<FS47RadioClip> C = Bank.FindId(L.Who);
				if (!C) return {};
				Out.Add(*C);
			}
			return Out.IsEmpty() ? TOptional<TArray<FS47RadioClip>>() : TOptional<TArray<FS47RadioClip>>(Out);
		},
		// Both are a small receiver: the telephone's band (as Evening and HorrorStory hear his walkie and his telephone).
		[](const FS47RadioClip&) { return true; },
		[Ch](const FS47RadioClip&) { FS47RecordedDialogue::FOutput O; O.Channel = Ch; return O; });
}
FS47DaleVoice::~FS47DaleVoice() { if (Dialogue) Dialogue->Stop(); Dialogue.Reset(); }

TOptional<FS47RadioClip> FS47DaleVoice::FindText(const FString& Text) const
{
	if (!Dialogue || !Dialogue->Bank) return {};
	const TOptional<TArray<FS47RadioClip>> Found = Dialogue->Bank->Resolve(Spoken(Text), 0);
	if (!Found || Found->Num() != 1) return {};
	const FS47RadioClip& C = (*Found)[0];
	if (!C.Channel.IsEmpty() && C.Channel != Channel) return {};
	return C;
}
TOptional<FS47RadioClip> FS47DaleVoice::FindId(const FString& Id) const
{
	if (!Dialogue || !Dialogue->Bank) return {};
	TOptional<FS47RadioClip> C = Dialogue->Bank->FindId(Id);
	if (C && !C->Channel.IsEmpty() && C->Channel != Channel) return {};
	return C;
}

FS47DaleVoice::FPlan FS47DaleVoice::PlanWalkie(const FString& Text) const
{
	namespace X = S47PredecessorText;
	return Walkie(Text, [this](const FString& T) { return FindText(T); }, X::WalkieLine(TEXT("finally")), X::WalkieLine(TEXT("relay_pre")), X::WalkieLine(TEXT("relay_post")));
}
FS47DaleVoice::FPlan FS47DaleVoice::PlanLines(const TArray<FString>& Texts) const
{
	return Lines(Texts, [this](const FString& T) { return FindText(T); });
}

bool FS47DaleVoice::Play(const FPlan& Plan)
{
	Stop();
	US47Audio* A = Audio.Get();
	if (!Dialogue || !A || !A->bCharacterVoices || !A->Ctx.IsValid()) return false;
	FS47DialogueInput Input; Input.Voice = TEXT("dale");
	for (const FPart& P : Plan.Parts) if (P.Clip) Input.Lines.Add({ P.Clip->Id, P.Clip->Text });
	if (Input.Lines.IsEmpty()) return false;
	Dialogue->SetEnabled(true);
	Dialogue->Play(Input);
	// Loaded and started at once (RadioVoice reads its short takes synchronously); anything else is no voice at all.
	if (!Dialogue->Playing()) { Dialogue->Stop(); return false; }
	StartedAt = Dialogue->StartedAt();
	UE_LOG(LogTemp, Display, TEXT("S47: Dale's voice on the %s: %d of %d parts, %.2f s"), *Channel, Input.Lines.Num(), Plan.Parts.Num(), Plan.Seconds);
	return true;
}
void FS47DaleVoice::Stop() { if (Dialogue) Dialogue->Stop(); StartedAt = -1; }
bool FS47DaleVoice::Playing() const { return Dialogue && StartedAt >= 0 && (Dialogue->Playing() || Dialogue->Loading()); }
double FS47DaleVoice::Clock() const
{
	const US47Audio* A = Audio.Get();
	if (!Playing() || !A || !A->Ctx.IsValid()) return -1;
	return A->Ctx->CurrentTime() - StartedAt;
}
TFunction<double()> FS47DaleVoice::CaptionClock() const
{
	const US47Audio* A = Audio.Get();
	if (!A || !A->Ctx.IsValid()) return {};
	const TWeakPtr<FS47AudioContext> Ctx = A->Ctx;
	// Gone with the game's sound: the caption is let go.
	return [Ctx] { const auto C = Ctx.Pin(); return C ? C->CurrentTime() : 1e20; };
}

bool FS47DaleVoice::SelfTest(TArray<FString>& Lines)
{
	namespace X = S47PredecessorText;
	bool bOk = true;
	auto Check = [&](bool b, const TCHAR* T) { Lines.Add(FString::Printf(TEXT("%s dale voice: %s"), b ? TEXT("ok") : TEXT("FAIL"), T)); bOk &= b; };
	const FString Hello = X::Barks()[0].Line, Finally = X::WalkieLine(TEXT("finally")), Pre = X::WalkieLine(TEXT("relay_pre")), Post = X::WalkieLine(TEXT("relay_post"));
	const FString Tip = TEXT("A rancher on the county road reports lights low over the Baca lease. He says he will not go and look.");
	const FString Relay = FString::Printf(TEXT("DALE: %s %s %s"), *Pre, *Tip, *Post);
	Check(Hello.StartsWith(TEXT("Dale, field maintenance.")) && !Finally.IsEmpty() && !Pre.IsEmpty() && !Post.IsEmpty(), TEXT("his lines are the ones in S47PredecessorText"));
	// One file stands for every take: the bank only asks that it is there (FS47RadioVoice::Valid).
	const FString Root = FPaths::ConvertRelativePathToFull(FPlatformProcess::UserTempDir()), File = TEXT("S47DaleVoiceSelfTest.wav");
	if (!FFileHelper::SaveStringToFile(TEXT("RIFF"), *(Root / File))) { Check(false, TEXT("test take written")); return false; }
	auto Catalog = [&](const TArray<TArray<FString>>& Rows)
	{
		TArray<TSharedPtr<FJsonValue>> Clips;
		for (const TArray<FString>& R : Rows)
		{
			const TSharedPtr<FJsonObject> O = MakeShared<FJsonObject>();
			O->SetStringField(TEXT("id"), R[0]); O->SetStringField(TEXT("text"), R[1]); O->SetStringField(TEXT("channel"), R[2]);
			O->SetStringField(TEXT("voice"), TEXT("dale")); O->SetStringField(TEXT("file"), R.IsValidIndex(3) ? R[3] : File);
			O->SetNumberField(TEXT("duration"), 2); O->SetNumberField(TEXT("speech_start_seconds"), .1); O->SetNumberField(TEXT("speech_end_seconds"), 1.8);
			Clips.Add(MakeShared<FJsonValueObject>(O));
		}
		const TSharedPtr<FJsonObject> Cat = MakeShared<FJsonObject>(); Cat->SetArrayField(TEXT("clips"), Clips);
		return Cat;
	};
	FS47DaleVoice Walkie(nullptr, TEXT("walkie")), Phone(nullptr, TEXT("phone"));
	// No bank (or an empty one): the line is unchanged text and nothing is played.
	Walkie.Dialogue->Bank->UseCatalog(Catalog({}), Root);
	{
		const FPlan P = Walkie.PlanWalkie(TEXT("DALE: ") + Hello);
		Check(P.Voiced() == 0 && P.Text() == Hello && !Walkie.Play(P) && !Walkie.Playing() && Walkie.Clock() < 0, TEXT("no bank: his words as they were, and no voice"));
		const FPlan R = Walkie.PlanWalkie(Relay);
		Check(R.Voiced() == 0 && R.Text() == Spoken(Relay), TEXT("no bank: a passed-on report keeps every word"));
	}
	// A made-up catalog with one id.
	Walkie.Dialogue->Bank->UseCatalog(Catalog({ { TEXT("FF_BARK_hello"), Hello, TEXT("walkie") } }), Root);
	Phone.Dialogue->Bank->UseCatalog(Catalog({ { TEXT("FF_BARK_hello"), Hello, TEXT("walkie") } }), Root);
	{
		const auto ById = Walkie.FindId(TEXT("FF_BARK_hello")), ByText = Walkie.FindText(Hello);
		Check(ById && ById->Id == TEXT("FF_BARK_hello") && ByText && ByText->Id == TEXT("FF_BARK_hello") && FMath::IsNearlyEqual(ById->SpeechEnd, 1.8), TEXT("one id: found by id and by exact text, with its speech window"));
		const auto Head = Walkie.FindText(TEXT("DALE: ") + Hello), Tight = Walkie.FindText(TEXT("DALE:") + Hello), Loose = Walkie.FindText(TEXT("  DALE:   ") + Hello.Replace(TEXT(". "), TEXT(".\n ")));
		Check(Head && Tight && Loose && Head->Id == ById->Id && Tight->Id == ById->Id && Loose->Id == ById->Id, TEXT("with the DALE: head, with or without its space, and loose whitespace: the same id"));
		Check(!Walkie.FindId(TEXT("FF_BARK_none")) && !Walkie.FindText(Hello + TEXT(" Out.")), TEXT("another id or other words find nothing"));
		Check(!Phone.FindId(TEXT("FF_BARK_hello")) && !Phone.FindText(Hello), TEXT("a walkie take is not played on the telephone"));
		const FPlan P = Walkie.PlanWalkie(TEXT("DALE: ") + Hello);
		Check(P.All() && P.Parts.Num() == 1 && P.Parts[0].Clip->Id == TEXT("FF_BARK_hello") && FMath::IsNearlyEqual(P.SpeechStart, .1) && FMath::IsNearlyEqual(P.SpeechEnd, 1.8) && FMath::IsNearlyEqual(P.Seconds, 2.), TEXT("the whole line is one take, its caption from his first word to his last"));
		Check(!Walkie.Play(P) && !Walkie.Playing(), TEXT("no game sound: a found take is still not played"));
		Walkie.Dialogue->Bank->UseCatalog(Catalog({ { TEXT("FF_BARK_hello"), Hello, TEXT("walkie"), TEXT("S47DaleVoiceSelfTestMissing.wav") } }), Root);
		Check(!Walkie.FindId(TEXT("FF_BARK_hello")) && Walkie.PlanWalkie(TEXT("DALE: ") + Hello).Voiced() == 0, TEXT("a catalog entry whose file is missing is no take"));
	}
	// Lines made of lines: one take after another, in order.
	Walkie.Dialogue->Bank->UseCatalog(Catalog({ { TEXT("FF_BARK_hello"), Hello, TEXT("walkie") }, { TEXT("FF_WALKIE_finally"), Finally, TEXT("walkie") },
		{ TEXT("FF_WALKIE_relay_pre"), Pre, TEXT("walkie") }, { TEXT("FF_WALKIE_relay_post"), Post, TEXT("walkie") } }), Root);
	{
		const FPlan P = Walkie.PlanWalkie(TEXT("DALE: ") + Finally + TEXT(" ") + Hello);
		Check(P.All() && P.Parts.Num() == 2 && P.Parts[0].Clip->Id == TEXT("FF_WALKIE_finally") && P.Parts[1].Clip->Id == TEXT("FF_BARK_hello"), TEXT("finally before a bark: two takes, finally first"));
		Check(FMath::IsNearlyEqual(P.Seconds, 4 + Gap) && FMath::IsNearlyEqual(P.SpeechStart, .1) && FMath::IsNearlyEqual(P.SpeechEnd, 2 + Gap + 1.8), TEXT("its caption runs from the first take's first word to the second take's last"));
		const FPlan R = Walkie.PlanWalkie(Relay);
		Check(R.Parts.Num() == 3 && R.Voiced() == 2 && !R.All() && R.Parts[0].Clip && R.Parts[0].Clip->Id == TEXT("FF_WALKIE_relay_pre") && !R.Parts[1].Clip && R.Parts[1].Text == Tip && R.Parts[2].Clip && R.Parts[2].Clip->Id == TEXT("FF_WALKIE_relay_post") && R.Text() == Spoken(Relay),
			TEXT("a passed-on report: relay_pre, the report as text only, relay_post"));
		Check(FMath::IsNearlyEqual(R.Seconds, 4 + Gap), TEXT("the report itself adds no time to the voice"));
		FS47DialogueInput Input;
		for (const FPart& Part : R.Parts) if (Part.Clip) Input.Lines.Add({ Part.Clip->Id, Part.Clip->Text });
		Check(Input.Lines.Num() == 2 && Walkie.Dialogue->Available(Input), TEXT("the dialogue queue resolves the plan's takes by id, all of them"));
		Input.Lines.Add({ TEXT("FF_BARK_none"), TEXT("Nothing.") });
		Check(!Walkie.Dialogue->Available(Input), TEXT("and refuses a plan with a take it has not got"));
		const FString Big = TEXT("DALE: Herrgott. Every dish on remote is looking at 026 this morning, 41 degrees up, like dogs at a door. Not on my log. Not on yours? Good. Then nobody did it.");
		Check(Walkie.PlanWalkie(Big).Voiced() == 0 && Walkie.PlanWalkie(Big).Text() == Spoken(Big), TEXT("a line with a bearing put in is text only"));
		Check(Walkie.PlanWalkie(TEXT("DALE: ") + Finally).All() && Walkie.PlanWalkie(TEXT("DALE: ") + Finally).Parts.Num() == 1, TEXT("finally alone is its own take, not a split"));
	}
	// The telephone: each line of his on the page, in order; one without a take is only text.
	Phone.Dialogue->Bank->UseCatalog(Catalog({ { TEXT("FF_PHONE_reyes_1"), TEXT("No."), TEXT("phone") }, { TEXT("FF_PHONE_reyes_2"), TEXT("I said no. Is this the service line? Then you ask me about service."), TEXT("phone") } }), Root);
	{
		const FPlan P = Phone.PlanLines({ TEXT("No."), TEXT("Something he never said."), TEXT("I said no.  Is this the service line? Then you ask me about service.") });
		Check(P.Parts.Num() == 3 && P.Voiced() == 2 && P.Parts[0].Clip->Id == TEXT("FF_PHONE_reyes_1") && !P.Parts[1].Clip && P.Parts[2].Clip->Id == TEXT("FF_PHONE_reyes_2"), TEXT("the telephone: his lines' takes in the order the page shows them"));
		Check(!Walkie.FindText(TEXT("No.")) && !Phone.FindText(Hello), TEXT("the telephone's takes stay off the walkie"));
	}
	IFileManager::Get().Delete(*(Root / File));
	return bOk;
}

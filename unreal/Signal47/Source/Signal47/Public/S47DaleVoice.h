#pragma once
#include "CoreMinimal.h"
#include "S47RadioVoice.h"

class US47Audio;
class FS47RecordedDialogue;

// DALE-WIRE-20261109: Dale's own voice in Freeform, from the "dale" bank in Content/Audio/Raw/voices.json (Narrative/DaleVoice/
// catalog.py: one take per manifest id, with its exact text and its measured speech). A line is looked up whole, before any word of
// it is lost on the set; "finally" before a line and a stranger's report between relay_pre and relay_post are their parts, one take
// after another. A part with no take (a line with a place or a bearing in it, or no takes made yet) is only text. With no take for
// any part nothing is played and the caller does what it did before: the walkie's own chatter, the telephone's text.
// Played through FS47RecordedDialogue with the receiver's filter, on "walkie" (FS47FieldRadio) or "phone" (FS47Predecessor).
class SIGNAL47_API FS47DaleVoice
{
public:
	// FS47RecordedDialogue's pause between one take and the next.
	static constexpr double Gap = .22;
	struct FPart { FString Text; TOptional<FS47RadioClip> Clip; };
	struct FPlan
	{
		TArray<FPart> Parts;
		// On the plan's own clock, from its first take's start: all of it, his first word and his last.
		double Seconds = 0, SpeechStart = 0, SpeechEnd = 0;
		int32 Voiced() const;
		bool All() const { return Parts.Num() > 0 && Voiced() == Parts.Num(); }
		// The parts' words together again: the line as it was given, whitespace made single.
		FString Text() const;
	};
	using FFind = TFunction<TOptional<FS47RadioClip>(const FString& Text)>;
	// Without his caption head ("DALE: " or "DALE:"), with the whitespace made single as the bank keeps it.
	static FString Spoken(const FString& Text);
	// One message on the walkie: whole; else Finally and the rest; else Pre, the report and Post; else only text.
	static FPlan Walkie(const FString& Text, const FFind& Find, const FString& Finally, const FString& Pre, const FString& Post);
	// What the telephone shows of him, line for line.
	static FPlan Lines(const TArray<FString>& Texts, const FFind& Find);
	static void Time(FPlan& Plan);

	FS47DaleVoice(US47Audio* InAudio, const FString& InChannel);
	~FS47DaleVoice();
	// With his walkie lines from S47PredecessorText ("finally", "relay_pre", "relay_post").
	FPlan PlanWalkie(const FString& Text) const;
	FPlan PlanLines(const TArray<FString>& Texts) const;
	// A take on this channel, by exact text (caption head allowed) or by manifest id.
	TOptional<FS47RadioClip> FindText(const FString& Text) const;
	TOptional<FS47RadioClip> FindId(const FString& Id) const;
	// The plan's takes, in order. False, and nothing heard: no take in it, character voices off, or no running sound.
	bool Play(const FPlan& Plan);
	void Stop();
	bool Playing() const;
	// Seconds since the plan's first take began, on the sound's own clock (it stops with a pause); below 0 when nothing plays.
	double Clock() const;
	// The sound's clock itself, for a caption that should run out on it (US47UI::Toast).
	TFunction<double()> CaptionClock() const;
	const FString& ChannelName() const { return Channel; }
	static bool SelfTest(TArray<FString>& Lines);
private:
	TWeakObjectPtr<US47Audio> Audio;
	FString Channel;
	TSharedPtr<FS47RecordedDialogue> Dialogue;
	double StartedAt = -1;
};

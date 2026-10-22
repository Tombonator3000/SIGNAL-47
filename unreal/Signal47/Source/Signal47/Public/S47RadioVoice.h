#pragma once

#include "S47WebAudio.h"

struct FS47RadioClip
{
	FString Id, Text;
	double Duration = 0;
	FString File, Voice;
};

// web/src/core/radioVoice.ts: bounded lazy banks; identities include role and exact text.
class SIGNAL47_API FS47RadioVoice
{
public:
	explicit FS47RadioVoice(const FString& InKind = TEXT("host"));
	int32 BufferCount() const { return Buffers.Num(); }
	TOptional<TArray<FS47RadioClip>> Resolve(const FString& Text, double Clock, const FString& Voice = FString()) const;
	// Native difference: selected short PCM clips load synchronously; this never starts audio.
	bool Load(const TArray<FS47RadioClip>& Clips, TMap<FString, TSharedPtr<FS47AudioBuffer>>& Result, const TFunction<bool()>& IsAborted = {});
	static bool SelfTest(TArray<FString>& Lines);

private:
	struct FTimePart { int32 Value = 0; FString Id, Text; };
	struct FTemplate { FString Prefix, Suffix; bool bCapitalize = false; FString Before, After; };
	FString Kind, Root;
	TArray<FS47RadioClip> Clips;
	TMap<FString, int32> ById, ByText;
	TArray<FTemplate> Templates;
	TArray<FTimePart> Hours, Minutes;
	TMap<FString, TSharedPtr<FS47AudioBuffer>> Buffers;
	TArray<FString> Order;
	int64 Bytes = 0;
	static FString Clean(const FString& Text);
	FString Url(const FS47RadioClip& Clip) const;
	bool Valid(const TArray<FS47RadioClip>& List) const;
	void Trim();
};

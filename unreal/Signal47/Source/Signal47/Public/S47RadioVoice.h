#pragma once

#include "S47WebAudio.h"

class FJsonObject;

struct FS47RadioClip
{
	FString Id, Text;
	double Duration = 0;
	FString File, Voice;
	// DALE-WIRE-20261109: the measured speech inside the take (takes.json, seconds from its start) and the set it is heard on
	// (Dale's bank: walkie or phone). 0 and empty where a catalog has none.
	double SpeechStart = 0, SpeechEnd = 0;
	FString Channel;
};

// web/src/core/radioVoice.ts: bounded lazy banks; identities include role and exact text.
class SIGNAL47_API FS47RadioVoice
{
public:
	explicit FS47RadioVoice(const FString& InKind = TEXT("host"));
	int32 BufferCount() const { return Buffers.Num(); }
	TOptional<TArray<FS47RadioClip>> Resolve(const FString& Text, double Clock, const FString& Voice = FString()) const;
	// DALE-WIRE-20261109: one clip by its catalog id (Dale's bank: the id in Narrative/DaleVoice/manifest.json), checked as Resolve
	// checks it (a length and a file). Unset: no such clip, or no recording for it.
	TOptional<FS47RadioClip> FindId(const FString& Id) const;
	// The catalog replaced by this one, its files under InRoot: for the self-tests' made-up catalogs.
	void UseCatalog(const TSharedPtr<FJsonObject>& Catalog, const FString& InRoot);
	// Native difference: selected short PCM clips load synchronously; this never starts audio.
	bool Load(const TArray<FS47RadioClip>& Clips, TMap<FString, TSharedPtr<FS47AudioBuffer>>& Result, const TFunction<bool()>& IsAborted = {});
	// VOICE-OUT-3 (Tom 9 November 2026): the web version's ensemble voices bacall (N. Vega), price (THE LINE)
	// and lynch (the souvenir man), and KINSKI-READ-OUT's kinski bank, are clones of real people's voices and
	// out of the game. No bank or role may have one of these names; their clips are never indexed, not even
	// from an old voices.json or index.json.
	static const TArray<FString>& RetiredVoices();
	static bool RetiredVoice(const FString& VoiceOrBank) { return RetiredVoices().Contains(VoiceOrBank.ToLower()); }
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
	void Index(const TSharedPtr<FJsonObject>& Catalog);
	FString Url(const FS47RadioClip& Clip) const;
	bool Valid(const TArray<FS47RadioClip>& List) const;
	void Trim();
};

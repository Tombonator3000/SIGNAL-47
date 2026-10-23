#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Authored EveningShift JSON adapter. World actions remain the host's responsibility.
struct FS47EveningLine
{
	FString Id, Speaker, Text, VoiceProfile, RecordingId;
	bool bRecord = false;
	// Measured take-relative seconds. Zero duration means no supplied recording.
	double Duration = 0, SpeechStart = 0, SpeechEnd = 0;
};

struct FS47EveningRequirements
{
	TArray<FString> All, None;
};

struct FS47EveningEffects
{
	TMap<FString, bool> Flags;
	TArray<FString> Documents;
};

struct FS47EveningChoice
{
	FString Id, Label, Next;
	FS47EveningRequirements Requires;
	FS47EveningEffects OnSelect;
};

struct FS47EveningScene
{
	FString Id, Phase, Channel, Mode, Entry;
	TArray<FS47EveningLine> Lines;
	TArray<FS47EveningChoice> Choices;
	FS47EveningRequirements Requires;
	FS47EveningEffects OnComplete;
};

struct FS47EveningDocument
{
	FString Id, Title, Kind, Stamp;
	TArray<FString> Body;
};

class FS47EveningPack
{
public:
	bool Initialize(FString& Error);
	void Reset();
	// A host may only change phase while idle. Context flags are never script-writable.
	bool SetPhase(const FString& Phase);
	const FString& GetPhase() const { return Phase; }
	bool SetExternal(const FString& Id, bool bValue);
	bool Flag(const FString& Id) const;
	// Host-only physical meal transaction; preserves the active dialogue and pending reply.
	bool EatTakeaway();
	bool CanStart(const FString& Id) const;
	bool Start(const FString& Id);
	const FS47EveningScene* CurrentScene() const;
	const FS47EveningScene* FindScene(const FString& Id) const { return Scenes.Find(Id); }
	const FS47EveningLine* CurrentLine() const;
	int32 NextLine() const { return Cursor; }
	const FString& GetPendingChoice() const { return PendingChoice; }
	// Advance acknowledges the currently displayed line. Menus never complete here.
	bool Advance();
	TArray<const FS47EveningChoice*> Choices() const;
	// Choice effects and the pending response are one synchronous state transaction.
	bool Select(const FString& Id);
	TSharedPtr<FJsonObject> Save() const;
	bool Load(const TSharedPtr<FJsonObject>& Json, FString& Error);
	const TArray<FString>& Documents() const { return OwnedDocuments; }
	const FS47EveningDocument* FindDocument(const FString& Id) const { return DocumentData.Find(Id); }
	FString String(const FString& Id) const { return Strings.FindRef(Id); }
	FString RecordingId(const FString& Cue) const;
	// Data/state tests only; this does not claim playback, world input, or rendered proof.
	static bool SelfTest(FString& Error);

private:
	TMap<FString, FS47EveningScene> Scenes;
	TMap<FString, FS47EveningDocument> DocumentData;
	TMap<FString, FString> Strings, Aliases;
	TMap<FString, bool> Defaults, Flags, External;
	TArray<FString> Phases, OwnedDocuments;
	FString Phase = TEXT("commute"), ActiveScene, PendingChoice;
	int32 Cursor = 0;
	bool Meets(const FS47EveningRequirements& Requires, const TMap<FString, bool>& Internal) const;
	bool Eligible(const FS47EveningChoice& Choice) const;
	void Apply(const FS47EveningEffects& Effects);
	void CompleteLeaf();
};

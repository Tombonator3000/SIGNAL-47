#pragma once

#include "CoreMinimal.h"
#include "S47EveningPack.h"

// The authored H/SC pack is separate from EveningShift. Shared value types keep
// existing subtitle/document rendering; all factual context belongs to the host.
struct FS47HorrorSceneRegistration
{
	FString Id;
	TArray<FString> ScenarioIds, MissionIds;
};

class FS47HorrorPack
{
public:
	bool Initialize(FString& Error);
	void Reset();
	bool SetPhase(const FString& Value);
	const FString& GetPhase() const { return Phase; }
	bool SetExternal(const FString& Id, bool bValue);
	void ClearExternal();
	bool Flag(const FString& Id) const;
	bool CanStart(const FString& Id) const;
	bool Start(const FString& Id);
	void Cancel(); // No completion effects when the world interrupts a scene.
	const FS47EveningScene* CurrentScene() const;
	const FS47EveningScene* FindScene(const FString& Id) const { return Scenes.Find(Id); }
	const FS47EveningLine* CurrentLine() const;
	int32 NextLine() const { return Cursor; }
	const FString& GetPendingChoice() const { return PendingChoice; }
	bool Advance();
	TArray<const FS47EveningChoice*> Choices() const;
	bool Select(const FString& Id);
	TSharedPtr<FJsonObject> Save() const;
	bool Load(const TSharedPtr<FJsonObject>& Json, FString& Error);
	const TArray<FString>& Documents() const { return OwnedDocuments; }
	const FS47EveningDocument* FindDocument(const FString& Id) const { return DocumentData.Find(Id); }
	FString String(const FString& Id) const { return Strings.FindRef(Id); }
	FString RecordingId(const FString& Cue) const;
	FString ActorName(const FString& Id) const { return ActorNames.FindRef(Id); }
	const FS47HorrorSceneRegistration* FindRegistration(const FString& Id) const { return Registry.Find(Id); }
	TArray<FString> SceneIdsForScenario(const FString& Id) const;
	const TArray<FString>& SceneIds() const { return AuthoredSceneIds; }
	TArray<FString> ExternalIds() const;
	// Data/state tests. They do not attest world input, speech or rendered scenes.
	static bool SelfTest(FString& Error);

private:
	TMap<FString, FS47EveningScene> Scenes;
	TMap<FString, FS47EveningDocument> DocumentData;
	TMap<FString, FS47HorrorSceneRegistration> Registry;
	TMap<FString, FString> Strings, Aliases, ActorNames;
	TMap<FString, bool> Defaults, Flags, External;
	TArray<FString> Phases, OwnedDocuments, AuthoredSceneIds;
	FString Phase = TEXT("commute"), ActiveScene, PendingChoice;
	int32 Cursor = 0;
	bool Meets(const FS47EveningRequirements& Requires, const TMap<FString, bool>& Internal) const;
	bool Eligible(const FS47EveningChoice& Choice) const;
	void Apply(const FS47EveningEffects& Effects);
	void CompleteLeaf();
};

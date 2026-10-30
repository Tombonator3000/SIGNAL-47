#pragma once
#include "CoreMinimal.h"
class AS47Web;
class FJsonObject;
struct FS47RadioWomanState;

// Preserved private prototype. Build returns null unless both
// -S47RadioWomanPrototype and -S47TripoNpcsOff are supplied.
// Original person/radio/proxy remain the fallback during prototype runs.
class FS47RadioWoman
{
public:
 static TSharedPtr<FS47RadioWoman> Build(AS47Web* Web);
 // Called after each actual Web::Enter, including a same-area full Continue.
 void InvalidateAreaEpoch();
 // Only called by TownLayer immediately after the ordinary Town->Update.
 void AfterTownStep();
 bool IsActive() const;
 bool Audit(TArray<FString>& Lines) const;
 TSharedPtr<FJsonObject> Readback() const;
private:
 TSharedPtr<FS47RadioWomanState> State;
};

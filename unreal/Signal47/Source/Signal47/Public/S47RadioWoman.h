#pragma once
#include "CoreMinimal.h"
class AS47Web;
class FJsonObject;
struct FS47RadioWomanState;

// Optional private static mannequin. Original person/radio/proxy remain the fallback.
class FS47RadioWoman
{
public:
 static TSharedPtr<FS47RadioWoman> Build(AS47Web* Web);
 bool IsActive() const;
 bool Audit(TArray<FString>& Lines) const;
 TSharedPtr<FJsonObject> Readback() const;
private:
 TSharedPtr<FS47RadioWomanState> State;
};

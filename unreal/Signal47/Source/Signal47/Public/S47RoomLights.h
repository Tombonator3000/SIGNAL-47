#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
class US47Main;

// Player preference only. Supply, flicker and Story blackouts remain upstream.
class FS47RoomLights
{
public:
 explicit FS47RoomLights(US47Main* InM) : M(InM) {}
 ~FS47RoomLights();
 void Begin(const S47J::FObj& Saved = nullptr);
 S47J::FObj Save() const;
 void Step(double Dt);
 void Stop();
 bool RoomEnabled(const FString& Room) const;
 bool Use(const FString& Id);
 FString Label(const FString& Id) const;
 FVector InteractionPoint(const FString& Room) const;
 FVector ApproachPoint(const FString& Room, double Distance = .85) const;
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M = nullptr;
 TSharedPtr<uint8> Life = MakeShared<uint8>(0);
 bool bActive = false, bBuilt = false;
 bool Enabled[4] = { true, true, true, true };
 TArray<FS47Obj> Parts;
 FS47Obj Plates[4], Levers[4];
 double Retry = 0;
 bool Build();
 bool Available(int32 Index) const;
 void Apply();
};

#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"

class US47Main;

// SC01's physical scene. This owns only rendered objects and measured local
// observations; the story host alone translates those facts into dialogue gates.
class FS47HorrorVisual
{
public:
 explicit FS47HorrorVisual(US47Main* InM);
 ~FS47HorrorVisual();
 void Begin(const S47J::FObj& Saved, bool bEnabled);
 void Step(double Dt);
 void Stop();
 void ReleaseLighting();
 bool Use(const FString& Id);
 void ArmWindow();
 S47J::FObj Save() const;
 bool Ready() const { return bBuilt; }
 bool Enabled() const { return bEnabled; }
 bool HandSeen() const { return bHandSeen; } // the slap on the glass was heard (no hand is shown)
 bool MarksPresent() const { return HandPhase == 2; }
 bool MarksChecked() const { return bMarksChecked; }
 bool ScreenClosed() const { return ScreenK >= .995; }
 bool ScreenChecked() const { return bScreenChecked; }
 bool LampDirectChecked() const { return DirectMask == 3; }
 bool LampGlassChecked() const { return GlassMask == 3; }
 // Tom 28.10.2026: one inside comparison through the glass. Directly outside is optional.
 bool LampChangeChecked() const { return GlassMask == 3; }
 bool LampOn() const { return bLampOn; }
 bool RoomLampOn() const { return bRoomLampOn; }
 bool ReferenceInView() const;
 bool WindowArmed() const { return bWindowArmed; }
 int32 WindowPhase() const { return HandPhase; }
 double WindowTime() const { return HandT; }
 FVector HandPosition() const;
 FS47Obj Object(const FString& Id) const;
 static FVector WindowPoint() { return FVector(4.45, 1.70, -4.58); }
 static FVector LampPoint() { return FVector(4.45, 1.31, -5.48); }
 static bool SelfTest(TArray<FString>& Lines);

private:
 struct FVisual;
 TUniquePtr<FVisual> Visual;
 US47Main* M = nullptr;
 bool bBuilt = false, bBuildFailed = false, bEnabled = false, bWindowArmed = false;
 bool bHandSeen = false, bMarksChecked = false, bLampOn = true, bRoomLampOn = true;
 bool bScreenTarget = false, bScreenChecked = false, bClearSeen = false;
 bool bLightingCaptured = false, bLightingReleased = false, bOriginalRoomLamp = true;
 int32 HandPhase = 0, DirectMask = 0, GlassMask = 0, MarksMask = 0;
 double HandT = 0, ScreenK = 0, HandDwell = 0, LampDwell = 0;
 bool Build();
 bool Here() const;
 bool Held() const;
 bool VisiblePoint(const FVector& At, const FS47Obj& Target, double MaxDistance) const;
 bool ClearLine(const FVector& From, const FVector& To, const FS47Obj& Target, bool* bScreenHit = nullptr) const;
 bool InReach(const FString& Id) const;
 void Apply();
 void Changed();
};

#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
class US47Main;
class UMaterialInstanceDynamic;

// D45-20261110 (D45-17, Tom: "fremkalle foto: må lukke døra, slå på bryter for rødt lys før man får fremkalle"). Freeform's photo lab
// as a darkroom. One SAFELIGHT changeover switch inside the lab, by the door: up, the white tubes go out and the red safelights and the
// DARKROOM IN USE lamp outside come on; down, white light again. Developing and the fixer need power, the door shut and the safelight
// on (the wet bench and the enlarger ask the same rule here). Light that reaches a print in the developer (the door opened, or the
// switch back to white) ruins that print: it goes back to exposed film, the negative kept, and a new print costs nothing. A power cut
// and a loaded save never ruin anything. Freeform only: Story keeps its own darkroom (US47Chapter1::Darkroom). Saved as
// darkroom.safelight, absent in old saves (white, as before).
class FS47Darkroom
{
public:
 explicit FS47Darkroom(US47Main* InM) : M(InM) {}
 ~FS47Darkroom();
 void Begin(const S47J::FObj& Saved = nullptr);
 S47J::FObj Save() const;
 void Step(double Dt);
 void Stop();
 bool SafelightOn() const { return bSafe; }
 // Empty when developing may go on; else the one plain line to show.
 FString Block() const;
 // A print in the developer (LabStage 1): light now would ruin it.
 bool PrintAtRisk() const;
 // The switch. Tests pass bSilent (no toast, no click), as SetDoorOpen(..., instant); the rule (fog) still holds.
 void SetSafelight(bool bOn, bool bSilent);
 bool Use();
 FString Label() const;
 FVector InteractionPoint() const;
 // This film's last print was ruined by light: the bench offers a new print from the negative.
 bool WasFogged(const FString& Id) const { return Fogged.Contains(Id); }
 void Reprinted(const FString& Id) { Fogged.Remove(Id); }
 static const TCHAR* const SwitchId;
 // ---------- the pure rules (tested) ----------
 static FString BlockFor(bool bPower, bool bDoorOpen, bool bSafeOn);
 static bool FogsOn(bool bWasDoor, bool bDoorNow, bool bWasSafe, bool bSafeNow, bool bAtRisk);
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M = nullptr;
 TSharedPtr<uint8> Life = MakeShared<uint8>(0);
 bool bActive = false, bBuilt = false, bSafe = false, bApplied = false, bPendingApply = false, bPrimed = false, bLastDoor = false;
 int32 Shown = -1; // what Refresh last showed: -1 nothing yet, else (visible, safe) packed
 double PrimeWait = 0, Retry = 0;
 TSet<FString> Fogged;
 TArray<FS47Obj> Parts;
 FS47Obj Plate, Lever;
 TWeakObjectPtr<UMaterialInstanceDynamic> Pilot;
 bool Build();
 bool Available() const;
 void Apply();
 void Refresh(bool bForce);
 void Fog();
};

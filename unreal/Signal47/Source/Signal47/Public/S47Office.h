#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"

class US47Main;

// Operations is an actual room reached from the annex corridor. Its own spare key and
// physical papers never borrow Nora's key, REEL047's drawer or the survey-station file.
class FS47Office
{
public:
 explicit FS47Office(US47Main* InM);
 ~FS47Office();
 void Init();
 void Begin(const S47J::FObj& Saved = nullptr, bool bNewNight = false);
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Ready() const { return bReady; }
 bool HasKey() const { return bKey; }
 bool HasFieldSheet() const;
 bool WasUnlocked() const { return bKey && bUnlocked; }
 bool DoorOpen() const { return bReady && DoorK >= .995; }
 bool RouteRequired() const { return bRequired; }
 bool DepartureReleased() const { return bDeparted && HasFieldSheet(); }
 bool WantsDeparture() const;
 bool Contains(const FVector& P) const;
 bool InThreshold(const FVector& P, double Radius = 0) const;
 bool RestoreDoorwayAt(const FVector& P, double Radius);
 bool Use(const FString& Id);
 FS47Obj Object(const FString& Id) const;
 // The chapter host owns Nora, P04/P05, calls and its original end callback.
 TFunction<bool()> CanDepart;
 TFunction<void()> OnDepart;
private:
 struct FVisual;
 TUniquePtr<FVisual> Visual;
 US47Main* M = nullptr;
 bool bBuilt = false, bReady = false, bRequired = false;
 bool bKey = false, bUnlocked = false, bEverOpened = false, bEntered = false;
 bool bDoorInspected = false, bDoorTarget = false, bDrawerTarget = false;
 bool bDeskSeen = false, bIndexRead = false, bClippingsRead = false;
 bool bNoteRead = false, bContactRead = false, bFieldSheet = false, bDeparted = false;
 double DoorK = 0, DrawerK = 0, FieldSheetAt = 0, SyncWait = 0;
 bool Here() const;
 bool InReach(const FString& Id) const;
 bool ArchiveAvailable() const;
 void Apply();
 void SyncFacts();
 void OpenDocument(const FString& Id);
 void Changed();
};

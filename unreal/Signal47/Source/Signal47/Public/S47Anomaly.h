#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Docs.h"
#include "S47EveningPack.h"

class US47Main;
struct FS47AlbumShot;
struct FS47Task;

// Eksponering, vurdering og fysisk variant er forskjellige, vedvarende fakta.
struct FS47AnomalyData
{
 bool bEarlyCamera = false, bLegacyCutout = false, bRoutineTimeKnown = false;
 bool N01Routine = false, N01Compared = false, N01Followed = false;
 bool N02Observed = false, N02Control = false, N02Compared = false;
 bool A02Moved = false, A02Observed = false, A02Compared = false, A02Followed = false;
 bool P01Pending = false, P01Heard = false, P01Followed = false;
 bool bDatedReference = false;
 FString ReferencePhoto, ChangedPhoto, N02Photo, ControlPhoto;
 double RoutineClock = 0, ReferenceClock = 0, ChangedClock = 0;
 double N02Clock = 0, N02PhotoClock = 0, ControlClock = 0, P01Clock = 0;
 FString N01Assessment, N01Choice, N02Assessment, A02Assessment;
};

// Pilotens undersøkelsesbok bruker eksisterende album, modalpause og dokumenter.
// Ingen bildefiler eller gamle bildestempler rekonstrueres ved lasting.
class FS47Anomaly
{
public:
 explicit FS47Anomaly(US47Main* InM) : M(InM) {}
 FS47AnomalyData Data;
 void Begin(const S47J::FObj& Saved = nullptr);
 S47J::FObj Save() const;
 void Step(double Dt);
 bool Use(const FString& Id);
 void Photo(const FS47AlbumShot& Shot);
 void Open(const FString& Id = FString());
 bool OpenDoc(const FS47Doc& Doc);
 TArray<FS47Task> Tasks() const;
 // Verden kaller dette etter et faktisk blikk, kontroll eller fysisk etterspill.
 void Changed();
 // Samme konkrete valg som spillerknappene; også egnet til API-kontroll.
 bool Assess(const FString& Id, const FString& Choice);
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M = nullptr;
 FS47EveningPack Script;
 bool bScriptReady = false;
 FString Feedback;
 mutable FString ArchiveUrl;
 bool bArchiveUnavailable = false;
 double Clock() const;
 bool HasPrintout() const;
 const FS47AlbumShot* Shot(const FString& Id) const;
 const FS47AlbumShot* ReadyShot(const FString& Id) const;
 bool ShotUnavailable(const FString& Id) const;
 void Refile();
 void File(const FString& Id, const FString& Title, const FString& Page);
 FS47Doc ReferenceDoc() const;
 FS47EveningPack& Narrative(bool bRoutine = false);
 void Routine();
 bool StartComparison(FS47EveningPack& Pack);
 FString SceneText(const FString& Id) const;
 FString CompleteScene(FS47EveningPack& Pack);
 void Sync();
 void Log(const FString& Event) const;
};

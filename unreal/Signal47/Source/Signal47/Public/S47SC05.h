#pragma once
#include "CoreMinimal.h"
#include "S47Docs.h"
#include "S47Json.h"
class US47Main;
struct FS47Task;

// Unreal, approved SC05 revision (29 October): the existing residual is solved after SC04.
// Facts come from the ordinary phone, the real 47-second room event and physical encoder state.
class FS47SC05
{
public:
 explicit FS47SC05(US47Main* InM) : M(InM) {}
 void Begin(const S47J::FObj& Saved, bool bFresh);
 void Stop();
 S47J::FObj Save() const;
 bool Enabled() const { return bEnabled; }
 bool Active() const;
 bool Done() const { return bDone; }
 void Step(double Dt);
 void LineDead();
 void RoomEvent();
 void CeramicShattered();
 FString Label(const FString& Id) const;
 bool Use(const FString& Id);
 FString Objective() const;
 TArray<FS47Task> Tasks() const;
 double ActualInterval() const { return RoomAfter; }
 bool RoomCompared() const { return bCompared; }
 bool EncoderRead() const { return bEncoderRead; }
 bool Shattered() const { return bCeramic; }
 static bool SelfTest(TArray<FString>& Out);
 static bool CanAdoptSC04(const S47J::FObj& Saved);
private:
 US47Main* M = nullptr;
 bool bEnabled = false, bDone = false, bLineDead = false, bRoom = false, bCeramic = false;
 bool bCompared = false, bEncoderRead = false, bMoveLogged = false;
 double DeadAt = 0, RoomAt = 0, DeadGt = -1, RoomAfter = -1, LastGt = -1;
 TArray<FS47Doc> Papers;
 TArray<FString> Notes;
 void Keep(const FS47Doc& Doc);
 void Note(const FString& Text);
 void Refile();
 void MaybeFinish();
};

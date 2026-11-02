#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
struct FS47AlbumShot;

// Freeform schedules contextual, bounded world events. Story's authored scenes remain separate.
class FS47FreeformDirector
{
public:
 explicit FS47FreeformDirector(US47Main* Main);
 ~FS47FreeformDirector();
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Busy() const;
 bool Use(const FString& Id);
 FString Label(const FString& Id) const;
 void RoomLightInput(const FString& Room,int32 Serial,bool bOff);
 FString InspectRoomLight(const FString& Room);
 bool Force(const FString& Id); // explicit developer/test action; still needs a usable world
 void Photo(const FS47AlbumShot& Shot);
 const FString& ActiveEvent() const { return Event; }
 const FString& EventId() const { return Event; }
 FString StageName() const { return FString::Printf(TEXT("phase%d"),Stage); }
 bool CancelForTest();
 int32 EventStage() const { return Stage; }
 double EventAge() const { return Age; }
 FVector EventAnchor() const { return Anchor; }
 bool PhysicalObjectPresented() const; // actual component visibility: physical props/sky, including scene and adopted ancestors
 FString ObservationNote() const;
 FString Status() const;
 static TArray<FString> EventIds();
 static TArray<FString> CatalogIds() { return EventIds(); }
 static bool SelfTest(TArray<FString>& Lines);
private:
 struct FEffects;
 TUniquePtr<FEffects> Effects;
 US47Main* M=nullptr;
 bool bEnabled=false,bApplied=false,bEffectsApplied=false,bChoiceStored=false,bSeen=false,bPhoto=false,bControlled=false,bCompared=false;
 FString Event,PhotoId,Scene;
 int32 Stage=0,Variant=0,SeenDay=0,MinorLeft=0,InvestLeft=0,ThreatRollDay=0,MajorRollDay=0,LastThreatDay=0,LastMajorDay=-3;
 double Age=0,ActiveTime=0,NextAt=90,RestUntil=0,SoundAt=0,LastPhotoAt=-10;
 FVector Anchor=FVector::ZeroVector;
 FRandomStream Random;
 TMap<FString,int32> LastDays;
 S47J::FObj GroupRestore;
 FString ReactiveRoom;
 int32 ReactiveSerial=0;
 double ReactiveAge=0,ReactiveOffClock=0,ReactiveOnClock=0;
 bool bReactiveLeft=false,bReactiveApplied=false,bReactiveInspected=false;
 bool ReactiveValid() const;
 bool ReactiveReady() const;
 void StepReactive(double Dt);
 bool Start(const FString& Id,bool bForced);
 bool Eligible(const FString& Id,bool bForced) const;
 void Apply();
 void Draw();
 void Finish(bool bEvidence,const FString& Text=FString());
 void Changed();
 bool Near(const FVector& At,double Range) const;
 bool InView(const FVector& At,double Range=120) const;
 bool Night() const;
 void StepGroup(double Dt);
};

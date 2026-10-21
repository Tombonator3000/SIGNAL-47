#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "S47Audio.generated.h"

class UAudioComponent;
class USoundBase;

// The game's sounds, after web/src/core/Audio.ts: one-shots by name, loops by key, and the
// receiver's carrier, the anomaly's pulses and the dish motors as looping components whose
// volume follows the story. Sounds are looked up in /Game/Licensed/Sfx (the bought pack, if
// imported), /Game/Audio/Sfx (the free sounds) and /Game/Audio/Synth (tones made by
// Scripts/make-synth.py). A missing sound is logged once and skipped; the game goes on.
UCLASS()
class SIGNAL47_API US47Audio : public UObject
{
	GENERATED_BODY()

public:
	void Init(UWorld* InWorld);
	void Tick(float DeltaSeconds);

	void Play(const FString& Name, float Gain = 1.f, const FVector& At = FVector::ZeroVector, float Rate = 1.f);
	void Loop(const FString& Key, const FString& Name, float Gain = 1.f, const FVector& At = FVector::ZeroVector);
	void Stop(const FString& Key, float Fade = 0.05f);
	void Pour(const FVector& At);
	void StartCarrier();
	void SetCarrier(float Level, float PitchOffset, float StaticLevel);
	void SignalFrame(float Clarity, float Level);
	void SignalOff();
	void StartMotors();
	void StopMotors();
	void Boom(float Strength);
	void HoldRoomTone(float Level);
	void StartRoomTone();
	double FutureCall(); // the call from the future: its sounds in order, returns how long it is
	void AmRadio(bool bOn, const FVector& At);
	void AmRadioJam(bool bOn);

private:
	TWeakObjectPtr<UWorld> World;
	UPROPERTY() TMap<FString, TObjectPtr<USoundBase>> Cache;
	UPROPERTY() TMap<FString, TObjectPtr<UAudioComponent>> Loops;
	TSet<FString> Missing;
	float RoomToneTarget = 1.f, RoomToneLevel = 1.f;
	USoundBase* Find(const FString& Name);
	UAudioComponent* LoopComponent(const FString& Key, const FString& Name, float Gain, const FVector& At, bool b2D);
};

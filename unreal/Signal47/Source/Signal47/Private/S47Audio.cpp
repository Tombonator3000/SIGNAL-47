#include "S47Audio.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Audio, Log, All);

void US47Audio::Init(UWorld* InWorld)
{
	World = InWorld;
}

USoundBase* US47Audio::Find(const FString& Name)
{
	if (TObjectPtr<USoundBase>* C = Cache.Find(Name)) return C->Get();
	static const TCHAR* Folders[] = { TEXT("/Game/Licensed/Sfx"), TEXT("/Game/Audio/Sfx"), TEXT("/Game/Audio/Synth"), TEXT("/Game/Audio/Music"), TEXT("/Game/Audio/Radio") };
	for (const TCHAR* F : Folders)
	{
		if (USoundBase* S = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("%s/%s.%s"), F, *Name, *Name)))
		{
			Cache.Add(Name, S);
			return S;
		}
	}
	if (!Missing.Contains(Name))
	{
		Missing.Add(Name);
		UE_LOG(LogS47Audio, Warning, TEXT("S47: no sound named %s (run Scripts/import-audio.sh)"), *Name);
	}
	return nullptr;
}

void US47Audio::Play(const FString& Name, float Gain, const FVector& At, float Rate)
{
	USoundBase* S = Find(Name);
	if (!S || !World.IsValid()) return;
	if (At.IsNearlyZero()) UGameplayStatics::PlaySound2D(World.Get(), S, Gain, Rate);
	else UGameplayStatics::PlaySoundAtLocation(World.Get(), S, At, Gain, Rate);
}

UAudioComponent* US47Audio::LoopComponent(const FString& Key, const FString& Name, float Gain, const FVector& At, bool b2D)
{
	Stop(Key, 0.f);
	USoundBase* S = Find(Name);
	if (!S || !World.IsValid()) return nullptr;
	UAudioComponent* C = b2D ? UGameplayStatics::SpawnSound2D(World.Get(), S, Gain, 1.f, 0.f, nullptr, true, false)
		: UGameplayStatics::SpawnSoundAtLocation(World.Get(), S, At, FRotator::ZeroRotator, Gain, 1.f, 0.f, nullptr, nullptr, false);
	if (C)
	{
		C->bAutoDestroy = false;
		Loops.Add(Key, C);
	}
	return C;
}

void US47Audio::Loop(const FString& Key, const FString& Name, float Gain, const FVector& At)
{
	LoopComponent(Key, Name, Gain, At, At.IsNearlyZero());
}

void US47Audio::Stop(const FString& Key, float Fade)
{
	if (TObjectPtr<UAudioComponent>* C = Loops.Find(Key))
	{
		if (C->Get())
		{
			if (Fade > 0.f) (*C)->FadeOut(Fade, 0.f); else (*C)->Stop();
		}
		Loops.Remove(Key);
	}
}

void US47Audio::Pour(const FVector& At) { Play(TEXT("pour"), 0.8f, At); }

void US47Audio::StartCarrier()
{
	if (!Loops.Contains(TEXT("carrier"))) LoopComponent(TEXT("carrier"), TEXT("carrier_hum"), 0.f, FVector::ZeroVector, true);
	if (!Loops.Contains(TEXT("static"))) LoopComponent(TEXT("static"), TEXT("static_noise"), 0.f, FVector::ZeroVector, true);
}

void US47Audio::SetCarrier(float Level, float PitchOffset, float StaticLevel)
{
	if (TObjectPtr<UAudioComponent>* C = Loops.Find(TEXT("carrier"))) if (C->Get()) { (*C)->SetVolumeMultiplier(Level * 4.f); (*C)->SetPitchMultiplier(1.f + PitchOffset * 4.f); }
	if (TObjectPtr<UAudioComponent>* C = Loops.Find(TEXT("static"))) if (C->Get()) (*C)->SetVolumeMultiplier(StaticLevel * 6.f);
}

void US47Audio::SignalFrame(float Clarity, float Level)
{
	// the anomaly: static that turns into the pulsing signal as the receiver closes on it
	if (!Loops.Contains(TEXT("signal"))) LoopComponent(TEXT("signal"), TEXT("signal_pulses"), 0.f, FVector::ZeroVector, true);
	if (TObjectPtr<UAudioComponent>* C = Loops.Find(TEXT("signal"))) if (C->Get()) (*C)->SetVolumeMultiplier(Clarity * Level * 0.6f);
	if (TObjectPtr<UAudioComponent>* C = Loops.Find(TEXT("static"))) if (C->Get()) (*C)->SetVolumeMultiplier((1.f - Clarity) * Level * 0.5f);
}

void US47Audio::SignalOff() { Stop(TEXT("signal"), 0.1f); }

void US47Audio::StartMotors() { LoopComponent(TEXT("motors"), TEXT("motors"), 0.7f, FVector::ZeroVector, true); }
void US47Audio::StopMotors() { Stop(TEXT("motors"), 0.6f); }

void US47Audio::Boom(float Strength) { Play(TEXT("boom"), Strength); }

void US47Audio::StartRoomTone()
{
	LoopComponent(TEXT("room"), TEXT("room_tone"), 0.35f, FVector::ZeroVector, true);
}

void US47Audio::HoldRoomTone(float Level) { RoomToneTarget = Level; }

void US47Audio::Tick(float DeltaSeconds)
{
	RoomToneLevel += (RoomToneTarget - RoomToneLevel) * FMath::Min(1.f, DeltaSeconds * 8.f);
	if (TObjectPtr<UAudioComponent>* C = Loops.Find(TEXT("room"))) if (C->Get()) (*C)->SetVolumeMultiplier(0.35f * RoomToneLevel);
}

double US47Audio::FutureCall()
{
	// Static, a room with a printer in it, a heavy thud, a gasp, something ceramic breaking.
	if (!World.IsValid()) return 6.0;
	FTimerManager& TM = World->GetTimerManager();
	Play(TEXT("static_noise"), 0.5f);
	FTimerHandle H1, H2, H3, H4;
	TM.SetTimer(H1, [this] { Play(TEXT("printer"), 0.25f, FVector::ZeroVector, 1.1f); }, 0.6f, false);
	TM.SetTimer(H2, [this] { Play(TEXT("thud_soft"), 0.9f, FVector::ZeroVector, 0.5f); }, 2.0f, false);
	TM.SetTimer(H3, [this] { Play(TEXT("ceramic"), 0.7f); }, 2.7f, false);
	TM.SetTimer(H4, [this] { Play(TEXT("click"), 0.6f); }, 5.6f, false);
	return 5.8;
}

void US47Audio::AmRadio(bool bOn, const FVector& At)
{
	if (bOn) LoopComponent(TEXT("amradio"), TEXT("radio_static"), 0.35f, At, false);
	else Stop(TEXT("amradio"), 0.1f);
}

void US47Audio::AmRadioJam(bool bOn)
{
	if (TObjectPtr<UAudioComponent>* C = Loops.Find(TEXT("amradio"))) if (C->Get()) (*C)->SetVolumeMultiplier(bOn ? 0.7f : 0.35f);
}

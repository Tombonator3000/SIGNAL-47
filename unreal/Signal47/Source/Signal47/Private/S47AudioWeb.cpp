// core/Audio.ts by its own names. The recordings play by name now; the sounds the web version
// makes with Web Audio (beeps, hums, the telephone line, the radios) are ported one by one;
// until one is, it logs once and stays silent, as the web version does without a recording.
#include "S47Audio.h"

#include "S47Three.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47AudioWeb, Log, All);

void US47Audio::NotYet(const TCHAR* What) const
{
	static TSet<FString> Told;
	if (!Told.Contains(What)) { Told.Add(What); UE_LOG(LogS47AudioWeb, Display, TEXT("S47: audio.%s is not ported yet"), What); }
}

void US47Audio::Sfx(const FString& Name, const FS47Sound& O)
{
	Play(Name, O.Gain, O.At ? S47Three::ToUE(*O.At) : FVector::ZeroVector, O.Rate);
}

FString US47Audio::Or(const FString& Name, const FString& Old) const
{
	// the bought pack's recording when it is imported, else the old name
	return Old.IsEmpty() ? Name : Old;
}

void US47Audio::LoopSfx(const FString& Key, const FString& Name, const FS47Sound& O)
{
	Loop(Key, Name, O.Gain, O.At ? S47Three::ToUE(*O.At) : FVector::ZeroVector);
}

void US47Audio::Beep(float, float, float, float, const FString&) { NotYet(TEXT("beep")); }
void US47Audio::Hum(const FString&, const FVector&, float, float) { NotYet(TEXT("hum")); }
void US47Audio::StopHums() {}
void US47Audio::HumLevel(float, float) {}
void US47Audio::Servo(float, TOptional<FVector>) { NotYet(TEXT("servo")); }
void US47Audio::Signature(float) { NotYet(TEXT("signature")); }
void US47Audio::Shutter(const FString&, float) { NotYet(TEXT("shutter")); }
void US47Audio::PourThree(TOptional<FVector> At) { Pour(At ? S47Three::ToUE(*At) : FVector::ZeroVector); }
void US47Audio::Spatial(const FString& Name, const FVector& At, float) { Play(Name, 1.f, S47Three::ToUE(At)); }
TOptional<FS47CallTimeline> US47Audio::PhoneCall(const TArray<FS47PhoneCue>&, float, float) { NotYet(TEXT("phoneCall")); return {}; }
void US47Audio::StopPhoneCalls() { bPhonePlaying = false; }
void US47Audio::CallLine(float) { NotYet(TEXT("callLine")); }
float US47Audio::Dial(const FString& Number) { NotYet(TEXT("dial")); return Number.Len() * 0.35f; }
float US47Audio::Ringback(int32 Times, float) { NotYet(TEXT("ringback")); return Times * 4.f; }
void US47Audio::Radio(TOptional<FVector>, float) { NotYet(TEXT("radio")); }
void US47Audio::RadioVoice(float, float) { NotYet(TEXT("radioVoice")); }
void US47Audio::TvHiss(TOptional<FVector>) { NotYet(TEXT("tvHiss")); }
void US47Audio::RelayClack(const FVector&, bool) { NotYet(TEXT("relayClack")); }
void US47Audio::Contactor(const FVector&, float) { NotYet(TEXT("contactor")); }
void US47Audio::PowerDown(const FVector&) { NotYet(TEXT("powerDown")); }
void US47Audio::PowerUp(const FVector&) { NotYet(TEXT("powerUp")); }
void US47Audio::TelexMotor(TOptional<FVector>) { NotYet(TEXT("telexMotor")); }
void US47Audio::TypeBar(const FVector&, bool) { NotYet(TEXT("typeBar")); }
void US47Audio::CabRadio(const FString&) { NotYet(TEXT("cabRadio")); }
void US47Audio::CabLevel(float) {}
void US47Audio::CabVoice(float, float) { NotYet(TEXT("cabVoice")); }
void US47Audio::CupDown(bool) { NotYet(TEXT("cupDown")); }
void US47Audio::Pressure(bool) { NotYet(TEXT("pressure")); }
void US47Audio::Silence(bool) { NotYet(TEXT("silence")); }
void US47Audio::SetSpace(const FString&) {}
void US47Audio::Step(const FString&) { NotYet(TEXT("step")); }
void US47Audio::Gasp(float) { NotYet(TEXT("gasp")); }
void US47Audio::Thunder(float) { NotYet(TEXT("thunder")); }
void US47Audio::HoldThunder(float) {}
void US47Audio::DuckAll(float) {}

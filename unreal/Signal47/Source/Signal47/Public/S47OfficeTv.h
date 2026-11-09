#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"
#include "S47TvProgramme.h"
#include "UObject/StrongObjectPtr.h"

class US47Main;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UTexture2D;
class FS47GainNode;
class FS47BiquadFilterNode;

// TV-SCHEDULE-20261109 (Tom: «Unarius-klippet skal også vises på andre tv-er», the set in Dale's office as the example). The KELVORA
// set on the shelf in Operations (S47Office.cpp, office:fieldDeck) is left on, on the late station: the programme in its hours
// (FS47TvProgramme::OnAir, 23:30 to 03:30), snow the rest of the day. The picture, the card and the snow are drawn as the motel
// office's set draws them (S47MotelTv: its card, its tube's scan lines and its little speaker; S47Room6: its snow), the sound from
// the set itself, duller and quieter from the corridor. Without the local frame pack (WebData/tv/unarius.s47tv: the clip is kept
// only locally, its rights not cleared) the set shows the test card in the programme's hours and snow otherwise; nothing borrowed
// is needed. In Freeform it is dark while SARO has no power. Shared room mechanics: FS47Office steps it in both modes.
class SIGNAL47_API FS47OfficeTv
{
public:
	FS47OfficeTv(US47Main* InM, UStaticMeshComponent* InSet, const FS47Obj& InObj, TFunction<bool()> InInside);
	~FS47OfficeTv();
	void Step(double Dt);
	void Silence();
	/** What it shows now: "programme", "card", "snow" or "off" (and the programme's picture and sound). */
	FString State() const;
	enum class EShow : uint8 { Off, Programme, Card, Snow };
	/** What a set on the late station shows at this clock: the programme (or its card without the frame pack) in the station's
	 *  hours, snow otherwise; nothing without power. */
	static EShow ShowAt(double Clock, bool bPowered, bool bHaveProgramme, bool bForced);
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	TWeakObjectPtr<UStaticMeshComponent> Set;
	FS47Obj Obj;
	TFunction<bool()> Inside;
	TWeakObjectPtr<UMaterialInstanceDynamic> Screen, Pilot;
	TWeakObjectPtr<UPointLightComponent> Glow;
	TStrongObjectPtr<UTexture2D> Tube;
	TArray<FColor> Card;
	FS47TvProgramme Programme;
	EShow Show = EShow::Off;
	bool bShowing = false;   // the programme shown last frame (its clock is set to the station's when it comes back on)
	int32 PaintedFrame = -1;
	double PaintT = 0, Lum = 0, Light = 0;
	struct FSnd { TSharedPtr<FS47GainNode> Tone, Hiss, Whine; TSharedPtr<FS47BiquadFilterNode> Wall; TFunction<void()> Stop; };
	TOptional<FSnd> Snd;
	double Clock() const;
	bool Powered() const;
	FVector Speaker() const;
	void Paint();
	void Upload(const TArray<FColor>& Pixels);
	void Sound(double Dist, bool bThrough);
	void Hush();   // the tone, the hiss and the whine stop (the programme's own sound is FS47TvProgramme's)
	void SetScreen(bool bOn);
};

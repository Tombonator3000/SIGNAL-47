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
class FS47OscillatorNode;

// TV-SCHEDULE-20261109 (Tom: «Unarius-klippet skal også vises på andre tv-er», the set in Dale's office as the example). The KELVORA
// set on the shelf in Operations (S47Office.cpp, office:fieldDeck) is left on, on the late station: the programme in its hours
// (FS47TvProgramme::OnAir, 23:30 to 03:30), snow the rest of the day. The picture, the card and the snow are drawn as the motel
// office's set draws them (S47MotelTv: its card, its tube's scan lines and its little speaker; S47Room6: its snow), the sound from
// the set itself, duller and quieter from the corridor. Without the local frame pack (WebData/tv/unarius.s47tv: the clip is kept
// only locally, its rights not cleared) the set shows the test card in the programme's hours and snow otherwise; nothing borrowed
// is needed. In Freeform it is dark while SARO has no power. Shared room mechanics: FS47Office steps it in both modes.
// TV-CHANNEL-20261110 (Tom: «Dale tv må ha av på knapp, alle tver spiller av unarius på en kanal, static eller test bilde når
// sender slutt, ved sende slutt amerikanske nasjonalsangen og flagget; hvis tv står på ved ufo aliens eller lignende er tv static
// og blafring med rare lyder innimellom»). In Freeform the set is on the one channel (FS47TvProgramme::Air: the programme through
// the broadcast day, the sign-off, the card, snow), it has its switch (Use; the office keeps and saves it), and what is on the air
// around the station (the screens' own measure, FS47FreeformSession::ScreenInterference) breaks its picture up: tearing, a rolling
// frame, snow through the picture, a flicker on the room, the hiss up and a tone that slides about. The story's set is as it was.
class SIGNAL47_API FS47OfficeTv
{
public:
	FS47OfficeTv(US47Main* InM, UStaticMeshComponent* InSet, const FS47Obj& InObj, TFunction<bool()> InInside, bool* InOff = nullptr);
	~FS47OfficeTv();
	void Step(double Dt);
	void Silence();
	/** What it shows now: "programme", "card", "snow" or "off" (and the programme's picture and sound). */
	FString State() const;
	enum class EShow : uint8 { Off, Programme, Card, Snow, SignOff };
	/** What a set on the late station shows at this clock: the programme (or its card without the frame pack) in the station's
	 *  hours, snow otherwise; nothing without power. */
	static EShow ShowAt(double Clock, bool bPowered, bool bHaveProgramme, bool bForced);
	/** Freeform: what a set on the one channel shows: nothing without power or switched off; the programme (the card without the
	 *  frame pack), the sign-off, the card or snow as the station has it. */
	static EShow ShowOn(FS47TvProgramme::EAir Air, bool bPowered, bool bSwitched, bool bHaveProgramme);
	/** How much of the picture is lost at this moment (0 to 1) for what is on the air near the set (0 to 1): nothing under a
	 *  trace of it, then in gusts, the whole picture gone to snow at the worst of the strongest. */
	static double Break(double Jam, double T);
	static bool SelfTest(TArray<FString>& Lines);
	/** The set's switch (Freeform; the player's Use from inside the room, and F10). */
	bool Switched() const { return !Off || !*Off; }
	void Switch();

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
	struct FSnd { TSharedPtr<FS47GainNode> Tone, Hiss, Whine; TSharedPtr<FS47BiquadFilterNode> Wall; TSharedPtr<FS47OscillatorNode> Osc; TFunction<void()> Stop; };
	TOptional<FSnd> Snd;
	// TV-CHANNEL-20261110: the switch (the office's, saved with it), the tube coming up and going down, what is on the air
	bool* Off = nullptr;
	bool bLookOnly = true;
	double Warm = 1, Fade = 0, Jam = 0, Lost = 0, OddNext = 0, OddUntil = 0;
	bool bOdd = false;
	void Disturb(TArray<FColor>& Px);
	void PaintDot();
	double Clock() const;
	bool Powered() const;
	FVector Speaker() const;
	void Paint();
	void Upload(const TArray<FColor>& Pixels);
	void Sound(double Dist, bool bThrough);
	void Hush();   // the tone, the hiss and the whine stop (the programme's own sound is FS47TvProgramme's)
	void SetScreen(bool bOn);
};

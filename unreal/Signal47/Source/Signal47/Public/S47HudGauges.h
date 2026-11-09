#pragma once

#include "CoreMinimal.h"

// The HUD's small instruments (HUD-20261108, HUD D from HUD-D-20261109), as plain functions so they can be tested without a
// screen: the flashlight's battery as five strokes, like the mark on an old battery (no numbers), the radio line (who is
// sending, and a small wave for the signal), the segment clock's face, the task's cassette card and the HUD's colours, which
// the player picks in Settings. SS47Hud reads the game and paints; the choices are made here.
namespace S47HudGauges
{
	constexpr int32 BatteryStrokes = 5;

	/** How many of the five strokes are lit: 0 only when empty, 1 for anything left, 5 for a full battery. */
	SIGNAL47_API int32 Strokes(double Level);
	/** The mark shows only while the lamp is on or the battery is low, and only where the lamp has a battery (Freeform). */
	SIGNAL47_API bool BatteryShown(bool bDrains, bool bHave, bool bOn, double Level, double Low);

	/** What the radio line says. The CB under the truck's dash and at SARO's base set: CB 19. The carried walkie (CB-WALKIE-20261108:
	 *  Dale's): its channel and who is sending, "WALKIE 1 · DALE". The two are told apart by these words; both are drawn in the
	 *  palette's one radio colour (HUD-D-20261109). */
	SIGNAL47_API FString CbLine();
	SIGNAL47_API FString WalkieLine(int32 Channel = 1, const FString& Who = FString());
	/** A walkie caption's head ("WALKIE 1, DALE"): its channel and who speaks. False for anything else, and for the set's own head
	 *  with nobody speaking ("WALKIE 2", a turn of the knob). */
	SIGNAL47_API bool WalkieSpeaker(const FString& Head, int32& Channel, FString& Who);
	/** Which line shows: 0 none, 1 the CB, 2 the walkie. Only while somebody is sending: a call on the CB where a CB is, Dale's hail,
	 *  the player keyed, or words still coming over the set (SpeechLeft seconds). Having the set, or having just turned it, is not
	 *  sending. */
	SIGNAL47_API int32 RadioKind(bool bCbBusy, bool bCbAtHand, bool bWalkieOwned, bool bHailing, bool bKeyed, double SpeechLeft);
	/** Who the walkie line names: the player while keyed, Dale while he hails, else whoever is speaking. */
	SIGNAL47_API FString WalkieWho(bool bHailing, bool bKeyed, const FString& Speaker);
	/** The CB's signal while a call is on (steady, a little unsteady). T in seconds. */
	SIGNAL47_API double CbStrength(double T);

	/** One point of the wave, -1 to 1: X from 0 to 1 along the line, Phase moving, Strength 0 to 1 sets the swing. A weak signal is
	 *  small and ragged, a strong one is a clean wave. Zero at both ends. */
	SIGNAL47_API double Wave(double X, double Phase, double Strength);

	/** SAVE-SARO-20261108: the small stamp over the gauges when the autosave on leaving SARO is written. LOG FILED and the clock the
	 *  save's card has (S47SaroSave.h). */
	SIGNAL47_API FString FiledLine(const FString& Clock);
	/** How long the stamp shows, and its strength T seconds after the save: in over a quarter second, held, out over the last 0.8. */
	constexpr double FiledFor = 4.5;
	SIGNAL47_API double FiledAlpha(double T);

	// ---------- HUD D (HUD-D-20261109) ----------

	/** The HUD's colours, chosen in Settings > Picture > HUD color and kept with the other settings ("hudColor"). Colours are
	 *  sRGB (0xRRGGBB), strengths 0 to 1. The clock's lit and ghost segments; the battery's frame, lit and ghost strokes, dimmed to
	 *  the clock's level; the low battery in a colour of its own at full strength, so it always stands apart; the radio line's
	 *  words and wave, one colour for the CB and the walkie. */
	enum class EHudColor : uint8 { StationTeal, Amber, PhosphorGreen, PaperWhite, RedNight };
	constexpr int32 HudColors = 5;
	struct FHudPalette
	{
		const TCHAR* Id;
		const TCHAR* Label;
		const TCHAR* Help;
		uint32 Clock; float ClockOn, ClockGhost;
		uint32 Battery; float BatteryFrame, BatteryOn, BatteryGhost;
		uint32 Low; float LowFrame, LowOn, LowGhost;
		uint32 Radio; float RadioText, RadioWave;
	};
	SIGNAL47_API const FHudPalette& Palette(EHudColor Color);
	/** The palette the player has chosen (Station teal until one is set). */
	SIGNAL47_API const FHudPalette& Palette();
	SIGNAL47_API EHudColor HudColor();
	SIGNAL47_API void SetHudColor(EHudColor Color);
	SIGNAL47_API FString HudColorId(EHudColor Color);
	/** The palette an id names ("teal", "amber", "green", "white", "red"); anything else is Station teal. */
	SIGNAL47_API EHudColor HudColorFrom(const FString& Id);
	/** How far apart two sRGB colours are, 0 to about 1.73 (the plain distance of their channels, 0 to 1 each). */
	SIGNAL47_API double ColorDistance(uint32 A, uint32 B);

	/** The segment clock's face (12 hours with AM and PM, as American clocks were) from the watch's words: "23:41", or "23:41:07"
	 *  while a warning counts its seconds, or "88:88" while something near makes the watch jump (all segments lit). */
	struct FClockFace { bool bValid = false, bAll = false, bPm = false; int32 Hour = 12, Minute = 0, Second = -1; };
	SIGNAL47_API FClockFace ClockFace(const FString& Watch);
	/** The segments a figure lights, a to g as bits 0 to 6 (a top, b top right, c bottom right, d foot, e bottom left, f top left,
	 *  g middle); 0 for anything that is not 0 to 9. */
	SIGNAL47_API uint8 Segments(int32 Digit);

	/** The task's cassette card: it comes in when the task changes, stands six seconds and fades out over one. NoteAlpha: its
	 *  strength T seconds after the change. */
	constexpr double NoteIn = 0.25, NoteHold = 6.0, NoteFade = 1.0;
	SIGNAL47_API double NoteAlpha(double T);
	/** A task in capitals as the player's hand writes it on the card: Title for the place ("Caliche Junction"), sentence case for
	 *  the task ("Back to the truck. Keep the record"). Figures and the station's capitals (SARO, CB, MHz) stay as they are; words
	 *  already written in small letters are left alone. */
	SIGNAL47_API FString Handwritten(const FString& Text, bool bTitle);
	/** HUD-D2-20261109: how strong the dark rim under the clock, the day and the battery is, 0 to 1, from the sky's daylight (0 night,
	 *  1 full day). Faint at night, where the dark sky is backing enough and the HUD looks as Tom chose it; full against a bright sky,
	 *  where the teal would wash out. */
	SIGNAL47_API double BackingStrength(double Daylight);

	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}

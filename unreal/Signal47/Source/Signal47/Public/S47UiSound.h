#pragma once
#include "CoreMinimal.h"

class US47Audio;
class FS47AudioBuffer;

// FIX10-7 (14 Nov 2026): every press the player makes on SARO's machines has a small mechanical answer, and a TV left on is
// heard as static. Baked from the bought bundle by Tools/uisound/build_uisound.py into Content/Audio/Raw/media/uisound (ignored by
// Git, read here at first use, its manifest uisound.json beside it). Without the files every call says false and the caller plays
// today's sound: the free click, the switches, the synthesised hiss (CI and other machines have none of them).
//
// One-shots go to the SFX bus (under the master volume), each kind's files in turn and never the same one twice running, at a
// gain that keeps them at or under the sound they replace (the table in S47UiSound.cpp, the same as the build script's KINDS).
// At most 12 a second (and none closer than 1/24 s): a held key or a dragged knob does not machine-gun. While the pause menu is
// open only its own two kinds sound; nothing sounds in a story cinematic. Hover never sounds here.
namespace S47UiSound
{
	enum class EKind : uint8
	{
		Key,       // a key on the three SARO PCs, the receiver's step keys (FS47FreeformSession TerminalKey, kind 0)
		Tab,       // a tab or a menu line on a PC (kind 2)
		Action,    // a PC's primary key: RETAIN TAKE, START RECORDING (kind 1)
		Detent,    // a TUNE dial's or a slider's step (kind 3)
		Transport, // the reel deck's transport keys and its reference keys
		Mark,      // the reel deck's MARK IN and MARK OUT
		Retain,    // the reel deck's RETAIN (the Action's files, heavier)
		Page,      // a page of the notebook or the binder turned
		MenuMove,  // the pause menu: the selection moved
		MenuOk,    // the pause menu: a line chosen
		Count
	};
	enum class ETv : uint8 { Thunk, PowerOn, PowerOff, Count };
	// which of the TVs' filter chains the static goes through (its lift to today's hiss level differs)
	enum class EChain : uint8 { Office, Motel };

	/** Plays a kind. False only when it has no files (the caller then plays today's sound); true also when it was held quiet
	 *  (the rate, the pause, a cinematic). bDeferred: it starts 30 ms late, so that Retag in the same frame may still name it.
	 *  Audio null: the game's (US47Main). */
	SIGNAL47_API bool Play(US47Audio* Audio, EKind Kind, bool bDeferred = false);
	/** The reel deck: its panel's keys report only "a key" (FS47FreeformAudioDesk::Use then knows which); the press made this frame
	 *  with bDeferred, not yet sounding, is given this kind's sound instead. Nothing otherwise. */
	SIGNAL47_API void Retag(US47Audio* Audio, EKind Kind);
	SIGNAL47_API bool Has(EKind Kind);
	/** The static bed (a loop), to stand in for the white noise a TV's hiss is made of, or null. */
	SIGNAL47_API TSharedPtr<FS47AudioBuffer> TvBed();
	/** The gain the bed needs on that chain to be exactly as loud as the noise it stands in for (from the manifest; 1 to 4). */
	SIGNAL47_API double TvBedLift(EChain Chain);
	/** A TV's channel change or its power going on or off, at the set (three.js metres), through a low-pass (0: none). */
	SIGNAL47_API bool Tv(US47Audio* Audio, ETv What, const FVector& AtThree, float Gain, float Lp = 0.f);
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}

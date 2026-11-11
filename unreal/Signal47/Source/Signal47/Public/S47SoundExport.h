#pragma once

#include "CoreMinimal.h"
#include "S47Audio.h"
#include "UObject/StrongObjectPtr.h"

// N1110 (dev only, not in shipping): -S47ExportSounds=<dir> renders today's synthesised sounds offline into 16-bit 48 kHz WAVE files
// (and an index.txt that says where each one plays), then lets the game go on; -S47ExportSoundsQuit exits after the export.
// Every clip runs the game's own code (FS47CB, FS47EncounterSound, S47AlienKinds, FS47ReceiverSound, US47Audio::Step and Beep) on a
// shadow of the live audio: a fresh context at time zero with the same buffers and bus levels (US47Audio::MakeOffline). Nothing
// is normalised: what is written is what the master would send out.
namespace S47SoundExport
{
	struct FClip { FString File; FString Where; TArray<float> Pcm; }; // 48 kHz, two channels interleaved

	/** One offline run: a shadow of Live on a fresh context; Go renders on from where it stands into Pcm. */
	struct SIGNAL47_API FRun
	{
		explicit FRun(const US47Audio& Live);
		FRun(const FRun&) = delete;
		FRun& operator=(const FRun&) = delete;
		US47Audio* Audio() const { return A.Get(); }
		void Go(double Seconds);
		TStrongObjectPtr<US47Audio> A;
		TArray<float> Pcm;
	};

	/** At startup: does the export when -S47ExportSounds=<dir> is on the command line. True when it ran. */
	SIGNAL47_API bool RunIfAsked(const US47Audio& Live);
	/** S47Happenings.cpp: the Geiger counter at three doses and the scorched ring's crackle. */
	SIGNAL47_API void HappeningClips(const US47Audio& Live, TArray<FClip>& Out);
}

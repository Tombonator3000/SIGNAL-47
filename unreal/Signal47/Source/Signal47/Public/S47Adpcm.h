#pragma once

#include "CoreMinimal.h"

// SIDEBAND-STEMS-ADPCM (14 November 2026): IMA ADPCM, 4 bits a sample, in the standard WAVE layout (wFormatTag 0x11, as every
// audio tool reads it). FS47MediaSource plays such a file beside 16-bit PCM (S47Media.cpp); Tools/music/ima_adpcm.py writes them
// and holds a bit-exact mirror of this decoder.
//
// A block is BlockAlign bytes: per channel a 4-byte header (the block's first sample as int16, the step index 0..88, a zero byte),
// then 4-byte words channel after channel, each word 8 samples of its channel, the low nibble of each byte first. Every block
// starts from its own header, so a sample's value depends on its block alone: a seek is a jump to the block and a decode forward
// in it, and a deck that seeks gives exactly the samples of one that played through. A nibble's step is
// ((2 * (code & 7) + 1) * step) >> 3, the rounding of ffmpeg's adpcm_ima_wav (bit-exact with it; Windows' ACM codec, which adds
// shifted steps, decodes the same file within some tens of LSB).
namespace S47Adpcm
{
	inline constexpr uint16 FormatTag = 0x11;
	inline constexpr int32 MaxBlockAlign = 1 << 15; // (a sanity bound for a header: the staged stems use 512 bytes a channel)
	// Frames in a block of Bytes bytes (a whole block or a shorter last one); 0 when Bytes is not 4 * Channels times a whole
	// number of words plus one header.
	SIGNAL47_API int32 FramesIn(int32 Bytes, int32 Channels);
	// One block (whole or a shorter last one) to Frames x Channels interleaved int16. Returns the frames written, or -1 when the
	// block cannot be one (its size, or a step index over 88 in a header).
	SIGNAL47_API int32 DecodeBlock(const uint8* Block, int32 Bytes, int32 Channels, int16* Out);
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);

	// For the self-tests only (the game never encodes; the staged files are Tools/music/ima_adpcm.py's): a plain encoder (each
	// nibble the nearest of the 16, the block's step index carried over from the block before), and a WAVE around its blocks.
	namespace Test
	{
		SIGNAL47_API void Encode(const int16* Pcm, int32 Frames, int32 Channels, int32 BlockAlign, TArray<uint8>& Data);
		SIGNAL47_API TArray<uint8> Wave(const TArray<uint8>& Data, int32 Channels, int32 Rate, int32 BlockAlign, int32 Frames);
	}
}

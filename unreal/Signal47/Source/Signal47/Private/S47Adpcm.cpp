// SIDEBAND-STEMS-ADPCM: the IMA ADPCM block decoder FS47MediaSource reads the 4-bit stems with (S47Adpcm.h).
#include "S47Adpcm.h"

#include "Math/RandomStream.h"

namespace S47AdpcmTables
{
	static const int32 Steps[89] = {
		7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
		157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411,
		1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493,
		10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767 };
	static const int32 IndexStep[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };

	// one nibble: the predictor and the step index after it
	FORCEINLINE void Nibble(int32 Code, int32& Pred, int32& Index)
	{
		const int32 Diff = ((2 * (Code & 7) + 1) * Steps[Index]) >> 3;
		Pred = FMath::Clamp(Code & 8 ? Pred - Diff : Pred + Diff, -32768, 32767);
		Index = FMath::Clamp(Index + IndexStep[Code], 0, 88);
	}
}

int32 S47Adpcm::FramesIn(int32 Bytes, int32 Channels)
{
	if (Channels < 1 || Channels > 2 || Bytes < 4 * Channels || Bytes > MaxBlockAlign || (Bytes - 4 * Channels) % (4 * Channels)) return 0;
	return (Bytes - 4 * Channels) / (4 * Channels) * 8 + 1;
}

int32 S47Adpcm::DecodeBlock(const uint8* Block, int32 Bytes, int32 Channels, int16* Out)
{
	const int32 Frames = FramesIn(Bytes, Channels);
	if (Frames <= 0 || !Block || !Out) return -1;
	const int32 Words = (Frames - 1) / 8;
	for (int32 C = 0; C < Channels; ++C)
	{
		const uint8* H = Block + 4 * C;
		int32 Pred = int32(int16(uint16(H[0]) | uint16(uint16(H[1]) << 8)));
		int32 Index = H[2];
		if (Index > 88) return -1;
		Out[C] = int16(Pred);
		int32 At = 1;
		for (int32 W = 0; W < Words; ++W)
		{
			const uint8* P = Block + 4 * Channels + (W * Channels + C) * 4;
			for (int32 B = 0; B < 4; ++B)
			{
				S47AdpcmTables::Nibble(P[B] & 15, Pred, Index); Out[At++ * Channels + C] = int16(Pred);
				S47AdpcmTables::Nibble(P[B] >> 4, Pred, Index); Out[At++ * Channels + C] = int16(Pred);
			}
		}
	}
	return Frames;
}

void S47Adpcm::Test::Encode(const int16* Pcm, int32 Frames, int32 Channels, int32 BlockAlign, TArray<uint8>& Data)
{
	const int32 Per = FramesIn(BlockAlign, Channels);
	Data.Reset();
	if (Per <= 0 || Frames <= 0) return;
	const int32 Blocks = (Frames + Per - 1) / Per;
	Data.SetNumZeroed(Blocks * BlockAlign);
	const auto Sample = [Pcm, Frames, Channels](int32 Frame, int32 C) { return Frame < Frames ? int32(Pcm[Frame * Channels + C]) : 0; };
	for (int32 C = 0; C < Channels; ++C)
	{
		int32 Index = 0;
		for (int32 Bk = 0; Bk < Blocks; ++Bk)
		{
			uint8* Blk = Data.GetData() + Bk * BlockAlign;
			const int32 First = Bk * Per;
			int32 Pred = Sample(First, C);
			const uint16 H = uint16(int16(Pred));
			Blk[4 * C] = uint8(H & 255); Blk[4 * C + 1] = uint8(H >> 8); Blk[4 * C + 2] = uint8(Index); Blk[4 * C + 3] = 0;
			for (int32 K = 1; K < Per; ++K)
			{
				const int32 Want = Sample(First + K, C);
				int32 Best = 0, BestErr = MAX_int32;
				for (int32 Code = 0; Code < 16; ++Code)
				{
					int32 P = Pred, I = Index;
					S47AdpcmTables::Nibble(Code, P, I);
					if (FMath::Abs(P - Want) < BestErr) { BestErr = FMath::Abs(P - Want); Best = Code; }
				}
				S47AdpcmTables::Nibble(Best, Pred, Index);
				const int32 W = (K - 1) / 8, N = (K - 1) % 8;
				uint8& Byte = Blk[4 * Channels + (W * Channels + C) * 4 + N / 2];
				Byte = uint8(N & 1 ? (Byte | (Best << 4)) : (Byte | Best));
			}
		}
	}
}

TArray<uint8> S47Adpcm::Test::Wave(const TArray<uint8>& Data, int32 Channels, int32 Rate, int32 BlockAlign, int32 Frames)
{
	TArray<uint8> W;
	const auto Put = [&W](const void* P, int32 N) { W.Append(static_cast<const uint8*>(P), N); };
	const auto Put16 = [&W](uint32 V) { W.Add(uint8(V)); W.Add(uint8(V >> 8)); };
	const auto Put32 = [&Put16](uint32 V) { Put16(V & 0xffff); Put16(V >> 16); };
	const int32 Per = FramesIn(BlockAlign, Channels);
	Put("RIFF", 4); Put32(uint32(4 + 28 + 12 + 8 + Data.Num())); Put("WAVE", 4);
	Put("fmt ", 4); Put32(20); Put16(FormatTag); Put16(uint32(Channels)); Put32(uint32(Rate)); Put32(Per > 0 ? uint32(int64(Rate) * BlockAlign / Per) : 0u);
	Put16(uint32(BlockAlign)); Put16(4); Put16(2); Put16(uint32(Per));
	Put("fact", 4); Put32(4); Put32(uint32(Frames));
	Put("data", 4); Put32(uint32(Data.Num())); W.Append(Data);
	return W;
}

bool S47Adpcm::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	const auto Check = [&Lines, &bOk](bool bPass, const FString& What) { Lines.Add(FString::Printf(TEXT("%s adpcm: %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), *What)); bOk &= bPass; };
	// test vectors from Tools/music/ima_adpcm.py's mirror (random nibbles; headers at both rails; clipping on the way)
	static const uint8 Block1[36] = { 0x34, 0xf2, 0x28, 0x00, 0x02, 0x42, 0x8a, 0x29, 0x08, 0x0c, 0x0c, 0xe7, 0x9a, 0xee, 0x69, 0xcb, 0x77, 0xdd, 0xb3, 0xe0, 0x77, 0x85, 0x54, 0x80, 0x1a, 0x00, 0xd8, 0xa5, 0x1e, 0x70, 0xe5, 0xc7, 0x13, 0x99, 0x4d, 0x1c };
	static const int16 Want1[65] = { -3532, -3322, -3284, -3110, -2826, -3017, -3051, -3145, -3002, -3028, -3005, -3199, -3173, -3386, -3358, -2967, -3696, -4193, -4464, -5533, -7427, -8201, -5150, -8061, -11463, -4601, 10108, -13016, -32768, -4097, -30166, -26781, -32768, 28670, 32767, 32767, 28672, 32767, 32767, 32767, 29043, 12115, 21348, 24146, 26689, 24377, 1253, 32767, 12288, -32768, -20481, -16757, 32767, 32767, -20479, 32767, -4095, 24576, 32767, 22610, 13377, -17402, 19460, -17402, -5115 };
	static const uint8 Block2[24] = { 0xff, 0x7f, 0x58, 0x00, 0x00, 0x80, 0x00, 0x00, 0x6d, 0xba, 0x0d, 0xb7, 0x4a, 0xba, 0x5f, 0xde, 0x5f, 0x0b, 0x4a, 0x33, 0x40, 0x71, 0x2e, 0xd1 };
	static const int16 Want2[34] = { 32767, -32768, -12287, -32768, 32767, -32761, 12288, -32766, -13781, -32768, -32768, -32768, -28673, -32746, 27190, -32768, -1481, -32768, -32768, -32761, 12286, -32700, -16385, -32676, -12661, -32564, -29589, -32768, -1889, -32625, 24180, -32547, 32767, -32768 };
	{
		int16 Got1[65], Got2[34];
		const bool bOne = DecodeBlock(Block1, 36, 1, Got1) == 65 && !FMemory::Memcmp(Got1, Want1, sizeof(Want1));
		const bool bTwo = DecodeBlock(Block2, 24, 2, Got2) == 17 && !FMemory::Memcmp(Got2, Want2, sizeof(Want2));
		Check(bOne && bTwo, TEXT("the decoder equals the Python mirror bit for bit (one channel 65 frames, two channels 17 frames, both rails)"));
		uint8 Bad[36]; FMemory::Memcpy(Bad, Block1, 36); Bad[2] = 89;
		Check(DecodeBlock(Bad, 36, 1, Got1) == -1 && DecodeBlock(Block1, 35, 1, Got1) == -1 && DecodeBlock(Block2, 20, 2, Got2) == -1 && DecodeBlock(Block1, 36, 3, Got1) == -1,
			TEXT("a step index over 88, a size that is not whole words and three channels are refused"));
		Check(FramesIn(512, 1) == 1017 && FramesIn(1024, 2) == 1017 && FramesIn(8, 2) == 1 && FramesIn(10, 1) == 0 && FramesIn(0, 1) == 0,
			TEXT("1017 frames in 512 bytes a channel, one- or two-channel alike"));
	}
	{
		// a round trip through the test encoder: a chord with a swell and a little noise, two channels, a partial last block
		constexpr int32 Rate = 22050, Frames = Rate * 3 + 123;
		TArray<int16> Pcm; Pcm.SetNumUninitialized(Frames * 2);
		FRandomStream R(47);
		for (int32 I = 0; I < Frames; ++I)
		{
			const double T = double(I) / Rate, V = (.3 * FMath::Sin(2. * PI * 220. * T) + .2 * FMath::Sin(2. * PI * 331. * T + 1.) + .05 * (R.FRand() * 2. - 1.)) * FMath::Min(1., T);
			Pcm[I * 2] = int16(FMath::RoundToInt(V * 32767.)); Pcm[I * 2 + 1] = int16(FMath::RoundToInt(V * .7 * 32767.));
		}
		TArray<uint8> Data; Test::Encode(Pcm.GetData(), Frames, 2, 1024, Data);
		const int32 Per = FramesIn(1024, 2), Blocks = Data.Num() / 1024;
		TArray<int16> Back; Back.SetNumZeroed(Blocks * Per * 2);
		bool bDecoded = Blocks * Per >= Frames;
		for (int32 Bk = 0; Bk < Blocks; ++Bk) bDecoded &= DecodeBlock(Data.GetData() + Bk * 1024, 1024, 2, Back.GetData() + Bk * Per * 2) == Per;
		double S = 0, E = 0;
		for (int32 I = 0; I < Frames * 2; ++I) { S += double(Pcm[I]) * Pcm[I]; E += FMath::Square(double(Back[I]) - Pcm[I]); }
		const double Snr = 10. * FMath::LogX(10., S / FMath::Max(E, 1.));
		Check(bDecoded && Snr > 20., FString::Printf(TEXT("round trip of a two-channel chord with noise: SNR %.1f dB (over 20), %d blocks"), Snr, Blocks));
		// silence stays silent: zeros from a zero step index decode to zeros
		TArray<int16> Zero; Zero.SetNumZeroed(3000); TArray<uint8> ZData; Test::Encode(Zero.GetData(), 3000, 1, 512, ZData);
		TArray<int16> ZBack; ZBack.SetNumUninitialized(FramesIn(512, 1) * 3); bool bSilent = true;
		for (int32 Bk = 0; Bk < 3; ++Bk) bSilent &= DecodeBlock(ZData.GetData() + Bk * 512, 512, 1, ZBack.GetData() + Bk * FramesIn(512, 1)) > 0;
		for (int16 V : ZBack) bSilent &= V == 0;
		Check(bSilent, TEXT("silence decodes to exact silence"));
	}
	return bOk;
}

#include "S47TvProgramme.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

namespace
{
	struct FProgramme
	{
		TArray<uint8> Bytes;
		TArray<uint32> Offsets;
		uint32 Fps = 0;
		uint32 Read(int32 At) const
		{
			return uint32(Bytes[At]) | (uint32(Bytes[At+1]) << 8) | (uint32(Bytes[At+2]) << 16) | (uint32(Bytes[At+3]) << 24);
		}
		FProgramme()
		{
			const FString Path = FPaths::ProjectContentDir() / TEXT("WebData/tv/unarius.s47tv");
			if (!FFileHelper::LoadFileToArray(Bytes, *Path)) return;
			if (Bytes.Num() < 28 || Bytes.Num() > 32 * 1024 * 1024 || FMemory::Memcmp(Bytes.GetData(), "S47TV001", 8)) { Bytes.Reset(); return; }
			const uint32 N = Read(20);
			if (Read(8) != 8 || Read(12) != 320 || Read(16) != 240 || N == 0 || N > 24000 || 24 + 4 * (N + 1) > uint32(Bytes.Num())) { Bytes.Reset(); return; }
			for (uint32 I = 0; I <= N; ++I)
			{
				const uint32 Offset = Read(24 + 4 * I);
				if (Offset > uint32(Bytes.Num()) || (I == 0 && Offset != 24 + 4 * (N + 1)) || (I > 0 && (Offset <= Offsets.Last() || Offset - Offsets.Last() > 1024 * 1024)))
				{ Bytes.Reset(); Offsets.Reset(); return; }
				Offsets.Add(Offset);
			}
			if (Offsets.Last() != uint32(Bytes.Num())) { Bytes.Reset(); Offsets.Reset(); return; }
			Fps = 8;
			UE_LOG(LogTemp, Display, TEXT("S47: Unarius TV programme ready: %u frames, %d bytes"), N, Bytes.Num());
		}
	};
	const FProgramme& Programme() { static const FProgramme P; return P; }
}

bool FS47TvProgramme::Available() const { return Programme().Fps != 0; }

bool FS47TvProgramme::Frame(double Seconds, TArray<FColor>& Out, int32 Width, int32 Height)
{
	const FProgramme& P = Programme();
	if (!P.Fps || !FMath::IsFinite(Seconds) || Width <= 0 || Height <= 0) return false;
	const int32 Index = int32(FMath::Fmod(FMath::Max(0.0, Seconds) * P.Fps, double(P.Offsets.Num() - 1)));
	if (Index != LastFrame)
	{
		auto& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		const auto Decoder = Module.CreateImageWrapper(EImageFormat::JPEG);
		TArray<uint8> Raw;
		if (!Decoder || !Decoder->SetCompressed(P.Bytes.GetData() + P.Offsets[Index], P.Offsets[Index+1] - P.Offsets[Index])
			|| Decoder->GetWidth() != 320 || Decoder->GetHeight() != 240 || !Decoder->GetRaw(ERGBFormat::BGRA, 8, Raw)) return false;
		Pixels.SetNumUninitialized(320 * 240);
		FMemory::Memcpy(Pixels.GetData(), Raw.GetData(), Pixels.Num() * sizeof(FColor));
		if (LastFrame < 0) UE_LOG(LogTemp, Display, TEXT("S47: Unarius TV decoded frame %d"), Index);
		LastFrame = Index;
	}
	Out.SetNumUninitialized(Width * Height);
	for (int32 Y = 0; Y < Height; ++Y) for (int32 X = 0; X < Width; ++X)
		Out[Y * Width + X] = Pixels[FMath::Min(239, Y * 240 / Height) * 320 + FMath::Min(319, X * 320 / Width)];
	return true;
}

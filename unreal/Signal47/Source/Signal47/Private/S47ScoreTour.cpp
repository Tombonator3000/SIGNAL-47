// A short listening sequence through the actual adaptive score and imported tracks.
#include "S47Score.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"

bool FS47Score::RenderTour(const FString& Path, TArray<FString>& Lines)
{
	auto C = FS47AudioContext::Make(48000);
	FS47Score Score(nullptr, C, C->Destination);
	Score.Start(TEXT("calm"), TEXT("solitude"));
	constexpr int32 Seconds = 45, Rate = 48000, Step = 800;
	TArray<float> Samples; Samples.SetNumZeroed(Seconds * Rate * 2);
	bool bOverlap = false, bAction = false, bRecovery = false, bFinite = true;
	FString Last;
	for (int32 F = 0; F < Seconds * Rate; F += Step)
	{
		const double T = double(F) / Rate;
		const FString Want = T < 8 ? TEXT("calm") : T < 15 ? TEXT("tension") : T < 22 ? TEXT("action") : TEXT("calm");
		Score.RequestMood(Want, Want == TEXT("action") ? 1 : 0);
		Score.Update(1.0 / 60, Want, T >= 41);
		bOverlap |= Score.ActiveDecks() == 2;
		bAction |= Score.Playing == TEXT("action"); bRecovery |= Score.Playing == TEXT("recovery");
		if (Score.Playing != Last)
		{
			Last = Score.Playing;
			Lines.Add(FString::Printf(TEXT("music preview %.2fs: %s / %s"), T, *Score.Playing, *Score.Track));
		}
		C->Render(Samples.GetData() + F * 2, Step, 2); C->Tick(1.0 / 60);
		// The offline clock runs much faster than the speaker. Let the ordinary bounded
		// media reader finish queued disk chunks without changing production streaming.
		if (F % (Step * 4) == 0) FPlatformProcess::Sleep(0.001f);
	}
	double Energy = 0, Peak = 0, Tail = 0;
	for (int32 I = 0; I < Samples.Num(); ++I)
	{
		const float X = Samples[I]; bFinite &= FMath::IsFinite(X);
		Energy += X * X; Peak = FMath::Max(Peak, double(FMath::Abs(X)));
		if (I >= (Seconds - 1) * Rate * 2) Tail += X * X;
	}
	Score.Stop(); C->Close();
	const bool bOk = bFinite && Energy > 0.001 && Peak < 1 && Tail < 0.000001 && bOverlap && bAction && bRecovery;
	Lines.Add(FString::Printf(TEXT("%s adaptive music preview: overlap=%d action=%d recovery=%d peak=%.5f tail=%.9f"), bOk ? TEXT("PASS") : TEXT("FAIL"), bOverlap, bAction, bRecovery, Peak, Tail));
	if (!bOk) return false;
	TArray<uint8> Bytes;
	auto U32 = [&Bytes](uint32 V) { for (int I=0;I<4;++I) Bytes.Add(uint8(V>>(8*I))); };
	auto U16 = [&Bytes](uint16 V) { Bytes.Add(uint8(V)); Bytes.Add(uint8(V>>8)); };
	auto Tag = [&Bytes](const char* S) { for (int I=0;I<4;++I) Bytes.Add(uint8(S[I])); };
	Tag("RIFF"); U32(36+Samples.Num()*2); Tag("WAVE");
	Tag("fmt "); U32(16); U16(1); U16(2); U32(Rate); U32(Rate*4); U16(4); U16(16);
	Tag("data"); U32(Samples.Num()*2);
	for (float X : Samples) U16(uint16(int16(FMath::Clamp(X,-1.f,1.f)*32767.f)));
	return FFileHelper::SaveArrayToFile(Bytes,*Path);
}

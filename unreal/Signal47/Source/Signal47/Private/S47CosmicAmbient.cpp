#include "S47CosmicAmbient.h"
#include "S47Score.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

FS47CosmicAmbient::FS47CosmicAmbient(const TSharedRef<FS47AudioContext>& InCtx)
    : FS47AudioNode(InCtx), Synth(InCtx->SampleRate) {}

void FS47CosmicAmbient::AddReference(const TSharedPtr<FS47AudioBuffer>& Buffer)
{
    FS47AudioContext::FBatch Batch(*Ctx);
    if (Buffer && Buffer->Length() > 0 && References.Num() < 3 && !References.Contains(Buffer)) References.Add(Buffer);
}

void FS47CosmicAmbient::Set(const FString& Cue, bool bHeld)
{
    FS47AudioContext::FBatch Batch(*Ctx);
    S47Cosmic::State S;
    if (Cue == TEXT("night") || Cue == TEXT("calm")) S = {.55, .55, .2};
    else if (Cue == TEXT("eerie")) S = {.65, .3, .65};
    else if (Cue == TEXT("tension")) S = {.5, .15, .9};
    else if (Cue == TEXT("dawn")) S = {.45, .9, .1};
    else if (Cue == TEXT("drive")) S = {.35, .6, .2};
    else if (Cue == TEXT("recovery")) S = {.35, .7, .25};
    // Town, motel and action keep their established identity. Unknown cues stay silent.
    bListening = bHeld || S.Level == 0;
    if (bListening) S.Level = 0;
    if (bListening)
    {
        ActiveReference = INDEX_NONE; Cursor = 0;
        NextAt = Synth.Seconds() + 37; // No new reference immediately after a listening hold.
    }
    Synth.Set(S);
}

void FS47CosmicAmbient::Pause(bool On)
{
    FS47AudioContext::FBatch Batch(*Ctx);
    Synth.Pause(On);
}

void FS47CosmicAmbient::Process(int64 Quantum)
{
    Out.Zero(2);
    for (int32 I = 0; I < S47Quantum; ++I)
    {
        if (!Synth.Paused() && !bListening && References.Num() && Synth.Seconds() >= NextAt && ActiveReference == INDEX_NONE)
        {
            ActiveReference = NextReference++ % References.Num(); Cursor = 0;
            NextAt = Synth.Seconds() + 61 + (NextReference * 17 % 29);
        }
        double L = 0, R = 0;
        if (!Synth.Paused() && ActiveReference != INDEX_NONE)
        {
            const auto& B = References[ActiveReference];
            const int32 At = int32(Cursor);
            if (At < B->Length())
            {
                const int32 Next = FMath::Min(At + 1, B->Length() - 1);
                const double Fraction = Cursor - At;
                // Original playback rate; soft edge fade only, no loops or evidence gating.
                const double Edge = FMath::Min(1., FMath::Min(Cursor / B->SampleRate(), (B->Length() - Cursor) / B->SampleRate()));
                auto Sample = [&](int32 Channel)
                {
                    const float* Data = B->GetChannelData(Channel);
                    return Edge * FMath::Lerp(double(Data[At]), double(Data[Next]), Fraction);
                };
                L = Sample(0); R = Sample(B->NumberOfChannels() > 1 ? 1 : 0);
                Cursor += B->SampleRate() / double(Ctx->SampleRate);
            }
            else { ActiveReference = INDEX_NONE; Cursor = 0; }
        }
        Synth.Next(Out.D[0][I], Out.D[1][I], L, R);
    }
}

bool FS47CosmicAmbient::SelfTest(TArray<FString>& Lines)
{
    auto C = FS47AudioContext::Make(48000);
    auto Node = MakeShared<FS47CosmicAmbient>(C); Node->Connect(C->Destination);
    Node->Set(TEXT("night"), false);
    float Samples[S47Quantum * 2]; double Energy = 0, Peak = 0;
    for (int32 I = 0; I < 750; ++I)
    {
        C->Render(Samples, S47Quantum, 2);
        for (float X : Samples) { Energy += X * X; Peak = FMath::Max(Peak, double(FMath::Abs(X))); }
    }
    bool Ok = FMath::IsFinite(Energy) && Energy > .00001 && Peak < .18;
    // A one-second 8 kHz reference fixture checks rate conversion and exact pause.
    auto Ref = C->CreateBuffer(1, 8000, 8000);
    for (int32 I = 0; I < Ref->Length(); ++I) Ref->GetChannelData(0)[I] = .25f;
    Node->AddReference(Ref); Node->NextAt = 0;
    C->Render(Samples, S47Quantum, 2);
    const double Cursor = Node->Cursor, Time = Node->Synth.Seconds();
    Ok &= Node->ActiveReference == 0 && FMath::Abs(Cursor - S47Quantum / 6.) < 1e-6;
    Node->Pause(true);
    C->Render(Samples, S47Quantum, 2);
    for (float X : Samples) Ok &= X == 0;
    Ok &= Node->Cursor == Cursor && Node->Synth.Seconds() == Time;
    Node->Pause(false);
    C->Render(Samples, S47Quantum, 2); Ok &= Node->Cursor > Cursor;
    Node->Set(TEXT("night"), true);
    for (int32 I = 0; I < 1500; ++I) C->Render(Samples, S47Quantum, 2);
    for (float X : Samples) Ok &= X == 0;
    Node->Disconnect(); C->Close();
    Lines.Add(FString::Printf(TEXT("%s cosmic ambient graph: stereo synthesis, bounded output, 8k reference rate, pause/resume, listening release"), Ok ? TEXT("PASS") : TEXT("FAIL")));
    return Ok;
}


bool FS47CosmicAmbient::RenderPreview(const FString& Path)
{
    TArray<FString> Lines;
    bool Ok = FS47Score::SelfTest(Lines);
    auto C = FS47AudioContext::Make(48000);
    auto Node = MakeShared<FS47CosmicAmbient>(C); Node->Connect(C->Destination);
    for (const TCHAR* Key : { TEXT("nasa_earth_chorus"), TEXT("nasa_cassini_jupiter"), TEXT("space_nebula") })
    {
        TArray<uint8> Bytes;
        if (FFileHelper::LoadFileToArray(Bytes, *(FPaths::ProjectContentDir() / TEXT("Audio/Raw/sound_design") / (FString(Key) + TEXT(".wav")))))
            Node->AddReference(S47Wa::DecodeWav(Bytes));
    }
    constexpr int32 Rate = 48000, Seconds = 72, Step = 800;
    TArray<float> Samples; Samples.SetNumZeroed(Rate * Seconds * 2);
    for (int32 F = 0; F < Rate * Seconds; F += Step)
    {
        const double T = double(F) / Rate;
        Node->Set(T < 20 ? TEXT("night") : T < 40 ? TEXT("eerie") : TEXT("dawn"), T >= 60);
        C->Render(Samples.GetData() + F * 2, Step, 2); C->Tick(1. / 60);
    }
    double Energy = 0, Peak = 0, Tail = 0;
    for (int32 I = 0; I < Samples.Num(); ++I)
    {
        const double X = Samples[I]; Ok &= FMath::IsFinite(X);
        Energy += X * X; Peak = FMath::Max(Peak, FMath::Abs(X));
        if (I >= 61 * Rate * 2) Tail += X * X;
    }
    Ok &= Energy > .001 && Peak < .18 && Tail == 0;
    TArray<uint8> Bytes;
    auto U16 = [&Bytes](uint16 V) { Bytes.Add(uint8(V)); Bytes.Add(uint8(V >> 8)); };
    auto U32 = [&Bytes](uint32 V) { for (int I = 0; I < 4; ++I) Bytes.Add(uint8(V >> (8 * I))); };
    auto Tag = [&Bytes](const char* S) { for (int I = 0; I < 4; ++I) Bytes.Add(uint8(S[I])); };
    Tag("RIFF"); U32(36 + Samples.Num() * 2); Tag("WAVE"); Tag("fmt "); U32(16);
    U16(1); U16(2); U32(Rate); U32(Rate * 4); U16(4); U16(16); Tag("data"); U32(Samples.Num() * 2);
    for (float X : Samples) U16(uint16(int16(FMath::Clamp(X, -1.f, 1.f) * 32767.f)));
    Ok &= FFileHelper::SaveArrayToFile(Bytes, *Path);
    for (const FString& Line : Lines) UE_LOG(LogTemp, Display, TEXT("S47: %s"), *Line);
    UE_LOG(LogTemp, Display, TEXT("S47: cosmic preview %s refs=%d peak=%.6f rms=%.6f tail=%.9f %s"),
        Ok ? TEXT("PASS") : TEXT("FAIL"), Node->References.Num(), Peak, FMath::Sqrt(Energy / Samples.Num()), Tail, *Path);
    Node->Disconnect(); C->Close();
    return Ok;
}

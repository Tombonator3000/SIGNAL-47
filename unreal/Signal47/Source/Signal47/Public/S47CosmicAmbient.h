#pragma once
#include "S47WebAudio.h"
#include "S47CosmicDSP.h"

// Non-diegetic music texture. Reference recordings never become receiver evidence.
class SIGNAL47_API FS47CosmicAmbient : public FS47AudioNode
{
public:
    explicit FS47CosmicAmbient(const TSharedRef<FS47AudioContext>& InCtx);
    void Set(const FString& Cue, bool bHeld);
    void Pause(bool On);
    void AddReference(const TSharedPtr<FS47AudioBuffer>& Buffer);
    static bool SelfTest(TArray<FString>& Lines);
    static bool RenderPreview(const FString& Path);
protected:
    virtual void Process(int64 Quantum) override;
private:
    S47Cosmic::Synth Synth;
    TArray<TSharedPtr<FS47AudioBuffer>> References;
    int32 NextReference = 0, ActiveReference = INDEX_NONE;
    double Cursor = 0, NextAt = 37;
    bool bListening = true;
};

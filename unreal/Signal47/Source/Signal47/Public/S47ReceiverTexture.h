#pragma once
#include "CoreMinimal.h"
class FS47AudioContext;
class FS47AudioBuffer;

// Original radio-monitor synthesis. Presentation only: no pulse groups, source
// classification, discoveries or evidence. Live and replay use the same source ID.
namespace S47ReceiverTexture
{
 enum class ELayer : uint8 { Source, Static };
 constexpr double LoopSeconds=12.;
 // Stable across processes/platforms; do not use transient job numbers as identity.
 uint32 Seed(const FString& SourceId);
 // Quality/Bandwidth/Gain are normalised 0..1. Bandwidth=1 is the open audio band.
 float SourceLevel(float Quality,float Gain);
 float StaticLevel(float Quality,float Gain);
 // Stateless source-time evaluation: identical samples regardless of chunk order.
 float Sample(uint32 Identity,double SourceTime,float Quality,float Gain,float Bandwidth=1.f);
 // Loop both layers from the SAME source-time offset and ramp their gain nodes with
 // SourceLevel/StaticLevel. Cache per identity/bandwidth; never regenerate per frame.
 // Mono, context sample rate, exactly LoopSeconds; boundary is intrinsically periodic.
 TSharedPtr<FS47AudioBuffer> MakeLoop(const TSharedRef<FS47AudioContext>& Context,uint32 Identity,ELayer Layer,float Bandwidth=1.f);
 bool SelfTest(TArray<FString>& Lines);
}

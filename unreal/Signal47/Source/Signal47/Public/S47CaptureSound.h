#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class FS47AudioContext;
class FS47AudioNode;
class FS47AudioBuffer;
class FS47BufferSourceNode;

// New full-rate pulsar captures only. Existing v1/v2 originals are untouched.
namespace S47CaptureSound
{
 struct FFrame { double Start=0,End=0,Frequency=0,Gain=0,Bandwidth=0,Quality=0; };
 struct FTrack
 {
  FString Id,Failure;
  double Duration=0;
  bool Complete=false,Control=false;
  TArray<FFrame> Frames;
  TArray<double> Ticks;
  bool Enabled() const { return !Id.IsEmpty(); }
  bool Eligible() const { return Enabled()&&Failure.IsEmpty(); }
  bool Playable() const { return Eligible()&&Complete; }
  void Begin(const FString& InId,bool InControl,const TArray<double>& ActualTicks);
  void Interrupt(const FString& Reason);
  bool Append(double From,double To,double ActiveDt,double Rate,double Frequency,double Gain,double Bandwidth,double Quality);
  float Sample(double At) const;
  S47J::FObj Save() const;
  void Load(const S47J::FObj& O);
 };
 TSharedPtr<FS47AudioBuffer> Buffer(const FTrack& T,const TSharedRef<FS47AudioContext>& Ctx,int64 First,int64 Last);
 bool SelfTest(TArray<FString>& Lines);
}

// Separate capture/playback ownership, no global source-ID buffer cache.
class FS47CapturePlayer
{
public:
 FS47CapturePlayer(const TSharedRef<FS47AudioContext>& Context,const TSharedPtr<FS47AudioNode>& Destination);
 ~FS47CapturePlayer();
 bool Queue(const S47CaptureSound::FTrack& T); // newest observed frame, integer sample schedule
 bool Prepare(const S47CaptureSound::FTrack& T); // no source or transport start
 bool Ready(const S47CaptureSound::FTrack& T) const;
 bool Play(const S47CaptureSound::FTrack& T,double Cursor); // requires prepared PCM
 void Audible(bool Enabled);
 void Stop();
 bool Pending() const;
 bool Aligned(double Cursor) const;
 int64 ScheduledStart() const { return BaseFrame; }
 int64 QueuedSamples() const { return NextFrame; }
 int64 RenderedSamples() const;
private:
 TSharedRef<FS47AudioContext> Ctx;
 TSharedPtr<FS47AudioNode> Destination;
 struct FBlock { TSharedPtr<FS47BufferSourceNode> Source; int64 End=0,Count=0; };
 TArray<FBlock> Sources;
 FString Id,CachedId;
 TSharedPtr<FS47AudioBuffer> CachedBuffer; // at most one completed replay, independent of global source cache
 int64 BaseFrame=0,NextFrame=0,EndFrame=0,DoneSamples=0,ReplayOffset=0;
 bool Replaying=false;
 class FGate;
 TSharedPtr<FGate> Gate;
};

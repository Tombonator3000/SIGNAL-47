#include "S47Audio.h"
#include "S47Rx.h"
#include "S47ReceiverTexture.h"

void US47Audio::ReceiverTextureStop(FReceiverTextureGraph& G)
{
 if(!Ctx||!G.Source)return;
 FS47AudioContext::FBatch Batch(*Ctx);
 const double T=Ctx->CurrentTime();
 for(auto Gain:{G.SignalGain,G.NoiseGain}){Gain->Gain.CancelAndHoldAtTime(T);Gain->Gain.LinearRampToValueAtTime(0,T+.06);}
 G.Source->Stop(T+.07);G.Noise->Stop(T+.07);G=FReceiverTextureGraph();
}
void US47Audio::ReceiverTextureFrame(FReceiverTextureGraph& G,const FString& Id,double Gain,double Bandwidth,double Quality,double Offset,int32 TextureVersion,double SignalMul,double NoiseMul)
{
 if((TextureVersion!=1&&TextureVersion!=2)||!Ctx||!FMath::IsFinite(Gain)||!FMath::IsFinite(Bandwidth)||!FMath::IsFinite(Quality)||!FMath::IsFinite(Offset)){ReceiverTextureStop(G);return;}
 const double T=Ctx->CurrentTime();
 if(G.Source&&(G.Id!=Id||G.Version!=TextureVersion||(Offset>=0&&FMath::Abs(Offset-(G.Offset+T-G.Started))>.25)))ReceiverTextureStop(G);
 if(!G.Source)
 {
  // Fixed small catalogue, cached once. No per-frame PCM generation or global RNG.
  const FString StableId=Id.Left(80);
  const FString CacheId=StableId+FString::Printf(TEXT("|v%d|"),TextureVersion);
  for(int32 I=0;I<2;++I)
  {
   const FString Key=CacheId+FString::FromInt(I);
   if(!ReceiverBuffers.Contains(Key))ReceiverBuffers.Add(Key,S47ReceiverTexture::MakeLoop(Ctx.ToSharedRef(),S47ReceiverTexture::Seed(StableId),I==0?S47ReceiverTexture::ELayer::Source:S47ReceiverTexture::ELayer::Static,1.f,TextureVersion));
  }
  FS47AudioContext::FBatch Batch(*Ctx);
  G.Id=Id;G.Version=TextureVersion;G.Started=T;G.Offset=FMath::Max(0.,Offset);
  G.Source=Ctx->CreateBufferSource();G.Noise=Ctx->CreateBufferSource();
  G.Source->Buffer=ReceiverBuffers[CacheId+TEXT("0")];G.Noise->Buffer=ReceiverBuffers[CacheId+TEXT("1")];
  G.Source->bLoop=G.Noise->bLoop=true;
  G.SignalGain=Ctx->CreateGain();G.NoiseGain=Ctx->CreateGain();G.SignalGain->Gain.SetValue(0);G.NoiseGain->Gain.SetValue(0);
  G.Filter=Ctx->CreateBiquadFilter();G.Filter->SetType(TEXT("lowpass"));G.Filter->Q.SetValue(.65);
  G.Source->Connect(G.SignalGain);G.Noise->Connect(G.NoiseGain);G.SignalGain->Connect(G.Filter);G.NoiseGain->Connect(G.Filter);G.Filter->Connect(SfxBus);
  const double From=FMath::Fmod(G.Offset,S47ReceiverTexture::LoopSeconds);
  G.Source->Start(T,From);G.Noise->Start(T,From);
 }
 FS47AudioContext::FBatch Batch(*Ctx);
 const float Q=FMath::Clamp(Quality,0.,1.),Level=.42*FMath::Clamp(Gain/100.,0.,1.);
 G.SignalGain->Gain.SetTargetAtTime(S47ReceiverTexture::SourceLevel(Q,Level)*SignalMul,T,.10);
 G.NoiseGain->Gain.SetTargetAtTime(S47ReceiverTexture::StaticLevel(Q,Level)*NoiseMul,T,.04);
 G.Filter->Frequency.SetTargetAtTime(650.+3300.*FMath::Sqrt(FMath::Clamp(Bandwidth/100.,0.,1.)),T,.08);
}

// Freeform monitor: source texture emerges through receiver noise as actual
// tuning AND physical reception improve. Story's carrier/signature stay separate.
void US47Audio::MonitorReceiver(const FS47Rx& Rx,const FS47Profile& Profile,const FString& SourceId,double Reception,int32 TextureVersion)
{
 const double Quality=Rx.Quality(Profile)*FMath::Clamp(Reception,0.,1.);
 if((TextureVersion!=1&&TextureVersion!=2)||!Ctx||!Rx.bPowered||!FMath::IsFinite(Reception)||!FMath::IsFinite(Quality)||!FMath::IsFinite(Rx.Frequency)||!FMath::IsFinite(Rx.Gain)||!FMath::IsFinite(Rx.Bandwidth)){ReceiverMonitorOff();return;}
 bReceiverMonitor=true;
 // Atmosphere of a live search, on the monitor only, never in a recording or its measured pulses:
 // the static scrapes, the wanted signal swells harder as the search closes on 100 %, and now
 // and then another station's telegraph key is faintly in the band.
 const double T=Ctx->CurrentTime();
 auto Hash=[](double N){const double V=FMath::Sin(N*12.9898)*43758.5453;return V-FMath::FloorToDouble(V);};
 const double Scrape=.5+.9*Hash(FMath::FloorToDouble(T*11.))*(.5+.5*FMath::Sin(T*.73));
 const double Near=FMath::SmoothStep(.55,1.,Quality);
 const double Swell=1.+1.1*Near*FMath::Pow(.5+.5*FMath::Sin(T*2.*PI/.84),3.);
 ReceiverTextureFrame(ReceiverTexture,SourceId,Rx.Gain,Rx.Bandwidth,Quality,-1,TextureVersion,Swell,Scrape);
 const int64 Slot=int64(FMath::FloorToDouble(T/6.));
 if(Slot!=ReceiverStationSlot)
 {
  ReceiverStationSlot=Slot;
  if(Hash(double(Slot))<.5&&Quality<.97)
  {
   // Well away from the 820/1240 Hz measured pulses.
   const float Freq=float(470.+290.*Hash(Slot*1.7));const float Level=float(.016*(1.-.7*Near));float At=0;
   const int32 Marks=3+int32(5.*Hash(Slot*2.3));
   for(int32 I=0;I<Marks;++I){const float Len=Hash(Slot+I*3.1)<.4?.27f:.09f;Beep(Freq,Len,Level,At,TEXT("sine"));At+=Len+.09f;}
  }
 }
}
void US47Audio::ReceiverMonitorOff()
{
 if(!bReceiverMonitor)return;
 bReceiverMonitor=false;ReceiverTextureStop(ReceiverTexture);
}
void US47Audio::MonitorReceiverRecording(const FString& SourceId,double Gain,double Bandwidth,double Quality,double Cursor,int32 TextureVersion)
{
 if(Cursor<0){ReceiverRecordingOff();return;}
 ReceiverTextureFrame(ReceiverTape,SourceId,Gain,Bandwidth,Quality,Cursor,TextureVersion);
}
void US47Audio::ReceiverRecordingOff(){ReceiverTextureStop(ReceiverTape);}

bool US47Audio::ReceiverMonitorSelfTest(TArray<FString>& Lines)
{
 bool Good=S47ReceiverTexture::SelfTest(Lines);
 auto Check=[&](bool Ok,const TCHAR* Name){Good&=Ok;Lines.Add(FString::Printf(TEXT("%s receiver monitor: %s"),Ok?TEXT("PASS"):TEXT("FAIL"),Name));};
 auto* A=NewObject<US47Audio>();A->Unlock(false,false);
 auto Render=[&]()
 {
  const int32 N=48000;TArray<float> PCM;PCM.SetNumZeroed(N*2);A->Ctx->Render(PCM.GetData(),N,2);
  double Energy=0;bool Finite=true;for(int32 I=N;I<PCM.Num();++I){Finite&=FMath::IsFinite(PCM[I])&&FMath::Abs(PCM[I])<1.;Energy+=double(PCM[I])*PCM[I];}
  Check(Finite,TEXT("rendered PCM is finite and below clipping"));return FMath::Sqrt(Energy/N);
 };
 const FS47Profile& P=S47Signal::Profiles()[0];FS47Rx Rx;Rx.bPowered=true;
 Rx.Frequency=P.F;Rx.Gain=P.Gain[0];Rx.Bandwidth=P.Bw[0];Rx.Azimuth=(P.Az[0]+P.Az[1])*.5;
 A->MonitorReceiver(Rx,P);const double Locked=Render();
 Check(A->ReceiverMonitorOn()&&Locked>.005,TEXT("locked receiver renders audible source texture"));
 Rx.Gain=P.Gain[1];A->MonitorReceiver(Rx,P);const double Higher=Render();
 Check(Higher>Locked*1.1,TEXT("gain changes audible level inside the accepted window"));
 const double Narrow=A->ReceiverTexture.Filter->Frequency.Value();Rx.Bandwidth=P.Bw[1];A->MonitorReceiver(Rx,P);Render();
 Check(A->ReceiverTexture.Filter->Frequency.Value()>Narrow+100,TEXT("bandwidth changes the rendered noise filter within the accepted window"));
 Rx.Frequency=P.F+.4;A->MonitorReceiver(Rx,P);const double Searching=Render();
 Check(Searching>.0001&&FMath::Abs(Searching-Locked)>.0001,TEXT("off-frequency search renders a different noise-dominated signal"));
 Check(A->ReceiverTexture.SignalGain->Gain.Value()<1e-4,TEXT("off-frequency search hides source structure"));
 Rx.Frequency=P.F;A->MonitorReceiver(Rx,P,TEXT("receiver"),0);Render();
 Check(A->ReceiverTexture.SignalGain->Gain.Value()<1e-4,TEXT("unpointed array cannot expose a perfectly tuned source"));
 Rx.bPowered=false;A->MonitorReceiver(Rx,P);const double Silent=Render();
 Check(!A->ReceiverMonitorOn()&&Silent<1e-5,TEXT("power loss releases the monitor and fades PCM to silence"));
 Rx.bPowered=true;Rx.Frequency=P.F;A->MonitorReceiver(Rx,P);Render();A->ReceiverMonitorOff();
 Check(Render()<1e-5&&!A->ReceiverMonitorOn(),TEXT("leaving the receiver fades it to silence"));
 A->MonitorReceiverRecording(TEXT("freeform-source-1"),60,50,1,0);const double Tape=Render();
 A->ReceiverMonitorOff();Check(Render()>.001&&Tape>.001,TEXT("live monitor release leaves separately owned tape playing"));
 const auto V1Buffer=A->ReceiverTape.Source->Buffer;
 A->MonitorReceiverRecording(TEXT("freeform-source-1"),60,50,1,0,2);Render();
 Check(A->ReceiverTape.Version==2&&A->ReceiverTape.Source->Buffer!=V1Buffer,TEXT("v2 playback uses a version-specific cached graph"));
 A->MonitorReceiverRecording(TEXT("freeform-source-1"),60,50,1,0,1);Render();
 Check(A->ReceiverTape.Version==1&&A->ReceiverTape.Source->Buffer==V1Buffer,TEXT("switching back restores the original v1 buffer without rewriting it"));
 A->ReceiverRecordingOff();Check(Render()<1e-5,TEXT("tape stop releases its graph"));
 A->StartCarrier();A->SetCarrier(.04,0,0);A->ReceiverMonitorOff();
 Check(Render()>.005,TEXT("inactive monitor does not mute the Story-owned carrier"));
 return Good;
}

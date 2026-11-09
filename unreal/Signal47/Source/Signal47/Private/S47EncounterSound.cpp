#include "S47EncounterSound.h"
#include "S47Audio.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
 constexpr int32 Rate=48000;
 // T16-12 levels. The transmission's peak after levelling and its largest playback gain: at full radio level its peak is
 // .42*.62=.26 (-12 dBFS) and its rms about .07, 7 dB over the hiss, where it had been -3 dBFS rms.
 // T16-12, second round (Tom: still screams too much and too loud when Roswell turns up): the set is mostly crackle now. The
 // transmission plays at a third of that (peak .42*.2=.084, -21.5 dBFS), only once the set has heard one for a while (not
 // at a glimpse), as a short piece of its seven seconds and seldom; the hiss is half; the crackle stays on top.
 constexpr float TransmissionPeak=.42f,TransmissionGainMax=.2f;
 constexpr double TransmissionLevel=.4;                          // the set must hear one this strongly,
 constexpr double TransmissionAfter=8.;                          // for this long, before anything comes through it
 constexpr double TransmissionShortest=1.2,TransmissionLongest=2.2; // seconds of the recording, from a random place in it
 constexpr double TransmissionGapMin=100.,TransmissionGapMax=200.;
 constexpr double RadioHiss=.12;                                 // the hiss at full level (was .24)
 constexpr double CrackleBase=.04,CrackleSpan=.28;               // a crackle's gain .04..0.32 by strength (was .06..0.48)
 double Smooth(double X) { X=FMath::Clamp(X,0.,1.); return X*X*(3.-2.*X); }
 int32 Index(FS47EncounterSound::EKind K) { return FMath::Clamp(int32(K),0,2); }
 double Duration(int32 K) { return K==0?.95:K==1?1.35:1.9; }
 TSharedPtr<FS47AudioBuffer> CueBuffer(int32 K)
 {
  const double Dur=Duration(K), Base=K==0?70.:K==1?184.:82., Drop=K==0?4.:K==1?68.:12.;
  auto B=MakeShared<FS47AudioBuffer>(1,FMath::RoundToInt(Rate*Dur),Rate);
  FRandomStream R(47001+K*193); double Phase=0, Breath=0;
  for (int32 I=0;I<B->Length();++I)
  {
   const double T=double(I)/Rate, X=T/Dur;
   Phase+=2.*PI*(Base-Drop*Smooth(X))/Rate;
   Breath+=(R.FRandRange(-1.f,1.f)-Breath)*.08;
   const double Envelope=Smooth(T/(K==0?.16:.045))*Smooth((Dur-T)/(K==0?.35:.48));
   const double Pulse=K==2?.45+.55*FMath::Square(FMath::Sin(3.*PI*X)):1.;
   // Inharmonic, decaying textures rather than a tune or a loud jump sting.
   const double Tone=.64*FMath::Sin(Phase)+.22*FMath::Sin(Phase*(K==0?1.031:1.493))
    +.08*FMath::Sin(Phase*5.09+FMath::Sin(T*3.));
   B->GetChannelData(0)[I]=float((Tone+Breath*.18)*Envelope*Pulse*(K==0?.027:K==1?.068:.074));
  }
  return B;
 }
 // Made here, with no recording: a pulse voice through two moving formants and a small speaker's narrow band, in
 // syllables at about the rate of speech, under hiss.
 TSharedPtr<FS47AudioBuffer> MessageBuffer()
 {
  static constexpr int32 MsgRate=22050;static constexpr double Dur=3.5,Open=.16,Close=.14;
  auto B=MakeShared<FS47AudioBuffer>(1,FMath::RoundToInt(MsgRate*Dur),MsgRate);FRandomStream R(47471);
  float* Out=B->GetChannelData(0);const int32 N=B->Length();
  // The syllables: when each begins, how long, and where the mouth is.
  struct FSyl{double At,Len,F1,F2;};TArray<FSyl> Syl;
  for(double T=Open+.06;T<Dur-Close-.2;){const double Len=R.FRandRange(.09f,.24f);Syl.Add({T,Len,R.FRandRange(320.f,780.f),R.FRandRange(1000.f,2300.f)});T+=Len+(R.FRand()<.2f?R.FRandRange(.14f,.3f):R.FRandRange(.02f,.06f));}
  double Phase=0,B1=0,B1v=0,B2=0,B2v=0,Lp=0,Hp=0,Crack=0;int32 S=0;
  for(int32 I=0;I<N;++I)
  {
   const double T=double(I)/MsgRate;
   while(S<Syl.Num()-1&&T>Syl[S].At+Syl[S].Len)++S;
   const FSyl& Y=Syl[S];const double U=(T-Y.At)/Y.Len;
   const double Env=U>=0&&U<=1?FMath::Sin(UE_DOUBLE_PI*U):0.;
   const double F0=118.+18.*FMath::Sin(T*2.1)+10.*FMath::Sin(T*7.3);
   Phase+=F0/MsgRate;const bool Pulse=Phase>=1.;if(Pulse)Phase-=1.;
   const double Src=(Pulse?1.:0.)*Env;
   // Two resonators.
   const auto Res=[&](double F,double Bw,double& X,double& V,double In)
   {const double W=2*UE_DOUBLE_PI*F/MsgRate,Rr=FMath::Exp(-UE_DOUBLE_PI*Bw/MsgRate);const double Nx=2*Rr*FMath::Cos(W)*X-Rr*Rr*V+In;V=X;X=Nx;return Nx;};
   const double V=Res(Y.F1,90.,B1,B1v,Src)*.9+Res(Y.F2,140.,B2,B2v,Src)*.55;
   // Small speaker, and the set dropping words now and then.
   Lp+=.42*(V-Lp);Hp+=.06*(Lp-Hp);
   if(R.FRand()<.00012f)Crack=R.FRandRange(.03f,.1f);
   const double Keep=Crack>0?.1:1.;Crack=FMath::Max(0.,Crack-1./MsgRate);
   const double Voice=FMath::Atan((Lp-Hp)*1.5)*.5*Keep;
   const bool Sq=T<Open||T>Dur-Close;
   const double Hiss=(R.FRand()*2.f-1.f)*(Sq?.5:.09);
   const double Click=FMath::Exp(-T*340.)*.45+FMath::Exp(-FMath::Max(0.,T-(Dur-Close))*340.)*(T>Dur-Close?.3:0.);
   const double Fade=FMath::Clamp((Dur-T)/.01,0.,1.);
   Out[I]=float((Voice+Hiss+Click)*Fade);
  }
  return B;
 }
 TSharedPtr<FS47AudioBuffer> HissBuffer()
 {
  auto B=MakeShared<FS47AudioBuffer>(1,Rate*2,Rate); FRandomStream R(47241);
  // Steady noise only: no random impulses, crackle slices, squelch pops or
  // the Room 6 walkie's authored 4/7 sequence. The graph supplies the RF band.
  for (int32 I=0;I<B->Length();++I) B->GetChannelData(0)[I]=R.FRandRange(-.65f,.65f);
  return B;
 }
 // Dry pops on a small speaker, a few to a burst, none alike.
 TSharedPtr<FS47AudioBuffer> CrackleBuffer(int32 Seed)
 {
  const double Dur=.42;
  auto B=MakeShared<FS47AudioBuffer>(1,FMath::RoundToInt(Rate*Dur),Rate);FRandomStream R(47611+Seed*37);
  float* Out=B->GetChannelData(0);const int32 N=B->Length();
  for(int32 I=0;I<N;++I)Out[I]=0.f;
  const int32 Pops=R.RandRange(4,9);
  for(int32 P=0;P<Pops;++P)
  {
   const int32 At=R.RandRange(0,N-1);const double Peak=R.FRandRange(.25f,1.f),Fall=R.FRandRange(900.f,3400.f);
   const int32 Len=FMath::Min(N-At,int32(Rate*.006));double Last=0;
   for(int32 I=0;I<Len;++I)
   {
    // Noise that dies at once, differenced so that it has no thump in it.
    const double Now=(R.FRand()*2.f-1.f)*Peak*FMath::Exp(-double(I)/Rate*Fall);
    Out[At+I]=FMath::Clamp(Out[At+I]+float(Now-Last*.6),-1.f,1.f);Last=Now;
   }
  }
  return B;
 }
 US47Audio* OfflineAudio()
 {
  auto* A=NewObject<US47Audio>(GetTransientPackage()); A->AddToRoot();
  A->Ctx=FS47AudioContext::Make(Rate); A->bReady=true;
  A->Master=A->Ctx->CreateGain(); A->Master->Gain.SetValue(.8); A->Master->Connect(A->Ctx->Destination);
  A->SfxBus=A->Ctx->CreateGain(); A->SfxBus->Connect(A->Master);
  return A;
 }
 void ReleaseOffline(US47Audio* A) { A->Ctx->Close(); A->RemoveFromRoot(); }
 bool WriteWave(const FString& Path,const TArray<float>& Samples)
 {
  TArray<uint8> Wav;
  const auto U16=[&](uint16 V){ Wav.Add(uint8(V)); Wav.Add(uint8(V>>8)); };
  const auto U32=[&](uint32 V){ Wav.Add(uint8(V)); Wav.Add(uint8(V>>8)); Wav.Add(uint8(V>>16)); Wav.Add(uint8(V>>24)); };
  const auto Tag=[&](const ANSICHAR* S){ for (int32 I=0;I<4;++I) Wav.Add(uint8(S[I])); };
  const uint32 Bytes=Samples.Num()*2;
  Tag("RIFF"); U32(36+Bytes); Tag("WAVE"); Tag("fmt "); U32(16); U16(1); U16(2);
  U32(Rate); U32(Rate*4); U16(4); U16(16); Tag("data"); U32(Bytes);
  for (float V:Samples) U16(uint16(int16(FMath::RoundToInt(FMath::Clamp(FMath::IsFinite(V)?V:0.f,-1.f,1.f)*32767.f))));
  return FFileHelper::SaveArrayToFile(Wav,*Path);
 }
}

FS47EncounterSound::~FS47EncounterSound() { Reset(); }

bool FS47EncounterSound::Bind(US47Audio* A)
{
 if (!A || !A->bReady || !A->Ctx || !A->SfxBus || A->Ctx->State()!=TEXT("running")) return false;
 if (Context==A->Ctx && Owner.Get()==A) return true;
 Reset(); Context=A->Ctx; Owner=A;
 // Optional private recording. Decode once, never from the audio callback.
 TArray<uint8> Bytes;
 // The original synthesised transmission (Tools/sound-workshop/alien_transmission.py) first; the private film clip
 // alien_interference.wav is only a fallback for local trials and is what keeps distribute-linux.py from packaging.
 const TCHAR* const Names[]={TEXT("radio_transmission_original.wav"),TEXT("alien_interference.wav")};
 bool bOriginal=false;
 for (const TCHAR* Name : Names)
 {
  if (FFileHelper::LoadFileToArray(Bytes,*(FPaths::ProjectContentDir()/TEXT("Audio/Raw/sound_design")/Name)))
   TransmissionBuffer=S47Wa::DecodeWav(Bytes);
  if (TransmissionBuffer) { bOriginal=Name==Names[0]; break; }
 }
 if (TransmissionBuffer)
 {
  // T16-12: only the quiet film clip (rms .004) needs lifting and squashing. The original synthesis is already
  // levelled and soft-clipped (peak .62, rms .18); squashing it again with drive 10 made it a near full-scale square-ish
  // screech (rms .70, 12 dB up), 25 dB over the hiss. It is only brought to a fixed low peak, and played low (below).
  float Peak=0;
  for (int32 Ch=0;Ch<TransmissionBuffer->NumberOfChannels();++Ch){const float* P=TransmissionBuffer->GetChannelData(Ch);for (int32 I=0;I<TransmissionBuffer->Length();++I)Peak=FMath::Max(Peak,FMath::Abs(P[I]));}
  if (Peak>.0001f)
  {
   static constexpr float Drive=10.f;const float Lift=1.f/Peak,Top=FMath::Atan(Drive);
   for (int32 Ch=0;Ch<TransmissionBuffer->NumberOfChannels();++Ch)
   {
    float* P=TransmissionBuffer->GetChannelData(Ch);
    for (int32 I=0;I<TransmissionBuffer->Length();++I)P[I]=(bOriginal?P[I]*Lift:FMath::Atan(P[I]*Lift*Drive)/Top)*TransmissionPeak;
   }
  }
  UE_LOG(LogTemp,Display,TEXT("S47: alien interference recording loaded (%s), %.1f s, peak %.3f before levelling, %.2f after, played at most x%.2f"),bOriginal?TEXT("original synthesis"):TEXT("fallback clip"),TransmissionBuffer->Duration(),Peak,TransmissionPeak,TransmissionGainMax);
  // The first one comes well after the first contact, not with it (and only after TransmissionAfter of it: UpdateRadio).
  NextTransmission=Context->CurrentTime()+FMath::FRandRange(12.f,25.f);
 }
 else UE_LOG(LogTemp,Display,TEXT("S47: alien interference recording absent, radio hiss only"));
 return true;
}

bool FS47EncounterSound::Message(US47Audio* Audio)
{
 if(!Bind(Audio)||bHeld)return false;
 if(!MessageSound)MessageSound=MessageBuffer();
 FS47AudioContext::FBatch Batch(*Context);
 auto Src=Context->CreateBufferSource();Src->Buffer=MessageSound;
 auto Level=Context->CreateGain();Level->Gain.SetValue(.8*FMath::Clamp(double(Owner->RadioLevel),.35,1.));
 Src->Connect(Level);Level->Connect(Owner->SfxBus);Src->Start(Context->CurrentTime());
 MessageSource=Src;
 return true;
}

bool FS47EncounterSound::Talk(US47Audio* Audio,const FVector& At)
{
 if(!Bind(Audio)||bHeld||At.ContainsNaN())return false;
 if(!MessageSound)MessageSound=MessageBuffer();
 FS47AudioContext::FBatch Batch(*Context);
 auto Src=Context->CreateBufferSource();Src->Buffer=MessageSound;
 Src->PlaybackRate.SetValue(FMath::FRandRange(.55f,.72f));
 auto Level=Context->CreateGain();Level->Gain.SetValue(.7);
 auto Pan=Context->CreatePanner();Pan->RefDistance=2.;Pan->RolloffFactor=.6;Pan->MaxDistance=30.;Pan->SetPosition(At.X,At.Y,At.Z);
 Src->Connect(Level);Level->Connect(Pan);Pan->Connect(Owner->SfxBus);Src->Start(Context->CurrentTime());
 TalkSource=Src;
 return true;
}

bool FS47EncounterSound::Crackle(US47Audio* Audio,double Strength)
{
 if(!Bind(Audio)||bHeld||!RadioVolume||!FMath::IsFinite(Strength)||Strength<=0)return false;
 if(Crackles.IsEmpty())for(int32 I=0;I<4;++I)Crackles.Add(CrackleBuffer(I));
 FS47AudioContext::FBatch Batch(*Context);
 Voices.RemoveAll([](const FVoice& V){ return !V.Source || V.Source->Ended(); });
 const double Now=Context->CurrentTime();
 auto Src=Context->CreateBufferSource();Src->Buffer=Crackles[FMath::RandRange(0,Crackles.Num()-1)];
 Src->PlaybackRate.SetValue(FMath::FRandRange(.8f,1.3f));
 auto Gain=Context->CreateGain();Gain->Gain.SetValue(CrackleBase+CrackleSpan*FMath::Clamp(Strength,0.,1.));
 // Under the player's radio slider, as the hiss is.
 Src->Connect(Gain);Gain->Connect(RadioVolume);Src->Start(Now);
 Voices.Add({Src,Gain,Now+.6});
 return true;
}

void FS47EncounterSound::FadeVoices(double Secs)
{
 if (!Context) return;
 FS47AudioContext::FBatch Batch(*Context); const double Now=Context->CurrentTime();
 for (auto& V:Voices) if (V.Source && !V.Source->Ended())
 {
  V.Gain->Gain.CancelAndHoldAtTime(Now);
  V.Gain->Gain.LinearRampToValueAtTime(0,Now+Secs); V.Source->Stop(Now+Secs);
 }
}

bool FS47EncounterSound::Cue(US47Audio* A,EKind Kind,TOptional<FVector> At,double Strength)
{
 if (bHeld || !FMath::IsFinite(Strength) || Strength<=0 || !Bind(A)) return false;
 if (At && (At->ContainsNaN())) return false;
 const int32 K=Index(Kind); const double Now=Context->CurrentTime();
 // One shared mixer avoids a chorus when six Roswells become visible. A new
 // discovery/chase may replace a subtler cue, but cannot stack over its peak.
 if (Now<NextKind[K] || (Now<NextAny && !(K>LastKind && Now>=NextAny-3.1))) return false;
 FS47AudioContext::FBatch Batch(*Context);
 Voices.RemoveAll([](const FVoice& V){ return !V.Source || V.Source->Ended(); });
 FadeVoices(.07);
 if (Cues.IsEmpty()) for (int32 I=0;I<3;++I) Cues.Add(CueBuffer(I));
 auto Src=Context->CreateBufferSource(); Src->Buffer=Cues[K];
 auto Gain=Context->CreateGain(); Gain->Gain.SetValue(FMath::Clamp(Strength,0.,1.)); Src->Connect(Gain);
 if (At)
 {
  auto Pan=Context->CreatePanner(); Pan->RefDistance=2.; Pan->RolloffFactor=.45; Pan->MaxDistance=35.;
  Pan->SetPosition(At->X,At->Y,At->Z); Gain->Connect(Pan); Pan->Connect(A->SfxBus);
 }
 else Gain->Connect(A->SfxBus);
 Src->Start(Now); Voices.Add({Src,Gain,Now+Duration(K)});
 NextAny=Now+3.5; NextKind[K]=Now+(K==0?18.:K==1?9.:7.); LastKind=K;
 CueUntil=Now+Duration(K); RadioMix();
 return true;
}

void FS47EncounterSound::RadioMix()
{
 if (!Context || !RadioGain || !RadioVolume) return;
 FS47AudioContext::FBatch Batch(*Context); const double Now=Context->CurrentTime();
 const bool Duck=Now<CueUntil;
 const double Want=bHeld?0.:RadioLevel*RadioHiss*(Duck?.28:1.);
 RadioGain->Gain.CancelAndHoldAtTime(Now);
 RadioGain->Gain.SetTargetAtTime(Want,Now,bHeld?.025:Duck?.06:.16);
 // Route through SfxBus for the game's quiet scenes, with the independent
 // radio slider applied locally. Never reconnect or modify shared buses.
 RadioVolume->Gain.SetValue(Owner.IsValid()?FMath::Clamp(double(Owner->RadioLevel),0.,1.):0.);
}

void FS47EncounterSound::UpdateRadio(US47Audio* A,double Level,TOptional<FVector> At,bool bInHeld)
{
 if (!FMath::IsFinite(Level)) Level=0.;
 if (At && At->ContainsNaN()) { At.Reset(); Level=0.; }
 if (!Bind(A)) { Hold(true); return; }
 Hold(bInHeld); RadioLevel=FMath::Clamp(Level,0.,1.);
 FS47AudioContext::FBatch Batch(*Context);
 Voices.RemoveAll([](const FVoice& V){ return !V.Source || V.Source->Ended(); });
 if (!RadioSource && RadioLevel>0 && !bHeld)
 {
  RadioSource=Context->CreateBufferSource(); RadioSource->Buffer=HissBuffer(); RadioSource->bLoop=true;
  RadioGain=Context->CreateGain(); RadioGain->Gain.SetValue(0);
  RadioVolume=Context->CreateGain(); RadioVolume->Connect(A->SfxBus);
  auto HP=Context->CreateBiquadFilter(); HP->SetType(TEXT("highpass")); HP->Frequency.SetValue(520.);
  auto LP=Context->CreateBiquadFilter(); LP->SetType(TEXT("lowpass")); LP->Frequency.SetValue(3400.);
  RadioSource->Connect(HP); HP->Connect(LP); LP->Connect(RadioGain);
  RadioPan=Context->CreatePanner(); RadioPan->RefDistance=.7; RadioPan->RolloffFactor=1.; RadioPan->MaxDistance=15.;
  RadioPan->Connect(RadioVolume); RadioGain->Connect(RadioVolume);
  bRadioPlaced=false; RadioSource->Start(Context->CurrentTime());
 }
 if (RadioGain && bRadioPlaced!=At.IsSet())
 {
  RadioGain->Disconnect(); bRadioPlaced=At.IsSet();
  if (bRadioPlaced) RadioGain->Connect(RadioPan); else RadioGain->Connect(RadioVolume);
 }
 if (At && RadioPan) RadioPan->SetPosition(At->X,At->Y,At->Z);
 // A transmission is an occasional intrusion, not a repeating proximity tone.
 const double Now=Context->CurrentTime();
 if (Transmission && (RadioLevel<=.015 || bHeld))
 { StopTransmission(); }
 // How long the set has heard one strongly: a glimpse at the edge of its reach is crackle only.
 if (RadioLevel>TransmissionLevel) { if (LoudSince<0) LoudSince=Now; }
 else if (RadioLevel<.1) LoudSince=-1;
 if (TransmissionBuffer && RadioGain && !bHeld && LoudSince>=0 && Now-LoudSince>=TransmissionAfter && Now>=NextTransmission
     && (!Transmission || Transmission->Ended()))
 {
  Transmission=Context->CreateBufferSource(); Transmission->Buffer=TransmissionBuffer;
  // A short piece of it, from anywhere in it: a voice breaking in and gone, not the whole seven seconds of it.
  const double Whole=TransmissionBuffer->Duration();
  const double Len=FMath::Min(Whole,double(FMath::FRandRange(float(TransmissionShortest),float(TransmissionLongest))));
  const double From=FMath::FRandRange(0.f,float(FMath::Max(0.,Whole-Len)));
  // In over a quarter of a second and out over a third, so it comes up through the hiss and leaves without a cut.
  const double Peak=TransmissionGainMax*RadioLevel;
  TransmissionGain=Context->CreateGain(); TransmissionGain->Gain.SetValueAtTime(0,Now).LinearRampToValueAtTime(Peak,Now+.25)
   .SetValueAtTime(Peak,FMath::Max(Now+.25,Now+Len-.35)).LinearRampToValueAtTime(0,Now+Len);
  // Duller than before (3 kHz): the whistle in it was the screaming part.
  auto Band=Context->CreateBiquadFilter(); Band->SetType(TEXT("lowpass")); Band->Frequency.SetValue(2400.);
  Transmission->Connect(Band); Band->Connect(TransmissionGain);
  // Past the hiss's own low level, still under the player's radio slider and placed with the set.
  if (bRadioPlaced&&RadioPan) TransmissionGain->Connect(RadioPan); else TransmissionGain->Connect(RadioVolume);
  Transmission->Start(Now,From,Len);
  NextTransmission=Now+Len+FMath::FRandRange(float(TransmissionGapMin),float(TransmissionGapMax));
  UE_LOG(LogTemp,Display,TEXT("S47: alien interference transmission at radio level %.2f, gain peak %.3f, %.1f s from %.1f s, next not before %.0f s"),RadioLevel,Peak,Len,From,NextTransmission-Now);
 }
 RadioMix();
}

void FS47EncounterSound::StopTransmission()
{
 if(Transmission && Context)
 {
  FS47AudioContext::FBatch Batch(*Context);
  const double Now=Context->CurrentTime();
  if(TransmissionGain) { TransmissionGain->Gain.CancelAndHoldAtTime(Now); TransmissionGain->Gain.LinearRampToValueAtTime(0,Now+.035); }
  Transmission->Stop(Now+.035);
 }
 Transmission.Reset(); TransmissionGain.Reset();
}

void FS47EncounterSound::Hold(bool bInHeld)
{
 if (bHeld==bInHeld) return;
 bHeld=bInHeld;
 if (bHeld) { FadeVoices(.035); CueUntil=0; if(Transmission) { StopTransmission(); } }
 RadioMix();
}

void FS47EncounterSound::Reset()
{
 if (Context)
 {
  FS47AudioContext::FBatch Batch(*Context);
  for (auto& V:Voices) if (V.Source)
  { V.Gain->Gain.CancelScheduledValues(0); V.Gain->Gain.SetValue(0); V.Source->Stop(); }
  if (RadioVolume) RadioVolume->Gain.SetValue(0);
  if (RadioSource) RadioSource->Stop();
  if (Transmission) Transmission->Stop();
 }
 Transmission.Reset(); TransmissionGain.Reset(); TransmissionBuffer.Reset(); NextTransmission=0; LoudSince=-1;
 Voices.Reset(); Cues.Reset(); RadioSource.Reset(); RadioGain.Reset(); RadioVolume.Reset(); RadioPan.Reset();
 Context.Reset(); Owner.Reset(); bHeld=bRadioPlaced=false; RadioLevel=NextAny=CueUntil=0; LastKind=-1;
 for (double& T:NextKind) T=0;
}

bool FS47EncounterSound::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why)
 { OK&=B; Lines.Add(FString::Printf(TEXT("%s encounter audio: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why)); };
 {
  // The walkie message must carry: heard over a small set, not silent and not all noise.
  const auto Msg=MessageBuffer();const float* P=Msg->GetChannelData(0);double Mid=0;int32 Count=0;float Peak=0;
  for(int32 I=0;I<Msg->Length();++I){Peak=FMath::Max(Peak,FMath::Abs(P[I]));const double Sec=double(I)/22050;if(Sec>.3&&Sec<3.2){Mid+=P[I]*P[I];++Count;}}
  const double Rms=FMath::Sqrt(Mid/FMath::Max(1,Count));
  Lines.Add(FString::Printf(TEXT("INFO walkie message RMS %.3f peak %.2f"),Rms,Peak));
  Check(Rms>.05&&Rms<.5&&Peak<=1.2f,TEXT("walkie message is audible, with a voice over its hiss"));
 }
 auto* A=OfflineAudio(); FS47EncounterSound Mix;
 TArray<float> Samples; Samples.SetNumZeroed(Rate/4*2);
 Check(Mix.Cue(A,EKind::Presence),TEXT("presence starts without any imported audio"));
 Check(!Mix.Cue(A,EKind::Presence) && !Mix.Cue(A,EKind::Discovery),TEXT("simultaneous observations cannot stack cues"));
 A->Ctx->Render(Samples.GetData(),Rate/4,2); A->Ctx->Tick(.25);
 double Peak=0; bool Finite=true;
 for (float V:Samples) { Finite&=FMath::IsFinite(V); Peak=FMath::Max(Peak,FMath::Abs(double(V))); }
 Check(Finite && Peak>.001 && Peak<.12,TEXT("original cue produces finite, bounded low-level audio"));
 Mix.Reset(); Mix.Bind(A);
 // Synthetic fixture proves the optional recording path without publishing private audio.
 Mix.TransmissionBuffer=CueBuffer(0); Mix.NextTransmission=0;
 Mix.UpdateRadio(A,1.);
 Check(!Mix.Transmission && Mix.LoudSince>=0,TEXT("T16-12: a set that has only just heard one gives crackle and hiss, no transmission yet"));
 Mix.LoudSince=A->Ctx->CurrentTime()-TransmissionAfter-.1; Mix.UpdateRadio(A,1.);
 const auto FirstTransmission=Mix.Transmission;
 const double Next=Mix.NextTransmission;
 Check(FirstTransmission && Next>A->Ctx->CurrentTime()+TransmissionGapMin,TEXT("nearby radio starts a bounded recording with a quiet cooldown (100 s or more)"));
 Mix.UpdateRadio(A,1.);
 Check(Mix.Transmission==FirstTransmission && Mix.NextTransmission==Next,TEXT("continuous presence cannot stack or retrigger the recording"));
 Mix.UpdateRadio(A,0.);
 Check(!Mix.Transmission && Mix.LoudSince<0,TEXT("leaving the proximity range stops the recording"));
 {
  // T16-12: the loudest the set gets with one right by it: the hiss at full and a transmission as loud as the real one is after
  // levelling (full-scale noise at TransmissionPeak), rendered for a second through the real graph.
  const auto Saved=Mix.TransmissionBuffer;
  auto Loud=MakeShared<FS47AudioBuffer>(1,Rate*7,Rate);FRandomStream R(47123);
  for (int32 I=0;I<Loud->Length();++I) Loud->GetChannelData(0)[I]=R.FRandRange(-TransmissionPeak,TransmissionPeak);
  Mix.TransmissionBuffer=Loud; Mix.NextTransmission=0; Mix.LoudSince=A->Ctx->CurrentTime()-TransmissionAfter-.1; Mix.UpdateRadio(A,1.);
  const bool Started=bool(Mix.Transmission);
  double Top=0,Sum=0;int32 N=0;
  for (int32 Chunk=0;Chunk<4;++Chunk)
  {
   A->Ctx->Render(Samples.GetData(),Rate/4,2); A->Ctx->Tick(.25); Mix.UpdateRadio(A,1.);
   if (Chunk==0) continue; // past the fade-in
   for (float V:Samples) { Top=FMath::Max(Top,FMath::Abs(double(V))); Sum+=double(V)*V; ++N; }
  }
  const double Rms=FMath::Sqrt(Sum/FMath::Max(1,N));
  Lines.Add(FString::Printf(TEXT("INFO walkie at full level with a transmission: peak %.3f, rms %.4f"),Top,Rms));
  Check(Started && Top<.2 && Rms<.05,TEXT("T16-12: the walkie at its loudest stays under -14 dBFS peak and -26 dBFS rms"));
  Check(TransmissionGainMax<=.25f && TransmissionLongest<=2.5 && TransmissionGapMin>=90. && RadioHiss<=.15 && CrackleBase+CrackleSpan<=.35,
   TEXT("T16-12: transmissions low, short and seldom; hiss and crackle down"));
  Mix.UpdateRadio(A,0.); Mix.TransmissionBuffer=Saved;
 }
 Mix.NextTransmission=0; Mix.LoudSince=A->Ctx->CurrentTime()-TransmissionAfter-.1; Mix.UpdateRadio(A,1.);
 Check(bool(Mix.Transmission),TEXT("the held-radio fixture has a transmission to stop"));
 const double Before=A->Ctx->CurrentTime();
 Mix.Hold(true);
 Check(!Mix.Transmission,TEXT("pause stops the optional transmission"));
 Check(!Mix.Cue(A,EKind::Hunt) && A->Ctx->State()==TEXT("running"),TEXT("holding this mixer blocks cues without suspending the game"));
 A->Ctx->Render(Samples.GetData(),Rate/4,2); A->Ctx->Tick(.25);
 Check(A->Ctx->CurrentTime()>Before,TEXT("unrelated audio clock continues while this mixer is held"));
 Mix.UpdateRadio(A,1.,{},false); A->RadioLevel=0; Mix.UpdateRadio(A,1.);
 A->Ctx->Render(Samples.GetData(),Rate/4,2); A->Ctx->Tick(.25);
 Peak=0; for (int32 I=256;I<Samples.Num();++I) Peak=FMath::Max(Peak,FMath::Abs(double(Samples[I])));
 Check(Mix.RadioVolume && Mix.RadioVolume->Gain.Value()==0 && Peak<.00001,
  TEXT("radio slider really mutes rendered receiver audio"));
 A->RadioLevel=1; Mix.UpdateRadio(A,1.); A->SfxBus->Gain.SetValue(0);
 A->Ctx->Render(Samples.GetData(),Rate/4,2); A->Ctx->Tick(.25);
 Peak=0; for (int32 I=256;I<Samples.Num();++I) Peak=FMath::Max(Peak,FMath::Abs(double(Samples[I])));
 Check(Peak<.00001,TEXT("global quiet scene suppresses the receiver through the existing SFX bus"));
 A->SfxBus->Gain.SetValue(1);
 Mix.Reset();
 A->Ctx->Render(Samples.GetData(),Rate/4,2); A->Ctx->Tick(.25);
 Peak=0; for (float V:Samples) Peak=FMath::Max(Peak,FMath::Abs(double(V)));
 Check(Peak<.00001,TEXT("scene reset leaves no loop or cue playing"));
 ReleaseOffline(A); return OK;
}

bool FS47EncounterSound::RenderTour(const FString& Path)
{
 auto* A=OfflineAudio(); FS47EncounterSound Mix;
 constexpr int32 Frames=Rate*6, Chunk=800;
 TArray<float> Samples; Samples.SetNumZeroed(Frames*2);
 for (int32 F=0;F<Frames;F+=Chunk)
 {
  const double T=double(F)/Rate;
  if (F==Rate/10/Chunk*Chunk) Mix.Cue(A,EKind::Presence);
  if (F==Rate*18/10/Chunk*Chunk) Mix.Cue(A,EKind::Discovery);
  if (F==Rate*35/10/Chunk*Chunk) Mix.Cue(A,EKind::Hunt);
  Mix.UpdateRadio(A,Smooth(T/2.)*Smooth((6.-T)/1.));
  A->Ctx->Render(Samples.GetData()+F*2,Chunk,2); A->Ctx->Tick(double(Chunk)/Rate);
 }
 Mix.Reset(); ReleaseOffline(A); return WriteWave(Path,Samples);
}

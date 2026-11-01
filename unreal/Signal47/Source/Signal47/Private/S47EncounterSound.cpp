#include "S47EncounterSound.h"
#include "S47Audio.h"
#include "Misc/FileHelper.h"

namespace
{
 constexpr int32 Rate=48000;
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
 TSharedPtr<FS47AudioBuffer> HissBuffer()
 {
  auto B=MakeShared<FS47AudioBuffer>(1,Rate*2,Rate); FRandomStream R(47241);
  // Steady noise only: no random impulses, crackle slices, squelch pops or
  // the Room 6 walkie's authored 4/7 sequence. The graph supplies the RF band.
  for (int32 I=0;I<B->Length();++I) B->GetChannelData(0)[I]=R.FRandRange(-.65f,.65f);
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
 const double Want=bHeld?0.:RadioLevel*.12*(Duck?.28:1.);
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
 RadioMix();
}

void FS47EncounterSound::Hold(bool bInHeld)
{
 if (bHeld==bInHeld) return;
 bHeld=bInHeld;
 if (bHeld) { FadeVoices(.035); CueUntil=0; }
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
 }
 Voices.Reset(); Cues.Reset(); RadioSource.Reset(); RadioGain.Reset(); RadioVolume.Reset(); RadioPan.Reset();
 Context.Reset(); Owner.Reset(); bHeld=bRadioPlaced=false; RadioLevel=NextAny=CueUntil=0; LastKind=-1;
 for (double& T:NextKind) T=0;
}

bool FS47EncounterSound::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why)
 { OK&=B; Lines.Add(FString::Printf(TEXT("%s encounter audio: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why)); };
 auto* A=OfflineAudio(); FS47EncounterSound Mix;
 TArray<float> Samples; Samples.SetNumZeroed(Rate/4*2);
 Check(Mix.Cue(A,EKind::Presence),TEXT("presence starts without any imported audio"));
 Check(!Mix.Cue(A,EKind::Presence) && !Mix.Cue(A,EKind::Discovery),TEXT("simultaneous observations cannot stack cues"));
 A->Ctx->Render(Samples.GetData(),Rate/4,2); A->Ctx->Tick(.25);
 double Peak=0; bool Finite=true;
 for (float V:Samples) { Finite&=FMath::IsFinite(V); Peak=FMath::Max(Peak,FMath::Abs(double(V))); }
 Check(Finite && Peak>.001 && Peak<.12,TEXT("original cue produces finite, bounded low-level audio"));
 Mix.UpdateRadio(A,1.); const double Before=A->Ctx->CurrentTime();
 Mix.Hold(true);
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

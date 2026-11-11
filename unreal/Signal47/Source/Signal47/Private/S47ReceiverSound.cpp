#include "S47ReceiverSound.h"
#include "S47Audio.h"
#include "S47SignalVoice.h"
#include "S47WebAudio.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
 const TCHAR* const MixKey = TEXT("receiver_reference");
 // D48-20261110 (D46-16): Source null: synthesized here (no recording of it is staged), a plain loop of a few seconds.
 struct FReference { const TCHAR* Recipe; const TCHAR* Name; const TCHAR* Source; int32 Kind; int32 Cause; const TCHAR* Blurb; };
 // These are listening references, never substituted for a SARO acquisition. Actual
 // spacecraft/instrument/date provenance remains in the bank and source credits.
 const FReference References[] = {
  {TEXT("nasa_saturn_lightning_scan"), TEXT("LIGHTNING SFERICS"), TEXT("nasa_saturn_lightning"), 0, -1, TEXT("Sharp crackles and pops from storms a long way off. Random, with no rhythm to them.")},
  {TEXT("nasa_earth_chorus_scan"), TEXT("WHISTLERS AND CHORUS"), TEXT("nasa_earth_chorus"), 0, -1, TEXT("Falling whistles and rising chirps from the Earth's own magnetic field, loudest near dawn.")},
  {TEXT("nasa_jupiter_scan"), TEXT("PLANET BURSTS"), TEXT("nasa_cassini_jupiter"), 0, -1, TEXT("Slow rushing roars from a planet's magnetic field. They rise and fall, but keep no beat.")},
  {TEXT("nasa_saturn_scan"), TEXT("AURORAL HISS"), TEXT("nasa_saturn_eerie"), 0, -1, TEXT("A thin, steady whistling from a planet's aurora. Eerie, but it never repeats a pattern.")},
  {TEXT("nasa_saturn_bizarre_scan"), TEXT("AURORAL WARBLE"), TEXT("nasa_saturn_bizarre"), 0, -1, TEXT("Warbling tones that slide up and down, also from an aurora. Nothing in it is timed.")},
  {TEXT("nebula_ambience"), TEXT("GALACTIC BACKGROUND"), TEXT("space_nebula"), 0, -1, TEXT("The faint, even hiss of the whole sky. Nothing in it stands out.")},
  {TEXT("synth_pulsar"), TEXT("PULSAR"), nullptr, 0, -1, TEXT("A dead regular tick, one every fraction of a second, that never misses and never drifts off its pace.")},
  {TEXT("synth_radar_sweep"), TEXT("AIRCRAFT RADAR SWEEP"), nullptr, 1, 1, TEXT("A rough burst every few seconds as an airport radar dish turns past. It keeps the clock, not the stars.")},
  {TEXT("synth_power_hum"), TEXT("POWER-LINE HUM"), nullptr, 1, 0, TEXT("A steady low drone with overtones, from the mains. It swells when the station load comes up.")},
  {TEXT("synth_sat_beacon"), TEXT("SATELLITE BEACON"), nullptr, 1, 1, TEXT("A clean tone beeping on and off at one pitch, drifting slowly as the satellite passes over.")}
 };
 uint32 Hash(uint32 V){V^=V>>16;V*=0x7feb352du;V^=V>>15;V*=0x846ca68bu;return V^(V>>16);}
 float Noise(uint32 V){return float(Hash(V)&0xffffff)/float(0x800000)-1.f;}
 // One mono loop of eight seconds. Whole cycles of every tone, so the loop has no click.
 TSharedRef<FS47AudioBuffer> MakeSynth(FS47AudioContext& Ctx,int32 Index)
 {
  const float Rate=float(Ctx.SampleRate);const int32 Len=int32(Rate*8);
  TSharedRef<FS47AudioBuffer> B=Ctx.CreateBuffer(1,Len,Rate);float* D=B->GetChannelData(0);
  double Phase=0;
  for(int32 I=0;I<Len;++I)
  {
   const double T=I/double(Rate);const float W=Noise(uint32(I)*2654435761u+uint32(Index));double V=0;
   switch(Index)
   {
   case 6: // pulsar: a tick every 0.8 s
   {const double P=FMath::Fmod(T,.8);V=.03*W+(P<.012?(1-P/.012)*(.55*W+.35*FMath::Sin(2*PI*700*P)):0.);break;}
   case 7: // radar: a rough sweep every 4 s as the dish turns past
   {const double P=FMath::Fmod(T,4.);const double E=P<.5?FMath::Square(FMath::Sin(PI*P/.5)):0.;V=.02*W+E*(.45*W+.4*FMath::Sin(2*PI*(500*P+700*P*P)));break;}
   case 8: // power line: sixty hertz with overtones and a slow swell
   {const double Swell=.8+.2*FMath::Sin(2*PI*T/8);V=Swell*(.34*FMath::Sin(2*PI*60*T)+.2*FMath::Sin(2*PI*120*T)+.12*FMath::Sin(2*PI*180*T)+.07*FMath::Sin(2*PI*300*T))+.015*W;break;}
   default: // beacon: a tone for a quarter second every second, pitch drifting a little
   {const double P=FMath::Fmod(T,1.);const double Env=FMath::Clamp(FMath::Min(P,.25-P)/.006,0.,1.);Phase+=2*PI*(1000+25*FMath::Sin(2*PI*T/8))/Rate;V=.01*W+Env*.4*FMath::Sin(Phase);break;}
   }
   D[I]=float(FMath::Clamp(V,-.9,.9));
  }
  return B;
 }
 bool ReferenceMetadata(const FString& Name, double& Duration, bool& Loop)
 {
  FString Text;
  if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir()/TEXT("Audio/Raw/sound_design/bank.json")))) return false;
  TSharedPtr<FJsonObject> Bank;
  if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Bank) || !Bank) return false;
  const TArray<TSharedPtr<FJsonValue>>* Recipes = nullptr;
  const TSharedPtr<FJsonObject>* Sources = nullptr;
  if (!Bank->TryGetArrayField(TEXT("recipes"), Recipes) || !Bank->TryGetObjectField(TEXT("sources"), Sources)) return false;
  for (const auto& Value : *Recipes)
  {
   const auto J = Value->AsObject(); FString Id;
   if (!J || !J->TryGetStringField(TEXT("name"), Id) || Id != Name) continue;
   FS47MixRecipe R; TArray<FString> Errors;
   if (!FS47SoundMixer::ParseRecipe(J, R, Errors) || R.Use != ES47SoundUse::Receiver || R.Layers.Num() != 1) return false;
   const auto& L = R.Layers[0];
   bool Identity = false; for (const auto& Ref : References) if (Name == Ref.Recipe && L.Source == Ref.Source) Identity = true;
   if (!Identity) return false;
   const bool Synthetic = Name == TEXT("nebula_ambience");
   if (L.Rate != 1 || L.Start != 0 || L.Length != 0 || L.HighPass != 0 || L.LowPass != 0 || L.Envelope != TEXT("constant") || L.bLoop != Synthetic) return false;
   const TSharedPtr<FJsonObject>* Source = nullptr; FString Role;
   if (!(*Sources)->TryGetObjectField(L.Source, Source) || !(*Source)->TryGetStringField(TEXT("role"), Role)
    || Role != (Synthetic ? TEXT("designed_ambience") : TEXT("scientific_recording"))) return false;
   Loop = L.bLoop;
   return (*Source)->TryGetNumberField(TEXT("prepared_duration_s"), Duration) && FMath::IsFinite(Duration) && Duration > 0;
  }
  return false;
 }
}

FS47ReceiverSound::FS47ReceiverSound(US47Audio* InAudio, TSharedPtr<FS47AudioNode> InDestination)
 : Audio(InAudio), Destination(InDestination) {}
FS47ReceiverSound::~FS47ReceiverSound() { Stop(); }

bool FS47ReceiverSound::Allows(const FString& Recipe, ES47SoundUse Use)
{
 if (Use != ES47SoundUse::Receiver) return false;
 for (const auto& R : References) if (Recipe == R.Recipe) return true;
 return false;
}
void FS47ReceiverSound::Next()
{
 Stop(); Selected = (Selected + 1) % UE_ARRAY_COUNT(References); Feedback.Empty();
}
bool FS47ReceiverSound::Toggle()
{
 if (bPlaying) { Stop(); return true; }
 auto* A = Audio.Get();
 if (!A || !A->Ctx || A->Ctx->State() != TEXT("running")) return false;
 TArray<FString> Errors;
 const FString Recipe = References[Selected].Recipe;
 if (!References[Selected].Source)
 {
  SynthSource = A->Ctx->CreateBufferSource(); SynthSource->Buffer = MakeSynth(*A->Ctx, Selected); SynthSource->bLoop = true;
  SynthGain = A->Ctx->CreateGain(); SynthGain->Gain.SetValue(.5);
  SynthSource->Connect(SynthGain); SynthGain->Connect(Destination.IsValid() ? Destination : TSharedPtr<FS47AudioNode>(A->Ctx->Destination));
  SynthSource->Start();
  Feedback.Empty(); bPlaying = true; EndAt = 0;
  return true;
 }
 double Duration = 0; bool Loop = false;
 if (!Allows(Recipe, ES47SoundUse::Receiver) || !ReferenceMetadata(Recipe, Duration, Loop)
  || !A->PlaySoundMix(MixKey, Recipe, ES47SoundUse::Receiver, Errors, Destination))
 {
  Feedback = TEXT("REFERENCE UNAVAILABLE");
  for (const auto& Error : Errors) UE_LOG(LogTemp, Warning, TEXT("S47: receiver reference: %s"), *Error);
  return false;
 }
 Feedback.Empty(); bPlaying = true;
 EndAt = Loop ? 0 : A->Ctx->CurrentTime() + Duration;
 A->SoundMixFrame(MixKey, 1, 0, 1, A->Ctx->CurrentTime());
 return true;
}
void FS47ReceiverSound::Stop()
{
 if (SynthSource.IsValid()) { SynthSource->Stop(); SynthSource->Disconnect(); if (SynthGain.IsValid()) SynthGain->Disconnect(); SynthSource.Reset(); SynthGain.Reset(); }
 else if (auto* A = Audio.Get()) if (bPlaying) A->StopSoundMix(MixKey, 0);
 bPlaying = false;
 EndAt = 0;
}
void FS47ReceiverSound::Step(bool bMayListen, double T)
{
 if (!bMayListen) { Stop(); return; }
 if (auto* A = Audio.Get())
 {
  if (!A->Ctx || A->Ctx->State() == TEXT("closed")) { Stop(); return; }
  if (bPlaying && EndAt > 0 && A->Ctx->CurrentTime() >= EndAt) { Stop(); return; }
  if (bPlaying && !SynthSource.IsValid()) A->SoundMixFrame(MixKey, 1, 0, 1, T);
 }
 else bPlaying = false;
}
int32 FS47ReceiverSound::Count() { return UE_ARRAY_COUNT(References); }
const TCHAR* FS47ReceiverSound::NameOf(int32 I) { return References[FMath::Clamp(I, 0, Count() - 1)].Name; }
const TCHAR* FS47ReceiverSound::BlurbOf(int32 I) { return References[FMath::Clamp(I, 0, Count() - 1)].Blurb; }
int32 FS47ReceiverSound::KindOf(int32 I) { return References[FMath::Clamp(I, 0, Count() - 1)].Kind; }
int32 FS47ReceiverSound::FeederCause(int32 I) { return I >= 0 && I < Count() ? References[I].Cause : -1; }
float FS47ReceiverSound::Signature(int32 Index, float T, float F)
{
 // The look of each source on a spectrogram: time and frequency 0..1, brightness 0..1. Faint hash under everything.
 const float Base = .08f + .07f * (Noise(uint32(T * 97) * 7919u + uint32(F * 53) * 104729u + uint32(Index) * 31u) * .5f + .5f);
 float V = 0;
 switch (Index)
 {
 case 0: V = Noise(uint32(T * 61) * 2654435761u + 5u) > .62f ? 1.f - F * .5f : 0.f; break; // sferics: broadband spikes, no rhythm
 case 1: V = FMath::Max(FMath::Exp(-FMath::Square((F - (.85f - .6f * FMath::Frac(T * 3.f))) * 14)) * .9f * (FMath::Frac(T * 3.f) < .8f), 0.f); break; // chorus: falling whistles
 case 2: V = FMath::Exp(-FMath::Square((F - (.5f + .3f * FMath::Sin(T * 9.f))) * 5)) * (.5f + .5f * FMath::Sin(T * 23.f)); break; // bursts: wide slow bands
 case 3: V = FMath::Exp(-FMath::Square((F - (.3f + .05f * FMath::Sin(T * 17.f))) * 22)) * .8f; break; // hiss: one thin steady line
 case 4: V = FMath::Exp(-FMath::Square((F - (.5f + .35f * FMath::Sin(T * 21.f))) * 18)) * .85f; break; // warble: sliding line
 case 5: V = 0; break; // background: nothing stands out
 case 6: V = FMath::Frac(T * 10.f) < .08f ? .95f : 0.f; break; // pulsar: even thin vertical lines
 case 7: V = FMath::Frac(T * 2.f) < .22f ? .9f * FMath::Exp(-FMath::Square((F - (.15f + .7f * FMath::Frac(T * 2.f) / .22f)) * 4)) + .3f : 0.f; break; // radar: a wide chirp each sweep
 case 8: V = (FMath::Exp(-FMath::Square((F - .12f) * 60)) + .6f * FMath::Exp(-FMath::Square((F - .24f) * 60)) + .4f * FMath::Exp(-FMath::Square((F - .36f) * 60)) + .25f * FMath::Exp(-FMath::Square((F - .6f) * 60))); break; // hum: stacked steady lines
 default: V = FMath::Frac(T * 8.f) < .25f ? .95f * FMath::Exp(-FMath::Square((F - (.55f + .02f * FMath::Sin(T * 6.f))) * 70)) : 0.f; break; // beacon: short dashes on one pitch
 }
 return FMath::Clamp(Base + V, 0.f, 1.f);
}
FString FS47ReceiverSound::Label() const
{
 return FString::Printf(TEXT("REFERENCE PLAYBACK\n%s / %s"), References[Selected].Name,
  Feedback.IsEmpty() ? bPlaying ? TEXT("PLAY") : TEXT("STOP") : *Feedback);
}

bool FS47ReceiverSound::SelfTest(TArray<FString>& Lines)
{
 bool Ok = true;
 auto Check = [&](bool Good, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s receiver reference: %s"), Good ? TEXT("PASS") : TEXT("FAIL"), What)); Ok &= Good; };
 Check(Allows(TEXT("nasa_saturn_scan"), ES47SoundUse::Receiver)
  && Allows(TEXT("nebula_ambience"), ES47SoundUse::Receiver)
  && !Allows(TEXT("nasa_saturn_scan"), ES47SoundUse::PcTransfer)
  && !Allows(TEXT("modem_transfer"), ES47SoundUse::Receiver), TEXT("reference selections require receiver use and cannot select modem transfers"));
 FS47ReceiverSound Missing(nullptr);
 Check(!Missing.Toggle() && !Missing.Playing(), TEXT("missing audio stays stopped"));
 const FString Path = FPaths::ProjectContentDir() / TEXT("Audio/Raw/sound_design/bank.json");
 if (!IFileManager::Get().FileExists(*Path)) return Ok;
 US47Audio* A = NewObject<US47Audio>(); A->Ctx = FS47AudioContext::Make();
 TArray<FString> Errors;
 Check(A->LoadSoundBank(Path, Errors), TEXT("staged receiver reference bank loads"));
 {
  FS47ReceiverSound Ref(A, A->Ctx->Destination);
  bool Each = true;
  constexpr int32 Frames = 12288; // whole render quanta, including after graph mutations
  TArray<float> Samples; Samples.SetNumZeroed(Frames * 2);
  for (int32 I = 0; I < UE_ARRAY_COUNT(References); ++I)
  {
   Each &= Ref.Toggle() && Ref.Playing();
   A->Ctx->Render(Samples.GetData(), Frames);
   double Energy = 0; for (float V : Samples) { Each &= FMath::IsFinite(V); Energy += V * V; }
   Each &= Energy > 0;
   Ref.Next(); Each &= !Ref.Playing();
  }
  Check(Each, TEXT("every staged reference plays finite nonzero samples and selecting another stops it"));
  bool Roles = true;
  for (const auto& R : References) { double Duration=0; bool Loop=false; Roles &= !R.Source || ReferenceMetadata(R.Recipe,Duration,Loop); }
  Check(Roles,TEXT("references preserve rate one, one source, source role and original playback envelope"));
  {
   // D48-20261110 (D46-16): the man-made and pulsar references are synthesized; each is named, described, finite, different from the others,
   // and only the two transmitters and the feeder stand for a cause of the first case.
   bool Set = Count() == 10;
   for (int32 I = 0; I < Count(); ++I) Set &= FString(NameOf(I)).Len() > 3 && FString(BlurbOf(I)).Len() > 30 && (KindOf(I) == 1) == (FeederCause(I) >= 0);
   float Seen = 0; for (int32 I = 0; I < Count(); ++I) for (int32 X = 0; X < 12; ++X) for (int32 Y = 0; Y < 8; ++Y) { const float V = Signature(I, X / 12.f, Y / 8.f); Set &= FMath::IsFinite(V) && V >= 0 && V <= 1; Seen += V; }
   Check(Set && Seen > 0 && FeederCause(8) == 0 && FeederCause(7) == 1 && FeederCause(9) == 1 && FeederCause(6) == -1, TEXT("references are named known sources with a plain description and a spectrogram signature, the man-made ones standing for a cause"));
  }
  Check(Ref.Toggle(), TEXT("reference transport restarts after selection"));
  const double BeforePause=A->Ctx->CurrentTime();
  A->Ctx->Suspend(true); A->Ctx->Render(Samples.GetData(), Frames);
  bool Quiet = true; for (float V : Samples) Quiet &= V == 0;
  Check(Quiet && Ref.Playing() && A->Ctx->CurrentTime()==BeforePause, TEXT("global audio pause silences reference and freezes its audio clock"));
  A->Ctx->Suspend(false); A->Ctx->Render(Samples.GetData(),Frames);
  double ResumedEnergy=0;for(float V:Samples)ResumedEnergy+=V*V;
  Check(ResumedEnergy>0&&A->Ctx->CurrentTime()>BeforePause,TEXT("reference resumes audible samples after global audio pause"));
  Ref.Step(false, A->Ctx->CurrentTime());
  A->Ctx->Render(Samples.GetData(), Frames); Quiet = true; for (float V : Samples) Quiet &= V == 0;
  Check(Quiet && !Ref.Playing(), TEXT("leaving the listening position stops reference playback"));
 }
 // Exercise the actual receiver/RAW voice with the staged recordings at both
 // normal and slowed tape speed, including releasing its looping sources.
 const auto Contact=A->SoundSource(TEXT("contact")), Nebula=A->SoundSource(TEXT("space_nebula"));
 Check(Contact.IsValid()&&Nebula.IsValid(),TEXT("receiver and RAW recording textures are available"));
 for(double Rate:{1.,.5})
 {
  auto C=FS47AudioContext::Make(48000);
  auto Noise=C->CreateBuffer(1,48000,48000);
  FS47SignalVoice Voice(C,C->Destination,Noise,Rate,Contact,Nebula);
  float Samples[256];double Energy=0;bool Finite=true;
  for(int32 Frame=0;Frame<96000;Frame+=128)
  {
   Voice.Frame(.9,.6,Frame/48000.*Rate);C->Render(Samples,128);
   for(float V:Samples){Finite&=FMath::IsFinite(V)&&FMath::Abs(V)<=1;Energy+=V*V;}
   C->Tick(128./48000);
  }
  Voice.Stop();
  for(int32 Frame=0;Frame<96000;Frame+=128){C->Render(Samples,128);C->Tick(128./48000);}
  bool Quiet=true;for(float V:Samples)Quiet&=FMath::Abs(V)<1.e-7;
  Check(Finite&&Energy>0&&Quiet,Rate==1?TEXT("recorded receiver voice renders finite samples and releases loops"):TEXT("recorded RAW voice at half speed renders finite samples and releases loops"));
  C->Close();
 }
 A->Ctx->Close(); A->Ctx.Reset();
 return Ok;
}

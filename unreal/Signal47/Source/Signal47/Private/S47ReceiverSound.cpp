#include "S47ReceiverSound.h"
#include "S47Audio.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
 const TCHAR* const MixKey = TEXT("receiver_reference");
 struct FReference { const TCHAR* Recipe; const TCHAR* Name; const TCHAR* Source; };
 // These are listening references, never substituted for a SARO acquisition. Actual
 // spacecraft/instrument/date provenance remains in the bank and source credits.
 const FReference References[] = {
  {TEXT("nasa_saturn_scan"), TEXT("PLANETARY RADIO A"), TEXT("nasa_saturn_eerie")},
  {TEXT("nasa_jupiter_scan"), TEXT("PLANETARY RADIO B"), TEXT("nasa_cassini_jupiter")},
  {TEXT("nasa_saturn_bizarre_scan"), TEXT("PLANETARY RADIO C"), TEXT("nasa_saturn_bizarre")},
  {TEXT("nasa_saturn_lightning_scan"), TEXT("PLANETARY RADIO D"), TEXT("nasa_saturn_lightning")},
  {TEXT("nasa_earth_chorus_scan"), TEXT("ELECTROMAGNETIC CHORUS"), TEXT("nasa_earth_chorus")},
  {TEXT("nebula_ambience"), TEXT("SYNTHETIC SPACE TEXTURE"), TEXT("space_nebula")}
 };
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
 if (auto* A = Audio.Get()) if (bPlaying) A->StopSoundMix(MixKey, 0);
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
  if (bPlaying) A->SoundMixFrame(MixKey, 1, 0, 1, T);
 }
 else bPlaying = false;
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
  for (const auto& R : References) { double Duration=0; bool Loop=false; Roles &= ReferenceMetadata(R.Recipe,Duration,Loop); }
  Check(Roles,TEXT("references preserve rate one, one source, source role and original playback envelope"));
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
 A->Ctx->Close(); A->Ctx.Reset();
 return Ok;
}

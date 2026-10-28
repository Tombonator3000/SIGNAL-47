#include "S47GraphicsTest.h"

#include "S47Flow.h"
#include "S47Graphics.h"
#include "S47Json.h"
#include "S47Keys.h"
#include "S47Main.h"
#include "S47UI.h"
#include "S47Web.h"
#include "SS47Settings.h"
#include "Engine/Engine.h"
#include "Engine/Scene.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47GraphicsTest, Log, All);

namespace
{
 FS47GraphicsSettings Custom(bool bAfterMenu)
 {
  auto G = FS47GraphicsSettings::ForPreset(TEXT("high"));
  G.RenderScale = bAfterMenu ? 80 : 75; G.Shadows = 1; G.IndirectLighting = 0;
  G.Reflections = 1; G.Textures = 1; G.AntiAliasing = 1; G.Fog = 0; G.Clouds = 0;
  G.Effects = 0; G.Detail = 0; G.bHardwareRayTracing = false;
  return G;
 }
 double Cvar(const TCHAR* Name)
 {
  const IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name);
  return V ? V->GetFloat() : -99999.;
 }
 TSharedPtr<SS47Settings> Menu()
 {
  if (!FSlateApplication::IsInitialized()) return {};
  TSharedPtr<SWidget> W = FSlateApplication::Get().GetKeyboardFocusedWidget();
  return W.IsValid() && W->GetTypeAsString() == TEXT("SS47Settings") ? StaticCastSharedPtr<SS47Settings>(W) : nullptr;
 }
 FString StorePath() { return FPaths::ProjectSavedDir() / TEXT("SaveGames/settings.json"); }
}

struct FS47GraphicsTest::FState
{
 FString Mode, Root, Picture;
 int32 Stage = 0, Checks = 0;
 bool bDone = false;
 double Elapsed = 0, At = 0, Hold = 3, EffectsGroup = 0, MaterialQuality = 0;
 float Brightness = 0;
 TArray<TSharedPtr<FJsonValue>> Evidence;
};

FS47GraphicsTest::FS47GraphicsTest(US47Main* InM) : State(MakeUnique<FState>()), M(InM)
{
 FParse::Value(FCommandLine::Get(), TEXT("S47GraphicsContract="), State->Mode);
}
FS47GraphicsTest::~FS47GraphicsTest() = default;

bool FS47GraphicsTest::Check(bool bGood, const FString& What)
{
 ++State->Checks;
 auto E = S47J::New(); S47J::Set(E, TEXT("check"), What); S47J::Set(E, TEXT("pass"), bGood); S47J::Set(E, TEXT("elapsed"), State->Elapsed);
 State->Evidence.Add(MakeShared<FJsonValueObject>(E));
 if (!bGood) { Finish(false, What); return false; }
 UE_LOG(LogS47GraphicsTest, Display, TEXT("S47: graphics contract PASS: %s"), *What); return true;
}

void FS47GraphicsTest::Stage(int32 Next, double Hold)
{
 State->Stage = Next; State->At = State->Elapsed; State->Hold = Hold;
}

void FS47GraphicsTest::Finish(bool bSuccess, const FString& Reason)
{
 if (State->bDone) return;
 State->bDone = true;
 auto R = S47J::New(); S47J::Set(R, TEXT("status"), bSuccess ? TEXT("PASS") : TEXT("FAIL"));
 S47J::Set(R, TEXT("mode"), State->Mode); S47J::Set(R, TEXT("checks"), State->Checks);
 S47J::Set(R, TEXT("reason"), Reason); S47J::Set(R, TEXT("stage"), State->Stage); R->SetArrayField(TEXT("evidence"), State->Evidence);
 if (M && M->Flow) S47J::Set(R, TEXT("graphics"), M->Flow->Graphics.ToJson());
 if (M && M->Web) S47J::Set(R, TEXT("renderer"), M->Web->GraphicsReadback());
 S47J::Set(R, TEXT("limits"), TEXT("Isolated production settings callbacks, actual Slate key events, native renderer readbacks and a separate-process settings reload. No personal profile, story progression, hardware gamepad, human input or cross-hardware FPS guarantee."));
 if (!State->Root.IsEmpty() && !FFileHelper::SaveStringToFile(S47J::Write(R, true), *(State->Root / TEXT("result.json")))) bSuccess = false;
 UE_LOG(LogS47GraphicsTest, Display, TEXT("S47: graphics contract %s COMPLETE: %s, %d checks, %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *State->Mode, State->Checks, *Reason);
 FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1);
}

void FS47GraphicsTest::Shot(const TCHAR* Name)
{
 FScreenshotRequest::RequestScreenshot(State->Root / (FString(Name) + TEXT(".png")), true, false);
}

bool FS47GraphicsTest::VerifyRenderer(bool bVeryLow)
{
 const auto& G = M->Flow->Graphics;
 const FPostProcessSettings* P = M->Web->GetGraphicsPostProcess();
 if (!Check(P != nullptr, TEXT("native lighting postprocess is available"))) return false;
 if (!Check(FMath::IsNearlyEqual(Cvar(TEXT("r.ScreenPercentage")), double(G.RenderScale), .01), TEXT("render scale reaches the active engine CVar"))) return false;
 if (!Check(FMath::IsNearlyEqual(Cvar(TEXT("sg.ShadowQuality")), double(G.Shadows), .01) && Cvar(TEXT("sg.AntiAliasingQuality")) == G.AntiAliasing,
  TEXT("shadow and antialiasing choices reach their engine groups"))) return false;
 if (!Check(P->DynamicGlobalIlluminationMethod == EDynamicGlobalIlluminationMethod::None && Cvar(TEXT("r.Lumen.HardwareRayTracing")) == 0,
  TEXT("indirect lighting and hardware tracing are disabled in native rendering"))) return false;
 if (!Check(P->ReflectionMethod == (bVeryLow ? EReflectionMethod::None : EReflectionMethod::ScreenSpace), TEXT("native reflection method matches off or screen-space selection"))) return false;
 if (!Check(Cvar(TEXT("r.VolumetricFog")) == 0 && !S47J::Bool(M->Web->GraphicsReadback(), TEXT("cloudVisible"), true),
  TEXT("fog and cloud choices remain off after shadow-group application"))) return false;
 if (!Check(Cvar(TEXT("r.Streaming.MipBias")) == (G.Textures == 0 ? 2 : G.Textures == 1 ? 1 : 0), TEXT("streamed texture detail changes the active mip allowance"))) return false;
 if (!Check(Cvar(TEXT("sg.EffectsQuality")) == State->EffectsGroup && Cvar(TEXT("r.MaterialQualityLevel")) == State->MaterialQuality,
  TEXT("graphics controls do not mutate the broad effects group or material permutation level"))) return false;
 if (!Check(M->Web->GetGraphicsSettings() == G, TEXT("renderer retains the complete effective custom settings"))) return false;
 return true;
}

void FS47GraphicsTest::Step(double Dt)
{
 auto& T = *State;
 if (T.Mode.IsEmpty() || T.bDone) return;
 T.Elapsed += FMath::Max(0., Dt);
 if (T.Elapsed > 180) { Finish(false, TEXT("graphics contract timed out")); return; }
 if (!M || !M->Flow || !M->Web || !M->Web->IsLoaded() || T.Elapsed - T.At < T.Hold) return;
 if (T.Stage == 0)
 {
  FString UserDir, Quality;
  FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir);
  FParse::Value(FCommandLine::Get(), TEXT("S47Quality="), Quality);
  UserDir = FPaths::ConvertRelativePathToFull(UserDir); FPaths::CollapseRelativeDirectories(UserDir);
  if (!((UserDir.StartsWith(TEXT("/tmp/")) || UserDir.StartsWith(TEXT("/var/tmp/"))) && FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()), UserDir)) ||
   (T.Mode != TEXT("write") && T.Mode != TEXT("read")) || FParse::Param(FCommandLine::Get(), TEXT("S47SmokeTest")) || !Quality.IsEmpty() || !FParse::Param(FCommandLine::Get(), TEXT("S47Title")))
  { Finish(false, TEXT("requires write/read, isolated /tmp UserDir, S47Title, no SmokeTest and no S47Quality override")); return; }
  T.Root = FPaths::ProjectSavedDir() / TEXT("GraphicsContract") / T.Mode;
  IFileManager::Get().MakeDirectory(*T.Root, true);
  if (!Check(M->Flow->CaseId == 0 && M->Mode == US47Main::EMode::Title, TEXT("isolated title profile does not enter a story case"))) return;
  auto O = M->Flow->Settings();
  if (!Check(bool(O.OnGraphics) && bool(O.OnReapplyGraphics), TEXT("production graphics and display-reapply callbacks are present"))) return;
  T.EffectsGroup = Cvar(TEXT("sg.EffectsQuality")); T.MaterialQuality = Cvar(TEXT("r.MaterialQualityLevel"));
  T.Picture = M->Flow->Picture; T.Brightness = M->Flow->Brightness;
  if (T.Mode == TEXT("read"))
  {
   if (!Check(M->Flow->Graphics == Custom(true), TEXT("a fresh process restores every saved custom field before any settings write"))) return;
   if (!VerifyRenderer(false)) return;
   FString Raw; const bool bRead = FFileHelper::LoadFileToString(Raw, *StorePath());
   const auto Store = bRead ? S47J::Read(Raw) : S47J::FObj();
   if (!Check(Store.IsValid() && FS47GraphicsSettings::FromJson(S47J::Obj(Store, TEXT("graphics")), TEXT("high")) == Custom(true), TEXT("fresh disk settings agree with the live startup settings"))) return;
   if (!Check(M->Flow->OpenTestView(TEXT("settings:graphics")), TEXT("restored graphics page opens through the production flow"))) return;
   Stage(20); return;
  }
  FString InitialRaw; const bool bInitialRead = FFileHelper::LoadFileToString(InitialRaw, *StorePath());
  const auto InitialStore = bInitialRead ? S47J::Read(InitialRaw) : S47J::FObj();
  if (!Check(!S47J::Obj(InitialStore, TEXT("graphics")).IsValid() && M->Flow->Graphics == FS47GraphicsSettings::ForPreset(TEXT("high")), TEXT("profile without a graphics object retains the legacy High defaults"))) return;
  O.OnGraphics(FS47GraphicsSettings::ForPreset(TEXT("verylow")));
  Stage(1); return;
 }
 if (T.Stage == 1)
 {
  if (!Check(M->Flow->Graphics.PresetId() == TEXT("verylow"), TEXT("Very Low preset is applied through the production callback")) || !VerifyRenderer(true)) return;
  auto O = M->Flow->Settings(); O.OnGraphics(FS47GraphicsSettings::ForPreset(TEXT("high")));
  Stage(2); return;
 }
 if (T.Stage == 2)
 {
  const auto High = FS47GraphicsSettings::ForPreset(TEXT("high"));
  const FPostProcessSettings* P = M->Web->GetGraphicsPostProcess();
  if (!Check(M->Flow->Graphics == High && M->Web->GetGraphicsSettings() == High && M->Flow->Graphics.PresetId() == TEXT("high"), TEXT("High preset restores the complete settings after Very Low"))) return;
  if (!Check(P && P->DynamicGlobalIlluminationMethod == EDynamicGlobalIlluminationMethod::Lumen && P->ReflectionMethod == EReflectionMethod::Lumen && Cvar(TEXT("r.VolumetricFog")) == 1,
   TEXT("High restores native Lumen GI, reflections and volumetric fog"))) return;
  auto Requested = High; Requested.IndirectLighting = 0; Requested.Reflections = 2; Requested.bHardwareRayTracing = false;
  auto O = M->Flow->Settings(); O.OnGraphics(Requested);
  Stage(12); return;
 }
 if (T.Stage == 12)
 {
  const FPostProcessSettings* P = M->Web->GetGraphicsPostProcess();
  if (!Check(M->Flow->Graphics.Reflections == 2 && M->Flow->Graphics.ResolvedReflections() == 1,
   TEXT("GI off preserves requested Lumen reflections while resolving the portable screen-space fallback"))) return;
  if (!Check(P && P->ReflectionMethod == EReflectionMethod::ScreenSpace && Cvar(TEXT("r.Lumen.Reflections.Allow")) == 0,
   TEXT("unsupported GI-off software Lumen combination actually uses screen-space reflections"))) return;
  auto G = M->Flow->Graphics; G.IndirectLighting = 1;
  auto O = M->Flow->Settings(); O.OnGraphics(G);
  Stage(13); return;
 }
 if (T.Stage == 13)
 {
  const FPostProcessSettings* P = M->Web->GetGraphicsPostProcess();
  if (!Check(M->Flow->Graphics.Reflections == 2 && M->Flow->Graphics.ResolvedReflections() == 2 && P && P->ReflectionMethod == EReflectionMethod::Lumen && Cvar(TEXT("r.Lumen.Reflections.Allow")) == 1,
   TEXT("reenabling GI restores requested Lumen reflections without a sticky fallback"))) return;
  auto O = M->Flow->Settings(); O.OnGraphics(Custom(false));
  Stage(3); return;
 }
 if (T.Stage == 3)
 {
  if (!Check(M->Flow->Graphics == Custom(false) && M->Flow->Graphics.PresetId() == TEXT("custom"), TEXT("individual choices become Custom without resetting other fields")) || !VerifyRenderer(false)) return;
  UGameUserSettings* GS = GEngine ? GEngine->GetGameUserSettings() : nullptr;
  if (!Check(GS != nullptr, TEXT("native display settings object is present"))) return;
  GS->ApplySettings(false);
  auto O = M->Flow->Settings(); O.OnReapplyGraphics();
  Stage(4); return;
 }
 if (T.Stage == 4)
 {
  if (!VerifyRenderer(false)) return;
  if (!Check(M->Flow->Picture == T.Picture && FMath::IsNearlyEqual(M->Flow->Brightness, T.Brightness), TEXT("graphics and display apply preserve the selected picture look and brightness"))) return;
  if (!Check(M->Flow->OpenTestView(TEXT("settings:graphics")), TEXT("graphics page opens through the production flow"))) return;
  Stage(5); return;
 }
 if (T.Stage == 5)
 {
  const auto W = Menu();
  if (!Check(W && W->ActiveTabId() == TEXT("graphics") && W->SelectedRowLabel() == TEXT("Graphics preset"), TEXT("actual focused Slate graphics page starts on its preset row"))) return;
  S47Keys::TestPresses(TEXT("Down,Right"), .15f);
  Stage(6, .8); return;
 }
 if (T.Stage == 6)
 {
  const auto W = Menu();
  if (!Check(W && W->SelectedRowLabel() == TEXT("Render scale") && M->Flow->Graphics == Custom(true), TEXT("real Slate Down/Right input changes only render scale from 75 to 80 percent"))) return;
  Shot(TEXT("graphics-custom"));
  Stage(7, .6); return;
 }
 if (T.Stage == 7)
 {
  S47Keys::TestPresses(TEXT("Gamepad_RightShoulder,Gamepad_LeftShoulder"), .15f);
  Stage(8, .8); return;
 }
 if (T.Stage == 8)
 {
  const auto W = Menu();
  if (!Check(W && W->ActiveTabId() == TEXT("graphics") && W->SelectedRowLabel() == TEXT("Graphics preset"), TEXT("gamepad shoulder key events cycle out and back to the new graphics tab"))) return;
  FString Keys;
  for (int32 I = 0; I < 12; ++I) Keys += I ? TEXT(",Gamepad_DPad_Down") : TEXT("Gamepad_DPad_Down");
  S47Keys::TestPresses(Keys, .08f);
  Stage(9, 1.5); return;
 }
 if (T.Stage == 9)
 {
  const auto W = Menu();
  if (!Check(W && W->SelectedRowLabel() == TEXT("Restore High defaults"), TEXT("gamepad navigation reaches the final graphics row through all scroll sections"))) return;
  if (!Check(M->Flow->Graphics == Custom(true), TEXT("tab switching and scrolling preserve every custom setting"))) return;
  FString Raw; const bool bRead = FFileHelper::LoadFileToString(Raw, *StorePath());
  const auto Store = bRead ? S47J::Read(Raw) : S47J::FObj();
  if (!Check(Store.IsValid() && FS47GraphicsSettings::FromJson(S47J::Obj(Store, TEXT("graphics")), TEXT("high")) == Custom(true), TEXT("the production settings writer persisted all custom fields to disk"))) return;
  if (!Check(M->Flow->CaseId == 0, TEXT("settings tests still have no story case"))) return;
  Shot(TEXT("graphics-last-row"));
  Stage(21, 1); return;
 }
 if (T.Stage == 20)
 {
  const auto W = Menu();
  if (!Check(W && W->ActiveTabId() == TEXT("graphics") && M->Flow->Graphics.PresetId() == TEXT("custom"), TEXT("reopened settings page retains the restored Custom profile"))) return;
  Shot(TEXT("graphics-restored")); Stage(21, 1); return;
 }
 if (T.Stage == 21) Finish(true);
}

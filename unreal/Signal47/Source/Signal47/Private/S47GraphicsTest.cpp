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
#include "Engine/GameViewportClient.h"
#include "Containers/Ticker.h"
#include "Layout/Children.h"
#include "Layout/WidgetPath.h"
#include "Misc/ConfigCacheIni.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

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
 // The same visible-text/hit-path route used by S47MenuPointerTest. No settings setters.
 TSharedPtr<SWidget> DisplayText(const TSharedRef<SWidget>& W, const FString& Text, FIntPoint* Resolution = nullptr)
 {
  if (!W->GetVisibility().IsVisible()) return nullptr;
  if (W->GetType() == FName(TEXT("STextBlock")))
  {
   const FString Value = StaticCastSharedRef<STextBlock>(W)->GetText().ToString();
   FString X, Y;
   const bool Match = Resolution ? Value.Split(TEXT(" × "), &X, &Y) && X.IsNumeric() && Y.IsNumeric() : Value == Text;
   if (Match && W->GetCachedGeometry().GetLocalSize().X > 1 && W->GetCachedGeometry().GetLocalSize().Y > 1)
   {
    if (Resolution) *Resolution = FIntPoint(FCString::Atoi(*X), FCString::Atoi(*Y));
    return W;
   }
  }
  if (FChildren* Children = W->GetChildren()) for (int32 I = 0; I < Children->Num(); ++I)
   if (auto Found = DisplayText(Children->GetChildAt(I), Text, Resolution)) return Found;
  return nullptr;
 }
 TSharedPtr<SWindow> DisplayWindow() { return GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWindow() : nullptr; }
 bool DisplayHas(const FString& Text)
 {
  const auto W = DisplayWindow(); return W && DisplayText(W.ToSharedRef(), Text).IsValid();
 }
 // Read the displayed row, including the toggle's actual selected-cell styling.
 TSharedPtr<SWidget> DisplayRow(const TCHAR* Label)
 {
  const auto W=Menu(); if (!W) return nullptr;
  auto Row=DisplayText(W.ToSharedRef(),Label);
  while (Row && Row->GetType()!=FName(TEXT("SS47Row"))) Row=Row->GetParentWidget();
  return Row;
 }
 bool DisplayFrameLimit(double& Fps)
 {
  const auto Row=DisplayRow(TEXT("Frame limit")); if (!Row) return false;
  for (const TCHAR* Value : {TEXT("30 FPS"),TEXT("60 FPS"),TEXT("90 FPS"),TEXT("120 FPS"),TEXT("144 FPS"),TEXT("UNLIMITED")})
   if (DisplayText(Row.ToSharedRef(),Value)) { Fps=FString(Value)==TEXT("UNLIMITED") ? 0 : FCString::Atod(Value); return true; }
  return false;
 }
 bool DisplaySync(bool& On)
 {
  const auto Row=DisplayRow(TEXT("V-sync")); if (!Row) return false;
  const auto Yes=DisplayText(Row.ToSharedRef(),TEXT("ON")), No=DisplayText(Row.ToSharedRef(),TEXT("OFF"));
  if (!Yes || !No) return false;
  // S47Theme::Toggle renders its selected label dark against the filled amber cell.
  const auto A=StaticCastSharedPtr<STextBlock>(Yes)->GetColorAndOpacity().GetSpecifiedColor();
  const auto B=StaticCastSharedPtr<STextBlock>(No)->GetColorAndOpacity().GetSpecifiedColor();
  const float D=A.R+A.G+A.B-B.R-B.G-B.B;
  if (FMath::Abs(D)<.01f) return false;
  On=D<0; return true;
 }
 bool ClickDisplayApply()
 {
  const auto W = DisplayWindow(); if (!W || !FSlateApplication::IsInitialized()) return false;
  const auto Label = DisplayText(W.ToSharedRef(), TEXT("APPLY DISPLAY")); if (!Label) return false;
  const FGeometry& G = Label->GetCachedGeometry(); const FVector2D At = G.LocalToAbsolute(G.GetLocalSize()*.5);
  auto& App = FSlateApplication::Get(); const auto Hit = App.LocateWindowUnderMouse(At, App.GetInteractiveTopLevelWindows());
  TSharedPtr<SWidget> Button = Label;
  while (Button && Button->GetType()!=FName(TEXT("Local::SS47Click"))) Button=Button->GetParentWidget();
  UE_LOG(LogS47GraphicsTest,Display,TEXT("S47: display APPLY pointer path: hit=%s button=%s at=%.1f,%.1f"), Hit.IsValid()?*Hit.GetLastWidget()->GetTypeAsString():TEXT("none"), Button?*Button->GetTypeAsString():TEXT("none"),At.X,At.Y);
  if (!Hit.IsValid() || !Button || !Hit.ContainsWidget(Button.Get())) return false;
  const TSet<FKey> None, Left{EKeys::LeftMouseButton};
  App.ProcessMouseMoveEvent(FPointerEvent(0,At,At-FVector2D(1,0),None,EKeys::Invalid,0,App.GetModifierKeys()));
  App.ProcessMouseButtonDownEvent(nullptr,FPointerEvent(0,At,At,Left,EKeys::LeftMouseButton,0,App.GetModifierKeys()));
  FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([At](float) {
   if (!FSlateApplication::IsInitialized()) return false;
   auto& A = FSlateApplication::Get(); const TSet<FKey> None;
   A.ProcessMouseButtonUpEvent(FPointerEvent(0,At,At,None,EKeys::LeftMouseButton,0,A.GetModifierKeys())); return false;
  }), .12f);
  return true;
 }
 struct FDisplayReadback
 {
  FIntPoint Viewport = FIntPoint::ZeroValue, Client = FIntPoint::ZeroValue, Resolution = FIntPoint::ZeroValue;
  int32 WindowMode = -1, SettingsMode = -1;
  double Fps = 0, LiveFps = 0, LiveVSync = 0; bool VSync = false;
  bool Read()
  {
   const auto W = DisplayWindow(); auto* GS = GEngine ? GEngine->GetGameUserSettings() : nullptr;
   if (!W || !GS) return false;
   FVector2D V; GEngine->GameViewport->GetViewportSize(V); const FVector2D C = W->GetClientSizeInScreen();
   Viewport = FIntPoint(FMath::RoundToInt(V.X),FMath::RoundToInt(V.Y)); Client = FIntPoint(FMath::RoundToInt(C.X),FMath::RoundToInt(C.Y));
   Resolution = GS->GetScreenResolution(); WindowMode = int32(W->GetWindowMode()); SettingsMode = int32(GS->GetFullscreenMode());
   Fps = GS->GetFrameRateLimit(); VSync = GS->IsVSyncEnabled();
   LiveFps=Cvar(TEXT("t.MaxFPS")); LiveVSync=Cvar(TEXT("r.VSync")); return true;
  }
  bool Matches(FIntPoint Expected, double ExpectedFps, bool ExpectedVSync) const
  {
   return Resolution == Expected && WindowMode == int32(EWindowMode::Windowed) && SettingsMode == int32(EWindowMode::Windowed)
    && FMath::Abs(Viewport.X-Expected.X)<=2 && FMath::Abs(Viewport.Y-Expected.Y)<=2
    && FMath::Abs(Client.X-Expected.X)<=2 && FMath::Abs(Client.Y-Expected.Y)<=2
    && FMath::IsNearlyEqual(Fps,ExpectedFps,.01) && VSync == ExpectedVSync
    && FMath::IsNearlyEqual(LiveFps,ExpectedFps,.01) && LiveVSync == (ExpectedVSync ? 1. : 0.);
  }
  S47J::FObj Json() const
  {
   auto O = S47J::New(); S47J::Set(O,TEXT("viewport"),TArray<double>{double(Viewport.X),double(Viewport.Y)});
   S47J::Set(O,TEXT("windowClient"),TArray<double>{double(Client.X),double(Client.Y)});
   S47J::Set(O,TEXT("settingsResolution"),TArray<double>{double(Resolution.X),double(Resolution.Y)});
   S47J::Set(O,TEXT("windowMode"),WindowMode); S47J::Set(O,TEXT("settingsMode"),SettingsMode);
   S47J::Set(O,TEXT("fps"),Fps); S47J::Set(O,TEXT("vsync"),VSync);
   S47J::Set(O,TEXT("t.MaxFPS"),LiveFps); S47J::Set(O,TEXT("r.VSync"),LiveVSync); return O;
  }
 };
 // UE 5.8 known-config globals are cache keys, not physical filenames.
 FString DisplayIniPath() { return FConfigCacheIni::GetDestIniFilename(TEXT("GameUserSettings"), nullptr, *FPaths::GeneratedConfigDir()); }
 bool DisplayIniMatches(FIntPoint Resolution, double Fps, bool VSync)
 {
  FConfigFile Ini; Ini.Read(DisplayIniPath()); const TCHAR* Section=TEXT("/Script/Engine.GameUserSettings");
  int32 X=-1,Y=-1,Mode=-1; float SavedFps=-1; bool SavedVSync=!VSync;
  return Ini.GetInt(Section,TEXT("ResolutionSizeX"),X) && Ini.GetInt(Section,TEXT("ResolutionSizeY"),Y)
   && Ini.GetInt(Section,TEXT("FullscreenMode"),Mode) && Ini.GetFloat(Section,TEXT("FrameRateLimit"),SavedFps)
   && Ini.GetBool(Section,TEXT("bUseVSync"),SavedVSync) && FIntPoint(X,Y)==Resolution
   && Mode==int32(EWindowMode::Windowed) && FMath::IsNearlyEqual(double(SavedFps),Fps,.01) && SavedVSync==VSync;
 }

}

struct FS47GraphicsTest::FState
{
 FString Mode, Root, Picture;
 int32 Stage = 0, Checks = 0;
 bool bDone = false;
 double Elapsed = 0, At = 0, Hold = 3, EffectsGroup = 0, MaterialQuality = 0;
 float Brightness = 0;
 FIntPoint DisplayInitial = FIntPoint::ZeroValue, DisplayKept = FIntPoint::ZeroValue, DisplayRequested = FIntPoint::ZeroValue;
 FDisplayReadback DisplayBefore;
 double DisplayFps = 0; bool DisplayVSync = false; int32 DisplayMoves = 0;
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
 if (State->Mode.StartsWith(TEXT("display-"))) { FDisplayReadback D; if (D.Read()) S47J::Set(R,TEXT("display"),D.Json()); }
 S47J::Set(R, TEXT("limits"), TEXT("Isolated production settings callbacks, actual Slate key events, native renderer readbacks and a separate-process settings reload. No personal profile, story progression, hardware gamepad, human input or cross-hardware FPS guarantee."));
 if (State->Mode.StartsWith(TEXT("display-"))) S47J::Set(R,TEXT("limits"),TEXT("Isolated title profile; actual Slate mouse/key/gamepad-key routing, native window/client/viewport and GameUserSettings readback, ten-second UI timeout, and separate-process restart. Windowed resolutions only; no exclusive fullscreen, physical controller transport, monitor mode matrix or crash-during-confirmation claim."));
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
 if (T.Mode == TEXT("display-write") || T.Mode == TEXT("display-read")) { StepDisplay(); return; }
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
  // (thirteen rows below the preset: twelve controls, Vehicle mirrors, then Restore)
  for (int32 I = 0; I < 13; ++I) Keys += I ? TEXT(",Gamepad_DPad_Down") : TEXT("Gamepad_DPad_Down");
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


void FS47GraphicsTest::StepDisplay()
{
 auto& T = *State;
 const FString Receipt = FPaths::ProjectSavedDir()/TEXT("GraphicsContract/display-expected.json");
 auto Preserved = [&] {
  return Check(M->Flow->Graphics == Custom(true) && M->Flow->Picture == TEXT("off") && FMath::IsNearlyEqual(M->Flow->Brightness,.65f),
   TEXT("display apply/rollback retains custom graphics, picture and brightness")) && VerifyRenderer(false);
 };
 auto Actual = [&](FIntPoint Expected, const TCHAR* Why) {
  FDisplayReadback D; const bool Match=D.Read() && D.Matches(Expected,T.DisplayFps,T.DisplayVSync);
  if (!Match && T.Elapsed-T.At<5) return false; // let the native resize finish, not just GS's requested fields
  auto E=D.Json(); S47J::Set(E,TEXT("phase"),Why); S47J::Set(E,TEXT("expected"),TArray<double>{double(Expected.X),double(Expected.Y)});
  T.Evidence.Add(MakeShared<FJsonValueObject>(E)); return Check(Match,Why);
 };
 auto Pending = [&](FIntPoint Before) {
  const auto W=Menu(); const auto Window=DisplayWindow(); FIntPoint Requested;
  if (!Check(W && W->ActiveTabId()==TEXT("picture") && W->SelectedRowLabel()==TEXT("Resolution") && Window
    && DisplayText(Window.ToSharedRef(),FString(),&Requested).IsValid() && Requested.X>0 && Requested.Y>0 && Requested!=Before,
    TEXT("actual Resolution row offers a different supported windowed size"))) return false;
  T.DisplayRequested=Requested;
  FDisplayReadback D;
  const bool Unchanged = T.Stage==2 ? D.Read() && D.Viewport==T.DisplayBefore.Viewport && D.Client==T.DisplayBefore.Client
   && D.Resolution==T.DisplayBefore.Resolution && D.WindowMode==T.DisplayBefore.WindowMode && D.SettingsMode==T.DisplayBefore.SettingsMode
   && FMath::IsNearlyEqual(D.Fps,T.DisplayBefore.Fps,.01) && D.VSync==T.DisplayBefore.VSync
   && FMath::IsNearlyEqual(D.LiveFps,T.DisplayBefore.LiveFps,.01) && D.LiveVSync==T.DisplayBefore.LiveVSync
   : Actual(Before,TEXT("second pending choice leaves the kept native display unchanged"));
  // Startup -ResX/-windowed may differ from GS until the first real APPLY; pending input must preserve both.
  return Check(Unchanged,TEXT("pending resolution/frame limit/V-sync choices leave actual window, GS and live CVars untouched"))
   && Check(DisplayHas(TEXT("Display changes are waiting. Choose APPLY DISPLAY below.")),TEXT("pending display status explicitly asks for APPLY DISPLAY"));
 };
 if (T.Stage==0)
 {
  FString User, Override; FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
  User=FPaths::ConvertRelativePathToFull(User); FPaths::CollapseRelativeDirectories(User);
  const bool Read=T.Mode==TEXT("display-read");
  if (!Check((User.StartsWith(TEXT("/tmp/")) || User.StartsWith(TEXT("/var/tmp/")))
   && FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),User)
   && FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(DisplayIniPath()),User)
   && FParse::Param(FCommandLine::Get(),TEXT("S47Title")) && !FParse::Param(FCommandLine::Get(),TEXT("S47SmokeTest"))
   && !FParse::Value(FCommandLine::Get(),TEXT("S47Quality="),Override) && !FParse::Value(FCommandLine::Get(),TEXT("S47Picture="),Override)
   && !FParse::Value(FCommandLine::Get(),TEXT("ExecCmds="),Override)
   && (!Read || (!FParse::Param(FCommandLine::Get(),TEXT("windowed")) && !FParse::Param(FCommandLine::Get(),TEXT("fullscreen"))
     && !FParse::Value(FCommandLine::Get(),TEXT("ResX="),Override) && !FParse::Value(FCommandLine::Get(),TEXT("ResY="),Override))),
   TEXT("display contract uses isolated title settings; restart has no display/graphics overrides"))) return;
  T.Root=FPaths::ProjectSavedDir()/TEXT("GraphicsContract")/T.Mode; IFileManager::Get().MakeDirectory(*T.Root,true);
  if (!Check(M->Mode==US47Main::EMode::Title && M->Flow->CaseId==0,TEXT("display contract stays outside personal story cases"))) return;
  T.EffectsGroup=Cvar(TEXT("sg.EffectsQuality")); T.MaterialQuality=Cvar(TEXT("r.MaterialQualityLevel"));
  if (Read)
  {
   FString Raw; const bool Loaded=FFileHelper::LoadFileToString(Raw,*Receipt); const auto Expected=Loaded?S47J::Read(Raw):S47J::FObj();
   if (!Check(Expected.IsValid() && S47J::Bool(Expected,TEXT("verified")),TEXT("fresh process loads the completed write-phase receipt"))) return;
   T.DisplayKept=FIntPoint(int32(S47J::Num(Expected,TEXT("x"))),int32(S47J::Num(Expected,TEXT("y"))));
   T.DisplayFps=S47J::Num(Expected,TEXT("fps")); T.DisplayVSync=S47J::Bool(Expected,TEXT("vsync"));
   Stage(20,.5); return;
  }
  FDisplayReadback D;
  if (!Check(D.Read() && D.WindowMode==int32(EWindowMode::Windowed) && D.Viewport.X>=800 && D.Viewport.Y>=600,
    TEXT("write fixture begins in a usable actual windowed viewport"))) return;
  T.DisplayBefore=D; T.DisplayInitial=D.Viewport; T.DisplayFps=D.Fps; T.DisplayVSync=D.VSync;
  auto O=M->Flow->Settings();
  if (!Check(bool(O.OnGraphics)&&bool(O.OnPicture)&&bool(O.OnBrightness)&&bool(O.OnReapplyGraphics),TEXT("production immediate-setting callbacks exist"))) return;
  O.OnGraphics(Custom(true)); O.OnPicture(TEXT("off")); O.OnBrightness(.65f);
  if (!Check(M->Flow->OpenTestView(TEXT("settings:picture")),TEXT("production flow opens the picture page"))) return;
  Stage(1,1); return;
 }
 if (T.Stage==1)
 {
  const auto W=Menu();
  if (!Check(W && W->SelectedRowLabel()==TEXT("Display mode"),TEXT("actual Slate focus starts on the display mode row"))) return;
  // Verify the visible choice instead of inferring Shown from command-line or GS state.
  // ReadDisplay normally reconciles it to the real window; retain a bounded UI route if not.
  if (!DisplayText(W.ToSharedRef(),TEXT("WINDOWED")).IsValid())
  {
   if (!Check(++T.DisplayMoves<=3,TEXT("bounded Slate mode selection reaches WINDOWED"))) return;
   S47Keys::TestPresses(TEXT("Right"),.05f); Stage(1,.3); return;
  }
  if (!Check(DisplayText(W.ToSharedRef(),TEXT("WINDOWED")).IsValid(),TEXT("visible Display mode is WINDOWED before choosing a resolution")) || !Preserved()) return;
  Shot(TEXT("01-before")); T.DisplayMoves=0; Stage(40,.3); return;
 }
 if (T.Stage==40)
 {
  const auto W=Menu(); if (!Check(W.IsValid(),TEXT("frame-limit navigation keeps actual Slate focus"))) return;
  if (W->SelectedRowLabel()!=TEXT("Frame limit"))
  {
   if (!Check(++T.DisplayMoves<=4,TEXT("bounded Slate Down reaches Frame limit"))) return;
   S47Keys::TestPresses(TEXT("Down"),.05f); Stage(40,.2); return;
  }
  S47Keys::TestPresses(TEXT("Right"),.05f); Stage(41,.3); return;
 }
 if (T.Stage==41)
 {
  const auto W=Menu();
  if (!Check(W && W->SelectedRowLabel()==TEXT("Frame limit") && DisplayFrameLimit(T.DisplayFps)
   && !FMath::IsNearlyEqual(T.DisplayFps,T.DisplayBefore.Fps,.01),TEXT("Slate changes frame limit to a different visibly offered value"))) return;
  S47Keys::TestPresses(TEXT("Down,Enter"),.15f); Stage(42,.5); return;
 }
 if (T.Stage==42)
 {
  const auto W=Menu();
  if (!Check(W && W->SelectedRowLabel()==TEXT("V-sync") && DisplaySync(T.DisplayVSync)
   && T.DisplayVSync!=T.DisplayBefore.VSync,TEXT("Slate toggles V-sync and its visible selected cell supplies the expected value"))) return;
  T.DisplayMoves=0; Stage(43,.1); return;
 }
 if (T.Stage==43)
 {
  const auto W=Menu(); if (!Check(W.IsValid(),TEXT("return navigation keeps actual Slate focus"))) return;
  if (W->SelectedRowLabel()!=TEXT("Resolution"))
  {
   if (!Check(++T.DisplayMoves<=4,TEXT("bounded Slate Up returns to Resolution"))) return;
   S47Keys::TestPresses(TEXT("Up"),.05f); Stage(43,.2); return;
  }
  S47Keys::TestPresses(TEXT("Right"),.05f); Stage(2,.4); return;
 }
 if (T.Stage==2)
 {
  if (!Pending(T.DisplayInitial)) return;
  Shot(TEXT("02-pending")); Stage(30,.3); return;
 }
 if (T.Stage==30)
 {
  if (!Check(ClickDisplayApply(),TEXT("mouse down/up targets the real visible APPLY DISPLAY hit path"))) return;
  Stage(3,.6); return;
 }
 if (T.Stage==3)
 {
  if (!Actual(T.DisplayRequested,TEXT("APPLY changes the settled viewport, window client and GS size"))) return;
  if (!Check(DisplayHas(TEXT("Keep this display?")),TEXT("resolution change presents the actual confirmation overlay")) || !Preserved()) return;
  Shot(TEXT("03-confirm")); T.DisplayKept=T.DisplayRequested;
  S47Keys::TestPresses(TEXT("Gamepad_FaceButton_Bottom"),.15f); Stage(4,.5); return;
 }
 if (T.Stage==4)
 {
  if (!Check(!DisplayHas(TEXT("Keep this display?")) && DisplayHas(TEXT("Display settings applied.")),TEXT("gamepad A routed through Slate keeps the display and clears confirmation"))) return;
  if (!Actual(T.DisplayKept,TEXT("kept display remains the actual native window"))) return;
  Shot(TEXT("04-kept")); Stage(31,.3); return;
 }
 if (T.Stage==31)
 {
  M->UI->Close(true);
  if (!Check(M->Flow->OpenTestView(TEXT("settings:picture")),TEXT("settings reopen after Keep"))) return;
  Stage(5,.6); return;
 }
 if (T.Stage==5)
 {
  if (!Actual(T.DisplayKept,TEXT("reopened settings do not revert the kept display"))) return;
  S47Keys::TestPresses(TEXT("Down,Right"),.15f); Stage(6,.6); return;
 }
 if (T.Stage==6)
 {
  if (!Pending(T.DisplayKept)) return;
  T.DisplayMoves=0; Stage(7,.1); return;
 }
 if (T.Stage==7)
 {
  const auto W=Menu(); if (!Check(W.IsValid(),TEXT("keyboard traversal retains the focused settings widget"))) return;
  if (!W->SelectedRowLabel().IsEmpty())
  {
   if (!Check(++T.DisplayMoves<=16,TEXT("bounded keyboard traversal reaches the display buttons"))) return;
   S47Keys::TestPresses(TEXT("Down"),.05f); Stage(7,.15); return;
  }
  S47Keys::TestPresses(TEXT("Enter"),.1f); Stage(8,.5); return;
 }
 if (T.Stage==8)
 {
  if (!Actual(T.DisplayRequested,TEXT("keyboard APPLY reaches the second native resolution"))) return;
  if (!Check(DisplayHas(TEXT("Keep this display?")),TEXT("second change awaits confirmation before real timeout"))) return;
  Shot(TEXT("05-timeout-pending")); Stage(9,10.5); return; // ordinary widget Tick, never call Revert or alter its clock
 }
 if (T.Stage==9)
 {
  if (!Actual(T.DisplayKept,TEXT("ten-second UI timeout restores the previous actual native display"))) return;
  if (!Check(!DisplayHas(TEXT("Keep this display?")) && DisplayHas(TEXT("The display is back as it was.")),TEXT("timeout removes confirmation and reports rollback")) || !Preserved()) return;
  if (!Check(DisplayIniMatches(T.DisplayKept,T.DisplayFps,T.DisplayVSync),TEXT("rollback writes the kept resolution/mode/frame limit/vsync back to GameUserSettings.ini"))) return;
  Shot(TEXT("06-rolled-back")); Stage(32,.3); return;
 }
 if (T.Stage==32)
 {
  M->UI->Close(true);
  if (!Check(M->Flow->OpenTestView(TEXT("settings:picture")),TEXT("settings reopen after actual timeout rollback"))) return;
  Stage(10,.6); return;
 }
 if (T.Stage==10)
 {
  if (!Actual(T.DisplayKept,TEXT("reopening after rollback preserves the kept native display"))) return;
  if (!Check(DisplayHas(TEXT("Display settings are up to date. Other changes take effect immediately and are saved automatically.")),TEXT("reopened page clearly distinguishes applied display from automatic settings"))) return;
  auto O=S47J::New(); S47J::Set(O,TEXT("verified"),true); S47J::Set(O,TEXT("x"),T.DisplayKept.X); S47J::Set(O,TEXT("y"),T.DisplayKept.Y);
  S47J::Set(O,TEXT("fps"),T.DisplayFps); S47J::Set(O,TEXT("vsync"),T.DisplayVSync);
  if (!Check(FFileHelper::SaveStringToFile(S47J::Write(O,true),*Receipt),TEXT("verified native display receipt saved for the next process"))) return;
  Shot(TEXT("07-reopened")); Stage(21,.5); return;
 }
 if (T.Stage==20)
 {
  if (!Actual(T.DisplayKept,TEXT("fresh process restores the confirmed native window without resolution flags")) || !Preserved()) return;
  if (!Check(DisplayIniMatches(T.DisplayKept,T.DisplayFps,T.DisplayVSync),TEXT("fresh GameUserSettings.ini matches actual startup window and display controls"))) return;
  FString Raw; const auto Disk=FFileHelper::LoadFileToString(Raw,*StorePath())?S47J::Read(Raw):S47J::FObj();
  if (!Check(Disk.IsValid() && FS47GraphicsSettings::FromJson(S47J::Obj(Disk,TEXT("graphics")),TEXT("high"))==Custom(true)
   && S47J::Str(Disk,TEXT("picture"))==TEXT("off") && FMath::IsNearlyEqual(S47J::Num(Disk,TEXT("brightness")),.65,.0001),
   TEXT("fresh process retains independently persisted custom graphics, picture and brightness"))) return;
  if (!Check(M->Flow->OpenTestView(TEXT("settings:picture")),TEXT("fresh-process settings opens through production flow"))) return;
  Stage(22,.6); return;
 }
 if (T.Stage==22)
 {
  if (!Actual(T.DisplayKept,TEXT("opening fresh-process settings leaves restored display unchanged"))) return;
  if (!Check(DisplayHas(TEXT("Display settings are up to date. Other changes take effect immediately and are saved automatically.")),TEXT("fresh-process page reports an applied display"))) return;
  Shot(TEXT("08-restarted")); Stage(21,.5); return;
 }
 if (T.Stage==21) Finish(true,TEXT("native display apply/keep/timeout/reopen contract completed"));
}

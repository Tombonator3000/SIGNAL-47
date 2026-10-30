#include "S47FinaleMovie.h"

#include "S47Chapter6.h"
#include "S47Flow.h"
#include "S47Json.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Walker.h"
#include "S47World.h"
#include "SS47Ending.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47FinaleMovie, Log, All);

void FS47FinaleMovie::Step(US47Main* M, double Dt)
{
	FString Output;
	if (!M || !FParse::Value(FCommandLine::Get(), TEXT("S47FinaleMovie="), Output)) return;
	struct FState
	{
		bool bInit = false, bOwnsOutput = false, bStarted = false, bDone = false, bEarly = false;
		bool bSawEvent = false, bSawPullout = false, bSawEnd = false, bSawPhoto = false, bSavedPhoto = false;
		double WallStart = FPlatformTime::Seconds(), SimTotal = 0, SimStart = 0, CaptureWallStart = 0;
		double NextFrame = 0, RequestWall = 0, EndAt = -1, PhotoAt = -1, PhotoCardAt = -1;
		double TargetFps = 15, Tail = 8, EarlyTail = 8;
		FString Dir, Pending, LastStage;
		S47J::FObj PendingRow;
		TArray<TSharedPtr<FJsonValue>> Frames;
	};
	static TMap<TWeakObjectPtr<US47Main>, FState> States;
	for (auto It = States.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
	auto& S = States.FindOrAdd(M);
	if (S.bDone) return;
	S.SimTotal += FMath::Max(0., Dt);
	const double WallNow = FPlatformTime::Seconds();
	const double SimNow = S.bStarted ? M->T - S.SimStart : 0;
	auto& Files = IFileManager::Get();
	const auto Finish = [&](bool bComplete, const FString& Reason)
	{
		S.bDone = true;
		if (S.bOwnsOutput)
		{
			auto Manifest = S47J::New();
			S47J::Set(Manifest, TEXT("schema"), 1);
			S47J::Set(Manifest, TEXT("kind"), TEXT("developer-jump-gameplay-visual-capture"));
			S47J::Set(Manifest, TEXT("source"), TEXT("chapter6:line developer jump; subsequent gameplay timing and camera remain untouched"));
			S47J::Set(Manifest, TEXT("complete"), bComplete);
			S47J::Set(Manifest, TEXT("reason"), Reason);
			S47J::Set(Manifest, TEXT("fullEnding"), bComplete && !S.bEarly && S.bSawEvent && S.bSawPullout && S.bSawEnd && S.bSavedPhoto);
			S47J::Set(Manifest, TEXT("earlyEnd"), S.bEarly);
			S47J::Set(Manifest, TEXT("photoObserved"), S.bSawPhoto);
			S47J::Set(Manifest, TEXT("photoFrameSaved"), S.bSavedPhoto);
			S47J::Set(Manifest, TEXT("photoFirstSimulationSeconds"), S.PhotoAt);
			S47J::Set(Manifest, TEXT("endSimulationSeconds"), SimNow);
			S47J::Set(Manifest, TEXT("targetFps"), S.TargetFps);
			S47J::Set(Manifest, TEXT("showUI"), true);
			S47J::Set(Manifest, TEXT("eventSeconds"), 47.);
			S47J::Set(Manifest, TEXT("pulloutSeconds"), 34.);
			S47J::Set(Manifest, TEXT("timing"), TEXT("simulationSeconds records Main's simulation time at each screenshot request. Use adjacent timestamps for variable-frame-rate playback; wallSeconds also records the request time. Ending cards retain their ordinary Slate clock."));
			S47J::Set(Manifest, TEXT("limits"), TEXT("Visual motion capture only; screenshot cost changes frame pacing and supplies no profiling evidence. Audio is not recorded."));
			Manifest->SetArrayField(TEXT("frames"), S.Frames);
			if (!FFileHelper::SaveStringToFile(S47J::Write(Manifest, true), *(S.Dir / TEXT("frames.json")),
				FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &Files, FILEWRITE_NoReplaceExisting))
			{
				UE_LOG(LogS47FinaleMovie, Error, TEXT("S47: finale movie could not write a fresh frames.json"));
				FPlatformMisc::RequestExitWithStatus(false, 1); return;
			}
		}
		UE_LOG(LogS47FinaleMovie, Display, TEXT("S47: finale movie %s: %s, %d frames, %.3f simulation seconds"),
			bComplete ? TEXT("COMPLETE") : TEXT("STOPPED"), *Reason, S.Frames.Num(), SimNow);
		FPlatformMisc::RequestExitWithStatus(false, bComplete ? 0 : 1);
	};
	if (!S.bInit)
	{
		S.bInit = true;
		FString UserDir, Jump, Other;
		double AutoSpeed = 0;
		const bool bExplicitUserDir = FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir);
		if (!bExplicitUserDir || FPaths::IsRelative(UserDir)) { Finish(false, TEXT("explicit isolated /tmp UserDir required")); return; }
		FPaths::NormalizeDirectoryName(UserDir); FPaths::CollapseRelativeDirectories(UserDir);
		FString SavedDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
		FPaths::NormalizeDirectoryName(SavedDir); FPaths::CollapseRelativeDirectories(SavedDir);
		if (!UserDir.StartsWith(TEXT("/tmp/")) || !FPaths::IsUnderDirectory(SavedDir, UserDir))
		{ Finish(false, TEXT("Saved must remain inside the explicit /tmp UserDir")); return; }
		FParse::Value(FCommandLine::Get(), TEXT("S47Jump="), Jump);
		FParse::Value(FCommandLine::Get(), TEXT("S47Auto="), AutoSpeed);
		if (Jump != TEXT("chapter6:line") || !FMath::IsNearlyEqual(AutoSpeed, 24.6, .001)
			|| FParse::Value(FCommandLine::Get(), TEXT("S47FinalePreview="), Other)
			|| FParse::Value(FCommandLine::Get(), TEXT("S47FinaleSweep="), Other))
		{ Finish(false, TEXT("requires ordinary chapter6:line jump and S47Auto=24.6, without a frame fixture")); return; }
		if (Output.IsEmpty() || FPaths::IsRelative(Output)) { Finish(false, TEXT("absolute output directory required")); return; }
		S.Dir = Output; FPaths::NormalizeDirectoryName(S.Dir); FPaths::CollapseRelativeDirectories(S.Dir);
		if (Files.DirectoryExists(*S.Dir) || Files.FileExists(*S.Dir))
		{ Finish(false, TEXT("refusing an existing output path")); return; }
		if (!Files.MakeDirectory(*S.Dir, true)) { Finish(false, TEXT("cannot create output directory")); return; }
		S.bOwnsOutput = true;
		FParse::Value(FCommandLine::Get(), TEXT("S47FinaleMovieFps="), S.TargetFps);
		FParse::Value(FCommandLine::Get(), TEXT("S47FinaleMovieTail="), S.Tail);
		S.bEarly = FParse::Value(FCommandLine::Get(), TEXT("S47FinaleMovieEarlyEnd="), S.EarlyTail);
		S.TargetFps = FMath::Clamp(S.TargetFps, 12., 15.); S.Tail = FMath::Clamp(S.Tail, .1, 12.);
		S.EarlyTail = FMath::Clamp(S.EarlyTail, 0., 30.);
	}
	if (WallNow - S.WallStart >= 600 || S.SimTotal >= 260)
	{ Finish(false, TEXT("capture timeout (260 simulation / 600 wall seconds)")); return; }
	auto* C = M->Story ? Cast<US47Chapter6>(M->Story->Chapter(TEXT("ch6"))) : nullptr;
	if (!S.bStarted)
	{
		if (!C || !C->bActive || M->Mode != US47Main::EMode::Play || !M->World || M->World->Area != TEXT("roswell")) return;
		if (!M->Flow || !M->Flow->bDevRun || M->Flow->CaseId != 0)
		{ Finish(false, TEXT("capture requires an isolated developer case")); return; }
		S.bStarted = true; S.SimStart = M->T; S.CaptureWallStart = WallNow;
		UE_LOG(LogS47FinaleMovie, Display, TEXT("S47: finale movie observes normal gameplay at %.1f target fps, UI included; %s"), S.TargetFps, *S.Dir);
	}
	if (!C) { Finish(false, TEXT("chapter six disappeared")); return; }
	const FString Stage = C->Stage;
	S.bSawEvent |= Stage == TEXT("event"); S.bSawPullout |= Stage == TEXT("pullout");
	if (Stage == TEXT("end") || M->Mode == US47Main::EMode::End)
	{
		S.bSawEnd = true;
		if (S.EndAt < 0) S.EndAt = SimNow;
	}
	FString EndingPhase = TEXT("none");
	double CardSeconds = 0, PhotoAlpha = 0;
	bool bPhotoVisible = false;
	const auto End = C->Ending;
	if (End && !End->Disposed())
	{
		CardSeconds = End->T;
		switch (End->Card)
		{
		case SS47Ending::ECard::Title: EndingPhase = TEXT("title"); break;
		case SS47Ending::ECard::Credits: EndingPhase = TEXT("credits"); break;
		case SS47Ending::ECard::Photo:
			EndingPhase = TEXT("photo"); PhotoAlpha = End->CardAlpha();
			bPhotoVisible = PhotoAlpha > .01 && End->Photograph.Get() != nullptr; break;
		default: break;
		}
	}
	if (bPhotoVisible && !S.bSawPhoto)
	{
		S.bSawPhoto = true; S.PhotoAt = SimNow; S.PhotoCardAt = CardSeconds;
		UE_LOG(LogS47FinaleMovie, Display, TEXT("S47: finale movie observed original photograph at simulation %.3f, alpha %.3f"), SimNow, PhotoAlpha);
	}
	if (Stage != S.LastStage)
	{
		UE_LOG(LogS47FinaleMovie, Display, TEXT("S47: finale movie stage=%s simulation=%.3f"), *Stage, SimNow);
		S.LastStage = Stage;
	}
	if (!S.Pending.IsEmpty())
	{
		if (!FScreenshotRequest::IsScreenshotRequested() && Files.FileSize(*S.Pending) > 0)
		{
			S.bSavedPhoto |= S47J::Bool(S.PendingRow, TEXT("photoVisible"));
			S.Frames.Add(MakeShared<FJsonValueObject>(S.PendingRow)); S.Pending.Reset(); S.PendingRow.Reset();
		}
		else if (WallNow - S.RequestWall > 30) { Finish(false, TEXT("screenshot request did not produce a file")); return; }
		else return;
	}
	const bool bTailDone = S.bEarly ? S.EndAt >= 0 && SimNow - S.EndAt >= S.EarlyTail
		: S.bSavedPhoto && EndingPhase == TEXT("photo") && CardSeconds - S.PhotoCardAt >= S.Tail;
	if (bTailDone)
	{ Finish(true, S.bEarly ? TEXT("early stop after pullout; full ending not captured") : TEXT("original photograph visible for requested tail")); return; }
	if (SimNow + 1.e-6 < S.NextFrame || FScreenshotRequest::IsScreenshotRequested()) return;
	const FString Name = FString::Printf(TEXT("frame-%06d.png"), S.Frames.Num());
	S.Pending = S.Dir / Name;
	if (Files.FileExists(*S.Pending)) { S.Pending.Reset(); Finish(false, TEXT("refusing a previous frame overwrite")); return; }
	S.PendingRow = S47J::New();
	S47J::Set(S.PendingRow, TEXT("file"), Name); S47J::Set(S.PendingRow, TEXT("simulationSeconds"), SimNow);
	S47J::Set(S.PendingRow, TEXT("wallSeconds"), WallNow - S.CaptureWallStart);
	S47J::Set(S.PendingRow, TEXT("stage"), Stage);
	S47J::Set(S.PendingRow, TEXT("clockSeconds"), M->Story ? M->Story->Clock() : 0.);
	S47J::Set(S.PendingRow, TEXT("eventSeconds"), C->Et); S47J::Set(S.PendingRow, TEXT("pulloutSeconds"), C->Pt);
	S47J::Set(S.PendingRow, TEXT("endingPhase"), EndingPhase); S47J::Set(S.PendingRow, TEXT("endingCardSeconds"), CardSeconds);
	S47J::Set(S.PendingRow, TEXT("photoAlpha"), PhotoAlpha); S47J::Set(S.PendingRow, TEXT("photoVisible"), bPhotoVisible);
	if (M->Player)
	{
		const FVector Eye = M->Player->EyeUE();
		const bool bFinite = !Eye.ContainsNaN();
		S47J::Set(S.PendingRow, TEXT("eyeFinite"), bFinite);
		if (bFinite) S47J::Set(S.PendingRow, TEXT("eyeUE"), TArray<double>{ Eye.X, Eye.Y, Eye.Z });
		else S47J::SetNull(S.PendingRow, TEXT("eyeUE"));
	}
	S.RequestWall = WallNow;
	// Keep cadence on the simulation grid; a slow PNG skips slots instead of queuing work.
	S.NextFrame = (FMath::FloorToDouble(SimNow * S.TargetFps) + 1) / S.TargetFps;
	// The filename is unique, UI is retained, and the next request waits for this PNG.
	FScreenshotRequest::RequestScreenshot(S.Pending, true, false, false, FIntRect(), true);
}

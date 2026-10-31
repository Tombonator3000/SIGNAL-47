#include "S47FinaleMovie.h"

#include "S47Chapter6.h"
#include "S47Flow.h"
#include "S47Json.h"
#include "S47Main.h"
#include "S47OldRoad.h"
#include "S47Story.h"
#include "S47Walker.h"
#include "S47World.h"
#include "SS47Ending.h"
#include "AudioDeviceManager.h"
#include "AudioMixerDevice.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Sound/SampleBufferIO.h"
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
		bool bAudioRequested = false, bAudioStarted = false;
		bool bWaitForMile = false;
		double WallStart = FPlatformTime::Seconds(), SimTotal = 0, SimStart = 0, CaptureWallStart = 0;
		double NextFrame = 0, RequestWall = 0, EndAt = -1, PhotoAt = -1, PhotoCardAt = -1;
		double TargetFps = 15, Tail = 8, EarlyTail = 8;
		double StartMile = 5.9;
		TOptional<double> CaptureStartMile;
		double AudioStartRequestOffset = -1;
		Audio::FMixerDevice* AudioMixer = nullptr;
		FString Dir, Pending, LastStage, AudioError, Jump;
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
		FString FinalReason = Reason, AudioFile, AudioError = S.AudioError;
		bool bAudioSaved = false;
		int32 AudioChannels = 0, AudioSampleRate = 0;
		double AudioDuration = 0;
		if (S.bAudioRequested)
		{
			if (S.bAudioStarted)
			{
				S.bAudioStarted = false;
				auto* Mixer = FAudioDeviceManager::GetAudioMixerDeviceFromWorldContext(M->Player.Get());
				if (!Mixer || Mixer != S.AudioMixer) AudioError = TEXT("master audio device disappeared or changed");
				else
				{
					float Channels = 0, SampleRate = 0;
					const auto& Recorded = Mixer->StopRecording(nullptr, Channels, SampleRate);
					if (Recorded.Num() == 0 || !FMath::IsFinite(Channels) || !FMath::IsFinite(SampleRate)
						|| Channels < 1 || SampleRate < 1)
						AudioError = TEXT("master recording produced no valid audio samples");
					else
					{
						AudioChannels = FMath::RoundToInt(Channels); AudioSampleRate = FMath::RoundToInt(SampleRate);
						AudioDuration = double(Recorded.Num()) / AudioChannels / AudioSampleRate;
						const FString WavPath = S.Dir / TEXT("master-mix.wav");
						if (Files.FileExists(*WavPath)) AudioError = TEXT("refusing an existing master-mix.wav");
						else
						{
							const Audio::TSampleBuffer<int16> Buffer(Recorded, AudioChannels, AudioSampleRate);
							Audio::FSoundWavePCMWriter Writer;
							FString WrittenPath;
							// Finish disk I/O before manifest/Exit; a WAV export creates no sound asset.
							const bool bWriteReturned = Writer.SynchronouslyWriteToWavFile(Buffer, TEXT("master-mix"), S.Dir, &WrittenPath);
							Audio::ESoundWavePCMWriterState WriteState = Audio::ESoundWavePCMWriterState::Idle;
							Writer.CheckStatus(&WriteState);
							bAudioSaved = bWriteReturned && Writer.IsDone()
								&& WriteState == Audio::ESoundWavePCMWriterState::Suceeded
								&& Files.FileSize(*WavPath) > int64(Buffer.GetNumSamples()) * int64(sizeof(int16));
							if (bAudioSaved)
							{
								AudioFile = FPaths::GetCleanFilename(WrittenPath);
								UE_LOG(LogS47FinaleMovie, Display, TEXT("S47: finale movie saved %s: %d channels, %d Hz, %.3f audio seconds"),
									*AudioFile, AudioChannels, AudioSampleRate, AudioDuration);
							}
							else AudioError = TEXT("master mix WAV write did not complete successfully");
						}
					}
				}
			}
			else if (AudioError.IsEmpty()) AudioError = TEXT("master recording was not started");
			if (!bAudioSaved)
			{
				bComplete = false;
				FinalReason += TEXT("; requested audio incomplete: ") + AudioError;
				UE_LOG(LogS47FinaleMovie, Error, TEXT("S47: finale movie audio incomplete: %s; PNG files retained"), *AudioError);
			}
		}
		if (S.bOwnsOutput)
		{
			auto Manifest = S47J::New();
			S47J::Set(Manifest, TEXT("schema"), 1);
			S47J::Set(Manifest, TEXT("kind"), S.bAudioRequested ? TEXT("developer-jump-gameplay-audiovisual-capture") : TEXT("developer-jump-gameplay-visual-capture"));
			const FString StartedAt = S.CaptureStartMile.IsSet() ? FString::Printf(TEXT("%.3f"), *S.CaptureStartMile) : TEXT("not started");
			const FString RequestedAt = S.bWaitForMile ? FString::Printf(TEXT(" (requested >= %.3f)"), S.StartMile) : FString();
			S47J::Set(Manifest, TEXT("source"), FString::Printf(TEXT("%s developer jump; capture start mile %s%s; subsequent gameplay timing and camera remain untouched"),
				*S.Jump, *StartedAt, *RequestedAt));
			S47J::Set(Manifest, TEXT("jump"), S.Jump);
			if (S.bWaitForMile) S47J::Set(Manifest, TEXT("startMileThreshold"), S.StartMile);
			else S47J::SetNull(Manifest, TEXT("startMileThreshold"));
			if (S.CaptureStartMile.IsSet()) S47J::Set(Manifest, TEXT("captureStartMile"), *S.CaptureStartMile);
			else S47J::SetNull(Manifest, TEXT("captureStartMile"));
			S47J::Set(Manifest, TEXT("complete"), bComplete);
			S47J::Set(Manifest, TEXT("reason"), FinalReason);
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
			S47J::Set(Manifest, TEXT("audioRequested"), S.bAudioRequested);
			if (S.bAudioRequested)
			{
				S47J::Set(Manifest, TEXT("audioComplete"), bAudioSaved);
				S47J::SetOrNull(Manifest, TEXT("audioFile"), AudioFile);
				S47J::SetOrNull(Manifest, TEXT("audioError"), AudioError);
				if (AudioChannels > 0)
				{
					S47J::Set(Manifest, TEXT("audioChannels"), AudioChannels);
					S47J::Set(Manifest, TEXT("audioSampleRate"), AudioSampleRate);
					S47J::Set(Manifest, TEXT("audioDurationSeconds"), AudioDuration);
				}
				else
				{
					S47J::SetNull(Manifest, TEXT("audioChannels"));
					S47J::SetNull(Manifest, TEXT("audioSampleRate"));
					S47J::SetNull(Manifest, TEXT("audioDurationSeconds"));
				}
				if (S.AudioStartRequestOffset >= 0) S47J::Set(Manifest, TEXT("audioStartRequestOffsetSeconds"), S.AudioStartRequestOffset);
				else S47J::SetNull(Manifest, TEXT("audioStartRequestOffsetSeconds"));
				S47J::Set(Manifest, TEXT("audioPlaybackTimeline"), TEXT("wallSeconds"));
			}
			S47J::Set(Manifest, TEXT("timing"), S.bAudioRequested
				? TEXT("For playback with live master audio, use wallSeconds and adjacent screenshot request times for variable-frame-rate picture timing. Place the WAV at audioStartRequestOffsetSeconds from CaptureWallStart. This is the start request offset, not a sample-accurate audio-thread timestamp. simulationSeconds records Main's simulation time. Ending cards retain their ordinary Slate clock.")
				: TEXT("simulationSeconds records Main's simulation time at each screenshot request. Use adjacent timestamps for variable-frame-rate playback; wallSeconds also records the request time. Ending cards retain their ordinary Slate clock."));
			S47J::Set(Manifest, TEXT("limits"), S.bAudioRequested
				? TEXT("Screenshot cost changes frame pacing and supplies no profiling evidence. audioComplete reports whether the live master mix was saved; an incomplete capture retains all PNG files.")
				: TEXT("Visual motion capture only; screenshot cost changes frame pacing and supplies no profiling evidence. Audio is not recorded."));
			Manifest->SetArrayField(TEXT("frames"), S.Frames);
			if (!FFileHelper::SaveStringToFile(S47J::Write(Manifest, true), *(S.Dir / TEXT("frames.json")),
				FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &Files, FILEWRITE_NoReplaceExisting))
			{
				UE_LOG(LogS47FinaleMovie, Error, TEXT("S47: finale movie could not write a fresh frames.json"));
				FPlatformMisc::RequestExitWithStatus(false, 1); return;
			}
		}
		UE_LOG(LogS47FinaleMovie, Display, TEXT("S47: finale movie %s: %s, %d frames, %.3f simulation seconds"),
			bComplete ? TEXT("COMPLETE") : TEXT("STOPPED"), *FinalReason, S.Frames.Num(), SimNow);
		FPlatformMisc::RequestExitWithStatus(false, bComplete ? 0 : 1);
	};
	if (!S.bInit)
	{
		S.bInit = true;
		S.bAudioRequested = FParse::Param(FCommandLine::Get(), TEXT("S47FinaleMovieAudio"));
		FString UserDir, Other;
		double AutoSpeed = 0;
		const bool bExplicitUserDir = FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir);
		if (!bExplicitUserDir || FPaths::IsRelative(UserDir)) { Finish(false, TEXT("explicit isolated /tmp UserDir required")); return; }
		FPaths::NormalizeDirectoryName(UserDir); FPaths::CollapseRelativeDirectories(UserDir);
		FString SavedDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
		FPaths::NormalizeDirectoryName(SavedDir); FPaths::CollapseRelativeDirectories(SavedDir);
		if (!UserDir.StartsWith(TEXT("/tmp/")) || !FPaths::IsUnderDirectory(SavedDir, UserDir))
		{ Finish(false, TEXT("Saved must remain inside the explicit /tmp UserDir")); return; }
		FParse::Value(FCommandLine::Get(), TEXT("S47Jump="), S.Jump);
		FParse::Value(FCommandLine::Get(), TEXT("S47Auto="), AutoSpeed);
		const bool bAllowedJump = S.Jump == TEXT("chapter6") || S.Jump == TEXT("chapter6:lastmile") || S.Jump == TEXT("chapter6:line");
		if (!bAllowedJump || !FMath::IsNearlyEqual(AutoSpeed, 24.6, .001)
			|| FParse::Value(FCommandLine::Get(), TEXT("S47FinalePreview="), Other)
			|| FParse::Value(FCommandLine::Get(), TEXT("S47FinaleSweep="), Other))
		{ Finish(false, TEXT("requires chapter6, chapter6:lastmile or chapter6:line and S47Auto=24.6, without a frame fixture")); return; }
		if (Output.IsEmpty() || FPaths::IsRelative(Output)) { Finish(false, TEXT("absolute output directory required")); return; }
		S.Dir = Output; FPaths::NormalizeDirectoryName(S.Dir); FPaths::CollapseRelativeDirectories(S.Dir);
		if (Files.DirectoryExists(*S.Dir) || Files.FileExists(*S.Dir))
		{ Finish(false, TEXT("refusing an existing output path")); return; }
		if (!Files.MakeDirectory(*S.Dir, true)) { Finish(false, TEXT("cannot create output directory")); return; }
		S.bOwnsOutput = true;
		FParse::Value(FCommandLine::Get(), TEXT("S47FinaleMovieFps="), S.TargetFps);
		FParse::Value(FCommandLine::Get(), TEXT("S47FinaleMovieTail="), S.Tail);
		S.bWaitForMile = FParse::Value(FCommandLine::Get(), TEXT("S47FinaleMovieStartMile="), S.StartMile);
		if (S.bWaitForMile && !FMath::IsFinite(S.StartMile)) { Finish(false, TEXT("capture start mile must be finite")); return; }
		S.bEarly = FParse::Value(FCommandLine::Get(), TEXT("S47FinaleMovieEarlyEnd="), S.EarlyTail);
		S.TargetFps = FMath::Clamp(S.TargetFps, 12., 15.); S.Tail = FMath::Clamp(S.Tail, .1, 12.);
		S.EarlyTail = FMath::Clamp(S.EarlyTail, 0., 30.);
	}
	if (!S.bStarted && (WallNow - S.WallStart >= 1500 || S.SimTotal >= 1200))
	{ Finish(false, TEXT("capture wait timeout (1200 simulation / 1500 wall seconds)")); return; }
	if (S.bStarted && (WallNow - S.CaptureWallStart >= 600 || SimNow >= 400))
	{ Finish(false, TEXT("capture timeout after start (400 simulation / 600 wall seconds)")); return; }
	TOptional<double> Mile;
	if (M->World)
	{
		const auto* Road = M->World->OldRoad(); const auto* Driver = M->World->OldDrive();
		if (Road && Driver)
		{
			const double ActualMile = Road->Where(Driver->Pos.X, Driver->Pos.Z).Mi;
			if (FMath::IsFinite(ActualMile)) Mile = ActualMile;
		}
	}
	auto* C = M->Story ? Cast<US47Chapter6>(M->Story->Chapter(TEXT("ch6"))) : nullptr;
	if (!S.bStarted)
	{
		if (!C || !C->bActive || M->Mode != US47Main::EMode::Play || !M->World || M->World->Area != TEXT("roswell")) return;
		if (!M->Flow || !M->Flow->bDevRun || M->Flow->CaseId != 0)
		{ Finish(false, TEXT("capture requires an isolated developer case")); return; }
		// Observe the real approach. Neither this filter nor the waiting observer moves gameplay.
		if (!Mile.IsSet() || (S.bWaitForMile && *Mile < S.StartMile)) return;
		S.bStarted = true; S.SimStart = M->T; S.CaptureWallStart = WallNow;
		S.CaptureStartMile = Mile;
		if (S.bAudioRequested)
		{
			S.AudioMixer = FAudioDeviceManager::GetAudioMixerDeviceFromWorldContext(M->Player.Get());
			if (S.AudioMixer)
			{
				S.AudioStartRequestOffset = FPlatformTime::Seconds() - S.CaptureWallStart;
				S.AudioMixer->StartRecording(nullptr, 600.f);
				S.bAudioStarted = true;
				UE_LOG(LogS47FinaleMovie, Display, TEXT("S47: finale movie requested live master audio at wall offset %.6f"), S.AudioStartRequestOffset);
			}
			else
			{
				S.AudioError = TEXT("no audio mixer device was available at capture start");
				UE_LOG(LogS47FinaleMovie, Warning, TEXT("S47: finale movie %s; continuing PNG capture"), *S.AudioError);
			}
		}
		UE_LOG(LogS47FinaleMovie, Display, TEXT("S47: finale movie observes %s gameplay from actual mile %.3f at %.1f target fps, UI included; %s"),
			*S.Jump, *Mile, S.TargetFps, *S.Dir);
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
	if (Mile.IsSet()) S47J::Set(S.PendingRow, TEXT("mile"), *Mile);
	else S47J::SetNull(S.PendingRow, TEXT("mile"));
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

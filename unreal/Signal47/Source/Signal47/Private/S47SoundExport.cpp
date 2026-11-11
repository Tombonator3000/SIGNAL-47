#include "S47SoundExport.h"
#include "S47AlienKinds.h"
#include "S47Breather.h"
#include "S47CB.h"
#include "S47EncounterSound.h"
#include "S47ReceiverSound.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SoundExport, Log, All);

// N1110: see the header.
namespace
{
	constexpr int32 ExportRate = 48000;

	bool WriteWav(const FString& Path, const TArray<float>& Stereo)
	{
		TArray<uint8> Wav;
		const uint32 Bytes = uint32(Stereo.Num()) * 2u;
		const auto Tag = [&Wav](const char* Text) { for (int32 I = 0; I < 4; ++I) Wav.Add(uint8(Text[I])); };
		const auto U32 = [&Wav](uint32 V) { for (int32 I = 0; I < 4; ++I) Wav.Add(uint8((V >> (8 * I)) & 0xff)); };
		const auto U16 = [&Wav](uint16 V) { Wav.Add(uint8(V & 0xff)); Wav.Add(uint8(V >> 8)); };
		Wav.Reserve(44 + int32(Bytes));
		Tag("RIFF"); U32(36u + Bytes); Tag("WAVE"); Tag("fmt "); U32(16); U16(1); U16(2); U32(ExportRate); U32(ExportRate * 4); U16(4); U16(16); Tag("data"); U32(Bytes);
		for (const float V : Stereo) U16(uint16(int16(FMath::RoundToInt32(FMath::Clamp(FMath::IsFinite(V) ? V : 0.f, -1.f, 1.f) * 32767.f))));
		return FFileHelper::SaveArrayToFile(Wav, *Path);
	}

	// The CB's and the walkie's sound, the five signatures, the receiver's references, the probe, the generator, the dread layer and the steps.
	void OwnClips(const US47Audio& Live, TArray<S47SoundExport::FClip>& Out)
	{
		using namespace S47SoundExport;
		const auto Add = [&Out](const TCHAR* File, const TCHAR* Where, FRun& Run) { Out.Add({ File, Where, MoveTemp(Run.Pcm) }); };

		{ // the CB: the static voice in the words' rhythm, through the dash speaker and its squelch
			FRun R(Live);
			const TSharedRef<FS47CB> Cb = MakeShared<FS47CB>(R.Audio());
			TArray<FS47CBLine> Lines;
			Lines.Add({ TEXT("SARO seven, this is Sierra. You hear me?"), TEXT("SIERRA"), FString() });
			const double Secs = Cb->Call(Lines, TOptional<FVector>(), nullptr);
			if (Secs > 0) { R.Go(Secs + 0.8); Add(TEXT("cb_static_voice"), TEXT("The CB in the truck cab: any CB call line (FS47CB::Call), the set's static in the words' rhythm, through the dash speaker, with the squelch opening and closing. Not placed (in the cab)."), R); }
			else UE_LOG(LogS47SoundExport, Warning, TEXT("S47: export: the CB call did not start"));
		}
		{ // a message on the field radio
			FRun R(Live);
			FS47EncounterSound Enc;
			if (Enc.Message(R.Audio())) { R.Go(4.4); Add(TEXT("walkie_garble"), TEXT("A message on the Freeform field radio / walkie (FS47EncounterSound::Message): the set's static breaking up in speech's rhythm, new each time. Heard when an alien or a happening talks on the set."), R); }
		}
		for (int32 K = 0; K < int32(ES47AlienKind::Num); ++K)
		{ // the five signatures as the receiver passes them (S47AlienKinds::Follow), tier Full
			static const TCHAR* const Names[] = { TEXT("alien_roswell"), TEXT("alien_tall"), TEXT("alien_pale"), TEXT("alien_nordic"), TEXT("alien_cosmic") };
			FRun R(Live);
			const FString Key = FString::Printf(TEXT("n1110_export_%d"), K);
			S47AlienKinds::Follow(R.Audio(), *Key, true, S47AlienKinds::SignalFrom, ES47AlienKind(K), 4747u, 1.f);
			R.Go(S47AlienKinds::SignalSeconds + 0.3);
			S47AlienKinds::Hush(*Key);
			Add(Names[K], *FString::Printf(TEXT("The %s one's signature inside a SARO recording, heard on the receiver / signal-processor tape at tape time %.1f s (%.1f s long). Full tier (after 02:00); before midnight it plays at 40 %% of this level, glimpse tier at 70 %%."), S47AlienKinds::Temper(ES47AlienKind(K)).Id, S47AlienKinds::SignalFrom, S47AlienKinds::SignalSeconds), R);
		}

		HappeningClips(Live, Out);

		{ // the receiver's synthesised references, in the order a player meets them on the list: radar, hum, beacon, pulsar
			static const int32 Index[] = { 7, 8, 9, 6 };
			static const TCHAR* const Names[] = { TEXT("ref_aircraft_radar"), TEXT("ref_power_line_hum"), TEXT("ref_satellite_beacon"), TEXT("ref_pulsar") };
			static const TCHAR* const Wheres[] = {
				TEXT("Receiver reference AIRCRAFT RADAR SWEEP (a rough burst every 4 s): the man-made look-alike the receiver can play beside a take. One 8 s loop, as played."),
				TEXT("Receiver reference POWER-LINE HUM (60 Hz with overtones, slow swell). One 8 s loop, as played."),
				TEXT("Receiver reference SATELLITE BEACON (a 1 kHz tone, a quarter second every second, drifting). One 8 s loop, as played."),
				TEXT("Receiver reference PULSAR (a tick every 0.8 s). One 8 s loop, as played.") };
			for (int32 I = 0; I < 4; ++I)
			{
				FRun R(Live);
				US47Audio* Au = R.Audio();
				FS47ReceiverSound Ref(Au, Au->SfxBus);
				for (int32 N = 0; N < Index[I]; ++N) Ref.Next();
				if (Ref.Index() == Index[I] && Ref.Toggle()) { R.Go(8.0); Add(Names[I], Wheres[I], R); }
			}
		}
		{ // the tracking terminal's search tick (TerminalProbe, S47FreeformScreens.cpp: copied, keep in step), far to on the source
			FRun R(Live);
			US47Audio* Au = R.Audio();
			for (const float Near : { 0.f, 0.25f, 0.5f, 0.75f, 1.f })
			{
				const double Gap = FMath::Lerp(0.7, 0.12, double(Near)); // the plot quickens the tick as the pointer closes in; these gaps are only for the ear
				for (double T = 0; T < 1.4; T += Gap)
				{
					if (Near >= 1.f) Au->Beep(1480.f, .022f, .030f, 0.f, TEXT("square"));
					else Au->Beep(FMath::Lerp(420.f, 1350.f, Near), .016f, FMath::Lerp(.016f, .028f, Near), 0.f, TEXT("square"));
					R.Go(Gap);
				}
				R.Go(0.5);
			}
			Add(TEXT("probe_ticks"), TEXT("Tracking terminal (FREEFORM terminal, search plot): the tick while the pointer searches, in five steps from far off (low, slow) to on the source (1480 Hz, steady)."), R);
		}
		if (Live.Buf.Contains(TEXT("engineStart")) && Live.Buf.Contains(TEXT("engineIdle")) && Live.Buf.Contains(TEXT("engineOff")))
		{ // the backup generator at SARO (FS47FreeformWorld::GeneratorSound / StepGeneratorSound, copied, keep in step), unplaced
			FRun R(Live);
			US47Audio* Au = R.Audio();
			constexpr double GenRate = .62, GenLevel = .34;
			const TCHAR* const LoopKey = TEXT("n1110Generator");
			FS47Sound O; O.Lp = 2400; O.Rate = .82f; O.Gain = .6f;
			Au->Sfx(TEXT("engineStart"), O);
			const double LoopDelay = 2.3 / .82;
			FS47Sound L; L.Gain = float(GenLevel); L.Rate = float(GenRate); L.Lp = 900; L.bLoop = true; L.When = float(LoopDelay);
			const FS47Play P = Au->LoopSfx(LoopKey, TEXT("engineIdle"), L);
			if (P.Gain.IsValid()) { FS47AudioContext::FBatch Bt(*Au->Ctx); const double T = Au->Ctx->CurrentTime() + LoopDelay; P.Gain->Gain.SetValueAtTime(0, T); P.Gain->Gain.LinearRampToValueAtTime(GenLevel, T + .5); }
			R.Go(8.0);
			if (P.Gain.IsValid() && P.Src.IsValid())
			{ // the last twenty minutes of fuel: it hunts
				FS47AudioContext::FBatch Bt(*Au->Ctx); const double T = Au->Ctx->CurrentTime();
				FS47AudioParam& G = P.Gain->Gain; G.CancelAndHoldAtTime(T); G.LinearRampToValueAtTime(GenLevel * .1, T + .09); G.LinearRampToValueAtTime(GenLevel * 1.15, T + .26); G.LinearRampToValueAtTime(GenLevel, T + .45);
				FS47AudioParam& Pr = P.Src->PlaybackRate; Pr.CancelAndHoldAtTime(T); Pr.LinearRampToValueAtTime(GenRate * .72, T + .12); Pr.LinearRampToValueAtTime(GenRate, T + .4);
			}
			R.Go(3.0);
			FS47Sound Off = O; Off.Gain = .5f; Au->Sfx(TEXT("engineOff"), Off); Au->Stop(LoopKey, .35f);
			R.Go(3.5);
			Add(TEXT("generator_start_loop_cough_stop"), TEXT("SARO backup generator: start (0 s), the loop settling in (2.8 s), one fuel cough (8 s, in the last 20 minutes of fuel), switched off (11 s). In the game it is placed at the control box behind the control room, so it is quieter and panned there."), R);
		}
		else UE_LOG(LogS47SoundExport, Warning, TEXT("S47: export: no engineStart/engineIdle/engineOff recordings, the generator is skipped"));

		if (Live.Lic(TEXT("dread_low")).IsValid())
		{ // the dread layer at its game levels (Dread:: in S47EncounterSound.cpp)
			{ FRun R(Live); FS47EncounterSound Enc; Enc.Dread(R.Audio(), 1., 0.); R.Go(10.0); Add(TEXT("dread_ground"), TEXT("Freeform: the low dread loop that comes up when something is near and unseen (lic:dread_low at level .55, fading in over about 3 s; first 10 s of it)."), R); }
			if (Live.Lic(TEXT("dread_high")).IsValid()) { FRun R(Live); FS47EncounterSound Enc; Enc.Dread(R.Audio(), 1., 1.); R.Go(10.0); Add(TEXT("dread_ground_and_layer"), TEXT("Freeform: the ground with the layer over it, as when it is seen close or hunts (lic:dread_low .55 and lic:dread_high .5 together)."), R); }
		}
		for (const int32 S : { 0, 1, 2, 3 })
		{
			static const TCHAR* const Names[] = { TEXT("scare_soft"), TEXT("scare_hard"), TEXT("scare_braam"), TEXT("scare_swell") };
			static const TCHAR* const Wheres[] = {
				TEXT("Freeform: the softer bought hit at a first sight (gain .17, first take of two)."),
				TEXT("Freeform: the hard bought hit when one comes for you (gain .24, first take of three)."),
				TEXT("Freeform: the braam when a landing or the big night is first seen close (gain .21)."),
				TEXT("Freeform: the swell when a light or a figure is first seen far off (gain .17).") };
			FRun R(Live);
			FS47EncounterSound Enc;
			if (Enc.Scare(R.Audio(), FS47EncounterSound::EScare(S), 1., true)) { R.Go(6.0); Add(Names[S], Wheres[S], R); }
		}

		{ // N1110: the Breather's draft as heard at its gate by night, then the once-a-week hush (FS47Breather::StepSound with Level and
		  // Held, copied, keep in step; the panner's fall-off at 5 m from the mouth, reference 3 m, rolloff 1.2, put in by hand)
			FRun R(Live);
			US47Audio* Au = R.Audio();
			const TCHAR* const Key = TEXT("n1110Breather");
			const double AtGate = S47Breather::Level(5.0, 2.5) * 3.0 / (3.0 + 1.2 * (5.0 - 3.0));
			if (S47Breather::StartLoop(Au, Key, TOptional<FVector>(), float(AtGate)).IsValid())
			{
				Au->LoopLevel(Key, float(AtGate), 1.f, 1200.f);
				R.Go(S47Breather::LoopSecs);
				const double Hold = S47Breather::HushSecs();
				for (double Since = 0; Since < Hold + 4.0; Since += 0.05) { Au->LoopLevel(Key, float(AtGate * S47Breather::Held(Since, Hold)), 1.f, 1200.f); R.Go(0.05); }
				R.Go(12.0);
				Au->Stop(Key, .5f); R.Go(.6);
				Add(TEXT("breather_draft_and_hush"), *FString::Printf(TEXT("Freeform, Caliche: the Breather, the cave tour gate west of Main Street (road x -57.5, z 2146). The cave's cold draft as heard at the gate after 02:00, unplaced: two breaths of about %.0f s (0 to %.0f s), then the hush that comes at most once a week (the draft stops in under half a second and holds %.1f s, then comes back over 3 s; in the game the night's crickets and wind hold with it), then one more breath. Louder near the mouth, quieter by day, gone past %.0f m."), S47Breather::Breath, S47Breather::LoopSecs, Hold, S47Breather::Gone), R);
			}
			else UE_LOG(LogS47SoundExport, Warning, TEXT("S47: export: the Breather's draft did not start"));
		}
		{ // six steps on each surface, the game's Step (random takes, gain, rate and low-pass per surface)
			for (const TCHAR* const Surface : { TEXT("tile"), TEXT("concrete"), TEXT("dirt"), TEXT("wood"), TEXT("carpet") })
			{
				FRun R(Live);
				R.Audio()->StepScale = 1.f;
				for (int32 I = 0; I < 6; ++I) { R.Audio()->Step(Surface); R.Go(0.55); }
				R.Go(0.3);
				Add(*FString::Printf(TEXT("steps_%s"), Surface), *FString::Printf(TEXT("Footsteps on %s, six walking steps (US47Audio::Step; bought lic:step_* takes when the build has them)."), Surface), R);
			}
		}
	}
}

S47SoundExport::FRun::FRun(const US47Audio& Live) : A(US47Audio::MakeOffline(Live)) {}

void S47SoundExport::FRun::Go(double Seconds)
{
	US47Audio* Au = A.Get();
	if (!Au || !Au->Ctx.IsValid()) return;
	int32 Left = FMath::Max(0, FMath::RoundToInt32(Seconds * ExportRate));
	TArray<float> Chunk;
	while (Left > 0)
	{
		const int32 N = FMath::Min(Left, 12000);
		Chunk.Reset(); Chunk.SetNumZeroed(N * 2);
		Au->Ctx->Render(Chunk.GetData(), N, 2);
		Au->Ctx->Tick(double(N) / ExportRate); // onended and the timeouts (a stop's fade)
		Pcm.Append(Chunk);
		Left -= N;
	}
}

bool S47SoundExport::RunIfAsked(const US47Audio& Live)
{
	FString Dir;
	if (!FParse::Value(FCommandLine::Get(), TEXT("S47ExportSounds="), Dir) || Dir.IsEmpty()) return false;
	Dir = FPaths::ConvertRelativePathToFull(Dir);
	IFileManager::Get().MakeDirectory(*Dir, true);
	TArray<FClip> Clips;
	OwnClips(Live, Clips);
	FString Index = TEXT("SIGNAL / 47 sound export (N1110): 16-bit 48 kHz stereo, not normalised, as the master would send it (master volume ") + FString::Printf(TEXT("%.2f).\n"), Live.Volume);
	int32 Count = 0;
	for (const FClip& C : Clips)
	{
		if (C.Pcm.IsEmpty()) continue;
		++Count;
		double Peak = 0, Sum = 0;
		for (const float V : C.Pcm) { Peak = FMath::Max(Peak, double(FMath::Abs(V))); Sum += double(V) * V; }
		const FString File = FString::Printf(TEXT("%02d_%s.wav"), Count, *C.File);
		if (!WriteWav(Dir / File, C.Pcm)) { UE_LOG(LogS47SoundExport, Warning, TEXT("S47: export: could not write %s"), *(Dir / File)); continue; }
		Index += FString::Printf(TEXT("%s | %.1f s | peak %.1f dBFS, rms %.1f dBFS | %s\n"), *File, C.Pcm.Num() / 2.0 / ExportRate,
			20. * FMath::LogX(10., FMath::Max(Peak, 1e-9)), 10. * FMath::LogX(10., FMath::Max(Sum / FMath::Max(1, C.Pcm.Num()), 1e-18)), *C.Where);
	}
	FFileHelper::SaveStringToFile(Index, *(Dir / TEXT("index.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	UE_LOG(LogS47SoundExport, Display, TEXT("S47: exported %d sounds to %s"), Count, *Dir);
	if (FParse::Param(FCommandLine::Get(), TEXT("S47ExportSoundsQuit"))) FPlatformMisc::RequestExitWithStatus(false, 0);
	return true;
}

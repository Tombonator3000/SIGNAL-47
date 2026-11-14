#include "S47UiSound.h"
#include "S47Audio.h"
#include "S47Main.h"
#include "S47Sideband.h"
#include "S47Story.h"
#include "S47WebAudio.h"
#include "CoreGlobals.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47UiSound, Log, All);

// (its own namespace, not an anonymous one: the unity build puts other files' helpers beside these)
namespace S47UiSoundImpl
{
	using S47UiSound::EKind;
	using S47UiSound::ETv;
	constexpr int32 Kinds = int32(EKind::Count);
	// Every one-shot is baked to this loudest-50-ms RMS with its peak under BakedPeak (build_uisound.py BAKE_RMS, BAKE_PEAK); a file
	// over the peak by more than half a decibel is not taken (an ear-safe guard against a wrong file in the folder).
	constexpr double BakedRms = -24.0, BakedPeak = -6.0;
	// Each kind's gain in the game (dB on the baked level), the manifest's names for its files, and what it may not pass: the sound
	// it replaces at the same call, measured 14 Nov (peak, loudest 50 ms; dBFS before the master). Nothing played before for the
	// page and the pause menu: they stay under the quietest key. Tom tunes these by ear; build_uisound.py KINDS has the same table.
	struct FKindRow { const TCHAR* Files; double GainDb, RefPeak, RefRms; };
	const FKindRow Rows[Kinds] = {
		{ TEXT("key"), -12.0, -12.1, -34.5 },       // the free click at .30 (kept: the terminal chip's beep after it)
		{ TEXT("tab"), -11.5, -12.1, -34.5 },       // the same click at .30, a little slower
		{ TEXT("action"), -9.0, -9.2, -31.6 },      // the click at .42 (kept: the two-note chirp)
		{ TEXT("detent"), -17.0, -12.1, -39.6 },    // a 2.3 kHz square for 7 ms at .028 (replaced)
		{ TEXT("transport"), -3.0, -3.6, -18.6 },   // the bought small_switch at .38 x 2.2 (replaced, quieter)
		{ TEXT("mark"), -6.0, -3.6, -18.6 },        // the same switch (replaced)
		{ TEXT("action"), -2.0, -5.4, -19.3 },      // the bought rack_switch at .42 x 1.6 (replaced)
		{ TEXT("page"), -13.0, -12.1, -34.5 },
		{ TEXT("menu_move"), -14.0, -12.1, -34.5 },
		{ TEXT("menu_ok"), -11.0, -12.1, -34.5 },
	};
	const TCHAR* const TvFiles[int32(ETv::Count)] = { TEXT("tv_thunk"), TEXT("tv_power_on"), TEXT("tv_power_off") };
	constexpr int32 PerSecond = 12;
	constexpr double Closest = 1.0 / 24, Defer = 0.03, Jitter = 0.02;

	struct FBank
	{
		bool bTried = false;
		TArray<TSharedPtr<FS47AudioBuffer>> Files[Kinds];
		TSharedPtr<FS47AudioBuffer> Tv[int32(ETv::Count)];
		TSharedPtr<FS47AudioBuffer> Bed;
		double LiftDb[2] = { 0, 0 };
		bool bLift[2] = { false, false }; // the manifest says this chain's lift (no lift: no bed on that chain)
		int32 Last[Kinds] = {};
	};
	struct FLimiter
	{
		double Times[PerSecond] = {};
		int32 Count = 0, Next = 0;
		void Reset() { Count = 0; Next = 0; }
		bool Take(double Now)
		{
			if (Count > 0)
			{
				const double Newest = Times[(Next + PerSecond - 1) % PerSecond];
				if (Now - Newest < Closest) return false;
				if (Count == PerSecond && Now - Times[Next] < 1.0) return false;
			}
			Times[Next] = Now; Next = (Next + 1) % PerSecond; Count = FMath::Min(PerSecond, Count + 1);
			return true;
		}
	};
	struct FPending { TWeakPtr<FS47BufferSourceNode> Src; TWeakPtr<FS47GainNode> Gain; uint64 Frame = 0; double At = 0; };

	FBank& Bank() { static FBank B; return B; }
	FLimiter& Limiter() { static FLimiter L; return L; }
	FPending& Pending() { static FPending P; return P; }
	FRandomStream& Dice() { static FRandomStream R(static_cast<int32>(FPlatformTime::Cycles())); return R; }

	double Peak(const FS47AudioBuffer& B)
	{
		double P = 0;
		for (int32 C = 0; C < B.NumberOfChannels(); ++C) { const float* D = B.GetChannelData(C); for (int32 I = 0; I < B.Length(); ++I) P = FMath::Max(P, double(FMath::Abs(D[I]))); }
		return P;
	}
	double Db(double V) { return FMath::Pow(10.0, V / 20.0); }

	TSharedPtr<FS47AudioBuffer> Read(const FString& Folder, const FString& Name, double MaxPeakDb, double MaxSecs)
	{
		TArray<uint8> Bytes;
		const FString Path = Folder / (Name + TEXT(".wav"));
		if (!IFileManager::Get().FileExists(*Path) || !FFileHelper::LoadFileToArray(Bytes, *Path, FILEREAD_Silent)) return nullptr;
		TSharedPtr<FS47AudioBuffer> B = S47Wa::DecodeWav(Bytes);
		if (!B.IsValid() || B->Length() == 0) return nullptr;
		if (Peak(*B) > Db(MaxPeakDb + 0.5) || B->Duration() > MaxSecs)
		{
			UE_LOG(LogS47UiSound, Warning, TEXT("S47: %s left out: louder or longer than it was baked (Tools/uisound/build_uisound.py)"), *Path);
			return nullptr;
		}
		return B;
	}

	// The bank from a folder: its manifest (uisound.json) and the files it names (the self-test runs this on a folder of its own).
	void LoadFrom(const FString& Folder)
	{
		FBank& K = Bank();
		FString Text;
		TSharedPtr<FJsonObject> J;
		if (!FFileHelper::LoadFileToString(Text, *(Folder / TEXT("uisound.json")), FFileHelper::EHashOptions::None, FILEREAD_Silent)
			|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) || !J.IsValid())
		{
			UE_LOG(LogS47UiSound, Display, TEXT("S47: no UI clicks or TV static in %s (the game keeps today's sounds)"), *Folder);
			return;
		}
		const TSharedPtr<FJsonObject>* Names = nullptr;
		int32 Got = 0;
		if (J->TryGetObjectField(TEXT("kinds"), Names))
			for (int32 I = 0; I < Kinds; ++I)
			{
				const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
				if (!(*Names)->TryGetArrayField(Rows[I].Files, List)) continue;
				for (const TSharedPtr<FJsonValue>& V : *List)
					if (const TSharedPtr<FS47AudioBuffer> B = Read(Folder, V->AsString(), BakedPeak, 0.5)) { K.Files[I].Add(B); ++Got; }
			}
		for (int32 I = 0; I < int32(ETv::Count); ++I) K.Tv[I] = Read(Folder, TvFiles[I], BakedPeak, 1.5);
		K.Bed = Read(Folder, TEXT("tv_static"), -1.0, 20.0);
		const TSharedPtr<FJsonObject>* Lift = nullptr;
		// (each chain's lift on its own: a chain whose lift is not said gets no bed, as the bed would be quieter than the hiss it
		// stands in for there; its noise stays. Review 14 Nov: a missing field is not asked for, so it logs no error.)
		K.bLift[0] = K.bLift[1] = false;
		if (K.Bed.IsValid() && J->TryGetObjectField(TEXT("tv_lift_db"), Lift))
		{
			const TCHAR* const Chains[2] = { TEXT("office"), TEXT("motel") };
			for (int32 C = 0; C < 2; ++C)
			{
				double Lifted = 0;
				if ((*Lift)->TryGetNumberField(Chains[C], Lifted) && FMath::IsFinite(Lifted)) { K.LiftDb[C] = FMath::Clamp(Lifted, 0.0, 12.0); K.bLift[C] = true; }
			}
		}
		if (!K.bLift[0] && !K.bLift[1]) K.Bed.Reset();
		UE_LOG(LogS47UiSound, Display, TEXT("S47: %d UI clicks, %d TV one-shots and %s in %s"), Got, int32(K.Tv[0].IsValid()) + K.Tv[1].IsValid() + K.Tv[2].IsValid(),
			!K.Bed.IsValid() ? TEXT("no static bed") : K.bLift[0] && K.bLift[1] ? TEXT("the static bed") : K.bLift[0] ? TEXT("the static bed (office only: no motel lift)") : TEXT("the static bed (motel only: no office lift)"), *Folder);
	}
	void Load()
	{
		FBank& K = Bank();
		if (K.bTried) return;
		K.bTried = true;
		LoadFrom(FPaths::ProjectContentDir() / TEXT("Audio/Raw/media/uisound"));
	}

	// Quiet: the pause menu lets only its own kinds through; a cinematic lets nothing through.
	bool Gate(EKind Kind, bool bPaused, bool bCinematic)
	{
		if (bCinematic) return false;
		return !bPaused || Kind == EKind::MenuMove || Kind == EKind::MenuOk;
	}
	// never the same file twice running
	int32 Choose(int32 Kind, int32 Num, FRandomStream& R)
	{
		if (Num <= 1) return 0;
		int32 I = R.RandRange(0, Num - 2);
		if (I >= Bank().Last[Kind]) ++I;
		return Bank().Last[Kind] = I;
	}
	enum class EResult : uint8 { Played, Quiet, Missing };
	EResult PlayAt(US47Audio* Audio, EKind Kind, bool bDeferred, double Now, bool bPaused, bool bCinematic)
	{
		const int32 I = FMath::Clamp(int32(Kind), 0, Kinds - 1);
		const TArray<TSharedPtr<FS47AudioBuffer>>& Files = Bank().Files[I];
		if (!IsValid(Audio) || !Audio->Ctx.IsValid() || !Audio->SfxBus.IsValid() || Files.Num() == 0) return EResult::Missing;
		if (!Gate(Kind, bPaused, bCinematic) || !Limiter().Take(Now)) return EResult::Quiet;
		const TSharedPtr<FS47AudioContext> Ctx = Audio->Ctx;
		FS47AudioContext::FBatch Batch(*Ctx);
		auto Src = Ctx->CreateBufferSource(); Src->Buffer = Files[Choose(I, Files.Num(), Dice())];
		Src->PlaybackRate.SetValue(1.0 + Dice().FRandRange(-Jitter, Jitter));
		auto G = Ctx->CreateGain(); G->Gain.SetValue(Db(Rows[I].GainDb));
		Src->Connect(G); G->Connect(Audio->SfxBus);
		const double At = Ctx->CurrentTime() + (bDeferred ? Defer : 0.0);
		Src->Start(At);
		if (bDeferred) Pending() = FPending{ Src, G, GFrameCounter, At };
		return EResult::Played;
	}
	void RetagAt(US47Audio* Audio, EKind Kind)
	{
		FPending P = Pending(); Pending() = FPending();
		const int32 I = FMath::Clamp(int32(Kind), 0, Kinds - 1);
		const TSharedPtr<FS47BufferSourceNode> Src = P.Src.Pin(); const TSharedPtr<FS47GainNode> G = P.Gain.Pin();
		if (!IsValid(Audio) || !Audio->Ctx.IsValid() || !Src || !G || P.Frame != GFrameCounter || Bank().Files[I].Num() == 0) return;
		FS47AudioContext::FBatch Batch(*Audio->Ctx);
		if (Audio->Ctx->CurrentTime() > P.At - 0.005) return; // already sounding: it stays as it was
		Src->Buffer = Bank().Files[I][Choose(I, Bank().Files[I].Num(), Dice())];
		G->Gain.SetValue(Db(Rows[I].GainDb));
	}
}

namespace S47UiSound
{
	bool Play(US47Audio* Audio, EKind Kind, bool bDeferred)
	{
		namespace Ui = S47UiSoundImpl;
		Ui::Load();
		const US47Main* M = US47Main::Get(nullptr);
		if (!Audio && M) Audio = M->Audio; // (the paper and the menus have no audio of their own: the game's)
		const bool bPaused = M && M->bPausedByMenu, bCinematic = M && M->Story && M->Story->IsCinematic();
		return Ui::PlayAt(Audio, Kind, bDeferred, FPlatformTime::Seconds(), bPaused, bCinematic) != Ui::EResult::Missing;
	}
	void Retag(US47Audio* Audio, EKind Kind) { S47UiSoundImpl::RetagAt(Audio, Kind); }
	bool Has(EKind Kind) { S47UiSoundImpl::Load(); return S47UiSoundImpl::Bank().Files[FMath::Clamp(int32(Kind), 0, S47UiSoundImpl::Kinds - 1)].Num() > 0; }
	TSharedPtr<FS47AudioBuffer> TvBed(EChain Chain) { S47UiSoundImpl::Load(); const S47UiSoundImpl::FBank& K = S47UiSoundImpl::Bank(); return K.bLift[Chain == EChain::Motel ? 1 : 0] ? K.Bed : TSharedPtr<FS47AudioBuffer>(); }
	double TvBedLift(EChain Chain) { S47UiSoundImpl::Load(); return S47UiSoundImpl::Db(S47UiSoundImpl::Bank().LiftDb[Chain == EChain::Motel ? 1 : 0]); }
	bool Tv(US47Audio* Audio, ETv What, const FVector& AtThree, float Gain, float Lp)
	{
		namespace Ui = S47UiSoundImpl;
		Ui::Load();
		const TSharedPtr<FS47AudioBuffer> B = Ui::Bank().Tv[FMath::Clamp(int32(What), 0, int32(ETv::Count) - 1)];
		if (!IsValid(Audio) || !Audio->Ctx.IsValid() || !B.IsValid()) return false;
		const TSharedPtr<FS47AudioNode> Spk = Audio->Spatial(AtThree, 0.9f);
		if (!Spk.IsValid()) return false;
		FS47AudioContext::FBatch Batch(*Audio->Ctx);
		auto Src = Audio->Ctx->CreateBufferSource(); Src->Buffer = B;
		auto G = Audio->Ctx->CreateGain(); G->Gain.SetValue(FMath::Clamp(Gain, 0.f, 1.f));
		if (Lp > 0.f) { auto F = Audio->Ctx->CreateBiquadFilter(); F->SetType(TEXT("lowpass")); F->Frequency.SetValue(Lp); Src->Connect(F); F->Connect(G); }
		else Src->Connect(G);
		G->Connect(Spk);
		Src->Start(Audio->Ctx->CurrentTime());
		return true;
	}

	// Made buffers, an offline context, no engine audio; the loading path on a folder of made files under Saved/ (removed after).
	bool SelfTest(TArray<FString>& Lines)
	{
		namespace Ui = S47UiSoundImpl;
		bool Ok = true;
		const auto Check = [&](bool B, const TCHAR* Why) { Ok &= B; Lines.Add(FString::Printf(TEXT("%s uisound: %s"), B ? TEXT("PASS") : TEXT("FAIL"), Why)); };
		const Ui::FBank Saved = Ui::Bank(); const Ui::FLimiter SavedLimit = Ui::Limiter();
		ON_SCOPE_EXIT { Ui::Bank() = Saved; Ui::Limiter() = SavedLimit; Ui::Pending() = Ui::FPending(); };

		// the gains: no kind over the sound it replaces, at its peak and its loudest 50 ms; the TV lift never past 12 dB
		bool bUnder = true;
		for (int32 I = 0; I < Ui::Kinds; ++I) bUnder &= Ui::BakedPeak + Ui::Rows[I].GainDb <= Ui::Rows[I].RefPeak + 1e-6 && Ui::BakedRms + Ui::Rows[I].GainDb <= Ui::Rows[I].RefRms + 1e-6 && Ui::Rows[I].GainDb < 0;
		Check(bUnder, TEXT("every kind at or under what it replaces (peak and loudest 50 ms), and quieter than its baked file"));
		Check(Ui::Rows[int32(EKind::Retain)].GainDb > Ui::Rows[int32(EKind::Action)].GainDb && Ui::Rows[int32(EKind::Transport)].GainDb > Ui::Rows[int32(EKind::Key)].GainDb,
			TEXT("the deck's keys are heavier than a PC's, and still under the switches they replace"));

		// no files: every kind says false (the caller plays today's sound)
		Ui::Bank() = Ui::FBank(); Ui::Bank().bTried = true; Ui::Limiter().Reset();
		US47Audio* A = NewObject<US47Audio>(GetTransientPackage()); A->AddToRoot();
		A->Ctx = FS47AudioContext::Make(48000.f); A->bReady = true;
		A->Master = A->Ctx->CreateGain(); A->Master->Connect(A->Ctx->Destination);
		A->SfxBus = A->Ctx->CreateGain(); A->SfxBus->Connect(A->Master);
		bool bAllMissing = true;
		for (int32 I = 0; I < Ui::Kinds; ++I) bAllMissing &= Ui::PlayAt(A, EKind(I), false, 100.0 + I, false, false) == Ui::EResult::Missing;
		Check(bAllMissing && !TvBed().IsValid() && FMath::IsNearlyEqual(TvBedLift(EChain::Office), 1.0), TEXT("without the files: every kind falls back, no bed, no lift"));

		// three files to a kind: each heard, never the same twice running
		const auto Click = [](float Level) { auto Made = MakeShared<FS47AudioBuffer>(1, 480, 48000.f); for (int32 S = 0; S < 480; ++S) Made->GetChannelData(0)[S] = Level * (S < 24 ? 1.f : 0.f); return Made; };
		for (int32 I = 0; I < Ui::Kinds; ++I) if (I != int32(EKind::Retain)) for (int32 V = 0; V < 3; ++V) Ui::Bank().Files[I].Add(Click(.1f + .1f * V));
		Ui::Bank().Files[int32(EKind::Retain)] = Ui::Bank().Files[int32(EKind::Action)];
		bool bThree = true;
		for (int32 I = 0; I < Ui::Kinds; ++I) bThree &= Ui::Bank().Files[I].Num() >= 3;
		FRandomStream R(47);
		int32 Seen[3] = {}, Prev = -1; bool bRepeat = false;
		for (int32 N = 0; N < 60; ++N) { const int32 C = Ui::Choose(0, 3, R); bRepeat |= C == Prev; Prev = C; ++Seen[FMath::Clamp(C, 0, 2)]; }
		Check(bThree && !bRepeat && Seen[0] > 0 && Seen[1] > 0 && Seen[2] > 0, TEXT("with the files: at least three to a kind, all heard, never one twice running"));

		// the rate: a held key at 60 a second is 12 in its first second, none closer than 1/24 s
		Ui::Limiter().Reset();
		int32 Played = 0, Quiet = 0;
		for (int32 N = 0; N < 60; ++N) { const Ui::EResult Res = Ui::PlayAt(A, EKind::Key, false, 200.0 + N / 60.0, false, false); Played += Res == Ui::EResult::Played; Quiet += Res == Ui::EResult::Quiet; }
		Ui::Limiter().Reset();
		const bool bFirst = Ui::PlayAt(A, EKind::Key, false, 300.0, false, false) == Ui::EResult::Played, bTooSoon = Ui::PlayAt(A, EKind::Key, false, 300.02, false, false) == Ui::EResult::Quiet;
		Check(Played == Ui::PerSecond && Quiet == 60 - Ui::PerSecond && bFirst && bTooSoon, TEXT("at most 12 a second, none within 1/24 s of the last; held quiet, not fallen back"));

		// the pause menu and a cinematic
		Ui::Limiter().Reset();
		const bool bPauseKey = Ui::PlayAt(A, EKind::Key, false, 400.0, true, false) == Ui::EResult::Quiet, bPauseMenu = Ui::PlayAt(A, EKind::MenuMove, false, 401.0, true, false) == Ui::EResult::Played;
		const bool bFilm = Ui::PlayAt(A, EKind::MenuOk, false, 402.0, false, true) == Ui::EResult::Quiet;
		Check(bPauseKey && bPauseMenu && bFilm, TEXT("under the pause menu only its own clicks; nothing in a cinematic"));

		// rendered: a click reaches the SFX bus at its gain; the deck's press renamed in its frame takes the other kind's file
		Ui::Limiter().Reset();
		const bool bDeck = Ui::PlayAt(A, EKind::Transport, true, 500.0, false, false) == Ui::EResult::Played;
		const TSharedPtr<FS47BufferSourceNode> Src = Ui::Pending().Src.Pin();
		Ui::RetagAt(A, EKind::Mark);
		const bool bRenamed = Src.IsValid() && Ui::Bank().Files[int32(EKind::Mark)].Contains(Src->Buffer);
		// (the clicks of the checks above were started in this same context and have not been rendered: let their 0.5 ms pass
		// first, 10 ms in all, well before the deferred key's 30 ms; then measure)
		{ TArray<float> Past; Past.SetNumZeroed(480 * 2); A->Ctx->Render(Past.GetData(), 480, 2); }
		TArray<float> Out; Out.SetNumZeroed(4800 * 2);
		A->Ctx->Render(Out.GetData(), 4800, 2);
		double Top = 0; for (const float V : Out) Top = FMath::Max(Top, double(FMath::Abs(V)));
		const double Want = Ui::Peak(*Src->Buffer) * Ui::Db(Ui::Rows[int32(EKind::Mark)].GainDb);
		Check(bDeck && bRenamed && Top > Want * .7 && Top < Want * 1.3, TEXT("the deck's key, named MARK in its frame, sounds as MARK at MARK's gain"));
		// Review 14 Nov: the real loading path (LoadFrom, Read, the manifest) on a folder of made files, and the TV calls on what it
		// loaded: kinds and their variants, a file too loud left out, the TV one-shots, the bed and each chain's lift (one not said)
		{
			const FString Dir = FPaths::ProjectSavedDir() / TEXT("S47UiSoundSelfTest");
			IFileManager::Get().DeleteDirectory(*Dir, false, true);
			const auto Wav = [&Dir](const TCHAR* Name, float Level, int32 Frames)
			{
				TArray<float> Stereo; Stereo.SetNumZeroed(Frames * 2);
				for (int32 S = 0; S < Frames; ++S) Stereo[S * 2] = Stereo[S * 2 + 1] = Level * FMath::Sin(S * .2f);
				return FS47Sideband::WriteWav(Dir / (FString(Name) + TEXT(".wav")), Stereo, 48000);
			};
			// baked at or under -6 dBFS (.5): .4 is taken, .9 is not; the bed may reach -1 dBFS
			bool bMade = Wav(TEXT("k1"), .4f, 2400) && Wav(TEXT("k2"), .35f, 2400) && Wav(TEXT("k3"), .3f, 2400) && Wav(TEXT("k_loud"), .9f, 2400)
				&& Wav(TEXT("a1"), .4f, 2400) && Wav(TEXT("tv_power_on"), .4f, 9600) && Wav(TEXT("tv_static"), .5f, 48000);
			bMade &= FFileHelper::SaveStringToFile(TEXT("{\"kinds\": {\"key\": [\"k1\", \"k2\", \"k3\", \"k_loud\"], \"action\": [\"a1\"], \"page\": [\"nothing_here\"]}, \"tv_lift_db\": {\"office\": 6.0}}"), *(Dir / TEXT("uisound.json")));
			Ui::Bank() = Ui::FBank(); Ui::Bank().bTried = true;
			Ui::LoadFrom(Dir);
			const Ui::FBank& K = Ui::Bank();
			const bool bKinds = K.Files[int32(EKind::Key)].Num() == 3 && K.Files[int32(EKind::Action)].Num() == 1 && K.Files[int32(EKind::Retain)].Num() == 1
				&& K.Files[int32(EKind::Page)].Num() == 0 && K.Files[int32(EKind::Tab)].Num() == 0 && Has(EKind::Key) && !Has(EKind::Tab);
			const bool bTvFiles = K.Tv[int32(ETv::PowerOn)].IsValid() && !K.Tv[int32(ETv::Thunk)].IsValid() && !K.Tv[int32(ETv::PowerOff)].IsValid();
			const bool bLift = TvBed(EChain::Office).IsValid() && FMath::IsNearlyEqual(TvBedLift(EChain::Office), Ui::Db(6.0), 1e-6) && !TvBed(EChain::Motel).IsValid() && TvBed().IsValid();
			const bool bCalls = Tv(A, ETv::PowerOn, FVector::ZeroVector, TubeWithSwitch) && !Tv(A, ETv::Thunk, FVector::ZeroVector, .35f) && !Tv(nullptr, ETv::PowerOn, FVector::ZeroVector, .4f);
			Check(bMade && bKinds && bTvFiles && bLift && bCalls,
				TEXT("loaded from a folder through the manifest: three keys (the loud one left out), RETAIN on ACTION's file, no page, the power-on one-shot played and no thunk; the bed on the office chain at its 6 dB lift, none on the motel's (its lift not said)"));
			IFileManager::Get().DeleteDirectory(*Dir, false, true);
		}
		Check(SwitchWithTube + TubeWithSwitch <= SwitchAlone + 1e-6f && SwitchWithTube > 0 && TubeWithSwitch > 0,
			TEXT("the office TV's switch and the tube's one-shot under it are together no louder than the switch alone"));
		A->Ctx->Close(); A->RemoveFromRoot();
		return Ok;
	}
}

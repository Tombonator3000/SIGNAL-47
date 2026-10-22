// The long media elements of core/radio.ts and core/score.ts, connected to the Web Audio graph.
#include "S47Media.h"

#include "Async/Async.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Archive.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Media, Log, All);

namespace
{
	constexpr int64 ChunkFrames = 32768;
	uint16 U16(const uint8* P) { return uint16(P[0]) | (uint16(P[1]) << 8); }
	uint32 U32(const uint8* P) { return uint32(P[0]) | (uint32(P[1]) << 8) | (uint32(P[2]) << 16) | (uint32(P[3]) << 24); }
	struct FChunk { int64 At = 0; int32 Frames = 0; TArray<float> Data; };
	using FChunkPtr = TSharedPtr<FChunk, ESPMode::ThreadSafe>;
}

struct FS47MediaStream
{
	FString Path;
	int32 Channels = 0, Rate = 0;
	int64 DataAt = 0, Frames = 0;
	FCriticalSection Lock;
	TMap<int64, FChunkPtr> Chunks;
	TSet<int64> Pending;
	uint64 Generation = 0;
	int64 Anchor = 0;
	bool bFailed = false, bClosed = false;
};

namespace
{
	bool Header(FS47MediaStream& S)
	{
		TUniquePtr<FArchive> F(IFileManager::Get().CreateFileReader(*S.Path));
		if (!F || F->TotalSize() < 12) return false;
		uint8 H[16]; F->Serialize(H, 12);
		if (FMemory::Memcmp(H, "RIFF", 4) || FMemory::Memcmp(H + 8, "WAVE", 4)) return false;
		bool bFmt = false, bData = false;
		while (F->Tell() + 8 <= F->TotalSize())
		{
			F->Serialize(H, 8);
			const int64 Len = U32(H + 4), At = F->Tell();
			if (Len > F->TotalSize() - At) return false;
			if (!FMemory::Memcmp(H, "fmt ", 4))
			{
				if (Len < 16) return false;
				F->Serialize(H, 16);
				S.Channels = U16(H + 2); S.Rate = int32(U32(H + 4));
				if (U16(H) != 1 || U16(H + 14) != 16 || S.Channels < 1 || S.Channels > 2 || S.Rate <= 0 || U16(H + 12) != S.Channels * 2) return false;
				bFmt = true;
			}
			else if (!FMemory::Memcmp(H, "data", 4)) { S.DataAt = At; S.Frames = Len; bData = true; }
			F->Seek(At + Len + (Len & 1));
		}
		if (!bFmt || !bData) return false;
		S.Frames /= S.Channels * 2;
		return !F->IsError() && S.Frames > 0;
	}

	FChunkPtr Read(const FS47MediaStream& S, int64 Index)
	{
		const int64 At = Index * ChunkFrames;
		if (At < 0 || At >= S.Frames) return nullptr;
		TUniquePtr<FArchive> F(IFileManager::Get().CreateFileReader(*S.Path));
		if (!F) return nullptr;
		auto C = MakeShared<FChunk, ESPMode::ThreadSafe>(); C->At = At; C->Frames = int32(FMath::Min(ChunkFrames, S.Frames - At));
		TArray<uint8> Bytes; Bytes.SetNumUninitialized(C->Frames * S.Channels * 2);
		F->Seek(S.DataAt + At * S.Channels * 2); F->Serialize(Bytes.GetData(), Bytes.Num());
		if (F->IsError()) return nullptr;
		C->Data.SetNumUninitialized(C->Frames * S.Channels);
		for (int32 I = 0; I < C->Data.Num(); ++I) C->Data[I] = int16(U16(Bytes.GetData() + I * 2)) / 32768.f;
		return C;
	}

	void Request(const TSharedPtr<FS47MediaStream, ESPMode::ThreadSafe>& S, int64 Index)
	{
		if (Index * ChunkFrames >= S->Frames) return;
		uint64 Generation;
		{
			FScopeLock L(&S->Lock);
			if (S->bClosed || S->Chunks.Contains(Index) || S->Pending.Contains(Index)) return;
			S->Pending.Add(Index); Generation = S->Generation;
		}
		Async(EAsyncExecution::ThreadPool, [S, Index, Generation]
		{
			FChunkPtr C = Read(*S, Index);
			FScopeLock L(&S->Lock);
			if (S->bClosed || S->Generation != Generation) return;
			S->Pending.Remove(Index);
			if (!C.IsValid()) S->bFailed = true;
			else if (Index >= S->Anchor - 1 && Index <= S->Anchor + 2) S->Chunks.Add(Index, C);
		});
	}
}

FS47MediaSource::FS47MediaSource(const TSharedRef<FS47AudioContext>& InCtx) : FS47AudioNode(InCtx) {}
FS47MediaSource::~FS47MediaSource()
{
	if (Stream.IsValid()) { FScopeLock L(&Stream->Lock); Stream->bClosed = true; ++Stream->Generation; }
}

TSharedPtr<FS47MediaSource> FS47MediaSource::Open(const TSharedRef<FS47AudioContext>& Ctx, const FString& File)
{
	auto S = MakeShared<FS47MediaStream, ESPMode::ThreadSafe>();
	S->Path = FPaths::IsRelative(File) ? FPaths::ProjectContentDir() / TEXT("Audio/Raw/media") / FPaths::ChangeExtension(File, TEXT("wav")) : File;
	if (!Header(*S)) { UE_LOG(LogS47Media, Warning, TEXT("S47: media missing or not PCM16 WAVE: %s"), *S->Path); return nullptr; }
	auto M = MakeShared<FS47MediaSource>(Ctx); M->Stream = S; M->Name = File;
	M->Seek(0);
	if (S->bFailed) return nullptr;
	return M;
}

bool FS47MediaSource::Play()
{
	double NeedSeek = -1;
	{
		FS47AudioContext::FBatch B(*Ctx);
		if (!Stream.IsValid() || bFailed) return false;
		if (Pos >= Stream->Frames) NeedSeek = 0;
		else
		{
			bool bReady;
			{ FScopeLock L(&Stream->Lock); bReady = Stream->Chunks.Contains(int64(Pos) / ChunkFrames); }
			if (!bReady) NeedSeek = Pos / Stream->Rate;
		}
	}
	if (NeedSeek >= 0) Seek(NeedSeek);
	FS47AudioContext::FBatch B(*Ctx);
	if (bFailed) return false;
	if (!bPlaying) ++Serial;
	bPlaying = true;
	return true;
}

void FS47MediaSource::Pause(bool bOn) { if (!bOn) { Play(); return; } FS47AudioContext::FBatch B(*Ctx); bPlaying = false; }
void FS47MediaSource::Seek(double Seconds)
{
	if (!Stream.IsValid() || !FMath::IsFinite(Seconds)) return;
	// First chunks load on the game thread without the context lock. Audio can keep rendering
	// the old position while they load; only the completed seek is swapped under the lock.
	const auto S = Stream;
	const double At = FMath::Clamp(Seconds * S->Rate, 0.0, double(S->Frames));
	const int64 Index = int64(At) / ChunkFrames;
	TMap<int64, FChunkPtr> Chunks;
	bool bReadFailed = false;
	for (int64 I = Index; I <= Index + 1 && I * ChunkFrames < S->Frames; ++I)
	{
		FChunkPtr C = Read(*S, I);
		if (C.IsValid()) Chunks.Add(I, C); else bReadFailed = true;
	}
	FS47AudioContext::FBatch B(*Ctx);
	Pos = At; ++Serial;
	FScopeLock L(&S->Lock);
	++S->Generation; S->Chunks = MoveTemp(Chunks); S->Pending.Reset(); S->Anchor = Index; S->bFailed = bReadFailed;
	bFailed = bReadFailed;
}

void FS47MediaSource::Stop() { FS47AudioContext::FBatch B(*Ctx); bPlaying = false; Pos = 0; ++Serial; }
double FS47MediaSource::Duration() const { return Stream.IsValid() ? double(Stream->Frames) / Stream->Rate : 0; }
double FS47MediaSource::CurrentTime() const { FS47AudioContext::FBatch B(*Ctx); return Stream.IsValid() ? Pos / Stream->Rate : 0; }
bool FS47MediaSource::Paused() const { FS47AudioContext::FBatch B(*Ctx); return !bPlaying; }

void FS47MediaSource::Notify(bool bError)
{
	const TWeakPtr<FS47AudioNode> Weak = AsShared(); const uint64 AtSerial = Serial;
	const TSharedRef<FS47AudioContext> C = Ctx;
	Ctx->Post([Weak, AtSerial, bError, C]
	{
		TFunction<void()> Fn;
		{
			FS47AudioContext::FBatch B(*C);
			const TSharedPtr<FS47AudioNode> Base = Weak.Pin();
			if (!Base.IsValid()) return;
			const auto M = StaticCastSharedPtr<FS47MediaSource>(Base);
			if (M->Serial == AtSerial) Fn = bError ? M->OnError : M->OnEnded;
		}
		if (Fn) Fn();
	});
}

void FS47MediaSource::Process(int64 Quantum)
{
	(void)Quantum;
	Out.Zero(Stream.IsValid() ? Stream->Channels : 1);
	if (!bPlaying || !Stream.IsValid()) return;
	const auto S = Stream; const double Step = double(S->Rate) / Ctx->SampleRate;
	int64 CachedIndex = -1;
	FChunkPtr Chunk, Next;
	for (int32 I = 0; I < S47Quantum; ++I)
	{
		if (Pos >= S->Frames) { bPlaying = false; Notify(false); break; }
		const int64 Frame = int64(Pos), Index = Frame / ChunkFrames;
		if (CachedIndex != Index)
		{
			bool bReadFailed;
			{
				FScopeLock L(&S->Lock); S->Anchor = Index;
				for (auto It = S->Chunks.CreateIterator(); It; ++It) if (It.Key() < Index - 1 || It.Key() > Index + 2) It.RemoveCurrent();
				Chunk = S->Chunks.FindRef(Index); Next = S->Chunks.FindRef(Index + 1); bReadFailed = S->bFailed;
			}
			if (bReadFailed) { bFailed = true; bPlaying = false; Notify(true); break; }
			if (!Chunk.IsValid()) { Request(S, Index); break; } // buffering: media time stands still
			Request(S, Index + 2); CachedIndex = Index;
		}
		const int32 A = int32(Frame - Chunk->At); const float F = float(Pos - Frame);
		for (int32 C = 0; C < S->Channels; ++C)
		{
			const float V0 = Chunk->Data[A * S->Channels + C];
			const float V1 = A + 1 < Chunk->Frames ? Chunk->Data[(A + 1) * S->Channels + C] : Next.IsValid() ? Next->Data[C] : V0;
			Out.D[C][I] = V0 + (V1 - V0) * F;
		}
		Pos += Step;
	}
}

bool FS47MediaSource::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&Lines, &bOk](bool b, const TCHAR* Text) { Lines.Add(FString::Printf(TEXT("%s media: %s"), b ? TEXT("PASS") : TEXT("FAIL"), Text)); bOk &= b; };
	const FString Path = FPaths::ProjectSavedDir() / TEXT("S47MediaSelfTest.wav");
	constexpr int32 Frames = int32(ChunkFrames * 3) + 512;
	TArray<uint8> Bytes; Bytes.SetNumZeroed(44 + Frames * 2);
	FMemory::Memcpy(Bytes.GetData(), "RIFF", 4); FMemory::Memcpy(Bytes.GetData() + 8, "WAVEfmt ", 8); FMemory::Memcpy(Bytes.GetData() + 36, "data", 4);
	auto Set16 = [&Bytes](int32 At, uint16 V) { Bytes[At] = uint8(V); Bytes[At + 1] = uint8(V >> 8); };
	auto Set32 = [&Set16](int32 At, uint32 V) { Set16(At, uint16(V)); Set16(At + 2, uint16(V >> 16)); };
	Set32(4, Bytes.Num() - 8); Set32(16, 16); Set16(20, 1); Set16(22, 1); Set32(24, 48000); Set32(28, 96000); Set16(32, 2); Set16(34, 16); Set32(40, Frames * 2);
	for (int32 I = 0; I < Frames; ++I) Set16(44 + I * 2, uint16((I % 1024) * 32));
	if (!FFileHelper::SaveArrayToFile(Bytes, *Path)) { Check(false, TEXT("test WAVE written")); return false; }
	auto Ctx = FS47AudioContext::Make(); auto S = Open(Ctx, FPaths::ConvertRelativePathToFull(Path));
	Check(S.IsValid(), TEXT("PCM16 source opens"));
	if (S.IsValid())
	{
		S->Connect(Ctx->Destination); S->Play(); float Out[256]; Ctx->Render(Out, 128);
		Check(FMath::Abs(Out[100] - 50.f / 1024.f) < 1e-6f && FMath::Abs(S->CurrentTime() - 128.0 / 48000) < 1e-9, TEXT("recording enters context graph at exact media time"));
		S->Pause(); Ctx->Render(Out, 128); Check(Out[100] == 0 && FMath::Abs(S->CurrentTime() - 128.0 / 48000) < 1e-9, TEXT("pause holds media position"));
		S->Seek(256.0 / 48000); S->Pause(false); Ctx->Render(Out, 128); Check(FMath::Abs(Out[100] - 306.f / 1024.f) < 1e-6f, TEXT("seek and resume"));
		S->Seek((ChunkFrames - 64.0) / 48000); Ctx->Render(Out, 128);
		Check(FMath::Abs(Out[126] - 1023.f / 1024.f) < 1e-6f && FMath::Abs(Out[128]) < 1e-6f, TEXT("samples cross a streamed chunk boundary"));
		S->Seek((Frames - 128.0) / 48000);
		bool bEnded = false; S->OnEnded = [&bEnded] { bEnded = true; }; Ctx->Render(Out, 128); Ctx->Render(Out, 128); Check(!bEnded, TEXT("ended waits for game thread")); Ctx->Tick(0); Check(bEnded, TEXT("ended dispatched on context tick"));
		S->Stop(); Check(S->Paused() && S->CurrentTime() == 0, TEXT("stop resets position")); S->Disconnect();
	}
	{
		auto C = FS47AudioContext::Make(96000); auto R = Open(C, FPaths::ConvertRelativePathToFull(Path));
		if (R.IsValid()) { R->Connect(C->Destination); R->Play(); float Out[256]; C->Render(Out, 128); Check(FMath::Abs(Out[102] - 25.5f / 1024.f) < 1e-6f, TEXT("recording sample rate resamples through context")); R->Disconnect(); }
		else Check(false, TEXT("resampling source opens"));
		C->Close();
	}
	Ctx->Close(); IFileManager::Get().Delete(*Path);
	return bOk;
}

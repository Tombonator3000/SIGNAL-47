// The long media elements of core/radio.ts and core/score.ts, connected to the Web Audio graph.
#include "S47Media.h"

#include "S47Adpcm.h"
#include "Async/Async.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Archive.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Media, Log, All);

namespace
{
	constexpr int64 ChunkFrames = 32768; // a PCM file's chunk; an ADPCM file's is the whole blocks that fit in it (FS47MediaStream::ChunkLen)
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
	// SIDEBAND-STEMS-ADPCM: a file is 16-bit PCM or IMA ADPCM (S47Adpcm.h). Either is read in chunks of ChunkLen frames; an ADPCM
	// chunk is BlocksPerChunk whole blocks, decoded to floats where the chunk is read (the reader's thread, or a seek on the game
	// thread), so the audio thread only ever reads floats, as from a PCM file.
	bool bAdpcm = false;
	int32 BlockAlign = 0, BlockFrames = 0;
	int64 DataBytes = 0, ChunkLen = ChunkFrames, BlocksPerChunk = 0;
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
		bool bFmt = false, bData = false, bFact = false;
		int64 Fact = 0;
		while (F->Tell() + 8 <= F->TotalSize())
		{
			F->Serialize(H, 8);
			const int64 Len = U32(H + 4), At = F->Tell();
			if (Len > F->TotalSize() - At) return false; // (a truncated file: its last chunk says more than is there)
			if (!FMemory::Memcmp(H, "fmt ", 4))
			{
				if (Len < 16) return false;
				F->Serialize(H, 16);
				S.Channels = U16(H + 2); S.Rate = int32(U32(H + 4));
				if (S.Channels < 1 || S.Channels > 2 || S.Rate <= 0) return false;
				if (U16(H) == 1)
				{
					if (U16(H + 14) != 16 || U16(H + 12) != S.Channels * 2) return false;
				}
				else if (U16(H) == S47Adpcm::FormatTag)
				{
					// IMA ADPCM: 4 bits, a block of whole words, and the samples per block the extension states must be the ones that
					// block holds (a header that says otherwise is not trusted)
					if (Len < 20) return false;
					uint8 X[4]; F->Serialize(X, 4);
					S.bAdpcm = true; S.BlockAlign = U16(H + 12); S.BlockFrames = S47Adpcm::FramesIn(S.BlockAlign, S.Channels);
					if (U16(H + 14) != 4 || S.BlockFrames <= 1 || U16(X) < 2 || U16(X + 2) != S.BlockFrames) return false;
				}
				else return false;
				bFmt = true;
			}
			else if (!FMemory::Memcmp(H, "fact", 4) && Len >= 4) { F->Serialize(H, 4); Fact = U32(H); bFact = true; }
			else if (!FMemory::Memcmp(H, "data", 4)) { S.DataAt = At; S.DataBytes = Len; bData = true; }
			F->Seek(At + Len + (Len & 1));
		}
		if (!bFmt || !bData || F->IsError()) return false;
		if (!S.bAdpcm) { S.Frames = S.DataBytes / (S.Channels * 2); return S.Frames > 0; }
		// what the blocks hold: the whole ones, and a shorter last one if it is whole words (other tools write one)
		const int64 Whole = S.DataBytes / S.BlockAlign, Rest = S.DataBytes - Whole * S.BlockAlign;
		const int32 RestFrames = Rest ? S47Adpcm::FramesIn(int32(Rest), S.Channels) : 0;
		if (Rest && RestFrames <= 0) return false;
		const int64 Held = Whole * S.BlockFrames + RestFrames;
		// the fact chunk is the length (the last block is padded); one that claims more than the blocks hold is a lie
		if (bFact && (Fact <= 0 || Fact > Held)) return false;
		S.Frames = bFact ? Fact : Held;
		S.BlocksPerChunk = FMath::Max<int64>(1, ChunkFrames / S.BlockFrames);
		S.ChunkLen = S.BlocksPerChunk * S.BlockFrames;
		return S.Frames > 0;
	}

	FChunkPtr Read(const FS47MediaStream& S, int64 Index)
	{
		const int64 At = Index * S.ChunkLen;
		if (At < 0 || At >= S.Frames) return nullptr;
		TUniquePtr<FArchive> F(IFileManager::Get().CreateFileReader(*S.Path));
		if (!F) return nullptr;
		auto C = MakeShared<FChunk, ESPMode::ThreadSafe>(); C->At = At; C->Frames = int32(FMath::Min(S.ChunkLen, S.Frames - At));
		if (S.bAdpcm)
		{
			// SIDEBAND-STEMS-ADPCM: the chunk's whole blocks, each decoded from its own header (no state from the block before: the
			// samples are the same whichever way a deck came to them)
			const int64 From = Index * S.BlocksPerChunk * S.BlockAlign, Len = FMath::Min<int64>(S.BlocksPerChunk * S.BlockAlign, S.DataBytes - From);
			if (Len <= 0) return nullptr;
			TArray<uint8> Coded; Coded.SetNumUninitialized(int32(Len));
			F->Seek(S.DataAt + From); F->Serialize(Coded.GetData(), Coded.Num());
			if (F->IsError()) return nullptr;
			TArray<int16> Pcm; Pcm.SetNumUninitialized(int32(S.BlocksPerChunk * S.BlockFrames * S.Channels));
			int32 Got = 0;
			for (int32 B = 0; B < Coded.Num(); B += S.BlockAlign)
			{
				const int32 N = S47Adpcm::DecodeBlock(Coded.GetData() + B, FMath::Min(S.BlockAlign, Coded.Num() - B), S.Channels, Pcm.GetData() + Got * S.Channels);
				if (N < 0) return nullptr; // a broken block: the deck fails as at a read error
				Got += N;
			}
			if (Got < C->Frames) return nullptr;
			C->Data.SetNumUninitialized(C->Frames * S.Channels);
			for (int32 I = 0; I < C->Data.Num(); ++I) C->Data[I] = Pcm[I] / 32768.f;
			return C;
		}
		TArray<uint8> Bytes; Bytes.SetNumUninitialized(C->Frames * S.Channels * 2);
		F->Seek(S.DataAt + At * S.Channels * 2); F->Serialize(Bytes.GetData(), Bytes.Num());
		if (F->IsError()) return nullptr;
		C->Data.SetNumUninitialized(C->Frames * S.Channels);
		for (int32 I = 0; I < C->Data.Num(); ++I) C->Data[I] = int16(U16(Bytes.GetData() + I * 2)) / 32768.f;
		return C;
	}

	void Request(const TSharedPtr<FS47MediaStream, ESPMode::ThreadSafe>& S, int64 Index)
	{
		if (Index * S->ChunkLen >= S->Frames) return;
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

// SIDEBAND-20261113: what grouped decks share (FS47MediaSource::Group). All of it under the context's lock.
struct FS47MediaSync
{
	TArray<TWeakPtr<FS47MediaSource>> Members; // the first one leads
	int64 Quantum = -1;                        // the quantum Pos and bGo were set for
	double Pos = 0;
	bool bGo = false;
};

FS47MediaSource::FS47MediaSource(const TSharedRef<FS47AudioContext>& InCtx) : FS47AudioNode(InCtx) {}
FS47MediaSource::~FS47MediaSource()
{
	if (Stream.IsValid()) { FScopeLock L(&Stream->Lock); Stream->bClosed = true; ++Stream->Generation; }
}

TSharedPtr<FS47MediaSource> FS47MediaSource::Open(const TSharedRef<FS47AudioContext>& Ctx, const FString& File)
{
	auto S = MakeShared<FS47MediaStream, ESPMode::ThreadSafe>();
	S->Path = FPaths::IsRelative(File) ? FPaths::ProjectContentDir() / TEXT("Audio/Raw/media") / FPaths::ChangeExtension(File, TEXT("wav")) : File;
	if (!Header(*S)) { UE_LOG(LogS47Media, Warning, TEXT("S47: media missing, or not a PCM16 or IMA ADPCM WAVE that holds what its header says: %s"), *S->Path); return nullptr; }
	auto M = MakeShared<FS47MediaSource>(Ctx); M->Stream = S; M->Name = File;
	M->Seek(0);
	if (S->bFailed) return nullptr;
	return M;
}

bool FS47MediaSource::Group(const TArray<TSharedPtr<FS47MediaSource>>& Decks)
{
	if (Decks.Num() < 2) return false;
	for (const TSharedPtr<FS47MediaSource>& D : Decks)
	{
		if (!D.IsValid() || !D->Stream.IsValid() || D->Sync.IsValid() || D->Ctx != Decks[0]->Ctx) return false;
		if (D->Stream->Frames != Decks[0]->Stream->Frames || D->Stream->Rate != Decks[0]->Stream->Rate) return false;
	}
	FS47AudioContext::FBatch B(*Decks[0]->Ctx);
	const TSharedPtr<FS47MediaSync> S = MakeShared<FS47MediaSync>();
	for (const TSharedPtr<FS47MediaSource>& D : Decks) { S->Members.Add(D); D->Sync = S; D->bPlaying = false; D->Pos = Decks[0]->Pos; }
	return true;
}

bool FS47MediaSource::Leads() const
{
	return Sync.IsValid() && Sync->Members.Num() > 0 && Sync->Members[0].HasSameObject(this);
}

// The chunks a quantum from At needs are in memory (asks for the ones that are not).
bool FS47MediaSource::ReadyAt(double At, double Step) const
{
	if (!Stream.IsValid() || bFailed) return false;
	if (At >= Stream->Frames) return true; // at its end: nothing to wait for
	const int64 First = int64(At) / Stream->ChunkLen, Last = FMath::Min(int64(At + Step * S47Quantum), Stream->Frames - 1) / Stream->ChunkLen;
	bool bHave = true;
	for (int64 I = First; I <= Last; ++I)
	{
		bool bIn;
		{ FScopeLock L(&Stream->Lock); bIn = Stream->Chunks.Contains(I); }
		if (!bIn) { Request(Stream, I); bHave = false; }
	}
	return bHave;
}

bool FS47MediaSource::Play()
{
	if (!Leads()) return PlaySelf();
	// one hold of the lock: every deck of the group is playing when the audio thread next looks, or none is
	FS47AudioContext::FBatch B(*Ctx);
	bool bOk = true;
	for (const TWeakPtr<FS47MediaSource>& W : Sync->Members) if (const TSharedPtr<FS47MediaSource> D = W.Pin()) bOk &= D->PlaySelf();
	return bOk;
}

bool FS47MediaSource::PlaySelf()
{
	double NeedSeek = -1;
	{
		FS47AudioContext::FBatch B(*Ctx);
		if (!Stream.IsValid() || bFailed) return false;
		if (Pos >= Stream->Frames) NeedSeek = 0;
		else
		{
			bool bReady;
			{ FScopeLock L(&Stream->Lock); bReady = Stream->Chunks.Contains(int64(Pos) / Stream->ChunkLen); }
			if (!bReady) NeedSeek = Pos / Stream->Rate;
		}
	}
	if (NeedSeek >= 0) SeekSelf(NeedSeek);
	FS47AudioContext::FBatch B(*Ctx);
	if (bFailed) return false;
	if (!bPlaying) ++Serial;
	bPlaying = true;
	return true;
}

void FS47MediaSource::Pause(bool bOn)
{
	if (!bOn) { Play(); return; }
	FS47AudioContext::FBatch B(*Ctx);
	bPlaying = false;
	if (Leads()) for (const TWeakPtr<FS47MediaSource>& W : Sync->Members) if (const TSharedPtr<FS47MediaSource> D = W.Pin()) D->bPlaying = false;
}

void FS47MediaSource::Seek(double Seconds)
{
	if (!Leads()) { SeekSelf(Seconds); return; }
	for (const TWeakPtr<FS47MediaSource>& W : Sync->Members) if (const TSharedPtr<FS47MediaSource> D = W.Pin()) D->SeekSelf(Seconds);
}

void FS47MediaSource::SeekSelf(double Seconds)
{
	if (!Stream.IsValid() || !FMath::IsFinite(Seconds)) return;
	// First chunks load on the game thread without the context lock. Audio can keep rendering
	// the old position while they load; only the completed seek is swapped under the lock.
	const auto S = Stream;
	const double At = FMath::Clamp(Seconds * S->Rate, 0.0, double(S->Frames));
	const int64 Index = int64(At) / S->ChunkLen;
	TMap<int64, FChunkPtr> Chunks;
	bool bReadFailed = false;
	for (int64 I = Index; I <= Index + 1 && I * S->ChunkLen < S->Frames; ++I)
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

void FS47MediaSource::Stop()
{
	FS47AudioContext::FBatch B(*Ctx);
	bPlaying = false; Pos = 0; ++Serial;
	if (Leads()) for (const TWeakPtr<FS47MediaSource>& W : Sync->Members) if (const TSharedPtr<FS47MediaSource> D = W.Pin()) { D->bPlaying = false; D->Pos = 0; ++D->Serial; }
}
bool FS47MediaSource::Adpcm() const { return Stream.IsValid() && Stream->bAdpcm; }
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
	Out.Zero(Stream.IsValid() ? Stream->Channels : 1);
	if (!bPlaying || !Stream.IsValid()) return;
	const auto S = Stream; const double Step = double(S->Rate) / Ctx->SampleRate;
	if (Sync.IsValid())
	{
		// SIDEBAND-20261113: the first deck of the group pulled in a quantum speaks for all of them: its position is theirs, and
		// they play this quantum only if every one of them has its samples (else media time stands still for all)
		if (Sync->Quantum != Quantum)
		{
			Sync->Quantum = Quantum; Sync->Pos = Pos; Sync->bGo = true;
			for (const TWeakPtr<FS47MediaSource>& W : Sync->Members)
			{
				const TSharedPtr<FS47MediaSource> D = W.Pin();
				if (D.IsValid() && !D->ReadyAt(Pos, Step)) Sync->bGo = false;
			}
		}
		else Pos = Sync->Pos;
		if (!Sync->bGo) return;
	}
	int64 CachedIndex = -1;
	FChunkPtr Chunk, Next;
	for (int32 I = 0; I < S47Quantum; ++I)
	{
		if (Pos >= S->Frames) { bPlaying = false; Notify(false); break; }
		const int64 Frame = int64(Pos), Index = Frame / S->ChunkLen;
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
		// SIDEBAND-20261113: grouped decks (the stems of a piece) start in one quantum and stay on one position
		auto C = FS47AudioContext::Make();
		auto A = Open(C, FPaths::ConvertRelativePathToFull(Path)), B2 = Open(C, FPaths::ConvertRelativePathToFull(Path)), Alone = Open(C, FPaths::ConvertRelativePathToFull(Path));
		if (A.IsValid() && B2.IsValid() && Alone.IsValid())
		{
			// A to the left, B2 to the right (through a panner each): equal channels mean equal positions
			auto PanA = C->CreateStereoPanner(), PanB = C->CreateStereoPanner(); PanA->Pan.SetValue(-1); PanB->Pan.SetValue(1);
			A->Connect(PanA); B2->Connect(PanB); PanA->Connect(C->Destination); PanB->Connect(C->Destination);
			Check(Group({ A, B2 }) && A->Grouped() && B2->Grouped() && !Group({ A, Alone }), TEXT("two decks of one length group, and a grouped deck is not grouped twice"));
			TArray<float> Pcm; Pcm.SetNumZeroed(4096 * 2);
			C->Render(Pcm.GetData(), 256, 2);
			bool bSilent = true; for (int32 I = 0; I < 512; ++I) bSilent &= Pcm[I] == 0;
			A->Play();
			Check(bSilent && !A->Paused() && !B2->Paused(), TEXT("playing the first of a group starts every deck of it"));
			bool bEqual = true, bMoved = false; int32 Quanta = 0;
			// (the reader's thread brings the next chunks in: render on, a quantum at a time, until the boundary is well behind)
			for (int32 Try = 0; Try < 4000 && A->CurrentTime() < (ChunkFrames + 4096.0) / 48000; ++Try)
			{
				C->Render(Pcm.GetData(), 128, 2); ++Quanta;
				for (int32 I = 0; I < 128; ++I) { bEqual &= Pcm[I * 2] == Pcm[I * 2 + 1]; bMoved |= Pcm[I * 2] != 0; }
				bEqual &= A->CurrentTime() == B2->CurrentTime();
				if (Try % 8 == 7) FPlatformProcess::Sleep(0.001f);
			}
			Check(bEqual && bMoved && A->CurrentTime() >= (ChunkFrames + 4096.0) / 48000, TEXT("grouped decks give the same samples and the same position across a streamed chunk boundary"));
			A->Pause(); const double Held = A->CurrentTime(); C->Render(Pcm.GetData(), 256, 2);
			Check(B2->Paused() && B2->CurrentTime() == Held && A->CurrentTime() == Held, TEXT("pausing the first holds every deck at one position"));
			A->Pause(false); C->Render(Pcm.GetData(), 128, 2);
			bEqual = true; for (int32 I = 0; I < 128; ++I) bEqual &= Pcm[I * 2] == Pcm[I * 2 + 1];
			Check(bEqual && A->CurrentTime() == B2->CurrentTime() && A->CurrentTime() > Held, TEXT("and they go on together"));
			A->Stop(); Check(B2->Paused() && B2->CurrentTime() == 0, TEXT("stopping the first stops the group"));
			A->Disconnect(); B2->Disconnect();
		}
		else Check(false, TEXT("group test sources open"));
		C->Close();
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

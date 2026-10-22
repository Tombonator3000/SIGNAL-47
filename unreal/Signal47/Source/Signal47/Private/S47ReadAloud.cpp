// core/readAloud.ts and core/narrationBanks.ts, in the source's order.
#include "S47ReadAloud.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "S47Audio.h"
#include "S47Docs.h"
#include "S47Opening.h"
#include "S47WebAudio.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47ReadAloud, Log, All);

namespace
{
	constexpr int32 MAX_BUFFERS = 4;
	constexpr int64 MAX_BYTES = 32 * 1024 * 1024;
	constexpr int32 MAX_RESOLVED = 128;

	// Imported URLs are the bank's WAVE files (Scripts/audio-raw.py), listed once per folder.
	// Only the recording being read is loaded and decoded, never the whole library.
	void LoadBank(const TCHAR* Name, FS47ReadingBank& Into)
	{
		const FString RawDir = FPaths::ProjectContentDir() / TEXT("Audio/Raw");
		FString Text; TSharedPtr<FJsonObject> Voices;
		if (!FFileHelper::LoadFileToString(Text, *(RawDir / TEXT("voices.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Voices) || !Voices) return;
		const TSharedPtr<FJsonObject>* B = nullptr;
		const TSharedPtr<FJsonObject>* Cat = nullptr;
		if (!Voices->TryGetObjectField(Name, B) || !B || !*B || !(*B)->TryGetObjectField(TEXT("catalog"), Cat) || !Cat || !*Cat) return;
		const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
		if (!(*Cat)->TryGetArrayField(TEXT("clips"), List) || !List) return;
		// the WAVE files there (one listing of the folder, not a look for each of some 1500 files)
		TSet<FString> Have;
		{
			TArray<FString> Found;
			IFileManager::Get().FindFiles(Found, *(RawDir / TEXT("voice") / Name), TEXT("wav"));
			for (const FString& F : Found) Have.Add(FString(TEXT("voice/")) + Name + TEXT("/") + F);
		}
		for (const TSharedPtr<FJsonValue>& V : *List)
		{
			const TSharedPtr<FJsonObject> O = V->AsObject();
			if (!O) continue;
			FS47ReadingClip C;
			O->TryGetStringField(TEXT("id"), C.Id); O->TryGetStringField(TEXT("text"), C.Text);
			O->TryGetNumberField(TEXT("duration"), C.Duration); O->TryGetStringField(TEXT("file"), C.File);
			FString Raw;
			if (O->TryGetStringField(TEXT("raw"), Raw) && Have.Contains(Raw)) Into.Urls.Add(C.File, RawDir / Raw);
			Into.Clips.Add(MoveTemp(C));
		}
	}

	FString InitialCaseAlternative(const FString& Text)
	{
		if (Text.IsEmpty()) return Text;
		const TCHAR First = Text[0], Upper = FChar::ToUpper(First);
		return FString::Chr(First == Upper ? FChar::ToLower(First) : Upper) + Text.Mid(1);
	}

	// /[\p{L}\p{N}_]/u, by code unit: the letters and digits of the scripts the game's text can have
	// (ASCII, Latin-1 and Latin Extended, Greek, Cyrillic and the rest below U+2000 save the combining
	// marks, and the scripts past U+3040). (Slate has no \p{} classes outside its regular expressions.)
	bool WordCharacter(TCHAR C)
	{
		if (C < 0x80) return (C >= '0' && C <= '9') || (C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z') || C == '_';
		if (C < 0x100) return (C >= 0xC0 && C != 0xD7 && C != 0xF7) || C == 0xAA || C == 0xB5 || C == 0xBA || C == 0xB2 || C == 0xB3 || C == 0xB9 || (C >= 0xBC && C <= 0xBE);
		if (C < 0x2000) return !(C >= 0x300 && C < 0x370);
		if (C >= 0xD800 && C < 0xE000) return false;
		return C >= 0x3040;
	}
	bool Apostrophe(TCHAR C) { return C == 0x2019 || C == '\''; }
	bool InsideWord(const FString& Text, int32 At)
	{
		const int32 N = Text.Len();
		return At > 0 && At < N && (
			(WordCharacter(Text[At - 1]) && WordCharacter(Text[At])) ||
			(Apostrophe(Text[At - 1]) && At > 1 && WordCharacter(Text[At - 2]) && WordCharacter(Text[At])) ||
			(Apostrophe(Text[At]) && At + 1 < N && WordCharacter(Text[At - 1]) && WordCharacter(Text[At + 1])));
	}
	// Punctuation has no spoken content. Signs and hyphens are deliberately absent:
	// a missing minus in -39 LY must never turn into an apparently complete reading.
	bool SilentCharacter(TCHAR C)
	{
		// \s: JavaScript's white space and line terminators
		if (C == ' ' || C == '\t' || C == '\n' || C == '\r' || C == 0x0B || C == 0x0C || C == 0xA0 || C == 0x1680 || (C >= 0x2000 && C <= 0x200A)
			|| C == 0x2028 || C == 0x2029 || C == 0x202F || C == 0x205F || C == 0x3000 || C == 0xFEFF) return true;
		static const FString Marks = TEXT(".,;:!?\"'“”‘’()[]{}/\\|…•");
		int32 I = 0;
		return Marks.FindChar(C, I);
	}

	const TCHAR* SMALL[20] = { TEXT("zero"), TEXT("one"), TEXT("two"), TEXT("three"), TEXT("four"), TEXT("five"), TEXT("six"), TEXT("seven"), TEXT("eight"), TEXT("nine"), TEXT("ten"), TEXT("eleven"), TEXT("twelve"), TEXT("thirteen"), TEXT("fourteen"), TEXT("fifteen"), TEXT("sixteen"), TEXT("seventeen"), TEXT("eighteen"), TEXT("nineteen") };
	const TCHAR* TENS[10] = { TEXT(""), TEXT(""), TEXT("twenty"), TEXT("thirty"), TEXT("forty"), TEXT("fifty"), TEXT("sixty"), TEXT("seventy"), TEXT("eighty"), TEXT("ninety") };

	TArray<FString> Cardinal(int64 Value)
	{
		if (Value < 20) return { SMALL[Value] };
		if (Value < 100) { TArray<FString> W = { TENS[Value / 10] }; if (Value % 10) W.Append(Cardinal(Value % 10)); return W; }
		static const TPair<int64, const TCHAR*> Scales[3] = { { 1000000, TEXT("million") }, { 1000, TEXT("thousand") }, { 100, TEXT("hundred") } };
		for (const TPair<int64, const TCHAR*>& S : Scales)
		{
			if (Value < S.Key) continue;
			TArray<FString> W = Cardinal(Value / S.Key);
			W.Add(S.Value);
			if (Value % S.Key) W.Append(Cardinal(Value % S.Key));
			return W;
		}
		return {};
	}

	TArray<FString> Digits(const FString& Value)
	{
		TArray<FString> W;
		for (const TCHAR D : Value) W.Add(SMALL[FMath::Clamp<int32>(D - '0', 0, 9)]);
		return W;
	}

	TArray<FString> IntegerWords(const FString& Value)
	{
		// Leading zeroes in angles and reference numbers are meaningful, as are the
		// individual digits of unusually large identifiers. Do not round either.
		if ((Value.Len() > 1 && Value[0] == '0') || Value.Len() > 9) return Digits(Value);
		return Cardinal(FCString::Atoi64(*Value));
	}

	TArray<FString> NumberWords(const FString& Token)
	{
		TArray<FString> W;
		if (Token.Contains(TEXT(":")))
		{
			TArray<FString> Parts;
			Token.ParseIntoArray(Parts, TEXT(":"), false);
			for (int32 I = 0; I < Parts.Num(); ++I)
			{
				const int64 Value = FCString::Atoi64(*Parts[I]);
				// A clock reads 03:05 as "three zero five"; seconds use the same rule.
				if (I && Value < 10) W.Add(TEXT("zero"));
				W.Append(Cardinal(Value));
			}
			return W;
		}
		if (Token[0] == '-') W.Add(TEXT("minus"));
		else if (Token[0] == '+') W.Add(TEXT("plus"));
		FString Unsigned = Token[0] == '-' || Token[0] == '+' ? Token.Mid(1) : Token;
		if (Unsigned.EndsWith(TEXT("%"))) Unsigned.LeftChopInline(1);
		Unsigned.ReplaceInline(TEXT(","), TEXT(""));
		FString Whole = Unsigned, Fraction;
		const bool bFraction = Unsigned.Split(TEXT("."), &Whole, &Fraction);
		W.Append(IntegerWords(Whole));
		if (bFraction) { W.Add(TEXT("point")); W.Append(Digits(Fraction)); }
		if (Token.EndsWith(TEXT("%"))) W.Add(TEXT("percent"));
		return W;
	}

	struct FToken { FString Token; TArray<FString> Words; };

	TOptional<FToken> NumericAt(const FString& Text, int32 At)
	{
		if (At && WordCharacter(Text[At - 1])) return {};
		const FString Rest = Text.Mid(At);
		static const FRegexPattern Clock(TEXT("^[0-9]{1,2}:[0-9]{2}(?::[0-9]{2})?(?![\\p{L}\\p{N}_])"));
		static const FRegexPattern Number(TEXT("^[+-]?(?:[0-9]{1,3}(?:,[0-9]{3})+|[0-9]+)(?:\\.[0-9]+)?%?(?![\\p{L}\\p{N}_])"));
		FRegexMatcher C(Clock, Rest);
		if (C.FindNext()) { const FString T = Rest.Mid(0, C.GetMatchEnding()); return FToken{ T, NumberWords(T) }; }
		FRegexMatcher N(Number, Rest);
		if (N.FindNext()) { const FString T = Rest.Mid(0, N.GetMatchEnding()); return FToken{ T, NumberWords(T) }; }
		return {};
	}

	TOptional<FToken> IdentifierAt(const FString& Text, int32 At)
	{
		if (At && WordCharacter(Text[At - 1])) return {};
		const FString Rest = Text.Mid(At);
		static const FRegexPattern Code(TEXT("^([A-Z]{1,3}|[a-z])-?([0-9]{1,3})([A-Za-z])?(?![\\p{L}\\p{N}_])"));
		FRegexMatcher C(Code, Rest);
		if (C.FindNext())
		{
			FToken T;
			T.Token = Rest.Mid(0, C.GetMatchEnding());
			for (const TCHAR L : C.GetCaptureGroup(1).ToUpper()) T.Words.Add(FString::Chr(L));
			T.Words.Append(IntegerWords(C.GetCaptureGroup(2)));
			const FString Suffix = C.GetCaptureGroup(3);
			if (!Suffix.IsEmpty()) T.Words.Add(Suffix.ToUpper());
			return T;
		}
		static const FRegexPattern Frame(TEXT("^(?:frame|FRAME)([0-9]{1,3})(?![\\p{L}\\p{N}_])"));
		FRegexMatcher F(Frame, Rest);
		if (!F.FindNext()) return {};
		FToken T;
		T.Token = Rest.Mid(0, F.GetMatchEnding());
		T.Words.Add(TEXT("frame"));
		T.Words.Append(IntegerWords(F.GetCaptureGroup(1)));
		return T;
	}
}

FString S47Reading::NormalizeReadingText(const FString& Text) { return US47Audio::NormalizeReadingText(Text); }

TArray<FString> S47Reading::DocumentReadingParagraphs(const FString& Text)
{
	// text.replace(/^~[ \t]+/gm, '').replace(/^[ \t]*(?:[-=][ \t]*){3,}$/gm, '')
	TArray<FString> Lines;
	Text.ParseIntoArray(Lines, TEXT("\n"), false);
	for (FString& L : Lines)
	{
		if (L.Len() > 1 && L[0] == '~' && (L[1] == ' ' || L[1] == '\t'))
		{
			int32 I = 1;
			while (I < L.Len() && (L[I] == ' ' || L[I] == '\t')) ++I;
			L = L.Mid(I);
		}
		int32 Marks = 0;
		bool bRule = true;
		for (const TCHAR C : L)
		{
			if (C == '-' || C == '=') ++Marks;
			else if (C != ' ' && C != '\t') { bRule = false; break; }
		}
		// (the line's own spaces before the first mark go with it: [ \t]* then each mark and its spaces)
		if (bRule && Marks >= 3) L.Reset();
	}
	const FString Joined = FString::Join(Lines, TEXT("\n"));
	// .split(/\n\s*\n/).map(normalizeReadingText).filter(Boolean)
	TArray<FString> Out;
	static const FRegexPattern Gap(TEXT("\\n\\s*\\n"));
	FRegexMatcher M(Gap, Joined);
	int32 From = 0;
	auto Take = [&Out](const FString& P) { const FString N = NormalizeReadingText(P); if (!N.IsEmpty()) Out.Add(N); };
	while (M.FindNext())
	{
		Take(Joined.Mid(From, M.GetMatchBeginning() - From));
		From = M.GetMatchEnding();
	}
	Take(Joined.Mid(From));
	return Out;
}

const FS47ReadingBank& S47Reading::KinskiBank()
{
	static const FS47ReadingBank B = []
	{
		FS47ReadingBank K;
		K.Label = TEXT("Synthetic Kinski parody");
		LoadBank(TEXT("kinski"), K);
		return K;
	}();
	return B;
}

const FS47ReadingBank& S47Reading::SerlingBank()
{
	// clips: [...clips, ...openingClips]; urls from both (the opening's last)
	static const FS47ReadingBank B = []
	{
		FS47ReadingBank S;
		S.Label = TEXT("Synthetic Rod Serling parody");
		LoadBank(TEXT("serling"), S);
		LoadBank(TEXT("serling-intro"), S);
		return S;
	}();
	return B;
}

// ---------- ReadAloud ----------

FS47ReadAloud::FS47ReadAloud(US47Audio* InAudio, const FS47ReadingBank& InBank) : Audio(InAudio), Bank(InBank), Life(MakeShared<FLife>())
{
	Life->Owner = this;
	// this.clips = new Map(bank.clips.map((clip) => [normalizeReadingText(clip.text), clip]))
	for (int32 I = 0; I < Bank.Clips.Num(); ++I) Clips.Add(S47Reading::NormalizeReadingText(Bank.Clips[I].Text), I);
	// readingIndex(clips, urls)
	for (const TPair<FString, int32>& E : Clips)
	{
		const FString& Text = E.Key;
		if (Text.IsEmpty() || !HasUrl(E.Value)) continue;
		// A clock-prefixed observation can also be stored without its clock. The
		// leading sentence punctuation in its recorded fragment carries no words.
		// Keep signs and hyphens: a recorded minus must never disappear this way.
		TArray<FString> Bases = { Text };
		int32 Lead = 0;
		while (Lead < Text.Len() && (FChar::IsWhitespace(Text[Lead]) || FString(TEXT(".,;:!?")).Contains(FString::Chr(Text[Lead])))) ++Lead;
		if (Lead && Lead < Text.Len()) Bases.AddUnique(Text.Mid(Lead));
		// Album captions capitalize the first letter of their stored description.
		// Preserve every other letter: US/us and AM/am are not interchangeable.
		TArray<FString> Keys;
		for (const FString& Base : Bases)
		{
			Keys.AddUnique(Base);
			const FString Alternative = InitialCaseAlternative(Base);
			if (Alternative != Base && !Clips.Contains(Alternative)) Keys.AddUnique(Alternative);
		}
		for (const FString& Key : Keys) Indexed.FindOrAdd(Key[0]).Add({ Key, E.Value });
	}
	for (TPair<TCHAR, TArray<FIndexed>>& L : Indexed) L.Value.StableSort([](const FIndexed& A, const FIndexed& B) { return A.Text.Len() > B.Text.Len(); });
}

FS47ReadAloud::~FS47ReadAloud()
{
	Notify = {};
	FinishSource = {};
	Stop();
	Life->Owner = nullptr;
}

void FS47ReadAloud::SetEnabled(bool bValue) { bOn = bValue; if (!bValue) Stop(); }

bool FS47ReadAloud::HasUrl(int32 Clip) const { return Bank.Clips.IsValidIndex(Clip) && Bank.Urls.Contains(Bank.Clips[Clip].File); }

const int32* FS47ReadAloud::ClipOf(const FString& Text) const
{
	// this.clips.get(text) ?? this.clips.get(initialCaseAlternative(text))
	const int32* C = Clips.Find(Text);
	return C ? C : Clips.Find(InitialCaseAlternative(Text));
}

TOptional<TArray<FString>> FS47ReadAloud::Segments(const TArray<FString>& Texts) const
{
	const TOptional<TArray<int32>> R = Resolve(Texts);
	if (!R) return {};
	TArray<FString> Out;
	for (const int32 C : *R) Out.Add(Bank.Clips[C].Text);
	return Out;
}

TOptional<TArray<int32>> FS47ReadAloud::Resolve(const TArray<FString>& Texts) const
{
	if (!Texts.Num()) return {};
	TArray<int32> Result;
	for (const FString& Text : Texts)
	{
		const TOptional<TArray<int32>> Sequence = ResolveText(S47Reading::NormalizeReadingText(Text));
		// A changed or missing word must not silently vanish from a description.
		if (!Sequence) return {};
		Result.Append(*Sequence);
	}
	return Result;
}

TOptional<TArray<int32>> FS47ReadAloud::ResolveText(const FString& Text) const
{
	if (Text.IsEmpty()) return {};
	if (const TOptional<TArray<int32>>* Known = Resolved.Find(Text)) return *Known;
	const int32* Whole = ClipOf(Text);
	TOptional<TArray<int32>> Result;
	if (Whole && HasUrl(*Whole)) Result = TArray<int32>{ *Whole };
	else
	{
		// Work backwards so a long matching prefix cannot hide a shorter valid
		// route. Links avoid recursion and keep long notebooks cheap to resolve.
		struct FPath { TArray<int32> Clips; int32 Next = 0; };
		const int32 N = Text.Len();
		TArray<TOptional<FPath>> Paths;
		Paths.SetNum(N + 1);
		Paths[N] = FPath{ {}, N };
		for (int32 At = N - 1; At >= 0; --At)
		{
			if (const TArray<FIndexed>* List = Indexed.Find(Text[At]))
			{
				for (const FIndexed& Item : *List)
				{
					const int32 End = At + Item.Text.Len();
					if (End > N || !Paths[End] || FCString::Strncmp(*Text + At, *Item.Text, Item.Text.Len()) != 0) continue;
					if (WordCharacter(Item.Text[0]) && InsideWord(Text, At)) continue;
					if (WordCharacter(Item.Text[Item.Text.Len() - 1]) && InsideWord(Text, End)) continue;
					Paths[At] = FPath{ { Item.Clip }, End };
					break;
				}
			}
			if (Paths[At]) continue;
			const TCHAR C = Text[At];
			const TOptional<FToken> Token = (C == '+' || C == '-' || (C >= '0' && C <= '9')) ? NumericAt(Text, At)
				: ((C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z')) ? IdentifierAt(Text, At) : TOptional<FToken>();
			if (Token && Paths[At + Token->Token.Len()])
			{
				TArray<int32> Spoken;
				for (const FString& Word : Token->Words)
				{
					const int32* W = ClipOf(Word);
					if (!W || !HasUrl(*W)) { Spoken.Reset(); break; }
					Spoken.Add(*W);
				}
				if (Spoken.Num() == Token->Words.Num()) Paths[At] = FPath{ Spoken, At + Token->Token.Len() };
			}
			if (!Paths[At] && SilentCharacter(C) && Paths[At + 1]) Paths[At] = FPath{ {}, At + 1 };
		}
		if (Paths[0])
		{
			TArray<int32> Walk;
			for (int32 At = 0; At < N;) { const FPath& P = *Paths[At]; Walk.Append(P.Clips); At = P.Next; }
			if (Walk.Num()) Result = MoveTemp(Walk);
		}
	}
	if (Resolved.Num() >= MAX_RESOLVED && ResolvedOrder.Num()) { Resolved.Remove(ResolvedOrder[0]); ResolvedOrder.RemoveAt(0); }
	Resolved.Add(Text, Result);
	ResolvedOrder.Add(Text);
	return Result;
}

void FS47ReadAloud::StateTo(const FString& S)
{
	CurrentState = S;
	const TFunction<void(const FString&)> On = Notify;
	if (On) On(S);
}

void FS47ReadAloud::Stop()
{
	++Generation;
	bPending = false;
	if (Source.IsValid())
	{
		Source->OnEnded = nullptr;
		Source->Stop(); Source->Disconnect(); Source.Reset();
	}
	Queue.Reset(); TextStarts.Reset(); OnTextFn = {};
	// Resolve the old play() even when its onended handler has been removed.
	TFunction<void()> Finish = MoveTemp(FinishSource);
	FinishSource = {};
	StateTo(TEXT("idle")); Notify = {};
	if (Finish) Finish();
}

void FS47ReadAloud::Play(const TArray<FString>& Texts, TFunction<void(const FString&)> OnState, TFunction<void(int32)> OnText, TFunction<void()> OnDone)
{
	Stop();
	Notify = MoveTemp(OnState);
	if (!Enabled()) { StateTo(TEXT("idle")); Notify = {}; if (OnDone) OnDone(); return; }
	const TOptional<TArray<int32>> Found = Resolve(Texts);
	if (!Found) { StateTo(TEXT("error")); Notify = {}; if (OnDone) OnDone(); return; }
	TMap<int32, int32> Starts;
	int32 Ordinal = 0;
	for (int32 I = 0; I < Texts.Num(); ++I) { Starts.Add(Ordinal, I); Ordinal += Resolve({ Texts[I] })->Num(); }
	const int32 Gen = Generation;
	bPending = true;
	UE_LOG(LogS47ReadAloud, Display, TEXT("S47: %s reads %d texts in %d recordings"), *Bank.Label, Texts.Num(), Found->Num());
	Queue = *Found; TextStarts = MoveTemp(Starts); OnTextFn = MoveTemp(OnText);
	FinishSource = MoveTemp(OnDone);
	StateTo(TEXT("loading"));
	if (!Active(Gen)) return;
	// (audio.unlock(): the game's context was started by its first gesture; a reading must not resume a
	// paused night, so it fails as the web version's does when the context is not running)
	US47Audio* A = Audio.Get();
	if (!A || !A->Ctx.IsValid() || A->Ctx->State() != TEXT("running")) { Fail(Gen, TEXT("Audio could not start")); return; }
	PlayNext(Gen, 0);
}

void FS47ReadAloud::PlayNext(int32 Gen, int32 I)
{
	if (!Active(Gen)) return;
	if (I >= Queue.Num())
	{
		bPending = false;
		Queue.Reset(); TextStarts.Reset(); OnTextFn = {};
		TFunction<void()> Finish = MoveTemp(FinishSource);
		FinishSource = {};
		StateTo(TEXT("idle")); Notify = {};
		if (Finish) Finish();
		return;
	}
	if (const int32* T = TextStarts.Find(I)) { const TFunction<void(int32)> On = OnTextFn; if (On) On(*T); }
	if (!Active(Gen)) return;
	StateTo(TEXT("loading"));
	if (!Active(Gen)) return;
	const TSharedPtr<FS47AudioBuffer> Buffer = Load(Bank.Clips[Queue[I]]);
	if (!Active(Gen)) return;
	US47Audio* A = Audio.Get();
	if (!Buffer.IsValid() || !A || !A->Ctx.IsValid()) { Fail(Gen, FString::Printf(TEXT("Recording unavailable (%s)"), *Bank.Clips[Queue[I]].File)); return; }
	const TSharedRef<FS47AudioContext> Ctx = A->Ctx.ToSharedRef();
	FS47AudioContext::FBatch Batch(*Ctx);
	const TSharedRef<FS47BufferSourceNode> Src = Ctx->CreateBufferSource();
	Src->Buffer = Buffer;
	Src->Connect(A->Master);
	Source = Src;
	const TWeakPtr<FLife> WeakLife = Life;
	const TWeakPtr<FS47BufferSourceNode> WeakSource = Src;
	Src->OnEnded = [WeakLife, WeakSource, Gen, I]
	{
		const TSharedPtr<FLife> L = WeakLife.Pin();
		const TSharedPtr<FS47BufferSourceNode> Ended = WeakSource.Pin();
		if (Ended.IsValid()) Ended->Disconnect();
		if (!L.IsValid() || !L->Owner) return;
		FS47ReadAloud* Self = L->Owner;
		if (Self->Source == Ended) Self->Source.Reset();
		Self->PlayNext(Gen, I + 1);
	};
	Src->Start();
	StateTo(TEXT("playing"));
}

void FS47ReadAloud::Fail(int32 Gen, const FString& Why)
{
	if (!Active(Gen)) return;
	bPending = false;
	if (Source.IsValid())
	{
		Source->OnEnded = nullptr;
		Source->Stop(); Source->Disconnect(); Source.Reset();
	}
	Queue.Reset(); TextStarts.Reset(); OnTextFn = {};
	TFunction<void()> Finish = MoveTemp(FinishSource);
	FinishSource = {};
	StateTo(TEXT("error")); Notify = {};
	UE_LOG(LogS47ReadAloud, Warning, TEXT("S47: read-aloud could not play: %s"), *Why);
	if (Finish) Finish();
}

TSharedPtr<FS47AudioBuffer> FS47ReadAloud::Load(const FS47ReadingClip& Clip)
{
	if (const TSharedPtr<FS47AudioBuffer>* Cached = Buffers.Find(Clip.Id))
	{
		BufferOrder.Remove(Clip.Id); BufferOrder.Add(Clip.Id);
		return *Cached;
	}
	const FString* Url = Bank.Urls.Find(Clip.File);
	TArray<uint8> Encoded;
	if (!Url || !FFileHelper::LoadFileToArray(Encoded, **Url)) return nullptr;
	const TSharedPtr<FS47AudioBuffer> Buffer = S47Wa::DecodeWav(Encoded);
	if (!Buffer.IsValid()) return nullptr;
	const int64 Bytes = int64(Buffer->Length()) * Buffer->NumberOfChannels() * int64(sizeof(float));
	if (Bytes <= MAX_BYTES)
	{
		while (Buffers.Num() && (Buffers.Num() >= MAX_BUFFERS || BufferBytes + Bytes > MAX_BYTES))
		{
			const FString Oldest = BufferOrder[0];
			BufferOrder.RemoveAt(0);
			if (const TSharedPtr<FS47AudioBuffer> Old = Buffers.FindRef(Oldest)) BufferBytes -= int64(Old->Length()) * Old->NumberOfChannels() * int64(sizeof(float));
			Buffers.Remove(Oldest);
		}
		Buffers.Add(Clip.Id, Buffer); BufferOrder.Add(Clip.Id); BufferBytes += Bytes;
	}
	return Buffer;
}

// ---------- tests ----------

bool FS47ReadAloud::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&Lines, &bOk](bool b, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s readAloud: %s"), b ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= b; };
	auto Seg = [](const TOptional<TArray<FString>>& S) { return S ? FString::Join(*S, TEXT("|")) : FString(TEXT("<null>")); };

	// documentReadingParagraphs: pencil marks and paper separators carry no words
	const TArray<FString> P = S47Reading::DocumentReadingParagraphs(TEXT("~ pencil  line\n\n----\nTyped\n  line\n\n\n= = =\n\nNext"));
	Check(P == TArray<FString>({ TEXT("pencil line"), TEXT("Typed line"), TEXT("Next") }), TEXT("document paragraphs without pencil marks and rules"));

	// a bank of made-up recordings (the text matching needs no sound)
	FS47ReadingBank B;
	B.Label = TEXT("Test bank");
	int32 N = 0;
	auto Clip = [&B, &N](const FString& Text, bool bFile = true)
	{
		FS47ReadingClip C; C.Id = FString::Printf(TEXT("c%d"), N); C.File = C.Id + TEXT(".mp3"); C.Text = Text; C.Duration = 1; ++N;
		if (bFile) B.Urls.Add(C.File, TEXT("unused.wav"));
		B.Clips.Add(C);
	};
	Clip(TEXT("The antenna moved. The controller did not send a command."));
	Clip(TEXT("Nothing else."));
	Clip(TEXT(". Dr. Ward read the report."));
	for (const TCHAR* W : { TEXT("zero"), TEXT("one"), TEXT("two"), TEXT("three"), TEXT("four"), TEXT("five"), TEXT("six"), TEXT("nine"), TEXT("twelve"), TEXT("fourteen"),
		TEXT("twenty"), TEXT("thirty"), TEXT("sixty"), TEXT("thousand"), TEXT("hundred"), TEXT("point"), TEXT("percent"), TEXT("B"), TEXT("frame"), TEXT("LY"), TEXT("an"), TEXT("Source distance") }) Clip(W);
	Clip(TEXT("minus"), false); // a recording without its file: never read
	FS47ReadAloud R(nullptr, B);
	Check(R.Label() == TEXT("Test bank"), TEXT("the bank's label"));
	Check(Seg(R.Segments({ TEXT("The antenna moved.  The controller\n did not send a command.") })) == TEXT("The antenna moved. The controller did not send a command."), TEXT("a whole recording, white space normalised"));
	Check(Seg(R.Segments({ TEXT("the antenna moved. The controller did not send a command.") })) == TEXT("The antenna moved. The controller did not send a command."), TEXT("the first letter's other case"));
	Check(Seg(R.Segments({ TEXT("The antenna moved. The controller did not send a command. Nothing else.") })) == TEXT("The antenna moved. The controller did not send a command.|Nothing else."), TEXT("two recordings pieced together"));
	Check(Seg(R.Segments({ TEXT("02:14. Dr. Ward read the report.") })) == TEXT("two|fourteen|. Dr. Ward read the report."), TEXT("a clock before a recorded observation"));
	Check(Seg(R.Segments({ TEXT("Dr. Ward read the report.") })) == TEXT(". Dr. Ward read the report."), TEXT("the observation without its clock"));
	Check(Seg(R.Segments({ TEXT("03:05") })) == TEXT("three|zero|five"), TEXT("a clock reads 03:05 as three zero five"));
	Check(Seg(R.Segments({ TEXT("026") })) == TEXT("zero|two|six"), TEXT("leading zeroes read digit by digit"));
	Check(Seg(R.Segments({ TEXT("1420.405") })) == TEXT("one|thousand|four|hundred|twenty|point|four|zero|five"), TEXT("a frequency, its decimals digit by digit"));
	Check(Seg(R.Segments({ TEXT("60%") })) == TEXT("sixty|percent"), TEXT("a percentage"));
	Check(Seg(R.Segments({ TEXT("B-12") })) == TEXT("B|twelve"), TEXT("an identifier: its letters, then its number"));
	Check(Seg(R.Segments({ TEXT("FRAME04") })) == TEXT("frame|zero|four"), TEXT("FRAME04: frame, then its number as written"));
	Check(!R.Available({ TEXT("Source distance -39 LY") }), TEXT("a missing minus never makes a complete reading"));
	Check(Seg(R.Segments({ TEXT("Source distance 39 LY") })) == TEXT("Source distance|thirty|nine|LY"), TEXT("the same without the sign"));
	Check(!R.Available({ TEXT("antenna") }), TEXT("a fragment never matches inside a word"));
	Check(!R.Available({ TEXT("Something nobody recorded.") }) && !R.Available({}), TEXT("nothing to read: not available"));
	Check(!R.Available({ TEXT("Nothing else."), TEXT("Something nobody recorded.") }), TEXT("one missing text: the whole reading is not available"));

	// play(): off, unavailable, and a reading to the end with its states, texts and settling
	TArray<FString> States; int32 Done = 0;
	R.Play({ TEXT("Nothing else.") }, [&States](const FString& S) { States.Add(S); }, {}, [&Done] { ++Done; });
	Check(R.State() == TEXT("idle") && States == TArray<FString>({ TEXT("idle") }) && Done == 1, TEXT("switched off: idle at once, and settled"));
	R.SetEnabled(true); States.Reset();
	R.Play({ TEXT("Something nobody recorded.") }, [&States](const FString& S) { States.Add(S); }, {}, [&Done] { ++Done; });
	Check(R.State() == TEXT("error") && Done == 2, TEXT("no recording: error, and settled"));
	// a reading with sound: two made-up recordings of a hundredth of a second, written as WAVE files
	US47Audio* A = NewObject<US47Audio>();
	A->Ctx = FS47AudioContext::Make(); A->Master = A->Ctx->CreateGain(); A->Master->Connect(A->Ctx->Destination);
	const FString Wav = FPaths::ProjectSavedDir() / TEXT("S47ReadAloudSelfTest.wav");
	{
		TArray<uint8> W; W.SetNumZeroed(44 + 960);
		FMemory::Memcpy(W.GetData(), "RIFF", 4); FMemory::Memcpy(W.GetData() + 8, "WAVEfmt ", 8); FMemory::Memcpy(W.GetData() + 36, "data", 4);
		auto Set16 = [&W](int32 At, uint16 V) { W[At] = uint8(V); W[At + 1] = uint8(V >> 8); };
		auto Set32 = [&Set16](int32 At, uint32 V) { Set16(At, uint16(V)); Set16(At + 2, uint16(V >> 16)); };
		Set32(4, 36 + 960); Set32(16, 16); Set16(20, 1); Set16(22, 1); Set32(24, 48000); Set32(28, 96000); Set16(32, 2); Set16(34, 16); Set32(40, 960);
		FFileHelper::SaveArrayToFile(W, *Wav);
	}
	for (TPair<FString, FString>& U : B.Urls) U.Value = Wav;
	FS47ReadAloud S(A, B);
	S.SetEnabled(true);
	TArray<int32> Texts; States.Reset(); Done = 0;
	S.Play({ TEXT("Nothing else."), TEXT("12") }, [&States](const FString& X) { States.Add(X); }, [&Texts](int32 I) { Texts.Add(I); }, [&Done] { ++Done; });
	Check(S.Playing() && Texts == TArray<int32>({ 0 }) && States == TArray<FString>({ TEXT("loading"), TEXT("loading"), TEXT("playing") }), TEXT("loading, then the first recording playing"));
	TArray<float> Pcm;
	for (int32 K = 0; K < 6 && S.State() != TEXT("idle"); ++K) { Pcm.SetNumZeroed(1024 * 2); A->Ctx->Render(Pcm.GetData(), 1024); A->Ctx->Tick(0.03); }
	Check(S.State() == TEXT("idle") && Texts == TArray<int32>({ 0, 1 }) && Done == 1, TEXT("each text in turn, to the end, settled once"));
	S.Play({ TEXT("Nothing else.") }, {}, {}, [&Done] { ++Done; });
	const int32 Run = S.RunId();
	S.Stop();
	Check(S.State() == TEXT("idle") && Done == 2 && S.RunId() != Run, TEXT("stop settles the reading and starts a new run"));
	IFileManager::Get().Delete(*Wav);

	// the banks themselves, when their recordings are in this build (Scripts/audio-raw.py); what they
	// have is written in the log as well
	auto Info = [&Lines](const FString& S) { Lines.Add(TEXT("INFO readAloud: ") + S); UE_LOG(LogS47ReadAloud, Display, TEXT("S47: readAloud self test: %s"), *S); };
	const FS47ReadingBank& K = S47Reading::KinskiBank();
	const FS47ReadingBank& Se = S47Reading::SerlingBank();
	Info(FString::Printf(TEXT("Kinski %d clips (%d with recordings), Serling %d (%d)"), K.Clips.Num(), K.Urls.Num(), Se.Clips.Num(), Se.Urls.Num()));
	if (Se.Urls.Num())
	{
		FS47ReadAloud Narrator(nullptr, Se);
		Check(Narrator.Available(FS47Opening::Paragraphs()), TEXT("the narrator has the opening card's five paragraphs"));
		const FS47Doc Log = S47Docs::ShiftLog();
		Info(FString::Printf(TEXT("the shift log's transcript %s for the narrator"), Narrator.Available({ Log.Transcript }) ? TEXT("recorded") : TEXT("not recorded")));
	}
	if (K.Urls.Num())
	{
		FS47ReadAloud Kinski(nullptr, K);
		const FS47Doc Log = S47Docs::ShiftLog();
		Info(FString::Printf(TEXT("the shift log's page %s for Kinski: %s"), Kinski.Available(S47Reading::DocumentReadingParagraphs(Log.Page)) ? TEXT("recorded") : TEXT("not recorded"),
			*FString::Join(Kinski.Segments(S47Reading::DocumentReadingParagraphs(Log.Page)).Get(TArray<FString>()), TEXT(" | ")).Left(160)));
	}
	return bOk;
}

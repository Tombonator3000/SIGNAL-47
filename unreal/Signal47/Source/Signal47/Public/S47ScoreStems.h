#pragma once

#include "CoreMinimal.h"
#include "S47WebAudio.h"

class FS47MediaSource;

// SIDEBAND-20261113, channels: the nine own mixes (s47_*) as stems. Tools/music/build_music.py stems renders each piece again as
// two to four stems that sum to the mix before the bus and stages them out of Git (Content/Audio/Raw/media/music/stems, with
// stems.json); FS47Score plays such a piece as that many decks grouped on one position (FS47MediaSource::Group) with the one
// tape curve of the mix's bus over their sum, and lets a stem in or out with the tension (0 to 1, from SIDEBAND's drivers) on
// the piece's bar lines. With every stem in, it is the mix. Without the files, or with -S47NoStems, the mix file plays as before.
namespace S47Stems
{
	// A stem sounds while From <= tension <= To. In/Out: bars the change takes from the bar line (free time: seconds).
	struct FRow { const TCHAR* Name; double From, To, In, Out; };
	struct FPiece { const TCHAR* File; double Bpm; TArray<FRow> Stems; }; // File: the piece's base name; Bpm 0: free time
	SIGNAL47_API const TArray<FPiece>& Pieces();
	SIGNAL47_API const FPiece* Find(const FString& TrackFile); // "s47_cold_dish.mp3" or the base name
	// In or out at this tension; a stem that is in stays in a little past its edges, one that is out stays out (no chatter).
	SIGNAL47_API bool Sounds(const FRow& Row, double Tension, bool bWasIn);
	// Seconds from a position in the piece to its next bar line (four beats to the bar); 0 in free time.
	SIGNAL47_API double ToBarLine(double Bpm, double Position);
	SIGNAL47_API FString WebName(const FPiece& Piece, const FRow& Row); // "music/stems/s47_cold_dish__ground.mp3"
	// The gain between the mix's working level and its staged level, from stems.json; 0: the piece is not staged as stems.
	SIGNAL47_API double StagedLevel(const FString& BaseName);
	SIGNAL47_API bool Enabled(); // not -S47NoStems
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
	inline constexpr double Hysteresis = .04;
}

class SIGNAL47_API FS47ScoreStems
{
public:
	// The piece's stems, grouped and wired: deck -> Gain each -> sum -> the tape curve at the working level -> Out. Null when the
	// piece has none, a file is missing or they do not group: the caller plays the mix. Tension: what is in from the first frame.
	static TSharedPtr<FS47ScoreStems> Open(const TSharedRef<FS47AudioContext>& Ctx, const FString& TrackFile, double Tension);
	// For the tests: any decks of one length as the stems of a piece, at a level.
	static TSharedPtr<FS47ScoreStems> Make(const TSharedRef<FS47AudioContext>& Ctx, const S47Stems::FPiece& Piece, const TArray<TSharedPtr<FS47MediaSource>>& Decks, double Level, double Tension);
	~FS47ScoreStems();
	TSharedPtr<FS47MediaSource> Leader() const { return Decks.Num() ? Decks[0] : nullptr; }
	TSharedPtr<FS47AudioNode> Out() const { return Post; }
	// Each frame: lets stems in and out for this tension, each change on the next bar line.
	void Step(double Tension);
	void Close();
	int32 Num() const { return Decks.Num(); }
	int32 Wanted() const; // how many stems are in, or on their way in
	bool In(int32 Index) const { return bIn.IsValidIndex(Index) && bIn[Index]; }
	double ChangeAt(int32 Index) const { return Change.IsValidIndex(Index) ? Change[Index] : 0.; }
	const S47Stems::FPiece* Piece = nullptr;

private:
	TSharedPtr<FS47AudioContext> Ctx;
	TArray<TSharedPtr<FS47MediaSource>> Decks;
	TArray<TSharedPtr<FS47GainNode>> Gains;
	TArray<bool> bIn;
	TArray<double> Change; // the context time each stem's last change is over
	TSharedPtr<FS47GainNode> Sum, Post;
	TSharedPtr<FS47WaveShaperNode> Tape;
};

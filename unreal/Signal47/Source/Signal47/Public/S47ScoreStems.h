#pragma once

#include "CoreMinimal.h"
#include "S47WebAudio.h"

class FS47MediaSource;

// SIDEBAND-20261113, channels: the nine own mixes (s47_*) as stems. Tools/music/build_music.py stems renders each piece again as
// two to four stems that sum to the mix before the bus and stages them out of Git (Content/Audio/Raw/media/music/stems, with
// stems.json); FS47Score plays such a piece as that many decks grouped on one position (FS47MediaSource::Group) with the one
// tape curve of the mix's bus over their sum, and lets a stem in or out with the tension (0 to 1, from SIDEBAND's drivers) on
// the piece's bar lines. With every stem in, it is the mix. Without the files, or with -S47NoStems, the mix file plays as before.
//
// The staged form (Tom, 13 November: can they take less space?): 16 bit at 22.05 kHz, half the mixes' rate. Nothing audible is
// lost by the rate: the bus's 9.5 kHz low-pass is baked into every stem, and what lies above 11 kHz is 47 to 70 dB under the whole.
// But FS47MediaSource plays a file by linear interpolation to the context's rate, which dulls a 22.05 kHz file (1.5 dB at 5 kHz,
// 3.9 dB at 8 kHz) and leaves images above 12 kHz. So a fourth-order low-pass at stems.json's "lowpass_hz" (11 kHz) runs over the
// stems' sum here, against the images, and the recipe bakes an equaliser into the stems that gives back what the interpolation
// and this low-pass take (magnitude and phase). A stem whose sides are 30 dB or more under its middle is staged as one channel:
// a one-channel deck goes to both speakers as it is (the graph's up-mix has no pan law), so it sits where its two-channel form sat.
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
	// The low-pass the game runs over the piece's stems (Hz, from stems.json); 0: none (stems staged at the mixes' own rate).
	SIGNAL47_API double StagedLowpass(const FString& BaseName);
	// The two sections of that low-pass (a fourth-order Butterworth), as a BiquadFilterNode's Q in dB.
	inline constexpr double LowpassQ[2] = { -5.3329, 2.3226 };
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
	// Lowpass: Hz of the low-pass over the sum (0: none).
	static TSharedPtr<FS47ScoreStems> Make(const TSharedRef<FS47AudioContext>& Ctx, const S47Stems::FPiece& Piece, const TArray<TSharedPtr<FS47MediaSource>>& Decks, double Level, double Tension, double Lowpass = 0.);
	bool Filtered() const { return Lp[0].IsValid(); }
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
	TSharedPtr<FS47BiquadFilterNode> Lp[2];
};

#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"
#include "S47WebAudio.h"

class US47Audio;
class FS47DecoderPlayer;
class UTextureRenderTarget2D;
class FWidgetRenderer;
class SS47Dsp;

// web/src/story/reel047.ts. Reel 047, 10 August 1979, on the signal processor's deck in the control room
// (story/archivePhone.ts; FORSLAG.md, 20 October, item 5 and Valg for Tom 1). The commissioning series
// ran one reel a night from 25 June 1979 (story/nightshift.ts, reelDate): 047 is the night SARO's
// solver first came out negative, -32, the one that is not in the exceptions binder and not in the
// tape catalogue (SHOW TAPES: 001-046, 048-1311). On it, in order:
//
//   the pulse group   four and seven, every 5.2 s, raw, as tape B has it tonight (core/decoder.ts)
//   a crank           a field telephone rung by its handle: the crank telephone's bell (the motel
//                     office's own, story/officePhone.ts), a little way off, two turns
//   a cut             metal going through metal, a snap: a cable through cutters
//   nothing           for 47 seconds, not even the hiss: the tape has nothing on it at all. The deck
//                     runs on and its reels go round; the night outside goes quiet with it
//   a relay           close and loud, on the tape: the way K3 trips on the rack beside the deck
//   the range         the processor reads the data track's last solve off the end of the tape:
//                     SOURCE DISTANCE -32 LY, on its display
//
// None of it is explained. It pays off later: FRAME 04 (the cable at STATION 01, cut), E10 (02:17:00
// the carrier ceased, 02:17:47 the relay and the reference moving), N. Vega's correction (she cut it
// with Tomás in the reference). -32 is 1979 less 1947; nobody says so (HISTORIE.md).
//
// Driven on the game's own time (update(dt)), not the AudioContext's, so a test can run it fast; each
// sound starts when its moment comes. The deck's motor, the hiss and the 4/7 are continuous and are
// gated on their own nodes. Without sound it runs the same, silent, with its captions.

/** 'thread' | 'start' | 'pulse' | 'crank' | 'cut' | 'hole' | 'relay' | 'back' | 'range' | 'leader' | 'end' */
struct FS47ReelCue { double At = 0; FString Kind; };

namespace S47Reel047
{
	/** When each thing comes, seconds from play(). The hole is exactly 47 s. */
	SIGNAL47_API const TArray<FS47ReelCue>& REEL_PLAN();
	inline constexpr double HOLE_FROM = 18.6, HOLE_LEN = 47;
	/** story/officePhone.ts crankBell: the office's own bell, made once (two small brass gongs a little
	 *  apart in pitch and a clapper between them, struck in turn as fast as the generator at the other
	 *  end is turned). (officePhone.ts is the phones' port; the bell is made here too for the reel.) */
	SIGNAL47_API TSharedRef<FS47AudioBuffer> CrankBell(const FS47AudioContext& Ctx);
}

/** What the processor's display shows now (draw()): the reel's state, its counter, and whether the
 *  tape has run to its end. */
struct FS47DspPicture { FString State, Counter; bool bFinished = false; double T = 0; };

/** The processor's display (room.dspScreen: a 256 x 72 canvas), drawn into a render target on its
 *  material as the control room's CRTs are. */
class SIGNAL47_API FS47DspScreen
{
public:
	~FS47DspScreen();
	/** The target the screen's material shows, and the material's id (the one whose map is room.dspScreen.tex). */
	void Init(UTextureRenderTarget2D* InTarget);
	void Draw(const FS47DspPicture& P);
	bool IsValid() const { return Target.IsValid(); }

private:
	TWeakObjectPtr<UTextureRenderTarget2D> Target;
	FWidgetRenderer* Renderer = nullptr;
	TSharedPtr<SS47Dsp> Widget;
};

struct FS47Reel047Deps
{
	TWeakObjectPtr<US47Audio> Audio;
	/** Where the deck is (the signal processor's reels), its two reels, and its display. */
	TFunction<FVector()> Deck;
	TArray<FS47Obj> Reels;
	FS47DspScreen* Screen = nullptr;
	/** The screen shows the reel from now (a render target over the processor's own canvas), or the
	 *  processor's own again (false). */
	TFunction<void(bool)> ShowScreen;
	/** The night outside quiet (secs, level, tau, back), as Ambience.hush. */
	TFunction<void(double, double, double, double)> Hush;
};

class SIGNAL47_API FS47Reel047
{
public:
	explicit FS47Reel047(FS47Reel047Deps InD);
	~FS47Reel047();

	/** Playing now (one thing at a time: main.ts holds the night and the music for it). */
	bool bPlaying = false;
	/** Seconds into it. */
	double T = 0;
	/** The cues as they came (tests). */
	TArray<FString> Heard;
	/** It reached the end (not stopped before). */
	bool bFinished = false;
	/** What is on the tape now, 0 (nothing at all: the 47 s, not even the hiss) to 1 (tests). */
	double Gate = 0;

	/** Thread it and play it; `Cue` gets each thing on it as it comes. */
	void Play(TFunction<void(const FS47ReelCue&)> Cue);
	/** Stopped where it is (a new night, the title): not heard to the end. */
	void Stop();
	/** A night opened or put back: stopped, and nothing of it on the processor. */
	void Reset();
	void Update(double Dt);

	/** What the processor shows: 'THREADING' | 'PLAY' | 'PULSE 4/7' | 'NO SIGNAL' | 'RELAY' | 'RANGE' ('' before it plays). */
	FString State() const;
	void Draw();
	/** (Unreal: the VU unit on the processor reads it, S47ConsoleKit.h) The level of what comes off the
	 *  tape since the last call (RMS): the tape's chain and the pulse group's playback. */
	double Level();

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47Reel047Deps D;
	int32 K = 0;
	double Spin = 0;
	double ScrT = 0;
	TSharedPtr<FS47DecoderPlayer> Dec;
	TSharedPtr<class FS47LevelMeter> Meter;
	TArray<TSharedPtr<FS47AudioNode>> Nodes;
	TArray<TSharedPtr<FS47ScheduledSourceNode>> Srcs;
	TSharedPtr<FS47GainNode> Tape;
	struct FMotor { TSharedPtr<FS47OscillatorNode> O; TSharedPtr<FS47GainNode> G; };
	TOptional<FMotor> Motor;
	TFunction<void(const FS47ReelCue&)> Cb;

	void Step(double Dt);
	void Build();
	void Sound(const FS47ReelCue& C);
	void Release(bool bNow);
};

#pragma once

#include "CoreMinimal.h"
#include "S47Decoder.h"
#include "S47Obj.h"
#include "S47Panels.h"
#include "UObject/StrongObjectPtr.h"

class US47UI;
class US47Audio;
class US47Inter;
class AS47Web;
class UTextureRenderTarget2D;
class FWidgetRenderer;
class SS47DeskDsp;

// web/src/story/Decoder.ts. The signal processor next to RX bank 3: it holds the tape of the anomaly
// once the receiver has locked on it (from 02:14 in the prologue, and all night after that) and plays
// it back raw or harmonized (core/decoder.ts, FS47DecoderPlayer). Its display and reels follow the
// playback. It belongs to the control room, not to a chapter: AS47Story (Prologue.ts) holds it.
//
// (room.dspScreen is a 256 x 72 canvas in the web version; here the processor's own picture is drawn
// into a render target of that size set on the screen's material, as reel 047's is, S47SaroNight.cpp.
// Reel 047 shows its own target on the same material while it has the display, and gives it back.)

struct FS47DecoderDeskDeps
{
	TWeakObjectPtr<US47UI> UI;
	TWeakObjectPtr<US47Audio> Audio;
	TWeakObjectPtr<AS47Web> Room;
	TWeakObjectPtr<US47Inter> Inter;
	TFunction<bool()> Locked;
	TFunction<FString()> Recorded;
	TFunction<bool()> Busy;
	TFunction<bool()> Power;
};

class SIGNAL47_API FS47DecoderDesk
{
public:
	explicit FS47DecoderDesk(FS47DecoderDeskDeps InD);
	~FS47DecoderDesk();

	bool Playing() const;
	/** Reel 047 on the deck (story/reel047.ts, story/archivePhone.ts; set by main.ts, S47Phones.cpp): its
	 *  label and its use while it is in the kit or playing (empty label: null; Use false: not its), and
	 *  whether it has the display and the reels now. */
	struct FReel { TFunction<FString()> Label; TFunction<bool()> Use; TFunction<bool()> Owns; };
	FReel Reel;

	void Stop();
	/** Per frame (Prologue.update: after the chapters). */
	void Update(double Dt);
	/** (Unreal) The playback steps back under a phone or a voice (AS47Story::VoicesDuck). */
	void Duck(double K) { if (Player.IsValid()) Player->Duck(K); }

	/** The interactable 'decoder' (inter.add in the constructor): its label and its use. */
	FString Label() const;
	void Use();

	/** The panel's view, and the panel itself (private open() in Decoder.ts; public here for the
	 *  developer's -S47Decoder run). Open returns the toast it said instead ('' once the panel is up). */
	S47Panels::FDecoderView View() const;
	FString Open();
	/** The panel's onMode, onParam and onPlay. */
	void OnMode(const FString& M);
	void OnParam(const FString& K, double V);
	void OnPlay(bool bPlay);
	/** The parameters as the panel sets them (tests). */
	const FS47DecoderParams& Params() const { return P; }
	/** The display's picture now (tests): 'NO TAPE', or the mode line. */
	FString ScreenLine() const;

	static bool SelfTest(TArray<FString>& Lines);

private:
	TSharedRef<bool> Alive = MakeShared<bool>(true);
	FS47DecoderDeskDeps D;
	TSharedPtr<FS47DecoderPlayer> Player;
	FS47DecoderParams P;
	double ScrT = 0;
	double Spin = 0;
	TArray<FS47Obj> Reels;

	// room.dspScreen: the render target, its material and the widget drawn into it
	TStrongObjectPtr<UTextureRenderTarget2D> Target;
	int32 Mat = -1;
	FWidgetRenderer* Renderer = nullptr;
	TSharedPtr<SS47DeskDsp> Widget;

	void Play();
	void DrawScreen();
	void InitScreen();
};

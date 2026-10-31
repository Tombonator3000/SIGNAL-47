#include "S47DecoderDesk.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
// story/Decoder.ts

#include "Brushes/SlateColorBrush.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Rendering/DrawElements.h"
#include "RenderingThread.h"
#include "S47Audio.h"
#include "S47ConsoleKit.h"
#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Json.h"
#include "S47Main.h"
#include "S47Observing.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47Yard.h"
#include "Slate/WidgetRenderer.h"
#include "SS47Widgets.h"
#include "UnrealClient.h"
#include "UObject/Package.h"
#include "Widgets/SLeafWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Desk, Log, All);

// ---------- (Unreal only, on Tom's request) the answer's papers ----------

namespace
{
	// T. Vega's page under the spare reel: the seven numbered as they come, and what the line sang back
	const TCHAR* VEGA_PAGE = TEXT("Jul --. Hut receiver, past two.\n\nFour, a gap, seven. With the lamp off they are not clicks. They are notes. The four go up. The seven come down, a step at a time.\n\nNumbered the seven as they come. 1 the top, 7 the bottom.\n\nWhistled the four back down the line.\n\nIt sang back the seven. Not in that order:\n\n7  5  3  2  1  4  6\n\nTwice. The same both times. N. asleep on the bench. Not telling her until I have it three times.\n\nT.V.");
	// TAPE A: the wire in the hut at STATION 01, the night of the last observation, on the field line from C:
	// he answers the signal as the player has just answered it, and what comes back is the processor's
	// own playing, in the hut (1947 took in this night, HISTORIE.md). E10's 02:17:00 and 02:17:47, N.
	// Vega's account in room 6 (the lamp off, the arrangement answering him, the cable cut), plate 4's
	// third figure (field book: "Three on plate 4. There were two of us."), the reflective band of the
	// SARO jacket on the one who stands out there (story/recording.ts), reel 047's crank, cut and relay.
	const TCHAR* TAPE_A = TEXT("FIELD WIRE / STATION 01 / TAPE A\nWire recorder in the hut, on the field line from C. July 1947.\n\n02:15:10 / Hiss. The relay: four, a gap, seven.\nT. VEGA (on the line, out on C): Nora, the lamp is off. It is still coming in.\nT. VEGA: I am giving it the four. Then the seven, the way it sang them to me.\n02:15:41 / A whistle on the line: four notes up, then seven. 7 5 3 2 1 4 6.\n02:15:58 / The seven come back. Not on the line: in the hut, close to the microphone. Slow, like an organ in a big room.\nN. VEGA (in the hut): Tomás. Come in off the line.\nT. VEGA: There is someone at the transit. A stripe across the back. It shines, and there is no light on it.\nT. VEGA: The third one. From plate 4.\n02:16:53 / The field telephone, rung by its handle. Twice.\nN. VEGA: Now, Tomás.\nT. VEGA: Last reading. Reference west. No. East. Hold the last reading.\n02:17:00 / Cutters. The carrier stops. Nothing on the wire, not even the hiss.\n02:17:47 / A relay, close and loud.\nN. VEGA: Tomás?\nThe hiss comes back. Nobody answers her.");
}

// ---------- the processor's display (room.dspScreen) ----------

/** What drawScreen() draws: 'NO TAPE' before anything is locked, else the mode line and the eleven
 *  pulses with the sounding one lit. */
struct FS47DeskPicture { bool bLocked = false; FString Line; int32 Cur = -1; };

class SS47DeskDsp : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47DeskDsp) {}
	SLATE_END_ARGS()
	void Construct(const FArguments&) { White = MakeUnique<FSlateColorBrush>(FLinearColor::White); }
	FS47DeskPicture P;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(256, 72); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
	{
		// the canvas's colours, linear, into an sRGB target (as reel 047's display, S47Reel047.cpp)
		auto Css = [](uint32 C, float A = 1.f) { FLinearColor L = FLinearColor::FromSRGBColor(FColor((C >> 16) & 255, (C >> 8) & 255, C & 255)); L.A = A; return L; };
		auto Box = [&](float X, float Y, float W, float H, const FLinearColor& C) { FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(X, Y))), White.Get(), ESlateDrawEffect::None, C); };
		// '20px VT323', textBaseline 'top': the text's top at y
		auto Txt = [&](const FString& S, float X, float Y, const FLinearColor& C) { FSlateDrawElement::MakeText(Out, Layer + 1, G.ToPaintGeometry(FVector2f(250.f, 24.f), FSlateLayoutTransform(FVector2f(X, Y))), S, S47Style::Mono(20.f), ESlateDrawEffect::None, C); };
		Box(0, 0, 256, 72, Css(0x031008));
		if (!P.bLocked) { Txt(TEXT("NO TAPE"), 10, 8, Css(0x8cffa4, 0.35f)); return Layer + 2; }
		Txt(P.Line, 8, 6, Css(0x8cffa4));
		for (int32 I = 0; I < 11; ++I) Box(float(10 + (I < 4 ? I : I + 1) * 20), 40, 14, 22, I == P.Cur ? Css(0xb9ffc7) : Css(0x6edc96, 0.3f));
		return Layer + 2;
	}

private:
	TUniquePtr<FSlateBrush> White;
};

// ---------- the desk ----------

FS47DecoderDesk::FS47DecoderDesk(FS47DecoderDeskDeps InD) : D(MoveTemp(InD))
{
	US47Inter* Inter = D.Inter.Get();
	AS47Web* Room = D.Room.Get();
	// (Unreal only, on Tom's request) the bought unit on the console desk is the processor when the export
	// has it; the deck on the rack is taken away (S47ConsoleKit.h)
	if (Room && FS47ConsoleKit::Find(Room) >= 0) KitPtr = MakeUnique<FS47ConsoleKit>(Room);
	if (Inter && Room)
	{
		const TWeakPtr<bool> Life = Alive;
		FS47Interactable I;
		I.Id = TEXT("decoder"); I.Object = KitPtr ? KitPtr->Obj() : Room->Obj(TEXT("room.objs.decoder")); I.Range = 2.4;
		I.Label = [this, Life]
		{
			if (!Life.IsValid()) return FString();
			if (const auto* M = US47Main::Get(D.Room.Get()); M && M->IsFreeform() && M->Freeform)
				if (const FString L = M->Freeform->Label(TEXT("decoder")); !L.IsEmpty()) return L;
			if (const auto* M = US47Main::Get(D.Room.Get()); M && M->Observing)
				if (const FString L = M->Observing->Label(TEXT("decoder")); !L.IsEmpty()) return L;
			return Label();
		};
		I.Use = [this, Life]
		{
			if (!Life.IsValid()) return;
			if (const auto* M = US47Main::Get(D.Room.Get()); M && M->IsFreeform() && M->Freeform && M->Freeform->Use(TEXT("decoder"))) return;
			if (const auto* M = US47Main::Get(D.Room.Get()); M && M->Observing && M->Observing->Use(TEXT("decoder"))) return;
			Use();
		};
		// (Unreal: the closer look at the bought unit, not story/looks.ts's deck behind smoked plastic)
		if (KitPtr) I.Examine = [] { return FString(TEXT("The signal processor: a deck with a small green display, and a meter unit on top of it, two VU meters lit amber. Headphones and a desk microphone in front.")); };
		Inter->Add(I);
		// T. Vega's page under the spare reel, from the lock on (the answer)
		if (KitPtr && KitPtr->Page())
		{
			FS47Interactable Pg;
			Pg.Id = TEXT("vegaPage"); Pg.Object = KitPtr->Page(); Pg.Range = 2.0;
			Pg.Label = [this, Life] { return Life.IsValid() && PageThere() ? FString(TEXT("A folded page under the spare reel")) : FString(); };
			Pg.Use = [this, Life] { if (Life.IsValid() && PageThere()) ReadPage(); };
			Pg.Examine = [this, Life] { return Life.IsValid() && PageThere() ? FString(TEXT("A page torn out of a field book, folded twice, pushed half under the spare reel. The paper has gone yellow at the folds. Pencil.")) : FString(); };
			Inter->Add(Pg);
		}
	}
	// (the bought unit has no reels that turn: the spare reel beside it is part of the desk's clutter)
	if (Room && !KitPtr) for (const FString& H : Room->PathsUnder(TEXT("room.reels["))) Reels.Add(Room->Obj(H));
	InitScreen();
	DrawScreen();
	// -S47Answer (with -S47Jump=locked or later): the panel opened three seconds in and the answer keyed;
	// =tape: TAPE A opened once it is filed, =notebook: the notebook's papers then; =page: T. Vega's page
	// read instead; =pagepanel: the page filed and the panel opened (the developer's, for the screenshots)
	bTestAnswer = FParse::Param(FCommandLine::Get(), TEXT("S47Answer")) || FParse::Value(FCommandLine::Get(), TEXT("S47Answer="), TestMode);
	FParse::Value(FCommandLine::Get(), TEXT("S47KitShots="), ShotPrefix);
	float At = -1.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("S47KitShotAt="), At)) ShotAt = At;
}

FS47DecoderDesk::~FS47DecoderDesk()
{
	if (Player.IsValid()) Player->Stop();
	if (Renderer) { BeginCleanup(Renderer); Renderer = nullptr; }
	Widget.Reset();
}

void FS47DecoderDesk::InitScreen()
{
	// room.dspScreen: the material whose map is its canvas (room.dspScreen.tex), as S47SaroNight finds it
	// for reel 047; the processor's own picture is a render target of the canvas's size set on it
	AS47Web* W = D.Room.Get();
	if (!W || !FApp::CanEverRender()) return;
	const int32 Tex = W->HandleTex(TEXT("room.dspScreen.tex"));
	for (const TSharedPtr<FJsonValue>& V : US47Yard::ManifestSection(TEXT("materials")))
	{
		const S47J::FObj O = V->AsObject();
		if (O.IsValid() && S47J::Has(O, TEXT("map")) && S47J::Int(O, TEXT("map"), -1) == Tex && Tex >= 0) { Mat = S47J::Int(O, TEXT("id"), -1); break; }
	}
	UTextureRenderTarget2D* T = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
	T->ClearColor = FLinearColor(0.001f, 0.005f, 0.002f);
	T->InitCustomFormat(256, 72, PF_B8G8R8A8, false);
	T->UpdateResourceImmediate(true);
	Target.Reset(T);
	Renderer = new FWidgetRenderer(false);
	Widget = SNew(SS47DeskDsp);
}

bool FS47DecoderDesk::Playing() const { return Player.IsValid() && (Player->bPlaying || Player->bPhrase); }

FString FS47DecoderDesk::Label() const
{
	// this.reel?.label() ?? (this.playing ? ... : ...)
	if (Reel.Label) { const FString L = Reel.Label(); if (!L.IsEmpty()) return L; }
	return Playing() ? TEXT("Signal processor (playing)") : TEXT("Signal processor");
}

void FS47DecoderDesk::Use()
{
	if (Reel.Use && Reel.Use()) return;
	Open();
}

S47Panels::FDecoderView FS47DecoderDesk::View() const
{
	S47Panels::FDecoderView V;
	V.Tape = FString::Printf(TEXT("TAPE B / 1420.405 MHz / PULSE GROUP 4 / 7 / RECORDED %s"), D.Recorded ? *D.Recorded() : TEXT(""));
	V.Mode = P.Mode; V.Pitch = P.Pitch; V.Stretch = P.Stretch; V.Space = P.Space;
	V.bPlaying = Playing();
	const TWeakPtr<FS47DecoderPlayer> Pl = Player;
	V.Current = [Pl] { const TSharedPtr<FS47DecoderPlayer> X = Pl.Pin(); return X.IsValid() ? X->Current : -1; };
	V.Keys = KeyLog;
	V.bAnswering = Answering();
	V.bPageThere = PageThere();
	V.bPageRead = D.HasPaper && D.HasPaper(TEXT("vegaPage"));
	return V;
}

FString FS47DecoderDesk::Open()
{
	US47UI* UI = D.UI.Get();
	US47Audio* A = D.Audio.Get();
	auto Say = [UI](const FString& S, float Secs) { if (UI) UI->Toast(S, Secs); return S; };
	if (D.Busy && D.Busy()) return TEXT("busy");
	if (!(D.Power && D.Power())) return Say(TEXT("No power to the processor. It runs off RX bank 3."), 3);
	if (!(D.Locked && D.Locked())) return Say(TEXT("The deck records whatever the receiver locks on. Nothing has been locked tonight."), 3.6f);
	if (!A || !A->Ctx.IsValid()) return Say(TEXT("The processor needs sound. Tap the screen first."), 3);
	const TWeakPtr<bool> Life = Alive;
	S47Panels::FDecoderPanel O;
	O.View = [this, Life] { return Life.IsValid() ? View() : S47Panels::FDecoderView(); };
	O.OnMode = [this, Life](const FString& M) { if (Life.IsValid()) OnMode(M); };
	O.OnParam = [this, Life](const FString& K, double V) { if (Life.IsValid()) OnParam(K, V); };
	O.OnPlay = [this, Life](bool bPlay) { if (Life.IsValid()) OnPlay(bPlay); };
	O.OnKey = [this, Life](int32 K) { if (Life.IsValid()) OnKey(K); };
	O.bOnDesk = KitPtr.IsValid();
	O.OnClose = [this, Life] { if (Life.IsValid()) bPanelOpen = false; };
	S47Panels::DecoderPanel(UI, O);
	bPanelOpen = UI != nullptr;
	return FString();
}

void FS47DecoderDesk::OnMode(const FString& M)
{
	P.Mode = M;
	if (Player.IsValid()) { FS47DecoderSet S; S.Mode = M; Player->Set(S); }
}

void FS47DecoderDesk::OnParam(const FString& K, double V)
{
	// this.params[k] = v; this.player?.set({ [k]: v })
	FS47DecoderSet S;
	if (K == TEXT("pitch")) { P.Pitch = V; S.Pitch = V; }
	else if (K == TEXT("stretch")) { P.Stretch = V; S.Stretch = V; }
	else if (K == TEXT("space")) { P.Space = V; S.Space = V; }
	else return;
	if (Player.IsValid()) Player->Set(S);
}

void FS47DecoderDesk::OnPlay(bool bPlay)
{
	if (bPlay) Play(); else Stop();
}

FVector FS47DecoderDesk::SoundAt() const
{
	// (Unreal: the bought unit's front; in the web version the deck on the rack, a little up and out)
	if (KitPtr) return KitPtr->SoundAt();
	return (D.Room.IsValid() ? D.Room->Obj(TEXT("room.objs.decoder")).WorldPosition() : FVector::ZeroVector) + FVector(-0.3, 1.1, 0);
}

bool FS47DecoderDesk::EnsurePlayer()
{
	US47Audio* A = D.Audio.Get();
	const TSharedPtr<FS47AudioBuffer> Noise = A ? A->NoiseBuffer() : nullptr;
	if (!A || !A->Ctx.IsValid() || !Noise.IsValid()) return false;
	if (!Player.IsValid()) Player = MakeShared<FS47DecoderPlayer>(A->Ctx.ToSharedRef(), A->SfxBus, SoundAt(), Noise);
	return true;
}

void FS47DecoderDesk::Play()
{
	US47Audio* A = D.Audio.Get();
	if (!EnsurePlayer()) return;
	AbortAnswer(); // (Unreal: the tape over the answer, if it was still giving it back)
	FS47Sound C; C.Gain = 0.5f; A->Sfx(TEXT("click"), C);
	FS47DecoderSet S; S.Mode = P.Mode; S.Pitch = P.Pitch; S.Stretch = P.Stretch; S.Space = P.Space;
	Player->Start(S);
}

void FS47DecoderDesk::Stop()
{
	AbortAnswer();
	if (!Playing()) return;
	if (US47Audio* A = D.Audio.Get()) { FS47Sound C; C.Gain = 0.4f; A->Sfx(TEXT("click"), C); }
	Player->Stop();
	DrawScreen();
}

// ---------- per frame ----------

UTexture* FS47DecoderDesk::CanvasMap() const
{
	// what room.dspScreen's canvas shows now: the processor's own picture, or reel 047's
	AS47Web* W = D.Room.Get();
	UMaterialInstanceDynamic* Mid = W && Mat >= 0 ? W->SharedMid(Mat) : nullptr;
	return Mid ? Mid->K2_GetTextureParameterValue(TEXT("Map")) : nullptr;
}

void FS47DecoderDesk::Update(double Dt)
{
	const auto* Main = US47Main::Get(D.Room.Get());
	const auto* Obs = Main && Main->Observing ? Main->Observing.Get() : nullptr;
	const auto* Free = Main && Main->IsFreeform() && Main->Freeform ? Main->Freeform.Get() : nullptr;
	const bool bObservingAudio = (Free && !Free->AudioDisplay().IsEmpty()) || (Obs && !Obs->AudioDisplay().IsEmpty());
	if (Player.IsValid()) Player->Update();
	// (Unreal only) the answer's timeline and the developer's run of it; the bought unit's needles, display
	// and page, whoever has the deck
	if (AnswerT >= 0) StepAnswer(Dt);
	if (bTestAnswer) TestAnswer(Dt);
	LifeT += Dt;
	if (!ShotPrefix.IsEmpty() && ShotAt >= 0 && Shots < 2 && LifeT >= ShotAt + Shots)
	{
		const FString Path = FString::Printf(TEXT("%s_%c.png"), *ShotPrefix, TCHAR('a' + Shots));
		FScreenshotRequest::RequestScreenshot(Path, true, false);
		UE_LOG(LogS47Desk, Display, TEXT("S47: kit shot %s: needles %.3f %.3f, display %s; TAPE A %s, page %s, papers tapeA %s"), *Path, KitPtr ? KitPtr->Needle(0) : -1.0, KitPtr ? KitPtr->Needle(1) : -1.0, *ScreenLine(),
			D.Answered && D.Answered() ? TEXT("answered") : TEXT("not answered"), PageThere() ? TEXT("there") : TEXT("not there"), D.HasPaper && D.HasPaper(TEXT("tapeA")) ? TEXT("filed") : TEXT("not filed"));
		++Shots;
	}
	if (KitPtr)
	{
		const double Mine = Player.IsValid() ? Player->TakeLevel() : 0.0;
		const double Reel047 = D.ReelLevel ? D.ReelLevel() : 0.0;
		const double ObservingLevel = Free ? Free->AudioLevel() : bObservingAudio ? Obs->AudioLevel() : 0.0;
		KitPtr->Update(Dt, FMath::Max3(Mine, Reel047, ObservingLevel), CanvasMap());
		KitPtr->ShowPage(PageThere());
		// (the developer's runs: the level and the needles in the log, for the meters' calibration)
		if ((!ShotPrefix.IsEmpty() || !TestMode.IsEmpty()) && (Mine > 0 || Reel047 > 0) && (LogT -= Dt) <= 0)
		{
			LogT = 0.5;
			UE_LOG(LogS47Desk, Display, TEXT("S47: processor level %.4f (reel %.4f), needles %.3f %.3f"), Mine, Reel047, KitPtr->Needle(0), KitPtr->Needle(1));
		}
	}
	if (Reel.Owns && Reel.Owns()) return;   // (reel 047 is on the deck: it has the display and the reels)
	if (Playing())
	{
		Spin += Dt * (P.Mode == TEXT("raw") ? 2.4 : 1.6) / P.Stretch;
		for (int32 I = 0; I < Reels.Num(); ++I) Reels[I].SetRotX(Spin * (I ? 0.8 : 1.15));
	}
	ScrT -= Dt;
	if (ScrT <= 0 && (bObservingAudio || Playing() || ScrT < -1 || (Player.IsValid() && Player->Current >= 0))) { ScrT = 1.0 / 12; DrawScreen(); }
}

FString FS47DecoderDesk::ScreenLine() const
{
	if (const auto* M = US47Main::Get(D.Room.Get()); M && M->IsFreeform() && M->Freeform)
		if (const FString Line = M->Freeform->AudioDisplay(); !Line.IsEmpty()) return Line;
	if (const auto* M = US47Main::Get(D.Room.Get()); M && M->Observing)
		if (const FString Line = M->Observing->AudioDisplay(); !Line.IsEmpty()) return Line;
	if (!(D.Locked && D.Locked())) return TEXT("NO TAPE");
	// (Unreal only) the answer as it comes back, then the wire off the end of tape B
	if (Answering()) return AnswerT < Ans.Wire() ? TEXT("ANSWER") : AnswerT < Ans.Cut() ? TEXT("TAPE A 1947") : TEXT("TAPE A  ----");
	return FString::Printf(TEXT("%s %s%sST X%.1f"), P.Mode == TEXT("raw") ? TEXT("RAW ") : TEXT("HARM"), P.Pitch >= 0 ? TEXT("+") : TEXT(""), *FString::SanitizeFloat(P.Pitch, 0), P.Stretch);
}

// The display on the processor: mode, settings, and the pulses as they pass.
void FS47DecoderDesk::DrawScreen()
{
	if (!Renderer || !Widget.IsValid() || !Target.IsValid() || !FSlateApplication::IsInitialized()) return;
	FS47DeskPicture Pic;
	Pic.bLocked = D.Locked && D.Locked();
	Pic.Line = ScreenLine();
	// (Unreal: a key's note lights its bar while nothing plays, S47Decoder.h Key)
	Pic.Cur = Player.IsValid() && !(Answering() && AnswerT >= Ans.Wire()) ? Player->Current : -1;
	if (const auto* M = US47Main::Get(D.Room.Get()); M && M->Observing && !M->Observing->AudioDisplay().IsEmpty())
	{
		Pic.bLocked = true;
		const double Level = M->Observing->AudioLevel();
		Pic.Cur = Level > .01 ? FMath::Clamp(FMath::RoundToInt(Level * 10), 0, 10) : -1;
	}
	if (const auto* M = US47Main::Get(D.Room.Get()); M && M->IsFreeform() && M->Freeform && !M->Freeform->AudioDisplay().IsEmpty())
	{ Pic.bLocked=true; const double Level=M->Freeform->AudioLevel(); Pic.Cur=Level>.01?FMath::Clamp(FMath::RoundToInt(Level*10),0,10):-1; }
	Widget->P = Pic;
	Renderer->DrawWidget(Target.Get(), Widget.ToSharedRef(), FVector2D(256, 72), 0.f);
	// (one canvas in the web version, drawn by whichever draws last: here the screen shows this target
	// again once reel 047 has shown its own on it)
	AS47Web* W = D.Room.Get();
	UMaterialInstanceDynamic* Mid = W && Mat >= 0 ? W->SharedMid(Mat) : nullptr;
	if (Mid && Mid->K2_GetTextureParameterValue(TEXT("Map")) != Target.Get()) FS47Mat{ W, Mat }.SetMap(Target.Get(), true);
}

// ---------- (Unreal only, on Tom's request) T. Vega's page and the answer ----------

const TArray<int32>& FS47DecoderDesk::ANSWER()
{
	// what the line sang back to him (his page): not the order the seven come in, nor its reverse
	static const TArray<int32> A = { 7, 5, 3, 2, 1, 4, 6 };
	return A;
}

bool FS47DecoderDesk::IsAnswer(const TArray<int32>& Last)
{
	// the last seven keys pressed, whatever came before them
	const TArray<int32>& A = ANSWER();
	if (Last.Num() < A.Num()) return false;
	for (int32 I = 0; I < A.Num(); ++I) if (Last[Last.Num() - A.Num() + I] != A[I]) return false;
	return true;
}

FS47Doc FS47DecoderDesk::PageDoc()
{
	FS47Doc Doc;
	Doc.Id = TEXT("vegaPage"); Doc.Title = TEXT("A page from T. Vega's field book, 1947"); Doc.Kind = TEXT("hand"); Doc.Stamp = TEXT("T. VEGA");
	Doc.Page = VEGA_PAGE;
	Doc.Transcript = TEXT("Under the spare reel beside the signal processor: a page torn out of a field book and folded twice. Pencil, T. Vega's hand. The seven of the pulse group, numbered as they come, and the order they came back in.");
	return Doc;
}

FS47Doc FS47DecoderDesk::TapeDoc(const FString& Recorded)
{
	FS47Doc Doc;
	Doc.Id = TEXT("tapeA"); Doc.Title = TEXT("TAPE A / 1947"); Doc.Kind = TEXT("typed"); Doc.Stamp = TEXT("TAPE A");
	Doc.Page = TAPE_A;
	Doc.Transcript = TEXT("The processor ran on past the end of tape B after the answer and played this, a wire recording, typed out as it came. Tape B was recorded tonight at ") + Recorded + TEXT(". It ends where the pulse group ends.");
	return Doc;
}

TOptional<FS47Doc> FS47DecoderDesk::PaperById(const FString& Id)
{
	if (Id == TEXT("vegaPage")) return PageDoc();
	if (Id == TEXT("tapeA")) return TapeDoc();
	return {};
}

bool FS47DecoderDesk::PageThere() const
{
	return KitPtr.IsValid() && D.Locked && D.Locked() && !(D.HasPaper && D.HasPaper(TEXT("vegaPage")));
}

void FS47DecoderDesk::ReadPage()
{
	const FS47Doc Doc = PageDoc();
	const TWeakPtr<bool> Life = Alive;
	auto Filed = [this, Life, Doc]
	{
		if (!Life.IsValid() || (D.HasPaper && D.HasPaper(Doc.Id))) return;
		if (D.File) D.File(Doc, FString::Printf(TEXT("%s. Under the spare reel by the processor: a page out of a field book, in pencil. T. Vega, July 1947. He numbered the seven and wrote down what the line sang back to him. I do not remember it being there."), *ClockNow()));
		if (KitPtr) KitPtr->ShowPage(false);
		if (D.Save) D.Save();
	};
	if (US47UI* UI = D.UI.Get()) UI->Document(Doc, Filed); else Filed();
}

void FS47DecoderDesk::OnKey(int32 K)
{
	// a key: its note through the processor, and the last seven checked against his
	if (K < 1 || K > 7 || Answering() || (D.Busy && D.Busy())) return;
	KeyLog.Add(K);
	if (KeyLog.Num() > 7) KeyLog.RemoveAt(0);
	if (EnsurePlayer()) Player->Key(K - 1);
	if (IsAnswer(KeyLog)) BeginAnswer();
	DrawScreen();
}

void FS47DecoderDesk::BeginAnswer()
{
	KeyLog.Reset();
	Ans = FS47AnswerTimes();
	Ans.K = FS47AnswerTimes::KFor(P.Stretch);
	AnswerT = 0; AnswerStep = 0;
	bAnswerFirst = !(D.Answered && D.Answered());
	TArray<double> Seven;
	for (const int32 K : ANSWER()) Seven.Add(FS47DecoderPlayer::SevenDegree(K - 1));
	// his correction on the wire, the line's own recording (the receiver in the hut plays it, S47Chapter3);
	// without it the wire keeps the same time with nothing in it there
	US47Audio* A = D.Audio.Get();
	const TSharedPtr<FS47AudioBuffer> Voice = A ? A->LineBuffer(TEXT("THE LINE"), TEXT("Reference west. No. East. Hold the last reading.")) : nullptr;
	if (Voice.IsValid() && FMath::IsFinite(Voice->Duration()) && Voice->Duration() > 0.5) Ans.VoiceSecs = FMath::Min(double(Voice->Duration()), 8.0);
	if (EnsurePlayer()) Player->Phrase(Seven, Ans, Voice);
	UE_LOG(LogS47Desk, Display, TEXT("S47: decoder: the answer keyed; the processor gives it back (K %.1f, %.1f s)%s"), Ans.K, Ans.End(), bAnswerFirst ? TEXT(", the first time in this case") : TEXT(", given before: the music only"));
}

void FS47DecoderDesk::StepAnswer(double Dt)
{
	AnswerT += Dt;
	// a beat after the seventh key the panel goes by itself: the processor answers on its own
	if (AnswerStep == 0 && AnswerT >= Ans.Lead) { AnswerStep = 1; if (bPanelOpen && D.UI.IsValid()) D.UI->Close(); }
	if (!bAnswerFirst) { if (AnswerT >= Ans.End()) EndAnswer(false); return; }
	// the seven coming back: the red lamps out on the array blink as one
	if (AnswerStep == 1 && AnswerT >= Ans.Seven(0)) { AnswerStep = 2; if (D.Lamps) D.Lamps(true); UE_LOG(LogS47Desk, Display, TEXT("S47: decoder: the seven back; the array's lamps as one")); }
	// the last of them: the screens break up and the lights sag
	if (AnswerStep == 2 && AnswerT >= Ans.Last()) { AnswerStep = 3; if (D.React) D.React(1.4, 0.9, 0); UE_LOG(LogS47Desk, Display, TEXT("S47: decoder: the last note; the screens glitch, the lights dip")); }
	// the seven coming back on the wire from 1947, in the hut: the lights stutter with them
	if (AnswerStep == 3 && AnswerT >= Ans.Back(0)) { AnswerStep = 4; if (D.React) D.React(0.6, 0, 0.22 * Ans.K * 6 + 0.8); UE_LOG(LogS47Desk, Display, TEXT("S47: decoder: the seven back on the wire; the lights flicker")); }
	// the cut on the wire: the lamps let go, and the night outside goes as quiet as the wire (as in reel 047's hole)
	if (AnswerStep == 4 && AnswerT >= Ans.Cut()) { AnswerStep = 5; if (D.Lamps) D.Lamps(false); if (D.Hush) D.Hush(Ans.End() - Ans.Cut() + 2.5, 0.04, 0.05, 1.6); }
	if (AnswerStep == 5 && AnswerT >= Ans.End()) EndAnswer(true);
}

void FS47DecoderDesk::EndAnswer(bool bUnlock)
{
	AnswerT = -1; AnswerStep = 0;
	if (bUnlock && !(D.Answered && D.Answered()))
	{
		if (D.SetAnswered) D.SetAnswered();
		if (D.File) D.File(TapeDoc(D.Recorded ? D.Recorded() : FString(TEXT("02:14"))), FString::Printf(TEXT("%s. I keyed T. Vega's seven into the processor. It played them back by itself, slower, and the red lamps on the dishes blinked as one. Then it ran on past the end of tape B: a wire from 1947. Someone whistling the four and the seven down a field line, and the seven coming back. A field telephone, his correction, cutters. Nothing after. TAPE A."), *ClockNow()));
		if (US47UI* UI = D.UI.Get()) UI->Toast(TEXT("The processor ran on past the end of tape B. TAPE A / 1947 filed with the papers."), 4.6f);
		if (D.Save) D.Save();
		UE_LOG(LogS47Desk, Display, TEXT("S47: decoder: TAPE A / 1947 filed"));
	}
	DrawScreen();
}

void FS47DecoderDesk::AbortAnswer()
{
	// stopped before it was done (PLAY TAPE, STOP, reel 047, a new night): nothing comes of it
	if (AnswerT < 0) return;
	if (bAnswerFirst && AnswerStep >= 2 && AnswerStep < 5 && D.Lamps) D.Lamps(false);
	AnswerT = -1; AnswerStep = 0;
	if (Player.IsValid() && Player->bPhrase) Player->Stop();
}

void FS47DecoderDesk::TestAnswer(double Dt)
{
	// -S47Answer: three seconds after the lock (and the power) the panel is opened, and his seven keyed
	// one by one, seven tenths of a second apart
	if (!(D.Locked && D.Locked()) || !(D.Power && D.Power())) return;
	TestT += Dt;
	// =pagepanel: the page filed as reading it files it, and the panel opened (its ANSWER row's header)
	if (TestMode == TEXT("pagepanel"))
	{
		if (TestT >= 3)
		{
			bTestAnswer = false;
			const bool bThere = PageThere();
			if (bThere && D.File) { D.File(PageDoc(), TEXT("(test) T. Vega's page.")); if (KitPtr) KitPtr->ShowPage(false); }
			const FString Why = Open();
			UE_LOG(LogS47Desk, Display, TEXT("S47: -S47Answer=pagepanel: the page %s, panel %s"), bThere ? TEXT("filed") : TEXT("not there"), Why.IsEmpty() ? TEXT("open") : *Why);
		}
		return;
	}
	if (TestMode == TEXT("page"))
	{
		if (TestT >= 3) { bTestAnswer = false; UE_LOG(LogS47Desk, Display, TEXT("S47: -S47Answer=page: the page %s"), PageThere() ? TEXT("read") : TEXT("not there")); if (PageThere()) ReadPage(); }
		return;
	}
	if (TestK == 0 && TestT >= 3)
	{
		const FString Why = Open();
		UE_LOG(LogS47Desk, Display, TEXT("S47: -S47Answer: panel %s"), Why.IsEmpty() ? TEXT("open") : *(TEXT("refused: ") + Why));
		if (!Why.IsEmpty()) { TestT = 1; return; }
		TestK = 1;
		TestT = 3;
	}
	if (TestK >= 1 && TestK <= 7 && TestT >= 3 + 0.7 * TestK)
	{
		const int32 K = ANSWER()[TestK - 1];
		OnKey(K);
		UE_LOG(LogS47Desk, Display, TEXT("S47: -S47Answer: key %d (%d of 7)%s"), K, TestK, Answering() ? TEXT(": the answer") : TEXT(""));
		++TestK;
	}
	// =tape, =notebook: once TAPE A is filed, it is opened, or the notebook at its papers
	if (TestK > 7 && !Answering())
	{
		bTestAnswer = false;
		if (TestMode == TEXT("tape")) { if (US47UI* UI = D.UI.Get()) UI->Document(TapeDoc()); }
		else if (TestMode == TEXT("notebook")) { if (US47Main* M = US47Main::Get(D.Room.Get()); M && M->Flow) M->Flow->OpenTestView(TEXT("notebook:papers")); }
		UE_LOG(LogS47Desk, Display, TEXT("S47: -S47Answer: done, TAPE A %s"), D.Answered && D.Answered() ? TEXT("filed") : TEXT("not filed"));
	}
}

// ---------- tests ----------

bool FS47DecoderDesk::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&Lines, &bOk](bool b, const FString& What) { Lines.Add(FString::Printf(TEXT("%s decoder: %s"), b ? TEXT("PASS") : TEXT("FAIL"), *What)); bOk &= b; };
	bool bLocked = false, bPower = false, bBusy = false;
	FS47DecoderDeskDeps Dd;
	Dd.Locked = [&bLocked] { return bLocked; };
	Dd.Recorded = [] { return FString(TEXT("02:14")); };
	Dd.Busy = [&bBusy] { return bBusy; };
	Dd.Power = [&bPower] { return bPower; };
	FS47DecoderDesk Desk(Dd);
	Check(Desk.View().Tape == TEXT("TAPE B / 1420.405 MHz / PULSE GROUP 4 / 7 / RECORDED 02:14"), TEXT("the tape line"));
	Check(Desk.Params().Mode == TEXT("harmonized") && Desk.Params().Pitch == 0 && Desk.Params().Stretch == 2 && Desk.Params().Space == 0.6, TEXT("harmonized, +0, x2.0, 60% to begin with"));
	Check(Desk.Label() == TEXT("Signal processor") && !Desk.Playing(), TEXT("the label with nothing on the deck"));
	Check(Desk.ScreenLine() == TEXT("NO TAPE"), TEXT("NO TAPE before the lock"));
	Check(Desk.Open() == TEXT("No power to the processor. It runs off RX bank 3."), TEXT("no power: the toast, no panel"));
	bPower = true;
	Check(Desk.Open() == TEXT("The deck records whatever the receiver locks on. Nothing has been locked tonight."), TEXT("nothing locked: the toast"));
	bLocked = true;
	Check(Desk.Open() == TEXT("The processor needs sound. Tap the screen first."), TEXT("no sound: the toast"));
	bBusy = true;
	Check(Desk.Open() == TEXT("busy"), TEXT("busy (a cinematic): nothing at all"));
	bBusy = false;
	Check(Desk.ScreenLine() == TEXT("HARM +0ST X2.0"), TEXT("the display's mode line once locked"));
	Desk.OnMode(TEXT("raw")); Desk.OnParam(TEXT("pitch"), -3); Desk.OnParam(TEXT("stretch"), 4.5); Desk.OnParam(TEXT("space"), 0.25);
	Check(Desk.ScreenLine() == TEXT("RAW  -3ST X4.5") && Desk.View().Mode == TEXT("raw") && Desk.View().Space == 0.25, TEXT("the panel's mode and knobs set the parameters"));
	Desk.OnParam(TEXT("pitch"), 12);
	Check(Desk.ScreenLine() == TEXT("RAW  +12ST X4.5"), TEXT("+12 semitones"));
	Desk.OnPlay(true);
	Check(!Desk.Playing() && Desk.View().Current() == -1, TEXT("no sound: PLAY TAPE plays nothing"));
	bool bOwns = true;
	Desk.Reel.Label = [] { return FString(TEXT("Thread reel 047 on the deck")); };
	Desk.Reel.Owns = [&bOwns] { return bOwns; };
	Check(Desk.Label() == TEXT("Thread reel 047 on the deck"), TEXT("reel 047's label first"));
	Desk.Reel.Label = [] { return FString(); };
	Check(Desk.Label() == TEXT("Signal processor"), TEXT("the processor's own label when the reel has none"));
	int32 Used = 0;
	Desk.Reel.Use = [&Used] { ++Used; return true; };
	Desk.Use();
	Check(Used == 1, TEXT("the reel's use first"));
	Desk.Update(0.5);
	Desk.Reel = FReel();
	double X = 0, Hh = 0;
	S47Panels::DecoderScopeBar(0, TEXT("raw"), 120, X, Hh);
	Check(X == 30 && FMath::IsNearlyEqual(Hh, 52.8), TEXT("the scope's first bar, raw"));
	S47Panels::DecoderScopeBar(4, TEXT("harmonized"), 120, X, Hh);
	Check(X == 30 + 5.5 * 47 && FMath::IsNearlyEqual(Hh, 99.0), TEXT("the seven-group's first, harmonized: the octave over"));
	S47Panels::DecoderScopeBar(10, TEXT("harmonized"), 120, X, Hh);
	Check(X == 30 + 11.5 * 47 && FMath::IsNearlyEqual(Hh, 45.0), TEXT("the last bar, harmonized"));

	// ---------- (Unreal only) the answer ----------
	// the seven as the harmonized playback sounds them: key 1 the top (an octave over), key 7 the bottom
	Check(FS47DecoderPlayer::SevenDegree(0) == 12 && FS47DecoderPlayer::SevenDegree(6) == 0 && FS47DecoderPlayer::FourDegree(3) == 10, TEXT("the keys' notes: 1 an octave up, 7 the root"));
	// the sequence: the last seven keys, whatever came before; not the seven in order, nor reversed
	Check(IsAnswer({ 7, 5, 3, 2, 1, 4, 6 }) && IsAnswer({ 1, 1, 4, 7, 5, 3, 2, 1, 4, 6 }), TEXT("his seven, alone or after other keys"));
	Check(!IsAnswer({ 1, 2, 3, 4, 5, 6, 7 }) && !IsAnswer({ 7, 6, 5, 4, 3, 2, 1 }) && !IsAnswer({ 5, 3, 2, 1, 4, 6 }) && !IsAnswer({ 7, 5, 3, 2, 1, 4, 6, 6 }), TEXT("not in order, not reversed, not six of them, not with one after"));
	// played on a desk without sound: the timeline runs all the same; the room answers once, TAPE A filed
	// once with a note, and kept as answered; the keys are dead while it gives it back
	TArray<FString> Filed, NotesSeen;
	bool bAnswered = false;
	int32 Reacts = 0, Flicks = 0, LampsOn = 0, LampsOff = 0, Saves = 0, Hushes = 0;
	FS47DecoderDeskDeps Da = Dd;
	Da.HasPaper = [&Filed](const FString& Id) { return Filed.Contains(Id); };
	Da.File = [&Filed, &NotesSeen](const FS47Doc& Doc, const FString& N) { Filed.AddUnique(Doc.Id); NotesSeen.Add(N); };
	Da.Answered = [&bAnswered] { return bAnswered; };
	Da.SetAnswered = [&bAnswered] { bAnswered = true; };
	Da.Save = [&Saves] { ++Saves; };
	Da.React = [&Reacts, &Flicks](double, double Dip, double Flick) { if (Dip > 0) ++Reacts; if (Flick > 0) ++Flicks; };
	Da.Lamps = [&LampsOn, &LampsOff](bool bOn) { (bOn ? LampsOn : LampsOff)++; };
	Da.Hush = [&Hushes](double, double, double, double) { ++Hushes; };
	Da.Clock = [] { return FString(TEXT("02:31")); };
	FS47DecoderDesk Ad(Da);
	for (const int32 K : { 4, 4, 7, 5, 3, 2, 1, 4 }) Ad.OnKey(K);
	Check(!Ad.Answering() && Ad.Keys().Num() == 7 && Ad.Keys()[0] == 4 && Ad.Keys()[6] == 4, TEXT("the register keeps the last seven keys"));
	Ad.OnKey(5);
	Check(!Ad.Answering(), TEXT("7 5 3 2 1 4 and a wrong last key: nothing"));
	for (const int32 K : { 7, 5, 3, 2, 1, 4, 6 }) Ad.OnKey(K);
	Check(Ad.Answering() && Ad.Keys().Num() == 0 && Ad.ScreenLine() == TEXT("ANSWER"), TEXT("his seven: the processor gives it back, ANSWER on the display"));
	Ad.OnKey(3);
	Check(Ad.Keys().Num() == 0, TEXT("the keys are dead while it answers"));
	FS47AnswerTimes Tm; Tm.K = FS47AnswerTimes::KFor(2); // (the panel's TIME as it starts, x2.0)
	Check(Tm.Last() < Tm.Wire() && Tm.Whistle(3) < Tm.Whistle(4) - 0.5 && Tm.Whistle(10) < Tm.Back(0) && Tm.Back(6) < Tm.Crank() && Tm.Crank() + 3 < Tm.Voice() && Tm.Voice() + Tm.VoiceSecs < Tm.Cut() && Tm.Cut() < Tm.End(),
		FString::Printf(TEXT("the answer's timeline in order: the seven %.1f, the wire %.1f, the whistle, the seven back %.1f, the crank %.1f, his voice %.1f, the cut %.1f, the end %.1f s"), Tm.Seven(0), Tm.Wire(), Tm.Back(0), Tm.Crank(), Tm.Voice(), Tm.Cut(), Tm.End()));
	for (double T = 0; T < Tm.Wire() + 0.5; T += 0.05) Ad.Update(0.05);
	Check(LampsOn == 1 && Reacts == 1 && Flicks == 0 && LampsOff == 0 && Ad.ScreenLine() == TEXT("TAPE A 1947"), TEXT("the lamps as one with the seven, the room at the last note, then TAPE A on the display"));
	for (double T = Tm.Wire() + 0.5; T < Tm.Back(0) + 0.2; T += 0.05) Ad.Update(0.05);
	Check(Flicks == 1 && LampsOff == 0 && Hushes == 0, TEXT("the seven back on the wire: the lights flicker"));
	for (double T = 0; T < 30; T += 0.05) Ad.Update(0.05);
	Check(!Ad.Answering() && LampsOff == 1 && Hushes == 1 && bAnswered && Filed.Contains(TEXT("tapeA")) && Saves == 1 && NotesSeen.Num() == 1 && NotesSeen[0].StartsWith(TEXT("02:31. I keyed T. Vega's seven")), TEXT("then the cut (the lamps let go, the night hushed), TAPE A filed with its note, the case marked and saved"));
	for (const int32 K : { 7, 5, 3, 2, 1, 4, 6 }) Ad.OnKey(K);
	for (double T = 0; T < 45; T += 0.05) Ad.Update(0.05);
	Check(Reacts == 1 && Flicks == 1 && Hushes == 1 && LampsOn == 1 && Filed.Num() == 1 && Saves == 1, TEXT("once a case: given again, it is the music only"));
	// stopped halfway: nothing comes of it, the lamps let go
	bAnswered = false; Filed.Reset(); LampsOn = LampsOff = 0;
	for (const int32 K : { 7, 5, 3, 2, 1, 4, 6 }) Ad.OnKey(K);
	for (double T = 0; T < Tm.Seven(2); T += 0.05) Ad.Update(0.05);
	Ad.Stop();
	for (double T = 0; T < 30; T += 0.05) Ad.Update(0.05);
	Check(!Ad.Answering() && LampsOn == 1 && LampsOff == 1 && !bAnswered && Filed.Num() == 0, TEXT("stopped halfway: no TAPE A, the lamps back in their own blink"));
	// the papers by id, as the case puts them back; no page without the bought unit
	Check(PaperById(TEXT("vegaPage")).IsSet() && PaperById(TEXT("tapeA")).IsSet() && !PaperById(TEXT("binder")).IsSet() && !Ad.PageThere(), TEXT("the two papers by id; no page to find without the bought unit"));
	const int32 Words = [] { TArray<FString> W; FString(TAPE_A).ParseIntoArrayWS(W); return W.Num(); }();
	Check(Words >= 120 && Words <= 220 && FString(TAPE_A).Contains(TEXT("Reference west. No. East. Hold the last reading.")) && FString(TAPE_A).Contains(TEXT("02:17:47")), TEXT("TAPE A: its length, T. Vega's correction word for word, the relay at 02:17:47 (E10)"));
	Check(FString(VEGA_PAGE).Contains(TEXT("7  5  3  2  1  4  6")), TEXT("his page has the seven in the answer's order"));
	// the answer's sound rendered offline: it stays a sound to the end (nothing in it runs away), and it
	// is silent once the wire is cut
	{
		TArray<double> Levels;
		auto Render = [&Levels](TFunctionRef<void(FS47DecoderPlayer&, const TSharedRef<FS47AudioContext>&)> Setup, double Secs, double& OutMax, double& OutLast, FString& OutTrace)
		{
			const TSharedRef<FS47AudioContext> Ctx = FS47AudioContext::Make(48000.f);
			const TSharedRef<FS47AudioBuffer> Noise = Ctx->CreateBuffer(1, 48000 * 2, 48000.f);
			FRandomStream R(47);
			for (int32 I = 0; I < Noise->Length(); ++I) Noise->GetChannelData(0)[I] = R.FRand() * 2 - 1;
			FS47DecoderPlayer Pl(Ctx, Ctx->Destination, FVector::ZeroVector, Noise);
			Setup(Pl, Ctx);
			TArray<float> Buf;
			Buf.SetNumZeroed(12000 * 2);
			OutMax = 0; OutLast = 0; Levels.Reset();
			for (double T = 0; T < Secs; T += 0.25)
			{
				Ctx->Render(Buf.GetData(), 12000, 2);
				Pl.Update();
				OutLast = Pl.TakeLevel();
				OutMax = FMath::Max(OutMax, FMath::IsFinite(OutLast) ? OutLast : 1e30);
				Levels.Add(FMath::IsFinite(OutLast) ? OutLast : 1e30);
				OutTrace += FString::Printf(TEXT("%.3f "), OutLast);
			}
		};
		TArray<double> Seven;
		for (const int32 K : ANSWER()) Seven.Add(FS47DecoderPlayer::SevenDegree(K - 1));
		const FS47AnswerTimes At3 = [] { FS47AnswerTimes A; A.K = 3; return A; }();
		double Max = 0, Last = 0;
		FString Trace;
		Render([&Seven, &At3](FS47DecoderPlayer& Pl, const TSharedRef<FS47AudioContext>&) { Pl.Phrase(Seven, At3); }, At3.End() + 1, Max, Last, Trace);
		UE_LOG(LogS47Desk, Display, TEXT("S47: decoder self test: the answer's level every 0.25 s: %s"), *Trace);
		Check(Max > 0.01 && Max < 1.0, TEXT("the answer offline: heard, and nothing in it runs away"));
		// the seven back on the wire are heard, and after the cut there is nothing (the reverb's tail out)
		double Back = 0, After = 0;
		for (int32 I = 0; I < Levels.Num(); ++I)
		{
			const double T = I * 0.25;
			if (T > At3.Back(0) && T < At3.Back(6) + 0.5) Back = FMath::Max(Back, Levels[I]);
			if (T > At3.Cut() + 1.5) After = FMath::Max(After, Levels[I]);
		}
		Check(Back > 0.004 && After < 0.002, FString::Printf(TEXT("the wire offline: the seven back heard (%.4f), nothing after the cut (%.4f)"), Back, After));
		Trace.Reset();
		Render([](FS47DecoderPlayer& Pl, const TSharedRef<FS47AudioContext>&) { Pl.Key(0); }, 6, Max, Last, Trace);
		UE_LOG(LogS47Desk, Display, TEXT("S47: decoder self test: a key's level every 0.25 s: %s"), *Trace);
		Check(Max > 0.005 && Max < 1.0 && Last < 0.001, TEXT("a key offline: a note, its tail, then quiet"));
		// a key alone lights its bar while its note sounds (the panel's scope and the display), then not
		{
			const TSharedRef<FS47AudioContext> Ctx = FS47AudioContext::Make(48000.f);
			const TSharedRef<FS47AudioBuffer> Noise = Ctx->CreateBuffer(1, 4800, 48000.f);
			FS47DecoderPlayer Pl(Ctx, Ctx->Destination, FVector::ZeroVector, Noise);
			TArray<float> Buf;
			Buf.SetNumZeroed(12000 * 2);
			Pl.Key(2);
			Ctx->Render(Buf.GetData(), 12000, 2); Pl.Update();
			const int32 Lit = Pl.Current;
			Ctx->Render(Buf.GetData(), 12000, 2); Pl.Update();
			Check(Lit == 6 && Pl.Current == -1, FString::Printf(TEXT("a key lights its bar while it sounds (key 3: bar %d, then %d)"), Lit, Pl.Current));
		}
	}
	bOk &= FS47ConsoleKit::SelfTest(Lines);
	return bOk;
}

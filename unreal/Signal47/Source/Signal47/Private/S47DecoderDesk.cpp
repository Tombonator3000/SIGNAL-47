// story/Decoder.ts
#include "S47DecoderDesk.h"

#include "Brushes/SlateColorBrush.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "Rendering/DrawElements.h"
#include "RenderingThread.h"
#include "S47Audio.h"
#include "S47Inter.h"
#include "S47Json.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47Yard.h"
#include "Slate/WidgetRenderer.h"
#include "SS47Widgets.h"
#include "UObject/Package.h"
#include "Widgets/SLeafWidget.h"

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
	if (Inter && Room)
	{
		const TWeakPtr<bool> Life = Alive;
		FS47Interactable I;
		I.Id = TEXT("decoder"); I.Object = Room->Obj(TEXT("room.objs.decoder")); I.Range = 2.4;
		I.Label = [this, Life] { return Life.IsValid() ? Label() : FString(); };
		I.Use = [this, Life] { if (Life.IsValid()) Use(); };
		Inter->Add(I);
	}
	if (Room) for (const FString& H : Room->PathsUnder(TEXT("room.reels["))) Reels.Add(Room->Obj(H));
	InitScreen();
	DrawScreen();
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

bool FS47DecoderDesk::Playing() const { return Player.IsValid() && Player->bPlaying; }

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
	S47Panels::DecoderPanel(UI, O);
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

void FS47DecoderDesk::Play()
{
	US47Audio* A = D.Audio.Get();
	const TSharedPtr<FS47AudioBuffer> Noise = A ? A->NoiseBuffer() : nullptr;
	if (!A || !A->Ctx.IsValid() || !Noise.IsValid()) return;
	if (!Player.IsValid())
	{
		const FVector At = (D.Room.IsValid() ? D.Room->Obj(TEXT("room.objs.decoder")).WorldPosition() : FVector::ZeroVector) + FVector(-0.3, 1.1, 0);
		Player = MakeShared<FS47DecoderPlayer>(A->Ctx.ToSharedRef(), A->SfxBus, At, Noise);
	}
	FS47Sound C; C.Gain = 0.5f; A->Sfx(TEXT("click"), C);
	FS47DecoderSet S; S.Mode = P.Mode; S.Pitch = P.Pitch; S.Stretch = P.Stretch; S.Space = P.Space;
	Player->Start(S);
}

void FS47DecoderDesk::Stop()
{
	if (!Playing()) return;
	if (US47Audio* A = D.Audio.Get()) { FS47Sound C; C.Gain = 0.4f; A->Sfx(TEXT("click"), C); }
	Player->Stop();
	DrawScreen();
}

// ---------- per frame ----------

void FS47DecoderDesk::Update(double Dt)
{
	if (Player.IsValid()) Player->Update();
	if (Reel.Owns && Reel.Owns()) return;   // (reel 047 is on the deck: it has the display and the reels)
	if (Playing())
	{
		Spin += Dt * (P.Mode == TEXT("raw") ? 2.4 : 1.6) / P.Stretch;
		for (int32 I = 0; I < Reels.Num(); ++I) Reels[I].SetRotX(Spin * (I ? 0.8 : 1.15));
	}
	ScrT -= Dt;
	if (ScrT <= 0 && (Playing() || ScrT < -1)) { ScrT = 1.0 / 12; DrawScreen(); }
}

FString FS47DecoderDesk::ScreenLine() const
{
	if (!(D.Locked && D.Locked())) return TEXT("NO TAPE");
	return FString::Printf(TEXT("%s %s%sST X%.1f"), P.Mode == TEXT("raw") ? TEXT("RAW ") : TEXT("HARM"), P.Pitch >= 0 ? TEXT("+") : TEXT(""), *FString::SanitizeFloat(P.Pitch, 0), P.Stretch);
}

// The display on the processor: mode, settings, and the pulses as they pass.
void FS47DecoderDesk::DrawScreen()
{
	if (!Renderer || !Widget.IsValid() || !Target.IsValid() || !FSlateApplication::IsInitialized()) return;
	FS47DeskPicture Pic;
	Pic.bLocked = D.Locked && D.Locked();
	Pic.Line = ScreenLine();
	Pic.Cur = Playing() ? Player->Current : -1;
	Widget->P = Pic;
	Renderer->DrawWidget(Target.Get(), Widget.ToSharedRef(), FVector2D(256, 72), 0.f);
	// (one canvas in the web version, drawn by whichever draws last: here the screen shows this target
	// again once reel 047 has shown its own on it)
	AS47Web* W = D.Room.Get();
	UMaterialInstanceDynamic* Mid = W && Mat >= 0 ? W->SharedMid(Mat) : nullptr;
	if (Mid && Mid->K2_GetTextureParameterValue(TEXT("Map")) != Target.Get()) FS47Mat{ W, Mat }.SetMap(Target.Get(), true);
}

// ---------- tests ----------

bool FS47DecoderDesk::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&Lines, &bOk](bool b, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s decoder: %s"), b ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= b; };
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
	return bOk;
}

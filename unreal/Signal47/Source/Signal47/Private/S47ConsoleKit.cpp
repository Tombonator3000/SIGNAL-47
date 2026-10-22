// (Unreal only, on Tom's request) the bought unit as the signal processor: S47ConsoleKit.h
#include "S47ConsoleKit.h"

#include "Brushes/SlateColorBrush.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "RenderingThread.h"
#include "S47Figures.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Slate/WidgetRenderer.h"
#include "SS47Widgets.h"
#include "UObject/Package.h"
#include "Widgets/SLeafWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Kit, Log, All);

namespace
{
	// world/ControlRoom.ts placeModels: the desks' top T; the signal analyzer at (2.95, T, -3.95), the
	// VU unit on it at (2.95, T + 0.147, -3.955). console_kit.glb extras, in each part's own terms
	// (origin under the middle of its foot, front to +z): the analyzer's display, the two meters'
	// windows (their visible parts). In the room's terms, as the merged mesh is.
	const double T = 0.78;
	const FVector ANALYZER(2.95, T, -3.95), VU_UNIT(2.95, T + 0.147, -3.955);
	const FVector DISPLAY(-0.0582, 0.1033, 0.097);
	const double DISPLAY_W = 0.1802, DISPLAY_H = 0.0396;
	const FVector METERS[2] = { FVector(-0.0806, 0.1791, 0.0801), FVector(0.0809, 0.1791, 0.0801) };
	const double METER_W = 0.1165, METER_H = 0.0761;
	// world/dressing.ts dressControlRoom: the spare reel beside the unit, c.reel(3.32, T, -3.66, 0.12)
	const FVector REEL(3.32, T, -3.66);
	// the homemade deck's place on the floor by RX bank 3 (ControlRoom.ts eastWall: this.col(5.4, 6, -1.92, -1.18))
	const double DECK_COL[4] = { 5.4, 6, -1.92, -1.18 };
	constexpr int32 FACE_W = 192, FACE_H = 125, PAGE_W = 240, PAGE_H = 336;
	// a lift off each surface, so the quads sit over the export's faces and not in them
	constexpr double LIFT = 0.0012;

	// colours as the canvases write them, linear, into an sRGB target (as the processor's display)
	FLinearColor Css(uint32 C, float A = 1.f) { FLinearColor L = FLinearColor::FromSRGBColor(FColor((C >> 16) & 255, (C >> 8) & 255, C & 255)); L.A = A; return L; }
}

// ---------- the needles ----------

void FS47VuNeedle::Step(double Target, double Dt)
{
	// x'' = w^2 (target - x) - 2 z w x', in small steps so a long frame cannot shake it loose
	const int32 N = FMath::Clamp(FMath::CeilToInt(Dt * 480.0), 1, 240);
	const double H = Dt / N;
	for (int32 I = 0; I < N; ++I)
	{
		const double A = Omega * Omega * (Target - X) - 2 * Zeta * Omega * V;
		V += A * H;
		X += V * H;
	}
	// the pins at either end of the scale
	if (X < -0.02) { X = -0.02; V = FMath::Max(0.0, V); }
	if (X > 1.06) { X = 1.06; V = FMath::Min(0.0, V); }
}

double FS47VuNeedle::Mark(double Vu) { return FMath::Pow(10.0, Vu / 20.0) / FMath::Pow(10.0, 3.0 / 20.0); }
double FS47VuNeedle::Target(double Rms, double Ref) { return Ref > 0 ? FMath::Clamp(Rms / Ref * Mark(0), 0.0, 1.1) : 0.0; }

// ---------- a meter's face ----------

/** A VU meter's face as the unit's are (amber paper lit from behind, the scale from -20 to +3 with the
 *  red over 0, VU under it) with its needle at X, its pivot under the window. */
class SS47VuFace : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47VuFace) {}
	SLATE_END_ARGS()
	void Construct(const FArguments&) { White = MakeUnique<FSlateColorBrush>(FLinearColor::White); }
	double X = 0;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(FACE_W, FACE_H); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
	{
		const float W = FACE_W, H = FACE_H;
		auto Box = [&](float X0, float Y0, float Bw, float Bh, const FLinearColor& C, int32 L) { FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(FVector2f(Bw, Bh), FSlateLayoutTransform(FVector2f(X0, Y0))), White.Get(), ESlateDrawEffect::None, C); };
		auto Line = [&](TArray<FVector2f> P, const FLinearColor& C, float Th, int32 L) { FSlateDrawElement::MakeLines(Out, L, G.ToPaintGeometry(), MoveTemp(P), ESlateDrawEffect::None, C, true, Th); };
		const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		auto Txt = [&](const FString& S, float Cx, float Y0, const FSlateFontInfo& F, const FLinearColor& C, int32 L)
		{
			const FVector2D Sz = Measure->Measure(S, F);
			FSlateDrawElement::MakeText(Out, L, G.ToPaintGeometry(FVector2f(float(Sz.X) + 2.f, float(Sz.Y)), FSlateLayoutTransform(FVector2f(Cx - float(Sz.X) / 2, Y0))), S, F, ESlateDrawEffect::None, C);
		};
		// the paper, lit from behind: warm and bright where the lamp is, browner to the bezel
		const int32 Bands = 25;
		for (int32 I = 0; I < Bands; ++I)
		{
			const float V = (I + 0.5f) / Bands;
			const float K = FMath::Clamp(1.f - FMath::Square((V - 0.45f) / 0.62f), 0.f, 1.f);
			Box(0, H * I / Bands, W, H / Bands + 1, FMath::Lerp(Css(0x8f531c), Css(0xf6d394), K), Layer);
		}
		for (int32 I = 0; I < 10; ++I)
		{
			const FLinearColor Shade(0.10f, 0.04f, 0.01f, 0.3f * (1 - I / 10.f));
			Box(I * 2.5f, 0, 2.5f, H, Shade, Layer + 1);
			Box(W - (I + 1) * 2.5f, 0, 2.5f, H, Shade, Layer + 1);
		}
		// the scale: an arc round the pivot under the window, -20 at the left, +3 at the right
		const FVector2f Pivot(W / 2, H * 1.12f);
		const float R = H * 0.9f, Span = FMath::DegreesToRadians(48.f);
		auto At = [&](double F, float Rr) { const float A = float(-Span + 2 * Span * F); return Pivot + FVector2f(FMath::Sin(A), -FMath::Cos(A)) * Rr; };
		const FLinearColor Ink = Css(0x24170c), Red = Css(0xb3261e);
		TArray<FVector2f> Arc, RedArc;
		for (int32 I = 0; I <= 40; ++I) { const double F = I / 40.0; (F <= FS47VuNeedle::Mark(0) + 0.001 ? Arc : RedArc).Add(At(F, R)); }
		RedArc.Insert(At(FS47VuNeedle::Mark(0), R), 0);
		Line(Arc, Ink, 1.3f, Layer + 2);
		TArray<FVector2f> RedBand;
		for (int32 I = 0; I <= 12; ++I) RedBand.Add(At(FS47VuNeedle::Mark(0) + (1 - FS47VuNeedle::Mark(0)) * I / 12.0, R + 3));
		Line(RedArc, Red, 1.3f, Layer + 2);
		Line(RedBand, Red, 4.f, Layer + 2);
		const struct { double Vu; const TCHAR* Label; } Marks[] = {
			{ -20, TEXT("20") }, { -10, TEXT("10") }, { -7, TEXT("7") }, { -5, TEXT("5") }, { -3, TEXT("3") }, { -2, nullptr }, { -1, nullptr },
			{ 0, TEXT("0") }, { 1, nullptr }, { 2, nullptr }, { 3, TEXT("3") } };
		const FSlateFontInfo Small = S47Style::Mono(13.f);
		for (const auto& Mk : Marks)
		{
			const double F = FS47VuNeedle::Mark(Mk.Vu);
			const FLinearColor C = Mk.Vu > 0 ? Red : Ink;
			Line({ At(F, R), At(F, R + (Mk.Label ? 8.f : 5.f)) }, C, 1.3f, Layer + 2);
			if (Mk.Label) { const FVector2f P = At(F, R + 17.f); Txt(Mk.Label, P.X, P.Y - 8.f, Small, C, Layer + 3); }
		}
		Txt(TEXT("-"), At(0, R + 16.f).X - 9.f, At(0, R + 16.f).Y - 9.f, Small, Ink, Layer + 3);
		Txt(TEXT("+"), At(1, R + 16.f).X + 10.f, At(1, R + 16.f).Y - 9.f, Small, Red, Layer + 3);
		Txt(TEXT("VU"), W / 2, H * 0.6f, S47Style::Display(19.f), Ink, Layer + 3);
		// the needle, from under the window across the scale
		// (drawn thick enough to read across the room: the face is a hand's width)
		const FVector2f Tip = At(FMath::Clamp(X, -0.03, 1.07), R + 5.f);
		Line({ Pivot + FVector2f(2.f, 1.f), Tip + FVector2f(2.f, 1.f) }, FLinearColor(0.25f, 0.12f, 0.03f, 0.35f), 3.f, Layer + 4);
		Line({ Pivot, Tip }, FLinearColor(0.012f, 0.01f, 0.008f, 1.f), 3.f, Layer + 4);
		// the bezel's shadow along the bottom, where the pivot hides
		for (int32 I = 0; I < 6; ++I) Box(0, H - (I + 1) * 2.f, W, 2.f, FLinearColor(0.06f, 0.025f, 0.005f, 0.42f * (1 - I / 6.f)), Layer + 5);
		return Layer + 6;
	}

private:
	TUniquePtr<FSlateBrush> White;
};

// ---------- T. Vega's page ----------

/** A page torn out of a field book (squared, the torn edge on the left), folded twice and flattened
 *  again, in pencil: a few lines, and the seven numbers bigger than the rest. */
class SS47TornPage : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47TornPage) {}
	SLATE_END_ARGS()
	void Construct(const FArguments&) { White = MakeUnique<FSlateColorBrush>(FLinearColor::White); }
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(PAGE_W, PAGE_H); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
	{
		const float W = PAGE_W, H = PAGE_H;
		auto Box = [&](float X0, float Y0, float Bw, float Bh, const FLinearColor& C, int32 L) { FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(FVector2f(Bw, Bh), FSlateLayoutTransform(FVector2f(X0, Y0))), White.Get(), ESlateDrawEffect::None, C); };
		auto Txt = [&](const FString& S, float X0, float Y0, float Px, const FLinearColor& C) { FSlateDrawElement::MakeText(Out, Layer + 3, G.ToPaintGeometry(FVector2f(W, Px * 1.3f), FSlateLayoutTransform(FVector2f(X0, Y0))), S, S47Style::Hand(Px), ESlateDrawEffect::None, C); };
		Box(0, 0, W, H, Css(0xe4d9be), Layer);
		// the squares, faint blue-grey
		for (float Y = 18; Y < H; Y += 15) Box(0, Y, W, 1, Css(0x9fb0b8, 0.45f), Layer + 1);
		for (float X = 22; X < W; X += 15) Box(X, 0, 1, H, Css(0x9fb0b8, 0.3f), Layer + 1);
		// the folds, a shade darker
		Box(0, H / 3, W, 2, Css(0x8a7d62, 0.35f), Layer + 2);
		Box(0, 2 * H / 3, W, 2, Css(0x8a7d62, 0.35f), Layer + 2);
		// the torn edge: bites out of the left side, the desk's dark through them
		for (int32 I = 0; I < 34; ++I)
		{
			const float Y = I * H / 34.f, D = 3.f + 4.f * FMath::Abs(FMath::Sin(I * 2.3f) + 0.5f * FMath::Sin(I * 5.1f));
			Box(0, Y, D, H / 34.f + 1, Css(0x2a2622), Layer + 2);
		}
		const FLinearColor Pencil = Css(0x4a464a, 0.92f);
		Txt(TEXT("Jul --  hut rcvr"), 18, 10, 26.f, Pencil);
		Txt(TEXT("4 up.  7 down, a step"), 18, 46, 24.f, Pencil);
		Txt(TEXT("at a time.  1 top, 7 bottom"), 18, 74, 24.f, Pencil);
		Txt(TEXT("whistled the 4 back."), 18, 112, 24.f, Pencil);
		Txt(TEXT("it sang back:"), 18, 140, 24.f, Pencil);
		Txt(TEXT("7 5 3 2 1 4 6"), 26, 180, 40.f, Css(0x353236, 0.95f));
		Txt(TEXT("twice. same."), 18, 244, 24.f, Pencil);
		Txt(TEXT("T.V."), 160, 290, 26.f, Pencil);
		return Layer + 4;
	}

private:
	TUniquePtr<FSlateBrush> White;
};

// ---------- the unit ----------

int32 FS47ConsoleKit::Find(AS47Web* W)
{
	// world/ControlRoom.ts placeModels: add(L.merged('console_kit', ...), g), named 'lic:console_kit' by
	// the export, in the room (the group room.objs.decoder hangs in)
	if (!W) return -1;
	const FS47Obj Room = W->Obj(TEXT("room.objs.decoder")).Parent();
	const FS47Obj Kit = Room ? Room.Find(TEXT("lic:console_kit")) : FS47Obj();
	return Kit ? Kit.I : -1;
}

FS47ConsoleKit::FS47ConsoleKit(AS47Web* InWeb) : Web(InWeb)
{
	Node = Find(InWeb);
	if (Node < 0)
	{
		UE_LOG(LogS47Kit, Display, TEXT("S47: console kit: no bought unit in the export; the homemade deck on the rack stays the signal processor"));
		return;
	}
	TakeAwayTheDeck();
	if (FApp::CanEverRender()) Build();
	UE_LOG(LogS47Kit, Display, TEXT("S47: console kit: the bought unit (node %d) is the signal processor; the deck on the rack taken away"), Node);
}

FS47ConsoleKit::~FS47ConsoleKit()
{
	if (Renderer) { BeginCleanup(Renderer); Renderer = nullptr; }
	Vu.Reset();
	PageW.Reset();
}

FS47Obj FS47ConsoleKit::Obj() const { return Node >= 0 && Web.IsValid() ? Web->ObjAt(Node) : FS47Obj(); }
FS47Obj FS47ConsoleKit::Page() const { return PageNode >= 0 && Web.IsValid() ? Web->ObjAt(PageNode) : FS47Obj(); }
FVector FS47ConsoleKit::SoundAt() const { const FS47Obj O = Obj(); const FVector P(2.95, T + 0.2, -3.84); return O ? O.LocalToWorld(P) : P; }
FVector FS47ConsoleKit::At() const { const FS47Obj O = Obj(); const FVector P(2.95, T + 0.2, -3.95); return O ? O.LocalToWorld(P) : P; }

void FS47ConsoleKit::TakeAwayTheDeck()
{
	// the homemade deck on the rack (room.objs.decoder: the cabinet, the processor's front, its display
	// room.dspScreen and the deck with room.reels) is not drawn, nothing traces against it, and the
	// floor where it stood is free to walk
	AS47Web* W = Web.Get();
	FS47Obj Deck = W->Obj(TEXT("room.objs.decoder"));
	Deck.Traverse([W](FS47Obj O)
	{
		const FS47WebNode* N = W->Node(O.I);
		if (UPrimitiveComponent* P = N ? Cast<UPrimitiveComponent>(N->Comp.Get()) : nullptr) P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	});
	Deck.SetVisible(false);
	TArray<FS47Rect>& Cols = W->CollidersMutable(TEXT("saro"));
	const int32 Before = Cols.Num();
	Cols.RemoveAll([](const FS47Rect& R)
	{
		return FMath::IsNearlyEqual(R.MinX, DECK_COL[0], 0.01) && FMath::IsNearlyEqual(R.MaxX, DECK_COL[1], 0.01) && FMath::IsNearlyEqual(R.MinZ, DECK_COL[2], 0.01) && FMath::IsNearlyEqual(R.MaxZ, DECK_COL[3], 0.01);
	});
	UE_LOG(LogS47Kit, Display, TEXT("S47: console kit: the deck's collider %s"), Cols.Num() < Before ? TEXT("taken out") : TEXT("not found"));
}

void FS47ConsoleKit::Build()
{
	AS47Web* W = Web.Get();
	const FS47WebNode* N = W->Node(Node);
	USceneComponent* Parent = N ? N->Comp.Get() : nullptr;
	if (!Parent) return;
	// (the parts hang under the unit's own mesh, in its terms: the room's, as the merged mesh has them)
	G = MakeUnique<FS47Group>(W, TEXT("ConsoleKit"), Parent);
	// the display: the dark glass the width of the analyzer's display, and the canvas's picture on it at
	// its own shape (256 x 72), unlit and not tone mapped (room.dspScreen: MeshBasicMaterial, toneMapped false)
	const FVector Disp = ANALYZER + DISPLAY;
	if (UMaterialInstanceDynamic* Glass = G->Basic(Css(0x031008)))
	{
		Glass->SetScalarParameterValue(TEXT("Untonemapped"), 1.f);
		G->Plane(nullptr, DISPLAY_W, DISPLAY_H, Glass, Disp + FVector(0, 0, LIFT));
	}
	if (UMaterialInstanceDynamic* Pic = G->Basic(FLinearColor::White))
	{
		Pic->SetScalarParameterValue(TEXT("Untonemapped"), 1.f);
		PicMid.Reset(Pic);
		G->Plane(nullptr, DISPLAY_H * 256.0 / 72.0, DISPLAY_H, Pic, Disp + FVector(0, 0, 2 * LIFT));
	}
	// the meters: a face each, in a render target, lit from behind as the export's faces are (unlit,
	// through the tone curve as the emissive faces go)
	for (int32 I = 0; I < 2; ++I)
	{
		UTextureRenderTarget2D* Rt = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
		Rt->ClearColor = FLinearColor(0.6f, 0.35f, 0.1f);
		Rt->InitCustomFormat(FACE_W, FACE_H, PF_B8G8R8A8, false);
		Rt->UpdateResourceImmediate(true);
		Face[I].Reset(Rt);
		if (UMaterialInstanceDynamic* M = G->Basic(FLinearColor::White))
		{
			S47Figures::SetMap(M, Rt);
			G->Plane(nullptr, METER_W, METER_H, M, VU_UNIT + METERS[I] + FVector(0, 0, LIFT));
		}
	}
	G->AddToScene();
	Renderer = new FWidgetRenderer(false);
	Vu = SNew(SS47VuFace);

	// T. Vega's page, half under the spare reel on the desk, lying flat, its top to the window: lit by
	// the room as the desk is; taken in as a node under the unit so the things to use can name it
	UTextureRenderTarget2D* Pt = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
	Pt->ClearColor = Css(0xe4d9be);
	Pt->InitCustomFormat(PAGE_W, PAGE_H, PF_B8G8R8A8, false);
	Pt->UpdateResourceImmediate(true);
	PageTex.Reset(Pt);
	PageW = SNew(SS47TornPage);
	PageG = MakeUnique<FS47Group>(W, TEXT("VegaPage"), Parent);
	UMaterialInstanceDynamic* Pm = PageG->Basic(FLinearColor::White, TEXT("M_Web_Lit_Opaque"));
	if (Pm)
	{
		S47Figures::SetMap(Pm, Pt);
		Pm->SetScalarParameterValue(TEXT("Roughness"), 0.92f);
	}
	FS47Node* Holder = PageG->Plane(nullptr, 0.10, 0.14, Pm, REEL + FVector(0.108, 0.0012, 0.072), FVector(-UE_DOUBLE_HALF_PI, 0, -0.55));
	PageG->AddToScene();
	USceneComponent* Hc = Holder ? Holder->C.Get() : nullptr;
	UPrimitiveComponent* Mesh = Hc && Hc->GetNumChildrenComponents() > 0 ? Cast<UPrimitiveComponent>(Hc->GetChildComponent(0)) : nullptr;
	if (Mesh) PageNode = W->AdoptNode(Mesh, Node, TEXT("s47:vegaPage"));
	ShowPage(false);
	DrawFaces(true);
}

void FS47ConsoleKit::ShowPage(bool bOn)
{
	FS47Obj P = Page();
	if (!P) return;
	if (P.Visible() != bOn) P.SetVisible(bOn);
	const FS47WebNode* N = Web.IsValid() ? Web->Node(PageNode) : nullptr;
	if (UPrimitiveComponent* C = N ? Cast<UPrimitiveComponent>(N->Comp.Get()) : nullptr)
	{
		// (not in the way of the use trace while it is not there)
		const ECollisionResponse R = bOn ? ECR_Block : ECR_Ignore;
		if (C->GetCollisionResponseToChannel(ECC_GameTraceChannel1) != R) C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, R);
	}
}

void FS47ConsoleKit::Update(double Dt, double Level, UTexture* Display)
{
	if (!IsValid()) return;
	// the needles: the left one reads the level, the right one a hair lower and a hair slower (two
	// meters on one source are never quite the same)
	Left.Step(FS47VuNeedle::Target(Level, REF), Dt);
	Right.Omega = 12.2;
	Right.Step(FS47VuNeedle::Target(Level * 0.94, REF), Dt);
	if (!G.IsValid()) return;
	// drawn with the room (the parts are not the export's: they follow its visibility here)
	const bool bOn = Obj().Shown();
	if (bOn != bShown) { bShown = bOn; G->SetVisible(bOn); }
	if (!bOn) return;
	// the display shows whatever the canvas shows now
	if (PicMid.IsValid() && Display && Shown.Get() != Display) { S47Figures::SetMap(PicMid.Get(), Display); Shown = Display; }
	if (!bPageDrawn) DrawPage();
	DrawT -= Dt;
	if (DrawT <= 0) { DrawT = 1.0 / 40; DrawFaces(false); }
}

void FS47ConsoleKit::DrawFaces(bool bAll)
{
	if (!Renderer || !Vu.IsValid() || !FSlateApplication::IsInitialized()) return;
	for (int32 I = 0; I < 2; ++I)
	{
		const double X = I == 0 ? Left.X : Right.X;
		if (!Face[I].IsValid() || (!bAll && FMath::Abs(X - Drawn[I]) < 0.0015)) continue;
		Vu->X = X;
		Renderer->DrawWidget(Face[I].Get(), Vu.ToSharedRef(), FVector2D(FACE_W, FACE_H), 0.f);
		Drawn[I] = X;
	}
}

void FS47ConsoleKit::DrawPage()
{
	if (!Renderer || !PageW.IsValid() || !PageTex.IsValid() || !FSlateApplication::IsInitialized()) return;
	Renderer->DrawWidget(PageTex.Get(), PageW.ToSharedRef(), FVector2D(PAGE_W, PAGE_H), 0.f);
	bPageDrawn = true;
}

// ---------- tests ----------

bool FS47ConsoleKit::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&Lines, &bOk](bool b, const FString& What) { Lines.Add(FString::Printf(TEXT("%s console kit: %s"), b ? TEXT("PASS") : TEXT("FAIL"), *What)); bOk &= b; };
	// a step to 0 VU: 99 % of it in 0.3 s (IEC 60268-17), a little past it and back, never far past
	FS47VuNeedle N;
	const double Goal = FS47VuNeedle::Mark(0);
	double Max = 0, At99 = -1;
	for (int32 I = 1; I <= 600; ++I)
	{
		N.Step(Goal, 1.0 / 600);
		Max = FMath::Max(Max, N.X);
		if (At99 < 0 && N.X >= 0.99 * Goal) At99 = I / 600.0;
	}
	Check(At99 > 0.27 && At99 < 0.33, FString::Printf(TEXT("99 %% of a step in %.3f s (0.3)"), At99));
	Check(Max > Goal * 1.005 && Max < Goal * 1.025, FString::Printf(TEXT("over by %.1f %% (1 to 2)"), (Max / Goal - 1) * 100));
	// the same in two long frames as in many short ones (the steps inside a frame)
	FS47VuNeedle A, B;
	for (int32 I = 0; I < 30; ++I) A.Step(Goal, 0.01);
	B.Step(Goal, 0.15); B.Step(Goal, 0.15);
	Check(FMath::Abs(A.X - B.X) < 0.01, TEXT("the movement does not depend on the frame rate"));
	// the scale: 0 VU at 1 / 1.413 of the way, -20 near the left, +3 at the end; REF reads 0
	Check(FMath::IsNearlyEqual(FS47VuNeedle::Mark(0), 0.7079, 0.001) && FMath::IsNearlyEqual(FS47VuNeedle::Mark(3), 1.0, 1e-9) && FS47VuNeedle::Mark(-20) < 0.08, TEXT("the marks: 0 VU at 0.708, +3 at the end, -20 near the stop"));
	Check(FMath::IsNearlyEqual(FS47VuNeedle::Target(REF, REF), Goal, 1e-9) && FS47VuNeedle::Target(0, REF) == 0 && FS47VuNeedle::Target(1, REF) <= 1.1, TEXT("the level as the needle's target: REF at 0 VU, silence at the stop, pinned past +3"));
	// back to the stop when the sound goes, and never through it
	for (int32 I = 0; I < 100; ++I) N.Step(0, 0.01);
	Check(N.X >= -0.02 && N.X < 0.01, TEXT("back to the stop in silence"));
	// no unit without a world: nothing drawn, nothing found
	FS47ConsoleKit None(nullptr);
	Check(!None.IsValid() && FS47ConsoleKit::Find(nullptr) == -1 && !None.Page(), TEXT("without the export's unit: none (the homemade deck stays)"));
	return bOk;
}

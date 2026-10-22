// ui/MapOverlay.ts
#include "SS47MapOverlay.h"

#include "Engine/Texture2D.h"
#include "S47Theme.h"
#include "Rendering/DrawElements.h"
#include "S47Canvas.h"
#include "S47Drawings.h"
#include "S47Image.h"
#include "S47Kit.h"
#include "S47UI.h"
#include "SS47Widgets.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// sizes: the web version's CSS pixels times this, as S47Kit's
	constexpr float PX = 1.25f;
	// .mapov canvas: as wide as the panel, but no wider than (100vh - 300px) * 1.5, so the question
	// and its answers stay in sight under it: 900 here (the panel is 1040 wide)
	constexpr float MAP_W = 900.f;

	// The two pictures, made once a session: the service map on its paper, and the tracing on a sheet
	// of its own with a little room round it. (The web version draws both on its canvas again at every
	// move of the pointer: drawImage(base), then drawTracing. Here the tracing is laid over the map
	// where it lies as Slate draws the panel, so a drag does not wait for its letters to be drawn again.)
	constexpr int32 TPAD = 4;
	constexpr int32 TW = 620 + 2 * TPAD, TH = 300 + 2 * TPAD;
	constexpr double TX = 70 + TPAD, TY = 150 + TPAD; // the transit, on the tracing's own picture

	UTexture2D* MapTexture()
	{
		static TWeakObjectPtr<UTexture2D> T;
		if (!T.IsValid())
		{
			const S47Drawings::FServiceMap& Mp = S47Drawings::SERVICE_MAP();
			FS47Canvas G(Mp.W, Mp.H);
			S47Drawings::ServiceMapCanvas(G);
			T = S47Image::FromPixels(G.Px, G.W, G.H);
		}
		return T.Get();
	}

	UTexture2D* TracingTexture()
	{
		static TWeakObjectPtr<UTexture2D> T;
		if (!T.IsValid())
		{
			FS47Canvas G(TW, TH);
			S47Drawings::DrawTracing(G, TX, TY);
			// (the clear room round the sheet takes the onion-skin's colour, so the filtering at its edge
			// does not darken it)
			for (FColor& C : G.Px) if (C.A == 0) C = FColor(214, 226, 232, 0);
			T = S47Image::FromPixels(G.Px, G.W, G.H);
		}
		return T.Get();
	}
}

// The tracing where it lies and the drag (mapOverlay's x, y, grab, aligned, draw, drop), in the
// map's own coordinates (1200 x 800).
struct FS47MapDrag
{
	double X = 0, Y = 0;
	TOptional<FVector2D> Grab;
	FString Status;           // [data-status]
	bool bAskHidden = false;  // [data-ask] hidden
	FString Say;              // [data-say]
	int32 Tone = 0;           // good 1, bad -1

	bool Aligned() const
	{
		const FVector2D& S = S47Drawings::SERVICE_MAP().Station;
		return FMath::Sqrt(FMath::Square(X - S.X) + FMath::Square(Y - S.Y)) < 0.5;
	}
	// draw(): the canvas is drawn by the widget each frame; the status line goes with it
	void Draw()
	{
		Status = Aligned()
			? TEXT("The tracing's transit sits on STATION 01, north to north. C runs east across the map.")
			: TEXT("Drag the tracing until the transit lies on STATION 01.");
	}
	// pointerdown: the tracing sheet is 620 by 300 with the transit 70 in from its left edge
	bool Down(double Px, double Py)
	{
		if (Px < X - 70 || Px > X + 550 || Py < Y - 150 || Py > Y + 150) return false;
		Grab = FVector2D(Px - X, Py - Y);
		return true;
	}
	// pointermove
	bool Move(double Px, double Py)
	{
		if (!Grab) return false;
		const S47Drawings::FServiceMap& Mp = S47Drawings::SERVICE_MAP();
		X = FMath::Min(Mp.W - 40.0, FMath::Max(40.0, Px - Grab->X));
		Y = FMath::Min(Mp.H - 40.0, FMath::Max(40.0, Py - Grab->Y));
		Draw();
		return true;
	}
	// drop (pointerup, pointercancel): true when it was being dragged
	bool Drop()
	{
		if (!Grab) return false;
		Grab.Reset();
		const S47Drawings::FServiceMap& Mp = S47Drawings::SERVICE_MAP();
		if (FMath::Sqrt(FMath::Square(X - Mp.Station.X) + FMath::Square(Y - Mp.Station.Y)) < Mp.Snap) { X = Mp.Station.X; Y = Mp.Station.Y; }
		Draw();
		return true;
	}
};

// <div class="mapov"><canvas data-map></canvas></div>: the map with the tracing on it, dragged with
// the mouse (the page's pointer events; there is no touch input in this version).
class SS47MapCanvas : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47MapCanvas) {}
		SLATE_ARGUMENT(TSharedPtr<FS47MapDrag>, Drag)
		SLATE_ARGUMENT(TFunction<void(double, double, bool)>, OnMove)
	SLATE_END_ARGS()

	void Construct(const FArguments& A)
	{
		D = A._Drag.IsValid() ? A._Drag : MakeShared<FS47MapDrag>();
		OnMove = A._OnMove;
		Base = S47Kit::BrushFor(MapTexture());
		Sheet = S47Kit::BrushFor(TracingTexture());
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		const S47Drawings::FServiceMap& Mp = S47Drawings::SERVICE_MAP();
		return FVector2D(MAP_W, MAP_W * Mp.H / Mp.W);
	}

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
	{
		const S47Drawings::FServiceMap& Mp = S47Drawings::SERVICE_MAP();
		const float W = G.GetLocalSize().X, H = W * Mp.H / Mp.W, S = W / Mp.W;
		const FGeometry Box = G.MakeChild(FVector2f(W, H), FSlateLayoutTransform());
		Out.PushClip(FSlateClippingZone(Box));
		// g.drawImage(base, 0, 0); drawTracing(g, x, y)
		if (Base.IsValid()) FSlateDrawElement::MakeBox(Out, Layer, Box.ToPaintGeometry(), Base.Get(), ESlateDrawEffect::None, FLinearColor::White);
		if (Sheet.IsValid())
		{
			const FVector2f At(float((D->X - TX) * S), float((D->Y - TY) * S));
			FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(FVector2f(TW * S, TH * S), FSlateLayoutTransform(At)), Sheet.Get(), ESlateDrawEffect::None, FLinearColor::White);
		}
		Out.PopClip();
		return Layer + 2;
	}

	// map coordinates of a pointer (toMap)
	FVector2D ToMap(const FGeometry& G, const FPointerEvent& E) const
	{
		const S47Drawings::FServiceMap& Mp = S47Drawings::SERVICE_MAP();
		const FVector2D P = G.AbsoluteToLocal(E.GetScreenSpacePosition());
		const double W = FMath::Max(1.0, double(G.GetLocalSize().X));
		return FVector2D(P.X * Mp.W / W, P.Y * Mp.W / W);
	}

	virtual FReply OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override
	{
		if (E.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
		const FVector2D P = ToMap(G, E);
		if (!D->Down(P.X, P.Y)) return FReply::Unhandled();
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	virtual FReply OnMouseMove(const FGeometry& G, const FPointerEvent& E) override
	{
		const FVector2D P = ToMap(G, E);
		return D->Move(P.X, P.Y) ? FReply::Handled() : FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent& E) override
	{
		if (E.GetEffectingButton() != EKeys::LeftMouseButton || !D->Grab) return FReply::Unhandled();
		Drop();
		return FReply::Handled().ReleaseMouseCapture();
	}

	// pointercancel
	virtual void OnMouseCaptureLost(const FCaptureLostEvent&) override { Drop(); }

	// cursor: grab, and grabbing while it is held (.mapov canvas:active)
	virtual FCursorReply OnCursorQuery(const FGeometry&, const FPointerEvent&) const override
	{
		return FCursorReply::Cursor(S47Theme::GrabCursor(bool(D->Grab)));
	}

private:
	TSharedPtr<FS47MapDrag> D;
	TFunction<void(double, double, bool)> OnMove;
	TSharedPtr<FSlateBrush> Base, Sheet;

	void Drop()
	{
		if (!D->Drop()) return;
		if (OnMove) OnMove(D->X, D->Y, D->Aligned());
	}
};

TSharedRef<SWidget> S47MapOverlay::MapOverlay(US47UI* UI, const FOpts& O)
{
	TSharedRef<FS47MapDrag> D = MakeShared<FS47MapDrag>();
	D->X = O.At.X; D->Y = O.At.Y;
	D->bAskHidden = !O.Solved.IsEmpty();
	if (!O.Solved.IsEmpty()) { D->Say = O.Solved; D->Tone = 1; }
	// <p class="lp-sub" data-status>
	const S47Kit::FW Status = SNew(STextBlock).Text_Lambda([D] { return FText::FromString(D->Status); })
		.Font(S47Style::Mono(19.f * PX)).ColorAndOpacity(S47Kit::Css(0xffb052)).AutoWrapText(true).Margin(FMargin(0, 0, 0, 10));
	// <div class="mapov"><canvas data-map width="1200" height="800">
	const S47Kit::FW Map = SNew(SBox).HAlign(HAlign_Center).Padding(FMargin(0, 4 * PX, 0, 10 * PX))
		[
			SNew(SS47MapCanvas).Drag(D).OnMove(O.OnMove)
		];
	// <div data-ask>: the question and a button for each answer
	TArray<S47Kit::FW> Buttons;
	const TFunction<FS47Result(const FString&)> OnChoose = O.OnChoose;
	for (const FChoice& C : O.Choices)
	{
		const FString Id = C.Id;
		Buttons.Add(S47Kit::Button(C.Label, [D, OnChoose, Id]
		{
			if (!OnChoose) return;
			const FS47Result R = OnChoose(Id);
			// panelSay(el, '[data-say]', r)
			D->Say = R.Text; D->Tone = R.bOk ? 1 : -1;
			if (R.bOk) D->bAskHidden = true;
		}));
	}
	const S47Kit::FW Ask = SNew(SBox).Visibility_Lambda([D] { return D->bAskHidden ? EVisibility::Collapsed : EVisibility::Visible; })
		[
			S47Kit::Col({ S47Kit::Question(O.Question), S47Kit::Col(Buttons) }, 0.f)
		];
	// <p class="lp-text" data-say>
	const S47Kit::FW Say = S47Kit::Text(TAttribute<FString>::CreateLambda([D] { return D->Say; }), TAttribute<int32>::CreateLambda([D] { return D->Tone; }));
	D->Draw();
	const S47Kit::FW El = S47Kit::Shell(UI, TEXT("THE BOOTH / SERVICE MAP AND E09A"), S47Kit::Col({ Status, Map, Ask, Say }, 0.f));
	if (UI) UI->Open(El, O.OnClose);
	return El;
}

bool S47MapOverlay::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s map overlay: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// tools/chapter5.py's drag: from where the tracing lies (300, 690), grabbed 30 to the right of the
	// transit, let go at 474, 363 from the station: it settles on STATION 01
	FS47MapDrag D;
	D.X = 300; D.Y = 690; D.Draw();
	Check(!D.Aligned() && D.Status.StartsWith(TEXT("Drag the tracing")), TEXT("the tracing off the station: drag it"));
	Check(!D.Down(220, 690) && !D.Down(300, 535) && D.Down(330, 690), TEXT("a press takes the sheet only on it (620 by 300, the transit 70 in)"));
	for (int32 n = 1; n <= 12; ++n) D.Move(330 + (504 - 330) * n / 12.0, 690 + (363 - 690) * n / 12.0);
	Check(FMath::IsNearlyEqual(D.X, 474.0) && FMath::IsNearlyEqual(D.Y, 363.0) && !D.Aligned(), TEXT("dragged, it follows the pointer"));
	Check(D.Drop() && D.X == 470 && D.Y == 360 && D.Aligned() && D.Status.Contains(TEXT("sits on STATION 01")) && !D.Drop(), TEXT("let go within 18 of the station, it settles there, north to north"));
	D.Down(500, 360);
	D.Move(5000, -100);
	Check(D.X == 1160 && D.Y == 40, TEXT("kept on the map, 40 from its edges"));
	D.Drop();
	Check(D.X == 1160 && !D.Aligned(), TEXT("let go far off, it stays where it is"));
	return bOk;
}

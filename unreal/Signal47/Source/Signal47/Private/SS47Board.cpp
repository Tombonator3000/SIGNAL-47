#include "SS47Board.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Texture2D.h"
#include "S47Theme.h"
#include "S47Gamepad.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/UserInterfaceSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "S47Image.h"
#include "S47Kit.h"
#include "S47UI.h"
#include "SS47Widgets.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"

namespace
{
	using namespace S47Kit;
	using FW = TSharedRef<SWidget>;

	const FSlateBrush* Box(uint32 Fill, uint32 Line = 0, float Alpha = 1.f, float LineAlpha = 0.f, float Width = 1.f)
	{
		static TMap<FString, TSharedPtr<FSlateBrush>> Cache;
		const FString Key = FString::Printf(TEXT("%u/%u/%g/%g/%g"), Fill, Line, Alpha, LineAlpha, Width);
		TSharedPtr<FSlateBrush>& B = Cache.FindOrAdd(Key);
		if (!B) B = MakeShared<FSlateRoundedBoxBrush>(Css(Fill, Alpha), 0.f, Css(Line, LineAlpha), Width);
		return B.Get();
	}

	const FButtonStyle* ButtonStyle(bool bRecord = false, bool bText = false)
	{
		static const FButtonStyle Plain = [] {
			FButtonStyle S;
			S.SetNormal(*Box(0xffc896, 0xffd6aa, 0.05f, 0.35f)).SetHovered(*Box(0xffc896, 0xffd6aa, 0.14f, 0.35f));
			S.SetPressed(S.Hovered).SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
			return S;
		}();
		static const FButtonStyle Record = [] {
			FButtonStyle S;
			S.SetNormal(*Box(0xa3291c, 0x6e1d13, 1, 1)).SetHovered(*Box(0xb93322, 0x6e1d13, 1, 1));
			S.SetPressed(S.Hovered).SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
			return S;
		}();
		static const FButtonStyle Text = [] {
			FButtonStyle S;
			S.SetNormal(*Box(0, 0, 0, 0)).SetHovered(*Box(0xc4241c, 0, 0.08f, 0));
			S.SetPressed(S.Hovered).SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
			return S;
		}();
		return bRecord ? &Record : bText ? &Text : &Plain;
	}

	FW Text(const FString& S, float Size, uint32 Colour, bool bTerm = false, double Wrap = 0, float LineHeight = 1.12f)
	{
		return SNew(STextBlock).Text(FText::FromString(S)).Font(bTerm ? S47Style::Mono(Size) : S47Style::Body(Size))
			.ColorAndOpacity(Css(Colour)).WrapTextAt(Wrap).LineHeightPercentage(LineHeight);
	}

	FW Button(const FString& Label, TFunction<void()> Fn, float FontSize, uint32 Colour, double Width = 44, double Height = 40, bool bRecord = false, bool bText = false)
	{
		return SNew(SButton).ButtonStyle(ButtonStyle(bRecord, bText)).ContentPadding(FMargin(bRecord ? 14 : 6, 2))
			.OnClicked_Lambda([Fn] { if (Fn) Fn(); return FReply::Handled(); })
			[SNew(SBox).MinDesiredWidth(Width - (bRecord ? 28 : 12)).MinDesiredHeight(Height - 4).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[Text(Label, FontSize, Colour, true)]];
	}

	TSharedPtr<FSlateBrush> Art(const TCHAR* Name) { return S47Kit::BrushFor(S47Image::Art(Name)); }
	TSharedPtr<FSlateBrush> Picture(const FString& S)
	{
		return S47Kit::BrushFor(S.StartsWith(TEXT("data:")) ? S47Image::FromDataUrl(S) : S47Image::Art(S));
	}

	struct FCurve
	{
		FVector2D A, C, B;
		FVector2D At(double T) const { return A * ((1 - T) * (1 - T)) + C * (2 * (1 - T) * T) + B * (T * T); }
	};
	// ui/Board.ts curve(): quadratic yarn, in board pixels.
	FCurve Curve(const FVector2D& A, const FVector2D& B, double Slack = 0.12, double Max = 60)
	{
		return { A, (A + B) / 2 + FVector2D(0, FMath::Min(Max, (B - A).Size() * Slack)), B };
	}

	bool Inside(const FVector2D& P, const FVector2D& At, const FVector2D& Size)
	{
		return P.X >= At.X && P.Y >= At.Y && P.X <= At.X + Size.X && P.Y <= At.Y + Size.Y;
	}
	bool Overlap(const FVector2D& A, const FVector2D& Size, const FVector2D& B, const FVector2D& Other, double Gap)
	{
		return A.X < B.X + Other.X + Gap && A.X + Size.X + Gap > B.X && A.Y < B.Y + Other.Y + Gap && A.Y + Size.Y + Gap > B.Y;
	}

	// The width of the yarn on screen (departs from the web version, whose 2.6 px SVG stroke and
	// 1.5 px yarn picture Tom found nearly invisible): a board width scaled by the zoom, never
	// thinner than a line that still reads at 900 lines, never a rope when zoomed right in.
	double YarnWidth(double BoardWidth, double Px, double Min = 2.5)
	{
		return FMath::Clamp(BoardWidth * Px, Min, 12.0);
	}

	// The help strip under the board's heading stays hidden for the session once dismissed.
	bool GHowHidden = false;

	// Slate's scroll box owns the long TEXT area. Ctrl+wheel must still reach the board zoom.
	class SS47BoardPaper : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SS47BoardPaper) {} SLATE_ARGUMENT(const FSlateBrush*, Brush) SLATE_END_ARGS()
		const FSlateBrush* Brush = nullptr;
		void Construct(const FArguments& A) { Brush = A._Brush; SetClipping(EWidgetClipping::ClipToBounds); }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const override
		{
			if (Brush)
			{
				const FVector2D ImageSize(G.GetLocalSize().X, G.GetLocalSize().X * Brush->ImageSize.Y / FMath::Max(1.f, Brush->ImageSize.X));
				FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(ImageSize, FSlateLayoutTransform()), Brush);
			}
			return L;
		}
	};

	// ui/Board.ts .bnote.hand: a torn scrap, neutral to text layout and inheriting the card transform.
	class SS47BoardScrap : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SS47BoardScrap) {} SLATE_END_ARGS()
		void Construct(const FArguments&) {}
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L, const FWidgetStyle&, bool) const override
		{
			const FVector2D Edge[] = { {0, .04}, {.18, 0}, {.46, .06}, {.72, .01}, {1, .05}, {.98, .92}, {.7, 1}, {.38, .95}, {.12, 1}, {.01, .94} };
			TArray<FSlateVertex> Verts; TArray<SlateIndex> Indices;
			const FColor Paper(0xec, 0xe5, 0xd1);
			Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(G.GetAccumulatedRenderTransform(), FVector2f(G.GetLocalSize() / 2), FVector2f::ZeroVector, Paper));
			for (const FVector2D& V : Edge)
				Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(G.GetAccumulatedRenderTransform(), FVector2f(V * G.GetLocalSize()), FVector2f::ZeroVector, Paper));
			for (int32 I = 0; I < UE_ARRAY_COUNT(Edge); ++I) { Indices.Add(0); Indices.Add(I + 1); Indices.Add((I + 1) % UE_ARRAY_COUNT(Edge) + 1); }
			const FSlateResourceHandle Handle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")));
			FSlateDrawElement::MakeCustomVerts(Out, L, Handle, Verts, Indices, nullptr, 0, 0);
			return L;
		}
	};

	class SS47BoardScroll : public SScrollBox
	{
	public:
		virtual FReply OnMouseWheel(const FGeometry& G, const FPointerEvent& E) override
		{
			if (E.IsControlDown() || E.IsCommandDown()) return FReply::Unhandled();
			const bool Overflow = GetScrollOffsetOfEnd() > 2;
			const FReply Reply = SScrollBox::OnMouseWheel(G, E);
			// ui/Board.ts scrolling(): an overflow TEXT area owns the wheel even at either end.
			return Overflow ? FReply::Handled() : Reply;
		}
	};
}

class SS47BoardSurface;

struct FS47BoardImpl
{
	FS47BoardSpec O;
	TWeakObjectPtr<US47UI> UI;
	TSharedPtr<SCanvas> Canvas;
	TSharedPtr<SS47BoardSurface> Surface;
	TSharedPtr<STextBlock> StatusEl, SayEl, HowEl;
	// Departs from the web version: a dismissable strip that says how the board works, a hint
	// offered after a thread that does not hold or a while without progress, and the hint itself
	// in a readable box rather than the reply line.
	TSharedPtr<SWidget> HowStrip, HintBox, OfferEl;
	TSharedPtr<STextBlock> HintText, HintHead, OfferText;
	FString Offer, Hinted;
	double LastProgress = 0;
	int32 Misses = 0;
	TMap<FString, TArray<FString>> RegionHave, RegionMissing;   // record stamps by board region, for the question cards
	FString TestFrom, TestTo;                                    // -S47BoardTest drag: a thread held over a pin
	TSet<FString> Open;
	TMap<FString, int32> Zs;
	TMap<FString, float> TextScrolls;
	bool bTextToggleRender = false;
	FString Selected;
	int32 Top = 10;
	double S = 1, Tx = 0, Ty = 0;
	double ParentScale = 1;
	FVector2D ViewSize = FVector2D::ZeroVector;
	bool bInitialFocus = true, bChanged = false, bCoarse = false;
	FString PendingLook, PendingShow;
	TArray<FString> PendingFrame;
	TMap<FString, TSharedPtr<FSlateBrush>> ArtBrushes;

	struct FPin
	{
		FString Id;
		FVector2D Offset = FVector2D::ZeroVector;
		FVector2D LineAt = FVector2D::ZeroVector, LineSize = FVector2D::ZeroVector;
		TSharedPtr<SWidget> LineWidget;
	};
	struct FBox
	{
		FString Id, Region, Node;
		FVector2D At = FVector2D::ZeroVector, Size = FVector2D::ZeroVector;
		TSharedPtr<SWidget> Widget;
		TSharedPtr<SScrollBox> Full;
		TArray<FPin> Pins;
		bool bFree = false, bLines = false, bHand = false, bNote = false, bLift = false;
		bool bQuestion = false, bDone = false, bFinding = false, bPhoto = false;
		int32 Z = 2;
	};
	TArray<TSharedPtr<FBox>> Boxes;
	struct FSlack { FCurve Q; double Born = 0; };
	TArray<FSlack> Slack;
	struct FDrag
	{
		enum class EKind { None, Thread, Card, Pan } Kind = EKind::None;
		FString From;
		TSharedPtr<FBox> Box;
		FVector2D Start = FVector2D::ZeroVector, Now = FVector2D::ZeroVector, Grab = FVector2D::ZeroVector, T0 = FVector2D::ZeroVector;
		bool bMoved = false;
	} Drag;
	TMap<int32, FVector2D> Ptrs;
	struct FPinch { double D0 = 1, S0 = 1; FVector2D Mid = FVector2D::ZeroVector; };
	TOptional<FPinch> Pinch;

	TArray<FS47BoardRegion> Regions() const { return O.Regions ? O.Regions() : TArray<FS47BoardRegion>(); }
	TOptional<FS47BoardRegion> RegionOf(const FString& Id) const
	{
		for (const FS47BoardRegion& R : Regions()) if (R.Id == Id) return R;
		return {};
	}
	FVector2D ScreenSize() const;
	bool Small() const;
	void Say(const FString& V, int32 Tone = 0);
	TOptional<FVector2D> PlaceOf(const FString& Id, const FString& Region = FString(), TOptional<double> X = {}, TOptional<double> Y = {}) const;
	TOptional<FVector2D> Centre(const FString& Id) const;
	void RefreshPins(const TSharedPtr<FBox>& B) const;
	void AddBox(const TSharedPtr<FBox>& B, const FString& Region, TOptional<double> X, TOptional<double> Y, double Width);
	TSharedRef<SWidget> CardHtml(const FS47BoardCard& C, const TSharedPtr<FBox>& B);
	TSharedRef<SWidget> QuestionHtml(const FS47BoardQuestion& Q, const TSharedPtr<FBox>& B);
	void PlaceFree();
	void Render();
	FS47Result Link(const FString& A, const FString& B);
	void TapNode(const FString& N);
	FString OpenQuestionIn(const FString& Region) const;
	void OfferHint(const FString& Q);
	void ShowHint(const FString& Q);
	void ShowHow(bool bShow);
	FVector2D ToBoardXY(const FVector2D& P) const { return (P - FVector2D(Tx, Ty)) / S; }
	double MinScale() const { return FMath::Min(0.5, FMath::Max(0.12, FMath::Min(ViewSize.X / O.Size.X, ViewSize.Y / O.Size.Y) * 0.95)); }
	void ClampView();
	void ZoomAt(double Ns, const FVector2D& P);
	bool PadView(const FKey& Key);
	void Look(const FS47BoardRegion& R);
	bool Frame(const TArray<FString>& Ids);
	bool Show(const FString& Id);
	void Follow();
	void Edge();
	struct FHit { TSharedPtr<FBox> Box; FString Pull, Node; };
	FHit Hit(const FVector2D& P) const;
	bool Scrolling(const FHit& H, const FVector2D& P) const;
	void Down(int32 Id, const FVector2D& P, bool bTouch);
	void Move(int32 Id, const FVector2D& P);
	void End(int32 Id, const FVector2D& P, bool bCancel);
	void Tick(const FVector2D& InSize);
	int32 Paint(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer, bool bAbove) const;
};

// Slate has no DOM absolute-positioned element: SCanvas and a single render transform supply
// the web board's CSS transform, while the viewport clips both the children and the yarn.
class SS47BoardSurface : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47BoardSurface) {} SLATE_ARGUMENT(FS47BoardImpl*, Board) SLATE_END_ARGS()
	FS47BoardImpl* P = nullptr;
	void Construct(const FArguments& A)
	{
		P = A._Board;
		SetClipping(EWidgetClipping::ClipToBoundsAlways);
		ChildSlot[SAssignNew(P->Canvas, SCanvas)
			.RenderTransform_Lambda([this] { return TOptional<FSlateRenderTransform>(FSlateRenderTransform(FScale2D(P->S), FVector2D(P->Tx, P->Ty))); })
			.RenderTransformPivot(FVector2D::ZeroVector)];
	}
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
	virtual void Tick(const FGeometry& G, const double, const float) override { P->Tick(G.GetLocalSize()); }
	virtual int32 OnPaint(const FPaintArgs& A, const FGeometry& G, const FSlateRect& C, FSlateWindowElementList& Out, int32 L, const FWidgetStyle& Style, bool Enabled) const override
	{
		const int32 Background = P->Paint(G, Out, L, false);
		const int32 End = SCompoundWidget::OnPaint(A, G, C, Out, Background + 1, Style, Enabled);
		return P->Paint(G, Out, End + 2, true);
	}
	virtual FCursorReply OnCursorQuery(const FGeometry& G, const FPointerEvent& E) const override
	{
		// (S47Theme::GrabCursor: the packaged game has no grab hand to show)
		if (P->Drag.bMoved && P->Drag.Kind != FS47BoardImpl::FDrag::EKind::Thread) return FCursorReply::Cursor(S47Theme::GrabCursor(true));
		const FS47BoardImpl::FHit H = P->Hit(P->ToBoardXY(G.AbsoluteToLocal(E.GetScreenSpacePosition())));
		return FCursorReply::Cursor(H.Pull.IsEmpty() ? S47Theme::GrabCursor(false) : EMouseCursor::Crosshairs);
	}
	virtual FReply OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override
	{
		if (E.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
		P->Down(E.GetPointerIndex(), G.AbsoluteToLocal(E.GetScreenSpacePosition()), false);
		return FReply::Handled().CaptureMouse(AsShared()).PreventThrottling();
	}
	virtual FReply OnMouseMove(const FGeometry& G, const FPointerEvent& E) override
	{
		if (!P->Ptrs.Contains(E.GetPointerIndex())) return FReply::Unhandled();
		P->Move(E.GetPointerIndex(), G.AbsoluteToLocal(E.GetScreenSpacePosition()));
		return FReply::Handled();
	}
	virtual FReply OnMouseButtonUp(const FGeometry& G, const FPointerEvent& E) override
	{
		if (E.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
		P->End(E.GetPointerIndex(), G.AbsoluteToLocal(E.GetScreenSpacePosition()), false);
		return FReply::Handled().ReleaseMouseCapture();
	}
	virtual FReply OnTouchStarted(const FGeometry& G, const FPointerEvent& E) override
	{
		const FVector2D At = G.AbsoluteToLocal(E.GetScreenSpacePosition());
		if (P->Scrolling(P->Hit(P->ToBoardXY(At)), At)) return FReply::Unhandled();
		P->Down(E.GetPointerIndex(), At, true);
		return FReply::Handled().CaptureMouse(AsShared());
	}
	virtual FReply OnTouchMoved(const FGeometry& G, const FPointerEvent& E) override { return OnMouseMove(G, E); }
	virtual FReply OnTouchEnded(const FGeometry& G, const FPointerEvent& E) override
	{
		P->End(E.GetPointerIndex(), G.AbsoluteToLocal(E.GetScreenSpacePosition()), false);
		return P->Ptrs.IsEmpty() ? FReply::Handled().ReleaseMouseCapture() : FReply::Handled();
	}
	virtual void OnMouseCaptureLost(const FCaptureLostEvent&) override
	{
		// A lost Slate pointer capture is the browser's pointercancel.
		if (P->Drag.Box) P->Drag.Box->bLift = false;
		P->Drag = {}; P->Ptrs.Empty(); P->Pinch.Reset();
	}
	virtual FReply OnMouseWheel(const FGeometry& G, const FPointerEvent& E) override
	{
		const FVector2D At = G.AbsoluteToLocal(E.GetScreenSpacePosition());
		// Slate reports wheel notches with the opposite sign to WheelEvent.deltaY.
		const double Dy = -E.GetWheelDelta() * 100;
		if (E.IsControlDown() || E.IsCommandDown()) P->ZoomAt(P->S * FMath::Exp(-Dy * 0.0025), At);
		else if (E.IsShiftDown()) { P->Tx -= Dy; P->ClampView(); }
		else { P->Ty -= Dy; P->ClampView(); }
		return FReply::Handled();
	}
};

FVector2D FS47BoardImpl::ScreenSize() const
{
	const TSharedPtr<SWindow> Window = FSlateApplication::Get().GetActiveTopLevelWindow();
	return Window ? FVector2D(Window->GetClientSizeInScreen()) : ViewSize;
}

bool FS47BoardImpl::Small() const
{
	// The HTML test uses innerWidth/innerHeight, including the board's heading and reply.
	const FVector2D Window = ScreenSize();
	return Window.Y < 560 || Window.X < 700;
}

void FS47BoardImpl::Say(const FString& V, int32 Tone)
{
	SayEl->SetText(FText::FromString(V));
	SayEl->SetColorAndOpacity(Css(Tone > 0 ? 0xb9ffc7 : Tone < 0 ? 0xffb3a0 : 0xddd5c2));
}

TOptional<FVector2D> FS47BoardImpl::PlaceOf(const FString& Id, const FString& Region, TOptional<double> X, TOptional<double> Y) const
{
	const TOptional<FVector2D> Saved = O.LayoutGet ? O.LayoutGet(Id) : TOptional<FVector2D>();
	if (Saved) return Saved;
	const TOptional<FS47BoardRegion> R = Region.IsEmpty() ? TOptional<FS47BoardRegion>() : RegionOf(Region);
	if (R && X && Y) return FVector2D(R->X + *X, R->Y + *Y);
	return {};
}

void FS47BoardImpl::RefreshPins(const TSharedPtr<FBox>& B) const
{
	for (FPin& Pin : B->Pins)
	{
		if (!Pin.LineWidget) continue;
		const FGeometry& G = Pin.LineWidget->GetCachedGeometry();
		if (G.GetLocalSize().X <= 0 || G.GetLocalSize().Y <= 0) continue;
		const FGeometry& Root = B->Widget->GetCachedGeometry();
		Pin.LineAt = Root.AbsoluteToLocal(G.LocalToAbsolute(FVector2D::ZeroVector));
		Pin.LineSize = G.GetLocalSize();
		Pin.Offset = Pin.LineAt + FVector2D(Pin.LineSize.X - (ArtBrushes.FindRef(TEXT("pin_red")) ? 13 : 12), Pin.LineSize.Y / 2);
	}
}

TOptional<FVector2D> FS47BoardImpl::Centre(const FString& Id) const
{
	for (const TSharedPtr<FBox>& B : Boxes)
	{
		if (B->bFree) continue;
		RefreshPins(B);
		for (const FPin& Pin : B->Pins) if (Pin.Id == Id)
		{
			const FGeometry& Root = B->Widget->GetCachedGeometry();
			if (Surface && Root.GetLocalSize().X > 0)
				return ToBoardXY(Surface->GetCachedGeometry().AbsoluteToLocal(Root.LocalToAbsolute(Pin.Offset)));
			return B->At + Pin.Offset;
		}
	}
	return {};
}

void FS47BoardImpl::AddBox(const TSharedPtr<FBox>& B, const FString& Region, TOptional<double> X, TOptional<double> Y, double Width)
{
	B->Region = Region;
	B->Z = Zs.Contains(B->Id) ? Zs[B->Id] : Open.Contains(B->Id) ? 2000 : 2;
	const TOptional<FVector2D> Place = PlaceOf(B->Id, Region, X, Y);
	B->bFree = !Place;
	if (Place) B->At = *Place;
	B->Widget = SNew(SBox).WidthOverride(Width)[B->Widget.ToSharedRef()];
	B->Widget->SlatePrepass();
	B->Size = FVector2D(Width, B->Widget->GetDesiredSize().Y);
	Boxes.Add(B);
}

TSharedRef<SWidget> FS47BoardImpl::CardHtml(const FS47BoardCard& C, const TSharedPtr<FBox>& B)
{
	const double Width = C.W.Get(C.Kind == TEXT("photo") || C.Kind == TEXT("map") ? 220 : 240);
	const bool Plain = C.Lines.IsEmpty(), Sealed = C.Kind == TEXT("sealed"), Photo = C.Kind == TEXT("photo"), Map = C.Kind == TEXT("map");
	const bool bOpen = Open.Contains(C.Id);
	B->Node = C.bHave && Plain ? C.Id : FString();
	B->bLines = C.bHave && !Plain;
	B->bFinding = C.bHave && C.Kind == TEXT("finding");
	B->bPhoto = C.bHave && Photo;
	if (!B->Node.IsEmpty()) B->Pins.Add({ C.Id, FVector2D(Width / 2, 2) });
	const uint32 Fill = Sealed ? 0x2b2a2c : Photo || Map ? 0xf6f2ea : C.Kind == TEXT("thing") ? 0xdfe6dc : C.Kind == TEXT("finding") ? 0xe7eedf : 0xebe3cf;
	const uint32 Ink = Sealed ? 0xd8d2c4 : 0x1e2430;
	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	TSharedRef<SHorizontalBox> Head = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)[Text(C.Stamp, 20, Sealed ? 0xffb09a : 0x8f2a1d, true, 0, 1.35f)]
		+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[Text(C.Title, 17, Ink, true, FMath::Max(40.0, Width - 85 - (C.Full.IsEmpty() ? 0 : 44)), 1.35f)];
	if (C.bHave && !C.Full.IsEmpty()) Head->AddSlot().AutoWidth().VAlign(VAlign_Center)
		[Button(TEXT("TEXT"), [this, Id = C.Id] {
			// The DOM toggles only this card: other open cards keep their reading position.
			for (const TSharedPtr<FBox>& Card : Boxes) if (Card->Full) TextScrolls.Add(Card->Id, Card->Full->GetScrollOffset());
			if (Open.Contains(Id)) Open.Remove(Id);
			else { Open.Add(Id); Zs.Add(Id, ++Top); }
			bTextToggleRender = true; Render(); bTextToggleRender = false;
			for (const TSharedPtr<FBox>& Card : Boxes) if (Card->Full && TextScrolls.Contains(Card->Id)) Card->Full->SetScrollOffset(TextScrolls[Card->Id]);
		}, 16, Sealed ? 0x9fb4ff : 0x233a7a, 44, 30, false, true)];
	Body->AddSlot().AutoHeight()[SNew(SBorder).BorderImage(Box(0, 0x1e2430, 0, 0.18f)).Padding(FMargin(10, 6))[Head]];
	if (!C.bHave)
	{
		Body->AddSlot().AutoHeight().Padding(10, 8)[Text(TEXT("Not on the table yet. ") + C.Where, 13, 0x3a3326, false, Width - 20, 1.35f)];
		return SNew(SBorder).BorderImage(Box(0xebe3cf, 0, 0.45f)).Padding(FMargin(0, 0, 0, 6))[Body];
	}
	if (!C.Image.IsEmpty())
	{
		TSharedPtr<FSlateBrush> Image = Picture(C.Image);
		if (Image)
		{
			const double ImageWidth = Width - (Photo || Map ? 16 : 20);
			const double NaturalHeight = ImageWidth * Image->ImageSize.Y / FMath::Max(1.f, Image->ImageSize.X);
			const double Height = C.Id == TEXT("hole") && !bOpen ? ImageWidth * 5 / 8 : FMath::Min(bOpen ? 260.0 : Photo || Map ? 150.0 : 84.0, NaturalHeight);
			Body->AddSlot().AutoHeight().Padding(Photo || Map ? FMargin(0, 4) : FMargin(10, 6, 10, 2))
			[SNew(SBox).HeightOverride(Height)[SNew(SBorder).BorderImage(Box(Photo || Map ? 0x111111 : 0xfff8e8)).Padding(0)
			[SNew(SScaleBox).Stretch(Photo || Map ? EStretch::ScaleToFill : EStretch::ScaleToFit).Clipping(EWidgetClipping::ClipToBounds)
			[SNew(SImage).Image(Image.Get())]]]];
		}
		else
		{
			const double ImageWidth = Width - (Photo || Map ? 16 : 20);
			// HTML reserves hole's 8:5 print even before the image is available.
			Body->AddSlot().AutoHeight().Padding(Photo || Map ? FMargin(0, 4) : FMargin(10, 6, 10, 2))
				[SNew(SBox).HeightOverride(C.Id == TEXT("hole") ? ImageWidth * 5 / 8 : 0)[SNew(SBorder).BorderImage(Box(Photo || Map ? 0x111111 : 0xfff8e8)).Padding(0)]];
		}
	}
	if (!C.Text.IsEmpty()) Body->AddSlot().AutoHeight().Padding(10, 6, 10, 2)[Text(C.Text, 12.5f, Sealed ? 0xbdb6a6 : 0x3a3326, false, Width - 20, 1.32f)];
	if (bOpen && !C.Full.IsEmpty())
	{
		TSharedRef<SVerticalBox> Paras = SNew(SVerticalBox);
		TArray<FString> Ps; C.Full.ParseIntoArray(Ps, TEXT("\n\n"), false);
		for (const FString& Paragraph : Ps) Paras->AddSlot().AutoHeight().Padding(0, 0, 0, 6)[Text(Paragraph, 12.5f, 0x3a3326, false, Width - 20, 1.35f)];
		TSharedPtr<SS47BoardScroll> Scroll;
		Body->AddSlot().AutoHeight().Padding(10, 2)[SNew(SBox).MaxDesiredHeight(420)
			[SAssignNew(Scroll, SS47BoardScroll).ScrollBarThickness(FVector2D(5, 5)) + SScrollBox::Slot()[Paras]]];
		B->Full = Scroll;
	}
	for (const FS47BoardLine& L : C.Lines)
	{
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		if (!L.Icon.IsEmpty())
		{
			TSharedPtr<FSlateBrush> Icon = Picture(L.Icon);
			if (Icon) Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)[SNew(SBox).WidthOverride(26).HeightOverride(26)[SNew(SImage).Image(Icon.Get())]];
		}
		Row->AddSlot().FillWidth(1).VAlign(VAlign_Center)[Text(L.Text, 14, Ink, false, Width - 62 - (L.Icon.IsEmpty() ? 0 : 32), 1.35f)];
		TSharedRef<SWidget> Line = SNew(SBox).MinDesiredHeight(44)
			[SNew(SBorder).BorderImage_Lambda([this, Id = L.Id] { return Selected == Id ? Box(0xc4241c, 0xc4241c, 0.1f, 1) : Box(0, 0xc4241c, 0, 0); }).Padding(FMargin(6, 5, 24, 5))[Row]];
		Body->AddSlot().AutoHeight().Padding(8, 4, 8, 0)[Line];
		// Before the first Slate arrangement, desired heights give centre() a faithful fallback.
		Body->SlatePrepass();
		FPin Pin; Pin.Id = L.Id; Pin.LineWidget = Line;
		Pin.LineSize = FVector2D(Width - 16, Line->GetDesiredSize().Y);
		Pin.LineAt = FVector2D(8, Body->GetDesiredSize().Y - Pin.LineSize.Y + (Plain ? 6 : 0) + (Photo || Map ? 8 : 0));
		Pin.Offset = Pin.LineAt + FVector2D(Pin.LineSize.X - (ArtBrushes.FindRef(TEXT("pin_red")) ? 13 : 12), Pin.LineSize.Y / 2);
		B->Pins.Add(Pin);
	}
	return SNew(SBorder)
		.BorderImage_Lambda([this, Id = C.Id, Fill] { return Selected == Id ? Box(0xffece2, 0xc4241c, 0.95f, 1, 2) : Box(Fill); })
		.Padding(Photo || Map ? FMargin(8) : FMargin(0, Plain ? 6 : C.Kind == TEXT("finding") ? 4 : 0, 0, 6))[Body];
}

TSharedRef<SWidget> FS47BoardImpl::QuestionHtml(const FS47BoardQuestion& Q, const TSharedPtr<FBox>& B)
{
	const double Width = Q.W.Get(260);
	const bool bDone = !Q.Done.IsEmpty(), Ready = !bDone && Q.Slots > 0 && Q.Filled.Num() >= Q.Slots;
	B->bQuestion = true; B->bDone = bDone;
	if (bDone) { B->Node = Q.Id; B->Pins.Add({ Q.Id, FVector2D(Width / 2, 2) }); }
	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	const FString Count = bDone ? TEXT("RECORDED") : Q.Slots ? FString::Printf(TEXT("%d / %d"), Q.Filled.Num(), Q.Slots) : TEXT("OPEN");
	Body->AddSlot().AutoHeight()[SNew(SBorder).BorderImage(Box(0, 0x1e2430, 0, 0.18f)).Padding(FMargin(10, 6))
		[SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)[Text(Q.Stamp, 20, 0x8f2a1d, true)]
		+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[Text(Count, 17, bDone ? 0x2f6a3c : 0x6b6458, true)]]];
	Body->AddSlot().AutoHeight().Padding(10, 6)[Text(Q.Text, 15, 0x1e2430, false, Width - 20)];
	if (bDone) Body->AddSlot().AutoHeight().Padding(10, 6, 10, 0)[Text(Q.Done, 12.5f, 0x2c4a30, false, Width - 20)];
	else
	{
		if (Q.Slots)
		{
			// Departs from the web version: the card says what it wants and what there is to work
			// with (the records of its patch on the board, and those still to find), not which.
			const FString Wants = Ready ? TEXT("Every line is filled. RECORD it.")
				: FString::Printf(TEXT("Wants %d thread%s that hold%s, tied between the records. Each fills a line below."), Q.Slots, Q.Slots == 1 ? TEXT("") : TEXT("s"), Q.Slots == 1 ? TEXT("s") : TEXT(""));
			Body->AddSlot().AutoHeight().Padding(10, 0, 10, 4)[Text(Wants, 12, Ready ? 0x2f6a3c : 0x7a3a22, false, Width - 20, 1.2f)];
			for (int32 I = 0; I < Q.Slots; ++I)
			{
				const bool On = Q.Filled.IsValidIndex(I) && !Q.Filled[I].IsEmpty();
				Body->AddSlot().AutoHeight().Padding(10, I == 0 ? 2 : 0, 10, 4)
				[SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)[SNew(SBox).WidthOverride(20)[Text(FString::Printf(TEXT("%d."), I + 1), 12.5f, 0x555555)]]
				+ SHorizontalBox::Slot().FillWidth(1)[SNew(SBox).MinDesiredHeight(18)
				[SNew(SBorder).BorderImage(Box(0xf1dc7c, 0x1e2430, On ? 0.55f : 0, 0.25f)).Padding(FMargin(3, 1))
				[Text(On ? Q.Filled[I] : TEXT("(empty: tie a thread)"), On ? 12.5f : 11.5f, On ? 0x1e2430 : 0x9a9182, false, Width - 46)]]]];
			}
			const TArray<FString>* Have = RegionHave.Find(Q.Region);
			const TArray<FString>* Missing = RegionMissing.Find(Q.Region);
			FString Pieces;
			if (Have && Have->Num()) Pieces = TEXT("On the board here: ") + FString::Join(*Have, TEXT(", ")) + TEXT(".");
			if (Missing && Missing->Num()) Pieces += (Pieces.IsEmpty() ? FString() : FString(TEXT(" "))) + TEXT("Still to find: ") + FString::Join(*Missing, TEXT(", ")) + TEXT(".");
			if (!Pieces.IsEmpty() && !Ready) Body->AddSlot().AutoHeight().Padding(10, 2, 10, 6)[Text(Pieces, 11.5f, 0x6b6458, false, Width - 20, 1.2f)];
		}
		else
		{
			// ui/Board.ts pencil(): each line beginning "~ " is in Reenie Beanie.
			TArray<FString> Lines; Q.Elsewhere.ParseIntoArray(Lines, TEXT("\n"), false);
			TSharedRef<SVerticalBox> Else = SNew(SVerticalBox);
			for (const FString& L : Lines)
			{
				const bool Pencil = L.StartsWith(TEXT("~ "));
				Else->AddSlot().AutoHeight().Padding(Pencil ? 6 : 0, Pencil ? 2 : 0, 0, 0)
				[SNew(STextBlock).Text(FText::FromString(Pencil ? L.Mid(2) : L)).Font(Pencil ? S47Style::Hand(25) : S47Style::Body(12.5f))
				.ColorAndOpacity(Css(Pencil ? 0x3f3e47 : 0x5d564a)).WrapTextAt(Width - 20).LineHeightPercentage(Pencil ? 1.05f : 1.12f)];
			}
			Body->AddSlot().AutoHeight().Padding(10, 4, 10, 6)[Else];
		}
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		if (Ready) Row->AddSlot().AutoWidth().Padding(0, 0, 8, 0)
			[Button(TEXT("RECORD"), [this, Id = Q.Id] { const FS47Result R = O.Record ? O.Record(Id) : FS47Result(); Say(R.Text, R.bOk ? 1 : -1); if (R.bOk) Render(); }, 19, 0xfff3e2, 44, 36, true)];
		Row->AddSlot().AutoWidth()[Button(TEXT("Hint"), [this, Id = Q.Id] { ShowHint(Id); }, 16, 0x233a7a, 44, 36)];
		Body->AddSlot().AutoHeight().Padding(10, 0)[Row];
	}
	TSharedPtr<FSlateBrush> Card = ArtBrushes.FindRef(TEXT("card_index_blank"));
	if (!Card) return SNew(SBorder)
		.BorderImage_Lambda([this, Ready, Id = Q.Id] { return Ready || Selected == Id ? Box(0xf4efe4, 0xc4241c, 1, 1, 2) : Box(0xf4efe4); })
		.Padding(FMargin(0, 4, 0, 8))[Body];
	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(Box(0xf4efe4)).Padding(0)[SNew(SS47BoardPaper).Brush(Card.Get())]]
		+ SOverlay::Slot()[SNew(SBorder).BorderImage_Lambda([this, Ready, Id = Q.Id] {
			return Ready || Selected == Id ? Box(0, 0xc4241c, 0, 1, 2) : Box(0, 0, 0, 0);
		}).Padding(FMargin(0, 4, 0, 8))[Body]];
}

// ui/Board.ts placeFree(): the first unoccupied spot in the chapter's patch, kept in the save.
void FS47BoardImpl::PlaceFree()
{
	TArray<TSharedPtr<FBox>> Taken;
	for (const TSharedPtr<FBox>& B : Boxes) if (!B->bFree) Taken.Add(B);
	for (const TSharedPtr<FBox>& B : Boxes)
	{
		if (!B->bFree) continue;
		TOptional<FS47BoardRegion> Region = RegionOf(B->Region);
		if (!Region && !Regions().IsEmpty()) Region = Regions()[0];
		if (!Region) { FS47BoardRegion R; R.X = 20; R.Y = 20; R.W = O.Size.X - 40; R.H = O.Size.Y - 40; Region = R; }
		const FS47BoardRegion& R = *Region;
		TOptional<FVector2D> At;
		for (double Y = R.Y + 14; !At && Y + FMath::Min(B->Size.Y, 120.0) <= R.Y + R.H; Y += 18)
		{
			for (double X = R.X + 14; X + B->Size.X <= R.X + R.W; X += 18)
			{
				const FVector2D P(X, Y);
				if (!Taken.ContainsByPredicate([&](const TSharedPtr<FBox>& T) { return Overlap(P, B->Size, T->At, T->Size, 14); })) { At = P; break; }
			}
		}
		B->At = At.Get(FVector2D(R.X + R.W - B->Size.X - 14 - (Taken.Num() % 6) * 12, R.Y + 14 + (Taken.Num() % 6) * 22));
		B->bFree = false;
		Taken.Add(B);
		if (O.LayoutSet) O.LayoutSet(B->Id, B->At.X, B->At.Y);
	}
}

void FS47BoardImpl::Render()
{
	if (!bTextToggleRender) TextScrolls.Empty();
	StatusEl->SetText(FText::FromString(O.Status ? O.Status() : FString()));
	HowEl->SetText(FText::FromString(bCoarse
		? TEXT("Drag a card to move it. Drag the cork to look around; pinch to zoom. Tap one piece of evidence, then another, to tie a red thread between them.")
		: TEXT("Drag the cards about. Pull a red thread from a pin to another piece of evidence, or click one and then the other.")));
	Canvas->ClearChildren();
	Boxes.Empty();
	const TArray<FS47BoardCard> CardList = O.Cards ? O.Cards() : TArray<FS47BoardCard>();
	RegionHave.Reset(); RegionMissing.Reset();
	for (const FS47BoardCard& C : CardList)
	{
		if (C.Stamp.IsEmpty() || C.Kind == TEXT("finding")) continue;
		TArray<FString>& List = (C.bHave ? RegionHave : RegionMissing).FindOrAdd(C.Region);
		if (List.Num() < 7) List.AddUnique(C.Stamp);
	}
	for (const FS47BoardQuestion& Q : O.Questions ? O.Questions() : TArray<FS47BoardQuestion>())
	{
		TSharedPtr<FBox> B = MakeShared<FBox>(); B->Id = Q.Id;
		B->Widget = QuestionHtml(Q, B);
		AddBox(B, Q.Region, Q.X, Q.Y, Q.W.Get(260));
	}
	for (const FS47BoardCard& C : CardList)
	{
		TSharedPtr<FBox> B = MakeShared<FBox>(); B->Id = C.Id;
		B->Widget = CardHtml(C, B);
		AddBox(B, C.Region, C.X, C.Y, C.W.Get(C.Kind == TEXT("photo") || C.Kind == TEXT("map") ? 220 : 240));
	}
	PlaceFree();
	// ui/Board.ts render(): notes near their two pins, without covering another card or a pin.
	for (const FS47BoardNote& N : O.Notes ? O.Notes() : TArray<FS47BoardNote>())
	{
		TSharedPtr<FBox> B = MakeShared<FBox>(); B->Id = N.Id; B->bNote = true; B->bHand = N.bHand;
		const TOptional<FVector2D> Saved = PlaceOf(N.Id);
		B->Widget = SNew(SBorder).BorderImage(N.bHand ? Box(0, 0, 0, 0) : Box(0xf1dc7c)).Padding(N.bHand ? FMargin(12, 3, 12, 1) : FMargin(10, 14, 10, 8))
			[SNew(STextBlock).Text(FText::FromString(N.Text)).Font(N.bHand ? S47Style::Hand(34) : S47Style::Body(12.5f))
			.ColorAndOpacity(Css(N.bHand ? 0x3f3e47 : 0x1e2430)).WrapTextAt(N.bHand ? 0 : 170).LineHeightPercentage(N.bHand ? 1.f : 1.28f)];
		if (N.bHand) B->Widget = SNew(SOverlay) + SOverlay::Slot()[SNew(SS47BoardScrap)] + SOverlay::Slot()[B->Widget.ToSharedRef()];
		B->Widget->SlatePrepass();
		const double Width = N.bHand ? B->Widget->GetDesiredSize().X : 190;
		AddBox(B, N.Region, {}, {}, Width);
		B->bFree = false;
		if (Saved) { B->At = *Saved; continue; }
		const TOptional<FVector2D> A = Centre(N.FromA), C = Centre(N.FromB);
		const FVector2D Want = A && C ? (*A + *C) / 2 + FVector2D(-95, 18) : FVector2D(60, 60);
		auto Free = [&](const FVector2D& At) {
			return At.X >= 4 && At.Y >= 4 && At.X + B->Size.X <= O.Size.X - 4 && At.Y + B->Size.Y <= O.Size.Y - 4
				&& !Boxes.ContainsByPredicate([&](const TSharedPtr<FBox>& T) { return T != B && Overlap(At, B->Size, T->At, T->Size, 10); });
		};
		TOptional<FVector2D> At;
		if (N.bHand && A && C)
		{
			const FCurve Q = Curve(*A, *C);
			double Best = TNumericLimits<double>::Max();
			for (double K = 0.1; K <= 0.9001; K += 0.04)
			{
				const FVector2D Pt = Q.At(K);
				const FVector2D Here(FMath::RoundToDouble(Pt.X - B->Size.X / 2), FMath::RoundToDouble(Pt.Y - B->Size.Y * 0.3));
				if (Here.X < 4 || Here.Y < 4 || Here.X + B->Size.X > O.Size.X - 4 || Here.Y + B->Size.Y > O.Size.Y - 4) continue;
				bool bPin = false;
				for (const TSharedPtr<FBox>& Other : Boxes) for (const FPin& Pin : Other->Pins)
				{
					const FVector2D P = Other->At + Pin.Offset;
					if (P.X > Here.X - 12 && P.X < Here.X + B->Size.X + 12 && P.Y > Here.Y - 12 && P.Y < Here.Y + B->Size.Y + 12) bPin = true;
				}
				if (bPin) continue;
				double Cover = 0;
				for (const TSharedPtr<FBox>& T : Boxes) if (T != B)
					Cover += FMath::Max(0.0, FMath::Min(Here.X + B->Size.X, T->At.X + T->Size.X) - FMath::Max(Here.X, T->At.X))
						* FMath::Max(0.0, FMath::Min(Here.Y + B->Size.Y, T->At.Y + T->Size.Y) - FMath::Max(Here.Y, T->At.Y));
				const double Score = Cover + FMath::Abs(K - 0.5) * 40;
				if (Score < Best) { Best = Score; At = Here; }
			}
		}
		for (int32 R = 0; !At && R <= 900; R += 24)
		{
			const int32 M = FMath::Max(1, FMath::RoundToInt(double(R) / 12));
			for (int32 K = 0; K < M; ++K)
			{
				const double Ang = (double(K) / M) * PI * 2;
				const FVector2D Here(FMath::RoundToDouble(Want.X + FMath::Cos(Ang) * R), FMath::RoundToDouble(Want.Y + FMath::Sin(Ang) * R));
				if (Free(Here)) { At = Here; break; }
			}
		}
		B->At = At.Get(FVector2D(FMath::RoundToDouble(FMath::Clamp(Want.X, 4.0, O.Size.X - B->Size.X - 4)), FMath::RoundToDouble(FMath::Clamp(Want.Y, 4.0, O.Size.Y - B->Size.Y - 4))));
		if (O.LayoutSet) O.LayoutSet(N.Id, B->At.X, B->At.Y);
	}
	for (const FS47BoardRegion& R : Regions())
	{
		if (R.Label.IsEmpty()) continue;
		FSlateFontInfo Font = S47Style::Body(18); Font.LetterSpacing = 120;
		TSharedRef<SWidget> Label = SNew(SBorder).BorderImage(Box(0xe8dcc0)).Padding(FMargin(12, 2, 12, 0))
			[SNew(STextBlock).Text(FText::FromString(R.Label)).Font(Font).ColorAndOpacity(Css(0x2b2116))];
		Label->SlatePrepass();
		Canvas->AddSlot().Position(FVector2D(R.X, R.Y - 34)).Size(Label->GetDesiredSize())[Label];
		Label->SetRenderTransform(FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(-0.6f))));
		Label->SetRenderTransformPivot(FVector2D(0.5, 0.5));
	}
	Boxes.StableSort([](const TSharedPtr<FBox>& A, const TSharedPtr<FBox>& B) { return A->Z < B->Z; });
	for (const TSharedPtr<FBox>& B : Boxes)
	{
		Canvas->AddSlot().Position_Lambda([B] { return B->At; }).Size_Lambda([B] { return B->Size; })[B->Widget.ToSharedRef()];
		const TWeakPtr<FBox> WeakBox = B;
		B->Widget->SetRenderTransform(TAttribute<TOptional<FSlateRenderTransform>>::CreateLambda([WeakBox] {
			const TSharedPtr<FBox> BoxValue = WeakBox.Pin();
			if (!BoxValue) return TOptional<FSlateRenderTransform>();
			const double Angle = BoxValue->bLift ? -0.8 : BoxValue->bNote ? BoxValue->bHand ? -5.0 : -1.2 : 0;
			return TOptional<FSlateRenderTransform>(FSlateRenderTransform(Concatenate(FScale2D(BoxValue->bLift ? 1.02 : 1.0), FQuat2D(FMath::DegreesToRadians(Angle)))));
		}));
		B->Widget->SetRenderTransformPivot(FVector2D(0.5, 0.5));
	}
}

FS47Result FS47BoardImpl::Link(const FString& A, const FString& B)
{
	Selected.Empty();
	const FS47Result R = O.Connect ? O.Connect(A, B) : FS47Result();
	Say(R.Text, R.bOk ? 1 : -1);
	if (R.bOk)
	{
		Render();
		LastProgress = FPlatformTime::Seconds(); Misses = 0;
		OfferHint(FString());
	}
	else
	{
		const TOptional<FVector2D> Pa = Centre(A), Pb = Centre(B);
		if (Pa && Pb) Slack.Add({ Curve(*Pa, *Pb, 0.4, 140), FPlatformTime::Seconds() });
		// A thread that does not hold: offer the hint of the open question in that patch.
		++Misses;
		FString Region;
		for (const TSharedPtr<FBox>& Bx : Boxes) if (Bx->Pins.ContainsByPredicate([&](const FPin& Pin) { return Pin.Id == A; })) { Region = Bx->Region; break; }
		OfferHint(OpenQuestionIn(Region));
	}
	return R;
}

void FS47BoardImpl::TapNode(const FString& N)
{
	if (!Selected.IsEmpty() && Selected != N) { Link(Selected, N); return; }
	Selected = Selected == N ? FString() : N;
	if (!Selected.IsEmpty()) Say(TEXT("Now the piece of evidence it goes with."));
}

// The open question to offer a hint for: one with lines still to fill in the given patch, else
// in the patch the board opened on, else any.
FString FS47BoardImpl::OpenQuestionIn(const FString& Region) const
{
	const TArray<FS47BoardQuestion> Qs = O.Questions ? O.Questions() : TArray<FS47BoardQuestion>();
	auto Unfilled = [](const FS47BoardQuestion& Q) { return Q.Done.IsEmpty() && Q.Slots > 0 && Q.Filled.Num() < Q.Slots; };
	for (const FString& Rg : { Region, O.Focus })
		for (const FS47BoardQuestion& Q : Qs) if (Unfilled(Q) && !Rg.IsEmpty() && Q.Region == Rg) return Q.Id;
	for (const FS47BoardQuestion& Q : Qs) if (Unfilled(Q)) return Q.Id;
	return FString();
}

void FS47BoardImpl::OfferHint(const FString& Q)
{
	Offer = O.Hint ? Q : FString();
	if (OfferText) OfferText->SetText(FText::FromString(Offer.IsEmpty() ? FString() : FString::Printf(TEXT("STUCK ON %s? HINT"), *Offer)));
	if (OfferEl) OfferEl->SetVisibility(Offer.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible);
}

void FS47BoardImpl::ShowHint(const FString& Q)
{
	const FString Text = O.Hint ? O.Hint(Q) : FString();
	OfferHint(FString());
	if (Text.IsEmpty()) { Say(TEXT("No hint for that one.")); return; }
	Hinted = Q;
	if (!HintBox) { Say(Text); return; }
	HintHead->SetText(FText::FromString(FString::Printf(TEXT("HINT  /  %s"), *Q)));
	HintText->SetText(FText::FromString(Text));
	HintBox->SetVisibility(EVisibility::Visible);
	LastProgress = FPlatformTime::Seconds();
}

void FS47BoardImpl::ShowHow(bool bShow)
{
	GHowHidden = !bShow;
	if (HowStrip) HowStrip->SetVisibility(bShow ? EVisibility::Visible : EVisibility::Collapsed);
}

void FS47BoardImpl::ClampView()
{
	const double W = O.Size.X, H = O.Size.Y, Vw = ViewSize.X, Vh = ViewSize.Y, M = 60;
	Tx = W * S + 2 * M <= Vw ? (Vw - W * S) / 2 : FMath::Clamp(Tx, Vw - W * S - M, M);
	Ty = H * S + 2 * M <= Vh ? (Vh - H * S) / 2 : FMath::Clamp(Ty, Vh - H * S - M, M);
}

void FS47BoardImpl::ZoomAt(double Ns, const FVector2D& P)
{
	Ns = FMath::Clamp(Ns, MinScale(), 1.6);
	Tx = P.X - (P.X - Tx) * Ns / S; Ty = P.Y - (P.Y - Ty) * Ns / S; S = Ns;
	ClampView();
	if (Drag.Kind != FDrag::EKind::Pan) Follow();
}

// Xbox view actions leave card placement and yarn on the existing pointer path.
bool FS47BoardImpl::PadView(const FKey& K)
{
	const int32 X = K == EKeys::Gamepad_DPad_Left ? 1 : K == EKeys::Gamepad_DPad_Right ? -1 : 0;
	const int32 Y = K == EKeys::Gamepad_DPad_Up ? 1 : K == EKeys::Gamepad_DPad_Down ? -1 : 0;
	if (X || Y) { Tx += X * 80; Ty += Y * 80; ClampView(); Follow(); return true; }
	if (K == EKeys::Gamepad_LeftShoulder || K == EKeys::Gamepad_RightShoulder)
	{
		ZoomAt(S * (K == EKeys::Gamepad_LeftShoulder ? 0.8 : 1.25), ViewSize / 2);
		return true;
	}
	if (K == EKeys::Gamepad_FaceButton_Left) { S = MinScale(); ClampView(); return true; }
	if (K == EKeys::Gamepad_FaceButton_Top) { ShowHow(GHowHidden); return true; }
	return false;
}

void FS47BoardImpl::Look(const FS47BoardRegion& R)
{
	if (ViewSize.X <= 0 || ViewSize.Y <= 0) return;
	S = Small() ? 0.8 : FMath::Clamp(FMath::Min(ViewSize.X / (R.W + 24), ViewSize.Y / (R.H + 24)), 0.45, 1.25);
	Tx = R.W * S <= ViewSize.X ? ViewSize.X / 2 - (R.X + R.W / 2) * S : 12 - R.X * S;
	Ty = R.H * S <= ViewSize.Y ? ViewSize.Y / 2 - (R.Y + R.H / 2) * S : 12 - (R.Y - 30) * S;
	ClampView();
}

bool FS47BoardImpl::Frame(const TArray<FString>& Ids)
{
	if (Ids.IsEmpty()) return false;
	TArray<FVector2D> All;
	for (const FString& Id : Ids)
	{
		const TOptional<FVector2D> C = Centre(Id);
		if (C) { All.Add(*C); continue; }
		const TSharedPtr<FBox>* B = Boxes.FindByPredicate([&](const TSharedPtr<FBox>& V) { return V->Id == Id && !V->bFree; });
		if (!B) return false;
		All.Add((*B)->At); All.Add((*B)->At + (*B)->Size);
	}
	if (ViewSize.X <= 0 || ViewSize.Y <= 0) { PendingFrame = Ids; return true; }
	FVector2D Min = All[0], Max = All[0];
	for (const FVector2D& P : All) { Min.X = FMath::Min(Min.X, P.X); Min.Y = FMath::Min(Min.Y, P.Y); Max.X = FMath::Max(Max.X, P.X); Max.Y = FMath::Max(Max.Y, P.Y); }
	S = FMath::Clamp(FMath::Min(ViewSize.X / (Max.X - Min.X + 160), ViewSize.Y / (Max.Y - Min.Y + 200)), FMath::Max(MinScale(), 0.32), 1.0);
	Tx = ViewSize.X / 2 - (Min.X + Max.X) / 2 * S; Ty = ViewSize.Y / 2 - (Min.Y + Max.Y) / 2 * S;
	ClampView();
	return true;
}

bool FS47BoardImpl::Show(const FString& Id)
{
	const TOptional<FVector2D> C = Centre(Id);
	if (!C) return false;
	if (ViewSize.X <= 0 || ViewSize.Y <= 0) { PendingShow = Id; return true; }
	Tx = ViewSize.X / 2 - C->X * S; Ty = ViewSize.Y / 2 - C->Y * S;
	ClampView();
	return true;
}

// ui/Board.ts follow(): after a pan or zoom, keep the grabbed point under the pointer.
void FS47BoardImpl::Follow()
{
	if (Drag.Kind == FDrag::EKind::Card && Drag.bMoved && Drag.Box)
	{
		const FVector2D P = ToBoardXY(Drag.Now) - Drag.Grab;
		Drag.Box->At = FVector2D(FMath::Clamp(P.X, 0.0, O.Size.X - 60), FMath::Clamp(P.Y, 0.0, O.Size.Y - 40));
	}
}

void FS47BoardImpl::Edge()
{
	if (Drag.Kind == FDrag::EKind::None || Drag.Kind == FDrag::EKind::Pan || !Drag.bMoved) return;
	const int32 Vx = Drag.Now.X < 36 ? 1 : Drag.Now.X > ViewSize.X - 36 ? -1 : 0;
	const int32 Vy = Drag.Now.Y < 36 ? 1 : Drag.Now.Y > ViewSize.Y - 36 ? -1 : 0;
	if (!Vx && !Vy) return;
	Tx += Vx * 10; Ty += Vy * 10; ClampView(); Follow();
}

FS47BoardImpl::FHit FS47BoardImpl::Hit(const FVector2D& P) const
{
	// The card moved last is on top for both the Slate child order and evidence picking.
	for (int32 I = Boxes.Num() - 1; I >= 0; --I)
	{
		const TSharedPtr<FBox>& B = Boxes[I];
		RefreshPins(B);
		for (const FPin& Pin : B->Pins)
		{
			// The pin's ::after extends the hit area by 12px on each side.
			const double Radius = ArtBrushes.FindRef(TEXT("pin_red")) ? 23 : 19;
			if (Inside(P, B->At + Pin.Offset - FVector2D(Radius), FVector2D(Radius * 2))) return { B, Pin.Id, Pin.Id };
		}
		if (!Inside(P, B->At, B->Size)) continue;
		for (const FPin& Pin : B->Pins) if (Pin.LineWidget && Inside(P, B->At + Pin.LineAt, Pin.LineSize)) return { B, Pin.Id, Pin.Id };
		return { B, FString(), B->Node };
	}
	return {};
}

bool FS47BoardImpl::Scrolling(const FHit& H, const FVector2D& P) const
{
	if (!H.Box || !H.Box->Full || H.Box->Full->GetScrollOffsetOfEnd() <= 2 || !Surface) return false;
	const FGeometry& G = H.Box->Full->GetCachedGeometry();
	return G.IsUnderLocation(Surface->GetCachedGeometry().LocalToAbsolute(P));
}

void FS47BoardImpl::Down(int32 Id, const FVector2D& P, bool bTouch)
{
	if (bTouch && !bCoarse) { bCoarse = true; HowEl->SetText(FText::FromString(TEXT("Drag a card to move it. Drag the cork to look around; pinch to zoom. Tap one piece of evidence, then another, to tie a red thread between them."))); }
	Ptrs.Add(Id, P);
	if (Ptrs.Num() == 2)
	{
		if (Drag.Kind == FDrag::EKind::Card && Drag.bMoved && Drag.Box)
		{
			if (O.LayoutSet) O.LayoutSet(Drag.Box->Id, Drag.Box->At.X, Drag.Box->At.Y);
			Drag.Box->bLift = false;
			bChanged = true;
		}
		Drag = {};
		TArray<FVector2D> Points; Ptrs.GenerateValueArray(Points);
		Pinch = FPinch{ FMath::Max(1.0, (Points[1] - Points[0]).Size()), S, (Points[0] + Points[1]) / 2 };
		return;
	}
	if (Ptrs.Num() > 2) return;
	Drag = {};
	Drag.Start = Drag.Now = P;
	const FHit H = Hit(ToBoardXY(P));
	if (!H.Pull.IsEmpty()) { Drag.Kind = FDrag::EKind::Thread; Drag.From = H.Pull; }
	else if (H.Box) { Drag.Kind = FDrag::EKind::Card; Drag.Box = H.Box; Drag.Grab = ToBoardXY(P) - H.Box->At; }
	else { Drag.Kind = FDrag::EKind::Pan; Drag.T0 = FVector2D(Tx, Ty); }
}

void FS47BoardImpl::Move(int32 Id, const FVector2D& P)
{
	if (!Ptrs.Contains(Id)) return;
	Ptrs[Id] = P;
	if (Pinch && Ptrs.Num() >= 2)
	{
		TArray<FVector2D> Points; Ptrs.GenerateValueArray(Points);
		const FVector2D Mid = (Points[0] + Points[1]) / 2;
		Tx += Mid.X - Pinch->Mid.X; Ty += Mid.Y - Pinch->Mid.Y; Pinch->Mid = Mid;
		ZoomAt(Pinch->S0 * (Points[1] - Points[0]).Size() / Pinch->D0, Mid);
		return;
	}
	if (Drag.Kind == FDrag::EKind::None) return;
	Drag.Now = P;
	if (!Drag.bMoved)
	{
		if ((P - Drag.Start).Size() < (Drag.Kind == FDrag::EKind::Thread ? 8 : 6)) return;
		Drag.bMoved = true;
		if (Drag.Kind == FDrag::EKind::Card && Drag.Box)
		{
			const TSharedPtr<FBox> B = Drag.Box;
			B->bLift = true; B->Z = ++Top; Zs.Add(B->Id, Top);
			Boxes.Remove(B); Boxes.Add(B);
			Canvas->RemoveSlot(B->Widget.ToSharedRef());
			Canvas->AddSlot().Position_Lambda([B] { return B->At; }).Size_Lambda([B] { return B->Size; })[B->Widget.ToSharedRef()];
		}
	}
	if (Drag.Kind == FDrag::EKind::Pan)
	{
		Tx = Drag.T0.X + P.X - Drag.Start.X; Ty = Drag.T0.Y + P.Y - Drag.Start.Y;
		ClampView(); return;
	}
	Follow();
}

void FS47BoardImpl::End(int32 Id, const FVector2D& P, bool bCancel)
{
	if (!Ptrs.Remove(Id)) return;
	if (Pinch) { if (Ptrs.Num() < 2) Pinch.Reset(); return; }
	const FDrag D = Drag; Drag = {};
	if (D.Box) D.Box->bLift = false;
	if (D.Kind == FDrag::EKind::None || bCancel) return;
	if (D.Kind == FDrag::EKind::Pan)
	{
		if (!D.bMoved && !Selected.IsEmpty()) { Selected.Empty(); Say(FString()); }
		return;
	}
	if (D.Kind == FDrag::EKind::Card)
	{
		if (!D.Box) return;
		if (D.bMoved)
		{
			if (O.LayoutSet) O.LayoutSet(D.Box->Id, D.Box->At.X, D.Box->At.Y);
			bChanged = true; return;
		}
		if (!D.Box->Node.IsEmpty()) TapNode(D.Box->Node);
		else if (D.Box->bLines) Say(TEXT("Each line that matters on this record has its own pin. Tap the line, then what it goes with."));
		return;
	}
	if (D.bMoved)
	{
		// DOM elementFromPoint cannot find evidence hidden beyond the clipped viewport.
		if (!Inside(P, FVector2D::ZeroVector, ViewSize)) return;
		const FHit H = Hit(ToBoardXY(P));
		if (!H.Node.IsEmpty() && H.Node != D.From) Link(D.From, H.Node);
		return;
	}
	TapNode(D.From);
}

void FS47BoardImpl::Tick(const FVector2D& InSize)
{
	if (InSize != ViewSize) { ViewSize = InSize; ClampView(); }
	if (bInitialFocus && ViewSize.X > 0 && ViewSize.Y > 0)
	{
		bInitialFocus = false;
		TOptional<FS47BoardRegion> Focus = RegionOf(O.Focus);
		if (!Focus && !Regions().IsEmpty()) Focus = Regions()[0];
		if (Focus) Look(*Focus); else { S = MinScale(); ClampView(); }
		if (Small() && SayEl->GetText().IsEmpty()) Say(HowEl->GetText().ToString());
	}
	if (!PendingLook.IsEmpty())
	{
		const TOptional<FS47BoardRegion> R = RegionOf(PendingLook);
		if (R) Look(*R);
		PendingLook.Empty();
	}
	if (!PendingFrame.IsEmpty()) { const TArray<FString> Ids = MoveTemp(PendingFrame); PendingFrame.Empty(); Frame(Ids); }
	if (!PendingShow.IsEmpty()) { const FString Id = MoveTemp(PendingShow); PendingShow.Empty(); Show(Id); }
	const bool bSmall = Small();
	HowEl->SetVisibility(bSmall || (HowStrip && !GHowHidden) ? EVisibility::Collapsed : EVisibility::HitTestInvisible);
	if (HowStrip && bSmall) HowStrip->SetVisibility(EVisibility::Collapsed);
	else if (HowStrip) HowStrip->SetVisibility(GHowHidden ? EVisibility::Collapsed : EVisibility::Visible);
	SayEl->SetFont(S47Style::Body(bSmall ? 14 : 15.5f));
	Slack.RemoveAll([](const FSlack& T) { return FPlatformTime::Seconds() - T.Born >= 2.0; });
	// A while on the board without a thread that holds: offer the hint once.
	const double Now = FPlatformTime::Seconds();
	if (LastProgress <= 0) LastProgress = Now;
	if (Offer.IsEmpty() && Now - LastProgress > 75 && Drag.Kind == FDrag::EKind::None && !(HintBox && HintBox->GetVisibility() == EVisibility::Visible))
	{
		OfferHint(OpenQuestionIn(O.Focus));
		LastProgress = Now;
	}
	if (!TestFrom.IsEmpty() && ViewSize.X > 0)
	{
		// -S47BoardTest drag: the thread held from one pin over another (the screenshots)
		const TOptional<FVector2D> A = Centre(TestFrom), B = Centre(TestTo);
		if (A && B)
		{
			Drag = {}; Drag.Kind = FDrag::EKind::Thread; Drag.From = TestFrom; Drag.bMoved = true;
			Drag.Start = *A * S + FVector2D(Tx, Ty); Drag.Now = *B * S + FVector2D(Tx, Ty) + FVector2D(3, 2);
		}
	}
	// requestAnimationFrame(edge) becomes the Slate viewport tick, once per presented frame.
	Edge();
	if (Surface) Surface->Invalidate(EInvalidateWidgetReason::Paint);
}

int32 FS47BoardImpl::Paint(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer, bool bAbove) const
{
	const FGeometry Board = G.MakeChild(O.Size, FSlateLayoutTransform(S, FVector2D(Tx, Ty)));
	auto PaintBox = [&](const FVector2D& At, const FVector2D& Size, const FSlateBrush* Brush, const FLinearColor& Tint = FLinearColor::White, int32 L = 0) {
		FSlateDrawElement::MakeBox(Out, Layer + L, Board.ToPaintGeometry(Size, FSlateLayoutTransform(At)), Brush, ESlateDrawEffect::None, Tint * Brush->GetTint(FWidgetStyle()));
	};
	auto Tile = [&](const FSlateBrush& Original, double Pitch) {
		for (double Y = 0; Y < O.Size.Y; Y += Pitch) for (double X = 0; X < O.Size.X; X += Pitch)
		{
			const FVector2D Size(FMath::Min(Pitch, O.Size.X - X), FMath::Min(Pitch, O.Size.Y - Y));
			FSlateBrush Part = Original; Part.DrawAs = ESlateBrushDrawType::Image; Part.Tiling = ESlateBrushTileType::NoTile;
			Part.SetUVRegion(FBox2f(FVector2f::ZeroVector, FVector2f(Size / Pitch)));
			PaintBox(FVector2D(X, Y), Size, &Part);
		}
	};
	auto Rotated = [&](const FVector2D& At, const FVector2D& Size, const FSlateBrush* Brush, double Angle, const FLinearColor& Tint = FLinearColor::White, bool bOrigin = false) {
		FSlateDrawElement::MakeRotatedBox(Out, Layer, Board.ToPaintGeometry(Size, FSlateLayoutTransform(At)), Brush, ESlateDrawEffect::None,
			Angle, bOrigin ? TOptional<FVector2D>(FVector2D::ZeroVector) : TOptional<FVector2D>(), FSlateDrawElement::RelativeToElement, Tint * Brush->GetTint(FWidgetStyle()));
	};
	auto Line = [&](const TArray<FVector2D>& Points, const FLinearColor& Colour, double Thickness) {
		FSlateDrawElement::MakeLines(Out, Layer, Board.ToPaintGeometry(), Points, ESlateDrawEffect::None, Colour, true, Thickness);
	};
	auto Ring = [&](const FVector2D& At, double Radius, const FLinearColor& Colour, double Thickness) {
		TArray<FVector2D> Points;
		for (int32 I = 0; I <= 64; ++I) Points.Add(At + FVector2D(FMath::Cos(I * PI / 32), FMath::Sin(I * PI / 32)) * Radius);
		Line(Points, Colour, Thickness);
	};
	if (!bAbove)
	{
		FSlateDrawElement::MakeBox(Out, Layer++, G.ToPaintGeometry(), Box(0x1b120b), ESlateDrawEffect::None, Css(0x1b120b));
		const TSharedPtr<FSlateBrush> Cork = ArtBrushes.FindRef(TEXT("tex_cork"));
		if (Cork) Tile(*Cork, 512);
		else
		{
			PaintBox(FVector2D::ZeroVector, O.Size, Box(0x8a5f3a));
			// CSS radial gradients are a repeated image in Slate, rather than thousands of draw calls.
			static TSharedPtr<FSlateBrush> Pattern;
			if (!Pattern)
			{
				TArray<FColor> Pixels; Pixels.SetNum(77 * 77);
				for (int32 Y = 0; Y < 77; ++Y) for (int32 X = 0; X < 77; ++X)
				{
					const double Dx = FMath::Min(X % 7, 7 - X % 7), Dy = FMath::Min(Y % 7, 7 - Y % 7);
					const double Lx = (X - 3 + 77) % 11, Ly = (Y - 5 + 77) % 11;
					const double Dark = FMath::Clamp((1.6 - FMath::Sqrt(Dx * Dx + Dy * Dy)) / 0.6, 0.0, 1.0) * 0.18;
					const double Light = FMath::Clamp((1.8 - FMath::Sqrt(FMath::Square(FMath::Min(Lx, 11 - Lx)) + FMath::Square(FMath::Min(Ly, 11 - Ly)))) / 0.8, 0.0, 1.0) * 0.12;
					Pixels[Y * 77 + X] = FColor(uint8(138 * (1 - Dark) * (1 - Light) + 255 * Light), uint8(95 * (1 - Dark) * (1 - Light) + 220 * Light), uint8(58 * (1 - Dark) * (1 - Light) + 170 * Light));
				}
				Pattern = S47Kit::BrushFor(S47Image::FromPixels(Pixels, 77, 77));
				if (Pattern) Pattern->Tiling = ESlateBrushTileType::NoTile;
			}
			if (Pattern) Tile(*Pattern, 77);
		}
		++Layer;
		const TSharedPtr<FSlateBrush> Frame = ArtBrushes.FindRef(TEXT("frame_wood"));
		if (Frame)
		{
			// CSS border-image:64 / 40px stretch. Native Slate Border keeps physical texel margins.
			const double X[] = { 0, 40, O.Size.X - 40, O.Size.X }, Y[] = { 0, 40, O.Size.Y - 40, O.Size.Y };
			const double U[] = { 0, 64.0 / 2048, 1 - 64.0 / 2048, 1 }, V[] = { 0, 64.0 / 1280, 1 - 64.0 / 1280, 1 };
			for (int32 J = 0; J < 3; ++J) for (int32 I = 0; I < 3; ++I)
			{
				if (I == 1 && J == 1) continue;
				FSlateBrush Part = *Frame; Part.DrawAs = ESlateBrushDrawType::Image; Part.Tiling = ESlateBrushTileType::NoTile; Part.Margin = FMargin(0);
				Part.SetUVRegion(FBox2f(FVector2f(U[I], V[J]), FVector2f(U[I + 1], V[J + 1])));
				PaintBox(FVector2D(X[I], Y[J]), FVector2D(X[I + 1] - X[I], Y[J + 1] - Y[J]), &Part);
			}
		}
		else
		{
			PaintBox(FVector2D::ZeroVector, O.Size, Box(0, 0x4a3220, 0, 1, 16));
			PaintBox(FVector2D(16), O.Size - FVector2D(32), Box(0, 0x2e1f14, 0, 1, 2));
		}
		++Layer;
		const FVector2D Stain(FMath::RoundToDouble(O.Size.X * 0.43), FMath::RoundToDouble(O.Size.Y * 0.47));
		const TSharedPtr<FSlateBrush> Coffee = ArtBrushes.FindRef(TEXT("stain_coffee"));
		if (Coffee) Rotated(Stain, FVector2D(220), Coffee.Get(), FMath::DegreesToRadians(18.0), FLinearColor(1, 1, 1, 0.8f));
		else { Ring(Stain + FVector2D(110), 106.5, Css(0x46280f, 0.11f), 7); Ring(Stain + FVector2D(110), 101.5, Css(0x46280f, 0.03f), 5); }
		++Layer;
		for (const TSharedPtr<FBox>& B : Boxes)
		{
			const FVector2D Offset = B->bLift ? FVector2D(6, 14) : B->bHand ? FVector2D(1, 2) : FVector2D(2, 4);
			// Slate boxes approximate the CSS blurred shadows with concentric translucent rectangles.
			for (int32 I = B->bLift ? 12 : B->bHand ? 2 : 5; I >= 0; --I)
				PaintBox(B->At + Offset - FVector2D(I), B->Size + FVector2D(I * 2), Box(0, 0, 0.035f, 0));
		}
		return Layer + 1;
	}
	// ui/Board.ts threads(): draw over the cards, as the SVG's z-index:30000 does.
	// Departs from the web version (Tom: the threads were nearly invisible and badly drawn): the
	// yarn is Slate's anti-aliased lines in passes, its width in screen pixels from the zoom (the
	// line builder takes screen pixels): a soft shadow on the cork that falls further where the
	// yarn sags off it, a dark edge, the red core, a highlight on its upper side and the twist of
	// the ply. The pins go on last, so the yarn runs under their heads.
	const float Px = FMath::Max(0.01f, Board.GetAccumulatedRenderTransform().TransformVector(FVector2f(1, 0)).Size());
	const int32 LShadow = Layer + 1, LEdge = Layer + 2, LCore = Layer + 3, LLight = Layer + 4, LPins = Layer + 5;
	auto Lines = [&](const TArray<FVector2D>& Points, const FLinearColor& Colour, double ScreenWidth, int32 L) {
		FSlateDrawElement::MakeLines(Out, L, Board.ToPaintGeometry(), Points, ESlateDrawEffect::None, Colour, true, float(FMath::Max(1.0, ScreenWidth)));
	};
	auto Sample = [](const FCurve& Q) {
		TArray<FVector2D> Points;
		const int32 N = FMath::Clamp(int32((Q.B - Q.A).Size() / 14), 12, 64);
		for (int32 I = 0; I <= N; ++I) Points.Add(Q.At(double(I) / N));
		return Points;
	};
	// Lift: how far (screen px) the middle of the yarn stands off the cork.
	auto Shadow = [&](const TArray<FVector2D>& Points, double W, double Lift, float Alpha) {
		TArray<FVector2D> Under;
		for (int32 I = 0; I < Points.Num(); ++I)
		{
			const double T = double(I) / FMath::Max(1, Points.Num() - 1);
			Under.Add(Points[I] + FVector2D(0.55, 1) * ((0.3 + FMath::Sin(T * PI)) * Lift / Px));
		}
		Lines(Under, Css(0x1a0c04, Alpha * 0.35f), W * 2.4, LShadow);
		Lines(Under, Css(0x1a0c04, Alpha * 0.55f), W * 1.2, LShadow);
	};
	struct FLook { uint32 Core, Edge, Light; float Alpha = 1; bool bTwist = true, bLight = true; };
	auto Yarn = [&](const TArray<FVector2D>& Points, double W, const FLook& K, double Dash = 0, double Gap = 0) {
		// a dashed thread (one the story drew itself) is the same yarn in pieces
		TArray<TArray<FVector2D>> Runs;
		if (!Dash) Runs.Add(Points);
		else
		{
			double Along = 0; TArray<FVector2D> Run;
			for (int32 I = 0; I < Points.Num(); ++I)
			{
				if (I) Along += (Points[I] - Points[I - 1]).Size() * Px;
				if (FMath::Fmod(Along, Dash + Gap) < Dash) Run.Add(Points[I]);
				else { if (Run.Num() > 1) Runs.Add(Run); Run.Reset(); }
			}
			if (Run.Num() > 1) Runs.Add(Run);
		}
		for (const TArray<FVector2D>& Run : Runs)
		{
			Lines(Run, Css(K.Edge, K.Alpha), W + 1.6, LEdge);
			Lines(Run, Css(K.Core, K.Alpha), W, LCore);
			if (K.bLight && W >= 2.4)
			{
				// the light from above the board catches the upper side of the yarn
				TArray<FVector2D> Top;
				for (int32 I = 0; I < Run.Num(); ++I)
				{
					const FVector2D D = (Run[FMath::Min(I + 1, Run.Num() - 1)] - Run[FMath::Max(I - 1, 0)]).GetSafeNormal();
					FVector2D N(-D.Y, D.X); if (N.Y > 0) N = -N;
					Top.Add(Run[I] + N * (W * 0.24 / Px));
				}
				Lines(Top, Css(K.Light, K.Alpha * 0.6f), FMath::Max(1.0, W * 0.3), LLight);
			}
			if (K.bTwist && W >= 3)
			{
				// the twist of the ply: short dark strokes across the yarn at a slant
				const double Step = W * 1.35 / Px;
				double Next = Step * 0.5, Along = 0;
				for (int32 I = 1; I < Run.Num(); ++I)
				{
					const FVector2D D = Run[I] - Run[I - 1];
					const double Len = D.Size();
					if (Len <= 0) continue;
					const FVector2D T = D / Len, N(-T.Y, T.X);
					while (Next <= Along + Len)
					{
						const FVector2D P = Run[I - 1] + T * (Next - Along);
						const FVector2D Half = (N * 0.4 - T * 0.32) * (W / Px);
						Lines({ P + Half, P - Half }, Css(K.Edge, K.Alpha * 0.5f), FMath::Max(1.0, W * 0.2), LLight);
						Next += Step;
					}
					Along += Len;
				}
			}
		}
	};
	auto Circle = [&](const FVector2D& At, double Radius, const FLinearColor& Colour, double ScreenWidth, int32 L) {
		TArray<FVector2D> Points;
		for (int32 I = 0; I <= 48; ++I) Points.Add(At + FVector2D(FMath::Cos(I * PI / 24), FMath::Sin(I * PI / 24)) * Radius);
		Lines(Points, Colour, ScreenWidth, L);
	};
	// the pin art (a round map pin with its shine), larger than the web's 22 px and never
	// smaller than 22 screen pixels; without the art, a drawn head with a rim and a shine
	const double PinSize = FMath::Max(38.0, 22.0 / Px);
	auto Pin = [&](const FVector2D& At, const FString& Colour) {
		const TSharedPtr<FSlateBrush> B = ArtBrushes.FindRef(TEXT("pin_") + Colour);
		if (B)
		{
			FSlateDrawElement::MakeBox(Out, LPins, Board.ToPaintGeometry(FVector2D(PinSize), FSlateLayoutTransform(At - FVector2D(PinSize / 2))), B.Get(), ESlateDrawEffect::None, B->GetTint(FWidgetStyle()));
			return;
		}
		const double R = PinSize * 0.22;
		const uint32 C = Colour == TEXT("blue") ? 0x2f55b8 : Colour == TEXT("yellow") ? 0xd9b53a : 0xc4241c;
		Circle(At + FVector2D(1.5, 2.5) / Px * 1.5, R * 0.5, Css(0, 0.35f), R * Px, LPins);
		Circle(At, R * 0.5, Css(C), R * Px, LPins);
		Circle(At, R, Css(0, 0.35f), 1.2, LPins);
		Circle(At - FVector2D(R * 0.35), R * 0.18, Css(0xffffff, 0.8f), R * 0.36 * Px, LPins);
	};
	// Pin art and tape occupy the same positions as their CSS absolute elements.
	for (const TSharedPtr<FBox>& B : Boxes)
	{
		RefreshPins(B);
		if (B->bQuestion && !ArtBrushes.FindRef(TEXT("card_index_blank")))
			PaintBox(B->At, FVector2D(B->Size.X, 4), Box(B->bDone ? 0x2f6a3c : 0x8f2a1d));
		if (B->bFinding) PaintBox(B->At, FVector2D(B->Size.X, 4), Box(0x2f6a3c));
		if (B->bPhoto)
		{
			const TSharedPtr<FSlateBrush> Tape = ArtBrushes.FindRef(TEXT("tape_strip"));
			Rotated(B->At + FVector2D(B->Size.X / 2 - 45, Tape ? -14 : -9), FVector2D(90, Tape ? 34 : 22), Tape ? Tape.Get() : Box(0xece4c4, 0, 0.72f, 0), FMath::DegreesToRadians(-3.0));
		}
	}
	// tied threads: red yarn pulled taut between two pins, a little sag; a thread with no note is
	// thinner, one the story drew itself is dashed
	const FLook Tied{ 0xc8261c, 0x5a0b07, 0xff9a84 }, Loose{ 0xb3231b, 0x4e0a07, 0xf58a76, 0.92f };
	for (const FS47BoardThread& T : O.Threads ? O.Threads() : TArray<FS47BoardThread>())
	{
		const TOptional<FVector2D> A = Centre(T.A), B = Centre(T.B);
		if (!A || !B) continue;
		const TArray<FVector2D> Points = Sample(Curve(*A, *B));
		const double W = YarnWidth(T.bAuto ? 2.4 : T.bThin ? 2.8 : 3.8, Px, T.bAuto || T.bThin ? 2.0 : 2.8);
		Shadow(Points, W, 5, T.bAuto ? 0.6f : 1.f);
		Yarn(Points, W, T.bThin || T.bAuto ? Loose : Tied, T.bAuto ? W * 3 : 0, T.bAuto ? W * 2 : 0);
	}
	// a thread that does not hold: grey and slack, it falls away from the pins and fades
	for (const FSlack& T : Slack)
	{
		const double Age = FMath::Clamp((FPlatformTime::Seconds() - T.Born) / 2.0, 0.0, 1.0);
		const double Fall = FMath::Square(Age) * 40;
		FCurve Q = T.Q; Q.A.Y += Fall * 0.4; Q.C.Y += Fall; Q.B.Y += Fall * 0.4;
		const float Alpha = float(1 - FMath::Square(Age));
		const TArray<FVector2D> Points = Sample(Q);
		const double W = YarnWidth(2.6, Px, 2.2);
		Shadow(Points, W, 3, Alpha * 0.6f);
		Yarn(Points, W, FLook{ 0x8f8577, 0x3b332b, 0xd8cfc0, Alpha, false, false }, W * 2.6, W * 1.6);
	}
	// the thread in the hand: bright, held up off the cork, a ring on the pin it would go to
	if (Drag.Kind == FDrag::EKind::Thread && Drag.bMoved)
	{
		const TOptional<FVector2D> A = Centre(Drag.From);
		const FVector2D End = ToBoardXY(Drag.Now);
		if (A)
		{
			const TArray<FVector2D> Points = Sample(Curve(*A, End, 0.06, 30));
			const double W = YarnWidth(3.6, Px, 3.0);
			Shadow(Points, W, 12, 0.8f);
			Yarn(Points, W, FLook{ 0xe3311f, 0x6a0d08, 0xffb3a2 });
			const double Pulse = 0.5 + 0.5 * FMath::Sin(FPlatformTime::Seconds() * 6);
			Circle(*A, PinSize * 0.62, Css(0xffd25a, 0.85f), 2.5, LLight);
			const FHit H = Hit(End);
			if (!H.Node.IsEmpty() && H.Node != Drag.From)
			{
				const FVector2D To = Centre(H.Node).Get(End);
				Circle(To, PinSize * (0.7 + 0.12 * Pulse), Css(0xffd25a, 0.95f), 3.5, LLight);
				Circle(To, PinSize * (0.7 + 0.12 * Pulse) + 3 / Px, Css(0x1a0c04, 0.5f), 1.5, LLight);
			}
			// the loose end of the yarn, a pin in the fingers
			Pin(End, TEXT("red"));
		}
	}
	for (const TSharedPtr<FBox>& B : Boxes)
	{
		for (const FPin& P : B->Pins) Pin(Centre(P.Id).Get(B->At + P.Offset), B->bQuestion ? TEXT("blue") : TEXT("red"));
		if (B->bNote && !B->bHand) Pin(B->At + FVector2D(B->Size.X / 2, 2), TEXT("yellow"));
	}
	return LPins + 1;
}

void SS47Board::Construct(const FArguments& InArgs)
{
	P = MakeShared<FS47BoardImpl>(); P->O = InArgs._Spec; P->UI = InArgs._UI;
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D Size; GEngine->GameViewport->GetViewportSize(Size);
		P->ParentScale = GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(FIntPoint(FMath::RoundToInt(Size.X), FMath::RoundToInt(Size.Y)));
	}
	for (const TCHAR* Name : { TEXT("tex_cork"), TEXT("frame_wood"), TEXT("pin_red"), TEXT("pin_yellow"), TEXT("pin_blue"), TEXT("thread_red"), TEXT("card_index_blank"), TEXT("tape_strip"), TEXT("stain_coffee") })
		P->ArtBrushes.Add(Name, Art(*(FString(TEXT("board_")) + Name)));
	TSharedRef<SHorizontalBox> Head = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 14, 0)
		[SNew(SBox).MaxDesiredWidth_Lambda([this] { return FMath::Clamp(P->ScreenSize().X * 0.28, 80.0, 420.0); })
		[SNew(STextBlock).Text(FText::FromString(P->O.Title)).Font_Lambda([this] { FSlateFontInfo F = S47Style::Mono(P->Small() ? 18 : 24); F.LetterSpacing = P->Small() ? 140 : 300; return F; })
		.ColorAndOpacity(Css(0xffb09a)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]]
		+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(0, 0, 14, 0)
		[SAssignNew(P->StatusEl, STextBlock).Visibility_Lambda([this] { return P->ScreenSize().X <= 520 ? EVisibility::Hidden : EVisibility::HitTestInvisible; }).Font_Lambda([this] { return S47Style::Mono(P->Small() ? 15 : 18); }).ColorAndOpacity(Css(0xffb052)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 10, 0)
		[Button(TEXT("HOW"), [this] { P->ShowHow(GHowHidden); }, 18, 0xe9e2cf, 60)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
		[Button(TEXT("-"), [this] { P->ZoomAt(P->S * 0.8, P->ViewSize / 2); }, 22, 0xe9e2cf)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
		[Button(TEXT("ALL"), [this] { P->S = P->MinScale(); P->ClampView(); }, 22, 0xe9e2cf)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 14, 0)
		[Button(TEXT("+"), [this] { P->ZoomAt(P->S * 1.25, P->ViewSize / 2); }, 22, 0xe9e2cf)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[Button(TEXT("Close"), [this] { if (P->UI.IsValid()) P->UI->Close(); }, 22, 0xcfc8b5, 44, 40, false, true)];
	// Departs from the web version: how the board works, in three steps, until it is dismissed.
	auto Step = [this](const TCHAR* N, const TCHAR* Head, TFunction<FString()> Body) -> FW
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0, 0, 8, 0)
			[SNew(SBorder).BorderImage(Box(0xc4241c, 0, 0.9f)).Padding(FMargin(7, 0, 7, 0))[Text(N, 17, 0xfff3e2, true)]]
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
			[SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[Text(Head, 15, 0xffd6aa, true)]
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([Body] { return FText::FromString(Body()); }).Font(S47Style::Body(14.5f)).ColorAndOpacity(Css(0xd8cfbd)).AutoWrapText(true).LineHeightPercentage(1.2f)]];
	};
	P->HowStrip = SNew(SBorder).BorderImage(Box(0x2a1d14, 0xffbe8c, 0.95f, 0.28f)).Padding(FMargin(12, 8)).Visibility(GHowHidden ? EVisibility::Collapsed : EVisibility::Visible)
		[SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 16, 0)[Step(TEXT("1"), TEXT("TIE A THREAD"), [this] { return P->bCoarse
			? FString(TEXT("Tap a pin, then the record it goes with. A red thread ties them if they agree."))
			: FString(TEXT("Drag from a pin to the record it goes with, or click one and then the other. Each line that matters has its own pin.")); })]
		+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 16, 0)[Step(TEXT("2"), TEXT("FILL THE QUESTION"), [] { return FString(TEXT("A question card's numbered lines fill by themselves when the right threads hold. A wrong one falls slack.")); })]
		+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 16, 0)[Step(TEXT("3"), TEXT("RECORD"), [] { return FString(TEXT("When every line is filled, press RECORD on the card. Stuck: Hint, on the card.")); })]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Button(TEXT("GOT IT"), [this] { P->ShowHow(false); }, 17, 0xffd6aa, 80, 36)]];
	P->OfferEl = SNew(SBox).Visibility(EVisibility::Collapsed)
		[SNew(SButton).ButtonStyle(ButtonStyle()).ContentPadding(FMargin(12, 4))
		.OnClicked_Lambda([this] { if (!P->Offer.IsEmpty()) P->ShowHint(P->Offer); return FReply::Handled(); })
		[SAssignNew(P->OfferText, STextBlock).Font(S47Style::Mono(17)).ColorAndOpacity(Css(0xffd25a))]];
	P->HintBox = SNew(SBorder).BorderImage(Box(0xf1e6c8, 0xd9a441, 1, 1, 2)).Padding(FMargin(14, 8)).Visibility(EVisibility::Collapsed)
		[SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0, 0, 14, 0)[SAssignNew(P->HintHead, STextBlock).Font(S47Style::Mono(17)).ColorAndOpacity(Css(0x8f2a1d))]
		+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SAssignNew(P->HintText, STextBlock).Font(S47Style::Body(17)).ColorAndOpacity(Css(0x1e2430)).AutoWrapText(true).LineHeightPercentage(1.25f)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(10, 0, 0, 0)
		[Button(TEXT("x"), [this] { P->HintBox->SetVisibility(EVisibility::Collapsed); }, 17, 0x6b6458, 32, 28, false, true)]];
	ChildSlot
	// The web board uses viewport pixels, not Unreal's 1080p-scaled game UI coordinates.
	[SNew(SDPIScaler).DPIScale_Lambda([this] { return float(1 / FMath::Max(0.01, P->ParentScale)); })
	[SNew(SBorder).BorderImage(Box(0x020306, 0, 0.72f)).Padding(8)
		[SNew(SBorder).BorderImage(Box(0x0c0a09, 0xffbe8c, 0.97f, 0.25f)).Padding(FMargin(10, 8))
			[SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)[Head]
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Visibility_Lambda([this] { return P->ScreenSize().X <= 520 ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.Text_Lambda([this] { return P->StatusEl->GetText(); }).Font(S47Style::Mono(15)).ColorAndOpacity(Css(0xffb052)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
			[SAssignNew(P->HowEl, STextBlock).Font(S47Style::Body(14)).ColorAndOpacity(Css(0xb8b0a0)).AutoWrapText(true)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)[P->HowStrip.ToSharedRef()]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
			[SNew(STextBlock).Font(S47Style::Mono(17)).ColorAndOpacity(Css(0xffd6aa)).AutoWrapText(true)
			.Visibility_Lambda([] { return S47Gamepad::UsingGamepad() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.Text(FText::FromString(TEXT("RIGHT STICK: POINTER   A: CLICK / HOLD TO DRAG   D-PAD: PAN   LB/RB: ZOOM   X: ALL   Y: HOW   B: CLOSE")))]
			+ SVerticalBox::Slot().FillHeight(1).Padding(0, 0, 0, 6)[SAssignNew(P->Surface, SS47BoardSurface).Board(P.Get())]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)[P->HintBox.ToSharedRef()]
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
			[SNew(SBox).MinDesiredHeight(43.4f)[SAssignNew(P->SayEl, STextBlock).Font(S47Style::Body(15.5f)).LineHeightPercentage(1.4f).ColorAndOpacity(Css(0xddd5c2)).AutoWrapText(true)]]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12, 0, 0, 0)[P->OfferEl.ToSharedRef()]]]]]];
	Render();
}

SS47Board::~SS47Board() = default;
void SS47Board::Tick(const FGeometry& G, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(G, CurrentTime, DeltaTime);
	P->ParentScale = G.GetAccumulatedLayoutTransform().GetScale();
}
void SS47Board::Render() { P->Render(); }
FS47Result SS47Board::Link(const FString& A, const FString& B) { return P->Link(A, B); }
void SS47Board::Say(const FString& Text) { P->Say(Text); }
void SS47Board::Say(const FS47Result& R) { P->Say(R.Text, R.bOk ? 1 : -1); }
void SS47Board::Look(const FString& Id)
{
	if (P->ViewSize.X <= 0 || P->ViewSize.Y <= 0) { P->PendingLook = Id; return; }
	const TOptional<FS47BoardRegion> R = P->RegionOf(Id); if (R) P->Look(*R);
}
bool SS47Board::Frame(const TArray<FString>& Ids) { return P->Frame(Ids); }
bool SS47Board::Moved() const { return P->bChanged; }
bool SS47Board::Show(const FString& Id) { return P->Show(Id); }
FVector SS47Board::View() const { return FVector(P->S, P->Tx, P->Ty); }
void SS47Board::OfferHint(const FString& Q) { P->OfferHint(Q.IsEmpty() ? P->OpenQuestionIn(P->O.Focus) : Q); }
void SS47Board::ShowHint(const FString& Q) { P->ShowHint(Q.IsEmpty() ? P->OpenQuestionIn(P->O.Focus) : Q); }
void SS47Board::HoldThread(const FString& From, const FString& To) { P->TestFrom = From; P->TestTo = To; }
void SS47Board::HideHow() { P->ShowHow(false); }
void SS47Board::ShowAll() { P->S = P->MinScale(); P->ClampView(); }

FReply SS47Board::OnKeyDown(const FGeometry&, const FKeyEvent& E)
{
	if (E.GetKey() == EKeys::B || E.GetKey() == EKeys::Escape || E.GetKey() == EKeys::Tab || E.GetKey() == EKeys::Gamepad_FaceButton_Right)
	{
		if (P->UI.IsValid()) P->UI->Close();
		return FReply::Handled();
	}
	if (E.IsRepeat() && E.GetKey() == EKeys::Gamepad_FaceButton_Top) return FReply::Handled();
	if (P->PadView(E.GetKey())) return FReply::Handled();
	return FReply::Unhandled();
}

bool SS47Board::SelfTest(TArray<FString>& Lines)
{
	// ui/Board.ts API scenarios with no exported world. These are not native input or visual tests.
	bool Ok = true;
	auto Check = [&](bool Pass, const TCHAR* Name) { Lines.Add(FString(Pass ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("Board API: ") + Name); Ok &= Pass; };
	TMap<FString, FVector2D> Layout;
	int32 ConnectCalls = 0;
	FS47BoardImpl P;
	P.Canvas = SNew(SCanvas);
	P.StatusEl = SNew(STextBlock); P.HowEl = SNew(STextBlock); P.SayEl = SNew(STextBlock);
	P.O.Size = FVector2D(2400, 1800);
	P.O.Status = [] { return FString(TEXT("TWO PIECES")); };
	P.O.Regions = [] { FS47BoardRegion R; R.Id = TEXT("test"); R.X = 80; R.Y = 100; R.W = 1100; R.H = 850; return TArray<FS47BoardRegion>{ R }; };
	P.O.Cards = [] {
		FS47BoardCard A; A.Id = TEXT("a"); A.Stamp = TEXT("A"); A.Title = TEXT("First record"); A.bHave = true; A.Region = TEXT("test"); A.X = 30; A.Y = 45;
		FS47BoardCard B = A; B.Id = TEXT("b"); B.Stamp = TEXT("B"); B.X = 370; B.Lines = { { TEXT("b-line"), TEXT("The line that matters"), FString() } };
		FS47BoardCard C = A; C.Id = TEXT("free"); C.X.Reset(); C.Y.Reset();
		return TArray<FS47BoardCard>{ A, B, C };
	};
	P.O.LayoutGet = [&](const FString& Id) -> TOptional<FVector2D> { const FVector2D* At = Layout.Find(Id); return At ? TOptional<FVector2D>(*At) : TOptional<FVector2D>(); };
	P.O.LayoutSet = [&](const FString& Id, double X, double Y) { Layout.Add(Id, FVector2D(X, Y)); };
	P.O.Connect = [&](const FString& A, const FString& B) { ++ConnectCalls; return FS47Result{ A == TEXT("a") && B == TEXT("b-line"), TEXT("A test reply.") }; };
	P.Render();
	Check(P.StatusEl->GetText().ToString() == TEXT("TWO PIECES"), TEXT("render obtains the supplied status"));
	Check(P.Centre(TEXT("a")) == TOptional<FVector2D>(FVector2D(230, 147)) && P.Centre(TEXT("b-line")).IsSet(), TEXT("region coordinates and individual line pins are addressable"));
	Check(Layout.Contains(TEXT("free")) && !Overlap(Layout[TEXT("free")], P.Boxes[2]->Size, P.Boxes[0]->At, P.Boxes[0]->Size, 14)
		&& !Overlap(Layout[TEXT("free")], P.Boxes[2]->Size, P.Boxes[1]->At, P.Boxes[1]->Size, 14), TEXT("unplaced evidence gets a persistent unoccupied place"));
	const FVector2D Free = Layout[TEXT("free")]; P.Render();
	Check(Layout[TEXT("free")] == Free, TEXT("redraw preserves the free placement"));
	P.ViewSize = FVector2D(1100, 700); P.bInitialFocus = false;
	P.TapNode(TEXT("a")); P.TapNode(TEXT("b-line"));
	Check(ConnectCalls == 1 && P.Selected.IsEmpty() && P.SayEl->GetText().ToString() == TEXT("A test reply."), TEXT("two pin taps dispatch one connection and show the reply"));
	P.Link(TEXT("a"), TEXT("free"));
	Check(P.Slack.Num() == 1 && P.Slack[0].Q.C.Y > (P.Slack[0].Q.A.Y + P.Slack[0].Q.B.Y) / 2, TEXT("a rejected connection leaves a slack curve"));
	P.Slack[0].Born -= 2.1; P.Tick(P.ViewSize);
	Check(P.Slack.IsEmpty(), TEXT("slack thread expires after 2 seconds"));
	P.S = 1; P.Tx = P.Ty = 0;
	const TSharedPtr<FS47BoardImpl::FBox> Card = P.Boxes[0];
	const FVector2D Start = Card->At + FVector2D(30, 25);
	P.Down(0, Start, false); P.Move(0, Start + FVector2D(5, 0));
	Check(!P.Drag.bMoved, TEXT("movement below six pixels remains a tap"));
	P.Move(0, Start + FVector2D(35, 20)); P.End(0, Start + FVector2D(35, 20), false);
	Check(P.bChanged && Layout.Contains(Card->Id) && Layout[Card->Id] == Card->At, TEXT("moving a card saves its new layout on release"));
	P.ZoomAt(99, FVector2D(550, 350)); Check(FMath::IsNearlyEqual(P.S, 1.6), TEXT("zoom is limited to 1.6"));
	P.ZoomAt(0.001, FVector2D(550, 350)); Check(FMath::IsNearlyEqual(P.S, P.MinScale()), TEXT("zoom out keeps the whole board limit"));
	P.S = 1; P.Tx = P.Ty = 0; P.Down(1, FVector2D(1500, 1200), true); P.Down(2, FVector2D(1600, 1200), true);
	P.Move(2, FVector2D(1625, 1200));
	Check(P.Pinch.IsSet() && FMath::IsNearlyEqual(P.S, 1.25), TEXT("two pointer distances control pinch scale"));
	P.End(1, FVector2D(1500, 1200), false); P.End(2, FVector2D(1625, 1200), false);
	Check(!P.Pinch.IsSet() && P.Ptrs.IsEmpty(), TEXT("ending both pointers releases the pinch"));
	Check(P.Frame({ TEXT("a"), TEXT("b-line") }) && !P.Frame({ TEXT("missing") }) && P.Show(TEXT("a")), TEXT("framing and showing accept only evidence on the board"));
	P.S = 1; P.Ty = 0;
	const TOptional<FVector2D> A = P.Centre(TEXT("a"));
	if (A) P.Tx = 550 - A->X;
	for (const TSharedPtr<FS47BoardImpl::FBox>& B : P.Boxes) if (B->Id == TEXT("free") && A) B->At = *A + FVector2D(1200, 0);
	const TOptional<FVector2D> FreePin = P.Centre(TEXT("free"));
	if (A && FreePin)
	{
		const FVector2D From = *A + FVector2D(P.Tx, P.Ty), Outside = *FreePin + FVector2D(P.Tx, P.Ty);
		const int32 Before = ConnectCalls;
		P.Down(4, From, false); P.Move(4, Outside); P.End(4, Outside, false);
		Check(ConnectCalls == Before, TEXT("a thread dropped outside the viewport does not connect hidden evidence"));
	}
	P.O.Cards = [] { return TArray<FS47BoardCard>(); };
	P.O.Questions = [] { FS47BoardQuestion Q; Q.Id = TEXT("question"); Q.Stamp = TEXT("WHY"); Q.Text = TEXT("What do these show?"); Q.Slots = 2; Q.Region = TEXT("test"); return TArray<FS47BoardQuestion>{ Q }; };
	P.Render(); const double WithoutArt = P.Boxes[0]->Size.Y;
	TSharedPtr<FSlateBrush> IndexCard = MakeShared<FSlateBrush>(); IndexCard->ImageSize = FVector2D(768, 512);
	P.ArtBrushes.Add(TEXT("card_index_blank"), IndexCard); P.Render();
	Check(FMath::IsNearlyEqual(P.Boxes[0]->Size.Y, WithoutArt) && P.Boxes[0]->Size.Y < 512, TEXT("index-card background art does not enlarge question layout"));
	// the yarn, the help and the hints (departures from the web version)
	Check(FMath::IsNearlyEqual(YarnWidth(3.4, 1.2), 3.4 * 1.2) && YarnWidth(3.4, 0.3) >= 2.5 && YarnWidth(3.4, 9) <= 12, TEXT("the yarn's width follows the zoom, within a readable range"));
	Check(P.OpenQuestionIn(TEXT("test")) == TEXT("question") && P.OpenQuestionIn(TEXT("elsewhere")) == TEXT("question"), TEXT("the hint goes to the open question of the patch"));
	FString Hinted;
	P.O.Hint = [&](const FString& Q) { Hinted = Q; return FString(TEXT("Look at the dates.")); };
	const int32 MissesBefore = P.Misses;
	P.Link(TEXT("x"), TEXT("y"));
	Check(P.Offer == TEXT("question") && P.Misses == MissesBefore + 1, TEXT("a thread that does not hold offers a hint"));
	P.ShowHint(P.Offer);
	Check(Hinted == TEXT("question") && P.Offer.IsEmpty() && P.SayEl->GetText().ToString() == TEXT("Look at the dates."), TEXT("the offered hint is the question's own"));
	P.O.Questions = [] { FS47BoardQuestion Q; Q.Id = TEXT("question"); Q.Stamp = TEXT("WHY"); Q.Text = TEXT("What do these show?"); Q.Slots = 2; Q.Filled = { TEXT("one"), TEXT("two") }; Q.Region = TEXT("test"); return TArray<FS47BoardQuestion>{ Q }; };
	Check(P.OpenQuestionIn(TEXT("test")).IsEmpty(), TEXT("no hint is offered for a question whose lines are filled"));
	P.S = 1; P.Tx = P.Ty = 0;
	Check(P.PadView(EKeys::Gamepad_DPad_Right) && P.Tx == -80, TEXT("Xbox D-pad pans the board view without moving evidence"));
	const double BeforeZoom = P.S;
	Check(P.PadView(EKeys::Gamepad_RightShoulder) && P.S > BeforeZoom, TEXT("Xbox RB zooms the same board view"));
	Check(P.PadView(EKeys::Gamepad_FaceButton_Left) && FMath::IsNearlyEqual(P.S, P.MinScale()), TEXT("Xbox X fits all evidence"));
	Check(!P.PadView(EKeys::Gamepad_FaceButton_Bottom), TEXT("Xbox A stays on the pointer or focused-button route"));
	return Ok;
}

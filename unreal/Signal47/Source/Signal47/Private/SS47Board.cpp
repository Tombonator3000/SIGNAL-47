#include "SS47Board.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Texture2D.h"
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
	FVector2D ToBoardXY(const FVector2D& P) const { return (P - FVector2D(Tx, Ty)) / S; }
	double MinScale() const { return FMath::Min(0.5, FMath::Max(0.12, FMath::Min(ViewSize.X / O.Size.X, ViewSize.Y / O.Size.Y) * 0.95)); }
	void ClampView();
	void ZoomAt(double Ns, const FVector2D& P);
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
		if (P->Drag.bMoved && P->Drag.Kind != FS47BoardImpl::FDrag::EKind::Thread) return FCursorReply::Cursor(EMouseCursor::GrabHandClosed);
		const FS47BoardImpl::FHit H = P->Hit(P->ToBoardXY(G.AbsoluteToLocal(E.GetScreenSpacePosition())));
		return FCursorReply::Cursor(H.Pull.IsEmpty() ? EMouseCursor::GrabHand : EMouseCursor::Crosshairs);
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
			for (int32 I = 0; I < Q.Slots; ++I)
			{
				const bool On = Q.Filled.IsValidIndex(I) && !Q.Filled[I].IsEmpty();
				Body->AddSlot().AutoHeight().Padding(10, I == 0 ? 4 : 0, 10, 4)
				[SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)[SNew(SBox).WidthOverride(20)[Text(FString::Printf(TEXT("%d."), I + 1), 12.5f, 0x555555)]]
				+ SHorizontalBox::Slot().FillWidth(1)[SNew(SBox).MinDesiredHeight(18)
				[SNew(SBorder).BorderImage(Box(0xf1dc7c, 0x1e2430, On ? 0.55f : 0, 0.25f)).Padding(On ? FMargin(3, 1) : FMargin(0))
				[Text(On ? Q.Filled[I] : TEXT(" "), 12.5f, On ? 0x1e2430 : 0x555555, false, Width - 46)]]]];
			}
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
		Row->AddSlot().AutoWidth()[Button(TEXT("Hint"), [this, Id = Q.Id] { Say(O.Hint ? O.Hint(Id) : FString()); }, 16, 0x233a7a, 44, 36)];
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
	for (const FS47BoardQuestion& Q : O.Questions ? O.Questions() : TArray<FS47BoardQuestion>())
	{
		TSharedPtr<FBox> B = MakeShared<FBox>(); B->Id = Q.Id;
		B->Widget = QuestionHtml(Q, B);
		AddBox(B, Q.Region, Q.X, Q.Y, Q.W.Get(260));
	}
	for (const FS47BoardCard& C : O.Cards ? O.Cards() : TArray<FS47BoardCard>())
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
	if (R.bOk) Render();
	else
	{
		const TOptional<FVector2D> Pa = Centre(A), Pb = Centre(B);
		if (Pa && Pb) Slack.Add({ Curve(*Pa, *Pb, 0.4, 140), FPlatformTime::Seconds() });
	}
	return R;
}

void FS47BoardImpl::TapNode(const FString& N)
{
	if (!Selected.IsEmpty() && Selected != N) { Link(Selected, N); return; }
	Selected = Selected == N ? FString() : N;
	if (!Selected.IsEmpty()) Say(TEXT("Now the piece of evidence it goes with."));
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
	HowEl->SetVisibility(bSmall ? EVisibility::Collapsed : EVisibility::HitTestInvisible);
	SayEl->SetFont(S47Style::Body(bSmall ? 14 : 15.5f));
	Slack.RemoveAll([](const FSlack& T) { return FPlatformTime::Seconds() - T.Born >= 1.4; });
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
	const TSharedPtr<FSlateBrush> Yarn = ArtBrushes.FindRef(TEXT("thread_red"));
	auto DrawCurve = [&](const FCurve& Q, const FLinearColor& Colour, double Width, double Dash, double Gap, bool bYarn) {
		TArray<FVector2D> Points;
		FVector2D Previous = Q.At(0);
		double Along = 0;
		for (int32 I = 0; I <= 96; ++I)
		{
			const FVector2D P = Q.At(double(I) / 96);
			if (I) Along += (P - Previous).Size();
			if (!Dash || FMath::Fmod(Along, Dash + Gap) < Dash) Points.Add(P);
			else { if (Points.Num() > 1) Line(Points, Colour, Width); Points.Empty(); }
			Previous = P;
		}
		if (Points.Num() > 1) Line(Points, Colour, Width);
		if (bYarn && Yarn)
		{
			++Layer;
			FVector2D P = Q.At(0);
			const double Height = Width < 2 ? 3 : 6;
			for (int32 I = 1; I <= 12; ++I)
			{
				const FVector2D N = Q.At(double(I) / 12), Delta = N - P;
				Rotated(P + FVector2D(0, -Height / 2), FVector2D(Delta.Size() + 1, Height), Yarn.Get(), FMath::Atan2(Delta.Y, Delta.X), FLinearColor::White, true);
				P = N;
			}
		}
	};
	auto Pin = [&](const FVector2D& At, const FString& Colour, bool bTack = false) {
		const TSharedPtr<FSlateBrush> B = ArtBrushes.FindRef(TEXT("pin_") + Colour);
		if (B && !bTack) PaintBox(At - FVector2D(11), FVector2D(22), B.Get());
		else
		{
			const double Radius = bTack ? 4.5 : 7;
			const FLinearColor C = Css(bTack ? 0xe8c25a : Colour == TEXT("blue") ? 0x2f55b8 : Colour == TEXT("yellow") ? 0xd9b53a : 0xc4241c);
			// The CSS fallback is a shaded round tack; concentric circles keep its highlight.
			Ring(At + FVector2D(1, 2), Radius - 1, Css(0, 0.4f), 3);
			Ring(At, Radius / 2, C, Radius);
			if (!bTack) Ring(At + FVector2D(-2, -2), 1.5, Css(Colour == TEXT("blue") ? 0xa8c4ff : Colour == TEXT("yellow") ? 0xfff1a8 : 0xff8a78), 2);
		}
	};
	// Pin art and tape occupy the same positions as their CSS absolute elements.
	for (const TSharedPtr<FBox>& B : Boxes)
	{
		RefreshPins(B);
		if (B->bQuestion && !ArtBrushes.FindRef(TEXT("card_index_blank")))
			PaintBox(B->At, FVector2D(B->Size.X, 4), Box(B->bDone ? 0x2f6a3c : 0x8f2a1d));
		for (const FPin& P : B->Pins) Pin(Centre(P.Id).Get(B->At + P.Offset), B->bQuestion ? TEXT("blue") : TEXT("red"));
		if (B->bNote && !B->bHand) Pin(B->At + FVector2D(B->Size.X / 2, 2), TEXT("yellow"));
		if (B->bFinding) PaintBox(B->At, FVector2D(B->Size.X, 4), Box(0x2f6a3c));
		if (B->bPhoto)
		{
			const TSharedPtr<FSlateBrush> Tape = ArtBrushes.FindRef(TEXT("tape_strip"));
			Rotated(B->At + FVector2D(B->Size.X / 2 - 45, Tape ? -14 : -9), FVector2D(90, Tape ? 34 : 22), Tape ? Tape.Get() : Box(0xece4c4, 0, 0.72f, 0), FMath::DegreesToRadians(-3.0));
		}
	}
	++Layer;
	for (const FS47BoardThread& T : O.Threads ? O.Threads() : TArray<FS47BoardThread>())
	{
		const TOptional<FVector2D> A = Centre(T.A), B = Centre(T.B);
		if (!A || !B) continue;
		const FCurve Q = Curve(*A, *B);
		FCurve Shadow = Q; Shadow.A += FVector2D(1, 2); Shadow.C += FVector2D(1, 2); Shadow.B += FVector2D(1, 2);
		DrawCurve(Shadow, Css(0, 0.45f), T.bThin ? 1.2 : 2.6, T.bAuto ? 2 : 0, T.bAuto ? 3 : 0, false);
		DrawCurve(Q, Css(0xc4241c, T.bThin ? 0.7f : 1.f), T.bThin ? 1.2 : 2.6, T.bAuto ? 2 : 0, T.bAuto ? 3 : 0, true);
		Pin(*A, TEXT("yellow"), true); Pin(*B, TEXT("yellow"), true);
	}
	for (const FSlack& T : Slack)
	{
		const double Age = FMath::Clamp((FPlatformTime::Seconds() - T.Born) / 1.4, 0.0, 1.0);
		FCurve Q = T.Q; Q.A.Y += Age * 14; Q.C.Y += Age * 14; Q.B.Y += Age * 14;
		DrawCurve(Q, Css(0xa69c8c, 1 - Age), 2.6, 3, 4, false);
	}
	if (Drag.Kind == FDrag::EKind::Thread && Drag.bMoved)
	{
		const TOptional<FVector2D> A = Centre(Drag.From);
		if (A) DrawCurve(Curve(*A, ToBoardXY(Drag.Now)), Css(0xc4241c, 0.9f), 2.6, 6, 5, false);
	}
	return Layer + 1;
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
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
		[Button(TEXT("-"), [this] { P->ZoomAt(P->S * 0.8, P->ViewSize / 2); }, 22, 0xe9e2cf)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
		[Button(TEXT("ALL"), [this] { P->S = P->MinScale(); P->ClampView(); }, 22, 0xe9e2cf)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 14, 0)
		[Button(TEXT("+"), [this] { P->ZoomAt(P->S * 1.25, P->ViewSize / 2); }, 22, 0xe9e2cf)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[Button(TEXT("Close"), [this] { if (P->UI.IsValid()) P->UI->Close(); }, 22, 0xcfc8b5, 44, 40, false, true)];
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
			+ SVerticalBox::Slot().FillHeight(1).Padding(0, 0, 0, 6)[SAssignNew(P->Surface, SS47BoardSurface).Board(P.Get())]
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(SBox).MinDesiredHeight(43.4f)[SAssignNew(P->SayEl, STextBlock).Font(S47Style::Body(15.5f)).LineHeightPercentage(1.4f).ColorAndOpacity(Css(0xddd5c2)).AutoWrapText(true)]]]]]];
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

FReply SS47Board::OnKeyDown(const FGeometry&, const FKeyEvent& E)
{
	if (E.GetKey() == EKeys::B || E.GetKey() == EKeys::Escape || E.GetKey() == EKeys::Tab)
	{
		if (P->UI.IsValid()) P->UI->Close();
		return FReply::Handled();
	}
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
	P.Slack[0].Born -= 1.5; P.Tick(P.ViewSize);
	Check(P.Slack.IsEmpty(), TEXT("slack thread expires after 1.4 seconds"));
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
	return Ok;
}

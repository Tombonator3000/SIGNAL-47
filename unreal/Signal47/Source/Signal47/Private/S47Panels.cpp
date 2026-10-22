// ui/Panels.ts (chapter one's panels)
#include "S47Panels.h"

#include "Brushes/SlateColorBrush.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"
#include "S47FieldCamera.h"
#include "S47Image.h"
#include "S47Kit.h"
#include "S47UI.h"
#include "SS47Widgets.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// sizes: the web version's CSS pixels times this, as S47Kit's
	constexpr float PX = 1.25f;
	using FW = TSharedRef<SWidget>;
	constexpr double PrintAspect = double(US47FieldCamera::PRINT_H) / US47FieldCamera::PRINT_W;

	// say(el, '[data-say]', r): the reply line, good or bad for a result, plain for a hint
	struct FSay
	{
		FString Text;
		int32 Tone = 0;
		void Set(const FS47Result& R) { Text = R.Text; Tone = R.bOk ? 1 : -1; }
		void Set(const FString& S) { Text = S; Tone = 0; }
	};
	FW SayLine(const TSharedRef<FSay>& S)
	{
		return S47Kit::Text(TAttribute<FString>::CreateLambda([S] { return S->Text; }), TAttribute<int32>::CreateLambda([S] { return S->Tone; }));
	}

	// a print's photograph u, v from a point on the paper (fractions of PRINT); false off the photograph
	bool PhotoUv(double Px, double Py, double& U, double& V)
	{
		U = (Px * US47FieldCamera::PRINT_W - US47FieldCamera::PRINT_X) / US47FieldCamera::PHOTO_W;
		V = (Py * US47FieldCamera::PRINT_H - US47FieldCamera::PRINT_Y) / US47FieldCamera::PHOTO_H;
		return !(U < 0 || U > 1 || V < 0 || V > 1);
	}
}

// The loupe's state: the zoom, the centre of the window on the print, the mark (photo u, v).
struct FS47PrintState
{
	double Z = 1, Cx = 0.5, Cy = 0.5;
	TOptional<FVector2D> Mark;
	void Apply()
	{
		// keep the visible window inside the print
		const double Half = 0.5 / Z;
		Cx = FMath::Min(1 - Half, FMath::Max(Half, Cx));
		Cy = FMath::Min(1 - Half, FMath::Max(Half, Cy));
	}
};

// A print as .loupe / .pair-img show it: the picture over black, zoomed about the window's centre
// (scale(z) translate(...)), the mark (.mark: 6% of the picture's width, an amber square), a
// drag that moves the window when zoomed, and a click (a press and release within 8 px) that
// reports where on the paper it fell.
class SS47Print : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Print) : _Texture(nullptr), _Width(600.f), _Drag(false) {}
		SLATE_ARGUMENT(UTexture2D*, Texture)
		SLATE_ARGUMENT(TSharedPtr<FS47PrintState>, State)
		SLATE_ARGUMENT(float, Width)
		SLATE_ARGUMENT(bool, Drag)
		SLATE_ARGUMENT(TFunction<void(double, double)>, OnClick)
	SLATE_END_ARGS()

	void Construct(const FArguments& A)
	{
		Brush = S47Kit::BrushFor(A._Texture);
		St = A._State.IsValid() ? A._State : MakeShared<FS47PrintState>();
		Width = A._Width;
		bDrag = A._Drag;
		OnClick = A._OnClick;
		White = MakeUnique<FSlateColorBrush>(FLinearColor::White);
		Aspect = Brush.IsValid() && Brush->ImageSize.X > 0 ? Brush->ImageSize.Y / Brush->ImageSize.X : PrintAspect;
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(Width, Width * Aspect); }

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
	{
		const float W = G.GetLocalSize().X, H = float(W * Aspect);
		const FGeometry Box = G.MakeChild(FVector2f(W, H), FSlateLayoutTransform());
		Out.PushClip(FSlateClippingZone(Box));
		FSlateDrawElement::MakeBox(Out, Layer, Box.ToPaintGeometry(), White.Get(), ESlateDrawEffect::None, FLinearColor::Black);
		const float Z = float(St->Z);
		const float Ox = W / 2 - float(St->Cx) * W * Z, Oy = H / 2 - float(St->Cy) * H * Z;
		if (Brush.IsValid()) FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(FVector2f(W * Z, H * Z), FSlateLayoutTransform(FVector2f(Ox, Oy))), Brush.Get(), ESlateDrawEffect::None, FLinearColor::White);
		if (St->Mark)
		{
			const float Fx = float((US47FieldCamera::PRINT_X + St->Mark->X * US47FieldCamera::PHOTO_W) / US47FieldCamera::PRINT_W);
			const float Fy = float((US47FieldCamera::PRINT_Y + St->Mark->Y * US47FieldCamera::PHOTO_H) / US47FieldCamera::PRINT_H);
			const FVector2f C(Ox + Fx * W * Z, Oy + Fy * H * Z);
			const float R = 0.03f * W * Z;
			auto Square = [&](float Rr, const FLinearColor& Col, float Th, int32 L)
			{
				FSlateDrawElement::MakeLines(Out, L, G.ToPaintGeometry(), TArray<FVector2f>{ C + FVector2f(-Rr, -Rr), C + FVector2f(Rr, -Rr), C + FVector2f(Rr, Rr), C + FVector2f(-Rr, Rr), C + FVector2f(-Rr, -Rr) }, ESlateDrawEffect::None, Col, true, Th);
			};
			Square(R + 1.5f * Z, FLinearColor(0, 0, 0, 0.6f), 1.f, Layer + 2);
			Square(R, S47Kit::Css(0xffd27a), 2.f * Z, Layer + 3);
		}
		Out.PopClip();
		return Layer + 4;
	}

	virtual FReply OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override
	{
		if (E.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
		bDown = true;
		DownAt = G.AbsoluteToLocal(E.GetScreenSpacePosition());
		DownCx = St->Cx; DownCy = St->Cy;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	virtual FReply OnMouseMove(const FGeometry& G, const FPointerEvent& E) override
	{
		if (!bDown || !bDrag || St->Z <= 1) return FReply::Unhandled();
		const FVector2D P = G.AbsoluteToLocal(E.GetScreenSpacePosition());
		const double W = G.GetLocalSize().X, H = W * Aspect;
		St->Cx = DownCx - (P.X - DownAt.X) / W / St->Z;
		St->Cy = DownCy - (P.Y - DownAt.Y) / H / St->Z;
		St->Apply();
		return FReply::Handled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry& G, const FPointerEvent& E) override
	{
		if (!bDown || E.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
		bDown = false;
		const FReply R = FReply::Handled().ReleaseMouseCapture();
		const FVector2D P = G.AbsoluteToLocal(E.GetScreenSpacePosition());
		if (FVector2D::Distance(P, DownAt) > 8 || !OnClick) return R;
		const double W = G.GetLocalSize().X, H = W * Aspect;
		if (P.X < 0 || P.Y < 0 || P.X > W || P.Y > H) return R;
		// the point on the paper under the pointer, through the zoom
		OnClick(St->Cx + (P.X / W - 0.5) / St->Z, St->Cy + (P.Y / H - 0.5) / St->Z);
		return R;
	}

	virtual FCursorReply OnCursorQuery(const FGeometry&, const FPointerEvent&) const override
	{
		return OnClick ? FCursorReply::Cursor(EMouseCursor::Crosshairs) : FCursorReply::Unhandled();
	}

private:
	TSharedPtr<FSlateBrush> Brush;
	TSharedPtr<FS47PrintState> St;
	TUniquePtr<FSlateBrush> White;
	float Width = 600.f;
	double Aspect = PrintAspect;
	bool bDrag = false, bDown = false;
	FVector2D DownAt = FVector2D::ZeroVector;
	double DownCx = 0.5, DownCy = 0.5;
	TFunction<void(double, double)> OnClick;
};

// ---------- wet bench ----------

void S47Panels::WetBench(US47UI* UI, const FString& Frame, int32 Step, const FString& Empty, TFunction<void()> OnAct)
{
	const TArray<TPair<FString, FString>> Steps = {
		{ TEXT("1 / LOAD LIGHT-TIGHT TANK"), TEXT("The exposed film stays protected from room light.") },
		{ TEXT("2 / TRANSFER PRINT TO FIXER"), TEXT("Lift the contact print into the adjacent tray.") },
		{ TEXT("3 / COLLECT STABLE PRINT"), TEXT("The image can now be handled, filed and examined.") },
	};
	static const TCHAR* Labels[4] = { TEXT("LOAD SEALED FILM INTO TANK"), TEXT(""), TEXT("TRANSFER CONTACT PRINT TO FIXER"), TEXT("COLLECT AND EXAMINE PRINT") };
	const int32 Active = Step == 0 ? 0 : Step == 1 ? 0 : Step == 2 ? 1 : 2;
	TWeakObjectPtr<US47UI> U = UI;
	FW Body = SNullWidget::NullWidget;
	if (!Empty.IsEmpty()) Body = S47Kit::Text(Empty);
	else
	{
		Body = S47Kit::Col({
			S47Kit::Sub(Frame),
			S47Kit::Steps(Steps, Active),
			Step == 1 ? S47Kit::Text(TEXT("The film is in the tank. Wait for the timer."))
				: S47Kit::Button(Labels[FMath::Clamp(Step, 0, 3)], [U, OnAct] { if (U.IsValid()) U->Close(); if (OnAct) OnAct(); }, true) }, 0.f);
	}
	if (UI) UI->Open(S47Kit::Shell(UI, TEXT("WET BENCH"), Body));
}

// ---------- contact print with a loupe ----------

void S47Panels::PrintView(US47UI* UI, const FPrintView& O)
{
	TSharedPtr<FS47PrintState> St = MakeShared<FS47PrintState>();
	TSharedRef<FSay> Say = MakeShared<FSay>();
	Say->Set(O.Intro);
	if (O.Mark) St->Mark = O.Mark;
	const TFunction<FS47Result(double, double)> OnMark = O.OnMark;
	auto Click = [St, Say, OnMark](double Px, double Py)
	{
		double U = 0, V = 0;
		if (!OnMark || !PhotoUv(Px, Py, U, V)) return;
		const FS47Result R = OnMark(U, V);
		Say->Set(R);
		if (R.bOk) St->Mark = FVector2D(U, V);
	};
	const FW Loupe = SNew(SS47Print).Texture(S47Image::FromDataUrl(O.Url)).State(St).Width(750.f).Drag(true).OnClick(O.OnMark ? TFunction<void(double, double)>(Click) : TFunction<void(double, double)>());
	TArray<FW> Side;
	Side.Add(SayLine(Say));
	// .lp-zoom: Loupe 1 to 4 by .75, X and Y 0 to 1 by .01 (the sliders show the window after it is kept inside)
	Side.Add(S47Kit::Col({
		S47Kit::Slider(TEXT("Loupe"), 1.f, 4.f, 0.75f, TAttribute<float>::CreateLambda([St] { return float(St->Z); }), [St](float V) { St->Z = V; St->Apply(); }),
		S47Kit::Slider(TEXT("X"), 0.f, 1.f, 0.01f, TAttribute<float>::CreateLambda([St] { return float(St->Cx); }), [St](float V) { St->Cx = V; St->Apply(); }),
		S47Kit::Slider(TEXT("Y"), 0.f, 1.f, 0.01f, TAttribute<float>::CreateLambda([St] { return float(St->Cy); }), [St](float V) { St->Cy = V; St->Apply(); }) }, 4.f));
	if (O.Hints.Num() > 0)
	{
		const TArray<FString> Hints = O.Hints;
		TSharedRef<int32> Hint = MakeShared<int32>(0);
		Side.Add(SNew(SBox).HAlign(HAlign_Left)[S47Kit::Button(TEXT("Hint"), [Say, Hints, Hint] { Say->Set(Hints[FMath::Min(*Hint, Hints.Num() - 1)]); ++*Hint; })]);
	}
	if (O.Extra.IsValid()) Side.Add(O.Extra.ToSharedRef());
	Side.Add(S47Kit::Small(O.OnMark ? TEXT("Click to mark a location. Marking never edits the source photograph.") : TEXT("The original photograph is filed.")));
	St->Apply();
	if (UI) UI->Open(S47Kit::Shell(UI, O.Title, S47Kit::Two(Loupe, S47Kit::Col(Side, 0.f), 1.6f, 1.f)), O.OnClose);
}

// ---------- reference file ----------

void S47Panels::ReferenceFile(US47UI* UI, const FReferenceFile& O)
{
	TSharedRef<FSay> Say = MakeShared<FSay>();
	Say->Set(!O.Hyp.IsEmpty() ? FString(TEXT("A working hypothesis is on file. The field test is at B-12.")) : O.bRefDone ? FString(TEXT("B-12 selected. Choose a cause you can test.")) : FString());
	TSharedRef<bool> bHyp = MakeShared<bool>(O.bRefDone);
	const TFunction<FS47Result(const FString&)> OnRef = O.OnRef, OnHyp = O.OnHyp;
	auto Ref = [Say, bHyp, OnRef](const FString& C) { if (!OnRef) return; const FS47Result R = OnRef(C); Say->Set(R); if (R.bOk) *bHyp = true; };
	auto Hyp = [Say, OnHyp](const FString& C) { if (OnHyp) Say->Set(OnHyp(C)); };
	const FW HypBox = SNew(SBox).Visibility_Lambda([bHyp] { return *bHyp ? EVisibility::Visible : EVisibility::Collapsed; })
	[
		S47Kit::Col({
			S47Kit::Question(TEXT("What could put an extra stripe on the film?")),
			S47Kit::Col({
				S47Kit::Button(TEXT("HYPOTHESIS: STRAY LIGHT"), [Hyp] { Hyp(TEXT("light")); }),
				S47Kit::Button(TEXT("HYPOTHESIS: ENCODER DRIFT"), [Hyp] { Hyp(TEXT("drift")); }),
				S47Kit::Button(TEXT("HYPOTHESIS: MOTOR COMMAND"), [Hyp] { Hyp(TEXT("command")); }) }) }, 0.f)
	];
	const FW Side = S47Kit::Col({
		S47Kit::Question(TEXT("Which reference is the marked detail?")),
		S47Kit::Row({ S47Kit::Button(TEXT("MATCH MARKED DETAIL: B-12"), [Ref] { Ref(TEXT("B-12")); }), S47Kit::Button(TEXT("MATCH MARKED DETAIL: R-07"), [Ref] { Ref(TEXT("R-07")); }) }),
		HypBox,
		SayLine(Say) }, 0.f);
	// .lp-paper.typed: a paragraph each
	if (UI) UI->Open(S47Kit::Shell(UI, TEXT("REFERENCE FILE"), S47Kit::Two(S47Kit::Paper(FString::Join(O.Sheet, TEXT("\n\n"))), Side)));
}

// ---------- B-12 local control ----------

void S47Panels::B12Control(US47UI* UI, const FString& Method, TFunction<void(const FString& M)> OnChoose, const FString& Review)
{
	TWeakObjectPtr<US47UI> U = UI;
	FW Body = SNullWidget::NullWidget;
	if (!Method.IsEmpty())
	{
		Body = S47Kit::Col({ S47Kit::Sub(TEXT("B-12 / LOCAL OPTICAL REFERENCE / MANUAL CONTROL ONLY")), S47Kit::Paper(Review) }, 0.f);
	}
	else
	{
		auto Choose = [U, OnChoose](const FString& M) { if (U.IsValid()) U->Close(); if (OnChoose) OnChoose(M); };
		Body = S47Kit::Col({
			S47Kit::Sub(TEXT("B-12 / LOCAL OPTICAL REFERENCE / MANUAL CONTROL ONLY")),
			S47Kit::Cards({
				S47Kit::Card(TEXT("PASSIVE / SHIELD THE LAMP"), TEXT("Fold the vane into its shield stop. The warm lamp is blocked and the reference motor remains isolated. A reflection should depend on its source."), [Choose] { Choose(TEXT("passive")); }),
				S47Kit::Card(TEXT("ACTIVE / DRIVE TO 042"), TEXT("Move the local reference to a known heading. Compare the visible vane with its encoder. The work lamp remains on."), [Choose] { Choose(TEXT("active")); }) }) }, 0.f);
	}
	if (UI) UI->Open(S47Kit::Shell(UI, TEXT("B-12 CONTROL"), Body));
}

// ---------- two exposures ----------

void S47Panels::TwoExposures(US47UI* UI, const FTwoExposures& O)
{
	TSharedRef<FSay> Say = MakeShared<FSay>();
	Say->Set(O.bConcluded ? FString(TEXT("Finding recorded. File both prints at the records desk.")) : O.OnBoard ? FString(TEXT("Mark the same reference in FRAME 02, then compare the prints on the case board.")) : FString(TEXT("Mark the reference detail in FRAME 02 before recording a conclusion.")));
	TSharedPtr<FS47PrintState> St = MakeShared<FS47PrintState>();
	if (O.Mark2) St->Mark = O.Mark2;
	const TFunction<FS47Result(double, double)> OnMark = O.OnMark;
	auto Click = [St, Say, OnMark](double Px, double Py)
	{
		double U = 0, V = 0;
		if (!OnMark || !PhotoUv(Px, Py, U, V)) return;
		const FS47Result Res = OnMark(U, V);
		Say->Set(Res);
		if (Res.bOk) St->Mark = FVector2D(U, V);
	};
	auto Figure = [](const FW& Img, const FString& Caption)
	{
		return S47Kit::Col({ Img, SNew(STextBlock).Text(FText::FromString(Caption)).Font(S47Style::Mono(16.f * PX)).ColorAndOpacity(S47Kit::Css(0xbdb5a3)).AutoWrapText(true) }, 4.f * PX);
	};
	const FW Pair = S47Kit::Two(
		Figure(SNew(SS47Print).Texture(S47Image::FromDataUrl(O.Url1)).Width(610.f), TEXT("FRAME 01 / S-03 APRON")),
		Figure(SNew(SS47Print).Texture(S47Image::FromDataUrl(O.Url2)).State(St).Width(610.f).OnClick(Click), TEXT("FRAME 02 / B-12 CONTROL. Mark the same reference.")));
	TArray<FW> Buttons;
	if (O.OnConclude)
	{
		const TFunction<FS47Result(const FString&)> OnConclude = O.OnConclude;
		static const TCHAR* Concl[3][2] = { { TEXT("lamp"), TEXT("THE LAMP CREATED THE MARK") }, { TEXT("commands"), TEXT("TWO ANTENNA COMMANDS") }, { TEXT("second"), TEXT("FILM RETAINS A SECOND REFERENCE") } };
		for (const auto& B : Concl)
		{
			const FString C = B[0];
			Buttons.Add(S47Kit::Button(B[1], [Say, OnConclude, C] { Say->Set(OnConclude(C)); }));
		}
	}
	if (O.OnBoard) { const TFunction<void()> OnBoard = O.OnBoard; Buttons.Add(S47Kit::Button(TEXT("CASE BOARD"), [OnBoard] { OnBoard(); }, true)); }
	{
		const TArray<FString> Hints = O.Hints;
		TSharedRef<int32> Hint = MakeShared<int32>(0);
		Buttons.Add(S47Kit::Button(TEXT("Hint"), [Say, Hints, Hint] { if (Hints.Num()) Say->Set(Hints[FMath::Min(*Hint, Hints.Num() - 1)]); ++*Hint; }));
	}
	if (UI) UI->Open(S47Kit::Shell(UI, TEXT("TWO EXPOSURES"), S47Kit::Col({ Pair, SayLine(Say), S47Kit::Row(Buttons) }, 0.f)));
}

// ---------- local report ----------

void S47Panels::LocalReport(US47UI* UI, const FString& Text, bool bFiled, TFunction<void()> OnFile)
{
	TWeakObjectPtr<US47UI> U = UI;
	const FW Body = S47Kit::Col({
		S47Kit::Paper(Text, true),
		bFiled ? S47Kit::Text(TEXT("Filed.")) : SNew(SBox).HAlign(HAlign_Left).Padding(FMargin(0, 14, 0, 0))[S47Kit::Button(TEXT("FILE PRINTS AND CLOSE THE LOCAL CASE"), [U, OnFile] { if (U.IsValid()) U->Close(); if (OnFile) OnFile(); }, true)] }, 0.f);
	if (UI) UI->Open(S47Kit::Shell(UI, TEXT("LOCAL REPORT"), Body));
}

// ui/Panels.ts (chapter one's panels)
#include "S47Panels.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformTime.h"
#include "Rendering/DrawElements.h"
#include "S47FieldCamera.h"
#include "S47Image.h"
#include "S47Kit.h"
#include "S47NpcDialogue.h"
#include "S47EarlDialogue.h"
#include "S47ReadAloud.h"
#include "S47UI.h"
#include "SS47Widgets.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
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

// ---------- chapter three: the null test at the STATION 01 field lamp ----------

// nullTest's page: drawn again (render) after the lamp is switched, after a finding, and when the
// chapter asks (refresh); the reply line starts empty each time, as the page's innerHTML does.
namespace
{
struct FS47NullTestPanel : public TSharedFromThis<FS47NullTestPanel>
{
	S47Panels::FNullTest O;
	TWeakPtr<STextBlock> StatusEl;
	TWeakPtr<SBox> PageEl;
	TSharedPtr<FSay> Say;
	int32 Hint = 0;

	void Render()
	{
		const S47Panels::FNullTestState St = O.State ? O.State() : S47Panels::FNullTestState();
		if (TSharedPtr<STextBlock> E = StatusEl.Pin()) E->SetText(FText::FromString(O.Status ? O.Status() : FString()));
		TSharedPtr<SBox> Page = PageEl.Pin();
		if (!Page.IsValid()) return;
		Say = MakeShared<FSay>();
		TWeakPtr<FS47NullTestPanel> W = AsShared();
		TArray<FW> Items = {
			S47Kit::Paper(TEXT("Screen the practical lamp and observe the ordinary local null condition. This isolates the source. It does not move the physical reference, and it does not finish the cable finding.")),
			S47Kit::Question(FString::Printf(TEXT("LAMP STATUS / %s"), St.bCovered ? TEXT("COVERED") : TEXT("EXPOSED"))),
			S47Kit::Row({
				S47Kit::Button(St.bCovered ? TEXT("UNCOVER LAMP") : TEXT("COVER LAMP"), [W] { if (TSharedPtr<FS47NullTestPanel> P = W.Pin()) P->Cover(); }),
				S47Kit::Button(St.bObserved ? TEXT("REVIEW NULL TEST") : TEXT("OBSERVE NULL TEST"), [W] { if (TSharedPtr<FS47NullTestPanel> P = W.Pin()) P->Observe(); }, true),
				S47Kit::Button(TEXT("Hint"), [W] { if (TSharedPtr<FS47NullTestPanel> P = W.Pin()) P->Hinted(); }) }),
		};
		if (St.bP07) { Items.Add(S47Kit::Paper(O.Supported)); Items.Add(S47Kit::Small(TEXT("P07 is pinned on the case board."))); }
		Items.Add(SayLine(Say.ToSharedRef()));
		Page->SetContent(S47Kit::Col(Items, 0.f));
	}
	void Cover()
	{
		const bool bCovered = O.State ? O.State().bCovered : false;
		if (O.OnLamp) O.OnLamp(!bCovered);
		Render();
	}
	void Observe()
	{
		if (!O.OnObserve) return;
		const FS47Result R = O.OnObserve();
		if (R.bOk) Render();
		if (Say.IsValid()) Say->Set(R);
	}
	void Hinted()
	{
		if (O.Hints.Num() && Say.IsValid()) Say->Set(O.Hints[FMath::Min(Hint, O.Hints.Num() - 1)]);
		++Hint;
	}
};
}

S47Panels::FPanelHandle S47Panels::NullTest(US47UI* UI, const FNullTest& O)
{
	TSharedRef<FS47NullTestPanel> P = MakeShared<FS47NullTestPanel>();
	P->O = O;
	// .lp-sub[data-status]
	TSharedRef<STextBlock> Status = SNew(STextBlock).Font(S47Style::Mono(19.f * PX)).ColorAndOpacity(S47Kit::Css(0xffb052)).AutoWrapText(true).Margin(FMargin(0, 0, 0, 10));
	TSharedRef<SBox> Page = SNew(SBox);
	P->StatusEl = Status;
	P->PageEl = Page;
	P->Render();
	// (the widgets keep the panel's state alive, and it holds them only weakly: closing the panel lets both go)
	const FW Body = SNew(SBox).Visibility_Lambda([P] { return EVisibility::Visible; })[S47Kit::Col({ Status, Page }, 0.f)];
	if (UI) UI->Open(S47Kit::Shell(UI, TEXT("STATION 01 / FIELD LAMP"), Body));
	TWeakPtr<FS47NullTestPanel> W = P;
	return { [W] { if (TSharedPtr<FS47NullTestPanel> Pp = W.Pin()) Pp->Render(); } };
}

// ---------- chapter three: the field prints ----------

void S47Panels::FieldPrints(US47UI* UI, const FFieldPrints& O)
{
	TArray<FW> Figs;
	const TFunction<void(int32, double, double)> OnLook = O.OnLook;
	for (int32 i = 0; i < O.Prints.Num(); ++i)
	{
		TSharedPtr<FS47PrintState> St = MakeShared<FS47PrintState>();
		auto Click = [St, OnLook, i](double Px, double Py)
		{
			// .lp-closer.close: a print looked at closer goes back with a click
			if (St->Z > 1) { St->Z = 1; St->Cx = 0.5; St->Cy = 0.5; return; }
			const double X = FMath::Clamp(Px, 0.0, 1.0), Y = FMath::Clamp(Py, 0.0, 1.0);
			// scale(3) about the point clicked (transform-origin there): the window shown is a third of
			// the print, with (1 + 4c) / 6 its middle
			St->Z = 3; St->Cx = (1 + 4 * X) / 6; St->Cy = (1 + 4 * Y) / 6;
			if (OnLook) OnLook(i, (X * US47FieldCamera::PRINT_W - US47FieldCamera::PRINT_X) / US47FieldCamera::PHOTO_W, (Y * US47FieldCamera::PRINT_H - US47FieldCamera::PRINT_Y) / US47FieldCamera::PHOTO_H);
		};
		const FW Img = SNew(SS47Print).Texture(S47Image::FromDataUrl(O.Prints[i].Url)).State(St).Width(610.f).OnClick(Click);
		Figs.Add(S47Kit::Col({ Img, SNew(STextBlock).Text(FText::FromString(O.Prints[i].Caption)).Font(S47Style::Mono(16.f * PX)).ColorAndOpacity(S47Kit::Css(0xbdb5a3)).AutoWrapText(true) }, 4.f * PX));
	}
	const FW Pair = Figs.Num() >= 2 ? S47Kit::Two(Figs[0], Figs[1]) : Figs.Num() == 1 ? Figs[0] : SNullWidget::NullWidget;
	// .lp-tip ('Tap' on a touch screen: there is no touch input in this version) and .lp-text
	const FW Body = S47Kit::Col({ Pair, S47Kit::Small(TEXT("Click a print to look closer.")), S47Kit::Text(O.Text) }, 0.f);
	if (UI) UI->Open(S47Kit::Shell(UI, TEXT("STATION 01 / FIELD PRINTS"), Body), O.OnClose);
}

// ---------- chapter four: a conversation in room 6 ----------

FString S47Panels::PortraitOf(const FString& Who)
{
	// core/portraits.ts: the names the conversations and calls give the speakers (BY_NAME), and the
	// picture by the name the brief gives (assets/art/people/portrait_<id>.jpg; Content/WebData/art)
	static const TMap<FString, FString> BY_NAME = {
		{ TEXT("N. VEGA"), TEXT("nora") }, { TEXT("NORA"), TEXT("nora") }, { TEXT("THE WAITRESS"), TEXT("waitress") }, { TEXT("THE DRIVER"), TEXT("driver") },
		{ TEXT("E. WARD"), TEXT("ward") }, { TEXT("WARD"), TEXT("ward") }, { TEXT("DALE"), TEXT("dale") },
		{ TEXT("THE ATTENDANT"), TEXT("attendant") }, { TEXT("THE DEPUTY"), TEXT("deputy") }, { TEXT("EARL"), TEXT("host") }, { TEXT("THE WOMAN"), TEXT("radio_woman") }, { TEXT("THE SOUVENIR MAN"), TEXT("souvenir_man") },
	};
	const FString* Id = BY_NAME.Find(Who.TrimStartAndEnd().ToUpper());
	return Id ? TEXT("portrait_") + *Id : FString();
}

namespace
{
	const FSlateBrush* TalkBox(const FLinearColor& Fill, const FLinearColor& Line)
	{
		static TMap<FString, TSharedPtr<FSlateBrush>> Cache;
		TSharedPtr<FSlateBrush>& B = Cache.FindOrAdd(Fill.ToString() + Line.ToString());
		if (!B.IsValid()) B = MakeShared<FSlateRoundedBoxBrush>(Fill, 0.f, Line, Line.A > 0 ? 1.f : 0.f);
		return B.Get();
	}

	// button { background: none; border: 0 }: the look is the SBorder inside
	const FButtonStyle* BareButton()
	{
		static FButtonStyle S = []
		{
			FButtonStyle B;
			FSlateBrush None;
			None.DrawAs = ESlateBrushDrawType::NoDrawType;
			B.SetNormal(None).SetHovered(None).SetPressed(None).SetDisabled(None);
			B.SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
			return B;
		}();
		return &S;
	}

	// a picture in a box of its own size, cropped to fill it (object-fit: cover)
	FW Cover(UTexture2D* T, float W, float H)
	{
		if (!T) return SNew(SBox).WidthOverride(W).HeightOverride(H);
		TSharedRef<FSlateBrush> B = MakeShared<FSlateBrush>();
		B->SetResourceObject(T);
		B->ImageSize = FVector2D(W, H);
		B->DrawAs = ESlateBrushDrawType::Image;
		const double Img = double(T->GetSizeX()) / FMath::Max(1, T->GetSizeY()), Box = W / H;
		if (Img > Box) { const double R = Box / Img; B->SetUVRegion(FBox2f(FVector2f(float(0.5 - R / 2), 0.f), FVector2f(float(0.5 + R / 2), 1.f))); }
		else if (Img < Box) { const double R = Img / Box; B->SetUVRegion(FBox2f(FVector2f(0.f, float(0.5 - R / 2)), FVector2f(1.f, float(0.5 + R / 2)))); }
		return SNew(SBox).WidthOverride(W).HeightOverride(H)[SNew(SImage).Image_Lambda([B] { return &B.Get(); })];
	}

	// CSS ease-out, cubic-bezier(0, 0, .58, 1), at x in [0, 1]
	double EaseOut(double X)
	{
		X = FMath::Clamp(X, 0.0, 1.0);
		double Lo = 0, Hi = 1, T = X;
		for (int32 I = 0; I < 24; ++I)
		{
			T = (Lo + Hi) / 2;
			const double Bx = 3 * (1 - T) * T * T * 0.58 + T * T * T;
			if (Bx < X) Lo = T; else Hi = T;
		}
		return 3 * (1 - T) * T * T + T * T * T;
	}

	// characterReadingControls's state: the Replay voice button and its status line
	struct FVoiceBox
	{
		bool bActive = false, bDisabled = false;
		ES47DialogueState State = ES47DialogueState::Idle;
		FString Parody;
	};
}

// The talk panel as one widget: render() draws the body from the chapter's view again (a click
// changes the state, then the panel is drawn anew), and lines that are new come in one after
// another (.talk-line.fresh, talk-in .45 s ease-out, .55 s apart).
class SS47Talk : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Talk) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&)
	{
		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SAssignNew(StatusText, STextBlock).Font(S47Style::Mono(19.f * PX)).ColorAndOpacity(S47Kit::Css(0xffb052)).AutoWrapText(true).Margin(FMargin(0, 0, 0, 10))]
			+ SVerticalBox::Slot().AutoHeight()[SAssignNew(Body, SBox)]
			+ SVerticalBox::Slot().AutoHeight()[SAssignNew(Reading, SBox)]
		];
	}

	void Setup(US47UI* InUI, const S47Panels::FTalk& InO) { UI = InUI; O = InO; }

	void Render();
	// a click: the state changes now, the panel is drawn again on the next frame (not inside the
	// button's own click)
	void Later() { bDirty = true; }

	virtual void Tick(const FGeometry& G, const double Now, const float Dt) override
	{
		SCompoundWidget::Tick(G, Now, Dt);
		if (bDirty) { bDirty = false; Render(); }
	}

	TWeakObjectPtr<US47UI> UI;
	S47Panels::FTalk O;
	void StopCharacter() const { if (!O.Character.IsEmpty()) if (const TSharedPtr<FS47NpcDialogue> N = Npc()) N->Stop(); }

private:
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SBox> Body, Reading;
	bool bDirty = false;
	FString Shown, ReadingShown, ShownAfter;
	TArray<S47Panels::FTalkLine> ShownLines;
	double FreshT0 = 0;

	TSharedPtr<FS47NpcDialogue> Npc() const { return UI.IsValid() ? UI->NpcDialogue : nullptr; }
	bool NpcOn() const { const TSharedPtr<FS47NpcDialogue> N = Npc(); return N.IsValid() && N->Enabled(); }
	static FString Key(const TArray<S47Panels::FTalkLine>& L)
	{
		// JSON.stringify(v.lines): the lines as they are, to tell a new exchange from a redraw
		FString K;
		for (const S47Panels::FTalkLine& X : L) K += X.Who + TEXT("\x1f") + X.Text + TEXT("\x1e");
		return K;
	}
	FW Line(const S47Panels::FTalkLine& L, bool bFresh, double Delay) const;
	TFunction<void()> CharacterReadingControls(const TSharedRef<SVerticalBox>& Parent, const FString& Voice, const TArray<FS47DialogueLine>& Lines);
};

FW SS47Talk::Line(const S47Panels::FTalkLine& L, bool bFresh, double Delay) const
{
	// <p class="talk-line[ aside][ fresh]"><span class="who">WHO</span>text</p>
	FSlateFontInfo Who = S47Style::Mono(19.f * PX);
	Who.LetterSpacing = 80;
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	if (!L.Who.IsEmpty())
	{
		// .who: inline-block, min-width 6.4em of its own font (VT323 is narrow: about half an em a letter), .6em after it
		Row->AddSlot().AutoWidth().Padding(0, 0, 0.6f * 19.f * PX, 0)
		[
			SNew(SBox).MinDesiredWidth(6.4f * 19.f * PX * 0.5f)[SNew(STextBlock).Text(FText::FromString(L.Who)).Font(Who).ColorAndOpacity(S47Kit::Css(0xffb7a4))]
		];
	}
	Row->AddSlot().FillWidth(1.f)
	[
		SNew(STextBlock).Text(FText::FromString(L.Text)).Font(S47Style::Body(16.5f * PX)).AutoWrapText(true).LineHeightPercentage(1.15f)
		.ColorAndOpacity(L.Who.IsEmpty() ? S47Kit::Css(0xa99f8c) : S47Kit::Css(0xf0e7d0))
	];
	if (!bFresh) return Row;
	// talk-in: from opacity 0 and 4 px down, after its delay ('both': hidden until then)
	const double T0 = FreshT0 + Delay;
	return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBorder")).Padding(0)
		.ColorAndOpacity_Lambda([T0] { const double K = EaseOut((FPlatformTime::Seconds() - T0) / 0.45); return FLinearColor(1, 1, 1, float(K)); })
		.RenderTransform_Lambda([T0] { const double K = EaseOut((FPlatformTime::Seconds() - T0) / 0.45); return FSlateRenderTransform(FVector2f(0.f, float(4 * PX * (1 - K)))); })
		[Row];
}

TFunction<void()> SS47Talk::CharacterReadingControls(const TSharedRef<SVerticalBox>& Parent, const FString& Voice, const TArray<FS47DialogueLine>& Lines)
{
	// ui.characterReadingControls: the Replay voice button, and its status; returns how to play the
	// lines now, or nothing when there is no recording of them
	const TSharedPtr<FS47NpcDialogue> Reader = Npc();
	if (!Reader.IsValid() || !Reader->Enabled()) return nullptr;
	const TArray<FS47DialogueLine> Speech = FS47NpcDialogue::CharacterLines(Voice, Lines);
	if (!Speech.Num()) return nullptr;
	TSharedRef<FVoiceBox> V = MakeShared<FVoiceBox>();
	V->Parody = Voice == TEXT("bacall") && Speech.ContainsByPredicate([](const FS47DialogueLine& L) { return L.Who == TEXT("THE LINE"); })
		? FString(TEXT("Synthetic Lauren Bacall / Vincent Price parodies")) : FString::Printf(TEXT("Synthetic %s parody"), *FS47NpcDialogue::Label(Voice));
	V->bDisabled = !Reader->Available(Voice, Lines);
	const TWeakPtr<FS47NpcDialogue> R = Reader;
	TFunction<void(ES47DialogueState)> Update = [V](ES47DialogueState S)
	{
		V->State = S;
		V->bActive = S == ES47DialogueState::Loading || S == ES47DialogueState::Playing;
	};
	TFunction<void()> Play = [R, Voice, Lines, Update]
	{
		const TSharedPtr<FS47NpcDialogue> P = R.Pin();
		if (!P.IsValid()) return;
		if (P->OnCharacterStart) P->OnCharacterStart();
		P->Play(Voice, Lines, Update);
	};
	Update(ES47DialogueState::Idle);
	FSlateFontInfo Term = S47Style::Mono(20.f * PX);
	Parent->AddSlot().AutoHeight().Padding(0, 10, 0, 10)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			// .lp-btn: Replay voice, or Stop voice while it plays
			SNew(SButton).ButtonStyle(BareButton()).IsEnabled_Lambda([V] { return !V->bDisabled; })
			.OnClicked_Lambda([V, R, Play] { if (V->bActive) { if (const TSharedPtr<FS47NpcDialogue> P = R.Pin()) P->Stop(); } else Play(); return FReply::Handled(); })
			[
				SNew(SBorder).BorderImage(TalkBox(S47Kit::Css(0xffc896, 0.05f), S47Kit::Css(0xffd6aa, 0.45f))).Padding(FMargin(14, 6))
				.ColorAndOpacity_Lambda([V] { return FLinearColor(1, 1, 1, V->bDisabled ? 0.5f : 1.f); })
				[
					SNew(SBox).MinDesiredHeight(44.f - 12.f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Font(Term).ColorAndOpacity(S47Kit::Css(0xf1e8d2))
						.Text_Lambda([V] { return FText::FromString(V->bActive ? TEXT("Stop voice") : TEXT("Replay voice")); })
					]
				]
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(14, 0, 0, 0)
		[
			SNew(STextBlock).Font(S47Style::Body(12.f * PX)).ColorAndOpacity(S47Kit::Css(0xb6ae9b)).AutoWrapText(true)
			.Text_Lambda([V]
			{
				return FText::FromString(V->State == ES47DialogueState::Loading ? FString(TEXT("Loading voice…")) : V->State == ES47DialogueState::Error ? FString(TEXT("Recording could not be played. Try again."))
					: V->bDisabled ? FString(TEXT("No recording for this reply.")) : V->Parody);
			})
		]
	];
	// a reply without a recording stays text, and says so (not a failed playback)
	return V->bDisabled ? TFunction<void()>() : Play;
}

void SS47Talk::Render()
{
	if (!UI.IsValid() || !O.View) return;
	const S47Panels::FTalkView V = O.View();
	StatusText->SetText(FText::FromString(V.Status));
	// new lines come one after another; a redraw of the same lines does not repeat that, and lines
	// added under the ones already there (a recording as it plays) come in alone
	const FString K = Key(V.Lines);
	const bool bFresh = K != Shown;
	int32 Kept = 0;
	if (bFresh && ShownLines.Num() && ShownLines.Num() < V.Lines.Num())
	{
		bool bSame = true;
		for (int32 I = 0; I < ShownLines.Num(); ++I) if (ShownLines[I].Who != V.Lines[I].Who || ShownLines[I].Text != V.Lines[I].Text) bSame = false;
		if (bSame) Kept = ShownLines.Num();
	}
	ShownLines = V.Lines;
	if (bFresh) FreshT0 = FPlatformTime::Seconds();
	FString Reply;
	const bool bReply = O.Character == TEXT("bacall") && NpcOn() && V.Reply && FS47NpcDialogue::NoraQuote(V.Reply->Text, Reply);
	TArray<FS47DialogueLine> Speech;
	if (bReply) Speech.Add({ TEXT("N. VEGA"), Reply });
	else for (const S47Panels::FTalkLine& L : V.Lines) Speech.Add({ L.Who, L.Text });
	const FString ReadingKey = K + TEXT("\x1d") + (bReply ? Reply : FString(TEXT("\x1c")));
	TFunction<void()> PlayCharacter;
	if (ReadingKey != ReadingShown)
	{
		if (UI.IsValid())
		{
			if (const TSharedPtr<FS47ReadAloud> D = UI->DescriptionReader()) D->Stop();
			if (UI->ReadAloud.IsValid()) UI->ReadAloud->Stop();
			if (O.bEarl && UI->EarlDialogue.IsValid()) UI->EarlDialogue->Stop();
		}
		StopCharacter();
		TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
		// the lines nobody voices get Read aloud (ui.readingControls): the character's own are its voice's
		// (Earl's voice in this panel, ui.earlReadingControls, is Caliche's own panel's in this version, so
		// o.earl takes no lines from Kinski here)
		TArray<FS47DialogueLine> All;
		for (const S47Panels::FTalkLine& L : V.Lines) All.Add({ L.Who, L.Text });
		const TArray<FS47DialogueLine> Voiced = !O.Character.IsEmpty() && NpcOn() ? FS47NpcDialogue::CharacterLines(O.Character, All) : TArray<FS47DialogueLine>();
		TArray<FString> Texts;
		for (const FS47DialogueLine& L : All) if (!Voiced.Contains(L)) Texts.Add(L.Text);
		if (Texts.Num() && UI.IsValid()) Box->AddSlot().AutoHeight()[UI->ReadingControls(Texts)];
		if (!O.Character.IsEmpty() && NpcOn())
		{
			const TFunction<void()> Play = CharacterReadingControls(Box, O.Character, Speech);
			if (bFresh || bReply) PlayCharacter = Play;
		}
		Reading->SetContent(Box);
	}
	ReadingShown = ReadingKey;
	Shown = K;
	// the face of whoever spoke last (core/portraits.ts), once there is a picture of them
	FString Face;
	for (int32 I = V.Lines.Num() - 1; I >= 0; --I)
	{
		if (!V.Lines[I].Who.IsEmpty() && V.Lines[I].Who != TEXT("YOU")) { Face = S47Panels::PortraitOf(V.Lines[I].Who); break; }
	}
	UTexture2D* FaceTex = Face.IsEmpty() ? nullptr : S47Image::Art(Face);
	const bool bAfterNew = V.After.IsSet() && V.After->Text != ShownAfter;
	ShownAfter = V.After ? V.After->Text : FString();

	TSharedRef<SVerticalBox> LinesBox = SNew(SVerticalBox);
	for (int32 I = 0; I < V.Lines.Num(); ++I)
	{
		const bool bF = bFresh && I >= Kept;
		LinesBox->AddSlot().AutoHeight().Padding(0, I ? 8.f : 0.f, 0, 0)[Line(V.Lines[I], bF, bF ? (I - Kept) * 0.55 : 0)];
	}
	if (V.After)
	{
		// .talk-after: an aside, a moment after the lines
		const double Delay = bAfterNew ? (bFresh ? (V.Lines.Num() - Kept) * 0.55 : 0) + 0.9 : 0;
		LinesBox->AddSlot().AutoHeight().Padding(0, V.Lines.Num() ? 8.f : 0.f, 0, 0)[Line({ FString(), V.After->Text }, bAfterNew, Delay)];
	}
	// .talk-lines: a rule down the left and a warm tint behind
	const FW Lines = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(2.f)[SNew(SBorder).BorderImage(TalkBox(S47Kit::Css(0xffb7a4, 0.5f), FLinearColor::Transparent))]]
		+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SBorder).BorderImage(TalkBox(S47Kit::Css(0xffc896, 0.035f), FLinearColor::Transparent)).Padding(FMargin(14, 12))[LinesBox]];

	const TWeakPtr<SS47Talk> Self = StaticCastSharedRef<SS47Talk>(AsShared());
	TSharedRef<SVerticalBox> B = SNew(SVerticalBox);
	// the portrait floats left of the lines (132 px, 4:5)
	if (FaceTex)
	{
		B->AddSlot().AutoHeight().Padding(0, 4, 0, 14)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 2, 16 * PX, 10)[Cover(FaceTex, 132 * PX, 165 * PX)]
			+ SHorizontalBox::Slot().FillWidth(1.f)[Lines]
		];
	}
	else B->AddSlot().AutoHeight().Padding(0, 4, 0, 14)[Lines];
	auto Click = [Self](TFunction<void(SS47Talk&)> Fn) -> TFunction<void()> { return [Self, Fn] { if (const TSharedPtr<SS47Talk> P = Self.Pin()) { Fn(*P); P->Later(); } }; };
	if (V.Table)
	{
		B->AddSlot().AutoHeight()[S47Kit::Question(V.Table->Prompt)];
		TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(4));
		for (int32 I = 0; I < V.Table->Chips.Num(); ++I)
		{
			const S47Panels::FTalkChip& C = V.Table->Chips[I];
			const FString Id = C.Id;
			TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
			if (!C.Image.IsEmpty()) Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 10, 0)[Cover(S47Image::FromDataUrl(C.Image), 54 * PX, 40 * PX)];
			Row->AddSlot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(C.Label + (C.bHave ? TEXT("") : TEXT(" / not in the case")))).Font(S47Style::Mono(19.f * PX)).AutoWrapText(true)
				.ColorAndOpacity(C.bOn ? S47Kit::Css(0x12100e) : S47Kit::Css(0xe9e2cf))
			];
			const TFunction<void()> OnChip = Click([Id](SS47Talk& T) { T.StopCharacter(); if (T.O.OnChip) T.O.OnChip(Id); });
			// .lp-choice.talk-chip, [aria-pressed=true], [disabled] at .45
			Grid->AddSlot(I % 3, I / 3)
			[
				SNew(SButton).ButtonStyle(BareButton()).IsEnabled(C.bHave)
				.OnClicked_Lambda([OnChip] { OnChip(); return FReply::Handled(); })
				[
					SNew(SBorder).ColorAndOpacity(FLinearColor(1, 1, 1, C.bHave ? 1.f : 0.45f))
					.BorderImage(C.bOn ? TalkBox(S47Kit::Css(0xc9d6c2), S47Kit::Css(0xc9d6c2)) : TalkBox(FLinearColor::Transparent, S47Kit::Css(0xe9e2cf, 0.25f)))
					.Padding(FMargin(10 * PX, 8 * PX))
					[
						SNew(SBox).MinDesiredHeight(48 * PX - 16 * PX).VAlign(VAlign_Center)[Row]
					]
				]
			];
		}
		B->AddSlot().AutoHeight().Padding(0, 4, 0, 8)[Grid];
		B->AddSlot().AutoHeight()[S47Kit::Row({ S47Kit::Button(V.Table->Submit, Click([](SS47Talk& T) { T.StopCharacter(); if (T.O.OnTable) T.O.OnTable(); }), true) })];
	}
	if (!V.Question.IsEmpty()) B->AddSlot().AutoHeight()[S47Kit::Question(V.Question)];
	{
		TArray<FW> Choices;
		for (const S47Panels::FTalkChoice& C : V.Choices)
		{
			const FString Id = C.Id;
			Choices.Add(S47Kit::Button(C.Label, Click([Id](SS47Talk& T)
			{
				// (Earl's voice, earlDialogue.stop(): not in this version)
				T.StopCharacter();
				if (T.O.OnChoice) T.O.OnChoice(Id);
			}), false, !C.bDisabled));
		}
		B->AddSlot().AutoHeight()[S47Kit::Col(Choices, 8.f)];
	}
	// (a voiced quote, [data-character-quote], is the voice's: the description reader leaves it)
	if (V.Reply) B->AddSlot().AutoHeight()[bReply ? S47Kit::Unread(S47Kit::Text(V.Reply->Text, V.Reply->bOk ? 1 : -1)) : S47Kit::Text(V.Reply->Text, V.Reply->bOk ? 1 : -1)];
	if (V.Given.Num())
	{
		B->AddSlot().AutoHeight()[S47Kit::Small(TEXT("On the table"))];
		TArray<FW> Read;
		for (const S47Panels::FTalkGiven& G : V.Given)
		{
			const FString Id = G.Id;
			Read.Add(S47Kit::Button(G.Label, [Self, Id] { if (const TSharedPtr<SS47Talk> P = Self.Pin(); P.IsValid() && P->O.OnRead) P->O.OnRead(Id); }));
		}
		B->AddSlot().AutoHeight()[S47Kit::Row(Read)];
	}
	Body->SetContent(B);
	if (bFresh && O.OnLines) O.OnLines(V.Lines);
	// (a line added under the others is brought into view: in this version's talks lines are only replaced)
	if (PlayCharacter && UI.IsValid() && UI->Modal()) PlayCharacter();
}

S47Panels::FTalkHandle S47Panels::Talk(US47UI* UI, const FTalk& O)
{
	TSharedRef<SS47Talk> W = SNew(SS47Talk);
	W->Setup(UI, O);
	// shell(ui, o.title, ..., o.onClose): opened first, then drawn (render() draws only while connected)
	if (UI) UI->Open(S47Kit::Shell(UI, O.Title, W), O.OnClose);
	W->Render();
	const TWeakPtr<SS47Talk> Wk = W;
	return { [Wk] { if (const TSharedPtr<SS47Talk> P = Wk.Pin()) P->Render(); } };
}

// ---------- the signal processor on the control room rack ----------

void S47Panels::DecoderScopeBar(int32 I, const FString& Mode, double H, double& OutX, double& OutHh)
{
	static const double Four[4] = { 0, 3, 7, 10 }, Seven[7] = { 12, 10, 7, 5, 3, 2, 0 };
	OutX = 30 + (I < 4 ? I : I + 1.5) * 47;
	OutHh = (I < 4 ? 0.55 : 0.75) * H * (Mode == TEXT("raw") ? 0.8 : 0.5 + 0.05 * (I < 4 ? Four[I] : Seven[I - 4]));
}

// <canvas class="dsp-scope" width="640" height="120">: the eleven pulses of one group cycle, the
// sounding one lit, drawn every frame (requestAnimationFrame) while the panel is open. The canvas is
// stretched to the panel's width at 120 px high (.dsp-scope width 100%, max-height 120px), so it is
// drawn here in its own 640 x 120 and scaled the same way.
class SS47DspScope : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SS47DspScope) {}
		SLATE_ARGUMENT(TFunction<S47Panels::FDecoderView()>, View)
	SLATE_END_ARGS()
	void Construct(const FArguments& A) { View = A._View; White = MakeUnique<FSlateColorBrush>(FLinearColor::White); }
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(640 * PX, 120 * PX); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
	{
		if (!View) return Layer;
		const S47Panels::FDecoderView V = View();
		const double W = 640, H = 120;
		const float Sx = float(G.GetLocalSize().X / W), Sy = float(G.GetLocalSize().Y / H);
		auto Rect = [&](double X, double Y, double Rw, double Rh, const FLinearColor& C, int32 L)
		{
			FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(FVector2f(float(Rw) * Sx, float(Rh) * Sy), FSlateLayoutTransform(FVector2f(float(X) * Sx, float(Y) * Sy))), White.Get(), ESlateDrawEffect::None, C);
		};
		// (Unreal: the ANSWER row's key lights its bar while nothing plays; the player says -1 otherwise)
		const int32 Cur = V.Current ? V.Current() : -1;
		Rect(0, 0, W, H, S47Kit::Css(0x04110a), Layer);
		for (int32 I = 0; I < 11; ++I)
		{
			double X = 0, Hh = 0;
			S47Panels::DecoderScopeBar(I, V.Mode, H, X, Hh);
			Rect(X, H - 10 - Hh, 30, Hh, I == Cur ? S47Kit::Css(0xb9ffc7) : S47Kit::Css(0x6edc96, 0.35f), Layer + 1);
		}
		// '18px VT323' at (12, 22) on its baseline: the text's top a little under 8
		const FString Line = FString::Printf(TEXT("%s  4 / 7  %s"), V.Mode == TEXT("raw") ? TEXT("RAW") : TEXT("HARM"), V.bPlaying ? TEXT("PLAY") : TEXT("STOP"));
		FSlateDrawElement::MakeText(Out, Layer + 2, G.ToPaintGeometry(FVector2f(600.f * Sx, 24.f * Sy), FSlateLayoutTransform(FVector2f(12.f * Sx, (22.f - 18.f * 0.8f) * Sy))), Line, S47Style::Mono(18.f * Sy), ESlateDrawEffect::None, S47Kit::Css(0x8cffa4, 0.8f));
		return Layer + 3;
	}

private:
	TFunction<S47Panels::FDecoderView()> View;
	TUniquePtr<FSlateBrush> White;
};

S47Panels::FPanelHandle S47Panels::DecoderPanel(US47UI* UI, const FDecoderPanel& O)
{
	const TFunction<FDecoderView()> View = O.View;
	const TFunction<void(const FString&)> OnMode = O.OnMode;
	const TFunction<void(const FString&, double)> OnParam = O.OnParam;
	const TFunction<void(bool)> OnPlay = O.OnPlay;
	auto V = [View] { return View ? View() : FDecoderView(); };
	// sync(): the tape line, which mode is pressed, the knobs' values and the play button follow the view
	// (Slate's attributes read them as the panel draws)
	auto Mode = [OnMode](const FString& M) { return [OnMode, M] { if (OnMode) OnMode(M); }; };
	// (Number(i.value): the range's value on its step, as the input gives it)
	auto Param = [OnParam](const FString& K, double Step) { return [OnParam, K, Step](float X) { if (OnParam) OnParam(K, FMath::RoundToDouble(double(X) / Step) * Step); }; };
	const FW Tape = SNew(STextBlock).Text_Lambda([V] { return FText::FromString(V().Tape); })
		.Font(S47Style::Mono(19.f * PX)).ColorAndOpacity(S47Kit::Css(0xffb052)).AutoWrapText(true).Margin(FMargin(0, 0, 0, 10));
	// .dsp-scope: margin 4px 0 10px, a faint green line round it
	static const FSlateRoundedBoxBrush ScopeLine(FLinearColor::Transparent, 0.f, S47Kit::Css(0x8cffa4, 0.25f), 1.f);
	const FW Scope = SNew(SBox).Padding(FMargin(0, 4 * PX, 0, 10 * PX))
		[
			SNew(SBorder).BorderImage(&ScopeLine).Padding(1.f)
			[
				SNew(SBox).HeightOverride(120 * PX)[SNew(SS47DspScope).View(View)]
			]
		];
	const FW Modes = S47Kit::Row({
		S47Kit::Choice(TEXT("RAW TAPE"), TAttribute<bool>::CreateLambda([V] { return V().Mode == TEXT("raw"); }), Mode(TEXT("raw")), true),
		S47Kit::Choice(TEXT("HARMONIZED"), TAttribute<bool>::CreateLambda([V] { return V().Mode == TEXT("harmonized"); }), Mode(TEXT("harmonized")), true) });
	// .dsp-knobs: three columns, 14 px apart, 10 px above and below
	const FW Knobs = SNew(SBox).Padding(FMargin(0, 10 * PX))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 7 * PX, 0)
			[
				S47Kit::Knob(TEXT("PITCH"), TAttribute<FString>::CreateLambda([V] { const double P = V().Pitch; return FString::Printf(TEXT("%s%s st"), P > 0 ? TEXT("+") : TEXT(""), *FString::SanitizeFloat(P, 0)); }),
					-12.f, 12.f, 1.f, TAttribute<float>::CreateLambda([V] { return float(V().Pitch); }), Param(TEXT("pitch"), 1))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(7 * PX, 0)
			[
				S47Kit::Knob(TEXT("TIME"), TAttribute<FString>::CreateLambda([V] { return FString::Printf(TEXT("x%.1f"), V().Stretch); }),
					1.f, 6.f, 0.5f, TAttribute<float>::CreateLambda([V] { return float(V().Stretch); }), Param(TEXT("stretch"), 0.5))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(7 * PX, 0, 0, 0)
			[
				S47Kit::Knob(TEXT("SPACE"), TAttribute<FString>::CreateLambda([V] { return FString::Printf(TEXT("%d%%"), int32(FMath::FloorToDouble(V().Space * 100 + 0.5))); }),
					0.f, 1.f, 0.05f, TAttribute<float>::CreateLambda([V] { return float(V().Space); }), Param(TEXT("space"), 0.05))
			]
		];
	const FW PlayRow = S47Kit::Row({ S47Kit::ButtonLive(TAttribute<FString>::CreateLambda([V] { return V().bPlaying ? FString(TEXT("STOP")) : FString(TEXT("PLAY TAPE")); }),
		[V, OnPlay] { if (OnPlay) OnPlay(!V().bPlaying); }, true) });
	// (Unreal only, on Tom's request: the ANSWER row, S47DecoderDesk.h) seven keys, the seven of the group
	// as the harmonized playback sounds them, 1 the first and highest; the last seven pressed beside them
	const TFunction<void(int32)> OnKey = O.OnKey;
	TArray<FW> Keys;
	Keys.Add(SNew(SBox).VAlign(VAlign_Center).Padding(FMargin(0, 0, 6 * PX, 0))
		[SNew(STextBlock).Text(FText::FromString(TEXT("ANSWER"))).Font(S47Style::Mono(19.f * PX)).ColorAndOpacity(S47Kit::Css(0xffb052))]);
	for (int32 K = 1; K <= 7; ++K)
		Keys.Add(S47Kit::Button(FString::FromInt(K), [OnKey, K] { if (OnKey) OnKey(K); }, false, TAttribute<bool>::CreateLambda([V] { return !V().bAnswering; })));
	Keys.Add(SNew(SBox).VAlign(VAlign_Center).Padding(FMargin(8 * PX, 0, 0, 0))
		[
			SNew(STextBlock).Text_Lambda([V]
			{
				const FDecoderView Vw = V();
				if (Vw.bAnswering) return FText::FromString(TEXT("- - -  ANSWERING  - - -"));
				FString S;
				for (int32 I = 0; I < 7; ++I) S += (I ? TEXT(" ") : TEXT("")) + (Vw.Keys.IsValidIndex(I) ? FString::FromInt(Vw.Keys[I]) : FString(TEXT("-")));
				return FText::FromString(S);
			}).Font(S47Style::Mono(19.f * PX)).ColorAndOpacity(S47Kit::Css(0x8cffa4))
		]);
	// (Unreal only) once his page has been read, a faint header over the keys: what the page and the
	// keys have to do with each other, not the order itself
	const FW Answer = S47Kit::Col({
		SNew(SBox).Padding(FMargin(2 * PX, 6 * PX, 0, 0)).Visibility_Lambda([V] { return V().bPageRead ? EVisibility::Visible : EVisibility::Collapsed; })
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("the order it sang back"))).Font(S47Style::Mono(16.f * PX)).ColorAndOpacity(S47Kit::Css(0xffb052, 0.42f))
		],
		S47Kit::Row(Keys) }, 2.f * PX);
	const FW Body = S47Kit::Col({
		Tape,
		Scope,
		Modes,
		Knobs,
		PlayRow,
		Answer,
		// (Unreal: the processor is the bought unit on the console desk, not the rack, when the export has it)
		S47Kit::Small(FString::Printf(TEXT("The deck keeps what the receiver locked on. Slowed down and harmonized, the pulses stop sounding like a machine. The processor keeps playing from the %s while you walk around."), O.bOnDesk ? TEXT("desk") : TEXT("rack"))),
		// (Unreal only) a nudge toward T. Vega's page while it is still under the spare reel
		SNew(SBox).Visibility_Lambda([V] { return V().bPageThere ? EVisibility::Visible : EVisibility::Collapsed; })
		[
			S47Kit::Small(TEXT("Someone keeps leaving papers under the spare reel."))
		] }, 0.f);
	if (UI) UI->Open(S47Kit::Shell(UI, TEXT("SIGNAL PROCESSOR / DSP-4"), Body), O.OnClose);
	// { el, refresh: sync }: the attributes follow the view by themselves
	return { [] {} };
}

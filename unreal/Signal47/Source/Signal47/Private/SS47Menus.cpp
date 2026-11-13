// The menus' screens in the approved look (S47Theme): the title (UI.title) and the pause menu
// (UI.pause). Declared in SS47Screens.h with the web version's other screens; the settings are
// SS47Settings and the case menus SS47SaveMenu.
#include "SS47Screens.h"

#include "Framework/Application/SlateApplication.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "S47Docs.h"
#include "S47Keys.h"
#include "S47Gamepad.h"
#include "S47Theme.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

namespace S47MenusLocal
{
	TSharedRef<SWidget> InputHints(const TArray<S47Theme::FHint>& Keyboard, const TArray<S47Theme::FHint>& Controller)
	{
		return SNew(SOverlay)
			+ SOverlay::Slot()[SNew(SBox).Visibility_Lambda([] { return S47Gamepad::UsingGamepad() ? EVisibility::Collapsed : EVisibility::Visible; })[S47Theme::KeyHints(Keyboard)]]
			+ SOverlay::Slot()[SNew(SBox).Visibility_Lambda([] { return S47Gamepad::UsingGamepad() ? EVisibility::Visible : EVisibility::Collapsed; })[S47Theme::KeyHints(Controller)]];
	}

	void Focus(const TSharedPtr<SWidget>& W)
	{
		if (W.IsValid() && FSlateApplication::IsInitialized()) FSlateApplication::Get().SetAllUserFocus(W, EFocusCause::SetDirectly); // (not Navigation: Slate would draw its own focus frame)
	}
}

// ---------- the title (UI.title) ----------

namespace S47MenusLocal
{
	// the logo's words: SIGNAL / 47, the slash in neon; Edge: one of the tape's coloured edges
	TSharedRef<SWidget> Logo(const TOptional<FLinearColor>& Edge, TFunction<float()> NumberAlpha, TFunction<SS47Title::FErase47Sample()> NumberSample)
	{
		const FSlateFontInfo F = S47Theme::Heading(150, 2);
		const FLinearColor Word = Edge.Get(S47Theme::Head()), Slash = Edge.Get(S47Theme::Neon());
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("SIGNAL "))).Font(F).ColorAndOpacity(Word)]
			+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("/"))).Font(F).ColorAndOpacity(Slash)]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock).Text(FText::FromString(TEXT(" 47"))).Font(F)
				.ColorAndOpacity_Lambda([NumberAlpha,Word] { auto C=Word;C.A*=NumberAlpha();return C; })
				.RenderTransform_Lambda([NumberSample]
				{
					const auto S=NumberSample();
					return FSlateRenderTransform(FScale2D(S.Scale.X,S.Scale.Y),FVector2D(S.Offset.X,S.Offset.Y));
				})
			];
	}

	// the night's clock on the title: from 23:41:07, in real seconds
	constexpr double TITLE_CLOCK = 23 * 3600 + 41 * 60 + 7;
}

void SS47Title::Construct(const FArguments& InArgs)
{
	S47_USING_THEME
	O = InArgs._Opts;
	bGated = O.bGate;
 PassT=0.f;
	EraseT=O.bErase47 ? FMath::Clamp(O.Erase47InitialTime,0.f,2.7f) : 0.f;
	bEraseStarted=O.bErase47 && EraseT>=1.f;
	// Even a completed seed may retry Done on its first visible tick when the
	// persisted flag could not be written. A persisted bHide47 bypasses callbacks.
	bEraseDone=false;
	// .menu: Continue (and where it would take you), New night, Load case, Settings, Quit
	Items.Add({ TEXT("CONTINUE"), !O.Cont.IsEmpty(), [this] { if (O.OnContinue) O.OnContinue(); } });
	Items.Add({ TEXT("NEW STORY"), true, [this] { if (O.OnStart) O.OnStart(); } });
	if (O.OnFreeform) Items.Add({ TEXT("NEW FREEFORM"), true, [this] { O.OnFreeform(); } });
	Items.Add({ TEXT("LOAD CASE"), O.bCanLoad, [this] { if (O.OnLoad) O.OnLoad(); } });
	Items.Add({ TEXT("SETTINGS"), true, [this] { if (O.OnSettings) O.OnSettings(); } });
	if (O.OnDev) Items.Add({ TEXT("DEVELOPER"), true, [this] { if (O.OnDev) O.OnDev(); } });
	if (O.OnQuit) Items.Add({ TEXT("QUIT"), true, [this] { if (O.OnQuit) O.OnQuit(); } });
	Sel = Items[0].bUsable ? 0 : 1;

	TSharedRef<SVerticalBox> Menu = SNew(SVerticalBox);
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		Menu->AddSlot().AutoHeight().Padding(0, 0, 0, 6 * U)
		[
			SNew(SS47MenuItem).Label(Items[i].Label).Px(42).Spacing(2).Pad(FMargin(15, 4, 18, 4)).bArrow(true)
			.Meta(i == 0 ? O.Cont.ToUpper() : FString())
			.Selected_Lambda([this, i] { return Sel == i && !bGated; })
			.Usable(Items[i].bUsable)
			.OnHover([this, i] { Sel = i; })
			.OnClick([this, i] { Sel = i; Choose(); })
		];
	}

	auto MenuVis = [this] { return bGated ? EVisibility::Collapsed : EVisibility::Visible; };
	auto FadeIn = [this] { return FLinearColor(1, 1, 1, 1.f - FMath::Square(1.f - FMath::Clamp(InT / 0.5f, 0.f, 1.f))); };
	// (line-height .86: Oswald's own line is about 1.48 em, so the words sit higher than the box CSS gives them)
	const float LogoTop = 128 - (150 * 1.482f - 150 * 0.86f) / 2.f;
	auto NumberAlpha=[this] { return Number47Opacity(); };
	auto NumberSample=[this]
	{
		auto Sample=SampleErase47(EraseT,O.bCalm);
		Sample.Offset*=S47Theme::U;
		return Sample;
	};

	ChildSlot
	[
		SNew(SOverlay)
		// the tape's display
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(Pad(56, 40, 0, 0))[SNew(SS47Osd).Text(FString(TEXT("PLAY"))).bPlay(true)]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(Pad(0, 40, 56, 0))[SNew(SS47Osd).Text(FString(TEXT("SP")))]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(Pad(0, 0, 56, 70))
		[
			SNew(SS47Osd).Px(30).Text_Lambda([this] { FString D, Clock; return Stamp(S47MenusLocal::TITLE_CLOCK + T).Split(TEXT("  "), &D, &Clock) ? Clock : FString(); }) // D46: Tom: no place or date on the title, only the tape's clock
		]
		// .logo: SIGNAL / 47 with the tape's red and cyan edges, and the line under it
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(Pad(118, LogoTop, 0, 0))
		[
			SNew(SOverlay).Visibility(EVisibility::HitTestInvisible)
			+ SOverlay::Slot()[SNew(SBox).RenderTransform(FSlateRenderTransform(FVector2f(3 * U, 0)))[S47MenusLocal::Logo(FLinearColor(1.f, 0.06f, 0.045f, 0.35f),NumberAlpha,NumberSample)]]
			+ SOverlay::Slot()[SNew(SBox).RenderTransform(FSlateRenderTransform(FVector2f(-3 * U, 0)))[S47MenusLocal::Logo(FLinearColor(0.06f, 0.52f, 1.f, 0.3f),NumberAlpha,NumberSample)]]
			+ SOverlay::Slot()[S47MenusLocal::Logo(TOptional<FLinearColor>(),NumberAlpha,NumberSample)]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(Pad(118, 128 + 129 + 18, 0, 0))
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("NIGHT SHIFT AT SARO."))).Font(Term(30, 3)).ColorAndOpacity(Amber())
		]
		// .menu, or the gate before it
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(Pad(104, 418, 0, 0))
		[
			SNew(SBox).WidthOverride(560 * U)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBorder")).Padding(0).Visibility_Lambda(MenuVis).ColorAndOpacity_Lambda(FadeIn)
					[
						Menu
					]
				]
				+ SOverlay::Slot().Padding(Pad(21, 4, 0, 0))
				[
					// .gate: blinks (opacity 1 to .35 and back, 1.4 s each way)
					SNew(STextBlock).Text_Lambda([this] { return FText::FromString(O.bTouch ? TEXT("TAP TO BEGIN") : S47Gamepad::UsingGamepad() ? TEXT("PRESS A TO BEGIN") : TEXT("PRESS ANY KEY")); }).Font(Term(42, 2))
					.ColorAndOpacity_Lambda([this]
					{
						const float Ph = FMath::Fmod(T, 2.8f), V = Ph < 1.4f ? Ph / 1.4f : 2.f - Ph / 1.4f;
						return FSlateColor(Paper(0.82f * (1.f - 0.65f * FMath::SmoothStep(0.f, 1.f, V))));
					})
					.Visibility_Lambda([this] { return bGated ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(Pad(122, 0, 0, 26))
		[
			SNew(SBox).Visibility_Lambda(MenuVis)
			[
				S47MenusLocal::InputHints(
					{ { TEXT("UP DOWN"), TEXT("SELECT"), nullptr }, { TEXT("ENTER"), TEXT("CONFIRM"), [this] { Choose(); } }, { TEXT("ESC"), TEXT("BACK"), [this] { Sel = Items.Num() - 1; } } },
					{ { TEXT("D-PAD"), TEXT("SELECT"), nullptr }, { TEXT("A"), TEXT("CONFIRM"), [this] { Choose(); } }, { TEXT("B"), TEXT("BACK"), [this] { Sel = Items.Num() - 1; } } })
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(Pad(0, 0, 40, 28))
		[
			SNew(STextBlock).Text(FText::FromString(O.Version)).Font(Term(24, 1)).ColorAndOpacity(Css(0x9a927f)) // D46: the build is readable
		]
		+ SOverlay::Slot()[Scanlines()]
	];
}

SS47Title::FErase47Sample SS47Title::SampleErase47(float Seconds, bool bCalm)
{
 FErase47Sample S;
 if(Seconds<1.f)return S;
 if(Seconds>=2.5f){S.Opacity=0;return S;}
 const float P=(Seconds-1.f)/1.5f;
 if(bCalm){S.Opacity=1.f-FMath::SmoothStep(0.f,1.f,P);return S;}
 // Uneven tape damage, confined to the two digits. No full-screen flashes.
 const float Shift[]={0,5,-3,0,12,-7,2,-14,4,0,-5,16,-2,7,-11,0,4};
 const float Alpha[]={1,.72f,.92f,.5f,.82f,.34f,.76f,.46f,.9f,.58f,.7f,.27f,.6f,.4f,.56f,.3f,.48f};
 const int32 I=FMath::Clamp(FMath::FloorToInt((Seconds-1.f)*11.3f),0,16);
 const float Envelope=FMath::SmoothStep(0.f,.24f,P);
 S.Opacity=FMath::Lerp(1.f,Alpha[I],Envelope)*(1.f-FMath::SmoothStep(.7f,1.f,P));
 S.Offset=FVector2f(Shift[I]*Envelope,((I%3)-1)*2.f*Envelope);
 S.Scale=FVector2f(1.f+FMath::Abs(Shift[I])*.018f*Envelope,1.f-(I%4)*.045f*Envelope);
 return S;
}

float SS47Title::Number47Opacity() const
{
 return O.bHide47 || bEraseDone ? 0.f : O.bErase47 ? SampleErase47(EraseT,O.bCalm).Opacity : 1.f;
}

// A distant lit object crossing the menu sky, not a world-space craft or story encounter.
SS47Title::FTitlePassSample SS47Title::SampleTitlePass(float Seconds)
{
 FTitlePassSample S;
 if(!FMath::IsFinite(Seconds) || Seconds<8.f)return S;
 const float Phase=FMath::Fmod(Seconds-8.f,35.f);
 if(Phase>=5.f)return S;
 const float P=Phase/5.f;
 S.Opacity=.9f*FMath::SmoothStep(0.f,.2f,P)*(1.f-FMath::SmoothStep(.8f,1.f,P));
 S.bVisible=S.Opacity>0.f;
 S.Position=FVector2f(FMath::Lerp(.96f,.64f,P),.19f-.035f*FMath::Sin(P*PI));
 return S;
}

void SS47Title::Tick(const FGeometry& G, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(G, InCurrentTime, InDeltaTime);
	T += InDeltaTime;
	InT += InDeltaTime;
 if(!bGated && GetVisibility().IsVisible() && FSlateApplication::IsInitialized())
 {
  auto& App=FSlateApplication::Get();const auto Window=App.FindWidgetWindow(SharedThis(this));
  if(App.IsActive() && Window.IsValid() && !Window->IsWindowMinimized())
  {
   const bool WasVisible=SampleTitlePass(PassT).bVisible;
   PassT+=FMath::Clamp(InDeltaTime,0.f,.1f);
   // Custom OnPaint geometry must invalidate Slate's otherwise-static title cache.
   if(WasVisible || SampleTitlePass(PassT).bVisible)Invalidate(EInvalidateWidgetReason::Paint);
  }
 }
	if(O.bErase47 && !O.bHide47 && !bEraseDone && !bGated && GetVisibility().IsVisible() && FSlateApplication::IsInitialized())
	{
		auto& App=FSlateApplication::Get(); const auto Window=App.FindWidgetWindow(SharedThis(this));
		if(App.IsActive() && Window.IsValid() && !Window->IsWindowMinimized())
		{
			// Presentation time only: hidden menus, inactive/minimized apps and a
			// slow frame cannot consume the entire one-time signal loss offscreen.
			EraseT=FMath::Min(2.7f,EraseT+FMath::Clamp(InDeltaTime,0.f,.1f));
			if(!bEraseStarted && EraseT>=1.f){bEraseStarted=true;if(O.OnErase47Start)O.OnErase47Start();}
			if(!bEraseDone && EraseT>=2.7f){bEraseDone=true;if(O.OnErase47Done)O.OnErase47Done();}
		}
	}
	// (focus() as the title is shown: Slate can only focus a widget that is on the screen)
	if (bFocusPending) { bFocusPending = false; FocusMenu(); }
}

int32 SS47Title::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	S47_USING_THEME
	// .shade: linear-gradient(90deg, rgba(5,7,13,.94) 0%, .82 34%, .35 58%, 0 75%) and from the foot, .85 to 0 at 30%; .vig
	const FVector2f S = FVector2f(G.GetLocalSize());
	Gradient(Out, Layer, G, FVector2f::ZeroVector, S, { { 0.f, Night(0.94f) }, { 0.34f, Night(0.82f) }, { 0.58f, Night(0.35f) }, { 0.75f, Night(0.f) }, { 1.f, Night(0.f) } }, true);
	Gradient(Out, Layer, G, FVector2f::ZeroVector, S, { { 0.f, Night(0.f) }, { 0.7f, Night(0.f) }, { 1.f, Night(0.85f) } }, false);
	Vignette(Out, Layer, G, 0.6f);
 const auto Pass=SampleTitlePass(PassT);
 if(Pass.bVisible)
 {
  // One lit object, clearly larger than any star: a soft glow and a white core, no craft shape.
  // A brush radius larger than the box draws a diamond, or nothing at the size of a star (Tom, 6 November).
  const FVector2f At(Pass.Position.X*S.X,Pass.Position.Y*S.Y);
  const float Scale=FMath::Clamp(S.X/1920.f,.65f,1.5f);
  static TMap<int32,TUniquePtr<FSlateRoundedBoxBrush>> Discs;
  const auto Dot=[&](float Radius,float Alpha,const FLinearColor& C)
  {
   const float R=FMath::Max(1.f,FMath::RoundToFloat(Radius*Scale*2.f)*.5f);
   TUniquePtr<FSlateRoundedBoxBrush>& Disc=Discs.FindOrAdd(FMath::RoundToInt32(R*2.f));
   if(!Disc)Disc=MakeUnique<FSlateRoundedBoxBrush>(FLinearColor::White,R);
   FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2f(2*R,2*R),FSlateLayoutTransform(At-FVector2f(R,R))),Disc.Get(),ESlateDrawEffect::None,C.CopyWithNewOpacity(Pass.Opacity*Alpha));
  };
  // Many faint discs add up to a falloff without a visible edge.
  for(int32 I=0;I<14;++I)Dot(FMath::Lerp(22.f,3.5f,FMath::Sqrt(I/13.f)),.04f,FLinearColor(1.f,.86f,.68f));
  Dot(3.5f,.8f,FLinearColor(1.f,.93f,.82f));Dot(2.5f,1.f,FLinearColor::White);
 }
	return SCompoundWidget::OnPaint(Args, G, Cull, Out, Layer + 1, Style, bParentEnabled);
}

void SS47Title::First()
{
	TFunction<void()> Fn = O.OnFirstTouch;
	O.OnFirstTouch = nullptr;
	if (Fn) Fn();
}

void SS47Title::Move(int32 Dir)
{
	for (int32 i = 1; i <= Items.Num(); ++i)
	{
		const int32 J = (Sel + Dir * i + Items.Num() * 2) % Items.Num();
		if (Items[J].bUsable) { Sel = J; return; }
	}
}

void SS47Title::Choose()
{
	if (bGated || !Items.IsValidIndex(Sel) || !Items[Sel].bUsable) return;
	// (a copy: choosing may take the title away)
	const TFunction<void()> Fn = Items[Sel].Fn;
	if (Fn) Fn();
}

FReply SS47Title::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	// main.ts firstTouch: the first key on the gated title brings the menu in (and the music)
	if (bGated) { Ungate(); First(); return FReply::Handled(); }
	First();
	const FKey K = E.GetKey();
	if (K == EKeys::Up || K == EKeys::W || K == EKeys::Gamepad_DPad_Up) { Move(-1); return FReply::Handled(); }
	if (K == EKeys::Down || K == EKeys::S || K == EKeys::Gamepad_DPad_Down) { Move(1); return FReply::Handled(); }
	if (K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom) { Choose(); return FReply::Handled(); }
	// Escape: to Quit (a second Escape does not quit by itself)
	if (K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right) { Sel = Items.Num() - 1; return FReply::Handled(); }
	return FReply::Unhandled();
}

FNavigationReply SS47Title::OnNavigation(const FGeometry& G, const FNavigationEvent& E)
{
	// a gamepad's stick (the d-pad and the keys are OnKeyDown's)
	if (bGated) return FNavigationReply::Stop();
	if (E.GetNavigationType() == EUINavigation::Up) Move(-1);
	else if (E.GetNavigationType() == EUINavigation::Down) Move(1);
	return FNavigationReply::Stop();
}

FReply SS47Title::OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E)
{
	if (bGated) { Ungate(); First(); return FReply::Handled(); }
	First();
	return FReply::Unhandled();
}

void SS47Title::Ungate()
{
	// UI.ungate
	if (!bGated) return;
	bGated = false;
	InT = 0.f;
	FocusMenu();
}

void SS47Title::FocusMenu()
{
	S47MenusLocal::Focus(SharedThis(this));
}

void SS47Title::FocusStart()
{
	if (bGated) return;
	Sel = 1;
	S47MenusLocal::Focus(SharedThis(this));
}

// ---------- the pause menu (UI.pause) ----------

namespace S47MenusLocal
{
	// .card: tonight's paper, a little crooked, ruled every 32 px, its shadow under it
	class SS47Card : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SS47Card) {}
			SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs) { ChildSlot.Padding(S47Theme::Pad(26, 22, 26, 24))[InArgs._Content.Widget]; }
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override
		{
			S47_USING_THEME
			const FVector2f S = FVector2f(G.GetLocalSize());
			// box-shadow: 0 18px 40px rgba(0,0,0,.55)
			Shadow(Out, Layer, G, FVector2f::ZeroVector, S, 18 * U, 30 * U, 0.55f);
			Fill(Out, Layer + 1, G, FVector2f::ZeroVector, S, Paper());
			// repeating-linear-gradient(180deg, transparent 0 31px, rgba(35,58,122,.18) 31px 32px)
			for (float Y = 31 * U; Y < S.Y; Y += 32 * U) Fill(Out, Layer + 1, G, FVector2f(0, Y), FVector2f(S.X, 1 * U), Css(0x233a7a, 0.18f));
			return SCompoundWidget::OnPaint(Args, G, Cull, Out, Layer + 2, Style, bParentEnabled);
		}
	};

	// TONIGHT-NOTE-20261109: the mark before a line of the note: a typed box, ticked by hand when it is done, and the supervisor's red
	// pencil arrow before the next step
	class SS47NoteMark : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SS47NoteMark) {}
			SLATE_ARGUMENT(S47TonightNote::EMark, Mark)
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs) { Mark = InArgs._Mark; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(40, 22) * S47Theme::U; }
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override
		{
			S47_USING_THEME
			const auto Line = [&](TArray<FVector2f> P, const FLinearColor& C, float Th) { FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), MoveTemp(P), ESlateDrawEffect::None, C, true, Th); };
			const float X = 22 * U, Y = 5 * U, B = 12 * U;
			Line({ { X, Y }, { X + B, Y }, { X + B, Y + B }, { X, Y + B }, { X, Y } }, Ink(Mark == S47TonightNote::EMark::Done ? 0.35f : 0.6f), 1.2f * U);
			if (Mark == S47TonightNote::EMark::Done)
				Line({ { X + 1.5f * U, Y + 5.5f * U }, { X + 5.f * U, Y + 10.5f * U }, { X + 15.f * U, Y - 4.f * U } }, Css(0x233a7a, 0.95f), 2.2f * U);
			if (Mark == S47TonightNote::EMark::Next)
			{
				const float M = Y + B * 0.5f;
				Line({ { 1.f * U, M + 0.5f * U }, { 17.f * U, M } }, Css(0x8a3b2c), 2.f * U);
				Line({ { 11.5f * U, M - 4.5f * U }, { 17.f * U, M }, { 11.f * U, M + 4.5f * U } }, Css(0x8a3b2c), 2.f * U);
			}
			return Layer;
		}
	private:
		S47TonightNote::EMark Mark = S47TonightNote::EMark::Open;
	};

	// the strip on the left: rgba(5,7,13,.96) to .86, with a thin amber line on its right
	class SS47Strip : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SS47Strip) {}
			SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs) { ChildSlot[InArgs._Content.Widget]; }
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override
		{
			S47_USING_THEME
			const FVector2f S = FVector2f(G.GetLocalSize());
			Gradient(Out, Layer, G, FVector2f::ZeroVector, S, { { 0.f, Night(0.96f) }, { 1.f, Night(0.86f) } }, true);
			Fill(Out, Layer, G, FVector2f(S.X - 1, 0), FVector2f(1, S.Y), Amber(0.25f));
			return SCompoundWidget::OnPaint(Args, G, Cull, Out, Layer + 1, Style, bParentEnabled);
		}
	};
}

void SS47Pause::Construct(const FArguments& InArgs)
{
	S47_USING_THEME
	O = InArgs._Opts.IsValid() ? InArgs._Opts : MakeShared<FS47PauseOpts>();
	OnClose = InArgs._OnClose;
	OnSettings = InArgs._OnSettings;
	OnSub = InArgs._OnSub;
	const TSharedPtr<FS47PauseOpts> P = O;
	// RESUME, SAVE CASE and LOAD CASE (when offered), SETTINGS, CONTROLS, QUIT TO TITLE
	Items.Add({ TEXT("RESUME"), [this] { if (OnClose) OnClose(false); } });
	if (O->OnSave) Items.Add({ TEXT("SAVE CASE"), [this] { if (OnSub) OnSub(O->OnSave); } });
	if (O->OnLoad) Items.Add({ TEXT("LOAD CASE"), [this] { if (OnSub) OnSub(O->OnLoad); } });
	Items.Add({ TEXT("SETTINGS"), [this] { if (OnSettings) OnSettings(O->Tab, Sel); } });
	Items.Add({ TEXT("CONTROLS"), [this] { if (OnSettings) OnSettings(TEXT("controls"), Sel); } });
	Items.Add({ TEXT("QUIT TO TITLE"), [this] { if (OnSub) OnSub(O->OnTitle); else if (O->OnTitle) O->OnTitle(); }, true });
	Sel = FMath::Clamp(InArgs._Selected, 0, Items.Num() - 1);

	TSharedRef<SVerticalBox> Menu = SNew(SVerticalBox);
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		Menu->AddSlot().AutoHeight().Padding(0, Items[i].bQuit ? 20 * U : 0.f, 0, 6 * U)
		[
			SNew(SS47MenuItem).Label(Items[i].Label).Px(38).Spacing(2).Pad(FMargin(14, 3, 14, 3))
			.Color(Items[i].bQuit ? Mute() : Paper(0.84f))
			.Selected_Lambda([this, i] { return Sel == i; })
			.OnHover([this, i] { Sel = i; })
			.OnClick([this, i] { Sel = i; const TFunction<void()> Fn = Items[i].Fn; if (Fn) Fn(); })
		];
	}

	// .st: the chapter (its number in amber), the clock and the place, the case and when it was saved
	const TPair<FString, FString> Ch = ChapterLine(P->Chapter);
	const FSlateFontInfo St = Term(25, 1);
	TSharedRef<SVerticalBox> Status = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(Ch.Key)).Font(St).ColorAndOpacity(Amber())]
			+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(Ch.Value.IsEmpty() ? FString() : TEXT(" · ") + Ch.Value)).Font(St).ColorAndOpacity(Mute())]
		];
	FString Where = P->Place.ToUpper();
	if (P->Clock >= 0) Where = S47Text::ClockText(P->Clock, false) + (Where.IsEmpty() ? FString() : TEXT(" · ") + Where);
	if (!Where.IsEmpty()) Status->AddSlot().AutoHeight().Padding(0, 8.75f * U, 0, 0)[SNew(STextBlock).Text(FText::FromString(Where)).Font(St).ColorAndOpacity(Mute())];
	if (!P->CaseLine.IsEmpty()) Status->AddSlot().AutoHeight().Padding(0, 8.75f * U, 0, 0)[SNew(STextBlock).Text(FText::FromString(P->CaseLine)).Font(St).ColorAndOpacity(Mute())];

	// .card: TONIGHT, the objective (the place before its //, what to do after it), the latest note in hand
	FString Place, Doing = P->Objective;
	if (P->Objective.Split(TEXT(" // "), &Place, &Doing)) Place = Place.TrimStartAndEnd();
	if (Doing.IsEmpty() && P->OpenTasks.Num()) Doing = P->OpenTasks[0];
	// TONIGHT-NOTE-20261109: the night's plan as the supervisor types it (S47TonightNote): each line's room and window, done ticked
	// and struck, the next step under the red arrow; then what else is open, and what the clock allows now in red pencil. The
	// objective alone, as before, only when there is no plan to show.
	const S47TonightNote::FNote& Note = P->Note;
	const bool bPlan = !Note.Lines.IsEmpty();
	const bool bCard = bPlan || !Note.IsEmpty() || !Doing.IsEmpty() || !P->LastNote.IsEmpty();
	TSharedRef<SVerticalBox> Card = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Text(FText::FromString(bPlan || Place.IsEmpty() ? TEXT("TONIGHT") : TEXT("TONIGHT · ") + Place)).Font(Term(20, 2)).ColorAndOpacity(Css(0x8a3b2c))
		];
	// TASKS-TEN-20261113 (change 6): in Freeform by group, WORK, FOLLOW-UP, YOUR OWN TIME (a head where the group changes), four lines
	// at most, each with its giver, window and worth under it (change 9), and one "+N open matters" for the notebook
	int32 LastKind = -1;
	for (const S47TonightNote::FLine& L : Note.Lines)
	{
		const bool bDone = L.Mark == S47TonightNote::EMark::Done, bNext = L.Mark == S47TonightNote::EMark::Next;
		const FString Tag = S47TonightNote::TagText(L);
		if (!Tag.IsEmpty() && !bDone && int32(L.Kind) != LastKind)
		{
			LastKind = L.Kind;
			Card->AddSlot().AutoHeight().Padding(0, 7 * U, 0, 0)[SNew(STextBlock).Text(FText::FromString(S47TonightNote::KindHead(L.Kind))).Font(Term(15, 2)).ColorAndOpacity(Css(0x8a3b2c, 0.85f))];
		}
		const FLinearColor Color = bDone ? Css(0x6d7895) : bNext ? Ink() : Ink(0.82f);
		FTextBlockStyle LineStyle = FTextBlockStyle().SetFont(Type(16)).SetColorAndOpacity(Color);
		if (bDone)
		{
			FSlateBrush Strike = *FCoreStyle::Get().GetBrush("GenericWhiteBox");
			Strike.TintColor = Color;
			LineStyle.SetStrikeBrush(Strike);
		}
		Card->AddSlot().AutoHeight().Padding(0, (bNext ? 6 : 5) * U, 0, 0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)[SNew(S47MenusLocal::SS47NoteMark).Mark(L.Mark)]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Top).Padding(4 * U, 0, 0, 0)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString((L.bPinned ? TEXT("* ") : TEXT("")) + S47TonightNote::LineText(L))).TextStyle(&LineStyle).AutoWrapText(true).LineHeightPercentage(1.1f)]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Visibility(Tag.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible).Text(FText::FromString(Tag)).Font(Term(13, 1)).ColorAndOpacity(Css(0x6d7895))]
			]
		];
	}
	if (Note.More > 0)
	{
		const bool bMatters = LastKind >= 0 || Note.Matters.Num() > 0;
		Card->AddSlot().AutoHeight().Padding(44 * U, 2 * U, 0, 0)
		[
			SNew(STextBlock).Text(FText::FromString(bMatters ? FString::Printf(TEXT("+%d open %s in the notebook"), Note.More, Note.More == 1 ? TEXT("matter") : TEXT("matters")) : FString::Printf(TEXT("+%d more in the notebook"), Note.More))).Font(Hand(26)).ColorAndOpacity(Css(0x233a7a))
		];
	}
	if (Note.Also.Num())
	{
		Card->AddSlot().AutoHeight().Padding(0, 10 * U, 0, 0)[SNew(STextBlock).Text(FText::FromString(TEXT("ALSO OPEN"))).Font(Term(17, 2)).ColorAndOpacity(Css(0x8a3b2c, 0.85f))];
		for (const FString& A : Note.Also)
			Card->AddSlot().AutoHeight().Padding(0, 2 * U, 0, 0)[SNew(STextBlock).Text(FText::FromString(A)).Font(Type(14)).ColorAndOpacity(Ink(0.78f)).AutoWrapText(true).LineHeightPercentage(1.1f)];
	}
	if (!Note.Clock.IsEmpty())
	{
		Card->AddSlot().AutoHeight().Padding(0, 8 * U, 0, 0)
		[
			SNew(STextBlock).Text(FText::FromString(Note.Clock)).Font(Hand(28)).ColorAndOpacity(Css(0x8a3b2c)).AutoWrapText(true).LineHeightPercentage(0.8f)
		];
	}
	if (!bPlan && !Doing.IsEmpty())
	{
		Card->AddSlot().AutoHeight().Padding(0, 6 * U, 0, 0)
		[
			SNew(STextBlock).Text(FText::FromString(Doing)).Font(Type(19)).ColorAndOpacity(Ink()).AutoWrapText(true).LineHeightPercentage(1.2f)
		];
	}
	if (!P->LastNote.IsEmpty())
	{
		Card->AddSlot().AutoHeight().Padding(0, 10 * U, 0, 0)
		[
			SNew(STextBlock).Text(FText::FromString(P->LastNote)).Font(Hand(30)).ColorAndOpacity(Css(0x233a7a)).AutoWrapText(true).LineHeightPercentage(0.8f)
		];
	}

	ChildSlot
	[
		SNew(SOverlay)
		// .bg { filter: blur(3px) brightness(.55) saturate(.8) }, .vig
		+ SOverlay::Slot()[Backdrop(3.f, FLinearColor(0, 0, 0, 0.45f), true, 0.6f)]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(Pad(0, 40, 56, 0))[SNew(SS47Osd).Text(FString(TEXT("PAUSE"))).bPause(true)]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(Pad(0, 0, 56, 40))
		[
			SNew(SS47Osd).Px(30).Text([&P] { FString D, Clock; return P->Clock >= 0 && Stamp(P->Clock).Split(TEXT("  "), &D, &Clock) ? Clock : FString(); }()) // D46: no date in the menu
		]
		// .card, bottom right, turned 1.4 degrees
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(Pad(0, 0, 120, 150))
		[
			SNew(SBox).WidthOverride((bPlan ? 480 : 430) * U).Visibility(bCard ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
			.RenderTransform(FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(-1.4f)))).RenderTransformPivot(FVector2D(0.5f, 0.5f))
			[
				SNew(S47MenusLocal::SS47Card)[Card]
			]
		]
		// .strip
		+ SOverlay::Slot().HAlign(HAlign_Left)
		[
			SNew(SBox).WidthOverride(560 * U)
			[
				SNew(S47MenusLocal::SS47Strip)
				[
					SNew(SOverlay)
					+ SOverlay::Slot().VAlign(VAlign_Top).Padding(Pad(96, 110, 0, 0))
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("PAUSED"))).Font(Heading(72, 3)).ColorAndOpacity(Head())
					]
					+ SOverlay::Slot().VAlign(VAlign_Top).Padding(Pad(98, 214, 0, 0))[Status]
					+ SOverlay::Slot().VAlign(VAlign_Top).Padding(Pad(84, 350, 56, 0))[Menu]
					+ SOverlay::Slot().VAlign(VAlign_Bottom).Padding(Pad(98, 0, 0, 26))
					[
						S47MenusLocal::InputHints(
							{ { TEXT("ENTER"), TEXT("CHOOSE"), [this] { if (Items.IsValidIndex(Sel)) { const TFunction<void()> Fn = Items[Sel].Fn; if (Fn) Fn(); } } },
							  { TEXT("ESC"), TEXT("RESUME"), [this] { if (OnClose) OnClose(false); } } },
							{ { TEXT("D-PAD"), TEXT("SELECT"), nullptr }, { TEXT("A"), TEXT("CHOOSE"), [this] { if (Items.IsValidIndex(Sel)) { const TFunction<void()> Fn = Items[Sel].Fn; if (Fn) Fn(); } } },
							  { TEXT("B / MENU"), TEXT("RESUME"), [this] { if (OnClose) OnClose(false); } } })
					]
				]
			]
		]
		+ SOverlay::Slot()[Scanlines()]
	];
}

void SS47Pause::Tick(const FGeometry& G, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(G, InCurrentTime, InDeltaTime);
	S47Theme::KeepFocus(SharedThis(this));
}

void SS47Pause::Move(int32 Dir)
{
	Sel = (Sel + Dir + Items.Num()) % Items.Num();
}

FReply SS47Pause::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	const FKey K = E.GetKey();
	if (K == EKeys::Up || K == EKeys::W || K == EKeys::Gamepad_DPad_Up) { Move(-1); return FReply::Handled(); }
	if (K == EKeys::Down || K == EKeys::S || K == EKeys::Gamepad_DPad_Down) { Move(1); return FReply::Handled(); }
	if (K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom)
	{
		if (Items.IsValidIndex(Sel)) { const TFunction<void()> Fn = Items[Sel].Fn; if (Fn) Fn(); }
		return FReply::Handled();
	}
	// main.ts: Escape or P with the menu open closes it (and resumes); P is the pause's key as bound
	if (K == EKeys::Escape || S47Keys::Is(TEXT("pause"), K) || K == EKeys::Gamepad_FaceButton_Right || K == EKeys::Gamepad_Special_Right)
	{
		if (OnClose) OnClose(false);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FNavigationReply SS47Pause::OnNavigation(const FGeometry& G, const FNavigationEvent& E)
{
	if (E.GetNavigationType() == EUINavigation::Up) Move(-1);
	else if (E.GetNavigationType() == EUINavigation::Down) Move(1);
	return FNavigationReply::Stop();
}


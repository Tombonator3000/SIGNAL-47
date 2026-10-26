#include "SS47ScreenView.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "GameFramework/PlayerController.h"
#include "Input/HittestGrid.h"
#include "Layout/ArrangedWidget.h"
#include "Layout/WidgetPath.h"
#include "RenderingThread.h"
#include "S47Keys.h"
#include "S47Theme.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "Slate/WidgetRenderer.h"
#include "SS47Widgets.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SVirtualWindow.h"

namespace
{
	constexpr float StepSecs = 0.6f;    // the camera's way to the glass and back
	constexpr float Fill = 0.86f;       // how much of the view's height the glass takes
	constexpr int32 VirtualSlot = 4;    // the screens' virtual user (Unreal's widget interaction uses 0)
	enum ERoute { RMove, RDown, RUp, RDouble, RWheel };

	// a direction in Unreal's space as the walker's yaw and pitch (three.js: the camera looks down -z,
	// Euler YXZ; S47Flow's title camera aims the same way)
	void Aim(const FVector& Dir, double& Yaw, double& Pitch)
	{
		const FVector D(Dir.X, Dir.Z, Dir.Y);
		Yaw = FMath::Atan2(-D.X, -D.Z);
		Pitch = FMath::Atan2(D.Y, FMath::Sqrt(D.X * D.X + D.Z * D.Z));
	}

	// a point on the glass's plane as the picture's pixels (0, 0 its top left)
	FVector2D Pixel(const FVector& P, const FVector& Centre, const FVector& Right, const FVector& Up, const FVector2D& Size)
	{
		const FVector Q = P - Centre;
		return FVector2D((FVector::DotProduct(Q, Right) / Size.X + 0.5) * SS47ScreenView::PicW, (0.5 - FVector::DotProduct(Q, Up) / Size.Y) * SS47ScreenView::PicH);
	}

	float Ease(float T) { return T * T * (3.f - 2.f * T); }
}

void SS47ScreenView::Construct(const FArguments& InArgs)
{
	Screen = InArgs._Screen;
	Web = InArgs._Web;
	Walker = InArgs._Walker;
	bUseLeaves = InArgs._bUseLeaves;
	OnLeft = InArgs._OnLeft;
	FVector Across;
	bWorld = Web.IsValid() && Walker.IsValid() && FSlateApplication::IsInitialized() && GEngine && GEngine->GameViewport && Web->CrtGlass(Screen, Centre, Up, Across, Size);
	if (bWorld)
	{
		// the glass faces the eye; its right is the eye's right looking at it
		const FVector Eye = Walker->EyeUE();
		Normal = FVector::CrossProduct(Across, Up).GetSafeNormal();
		if (FVector::DotProduct(Normal, Eye - Centre) < 0) Normal = -Normal;
		Right = FVector::CrossProduct(Normal, Up).GetSafeNormal();
		// close enough that the glass fills most of the view (the walker's vertical field of view)
		FVector2D VP(16, 9);
		GEngine->GameViewport->GetViewportSize(VP);
		const double TanV = FMath::Tan(FMath::DegreesToRadians(double(Walker->Fov) * 0.5)), Aspect = VP.X / FMath::Max(1.0, VP.Y);
		const double Dist = FMath::Max(Size.Y / Fill * 0.5 / TanV, Size.X / 0.94 * 0.5 / (TanV * Aspect));
		To.Eye = S47Three::ToThree(Centre + Normal * Dist);
		Aim(-Normal, To.Yaw, To.Pitch);
		Target.Reset(FWidgetRenderer::CreateTargetFor(FVector2D(PicW, PicH), TF_Bilinear, true));
		bWorld = Target.IsValid();
		if (bWorld)
		{
			// opaque: the screen's material takes the picture's colour whatever its alpha says
			Target->ClearColor = FLinearColor::Black;
			Target->UpdateResourceImmediate(true);
		}
	}
	if (bWorld)
	{
		// linear out: the target is sRGB and does the encoding itself (Slate's own gamma on top of it
		// lifted the dark phosphor to a grey green)
		Renderer = new FWidgetRenderer(false);
		Window = SNew(SVirtualWindow).Size(FVector2D(PicW, PicH));
		Window->SetIsFocusable(true);
		FSlateApplication::Get().RegisterVirtualWindow(Window.ToSharedRef());
		User = FSlateApplication::Get().FindOrCreateVirtualUser(VirtualSlot);
	}
}

SS47ScreenView::~SS47ScreenView()
{
	if (!bWorld) return;
	if (bStarted && !bLeft)
	{
		// closed from elsewhere (another view opened over it): the walker has its eyes back at once
		if (AS47Walker* W = Walker.Get()) W->CameraOverride = Before;
		if (AS47Web* Wb = Web.Get()) Wb->SetCrtPicture(Screen, nullptr);
	}
	if (FSlateApplication::IsInitialized() && Window.IsValid()) FSlateApplication::Get().UnregisterVirtualWindow(Window.ToSharedRef());
	User.Reset();
	if (Renderer) BeginCleanup(Renderer);
	Renderer = nullptr;
}

void SS47ScreenView::SetScreen(const TSharedRef<SWidget>& InContent, const TSharedPtr<SWidget>& InKeysTo)
{
	S47_USING_THEME
	Content = InContent;
	KeysTo = InKeysTo.IsValid() ? InKeysTo : TSharedPtr<SWidget>(InContent);
	// the way back, as the menus show their keys (.keys)
	const TSharedRef<SWidget> Hints = KeyHints({ { TEXT("ESC"), TEXT("BACK"), [this] { Leave(); } } });
	if (bWorld)
	{
		Window->SetContent(SNew(SS47CrtGlass)[InContent]);
		ChildSlot[SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, Px(26))[Hints]];
		return;
	}
	// .overlay { background: rgba(2,3,6,.72) } and the .crt in its bezel, 960 px wide
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Css(0x020306, 0.72f))]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(Px(960) + 2 * Px(16))[SNew(SS47CrtGlass).bBezel(true)[InContent]]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, Px(26))[Hints]
	];
}

void SS47ScreenView::Leave()
{
	if (bLeft) return;
	if (bWorld && bStarted)
	{
		bLeaving = true;
		return;
	}
	Finish();
}

void SS47ScreenView::Finish()
{
	if (bLeft) return;
	bLeft = true;
	if (bStarted)
	{
		if (AS47Walker* W = Walker.Get()) W->CameraOverride = Before;
		if (AS47Web* Wb = Web.Get()) Wb->SetCrtPicture(Screen, nullptr);
	}
	const TSharedRef<SWidget> Keep = AsShared();
	if (OnLeft) OnLeft();
}

SS47ScreenView::FEye SS47ScreenView::Now() const
{
	const double E = Ease(T);
	FEye V;
	V.Eye = FMath::Lerp(From.Eye, To.Eye, E);
	V.Yaw = From.Yaw + FMath::FindDeltaAngleRadians(From.Yaw, To.Yaw) * E;
	V.Pitch = FMath::Lerp(From.Pitch, To.Pitch, E);
	return V;
}

void SS47ScreenView::Tick(const FGeometry& G, const double InCurrentTime, const float Dt)
{
	SCompoundWidget::Tick(G, InCurrentTime, Dt);
	if (!bWorld || bLeft) return;
	AS47Walker* W = Walker.Get();
	AS47Web* Wb = Web.Get();
	if (!W || !Wb || !Content.IsValid()) { Finish(); return; }
	if (!bStarted)
	{
		// from where the walker's eye is now; the walker's camera is this view's until it is back
		bStarted = true;
		From.Eye = S47Three::ToThree(W->EyeUE());
		From.Yaw = W->Yaw;
		From.Pitch = W->Pitch;
		Before = W->CameraOverride;
		const TWeakPtr<SS47ScreenView> Self = StaticCastSharedRef<SS47ScreenView>(AsShared());
		W->CameraOverride = [Self](double, const FVector2D&, const FVector2D&, FS47View& Out)
		{
			const TSharedPtr<SS47ScreenView> V = Self.Pin();
			if (!V.IsValid()) return false;
			const FEye E = V->Now();
			Out.Eye = E.Eye;
			Out.Yaw = E.Yaw;
			Out.Pitch = E.Pitch;
			Out.Roll = 0;
			return true;
		};
		Wb->SetCrtPicture(Screen, Target.Get());
		UE_LOG(LogTemp, Display, TEXT("S47: screen %d in use on the glass (%.0f x %.0f cm)"), Screen, Size.X, Size.Y);
	}
	T = bLeaving ? FMath::Max(0.f, T - Dt / StepSecs) : FMath::Min(1.f, T + Dt / StepSecs);
	// opening the modal gave every user (this one too) the modal's focus: the keys go to the screen
	if (!KeysInside()) FocusInside();
	Renderer->DrawWindow(Target.Get(), Window->GetHittestGrid(), Window.ToSharedRef(), PicW / Units, FVector2D(PicW, PicH), Dt);
	if (bLeaving && T <= 0.f) Finish();
}

bool SS47ScreenView::KeysInside() const
{
	if (!User.IsValid() || !Window.IsValid() || !FSlateApplication::IsInitialized()) return false;
	for (TSharedPtr<SWidget> F = FSlateApplication::Get().GetUserFocusedWidget(User->GetUserIndex()); F.IsValid(); F = F->GetParentWidget())
	{
		if (F == Window) return true;
	}
	return false;
}

void SS47ScreenView::FocusInside()
{
	if (!User.IsValid() || !FSlateApplication::IsInitialized()) return;
	TSharedPtr<SWidget> K = KeysTo.Pin();
	if (!K.IsValid()) K = Content;
	if (!K.IsValid()) return;
	FSlateApplication::Get().SetUserFocus(User->GetUserIndex(), K, EFocusCause::SetDirectly);
	if (!bKeysLogged) UE_LOG(LogTemp, Display, TEXT("S47: screen %d keys to %s (%s)"), Screen, *K->GetTypeAsString(), KeysInside() ? TEXT("on the glass") : TEXT("NOT on the glass"));
	bKeysLogged = true;
}

FReply SS47ScreenView::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	// over the game (no glass): the content has the keys itself
	if (!bWorld)
	{
		const TSharedPtr<SWidget> K = KeysTo.Pin();
		if (K.IsValid()) return FReply::Handled().SetUserFocus(K.ToSharedRef(), EFocusCause::SetDirectly);
	}
	return FReply::Handled();
}

bool SS47ScreenView::ToPicture(const FGeometry& G, const FVector2D& ScreenPos, FVector2D& Px) const
{
	const AS47Walker* W = Walker.Get();
	APlayerController* PC = W && W->GetWorld() ? W->GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC || !GEngine || !GEngine->GameViewport) return false;
	FVector2D VP;
	GEngine->GameViewport->GetViewportSize(VP);
	const FVector2D L = G.AbsoluteToLocal(ScreenPos), S = G.GetLocalSize();
	if (S.X <= 0 || S.Y <= 0) return false;
	FVector O, D;
	if (!PC->DeprojectScreenPositionToWorld(L.X / S.X * VP.X, L.Y / S.Y * VP.Y, O, D)) return false;
	const double Den = FVector::DotProduct(D, Normal);
	if (Den > -1e-6) return false;
	const double K = FVector::DotProduct(Centre - O, Normal) / Den;
	if (K <= 0) return false;
	Px = Pixel(O + D * K, Centre, Right, Up, Size);
	return true;
}

FWidgetPath SS47ScreenView::PathAt(const FVector2D& Px) const
{
	TArray<FWidgetAndPointer> Hits = Window->GetHittestGrid().GetBubblePath(Px, 0.f, false, INDEX_NONE);
	const FVirtualPointerPosition V(Px, Last.X > -1 ? Last : Px);
	for (FWidgetAndPointer& H : Hits) H.SetPointerPosition(V);
	return FWidgetPath(Hits);
}

FReply SS47ScreenView::Route(const FGeometry& G, const FPointerEvent& E, int32 Kind)
{
	if (!bWorld || !bStarted || bLeft || !User.IsValid()) return FReply::Unhandled();
	FVector2D Px(-PicW, -PicH);
	const bool bRay = ToPicture(G, E.GetScreenSpacePosition(), Px);
	const bool bOn = bRay && !bLeaving && Px.X >= 0 && Px.Y >= 0 && Px.X < PicW && Px.Y < PicH;
	// a click off the glass steps back (.overlay: a click on the backdrop closes)
	if (Kind == RDown && !bOn && Held.IsEmpty())
	{
		Leave();
		return FReply::Handled();
	}
	if (Kind == RDown || Kind == RDouble) Held.Add(E.GetEffectingButton());
	else if (Kind == RUp) Held.Remove(E.GetEffectingButton());
	const bool bButton = Kind == RDown || Kind == RUp || Kind == RDouble;
	const FPointerEvent V(User->GetUserIndex(), 0, Px, Last.X > -1 ? Last : Px, Held, bButton ? E.GetEffectingButton() : FKey(), Kind == RWheel ? E.GetWheelDelta() : 0.f, E.GetModifierKeys());
	const FWidgetPath Path = bOn ? PathAt(Px) : FWidgetPath();
	Last = Px;
	FSlateApplication& App = FSlateApplication::Get();
	bForwarding = true;
	switch (Kind)
	{
	case RMove: App.RoutePointerMoveEvent(Path, V, false); break;
	case RDown: App.RoutePointerDownEvent(Path, V); break;
	case RUp: App.RoutePointerUpEvent(Path, V); break;
	case RDouble: App.RoutePointerDoubleClickEvent(Path, V); break;
	default: App.RouteMouseWheelOrGestureEvent(Path, V, nullptr); break;
	}
	bForwarding = false;
	// the real pointer stays with the screen while a button is held (a drag that leaves the window)
	if (Kind == RDown || Kind == RDouble) return FReply::Handled().CaptureMouse(SharedThis(this));
	if (Kind == RUp && Held.IsEmpty()) return FReply::Handled().ReleaseMouseCapture();
	return FReply::Handled();
}

FReply SS47ScreenView::OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E)
{
	if (!bWorld)
	{
		// over the game: a click beside the screen closes it (the screen keeps its own clicks)
		Leave();
		return FReply::Handled();
	}
	return Route(G, E, RDown);
}

FReply SS47ScreenView::OnMouseButtonUp(const FGeometry& G, const FPointerEvent& E) { return Route(G, E, RUp); }
FReply SS47ScreenView::OnMouseButtonDoubleClick(const FGeometry& G, const FPointerEvent& E) { return bWorld ? Route(G, E, RDouble) : FReply::Unhandled(); }
FReply SS47ScreenView::OnMouseMove(const FGeometry& G, const FPointerEvent& E) { return Route(G, E, RMove); }
FReply SS47ScreenView::OnMouseWheel(const FGeometry& G, const FPointerEvent& E) { return Route(G, E, RWheel); }

FCursorReply SS47ScreenView::OnCursorQuery(const FGeometry& G, const FPointerEvent& E) const
{
	if (!bWorld || !bStarted || bLeft || !Window.IsValid()) return FCursorReply::Unhandled();
	FVector2D Px;
	if (!ToPicture(G, E.GetScreenSpacePosition(), Px) || Px.X < 0 || Px.Y < 0 || Px.X >= PicW || Px.Y >= PicH) return FCursorReply::Cursor(EMouseCursor::Default);
	// what the widget under the pointer on the glass would show
	const FWidgetPath Path = PathAt(Px);
	const FPointerEvent V(User.IsValid() ? User->GetUserIndex() : 0, 0, Px, Px, TSet<FKey>(), FKey(), 0.f, E.GetModifierKeys());
	for (int32 i = Path.Widgets.Num() - 1; i >= 0; --i)
	{
		const FArrangedWidget& A = Path.Widgets[i];
		const FCursorReply R = A.Widget->OnCursorQuery(A.Geometry, V);
		if (R.IsEventHandled()) return R;
	}
	return FCursorReply::Cursor(EMouseCursor::Default);
}

FReply SS47ScreenView::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	if (bForwarding) return FReply::Unhandled();
	const FKey K = E.GetKey();
	// (use's other key, Enter, and Space press the key chosen on the screen)
	if (K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right || (bUseLeaves && !K.IsGamepadKey() && S47Keys::Is(TEXT("use"), K) && K != EKeys::Enter && K != EKeys::SpaceBar))
	{
		Leave();
		return FReply::Handled();
	}
	if (!bWorld) return FReply::Unhandled();
	if (bLeaving || !User.IsValid()) return FReply::Handled();
	if (!KeysInside()) FocusInside();
	bForwarding = true;
	FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(K, E.GetModifierKeys(), User->GetUserIndex(), E.IsRepeat(), E.GetCharacter(), E.GetKeyCode()));
	bForwarding = false;
	return FReply::Handled();
}

FReply SS47ScreenView::OnKeyUp(const FGeometry& G, const FKeyEvent& E)
{
	if (bForwarding || !bWorld || !User.IsValid()) return FReply::Unhandled();
	bForwarding = true;
	FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(E.GetKey(), E.GetModifierKeys(), User->GetUserIndex(), E.IsRepeat(), E.GetCharacter(), E.GetKeyCode()));
	bForwarding = false;
	return FReply::Handled();
}

FReply SS47ScreenView::OnKeyChar(const FGeometry& G, const FCharacterEvent& E)
{
	if (bForwarding || !bWorld || bLeaving || !User.IsValid()) return FReply::Unhandled();
	bForwarding = true;
	FSlateApplication::Get().ProcessKeyCharEvent(FCharacterEvent(E.GetCharacter(), E.GetModifierKeys(), User->GetUserIndex(), E.IsRepeat()));
	bForwarding = false;
	return FReply::Handled();
}

bool SS47ScreenView::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bPass, const FString& What) { Lines.Add(FString::Printf(TEXT("%s screen view: %s"), bPass ? TEXT("ok  ") : TEXT("FAIL"), *What)); bOk &= bPass; };
	// the walker's yaw and pitch from a direction turn its camera that way
	for (const FVector& Dir : { FVector(1, 0, 0), FVector(0, -1, 0), FVector(-0.3, 0.9, -0.2).GetSafeNormal(), FVector(0.2, 0.1, 0.97).GetSafeNormal() })
	{
		double Yaw = 0, Pitch = 0;
		Aim(Dir, Yaw, Pitch);
		const FVector Got = S47Three::CameraRotation(Yaw, Pitch).Vector();
		Check(FVector::DotProduct(Got, Dir) > 0.9999, FString::Printf(TEXT("aim at (%.2f, %.2f, %.2f) looks along (%.2f, %.2f, %.2f)"), Dir.X, Dir.Y, Dir.Z, Got.X, Got.Y, Got.Z));
	}
	// the glass's centre is the picture's middle, its top right corner the picture's (PicW, 0)
	const FVector C(100, 200, 90), R(0, -1, 0), U(0, 0, 1);
	const FVector2D S(37, 28);
	const FVector2D Mid = Pixel(C, C, R, U, S), Corner = Pixel(C + R * S.X / 2 + U * S.Y / 2, C, R, U, S);
	Check(Mid.Equals(FVector2D(PicW / 2, PicH / 2), 0.01) && Corner.Equals(FVector2D(PicW, 0), 0.01), FString::Printf(TEXT("glass to pixels: middle %s, top right %s"), *Mid.ToString(), *Corner.ToString()));
	int32 Left = 0;
	const TSharedRef<SS47ScreenView> Fallback = SNew(SS47ScreenView).OnLeft([&Left] { ++Left; });
	const auto Key = [](const FKey& K) { return FKeyEvent(K, FModifierKeysState(), 0, false, 0, 0); };
	Check(!Fallback->OnKeyDown(FGeometry(), Key(EKeys::Gamepad_FaceButton_Bottom)).IsEventHandled() && Left == 0, TEXT("A stays available to fallback screen content, never use-to-leave"));
	Fallback->OnKeyDown(FGeometry(), Key(EKeys::Gamepad_FaceButton_Right));
	Check(Fallback->OnKeyDown(FGeometry(), Key(EKeys::Gamepad_FaceButton_Right)).IsEventHandled() && Left == 1, TEXT("B leaves a fallback screen exactly once"));
	bOk &= SS47Document::SelfTest(Lines);
	bOk &= SS47Ops::SelfTest(Lines);
	bOk &= S47WidgetInput::SelfTest(Lines);
	Check(Ease(0.f) == 0.f && Ease(1.f) == 1.f && FMath::IsNearlyEqual(Ease(0.5f), 0.5f), TEXT("the steps ease in and out"));
	Check(S47Theme::GrabCursor(false) != EMouseCursor::None && S47Theme::GrabCursor(true) != EMouseCursor::None, TEXT("the case board's grab cursor is one that shows"));
	return bOk;
}

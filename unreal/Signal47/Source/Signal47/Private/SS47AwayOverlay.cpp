// The time away's light and frames over the game (story/abduction.ts's CSS and canvas)
#include "SS47AwayOverlay.h"

#include "Brushes/SlateColorBrush.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"
#include "S47Image.h"

void SS47AwayOverlay::Construct(const FArguments& InArgs)
{
	(void)InArgs;
	SetVisibility(EVisibility::HitTestInvisible);
	SetClipping(EWidgetClipping::ClipToBounds);
	Black = MakeUnique<FSlateColorBrush>(FLinearColor::White);
	// radial-gradient(ellipse at 50% -10%, #fff 0%, #eef4f6 45%, rgba(220,232,236,.85) 100%): the
	// ellipse through the farthest corner, in the box's own proportions (so a square picture of it,
	// stretched over the view, is the same on any screen)
	const int32 N = 128;
	TArray<FColor> Px;
	Px.SetNumUninitialized(N * N);
	auto Mix = [](double A, double B, double K) { return A + (B - A) * K; };
	for (int32 Y = 0; Y < N; ++Y) for (int32 X = 0; X < N; ++X)
	{
		const double U = (X + 0.5) / N, V = (Y + 0.5) / N;
		const double T = FMath::Sqrt(FMath::Square((U - 0.5) / (0.5 * UE_DOUBLE_SQRT_2)) + FMath::Square((V + 0.1) / (1.1 * UE_DOUBLE_SQRT_2)));
		double R, G, B, A;
		if (T <= 0.45) { const double K = T / 0.45; R = Mix(255, 238, K); G = Mix(255, 244, K); B = Mix(255, 246, K); A = 1; }
		else { const double K = FMath::Min(1.0, (T - 0.45) / 0.55); R = Mix(238, 220, K); G = Mix(244, 232, K); B = Mix(246, 236, K); A = Mix(1, 0.85, K); }
		Px[Y * N + X] = FColor(uint8(FMath::RoundToInt(R)), uint8(FMath::RoundToInt(G)), uint8(FMath::RoundToInt(B)), uint8(FMath::RoundToInt(A * 255)));
	}
	LightTex.Reset(S47Image::FromPixels(Px, N, N));
	LightBrush.SetResourceObject(LightTex.Get());
	LightBrush.ImageSize = FVector2D(N, N);
	LightBrush.DrawAs = ESlateBrushDrawType::Image;
}

void SS47AwayOverlay::ShowFrame(UTexture2D* T, double InAlpha, bool bFadeBlack)
{
	bFrame = true;
	Alpha = InAlpha; bFrameFadeBlack = bFadeBlack;
	if (T != Frame.Get())
	{
		Frame.Reset(T);
		FrameBrush.SetResourceObject(T);
		FrameBrush.ImageSize = FVector2D(640, 400);
		FrameBrush.DrawAs = ESlateBrushDrawType::Image;
	}
}

int32 SS47AwayOverlay::OnPaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	(void)Args; (void)Cull; (void)Style; (void)bParentEnabled;
	const FVector2f S = FVector2f(Geo.GetLocalSize());
	// the light from above (z-index 39)
	if (Light && *Light > 0 && LightTex.IsValid())
	{
		FSlateDrawElement::MakeBox(Out, Layer, Geo.ToPaintGeometry(), &LightBrush, ESlateDrawEffect::None, FLinearColor(1, 1, 1, float(FMath::Clamp(*Light, 0.0, 1.0))));
	}
	// the frame canvas (z-index 40): black, and the picture over it, covering the view
	if (bFrame)
	{
		FSlateDrawElement::MakeBox(Out, Layer + 1, Geo.ToPaintGeometry(), Black.Get(), ESlateDrawEffect::None, FLinearColor(0, 0, 0, bFrameFadeBlack ? 0.f : 1.f));
		if (Frame.IsValid() && Alpha > 0)
		{
			const float K = FMath::Max(S.X / 640.f, S.Y / 400.f), W = 640.f * K, H = 400.f * K;
			FSlateDrawElement::MakeBox(Out, Layer + 2, Geo.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f((S.X - W) / 2, (S.Y - H) / 2))), &FrameBrush, ESlateDrawEffect::None, FLinearColor(1, 1, 1, float(FMath::Clamp(Alpha, 0.0, 1.0))));
		}
	}
	return Layer + 2;
}

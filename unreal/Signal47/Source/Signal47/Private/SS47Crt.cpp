#include "SS47Crt.h"

#include "Brushes/SlateColorBrush.h"
#include "Rendering/DrawElements.h"
#include "S47Web.h"
#include "S47Story.h"
#include "SS47Widgets.h"
#include "S47Image.h"
#include "S47Kit.h"
#include "S47Theme.h"
#include "S47Main.h"
#include "S47Freeform.h"

namespace
{
	// the canvas's colours ('#8cffa4', '#ffd27a', '#c8ffd2', 'rgba(140,255,164,.7)'): sRGB, as Slate's
	// linear colours (they were the sRGB numbers taken as linear, which washed the screens out)
	FLinearColor CrtCss(uint32 Rgb, float A = 1.f) { return S47Theme::Css(Rgb, A); }
	const FLinearColor Phosphor = CrtCss(0x8cffa4);
	const FLinearColor Warn = CrtCss(0xffd27a);
	const FLinearColor Bright = CrtCss(0xc8ffd2);
	const FLinearColor Dim = CrtCss(0x8cffa4, 0.7f);
}

void SS47Crt::Construct(const FArguments& InArgs)
{
	Kind = InArgs._Kind;
	Story = InArgs._Story;
	Saro = InArgs._Saro;
	WhiteBrush = MakeUnique<FSlateColorBrush>(FLinearColor::White);
}

void SS47Crt::Txt(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer, const FString& S, float X, float Y, float Size, const FLinearColor& Color) const
{
	// Canvas text sits on its baseline; Slate's on its top: shift up by about the ascent.
	FSlateDrawElement::MakeText(Out, Layer, G.ToPaintGeometry(FVector2f(520.f, Size + 8.f), FSlateLayoutTransform(FVector2f(X, Y - Size * 0.8f))), S, S47Style::Mono(Size), ESlateDrawEffect::None, Color);
}

void SS47Crt::Clear(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer) const
{
	FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(), WhiteBrush.Get(), ESlateDrawEffect::None, CrtCss(0x021006));
}

void SS47Crt::Scanlines(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer) const
{
	for (float Y = 0; Y < 384.f; Y += 3.f)
	{
		FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(512.f, 1.f), FSlateLayoutTransform(FVector2f(0, Y))), WhiteBrush.Get(), ESlateDrawEffect::None, FLinearColor(0, 0, 0, 0.22f));
	}
	// createRadialGradient(256, 192, 120, 256, 192, 330): clear inside 120 px, rgba(0,0,0,.55) at 330
	static TSharedPtr<FSlateBrush> Vig;
	if (!Vig.IsValid())
	{
		const int32 W = 128, H = 96;
		TArray<FColor> Px;
		Px.SetNumUninitialized(W * H);
		for (int32 y = 0; y < H; ++y)
		{
			for (int32 x = 0; x < W; ++x)
			{
				const float D = FMath::Sqrt(FMath::Square((x + 0.5f) * 4.f - 256.f) + FMath::Square((y + 0.5f) * 4.f - 192.f));
				Px[y * W + x] = FColor(0, 0, 0, uint8(FMath::RoundToInt32(0.55f * FMath::Clamp((D - 120.f) / 210.f, 0.f, 1.f) * 255.f)));
			}
		}
		if (UTexture2D* T = S47Image::FromPixels(Px, W, H)) Vig = S47Kit::BrushFor(T);
	}
	if (Vig.IsValid()) FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(512.f, 384.f), FSlateLayoutTransform()), Vig.Get(), ESlateDrawEffect::None, FLinearColor::White);
}

void SS47Crt::Boot(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer, double E) const
{
	Clear(G, Out, Layer);
	static const TCHAR* Lines[] = { TEXT("SARO RX/DSP  REV 2.3"), TEXT("(C) 1983 SOUTHWEST ASTRONOMICAL"), TEXT("MEMORY TEST ...... 512K OK"), TEXT("RX BANK 3 ........ ONLINE"), TEXT("ARRAY LINK ....... 15/15"), TEXT("LOADING SCHEDULE") };
	const int32 N = FMath::Min(6, int32(E * 3));
	for (int32 i = 0; i < N; ++i) Txt(G, Out, Layer + 1, Lines[i], 22, 50 + i * 34.f);
	if (int32(E * 4) % 2) Txt(G, Out, Layer + 1, TEXT("_"), 22, 50 + N * 34.f);
}

void SS47Crt::Status(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer) const
{
	Clear(G, Out, Layer);
	const bool bEv = Story->At(ES47Phase::Event);
	// (the array put back on the schedule, story/arrayBack.ts: its mode and its line while it runs)
	const TOptional<FS47ArrayStatus> Arr = bEv && Story->ArrayStatus ? Story->ArrayStatus() : TOptional<FS47ArrayStatus>();
	Txt(G, Out, Layer + 1, TEXT("SARO  ARRAY STATUS"), 18, 34, 28);
	Txt(G, Out, Layer + 1, Story->ClockText(), 384, 34, 26);
	Txt(G, Out, Layer + 1, TEXT("RX BANK 3   ONLINE"), 18, 68, 22);
	Txt(G, Out, Layer + 1, TEXT("MODE        ") + (Arr ? Arr->Mode : Story->SurveyModeText()), 18, 92, 22, bEv && (!Arr || Arr->bWarn) ? Warn : Phosphor);
	Txt(G, Out, Layer + 1, TEXT("SCHEDULE    AZ 042  EL 48  UNTIL 06:00"), 18, 116, 22);
	FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(FVector2f(476.f, 1.f), FSlateLayoutTransform(FVector2f(18, 128))), WhiteBrush.Get(), ESlateDrawEffect::None, CrtCss(0x8cffa4, 0.35f));
	// all 27, three columns of nine
	for (int32 i = 0; i < Saro->DishCount(); ++i)
	{
		const int32 Col = i / 9, Row = i % 9;
		// (d.moving: turning by itself, or slewing; amber while it is rogue or going back by itself)
		const bool bMv = Saro->DishMoving(i) || (Story->DishSpinning && Story->DishSpinning(i));
		const bool bOdd = Story->DishRogue && Story->DishRogue(i);
		Txt(G, Out, Layer + 1, FString::Printf(TEXT("%s %s %s%s"), *Saro->DishId(i), *S47Text::AngleText(Saro->DishAz(i), 3), *S47Text::AngleText(Saro->DishEl(i)), bMv ? TEXT(">") : TEXT(" ")), 18 + Col * 164.f, 152 + Row * 23.f, 20, (bOdd || (bMv && !Arr)) && bEv ? Warn : Phosphor);
	}
	// (the array on the schedule: its count, steady; going back with no command, blinking)
	if (Arr && !Arr->Line.IsEmpty() && (!Arr->bWarn || int32(Story->GameTime * 2) % 2 == 0)) Txt(G, Out, Layer + 1, Arr->Line, 18, 372, 24, Arr->bWarn ? Warn : Bright);
	else if (!Arr && bEv && int32(Story->GameTime * 2) % 2 == 0) Txt(G, Out, Layer + 1, TEXT("NO CONTROL COMMAND RECORDED"), 18, 372, 24, Warn);
}

void SS47Crt::Spectrum(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer) const
{
	SS47Spectrum::Paint(Story, G, Out, Layer, WhiteBrush.Get());
	const double T = FPlatformTime::Seconds();
	Txt(G, Out, Layer + 6, TEXT("SPECTRUM  1419.5 - 1420.7 MHz"), 14, 28, 24);
	Txt(G, Out, Layer + 6, FString::Printf(TEXT("%.3f MHz"), Story->Rx.Frequency), 360, 28, 24, Warn);
	const bool bWarn = Story->Rx.Status.StartsWith(TEXT("UNLOGGED"));
	if (!bWarn || int32(T * 2.5) % 2 == 0) Txt(G, Out, Layer + 6, Story->Rx.Status.Left(40), 14, 370, 22, bWarn ? Warn : CrtCss(0xb9ffc7));
}

void SS47Crt::Tracking(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer) const
{
	Clear(G, Out, Layer);
	const float Cx = 150, Cy = 200, R = 128;
	auto Circle = [&](float Rad, const FLinearColor& C, float W)
	{
		TArray<FVector2f> Pts;
		for (int32 i = 0; i <= 48; ++i) { const float A = i / 48.f * 2 * PI; Pts.Add(FVector2f(Cx + FMath::Cos(A) * Rad, Cy + FMath::Sin(A) * Rad)); }
		FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), Pts, ESlateDrawEffect::None, C, true, W);
	};
	for (float K : { 1.f, 0.66f, 0.33f }) Circle(R * K, CrtCss(0x8cffa4, 0.5f), 1.5f);
	FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(Cx - R, Cy), FVector2f(Cx + R, Cy) }, ESlateDrawEffect::None, CrtCss(0x8cffa4, 0.5f), true, 1.5f);
	FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(Cx, Cy - R), FVector2f(Cx, Cy + R) }, ESlateDrawEffect::None, CrtCss(0x8cffa4, 0.5f), true, 1.5f);
	const double Az = Saro->DishAz(0), El = Saro->DishEl(0);
	const float A = float(FMath::DegreesToRadians(Az)), Rr = R * float(1.0 - El / 90.0);
	FSlateDrawElement::MakeLines(Out, Layer + 2, G.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(Cx, Cy), FVector2f(Cx + FMath::Sin(A) * R, Cy - FMath::Cos(A) * R) }, ESlateDrawEffect::None, Phosphor, true, 2.f);
	FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(FVector2f(10.f, 10.f), FSlateLayoutTransform(FVector2f(Cx + FMath::Sin(A) * Rr - 5.f, Cy - FMath::Cos(A) * Rr - 5.f))), WhiteBrush.Get(), ESlateDrawEffect::None, Bright);
	Txt(G, Out, Layer + 2, TEXT("N"), Cx - 5, Cy - R - 8, 20);
	Txt(G, Out, Layer + 2, TEXT("TRACKING"), 300, 40, 28);
	Txt(G, Out, Layer + 2, TEXT("AZ ") + S47Text::AngleText(Az, 3), 300, 80, 30);
	Txt(G, Out, Layer + 2, TEXT("EL ") + S47Text::AngleText(El), 300, 112, 30);
	const float X = 300;
	const double T = FPlatformTime::Seconds();
	if (Story->Phase() == ES47Phase::Solving)
	{
		const float P = FMath::Min(1.f, float((Story->GameTime - Story->SolveT) / 4.2));
		Txt(G, Out, Layer + 2, TEXT("DIRECTION SOLVE"), X, 160, 22);
		FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(FVector2f(190.f, 16.f), FSlateLayoutTransform(FVector2f(X, 172))), WhiteBrush.Get(), ESlateDrawEffect::None, CrtCss(0x8cffa4, 0.25f));
		FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(FVector2f(186.f * P, 12.f), FSlateLayoutTransform(FVector2f(X + 2, 174))), WhiteBrush.Get(), ESlateDrawEffect::None, Phosphor);
		for (int32 i = 0; i < 5; ++i) Txt(G, Out, Layer + 2, FString::Printf(TEXT("ITER %s  %.3f"), *S47Text::Pad(FMath::FloorToDouble(P * 40) + i, 3), FMath::FRand() * 9.0), X, 216 + i * 24.f, 20, Dim);
	}
	else if (Story->Rx.bSolved)
	{
		Txt(G, Out, Layer + 2, TEXT("RA  05h 17m 32s"), X, 160, 22);
		Txt(G, Out, Layer + 2, TEXT("DEC -05  23' 14\""), X, 184, 22);
		if (Story->At(ES47Phase::Countdown))
		{
			const double E = Story->At(ES47Phase::Event) ? 47 : FMath::Min(47.0, Story->GameTime - Story->LineDeadT);
			Txt(G, Out, Layer + 2, TEXT("LINE DROP"), X, 232, 22, Warn);
			Txt(G, Out, Layer + 2, TEXT("T+ 00:00:") + S47Text::Pad(E), X, 264, 30, Warn);
		}
		else Txt(G, Out, Layer + 2, TEXT("DIST  -39 LY  ??"), X, 232, 22, Warn);
	}
	else if (Story->Rx.Stage >= 3)
	{
		if (int32(T * 2) % 2 == 0) Txt(G, Out, Layer + 2, TEXT("SOURCE LOCKED"), X, 160, 22, Warn);
		Txt(G, Out, Layer + 2, TEXT("SOLVE: READY"), X, 188, 22);
	}
	else Txt(G, Out, Layer + 2, Story->Rx.Stage >= 2 ? TEXT("SURVEY TRACK") : TEXT("PARKED"), X, 160, 22);
	if (Story->At(ES47Phase::Event)) Txt(G, Out, Layer + 2, TEXT("NO COMMAND"), X, 330, 26, Warn);
}

void SS47Crt::Ops(const FGeometry& G, FSlateWindowElementList& Out, int32 Layer) const
{
	Clear(G, Out, Layer);
	TArray<FString> Head;
	Story->Ops().Header().ParseIntoArray(Head, TEXT("\n"), false);
	const double T = FPlatformTime::Seconds();
	if (Head.Num() > 0) Txt(G, Out, Layer + 1, Head[0].Replace(TEXT("        "), TEXT(" ")).Replace(TEXT("   "), TEXT(" ")), 14, 30, 22);
	if (Head.Num() > 1) Txt(G, Out, Layer + 1, Head[1], 14, 56, 22);
	const TArray<FString>& Lines = Story->OpsLines;
	const int32 First = FMath::Max(0, Lines.Num() - 10);
	int32 Row = 0;
	for (int32 i = First; i < Lines.Num(); ++i, ++Row) Txt(G, Out, Layer + 1, Lines[i].Left(40), 14, 92 + Row * 26.f, 22);
	Txt(G, Out, Layer + 1, FString(TEXT("$ ")) + (int32(T * 2) % 2 ? TEXT("_") : TEXT("")), 14, 92 + Row * 26.f, 22);
}

int32 SS47Crt::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	if (!Story || !Saro)
	{
		FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(), WhiteBrush.Get(), ESlateDrawEffect::None, CrtCss(0x020a04));
		return Layer + 1;
	}
	if(const auto* M=US47Main::Get(Story);M&&M->IsFreeform()&&M->Freeform)
 {
  FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(),WhiteBrush.Get(),ESlateDrawEffect::None,CrtCss(0x020a04));
  const auto Lines=M->Freeform->TerminalLines(int32(Kind));
  for(int32 I=0;I<Lines.Num();++I)Txt(G,Out,Layer+1,Lines[I],16,38+I*40,I==0?25:20,I==0?Bright:Phosphor);
  Scanlines(G,Out,Layer+8);return Layer+9;
 }
	if (Kind == EKind::Ops)
	{
		Ops(G, Out, Layer);
		Scanlines(G, Out, Layer + 8);
		return Layer + 9;
	}
	const bool bGlitch = Story->CrtGlitch > 0.f && FMath::FRand() < 0.6f;
	// The phosphor's local light follows the very same black glitch frame as its texture.
	Saro->SetCrtFlicker(Kind == EKind::Status ? 0 : Kind == EKind::Spectrum ? 1 : 2, bGlitch ? 0.f : 1.f);
	const bool bBooting = Story->BootT >= 0 && Story->GameTime - Story->BootT < 2.6;
	if (bGlitch)
	{
		FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(), WhiteBrush.Get(), ESlateDrawEffect::None, FLinearColor::Black);
	}
	else if (bBooting) Boot(G, Out, Layer, Story->GameTime - Story->BootT);
	else
	{
		if (Kind == EKind::Status) Status(G, Out, Layer);
		else if (Kind == EKind::Spectrum) Spectrum(G, Out, Layer);
		else Tracking(G, Out, Layer);
		if (Story->At(ES47Phase::End) && int32(FPlatformTime::Seconds() * 2) % 2 == 0)
		{
			FSlateDrawElement::MakeBox(Out, Layer + 7, G.ToPaintGeometry(FVector2f(512.f, 84.f), FSlateLayoutTransform(FVector2f(0, 150))), WhiteBrush.Get(), ESlateDrawEffect::None, CrtCss(0x021006, 0.88f));
			FSlateDrawElement::MakeLines(Out, Layer + 7, G.ToPaintGeometry(), TArray<FVector2f>{ FVector2f(14, 158), FVector2f(498, 158), FVector2f(498, 226), FVector2f(14, 226), FVector2f(14, 158) }, ESlateDrawEffect::None, Phosphor, true, 2.f);
			Txt(G, Out, Layer + 7, TEXT("SIGNAL ACQUIRED"), 92, 208, 52, Bright);
		}
	}
	Scanlines(G, Out, Layer + 8);
	return Layer + 9;
}

// story/drawings.ts (the drawings chapter one and the chapters after it use from it)
#include "S47Drawings.h"

#include "S47Canvas.h"

namespace
{
	// fonts as the drawings name them: '600 26px Oswald' (only Oswald's regular weight is in
	// Content/Fonts, so the 600 is drawn as 400), '19px "Special Elite"'
	const TCHAR* OSWALD = TEXT("Oswald-Regular.ttf");
	const TCHAR* ELITE = TEXT("SpecialElite-Regular.ttf");
}

void S47Drawings::PaperBg(FS47Canvas& G, int32 W, int32 H)
{
	// paperBg
	G.SetFill(PAPER); G.FillRect(0, 0, W, H);
	uint32 S = uint32(W * 31 + H);
	auto R = [&S] { S = S * 1664525u + 1013904223u; return S / 4294967296.0; };
	for (int32 i = 0; i < 900; ++i)
	{
		G.SetFill(0x5a4628, R() * 0.05); // rgba(90,70,40,r*0.05)
		const double X = R() * W, Y = R() * H, Rw = 1 + R() * 2, Rh = 1 + R() * 2;
		G.FillRect(X, Y, Rw, Rh);
	}
}

FString S47Drawings::Sheet(int32 W, int32 H, TFunctionRef<void(FS47Canvas&)> Draw)
{
	// sheet
	FS47Canvas G(W, H);
	PaperBg(G, W, H);
	G.SetStroke(INK); G.SetFill(INK); G.LineWidth = 3; // lineCap and lineJoin round (FS47Canvas strokes so)
	Draw(G);
	return G.ToDataUrl(false);
}

void S47Drawings::Mark(FS47Canvas& G, double Cx, double Cy, double Size, bool bBar)
{
	// mark
	G.BeginPath();
	G.MoveTo(Cx, Cy - Size * 0.58); G.LineTo(Cx - Size * 0.55, Cy + Size * 0.4); G.LineTo(Cx + Size * 0.55, Cy + Size * 0.4); G.ClosePath();
	G.Stroke();
	if (bBar) { G.BeginPath(); G.MoveTo(Cx - Size * 0.4, Cy + Size * 0.62); G.LineTo(Cx + Size * 0.4, Cy + Size * 0.62); G.Stroke(); }
}

FString S47Drawings::Arrangement(bool bFull)
{
	// arrangement(full)
	return Sheet(960, 420, [bFull](FS47Canvas& G)
	{
		G.SetFont(OSWALD, 26);
		G.FillText(bFull ? TEXT("ARRANGEMENT / STATION 01 / 1947") : TEXT("ARRANGEMENT / STATION 01 / 1947 / SERVICE COPY"), 40, 56);
		G.SetFont(ELITE, 19);
		G.FillText(bFull ? TEXT("A: fixed survey point     B: comparison vane     C: closing sight line") : TEXT("A: fixed survey point     B: comparison vane"), 40, 92);
		G.GlobalAlpha = 0.5; G.LineWidth = 1.5;
		G.BeginPath(); G.MoveTo(40, 112); G.LineTo(920, 112); G.Stroke();
		G.GlobalAlpha = 1; G.LineWidth = 3;
		Mark(G, 150, 250, 64);
		G.BeginPath(); G.MoveTo(196, 250); G.LineTo(470, 250); G.Stroke();      // A to B
		G.BeginPath(); G.MoveTo(500, 218); G.LineTo(500, 282); G.Stroke();      // B, the vane
		if (bFull)
		{
			G.BeginPath(); G.MoveTo(506, 250); G.LineTo(790, 250); G.Stroke();    // the closing sight line
			G.BeginPath(); G.Arc(820, 250, 28, 0, PI * 2); G.Stroke();           // C
		}
		G.SetFont(ELITE, 28);
		G.FillText(TEXT("A"), 140, 340); G.FillText(TEXT("B"), 490, 340); if (bFull) G.FillText(TEXT("C"), 810, 340);
		G.SetFont(ELITE, 17);
		G.FillText(TEXT("N. VEGA"), 790, 396);
	});
}

FString S47Drawings::SleeveMark()
{
	// sleeveMark
	return Sheet(420, 420, [](FS47Canvas& G)
	{
		G.SetFont(OSWALD, 24);
		G.FillText(TEXT("FIXED-POINT SHEET"), 34, 54);
		G.SetFont(ELITE, 18);
		G.FillText(TEXT("INDEX DIAGRAM"), 34, 84);
		G.LineWidth = 4;
		Mark(G, 210, 220, 120);
		G.SetFont(ELITE, 24);
		G.FillText(TEXT("A  -  B  -  C"), 132, 340);
		G.SetFont(ELITE, 18);
		G.FillText(TEXT("STATION 01 / B-12"), 128, 376);
	});
}

FString S47Drawings::MarkIcon(const FString& Kind)
{
	// markIcon(kind)
	return Sheet(120, 120, [&Kind](FS47Canvas& G)
	{
		G.LineWidth = 4;
		if (Kind == TEXT("three-bars")) { for (int32 i = 0; i < 3; ++i) { G.BeginPath(); G.MoveTo(26, 38 + i * 22); G.LineTo(94, 38 + i * 22); G.Stroke(); } }
		else Mark(G, 60, 58, 70, Kind == TEXT("triangle-bar"));
	});
}

bool S47Drawings::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s drawings: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// the paper's speckle runs the same LCG as paperBg (values from node, a 120 x 120 sheet)
	{
		uint32 S = 120 * 31 + 120;
		auto R = [&S] { S = S * 1664525u + 1013904223u; return S / 4294967296.0; };
		const double Want[5] = { 0.724269293, 0.581025135, 0.099299545, 0.810711992, 0.614507199 };
		bool bSame = true;
		for (double V : Want) bSame &= FMath::Abs(R() - V) < 1e-8;
		Check(bSame, TEXT("paper speckle as paperBg"));
	}
	// Uint8ClampedArray stores (node: 0.5, 1.5, 2.5, -3, 254.5, 300 -> 0, 2, 2, 0, 254, 255)
	Check(FS47Canvas::Clamp8(0.5) == 0 && FS47Canvas::Clamp8(1.5) == 2 && FS47Canvas::Clamp8(2.5) == 2 && FS47Canvas::Clamp8(-3) == 0 && FS47Canvas::Clamp8(254.5) == 254 && FS47Canvas::Clamp8(300) == 255, TEXT("clamped bytes round half to even"));
	// a stroke: the mark's bar is ink in the middle and paper well off it
	{
		FS47Canvas G(120, 120);
		G.SetFill(PAPER); G.FillRect(0, 0, 120, 120);
		G.SetStroke(INK); G.LineWidth = 4;
		Mark(G, 60, 58, 70, true);
		const FColor On = G.Px[int32(58 + 70 * 0.62) * 120 + 60], Off = G.Px[5 * 120 + 5];
		Check(On.R == 0x22 && On.G == 0x37 && On.B == 0x2e && Off.R == 0xef && Off.G == 0xe8 && Off.B == 0xd6, FString::Printf(TEXT("the mark's bar in ink (%d %d %d), the paper clean (%d %d %d)"), On.R, On.G, On.B, Off.R, Off.G, Off.B));
	}
	// a half-covered pixel of a rectangle blends half way
	{
		FS47Canvas G(4, 4);
		G.SetFill(0x000000); G.FillRect(0, 0, 4, 4);
		G.SetFill(0xffffff); G.FillRect(1.5, 0, 2, 4);
		Check(G.Px[1].R == 128 && G.Px[2].R == 255 && G.Px[3].R == 128 && G.Px[0].R == 0, FString::Printf(TEXT("rectangle coverage (%d %d %d %d)"), G.Px[0].R, G.Px[1].R, G.Px[2].R, G.Px[3].R));
	}
	return bOk;
}

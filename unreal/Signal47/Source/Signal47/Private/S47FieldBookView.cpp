// ui/fieldBookView.ts
#include "S47FieldBookView.h"

#include "Brushes/SlateColorBrush.h"
#include "Engine/Texture2D.h"
#include "S47Audio.h"
#include "S47Canvas.h"
#include "S47Image.h"
#include "S47Kit.h"
#include "S47UI.h"
#include "SS47Widgets.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

using namespace S47FieldBookView;

namespace
{
	// ---------- a canvas's 2D context, the parts the envelope's pictures are drawn with ----------
	// (FS47Canvas has the pixels, the paths and the text; this adds what fieldBookView.ts asks for
	// besides: save and restore with a transform, gradients, curves and ellipses, strokeRect, and the
	// blur filter. Colours are CSS strings as the TypeScript writes them.)

	struct FRgba { double R = 0, G = 0, B = 0, A = 1; }; // 0..255, alpha 0..1

	/** '#rrggbb', 'rgb(r,g,b)' or 'rgba(r,g,b,a)' (fractional channels rounded, as a browser parses them) */
	FRgba Css(const FString& S)
	{
		FRgba C;
		if (S.StartsWith(TEXT("#")) && S.Len() == 7)
		{
			const uint32 V = FParse::HexNumber(*S.Mid(1));
			C.R = (V >> 16) & 255; C.G = (V >> 8) & 255; C.B = V & 255; C.A = 1;
			return C;
		}
		const int32 Open = S.Find(TEXT("(")), Close = S.Find(TEXT(")"));
		if (Open < 0 || Close < Open) return C;
		TArray<FString> P;
		S.Mid(Open + 1, Close - Open - 1).ParseIntoArray(P, TEXT(","));
		auto Ch = [](const FString& V) { return FMath::Clamp(double(FMath::RoundToInt(FCString::Atod(*V.TrimStartAndEnd()))), 0.0, 255.0); };
		if (P.Num() >= 3) { C.R = Ch(P[0]); C.G = Ch(P[1]); C.B = Ch(P[2]); }
		C.A = P.Num() >= 4 ? FMath::Clamp(FCString::Atod(*P[3].TrimStartAndEnd()), 0.0, 1.0) : 1.0;
		return C;
	}

	struct FAff
	{
		// x' = A x + C y + E, y' = B x + D y + F (a canvas's setTransform(a, b, c, d, e, f))
		double A = 1, B = 0, C = 0, D = 1, E = 0, F = 0;
		FVector2D Apply(double X, double Y) const { return FVector2D(A * X + C * Y + E, B * X + D * Y + F); }
		FAff Times(const FAff& O) const
		{
			FAff R;
			R.A = A * O.A + C * O.B; R.B = B * O.A + D * O.B;
			R.C = A * O.C + C * O.D; R.D = B * O.C + D * O.D;
			R.E = A * O.E + C * O.F + E; R.F = B * O.E + D * O.F + F;
			return R;
		}
		FAff Inverse() const
		{
			const double Det = A * D - B * C;
			FAff R;
			R.A = D / Det; R.B = -B / Det; R.C = -C / Det; R.D = A / Det;
			R.E = -(R.A * E + R.C * F); R.F = -(R.B * E + R.D * F);
			return R;
		}
		bool Plain() const { return A == 1 && B == 0 && C == 0 && D == 1; }
	};

	struct FGrad
	{
		bool bRadial = false;
		double X0 = 0, Y0 = 0, R0 = 0, X1 = 0, Y1 = 0, R1 = 0;
		TArray<TPair<double, FRgba>> Stops;
		void Add(double At, const TCHAR* Col) { Stops.Add({ At, Css(Col) }); }
		/** the colour at a point of user space; unset where a radial gradient paints nothing */
		TOptional<FRgba> At(double X, double Y) const
		{
			double T;
			if (!bRadial)
			{
				const double Dx = X1 - X0, Dy = Y1 - Y0, L2 = Dx * Dx + Dy * Dy;
				T = L2 > 0 ? ((X - X0) * Dx + (Y - Y0) * Dy) / L2 : 0;
			}
			else
			{
				// the two circles: the largest w with |p - c(w)| = r(w) and r(w) >= 0
				const double Cdx = X1 - X0, Cdy = Y1 - Y0, Dr = R1 - R0, Pdx = X - X0, Pdy = Y - Y0;
				const double Qa = Cdx * Cdx + Cdy * Cdy - Dr * Dr, Qb = Pdx * Cdx + Pdy * Cdy + R0 * Dr, Qc = Pdx * Pdx + Pdy * Pdy - R0 * R0;
				TOptional<double> W;
				if (FMath::Abs(Qa) < 1e-12) { if (FMath::Abs(Qb) > 1e-12) { const double Wv = Qc / (2 * Qb); if (R0 + Wv * Dr >= 0) W = Wv; } }
				else
				{
					const double Disc = Qb * Qb - Qa * Qc;
					if (Disc < 0) return {};
					const double S = FMath::Sqrt(Disc), W1 = (Qb + S) / Qa, W2 = (Qb - S) / Qa;
					const double Hi = FMath::Max(W1, W2), Lo = FMath::Min(W1, W2);
					if (R0 + Hi * Dr >= 0) W = Hi; else if (R0 + Lo * Dr >= 0) W = Lo;
				}
				if (!W) return {};
				T = *W;
			}
			if (Stops.Num() == 0) return {};
			// pad: the first stop's colour before it, the last's after; between two stops, premultiplied
			if (T <= Stops[0].Key) return Stops[0].Value;
			if (T >= Stops.Last().Key) return Stops.Last().Value;
			for (int32 i = 1; i < Stops.Num(); ++i)
			{
				if (T > Stops[i].Key) continue;
				const FRgba& P = Stops[i - 1].Value; const FRgba& Q = Stops[i].Value;
				const double Span = Stops[i].Key - Stops[i - 1].Key, K = Span > 0 ? (T - Stops[i - 1].Key) / Span : 1;
				FRgba C;
				C.A = P.A + (Q.A - P.A) * K;
				if (C.A <= 0) return C;
				C.R = (P.R * P.A + (Q.R * Q.A - P.R * P.A) * K) / C.A;
				C.G = (P.G * P.A + (Q.G * Q.A - P.G * P.A) * K) / C.A;
				C.B = (P.B * P.A + (Q.B * Q.A - P.B * P.A) * K) / C.A;
				return C;
			}
			return Stops.Last().Value;
		}
	};

	struct FPaint { FRgba Col; TSharedPtr<FGrad> Grad; };

	/** source-over, not premultiplied, in 8-bit sRGB values (as FS47Canvas blends) */
	void BlendPx(FS47Canvas& Cv, int32 X, int32 Y, double R, double G, double B, double A)
	{
		if (X < 0 || Y < 0 || X >= Cv.W || Y >= Cv.H || A <= 0) return;
		A = FMath::Min(1.0, A);
		FColor& D = Cv.Px[Y * Cv.W + X];
		const double Da = D.A / 255.0, Oa = A + Da * (1 - A);
		if (Oa <= 0) return;
		auto Ch = [&](double S, uint8 Dc) { return FS47Canvas::Clamp8((S * A + Dc * Da * (1 - A)) / Oa); };
		D = FColor(Ch(R, D.R), Ch(G, D.G), Ch(B, D.B), FS47Canvas::Clamp8(Oa * 255.0));
	}

	/** a gaussian blur of standard deviation Sigma over a layer (three box blurs, premultiplied; outside is clear) */
	void BlurLayer(FS47Canvas& L, double Sigma)
	{
		if (Sigma <= 0) return;
		const int32 Wd = L.W, Ht = L.H, N = Wd * Ht;
		TArray<float> Ch[4];
		for (int32 c = 0; c < 4; ++c) Ch[c].SetNumUninitialized(N);
		for (int32 i = 0; i < N; ++i)
		{
			const FColor& P = L.Px[i];
			const float A = P.A / 255.f;
			Ch[0][i] = P.R * A; Ch[1][i] = P.G * A; Ch[2][i] = P.B * A; Ch[3][i] = A;
		}
		// the three boxes for a gaussian of this sigma
		const double Wi = FMath::Sqrt(12 * Sigma * Sigma / 3 + 1);
		int32 Wl = FMath::FloorToInt(Wi); if (Wl % 2 == 0) Wl--;
		const int32 Wu = Wl + 2;
		const int32 M = FMath::RoundToInt((12 * Sigma * Sigma - 3 * Wl * Wl - 12 * Wl - 9) / (-4.0 * Wl - 4));
		TArray<float> Tmp; Tmp.SetNumUninitialized(N);
		auto Box = [&](TArray<float>& S, int32 R)
		{
			// horizontal, then vertical, a running sum with the outside clear
			const float Inv = 1.f / (2 * R + 1);
			for (int32 y = 0; y < Ht; ++y)
			{
				float Acc = 0;
				for (int32 x = -R; x <= R; ++x) if (x >= 0 && x < Wd) Acc += S[y * Wd + x];
				for (int32 x = 0; x < Wd; ++x)
				{
					Tmp[y * Wd + x] = Acc * Inv;
					const int32 Out = x - R, In = x + R + 1;
					if (Out >= 0) Acc -= S[y * Wd + Out];
					if (In < Wd) Acc += S[y * Wd + In];
				}
			}
			for (int32 x = 0; x < Wd; ++x)
			{
				float Acc = 0;
				for (int32 y = -R; y <= R; ++y) if (y >= 0 && y < Ht) Acc += Tmp[y * Wd + x];
				for (int32 y = 0; y < Ht; ++y)
				{
					S[y * Wd + x] = Acc * Inv;
					const int32 Out = y - R, In = y + R + 1;
					if (Out >= 0) Acc -= Tmp[Out * Wd + x];
					if (In < Ht) Acc += Tmp[In * Wd + x];
				}
			}
		};
		for (int32 Pass = 0; Pass < 3; ++Pass)
		{
			const int32 R = ((Pass < M ? Wl : Wu) - 1) / 2;
			for (int32 c = 0; c < 4; ++c) Box(Ch[c], R);
		}
		for (int32 i = 0; i < N; ++i)
		{
			const float A = Ch[3][i];
			if (A <= 1e-5f) { L.Px[i] = FColor(0, 0, 0, 0); continue; }
			L.Px[i] = FColor(FS47Canvas::Clamp8(Ch[0][i] / A), FS47Canvas::Clamp8(Ch[1][i] / A), FS47Canvas::Clamp8(Ch[2][i] / A), FS47Canvas::Clamp8(A * 255.0));
		}
	}

	class FPainter
	{
	public:
		FPainter(int32 InW, int32 InH) : Cv(InW, InH) {}
		FS47Canvas Cv;
		struct FState { FPaint Fill, Stroke; double LineWidth = 1, Blur = 0; const TCHAR* Font = TEXT("SpecialElite-Regular.ttf"); double FontPx = 10; FAff M; };
		FState S;
		TArray<FState> Stack;
		struct FSub { TArray<FVector2D> P; bool bClosed = false; };
		TArray<FSub> Path; // device space

		void Save() { Stack.Push(S); }
		void Restore() { if (Stack.Num()) S = Stack.Pop(); }
		void Translate(double X, double Y) { FAff T; T.E = X; T.F = Y; S.M = S.M.Times(T); }
		void Rotate(double A) { FAff R; R.A = FMath::Cos(A); R.B = FMath::Sin(A); R.C = -R.B; R.D = R.A; S.M = S.M.Times(R); }
		void FillStyle(const TCHAR* C) { S.Fill = FPaint{ Css(C), nullptr }; }
		void FillStyle(const TSharedPtr<FGrad>& G) { S.Fill = FPaint{ FRgba(), G }; }
		void StrokeStyle(const TCHAR* C) { S.Stroke = FPaint{ Css(C), nullptr }; }
		void Font(const TCHAR* File, double Px) { S.Font = File; S.FontPx = Px; }

		void BeginPath() { Path.Reset(); }
		void MoveTo(double X, double Y) { FSub& Sb = Path.AddDefaulted_GetRef(); Sb.P.Add(S.M.Apply(X, Y)); }
		void LineTo(double X, double Y) { if (!Path.Num() || Path.Last().bClosed) { MoveTo(X, Y); return; } Path.Last().P.Add(S.M.Apply(X, Y)); }
		void ClosePath()
		{
			if (!Path.Num()) return;
			Path.Last().bClosed = true;
			const FVector2D Start = Path.Last().P[0];
			FSub& Sb = Path.AddDefaulted_GetRef(); Sb.P.Add(Start);
		}
		void BezierCurveTo(double C1x, double C1y, double C2x, double C2y, double X, double Y)
		{
			if (!Path.Num()) MoveTo(C1x, C1y);
			// (flattened in device space: the transform is affine)
			const FVector2D P0 = Path.Last().P.Last(), P1 = S.M.Apply(C1x, C1y), P2 = S.M.Apply(C2x, C2y), P3 = S.M.Apply(X, Y);
			for (int32 k = 1; k <= 32; ++k)
			{
				const double T = k / 32.0, U = 1 - T;
				Path.Last().P.Add(P0 * (U * U * U) + P1 * (3 * U * U * T) + P2 * (3 * U * T * T) + P3 * (T * T * T));
			}
		}
		void Ellipse(double X, double Y, double Rx, double Ry, double Rot, double A0, double A1)
		{
			// clockwise from a0 to a1 (a whole turn when they are 2 pi apart): a line from where the path is to its start
			double Sweep = A1 - A0;
			if (Sweep >= 2 * UE_DOUBLE_PI) Sweep = 2 * UE_DOUBLE_PI;
			else { Sweep = FMath::Fmod(Sweep, 2 * UE_DOUBLE_PI); if (Sweep < 0) Sweep += 2 * UE_DOUBLE_PI; }
			const int32 N = FMath::Max(16, FMath::CeilToInt(Sweep * FMath::Max(Rx, Ry) / 1.5));
			const double Cr = FMath::Cos(Rot), Sr = FMath::Sin(Rot);
			for (int32 k = 0; k <= N; ++k)
			{
				const double T = A0 + Sweep * k / N, Ex = Rx * FMath::Cos(T), Ey = Ry * FMath::Sin(T);
				LineTo(X + Ex * Cr - Ey * Sr, Y + Ex * Sr + Ey * Cr);
			}
		}
		void Arc(double X, double Y, double R, double A0, double A1) { Ellipse(X, Y, R, R, 0, A0, A1); }

		void Fill() { const TArray<FSub> P = Path; Draw(S.Fill, [&P](FS47Canvas& C) { Replay(C, P); C.Fill(); }); }
		void Stroke()
		{
			const TArray<FSub> P = Path; const double Lw = S.LineWidth;
			Draw(S.Stroke, [&P, Lw](FS47Canvas& C) { Replay(C, P); C.LineWidth = Lw; C.Stroke(); });
		}
		void FillRect(double X, double Y, double Wd, double Ht)
		{
			if (S.M.Plain() && !S.Fill.Grad && S.Blur <= 0)
			{
				const FRgba& C = S.Fill.Col;
				Cv.SetFill((uint32(C.R) << 16) | (uint32(C.G) << 8) | uint32(C.B), C.A);
				Cv.FillRect(X + S.M.E, Y + S.M.F, Wd, Ht);
				return;
			}
			const TArray<FSub> Keep = Path;
			BeginPath(); MoveTo(X, Y); LineTo(X + Wd, Y); LineTo(X + Wd, Y + Ht); LineTo(X, Y + Ht); ClosePath();
			Fill();
			Path = Keep;
		}
		void StrokeRect(double X, double Y, double Wd, double Ht)
		{
			const TArray<FSub> Keep = Path;
			BeginPath(); MoveTo(X, Y); LineTo(X + Wd, Y); LineTo(X + Wd, Y + Ht); LineTo(X, Y + Ht); ClosePath();
			Stroke();
			Path = Keep;
		}
		void FillText(const FString& Text, double X, double Y)
		{
			const FRgba& C = S.Fill.Col;
			const uint32 Rgb = (uint32(C.R) << 16) | (uint32(C.G) << 8) | uint32(C.B);
			if (S.M.Plain())
			{
				Cv.SetFont(S.Font, S.FontPx); Cv.SetFill(Rgb, C.A);
				Cv.FillText(Text, X + S.M.E, Y + S.M.F);
				return;
			}
			// turned: the line drawn upright on a sheet of its own, then laid on through the transform
			constexpr int32 TW = 1400, TH = 320, OX = 60, OY = 200;
			FS47Canvas T(TW, TH);
			T.SetFont(S.Font, S.FontPx); T.SetFill(Rgb, 1.0);
			if (!T.FillText(Text, OX, OY)) return;
			int32 X0 = TW, Y0 = TH, X1 = -1, Y1 = -1;
			for (int32 y = 0; y < TH; ++y) for (int32 x = 0; x < TW; ++x) if (T.Px[y * TW + x].A) { X0 = FMath::Min(X0, x); Y0 = FMath::Min(Y0, y); X1 = FMath::Max(X1, x); Y1 = FMath::Max(Y1, y); }
			if (X1 < 0) return;
			FBox2D B(ForceInit);
			for (const FVector2D& Q : { FVector2D(X0, Y0), FVector2D(X1 + 1, Y0), FVector2D(X0, Y1 + 1), FVector2D(X1 + 1, Y1 + 1) }) B += S.M.Apply(X + Q.X - OX, Y + Q.Y - OY);
			const FAff Inv = S.M.Inverse();
			for (int32 py = FMath::Max(0, FMath::FloorToInt(B.Min.Y) - 1); py <= FMath::Min(Cv.H - 1, FMath::CeilToInt(B.Max.Y) + 1); ++py)
			{
				for (int32 px = FMath::Max(0, FMath::FloorToInt(B.Min.X) - 1); px <= FMath::Min(Cv.W - 1, FMath::CeilToInt(B.Max.X) + 1); ++px)
				{
					const FVector2D U = Inv.Apply(px + 0.5, py + 0.5);
					const double Tx = U.X - X + OX - 0.5, Ty = U.Y - Y + OY - 0.5;
					const int32 Ix = FMath::FloorToInt(Tx), Iy = FMath::FloorToInt(Ty);
					const double Fx = Tx - Ix, Fy = Ty - Iy;
					auto Al = [&T](int32 Xx, int32 Yy) { return Xx < 0 || Yy < 0 || Xx >= TW || Yy >= TH ? 0.0 : T.Px[Yy * TW + Xx].A / 255.0; };
					const double A = (Al(Ix, Iy) * (1 - Fx) + Al(Ix + 1, Iy) * Fx) * (1 - Fy) + (Al(Ix, Iy + 1) * (1 - Fx) + Al(Ix + 1, Iy + 1) * Fx) * Fy;
					if (A > 0) BlendPx(Cv, px, py, C.R, C.G, C.B, A * C.A);
				}
			}
		}

	private:
		static void Replay(FS47Canvas& C, const TArray<FSub>& P)
		{
			C.BeginPath();
			for (const FSub& Sb : P)
			{
				if (!Sb.P.Num()) continue;
				C.MoveTo(Sb.P[0].X, Sb.P[0].Y);
				for (int32 i = 1; i < Sb.P.Num(); ++i) C.LineTo(Sb.P[i].X, Sb.P[i].Y);
				if (Sb.bClosed) C.ClosePath();
			}
		}
		void Draw(const FPaint& P, TFunctionRef<void(FS47Canvas&)> Shape)
		{
			const uint32 Rgb = (uint32(P.Col.R) << 16) | (uint32(P.Col.G) << 8) | uint32(P.Col.B);
			if (!P.Grad && S.Blur <= 0)
			{
				Cv.SetFill(Rgb, P.Col.A); Cv.SetStroke(Rgb, P.Col.A);
				Shape(Cv);
				return;
			}
			FS47Canvas L(Cv.W, Cv.H);
			if (!P.Grad) { L.SetFill(Rgb, P.Col.A); L.SetStroke(Rgb, P.Col.A); Shape(L); }
			else
			{
				// the shape's coverage, then the gradient's colour at each pixel in user space
				FS47Canvas Mask(Cv.W, Cv.H);
				Mask.SetFill(0xffffff, 1); Mask.SetStroke(0xffffff, 1);
				Shape(Mask);
				const FAff Inv = S.M.Inverse();
				for (int32 y = 0; y < Cv.H; ++y) for (int32 x = 0; x < Cv.W; ++x)
				{
					const uint8 Cov = Mask.Px[y * Cv.W + x].A;
					if (!Cov) continue;
					const FVector2D U = Inv.Apply(x + 0.5, y + 0.5);
					const TOptional<FRgba> G = P.Grad->At(U.X, U.Y);
					if (!G || G->A <= 0) continue;
					L.Px[y * Cv.W + x] = FColor(FS47Canvas::Clamp8(G->R), FS47Canvas::Clamp8(G->G), FS47Canvas::Clamp8(G->B), FS47Canvas::Clamp8(Cov * G->A));
				}
			}
			BlurLayer(L, S.Blur);
			for (int32 i = 0; i < L.Px.Num(); ++i)
			{
				const FColor& C = L.Px[i];
				if (C.A) BlendPx(Cv, i % Cv.W, i / Cv.W, C.R, C.G, C.B, C.A / 255.0);
			}
		}
	};

	TSharedPtr<FGrad> Linear(double X0, double Y0, double X1, double Y1) { auto G = MakeShared<FGrad>(); G->X0 = X0; G->Y0 = Y0; G->X1 = X1; G->Y1 = Y1; return G; }
	TSharedPtr<FGrad> Radial(double X0, double Y0, double R0, double X1, double Y1, double R1) { auto G = MakeShared<FGrad>(); G->bRadial = true; G->X0 = X0; G->Y0 = Y0; G->R0 = R0; G->X1 = X1; G->Y1 = Y1; G->R1 = R1; return G; }

	// ---------- the envelope, drawn ----------
	constexpr int32 W = 900, H = 600;
	constexpr double EX = 150, EY = 120, EW = 600, EH = 360;   // the envelope on the picture
	const TCHAR* OSWALD = TEXT("Oswald-Regular.ttf");
	const TCHAR* OSWALD600 = TEXT("Oswald-Medium.ttf");          // (600: the heaviest Oswald the build has)
	const TCHAR* ELITE = TEXT("SpecialElite-Regular.ttf");
	const TCHAR* REENIE = TEXT("ReenieBeanie-Regular.ttf");
	int64 Rnd = 1;
	double R() { Rnd = (Rnd * 16807) % 2147483647; return double(Rnd - 1) / 2147483646; }

	void Grain(FPainter& G, double X, double Y, double Wd, double Ht, int32 N, const TCHAR* Dark, const TCHAR* Light)
	{
		for (int32 i = 0; i < N; ++i)
		{
			G.FillStyle(R() < 0.5 ? Dark : Light);
			const double A = R() * UE_DOUBLE_PI, L = 2 + R() * 9;
			G.Save();
			const double Tx = X + R() * Wd, Ty = Y + R() * Ht;
			G.Translate(Tx, Ty); G.Rotate(A); G.FillRect(-L / 2, 0, L, 0.8 + R() * 0.8);
			G.Restore();
		}
	}

	// what is written on the face of it, the same on both pictures (ink in front of the light)
	void Face(FPainter& G, bool bLit)
	{
		const TCHAR* Ink = bLit ? TEXT("rgba(28,20,14,.92)") : TEXT("rgba(30,26,34,.9)");
		// the shop's printed corner
		G.FillStyle(bLit ? TEXT("rgba(40,28,18,.8)") : TEXT("rgba(70,50,40,.85)"));
		G.Font(OSWALD600, 15);
		G.FillText(TEXT("PRINTS  .  ENLARGEMENTS  .  FINISHING"), EX + 26, EY + 34);
		G.Font(ELITE, 13);
		G.FillText(TEXT("ROSWELL, N. MEX."), EX + 26, EY + 54);
		// the stamp: three cents, purple, with its teeth
		const double Sx = EX + EW - 92, Sy = EY + 22;
		G.FillStyle(bLit ? TEXT("rgba(30,18,30,.95)") : TEXT("#6b4a7c"));
		for (int32 i = 0; i < 9; ++i) for (const FVector2D& P : { FVector2D(Sx + 4 + i * 7, Sy), FVector2D(Sx + 4 + i * 7, Sy + 82) }) { G.BeginPath(); G.Arc(P.X, P.Y, 2.4, 0, UE_DOUBLE_PI * 2); G.Fill(); }
		G.FillRect(Sx, Sy + 2, 66, 78);
		if (!bLit)
		{
			G.FillStyle(TEXT("#e9dcc6")); G.FillRect(Sx + 6, Sy + 8, 54, 66);
			G.FillStyle(TEXT("#6b4a7c")); G.FillRect(Sx + 9, Sy + 11, 48, 60);
			G.FillStyle(TEXT("#efe4d2")); G.Font(OSWALD600, 24); G.FillText(TEXT("3c"), Sx + 19, Sy + 50);
			G.Font(OSWALD, 9); G.FillText(TEXT("U.S. POSTAGE"), Sx + 11, Sy + 66);
		}
		// the postmark over it: a ring with the town and the month, the day under a smudge
		const double Px = Sx - 70, Py = Sy + 46;
		const TCHAR* Mark = bLit ? TEXT("rgba(25,18,14,.8)") : TEXT("rgba(40,40,60,.6)");
		G.StrokeStyle(Mark); G.S.LineWidth = 2;
		G.BeginPath(); G.Arc(Px, Py, 34, 0, UE_DOUBLE_PI * 2); G.Stroke();
		G.FillStyle(Mark); G.Font(ELITE, 11);
		G.FillText(TEXT("ROSWELL"), Px - 26, Py - 12); G.FillText(TEXT("JUL"), Px - 11, Py + 4); G.FillText(TEXT("1947"), Px - 14, Py + 18); G.FillText(TEXT("N.MEX."), Px - 19, Py + 30);
		G.FillStyle(bLit ? TEXT("rgba(25,18,14,.55)") : TEXT("rgba(40,40,60,.35)"));
		G.BeginPath(); G.Ellipse(Px + 14, Py + 4, 9, 6, 0.4, 0, UE_DOUBLE_PI * 2); G.Fill();
		for (int32 i = 0; i < 5; ++i)
		{
			G.BeginPath(); G.MoveTo(Px + 36, Py - 18 + i * 9);
			for (int32 X = 0; X <= 110; X += 10) G.LineTo(Px + 36 + X, Py - 18 + i * 9 + FMath::Sin(X / 9.0) * 3);
			G.Stroke();
		}
		// the address, typed
		G.FillStyle(Ink); G.Font(ELITE, 25);
		const double Ax = EX + 210, Ay = EY + 190;
		G.FillText(TEXT("T. Vega"), Ax, Ay);
		G.FillText(TEXT("c/o Sierra Motor Court"), Ax, Ay + 34);
		G.FillText(TEXT("Caliche, N. Mex."), Ax, Ay + 68);
		// the counter's pencil, along the bottom
		G.FillStyle(bLit ? TEXT("rgba(35,30,28,.85)") : TEXT("rgba(70,70,78,.85)"));
		G.Font(REENIE, 34);
		G.Save(); G.Translate(EX + 34, EY + EH - 30); G.Rotate(-0.025); G.FillText(TEXT("Plate 4.   1 print.   35c."), 0, 0); G.Restore();
	}

	// ---------- the field book's page (style.css .fbook .page.fb-page) ----------
	constexpr int32 PW = 440, PH = 667;
	UTexture2D* PagePaper()
	{
		static TWeakObjectPtr<UTexture2D> Made;
		if (Made.IsValid()) return Made.Get();
		TArray<FColor> Px;
		Px.SetNumUninitialized(PW * PH);
		auto Over = [](FRgba& D, double R, double G, double B, double A)
		{
			// source-over onto an opaque colour
			D.R = R * A + D.R * (1 - A); D.G = G * A + D.G * (1 - A); D.B = B * A + D.B * (1 - A);
		};
		// radial-gradient(ellipse at 30% 20%, rgba(255,250,235,.32), rgba(120,90,50,.22) 85%): farthest-corner,
		// the closest side's proportions
		const double Cx = PW * 0.3, Cy = PH * 0.2, Kx = FMath::Min(Cx, PW - Cx), Ky = FMath::Min(Cy, PH - Cy), Ratio = Kx / Ky;
		const double Fx = FMath::Max(Cx, PW - Cx), Fy = FMath::Max(Cy, PH - Cy), Ry = FMath::Sqrt(FMath::Square(Fx / Ratio) + Fy * Fy), Rx = Ry * Ratio;
		FGrad Light; Light.Add(0, TEXT("rgba(255,250,235,.32)")); Light.Add(0.85, TEXT("rgba(120,90,50,.22)"));
		const double RingX = PW * 0.82, RingY = PH * 0.88;
		for (int32 y = 0; y < PH; ++y)
		{
			for (int32 x = 0; x < PW; ++x)
			{
				const double X = x + 0.5, Y = y + 0.5;
				FRgba C = Css(TEXT("#e2d4ae"));
				// (the background images, the last listed first)
				const double T = FMath::Sqrt(FMath::Square((X - Cx) / Rx) + FMath::Square((Y - Cy) / Ry));
				const TOptional<FRgba> L = Light.At(T, 0);
				if (L) Over(C, L->R, L->G, L->B, L->A);
				// the ring a cup left: transparent 0 27px, rgba(120,80,30,.2) 28px 31px, rgba(120,80,30,.06) 32px, transparent 34px
				const double D = FMath::Sqrt(FMath::Square(X - RingX) + FMath::Square(Y - RingY));
				const double Ra = D <= 27 ? 0 : D < 28 ? 0.2 * (D - 27) : D <= 31 ? 0.2 : D < 32 ? 0.2 + (0.06 - 0.2) * (D - 31) : D < 34 ? 0.06 * (1 - (D - 32) / 2) : 0;
				if (Ra > 0) Over(C, 120, 80, 30, Ra);
				// the ruling: across every 28 px from 40 down (1 px), and fainter down every 28 px from 12 across
				if (FMath::Fmod(double(x) - 12 - 27 + 28 * 100, 28.0) < 1) Over(C, 80, 100, 80, 0.1);
				if (FMath::Fmod(double(y) - 40 - 27 + 28 * 100, 28.0) < 1) Over(C, 80, 100, 80, 0.2);
				// the two red margin lines
				const double U = X / PW;
				if ((U >= 0.114 && U < 0.117) || (U >= 0.123 && U < 0.126)) Over(C, 150, 60, 40, 0.42);
				// the inset shadows: all round (40 px), and down the bound edge
				const double E = FMath::Min(FMath::Min(X, PW - X), FMath::Min(Y, PH - Y));
				Over(C, 110, 80, 40, 0.35 / (1 + FMath::Exp(1.702 * E / 20)));
				Over(C, 60, 40, 20, 0.45 / (1 + FMath::Exp(1.702 * (X - 4) / 9)));
				Px[y * PW + x] = FColor(FS47Canvas::Clamp8(C.R), FS47Canvas::Clamp8(C.G), FS47Canvas::Clamp8(C.B), 255);
			}
		}
		UTexture2D* T = S47Image::FromPixels(Px, PW, PH);
		if (T) T->AddToRoot(); // (one sheet for the session)
		Made = T;
		return T;
	}

	void PaperSound(US47Audio* A)
	{
		// ui.onPaper (main.ts)
		if (!A) return;
		FS47Sound S; S.Gain = 0.35f; S.Rate = 0.95f + FMath::FRand() * 0.1f;
		A->Sfx(FMath::FRand() < 0.5f ? TEXT("paper0") : TEXT("paper1"), S);
	}

	FSlateFontInfo Px(FSlateFontInfo F, float Scale) { F.Size *= Scale; return F; }
}

// ---------- the book ----------

class SS47FieldBook : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47FieldBook) {}
	SLATE_END_ARGS()

	FBookOpts O;
	TWeakObjectPtr<US47UI> UI;
	TWeakObjectPtr<US47Audio> Audio;
	int32 At = -1;
	float TurnT = 0.f;
	TSharedPtr<FSlateBrush> PageBrush;
	TSharedPtr<SWidget> PageEl;

	void Construct(const FArguments&, const FBookOpts& InO, US47UI* InUI, US47Audio* InAudio)
	{
		O = InO; UI = InUI; Audio = InAudio;
		PageBrush = S47Kit::BrushFor(PagePaper());
		const float S = 1.1f; // (the page at the docview's size)
		const FLinearColor Hand = S47Kit::Css(0x26304e), Last = S47Kit::Css(0x171b2c);
		TSharedRef<SWidget> Page = SNew(SBox).WidthOverride(PW * S).HeightOverride(PH * S)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()[SNew(SImage).Image(PageBrush.Get())]
				// the hand: padding 11% 9% 8% 16% (the last page 13% left, 5% right; the cover centred, lower)
				+ SOverlay::Slot().Padding(TAttribute<FMargin>::CreateLambda([this, S]
				{
					const bool bLast = At >= 0 && O.Pages[At].bLast;
					return bLast ? FMargin(PW * 0.13f * S, PH * 0.38f * 0.55f * S, PW * 0.05f * S, PW * 0.08f * S) : FMargin(PW * 0.16f * S, PW * 0.11f * S + (At == 0 ? PW * 0.14f * 0.75f * S : 0.f), PW * 0.09f * S, PW * 0.08f * S);
				}))
				[
					SNew(STextBlock)
					.Text_Lambda([this] { return At >= 0 ? FText::FromString(O.Pages[At].Hand) : FText::GetEmpty(); })
					.Font_Lambda([this, S] { const bool bLast = At >= 0 && O.Pages[At].bLast; return Px(S47Style::Hand(bLast ? 34.f : 25.f), S); })
					.ColorAndOpacity_Lambda([this, Hand, Last] { return FSlateColor(At >= 0 && O.Pages[At].bLast ? Last : Hand); })
					.Justification_Lambda([this] { return At == 0 ? ETextJustify::Center : ETextJustify::Left; })
					.AutoWrapText(true)
					.RenderTransformPivot(FVector2D(0.5f, 0.5f))
					.RenderTransform_Lambda([this] { return At >= 0 && O.Pages[At].bLast ? TOptional<FSlateRenderTransform>(FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(-2.5f)))) : TOptional<FSlateRenderTransform>(); })
				]
				// the page's number, bottom right
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0, 0, PW * 0.07f * S, PH * 0.04f * S)
				[
					SNew(STextBlock).Text_Lambda([this] { return At > 0 ? FText::AsNumber(At) : FText::GetEmpty(); }).Font(Px(S47Style::Hand(18.f), S)).ColorAndOpacity(S47Kit::Css(0x28304e, 0.55f))
				]
			];
		PageEl = Page;
		const FLinearColor Off = S47Kit::Css(0x9a927e);
		ChildSlot
		[
			SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(S47Style::Overlay).Padding(0)
			.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent&) { if (UI.IsValid()) UI->Close(); return FReply::Handled(); })
			[
				SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(1240.f).HeightOverride(760.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(0.55f).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0, 0, 16, 0)
						[
							SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBorder")).Padding(0)
							.RenderTransformPivot(FVector2D(0.5f, 0.5f))
							.RenderTransform_Lambda([this]
							{
								// rotate(-1.4deg), and the turn: from -12 px over .32 s, ease-out
								const float K = TurnT > 0 ? 1.f - FMath::Square(TurnT / 0.32f) : 1.f;
								return TOptional<FSlateRenderTransform>(FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(-1.4f)), FVector2f(-12.f * (1.f - K), 0.f)));
							})
							.ColorAndOpacity_Lambda([this] { return FLinearColor(1, 1, 1, TurnT > 0 ? 0.3f + 0.7f * (1.f - FMath::Square(TurnT / 0.32f)) : 1.f); })
							.OnMouseButtonDown_Lambda([this](const FGeometry& G, const FPointerEvent& E)
							{
								// a click on the page turns it (the right part on, the left part back)
								const FVector2D L = G.AbsoluteToLocal(E.GetScreenSpacePosition());
								Go(L.X < G.GetLocalSize().X * 0.3 ? At - 1 : At + 1);
								return FReply::Handled();
							})
							[
								Page
							]
						]
						+ SHorizontalBox::Slot().FillWidth(0.45f)
						[
							SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f, 0.9f)).Padding(FMargin(28, 24))
							.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
								[
									SNew(STextBlock).Text(FText::FromString(O.Title)).Font(S47Style::Mono(30.f)).ColorAndOpacity(S47Style::Paper)
								]
								+ SVerticalBox::Slot().FillHeight(1.f)
								[
									SNew(SScrollBox)
									+ SScrollBox::Slot()
									[
										SNew(STextBlock).Text_Lambda([this] { return At >= 0 ? FText::FromString(O.Pages[At].Typed) : FText::GetEmpty(); })
										.AutoWrapText(true).Font(S47Style::Body(19.f)).ColorAndOpacity(FLinearColor(0.85f, 0.83f, 0.78f)).LineHeightPercentage(1.25f)
									]
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
								[
									SNew(STextBlock).Text_Lambda([this] { return FText::FromString(FString::Printf(TEXT("Page %d of %d"), At + 1, O.Pages.Num())); }).Font(S47Style::Mono(18.f * 1.25f)).ColorAndOpacity(Off)
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 0)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 22, 0)
									[
										S47Kit::Button(TEXT("< Back"), [this] { Go(At - 1); }, false, TAttribute<bool>::CreateLambda([this] { return At > 0; }))
									]
									+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 22, 0)
									[
										S47Kit::Button(TEXT("Next page >"), [this] { Go(At + 1); }, false, TAttribute<bool>::CreateLambda([this] { return At < O.Pages.Num() - 1; }))
									]
									+ SHorizontalBox::Slot().AutoWidth()
									[
										S47Kit::Button(TEXT("[E] Put it back"), [this] { if (UI.IsValid()) UI->Close(); }, true)
									]
								]
							]
						]
					]
				]
			]
		];
	}

	void Go(int32 N)
	{
		N = FMath::Clamp(N, 0, O.Pages.Num() - 1);
		if (N == At) return;
		const bool bFirst = At < 0;
		At = N;
		// the page turned: a new sheet under the hand, and its sound
		if (!bFirst) { PaperSound(Audio.Get()); TurnT = 0.32f; }
		// (ui.readingControls(side, [p.typed], 'Read document', 'document'): the narrators are not ported)
		if (O.OnPage) O.OnPage(N);
	}

	virtual void Tick(const FGeometry& G, const double Now, const float Dt) override
	{
		SCompoundWidget::Tick(G, Now, Dt);
		TurnT = FMath::Max(0.f, TurnT - Dt);
	}
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& E) override
	{
		const FKey K = E.GetKey();
		if (K == EKeys::Right || K == EKeys::D || K == EKeys::PageDown || K == EKeys::SpaceBar) { Go(At + 1); return FReply::Handled(); }
		if (K == EKeys::Left || K == EKeys::A || K == EKeys::PageUp) { Go(At - 1); return FReply::Handled(); }
		if (K == EKeys::E || K == EKeys::Escape || K == EKeys::Tab) { if (UI.IsValid()) UI->Close(); return FReply::Handled(); }
		return FReply::Unhandled();
	}
};

FBookView S47FieldBookView::Book(US47UI* UI, US47Audio* Audio, const FBookOpts& O)
{
	TSharedRef<SS47FieldBook> El = SNew(SS47FieldBook, O, UI, Audio);
	PaperSound(Audio);
	TWeakPtr<SS47FieldBook> Wk = El;
	if (UI) UI->Open(El, [Wk] { if (const TSharedPtr<SS47FieldBook> P = Wk.Pin()) if (P->O.OnClose) P->O.OnClose(P->At); });
	El->Go(O.Start);
	FBookView V;
	V.Page = [Wk] { const TSharedPtr<SS47FieldBook> P = Wk.Pin(); return P.IsValid() ? P->At : -1; };
	V.Go = [Wk](int32 N) { if (const TSharedPtr<SS47FieldBook> P = Wk.Pin()) P->Go(N); };
	return V;
}

// ---------- the envelope ----------

class SS47Envelope : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Envelope) {}
	SLATE_END_ARGS()

	FEnvelopeOpts O;
	TWeakObjectPtr<US47UI> UI;
	TWeakObjectPtr<US47Audio> Audio;
	bool bUp = false;
	float LitK = 0.f;   // the lit picture's opacity (a 1.1 s transition)
	TSharedPtr<FSlateBrush> FrontBrush, LitBrush;

	void Construct(const FArguments&, const FEnvelopeOpts& InO, US47UI* InUI, US47Audio* InAudio)
	{
		O = InO; UI = InUI; Audio = InAudio;
		FrontBrush = S47Kit::BrushFor(S47Image::FromDataUrl(O.Front));
		LitBrush = S47Kit::BrushFor(S47Image::FromDataUrl(O.Lit));
		ChildSlot
		[
			SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(S47Style::Overlay).Padding(0)
			.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent&) { if (UI.IsValid()) UI->Close(); return FReply::Handled(); })
			[
				SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(1240.f).HeightOverride(760.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(0.55f).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0, 0, 16, 0)
						[
							SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBorder")).Padding(0)
							.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })
							[
								SNew(SBox).WidthOverride(660.f).HeightOverride(440.f)
								[
								SNew(SOverlay)
								+ SOverlay::Slot()[SNew(SImage).Image(FrontBrush.Get()).ColorAndOpacity_Lambda([this] { return FSlateColor(FLinearColor(1, 1, 1, 1.f - LitK)); })]
								+ SOverlay::Slot()[SNew(SImage).Image(LitBrush.Get()).ColorAndOpacity_Lambda([this] { return FSlateColor(FLinearColor(1, 1, 1, LitK)); })]
								]
							]
						]
						+ SHorizontalBox::Slot().FillWidth(0.45f)
						[
							SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f, 0.9f)).Padding(FMargin(28, 24))
							.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
								[
									SNew(STextBlock).Text(FText::FromString(O.Title)).Font(S47Style::Mono(30.f)).ColorAndOpacity(S47Style::Paper).AutoWrapText(true)
								]
								+ SVerticalBox::Slot().FillHeight(1.f)
								[
									SNew(SScrollBox)
									+ SScrollBox::Slot()
									[
										// the paragraphs (text.split('\n\n'), each a <p>)
										SNew(STextBlock).Text_Lambda([this] { return FText::FromString((bUp ? O.LitText : O.Text)); })
										.AutoWrapText(true).Font(S47Style::Body(19.f)).ColorAndOpacity(FLinearColor(0.85f, 0.83f, 0.78f)).LineHeightPercentage(1.25f)
									]
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 0)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 22, 0)
									[
										SNew(SButton).OnClicked_Lambda([this] { Toggle(); return FReply::Handled(); })
										.ButtonStyle(FCoreStyle::Get(), "NoBorder")
										[
											SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(S47Kit::Css(0xffc896, 0.1f)).Padding(FMargin(14, 6))
											[
												SNew(STextBlock).Text_Lambda([this] { return FText::FromString(bUp ? TEXT("Lower it") : TEXT("Hold it up to the lamp")); }).Font(S47Style::Mono(24.f)).ColorAndOpacity(S47Style::Amber)
											]
										]
									]
									+ SHorizontalBox::Slot().AutoWidth()
									[
										S47Kit::Button(TEXT("[E] Put it back"), [this] { if (UI.IsValid()) UI->Close(); }, true)
									]
								]
							]
						]
					]
				]
			]
		];
	}

	void Toggle()
	{
		bUp = !bUp;
		PaperSound(Audio.Get());
		// (ui.readingControls: the narrators are not ported)
		if (bUp && O.OnHold) O.OnHold();
	}

	virtual void Tick(const FGeometry& G, const double Now, const float Dt) override
	{
		SCompoundWidget::Tick(G, Now, Dt);
		const float Want = bUp ? 1.f : 0.f;
		LitK = Want > LitK ? FMath::Min(Want, LitK + Dt / 1.1f) : FMath::Max(Want, LitK - Dt / 1.1f);
	}
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& E) override
	{
		const FKey K = E.GetKey();
		if (K == EKeys::E || K == EKeys::Escape || K == EKeys::Tab) { if (UI.IsValid()) UI->Close(); return FReply::Handled(); }
		return FReply::Unhandled();
	}
};

FEnvelopeView S47FieldBookView::Envelope(US47UI* UI, US47Audio* Audio, const FEnvelopeOpts& O)
{
	TSharedRef<SS47Envelope> El = SNew(SS47Envelope, O, UI, Audio);
	PaperSound(Audio);
	TWeakPtr<SS47Envelope> Wk = El;
	if (UI) UI->Open(El, [Wk] { if (const TSharedPtr<SS47Envelope> P = Wk.Pin()) if (P->O.OnClose) P->O.OnClose(); });
	FEnvelopeView V;
	V.Hold = [Wk] { if (const TSharedPtr<SS47Envelope> P = Wk.Pin()) if (!P->bUp) P->Toggle(); };
	return V;
}

// ---------- the envelope's pictures ----------

FString S47FieldBookView::EnvelopeFront()
{
	Rnd = 4711;
	FPainter G(W, H);
	// the counter: dark varnished wood
	TSharedPtr<FGrad> Wood = Linear(0, 0, 0, H);
	Wood->Add(0, TEXT("#2a1a10")); Wood->Add(1, TEXT("#170e08"));
	G.FillStyle(Wood); G.FillRect(0, 0, W, H);
	for (int32 i = 0; i < 70; ++i)
	{
		const double Cr = 60 + R() * 40, Cg = 35 + R() * 20, Cb = 18 + R() * 10;
		G.StrokeStyle(*FString::Printf(TEXT("rgba(%f,%f,%f,.25)"), Cr, Cg, Cb));
		G.S.LineWidth = 1 + R() * 2;
		const double Y = R() * H;
		G.BeginPath(); G.MoveTo(0, Y);
		const double A1 = Y + R() * 20 - 10, A2 = Y + R() * 20 - 10, A3 = Y + R() * 16 - 8;
		G.BezierCurveTo(300, A1, 600, A2, W, A3);
		G.Stroke();
	}
	// the lamp's pool on it, from the right
	TSharedPtr<FGrad> Pool = Radial(W * 0.78, H * 0.18, 30, W * 0.7, H * 0.3, 620);
	Pool->Add(0, TEXT("rgba(255,200,130,.32)")); Pool->Add(1, TEXT("rgba(255,200,130,0)"));
	G.FillStyle(Pool); G.FillRect(0, 0, W, H);
	G.Save(); G.Translate(W / 2.0, H / 2.0); G.Rotate(-0.035); G.Translate(-W / 2.0, -H / 2.0);
	G.FillStyle(TEXT("rgba(0,0,0,.45)")); G.FillRect(EX + 10, EY + 12, EW, EH);
	TSharedPtr<FGrad> Paper = Linear(EX, EY, EX + EW, EY + EH);
	Paper->Add(0, TEXT("#d6c193")); Paper->Add(0.6, TEXT("#c9b080")); Paper->Add(1, TEXT("#b39a6c"));
	G.FillStyle(Paper); G.FillRect(EX, EY, EW, EH);
	Grain(G, EX, EY, EW, EH, 2600, TEXT("rgba(110,85,50,.14)"), TEXT("rgba(255,245,220,.12)"));
	// 39 years on a counter: the edges gone darker, a ring where a cup stood once
	TSharedPtr<FGrad> Edge = Radial(EX + EW / 2, EY + EH / 2, EH * 0.4, EX + EW / 2, EY + EH / 2, EW * 0.62);
	Edge->Add(0, TEXT("rgba(90,60,30,0)")); Edge->Add(1, TEXT("rgba(90,60,30,.35)"));
	G.FillStyle(Edge); G.FillRect(EX, EY, EW, EH);
	G.StrokeStyle(TEXT("rgba(110,80,45,.22)")); G.S.LineWidth = 5; G.BeginPath(); G.Arc(EX + EW - 130, EY + EH - 90, 46, 0.3, 5.6); G.Stroke();
	Face(G, false);
	G.Restore();
	return G.Cv.ToDataUrl(true, 86);
}

FString S47FieldBookView::EnvelopeLit()
{
	Rnd = 4711;
	FPainter G(W, H);
	G.FillStyle(TEXT("#080604")); G.FillRect(0, 0, W, H);
	// the lamp behind it, a little right of the middle
	const double Lx = EX + EW * 0.6, Ly = EY + EH * 0.42;
	TSharedPtr<FGrad> Halo = Radial(Lx, Ly, 20, Lx, Ly, 560);
	Halo->Add(0, TEXT("rgba(255,190,110,.55)")); Halo->Add(0.4, TEXT("rgba(200,120,50,.18)")); Halo->Add(1, TEXT("rgba(0,0,0,0)"));
	G.FillStyle(Halo); G.FillRect(0, 0, W, H);
	G.Save(); G.Translate(W / 2.0, H / 2.0); G.Rotate(0.02); G.Translate(-W / 2.0, -H / 2.0);
	// the paper, the light coming through it
	TSharedPtr<FGrad> Through = Radial(Lx, Ly, 10, Lx, Ly, EW * 0.75);
	Through->Add(0, TEXT("#fff1c8")); Through->Add(0.35, TEXT("#f0c983")); Through->Add(0.8, TEXT("#b97b3c")); Through->Add(1, TEXT("#7a4a22"));
	G.FillStyle(Through); G.FillRect(EX, EY, EW, EH);
	Grain(G, EX, EY, EW, EH, 3600, TEXT("rgba(120,70,30,.16)"), TEXT("rgba(255,240,200,.1)"));
	// the flaps on the back, where the paper is double: darker, meeting under the middle
	G.FillStyle(TEXT("rgba(110,60,20,.22)"));
	G.BeginPath(); G.MoveTo(EX, EY); G.LineTo(EX + EW * 0.5, EY + EH * 0.62); G.LineTo(EX + EW, EY); G.ClosePath(); G.Fill();
	G.BeginPath(); G.MoveTo(EX, EY + EH); G.LineTo(EX + EW * 0.44, EY + EH * 0.5); G.LineTo(EX + EW * 0.56, EY + EH * 0.5); G.LineTo(EX + EW, EY + EH); G.ClosePath(); G.Fill();
	// the print inside: a smaller sheet, a little askew, as a shadow
	const double PX = EX + 92, PY = EY + 64, PWd = 400, PHt = 262;
	G.Save(); G.Translate(PX + PWd / 2, PY + PHt / 2); G.Rotate(0.022); G.Translate(-PWd / 2, -PHt / 2);
	G.S.Blur = 2.2;
	G.FillStyle(TEXT("rgba(70,32,10,.42)")); G.FillRect(0, 0, PWd, PHt);
	// what is light on the print lets the lamp through: the sky over a dark line of ground
	TSharedPtr<FGrad> Sky = Linear(0, 0, 0, PHt);
	Sky->Add(0, TEXT("rgba(255,220,160,.26)")); Sky->Add(0.36, TEXT("rgba(255,215,150,.38)")); Sky->Add(0.43, TEXT("rgba(60,25,8,.0)"));
	G.FillStyle(Sky); G.FillRect(6, 6, PWd - 12, PHt * 0.43);
	G.FillStyle(TEXT("rgba(55,22,6,.5)")); G.FillRect(0, PHt * 0.43, PWd, PHt * 0.57);
	// the transit on its three legs, on the line of the ground (above where the address is typed)
	const double GROUND = PHt * 0.43, Tx = 66, Ty = GROUND - 44;
	G.StrokeStyle(TEXT("rgba(40,14,2,.85)")); G.S.LineWidth = 3;
	for (const double Dx : { -20.0, 2.0, 19.0 }) { G.BeginPath(); G.MoveTo(Tx, Ty + 6); G.LineTo(Tx + Dx, GROUND + 2); G.Stroke(); }
	G.FillStyle(TEXT("rgba(40,14,2,.85)")); G.FillRect(Tx - 8, Ty - 8, 16, 12); G.FillRect(Tx - 2, Ty - 14, 4, 7);
	// three standing by it: two together, and one a step off and not as sharp
	auto Fig = [&G, GROUND](double X, double Hh, double Blur, double A)
	{
		G.S.Blur = Blur;
		G.FillStyle(*FString::Printf(TEXT("rgba(36,12,2,%f)"), A));
		const double Top = GROUND + 2 - Hh;
		G.BeginPath(); G.Ellipse(X, Top + Hh * 0.09, Hh * 0.075, Hh * 0.09, 0, 0, UE_DOUBLE_PI * 2); G.Fill();       // the head
		G.BeginPath(); G.MoveTo(X - Hh * 0.13, Top + Hh * 0.2); G.LineTo(X + Hh * 0.13, Top + Hh * 0.2);               // shoulders, body
		G.LineTo(X + Hh * 0.1, Top + Hh * 0.6); G.LineTo(X + Hh * 0.06, Top + Hh); G.LineTo(X - Hh * 0.06, Top + Hh); G.LineTo(X - Hh * 0.1, Top + Hh * 0.6); G.ClosePath(); G.Fill();
	};
	Fig(112, 66, 2.2, 0.88);
	Fig(138, 60, 2.2, 0.86);
	Fig(196, 64, 4.6, 0.6);
	G.S.Blur = 0;
	G.Restore();
	// the wax seal on the back, where the flaps meet
	G.FillStyle(TEXT("rgba(40,10,4,.7)")); G.BeginPath(); G.Arc(EX + EW * 0.5, EY + EH * 0.6, 15, 0, UE_DOUBLE_PI * 2); G.Fill();
	// the edges of the paper, darker where it curls away from the light
	G.StrokeStyle(TEXT("rgba(80,40,14,.6)")); G.S.LineWidth = 3; G.StrokeRect(EX, EY, EW, EH);
	Face(G, true);
	// the thumbs holding it up at the two bottom corners, dark against the light
	G.FillStyle(TEXT("#100a07"));
	const double Thumbs[2][4] = { { EX + 58, EY + EH + 34, EX + 70, EY + EH - 14 }, { EX + EW - 56, EY + EH + 34, EX + EW - 68, EY + EH - 12 } };
	for (const auto& Th : Thumbs)
	{
		G.StrokeStyle(TEXT("#100a07")); G.S.LineWidth = 44; G.BeginPath(); G.MoveTo(Th[0], Th[1]); G.LineTo(Th[2], Th[3]); G.Stroke();
		// the light round the edge of the thumb, through the paper
		G.StrokeStyle(TEXT("rgba(255,170,90,.18)")); G.S.LineWidth = 2; G.BeginPath(); G.Arc(Th[2], Th[3], 23, UE_DOUBLE_PI * 1.05, UE_DOUBLE_PI * 1.95); G.Stroke();
	}
	G.Restore();
	return G.Cv.ToDataUrl(true, 88);
}

bool S47FieldBookView::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s field book view: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	const FRgba A = Css(TEXT("rgba(73.6,40,18,.25)")), B = Css(TEXT("#6b4a7c"));
	Check(A.R == 74 && A.G == 40 && FMath::IsNearlyEqual(A.A, 0.25) && B.R == 0x6b && B.B == 0x7c && B.A == 1, TEXT("CSS colours as a canvas reads them"));
	// a radial gradient between two circles, and a linear one, padded at their ends
	FGrad L; L.X1 = 100; L.Add(0, TEXT("#000000")); L.Add(1, TEXT("#ffffff"));
	Check(FMath::IsNearlyEqual(L.At(50, 7)->R, 127.5) && L.At(-5, 0)->R == 0 && L.At(500, 0)->R == 255, TEXT("a linear gradient"));
	FGrad Rg; Rg.bRadial = true; Rg.X0 = 0; Rg.R0 = 10; Rg.X1 = 0; Rg.R1 = 110; Rg.Add(0, TEXT("rgba(255,0,0,1)")); Rg.Add(1, TEXT("rgba(255,0,0,0)"));
	Check(FMath::IsNearlyEqual(Rg.At(60, 0)->A, 0.5) && Rg.At(0, 0)->A == 1 && Rg.At(200, 0)->A == 0, TEXT("a radial gradient between two circles"));
	// the blur keeps what is drawn and spreads it
	FS47Canvas C(40, 40);
	C.SetFill(0xffffff, 1); C.FillRect(18, 18, 4, 4);
	BlurLayer(C, 2.2);
	double Sum = 0;
	for (const FColor& P : C.Px) Sum += P.A / 255.0;
	Check(FMath::IsNearlyEqual(Sum, 16.0, 0.5) && C.Px[20 * 40 + 20].A < 255 && C.Px[20 * 40 + 26].A > 0, TEXT("a gaussian blur, the coverage kept"));
	// the turn in user space
	FPainter P(10, 10);
	P.Translate(5, 5); P.Rotate(UE_DOUBLE_PI / 2);
	const FVector2D Q = P.S.M.Apply(1, 0), Back = P.S.M.Inverse().Apply(Q.X, Q.Y);
	Check(Q.Equals(FVector2D(5, 6), 1e-9) && Back.Equals(FVector2D(1, 0), 1e-9), TEXT("translate and rotate, and back"));
	return bOk;
}

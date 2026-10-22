// A 2D canvas on the CPU for the frames of the time away and of FLASH (see S47Raster.h)
#include "S47Raster.h"

#include "Algo/Reverse.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "S47Canvas.h"

namespace
{
	FVector4f Premul(const FVector4f& C) { return FVector4f(C.X * C.W, C.Y * C.W, C.Z * C.W, C.W); }
	FVector4f Unpremul(const FVector4f& C) { return C.W > 1e-6f ? FVector4f(C.X / C.W, C.Y / C.W, C.Z / C.W, C.W) : FVector4f(0, 0, 0, 0); }
	FVector4f Rgba(uint32 Rgb, double A) { return FVector4f(((Rgb >> 16) & 255) / 255.f, ((Rgb >> 8) & 255) / 255.f, (Rgb & 255) / 255.f, float(A)); }
	constexpr int32 SUB = 4; // sub-scanlines per pixel row (the edges' anti-aliasing)
}

// ---------- the state ----------

FS47Pix::FS47Pix(int32 InW, int32 InH) : W(FMath::Max(1, InW)), H(FMath::Max(1, InH))
{
	P.Init(FVector4f(0, 0, 0, 0), W * H);
}

FS47Pix::FPaint& FS47Pix::FPaint::Stop(double T, uint32 Rgb, double A)
{
	Stops.Add({ FMath::Clamp(T, 0.0, 1.0), Rgba(Rgb, A) });
	return *this;
}

FS47Pix::FPaint FS47Pix::Solid(uint32 Rgb, double A) { FPaint Pt; Pt.Kind = FPaint::EKind::Solid; Pt.C = Rgba(Rgb, A); return Pt; }
FS47Pix::FPaint FS47Pix::Radial(double X0, double Y0, double R0, double X1, double Y1, double R1)
{
	FPaint Pt; Pt.Kind = FPaint::EKind::Radial; Pt.X0 = X0; Pt.Y0 = Y0; Pt.R0 = R0; Pt.X1 = X1; Pt.Y1 = Y1; Pt.R1 = R1; return Pt;
}
FS47Pix::FPaint FS47Pix::Linear(double X0, double Y0, double X1, double Y1)
{
	FPaint Pt; Pt.Kind = FPaint::EKind::Linear; Pt.X0 = X0; Pt.Y0 = Y0; Pt.X1 = X1; Pt.Y1 = Y1; return Pt;
}

void FS47Pix::Save() { Stack.Add({ M, FillStyle, StrokeStyle, LineWidth, GlobalAlpha, bLighter, Filter, ClipMask }); }
void FS47Pix::Restore()
{
	if (!Stack.Num()) return;
	const FState S = Stack.Pop();
	M = S.M; FillStyle = S.Fill; StrokeStyle = S.Stroke; LineWidth = S.LineWidth; GlobalAlpha = S.GlobalAlpha; bLighter = S.bLighter; Filter = S.Filter; ClipMask = S.Clip;
}
void FS47Pix::Translate(double X, double Y) { M.E += M.A * X + M.C * Y; M.F += M.B * X + M.D * Y; }
void FS47Pix::Rotate(double T)
{
	const double Cs = FMath::Cos(T), Sn = FMath::Sin(T);
	const FMat O = M;
	M.A = O.A * Cs + O.C * Sn; M.B = O.B * Cs + O.D * Sn;
	M.C = -O.A * Sn + O.C * Cs; M.D = -O.B * Sn + O.D * Cs;
}
void FS47Pix::Scale(double X, double Y) { M.A *= X; M.B *= X; M.C *= Y; M.D *= Y; }

FS47Pix::FMat FS47Pix::FMat::Inverse() const
{
	const double Det = A * D - B * C;
	FMat I;
	if (FMath::Abs(Det) < 1e-12) return I;
	I.A = D / Det; I.B = -B / Det; I.C = -C / Det; I.D = A / Det;
	I.E = -(I.A * E + I.C * F); I.F = -(I.B * E + I.D * F);
	return I;
}

// ---------- paths ----------

void FS47Pix::BeginPath() { Subs.Reset(); }
FS47Pix::FSub& FS47Pix::Cur() { if (!Subs.Num() || Subs.Last().bClosed) { FSub S; if (Subs.Num() && Subs.Last().Pts.Num()) S.Pts.Add(Subs.Last().Pts[0]); Subs.Add(S); } return Subs.Last(); }
void FS47Pix::MoveTo(double X, double Y) { FSub S; S.Pts.Add(M.Apply(X, Y)); Subs.Add(S); }
void FS47Pix::LineTo(double X, double Y)
{
	if (!Subs.Num() || !Subs.Last().Pts.Num()) { MoveTo(X, Y); return; }
	Cur().Pts.Add(M.Apply(X, Y));
}
void FS47Pix::ClosePath() { if (Subs.Num() && Subs.Last().Pts.Num()) Subs.Last().bClosed = true; }

double FS47Pix::Seg(double DeviceLen) const { return FMath::Clamp(FMath::CeilToDouble(DeviceLen / 2.0), 6.0, 512.0); }

void FS47Pix::QuadraticCurveTo(double Cx, double Cy, double X, double Y)
{
	if (!Subs.Num() || !Subs.Last().Pts.Num()) MoveTo(Cx, Cy);
	FSub& S = Cur();
	const FVector2D Last = S.Pts.Last();
	const FMat I = M.Inverse();
	const FVector2D P0 = I.Apply(Last.X, Last.Y);
	const double L = (M.Apply(Cx, Cy) - Last).Size() + (M.Apply(X, Y) - M.Apply(Cx, Cy)).Size();
	const int32 N = int32(Seg(L));
	for (int32 k = 1; k <= N; ++k)
	{
		const double T = double(k) / N, U = 1 - T;
		Cur().Pts.Add(M.Apply(U * U * P0.X + 2 * U * T * Cx + T * T * X, U * U * P0.Y + 2 * U * T * Cy + T * T * Y));
	}
}

void FS47Pix::BezierCurveTo(double C1x, double C1y, double C2x, double C2y, double X, double Y)
{
	if (!Subs.Num() || !Subs.Last().Pts.Num()) MoveTo(C1x, C1y);
	FSub& S = Cur();
	const FVector2D Last = S.Pts.Last();
	const FMat I = M.Inverse();
	const FVector2D P0 = I.Apply(Last.X, Last.Y);
	const double L = (M.Apply(C1x, C1y) - Last).Size() + (M.Apply(C2x, C2y) - M.Apply(C1x, C1y)).Size() + (M.Apply(X, Y) - M.Apply(C2x, C2y)).Size();
	const int32 N = int32(Seg(L));
	for (int32 k = 1; k <= N; ++k)
	{
		const double T = double(k) / N, U = 1 - T;
		const double A = U * U * U, B = 3 * U * U * T, C = 3 * U * T * T, D = T * T * T;
		Cur().Pts.Add(M.Apply(A * P0.X + B * C1x + C * C2x + D * X, A * P0.Y + B * C1y + C * C2y + D * Y));
	}
}

void FS47Pix::Ellipse(double Cx, double Cy, double Rx, double Ry, double Rot, double A0, double A1, bool bCcw)
{
	// the canvas's sweep: a whole turn or more is the whole ellipse; otherwise the way round asked for
	const double Tau = 2 * UE_DOUBLE_PI;
	double Sweep;
	if (!bCcw && A1 - A0 >= Tau) Sweep = Tau;
	else if (bCcw && A0 - A1 >= Tau) Sweep = -Tau;
	else
	{
		Sweep = FMath::Fmod(A1 - A0, Tau);
		if (!bCcw && Sweep < 0) Sweep += Tau;
		if (bCcw && Sweep > 0) Sweep -= Tau;
	}
	const double Cr = FMath::Cos(Rot), Sr = FMath::Sin(Rot);
	auto At = [&](double A) { const double Ex = Rx * FMath::Cos(A), Ey = Ry * FMath::Sin(A); return M.Apply(Cx + Ex * Cr - Ey * Sr, Cy + Ex * Sr + Ey * Cr); };
	const double Scale = FMath::Sqrt(FMath::Abs(M.A * M.D - M.B * M.C));
	const int32 N = int32(Seg(FMath::Abs(Sweep) * FMath::Max(Rx, Ry) * Scale));
	const FVector2D First = At(A0);
	if (!Subs.Num() || !Subs.Last().Pts.Num() || Subs.Last().bClosed) { FSub S; S.Pts.Add(First); Subs.Add(S); }
	else Subs.Last().Pts.Add(First);
	for (int32 k = 1; k <= N; ++k) Subs.Last().Pts.Add(At(A0 + Sweep * k / N));
}

void FS47Pix::Arc(double Cx, double Cy, double R, double A0, double A1, bool bCcw) { Ellipse(Cx, Cy, R, R, 0, A0, A1, bCcw); }

void FS47Pix::RoundRect(double X, double Y, double Wd, double Ht, double R)
{
	R = FMath::Max(0.0, FMath::Min(R, FMath::Min(FMath::Abs(Wd), FMath::Abs(Ht)) / 2));
	const double Pi = UE_DOUBLE_PI;
	MoveTo(X + R, Y);
	LineTo(X + Wd - R, Y);
	Arc(X + Wd - R, Y + R, R, -Pi / 2, 0);
	LineTo(X + Wd, Y + Ht - R);
	Arc(X + Wd - R, Y + Ht - R, R, 0, Pi / 2);
	LineTo(X + R, Y + Ht);
	Arc(X + R, Y + Ht - R, R, Pi / 2, Pi);
	LineTo(X, Y + R);
	Arc(X + R, Y + R, R, Pi, Pi * 1.5);
	ClosePath();
}

void FS47Pix::Polys(TArray<TArray<FVector2D>>& Out, bool bForStroke) const
{
	if (!bForStroke)
	{
		for (const FSub& S : Subs) if (S.Pts.Num() >= 3) Out.Add(S.Pts);
		return;
	}
	// a stroke as the union of each segment's quad and a round join at each corner (the canvas's
	// miter joins and butt caps are near enough at the widths the frames use)
	const double Hw = LineWidth * FMath::Sqrt(FMath::Abs(M.A * M.D - M.B * M.C)) / 2;
	auto Ccw = [](TArray<FVector2D>& Pg)
	{
		double A = 0;
		for (int32 i = 0; i < Pg.Num(); ++i) { const FVector2D& P0 = Pg[i]; const FVector2D& P1 = Pg[(i + 1) % Pg.Num()]; A += P0.X * P1.Y - P1.X * P0.Y; }
		if (A < 0) Algo::Reverse(Pg);
	};
	for (const FSub& S : Subs)
	{
		const int32 N = S.Pts.Num();
		if (N < 2) continue;
		const int32 Segs = S.bClosed ? N : N - 1;
		for (int32 i = 0; i < Segs; ++i)
		{
			const FVector2D A = S.Pts[i], B = S.Pts[(i + 1) % N];
			FVector2D D = B - A;
			const double L = D.Size();
			if (L < 1e-9) continue;
			D /= L;
			const FVector2D Nn(-D.Y * Hw, D.X * Hw);
			TArray<FVector2D> Q = { A + Nn, B + Nn, B - Nn, A - Nn };
			Ccw(Q);
			Out.Add(Q);
		}
		for (int32 i = S.bClosed ? 0 : 1; i < (S.bClosed ? N : N - 1); ++i)
		{
			TArray<FVector2D> C;
			for (int32 k = 0; k < 12; ++k) C.Add(S.Pts[i] + FVector2D(FMath::Cos(k * UE_DOUBLE_PI / 6), FMath::Sin(k * UE_DOUBLE_PI / 6)) * Hw);
			Ccw(C);
			Out.Add(C);
		}
	}
}

void FS47Pix::Cover(const TArray<TArray<FVector2D>>& Polys, TArray<float>& Out, FIntRect& Box) const
{
	double X0 = 1e30, X1 = -1e30, Y0 = 1e30, Y1 = -1e30;
	for (const TArray<FVector2D>& Pg : Polys) for (const FVector2D& Q : Pg) { X0 = FMath::Min(X0, Q.X); X1 = FMath::Max(X1, Q.X); Y0 = FMath::Min(Y0, Q.Y); Y1 = FMath::Max(Y1, Q.Y); }
	Box = FIntRect(FMath::Max(0, FMath::FloorToInt(X0)), FMath::Max(0, FMath::FloorToInt(Y0)), FMath::Min(W, FMath::CeilToInt(X1) + 1), FMath::Min(H, FMath::CeilToInt(Y1) + 1));
	if (Box.Width() <= 0 || Box.Height() <= 0) { Box = FIntRect(); Out.Reset(); return; }
	const int32 BW = Box.Width();
	Out.Init(0.f, BW * Box.Height());
	struct FEdge { double X0, Y0, X1, Y1; int32 Dir; };
	TArray<FEdge> Edges;
	for (const TArray<FVector2D>& Pg : Polys)
	{
		for (int32 i = 0; i < Pg.Num(); ++i)
		{
			const FVector2D& A = Pg[i]; const FVector2D& B = Pg[(i + 1) % Pg.Num()];
			if (A.Y == B.Y) continue;
			Edges.Add({ A.X, A.Y, B.X, B.Y, B.Y > A.Y ? 1 : -1 });
		}
	}
	TArray<TPair<double, int32>> X;
	for (int32 Y = Box.Min.Y; Y < Box.Max.Y; ++Y)
	{
		float* Row = &Out[(Y - Box.Min.Y) * BW];
		for (int32 s = 0; s < SUB; ++s)
		{
			const double Sy = Y + (s + 0.5) / SUB;
			X.Reset();
			for (const FEdge& E : Edges)
			{
				const double Lo = FMath::Min(E.Y0, E.Y1), Hi = FMath::Max(E.Y0, E.Y1);
				if (Sy < Lo || Sy >= Hi) continue;
				X.Add({ E.X0 + (Sy - E.Y0) / (E.Y1 - E.Y0) * (E.X1 - E.X0), E.Dir });
			}
			if (X.Num() < 2) continue;
			X.Sort([](const TPair<double, int32>& A, const TPair<double, int32>& B) { return A.Key < B.Key; });
			int32 Wind = 0;
			for (int32 i = 0; i + 1 < X.Num(); ++i)
			{
				Wind += X[i].Value;
				if (Wind == 0) continue;
				const double Xa = FMath::Max(double(Box.Min.X), X[i].Key), Xb = FMath::Min(double(Box.Max.X), X[i + 1].Key);
				if (Xb <= Xa) continue;
				for (int32 Ix = FMath::FloorToInt(Xa); Ix < Box.Max.X && Ix < Xb; ++Ix)
				{
					const double C = FMath::Min(Xb, double(Ix + 1)) - FMath::Max(Xa, double(Ix));
					if (C > 0) Row[Ix - Box.Min.X] += float(C / SUB);
				}
			}
		}
	}
}

FVector4f FS47Pix::PaintAt(const FPaint& Pt, double X, double Y, const FMat& Inv) const
{
	if (Pt.Kind == FPaint::EKind::Solid) return Premul(Pt.C);
	if (!Pt.Stops.Num()) return FVector4f(0, 0, 0, 0);
	const FVector2D U = Inv.Apply(X, Y);
	double T = 0;
	if (Pt.Kind == FPaint::EKind::Linear)
	{
		const double Dx = Pt.X1 - Pt.X0, Dy = Pt.Y1 - Pt.Y0, L2 = Dx * Dx + Dy * Dy;
		T = L2 > 0 ? ((U.X - Pt.X0) * Dx + (U.Y - Pt.Y0) * Dy) / L2 : 0;
	}
	else
	{
		// the two circles' cone: the largest t whose circle passes through the point with r(t) >= 0
		const double Cdx = Pt.X1 - Pt.X0, Cdy = Pt.Y1 - Pt.Y0, Dr = Pt.R1 - Pt.R0;
		const double Px = U.X - Pt.X0, Py = U.Y - Pt.Y0;
		const double A = Cdx * Cdx + Cdy * Cdy - Dr * Dr, B = Px * Cdx + Py * Cdy + Pt.R0 * Dr, C = Px * Px + Py * Py - Pt.R0 * Pt.R0;
		TOptional<double> Best;
		auto Try = [&](double V) { if (Pt.R0 + V * Dr >= 0 && (!Best || V > *Best)) Best = V; };
		if (FMath::Abs(A) < 1e-12) { if (FMath::Abs(B) > 1e-12) Try(C / (2 * B)); }
		else
		{
			const double Disc = B * B - A * C;
			if (Disc >= 0) { const double Sq = FMath::Sqrt(Disc); Try((B + Sq) / A); Try((B - Sq) / A); }
		}
		if (!Best) return FVector4f(0, 0, 0, 0);
		T = *Best;
	}
	T = FMath::Clamp(T, 0.0, 1.0);
	const TArray<TPair<double, FVector4f>>& S = Pt.Stops;
	if (T <= S[0].Key) return Premul(S[0].Value);
	for (int32 i = 1; i < S.Num(); ++i)
	{
		if (T <= S[i].Key)
		{
			const double K = S[i].Key > S[i - 1].Key ? (T - S[i - 1].Key) / (S[i].Key - S[i - 1].Key) : 1;
			const FVector4f A = Premul(S[i - 1].Value), B = Premul(S[i].Value);
			return A + (B - A) * float(K);
		}
	}
	return Premul(S.Last().Value);
}

void FS47Pix::ApplyFilters(TArray<FVector4f>& L, FIntRect& Box) const
{
	for (const TPair<EFilter, double>& F : Filter)
	{
		if (F.Key == EFilter::Blur)
		{
			const double Sigma = F.Value;
			if (Sigma <= 0) continue;
			const int32 R = FMath::CeilToInt(Sigma * 3);
			const FIntRect Nb(FMath::Max(0, Box.Min.X - R), FMath::Max(0, Box.Min.Y - R), FMath::Min(W, Box.Max.X + R), FMath::Min(H, Box.Max.Y + R));
			const int32 NW = Nb.Width(), NH = Nb.Height();
			TArray<FVector4f> Big;
			Big.Init(FVector4f(0, 0, 0, 0), NW * NH);
			for (int32 y = Box.Min.Y; y < Box.Max.Y; ++y) for (int32 x = Box.Min.X; x < Box.Max.X; ++x) Big[(y - Nb.Min.Y) * NW + (x - Nb.Min.X)] = L[(y - Box.Min.Y) * Box.Width() + (x - Box.Min.X)];
			TArray<float> K;
			float Sum = 0;
			for (int32 i = -R; i <= R; ++i) { const float V = FMath::Exp(-float(i * i) / float(2 * Sigma * Sigma)); K.Add(V); Sum += V; }
			for (float& V : K) V /= Sum;
			TArray<FVector4f> Tmp;
			Tmp.Init(FVector4f(0, 0, 0, 0), NW * NH);
			for (int32 y = 0; y < NH; ++y) for (int32 x = 0; x < NW; ++x)
			{
				FVector4f A(0, 0, 0, 0);
				for (int32 i = -R; i <= R; ++i) { const int32 Xx = x + i; if (Xx >= 0 && Xx < NW) A += Big[y * NW + Xx] * K[i + R]; }
				Tmp[y * NW + x] = A;
			}
			for (int32 y = 0; y < NH; ++y) for (int32 x = 0; x < NW; ++x)
			{
				FVector4f A(0, 0, 0, 0);
				for (int32 i = -R; i <= R; ++i) { const int32 Yy = y + i; if (Yy >= 0 && Yy < NH) A += Tmp[Yy * NW + x] * K[i + R]; }
				Big[y * NW + x] = A;
			}
			L = MoveTemp(Big);
			Box = Nb;
			continue;
		}
		// the colour filters, on the straight colour, clamped after each
		const double A = F.Value;
		for (FVector4f& C : L)
		{
			FVector4f S = Unpremul(C);
			if (S.W <= 0) continue;
			double R = S.X, G = S.Y, B = S.Z;
			if (F.Key == EFilter::Grayscale)
			{
				const double K = 1 - FMath::Clamp(A, 0.0, 1.0);
				const double Nr = (0.2126 + 0.7874 * K) * R + (0.7152 - 0.7152 * K) * G + (0.0722 - 0.0722 * K) * B;
				const double Ng = (0.2126 - 0.2126 * K) * R + (0.7152 + 0.2848 * K) * G + (0.0722 - 0.0722 * K) * B;
				const double Nb2 = (0.2126 - 0.2126 * K) * R + (0.7152 - 0.7152 * K) * G + (0.0722 + 0.9278 * K) * B;
				R = Nr; G = Ng; B = Nb2;
			}
			else if (F.Key == EFilter::Contrast) { R = (R - 0.5) * A + 0.5; G = (G - 0.5) * A + 0.5; B = (B - 0.5) * A + 0.5; }
			else if (F.Key == EFilter::Brightness) { R *= A; G *= A; B *= A; }
			S.X = float(FMath::Clamp(R, 0.0, 1.0)); S.Y = float(FMath::Clamp(G, 0.0, 1.0)); S.Z = float(FMath::Clamp(B, 0.0, 1.0));
			C = Premul(S);
		}
	}
}

void FS47Pix::Layer(TArray<FVector4f>& L, const FIntRect& InBox)
{
	// the drawn layer through the filter, the clip and the composite, onto the pixels
	FIntRect Box = InBox;
	if (Filter.Num()) ApplyFilters(L, Box);
	const int32 BW = Box.Width();
	const TArray<float>* Clip = ClipMask.IsValid() ? ClipMask.Get() : nullptr;
	for (int32 y = Box.Min.Y; y < Box.Max.Y; ++y) for (int32 x = Box.Min.X; x < Box.Max.X; ++x)
	{
		FVector4f S = L[(y - Box.Min.Y) * BW + (x - Box.Min.X)];
		if (Clip) S *= (*Clip)[y * W + x];
		if (S.W <= 0 && S.X <= 0 && S.Y <= 0 && S.Z <= 0) continue;
		FVector4f& D = P[y * W + x];
		if (bLighter) D = FVector4f(FMath::Min(1.f, S.X + D.X), FMath::Min(1.f, S.Y + D.Y), FMath::Min(1.f, S.Z + D.Z), FMath::Min(1.f, S.W + D.W));
		else D = S + D * (1.f - S.W);
	}
}

void FS47Pix::Composite(const TArray<float>& Cov, const FIntRect& Box, const FPaint& Pt)
{
	if (Box.Width() <= 0 || Box.Height() <= 0) return;
	const FMat Inv = M.Inverse();
	const int32 BW = Box.Width();
	TArray<FVector4f> L;
	L.SetNumUninitialized(BW * Box.Height());
	for (int32 y = Box.Min.Y; y < Box.Max.Y; ++y) for (int32 x = Box.Min.X; x < Box.Max.X; ++x)
	{
		const int32 i = (y - Box.Min.Y) * BW + (x - Box.Min.X);
		const float C = FMath::Min(1.f, Cov[i]);
		L[i] = C > 0 ? PaintAt(Pt, x + 0.5, y + 0.5, Inv) * float(C * GlobalAlpha) : FVector4f(0, 0, 0, 0);
	}
	Layer(L, Box);
}

void FS47Pix::Fill()
{
	TArray<TArray<FVector2D>> Pg;
	Polys(Pg, false);
	if (!Pg.Num()) return;
	TArray<float> Cov; FIntRect Box;
	Cover(Pg, Cov, Box);
	Composite(Cov, Box, FillStyle);
}

void FS47Pix::Stroke()
{
	TArray<TArray<FVector2D>> Pg;
	Polys(Pg, true);
	if (!Pg.Num()) return;
	TArray<float> Cov; FIntRect Box;
	Cover(Pg, Cov, Box);
	Composite(Cov, Box, StrokeStyle);
}

void FS47Pix::Clip()
{
	TArray<TArray<FVector2D>> Pg;
	Polys(Pg, false);
	TArray<float> Cov; FIntRect Box;
	if (Pg.Num()) Cover(Pg, Cov, Box);
	TSharedPtr<TArray<float>> Mask = MakeShared<TArray<float>>();
	Mask->Init(0.f, W * H);
	for (int32 y = Box.Min.Y; y < Box.Max.Y; ++y) for (int32 x = Box.Min.X; x < Box.Max.X; ++x)
	{
		const float C = FMath::Min(1.f, Cov[(y - Box.Min.Y) * Box.Width() + (x - Box.Min.X)]);
		(*Mask)[y * W + x] = C * (ClipMask.IsValid() ? (*ClipMask)[y * W + x] : 1.f);
	}
	ClipMask = Mask;
}

void FS47Pix::FillRect(double X, double Y, double Wd, double Ht)
{
	// (a solid rectangle with no turn, filter or clip, as the grain's thousands are: its coverage straight
	// off the pixels, the same as the path would give)
	if (FillStyle.Kind == FPaint::EKind::Solid && !Filter.Num() && !ClipMask.IsValid() && M.B == 0 && M.C == 0 && Wd > 0 && Ht > 0)
	{
		const double X0 = M.A * X + M.E, X1 = M.A * (X + Wd) + M.E, Y0 = M.D * Y + M.F, Y1 = M.D * (Y + Ht) + M.F;
		const double Lx = FMath::Min(X0, X1), Rx = FMath::Max(X0, X1), Ty = FMath::Min(Y0, Y1), By = FMath::Max(Y0, Y1);
		const FIntRect Box(FMath::Max(0, FMath::FloorToInt(Lx)), FMath::Max(0, FMath::FloorToInt(Ty)), FMath::Min(W, FMath::CeilToInt(Rx)), FMath::Min(H, FMath::CeilToInt(By)));
		if (Box.Width() <= 0 || Box.Height() <= 0) return;
		TArray<FVector4f> L;
		L.SetNumUninitialized(Box.Width() * Box.Height());
		const FVector4f C = FVector4f(FillStyle.C.X * FillStyle.C.W, FillStyle.C.Y * FillStyle.C.W, FillStyle.C.Z * FillStyle.C.W, FillStyle.C.W);
		for (int32 y = Box.Min.Y; y < Box.Max.Y; ++y)
		{
			// (the path's sub-scanlines: a row is covered by the share of its four that fall inside)
			int32 In = 0;
			for (int32 s = 0; s < SUB; ++s) { const double Sy = y + (s + 0.5) / SUB; In += Sy >= Ty && Sy < By ? 1 : 0; }
			for (int32 x = Box.Min.X; x < Box.Max.X; ++x)
			{
				const double Cx = FMath::Max(0.0, FMath::Min(Rx, double(x + 1)) - FMath::Max(Lx, double(x)));
				L[(y - Box.Min.Y) * Box.Width() + (x - Box.Min.X)] = C * float(FMath::Min(1.0, Cx * In / SUB) * GlobalAlpha);
			}
		}
		Layer(L, Box);
		return;
	}
	// (a path of its own, the current one kept)
	const TArray<FSub> Keep = Subs;
	Subs.Reset();
	MoveTo(X, Y); LineTo(X + Wd, Y); LineTo(X + Wd, Y + Ht); LineTo(X, Y + Ht); ClosePath();
	Fill();
	Subs = Keep;
}

void FS47Pix::ClearRect(int32 X, int32 Y, int32 Wd, int32 Ht)
{
	for (int32 y = FMath::Max(0, Y); y < FMath::Min(H, Y + Ht); ++y) for (int32 x = FMath::Max(0, X); x < FMath::Min(W, X + Wd); ++x) P[y * W + x] = FVector4f(0, 0, 0, 0);
}

void FS47Pix::DrawImage(const FS47Pix& Src, double Sx, double Sy, double Sw, double Sh, double Dx, double Dy, double Dw, double Dh)
{
	if (Sw == 0 || Sh == 0 || Dw == 0 || Dh == 0) return;
	const FVector2D C[4] = { M.Apply(Dx, Dy), M.Apply(Dx + Dw, Dy), M.Apply(Dx + Dw, Dy + Dh), M.Apply(Dx, Dy + Dh) };
	double X0 = 1e30, X1 = -1e30, Y0 = 1e30, Y1 = -1e30;
	for (const FVector2D& Q : C) { X0 = FMath::Min(X0, Q.X); X1 = FMath::Max(X1, Q.X); Y0 = FMath::Min(Y0, Q.Y); Y1 = FMath::Max(Y1, Q.Y); }
	const FIntRect Box(FMath::Max(0, FMath::FloorToInt(X0)), FMath::Max(0, FMath::FloorToInt(Y0)), FMath::Min(W, FMath::CeilToInt(X1) + 1), FMath::Min(H, FMath::CeilToInt(Y1) + 1));
	if (Box.Width() <= 0 || Box.Height() <= 0) return;
	const FMat Inv = M.Inverse();
	const int32 BW = Box.Width();
	TArray<FVector4f> L;
	L.Init(FVector4f(0, 0, 0, 0), BW * Box.Height());
	// the source rectangle, sampled bilinearly and clamped to its own edges (as a browser smooths it)
	const double SxA = FMath::Clamp(Sx, 0.0, double(Src.W)), SyA = FMath::Clamp(Sy, 0.0, double(Src.H));
	const double SxB = FMath::Clamp(Sx + Sw, 0.0, double(Src.W)), SyB = FMath::Clamp(Sy + Sh, 0.0, double(Src.H));
	auto Px = [&](int32 X, int32 Y) { return Src.P[FMath::Clamp(Y, 0, Src.H - 1) * Src.W + FMath::Clamp(X, 0, Src.W - 1)]; };
	for (int32 y = Box.Min.Y; y < Box.Max.Y; ++y) for (int32 x = Box.Min.X; x < Box.Max.X; ++x)
	{
		const FVector2D U = Inv.Apply(x + 0.5, y + 0.5);
		const double Kx = (U.X - Dx) / Dw, Ky = (U.Y - Dy) / Dh;
		if (Kx < 0 || Kx >= 1 || Ky < 0 || Ky >= 1) continue;
		const double Su = FMath::Clamp(Sx + Kx * Sw, SxA + 0.5, FMath::Max(SxA + 0.5, SxB - 0.5)) - 0.5;
		const double Sv = FMath::Clamp(Sy + Ky * Sh, SyA + 0.5, FMath::Max(SyA + 0.5, SyB - 0.5)) - 0.5;
		const int32 Ix = FMath::FloorToInt(Su), Iy = FMath::FloorToInt(Sv);
		const float Fx = float(Su - Ix), Fy = float(Sv - Iy);
		const FVector4f A = Px(Ix, Iy) * (1 - Fx) + Px(Ix + 1, Iy) * Fx, B = Px(Ix, Iy + 1) * (1 - Fx) + Px(Ix + 1, Iy + 1) * Fx;
		L[(y - Box.Min.Y) * BW + (x - Box.Min.X)] = (A * (1 - Fy) + B * Fy) * float(GlobalAlpha);
	}
	Layer(L, Box);
}

bool FS47Pix::Text(const FString& S, double X, double Y, const TCHAR* FontFile, double SizePx)
{
	// (Slate draws the glyphs: S47Canvas fillText, onto these pixels as they stand)
	FS47Canvas C(W, H);
	C.Px = Colors();
	const FVector4f F = FillStyle.C;
	C.SetFill((uint32(Clamp8(F.X * 255.0)) << 16) | (uint32(Clamp8(F.Y * 255.0)) << 8) | uint32(Clamp8(F.Z * 255.0)), F.W * GlobalAlpha);
	C.SetFont(FontFile, SizePx);
	const bool bOk = C.FillText(S, X, Y);
	if (bOk) SetColors(C.Px);
	return bOk;
}

// ---------- the pixels ----------

uint8 FS47Pix::Clamp8(double V)
{
	if (!(V > 0)) return 0;
	if (V >= 255) return 255;
	const double F = FMath::FloorToDouble(V), D = V - F;
	double R = D > 0.5 ? F + 1 : D < 0.5 ? F : (FMath::Fmod(F, 2.0) == 0 ? F : F + 1);
	return uint8(R);
}

TArray<FColor> FS47Pix::Colors() const
{
	TArray<FColor> Out;
	Out.SetNumUninitialized(W * H);
	for (int32 i = 0; i < W * H; ++i)
	{
		const FVector4f S = Unpremul(P[i]);
		Out[i] = FColor(Clamp8(S.X * 255.0), Clamp8(S.Y * 255.0), Clamp8(S.Z * 255.0), Clamp8(S.W * 255.0));
	}
	return Out;
}

void FS47Pix::SetColors(const TArray<FColor>& C)
{
	for (int32 i = 0; i < W * H && i < C.Num(); ++i) P[i] = Premul(FVector4f(C[i].R / 255.f, C[i].G / 255.f, C[i].B / 255.f, C[i].A / 255.f));
}

FS47Pix FS47Pix::FromColors(const TArray<FColor>& C, int32 InW, int32 InH)
{
	FS47Pix Out(InW, InH);
	Out.SetColors(C);
	return Out;
}

// ---------- the art ----------

namespace
{
	TMap<FString, TSharedPtr<FS47Pix>> ArtPix;
	TSet<FString> ArtMissing;
}

bool S47Raster::Art(const FString& Name, FS47Pix& Out)
{
	if (const TSharedPtr<FS47Pix>* Have = ArtPix.Find(Name)) { Out = **Have; return true; }
	if (ArtMissing.Contains(Name)) return false;
	for (const TCHAR* Ext : { TEXT(".png"), TEXT(".jpg") })
	{
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *(FPaths::ProjectContentDir() / TEXT("WebData/art") / (Name + Ext)))) continue;
		IImageWrapperModule& Mod = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		const EImageFormat F = Mod.DetectImageFormat(Bytes.GetData(), Bytes.Num());
		if (F == EImageFormat::Invalid) continue;
		TSharedPtr<IImageWrapper> Wr = Mod.CreateImageWrapper(F);
		TArray<uint8> Raw;
		if (!Wr.IsValid() || !Wr->SetCompressed(Bytes.GetData(), Bytes.Num()) || !Wr->GetRaw(ERGBFormat::BGRA, 8, Raw)) continue;
		const int32 Wd = int32(Wr->GetWidth()), Ht = int32(Wr->GetHeight());
		TArray<FColor> Px;
		Px.SetNumUninitialized(Wd * Ht);
		FMemory::Memcpy(Px.GetData(), Raw.GetData(), Px.Num() * sizeof(FColor));
		TSharedPtr<FS47Pix> P = MakeShared<FS47Pix>(FS47Pix::FromColors(Px, Wd, Ht));
		ArtPix.Add(Name, P);
		Out = *P;
		return true;
	}
	ArtMissing.Add(Name);
	return false;
}

bool S47Raster::ArtLoaded(const FString& Name)
{
	if (ArtPix.Contains(Name)) return true;
	if (ArtMissing.Contains(Name)) return false;
	for (const TCHAR* Ext : { TEXT(".png"), TEXT(".jpg") }) if (FPaths::FileExists(FPaths::ProjectContentDir() / TEXT("WebData/art") / (Name + Ext))) return true;
	return false;
}

// ---------- the self test ----------

bool FS47Pix::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s raster: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bCond; };
	FS47Pix G(20, 10);
	G.SetFill(0x000000); G.FillRect(0, 0, 20, 10);
	G.SetFill(0xffffff, 0.5); G.FillRect(2.5, 2, 1, 1);
	const TArray<FColor> C = G.Colors();
	// half of each of the two pixels the rect straddles, at half alpha: a quarter of white over black
	Check(C[2 * 20 + 2].R == 64 && C[2 * 20 + 3].R == 64 && C[2 * 20 + 4].R == 0 && C[0].A == 255, TEXT("a fractional rectangle's coverage"));
	FS47Pix R(64, 64);
	FPaint Pt = Radial(32, 32, 0, 32, 32, 32);
	Pt.Stop(0, 0xffffff).Stop(1, 0x000000);
	R.SetFill(Pt); R.FillRect(0, 0, 64, 64);
	const TArray<FColor> Rc = R.Colors();
	// (sampled at the pixels' middles: 0.7 and 16.5 px from the centre of a 32 px radius)
	Check(Rc[32 * 64 + 32].R == 249 && Rc[0].R == 0 && FMath::Abs(int32(Rc[32 * 64 + 48].R) - 123) <= 1, TEXT("a radial gradient's stops"));
	FS47Pix B(30, 30);
	B.SetFill(0xffffff); B.Filter = { { EFilter::Blur, 2.0 } }; B.FillRect(10, 10, 10, 10);
	double Sum = 0;
	for (const FVector4f& V : B.P) Sum += V.W;
	Check(FMath::Abs(Sum - 100) < 1.0 && B.P[10 * 30 + 9].W > 0.05f && B.P[10 * 30 + 9].W < 0.5f, TEXT("blur keeps what was drawn and softens its edge"));
	FS47Pix Src(2, 1);
	Src.P[0] = FVector4f(1, 0, 0, 1); Src.P[1] = FVector4f(0, 0, 1, 1);
	FS47Pix D(4, 1);
	D.DrawImage(Src, 0, 0, 4, 1);
	const TArray<FColor> Dc = D.Colors();
	Check(Dc[0].R == 255 && Dc[0].B == 0 && Dc[3].B == 255 && Dc[1].R > Dc[2].R, TEXT("drawImage scales bilinearly"));
	S47Raster::FRng Rn(47);
	Check(FMath::Abs(Rn() - double(uint32(47u * 1664525u + 1013904223u)) / 4294967296.0) < 1e-12, TEXT("the seeded generator"));
	Check(Clamp8(2.5) == 2 && Clamp8(3.5) == 4 && Clamp8(-1) == 0 && Clamp8(300) == 255, TEXT("Uint8ClampedArray rounding"));
	return bOk;
}

// core/filmFace.ts
#include "S47FilmFace.h"

#include "S47Canvas.h"
#include "S47FieldCamera.h"

namespace
{
	const double DW = 640, DH = 400;
	/** The layer the head is worked out on: half the frame, which the blur hides. */
	const int32 LW = 320, LH = 200;
	const double STEP = DW / LW;

	/** lcg(seed): seed >>> 0 || 1, then s = (s * 1664525 + 1013904223) >>> 0, s / 2^32 */
	struct FLcg
	{
		uint32 S;
		explicit FLcg(double Seed) { S = US47FieldCamera::ToUint32(Seed); if (!S) S = 1; }
		double operator()() { S = S * 1664525u + 1013904223u; return S / 4294967296.0; }
	};

	/** Smooth value noise, 0 to 1, repeating every 64 units (its table a Float32Array). */
	struct FNoise
	{
		TArray<float> T;
		explicit FNoise(double Seed)
		{
			FLcg R(Seed);
			T.SetNumUninitialized(64 * 64);
			for (float& V : T) V = float(R());
		}
		double operator()(double X, double Y) const
		{
			const double Xf = FMath::FloorToDouble(X), Yf = FMath::FloorToDouble(Y), Fx = X - Xf, Fy = Y - Yf;
			const int32 Xi = int32(Xf), Yi = int32(Yf);
			const int32 X0 = Xi & 63, X1 = (Xi + 1) & 63, Y0 = (Yi & 63) * 64, Y1 = ((Yi + 1) & 63) * 64;
			const double A = T[Y0 + X0], B = T[Y0 + X1], C = T[Y1 + X0], D = T[Y1 + X1];
			const double Sx = Fx * Fx * (3 - 2 * Fx), Sy = Fy * Fy * (3 - 2 * Fy);
			return A + (B - A) * Sx + (C - A) * Sy + (A - B - C + D) * Sx * Sy;
		}
	};

	double Clamp01(double V) { return V < 0 ? 0 : V > 1 ? 1 : V; }
	double Smooth(double A, double B, double V) { const double T = Clamp01((V - A) / (B - A)); return T * T * (3 - 2 * T); }

	// The head, upright in its own frame, at the right edge (its middle, MID, just past it).
	const double MID = 684;
	struct { double Y, Rx, Ry, Rz; } const SKULL = { 70, 206, 214, 190 };
	struct { double Y, Rx, Ry, Rz, Z, Taper; } const FACE = { 236, 156, 172, 150, -10, 0.55 };
	/** Below the eye the face goes back from the lens, so much per unit down. */
	struct { double Y, K; } const RECEDE = { 200, 0.7 };
	// It leans in from the edge, the top of the head towards the middle of the frame, as if to see.
	struct { double X, Y, A; } const LEAN = { 620, 430, -0.13 };
	// The eye: its middle, its length and height, and its slant; the socket, the brow, the cheekbone.
	struct { double X, Y, L, H, Tilt; } const EYE = { 594, 182, 126, 58, 0.4 };
	const double SOCKET = 12, BROW = 14, CHEEK = 14;
	/** The flash, a foot from the face, which is turned to it. */
	struct { double X, Y, Z; } const CAM = { 640, 150, -320 };
	const double FLASH = 3.2, SKIN[3] = { 0.74, 0.81, 0.83 };

	FVector2D Unlean(double X, double Y)
	{
		const double C = FMath::Cos(LEAN.A), S = FMath::Sin(LEAN.A), Dx = X - LEAN.X, Dy = Y - LEAN.Y;
		return FVector2D(LEAN.X + Dx * C + Dy * S, LEAN.Y - Dx * S + Dy * C);
	}
	FVector2D Lean(double X, double Y)
	{
		const double C = FMath::Cos(LEAN.A), S = FMath::Sin(LEAN.A), Dx = X - LEAN.X, Dy = Y - LEAN.Y;
		return FVector2D(LEAN.X + Dx * C - Dy * S, LEAN.Y + Dx * S + Dy * C);
	}

	/** The head's depth at an upright point (further from the lens is larger), and how much of it is there. */
	FVector2D Depth(double Hx, double Hy, const FNoise& Wr)
	{
		const double U1 = (Hx - MID) / SKULL.Rx, V1 = (Hy - SKULL.Y) / SKULL.Ry, T1 = 1 - U1 * U1 - V1 * V1;
		const double K = Clamp01((Hy - FACE.Y) / FACE.Ry), Rxf = FACE.Rx * (1 - FACE.Taper * K * K);
		const double U2 = (Hx - MID) / Rxf, V2 = (Hy - FACE.Y) / FACE.Ry, T2 = 1 - U2 * U2 - V2 * V2;
		const double Cov = Smooth(0, 0.035, FMath::Max(T1, T2));
		if (Cov <= 0) return FVector2D(0, 0);
		const double Z1 = T1 > 0 ? SKULL.Rz - SKULL.Rz * FMath::Sqrt(T1) : 1e4;
		const double Z2 = T2 > 0 ? FACE.Rz + FACE.Z - FACE.Rz * FMath::Sqrt(T2) : 1e4;
		// the skull and the face run into each other without a seam
		const double H = Clamp01(0.5 + 0.5 * (Z2 - Z1) / 30);
		double Z = Z2 + (Z1 - Z2) * H - 30 * H * (1 - H);
		Z += FMath::Max(0.0, Hy - RECEDE.Y) * RECEDE.K;
		// the eye's socket, the brow over it, the cheekbone under it, a few soft folds round it
		const double C = FMath::Cos(EYE.Tilt), S = FMath::Sin(EYE.Tilt), Ex = Hx - EYE.X, Ey = Hy - EYE.Y;
		const double Lu = Ex * C + Ey * S, Lv = -Ex * S + Ey * C;
		const double R = FMath::Sqrt(FMath::Square(Lu / (EYE.L * 0.64)) + FMath::Square(Lv / (EYE.H * 1.2)));
		Z += SOCKET * (1 - Smooth(0.42, 1.3, R));
		Z -= BROW * FMath::Exp(-FMath::Square((Lv + EYE.H * 1.05) / (EYE.H * 0.5)) - FMath::Square(Lu / (EYE.L * 0.62)));
		Z -= CHEEK * FMath::Exp(-FMath::Square((Lv - EYE.H * 1.25) / (EYE.H * 0.6)) - FMath::Square((Lu + EYE.L * 0.08) / (EYE.L * 0.55)));
		if (R > 0.9 && R < 1.9) Z += 1.6 * FMath::Sin((R - 0.9) * 19) * (1 - Smooth(1.2, 1.9, R));
		// and the skin is not quite smooth
		Z += (Wr(Hx / 38, Hy / 38) - 0.5) * 5;
		return FVector2D(Z, Cov);
	}

	// ---------- a little of the canvas: RGBA bytes as ImageData has them ----------
	struct FFaceImage
	{
		int32 W = 0, H = 0;
		TArray<uint8> D; // r, g, b, a (not premultiplied)
		FFaceImage(int32 InW, int32 InH) : W(InW), H(InH) { D.SetNumZeroed(W * H * 4); }
		/** source-over of a colour with coverage A at a pixel */
		void Over(int32 X, int32 Y, double R, double G, double B, double A)
		{
			if (X < 0 || Y < 0 || X >= W || Y >= H || A <= 0) return;
			uint8* P = &D[(Y * W + X) * 4];
			const double Da = P[3] / 255.0, Oa = A + Da * (1 - A);
			if (Oa <= 0) return;
			const double Cs[3] = { R, G, B };
			for (int32 K = 0; K < 3; ++K) P[K] = FS47Canvas::Clamp8((Cs[K] * A + P[K] * Da * (1 - A)) / Oa);
			P[3] = FS47Canvas::Clamp8(Oa * 255);
		}
		/** a premultiplied sample, bilinear, edges clamped (drawImage with smoothing) */
		FVector4 Sample(double X, double Y) const
		{
			X = FMath::Clamp(X, 0.0, W - 1.0); Y = FMath::Clamp(Y, 0.0, H - 1.0);
			const int32 X0 = FMath::FloorToInt32(X), Y0 = FMath::FloorToInt32(Y), X1 = FMath::Min(X0 + 1, W - 1), Y1 = FMath::Min(Y0 + 1, H - 1);
			const double Fx = X - X0, Fy = Y - Y0;
			auto At = [this](int32 Xi, int32 Yi) { const uint8* P = &D[(Yi * W + Xi) * 4]; const double A = P[3] / 255.0; return FVector4(P[0] * A, P[1] * A, P[2] * A, A); };
			return (At(X0, Y0) * (1 - Fx) + At(X1, Y0) * Fx) * (1 - Fy) + (At(X0, Y1) * (1 - Fx) + At(X1, Y1) * Fx) * Fy;
		}
	};

	/** Three box blurs, rows then columns, on premultiplied RGBA inside [x0, x1) x [0, h). */
	void Blur(FFaceImage& Img, int32 Rad, int32 X0, int32 X1)
	{
		const int32 W = Img.W, H = Img.H, Wd = X1 - X0;
		if (Rad < 1 || Wd <= 0 || H <= 0) return;
		TArray<float> Buf; Buf.SetNumUninitialized(Wd * H * 4);
		for (int32 Y = 0; Y < H; ++Y) for (int32 X = 0; X < Wd; ++X)
		{
			const int32 I = (Y * W + X + X0) * 4, J = (Y * Wd + X) * 4;
			const double A = Img.D[I + 3] / 255.0;
			Buf[J] = float(Img.D[I] * A); Buf[J + 1] = float(Img.D[I + 1] * A); Buf[J + 2] = float(Img.D[I + 2] * A); Buf[J + 3] = Img.D[I + 3];
		}
		TArray<float> Tmp; Tmp.SetNumZeroed(FMath::Max(Wd, H) * 4);
		auto Pass = [&](int32 Len, int32 Count, TFunctionRef<int32(int32, int32)> At)
		{
			const int32 R = FMath::Min(Rad, Len - 1);
			const double Inv = 1.0 / (2 * R + 1);
			for (int32 N = 0; N < Count; ++N)
			{
				for (int32 C = 0; C < 4; ++C)
				{
					// edges clamped: the layer is empty there, or the head runs on off the frame
					double Sum = Buf[At(N, 0) + C] * (R + 1);
					for (int32 K = 1; K <= R; ++K) Sum += Buf[At(N, FMath::Min(Len - 1, K)) + C];
					for (int32 K = 0; K < Len; ++K)
					{
						Tmp[K * 4 + C] = float(Sum * Inv);
						Sum += Buf[At(N, FMath::Min(Len - 1, K + R + 1)) + C] - Buf[At(N, FMath::Max(0, K - R)) + C];
					}
				}
				for (int32 K = 0; K < Len; ++K) { const int32 O = At(N, K); Buf[O] = Tmp[K * 4]; Buf[O + 1] = Tmp[K * 4 + 1]; Buf[O + 2] = Tmp[K * 4 + 2]; Buf[O + 3] = Tmp[K * 4 + 3]; }
			}
		};
		for (int32 P = 0; P < 3; ++P)
		{
			Pass(Wd, H, [Wd](int32 Row, int32 K) { return (Row * Wd + K) * 4; });
			Pass(H, Wd, [Wd](int32 Col, int32 K) { return (K * Wd + Col) * 4; });
		}
		for (int32 Y = 0; Y < H; ++Y) for (int32 X = 0; X < Wd; ++X)
		{
			const int32 I = (Y * W + X + X0) * 4, J = (Y * Wd + X) * 4;
			const double A = Buf[J + 3], K = A > 0.5 ? 255 / A : 0;
			Img.D[I] = FS47Canvas::Clamp8(FMath::Min(255.0, Buf[J] * K)); Img.D[I + 1] = FS47Canvas::Clamp8(FMath::Min(255.0, Buf[J + 1] * K)); Img.D[I + 2] = FS47Canvas::Clamp8(FMath::Min(255.0, Buf[J + 2] * K));
			Img.D[I + 3] = FS47Canvas::Clamp8(A);
		}
	}

	// ---------- the eye's shapes, in its own frame (the outer end at -L/2, y down) ----------
	void Bezier(TArray<FVector2D>& Out, const FVector2D& P0, const FVector2D& C1, const FVector2D& C2, const FVector2D& P3, int32 N = 24)
	{
		for (int32 I = 1; I <= N; ++I)
		{
			const double T = double(I) / N, U = 1 - T;
			Out.Add(P0 * (U * U * U) + C1 * (3 * U * U * T) + C2 * (3 * U * T * T) + P3 * (T * T * T));
		}
	}
	TArray<FVector2D> EyePath(double L, double H)
	{
		TArray<FVector2D> P = { FVector2D(-L / 2, 0) };
		Bezier(P, P.Last(), FVector2D(-L * 0.22, -H * 0.64), FVector2D(L * 0.2, -H * 0.8), FVector2D(L * 0.42, -H * 0.4));
		Bezier(P, P.Last(), FVector2D(L * 0.56, -H * 0.16), FVector2D(L * 0.55, H * 0.32), FVector2D(L * 0.33, H * 0.45));
		Bezier(P, P.Last(), FVector2D(L * 0.06, H * 0.6), FVector2D(-L * 0.26, H * 0.34), FVector2D(-L / 2, 0));
		return P;
	}
	bool Inside(const TArray<FVector2D>& Poly, const FVector2D& Q)
	{
		// nonzero winding (canvas fill's default)
		int32 Wn = 0;
		for (int32 I = 0, J = Poly.Num() - 1; I < Poly.Num(); J = I++)
		{
			const FVector2D& A = Poly[J]; const FVector2D& B = Poly[I];
			const double Cr = (B.X - A.X) * (Q.Y - A.Y) - (Q.X - A.X) * (B.Y - A.Y);
			if (A.Y <= Q.Y) { if (B.Y > Q.Y && Cr > 0) ++Wn; }
			else if (B.Y <= Q.Y && Cr < 0) --Wn;
		}
		return Wn != 0;
	}
	double SegDist(const FVector2D& Q, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D Ab = B - A;
		const double T = FMath::Clamp(FVector2D::DotProduct(Q - A, Ab) / FMath::Max(1e-12, Ab.SizeSquared()), 0.0, 1.0);
		return (Q - (A + Ab * T)).Size();
	}
	/** createRadialGradient(x0, y0, r0, x1, y1, r1) at a point: the largest t with |q - c(t)| = r(t), r(t) >= 0 (unset: nothing painted) */
	TOptional<double> Radial(const FVector2D& Q, const FVector2D& C0, double R0, const FVector2D& C1, double R1)
	{
		const FVector2D Cd = C1 - C0, Pd = Q - C0;
		const double Dr = R1 - R0;
		const double A = Cd.SizeSquared() - Dr * Dr, B = FVector2D::DotProduct(Pd, Cd) + R0 * Dr, C = Pd.SizeSquared() - R0 * R0;
		TArray<double> Ts;
		if (FMath::Abs(A) < 1e-12) { if (FMath::Abs(B) > 1e-12) Ts.Add(C / (2 * B)); }
		else
		{
			const double Disc = B * B - A * C;
			if (Disc < 0) return {};
			const double S = FMath::Sqrt(Disc);
			Ts.Add((B + S) / A); Ts.Add((B - S) / A);
		}
		Ts.Sort([](double X, double Y) { return X > Y; });
		for (const double T : Ts) if (R0 + T * Dr >= 0) return T;
		return {};
	}
	struct FStop { double At; double R, G, B, A; };
	/** a gradient's colour at t, padded (non-premultiplied stops interpolated premultiplied) */
	FVector4 StopsAt(const TArray<FStop>& S, double T)
	{
		T = Clamp01(T);
		int32 I = 0;
		while (I + 1 < S.Num() - 1 && T > S[I + 1].At) ++I;
		const FStop& A = S[I]; const FStop& B = S[FMath::Min(I + 1, S.Num() - 1)];
		const double K = B.At > A.At ? Clamp01((T - A.At) / (B.At - A.At)) : 0;
		const double Al = A.A + (B.A - A.A) * K;
		auto Ch = [&](double Ca, double Cb) { const double P = Ca * A.A + (Cb * B.A - Ca * A.A) * K; return Al > 0 ? P / Al : 0.0; };
		return FVector4(Ch(A.R, B.R), Ch(A.G, B.G), Ch(A.B, B.B), Al);
	}
}

namespace S47FilmFace
{
	FVector2D FilmFaceEye(int32 Side)
	{
		const FVector2D P = Lean(EYE.X, EYE.Y);
		return FVector2D(Side > 0 ? P.X / DW : 1 - P.X / DW, P.Y / DH);
	}

	void FilmFace(TArray<FColor>& Raw, int32 W, int32 H, double Seed, int32 Side)
	{
		if (W <= 0 || H <= 0 || Raw.Num() < W * H) return;
		const FNoise Wr(Seed * 2654435761.0), Mot(Seed * 40503 + 7);
		FFaceImage Layer(LW, LH);
		/** The layer's columns the head can reach, and how far out of focus it is (on the layer). */
		const int32 X0 = FMath::FloorToInt32(380 / STEP), BLUR = 3;

		// ---- the head, as deep as it is, then lit by the flash ----
		TArray<float> Zs, Cs;
		Zs.SetNumZeroed(LW * LH); Cs.SetNumZeroed(LW * LH);
		for (int32 Ly = 0; Ly < LH; ++Ly) for (int32 Lx = X0; Lx < LW; ++Lx)
		{
			const FVector2D Hp = Unlean((Lx + 0.5) * STEP, (Ly + 0.5) * STEP);
			const FVector2D Zc = Depth(Hp.X, Hp.Y, Wr);
			Zs[Ly * LW + Lx] = float(Zc.X); Cs[Ly * LW + Lx] = float(Zc.Y);
		}
		for (int32 Ly = 0; Ly < LH; ++Ly) for (int32 Lx = X0; Lx < LW; ++Lx)
		{
			const int32 I = Ly * LW + Lx;
			const double C = Cs[I];
			if (C <= 0) continue;
			// past its edge the surface is taken to go on falling away
			auto At = [&](int32 X, int32 Y) { const int32 J = FMath::Min(LH - 1, FMath::Max(0, Y)) * LW + FMath::Min(LW - 1, FMath::Max(X0, X)); return Cs[J] > 0 ? double(Zs[J]) : double(Zs[I]) + 40; };
			// the surface's slope, so which way it faces
			const double Gx = (At(Lx + 1, Ly) - At(Lx - 1, Ly)) / (2 * STEP), Gy = (At(Lx, Ly + 1) - At(Lx, Ly - 1)) / (2 * STEP);
			const double Nl = 1 / FMath::Sqrt(Gx * Gx + Gy * Gy + 1), Nx = Gx * Nl, Ny = Gy * Nl, Nz = -Nl;
			const double Px = (Lx + 0.5) * STEP, Py = (Ly + 0.5) * STEP, Pz = Zs[I];
			double Vx = CAM.X - Px, Vy = CAM.Y - Py, Vz = CAM.Z - Pz;
			const double Dist = FMath::Sqrt(Vx * Vx + Vy * Vy + Vz * Vz); Vx /= Dist; Vy /= Dist; Vz /= Dist;
			const double Lam = FMath::Max(0.0, Nx * Vx + Ny * Vy + Nz * Vz);
			const double Fall = FMath::Pow(-CAM.Z / Dist, 3.0);
			// skin, a little mottled, with a broad wet sheen where it faces the flash squarely
			const double M = 0.84 + 0.32 * Mot(Px / 30, Py / 30) + 0.14 * (Wr(Px / 11, Py / 11) - 0.5);
			// (a hard terminator: the light goes off it quickly as it turns away)
			const double Turn = FMath::Pow(Lam, 1.7), Sheen = 0.2 * FMath::Pow(Lam, 26.0), Lit = FLASH * Fall;
			uint8* D = &Layer.D[I * 4];
			for (int32 K = 0; K < 3; ++K) D[K] = FS47Canvas::Clamp8(255 * (1 - FMath::Exp(-Lit * (Turn * SKIN[K] * M + Sheen) * 1.55)));
			D[3] = FS47Canvas::Clamp8(255 * C);
		}

		// ---- the eye: black, wet, too big ----
		const FVector2D E = Lean(EYE.X, EYE.Y);
		const double Tilt = EYE.Tilt + LEAN.A, Ct = FMath::Cos(Tilt), St = FMath::Sin(Tilt);
		// (the layer's pixel, back in the eye's frame: scale(1 / STEP), translate(ex, ey), rotate(tilt) undone)
		auto ToEye = [&](double X, double Y) { const double Dx = X * STEP - E.X, Dy = Y * STEP - E.Y; return FVector2D(Dx * Ct + Dy * St, -Dx * St + Dy * Ct); };
		const TArray<FVector2D> Fold = EyePath(EYE.L + 8, EYE.H + 8), Eye = EyePath(EYE.L, EYE.H);
		TArray<FVector2D> Lid = { FVector2D(-EYE.L * 0.4, EYE.H * 0.1) };
		Bezier(Lid, Lid.Last(), FVector2D(-EYE.L * 0.16, EYE.H * 0.5), FVector2D(EYE.L * 0.12, EYE.H * 0.6), FVector2D(EYE.L * 0.36, EYE.H * 0.42));
		const TArray<FStop> Shine = { { 0, 150, 160, 164, 0.2 }, { 0.5, 90, 98, 102, 0.07 }, { 1, 90, 98, 102, 0 } };
		const int32 SS = 4; // samples a side in a pixel at the shapes' edges
		const double Reach = (EYE.L + 16) * 0.6 / STEP;
		for (int32 Ly = FMath::Max(0, int32(E.Y / STEP - Reach)); Ly < FMath::Min(LH, int32(E.Y / STEP + Reach) + 1); ++Ly)
		{
			for (int32 Lx = FMath::Max(0, int32(E.X / STEP - Reach)); Lx < FMath::Min(LW, int32(E.X / STEP + Reach) + 1); ++Lx)
			{
				double CFold = 0, CEye = 0, CLid = 0;
				FVector4 Sh(0, 0, 0, 0);
				for (int32 Sy = 0; Sy < SS; ++Sy) for (int32 Sx = 0; Sx < SS; ++Sx)
				{
					const FVector2D Q = ToEye(Lx + (Sx + 0.5) / SS, Ly + (Sy + 0.5) / SS);
					if (Inside(Fold, Q)) CFold += 1;
					if (Inside(Eye, Q))
					{
						CEye += 1;
						// the curve of it gives back a little of the flash, high on the inner side (clipped to the eye)
						if (const TOptional<double> T = Radial(Q, FVector2D(EYE.L * 0.1, -EYE.H * 0.26), 2, FVector2D(EYE.L * 0.06, -EYE.H * 0.1), EYE.L * 0.46))
						{
							const FVector4 S = StopsAt(Shine, *T);
							Sh += FVector4(S.X * S.W, S.Y * S.W, S.Z * S.W, S.W);
						}
					}
					// the wet line of the lower lid (lineWidth 2.6)
					double Dm = 1e9;
					for (int32 K = 1; K < Lid.Num(); ++K) Dm = FMath::Min(Dm, SegDist(Q, Lid[K - 1], Lid[K]));
					if (Dm <= 1.3) CLid += 1;
				}
				const double N = SS * SS;
				// where the lids meet it, the skin folds in, dark
				Layer.Over(Lx, Ly, 16, 18, 18, 0.4 * CFold / N);
				Layer.Over(Lx, Ly, 4, 5, 6, CEye / N);
				if (Sh.W > 0) Layer.Over(Lx, Ly, Sh.X / Sh.W, Sh.Y / Sh.W, Sh.Z / Sh.W, Sh.W / N);
				Layer.Over(Lx, Ly, 220, 226, 222, 0.42 * CLid / N);
			}
		}

		// ---- out of focus: it is far too close for the lens ----
		Blur(Layer, BLUR, X0 - 2 * BLUR, LW);

		// ---- onto the print (mirrored for the left edge) ----
		auto Src = [&](int32 X) { return Side < 0 ? W - 1 - X : X; };
		for (int32 Y = 0; Y < H; ++Y) for (int32 X = 0; X < W; ++X)
		{
			const FVector4 S = Layer.Sample((Src(X) + 0.5) * LW / W - 0.5, (Y + 0.5) * LH / H - 0.5);
			if (S.W <= 0) continue;
			FColor& D = Raw[Y * W + X];
			D.R = FS47Canvas::Clamp8(S.X + D.R * (1 - S.W)); D.G = FS47Canvas::Clamp8(S.Y + D.G * (1 - S.W)); D.B = FS47Canvas::Clamp8(S.Z + D.B * (1 - S.W));
		}
		// what the flash blew out spills light over its own edge
		const int32 Gw = LW / 2, Gh = LH / 2;
		FFaceImage Glow(Gw, Gh);
		for (int32 Y = 0; Y < Gh; ++Y) for (int32 X = 0; X < Gw; ++X)
		{
			const FVector4 S = Layer.Sample(X * 2 + 0.5, Y * 2 + 0.5);
			const double A = S.W;
			const double R = A > 0 ? S.X / A : 0, G = A > 0 ? S.Y / A : 0, B = A > 0 ? S.Z / A : 0;
			const double L = (R * 0.3 + G * 0.59 + B * 0.11) * A;
			const double K = FMath::Max(0.0, (L - 176) / 79);
			uint8* P = &Glow.D[(Y * Gw + X) * 4];
			P[0] = 236; P[1] = 240; P[2] = 236; P[3] = FS47Canvas::Clamp8(FMath::RoundHalfFromZero(255 * K * K));
		}
		Blur(Glow, 4, 0, Gw);
		// globalCompositeOperation 'screen', globalAlpha 0.5
		for (int32 Y = 0; Y < H; ++Y) for (int32 X = 0; X < W; ++X)
		{
			const FVector4 S = Glow.Sample((Src(X) + 0.5) * Gw / W - 0.5, (Y + 0.5) * Gh / H - 0.5);
			const double As = S.W * 0.5;
			if (As <= 0) continue;
			FColor& D = Raw[Y * W + X];
			auto Screen = [As, &S](uint8 Cd, double Cs) { const double Dn = Cd / 255.0, Sn = S.W > 0 ? Cs / S.W / 255.0 : 0; return FS47Canvas::Clamp8(255 * (Dn + As * Sn * (1 - Dn))); };
			D.R = Screen(D.R, S.X); D.G = Screen(D.G, S.Y); D.B = Screen(D.B, S.Z);
		}

		// ---- the flash in the eye, high on the inner side: one soft spot, the way a highlight goes out of focus ----
		const double Hx = E.X + EYE.L * 0.2 * Ct + EYE.H * 0.3 * St, Hy = E.Y + EYE.L * 0.2 * St - EYE.H * 0.3 * Ct;
		const double Cr = FMath::Cos(Tilt * 0.5), Sr = FMath::Sin(Tilt * 0.5);
		const TArray<FStop> Hl = { { 0, 255, 255, 255, 0.86 }, { 0.55, 244, 250, 250, 0.64 }, { 1, 240, 250, 250, 0 } };
		const double Sxk = W / DW, Syk = H / DH;
		for (int32 Y = FMath::Max(0, int32((Hy - 9) * Syk)); Y < FMath::Min(H, int32((Hy + 9) * Syk) + 2); ++Y)
		{
			for (int32 X0p = FMath::Max(0, int32((Hx - 9) * Sxk)); X0p < FMath::Min(W, int32((Hx + 9) * Sxk) + 2); ++X0p)
			{
				FVector4 Acc(0, 0, 0, 0); // premultiplied, summed over the samples
				for (int32 Sy = 0; Sy < SS; ++Sy) for (int32 Sx = 0; Sx < SS; ++Sx)
				{
					// p.scale(W / DW, H / DH); translate(hx, hy); rotate(tilt / 2); scale(1.5, 1): back to the spot's own frame
					const double Dx = (X0p + (Sx + 0.5) / SS) / Sxk - Hx, Dy = (Y + (Sy + 0.5) / SS) / Syk - Hy;
					const double U = (Dx * Cr + Dy * Sr) / 1.5, V = -Dx * Sr + Dy * Cr;
					const double R = FMath::Sqrt(U * U + V * V);
					if (R <= 5) { const FVector4 S = StopsAt(Hl, R / 5); Acc += FVector4(S.X * S.W, S.Y * S.W, S.Z * S.W, S.W); }
				}
				if (Acc.W <= 0) continue;
				const double N = SS * SS, A = Acc.W / N;
				// source-over onto the print (mirrored for the left edge, as the transform is)
				FColor& D = Raw[Y * W + (Side < 0 ? W - 1 - X0p : X0p)];
				D.R = FS47Canvas::Clamp8(Acc.X / N + D.R * (1 - A));
				D.G = FS47Canvas::Clamp8(Acc.Y / N + D.G * (1 - A));
				D.B = FS47Canvas::Clamp8(Acc.Z / N + D.B * (1 - A));
			}
		}
	}

	bool SelfTest(TArray<FString>& Lines)
	{
		bool bOk = true;
		auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s film face: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
		auto Near = [](double A, double B, double Eps = 1e-6) { return FMath::Abs(A - B) < Eps; };
		// (the values filmFace.ts gives in node, seed 9067250 + 7919 * 3)
		const double Seed = 9067250 + 7919 * 3;
		const FVector2D E1 = FilmFaceEye(1), E2 = FilmFaceEye(-1);
		Check(Near(E1.X, 0.8782345678027049, 1e-12) && Near(E1.Y, 0.46865784516711156, 1e-12) && Near(E2.X, 0.12176543219729508, 1e-12), TEXT("the eye on the frame"));
		FLcg R(Seed);
		const double A = R(), B = R(), C = R();
		Check(Near(A, 0.4778958600945771, 1e-15) && Near(B, 0.8425918987486511, 1e-15) && Near(C, 0.5163325713947415, 1e-15), TEXT("lcg as JavaScript's"));
		const FNoise Wr(Seed * 2654435761.0), Mot(Seed * 40503 + 7);
		Check(Near(Wr(3.7, 5.2), 0.4268167588424682, 1e-7) && Near(Wr(-10.3, 70.9), 0.5051664640429624, 1e-7) && Near(Wr(100.5, 33.25), 0.7873901374987327, 1e-7) && Near(Mot(1.5, 2.5), 0.6246740221977234, 1e-7),
			TEXT("value noise (seed times 2654435761 as a double, then >>> 0)"));
		const FVector2D D1 = Depth(600, 150, Wr), D2 = Depth(640, 250, Wr), D3 = Depth(700, 60, Wr), D4 = Depth(560, 330, Wr), D5 = Depth(594, 182, Wr);
		Check(Near(D1.X, 35.83902115134814, 1e-5) && D1.Y == 1 && Near(D2.X, 31.90783743486968, 1e-5) && Near(D3.X, 1.53267012748788, 1e-5) && D4.Y == 0 && Near(D5.X, 39.610842580820204, 1e-5), TEXT("the head's depth"));
		const FVector2D Lp = Lean(594, 182), Up = Unlean(500, 100);
		Check(Near(Lp.X, 562.0701233937311, 1e-9) && Near(Lp.Y, 187.46313806684464, 1e-9) && Near(Up.X, 543.7918398187247, 1e-9) && Near(Up.Y, 87.2284779597565, 1e-9), TEXT("the lean"));
		// on a black frame: the head pale at the right edge, black in the eye, nothing at the left
		TArray<FColor> Px; Px.Init(FColor(8, 8, 8, 255), 640 * 400);
		FilmFace(Px, 640, 400, Seed, 1);
		auto Lum = [&Px](int32 X, int32 Y) { const FColor& P = Px[Y * 640 + X]; return P.R * 0.3 + P.G * 0.59 + P.B * 0.11; };
		Check(Lum(630, 120) > 120 && Lum(100, 200) < 10, FString::Printf(TEXT("the head at the edge (%.0f), the rest untouched (%.0f)"), Lum(630, 120), Lum(100, 200)));
		Check(Lum(int32(E1.X * 640) - 25, int32(E1.Y * 400) + 8) < Lum(630, 120) * 0.5, TEXT("the eye is dark in the pale face"));
		return bOk;
	}
}

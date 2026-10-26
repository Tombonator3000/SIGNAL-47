// story/flashFrames.ts: the single frames of FLASH
#include "S47FlashFrames.h"

#include "S47Docs.h"

namespace
{
	using S47Raster::FRng;
	const int32 W = 640, H = 400;
	const double PI2 = 7; // (the web version's arcs go round with 7 radians: a whole turn)

	FS47Pix Make(double Seed, TFunctionRef<void(FS47Pix&, FRng&)> Draw)
	{
		FS47Pix G(W, H);
		FRng R(Seed);
		G.SetFill(0x000000); G.FillRect(0, 0, W, H);
		Draw(G, R);
		// film grain, a cold cast and a heavy vignette, like a frame from a tape that should not exist
		TArray<FColor> D = G.Colors();
		for (FColor& C : D)
		{
			const double N = (R() - 0.5) * 46, L = (C.R + C.G + C.B) / 3.0;
			C.R = FS47Pix::Clamp8(FMath::Max(0.0, FMath::Min(255.0, L * 0.86 + N)));
			C.G = FS47Pix::Clamp8(FMath::Max(0.0, FMath::Min(255.0, L * 1.0 + N)));
			C.B = FS47Pix::Clamp8(FMath::Max(0.0, FMath::Min(255.0, L * 0.95 + N)));
		}
		G.SetColors(D);
		FS47Pix::FPaint V = FS47Pix::Radial(W / 2.0, H / 2.0, H * 0.25, W / 2.0, H / 2.0, W * 0.62);
		V.Stop(0, 0x000000, 0).Stop(1, 0x000000, 0.92);
		G.SetFill(V); G.FillRect(0, 0, W, H);
		for (int32 Y = 0; Y < H; Y += 3) { G.SetFill(0x000000, 0.18); G.FillRect(0, Y, W, 1); }
		return G;
	}

	// Codex's frame (round 11) when it is there, drawn over the whole frame; k varies it a little (the
	// face is shown three times).
	FS47Pix Art(const TCHAR* Id, double Seed, double K = 0)
	{
		return Make(Seed, [Id, K](FS47Pix& G, FRng&)
		{
			FS47Pix Im;
			if (!S47Raster::Art(Id, Im)) return;
			const double S = 1 + K * 0.05;
			if (K) G.Filter = { { FS47Pix::EFilter::Blur, K * 1.5 } }; else G.Filter.Reset();
			G.DrawImage(Im, (W - W * S) / 2 + K * 9, (H - H * S) / 2 + K * 5, W * S, H * S);
			G.Filter.Reset();
		});
	}

	FS47Pix Eye()
	{
		if (S47Raster::ArtLoaded(TEXT("flashEye"))) return Art(TEXT("flashEye"), 1);
		return Make(1, [](FS47Pix& G, FRng&)
		{
			G.Save(); G.Translate(W / 2.0, H / 2.0);
			auto Lid = [&G](double K) { G.BeginPath(); G.MoveTo(-260, 0); G.QuadraticCurveTo(0, -150 * K, 260, 0); G.QuadraticCurveTo(0, 150 * K, -260, 0); };
			Lid(1); G.SetFill(0x7a776e); G.Fill();
			G.Clip();
			FS47Pix::FPaint Iris = FS47Pix::Radial(0, 0, 20, 0, 0, 120);
			Iris.Stop(0, 0x0a0a0a).Stop(0.62, 0x0e0e0e).Stop(0.7, 0x4a4f48).Stop(1, 0x22231f);
			G.SetFill(Iris); G.BeginPath(); G.Arc(0, 0, 120, 0, PI2); G.Fill();
			// the pupil is too big, and too glossy
			G.SetFill(0x020202); G.BeginPath(); G.Arc(0, 0, 82, 0, PI2); G.Fill();
			G.SetFill(0xffffff, 0.9); G.BeginPath(); G.Arc(-28, -30, 9, 0, PI2); G.Fill();
			G.SetFill(0xffffff, 0.35); G.BeginPath(); G.Arc(24, 22, 4, 0, PI2); G.Fill();
			G.Restore();
			G.SetStroke(0x141414, 0.9); G.LineWidth = 10; G.Save(); G.Translate(W / 2.0, H / 2.0); Lid(1); G.Stroke(); G.Restore();
		});
	}

	FS47Pix Hand()
	{
		if (S47Raster::ArtLoaded(TEXT("flashHand"))) return Art(TEXT("flashHand"), 2);
		return Make(2, [](FS47Pix& G, FRng& R)
		{
			// dawn behind a fogged windshield, the hand dark against it
			FS47Pix::FPaint Sky = FS47Pix::Linear(0, 0, 0, H);
			Sky.Stop(0, 0x3a4048).Stop(0.7, 0xb4a68e).Stop(1, 0x5a5048);
			G.SetFill(Sky); G.FillRect(0, 0, W, H);
			for (int32 i = 0; i < 220; ++i)
			{
				G.SetFill(0xe6e6e1, 0.04 + R() * 0.06);
				const double X = R() * W, Y = R() * H, Rad = 6 + R() * 30;
				G.BeginPath(); G.Arc(X, Y, Rad, 0, PI2); G.Fill();
			}
			G.SetFill(0x0d0d0f);
			G.Save(); G.Translate(W / 2.0 + 10, H * 0.78); G.Rotate(-0.08);
			G.BeginPath(); G.Ellipse(0, 0, 70, 58, 0, 0, PI2); G.Fill();                          // the palm
			const double Fingers[4][3] = { { -52, 230, -0.32 }, { -18, 290, -0.1 }, { 18, 300, 0.08 }, { 52, 250, 0.3 } };
			for (const auto& F : Fingers)
			{
				G.Save(); G.Translate(F[0], -30); G.Rotate(F[2]);
				G.BeginPath(); G.RoundRect(-9, -F[1], 18, F[1], 9); G.Fill();
				G.BeginPath(); G.Ellipse(0, -F[1], 13, 16, 0, 0, PI2); G.Fill();                 // pads pressed flat on the glass
				G.Restore();
			}
			G.Restore();
			// where it touches, the fog is wiped
			G.bLighter = true;
			for (int32 i = 0; i < 40; ++i)
			{
				G.SetFill(0x28282c, 0.08);
				const double X = W / 2.0 - 140 + R() * 280, Y = H * 0.1 + R() * H * 0.6, Hh = 30 + R() * 60;
				G.FillRect(X, Y, 2, Hh);
			}
			G.bLighter = false;
		});
	}

	FS47Pix Face(int32 K)
	{
		if (S47Raster::ArtLoaded(TEXT("flashFace"))) return Art(TEXT("flashFace"), 3 + K, K);
		return Make(3 + K, [K](FS47Pix& G, FRng& R)
		{
			G.Save(); G.Translate(W / 2.0 + (K - 1) * 14, H / 2.0 + 20 + (K - 1) * 6);
			G.Filter = { { FS47Pix::EFilter::Blur, 3.0 + K * 2 } };
			FS47Pix::FPaint Sk = FS47Pix::Radial(-30, -60, 10, 0, -20, 190);
			Sk.Stop(0, 0x8d8b84).Stop(0.6, 0x3c3b37).Stop(1, 0x0c0c0c);
			G.SetFill(Sk);
			G.BeginPath(); G.MoveTo(0, -200); G.BezierCurveTo(150, -200, 160, -20, 60, 90); G.QuadraticCurveTo(0, 150, -60, 90); G.BezierCurveTo(-160, -20, -150, -200, 0, -200); G.Fill();
			G.SetFill(0x000000);
			for (const double S : { -1.0, 1.0 }) { G.Save(); G.Translate(S * 58, -40); G.Rotate(S * 0.45); G.BeginPath(); G.Ellipse(0, 0, 48, 24, 0, 0, PI2); G.Fill(); G.Restore(); }
			G.Restore();
			G.Filter.Reset();
			// most of it is lost in the dark and the motion
			G.SetFill(0x000000, 0.35 + R() * 0.2); G.FillRect(0, 0, W * (0.25 + K * 0.08), H);
		});
	}

	FS47Pix FromPhoto(double Seed, const double Rect[4], TFunction<void(FS47Pix&, FRng&)> Fallback, double Blur = 0)
	{
		const double Rc[4] = { Rect[0], Rect[1], Rect[2], Rect[3] };
		return Make(Seed, [&Rc, &Fallback, Blur](FS47Pix& G, FRng& R)
		{
			FS47Pix Im;
			if (S47Raster::Art(TEXT("photo1947"), Im))
			{
				const double Sx = Im.W / 2400.0, Sy = Im.H / 1600.0;
				if (Blur) G.Filter = { { FS47Pix::EFilter::Blur, Blur }, { FS47Pix::EFilter::Contrast, 1.3 } };
				else G.Filter = { { FS47Pix::EFilter::Contrast, 1.25 } };
				G.DrawImage(Im, Rc[0] * Sx, Rc[1] * Sy, Rc[2] * Sx, Rc[3] * Sy, 0, 0, W, H);
				G.Filter.Reset();
			}
			else Fallback(G, R);
		});
	}

	FS47Pix MesaLight()
	{
		const double Rect[4] = { 700, 200, 400, 250 };
		return FromPhoto(7, Rect, [](FS47Pix& G, FRng&)
		{
			G.SetFill(0x101214); G.FillRect(0, H * 0.7, W, H * 0.3);
			FS47Pix::FPaint L = FS47Pix::Radial(W / 2.0, H * 0.4, 4, W / 2.0, H * 0.4, 120);
			L.Stop(0, 0xffffff, 1).Stop(0.2, 0xdcdcd2, 0.5).Stop(1, 0x000000, 0);
			G.SetFill(L); G.FillRect(0, 0, W, H);
		});
	}

	FS47Pix Man()
	{
		if (S47Raster::ArtLoaded(TEXT("flashMan"))) return Art(TEXT("flashMan"), 8);
		// the surveyor with the rod, far right in the 1947 picture, enlarged until it falls apart
		const double Rect[4] = { 2030, 930, 220, 138 };
		return FromPhoto(8, Rect, [](FS47Pix& G, FRng&)
		{
			G.SetFill(0x2a2a28);
			G.BeginPath(); G.Ellipse(W / 2.0, 120, 30, 36, 0, 0, PI2); G.Fill();
			G.FillRect(W / 2.0 - 60, 160, 120, 240);
			G.FillRect(W / 2.0 - 70, 82, 140, 14);
		}, 3);
	}

	FS47Pix Stars()
	{
		return Make(9, [](FS47Pix& G, FRng& R)
		{
			for (int32 i = 0; i < 900; ++i)
			{
				const double B = FMath::Pow(R(), 4);
				G.SetFill(0xffffff, 0.25 + B * 0.75);
				const double X = R() * W, Y = R() * H;
				G.FillRect(X, Y, B > 0.6 ? 2 : 1, B > 0.6 ? 2 : 1);
			}
			FS47Pix::FPaint Band = FS47Pix::Linear(0, H, W, 0);
			Band.Stop(0.3, 0x000000, 0).Stop(0.5, 0xb4aac8, 0.18).Stop(0.7, 0x000000, 0);
			G.SetFill(Band); G.FillRect(0, 0, W, H);
		});
	}

	FS47Pix Terminal(bool bEveningTimeline)
	{
		return Make(10, [bEveningTimeline](FS47Pix& G, FRng&)
		{
			G.SetFill(0x031006); G.FillRect(40, 30, W - 80, H - 60);
			G.SetFill(0x5bff8a);
			// as the operations terminal shows it (story/nightshift.ts, DIR SURVEY.RAW)
			auto Row = [](const FString& N, int32 B, const FString& Dt) { return S47Text::PadEnd(N, 20) + S47Text::PadStart(FString::FromInt(B), 6) + TEXT("   ") + Dt; };
			const TArray<FString> Lines = { TEXT("$ DIR SURVEY.RAW"), TEXT(""), TEXT("Directory DUA1:[SURVEY.RAW]"), TEXT(""), Row(TEXT("RUN860412_2338.DAT"), 2214, TEXT("12-APR-1986 23:38")),
				Row(bEveningTimeline ? TEXT("RUN860413_1800.DAT") : TEXT("RUN860413_2341.DAT"), bEveningTimeline ? 4306 : 2271, bEveningTimeline ? TEXT("13-APR-1986 18:00") : TEXT("13-APR-1986 23:41")), Row(TEXT("RUN860414_0529.DAT"), 0, TEXT("14-APR-1986 05:29")), TEXT(""), TEXT("$ _") };
			for (int32 i = 0; i < Lines.Num(); ++i) G.Text(Lines[i], 64, 74 + i * 30, TEXT("VT323-Regular.ttf"), 22);
			FS47Pix::FPaint Glow = FS47Pix::Radial(W / 2.0, H / 2.0, 40, W / 2.0, H / 2.0, 320);
			Glow.Stop(0, 0x3cff78, 0.08).Stop(1, 0x000000, 0);
			G.SetFill(Glow); G.FillRect(0, 0, W, H);
		});
	}

	FS47Pix Photo()
	{
		// core/art.ts PHOTO_1947.A
		const double Rect[4] = { 600, 300, 960, 720 };
		return FromPhoto(11, Rect, [](FS47Pix& G, FRng&)
		{
			G.SetFill(0x1a1a18); G.FillRect(0, 0, W, H);
			G.SetFill(0x0a0a0a); G.FillRect(0, H * 0.72, W, H * 0.28);
			G.SetFill(0xf0f0e6, 0.8); G.BeginPath(); G.Arc(W * 0.45, H * 0.3, 14, 0, PI2); G.Fill();
		});
	}
}

TArray<S47FlashFrames::FFrame> S47FlashFrames::FlashFrames(bool bEveningTimeline)
{
	FS47Pix F0 = Face(0), F1 = Face(1), F2 = Face(2);
	TArray<FFrame> Out;
	Out.Add({ TEXT("eye"), Eye(), 90 });
	Out.Add({ TEXT("hand"), Hand(), 110 });
	Out.Add({ TEXT("face1"), MoveTemp(F0), 70 }); Out.Add({ TEXT("face2"), MoveTemp(F1), 70 }); Out.Add({ TEXT("face3"), MoveTemp(F2), 70 });
	Out.Add({ TEXT("light"), MesaLight(), 90 });
	Out.Add({ TEXT("man"), Man(), 110 });
	Out.Add({ TEXT("stars"), Stars(), 80 });
	Out.Add({ TEXT("terminal"), Terminal(bEveningTimeline), 100 });
	Out.Add({ TEXT("1947"), Photo(), 120 });
	return Out;
}

bool S47FlashFrames::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s flash frames: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// the stars frame needs no picture: dark, grainy, the corners darker than the middle
	const FS47Pix S = Stars();
	const TArray<FColor> C = S.Colors();
	double Mid = 0, Corner = 0;
	for (int32 Y = 180; Y < 220; ++Y) for (int32 X = 300; X < 340; ++X) Mid += C[Y * W + X].G;
	for (int32 Y = 0; Y < 40; ++Y) for (int32 X = 0; X < 40; ++X) Corner += C[Y * W + X].G;
	Check(S.W == 640 && S.H == 400 && Mid > Corner && C[0].A == 255, TEXT("a frame of the stars, vignetted"));
	return bOk;
}

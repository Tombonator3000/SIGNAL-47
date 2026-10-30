// story/theThird.ts: the third one
#include "S47TheThird.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "S47Figures.h"
#include "S47Raster.h"
#include "S47Three.h"
#include "S47Web.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Third, Log, All);

namespace
{
	using S47Three::Hex;
	/** recording.ts Walker.baked: how much a light from behind it (+z) falls on a face with this normal
	 *  (the back full, the shoulders and the sides some, the front none). */
	double Shade(const FVector& N) { return FMath::Min(1.0, 0.22 + 0.78 * FMath::Max(0.0, N.Z) + 0.3 * FMath::Max(0.0, N.Y) + 0.12 * FMath::Abs(N.X)); }
}

// ---------- ThirdFigure ----------

FS47ThirdFigure::FS47ThirdFigure(AS47Web* Web, const FString& Name)
{
	G = MakeUnique<FS47Group>(Web, Name);
	// Unreal: an original rounded garment silhouette replaces the old walker's boxes.
	// The feet, head and tape anchors remain the same for both FRAME 03 and the road.
	const FLinearColor Coat = Hex(0x2a3036), Dark = Hex(0x15171a);
	const FLinearColor Hair = Hex(0x221b15), Hand = Hex(0x312b25), Shoe = Hex(0x101216);
	FS47Node* Body = G->Group(&G->Root());
	// The shoulder yoke overlaps the soft back and the slightly flared jacket hem.
	// These volumes share their back-light response, so their joins do not draw seams.
	Rounded(Body, FVector(.52,.55,.30), Coat, FVector(0,1.18,.008));
	Rounded(Body, FVector(.51,.24,.285), Coat, FVector(0,1.345,.002));
	Rounded(Body, FVector(.44,.21,.29), Coat, FVector(0,.975,.006));
	Rounded(Body, FVector(.36,.21,.235), Dark, FVector(0,.86,.012));
	Rounded(Body, FVector(.18,.115,.18), Coat, FVector(0,1.435,.006));
	Rounded(Body, FVector(.105,.10,.12), Hair, FVector(0,1.482,-.018));
	for (int32 k = 0; k < 2; ++k)
	{
		const double Sd = k ? 1 : -1;
		// Slightly bent knees and narrower calves leave a readable gap between the legs.
		Rounded(Body, FVector(.185,.44,.22), Dark, FVector(Sd*.108,.659,.016), FVector(.06,0,Sd*.04));
		Rounded(Body, FVector(.143,.15,.16), Dark, FVector(Sd*.121,.438,-.009));
		Rounded(Body, FVector(.139,.34,.165), Dark, FVector(Sd*.125,.263,.022), FVector(-.06,0,-Sd*.012));
		Rounded(Body, FVector(.115,.115,.13), Dark, FVector(Sd*.125,.105,.027));
		// Soles touch y=0. The two toes turn out a little and are not in lockstep.
		Rounded(Body, FVector(.145,.11,.285), Shoe, FVector(Sd*.125,.055,k ? -.025 : -.055), FVector(0,-Sd*.07,0));

		// Dropped shoulders, a soft elbow and forward-hanging hands, with no straight
		// shoulder-to-wrist cylinder. The sleeve tape stays at its canonical 1.18 m.
		Rounded(Body, FVector(.175,.205,.19), Coat, FVector(Sd*.245,1.357,.004));
		Rounded(Body, FVector(.145,.30,.16), Coat, FVector(Sd*.290,1.30,.006), FVector(.035,0,Sd*.18));
		Rounded(Body, FVector(.126,.13,.145), Coat, FVector(Sd*.317,1.15,-.006));
		Rounded(Body, FVector(.116,.27,.135), Coat, FVector(Sd*.323,1.026,-.039), FVector(.16,0,-Sd*.045));
		Rounded(Body, FVector(.113,.058,.128), Coat, FVector(Sd*.319,.936,-.055));
		Rounded(Body, FVector(.085,.135,.093), Hand, FVector(Sd*.319,.874,-.066), FVector(.10,0,Sd*.035));
		ReflectiveBand(Body, FVector(Sd*.312,1.18,.006), .052, .057, .05, 8);
	}
	ReflectiveBand(Body, FVector(0,1.2,.008), .260, .158, .07, 12);
	// Only the back of the hair is defined; Reyes is never given a face.
	Rounded(Body, FVector(.235,.26,.245), Hair, FVector(0,1.575,-.045), FVector(-.12,0,0));
	Recolor();
	Show();
}

FS47ThirdFigure::~FS47ThirdFigure() {}

void FS47ThirdFigure::Rounded(FS47Node* Parent, const FVector& Size, const FLinearColor& Base, const FVector& P, const FVector& R)
{
	FFace F;
	F.M = G->Basic(FLinearColor::Black, TEXT("M_Web_Unlit_Opaque"));
	F.Base = Base; F.Shade = Shade(FVector(0,0,1));
	FS47Node* Part = G->Group(Parent, P, R);
	G->Sphere(Part, 1, F.M, FVector::ZeroVector, Size * .5);
	Faces.Add(F);
}

void FS47ThirdFigure::ReflectiveBand(FS47Node* Parent, const FVector& P, double HalfWidth, double HalfDepth, double Height, int32 Segments)
{
	// A thin tape strip follows the rear of the garment, instead of a bright box
	// through the torso. Each facet retains the original direction-dependent return.
	constexpr double Arc = 75 * UE_DOUBLE_PI / 180;
	for (int32 I = 0; I < Segments; ++I)
	{
		const double A = -Arc + 2 * Arc * I / Segments, B = -Arc + 2 * Arc * (I+1) / Segments;
		const FVector From(HalfWidth*FMath::Sin(A),0,HalfDepth*FMath::Cos(A));
		const FVector To(HalfWidth*FMath::Sin(B),0,HalfDepth*FMath::Cos(B));
		const FVector Along = To-From;
		const double Turn = FMath::Atan2(-Along.Z,Along.X);
		FFace F;
		F.M = G->Basic(FLinearColor::Black, TEXT("M_Web_Unlit_Opaque_2S"));
		F.Base = Hex(0xd8dccc); F.Shade = Shade(FVector(FMath::Sin(Turn),0,FMath::Cos(Turn))); F.bBand = true;
		G->Plane(Parent, Along.Size(), Height, F.M, P+(From+To)*.5, FVector(0,Turn,0));
		Faces.Add(F);
	}
}

void FS47ThirdFigure::Recolor()
{
	// diffuseColor = mix(colour * (uAmb + uLit * shade), uBand * (0.3 + 0.7 * shade), band)
	for (const FFace& F : Faces)
	{
		if (!F.M) continue;
		const float S = float(F.Shade);
		const FLinearColor C = F.bBand ? BandC * (0.3f + 0.7f * S) : F.Base * (AmbC + LitC * S);
		S47Figures::SetColor(F.M, FLinearColor(C.R, C.G, C.B, 1));
	}
}

void FS47ThirdFigure::Show() { G->SetVisible(bShown && FadeK >= 0.5); }
void FS47ThirdFigure::SetVisible(bool bOn) { bShown = bOn; Show(); }
void FS47ThirdFigure::AddToScene() { G->AddToScene(); }
void FS47ThirdFigure::RemoveFromScene() { G->RemoveFromScene(); }

void FS47ThirdFigure::Place(const FVector& At, double Yaw)
{
	Pos = At; YawV = Yaw;
	G->Root().Position = At; G->Root().Rotation = FVector(0, Yaw, 0); G->Root().Apply();
}

void FS47ThirdFigure::Light(const FLinearColor& Lit, const FLinearColor& Amb, const FLinearColor& Band)
{
	LitC = Lit; AmbC = Amb; BandC = Band;
	Recolor();
}

void FS47ThirdFigure::SetFade(double K) { FadeK = FMath::Clamp(K, 0.0, 1.0); Show(); }

TArray<FVector> FS47ThirdFigure::BandPoints(const FVector& Eye) const
{
	static const double Local[7][2] = { { -0.2, 1.2 }, { -0.1, 1.2 }, { 0, 1.2 }, { 0.1, 1.2 }, { 0.2, 1.2 }, { -0.32, 1.18 }, { 0.32, 1.18 } };
	const FQuat Q = S47Three::Euler(0, YawV, 0);
	TArray<FVector> Out;
	for (const auto& L : Local)
	{
		FVector P = Q.RotateVector(FVector(L[0], L[1], 0.16)) + Pos;
		FVector V = Eye - P; V.Y = 0;
		P += V.GetSafeNormal() * 0.14;
		Out.Add(P);
	}
	return Out;
}

FVector FS47ThirdFigure::Head() const { return S47Three::Euler(0, YawV, 0).RotateVector(FVector(0, 1.6, 0)) + Pos; }

// ---------- FRAME 03 ----------

TOptional<FS47FilmShot> FS47FilmThird::Shot(const FS47FilmIn& O)
{
	const FS47Cam& Cam = O.Cam;
	const FVector Eye = Cam.Pos;
	const FVector North(O.CDir.Z, 0, -O.CDir.X);   // C's stakes run east: north is to their left
	struct FBest { FVector At; double U, V, Deg; };
	TOptional<FBest> Best;
	// nearest C's stakes first, then out to the north (and a little to the south) of them
	TArray<double> Degs = { 0 };
	for (double D = 2; D <= FILM.NorthMax; D += 2) { Degs.Add(D); if (D <= FILM.SouthMax) Degs.Add(-D); }
	for (const double Deg : Degs)
	{
		const double A = FMath::DegreesToRadians(Deg);
		for (double R = FILM.From; R <= FILM.To && !Best; R += FILM.Step)
		{
			const FVector W = (O.CDir * FMath::Cos(A) + North * FMath::Sin(A)) * R + O.Transit;
			const FVector At(W.X, O.Ground ? O.Ground(W.X, W.Z) : 0, W.Z);
			const FVector Band(At.X, At.Y + 1.2, At.Z), Top(At.X, At.Y + 1.7, At.Z);
			const FVector P = Cam.Project(Band), Q = Cam.Project(Top);
			const double U = (P.X + 1) / 2, V = (1 - P.Y) / 2;
			const bool bInView = P.Z < 1 && Q.Z < 1 && U > 0.07 && U < 0.93 && V > 0.08 && V < 0.92 && (1 - Q.Y) / 2 > 0.03;
			if (!bInView || (O.Clear && !O.Clear(Eye, Band))) continue;
			Best = FBest{ At, U, V, Deg };
		}
		if (Best) break;
	}
	if (!Best) return {};
	const FBest B = *Best;
	// back to the camera, looking out the way C runs
	const double Yaw = FMath::Atan2(-O.CDir.X, -O.CDir.Z);
	const double Face = FMath::Atan2(-(B.At.X - Eye.X), -(B.At.Z - Eye.Z));
	// (turned towards C's way by up to 35 degrees, never side on to the camera)
	double Dd = Yaw - Face; Dd = FMath::Atan2(FMath::Sin(Dd), FMath::Cos(Dd));
	const double Turn = Face + FMath::Clamp(Dd, -0.6, 0.6);
	FS47FilmShot S;
	S.U = B.U; S.V = B.V; S.bOnC = FMath::Abs(B.Deg) <= FILM.OnC;
	S.Before = [this, B, Turn]
	{
		if (!Fig) Fig = MakeShared<FS47ThirdFigure>(Web.Get(), TEXT("third:film"));
		Fig->Place(B.At, Turn);
		// the flash does not reach it: a shape against the sky at most; the tape alone comes back
		Fig->Light(FLinearColor(0, 0, 0), FLinearColor(0.006f, 0.007f, 0.008f), FLinearColor(0.55f, 0.56f, 0.52f));
		Fig->SetFade(1);
		Fig->SetVisible(true);
		Fig->AddToScene();
	};
	S.After = [this] { if (Fig) { Fig->SetVisible(false); Fig->RemoveFromScene(); } };
	S.Post = [Cam, B, Turn](TArray<FColor>& Raw, int32 W, int32 H) { Glow(Raw, W, H, Cam, B.At, Turn); };
	UE_LOG(LogS47Third, Display, TEXT("S47: the third one on the film at %.2f %.2f%s"), B.U, B.V, S.bOnC ? TEXT(", on the C stakes") : TEXT(""));
	return S;
}

void FS47FilmThird::Glow(TArray<FColor>& Raw, int32 W, int32 H, const FS47Cam& Cam, const FVector& At, double Yaw)
{
	// the tape in the flash, on the raw frame: across the back and the sleeves, which at this distance run
	// together into one short line, with the halo film gives a hard light. Faint, and a few pixels across.
	FS47Pix G = FS47Pix::FromColors(Raw, W, H);
	const FVector Side(FMath::Cos(Yaw), 0, -FMath::Sin(Yaw));
	auto Px = [&](const FVector& P) { const FVector Q = Cam.Project(P); return FVector2D((Q.X + 1) / 2 * W, (1 - Q.Y) / 2 * H); };
	const FVector C(At.X, At.Y + 1.2, At.Z);
	const FVector2D Cc = Px(C);
	const double Lx = Px(C - Side * 0.4).X, Rx = Px(C + Side * 0.4).X;
	const double Ty = Px(FVector(C.X, C.Y + 0.05, C.Z)).Y, By = Px(FVector(C.X, C.Y - 0.05, C.Z)).Y;
	const double Half = FMath::Max(3.6, FMath::Abs(Rx - Lx) / 2 + 0.6), Tall = FMath::Max(1.0, FMath::Abs(By - Ty) / 2 + 0.35);
	G.Save();
	G.bLighter = true;
	auto Streak = [&G](double X, double Y, double Hw, double Hh, double A)
	{
		G.Save(); G.Translate(X, Y); G.Scale(Hw, Hh);
		FS47Pix::FPaint Gr = FS47Pix::Radial(0, 0, 0, 0, 0, 1);
		Gr.Stop(0, 0xe2e4d8, A).Stop(0.5, 0xced2c8, A * 0.6).Stop(1, 0xb4bab4, 0);
		G.SetFill(Gr); G.BeginPath(); G.Arc(0, 0, 1, 0, UE_DOUBLE_PI * 2); G.Fill(); G.Restore();
	};
	// the halo first, wide and low, then the line, brightest across the back
	Streak(Cc.X, Cc.Y, Half * 1.6, Tall * 4.2, 0.22);
	Streak(Cc.X, Cc.Y, Half * 1.08, Tall * 1.5, 0.5);
	Streak(Cc.X, Cc.Y, Half * 0.62, Tall, 0.55);
	G.Restore();
	Raw = G.Colors();
}

bool FS47FilmThird::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s the third one: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// a frame looking east along C from beside the transit: it stands on the stakes' line
	FS47FilmIn In;
	In.Transit = FVector(3, 0, 7996); In.CDir = FVector(1, 0, 0);
	In.Ground = [](double, double) { return 0.0; };
	In.Cam.Pos = FVector(1, 1.6, 7997); In.Cam.Forward = FVector(1, 0, 0); In.Cam.Right = FVector(0, 0, 1); In.Cam.Up = FVector(0, 1, 0); In.Cam.Fov = 44.6; In.Cam.Aspect = 1.6;
	FS47FilmThird F(nullptr);
	const TOptional<FS47FilmShot> S = F.Shot(In);
	Check(S && S->bOnC && FMath::Abs(S->U - 0.5) < 0.05, TEXT("on the C stakes' line, in the middle of a frame along them"));
	// the glow: a short pale line on the raw frame where the band is
	TArray<FColor> Raw;
	Raw.Init(FColor(0, 0, 0, 255), 96 * 60);
	if (S) S->Post(Raw, 96, 60);
	const int32 Cx = FMath::RoundToInt(S ? S->U * 96 : 0), Cy = FMath::RoundToInt(S ? S->V * 60 : 0);
	Check(S && Raw[FMath::Clamp(Cy, 0, 59) * 96 + FMath::Clamp(Cx, 0, 95)].R > 60 && Raw[0].R == 0, TEXT("the tape gives the flash back, there and nowhere else"));
	// nothing of C's ground in a frame looking the other way
	In.Cam.Forward = FVector(-1, 0, 0); In.Cam.Right = FVector(0, 0, -1);
	Check(!F.Shot(In), TEXT("not in a frame that looks away from C"));
	return bOk;
}

FS47FilmThird::FS47FilmThird(AS47Web* InWeb) : Web(InWeb) {}

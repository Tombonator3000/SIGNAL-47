// What the web version builds in code at run time, built here out of the engine's basic shapes (S47Figures.h)
#include "S47Figures.h"

#include "Components/MaterialBillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Image.h"
#include "S47Raster.h"
#include "S47Seen.h"
#include "S47Three.h"
#include "S47Web.h"

namespace
{
	using S47Three::Hex;
	double JsRound(double V) { return FMath::FloorToDouble(V + 0.5); } // Math.round

	// the three.js matrix of a node, and of it with every parent above it
	void LocalMat(const FS47Node& N, double* E) { S47Three::Compose(N.Position, S47Three::Euler(N.Rotation.X, N.Rotation.Y, N.Rotation.Z, N.Order), N.Scale, E); }
}

// ---------- a node ----------

void FS47Node::Apply() const
{
	if (USceneComponent* Cm = C.Get())
	{
		double E[16];
		LocalMat(*this, E);
		Cm->SetRelativeTransform(S47Three::MatToUE(E));
	}
}

FVector FS47Node::LocalToWorld(const FVector& L) const
{
	// (the components' own world transforms are three's, conjugated with the axis swap: S47Three.h)
	const USceneComponent* Cm = C.Get();
	return Cm ? S47Three::ToThree(Cm->GetComponentTransform().TransformPosition(S47Three::ToUE(L))) : L;
}

FVector FS47Node::WorldToLocal(const FVector& W) const
{
	const USceneComponent* Cm = C.Get();
	return Cm ? S47Three::ToThree(Cm->GetComponentTransform().InverseTransformPosition(S47Three::ToUE(W))) : W;
}

FQuat FS47Node::WorldQuaternion() const
{
	// the world rotation's three axes, read back from the component's (Unreal's axes swapped)
	const USceneComponent* Cm = C.Get();
	if (!Cm) return FQuat::Identity;
	const FTransform T = Cm->GetComponentTransform();
	auto Axis = [&](const FVector& Three) { return S47Three::ToThree(T.TransformVectorNoScale(S47Three::DirToUE(Three)) * 100.0).GetSafeNormal(); };
	const FVector X = Axis(FVector(1, 0, 0)), Y = Axis(FVector(0, 1, 0)), Z = Axis(FVector(0, 0, 1));
	double E[16] = { X.X, X.Y, X.Z, 0, Y.X, Y.Y, Y.Z, 0, Z.X, Z.Y, Z.Z, 0, 0, 0, 0, 1 };
	FVector P, S; FQuat Q;
	S47Three::Decompose(E, P, Q, S);
	return Q;
}

// ---------- the group ----------

namespace S47Figures
{
	UMaterialInterface* Base(const TCHAR* Name)
	{
		return LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Web/Base/%s.%s"), Name, Name));
	}
	void SetColor(UMaterialInstanceDynamic* M, const FLinearColor& C) { if (M) M->SetVectorParameterValue(TEXT("Color"), C); }
	void SetMap(UMaterialInstanceDynamic* M, UTexture* T)
	{
		if (!M) return;
		M->SetTextureParameterValue(TEXT("Map"), T);
		M->SetScalarParameterValue(TEXT("UseMap"), T ? 1.f : 0.f);
		// (the engine's plane has its texture's top row at its far edge, +Z of the plane's own terms
		// before it is stood up: as three's flipped upload has it, the picture's top is up)
		M->SetVectorParameterValue(TEXT("MapU"), FLinearColor(1, 0, 0, 0));
		M->SetVectorParameterValue(TEXT("MapV"), FLinearColor(0, 1, 0, 0));
	}
	void SetOpacity(UMaterialInstanceDynamic* M, double A) { if (M) M->SetScalarParameterValue(TEXT("Opacity"), float(A)); }

	TArray<FColor> FrostGlassPixels()
	{
		const int32 W = 512, H = 256;
		S47Raster::FRng R(47);
		// a coarse noise for the patches the frost grows in
		const int32 N = 12;
		TArray<double> Cell;
		for (int32 i = 0; i < (N + 1) * (N + 1); ++i) Cell.Add(R());
		auto Noise = [&Cell, N](double U, double V)
		{
			const double X = U * N, Y = V * N;
			const int32 Ix = FMath::FloorToInt(X), Iy = FMath::FloorToInt(Y);
			const double Ax = X - Ix, Ay = Y - Iy;
			auto At = [&](int32 I, int32 J) { return Cell[FMath::Min(N, J) * (N + 1) + FMath::Min(N, I)]; };
			return (At(Ix, Iy) * (1 - Ax) + At(Ix + 1, Iy) * Ax) * (1 - Ay) + (At(Ix, Iy + 1) * (1 - Ax) + At(Ix + 1, Iy + 1) * Ax) * Ay;
		};
		TArray<FColor> Px;
		Px.SetNumUninitialized(W * H);
		for (int32 Y = 0; Y < H; ++Y) for (int32 X = 0; X < W; ++X)
		{
			const double U = double(X) / W, V = double(Y) / H;
			// nearest edge, in parts of the glass; the lower corners and the bottom heaviest
			const double Edge = FMath::Min(FMath::Min(U, 1 - U), FMath::Min(V * 1.4, (1 - V) * 0.8));
			const double K = FMath::Pow(FMath::Max(0.0, 1 - Edge / 0.22), 1.6);
			// the driver's side (left, from inside) a little clearer in the middle, as a breath leaves it
			const double Dx = (U - 0.27) / 0.2, Dy = (V - 0.45) / 0.3, Clear = FMath::Exp(-(Dx * Dx + Dy * Dy));
			const double A = FMath::Min(1.0, (0.04 + 0.96 * K) * (0.35 + 0.85 * Noise(U, V)) * (1 - 0.85 * Clear)) * (0.6 + 0.4 * R());
			Px[Y * W + X] = FColor(236, 243, 250, FS47Pix::Clamp8(FMath::FloorToDouble(255 * A * 0.85 + 0.5)));
		}
		FS47Pix G = FS47Pix::FromColors(Px, W, H);
		// and the fern of the crystals, grown in from the rubber
		G.SetStroke(0xf5faff, 0.2); G.LineWidth = 1;
		for (int32 n = 0; n < 28; ++n)
		{
			const int32 Side = FMath::FloorToInt(R() * 4);
			double X = Side == 0 ? 0 : Side == 1 ? W : R() * W;
			double Y = Side == 2 ? H : Side == 3 ? 0 : R() * H;
			double A = Side == 0 ? 0 : Side == 1 ? UE_DOUBLE_PI : Side == 2 ? -UE_DOUBLE_HALF_PI : UE_DOUBLE_HALF_PI;
			A += (R() - 0.5) * 1.2;
			for (int32 s = 0; s < 4; ++s)
			{
				const double Len = 4 + R() * 9, Nx = X + FMath::Cos(A) * Len, Ny = Y + FMath::Sin(A) * Len;
				G.BeginPath(); G.MoveTo(X, Y); G.LineTo(Nx, Ny); G.Stroke();
				for (const double B : { -0.9, 0.9 }) { G.BeginPath(); G.MoveTo(Nx, Ny); G.LineTo(Nx + FMath::Cos(A + B) * Len * 0.45, Ny + FMath::Sin(A + B) * Len * 0.45); G.Stroke(); }
				X = Nx; Y = Ny; A += (R() - 0.5) * 0.7;
			}
		}
		return G.Colors();
	}

	UTexture2D* FrostGlass()
	{
		static TStrongObjectPtr<UTexture2D> Tex;
		if (!Tex.IsValid()) Tex.Reset(S47Image::FromPixels(FrostGlassPixels(), 512, 256));
		return Tex.Get();
	}
}

FS47Group::FS47Group(AS47Web* InWeb, const FString& InName, USceneComponent* Parent) : Name(InName), Web(InWeb)
{
	TUniquePtr<FS47Node> R = MakeUnique<FS47Node>();
	if (InWeb)
	{
		USceneComponent* C = NewObject<USceneComponent>(InWeb);
		C->SetMobility(EComponentMobility::Movable);
		C->SetupAttachment(Parent ? Parent : InWeb->GetRootComponent());
		C->RegisterComponent();
		R->C = C;
	}
	Nodes.Add(MoveTemp(R));
	Refresh();
}

FS47Group::~FS47Group()
{
	for (int32 i = Nodes.Num() - 1; i >= 0; --i) if (USceneComponent* C = Nodes[i]->C.Get()) C->DestroyComponent();
	Nodes.Reset();
	Mids.Reset();
}

void FS47Group::Refresh()
{
	// (Unreal's components do not take their parents' visibility: each is set as three would draw it)
	const bool bOn = bVisible && bInScene;
	for (const TUniquePtr<FS47Node>& N : Nodes)
	{
		USceneComponent* C = N->C.Get();
		if (!C) continue;
		bool bVis = bOn;
		for (const FS47Node* P = N.Get(); P && bVis; P = P->Parent) bVis &= P->bVisible;
		C->SetVisibility(bVis, false);
	}
}

void FS47Group::SetVisible(bool bOn) { if (bVisible != bOn) { bVisible = bOn; Refresh(); } }
void FS47Group::AddToScene() { if (!bInScene) { bInScene = true; Refresh(); } }
void FS47Group::RemoveFromScene() { if (bInScene) { bInScene = false; Refresh(); } }

UMaterialInstanceDynamic* FS47Group::Basic(const FLinearColor& Color, const TCHAR* BaseName)
{
	UMaterialInterface* B = S47Figures::Base(BaseName);
	if (!B || !Web.IsValid()) return nullptr;
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(B, Web.Get());
	M->SetVectorParameterValue(TEXT("Color"), Color);
	Mids.Emplace(M);
	return M;
}

FS47Node* FS47Group::Group(FS47Node* Parent, const FVector& P, const FVector& R, const FVector& S, const TCHAR* Order)
{
	TUniquePtr<FS47Node> N = MakeUnique<FS47Node>();
	N->Position = P; N->Rotation = R; N->Scale = S; N->Order = Order;
	FS47Node* Up = Parent ? Parent : &Root();
	if (Web.IsValid() && Up->C.IsValid())
	{
		USceneComponent* C = NewObject<USceneComponent>(Web.Get());
		C->SetMobility(EComponentMobility::Movable);
		C->SetupAttachment(Up->C.Get());
		C->RegisterComponent();
		N->C = C;
	}
	N->Parent = Up;
	N->Apply();
	Nodes.Add(MoveTemp(N));
	return Nodes.Last().Get();
}

FS47Node* FS47Group::Mesh(FS47Node* Parent, const TCHAR* Shape, UMaterialInterface* M, const FVector& P, const FVector& R, const FVector& S, const TCHAR* Order)
{
	TUniquePtr<FS47Node> N = MakeUnique<FS47Node>();
	N->Position = P; N->Rotation = R; N->Scale = S; N->Order = Order;
	FS47Node* Up = Parent ? Parent : &Root();
	if (Web.IsValid() && Up->C.IsValid())
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Web.Get());
		C->SetMobility(EComponentMobility::Movable);
		C->SetupAttachment(Up->C.Get());
		C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Shape, Shape)));
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->bAffectDistanceFieldLighting = false;
		if (M) C->SetMaterial(0, M);
		C->RegisterComponent();
		N->C = C;
	}
	N->Parent = Up;
	N->Apply();
	Nodes.Add(MoveTemp(N));
	Refresh();
	return Nodes.Last().Get();
}

FS47Node* FS47Group::Box(FS47Node* Parent, double Wd, double Ht, double Dp, UMaterialInterface* M, const FVector& P, const FVector& R, const FVector& S)
{
	// BoxGeometry(w, h, d): the 1 m cube scaled (a scale on the mesh goes onto the box's own)
	FS47Node* H = Group(Parent, P, R, S);
	Mesh(H, TEXT("Cube"), M, FVector::ZeroVector, FVector::ZeroVector, FVector(Wd, Ht, Dp));
	return H;
}

FS47Node* FS47Group::Sphere(FS47Node* Parent, double Rad, UMaterialInterface* M, const FVector& P, const FVector& S)
{
	FS47Node* H = Group(Parent, P, FVector::ZeroVector, S);
	Mesh(H, TEXT("Sphere"), M, FVector::ZeroVector, FVector::ZeroVector, FVector(2 * Rad));
	return H;
}

FS47Node* FS47Group::Capsule(FS47Node* Parent, double Rad, double Len, UMaterialInterface* M, const FVector& P, const FVector& R, const FVector& S)
{
	// CapsuleGeometry(radius, length): a cylinder `length` long about y, a hemisphere at each end
	FS47Node* H = Group(Parent, P, R, S);
	Mesh(H, TEXT("Cylinder"), M, FVector::ZeroVector, FVector::ZeroVector, FVector(2 * Rad, Len, 2 * Rad));
	Mesh(H, TEXT("Sphere"), M, FVector(0, Len / 2, 0), FVector::ZeroVector, FVector(2 * Rad));
	Mesh(H, TEXT("Sphere"), M, FVector(0, -Len / 2, 0), FVector::ZeroVector, FVector(2 * Rad));
	return H;
}

FS47Node* FS47Group::Plane(FS47Node* Parent, double Wd, double Ht, UMaterialInterface* M, const FVector& P, const FVector& R)
{
	// PlaneGeometry(w, h) faces +Z; the engine's plane lies flat (facing three's +Y): stood up by a
	// quarter turn about x, its far edge (-z) up
	FS47Node* H = Group(Parent, P, R);
	Mesh(H, TEXT("Plane"), M, FVector::ZeroVector, FVector(UE_DOUBLE_HALF_PI, 0, 0), FVector(Wd, 1, Ht));
	return H;
}

FS47Node* FS47Group::Sprite(FS47Node* Parent, UMaterialInstanceDynamic* M, const FVector& P)
{
	FS47Node* Up = Parent ? Parent : &Root();
	if (!Web.IsValid() || !Up->C.IsValid()) return nullptr;
	UMaterialBillboardComponent* B = NewObject<UMaterialBillboardComponent>(Web.Get());
	B->SetupAttachment(Up->C.Get());
	B->AddElement(M, nullptr, false, 50.f, 50.f, nullptr);
	B->RegisterComponent();
	TUniquePtr<FS47Node> N = MakeUnique<FS47Node>();
	N->C = B; N->Position = P; N->Parent = Up;
	N->Apply();
	Nodes.Add(MoveTemp(N));
	Refresh();
	return Nodes.Last().Get();
}

void FS47Group::SpriteSize(FS47Node* N, double Wd, double Ht)
{
	// (sprite.scale.set(w, h, 1): the element's size is half of it each way, in centimetres)
	UMaterialBillboardComponent* B = N ? Cast<UMaterialBillboardComponent>(N->C.Get()) : nullptr;
	if (!B || !B->Elements.Num()) return;
	B->Elements[0].BaseSizeX = float(Wd * 50.0);
	B->Elements[0].BaseSizeY = float(Ht * 50.0);
	B->MarkRenderStateDirty();
}

void FS47Group::RenderOrder(int32 Order)
{
	for (const TUniquePtr<FS47Node>& N : Nodes) if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(N->C.Get())) P->SetTranslucentSortPriority(Order);
}

// ---------- story/recording.ts Walker ----------

namespace
{
	/** Far off, the band's glint keeps about the same few pixels on any screen (recording.ts GLINT_FAR). */
	const double GLINT_FAR = 1 / (28 * FMath::Tan(FMath::DegreesToRadians(35.0)));
	FLinearColor Lerp(uint32 A, uint32 B, double K) { const FLinearColor Ca = Hex(A), Cb = Hex(B); return Ca + (Cb - Ca) * float(K); }
}

FS47JacketWalker::FS47JacketWalker(AS47Web* Web, const FString& Name)
{
	G = MakeUnique<FS47Group>(Web, Name);
	Coat = G->Basic(Hex(0x2a3036)); Dark = G->Basic(Hex(0x15171a));
	Band = G->Basic(Hex(0xd8dccc));
	UMaterialInstanceDynamic* Skin = G->Basic(Hex(0x6e5a4c));
	Body = G->Group(&G->Root());
	for (int32 k = 0; k < 2; ++k)
	{
		const double Sd = k ? 1 : -1;
		FS47Node* Hip = G->Group(Body, FVector(Sd * 0.11, 0.84, 0));
		G->Box(Hip, 0.2, 0.84, 0.2, Dark, FVector(0, -0.42, 0));
		Legs[k] = Hip;
		FS47Node* Sh = G->Group(Body, FVector(Sd * 0.32, 1.42, 0));
		G->Box(Sh, 0.12, 0.6, 0.14, Coat, FVector(0, -0.3, 0));
		// (and round each sleeve, as the field jackets have it: it swings with the arm)
		Tapes.Add(G->Box(Sh, 0.13, 0.05, 0.15, Band, FVector(0, -0.24, 0)));
		Arms[k] = Sh;
	}
	G->Box(Body, 0.5, 0.62, 0.28, Coat, FVector(0, 1.14, 0));
	Tapes.Insert(G->Box(Body, 0.52, 0.08, 0.3, Band, FVector(0, 1.2, 0)), 0);
	Head = G->Sphere(Body, 0.13, Skin, FVector(0, 1.6, 0));
	// the tape gives a beam back as a glint that carries much further than the jacket can be made
	// out: across the chest, and a small one at each sleeve that swings with the arm
	// (only in a world: without one, as in the self test, there is nothing to draw them in)
	if (UMaterialInterface* GlowBase = Web ? S47Figures::Base(TEXT("M_WebGlow")) : nullptr)
	{
		GlintMat = UMaterialInstanceDynamic::Create(GlowBase, Web);
		GlintMat->SetVectorParameterValue(TEXT("Color"), Hex(0xfff1da));
		GlintMat->SetScalarParameterValue(TEXT("Brightness"), 0.f);
		for (int32 i = 0; i < 3; ++i)
		{
			FS47Node* S = G->Sprite(&G->Root(), GlintMat, FVector(0, i ? 1.18 : 1.2, 0));
			if (!S) continue;
			S->bVisible = false;
			Glints.Add(S);
		}
		G->Refresh();
	}
	for (FS47Node* S : Glints) if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(S->C.Get())) P->SetTranslucentSortPriority(3);
}

void FS47JacketWalker::Shine(double K, const FS47Cam* Cam)
{
	if (FMath::Abs(K - Lit) >= 0.02 || (K == 0 && Lit != 0))
	{
		Lit = K;
		S47Figures::SetColor(Band, FLinearColor(float(0.69 + 2.2 * K), float(0.72 + 2.2 * K), float(0.6 + 1.9 * K)));
		// (and the jacket and the jeans a little out of the dark in the beam, enough to be someone)
		S47Figures::SetColor(Coat, Lerp(0x2a3036, 0x737a82, K));
		S47Figures::SetColor(Dark, Lerp(0x15171a, 0x464a52, K));
		if (GlintMat) GlintMat->SetScalarParameterValue(TEXT("Brightness"), float(0.9 * FMath::Pow(FMath::Min(1.0, 1.15 * K), 1.5)));
	}
	const bool bOn = Lit > 0.04;
	bool bChanged = false;
	for (FS47Node* S : Glints) if (S->bVisible != bOn) { S->bVisible = bOn; bChanged = true; }
	if (bChanged) G->Refresh();
	if (!bOn) return;
	const FVector GroupPos = G->Root().Position;
	const double Half = Cam ? FMath::Tan(FMath::DegreesToRadians(Cam->Fov / 2)) : 0.7;
	const double S = FMath::Max(1.0, (Cam ? FVector::Dist(Cam->Pos, GroupPos) : 40.0) * Half * GLINT_FAR);
	if (Glints.Num() == 3)
	{
		G->SpriteSize(Glints[0], 0.7 * S, 0.16 * S);
		G->SpriteSize(Glints[1], 0.18 * S, 0.18 * S); G->SpriteSize(Glints[2], 0.18 * S, 0.18 * S);
	}
	if (!Cam) return;
	// (each is put on the side of its tape that faces the eye, so the jacket does not hide it)
	for (int32 i = 0; i < Tapes.Num() && i < Glints.Num(); ++i)
	{
		FVector At = Tapes[i]->WorldPosition();
		FVector To = Cam->Pos - At; To.Y = 0;
		To = To.GetSafeNormal();
		Glints[i]->Position = G->Root().WorldToLocal(At + To * 0.3);
		Glints[i]->Apply();
	}
}

void FS47JacketWalker::Pose(double Swing, double Wide, double Lean, double Roll, double Lift)
{
	const double Leg = Wide * 0.62, Arm = Wide * 0.5;
	Legs[0]->Rotation.X = Swing * Leg; Legs[1]->Rotation.X = -Swing * Leg;
	Arms[0]->Rotation.X = -Swing * Arm; Arms[1]->Rotation.X = Swing * Arm;
	// (a hand out a little from the side when it runs)
	Arms[0]->Rotation.Z = -0.08 * Wide; Arms[1]->Rotation.Z = 0.08 * Wide;
	Body->Rotation = FVector(-Lean, 0, Roll);
	Body->Position.Y = Lift;
	for (FS47Node* N : { Legs[0], Legs[1], Arms[0], Arms[1], Body }) N->Apply();
}

// ---------- core/roadside.ts Silhouette (the painted outlines) ----------

namespace
{
	// The pictures are twice as high as wide, the figure in them 88 per cent of the height with its
	// feet 7 per cent above the bottom: a card 1.36 m high stands a figure of 1.2 m on the ground.
	const double CARD_H = 1.36, CARD_W = 0.68, FEET = 0.074 * CARD_H;
	/** The light along the pictures' edge is a cold grey; the colour given is what it should be. */
	const uint32 EDGE = 0x141923;
	/** The middle of the head in the standing pictures, metres above the ground. */
	const double HEAD_MID = 1.1;
	const TCHAR* PoseArt(FS47CardSilhouette::EPose P)
	{
		switch (P)
		{
		case FS47CardSilhouette::EPose::Turned: return TEXT("silhouetteTurned");
		case FS47CardSilhouette::EPose::Far: return TEXT("silhouetteFar");
		default: return TEXT("silhouetteStand");
		}
	}

	/** The standing picture with the head bent (roadside.ts makeBent): cut off at the neck (the narrowest
	 *  row under it) and put back lower, into the shoulders, tipped a little to one side: from below, a
	 *  head looking down at you. Made once from the picture itself. */
	struct FBent { TStrongObjectPtr<UTexture2D> Tex; double Head = HEAD_MID; };
	TOptional<FBent> Bent;
	bool bBentTried = false;
	const FBent* BentArt()
	{
		if (bBentTried) return Bent.GetPtrOrNull();
		FS47Pix Im;
		if (!S47Raster::Art(TEXT("silhouetteStand"), Im)) return nullptr;
		bBentTried = true;
		const int32 W = Im.W, H = Im.H;
		FS47Pix G(W, H);
		G.DrawImage(Im, 0, 0, W, H);
		const TArray<FColor> A = G.Colors();
		auto Row = [&](int32 Y) { int32 N = 0; double Sx = 0; for (int32 X = 0; X < W; ++X) if (A[Y * W + X].A > 128) { N++; Sx += X; } return TPair<int32, double>(N, N ? Sx / N : W / 2.0); };
		int32 Top = 0;
		while (Top < H && Row(Top).Key == 0) Top++;
		int32 Neck = Top + int32(JsRound(H * 0.16));
		int32 Nw = TNumericLimits<int32>::Max();
		for (int32 Y = Top + int32(JsRound(H * 0.08)); Y < Top + int32(JsRound(H * 0.24)) && Y < H; ++Y) { const TPair<int32, double> R = Row(Y); if (R.Key > 0 && R.Key < Nw) { Nw = R.Key; Neck = Y; } }
		const double Cx = Row(FMath::Clamp(Neck, 0, H - 1)).Value;
		const int32 HeadH = Neck - Top;
		// down by about a third of the head, into the shoulders; tipped to one side, about 15 degrees
		const int32 Drop = int32(JsRound(HeadH * 0.34));
		const double Tilt = -0.26;
		G.ClearRect(0, 0, W, Neck);
		G.Save(); G.Translate(Cx, Neck + Drop); G.Rotate(Tilt); G.Scale(1.06, 1.06);
		G.DrawImage(Im, 0, 0, W, Neck, -Cx, -Neck, W, Neck);
		G.Restore();
		FBent B;
		B.Tex.Reset(S47Image::FromPixels(G.Colors(), W, H));
		// where the head's middle is now, in the card's metres above the feet
		const double Mid = Top + HeadH / 2.0 + Drop;
		B.Head = CARD_H - FEET - (Mid / H) * CARD_H;
		UE_LOG(LogTemp, Display, TEXT("S47: the bent outline made: neck at row %d of %d, the head down %d px, its middle %.2f m up"), Neck, H, Drop, B.Head);
		Bent = MoveTemp(B);
		return Bent.GetPtrOrNull();
	}
}

FS47CardSilhouette::FS47CardSilhouette(AS47Web* Web, const FString& Name, uint32 Edge, EPose InWant) : Want(InWant)
{
	G = MakeUnique<FS47Group>(Web, Name);
	if (!S47Raster::ArtLoaded(PoseArt(EPose::Stand))) return;   // without them, nothing stands
	Mat = G->Basic(FLinearColor::White, TEXT("M_Web_Unlit_Trans_2S"));
	if (!Mat) return;
	S47Figures::SetMap(Mat, S47Image::Art(PoseArt(EPose::Stand)));
	Card = G->Group(&G->Root());
	G->Plane(Card, CARD_W, CARD_H, Mat, FVector(0, CARD_H / 2 - FEET, 0));
	bPainted = true;
	Pose(Want); Rim(Edge);
}

void FS47CardSilhouette::Pose(EPose P)
{
	Want = P;
	if (!Mat) return;
	const FBent* B = P == EPose::Bent ? BentArt() : nullptr;
	UTexture* T = B && B->Tex.IsValid() ? B->Tex.Get() : S47Image::Art(S47Raster::ArtLoaded(PoseArt(P)) ? PoseArt(P) : PoseArt(EPose::Stand));
	S47Figures::SetMap(Mat, T);
}

double FS47CardSilhouette::HeadY() const
{
	if (bPainted && Want == EPose::Bent) { const FBent* B = BentArt(); return B ? B->Head : HEAD_MID; }
	return HEAD_MID;
}

void FS47CardSilhouette::Rim(uint32 HexC)
{
	if (!Mat) return;
	const FLinearColor C = Hex(HexC), E = Hex(EDGE);
	S47Figures::SetColor(Mat, FLinearColor(FMath::Min(4.f, C.R / E.R), FMath::Min(4.f, C.G / E.G), FMath::Min(4.f, C.B / E.B)));
}

void FS47CardSilhouette::Face(const FVector& Eye)
{
	if (!Card) return;
	// card.matrixWorld = makeRotationY(atan2(c.x - p.x, c.z - p.z)) at its own place: the group's own
	// turn taken back off (the figures over you are only ever placed, not turned)
	const FVector P = G->Root().WorldPosition();
	Card->Rotation = FVector(0, FMath::Atan2(Eye.X - P.X, Eye.Z - P.Z) - G->Root().Rotation.Y, 0);
	Card->Apply();
}

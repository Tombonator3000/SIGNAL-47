// core/FieldCamera.ts
#include "S47FieldCamera.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "S47Canvas.h"
#include "S47Freeform.h"
#include "S47HUD.h"
#include "S47Image.h"
#include "S47Json.h"
#include "S47Main.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "SS47Widgets.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47FieldCamera, Log, All);

// ---------- Photo ----------

TSharedPtr<FJsonObject> FS47Photo::ToJson() const
{
	S47J::FObj O = S47J::New();
	S47J::Set(O, TEXT("id"), Id);
	S47J::Set(O, TEXT("subject"), Subject);
	S47J::Set(O, TEXT("method"), Method);
	S47J::Set(O, TEXT("clock"), Clock);
	S47J::Set(O, TEXT("url"), Url);
	S47J::FObj T = S47J::New();
	for (const TPair<FString, FVector2D>& K : Targets) S47J::Set(T, *K.Key, TArray<double>{ K.Value.X, K.Value.Y });
	S47J::Set(O, TEXT("targets"), T);
	return O;
}

TOptional<FS47Photo> FS47Photo::FromJson(const TSharedPtr<FJsonObject>& O)
{
	if (!O.IsValid() || !S47J::Has(O, TEXT("id"))) return {};
	FS47Photo P;
	P.Id = S47J::Str(O, TEXT("id"));
	P.Subject = S47J::Str(O, TEXT("subject"));
	P.Method = S47J::Str(O, TEXT("method"));
	P.Clock = S47J::Num(O, TEXT("clock"));
	P.Url = S47J::Str(O, TEXT("url"));
	if (const S47J::FObj T = S47J::Obj(O, TEXT("targets")))
	{
		for (const auto& KV : T->Values)
		{
			const FString K(KV.Key.ToView());
			if (const TOptional<FVector2D> Uv = S47J::Uv(T, *K)) P.Targets.Add(TPair<FString, FVector2D>(K, *Uv));
		}
	}
	return P;
}

// ---------- the camera ----------

FS47FrameRect US47FieldCamera::FrameRect() const
{
	// The viewfinder frame on screen. The overlay is drawn from the same numbers, in the HUD's own
	// units (innerWidth and innerHeight are the page's CSS pixels; Slate's units are the same idea).
	FVector2D S(0, 0);
	if (M && M->Hud)
	{
		if (SS47Hud* Hw = M->Hud->HudWidget()) S = FVector2D(Hw->GetTickSpaceGeometry().GetLocalSize());
	}
	if ((S.X <= 1 || S.Y <= 1) && GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(S);
	if (S.X <= 1 || S.Y <= 1) S = FVector2D(1600, 900);
	const double W = S.X, H = S.Y;
	const double w = FMath::Min(W * 0.92, H * 0.84 * 1.6);
	const double h = w / 1.6;
	FS47FrameRect R;
	R.X = (W - w) / 2; R.Y = (H - h) / 2; R.W = w; R.H = h; R.Frac = h / H;
	return R;
}

void US47FieldCamera::Raise(bool bOn)
{
	if (bOn == bRaised) return;
	bRaised = bOn;
	AS47Walker* Cam = M ? M->Player.Get() : nullptr;
	if (!Cam) return;
	// (the walker sets its camera from Fov every frame: three's updateProjectionMatrix)
	if (bOn) SavedFov = Cam->Fov;
	else Cam->Fov = SavedFov;
}

void US47FieldCamera::Sync()
{
	const FS47FrameRect R = FrameRect();
	AS47Walker* Cam = M ? M->Player.Get() : nullptr;
	if (!Cam) return;
	if (bRaised)
	{
		const double Fov = 2 * FMath::Atan(FMath::Tan(PHOTO_FOV * PI / 360) / R.Frac) * 180 / PI;
		if (FMath::Abs(Cam->Fov - Fov) > 0.01) Cam->Fov = float(Fov);
	}
	// photoCam.position / quaternion copy the camera's (the walker's camera sits at its root)
	PhotoLoc = Cam->EyeUE();
	PhotoRot = Cam->GetActorRotation();
}

FS47Projection US47FieldCamera::Project(const FVector& P) const
{
	// p.project(photoCam): the camera looks along Unreal's +X, its right is +Y and up +Z, which
	// are three's camera -Z, +X and +Y (S47Three's axes), so the division is the same
	const FVector L = PhotoRot.UnrotateVector(S47Three::ToUE(P) - PhotoLoc);
	const double TanV = FMath::Tan(PHOTO_FOV * PI / 360), TanH = TanV * PHOTO_W / PHOTO_H;
	const double Nx = (L.Y / L.X) / TanH, Ny = (L.Z / L.X) / TanV;
	FS47Projection R;
	R.U = (Nx + 1) / 2;
	R.V = (1 - Ny) / 2;
	R.bFront = L.X > 0;
	return R;
}

bool US47FieldCamera::InFrame(const FVector& P, double X0, double X1, double Y0, double Y1) const
{
	const FS47Projection R = Project(P);
	// limits are given like Unity viewport coordinates, y from the bottom
	const double Yb = 1 - R.V;
	return R.bFront && R.U >= X0 && R.U <= X1 && Yb >= Y0 && Yb <= Y1;
}

bool US47FieldCamera::ClearLine(const FVector& P, const TArray<FS47Obj>& Occluders) const
{
	// Raycaster(o, d, 0.1, len - 0.3).intersectObjects(occluders, true): every mesh in and under
	// them, seen or not (three's rays hit hidden meshes too), traced one by one
	const FVector O = S47Three::ToThree(PhotoLoc);
	FVector D = P - O;
	const double Len = D.Size();
	if (Len - 0.3 <= 0.1) return true;
	D /= Len;
	const FVector A = S47Three::ToUE(O + D * 0.1), B = S47Three::ToUE(O + D * (Len - 0.3));
	FCollisionQueryParams Q(SCENE_QUERY_STAT(S47ClearLine), true);
	for (const FS47Obj& Oc : Occluders)
	{
		bool bHit = false;
		Oc.Traverse([&](FS47Obj N)
		{
			if (bHit) return;
			const FS47WebNode* Wn = N.W ? N.W->Node(N.I) : nullptr;
			UPrimitiveComponent* C = Wn ? Cast<UPrimitiveComponent>(Wn->Comp.Get()) : nullptr;
			FHitResult H;
			if (C && C->LineTraceComponent(H, A, B, Q)) bHit = true;
		});
		if (bHit) return false;
	}
	return true;
}

bool US47FieldCamera::Render(int32 RW, int32 RH, TArray<FColor>& Out)
{
	// r.setSize(w, h); r.toneMappingExposure = exp * 1.9; r.render(scene, photoCam): a scene
	// capture from the photo camera, through the world's own post process (M_WebTone: fog, ACES,
	// sRGB). Its exposure is MPC_Web's ToneK (exposure / 0.6), raised for the one render and put
	// back; the tape (core/vhs.ts) is the screen's composer only, so it is off for the film.
	// (glowScale sizes three's glow sprites in screen pixels; the glow billboards here are sized in
	// metres, so the render size does not change them. ext.dishArray.cull(photoCam) only hides the
	// instanced dishes outside the view; Unreal culls instances per view by itself.)
	UWorld* World = M && M->Web ? M->Web->GetWorld() : nullptr;
	if (!World) return false;
	if (!Capture.IsValid())
	{
		FActorSpawnParameters Sp;
		Sp.ObjectFlags |= RF_Transient;
		Capture = World->SpawnActor<ASceneCapture2D>(Sp);
		if (!Capture.IsValid()) return false;
	}
	if (!Target || Target->SizeX != RW || Target->SizeY != RH)
	{
		Target = NewObject<UTextureRenderTarget2D>(this);
		Target->ClearColor = FLinearColor::Black;
		// the bytes M_WebTone writes (it encodes sRGB itself): read back as they are
		Target->InitCustomFormat(RW, RH, PF_B8G8R8A8, true);
		Target->UpdateResourceImmediate(true);
	}
	USceneCaptureComponent2D* C = Capture->GetCaptureComponent2D();
	C->TextureTarget = Target;
	C->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	C->bCaptureEveryFrame = false;
	C->bCaptureOnMovement = false;
	C->bAlwaysPersistRenderingState = true; // a view state: without one the world's post-process materials (M_WebTone) are skipped
	C->FOVAngle = S47Three::HorizontalFov(float(PHOTO_FOV), float(PHOTO_W) / float(PHOTO_H));
	C->bOverride_CustomNearClippingPlane = true;
	C->CustomNearClippingPlane = 5.f; // 0.05 m
	C->PostProcessBlendWeight = 1.f;
	C->PostProcessSettings.bOverride_ReflectionMethod = true; // as the screen (r.ReflectionMethod=2)
	C->PostProcessSettings.ReflectionMethod = EReflectionMethod::ScreenSpace;
	C->ShowFlags.SetTemporalAA(false); // one frame, no history
	Capture->SetActorLocationAndRotation(PhotoLoc, PhotoRot);

	UMaterialParameterCollection* Env = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Web/Base/MPC_Web.MPC_Web"));
	UMaterialParameterCollectionInstance* E = Env ? World->GetParameterCollectionInstance(Env) : nullptr;
	float ToneK = 1.75f, Vhs = 0.f;
	if (E)
	{
		E->GetScalarParameterValue(TEXT("ToneK"), ToneK);
		E->GetScalarParameterValue(TEXT("Vhs"), Vhs);
		// M7 (Tom, 9 November): the image intensifier on the lens (FS47Openings "night_camera") sees far more in the dark; by day it is off
		const bool bNightGlass = M && M->IsFreeform() && M->Freeform && M->Freeform->HasKit(TEXT("night_camera"))
			&& (FMath::Fmod(M->Freeform->Clock(), 86400.0) >= 19 * 3600.0 || FMath::Fmod(M->Freeform->Clock(), 86400.0) < 6 * 3600.0);
		E->SetScalarParameterValue(TEXT("ToneK"), ToneK * (bNightGlass ? 3.6f : 1.9f)); // long exposure: the film sees more than the eye
		E->SetScalarParameterValue(TEXT("Vhs"), 0.f);
	}
	// (CaptureScene sends the world's pending updates first: the echo shown, the exposure raised)
	if (OnExposure) OnExposure(true);
	C->CaptureScene();
	if (OnExposure) OnExposure(false);
	if (E)
	{
		E->SetScalarParameterValue(TEXT("ToneK"), ToneK);
		E->SetScalarParameterValue(TEXT("Vhs"), Vhs);
	}
	Out.Reset();
	if (!S47Image::ReadPixels(Target, Out) || Out.Num() != RW * RH) return false;
	for (FColor& Px : Out) Px.A = 255;
	return true;
}

bool US47FieldCamera::Expose(const FS47ExposeOptions& O, TArray<FColor>& OutPrint)
{
	if (O.Before) O.Before();
	TArray<FColor> Raw;
	const bool bOk = Render(PHOTO_W, PHOTO_H, Raw);
	if (O.After) O.After();
	if (O.Restore) O.Restore();
	if (!bOk)
	{
		UE_LOG(LogS47FieldCamera, Warning, TEXT("S47: the field camera could not render; an unexposed frame"));
		Raw.Init(FColor(0, 0, 0, 255), PHOTO_W * PHOTO_H);
	}
	if (O.Post) O.Post(Raw, PHOTO_W, PHOTO_H);
	MakePrint(Raw, O.Caption, O.Time, O.Seed, OutPrint);
	return bOk;
}

FS47Snap US47FieldCamera::Snap(const FS47SnapOptions& O)
{
	FS47Snap S;
	S.W = O.W;
	S.H = FMath::RoundToInt(O.W * double(PHOTO_H) / PHOTO_W);
	const int32 Up = FMath::Clamp(O.Super, 1, 3);
	if (O.Before) O.Before();
	TArray<FColor> Big;
	const bool bOk = Up > 1 ? Render(S.W * Up, S.H * Up, Big) : Render(S.W, S.H, S.Px); S.bRendered=bOk;
	if (O.After) O.After();
	if (O.Restore) O.Restore();
	if (bOk && Up > 1 && Big.Num() == S.W * Up * S.H * Up)
	{
		// The negative holds what the print is too small to show: the print is its average, Up by Up.
		if (O.Post) O.Post(Big, S.W * Up, S.H * Up);
		S.Px.SetNumUninitialized(S.W * S.H);
		for (int32 Y = 0; Y < S.H; ++Y) for (int32 X = 0; X < S.W; ++X)
		{
			int32 R = 0, G = 0, B = 0;
			for (int32 Dy = 0; Dy < Up; ++Dy) for (int32 Dx = 0; Dx < Up; ++Dx) { const FColor& C = Big[(Y * Up + Dy) * S.W * Up + X * Up + Dx]; R += C.R; G += C.G; B += C.B; }
			const int32 K = Up * Up;
			S.Px[Y * S.W + X] = FColor(uint8(R / K), uint8(G / K), uint8(B / K), 255);
		}
		S.Neg = MoveTemp(Big); S.NegW = S.W * Up; S.NegH = S.H * Up;
	}
	else
	{
		if (!bOk || S.Px.Num() != S.W * S.H) S.Px.Init(FColor(0, 0, 0, 255), S.W * S.H);
		if (O.Post) O.Post(S.Px, S.W, S.H);
	}
	S.Mean = FilmLook(S.Px, S.W, S.H, O.Seed);
	return S;
}

uint32 US47FieldCamera::ToUint32(double V)
{
	if (!FMath::IsFinite(V)) return 0;
	const double T = V < 0 ? FMath::CeilToDouble(V) : FMath::FloorToDouble(V);
	double R = FMath::Fmod(T, 4294967296.0);
	if (R < 0) R += 4294967296.0;
	return uint32(R);
}

double US47FieldCamera::FilmLook(TArray<FColor>& Raw, int32 W, int32 H, double Seed)
{
	// Film look: lifted blacks, warm cast, grain, vignette (in place). Returns the mean brightness.
	uint32 S = ToUint32(Seed);
	double Sum = 0;
	auto Rnd = [&S] { S = S * 1664525u + 1013904223u; return S / 4294967296.0; };
	for (int32 Y = 0; Y < H; ++Y)
	{
		for (int32 X = 0; X < W; ++X)
		{
			FColor& D = Raw[Y * W + X];
			const double Dx = (double(X) / W - 0.5) * 1.6, Dy = double(Y) / H - 0.5;
			const double Vig = 1 - FMath::Min(0.55, (Dx * Dx + Dy * Dy) * 0.9);
			const double N = (Rnd() - 0.5) * 22;
			// a little less colour than the screen: night film under sodium light
			const double Lum = D.R * 0.3 + D.G * 0.59 + D.B * 0.11;
			const double R = D.R * 0.78 + Lum * 0.22, Gg = D.G * 0.78 + Lum * 0.22, B = D.B * 0.78 + Lum * 0.22;
			D.R = FS47Canvas::Clamp8(FMath::Min(255.0, (10 + R * 1.04) * Vig + N));
			D.G = FS47Canvas::Clamp8(FMath::Min(255.0, (9 + Gg * 0.99) * Vig + N));
			D.B = FS47Canvas::Clamp8(FMath::Min(255.0, (12 + B * 0.9) * Vig + N));
			Sum += D.R * 0.3 + D.G * 0.59 + D.B * 0.11;
		}
	}
	return Sum / (double(W) * H);
}

FString US47FieldCamera::DataBackText(const FDateTime& Date)
{
	// The American order of a back bought in New Mexico: month, day, the year's last two figures behind a tick.
	return FString::Printf(TEXT("%d %d '%02d"), Date.GetMonth(), Date.GetDay(), Date.GetYear() % 100);
}

void US47FieldCamera::DataBack(TArray<FColor>& Px, int32 W, int32 H, const FString& Text)
{
	if (W <= 0 || H <= 0 || Px.Num() < W * H || Text.IsEmpty()) return;
	// A seven-segment cell: a b c d e f g as bits (a the top, b upper right, c lower right, d the bottom, e lower left, f upper left,
	// g the middle), each a stroke between two of the cell's six corner points.
	static const uint8 Digits[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };
	static const int32 Ends[7][4] = { { 0, 0, 1, 0 }, { 1, 0, 1, 1 }, { 1, 1, 1, 2 }, { 0, 2, 1, 2 }, { 0, 1, 0, 2 }, { 0, 0, 0, 1 }, { 0, 1, 1, 1 } };
	// The cell: the digits' height a twentieth of the frame (nine pixels on the small print, eighteen on the negative), slanted a
	// little as the LEDs were, sitting in from the lower right corner.
	const double Ch = FMath::Max(7., H / 22.), Cw = Ch * .56, Gap = Ch * .38, Stroke = FMath::Max(1.2, Ch / 7.), Slant = .14;
	const int32 N = Text.Len();
	const double Right = W - W * .045, Bottom = H - H * .07, Top = Bottom - Ch;
	double X0 = Right - (N * Cw + (N - 1) * Gap);
	struct FStroke { double Ax, Ay, Bx, By; };
	TArray<FStroke> Strokes;
	for (int32 I = 0; I < N; ++I, X0 += Cw + Gap)
	{
		const TCHAR C = Text[I];
		uint8 Mask = 0;
		if (C >= '0' && C <= '9') Mask = Digits[C - '0'];
		const auto At = [&](int32 Cx, int32 Cy, double& Ox, double& Oy) { Oy = Top + Cy * Ch / 2; Ox = X0 + Cx * Cw + (Bottom - Oy) * Slant; };
		for (int32 S = 0; S < 7; ++S) if (Mask & (1 << S))
		{
			FStroke K; At(Ends[S][0], Ends[S][1], K.Ax, K.Ay); At(Ends[S][2], Ends[S][3], K.Bx, K.By); Strokes.Add(K);
		}
		if (C == '\'') { FStroke K; At(0, 0, K.Ax, K.Ay); K.Bx = K.Ax - Cw * .15; K.By = K.Ay + Ch * .3; K.Ax += Cw * .1; Strokes.Add(K); }
		if (C == ':') for (int32 D = 0; D < 2; ++D) { FStroke K; At(0, 0, K.Ax, K.Ay); K.Ax += Cw * .5; K.Ay += Ch * (D ? .72 : .28); K.Bx = K.Ax + .01; K.By = K.Ay; Strokes.Add(K); }
	}
	if (Strokes.IsEmpty()) return;
	// Burned in: an orange LED through the film, hot in the stroke, with a soft bleed round it; shortened a little at each end so the
	// segments do not run together at the corners.
	const double R = Stroke / 2, Bleed = Stroke * 1.4;
	const int32 Y0 = FMath::Clamp(int32(Top - Bleed - 2), 0, H - 1), Y1 = FMath::Clamp(int32(Bottom + Bleed + 2), 0, H - 1);
	const int32 Xa = FMath::Clamp(int32(Right - (N * Cw + (N - 1) * Gap) - Bleed - 2), 0, W - 1), Xb = FMath::Clamp(int32(Right + Ch * Slant + Bleed + 2), 0, W - 1);
	for (int32 Y = Y0; Y <= Y1; ++Y) for (int32 X = Xa; X <= Xb; ++X)
	{
		double Near = 1e9;
		for (const FStroke& K : Strokes)
		{
			const double Dx = K.Bx - K.Ax, Dy = K.By - K.Ay, L2 = Dx * Dx + Dy * Dy;
			double T = L2 > 1e-9 ? ((X + .5 - K.Ax) * Dx + (Y + .5 - K.Ay) * Dy) / L2 : 0.;
			T = FMath::Clamp(T, .08, .92);
			const double Px_ = K.Ax + Dx * T, Py_ = K.Ay + Dy * T;
			Near = FMath::Min(Near, FMath::Sqrt(FMath::Square(X + .5 - Px_) + FMath::Square(Y + .5 - Py_)));
		}
		const double Hot = FMath::Clamp((R + .6 - Near) / 1.2, 0., 1.), Soft = FMath::Clamp(1. - Near / Bleed, 0., 1.);
		const double A = Hot + .22 * Soft * Soft * (1. - Hot);
		if (A <= 0) continue;
		FColor& D = Px[Y * W + X];
		D.R = FS47Canvas::Clamp8(D.R + A * 250);
		D.G = FS47Canvas::Clamp8(D.G + A * 105);
		D.B = FS47Canvas::Clamp8(D.B + A * 16);
	}
}

// N1110 (D46-13): the trip camera's surveillance tape
namespace
{
	// 5x7 dots, rows top to bottom, bit 4 the leftmost dot
	struct FTapeGlyph { TCHAR C; uint8 Row[7]; };
	const FTapeGlyph TapeGlyphs[] = {
		{ '0', { 14, 17, 19, 21, 25, 17, 14 } }, { '1', { 4, 12, 4, 4, 4, 4, 14 } }, { '2', { 14, 17, 1, 2, 4, 8, 31 } }, { '3', { 31, 2, 4, 2, 1, 17, 14 } },
		{ '4', { 2, 6, 10, 18, 31, 2, 2 } }, { '5', { 31, 16, 30, 1, 1, 17, 14 } }, { '6', { 6, 8, 16, 30, 17, 17, 14 } }, { '7', { 31, 1, 2, 4, 8, 8, 8 } },
		{ '8', { 14, 17, 17, 14, 17, 17, 14 } }, { '9', { 14, 17, 17, 15, 1, 2, 12 } }, { '/', { 1, 1, 2, 4, 8, 16, 16 } }, { ':', { 0, 12, 12, 0, 12, 12, 0 } },
		{ '.', { 0, 0, 0, 0, 0, 12, 12 } }, { '-', { 0, 0, 0, 31, 0, 0, 0 } },
		{ 'A', { 14, 17, 17, 31, 17, 17, 17 } }, { 'C', { 14, 17, 16, 16, 16, 17, 14 } }, { 'I', { 14, 4, 4, 4, 4, 4, 14 } }, { 'M', { 17, 27, 21, 21, 17, 17, 17 } },
		{ 'P', { 30, 17, 17, 30, 16, 16, 16 } }, { 'R', { 30, 17, 17, 30, 20, 18, 17 } }, { 'T', { 31, 4, 4, 4, 4, 4, 4 } },
	};
}

int32 US47FieldCamera::TripCamText(TArray<FColor>& Px, int32 W, int32 H, int32 X, int32 Y, int32 Scale, const FString& Text)
{
	if (W <= 0 || H <= 0 || Px.Num() < W * H || Scale < 1) return 0;
	const int32 Dot = Scale > 1 ? Scale - 1 : 1; // a gap between the dots, as a dot-matrix generator leaves it
	int32 Cx = X;
	for (const TCHAR Ch : Text)
	{
		const FTapeGlyph* G = nullptr;
		for (const FTapeGlyph& T : TapeGlyphs) if (T.C == Ch) { G = &T; break; }
		if (G) for (int32 R = 0; R < 7; ++R) for (int32 B = 0; B < 5; ++B) if (G->Row[R] & (16 >> B))
		{
			// a dark edge below and right keeps it readable over a pale sky, then the dot itself
			for (int32 Pass = 0; Pass < 2; ++Pass)
			{
				const int32 Ox = Cx + B * Scale + (Pass ? 0 : 1), Oy = Y + R * Scale + (Pass ? 0 : 1);
				for (int32 Dy = 0; Dy < Dot; ++Dy) for (int32 Dx = 0; Dx < Dot; ++Dx)
				{
					const int32 Px_ = Ox + Dx, Py_ = Oy + Dy;
					if (Px_ < 0 || Py_ < 0 || Px_ >= W || Py_ >= H) continue;
					Px[Py_ * W + Px_] = Pass ? FColor(214, 255, 214, 255) : FColor(0, 22, 0, 255);
				}
			}
		}
		Cx += 6 * Scale;
	}
	return Cx - X;
}

void US47FieldCamera::TripCamLook(TArray<FColor>& Px, int32 W, int32 H, double Seed, const FString& Stamp, const FString& Label)
{
	if (W < 8 || H < 8 || Px.Num() < W * H) return;
	uint32 S = ToUint32(Seed) ^ 0x7a3c5e11u;
	auto Rnd = [&S] { S = S * 1664525u + 1013904223u; return S / 4294967296.0; };
	// 1. the wide lens: what lies at a radius is taken from further out; scaled so the corners still land on the frame's edge
	const TArray<FColor> Src = Px;
	constexpr double K = 0.16;
	const double Norm = 1.0 / (1.0 + K * 2.0);
	for (int32 Y = 0; Y < H; ++Y) for (int32 X = 0; X < W; ++X)
	{
		const double U = (X + .5) / W * 2 - 1, V = (Y + .5) / H * 2 - 1;
		const double F = (1.0 + K * (U * U + V * V)) * Norm;
		const double Sx = ((U * F) * .5 + .5) * W - .5, Sy = ((V * F) * .5 + .5) * H - .5;
		const int32 X0 = FMath::Clamp(int32(FMath::FloorToDouble(Sx)), 0, W - 1), Y0 = FMath::Clamp(int32(FMath::FloorToDouble(Sy)), 0, H - 1);
		const int32 X1 = FMath::Min(X0 + 1, W - 1), Y1 = FMath::Min(Y0 + 1, H - 1);
		const double Fx = FMath::Clamp(Sx - X0, 0., 1.), Fy = FMath::Clamp(Sy - Y0, 0., 1.);
		const auto Ch = [](const FColor& P, int32 I) { return double(I == 0 ? P.R : I == 1 ? P.G : P.B); };
		double Out[3];
		for (int32 I = 0; I < 3; ++I)
		{
			const double A = FMath::Lerp(Ch(Src[Y0 * W + X0], I), Ch(Src[Y0 * W + X1], I), Fx), B = FMath::Lerp(Ch(Src[Y1 * W + X0], I), Ch(Src[Y1 * W + X1], I), Fx);
			Out[I] = FMath::Lerp(A, B, Fy);
		}
		FColor& D = Px[Y * W + X];
		D.R = FS47Canvas::Clamp8(Out[0]); D.G = FS47Canvas::Clamp8(Out[1]); D.B = FS47Canvas::Clamp8(Out[2]);
	}
	// 2. the tape: each line is pushed sideways a little in a few bands (tracking), smeared along the line, turned to a green monochrome,
	// scan lines, and noise that is stronger in a roll-bar band and now and then a dropout streak
	const int32 Bands = 5;
	TArray<int32> Shift; Shift.Init(0, Bands);
	for (int32 B = 0; B < Bands; ++B) Shift[B] = int32((Rnd() - .5) * W * .012);
	const double Bar = Rnd() * H;
	const int32 Drop = int32(Rnd() * H);
	TArray<FColor> Row; Row.SetNumUninitialized(W);
	for (int32 Y = 0; Y < H; ++Y)
	{
		const int32 Sh = Shift[Y * Bands / H];
		for (int32 X = 0; X < W; ++X) Row[X] = Px[Y * W + FMath::Clamp(X - Sh, 0, W - 1)];
		const double NearBar = FMath::Clamp(1. - FMath::Abs(Y - Bar) / (H * .05), 0., 1.);
		const double Line = (Y & 1) ? .86 : 1.;
		double Prev = Row[0].R * .3 + Row[0].G * .59 + Row[0].B * .11;
		for (int32 X = 0; X < W; ++X)
		{
			const FColor& C = Row[X];
			const double Lum = C.R * .3 + C.G * .59 + C.B * .11;
			Prev = Prev * .45 + Lum * .55; // the smear: the line drags a little of what came before it
			double V = (Prev * 1.12 + 10) * Line + (Rnd() - .5) * (16 + 46 * NearBar);
			if (Y == Drop && Rnd() < .5) V += 60;
			V = FMath::Clamp(V, 0., 255.);
			FColor& D = Px[Y * W + X];
			D.R = FS47Canvas::Clamp8(V * .42); D.G = FS47Canvas::Clamp8(V * 1.0 + 6); D.B = FS47Canvas::Clamp8(V * .46); D.A = 255;
		}
	}
	// 3. the unit's burned-in text, after the lens (it is on the tape, not in the scene): the label above, the date and time below
	const int32 Scale = FMath::Max(1, H / 200), Pad = FMath::Max(4, W / 40);
	TripCamText(Px, W, H, Pad, Pad, Scale, Label);
	TripCamText(Px, W, H, Pad, H - Pad - 7 * Scale, Scale, Stamp);
}

void US47FieldCamera::MakePrint(TArray<FColor>& Raw, const FString& Caption, const FString& Time, double Seed, TArray<FColor>& OutPrint)
{
	// The case print: the film look, then the paper around it.
	FilmLook(Raw, PHOTO_W, PHOTO_H, Seed);
	FS47Canvas Q(PRINT_W, PRINT_H);
	Q.SetFill(0xebe5d3); Q.FillRect(0, 0, PRINT_W, PRINT_H);
	Q.DrawImage(Raw, PHOTO_W, PHOTO_H, PRINT_X, PRINT_Y);
	Q.SetFill(0x282828, 0.25); Q.FillRect(PRINT_X, PRINT_Y + PHOTO_H, PHOTO_W, 2); // rgba(40,40,40,.25)
	Q.SetFill(0x26324f); Q.SetFont(TEXT("ReenieBeanie-Regular.ttf"), 46);
	Q.FillText(Caption, PRINT_X + 6, PRINT_Y + PHOTO_H + 60);
	Q.SetFill(0x2d2b27); Q.SetFont(TEXT("SpecialElite-Regular.ttf"), 30); Q.TextAlign = FS47Canvas::EAlign::Right;
	Q.FillText(Time, PRINT_W - PRINT_X - 6, PRINT_Y + PHOTO_H + 58);
	OutPrint = MoveTemp(Q.Px);
}

bool US47FieldCamera::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s field camera: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	Check(PRINT_W == 1016 && PRINT_H == 720 && PRINT_X == 28 && PRINT_Y == 28, TEXT("the print is 1016 x 720 with the photograph at 28, 28"));
	// filmLook on a 5 x 3 picture with the seed shutter() makes at 02:31:07.25 (values from node)
	{
		const int32 W = 5, H = 3;
		TArray<FColor> D;
		for (int32 k = 0; k < W * H; ++k) D.Add(FColor(uint8((k * 37) % 256), uint8((k * 91 + 13) % 256), uint8((k * 53 + 200) % 256), 255));
		const double Seed = FMath::FloorToDouble((2 * 3600 + 31 * 60 + 7.25) * 1000);
		const double Mean = FilmLook(D, W, H, Seed);
		const int32 Want[45] = { 3,6,66,47,74,130,86,152,66,82,30,69,92,76,85,104,108,100,171,59,35,33,130,69,82,217,123,68,57,109,66,77,92,144,193,55,183,95,82,217,167,121,7,6,98 };
		bool bSame = Seed == 9067250.0;
		for (int32 k = 0; k < W * H; ++k) bSame &= D[k].R == Want[k * 3] && D[k].G == Want[k * 3 + 1] && D[k].B == Want[k * 3 + 2];
		Check(bSame && FMath::Abs(Mean - 94.148667) < 1e-5, FString::Printf(TEXT("film look as filmLook (mean %.6f)"), Mean));
	}
	Check(ToUint32(-1) == 4294967295u && ToUint32(4294967296.0 + 5) == 5u && ToUint32(12.9) == 12u, TEXT("seed >>> 0"));
	// D48-20261110 (D46-27): the data back's imprint
	{
		Check(DataBackText(FDateTime(1986, 4, 29)) == TEXT("4 29 '86") && DataBackText(FDateTime(1986, 5, 3)) == TEXT("5 3 '86") && DataBackText(FDateTime(1986, 4, 13, 23, 58)) == TEXT("4 13 '86"), TEXT("the data back reads month, day and '86"));
		const int32 W = 640, H = 400;
		TArray<FColor> Dark; Dark.Init(FColor(8, 9, 14, 255), W * H);
		TArray<FColor> Same = Dark;
		DataBack(Dark, W, H, TEXT("4 29 '86"));
		int32 Lit = 0, Outside = 0, Hot = 0, MinX = W, MaxX = 0, MinY = H, MaxY = 0;
		bool bOrange = true;
		for (int32 Y = 0; Y < H; ++Y) for (int32 X = 0; X < W; ++X)
		{
			const FColor& C = Dark[Y * W + X];
			if (C == Same[Y * W + X]) continue;
			++Lit; MinX = FMath::Min(MinX, X); MaxX = FMath::Max(MaxX, X); MinY = FMath::Min(MinY, Y); MaxY = FMath::Max(MaxY, Y);
			if (C.R > 60) bOrange &= C.R > C.G && C.G > C.B;
			if (C.R > 200) ++Hot;
			if (X < W * .6 || Y < H * .8) ++Outside;
		}
		Lines.Add(FString::Printf(TEXT("field camera: data back lit %d pixels (%d hot) in x %d-%d, y %d-%d of %dx%d"), Lit, Hot, MinX, MaxX, MinY, MaxY, W, H));
		Check(Lit > 150 && Hot > 40 && Outside == 0 && bOrange && MaxX > W * .9 && MaxY > H * .88 && MaxY < H && MaxY - MinY >= 14 && MaxY - MinY <= 32 && MaxX - MinX > 60 && MaxX - MinX < 180,
			TEXT("the imprint is a small orange line of digits in the lower right corner"));
		TArray<FColor> Twice = Same; DataBack(Twice, W, H, TEXT("4 29 '86"));
		Check(Twice == Dark, TEXT("the imprint is the same every time (no randomness)"));
		TArray<FColor> Blank = Same; DataBack(Blank, W, H, TEXT("abc")); DataBack(Blank, W, H, FString()); DataBack(Blank, 0, 0, TEXT("1"));
		Check(Blank == Same, TEXT("letters, nothing and no frame burn nothing"));
		TArray<FColor> Small; Small.Init(FColor(0, 0, 0, 255), 320 * 200);
		DataBack(Small, 320, 200, TEXT("12 31 '86"));
		int32 LitSmall = 0; for (const FColor& C : Small) if (C.R > 0) ++LitSmall;
		Check(LitSmall > 60 && LitSmall < 2500, TEXT("on the small print the digits still come out"));
	}
	// N1110 (D46-13): the trip camera's tape look
	{
		const int32 W = 640, H = 400;
		TArray<FColor> Orig; Orig.SetNumUninitialized(W * H);
		for (int32 Y = 0; Y < H; ++Y) for (int32 X = 0; X < W; ++X) Orig[Y * W + X] = FColor(uint8(X * 255 / W), uint8(Y * 255 / H), uint8((X + Y) % 256), 255);
		TArray<FColor> A = Orig, B = Orig, C = Orig;
		const FString Stamp = TEXT("04/28/86  11:58:07 PM"), Label = TEXT("TRIP CAM 3");
		TripCamLook(A, W, H, 42., Stamp, Label); TripCamLook(B, W, H, 42., Stamp, Label); TripCamLook(C, W, H, 43., Stamp, Label);
		Check(A == B && A != C && A != Orig, TEXT("the tape look is the same for a seed and differs by seed"));
		// a green monochrome: green leads red and blue everywhere outside the text, except a few dropout pixels
		int32 Green = 0, Total = 0, Text = 0;
		for (int32 Y = 40; Y < H - 40; ++Y) for (int32 X = 40; X < W - 40; ++X) { const FColor& P = A[Y * W + X]; ++Total; if (P.G >= P.R && P.G >= P.B) ++Green; }
		for (const FColor& P : A) if (P == FColor(214, 255, 214, 255)) ++Text;
		Check(Green > Total * 98 / 100, TEXT("the tape is monochrome green"));
		Check(Text > 150 && Text < 3000, FString::Printf(TEXT("the burned-in label and date line are drawn in dots (%d lit)"), Text));
		int32 Top = 0, Low = 0, Mid = 0;
		for (int32 Y = 0; Y < H; ++Y) for (int32 X = 0; X < W; ++X) if (A[Y * W + X] == FColor(214, 255, 214, 255)) { if (Y < 40) ++Top; else if (Y > H - 40) ++Low; else ++Mid; }
		Check(Top > 60 && Low > 100 && Mid == 0, TEXT("the label sits top left and the date line bottom left, nothing between"));
		TArray<FColor> Flat; Flat.Init(FColor(100, 100, 100, 255), W * H);
		TArray<FColor> Wide = Flat; for (int32 X = 0; X < W; ++X) for (int32 Y = 0; Y < H; ++Y) if (X / 8 % 2) Wide[Y * W + X] = FColor(200, 200, 200, 255);
		TArray<FColor> Bent = Wide; TripCamLook(Bent, W, H, 1., FString(), FString());
		int32 Moved = 0; for (int32 X = 0; X < 160; ++X) if ((Bent[(H / 2) * W + W - 1 - X].G > 190) != (Wide[(H / 2) * W + W - 1 - X].G > 150)) ++Moved;
		Check(Moved > 10, TEXT("the wide lens bends the picture toward the edges"));
		TArray<FColor> Tiny, Short; Tiny.Init(FColor(1, 2, 3, 255), 4); Short = Tiny;
		TripCamLook(Tiny, 2, 2, 1., Stamp, Label); TripCamLook(Short, 640, 400, 1., Stamp, Label); TripCamLook(Short, 0, 0, 1., Stamp, Label);
		TArray<FColor> Small; Small.Init(FColor(30, 30, 30, 255), 320 * 200); TripCamLook(Small, 320, 200, 5., Stamp, Label);
		int32 SmallText = 0; for (const FColor& P : Small) if (P == FColor(214, 255, 214, 255)) ++SmallText;
		Check(Tiny.Num() == 4 && SmallText > 100, TEXT("a frame too small for the tape is left alone, and the small print still reads"));
	}
	// project(): a camera at the S-03 apron, yaw 0.2, pitch 0.31 (values from three's project)
	{
		US47FieldCamera* F = NewObject<US47FieldCamera>();
		F->PhotoLoc = S47Three::ToUE(10.4, 1.62, -11.6);
		F->PhotoRot = S47Three::CameraRotation(0.2, 0.31, 0.0);
		const FS47Projection A = F->Project(FVector(9.5, 9.0, -42)), B = F->Project(FVector(9.9, 2.35, -18.35)), C = F->Project(FVector(12, 3, -9));
		Check(FMath::Abs(A.U - 0.627609) < 1e-4 && FMath::Abs(A.V - 0.583738) < 1e-4 && A.bFront, FString::Printf(TEXT("S-03 projects to %.6f %.6f"), A.U, A.V));
		Check(FMath::Abs(B.U - 0.597986) < 1e-4 && FMath::Abs(B.V - 0.749300) < 1e-4 && B.bFront, FString::Printf(TEXT("the vane projects to %.6f %.6f"), B.U, B.V));
		Check(!C.bFront, TEXT("a point behind the camera is not in front"));
		Check(F->InFrame(FVector(9.5, 9.0, -42), 0.22, 0.78, 0.23, 0.78) && !F->InFrame(FVector(9.5, 9.0, -42), 0.36, 0.62, 0.30, 0.65), TEXT("inFrame with Unity's y from the bottom"));
	}
	return bOk;
}

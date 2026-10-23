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
		E->SetScalarParameterValue(TEXT("ToneK"), ToneK * 1.9f); // long exposure: the film sees more than the eye
		E->SetScalarParameterValue(TEXT("Vhs"), 0.f);
	}
	// (CaptureScene sends the world's pending updates first: the echo shown, the exposure raised)
	C->CaptureScene();
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
	if (O.Before) O.Before();
	const bool bOk = Render(S.W, S.H, S.Px); S.bRendered=bOk;
	if (O.After) O.After();
	if (O.Restore) O.Restore();
	if (!bOk) S.Px.Init(FColor(0, 0, 0, 255), S.W * S.H);
	if (O.Post) O.Post(S.Px, S.W, S.H);
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

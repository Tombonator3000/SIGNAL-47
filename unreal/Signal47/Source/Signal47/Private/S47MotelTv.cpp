// story/motelTv.ts
#include "S47MotelTv.h"

#include "CollisionQueryParams.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Math/RotationMatrix.h"
#include "RHI.h"
#include "Misc/Base64.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "S47Audio.h"
#include "S47Geo.h"
#include "S47Image.h"
#include "S47Inter.h"
#include "S47Json.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Web.h"

namespace
{
	const double GAP = 10 * 60, CARRIER = 47, FRAME = 2.6, DWELL = 6;
	const int32 W = 320, H = 240, TUBE_W = 256, TUBE_H = 192;
	const FVector OVER(-36, 30, 15);
	const double FOV = 40;
	// the broadcast's sound from the set's speaker (the plate beside the screen), and its light on the
	// office following the picture (the film is darker than the card: its mean is about 0.25)
	const double PROGRAMME = 0.32;
	const FVector GRILLE(0.155, 0.17, 0.183);
	double FilmLight(double Brightness) { return FMath::Clamp(FMath::Sqrt(FMath::Max(0.0, Brightness) / 0.25), 0.35, 1.35); }
	// motelTv.ts: new PointLight(0xe4ebf2, 700, 70, 1.6), held 22 m above the floor.
	const double TOP_INTENSITY = 700, TOP_DISTANCE = 70, TOP_DECAY = 1.6, TOP_HEIGHT = 22;
	double TopCandela()
	{
		// UE's non-inverse exponent is pow(1-(d/R)^2,e), not three's 1/d^e. Use physical
		// candelas and match the authored illuminance on the floor directly under the light:
		// Iue / height^2 = 700 / height^1.6. Both physical lights share (1-(d/70)^4)^2.
		// UE has no free inverse-power exponent: away from that distance its inverse-square law remains.
		return TOP_INTENSITY * FMath::Pow(TOP_HEIGHT, 2 - TOP_DECAY);
	}
	FColor Colour(uint32 Hex) { return FColor(uint8((Hex >> 16) & 255), uint8((Hex >> 8) & 255), uint8(Hex & 255), 255); }
	// Canvas2D draws in sRGB bytes; these raster operations keep those bytes, like S47Drawings.
	void Blend(FColor& C, const FColor& Ink, double A)
	{
		C.R = uint8(FMath::Clamp(FMath::RoundToInt(C.R * (1 - A) + Ink.R * A), 0, 255));
		C.G = uint8(FMath::Clamp(FMath::RoundToInt(C.G * (1 - A) + Ink.G * A), 0, 255));
		C.B = uint8(FMath::Clamp(FMath::RoundToInt(C.B * (1 - A) + Ink.B * A), 0, 255)); C.A = 255;
	}
	FColor Sample(const TArray<FColor>& P, int32 Sw, int32 Sh, double X, double Y)
	{
		X = FMath::Clamp(X, 0.0, double(Sw - 1)); Y = FMath::Clamp(Y, 0.0, double(Sh - 1));
		const int32 X0 = int32(X), Y0 = int32(Y), X1 = FMath::Min(Sw - 1, X0 + 1), Y1 = FMath::Min(Sh - 1, Y0 + 1);
		const double U = X - X0, V = Y - Y0;
		const FColor A = P[Y0 * Sw + X0], B = P[Y0 * Sw + X1], C = P[Y1 * Sw + X0], D = P[Y1 * Sw + X1];
		auto Ch = [&](uint8 Aa, uint8 Bb, uint8 Cc, uint8 Dd) { return uint8(FMath::Clamp(FMath::RoundToInt(FMath::Lerp(FMath::Lerp(double(Aa), double(Bb), U), FMath::Lerp(double(Cc), double(Dd), U), V)), 0, 255)); };
		return FColor(Ch(A.R,B.R,C.R,D.R), Ch(A.G,B.G,C.G,D.G), Ch(A.B,B.B,C.B,D.B), 255);
	}
}

FS47MotelTv::FS47MotelTv(US47Main* InM, FS47MotelTvDeps InD) : M(InM), D(MoveTemp(InD))
{
	Card = TestCard();
	// Round 15's blank card, if staged; otherwise the TypeScript's own colour bars remain.
	TArray<uint8> Bytes;
	if (FFileHelper::LoadFileToArray(Bytes, *(FPaths::ProjectContentDir() / TEXT("WebData/art/tv_testcard_blank.jpg"))))
	{
		TArray<FColor> Art; int32 Aw = 0, Ah = 0;
		if (S47Image::DecodePixels(TEXT("data:image/jpeg;base64,") + FBase64::Encode(Bytes), Art, Aw, Ah))
			for (int32 Y = 0; Y < H; ++Y) for (int32 X = 0; X < W; ++X) Card[Y * W + X] = Sample(Art, Aw, Ah, (X + 0.5) * Aw / W - 0.5, (Y + 0.5) * Ah / H - 0.5);
	}
	Tv = M && M->Web ? M->Web->Obj(TEXT("world.motel.tv.group")) : FS47Obj();
	BindScreen();
	const FS47Obj Px = M && M->Web ? M->Web->Obj(TEXT("world.motel.proxies.officeTv")) : FS47Obj();
	if (Px && M->Inter)
	{
		FS47Interactable It;
		It.Id = TEXT("court:officeTv"); It.Object = Px; It.Range = 3.6;
		It.Label = [this, Px]
		{
			if (!Px.Parent().Visible()) return FString();
			return Mode == TEXT("card") && D.Document && D.Document() ? FString(TEXT("Television / recorded frame")) : FString(TEXT("Television"));
		};
		It.Use = [this]
		{
			const TOptional<FS47Doc> Doc = D.Document ? D.Document() : TOptional<FS47Doc>();
			if (Mode == TEXT("card") && Doc) { if (D.Open) D.Open(*Doc); return; }
			if (Mode == TEXT("card") && Seen(TEXT("ev:tv"))) { Toast(Programme.Available() ? TEXT("The broadcast is back. The notebook has the observation, but the picture from that earlier case is unavailable.") : TEXT("The test card again. The notebook has the observation, but the picture from that earlier case is unavailable."), 5); return; }
			Toast(Mode == TEXT("card") && Programme.Available() ? TEXT("A strange late-night broadcast. Robes, spacecraft and a promise of visitors from the stars.") : Mode == TEXT("card") ? TEXT("A portable set on the shelf, left on. The station signed off hours ago: the test card, and a thin tone.")
				: Mode == TEXT("carrier") ? TEXT("Nothing on the screen but grey, and the hum.") : TEXT("The lot, from straight above. The truck exactly where you left it."), 4);
		};
		It.Examine = [this]
		{
			return Mode == TEXT("card") && Programme.Available() && !Seen(TEXT("ev:tv")) ? FString(TEXT("A faded broadcast of a cosmic pageant, flickering behind the glass.")) : Mode == TEXT("card") ? D.Document && D.Document() ? FString(Programme.Available() ? TEXT("The broadcast is back. The single picture of the lot is kept in the notebook.") : TEXT("The test card again. The single picture of the lot is kept in the notebook."))
				: Seen(TEXT("ev:tv")) ? FString(Programme.Available() ? TEXT("The broadcast is back. The notebook has the observation, but the earlier picture is unavailable.") : TEXT("The test card again. The notebook has the observation, but the earlier picture is unavailable.")) : FString(TEXT("Colour bars, and a thin tone. Nobody has turned it off."))
				: Mode == TEXT("carrier") ? FString(TEXT("A grey field. The hum of a carrier with nothing on it.")) : FString(TEXT("The motel lot, straight down. Your truck, at the same angle you parked it. No camera could stand there."));
		};
		M->Inter->Add(It);
	}
	Paint(0);
}

FS47MotelTv::~FS47MotelTv()
{
	// UObject teardown may already have destroyed M/Audio; the graph owns its stop closure.
	if (Snd && Snd->Stop) Snd->Stop();
	Snd.Reset();
	if (Capture.IsValid()) Capture->Destroy();
	if (Top.IsValid()) Top->DestroyComponent();
}

bool FS47MotelTv::Seen(const FString& Key) const { return M && M->TownSeen.Contains(Key); }
void FS47MotelTv::Toast(const FString& Text, double Secs) const { if (D.Toast) D.Toast(Text, Secs); }

double FS47MotelTv::Last() const
{
	double Out = -1e9;
	if (M) for (const FString& K : M->TownSeen.Array()) if (K.StartsWith(TEXT("mark:")))
	{
		double N = 0; if (LexTryParseString(N, *K.Mid(K.Find(TEXT("@")) + 1))) Out = FMath::Max(Out, N);
	}
	return Out;
}

void FS47MotelTv::Update(double Dt, const FS47MotelTvIn& S)
{
	const double Day = FMath::Fmod(FMath::Fmod(S.Clock, 86400.0) + 86400.0, 86400.0);
	TvAt = Tv.WorldPosition();
	const double InDist = S.bSaro ? FVector::Distance(S.Cam.Pos, TvAt) : TNumericLimits<double>::Max();
	Dist = InDist;
	Dwell = S.bOk && S.bHere ? Dwell + Dt : 0;
	const bool bCould = Mode == TEXT("card") && S.bOk && S.bSaro && S.bHere && !Seen(TEXT("ev:tv")) && S.bTruckOnLot && S.Clock >= NotBefore && S.Clock - Last() >= GAP
		&& Day >= 2 * 3600 + 30 * 60 && Day < 5 * 3600 + 5 * 60;
	bWaiting = bCould;
	Unrest = bCould ? FMath::Clamp(S.Unrest, 0.0, 1.0) : 0;
	if (Mode == TEXT("card"))
	{
		if (bCould && S.May != TOptional<bool>(false) && Dwell > DWELL)
		{
			Carrier();
			Toast(TEXT("On the television the picture is gone: a blank grey screen, and the hum of a carrier with nothing on it."), 5.5);
		}
	}
	else if (Mode == TEXT("carrier"))
	{
		if (!S.bSaro) Missed(S.Clock);
		else if (S.bOk) { Warm(); if ((T += Dt) >= CARRIER) ShowFrame(S, InDist); }
	}
	else if (Mode == TEXT("frame"))
	{
		if (!S.bSaro) Missed(S.Clock);
		else if (S.bOk)
		{
			Witness(S, InDist);
			if ((T += Dt) >= FRAME) { if (!Seen(TEXT("ev:tv"))) NotBefore = S.Clock + GAP; Mode = TEXT("card"); T = 0; Click(); }
		}
	}
	// (the broadcast is painted as its pictures change, at the film's own rate; the rest at 15 a second)
	const bool bFilm = Mode == TEXT("card") && Programme.Available();
	PaintT += Dt;
	if (InDist < 40 && (bFilm ? Programme.FrameIndex() != PaintedFrame : PaintT >= 1.0 / 15)) { PaintDt = bFilm ? 1.0 / Programme.Fps() : 1.0 / 15; PaintT = 0; Paint(FPlatformTime::Seconds()); }
	Sound(InDist);
	Broadcast(Dt, S, InDist);
	const double K = Mode == TEXT("card") ? (bFilm ? 1.3 * FilmLight(Programme.Brightness()) : 1.3) : Mode == TEXT("carrier") ? 0.75 : 1.6;
	Light = FLinearColor(Mode == TEXT("carrier") ? 0.62 : 0.66, Mode == TEXT("carrier") ? 0.66 : 0.72, Mode == TEXT("carrier") ? 0.7 : 0.92);
	if (M && M->Web)
	{
		const int32 Court = M->Web->FloodSet(TEXT("court"));
		// MotelFront's last slot, after the three truck head slots (setTvLight).
		M->Web->SetFlood(Court, 15, S.bSaro ? K * (0.94 + 0.06 * FMath::FRand()) : 0);
		M->Web->SetFloodColor(Court, 15, Light);
	}
}

void FS47MotelTv::Carrier() { Mode = TEXT("carrier"); T = 0; bWarmed = false; bCanWitness = false; Roll.Reset(); Click(); }
void FS47MotelTv::Missed(double Clock) { Mode = TEXT("card"); T = 0; Dwell = 0; NotBefore = Clock + GAP; bWarmed = false; bCanWitness = false; }

void FS47MotelTv::ShowFrame(const FS47MotelTvIn& S, double InDist)
{
	Mode = TEXT("frame"); T = 0;
	Click();
	const FS47Obj Truck = D.Truck ? D.Truck() : FS47Obj();
	bCanWitness = S.bSaro && S.bTruckOnLot && Truck && Truck.Shown();
	FrameUrl.Reset(); Shot.Reset();
	if (bCanWitness) Shoot();
	FrameClock = S.Clock;
	if (FrameUrl.IsEmpty()) { Missed(S.Clock); return; }
	Witness(S, InDist);
}

void FS47MotelTv::Witness(const FS47MotelTvIn& S, double InDist)
{
	if (Seen(TEXT("ev:tv")) || FrameUrl.IsEmpty() || !bCanWitness || !S.bOk || !S.bSaro || !(S.bHere || InDist < 9) || !InView(S.Cam)) return;
	const FString Time = S47Text::ClockText(FrameClock, false);
	if (D.File) D.File(FrameDocument(FrameUrl, Time));
	if (M) { M->TownSeen.Add(TEXT("ev:tv")); M->TownSeen.Add(FString::Printf(TEXT("mark:tv@%.0f"), FMath::FloorToDouble(FrameClock + 0.5))); }
	Toast(TEXT("For a moment the screen shows the motel's lot from straight above: the office roof, the sign, the truck where you left it. Nothing up there could hold a camera. Then the test card again."), 7);
	if (D.Note) D.Note(Time + TEXT(". Motel office TV. One frame of the lot from above. My truck in it."));
	if (D.Save) D.Save();
}

bool FS47MotelTv::InView(const FS47Cam& Cam) const
{
	if (!Tv) return false;
	const FVector Screen = Tv.LocalToWorld(FVector(-0.045, 0.18, 0.1815));
	const FVector Towards = Screen - Cam.Pos;
	if (FVector::DotProduct(Towards, Tv.WorldDirection()) >= 0) return false;
	const FVector Ndc = Cam.Project(Screen);
	if (!(Ndc.Z >= -1 && Ndc.Z <= 1 && FMath::Abs(Ndc.X) < 0.9 && FMath::Abs(Ndc.Y) < 0.9)) return false;
	UWorld* World = M && M->Web ? M->Web->GetWorld() : nullptr;
	if (!World) return false;
	const FVector Dir = Towards.GetSafeNormal();
	FVector A = S47Three::ToUE(Cam.Pos + Dir * 0.02);
	const FVector B = S47Three::ToUE(Cam.Pos + Dir * FMath::Max(0.02, Towards.Size() - 0.08));
	FCollisionQueryParams Q(SCENE_QUERY_STAT(S47MotelTvSight), true);
	for (int32 Tries = 0; Tries < 64; ++Tries)
	{
		FHitResult Hit;
		if (!World->LineTraceSingleByChannel(Hit, A, B, ECC_Visibility, Q)) return true;
		UPrimitiveComponent* C = Hit.GetComponent();
		bool bSolid = C && C->IsVisible();
		if (bSolid && M->Web->ItemOf(C))
		{
			const FS47WebNode* Node = M->Web->Node(M->Web->NodeOf(C));
			// Invisible interaction proxies have material opacity zero (the web's Raycaster ignores it).
			if (Node && Node->Name.StartsWith(TEXT("proxy"))) bSolid = false;
		}
		if (bSolid) if (const UMaterialInterface* Mat = C->GetMaterial(FMath::Max<int32>(0, Hit.ElementIndex)))
		{
			float Opacity = 1; Mat->GetScalarParameterValue(FName(TEXT("Opacity")), Opacity);
			if ((Mat->GetBlendMode() == BLEND_Translucent || Mat->GetBlendMode() == BLEND_Additive) && Opacity <= 0.5f) bSolid = false;
		}
		if (bSolid) return false;
		if (C) Q.AddIgnoredComponent(C);
		A = Hit.ImpactPoint;
	}
	return false;
}

void FS47MotelTv::Aim()
{
	FVector Over = OVER;
	const FS47Obj Truck = D.Truck ? D.Truck() : FS47Obj();
	const auto& A = S47Geo::MOTEL_AT();
	if (Truck)
	{
		const FBox Bounds = Truck.WorldBox();
		const double Half = OVER.Y * FMath::Tan(FOV * UE_DOUBLE_PI / 360);
		if (Bounds.Min.X < A.X + OVER.X - Half + 2 || Bounds.Max.X > A.X + OVER.X + Half - 2 || Bounds.Min.Z < A.Z + OVER.Z - Half * 4 / 3 + 2 || Bounds.Max.Z > A.Z + OVER.Z + Half * 4 / 3 - 2)
		{
			const double X0 = FMath::Min(-46.0, Bounds.Min.X - A.X - 2), X1 = FMath::Max(-27.0, Bounds.Max.X - A.X + 2);
			const double Z0 = FMath::Min(1.5, Bounds.Min.Z - A.Z - 2), Z1 = FMath::Max(30.0, Bounds.Max.Z - A.Z + 2);
			Over.X = (X0 + X1) / 2; Over.Z = (Z0 + Z1) / 2;
			Over.Y = FMath::Max3(OVER.Y, (X1 - X0) / (2 * FMath::Tan(FOV * UE_DOUBLE_PI / 360)), (Z1 - Z0) / (2 * FMath::Tan(FOV * UE_DOUBLE_PI / 360) * 4 / 3)) + 3;
		}
	}
	const FS47Obj Motel = M && M->Web ? M->Web->Obj(TEXT("world.motel.group")) : FS47Obj();
	CaptureAt = Motel ? Motel.LocalToWorld(Over) : FVector(A.X, A.Y, A.Z) + Over;
	UWorld* World = M && M->Web ? M->Web->GetWorld() : nullptr;
	if (!World) return;
	if (!Top.IsValid())
	{
		UPointLightComponent* L = NewObject<UPointLightComponent>(M->Web);
		L->SetMobility(EComponentMobility::Movable); L->SetUseInverseSquaredFalloff(true); L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(TopCandela()); L->SetLightColor(S47Three::Hex(0xe4ebf2)); L->SetAttenuationRadius(TOP_DISTANCE * 100);
		L->SetCastShadows(false);
		L->RegisterComponent(); Top = L;
	}
	Top->SetWorldLocation(S47Three::ToUE(FVector(CaptureAt.X, A.Y + TOP_HEIGHT, CaptureAt.Z)));
	Top->SetVisibility(true);
}

void FS47MotelTv::Warm()
{
	// Unreal loads the imported materials' compiled shaders. It has no three.js compile(part,cam,scene).
	// Keep the carrier's warm stage and its geometry/light aim without drawing a substitute picture.
	if (bWarmed) return;
	Aim(); if (Top.IsValid()) Top->SetVisibility(false); bWarmed = true;
}

bool FS47MotelTv::Shoot()
{
	UWorld* World = M && M->Web ? M->Web->GetWorld() : nullptr;
	if (!World) return false;
	Aim();
	if (!Capture.IsValid())
	{
		FActorSpawnParameters Sp; Sp.ObjectFlags |= RF_Transient;
		Capture = World->SpawnActor<ASceneCapture2D>(Sp);
	}
	if (!Capture.IsValid()) { if (Top.IsValid()) Top->SetVisibility(false); return false; }
	if (!Target)
	{
		Target.Reset(NewObject<UTextureRenderTarget2D>(M));
		Target->ClearColor = FLinearColor::Black; Target->InitCustomFormat(W, H, PF_B8G8R8A8, true); Target->UpdateResourceImmediate(true);
	}
	USceneCaptureComponent2D* C = Capture->GetCaptureComponent2D();
	C->TextureTarget = Target.Get(); C->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	C->bCaptureEveryFrame = false; C->bCaptureOnMovement = false; C->bAlwaysPersistRenderingState = true;
	// Drawing only the centre 4:3 gives the same ground as aim()'s full canvas and crop.
	C->FOVAngle = S47Three::HorizontalFov(FOV, 4.f / 3.f);
	C->bOverride_CustomNearClippingPlane = true; C->CustomNearClippingPlane = 100;
	C->MaxViewDistanceOverride = 120 * 100;
	C->PostProcessBlendWeight = 1; C->PostProcessSettings.bOverride_ReflectionMethod = true; C->PostProcessSettings.ReflectionMethod = EReflectionMethod::ScreenSpace;
	// Match AS47Web::BeginPlay's exposure on the capture's first frame as on the player's view.
	C->PostProcessSettings.bOverride_AutoExposureMethod = true; C->PostProcessSettings.AutoExposureMethod = AEM_Manual;
	C->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true; C->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
	C->PostProcessSettings.bOverride_AutoExposureBias = true; C->PostProcessSettings.AutoExposureBias = M->Web->ExposureOffset;
	C->ShowFlags.SetTemporalAA(false);
	const FVector Forward = S47Three::DirToUE(FVector(0, -1, 0)), Up = S47Three::DirToUE(FVector(-1, 0, 0));
	Capture->SetActorLocationAndRotation(S47Three::ToUE(CaptureAt), FRotationMatrix::MakeFromXZ(Forward, Up).Rotator());
	UMaterialParameterCollection* Env = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Web/Base/MPC_Web.MPC_Web"));
	UMaterialParameterCollectionInstance* E = Env ? World->GetParameterCollectionInstance(Env) : nullptr;
	float Vhs = 0;
	// shoot() renders the scene directly; VHS is only the web game's screen composer.
	if (E) { E->GetScalarParameterValue(TEXT("Vhs"), Vhs); E->SetScalarParameterValue(TEXT("Vhs"), 0); }
	C->CaptureScene();
	if (E) E->SetScalarParameterValue(TEXT("Vhs"), Vhs);
	if (Top.IsValid()) Top->SetVisibility(false);
	if (!S47Image::ReadPixels(Target.Get(), Shot) || Shot.Num() != W * H) { Shot.Reset(); return false; }
	// Canvas filter saturate(0.6) contrast(1.3) brightness(1.15), in this order.
	for (FColor& P : Shot)
	{
		const double L = 0.213 * P.R + 0.715 * P.G + 0.072 * P.B;
		auto Ch = [L](uint8 V) { return uint8(FMath::Clamp(FMath::RoundToInt(((L + (V - L) * 0.6 - 127.5) * 1.3 + 127.5) * 1.15), 0, 255)); };
		P = FColor(Ch(P.R), Ch(P.G), Ch(P.B), 255);
	}
	FrameUrl = S47Image::ToDataUrl(Shot, W, H, true, 88);
	return !FrameUrl.IsEmpty();
}

// ---------- the picture tube ----------
void FS47MotelTv::Paint(double InT)
{
	const int32 Width = TUBE_W, Height = TUBE_H;
	TArray<FColor> Pixels; Pixels.Init(FColor::Black, Width * Height);
	if (Mode == TEXT("carrier"))
	{
		const double BarY = Height - FMath::Fmod(InT * 11, Height + 60.0);
		for (int32 Y = 0; Y < Height; ++Y) for (int32 X = 0; X < Width; ++X)
		{
			FColor& P = Pixels[Y * Width + X]; P = Colour(0x3a3d40);
			const double A = 0.28 * FMath::Max(0.0, 1 - FMath::Abs(Y + 0.5 - BarY) / 30); Blend(P, FColor::Black, A);
		}
	}
	else
	{
		TArray<FColor> Film;
		const bool bBroadcast = M && Mode == TEXT("card") && Programme.Frame(Film, W, H);
		if (bBroadcast) PaintedFrame = Programme.FrameIndex();
		const TArray<FColor>& Src = bBroadcast ? Film : Mode == TEXT("frame") && Shot.Num() == W * H ? Shot : Card;
		// drawImage(src, 6, 5, W - 12, H - 10, ...): the card's overscan; the broadcast's is as even, so it stays 4:3
		const double Cx = bBroadcast ? (W - (H - 10) * 4.0 / 3) / 2 : 6;
		const int32 J = FMath::FRand() < 0.08 ? 1 : 0;
		const double U = Mode == TEXT("card") ? Unrest : 0;
		if (U > 0.08 && !Roll && FMath::FRand() < 0.012 + 0.09 * U * U) { Roll = FRoll{ 0, Height * (0.5 + 1.6 * U) }; Slips++; }
		auto Draw = [&](int32 AtY)
		{
			for (int32 Y = FMath::Max(0, AtY); Y < FMath::Min(Height, AtY + Height); ++Y)
				for (int32 X = 0; X < Width; ++X)
					Pixels[Y * Width + X] = Sample(Src, W, H, Cx + (X + 0.5) * (W - 2 * Cx) / Width - 0.5, 5 + (Y - AtY + 0.5) * (H - 10) / Height - 0.5);
		};
		if (Roll)
		{
			const int32 Y = FMath::RoundToInt(Roll->Y), Bar = FMath::Max(3, FMath::RoundToInt(Height * 0.07));
			Draw(Y); Draw(Y - Height - Bar);
			for (int32 Py = FMath::Max(0, Y - Bar); Py < FMath::Min(Height, Y); ++Py) for (int32 X = 0; X < Width; ++X) Pixels[Py * Width + X] = Colour(0x050505);
			if ((Roll->Y += Roll->V * PaintDt) >= Height + Bar) Roll.Reset();
		}
		else Draw(J);
		if (U > 0.2 && FMath::FRand() < 0.25 * U)
		{
			const int32 Y0 = FMath::FloorToInt(FMath::FRand() * Height), N = 2 + FMath::FloorToInt(FMath::FRand() * Height * 0.12 * U);
			const int32 Dx = FMath::RoundToInt((FMath::FRand() - 0.3) * Width * 0.08 * U);
			const TArray<FColor> Before = Pixels;
			for (int32 Y = Y0; Y < FMath::Min(Height, Y0 + N); ++Y) for (int32 X = 0; X < Width; ++X) if (X - Dx >= 0 && X - Dx < Width) Pixels[Y * Width + X] = Before[Y * Width + X - Dx];
		}
	}
	for (int32 Y = 0; Y < Height; Y += 2) for (int32 X = 0; X < Width; ++X) Blend(Pixels[Y * Width + X], FColor::Black, 0.24);
	for (int32 I = 0; I < (Mode == TEXT("carrier") ? 260 : 90); ++I)
	{
		const double A = FMath::FRand() * 0.12, X = FMath::FRand() * Width, Y = FMath::FRand() * Height;
		// Canvas fillRect at a fractional pixel covers up to four pixels.
		for (int32 Py = int32(Y); Py <= FMath::Min(Height - 1, int32(Y) + 1); ++Py) for (int32 Px = int32(X); Px <= FMath::Min(Width - 1, int32(X) + 1); ++Px)
		{
			const double CovX = FMath::Max(0.0, FMath::Min(Px + 1.0, X + 1) - FMath::Max(double(Px), X));
			const double CovY = FMath::Max(0.0, FMath::Min(Py + 1.0, Y + 1) - FMath::Max(double(Py), Y));
			Blend(Pixels[Py * Width + Px], FColor::White, A * CovX * CovY);
		}
	}
	const double Breath = 0.04 + 0.04 * FMath::FRand();
	for (int32 Y = 0; Y < Height; ++Y) for (int32 X = 0; X < Width; ++X)
	{
		const double Radius = FMath::Sqrt(FMath::Square(X + 0.5 - Width / 2.0) + FMath::Square(Y + 0.5 - Height / 2.0));
		const double A = FMath::Clamp((Radius - Height * 0.32) / (Width * 0.66 - Height * 0.32), 0.0, 1.0) * 0.6;
		Blend(Pixels[Y * Width + X], FColor::Black, A); Blend(Pixels[Y * Width + X], FColor::Black, Breath);
	}
	if (!M || !M->Web) return;
	if (!Tube) Tube.Reset(S47Image::FromPixels(Pixels, Width, Height));
	else
	{
		// Update a persistent canvas texture instead of allocating one UObject every fifteenth second.
		FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, Width, Height);
		uint8* Data = static_cast<uint8*>(FMemory::Malloc(Pixels.Num() * sizeof(FColor)));
		FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
		Tube->UpdateTextureRegions(0, 1, Region, Width * sizeof(FColor), sizeof(FColor), Data,
			[](uint8* Src, const FUpdateTextureRegion2D* R) { FMemory::Free(Src); delete R; });
	}
	if (ScreenMat.IsValid() && Tube)
	{
		ScreenMat->SetTextureParameterValue(TEXT("EmissiveMap"), Tube.Get());
		ScreenMat->SetScalarParameterValue(TEXT("UseEmissiveMap"), 1);
		ScreenMat->SetVectorParameterValue(TEXT("EmU"), FLinearColor(1, 0, 0, 0));
		ScreenMat->SetVectorParameterValue(TEXT("EmV"), FLinearColor(0, -1, 1, 0));
	}
}

// ---------- the set's little speaker ----------
void FS47MotelTv::Sound(double InDist)
{
	US47Audio* A = M ? M->Audio.Get() : nullptr;
	const auto Ctx = A ? A->Ctx : nullptr;
	if (!Ctx) return;
	if (InDist > 30) { if (Snd) Silence(); return; }
	FS47AudioContext::FBatch B(*Ctx);
	if (!Snd)
	{
		const auto Spk = A->Spatial(TvAt, 0.9); const auto Noise = A->NoiseBuffer();
		if (!Spk || !Noise) return;
		auto Gain = [&] { const auto G = Ctx->CreateGain(); G->Gain.SetValue(0); G->Connect(Spk); return G; };
		const auto Tone = Gain(); const auto TOsc = Ctx->CreateOscillator(); TOsc->Frequency.SetValue(1000);
		const auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(1300); Bp->Q.SetValue(0.5);
		TOsc->Connect(Bp); Bp->Connect(Tone);
		const auto HumGain = Gain(); const auto Hb = A->Lic(TEXT("tv_hum"));
		TSharedPtr<FS47ScheduledSourceNode> HOsc;
		if (Hb) { const auto S = Ctx->CreateBufferSource(); S->Buffer = Hb; S->bLoop = true; HOsc = S; }
		else { const auto O = Ctx->CreateOscillator(); O->SetType(TEXT("sawtooth")); O->Frequency.SetValue(60); HOsc = O; }
		const auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(380); HOsc->Connect(Lp); Lp->Connect(HumGain);
		const auto Hiss = Gain(); const auto N = Ctx->CreateBufferSource(); N->Buffer = Noise; N->bLoop = true;
		const auto Hp = Ctx->CreateBiquadFilter(); Hp->SetType(TEXT("highpass")); Hp->Frequency.SetValue(900); N->Connect(Hp); Hp->Connect(Hiss);
		const auto Whine = Gain(); const auto WOsc = Ctx->CreateOscillator(); WOsc->Frequency.SetValue(15734); WOsc->Connect(Whine);
		TOsc->Start(); HOsc->Start(); N->Start(); WOsc->Start();
		Snd = FSnd{ Tone, HumGain, Hb ? 1.8 : 1, Hiss, Whine, [TOsc, HOsc, N, WOsc, Spk] { TOsc->Stop(); HOsc->Stop(); N->Stop(); WOsc->Stop(); Spk->Disconnect(); } };
	}
	const double Now = Ctx->CurrentTime();
	auto Set = [Now](const TSharedPtr<FS47GainNode>& G, double V) { G->Gain.SetTargetAtTime(V, Now, 0.04); };
	const double U = Mode == TEXT("card") ? Unrest : 0, Waver = U > 0 ? 1 - U * (0.35 + 0.65 * FMath::FRand() * FMath::FRand()) : 1;
	Set(Snd->Tone, Mode == TEXT("card") && !Programme.Available() ? 0.03 * (Roll ? 0.3 : Waver) : 0);
	Set(Snd->Hum, Mode == TEXT("carrier") ? 0.11 * Snd->HumK : 0.05 * U * U * Snd->HumK);
	Set(Snd->Hiss, Mode == TEXT("carrier") ? 0.014 : Mode == TEXT("card") ? 0.006 : 0);
	Set(Snd->Whine, Mode == TEXT("frame") ? 0 : 0.0016);
}

/** The broadcast's own sound, in step with its picture: only while the set shows it (the carrier and
 *  the frame take it over, and give it back where the picture is), from the set's speaker, duller and
 *  quieter from outside the office, and stepping back under a phone, a call or a voice. */
void FS47MotelTv::Broadcast(double Dt, const FS47MotelTvIn& S, double InDist)
{
	if (!Programme.Available()) return;
	const bool bInside = M && M->Web && (M->Web->Rect(TEXT("world.motel.zones[2]")).Contains(S.Cam.Pos, 0.3) || M->Web->Rect(TEXT("world.motel.zones[3]")).Contains(S.Cam.Pos, 0.3));
	const double U = Mode == TEXT("card") ? Unrest : 0, Waver = U > 0 ? 1 - U * (0.35 + 0.65 * FMath::FRand() * FMath::FRand()) : 1;
	const double Duck = M && M->Story ? M->Story->VoicesDuck() : 1.0;
	const double Level = Mode == TEXT("card") && InDist <= 30 ? PROGRAMME * Duck * (Roll ? 0.3 : Waver) : 0;
	Programme.Update(Dt, M ? M->Audio.Get() : nullptr, Tv ? Tv.LocalToWorld(GRILLE) : TvAt, Level, !bInside);
}

void FS47MotelTv::Click()
{
	if (!M || !M->Audio) return;
	FS47Sound O; O.At = TvAt; O.Gain = 0.22; O.Rate = 1.3; O.Lp = 2400; M->Audio->Sfx(TEXT("click"), O);
}

void FS47MotelTv::Silence()
{
	Programme.Silence();
	if (!Snd) return;
	const FSnd S = *Snd; Snd.Reset();
	const auto Ctx = M && M->Audio ? M->Audio->Ctx : nullptr;
	if (Ctx)
	{
		FS47AudioContext::FBatch B(*Ctx);
		for (const auto& G : { S.Tone, S.Hum, S.Hiss, S.Whine }) G->Gain.SetTargetAtTime(0, Ctx->CurrentTime(), 0.03);
		Ctx->SetTimeout(0.3, S.Stop);
	}
	else if (S.Stop) S.Stop();
}

void FS47MotelTv::Reset()
{
	Mode = TEXT("card"); T = 0; Dwell = 0; NotBefore = 0; FrameUrl.Reset(); Shot.Reset(); bWarmed = false; bCanWitness = false;
	bWaiting = false; Unrest = 0; Roll.Reset();
	Silence(); Paint(0);
}

void FS47MotelTv::Refile(const FString& Url)
{
	if (Url.IsEmpty() || !Seen(TEXT("ev:tv"))) return;
	double Clock = 0; bool bFinite = false;
	if (M) for (const FString& K : M->TownSeen.Array()) if (K.StartsWith(TEXT("mark:tv@"))) { bFinite = LexTryParseString(Clock, *K.Mid(8)) && FMath::IsFinite(Clock); break; }
	if (D.File) D.File(FrameDocument(Url, bFinite ? S47Text::ClockText(Clock, false) : TEXT("time unrecorded")));
}

void FS47MotelTv::Now() { if (Mode == TEXT("card")) Carrier(); }

void FS47MotelTv::BindScreen()
{
	if (!M || !M->Web) return;
	const int32 TexId = M->Web->HandleTex(TEXT("world.motel.tv.tex"));
	if (TexId < 0) return;
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("WebData/manifest.json")))) return;
	const auto Manifest = S47J::Read(Text);
	for (const auto& V : S47J::Arr(Manifest, TEXT("materials")))
	{
		const auto Mat = V->AsObject();
		if (S47J::Int(Mat, TEXT("emissiveMap"), -1) != TexId) continue;
		// mergeStatic moved the screen geometry out of tv.group; its exported material is still shared.
		ScreenMat = M->Web->SharedMid(S47J::Int(Mat, TEXT("id"), -1));
		break;
	}
}

FS47Doc FS47MotelTv::FrameDocument(const FString& Image, const FString& Time)
{
	FS47Doc Doc; Doc.Id = TEXT("motel-tv-frame"); Doc.Kind = TEXT("photo"); Doc.Title = TEXT("TV FRAME / Sierra Motor Court (") + Time + TEXT(")"); Doc.Image = Image;
	Doc.Transcript = TEXT("Motel office television, ") + Time + TEXT(". After 47 seconds of bare carrier, one fixed frame of the lot from directly above. The office roof, the sign, and my truck exactly where I parked it, at the same angle. No camera could stand there. Then the test card again.");
	return Doc;
}

TArray<FColor> FS47MotelTv::TestCard()
{
	TArray<FColor> Out; Out.Init(FColor::Black, W * H);
	const double Bw = W / 7.0;
	auto Rect = [&](uint32 Col, double X, double Y, double Width, double Height)
	{
		for (int32 Py = FMath::Max(0, FMath::FloorToInt(Y)); Py < FMath::Min(H, FMath::CeilToInt(Y + Height)); ++Py)
			for (int32 Px = FMath::Max(0, FMath::FloorToInt(X)); Px < FMath::Min(W, FMath::CeilToInt(X + Width)); ++Px)
			{
				const double CovX = FMath::Max(0.0, FMath::Min(Px + 1.0, X + Width) - FMath::Max(double(Px), X));
				const double CovY = FMath::Max(0.0, FMath::Min(Py + 1.0, Y + Height) - FMath::Max(double(Py), Y));
				Blend(Out[Py * W + Px], Colour(Col), CovX * CovY);
			}
	};
	const uint32 Tops[] = { 0xc0c0c0, 0xc0c000, 0x00c0c0, 0x00c000, 0xc000c0, 0xc00000, 0x0000c0 };
	const uint32 Mids[] = { 0x0000c0, 0x131313, 0xc000c0, 0x131313, 0x00c0c0, 0x131313, 0xc0c0c0 };
	for (int32 I = 0; I < 7; ++I) Rect(Tops[I], FMath::FloorToDouble(I * Bw), 0, FMath::CeilToDouble(Bw), H * 0.67);
	for (int32 I = 0; I < 7; ++I) Rect(Mids[I], FMath::FloorToDouble(I * Bw), H * 0.67, FMath::CeilToDouble(Bw), H * 0.08);
	const double B = H * 0.75, Q = Bw * 5 / 4;
	const uint32 Bottom[] = { 0x00214c, 0xffffff, 0x32006a, 0x131313 };
	for (int32 I = 0; I < 4; ++I) Rect(Bottom[I], FMath::FloorToDouble(I * Q), B, FMath::CeilToDouble(Q), H - B);
	const uint32 Blacks[] = { 0x090909, 0x131313, 0x1d1d1d };
	for (int32 I = 0; I < 3; ++I) Rect(Blacks[I], FMath::FloorToDouble(5 * Bw + I * Bw / 3), B, FMath::CeilToDouble(Bw / 3), H - B);
	Rect(0x131313, FMath::FloorToDouble(6 * Bw), B, FMath::CeilToDouble(Bw), H - B);
	return Out;
}

bool FS47MotelTv::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s motelTv: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	const double Cutoff = FMath::Square(1 - FMath::Pow(TOP_HEIGHT / TOP_DISTANCE, 4));
	const double ThreeAtFloor = TOP_INTENSITY / FMath::Pow(TOP_HEIGHT, TOP_DECAY) * Cutoff;
	const double UeAtFloor = TopCandela() / FMath::Square(TOP_HEIGHT) * Cutoff;
	Check(FMath::IsNearlyEqual(ThreeAtFloor, UeAtFloor, 1e-12) && FMath::IsNearlyEqual(ThreeAtFloor, 4.883212936445403, 1e-12), TEXT("the physical top light matches three's 700/70/1.6 at its authored 22 m floor distance"));
	FS47MotelTv TvTest(nullptr, {}); FS47MotelTvIn S; S.bOk = true; S.bSaro = true; S.bHere = true; S.bTruckOnLot = true; S.Clock = 3 * 3600; S.May = false;
	TvTest.Update(7, S); Check(TvTest.Mode == TEXT("card") && TvTest.bWaiting, TEXT("six-second dwell waits for the night's peak"));
	S.May = true; TvTest.Update(0.01, S); Check(TvTest.Mode == TEXT("carrier") && TvTest.Busy(), TEXT("the peak permits the 47-second carrier"));
	S.bOk = false; TvTest.Update(50, S); Check(TvTest.Mode == TEXT("carrier") && TvTest.T == 0, TEXT("papers and cinematics do not consume the frame's chance"));
	S.bOk = true; TvTest.Update(46.99, S); Check(TvTest.Mode == TEXT("carrier"), TEXT("the carrier still holds before 47 seconds"));
	TvTest.Update(0.01, S); Check(TvTest.Mode == TEXT("card") && TvTest.NotBefore == S.Clock + 600 && TvTest.FrameUrl.IsEmpty(), TEXT("an unavailable actual scene cannot spend an event or invent artwork"));
	TvTest.Reset(); S.Clock = 5 * 3600 + 5 * 60; TvTest.Update(7, S); Check(!TvTest.bWaiting && TvTest.Mode == TEXT("card"), TEXT("the event's hours stop at 05:05"));
	const FS47Doc Doc = FrameDocument(TEXT("data:image/jpeg;base64,test"), TEXT("03:00"));
	Check(Doc.Id == TEXT("motel-tv-frame") && Doc.Title == TEXT("TV FRAME / Sierra Motor Court (03:00)") && Doc.Transcript.Contains(TEXT("After 47 seconds of bare carrier")), TEXT("the recorded frame keeps the exact finding and transcript"));
	const TArray<FColor> Bars = TestCard(); Check(Bars.Num() == 320 * 240 && Bars[10] == Colour(0xc0c0c0) && Bars[319] == Colour(0x0000c0), TEXT("the fallback card's 320 by 240 colour bars"));
	Check(FilmLight(0.25) == 1 && FilmLight(0) == 0.35 && FilmLight(1) == 1.35, TEXT("the broadcast's light on the office follows its picture, within the card's range"));
	bOk &= FS47TvProgramme::SelfTest(Lines);
	return bOk;
}

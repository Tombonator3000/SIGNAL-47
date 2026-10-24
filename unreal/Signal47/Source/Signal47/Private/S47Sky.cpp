#include "S47Sky.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "S47Three.h"
#include "S47Web.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Sky, Log, All);

namespace
{
	constexpr double DEG = UE_DOUBLE_PI / 180;
	// (measured as the chord between two directions: Sky.ts chord)
	double Chord(double Deg) { return 2 * FMath::Sin(Deg * DEG / 2); }
	double Clamp01(double V) { return FMath::Clamp(V, 0.0, 1.0); }
	double Smooth01(double V) { const double K = Clamp01(V); return K * K * (3 - 2 * K); }
	double Rand() { return FMath::FRand(); }
	FLinearColor V4(const FVector& V) { return FLinearColor(float(V.X), float(V.Y), float(V.Z), 0.f); }

	UStaticMeshComponent* Part(AS47Web* W, USceneComponent* Under, UStaticMesh* Mesh, UMaterialInterface* Mat, int32 SortPriority)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W);
		C->SetMobility(EComponentMobility::Movable);
		C->SetupAttachment(Under);
		C->SetStaticMesh(Mesh);
		if (Mat) C->SetMaterial(0, Mat);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->bAffectDistanceFieldLighting = false;
		C->SetVisibleInRayTracing(false);
		C->SetTranslucentSortPriority(SortPriority);
		C->RegisterComponent();
		return C;
	}
}

FVector S47SkyDir(double Az, double El)
{
	// skyDir: v.set(sin(az) cos(el), sin(el), -cos(az) cos(el))
	return FVector(FMath::Sin(Az * DEG) * FMath::Cos(El * DEG), FMath::Sin(El * DEG), -FMath::Cos(Az * DEG) * FMath::Cos(El * DEG));
}

double S47CalicheVisualWeight(const FVector& EyeThree)
{
	static const bool bLegacy = FParse::Param(FCommandLine::Get(), TEXT("S47LegacyCalicheConcept")) || FParse::Param(FCommandLine::Get(), TEXT("S47LegacyCalicheStreet")) || FParse::Param(FCommandLine::Get(), TEXT("S47PropsOff"));
	if (bLegacy) return 0;
	const double D = FVector2D(EyeThree.X - 8000, EyeThree.Z - 2050).Size();
	return 1 - Smooth01((D - 120) / 480);
}

double S47StarHoleAt(double Clock, double From)
{
	const double S = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
	if (S > 12 * 3600 || !FMath::IsFinite(From)) return 0;
	const double K = Clamp01((S - From) / STAR_HOLE.Fade);
	return K * K * (3 - 2 * K);
}

FS47Sky::FS47Sky()
{
	HoleR = FVector2D(Chord(STAR_HOLE.Inner), Chord(STAR_HOLE.Outer));
}

// ---------- Sky.ts ----------

void FS47Sky::MeteorNow()
{
	FMet& M = Met;
	if (M.T >= 0) return;
	// somewhere between 25 and 70 degrees up, any direction
	const double Az = Rand() * UE_DOUBLE_PI * 2, El = (25 + Rand() * 45) * UE_DOUBLE_PI / 180;
	M.Start = FVector(FMath::Sin(Az) * FMath::Cos(El), FMath::Sin(El), -FMath::Cos(Az) * FMath::Cos(El));
	// across the sky and mostly downwards, as they seem to fall towards the horizon
	const FVector Down = (FVector(0, -1, 0) + M.Start * M.Start.Y).GetSafeNormal();
	// (three's crossVectors, in its right-handed terms: the formula is the same)
	const FVector Side = FVector::CrossProduct(M.Start, Down).GetSafeNormal();
	M.Tan = (Down * (0.6 + Rand() * 0.6) + Side * ((Rand() * 2 - 1) * 1.2)).GetSafeNormal();
	M.Len = (8 + Rand() * 10) * UE_DOUBLE_PI / 180;
	M.Dur = 0.45 + Rand() * 0.5;
	// now and then a bright one
	M.Bright = Rand() < 0.15 ? 2.6 : 1.1 + Rand() * 0.6;
	M.T = 0;
	Meteors++;
}

void FS47Sky::MeteorFrom(const FVector& Dir)
{
	// (as meteorNow lays out its way, from a start the test chose)
	FMet& M = Met;
	if (M.T < 0) return;
	M.Start = Dir.GetSafeNormal();
	const FVector Down = (FVector(0, -1, 0) + M.Start * M.Start.Y).GetSafeNormal();
	const FVector Side = FVector::CrossProduct(M.Start, Down).GetSafeNormal();
	M.Tan = (Down * 0.9 + Side * 0.3).GetSafeNormal();
}

void FS47Sky::MeteorStep(double Dt)
{
	FMet& M = Met;
	M.Next -= Dt;
	// only while it is night: none in the dawn
	if (M.Next <= 0) { M.Next = 18 + Rand() * 22; if (Daylight() < 0.3) MeteorNow(); }
	if (M.T < 0) { bMetVisible = false; return; }
	M.T += Dt;
	const double TAIL = 0.2;                                   // the trail lingers a moment after the head is gone
	if (M.T > M.Dur + TAIL) { M.T = -1; bMetVisible = false; return; }
	const double P = FMath::Min(1.0, M.T / M.Dur);
	Head = M.Len * P;
	Back = FMath::Max(0.0, Head - M.Len * 0.65) + M.Len * 0.65 * FMath::Max(0.0, M.T - M.Dur) / TAIL;
	const double Fade = FMath::Min(1.0, M.T / 0.08) * (M.T > M.Dur ? 0.6 * (1 - (M.T - M.Dur) / TAIL) : 1 - 0.4 * P);
	MetI = M.Bright * Fade * (1 - Daylight());
	// (the strip's corners along the great circle from the start, and the way across it, are laid out
	// from these by its material, M_WebMeteor: the loop over MET_N is its vertex shader)
	bMetVisible = true;
}

void FS47Sky::SetClock(double Clock, TOptional<double> SunElevDeg)
{
	const double S = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
	const double K = S > 12 * 3600 ? 0 : Clamp01((S - (4 * 3600 + 40 * 60)) / (50 * 60));
	const double D = FMath::Pow(K, 1.6) * DawnLift;
	Dawn = D;
	// Rendering-only daytime plateau. Never raise Dawn: chapters and anomaly gates retain
	// the original 04:40-05:40 curve and per-area lift. Both joins have zero slope.
	Midday = Smooth01((S - (5 * 3600 + 40 * 60)) / (20 * 60)) *
		(1 - Smooth01((S - 17 * 3600) / 3600));
	// EveningShift extends the existing dawn palette backwards. Keep this separate from Dawn:
	// anomaly/story gates query the morning curve, while drawing and native light use Daylight().
	// Art-directed MST reference: sunset 18:27, civil twilight 18:53, last blue hour 19:07.
	// Keep the sunlit palette through shift arrival. The old full-night-to-dawn shader
	// otherwise reads 18:00 as dark twilight despite the sun still being 4.53 degrees up.
	const double EveningK = S > 12 * 3600 ? Clamp01(((19 * 3600 + 7 * 60) - S) / (67 * 60)) : 0;
	Evening = FMath::Pow(EveningK * EveningK * (3 - 2 * EveningK), 1.4);
	// from 02:15, where the dishes point (it fades with the stars at dawn, by itself)
	HoleK = S47StarHoleAt(Clock, HoleFrom);
	CloudCover = CloudCoverAt(Clock); Cirrus = CirrusAt(Clock);
	const FLinearColor A(0.043f, 0.059f, 0.098f), B(0.19f, 0.2f, 0.25f);
	FogColor = A + (B - A) * float(D * 0.8);
	// the sun: azimuth 79 degrees, rising about 0.18 degrees a minute; its upper limb reaches
	// the horizon at 05:30, so the glow shows a few minutes before
	const double El = (SunElevDeg.IsSet() ? *SunElevDeg : 0.33 + (S - (5 * 3600 + 30 * 60)) / 60 * 0.18) * UE_DOUBLE_PI / 180, Az = 79 * UE_DOUBLE_PI / 180;
	SunDir = FVector(FMath::Sin(Az) * FMath::Cos(El), FMath::Sin(El), -FMath::Cos(Az) * FMath::Cos(El));
	Sun = S > 12 * 3600 ? 0 : SunElevDeg.IsSet() ? 1 : Clamp01((S - (5 * 3600 + 26 * 60)) / 240) * DawnLift;
	if (Evening > 0)
	{
		const double Sunset = 18 * 3600 + 27 * 60;
		const double EveningEl = -0.33 + (Sunset - S) / 60 * 0.18;
		// Western sun: never mirror the old eastern sunrise or drag it round after dusk.
		SunDir = S47SkyDir(281, EveningEl);
		Sun = Clamp01((EveningEl + 0.55) / 1.5);
		const FLinearColor Haze(0.34f, 0.26f, 0.24f);
		FogColor = A + (Haze - A) * float(Evening * 0.8);
	}
	if (Midday > 0)
	{
		// A modest April/New Mexico solar approximation: north maps to -Z in three,
		// with solar noon at 12:00. Unlike the sunrise extrapolation, it crosses the
		// southern sky and cannot turn underground after noon. This is art calibration,
		// not a replacement for the authored 18:27 sunset or the finale's elevation.
		if (!SunElevDeg.IsSet())
		{
			const double Latitude = 33.4 * DEG, Declination = 9.1 * DEG;
			const double HourAngle = (S - 12 * 3600) * UE_DOUBLE_PI / (12 * 3600);
			const double East = -FMath::Cos(Declination) * FMath::Sin(HourAngle);
			const double Up = FMath::Sin(Latitude) * FMath::Sin(Declination) + FMath::Cos(Latitude) * FMath::Cos(Declination) * FMath::Cos(HourAngle);
			const double North = FMath::Cos(Latitude) * FMath::Sin(Declination) - FMath::Sin(Latitude) * FMath::Cos(Declination) * FMath::Cos(HourAngle);
			const FVector DaySun(East, Up, -North);
			SunDir = (SunDir * (1 - Midday) + DaySun * Midday).GetSafeNormal();
			Sun = Sun + (1 - Sun) * Midday;
		}
		// The dome daylight and horizon band's Day texture receive Daylight()==1. Cool the shared
		// haze too; this blend is zero throughout the documented dawn and evening fades.
		FogColor = FogColor + (FLinearColor(0.30f, 0.40f, 0.55f) - FogColor) * float(Midday);
	}
}

double FS47Sky::CloudCoverAt(double Clock)
{
	const double H = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0) / 3600;
	// A slow, deterministic variation over the day: broken alto/stratocumulus, 40-70 %.
	const double Day = 0.55 + 0.11 * FMath::Sin(H * 0.83 + 0.6) + 0.04 * FMath::Sin(H * 2.3 + 1.1);
	// A few scattered clouds that now and then cross the moon; never a closed overcast.
	const double Night = 0.11 + 0.04 * FMath::Sin(H * UE_DOUBLE_PI / 6 + 0.4); // 24 h periodic: no jump at midnight
	const double K = H >= 12 ? 1 - Smooth01((H - 18.8) / 2.2) : Smooth01((H - 4.5) / 2.0);
	// The dusk concept: a broad broken layer over the commute and the handover (about 17:40-19:00).
	const double Dusk = FMath::Exp(-FMath::Square((H - 18.35) / 0.55));
	return FMath::Min(0.7, Night + (Day + 0.16 * Dusk - Night) * K);
}

double FS47Sky::CirrusAt(double Clock)
{
	const double H = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0) / 3600;
	const double K = H >= 12 ? 1 - Smooth01((H - 19.0) / 2.5) : Smooth01((H - 4.2) / 2.0);
	return 0.12 + 0.36 * K + 0.08 * FMath::Sin(H * UE_DOUBLE_PI / 12 + 2.0);
}

void FS47Sky::SetCometClock(double Clock)
{
	const double S = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
	bComet = S > 20 * 3600 || S < 3 * 3600 + 5 * 60;
}

void FS47Sky::Update(double Dt, double T)
{
	Time = T;
	MeteorStep(Dt);
	// Sunlit daytime and the friendly arrival have no forced night storm or thunder.
	if (Evening > 0 || Midday > 0)
	{
		Flash = 0; FlashT = 0;
		return;
	}
	NextFlash -= Dt;
	if (NextFlash <= 0)
	{
		FlashT = 1;
		NextFlash = 14 + Rand() * 26;
		const double A = -0.2 + Rand() * 1.6;
		FlashDir = FVector(FMath::Sin(A), 0.04, -FMath::Cos(A)).GetSafeNormal();
		if (OnThunder) OnThunder(3 + Rand() * 5, 0.4 + Rand() * 0.6);
	}
	if (FlashT > 0)
	{
		FlashT = FMath::Max(0.0, FlashT - Dt * 2.2);
		const double Flick = FlashT > 0.55 ? (FMath::Sin(T * 90) > 0 ? 1 : 0.3) : FlashT;
		Flash = Flick * FlashT;
	}
	else Flash = 0;
}

// ---------- Unreal ----------

bool FS47Sky::Build(AS47Web* W)
{
	Web = W;
	const TSharedPtr<FJsonObject>& L = W ? W->LifeData() : nullptr;
	const TSharedPtr<FJsonObject>* SkyJ = nullptr;
	if (!L.IsValid() || !L->TryGetObjectField(TEXT("sky"), SkyJ)) return false;
	UMaterialInterface* DomeBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebSkyDome.M_WebSkyDome"));
	// A locally calibrated sky keeps the existing night/story uniforms and the old import as fallback.
	if (UMaterialInterface* Concept = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/CalicheStreet/Atmosphere/M_Caliche_Sky.M_Caliche_Sky"))) DomeBase = Concept;
	UMaterialInterface* StarsBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebStars.M_WebStars"));
	if (UMaterialInterface* Concept = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/CalicheStreet/Atmosphere/M_Caliche_Stars.M_Caliche_Stars"))) StarsBase = Concept;
	UMaterialInterface* MeteorBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebMeteor.M_WebMeteor"));
	UMaterialInterface* BandBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebSkyBand.M_WebSkyBand"));
	if (UMaterialInterface* Concept = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/CalicheStreet/Atmosphere/M_Caliche_Horizon.M_Caliche_Horizon"))) BandBase = Concept;
	if (!DomeBase || !StarsBase || !MeteorBase || !BandBase)
	{
		UE_LOG(LogS47Sky, Warning, TEXT("S47: the live sky's materials are not imported (Tools/webexport/import.sh): the sky stays the export's picture"));
		return false;
	}
	const TSharedPtr<FJsonObject>& S = *SkyJ;
	auto Num = [&S](const TCHAR* K, double D) { double V = D; S->TryGetNumberField(K, V); return V; };
	auto Tex = [](int32 Id) { return Id >= 0 ? LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Web/Tex/T%d.T%d"), Id, Id)) : nullptr; };
	GroupNode = int32(Num(TEXT("group"), -1));
	const FS47WebNode* G = W->Node(GroupNode);
	if (!G || !G->Comp.IsValid()) return false;
	ConceptBackdropNode = -1;
	for (int32 I = 0; I < W->PartCount(); ++I)
		if (const FS47WebNode* N = W->Node(I); N && N->Name == TEXT("prop:streetGroundMesas"))
			if (UStaticMeshComponent* C = Cast<UStaticMeshComponent>(N->Comp.Get()); C && C->GetStaticMesh()) ConceptBackdropNode = I;
	// Original horizon.ts meshes have baked night vertex colours. They stay black at
	// noon; the authored local escarpments replace them only while actually visible.
	ConceptLegacyMesas.Reset(); bConceptLegacyMesasHidden = false;
	for (int32 I = 0; I < W->PartCount(); ++I)
		if (const FS47WebNode* N = W->Node(I); N && N->Geo >= 0 &&
			(N->Name == TEXT("mesas:near") || N->Name == TEXT("mesas")))
			ConceptLegacyMesas.Emplace(I, W->IsVisible(I));
	const TArray<TSharedPtr<FJsonValue>>* HS = nullptr;
	if (S->TryGetArrayField(TEXT("holeSky"), HS) && HS->Num() >= 3) HoleSky = FLinearColor((*HS)[0]->AsNumber(), (*HS)[1]->AsNumber(), (*HS)[2]->AsNumber());
	auto Under = [W](int32 Node) -> USceneComponent* { const FS47WebNode* N = W->Node(Node); return N ? N->Comp.Get() : nullptr; };
	// the dome: Sky.ts's fragment shader with the painted Milky Way (milkyWay, its stars taken out)
	if (const FS47WebNode* N = W->Node(int32(Num(TEXT("dome"), -1))))
	{
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(DomeBase, W);
		if (UTexture2D* P = Tex(int32(Num(TEXT("panorama"), -1)))) D->SetTextureParameterValue(TEXT("Pano"), P);
		D->SetVectorParameterValue(TEXT("HoleSky"), HoleSky);
		DomeMid = D;
		DomeComp = Part(W, N->Comp.Get(), W->GeoMesh(N->Geo), D, 0);
	}
	// the stars: a quad each, turned to the eye (custom data: direction, colour, size, phase)
	const int32 StarsNode = int32(Num(TEXT("stars"), -1));
	const TArray<float>* Pts = W->PointsOf(StarsNode);
	const TArray<float>* Ph = W->PointsPhOf(StarsNode);
	if (Pts && Pts->Num() >= 7)
	{
		UInstancedStaticMeshComponent* I = NewObject<UInstancedStaticMeshComponent>(W);
		I->SetMobility(EComponentMobility::Movable);
		I->SetupAttachment(Under(StarsNode));
		I->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(StarsBase, W);
		StarsMid = D;
		I->SetMaterial(0, D);
		I->NumCustomDataFloats = 8;
		I->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		I->SetCastShadow(false);
		I->bAffectDistanceFieldLighting = false;
		I->SetVisibleInRayTracing(false);
		I->SetTranslucentSortPriority(-4);
		const int32 N = Pts->Num() / 7;
		TArray<FTransform> T;
		T.Reserve(N);
		for (int32 k = 0; k < N; ++k)
		{
			const float* P = Pts->GetData() + k * 7;
			T.Add(FTransform(FQuat::Identity, S47Three::ToUE(P[0], P[1], P[2]), FVector(0.001)));
		}
		I->AddInstances(T, false);
		for (int32 k = 0; k < N; ++k)
		{
			const float* P = Pts->GetData() + k * 7;
			const FVector Dir = FVector(P[0], P[1], P[2]).GetSafeNormal();
			const TArray<float> Cd = { float(Dir.X), float(Dir.Y), float(Dir.Z), P[3], P[4], P[5], P[6], Ph && Ph->IsValidIndex(k) ? (*Ph)[k] : 0.f };
			I->SetCustomData(k, Cd, false);
		}
		I->RegisterComponent();
		StarsComp = I;
	}
	// the comet (Halley's, April 1986: its own material from the export)
	if (const FS47WebNode* N = W->Node(int32(Num(TEXT("comet"), -1))))
	{
		CometComp = Part(W, N->Comp.Get(), W->GeoMesh(N->Geo), N->Mats.Num() ? W->MatAsset(N->Mats[0]) : nullptr, -3);
	}
	// the shooting star's strip
	if (const FS47WebNode* N = W->Node(int32(Num(TEXT("meteor"), -1))))
	{
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(MeteorBase, W);
		MeteorMid = D;
		MeteorComp = Part(W, N->Comp.Get(), W->GeoMesh(N->Geo), D, -2);
		if (MeteorComp.IsValid()) MeteorComp->SetVisibility(false);
	}
	// the painted horizon (backdrop.ts), over all of them
	if (const FS47WebNode* N = W->Node(int32(Num(TEXT("band"), -1))))
	{
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(BandBase, W);
		if (UTexture2D* T = Tex(int32(Num(TEXT("night"), -1)))) D->SetTextureParameterValue(TEXT("Night"), T);
		if (UTexture2D* T = Tex(int32(Num(TEXT("day"), -1)))) D->SetTextureParameterValue(TEXT("Day"), T);
		D->SetScalarParameterValue(TEXT("Texel"), float(Num(TEXT("texel"), 1.0 / 128)));
		BandMid = D;
		BandComp = Part(W, N->Comp.Get(), W->GeoMesh(N->Geo), D, -1);
	}
	bLive = DomeComp.IsValid() && DomeComp->GetStaticMesh() != nullptr;
	if (bLive && W->SkySphereComponent()) W->SkySphereComponent()->SetVisibility(false);
	UE_LOG(LogS47Sky, Display, TEXT("S47: live sky %s: dome %s, %d stars, comet %s, shooting star %s, band %s"), bLive ? TEXT("on") : TEXT("off"),
		DomeComp.IsValid() ? TEXT("yes") : TEXT("no"), StarsComp.IsValid() ? StarsComp->GetInstanceCount() : 0, CometComp.IsValid() ? TEXT("yes") : TEXT("no"),
		MeteorComp.IsValid() ? TEXT("yes") : TEXT("no"), BandComp.IsValid() ? TEXT("yes") : TEXT("no"));
	return bLive;
}

void FS47Sky::Draw(const FVector& Eye, double Yaw)
{
	AS47Web* W = Web.Get();
	if (!bLive || !W) return;
	// sky.group.rotation.y = world.skyYaw; group.position.copy(follow); ten times as far (SKY_SCALE)
	W->SetLocal(GroupNode, Eye, S47Three::Euler(0, Yaw, 0), FVector(SKY_SCALE));
	const FLinearColor Holes = V4(HoleDir), HoleRad(float(HoleR.X), float(HoleR.Y), 0.f, 0.f);
	auto Common = [&](UMaterialInstanceDynamic* D)
	{
		if (!D) return;
		D->SetScalarParameterValue(TEXT("Dawn"), float(Daylight()));
		D->SetVectorParameterValue(TEXT("HoleDir"), Holes);
		D->SetVectorParameterValue(TEXT("HoleR"), HoleRad);
		D->SetScalarParameterValue(TEXT("Hole"), float(HoleK));
		D->SetScalarParameterValue(TEXT("ConceptBlend"), float(S47CalicheVisualWeight(Eye)));
	};
	auto Sunny = [&](UMaterialInstanceDynamic* D)
	{
		if (!D) return;
		D->SetScalarParameterValue(TEXT("Yaw"), float(Yaw));
		D->SetScalarParameterValue(TEXT("Dawn"), float(Daylight()));
		D->SetScalarParameterValue(TEXT("Sun"), float(Sun));
		D->SetScalarParameterValue(TEXT("ConceptBlend"), float(S47CalicheVisualWeight(Eye)));
		D->SetVectorParameterValue(TEXT("SunDir"), V4(SunDir));
		for (int32 k = 0; k < 3; ++k) D->SetVectorParameterValue(*FString::Printf(TEXT("Glow%d"), k), FLinearColor(float(Glow[k].X), float(Glow[k].Y), float(Glow[k].Z), float(Glow[k].W)));
	};
	if (UMaterialInstanceDynamic* D = DomeMid.Get())
	{
		Sunny(D); Common(D);
		D->SetScalarParameterValue(TEXT("Flash"), float(Flash));
		D->SetScalarParameterValue(TEXT("ConceptBlend"), float(S47CalicheVisualWeight(Eye)));
		D->SetScalarParameterValue(TEXT("Midday"), float(Midday));
		D->SetVectorParameterValue(TEXT("FlashDir"), V4(FlashDir));
	}
	Common(StarsMid.Get());
	if (UMaterialInstanceDynamic* D = StarsMid.Get()) D->SetScalarParameterValue(TEXT("Dawn"), float(StarDimming()));
	// SKY.md: the volumetric layer. Its own clouds replace the painted ones (Low keeps the painted).
	const float Vol = W->CloudsActive() ? 1.f : 0.f;
	if (UMaterialInstanceDynamic* D = DomeMid.Get())
	{
		D->SetScalarParameterValue(TEXT("VolClouds"), Vol);
		// The painted cirrus above the layer, drifting slowly with the same west-southwest wind.
		D->SetScalarParameterValue(TEXT("Cirrus"), float(Cirrus));
		D->SetVectorParameterValue(TEXT("CirrusDrift"), FLinearColor(float(FMath::Fmod(Time * 0.0011, 1000.0)), float(FMath::Fmod(Time * 0.0004, 1000.0)), 0.f, 0.f));
	}
	if (UMaterialInstanceDynamic* D = StarsMid.Get()) D->SetScalarParameterValue(TEXT("VolClouds"), Vol);
	{
		// Sky terms -> three's world: the group turns by Yaw about +Y (three's rotation.y).
		auto Turn = [Yaw](const FVector& V) { const double C = FMath::Cos(Yaw), S = FMath::Sin(Yaw); return FVector(V.X * C + V.Z * S, V.Y, -V.X * S + V.Z * C); };
		// Story beats keep open sky: the star hole where the dishes point from 02:15, and Halley's comet.
		const FVector Hole = Turn(HoleDir.GetSafeNormal());
		FVector Comet = FVector(0, 0, 1); double CometK = 0;
		if (CometComp.IsValid() && CometComp->IsVisible())
		{
			const FVector D = (CometComp->Bounds.Origin - S47Three::ToUE(Eye)).GetSafeNormal();
			Comet = FVector(D.X, D.Z, D.Y); CometK = 1 - Daylight();
		}
		// By day the same slot keeps a low sun open where the player stands (late afternoon commute,
		// sunrise): its light still reaches the road under the layer; shadows still drift elsewhere.
		if (CometK < 0.05 && Sun > 0 && SunDir.Y > -0.02 && SunDir.Y < 0.4)
		{
			Comet = SunDir.GetSafeNormal(); CometK = 0.85 * (1 - FMath::Clamp((SunDir.Y - 0.25) / 0.15, 0.0, 1.0));
		}
		W->SetClouds(float(CloudCover), float(Cirrus), FVector4(Hole.X, Hole.Y, Hole.Z, HoleK), FVector4(Comet.X, Comet.Y, Comet.Z, CometK),
			float(Flash), Turn(FlashDir), Time);
	}
	if (CometComp.IsValid() && CometComp->IsVisible() != bComet) CometComp->SetVisibility(bComet);
	if (MeteorComp.IsValid() && MeteorComp->IsVisible() != bMetVisible) MeteorComp->SetVisibility(bMetVisible);
	if (UMaterialInstanceDynamic* D = MeteorMid.Get(); D && bMetVisible)
	{
		Common(D);
		D->SetScalarParameterValue(TEXT("Yaw"), float(Yaw));
		D->SetScalarParameterValue(TEXT("Scale"), float(SKY_SCALE * 100));
		D->SetScalarParameterValue(TEXT("Back"), float(Back));
		D->SetScalarParameterValue(TEXT("Head"), float(Head));
		D->SetVectorParameterValue(TEXT("Start"), V4(Met.Start));
		D->SetVectorParameterValue(TEXT("Way"), V4(Met.Tan));
		D->SetScalarParameterValue(TEXT("I"), float(MetI));
	}
	const FS47WebNode* BackdropNode = W->Node(ConceptBackdropNode);
	const UStaticMeshComponent* Backdrop = BackdropNode ? Cast<UStaticMeshComponent>(BackdropNode->Comp.Get()) : nullptr;
	const bool bBackdropReady = Backdrop && Backdrop->GetStaticMesh() && W->ObjAt(ConceptBackdropNode).Shown();
	const bool bReplaceLegacy = bBackdropReady && S47CalicheVisualWeight(Eye) > .999;
	if (bReplaceLegacy != bConceptLegacyMesasHidden)
	{
		for (const TPair<int32, bool>& Entry : ConceptLegacyMesas) W->SetVisible(Entry.Key, Entry.Value && !bReplaceLegacy);
		bConceptLegacyMesasHidden = bReplaceLegacy;
		UE_LOG(LogS47Sky, Display, TEXT("S47: Caliche legacy night mesas %s: %d original visibility flags preserved"),
			bReplaceLegacy ? TEXT("replaced") : TEXT("restored"), ConceptLegacyMesas.Num());
	}
	if (UMaterialInstanceDynamic* D = BandMid.Get())
	{
		Sunny(D);
		// A partial import must keep the painted horizon, even with the new sky material present.
		D->SetScalarParameterValue(TEXT("BackdropReady"), bBackdropReady ? 1.f : 0.f);
	}
	if (BandComp.IsValid() && BandComp->IsVisible() != BandVisible) BandComp->SetVisibility(BandVisible);
}

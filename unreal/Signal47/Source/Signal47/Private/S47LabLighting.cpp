#include "S47Web.h"

#include "Components/LocalLightComponent.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "S47Three.h"

namespace
{
	constexpr int32 SafeWall = 0, Ceiling = 1, Task = 2, Enlarger = 3, Warning = 4;
	const FLinearColor NormalColor = []
	{
		const FLinearColor C = FLinearColor::MakeFromColorTemperature(4000.f);
		return C / FMath::Max(1.f, FMath::Max3(C.R, C.G, C.B));
	}();
	const FLinearColor SafeColor = S47Three::Hex(0xff2616);
	const FLinearColor TaskColor = S47Three::Hex(0xffc98a);
	const FLinearColor PrintColor = S47Three::Hex(0xffe6c4);
}

void AS47Web::BuildLabLighting()
{
	for (FLabLight& P : LabLights) if (ULocalLightComponent* L = P.Light.Get()) L->DestroyComponent();
	LabLights.Reset();
	const int32 Lab = FloodSet(TEXT("lab"));
	LabScaleBase = FMath::Max(0.001f, FloodScaleOf(Lab));
	// Named controls are state only. FS47Light converts point intensities by 1.5, so its API
	// exposes a plain 0..1 level; only the fittings below create scene lights.
	auto Driver = [this](const TCHAR* Path, float Level, const FLinearColor& Color)
	{
		if (LightByUuid(Path)) return;
		UPointLightComponent* L = NewObject<UPointLightComponent>(this);
		L->SetupAttachment(GetRootComponent());
		L->SetMobility(EComponentMobility::Movable);
		L->bAffectsWorld = false;
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(Level * 1.5f);
		L->SetLightColor(Color);
		L->RegisterComponent();
		Lights.Add({ L, TEXT("saro"), Level * 1.5f, Path });
		HandleLights.Add(Path, Path);
	};
	Driver(TEXT("lab.normal"), 1.f, NormalColor);
	Driver(TEXT("lab.safelight"), 0.f, SafeColor);
	Driver(TEXT("lab.enlarger"), 0.f, PrintColor);
	UMaterialInterface* GlassBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
	auto Glass = [&](int32 Role, int32 Slot, const TCHAR* Path, const FVector& Facing, float Lumens, float Reach)
	{
		const int32 MatId = HandleMat(Path);
		if (MatId < 0) return -1;
		for (int32 I = 0; I < Nodes.Num(); ++I) if (Nodes[I].Mats.Contains(MatId))
		{
			UStaticMeshComponent* M = Cast<UStaticMeshComponent>(Nodes[I].Comp.Get());
			URectLightComponent* L = SurfaceLight(I, Facing, Lumens, Reach);
			if (!M || !L) return -1;
			FLabLight P;
			P.Node = I; P.Role = Role; P.Slot = Slot; P.Lumens = Lumens; P.Light = L;
			P.WidthBase = Slot >= 0 ? FMath::Max(0.001f, FloodPos(Lab, Slot).A) : 1.f;
			if (GlassBase)
			{
				UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(GlassBase, this);
				D->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.22f, 0.22f, 0.20f));
				D->SetScalarParameterValue(TEXT("Roughness"), 0.5f);
				D->SetVectorParameterValue(TEXT("Emissive"), FLinearColor::Black);
				for (int32 K = 0; K < M->GetNumMaterials(); ++K) M->SetMaterial(K, D);
				KeepMids.Add(D); P.Diffuser = D;
			}
			LabLights.Add(P);
			return I;
		}
		return -1;
	};
	const int32 Wall = Glass(SafeWall, 0, TEXT("yard.redGlass[0]"), FVector(0, 0, 1), 24.f, 4.f);
	// Normal work light serves the actual 40.9 m2 lab. Its former 45 lm left
	// the wet bench unlit even with Lumen. Safe ceiling flux stays independently 12 lm.
	Glass(Ceiling, 1, TEXT("yard.redGlass[1]"), FVector(0, -1, 0), 450.f, 6.f);
	Glass(Warning, 0, TEXT("yard.redGlass[2]"), FVector(-1, 0, 0), 0.5f, 0.8f);
	if (Wall >= 0)
	{
		const int32 Group = Nodes[Wall].Parent;
		// ServiceYard.ts: the task shade and enlarger lens were merged into the lab's plastic.
		// Their openings remain at these exact positions. Never brighten the whole merged mesh.
		int32 Plastic = -1;
		for (int32 I = 0; I < Nodes.Num(); ++I)
		{
			const UStaticMeshComponent* M = Cast<UStaticMeshComponent>(Nodes[I].Comp.Get());
			if (Nodes[I].Parent != Group || !M || !M->GetStaticMesh()) continue;
			const FBox B = M->GetStaticMesh()->GetBoundingBox();
			const FVector Lens = M->GetComponentTransform().InverseTransformPosition(S47Three::ToUE(17.0, 1.5, -5.45));
			if (!B.IsInsideOrOn(Lens)) continue;
			FLinearColor C;
			if (M->GetMaterial(0) && M->GetMaterial(0)->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Color")), C) && C.Equals(S47Three::Hex(0x1b1d1f), 0.001f)) { Plastic = I; break; }
		}
		if (Plastic >= 0)
		{
			FLabLight P;
			P.Node = Plastic; P.Role = Task; P.Slot = 2; P.Lumens = 18.f;
			P.WidthBase = FMath::Max(0.001f, FloodPos(Lab, 2).A);
			if (URectLightComponent* L = SurfaceLight(Wall, FVector(FMath::Sin(0.7), -FMath::Cos(0.7), 0), P.Lumens, 3.f))
			{
				L->SetWorldLocation(S47Three::ToUE(18.6151, 1.25646, -3.76));
				L->SetSourceWidth(20.f); L->SetSourceHeight(20.f);
				L->SetBarnDoorAngle(55.f); L->SetBarnDoorLength(12.f);
				P.Light = L; LabLights.Add(P);
			}
			USpotLightComponent* L = NewObject<USpotLightComponent>(this);
			L->SetupAttachment(GetRootComponent());
			L->SetMobility(EComponentMobility::Movable);
			// The lens's lower cap is at y=1.50, diameter .12 m. Start below its opaque cap.
			L->SetWorldLocationAndRotation(S47Three::ToUE(17.0, 1.49, -5.45), S47Three::DirToUE(FVector(0, -1, 0)).Rotation());
			L->SetIntensityUnits(ELightUnits::Lumens);
			L->SetIntensity(0.f); L->SetAttenuationRadius(120.f);
			L->SetInnerConeAngle(15.f); L->SetOuterConeAngle(22.f);
			L->SetSourceRadius(2.5f); L->SetSoftSourceRadius(1.f);
			L->SetCastShadows(true); L->SetCastVolumetricShadow(true);
			L->ContactShadowLengthInWS = true; L->ContactShadowLength = 6.f;
			L->ContactShadowCastingIntensity = 0.8f;
			L->SetShadowBias(0.3f); L->SetShadowSlopeBias(0.35f);
			L->SetVolumetricScatteringIntensity(0.f);
			L->SetIndirectLightingIntensity(1.f);
			L->RegisterComponent();
			FLabLight E;
			E.Node = Plastic; E.Role = Enlarger; E.Lumens = 8.f; E.Light = L;
			LabLights.Add(E);
		}
	}
	// Remove all legacy lab fills, including the unmodelled records-desk pool.
	for (int32 I = 0; I < FloodSlots(Lab); ++I) SuppressFloodLight(Lab, I);
	UpdateLabLighting();
	UE_LOG(LogTemp, Display, TEXT("S47: lab lighting %d physical sources, named normal/safelight/enlarger controls"), LabLights.Num());
}

void AS47Web::UpdateLabLighting()
{
	const int32 Lab = FloodSet(TEXT("lab"));
	const float Power = FMath::Max(0.f, FloodScaleOf(Lab) / FMath::Max(0.001f, LabScaleBase));
	auto Level = [this](const TCHAR* Path)
	{
		const FS47Light D = Light(Path);
		return D.Visible() ? FMath::Max(0.f, D.Intensity()) : 0.f;
	};
	const float Normal = Level(TEXT("lab.normal")), Safe = Level(TEXT("lab.safelight")), Print = Level(TEXT("lab.enlarger"));
	const FLinearColor White = Light(TEXT("lab.normal")).Color(), Red = Light(TEXT("lab.safelight")).Color(), Paper = Light(TEXT("lab.enlarger")).Color();
	for (FLabLight& P : LabLights)
	{
		ULocalLightComponent* L = P.Light.Get();
		UStaticMeshComponent* M = Nodes.IsValidIndex(P.Node) ? Cast<UStaticMeshComponent>(Nodes[P.Node].Comp.Get()) : nullptr;
		if (!L || !M) continue;
		const float Width = P.Slot >= 0 ? FMath::Max(0.f, FloodPos(Lab, P.Slot).A) / FMath::Max(0.001f, P.WidthBase) : 1.f;
		const float Gate = ObjAt(P.Node).Shown() && M->IsVisible() && !M->bHiddenInGame ? Power * Width : 0.f;
		float Lumens = 0.f, Glow = 0.f;
		FLinearColor C = Red;
		if (P.Role == Ceiling)
		{
			const float WhiteLm = P.Lumens * Normal, RedLm = 12.f * Safe;
			Lumens = WhiteLm + RedLm;
			C = Lumens > 0.0001f ? (White * WhiteLm + Red * RedLm) / Lumens : White;
			Glow = Normal + Safe;
		}
		else if (P.Role == Task) { Lumens = P.Lumens * Normal; Glow = Normal; C = TaskColor; }
		else if (P.Role == Enlarger) { Lumens = P.Lumens * Print; Glow = Print; C = Paper; }
		else { Lumens = P.Lumens * Safe; Glow = Safe; }
		const float Before = L->Intensity;
		const FLinearColor BeforeColor = L->GetLightColor();
		L->SetLightColor(C);
		L->SetIntensity(Lumens * Gate);
		if (L->IsVisible() != (Lumens * Gate > 0.0001f)) L->SetVisibility(Lumens * Gate > 0.0001f);
		const FLinearColor AppliedColor = L->GetLightColor();
		if ((!FMath::IsNearlyEqual(Before, L->Intensity, 0.001f) || !BeforeColor.Equals(AppliedColor, 0.001f)) && FParse::Param(FCommandLine::Get(), TEXT("S47LightLog")))
			UE_LOG(LogTemp, Display, TEXT("S47: lab role=%d node=%d power=%.4f width=%.4f normal=%.4f safe=%.4f print=%.4f lm=%.3f rgb=(%.4f,%.4f,%.4f)"), P.Role, P.Node, Power, Width, Normal, Safe, Print, L->Intensity, AppliedColor.R, AppliedColor.G, AppliedColor.B);
		if (P.Diffuser.IsValid())
		{
			// Story SetColor can clone all material slots. Keep every glass face on the actual MID.
			UMaterialInstanceDynamic* D = Cast<UMaterialInstanceDynamic>(M->GetMaterial(0));
			if (!D) D = P.Diffuser.Get();
			P.Diffuser = D;
			for (int32 I = 0; I < M->GetNumMaterials(); ++I) if (M->GetMaterial(I) != D) M->SetMaterial(I, D);
			D->SetScalarParameterValue(TEXT("Brightness"), 1.f);
			D->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.22f, 0.22f, 0.20f));
			D->SetVectorParameterValue(TEXT("Emissive"), C * (Glow * Gate * 1.4f));
		}
	}
	// These additive sprites belonged to the old lamp fills. The real glass now supplies their
	// visible glow, so no red halos remain over an off lamp or over the white ceiling diffuser.
	if (FS47GlowPoints* G = Glow(TEXT("yard.glow.points")))
		for (const FVector& At : { FVector(15, 2.43, -5.5), FVector(15.9, 2.62, -2.4), FVector(18.6, 1.3, -3.75), FVector(12.47, 2.3, -0.42) })
			for (int32 I = 0; I < G->Pos.Num(); ++I)
				// Some night states park a glow below ground. Its x/z identity remains stable.
				if (FMath::Abs(G->Pos[I].X - At.X) < 0.01 && FMath::Abs(G->Pos[I].Z - At.Z) < 0.01 && G->Bill.IsValidIndex(I) && G->Bill[I].IsValid()) G->Bill[I]->SetVisibility(false);
}

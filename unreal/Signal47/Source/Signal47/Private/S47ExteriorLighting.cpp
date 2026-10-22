#include "S47Web.h"

#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "S47Three.h"

void AS47Web::BuildExteriorLighting()
{
	for (FExteriorLight& P : ExteriorLights) if (P.Light.IsValid()) P.Light->DestroyComponent();
	ExteriorLights.Reset();
	const int32 Site = FloodSet(TEXT("site")), Court = FloodSet(TEXT("court"));
	const bool bLog = FParse::Param(FCommandLine::Get(), TEXT("S47LightLog"));
	auto Add = [&](const TCHAR* Name, const TCHAR* OwnerPath, int32 Set, int32 Slot, const TCHAR* GlowPath, int32 GlowIndex,
		const FVector& At, const FVector& Target, float Candela, float Reach, float Cone)
	{
		const FS47Obj Owner = Obj(OwnerPath);
		if (!Owner || Set < 0 || Slot < 0 || Slot >= FloodSlots(Set)) return;
		FExteriorLight P;
		P.Node = Owner.I; P.Set = Set; P.Slot = Slot; P.At = At; P.Target = Target; P.Candela = Candela;
		P.Reference = FMath::Max(0.001f, FloodPos(Set, Slot).A * FloodScaleOf(Set));
		P.GlowNode = Handle(GlowPath); P.Glow = GlowIndex;
		if (FS47GlowPoints* G = Glow(P.GlowNode); G && G->Col.IsValidIndex(GlowIndex))
		{
			const FLinearColor C = G->Col[GlowIndex];
			P.GlowReference = FMath::Max(0.001f, FMath::Max3(C.R, C.G, C.B));
			// A glint belongs to the aperture, not a metre-wide additive ball in front of it.
			// The native light and fog supply the pool and the atmospheric halo.
			const bool bPost = (Set == Site && ((Slot >= 5 && Slot <= 7) || (Slot >= 12 && Slot <= 15))) || (Set == Court && Slot >= 10);
			G->SetSize(GlowIndex, bPost ? 0.4f : 0.22f);
			G->SetPosition(GlowIndex, NodeWorldThree(G->Node).InverseTransformPosition(At));
		}
		USpotLightComponent* L = NewObject<USpotLightComponent>(this, FName(*FString::Printf(TEXT("S47Exterior_%s"), Name)));
		L->SetupAttachment(GetRootComponent()); L->SetMobility(EComponentMobility::Movable);
		L->SetIntensityUnits(ELightUnits::Candelas); L->SetIntensity(0);
		L->SetWorldLocationAndRotation(S47Three::ToUE(At), S47Three::DirToUE(Target - At).Rotation());
		L->SetInnerConeAngle(Cone * 0.65f); L->SetOuterConeAngle(Cone);
		L->SetAttenuationRadius(Reach * 100.f);
		L->SetSourceRadius(8.f); L->SetSoftSourceRadius(6.f);
		L->SetCastShadows(true); L->SetCastVolumetricShadow(true);
		L->SetVolumetricScatteringIntensity(0.35f); L->SetIndirectLightingIntensity(1.f);
		L->SetVisibility(false); L->RegisterComponent();
		P.Light = L;
		ExteriorLights.Add(P);
		// Adjacent material sets can retain a copy at the old pool centre. Retire every copy
		// once so none can create an orphan light after the real source moves to its fixture.
		const FLinearColor Original = FloodPos(Set, Slot);
		const FVector OldAt(Original.R, Original.G, Original.B);
		for (int32 S = 0; S < FloodSets.Num(); ++S) for (int32 I = 0; I < FloodSets[S].Pos.Num(); ++I)
		{
			if (FloodSets[S].Key == TEXT("site") && I < 5) continue;
			const FLinearColor F = FloodSets[S].Pos[I];
			if (FVector(F.R, F.G, F.B).Equals(OldAt, 0.12)) SuppressFloodLight(S, I);
		}
		if (bLog) UE_LOG(LogTemp, Display, TEXT("S47: exterior %s set=%d slot=%d source=%s target=%s cd=%.2f reach=%.2fm"),
			Name, Set, Slot, *At.ToString(), *Target.ToString(), Candela, Reach);
	};
	// Exterior.ts: the three old highway pools lie between eight separate armature heads.
	// Fit all eight actual sources to their exported glows, preserving the existing power state.
	if (const FS47GlowPoints* G = Glow(TEXT("ext.glow.points")))
	{
		const FTransform T = NodeWorldThree(G->Node);
		for (int32 I = 0; I < 8; ++I)
		{
			const int32 Point = 11 + I;
			if (!G->Pos.IsValidIndex(Point)) continue;
			const FVector At = T.TransformPosition(G->Pos[Point]);
			const FString Name = FString::Printf(TEXT("Highway%02d"), I + 1);
			Add(*Name, TEXT("ext.group"), Site, 5 + FMath::Min(I, 2), TEXT("ext.glow.points"), Point,
				At, FVector(-24, -0.62, At.Z), 90.f, 16.f, 58.f);
		}
	}
	// Building wall packs: a shared material pool never substitutes for the real housings.
	// Sources sit just outside the metal boxes; their cones point down along the facade.
	Add(TEXT("WestSouth"), TEXT("ext.group"), Site, 8, TEXT("ext.glow.points"), 19,
		FVector(-6.59, 2.47, 2), FVector(-7.8, 0, 2), 35.f, 6.f, 58.f);
	Add(TEXT("WestNorth"), TEXT("ext.group"), Site, 8, TEXT("ext.glow.points"), 22,
		FVector(-6.59, 2.47, -3), FVector(-7.8, 0, -3), 35.f, 6.f, 58.f);
	Add(TEXT("WingWest"), TEXT("ext.group"), Site, 9, TEXT("ext.glow.points"), 20,
		FVector(20.9, 2.665, 5.94), FVector(20.9, 0, 7.1), 35.f, 6.f, 58.f);
	Add(TEXT("WingEast"), TEXT("ext.group"), Site, 9, TEXT("ext.glow.points"), 21,
		FVector(30.4, 2.665, 5.94), FVector(30.4, 0, 7.1), 35.f, 6.f, 58.f);
	// ServiceYard.ts / props.lampPost: the four sodium heads over the walk and truck pad.
	const FVector YardAt[4] = { FVector(10.3, 4.15, 1.6), FVector(10.3, 4.15, -6), FVector(8.7, 4.15, -9.2), FVector(7.8, 4.15, 11.6) };
	const FVector YardTarget[4] = { FVector(9.5, 0, 1.6), FVector(9.5, 0, -6), FVector(9.5, 0, -11), FVector(9.5, 0, 9) };
	for (int32 I = 0; I < 4; ++I)
	{
		const FString Name = FString::Printf(TEXT("Yard%02d"), I + 1);
		Add(*Name, TEXT("yard.group"), Site, 12 + I, TEXT("yard.glow.points"), I, YardAt[I], YardTarget[I], 65.f, 9.f, 55.f);
	}
	Add(TEXT("B12Reference"), TEXT("yard.group"), Site, 16, TEXT("yard.glow.points"), 4,
		FVector(9.42, 2.82, -17.7), FVector(9.9, 2.35, -18.35), 12.f, 3.f, 45.f);
	Add(TEXT("LabDoor"), TEXT("yard.group"), Site, 17, TEXT("yard.glow.points"), 5,
		FVector(12.4, 2.635, -2), FVector(11.2, 0, -1.2), 30.f, 5.f, 58.f);
	Add(TEXT("AnnexSouth"), TEXT("annex.group"), Site, 18, TEXT("annex.glow.points"), 0,
		FVector(-2, 2.265, 11.065), FVector(-2, -0.62, 12.2), 35.f, 6.f, 58.f);
	// Crossing.ts supplies the fire-exit ramp's state; Annex.ts owns the physical canopy.
	Add(TEXT("FireExit"), TEXT("annex.group"), Site, 19, TEXT("annex.glow.points"), 1,
		FVector(-6.45, 2.20, 5.7), FVector(-7.7, 0, 5.7), 25.f, 5.f, 58.f);
	// MotelFront.ts: the lot lamps stand 650 m south, in the court group's own transform.
	const FS47Obj Motel = Obj(TEXT("world.motel.group"));
	if (Motel)
	{
		// Six shader pools used to wash the entire facade. Each of the eight actual porch
		// diffusers now sends a small downward beam; rooms 3/4 and 7/8 share their old feed.
		constexpr int32 PorchSlots[8] = { 2, 3, 4, 4, 5, 6, 7, 7 };
		for (int32 I = 0; I < UE_ARRAY_COUNT(PorchSlots); ++I)
		{
			const double Z = 30 + I * 5;
			const FString Name = FString::Printf(TEXT("CourtPorch%02d"), I + 1);
			Add(*Name, TEXT("world.motel.group"), Court, PorchSlots[I], TEXT("ext.glow.points"), 26 + I,
				Motel.LocalToWorld(FVector(-40.86, 2.1475, Z)), Motel.LocalToWorld(FVector(-40.4, 0, Z)), 10.f, 4.f, 65.f);
		}
		for (int32 I = 0; I < 2; ++I)
		{
			const double Z = I == 0 ? 18 : 47;
			const FString Name = FString::Printf(TEXT("CourtLot%02d"), I + 1);
			Add(*Name, TEXT("world.motel.group"), Court, 10 + I, TEXT("ext.glow.points"), 34 + I,
				Motel.LocalToWorld(FVector(-29.55, 6.235, Z)), Motel.LocalToWorld(FVector(-32, 0, Z)), 150.f, 12.f, 58.f);
		}
		// The office doorway's old spill pool has no fixture. The indoor lamp and the real
		// window source already transmit its light; never leave another floating source here.
		const FLinearColor Spill = FloodPos(Court, 1);
		const FVector OldAt(Spill.R, Spill.G, Spill.B);
		for (int32 S = 0; S < FloodSets.Num(); ++S) for (int32 I = 0; I < FloodSets[S].Pos.Num(); ++I)
		{
			if (FloodSets[S].Key == TEXT("site") && I < 5) continue;
			const FLinearColor F = FloodSets[S].Pos[I];
			if (FVector(F.R, F.G, F.B).Equals(OldAt, 0.12)) SuppressFloodLight(S, I);
		}
	}
	// Only the four exported lamp-bulb faces use this material in the yard. Resolve their
	// material id through Nodes.Mats and then their named GLB slot; never assume slot zero.
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
	const int32 YardRoot = Handle(TEXT("yard.group"));
	const int32 BulbMaterial = HandleMat(TEXT("world.site.m.lampBulb"));
	for (int32 NodeId = 0; Base && NodeId < Nodes.Num(); ++NodeId)
	{
		const FS47WebNode& N = Nodes[NodeId];
		UStaticMeshComponent* M = N.Root == YardRoot ? Cast<UStaticMeshComponent>(N.Comp.Get()) : nullptr;
		if (!M) continue;
		for (int32 MatSlot = 0; MatSlot < N.Mats.Num(); ++MatSlot)
		{
			const int32 MaterialId = N.Mats[MatSlot];
			FLinearColor ExistingColor;
			UMaterialInterface* Existing = Mat(MaterialId);
			// The older export's site handle addresses a separate copy of the same bulb
			// material. Match the actual yard diffuser colour when its material id differs.
			const bool bBulb = MaterialId == BulbMaterial || (Existing && Existing->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Color")), ExistingColor) && ExistingColor.Equals(S47Three::Hex(0xffd9a0), 0.01f));
			if (!bBulb) continue;
			const int32 MeshSlot = M->GetMaterialIndex(FName(*FString::Printf(TEXT("slot%d"), MatSlot)));
			if (MeshSlot < 0 && N.Mats.Num() != 1) continue;
			UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Base, this);
			D->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.35f, 0.35f, 0.33f));
			D->SetScalarParameterValue(TEXT("Roughness"), 0.5f);
			if (N.Mats.Num() == 1)
			{
				// Single-material Box GLBs may retain six placeholder primitives.
				for (int32 K = 0; K < M->GetNumMaterials(); ++K) M->SetMaterial(K, D);
			}
			else M->SetMaterial(MeshSlot, D);
			KeepMids.Add(D);
			for (FExteriorLight& P : ExteriorLights) if (P.Set == Site && P.Slot >= 12 && P.Slot <= 15) { P.Diffuser = D; P.Material = N.Mats[MatSlot]; }
		}
	}
}

void AS47Web::UpdateExteriorLighting()
{
	const bool bLog = FParse::Param(FCommandLine::Get(), TEXT("S47LightLog"));
	const FVector Camera = S47Three::ToThree(Eye);
	const int32 Court = FloodSet(TEXT("court"));
	TMap<UMaterialInstanceDynamic*, float> Emission;
	for (FExteriorLight& P : ExteriorLights)
	{
		USpotLightComponent* L = P.Light.Get();
		if (!L) continue;
		float K = FMath::Max(0.f, FloodPos(P.Set, P.Slot).A * FloodScaleOf(P.Set) / P.Reference);
		if (!ObjAt(P.Node).Shown()) K = 0;
		if (FS47GlowPoints* G = Glow(P.GlowNode); G && G->Pos.IsValidIndex(P.Glow) && G->Col.IsValidIndex(P.Glow))
		{
			const FLinearColor C = G->Col[P.Glow];
			const float GlowK = FMath::Max3(C.R, C.G, C.B) / P.GlowReference;
			if (NodeWorldThree(G->Node).TransformPosition(G->Pos[P.Glow]).Y < -900) K = 0;
			if (P.Set == Court && P.Slot >= 2 && P.Slot <= 7)
			{
				// MotelFront's distant porch glows were static, even when room 6 went dark.
				// Drive them from the same feed; never read our output back as a power gate.
				const FLinearColor Want = S47Three::Hex(0xffc070) * K;
				if (!C.Equals(Want, 0.0001f)) G->SetColor(P.Glow, Want);
			}
			else
			{
				// Pool and glow carry the same SARO power ramp. Taking their minimum preserves
				// that ramp once, while multiplying would dim the fixture twice.
				K = FMath::Min(K, FMath::Max(0.f, GlowK));
			}
		}
		if (UMaterialInstanceDynamic* D = P.Diffuser.Get()) Emission.FindOrAdd(D) = FMath::Max(Emission.FindRef(D), K);
		const bool bOn = K > 0.0001f && FVector::DistSquared(Camera, P.At) < FMath::Square(200.f);
		const bool bToggle = P.bWasOn != bOn;
		if (bToggle) L->SetVisibility(bOn);
		const FLinearColor C = FloodCol(P.Set, P.Slot);
		const float Gain = FMath::Max(1.f, FMath::Max3(C.R, C.G, C.B));
		// Unreal's setters ignore unchanged values; colour-only HDR changes still update power.
		L->SetIntensity(bOn ? P.Candela * K * Gain : 0.f);
		L->SetLightColor(C / Gain);
		if (bLog && bToggle) UE_LOG(LogTemp, Display, TEXT("S47: exterior %s %s k=%.4f cd=%.2f"), *L->GetName(), bOn ? TEXT("on") : TEXT("off"), K, L->Intensity);
		P.LastK = K; P.bWasOn = bOn;
	}
	for (const auto& Pair : Emission)
	{
		const FLinearColor Want = S47Three::Hex(0xffd9a0) * (1.5f * Pair.Value);
		if (!Pair.Key->K2_GetVectorParameterValue(TEXT("Emissive")).Equals(Want, 0.0001f)) Pair.Key->SetVectorParameterValue(TEXT("Emissive"), Want);
		const float Glass = 0.02f + 0.33f * FMath::Clamp(Pair.Value, 0.f, 1.f);
		const FLinearColor Surface(Glass, Glass, Glass * 0.94f);
		if (!Pair.Key->K2_GetVectorParameterValue(TEXT("Color")).Equals(Surface, 0.0001f)) Pair.Key->SetVectorParameterValue(TEXT("Color"), Surface);
	}
}

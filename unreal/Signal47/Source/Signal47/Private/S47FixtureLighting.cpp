#include "S47Web.h"
#include "S47Main.h"
#include "S47RoomLights.h"

#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "S47Three.h"

URectLightComponent* AS47Web::SurfaceLight(int32 NodeId, const FVector& FacingThree, float Lumens, float Reach)
{
	const FS47WebNode* N = Node(NodeId);
	const UStaticMeshComponent* M = N ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
	if (!M || !M->GetStaticMesh()) return nullptr;
	const FBox B = M->GetStaticMesh()->GetBoundingBox();
	const FTransform& T = M->GetComponentTransform();
	const FVector Ext = B.GetExtent();
	const FVector Facing = S47Three::DirToUE(FacingThree).GetSafeNormal();
	const FVector Axis[3] = { FVector::XAxisVector, FVector::YAxisVector, FVector::ZAxisVector };
	int32 Thin = 0;
	for (int32 I = 1; I < 3; ++I)
		if (FMath::Abs(FVector::DotProduct(T.TransformVectorNoScale(Axis[I]), Facing)) > FMath::Abs(FVector::DotProduct(T.TransformVectorNoScale(Axis[Thin]), Facing))) Thin = I;
	const int32 A = (Thin + 1) % 3, C = (Thin + 2) % 3;
	FVector Across = T.TransformVector(Axis[A] * Ext[A] * 2), Up = T.TransformVector(Axis[C] * Ext[C] * 2);
	if (Across.SizeSquared() < Up.SizeSquared()) Swap(Across, Up);
	if (Across.SizeSquared() < 0.01 || Up.SizeSquared() < 0.01) return nullptr;
	const float Depth = float(T.TransformVector(Axis[Thin] * Ext[Thin]).Size());
	const FVector Centre = T.TransformPosition(B.GetCenter()) + Facing * (Depth + 1.f);
	URectLightComponent* L = NewObject<URectLightComponent>(this);
	L->SetupAttachment(GetRootComponent());
	L->SetMobility(EComponentMobility::Movable);
	L->SetWorldLocation(Centre);
	L->SetWorldRotation(FRotationMatrix::MakeFromXY(Facing, Across.GetSafeNormal()).Rotator());
	L->SetSourceWidth(float(Across.Size())); L->SetSourceHeight(float(Up.Size()));
	L->SetIntensityUnits(ELightUnits::Lumens);
	L->SetIntensity(Lumens);
	L->SetAttenuationRadius(Reach * 100.f);
	L->SetBarnDoorAngle(70.f); L->SetBarnDoorLength(8.f);
	L->SetCastShadows(true); L->SetCastVolumetricShadow(true);
	// Centimetre-scale props need a short contact trace rather than a metre-wide dark halo.
	L->ContactShadowLengthInWS = true; L->ContactShadowLength = 12.f;
	L->ContactShadowCastingIntensity = 0.8f;
	L->SetShadowBias(0.35f); L->SetShadowSlopeBias(0.4f);
	// Fast screen/tube flicker must not leave a temporal ghost in the fog volume.
	L->SetVolumetricScatteringIntensity(0.f);
	L->SetIndirectLightingIntensity(1.f);
	L->RegisterComponent();
	UE_LOG(LogTemp, Display, TEXT("S47: surface light node=%d at=%s normal=%s size=%.2fx%.2fcm lm=%.2f reach=%.2fm"), NodeId, *S47Three::ToThree(Centre).ToString(), *FacingThree.ToString(), L->SourceWidth, L->SourceHeight, Lumens, Reach);
	return L;
}

void AS47Web::BuildFixtureLighting()
{
	// ControlRoom.ts's PointLights were broad fills between fittings, never physical sources.
	// Keep their components as the story's intensity/visibility state, without a scene proxy.
	for (const auto& P : CeilingLights) if (P.IsValid()) { P->bAffectsWorld = false; P->MarkRenderStateDirty(); }
	for (const auto& P : LampLights) if (P.IsValid()) { P->bAffectsWorld = false; P->MarkRenderStateDirty(); }
	if (CrtLight.IsValid()) { CrtLight->bAffectsWorld = false; CrtLight->MarkRenderStateDirty(); }
	UMaterialInterface* DiffuserBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
	auto Tube = [&](int32 Id, int32 TubeId, int32 Set, int32 Slot)
	{
		FFixtureLight P;
		P.Node = Id; P.Tube = TubeId; P.Set = Set; P.Slot = Slot;
		P.Lumens = Set >= 0 ? 32.f : 36.f;
		P.Color = S47Three::Hex(0xeef3ff);
		P.Light = SurfaceLight(Id, FVector(0, -1, 0), P.Lumens, 6.f);
		if (!P.Light.IsValid()) return;
		P.Light->SetLightColor(P.Color);
		if (Set >= 0) { P.Reference = FloodPos(Set, Slot).A * FloodScaleOf(Set); SuppressFloodLight(Set, Slot); }
		else P.Driver = WorldThree(Id).Z < 0 ? 0 : 1;
		if (UStaticMeshComponent* M = Cast<UStaticMeshComponent>(Nodes[Id].Comp.Get()); M && DiffuserBase)
		{
			UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(DiffuserBase, this);
			D->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.35f, 0.35f, 0.33f));
			D->SetScalarParameterValue(TEXT("Roughness"), 0.5f);
			D->SetVectorParameterValue(TEXT("Emissive"), P.Color);
			// Box GLBs retain slot0..5 even when the exported node has one material.
			for (int32 K = 0; K < M->GetNumMaterials(); ++K) M->SetMaterial(K, D);
			KeepMids.Add(D); P.Diffuser = D;
		}
		FixtureLights.Add(P);
	};
	for (int32 I = 0; I < TubeNodes.Num(); ++I) Tube(TubeNodes[I], I, -1, -1);
	const int32 Annex = FloodSet(TEXT("annex")), TubeMat = HandleMat(TEXT("annex.tubeOn"));
	for (int32 I = 0; I < Nodes.Num(); ++I) if (TubeMat >= 0 && Nodes[I].Mats.Contains(TubeMat))
	{
		int32 Slot = -1; double Near = 1;
		const FVector At = WorldThree(I);
		for (int32 K = 0; K < FloodSlots(Annex); ++K)
		{
			const FLinearColor F = FloodPos(Annex, K);
			const double D = FVector::DistSquared(At, FVector(F.R, F.G, F.B));
			if (D < Near) { Slot = K; Near = D; }
		}
		// Annex.ts's tired mesh and exported flicker slot refer to opposite records fittings.
		// Apply the flicker state at the actual tired tube, leaving the other fitting steady.
		if (I == Handle(TEXT("annex.flickerTube"))) Slot = 4;
		else if (Slot == 4) Slot = 3;
		if (Slot >= 0) Tube(I, -1, Annex, Slot);
	}
	for (int32 Driver = 0; Driver < LampLights.Num(); ++Driver)
	{
		if (!LampLights[Driver].IsValid()) continue;
		int32 Bulb = -1; double Near = 60 * 60;
		for (int32 I = 0; I < Nodes.Num(); ++I)
		{
			const UStaticMeshComponent* M = Cast<UStaticMeshComponent>(Nodes[I].Comp.Get());
			if (!M || !M->GetStaticMesh() || Nodes[I].Root != Handle(TEXT("room.group")) || M->Bounds.BoxExtent.GetMax() > 5.f) continue;
			FLinearColor Color;
			if (!M->GetMaterial(0) || !M->GetMaterial(0)->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Color")), Color) || !Color.Equals(S47Three::Hex(0xffd9a0), 0.01f)) continue;
			const double D = FVector::DistSquared(M->Bounds.Origin, LampLights[Driver]->GetComponentLocation());
			if (D < Near) { Bulb = I; Near = D; }
		}
		if (Bulb < 0) continue;
		FFixtureLight P;
		P.Node = Bulb; P.Driver = Driver; P.bDesk = true; P.Lumens = Driver == 0 ? 18.f : 14.f;
		P.Color = LampLights[Driver]->GetLightColor();
		// The merged shades also contain the base. Fit the source to the separate bulb and
		// the shade's open mouth (ControlRoom.ts: shade rotation x=.55 / z=.6, diameter .22).
		const FVector Facing = Driver == 0 ? FVector(0, -FMath::Cos(0.55), -FMath::Sin(0.55)) : FVector(FMath::Sin(0.6), -FMath::Cos(0.6), 0);
		P.Light = SurfaceLight(Bulb, Facing, P.Lumens, 3.f);
		if (!P.Light.IsValid()) continue;
		P.Light->SetSourceWidth(20.f); P.Light->SetSourceHeight(20.f);
		P.Light->SetBarnDoorAngle(55.f); P.Light->SetBarnDoorLength(12.f);
		P.Light->SetLightColor(P.Color);
		if (UStaticMeshComponent* M = Cast<UStaticMeshComponent>(Nodes[Bulb].Comp.Get()); M && DiffuserBase)
		{
			UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(DiffuserBase, this);
			D->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.35f, 0.35f, 0.33f));
			// Box GLBs retain slot0..5 even when the exported node has one material.
			for (int32 K = 0; K < M->GetNumMaterials(); ++K) M->SetMaterial(K, D);
			KeepMids.Add(D); P.Diffuser = D;
		}
		FixtureLights.Add(P);
	}
	// Annex.ts's green desk shade is a separate mesh, unlike the merged lab shade.
	for (int32 I = 0; I < Nodes.Num(); ++I)
	{
		const UStaticMeshComponent* M = Cast<UStaticMeshComponent>(Nodes[I].Comp.Get());
		if (!M || !M->GetStaticMesh() || !S47Three::ToThree(M->Bounds.Origin).Equals(FVector(-3.07, 1.155, 8.35), 0.01)) continue;
		FFixtureLight P;
		P.Node = I; P.Set = Annex; P.Slot = 5; P.Lumens = 28.f;
		P.Reference = FloodPos(Annex, 5).A * FloodScaleOf(Annex); P.Color = FloodCol(Annex, 5);
		P.Light = SurfaceLight(I, FVector(0, -1, 0), P.Lumens, 3.f);
		if (!P.Light.IsValid()) continue;
		P.Light->SetLightColor(P.Color); P.Light->SetBarnDoorAngle(60.f);
		FixtureLights.Add(P); SuppressFloodLight(Annex, 5);
	}
	// Unlike SARO's separate glass meshes, these fittings are merged into room geometry.
	// Fit their native emitter to the original opening, never to the merged room's bounds.
	auto Fixed = [&](const TCHAR* Name, const TCHAR* OwnerPath, int32 Set, int32 Slot,
		const FVector& At, const FVector& Facing, const FVector& Across, float Width, float Height, float Lumens, float Reach)
	{
		const FS47Obj Owner = Obj(OwnerPath);
		if (!Owner || Set < 0 || Slot < 0 || Slot >= FloodSlots(Set)) return;
		FFixtureLight P;
		P.Node = Owner.I; P.Set = Set; P.Slot = Slot; P.Lumens = Lumens;
		P.Reference = FMath::Max(0.001f, FloodPos(Set, Slot).A * FloodScaleOf(Set));
		P.Color = FloodCol(Set, Slot);
		URectLightComponent* L = NewObject<URectLightComponent>(this, FName(*FString::Printf(TEXT("S47Fixture_%s"), Name)));
		L->SetupAttachment(GetRootComponent()); L->SetMobility(EComponentMobility::Movable);
		L->SetWorldLocation(S47Three::ToUE(At));
		L->SetWorldRotation(FRotationMatrix::MakeFromXY(S47Three::DirToUE(Facing), S47Three::DirToUE(Across)).Rotator());
		L->SetSourceWidth(Width); L->SetSourceHeight(Height);
		L->SetIntensityUnits(ELightUnits::Lumens); L->SetIntensity(0.f);
		L->SetLightColor(P.Color); L->SetAttenuationRadius(Reach * 100.f);
		L->SetBarnDoorAngle(68.f); L->SetBarnDoorLength(10.f);
		L->SetCastShadows(true); L->SetCastVolumetricShadow(true);
		L->ContactShadowLengthInWS = true; L->ContactShadowLength = 12.f;
		L->ContactShadowCastingIntensity = 0.8f;
		L->SetShadowBias(0.35f); L->SetShadowSlopeBias(0.4f);
		L->SetVolumetricScatteringIntensity(0.f); L->SetIndirectLightingIntensity(1.f);
		L->SetVisibility(false); L->RegisterComponent();
		P.Light = L; FixtureLights.Add(P); SuppressFloodLight(Set, Slot);
		UE_LOG(LogTemp, Display, TEXT("S47: fixed fixture %s source=%s size=%.1fx%.1fcm lm=%.1f reach=%.1fm"), Name, *At.ToString(), Width, Height, Lumens, Reach);
	};
	const int32 Hut = FloodSet(TEXT("hut")), Diner = FloodSet(TEXT("diner"));
	const FS47Obj Station = Obj(TEXT("world.site.group"));
	if (Station)
	{
		// Station01.ts: green bench shade and the 32 cm tin ceiling shade, both aimed down.
		Fixed(TEXT("HutDesk"), TEXT("world.site.group"), Hut, 1, Station.LocalToWorld(FVector(-10.60, 1.075, 1.89)),
			FVector(FMath::Sin(0.5), -FMath::Cos(0.5), 0), FVector(0, 0, 1), 12.f, 12.f, 16.f, 2.7f);
		Fixed(TEXT("HutCeiling"), TEXT("world.site.group"), Hut, 2, Station.LocalToWorld(FVector(-11, 2.105, 3)),
			FVector(0, -1, 0), FVector(1, 0, 0), 22.f, 22.f, 28.f, 4.5f);
		// The exterior bulb now reaches through the actual door. Its old interior spill is
		// not another lamp and must not illuminate a shut door from inside the hut.
		SuppressFloodLight(Hut, 3);
	}
	const FS47Obj DinerGroup = Obj(TEXT("world.diner.group"));
	if (DinerGroup)
	{
		for (int32 I = 0; I < 3; ++I)
		{
			const FString Name = FString::Printf(TEXT("DinerTube%02d"), I + 1);
			Fixed(*Name, TEXT("world.diner.group"), Diner, 2 + I, DinerGroup.LocalToWorld(FVector(-2, 3.015, -5 + I * 5)),
				FVector(0, -1, 0), FVector(0, 0, 1), 250.f, 38.f, 40.f, 5.5f);
		}
		// The pass's narrow amber strip faces the counter. It gives the chrome a long
		// reflection and throws the cups' shadows onto the worktop instead of filling the kitchen.
		Fixed(TEXT("DinerHatch"), TEXT("world.diner.group"), Diner, 5, DinerGroup.LocalToWorld(FVector(-5.395, 2.205, -2.4)),
			FVector(0.65, -0.76, 0), FVector(0, 0, 1), 180.f, 4.5f, 22.f, 3.5f);
		// Slot 8 was an imaginary point lamp representing dawn. The real sky/sun own dawn.
		SuppressFloodLight(Diner, 8);
	}
	UpdateFixtureLighting();
	UE_LOG(LogTemp, Display, TEXT("S47: fixture lighting %d sources fitted to exported diffusers/shades"), FixtureLights.Num());
}

void AS47Web::UpdateFixtureLighting()
{
	const US47Main* Main = US47Main::Get(this);
	const FS47RoomLights* Rooms = Main && Main->RoomLights ? Main->RoomLights.Get() : nullptr;
	const int32 Annex = FloodSet(TEXT("annex"));
	for (FFixtureLight& P : FixtureLights)
	{
		URectLightComponent* L = P.Light.Get();
		if (!L) continue;
		float K = ObjAt(P.Node).Shown() ? 1.f : 0.f;
		if (P.Set >= 0) K *= FMath::Max(0.f, FloodPos(P.Set, P.Slot).A * FloodScaleOf(P.Set) / FMath::Max(0.001f, P.Reference));
		else
		{
			const auto& Drivers = P.bDesk ? LampLights : CeilingLights;
			const TArray<float>& Base = P.bDesk ? LampBase : CeilingBase;
			K *= Drivers.IsValidIndex(P.Driver) && Drivers[P.Driver].IsValid() && Drivers[P.Driver]->IsVisible() ? Drivers[P.Driver]->Intensity / FMath::Max(0.001f, Base[P.Driver]) : 0.f;
			if (P.Tube >= 0 && !TubeLit(P.Tube)) K = 0;
		}
		// Mask only the finished ceiling output. The upstream drivers still represent
		// supply/flicker for CRTs, instruments and Story's power snapshots.
		if (Rooms)
		{
			const TCHAR* Room = nullptr;
			if (P.Set < 0 && !P.bDesk && P.Tube >= 0) Room = TEXT("control");
			else if (P.Set == Annex && (P.Slot == 0 || P.Slot == 1)) Room = TEXT("corridor");
			else if (P.Set == Annex && (P.Slot == 3 || P.Slot == 4)) Room = TEXT("records");
			if (Room)
			{
				L->ComponentTags.AddUnique(FName(*(FString(TEXT("S47RoomCeiling:")) + Room)));
				if (!Rooms->RoomEnabled(Room)) K = 0.f;
			}
		}
		L->SetVisibility(K > 0.0001f);
		// Obj.SetColor can replace a diffuser with its private MID during a story flicker.
		// Follow the material actually on the mesh, including a replacement at unchanged power.
		bool bMaterialChanged = false;
		if (P.Diffuser.IsValid()) if (UStaticMeshComponent* M = Cast<UStaticMeshComponent>(Nodes[P.Node].Comp.Get()))
			if (UMaterialInstanceDynamic* D = Cast<UMaterialInstanceDynamic>(M->GetMaterial(0)))
			{
				bMaterialChanged = D != P.Diffuser.Get(); P.Diffuser = D;
				// NodeMids may split the box's six faces into private copies. This is one
				// uniform diffuser: keep every face, including underside slot3, on the live MID.
				for (int32 I = 1; I < M->GetNumMaterials(); ++I) if (M->GetMaterial(I) != D)
				{ M->SetMaterial(I, D); bMaterialChanged = true; }
			}
		if (bMaterialChanged || !FMath::IsNearlyEqual(K, P.LastK))
		{
			L->SetIntensity(P.Lumens * K);
			if (UMaterialInstanceDynamic* D = P.Diffuser.Get())
			{
				D->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.35f, 0.35f, 0.33f));
				D->SetVectorParameterValue(TEXT("Emissive"), P.Color * (K * 1.4f));
			}
			// An authored housing can replace the old visible mesh while this exact
			// exported node/slot continues to own power, dimming and flicker.
			if (UMaterialInstanceDynamic* D = P.NativeDiffuser.Get())
				D->SetVectorParameterValue(TEXT("EmissiveFactor"), P.Color * (K * 1.4f));
			P.LastK = K;
			if (FParse::Param(FCommandLine::Get(), TEXT("S47LightLog"))) UE_LOG(LogTemp, Display, TEXT("S47: fixture node=%d k=%.4f lm=%.2f"), P.Node, K, L->Intensity);
		}
	}
}

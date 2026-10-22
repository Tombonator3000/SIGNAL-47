#include "S47Web.h"

#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47ConsoleKit.h"
#include "S47Json.h"
#include "S47Three.h"
#include "S47Yard.h"

namespace
{
	float ScreenLevel(const UStaticMeshComponent* Mesh)
	{
		// Read the effective material: story code can replace it with a private MID later.
		const UMaterialInterface* Material = Mesh ? Mesh->GetMaterial(0) : nullptr;
		FLinearColor C = FLinearColor::White;
		float Brightness = 1.f;
		if (Material)
		{
			Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Color")), C);
			Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Brightness")), Brightness);
		}
		return FMath::Max(0.f, FMath::Max3(C.R, C.G, C.B) * Brightness);
	}

	FVector ScreenFacing(const UStaticMeshComponent* Mesh)
	{
		// Exported screen planes face local +Z in three; the importer makes that Unreal +Y.
		const FVector U = Mesh->GetComponentTransform().TransformVectorNoScale(FVector::YAxisVector);
		return FVector(U.X, U.Z, U.Y).GetSafeNormal();
	}
}

void AS47Web::BuildScreenLighting()
{
	for (FScreenLight& S : ScreenLights) if (URectLightComponent* L = S.Light.Get()) L->DestroyComponent();
	ScreenLights.Reset();
	auto Add = [this](int32 Node, int32 Screen, float Lumens, float Reach)
	{
		UStaticMeshComponent* M = Nodes.IsValidIndex(Node) ? Cast<UStaticMeshComponent>(Nodes[Node].Comp.Get()) : nullptr;
		if (!M || !M->GetStaticMesh()) return false;
		if (URectLightComponent* L = SurfaceLight(Node, ScreenFacing(M), Lumens, Reach))
		{
			L->SetLightColor(S47Three::Hex(0x8cffa4));
			L->SetVisibility(false);
			ScreenLights.Add({ Node, Screen, Lumens, Screen >= 0 ? 1.f : FMath::Max(0.001f, ScreenLevel(M)), L });
			return true;
		}
		return false;
	};
	// Four canvases fitted to the actual monitor glass, including the rotated west-desk terminal.
	for (int32 Screen = 0; Screen < 4; ++Screen)
	{
		if (CrtMat[Screen] < 0) continue;
		for (const FS47WebNode& N : Nodes) if (N.Mats.Contains(CrtMat[Screen]))
		{
			const int32 Node = int32(&N - Nodes.GetData());
			Add(Node, Screen, 20.f, 1.8f);
			break;
		}
	}
	const int32 BusMat = HandleMat(TEXT("yard.screens[0]"));
	TSet<int32> AnnexScreens;
	for (const auto& H : HandleMats) if (H.Key.StartsWith(TEXT("annex.screens[")) && H.Value >= 0) AnnexScreens.Add(H.Value);
	const int32 DspTex = HandleTex(TEXT("room.dspScreen.tex"));
	int32 DspMat = -1;
	if (DspTex >= 0) for (const TSharedPtr<FJsonValue>& V : US47Yard::ManifestSection(TEXT("materials")))
	{
		const S47J::FObj M = V->AsObject();
		if (S47J::Int(M, TEXT("map"), -1) == DspTex) { DspMat = S47J::Int(M, TEXT("id"), -1); break; }
	}
	for (int32 Node = 0; Node < Nodes.Num(); ++Node)
	{
		if (BusMat >= 0 && Nodes[Node].Mats.Contains(BusMat)) Add(Node, -1, 12.f, 1.4f);
		if (Nodes[Node].Mats.ContainsByPredicate([&AnnexScreens](int32 Id) { return AnnexScreens.Contains(Id); }))
		{
			// Annex.ts: the vending face owns annex slot 2; its real pane replaces that point pool.
			if (Add(Node, -1, 12.f, 1.4f)) SuppressFloodLight(FloodSet(TEXT("annex")), 2);
		}
		// The rack display is only a fallback: ConsoleKit hides the whole original deck.
		if (DspMat >= 0 && Nodes[Node].Mats.Contains(DspMat)) Add(Node, -3, 3.f, 1.f);
	}
	// The room's old CRT point light is a state driver only. The other terminal planes have no
	// exported pool of their own; the annex's backlit face is the sole replacement above.
}

void AS47Web::UpdateScreenLighting()
{
	// ConsoleKit creates its display after AS47Web::BeginPlay. Adopt that existing picture plane
	// once it appears, keeping its component and real attachment; the amber VU faces are separate.
	if (!ScreenLights.ContainsByPredicate([this](const FScreenLight& S) { return S.Screen == -2 && S.Light.IsValid() && Nodes.IsValidIndex(S.Node) && Nodes[S.Node].Comp.IsValid(); }))
	{
		const int32 Kit = FS47ConsoleKit::Find(this);
		USceneComponent* Root = Nodes.IsValidIndex(Kit) ? Nodes[Kit].Comp.Get() : nullptr;
		if (Root)
		{
			// S47ConsoleKit.cpp: ANALYZER + DISPLAY + 2 * LIFT, the 256 x 72 picture over its glass.
			const FVector At = Root->GetComponentTransform().TransformPosition(S47Three::ToUE(2.8918, 0.8833, -3.8506));
			TArray<USceneComponent*> Children;
			Root->GetChildrenComponents(true, Children);
			for (USceneComponent* C : Children)
			{
				UStaticMeshComponent* M = Cast<UStaticMeshComponent>(C);
				UMaterialInstanceDynamic* Mid = M ? Cast<UMaterialInstanceDynamic>(M->GetMaterial(0)) : nullptr;
				if (!M || !M->GetStaticMesh() || !Mid || !M->IsVisible() || Mid->K2_GetScalarParameterValue(TEXT("UseMap")) < 0.5f) continue;
				const FVector Centre = M->GetComponentTransform().TransformPosition(M->GetStaticMesh()->GetBoundingBox().GetCenter());
				if (!Centre.Equals(At, 0.05f)) continue;
				int32 Node = NodeOf(M);
				if (Node < 0) Node = AdoptNode(M, Kit, TEXT("s47:processorScreen"));
				if (URectLightComponent* L = SurfaceLight(Node, FVector(0, 0, 1), 3.f, 1.f))
				{
					L->SetLightColor(S47Three::Hex(0x8cffa4));
					L->SetVisibility(false);
					ScreenLights.Add({ Node, -2, 3.f, 1.f, L });
				}
				break;
			}
		}
	}
	// Station power is independent of RX bank 3. Read the original ceiling drivers, never a
	// brightness value already multiplied on the previous frame.
	float Power = 0.f;
	bool bPowerDriver = false;
	for (int32 I = 0; I < CeilingLights.Num() && I < CeilingBase.Num(); ++I)
		if (CeilingLights[I].IsValid() && CeilingBase[I] > 0.001f)
		{
			Power = FMath::Max(Power, CeilingLights[I]->Intensity / CeilingBase[I]);
			bPowerDriver = true;
		}
	if (!bPowerDriver) Power = 1.f;
	for (FScreenLight& S : ScreenLights)
	{
		URectLightComponent* L = S.Light.Get();
		UStaticMeshComponent* M = Nodes.IsValidIndex(S.Node) ? Cast<UStaticMeshComponent>(Nodes[S.Node].Comp.Get()) : nullptr;
		if (!L || !M) continue;
		if (S.Screen >= 0 || S.Screen == -2 || S.Screen == -3)
		{
			// Gate actual phosphor as well as its light: an off canvas otherwise keeps a faint
			// unlit glow, and the HUD refreshes the ops terminal's own power independently.
			const float Brightness = Power * (S.Screen < 0 || IsCrtPowered(S.Screen) ? 1.f : 0.f);
			UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(M->GetMaterial(0));
			// Preserve the fallback display's shared map binding instead of making a private copy.
			if (!Mid && Nodes[S.Node].Mats.Num() > 0) Mid = SharedMid(Nodes[S.Node].Mats[0]);
			if (Mid) Mid->SetScalarParameterValue(TEXT("Brightness"), Brightness);
			if (S.Screen == -2)
			{
				// The new processor picture and its dark glass are separate planes 1.2 mm apart.
				// Darken both; the amber meter faces elsewhere under the group keep their own hue.
				USceneComponent* Holder = M->GetAttachParent();
				USceneComponent* Group = Holder ? Holder->GetAttachParent() : nullptr;
				TArray<USceneComponent*> Parts;
				if (Group) Group->GetChildrenComponents(true, Parts);
				for (USceneComponent* C : Parts) if (UStaticMeshComponent* Pane = Cast<UStaticMeshComponent>(C))
					if (Pane->GetComponentLocation().Equals(M->GetComponentLocation(), 0.25f))
						if (UMaterialInstanceDynamic* Glass = Cast<UMaterialInstanceDynamic>(Pane->GetMaterial(0))) Glass->SetScalarParameterValue(TEXT("Brightness"), Power);
			}
		}
		float K = ScreenLevel(M) / FMath::Max(0.001f, S.Reference);
		if (S.Screen >= 0)
		{
			K *= IsCrtPowered(S.Screen) ? CrtFlicker[S.Screen] : 0.f;
			// Both station and RX drivers contain the station fade. Brightness already contains
			// Power, so use their minimum rather than applying that common fade twice.
			if (S.Screen < 3 && CrtLight.IsValid())
			{
				const float Glow = FMath::Max(0.f, Light(TEXT("room.lights.crt")).Intensity()) / FMath::Max(0.001f, CrtLightBase);
				K *= FMath::Min(1.f, Glow / FMath::Max(0.001f, Power));
			}
		}
		const bool bShown = ObjAt(S.Node).Shown() && M->IsVisible() && !M->bHiddenInGame;
		const float Lumens = bShown && K > 0.02f ? S.Base * K : 0.f;
		L->SetIntensity(Lumens);
		if (L->IsVisible() != (Lumens > 0.f)) L->SetVisibility(Lumens > 0.f);
	}
}

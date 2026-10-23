#include "S47Web.h"
#include "String/LexFromString.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "S47Json.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47World.h"
#include "S47Yard.h"
#include "UnrealClient.h"

namespace
{
	constexpr float IndicatorGain = 1.8f;

	bool Below(const AS47Web& W, int32 Node, int32 Ancestor)
	{
		if (Ancestor < 0) return false;
		for (const FS47WebNode* N = W.Node(Node); N; N = W.Node(N->Parent))
			if (N == W.Node(Ancestor)) return true;
		return false;
	}

	float Brightness(const UMaterialInterface* Material)
	{
		float Value = 1.f;
		if (Material) Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Brightness")), Value);
		return Value;
	}

	FLinearColor RackColor(int32 Index, bool bOn)
	{
		// ControlRoom.ts LED_COL / ledState: positions 7 and 15 are amber.
		// Off is zero emission, not the web version's faintly luminous dark paint.
		return bOn ? S47Three::Hex(Index == 7 || Index == 15 ? 0xffb040 : 0x45ff7a) * IndicatorGain : FLinearColor::Black;
	}

	bool NativeIndicatorSlot(const FString& NodeName, const UMaterialInterface* Material)
	{
		// Exact authored emitting surfaces: never replace a prop's PBR body or glass.
		if (!Material) return false;
		const FString Path = Material->GetPathName();
		return (NodeName == TEXT("prop:printer") && Path.Contains(TEXT("/dotmatrix_printer/Materials/M_PrinterLED.")))
			|| (NodeName == TEXT("prop:cbBase") && (Path.Contains(TEXT("/cb_base/Materials/Guage_1_Sclae."))
				|| Path.Contains(TEXT("/cb_base/Materials/Guage_2_Sclae.")) || Path.Contains(TEXT("/cb_base/Materials/Light."))))
			|| (NodeName == TEXT("prop:terminal") && Path.Contains(TEXT("/terminal_3278/Materials/display.")));
	}
}

float AS47Web::SaroLightingPower() const
{
	float Power = 0.f;
	bool bDriver = false;
	for (int32 I = 0; I < CeilingLights.Num() && I < CeilingBase.Num(); ++I)
		if (CeilingLights[I].IsValid() && CeilingBase[I] > 0.001f)
		{
			Power = FMath::Max(Power, CeilingLights[I]->Intensity / CeilingBase[I]);
			bDriver = true;
		}
	return bDriver ? FMath::Clamp(Power, 0.f, 1.f) : 1.f;
}

void AS47Web::BuildIndicatorLighting()
{
	IndicatorSurfaces.Reset();
	NativeIndicatorSurfaces.Reset();
	NativeIndicatorNodeCount = -1;
	RackLedOn.Init(false, LedCount());
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Unlit_Opaque.M_Web_Unlit_Opaque"));
	if (!Base)
	{
		UE_LOG(LogTemp, Error, TEXT("S47: indicator emissive master missing"));
		return;
	}
	TMap<int32, FLinearColor> Colors;
	TSet<int32> Panels;
	for (const TSharedPtr<FJsonValue>& V : US47Yard::ManifestSection(TEXT("materials")))
	{
		const S47J::FObj M = V->AsObject();
		const TArray<double> C = S47J::Nums(M, TEXT("color"));
		if (S47J::Str(M, TEXT("name")) == TEXT("console_kit") && S47J::Bool(M, TEXT("lit")) && S47J::Has(M, TEXT("emissiveMap")))
			Panels.Add(S47J::Int(M, TEXT("id"), -1));
		if (S47J::Bool(M, TEXT("lit")) || S47J::Bool(M, TEXT("shader")) || S47J::Has(M, TEXT("map")) || C.Num() != 3) continue;
		const FLinearColor Color{ float(C[0]), float(C[1]), float(C[2]) };
		for (uint32 Hex : { 0x45ff7au, 0xffb040u, 0xff3b2au })
			if (Color.Equals(S47Three::Hex(Hex), 0.0001f)) Colors.Add(S47J::Int(M, TEXT("id"), -1), Color);
	}
	const int32 Room = Handle(TEXT("room.group"));
	const int32 CrtOwners[] = { Handle(TEXT("room.objs.crtLeft")), Handle(TEXT("room.objs.crtCenter")),
		Handle(TEXT("room.objs.crtRight")), Handle(TEXT("room.objs.crtOps")) };
	for (int32 NodeId = 0; NodeId < Nodes.Num(); ++NodeId)
	{
		const FS47WebNode& N = Nodes[NodeId];
		UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(N.Comp.Get());
		if (!Mesh || !Mesh->GetStaticMesh() || N.Mats.Num() != 1 || !Below(*this, NodeId, Room)) continue;
		const bool bRack = NodeId == LedNode;
		const bool bPanel = Panels.Contains(N.Mats[0]);
		const FLinearColor* C = Colors.Find(N.Mats[0]);
		if (!bRack && !C && !bPanel) continue;
		UMaterialInstanceDynamic* Mid = nullptr;
		if (bPanel)
		{
			UMaterialInterface* Original = Mesh->GetMaterial(0);
			UMaterialInstanceDynamic* Source = Cast<UMaterialInstanceDynamic>(Original);
			Mid = UMaterialInstanceDynamic::Create(Source ? Source->Parent.Get() : Original, this);
			if (Source) Mid->CopyParameterOverrides(Source);
		}
		else
		{
			Mid = UMaterialInstanceDynamic::Create(Base, this);
			Mid->SetVectorParameterValue(TEXT("Color"), bRack ? FLinearColor::White : *C * IndicatorGain);
			Mid->SetScalarParameterValue(TEXT("UseInstanceColor"), bRack ? 1.f : 0.f);
			Mid->SetScalarParameterValue(TEXT("UseMap"), 0.f);
			Mid->SetScalarParameterValue(TEXT("Untonemapped"), 0.f);
		}
		if (!Mid) continue;
		// A BoxGeometry export has six GLB slots despite one source material. Its visible
		// west face is not slot0. Bind every slot of these single-material indicator meshes.
		for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot) Mesh->SetMaterial(Slot, Mid);
		NodeMidCache.Remove(NodeId);
		KeepMids.Add(Mid);
		FIndicatorSurface Surface;
		Surface.Node = NodeId; Surface.Mesh = Mesh; Surface.bFixed = true; Surface.bMapped = bPanel;
		for (int32 Screen = 0; Screen < 4; ++Screen) if (Below(*this, NodeId, CrtOwners[Screen])) Surface.Screen = Screen;
		IndicatorSurfaces.Add(MoveTemp(Surface));
		UE_LOG(LogTemp, Display, TEXT("S47: indicator node=%d slots=%d rack=%d screen=%d"), NodeId, Mesh->GetNumMaterials(), bRack, IndicatorSurfaces.Last().Screen);
	}
	for (int32 I = 0; I < RackLedOn.Num(); ++I) DrawRackIndicator(I);
	UpdateIndicatorLighting(SaroLightingPower());
}

void AS47Web::DrawRackIndicator(int32 Index)
{
	UInstancedStaticMeshComponent* Mesh = Nodes.IsValidIndex(LedNode) ? Cast<UInstancedStaticMeshComponent>(Nodes[LedNode].Comp.Get()) : nullptr;
	if (!Mesh || !RackLedOn.IsValidIndex(Index) || Index >= Mesh->GetInstanceCount() || Mesh->NumCustomDataFloats < 3) return;
	const FLinearColor C = RackColor(Index, RackLedOn[Index]);
	Mesh->SetCustomDataValue(Index, 0, C.R, false);
	Mesh->SetCustomDataValue(Index, 1, C.G, false);
	Mesh->SetCustomDataValue(Index, 2, C.B, true);
}

void AS47Web::GateIndicatorMesh(UStaticMeshComponent* Mesh, float Power)
{
	if (!Mesh) return;
	int32 Index = IndicatorSurfaces.IndexOfByPredicate([Mesh](const FIndicatorSurface& S) { return S.Mesh == Mesh; });
	if (Index == INDEX_NONE)
	{
		FIndicatorSurface Surface;
		Surface.Node = NodeOf(Mesh); Surface.Mesh = Mesh;
		Index = IndicatorSurfaces.Add(MoveTemp(Surface));
	}
	FIndicatorSurface& Surface = IndicatorSurfaces[Index];
	Surface.Slots.SetNum(Mesh->GetNumMaterials());
	// Several slots can share one MID. Read every input before writing any output,
	// otherwise slot1 mistakes slot0's fade for an authored change and keeps it after a clone.
	TArray<float, TInlineAllocator<6>> CurrentValues;
	TArray<float, TInlineAllocator<6>> EmissiveValues;
	TArray<bool, TInlineAllocator<6>> HasEmissive;
	for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot) CurrentValues.Add(Brightness(Mesh->GetMaterial(Slot)));
	for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
	{
		float Value = 1.f;
		const UMaterialInterface* Material = Mesh->GetMaterial(Slot);
		const bool bHasEmissive = Material && Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("EmissivePower")), Value);
		EmissiveValues.Add(Value);
		HasEmissive.Add(bHasEmissive);
	}
	for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
	{
		UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(Slot));
		if (!Mid) continue;
		FIndicatorBrightness& State = Surface.Slots[Slot];
		const float Current = CurrentValues[Slot];
		// NodeMids may clone this MID after a story change. A clone inherits Applied;
		// keep its authored value rather than treating our dimmed result as a new input.
		if (!FMath::IsNearlyEqual(Current, State.Applied, 0.00001f)) State.Authored = FMath::Max(0.f, Current);
		// SARO and a device can both carry the same fade. Their minimum applies it once.
		const float Applied = FMath::Min(State.Authored, FMath::Max(0.f, Power));
		if (!FMath::IsNearlyEqual(Current, Applied, 0.00001f)) Mid->SetScalarParameterValue(TEXT("Brightness"), Applied);
		State.Applied = Applied;
		if (HasEmissive[Slot])
		{
			if (!FMath::IsNearlyEqual(EmissiveValues[Slot], State.EmissiveApplied, 0.00001f)) State.EmissiveAuthored = FMath::Max(0.f, EmissiveValues[Slot]);
			const float EmissiveApplied = FMath::Min(State.EmissiveAuthored, FMath::Max(0.f, Power));
			if (!FMath::IsNearlyEqual(EmissiveValues[Slot], EmissiveApplied, 0.00001f)) Mid->SetScalarParameterValue(TEXT("EmissivePower"), EmissiveApplied);
			State.EmissiveApplied = EmissiveApplied;
		}
	}
}

void AS47Web::UpdateIndicatorLighting(float Power)
{
	// S47Props adopts these after BuildRoom. Discover again only when the node list grows;
	// retained source factors also survive a MID clone made while the station is dark.
	if (NativeIndicatorNodeCount != Nodes.Num())
	{
		for (int32 NodeId = 0; NodeId < Nodes.Num(); ++NodeId)
		{
			const FS47WebNode& N = Nodes[NodeId];
			if (N.Name != TEXT("prop:printer") && N.Name != TEXT("prop:cbBase") && N.Name != TEXT("prop:terminal")) continue;
			if (NativeIndicatorSurfaces.ContainsByPredicate([NodeId](const FNativeIndicatorSurface& S) { return S.Node == NodeId; })) continue;
			UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(N.Comp.Get());
			if (!Mesh) continue;
			for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
			{
				UMaterialInterface* Original = Mesh->GetMaterial(Slot);
				if (UMaterialInstanceDynamic* Dynamic = Cast<UMaterialInstanceDynamic>(Original)) Original = Dynamic->Parent.Get();
				FLinearColor Emissive;
				if (!NativeIndicatorSlot(N.Name, Original) || !Original->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveFactor")), Emissive)
					|| Emissive.GetMax() <= 0.f) continue;
				UMaterialInstanceDynamic* Mid = Mesh->CreateDynamicMaterialInstance(Slot);
				if (!Mid) continue;
				FNativeIndicatorSurface Surface;
				Surface.Node = NodeId; Surface.Slot = Slot; Surface.Mesh = Mesh; Surface.Emissive = Emissive;
				NativeIndicatorSurfaces.Add(Surface);
				KeepMids.Add(Mid);
				UE_LOG(LogTemp, Display, TEXT("S47: native indicator %s slot=%d material=%s"), *N.Name, Slot, *Original->GetName());
			}
		}
		NativeIndicatorNodeCount = Nodes.Num();
	}
	for (const FNativeIndicatorSurface& Surface : NativeIndicatorSurfaces)
		if (UStaticMeshComponent* Mesh = Surface.Mesh.Get())
			if (UMaterialInstanceDynamic* Mid = Mesh->CreateDynamicMaterialInstance(Surface.Slot))
				Mid->SetVectorParameterValue(TEXT("EmissiveFactor"), Surface.Emissive * FMath::Clamp(Power, 0.f, 1.f));
	// GateIndicatorMesh cannot add entries here: every fixed mesh is already registered.
	for (FIndicatorSurface& Surface : IndicatorSurfaces)
		if (Surface.bFixed && Surface.Mesh.IsValid())
			GateIndicatorMesh(Surface.Mesh.Get(), Surface.Screen >= 0 && !IsCrtPowered(Surface.Screen) ? 0.f : Power);
}

void AS47Web::ReviewIndicators()
{
	FString Directory;
	if (!FParse::Value(FCommandLine::Get(), TEXT("S47IndicatorTest="), Directory)) return;
	US47Main* Main = US47Main::Get(this);
	if (!Main || !Main->Story || !Main->Player || !Main->World) return;
	struct FReview
	{
		TWeakObjectPtr<AS47Web> World;
		double Begin = 0, Changed = 0;
		int32 Phase = -1;
		bool bShot = false, bDone = false, bFailed = false, bPaused = false, bDark = false;
		bool bPowered = false, bRxPowered = false, bK3Open = false;
		FString View, Area;
		FVector Position;
		double Yaw = 0, Pitch = 0, Eye = 1.62;
		TArray<double> Camera;
		TArray<bool> Leds, Screens;
		TArray<float> Ceiling, Lamps;
		float Crt = 0;
	};
	static FReview R;
	const double Now = FPlatformTime::Seconds();
	if (R.World != this)
	{
		R = FReview(); R.World = this; R.Begin = Now;
		R.View = TEXT("rack"); FParse::Value(FCommandLine::Get(), TEXT("S47IndicatorView="), R.View);
		if (R.View != TEXT("rack") && R.View != TEXT("console") && R.View != TEXT("printer") && R.View != TEXT("processor") && R.View != TEXT("auxiliary") && R.View != TEXT("terminal"))
		{
			UE_LOG(LogTemp, Error, TEXT("S47: indicator test invalid view %s"), *R.View); R.bDone = true; return;
		}
		R.bPaused = Main->Story->IsPaused(); R.bDark = Main->Story->bDark;
		R.bPowered = Main->Story->bPowered; R.bRxPowered = Main->Story->Rx.bPowered;
		R.bK3Open = Main->Story->Night().bK3Open;
		R.Area = CurrentArea; R.Position = Main->Player->Pos; R.Yaw = Main->Player->Yaw; R.Pitch = Main->Player->Pitch; R.Eye = Main->Player->Eye;
		FString CameraOverride;
		if (FParse::Value(FCommandLine::Get(), TEXT("S47IndicatorCamera="), CameraOverride, false))
		{
			TArray<FString> Fields; CameraOverride.ParseIntoArray(Fields, TEXT(";"), false);
			bool bValid = Fields.Num() == 5;
			if (bValid) for (const FString& Field : Fields) { double Value = 0; bValid &= LexTryParseString(Value, *Field) && FMath::IsFinite(Value); R.Camera.Add(Value); }
			bValid &= R.Camera.Num() == 5 && R.Camera[4] > 0 && R.Camera[4] <= 3;
			if (!bValid) { UE_LOG(LogTemp, Error, TEXT("S47: invalid indicator camera; expected finite x;z;yaw;pitch;eye, eye in (0,3]")); R.bDone = true; return; }
		}
		R.Leds = RackLedOn; R.Screens = CrtOn;
		for (const auto& L : CeilingLights) R.Ceiling.Add(L.IsValid() ? L->Intensity : 0.f);
		for (const auto& L : LampLights) R.Lamps.Add(L.IsValid() ? L->Intensity : 0.f);
		R.Crt = CrtLight.IsValid() ? CrtLight->Intensity : 0.f;
		IFileManager::Get().MakeDirectory(*Directory, true);
		IFileManager::Get().Delete(*(Directory / (R.View + TEXT("-checks.txt"))));
		for (const TCHAR* Phase : { TEXT("off"), TEXT("online"), TEXT("flicker"), TEXT("half-power"), TEXT("dark"), TEXT("restore") })
			IFileManager::Get().Delete(*(Directory / (R.View + TEXT("-") + Phase + TEXT(".png"))));
		Main->World->Enter(TEXT("saro"));
	}
	if (R.bDone) return;
	Main->Story->SetPaused(true);
	Main->Player->Eye = R.Eye;
	if (R.View == TEXT("rack")) { Main->Player->Place(4.25, -2.45, -UE_DOUBLE_HALF_PI); Main->Player->Pitch = -0.26; }
	else if (R.View == TEXT("console")) { Main->Player->Place(2.8, -2.65, 0.35); Main->Player->Pitch = -0.30; }
	else if (R.View == TEXT("printer")) { Main->Player->Place(4.4, -0.4, -UE_DOUBLE_HALF_PI); Main->Player->Pitch = -0.60; }
	else if (R.View == TEXT("auxiliary")) { Main->Player->Place(-0.4, 2.2, UE_DOUBLE_PI); Main->Player->Pitch = -0.35; }
	else if (R.View == TEXT("terminal")) { Main->Player->Place(-0.05, 3.4, 2.5); Main->Player->Pitch = -0.1; Main->Player->Eye = 1.0; }
	else { Main->Player->Place(2.90, -2.95, 0); Main->Player->Pitch = -0.55; }
	if (!R.Camera.IsEmpty()) { Main->Player->Place(R.Camera[0], R.Camera[1], R.Camera[2]); Main->Player->Pitch = R.Camera[3]; Main->Player->Eye = R.Camera[4]; }
	float Warmup = 3.f, Hold = 2.f;
	FParse::Value(FCommandLine::Get(), TEXT("S47IndicatorWarmup="), Warmup);
	FParse::Value(FCommandLine::Get(), TEXT("S47IndicatorHold="), Hold);
	Hold = FMath::Max(1.f, Hold);
	if (Now - R.Begin < Warmup) return;
	static const TCHAR* Names[] = { TEXT("off"), TEXT("online"), TEXT("flicker"), TEXT("half-power"), TEXT("dark"), TEXT("restore") };
	auto Check = [&Directory](bool bOk, const FString& What)
	{
		const FString Line = FString::Printf(TEXT("%s indicator: %s\n"), bOk ? TEXT("PASS") : TEXT("FAIL"), *What);
		UE_LOG(LogTemp, Display, TEXT("S47: %s"), *Line.TrimEnd());
		FFileHelper::SaveStringToFile(Line, *(Directory / (R.View + TEXT("-checks.txt"))), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
		R.bFailed |= !bOk;
	};
	if (R.Phase < 0 || (R.bShot && Now - R.Changed >= Hold && IFileManager::Get().FileSize(*(Directory / (R.View + TEXT("-") + Names[R.Phase] + TEXT(".png")))) > 32))
	{
		++R.Phase; R.Changed = Now; R.bShot = false;
		if (R.Phase == int32(UE_ARRAY_COUNT(Names)))
		{
			for (int32 I = 0; I < R.Ceiling.Num(); ++I) if (CeilingLights[I].IsValid()) CeilingLights[I]->SetIntensity(R.Ceiling[I]);
			for (int32 I = 0; I < R.Lamps.Num(); ++I) if (LampLights[I].IsValid()) LampLights[I]->SetIntensity(R.Lamps[I]);
			if (CrtLight.IsValid()) CrtLight->SetIntensity(R.Crt);
			for (int32 I = 0; I < R.Screens.Num(); ++I) SetCrtPowered(I, R.Screens[I]);
			for (int32 I = 0; I < R.Leds.Num(); ++I) SetLed(I, R.Leds[I]);
			Main->Story->bPowered = R.bPowered; Main->Story->Rx.bPowered = R.bRxPowered;
			Main->Story->NightMutable().bK3Open = R.bK3Open;
			Main->Story->PowerOut(R.bDark); Main->Story->SetPaused(R.bPaused);
			Main->Player->Eye = R.Eye; Main->World->Enter(R.Area); Main->Player->Place(R.Position.X, R.Position.Z, R.Yaw); Main->Player->Pitch = R.Pitch;
			UpdateNativeLighting(); R.bDone = true;
			UE_LOG(LogTemp, Display, TEXT("S47: indicator test complete %s view=%s phases=6"), R.bFailed ? TEXT("FAIL") : TEXT("PASS"), *R.View);
			return;
		}
		const bool bOnline = R.Phase != 0;
		// Pausing the story clock does not stop its live LED flicker. Drive the real
		// receiver/K3 flags as well, so the off frame cannot be relit before capture.
		const bool bReceiverOn = bOnline && R.Phase != 4;
		Main->Story->bPowered = bReceiverOn; Main->Story->Rx.bPowered = bReceiverOn;
		Main->Story->NightMutable().bK3Open = !bReceiverOn;
		SetLightsScale(R.Phase == 4 ? 0.f : R.Phase == 3 ? 0.5f : 1.f, R.Phase == 4 ? 0.f : R.Phase == 3 ? 0.5f : 1.f);
		Main->Story->PowerOut(R.Phase == 4);
		for (int32 I = 0; I < 3; ++I) SetCrtPowered(I, bOnline);
		SetCrtGlow(R.Phase == 4 ? 0.f : R.Phase == 3 ? 0.45f : bOnline ? 0.9f : 0.f);
		SetLeds(bOnline); // Includes SetLed(true) after the station has gone dark.
		if (R.Phase == 2) { SetLed(0, false); SetLed(7, false); FlickerLed(); }
		if (R.Phase == 4) NodeMids(LedNode); // Clone all dark slots; restore must still light all six.
		if (R.Phase == 4) for (const FNativeIndicatorSurface& Surface : NativeIndicatorSurfaces)
			if (UStaticMeshComponent* Mesh = Surface.Mesh.Get())
				if (UMaterialInstanceDynamic* Original = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(Surface.Slot)))
				{
					UMaterialInstanceDynamic* Clone = UMaterialInstanceDynamic::Create(Original->Parent.Get(), this);
					Clone->CopyParameterOverrides(Original);
					Mesh->SetMaterial(Surface.Slot, Clone); KeepMids.Add(Clone);
				}
		UpdateNativeLighting();
		if (R.Phase == 3)
		{
			// Exercise the same per-node copies / brightness writes as a print/flicker setter.
			const int32 Printer = Handle(TEXT("room.objs.printer"));
			bool bPrinter = false;
			for (FIndicatorSurface& Surface : IndicatorSurfaces) if (Surface.bFixed && Below(*this, Surface.Node, Printer))
			{
				bPrinter = true;
				ObjAt(Surface.Node).SetOpacity(0.4f); GateIndicatorMesh(Surface.Mesh.Get(), 0.5f);
				Check(FMath::IsNearlyEqual(Brightness(Surface.Mesh->GetMaterial(0)), 0.4f), TEXT("printer authored 0.4 survives station 0.5 without being multiplied twice"));
				ObjAt(Surface.Node).SetOpacity(1.f); GateIndicatorMesh(Surface.Mesh.Get(), 0.5f);
				break;
			}
			Check(bPrinter, TEXT("printer indicator exists for authored-state test"));
		}
	}
	if (R.bShot || Now - R.Changed < Hold - 0.2f) return;
	UInstancedStaticMeshComponent* Rack = Nodes.IsValidIndex(LedNode) ? Cast<UInstancedStaticMeshComponent>(Nodes[LedNode].Comp.Get()) : nullptr;
	const float Power = R.Phase == 4 ? 0.f : R.Phase == 3 ? 0.5f : 1.f;
	const FString Prefix = FString(Names[R.Phase]) + TEXT(": ");
	const bool bReceiverExpected = R.Phase != 0 && R.Phase != 4;
	Check(Main->Story->bPowered == bReceiverExpected && Main->Story->Rx.bPowered == bReceiverExpected
		&& Main->Story->Night().bK3Open == !bReceiverExpected
		&& (R.Phase != 0 || !RackLedOn.Contains(true)), Prefix + TEXT("actual receiver/K3 state agrees; powered-off bank cannot be relit by live story flicker"));
	Check(Rack && Rack->GetInstanceCount() == LedCount() && Rack->NumCustomDataFloats >= 3, Prefix + TEXT("18 receiver indicators carry per-instance RGB"));
	if (Rack)
	{
		bool bSlots = Rack->GetNumMaterials() > 0, bColors = true;
		for (int32 Slot = 0; Slot < Rack->GetNumMaterials(); ++Slot)
		{
			const UMaterialInterface* Mat = Rack->GetMaterial(Slot);
			float InstanceColor = 0;
			bSlots &= Mat && Mat->GetScalarParameterValue(FMaterialParameterInfo(TEXT("UseInstanceColor")), InstanceColor) && InstanceColor > 0.5f && FMath::IsNearlyEqual(Brightness(Mat), Power);
		}
		for (int32 I = 0; I < RackLedOn.Num(); ++I)
		{
			const FLinearColor C = RackColor(I, RackLedOn[I]);
			const int32 At = I * Rack->NumCustomDataFloats;
			bColors &= Rack->PerInstanceSMCustomData.IsValidIndex(At + 2)
				&& FMath::IsNearlyEqual(Rack->PerInstanceSMCustomData[At], C.R)
				&& FMath::IsNearlyEqual(Rack->PerInstanceSMCustomData[At + 1], C.G)
				&& FMath::IsNearlyEqual(Rack->PerInstanceSMCustomData[At + 2], C.B);
		}
		Check(bSlots, Prefix + FString::Printf(TEXT("all %d rack GLB slots have emissive instance material and one power fade"), Rack->GetNumMaterials()));
		Check(bColors, Prefix + TEXT("green/amber/off match live SetLed/FlickerLed state"));
	}
	int32 Fixed = 0;
	bool bFixed = true;
	for (const FIndicatorSurface& Surface : IndicatorSurfaces) if (Surface.bFixed && Surface.Mesh.IsValid())
	{
		++Fixed;
		const float Expected = Surface.Screen >= 0 && !IsCrtPowered(Surface.Screen) ? 0.f : Power;
		for (int32 Slot = 0; Slot < Surface.Mesh->GetNumMaterials(); ++Slot)
		{
			const UMaterialInterface* Material = Surface.Mesh->GetMaterial(Slot);
			float Value = Brightness(Material);
			if (Surface.bMapped)
			{
				const UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Material);
				float Imported = 1.f;
				bFixed &= Mid && Mid->Parent && Mid->Parent->GetScalarParameterValue(FMaterialParameterInfo(TEXT("EmissivePower")), Imported);
				if (Material) Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("EmissivePower")), Value);
			}
			bFixed &= FMath::IsNearlyEqual(Value, Expected);
		}
	}
	Check(Fixed >= 5 && bFixed, Prefix + TEXT("console/printer/fixed indicators preserve station and device power on every slot"));
	if (!FParse::Param(FCommandLine::Get(), TEXT("S47PropsOff")))
	{
		bool bNative = NativeIndicatorSurfaces.Num() == 5;
		for (const FNativeIndicatorSurface& Surface : NativeIndicatorSurfaces)
		{
			const UStaticMeshComponent* Mesh = Surface.Mesh.Get();
			const UMaterialInstanceDynamic* Mid = Mesh ? Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(Surface.Slot)) : nullptr;
			FLinearColor Factor;
			bNative &= Mid && Mid->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveFactor")), Factor)
				&& Factor.Equals(Surface.Emissive * Power, 0.00001f);
			if (!Mid || !Mid->Parent) { bNative = false; continue; }
			float OriginalStrength = 1.f, Strength = 1.f;
			Mid->Parent->GetScalarParameterValue(FMaterialParameterInfo(TEXT("EmissiveStrength")), OriginalStrength);
			Mid->GetScalarParameterValue(FMaterialParameterInfo(TEXT("EmissiveStrength")), Strength);
			bNative &= FMath::IsNearlyEqual(Strength, OriginalStrength);
			UTexture* OriginalMap = nullptr;
			if (Mid->Parent->GetTextureParameterValue(FMaterialParameterInfo(TEXT("EmissiveTexture")), OriginalMap))
			{
				UTexture* Map = nullptr;
				bNative &= Mid->GetTextureParameterValue(FMaterialParameterInfo(TEXT("EmissiveTexture")), Map) && Map == OriginalMap;
			}
		}
		Check(bNative, Prefix + TEXT("five native DP-80/CB/3278 emitting slots follow SARO power once, preserve strength/maps and restore after dark MID clones"));
	}
	bool bProcessor = false, bProcessorOk = true;
	for (const FScreenLight& Screen : ScreenLights) if (Screen.Screen == -2 && Nodes.IsValidIndex(Screen.Node))
	{
		bProcessor = true;
		const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Nodes[Screen.Node].Comp.Get());
		bProcessorOk &= Mesh && FMath::IsNearlyEqual(Brightness(Mesh->GetMaterial(0)), Power)
			&& Screen.Light.IsValid() && (Power > 0 ? Screen.Light->Intensity > 0 : Screen.Light->Intensity == 0);
		const USceneComponent* Holder = Mesh ? Mesh->GetAttachParent() : nullptr;
		const USceneComponent* Group = Holder ? Holder->GetAttachParent() : nullptr;
		TArray<USceneComponent*> Parts;
		if (Group) Group->GetChildrenComponents(true, Parts);
		int32 Faces = 0;
		for (const USceneComponent* Part : Parts) if (const UStaticMeshComponent* Pane = Cast<UStaticMeshComponent>(Part))
			if (const UMaterialInstanceDynamic* Face = Cast<UMaterialInstanceDynamic>(Pane->GetMaterial(0)))
			{
				float UseMap = 0.f;
				Face->GetScalarParameterValue(FMaterialParameterInfo(TEXT("UseMap")), UseMap);
				if (UseMap > 0.5f || Pane->GetComponentLocation().Equals(Mesh->GetComponentLocation(), 0.25f))
				{
					++Faces;
					bProcessorOk &= FMath::IsNearlyEqual(Brightness(Face), Power);
				}
			}
		bProcessorOk &= Faces >= 4;
	}
	Check(bProcessor && bProcessorOk, Prefix + TEXT("processor picture/glass, amber VU faces and fitted light follow station power"));
	FScreenshotRequest::RequestScreenshot(Directory / (R.View + TEXT("-") + Names[R.Phase] + TEXT(".png")), true, false);
	R.bShot = true;
}

#include "S47RenderAudit.h"
#include "S47Web.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshResources.h"

namespace S47RenderAudit
{
namespace
{
	UMaterialInterface* SourceAsset(UMaterialInterface* Material)
	{
		while (UMaterialInstanceDynamic* Dynamic = Cast<UMaterialInstanceDynamic>(Material)) Material = Dynamic->Parent.Get();
		return Material;
	}

	// NodeMids intentionally makes one MID per slot. Pointer identity is therefore
	// insufficient: compare their inherited asset and effective runtime parameters.
	bool SameMaterialState(UMaterialInterface* A, UMaterialInterface* B, FString& Difference)
	{
		if (A == B) return true;
		if (!A || !B || SourceAsset(A) != SourceAsset(B)) { Difference = TEXT("different source material asset"); return false; }
		TArray<FMaterialParameterInfo> Infos;
		TArray<FGuid> Ids;
		A->GetAllScalarParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			float AV = 0, BV = 0;
			const bool FoundA = A->GetScalarParameterValue(Info, AV), FoundB = B->GetScalarParameterValue(Info, BV);
			if (FoundA != FoundB || !FMath::IsNearlyEqual(AV, BV, 1.e-6f)) { Difference = TEXT("scalar:") + Info.Name.ToString(); return false; }
		}
		Infos.Reset(); Ids.Reset(); A->GetAllVectorParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			FLinearColor AV, BV;
			const bool FoundA = A->GetVectorParameterValue(Info, AV), FoundB = B->GetVectorParameterValue(Info, BV);
			if (FoundA != FoundB || (FoundA && !AV.Equals(BV, 1.e-6f))) { Difference = TEXT("vector:") + Info.Name.ToString(); return false; }
		}
		Infos.Reset(); Ids.Reset(); A->GetAllTextureParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			UTexture* AV = nullptr; UTexture* BV = nullptr;
			const bool FoundA = A->GetTextureParameterValue(Info, AV), FoundB = B->GetTextureParameterValue(Info, BV);
			if (FoundA != FoundB || AV != BV) { Difference = TEXT("texture:") + Info.Name.ToString(); return false; }
		}
		return true;
	}
}
void Tick(AS47Web* Web)
{
	FString Directory;
	if (!Web || !FParse::Value(FCommandLine::Get(), TEXT("S47RenderAudit="), Directory)) return;
	struct FState { TWeakObjectPtr<AS47Web> Web; FString Area; double At = 0; bool Done = false; int32 Serial = 0; };
	static FState State;
	const double Now = FPlatformTime::Seconds();
	if (State.Web != Web) { State = FState(); State.Web = Web; }
	if (State.Area != Web->Area()) { State.Area = Web->Area(); State.At = Now; State.Done = false; }
	if (State.Done || Now - State.At < 3.0) return;
	State.Done = true;
	auto Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("area"), State.Area);
	Report->SetStringField(TEXT("basis"), TEXT("Cooked LOD section material index -> component override/mesh interface -> CheckMaterialUsage_Concurrent. This predicts the engine's Default fallback; it is not a GPU pixel measurement."));
	TArray<UStaticMeshComponent*> Components;
	Web->GetComponents<UStaticMeshComponent>(Components);
	TArray<TSharedPtr<FJsonValue>> Rows;
	int32 ActiveFallbackSections = 0, ActiveMappingErrors = 0, InactiveFallbackSections = 0;
	for (UStaticMeshComponent* Component : Components)
	{
		UStaticMesh* Mesh = Component->GetStaticMesh();
		const FStaticMeshRenderData* Data = Mesh ? Mesh->GetRenderData() : nullptr;
		if (!Data) continue;
		UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(Component);
		const FS47WebNode* Node = Web->Node(Web->NodeOf(Component));
		const bool SingleMaterial = Node && Node->Geo >= 0 && Node->Mats.Num() == 1;
		if (!ISM && !(SingleMaterial && Component->GetNumMaterials() > 1)) continue;
		const bool Active = Component->IsRegistered() && Component->IsVisible() && !Component->bHiddenInGame && !Web->IsHidden() && (!ISM || ISM->GetInstanceCount() > 0);
		auto Row = MakeShared<FJsonObject>();
		Row->SetStringField(TEXT("component"), Component->GetName());
		Row->SetStringField(TEXT("mesh"), Mesh->GetPathName());
		Row->SetBoolField(TEXT("active"), Active);
		Row->SetBoolField(TEXT("hidden_in_game"), Component->bHiddenInGame);
		Row->SetBoolField(TEXT("casts_shadow"), Component->CastShadow);
		Row->SetBoolField(TEXT("visible_in_ray_tracing"), Component->bVisibleInRayTracing);
		Row->SetBoolField(TEXT("mesh_supports_ray_tracing"), Mesh->bSupportRayTracing);
		Row->SetNumberField(TEXT("instances"), ISM ? ISM->GetInstanceCount() : 1);
		Row->SetNumberField(TEXT("source_materials"), Node ? Node->Mats.Num() : 0);
		bool MappingOk = true;
		FString MappingDifference;
		if (SingleMaterial) for (int32 Slot = 1; Slot < Component->GetNumMaterials(); ++Slot)
		{
			FString Difference;
			if (!SameMaterialState(Component->GetMaterial(Slot), Component->GetMaterial(0), Difference))
			{
				MappingOk = false;
				if (MappingDifference.IsEmpty()) MappingDifference = FString::Printf(TEXT("slot%d: %s"), Slot, *Difference);
			}
		}
		Row->SetBoolField(TEXT("single_material_binding_ok"), MappingOk);
		Row->SetStringField(TEXT("single_material_binding_difference"), MappingDifference);
		if (Active && !MappingOk) ++ActiveMappingErrors;
		TArray<TSharedPtr<FJsonValue>> Sections;
		for (int32 LOD = 0; LOD < Data->LODResources.Num(); ++LOD)
		{
			const FStaticMeshLODResources& Resource = Data->LODResources[LOD];
			for (int32 SectionIndex = 0; SectionIndex < Resource.Sections.Num(); ++SectionIndex)
			{
				const FStaticMeshSection& Section = Resource.Sections[SectionIndex];
				UMaterialInterface* Material = Component->GetMaterial(Section.MaterialIndex);
				const bool IsmUsageOk = Material && (!ISM || Material->CheckMaterialUsage_Concurrent(MATUSAGE_InstancedStaticMeshes));
				const bool NaniteUsageOk = Material && (!Data->HasValidNaniteData() || Material->CheckMaterialUsage_Concurrent(MATUSAGE_Nanite));
				const bool UsageOk = IsmUsageOk && NaniteUsageOk;
				auto Entry = MakeShared<FJsonObject>();
				Entry->SetNumberField(TEXT("lod"), LOD);
				Entry->SetNumberField(TEXT("section"), SectionIndex);
				Entry->SetNumberField(TEXT("material_index"), Section.MaterialIndex);
				Entry->SetNumberField(TEXT("triangles"), Section.NumTriangles);
				Entry->SetStringField(TEXT("material"), GetPathNameSafe(Material));
				Entry->SetStringField(TEXT("source_material_asset"), GetPathNameSafe(SourceAsset(Material)));
				FLinearColor Color;
				if (Material && Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Color")), Color))
					Entry->SetStringField(TEXT("effective_color"), Color.ToString());
				Entry->SetBoolField(TEXT("ism_usage_ok"), IsmUsageOk);
				Entry->SetBoolField(TEXT("nanite_usage_ok"), NaniteUsageOk);
				Entry->SetStringField(TEXT("predicted_section_material"), GetPathNameSafe(UsageOk ? Material : UMaterial::GetDefaultMaterial(MD_Surface)));
				if (!UsageOk) { if (Active) ++ActiveFallbackSections; else ++InactiveFallbackSections; }
				Sections.Add(MakeShared<FJsonValueObject>(Entry));
			}
		}
		Row->SetArrayField(TEXT("sections"), Sections);
		Rows.Add(MakeShared<FJsonValueObject>(Row));
	}
	Report->SetArrayField(TEXT("components"), Rows);
	Report->SetNumberField(TEXT("active_fallback_sections"), ActiveFallbackSections);
	Report->SetNumberField(TEXT("inactive_fallback_sections"), InactiveFallbackSections);
	Report->SetNumberField(TEXT("active_single_material_mapping_errors"), ActiveMappingErrors);
	FString Json;
	FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json));
	IFileManager::Get().MakeDirectory(*Directory, true);
	const FString Filename = Directory / FString::Printf(TEXT("%02d-%s.json"), ++State.Serial, *State.Area);
	const bool Saved = FFileHelper::SaveStringToFile(Json, *Filename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	UE_LOG(LogTemp, Display, TEXT("S47: render audit saved=%d activeFallbackSections=%d inactiveFallbackSections=%d mappingErrors=%d path=%s"), Saved, ActiveFallbackSections, InactiveFallbackSections, ActiveMappingErrors, *Filename);
	// Engine writes actual BLAS byte counts/names to Saved/Profiling. This read-only command
	// does not change residency, LOD policy, warning thresholds or the geometry budget.
	// UWorld::Exec handles world/demo commands only. UEngine::Exec reaches ConsoleManager.
	const bool Dispatched = GEngine && GEngine->Exec(Web->GetWorld(), TEXT("r.RayTracing.DumpUnreferencedAlwaysResidentGeometries"));
	auto CvarValue = [](const TCHAR* Name) { const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name); return Variable ? Variable->GetInt() : -1; };
	UE_LOG(LogTemp, Display, TEXT("S47: RT residency dump dispatched=%d rayTracing=%d referenceBasedResidency=%d profilingDir=%s"),
		Dispatched, CvarValue(TEXT("r.RayTracing")), CvarValue(TEXT("r.RayTracing.UseReferenceBasedResidency")), *FPaths::ProfilingDir());
}
}

#include "S47FloraMeshComponent.h"

#include "Engine/InstancedStaticMesh.h"
#include "Engine/World.h"
#include "PSOPrecacheComponent.h"

namespace
{
	class FS47FloraSceneProxy final : public FInstancedStaticMeshSceneProxy
	{
	public:
		FS47FloraSceneProxy(UInstancedStaticMeshComponent* Component, ERHIFeatureLevel::Type FeatureLevel)
			: FInstancedStaticMeshSceneProxy(Component, FeatureLevel) {}

		virtual bool CanBeOccluded() const override { return false; }
	};
}

FPrimitiveSceneProxy* US47FloraMeshComponent::CreateStaticMeshSceneProxy(Nanite::FMaterialAudit& NaniteMaterials, bool bCreateNanite)
{
	if (bCreateNanite) return Super::CreateStaticMeshSceneProxy(NaniteMaterials, bCreateNanite);

	// Match UE 5.8's ISM proxy creation policy. Inherited CreateSceneProxy still
	// validates and flushes the instance data after this proxy has been created.
	if (CheckPSOPrecachingAndBoostPriority() && GetPSOPrecacheProxyCreationStrategy() == EPSOPrecacheProxyCreationStrategy::DelayUntilPSOPrecached)
		return nullptr;

	GetOrCreateInstanceDataSceneProxy();
	return ::new FS47FloraSceneProxy(this, GetWorld()->GetFeatureLevel());
}

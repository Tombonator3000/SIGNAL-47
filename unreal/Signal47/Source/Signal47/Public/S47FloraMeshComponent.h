#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "S47FloraMeshComponent.generated.h"

// The streamed plant batches surround the camera. Keep ordinary ISM rendering and
// distance/frustum culling, but do not reuse primitive depth occlusion for them.
UCLASS()
class SIGNAL47_API US47FloraMeshComponent : public UInstancedStaticMeshComponent
{

	GENERATED_BODY()

protected:
	virtual FPrimitiveSceneProxy* CreateStaticMeshSceneProxy(Nanite::FMaterialAudit& NaniteMaterials, bool bCreateNanite) override;
};

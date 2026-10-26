#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
class UStaticMeshComponent;

// Authored visual replacements only. The exported scene still owns poses and story anchors.
namespace S47CowMeshes
{
	struct FSet
	{
		UStaticMesh* Body = nullptr;
		UStaticMesh* Head = nullptr;
		UStaticMesh* Abducted = nullptr;
		explicit operator bool() const { return Body && Head && Abducted; }
	};
	FSet Load();
	void Replace(UStaticMeshComponent* Component, UStaticMesh* Mesh);
}

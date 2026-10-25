#include "S47CowMeshes.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47CowMeshes, Log, All);

S47CowMeshes::FSet S47CowMeshes::Load()
{
	if (FParse::Param(FCommandLine::Get(), TEXT("S47CowsOff"))) return {};
	FSet Set;
	Set.Body = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Signal47/Art/Cows/SM_Cow_Alive_Body.SM_Cow_Alive_Body"));
	Set.Head = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Signal47/Art/Cows/SM_Cow_Alive_Head.SM_Cow_Alive_Head"));
	Set.Abducted = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Signal47/Art/Cows/SM_Cow_Abducted.SM_Cow_Abducted"));
	// A partial import must never replace only one half of the live animal or its counterpart.
	for (UStaticMesh* Mesh : { Set.Body, Set.Head, Set.Abducted })
	{
		bool bValid = Mesh && Mesh->GetNumLODs() == 3 && Mesh->GetStaticMaterials().Num() > 0;
		if (bValid)
		{
			for (int32 Lod = 0; Lod < 3; ++Lod) bValid &= Mesh->GetNumTriangles(Lod) > 0;
			for (int32 Slot = 0; Slot < Mesh->GetStaticMaterials().Num(); ++Slot) bValid &= Mesh->GetMaterial(Slot) != nullptr;
		}
		if (!bValid)
		{
			UE_LOG(LogS47CowMeshes, Warning, TEXT("S47: cow models incomplete or invalid; original cows retained"));
			return {};
		}
		UE_LOG(LogS47CowMeshes, Display, TEXT("S47: cow native asset %s LODs=%d triangles=%d/%d/%d materialslots=%d"),
			*Mesh->GetPathName(), Mesh->GetNumLODs(), Mesh->GetNumTriangles(0), Mesh->GetNumTriangles(1), Mesh->GetNumTriangles(2), Mesh->GetStaticMaterials().Num());
	}
	return Set;
}

void S47CowMeshes::Replace(UStaticMeshComponent* Component, UStaticMesh* Mesh)
{
	check(Component && Mesh);
	Component->SetStaticMesh(Mesh);
	Component->EmptyOverrideMaterials();
	Component->SetCastShadow(true);
}

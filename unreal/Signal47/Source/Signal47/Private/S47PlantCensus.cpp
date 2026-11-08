// Developer aid (critic 8 November: a hedge of shrubs 60-150 m out that no flora rule explained). From where the eye stands:
// which instanced meshes have copies in a band of distance ahead of it, and, with "paint", each of the busiest in one flat colour
// so a picture tells them apart. Console: s47.PlantCensus [near far halfangle] [paint]. Logged as "S47: census".
#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectIterator.h"

namespace
{
	struct FCensusRow { TWeakObjectPtr<UInstancedStaticMeshComponent> C; int32 Band = 0, Total = 0; };

	void PlantCensus(const TArray<FString>& Args, UWorld* World)
	{
		if (!World) return;
		APlayerController* PC = World->GetFirstPlayerController();
		if (!PC) { UE_LOG(LogTemp, Warning, TEXT("S47: census: no player controller")); return; }
		FVector Eye; FRotator View;
		PC->GetPlayerViewPoint(Eye, View);
		double Near = 60, Far = 160, Half = 45;
		bool bPaint = false;
		TArray<double> Nums;
		for (const FString& A : Args) { if (A == TEXT("paint")) bPaint = true; else if (A.IsNumeric()) Nums.Add(FCString::Atod(*A)); }
		if (Nums.Num() > 0) Near = Nums[0];
		if (Nums.Num() > 1) Far = Nums[1];
		if (Nums.Num() > 2) Half = Nums[2];
		const FVector Fwd = FVector(View.Vector().X, View.Vector().Y, 0).GetSafeNormal();
		const double CosHalf = FMath::Cos(FMath::DegreesToRadians(Half));
		TArray<FCensusRow> Rows;
		for (TObjectIterator<UInstancedStaticMeshComponent> It; It; ++It)
		{
			UInstancedStaticMeshComponent* C = *It;
			if (!C || C->GetWorld() != World || !C->IsRegistered() || !C->IsVisible() || C->bHiddenInGame || !C->GetStaticMesh()) continue;
			FCensusRow R; R.C = C; R.Total = C->GetInstanceCount();
			for (int32 I = 0; I < R.Total; ++I)
			{
				FTransform T;
				if (!C->GetInstanceTransform(I, T, true)) continue;
				const FVector D = T.GetLocation() - Eye;
				const FVector Flat(D.X, D.Y, 0);
				const double M = Flat.Size() / 100.0;
				if (M < Near || M > Far) continue;
				if (FVector::DotProduct(Flat.GetSafeNormal(), Fwd) < CosHalf) continue;
				++R.Band;
			}
			if (R.Band > 0) Rows.Add(R);
		}
		Rows.Sort([](const FCensusRow& A, const FCensusRow& B) { return A.Band > B.Band; });
		UE_LOG(LogTemp, Display, TEXT("S47: census at %s yaw %.0f, %.0f-%.0f m within %.0f degrees: %d instanced meshes"), *(Eye / 100).ToString(), View.Yaw, Near, Far, Half, Rows.Num());
		static const FLinearColor Palette[] = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1, 0}, {1, 0, 1}, {0, 1, 1}, {1, .5f, 0}, {.5f, 0, 1}, {1, 1, 1}, {0, 0, 0} };
		static const TCHAR* Names[] = { TEXT("red"), TEXT("green"), TEXT("blue"), TEXT("yellow"), TEXT("magenta"), TEXT("cyan"), TEXT("orange"), TEXT("violet"), TEXT("white"), TEXT("black") };
		UMaterialInterface* Base = bPaint ? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque")) : nullptr;
		for (int32 K = 0; K < Rows.Num() && K < 30; ++K)
		{
			UInstancedStaticMeshComponent* C = Rows[K].C.Get();
			if (!C) continue;
			const bool bPainted = Base && K < UE_ARRAY_COUNT(Palette);
			if (bPainted)
			{
				UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Base, C);
				D->SetVectorParameterValue(TEXT("Color"), Palette[K]);
				D->SetScalarParameterValue(TEXT("NativeLighting"), 1);
				for (int32 S = 0; S < C->GetNumMaterials(); ++S) C->SetMaterial(S, D);
			}
			UE_LOG(LogTemp, Display, TEXT("S47: census %2d %-8s band %5d of %6d: %s (owner %s, parent %s) mesh %s"), K, bPainted ? Names[K] : TEXT("-"), Rows[K].Band, Rows[K].Total,
				*C->GetName(), *GetNameSafe(C->GetOwner()), *GetNameSafe(C->GetAttachParent()), *GetNameSafe(C->GetStaticMesh()));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs GPlantCensus(TEXT("s47.PlantCensus"),
		TEXT("Developer: instanced meshes with copies ahead of the eye. Args: near far halfangle (m, m, degrees; 60 160 45), paint."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PlantCensus));
}

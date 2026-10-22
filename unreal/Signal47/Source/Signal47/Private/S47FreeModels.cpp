// Unreal only (departs from the web version): a look at the free models in the world, see S47FreeModels.h.
#include "S47FreeModels.h"

#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "S47Main.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Free, Log, All);

namespace
{
	// a few of each kind: the hero pickup, a car, the props of the room and the diner, the desert
	const TCHAR* DEFAULT_MODELS = TEXT("pickup_c10_1966,fairheaven_lt80_cop,jukebox,payphone_blue,cb_base,terminal_3278,typewriter_selectric,metal_stool_01,yucca,namaqualand_boulder_04,motel_sign_mojave");

	struct FRow { FString Name, Asset; FVector Size; };

	TArray<FRow> Pick(const FString& Wanted)
	{
		TArray<FRow> Out;
		FString Text;
		const FString Path = FPaths::ProjectContentDir() / TEXT("Free/free_models.json");
		TSharedPtr<FJsonObject> Root;
		if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
		{
			UE_LOG(LogS47Free, Warning, TEXT("S47: free models: no manifest at %s (Tools/freemodels/import_free.sh)"), *Path);
			return Out;
		}
		TArray<FString> Names;
		Wanted.ParseIntoArray(Names, TEXT(","));
		for (const FString& N : Names)
		{
			for (const TSharedPtr<FJsonValue>& V : Root->GetArrayField(TEXT("models")))
			{
				const TSharedPtr<FJsonObject> O = V->AsObject();
				const FString Asset = O->GetStringField(TEXT("asset"));
				if (O->GetStringField(TEXT("name")) != N && FPaths::GetBaseFilename(Asset) != N) continue;
				const TArray<TSharedPtr<FJsonValue>> S = O->GetObjectField(TEXT("bounds_cm"))->GetArrayField(TEXT("size"));
				Out.Add({ N, Asset, FVector(S[0]->AsNumber(), S[1]->AsNumber(), S[2]->AsNumber()) });
				break;
			}
		}
		return Out;
	}
}

void S47FreeModels::Test(US47Main* M)
{
	FString Wanted;
	if (!FParse::Value(FCommandLine::Get(), TEXT("S47FreeModels="), Wanted, false))
	{
		if (!FParse::Param(FCommandLine::Get(), TEXT("S47FreeModels"))) return;
		Wanted = DEFAULT_MODELS;
	}
	if (!M || !M->Web) return;
	TWeakObjectPtr<US47Main> Main = M;
	FTimerHandle H;
	M->Web->GetWorldTimerManager().SetTimer(H, [Main, Wanted]
	{
		if (!Main.IsValid() || !Main->Web || !Main->Player) return;
		UWorld* W = Main->Web->GetWorld();
		const AS47Walker* P = Main->Player;
		const TArray<FRow> Rows = Pick(Wanted);
		// three's axes: ahead of the player, and the row across the view
		const FVector Fwd(-FMath::Sin(P->Yaw), 0, -FMath::Cos(P->Yaw)), Right(FMath::Cos(P->Yaw), 0, -FMath::Sin(P->Yaw));
		double Width = 0, Deep = 0;
		for (const FRow& R : Rows) { Width += FMath::Max(R.Size.X, R.Size.Y) / 100 + 0.6; Deep = FMath::Max(Deep, FMath::Max(R.Size.X, R.Size.Y) / 100); }
		// far enough that the row fills the view (about 70 degrees across)
		const double Ahead = FMath::Max(4.0, Width * 0.5 / FMath::Tan(FMath::DegreesToRadians(35.0)) * 0.8) + Deep * 0.5;
		double Along = -Width * 0.5;
		int32 Placed = 0;
		for (const FRow& R : Rows)
		{
			const double Span = FMath::Max(R.Size.X, R.Size.Y) / 100;
			Along += Span * 0.5 + 0.3;
			const FVector At = P->Pos + Fwd * Ahead + Right * Along;
			Along += Span * 0.5 + 0.3;
			UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *R.Asset);
			if (!Mesh) { UE_LOG(LogS47Free, Warning, TEXT("S47: free model %s: %s did not load"), *R.Name, *R.Asset); continue; }
			// on the ground: the first thing under the spot, from 2.5 m above the player's floor (under any roof)
			FVector Base = S47Three::ToUE(At.X, P->FloorY, At.Z);
			FHitResult Hit;
			if (W && W->LineTraceSingleByChannel(Hit, Base + FVector(0, 0, 250), Base - FVector(0, 0, 3000), ECC_Visibility)) Base = Hit.ImpactPoint;
			FActorSpawnParameters SP;
			SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			// facing the player: the row's models turned to show their length across the view
			const FRotator Rot(0, FMath::RadiansToDegrees(-P->Yaw), 0);
			AStaticMeshActor* A = W ? W->SpawnActor<AStaticMeshActor>(Base, Rot, SP) : nullptr;
			if (!A) continue;
			A->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
			A->GetStaticMeshComponent()->SetStaticMesh(Mesh);
			++Placed;
			UE_LOG(LogS47Free, Display, TEXT("S47: free model %s at %.1f %.1f (ground %.2f m), %.0f x %.0f x %.0f cm, %d triangles%s"), *R.Name, At.X, At.Z, Base.Z / 100,
				R.Size.X, R.Size.Y, R.Size.Z, Mesh->GetNumTriangles(0), Mesh->IsNaniteEnabled() ? TEXT(", Nanite") : TEXT(""));
		}
		UE_LOG(LogS47Free, Display, TEXT("S47: free models placed %d of %d, %.1f m ahead"), Placed, Rows.Num(), Ahead);
	}, 1.5f, false);
}

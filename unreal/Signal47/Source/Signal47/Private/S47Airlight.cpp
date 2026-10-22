#include "S47Airlight.h"

#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "S47Web.h"

namespace
{
	double Num(const TSharedPtr<FJsonObject>& O, const TCHAR* K, double D = 0) { double V = D; if (O) O->TryGetNumberField(K, V); return V; }
	FLinearColor Col3(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		return O && O->TryGetArrayField(K, A) && A->Num() >= 3 ? FLinearColor((*A)[0]->AsNumber(), (*A)[1]->AsNumber(), (*A)[2]->AsNumber()) : FLinearColor::White;
	}
	UStaticMeshComponent* RetireMesh(AS47Web* W, int32 Node)
	{
		const FS47WebNode* N = W ? W->Node(Node) : nullptr;
		UStaticMeshComponent* M = N ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
		if (M)
		{
			M->SetHiddenInGame(true);
			M->SetVisibility(false);
			M->SetCastShadow(false);
			M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return M;
	}
}

bool FS47LightCones::Build(AS47Web* W, const TSharedPtr<FJsonObject>& J)
{
	Web = W;
	if (!J) return false;
	// Native lights scatter in the exponential height fog. The old additive cone geometry
	// would remain visible through shadows and would double the real shafts.
	Mesh = RetireMesh(W, int32(Num(J, TEXT("node"), -1)));
	const TArray<TSharedPtr<FJsonValue>>* La = nullptr;
	if (J->TryGetArrayField(TEXT("lamps"), La))
	{
		for (const TSharedPtr<FJsonValue>& V : *La)
		{
			const TSharedPtr<FJsonObject> O = V->AsObject();
			FLamp L;
			L.P = FVector(Num(O, TEXT("x")), Num(O, TEXT("y")), Num(O, TEXT("z")));
			L.Ground = Num(O, TEXT("ground")); L.R = Num(O, TEXT("r")); L.Col = Col3(O, TEXT("col")); L.K0 = Num(O, TEXT("k0"), 1);
			const TArray<TSharedPtr<FJsonValue>>* G = nullptr;
			if (O->TryGetArrayField(TEXT("glow"), G) && G->Num() == 2) { L.GlowNode = int32((*G)[0]->AsNumber()); L.GlowIndex = int32((*G)[1]->AsNumber()); }
			Lamps.Add(L);
		}
	}
	return Mesh.IsValid();
}

double FS47LightCones::LampK(int32 I) const
{
	if (!Lamps.IsValidIndex(I)) return 0;
	const FLamp& L = Lamps[I];
	AS47Web* W = Web.Get();
	FS47GlowPoints* G = W ? W->Glow(L.GlowNode) : nullptr;
	if (!G || !G->Col.IsValidIndex(L.GlowIndex)) return 1;
	return G->Col[L.GlowIndex].R / FMath::Max(L.K0, 1e-6);
}

bool FS47LightCones::Visible() const { return false; }

void FS47LightCones::Update(bool, double)
{
	if (UStaticMeshComponent* M = Mesh.Get()) M->SetVisibility(false);
}

bool FS47GroundHaze::Build(AS47Web* W, int32 Node)
{
	// The fog volume replaces the two unshadowed sheets, including their fake lamp tint.
	Mesh = RetireMesh(W, Node);
	return Mesh.IsValid();
}

bool FS47GroundHaze::Visible() const { return false; }

void FS47GroundHaze::Update(double, double, const FVector&, TOptional<double>, double, const FLinearColor&, const TArray<FS47HazeLamp>&)
{
	if (UStaticMeshComponent* M = Mesh.Get()) M->SetVisibility(false);
}

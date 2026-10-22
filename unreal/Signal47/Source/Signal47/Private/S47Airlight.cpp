#include "S47Airlight.h"

#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Three.h"
#include "S47Web.h"

namespace
{
	double Num(const TSharedPtr<FJsonObject>& O, const TCHAR* K, double D = 0) { double V = D; if (O) O->TryGetNumberField(K, V); return V; }
	FLinearColor Col3(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		return O && O->TryGetArrayField(K, A) && A->Num() >= 3 ? FLinearColor((*A)[0]->AsNumber(), (*A)[1]->AsNumber(), (*A)[2]->AsNumber()) : FLinearColor::White;
	}
	/** The node's mesh shown again with one of the life's materials (the export hides a shader's). */
	UStaticMeshComponent* Take(AS47Web* W, int32 Node, const TCHAR* Material, TWeakObjectPtr<UMaterialInstanceDynamic>& Out)
	{
		const FS47WebNode* N = W ? W->Node(Node) : nullptr;
		UStaticMeshComponent* M = N ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, Material);
		if (!M || !Base) return nullptr;
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Base, W);
		for (int32 k = 0; k < M->GetNumMaterials(); ++k) M->SetMaterial(k, D);
		M->SetHiddenInGame(false);
		M->SetCastShadow(false);
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->SetVisibility(false);
		Out = D;
		return M;
	}
}

// ---------- LightCones ----------

bool FS47LightCones::Build(AS47Web* W, const TSharedPtr<FJsonObject>& J)
{
	Web = W;
	if (!J) return false;
	Mesh = Take(W, int32(Num(J, TEXT("node"), -1)), TEXT("/Game/Web/Base/M_WebAirCone.M_WebAirCone"), Mid);
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
	const int32 N = FMath::Max(1, Lamps.Num());
	K.Init(0.f, N);
	UTexture2D* T = UTexture2D::CreateTransient(N, 1, PF_R32_FLOAT);
	T->SRGB = false; T->Filter = TF_Nearest; T->AddressX = TA_Clamp; T->AddressY = TA_Clamp;
	T->UpdateResource();
	KTex = T;
	if (UMaterialInstanceDynamic* D = Mid.Get())
	{
		D->SetTextureParameterValue(TEXT("K"), T);
		D->SetScalarParameterValue(TEXT("Lamps"), float(N));
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

bool FS47LightCones::Visible() const { return Mesh.IsValid() && Mesh->IsVisible(); }

void FS47LightCones::Update(bool bOn, double Gain)
{
	UStaticMeshComponent* M = Mesh.Get();
	if (!M) return;
	// (a child of its area's group: drawn only while the area is: Unreal has no hierarchy of visibility)
	const FS47WebNode* N = Web.IsValid() ? Web->Node(Web->NodeOf(M)) : nullptr;
	const bool bArea = !N || N->Parent < 0 || FS47Obj(Web.Get(), N->Parent).Shown();
	const bool bVis = bOn && Gain > 0.002 && Lamps.Num() > 0;
	if (M->IsVisible() != (bVis && bArea)) M->SetVisibility(bVis && bArea);
	if (!bVis) return;
	if (UMaterialInstanceDynamic* D = Mid.Get()) D->SetScalarParameterValue(TEXT("Gain"), float(Gain));
	bool bChanged = false;
	for (int32 i = 0; i < Lamps.Num(); ++i)
	{
		const float V = float(FMath::Clamp(LampK(i), 0.0, 1.0));
		if (K[i] != V) { K[i] = V; bChanged = true; }
	}
	UTexture2D* T = KTex.Get();
	if (!bChanged || !T || !T->GetPlatformData() || T->GetPlatformData()->Mips.Num() == 0) return;
	float* Px = static_cast<float*>(T->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
	for (int32 i = 0; i < K.Num(); ++i) Px[i] = K[i];
	T->GetPlatformData()->Mips[0].BulkData.Unlock();
	T->UpdateResource();
}

// ---------- GroundHaze ----------

bool FS47GroundHaze::Build(AS47Web* W, int32 InNode)
{
	Web = W; Node = InNode;
	Mesh = Take(W, Node, TEXT("/Game/Web/Base/M_WebHaze.M_WebHaze"), Mid);
	if (UMaterialInstanceDynamic* D = Mid.Get()) D->SetScalarParameterValue(TEXT("R"), float(R));
	return Mesh.IsValid();
}

bool FS47GroundHaze::Visible() const { return Mesh.IsValid() && Mesh->IsVisible(); }

void FS47GroundHaze::Update(double Dt, double T, const FVector& Cam, TOptional<double> Ground, double K, const FLinearColor& Fog, const TArray<FS47HazeLamp>& Lamps)
{
	UStaticMeshComponent* M = Mesh.Get();
	AS47Web* W = Web.Get();
	if (!M || !W) return;
	const bool bVis = Ground.IsSet() && K > 0.002;
	if (M->IsVisible() != bVis) M->SetVisibility(bVis);
	if (!Ground.IsSet()) { bBase = false; return; }
	if (!bVis) return;
	// the layers follow the land under the camera, smoothly, so a step up does not lift the haze at once
	Base = bBase ? Base + (*Ground - Base) * FMath::Min(1.0, Dt * 1.5) : *Ground;
	bBase = true;
	W->SetPosition(Node, FVector(Cam.X, Base, Cam.Z));
	UMaterialInstanceDynamic* D = Mid.Get();
	if (!D) return;
	// the haze is the night's own colour, a little paler than the fog it sits in but greyer, so the
	// land under the horizon stays darker than the sky over it (world/flora.ts desertGround)
	const float L = Fog.R * 0.3f + Fog.G * 0.45f + Fog.B * 0.25f;
	const FLinearColor Grey(L, L, L);
	const FLinearColor C = (Grey + (Fog - Grey) * 0.45f) * 1.25f + FLinearColor(0.008f, 0.009f, 0.011f);
	D->SetVectorParameterValue(TEXT("Col"), FLinearColor(C.R, C.G, C.B, 0.f));
	D->SetScalarParameterValue(TEXT("K"), float(K));
	D->SetScalarParameterValue(TEXT("Time"), float(T));
	// the nearest few lamps tint it
	TArray<TPair<const FS47HazeLamp*, double>> Near;
	for (const FS47HazeLamp& Lp : Lamps) if (Lp.K > 0.02) Near.Add({ &Lp, FMath::Square(Lp.P.X - Cam.X) + FMath::Square(Lp.P.Z - Cam.Z) });
	Near.StableSort([](const TPair<const FS47HazeLamp*, double>& A, const TPair<const FS47HazeLamp*, double>& B) { return A.Value < B.Value; });
	for (int32 i = 0; i < 4; ++i)
	{
		const FName Pn(*FString::Printf(TEXT("Lamp%d"), i)), Cn(*FString::Printf(TEXT("LampCol%d"), i));
		if (!Near.IsValidIndex(i)) { D->SetVectorParameterValue(Pn, FLinearColor(0, -999, 0, 1)); D->SetVectorParameterValue(Cn, FLinearColor(0, 0, 0, 0)); continue; }
		const FS47HazeLamp& E = *Near[i].Key;
		D->SetVectorParameterValue(Pn, FLinearColor(float(E.P.X), float(E.P.Y), float(E.P.Z), float(E.R * E.R)));
		const FLinearColor Lc = E.Col * float(0.16 * E.K);
		D->SetVectorParameterValue(Cn, FLinearColor(Lc.R, Lc.G, Lc.B, 0.f));
	}
}

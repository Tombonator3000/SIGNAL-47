#include "S47WindowLight.h"

#include "Components/RectLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "S47Three.h"
#include "S47Web.h"

namespace
{
	double Num(const TSharedPtr<FJsonObject>& O, const TCHAR* K, double D = 0) { double V = D; if (O) O->TryGetNumberField(K, V); return V; }
	const TArray<TSharedPtr<FJsonValue>>& Arr(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		static const TArray<TSharedPtr<FJsonValue>> Empty;
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		return O && O->TryGetArrayField(K, A) ? *A : Empty;
	}
}

FS47WindowLights::~FS47WindowLights()
{
	FWorldDelegates::OnWorldPostActorTick.Remove(LightTick);
	for (const FWin& Wn : Wins) if (URectLightComponent* L = Wn.Light.Get()) L->DestroyComponent();
}

bool FS47WindowLights::Build(AS47Web* W, const TSharedPtr<FJsonObject>& J)
{
	Web = W;
	if (!J || !W) return false;
	J->TryGetStringField(TEXT("name"), Name);
	Set = int32(Num(J, TEXT("set"), -1));
	BaseScale = FMath::Max(0.001f, W->FloodScaleOf(Set));
	FVector Origin = FVector::ZeroVector;
	const TArray<TSharedPtr<FJsonValue>>& O = Arr(J, TEXT("origin"));
	if (O.Num() == 3) Origin = FVector(O[0]->AsNumber(), O[1]->AsNumber(), O[2]->AsNumber());
	TArray<FVector> Pos;
	const TArray<TSharedPtr<FJsonValue>>& P = Arr(J, TEXT("pos"));
	for (int32 k = 0; k + 2 < P.Num(); k += 3) Pos.Add(FVector(P[k]->AsNumber(), P[k + 1]->AsNumber(), P[k + 2]->AsNumber()));
	const FS47WebNode* N = W->Node(int32(Num(J, TEXT("node"), -1)));
	UStaticMeshComponent* M = N ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
	if (!M) return false;
	Parent = N->Parent;
	// Retire the additive ground projection. These panes now emit from the facade and objects
	// between the window and the ground can cast actual shadows into that light.
	M->SetHiddenInGame(true);
	M->SetVisibility(false);
	M->SetCastShadow(false);
	M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh = M;
	for (const TSharedPtr<FJsonValue>& V : Arr(J, TEXT("wins")))
	{
		const TSharedPtr<FJsonObject> Wo = V->AsObject();
		FWin Wn;
		Wo->TryGetStringField(TEXT("id"), Wn.Id);
		Wn.Slot = int32(Num(Wo, TEXT("slot"), -1)); Wn.K = Num(Wo, TEXT("k")); Wn.W0 = Num(Wo, TEXT("w0"), 1); Wn.Level = Num(Wo, TEXT("level"), 1);
		const TArray<TSharedPtr<FJsonValue>>& C = Arr(Wo, TEXT("col"));
		const FLinearColor Col = C.Num() >= 3 ? FLinearColor(C[0]->AsNumber(), C[1]->AsNumber(), C[2]->AsNumber()) : FLinearColor::White;
		const int32 At = int32(Num(Wo, TEXT("at"))), Count = int32(Num(Wo, TEXT("n")));
		// The export's final 5 x 5 glow grid starts on the wall and runs 1.6 m outward. It
		// retains the original pane's span and normal even in older manifests without transforms.
		const int32 Glow = At + Count - 25;
		if (Count >= 25 && Pos.IsValidIndex(Glow) && Pos.IsValidIndex(Glow + 24))
		{
			FVector Sill = FVector::ZeroVector, Outer = FVector::ZeroVector;
			for (int32 I = 0; I < 5; ++I) { Sill += Pos[Glow + I]; Outer += Pos[Glow + 20 + I]; }
			Sill /= 5; Outer /= 5;
			FVector Out = Outer - Sill; Out.Y = 0; Out.Normalize();
			const double Width = FMath::Max(0.5, FVector::Dist(Pos[Glow], Pos[Glow + 4]) - 0.7);
			const bool bTv = Wn.Id.StartsWith(TEXT("tv"));
			// Pane heights are the existing source dimensions in ControlRoom/MotelFront/Town.
			const double Height = Name == TEXT("saro") ? 1.8 : Wn.Id == TEXT("office") ? 1.36 : Wn.Id == TEXT("room6") ? 1.2 : bTv || Wn.Id == TEXT("warm") ? 1.15 : 1.7;
			const double CentreY = Name == TEXT("saro") ? 1.85 : Wn.Id == TEXT("office") ? 1.39 : Wn.Id == TEXT("room6") ? 1.38 : bTv || Wn.Id == TEXT("warm") ? 1.5 : 1.55;
			const FVector Location = Origin + Sill + Out * 0.08 + FVector(0, CentreY - 0.02, 0);
			URectLightComponent* L = NewObject<URectLightComponent>(W);
			L->SetupAttachment(W->GetRootComponent());
			L->SetMobility(EComponentMobility::Movable);
			L->SetIntensityUnits(ELightUnits::Lumens);
			L->SetIntensity(0);
			L->SetLightColor(Col);
			L->SetSourceWidth(float(Width * 100));
			L->SetSourceHeight(float(Height * 100));
			L->SetAttenuationRadius(900);
			L->SetBarnDoorAngle(70);
			L->SetBarnDoorLength(20);
			L->SetCastShadows(true);
			L->SetCastVolumetricShadow(true);
			L->SetVolumetricScatteringIntensity(0.35f);
			L->SetVisibility(false);
			L->RegisterComponent();
			L->SetWorldLocationAndRotation(S47Three::ToUE(Location), S47Three::DirToUE(Out).Rotation());
			Wn.Light = L;
		}
		Wins.Add(Wn);
	}
	const TWeakObjectPtr<AS47Web> WeakWeb = W;
	LightTick = FWorldDelegates::OnWorldPostActorTick.AddLambda([this, WeakWeb](UWorld* World, ELevelTick, float)
	{
		if (WeakWeb.IsValid() && WeakWeb->GetWorld() == World) Update();
	});
	Update();
	return Wins.Num() > 0;
}

void FS47WindowLights::SetLevel(const FString& Id, double K)
{
	for (FWin& Wn : Wins) if (Wn.Id == Id && Wn.Slot < 0) Wn.Level = K;
}

TMap<FString, double> FS47WindowLights::Levels() const
{
	TMap<FString, double> O;
	for (const FWin& Wn : Wins) O.Add(Wn.Id, FMath::Max(O.FindRef(Wn.Id), Wn.Level));
	return O;
}

double FS47WindowLights::Darkness() const { return 1; }

void FS47WindowLights::Update()
{
	AS47Web* W = Web.Get();
	if (!W) return;
	if (UStaticMeshComponent* M = Mesh.Get()) M->SetVisibility(false);
	const bool bArea = Parent < 0 || FS47Obj(W, Parent).Shown();
	const double Scale = FMath::Max(0.f, W->FloodScaleOf(Set)) / BaseScale;
	for (FWin& Wn : Wins)
	{
		if (Wn.Slot >= 0) Wn.Level = FMath::Max(0.f, W->FloodPos(Set, Wn.Slot).A) / FMath::Max(0.001, Wn.W0);
		if (URectLightComponent* L = Wn.Light.Get())
		{
			const float Lumens = bArea ? float(2000 * Wn.K * FMath::Max(0.0, Wn.Level) * Scale) : 0.f;
			L->SetIntensity(Lumens);
			if (Wn.Slot >= 0) L->SetLightColor(W->FloodCol(Set, Wn.Slot));
			if (L->IsVisible() != (Lumens > 0.01f)) L->SetVisibility(Lumens > 0.01f);
		}
	}
}

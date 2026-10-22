#include "S47WindowLight.h"

#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Web.h"

namespace
{
	constexpr double OFF = 1e-3;   // a lamp core/ultra.ts holds for a real light is a tiny negative: it still burns
	double Num(const TSharedPtr<FJsonObject>& O, const TCHAR* K, double D = 0) { double V = D; if (O) O->TryGetNumberField(K, V); return V; }
	const TArray<TSharedPtr<FJsonValue>>& Arr(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		static const TArray<TSharedPtr<FJsonValue>> Empty;
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		return O && O->TryGetArrayField(K, A) ? *A : Empty;
	}
}

bool FS47WindowLights::Build(AS47Web* W, const TSharedPtr<FJsonObject>& J)
{
	Web = W;
	if (!J || !W) return false;
	J->TryGetStringField(TEXT("name"), Name);
	Set = int32(Num(J, TEXT("set"), -1));
	FallOff = Num(J, TEXT("fall"), 0.03);
	for (const TSharedPtr<FJsonValue>& V : Arr(J, TEXT("own"))) Own.Add(int32(V->AsNumber()));
	const TArray<TSharedPtr<FJsonValue>>& O = Arr(J, TEXT("origin"));
	if (O.Num() == 3) Origin = FVector(O[0]->AsNumber(), O[1]->AsNumber(), O[2]->AsNumber());
	const TArray<TSharedPtr<FJsonValue>>& P = Arr(J, TEXT("pos"));
	for (int32 k = 0; k + 2 < P.Num(); k += 3) Pos.Add(FVector(P[k]->AsNumber(), P[k + 1]->AsNumber(), P[k + 2]->AsNumber()));
	const TArray<TSharedPtr<FJsonValue>>& T = Arr(J, TEXT("tints"));
	for (int32 k = 0; k + 2 < T.Num(); k += 3) Tints.Add(FLinearColor(T[k]->AsNumber(), T[k + 1]->AsNumber(), T[k + 2]->AsNumber()));
	for (const TSharedPtr<FJsonValue>& V : Arr(J, TEXT("wins")))
	{
		const TSharedPtr<FJsonObject> Wo = V->AsObject();
		FWin Wn;
		Wo->TryGetStringField(TEXT("id"), Wn.Id);
		Wn.Slot = int32(Num(Wo, TEXT("slot"), -1)); Wn.K = Num(Wo, TEXT("k")); Wn.W0 = Num(Wo, TEXT("w0"), 1); Wn.Level = Num(Wo, TEXT("level"), 1);
		const TArray<TSharedPtr<FJsonValue>>& C = Arr(Wo, TEXT("col"));
		Wn.Col = C.Num() >= 3 ? FLinearColor(C[0]->AsNumber(), C[1]->AsNumber(), C[2]->AsNumber()) : FLinearColor::White;
		Wn.At = int32(Num(Wo, TEXT("at"))); Wn.N = int32(Num(Wo, TEXT("n")));
		for (const TSharedPtr<FJsonValue>& F : Arr(Wo, TEXT("fall"))) Wn.Fall.Add(F->AsNumber());
		Wins.Add(Wn);
	}
	Dark.Init(1, Pos.Num());
	// the mesh, shown again with its own material (the export hides a shader's)
	const FS47WebNode* N = W->Node(int32(Num(J, TEXT("node"), -1)));
	UStaticMeshComponent* M = N ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebWindowLight.M_WebWindowLight"));
	if (!M || !Base || Pos.Num() == 0) return false;
	UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Base, W);
	const int32 MapId = int32(Num(J, TEXT("map"), -1));
	if (UTexture2D* Map = MapId >= 0 ? LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Web/Tex/T%d.T%d"), MapId, MapId)) : nullptr) D->SetTextureParameterValue(TEXT("Map"), Map);
	const TArray<TSharedPtr<FJsonValue>>& Ms = Arr(J, TEXT("mapSize"));
	if (Ms.Num() == 2) D->SetVectorParameterValue(TEXT("MapSize"), FLinearColor(Ms[0]->AsNumber(), Ms[1]->AsNumber(), 0, 0));
	ColsW = 64; ColsH = FMath::Max(1, (Pos.Num() + ColsW - 1) / ColsW);
	UTexture2D* Ct = UTexture2D::CreateTransient(ColsW, ColsH, PF_A32B32G32R32F);
	Ct->SRGB = false; Ct->Filter = TF_Nearest; Ct->AddressX = TA_Clamp; Ct->AddressY = TA_Clamp;
	Ct->UpdateResource();
	Cols = Ct;
	D->SetTextureParameterValue(TEXT("Cols"), Ct);
	D->SetScalarParameterValue(TEXT("ColsW"), float(ColsW));
	D->SetScalarParameterValue(TEXT("ColsH"), float(ColsH));
	for (int32 k = 0; k < M->GetNumMaterials(); ++k) M->SetMaterial(k, D);
	M->SetHiddenInGame(false);
	M->SetCastShadow(false);
	M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh = M; Mid = D;
	bAgain = true;
	Update();
	return true;
}

void FS47WindowLights::SetLevel(const FString& Id, double K)
{
	for (FWin& Wn : Wins) if (Wn.Id == Id && Wn.Slot < 0 && Wn.Level != K) { Wn.Level = K; bAgain = true; }
}

TMap<FString, double> FS47WindowLights::Levels() const
{
	TMap<FString, double> O;
	for (const FWin& Wn : Wins) O.Add(Wn.Id, FMath::Max(O.FindRef(Wn.Id), Wn.Level));
	return O;
}

double FS47WindowLights::Darkness() const
{
	double S = 0;
	for (double D : Dark) S += D;
	return S / FMath::Max(1, Dark.Num());
}

void FS47WindowLights::Update()
{
	AS47Web* W = Web.Get();
	if (!W) return;
	bool bChanged = bAgain;
	bAgain = false;
	for (FWin& Wn : Wins)
	{
		if (Wn.Slot < 0) continue;
		const double P = W->FloodPos(Set, Wn.Slot).A, Lv = P > 0 ? P / Wn.W0 : P > -OFF && P < 0 ? Wn.Level : 0;
		if (Lv != Wn.Level) { Wn.Level = Lv; bChanged = true; }
	}
	// which of the area's other lamps burn: the light already on the ground where the windows' fall
	FString Sig;
	for (int32 i = 0; i < W->FloodCount(Set); ++i)
	{
		if (Own.Contains(i) || W->FloodSkip(Set, i)) continue;
		const float Pw = W->FloodPos(Set, i).A;
		Sig.AppendChar(Pw > 0 || (Pw < 0 && Pw > -OFF) ? TEXT('1') : TEXT('0'));
	}
	if (Sig != Lit) { Lit = Sig; Street(); bChanged = true; }
	if (!bChanged) return;
	TArray<FLinearColor> C;
	C.Init(FLinearColor::Black, Pos.Num());
	for (const FWin& Wn : Wins) for (int32 k = 0; k < Wn.N; ++k)
	{
		const int32 I = Wn.At + k;
		if (!C.IsValidIndex(I) || !Wn.Fall.IsValidIndex(k)) continue;
		const double F = Wn.Fall[k] * Wn.K * Wn.Level * Dark[I];
		const FLinearColor& T = Tints.IsValidIndex(I) ? Tints[I] : FLinearColor::White;
		C[I] = FLinearColor(float(Wn.Col.R * F * T.R), float(Wn.Col.G * F * T.G), float(Wn.Col.B * F * T.B), 1.f);
	}
	Upload(C);
}

void FS47WindowLights::Street()
{
	// the light of the area's own lamps on the ground at each point (kit.floodlit's pools without the
	// angle), and what is left for the window's: about half under a street lamp
	AS47Web* W = Web.Get();
	if (!W) return;
	for (int32 k = 0; k < Pos.Num(); ++k)
	{
		const FVector Q = Pos[k] + Origin;
		double E = 0;
		for (int32 i = 0; i < W->FloodCount(Set); ++i)
		{
			if (Own.Contains(i) || W->FloodSkip(Set, i)) continue;
			// (a lamp handed to a real light in Ultra, core/ultra.ts, burns as it did before)
			const FLinearColor P = W->FloodPos(Set, i);
			double Wd = 0;
			if (P.A > 0) { LastW.Add(i, P.A); Wd = P.A; }
			else if (P.A < 0 && P.A > -OFF) Wd = LastW.FindRef(i);
			if (Wd <= 0) continue;
			const double D = FVector::Dist(FVector(P.R, P.G, P.B), Q), Win = FMath::Max(0.0, 1 - D / (Wd * 1.6 + 8));
			const FLinearColor Cc = W->FloodCol(Set, i);
			E += Wd * Win * Win / (1 + D * D * FallOff) * (Cc.R * 0.3 + Cc.G * 0.59 + Cc.B * 0.11);
		}
		Dark[k] = 1 / (1 + 1.5 * E * W->FloodScaleOf(Set));
	}
}

void FS47WindowLights::Upload(const TArray<FLinearColor>& C)
{
	UTexture2D* T = Cols.Get();
	if (!T || !T->GetPlatformData() || T->GetPlatformData()->Mips.Num() == 0) return;
	FLinearColor* Px = static_cast<FLinearColor*>(T->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
	for (int32 k = 0; k < ColsW * ColsH; ++k) Px[k] = C.IsValidIndex(k) ? C[k] : FLinearColor::Black;
	T->GetPlatformData()->Mips[0].BulkData.Unlock();
	T->UpdateResource();
}

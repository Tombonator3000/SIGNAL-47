#include "S47Obj.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Three.h"
#include "S47Web.h"

using namespace S47Three;

namespace
{
	// three's texture matrix rows for a picture: as stored (the exported pictures, flipY) or the
	// other way up (a render target or a picture decoded here, row 0 at the top)
	void SetMapRows(UMaterialInstanceDynamic* D, bool bFlipY)
	{
		D->SetVectorParameterValue(TEXT("MapU"), FLinearColor(1, 0, 0, 0));
		D->SetVectorParameterValue(TEXT("MapV"), bFlipY ? FLinearColor(0, -1, 1, 0) : FLinearColor(0, 1, 0, 0));
	}
}

// ---------- FS47Obj ----------

bool FS47Obj::IsValid() const { return W && W->Nodes.IsValidIndex(I); }
FString FS47Obj::Name() const { return IsValid() ? W->Nodes[I].Name : FString(); }

FVector FS47Obj::Position() const { return IsValid() ? W->Nodes[I].P : FVector::ZeroVector; }
void FS47Obj::SetPosition(const FVector& P) { if (IsValid()) W->SetPosition(I, P); }
void FS47Obj::SetX(double V) { FVector P = Position(); P.X = V; SetPosition(P); }
void FS47Obj::SetY(double V) { FVector P = Position(); P.Y = V; SetPosition(P); }
void FS47Obj::SetZ(double V) { FVector P = Position(); P.Z = V; SetPosition(P); }
FVector FS47Obj::Rotation() const { return IsValid() ? EulerXYZ(W->Nodes[I].Q) : FVector::ZeroVector; }
void FS47Obj::SetRotation(const FVector& R) { if (IsValid()) W->SetRotation(I, R.X, R.Y, R.Z); }
void FS47Obj::SetRotX(double V) { FVector R = Rotation(); R.X = V; SetRotation(R); }
void FS47Obj::SetRotY(double V) { FVector R = Rotation(); R.Y = V; SetRotation(R); }
void FS47Obj::SetRotZ(double V) { FVector R = Rotation(); R.Z = V; SetRotation(R); }
FQuat FS47Obj::Quaternion() const { return IsValid() ? W->Nodes[I].Q : FQuat::Identity; }
void FS47Obj::SetQuaternion(const FQuat& Q) { if (IsValid()) W->SetLocal(I, W->Nodes[I].P, Q, W->Nodes[I].S); }
FVector FS47Obj::Scale() const { return IsValid() ? W->Nodes[I].S : FVector::OneVector; }
void FS47Obj::SetScale(const FVector& S) { if (IsValid()) W->SetScale(I, S); }

bool FS47Obj::Visible() const { return IsValid() && W->Nodes[I].bVis; }
bool FS47Obj::Shown() const
{
	if (!IsValid()) return false;
	for (int32 N = I; N >= 0; N = W->Nodes[N].Parent) if (!W->Nodes[N].bVis) return false;
	return true;
}
void FS47Obj::SetVisible(bool bOn) { if (IsValid() && W->Nodes[I].bVis != bOn) W->SetVisible(I, bOn); }

FVector FS47Obj::WorldPosition() const { return IsValid() ? W->NodeWorldThree(I).GetLocation() : FVector::ZeroVector; }
FQuat FS47Obj::WorldQuaternion() const { return IsValid() ? W->NodeWorldThree(I).GetRotation() : FQuat::Identity; }
FVector FS47Obj::WorldDirection() const { return WorldQuaternion().RotateVector(FVector(0, 0, 1)); }
FVector FS47Obj::LocalToWorld(const FVector& P) const { return IsValid() ? W->NodeWorldThree(I).TransformPosition(P) : P; }
FVector FS47Obj::WorldToLocal(const FVector& P) const { return IsValid() ? W->NodeWorldThree(I).InverseTransformPosition(P) : P; }

FBox FS47Obj::WorldBox() const
{
	FBox B(ForceInit);
	if (!IsValid()) return B;
	Traverse([&B](FS47Obj O)
	{
		const UPrimitiveComponent* C = Cast<UPrimitiveComponent>(O.W->Nodes[O.I].Comp.Get());
		if (!C) return;
		const FBox U = C->Bounds.GetBox();
		if (!U.IsValid) return;
		// Unreal (cm, z up) back to three (m, y up)
		B += FBox(FVector(U.Min.X, U.Min.Z, U.Min.Y) / 100.0, FVector(U.Max.X, U.Max.Z, U.Max.Y) / 100.0);
	});
	return B;
}

void FS47Obj::LookAt(const FVector& Target)
{
	// Object3D.lookAt for an object that is not a camera: its +Z towards the point, up +Y
	if (!IsValid()) return;
	const FVector Eye = WorldPosition();
	FVector Z = Target - Eye;
	if (Z.SizeSquared() < 1e-12) return;
	Z.Normalize();
	FVector X = FVector::CrossProduct(FVector(0, 1, 0), Z);
	if (X.SizeSquared() < 1e-12) X = FVector::CrossProduct(FVector(0, 0, 1), Z);
	X.Normalize();
	const FVector Y = FVector::CrossProduct(Z, X);
	// a rotation matrix with columns X, Y, Z, as three's makeBasis
	double M[16] = { X.X, X.Y, X.Z, 0, Y.X, Y.Y, Y.Z, 0, Z.X, Z.Y, Z.Z, 0, 0, 0, 0, 1 };
	FVector P, S; FQuat Q;
	Decompose(M, P, Q, S);
	const FS47Obj Up = Parent();
	if (Up.IsValid()) Q = Up.WorldQuaternion().Inverse() * Q;
	SetQuaternion(Q);
}

FS47Obj FS47Obj::Parent() const { return IsValid() ? FS47Obj(W, W->Nodes[I].Parent) : FS47Obj(); }

TArray<FS47Obj> FS47Obj::Children() const
{
	TArray<FS47Obj> Out;
	if (IsValid()) for (int32 C : W->Nodes[I].Children) Out.Add(FS47Obj(W, C));
	return Out;
}

void FS47Obj::Traverse(TFunctionRef<void(FS47Obj)> Fn) const
{
	if (!IsValid()) return;
	TArray<int32> Stack = { I };
	while (Stack.Num())
	{
		const int32 N = Stack.Pop(EAllowShrinking::No);
		Fn(FS47Obj(W, N));
		for (int32 C : W->Nodes[N].Children) Stack.Add(C);
	}
}

FS47Obj FS47Obj::Find(const FString& InName) const
{
	FS47Obj Found;
	Traverse([&](FS47Obj O) { if (!Found.IsValid() && O.Name() == InName) Found = O; });
	return Found;
}

void FS47Obj::SetColor(const FLinearColor& C) const
{
	if (!IsValid()) return;
	Traverse([&](FS47Obj O) { for (UMaterialInstanceDynamic* D : O.W->NodeMids(O.I)) D->SetVectorParameterValue(TEXT("Color"), C); });
}

void FS47Obj::SetEmissive(const FLinearColor& C, float Intensity) const
{
	if (!IsValid()) return;
	Traverse([&](FS47Obj O) { for (UMaterialInstanceDynamic* D : O.W->NodeMids(O.I)) D->SetVectorParameterValue(TEXT("Emissive"), C * Intensity); });
}

void FS47Obj::SetOpacity(float A) const
{
	if (!IsValid()) return;
	Traverse([&](FS47Obj O) { for (UMaterialInstanceDynamic* D : O.W->NodeMids(O.I)) { D->SetScalarParameterValue(TEXT("Opacity"), A); D->SetScalarParameterValue(TEXT("Brightness"), A); } });
}

void FS47Obj::SetMap(UTexture* T, bool bFlipY) const
{
	if (!IsValid()) return;
	Traverse([&](FS47Obj O)
	{
		for (UMaterialInstanceDynamic* D : O.W->NodeMids(O.I))
		{
			D->SetScalarParameterValue(TEXT("UseMap"), T ? 1.f : 0.f);
			D->SetScalarParameterValue(TEXT("MapIsLinear"), 0.f);
			if (T) D->SetTextureParameterValue(TEXT("Map"), T);
			SetMapRows(D, bFlipY);
		}
	});
}

void FS47Obj::SetInstance(int32 K, const FVector& P, const FQuat& Q, const FVector& S, bool bFlush) const
{
	if (!IsValid()) return;
	UInstancedStaticMeshComponent* C = Cast<UInstancedStaticMeshComponent>(W->Nodes[I].Comp.Get());
	if (!C || K < 0 || K >= C->GetInstanceCount()) return;
	double E[16];
	Compose(P, Q, S, E);
	C->UpdateInstanceTransform(K, MatToUE(E), false, bFlush, true);
}

// ---------- FS47Light ----------

ULightComponent* FS47Light::Component() const { return W ? W->LightByUuid(Uuid) : nullptr; }
bool FS47Light::IsValid() const { return Component() != nullptr; }
float FS47Light::Intensity() const { return W ? W->LightIntensity(Uuid) : 0.f; }
void FS47Light::SetIntensity(float V) const { if (W) W->SetLightIntensity(Uuid, V); }
FLinearColor FS47Light::Color() const { const ULightComponent* L = Component(); return L ? L->GetLightColor() : FLinearColor::Black; }
void FS47Light::SetColor(const FLinearColor& C) const { if (ULightComponent* L = Component()) L->SetLightColor(C); }
bool FS47Light::Visible() const { const ULightComponent* L = Component(); return L && L->IsVisible(); }
void FS47Light::SetVisible(bool bOn) const { if (ULightComponent* L = Component()) L->SetVisibility(bOn); }
FVector FS47Light::Position() const { const ULightComponent* L = Component(); return L ? ToThree(L->GetComponentLocation()) : FVector::ZeroVector; }
void FS47Light::SetPosition(const FVector& P) const { if (ULightComponent* L = Component()) L->SetWorldLocation(ToUE(P)); }
float FS47Light::Distance() const
{
	const ULocalLightComponent* L = Cast<ULocalLightComponent>(Component());
	return L ? L->AttenuationRadius / 100.f : 0.f;
}
void FS47Light::SetDistance(float D) const
{
	if (ULocalLightComponent* L = Cast<ULocalLightComponent>(Component())) L->SetAttenuationRadius(D > 0 ? D * 100.f : 3000.f);
}

// ---------- FS47Mat ----------

void FS47Mat::SetColor(const FLinearColor& C) const { if (UMaterialInstanceDynamic* D = IsValid() ? W->SharedMid(Id) : nullptr) D->SetVectorParameterValue(TEXT("Color"), C); }
void FS47Mat::SetEmissive(const FLinearColor& C, float Intensity) const { if (UMaterialInstanceDynamic* D = IsValid() ? W->SharedMid(Id) : nullptr) D->SetVectorParameterValue(TEXT("Emissive"), C * Intensity); }
void FS47Mat::SetOpacity(float A) const { if (UMaterialInstanceDynamic* D = IsValid() ? W->SharedMid(Id) : nullptr) { D->SetScalarParameterValue(TEXT("Opacity"), A); D->SetScalarParameterValue(TEXT("Brightness"), A); } }
void FS47Mat::SetScalar(const FName& Param, float V) const { if (UMaterialInstanceDynamic* D = IsValid() ? W->SharedMid(Id) : nullptr) D->SetScalarParameterValue(Param, V); }
void FS47Mat::SetMap(UTexture* T, bool bFlipY) const
{
	UMaterialInstanceDynamic* D = IsValid() ? W->SharedMid(Id) : nullptr;
	if (!D) return;
	D->SetScalarParameterValue(TEXT("UseMap"), T ? 1.f : 0.f);
	D->SetScalarParameterValue(TEXT("MapIsLinear"), 0.f);
	if (T) D->SetTextureParameterValue(TEXT("Map"), T);
	SetMapRows(D, bFlipY);
}

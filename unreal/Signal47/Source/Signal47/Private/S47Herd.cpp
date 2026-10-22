// drive/herd.ts
#include "S47Herd.h"

#include "Components/MaterialBillboardComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Main.h"
#include "S47RoadShape.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47World.h"

#include <cmath>

const TCHAR* S47Herd::TAG_RANCH = TEXT("ALDERETE");
const TArray<FString>& S47Herd::Tags()
{
	static const TArray<FString> V = { TEXT("11"), TEXT("14"), TEXT("18"), TEXT("22"), TEXT("25"), TEXT("29"), TEXT("31"), TEXT("34"), TEXT("38") };
	return V;
}

namespace
{
	const double COWS[9][3] = {
		{ -27.0, 538.5, 2.3 }, { -33.5, 542.0, -0.7 }, { -26.5, 549.5, 1.1 }, { -40.5, 547.5, 2.9 }, { -30.5, 557.5, -2.2 },
		{ -46.0, 556.0, 0.6 }, { -38.0, 563.5, -1.6 }, { -52.5, 566.5, 1.9 }, { -34.0, 575.0, -2.8 },
	};
	constexpr double AZ = 26 * UE_DOUBLE_PI / 180;
	const FVector FACING(FMath::Sin(AZ), 0, -FMath::Cos(AZ));
}

FS47Herd::FS47Herd(US47Main* InM, const FString& InPath, const FString& InArea) : Area(InArea), M(InM)
{
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (W)
	{
		Group = W->Obj(InPath + TEXT(".group")); Bodies = W->Obj(InPath + TEXT(".bodies"));
		Heads = W->Obj(InPath + TEXT(".heads")); Ring = W->Obj(InPath + TEXT(".ring"));
	}
	for (int32 I = 0; I < Count(); ++I)
	{
		Ys.Add(S47RoadTerrain::SurfaceY(COWS[I][0], COWS[I][1]));
		LookAreas.Add(W ? W->Obj(FString::Printf(TEXT("%s.lookAreas[%d]"), *InPath, I)) : FS47Obj());
	}
	// The exported points retain their local positions. Private eye indices are not handles, so
	// match each eye to its own exported sprite rather than depending on other glow points' order.
	TArray<UMaterialBillboardComponent*> Glows;
	const FS47Obj Glow = W ? W->Obj(InPath + TEXT(".glow.points")) : FS47Obj();
	if (const FS47WebNode* N = Glow ? W->Node(Glow.I) : nullptr)
		if (USceneComponent* C = N->Comp.Get()) for (USceneComponent* Ch : C->GetAttachChildren())
			if (UMaterialBillboardComponent* B = Cast<UMaterialBillboardComponent>(Ch)) Glows.Add(B);
	bStill = true;
	for (int32 I = 0; I < Count(); ++I)
	{
		Pose(I, 0);
		for (int32 E = 0; E < 2; ++E)
		{
			const FVector P = HeadP + HeadQ.RotateVector(S47Herd::EYE[E]);
			EyeAt.Add(P);
			UMaterialBillboardComponent* Best = nullptr;
			double BestD = 0.001;
			for (UMaterialBillboardComponent* B : Glows)
			{
				const double Dist = FVector::DistSquared(S47Three::ToThree(B->GetRelativeLocation()), P);
				if (Dist < BestD) { Best = B; BestD = Dist; }
			}
			Eyes.Add(Best && Best->Elements.Num() ? Cast<UMaterialInstanceDynamic>(Best->Elements[0].Material) : nullptr);
		}
	}
	bStill = false;
	Place(0);
}

// Herd.pose: body and head matrices in road-local terms.
void FS47Herd::Pose(int32 I, double T)
{
	const double X = COWS[I][0], Z = COWS[I][1], Graze = COWS[I][2];
	double Yaw, Pitch, Turn;
	if (bStill) { Yaw = -AZ; Pitch = 0.06; Turn = 0; }
	else
	{
		const double C = std::fmod(T / (13 + I * 1.9) + I * 0.37, 1.0);
		const double Up = S47RoadShape::Smooth(0, 0.07, C) * (1 - S47RoadShape::Smooth(0.3, 0.37, C));
		Yaw = -Graze + 0.09 * FMath::Sin(T * 0.045 + I * 1.7);
		Pitch = -1.22 * (1 - Up) + 0.05 * Up + 0.05 * FMath::Sin(T * 1.3 + I) * (1 - Up);
		Turn = 0.28 * FMath::Sin(T * 0.29 + I * 2.1) * (1 - Up) + 0.35 * FMath::Sin(T * 0.21 + I) * Up;
	}
	BodyP = FVector(X, Ys[I], Z); BodyQ = S47Three::Euler(0, Yaw, 0);
	HeadP = BodyP + BodyQ.RotateVector(S47Herd::NECK);
	HeadQ = BodyQ * S47Three::Euler(Pitch, Turn, 0, TEXT("YXZ"));
}

void FS47Herd::Place(double T)
{
	for (int32 I = 0; I < Count(); ++I)
	{
		if (I == Missing)
		{
			Bodies.SetInstance(I, FVector::ZeroVector, FQuat::Identity, FVector::ZeroVector, I == Count() - 1);
			Heads.SetInstance(I, FVector::ZeroVector, FQuat::Identity, FVector::ZeroVector, I == Count() - 1);
			continue;
		}
		Pose(I, T);
		Bodies.SetInstance(I, BodyP, BodyQ, FVector::OneVector, I == Count() - 1);
		Heads.SetInstance(I, HeadP, HeadQ, FVector::OneVector, I == Count() - 1);
	}
}

void FS47Herd::Set(bool bInStill, int32 InMissing, bool bRing)
{
	if (bInStill != bStill || InMissing != Missing) { bStill = bInStill; Missing = InMissing; Place(LastT); }
	Ring.SetVisible(bRing);
	for (int32 I = 0; I < LookAreas.Num(); ++I) LookAreas[I].SetVisible(I != Missing);
	UpdateObstacles();
}

FS47Obstacle FS47Herd::Obstacle(const FVector2D& Origin)
{
	return FS47Obstacle::Circle(COWS[S47Herd::MISSING][0] + Origin.X, COWS[S47Herd::MISSING][1] + Origin.Y,
		Missing == S47Herd::MISSING ? 0 : S47Herd::COW_R);
}

void FS47Herd::UpdateObstacles()
{
	if (!M || !M->Web || Area != TEXT("road")) return;
	const FVector P = Group.LocalToWorld(FVector(COWS[S47Herd::MISSING][0], Ys[S47Herd::MISSING], COWS[S47Herd::MISSING][1]));
	// The native drive owns value arrays. Find the exported circle by its unchanged centre each
	// step, instead of holding a pointer into a TArray which the area's other builders can grow.
	if (M->World) if (FS47DriveController* D = M->World->CabDrive()) if (D->Area)
		for (FS47Obstacle& O : D->Area->Obstacles)
			if (O.bCircle && FMath::Abs(O.X - P.X) < 0.001 && FMath::Abs(O.Z - P.Z) < 0.001) O.R = Missing == S47Herd::MISSING ? 0 : S47Herd::COW_R;
	for (const TCHAR* A : { TEXT("road"), TEXT("diner") })
		for (FS47Rect& C : M->Web->CollidersMutable(A))
			if (FMath::Abs((C.MinX + C.MaxX) * 0.5 - P.X) < 0.001 && FMath::Abs((C.MinZ + C.MaxZ) * 0.5 - P.Z) < 0.001)
				C.bEnabled = Missing != S47Herd::MISSING;
}

double FS47Herd::BeamAt(const FVector& P) const
{
	const FVector L = FVector(BeamPos.X, BeamPos.Y, BeamPos.Z) - P;
	const double Length = L.Size(), Dist = Length == 0 ? 1e-3 : Length;
	const double Cone = S47RoadShape::Smooth(0.68, 0.96, -FVector::DotProduct(L, BeamDir) / Dist);
	return BeamPos.W * Cone * 1.4 / (1 + Dist * Dist / 2500);
}

void FS47Herd::Update(double T, const TOptional<FS47HerdHeadlights>& Headlights)
{
	LastT = T;
	if (!bStill || bDirty) { Place(T); bDirty = false; }
	if (Headlights)
	{
		const FVector4& F0 = Headlights->F0;
		BeamDir = FVector(Headlights->F2.X - F0.X, 0, Headlights->F2.Z - F0.Z);
		if (BeamDir.SizeSquared() > 1) BeamDir.Normalize(); else BeamDir = FVector(0, 0, -1);
		BeamPos = FVector4(F0.X - BeamDir.X * 4, F0.Y, F0.Z - BeamDir.Z * 4, F0.W > 0 ? Headlights->ColorR : 0);
	}
	else BeamPos.W = 0;
	// Herd.beamLit: the material's small custom expression is in Tools/webexport/import_web.py.
	for (const FS47Obj& O : { Bodies, Heads, Ring }) if (O.IsValid())
		for (UMaterialInstanceDynamic* D : O.W->NodeMids(O.I))
		{
			D->SetVectorParameterValue(TEXT("BeamPos"), FLinearColor(BeamPos.X, BeamPos.Y, BeamPos.Z, BeamPos.W));
			D->SetVectorParameterValue(TEXT("BeamDir"), FLinearColor(BeamDir.X, BeamDir.Y, BeamDir.Z));
			D->SetScalarParameterValue(TEXT("BeamFlat"), O == Ring ? 1.f : 0.f);
		}
	const bool bWant = bStill && BeamPos.W > 0;
	if (!bWant && !bEyesLit) return;
	bEyesLit = bWant;
	for (int32 I = 0; I < Count(); ++I) for (int32 E = 0; E < 2; ++E)
	{
		double K = 0;
		if (bWant && I != Missing)
		{
			const FVector P = Group ? Group.LocalToWorld(EyeAt[I * 2 + E]) : EyeAt[I * 2 + E];
			const double Fx = BeamPos.X - P.X, Fz = BeamPos.Z - P.Z;
			const double Length = FMath::Sqrt(Fx * Fx + Fz * Fz), Fd = Length == 0 ? 1 : Length;
			K = BeamAt(P) * S47RoadShape::Smooth(0.55, 0.9, (Fx * FACING.X + Fz * FACING.Z) / Fd) * 1.6;
		}
		if (Eyes[I * 2 + E].IsValid()) Eyes[I * 2 + E]->SetVectorParameterValue(TEXT("Color"), FLinearColor(K * 0.55, K * 0.9, K * 0.7));
	}
}

bool FS47Herd::CowAt(int32 I, FVector& Out) const
{
	if (I < 0 || I >= Count() || I == Missing) return false;
	Out = FVector(COWS[I][0], Ys[I] + 1.1, COWS[I][1]);
	if (Group) Out = Group.LocalToWorld(Out);
	return true;
}

double FS47Herd::LitAt(int32 I) const { FVector P; return CowAt(I, P) ? BeamAt(P) : 0; }
FVector FS47Herd::Middle() const
{
	const FVector P(S47RoadShape::HERD.X, S47RoadTerrain::SurfaceY(S47RoadShape::HERD.X, S47RoadShape::HERD.Z) + 1, S47RoadShape::HERD.Z);
	return Group ? Group.LocalToWorld(P) : P;
}

bool FS47Herd::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool b, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s herd: %s"), b ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= b; };
	FS47Herd H(nullptr, FString(), TEXT("road"));
	Check(H.Count() == 9 && S47Herd::Tags()[S47Herd::MISSING] == TEXT("25"), TEXT("nine Herefords, missing ALDERETE 25"));
	H.bStill = true; H.Pose(4, 0);
	const FVector P = H.HeadP; const FQuat Q = H.HeadQ;
	H.Pose(4, 947);
	Check(H.HeadP.Equals(P, 1e-10) && H.HeadQ.Equals(Q, 1e-10), TEXT("026 still pose does not move"));
	const FVector F = H.BodyQ.RotateVector(FVector(0, 0, -1));
	Check(F.Equals(FACING, 1e-10), TEXT("the body faces azimuth 026"));
	H.bStill = false; H.Pose(4, 0); const FQuat Graze = H.HeadQ; H.Pose(4, 7);
	Check(!H.HeadQ.Equals(Graze, 1e-4), TEXT("the grazing head moves on its own clock"));
	// Matrices sampled from the original Herd in node/three (Float32 InstancedMesh storage).
	double E[16]; S47Three::Compose(H.HeadP, H.HeadQ, FVector::OneVector, E);
	Check(FMath::Abs(E[0] - -0.44334670901298523) < 1e-7 && FMath::Abs(E[6] - 0.4123375415802002) < 1e-7
		&& H.HeadP.Equals(FVector(-31.06801414489746, -0.9288950562477112, 557.9743041992188), 4e-5), TEXT("grazing matrix at t=7 agrees with original three.js"));
	Check(H.Middle().Equals(FVector(-37, -1.1610001087188722, 556), 1e-8)
		&& H.EyeAt[8].Equals(FVector(-30.042487525003317, -0.7795167454258476, 556.174161169384), 1e-8), TEXT("original terrain height, pasture middle and still eye placement"));
	H.Set(true, S47Herd::MISSING, true); FVector Out;
	Check(!H.CowAt(S47Herd::MISSING, Out) && H.CowAt(0, Out) && H.LitAt(S47Herd::MISSING) == 0, TEXT("the ninth and its beam sample disappear"));
	Check(H.Obstacle(FVector2D(8000, 0)).R == 0, TEXT("missing cow does not stop the truck"));
	H.BeamPos = FVector4(0, 0, 0, 1); H.BeamDir = FVector(0, 0, -1);
	Check(FMath::Abs(H.BeamAt(FVector(0, 0, -50)) - 0.7) < 1e-12 && H.BeamAt(FVector(0, 0, 50)) == 0, TEXT("exact cone and 2500 metre-squared beam falloff"));
	return bOk;
}

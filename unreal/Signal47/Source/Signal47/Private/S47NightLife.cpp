#include "S47NightLife.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Drive.h"
#include "S47Flashlight.h"
#include "S47Flow.h"
#include "S47Geo.h"
#include "S47Glow.h"
#include "S47Legs.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47RoadShape.h"
#include "S47RoadTerrain.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Truck.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Life2, Log, All);

namespace
{
	const FVector ROAD_ORIGIN(8000, 0, 0);
	/** The way the wind blows the tumbleweeds and drifts the dust out in the open: towards the north-north-east. */
	const FVector WIND = FVector(0.45, 0, -0.89).GetSafeNormal();
	constexpr double WEED_R = 0.36;
	double Smoothstep(double X, double Min, double Max)
	{
		if (X <= Min) return 0;
		if (X >= Max) return 1;
		const double T = (X - Min) / (Max - Min);
		return T * T * (3 - 2 * T);
	}
	double Num(const TSharedPtr<FJsonObject>& O, const TCHAR* K, double D = 0) { double V = D; if (O) O->TryGetNumberField(K, V); return V; }
	const TArray<TSharedPtr<FJsonValue>>& Arr(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		static const TArray<TSharedPtr<FJsonValue>> Empty;
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		return O && O->TryGetArrayField(K, A) ? *A : Empty;
	}
	TSharedPtr<FJsonObject> Obj(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		const TSharedPtr<FJsonObject>* P = nullptr;
		return O && O->TryGetObjectField(K, P) ? *P : nullptr;
	}
	TArray<float> Floats(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		TArray<float> Out;
		for (const TSharedPtr<FJsonValue>& V : Arr(O, K)) Out.Add(float(V->AsNumber()));
		return Out;
	}
	FVector V3(const TArray<TSharedPtr<FJsonValue>>& A) { return A.Num() >= 3 ? FVector(A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber()) : FVector::ZeroVector; }
	/** A county road runs north and south 1.4 km east of the highway, beyond the swells (road's terms). */
	double COUNTY(double Z) { return 1450 + 70 * FMath::Sin(Z / 900 + 0.4); }
	const FLinearColor HEAD(1, 0.9f, 0.72f), TAIL(1, 0.05f, 0.03f), POOL(0.11f, 0.085f, 0.05f);
	/** three's applyAxisAngle about +y */
	FVector AboutUp(const FVector& V, double A) { return FVector(V.X * FMath::Cos(A) + V.Z * FMath::Sin(A), V.Y, -V.X * FMath::Sin(A) + V.Z * FMath::Cos(A)); }
	/** three's Quaternion.setFromAxisAngle and premultiply, in its own terms */
	FQuat AxisAngle(const FVector& Axis, double Angle) { const double S = FMath::Sin(Angle / 2); return FQuat(Axis.X * S, Axis.Y * S, Axis.Z * S, FMath::Cos(Angle / 2)); }
	FQuat QMul(const FQuat& A, const FQuat& B)
	{
		return FQuat(A.X * B.W + A.W * B.X + A.Y * B.Z - A.Z * B.Y, A.Y * B.W + A.W * B.Y + A.Z * B.X - A.X * B.Z,
			A.Z * B.W + A.W * B.Z + A.X * B.Y - A.Y * B.X, A.W * B.W - A.X * B.X - A.Y * B.Y - A.Z * B.Z);
	}
	/** The picture's lines (core/vhs.ts lines): 720 under the tape, 480 under the worn one, the screen's without. */
	double LinesNow(const US47Main* M)
	{
		const FString Pic = M && M->Flow ? M->Flow->Picture : FString(TEXT("vhs"));
		FVector2D Size(1600, 900);
		if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(Size);
		return Pic == TEXT("off") ? FMath::Max(1.0, Size.Y) : FMath::Min(FMath::Max(1.0, Size.Y), Pic == TEXT("heavy") ? 480.0 : 720.0);
	}
	UInstancedStaticMeshComponent* MakeCloud(AS47Web* W, USceneComponent* Under, UMaterialInterface* Base, int32 Floats, TWeakObjectPtr<UMaterialInstanceDynamic>& Mid)
	{
		UInstancedStaticMeshComponent* I = NewObject<UInstancedStaticMeshComponent>(W);
		I->SetMobility(EComponentMobility::Movable);
		I->SetupAttachment(Under);
		I->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Base, W);
		I->SetMaterial(0, D);
		I->NumCustomDataFloats = Floats;
		I->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		I->SetCastShadow(false);
		I->bAffectDistanceFieldLighting = false;
		I->SetTranslucentSortPriority(3);   // (renderOrder 3: over the lamps' own glows)
		Mid = D;
		return I;
	}
}

FS47LifeActivity S47LifeActivity(double Clock, double Dawn)
{
	// the night cools from about 13 °C at midnight to 6 at dawn: fewer moths as it cools; the bats
	// hunt the lamps in the early night, are gone by half past three, and a few come back before dawn
	double H = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0) / 3600;
	if (H > 12) H -= 24;
	const double Temp = 16 - 1.2 * (H + 3), Day = Smoothstep(Dawn, 0.15, 0.45);
	FS47LifeActivity A;
	A.Moth = FMath::Clamp((Temp - 4.5) / 5, 0.15, 1.0) * (1 - Day);
	const double Back = Smoothstep(H, 4.4, 4.7) * (1 - Smoothstep(H, 5.0, 5.3));
	A.Bat = FMath::Max(1 - Smoothstep(H, 2.9, 3.6), 0.6 * Back) * (1 - Smoothstep(Dawn, 0.2, 0.5));
	A.Hour = H;
	return A;
}

double FS47NightLife::Rand()
{
	// rng: s = (s * 1664525 + 1013904223) >>> 0; s / 2^32
	Seed = Seed * 1664525u + 1013904223u;
	return Seed / 4294967296.0;
}

FS47NightLife::FArea* FS47NightLife::Find(const FString& Id) { return Areas.FindByPredicate([&Id](const FArea& A) { return A.Id == Id; }); }

void FS47NightLife::FMotes::Drift(double Dt, const FVector& V)
{
	// the air carries them on at this speed (m/s): changing it changes how they move, not where they are
	Carried += V * Dt;
	Carried = FVector(FMath::Fmod(FMath::Fmod(Carried.X, Box.X) + Box.X, Box.X), FMath::Fmod(FMath::Fmod(Carried.Y, Box.Y) + Box.Y, Box.Y), FMath::Fmod(FMath::Fmod(Carried.Z, Box.Z) + Box.Z, Box.Z));
	if (UMaterialInstanceDynamic* D = Mid.Get()) D->SetVectorParameterValue(TEXT("Drift"), FLinearColor(float(Carried.X), float(Carried.Y), float(Carried.Z), 0));
}

// ---------- boot ----------

void FS47NightLife::Build(US47Main* InM)
{
	M = InM;
	AS47Web* W = M ? M->Web.Get() : nullptr;
	Web = W;
	Seed = uint32((FDateTime::Now().GetTicks() / 10000) % 2147483647) ^ 0x4747u;
	WeedT = Rnd(70, 160); CarT = Rnd(30, 90);
	const TSharedPtr<FJsonObject> L = W ? Obj(W->LifeData(), TEXT("life")) : nullptr;
	if (!L) return;
	UMaterialInterface* MothBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebMoths.M_WebMoths"));
	UMaterialInterface* MoteBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebMotes.M_WebMotes"));
	const TSharedPtr<FJsonObject> As = Obj(L, TEXT("areas"));
	for (const TCHAR* Id : { TEXT("saro"), TEXT("road"), TEXT("station01") })
	{
		const TSharedPtr<FJsonObject> A = Obj(As, Id);
		if (!A) continue;
		FArea Ar;
		Ar.Id = Id;
		// the moths and bats round the area's lamps: one cloud, a quad each
		if (const TSharedPtr<FJsonObject> Mo = Obj(A, TEXT("moths")))
		{
			FMoths C;
			C.Node = int32(Num(Mo, TEXT("node"), -1));
			const TSharedPtr<FJsonObject> N = Obj(Mo, TEXT("n"));
			C.Low = int32(Num(N, TEXT("low"))); C.Moths = int32(Num(N, TEXT("moths"))); C.Bats = int32(Num(N, TEXT("bats"))); C.Resting = int32(Num(N, TEXT("resting")));
			const FS47Obj Pts(W, C.Node);
			for (const TSharedPtr<FJsonValue>& V : Arr(Mo, TEXT("lamps")))
			{
				const TSharedPtr<FJsonObject> O = V->AsObject();
				FLamp Lp;
				Lp.P = V3(Arr(O, TEXT("p")));
				Lp.W = Pts.LocalToWorld(Lp.P);
				const FVector Cl = V3(Arr(O, TEXT("col")));
				Lp.Col = FLinearColor(Cl.X, Cl.Y, Cl.Z);
				bool bBig = false; O->TryGetBoolField(TEXT("big"), bBig); Lp.bBig = bBig;
				Lp.H = Num(O, TEXT("h")); Lp.K0 = Num(O, TEXT("k0"), 1);
				const TArray<TSharedPtr<FJsonValue>>& G = Arr(O, TEXT("glow"));
				if (G.Num() == 2) { Lp.GlowNode = int32(G[0]->AsNumber()); Lp.GlowIndex = int32(G[1]->AsNumber()); }
				C.Lamps.Add(Lp);
				C.Reach += Lp.W;
				if (Lp.bBig) Ar.LampsW.Add(Lp.W);
			}
			// round all the lamps in world terms, 60 m out: farther off nothing of them shows
			C.Reach = C.Reach.ExpandBy(60);
			const TArray<float> Pos = Floats(Mo, TEXT("pos")), Sd = Floats(Mo, TEXT("seed")), Li = Floats(Mo, TEXT("lamp")), Ki = Floats(Mo, TEXT("kind")), Co = Floats(Mo, TEXT("col")), Wa = Floats(Mo, TEXT("wall"));
			const FS47WebNode* Nd = W->Node(C.Node);
			const int32 Count = Pos.Num() / 3;
			if (MothBase && Nd && Nd->Comp.IsValid() && Count > 0 && Sd.Num() >= Count * 4 && Wa.Num() >= Count * 4 && Co.Num() >= Count * 3)
			{
				UInstancedStaticMeshComponent* I = MakeCloud(W, Nd->Comp.Get(), MothBase, 14, C.Mid);
				TArray<FTransform> Tf;
				TArray<float> Cd;
				for (int32 k = 0; k < Count; ++k)
				{
					Tf.Add(FTransform(FQuat::Identity, S47Three::ToUE(Pos[k * 3], Pos[k * 3 + 1], Pos[k * 3 + 2]), FVector(0.001)));
					Cd.Append({ Sd[k * 4], Sd[k * 4 + 1], Sd[k * 4 + 2], Sd[k * 4 + 3], Li[k], Ki[k], Co[k * 3], Co[k * 3 + 1], Co[k * 3 + 2], Wa[k * 4], Wa[k * 4 + 1], Wa[k * 4 + 2], Wa[k * 4 + 3],
						// what Low leaves out: the moths after its two thirds (the bats Low leaves out by the material's Low)
						k >= C.Low && k < C.Moths ? 1.f : 0.f });
				}
				I->AddInstances(Tf, false, false, false);
				I->SetCustomData(0, Count - 1, Cd, false);
				I->RegisterComponent();
				I->SetVisibility(false);
				C.Mesh = I;
				const int32 Nl = FMath::Max(1, C.Lamps.Num());
				C.K.Init(1.f, Nl);
				UTexture2D* Kt = UTexture2D::CreateTransient(Nl, 1, PF_R32_FLOAT);
				Kt->SRGB = false; Kt->Filter = TF_Nearest; Kt->AddressX = TA_Clamp; Kt->AddressY = TA_Clamp;
				Kt->UpdateResource();
				C.KTex = Kt;
				if (UMaterialInstanceDynamic* D = C.Mid.Get()) { D->SetTextureParameterValue(TEXT("K"), Kt); D->SetScalarParameterValue(TEXT("Lamps"), float(Nl)); }
			}
			Ar.Moths = C;
		}
		// the far lights out on the land (if it sees the county road), its glow points
		if (const TSharedPtr<FJsonObject> F = Obj(A, TEXT("far")))
		{
			FFar Fr;
			Fr.Node = int32(Num(F, TEXT("node"), -1));
			for (const TSharedPtr<FJsonValue>& V : Arr(F, TEXT("win")))
			{
				const TSharedPtr<FJsonObject> O = V->AsObject();
				FWin Wn;
				Wn.I = int32(Num(O, TEXT("i"), -1)); Wn.Ph = Num(O, TEXT("ph")); Wn.P = V3(Arr(O, TEXT("p")));
				Wn.X = Num(O, TEXT("x")); Wn.Z = Num(O, TEXT("z")); Wn.Y = Num(O, TEXT("y")); Wn.Size = Num(O, TEXT("size"), 1);
				Wn.Col = S47Three::Hex(uint32(Num(O, TEXT("col"))));
				const TArray<TSharedPtr<FJsonValue>>& On = Arr(O, TEXT("on"));
				if (On.Num() == 2) { Wn.On0 = On[0]->AsNumber(); Wn.On1 = On[1]->AsNumber(); }
				Wn.K = Num(O, TEXT("k"), 1);
				bool bTv = false; O->TryGetBoolField(TEXT("tv"), bTv); Wn.bTv = bTv;
				Fr.Win.Add(Wn);
			}
			const TArray<TSharedPtr<FJsonValue>>& Car3 = Arr(F, TEXT("car"));
			for (int32 k = 0; k < 3 && k < Car3.Num(); ++k) Fr.Car[k] = int32(Car3[k]->AsNumber());
			Ar.Far = Fr;
		}
		// its tumbleweed (rolling now and then) and those lying about
		if (const TSharedPtr<FJsonObject> Wd = Obj(A, TEXT("weed")))
		{
			FWeed We;
			We.Node = int32(Num(Wd, TEXT("node"), -1)); We.BaseMat = int32(Num(Wd, TEXT("base"), -1));
			for (const TSharedPtr<FJsonValue>& V : Arr(Wd, TEXT("floods"))) We.FloodMats.Add(int32(V->AsNumber()));
			W->SetVisible(We.Node, false);
			Ar.Weed = We;
		}
		for (const TSharedPtr<FJsonValue>& V : Arr(A, TEXT("resting")))
		{
			const TSharedPtr<FJsonObject> O = V->AsObject();
			FResting R;
			R.Node = int32(Num(O, TEXT("node"), -1)); R.Centre = V3(Arr(O, TEXT("centre"))); R.Radius = Num(O, TEXT("radius"));
			Ar.Resting.Add(R);
		}
		Areas.Add(Ar);
	}
	// the dust: the flashlight's box holds the near part of its beam whichever way it points, the
	// headlights' the first thirty metres of theirs
	if (MoteBase)
	{
		MotesBuild(Flash, 900, 61, FVector(3.4, 2.6, 3.4), false);
		MotesBuild(Head, 3000, 62, FVector(34, 4.6, 34), true);
	}
	UE_LOG(LogS47Life2, Display, TEXT("S47: the life in the night: %d areas, moths %s, dust %s"), Areas.Num(), MothBase ? TEXT("yes") : TEXT("no"), MoteBase ? TEXT("yes") : TEXT("no"));
}

bool FS47NightLife::MotesBuild(FMotes& Mo, int32 N, uint32 InSeed, const FVector& Box, bool bBottom)
{
	// Motes: each mote's place in a box that follows the beam, and four numbers of its own (rng(seed))
	AS47Web* W = Web.Get();
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebMotes.M_WebMotes"));
	if (!W || !Base) return false;
	uint32 S = InSeed;
	auto R = [&S]() { S = S * 1664525u + 1013904223u; return S / 4294967296.0; };
	UInstancedStaticMeshComponent* I = MakeCloud(W, W->GetRootComponent(), Base, 7, Mo.Mid);
	TArray<FTransform> Tf;
	TArray<float> Cd;
	for (int32 k = 0; k < N; ++k)
	{
		// (the headlights' dust lies thicker near the ground)
		const double X = R(), Y = bBottom ? R() * R() : R(), Z = R();
		const float S0 = float(R()), S1 = float(R()), S2 = float(R()), S3 = float(R());
		Tf.Add(FTransform(FQuat::Identity, S47Three::ToUE((X - 0.5) * Box.X, (Y - 0.5) * Box.Y, (Z - 0.5) * Box.Z), FVector(0.001)));
		Cd.Append({ float(X), float(Y), float(Z), S0, S1, S2, S3 });
	}
	I->AddInstances(Tf, false, false, false);
	I->SetCustomData(0, N - 1, Cd, false);
	I->RegisterComponent();
	I->SetVisibility(false);
	Mo.Mesh = I; Mo.N = N; Mo.Box = Box;
	if (UMaterialInstanceDynamic* D = Mo.Mid.Get()) D->SetVectorParameterValue(TEXT("Box"), FLinearColor(float(Box.X), float(Box.Y), float(Box.Z), 0));
	return true;
}

// ---------- every frame ----------

void FS47NightLife::Update(double Dt, double InT, const FS47Cam& Cam, double Dawn, bool bQuiet)
{
	AS47Web* W = Web.Get();
	if (!W || !M) return;
	T = InT; Last = Cam; bHaveLast = true;
	Lines = LinesNow(M);
	const bool bLow = M->Flow && M->Flow->Quality == TEXT("low"), bOn = !bOff;
	const FString Area = M->World ? M->World->Area : FString(TEXT("saro"));
	const FString NewHere = Area == TEXT("saro") ? TEXT("saro") : Area == TEXT("road") || Area == TEXT("diner") ? TEXT("road") : Area == TEXT("station01") ? TEXT("station01") : FString();
	if (NewHere != Here) { for (FArea& A : Areas) if (A.Weed) WeedStop(*A.Weed); Here = NewHere; }
	Act = S47LifeActivity(M->Story ? M->Story->Clock() : 0, Dawn);
	const double Night = 1 - Smoothstep(Dawn, 0.12, 0.4);
	// World.airOut: the title's view, the cab, outdoors
	const bool bOut = M->Mode != US47Main::EMode::Play || (M->World && M->World->bDriving) || M->Space == TEXT("yard");
	// the moths and bats of the area shown, out in the open; its tumbleweeds lying about
	for (FArea& A : Areas)
	{
		if (A.Moths) MothsUpdate(*A.Moths, T, bOn && A.Id == Here && bOut, bLow, Cam.Pos);
		for (const FResting& R : A.Resting) W->SetVisible(R.Node, bOn && A.Id == Here && FVector::Dist(Cam.Pos, R.Centre) < R.Radius + 150);
	}
	// the far lights and the car on the county road
	CarStep(Dt, bQuiet, bOn && Night > 0.2);
	for (FArea& A : Areas) if (A.Far) FarUpdate(*A.Far, A.Id, bOn && A.Id == Here, Act.Hour, Night, Cam.Pos);
	WeedStep(Dt, Cam, bQuiet, bOn && Night > 0.3, !(M->World && M->World->bDriving), bOut);
	Beams(Dt, Cam, bOn, bLow, bOut);
}

void FS47NightLife::MothsUpdate(FMoths& Mo, double InT, bool bOn, bool bLow, const FVector& Cam)
{
	UInstancedStaticMeshComponent* I = Mo.Mesh.Get();
	if (!I) return;
	const bool bVis = bOn && Act.Moth > 0.01 && Mo.Lamps.Num() > 0 && Mo.Reach.IsInside(Cam);
	if (I->IsVisible() != bVis) I->SetVisibility(bVis);
	if (!bVis) { Mo.DrawnMoths = Mo.DrawnBats = 0; return; }
	const int32 Bats = !bLow && Act.Bat > 0.01 ? Mo.Bats : 0;
	Mo.DrawnMoths = bLow ? Mo.Low : Mo.Moths; Mo.DrawnBats = Bats;
	if (UMaterialInstanceDynamic* D = Mo.Mid.Get())
	{
		D->SetScalarParameterValue(TEXT("Time"), float(InT));
		D->SetScalarParameterValue(TEXT("MothK"), float(Act.Moth));
		D->SetScalarParameterValue(TEXT("BatK"), Bats ? float(Act.Bat) : 0.f);
		D->SetScalarParameterValue(TEXT("Low"), bLow ? 1.f : 0.f);
		D->SetScalarParameterValue(TEXT("Lines"), float(Lines));
	}
	// each lamp lit as long as its glow is
	AS47Web* W = Web.Get();
	bool bChanged = false;
	for (int32 i = 0; i < Mo.Lamps.Num(); ++i)
	{
		const FLamp& L = Mo.Lamps[i];
		const FS47GlowPoints* G = W ? W->Glow(L.GlowNode) : nullptr;
		const float K = G && G->Col.IsValidIndex(L.GlowIndex) ? float(FMath::Clamp(G->Col[L.GlowIndex].R / FMath::Max(L.K0, 1e-6), 0.0, 1.0)) : 1.f;
		if (Mo.K[i] != K) { Mo.K[i] = K; bChanged = true; }
	}
	UTexture2D* Kt = Mo.KTex.Get();
	if (!bChanged || !Kt || !Kt->GetPlatformData() || Kt->GetPlatformData()->Mips.Num() == 0) return;
	float* Px = static_cast<float*>(Kt->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
	for (int32 i = 0; i < Mo.K.Num(); ++i) Px[i] = Mo.K[i];
	Kt->GetPlatformData()->Mips[0].BulkData.Unlock();
	Kt->UpdateResource();
}

// ---------- the car on the county road ----------

void FS47NightLife::CarStep(double Dt, bool bQuiet, bool bCan)
{
	const FArea* A = Here.IsEmpty() ? nullptr : Find(Here);
	if (Car)
	{
		Car->Z += Car->Dir * Car->Speed * Dt;
		if ((Car->End - Car->Z) * Car->Dir <= 0 || !bCan) Car.Reset();
		return;
	}
	if (!bCan || bQuiet || !A || !A->Far) return;
	if ((CarT -= Dt) > 0) return;
	CarT = Rnd(200, 480);
	CarNow(false);
}

bool FS47NightLife::CarNow(bool bNear)
{
	// a car on the county road, coming past the camera's place from one end: from out of sight, or
	// (near: the tests, the developer menu) already in view
	const FArea* A = Here.IsEmpty() ? nullptr : Find(Here);
	if (!A || !A->Far || !bHaveLast) return false;
	const FVector C = Last.Pos;
	const double Mz = A->Id == TEXT("saro") ? C.Z + S47Geo::SARO_IN_ROAD.Z : C.Z - ROAD_ORIGIN.Z;
	const double Dir = Rand() < 0.5 ? 1 : -1;
	FCar Cr;
	Cr.Z = Mz - Dir * (bNear ? 700 : 2200); Cr.Dir = Dir; Cr.End = Mz + Dir * 2200; Cr.Speed = Rnd(20, 25);
	Car = Cr;
	return true;
}

double FS47NightLife::CarK() const
{
	// in and out of sight behind the swells of the land, faded at its ends
	const FCar& C = *Car;
	const double S = C.Z * C.Dir, N = FMath::Sin(S * 0.013) * FMath::Sin(S * 0.0071 + 1.3) + 0.4 * FMath::Sin(S * 0.031);
	return Smoothstep(N, -0.55, -0.1) * FMath::Min(1.0, FMath::Abs(C.End - C.Z) / 300);
}

void FS47NightLife::FarUpdate(FFar& F, const FString& Area, bool bOn, double Hour, double Night, const FVector& Cam)
{
	AS47Web* W = Web.Get();
	FS47GlowPoints* G = W ? W->Glow(F.Node) : nullptr;
	if (!G) return;
	const bool bVis = bOn && Night > 0.01;
	if (FS47Obj(W, F.Node).Visible() != bVis) W->SetVisible(F.Node, bVis);
	if (!bVis) return;
	const double GlowScale = Lines / (2 * FMath::Tan(FMath::DegreesToRadians(Last.Fov) / 2));
	auto Set = [G](int32 I, const FLinearColor& C, double K) { G->SetColor(I, FLinearColor(float(C.R * K), float(C.G * K), float(C.B * K))); };
	// a glow's size: its own in metres, but never under three and a half pixels across
	auto Size = [G, GlowScale](int32 I, double Mt, double D)
	{
		if (!G->Size.IsValidIndex(I)) return;
		const double S = FMath::Max(Mt, 3.5 * D / GlowScale);
		if (FMath::Abs(G->Size[I] - S) > 0.05 * S) G->SetSize(I, float(S));
	};
	// a light this far off: dimmer and redder (the air near the ground), twinkling a little more
	auto Far = [this, &Set](int32 I, const FLinearColor& C, double K, double D, double Ph)
	{
		const double Fk = Smoothstep(D, 900, 1900), Amp = 0.04 + 0.16 * Smoothstep(D, 300, 1800);
		const double Tw = 1 + Amp * (0.6 * FMath::Sin(T * 1.9 + Ph) * FMath::Sin(T * 0.73 + Ph * 1.7) + 0.4 * FMath::Sin(T * 3.1 + Ph * 2.3));
		Set(I, FLinearColor(C.R, float(C.G * (1 - 0.12 * Fk)), float(C.B * (1 - 0.3 * Fk))), K * (1 - 0.35 * Fk) * Tw);
	};
	for (const FWin& Wn : F.Win)
	{
		const double Lit = (Hour >= Wn.On0 && Hour < Wn.On1 ? 1 : 0) * Night * Wn.K;
		const double D = FMath::Sqrt(FMath::Square(Wn.P.X - Cam.X) + FMath::Square(Wn.P.Z - Cam.Z));
		// (a television: its light jumps with the pictures, and is greyer between them)
		const double Tv = Wn.bTv ? 0.7 + 0.3 * FMath::Sin(T * 2.3 + Wn.Ph) * FMath::Sin(T * 5.1) + 0.15 * FMath::Sign(FMath::Sin(T * 0.37 + Wn.Ph)) : 1;
		Far(Wn.I, Wn.Col, Lit * Tv, D, Wn.Ph);
		Size(Wn.I, Wn.Size, D);
	}
	const FVector Off(0, -999, 0);
	if (!Car) { if (!F.bGone) for (int32 i = 0; i < 3; ++i) G->SetPosition(F.Car[i], Off); F.bGone = true; return; }
	F.bGone = false;
	const double Z = Car->Z, X = COUNTY(Z), Dx = COUNTY(Z + Car->Dir) - X, H = FMath::Sqrt(Dx * Dx + Car->Dir * Car->Dir);
	const double Fx = Dx / H, Fz = Car->Dir / H;
	// in the area's own terms (the map's are the road's), on its land
	const FVector P = Area == TEXT("saro") ? FVector(X - S47Geo::SARO_IN_ROAD.X, 0, Z - S47Geo::SARO_IN_ROAD.Z) : FVector(X + ROAD_ORIGIN.X, 0, Z + ROAD_ORIGIN.Z);
	double Y;
	if (Area == TEXT("saro"))
	{
		const double Sx = X - S47Geo::SARO_IN_ROAD.X, Sz = Z - S47Geo::SARO_IN_ROAD.Z, Fr = FMath::Sqrt(Sx * Sx + Sz * Sz);
		Y = Fr > 300 ? -2 + FMath::Sin(Sx * 0.004 + Sz * 0.003) * 6 * FMath::Min(1.0, (Fr - 300) / 600) : -0.62;
	}
	else Y = S47RoadTerrain::SurfaceY(X, Z) + ROAD_ORIGIN.Y;
	// the lamps as seen from here: headlights coming, tail lights going, a little of both side on
	const double Tx = Cam.X - P.X, Tz = Cam.Z - P.Z;
	double D = FMath::Sqrt(Tx * Tx + Tz * Tz);
	if (D == 0) D = 1;
	const double Face = (Fx * Tx + Fz * Tz) / D, FarK = 1 - Smoothstep(D, 1700, 2100);
	G->SetPosition(F.Car[0], FVector(P.X, Y + 0.9, P.Z));
	G->SetPosition(F.Car[1], FVector(P.X - Fx * 4, Y + 0.9, P.Z - Fz * 4));
	G->SetPosition(F.Car[2], FVector(P.X + Fx * 30, Y + 0.6, P.Z + Fz * 30));
	const double K = CarK() * FarK * Night;
	Far(F.Car[0], HEAD, K * (0.18 + 0.82 * Smoothstep(Face, -0.35, 0.9)), D, 3.1);
	Far(F.Car[1], TAIL, K * 0.7 * Smoothstep(-Face, -0.2, 0.9), D, 7.7);
	Size(F.Car[0], 8, D); Size(F.Car[1], 5, D);
	Set(F.Car[2], POOL, K);
}

// ---------- tumbleweeds ----------

double FS47NightLife::Ground(const FString& Area, double X, double Z) const
{
	if (Area == TEXT("saro")) return US47Night::SaroFloorAt(Web.Get(), X, Z);
	if (Area == TEXT("station01")) return S47Legs::StationGroundAt(X, Z);
	return S47RoadTerrain::SurfaceY(X - ROAD_ORIGIN.X, Z - ROAD_ORIGIN.Z) + ROAD_ORIGIN.Y;
}

bool FS47NightLife::OnHighway(const FString& Area, double X, double Z) const
{
	(void)Z;
	if (Area == TEXT("saro")) return FMath::Abs(X + S47Geo::SARO_IN_ROAD.X) < 5;
	if (Area == TEXT("road")) return FMath::Abs(X - ROAD_ORIGIN.X) < 5;
	return false;
}

bool FS47NightLife::Blocked(const FString& Area, double X, double Z) const
{
	// what stands in a tumbleweed's way: a wall, a fence, a parked truck (World.ts addArea's blocked)
	AS47Web* W = Web.Get();
	if (!W) return false;
	auto InBoxes = [X, Z](const TArray<FS47Rect>& Bx) { for (const FS47Rect& B : Bx) if (X > B.MinX - 0.4 && X < B.MaxX + 0.4 && Z > B.MinZ - 0.4 && Z < B.MaxZ + 0.4) return true; return false; };
	if (Area == TEXT("saro"))
	{
		// (the fences stop it too: the array's chain-link, and the ranch fence on the south and east lines)
		const bool bFenced = (FMath::Abs(Z - -20) < 0.35 && X > -80 && X < 60 && !(X > -28.6 && X < -19.4))
			|| (FMath::Abs(Z - 34.5) < 0.35 && X > -18.6 && X < 46.5) || (FMath::Abs(X - 46.5) < 0.35 && Z > -19.9 && Z < 34.5);
		return InBoxes(W->CollidersMutable(TEXT("saro"))) || bFenced;
	}
	if (Area == TEXT("station01")) return InBoxes(W->CollidersMutable(TEXT("station01")));
	const TSharedPtr<FJsonObject> L = W->DriveData().IsValid() ? W->DriveData() : nullptr;
	const TSharedPtr<FJsonObject>* Legs = nullptr, *Road = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* Obs = nullptr;
	if (L && L->TryGetObjectField(TEXT("legs"), Legs) && (*Legs)->TryGetObjectField(TEXT("road"), Road) && (*Road)->TryGetArrayField(TEXT("obstacles"), Obs))
	{
		for (const TSharedPtr<FJsonValue>& V : *Obs)
		{
			const TArray<TSharedPtr<FJsonValue>>& O = V->AsArray();
			if (O.Num() == 3 && FMath::Sqrt(FMath::Square(X - O[0]->AsNumber()) + FMath::Square(Z - O[1]->AsNumber())) < O[2]->AsNumber() + 0.4) return true;
			if (O.Num() == 4 && X > O[0]->AsNumber() - 0.4 && X < O[1]->AsNumber() + 0.4 && Z > O[2]->AsNumber() - 0.4 && Z < O[3]->AsNumber() + 0.4) return true;
		}
	}
	return InBoxes(W->CollidersMutable(TEXT("road")));
}

void FS47NightLife::WeedStop(FWeed& W)
{
	W.bOn = false;
	if (AS47Web* Wb = Web.Get()) Wb->SetVisible(W.Node, false);
}

void FS47NightLife::WeedPlace(FWeed& W, const FString& Area)
{
	FVector P = W.From + W.Dir * W.S;
	// a wobble across the way, as the wind veers
	P += W.Axis * (0.6 * FMath::Sin(W.S * 0.21) * FMath::Min(1.0, W.S / 6));
	const double Hop = W.Hop.H > 0 ? W.Hop.H * FMath::Sin(UE_DOUBLE_PI * W.Hop.Phase) : 0;
	P.Y = Ground(Area, P.X, P.Z) + WEED_R * 0.85 + Hop;
	W.At = P;
	if (AS47Web* Wb = Web.Get()) Wb->SetLocal(W.Node, P, W.Q, FVector::OneVector);
}

void FS47NightLife::WeedUpdate(FWeed& W, const FString& Area, double Dt, bool bSeen)
{
	if (!W.bOn) return;
	if (W.S >= W.Len)
	{
		// it has come to rest (against a fence, a wall, or the gust has died); gone once unseen
		W.Rest += Dt;
		if (W.Rest > 25 && !bSeen) WeedStop(W);
		return;
	}
	// the gust: up to speed, rolling, and dying away at the end of the way
	const double U = W.S / W.Len, V = W.Top * FMath::Max(0.12, FMath::Pow(FMath::Sin(UE_DOUBLE_PI * FMath::Min(0.999, U * 0.92 + 0.04)), 0.6));
	const double Ds = V * Dt;
	W.S += Ds;
	// a hop over a stone now and then
	if ((W.Hop.Next -= Ds) <= 0) { W.Hop.H = Rnd(0.15, 0.6); W.Hop.Phase = 0; W.Hop.Next = Rnd(2, 8); }
	if (W.Hop.H > 0) { W.Hop.Phase += Ds / (0.9 + W.Hop.H * 2.5); if (W.Hop.Phase >= 1) W.Hop.H = 0; }
	W.Q = QMul(AxisAngle(W.Axis, Ds / WEED_R), W.Q);
	W.Spin += Ds / WEED_R;
	WeedPlace(W, Area);
}

void FS47NightLife::WeedStep(double Dt, const FS47Cam& Cam, bool bQuiet, bool bCan, bool bFoot, bool bOut)
{
	FS47Truck* Tk = M->World && M->World->bDriving ? M->World->CabTruck() : nullptr;
	for (FArea& A : Areas)
	{
		if (!A.Weed || !A.Weed->bOn) continue;
		FWeed& W = *A.Weed;
		if (A.Id != Here || bOff) { WeedStop(W); continue; }
		const FVector D = W.At - Cam.Pos;
		const bool bSeen = D.Size() < 80 && FVector::DotProduct(D.GetSafeNormal(), Cam.Forward) > 0.45;
		WeedUpdate(W, A.Id, Dt, bSeen);
		// the truck runs it down: it goes under
		if (Tk && W.bOn)
		{
			const FVector Tp = Tk->Group.WorldPosition();
			if (FMath::Sqrt(FMath::Square(W.At.X - Tp.X) + FMath::Square(W.At.Z - Tp.Z)) < 3.2) WeedStop(W);
		}
	}
	if (!bCan || bQuiet || Here.IsEmpty() || !bOut || (!bFoot && !Tk)) return;
	if ((WeedT -= Dt) > 0) return;
	WeedT = TumbleNow() ? Rnd(180, 420) : Rnd(25, 50);
}

bool FS47NightLife::TumbleNow(bool bAnywhere)
{
	FArea* A = Here.IsEmpty() ? nullptr : Find(Here);
	AS47Web* Wb = Web.Get();
	if (!bHaveLast || !A || !A->Weed || !Wb) return false;
	const FVector Cam = Last.Pos;
	const FVector F = FVector(Last.Forward.X, 0, Last.Forward.Z).GetSafeNormal();
	FS47Truck* Tk = M->World && M->World->bDriving ? M->World->CabTruck() : nullptr;
	FS47DriveController* Dc = Tk ? M->World->CabDrive() : nullptr;
	for (int32 k = 0; k < 8; ++k)
	{
		FVector Q, Dir;
		double Back, Len;
		if (Tk && Dc)
		{
			// across the road ahead of the truck, so it is over before the truck comes to it
			const FVector H = FVector(Tk->Group.WorldQuaternion().RotateVector(FVector(0, 0, -1)).X, 0, Tk->Group.WorldQuaternion().RotateVector(FVector(0, 0, -1)).Z).GetSafeNormal();
			Q = Tk->Group.WorldPosition() + H * (30 + FMath::Max(0.0, Dc->Speed) * 2.6 + k * 4);
			if (!OnHighway(A->Id, Q.X, Q.Z) && Dc->Surface.Kind == ES47SurfaceKind::Asphalt) continue;
			Dir = FVector(-H.Z, 0, H.X);
			if (FVector::DotProduct(Dir, WIND) < 0) Dir = -Dir;
			Dir = (Dir + (WIND - Dir) * 0.25).GetSafeNormal();
			Back = 13; Len = 34;
		}
		else
		{
			Q = Cam + F * Rnd(10, 26) + FVector(-F.Z, 0, F.X) * Rnd(-7, 7);
			bool bLamp = false;
			for (const FVector& L : A->LampsW) bLamp |= FMath::Sqrt(FMath::Square(L.X - Q.X) + FMath::Square(L.Z - Q.Z)) < 13;
			const bool bTorch = M->Night && M->Night->Flashlight.IsValid() && M->Night->Flashlight->Intensity() > 1 && FVector::Dist(Cam, Q) < 15;
			if (!bLamp && !bTorch && !bAnywhere) continue;
			Dir = AboutUp(WIND, Rnd(-0.4, 0.4));
			Back = 26; Len = 55;
		}
		// what stands in its way: it comes from behind nothing, and stops against whatever it meets
		double B = 0, E = 0;
		while (B < Back && !Blocked(A->Id, Q.X - Dir.X * (B + 1), Q.Z - Dir.Z * (B + 1))) B += 1;
		if (Blocked(A->Id, Q.X, Q.Z) || B < Back * 0.6) continue;
		while (E < Len - Back && !Blocked(A->Id, Q.X + Dir.X * (E + 1), Q.Z + Dir.Z * (E + 1))) E += 1;
		FWeed& W = *A->Weed;
		// start(from, dir, len, top)
		W.From = Q - Dir * B; W.Dir = FVector(Dir.X, 0, Dir.Z).GetSafeNormal();
		W.S = 0; W.Len = B + E; W.Top = Rnd(4.5, 6.5); W.Rest = 0; W.bOn = true; W.Spin = 0;
		W.Axis = FVector::CrossProduct(FVector(0, 1, 0), W.Dir).GetSafeNormal();
		W.Hop = FWeed::FHop(); W.Hop.Next = Rnd(2, 6);
		W.Q = S47Three::Euler(Rand() * 6.283, Rand() * 6.283, Rand() * 6.283);
		// the material for the place: lit by the lamp set that lights it
		int32 Mat = W.BaseMat;
		if (A->Id == TEXT("saro") && W.FloodMats.Num() > 0 && S47Geo::InRect(S47Geo::MOTEL_RECT(), W.From.X, W.From.Z, -12)) Mat = W.FloodMats[0];
		if (A->Id == TEXT("road"))
		{
			const auto& Tn = S47RoadShape::TOWN();
			const FVector DinerAt(8000 + S47RoadShape::DINER().X, 0, S47RoadShape::DINER().Z);
			if (W.FloodMats.Num() > 0 && W.From.X > ROAD_ORIGIN.X + Tn.West && W.From.X < ROAD_ORIGIN.X + Tn.East && W.From.Z > Tn.Z0 && W.From.Z < Tn.Z1) Mat = W.FloodMats[0];
			else if (W.FloodMats.Num() > 1 && FMath::Sqrt(FMath::Square(W.From.X - DinerAt.X) + FMath::Square(W.From.Z - DinerAt.Z)) < 40) Mat = W.FloodMats[1];
		}
		if (const FS47WebNode* N = Wb->Node(W.Node))
		{
			if (UStaticMeshComponent* Mc = Cast<UStaticMeshComponent>(N->Comp.Get())) if (UMaterialInterface* Mi = Wb->MatAsset(Mat)) Mc->SetMaterial(0, Mi);
		}
		Wb->SetVisible(W.Node, true);
		WeedPlace(W, A->Id);
		WeedArea = Here;
		return true;
	}
	return false;
}

// ---------- dust in the beams ----------

void FS47NightLife::Beams(double Dt, const FS47Cam& Cam, bool bOn, bool bLow, bool bOut)
{
	// the flashlight's: a box round the near part of its beam, where the dust shows
	USpotLightComponent* L = M->Night && M->Night->Flashlight.IsValid() ? M->Night->Flashlight->Light() : nullptr;
	const double Full = 40 * 1.6;   // (L.userData.full: 40 FLASH_GAIN with the cookie)
	const double K = L ? M->Night->Flashlight->Intensity() / Full : 0;
	auto Show = [](FMotes& Mo, bool bV) { if (Mo.Mesh.IsValid() && Mo.Mesh->IsVisible() != bV) Mo.Mesh->SetVisibility(bV); };
	auto V4 = [](const FVector& V) { return FLinearColor(float(V.X), float(V.Y), float(V.Z), 0); };
	Show(Flash, bOn && K > 0.01);
	if (bOn && K > 0.01 && Flash.Mid.IsValid())
	{
		UMaterialInstanceDynamic* D = Flash.Mid.Get();
		const FVector Center = Cam.Pos + Cam.Forward * 1.6;
		Flash.Mesh->SetWorldLocation(S47Three::ToUE(Center));
		D->SetScalarParameterValue(TEXT("Time"), float(T));
		D->SetScalarParameterValue(TEXT("Lines"), float(Lines));
		D->SetVectorParameterValue(TEXT("Center"), V4(Center));
		D->SetVectorParameterValue(TEXT("BeamPos0"), V4(S47Three::ToThree(L->GetComponentLocation())));
		const FVector Fd = L->GetForwardVector();
		D->SetVectorParameterValue(TEXT("BeamDir0"), V4(FVector(Fd.X, Fd.Z, Fd.Y)));
		D->SetVectorParameterValue(TEXT("BeamK"), FLinearColor(float(K), 0, 0, 0));
		// (the flashlight's beam inside its cookie's corona, core/Flashlight.ts userData.cone)
		const double Beam = FMath::Atan(0.85 * FMath::Tan(0.4));
		D->SetVectorParameterValue(TEXT("Cos"), FLinearColor(float(FMath::Cos(Beam * 1.12)), float(FMath::Cos(Beam * 0.6)), 0, 0));
		D->SetScalarParameterValue(TEXT("Fall"), 0.12f); D->SetScalarParameterValue(TEXT("Size"), 0.02f); D->SetScalarParameterValue(TEXT("Head"), 0.f); D->SetScalarParameterValue(TEXT("Near"), 0.8f);
		D->SetScalarParameterValue(TEXT("Puff"), 2.f);
		// out in the open a breeze carries it (and a few gnats); indoors it hangs in the air
		if (bOut) { Flash.Drift(Dt, WIND * 0.35); D->SetScalarParameterValue(TEXT("Wobble"), 0.05f); D->SetScalarParameterValue(TEXT("Density"), bLow ? 0.375f : 0.75f); }
		else { Flash.Drift(Dt, FVector(0.01, 0.006, -0.004)); D->SetScalarParameterValue(TEXT("Wobble"), 0.09f); D->SetScalarParameterValue(TEXT("Density"), bLow ? 0.5f : 1.f); }
		D->SetVectorParameterValue(TEXT("Col"), FLinearColor(1.6f, 0.9f * 1.6f, 0.75f * 1.6f, 0));
	}
	// the truck's headlights: on gravel and dirt fine specks of the dust it raises; when it slows and
	// stops the cloud it trails catches up with it and hangs in the near part of the beam, and settles
	FS47Truck* Tk = M->World && M->World->bDriving ? M->World->CabTruck() : nullptr;
	FS47DriveController* Dc = Tk ? M->World->CabDrive() : nullptr;
	const bool bLoose = Tk && Dc && Dc->Surface.Kind != ES47SurfaceKind::Asphalt;
	const double Sp = Tk && Dc ? FMath::Abs(Dc->Speed) : 0;
	const double Want = bLoose ? FMath::Clamp(Sp / 10, 0.0, 1.0) : 0;
	Specks += (Want - Specks) * FMath::Min(1.0, Dt * (Want > Specks ? 0.8 : 0.3));
	const double Slowing = Dt > 0 && Tk ? FMath::Max(0.0, LastSpeed - Sp) : 0;
	LastSpeed = Sp;
	Cloud = FMath::Clamp(Cloud + (bLoose ? Slowing * 0.11 * FMath::Min(1.0, Specks * 1.5 + 0.3) : 0) - Dt * 0.09 * (0.4 + Cloud), 0.0, 1.0);
	if (!Tk) Cloud = 0;
	const bool bHead = bOn && Tk && Tk->Beam() > 0.01;
	Show(Head, bHead);
	if (bHead && Head.Mid.IsValid())
	{
		UMaterialInstanceDynamic* D = Head.Mid.Get();
		const FQuat Gq = Tk->Group.WorldQuaternion();
		const FVector Gp = Tk->Group.WorldPosition(), Fw = Gq.RotateVector(FVector(0, 0, -1));
		FVector Center = Gp + Fw * 15; Center.Y = Gp.Y + 1.9;
		Head.Mesh->SetWorldLocation(S47Three::ToUE(Center));
		D->SetScalarParameterValue(TEXT("Time"), float(T));
		D->SetScalarParameterValue(TEXT("Lines"), float(Lines));
		D->SetVectorParameterValue(TEXT("Center"), V4(Center));
		const FVector Down = Gq.RotateVector(FVector(0, -0.05, -1).GetSafeNormal());
		D->SetVectorParameterValue(TEXT("BeamPos0"), V4(Tk->Group.LocalToWorld(FVector(-0.62, 0.85, -2.45))));
		D->SetVectorParameterValue(TEXT("BeamPos1"), V4(Tk->Group.LocalToWorld(FVector(0.62, 0.85, -2.45))));
		D->SetVectorParameterValue(TEXT("BeamDir0"), V4(Down));
		D->SetVectorParameterValue(TEXT("BeamDir1"), V4(Down));
		D->SetVectorParameterValue(TEXT("BeamK"), FLinearColor(float(Tk->Beam()), float(Tk->Beam()), 0, 0));
		// (the pool on the road is about its width a dozen metres out: a cone of 12 degrees)
		D->SetVectorParameterValue(TEXT("Cos"), FLinearColor(float(FMath::Cos(0.21)), float(FMath::Cos(0.09)), 0, 0));
		D->SetScalarParameterValue(TEXT("Head"), 1.f); D->SetScalarParameterValue(TEXT("Near"), 0.8f); D->SetScalarParameterValue(TEXT("Fall"), 0.004f);
		const double C = Cloud, S = Specks;
		const double Dens = FMath::Max(0.04, 0.22 * S + 0.7 * C);
		D->SetScalarParameterValue(TEXT("Density"), float(bLow ? Dens * 0.5 : Dens));
		D->SetScalarParameterValue(TEXT("Size"), 0.03f);
		D->SetScalarParameterValue(TEXT("Puff"), float(1 - 0.16 * C)); D->SetScalarParameterValue(TEXT("PuffSize"), 2.f); D->SetScalarParameterValue(TEXT("PuffA"), 0.13f);
		D->SetScalarParameterValue(TEXT("Wobble"), float(0.12 + 0.2 * C));
		FVector Dv = WIND * 0.6 + Fw * (1.2 * C); Dv.Y = 0.1 * C;
		Head.Drift(Dt, Dv);
		D->SetVectorParameterValue(TEXT("Col"), FLinearColor(float(1 - 0.3 * C), float(0.9 * (1 - 0.3 * C)), float(0.72 * (1 - 0.3 * C)), 0));
	}
}

FString FS47NightLife::Stats() const
{
	FString S;
	for (const FArea& A : Areas)
	{
		S += FString::Printf(TEXT("%s: moths %d bats %d"), *A.Id, A.Moths ? A.Moths->DrawnMoths : 0, A.Moths ? A.Moths->DrawnBats : 0);
		if (A.Weed) S += FString::Printf(TEXT(" weed %s"), A.Weed->bOn ? TEXT("rolling") : TEXT("-"));
		S += TEXT("; ");
	}
	S += FString::Printf(TEXT("car %s; dust flash %s head %s"), Car ? TEXT("on the county road") : TEXT("-"),
		Flash.Mesh.IsValid() && Flash.Mesh->IsVisible() ? TEXT("on") : TEXT("off"), Head.Mesh.IsValid() && Head.Mesh->IsVisible() ? TEXT("on") : TEXT("off"));
	return S;
}

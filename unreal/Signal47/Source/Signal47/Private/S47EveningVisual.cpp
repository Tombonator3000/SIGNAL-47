#include "S47EveningVisual.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Figures.h"
#include "S47Main.h"
#include "S47Seen.h"
#include "S47Three.h"
#include "S47Truck.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"

namespace
{
	const FVector SERVICE_PARK(8.9, 0, 8);
	const TCHAR* WAGON = TEXT("/Game/Free/Vehicles/fairheaven_sw84/StaticMeshes/fairheaven_sw84.fairheaven_sw84");
	const TCHAR* PICKUP = TEXT("/Game/Free/Vehicles/pickup_c10_1966/StaticMeshes/pickup_c10_1966.pickup_c10_1966");

	UMaterialInstanceDynamic* Paint(FS47Group& G, uint32 Color, float Roughness = 0.7f)
	{
		UMaterialInstanceDynamic* Mat = G.Basic(S47Three::Hex(Color), TEXT("M_Web_Lit_Opaque"));
		if (Mat) Mat->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		return Mat;
	}
	void Shadows(FS47Group& G)
	{
		if (!G.Root().C.IsValid()) return;
		TArray<USceneComponent*> Children;
		G.Root().C->GetChildrenComponents(true, Children);
		for (USceneComponent* C : Children) if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(C))
		{
			Mesh->SetCastShadow(true);
			Mesh->bAffectDistanceFieldLighting = true;
		}
	}
	UStaticMeshComponent* Vehicle(AS47Web* W, const TCHAR* Asset, const TCHAR* Label)
	{
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Asset);
		if (!Mesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("S47: evening %s unavailable: %s; import existing credited Free vehicles"), Label, Asset);
			return nullptr;
		}
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W);
		C->SetupAttachment(W->GetRootComponent());
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Mesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(true);
		C->SetVisibility(false);
		C->RegisterComponent();
		return C;
	}
	void Pose(UStaticMeshComponent* C, const FVector& P, double Heading)
	{
		if (!C) return;
		double E[16];
		S47Three::Compose(P, S47Three::Euler(0, Heading, 0), FVector::OneVector, E);
		C->SetWorldTransform(S47Three::MatToUE(E));
		// Credited Free vehicles are centimetres, length along X; the driving frame faces -Z.
		C->AddLocalRotation(FRotator(0, -90, 0));
	}
	void GroupPose(FS47Group& G, const FVector& P, const FVector& Rotation)
	{
		G.Root().Position = P; G.Root().Rotation = Rotation; G.Root().Apply();
	}
	void Visible(FS47Node* N, bool bOn) { if (N) N->bVisible = bOn; }

	void Food(FS47Group& G, FS47Node*& Burger, FS47Node*& Sandwich, FS47Node*& Cup, FS47Node** PlateOut = nullptr)
	{
		auto* White = Paint(G, 0xe9e2cd, 0.35f);
		auto* Bread = Paint(G, 0xc79752);
		auto* Meat = Paint(G, 0x503020);
		auto* Leaf = Paint(G, 0x678746);
		auto* Tomato = Paint(G, 0xb94730);
		auto* Coffee = Paint(G, 0x27160e, 0.18f);
		// Plate and a short rolled napkin. All pieces use native lit material and cast shadows.
		FS47Node* Plate = G.Group(nullptr);
		G.Sphere(Plate, 0.16, White, FVector(0, 0.012, 0), FVector(1, 0.06, 1));
		G.Box(Plate, 0.06, 0.008, 0.16, White, FVector(-0.22, 0.005, 0));
		if (PlateOut) *PlateOut = Plate;
		Burger = G.Group(nullptr);
		G.Sphere(Burger, 0.095, Bread, FVector(0, 0.036, 0), FVector(1, 0.24, 1));
		G.Sphere(Burger, 0.09, Meat, FVector(0, 0.062, 0), FVector(1, 0.15, 1));
		G.Box(Burger, 0.19, 0.007, 0.17, Leaf, FVector(0, 0.078, 0), FVector(0, 0.3, 0));
		G.Sphere(Burger, 0.08, Tomato, FVector(0, 0.085, 0), FVector(1, 0.07, 1));
		G.Sphere(Burger, 0.098, Bread, FVector(0, 0.114, 0), FVector(1, 0.36, 1));
		Sandwich = G.Group(nullptr);
		G.Box(Sandwich, 0.17, 0.026, 0.17, Bread, FVector(0, 0.038, 0));
		G.Box(Sandwich, 0.18, 0.007, 0.18, Leaf, FVector(0, 0.056, 0), FVector(0, 0.12, 0));
		G.Box(Sandwich, 0.16, 0.017, 0.16, Meat, FVector(0, 0.068, 0));
		G.Box(Sandwich, 0.17, 0.026, 0.17, Bread, FVector(0, 0.090, 0));
		Cup = G.Group(nullptr, FVector(0.26, 0, 0));
		G.Capsule(Cup, 0.042, 0.05, White, FVector(0, 0.065, 0), FVector::ZeroVector, FVector(1, 1, 1));
		G.Sphere(Cup, 0.039, Coffee, FVector(0, 0.106, 0), FVector(1, 0.05, 1));
		G.Box(Cup, 0.022, 0.06, 0.02, White, FVector(0.059, 0.065, 0));
		G.Box(Cup, 0.05, 0.015, 0.02, White, FVector(0.038, 0.092, 0));
		G.Box(Cup, 0.05, 0.015, 0.02, White, FVector(0.038, 0.038, 0));
		Shadows(G);
	}
}

FS47EveningVisual::FS47EveningVisual(US47Main* InMain) : M(InMain) {}
FS47EveningVisual::~FS47EveningVisual()
{
	if (Personal.IsValid()) Personal->DestroyComponent();
	if (Service.IsValid()) Service->DestroyComponent();
}

void FS47EveningVisual::Build()
{
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (bBuilt || !W || !W->IsLoaded()) return;
	bBuilt = true;
	Personal = Vehicle(W, WAGON, TEXT("personal station wagon"));
	Service = Vehicle(W, PICKUP, TEXT("parked service pickup"));
	Pose(Service.Get(), SERVICE_PARK, UE_DOUBLE_PI);
	Hood = MakeUnique<FS47Group>(W, TEXT("EveningPersonalHood"));
	// The driving controller and dashboard are shared. This hood replaces SARO's identifiable body.
	auto* Metal = Paint(*Hood, 0x77816d, 0.4f);
	auto* Chrome = Paint(*Hood, 0xaaa798, 0.25f);
	Hood->Box(nullptr, 1.70, 0.08, 1.45, Metal, FVector(0, 0.96, -1.65), FVector(-0.035, 0, 0));
	Hood->Box(nullptr, 1.77, 0.08, 0.06, Chrome, FVector(0, 0.94, -2.40));
	Hood->Box(nullptr, 0.025, 0.018, 1.3, Chrome, FVector(0, 1.012, -1.63));
	Shadows(*Hood); Hood->AddToScene(); Hood->SetVisible(false);
	CounterFood = MakeUnique<FS47Group>(W, TEXT("EveningDinerOrder"));
	Food(*CounterFood, CounterBurger, CounterSandwich, CounterCup);
	CounterWater = CounterFood->Group(nullptr, FVector(0.27, 0, -0.18));
	auto* Glass = Paint(*CounterFood, 0x9ebac0, 0.12f);
	auto* Water = Paint(*CounterFood, 0xc8e4e7, 0.1f);
	CounterFood->Capsule(CounterWater, 0.037, 0.07, Glass, FVector(0, 0.074, 0));
	CounterFood->Sphere(CounterWater, 0.034, Water, FVector(0, 0.111, 0), FVector(1, 0.06, 1));
	Shadows(*CounterFood);
	CounterFood->AddToScene(); CounterFood->SetVisible(false);
	HeldFood = MakeUnique<FS47Group>(W, TEXT("EveningFoodInHand"));
	Food(*HeldFood, HeldBurger, HeldSandwich, HeldCup, &HeldPlate);
	HeldBag = HeldFood->Group(nullptr);
	auto* Paper = Paint(*HeldFood, 0xb49a71);
	auto* Fold = Paint(*HeldFood, 0x8f7755);
	HeldFood->Box(HeldBag, 0.22, 0.29, 0.16, Paper, FVector(0, 0.14, 0));
	HeldFood->Box(HeldBag, 0.225, 0.045, 0.055, Fold, FVector(0, 0.297, 0));
	HeldFood->Box(HeldBag, 0.14, 0.045, 0.003, Paint(*HeldFood, 0xe9dfbd), FVector(0, 0.18, -0.081));
	// Sleeve and hand remain below the platter/bag, with no invented character asset.
	HeldFood->Box(nullptr, 0.09, 0.065, 0.22, Paint(*HeldFood, 0x424c45), FVector(0.025, -0.065, 0.10));
	HeldFood->Sphere(nullptr, 0.055, Paint(*HeldFood, 0xaf8566), FVector(0, -0.024, 0.006), FVector(1.2, 0.55, 1));
	Shadows(*HeldFood); HeldFood->AddToScene(); HeldFood->SetVisible(false);
	UE_LOG(LogTemp, Display, TEXT("S47: evening visual built: personal wagon %s, separate SARO pickup %s, lit order/hand props"), Personal.IsValid() ? TEXT("ready") : TEXT("MISSING"), Service.IsValid() ? TEXT("ready") : TEXT("MISSING"));
}

void FS47EveningVisual::ParkPersonal(const FVector& Position, double Heading)
{
	PersonalPark = Position;
	PersonalHeading = Heading;
	// Parking on the old trigger pad must not leave both cars occupying SARO 07's service bay.
	if (FVector2D(PersonalPark.X - SERVICE_PARK.X, PersonalPark.Z - SERVICE_PARK.Z).Size() < 4.5)
		PersonalPark = FVector(2, 0, 8);
	bParked = true;
}

void FS47EveningVisual::SetParked(bool bOn)
{
	bParked = bOn;
	if (!bOn) { PersonalPark = FVector(2, 0, 8); PersonalHeading = UE_DOUBLE_PI; }
}

void FS47EveningVisual::Tick(bool bCommute, bool bDriving, FS47Truck* Carrier, bool bArrived,
	bool bFoodBought, bool bFoodTaken, bool bFoodEaten, bool bWater, bool bBurger)
{
	Build();
	if (!bBuilt || !M || !M->World || !M->Player) return;
	const FString Area = M->World->Area;
	const bool bSaro = Area == TEXT("saro");
	const bool bDiner = Area == TEXT("diner");
	if (bArrived && !bParked) SetParked(true);
	if (bCommute && Carrier)
	{
		Pose(Personal.Get(), Carrier->Position, Carrier->Rotation.Y);
		GroupPose(*Hood, Carrier->Position, Carrier->Rotation);
	}
	else if (bParked) Pose(Personal.Get(), PersonalPark, PersonalHeading);
	if (Personal.IsValid()) Personal->SetVisibility((bCommute && !bDriving && Carrier) || (!bCommute && bParked && bSaro));
	Hood->SetVisible(bCommute && bDriving && Carrier);
	if (Service.IsValid()) Service->SetVisibility(bCommute && bSaro && Carrier && FVector::Distance(Carrier->Position, SERVICE_PARK) > 4.5);
	// Keep a real order/empty plate at the counter; stable purchase flags reconstruct it after load.
	const bool bOrder = bFoodBought || bWater;
	if (bDiner && bOrder)
	{
		const FS47Obj Counter = M->Web->Obj(TEXT("world.diner.proxies.counter"));
		FVector At = Counter.IsValid() ? Counter.WorldPosition() + FVector(0, 0.15, 0.6) : FVector(7975.45, 5.329, 2000.55);
		GroupPose(*CounterFood, At, FVector::ZeroVector);
	}
	CounterFood->SetVisible(bDiner && bOrder && !bFoodTaken);
	Visible(CounterBurger, bFoodBought && bBurger && !bFoodTaken && !bFoodEaten);
	Visible(CounterSandwich, bFoodBought && !bBurger && !bFoodTaken && !bFoodEaten);
	Visible(CounterCup, bFoodBought);
	Visible(CounterWater, bWater);
	CounterFood->Refresh();
	const bool bHeld = !bDriving && !bFoodEaten && (bFoodTaken || (bDiner && bFoodBought));
	if (bHeld)
	{
		const FS47Cam Cam = FS47Cam::Of(M->Player);
		const FVector At = Cam.Pos + Cam.Forward * 0.58 + Cam.Right * 0.24 - Cam.Up * 0.30;
		GroupPose(*HeldFood, At, S47Three::EulerXYZ(Cam.Quaternion()));
	}
	HeldFood->SetVisible(bHeld);
	Visible(HeldPlate, !bFoodTaken);
	Visible(HeldBurger, bBurger && !bFoodTaken);
	Visible(HeldSandwich, !bBurger && !bFoodTaken);
	Visible(HeldBag, bFoodTaken);
	Visible(HeldCup, !bFoodTaken);
	HeldFood->Refresh();
}

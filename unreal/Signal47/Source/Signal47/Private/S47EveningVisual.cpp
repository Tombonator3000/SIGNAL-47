#include "S47EveningVisual.h"
#include "Components/InstancedStaticMeshComponent.h"

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
	const TCHAR* WAGON = TEXT("/Game/Free/Vehicles/fairheaven_sw84/StaticMeshes/fairheaven_sw84_body.fairheaven_sw84_body");
	const TCHAR* WAGON_DOOR = TEXT("/Game/Free/Vehicles/fairheaven_sw84/StaticMeshes/fairheaven_sw84_door_driver.fairheaven_sw84_door_driver");

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
	void Pose(UStaticMeshComponent* C, const FVector& P, const FQuat& Rotation)
	{
		if (!C) return;
		double E[16];
		S47Three::Compose(P, Rotation, FVector::OneVector, E);
		C->SetWorldTransform(S47Three::MatToUE(E));
		// Credited Free vehicles are centimetres, length along X; the driving frame faces -Z.
		C->AddLocalRotation(FRotator(0, -90, 0));
	}
	void Pose(UStaticMeshComponent* C, const FVector& P, double Heading)
	{
		Pose(C, P, S47Three::Euler(0, Heading, 0));
	}
	void WagonDoorPose(UStaticMeshComponent* C, const FVector& P, const FQuat& Car, double Angle)
	{
		// The cut leaf keeps the author's whole-car pivot. Rotate it about its actual front hinge.
		const FVector Hinge(-0.73, 0.72, -0.86);
		const FQuat Leaf = S47Three::Euler(0, -Angle, 0);
		Pose(C, P + Car.RotateVector(Hinge - Leaf.RotateVector(Hinge)), Car * Leaf);
	}
	void GroupPose(FS47Group& G, const FVector& P, const FVector& Rotation)
	{
		G.Root().Position = P; G.Root().Rotation = Rotation; G.Root().Apply();
	}
	void Visible(FS47Node* N, bool bOn) { if (N) N->bVisible = bOn; }
	bool WagonOverlaps(const FVector& P, double Heading, double MinX, double MaxX, double MinZ, double MaxZ)
	{
		// Separating-axis test for the actual rotated footprint against a protected rectangle.
		// A small margin keeps the car's visible edges off the passage and stair railing.
		const double W = FS47EveningVisual::PersonalHalfWidth() + 0.15;
		const double L = FS47EveningVisual::PersonalHalfLength() + 0.15;
		const double C = FMath::Cos(Heading), S = FMath::Sin(Heading);
		const double DX = P.X - (MinX + MaxX) * 0.5, DZ = P.Z - (MinZ + MaxZ) * 0.5;
		const double RX = (MaxX - MinX) * 0.5, RZ = (MaxZ - MinZ) * 0.5;
		return FMath::Abs(DX) <= RX + W * FMath::Abs(C) + L * FMath::Abs(S)
			&& FMath::Abs(DZ) <= RZ + W * FMath::Abs(S) + L * FMath::Abs(C)
			&& FMath::Abs(DX * C - DZ * S) <= W + RX * FMath::Abs(C) + RZ * FMath::Abs(S)
			&& FMath::Abs(DX * S + DZ * C) <= L + RX * FMath::Abs(S) + RZ * FMath::Abs(C);
	}

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
	if (PersonalDoor.IsValid()) PersonalDoor->DestroyComponent();
	for (auto& Part:ServiceParts) if (Part.IsValid()) Part->DestroyComponent();
}

void FS47EveningVisual::Build()
{
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (bBuilt || !W || !W->IsLoaded()) return;
	bBuilt = true;
	Personal = Vehicle(W, WAGON, TEXT("personal station wagon"));
	PersonalDoor = Vehicle(W, WAGON_DOOR, TEXT("personal station wagon driver door"));
	// Snapshot the ACTUAL service body, not a different C10 model that changes at handover.
	// The hidden arrival carrier can move independently; identical originals take over at work.
	if (FS47Truck* Truck=M->World->SaroTruck())
	{
	 const FS47WebNode* Root=W->Node(Truck->Group.I);TSet<int32> Copied;
	 TMap<UMaterialInterface*,UMaterialInterface*> FrozenMaterials;
	 if (Root && Root->Comp.IsValid())
	 {
	  double E[16];S47Three::Compose(SERVICE_PARK,S47Three::Euler(0,UE_DOUBLE_PI,0),FVector::OneVector,E);
	  const FTransform Park=S47Three::MatToUE(E),Source=Root->Comp->GetComponentTransform();
	  const auto Copy=[&](FS47Obj O)
	  {
	   if(Copied.Contains(O.I))return;Copied.Add(O.I);
	   const FS47WebNode* N=W->Node(O.I);auto* Original=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
	   if(!Original||!Original->GetStaticMesh())return;
	   // Ignore only the outer carrier gate. Preserve hidden alternatives/LOD descendants.
	   for(int32 A=O.I;A>=0&&A!=Truck->Group.I;)
	   {
	    const FS47WebNode* Ancestor=W->Node(A);
	    if(!Ancestor||!Ancestor->bVis)return;
	    A=Ancestor->Parent;
	   }
	   UStaticMeshComponent* Part=nullptr;
	   if(auto* Instances=Cast<UInstancedStaticMeshComponent>(Original))
	   {
	    auto* I=NewObject<UInstancedStaticMeshComponent>(W);I->SetStaticMesh(Original->GetStaticMesh());
	    I->NumCustomDataFloats=Instances->NumCustomDataFloats;
	    I->InstancingRandomSeed=Instances->InstancingRandomSeed;I->AdditionalRandomSeeds=Instances->AdditionalRandomSeeds;
	    for(int32 K=0;K<Instances->GetInstanceCount();++K){FTransform T;Instances->GetInstanceTransform(K,T,false);I->AddInstance(T,false);}
	    I->PerInstanceSMCustomData=Instances->PerInstanceSMCustomData;Part=I;
	   }
	   else {Part=NewObject<UStaticMeshComponent>(W);Part->SetStaticMesh(Original->GetStaticMesh());}
	   Part->SetupAttachment(W->GetRootComponent());Part->SetMobility(EComponentMobility::Movable);
	   Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetCastShadow(Original->CastShadow);
	   for(int32 K=0;K<Original->GetNumMaterials();++K)
	   {
	    UMaterialInterface* Material=Original->GetMaterial(K);
	    if(auto* Dynamic=Cast<UMaterialInstanceDynamic>(Material))
	    {
	     UMaterialInterface** Existing=FrozenMaterials.Find(Material);
	     if(Existing)Material=*Existing;
	     else
	     {
	      // Retain the static parent hierarchy, but never inherit the driven truck's MID.
	      UMaterialInterface* Parent=Dynamic->Parent;
	      while(auto* DynamicParent=Cast<UMaterialInstanceDynamic>(Parent))Parent=DynamicParent->Parent;
	      auto* Frozen=UMaterialInstanceDynamic::Create(Parent,W);
	      Frozen->CopyMaterialUniformParameters(Dynamic); // Exported UE API; includes hierarchy values/textures.
	      FrozenMaterials.Add(Material,Frozen);Material=Frozen;
	     }
	    }
	    Part->SetMaterial(K,Material);
	   }
	   Part->SetVisibility(false);Part->RegisterComponent();
	   Part->SetWorldTransform(Original->GetComponentTransform().GetRelativeTransform(Source)*Park);ServiceParts.Add(Part);
	  };
	  Truck->Body.Traverse(Copy);Truck->Shell.Traverse(Copy);W->Obj(Truck->Path+TEXT(".wheels")).Traverse(Copy);
	 }
	}
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
	UE_LOG(LogTemp, Display, TEXT("S47: evening visual built: personal wagon %s, separate SARO pickup %s, lit order/hand props"), Personal.IsValid() ? TEXT("ready") : TEXT("MISSING"), !ServiceParts.IsEmpty() ? TEXT("ready") : TEXT("MISSING"));
}

void FS47EveningVisual::ParkPersonal(const FVector& Position, double Heading)
{
	PersonalPark = Position;
	PersonalHeading = Heading;
	// Repair unsafe arrival and restored poses without changing story/evidence state. The whole
	// rotated body must clear SARO 07's pad, both entrance landings and the annex plinth.
	if (!FMath::IsFinite(Position.X)||!FMath::IsFinite(Position.Y)||!FMath::IsFinite(Position.Z)||!FMath::IsFinite(Heading)
        || WagonOverlaps(PersonalPark, Heading, 6.6, 13, 2.7, 12.4)
		|| WagonOverlaps(PersonalPark, Heading, 5, 13, -0.6, 4.5)
		|| WagonOverlaps(PersonalPark, Heading, -6, 6.6, 4.8, 10.8))
	{
		PersonalPark = ArrivalPark();
		PersonalHeading = 0;
	}
	bParked = true;
}

bool FS47EveningVisual::FitsArrivalBay(const FVector& P,double H)
{
 if(!FMath::IsFinite(P.X)||!FMath::IsFinite(P.Z)||!FMath::IsFinite(H))return false;
 const double C=FMath::Abs(FMath::Cos(H)),S=FMath::Abs(FMath::Sin(H));
 const double X=PersonalHalfWidth()*C+PersonalHalfLength()*S,Z=PersonalHalfWidth()*S+PersonalHalfLength()*C;
 const double Heading=FMath::Abs(FMath::Atan2(FMath::Sin(H),FMath::Cos(H)));
 return Heading<.32 && P.X-X>=-14.45 && P.X+X<=-11.15 && P.Z-Z>=11.40 && P.Z+Z<=17.20;
}
void FS47EveningVisual::RestorePersonal(const FVector& P,double H,int32 Version)
{
 const bool OldFallback=FVector2D(P.X-4.5,P.Z-14).Size()<.15;
 if(Version<2&&OldFallback)ParkPersonal(ArrivalPark(),0);else ParkPersonal(P,H);
}
bool FS47EveningVisual::ServiceWaitingVisible() const
{
 return !ServiceParts.IsEmpty()&&ServiceParts[0].IsValid()&&ServiceParts[0]->IsVisible();
}

void FS47EveningVisual::SetParked(bool bOn)
{
	if (bOn) ParkPersonal(PersonalPark, PersonalHeading);
	else { bParked = false; PersonalPark = ArrivalPark(); PersonalHeading = 0; }
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
		PresentedPosition = Carrier->Position;
		Pose(Personal.Get(), PresentedPosition, Carrier->Quat());
	}
	else if (bParked) { PresentedPosition = PersonalPark; Pose(Personal.Get(), PresentedPosition, PersonalHeading); }
	const bool bPersonalShown = (bCommute && Carrier) || (!bCommute && bParked && bSaro);
	// A private wagon has one coherent cabin, including the driver's own door and dashboard.
	// The hidden service-truck controller supplies driving only; none of its meshes are reused.
	DoorAngle = bPersonalShown && !bDriving ? 1.02 : 0;
	const FQuat Rotation = bCommute && Carrier ? Carrier->Quat() : S47Three::Euler(0, PersonalHeading, 0);
	WagonDoorPose(PersonalDoor.Get(), PresentedPosition, Rotation, DoorAngle);
	if (Personal.IsValid()) Personal->SetVisibility(bPersonalShown);
	if (PersonalDoor.IsValid()) PersonalDoor->SetVisibility(bPersonalShown);
	M->World->SetPersonalPark(PersonalPark, PersonalHeading, bPersonalShown && !bCommute);
	const bool ServiceShown=bCommute && bSaro && !ServiceParts.IsEmpty();
	for (auto& Part:ServiceParts)if(Part.IsValid())Part->SetVisibility(ServiceShown);
	M->World->SetArrivalServiceWaiting(ServiceShown);
	// A paid serving has one presentation: in the diner's handoff it is held, then
	// takeaway owns it. Only dine-in leftovers/water remain on the counter after eating.
	const bool bHeld = !bDriving && !bFoodEaten && !bFoodTaken && bDiner && bFoodBought;
	const bool bOrder = bFoodBought || bWater;
	if (bDiner && bOrder)
	{
		const FS47Obj Counter = M->Web->Obj(TEXT("world.diner.proxies.counter"));
		FVector At = Counter.IsValid() ? Counter.WorldPosition() + FVector(0, 0.15, 0.6) : FVector(7975.45, 5.329, 2000.55);
		GroupPose(*CounterFood, At, FVector::ZeroVector);
	}
	CounterFood->SetVisible(bDiner && bOrder && !bFoodTaken && !bHeld);
	Visible(CounterBurger, bFoodBought && bBurger && !bFoodTaken && !bFoodEaten);
	Visible(CounterSandwich, bFoodBought && !bBurger && !bFoodTaken && !bFoodEaten);
	Visible(CounterCup, bFoodBought);
	Visible(CounterWater, bWater);
	CounterFood->Refresh();
	// Takeaway has its own saved hand/surface state; this group only presents dine-in food.
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

void FS47EveningVisual::Hide()
{
 if (Personal.IsValid()) Personal->SetVisibility(false);
 if (PersonalDoor.IsValid()) PersonalDoor->SetVisibility(false);
 for (auto& Part:ServiceParts)if(Part.IsValid())Part->SetVisibility(false);
 if(M&&M->World)M->World->SetArrivalServiceWaiting(false);
 for (FS47Group* G : {CounterFood.Get(), HeldFood.Get()}) if (G) G->SetVisible(false);
 if (M && M->World) M->World->SetPersonalPark(PersonalPark, PersonalHeading, false);
}

bool FS47EveningVisual::PersonalVisible() const { return Personal.IsValid() && Personal->IsVisible(); }
bool FS47EveningVisual::PersonalDoorVisible() const { return PersonalDoor.IsValid() && PersonalDoor->IsVisible(); }

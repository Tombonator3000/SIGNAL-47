#include "S47Saro.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47, Log, All);

namespace
{
	const FLinearColor WallPaint(0.42f, 0.47f, 0.48f);   // 0xb3c2c4 in the web version, darker here under real light
	const FLinearColor WallLow(0.16f, 0.22f, 0.23f);
	const FLinearColor FloorTan(0.36f, 0.31f, 0.25f);
	const FLinearColor CeilingTile(0.62f, 0.61f, 0.57f);
	const FLinearColor Concrete(0.30f, 0.30f, 0.29f);
	const FLinearColor Sand(0.16f, 0.12f, 0.08f);
	const FLinearColor FrameGrey(0.22f, 0.23f, 0.24f);
	const FLinearColor DoorGreen(0.12f, 0.17f, 0.14f);
	const FLinearColor DeskGreen(0.18f, 0.22f, 0.19f);
	const FLinearColor Cream(0.62f, 0.58f, 0.48f);
	const FLinearColor Bakelite(0.05f, 0.05f, 0.05f);
	const FLinearColor Beige(0.55f, 0.5f, 0.4f);
	const FLinearColor MugWhite(0.8f, 0.78f, 0.72f);
	const FLinearColor RackGrey(0.14f, 0.15f, 0.16f);
	const FLinearColor DishWhite(0.7f, 0.7f, 0.68f);

	// A small deterministic generator for the yard dressing, so every run shows the same night.
	struct FRng
	{
		uint32 Seed;
		explicit FRng(uint32 S) : Seed(S) {}
		float Next()
		{
			Seed += 0x6d2b79f5u;
			uint32 T = (Seed ^ (Seed >> 15)) * (1u | Seed);
			T = (T + (T ^ (T >> 7)) * (61u | T)) ^ T;
			return float(T ^ (T >> 14)) / 4294967296.f;
		}
	};
}

AS47Saro::AS47Saro()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

UStaticMesh* AS47Saro::Mesh(const FString& Asset) const
{
	return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Models/%s.%s"), *Asset, *Asset));
}

UMaterialInstanceDynamic* AS47Saro::FlatMid(UObject* Outer, const FLinearColor& Color, float Roughness, float Metallic, const FLinearColor& Emissive)
{
	if (!Flat) return nullptr;
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Flat, Outer);
	M->SetVectorParameterValue(TEXT("Color"), Color);
	M->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	M->SetScalarParameterValue(TEXT("Metallic"), Metallic);
	M->SetVectorParameterValue(TEXT("Emissive"), Emissive);
	return M;
}

AStaticMeshActor* AS47Saro::Box(const FString& Name, const FVector& Center, const FVector& Size, const FLinearColor& Color, float Roughness, const FRotator& Rot, const FLinearColor& Emissive)
{
	FActorSpawnParameters P;
	P.Name = FName(*Name);
	AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(Rot, Center), P);
	UStaticMeshComponent* M = A->GetStaticMeshComponent();
	M->SetMobility(EComponentMobility::Static);
	M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	M->SetWorldScale3D(Size / 100.f);
	if (UMaterialInstanceDynamic* Mid = FlatMid(A, Color, Roughness, 0.f, Emissive)) M->SetMaterial(0, Mid);
	Parts.Add(A);
	return A;
}

namespace
{
	// The Refined and Workstation18 sets were modelled larger than life (a terminal 115 cm wide);
	// these bring them to real sizes. Measured from the import log, 21 October 2026.
	float AssetScale(const FString& Asset)
	{
		static const TMap<FString, float> Table = {
			{ TEXT("ControlDesk"), 1.0f }, { TEXT("CrtTerminal"), 0.42f }, { TEXT("DeskPhone"), 0.3f }, { TEXT("Printer"), 0.42f },
			{ TEXT("MugIntact"), 0.25f }, { TEXT("ReceiverRack"), 0.75f }, { TEXT("StoryPhone"), 0.3f },
			{ TEXT("WorkCrt"), 0.37f }, { TEXT("Keyboard"), 0.41f }, { TEXT("DeskLamp"), 0.01f }, { TEXT("DishBowl"), 0.5f },
		};
		const float* S = Table.Find(Asset);
		return S ? *S : 1.f;
	}
}

AStaticMeshActor* AS47Saro::Model(const FString& Name, const FString& Asset, const FVector& Pos, float Yaw, float Pitch, float Scale, const FLinearColor* Tint, float Roughness)
{
	Scale *= AssetScale(Asset);
	UStaticMesh* SM = Mesh(Asset);
	if (!SM)
	{
		UE_LOG(LogS47, Warning, TEXT("S47: model %s is missing, run Scripts/import-models.sh"), *Asset);
		return nullptr;
	}
	FActorSpawnParameters P;
	P.Name = FName(*Name);
	AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(FRotator(Pitch, Yaw, 0), Pos, FVector(Scale)), P);
	UStaticMeshComponent* M = A->GetStaticMeshComponent();
	M->SetMobility(EComponentMobility::Static);
	M->SetStaticMesh(SM);
	if (Tint)
	{
		for (int32 i = 0; i < M->GetNumMaterials(); ++i)
		{
			if (UMaterialInterface* Cur = M->GetMaterial(i))
			{
				if (Cur->GetName().Contains(TEXT("Material_0")))
				{
					if (UMaterialInstanceDynamic* Mid = FlatMid(A, *Tint, Roughness)) M->SetMaterial(i, Mid);
				}
			}
		}
	}
	Parts.Add(A);
	return A;
}

void AS47Saro::Lamp(const FString& Name, const FVector& Pos, const FLinearColor& Color, float Cd, float Radius, bool bShadows)
{
	FActorSpawnParameters P;
	P.Name = FName(*Name);
	APointLight* L = GetWorld()->SpawnActor<APointLight>(APointLight::StaticClass(), FTransform(Pos), P);
	L->PointLightComponent->SetMobility(EComponentMobility::Movable);
	L->PointLightComponent->SetIntensityUnits(ELightUnits::Candelas);
	L->PointLightComponent->SetIntensity(Cd);
	L->PointLightComponent->SetLightColor(Color);
	L->PointLightComponent->SetAttenuationRadius(Radius);
	L->PointLightComponent->SetSourceRadius(5.f);
	L->PointLightComponent->SetCastShadows(bShadows);
	Parts.Add(L);
}

void AS47Saro::Door(const FString& Id, const FVector& HingeBottom, float Width, float Height, float HingeYaw)
{
	// The leaf hangs on a hinge at the actor's origin, so opening it is a yaw of the actor.
	FActorSpawnParameters P;
	P.Name = FName(*Id);
	AActor* A = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform(FRotator(0, HingeYaw, 0), HingeBottom), P);
	USceneComponent* Root = NewObject<USceneComponent>(A, TEXT("Hinge"));
	A->SetRootComponent(Root);
	Root->RegisterComponent();
	UStaticMeshComponent* Leaf = NewObject<UStaticMeshComponent>(A, TEXT("Leaf"));
	Leaf->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Leaf->SetRelativeLocation(FVector(0, Width / 2, Height / 2));
	Leaf->SetRelativeScale3D(FVector(0.05f, Width / 100.f, Height / 100.f));
	if (UMaterialInstanceDynamic* Mid = FlatMid(A, DoorGreen, 0.6f)) Leaf->SetMaterial(0, Mid);
	Leaf->SetupAttachment(Root);
	Leaf->RegisterComponent();
	A->Tags.Add(FName(*(TEXT("Use:") + Id)));
	Doors.Add(Id, A);
	DoorOpen.Add(Id, false);
	Parts.Add(A);
}

void AS47Saro::ToggleDoor(const FString& Id)
{
	if (TWeakObjectPtr<AActor>* D = Doors.Find(Id))
	{
		if (!D->IsValid()) return;
		const bool bOpen = !DoorOpen.FindRef(Id);
		DoorOpen.Add(Id, bOpen);
		const FRotator R = (*D)->GetActorRotation();
		(*D)->SetActorRotation(FRotator(0, R.Yaw + (bOpen ? -95.f : 95.f), 0));
	}
}

bool AS47Saro::IsDoorOpen(const FString& Id) const { return DoorOpen.FindRef(Id); }

void AS47Saro::SetCrtPowered(bool bOn)
{
	if (CrtFace) CrtFace->SetVectorParameterValue(TEXT("Emissive"), bOn ? FLinearColor(0.10f, 0.9f, 0.35f) * 0.6f : FLinearColor(0.004f, 0.012f, 0.008f));
}

void AS47Saro::BeginPlay()
{
	Super::BeginPlay();
	Flat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_S47Flat.M_S47Flat"));
	Sky = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_S47Sky.M_S47Sky"));
	UE_LOG(LogS47, Display, TEXT("S47: materials %s"), (Flat && Sky) ? TEXT("loaded") : TEXT("MISSING, run Scripts/import-models.sh"));
	BuildRoom();
	BuildFurniture();
	BuildYard();
	BuildSkyAndLight();
	Spots.Add(TEXT("room"), FTransform(FRotator(0, 0, 0), FVector(-150, 0, 92)));
	Spots.Add(TEXT("window"), FTransform(FRotator(8, 0, 0), FVector(250, -250, 92)));
	Spots.Add(TEXT("outside"), FTransform(FRotator(6, 0, 0), FVector(900, 0, 92)));
	Spots.Add(TEXT("dish"), FTransform(FRotator(18, 0, 0), FVector(2000, 150, 92)));
	UE_LOG(LogS47, Display, TEXT("S47: station built, %d parts"), Parts.Num());
}

FTransform AS47Saro::StandingSpot(const FString& Id) const
{
	const FTransform* T = Spots.Find(Id);
	return T ? *T : Spots.FindRef(TEXT("room"));
}

void AS47Saro::BuildRoom()
{
	const float HX = RoomX / 2, HY = RoomY / 2, W = Wall;
	// Floor and ceiling, a little wider than the room so the walls stand on them.
	Box(TEXT("Floor"), FVector(0, 0, -5), FVector(RoomX + 2 * W, RoomY + 2 * W, 10), FloorTan, 0.55f);
	Box(TEXT("Ceiling"), FVector(0, 0, RoomZ + 5), FVector(RoomX + 2 * W, RoomY + 2 * W, 10), CeilingTile, 1.f);
	// North wall: a sill below the window, a head above it, and mullions between the panes.
	const float SillZ = 95.f, HeadZ = 275.f;
	Box(TEXT("NorthSill"), FVector(HX + W / 2, 0, SillZ / 2), FVector(W, RoomY + 2 * W, SillZ), WallPaint, 0.92f);
	Box(TEXT("NorthHead"), FVector(HX + W / 2, 0, (HeadZ + RoomZ) / 2), FVector(W, RoomY + 2 * W, RoomZ - HeadZ), WallPaint, 0.92f);
	Box(TEXT("NorthEndW"), FVector(HX + W / 2, -HY - W / 2 + 30, (SillZ + HeadZ) / 2), FVector(W, 60 + W, HeadZ - SillZ), WallPaint, 0.92f);
	Box(TEXT("NorthEndE"), FVector(HX + W / 2, HY + W / 2 - 30, (SillZ + HeadZ) / 2), FVector(W, 60 + W, HeadZ - SillZ), WallPaint, 0.92f);
	for (int32 k = 0; k <= 6; ++k)
	{
		Box(FString::Printf(TEXT("Mullion%d"), k), FVector(HX + W / 2, -540 + k * 180.f, (SillZ + HeadZ) / 2), FVector(22, 9, HeadZ - SillZ), FrameGrey, 0.5f);
	}
	Box(TEXT("SillTop"), FVector(HX - 6, 0, SillZ + 3), FVector(42, 1090, 6), FrameGrey, 0.5f);
	// Window glass: one thin pane, nearly clear, with a faint sheen.
	{
		AStaticMeshActor* Glass = Box(TEXT("WindowGlass"), FVector(HX + W / 2, 0, (SillZ + HeadZ) / 2), FVector(2, 1080, HeadZ - SillZ), FLinearColor(0.6f, 0.75f, 0.8f), 0.05f);
		if (UMaterialInterface* GlassMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_S47Glass.M_S47Glass")))
		{
			Glass->GetStaticMeshComponent()->SetMaterial(0, GlassMat);
		}
		Glass->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	// South wall with the corridor door (web x -2.9 to -1.9: Y -290 to -190).
	Box(TEXT("SouthW"), FVector(-HX - W / 2, (-HY - W - 290) / 2, RoomZ / 2), FVector(W, HY + W - 290, RoomZ), WallPaint, 0.92f);
	Box(TEXT("SouthE"), FVector(-HX - W / 2, (HY + W - 190) / 2, RoomZ / 2), FVector(W, HY + W + 190, RoomZ), WallPaint, 0.92f);
	Box(TEXT("SouthLintel"), FVector(-HX - W / 2, -240, (210 + RoomZ) / 2), FVector(W, 100, RoomZ - 210), WallPaint, 0.92f);
	Door(TEXT("door-south"), FVector(-HX - W / 2, -290, 0), 100, 210, 0);
	// A short dark corridor behind it, so the open door does not show the night.
	Box(TEXT("CorridorFloor"), FVector(-HX - W - 150, -240, -5), FVector(300, 200, 10), Concrete, 0.9f);
	Box(TEXT("CorridorBack"), FVector(-HX - W - 300, -240, RoomZ / 2), FVector(W, 200, RoomZ), WallLow, 0.9f);
	Box(TEXT("CorridorSideW"), FVector(-HX - W - 150, -340 - W / 2, RoomZ / 2), FVector(300, W, RoomZ), WallLow, 0.9f);
	Box(TEXT("CorridorSideE"), FVector(-HX - W - 150, -140 + W / 2, RoomZ / 2), FVector(300, W, RoomZ), WallLow, 0.9f);
	Box(TEXT("CorridorCeiling"), FVector(-HX - W - 150, -240, RoomZ + 5), FVector(300, 200, 10), WallLow, 0.9f);
	// East wall with the service yard door (web z 1.1 to 2.1: X -110 to -210).
	Box(TEXT("EastN"), FVector((HX + W + (-110)) / 2, HY + W / 2, RoomZ / 2), FVector(HX + W + 110, W, RoomZ), WallPaint, 0.92f);
	Box(TEXT("EastS"), FVector((-HX - W + (-210)) / 2, HY + W / 2, RoomZ / 2), FVector(HX + W - 210, W, RoomZ), WallPaint, 0.92f);
	Box(TEXT("EastLintel"), FVector(-160, HY + W / 2, (210 + RoomZ) / 2), FVector(100, W, RoomZ - 210), WallPaint, 0.92f);
	Door(TEXT("door-east"), FVector(-110, HY + W / 2, 0), 100, 210, 90);
	// West wall, full.
	Box(TEXT("West"), FVector(0, -HY - W / 2, RoomZ / 2), FVector(RoomX + 2 * W, W, RoomZ), WallPaint, 0.92f);
	// The painted lower band along the walls, as in the web version.
	Box(TEXT("BandW"), FVector(0, -HY + 1, 47.5f), FVector(RoomX, 2, 95), WallLow, 0.9f);
	Box(TEXT("BandS"), FVector(-HX + 1, 0, 47.5f), FVector(2, RoomY, 95), WallLow, 0.9f);
	Box(TEXT("BandE"), FVector(0, HY - 1, 47.5f), FVector(RoomX, 2, 95), WallLow, 0.9f);
}

void AS47Saro::BuildFurniture()
{
	// The control desk faces the window; the operator sits with the window in front.
	const FVector DeskPos(300, 0, 0);
	// The desk model is 6.8 m long along its X; yaw 90 lays it along the window.
	AStaticMeshActor* Desk = Model(TEXT("ControlDesk"), TEXT("ControlDesk"), DeskPos, 90.f, 0.f, 1.f, &DeskGreen, 0.5f);
	float DeskTop = 76.f, DeskHalfW = 120.f;
	if (Desk)
	{
		const FBox B = Desk->GetStaticMeshComponent()->Bounds.GetBox();
		DeskTop = B.Max.Z;
		DeskHalfW = (B.Max.Y - B.Min.Y) / 2;
	}
	for (int32 i = 0; i < 3; ++i)
	{
		const float Y = (i - 1) * FMath::Min(130.f, DeskHalfW * 0.7f);
		AStaticMeshActor* Crt = Model(FString::Printf(TEXT("Crt%d"), i), TEXT("CrtTerminal"), FVector(DeskPos.X + 10, Y, DeskTop), 90.f, 0.f, 1.f, &Cream, 0.55f);
		if (Crt && i == 1)
		{
			Crt->Tags.Add(TEXT("Use:crt"));
			// The centre screen's face: a flat material we can switch on.
			// The FBX has one unnamed slot, so the screen face is a small box just in front of
			// the terminal: dark green glass, lit when the terminal is in use.
			AStaticMeshActor* Face = Box(TEXT("CrtFace"), FVector(DeskPos.X + 10 - 1, Y, DeskTop + 24), FVector(1.5f, 30, 22), FLinearColor(0.01f, 0.03f, 0.02f), 0.2f);
			CrtFace = Cast<UMaterialInstanceDynamic>(Face->GetStaticMeshComponent()->GetMaterial(0));
			Face->Tags.Add(TEXT("Use:crt"));
			Spots.Add(TEXT("crt"), FTransform(FRotator(-30, 0, 0), FVector(DeskPos.X - 90, Y, 92)));
		}
	}
	SetCrtPowered(false);
	if (AStaticMeshActor* Phone = Model(TEXT("DeskPhone"), TEXT("DeskPhone"), FVector(DeskPos.X - 10, -DeskHalfW + 45, DeskTop), 70.f, 0.f, 1.f, &Bakelite, 0.35f))
	{
		Phone->Tags.Add(TEXT("Use:phone"));
		Spots.Add(TEXT("phone"), FTransform(FRotator(-30, -20, 0), FVector(DeskPos.X - 95, -DeskHalfW + 60, 92)));
	}
	Model(TEXT("Printer"), TEXT("Printer"), FVector(DeskPos.X, DeskHalfW - 50, DeskTop), 90.f, 0.f, 1.f, &Beige, 0.6f);
	Model(TEXT("Mug"), TEXT("MugIntact"), FVector(DeskPos.X - 25, 60, DeskTop), 30.f, 0.f, 1.f, &MugWhite, 0.3f);
	Model(TEXT("Chair"), TEXT("Chair"), FVector(DeskPos.X - 110, 0, 0), 0.f);
	// The receiver rack against the west wall, the operations desk beside it.
	Model(TEXT("ReceiverRack"), TEXT("ReceiverRack"), FVector(150, -RoomY / 2 + 35, 0), 0.f, 0.f, 1.f, &RackGrey, 0.5f);
	const FVector OpsPos(-250, -RoomY / 2 + 60, 0);
	AStaticMeshActor* Ops = Model(TEXT("OpsDesk"), TEXT("OfficeDesk"), OpsPos, 0.f);
	float OpsTop = 75.f;
	if (Ops) OpsTop = Ops->GetStaticMeshComponent()->Bounds.GetBox().Max.Z;
	Model(TEXT("OpsCrt"), TEXT("WorkCrt"), FVector(OpsPos.X, OpsPos.Y - 5, OpsTop), 0.f);
	Model(TEXT("OpsKeyboard"), TEXT("Keyboard"), FVector(OpsPos.X, OpsPos.Y + 30, OpsTop), 0.f);
	Model(TEXT("OpsChair"), TEXT("Chair"), FVector(OpsPos.X, OpsPos.Y + 110, 0), 90.f);
	Model(TEXT("Transceiver"), TEXT("Transceiver"), FVector(OpsPos.X - 70, OpsPos.Y, OpsTop), 0.f);
	// Ceiling fluorescents and the lights that go with them.
	Model(TEXT("TubeW"), TEXT("TwinFluorescent"), FVector(0, -300, RoomZ - 2), 0.f);
	Model(TEXT("TubeE"), TEXT("TwinFluorescent"), FVector(0, 300, RoomZ - 2), 0.f);
	Lamp(TEXT("CeilingW"), FVector(0, -300, RoomZ - 25), FLinearColor(1.0f, 0.89f, 0.72f), 5.f, 1300.f);
	Lamp(TEXT("CeilingE"), FVector(0, 300, RoomZ - 25), FLinearColor(1.0f, 0.89f, 0.72f), 4.f, 1300.f);
	Lamp(TEXT("OpsLampLight"), FVector(OpsPos.X + 50, OpsPos.Y + 20, OpsTop + 45), FLinearColor(1.0f, 0.66f, 0.35f), 2.5f, 500.f);
	Lamp(TEXT("CrtGlow"), FVector(DeskPos.X - 30, 0, DeskTop + 30), FLinearColor(0.36f, 1.0f, 0.54f), 0.0f, 360.f, false);
	Model(TEXT("WallLamp"), TEXT("WallLamp"), FVector(-RoomX / 2 + 8, 300, 220), 0.f);
	Lamp(TEXT("WallLampLight"), FVector(-RoomX / 2 + 25, 300, 215), FLinearColor(1.0f, 0.78f, 0.5f), 1.2f, 500.f);
}

void AS47Saro::BuildYard()
{
	// The ground: dark sand, 400 m across, 2 cm below the floor so the two never fight.
	Box(TEXT("Ground"), FVector(0, 0, -7), FVector(40000, 40000, 10), Sand, 0.95f);
	// The building's outside: concrete around the room, a flat roof.
	Box(TEXT("Roof"), FVector(0, 0, RoomZ + 20), FVector(RoomX + 2 * Wall + 20, RoomY + 2 * Wall + 20, 20), Concrete, 0.9f);
	Box(TEXT("Apron"), FVector(RoomX / 2 + Wall + 150, 0, -4), FVector(300, RoomY + 2 * Wall, 6), Concrete, 0.9f);
	// The array: one dish straight north of the window, two more either side, all slaved to
	// az 026, el 032 for the survey sweep (memory.md canon).
	auto Dish = [this](const FString& Name, const FVector& Pos)
	{
		AStaticMeshActor* Ped = Model(Name + TEXT("Pedestal"), TEXT("DishPedestal"), Pos, 0.f, 0.f, 1.f, &DishWhite, 0.6f);
		float Top = 300.f;
		if (Ped) Top = Ped->GetStaticMeshComponent()->Bounds.GetBox().Max.Z;
		Model(Name + TEXT("Bowl"), TEXT("DishBowl"), FVector(Pos.X, Pos.Y, Top), 26.f, 32.f, 1.f, &DishWhite, 0.6f);
	};
	Dish(TEXT("Dish026"), FVector(2600, 0, 0));
	Dish(TEXT("DishW"), FVector(2900, -1900, 0));
	Dish(TEXT("DishE"), FVector(2900, 1900, 0));
	Dish(TEXT("DishFar"), FVector(4800, 600, 0));
	// Yard dressing: the generator and cabinet by the east door, a lamp pole, stones and tufts.
	Model(TEXT("Generator"), TEXT("Generator"), FVector(-150, RoomY / 2 + Wall + 160, 0), 0.f);
	Model(TEXT("Cabinet"), TEXT("Cabinet"), FVector(100, RoomY / 2 + Wall + 40, 0), 90.f);
	AStaticMeshActor* Pole = Model(TEXT("LampPole"), TEXT("LampPole"), FVector(900, 750, 0), 0.f);
	float PoleTop = 450.f;
	if (Pole) PoleTop = Pole->GetStaticMeshComponent()->Bounds.GetBox().Max.Z;
	Lamp(TEXT("YardLight"), FVector(880, 740, PoleTop - 30), FLinearColor(1.0f, 0.8f, 0.55f), 14.f, 2500.f);
	FRng R(1986);
	static const TCHAR* Tufts[] = { TEXT("TuftA"), TEXT("TuftB"), TEXT("TuftC") };
	for (int32 i = 0; i < 90; ++i)
	{
		const float A = R.Next() * 2 * PI, D = 900.f + R.Next() * 5500.f;
		const FVector P(FMath::Cos(A) * D, FMath::Sin(A) * D, 0);
		if (FMath::Abs(P.X) < RoomX / 2 + 150 && FMath::Abs(P.Y) < RoomY / 2 + 150) continue;
		Model(FString::Printf(TEXT("Tuft%d"), i), Tufts[i % 3], P, R.Next() * 360.f, 0.f, 0.8f + R.Next() * 0.6f);
	}
	static const TCHAR* Stones[] = { TEXT("Boulder"), TEXT("SandstoneA"), TEXT("SandstoneB") };
	for (int32 i = 0; i < 14; ++i)
	{
		const float A = R.Next() * 2 * PI, D = 1500.f + R.Next() * 5000.f;
		const FVector P(FMath::Cos(A) * D, FMath::Sin(A) * D, 0);
		if (FMath::Abs(P.X) < RoomX / 2 + 200 && FMath::Abs(P.Y) < RoomY / 2 + 200) continue;
		Model(FString::Printf(TEXT("Stone%d"), i), Stones[i % 3], P, R.Next() * 360.f, 0.f, 0.7f + R.Next() * 0.9f);
	}
}

void AS47Saro::BuildSkyAndLight()
{
	// Stars: a texture made here, a few thousand points of varied brightness on black, on the
	// inside of a big sphere. Fictional sky; no catalogue.
	const int32 TW = 2048, TH = 1024;
	Stars = UTexture2D::CreateTransient(TW, TH, PF_B8G8R8A8);
	if (Stars)
	{
		TArray<FColor> Px;
		Px.SetNum(TW * TH);
		for (int32 Y = 0; Y < TH; ++Y)
		{
			// Row 0 is the zenith, the horizon is half way down; a faint glow sits just above it.
			const float H = FMath::Clamp(1.f - FMath::Abs(Y - TH * 0.5f) / (TH * 0.08f), 0.f, 1.f);
			const FColor Row(uint8(2 + 8 * H), uint8(3 + 10 * H), uint8(8 + 22 * H), 255);
			for (int32 X = 0; X < TW; ++X) Px[Y * TW + X] = Row;
		}
		FRng R(47);
		for (int32 i = 0; i < 4000; ++i)
		{
			const int32 X = 1 + int32(R.Next() * (TW - 2)), Y = 1 + int32(FMath::Pow(R.Next(), 0.8f) * TH * 0.48f);
			const float B = R.Next();
			const uint8 V = uint8(120 + B * B * 135);
			const FColor C(V, V, uint8(FMath::Min(255, int32(V) + 20)), 255);
			Px[Y * TW + X] = C;
			// A soft edge around each star, and a bigger halo around the bright ones.
			const uint8 E = uint8(V / (B > 0.8f ? 2 : 4));
			const FColor Edge(E, E, E, 255);
			Px[Y * TW + X + 1] = Edge; Px[Y * TW + X - 1] = Edge; Px[(Y + 1) * TW + X] = Edge; Px[(Y - 1) * TW + X] = Edge;
		}
		void* Data = Stars->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Data, Px.GetData(), Px.Num() * sizeof(FColor));
		Stars->GetPlatformData()->Mips[0].BulkData.Unlock();
		Stars->SRGB = false;
		Stars->Filter = TF_Bilinear;
		Stars->UpdateResource();
	}
	{
		FActorSpawnParameters P;
		P.Name = FName(TEXT("SkySphere"));
		AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(FRotator::ZeroRotator, FVector(0, 0, -2000), FVector(400.f)), P);
		UStaticMeshComponent* M = A->GetStaticMeshComponent();
		M->SetMobility(EComponentMobility::Movable);
		M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->SetCastShadow(false);
		M->bAffectDynamicIndirectLighting = false;
		M->bAffectDistanceFieldLighting = false;
		if (Sky)
		{
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Sky, A);
			if (Stars) Mid->SetTextureParameterValue(TEXT("Stars"), Stars);
			M->SetMaterial(0, Mid);
		}
		Parts.Add(A);
	}
	// Moonlight from the south-west, low, and a faint blue fill from the sky.
	{
		ADirectionalLight* Moon = GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FTransform(FRotator(-32, 215, 0)));
		UDirectionalLightComponent* C = Cast<UDirectionalLightComponent>(Moon->GetLightComponent());
		C->SetMobility(EComponentMobility::Movable);
		C->SetIntensity(0.9f);
		C->SetLightColor(FLinearColor(0.56f, 0.64f, 0.85f));
		C->SetCastShadows(true);
		C->SetAtmosphereSunLight(false);
		Parts.Add(Moon);
		ASkyLight* SkyL = GetWorld()->SpawnActor<ASkyLight>(ASkyLight::StaticClass());
		SkyL->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		SkyL->GetLightComponent()->SetIntensity(0.08f);
		SkyL->GetLightComponent()->SetLightColor(FLinearColor(0.5f, 0.6f, 0.85f));
		Parts.Add(SkyL);
	}
	{
		AExponentialHeightFog* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass());
		Fog->GetComponent()->SetFogDensity(0.001f);
		Fog->GetComponent()->SetFogHeightFalloff(0.5f);
		Fog->GetComponent()->SetFogMaxOpacity(0.7f);
		Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(0.012f, 0.016f, 0.03f));
		Fog->GetComponent()->SetStartDistance(600.f);
		Parts.Add(Fog);
	}
	{
		// Fixed exposure so the night stays dark on purpose; a little bloom for the lamps.
		APostProcessVolume* V = GetWorld()->SpawnActor<APostProcessVolume>();
		V->bUnbound = true;
		FPostProcessSettings& S = V->Settings;
		S.bOverride_AutoExposureMethod = true; S.AutoExposureMethod = AEM_Manual;
		S.bOverride_AutoExposureApplyPhysicalCameraExposure = true; S.AutoExposureApplyPhysicalCameraExposure = false;
		S.bOverride_AutoExposureBias = true; S.AutoExposureBias = 0.f;
		S.bOverride_BloomIntensity = true; S.BloomIntensity = 0.35f;
		S.bOverride_VignetteIntensity = true; S.VignetteIntensity = 0.35f;
		S.bOverride_MotionBlurAmount = true; S.MotionBlurAmount = 0.f;
		Parts.Add(V);
	}
}

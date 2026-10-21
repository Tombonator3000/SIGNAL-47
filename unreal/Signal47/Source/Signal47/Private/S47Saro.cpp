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
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47, Log, All);

namespace
{
	const FLinearColor WallPaint(0.42f, 0.47f, 0.48f);
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
	const FLinearColor Paper(0.85f, 0.82f, 0.74f);
	const FLinearColor PaperOld(0.78f, 0.72f, 0.58f);
	const FLinearColor LedOff(0.08f, 0.09f, 0.1f), LedGreen(0.27f, 1.0f, 0.48f), LedAmber(1.0f, 0.69f, 0.25f);

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

	// The Refined and Workstation18 sets were modelled larger than life; these bring them to
	// real sizes. Measured from the import log, 21 October 2026.
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

	// The 27 antennas (web/src/world/arrayLayout.ts): web x east and z north negative, metres.
	struct FDishDef { const TCHAR* Id; double X, Z; bool bLight; };
	const FDishDef DishLayout[] = {
		{ TEXT("S-01"), -48, -66, false }, { TEXT("S-02"), -16, -52, false }, { TEXT("S-03"), 9.5, -42, false }, { TEXT("S-04"), 6, -88, false },
		{ TEXT("S-05"), 44, -96, false }, { TEXT("S-06"), -35, -116, false }, { TEXT("S-07"), -80, -104, false }, { TEXT("S-08"), 20, -146, false },
		{ TEXT("S-09"), 66, -156, false }, { TEXT("S-10"), -58, -168, false }, { TEXT("S-11"), -10, -200, false }, { TEXT("S-12"), 100, -120, false },
		{ TEXT("S-13"), -120, -150, false }, { TEXT("S-14"), 40, -230, false }, { TEXT("S-15"), -96, -230, false },
		{ TEXT("S-16"), 130, -210, true }, { TEXT("S-17"), -150, -270, true }, { TEXT("S-18"), 80, -290, true }, { TEXT("S-19"), -40, -300, true },
		{ TEXT("S-20"), 170, -260, true }, { TEXT("S-21"), -200, -210, true }, { TEXT("S-22"), 10, -360, true }, { TEXT("S-23"), 120, -350, true },
		{ TEXT("S-24"), -120, -340, true }, { TEXT("S-25"), -230, -300, true }, { TEXT("S-26"), 210, -330, true }, { TEXT("S-27"), -60, -420, true },
	};
}

AS47Saro::AS47Saro()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

// ---------- building blocks ----------

UStaticMesh* AS47Saro::Mesh(const FString& Asset) const
{
	return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Models/%s.%s"), *Asset, *Asset));
}

UStaticMesh* AS47Saro::LicensedMesh(const FString& Pack, const FString& MeshName) const
{
	return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Licensed/Models/%s/StaticMeshes/%s.%s"), *Pack, *MeshName, *MeshName));
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
	M->SetMobility(EComponentMobility::Movable);
	M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	M->SetWorldScale3D(Size / 100.f);
	if (UMaterialInstanceDynamic* Mid = FlatMid(A, Color, Roughness, 0.f, Emissive)) M->SetMaterial(0, Mid);
	Parts.Add(A);
	return A;
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
	M->SetMobility(EComponentMobility::Movable);
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

AStaticMeshActor* AS47Saro::Licensed(const FString& Name, const FString& Pack, const FString& MeshName, const FVector& Pos, float Yaw, float Scale)
{
	UStaticMesh* SM = LicensedMesh(Pack, MeshName);
	if (!SM) return nullptr;
	FActorSpawnParameters P;
	P.Name = FName(*Name);
	AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(FRotator(0, Yaw, 0), Pos, FVector(Scale)), P);
	A->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
	A->GetStaticMeshComponent()->SetStaticMesh(SM);
	Parts.Add(A);
	return A;
}

void AS47Saro::Lamp(const FString& Name, const FVector& Pos, const FLinearColor& Color, float Cd, float Radius, bool bShadows, TArray<TObjectPtr<UPointLightComponent>>* Keep)
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
	if (Keep) Keep->Add(L->PointLightComponent);
	Parts.Add(L);
}

void AS47Saro::Tag(AActor* A, const FString& Id)
{
	if (!A || Id.IsEmpty()) return;
	A->Tags.Add(FName(*(TEXT("Use:") + Id)));
	if (!Props.Contains(Id)) Props.Add(Id, A);
}

FBox AS47Saro::Bounds(AActor* A) const
{
	if (AStaticMeshActor* S = Cast<AStaticMeshActor>(A)) return S->GetStaticMeshComponent()->Bounds.GetBox();
	return A ? A->GetComponentsBoundingBox() : FBox(ForceInit);
}

void AS47Saro::Door(const FString& Id, const FVector& HingeBottom, float Width, float Height, float HingeYaw)
{
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
	Tag(A, Id);
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

FVector AS47Saro::PropLocation(const FString& Id) const
{
	const TWeakObjectPtr<AActor>* A = Props.Find(Id);
	return (A && A->IsValid()) ? (*A)->GetActorLocation() : FVector::ZeroVector;
}

FTransform AS47Saro::StandingSpot(const FString& Id) const
{
	const FTransform* T = Spots.Find(Id);
	return T ? *T : Spots.FindRef(TEXT("room"));
}

// ---------- the story's handles ----------

void AS47Saro::SetLever(bool bUp)
{
	if (Lever.IsValid()) Lever->SetActorRotation(FRotator(bUp ? 35.f : -35.f, 0, 0));
}

void AS47Saro::SetLeds(bool bOn)
{
	for (int32 i = 0; i < LedMids.Num(); ++i) SetLed(i, bOn);
}

void AS47Saro::SetLed(int32 Index, bool bOn)
{
	if (!LedMids.IsValidIndex(Index) || !LedMids[Index]) return;
	LedOn[Index] = bOn;
	// Sixteen green, two amber. Amber is normal, according to Dale.
	const bool bAmber = Index == 2 || Index == 9;
	const FLinearColor C = bOn ? (bAmber ? LedAmber : LedGreen) : LedOff;
	LedMids[Index]->SetVectorParameterValue(TEXT("Emissive"), bOn ? C * 1.5f : FLinearColor::Black);
	LedMids[Index]->SetVectorParameterValue(TEXT("Color"), C * 0.3f);
}

void AS47Saro::FlickerLed()
{
	if (LedMids.Num() == 0) return;
	SetLed(FMath::RandRange(0, LedMids.Num() - 1), FMath::FRand() >= 0.3f);
}

void AS47Saro::SetCrtPowered(int32 Screen, bool bOn)
{
	if (!CrtOn.IsValidIndex(Screen)) return;
	CrtOn[Screen] = bOn;
	if (CrtFaces.IsValidIndex(Screen) && CrtFaces[Screen]) CrtFaces[Screen]->SetScalarParameterValue(TEXT("Glow"), bOn ? 1.f : 0.f);
}

void AS47Saro::SetCrtGlow(float Intensity)
{
	if (CrtLight) CrtLight->SetIntensity(Intensity * 0.8f);
}

void AS47Saro::SetPrinterPaper(float Share)
{
	if (!PrinterPaper.IsValid()) return;
	PrinterPaper->SetActorHiddenInGame(Share < 0.f);
	const float S = FMath::Clamp(Share, 0.001f, 1.f);
	// the sheet hangs out of the printer's front, growing downwards from its top edge
	const FVector Base = PropLocation(TEXT("printer"));
	PrinterPaper->SetActorScale3D(FVector(0.004f, 0.22f, 0.28f * S));
	PrinterPaper->SetActorLocation(Base + FVector(-26.f, 0, 42.f - 14.f * S));
}

void AS47Saro::SetHandsetVisible(bool bOn) { if (Handset.IsValid()) Handset->SetActorHiddenInGame(!bOn); }
void AS47Saro::SetHandsetRattle(bool bOn) { bHandsetRattle = bOn; }
bool AS47Saro::IsMugVisible() const { return Mug.IsValid() && !Mug->IsHidden(); }
void AS47Saro::SetCoffee(bool bFull) { if (Coffee.IsValid()) Coffee->SetActorHiddenInGame(!bFull); }
void AS47Saro::SetJacketVisible(bool bOn) { if (Jacket.IsValid()) Jacket->SetActorHiddenInGame(!bOn); }

void AS47Saro::DropMug()
{
	if (!Mug.IsValid() || Mug->IsHidden() || MugFall) return;
	MugFall = FMugFall{ 0.f, FVector(-50.f, 0, 0), 6.f + FMath::FRand() * 3.f, 0 };
}

void AS47Saro::ResetMug()
{
	MugFall.Reset();
	if (Mug.IsValid()) { Mug->SetActorHiddenInGame(false); Mug->SetActorLocation(MugHome); Mug->SetActorRotation(FRotator(0, 30, 0)); }
}

void AS47Saro::SetClock(double Sec)
{
	const double S = FMath::Fmod(FMath::Fmod(Sec, 86400.0) + 86400.0, 86400.0);
	const float H = float(FMath::Fmod(S / 3600.0, 12.0) * 30.0), M = float(FMath::Fmod(S / 60.0, 60.0) * 6.0), Ss = float(FMath::Fmod(S, 60.0) * 6.0);
	// the clock face is on the south wall and faces north: the hands turn about X
	if (HandH.IsValid()) HandH->SetActorRotation(FRotator(0, 0, -H));
	if (HandM.IsValid()) HandM->SetActorRotation(FRotator(0, 0, -M));
	if (HandS.IsValid()) HandS->SetActorRotation(FRotator(0, 0, -Ss));
}

void AS47Saro::SetLightsScale(float Ceiling, float Lamps)
{
	for (int32 i = 0; i < CeilingLights.Num(); ++i) if (CeilingLights[i]) CeilingLights[i]->SetIntensity(CeilingBase[i] * Ceiling);
	for (int32 i = 0; i < LampLights.Num(); ++i) if (LampLights[i]) LampLights[i]->SetIntensity(LampBase[i] * Lamps);
}

void AS47Saro::SetTelexRoll(const TArray<FString>& Lines) {}

void AS47Saro::PointDishes(double Az, double El, double Slew)
{
	for (FDish& D : Dishes) { D.TgtAz = Az; D.TgtEl = El; if (Slew > 0) D.Slew = Slew; }
}

void AS47Saro::SnapDishes(double Az, double El)
{
	for (FDish& D : Dishes) { D.CurAz = D.TgtAz = Az; D.CurEl = D.TgtEl = El; }
}

bool AS47Saro::DishMoving(int32 I) const
{
	if (!Dishes.IsValidIndex(I)) return false;
	return FMath::Abs(Dishes[I].CurAz - Dishes[I].TgtAz) > 0.05 || FMath::Abs(Dishes[I].CurEl - Dishes[I].TgtEl) > 0.05;
}

bool AS47Saro::DishesMoving() const
{
	for (int32 i = 0; i < Dishes.Num(); ++i) if (DishMoving(i)) return true;
	return false;
}

void AS47Saro::Tick(float Dt)
{
	Super::Tick(Dt);
	for (FDish& D : Dishes)
	{
		const double Step = D.Slew * Dt;
		const double DA = D.TgtAz - D.CurAz, DE = D.TgtEl - D.CurEl;
		D.CurAz += FMath::Sign(DA) * FMath::Min(FMath::Abs(DA), Step);
		D.CurEl += FMath::Sign(DE) * FMath::Min(FMath::Abs(DE), Step * 0.6);
		if (D.Bowl.IsValid()) D.Bowl->SetActorRotation(FRotator(float(D.CurEl), float(D.CurAz), 0));
	}
	if (Handset.IsValid())
	{
		HandsetT += Dt;
		const float R = bHandsetRattle ? FMath::Sin(HandsetT * 70.f) * 3.f * (FMath::Sin(HandsetT * 2.2f) > -0.2f ? 1.f : 0.f) : 0.f;
		Handset->SetActorRotation(FRotator(0, 160.f, R));
	}
	if (MugFall && Mug.IsValid())
	{
		FMugFall& F = *MugFall;
		F.T += Dt;
		FVector P = Mug->GetActorLocation();
		if (F.Stage == 0)
		{
			// walks off the desk edge towards the player, with a little shiver
			P.X = MugHome.X - FMath::Min(1.f, F.T / 0.55f) * 27.f + (FMath::FRand() - 0.5f) * 0.6f;
			P.Y = MugHome.Y + (FMath::FRand() - 0.5f) * 0.6f;
			Mug->SetActorLocation(P);
			Mug->AddActorWorldRotation(FRotator(0, (FMath::FRand() - 0.5f) * 8.f, 0));
			if (F.T >= 0.55f) F.Stage = 1;
		}
		else
		{
			F.V.Z -= 980.f * Dt;
			P += F.V * Dt;
			Mug->SetActorLocation(P);
			Mug->AddActorWorldRotation(FRotator(-F.Spin * Dt * 57.f, 0, 0));
			if (P.Z <= 3.f)
			{
				Mug->SetActorHiddenInGame(true);
				MugFall.Reset();
				if (OnMugShatter) OnMugShatter(FVector(P.X, P.Y, 0));
			}
		}
	}
}

// ---------- building ----------

void AS47Saro::BeginPlay()
{
	Super::BeginPlay();
	Flat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_S47Flat.M_S47Flat"));
	Sky = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_S47Sky.M_S47Sky"));
	ScreenMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_S47Screen.M_S47Screen"));
	UE_LOG(LogS47, Display, TEXT("S47: materials %s, licensed pack %s"), (Flat && Sky && ScreenMat) ? TEXT("loaded") : TEXT("MISSING, run Scripts/import-models.sh"),
		LicensedMesh(TEXT("crt_terminal"), TEXT("crt_terminal")) ? TEXT("found") : TEXT("not imported"));
	for (int32 i = 0; i < 4; ++i)
	{
		UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this, *FString::Printf(TEXT("CrtTarget%d"), i));
		RT->ClearColor = FLinearColor(0.004f, 0.02f, 0.008f);
		RT->InitCustomFormat(CrtW, CrtH, PF_B8G8R8A8, false);
		RT->UpdateResourceImmediate(true);
		CrtTargets.Add(RT);
		CrtOn.Add(false);
	}
	CrtFaces.SetNum(4);
	BuildRoom();
	BuildFurniture();
	BuildPapers();
	BuildYard();
	BuildArray();
	BuildSkyAndLight();
	Spots.Add(TEXT("room"), FTransform(FRotator(0, 0, 0), FVector(-150, 0, 92)));
	Spots.Add(TEXT("window"), FTransform(FRotator(8, 0, 0), FVector(250, -250, 92)));
	Spots.Add(TEXT("outside"), FTransform(FRotator(6, 0, 0), FVector(900, 0, 92)));
	Spots.Add(TEXT("dish"), FTransform(FRotator(18, 20, 0), FVector(2800, 700, 92)));
	UE_LOG(LogS47, Display, TEXT("S47: station built, %d parts, %d dishes"), Parts.Num(), Dishes.Num());
}

void AS47Saro::EndPlay(const EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);
}

void AS47Saro::BuildRoom()
{
	const float HX = RoomX / 2, HY = RoomY / 2, W = Wall;
	Box(TEXT("Floor"), FVector(0, 0, -5), FVector(RoomX + 2 * W, RoomY + 2 * W, 10), FloorTan, 0.55f);
	Box(TEXT("Ceiling"), FVector(0, 0, RoomZ + 5), FVector(RoomX + 2 * W, RoomY + 2 * W, 10), CeilingTile, 1.f);
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
	{
		AStaticMeshActor* Glass = Box(TEXT("WindowGlass"), FVector(HX + W / 2, 0, (SillZ + HeadZ) / 2), FVector(2, 1080, HeadZ - SillZ), FLinearColor(0.6f, 0.75f, 0.8f), 0.05f);
		if (UMaterialInterface* GlassMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_S47Glass.M_S47Glass")))
		{
			Glass->GetStaticMeshComponent()->SetMaterial(0, GlassMat);
		}
	}
	Box(TEXT("SouthW"), FVector(-HX - W / 2, (-HY - W - 290) / 2, RoomZ / 2), FVector(W, HY + W - 290, RoomZ), WallPaint, 0.92f);
	Box(TEXT("SouthE"), FVector(-HX - W / 2, (HY + W - 190) / 2, RoomZ / 2), FVector(W, HY + W + 190, RoomZ), WallPaint, 0.92f);
	Box(TEXT("SouthLintel"), FVector(-HX - W / 2, -240, (210 + RoomZ) / 2), FVector(W, 100, RoomZ - 210), WallPaint, 0.92f);
	Door(TEXT("door-south"), FVector(-HX - W / 2, -290, 0), 100, 210, 0);
	Box(TEXT("CorridorFloor"), FVector(-HX - W - 150, -240, -5), FVector(300, 200, 10), Concrete, 0.9f);
	Box(TEXT("CorridorBack"), FVector(-HX - W - 300, -240, RoomZ / 2), FVector(W, 200, RoomZ), WallLow, 0.9f);
	Box(TEXT("CorridorSideW"), FVector(-HX - W - 150, -340 - W / 2, RoomZ / 2), FVector(300, W, RoomZ), WallLow, 0.9f);
	Box(TEXT("CorridorSideE"), FVector(-HX - W - 150, -140 + W / 2, RoomZ / 2), FVector(300, W, RoomZ), WallLow, 0.9f);
	Box(TEXT("CorridorCeiling"), FVector(-HX - W - 150, -240, RoomZ + 5), FVector(300, 200, 10), WallLow, 0.9f);
	Box(TEXT("EastN"), FVector((HX + W + (-110)) / 2, HY + W / 2, RoomZ / 2), FVector(HX + W + 110, W, RoomZ), WallPaint, 0.92f);
	Box(TEXT("EastS"), FVector((-HX - W + (-210)) / 2, HY + W / 2, RoomZ / 2), FVector(HX + W - 210, W, RoomZ), WallPaint, 0.92f);
	Box(TEXT("EastLintel"), FVector(-160, HY + W / 2, (210 + RoomZ) / 2), FVector(100, W, RoomZ - 210), WallPaint, 0.92f);
	Door(TEXT("door-east"), FVector(-110, HY + W / 2, 0), 100, 210, 90);
	Box(TEXT("West"), FVector(0, -HY - W / 2, RoomZ / 2), FVector(RoomX + 2 * W, W, RoomZ), WallPaint, 0.92f);
	Box(TEXT("BandW"), FVector(0, -HY + 1, 47.5f), FVector(RoomX, 2, 95), WallLow, 0.9f);
	Box(TEXT("BandS"), FVector(-HX + 1, 0, 47.5f), FVector(2, RoomY, 95), WallLow, 0.9f);
	Box(TEXT("BandE"), FVector(0, HY - 1, 47.5f), FVector(RoomX, 2, 95), WallLow, 0.9f);
}

AStaticMeshActor* AS47Saro::CrtScreen(int32 Screen, const FString& Name, const FVector& Pos, float Yaw, const FString& UseId)
{
	// The bought CRT when the pack is there (its screen sits 40 cm up, 37 by 28 cm, on the
	// front), otherwise the archive's blocky terminal. The picture is a thin box over the glass,
	// facing the player (-X).
	// The bought model's front is its +Y; yaw 90 turns it to -X, towards the player.
	AStaticMeshActor* Body = Licensed(Name, TEXT("crt_terminal"), TEXT("crt_terminal"), Pos, Yaw + 90.f);
	FVector FaceCenter, FaceSize;
	if (Body)
	{
		Licensed(Name + TEXT("Keys"), TEXT("crt_terminal"), TEXT("crt_keyboard"), Pos + FVector(-45.f, 0, 0), Yaw + 90.f);
		const FBox B = Bounds(Body);
		FaceCenter = FVector(B.Min.X - 0.5f, Pos.Y, Pos.Z + 39.7f);
		FaceSize = FVector(1.f, 35.5f, 28.3f);
	}
	else
	{
		Body = Model(Name, TEXT("CrtTerminal"), Pos, 90.f, 0.f, 1.f, &Cream, 0.55f);
		const FBox B = Body ? Bounds(Body) : FBox(Pos - FVector(20), Pos + FVector(20, 20, 40));
		FaceCenter = FVector(B.Min.X - 0.5f, Pos.Y, Pos.Z + 24.f);
		FaceSize = FVector(1.f, 30.f, 22.f);
	}
	AStaticMeshActor* Face = Box(Name + TEXT("Face"), FaceCenter, FaceSize, FLinearColor(0.01f, 0.03f, 0.02f), 0.2f);
	UMaterialInstanceDynamic* Mid = nullptr;
	if (ScreenMat)
	{
		Mid = UMaterialInstanceDynamic::Create(ScreenMat, Face);
		Mid->SetTextureParameterValue(TEXT("Screen"), CrtTargets[Screen]);
		Mid->SetScalarParameterValue(TEXT("Glow"), 0.f);
		Face->GetStaticMeshComponent()->SetMaterial(0, Mid);
	}
	CrtFaces[Screen] = Mid;
	Tag(Face, UseId);
	if (Body) Tag(Body, UseId);
	return Face;
}

void AS47Saro::BuildFurniture()
{
	// The control desk along the window; the three screens on it: status, spectrum, tracking.
	const FVector DeskPos(300, 0, 0);
	AStaticMeshActor* Desk = Model(TEXT("ControlDesk"), TEXT("ControlDesk"), DeskPos, 90.f, 0.f, 1.f, &DeskGreen, 0.5f);
	float DeskTop = 82.f, DeskHalfW = 340.f;
	if (Desk)
	{
		const FBox B = Bounds(Desk);
		DeskTop = B.Max.Z;
		DeskHalfW = (B.Max.Y - B.Min.Y) / 2;
	}
	CrtScreen(0, TEXT("CrtLeft"), FVector(DeskPos.X + 12, -170, DeskTop), 0.f, TEXT("crtLeft"));
	CrtScreen(1, TEXT("CrtCenter"), FVector(DeskPos.X + 12, 0, DeskTop), 0.f, TEXT("crtCenter"));
	CrtScreen(2, TEXT("CrtRight"), FVector(DeskPos.X + 12, 170, DeskTop), 0.f, TEXT("crtRight"));
	Spots.Add(TEXT("crt"), FTransform(FRotator(-22, 0, 0), FVector(DeskPos.X - 110, 0, 92)));
	// the taped card on the console, under the centre screen
	Tag(Box(TEXT("RfiCard"), FVector(DeskPos.X - 30, 60, DeskTop + 0.6f), FVector(9, 13, 0.4f), PaperOld, 0.8f, FRotator(0, 12, 0)), TEXT("rfiCard"));
	// the phone, the logbook and the work order on the left; the printer on the right
	if (AStaticMeshActor* Phone = Model(TEXT("DeskPhone"), TEXT("DeskPhone"), FVector(DeskPos.X - 30, -DeskHalfW + 40, DeskTop), 70.f, 0.f, 1.f, &Bakelite, 0.35f))
	{
		Tag(Phone, TEXT("phone"));
		const FBox B = Bounds(Phone);
		Handset = Box(TEXT("Handset"), FVector(B.GetCenter().X, B.GetCenter().Y, B.Max.Z + 1.5f), FVector(22, 5, 4), Bakelite, 0.35f, FRotator(0, 160, 0));
		Tag(Handset.Get(), TEXT("phone"));
		Spots.Add(TEXT("phone"), FTransform(FRotator(-32, -10, 0), FVector(DeskPos.X - 110, -DeskHalfW + 60, 92)));
	}
	// the papers lie at the front of the desk, clear of the screens behind them
	Tag(Box(TEXT("Logbook"), FVector(DeskPos.X - 32, -DeskHalfW + 100, DeskTop + 1.f), FVector(22, 30, 2), FLinearColor(0.9f, 0.87f, 0.8f), 0.85f, FRotator(0, -8, 0)), TEXT("logbook"));
	Spots.Add(TEXT("log"), FTransform(FRotator(-46, 0, 0), FVector(DeskPos.X - 112, -DeskHalfW + 100, 92)));
	Tag(Box(TEXT("WorkOrder"), FVector(DeskPos.X - 38, -DeskHalfW + 135, DeskTop + 0.5f), FVector(22, 28, 0.4f), Paper, 0.85f, FRotator(0, 5, 0)), TEXT("workOrder"));
	if (AStaticMeshActor* Printer = Model(TEXT("Printer"), TEXT("Printer"), FVector(DeskPos.X, DeskHalfW - 60, DeskTop), 90.f, 0.f, 1.f, &Beige, 0.6f))
	{
		Tag(Printer, TEXT("printer"));
		PrinterPaper = Box(TEXT("PrinterPaper"), FVector(DeskPos.X - 26, DeskHalfW - 60, DeskTop + 30), FVector(0.4f, 22, 28), Paper, 0.9f);
		Tag(PrinterPaper.Get(), TEXT("printer"));
		SetPrinterPaper(-1.f);
		Spots.Add(TEXT("printer"), FTransform(FRotator(-25, 0, 0), FVector(DeskPos.X - 110, DeskHalfW - 60, 92)));
	}
	// the mug, with the coffee as a dark disc inside it
	if (AStaticMeshActor* M = Model(TEXT("Mug"), TEXT("MugIntact"), FVector(DeskPos.X - 25, 95, DeskTop), 30.f, 0.f, 1.f, &MugWhite, 0.3f))
	{
		Tag(M, TEXT("mug"));
		Mug = M;
		MugHome = M->GetActorLocation();
		const FBox B = Bounds(M);
		Coffee = Box(TEXT("Coffee"), FVector(B.GetCenter().X, B.GetCenter().Y, B.Max.Z - 1.5f), FVector(7, 7, 0.5f), FLinearColor(0.09f, 0.05f, 0.02f), 0.1f);
		SetCoffee(false);
	}
	Model(TEXT("Chair"), TEXT("Chair"), FVector(DeskPos.X - 110, 0, 0), 0.f);
	if (!Licensed(TEXT("FoldingChair"), TEXT("folding_chair"), TEXT("folding_chair"), FVector(DeskPos.X - 120, 230, 0), 160.f))
	{
		Model(TEXT("Chair2"), TEXT("Chair"), FVector(DeskPos.X - 120, 230, 0), 160.f);
	}
	// The receiver rack on the east wall by the printer (web: "Bank 3 is on the east wall, by
	// the printer"): the lever, the status lamps and the service record on it.
	const FVector RackPos(150, RoomY / 2 - 40, 0);
	AStaticMeshActor* Rack = Model(TEXT("ReceiverRack"), TEXT("ReceiverRack"), RackPos, 90.f, 0.f, 1.f, &RackGrey, 0.5f);
	float RackFaceY = RoomY / 2 - 70;
	if (Rack)
	{
		RackFaceY = Bounds(Rack).Min.Y;
		Tag(Rack, TEXT("rack"));
	}
	{
		// the lever on a plate at hand height, pointing out of the rack face
		FActorSpawnParameters P;
		P.Name = FName(TEXT("LeverPivot"));
		AActor* Pivot = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform(FRotator::ZeroRotator, FVector(RackPos.X - 40, RackFaceY - 2, 120)), P);
		USceneComponent* Root = NewObject<USceneComponent>(Pivot, TEXT("Root"));
		Pivot->SetRootComponent(Root);
		Root->RegisterComponent();
		UStaticMeshComponent* Arm = NewObject<UStaticMeshComponent>(Pivot, TEXT("Arm"));
		Arm->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Arm->SetRelativeLocation(FVector(0, -6.f, 7.f));
		Arm->SetRelativeScale3D(FVector(0.03f, 0.04f, 0.18f));
		if (UMaterialInstanceDynamic* Mid = FlatMid(Pivot, FLinearColor(0.7f, 0.08f, 0.06f), 0.4f)) Arm->SetMaterial(0, Mid);
		Arm->SetupAttachment(Root);
		Arm->RegisterComponent();
		Tag(Pivot, TEXT("rack"));
		Parts.Add(Pivot);
		Lever = Pivot;
		SetLever(false);
	}
	Box(TEXT("LeverPlate"), FVector(RackPos.X - 40, RackFaceY - 0.5f, 120), FVector(14, 1, 18), FLinearColor(0.2f, 0.2f, 0.2f), 0.5f);
	for (int32 i = 0; i < 18; ++i)
	{
		const int32 Col = i % 6, Row = i / 6;
		AStaticMeshActor* L = Box(FString::Printf(TEXT("Led%d"), i), FVector(RackPos.X + 20 + Col * 7.f, RackFaceY - 0.6f, 150 + Row * 7.f), FVector(2.2f, 1.f, 2.2f), LedOff, 0.3f);
		LedMids.Add(Cast<UMaterialInstanceDynamic>(L->GetStaticMeshComponent()->GetMaterial(0)));
		LedOn.Add(false);
	}
	Tag(Box(TEXT("ServiceRecord"), FVector(RackPos.X + 10, RackFaceY - 1.2f, 95), FVector(22, 1.2f, 30), Paper, 0.85f), TEXT("serviceRecord"));
	Spots.Add(TEXT("rack"), FTransform(FRotator(-8, 90, 0), FVector(RackPos.X - 10, RackFaceY - 110, 92)));
	// the exceptions binder on a shelf above the printer
	Box(TEXT("ShelfE"), FVector(DeskPos.X - 20, RoomY / 2 - 15, 175), FVector(90, 30, 3), FLinearColor(0.2f, 0.18f, 0.15f), 0.7f);
	Tag(Box(TEXT("Binder"), FVector(DeskPos.X - 10, RoomY / 2 - 15, 192), FVector(28, 7, 32), FLinearColor(0.1f, 0.12f, 0.2f), 0.6f), TEXT("excBinder"));
	// The operations desk against the west wall, with the terminal on it.
	const FVector OpsPos(-250, -RoomY / 2 + 60, 0);
	AStaticMeshActor* Ops = Model(TEXT("OpsDesk"), TEXT("OfficeDesk"), OpsPos, 0.f);
	float OpsTop = 75.f;
	if (Ops) OpsTop = Bounds(Ops).Max.Z;
	{
		AStaticMeshActor* Body = Licensed(TEXT("OpsCrt"), TEXT("crt_terminal"), TEXT("crt_terminal"), FVector(OpsPos.X, OpsPos.Y + 10, OpsTop), 0.f);
		FVector FaceCenter, FaceSize;
		if (Body)
		{
			Licensed(TEXT("OpsKeys"), TEXT("crt_terminal"), TEXT("crt_keyboard"), FVector(OpsPos.X, OpsPos.Y + 55, OpsTop), 0.f);
			const FBox B = Bounds(Body);
			FaceCenter = FVector(OpsPos.X, B.Max.Y + 0.5f, OpsTop + 39.7f);
			FaceSize = FVector(35.5f, 1.f, 28.3f);
		}
		else
		{
			Body = Model(TEXT("OpsCrt"), TEXT("WorkCrt"), FVector(OpsPos.X, OpsPos.Y - 5, OpsTop), 0.f);
			Model(TEXT("OpsKeyboard"), TEXT("Keyboard"), FVector(OpsPos.X, OpsPos.Y + 30, OpsTop), 0.f);
			const FBox B = Body ? Bounds(Body) : FBox(FVector(OpsPos.X - 20, OpsPos.Y - 20, OpsTop), FVector(OpsPos.X + 20, OpsPos.Y + 20, OpsTop + 40));
			FaceCenter = FVector(OpsPos.X, B.Max.Y + 0.5f, OpsTop + 22.f);
			FaceSize = FVector(28.f, 1.f, 22.f);
		}
		AStaticMeshActor* Face = Box(TEXT("OpsFace"), FaceCenter, FaceSize, FLinearColor(0.01f, 0.03f, 0.02f), 0.2f);
		UMaterialInstanceDynamic* Mid = nullptr;
		if (ScreenMat)
		{
			Mid = UMaterialInstanceDynamic::Create(ScreenMat, Face);
			Mid->SetTextureParameterValue(TEXT("Screen"), CrtTargets[3]);
			Mid->SetScalarParameterValue(TEXT("Glow"), 0.f);
			Face->GetStaticMeshComponent()->SetMaterial(0, Mid);
		}
		CrtFaces[3] = Mid;
		Tag(Face, TEXT("opsTerminal"));
		if (Body) Tag(Body, TEXT("opsTerminal"));
		Spots.Add(TEXT("ops"), FTransform(FRotator(-20, -90, 0), FVector(OpsPos.X, OpsPos.Y + 150, 92)));
	}
	Model(TEXT("OpsChair"), TEXT("Chair"), FVector(OpsPos.X, OpsPos.Y + 110, 0), 90.f);
	if (!Licensed(TEXT("Oscilloscope"), TEXT("oscilloscope"), TEXT("oscilloscope"), FVector(OpsPos.X - 75, OpsPos.Y + 5, OpsTop), 270.f))
	{
		Model(TEXT("Transceiver"), TEXT("Transceiver"), FVector(OpsPos.X - 70, OpsPos.Y, OpsTop), 0.f);
	}
	Licensed(TEXT("SignalAnalyzer"), TEXT("console_kit"), TEXT("signal_analyzer"), FVector(OpsPos.X + 75, OpsPos.Y + 5, OpsTop), 270.f);
	Licensed(TEXT("FilingCabinet"), TEXT("filing_cabinet"), TEXT("filing_cabinet_rec"), FVector(-420, -RoomY / 2 + 35, 0), 270.f);
	Licensed(TEXT("FilingCabinet2"), TEXT("filing_cabinet"), TEXT("filing_cabinet_ctrl"), FVector(-420, -RoomY / 2 + 95, 0), 270.f);
	Licensed(TEXT("ArchiveBox"), TEXT("archive_box"), TEXT("archive_box"), FVector(-380, RoomY / 2 - 60, 0), 20.f);
	// the coffee machine in the south-west corner, the radio on the cabinets, the jacket on its hook
	Box(TEXT("CoffeeTable"), FVector(-400, -RoomY / 2 + 200, 40), FVector(50, 60, 80), FLinearColor(0.25f, 0.24f, 0.22f), 0.7f);
	Tag(Box(TEXT("CoffeePot"), FVector(-400, -RoomY / 2 + 200, 97), FVector(22, 26, 34), FLinearColor(0.12f, 0.12f, 0.12f), 0.4f), TEXT("coffeePot"));
	Box(TEXT("CoffeeJug"), FVector(-392, -RoomY / 2 + 200, 92), FVector(14, 14, 18), FLinearColor(0.3f, 0.2f, 0.12f), 0.15f);
	Tag(Box(TEXT("Radio"), FVector(-420, -RoomY / 2 + 65, 128), FVector(18, 30, 14), FLinearColor(0.35f, 0.25f, 0.15f), 0.6f), TEXT("radio"));
	Jacket = Box(TEXT("Jacket"), FVector(-RoomX / 2 + 8, -400, 150), FVector(10, 40, 70), FLinearColor(0.2f, 0.26f, 0.2f), 0.9f);
	Tag(Jacket.Get(), TEXT("jacket"));
	Box(TEXT("JacketBand"), FVector(-RoomX / 2 + 2, -400, 150), FVector(1, 42, 6), FLinearColor(0.8f, 0.8f, 0.7f), 0.3f, FRotator::ZeroRotator, FLinearColor(0.2f, 0.2f, 0.15f));
	// Ceiling fluorescents and the lights that go with them.
	Model(TEXT("TubeW"), TEXT("TwinFluorescent"), FVector(0, -300, RoomZ - 2), 0.f);
	Model(TEXT("TubeE"), TEXT("TwinFluorescent"), FVector(0, 300, RoomZ - 2), 0.f);
	Lamp(TEXT("CeilingW"), FVector(0, -300, RoomZ - 25), FLinearColor(1.0f, 0.89f, 0.72f), 5.f, 1300.f, true, &CeilingLights);
	Lamp(TEXT("CeilingE"), FVector(0, 300, RoomZ - 25), FLinearColor(1.0f, 0.89f, 0.72f), 4.f, 1300.f, true, &CeilingLights);
	Lamp(TEXT("OpsLampLight"), FVector(OpsPos.X + 50, OpsPos.Y + 20, OpsTop + 45), FLinearColor(1.0f, 0.66f, 0.35f), 2.5f, 500.f, true, &LampLights);
	Model(TEXT("WallLamp"), TEXT("WallLamp"), FVector(-RoomX / 2 + 8, 300, 220), 0.f);
	Lamp(TEXT("WallLampLight"), FVector(-RoomX / 2 + 25, 300, 215), FLinearColor(1.0f, 0.78f, 0.5f), 1.2f, 500.f, true, &LampLights);
	for (UPointLightComponent* L : CeilingLights) CeilingBase.Add(L->Intensity);
	for (UPointLightComponent* L : LampLights) LampBase.Add(L->Intensity);
	{
		FActorSpawnParameters P;
		P.Name = FName(TEXT("CrtGlow"));
		APointLight* L = GetWorld()->SpawnActor<APointLight>(APointLight::StaticClass(), FTransform(FVector(DeskPos.X - 40, 0, DeskTop + 40)), P);
		L->PointLightComponent->SetMobility(EComponentMobility::Movable);
		L->PointLightComponent->SetIntensityUnits(ELightUnits::Candelas);
		L->PointLightComponent->SetIntensity(0.f);
		L->PointLightComponent->SetLightColor(FLinearColor(0.36f, 1.0f, 0.54f));
		L->PointLightComponent->SetAttenuationRadius(360.f);
		L->PointLightComponent->SetCastShadows(false);
		CrtLight = L->PointLightComponent;
		Parts.Add(L);
	}
}

void AS47Saro::BuildPapers()
{
	// The wall clock and the map on the south wall, the Halley poster by the door, the telex in the corner.
	const float SX = -RoomX / 2 + 2;
	Box(TEXT("ClockFace"), FVector(SX, 100, 225), FVector(2, 36, 36), FLinearColor(0.9f, 0.9f, 0.86f), 0.6f);
	Tag(Box(TEXT("ClockRim"), FVector(SX - 0.5f, 100, 225), FVector(1, 40, 40), FLinearColor(0.08f, 0.08f, 0.08f), 0.4f), TEXT("clock"));
	auto Hand = [this](const TCHAR* Name, float Len, float Wd, const FLinearColor& C) -> AActor*
	{
		FActorSpawnParameters P;
		P.Name = FName(Name);
		AActor* A = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform(FRotator::ZeroRotator, FVector(-RoomX / 2 + 3.5f, 100, 225)), P);
		USceneComponent* Root = NewObject<USceneComponent>(A, TEXT("Root"));
		A->SetRootComponent(Root);
		Root->RegisterComponent();
		UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(A, TEXT("Hand"));
		M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		M->SetRelativeLocation(FVector(0, 0, Len / 2));
		M->SetRelativeScale3D(FVector(0.006f, Wd / 100.f, Len / 100.f));
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (UMaterialInstanceDynamic* Mid = FlatMid(A, C, 0.5f)) M->SetMaterial(0, Mid);
		M->SetupAttachment(Root);
		M->RegisterComponent();
		Parts.Add(A);
		return A;
	};
	HandH = Hand(TEXT("HandH"), 10.f, 1.6f, FLinearColor(0.05f, 0.05f, 0.05f));
	HandM = Hand(TEXT("HandM"), 15.f, 1.2f, FLinearColor(0.05f, 0.05f, 0.05f));
	HandS = Hand(TEXT("HandS"), 16.f, 0.5f, FLinearColor(0.7f, 0.1f, 0.08f));
	Tag(Box(TEXT("Map"), FVector(SX, 320, 190), FVector(1.5f, 110, 80), FLinearColor(0.78f, 0.74f, 0.6f), 0.9f), TEXT("map"));
	Box(TEXT("MapX"), FVector(SX - 0.5f, 330, 185), FVector(1, 4, 4), FLinearColor(0.7f, 0.1f, 0.08f), 0.9f);
	Tag(Box(TEXT("Halley"), FVector(SX, -150, 200), FVector(1.5f, 60, 85), FLinearColor(0.12f, 0.16f, 0.3f), 0.9f), TEXT("halley"));
	// the teleprinter in the south-east corner with its roll
	Tag(Box(TEXT("Telex"), FVector(-RoomX / 2 + 60, RoomY / 2 - 80, 50), FVector(50, 50, 100), FLinearColor(0.3f, 0.3f, 0.28f), 0.6f), TEXT("telex"));
	Box(TEXT("TelexRoll"), FVector(-RoomX / 2 + 48, RoomY / 2 - 80, 103), FVector(26, 24, 6), Paper, 0.9f);
	Box(TEXT("TelexPaper"), FVector(-RoomX / 2 + 36, RoomY / 2 - 80, 80), FVector(0.4f, 22, 50), Paper, 0.9f);
}

void AS47Saro::BuildYard()
{
	Box(TEXT("Ground"), FVector(0, 0, -7), FVector(60000, 60000, 10), Sand, 0.95f);
	Box(TEXT("Roof"), FVector(0, 0, RoomZ + 20), FVector(RoomX + 2 * Wall + 20, RoomY + 2 * Wall + 20, 20), Concrete, 0.9f);
	Box(TEXT("Apron"), FVector(RoomX / 2 + Wall + 150, 0, -4), FVector(300, RoomY + 2 * Wall, 6), Concrete, 0.9f);
	// The chain-link fence between the windows side and the array (web z -20: X 2000), with the gate.
	for (int32 k = -12; k <= 12; ++k)
	{
		if (k == 0) continue;
		Box(FString::Printf(TEXT("FencePost%d"), k + 12), FVector(2000, k * 300.f, 100), FVector(6, 6, 200), FrameGrey, 0.6f);
	}
	Box(TEXT("FenceRailW"), FVector(2000, -1950, 195), FVector(3, 3600, 3), FrameGrey, 0.6f);
	Box(TEXT("FenceRailE"), FVector(2000, 1950, 195), FVector(3, 3600, 3), FrameGrey, 0.6f);
	Model(TEXT("Generator"), TEXT("Generator"), FVector(-150, RoomY / 2 + Wall + 160, 0), 0.f);
	Model(TEXT("Cabinet"), TEXT("Cabinet"), FVector(100, RoomY / 2 + Wall + 40, 0), 90.f);
	AStaticMeshActor* Pole = Model(TEXT("LampPole"), TEXT("LampPole"), FVector(900, 750, 0), 0.f);
	float PoleTop = 450.f;
	if (Pole) PoleTop = Bounds(Pole).Max.Z;
	Lamp(TEXT("YardLight"), FVector(880, 740, PoleTop - 30), FLinearColor(1.0f, 0.8f, 0.55f), 14.f, 2500.f);
	FRng R(1986);
	static const TCHAR* Tufts[] = { TEXT("TuftA"), TEXT("TuftB"), TEXT("TuftC") };
	for (int32 i = 0; i < 160; ++i)
	{
		const float A = R.Next() * 2 * PI, D = 900.f + R.Next() * 9000.f;
		const FVector P(FMath::Cos(A) * D, FMath::Sin(A) * D, 0);
		if (FMath::Abs(P.X) < RoomX / 2 + 150 && FMath::Abs(P.Y) < RoomY / 2 + 150) continue;
		Model(FString::Printf(TEXT("Tuft%d"), i), Tufts[i % 3], P, R.Next() * 360.f, 0.f, 0.8f + R.Next() * 0.6f);
	}
	static const TCHAR* Stones[] = { TEXT("Boulder"), TEXT("SandstoneA"), TEXT("SandstoneB") };
	for (int32 i = 0; i < 24; ++i)
	{
		const float A = R.Next() * 2 * PI, D = 1500.f + R.Next() * 9000.f;
		const FVector P(FMath::Cos(A) * D, FMath::Sin(A) * D, 0);
		if (FMath::Abs(P.X) < RoomX / 2 + 200 && FMath::Abs(P.Y) < RoomY / 2 + 200) continue;
		Model(FString::Printf(TEXT("Stone%d"), i), Stones[i % 3], P, R.Next() * 360.f, 0.f, 0.7f + R.Next() * 0.9f);
	}
}

void AS47Saro::BuildArray()
{
	for (const FDishDef& D : DishLayout)
	{
		FDish Dish;
		Dish.Id = D.Id;
		Dish.Base = FVector(float(-D.Z * 100.0), float(D.X * 100.0), 0);
		Dish.bLight = D.bLight;
		AStaticMeshActor* Ped = Model(FString::Printf(TEXT("%sPedestal"), D.Id), TEXT("DishPedestal"), Dish.Base, 0.f, 0.f, 1.f, &DishWhite, 0.6f);
		float Top = 236.f;
		if (Ped) Top = Bounds(Ped).Max.Z;
		AStaticMeshActor* Bowl = Model(FString::Printf(TEXT("%sBowl"), D.Id), TEXT("DishBowl"), FVector(Dish.Base.X, Dish.Base.Y, Top), float(Dish.CurAz), float(Dish.CurEl), 1.f, &DishWhite, 0.6f);
		Dish.Bowl = Bowl;
		// the drive cabinet at the base, as on the web version's aprons
		Box(FString::Printf(TEXT("%sCabinet"), D.Id), Dish.Base + FVector(-160, 0, 40), FVector(40, 60, 80), FrameGrey, 0.6f);
		Dishes.Add(Dish);
	}
}

void AS47Saro::BuildSkyAndLight()
{
	// Stars: a texture made here, points of varied brightness on near black, on the inside of
	// a big sphere. Fictional sky; no catalogue. The sphere's V runs from one pole to the other,
	// so the stars go on the whole texture and the ground hides the half below the horizon.
	const int32 TW = 2048, TH = 1024;
	Stars = UTexture2D::CreateTransient(TW, TH, PF_B8G8R8A8);
	if (Stars)
	{
		TArray<FColor> Px;
		Px.SetNum(TW * TH);
		for (int32 Y = 0; Y < TH; ++Y)
		{
			const float H = FMath::Clamp(1.f - FMath::Abs(Y - TH * 0.5f) / (TH * 0.08f), 0.f, 1.f);
			const FColor Row(uint8(2 + 8 * H), uint8(3 + 10 * H), uint8(8 + 22 * H), 255);
			for (int32 X = 0; X < TW; ++X) Px[Y * TW + X] = Row;
		}
		FRng R(47);
		for (int32 i = 0; i < 7000; ++i)
		{
			const int32 X = 1 + int32(R.Next() * (TW - 2)), Y = 1 + int32(R.Next() * (TH - 2));
			const float B = R.Next();
			const uint8 V = uint8(120 + B * B * 135);
			const FColor C(V, V, uint8(FMath::Min(255, int32(V) + 20)), 255);
			Px[Y * TW + X] = C;
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
		AStaticMeshActor* A = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(FRotator::ZeroRotator, FVector(0, 0, -2000), FVector(500.f)), P);
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

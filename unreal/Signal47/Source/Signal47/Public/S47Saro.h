#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "S47Saro.generated.h"

class AStaticMeshActor;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture2D;
class UTextureRenderTarget2D;
class UPointLightComponent;
class FWidgetRenderer;
class SWidget;

// SARO on the night of the prologue, built at run time so the layout can be read and changed
// as text: the control room (12 by 9 m, window to the north, as in the web version), the yard,
// the 27 antennas of the array, the sky and the lights. Models come from the Blender sources in
// the Unity archive (Scripts/import-models.py) and, when the private clone is there, from the
// bought Vintage Liminal Props pack (Scripts/import-licensed.py); the rest is basic shapes.
// Axes: X north, Y east, Z up, in centimetres. The web version's (x, y, z) in metres is (Y, Z, -X) / 100.
// Everything the story can touch has an id (actor tag "Use:<id>"), and the story drives it
// through the setters below (lever, lamps, screens, printer, handset, mug, dishes, lights).
UCLASS()
class SIGNAL47_API AS47Saro : public AActor
{
	GENERATED_BODY()

public:
	AS47Saro();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	// Spots to stand at (-S47Face): room, crt, phone, window, outside, dish, rack, ops, printer, log.
	FTransform StandingSpot(const FString& Id) const;
	FVector PropLocation(const FString& Id) const;
	void ToggleDoor(const FString& Id);
	bool IsDoorOpen(const FString& Id) const;
	int32 PartCount() const { return Parts.Num(); }

	// What the story drives.
	void SetLever(bool bUp);
	void SetLeds(bool bOn);
	void SetLed(int32 Index, bool bOn);
	void FlickerLed();
	void SetCrtPowered(int32 Screen, bool bOn); // 0 left (status), 1 centre (spectrum), 2 right (tracking), 3 ops
	void SetCrtGlow(float Intensity);
	void SetPrinterPaper(float Share); // 0..1 fed out, below 0 hidden
	void SetHandsetVisible(bool bOn);
	void SetHandsetRattle(bool bOn);
	bool IsMugVisible() const;
	void SetCoffee(bool bFull);
	void DropMug();
	void ResetMug();
	TFunction<void(const FVector&)> OnMugShatter;
	void SetJacketVisible(bool bOn);
	void SetClock(double SecondsAfterMidnight);
	void SetLightsScale(float Ceiling, float Lamps);
	void SetTelexRoll(const TArray<FString>& Lines);

	// The array: 27 antennas, az and el per dish, slewing at a rate in degrees per second.
	struct FDish { FString Id; FVector Base; double CurAz = 18, CurEl = 48, TgtAz = 18, TgtEl = 48, Slew = 6; TWeakObjectPtr<AStaticMeshActor> Bowl; bool bLight = false; };
	void PointDishes(double Az, double El, double Slew);
	void SnapDishes(double Az, double El);
	bool DishesMoving() const;
	int32 DishCount() const { return Dishes.Num(); }
	double DishAz(int32 I) const { return Dishes.IsValidIndex(I) ? Dishes[I].CurAz : 0; }
	double DishEl(int32 I) const { return Dishes.IsValidIndex(I) ? Dishes[I].CurEl : 0; }
	bool DishMoving(int32 I) const;
	FString DishId(int32 I) const { return Dishes.IsValidIndex(I) ? Dishes[I].Id : FString(); }

	// The four CRT screens are render targets; the HUD draws the pictures into them.
	UTextureRenderTarget2D* CrtTarget(int32 Screen) const { return CrtTargets.IsValidIndex(Screen) ? CrtTargets[Screen].Get() : nullptr; }
	static constexpr int32 CrtW = 512, CrtH = 384;
	bool IsCrtPowered(int32 Screen) const { return CrtOn.IsValidIndex(Screen) && CrtOn[Screen]; }

	static constexpr float RoomX = 900.f, RoomY = 1200.f, RoomZ = 300.f, Wall = 30.f;

private:
	UPROPERTY() TObjectPtr<UMaterialInterface> Flat;
	UPROPERTY() TObjectPtr<UMaterialInterface> Sky;
	UPROPERTY() TObjectPtr<UMaterialInterface> ScreenMat;
	UPROPERTY() TArray<TObjectPtr<AActor>> Parts;
	UPROPERTY() TObjectPtr<UTexture2D> Stars;
	UPROPERTY() TArray<TObjectPtr<UTextureRenderTarget2D>> CrtTargets;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> CrtFaces;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> LedMids;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> CeilingLights;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> LampLights;
	UPROPERTY() TObjectPtr<UPointLightComponent> CrtLight;
	TArray<float> CeilingBase, LampBase;
	TArray<bool> CrtOn;
	TArray<bool> LedOn;
	TMap<FString, TWeakObjectPtr<AActor>> Doors;
	TMap<FString, bool> DoorOpen;
	TMap<FString, FTransform> Spots;
	TMap<FString, TWeakObjectPtr<AActor>> Props;
	TArray<FDish> Dishes;
	TWeakObjectPtr<AActor> Lever, PrinterPaper, Handset, Mug, Coffee, Jacket, HandH, HandM, HandS;
	FVector MugHome;
	struct FMugFall { float T = 0; FVector V; float Spin = 0; int32 Stage = 0; };
	TOptional<FMugFall> MugFall;
	float HandsetT = 0.f;
	bool bHandsetRattle = false;

	UMaterialInstanceDynamic* FlatMid(UObject* Outer, const FLinearColor& Color, float Roughness, float Metallic = 0.f, const FLinearColor& Emissive = FLinearColor::Black);
	AStaticMeshActor* Box(const FString& Name, const FVector& Center, const FVector& Size, const FLinearColor& Color, float Roughness, const FRotator& Rot = FRotator::ZeroRotator, const FLinearColor& Emissive = FLinearColor::Black);
	AStaticMeshActor* Model(const FString& Name, const FString& Asset, const FVector& Pos, float Yaw = 0.f, float Pitch = 0.f, float Scale = 1.f, const FLinearColor* Tint = nullptr, float Roughness = 0.6f);
	UStaticMesh* Mesh(const FString& Asset) const;
	// A mesh from the bought pack (/Game/Licensed/Models/<pack>/StaticMeshes/<mesh>), or null when the pack is not there.
	UStaticMesh* LicensedMesh(const FString& Pack, const FString& MeshName) const;
	AStaticMeshActor* Licensed(const FString& Name, const FString& Pack, const FString& MeshName, const FVector& Pos, float Yaw = 0.f, float Scale = 1.f);
	void Lamp(const FString& Name, const FVector& Pos, const FLinearColor& Color, float Cd, float Radius, bool bShadows = true, TArray<TObjectPtr<UPointLightComponent>>* Keep = nullptr);
	void Door(const FString& Id, const FVector& HingeBottom, float Width, float Height, float HingeYaw);
	void Tag(AActor* A, const FString& Id);
	FBox Bounds(AActor* A) const;
	void BuildRoom();
	void BuildFurniture();
	void BuildPapers();
	void BuildYard();
	void BuildArray();
	void BuildSkyAndLight();
	AStaticMeshActor* CrtScreen(int32 Screen, const FString& Name, const FVector& Pos, float Yaw, const FString& UseId);
};

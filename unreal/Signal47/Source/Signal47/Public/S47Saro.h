#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "S47Saro.generated.h"

class AStaticMeshActor;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture2D;

// SARO on the night of the prologue, built at run time so the layout can be read and changed
// as text: the control room (12 by 9 m, window to the north, as in the web version), the yard
// with the dishes, the sky and the lights. Models come from the Blender sources in the Unity
// archive (Scripts/import-models.py); the rest is the engine's basic shapes.
// Axes: X north, Y east, Z up, in centimetres. The web version's (x, y, z) is (Y, Z, -X).
UCLASS()
class SIGNAL47_API AS47Saro : public AActor
{
	GENERATED_BODY()

public:
	AS47Saro();
	virtual void BeginPlay() override;

	// Where to stand for a named spot: "room" (the middle), "crt", "phone", "window", "dish", "outside".
	FTransform StandingSpot(const FString& Id) const;
	// A door is opened or closed by its id ("door-east", "door-south").
	void ToggleDoor(const FString& Id);
	bool IsDoorOpen(const FString& Id) const;
	void SetCrtPowered(bool bOn);
	int32 PartCount() const { return Parts.Num(); }

	static constexpr float RoomX = 900.f, RoomY = 1200.f, RoomZ = 300.f, Wall = 30.f;

private:
	UPROPERTY() TObjectPtr<UMaterialInterface> Flat;
	UPROPERTY() TObjectPtr<UMaterialInterface> Sky;
	UPROPERTY() TArray<TObjectPtr<AActor>> Parts;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CrtFace;
	UPROPERTY() TObjectPtr<UTexture2D> Stars;
	TMap<FString, TWeakObjectPtr<AActor>> Doors;
	TMap<FString, bool> DoorOpen;
	TMap<FString, FTransform> Spots;

	UMaterialInstanceDynamic* FlatMid(UObject* Outer, const FLinearColor& Color, float Roughness, float Metallic = 0.f, const FLinearColor& Emissive = FLinearColor::Black);
	AStaticMeshActor* Box(const FString& Name, const FVector& Center, const FVector& Size, const FLinearColor& Color, float Roughness, const FRotator& Rot = FRotator::ZeroRotator, const FLinearColor& Emissive = FLinearColor::Black);
	// Tint: the colour for models whose FBX carries no named material (the Refined set has one
	// unnamed slot; its colours lived in Unity materials the FBX does not reference).
	AStaticMeshActor* Model(const FString& Name, const FString& Asset, const FVector& Pos, float Yaw = 0.f, float Pitch = 0.f, float Scale = 1.f, const FLinearColor* Tint = nullptr, float Roughness = 0.6f);
	UStaticMesh* Mesh(const FString& Asset) const;
	void Lamp(const FString& Name, const FVector& Pos, const FLinearColor& Color, float Cd, float Radius, bool bShadows = true);
	void Door(const FString& Id, const FVector& HingeBottom, float Width, float Height, float HingeYaw);
	void BuildRoom();
	void BuildFurniture();
	void BuildYard();
	void BuildSkyAndLight();
};

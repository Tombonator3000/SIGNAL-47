#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "S47Web.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class USpotLightComponent;
class UDirectionalLightComponent;
class USkyLightComponent;
class UTextureRenderTarget2D;
class UTexture2D;

// A walkable rectangle or a collider, in the web version's terms (metres, x and z).
struct FS47Rect { double MinX = 0, MaxX = 0, MinZ = 0, MaxZ = 0; FString Id; bool bEnabled = true; };

// One node of the web scene: its three.js local matrix (kept as position, quaternion and
// scale so the story can move it as the web version does), and the component that draws it.
struct FS47WebNode
{
	int32 Parent = -1;
	FString Name;
	double Local[16];
	FVector P = FVector::ZeroVector, S = FVector::OneVector;
	FQuat Q = FQuat::Identity;
	bool bVis = true;
	int32 Geo = -1;
	TArray<int32> Mats;
	TArray<int32> Children;
	TWeakObjectPtr<USceneComponent> Comp;
	bool bInstanced = false;
	int32 Root = -1; // the top-level node it hangs under
};

struct FS47WebItem { FString Id; int32 Node = -1; double Range = 2.4; bool bLook = false; FVector Min, Max; bool bBox = false; };

// The web version's whole world, rebuilt from its own scene (Tools/webexport): every node, mesh,
// material, texture, light, floodlight, glow point, door, antenna, interactable thing and walk
// zone, in the same places. AS47Story drives the moving parts through the setters below, which
// do what the web version's ControlRoom, Exterior and Doors do.
UCLASS()
class SIGNAL47_API AS47Web : public AActor
{
	GENERATED_BODY()

public:
	AS47Web();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	bool IsLoaded() const { return bLoaded; }
	int32 PartCount() const { return Nodes.Num(); }
	FString Summary() const;

	// ---------- nodes ----------
	int32 Handle(const FString& Key) const; // "room.objs.mug" and the like, -1 when unknown
	int32 HandleMat(const FString& Key) const;
	int32 HandleTex(const FString& Key) const;
	void SetVisible(int32 Node, bool bOn);
	bool IsVisible(int32 Node) const { return Nodes.IsValidIndex(Node) && Nodes[Node].bVis; }
	void SetLocal(int32 Node, const FVector& P, const FQuat& Q, const FVector& S);
	void SetPosition(int32 Node, const FVector& P) { if (Nodes.IsValidIndex(Node)) SetLocal(Node, P, Nodes[Node].Q, Nodes[Node].S); }
	void SetRotation(int32 Node, double X, double Y, double Z) { if (Nodes.IsValidIndex(Node)) SetLocal(Node, Nodes[Node].P, S47ThreeEuler(X, Y, Z), Nodes[Node].S); }
	void SetScale(int32 Node, const FVector& S) { if (Nodes.IsValidIndex(Node)) SetLocal(Node, Nodes[Node].P, Nodes[Node].Q, S); }
	const FS47WebNode* Node(int32 I) const { return Nodes.IsValidIndex(I) ? &Nodes[I] : nullptr; }
	FVector WorldThree(int32 Node) const; // three.js world position of a node
	FVector WorldUE(int32 Node) const;

	// ---------- areas ----------
	void Enter(const FString& Area);
	FString Area() const { return CurrentArea; }
	const TArray<FS47Rect>& Zones() const;
	const TArray<FS47Rect>& Colliders() const;
	TArray<FS47Rect>& ZonesMutable(const FString& A) { return Walks.FindOrAdd(A).Key; }
	TArray<FS47Rect>& CollidersMutable(const FString& A) { return Walks.FindOrAdd(A).Value; }

	// ---------- interactable things ----------
	const TArray<FS47WebItem>& Items() const { return ItemList; }
	const FS47WebItem* Item(const FString& Id) const;
	// The item a component belongs to (it, or a node above it), or null.
	const FS47WebItem* ItemOf(const UPrimitiveComponent* C) const;
	FVector PropLocation(const FString& Id) const; // Unreal position, for sounds

	// ---------- the control room (ControlRoom.ts) ----------
	void SetLever(bool bOn);
	void SetLeds(bool bOn);
	void SetLed(int32 Index, bool bOn);
	int32 LedCount() const { return 18; }
	void FlickerLed();
	void SetCrtPowered(int32 Screen, bool bOn); // 0 left, 1 centre, 2 right, 3 ops
	bool IsCrtPowered(int32 Screen) const { return CrtOn.IsValidIndex(Screen) && CrtOn[Screen]; }
	void SetCrtGlow(float Intensity);
	UTextureRenderTarget2D* CrtTarget(int32 Screen) const { return CrtTargets.IsValidIndex(Screen) ? CrtTargets[Screen].Get() : nullptr; }
	static constexpr int32 CrtW = 512, CrtH = 384;
	void SetPrinterPaper(float Share);
	void SetHandsetVisible(bool bOn);
	void SetHandsetRattle(bool bOn) { bRattle = bOn; }
	bool IsMugVisible() const;
	void SetCoffee(bool bFull) { CoffeeState = bFull ? 1 : 0; ApplyCoffee(); }
	void SetCoffeeLevel(double Y);
	void DropMug();
	void ResetMug();
	TFunction<void(const FVector&)> OnMugShatter; // Unreal position
	void SetJacketVisible(bool bOn);
	void SetClock(double Seconds);
	void SetLightsScale(float Ceiling, float Lamps);
	void SetTubes(bool bOn, int32 KeepOff = -1);

	// ---------- the array (Exterior.ts, Dish.ts) ----------
	void PointDishes(double Az, double El, double Slew);
	void SnapDishes(double Az, double El);
	bool DishesMoving() const;
	int32 DishCount() const { return Dishes.Num(); }
	double DishAz(int32 I) const { return Dishes.IsValidIndex(I) ? Dishes[I].CurAz : 0; }
	double DishEl(int32 I) const { return Dishes.IsValidIndex(I) ? Dishes[I].CurEl : 0; }
	bool DishMoving(int32 I) const;
	FString DishId(int32 I) const { return Dishes.IsValidIndex(I) ? Dishes[I].Id : FString(); }

	// ---------- doors (Doors.ts) ----------
	bool IsDoor(const FString& ItemId) const;
	FString DoorLabel(const FString& ItemId) const;
	void ToggleDoor(const FString& ItemId);
	bool IsDoorOpen(const FString& ItemId) const;
	void CloseAllDoors();
	TFunction<void(const FString&, bool, const FVector&, bool)> OnDoor; // id, open, where (UE), heavy

	// ---------- the sky, lights and floods ----------
	void SetPlayerEye(const FVector& UE) { Eye = UE; }
	FVector SpawnThree() const { return Spawn; } // x, yaw, z
	double ToneMappingExposure = 1.05;
	float ExposureOffset = 0.f; // Unreal against three.js, set by -S47Exposure= for calibration
	float SceneScale = 1.f;     // and -S47Scene=, a factor on the light going into the tone curve
	FLinearColor FogColor = FLinearColor(0.0155f, 0.0212f, 0.0353f);
	float FogDensity = 0.0021f;

private:
	static FQuat S47ThreeEuler(double X, double Y, double Z);
	bool bLoaded = false;
	TArray<FS47WebNode> Nodes;
	TArray<FString> GeoPaths;
	TMap<FString, int32> Handles, HandleMats, HandleTexs;
	TMap<FString, TArray<int32>> AreaRoots;
	TMap<int32, FString> RootArea;
	TMap<FString, TPair<TArray<FS47Rect>, TArray<FS47Rect>>> Walks;
	FString CurrentArea = TEXT("saro");
	TArray<FS47WebItem> ItemList;
	TMap<int32, int32> NodeItem;
	TMap<const UPrimitiveComponent*, int32> CompNode;
	UPROPERTY() TArray<TObjectPtr<UMaterialInterface>> MatCache;
	TArray<bool> MatLoaded;
	UMaterialInterface* Mat(int32 Id);

	// lights, per area
	struct FLightRef { TWeakObjectPtr<ULightComponent> L; FString Area; float Base = 0; FString Uuid; };
	TArray<FLightRef> Lights;
	UPROPERTY() TObjectPtr<class UMaterialParameterCollection> Env;
	struct FHemi { FLinearColor Sky, Ground; float Intensity; };
	TMap<FString, FHemi> Hemis;
	ULightComponent* LightByUuid(const FString& Uuid) const;
	TArray<TWeakObjectPtr<UPointLightComponent>> CeilingLights, LampLights;
	TArray<float> CeilingBase, LampBase;
	TWeakObjectPtr<UPointLightComponent> CrtLight;
	float CrtLightBase = 0.9f;

	// the sky picture, per area, on a sphere round the eye
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SkySphere;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SkyMid;
	TMap<FString, FString> SkyTex;
	FVector Eye = FVector::ZeroVector;

	// the control room
	UPROPERTY() TArray<TObjectPtr<UTextureRenderTarget2D>> CrtTargets;
	TArray<bool> CrtOn;
	int32 CrtMat[4] = { -1, -1, -1, -1 };
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> CrtMids; // one per screen, shared by its slots
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> KeepMids;
	UPROPERTY() TArray<TObjectPtr<UTexture2D>> FloodTex;
	int32 LeverNode = -1, LedNode = -1, PaperNode = -1, HandsetNode = -1, MugNode = -1, CoffeeNode = -1, JacketNode = -1;
	int32 HandH = -1, HandM = -1, HandS = -1;
	TArray<int32> TubeNodes;
	int32 TubeOnMat = -1;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> TubeOffMid;
	FVector MugHome = FVector::ZeroVector;
	int32 CoffeeState = 0;
	double CoffeeY = 0.088;
	bool bRattle = false;
	float RattleT = 0.f;
	struct FMugFall { float T = 0; FVector V; float Spin = 0; int32 Stage = 0; FVector Rot; };
	TOptional<FMugFall> MugFall;
	void ApplyCoffee();

	// the dishes: pointing and the instanced parts that draw them
	struct FDish { FString Id; double X = 0, Z = 0, Scale = 1, CurAz = 18, CurEl = 48, TgtAz = 18, TgtEl = 48, Slew = 6; };
	TArray<FDish> Dishes;
	struct FDishPart { int32 Node = -1; int32 Level = 0; TArray<int32> DishIdx; };
	TArray<FDishPart> DishParts;
	void DrawDishes();

	// doors
	struct FDoor { FString Id, Inter, Name, Area, Sound; double Range = 2.4; bool bOpen = false; double K = 0, From = 0, T0 = -1; TArray<int32> MovedNodes; TArray<TArray<double>> Shut, Open; TArray<int32> Zones; TOptional<FS47Rect> Leaf; int32 Proxy = -1; };
	TArray<FDoor> Doors;
	double DoorT = 0;
	void ApplyDoor(FDoor& D, double K);
	void SetDoor(FDoor& D, bool bOpen, bool bInstant);
	FDoor* DoorByItem(const FString& ItemId);
	const FDoor* DoorByItem(const FString& ItemId) const;

	FVector Spawn = FVector::ZeroVector;

	bool Load();
	void Build();
	void BuildLights(const TArray<TSharedPtr<class FJsonValue>>& Json);
	void BuildFloods(const TArray<TSharedPtr<class FJsonValue>>& Json);
	void BuildRoom();
	void ApplyVisibility(int32 Node, bool bParent);
	void UpdateCrtMaterials();
};

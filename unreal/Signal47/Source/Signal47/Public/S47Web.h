#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "S47Obj.h"
#include "S47Glow.h"
#include "S47Graphics.h"
#include "S47Web.generated.h"

struct FPostProcessSettings;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class USpotLightComponent;
class URectLightComponent;
class UDirectionalLightComponent;
class USkyLightComponent;
class UTextureRenderTarget2D;
class UTexture2D;
class ULightComponent;
class ULocalLightComponent;
class UPrimitiveComponent;
class FS47SaroArrival;
class FS47SaroSite;
class FS47FleetParked;
class FS47TownBuildings;
class FS47CalicheHomes;
class FS47CinemaPosters;
class FJsonObject;
class FS47MotelV2;
class FS47RouteNature;
struct FS47HighwayRanch;

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
	int32 GeoB = -1; // a pair of flora kinds in one mesh: the second kind's shape (export.js pairGeo)
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
	void SetLightingQuality(const FString& Quality);
	void ApplyGraphicsSettings(const FS47GraphicsSettings& Settings);
	const FS47GraphicsSettings& GetGraphicsSettings() const { return Graphics; }
	const FPostProcessSettings* GetGraphicsPostProcess() const;
	TSharedPtr<FJsonObject> GraphicsReadback() const;
	void SetSkyLighting(float Daylight, const FVector& SunDirection, float Sun, float Flash, float Evening = 0.f);
	/** SKY.md: the volumetric cloud layer. Cover/cirrus 0..1 from the clock; clear directions (three's
	 *  world terms, w = strength) keep story beats in open sky; Flash/FlashDir light them from inside. */
	void SetClouds(float Cover, float Cirrus, const FVector4& Clear0, const FVector4& Clear1, float Flash, const FVector& FlashDir, double Time, float PureSkyWeight = 0.f);
	float CloudCoverNow() const { return CloudCover; }
	bool CloudsActive() const;
	void SuppressFloodLight(int32 Set, int32 Index);
	float LightIntensity(const FString& Uuid) const;
	void SetLightIntensity(const FString& Uuid, float Value);
	int32 PartCount() const { return Nodes.Num(); }
	FString Summary() const;

	// ---------- the scene by the TypeScript code's paths (S47Obj.h) ----------
	FS47Obj Obj(const FString& Path);                 // "room.objs.mug"; invalid when unknown
	FS47Obj ObjAt(int32 Node) { return FS47Obj(this, Node); }
	FS47Light Light(const FString& PathOrUuid);
	FS47Mat Material(const FString& Path);            // a material handle ("yard.m.screen")
	FVector Vec(const FString& Path, const FVector& Default = FVector::ZeroVector) const; // a Vector3 the code holds
	FS47Area Rect(const FString& Path) const;         // a {minX, maxX, minZ, maxZ} the code holds
	FS47Pose Pose(const FString& Path) const;         // an {x, z, yaw} the code holds (an anchor)
	bool HasPath(const FString& Path) const;
	bool MesaDinerReady() const { return bMesaDinerReady; }
	bool HighwayLandscapeReady() const { return HighwayLandscapeRoot.IsValid(); }
	FVector HighwayMapEye(const FVector& ThreeEye) const;
	bool AuditHighwayLandscape(TArray<FString>& Lines) const;
	bool AuditHighwayRanch(TArray<FString>& Lines) const;
	bool AuditSaroArrival(TArray<FString>& Lines) const;
	bool AuditCalicheHomes(TArray<FString>& Lines) const;
	bool AuditCinemaPosters(TArray<FString>& Lines) const;
	TSharedPtr<FJsonObject> CinemaPostersReadback() const;
	TArray<FString> PathsUnder(const FString& Prefix) const;
	// what FS47Obj and FS47Mat use
	TArray<class UMaterialInstanceDynamic*> NodeMids(int32 Node);
	class UMaterialInstanceDynamic* SharedMid(int32 MatId);
	ULightComponent* LightByUuid(const FString& Uuid) const;
	FTransform NodeWorldThree(int32 Node) const;        // three.js world matrix of a node, as a transform in metres
	// A thing the player can use: its node and everything under it answer the use trace.
	void MarkUsable(int32 Node, const FString& Id);
	FString UsableAt(const UPrimitiveComponent* C) const; // the id of what a trace hit, or empty
	int32 NodeOf(const UPrimitiveComponent* C) const { const int32* N = CompNode.Find(C); return N ? *N : -1; }
	// (Unreal only) A part built here and not in the export (S47ConsoleKit.h: the page under the reel)
	// taken in as a node under Parent, so the things to use can name it (FS47Obj, MarkUsable). Its
	// transform stays the component's own: only its visibility follows the node's. Returns its index.
	int32 AdoptNode(UPrimitiveComponent* C, int32 Parent, const FString& Name);

	void SetFogDensity(float Density); // the scene's FogExp2 density (World.enter)

	// ---------- story lamp state (kit.ts floodSet / setFlood) ----------
	int32 FloodSet(const FString& Key) const;         // 'site', 'annex', ...; -1 when unknown
	void SetFlood(int32 Set, int32 Index, float Width); // setFlood(i, intensity, set)
	void SetFloodColor(int32 Set, int32 Index, const FLinearColor& C);
	void SetFloodScale(int32 Set, float Scale);
	// A pool moved, as kit.ts sets pos[i] (x, y, z, reach) and col[i] (the truck's headlights and
	// its cab, Truck.ts placeFloods): bUpload false leaves it for FlushFlood, so a set changed in
	// several slots is sent once.
	void SetFloodPool(int32 Set, int32 Index, const FVector& P, float Width, const FLinearColor& C, bool bUpload = true);
	void FlushFlood(int32 Set);
	// Apply changed pool state before a one-shot photo, without advancing fog or sky captures.
	void SyncFloodLights();
	// A pool as it is now (pos[i]: x, y, z and its width) and its colour
	// (world/saroPower.ts takes SARO's lamps down from what they are and puts them back).
	FLinearColor FloodPool(int32 Set, int32 Index) const { return FloodSets.IsValidIndex(Set) && FloodSets[Set].Pos.IsValidIndex(Index) ? FloodSets[Set].Pos[Index] : FLinearColor(0, -999, 0, 0); }
	FLinearColor FloodColOf(int32 Set, int32 Index) const { return FloodSets.IsValidIndex(Set) && FloodSets[Set].Col.IsValidIndex(Index) ? FloodSets[Set].Col[Index] : FLinearColor::Black; }
	// (FloodCount and FloodScaleOf: with the world life's below; the count is kit.ts's count in use)
	// core/cutouts.ts litMat: the wandering card switches to the shown area's flood set.
	void UseFlood(const FS47Obj& Object, int32 Set, float Fall);
	bool FloodPool(int32 Set, int32 Index, FVector& P, float& Width, FLinearColor& Color) const;
	void SetHemi(const FString& Area, const FLinearColor& Sky, const FLinearColor& Ground, float Intensity);

	// What the drive needs that only exists while the web game runs (export.js extras: drive):
	// each leg's obstacles, lots and headlight slots, each truck's own flood sets. Null when the
	// manifest is older than that.
	const TSharedPtr<class FJsonObject>& DriveData() const { return Drive; }

	// ---------- the world's own life (S47WorldLife) ----------
	// What the world's modules change every frame and keep in private fields (export.js lifeOf): the
	// sky's parts, the shared lights' slots, the light in the air, the life in the night, the windows'
	// light. Null when the manifest is older than that.
	const TSharedPtr<class FJsonObject>& LifeData() const { return Life; }
	// world/glow.ts: the GlowPoints of a points node (its billboards), or null
	FS47GlowPoints* Glow(int32 Node);
	FS47GlowPoints* Glow(const FString& Path) { return Glow(Handle(Path)); }
	void UpdateGlows(double T); // every GlowPoints.update(t), on main.ts's t
	// a points node's points as exported (x, y, z, r, g, b, size: seven floats a point), and the
	// stars' twinkle phases (aPh); null for a node without
	const TArray<float>* PointsOf(int32 Node) const { return PointsRaw.Find(Node); }
	const TArray<float>* PointsPhOf(int32 Node) const { return PointsPh.Find(Node); }
	// the geometry and the material assets, by the export's ids (for the parts the world's life builds)
	class UStaticMesh* GeoMesh(int32 Geo) const;
	UMaterialInterface* MatAsset(int32 Id) { return Mat(Id); }
	// a copy of a node's own material with all it has, lit by the same floods (a pair's second flora kind)
	class UMaterialInstanceDynamic* CopyMid(class UMaterialInstanceDynamic* Of);
	// a value of MPC_Web (the wind's, world/wind.ts)
	void SetEnvScalar(const FName& Name, float V);
	// the fake floodlights as kit.ts's FloodSet holds them: n slots, count in use, those never handed
	// to a real light (skip), each pool's place and strength (pos: x, y, z, w) and colour, the scale
	int32 FloodSetCount() const { return FloodSets.Num(); }
	FString FloodKey(int32 Set) const { return FloodSets.IsValidIndex(Set) ? FloodSets[Set].Key : FString(); }
	int32 FloodSlots(int32 Set) const { return FloodSets.IsValidIndex(Set) ? FloodSets[Set].W : 0; }
	int32 FloodCount(int32 Set) const { return FloodSets.IsValidIndex(Set) ? FloodSets[Set].Count : 0; }
	bool FloodSkip(int32 Set, int32 I) const { return FloodSets.IsValidIndex(Set) && FloodSets[Set].Skip.Contains(I); }
	FLinearColor FloodPos(int32 Set, int32 I) const { return FloodSets.IsValidIndex(Set) && FloodSets[Set].Pos.IsValidIndex(I) ? FloodSets[Set].Pos[I] : FLinearColor(0, -999, 0, 0); }
	FLinearColor FloodCol(int32 Set, int32 I) const { return FloodSets.IsValidIndex(Set) && FloodSets[Set].Col.IsValidIndex(I) ? FloodSets[Set].Col[I] : FLinearColor::Black; }
	float FloodScaleOf(int32 Set) const { return FloodSets.IsValidIndex(Set) ? FloodSets[Set].Scale : 0.f; }
	// main.ts: the scene's fog colour (the sky's haze, every frame), and the area's hemisphere light at
	// an intensity of its own (room.lights.hemi.intensity: the lightning's flash in it)
	void SetFogColor(const FLinearColor& C);
	void SetHemiIntensity(float Intensity);
	class UStaticMeshComponent* SkySphereComponent() const { return SkySphere; }

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
	void SetCrtFlicker(int32 Screen, float K) { if (Screen >= 0 && Screen < 4) CrtFlicker[Screen] = K; }
	UTextureRenderTarget2D* CrtTarget(int32 Screen) const { return CrtTargets.IsValidIndex(Screen) ? CrtTargets[Screen].Get() : nullptr; }
	static constexpr int32 CrtW = 512, CrtH = 384;
	// A screen in use (SS47ScreenView): its picture from another texture while it is (null: its own
	// render target again), and its glass as a rectangle in Unreal's space, the plane the picture is
	// on: the centre, its two directions (unit; Up runs up the picture, Across along it, either way)
	// and its size in centimetres. False when the export has no such screen.
	void SetCrtPicture(int32 Screen, class UTexture* Picture);
	bool CrtGlass(int32 Screen, FVector& Centre, FVector& Up, FVector& Across, FVector2D& Size) const;
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
	// the tubes one by one (room.tubes[k].material: world/saroPower.ts keeps them as they were)
	int32 TubeCount() const { return TubeNodes.Num(); }
	bool TubeLit(int32 K) const;
	void SetTube(int32 K, bool bLit);
	// room.lights.hemi.intensity = hemiBase * k (main.ts: the starlight as the eyes open, story/saroDark.ts)
	void SetHemiScale(float K);

	// ---------- the array (Exterior.ts, Dish.ts) ----------
	void PointDishes(double Az, double El, double Slew);
	void SnapDishes(double Az, double El);
	bool DishesMoving() const;
	int32 DishCount() const { return Dishes.Num(); }
	double DishAz(int32 I) const { return Dishes.IsValidIndex(I) ? Dishes[I].CurAz : 0; }
	double DishEl(int32 I) const { return Dishes.IsValidIndex(I) ? Dishes[I].CurEl : 0; }
	bool DishMoving(int32 I) const;
	FString DishId(int32 I) const { return Dishes.IsValidIndex(I) ? Dishes[I].Id : FString(); }
	// where a dish stands and its size (ext.dishes: x, z, scale)
	double DishX(int32 I) const { return Dishes.IsValidIndex(I) ? Dishes[I].X : 0; }
	double DishZ(int32 I) const { return Dishes.IsValidIndex(I) ? Dishes[I].Z : 0; }
	double DishScale(int32 I) const { return Dishes.IsValidIndex(I) ? Dishes[I].Scale : 1; }
	// one dish (Dish.snap: story/arrayBack.ts and story/dishesFollow.ts turn them one by one); drawn
	// with the next frame's
	void SnapDish(int32 I, double Az, double El);

	// ---------- doors (Doors.ts) ----------
	bool IsDoor(const FString& ItemId) const;
	FString DoorLabel(const FString& ItemId) const;
	bool ToggleDoor(const FString& ItemId);
	bool RestoreDoorwayAt(const FVector& PlayerThree, double Radius);
	// Doors.set(id, open, instant): by the door's id ('south') or its thing's id ('doorSouth');
	// instant: no swing and no sound (restoring a save)
	void SetDoorOpen(const FString& DoorOrItemId, bool bOpen, bool bInstant = true);
	bool IsDoorOpen(const FString& ItemId) const;
	void CloseAllDoors();
	TFunction<void(const FString&, bool, const FVector&, bool)> OnDoor; // id, open, where (UE), heavy
	// Doors.states / restore / set by the door's own id ('east', 'south', 'lab', 'exit', 'records'),
	// and Doors.onChange: whenever a door changes, however it was moved
	TMap<FString, bool> DoorStates() const;
	void SetDoorById(const FString& Id, bool bOpen, bool bInstant);
	TFunction<void(const FString&, bool)> OnDoorChanged;

	// ---------- the sky, lights and floods ----------
	void SetPlayerEye(const FVector& UE) { Eye = UE; }
	FVector SpawnThree() const { return Spawn; } // x, yaw, z
	double ToneMappingExposure = 1.05;
	float ExposureOffset = 0.f; // Unreal against three.js, set by -S47Exposure= for calibration
	float SceneScale = 1.f;     // and -S47Scene=, a factor on the light going into the tone curve
	FLinearColor FogColor = FLinearColor(0.0155f, 0.0212f, 0.0353f);
	float FogDensity = 0.0021f;

private:
	friend struct FS47Obj;
	friend struct FS47Light;
	friend struct FS47Mat;
	friend class FS47CinemaPosters;
	static FQuat S47ThreeEuler(double X, double Y, double Z);
	bool bLoaded = false;
	void BuildRetroProps();
	void BuildFieldModels();
	void UpdateFieldModels();
	TArray<TWeakObjectPtr<class UStaticMeshComponent>> FieldModelComponents, FieldModelRetired;
	struct FFieldModelBinding { TWeakObjectPtr<class UMaterialInstanceDynamic> Mid; int32 Source; bool Shared; float Emission; };
	TArray<FFieldModelBinding> FieldModelBindings;
	void BuildRadioStation();
	void BuildCalicheStreet();
	void BuildMesaDiner();
	bool bMesaDinerReady = false;
	void BuildCalicheNature();
	void BuildHighwayPower();
	void BuildHighwayRanch();
	void BuildHighwayLandscape();
	void BeginHighwayAudit();
	void BeginSaroArrivalViews();
	TSharedPtr<FS47SaroArrival> SaroArrival;
	TSharedPtr<FS47SaroSite> SaroSite;
	TSharedPtr<FS47FleetParked> FleetParked;
	TSharedPtr<FS47TownBuildings> TownBuildings;
	TSharedPtr<FS47CalicheHomes> CalicheHomes;
	TSharedPtr<FS47CinemaPosters> CinemaPosters;
	void StartCinemaPostersContract();
	TSharedPtr<FS47MotelV2> MotelV2;
	TSharedPtr<FS47RouteNature> RouteNature;
	void BeginProductionViews();
	void BeginMotelV2Route();
	void BeginCalicheHomesContract();
	void StartFarmAccessContract();
	void UpdateHighwayLandscape();
	TSharedPtr<FS47HighwayRanch> HighwayRanch;
	TWeakObjectPtr<USceneComponent> HighwayLandscapeRoot;
	TMap<int32, FTransform> HighwayVisuals;
	TSet<int32> HighwaySharedNodes;
	FVector HighwayOffset = FVector::ZeroVector;
	int32 HighwayBridgeInstances = 0;
	void BuildAuthoredArray();
	void UpdateRetroProps();
	struct FRetroTv { TWeakObjectPtr<UMaterialInstanceDynamic> Screen, Pilot; };
	TArray<FRetroTv> RetroTvs;
	TArray<FS47WebNode> Nodes;
	TArray<FString> GeoPaths;
	TMap<FString, int32> Handles, HandleMats, HandleTexs;
	TMap<FString, FString> HandleLights;
	TMap<FString, FVector> HandleVecs;
	TMap<FString, FS47Area> HandleRects;
	TMap<FString, FS47Pose> HandlePoses;
	TMap<int32, TArray<TWeakObjectPtr<class UMaterialInstanceDynamic>>> NodeMidCache;
	TMap<int32, TWeakObjectPtr<class UMaterialInstanceDynamic>> SharedMidCache;
	TMap<int32, FString> Usable;
	struct FFloodSet { FString Key; TArray<FLinearColor> Pos, Col; float Scale = 0.07f; int32 W = 1; int32 Count = 0; TSet<int32> Skip; };
	TArray<FFloodSet> FloodSets;
	TMap<int32, TArray<TWeakObjectPtr<class UMaterialInstanceDynamic>>> FloodMids;
	TMap<int32, int32> MatFloodSet; // material id -> flood set
	void UploadFlood(int32 Set);
	TSet<int32> FloodDirty;
	TSharedPtr<class FJsonObject> Drive;
	TSharedPtr<class FJsonObject> Life;
	TMap<int32, TSharedPtr<FS47GlowPoints>> Glows;
	TMap<int32, TArray<float>> PointsRaw, PointsPh;
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

	// Exported lamp state feeds native lights; shared material pools never add light twice.
	UPROPERTY() TObjectPtr<class APostProcessVolume> LightingPost;
	UPROPERTY() TObjectPtr<class UExponentialHeightFogComponent> LightingFog;
	UPROPERTY() TObjectPtr<USkyLightComponent> NightSky;
	UPROPERTY() TObjectPtr<UDirectionalLightComponent> DawnSun;
	struct FPoolLight { int32 Set = -1, Slot = -1; float Reach = 0; TWeakObjectPtr<UPointLightComponent> Light; };
	TArray<FPoolLight> PoolLights;
	struct FArrayLight { int32 Dish = -1, Glow = -1; FVector At; TWeakObjectPtr<USpotLightComponent> Light; };
	TArray<FArrayLight> ArrayLights;
	void BuildArrayLighting();
	void UpdateArrayLighting();
	struct FExteriorLight
	{
		int32 Node = -1, Set = -1, Slot = -1, GlowNode = -1, Glow = -1, Material = -1;
		FVector At, Target;
		float Reference = 1, Candela = 0, GlowReference = 1, LastK = -1;
		bool bWasOn = false;
		bool bAuthoredLens = false;
		TWeakObjectPtr<USpotLightComponent> Light;
		TWeakObjectPtr<UMaterialInstanceDynamic> Diffuser;
		TWeakObjectPtr<UStaticMeshComponent> Fixture;
	};
	TArray<FExteriorLight> ExteriorLights;
	void BuildExteriorLighting();
	void UpdateExteriorLighting();
	struct FLabLight
	{
		int32 Node = -1, Role = -1, Slot = -1;
		float WidthBase = 1, Lumens = 0;
		TWeakObjectPtr<ULocalLightComponent> Light;
		TWeakObjectPtr<UMaterialInstanceDynamic> Diffuser;
	};
	TArray<FLabLight> LabLights;
	float LabScaleBase = 0.22f;
	void BuildLabLighting();
	void UpdateLabLighting();
	// Physical sources fitted to exported diffuser/screen meshes. Original room lights carry state only.
	struct FFixtureLight
	{
		int32 Node = -1, Set = -1, Slot = -1, Tube = -1, Driver = -1;
		float Reference = 1, Lumens = 0, LastK = -1;
		bool bDesk = false;
		FLinearColor Color = FLinearColor::White;
		TWeakObjectPtr<URectLightComponent> Light;
		TWeakObjectPtr<UMaterialInstanceDynamic> Diffuser;
	};
	TArray<FFixtureLight> FixtureLights;
	struct FScreenLight { int32 Node = -1, Screen = -1; float Base = 0, Reference = 1; TWeakObjectPtr<URectLightComponent> Light; };
	TArray<FScreenLight> ScreenLights;
	float CrtFlicker[4] = { 1, 1, 1, 1 };
	void BuildFixtureLighting();
	void UpdateFixtureLighting();
	void BuildScreenLighting();
	void UpdateScreenLighting();
	// Indicator state stays separate from station dimming and from the generated material slots.
	struct FIndicatorBrightness { float Authored = 1, Applied = 1, EmissiveAuthored = 1, EmissiveApplied = 1; };
	struct FIndicatorSurface
	{
		int32 Node = -1, Screen = -1;
		bool bFixed = false, bMapped = false;
		TWeakObjectPtr<UStaticMeshComponent> Mesh;
		TArray<FIndicatorBrightness> Slots;
	};
	TArray<FIndicatorSurface> IndicatorSurfaces;
	// Native glTF props are adopted after BuildRoom; only their authored emitting slots are gated.
	struct FNativeIndicatorSurface
	{
		int32 Node = -1, Slot = -1;
		FLinearColor Emissive = FLinearColor::Black;
		TWeakObjectPtr<UStaticMeshComponent> Mesh;
	};
	TArray<FNativeIndicatorSurface> NativeIndicatorSurfaces;
	int32 NativeIndicatorNodeCount = -1;
	TArray<bool> RackLedOn;
	void BuildIndicatorLighting();
	void DrawRackIndicator(int32 Index);
	void UpdateIndicatorLighting(float Power);
	void GateIndicatorMesh(UStaticMeshComponent* Mesh, float Power);
	float SaroLightingPower() const;
	void ReviewIndicators(); // -S47IndicatorTest=<directory>, one short camera run at a time
	URectLightComponent* SurfaceLight(int32 Node, const FVector& FacingThree, float Lumens, float Reach);
	TArray<TArray<int32>> PoolRoots;
	bool bSkyCaptured = false;
	float PureSkyCaptureWeight = 0;
	bool bLightingProfiled = false;
	float CapturedDawn = 0;
	float CapturedCaliche = 0;
	double SkyCaptureTime = 0;
	TArray<float> PerfFrames, PerfGame, PerfDraw, PerfGpu;
	TSet<uint64> SuppressedPools;
	int32 LightingQuality = 1;
	FS47GraphicsSettings Graphics;
	float SkyDawn = 0, SkySun = 0, SkyFlash = 0, SkyEvening = 0, SkyGlow = 0;
	// SKY.md: volumetric clouds, their cloud-only light (set sun from below / moon) and drift.
	UPROPERTY() TObjectPtr<class UVolumetricCloudComponent> Clouds;
	UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> CloudMid;
	UPROPERTY() TObjectPtr<UDirectionalLightComponent> CloudLight;
	float CloudCover = 0, CloudCirrus = 0;
	double CloudTime = -1;
	FVector CloudWind = FVector::ZeroVector;
	void BuildClouds();
	void ApplyCloudQuality();
	void UpdateCloudLight(const FVector& SunDirection, float Daylight);
	void BuildNativeLighting();
	void UpdateNativeLighting();
	void UpdateNativeFog();
	void SampleLightingPerformance(float Dt);
	void ReviewLighting(); // -S47LightingReview=<directory>: fixed cameras, no gameplay input
	int32 LightingReviewStep = -1, LightingReviewShot = -1;
	bool bLightingReviewPower = false;
	double LightingRunTime = 0, LightingLogTime = 0;
	int32 AtmosphereReviewShot = 0; // opt-in two-frame wind evidence only
	// lights, per area
	struct FLightRef { TWeakObjectPtr<ULightComponent> L; FString Area; float Base = 0; FString Uuid; };
	TArray<FLightRef> Lights;
	UPROPERTY() TObjectPtr<class UMaterialParameterCollection> Env;
	struct FHemi { FLinearColor Sky, Ground; float Intensity; };
	TMap<FString, FHemi> Hemis;
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
	TArray<bool> TubePowered;
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
	bool bDishesDirty = false;
	float HemiK = 1.f;
	void ApplyHemi();

	// doors
	struct FDoor { FString Id, Inter, Name, Area, Sound; double Range = 2.4; bool bOpen = false; double K = 0, From = 0, T0 = -1; TArray<int32> MovedNodes; TArray<TArray<double>> Shut, Open; TArray<int32> Zones; TOptional<FS47Rect> Leaf; int32 Proxy = -1; TWeakObjectPtr<class UBoxComponent> Doorway; };
	TArray<FDoor> Doors;
	double DoorT = 0;
	void ApplyDoor(FDoor& D, double K);
	void SetDoor(FDoor& D, bool bOpen, bool bInstant);
	void DoorwayPick(FDoor& D);
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

	// ---------- Chapter6.ts / main.ts: the pull-out's air and sky overrides ----------
public:
	void SetChapter6Fog(TOptional<double> Density); // null: the area's own FogExp2 density
	void SetDawnLift(TOptional<double> K);
	void SetSunElevation(TOptional<double> Degrees);
	TOptional<double> DawnLift() const { return Chapter6DawnLift; }
	TOptional<double> SunElevation() const { return Chapter6SunElevation; }
	// M_WebSky currently contains an exported panorama only. The world-life sky renderer
	// binds these hooks (or reads the getters); its dawn/sun material is a separate dependency.
	TFunction<void(TOptional<double>)> OnDawnLift;
	TFunction<void(TOptional<double>)> OnSunElevation;

private:
	TOptional<double> Chapter6DawnLift, Chapter6SunElevation;
};

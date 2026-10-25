// world/Station01.ts (the behaviour; the geometry comes from the web version itself)
#include "S47Station.h"

#include "Components/MaterialBillboardComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Json.h"
#include "S47Main.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47Yard.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Station, Log, All);

const US47Station::FSlots US47Station::SLOT;
const US47Station::FBase US47Station::BASE;

namespace
{
	const TCHAR* SITE = TEXT("world.site.");
	// Station01's places the glow points are added at, in the station's own terms (glow.add):
	// WALL_LAMP (-9.95, 2.42, 5.0); the windows of the hut (HUT x1 -9, z1 4.5, t 0.2; WIN_S x -12.3
	// to -11.3, WIN_E z 2.3 to 3.3); the receiver rx (-11.85, 0.8, 1.8); the desk lamp dl (-10.35,
	// 0.8, 1.75); the shed's x1 -15.1 and G -0.15; the yard light (POLE.x - 1.05, G + 6.1, POLE.z - 0.3)
	const FVector GLOW_YARD(-7.65, 5.95 + 0.05, 6.7);
	const FVector GLOW_WIN_S((-12.3 + -11.3) / 2, 1.4, 4.5 + 0.2 + 0.08);
	const FVector GLOW_WIN_E(-9 + 0.2 + 0.08, 1.4, (2.3 + 3.3) / 2);
	const FVector GLOW_WALL(-9.95, 2.4, 5.0);
	const FVector GLOW_DIAL(-11.85 + 0.06, 0.8 + 0.2, 1.8 + 0.2);
	const FVector GLOW_DESK(-10.35 - 0.25, 0.8 + 0.29, 1.75 + 0.14);
	const FVector GLOW_CEIL(-11.0, 2.14, 3.0);
	const FVector GLOW_SHED(-15.1 + 0.12, -0.15 + 1.9, -0.4);
	// the field lamp's: its head at LAMP (1.6, 1.86, -2.6) looks at the transit; the flood is 0.3 m
	// out along it, the glow 0.11 m (lamp())
	const FVector LAMP_HEAD(1.6, 1.86, -2.6);
}

// ---------- construction ----------

void US47Station::Init(US47Main* InM)
{
	M = InM;
	if (!M || !M->Web) return;
	AS47Web* W = M->Web;
	bBuilt = W->Obj(TEXT("world.site.group")).IsValid();
	Occluders.Reset();
	for (const FString& P : W->PathsUnder(TEXT("world.site.occluders["))) Occluders.Add(W->Obj(P));
	FieldFlood = W->FloodSet(TEXT("field"));
	HutFlood = W->FloodSet(TEXT("hut"));
	// the slots are the constructor's order; the export's pools say whether that still holds
	for (const TSharedPtr<FJsonValue>& V : US47Yard::ManifestSection(TEXT("floods")))
	{
		const S47J::FObj F = V->AsObject();
		const FString Key = S47J::Str(F, TEXT("key"));
		const TArray<TSharedPtr<FJsonValue>> Pools = S47J::Arr(F, TEXT("pools"));
		auto R = [&Pools](int32 I) { return Pools.IsValidIndex(I) ? S47J::Num(Pools[I]->AsObject(), TEXT("r")) : -1.0; };
		if (Key == TEXT("field") && (R(SLOT.Yard) != BASE.Yard || R(SLOT.Wall) != BASE.Wall || R(SLOT.Spill) != BASE.Spill || R(SLOT.Shed) != BASE.Shed))
		{
			UE_LOG(LogS47Station, Warning, TEXT("S47: STATION 01's field floods are not in the constructor's order in this export"));
		}
		if (Key == TEXT("hut") && (R(SLOT.Desk) != BASE.Desk || R(SLOT.Ceil) != BASE.Ceil || R(SLOT.Door) != BASE.Door))
		{
			UE_LOG(LogS47Station, Warning, TEXT("S47: STATION 01's hut floods are not in the constructor's order in this export"));
		}
	}
	// the export holds the station as the web version builds it: lamp uncovered, receiver on, the
	// hut lit, the lamps outside on, the drawer in, the walk gap chained
	bLampCovered = false; bReceiverOn = true; bHutLightOn = true; bOutLampsOn = true; bDrawerOpen = false;
	DrawerZ = Obj(TEXT("drawer")).Position().Z;
	FindGlows();
	UE_LOG(LogS47Station, Display, TEXT("S47: STATION 01 %s, %d occluders, floods field %d hut %d"), IsBuilt() ? TEXT("built") : TEXT("not in the export"), Occluders.Num(), FieldFlood, HutFlood);
}

bool US47Station::IsBuilt() const { return bBuilt; }

FS47Obj US47Station::Proxy(const FString& Key) const { return M && M->Web ? M->Web->Obj(SITE + FString(TEXT("proxies.")) + Key) : FS47Obj(); }
FS47Obj US47Station::Obj(const FString& Key) const { return M && M->Web ? M->Web->Obj(SITE + FString(TEXT("objs.")) + Key) : FS47Obj(); }

FS47Pose US47Station::PoseAt(const FString& Path) const { return M && M->Web ? M->Web->Pose(SITE + Path) : FS47Pose(); }
FS47Pose US47Station::Arrive() const { return PoseAt(TEXT("anchors.arrive")); }
FVector US47Station::MarkerTarget() const { return M && M->Web ? M->Web->Vec(TEXT("world.site.anchors.markerTarget")) : FVector::ZeroVector; }
FVector US47Station::CableTarget() const { return M && M->Web ? M->Web->Vec(TEXT("world.site.anchors.cableTarget")) : FVector::ZeroVector; }
FS47Pose US47Station::PhotoSpot(const FString& Which) const { return PoseAt(TEXT("anchors.photoSpots.") + Which); }
FVector US47Station::Point(const FString& Key) const { return M && M->Web ? M->Web->Vec(TEXT("world.site.anchors.points.") + Key) : FVector::ZeroVector; }
FS47Area US47Station::HutBounds() const { return M && M->Web ? M->Web->Rect(TEXT("world.site.hutBounds")) : FS47Area(); }

void US47Station::FindGlows()
{
	// Station01's gx indices are not handles the export keeps: each glow point is the billboard at
	// the place glow.add put it, under world.site.glow.points
	Gx = FGlows();
	const FS47Obj Pts = M && M->Web ? M->Web->Obj(TEXT("world.site.glow.points")) : FS47Obj();
	const FS47WebNode* N = Pts ? Pts.W->Node(Pts.I) : nullptr;
	if (!N || !N->Comp.IsValid()) return;
	// lp = head + fwd * 0.3 is the field flood's pool (in the manifest); gp = head + fwd * 0.11
	FVector FieldGlow = LAMP_HEAD + FVector(0.2054, -0.0748, -0.2054) * (0.11 / 0.3);
	for (const TSharedPtr<FJsonValue>& V : US47Yard::ManifestSection(TEXT("floods")))
	{
		const S47J::FObj F = V->AsObject();
		const TArray<TSharedPtr<FJsonValue>> Pools = S47J::Arr(F, TEXT("pools"));
		if (S47J::Str(F, TEXT("key")) != TEXT("field") || !Pools.IsValidIndex(SLOT.Field)) continue;
		const S47J::FObj P = Pools[SLOT.Field]->AsObject();
		const FVector O = M->Web->Vec(TEXT("world.site.o"), FVector(0, 0, 8000));
		const FVector Lp = FVector(S47J::Num(P, TEXT("x")), S47J::Num(P, TEXT("y")), S47J::Num(P, TEXT("z"))) - O;
		FieldGlow = LAMP_HEAD + (Lp - LAMP_HEAD) * (0.11 / 0.3);
	}
	const TPair<TWeakObjectPtr<UMaterialBillboardComponent>*, FVector> Want[] = {
		{ &Gx.Yard, GLOW_YARD }, { &Gx.WinS, GLOW_WIN_S }, { &Gx.WinE, GLOW_WIN_E }, { &Gx.Wall, GLOW_WALL }, { &Gx.Dial, GLOW_DIAL },
		{ &Gx.Desk, GLOW_DESK }, { &Gx.Ceil, GLOW_CEIL }, { &Gx.Shed, GLOW_SHED }, { &Gx.Field, FieldGlow } };
	int32 Found = 0;
	for (USceneComponent* C : N->Comp->GetAttachChildren())
	{
		UMaterialBillboardComponent* B = Cast<UMaterialBillboardComponent>(C);
		if (!B) continue;
		for (const auto& Wn : Want)
		{
			if (!Wn.Key->IsValid() && FVector::Dist(B->GetRelativeLocation(), S47Three::ToUE(Wn.Value)) < 1.5) { *Wn.Key = B; ++Found; }
		}
	}
	if (Found < int32(UE_ARRAY_COUNT(Want))) UE_LOG(LogS47Station, Warning, TEXT("S47: STATION 01's glow points: %d of %d found"), Found, int32(UE_ARRAY_COUNT(Want)));
}

// ---------- small helpers ----------

void US47Station::SetGlow(const TWeakObjectPtr<UMaterialBillboardComponent>& G, uint32 Color, double K) const
{
	// setGlow(i, color, k): the point's colour, new THREE.Color(color).multiplyScalar(k)
	UMaterialBillboardComponent* B = G.Get();
	if (!B || B->Elements.Num() == 0) return;
	const FLinearColor C = S47Three::Hex(Color) * float(K);
	if (UMaterialInstanceDynamic* D = Cast<UMaterialInstanceDynamic>(B->Elements[0].Material)) D->SetVectorParameterValue(TEXT("Color"), FLinearColor(C.R, C.G, C.B, 1.f));
}

void US47Station::SetFloodIn(int32 Set, int32 Slot, double Intensity) const
{
	// setFlood(slot, intensity, set)
	if (M && M->Web && Set >= 0 && Slot >= 0) M->Web->SetFlood(Set, Slot, float(Intensity));
}

void US47Station::SetMatColor(const TCHAR* Name, uint32 Color) const
{
	// this.m.<name>.color.set(color): the material shared by every mesh that has it
	if (M && M->Web) M->Web->Material(SITE + FString(TEXT("m.")) + Name).SetColor(S47Three::Hex(Color));
}

// ---------- switches ----------

void US47Station::SetLampCovered(bool bOn)
{
	bLampCovered = bOn;
	Obj(TEXT("lampHood")).SetRotX(bOn ? 0 : HOOD_OPEN);
	SetFloodIn(FieldFlood, SLOT.Field, bOn ? 0 : BASE.Field);
	SetMatColor(TEXT("fieldBulb"), bOn ? 0x151311 : 0xfff2d8);
	SetGlow(Gx.Field, 0xffd29a, bOn ? 0 : 1);
}

void US47Station::SetFlash(const TOptional<FVector>& P, double W)
{
	if (!M || !M->Web || FieldFlood < 0) return;
	if (P) M->Web->SetFloodPool(FieldFlood, 0, *P, float(W), S47Three::Hex(0xfff6ec), true);
	else M->Web->SetFlood(FieldFlood, 0, 0.f); // fieldFlood.pos[0].w = 0
	// Expose captures before the next world tick, then clears the flash in the same call.
	// Push both states to native lights now; a deferred pool update never lights the film.
	M->Web->SyncFloodLights();
}

void US47Station::SetReceiver(bool bOn)
{
	bReceiverOn = bOn;
	SetMatColor(TEXT("dial"), bOn ? 0xffffff : 0x141210);
	SetFloodIn(HutFlood, SLOT.Dial, bOn ? BASE.Dial : 0);
	SetGlow(Gx.Dial, 0xffb050, bOn ? 1 : 0);
}

void US47Station::SetHutLight(bool bOn)
{
	bHutLightOn = bOn;
	SetMatColor(TEXT("hutBulb"), bOn ? 0xfff0d0 : 0x22201c);
	SetFloodIn(HutFlood, SLOT.Ceil, bOn ? BASE.Ceil : 0);
	SetFloodIn(HutFlood, SLOT.Desk, bOn ? BASE.Desk : 0);
	SetFloodIn(FieldFlood, SLOT.Spill, bOn ? BASE.Spill : 0);
	SetMatColor(TEXT("paneOut"), bOn ? 0xffb56b : 0x0b0e14);
	if (M && M->Web) M->Web->Material(TEXT("world.site.m.paneOut")).SetOpacity(bOn ? 0.5f : 0.7f);
	SetGlow(Gx.Ceil, 0xffe0b0, bOn ? 1 : 0);
	SetGlow(Gx.Desk, 0xffd59a, bOn ? 1 : 0);
	SetGlow(Gx.WinS, 0x6a4018, bOn ? 1 : 0);
	SetGlow(Gx.WinE, 0x6a4018, bOn ? 1 : 0);
}

void US47Station::SetHutDoor(double K)
{
	const FS47Obj Leaf = Obj(TEXT("hutDoorLeaf"));
	if (!Leaf) return;
	const double E = FMath::Clamp(K, 0.0, 1.0);
	// (it swings faster as it goes, the way a door let go of does, and stops in the frame)
	FS47Obj(Leaf).SetRotY(-1.75 * (1 - E * E));
	const bool bShut = E > 0.8;
	if (bShut == bDoorCol || !M || !M->Web) return;
	TArray<FS47Rect>& Cols = M->Web->CollidersMutable(TEXT("station01"));
	const FVector O = M->Web->Vec(TEXT("world.site.o"), FVector(0, 0, 8000));
	if (bShut)
	{
		// HUT z1 4.5, t 0.2; DOOR x -10.5 to -9.4
		FS47Rect R;
		R.MinX = -10.5 + O.X; R.MaxX = -9.4 + O.X; R.MinZ = 4.5 - 0.06 + O.Z; R.MaxZ = 4.5 + 0.2 + 0.08 + O.Z;
		R.Id = TEXT("s1hutDoorShut");
		Cols.Add(R);
	}
	else Cols.RemoveAll([](const FS47Rect& R) { return R.Id == TEXT("s1hutDoorShut"); });
	bDoorCol = bShut;
}

void US47Station::SetOutLamps(bool bOn)
{
	bOutLampsOn = bOn;
	SetFloodIn(FieldFlood, SLOT.Yard, bOn ? BASE.Yard : 0);
	SetFloodIn(FieldFlood, SLOT.Shed, bOn ? BASE.Shed : 0);
	SetFloodIn(FieldFlood, SLOT.Wall, bOn ? BASE.Wall * WallK : 0);
	SetFloodIn(HutFlood, SLOT.Door, bOn ? BASE.Door * WallK : 0);
	const bool bField = bOn && !bLampCovered;
	SetFloodIn(FieldFlood, SLOT.Field, bField ? BASE.Field : 0);
	SetMatColor(TEXT("wallBulb"), bOn ? 0xfff0d6 : 0x1c1a17);
	SetMatColor(TEXT("lampBulb"), bOn ? 0xffd9a0 : 0x1c1a17);
	SetMatColor(TEXT("lens"), bOn ? 0xffd28a : 0x15140f);
	SetMatColor(TEXT("fieldBulb"), bField ? 0xfff2d8 : 0x151311);
	SetGlow(Gx.Yard, 0xffc07a, bOn ? 1 : 0);
	SetGlow(Gx.Shed, 0xffc888, bOn ? 1 : 0);
	SetGlow(Gx.Wall, 0xffc27a, bOn ? WallK : 0);
	SetGlow(Gx.Field, 0xffd29a, bField ? 1 : 0);
}

void US47Station::SetWalkGate(bool bOpen)
{
	if (WalkOpen && *WalkOpen == bOpen) return;
	WalkOpen = bOpen;
	if (!M || !M->Web) return;
	if (const FS47Obj Chain = M->Web->Obj(TEXT("world.site.walkChain"))) FS47Obj(Chain).SetVisible(!bOpen);
	// open, its hit box goes out of the way of the crosshair (layer 31 in three, as hidden things
	// do): here its mesh stops answering the line of sight
	Proxy(TEXT("walkGate")).Traverse([bOpen](FS47Obj O)
	{
		const FS47WebNode* N = O.W->Node(O.I);
		if (UPrimitiveComponent* P = N ? Cast<UPrimitiveComponent>(N->Comp.Get()) : nullptr) P->SetCollisionEnabled(bOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
	});
	for (FS47Rect& Z : M->Web->ZonesMutable(TEXT("station01"))) if (Z.Id == TEXT("walkThrough")) Z.bEnabled = bOpen;
}

void US47Station::SetDrawer(bool bOpen, bool bNow)
{
	bDrawerOpen = bOpen;
	if (bNow) DrawerK = bOpen ? 1 : 0;
	PlaceDrawer();
}

void US47Station::PlaceDrawer()
{
	FS47Obj Dw = Obj(TEXT("drawer"));
	if (!Dw) return;
	const double K = DrawerK, E = K * K * (3 - 2 * K);
	Dw.SetZ(DrawerZ + 0.27 * E);
	if (FS47Obj Inside = Obj(TEXT("drawerInside"))) Inside.SetVisible(K > 0.05);
}

// ---------- per frame ----------

void US47Station::Update(double Dt, double Tt)
{
	// (glow.update(t) and the fx's time: the glow points and the dust are the export's own)
	// the drawer slides (0.45 s), a little stiff at the start
	const double Want = bDrawerOpen ? 1 : 0;
	if (DrawerK != Want)
	{
		DrawerK = Want > DrawerK ? FMath::Min(1.0, DrawerK + Dt / 0.45) : FMath::Max(0.0, DrawerK - Dt / 0.35);
		PlaceDrawer();
	}
	// the bulb over the door is loose in its socket: now and then it dips
	const bool bDip = FMath::Sin(Tt * 0.83) > 0.93 && FMath::Sin(Tt * 31 + FMath::Sin(Tt * 5)) > -0.2;
	const double K = bDip ? 0.3 + 0.25 * FMath::Abs(FMath::Sin(Tt * 47)) : 0.95 + 0.05 * FMath::Sin(Tt * 13.7) * FMath::Sin(Tt * 4.1);
	if (bOutLampsOn && FMath::Abs(K - WallK) > 0.01)
	{
		WallK = K;
		SetFloodIn(FieldFlood, SLOT.Wall, BASE.Wall * K);
		SetFloodIn(HutFlood, SLOT.Door, BASE.Door * K);
		// wallBulb.color.setRGB(k, k * 0.94, k * 0.84): linear values
		if (M && M->Web) M->Web->Material(TEXT("world.site.m.wallBulb")).SetColor(FLinearColor(float(K), float(K * 0.94), float(K * 0.84), 1.f));
		SetGlow(Gx.Wall, 0xffc27a, K);
	}
}

void US47Station::Tick(float DeltaTime)
{
	// main.ts step(): nothing while the pause menu holds the frame or off the play
	if (!M || M->Mode != US47Main::EMode::Play || M->bPausedByMenu) return;
	T += DeltaTime;
	// world.setStoryPlaces: the walk gap open from chapter three on
	if (StationOpen) SetWalkGate(StationOpen());
	// World.update: if (this.area === 'station01') this.site?.update(dt, t)
	if (M->Web && M->Web->Area() == TEXT("station01")) Update(DeltaTime, T);
}

ETickableTickType US47Station::GetTickableTickType() const
{
	return HasAnyFlags(RF_ClassDefaultObject) ? ETickableTickType::Never : ETickableTickType::Conditional;
}

bool US47Station::IsTickable() const
{
	return !HasAnyFlags(RF_ClassDefaultObject) && M && M->Web && IsBuilt();
}

UWorld* US47Station::GetTickableGameObjectWorld() const
{
	return M && M->Web ? M->Web->GetWorld() : nullptr;
}

#include "S47GasBayDoor.h"
#include "S47Audio.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Props.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47GasBay, Log, All);

// GAS-BAY-DOOR-20261113. See S47GasBayDoor.h.
namespace
{
	using namespace S47GasBayDoor;
	const TCHAR* const UseId = TEXT("town:gasBay");
	const TCHAR* const DoorRect = TEXT("door:gasBay");
	const TCHAR* const NorthRect = TEXT("prop:gasStation:bay-north");
	const TCHAR* const Walk = TEXT("road");
	const TCHAR* const FloorTag = TEXT("gasbay");
	// The bay's east wall in town.json's blocks (the station's metres), and the old export's stoop before the door (Town.ts stoop:
	// town x -17.4..-16.5, z 2081.2..2084.8, walk only; three's world x is the town's + 8000).
	constexpr double WallX0 = -0.964, WallX1 = -0.694, WallZ0 = 13.093, WallZ1 = 23.093;
	constexpr double StoopX0 = 7982.6, StoopX1 = 7983.5, StoopZ0 = 2081.2, StoopZ1 = 2084.8;
	// The dock in front (gas_station.py: BFX to -16.42, z 2080.95..2085.05, its top at F2 + .012), from the bay floor's edge.
	constexpr double DockX0 = -0.744, DockX1 = 0.236, DockZ0 = 16.0425, DockZ1 = 20.1425, DockY = 2.22;
	constexpr double LeafX = -0.689, LeafT = .032, MidZ = 18.0925, Width = 3.604;

	struct FState
	{
		TWeakObjectPtr<US47Main> Main;
		TWeakObjectPtr<UStaticMeshComponent> Station, Leaf, Bar, Handle, Strap;
		bool bBuilt = false, bFailed = false, bWant = false, bMoving = false;
		double K = 0, Last = -1;
		int32 Applied = -1;   // the lock as last written: -1 not yet
		FS47Rect Door;
		int32 Tries = 0;
	};
	FState S;

	bool Near(double A, double B) { return FMath::Abs(A - B) < .06; }
	FVector2D WalkOf(const UStaticMeshComponent* C, double X, double Z)
	{
		const FVector U = C->GetComponentTransform().TransformPosition(S47Three::ToUE(X, 0.0, Z));
		return FVector2D(U.X / 100.0, U.Y / 100.0);
	}
	FS47Rect WalkBox(const UStaticMeshComponent* C, double X0, double X1, double Z0, double Z1)
	{
		FBox2D B(ForceInit);
		for (const double X : { X0, X1 }) for (const double Z : { Z0, Z1 }) B += WalkOf(C, X, Z);
		FS47Rect R; R.MinX = B.Min.X; R.MaxX = B.Max.X; R.MinZ = B.Min.Y; R.MaxZ = B.Max.Y;
		return R;
	}
	UStaticMeshComponent* Part(UStaticMeshComponent* Parent, const TCHAR* Name, UStaticMesh* Cube, UMaterialInterface* Mat)
	{
		AActor* Owner = Parent->GetOwner();
		auto* C = NewObject<UStaticMeshComponent>(Owner, MakeUniqueObjectName(Owner, UStaticMeshComponent::StaticClass(), FName(Name)));
		C->SetMobility(EComponentMobility::Movable);
		C->SetupAttachment(Parent);
		C->SetStaticMesh(Cube);
		if (Mat) C->SetMaterial(0, Mat);
		C->SetReceivesDecals(false);
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		C->SetCollisionResponseToAllChannels(ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		C->RegisterComponent();
		return C;
	}
	// A box in the station's metres: its middle and size (x across the wall, y up, z along it).
	void Put(UStaticMeshComponent* C, double X, double Y, double Z, double SX, double SY, double SZ)
	{
		if (!C) return;
		C->SetRelativeLocation(S47Three::ToUE(X, Y, Z));
		C->SetRelativeScale3D(FVector(FMath::Max(SX, .001), FMath::Max(SZ, .001), FMath::Max(SY, .001)));
	}
	void Pose()
	{
		const double H = LeafHeight(S.K), Low = Top - H;
		Put(S.Leaf.Get(), LeafX, Top - H * .5, MidZ, LeafT, H, Width);
		// (a hair bigger than the baked bar and handle: shut, they cover them as they were)
		Put(S.Bar.Get(), -0.654, Low + .035, MidZ, .044, .074, Width + .002);
		Put(S.Handle.Get(), -0.621, Low + .295, MidZ, .028, .034, .305);
		if (UStaticMeshComponent* Hd = S.Handle.Get()) Hd->SetHiddenInGame(H < .45);
		// the pull strap hangs from the bar once it is up out of reach, down to about a hand's height
		const double Strap = FMath::Min(1.1, Low - (Bottom + 1.05));
		Put(S.Strap.Get(), -0.64, Low - Strap * .5, MidZ + .55, .012, Strap, .04);
		if (UStaticMeshComponent* St = S.Strap.Get()) St->SetHiddenInGame(Strap < .05);
	}
	void ApplyWalk(US47Main* M)
	{
		if (!M || !M->Web) return;
		bool bWantShut = Blocks(S.K);
		// (it does not come down on one standing under it: the lock waits till they are out of the way)
		if (bWantShut && M->Player)
		{
			const double R = M->Player->Radius + .05, X = M->Player->Pos.X, Z = M->Player->Pos.Z;
			if (X > S.Door.MinX - R && X < S.Door.MaxX + R && Z > S.Door.MinZ - R && Z < S.Door.MaxZ + R) bWantShut = false;
		}
		if (S.Applied == int32(bWantShut)) return;
		S.Applied = int32(bWantShut);
		for (FS47Rect& C : M->Web->CollidersMutable(Walk)) if (C.Id == DoorRect) C.bEnabled = bWantShut;
	}
	void UseDoor()
	{
		US47Main* M = S.Main.Get();
		S.bWant = !S.bWant; S.bMoving = true;
		if (M && M->Audio && S.Leaf.IsValid()) M->Audio->Play(TEXT("door_metal"), .55f, S.Leaf->GetComponentLocation(), .7f);
		UE_LOG(LogS47GasBay, Display, TEXT("S47: gas station bay door: %s by hand"), S.bWant ? TEXT("raised") : TEXT("lowered"));
	}
	bool Build(US47Main* M)
	{
		if (S.bFailed || !M || !M->Web || !M->Inter) return false;
		UStaticMeshComponent* C = AS47Props::PlacedMesh(TEXT("gasStation"));
		if (!C || !C->GetStaticMesh()) return false;
		// The opening's slot: M_RollUp, the door's two slat faces and nothing else (gas_station.py).
		int32 Slot = INDEX_NONE;
		const TArray<FName> Names = C->GetMaterialSlotNames();
		for (int32 I = 0; I < C->GetNumMaterials(); ++I)
		{
			const UMaterialInterface* Mi = C->GetMaterial(I);
			if ((Names.IsValidIndex(I) && Names[I].ToString().Contains(TEXT("RollUp"))) || (Mi && Mi->GetName().Contains(TEXT("RollUp")))) { Slot = I; break; }
		}
		UMaterialInterface* Masked = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Masked.M_Web_Lit_Masked"));
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		// The east wall's block, to be cut round the opening.
		const FS47Rect Want = WalkBox(C, WallX0, WallX1, WallZ0, WallZ1);
		TArray<FS47Rect>& Rects = M->Web->CollidersMutable(Walk);
		const int32 WallIndex = Rects.IndexOfByPredicate([&Want](const FS47Rect& R)
		{
			return R.Id.StartsWith(TEXT("prop:gasStation:")) && Near(R.MinX, Want.MinX) && Near(R.MaxX, Want.MaxX) && Near(R.MinZ, Want.MinZ) && Near(R.MaxZ, Want.MaxZ);
		});
		const FVector2D A = WalkOf(C, Face, OpenZ0), B = WalkOf(C, Face, OpenZ1);
		FS47Rect South, Door, North;
		if (Slot == INDEX_NONE || !Masked || !Cube || WallIndex == INDEX_NONE || !SplitWall(Rects[WallIndex], FMath::Min(A.Y, B.Y), FMath::Max(A.Y, B.Y), South, Door, North))
		{
			// (the model or the blocks differ from what this was written for: the door stays the baked, shut one)
			S.bFailed = ++S.Tries >= 3;
			UE_LOG(LogS47GasBay, Warning, TEXT("S47: gas station bay door not built: slot %d, masked %d, cube %d, wall %d (it stays shut, as baked)"), Slot, Masked ? 1 : 0, Cube ? 1 : 0, WallIndex);
			return false;
		}
		UMaterialInterface* Slats = C->GetMaterial(Slot);
		UMaterialInstanceDynamic* Gone = UMaterialInstanceDynamic::Create(Masked, C);
		Gone->SetScalarParameterValue(TEXT("Opacity"), 0.f);
		C->SetMaterial(Slot, Gone);
		auto Mat = [](const TCHAR* Name) { return LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Free/Town/gas_station/Materials/%s.%s"), Name, Name)); };
		S.Station = C;
		S.Leaf = Part(C, TEXT("S47GasBayLeaf"), Cube, Slats);
		S.Bar = Part(C, TEXT("S47GasBayBar"), Cube, Mat(TEXT("M_Galv")));
		S.Handle = Part(C, TEXT("S47GasBayHandle"), Cube, Mat(TEXT("M_SteelDark")));
		S.Strap = Part(C, TEXT("S47GasBayStrap"), Cube, Mat(TEXT("M_Rubber")));
		Rects[WallIndex] = South;
		Rects.Add(North);
		Door.bEnabled = Blocks(S.K);
		Rects.Add(Door);
		S.Door = Door;
		// the old stoop's box in front, walk only: the dock stands there now, a floor of its own
		for (const TCHAR* Area : { TEXT("road"), TEXT("diner") })
			for (FS47Rect& R : M->Web->CollidersMutable(Area))
				if (Near(R.MinX, StoopX0) && Near(R.MaxX, StoopX1) && Near(R.MinZ, StoopZ0) && Near(R.MaxZ, StoopZ1)) R.bEnabled = false;
		const FS47Rect Dock = WalkBox(C, DockX0, DockX1, DockZ0, DockZ1);
		const double Base = C->GetComponentLocation().Z / 100.0;
		AS47Props::RemoveFloors(FloorTag);
		AS47Props::AddFloor(Dock.MinX, Dock.MaxX, Dock.MinZ, Dock.MaxZ, Base + DockY, Base + DockY, TEXT("x"), true, FloorTag);
		// the leaf and what goes with it answer the use trace as one
		int32 Parent = AS47Props::PlacedNode(TEXT("gasStation"));
		if (Parent < 0) Parent = M->Web->Handle(TEXT("world.town.group"));
		const int32 LeafNode = M->Web->AdoptNode(S.Leaf.Get(), Parent, TEXT("gasbay:leaf"));
		for (UStaticMeshComponent* Pc : { S.Bar.Get(), S.Handle.Get(), S.Strap.Get() }) M->Web->MarkUsable(M->Web->AdoptNode(Pc, Parent, TEXT("gasbay:") + Pc->GetName()), UseId);
		FS47Interactable It;
		It.Id = UseId; It.Object = M->Web->ObjAt(LeafNode); It.Range = 2.6;
		It.Label = [] { return S.bWant ? FString(TEXT("Lower the workshop door")) : FString(TEXT("Raise the workshop door")); };
		It.Use = [] { UseDoor(); };
		It.Examine = [] { return FString(TEXT("The service bay's roll-up door, steel slats on a hood, lifted by hand. The padlock hangs open on its hasp.")); };
		M->Inter->Add(It);
		M->Web->MarkUsable(LeafNode, UseId);
		S.bBuilt = true;
		Pose();
		ApplyWalk(M);
		UE_LOG(LogS47GasBay, Display, TEXT("S47: gas station bay door built: slot %d hidden, wall cut at z %.2f..%.2f, %s"), Slot, Door.MinZ, Door.MaxZ, S.bWant ? TEXT("up") : TEXT("down"));
		return true;
	}
}

namespace S47GasBayDoor
{
	double LeafHeight(double K) { return (Top - Bottom) * (1.0 - .96 * FMath::Clamp(K, 0.0, 1.0)); }
	bool Blocks(double K) { return K < WalkAt; }
	bool SplitWall(const FS47Rect& Wall, double Z0, double Z1, FS47Rect& OutSouth, FS47Rect& OutDoor, FS47Rect& OutNorth)
	{
		if (Wall.MaxX - Wall.MinX > .6 || !(Wall.MinZ < Z0 && Z0 < Z1 && Z1 < Wall.MaxZ)) return false;
		OutSouth = Wall; OutSouth.MaxZ = Z0;
		OutDoor = Wall; OutDoor.MinZ = Z0; OutDoor.MaxZ = Z1; OutDoor.Id = DoorRect;
		OutNorth = Wall; OutNorth.MinZ = Z1; OutNorth.Id = NorthRect;
		return true;
	}
	bool IsOpen() { return S.bWant; }
	bool Built() { return S.bBuilt && S.Station.IsValid(); }
	void SetOpen(bool bOpen, bool bInstant)
	{
		S.bWant = bOpen;
		if (bInstant) { S.K = bOpen ? 1 : 0; S.bMoving = false; }
		else S.bMoving = true;
		S.Applied = -1;
		if (Built()) { Pose(); ApplyWalk(S.Main.Get()); }
	}
	void Update(US47Main* M)
	{
		if (!M || !M->Web) return;
		// a new world: a new station, shut as a new game has it until a save says otherwise
		// (a state set before the first frame, a load's, is kept)
		if (S.Main.Get() != M) { if (S.Main.IsValid() || S.bBuilt) S = FState(); S.Main = M; }
		if (S.bBuilt && !S.Station.IsValid()) { const bool bWas = S.bWant; S = FState(); S.Main = M; S.bWant = bWas; S.K = bWas ? 1 : 0; }
		if (!S.bBuilt && !Build(M)) return;
		const UWorld* World = M->Web->GetWorld();
		const double Now = World ? World->GetTimeSeconds() : 0.0;
		const double Dt = S.Last < 0 ? 0.0 : FMath::Clamp(Now - S.Last, 0.0, .1);
		S.Last = Now;
		// (called from two places a frame: the second has nothing left to do)
		const double Target = S.bWant ? 1.0 : 0.0;
		if (S.K != Target && Dt > 0)
		{
			S.K = FMath::Clamp(S.K + (S.bWant ? 1.0 : -1.0) * Dt / Travel, 0.0, 1.0);
			Pose();
			if (S.K == Target && S.bMoving)
			{
				S.bMoving = false;
				if (M->Audio && S.Leaf.IsValid()) M->Audio->Play(TEXT("door_metal"), .42f, S.Leaf->GetComponentLocation(), S.bWant ? 1.f : .85f);
			}
		}
		ApplyWalk(M);
	}
	bool SelfTest(TArray<FString>& Lines)
	{
		bool bOk = true;
		auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s gas bay door: %s"), bCond ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= bCond; };
		Check(FMath::IsNearlyEqual(LeafHeight(0), 3.1) && LeafHeight(1) > .1 && LeafHeight(1) < .15 && LeafHeight(.5) < LeafHeight(.2), TEXT("shut it fills the 3.1 m opening; up, a hand's breadth is left under the hood"));
		Check(Blocks(0) && Blocks(.5) && !Blocks(WalkAt) && !Blocks(1) && Top - LeafHeight(WalkAt) - Bottom > 2.0, TEXT("the walk is locked until it is up enough to walk under (more than 2 m clear)"));
		FS47Rect Wall; Wall.MinX = 7982.38; Wall.MaxX = 7982.65; Wall.MinZ = 2078.0; Wall.MaxZ = 2088.0; Wall.Id = TEXT("prop:gasStation:18");
		FS47Rect So, Dr, No;
		const bool bCut = SplitWall(Wall, 2081.2, 2084.8, So, Dr, No);
		Check(bCut && So.MinZ == 2078.0 && So.MaxZ == 2081.2 && Dr.MinZ == 2081.2 && Dr.MaxZ == 2084.8 && No.MinZ == 2084.8 && No.MaxZ == 2088.0 && Dr.Id == DoorRect && So.Id == Wall.Id && Dr.MinX == Wall.MinX && Dr.MaxX == Wall.MaxX,
			TEXT("the bay's east wall cut in three round the opening, the door's piece its own"));
		Check(!SplitWall(Wall, 2077.0, 2084.8, So, Dr, No) && !SplitWall(Wall, 2084.8, 2081.2, So, Dr, No), TEXT("a wall the opening does not lie across is left whole"));
		Check(FMath::IsNearlyEqual(OpenZ1 - OpenZ0, 3.6) && FMath::IsNearlyEqual(Top - Bottom, 3.1) && WallX0 < Face && Face < WallX1 + .03, TEXT("the opening as the model has it: 3.6 m wide, 3.1 m high, in the wall's line"));
		return bOk;
	}
}

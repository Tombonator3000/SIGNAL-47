#include "S47Convent.h"
#include "S47Audio.h"
#include "S47CalicheAccess.h"
#include "S47FreeformTerrain.h"
#include "S47GroundEdits.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47PropCatalogue.h"
#include "S47Props.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47TownSites.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Convent, Log, All);

// CHURCH-COLONIAL-20261113. See S47Convent.h.
namespace S47Convent
{
namespace
{
	const TCHAR* const BackUse = TEXT("access:ChurchBack");
	const TCHAR* const BackRect = TEXT("convent:door:back");
	const TCHAR* const FloorTag = TEXT("convent");
	const TCHAR* const PadId = TEXT("church");
	constexpr double Swing = .45;      // seconds, the back door by hand
	constexpr double HalfWall = .2;

	struct FLandPiece { TWeakObjectPtr<USceneComponent> C; double X = 0, Z = 0, Up = 0; };
	struct FNightLamp { TWeakObjectPtr<UPointLightComponent> L; float Lumens = 0; };
	struct FState
	{
		TWeakObjectPtr<US47Main> Main;
		TWeakObjectPtr<UStaticMeshComponent> Leaf;
		FVector Root = FVector::ZeroVector, Origin = FVector::ZeroVector;
		bool bBuilt = false, bOpen = false;
		double K = 0, Last = -1, SeatIn = 0, Age = 0;
		bool bGroundSaid = false;
		int32 Applied = -1, Up = 0;
		TArray<FLandPiece> Land;
		TArray<FNightLamp> Night;
		float LastMask = -1.f;
	};
	FState S;
	bool bWantOpen = false, bWantSet = false;   // a load's state, come before the church is built

	FVector Vec3(const TArray<TSharedPtr<FJsonValue>>& A, const FVector& Else = FVector::ZeroVector)
	{
		return A.Num() >= 3 ? FVector(A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber()) : Else;
	}
	FPlan ReadPlan()
	{
		FPlan P;
		const FString Path = FPaths::ProjectConfigDir() / TEXT("Props/church_convent.json");
		FString Text; TSharedPtr<FJsonObject> J;
		if (!FFileHelper::LoadFileToString(Text, *Path)) { P.Error = TEXT("no Config/Props/church_convent.json"); return P; }
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) || !J) { P.Error = TEXT("church_convent.json is not JSON"); return P; }
		J->TryGetStringField(TEXT("meshes"), P.MeshRoot); J->TryGetStringField(TEXT("room"), P.Room);
		const TSharedPtr<FJsonObject>* O = nullptr;
		if (J->TryGetObjectField(TEXT("doors"), O) && O && O->IsValid())
		{
			(*O)->TryGetNumberField(TEXT("width"), P.DoorWidth); (*O)->TryGetNumberField(TEXT("height"), P.DoorHeight);
			const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
			if ((*O)->TryGetArrayField(TEXT("front"), A) && A->Num() >= 2) P.Front = FVector2D((*A)[0]->AsNumber(), (*A)[1]->AsNumber());
			if ((*O)->TryGetArrayField(TEXT("back"), A) && A->Num() >= 2) P.Back = FVector2D((*A)[0]->AsNumber(), (*A)[1]->AsNumber());
		}
		if (J->TryGetObjectField(TEXT("pad"), O) && O && O->IsValid())
		{
			(*O)->TryGetNumberField(TEXT("x0"), P.PadX0); (*O)->TryGetNumberField(TEXT("x1"), P.PadX1); (*O)->TryGetNumberField(TEXT("z0"), P.PadZ0); (*O)->TryGetNumberField(TEXT("z1"), P.PadZ1);
			(*O)->TryGetNumberField(TEXT("under"), P.PadUnder); (*O)->TryGetNumberField(TEXT("bank"), P.PadBank);
		}
		const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
		if (J->TryGetArrayField(TEXT("pieces"), List)) for (const TSharedPtr<FJsonValue>& V : *List)
		{
			const TSharedPtr<FJsonObject> E = V->AsObject(); if (!E) continue;
			FPiece Q; E->TryGetStringField(TEXT("id"), Q.Id); E->TryGetStringField(TEXT("mesh"), Q.Mesh);
			const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
			if (E->TryGetArrayField(TEXT("at"), A)) Q.At = Vec3(*A);
			if (E->TryGetArrayField(TEXT("scale"), A)) Q.Scale = Vec3(*A, FVector::OneVector);
			E->TryGetNumberField(TEXT("yaw"), Q.Yaw); E->TryGetBoolField(TEXT("land"), Q.bLand); E->TryGetBoolField(TEXT("shadow"), Q.bShadow);
			P.Pieces.Add(Q);
		}
		if (J->TryGetArrayField(TEXT("boxes"), List)) for (const TSharedPtr<FJsonValue>& V : *List)
		{
			const TSharedPtr<FJsonObject> E = V->AsObject(); if (!E) continue;
			FBoxPart Q; E->TryGetStringField(TEXT("id"), Q.Id);
			const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
			if (E->TryGetArrayField(TEXT("at"), A)) Q.At = Vec3(*A);
			if (E->TryGetArrayField(TEXT("size"), A)) Q.Size = Vec3(*A, FVector::OneVector);
			FString Hex; if (E->TryGetStringField(TEXT("colour"), Hex)) Q.Colour = uint32(FCString::Strtoui64(*Hex, nullptr, 16));
			E->TryGetBoolField(TEXT("shadow"), Q.bShadow); E->TryGetBoolField(TEXT("land"), Q.bLand);
			P.Boxes.Add(Q);
		}
		if (J->TryGetArrayField(TEXT("words"), List)) for (const TSharedPtr<FJsonValue>& V : *List)
		{
			const TSharedPtr<FJsonObject> E = V->AsObject(); if (!E) continue;
			FWordsL Q; E->TryGetStringField(TEXT("text"), Q.Text);
			const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
			if (E->TryGetArrayField(TEXT("at"), A)) Q.At = Vec3(*A);
			E->TryGetNumberField(TEXT("yaw"), Q.Yaw); E->TryGetNumberField(TEXT("size"), Q.Size); E->TryGetBoolField(TEXT("land"), Q.bLand);
			P.Words.Add(Q);
		}
		const auto Rects = [&J](const TCHAR* Key, TArray<FRectL>& Out)
		{
			const TArray<TSharedPtr<FJsonValue>>* L = nullptr;
			if (J->TryGetArrayField(Key, L)) for (const TSharedPtr<FJsonValue>& V : *L)
			{
				const TArray<TSharedPtr<FJsonValue>>& A = V->AsArray();
				if (A.Num() < 4) continue;
				FRectL R; R.X0 = A[0]->AsNumber(); R.X1 = A[1]->AsNumber(); R.Z0 = A[2]->AsNumber(); R.Z1 = A[3]->AsNumber();
				if (A.Num() > 4) R.Id = A[4]->AsString();
				Out.Add(R);
			}
		};
		Rects(TEXT("walls"), P.Walls); Rects(TEXT("blocks"), P.Blocks);
		if (J->TryGetArrayField(TEXT("lights"), List)) for (const TSharedPtr<FJsonValue>& V : *List)
		{
			const TSharedPtr<FJsonObject> E = V->AsObject(); if (!E) continue;
			FLightL Q; E->TryGetStringField(TEXT("id"), Q.Id);
			const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
			if (E->TryGetArrayField(TEXT("at"), A)) Q.At = Vec3(*A);
			if (E->TryGetArrayField(TEXT("colour"), A)) { const FVector C = Vec3(*A, FVector::OneVector); Q.Colour = FLinearColor(float(C.X), float(C.Y), float(C.Z)); }
			double N = 0; if (E->TryGetNumberField(TEXT("lumens"), N)) Q.Lumens = float(N);
			E->TryGetNumberField(TEXT("reach"), Q.Reach); E->TryGetBoolField(TEXT("shadows"), Q.bShadows); E->TryGetBoolField(TEXT("night"), Q.bNight);
			P.Lights.Add(Q);
		}
		if (P.MeshRoot.IsEmpty() || P.Pieces.IsEmpty() || P.Walls.IsEmpty()) { P.Error = TEXT("church_convent.json has no meshes root, pieces or walls"); return P; }
		P.bRead = true;
		return P;
	}
	FS47Rect WalkRect(const FVector& Root, double X0, double X1, double Z0, double Z1, const FString& Id)
	{
		FS47Rect R; R.MinX = Root.X + FMath::Min(X0, X1); R.MaxX = Root.X + FMath::Max(X0, X1); R.MinZ = Root.Z + FMath::Min(Z0, Z1); R.MaxZ = Root.Z + FMath::Max(Z0, Z1); R.Id = Id;
		return R;
	}
	void Prim(UStaticMeshComponent* C, bool bShadow)
	{
		C->SetMobility(EComponentMobility::Movable);
		C->SetCastShadow(bShadow);
		// sight, the camera and the use ray stop at it; the walker is held by the plan's own rectangles
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
	double LandY(const FVector& Origin, double WorldX, double WorldZ) { return Origin.Y + S47RoadTerrain::SurfaceY(WorldX - Origin.X, WorldZ - Origin.Z); }
	void PoseLeaf()
	{
		UStaticMeshComponent* C = S.Leaf.Get(); if (!C) return;
		const FPlan& P = Plan();
		// hung on the north jamb, its outside to the east; it swings in to the west
		C->SetWorldLocation(S47Three::ToUE(S.Root + FVector(P.Back.X, 0, P.Back.Y - P.DoorWidth * .5)));
		C->SetWorldRotation(FRotator(0, 180.0 + 95.0 * FMath::Clamp(S.K, 0.0, 1.0), 0));
	}
	void ApplyWalk(US47Main* M)
	{
		if (!M || !M->Web) return;
		const bool bShut = !S.bOpen;
		if (S.Applied == int32(bShut)) return;
		S.Applied = int32(bShut);
		for (const TCHAR* Area : { TEXT("road"), TEXT("diner") }) for (FS47Rect& C : M->Web->CollidersMutable(Area)) if (C.Id == BackRect) C.bEnabled = bShut;
	}
	void UseBack()
	{
		US47Main* M = S.Main.Get(); if (!M || !M->Web) return;
		// (shut only when nobody stands in the doorway, as the town's other doors)
		if (S.bOpen && M->Player && M->World && !M->World->bDriving)
		{
			const double R = M->Player->Radius, X = M->Player->Pos.X, Z = M->Player->Pos.Z;
			for (const FS47Rect& C : M->Web->CollidersMutable(TEXT("road"))) if (C.Id == BackRect && X + R > C.MinX && X - R < C.MaxX && Z + R > C.MinZ && Z - R < C.MaxZ)
			{
				if (M->UI) M->UI->Toast(TEXT("Step clear of the doorway before closing it."), 3);
				return;
			}
		}
		S.bOpen = !S.bOpen; ApplyWalk(M);
		if (M->Audio) M->Audio->Sfx(M->Audio->Or(TEXT("door_latch"), TEXT("switch")), FS47Sound());
		M->RequestAutosave(false);
		UE_LOG(LogS47Convent, Display, TEXT("S47: church: the sacristy's back door %s by hand"), S.bOpen ? TEXT("opened") : TEXT("shut"));
	}
}
	const FPlan& Plan() { static const FPlan P = ReadPlan(); return P; }
	bool Present()
	{
		static const bool bHere = []
		{
			const FPlan& P = Plan();
			if (FParse::Param(FCommandLine::Get(), TEXT("S47NoConvent"))) { UE_LOG(LogS47Convent, Display, TEXT("S47: church: -S47NoConvent, the white board church stands")); return false; }
			if (!P.bRead) { UE_LOG(LogS47Convent, Warning, TEXT("S47: church: %s; the white board church stands"), *P.Error); return false; }
			TSet<FString> Missing;
			for (const FPiece& Q : P.Pieces) if (!S47PropCatalogue::Here(P.AssetOf(Q))) Missing.Add(Q.Mesh);
			if (Missing.Num()) { UE_LOG(LogS47Convent, Display, TEXT("S47: church: %d Cosmos meshes of the mission church are not here (Tools/cosmosconvent), the white board church stands: %s"), Missing.Num(), *FString::Join(Missing.Array(), TEXT(", "))); return false; }
			return true;
		}();
		return bHere;
	}
	bool Standing() { return S.bBuilt && S.Main.IsValid(); }
	bool BackOpen() { return Standing() ? S.bOpen : bWantOpen; }
	void SetBackOpen(bool bOpen, bool bQuiet)
	{
		bWantOpen = bOpen; bWantSet = true;
		if (!Standing()) return;
		S.bOpen = bOpen; S.Applied = -1;
		if (bQuiet) { S.K = bOpen ? 1 : 0; PoseLeaf(); }
		ApplyWalk(S.Main.Get());
	}
	FVector2D BackDoorRoad()
	{
		// (the room's root is fixed by the town's assembly: world x 8021, z 2095.5; the road's metres are the world's less 8000)
		const FPlan& P = Plan();
		return FVector2D(21.0 + P.Back.X + .3, 2095.5 + P.Back.Y);   // (the arch piece's outer face stands 0.3 m out from the wall's middle plane)
	}

	void Build(US47Main* M, const FVector& Root, USceneComponent* Parent, int32 ParentNode, UStaticMesh* DoorLeaf)
	{
		S = FState();
		if (!M || !M->Web || !Parent || !Present()) return;
		const FPlan& P = Plan();
		AS47Web* W = M->Web;
		S.Main = M; S.Root = Root;
		if (const FS47Obj G = W->Obj(TEXT("world.town.group"))) S.Origin = G.WorldPosition();
		// The ground: level with the floor under the church and its forecourt, a soft bank down to the land round it (the ground mesh
		// holds the same when Tools/freeformterrain/site_ground.py has cut it in; the walls' feet go a metre down meanwhile).
		if (P.PadX1 > P.PadX0)
		{
			S47GroundEdits::FPad Pad;
			Pad.R.X = Root.X - S.Origin.X + (P.PadX0 + P.PadX1) * .5; Pad.R.Z = Root.Z - S.Origin.Z + (P.PadZ0 + P.PadZ1) * .5;
			Pad.R.HalfX = (P.PadX1 - P.PadX0) * .5; Pad.R.HalfZ = (P.PadZ1 - P.PadZ0) * .5;
			Pad.Y = Root.Y - S.Origin.Y - P.PadUnder; Pad.Bank = P.PadBank;
			S47GroundEdits::Set(PadId, { Pad }, {});
			S47FreeformTerrain::SitesChanged(W);
		}
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		UMaterialInterface* Plain = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
		TMap<FString, UStaticMesh*> Meshes;
		int32 Tris = 0;
		for (const FPiece& Q : P.Pieces)
		{
			UStaticMesh*& Mesh = Meshes.FindOrAdd(Q.Mesh);
			if (!Mesh) Mesh = S47PropCatalogue::LoadIfHere(P.AssetOf(Q));
			if (!Mesh) continue;
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, MakeUniqueObjectName(W, UStaticMeshComponent::StaticClass(), FName(*(TEXT("S47Convent_") + Q.Id))));
			C->SetupAttachment(Parent); C->SetStaticMesh(Mesh); Prim(C, Q.bShadow);
			C->RegisterComponent();
			const double WX = Root.X + Q.At.X, WZ = Root.Z + Q.At.Z;
			const double Y = Q.bLand ? LandY(S.Origin, WX, WZ) + Q.At.Y : Root.Y + Q.At.Y;
			C->SetWorldLocation(S47Three::ToUE(WX, Y, WZ)); C->SetWorldRotation(FRotator(0, Q.Yaw, 0)); C->SetWorldScale3D(Q.Scale);
			W->AdoptNode(C, ParentNode, TEXT("caliche-convent:") + Q.Id);
			if (Q.bLand) S.Land.Add({ C, WX, WZ, Q.At.Y });
			Tris += Mesh->GetNumTriangles(0);
			++S.Up;
		}
		if (Cube && Plain) for (const FBoxPart& B : P.Boxes)
		{
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, MakeUniqueObjectName(W, UStaticMeshComponent::StaticClass(), FName(*(TEXT("S47ConventBox_") + B.Id))));
			C->SetupAttachment(Parent); C->SetStaticMesh(Cube); Prim(C, B.bShadow);
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Plain, C);
			Mid->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(B.Colour)); Mid->SetScalarParameterValue(TEXT("Roughness"), .95f);
			C->SetMaterial(0, Mid);
			C->RegisterComponent();
			const double BX = Root.X + B.At.X, BZ = Root.Z + B.At.Z;
			C->SetWorldLocation(S47Three::ToUE(BX, B.bLand ? LandY(S.Origin, BX, BZ) + B.At.Y : Root.Y + B.At.Y, BZ)); C->SetWorldScale3D(FVector(B.Size.X, B.Size.Z, B.Size.Y));
			W->AdoptNode(C, ParentNode, TEXT("caliche-convent:box:") + B.Id);
			if (B.bLand) S.Land.Add({ C, BX, BZ, B.At.Y });
		}
		// Words on a board (the letter board by the street), pale paint; TextRender faces its own +X.
		for (const FWordsL& Wd : P.Words)
		{
			UTextRenderComponent* T = NewObject<UTextRenderComponent>(W);
			T->SetMobility(EComponentMobility::Movable); T->SetupAttachment(Parent);
			T->SetText(FText::FromString(Wd.Text.Replace(TEXT("|"), TEXT("\n")))); T->SetWorldSize(float(Wd.Size * 100.0));
			T->SetHorizontalAlignment(EHTA_Center); T->SetVerticalAlignment(EVRTA_TextCenter);
			T->SetTextRenderColor(FColor(236, 232, 214)); T->SetCollisionEnabled(ECollisionEnabled::NoCollision); T->SetCastShadow(false);
			T->RegisterComponent();
			const double TX = Root.X + Wd.At.X, TZ = Root.Z + Wd.At.Z;
			T->SetWorldLocationAndRotation(S47Three::ToUE(TX, Wd.bLand ? LandY(S.Origin, TX, TZ) + Wd.At.Y : Root.Y + Wd.At.Y, TZ), FRotator(0, Wd.Yaw, 0));
			if (Wd.bLand) S.Land.Add({ T, TX, TZ, Wd.At.Y });
		}
		// What the walker meets: the plan's walls and furniture, and the back door's own piece.
		for (const TCHAR* Area : { TEXT("road"), TEXT("diner") })
		{
			TArray<FS47Rect>& Cs = W->CollidersMutable(Area);
			for (const FRectL& R : P.Walls) Cs.Add(WalkRect(Root, R.X0, R.X1, R.Z0, R.Z1, TEXT("convent:wall:") + R.Id));
			for (const FRectL& R : P.Blocks) Cs.Add(WalkRect(Root, R.X0, R.X1, R.Z0, R.Z1, TEXT("convent:block:") + R.Id));
			FS47Rect Door = WalkRect(Root, P.Back.X - .11, P.Back.X + .11, P.Back.Y - P.DoorWidth * .5 + .04, P.Back.Y + P.DoorWidth * .5 - .04, BackRect);
			Door.bEnabled = true;
			Cs.Add(Door);
		}
		// The sacristy's floor past the room's own footprint (CalicheAccess gives the room's), level with it.
		AS47Props::RemoveFloors(FloorTag);
		AS47Props::AddFloor(Root.X + 13.9, Root.X + P.Back.X + HalfWall, Root.Z - 2.45, Root.Z + 2.45, Root.Y, Root.Y, TEXT("x"), true, FloorTag);
		// The back door: the church's own door leaf, made to fit the arch.
		if (DoorLeaf && M->Inter)
		{
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, MakeUniqueObjectName(W, UStaticMeshComponent::StaticClass(), FName(TEXT("S47ConventBackDoor"))));
			C->SetupAttachment(Parent); C->SetStaticMesh(DoorLeaf); Prim(C, true);
			C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
			C->RegisterComponent();
			const FVector Size = DoorLeaf->GetBounds().BoxExtent * 2.0;
			C->SetWorldScale3D(FVector(1, Size.Y > 1 ? P.DoorWidth * 100.0 / Size.Y : 1.0, Size.Z > 1 ? P.DoorHeight * 100.0 / Size.Z : 1.0));
			S.Leaf = C;
			const int32 Node = W->AdoptNode(C, ParentNode, TEXT("caliche-convent:backDoor"));
			FS47Interactable It;
			It.Id = BackUse; It.Object = W->ObjAt(Node); It.Range = 2.7;
			It.Label = [] { return S.bOpen ? FString(TEXT("Close door")) : FString(TEXT("Open door")); };
			It.Use = [] { UseBack(); };
			It.Examine = [] { return FString(TEXT("The sacristy's back door, old planks on iron straps. It gives on the cemetery gate.")); };
			M->Inter->Add(It);
			W->MarkUsable(Node, BackUse);
		}
		// Light: a few warm lamps of the game's own; the inside ones under the town's switch (a blackout), the outside ones only by night.
		for (const FLightL& L : P.Lights)
		{
			UPointLightComponent* C = NewObject<UPointLightComponent>(W);
			C->SetupAttachment(Parent); C->SetMobility(EComponentMobility::Movable);
			C->SetIntensityUnits(ELightUnits::Lumens); C->SetIntensity(L.Lumens); C->SetLightColor(L.Colour);
			C->SetAttenuationRadius(float(L.Reach * 100.0)); C->SetCastShadows(L.bShadows); C->SetSourceRadius(4.f);
			C->RegisterComponent();
			C->SetWorldLocation(S47Three::ToUE(Root + L.At));
			if (L.bNight) S.Night.Add({ C, L.Lumens });
			else S47CalicheAccess::AddLamp(TEXT("Church:") + L.Id, C, L.Lumens);
		}
		S.bBuilt = true;
		S.bOpen = bWantSet ? bWantOpen : false; S.K = S.bOpen ? 1 : 0;
		PoseLeaf(); ApplyWalk(M);
		const double Before = LandY(S.Origin, Root.X - 3, Root.Z), Rear = LandY(S.Origin, Root.X + P.Back.X + 1, Root.Z), North = LandY(S.Origin, Root.X + 6, Root.Z - 5.5), South = LandY(S.Origin, Root.X + 6, Root.Z + 5.5);
		UE_LOG(LogS47Convent, Display, TEXT("S47: church: San Isidro stands as the mission church: %d pieces of %d meshes (%d triangles), %d boxes, %d walls, %d blocks, %d lamps; floor %.2f, the land before the door %+.2f, behind %+.2f, north %+.2f, south %+.2f; ground pad %s"),
			S.Up, Meshes.Num(), Tris, P.Boxes.Num(), P.Walls.Num(), P.Blocks.Num(), P.Lights.Num(), Root.Y, Before - Root.Y, Rear - Root.Y, North - Root.Y, South - Root.Y, S47GroundEdits::Baked(PadId) ? TEXT("baked") : TEXT("NOT in the ground mesh"));
	}

	void Update(US47Main* M)
	{
		if (!M || !M->Web || !S.bBuilt || S.Main.Get() != M) return;
		const UWorld* World = M->Web->GetWorld();
		const double Now = World ? World->GetTimeSeconds() : 0.0;
		const double Dt = S.Last < 0 ? 0.0 : FMath::Clamp(Now - S.Last, 0.0, .1);
		S.Last = Now;
		const double Target = S.bOpen ? 1.0 : 0.0;
		if (S.K != Target && Dt > 0) { S.K = FMath::Clamp(S.K + (S.bOpen ? 1.0 : -1.0) * Dt / Swing, 0.0, 1.0); PoseLeaf(); }
		ApplyWalk(M);
		// the pieces on the land follow it (the relief may come after the town; a second is soon enough)
		S.SeatIn -= Dt;
		if (S.SeatIn <= 0)
		{
			S.SeatIn = 1.0;
			for (const FLandPiece& L : S.Land) if (USceneComponent* C = L.C.Get())
			{
				FVector At = C->GetComponentLocation();
				const double Z = (LandY(S.Origin, L.X, L.Z) + L.Up) * 100.0;
				if (FMath::Abs(At.Z - Z) > .5) { At.Z = Z; C->SetWorldLocation(At); }
			}
		}
		// (the town is built before Freeform's relief comes on: what the ground really does is said once, a little later)
		S.Age += Dt;
		if (!S.bGroundSaid && S.Age > 4)
		{
			S.bGroundSaid = true;
			const FPlan& P = Plan();
			UE_LOG(LogS47Convent, Display, TEXT("S47: church: the ground at the mission church: pad %s; under the floor's level before the door %+.2f, behind the back door %+.2f, north %+.2f, south %+.2f m"),
				S47GroundEdits::Baked(PadId) ? TEXT("baked into the ground mesh") : TEXT("NOT in the ground mesh"), LandY(S.Origin, S.Root.X - 3, S.Root.Z) - S.Root.Y, LandY(S.Origin, S.Root.X + P.Back.X + 1, S.Root.Z) - S.Root.Y,
				LandY(S.Origin, S.Root.X + 6, S.Root.Z - 5.5) - S.Root.Y, LandY(S.Origin, S.Root.X + 6, S.Root.Z + 5.5) - S.Root.Y);
		}
		const float Mask = M->Web->ExteriorDaylightMask(M->Web->FloodSet(TEXT("town")), 1);
		if (Mask != S.LastMask)
		{
			S.LastMask = Mask;
			for (const FNightLamp& L : S.Night) if (UPointLightComponent* C = L.L.Get()) { C->SetIntensity(L.Lumens * Mask); C->SetVisibility(Mask > .01f); }
		}
	}

	bool WayOut(const FPlan& P, bool bFront, bool bBack, int32* OutPocket)
	{
		// A grid of 0.1 m over the plan and 3 m of ground round it; a cell is shut when a wall or a block comes within 0.3 m of it.
		constexpr double Cell = .1, Fat = .3;
		double X0 = 1e9, X1 = -1e9, Z0 = 1e9, Z1 = -1e9;
		for (const FRectL& R : P.Walls) { X0 = FMath::Min(X0, R.X0); X1 = FMath::Max(X1, R.X1); Z0 = FMath::Min(Z0, R.Z0); Z1 = FMath::Max(Z1, R.Z1); }
		if (X1 <= X0) return false;
		X0 -= 3; X1 += 3; Z0 -= 3; Z1 += 3;
		const int32 NX = FMath::CeilToInt((X1 - X0) / Cell), NZ = FMath::CeilToInt((Z1 - Z0) / Cell);
		TArray<uint8> Shut; Shut.SetNumZeroed(NX * NZ);
		const auto Close = [&](double AX0, double AX1, double AZ0, double AZ1)
		{
			const int32 I0 = FMath::Max(0, FMath::FloorToInt((AX0 - Fat - X0) / Cell)), I1 = FMath::Min(NX - 1, FMath::FloorToInt((AX1 + Fat - X0) / Cell));
			const int32 K0 = FMath::Max(0, FMath::FloorToInt((AZ0 - Fat - Z0) / Cell)), K1 = FMath::Min(NZ - 1, FMath::FloorToInt((AZ1 + Fat - Z0) / Cell));
			for (int32 K = K0; K <= K1; ++K) for (int32 I = I0; I <= I1; ++I) Shut[K * NX + I] = 1;
		};
		for (const FRectL& R : P.Walls) Close(R.X0, R.X1, R.Z0, R.Z1);
		for (const FRectL& R : P.Blocks) Close(R.X0, R.X1, R.Z0, R.Z1);
		if (!bFront) Close(P.Front.X - .11, P.Front.X + .11, P.Front.Y - P.DoorWidth * .5, P.Front.Y + P.DoorWidth * .5);
		if (!bBack) Close(P.Back.X - .11, P.Back.X + .11, P.Back.Y - P.DoorWidth * .5, P.Back.Y + P.DoorWidth * .5);
		TArray<uint8> Seen; Seen.SetNumZeroed(NX * NZ);
		TArray<int32> Todo;
		// the middle of the nave's aisle, a third of the way in
		const int32 Start = FMath::FloorToInt((P.Front.Y - Z0) / Cell) * NX + FMath::FloorToInt((P.Front.X + 1.2 - X0) / Cell);
		if (!Shut.IsValidIndex(Start) || Shut[Start]) return false;
		Todo.Add(Start); Seen[Start] = 1;
		bool bOut = false;
		while (Todo.Num())
		{
			const int32 At = Todo.Pop(EAllowShrinking::No), I = At % NX, K = At / NX;
			if (I == 0 || K == 0 || I == NX - 1 || K == NZ - 1) bOut = true;
			for (const int32 N : { At - 1, At + 1, At - NX, At + NX })
			{
				if (N < 0 || N >= NX * NZ || Seen[N] || Shut[N]) continue;
				if ((N == At - 1 && I == 0) || (N == At + 1 && I == NX - 1)) continue;
				Seen[N] = 1; Todo.Add(N);
			}
		}
		if (OutPocket)
		{
			// free floor inside the building's own walls (the nave and the sacristy) the flood did not come to
			*OutPocket = 0;
			for (int32 K = 0; K < NZ; ++K) for (int32 I = 0; I < NX; ++I)
			{
				const double X = X0 + (I + .5) * Cell, Z = Z0 + (K + .5) * Cell;
				const bool bInside = (X > P.Front.X && X < P.Back.X && FMath::Abs(Z - P.Front.Y) < 2.0) || (X > P.Front.X && X < P.Back.X - 3.2 && FMath::Abs(Z - P.Front.Y) < 4.2);
				if (bInside && !Shut[K * NX + I] && !Seen[K * NX + I]) ++*OutPocket;
			}
		}
		return bOut;
	}

	bool SelfTest(TArray<FString>& Lines)
	{
		bool bOk = true;
		auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s church site: %s"), bCond ? TEXT("PASS") : TEXT("FAIL"), *What)); bOk &= bCond; };
		const FPlan& P = Plan();
		TSet<FString> Meshes; int32 Here = 0, Licensed = 0;
		for (const FPiece& Q : P.Pieces) Meshes.Add(Q.Mesh);
		for (const FString& N : Meshes) { FPiece Q; Q.Mesh = N; Here += S47PropCatalogue::Here(P.AssetOf(Q)) ? 1 : 0; Licensed += P.AssetOf(Q).StartsWith(TEXT("/Game/Licensed/")) ? 1 : 0; }
		Check(P.bRead && P.Room == TEXT("Church") && Licensed == Meshes.Num() && P.Pieces.Num() > 60 && P.Lights.Num() >= 3,
			FString::Printf(TEXT("church_convent.json reads%s: %d pieces of %d meshes, all under /Game/Licensed (bought, not in Git), %d walls, %d blocks, %d lamps; %d of the meshes are here"),
				P.bRead ? TEXT("") : *(TEXT(" NOT (") + P.Error + TEXT(")")), P.Pieces.Num(), Meshes.Num(), P.Walls.Num(), P.Blocks.Num(), P.Lights.Num(), Here));
		// with the pack or without it: exactly one church
		const bool bNew = Present();
		const bool bRule = S47CalicheAccess::OldChurchStands(true) == false && S47CalicheAccess::OldChurchStands(false) == true && (Here == Meshes.Num() || !bNew);
		const int32 Old = S47CalicheAccess::OldChurchParts();
		const bool bWorld = S47CalicheAccess::HasDoor(TEXT("Church"));
		Check(bRule && (!bWorld || ((Standing() ? 1 : 0) + (Old > 0 ? 1 : 0) == 1 && Standing() == bNew)),
			FString::Printf(TEXT("with the pack the mission church and without it the white board church, exactly one: the pack is %s; %s"), bNew ? TEXT("here") : TEXT("not here"),
				bWorld ? *FString::Printf(TEXT("in this world the mission church %s, %d parts of the old one"), Standing() ? TEXT("stands") : TEXT("does not stand"), Old) : TEXT("no town in this world")));
		if (!P.bRead) return bOk;
		int32 Pocket = -1;
		const bool bBoth = WayOut(P, true, true, &Pocket), bByBack = WayOut(P, false, true), bByFront = WayOut(P, true, false), bNone = WayOut(P, false, false);
		Check(bByBack && bByFront && !bNone, TEXT("two ways out: with the front door blocked one leaves by the sacristy's back door, with the back door blocked by the front; with both shut the walls hold"));
		Check(bBoth && Pocket == 0, FString::Printf(TEXT("no collider seals the nave: from the aisle past the pews, round the altar screen and through the sacristy to the open ground; %d cells of floor cut off"), Pocket));
		// the cemetery's path starts at the back door
		bool bPath = false; double PathX = 0, PathZ = 0;
		if (const S47TownSites::FSite* C = S47TownSites::Find(TEXT("cemetery")))
		{
			double West = 1e9;
			for (const S47TownSites::FPart& Part : C->Parts) if (Part.bGround && FMath::Abs(Part.Z) < .5)
			{
				const FVector2D At = S47TownSites::ToRoad(*C, Part.X - Part.W * .5, Part.Z);
				if (At.X < West) { West = At.X; PathX = At.X; PathZ = At.Y; }
			}
			const FVector2D Door = BackDoorRoad();
			bPath = bNew ? FMath::Abs(PathX - Door.X) < .35 && FMath::Abs(PathZ - Door.Y) < P.DoorWidth * .5 : PathX < Door.X;
		}
		Check(bPath, FString::Printf(TEXT("the cemetery path starts at a door: its west end at road x %.2f, z %.1f; the sacristy's back door at x %.2f, z %.1f (%s)"), PathX, PathZ, BackDoorRoad().X, BackDoorRoad().Y,
			bNew ? TEXT("the mission church") : TEXT("the old church has no back door: the path runs round its corner")));
		return bOk;
	}
}

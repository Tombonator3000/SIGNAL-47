#include "S47StationDressing.h"

#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "S47Main.h"
#include "S47Props.h"
#include "S47PropCatalogue.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47StationDressing, Log, All);

// A named namespace, not an anonymous one: unity builds put several files in one translation unit.
namespace S47StationDressingLocal
{
	const TCHAR* StationId = TEXT("gasStation");
	const TCHAR* Walk = TEXT("road");          // the station's walk area (Config/Props/town.json gasStation "walk")
	const TCHAR* Root = TEXT("/Game/Licensed/CosmosShop/");
	FString FilePath() { return FPaths::ProjectConfigDir() / TEXT("Props") / TEXT("gas_cosmos.json"); }

	// The shop in town metres (the Blender model's interior_layout.json): its inside, floor and ceiling; the game's shop card on the
	// counter and the attendant's place behind it stay clear.
	constexpr double X0 = -26.8, X1 = -17.2, Z0 = 2066.2, Z1 = 2077.78, Floor = 5.4455, Ceiling = 8.55;
	struct FBox2 { double X0, X1, Z0, Z1; bool Has(double X, double Z) const { return X >= X0 && X <= X1 && Z >= Z0 && Z <= Z1; } };
	const FBox2 Card{ -20.72, -20.02, 2072.75, 2073.95 }, Attendant{ -18.2, -17.2, 2072.45, 2073.55 };

	double Num(const TSharedPtr<FJsonObject>& O, const TCHAR* K, double Default) { double D; return O->TryGetNumberField(K, D) ? D : Default; }
	bool Vec(const TSharedPtr<FJsonObject>& O, const TCHAR* K, FVector& Out)
	{
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		if (!O->TryGetArrayField(K, A) || A->Num() != 3) return false;
		Out = FVector((*A)[0]->AsNumber(), (*A)[1]->AsNumber(), (*A)[2]->AsNumber());
		return !Out.ContainsNaN();
	}
	bool Here(const FString& Asset) { return FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Asset)); }
}
namespace SSD = S47StationDressingLocal;

bool FS47StationDressing::ReadFile(FVector& Origin, TArray<FPiece>& Out, FString* Error)
{
	auto Fail = [&](const FString& Why) { if (Error) *Error = Why; return false; };
	FString Text;
	TSharedPtr<FJsonObject> J;
	if (!FFileHelper::LoadFileToString(Text, *SSD::FilePath())) return Fail(TEXT("no Config/Props/gas_cosmos.json"));
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) || !J.IsValid()) return Fail(TEXT("gas_cosmos.json does not read"));
	if (!SSD::Vec(J, TEXT("origin_town"), Origin)) return Fail(TEXT("gas_cosmos.json has no origin_town"));
	const TArray<TSharedPtr<FJsonValue>>* Pieces = nullptr;
	if (!J->TryGetArrayField(TEXT("pieces"), Pieces)) return Fail(TEXT("gas_cosmos.json has no pieces"));
	for (const TSharedPtr<FJsonValue>& V : *Pieces)
	{
		const TSharedPtr<FJsonObject> P = V->AsObject();
		if (!P.IsValid()) return Fail(TEXT("a piece is not an object"));
		FPiece Pc;
		P->TryGetStringField(TEXT("id"), Pc.Id);
		P->TryGetStringField(TEXT("asset"), Pc.Asset);
		// PROPS-REUSE-20261113: or a shared catalogue model by its id (Config/Props/catalogue.json)
		if (Pc.Asset.IsEmpty() && P->TryGetStringField(TEXT("model"), Pc.Model)) Pc.Asset = S47PropCatalogue::Resolve(Pc.Model);
		if (!SSD::Vec(P, TEXT("at"), Pc.Town)) return Fail(FString::Printf(TEXT("%s has no at"), *Pc.Id));
		Pc.Yaw = SSD::Num(P, TEXT("yaw"), 0);
		Pc.Pitch = SSD::Num(P, TEXT("pitch"), 0);
		Pc.Roll = SSD::Num(P, TEXT("roll"), 0);
		Pc.Scale = SSD::Num(P, TEXT("scale"), 1);
		P->TryGetBoolField(TEXT("collide"), Pc.bCollide);
		P->TryGetBoolField(TEXT("pivot"), Pc.bPivot);
		P->TryGetStringField(TEXT("anchor_of"), Pc.AnchorOf);
		P->TryGetStringField(TEXT("standin"), Pc.Standin);
		P->TryGetBoolField(TEXT("wait_for_standin"), Pc.bWaitForStandin);
		Out.Add(MoveTemp(Pc));
	}
	return true;
}

FS47StationDressing::FUse FS47StationDressing::Decide(bool bModelHere, bool bHasStandin, bool bStandinHere, bool bWait)
{
	FUse U;
	U.bModel = bModelHere && (!bWait || !bHasStandin || bStandinHere);
	U.bStandin = bHasStandin && bStandinHere && !U.bModel;
	return U;
}

void FS47StationDressing::SetVisible(bool bOn)
{
	if (bOn == bVisible) return;
	bVisible = bOn;
	for (const TWeakObjectPtr<UStaticMeshComponent>& C : Comps)
		if (UStaticMeshComponent* P = C.Get()) P->SetVisibility(bOn);
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (W)
		for (FS47Rect& R : W->CollidersMutable(SSD::Walk))
			if (R.Id.StartsWith(TEXT("cosmos:"))) R.bEnabled = bOn;
}

void FS47StationDressing::Build(UStaticMeshComponent* Station)
{
	BuiltFor = Station;
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W) return;
	for (const TWeakObjectPtr<UStaticMeshComponent>& C : Comps)
		if (UStaticMeshComponent* P = C.Get()) P->DestroyComponent();
	Comps.Reset();
	W->CollidersMutable(SSD::Walk).RemoveAll([](const FS47Rect& R) { return R.Id.StartsWith(TEXT("cosmos:")); });
	bVisible = true;

	FVector Origin;
	TArray<FPiece> Pieces;
	FString Error;
	if (!ReadFile(Origin, Pieces, &Error)) { UE_LOG(LogS47StationDressing, Warning, TEXT("S47: station dressing: %s"), *Error); return; }
	TArray<FString> Missing;
	int32 Blocks = 0;
	// load-if-present: a missing package is not asked for (no load warning), only counted
	auto Load = [](const FString& Asset) { return SSD::Here(Asset) ? LoadObject<UStaticMesh>(nullptr, *Asset) : nullptr; };
	int32 Standins = 0, Waiting = 0;
	for (const FPiece& Pc : Pieces)
	{
		// PROPS-REUSE-20261113: the old stand-in the piece replaces, as its own part of the station model (the part's pivot is the
		// station's, so it hangs at the station's own transform). It comes back only while the real model is not here; a piece that
		// waits for it is not stood up while the stand-in is still baked into the station mesh (the two would overlap).
		UStaticMesh* Real = Load(Pc.Asset);
		UStaticMesh* Old = Pc.Standin.IsEmpty() ? nullptr : Load(Pc.Standin);
		const FUse Use = Decide(Real != nullptr, !Pc.Standin.IsEmpty(), Old != nullptr, Pc.bWaitForStandin);
		if (Use.bStandin)
		{
			UStaticMeshComponent* SC = NewObject<UStaticMeshComponent>(W, *FString::Printf(TEXT("Standin_%s_%d"), *Pc.Id, Builds));
			SC->SetMobility(EComponentMobility::Movable);
			SC->SetStaticMesh(Old);
			SC->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			SC->SetCollisionResponseToAllChannels(ECR_Ignore);
			SC->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			SC->SetCastShadow(true);
			SC->RegisterComponent();
			SC->AttachToComponent(Station, FAttachmentTransformRules::KeepRelativeTransform);
			SC->SetRelativeTransform(FTransform::Identity);
			Comps.Add(SC);
			++Standins;
		}
		if (!Use.bModel)
		{
			if (Real) ++Waiting; else Missing.Add(Pc.Id);
			continue;
		}
		UStaticMesh* Mesh = Real;
		// "at" is where the bottom centre of the piece's turned and scaled bounds goes (on a counter, a floor, against a wall), so the
		// packs' corner pivots do not matter; "anchor_of" takes another piece's bounds (a register's drawer with its body); "pivot" the
		// mesh's own pivot.
		const FRotator Rot(Pc.Pitch, Pc.Yaw, Pc.Roll);
		FVector Anchor = FVector::ZeroVector;
		if (!Pc.bPivot)
		{
			const FPiece* Of = Pc.AnchorOf.IsEmpty() ? nullptr : Pieces.FindByPredicate([&](const FPiece& O) { return O.Id == Pc.AnchorOf; });
			UStaticMesh* AnchorMesh = Of ? Load(Of->Asset) : Mesh;
			if (AnchorMesh)
			{
				const FBox B = AnchorMesh->GetBoundingBox().TransformBy(FTransform(Rot, FVector::ZeroVector, FVector(Pc.Scale)));
				Anchor = FVector(B.GetCenter().X, B.GetCenter().Y, B.Min.Z);
			}
		}
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, *FString::Printf(TEXT("Cosmos_%s_%d"), *Pc.Id, Builds));
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Mesh);
		// sight and the camera see it, as a placed prop; nothing to use
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		C->SetCollisionResponseToAllChannels(ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		C->SetCastShadow(true);
		C->RegisterComponent();
		// in the station model's own terms (three metres from its origin), with Unreal's yaw, pitch and roll on top
		C->AttachToComponent(Station, FAttachmentTransformRules::KeepRelativeTransform);
		C->SetRelativeTransform(FTransform(Rot, S47Three::ToUE(Pc.Town - Origin) - Anchor, FVector(Pc.Scale)));
		Comps.Add(C);
		if (Pc.bCollide)
		{
			// the walk block from its bounds, as AS47Props::AddCollider does (Unreal x is three's x, Unreal y three's z)
			const FBox B = C->Bounds.GetBox();
			FS47Rect R;
			R.MinX = B.Min.X / 100.0 + .03; R.MaxX = B.Max.X / 100.0 - .03;
			R.MinZ = B.Min.Y / 100.0 + .03; R.MaxZ = B.Max.Y / 100.0 - .03;
			R.Id = TEXT("cosmos:") + Pc.Id;
			if (R.MaxX > R.MinX && R.MaxZ > R.MinZ) { W->CollidersMutable(SSD::Walk).Add(R); ++Blocks; }
		}
	}
	if (Missing.Num())
		UE_LOG(LogS47StationDressing, Display, TEXT("S47: station dressing: %d of %d Cosmos pieces not here (Tools/cosmosshop), the shop as before without them: %s"),
			Missing.Num(), Pieces.Num(), *FString::Join(Missing, TEXT(", ")));
	if (Comps.Num()) UE_LOG(LogS47StationDressing, Display, TEXT("S47: station dressing: %d pieces in the shop (%d of them old stand-ins), %d in the way"), Comps.Num(), Standins, Blocks);
	if (Waiting) UE_LOG(LogS47StationDressing, Display, TEXT("S47: station dressing: %d real models wait until the station model has their stand-ins as parts of their own (a new Blender build, then Tools/freemodels/import_free.sh)"), Waiting);
	++Builds;
}

void FS47StationDressing::Step()
{
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W) return;
	const bool bFree = M->IsFreeform();
	if (bFree)
		if (UStaticMeshComponent* Station = AS47Props::PlacedMesh(SSD::StationId); Station && BuiltFor.Get() != Station) Build(Station);
	SetVisible(bFree);
}

bool FS47StationDressing::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s station dressing: %s"), bCond ? TEXT("PASS") : TEXT("FAIL"), *What)); bOk &= bCond; };
	FVector Origin;
	TArray<FPiece> Pieces;
	FString Error;
	const bool bRead = ReadFile(Origin, Pieces, &Error);
	Check(bRead && Pieces.Num() > 0, bRead ? FString::Printf(TEXT("Config/Props/gas_cosmos.json: %d pieces"), Pieces.Num()) : Error);
	if (!bRead) return bOk;
	TArray<FString> Bad;
	TSet<FString> Ids;
	int32 HereN = 0;
	for (const FPiece& Pc : Pieces)
	{
		const double X = Pc.Town.X, Y = Pc.Town.Y, Z = Pc.Town.Z;
		// a bought Cosmos piece by its path, or a shared catalogue model by its id (PROPS-REUSE-20261113) that the catalogue knows
		const bool bAsset = Pc.Model.IsEmpty() ? Pc.Asset.StartsWith(SSD::Root) : (S47PropCatalogue::Find(Pc.Model) != nullptr && Pc.Asset.StartsWith(TEXT("/Game/")));
		const bool bGood = !Pc.Id.IsEmpty() && !Ids.Contains(Pc.Id) && bAsset && Pc.Scale > .2 && Pc.Scale < 3
			&& X > SSD::X0 && X < SSD::X1 && Z > SSD::Z0 && Z < SSD::Z1 && Y >= SSD::Floor - .01 && Y < SSD::Ceiling + .01
			&& !SSD::Card.Has(X, Z) && !SSD::Attendant.Has(X, Z);
		Ids.Add(Pc.Id);
		if (!bGood) Bad.Add(Pc.Id.IsEmpty() ? TEXT("(no id)") : Pc.Id);
		HereN += SSD::Here(Pc.Asset) ? 1 : 0;
	}
	Check(Bad.IsEmpty(), Bad.IsEmpty() ? TEXT("every piece licensed or in the shared catalogue, sane in scale, inside the shop, off the card patch and the attendant's place")
		: TEXT("out of place: ") + FString::Join(Bad, TEXT(", ")));
	// PROPS-REUSE-20261113: the real model or its old stand-in, never both
	{
		const FUse A = Decide(true, true, true, true), B = Decide(true, true, false, true), C = Decide(false, true, true, true), D = Decide(true, true, false, false),
			E = Decide(false, false, false, false), F = Decide(true, false, false, true);
		Check(A.bModel && !A.bStandin && !B.bModel && !B.bStandin && !C.bModel && C.bStandin && D.bModel && !D.bStandin && !E.bModel && !E.bStandin && F.bModel && !F.bStandin,
			TEXT("real model or old stand-in: the model when here (a waiting one only once its stand-in is a part of its own), the stand-in only without it"));
		TArray<FString> BadStandin;
		for (const FPiece& Pc : Pieces)
			if (!Pc.Standin.IsEmpty() && !Pc.Standin.StartsWith(TEXT("/Game/Free/Town/gas_station/"))) BadStandin.Add(Pc.Id);
		Check(BadStandin.IsEmpty(), BadStandin.IsEmpty() ? FString(TEXT("every stand-in is a part of the station model (/Game/Free/Town/gas_station)")) : TEXT("stand-in not of the station: ") + FString::Join(BadStandin, TEXT(", ")));
	}
	// load-if-present: either way is a pass; the line says which
	Lines.Add(FString::Printf(TEXT("PASS station dressing: %d of %d Cosmos assets here%s"), HereN, Pieces.Num(),
		HereN == 0 ? TEXT(" (none: the shop as before)") : TEXT("")));
	return bOk;
}

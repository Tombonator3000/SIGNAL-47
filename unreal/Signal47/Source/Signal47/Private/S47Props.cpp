// Unreal only (departs from the web version): the free models placed in the world, see S47Props.h.
#include "S47Props.h"
#include "S47StationPhone.h"
#include "S47StationWorldRoute.h"
#include "S47TerrainStripTest.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RandomStream.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "S47Audio.h"
#include "S47Glow.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Props, Log, All);

TWeakObjectPtr<AS47Props> AS47Props::Instance;
TArray<AS47Props::FFloor> AS47Props::Floors;

namespace
{
	FString PropsDir() { return FPaths::ProjectConfigDir() / TEXT("Props"); }

	TSharedPtr<FJsonObject> ReadJson(const FString& Path)
	{
		FString Text;
		TSharedPtr<FJsonObject> O;
		if (!FFileHelper::LoadFileToString(Text, *Path)) return nullptr;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), O)) return nullptr;
		return O;
	}

	TArray<FString> PropFiles()
	{
		TArray<FString> Out;
		IFileManager::Get().FindFiles(Out, *(PropsDir() / TEXT("*.json")), true, false);
		// Config/Props is staged as one directory, but these two files belong to
		// S47Lookdev and S47Lighting, not the placement schema. Use the same explicit
		// discovery contract at runtime and in SelfTest; do not hide an unknown or
		// malformed placement file merely because its required area/props is absent.
		Out.RemoveAll([](const FString& Name)
		{
			return Name == TEXT("lookdev-bindings.json") || Name == TEXT("lookdev-environment.json");
		});
		Out.Sort();
		return Out;
	}

	FString Str(const TSharedPtr<FJsonObject>& O, const TCHAR* K, const FString& Default = FString()) { FString S; return O.IsValid() && O->TryGetStringField(K, S) ? S : Default; }
	double Num(const TSharedPtr<FJsonObject>& O, const TCHAR* K, double Default) { double D; return O.IsValid() && O->TryGetNumberField(K, D) ? D : Default; }
	bool Bool(const TSharedPtr<FJsonObject>& O, const TCHAR* K, bool Default) { bool B; return O.IsValid() && O->TryGetBoolField(K, B) ? B : Default; }
	TArray<FString> Strs(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		TArray<FString> Out;
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		if (O.IsValid() && O->TryGetArrayField(K, A)) for (const TSharedPtr<FJsonValue>& V : *A) Out.Add(V->AsString());
		return Out;
	}

	// a prop's place in three's terms; false when it has none. y may be null (NaN: found from above)
	bool At(const TSharedPtr<FJsonObject>& P, FVector& Out)
	{
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		if (!P->TryGetArrayField(TEXT("at"), A) || A->Num() != 3) return false;
		Out.X = (*A)[0]->AsNumber();
		Out.Y = (*A)[1]->IsNull() ? std::numeric_limits<double>::quiet_NaN() : (*A)[1]->AsNumber();
		Out.Z = (*A)[2]->AsNumber();
		return true;
	}

	// the props' three.js matrix (position, rotation.y, scale) as Unreal's (S47Three: the glTF swap);
	// the free models came in through the same glTF import, so a model's own axes are three's
	FTransform ThreeToUE(const FVector& P, double YawDeg, double Scale, double PitchDeg = 0, double RollDeg = 0)
	{
		double E[16];
		S47Three::Compose(P, S47Three::Euler(FMath::DegreesToRadians(PitchDeg), FMath::DegreesToRadians(YawDeg), FMath::DegreesToRadians(RollDeg)), FVector(Scale), E);
		return S47Three::MatToUE(E);
	}
}

AS47Props::AS47Props()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AS47Props::LoadManifests()
{
	if (const TSharedPtr<FJsonObject> Free = ReadJson(FPaths::ProjectContentDir() / TEXT("Free/free_models.json")))
	{
		for (const TSharedPtr<FJsonValue>& V : Free->GetArrayField(TEXT("models")))
		{
			const TSharedPtr<FJsonObject> O = V->AsObject();
			const FString Asset = O->GetStringField(TEXT("asset"));
			Assets.Add(FPaths::GetBaseFilename(Asset), Asset);
			if (!Assets.Contains(O->GetStringField(TEXT("name")))) Assets.Add(O->GetStringField(TEXT("name")), Asset);
		}
		const TSharedPtr<FJsonObject>* Tr = nullptr;
		if (Free->TryGetObjectField(TEXT("trims"), Tr)) Trims = *Tr;
	}
	// the manifest's areas (export.js: the nodes each area shows), read without the rest of it
	FString Text;
	if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("WebData/manifest.json"))))
	{
		const int32 K = Text.Find(TEXT("\"areas\":"));
		if (K >= 0)
		{
			const int32 S = Text.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, K);
			const int32 E = S >= 0 ? Text.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, S) : -1;
			TSharedPtr<FJsonObject> A;
			if (E > S && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text.Mid(S, E - S + 1)), A) && A.IsValid())
			{
				for (const auto& KV : A->Values)
				{
					TSet<int32>& Set = AreaNodes.FindOrAdd(FString(KV.Key));
					for (const TSharedPtr<FJsonValue>& V : KV.Value->AsArray()) Set.Add(int32(V->AsNumber()));
				}
			}
		}
	}
}

UStaticMesh* AS47Props::MeshOf(const FString& Model) const
{
	const FString* Asset = Assets.Find(Model);
	const FString Path = Asset ? *Asset : (Model.StartsWith(TEXT("/")) ? Model : FString());
	if (Path.IsEmpty()) return nullptr;
	return LoadObject<UStaticMesh>(nullptr, *Path);
}

bool AS47Props::InArea(int32 Node, const TArray<FString>& Areas) const
{
	const AS47Web* W = Web.Get();
	for (int32 I = Node; W && I >= 0; I = W->Node(I) ? W->Node(I)->Parent : -1)
	{
		for (const FString& A : Areas) if (const TSet<int32>* S = AreaNodes.Find(A)) if (S->Contains(I)) return true;
	}
	return false;
}

bool AS47Props::Ground(const FVector& Three, double From, double To, const TArray<FString>& Areas, const TArray<UPrimitiveComponent*>& Ignore, double& OutY) const
{
	// the first surface under the spot that belongs to the area (the areas lie over each other in
	// places, far apart in the story but not in space): whatever else is hit is stepped through
	UWorld* W = GetWorld();
	if (!W || !Web.IsValid()) return false;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(S47PropsGround), true);
	Q.AddIgnoredComponents(Ignore);
	for (const FPlaced& P : Placed) if (P.Comp.IsValid()) Q.AddIgnoredComponent(P.Comp.Get());
	const FVector A = S47Three::ToUE(Three.X, From, Three.Z), B = S47Three::ToUE(Three.X, To, Three.Z);
	for (int32 Tries = 0; Tries < 24; ++Tries)
	{
		FHitResult Hit;
		if (!W->LineTraceSingleByChannel(Hit, A, B, ECC_Visibility, Q)) return false;
		UPrimitiveComponent* C = Hit.GetComponent();
		const int32 N = Web->NodeOf(C);
		if (N >= 0 && InArea(N, Areas)) { OutY = Hit.ImpactPoint.Z / 100.0; return true; }
		Q.AddIgnoredComponent(C);
	}
	return false;
}

void AS47Props::HideMeshes(int32 Node, TArray<UPrimitiveComponent*>& Hidden)
{
	// out of sight, but still there for the use trace and the floor (and the story's own changes to
	// its visibility, which leave bHiddenInGame alone)
	const FS47WebNode* N = Web.IsValid() ? Web->Node(Node) : nullptr;
	if (!N) return;
	if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(N->Comp.Get()))
	{
		P->SetHiddenInGame(true);
		P->SetCastShadow(false);
		Hidden.Add(P);
	}
	// a glow (world/glow.ts points): its discs shrunk to nothing, the points kept for the story
	if (FS47GlowPoints* G = Web->Glow(Node)) for (int32 I = 0; I < G->Num(); ++I) G->SetSize(I, 0.f);
	for (int32 Ch : N->Children)
	{
		// (not a prop placed under it)
		if (Placed.ContainsByPredicate([Ch](const FPlaced& P) { return P.Node == Ch; })) continue;
		HideMeshes(Ch, Hidden);
	}
}

void AS47Props::Build(AS47Web* InWeb)
{
	Web = InWeb;
	Instance = this;
	Floors.Reset();
	if (!InWeb || !InWeb->IsLoaded()) return;
	LoadManifests();
	if (Assets.IsEmpty()) UE_LOG(LogS47Props, Warning, TEXT("S47: props: no Content/Free/free_models.json (Tools/freemodels/import_free.sh): nothing placed, nothing hidden"));
	PaperNode = InWeb->Handle(TEXT("room.printerPaper"));
	FString Tour;
	TArray<FString> Only;
	if (FParse::Value(FCommandLine::Get(), TEXT("S47PropsTour="), Tour, false))
	{
		FString Names;
		if (Tour.Split(TEXT("@"), &TourDir, &Names)) Names.ParseIntoArray(Only, TEXT(","));
		else TourDir = Tour;
	}
	const bool bOff = FParse::Param(FCommandLine::Get(), TEXT("S47PropsOff"));
	for (const FString& F : PropFiles())
	{
		const TSharedPtr<FJsonObject> File = ReadJson(PropsDir() / F);
		if (!File) { UE_LOG(LogS47Props, Error, TEXT("S47: props: %s does not read"), *F); continue; }
		if (!TourDir.IsEmpty()) ReadViews(File, Only);
		if (!bOff) BuildFile(File, F);
	}
	if (!TourDir.IsEmpty()) UE_LOG(LogS47Props, Display, TEXT("S47: props tour of %d views into %s%s"), Views.Num(), *TourDir, bOff ? TEXT(" without the props") : TEXT(""));
	UE_LOG(LogS47Props, Display, TEXT("S47: props placed %d"), Placed.Num());
	// -S47PropsDoor=<use id>: that door open from the start (the walk in, the pictures inside)
	FString OpenDoor;
	if (FParse::Value(FCommandLine::Get(), TEXT("S47PropsDoor="), OpenDoor)) UseDoor(OpenDoor);
}

void AS47Props::BuildFile(const TSharedPtr<FJsonObject>& File, const FString& Name)
{
	AS47Web* W = Web.Get();
	const FString Area = Str(File, TEXT("area"), TEXT("saro"));
	const FString Walk = Str(File, TEXT("walk"), Area);
	const FString Leg = Str(File, TEXT("leg"));
	TArray<FString> Areas = Strs(File, TEXT("grounds"));
	if (Areas.IsEmpty()) Areas.Add(Area);
	// the default node to hang under: the file's, or the area's first root
	int32 FileParent = W->Handle(Str(File, TEXT("parent")));
	if (FileParent < 0) if (const TSet<int32>* S = AreaNodes.Find(Area)) if (TSet<int32>::TConstIterator It(*S); It) FileParent = *It;
	int32 Count = 0, Blocks = 0;
	const TArray<TSharedPtr<FJsonValue>> None;
	const TArray<TSharedPtr<FJsonValue>>* Props = &None;
	File->TryGetArrayField(TEXT("props"), Props);
	for (const TSharedPtr<FJsonValue>& V : *Props)
	{
		const TSharedPtr<FJsonObject> P = V->AsObject();
		const FString Id = Str(P, TEXT("id"), Str(P, TEXT("model")));
		UStaticMesh* Mesh = MeshOf(Str(P, TEXT("model")));
		FVector Th;
		if (!Mesh || !At(P, Th)) { UE_LOG(LogS47Props, Warning, TEXT("S47: props: %s %s: no mesh or no place"), *Name, *Id); continue; }
		// what it replaces: out of sight, and the first is its parent
		TArray<UPrimitiveComponent*> Hidden;
		int32 Parent = W->Handle(Str(P, TEXT("parent")));
		for (const FString& R : Strs(P, TEXT("replaces")))
		{
			const int32 N = W->Handle(R);
			if (N < 0) { UE_LOG(LogS47Props, Warning, TEXT("S47: props: %s %s replaces %s, which the export has not"), *Name, *Id, *R); continue; }
			HideMeshes(N, Hidden);
			if (Parent < 0) Parent = N;
		}
		if (Parent < 0) Parent = FileParent;
		const FS47WebNode* PN = W->Node(Parent);
		if (!PN) continue;
		const bool bLocal = Bool(P, TEXT("local"), false);
		// on the ground (or the desk) under it
		if (!bLocal && (Bool(P, TEXT("ground"), true) || FMath::IsNaN(Th.Y)))
		{
			const double Lift = Num(P, TEXT("lift"), 0.6);
			const bool bHigh = FMath::IsNaN(Th.Y);
			double Y = 0;
			if (Ground(Th, bHigh ? 600.0 : Th.Y + Lift, bHigh ? -600.0 : Th.Y - 4.0, Areas, Hidden, Y)) Th.Y = Y - Num(P, TEXT("sink"), 0.0);
			else if (bHigh) { UE_LOG(LogS47Props, Warning, TEXT("S47: props: %s %s: no ground under %.1f %.1f"), *Name, *Id, Th.X, Th.Z); Th.Y = 0; }
		}
		const double Yaw = Num(P, TEXT("yaw"), 0), Scale = Num(P, TEXT("scale"), 1);
		// Office files can stand upright or lean inside a magazine holder. Existing
		// placements retain zero pitch/roll, including every door and vehicle.
		const double Pitch = Num(P, TEXT("pitch"), 0), Roll = Num(P, TEXT("roll"), 0);
		const FTransform T = ThreeToUE(Th, Yaw, Scale, Pitch, Roll);
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, *FString::Printf(TEXT("Prop_%s_%d"), *Id, Placed.Num()));
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Mesh);
		if(Id==TEXT("stationPayphone"))S47StationPhone::Register(W,C);
		// the floor and sight see it; the use trace only when it stands for a thing to use
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		C->SetCollisionResponseToAllChannels(ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		// (trace false: the exported things to use behind it answer the use trace as before)
		if (!Hidden.IsEmpty() && Bool(P, TEXT("trace"), true)) C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		C->SetCastShadow(true);
		C->RegisterComponent();
		if (bLocal)
		{
			// in the parent's own terms: it goes where the story moves the node (S47Town PlaceCar)
			if (FMath::IsNaN(Th.Y)) Th.Y = 0;
			C->AttachToComponent(PN->Comp.IsValid() ? PN->Comp.Get() : W->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			C->SetRelativeTransform(ThreeToUE(Th, Yaw, Scale, Pitch, Roll));
		}
		else
		{
			C->AttachToComponent(PN->Comp.IsValid() ? PN->Comp.Get() : W->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
			C->SetWorldTransform(T);
		}
		FPlaced Pl;
		Pl.Id = Id;
		Pl.Comp = C;
		Pl.Node = W->AdoptNode(C, Parent, TEXT("prop:") + Id);
		Pl.When = W->Handle(Str(P, TEXT("when")));
		Pl.bPaper = Bool(P, TEXT("paper"), false);
		Pl.Rest = C->GetRelativeTransform();
		Placed.Add(Pl);
		++Count;
		const TSharedPtr<FJsonObject>* Sub = nullptr;
		if (P->TryGetObjectField(TEXT("swing"), Sub)) AddSwing(C, *Sub);
		if (P->TryGetObjectField(TEXT("beacon"), Sub)) AddBeacon(C, *Sub);
		if (P->TryGetObjectField(TEXT("lit"), Sub)) AddLit(C, Pl.Node, *Sub);
		if (P->TryGetObjectField(TEXT("door"), Sub)) AddDoor(C, *Sub, Str(P, TEXT("walk"), Walk), T);
		if (P->HasField(TEXT("trim"))) ApplyTrim(Str(P, TEXT("trim")));
		// unblock: the exported colliders of what it opens up (a building one walks into now), moved away
		const FString UWalk = Str(P, TEXT("walk"), Walk);
		const TArray<TSharedPtr<FJsonValue>>* Un = nullptr;
		if (P->TryGetArrayField(TEXT("unblock"), Un)) for (const TSharedPtr<FJsonValue>& UV : *Un)
		{
			const TArray<TSharedPtr<FJsonValue>>& Q = UV->AsArray();
			if (Q.Num() != 4) continue;
			int32 Moved = 0;
			for (FS47Rect& CR : W->CollidersMutable(UWalk))
			{
				if (FMath::Abs(CR.MinX - Q[0]->AsNumber()) < 0.06 && FMath::Abs(CR.MaxX - Q[1]->AsNumber()) < 0.06 && FMath::Abs(CR.MinZ - Q[2]->AsNumber()) < 0.06 && FMath::Abs(CR.MaxZ - Q[3]->AsNumber()) < 0.06)
				{
					// (still there for what grows round the town's walls, S47WorldLife: only the walk steps over it)
					CR.bEnabled = false;
					++Moved;
				}
			}
			if (!Moved) UE_LOG(LogS47Props, Warning, TEXT("S47: props: %s unblock %.2f %.2f %.2f %.2f: no such collider in %s"), *Id, Q[0]->AsNumber(), Q[1]->AsNumber(), Q[2]->AsNumber(), Q[3]->AsNumber(), *UWalk);
		}
		// zones: walkable ground of its own (its inside), floors: the heights there (a ramp along x or z)
		auto ToWorld = [&T](double X, double Z) { const FVector U = T.TransformPosition(S47Three::ToUE(X, 0.0, Z)); return FVector2D(U.X / 100.0, U.Y / 100.0); };
		const TArray<TSharedPtr<FJsonValue>>* Zs = nullptr;
		if (!bLocal && P->TryGetArrayField(TEXT("zones"), Zs)) for (const TSharedPtr<FJsonValue>& ZV : *Zs)
		{
			const TArray<TSharedPtr<FJsonValue>>& Q = ZV->AsArray();
			if (Q.Num() != 4) continue;
			const FVector2D A = ToWorld(Q[0]->AsNumber(), Q[2]->AsNumber()), B = ToWorld(Q[1]->AsNumber(), Q[3]->AsNumber());
			FS47Rect Z;
			Z.MinX = FMath::Min(A.X, B.X); Z.MaxX = FMath::Max(A.X, B.X); Z.MinZ = FMath::Min(A.Y, B.Y); Z.MaxZ = FMath::Max(A.Y, B.Y);
			Z.Id = TEXT("prop:") + Id;
			W->ZonesMutable(UWalk).Add(Z);
		}
		const TArray<TSharedPtr<FJsonValue>>* Fs = nullptr;
		if (!bLocal && P->TryGetArrayField(TEXT("floors"), Fs)) for (const TSharedPtr<FJsonValue>& FV : *Fs)
		{
			const TArray<TSharedPtr<FJsonValue>>& Q = FV->AsArray();
			if (Q.Num() < 5) continue;
			const FVector2D A = ToWorld(Q[0]->AsNumber(), Q[2]->AsNumber()), B = ToWorld(Q[1]->AsNumber(), Q[3]->AsNumber());
			FFloor Fl;
			Fl.MinX = FMath::Min(A.X, B.X); Fl.MaxX = FMath::Max(A.X, B.X); Fl.MinZ = FMath::Min(A.Y, B.Y); Fl.MaxZ = FMath::Max(A.Y, B.Y);
			const double Base = T.GetLocation().Z / 100.0;
			Fl.Y0 = Base + Q[4]->AsNumber();
			Fl.Y1 = Base + (Q.Num() > 5 ? Q[5]->AsNumber() : Q[4]->AsNumber());
			Fl.Axis = Q.Num() > 6 ? Q[6]->AsString() : FString(TEXT("x"));
			Floors.Add(Fl);
		}
		// blocks: its own footprints in the walk and the drive (a building's island, steps and drums);
		// walk: a walk of its own (Caliche's street is the road's walk, the diner's lot the diner's)
		const FString PWalk = Str(P, TEXT("walk"), Walk);
		// (leg: the drive legs its blocks stand in, if not the file's; "" for the walk only)
		const FString PLeg = P->HasField(TEXT("leg")) ? Str(P, TEXT("leg")) : Leg;
		const TArray<TSharedPtr<FJsonValue>>* Rects = nullptr;
		if (!bLocal && P->TryGetArrayField(TEXT("blocks"), Rects))
		{
			int32 K = 0;
			for (const TSharedPtr<FJsonValue>& RV : *Rects)
			{
				const TArray<TSharedPtr<FJsonValue>>& Q = RV->AsArray();
				if (Q.Num() != 4) continue;
				FBox Box(ForceInit);
				for (const double X : { Q[0]->AsNumber(), Q[1]->AsNumber() }) for (const double Z : { Q[2]->AsNumber(), Q[3]->AsNumber() })
					Box += T.TransformPosition(S47Three::ToUE(X, 0.0, Z));
				Box.Max.Z += 100.0;
				Blocks += AddCollider(PWalk, PLeg, Box, 0.0, FString::Printf(TEXT("prop:%s:%d"), *Id, K++));
			}
		}
		// in the way: the walk's collider (and the drive's), the model's footprint turned as it stands
		// (a local prop moves: what it stands for keeps its own collider, the patrol car's carCol)
		if (Bool(P, TEXT("collide"), false) && !bLocal) Blocks += AddCollider(PWalk, Leg, Mesh->GetBoundingBox().TransformBy(T), Num(P, TEXT("pad"), 0.05), TEXT("prop:") + Id);
	}
	Blocks += BuildRows(File, Name, Walk, Leg, Areas, FileParent, Count);
	Blocks += BuildScatter(File, Name, Walk, Leg, Areas, FileParent, Count);
	UE_LOG(LogS47Props, Display, TEXT("S47: props %s: %d placed, %d in the way"), *Name, Count, Blocks);
}

int32 AS47Props::AddCollider(const FString& Walk, const FString& Leg, const FBox& B, double Pad, const FString& Id)
{
	AS47Web* W = Web.Get();
	FS47Rect R;
	R.MinX = B.Min.X / 100.0 + Pad; R.MaxX = B.Max.X / 100.0 - Pad; // (Unreal x is three's x, Unreal y three's z)
	R.MinZ = B.Min.Y / 100.0 + Pad; R.MaxZ = B.Max.Y / 100.0 - Pad;
	R.Id = Id;
	if (!W || R.MaxX <= R.MinX || R.MaxZ <= R.MinZ) return 0;
	W->CollidersMutable(Walk).Add(R);
	// the drive's legs read their obstacles from the export (US47World ObstaclesOf): a box
	// [minX, maxX, minZ, maxZ]; SARO's leg takes the walk's colliders as they are
	// ("leg" may name several, "road,roswell": the diner's lot is in both)
	const TSharedPtr<FJsonObject>& D = W->DriveData();
	TArray<FString> LegNames;
	Leg.ParseIntoArray(LegNames, TEXT(","));
	for (const FString& LegName : LegNames)
	{
		const TSharedPtr<FJsonObject>* Legs = nullptr, *L = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Obs = nullptr;
		if (!D.IsValid() || !D->TryGetObjectField(TEXT("legs"), Legs) || !(*Legs)->TryGetObjectField(LegName, L) || !(*L)->TryGetArrayField(TEXT("obstacles"), Obs)) continue;
		TArray<TSharedPtr<FJsonValue>> Box = { MakeShared<FJsonValueNumber>(R.MinX), MakeShared<FJsonValueNumber>(R.MaxX), MakeShared<FJsonValueNumber>(R.MinZ), MakeShared<FJsonValueNumber>(R.MaxZ) };
		TArray<TSharedPtr<FJsonValue>> All = *Obs;
		All.Add(MakeShared<FJsonValueArray>(Box));
		(*L)->SetArrayField(TEXT("obstacles"), All);
	}
	return 1;
}

UInstancedStaticMeshComponent* AS47Props::Instanced(UStaticMesh* Mesh, int32 Parent, const FString& Id, bool bShadow)
{
	AS47Web* W = Web.Get();
	const FS47WebNode* PN = W ? W->Node(Parent) : nullptr;
	if (!Mesh || !PN) return nullptr;
	UInstancedStaticMeshComponent* C = NewObject<UInstancedStaticMeshComponent>(W, *FString::Printf(TEXT("PropISM_%s_%d"), *Id, Placed.Num()));
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(Mesh);
	C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	C->SetCollisionResponseToAllChannels(ECR_Ignore);
	C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	C->SetCastShadow(bShadow);
	// (far things: drawn to the horizon, culled per instance by distance only where the area says)
	C->RegisterComponent();
	C->AttachToComponent(PN->Comp.IsValid() ? PN->Comp.Get() : W->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
	C->SetWorldTransform(FTransform::Identity);
	FPlaced Pl;
	Pl.Id = Id;
	Pl.Node = W->AdoptNode(C, Parent, TEXT("props:") + Id);
	Pl.Rest = C->GetRelativeTransform();
	Placed.Add(Pl);
	return C;
}

int32 AS47Props::BuildRows(const TSharedPtr<FJsonObject>& File, const FString& Name, const FString& Walk, const FString& Leg, const TArray<FString>& Areas, int32 Parent, int32& Count)
{
	// "rows": [ { id, model, from [x, z], to [x, z], step, yaw (degrees, 0: the model's x along the
	// row), scale, sink, collide, pad, wires { at [[across, up], ...] (metres from the pole's foot,
	// across the row and up), sag, radius } } ]: a fence or a line of poles on the ground, one
	// instance every step; the wires hang between the poles in a shallow curve
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!File->TryGetArrayField(TEXT("rows"), Rows)) return 0;
	int32 Blocks = 0;
	UInstancedStaticMeshComponent* WireIsm = nullptr;
	for (const TSharedPtr<FJsonValue>& V : *Rows)
	{
		const TSharedPtr<FJsonObject> R = V->AsObject();
		const FString Id = Str(R, TEXT("id"), Str(R, TEXT("model")));
		UStaticMesh* Mesh = MeshOf(Str(R, TEXT("model")));
		const TArray<TSharedPtr<FJsonValue>>* From = nullptr, *To = nullptr;
		if (!Mesh || !R->TryGetArrayField(TEXT("from"), From) || !R->TryGetArrayField(TEXT("to"), To) || From->Num() < 2 || To->Num() < 2) { UE_LOG(LogS47Props, Warning, TEXT("S47: props: %s row %s: no mesh or no line"), *Name, *Id); continue; }
		const FVector2D A((*From)[0]->AsNumber(), (*From)[1]->AsNumber()), B((*To)[0]->AsNumber(), (*To)[1]->AsNumber());
		const double Len = (B - A).Size(), Step = FMath::Max(0.2, Num(R, TEXT("step"), 10));
		if (Len < 0.01) continue;
		const FVector2D D = (B - A) / Len;
		const double Heading = FMath::RadiansToDegrees(FMath::Atan2(-D.Y, D.X)); // three's rotation.y that turns +x onto the row
		const double Yaw = Heading + Num(R, TEXT("yaw"), 0), Scale = Num(R, TEXT("scale"), 1), Sink = Num(R, TEXT("sink"), 0);
		UInstancedStaticMeshComponent* Ism = Instanced(Mesh, Parent, Id, Bool(R, TEXT("shadow"), true));
		if (!Ism) continue;
		// scale3 [x, y, z]: a stretched shape (the engine's cylinder as a fence post, three's axes);
		// color [r, g, b]: its own plain material (linear)
		FVector Scale3(Scale);
		const TArray<TSharedPtr<FJsonValue>>* S3 = nullptr;
		if (R->TryGetArrayField(TEXT("scale3"), S3) && S3->Num() == 3) Scale3 = FVector((*S3)[0]->AsNumber(), (*S3)[1]->AsNumber(), (*S3)[2]->AsNumber());
		const TArray<TSharedPtr<FJsonValue>>* Col = nullptr;
		if (R->TryGetArrayField(TEXT("color"), Col) && Col->Num() == 3)
		{
			if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque")))
			{
				UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, this);
				M->SetVectorParameterValue(TEXT("Color"), FLinearColor(float((*Col)[0]->AsNumber()), float((*Col)[1]->AsNumber()), float((*Col)[2]->AsNumber())));
				for (int32 K = 0; K < Ism->GetNumMaterials(); ++K) Ism->SetMaterial(K, M);
			}
		}
		TArray<FVector> Feet;
		const int32 N = int32(Len / Step + 1e-6) + 1;
		for (int32 I = 0; I < N; ++I)
		{
			const FVector2D P = A + D * (I * Step);
			FVector Th(P.X, FMath::Max(-0.62, 0.0), P.Y);
			double Y = 0;
			if (!Ground(Th, 600.0, -600.0, Areas, {}, Y)) continue;
			Th.Y = Y - Sink;
			double E[16];
			S47Three::Compose(Th, S47Three::Euler(0, FMath::DegreesToRadians(Yaw), 0), Scale3, E);
			const FTransform T = S47Three::MatToUE(E);
			Ism->AddInstance(T, true);
			Feet.Add(FVector(Th.X, Y, Th.Z)); // (the wires from the ground, not from the sunk foot)
			if (Bool(R, TEXT("collide"), false)) Blocks += AddCollider(Walk, Leg, Mesh->GetBoundingBox().TransformBy(T), Num(R, TEXT("pad"), 0.0), FString::Printf(TEXT("props:%s:%d"), *Id, I));
		}
		Count += Feet.Num();
		const TSharedPtr<FJsonObject>* Wires = nullptr;
		if (!R->TryGetObjectField(TEXT("wires"), Wires) || Feet.Num() < 2) continue;
		if (!WireIsm) WireIsm = Instanced(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")), Parent, Id + TEXT("_wires"), false);
		if (!WireIsm) continue;
		if (WireIsm->GetNumMaterials() > 0 && WireIsm->GetMaterial(0) && !WireIsm->GetMaterial(0)->IsA<UMaterialInstanceDynamic>())
		{
			if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque")))
			{
				UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, this);
				M->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.012f, 0.011f, 0.010f));
				WireIsm->SetMaterial(0, M);
			}
		}
		const double Sag = Num(*Wires, TEXT("sag"), 0.8), Radius = Num(*Wires, TEXT("radius"), 0.012);
		const FVector Across(-D.Y, 0, D.X); // (three: perpendicular to the row on the ground)
		const TArray<TSharedPtr<FJsonValue>>* At = nullptr;
		if (!(*Wires)->TryGetArrayField(TEXT("at"), At)) continue;
		constexpr int32 Segs = 10;
		for (int32 I = 0; I + 1 < Feet.Num(); ++I)
		{
			if (FVector::Dist2D(Feet[I], Feet[I + 1]) > Step * 1.6) continue; // (a gap in the row: no wire over it)
			for (const TSharedPtr<FJsonValue>& O : *At)
			{
				const TArray<TSharedPtr<FJsonValue>>& Off = O->AsArray();
				if (Off.Num() < 2) continue;
				const FVector Lift = Across * Off[0]->AsNumber() * Scale + FVector(0, Off[1]->AsNumber() * Scale, 0);
				const FVector P0 = Feet[I] + Lift, P1 = Feet[I + 1] + Lift;
				FVector Prev = S47Three::ToUE(P0);
				for (int32 K = 1; K <= Segs; ++K)
				{
					const double T = double(K) / Segs;
					FVector Q = FMath::Lerp(P0, P1, T);
					Q.Y -= 4 * Sag * T * (1 - T);
					const FVector Cur = S47Three::ToUE(Q);
					const FVector Dir = Cur - Prev;
					const double L = Dir.Size();
					if (L > 1)
					{
						// the engine's cylinder: 100 cm high along z, 100 cm across, its pivot in the middle
						const FTransform Seg(FRotationMatrix::MakeFromZ(Dir / L).ToQuat(), (Cur + Prev) * 0.5, FVector(Radius * 2, Radius * 2, L / 100.0));
						WireIsm->AddInstance(Seg, true);
					}
					Prev = Cur;
				}
			}
		}
	}
	return Blocks;
}

int32 AS47Props::BuildScatter(const TSharedPtr<FJsonObject>& File, const FString& Name, const FString& Walk, const FString& Leg, const TArray<FString>& Areas, int32 Parent, int32& Count)
{
	// "scatter": [ { id, models [names], rect [minX, maxX, minZ, maxZ], count, seed, scale [min, max],
	// sink, spacing, avoid [[minX, maxX, minZ, maxZ], ...], collide (from this many metres across),
	// tilt (degrees) } ]: things strewn on the ground, the same each run (the seed), none in the
	// rectangles to avoid (roads, paths, buildings), none closer than the spacing
	const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
	if (!File->TryGetArrayField(TEXT("scatter"), List)) return 0;
	int32 Blocks = 0;
	auto Rect = [](const TArray<TSharedPtr<FJsonValue>>& A, double R[4]) { if (A.Num() < 4) return false; for (int32 I = 0; I < 4; ++I) R[I] = A[I]->AsNumber(); return true; };
	for (const TSharedPtr<FJsonValue>& V : *List)
	{
		const TSharedPtr<FJsonObject> S = V->AsObject();
		const FString Id = Str(S, TEXT("id"), TEXT("scatter"));
		TArray<UStaticMesh*> Meshes;
		TArray<UInstancedStaticMeshComponent*> Isms;
		for (const FString& Model : Strs(S, TEXT("models")))
		{
			UStaticMesh* M = MeshOf(Model);
			if (!M) { UE_LOG(LogS47Props, Warning, TEXT("S47: props: %s scatter %s: no model %s"), *Name, *Id, *Model); continue; }
			Meshes.Add(M);
			Isms.Add(nullptr);
		}
		const TArray<TSharedPtr<FJsonValue>>* RA = nullptr;
		double Box[4];
		if (Meshes.IsEmpty() || !S->TryGetArrayField(TEXT("rect"), RA) || !Rect(*RA, Box)) continue;
		TArray<TArray<double>> Avoid;
		const TArray<TSharedPtr<FJsonValue>>* AA = nullptr;
		if (S->TryGetArrayField(TEXT("avoid"), AA)) for (const TSharedPtr<FJsonValue>& X : *AA) { double Q[4]; if (Rect(X->AsArray(), Q)) Avoid.Add({ Q[0], Q[1], Q[2], Q[3] }); }
		double ScaleMin = 1, ScaleMax = 1;
		const TArray<TSharedPtr<FJsonValue>>* SA = nullptr;
		if (S->TryGetArrayField(TEXT("scale"), SA) && SA->Num() == 2) { ScaleMin = (*SA)[0]->AsNumber(); ScaleMax = (*SA)[1]->AsNumber(); }
		const int32 Want = int32(Num(S, TEXT("count"), 10));
		const double Spacing = Num(S, TEXT("spacing"), 2), Sink = Num(S, TEXT("sink"), 0), CollideFrom = Num(S, TEXT("collide"), 1e9), Tilt = Num(S, TEXT("tilt"), 0);
		FRandomStream Rng(int32(Num(S, TEXT("seed"), 1)));
		TArray<FVector2D> Taken;
		int32 Made = 0;
		for (int32 Try = 0; Try < Want * 30 && Made < Want; ++Try)
		{
			const FVector2D P(Rng.FRandRange(Box[0], Box[1]), Rng.FRandRange(Box[2], Box[3]));
			const int32 K = Rng.RandRange(0, Meshes.Num() - 1);
			const double Sc = Rng.FRandRange(ScaleMin, ScaleMax), Yaw = Rng.FRandRange(0, 360);
			const double Rx = Rng.FRandRange(-Tilt, Tilt), Rz = Rng.FRandRange(-Tilt, Tilt);
			const FVector Ext = Meshes[K]->GetBoundingBox().GetExtent() * Sc / 100.0;
			const double Reach = FMath::Max(Ext.X, Ext.Y);
			bool bOk = true;
			for (const TArray<double>& Q : Avoid) if (P.X > Q[0] - Reach && P.X < Q[1] + Reach && P.Y > Q[2] - Reach && P.Y < Q[3] + Reach) { bOk = false; break; }
			for (const FVector2D& O : Taken) if (bOk && FVector2D::Distance(O, P) < Spacing + Reach) bOk = false;
			if (!bOk) continue;
			FVector Th(P.X, 0, P.Y);
			double Y = 0;
			if (!Ground(Th, 600.0, -600.0, Areas, {}, Y)) continue;
			Th.Y = Y - Sink * Sc;
			if (!Isms[K]) Isms[K] = Instanced(Meshes[K], Parent, FString::Printf(TEXT("%s_%d"), *Id, K), true);
			if (!Isms[K]) continue;
			double E[16];
			S47Three::Compose(Th, S47Three::Euler(FMath::DegreesToRadians(Rx), FMath::DegreesToRadians(Yaw), FMath::DegreesToRadians(Rz)), FVector(Sc), E);
			const FTransform T = S47Three::MatToUE(E);
			Isms[K]->AddInstance(T, true);
			Taken.Add(P);
			++Made;
			if (2 * Reach >= CollideFrom) Blocks += AddCollider(Walk, Leg, Meshes[K]->GetBoundingBox().TransformBy(T), FMath::Min(0.25, Reach * 0.2), FString::Printf(TEXT("props:%s:%d"), *Id, Made));
		}
		Count += Made;
		if (Made < Want) UE_LOG(LogS47Props, Display, TEXT("S47: props %s scatter %s: %d of %d found room"), *Name, *Id, Made, Want);
	}
	return Blocks;
}

void AS47Props::ReadViews(const TSharedPtr<FJsonObject>& File, const TArray<FString>& Only)
{
	const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
	if (!File->TryGetArrayField(TEXT("views"), A)) return;
	for (const TSharedPtr<FJsonValue>& V : *A)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		const TArray<TSharedPtr<FJsonValue>>* At = nullptr;
		FView W;
		W.Name = Str(O, TEXT("name"));
		W.Area = Str(O, TEXT("area"), Str(File, TEXT("area"), TEXT("saro")));
		W.Car = Str(O, TEXT("car"));
		W.Clock = Num(O, TEXT("clock"), -1);
		if (W.Name.IsEmpty() || !O->TryGetArrayField(TEXT("at"), At) || At->Num() < 3) continue;
		if (!Only.IsEmpty() && !Only.ContainsByPredicate([&W](const FString& N) { return W.Name.StartsWith(N); })) continue;
		W.X = (*At)[0]->AsNumber(); W.Z = (*At)[1]->AsNumber(); W.Yaw = (*At)[2]->AsNumber();
		W.Pitch = At->Num() > 3 ? (*At)[3]->AsNumber() : 0;
		Views.Add(W);
	}
}

void AS47Props::Tour(float Dt)
{
	// a view every 4 s once the world has had 6 s to settle (the lights, the sky's capture); the
	// picture 3 s into each
	US47Main* M = US47Main::Get(this);
	if (TourDir.IsEmpty() || !M || !M->Player || !M->World) return;
	TourT += Dt;
	const int32 Step = TourT < 6 ? -1 : int32((TourT - 6) / 4);
	if (Step >= Views.Num())
	{
		if (TourT > 6 + Views.Num() * 4 + 1.5) { UE_LOG(LogS47Props, Display, TEXT("S47: props tour done, %d pictures"), Views.Num()); FPlatformMisc::RequestExit(false, TEXT("S47PropsTour")); TourDir.Empty(); }
		return;
	}
	if (Step < 0) return;
	const FView& V = Views[Step];
	if (Step != TourStep)
	{
		TourStep = Step;
		bTourShot = false;
		if (M->Web && M->Web->Area() != V.Area) M->World->Enter(V.Area);
		if (V.Clock >= 0 && M->Story) M->Story->SetClock(V.Clock);
		if (!V.Car.IsEmpty() && M->TownLayer && M->TownLayer->Town)
		{
			// the deputy's car: 'out' on the shoulder (event 12), 'in' at the office
			M->TownLayer->Town->SetDeputy(V.Car == TEXT("in"));
			if (V.Car == TEXT("out")) M->TownLayer->Town->SetCarOut(true);
		}
		M->Player->Place(V.X, V.Z, V.Yaw);
		M->Player->Pitch = V.Pitch;
		UE_LOG(LogS47Props, Display, TEXT("S47: props tour %d/%d %s (%s %.2f %.2f)"), Step + 1, Views.Num(), *V.Name, *V.Area, V.X, V.Z);
	}
	if (!bTourShot && TourT >= 6 + Step * 4 + 3)
	{
		bTourShot = true;
		FScreenshotRequest::RequestScreenshot(FPaths::Combine(TourDir, V.Name + TEXT(".png")), false, false);
	}
}

bool AS47Props::Shown(int32 Node) const
{
	const AS47Web* W = Web.Get();
	if (!W || !W->Node(Node)) return false;
	for (int32 I = Node; I >= 0; I = W->Node(I)->Parent) if (!W->IsVisible(I)) return false;
	return true;
}

bool AS47Props::RetargetPatrolCar(UStaticMesh* Body,UStaticMesh* Door,const FVector& HingeThree)
{
 AS47Props* P=Instance.Get();if(!P||!Body||!Door)return false;
 FPlaced* BodyPlace=P->Placed.FindByPredicate([](const FPlaced& V){return V.Id==TEXT("patrolCar");});
 FPlaced* DoorPlace=P->Placed.FindByPredicate([](const FPlaced& V){return V.Id==TEXT("patrolCarDoor");});
 if(!BodyPlace||!DoorPlace||!BodyPlace->Comp.IsValid()||!DoorPlace->Comp.IsValid())return false;
 UStaticMeshComponent* Car=BodyPlace->Comp.Get();UStaticMeshComponent* Leaf=DoorPlace->Comp.Get();
 FSwing* Swing=P->Swings.FindByPredicate([&](const FSwing& S){return S.Comp.Get()==Leaf;});
 if(!Swing)return false;
 Car->SetStaticMesh(Body);Car->EmptyOverrideMaterials();Car->SetRelativeTransform(FTransform::Identity);BodyPlace->Rest=FTransform::Identity;
 Leaf->SetStaticMesh(Door);Leaf->EmptyOverrideMaterials();
 Swing->Hinge=FVector::ZeroVector;Swing->Rest=FTransform(S47Three::ToUE(HingeThree));Swing->Deg=58;Swing->K=-1;
 Leaf->SetRelativeTransform(Swing->Rest);DoorPlace->Rest=Swing->Rest;
 // Existing story switch, rotating-beacon timing and component identities stay.
 // Only the physical fixtures follow the actual Tripo roof bar.
 for(FBeacon& B:P->Beacons)if(B.Car.Get()==Car)for(int32 Side=0;Side<2;++Side)
 {
  const FVector At=S47Three::ToUE(Side==0?-.46:.46,1.67,.21);
  if(auto* C=B.Lens[Side].Get()){C->SetRelativeLocation(At);C->SetRelativeScale3D(FVector(.44,.28,.10));}
  if(auto* L=B.Light[Side].Get())L->SetRelativeLocation(At+FVector(0,0,9));
 }
 UE_LOG(LogS47Props,Display,TEXT("S47: Tripo patrol body/source door retargeted; deputy proxies, door switch and beacon timing retained"));
 return true;
}

void AS47Props::AddSwing(UStaticMeshComponent* C, const TSharedPtr<FJsonObject>& J)
{
	FSwing S;
	S.Comp = C;
	S.Open = Web->Handle(Str(J, TEXT("open")));
	const TArray<TSharedPtr<FJsonValue>>* H = nullptr;
	if (J->TryGetArrayField(TEXT("hinge"), H) && H->Num() == 2) S.Hinge = S47Three::ToUE((*H)[0]->AsNumber(), 0, (*H)[1]->AsNumber());
	S.Deg = float(Num(J, TEXT("deg"), 55));
	S.Rest = C->GetRelativeTransform();
	S.K = -1; // (set on the first tick)
	if (S.Open < 0) UE_LOG(LogS47Props, Warning, TEXT("S47: props: %s swings on %s, which the export has not"), *C->GetName(), *Str(J, TEXT("open")));
	Swings.Add(S);
}

void AS47Props::AddLit(UStaticMeshComponent* C, int32 Node, const TSharedPtr<FJsonObject>& J)
{
	AS47Web* W = Web.Get();
	if (!W || !C) return;
	FLit L;
	L.Comp = C;
	L.Node = Node;
	// Some small shop/display practicals remain lit in daylight. Explicitly opt in;
	// every existing flood-controlled fixture keeps its original schedule.
	const bool bConstant = Bool(J, TEXT("constant"), false);
	L.Set = bConstant ? -1 : W->FloodSet(Str(J, TEXT("flood")));
	L.Slot = bConstant ? -1 : int32(Num(J, TEXT("slot"), -1));
	L.Gain = float(Num(J, TEXT("gain"), 1.0));
	if (Bool(J, TEXT("suppress"), false) && L.Set >= 0 && L.Slot >= 0) W->SuppressFloodLight(L.Set, L.Slot);
	// the emissive materials (glTF's EmissiveFactor, as Interchange made them), each its own instance
	const FString Flicker = Str(J, TEXT("flicker"));
	for (int32 I = 0; I < C->GetNumMaterials(); ++I)
	{
		UMaterialInterface* M = C->GetMaterial(I);
		FLinearColor F;
		if (!M || !M->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("EmissiveFactor")), F) || F.R + F.G + F.B < 0.001f) continue;
		FLit::FGlow G;
		G.Mid = C->CreateDynamicMaterialInstance(I, M);
		// Optional art tint changes a fixture's tube phosphor without changing its flood schedule.
		const TArray<TSharedPtr<FJsonValue>>* Tint = nullptr;
		if (J->TryGetArrayField(TEXT("emissive_tint"), Tint) && Tint->Num() == 3)
			F *= FLinearColor(float((*Tint)[0]->AsNumber()), float((*Tint)[1]->AsNumber()), float((*Tint)[2]->AsNumber()));
		G.Factor = F;
		G.bFlicker = !Flicker.IsEmpty() && M->GetName().Contains(Flicker);
		L.Glows.Add(G);
	}
	// the lights: rect sources in the model's own terms, attached to it
	const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
	if (J->TryGetArrayField(TEXT("lights"), A)) for (const TSharedPtr<FJsonValue>& V : *A)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		auto Vec = [&O](const TCHAR* K, const FVector& D) { const TArray<TSharedPtr<FJsonValue>>* X = nullptr; return O->TryGetArrayField(K, X) && X->Num() == 3 ? FVector((*X)[0]->AsNumber(), (*X)[1]->AsNumber(), (*X)[2]->AsNumber()) : D; };
		const FVector At = Vec(TEXT("at"), FVector::ZeroVector), Face = Vec(TEXT("face"), FVector(0, -1, 0)), Col = Vec(TEXT("color"), FVector(1, 1, 1));
		const TArray<TSharedPtr<FJsonValue>>* Sz = nullptr;
		const bool bSize = O->TryGetArrayField(TEXT("size"), Sz) && Sz->Num() == 2;
		const double Wd = bSize ? (*Sz)[0]->AsNumber() : 1.0, Ht = bSize ? (*Sz)[1]->AsNumber() : 1.0;
		URectLightComponent* R = NewObject<URectLightComponent>(C, *FString::Printf(TEXT("PropLit%d_%d"), Lits.Num(), L.Lamps.Num()));
		R->SetupAttachment(C);
		R->SetMobility(EComponentMobility::Movable);
		R->SetRelativeLocation(S47Three::ToUE(At));
		R->SetRelativeRotation(S47Three::DirToUE(Face).GetSafeNormal().Rotation());
		R->SetSourceWidth(float(Wd * 100.0));
		R->SetSourceHeight(float(Ht * 100.0));
		R->SetBarnDoorAngle(float(Num(O, TEXT("barn"), 88.0)));
		R->SetBarnDoorLength(float(Num(O, TEXT("barnLength"), 10.0)));
		R->SetIntensityUnits(ELightUnits::Lumens);
		R->SetIntensity(0.f);
		R->SetLightColor(FLinearColor(float(Col.X), float(Col.Y), float(Col.Z)));
		R->SetAttenuationRadius(float(Num(O, TEXT("reach"), 12.0) * 100.0));
		R->SetCastShadows(Bool(O, TEXT("shadow"), false));
		// (a broad source low over long flat walls: no acne where its light grazes them)
		R->SetShadowBias(float(Num(O, TEXT("bias"), 0.6)));
		R->SetShadowSlopeBias(float(Num(O, TEXT("slopeBias"), 0.8)));
		R->SetVolumetricScatteringIntensity(float(Num(O, TEXT("haze"), 0.4)));
		R->SetIndirectLightingIntensity(1.f);
		R->SetVisibility(false);
		R->RegisterComponent();
		FLit::FLamp Lamp;
		Lamp.Light = R;
		Lamp.Lumens = float(Num(O, TEXT("lumens"), 500.0));
		Lamp.Flicker = float(Num(O, TEXT("flicker"), 0.0));
		L.Lamps.Add(Lamp);
	}
	UE_LOG(LogS47Props, Display, TEXT("S47: props: %s lit by flood %d slot %d: %d glowing materials, %d lights"), *C->GetName(), L.Set, L.Slot, L.Glows.Num(), L.Lamps.Num());
	Lits.Add(L);
}

void AS47Props::AddDoor(UStaticMeshComponent* C, const TSharedPtr<FJsonObject>& J, const FString& Walk, const FTransform& T)
{
	AS47Web* W = Web.Get();
	if (!W || !C) return;
	FDoor D;
	D.Comp = C;
	D.Use = Str(J, TEXT("use"));
	D.Walk = Walk;
	const TArray<TSharedPtr<FJsonValue>>* H = nullptr;
	if (J->TryGetArrayField(TEXT("hinge"), H) && H->Num() == 2) D.Hinge = S47Three::ToUE((*H)[0]->AsNumber(), 0, (*H)[1]->AsNumber());
	D.Deg = float(Num(J, TEXT("deg"), 95));
	D.Rest = C->GetRelativeTransform();
	// the closed leaf's collider in the walk (and the drive: a car does not drive through a door either)
	const TArray<TSharedPtr<FJsonValue>>* Q = nullptr;
	if (J->TryGetArrayField(TEXT("block"), Q) && Q->Num() == 4)
	{
		FBox Box(ForceInit);
		for (const double X : { (*Q)[0]->AsNumber(), (*Q)[1]->AsNumber() }) for (const double Z : { (*Q)[2]->AsNumber(), (*Q)[3]->AsNumber() })
			Box += T.TransformPosition(S47Three::ToUE(X, 0.0, Z));
		D.Closed.MinX = Box.Min.X / 100.0; D.Closed.MaxX = Box.Max.X / 100.0; D.Closed.MinZ = Box.Min.Y / 100.0; D.Closed.MaxZ = Box.Max.Y / 100.0;
		D.Closed.Id = TEXT("door:") + D.Use;
		W->CollidersMutable(Walk).Add(D.Closed);
	}
	Doors.Add(D);
}

bool AS47Props::UseDoor(const FString& Use)
{
	// a door of a prop's (S47Caliche: the gas station's front door): opened or shut, its collider with it
	AS47Props* P = Instance.Get();
	if (!P) return false;
	for (FDoor& D : P->Doors)
	{
		if (D.Use != Use || !D.Comp.IsValid()) continue;
		D.bOpen = !D.bOpen;
		if (P->Web.IsValid())
		{
			for (FS47Rect& R : P->Web->CollidersMutable(D.Walk))
			{
				if (R.Id != D.Closed.Id) continue;
				if (D.bOpen) R.MinX = R.MaxX = R.MinZ = R.MaxZ = 1e9;
				else R = D.Closed;
			}
		}
		if (US47Main* M = US47Main::Get(P))
		{
			if (M->Audio) M->Audio->Sfx(M->Audio->Or(TEXT("door_latch"), TEXT("switch")), FS47Sound());
		}
		UE_LOG(LogS47Props, Display, TEXT("S47: props: door %s %s"), *Use, D.bOpen ? TEXT("opened") : TEXT("shut"));
		return true;
	}
	return false;
}

bool AS47Props::IsDoorOpen(const FString& Use)
{
	if (const AS47Props* P = Instance.Get()) for (const FDoor& D : P->Doors) if (D.Use == Use) return D.bOpen;
	return false;
}

void AS47Props::ApplyTrim(const FString& Model)
{
	// trim_web.py's meshes in place of the exported ones of the same name under the group, where the
	// exported mesh is still the one it was cut from (as many triangles): their materials stay
	AS47Web* W = Web.Get();
	const TSharedPtr<FJsonObject>* M = nullptr;
	if (!W || !Trims.IsValid() || !Trims->TryGetObjectField(Model, M)) { UE_LOG(LogS47Props, Warning, TEXT("S47: props: no trim of %s in free_models.json"), *Model); return; }
	const int32 Group = W->Handle(Str(*M, TEXT("group")));
	const TSharedPtr<FJsonObject>* Meshes = nullptr;
	if (Group < 0 || !(*M)->TryGetObjectField(TEXT("meshes"), Meshes)) return;
	int32 Swapped = 0, Hidden = 0, Stale = 0;
	for (int32 I = 0; I < W->PartCount(); ++I)
	{
		const FS47WebNode* N = W->Node(I);
		if (!N || N->Parent != Group || N->Geo < 0) continue;
		const TSharedPtr<FJsonObject>* G = nullptr;
		if (!(*Meshes)->TryGetObjectField(FString::Printf(TEXT("g%d"), N->Geo), G)) continue;
		UStaticMeshComponent* C = Cast<UStaticMeshComponent>(N->Comp.Get());
		UStaticMesh* Old = C ? C->GetStaticMesh() : nullptr;
		const int32 Tris = Old && Old->GetRenderData() && Old->GetRenderData()->LODResources.Num() ? Old->GetRenderData()->LODResources[0].GetNumTriangles() : -1;
		if (!C || Tris != int32(Num(*G, TEXT("tris"), -2))) { ++Stale; continue; }
		if (Bool(*G, TEXT("hidden"), false)) { C->SetHiddenInGame(true); C->SetCollisionEnabled(ECollisionEnabled::NoCollision); ++Hidden; continue; }
		if (UStaticMesh* Cut = LoadObject<UStaticMesh>(nullptr, *Str(*G, TEXT("asset")))) { C->SetStaticMesh(Cut); ++Swapped; }
		else ++Stale;
	}
	UE_LOG(LogS47Props, Display, TEXT("S47: props: %s cut out of the export: %d meshes trimmed, %d hidden%s"), *Model, Swapped, Hidden, Stale ? *FString::Printf(TEXT(", %d STALE (a new export needs Tools/freemodels/import_free.sh again)"), Stale) : TEXT(""));
}

double AS47Props::FloorAt(double X, double Z)
{
	for (const FFloor& F : Floors)
	{
		if (X < F.MinX || X > F.MaxX || Z < F.MinZ || Z > F.MaxZ) continue;
		const double K = F.Axis == TEXT("z") ? (Z - F.MinZ) / FMath::Max(F.MaxZ - F.MinZ, 1e-6) : (X - F.MinX) / FMath::Max(F.MaxX - F.MinX, 1e-6);
		return F.Y0 + (F.Y1 - F.Y0) * K;
	}
	return std::numeric_limits<double>::quiet_NaN();
}

float AS47Props::TubeAt(double T)
{
	// a starter giving up: every eleven seconds 0.65 s of dark and lit by turns (14 a second, the
	// same each night), otherwise the 120 Hz hum seen as the faintest unsteadiness
	const double P = FMath::Fmod(T, 11.0);
	if (P < 0.65)
	{
		const double N = FMath::Frac(FMath::Sin(FMath::FloorToDouble(T * 14.0) * 12.9898) * 43758.5453);
		return N > 0.45 ? 0.9f : 0.06f;
	}
	return float(0.985 + 0.015 * FMath::Sin(T * 60.0));
}

void AS47Props::UpdateLit(FLit& L, float Dt)
{
	AS47Web* W = Web.Get();
	UStaticMeshComponent* C = L.Comp.Get();
	if (!W || !C) return;
	const bool bShown = L.Node >= 0 ? Shown(L.Node) : C->IsVisible();
	// The photocell is a render mask, independent of the existing power/event slot.
	// Constant indoor practicals keep their explicit always-on behaviour.
	const float DayMask = L.Set < 0 || L.Slot < 0 ? 1.f : W->ExteriorDaylightMask(L.Set, L.Slot);
	const float K = bShown && (L.Set < 0 || L.Slot < 0 || W->FloodPool(L.Set, L.Slot).A > 0.001f) ? DayMask : 0.f;
	L.T += Dt;
	const float Tube = K > 0 ? TubeAt(L.T) : 1.f;
	const bool bChange = K != L.K;
	L.K = K;
	for (FLit::FGlow& G : L.Glows)
	{
		if (!G.Mid.IsValid() || (!bChange && !(G.bFlicker && K > 0))) continue;
		G.Mid->SetVectorParameterValue(TEXT("EmissiveFactor"), G.Factor * (K * L.Gain * (G.bFlicker ? Tube : 1.f)));
	}
	for (FLit::FLamp& P : L.Lamps)
	{
		URectLightComponent* R = P.Light.Get();
		if (!R) continue;
		if (bChange) R->SetVisibility(K > 0);
		if (bChange || (K > 0 && P.Flicker > 0)) R->SetIntensity(P.Lumens * K * (1.f - P.Flicker * (1.f - Tube)));
	}
}

void AS47Props::AddBeacon(UStaticMeshComponent* Car, const TSharedPtr<FJsonObject>& J)
{
	// the bar's glass as two lenses (cubes a little proud of it), red to the driver's side
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
	const TArray<TSharedPtr<FJsonValue>>* Bar = nullptr;
	if (!Cube || !Base || !J->TryGetArrayField(TEXT("bar"), Bar) || Bar->Num() != 6) { UE_LOG(LogS47Props, Warning, TEXT("S47: props: beacon without a bar, a cube or the base material")); return; }
	double V[6];
	for (int32 I = 0; I < 6; ++I) V[I] = (*Bar)[I]->AsNumber();
	FBeacon B;
	B.Car = Car;
	B.On = Web->Handle(Str(J, TEXT("on")));
	const FLinearColor Lens[2] = { FLinearColor(0.32f, 0.015f, 0.012f), FLinearColor(0.02f, 0.035f, 0.30f) };
	const FLinearColor Beam[2] = { FLinearColor(1.f, 0.06f, 0.03f), FLinearColor(0.12f, 0.28f, 1.f) };
	for (int32 H = 0; H < 2; ++H)
	{
		// the model's z runs to its right; three's z is Unreal's y (S47Three::ToUE)
		const double Z0 = H == 0 ? V[4] : 0.03, Z1 = H == 0 ? -0.03 : V[5];
		const FVector Lo = S47Three::ToUE(V[0], V[2], Z0), Hi = S47Three::ToUE(V[1], V[3], Z1);
		const FVector Mid = (Lo + Hi) * 0.5, Size = (Hi - Lo).GetAbs();
		UStaticMeshComponent* L = NewObject<UStaticMeshComponent>(Car, *FString::Printf(TEXT("PropBeaconLens%d"), H));
		L->SetStaticMesh(Cube);
		L->SetMobility(EComponentMobility::Movable);
		L->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		L->SetCastShadow(false);
		L->SetupAttachment(Car);
		L->SetRelativeLocation(Mid);
		L->SetRelativeScale3D(Size / 100.0);
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, this);
		M->SetVectorParameterValue(TEXT("Color"), Lens[H]);
		M->SetVectorParameterValue(TEXT("Emissive"), FLinearColor::Black);
		L->SetMaterial(0, M);
		L->RegisterComponent();
		B.Lens[H] = L;
		B.Mid[H] = M;
		// the beam: a small spot in the middle of the half, turning about the bar's up axis
		USpotLightComponent* S = NewObject<USpotLightComponent>(Car, *FString::Printf(TEXT("PropBeaconBeam%d"), H));
		S->SetupAttachment(Car);
		S->SetMobility(EComponentMobility::Movable);
		S->SetRelativeLocation(Mid + FVector(0, 0, 9)); // (above the bar: the car's own roof and trunk stay out of the beam)
		S->SetIntensityUnits(ELightUnits::Candelas);
		S->SetIntensity(0);
		S->SetLightColor(Beam[H]);
		S->SetInnerConeAngle(7.f);
		S->SetOuterConeAngle(17.f);
		S->SetAttenuationRadius(2600.f);
		S->SetSourceRadius(4.f);
		S->SetCastShadows(false);
		S->SetVolumetricScatteringIntensity(1.2f);
		S->SetIndirectLightingIntensity(0.3f);
		S->SetVisibility(false);
		S->RegisterComponent();
		B.Light[H] = S;
	}
	if (B.On < 0) UE_LOG(LogS47Props, Warning, TEXT("S47: props: the beacon's switch %s is not in the export"), *Str(J, TEXT("on")));
	Beacons.Add(B);
}

void AS47Props::BeaconAt(double T, float Lens[2], float Turn[2])
{
	// a rotating beacon of the early eighties: 1.5 turns a second, the halves half a turn apart; each
	// half has its third of a second, and in it the lens flashes twice (60 ms, 40 ms dark, 60 ms)
	constexpr double Rev = 1.5;
	const double Phase = FMath::Fmod(T * Rev, 1.0);
	const int32 Active = Phase < 0.5 ? 0 : 1;
	const double Local = Phase * 2 - Active;
	const bool bFlash = Local < 0.18 || (Local >= 0.30 && Local < 0.48);
	for (int32 H = 0; H < 2; ++H)
	{
		Lens[H] = H != Active ? 0.06f : bFlash ? 1.f : 0.3f;
		Turn[H] = float(FMath::Fmod(T * Rev * 360.0 + H * 180.0, 360.0));
	}
}

void AS47Props::UpdateBeacon(FBeacon& B, float Dt)
{
	UStaticMeshComponent* Car = B.Car.Get();
	const bool bLit = Car && B.On >= 0 && Shown(B.On) && Car->IsVisible();
	if (bLit != B.bLit)
	{
		B.bLit = bLit;
		for (int32 H = 0; H < 2; ++H)
		{
			if (B.Light[H].IsValid()) B.Light[H]->SetVisibility(bLit);
			if (B.Mid[H].IsValid() && !bLit) B.Mid[H]->SetVectorParameterValue(TEXT("Emissive"), FLinearColor::Black);
		}
	}
	for (int32 H = 0; H < 2; ++H) if (B.Lens[H].IsValid()) B.Lens[H]->SetVisibility(Car && Car->IsVisible());
	if (!bLit) return;
	B.T += Dt;
	float Lens[2], Turn[2];
	BeaconAt(B.T, Lens, Turn);
	// the lens's glow (red reads weaker than blue to the eye at night: a little more of it)
	const FLinearColor Glow[2] = { FLinearColor(1.f, 0.03f, 0.015f) * 34.f, FLinearColor(0.05f, 0.16f, 1.f) * 40.f };
	const float Candela[2] = { 900.f, 1100.f };
	for (int32 H = 0; H < 2; ++H)
	{
		if (B.Mid[H].IsValid()) B.Mid[H]->SetVectorParameterValue(TEXT("Emissive"), Glow[H] * Lens[H]);
		if (USpotLightComponent* S = B.Light[H].Get())
		{
			S->SetRelativeRotation(FRotator(-4.f, Turn[H], 0.f));
			S->SetIntensity(Candela[H] * (0.55f + 0.45f * Lens[H]));
		}
	}
}

void AS47Props::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AS47Web* W = Web.Get();
	if (!W) return;
	S47StationPhone::Update(W);
	S47StationWorldRoute::Update(W);
	S47TerrainStripTest::Update(W);
	for (FSwing& S : Swings)
	{
		UStaticMeshComponent* C = S.Comp.Get();
		if (!C) continue;
		const float K = S.Open >= 0 && Shown(S.Open) ? 1.f : 0.f;
		if (K == S.K) continue;
		S.K = K;
		// about the hinge's upright, in the model's own terms, then where the model stands
		const FTransform Turn = FTransform(-S.Hinge) * FTransform(FRotator(0, S.Deg * K, 0)) * FTransform(S.Hinge);
		C->SetRelativeTransform(Turn * S.Rest);
	}
	for (FBeacon& B : Beacons) UpdateBeacon(B, DeltaSeconds);
	for (FLit& L : Lits) UpdateLit(L, DeltaSeconds);
	for (FDoor& D : Doors)
	{
		UStaticMeshComponent* C = D.Comp.Get();
		if (!C) continue;
		const float Want = D.bOpen ? 1.f : 0.f;
		if (D.K == Want) continue;
		// about 0.7 s each way, eased
		D.K = FMath::FInterpConstantTo(D.K, Want, DeltaSeconds, 1.4f);
		const float E = D.K * D.K * (3 - 2 * D.K);
		const FTransform Turn = FTransform(-D.Hinge) * FTransform(FRotator(0, D.Deg * E, 0)) * FTransform(D.Hinge);
		C->SetRelativeTransform(Turn * D.Rest);
	}
	Tour(DeltaSeconds);
	for (FPlaced& P : Placed)
	{
		UStaticMeshComponent* C = P.Comp.Get();
		if (!C) continue;
		if (P.When >= 0)
		{
			bool bOn = true;
			for (int32 I = P.When; I >= 0; I = W->Node(I)->Parent) bOn &= W->IsVisible(I);
			C->SetHiddenInGame(!bOn);
		}
		if (P.bPaper)
		{
			// ControlRoom.ts printerPaper: shown while printed, scale.y the share out (SetPrinterPaper);
			// the DP-80's own sheet grows out of the platen the same share (its base is where it leaves)
			const FS47WebNode* N = W->Node(PaperNode);
			const bool bOut = N && N->bVis;
			const float K = bOut ? FMath::Clamp(float(N->S.Y), 0.001f, 1.f) : 0.f;
			C->SetHiddenInGame(!bOut);
			FTransform T = P.Rest;
			T.SetScale3D(P.Rest.GetScale3D() * FVector(1, 1, K));
			C->SetRelativeTransform(T);
		}
	}
}

// ---------- checks ----------

bool AS47Props::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	TSet<FString> Known;
	bool bManifest = false;
	if (const TSharedPtr<FJsonObject> Free = ReadJson(FPaths::ProjectContentDir() / TEXT("Free/free_models.json")))
	{
		bManifest = true;
		for (const TSharedPtr<FJsonValue>& V : Free->GetArrayField(TEXT("models")))
		{
			Known.Add(V->AsObject()->GetStringField(TEXT("name")));
			Known.Add(FPaths::GetBaseFilename(V->AsObject()->GetStringField(TEXT("asset"))));
		}
	}
	const TArray<FString> Files = PropFiles();
	int32 Props = 0;
	if (Files.IsEmpty()) { Lines.Add(TEXT("props: FAIL no Config/Props/*.json")); bOk = false; }
	for (const FString& F : Files)
	{
		const TSharedPtr<FJsonObject> J = ReadJson(PropsDir() / F);
		if (!J) { Lines.Add(FString::Printf(TEXT("props: FAIL %s does not read"), *F)); bOk = false; continue; }
		if (Str(J, TEXT("area")).IsEmpty()) { Lines.Add(FString::Printf(TEXT("props: FAIL %s has no area"), *F)); bOk = false; continue; }
		TSet<FString> Ids;
		// a model the manifest knows (or an engine shape by its path); no Joshua trees in New Mexico (MODELLER.md)
		auto CheckModel = [&](const FString& Id, const FString& Model)
		{
			if (Model.IsEmpty()) { Lines.Add(FString::Printf(TEXT("props: FAIL %s %s: no model"), *F, *Id)); bOk = false; return; }
			if (bManifest && !Model.StartsWith(TEXT("/")) && !Known.Contains(Model)) { Lines.Add(FString::Printf(TEXT("props: FAIL %s %s: %s is not in free_models.json"), *F, *Id, *Model)); bOk = false; }
			if (Model.Contains(TEXT("joshua"))) { Lines.Add(FString::Printf(TEXT("props: FAIL %s %s: no Joshua trees in New Mexico"), *F, *Id)); bOk = false; }
		};
		auto CheckId = [&](const FString& Id)
		{
			if (Ids.Contains(Id)) { Lines.Add(FString::Printf(TEXT("props: FAIL %s: %s twice"), *F, *Id)); bOk = false; }
			Ids.Add(Id);
			++Props;
		};
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		if (J->TryGetArrayField(TEXT("props"), A)) for (const TSharedPtr<FJsonValue>& V : *A)
		{
			const TSharedPtr<FJsonObject> P = V->AsObject();
			FVector At3;
			const FString Id = Str(P, TEXT("id"), Str(P, TEXT("model")));
			CheckModel(Id, Str(P, TEXT("model")));
			if (!At(P, At3)) { Lines.Add(FString::Printf(TEXT("props: FAIL %s %s: no place"), *F, *Id)); bOk = false; }
			const TSharedPtr<FJsonObject>* LitO = nullptr;
			if (P->TryGetObjectField(TEXT("lit"), LitO))
			{
				const TArray<TSharedPtr<FJsonValue>>* Ls = nullptr;
				bool bLit = Bool(*LitO, TEXT("constant"), false) || (!Str(*LitO, TEXT("flood")).IsEmpty() && Num(*LitO, TEXT("slot"), -1) >= 0);
				if ((*LitO)->TryGetArrayField(TEXT("lights"), Ls)) for (const TSharedPtr<FJsonValue>& LV : *Ls)
				{
					const TArray<TSharedPtr<FJsonValue>>* AtL = nullptr;
					const double Lm = Num(LV->AsObject(), TEXT("lumens"), 0);
					bLit &= LV->AsObject()->TryGetArrayField(TEXT("at"), AtL) && AtL->Num() == 3 && Lm > 0 && Lm < 20000;
				}
				if (!bLit) { Lines.Add(FString::Printf(TEXT("props: FAIL %s %s: lit needs explicit constant or flood/slot, and lights with a place and lumens"), *F, *Id)); bOk = false; }
			}
			const TArray<TSharedPtr<FJsonValue>>* Bl = nullptr;
			if (P->TryGetArrayField(TEXT("blocks"), Bl)) for (const TSharedPtr<FJsonValue>& BV : *Bl)
			{
				const TArray<TSharedPtr<FJsonValue>>& Q = BV->AsArray();
				if (Q.Num() != 4 || Q[1]->AsNumber() <= Q[0]->AsNumber() || Q[3]->AsNumber() <= Q[2]->AsNumber()) { Lines.Add(FString::Printf(TEXT("props: FAIL %s %s: a block is [x0, x1, z0, z1]"), *F, *Id)); bOk = false; }
			}
			const double S = Num(P, TEXT("scale"), 1);
			if (S <= 0.05 || S > 20) { Lines.Add(FString::Printf(TEXT("props: FAIL %s %s: scale %.2f"), *F, *Id, S)); bOk = false; }
			CheckId(Id);
		}
		if (J->TryGetArrayField(TEXT("rows"), A)) for (const TSharedPtr<FJsonValue>& V : *A)
		{
			const TSharedPtr<FJsonObject> R = V->AsObject();
			const FString Id = Str(R, TEXT("id"), Str(R, TEXT("model")));
			CheckModel(Id, Str(R, TEXT("model")));
			const TArray<TSharedPtr<FJsonValue>>* P0 = nullptr, *P1 = nullptr;
			if (!R->TryGetArrayField(TEXT("from"), P0) || !R->TryGetArrayField(TEXT("to"), P1) || P0->Num() != 2 || P1->Num() != 2 || Num(R, TEXT("step"), 0) <= 0.1) { Lines.Add(FString::Printf(TEXT("props: FAIL %s row %s: from, to and step"), *F, *Id)); bOk = false; }
			CheckId(Id);
		}
		if (J->TryGetArrayField(TEXT("scatter"), A)) for (const TSharedPtr<FJsonValue>& V : *A)
		{
			const TSharedPtr<FJsonObject> S = V->AsObject();
			const FString Id = Str(S, TEXT("id"), TEXT("scatter"));
			const TArray<FString> Models = Strs(S, TEXT("models"));
			if (Models.IsEmpty()) CheckModel(Id, FString());
			for (const FString& M : Models) CheckModel(Id, M);
			const TArray<TSharedPtr<FJsonValue>>* Box = nullptr;
			if (!S->TryGetArrayField(TEXT("rect"), Box) || Box->Num() != 4 || (*Box)[1]->AsNumber() <= (*Box)[0]->AsNumber() || (*Box)[3]->AsNumber() <= (*Box)[2]->AsNumber()) { Lines.Add(FString::Printf(TEXT("props: FAIL %s scatter %s: rect"), *F, *Id)); bOk = false; }
			CheckId(Id);
		}
	}
	Lines.Add(FString::Printf(TEXT("props: %d files, %d props%s %s"), Files.Num(), Props, bManifest ? TEXT("") : TEXT(" (no free_models.json to check the models against)"), bOk ? TEXT("OK") : TEXT("FAIL")));
	// the beacon: red flashes first, blue a third of a second later, the beams half a turn apart,
	// 1.5 turns a second, and between a half's two flashes a dark gap
	{
		float L0[2], T0[2], L1[2], T1[2], Lg[2], Tg[2], Lp[2], Tp[2];
		BeaconAt(0.01, L0, T0); BeaconAt(1.0 / 3.0 + 0.01, L1, T1); BeaconAt(1.0 / 1.5 + 0.01, Lp, Tp);
		BeaconAt(0.24 / 3.0, Lg, Tg); // (a half lasts 1/3 s: 24 % into red's is the gap)
		const bool bBeacon = L0[0] == 1.f && L0[1] < 0.1f && L1[1] == 1.f && L1[0] < 0.1f && Lg[0] < 1.f && Lg[0] > Lg[1]
			&& FMath::IsNearlyEqual(FMath::Fmod(T0[1] - T0[0] + 360.f, 360.f), 180.f, 0.01f) && FMath::IsNearlyEqual(Tp[0], T0[0], 0.01f) && Lp[0] == 1.f;
		Lines.Add(FString::Printf(TEXT("props: beacon red, blue, double flash and turn %s"), bBeacon ? TEXT("OK") : TEXT("FAIL")));
		bOk &= bBeacon;
	}
	// the failing tube: lit almost steadily, dark for a moment in each stutter, never brighter than lit
	{
		float Lo = 1.f, Hi = 0.f, Calm = 1.f;
		for (int32 I = 0; I < 1100; ++I) { const float V = TubeAt(I * 0.01); Lo = FMath::Min(Lo, V); Hi = FMath::Max(Hi, V); }
		for (int32 I = 70; I < 1100; ++I) Calm = FMath::Min(Calm, TubeAt(I * 0.01));
		const bool bTube = Lo < 0.1f && Hi <= 1.f && Calm > 0.96f;
		Lines.Add(FString::Printf(TEXT("props: failing tube stutters and hums %s"), bTube ? TEXT("OK") : TEXT("FAIL")));
		bOk &= bTube;
	}
	return bOk;
}

void AS47Props::TestPlace(US47Main* M)
{
	FString Arg;
	if (!FParse::Value(FCommandLine::Get(), TEXT("S47PropsAt="), Arg, false) || !M || !M->Web) return;
	TWeakObjectPtr<US47Main> Main = M;
	FTimerHandle H;
	M->Web->GetWorldTimerManager().SetTimer(H, [Main, Arg]
	{
		if (!Main.IsValid() || !Main->Player || !Main->World) return;
		FString Area, Rest;
		if (!Arg.Split(TEXT(":"), &Area, &Rest)) return;
		TArray<FString> V;
		Rest.ParseIntoArray(V, TEXT(";"));
		if (V.Num() < 3) return;
		Main->World->Enter(Area);
		Main->Player->Place(FCString::Atod(*V[0]), FCString::Atod(*V[1]), FCString::Atod(*V[2]));
		if (V.Num() >= 4) Main->Player->Pitch = FCString::Atod(*V[3]);
		UE_LOG(LogS47Props, Display, TEXT("S47: props view at %s"), *Arg);
	}, 1.5f, false);
}

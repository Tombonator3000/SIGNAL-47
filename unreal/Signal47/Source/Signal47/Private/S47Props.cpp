// Unreal only (departs from the web version): the free models placed in the world, see S47Props.h.
#include "S47Props.h"

#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "S47Main.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Props, Log, All);

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
	FTransform ThreeToUE(const FVector& P, double YawDeg, double Scale)
	{
		double E[16];
		S47Three::Compose(P, S47Three::Euler(0, FMath::DegreesToRadians(YawDeg), 0), FVector(Scale), E);
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
					TSet<int32>& Set = AreaNodes.FindOrAdd(KV.Key);
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
	if (!InWeb || !InWeb->IsLoaded()) return;
	LoadManifests();
	if (Assets.IsEmpty()) UE_LOG(LogS47Props, Warning, TEXT("S47: props: no Content/Free/free_models.json (Tools/freemodels/import_free.sh); only the trims and hiding apply"));
	ApplyTrims();
	PaperNode = InWeb->Handle(TEXT("room.printerPaper"));
	for (const FString& F : PropFiles())
	{
		const TSharedPtr<FJsonObject> File = ReadJson(PropsDir() / F);
		if (!File) { UE_LOG(LogS47Props, Error, TEXT("S47: props: %s does not read"), *F); continue; }
		BuildFile(File, F);
	}
	UE_LOG(LogS47Props, Display, TEXT("S47: props placed %d"), Placed.Num());
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
	if (FileParent < 0) if (const TSet<int32>* S = AreaNodes.Find(Area)) for (int32 I : *S) { FileParent = I; break; }
	int32 Count = 0, Blocks = 0;
	const TArray<TSharedPtr<FJsonValue>>* Props = nullptr;
	if (!File->TryGetArrayField(TEXT("props"), Props)) return;
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
		// on the ground (or the desk) under it
		if (Bool(P, TEXT("ground"), true) || FMath::IsNaN(Th.Y))
		{
			const double Lift = Num(P, TEXT("lift"), 0.6);
			const bool bHigh = FMath::IsNaN(Th.Y);
			double Y = 0;
			if (Ground(Th, bHigh ? 600.0 : Th.Y + Lift, bHigh ? -600.0 : Th.Y - 4.0, Areas, Hidden, Y)) Th.Y = Y - Num(P, TEXT("sink"), 0.0);
			else if (bHigh) { UE_LOG(LogS47Props, Warning, TEXT("S47: props: %s %s: no ground under %.1f %.1f"), *Name, *Id, Th.X, Th.Z); Th.Y = 0; }
		}
		const double Yaw = Num(P, TEXT("yaw"), 0), Scale = Num(P, TEXT("scale"), 1);
		const FTransform T = ThreeToUE(Th, Yaw, Scale);
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, *FString::Printf(TEXT("Prop_%s_%d"), *Id, Placed.Num()));
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Mesh);
		// the floor and sight see it; the use trace only when it stands for a thing to use
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		C->SetCollisionResponseToAllChannels(ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		if (!Hidden.IsEmpty()) C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		C->SetCastShadow(true);
		C->RegisterComponent();
		C->AttachToComponent(PN->Comp.IsValid() ? PN->Comp.Get() : W->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
		C->SetWorldTransform(T);
		FPlaced Pl;
		Pl.Id = Id;
		Pl.Comp = C;
		Pl.Node = W->AdoptNode(C, Parent, TEXT("prop:") + Id);
		Pl.When = W->Handle(Str(P, TEXT("when")));
		Pl.bPaper = Bool(P, TEXT("paper"), false);
		Pl.Rest = C->GetRelativeTransform();
		Placed.Add(Pl);
		++Count;
		// in the way: the walk's collider (and the drive's), the model's footprint turned as it stands
		if (Bool(P, TEXT("collide"), false))
		{
			const FBox B = Mesh->GetBoundingBox().TransformBy(T);
			const double Pad = Num(P, TEXT("pad"), 0.05);
			FS47Rect R;
			R.MinX = B.Min.X / 100.0 + Pad; R.MaxX = B.Max.X / 100.0 - Pad; // (Unreal x is three's x, Unreal y three's z)
			R.MinZ = B.Min.Y / 100.0 + Pad; R.MaxZ = B.Max.Y / 100.0 - Pad;
			R.Id = TEXT("prop:") + Id;
			if (R.MaxX > R.MinX && R.MaxZ > R.MinZ)
			{
				W->CollidersMutable(Walk).Add(R);
				++Blocks;
				// the drive's legs read their obstacles from the export (US47World ObstaclesOf): a box
				// [minX, maxX, minZ, maxZ]; SARO's leg takes the walk's colliders as they are
				const TSharedPtr<FJsonObject>& D = W->DriveData();
				const TSharedPtr<FJsonObject>* Legs = nullptr, *L = nullptr;
				const TArray<TSharedPtr<FJsonValue>>* Obs = nullptr;
				if (!Leg.IsEmpty() && D.IsValid() && D->TryGetObjectField(TEXT("legs"), Legs) && (*Legs)->TryGetObjectField(Leg, L) && (*L)->TryGetArrayField(TEXT("obstacles"), Obs))
				{
					TArray<TSharedPtr<FJsonValue>> Box = { MakeShared<FJsonValueNumber>(R.MinX), MakeShared<FJsonValueNumber>(R.MaxX), MakeShared<FJsonValueNumber>(R.MinZ), MakeShared<FJsonValueNumber>(R.MaxZ) };
					TArray<TSharedPtr<FJsonValue>> All = *Obs;
					All.Add(MakeShared<FJsonValueArray>(Box));
					(*L)->SetArrayField(TEXT("obstacles"), All);
				}
			}
		}
	}
	UE_LOG(LogS47Props, Display, TEXT("S47: props %s: %d placed, %d in the way"), *Name, Count, Blocks);
}

void AS47Props::ApplyTrims()
{
	// Content/Free/Trim/trim.json (Tools/props/trim.py): exported meshes with the triangles of what a
	// prop replaces cut out, by geometry id; used only when the mesh they were cut from is the one here
	const TSharedPtr<FJsonObject> J = ReadJson(FPaths::ProjectContentDir() / TEXT("Free/Trim/trim.json"));
	if (!J) return;
	AS47Web* W = Web.Get();
	int32 Done = 0;
	for (const auto& KV : J->Values)
	{
		const TSharedPtr<FJsonObject> O = KV.Value->AsObject();
		const int32 Geo = FCString::Atoi(*KV.Key.Mid(1));
		UStaticMesh* Cut = LoadObject<UStaticMesh>(nullptr, *Str(O, TEXT("asset")));
		const int32 Tris = int32(Num(O, TEXT("tris"), -1));
		for (int32 I = 0; I < W->PartCount(); ++I)
		{
			const FS47WebNode* N = W->Node(I);
			UStaticMeshComponent* C = N && N->Geo == Geo ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
			if (!C || !Cut) continue;
			if (C->GetStaticMesh() && C->GetStaticMesh()->GetNumTriangles(0) != Tris)
			{
				UE_LOG(LogS47Props, Warning, TEXT("S47: props: trim g%d is for a mesh of %d triangles, this one has %d (a newer export: run Tools/props/trim.sh)"), Geo, Tris, C->GetStaticMesh()->GetNumTriangles(0));
				continue;
			}
			// the component keeps its own materials (slot by slot, AS47Web::Build)
			TArray<UMaterialInterface*> Mats;
			for (int32 k = 0; k < C->GetNumMaterials(); ++k) Mats.Add(C->GetMaterial(k));
			C->SetStaticMesh(Cut);
			for (int32 k = 0; k < Mats.Num(); ++k) C->SetMaterial(k, Mats[k]);
			++Done;
		}
	}
	UE_LOG(LogS47Props, Display, TEXT("S47: props: %d exported meshes trimmed"), Done);
}

void AS47Props::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AS47Web* W = Web.Get();
	if (!W) return;
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
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		if (!J->TryGetArrayField(TEXT("props"), A) || Str(J, TEXT("area")).IsEmpty()) { Lines.Add(FString::Printf(TEXT("props: FAIL %s has no area or props"), *F)); bOk = false; continue; }
		TSet<FString> Ids;
		for (const TSharedPtr<FJsonValue>& V : *A)
		{
			const TSharedPtr<FJsonObject> P = V->AsObject();
			FVector At3;
			const FString Id = Str(P, TEXT("id"), Str(P, TEXT("model")));
			const FString Model = Str(P, TEXT("model"));
			if (Model.IsEmpty() || !At(P, At3)) { Lines.Add(FString::Printf(TEXT("props: FAIL %s %s: no model or no place"), *F, *Id)); bOk = false; }
			else if (bManifest && !Known.Contains(Model)) { Lines.Add(FString::Printf(TEXT("props: FAIL %s %s: %s is not in free_models.json"), *F, *Id, *Model)); bOk = false; }
			const double S = Num(P, TEXT("scale"), 1);
			if (S <= 0.05 || S > 20) { Lines.Add(FString::Printf(TEXT("props: FAIL %s %s: scale %.2f"), *F, *Id, S)); bOk = false; }
			// (no Joshua trees in New Mexico: MODELLER.md)
			if (Model.Contains(TEXT("joshua"))) { Lines.Add(FString::Printf(TEXT("props: FAIL %s %s: no Joshua trees in New Mexico"), *F, *Id)); bOk = false; }
			Ids.Add(Id);
			++Props;
		}
	}
	Lines.Add(FString::Printf(TEXT("props: %d files, %d props%s %s"), Files.Num(), Props, bManifest ? TEXT("") : TEXT(" (no free_models.json to check the models against)"), bOk ? TEXT("OK") : TEXT("FAIL")));
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

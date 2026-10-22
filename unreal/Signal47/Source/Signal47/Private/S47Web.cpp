#include "S47Web.h"

#include "Components/BillboardComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "S47Three.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Web, Log, All);

using namespace S47Three;

namespace
{
	constexpr double DEG = PI / 180.0;
	constexpr double SWING = 0.9; // Doors.ts

	double Num(const TSharedPtr<FJsonObject>& O, const TCHAR* K, double D = 0) { double V = D; O->TryGetNumberField(K, V); return V; }
	FString Str(const TSharedPtr<FJsonObject>& O, const TCHAR* K) { FString V; O->TryGetStringField(K, V); return V; }
	bool Bool(const TSharedPtr<FJsonObject>& O, const TCHAR* K, bool D = false) { bool V = D; O->TryGetBoolField(K, V); return V; }
	const TArray<TSharedPtr<FJsonValue>>& Arr(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		static const TArray<TSharedPtr<FJsonValue>> Empty;
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		return O->TryGetArrayField(K, A) ? *A : Empty;
	}
	FLinearColor Col(const TArray<TSharedPtr<FJsonValue>>& A) { return A.Num() >= 3 ? FLinearColor(A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber()) : FLinearColor::White; }
	FS47Rect ToRect(const TSharedPtr<FJsonObject>& O)
	{
		FS47Rect R;
		R.MinX = Num(O, TEXT("minX")); R.MaxX = Num(O, TEXT("maxX")); R.MinZ = Num(O, TEXT("minZ")); R.MaxZ = Num(O, TEXT("maxZ"));
		R.Id = Str(O, TEXT("id"));
		R.bEnabled = Bool(O, TEXT("enabled"), true);
		return R;
	}
	void Mat16(const TArray<TSharedPtr<FJsonValue>>& A, double* Out)
	{
		for (int32 k = 0; k < 16; ++k) Out[k] = A.IsValidIndex(k) ? A[k]->AsNumber() : (k % 5 == 0 ? 1.0 : 0.0);
	}
}

AS47Web::AS47Web()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

FQuat AS47Web::S47ThreeEuler(double X, double Y, double Z) { return Euler(X, Y, Z); }

FString AS47Web::Summary() const
{
	int32 Meshes = 0;
	for (const FS47WebNode& N : Nodes) if (N.Geo >= 0) ++Meshes;
	return FString::Printf(TEXT("%d nodes, %d meshes, %d lights, %d items, %d doors, %d dishes"), Nodes.Num(), Meshes, Lights.Num(), ItemList.Num(), Doors.Num(), Dishes.Num());
}

UMaterialInterface* AS47Web::Mat(int32 Id)
{
	if (Id < 0) return nullptr;
	if (MatCache.Num() <= Id) { MatCache.SetNum(Id + 1); MatLoaded.SetNum(Id + 1); }
	if (!MatLoaded[Id])
	{
		MatLoaded[Id] = true;
		MatCache[Id] = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Web/Mat/MI_W%d.MI_W%d"), Id, Id));
	}
	return MatCache[Id];
}

int32 AS47Web::Handle(const FString& Key) const { const int32* I = Handles.Find(Key); return I ? *I : -1; }
int32 AS47Web::HandleMat(const FString& Key) const { const int32* I = HandleMats.Find(Key); return I ? *I : -1; }
int32 AS47Web::HandleTex(const FString& Key) const { const int32* I = HandleTexs.Find(Key); return I ? *I : -1; }

// ---------- loading ----------

bool AS47Web::Load()
{
	const FString Path = FPaths::ProjectContentDir() / TEXT("WebData/manifest.json");
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		UE_LOG(LogS47Web, Error, TEXT("S47: no web manifest at %s (run Tools/webexport/import.sh)"), *Path);
		return false;
	}
	TSharedPtr<FJsonObject> J;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) || !J.IsValid())
	{
		UE_LOG(LogS47Web, Error, TEXT("S47: the web manifest is not valid JSON"));
		return false;
	}
	// geometry files: which glb holds which geometry
	int32 MaxGeo = 0;
	for (const TSharedPtr<FJsonValue>& V : Arr(J, TEXT("geoFiles"))) MaxGeo = FMath::Max(MaxGeo, int32(Num(V->AsObject(), TEXT("to"))));
	GeoPaths.SetNum(MaxGeo);
	for (const TSharedPtr<FJsonValue>& V : Arr(J, TEXT("geoFiles")))
	{
		const TSharedPtr<FJsonObject> F = V->AsObject();
		const FString Folder = FPaths::GetBaseFilename(Str(F, TEXT("file")));
		for (int32 g = int32(Num(F, TEXT("from"))); g < int32(Num(F, TEXT("to"))); ++g)
		{
			GeoPaths[g] = FString::Printf(TEXT("/Game/Web/Geo/%s/StaticMeshes/g%d.g%d"), *Folder, g, g);
		}
	}
	// materials that must not be drawn as they are (shaders we cannot rebuild, hidden proxies)
	TSet<int32> ShaderMats, HiddenMats;
	TMap<int32, int32> MatFlood;
	TMap<int32, float> MatFall;
	for (const TSharedPtr<FJsonValue>& V : Arr(J, TEXT("materials")))
	{
		const TSharedPtr<FJsonObject> M = V->AsObject();
		const int32 Id = int32(Num(M, TEXT("id")));
		if (Bool(M, TEXT("shader"))) ShaderMats.Add(Id);
		if (!Bool(M, TEXT("visible"), true)) HiddenMats.Add(Id);
		const int32 Fs = int32(Num(M, TEXT("floodSet"), -1));
		if (Fs >= 0) { MatFlood.Add(Id, Fs); MatFall.Add(Id, float(Num(M, TEXT("floodFall"), 0.012))); }
	}
	// nodes
	const TArray<TSharedPtr<FJsonValue>>& JN = Arr(J, TEXT("nodes"));
	Nodes.SetNum(JN.Num());
	TArray<TSharedPtr<FJsonObject>> Raw;
	Raw.SetNum(JN.Num());
	for (int32 i = 0; i < JN.Num(); ++i)
	{
		const TSharedPtr<FJsonObject> O = JN[i]->AsObject();
		Raw[i] = O;
		FS47WebNode& N = Nodes[i];
		N.Parent = int32(Num(O, TEXT("p"), -1));
		N.Name = Str(O, TEXT("name"));
		Mat16(Arr(O, TEXT("m")), N.Local);
		Decompose(N.Local, N.P, N.Q, N.S);
		N.bVis = Bool(O, TEXT("vis"), true);
		N.Geo = int32(Num(O, TEXT("geo"), -1));
		for (const TSharedPtr<FJsonValue>& M : Arr(O, TEXT("mats"))) N.Mats.Add(int32(M->AsNumber()));
		N.bInstanced = O->HasField(TEXT("inst"));
		N.Root = N.Parent < 0 ? i : Nodes[N.Parent].Root;
		if (N.Parent >= 0) Nodes[N.Parent].Children.Add(i);
	}
	// handles
	const TSharedPtr<FJsonObject>* H = nullptr;
	if (J->TryGetObjectField(TEXT("handles"), H))
	{
		for (const auto& KV : (*H)->Values)
		{
			const TSharedPtr<FJsonObject> O = KV.Value->AsObject();
			if (!O) continue;
			if (O->HasField(TEXT("node"))) Handles.Add(FString(KV.Key.ToView()), int32(Num(O, TEXT("node"))));
			if (O->HasField(TEXT("mat"))) HandleMats.Add(FString(KV.Key.ToView()), int32(Num(O, TEXT("mat"))));
			if (O->HasField(TEXT("tex"))) HandleTexs.Add(FString(KV.Key.ToView()), int32(Num(O, TEXT("tex"))));
			if (O->HasField(TEXT("light"))) HandleLights.Add(FString(KV.Key.ToView()), Str(O, TEXT("light")));
			const TArray<TSharedPtr<FJsonValue>>& Vv = Arr(O, TEXT("vec"));
			if (Vv.Num() == 3) HandleVecs.Add(FString(KV.Key.ToView()), FVector(Vv[0]->AsNumber(), Vv[1]->AsNumber(), Vv[2]->AsNumber()));
			const TArray<TSharedPtr<FJsonValue>>& Rv = Arr(O, TEXT("rect"));
			if (Rv.Num() == 4)
			{
				FS47Area R;
				R.MinX = Rv[0]->AsNumber(); R.MaxX = Rv[1]->AsNumber(); R.MinZ = Rv[2]->AsNumber(); R.MaxZ = Rv[3]->AsNumber(); R.bValid = true;
				HandleRects.Add(FString(KV.Key.ToView()), R);
			}
		}
	}
	// areas
	const TSharedPtr<FJsonObject>* A = nullptr;
	if (J->TryGetObjectField(TEXT("areas"), A))
	{
		for (const auto& KV : (*A)->Values)
		{
			for (const TSharedPtr<FJsonValue>& V : KV.Value->AsArray())
			{
				const int32 R = int32(V->AsNumber());
				AreaRoots.FindOrAdd(FString(KV.Key.ToView())).Add(R);
				RootArea.Add(Nodes.IsValidIndex(R) ? Nodes[R].Root : R, FString(KV.Key.ToView()));
			}
		}
	}
	// walks
	const TSharedPtr<FJsonObject>* W = nullptr;
	if (J->TryGetObjectField(TEXT("walks"), W))
	{
		for (const auto& KV : (*W)->Values)
		{
			const TSharedPtr<FJsonObject> O = KV.Value->AsObject();
			auto& P = Walks.FindOrAdd(FString(KV.Key.ToView()));
			for (const TSharedPtr<FJsonValue>& V : Arr(O, TEXT("zones"))) P.Key.Add(ToRect(V->AsObject()));
			for (const TSharedPtr<FJsonValue>& V : Arr(O, TEXT("colliders"))) P.Value.Add(ToRect(V->AsObject()));
		}
	}
	// items
	for (const TSharedPtr<FJsonValue>& V : Arr(J, TEXT("items")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FS47WebItem It;
		It.Id = Str(O, TEXT("id")); It.Node = int32(Num(O, TEXT("node"), -1)); It.Range = Num(O, TEXT("range"), 2.4); It.bLook = Bool(O, TEXT("look"));
		const TArray<TSharedPtr<FJsonValue>>& Mn = Arr(O, TEXT("min"));
		const TArray<TSharedPtr<FJsonValue>>& Mx = Arr(O, TEXT("max"));
		if (Mn.Num() == 3 && Mx.Num() == 3) { It.Min = FVector(Mn[0]->AsNumber(), Mn[1]->AsNumber(), Mn[2]->AsNumber()); It.Max = FVector(Mx[0]->AsNumber(), Mx[1]->AsNumber(), Mx[2]->AsNumber()); It.bBox = true; }
		const int32 Idx = ItemList.Add(It);
		if (It.Node >= 0) NodeItem.Add(It.Node, Idx);
	}
	// dishes
	for (const TSharedPtr<FJsonValue>& V : Arr(J, TEXT("dishes")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FDish D;
		D.Id = Str(O, TEXT("id")); D.X = Num(O, TEXT("x")); D.Z = Num(O, TEXT("z")); D.Scale = Num(O, TEXT("scale"), 1);
		D.CurAz = D.TgtAz = Num(O, TEXT("az"), 18); D.CurEl = D.TgtEl = Num(O, TEXT("el"), 48);
		Dishes.Add(D);
	}
	for (const TSharedPtr<FJsonValue>& V : Arr(J, TEXT("dishParts")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FDishPart P;
		P.Node = int32(Num(O, TEXT("node"), -1)); P.Level = int32(Num(O, TEXT("level")));
		for (const TSharedPtr<FJsonValue>& D : Arr(O, TEXT("dishes"))) P.DishIdx.Add(int32(D->AsNumber()));
		DishParts.Add(P);
	}
	// doors
	for (const TSharedPtr<FJsonValue>& V : Arr(J, TEXT("doors")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FDoor D;
		D.Id = Str(O, TEXT("id")); D.Inter = Str(O, TEXT("inter")); D.Name = Str(O, TEXT("name")); D.Area = Str(O, TEXT("area")); D.Sound = Str(O, TEXT("sound"));
		D.Range = Num(O, TEXT("range"), 2.4); D.Proxy = int32(Num(O, TEXT("proxy"), -1));
		for (const TSharedPtr<FJsonValue>& M : Arr(O, TEXT("moved")))
		{
			const TSharedPtr<FJsonObject> MO = M->AsObject();
			D.MovedNodes.Add(int32(Num(MO, TEXT("node"))));
			TArray<double> Sh, Op;
			for (const TSharedPtr<FJsonValue>& X : Arr(MO, TEXT("shut"))) Sh.Add(X->AsNumber());
			for (const TSharedPtr<FJsonValue>& X : Arr(MO, TEXT("open"))) Op.Add(X->AsNumber());
			D.Shut.Add(Sh); D.Open.Add(Op);
		}
		for (const TSharedPtr<FJsonValue>& Z : Arr(O, TEXT("zones"))) D.Zones.Add(int32(Z->AsNumber()));
		const TSharedPtr<FJsonObject>* L = nullptr;
		if (O->TryGetObjectField(TEXT("leaf"), L)) D.Leaf = ToRect(*L);
		Doors.Add(D);
	}
	const TSharedPtr<FJsonObject>* Sp = nullptr;
	if (J->TryGetObjectField(TEXT("spawn"), Sp)) Spawn = FVector(Num(*Sp, TEXT("x")), Num(*Sp, TEXT("yaw")), Num(*Sp, TEXT("z")));
	ToneMappingExposure = Num(J, TEXT("toneMappingExposure"), 1.05);
	const TSharedPtr<FJsonObject>* Fg = nullptr;
	if (J->TryGetObjectField(TEXT("fog"), Fg)) { FogColor = Col(Arr(*Fg, TEXT("color"))); FogDensity = float(Num(*Fg, TEXT("density"), 0.0021)); }
	// skies
	const TSharedPtr<FJsonObject>* Sk = nullptr;
	if (J->TryGetObjectField(TEXT("skies"), Sk)) for (const auto& KV : (*Sk)->Values) { const FString K(KV.Key.ToView()); SkyTex.Add(K, FString::Printf(TEXT("/Game/Web/Tex/Sky_%s.Sky_%s"), *K, *K)); }

	// the fake floodlights (kit.ts floodlit): each set's pools in a small float texture, row 0
	// position and reach, row 1 colour; every material lit by a set reads it in its emissive.
	// Every slot is there, the unused ones too, so a lamp is switched by its index (setFlood).
	{
		for (const TSharedPtr<FJsonValue>& V : Arr(J, TEXT("floods")))
		{
			const TSharedPtr<FJsonObject> F = V->AsObject();
			const TArray<TSharedPtr<FJsonValue>>& Pools = Arr(F, TEXT("pools"));
			FFloodSet Set;
			Set.Key = Str(F, TEXT("key"));
			Set.Scale = float(Num(F, TEXT("scale"), 0.07));
			Set.W = FMath::Clamp(Pools.Num(), 1, 32);
			for (int32 k = 0; k < Set.W; ++k)
			{
				const TSharedPtr<FJsonObject> P = Pools.IsValidIndex(k) ? Pools[k]->AsObject() : nullptr;
				Set.Pos.Add(P ? FLinearColor(Num(P, TEXT("x")), Num(P, TEXT("y")), Num(P, TEXT("z")), Num(P, TEXT("r"))) : FLinearColor(0, -999, 0, 0));
				Set.Col.Add(P ? Col(Arr(P, TEXT("col"))) : FLinearColor::Black);
			}
			UTexture2D* T = UTexture2D::CreateTransient(Set.W, 2, PF_A32B32G32R32F);
			T->SRGB = false;
			T->Filter = TF_Nearest;
			T->AddressX = TA_Clamp; T->AddressY = TA_Clamp;
			FloodTex.Add(T);
			FloodSets.Add(Set);
			UploadFlood(FloodSets.Num() - 1);
		}
		int32 Lit = 0;
		for (const auto& KV : MatFlood)
		{
			if (!FloodSets.IsValidIndex(KV.Value)) continue;
			UMaterialInterface* Base = Mat(KV.Key);
			if (!Base) continue;
			const FFloodSet& Set = FloodSets[KV.Value];
			UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Base, this);
			D->SetTextureParameterValue(TEXT("FloodTex"), FloodTex[KV.Value]);
			D->SetScalarParameterValue(TEXT("FloodCount"), float(Set.W));
			D->SetScalarParameterValue(TEXT("FloodScale"), Set.Scale);
			D->SetScalarParameterValue(TEXT("FloodFall"), MatFall.FindRef(KV.Key));
			MatCache[KV.Key] = D;
			FloodMids.FindOrAdd(KV.Value).Add(D);
			++Lit;
		}
		MatFloodSet = MatFlood;
		UE_LOG(LogS47Web, Display, TEXT("S47: %d flood sets, %d materials lit by them"), FloodSets.Num(), Lit);
	}

	// ---------- build the components ----------
	USceneComponent* Root = GetRootComponent();
	UMaterialInterface* GlowBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebGlow.M_WebGlow"));
	const int32 SkyRoot = Handle(TEXT("sky.group")), BackRoot = Handle(TEXT("backdrop.group"));
	// which item each node is part of (an item's object and everything under it)
	TArray<int32> ItemOfNode;
	ItemOfNode.Init(-1, Nodes.Num());
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		if (const int32* It = NodeItem.Find(i)) ItemOfNode[i] = *It;
		else if (Nodes[i].Parent >= 0) ItemOfNode[i] = ItemOfNode[Nodes[i].Parent];
	}
	int32 Built = 0, Missing = 0;
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		FS47WebNode& N = Nodes[i];
		const TSharedPtr<FJsonObject>& O = Raw[i];
		USceneComponent* Parent = N.Parent >= 0 && Nodes[N.Parent].Comp.IsValid() ? Nodes[N.Parent].Comp.Get() : Root;
		USceneComponent* C = nullptr;
		const bool bSky = N.Root == SkyRoot || N.Root == BackRoot;
		if (N.Geo >= 0 && GeoPaths.IsValidIndex(N.Geo) && !bSky)
		{
			UStaticMesh* SM = LoadObject<UStaticMesh>(nullptr, *GeoPaths[N.Geo]);
			if (!SM) { ++Missing; }
			UStaticMeshComponent* M = nullptr;
			if (N.bInstanced)
			{
				UInstancedStaticMeshComponent* I = NewObject<UInstancedStaticMeshComponent>(this, *FString::Printf(TEXT("N%d"), i));
				M = I;
			}
			else M = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("N%d"), i));
			M->SetMobility(EComponentMobility::Movable);
			M->SetupAttachment(Parent);
			M->SetRelativeTransform(MatToUE(N.Local));
			if (SM) M->SetStaticMesh(SM);
			bool bShader = false, bHidden = false;
			for (int32 k = 0; k < N.Mats.Num(); ++k)
			{
				if (ShaderMats.Contains(N.Mats[k])) bShader = true;
				if (HiddenMats.Contains(N.Mats[k])) bHidden = true;
				if (UMaterialInterface* MI = Mat(N.Mats[k])) M->SetMaterialByName(FName(*FString::Printf(TEXT("slot%d"), k)), MI);
			}
			const bool bWire = Bool(O, TEXT("wire"));
			M->SetCastShadow(!bWire && !bShader);
			M->SetCollisionEnabled(bWire ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
			M->SetCollisionResponseToAllChannels(ECR_Ignore);
			M->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			if (ItemOfNode[i] >= 0) M->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
			// the instances and their colours go in before the component is registered, so its
			// render data is made with them
			if (N.bInstanced)
			{
				UInstancedStaticMeshComponent* I = Cast<UInstancedStaticMeshComponent>(M);
				const TArray<TSharedPtr<FJsonValue>>& Inst = Arr(O, TEXT("inst"));
				const TArray<TSharedPtr<FJsonValue>>& IC = Arr(O, TEXT("instColor"));
				if (IC.Num() > 0) I->NumCustomDataFloats = 3;
				for (int32 k = 0; k < Inst.Num(); ++k)
				{
					double E[16];
					Mat16(Inst[k]->AsArray(), E);
					I->AddInstance(MatToUE(E), false);
				}
				if (IC.Num() > 0)
				{
					for (int32 k = 0; k < Inst.Num(); ++k)
					{
						TArray<float> Cd;
						for (int32 c = 0; c < 3; ++c) Cd.Add(IC.IsValidIndex(k * 3 + c) ? float(IC[k * 3 + c]->AsNumber()) : 1.f);
						I->SetCustomData(k, Cd, false);
					}
					for (int32 k = 0; k < N.Mats.Num(); ++k)
					{
						if (UMaterialInterface* MI = Mat(N.Mats[k]))
						{
							// made from the material asset, with the flood's values copied over (a
							// dynamic instance of a dynamic instance did not take the instance colours)
							UMaterialInstanceDynamic* Src = Cast<UMaterialInstanceDynamic>(MI);
							UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Src ? Src->Parent.Get() : MI, this);
							if (Src) D->CopyParameterOverrides(Src);
							if (const int32* Fs = MatFloodSet.Find(N.Mats[k])) FloodMids.FindOrAdd(*Fs).Add(D);
							D->SetScalarParameterValue(TEXT("UseInstanceColor"), 1.f);
							I->SetMaterialByName(FName(*FString::Printf(TEXT("slot%d"), k)), D);
							KeepMids.Add(D);
						}
					}
				}
			}
			M->RegisterComponent();
			// shader materials are drawn by other means (sky picture, lights); hidden proxies only collide
			if (bShader || bHidden) M->SetHiddenInGame(true);
			CompNode.Add(M, i);
			C = M;
			++Built;
		}
		else
		{
			C = NewObject<USceneComponent>(this, *FString::Printf(TEXT("N%d"), i));
			C->SetMobility(EComponentMobility::Movable);
			C->SetupAttachment(Parent);
			C->SetRelativeTransform(MatToUE(N.Local));
			C->RegisterComponent();
			// glow points (GlowPoints): a billboard each, of the point's size in metres
			const TArray<TSharedPtr<FJsonValue>>& Pts = Arr(O, TEXT("points"));
			if (Pts.Num() > 0 && Pts.Num() < 2000 && N.bVis && !bSky && GlowBase)
			{
				for (const TSharedPtr<FJsonValue>& PV : Pts)
				{
					const TArray<TSharedPtr<FJsonValue>>& P = PV->AsArray();
					if (P.Num() < 7) continue;
					UMaterialBillboardComponent* B = NewObject<UMaterialBillboardComponent>(this);
					B->SetupAttachment(C);
					B->SetRelativeLocation(ToUE(P[0]->AsNumber(), P[1]->AsNumber(), P[2]->AsNumber()));
					UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(GlowBase, this);
					D->SetVectorParameterValue(TEXT("Color"), FLinearColor(P[3]->AsNumber(), P[4]->AsNumber(), P[5]->AsNumber()));
					KeepMids.Add(D);
					// aSize is the sprite's width in metres; the element's size is half of it each way
					const float Size = float(P[6]->AsNumber() * 50.0);
					B->AddElement(D, nullptr, false, Size, Size, nullptr);
					B->RegisterComponent();
				}
			}
		}
		N.Comp = C;
	}
	// the sky and the backdrop are the sky picture now
	if (SkyRoot >= 0) SetVisible(SkyRoot, false);
	if (BackRoot >= 0) SetVisible(BackRoot, false);
	for (int32 i = 0; i < Nodes.Num(); ++i) if (Nodes[i].Parent < 0) ApplyVisibility(i, true);
	UE_LOG(LogS47Web, Display, TEXT("S47: web world built, %d meshes, %d missing"), Built, Missing);

	BuildLights(Arr(J, TEXT("lights")));
	// the control room's lights by role (ControlRoom.ts this.lights)
	const TSharedPtr<FJsonObject>* RL = nullptr;
	if (J->TryGetObjectField(TEXT("roomLights"), RL))
	{
		for (const TSharedPtr<FJsonValue>& U : Arr(*RL, TEXT("ceiling")))
			if (UPointLightComponent* P = Cast<UPointLightComponent>(LightByUuid(U->AsString()))) { CeilingLights.Add(P); CeilingBase.Add(P->Intensity); }
		for (const TSharedPtr<FJsonValue>& U : Arr(*RL, TEXT("lamps")))
			if (UPointLightComponent* P = Cast<UPointLightComponent>(LightByUuid(U->AsString()))) { LampLights.Add(P); LampBase.Add(P->Intensity); }
		CrtLight = Cast<UPointLightComponent>(LightByUuid(Str(*RL, TEXT("crt"))));
	}
	UE_LOG(LogS47Web, Display, TEXT("S47: room lights %d ceiling, %d lamps, CRT %s"), CeilingLights.Num(), LampLights.Num(), CrtLight.IsValid() ? TEXT("yes") : TEXT("no"));
	return true;
}

void AS47Web::BuildLights(const TArray<TSharedPtr<FJsonValue>>& JL)
{
	USceneComponent* Root = GetRootComponent();
	for (const TSharedPtr<FJsonValue>& V : JL)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		const FString Type = Str(O, TEXT("type"));
		const int32 Parent = int32(Num(O, TEXT("parent"), -1));
		const FString* Ar = Parent >= 0 && Nodes.IsValidIndex(Parent) ? RootArea.Find(Nodes[Parent].Root) : nullptr;
		const FString Area = Ar ? *Ar : TEXT("global");
		const TArray<TSharedPtr<FJsonValue>>& P = Arr(O, TEXT("pos"));
		const FVector Pos = P.Num() == 3 ? FVector(P[0]->AsNumber(), P[1]->AsNumber(), P[2]->AsNumber()) : FVector::ZeroVector;
		const FLinearColor C = Col(Arr(O, TEXT("color")));
		const float I = float(Num(O, TEXT("intensity"), 1));
		const FString Uuid = Str(O, TEXT("uuid"));
		if (Type == TEXT("HemisphereLight"))
		{
			Hemis.Add(Area, { C, Col(Arr(O, TEXT("ground"))), I });
			continue;
		}
		if (Type == TEXT("PointLight"))
		{
			UPointLightComponent* L = NewObject<UPointLightComponent>(this);
			L->SetupAttachment(Root);
			L->SetWorldLocation(ToUE(Pos));
			L->SetMobility(EComponentMobility::Movable);
			L->SetIntensityUnits(ELightUnits::Candelas);
			L->SetIntensity(I);
			L->SetLightColor(C);
			const double D = Num(O, TEXT("distance"), 0);
			L->SetAttenuationRadius(float(D > 0 ? D * 100.0 : 3000.0));
			L->SetSourceRadius(8.f);
			L->SetCastShadows(Bool(O, TEXT("castShadow")));
			L->SetVisibility(Bool(O, TEXT("vis"), true));
			L->RegisterComponent();
			Lights.Add({ L, Area, I, Uuid });
		}
		else if (Type == TEXT("DirectionalLight"))
		{
			const TArray<TSharedPtr<FJsonValue>>& Tg = Arr(O, TEXT("target"));
			const FVector To = Tg.Num() == 3 ? FVector(Tg[0]->AsNumber(), Tg[1]->AsNumber(), Tg[2]->AsNumber()) : FVector::ZeroVector;
			UDirectionalLightComponent* L = NewObject<UDirectionalLightComponent>(this);
			L->SetupAttachment(Root);
			L->SetWorldRotation(DirToUE(To - Pos).Rotation());
			L->SetMobility(EComponentMobility::Movable);
			L->SetIntensity(I);
			L->SetLightColor(C);
			L->SetCastShadows(Bool(O, TEXT("castShadow")));
			L->SetAtmosphereSunLight(false);
			L->RegisterComponent();
			Lights.Add({ L, Area, I, Uuid });
		}
	}
}

ULightComponent* AS47Web::LightByUuid(const FString& Uuid) const
{
	for (const FLightRef& L : Lights) if (L.Uuid == Uuid && L.L.IsValid()) return L.L.Get();
	return nullptr;
}

void AS47Web::BuildRoom()
{
	// The CRTs: their canvases become render targets the HUD draws into.
	static const TCHAR* Names[4] = { TEXT("crtLeft"), TEXT("crtCenter"), TEXT("crtRight"), TEXT("crtOps") };
	UMaterialInterface* Unlit = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Unlit_Opaque.M_Web_Unlit_Opaque"));
	for (int32 s = 0; s < 4; ++s)
	{
		UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this);
		RT->ClearColor = FLinearColor(0.002f, 0.008f, 0.003f);
		RT->InitCustomFormat(CrtW, CrtH, PF_B8G8R8A8, false);
		RT->UpdateResourceImmediate(true);
		CrtTargets.Add(RT);
		CrtOn.Add(false);
		CrtMat[s] = HandleMat(FString::Printf(TEXT("room.%s.tex"), Names[s]));
		if (CrtMat[s] < 0) CrtMat[s] = HandleMat(FString::Printf(TEXT("room.%s.mat"), Names[s]));
		UMaterialInterface* Base = Mat(CrtMat[s]) ? Mat(CrtMat[s]) : Unlit;
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Base ? Base : Unlit, this);
		D->SetScalarParameterValue(TEXT("UseMap"), 1.f);
		D->SetTextureParameterValue(TEXT("Map"), RT);
		// the canvas is uploaded flipped in three (flipY) and the exported pictures are stored so;
		// the render target is not, so its rows run the other way
		D->SetVectorParameterValue(TEXT("MapU"), FLinearColor(1, 0, 0, 0));
		D->SetVectorParameterValue(TEXT("MapV"), FLinearColor(0, -1, 1, 0));
		D->SetVectorParameterValue(TEXT("Color"), Hex(0x0d1410));
		CrtMids.Add(D);
	}
	UpdateCrtMaterials();
	LeverNode = Handle(TEXT("room.lever"));
	LedNode = Handle(TEXT("room.leds"));
	PaperNode = Handle(TEXT("room.printerPaper"));
	HandsetNode = Handle(TEXT("room.handset"));
	MugNode = Handle(TEXT("room.mug"));
	CoffeeNode = Handle(TEXT("room.coffee"));
	JacketNode = Handle(TEXT("room.objs.jacket"));
	HandH = Handle(TEXT("room.clockHands.h")); HandM = Handle(TEXT("room.clockHands.m")); HandS = Handle(TEXT("room.clockHands.s"));
	for (int32 k = 0; k < 8; ++k) { const int32 T = Handle(FString::Printf(TEXT("room.tubes[%d]"), k)); if (T >= 0) TubeNodes.Add(T); }
	if (TubeNodes.Num() > 0 && Nodes[TubeNodes[0]].Mats.Num() > 0)
	{
		TubeOnMat = Nodes[TubeNodes[0]].Mats[0];
		if (UMaterialInterface* On = Mat(TubeOnMat))
		{
			TubeOffMid = UMaterialInstanceDynamic::Create(On, this);
			TubeOffMid->SetVectorParameterValue(TEXT("Emissive"), FLinearColor::Black);
			TubeOffMid->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.35f, 0.35f, 0.33f));
		}
	}
	if (MugNode >= 0) MugHome = Nodes[MugNode].P;
	if (CoffeeNode >= 0) CoffeeY = Nodes[CoffeeNode].P.Y;
	// the room's lights by role (ControlRoom.ts this.lights)
	FString Path = FPaths::ProjectContentDir() / TEXT("WebData/manifest.json");
	(void)Path;
}

void AS47Web::UpdateCrtMaterials()
{
	// every slot drawn with a CRT's material gets that screen's render target material
	for (FS47WebNode& N : Nodes)
	{
		UStaticMeshComponent* M = Cast<UStaticMeshComponent>(N.Comp.Get());
		if (!M) continue;
		for (int32 k = 0; k < N.Mats.Num(); ++k)
		{
			for (int32 s = 0; s < 4; ++s)
			{
				if (CrtMat[s] >= 0 && N.Mats[k] == CrtMat[s] && CrtMids.IsValidIndex(s)) M->SetMaterialByName(FName(*FString::Printf(TEXT("slot%d"), k)), CrtMids[s]);
			}
		}
	}
}

void AS47Web::BeginPlay()
{
	Super::BeginPlay();
	FParse::Value(FCommandLine::Get(), TEXT("S47Exposure="), ExposureOffset);
	bLoaded = Load();
	if (!bLoaded) return;
	BuildRoom();
	// the sky picture on a big sphere round the eye
	SkySphere = NewObject<UStaticMeshComponent>(this, TEXT("SkySphere"));
	SkySphere->SetupAttachment(GetRootComponent());
	SkySphere->SetMobility(EComponentMobility::Movable);
	SkySphere->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	SkySphere->SetWorldScale3D(FVector(3000.f)); // 1.5 km radius
	SkySphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkySphere->SetCastShadow(false);
	SkySphere->bAffectDistanceFieldLighting = false;
	SkySphere->RegisterComponent();
	if (UMaterialInterface* SkyBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebSky.M_WebSky")))
	{
		SkyMid = UMaterialInstanceDynamic::Create(SkyBase, this);
		SkySphere->SetMaterial(0, SkyMid);
	}
	// The web version's picture: its fog, ACES curve and exposure in M_WebTone, in place of
	// Unreal's tone mapping; Unreal's exposure held at 1 so the light values are three's.
	FParse::Value(FCommandLine::Get(), TEXT("S47Scene="), SceneScale);
	Env = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Web/Base/MPC_Web.MPC_Web"));
	APostProcessVolume* V = GetWorld()->SpawnActor<APostProcessVolume>();
	V->bUnbound = true;
	FPostProcessSettings& S = V->Settings;
	S.bOverride_AutoExposureMethod = true; S.AutoExposureMethod = AEM_Manual;
	S.bOverride_AutoExposureApplyPhysicalCameraExposure = true; S.AutoExposureApplyPhysicalCameraExposure = false;
	S.bOverride_AutoExposureBias = true; S.AutoExposureBias = ExposureOffset;
	S.bOverride_BloomIntensity = true; S.BloomIntensity = 0.f;
	S.bOverride_VignetteIntensity = true; S.VignetteIntensity = 0.f;
	S.bOverride_MotionBlurAmount = true; S.MotionBlurAmount = 0.f;
	S.bOverride_AmbientOcclusionIntensity = true; S.AmbientOcclusionIntensity = 0.f;
	S.bOverride_LocalExposureHighlightContrastScale = true; S.LocalExposureHighlightContrastScale = 1.f;
	S.bOverride_LocalExposureShadowContrastScale = true; S.LocalExposureShadowContrastScale = 1.f;
	if (UMaterialInterface* Tone = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebTone.M_WebTone")))
	{
		S.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, Tone));
	}
	if (UMaterialParameterCollectionInstance* E = Env ? GetWorld()->GetParameterCollectionInstance(Env) : nullptr)
	{
		E->SetScalarParameterValue(TEXT("ToneK"), float(ToneMappingExposure / 0.6) * SceneScale);
		E->SetScalarParameterValue(TEXT("FogDensity"), FogDensity);
		E->SetVectorParameterValue(TEXT("FogColor"), FogColor);
		// the picture: the tape look by default, as the web version on every tier but Low
		FString Picture = TEXT("vhs");
		FParse::Value(FCommandLine::Get(), TEXT("S47Picture="), Picture);
		E->SetScalarParameterValue(TEXT("Vhs"), Picture == TEXT("off") ? 0.f : 1.f);
	}
	Enter(TEXT("saro"));
	CloseAllDoors();
	// for finding things in the picture: -S47Hide=870,874 hides those nodes
	FString Hide;
	if (FParse::Value(FCommandLine::Get(), TEXT("S47Hide="), Hide))
	{
		TArray<FString> Ids;
		Hide.ParseIntoArray(Ids, TEXT(","));
		for (const FString& I : Ids) SetVisible(FCString::Atoi(*I), false);
	}
	DrawDishes();
	UE_LOG(LogS47Web, Display, TEXT("S47: web world ready, %s"), *Summary());
}

// ---------- visibility and transforms ----------

void AS47Web::ApplyVisibility(int32 I, bool bParent)
{
	FS47WebNode& N = Nodes[I];
	const bool bOn = bParent && N.bVis;
	if (USceneComponent* C = N.Comp.Get())
	{
		if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(C)) P->SetVisibility(bOn, false);
		for (USceneComponent* Ch : C->GetAttachChildren()) if (Ch && !CompNode.Contains(Cast<UPrimitiveComponent>(Ch)) && Ch->IsA<UMaterialBillboardComponent>()) Ch->SetVisibility(bOn, false);
	}
	for (int32 Ch : N.Children) ApplyVisibility(Ch, bOn);
}

void AS47Web::SetVisible(int32 I, bool bOn)
{
	if (!Nodes.IsValidIndex(I)) return;
	Nodes[I].bVis = bOn;
	bool bParent = true;
	for (int32 P = Nodes[I].Parent; P >= 0; P = Nodes[P].Parent) bParent &= Nodes[P].bVis;
	ApplyVisibility(I, bParent);
}

void AS47Web::SetLocal(int32 I, const FVector& P, const FQuat& Q, const FVector& S)
{
	if (!Nodes.IsValidIndex(I)) return;
	FS47WebNode& N = Nodes[I];
	N.P = P; N.Q = Q; N.S = S;
	Compose(P, Q, S, N.Local);
	if (USceneComponent* C = N.Comp.Get()) C->SetRelativeTransform(MatToUE(N.Local));
}

FVector AS47Web::WorldThree(int32 I) const
{
	if (!Nodes.IsValidIndex(I) || !Nodes[I].Comp.IsValid()) return FVector::ZeroVector;
	return ToThree(Nodes[I].Comp->GetComponentLocation());
}

FVector AS47Web::WorldUE(int32 I) const
{
	return Nodes.IsValidIndex(I) && Nodes[I].Comp.IsValid() ? Nodes[I].Comp->GetComponentLocation() : FVector::ZeroVector;
}

// ---------- areas ----------

void AS47Web::Enter(const FString& Area)
{
	CurrentArea = Area;
	// the motel's lot is SARO's area, the diner stands in the road's (World.enter)
	auto Shown = [&](const FString& A)
	{
		if (A == Area) return true;
		if ((Area == TEXT("road") && A == TEXT("diner")) || (Area == TEXT("diner") && A == TEXT("road"))) return true;
		if (Area == TEXT("roswell") && A == TEXT("diner")) return true;
		return false;
	};
	for (const auto& KV : AreaRoots) for (int32 R : KV.Value) SetVisible(R, Shown(KV.Key));
	for (FLightRef& L : Lights)
	{
		if (!L.L.IsValid()) continue;
		const bool bOn = L.Area == TEXT("global") || Shown(L.Area);
		L.L->SetVisibility(bOn);
	}
	const FHemi* H = Hemis.Find(Area);
	if (!H) H = Hemis.Find(TEXT("saro"));
	UMaterialParameterCollectionInstance* E = Env ? GetWorld()->GetParameterCollectionInstance(Env) : nullptr;
	if (H && E)
	{
		E->SetVectorParameterValue(TEXT("HemiSky"), H->Sky * H->Intensity);
		E->SetVectorParameterValue(TEXT("HemiGround"), H->Ground * H->Intensity);
	}
	const FString* T = SkyTex.Find(Area);
	if (!T) T = SkyTex.Find(TEXT("saro"));
	if (T && SkyMid)
	{
		if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, **T)) SkyMid->SetTextureParameterValue(TEXT("Sky"), Tex);
	}
}

const TArray<FS47Rect>& AS47Web::Zones() const
{
	static const TArray<FS47Rect> Empty;
	const auto* P = Walks.Find(CurrentArea == TEXT("diner") && !Walks.Contains(TEXT("diner")) ? TEXT("road") : CurrentArea);
	return P ? P->Key : Empty;
}

const TArray<FS47Rect>& AS47Web::Colliders() const
{
	static const TArray<FS47Rect> Empty;
	const auto* P = Walks.Find(CurrentArea);
	return P ? P->Value : Empty;
}

// ---------- items ----------

const FS47WebItem* AS47Web::Item(const FString& Id) const
{
	return ItemList.FindByPredicate([&](const FS47WebItem& I) { return I.Id == Id; });
}

const FS47WebItem* AS47Web::ItemOf(const UPrimitiveComponent* C) const
{
	const int32* N = CompNode.Find(C);
	if (!N) return nullptr;
	for (int32 I = *N; I >= 0; I = Nodes[I].Parent)
	{
		if (const int32* It = NodeItem.Find(I)) return &ItemList[*It];
	}
	return nullptr;
}

FVector AS47Web::PropLocation(const FString& Id) const
{
	if (const FS47WebItem* I = Item(Id))
	{
		if (I->bBox) return ToUE((I->Min + I->Max) * 0.5);
		return WorldUE(I->Node);
	}
	const int32 H = Handle(TEXT("room.objs.") + Id);
	return H >= 0 ? WorldUE(H) : FVector::ZeroVector;
}

// ---------- the control room ----------

void AS47Web::SetLever(bool bOn)
{
	// applyK3: open (off) -0.6, closed (on) 0.6 about z
	if (LeverNode < 0) return;
	const FVector E = EulerXYZ(Nodes[LeverNode].Q);
	SetRotation(LeverNode, E.X, E.Y, bOn ? 0.6 : -0.6);
}

void AS47Web::SetLed(int32 Index, bool bOn)
{
	if (LedNode < 0) return;
	UInstancedStaticMeshComponent* I = Cast<UInstancedStaticMeshComponent>(Nodes[LedNode].Comp.Get());
	if (!I || Index < 0 || Index >= I->GetInstanceCount()) return;
	// LED_COL in ControlRoom.ts; ledState: 7 and 15 amber, the rest green
	const FLinearColor C = !bOn ? Hex(0x141619) : (Index == 7 || Index == 15) ? Hex(0xffb040) : Hex(0x45ff7a);
	I->SetCustomDataValue(Index, 0, C.R, false);
	I->SetCustomDataValue(Index, 1, C.G, false);
	I->SetCustomDataValue(Index, 2, C.B, true);
}

void AS47Web::SetLeds(bool bOn)
{
	for (int32 i = 0; i < LedCount(); ++i) SetLed(i, bOn);
}

void AS47Web::FlickerLed()
{
	const int32 I = FMath::RandRange(0, LedCount() - 1);
	SetLed(I, FMath::FRand() >= 0.3f);
}

void AS47Web::SetCrtPowered(int32 Screen, bool bOn)
{
	if (!CrtOn.IsValidIndex(Screen)) return;
	CrtOn[Screen] = bOn;
	if (CrtMids.IsValidIndex(Screen) && CrtMids[Screen]) CrtMids[Screen]->SetVectorParameterValue(TEXT("Color"), bOn ? FLinearColor::White : Hex(0x0d1410));
}

void AS47Web::SetCrtGlow(float Intensity)
{
	if (CrtLight.IsValid()) CrtLight->SetIntensity(Intensity);
}

void AS47Web::SetPrinterPaper(float Share)
{
	if (PaperNode < 0) return;
	SetVisible(PaperNode, Share >= 0.f);
	if (Share >= 0.f) SetScale(PaperNode, FVector(Nodes[PaperNode].S.X, FMath::Clamp(Share, 0.001f, 1.f), Nodes[PaperNode].S.Z));
}

void AS47Web::SetHandsetVisible(bool bOn) { if (HandsetNode >= 0) SetVisible(HandsetNode, bOn); }
bool AS47Web::IsMugVisible() const { return MugNode >= 0 && Nodes[MugNode].bVis; }
void AS47Web::SetJacketVisible(bool bOn) { if (JacketNode >= 0) SetVisible(JacketNode, bOn); }

void AS47Web::SetCoffeeLevel(double Y) { CoffeeY = Y; ApplyCoffee(); }

void AS47Web::ApplyCoffee()
{
	if (CoffeeNode < 0) return;
	SetVisible(CoffeeNode, CoffeeState > 0);
	const FS47WebNode& N = Nodes[CoffeeNode];
	SetPosition(CoffeeNode, FVector(N.P.X, CoffeeY, N.P.Z));
}

void AS47Web::DropMug()
{
	if (MugNode < 0 || !Nodes[MugNode].bVis || MugFall) return;
	MugFall = FMugFall{ 0.f, FVector(0, 0, -0.5), 6.f + FMath::FRand() * 3.f, 0, EulerXYZ(Nodes[MugNode].Q) };
}

void AS47Web::ResetMug()
{
	MugFall.Reset();
	if (MugNode < 0) return;
	SetVisible(MugNode, true);
	SetLocal(MugNode, MugHome, FQuat::Identity, Nodes[MugNode].S);
}

void AS47Web::SetClock(double Seconds)
{
	const double S = FMath::Fmod(Seconds, 60.0), M = FMath::Fmod(Seconds / 60.0, 60.0), H = FMath::Fmod(Seconds / 3600.0, 12.0);
	if (HandS >= 0) SetRotation(HandS, 0, 0, -FMath::FloorToDouble(S) / 60.0 * PI * 2);
	if (HandM >= 0) SetRotation(HandM, 0, 0, -M / 60.0 * PI * 2);
	if (HandH >= 0) SetRotation(HandH, 0, 0, -H / 12.0 * PI * 2);
}

void AS47Web::SetLightsScale(float Ceiling, float Lamps)
{
	for (int32 i = 0; i < CeilingLights.Num(); ++i) if (CeilingLights[i].IsValid()) CeilingLights[i]->SetIntensity(CeilingBase[i] * Ceiling);
	for (int32 i = 0; i < LampLights.Num(); ++i) if (LampLights[i].IsValid()) LampLights[i]->SetIntensity(LampBase[i] * Lamps);
}

void AS47Web::SetTubes(bool bOn, int32 KeepOff)
{
	for (int32 k = 0; k < TubeNodes.Num(); ++k)
	{
		UStaticMeshComponent* M = Cast<UStaticMeshComponent>(Nodes[TubeNodes[k]].Comp.Get());
		if (!M) continue;
		const bool bLit = bOn && k != KeepOff;
		M->SetMaterialByName(TEXT("slot0"), bLit ? Mat(TubeOnMat) : (UMaterialInterface*)TubeOffMid.Get());
	}
}

// ---------- the dishes ----------

void AS47Web::PointDishes(double Az, double El, double Slew)
{
	for (FDish& D : Dishes) { D.TgtAz = Az; D.TgtEl = El; if (Slew > 0) D.Slew = Slew; }
}

void AS47Web::SnapDishes(double Az, double El)
{
	for (FDish& D : Dishes) { D.CurAz = D.TgtAz = Az; D.CurEl = D.TgtEl = El; }
	DrawDishes();
}

bool AS47Web::DishMoving(int32 I) const
{
	if (!Dishes.IsValidIndex(I)) return false;
	return FMath::Abs(Dishes[I].CurAz - Dishes[I].TgtAz) > 0.05 || FMath::Abs(Dishes[I].CurEl - Dishes[I].TgtEl) > 0.05;
}

bool AS47Web::DishesMoving() const
{
	for (int32 i = 0; i < Dishes.Num(); ++i) if (DishMoving(i)) return true;
	return false;
}

void AS47Web::DrawDishes()
{
	// Dish.matrices: root at (x, 0, z) scaled; az = root * T(0, 7.8, 0) * Ry(-az); el = az * T(0, 4.6, 0) * Rx(-(90 - el))
	for (const FDishPart& P : DishParts)
	{
		UInstancedStaticMeshComponent* I = P.Node >= 0 ? Cast<UInstancedStaticMeshComponent>(Nodes[P.Node].Comp.Get()) : nullptr;
		if (!I) continue;
		for (int32 k = 0; k < P.DishIdx.Num() && k < I->GetInstanceCount(); ++k)
		{
			if (!Dishes.IsValidIndex(P.DishIdx[k])) continue;
			const FDish& D = Dishes[P.DishIdx[k]];
			double Root[16], Az[16], El[16], T[16], R[16];
			Compose(FVector(D.X, 0, D.Z), FQuat::Identity, FVector(D.Scale), Root);
			double* Out = Root;
			if (P.Level >= 1)
			{
				Compose(FVector(0, 7.8, 0), FQuat::Identity, FVector::OneVector, T);
				Compose(FVector::ZeroVector, Euler(0, -D.CurAz * DEG, 0), FVector::OneVector, R);
				Multiply(Root, T, Az); Multiply(Az, R, Az);
				Out = Az;
			}
			if (P.Level >= 2)
			{
				Compose(FVector(0, 4.6, 0), FQuat::Identity, FVector::OneVector, T);
				Compose(FVector::ZeroVector, Euler(-(90 - D.CurEl) * DEG, 0, 0), FVector::OneVector, R);
				Multiply(Az, T, El); Multiply(El, R, El);
				Out = El;
			}
			I->UpdateInstanceTransform(k, MatToUE(Out), false, k == P.DishIdx.Num() - 1, true);
		}
	}
}

// ---------- doors ----------

AS47Web::FDoor* AS47Web::DoorByItem(const FString& ItemId) { return Doors.FindByPredicate([&](const FDoor& D) { return D.Inter == ItemId; }); }
const AS47Web::FDoor* AS47Web::DoorByItem(const FString& ItemId) const { return Doors.FindByPredicate([&](const FDoor& D) { return D.Inter == ItemId; }); }
bool AS47Web::IsDoor(const FString& ItemId) const { return DoorByItem(ItemId) != nullptr; }
bool AS47Web::IsDoorOpen(const FString& ItemId) const { const FDoor* D = DoorByItem(ItemId); return D && D->bOpen; }
FString AS47Web::DoorLabel(const FString& ItemId) const
{
	const FDoor* D = DoorByItem(ItemId);
	return D ? FString::Printf(TEXT("%s %s"), D->bOpen ? TEXT("Close") : TEXT("Open"), *D->Name) : FString();
}

void AS47Web::ApplyDoor(FDoor& D, double K)
{
	D.K = K;
	for (int32 m = 0; m < D.MovedNodes.Num(); ++m)
	{
		if (D.Shut[m].Num() < 16 || D.Open[m].Num() < 16) continue;
		FVector P0, S0, P1, S1; FQuat Q0, Q1;
		Decompose(D.Shut[m].GetData(), P0, Q0, S0);
		Decompose(D.Open[m].GetData(), P1, Q1, S1);
		SetLocal(D.MovedNodes[m], FMath::Lerp(P0, P1, K), FQuat::Slerp(Q0, Q1, K), FMath::Lerp(S0, S1, K));
	}
}

void AS47Web::SetDoor(FDoor& D, bool bOpen, bool bInstant)
{
	const bool bChanged = D.bOpen != bOpen;
	D.bOpen = bOpen;
	auto& W = Walks.FindOrAdd(D.Area);
	for (int32 Z : D.Zones) if (W.Key.IsValidIndex(Z)) W.Key[Z].bEnabled = bOpen;
	if (D.Leaf)
	{
		const int32 I = W.Value.IndexOfByPredicate([&](const FS47Rect& R) { return R.MinX == D.Leaf->MinX && R.MaxX == D.Leaf->MaxX && R.MinZ == D.Leaf->MinZ && R.MaxZ == D.Leaf->MaxZ; });
		if (bOpen && I < 0) W.Value.Add(*D.Leaf);
		if (!bOpen && I >= 0) W.Value.RemoveAt(I);
	}
	if (bInstant) { D.T0 = -1; ApplyDoor(D, bOpen ? 1 : 0); }
	else if (bChanged) { D.From = D.K; D.T0 = DoorT; }
}

void AS47Web::ToggleDoor(const FString& ItemId)
{
	FDoor* D = DoorByItem(ItemId);
	if (!D) return;
	SetDoor(*D, !D->bOpen, false);
	if (OnDoor) OnDoor(D->Id, D->bOpen, D->Proxy >= 0 ? WorldUE(D->Proxy) : PropLocation(ItemId), D->Sound != TEXT("light"));
}

void AS47Web::CloseAllDoors()
{
	for (FDoor& D : Doors) SetDoor(D, false, true);
}

void AS47Web::SetDoorOpen(const FString& DoorId, bool bOpen, bool bInstant)
{
	if (FDoor* D = Doors.FindByPredicate([&](const FDoor& X) { return X.Id == DoorId; })) SetDoor(*D, bOpen, bInstant);
}

// ---------- per frame ----------

void AS47Web::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!bLoaded) return;
	// the sky follows the eye
	if (SkySphere) SkySphere->SetWorldLocation(Eye);
	// dishes slew (Dish.update)
	bool bMoved = false;
	for (FDish& D : Dishes)
	{
		const double Step = D.Slew * Dt;
		const double DA = D.TgtAz - D.CurAz, DE = D.TgtEl - D.CurEl;
		if (FMath::Abs(DA) < 1e-9 && FMath::Abs(DE) < 1e-9) continue;
		D.CurAz += FMath::Sign(DA) * FMath::Min(FMath::Abs(DA), Step);
		D.CurEl += FMath::Sign(DE) * FMath::Min(FMath::Abs(DE), Step * 0.6);
		bMoved = true;
	}
	if (bMoved) DrawDishes();
	// doors swing (Doors.update)
	DoorT += Dt;
	for (FDoor& D : Doors)
	{
		if (D.T0 < 0) continue;
		const double U = FMath::Min(1.0, (DoorT - D.T0) / SWING);
		const double E = 1 - FMath::Pow(1 - U, 3);
		ApplyDoor(D, D.From + ((D.bOpen ? 1 : 0) - D.From) * E);
		if (U >= 1) D.T0 = -1;
	}
	// the handset rattles while it rings
	if (HandsetNode >= 0)
	{
		RattleT += Dt;
		const FVector E = EulerXYZ(Nodes[HandsetNode].Q);
		const double Z = bRattle ? FMath::Sin(RattleT * 70.0) * 0.05 * (FMath::Sin(RattleT * 2.2) > -0.2 ? 1 : 0) : 0;
		if (bRattle || FMath::Abs(E.Z) > 1e-6) SetRotation(HandsetNode, E.X, E.Y, Z);
	}
	// the mug walks off the desk edge, falls, shatters (Prologue.update)
	if (MugFall && MugNode >= 0)
	{
		FMugFall& F = *MugFall;
		F.T += Dt;
		FS47WebNode& N = Nodes[MugNode];
		FVector P = N.P;
		if (F.Stage == 0)
		{
			P.Z = MugHome.Z - FMath::Min(1.f, F.T / 0.55f) * 0.27 + (FMath::FRand() - 0.5f) * 0.006;
			P.X = MugHome.X + (FMath::FRand() - 0.5f) * 0.006;
			F.Rot.Y += (FMath::FRand() - 0.5f) * 0.15f;
			if (F.T >= 0.55f) F.Stage = 1;
		}
		else
		{
			F.V.Y -= 9.8 * Dt;
			P += F.V * Dt;
			F.Rot.X -= F.Spin * Dt;
		}
		SetLocal(MugNode, P, Euler(F.Rot.X, F.Rot.Y, F.Rot.Z), N.S);
		if (F.Stage == 1 && P.Y <= 0.03)
		{
			MugFall.Reset();
			SetVisible(MugNode, false);
			if (OnMugShatter) OnMugShatter(WorldUE(MugNode) * FVector(1, 1, 0) + FVector(0, 0, 20));
		}
	}
}

// ---------- the scene by path (S47Obj.h) ----------

FS47Obj AS47Web::Obj(const FString& Path)
{
	const int32* N = Handles.Find(Path);
	if (!N)
	{
		static TSet<FString> Told;
		if (!Told.Contains(Path)) { Told.Add(Path); UE_LOG(LogS47Web, Warning, TEXT("S47: no object at %s"), *Path); }
		return FS47Obj();
	}
	return FS47Obj(this, *N);
}

FS47Light AS47Web::Light(const FString& PathOrUuid)
{
	FS47Light L;
	L.W = this;
	const FString* U = HandleLights.Find(PathOrUuid);
	L.Uuid = U ? *U : PathOrUuid;
	return L;
}

FS47Mat AS47Web::Material(const FString& Path)
{
	FS47Mat M;
	M.W = this;
	M.Id = HandleMat(Path);
	return M;
}

FVector AS47Web::Vec(const FString& Path, const FVector& Default) const
{
	const FVector* V = HandleVecs.Find(Path);
	if (V) return *V;
	const int32* N = Handles.Find(Path);
	return N ? NodeWorldThree(*N).GetLocation() : Default;
}

FS47Area AS47Web::Rect(const FString& Path) const
{
	const FS47Area* R = HandleRects.Find(Path);
	return R ? *R : FS47Area();
}

bool AS47Web::HasPath(const FString& Path) const
{
	return Handles.Contains(Path) || HandleLights.Contains(Path) || HandleVecs.Contains(Path) || HandleRects.Contains(Path) || HandleMats.Contains(Path) || HandleTexs.Contains(Path);
}

TArray<FString> AS47Web::PathsUnder(const FString& Prefix) const
{
	TArray<FString> Out;
	for (const auto& KV : Handles) if (KV.Key.StartsWith(Prefix)) Out.Add(KV.Key);
	Out.Sort();
	return Out;
}

FTransform AS47Web::NodeWorldThree(int32 Node) const
{
	// three's matrixWorld, composed up the parents in three's own terms
	FTransform T = FTransform::Identity;
	for (int32 N = Node; N >= 0 && Nodes.IsValidIndex(N); N = Nodes[N].Parent)
	{
		T = T * FTransform(Nodes[N].Q, Nodes[N].P, Nodes[N].S);
	}
	return T;
}

TArray<UMaterialInstanceDynamic*> AS47Web::NodeMids(int32 Node)
{
	TArray<UMaterialInstanceDynamic*> Out;
	if (!Nodes.IsValidIndex(Node)) return Out;
	UMeshComponent* M = Cast<UMeshComponent>(Nodes[Node].Comp.Get());
	if (!M) return Out;
	TArray<TWeakObjectPtr<UMaterialInstanceDynamic>>& Cache = NodeMidCache.FindOrAdd(Node);
	if (Cache.Num() == M->GetNumMaterials())
	{
		for (const auto& D : Cache) if (D.IsValid()) Out.Add(D.Get());
		if (Out.Num() == Cache.Num()) return Out;
		Out.Reset();
	}
	Cache.Reset();
	for (int32 k = 0; k < M->GetNumMaterials(); ++k)
	{
		UMaterialInterface* Cur = M->GetMaterial(k);
		if (!Cur) continue;
		// its own copy, with what the shared one had (flood values, instance colours, a CRT's screen)
		UMaterialInstanceDynamic* Src = Cast<UMaterialInstanceDynamic>(Cur);
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Src ? Src->Parent.Get() : Cur, this);
		if (Src) D->CopyParameterOverrides(Src);
		for (const auto& KV : MatFloodSet)
		{
			if (Mat(KV.Key) == Cur) { FloodMids.FindOrAdd(KV.Value).Add(D); break; }
		}
		M->SetMaterial(k, D);
		KeepMids.Add(D);
		Cache.Add(D);
		Out.Add(D);
	}
	return Out;
}

UMaterialInstanceDynamic* AS47Web::SharedMid(int32 MatId)
{
	if (const TWeakObjectPtr<UMaterialInstanceDynamic>* Have = SharedMidCache.Find(MatId); Have && Have->IsValid()) return Have->Get();
	UMaterialInterface* Base = Mat(MatId);
	if (!Base) return nullptr;
	UMaterialInstanceDynamic* Src = Cast<UMaterialInstanceDynamic>(Base);
	UMaterialInstanceDynamic* D = Src ? Src : UMaterialInstanceDynamic::Create(Base, this);
	if (!Src)
	{
		KeepMids.Add(D);
		MatCache[MatId] = D;
		// every slot drawn with it now draws with the dynamic one
		for (FS47WebNode& N : Nodes)
		{
			UMeshComponent* M = Cast<UMeshComponent>(N.Comp.Get());
			if (!M) continue;
			for (int32 k = 0; k < N.Mats.Num(); ++k)
			{
				if (N.Mats[k] == MatId && M->GetMaterial(k) == Base) M->SetMaterial(k, D);
			}
		}
	}
	SharedMidCache.Add(MatId, D);
	return D;
}

void AS47Web::MarkUsable(int32 Node, const FString& Id)
{
	if (!Nodes.IsValidIndex(Node)) return;
	Usable.Add(Node, Id);
	ObjAt(Node).Traverse([](FS47Obj O)
	{
		if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(O.W->Nodes[O.I].Comp.Get()))
		{
			if (P->GetCollisionEnabled() == ECollisionEnabled::NoCollision) P->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			P->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		}
	});
}

FString AS47Web::UsableAt(const UPrimitiveComponent* C) const
{
	const int32* N = CompNode.Find(C);
	if (!N) return FString();
	for (int32 I = *N; I >= 0; I = Nodes[I].Parent)
	{
		if (const FString* Id = Usable.Find(I)) return *Id;
		if (const int32* It = NodeItem.Find(I)) return ItemList[*It].Id;
	}
	return FString();
}

// ---------- the fake floodlights ----------

int32 AS47Web::FloodSet(const FString& Key) const
{
	return FloodSets.IndexOfByPredicate([&](const FFloodSet& S) { return S.Key == Key; });
}

void AS47Web::UploadFlood(int32 Set)
{
	if (!FloodSets.IsValidIndex(Set) || !FloodTex.IsValidIndex(Set) || !FloodTex[Set]) return;
	const FFloodSet& F = FloodSets[Set];
	UTexture2D* T = FloodTex[Set];
	FLinearColor* Px = static_cast<FLinearColor*>(T->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
	for (int32 k = 0; k < F.W; ++k) { Px[k] = F.Pos[k]; Px[F.W + k] = F.Col[k]; }
	T->GetPlatformData()->Mips[0].BulkData.Unlock();
	T->UpdateResource();
}

void AS47Web::SetFlood(int32 Set, int32 Index, float Width)
{
	if (!FloodSets.IsValidIndex(Set) || !FloodSets[Set].Pos.IsValidIndex(Index)) return;
	if (FloodSets[Set].Pos[Index].A == Width) return;
	FloodSets[Set].Pos[Index].A = Width;
	UploadFlood(Set);
}

void AS47Web::SetFloodColor(int32 Set, int32 Index, const FLinearColor& C)
{
	if (!FloodSets.IsValidIndex(Set) || !FloodSets[Set].Col.IsValidIndex(Index)) return;
	FloodSets[Set].Col[Index] = C;
	UploadFlood(Set);
}

void AS47Web::SetFloodScale(int32 Set, float Scale)
{
	if (!FloodSets.IsValidIndex(Set)) return;
	FloodSets[Set].Scale = Scale;
	if (TArray<TWeakObjectPtr<UMaterialInstanceDynamic>>* Ms = FloodMids.Find(Set))
	{
		for (const auto& D : *Ms) if (D.IsValid()) D->SetScalarParameterValue(TEXT("FloodScale"), Scale);
	}
}

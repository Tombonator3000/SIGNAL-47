#include "S47StationLook.h"

#include "Components/BoxComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "S47Main.h"
#include "S47Props.h"
#include "S47StationDressing.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47StationLook, Log, All);

// A named namespace, not an anonymous one: unity builds put several files in one translation unit.
namespace S47StationLookLocal
{
	const TCHAR* StationId = TEXT("gasStation");
	FString FilePath() { return FPaths::ProjectConfigDir() / TEXT("Props") / TEXT("gas_look.json"); }

	// The shop's interior in town metres relative to the station model's pivot (the Blender model's interior_layout.json: town x
	// -26.8..-17.2, y up 5.4455..8.55, z 2066.2..2077.78; the pivot at (-16.656, 3.94, 2064.9075), gas_cosmos.json origin_town).
	// Town x is Unreal x, town z Unreal y, town y (up) Unreal z (S47Three::ToUE); the window and door wall is the +x side.
	constexpr double X0 = -10.144, X1 = -0.544, Y0 = 1.5055, Y1 = 4.61, Z0 = 1.2925, Z1 = 12.8725;
	const FVector Pivot(-16.655942535400392, 3.9399999991059302, 2064.9074993133545);

	double Num(const TSharedPtr<FJsonObject>& O, const TCHAR* K, double Default) { double D; return O->TryGetNumberField(K, D) ? D : Default; }
	bool Vec(const TSharedPtr<FJsonObject>& O, const TCHAR* K, FVector& Out)
	{
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		if (!O->TryGetArrayField(K, A) || A->Num() != 3) return false;
		const FVector V((*A)[0]->AsNumber(), (*A)[1]->AsNumber(), (*A)[2]->AsNumber());
		if (V.ContainsNaN()) return false;
		Out = V;
		return true;
	}
	bool Here(const FString& Asset) { return !Asset.IsEmpty() && FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Asset)); }
	FString V3(const FVector& V) { return FString::Printf(TEXT("%.3f %.3f %.3f"), V.X, V.Y, V.Z); }

	// One grade's keys (the shop's at the file's top, the forecourt's in "exterior") over the defaults already in L.
	void ReadGrade(const TSharedPtr<FJsonObject>& J, FS47StationLook::FLook& L)
	{
		L.Saturation = Num(J, TEXT("saturation"), L.Saturation);
		L.Contrast = Num(J, TEXT("contrast"), L.Contrast);
		Vec(J, TEXT("gamma"), L.Gamma);
		Vec(J, TEXT("gain"), L.Gain);
		Vec(J, TEXT("offset_shadows"), L.OffsetShadows);
		Vec(J, TEXT("gain_highlights"), L.GainHighlights);
		L.Grain = Num(J, TEXT("grain"), L.Grain);
		L.GrainSize = Num(J, TEXT("grain_size"), L.GrainSize);
		L.Vignette = Num(J, TEXT("vignette"), L.Vignette);
		L.Bloom = Num(J, TEXT("bloom"), L.Bloom);
		J->TryGetBoolField(TEXT("enabled"), L.bEnabled);
		J->TryGetStringField(TEXT("material"), L.Material);
	}

	// Only the grade: the global volume keeps the tone material, the exposure and everything else it sets.
	void Grade(FPostProcessSettings& S, const FS47StationLook::FLook& L, bool bBloom)
	{
		S.bOverride_ColorSaturation = true; S.ColorSaturation = FVector4(FVector(L.Saturation), 1.0);
		S.bOverride_ColorContrast = true; S.ColorContrast = FVector4(FVector(L.Contrast), 1.0);
		S.bOverride_ColorGamma = true; S.ColorGamma = FVector4(L.Gamma, 1.0);
		S.bOverride_ColorGain = true; S.ColorGain = FVector4(L.Gain, 1.0);
		S.bOverride_ColorOffsetShadows = true; S.ColorOffsetShadows = FVector4(L.OffsetShadows, 0.0);
		S.bOverride_ColorGainHighlights = true; S.ColorGainHighlights = FVector4(L.GainHighlights, 1.0);
		S.bOverride_FilmGrainIntensity = true; S.FilmGrainIntensity = float(L.Grain);
		// (grain_size: this engine has no live grain size setting, FilmGrainTexelSize is deprecated; kept in the file for the look material)
		S.bOverride_VignetteIntensity = true; S.VignetteIntensity = float(L.Vignette);
		if (bBloom) { S.bOverride_BloomIntensity = true; S.BloomIntensity = float(L.Bloom); }
	}

	// The look material, if here: a post-process material after the tone pass with the grade's values as parameters.
	UMaterialInstanceDynamic* LookMaterial(UObject* Owner, FPostProcessSettings& S, const FS47StationLook::FLook& L)
	{
		UMaterialInterface* Mat = Here(L.Material) ? LoadObject<UMaterialInterface>(nullptr, *L.Material) : nullptr;
		if (!Mat) return nullptr;
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Mat, Owner);
		Mid->SetScalarParameterValue(TEXT("Saturation"), float(L.Saturation));
		Mid->SetScalarParameterValue(TEXT("Contrast"), float(L.Contrast));
		Mid->SetVectorParameterValue(TEXT("Gamma"), FLinearColor(L.Gamma));
		Mid->SetVectorParameterValue(TEXT("Gain"), FLinearColor(L.Gain));
		Mid->SetVectorParameterValue(TEXT("OffsetShadows"), FLinearColor(L.OffsetShadows));
		Mid->SetVectorParameterValue(TEXT("GainHighlights"), FLinearColor(L.GainHighlights));
		Mid->SetScalarParameterValue(TEXT("Grain"), float(L.Grain));
		Mid->SetScalarParameterValue(TEXT("GrainSize"), float(L.GrainSize));
		Mid->SetScalarParameterValue(TEXT("Vignette"), float(L.Vignette));
		S.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, Mid));
		return Mid;
	}

	// One plain actor: a shape as its root (the volume's bounds), hung under the station's mesh in the model's own terms, as the
	// shop's Cosmos props are, and the post-process component under the shape, which takes it as its bounds.
	AActor* SpawnVolume(UWorld* World, const TCHAR* Tag, int32 Builds)
	{
		FActorSpawnParameters SP;
		SP.Name = MakeUniqueObjectName(World, AActor::StaticClass(), *FString::Printf(TEXT("S47StationLook%s_%d"), Tag, Builds));
		SP.ObjectFlags |= RF_Transient;
		return World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SP);
	}
	UPostProcessComponent* Volume(AActor* A, UShapeComponent* Shape, UStaticMeshComponent* Station, const FVector& Centre, double Priority, double BlendCm)
	{
		Shape->SetMobility(EComponentMobility::Movable);
		Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shape->SetGenerateOverlapEvents(false);
		A->SetRootComponent(Shape);
		Shape->RegisterComponent();
		Shape->AttachToComponent(Station, FAttachmentTransformRules::KeepRelativeTransform);
		Shape->SetRelativeTransform(FTransform(FRotator::ZeroRotator, Centre, FVector::OneVector));
		UPostProcessComponent* P = NewObject<UPostProcessComponent>(A, TEXT("Look"));
		P->SetupAttachment(Shape);
		P->bUnbound = false;
		P->bEnabled = true;
		P->Priority = float(Priority);
		P->BlendRadius = float(BlendCm);
		P->BlendWeight = 1.f;
		return P;
	}

	// Whether the game's global volume replaces Unreal's tonemapper (M_WebTone): then the engine draws no colour grading, grain or
	// vignette from any volume's settings, and only a look material shows the grade.
	bool TonemapperReplaced(const AS47Web* W)
	{
		const FPostProcessSettings* G = W ? W->GetGraphicsPostProcess() : nullptr;
		if (!G) return false;
		for (const FWeightedBlendable& B : G->WeightedBlendables.Array)
			if (const UMaterialInterface* Mat = Cast<UMaterialInterface>(B.Object.Get()))
				if (Mat->GetBlendableLocation(Mat->GetMaterial()) == BL_ReplacingTonemapper) return true;
		return false;
	}
	const TCHAR* Drawn(bool bMaterial, bool bReplaced)
	{
		return bMaterial ? TEXT("look material on") : bReplaced ? TEXT("no look material and the tone pass replaces Unreal's tonemapper: the grade is set but not drawn yet")
			: TEXT("no look material, Unreal's tonemapper draws the grade");
	}
}
namespace SSL = S47StationLookLocal;

bool FS47StationLook::ReadFile(FLook& Out, FOutside& Outside, FString* Error)
{
	auto Fail = [&](const FString& Why) { if (Error) *Error = Why; return false; };
	// the forecourt's defaults, off until the file says so
	FOutside O;
	O.Grade.bEnabled = false;
	O.Grade.Saturation = .88; O.Grade.Contrast = .94;
	O.Grade.Gain = FVector(1, .97, .9);
	O.Grade.OffsetShadows = FVector(.008, .008, .007);
	O.Grade.GainHighlights = FVector(.95, .95, .95);
	O.Grade.Grain = .22; O.Grade.Vignette = .32;
	Outside = O;
	FString Text;
	TSharedPtr<FJsonObject> J;
	if (!FFileHelper::LoadFileToString(Text, *SSL::FilePath())) return Fail(TEXT("no Config/Props/gas_look.json"));
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) || !J.IsValid()) return Fail(TEXT("gas_look.json does not read"));
	FLook L; // the defaults; written to Out only when the whole file reads
	SSL::ReadGrade(J, L);
	L.BlendCm = SSL::Num(J, TEXT("blend_cm"), L.BlendCm);
	L.Priority = SSL::Num(J, TEXT("priority"), L.Priority);
	L.WindowInsetCm = SSL::Num(J, TEXT("window_inset_cm"), L.WindowInsetCm);
	Out = L;
	const TSharedPtr<FJsonObject>* Ext = nullptr;
	if (J->TryGetObjectField(TEXT("exterior"), Ext) && Ext && Ext->IsValid())
	{
		SSL::ReadGrade(*Ext, O.Grade);
		SSL::Vec(*Ext, TEXT("centre"), O.Centre);
		O.RadiusM = SSL::Num(*Ext, TEXT("radius_m"), O.RadiusM);
		O.BlendM = SSL::Num(*Ext, TEXT("blend_m"), O.BlendM);
		Outside = O;
	}
	return true;
}

void FS47StationLook::BoxOf(const FLook& L, FVector& Min, FVector& Max)
{
	Min = S47Three::ToUE(SSL::X0, SSL::Y0, SSL::Z0);
	Max = S47Three::ToUE(SSL::X1, SSL::Y1, SSL::Z1);
	Max.X -= L.WindowInsetCm; // in from the window and door wall, so the blend does not reach the pavement
}

void FS47StationLook::SetOn(bool bEnable)
{
	if (bEnable == bOn) return;
	bOn = bEnable;
	if (UPostProcessComponent* P = Volume.Get()) P->bEnabled = bEnable;
	if (UPostProcessComponent* P = OutVolume.Get()) P->bEnabled = bEnable;
}

void FS47StationLook::Build(UStaticMeshComponent* Station)
{
	BuiltFor = Station;
	AS47Web* W = M ? M->Web.Get() : nullptr;
	UWorld* World = W ? W->GetWorld() : nullptr;
	if (!World) return;
	if (AActor* Old = Actor.Get()) Old->Destroy();
	if (AActor* Old = OutActor.Get()) Old->Destroy();
	Actor = nullptr; OutActor = nullptr;
	Volume = nullptr; OutVolume = nullptr;
	bOn = true;

	FLook L;
	FOutside Out;
	FString Error;
	if (!ReadFile(L, Out, &Error)) UE_LOG(LogS47StationLook, Warning, TEXT("S47: station look: %s, the defaults stand"), *Error);
	const bool bReplaced = SSL::TonemapperReplaced(W);
	const int32 BuildN = Builds++;

	// the shop: a box on the room
	if (!L.bEnabled) { UE_LOG(LogS47StationLook, Display, TEXT("S47: station look: the shop's grade off in gas_look.json")); }
	else if (AActor* A = SSL::SpawnVolume(World, TEXT(""), BuildN))
	{
		FVector Min, Max;
		BoxOf(L, Min, Max);
		UBoxComponent* Box = NewObject<UBoxComponent>(A, TEXT("LookBox"));
		Box->SetBoxExtent(FVector((Max - Min) * .5), false);
		UPostProcessComponent* P = SSL::Volume(A, Box, Station, (Min + Max) * .5, L.Priority, L.BlendCm);
		SSL::Grade(P->Settings, L, true);
		const UMaterialInstanceDynamic* Mid = SSL::LookMaterial(A, P->Settings, L);
		P->RegisterComponent();
		Actor = A;
		Volume = P;
		UE_LOG(LogS47StationLook, Display,
			TEXT("S47: station look: saturation %.2f contrast %.2f gamma %s gain %s shadows +%s highlights %s grain %.2f (size %.2f) vignette %.2f bloom %.2f, blend %.0f cm, priority %.0f; box x %.0f..%.0f y %.0f..%.0f z %.0f..%.0f cm from the station's pivot (window wall pulled in %.0f cm); %s"),
			L.Saturation, L.Contrast, *SSL::V3(L.Gamma), *SSL::V3(L.Gain), *SSL::V3(L.OffsetShadows), *SSL::V3(L.GainHighlights), L.Grain, L.GrainSize, L.Vignette, L.Bloom,
			L.BlendCm, L.Priority, Min.X, Max.X, Min.Y, Max.Y, Min.Z, Max.Z, L.WindowInsetCm, SSL::Drawn(Mid != nullptr, bReplaced));
	}

	// the forecourt: a sphere round the canopy, a wide blend, one priority under the shop's box
	if (!Out.Grade.bEnabled) return;
	if (AActor* A = SSL::SpawnVolume(World, TEXT("Outside"), BuildN))
	{
		const FVector Centre = S47Three::ToUE(Out.Centre);
		USphereComponent* Sphere = NewObject<USphereComponent>(A, TEXT("LookSphere"));
		Sphere->SetSphereRadius(float(Out.RadiusM * 100.0), false);
		UPostProcessComponent* P = SSL::Volume(A, Sphere, Station, Centre, L.Priority - 1, Out.BlendM * 100.0);
		SSL::Grade(P->Settings, Out.Grade, false);
		const UMaterialInstanceDynamic* Mid = SSL::LookMaterial(A, P->Settings, Out.Grade);
		P->RegisterComponent();
		OutActor = A;
		OutVolume = P;
		UE_LOG(LogS47StationLook, Display,
			TEXT("S47: station look outside: saturation %.2f contrast %.2f gamma %s gain %s shadows +%s highlights %s grain %.2f vignette %.2f (bloom as the game has it), sphere %.0f m round (%.0f %.0f %.0f) cm from the station's pivot, blend %.0f m, priority %.0f; %s"),
			Out.Grade.Saturation, Out.Grade.Contrast, *SSL::V3(Out.Grade.Gamma), *SSL::V3(Out.Grade.Gain), *SSL::V3(Out.Grade.OffsetShadows), *SSL::V3(Out.Grade.GainHighlights),
			Out.Grade.Grain, Out.Grade.Vignette, Out.RadiusM, Centre.X, Centre.Y, Centre.Z, Out.BlendM, L.Priority - 1, SSL::Drawn(Mid != nullptr, bReplaced));
	}
}

void FS47StationLook::Step()
{
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W) return;
	const bool bFree = M->IsFreeform();
	if (bFree)
		if (UStaticMeshComponent* Station = AS47Props::PlacedMesh(SSL::StationId); Station && BuiltFor.Get() != Station) Build(Station);
	SetOn(bFree);
}

bool FS47StationLook::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s station look: %s"), bCond ? TEXT("PASS") : TEXT("FAIL"), *What)); bOk &= bCond; };
	FLook L;
	FOutside Out;
	FString Error;
	const bool bRead = ReadFile(L, Out, &Error);
	Check(bRead, bRead ? TEXT("Config/Props/gas_look.json reads") : Error);
	auto In = [](double V, double Lo, double Hi) { return V >= Lo && V <= Hi; };
	auto VecIn = [&](const FVector& V, double Lo, double Hi) { return In(V.X, Lo, Hi) && In(V.Y, Lo, Hi) && In(V.Z, Lo, Hi); };
	auto Sane = [&](const FLook& G) { return In(G.Saturation, .3, 1.5) && In(G.Contrast, .5, 1.5) && VecIn(G.Gamma, .5, 1.5) && VecIn(G.Gain, .5, 1.5)
		&& VecIn(G.OffsetShadows, -.1, .1) && VecIn(G.GainHighlights, .5, 1.5) && In(G.Grain, 0, 1) && In(G.GrainSize, .25, 4) && In(G.Vignette, 0, 1) && In(G.Bloom, 0, 2); };
	const bool bSane = Sane(L) && In(L.BlendCm, 0, 300) && In(L.WindowInsetCm, 0, 200) && In(L.Priority, 2, 1000);
	Check(bSane, bSane ? FString::Printf(TEXT("the shop's values sane: saturation %.2f contrast %.2f gain %s grain %.2f vignette %.2f, blend %.0f cm, priority %.0f"),
		L.Saturation, L.Contrast, *SSL::V3(L.Gain), L.Grain, L.Vignette, L.BlendCm, L.Priority) : TEXT("a value out of range in gas_look.json"));
	// the box: inside the shop's interior (StationDressing's shop in town metres, from the same pivot as gas_cosmos.json has it),
	// its window side at least 10 cm in from the wall, a room-sized volume
	FVector Min, Max;
	BoxOf(L, Min, Max);
	const FVector RoomMin = S47Three::ToUE(SSL::X0, SSL::Y0, SSL::Z0), RoomMax = S47Three::ToUE(SSL::X1, SSL::Y1, SSL::Z1);
	const bool bBox = Min.Equals(RoomMin, .01) && Max.X <= RoomMax.X - 10 && Max.X > Min.X + 500 && FMath::IsNearlyEqual(Max.Y, RoomMax.Y, .01)
		&& FMath::IsNearlyEqual(Max.Z, RoomMax.Z, .01) && FMath::IsNearlyEqual(SSL::Pivot.X + SSL::X0, -26.8, .01)
		&& FMath::IsNearlyEqual(SSL::Pivot.X + SSL::X1, -17.2, .01) && FMath::IsNearlyEqual(SSL::Pivot.Z + SSL::Z1, 2077.78, .01)
		&& FMath::IsNearlyEqual(SSL::Pivot.Y + SSL::Y1, 8.55, .01);
	Check(bBox, FString::Printf(TEXT("box x %.0f..%.0f y %.0f..%.0f z %.0f..%.0f cm from the station's pivot, inside the shop, %.0f cm in from the window wall"),
		Min.X, Max.X, Min.Y, Max.Y, Min.Z, Max.Z, L.WindowInsetCm));
	FVector Origin;
	TArray<FS47StationDressing::FPiece> Pieces;
	if (FS47StationDressing::ReadFile(Origin, Pieces))
		Check(Origin.Equals(SSL::Pivot, .001), TEXT("the pivot is gas_cosmos.json's origin_town"));
	// the forecourt: milder than the shop, a sphere that covers the canopy with a blend wide enough to hide its edge
	if (Out.Grade.bEnabled)
	{
		const bool bOutside = Sane(Out.Grade) && Out.Grade.Saturation >= L.Saturation && Out.Grade.Grain <= L.Grain && Out.Grade.Vignette <= L.Vignette
			&& In(Out.RadiusM, 10, 100) && In(Out.BlendM, 5, 60) && Out.Centre.Size() < 30;
		Check(bOutside, bOutside ? FString::Printf(TEXT("the forecourt's values sane and milder: saturation %.2f grain %.2f vignette %.2f, sphere %.0f m at %s, blend %.0f m"),
			Out.Grade.Saturation, Out.Grade.Grain, Out.Grade.Vignette, Out.RadiusM, *SSL::V3(Out.Centre), Out.BlendM) : TEXT("the forecourt's values out of range or stronger than the shop's"));
	}
	else Lines.Add(TEXT("PASS station look: the forecourt's grade off"));
	// load-if-present: either way is a pass; the line says which
	Lines.Add(FString::Printf(TEXT("PASS station look: look material %s"), L.Material.IsEmpty() ? TEXT("not named (Unreal's settings only)")
		: SSL::Here(L.Material) ? TEXT("here") : TEXT("not here (Unreal's settings only)")));
	return bOk;
}

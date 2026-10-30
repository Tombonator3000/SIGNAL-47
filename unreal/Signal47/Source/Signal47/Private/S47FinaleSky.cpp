#include "S47FinaleSky.h"

#include "S47Main.h"
#include "S47OldRoad.h"
#include "S47OldRoadLayout.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MaterialShared.h"
#include "Misc/PackageName.h"
#include "RHIShaderPlatform.h"
#include "RHIResources.h"
#include "PixelFormat.h"
#include "TextureResource.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogS47FinaleSky, Log, All);

namespace
{
	const TCHAR* CraftPath = TEXT("/Game/FinaleSky/SM_S47_FinalCraft.SM_S47_FinalCraft");
	const TCHAR* CloudPath = TEXT("/Game/FinaleSky/M_S47_FinalCloud.M_S47_FinalCloud");
	TAutoConsoleVariable<float> CVarCraftForward(TEXT("r.S47.Finale.CraftForward"), 1100.f,
		TEXT("Finale craft distance ahead of C8.15, metres (100..10000)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCraftRight(TEXT("r.S47.Finale.CraftRight"), 120.f,
		TEXT("Finale craft offset right of the C8.15 road tangent, metres (-10000..10000)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCraftHeight(TEXT("r.S47.Finale.CraftHeight"), 315.f,
		TEXT("Finale craft height above C8.15, metres (10..10000); existing slow drift is added."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCraftScale(TEXT("r.S47.Finale.CraftScale"), 680.f,
		TEXT("Finale craft uniform authored-mesh scale (10..5000); default gives roughly 680m radius."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCraftFill(TEXT("r.S47.Finale.CraftFill"), .018f,
		TEXT("Very weak original Hull/Ribs material glow (0..0.15); Light and Recess stay authored."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBankForward(TEXT("r.S47.Finale.BankForward"), 920.f,
		TEXT("Finale cloud-bank distance ahead of C8.15, metres (50..10000); existing 90m advance is subtracted."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBankSpread(TEXT("r.S47.Finale.BankSpread"), 495.f,
		TEXT("Finale cloud-bank base distance to each side of the road tangent, metres (0..5000); BankRevealSpread is added over the reveal."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBankRevealSpread(TEXT("r.S47.Finale.BankRevealSpread"), 235.f,
		TEXT("Finale cloud-bank extra distance to each side at full reveal, metres (0..5000)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBankRevealRise(TEXT("r.S47.Finale.BankRevealRise"), 300.f,
		TEXT("Finale cloud-bank vertical rise over the existing reveal, metres (0..2000)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBankHeight(TEXT("r.S47.Finale.BankHeight"), 351.f,
		TEXT("Finale cloud-bank base height above C8.15, metres (10..10000); existing slow roll is added."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBankDepth(TEXT("r.S47.Finale.BankDepth"), 420.f,
		TEXT("Finale cloud-bank forward half-extent, metres (50..2000)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBankWidth(TEXT("r.S47.Finale.BankWidth"), 560.f,
		TEXT("Finale cloud-bank sideways half-extent, metres (50..2000)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBankThickness(TEXT("r.S47.Finale.BankThickness"), 255.f,
		TEXT("Finale cloud-bank vertical half-extent, metres (50..2000)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDensity(TEXT("r.S47.Finale.Density"), 1.f,
		TEXT("Finale baked cloud-volume density multiplier (0.1..12); analytic material ignores an unbound input."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarSamples(TEXT("r.S47.Finale.Samples"), 64,
		TEXT("Finale cloud raymarch samples (16..128); Low effects caps the applied value at 28."), ECVF_Default);
	TAutoConsoleVariable<float> CVarGlow(TEXT("r.S47.Finale.Glow"), 1.f,
		TEXT("Finale cloud glow multiplier (0..10); 1 keeps the authored 0.12..0.34 phase curve."), ECVF_Default);
	TAutoConsoleVariable<float> CVarShade(TEXT("r.S47.Finale.Shade"), .82f,
		TEXT("Maximum view-based native ground-light reduction (0..0.82), faded by finale phase, proximity and pullout height; not a physical cloud shadow."), ECVF_Default);
	double Tune(const TAutoConsoleVariable<float>& Variable, double Min, double Max)
	{
		return FMath::Clamp(double(Variable.GetValueOnGameThread()), Min, Max);
	}
	double Smooth(double A, double B, double X)
	{
		const double V = FMath::Clamp((X - A) / (B - A), 0., 1.);
		return V * V * (3 - 2 * V);
	}
	bool ShaderReady(UMaterialInterface* Material)
	{
		if (!Material || !Material->GetMaterial()) return false;
#if WITH_EDITOR
		UMaterial* Base = Material->GetMaterial();
		const auto* Resource = Base->GetMaterialResource(GMaxRHIShaderPlatform);
		// Uncooked editor runs explicitly request these own maps with EnsureIsComplete.
		// On-demand maps can be complete while IsCompilationFinalized still reports false.
		return Resource && Resource->GetGameThreadShaderMap() && Resource->IsGameThreadShaderMapComplete()
			&& Resource->GetCompileErrors().IsEmpty() && !Base->IsCompilingOrHadCompileError(GMaxRHIShaderPlatform);
#else
		return true;
#endif
	}
	TArray<double> XYZ(const FVector& V) { return { V.X, V.Y, V.Z }; }
}

FS47FinaleSky::FS47FinaleSky(US47Main* InMain) : Main(InMain) {}
FS47FinaleSky::~FS47FinaleSky() { Drop(); }

bool FS47FinaleSky::WaitFor(const FString& Reason)
{
	if (Failure != Reason)
	{
		Failure = Reason;
		UE_LOG(LogS47FinaleSky, Display, TEXT("S47: FinaleSky waiting: %s; original ending continues"), *Reason);
	}
	return false;
}

bool FS47FinaleSky::Prepare()
{
	if (bReady && Craft.IsValid() && Banks[0].IsValid() && Banks[1].IsValid()) return true;
	const double Now = FPlatformTime::Seconds();
	if (Now < NextPrepareTime) return false;
	NextPrepareTime = Now + 1.; // Retry asset/shader readiness without a per-frame load or audit.
	auto* M = Main.Get();
	if (!M || !M->Web || !M->Web->GetRootComponent() || !M->World || !M->World->OldRoad())
		return WaitFor(TEXT("old-road scene"));
	if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(CraftPath)))
		|| !FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(CloudPath))))
		return WaitFor(TEXT("own /Game/FinaleSky assets"));
	auto* Mesh = LoadObject<UStaticMesh>(nullptr, CraftPath, nullptr, LOAD_NoWarn);
	auto* Cloud = LoadObject<UMaterialInterface>(nullptr, CloudPath, nullptr, LOAD_NoWarn);
	auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube || !Mesh || !Mesh->GetRenderData() || !Cloud || !Cloud->GetMaterial())
		return WaitFor(TEXT("craft/cloud render resources"));
	if (Cloud->GetBlendMode() != BLEND_Translucent || Cloud->GetMaterial()->MaterialDomain != MD_Surface)
		return WaitFor(TEXT("surface translucent cloud material"));
#if WITH_EDITOR
	UMaterial* CloudBase = Cloud->GetMaterial();
	if (GShaderCompilingManager) GShaderCompilingManager->ProcessAsyncResults(false, false);
	const int32 JobsBefore = GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : -1;
	bool bEnsureSubmitted = false, bCacheSubmitted = false;
	int32 EnsuredCraftBases = 0;
	if (!bFinaleShadersRequested)
	{
		bFinaleShadersRequested = true; bEnsureSubmitted = true;
		// UE5.8's direct on-demand request submits this material's missing jobs and
		// completes its own compilation. CacheShaders alone can leave a nonfinalized map.
		Cloud->EnsureIsComplete();
		if (!ShaderReady(Cloud))
		{
			// Public wrapper for private CacheResourceShadersForRendering(false).
			FMaterialUpdateContext Update(FMaterialUpdateContext::EOptions::SyncWithRenderingThread);
			Update.AddMaterial(CloudBase);
			CloudBase->CacheShaders(EMaterialShaderPrecompileMode::Default); bCacheSubmitted = true;
		}
		// The four authored opaque slots share their surface base. Request each actual
		// base only once, including when the cloud map was already ready on load.
		TSet<UMaterial*> RequestedBases;
		RequestedBases.Add(CloudBase);
		for (const auto& Slot : Mesh->GetStaticMaterials())
		{
			auto* Material = Slot.MaterialInterface.Get();
			UMaterial* Base = Material ? Material->GetMaterial() : nullptr;
			if (Base && Base->MaterialDomain == MD_Surface && Material->GetBlendMode() == BLEND_Opaque
				&& !RequestedBases.Contains(Base))
			{
				RequestedBases.Add(Base); Base->EnsureIsComplete(); ++EnsuredCraftBases;
			}
		}
		if (GShaderCompilingManager) GShaderCompilingManager->ProcessAsyncResults(false, false);
	}
	if (!bCloudShaderDiagnosed)
	{
		bCloudShaderDiagnosed = true;
		const auto* Resource = CloudBase->GetMaterialResource(GMaxRHIShaderPlatform);
		const auto* Quality = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MaterialQualityLevel"));
		const auto* JobCache = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShaderCompiler.JobCache"));
		const int32 JobsAfter = GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : -1;
		UE_LOG(LogS47FinaleSky, Display, TEXT("S47: FinaleSky cloud shader once material=%s resource=%d shaderMap=%d validFinalized=%d complete=%d compiling=%d errors=%d platform=%d quality=%d jobCache=%d ensure=%d cache=%d craftBases=%d jobsBefore=%d jobsAfter=%d"),
			*GetPathNameSafe(CloudBase), Resource != nullptr, Resource && Resource->GetGameThreadShaderMap(),
			Resource && Resource->HasValidGameThreadShaderMap(), Resource && Resource->IsGameThreadShaderMapComplete(), CloudBase->IsCompiling(),
			Resource ? Resource->GetCompileErrors().Num() : 0, int32(GMaxRHIShaderPlatform),
			Quality ? Quality->GetInt() : -1, JobCache ? JobCache->GetInt() : -1,
			bEnsureSubmitted, bCacheSubmitted, EnsuredCraftBases, JobsBefore, JobsAfter);
	}
#endif
	if (!ShaderReady(Cloud)) return WaitFor(TEXT("cloud shader"));
	if (Mesh->GetStaticMaterials().IsEmpty()) return WaitFor(TEXT("opaque craft materials"));
	for (const auto& Slot : Mesh->GetStaticMaterials())
	{
		auto* Material = Slot.MaterialInterface.Get();
		if (!Material || !Material->GetMaterial() || Material->GetMaterial()->MaterialDomain != MD_Surface
			|| Material->GetBlendMode() != BLEND_Opaque || !ShaderReady(Material))
			return WaitFor(TEXT("opaque craft shaders"));
	}
	Drop();
	const auto* Road = M->World->OldRoad();
	// This is C8.15, never the diner's mile-zero junction. Derive orientation at C itself.
	const double CrossS = S47OldRoadLayout::SAt(8.15);
	Crossing = Road->Lane(CrossS);
	ForwardUE = S47Three::DirToUE(Road->Lane(CrossS + 20) - Road->Lane(CrossS - 20)).GetSafeNormal2D();
	RightUE = FVector::CrossProduct(FVector::UpVector, ForwardUE).GetSafeNormal();
	if (ForwardUE.IsNearlyZero()) return WaitFor(TEXT("crossing tangent"));
	const auto Make = [&](const TCHAR* Name, UStaticMesh* Asset)
	{
		auto* C = NewObject<UStaticMeshComponent>(M->Web,
			MakeUniqueObjectName(M->Web, UStaticMeshComponent::StaticClass(), FName(Name)));
		C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(M->Web->GetRootComponent()); C->SetStaticMesh(Asset);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetGenerateOverlapEvents(false);
		C->SetCanEverAffectNavigation(false); C->SetCastShadow(false); C->bAffectDistanceFieldLighting = false;
		C->bVisibleInReflectionCaptures = false; C->bVisibleInRealTimeSkyCaptures = false;
		C->SetVisibleInRayTracing(false); C->SetVisibility(false); C->RegisterComponent();
		return C;
	};
	Craft = Make(TEXT("SC13OriginalFinalCraft"), Mesh);
	for (int32 I = 0; I < Mesh->GetStaticMaterials().Num(); ++I)
	{
		auto* Parent = Mesh->GetStaticMaterials()[I].MaterialInterface.Get();
		const FName Name = Parent ? Parent->GetFName() : NAME_None;
		const int32 Fill = Name == FName(TEXT("MI_S47_FinalHull")) ? 0 : Name == FName(TEXT("MI_S47_FinalRibs")) ? 1 : INDEX_NONE;
		if (Fill == INDEX_NONE) continue;
		auto* Mid = UMaterialInstanceDynamic::Create(Parent, Craft.Get());
		CraftFillMids[Fill] = Mid; Craft->SetMaterial(I, Mid);
	}
	for (int32 I = 0; I < 2; ++I)
	{
		Banks[I] = Make(I ? TEXT("SC13CloudBankRight") : TEXT("SC13CloudBankLeft"), Cube);
		// Render one exit face per ray, including when the camera is inside a bank.
		Banks[I]->SetReverseCulling(true);
		auto* Mid = UMaterialInstanceDynamic::Create(Cloud, Banks[I].Get());
		BankMids[I] = Mid; Banks[I]->SetMaterial(0, Mid); Banks[I]->SetTranslucentSortPriority(-2 + I);
	}
	bReady = true; Failure.Reset();
	const FVector At = S47Three::ToUE(Crossing);
	UE_LOG(LogS47FinaleSky, Display, TEXT("S47: FinaleSky ready at C8.15 UE=(%.1f,%.1f,%.1f) forward=(%.4f,%.4f,%.4f)"),
		At.X, At.Y, At.Z, ForwardUE.X, ForwardUE.Y, ForwardUE.Z);
	return true;
}

void FS47FinaleSky::Hide()
{
	bShown = false;
	GroundShade = 0;
	if (auto* M = Main.Get()) if (M->Web) M->Web->SetFinaleGroundShade(0);
	if (Craft.IsValid()) Craft->SetVisibility(false);
	for (auto& B : Banks) if (B.IsValid()) B->SetVisibility(false);
}

void FS47FinaleSky::Drop()
{
	Hide();
	if (Craft.IsValid()) Craft->DestroyComponent(); Craft.Reset();
	for (auto& Mid : CraftFillMids) Mid.Reset();
	for (int32 I = 0; I < 2; ++I)
	{
		if (Banks[I].IsValid()) Banks[I]->DestroyComponent();
		Banks[I].Reset(); BankMids[I].Reset();
	}
	bReady = false;
}

void FS47FinaleSky::Step(double Mile, const FString& Stage, double E, double P)
{
	auto* M = Main.Get();
	if (!M || !M->World || M->World->Area != TEXT("roswell") || Stage == TEXT("end")) { Hide(); return; }
	// No independent clock: a paused or restored C6 supplies exactly the same cloud field.
	Phase = Stage == TEXT("drive") ? -80 * (1 - Smooth(5.9, 8.15, Mile)) : Stage == TEXT("event") ? E : 47 + P;
	Growth = Smooth(-80, 6, Phase); Reveal = Smooth(5, 21, Phase);
	if (!Prepare()) return;
	Tuning.CraftForward = Tune(CVarCraftForward, 100, 10000);
	Tuning.CraftRight = Tune(CVarCraftRight, -10000, 10000);
	Tuning.CraftHeight = Tune(CVarCraftHeight, 10, 10000);
	Tuning.CraftScale = Tune(CVarCraftScale, 10, 5000);
	Tuning.CraftFill = Tune(CVarCraftFill, 0, .15);
	for (auto& Mid : CraftFillMids) if (Mid.IsValid()) Mid->SetScalarParameterValue(TEXT("Glow"), Tuning.CraftFill);
	Tuning.BankForward = Tune(CVarBankForward, 50, 10000);
	Tuning.BankSpread = Tune(CVarBankSpread, 0, 5000);
	Tuning.BankRevealSpread = Tune(CVarBankRevealSpread, 0, 5000);
	Tuning.BankRevealRise = Tune(CVarBankRevealRise, 0, 2000);
	Tuning.BankHeight = Tune(CVarBankHeight, 10, 10000);
	Tuning.BankDepth = Tune(CVarBankDepth, 50, 2000);
	Tuning.BankWidth = Tune(CVarBankWidth, 50, 2000);
	Tuning.BankThickness = Tune(CVarBankThickness, 50, 2000);
	Tuning.Density = Tune(CVarDensity, .1, 12);
	Tuning.Glow = Tune(CVarGlow, 0, 10);
	Tuning.Shade = Tune(CVarShade, 0, .82);
	Tuning.Samples = FMath::Clamp(CVarSamples.GetValueOnGameThread(), 16, 128);
	GlowLevel = Tuning.Glow * (.12 + .22 * Smooth(0, 17, Phase));
	bLow = M->Web->GetGraphicsSettings().Effects == 0;
	AppliedSamples = bLow ? FMath::Min(Tuning.Samples, 28) : Tuning.Samples;
	bShown = Growth > .001;
	const FVector Base = S47Three::ToUE(Crossing), Up = FVector::UpVector;
	const FVector Eye = M->Player ? M->Player->EyeUE() : Base;
	const double DistanceM = FVector::Dist2D(Eye, Base) * .01;
	const double HeightM = FMath::Max(0., (Eye.Z - Base.Z) * .01);
	// This is view-based negative fill, not a world-space shadow. Preserve the sunrise
	// as the existing pullout rises above the scene; headlights and dash keep their own light.
	GroundShade = Tuning.Shade * Smooth(-45, 0, Phase) * (1 - Smooth(1200, 4500, DistanceM))
		* (1 - Smooth(90, 400, HeightM));
	M->Web->SetFinaleGroundShade(float(GroundShade));
	// The hull stays opaque. The finite cloud volumes physically hide and uncover it.
	const FVector CraftAt = Base + ForwardUE * (100 * Tuning.CraftForward) + RightUE * (100 * Tuning.CraftRight)
		+ Up * (100 * Tuning.CraftHeight + 160 * FMath::Sin(Phase * .024));
	Craft->SetWorldLocation(CraftAt); Craft->SetWorldScale3D(FVector(Tuning.CraftScale));
	Craft->SetWorldRotation(FRotationMatrix::MakeFromXZ(ForwardUE, Up).Rotator() + FRotator(0, 0, -1.6));
	// Establish the foreground clouds during the drive before submitting the opaque hull.
	// From the stop onward only cloud coverage changes its visibility to the camera.
	Craft->SetVisibility(bShown && Phase >= 0);
	for (int32 I = 0; I < 2; ++I)
	{
		const double Side = I ? 1. : -1.;
		const FVector Extent = FVector(Tuning.BankDepth, Tuning.BankWidth, Tuning.BankThickness) * 100;
		FVector Center = Base + ForwardUE * (100 * Tuning.BankForward - 9000 * Smooth(-65, 22, Phase))
			+ RightUE * (Side * 100 * (Tuning.BankSpread + Tuning.BankRevealSpread * Reveal))
			+ Up * (100 * Tuning.BankHeight + 1800 * FMath::Sin(Phase * .018 + I));
		Center.Z += 100 * Tuning.BankRevealRise * Reveal;
		auto* B = Banks[I].Get(); auto* D = BankMids[I].Get();
		if (!B || !D) continue;
		B->SetWorldLocation(Center); B->SetWorldRotation(FRotationMatrix::MakeFromXZ(ForwardUE, Up).Rotator());
		B->SetWorldScale3D(Extent / 50.); B->SetVisibility(bShown);
		D->SetVectorParameterValue(TEXT("BankCenter"), FLinearColor(Center.X, Center.Y, Center.Z, 0));
		D->SetVectorParameterValue(TEXT("BankExtent"), FLinearColor(Extent.X, Extent.Y, Extent.Z, 0));
		D->SetVectorParameterValue(TEXT("Forward"), FLinearColor(ForwardUE.X, ForwardUE.Y, ForwardUE.Z, 0));
		D->SetVectorParameterValue(TEXT("Right"), FLinearColor(RightUE.X, RightUE.Y, RightUE.Z, 0));
		D->SetScalarParameterValue(TEXT("Phase"), Phase); D->SetScalarParameterValue(TEXT("Growth"), Growth);
		D->SetScalarParameterValue(TEXT("Side"), Side); D->SetScalarParameterValue(TEXT("Samples"), AppliedSamples);
		D->SetScalarParameterValue(TEXT("Glow"), GlowLevel);
		D->SetScalarParameterValue(TEXT("Density"), Tuning.Density);
	}
	const int32 Band = Phase < 0 ? 0 : Phase < 6 ? 1 : Phase < 21 ? 2 : Phase < 47 ? 3 : 4;
	if (LastStage != Stage || LastPhaseBand != Band)
	{
		UE_LOG(LogS47FinaleSky, Display, TEXT("S47: FinaleSky placed stage=%s phase=%.2f growth=%.3f reveal=%.3f shown=%d samples=%d"),
			*Stage, Phase, Growth, Reveal, bShown, AppliedSamples);
		LastStage = Stage; LastPhaseBand = Band;
	}
}

S47J::FObj FS47FinaleSky::Readback() const
{
	auto O = S47J::New();
	S47J::Set(O, TEXT("ready"), bReady); S47J::Set(O, TEXT("shown"), bShown); S47J::Set(O, TEXT("failure"), Failure);
	S47J::Set(O, TEXT("phase"), Phase); S47J::Set(O, TEXT("growth"), Growth); S47J::Set(O, TEXT("reveal"), Reveal);
	S47J::Set(O, TEXT("samples"), AppliedSamples); S47J::Set(O, TEXT("craftOpacity"), 1.);
	auto Applied = S47J::New();
	S47J::Set(Applied, TEXT("craftForwardM"), Tuning.CraftForward); S47J::Set(Applied, TEXT("craftRightM"), Tuning.CraftRight);
	S47J::Set(Applied, TEXT("craftHeightM"), Tuning.CraftHeight); S47J::Set(Applied, TEXT("craftScale"), Tuning.CraftScale);
	S47J::Set(Applied, TEXT("craftFill"), Tuning.CraftFill);
	S47J::Set(Applied, TEXT("bankForwardM"), Tuning.BankForward); S47J::Set(Applied, TEXT("bankSpreadM"), Tuning.BankSpread);
	S47J::Set(Applied, TEXT("bankRevealSpreadM"), Tuning.BankRevealSpread);
	S47J::Set(Applied, TEXT("bankRevealRiseM"), Tuning.BankRevealRise);
	S47J::Set(Applied, TEXT("bankDepthM"), Tuning.BankDepth); S47J::Set(Applied, TEXT("bankWidthM"), Tuning.BankWidth);
	S47J::Set(Applied, TEXT("bankThicknessM"), Tuning.BankThickness); S47J::Set(Applied, TEXT("density"), Tuning.Density);
	S47J::Set(Applied, TEXT("bankHeightM"), Tuning.BankHeight); S47J::Set(Applied, TEXT("glowMultiplier"), Tuning.Glow);
	S47J::Set(Applied, TEXT("shadeMaximum"), Tuning.Shade); S47J::Set(Applied, TEXT("groundShade"), GroundShade);
	S47J::Set(Applied, TEXT("requestedSamples"), Tuning.Samples); S47J::Set(Applied, TEXT("samples"), AppliedSamples);
	S47J::Set(Applied, TEXT("glowLevel"), GlowLevel); S47J::Set(O, TEXT("tuning"), Applied);
	// Inspect only textures already bound to the actual bank material. Never load or wait here.
	UMaterialInterface* BankMaterial = Banks[0].IsValid() ? Banks[0]->GetMaterial(0)
		: Banks[1].IsValid() ? Banks[1]->GetMaterial(0) : nullptr;
	float DecodeScale = 0;
	if (BankMaterial && BankMaterial->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("DecodeScale")), DecodeScale))
		S47J::Set(O, TEXT("decodeScale"), double(DecodeScale));
	else S47J::SetNull(O, TEXT("decodeScale"));
	TArray<TSharedPtr<FJsonValue>> DensityTextures;
	for (const TCHAR* Parameter : { TEXT("DensityA"), TEXT("DensityB"), TEXT("DensityC") })
	{
		UTexture* Texture = nullptr;
		if (!BankMaterial || !BankMaterial->GetTextureParameterValue(FHashedMaterialParameterInfo(Parameter), Texture) || !Texture) continue;
		auto Row = S47J::New();
		S47J::Set(Row, TEXT("parameter"), Parameter); S47J::Set(Row, TEXT("texture"), GetPathNameSafe(Texture));
		const FTextureResource* Resource = Texture->GetResource();
		const FRHITexture* GPU = Resource && Resource->TextureRHI.IsValid() ? Resource->TextureRHI.GetReference() : nullptr;
		S47J::Set(Row, TEXT("gpuResourceAvailable"), GPU != nullptr);
		if (GPU)
		{
			S47J::Set(Row, TEXT("gpuFormat"), GetPixelFormatString(GPU->GetFormat()));
			S47J::Set(Row, TEXT("gpuFormatIndex"), int32(GPU->GetFormat()));
			if (GPU->GetDesc().Dimension == ETextureDimension::Texture3D)
			{
				const FIntVector Size = GPU->GetSizeXYZ();
				S47J::Set(Row, TEXT("dimensions3D"), TArray<double>{ double(Size.X), double(Size.Y), double(Size.Z) });
			}
		}
		DensityTextures.Add(MakeShared<FJsonValueObject>(Row));
	}
	O->SetArrayField(TEXT("densityTextures"), DensityTextures);
	S47J::Set(O, TEXT("crossingMile"), 8.15); S47J::Set(O, TEXT("crossingUE"), XYZ(S47Three::ToUE(Crossing)));
	S47J::Set(O, TEXT("forwardUE"), XYZ(ForwardUE)); S47J::Set(O, TEXT("rightUE"), XYZ(RightUE));
	if (Craft.IsValid())
	{
		S47J::Set(O, TEXT("craftUE"), XYZ(Craft->GetComponentLocation()));
		S47J::Set(O, TEXT("craftShown"), Craft->IsVisible());
	}
	TArray<TSharedPtr<FJsonValue>> Parts;
	for (int32 I = 0; I < 2; ++I)
	{
		auto B = S47J::New(); S47J::Set(B, TEXT("side"), I ? 1 : -1);
		if (Banks[I].IsValid()) S47J::Set(B, TEXT("centerUE"), XYZ(Banks[I]->GetComponentLocation()));
		Parts.Add(MakeShared<FJsonValueObject>(B));
	}
	O->SetArrayField(TEXT("banks"), Parts);
	return O;
}

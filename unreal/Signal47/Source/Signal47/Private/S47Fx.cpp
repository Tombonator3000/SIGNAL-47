#include "S47Fx.h"
#include "S47Web.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Fx, Log, All);

bool S47Fx::Off() { return FParse::Param(FCommandLine::Get(), TEXT("S47FxOff")); }

UNiagaraComponent* S47Fx::Make(AS47Web* W, USceneComponent* Parent, const TCHAR* Path, const TCHAR* Name)
{
	if (!W || !Parent) return nullptr;
	if (Off()) { UE_LOG(LogS47Fx, Display, TEXT("S47: fx %s off (-S47FxOff); inert"), Name); return nullptr; }
	// A scoped failure injection exercises the same null-asset path in packaged checks.
	const bool bMissingProbe = FParse::Param(FCommandLine::Get(), TEXT("S47ArcMissingAsset"));
	UNiagaraSystem* S = bMissingProbe ? nullptr : LoadObject<UNiagaraSystem>(nullptr, Path);
	if (!S || !S->IsValid() || !S->IsReadyToRun())
	{ UE_LOG(LogS47Fx, Warning, TEXT("S47: fx %s absent or not ready (%s); inert"), Name, Path); return nullptr; }
	UNiagaraComponent* C = NewObject<UNiagaraComponent>(W, Name);
	C->SetAsset(S); C->SetAutoActivate(false); C->SetAutoDestroy(false);
	C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(Parent);
	// Niagara sprites enter ray tracing by default (r.RayTracing.Geometry.NiagaraSprites); RT memory sits near its 80 MiB line.
	C->SetCastShadow(false); C->SetVisibleInRayTracing(false); C->bAffectDistanceFieldLighting = false; C->SetReceivesDecals(false);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCanEverAffectNavigation(false);
	C->RegisterComponent();
	// Install runs behind the world loading screen. Request renderer PSOs and initialize
	// the CPU emitter invisibly here, rather than first allocating it at the scare.
	C->PrecachePSOs(); C->SetVisibility(false); C->Activate(true);
	C->AdvanceSimulation(2, 1.f / 60.f); C->DeactivateImmediate(); C->SetVisibility(true);
	UE_LOG(LogS47Fx, Display, TEXT("S47: fx %s ready (%s); ray tracing off, no shadow, no collision"), Name, Path);
	return C;
}

bool S47Fx::Safe(const UNiagaraComponent* C)
{
	return C && !C->bVisibleInRayTracing && !C->CastShadow && !C->bAffectDistanceFieldLighting && !C->CanEverAffectNavigation() && C->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !C->bAutoActivate;
}

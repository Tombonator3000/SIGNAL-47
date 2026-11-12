#include "S47StationInsects.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "NiagaraComponent.h"
#include "S47Flow.h"
#include "S47Fx.h"
#include "S47Main.h"
#include "S47NightLife.h"
#include "S47Props.h"
#include "S47Soundscape.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Weather.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47WorldLife.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47StationInsects, Log, All);

// A named namespace, not an anonymous one: unity builds put several files in one translation unit.
namespace S47StationInsectsLocal
{
	const TCHAR* InsectSystem = TEXT("/Game/Signal47/FX/NS_S47_Insects.NS_S47_Insects");
	const TCHAR* CanopyId = TEXT("gasCanopyTubes");
	// Config/Props/town.json gasCanopyTubes: its light's centre in the model's own terms (three metres), 16.2 m along z by 4.5 m
	// across x, facing down. The swarm's box hangs 30 cm under it: Extent is its half-size in centimetres (x across, y along, z down).
	const FVector TubeAt(6.406, 4.68 - .30, -9.907);
	const FVector Extent(225, 810, 150);
	// The old canopy lamps of the web export lie 3 m either side of the tubes' centre; nothing else of the town within 9 m.
	constexpr double CedeRadius = 9;
	// The light the specks catch: pale, a little warm, low. s47.insects.Gain scales it live for tuning against the fixed exposure.
	const FLinearColor Tint(1.f, .84f, .62f, 1.f);
	TAutoConsoleVariable<float> CVarInsectGain(TEXT("s47.insects.Gain"), 1.5f, TEXT("GAS-STATION-LIFT-01: the canopy insects' and motes' brightness, times (1.5 default)."));
	const FName UCount(TEXT("User.Count")), UMotes(TEXT("User.Motes")), UExtent(TEXT("User.Extent")), UWind(TEXT("User.Wind")), UTint(TEXT("User.Tint"));
}
namespace SIL = S47StationInsectsLocal;

double FS47StationInsects::InsectRate(const FIn& In)
{
	if (In.Lit <= 0 || In.Moth <= .01) return 0;
	const double Calm = 1 - FMath::SmoothStep(.2, .8, In.Wind);
	const double Dry = 1 - FMath::SmoothStep(.05, .5, In.Rain);
	const double Here = 1 - FMath::Clamp(In.Quiet, 0., 1.);
	return MaxRate * FMath::Clamp(In.Lit, 0., 1.) * FMath::Clamp(In.Moth, 0., 1.) * Calm * Dry * Here * (In.bLow ? .5 : 1.);
}

double FS47StationInsects::MoteRate(const FIn& In)
{
	if (In.Lit <= 0) return 0;
	const double Blow = FMath::SmoothStep(.08, .45, In.Wind);
	const double Dry = 1 - FMath::SmoothStep(.05, .5, In.Rain);
	return MaxMotes * Blow * Dry * (.6 + .4 * FMath::Clamp(In.Dust, 0., 1.)) * (In.bLow ? .5 : 1.);
}

void FS47StationInsects::Reset()
{
	SetOn(false, TEXT("out of Freeform"));
	Quiet = 0;
}

void FS47StationInsects::SetOn(bool bNow, const TCHAR* Why)
{
	if (bNow == bOn) return;
	bOn = bNow;
	UNiagaraComponent* C = Fx.Get();
	if (!C) return;
	if (bNow) C->Activate(true);
	else C->DeactivateImmediate();
	UE_LOG(LogS47StationInsects, Display, TEXT("S47: station insects %s (%s)"), bNow ? TEXT("on") : TEXT("off"), Why);
}

void FS47StationInsects::Build(UStaticMeshComponent* Canopy)
{
	BuiltFor = Canopy;
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W || !W->GetRootComponent()) return;
	if (UNiagaraComponent* Old = Fx.Get()) Old->DestroyComponent();
	bOn = false;
	UNiagaraComponent* C = S47Fx::Make(W, W->GetRootComponent(), SIL::InsectSystem, *FString::Printf(TEXT("StationInsectsFx%d"), Builds++));
	Fx = C;
	if (!C) return; // Make has said why; the night life's moths at the canopy stay
	// The canopy does not move: placed once, with its rotation, so the box's x runs across the canopy and y along the tubes.
	const FTransform T = Canopy->GetComponentTransform();
	const FVector At = T.TransformPosition(S47Three::ToUE(SIL::TubeAt));
	C->SetWorldLocationAndRotation(At, T.GetRotation());
	C->SetVariableVec3(SIL::UExtent, SIL::Extent);
	C->SetVariableLinearColor(SIL::UTint, SIL::Tint);
	C->SetVariableFloat(SIL::UCount, 0.f);
	C->SetVariableFloat(SIL::UMotes, 0.f);
	C->SetVariableVec3(SIL::UWind, FVector::ZeroVector);
	if (!bCeded && M->Life && M->Life->Life)
	{
		bCeded = true;
		const int32 N = M->Life->Life->Cede(TEXT("road"), S47Three::ToThree(At), SIL::CedeRadius);
		UE_LOG(LogS47StationInsects, Display, TEXT("S47: station insects under the canopy at (%.1f, %.1f, %.1f) three; %d of the night life's lamps handed over"), S47Three::ToThree(At).X, S47Three::ToThree(At).Y, S47Three::ToThree(At).Z, N);
	}
}

void FS47StationInsects::Step(double Dt)
{
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W || !M->IsFreeform() || M->Mode != US47Main::EMode::Play) { if (bOn) Reset(); return; }
	UStaticMeshComponent* Canopy = AS47Props::PlacedMesh(SIL::CanopyId);
	if (!Canopy) { SetOn(false, TEXT("no canopy")); return; }
	if (BuiltFor.Get() != Canopy) Build(Canopy);
	UNiagaraComponent* C = Fx.Get();
	if (!C) return;

	// The hush: no more come within a second and a half; after, they come back over ten.
	const bool bHush = M->Soundscape && M->Soundscape->Hushed();
	if (FMath::IsFinite(Dt) && Dt > 0) Quiet = bHush ? FMath::Min(1., Quiet + Dt / 1.5) : FMath::Max(0., Quiet - Dt / 10.);

	FIn In;
	In.Lit = FMath::Max(0.f, AS47Props::LitLevel(SIL::CanopyId));
	In.Moth = M->Life && M->Life->Life ? M->Life->Life->Activity().Moth : 1.;
	In.Quiet = Quiet;
	In.bLow = M->Flow && (M->Flow->Quality == TEXT("low") || M->Flow->Graphics.Effects == 0);
	const S47Weather::FLive& Live = S47Weather::Live();
	if (Live.bOn) { In.Wind = Live.Wind; In.Rain = Live.Rain; In.Dust = Live.Dust; }

	// Where the eye is: the camera (the director's, a still's), else the player's eyes.
	FVector Eye = M->Player ? M->Player->EyeUE() : C->GetComponentLocation();
	if (const UWorld* World = W->GetWorld())
		if (const APlayerController* Pc = World->GetFirstPlayerController())
			if (Pc->PlayerCameraManager) Eye = Pc->PlayerCameraManager->GetCameraLocation();
	const bool bNear = FVector::DistSquared(Eye, C->GetComponentLocation()) < FMath::Square(FarM * 100.);
	// (the world's own copy of the area: AS47Web::Area() returns a new string each call)
	const bool bTown = M->World && M->World->Area == TEXT("diner");
	const TCHAR* Why = In.Lit <= 0 ? TEXT("the tubes are dark") : In.Moth <= .01 ? TEXT("day") : !bTown ? TEXT("another area") : !bNear ? TEXT("far") : TEXT("lit, near");
	SetOn(In.Lit > 0 && In.Moth > .01 && bTown && bNear, Why);
	if (!bOn) return;

	C->SetVariableFloat(SIL::UCount, float(InsectRate(In)));
	C->SetVariableFloat(SIL::UMotes, float(MoteRate(In)));
	// The wind in the box's own terms (three's x east, z south), slow enough to read as drift, not streaks.
	const FVector Blow = S47Three::DirToUE(FVector(Live.DirX, 0, Live.DirZ)).GetSafeNormal2D() * (In.Wind * WindCmS);
	C->SetVariableVec3(SIL::UWind, C->GetComponentTransform().InverseTransformVectorNoScale(Blow).GetClampedToMaxSize(WindCmS));
	C->SetVariableLinearColor(SIL::UTint, SIL::Tint * (SIL::CVarInsectGain.GetValueOnGameThread() * float(In.Lit)));
}

bool FS47StationInsects::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s station insects: %s"), bCond ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= bCond; };
	const FIn Night;
	FIn Low = Night; Low.bLow = true;
	FIn Dark = Night; Dark.Lit = 0;
	FIn Day = Night; Day.Moth = 0;
	FIn Gale = Night; Gale.Wind = .85;
	FIn Breeze = Night; Breeze.Wind = .4;
	FIn Shower = Night; Shower.Rain = .6; Shower.Wind = .4;
	FIn Hush = Night; Hush.Quiet = 1;
	FIn Cool = Night; Cool.Moth = .3;
	Check(FMath::IsNearlyEqual(InsectRate(Night), MaxRate) && FMath::IsNearlyEqual(InsectRate(Low), MaxRate * .5), TEXT("a warm calm night under lit tubes: the full swarm, half on Low"));
	Check(InsectRate(Dark) == 0 && InsectRate(Day) == 0 && MoteRate(Dark) == 0, TEXT("none when the tubes are dark (a power cut, the dawn) or by day"));
	Check(InsectRate(Gale) == 0 && InsectRate(Breeze) > 0 && InsectRate(Breeze) < MaxRate && InsectRate(Shower) == 0, TEXT("thinned by wind, none in a strong wind or in rain"));
	Check(InsectRate(Hush) == 0 && InsectRate(Cool) < InsectRate(Night) * .5, TEXT("none in the night's hush; fewer as the night cools"));
	Check(MoteRate(Night) == 0 && MoteRate(Breeze) > 0 && MoteRate(Shower) == 0 && MoteRate(Breeze) <= MaxMotes, TEXT("motes only in the wind, none in rain"));
	return bOk;
}

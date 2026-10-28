#pragma once

#include "CoreMinimal.h"

class AS47Web;
class USceneComponent;
class UNiagaraComponent;
class UPrimitiveComponent;

// Niagara effects in the native pattern: C++ owns state, time, light, counts and seeds;
// the system only draws. Assets live under /Game/Signal47/FX and are made by Tools/fx.
// Nothing here touches sg.EffectsQuality: that group also sets r.MaterialQualityLevel.
namespace S47Fx
{
	// -S47FxOff: every Niagara effect stays inert, for A/B timing in the same build.
	bool Off();
	// Scoped native FX precache; existing startup queues wait for these component requests.
	void Precache(UPrimitiveComponent* C);
	// A deactivated component under Parent: no ray tracing, no shadow, no collision, no
	// distance field lighting. Returns nullptr (and logs) when off or when the asset is absent.
	UNiagaraComponent* Make(AS47Web* W, USceneComponent* Parent, const TCHAR* Path, const TCHAR* Name);
	// The flags Make sets, read back from a live component (tests and logs).
	bool Safe(const UNiagaraComponent* C);
}

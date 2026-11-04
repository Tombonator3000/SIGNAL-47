#pragma once
#include "CoreMinimal.h"
#include "Math/RandomStream.h"

// Fluorescent tube behaviour shared by every tube light in the game (fixture, lab and east wing).
// Feed the logical target level each update; Apply returns the level to render:
//  - switched ON after being dark >= 0.6 s: the starter strikes 1-3 times before the tube holds;
//  - switched OFF after being lit >= 1 s: a brief die-off glow and one last blip.
// Shorter changes (authored story flicker, brownout blips) pass through untouched, and the very
// first update never flickers, so loading a save or entering an area shows steady light.
struct FS47TubeFlicker
{
	float Apply(double Now, float TargetK, uint32 Salt = 0)
	{
		const bool On = TargetK > 1e-4f;
		if (!bInit) { bInit = true; bOn = On; Changed = Now - 100.0; Start = -100.0; LastOnK = TargetK; return TargetK; }
		if (On != bOn)
		{
			const double Held = Now - Changed;
			bOn = On; Changed = Now;
			// Only a real switch event restarts the ballast; fast authored flicker is left alone.
			if ((On && Held >= .6) || (!On && Held >= 1.0)) { Start = Now; Seed = Seed * 1664525u + 1013904223u + Salt; }
			else Start = -100.0;
		}
		if (On) LastOnK = TargetK;
		const double T = Now - Start;
		if (On)
		{
			FRandomStream R(int32(Seed & 0x7fffffff));
			double At = .05 + R.FRand() * .08;
			const int32 Strikes = R.RandRange(1, 3);
			for (int32 I = 0; I < Strikes; ++I)
			{
				const double Len = .03 + R.FRand() * .05, Gap = .07 + R.FRand() * .16;
				const float Level = .45f + R.FRand() * .5f;
				if (T < At) return 0.f;
				if (T < At + Len) return TargetK * Level;
				At += Len + Gap;
			}
			if (T < At) return 0.f;
			if (T < At + .18) return TargetK * float(.7 + .3 * (T - At) / .18);  // tube warming to full output
			return TargetK;
		}
		if (T < .05) return LastOnK * .55f;
		if (T > .09 && T < .12) return LastOnK * .28f;
		return 0.f;
	}
	bool Active(double Now) const { return Now - Start < 1.5; }

private:
	bool bInit = false, bOn = false;
	double Changed = 0, Start = -100.0;
	uint32 Seed = 0x5a17u;
	float LastOnK = 0.f;
};

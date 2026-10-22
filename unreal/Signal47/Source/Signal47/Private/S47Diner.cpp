#include "S47Diner.h"

#include "S47Main.h"
#include "S47Three.h"
#include "S47Web.h"
#include <limits>

FS47Diner::FS47Diner(US47Main* InM) : M(InM)
{
	if (!M || !M->Web) return;
	Group = M->Web->Obj(TEXT("world.diner.group")); Interior = M->Web->Obj(TEXT("world.diner.interior"));
	Flood = M->Web->FloodSet(TEXT("diner"));
	// Diner.ts constructor: geometry, the people and proxies come from the web export.
	SetDawn(0.25);
}
FS47Obj FS47Diner::Obj(const FString& Id) const { return M && M->Web ? M->Web->Obj(TEXT("world.diner.objs.") + Id) : FS47Obj(); }
FS47Obj FS47Diner::Proxy(const FString& Id) const { return M && M->Web ? M->Web->Obj(TEXT("world.diner.proxies.") + Id) : FS47Obj(); }

void FS47Diner::SetSurfaceArt(UTexture* Booth, UTexture* Wall)
{
	if (!M || !M->Web) return;
	M->Web->Material(TEXT("world.diner.m.vinyl")).SetMap(Booth);
	M->Web->Material(TEXT("world.diner.m.panel")).SetMap(Wall);
}

// Diner.ts setSignLit: the same two neon slots and the shared sign paint.
void FS47Diner::SetSignLit(bool bOn)
{
	bSignLit = bOn; if (!M || !M->Web) return;
	M->Web->SetFlood(Flood, 0, bOn ? 8 : 0); M->Web->SetFlood(Flood, 1, bOn ? 4 : 0);
	const FS47Mat Sign = M->Web->Material(TEXT("world.diner.m.sign"));
	Sign.SetEmissive(S47Three::Hex(0xffffff), bOn ? 0.65f : 0); Sign.SetColor(FLinearColor(bOn ? 1.f : 0.32f, bOn ? 1.f : 0.32f, bOn ? 1.f : 0.32f));
}

void FS47Diner::SetDawn(double K)
{
	Dawn = FMath::Clamp(FMath::IsFinite(K) ? K : 0.0, 0.0, 1.0); if (!M || !M->Web) return;
	M->Web->SetFlood(Flood, 8, float(Dawn * 8));
	// Unreal represents HemisphereLight through the area's environment parameters.
	M->Web->SetHemi(TEXT("diner"), FMath::Lerp(S47Three::Hex(0x6d87a4), S47Three::Hex(0xb0bbc2), float(Dawn)), S47Three::Hex(0x55402f), float((0.55 + Dawn * 0.3) * Near));
	const FS47Mat Glass = M->Web->Material(TEXT("world.diner.m.glass"));
	Glass.SetColor(FMath::Lerp(S47Three::Hex(0x627584), S47Three::Hex(0xc6c6b6), float(Dawn)));
	Glass.SetEmissive(S47Three::Hex(0x8fabc2), float(Dawn * 0.06));
}

void FS47Diner::SetNear(double W)
{
	Near = FMath::Clamp(W, 0.0, 1.0);
	if (M && M->Web) M->Web->SetHemi(TEXT("diner"), FMath::Lerp(S47Three::Hex(0x6d87a4), S47Three::Hex(0xb0bbc2), float(Dawn)), S47Three::Hex(0x55402f), float((0.55 + Dawn * 0.3) * Near));
}

// Diner.ts shine: the driven truck's three headlight pools, colour scaled by K.
void FS47Diner::Shine(int32 Src, const TArray<int32>& Slots, double K)
{
	if (!M || !M->Web) return;
	for (int32 J = 0; J < 3; ++J)
	{
		FVector P; float W = 0; FLinearColor C;
		if (Src < 0 || !Slots.IsValidIndex(J) || Slots[J] < 0 || !M->Web->FloodPool(Src, Slots[J], P, W, C)) { M->Web->SetFlood(Flood, 10 + J, 0); continue; }
		M->Web->SetFloodPool(Flood, 10 + J, P, W, C * float(K), false);
	}
	M->Web->FlushFlood(Flood);
}

// Diner.ts neon: its red/cyan sign as two lamps in the road's or old road's set.
void FS47Diner::Neon(int32 Out, const TArray<int32>& Slots, double Reach, double K)
{
	if (!M || !M->Web) return;
	for (int32 J = 0; J < 2; ++J)
	{
		if (!Slots.IsValidIndex(J) || Slots[J] < 0) continue;
		FVector P; float W = 0; FLinearColor C; if (!M->Web->FloodPool(Flood, J, P, W, C)) continue;
		M->Web->SetFloodPool(Out, Slots[J], P, float(W * Reach), C * float(K), false);
	}
	M->Web->FlushFlood(Out);
}

void FS47Diner::Update(double Dt, double T)
{
	(void)Dt; if (!M || !M->Web) return;
	if (bSignLit)
	{
		M->Web->SetFlood(Flood, 0, float(8 * (0.965 + 0.035 * FMath::Sin(T * 7.3))));
		M->Web->SetFlood(Flood, 1, float(4 * (0.985 + 0.015 * FMath::Sin(T * 4.7))));
	}
	const double K = 0.98 + 0.02 * FMath::Sin(T * 8.1) * FMath::Sin(T * 0.9);
	M->Web->SetFlood(Flood, 3, float(4.5 * K));
	M->Web->Material(TEXT("world.diner.m.tube")).SetEmissive(S47Three::Hex(0xddeaff), float(0.75 * K));
}

bool FS47Diner::SelfTest(TArray<FString>& Lines)
{
	FS47Diner D(nullptr); D.SetDawn(-1); bool Ok = D.Dawn == 0; D.SetDawn(2); Ok &= D.Dawn == 1;
	D.SetDawn(std::numeric_limits<double>::quiet_NaN()); Ok &= D.Dawn == 0; D.SetSignLit(false); Ok &= !D.bSignLit;
	Lines.Add(FString::Printf(TEXT("%s Diner: finite/clamped dawn and sign state; geometry requires web export"), Ok ? TEXT("PASS") : TEXT("FAIL"))); return Ok;
}

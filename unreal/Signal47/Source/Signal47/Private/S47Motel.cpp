// world/MotelFront.ts and world/Crossing.ts (the behaviour chapter four needs)
#include "S47Motel.h"

#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Main.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47World.h"

using S47Three::Hex;

namespace
{
	// an exported texture by its handle (world.motel.litWindow ...): /Game/Web/Tex/T<id>
	UTexture* TexOf(AS47Web* W, const TCHAR* Path)
	{
		const int32 Id = W ? W->HandleTex(Path) : -1;
		return Id >= 0 ? LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/Web/Tex/T%d.T%d"), Id, Id)) : nullptr;
	}
}

void US47Motel::Init(US47Main* InM)
{
	M = InM;
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W) return;
	Flood = W->FloodSet(TEXT("court"));
	LitWindow = TexOf(W, TEXT("world.motel.litWindow"));
	DarkWindow = TexOf(W, TEXT("world.motel.darkWindow"));
	bInit = true;
}

// (World.ts: court = motel, one MotelFront; the export names it by its first path, world.motel)
FS47Obj US47Motel::Proxy(const FString& Id) const { return M && M->Web ? M->Web->Obj(TEXT("world.motel.proxies.") + Id) : FS47Obj(); }
FS47Obj US47Motel::Obj(const FString& Id) const { return M && M->Web ? M->Web->Obj(TEXT("world.motel.objs.") + Id) : FS47Obj(); }
FS47Pose US47Motel::Anchor(const FString& Id) const { return M && M->Web ? M->Web->Pose(TEXT("world.motel.anchors.") + Id) : FS47Pose(); }
FS47Obj US47Motel::Brick() const { return M && M->Web ? M->Web->Obj(TEXT("world.crossing.brick")) : FS47Obj(); }

void US47Motel::SetRoom6Light(bool bOn)
{
	bRoom6LightOn = bOn;
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W) return;
	if (Flood >= 0) W->SetFlood(Flood, SlotRoom6, bOn ? 4.f : 0.f);
	// eachVariant(this.m.room6Lamp): the porch lamp's emissive (0xffc478) at .8
	W->Material(TEXT("world.motel.m.room6Lamp")).SetEmissive(Hex(0xffc478), bOn ? 0.8f : 0.f);
	// eachVariant(this.m.room6Window): the lit window or the dark one, glowing (0xd99742) at .3. Only the
	// picture changes: the material keeps its own placement of it.
	const FS47Mat Window = W->Material(TEXT("world.motel.m.room6Window"));
	if (UMaterialInstanceDynamic* D = Window ? W->SharedMid(Window.Id) : nullptr)
	{
		if (UTexture* T = (bOn ? LitWindow : DarkWindow).Get()) D->SetTextureParameterValue(TEXT("Map"), T);
	}
	Window.SetEmissive(Hex(0xd99742), bOn ? 0.3f : 0.f);
}

void US47Motel::Update(double Dt, double T)
{
	(void)Dt;
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W || Flood < 0) return;
	// the sign's red flickers a little, its cyan breathes, and one porch lamp is going
	const double K = 0.97 + 0.03 * FMath::Sin(T * 5.3) * FMath::Sin(T * 0.73);
	W->SetFlood(Flood, SlotRed, float(9 * K)); W->SetFlood(Flood, SlotCyan, float(4 * (0.985 + 0.015 * FMath::Sin(T * 3.1))));
	W->SetFlood(Flood, SlotR34, float(5 * (0.96 + 0.04 * FMath::Sin(T * 8.3))));
	// (this.windowLights.update(): the curtains' light on the walk, world/windowLight.ts, is not in this version)
}

void US47Motel::Tick(float DeltaTime)
{
	// main.ts step: world.update (not behind the title; while the truck is driven, not behind a panel);
	// World.update: the court while one is at SARO
	if (!M || M->Mode != US47Main::EMode::Play || !M->World || M->World->Area != TEXT("saro")) return;
	if (M->World->bDriving && M->ModalOpen()) return;
	Update(DeltaTime, M->T);
}

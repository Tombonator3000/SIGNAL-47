// world/Room6.ts (the behaviour)
#include "S47Room6.h"

#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Canvas.h"
#include "S47Main.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47World.h"
#include "TextureResource.h"

using S47Three::Hex;

void US47Room6::Init(US47Main* InM)
{
	M = InM;
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W) return;
	// motelFlood: floodSet(6, 'motel', 0.2); its slots in the order the constructor adds them (the table
	// lamp, the nightstand lamp, the neon through the window, the window's cold light, the television)
	Flood = W->FloodSet(TEXT("motel"));
	// this.tvTex: the screen's picture, 96x72, black until the set is switched on (a canvas texture in
	// sRGB). The screen material shows it as its emissive map.
	TvTex = UTexture2D::CreateTransient(TV_W, TV_H, PF_B8G8R8A8);
	if (TvTex)
	{
		TvTex->SRGB = true;
		TvTex->Filter = TF_Bilinear;
		TvPx.Init(FColor(0, 0, 0, 255), TV_W * TV_H);
		void* Data = TvTex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Data, TvPx.GetData(), TV_W * TV_H * sizeof(FColor));
		TvTex->GetPlatformData()->Mips[0].BulkData.Unlock();
		TvTex->UpdateResource();
		const FS47Mat Screen = W->Material(TEXT("world.room6.tvMaterial"));
		if (UMaterialInstanceDynamic* D = Screen ? W->SharedMid(Screen.Id) : nullptr)
		{
			D->SetScalarParameterValue(TEXT("UseEmissiveMap"), 1.f);
			D->SetTextureParameterValue(TEXT("EmissiveMap"), TvTex);
		}
	}
	bInit = true;
}

void US47Room6::BeginDestroy()
{
	bInit = false;
	Super::BeginDestroy();
}

FS47Obj US47Room6::Proxy(const FString& Id) const { return M && M->Web ? M->Web->Obj(TEXT("world.room6.proxies.") + Id) : FS47Obj(); }
FS47Obj US47Room6::Obj(const FString& Id) const { return M && M->Web ? M->Web->Obj(TEXT("world.room6.objs.") + Id) : FS47Obj(); }

US47Room6::FAnchor US47Room6::Anchor(const FString& Id) const
{
	const FS47Pose P = M && M->Web ? M->Web->Pose(TEXT("world.room6.anchors.") + Id) : FS47Pose();
	return { P.X, P.Z, P.Yaw };
}

void US47Room6::SetLamp(bool bOn)
{
	bLampOn = bOn;
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W) return;
	if (Flood >= 0) W->SetFlood(Flood, LampSlot, bOn ? 3.3f : 0.f);
	// eachVariant(this.lampMaterial): its emissive (0xffbb68) at .8, and its colour
	const FS47Mat Lamp = W->Material(TEXT("world.room6.lampMaterial"));
	Lamp.SetEmissive(Hex(0xffbb68), bOn ? 0.8f : 0.f);
	Lamp.SetColor(Hex(bOn ? 0xc7a870 : 0x897652));
	// the bought lamp's own shade (placeModels: emissive 0xffc27a)
	for (int32 I = 0; W->HasPath(FString::Printf(TEXT("world.room6.lampShades[%d]"), I)); ++I)
	{
		const FS47Mat Shade = W->Material(FString::Printf(TEXT("world.room6.lampShades[%d]"), I));
		Shade.SetEmissive(Hex(0xffc27a), bOn ? 0.6f : 0.f);
		const float K = bOn ? 1.f : 0.69f;
		Shade.SetColor(FLinearColor(K, K, K));
	}
}

void US47Room6::SetTv(bool bOn)
{
	bTvOn = bOn;
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W) return;
	// eachVariant(this.tvMaterial): emissive white through the snow, at 1.15
	const FS47Mat Screen = W->Material(TEXT("world.room6.tvMaterial"));
	Screen.SetEmissive(FLinearColor::White, bOn ? 1.15f : 0.f);
	Screen.SetColor(Hex(bOn ? 0x6d7c84 : 0x243336));
	if (!bOn && Flood >= 0) W->SetFlood(Flood, TvSlot, 0.f);
}

void US47Room6::DrawSnow(TArray<FColor>& Px, double T, TFunctionRef<double()> Random)
{
	// a new field of snow, a little rolling band in it (createImageData: the values are stored into a
	// Uint8ClampedArray, rounded)
	Px.SetNum(TV_W * TV_H);
	const double Band = FMath::Fmod(T * 9, double(TV_H));
	for (int32 Y = 0; Y < TV_H; ++Y)
	{
		const double Lift = FMath::Abs(Y - Band) < 5 ? 40 : 0;
		for (int32 X = 0; X < TV_W; ++X)
		{
			const double V = FMath::Min(255.0, Random() * 215 + Lift);
			Px[Y * TV_W + X] = FColor(FS47Canvas::Clamp8(V * 0.92), FS47Canvas::Clamp8(V * 0.96), FS47Canvas::Clamp8(V), 255);
		}
	}
}

void US47Room6::UploadSnow()
{
	if (!TvTex || !TvTex->GetResource()) return;
	// g.putImageData(img, 0, 0); this.tvTex.needsUpdate = true
	const int32 Bytes = TV_W * TV_H * sizeof(FColor);
	uint8* Data = static_cast<uint8*>(FMemory::Malloc(Bytes));
	FMemory::Memcpy(Data, TvPx.GetData(), Bytes);
	FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, TV_W, TV_H);
	TvTex->UpdateTextureRegions(0, 1, Region, TV_W * sizeof(FColor), sizeof(FColor), Data, [](uint8* Src, const FUpdateTextureRegion2D* R) { FMemory::Free(Src); delete R; });
}

void US47Room6::Update(double Dt, double T)
{
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W) return;
	if (Flood >= 0) W->SetFlood(Flood, NeonSlot, float(0.9 * (0.96 + 0.04 * FMath::Sin(T * 0.7))));
	if (!bTvOn) return;
	// a new field of snow 15 times a second, a little rolling band in it, and the room's light
	// jumping with the brightness of the picture
	TvAcc += Dt;
	if (TvAcc >= 1.0 / 15)
	{
		TvAcc = 0;
		// Local programme footage replaces snow when present; no codec plugin is needed.
		if (!Programme.Frame(T + 138.5, TvPx, TV_W, TV_H)) DrawSnow(TvPx, T, [] { return double(FMath::FRand()); });
		UploadSnow();
		TvGlow += ((2.6 + FMath::FRand() * 1.8) - TvGlow) * 0.6;
	}
	if (Flood >= 0) W->SetFlood(Flood, TvSlot, float(TvGlow));
}

void US47Room6::Tick(float DeltaTime)
{
	// main.ts step: world.update (not behind the title); World.update: the room while one is in it
	if (!M || M->Mode != US47Main::EMode::Play || !M->World || M->World->Area != TEXT("room6")) return;
	Update(DeltaTime, M->T);
}

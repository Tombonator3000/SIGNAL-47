// world/Room6.ts (the behaviour)
#include "S47Room6.h"

#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Audio.h"
#include "S47Canvas.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "TextureResource.h"

using S47Three::Hex;

namespace
{
	// the screen: box(.51, .36) scaled .98 wide (Room6.ts), so the 96x72 canvas is drawn a little
	// wider than 4:3; the broadcast keeps its shape with a thin black bar at each side
	const double TV_ASPECT = 0.51 * 0.98 / 0.36;
	// the broadcast's sound from the set's speaker grille (the slats beside the screen), and its light
	// on the room: the snow's mean lit it at about 3.5; the film's mean (about 0.25) gives the same
	const double PROGRAMME = 0.28;
	const FVector GRILLE(0.278, 0.14, 0.283);
	double FilmGlow(double Brightness) { return FMath::Clamp(3.3 * FMath::Sqrt(FMath::Max(0.0, Brightness) / 0.25), 0.8, 5.2); }
}

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
	Programme.Silence();
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
	// (the broadcast's pictures keep their blacks: the glass stays dark under them; the snow lifts it)
	Screen.SetColor(Hex(bOn && !Programme.Available() ? 0x6d7c84 : 0x243336));
	if (!bOn && Flood >= 0) W->SetFlood(Flood, TvSlot, 0.f);
	// off: the broadcast's sound with it at once (its clock goes on); on: its picture at once
	if (!bOn) Programme.Silence();
	TvFrame = -1;
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
	// g.putImageData(img, 0, 0); this.tvTex.needsUpdate = true. (The screen's texture coordinates run
	// up from the bottom, as three's flipped canvas textures do: the rows go up bottom first, or the
	// broadcast's pictures stand on their heads. The snow never showed it.)
	const int32 Bytes = TV_W * TV_H * sizeof(FColor), Row = TV_W * sizeof(FColor);
	uint8* Data = static_cast<uint8*>(FMemory::Malloc(Bytes));
	for (int32 Y = 0; Y < TV_H; ++Y) FMemory::Memcpy(Data + (TV_H - 1 - Y) * Row, TvPx.GetData() + Y * TV_W, Row);
	FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, TV_W, TV_H);
	TvTex->UpdateTextureRegions(0, 1, Region, TV_W * sizeof(FColor), sizeof(FColor), Data, [](uint8* Src, const FUpdateTextureRegion2D* R) { FMemory::Free(Src); delete R; });
}

void US47Room6::Update(double Dt, double T)
{
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W) return;
	if (Flood >= 0) W->SetFlood(Flood, NeonSlot, float(0.9 * (0.96 + 0.04 * FMath::Sin(T * 0.7))));
	Broadcast(T);
	if (!bTvOn) return;
	// a new field of snow 15 times a second, a little rolling band in it, and the room's light
	// jumping with the brightness of the picture. (The local broadcast, when staged, instead: each of
	// its pictures as it comes, at the film's own rate, and the light following the picture.)
	TvAcc += Dt;
	const bool bFilm = Programme.Available();
	if (bFilm ? Programme.FrameIndex() != TvFrame : TvAcc >= 1.0 / 15)
	{
		TvAcc = 0;
		TvFrame = Programme.FrameIndex();
		if (bFilm && Programme.Frame(TvPx, TV_W, TV_H, TV_ASPECT))
		{
			UploadSnow();
			TvGlow += (FilmGlow(Programme.Brightness()) - TvGlow) * 0.6;
		}
		else
		{
			DrawSnow(TvPx, T, [] { return double(FMath::FRand()); });
			UploadSnow();
			TvGlow += ((2.6 + FMath::FRand() * 1.8) - TvGlow) * 0.6;
		}
	}
	if (Flood >= 0) W->SetFlood(Flood, TvSlot, float(TvGlow));
}

void US47Room6::Broadcast(double T)
{
	// The broadcast's clock goes on with main.ts t, also while the set is off or the player is out
	// (it starts at the trailer); its sound only while the set is on, in step with the picture, from
	// the set's grille, duller and quieter from the walk outside, and stepping back under a voice.
	if (!Programme.Available()) return;
	if (ProgrammeT < 0) Programme.Time = T + 138.5;
	const double Dt = ProgrammeT < 0 ? 0.0 : FMath::Max(0.0, T - ProgrammeT);
	ProgrammeT = T;
	const FS47Obj Tv = Obj(TEXT("tv"));
	const bool bInside = M->World && M->Player && M->World->Indoors(M->Player->Pos);
	const double Duck = M->Story ? M->Story->VoicesDuck() : 1.0;
	Programme.Update(Dt, M->Audio.Get(), Tv ? Tv.LocalToWorld(GRILLE) : FVector::ZeroVector, bTvOn && Tv ? PROGRAMME * Duck : 0.0, !bInside);
}

void US47Room6::Tick(float DeltaTime)
{
	// main.ts step: world.update (not behind the title); World.update: the room while one is in it
	// (out of it, the set's broadcast is not heard)
	if (!M || M->Mode != US47Main::EMode::Play || !M->World || M->World->Area != TEXT("room6")) { Programme.Silence(); return; }
	Update(DeltaTime, M->T);
}

#include "S47Annex.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Image.h"
#include "S47Main.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Annex, Log, All);

namespace
{
	TWeakObjectPtr<US47Annex> GAnnex;
	constexpr int32 DustN = 192;
	// archive047.ts DRAWER
	constexpr double DrawerX = -5.994, DrawerTravel = 0.3;

	FLinearColor Scaled(const FLinearColor& C, double K) { return FLinearColor(float(C.R * K), float(C.G * K), float(C.B * K), C.A); }
}

// ---------- archive047.ts ----------

void FS47Arc::SetDrawer(double K) const
{
	FS47Obj D = Drawer; // (a handle: moving it moves the drawer)
	D.SetX(DrawerX + DrawerTravel * FMath::Clamp(K, 0.0, 1.0));
}

void FS47Arc::Lamp(const FString& Line, double K) const
{
	// m.color.setRGB(0.14 + 0.86 k, 0.08 + 0.5 k, 0.05 + 0.13 k) on the line's own lamp material.
	// The two lamps are closure locals of buildArchive047, not reachable by a path, so the export
	// has no handle for them (Tools/webexport finds what the code holds by name).
	static bool bTold = false;
	if (!bTold) { bTold = true; UE_LOG(LogS47Annex, Display, TEXT("S47: annex.arc.lamp(%s, %.2f): the switchboard's line lamps are not in the export"), *Line, K); }
}

void FS47Arc::SetStrip(const TArray<FString>& Lines) const
{
	// The strip out of the call recorder is drawn on a canvas held by buildArchive047's closure;
	// neither it nor its mesh is in the export.
	static bool bTold = false;
	if (!bTold) { bTold = true; UE_LOG(LogS47Annex, Display, TEXT("S47: annex.arc.setStrip (%d lines): the recorder's strip is not in the export"), Lines.Num()); }
}

// ---------- the annex ----------

US47Annex* US47Annex::Get() { return GAnnex.Get(); }

void US47Annex::Init(US47Main* InM)
{
	M = InM;
	GAnnex = this;
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W) return;
	FloodSet = W->FloodSet(TEXT("annex"));
	Arc.M = M;
	Arc.Group = W->Obj(TEXT("annex.arc.group"));
	Arc.Drawer = W->Obj(TEXT("annex.arc.drawer"));
	Arc.Reel = W->Obj(TEXT("annex.arc.reel"));
	Arc.Papers = W->Obj(TEXT("annex.arc.papers"));
	Arc.Hit.Drawer = W->Obj(TEXT("annex.arc.hit.drawer"));
	Arc.Hit.Ksu = W->Obj(TEXT("annex.arc.hit.ksu"));
	Arc.Hit.Recorder = W->Obj(TEXT("annex.arc.hit.recorder"));
	Arc.RecorderAt = W->Vec(TEXT("annex.arc.recorderAt"), FVector(-1.12 - 0.06, 1.0 + 0.06, 10.22));
	Arc.DrawerAt = W->Vec(TEXT("annex.arc.drawerAt"), FVector(DrawerX + 0.1, 1.27, 8.6));
}

FS47Area US47Annex::Zone(const FString& Id) const
{
	return M && M->Web ? M->Web->Rect(TEXT("annex.zone.") + Id) : FS47Area();
}

FS47Obj US47Annex::Obj(const FString& Name) const
{
	return M && M->Web ? M->Web->Obj(TEXT("annex.objs.") + Name) : FS47Obj();
}

FS47Obj US47Annex::Interior() const
{
	return M && M->Web ? M->Web->Obj(TEXT("annex.interior")) : FS47Obj();
}

void US47Annex::HoldTubes(const FString& Mode)
{
	TubeHold = Mode;
	if (Mode == TEXT("stutter")) StutterLeft = 0.75;
}

void US47Annex::SetRoofFrost(const FString& Corner)
{
	FrostAt = Corner;
	if (!M || !M->Web) return;
	M->Web->Obj(TEXT("annex.frost.group")).SetVisible(!Corner.IsEmpty());
	M->Web->Obj(TEXT("annex.frost.lips.sw")).SetVisible(Corner == TEXT("sw"));
	M->Web->Obj(TEXT("annex.frost.lips.se")).SetVisible(Corner == TEXT("se"));
}

FLinearColor US47Annex::TubeOnColor() const { return Scaled(S47Three::Hex(0xeef3ff), 0.06 + 0.94 * Power); }
FLinearColor US47Annex::TubeDimColor() const { return Scaled(S47Three::Hex(0x6d7378), 0.12 + 0.88 * Power); }

void US47Annex::SetPower(double K)
{
	Power = K;
	if (M && M->Web) M->Web->Material(TEXT("annex.tubeOn")).SetColor(TubeOnColor());
	LastOn = -1; // (the tired tube has its own copy: set again on the next update)
}

// ---------- the dust ----------

void US47Annex::Sift(double X, double Z, int32 N, double Spread, double Delay)
{
	auto R = [] { return double(FMath::FRand()); };
	if (!Dust)
	{
		FDust D;
		D.Pos.Init(FVector(-50, -50, -50), DustN);
		D.V.Init(0.f, DustN); D.At.Init(0.f, DustN); D.Live.Init(0, DustN);
		Dust = D;
		// a soft round mote, not a square: 32 x 32, white, its alpha from the canvas's radial
		// gradient (1 at the centre, 0.55 at 0.45, 0 at the edge)
		AS47Web* W = M ? M->Web.Get() : nullptr;
		UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Unlit_Trans.M_Web_Unlit_Trans"));
		if (W && Plane && Base)
		{
			TArray<FColor> Px;
			Px.SetNumUninitialized(32 * 32);
			for (int32 y = 0; y < 32; ++y) for (int32 x = 0; x < 32; ++x)
			{
				const double T = FMath::Min(1.0, FMath::Sqrt(FMath::Square(x + 0.5 - 16) + FMath::Square(y + 0.5 - 16)) / 16.0);
				const double A = T < 0.45 ? FMath::Lerp(1.0, 0.55, T / 0.45) : FMath::Lerp(0.55, 0.0, (T - 0.45) / 0.55);
				Px[y * 32 + x] = FColor(255, 255, 255, uint8(FMath::RoundToInt(A * 255)));
			}
			DustMat = UMaterialInstanceDynamic::Create(Base, this);
			DustMat->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(0xcfc7b6));
			DustMat->SetScalarParameterValue(TEXT("Opacity"), 0.6f);
			if (UTexture2D* Mote = S47Image::FromPixels(Px, 32, 32))
			{
				DustMat->SetScalarParameterValue(TEXT("UseMap"), 1.f);
				DustMat->SetTextureParameterValue(TEXT("Map"), Mote);
			}
			DustMesh = NewObject<UInstancedStaticMeshComponent>(W, TEXT("AnnexDust"));
			DustMesh->SetStaticMesh(Plane);
			DustMesh->SetMobility(EComponentMobility::Movable);
			DustMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			DustMesh->SetCastShadow(false);
			DustMesh->SetMaterial(0, DustMat);
			DustMesh->SetupAttachment(W->GetRootComponent());
			DustMesh->RegisterComponent();
			TArray<FTransform> All;
			All.Init(FTransform(FQuat::Identity, S47Three::ToUE(-50, -50, -50), FVector(0.014)), DustN);
			DustMesh->AddInstances(All, false, true);
			DustMesh->SetVisibility(false);
		}
		else UE_LOG(LogS47Annex, Display, TEXT("S47: annex dust has nothing to draw with (plane %s, material %s)"), Plane ? TEXT("found") : TEXT("missing"), Base ? TEXT("found") : TEXT("missing"));
	}
	FDust& D = *Dust;
	for (int32 k = 0; k < N; ++k)
	{
		const int32 i = D.Next;
		D.Next = (D.Next + 1) % DustN;
		D.Pos[i].X = X + (R() - 0.5) * Spread;
		D.Pos[i].Y = 2.775;
		D.Pos[i].Z = Z + (R() - 0.5) * Spread * 0.6;
		D.V[i] = float(0.18 + R() * 0.34);
		D.At[i] = float(D.T + FMath::Pow(R(), 1.7) * Delay);
		D.Live[i] = 1;
	}
	D.bVisible = true;
	DrawDust();
}

int32 US47Annex::DustLive() const
{
	if (!Dust || !Dust->bVisible) return 0;
	int32 N = 0;
	for (uint8 L : Dust->Live) N += L;
	return N;
}

void US47Annex::DustStep(double Dt)
{
	if (!Dust || !Dust->bVisible) return;
	FDust& D = *Dust;
	D.T += Dt;
	bool bAny = false;
	for (int32 i = 0; i < D.V.Num(); ++i)
	{
		if (!D.Live[i]) continue;
		bAny = true;
		if (D.T < D.At[i]) continue;              // still caught in the seam
		D.Pos[i].Y -= D.V[i] * Dt;
		D.Pos[i].X += FMath::Sin((D.T + i) * 2.3) * 0.03 * Dt;
		if (D.Pos[i].Y < 0.02) { D.Live[i] = 0; D.Pos[i].Y = -50; }
	}
	if (!bAny) D.bVisible = false;
	DrawDust();
}

void US47Annex::DrawDust()
{
	// three's Points (size 0.014, attenuated with distance): a small square facing the eye per mote
	if (!Dust || !DustMesh) return;
	const bool bShow = Dust->bVisible && Interior().Shown();
	DustMesh->SetVisibility(bShow);
	if (!bShow) return;
	const FVector Eye = M && M->Player ? M->Player->EyeUE() : FVector::ZeroVector;
	TArray<FTransform> T;
	T.Reserve(DustN);
	for (int32 i = 0; i < DustN; ++i)
	{
		const FVector P = S47Three::ToUE(Dust->Pos[i]);
		T.Add(FTransform(FRotationMatrix::MakeFromZ((Eye - P).GetSafeNormal()).ToQuat(), P, FVector(0.014)));
	}
	DustMesh->BatchUpdateInstancesTransforms(0, T, true, true, true);
}

// ---------- per frame ----------

void US47Annex::Update(double T)
{
	// (glow.update(t): the glow points do not blink in AS47Web yet)
	const double Dt = LastT < 0 ? 0 : FMath::Max(0.0, FMath::Min(0.1, T - LastT));
	LastT = T;
	DustStep(Dt);
	if (TubeHold == TEXT("stutter") && (StutterLeft -= Dt) <= 0) TubeHold.Empty();
	const bool bOn = TubeHold == TEXT("steady") ? true
		: TubeHold == TEXT("out") ? false
		: TubeHold == TEXT("stutter") ? FMath::Sin(T * 53) + FMath::Sin(T * 31) > -0.2
		: !(FMath::Sin(T * 0.37) > 0.985 && FMath::Sin(T * 41) > 0);
	if (int32(bOn) == LastOn || !M || !M->Web) return;
	LastOn = int32(bOn);
	// flickerTube.material = on ? tubeOn : tubeDim (the tube's own copy takes the colour)
	M->Web->Obj(TEXT("annex.flickerTube")).SetColor(bOn ? TubeOnColor() : TubeDimColor());
	if (FloodSet >= 0) M->Web->SetFlood(FloodSet, FlickerFlood, bOn ? 5.5f : 1.8f);
}

bool US47Annex::IsTickable() const
{
	return !HasAnyFlags(RF_ClassDefaultObject) && M && M->Web;
}

void US47Annex::Tick(float DeltaTime)
{
	// main.ts step(): annex.update(t), t its seconds since start
	Update(M->T);
}

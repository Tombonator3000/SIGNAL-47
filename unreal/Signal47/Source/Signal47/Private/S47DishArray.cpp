// world/Dish.ts (Dish, DishArray's red lamps) and world/Exterior.ts fieldLights
#include "S47DishArray.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Image.h"
#include "S47Three.h"
#include "S47Web.h"

namespace
{
	const double DEG = UE_DOUBLE_PI / 180;
	// Dish.ts: the reflector's focal length; the lamp sits 0.9 m over the feed's apex
	const double R = 9, F = 7.2;
	const double APEX_Y = 1.1 + F;
	const FVector BEACON(0, APEX_Y + 0.9, 0);
	// Exterior.ts: the sodium floods near the closest dishes, and LAMPS (world/arrayLayout.ts)
	const double FLOODS[5][3] = { { 30, -30, 46 }, { -10, -42, 40 }, { -40, -56, 34 }, { 12, -78, 30 }, { 48, -86, 28 } };
	const double LAMPS[11][2] = { { 30, -30 }, { -10, -42 }, { -40, -56 }, { 12, -78 }, { 48, -86 }, { -70, -94 }, { -33, -100 }, { 60, -146 }, { -52, -158 }, { -4, -190 }, { 94, -110 } };

	/** Dish.matrices: the elevation level's transform of a point in the dish's own terms. root at
	 *  (x, 0, z) scaled; az = root * T(0, 7.8, 0) * Ry(-az); el = az * T(0, 4.6, 0) * Rx(-(90 - el)). */
	FVector OnElevation(const FS47Dish& D, double Az, double El, const FVector& P)
	{
		const double Ax = -(90 - El) * DEG, Ay = -Az * DEG;
		// Rx
		FVector Q(P.X, P.Y * FMath::Cos(Ax) - P.Z * FMath::Sin(Ax), P.Y * FMath::Sin(Ax) + P.Z * FMath::Cos(Ax));
		Q.Y += 4.6;
		// Ry
		Q = FVector(Q.X * FMath::Cos(Ay) + Q.Z * FMath::Sin(Ay), Q.Y, -Q.X * FMath::Sin(Ay) + Q.Z * FMath::Cos(Ay));
		Q.Y += 7.8;
		return FVector(D.X, 0, D.Z) + Q * D.Scale;
	}
}

FS47DishArray::FS47DishArray(AS47Web* InWeb) : Web(InWeb)
{
	if (!InWeb) return;
	// for (const d of dishes) this.beacons.add(0, -999, 0, 3.2 * d.scale, 0xff3c28, 1.6, d.phase): the
	// export has the points with their blink, so each dish's phase (Math.random() * 6) is its lamp's
	Beacons = FS47GlowView(InWeb, TEXT("ext.dishArray.beacons.points"));
	const FS47GlowPoints* Bg = BeaconGlow();
	for (int32 I = 0; I < InWeb->DishCount(); ++I)
	{
		FS47Dish D;
		D.Id = InWeb->DishId(I); D.X = InWeb->DishX(I); D.Z = InWeb->DishZ(I); D.Scale = InWeb->DishScale(I);
		D.Phase = Bg && Bg->Blink.IsValidIndex(I) && Bg->Blink[I].X > 0 ? Bg->Blink[I].Y : FMath::FRand() * 6;
		Dishes.Add(D);
	}
	for (int32 I = 0; I < Beacons.Num(); ++I) BeaconCol.Add(S47Three::Hex(0xff3c28));
	for (const FS47Dish& D : Dishes) BeaconPh.Add(D.Phase);
	Last.Init(-1e9, Dishes.Num());
	FindField();
	int32 Glows = 0, Floods = 0;
	for (const FFieldLamp& L : FieldLamps) { Glows += L.Glow >= 0; Floods += L.Flood >= 0; }
	UE_LOG(LogTemp, Display, TEXT("S47: dishes: %d red lamps (the first at %s, its feed's at %s); the field's lamps %d with glows, %d with floods"), Beacons.Num(),
		Beacons.Num() ? *Beacons.Pos(0).ToString() : TEXT("-"), Dishes.Num() ? *BeaconAt(0).ToString() : TEXT("-"), Glows, Floods);
}

double FS47DishArray::CurAz(int32 I) const { return Web.IsValid() ? Web->DishAz(I) : 0; }
double FS47DishArray::CurEl(int32 I) const { return Web.IsValid() ? Web->DishEl(I) : 0; }
void FS47DishArray::Snap(int32 I, double Az, double El) { if (Web.IsValid()) Web->SnapDish(I, Az, El); }
bool FS47DishArray::Moving(int32 I) const
{
	return (Dishes.IsValidIndex(I) && Dishes[I].Spin > 0.05) || (Web.IsValid() && Web->DishMoving(I));
}
int32 FS47DishArray::IndexOf(const FString& Id) const { return Dishes.IndexOfByPredicate([&](const FS47Dish& D) { return D.Id == Id; }); }

FVector FS47DishArray::BeaconAt(int32 I) const
{
	return Dishes.IsValidIndex(I) ? OnElevation(Dishes[I], CurAz(I), CurEl(I), BEACON) : FVector::ZeroVector;
}

FS47GlowPoints* FS47DishArray::BeaconGlow() const
{
	return Web.IsValid() ? Web->Glow(TEXT("ext.dishArray.beacons.points")) : nullptr;
}

void FS47DishArray::BeaconPhase(int32 I, double Phase)
{
	// beacons.setPhase(i, phase): its rate stays
	if (BeaconPh.IsValidIndex(I)) BeaconPh[I] = Phase;
	if (FS47GlowPoints* G = BeaconGlow()) G->SetPhase(I, Phase);
}

void FS47DishArray::HoldBeacons(TOptional<double> InLevel, double InFlare)
{
	// beacons.hold(level)
	bHold = InLevel.IsSet();
	if (InLevel) Level = *InLevel;
	if (FS47GlowPoints* G = BeaconGlow()) G->HoldAt(InLevel ? TOptional<float>(float(*InLevel)) : TOptional<float>());
	if (InFlare > 0 && !Flare.IsValid()) MakeFlare();
	if (Flare.IsValid())
	{
		bFlareOn = InFlare > 0;
		FlareLevel = InFlare;
		Flare->SetVisibility(bFlareOn);
		// (f.hold(flare): the haze's colour at that level)
		if (FlareMat.IsValid()) FlareMat->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(0xff2410) * float(FlareLevel));
	}
}

void FS47DishArray::MakeFlare()
{
	// a small bright core and a long faint tail: light in the night air, not a disc. (three's Points
	// keep facing the eye; here camera-facing planes, turned each frame while they are drawn.)
	AS47Web* W = Web.Get();
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Unlit_Add.M_Web_Unlit_Add"));
	if (!W || !Plane || !Base) return;
	// the haze: canvasTex(64, 64) with a radial gradient, white, its alpha 1 / .55 / .16 / .045 / 0 at
	// 0, .07, .22, .5 and 1 of the radius
	TArray<FColor> Px;
	Px.SetNumUninitialized(64 * 64);
	const double Stops[5][2] = { { 0, 1 }, { 0.07, 0.55 }, { 0.22, 0.16 }, { 0.5, 0.045 }, { 1, 0 } };
	for (int32 Y = 0; Y < 64; ++Y) for (int32 X = 0; X < 64; ++X)
	{
		const double T = FMath::Min(1.0, FMath::Sqrt(FMath::Square(X + 0.5 - 32) + FMath::Square(Y + 0.5 - 32)) / 32.0);
		double A = 0;
		for (int32 K = 0; K < 4; ++K) if (T >= Stops[K][0] && T <= Stops[K + 1][0]) { A = FMath::Lerp(Stops[K][1], Stops[K + 1][1], (T - Stops[K][0]) / (Stops[K + 1][0] - Stops[K][0])); break; }
		Px[Y * 64 + X] = FColor(255, 255, 255, uint8(FMath::RoundToInt(A * 255)));
	}
	UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Base, W);
	D->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(0xff2410));
	D->SetVectorParameterValue(TEXT("MapU"), FLinearColor(1, 0, 0, 0));
	D->SetVectorParameterValue(TEXT("MapV"), FLinearColor(0, 1, 0, 0));
	if (UTexture2D* Haze = S47Image::FromPixels(Px, 64, 64))
	{
		D->SetScalarParameterValue(TEXT("UseMap"), 1.f);
		D->SetTextureParameterValue(TEXT("Map"), Haze);
	}
	UInstancedStaticMeshComponent* I = NewObject<UInstancedStaticMeshComponent>(W, TEXT("DishFlare"));
	I->SetStaticMesh(Plane);
	I->SetMobility(EComponentMobility::Movable);
	I->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	I->SetCastShadow(false);
	I->SetMaterial(0, D);
	I->SetupAttachment(W->GetRootComponent());
	I->RegisterComponent();
	// f.add(v.x, v.y, v.z, 30 * d.scale, 0xff2410) for each dish's lamp
	TArray<FTransform> All;
	for (int32 K = 0; K < Dishes.Num(); ++K) All.Add(FTransform(FQuat::Identity, S47Three::ToUE(BeaconAt(K)), FVector(30 * Dishes[K].Scale)));
	I->AddInstances(All, false, true);
	I->SetVisibility(false);
	Flare = I;
	FlareMat = D;
}

void FS47DishArray::Update(double T, const FVector& Eye)
{
	// refresh: the lamps of the dishes that have turned, where their feeds are now
	for (int32 I = 0; I < Dishes.Num(); ++I)
	{
		const double Key = CurAz(I) * 1000 + CurEl(I);
		if (Last[I] == Key) continue;
		Last[I] = Key;
		if (I < Beacons.Num()) Beacons.SetPos(I, BeaconAt(I));
	}
	// beacons.update(t): the blink, 0.55 + 0.45 * max(0, sin(uTime * rate + phase)), mixed to the held
	// level while they are held, is the glow points' own (AS47Web's, stepped with the world's glows by
	// US47WorldLife on main.ts's t; BeaconPhase and HoldBeacons set them): here only the lamps' own
	// colour (aCol), which world/saroPower.ts takes down with SARO's power
	(void)T;
	for (int32 I = 0; I < Beacons.Num() && I < Dishes.Num(); ++I) Beacons.SetCol(I, BeaconCol[I]);
	// the flare, facing the eye (three draws its points facing the camera)
	if (bFlareOn && Flare.IsValid())
	{
		for (int32 I = 0; I < Dishes.Num() && I < Flare->GetInstanceCount(); ++I)
		{
			const FVector At = BeaconAt(I);
			const FVector To = S47Three::DirToUE((Eye - At).GetSafeNormal());
			// the plane's normal (+Z) towards the eye; 100 cm square at scale 1: 30 * scale metres
			const FQuat Q = FRotationMatrix::MakeFromZ(To).ToQuat();
			Flare->UpdateInstanceTransform(I, FTransform(Q, S47Three::ToUE(At), FVector(30 * Dishes[I].Scale)), true, I == Dishes.Num() - 1, true);
		}
	}
}

// ---------- Exterior.fieldLights ----------

void FS47DishArray::FindField()
{
	AS47Web* W = Web.Get();
	if (!W) return;
	SiteSet = W->FloodSet(TEXT("site"));
	ExtGlow = FS47GlowView(W, TEXT("ext.glow.points"));
	// floods[i]: the site set's pool at each of the five (addFlood(x, 3, z, w))
	TArray<FFieldLamp> Floods;
	for (const auto& Fl : FLOODS)
	{
		FFieldLamp L; L.W = Fl[2];
		for (int32 K = 0; K < W->FloodCount(SiteSet); ++K)
		{
			const FLinearColor P = W->FloodPool(SiteSet, K);
			if (FMath::Abs(P.R - Fl[0]) < 0.01 && FMath::Abs(P.G - 3) < 0.01 && FMath::Abs(P.B - Fl[1]) < 0.01) { L.Flood = K; break; }
		}
		Floods.Add(L);
	}
	// LAMPS.forEach: a glow each, 0.5 under the top of its post (7 m for the first five, 6 for the rest)
	FieldLamps.Reset();
	for (int32 I = 0; I < 11; ++I)
	{
		FFieldLamp L = I < 5 ? Floods[I] : FFieldLamp();
		L.At = FVector(LAMPS[I][0], (I < 5 ? 7 : 6) - 0.5, LAMPS[I][1]);
		for (int32 G = 0; G < ExtGlow.Num(); ++G) if (FVector::Dist(ExtGlow.Pos(G), L.At) < 0.05) { L.Glow = G; break; }
		FieldLamps.Add(L);
	}
}

void FS47DishArray::FieldLights(TOptional<double> K, bool bRed)
{
	FieldNow = !K ? TEXT("on") : bRed ? TEXT("red") : TEXT("out");
	AS47Web* W = Web.Get();
	if (!W) return;
	for (const FFieldLamp& L : FieldLamps)
	{
		const bool bOn = !K;
		if (L.Flood >= 0)
		{
			const FLinearColor P = W->FloodPool(SiteSet, L.Flood);
			W->SetFloodPool(SiteSet, L.Flood, FVector(P.R, P.G, P.B), float(bOn ? L.W : L.W * *K), S47Three::Hex(bOn || !bRed ? 0xff9a45 : 0xff1e10), false);
		}
		// glow.setPosition(l.glow, on ? l.at : hide.copy(l.at).setY(-999))
		if (L.Glow >= 0) ExtGlow.SetPos(L.Glow, bOn ? L.At : FVector(L.At.X, -999, L.At.Z));
	}
	W->FlushFlood(SiteSet);
}

bool FS47DishArray::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s dishes: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// the lamp on the feed: straight up (el 90) it is 7.8 + 4.6 + 9.2 m over the dish's root
	FS47Dish D; D.X = 10; D.Z = -40; D.Scale = 1;
	const FVector Up = OnElevation(D, 26, 90, BEACON);
	Check(Up.Equals(FVector(10, 7.8 + 4.6 + 9.2, -40), 1e-6), TEXT("the red lamp straight over the axes at el 90"));
	// at el 0, az 0 (north: -z) the feed points north, the lamp 9.2 m out along -z
	const FVector North = OnElevation(D, 0, 0, BEACON);
	Check(North.Equals(FVector(10, 7.8 + 4.6, -40 - 9.2), 1e-6), TEXT("at az 0, el 0 the feed points north (-z)"));
	// az 90 (east: +x)
	const FVector East = OnElevation(D, 90, 0, BEACON);
	Check(East.Equals(FVector(10 + 9.2, 7.8 + 4.6, -40), 1e-6), TEXT("at az 90 the feed points east (+x)"));
	return bOk;
}

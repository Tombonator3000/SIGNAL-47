#include "S47Town.h"
#include "S47Timeline.h"

#include "Components/MaterialBillboardComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Main.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47Web.h"

namespace
{
	constexpr double LAMPS[] = {2030, 2092, 2122, 2150, 2184, 2222, 2262};
	constexpr int32 OFF[] = {0, 9, 3, 14, 6, 21, 11};
	FLinearColor PanelColor(const FString& Id)
	{
		if (Id == TEXT("lamps")) return S47Three::Hex(0xffb860);
		if (Id == TEXT("blink")) return S47Three::Hex(0xffc020);
		if (Id == TEXT("canopy")) return S47Three::Hex(0xf4f1e0);
		return FLinearColor::White;
	}
}

FS47Town::FS47Town(US47Main* InM) : M(InM)
{
	for (int32 I = 0; I < 7; ++I) Lamps.Add({ LAMPS[I], 5 * 3600 + 5 * 60 + OFF[I] * 60 + FMath::FloorToDouble(S47RoadShape::Hash(LAMPS[I], 7) * 50), I + 1, (I + 4) * 4, true });
	if (!M || !M->Web) return;
	AS47Web* W = M->Web;
	Group = W->Obj(TEXT("world.town.group")); Origin = W->Vec(TEXT("world.town.origin"), Origin);
	Car = W->Obj(TEXT("world.town.car")); CarDoor = W->Obj(TEXT("world.town.carDoor")); Paper = W->Obj(TEXT("world.town.paper"));
	Woman = W->Obj(TEXT("world.town.woman")); Smoker = W->Obj(TEXT("world.town.smoker")); Cutout = W->Obj(TEXT("world.town.cutout"));
	CutoutAt = W->Vec(TEXT("world.town.cutoutAt"), CutoutAt); CarShoulder = W->Vec(TEXT("world.town.carShoulder"), CarShoulder);
	Flood = W->FloodSet(TEXT("town"));
	const FS47Area C = W->Rect(TEXT("world.town.carCol"));
	for (const FString& Area : { FString(TEXT("diner")), FString(TEXT("road")) })
	{
		const auto& Cs = W->CollidersMutable(Area);
		for (int32 I = 0; I < Cs.Num(); ++I) if (C.bValid && FMath::Abs(Cs[I].MinX - C.MinX) < 0.01 && FMath::Abs(Cs[I].MinZ - C.MinZ) < 0.01) CarColliders.Add({ Area, I });
	}
	// Town.ts constructor: every static mesh, person and proxy is from the exported scene.
	SetDeputy(true); SetPaper(false); SetCutout(false);
}

FS47Obj FS47Town::Proxy(const FString& Id) const { return M && M->Web ? M->Web->Obj(TEXT("world.town.proxies.") + Id) : FS47Obj(); }
void FS47Town::SetFlood(int32 I, double W) { if (M && M->Web) M->Web->SetFlood(Flood, I, float(W)); }

// Town.ts setPanel. The exporter splits the original atlas quads; no geometry is invented.
void FS47Town::SetPanel(const FString& Id, double K, TOptional<int32> Only)
{
	if (!M || !M->Web) return;
	const FString Prefix = TEXT("world.town.runtime.panels.") + Id + TEXT(".");
	for (const FString& Path : M->Web->PathsUnder(Prefix))
	{
		if (Only.IsSet() && Path != Prefix + FString::FromInt(Only.GetValue())) continue;
		const FLinearColor Base = PanelColor(Id);
		const FVector C = M->Web->Vec(TEXT("world.town.runtime.colors.") + Id, FVector(Base.R, Base.G, Base.B));
		M->Web->Obj(Path).SetColor(FLinearColor(float(C.X * K), float(C.Y * K), float(C.Z * K)));
	}
}

void FS47Town::SetQuadRect(bool bAir)
{
	if (!M || !M->Web) return;
	// Town.ts setQuadRect: identical atlas rectangles, represented by an affine UV offset.
	for (const FString& Path : M->Web->PathsUnder(TEXT("world.town.runtime.panels.studio.")))
	{
		const FS47Obj O = M->Web->Obj(Path);
		for (UMaterialInstanceDynamic* D : M->Web->NodeMids(O.I))
		{
			D->SetVectorParameterValue(TEXT("MapU"), FLinearColor(1, 0, bAir ? 0 : 0.5f, 0));
			D->SetVectorParameterValue(TEXT("MapV"), FLinearColor(0, 1, bAir ? 0 : -66.f / 2048.f, 0));
		}
	}
}

void FS47Town::GlowColor(int32 I, uint32 C)
{
	if (!M || !M->Web) return;
	const FS47Obj O = M->Web->Obj(FString::Printf(TEXT("world.town.runtime.glows[%d]"), I));
	const FS47WebNode* Node = M->Web->Node(O.I); if (!Node || !Node->Comp.IsValid()) return;
	TArray<USceneComponent*> Children; Node->Comp->GetChildrenComponents(true, Children);
	for (USceneComponent* Child : Children) if (UMaterialBillboardComponent* B = Cast<UMaterialBillboardComponent>(Child))
	{
		for (int32 J = 0; J < B->Elements.Num(); ++J) if (UMaterialInstanceDynamic* D = Cast<UMaterialInstanceDynamic>(B->GetMaterial(J))) D->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(C));
	}
}

void FS47Town::SetCutout(bool bOn) { if (bAnomalyCutoutOwned) return; bCutoutOut = bOn; Cutout.SetVisible(bOn); }
FVector FS47Town::PilotCutoutAt(bool bMoved) const
{
	// Near the shuttered studio on the return visit, within the same walkable street segment.
	const double X = bMoved ? 17.8 : 20.31, Z = bMoved ? 2044.0 : 2062.3;
	return FVector(X, S47RoadTerrain::SurfaceY(X, Z) + (bMoved ? 0 : .22), Z);
}
void FS47Town::SetPilotCutout(bool bMoved)
{
	bAnomalyCutoutOwned = true; bCutoutOut = bMoved;
	Cutout.SetPosition(PilotCutoutAt(bMoved)); Cutout.SetRotY(PI / 2); Cutout.SetVisible(true);
}
void FS47Town::ReleasePilotCutout()
{
	bAnomalyCutoutOwned = false; bCutoutOut = false;
	Cutout.SetPosition(FVector(CutoutAt.X, S47RoadTerrain::SurfaceY(CutoutAt.X, CutoutAt.Z), CutoutAt.Z));
	Cutout.SetRotY(0); Cutout.SetVisible(false);
}
TArray<FVector> FS47Town::WaveOrder() const
{
	TArray<FVector> Out; for (int32 I = Lamps.Num() - 1; I >= 0; --I) Out.Add(FVector(-3.9 + Origin.X, Origin.Y, Lamps[I].Z + Origin.Z)); return Out;
}

void FS47Town::SetPeople(bool bWoman, bool bSmoker)
{
	bWomanIn = bWoman; Woman.SetVisible(bWoman); bSmokerIn = bSmoker; Smoker.SetVisible(bSmoker);
	// Hidden proxies do not take the interaction trace (Town.ts present).
	FS47Obj Wp = Proxy(TEXT("woman")), Sp = Proxy(TEXT("smoker")); Wp.SetVisible(bWoman); Sp.SetVisible(bSmoker);
	if (!bSmoker) GlowColor(37, 0); GlowColor(36, bSmoker ? 0xffd890 : 0); SetFlood(16, bSmoker ? 3.2 : 0);
}

void FS47Town::SetDeputy(bool bOn)
{
	bDeputyIn = bOn; if (bOn) bCarOut = false; PlaceCar();
	SetPanel(TEXT("sheriffWin"), bOn ? 1 : 0.07); SetFlood(10, bOn ? 4.5 : 0); GlowColor(21, bOn ? 0x2a2618 : 0);
}
void FS47Town::SetCarOut(bool bOn) { bCarOut = bOn && !bDeputyIn; PlaceCar(); }

void FS47Town::PlaceCar()
{
	const bool bOut = bCarOut, bHere = bDeputyIn || bOut;
	const double X = bOut ? CarShoulder.X : -12.3, Z = bOut ? CarShoulder.Z : 2106.5, Y = S47RoadTerrain::SurfaceY(X, Z);
	Car.SetVisible(bHere); CarDoor.SetVisible(bOut); Car.SetPosition(FVector(X, Y, Z)); Car.SetRotY(bOut ? 0.04 : 0);
	FS47Obj P = Proxy(TEXT("patrolCar")); P.SetPosition(FVector(X, Y + 0.8, Z));
	FS47Area C; C.bValid = true; C.MinX = bHere ? X - 1 + Origin.X : 1e9; C.MaxX = bHere ? X + 1 + Origin.X : 1e9;
	C.MinZ = bHere ? Z - 2.7 + Origin.Z : 1e9; C.MaxZ = bHere ? Z + 2.7 + Origin.Z : 1e9;
	if (M && M->Web) for (const auto& Pair : CarColliders)
	{
		auto& Cs = M->Web->CollidersMutable(Pair.Key); if (!Cs.IsValidIndex(Pair.Value)) continue;
		Cs[Pair.Value].MinX = C.MinX; Cs[Pair.Value].MaxX = C.MaxX; Cs[Pair.Value].MinZ = C.MinZ; Cs[Pair.Value].MaxZ = C.MaxZ;
	}
	if (OnCarCollider) OnCarCollider(C);
}
void FS47Town::SetPaper(bool bOn) { bPaperIn = bOn; Paper.SetVisible(bOn); }

// Town.ts update: the same hours, lamp wave, smoker, studio, blinker and flickering tubes.
void FS47Town::Update(double T, double Clock, bool bHold)
{
	const double Day = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400, 86400.0);
	const bool bDeputy = S47Timeline::DeputyOnDuty(Clock);
	if (bDeputy != bDeputyIn && !(bDeputy && bCarOut && bHold)) SetDeputy(bDeputy);
	const bool bPaper = Day >= 4 * 3600 + 45 * 60 && Day < 12 * 3600;
	if (bPaper != bPaperIn) SetPaper(bPaper);
	const bool bWoman = S47Timeline::NightPerson(Clock, 4 * 3600 + 50 * 60), bSmoker = S47Timeline::NightPerson(Clock, 4 * 3600 + 10 * 60);
	if (bWoman != bWomanIn || bSmoker != bSmokerIn) SetPeople(bWoman, bSmoker);
	const bool bDraw = bSmoker && FMath::Fmod(T, 9.0) < 1.4;
	if (bSmoker && bDraw != bDrawing) { bDrawing = bDraw; GlowColor(37, bDraw ? 0xff6a20 : 0x5a2408); }
	for (int32 I = 0; I < Lamps.Num(); ++I)
	{
		FLamp& L = Lamps[I]; const int32 K = 6 - I; const double W = T - WaveT0 - K * WAVE_STEP;
		const bool bOn = (Day >= 18 * 3600 || Day < L.Off) && !(W >= 0 && W < WAVE_HOLD); if (bOn == L.bOn) continue; L.bOn = bOn;
		SetFlood(L.Slot, bOn ? 13 : 0); GlowColor(2 + I * 2, bOn ? 0xffb060 : 0); GlowColor(3 + I * 2, bOn ? 0x2c1806 : 0);
		SetPanel(TEXT("lamps"), bOn ? 1 : 0.05, L.Quad);
	}
	const bool bAir = S47Timeline::LocalOnAir(Clock);
	SetPanel(TEXT("onAir"), bAir ? 1 : 0);
	const bool bHaveEmpty = !M || !M->Web || M->Web->Vec(TEXT("world.town.runtime.hasStudioEmpty"), FVector(1, 0, 0)).X != 0;
	if (!bHaveEmpty) SetPanel(TEXT("studio"), bAir ? 1 : 0.3);
	else if (bAir == bStudioEmpty) { bStudioEmpty = !bAir; SetQuadRect(bAir); SetPanel(TEXT("studio"), bAir ? 1 : 1.8); }
	SetFlood(12, bAir ? 4 : 1.2);
	const bool bBlink = FMath::Fmod(T, 1.1) < 0.6;
	if (bBlink != bBlinkOn) { bBlinkOn = bBlink; SetPanel(TEXT("blink"), bBlink ? 1 : 0.06); SetFlood(0, bBlink ? 5 : 0); GlowColor(0, bBlink ? 0xffb420 : 0); GlowColor(1, bBlink ? 0x2a1a04 : 0); }
	auto Flick = [&](double Ph) { const double N = S47RoadShape::Hash(FMath::FloorToDouble(T * 8 + Ph), Ph); return 0.55 + 0.3 * N + 0.15 * FMath::Sin(T * 11 + Ph); };
	const double Tv1 = (Day >= 18 * 3600 || Day < 3 * 3600 + 40 * 60) ? Flick(1) : 0;
	SetPanel(TEXT("tv1"), Tv1 != 0 ? Tv1 : 0.05); SetFlood(14, Tv1 * 3); GlowColor(32, Tv1 != 0 ? 0x101828 : 0);
	const double Tv2 = Flick(7); SetPanel(TEXT("tv2"), Tv2); SetFlood(15, Tv2 * 3);
	const double St = S47RoadShape::Hash(FMath::FloorToDouble(T * 9), 3) > 0.985 ? 0.45 : 0.97 + 0.03 * FMath::Sin(T * 31);
	SetPanel(TEXT("laundroWin"), St); SetFlood(11, 6 * St);
	// GlowPoints.update and windowLight.ts are owned by the world's life port.
}

// Town.ts shine: the truck's current headlights copied into the town's three slots.
void FS47Town::Shine(int32 Src, const TArray<int32>& Slots, double K)
{
	if (!M || !M->Web) return;
	for (int32 J = 0; J < 3; ++J)
	{
		M->Web->SuppressFloodLight(Flood, 17 + J);
		FVector P; float W = 0; FLinearColor C;
		if (Src < 0 || !Slots.IsValidIndex(J) || Slots[J] < 0 || !M->Web->FloodPool(Src, Slots[J], P, W, C)) { M->Web->SetFlood(Flood, 17 + J, 0); continue; }
		M->Web->SetFloodPool(Flood, 17 + J, P, W, C * float(K), false);
	}
	M->Web->FlushFlood(Flood);
}

// Town.ts lampsTo: the nearest lit street lamps, canopy and crossing on the road.
void FS47Town::LampsTo(int32 Out, const TArray<int32>& Slots, const FVector& At, double K)
{
	if (!M || !M->Web) return;
	struct FCandidate { FVector P; float W = 0; FLinearColor C; double D = 0; };
	TArray<FCandidate> Near;
	TArray<int32> Cand; for (const FLamp& L : Lamps) Cand.Add(L.Slot); Cand.Add(8); Cand.Add(0);
	for (int32 I : Cand)
	{
		FCandidate A; if (!M->Web->FloodPool(Flood, I, A.P, A.W, A.C) || A.W <= 0) continue;
		A.D = FMath::Square(A.P.X - At.X) + FMath::Square(A.P.Z - At.Z); Near.Add(A);
	}
	Near.StableSort([](const FCandidate& A, const FCandidate& B) { return A.D < B.D; });
	for (int32 J = 0; J < Slots.Num(); ++J)
	{
		if (Slots[J] < 0) continue;
		M->Web->SuppressFloodLight(Out, Slots[J]);
		if (!Near.IsValidIndex(J)) { M->Web->SetFlood(Out, Slots[J], 0); continue; }
		const FCandidate& A = Near[J]; M->Web->SetFloodPool(Out, Slots[J], A.P, A.W, A.C * float(K), false);
	}
	M->Web->FlushFlood(Out);
}

bool FS47Town::InTown(double X, double Z) const
{
	if (M && M->Web) for (int32 I = 0; I < 2; ++I) if (M->Web->Rect(FString::Printf(TEXT("world.town.zones[%d]"), I)).Contains(X, Z)) return true;
	return !M && ((X >= Origin.X + S47RoadShape::DINER().X + 16 && X <= Origin.X + 16.4 && Z >= Origin.Z + 1978 && Z <= Origin.Z + 2016.5) || (X >= Origin.X - 17.6 && X <= Origin.X + 19.5 && Z >= Origin.Z + 2015 && Z <= Origin.Z + 2256));
}
FString FS47Town::PlaceAt(double X, double Z) const
{
	const double Lx = X - Origin.X, Lz = Z - Origin.Z; FString Best = TEXT("Caliche"); double D = 64;
	struct FPlace { const TCHAR* Name; double X, Z; };
	for (const FPlace& P : { FPlace{TEXT("Caliche, the gas station"), -12, 2064}, FPlace{TEXT("Caliche, the sheriff's substation"), -14, 2106}, FPlace{TEXT("Caliche, the laundromat"), -13, 2133}, FPlace{TEXT("Caliche, the Plaza"), -13, 2162}, FPlace{TEXT("Caliche, the feed store"), -12, 2202}, FPlace{TEXT("Caliche, 1420 AM"), 17, 2036}, FPlace{TEXT("Caliche, the souvenir shop"), 17, 2064}, FPlace{TEXT("Caliche, the church"), 17, 2094} })
	{
		const double E = FMath::Square(P.X - Lx) + FMath::Square(P.Z - Lz); if (E < D) { D = E; Best = P.Name; }
	}
	return Lz < 2016 ? TEXT("Caliche, the crossing") : Best;
}
FString FS47Town::SurfaceAt(double X, double Z) const
{
	const double Lx = X - Origin.X, Lz = Z - Origin.Z;
	if (FMath::Abs(Lx) < 6 || (Lx > -17.6 && Lx < -5.6 && Lz > 2040 && Lz < 2090) || (Lx > -17.6 && Lx < -14.2) || (Lx > 16.6 && Lx < 20.2 && Lz < 2200)) return TEXT("concrete");
	return TEXT("dirt");
}
double FS47Town::FloorAt(double X, double Z) const
{
	const double Lx = X - Origin.X, Lz = Z - Origin.Z; double G = Origin.Y + S47RoadTerrain::SurfaceY(Lx, Lz);
	const auto& Diner = S47RoadShape::DINER();
	if (Lx > 5 && Lx < 8 && FMath::Abs(Lz - Diner.Z) < 6) { const double A = S47RoadTerrain::SurfaceY(5, Lz), B = S47RoadTerrain::SurfaceY(8, Lz); G = Origin.Y + A + (B - A) * (Lx - 5) / 3; }
	const double Dx = FMath::Max3(Diner.X - 8.2 - Lx, 0.0, Lx - (Diner.X + 16.7)), Dz = FMath::Max3(Diner.Z - 16.2 - Lz, 0.0, Lz - (Diner.Z + 16.2));
	const double D = FMath::Sqrt(Dx * Dx + Dz * Dz), Y = Diner.Y + 0.1 + Origin.Y;
	return D >= 1.2 ? G : Y + (G - Y) * S47RoadShape::Smooth(0, 1.2, D);
}

bool FS47Town::SelfTest(TArray<FString>& Lines)
{
	FS47Town T(nullptr); T.Update(0, 3 * 3600); bool Ok = !T.bDeputyIn && T.bWomanIn && T.bSmokerIn;
	T.SetCarOut(true); T.Update(1, 4 * 3600 + 30 * 60, true); Ok &= T.bCarOut && !T.bDeputyIn;
	T.Update(2, 4 * 3600 + 45 * 60); Ok &= T.bDeputyIn && !T.bCarOut && T.bPaperIn && !T.bSmokerIn && T.bStudioEmpty;
	T.Update(3, 4 * 3600 + 50 * 60); Ok &= !T.bWomanIn;
	Ok &= T.WaveOrder().Num() == 7 && T.WaveOrder()[0].Z == 2262 && T.PlaceAt(7988, 2064) == TEXT("Caliche, the gas station") && T.SurfaceAt(8000, 2060) == TEXT("concrete");
	Lines.Add(FString::Printf(TEXT("%s Town: hours, held patrol car, people, papers, lamp order and places"), Ok ? TEXT("PASS") : TEXT("FAIL"))); return Ok;
}

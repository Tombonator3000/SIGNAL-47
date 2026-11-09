#include "S47SaroCot.h"
#include "S47Audio.h"
#include "S47Freeform.h"
#include "S47Fatigue.h"
#include "S47Handling.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "CollisionShape.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/GarbageCollection.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SaroCot, Log, All);

namespace
{
	const TCHAR* const CotUseId = TEXT("saro:cot");
	// Where a cot could go, in order: three-space middle of it on the floor, and whether its length runs along z.
	struct FCotSpot { const TCHAR* Name; double X, Z; bool bAlongZ; };
	const FCotSpot CotSpots[] = {
		// (The control room has no free wall 1.9 m long: the old cabinet and the tape shelf of S47Lived stand on the west wall, the
		// racks on the east. The records room is full of its table. The first sweep, 9 November, measured them.)
		// the east wing's test lab, along its west wall south of the glass cabinet: the quietest room with a door that shuts
		{ TEXT("the east wing's test lab, by the west wall"), 21.24, 3.4, true },
		// the parts storage, along the partition to calibration
		{ TEXT("the parts storage, along the partition"), 28.65, -.1, false },
		// the calibration room, along its partition to storage, east of the bench
		{ TEXT("the calibration room, along the partition"), 29.6, 1.0, false },
	};
	bool CotLive(const UObject* O) { return IsValid(O) && !O->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed); }
}

FS47SaroCot::~FS47SaroCot() { Life.Reset(); }

double FS47SaroCot::MorningAfter(double Clock)
{
	constexpr double Day = 86400, Eight = 8 * 3600;
	return (FMath::FloorToDouble((Clock - Eight) / Day) + 1) * Day + Eight;
}

bool FS47SaroCot::Live() const
{
	return M && !IsEngineExitRequested() && !IsGarbageCollecting() && CotLive(M) && CotLive(M->Web.Get()) && CotLive(M->Player.Get()) && M->Web->GetWorld();
}

FString FS47SaroCot::Place() const { return Spot >= 0 && Spot < int32(UE_ARRAY_COUNT(CotSpots)) ? FString(CotSpots[Spot].Name) : FString(TEXT("nowhere: no place was clear")); }

FString FS47SaroCot::Measure(double X, double Z, bool bAlong) const
{
	if (!Live()) return TEXT("no world");
	AS47Web* W = M->Web.Get();
	UWorld* World = W->GetWorld();
	const double HX = (bAlong ? Width : Length) / 2, HZ = (bAlong ? Length : Width) / 2;
	// (a) on the room's own floor: every corner in its walk zones
	for (const FVector2D C : { FVector2D(X - HX, Z - HZ), FVector2D(X + HX, Z - HZ), FVector2D(X - HX, Z + HZ), FVector2D(X + HX, Z + HZ), FVector2D(X, Z) })
	{
		// the zones themselves, not the walker's test (that keeps the player's own width off every edge)
		bool bIn = false;
		for (const FS47Rect& Z0 : W->ZonesMutable(TEXT("saro"))) bIn |= Z0.bEnabled && C.X >= Z0.MinX - 1e-6 && C.X <= Z0.MaxX + 1e-6 && C.Y >= Z0.MinZ - 1e-6 && C.Y <= Z0.MaxZ + 1e-6;
		// (the east wing is walked as open land once it is built, S47EastWing::Walls, not as zones)
		if (!bIn && M->Player->OpenLand) bIn = M->Player->OpenLand(C.X, C.Y);
		if (!bIn) return FString::Printf(TEXT("a corner off the walk zones at %.2f, %.2f"), C.X, C.Y);
	}
	// (b) clear of everything the player walks round
	for (const FS47Rect& C : W->CollidersMutable(TEXT("saro")))
		if (C.bEnabled && C.Id != CotUseId && C.MaxX > X - HX && C.MinX < X + HX && C.MaxZ > Z - HZ && C.MinZ < Z + HZ)
			return FString::Printf(TEXT("a walk collider %s (%.2f..%.2f, %.2f..%.2f)"), C.Id.IsEmpty() ? TEXT("(exported)") : *C.Id, C.MinX, C.MaxX, C.MinZ, C.MaxZ);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SaroCot), true);
	Q.AddIgnoredActor(M->Player.Get());
	for (const auto& P : Parts) if (P.IsValid()) Q.AddIgnoredComponent(P.Get());
	// (c) a floor under it
	FHitResult Floor;
	if (!World->LineTraceSingleByChannel(Floor, S47Three::ToUE(X, 1.2, Z), S47Three::ToUE(X, -1.5, Z), ECC_Visibility, Q)) return TEXT("no floor under it");
	const double Y0 = Floor.ImpactPoint.Z / 100.0;
	auto Hit = [&](const FVector& A, const FVector& B, FString& What)
	{
		FHitResult H;
		FCollisionQueryParams Solid = Q;
		bool bAny = false;
		for (int32 K = 0; K < 6; ++K)
		{
			if (!World->LineTraceSingleByChannel(H, S47Three::ToUE(A), S47Three::ToUE(B), ECC_Visibility, Solid)) break;
			if (FS47Handling::Solid(H.GetComponent())) { bAny = true; break; }
			Solid.AddIgnoredComponent(H.GetComponent());
		}
		if (!bAny) return false;
		const UPrimitiveComponent* C = H.GetComponent();
		What = FString::Printf(TEXT("%s at %.2f m over the floor"), C ? *C->GetName() : TEXT("something"), H.ImpactPoint.Z / 100.0 - Y0);
		return true;
	};
	FString What;
	// (d) nothing in the space it takes, from a little over the floor to over a table's height
	for (int32 I = 0; I < 5; ++I) for (int32 J = 0; J < 3; ++J)
	{
		const double PX = X - HX + .03 + (2 * HX - .06) * (bAlong ? J / 2.0 : I / 4.0);
		const double PZ = Z - HZ + .03 + (2 * HZ - .06) * (bAlong ? I / 4.0 : J / 2.0);
		if (Hit(FVector(PX, Y0 + 1.15, PZ), FVector(PX, Y0 + .04, PZ), What)) return TEXT("in the way: ") + What;
	}
	// (e) no wall through it: along each edge, low and at the rail's height
	for (const double Y : { Y0 + .12, Y0 + .45 })
	{
		const FVector C00(X - HX, Y, Z - HZ), C10(X + HX, Y, Z - HZ), C01(X - HX, Y, Z + HZ), C11(X + HX, Y, Z + HZ);
		for (const auto& E : { TPair<FVector, FVector>(C00, C10), TPair<FVector, FVector>(C10, C11), TPair<FVector, FVector>(C11, C01), TPair<FVector, FVector>(C01, C00), TPair<FVector, FVector>(C00, C11), TPair<FVector, FVector>(C10, C01) })
			if (Hit(E.Key, E.Value, What)) return TEXT("across it: ") + What;
	}
	// (f) and the simple collision of anything that has it
	const FVector Mid = S47Three::ToUE(X, Y0 + .06 + Height / 2, Z);
	// (three x is Unreal x, three z is Unreal y)
	const FVector Half = FVector(HX, HZ, Height / 2 - .03) * 100;
	TArray<FOverlapResult> Over;
	World->OverlapMultiByChannel(Over, Mid, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeBox(Half), Q);
	for (const FOverlapResult& O : Over) if (O.bBlockingHit && FS47Handling::Solid(O.GetComponent())) return FString::Printf(TEXT("%s overlaps the space"), O.GetComponent() ? *O.GetComponent()->GetName() : TEXT("a body"));
	return FString();
}

void FS47SaroCot::Build()
{
	if (bBuilt || !Live() || !M->Web->IsLoaded() || M->Web->Area() != TEXT("saro")) return;
	Notes.Reset();
	Spot = -1;
	for (int32 I = 0; I < int32(UE_ARRAY_COUNT(CotSpots)); ++I)
	{
		const FString Why = Measure(CotSpots[I].X, CotSpots[I].Z, CotSpots[I].bAlongZ);
		Notes.Add(FString::Printf(TEXT("%s: %s"), CotSpots[I].Name, Why.IsEmpty() ? TEXT("clear") : *Why));
		UE_LOG(LogS47SaroCot, Display, TEXT("S47: cot: %s"), *Notes.Last());
		if (Why.IsEmpty()) { Spot = I; break; }
	}
	if (Spot < 0)
	{
		// the rooms may still be being furnished: try again in a while, a few times, before giving up
		if (++Tries < 4) { Wait = 5; return; }
		bBuilt = true;
		UE_LOG(LogS47SaroCot, Error, TEXT("S47: cot: no place at SARO was clear; no cot"));
		return;
	}
	bBuilt = true;
	AS47Web* W = M->Web.Get();
	UWorld* World = W->GetWorld();
	bAlongZ = CotSpots[Spot].bAlongZ;
	FHitResult Floor;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SaroCot), true);
	Q.AddIgnoredActor(M->Player.Get());
	const double Y0 = World->LineTraceSingleByChannel(Floor, S47Three::ToUE(CotSpots[Spot].X, 1.2, CotSpots[Spot].Z), S47Three::ToUE(CotSpots[Spot].X, -1.5, CotSpots[Spot].Z), ECC_Visibility, Q) ? Floor.ImpactPoint.Z / 100.0 : 0.0;
	At = FVector(CotSpots[Spot].X, Y0, CotSpots[Spot].Z);
	auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
	auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	auto* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (!Base || !Cube || !Cyl) return;
	// its own frame: x along its length, y across, z up, centimetres from the middle of it on the floor
	const FVector Origin = S47Three::ToUE(At);
	const FQuat Turn(FVector::UpVector, bAlongZ ? UE_HALF_PI : 0.0);
	auto Mat = [&](uint32 Hex, float Rough)
	{
		auto* Mid = UMaterialInstanceDynamic::Create(Base, W);
		Mid->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(Hex));
		Mid->SetScalarParameterValue(TEXT("Roughness"), Rough);
		Mid->SetScalarParameterValue(TEXT("NativeLighting"), 1);
		Mid->SetScalarParameterValue(TEXT("EmissivePower"), 0);
		return Mid;
	};
	int32 N = 0, FirstNode = -1;
	auto Part = [&](UStaticMesh* Mesh, const FVector& Local, const FRotator& Rot, const FVector& SizeCm, uint32 Hex, float Rough)
	{
		auto* C = NewObject<UStaticMeshComponent>(W, *FString::Printf(TEXT("S47SaroCot_%d"), N++));
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Mesh);
		C->SetMaterial(0, Mat(Hex, Rough));
		C->SetupAttachment(W->GetRootComponent());
		C->SetCastShadow(true);
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		C->SetCollisionResponseToAllChannels(ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		C->RegisterComponent();
		C->SetWorldLocationAndRotation(Origin + Turn.RotateVector(Local), Turn * Rot.Quaternion());
		C->SetWorldScale3D(SizeCm / 100.0);
		const int32 Node = W->AdoptNode(C, INDEX_NONE, FString::Printf(TEXT("saro-cot:%d"), N));
		if (Node >= 0) { W->MarkUsable(Node, CotUseId); if (FirstNode < 0) FirstNode = Node; }
		Parts.Add(C);
	};
	const uint32 Canvas = 0x55593a, CanvasDark = 0x474a30, Alloy = 0x8b8e86, Wool = 0x5e5547, Stripe = 0x2f3a52, Ticking = 0xc9c2ae;
	const double L = Length * 100, Wd = Width * 100, Top = Height * 100;
	const FRotator AlongX(90, 0, 0), Across(0, 0, 90);
	// the two side rails in their canvas sleeves, and the canvas between them, a little slack in the middle
	for (const double Y : { -Wd / 2 + 3, Wd / 2 - 3 })
	{
		Part(Cyl, FVector(0, Y, Top - 2), AlongX, FVector(3.4, 3.4, L), Alloy, .45f);
		Part(Cube, FVector(0, Y, Top - 2), FRotator::ZeroRotator, FVector(L - 14, 5, 4.4), CanvasDark, .95f);
	}
	Part(Cube, FVector(0, 0, Top - 2.8), FRotator::ZeroRotator, FVector(L - 14, Wd - 8, 1.4), Canvas, .95f);
	Part(Cube, FVector(0, 0, Top - 3.6), FRotator::ZeroRotator, FVector(L - 60, Wd - 14, 1.0), CanvasDark, .97f);
	// the end bars
	for (const double X : { -L / 2 + 4, L / 2 - 4 }) Part(Cyl, FVector(X, 0, Top - 2), Across, FVector(3, 3, Wd - 2), Alloy, .45f);
	// three X legs, each two struts crossing under it
	const double Leg = FMath::Sqrt(FMath::Square(Wd - 10) + FMath::Square(Top - 3)), Lean = FMath::RadiansToDegrees(FMath::Atan2(Wd - 10, Top - 3));
	for (const double X : { -L / 2 + 14, 0.0, L / 2 - 14 })
		for (const double S : { -1.0, 1.0 }) Part(Cyl, FVector(X + S * 1.6, 0, (Top - 3) / 2), FRotator(0, 0, S * Lean), FVector(2.6, 2.6, Leg), Alloy, .5f);
	// a wool blanket folded at the foot, with its stripe, and a pillow at the head
	Part(Cube, FVector(L / 2 - 30, 0, Top + 3.5), FRotator(0, 2, 0), FVector(42, Wd - 10, 9), Wool, .98f);
	Part(Cube, FVector(L / 2 - 42, 0, Top + 3.6), FRotator(0, 2, 0), FVector(5, Wd - 9.6, 9.2), Stripe, .98f);
	Part(Cube, FVector(-L / 2 + 26, 0, Top + 4), FRotator(0, -5, 0), FVector(32, Wd - 18, 10), Ticking, .95f);
	SetCollider(true);
	// what the player uses
	const TWeakPtr<uint8> Guard = Life;
	FS47Interactable I;
	I.Id = CotUseId;
	I.Object = W->ObjAt(FirstNode);
	I.Range = 2.4;
	I.Label = [this, Guard]() -> FString
	{
		if (!Guard.IsValid() || !M || !M->IsFreeform() || !M->Freeform || !M->Freeform->Active()) return FString();
		return TEXT("Lie down on the cot");
	};
	I.Use = [this, Guard] { if (Guard.IsValid()) OpenMenu(); };
	I.Examine = [Guard]() -> FString { return Guard.IsValid() ? FString(TEXT("An army folding cot, olive canvas on aluminium, a wool blanket folded at the foot. Somebody has slept on it before you; the canvas sags in the middle.")) : FString(); };
	M->Inter->Add(I);
	bShown = true;
	UE_LOG(LogS47SaroCot, Display, TEXT("S47: cot: put up in %s at %s, %d parts"), *Place(), *At.ToCompactString(), Parts.Num());
}

void FS47SaroCot::SetCollider(bool bOn)
{
	if (!Live() || Spot < 0) return;
	auto& Rows = M->Web->CollidersMutable(TEXT("saro"));
	const double HX = (bAlongZ ? Width : Length) / 2 + .02, HZ = (bAlongZ ? Length : Width) / 2 + .02;
	FS47Rect* R = Rows.FindByPredicate([](const FS47Rect& C) { return C.Id == CotUseId; });
	if (!R) { FS47Rect New; New.Id = CotUseId; R = &Rows.Add_GetRef(New); }
	R->MinX = At.X - HX; R->MaxX = At.X + HX; R->MinZ = At.Z - HZ; R->MaxZ = At.Z + HZ; R->bEnabled = bOn;
}

void FS47SaroCot::Show(bool bOn)
{
	if (bShown == bOn) return;
	bShown = bOn;
	for (const auto& P : Parts) if (UStaticMeshComponent* C = P.Get())
	{
		C->SetVisibility(bOn, true);
		C->SetCollisionEnabled(bOn ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
	SetCollider(bOn);
}

void FS47SaroCot::Begin()
{
	Wait = 3; FadeLeft = -1;
	// a new world: the old parts went with it, and it is measured and put up again
	if (bBuilt && Parts.ContainsByPredicate([](const TWeakObjectPtr<UStaticMeshComponent>& P) { return !P.IsValid(); })) { Parts.Reset(); bBuilt = false; bShown = false; Spot = -1; Tries = 0; }
	if (bBuilt && Spot < 0) { bBuilt = false; Tries = 0; } // measured again: the rooms may have changed
	if (bBuilt && Spot >= 0) { bShown = false; Show(true); }
}

void FS47SaroCot::Stop()
{
	if (!Live()) return;
	if (FadeLeft >= 0 && M->UI) M->UI->Fade(false);
	FadeLeft = -1;
	Show(false);
}

void FS47SaroCot::Step(double Dt)
{
	if (!Live() || !M->IsFreeform() || !M->Freeform || !M->Freeform->Active()) return;
	if (!bBuilt)
	{
		// a moment for the rooms to be furnished (the console kit, the east wing) before it is measured against them
		Wait -= Dt;
		if (Wait <= 0) Build();
	}
	if (bBuilt && Spot >= 0) Show(M->Web->Area() == TEXT("saro"));
	if (FadeLeft >= 0)
	{
		FadeLeft -= Dt;
		if (FadeLeft < 0)
		{
			if (M->UI) { M->UI->Fade(false); if (!After.IsEmpty()) M->UI->Toast(After, 6); }
			After.Empty();
		}
	}
}

bool FS47SaroCot::Sleep(double Hours)
{
	if (!Live() || !M->Freeform || !M->Freeform->Active()) return false;
	FS47FreeformSession* F = M->Freeform.Get();
	const double Target = Hours > 0 ? F->Clock() + Hours * 3600 : MorningAfter(F->Clock());
	if (M->UI) M->UI->Close(true);
	// the same rest as OPS: refused for what is close, woken by the night's own, counted as a night at the station
	const bool bGood = F->WaitUntil(Target);
	const FString Said = F->FeedbackText();
	if (!bGood)
	{
		if (M->UI) M->UI->Toast(Said.IsEmpty() ? FString(TEXT("Not now.")) : Said, 6);
		UE_LOG(LogS47SaroCot, Display, TEXT("S47: cot: sleep refused: %s"), *Said);
		return false;
	}
	if (M->Audio) { FS47Sound S; S.At = At + FVector(0, .4, 0); S.Gain = .3f; S.Rate = .55f; M->Audio->Sfx(TEXT("doorCreak0"), S); }
	if (M->UI) M->UI->Fade(true);
	FadeLeft = 1.8; After = Said;
	UE_LOG(LogS47SaroCot, Display, TEXT("S47: cot: slept to clock %.0f: %s"), F->Clock(), *Said);
	return true;
}

void FS47SaroCot::OpenMenu()
{
	if (!Live() || !M->UI || !M->Freeform || !M->Freeform->Active()) return;
	FS47FreeformSession* F = M->Freeform.Get();
	const int32 Secs = int32(FMath::Fmod(F->Clock(), 86400.0));
	TArray<S47Kit::FW> Rows;
	Rows.Add(S47Kit::Small(FString::Printf(TEXT("%02d:%02d. The cot creaks when you sit on it. The station sleeps with you: the same rest as REST UNTIL MORNING at OPS."), Secs / 3600, (Secs / 60) % 60)));
	const TWeakPtr<uint8> Guard = Life;
	// what kind of sleep it will be, in the same words as OPS (FS47Fatigue::BedLine: the cot here is the station's bed)
	const auto Bed = [F](double Seconds) { return F->Fatigue && F->RulesLive() ? F->Fatigue->BedLine(Seconds, FS47Fatigue::EBed::Station) : FString(); };
	Rows.Add(S47Kit::Button(TEXT("Sleep until morning"), [this, Guard] { if (Guard.IsValid()) Sleep(0); }, true));
	if (const FString L = Bed(MorningAfter(F->Clock()) - F->Clock()); !L.IsEmpty()) Rows.Add(S47Kit::Small(L));
	Rows.Add(S47Kit::Button(TEXT("Sleep for two hours"), [this, Guard] { if (Guard.IsValid()) Sleep(2); }));
	if (const FString L = Bed(2 * 3600.0); !L.IsEmpty()) Rows.Add(S47Kit::Small(L));
	Rows.Add(S47Kit::Button(TEXT("Get up again"), [this, Guard] { if (Guard.IsValid() && M->UI) M->UI->Close(); }));
	M->UI->Open(S47Kit::Shell(M->UI, TEXT("THE COT"), S47Kit::Col(Rows), 640));
}

bool FS47SaroCot::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool b, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s saro cot: %s"), b ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= b; };
	constexpr double Day = 86400;
	Check(MorningAfter(23 * 3600.0) == Day + 8 * 3600 && MorningAfter(Day + 3 * 3600) == Day + 8 * 3600 && MorningAfter(Day + 8 * 3600) == 2 * Day + 8 * 3600 && MorningAfter(7 * 3600.0) == 8 * 3600,
		TEXT("until morning is the next 08:00, as at OPS (at 08:00 itself, the day after)"));
	Check(Length > 1.7 && Length < 2.1 && Width > .55 && Width < .8 && Height > .3 && Height < .5, TEXT("an army cot's size: about 1.9 by 0.7 m, the canvas some 40 cm up"));
	bool bInside = true;
	for (const FCotSpot& S : CotSpots) bInside &= FMath::IsFinite(S.X) && FMath::IsFinite(S.Z);
	// the first place is in the east wing's test lab (x 20.85..26, z -5.25..5.25 inside its walls), along its west wall
	const FCotSpot& First = CotSpots[0];
	Check(bInside && First.bAlongZ && First.X - Width / 2 > 20.85 && First.X + Width / 2 < 22.0 && First.Z - Length / 2 > -5.25 && First.Z + Length / 2 < 5.25,
		TEXT("the first place is along the test lab's west wall, inside its walls"));
	return bOk;
}

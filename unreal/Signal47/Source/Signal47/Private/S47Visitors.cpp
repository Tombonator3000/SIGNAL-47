#include "S47Visitors.h"
#include "S47EventPlaces.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47NightRhythm.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47Audio.h"
#include "S47Three.h"
#include "S47TownSites.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Crc.h"
#include "Misc/PackageName.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Visitors, Log, All);

namespace S47VisitorsLocal
{
	const TCHAR* const Weekdays[] = { TEXT("Monday"), TEXT("Tuesday"), TEXT("Wednesday"), TEXT("Thursday"), TEXT("Friday"), TEXT("Saturday"), TEXT("Sunday") };
	FString WeekdayOf(int32 Day) { return Weekdays[(FMath::Max(1, Day) - 1) % 7]; }
	// The van on the Mesa's lot, north of the diner (road-local, clear of its walls and its lot).
	constexpr double VanX = -40, VanZ = 1982;
	const TCHAR* const SedanPath = TEXT("/Game/Free/Vehicles/negotiator_80/StaticMeshes/negotiator_80.negotiator_80");
	// Old stories from the porch. Nothing of 1947 (Freeform keeps that for Story).
	const TCHAR* const OldTales[] = {
		TEXT("Spring of '58 every cow on the lease walked to the east fence one night and stood there facing the same way till sunup. I counted them twice."),
		TEXT("In '73 a light sat over the playa three nights running. The third night my wife wouldn't let me go out to it. She was right, I expect."),
		TEXT("Fellow from the government came by in '66 asking did my radio ever make noises. I said it was a radio. He wrote that down."),
		TEXT("That windmill on my lease turned against the wind one night. No wind at all. I've never told anybody but the priest, and now you."),
		TEXT("There used to be a light came along the rim like a man with a lantern. Then the man with the lantern died. So I don't know."),
		TEXT("My dog won't go past the stock tank after dark. Fourteen years old and never has."),
		TEXT("In '79 I lost an afternoon. Sat down on this porch at two and it was dark. My coffee was still hot. Explain that."),
		TEXT("Those dishes went up and the coyotes changed their hours. You can write that down. You people write everything down.") };
	// Where he says to go and look that is nowhere at all (pencilled on the sheet like the souvenir man's nonsense).
	struct FBunk { const TCHAR* Label; double X, Z; };
	constexpr int32 NumTales = UE_ARRAY_COUNT(OldTales);
	const FBunk BacaBunk[] = { { TEXT("Baca's burned circle"), 1150, 520 }, { TEXT("Baca's singing rock"), 980, 1820 }, { TEXT("Baca's cold spot"), 640, 2350 } };
	const TCHAR* const NotSaid[] = { TEXT("chevy"), TEXT("ford"), TEXT("dodge"), TEXT("jeep"), TEXT("coke"), TEXT("alien"), TEXT("ufo"), TEXT("roswell"), TEXT("tyre"), TEXT("grey"), TEXT("colour"), TEXT("petrol") };
}

// ---------- rules ----------

bool FS47Visitors::VanDay(int32 Seed, int32 Day)
{
	return Day >= 4 && FS47NightRhythm::Roll(Seed, Day, 0x56414e21u) < .22f;
}

bool FS47Visitors::Big(const FString& Kind)
{
	for (const TCHAR* K : { TEXT("site_visit"), TEXT("landing"), TEXT("marks"), TEXT("taken"), TEXT("field_visit"), TEXT("crash"), TEXT("dev_military") })
		if (Kind.Contains(K)) return true;
	return false;
}

bool FS47Visitors::Comes(int32 Seed, int32 Night, const FString& Place, bool bKnown)
{
	return FS47NightRhythm::Roll(Seed, Night, FCrc::StrCrc32(*Place) ^ 0x4d494c21u) < (bKnown ? .6f : .3f);
}

bool FS47Visitors::Cleared(const FString& Place, int32 Night) const
{
	return Visits.ContainsByPredicate([&](const FVisit& V) { return V.Place == Place && V.Night == Night && !V.bLooked; });
}

bool FS47Visitors::Has(const FString& Key) const
{
	if (Key == TEXT("baca_gone")) return BacaGone();
	if (Key == TEXT("military")) { const int32 N = M && M->Freeform ? FS47FreeformSession::NightOf(M->Freeform->Clock()) : 0; return Visits.ContainsByPredicate([N](const FVisit& V) { return N - V.Night <= 3; }); }
	return Flags.Contains(Key);
}

double FS47Visitors::Hour() const { return M && M->Freeform ? FMath::Fmod(M->Freeform->Clock(), 86400.) / 3600. : 12; }
FString FS47Visitors::PlaceTitle(const FString& Id) const { return M && M->Freeform && M->Freeform->EventPlaces ? M->Freeform->EventPlaces->TitleOf(Id) : FString(TEXT("somewhere out there")); }
void FS47Visitors::Note(const FString& Flag) { if (!Flags.Contains(Flag)) { Flags.Add(Flag); UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors: %s"), *Flag); if (M) M->RequestAutosave(); } }

// ---------- the session ----------

void FS47Visitors::Begin(const S47J::FObj& Saved)
{
	BacaTold.Reset(); Flags.Reset(); Visits.Reset();
	BacaDay = BacaBack = BacaCheckedNight = VanChoiceDay = ForcedVanDay = OutNight = CheckedNight = 0; BacaLast.Empty(); VanChoice.Empty(); OutPlace.Empty();
	if (Saved.IsValid())
	{
		for (const FString& T : S47J::Strs(Saved, TEXT("bacaTold"))) if (T.Len() < 96 && BacaTold.Num() < 500) BacaTold.Add(T);
		for (const FString& T : S47J::Strs(Saved, TEXT("flags"))) if (T.Len() < 32) Flags.Add(T);
		BacaDay = FMath::Clamp(S47J::Int(Saved, TEXT("bacaDay")), 0, 1000000); BacaBack = FMath::Clamp(S47J::Int(Saved, TEXT("bacaBack")), 0, 1000000);
		BacaCheckedNight = FMath::Clamp(S47J::Int(Saved, TEXT("bacaNight")), 0, 1000000); BacaLast = S47J::Str(Saved, TEXT("bacaLast")).Left(400);
		VanChoiceDay = FMath::Clamp(S47J::Int(Saved, TEXT("vanDay")), 0, 1000000); VanChoice = S47J::Str(Saved, TEXT("vanChoice")).Left(8);
		OutPlace = S47J::Str(Saved, TEXT("outPlace")).Left(64); OutNight = FMath::Clamp(S47J::Int(Saved, TEXT("outNight")), 0, 1000000);
		CheckedNight = FMath::Clamp(S47J::Int(Saved, TEXT("checkedNight")), 0, 1000000);
		for (const auto& V : S47J::Arr(Saved, TEXT("visits")))
		{
			if (!V || V->Type != EJson::Object || Visits.Num() >= 64) continue;
			const auto O = V->AsObject(); FVisit X;
			X.Place = S47J::Str(O, TEXT("place")).Left(64); X.Night = S47J::Int(O, TEXT("night")); X.Start = S47J::Num(O, TEXT("start")); X.bLooked = S47J::Bool(O, TEXT("looked")); X.bGone = true; // a Continue finds them gone
			if (!X.Place.IsEmpty() && X.Night > 0) Visits.Add(X);
		}
	}
	bLive = M != nullptr;
	if (M && IsValid(M->Web.Get()) && Scene.Get() != M->Web.Get()) { bBuilt = false; VanParts.Reset(); HeliParts.Reset(); CarParts.Reset(); }
}

void FS47Visitors::Stop()
{
	bLive = false;
	if (!IsValid(Scene.Get())) return;
	ShowVan(false);
	for (const auto& P : HeliParts) if (auto* C = P.Get()) C->SetHiddenInGame(true);
	for (const auto& P : CarParts) if (auto* C = P.Get()) C->SetHiddenInGame(true);
	if (auto* C = Porch.Get()) C->SetHiddenInGame(true);
}

S47J::FObj FS47Visitors::Save() const
{
	auto O = S47J::New();
	if (!BacaTold.IsEmpty()) { TArray<FString> A = BacaTold.Array(); A.Sort(); S47J::Set(O, TEXT("bacaTold"), A); }
	if (!Flags.IsEmpty()) { TArray<FString> A = Flags.Array(); A.Sort(); S47J::Set(O, TEXT("flags"), A); }
	if (BacaDay) { S47J::Set(O, TEXT("bacaDay"), BacaDay); S47J::Set(O, TEXT("bacaLast"), BacaLast); }
	if (BacaBack) S47J::Set(O, TEXT("bacaBack"), BacaBack);
	if (BacaCheckedNight) S47J::Set(O, TEXT("bacaNight"), BacaCheckedNight);
	if (VanChoiceDay) { S47J::Set(O, TEXT("vanDay"), VanChoiceDay); S47J::Set(O, TEXT("vanChoice"), VanChoice); }
	if (!OutPlace.IsEmpty()) { S47J::Set(O, TEXT("outPlace"), OutPlace); S47J::Set(O, TEXT("outNight"), OutNight); }
	if (CheckedNight) S47J::Set(O, TEXT("checkedNight"), CheckedNight);
	if (!Visits.IsEmpty())
	{
		TArray<TSharedPtr<FJsonValue>> A;
		for (const FVisit& V : Visits) { auto X = S47J::New(); S47J::Set(X, TEXT("place"), V.Place); S47J::Set(X, TEXT("night"), V.Night); S47J::Set(X, TEXT("start"), V.Start); if (V.bLooked) S47J::Set(X, TEXT("looked"), true); A.Add(MakeShared<FJsonValueObject>(X)); }
		O->SetArrayField(TEXT("visits"), A);
	}
	return O;
}

// ---------- stand-ins ----------

bool FS47Visitors::Build()
{
	using namespace S47VisitorsLocal;
	if (bBuilt) return true;
	if (!M || !IsValid(M->Web.Get()) || !IsValid(M->Inter)) return false;
	AS47Web* W = M->Web.Get();
	const int32 Road = W->Handle(TEXT("world.legs.road.area.group")), Saro = W->Handle(TEXT("ext.group"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Ball = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* Lit = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
	if (Road < 0 || Saro < 0 || !Cube || !Cyl || !Ball || !Lit) return false;
	UStaticMesh* Sedan = FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(SedanPath))) ? LoadObject<UStaticMesh>(nullptr, SedanPath) : nullptr;
	Scene = M->Web;
	const auto Mat = [W, Lit](uint32 Hex, float Rough = .8f) { auto* D = UMaterialInstanceDynamic::Create(Lit, W); D->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(Hex)); D->SetScalarParameterValue(TEXT("Roughness"), Rough); D->SetScalarParameterValue(TEXT("NativeLighting"), 1); return D; };
	TMap<UStaticMeshComponent*, int32> Nodes;
	const auto Part = [&](const FString& Name, UStaticMesh* Mesh, uint32 Hex, int32 Root, bool bUse) -> UStaticMeshComponent*
	{
		auto* C = NewObject<UStaticMeshComponent>(W, *Name);
		C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(W->GetRootComponent()); C->SetStaticMesh(Mesh); C->SetCastShadow(true);
		if (Hex) C->SetMaterial(0, Mat(Hex));
		C->SetCollisionEnabled(bUse ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision); C->SetCollisionResponseToAllChannels(ECR_Ignore);
		if (bUse) { C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block); }
		C->RegisterComponent(); C->SetHiddenInGame(true);
		Nodes.Add(C, W->AdoptNode(C, Root, Name));
		return C;
	};
	const TWeakPtr<uint8> Guard = Alive;
	const auto Usable = [&](UStaticMeshComponent* C, const FString& Id, TFunction<FString()> Label, TFunction<void()> Use)
	{
		const int32 Node = Nodes.FindRef(C);
		FS47Interactable I; I.Id = Id; I.Object = W->ObjAt(Node); I.Range = 3.5; I.Label = Label; I.Use = Use; I.Examine = Label;
		M->Inter->Add(I); if (Node >= 0) W->MarkUsable(Node, Id);
	};
	// The van: a box with a stripe, four wheels, words on both sides.
	{
		const FVector Base = S47Three::ToUE(FVector(8000 + VanX, S47FarmWorld::FloorRoad(VanX, VanZ), VanZ));
		const auto Put = [&](UStaticMeshComponent* C, const FVector& Off, const FVector& Size) { C->SetWorldLocation(Base + Off * 100.0); C->SetWorldScale3D(Size); VanParts.Add(C); };
		UStaticMeshComponent* Body = Part(TEXT("S47VisitVan"), Cube, 0xd8ccb0, Road, true);
		Put(Body, FVector(0, 0, 1.4), FVector(2.0, 5.4, 2.0));
		Put(Part(TEXT("S47VisitVanStripe"), Cube, 0x3c6e8f, Road, false), FVector(0, 0, 1.2), FVector(2.02, 5.42, .3));
		for (const double Sx : { -.85, .85 }) for (const double Sy : { -1.7, 1.7 })
		{
			UStaticMeshComponent* Wheel = Part(FString::Printf(TEXT("S47VisitVanWheel%d%d"), int32(Sx > 0), int32(Sy > 0)), Cyl, 0x1c1b1a, Road, false);
			Put(Wheel, FVector(Sx, Sy, .38), FVector(.76, .76, .25)); Wheel->SetWorldRotation(FRotator(0, 0, 90));
		}
		for (const double Side : { -1.0, 1.0 })
		{
			auto* T = NewObject<UTextRenderComponent>(W, *FString::Printf(TEXT("S47VisitVanWords%d"), int32(Side > 0)));
			T->SetMobility(EComponentMobility::Movable); T->SetupAttachment(W->GetRootComponent()); T->SetText(FText::FromString(TEXT("SOCORRO\nSKY WATCH"))); T->SetWorldSize(32.f);
			T->SetHorizontalAlignment(EHTA_Center); T->SetVerticalAlignment(EVRTA_TextCenter); T->SetTextRenderColor(FColor(40, 70, 110)); T->SetCollisionEnabled(ECollisionEnabled::NoCollision); T->RegisterComponent();
			T->SetWorldLocationAndRotation(Base + FVector(Side * 102, 0, 175), FRotator(0, Side > 0 ? 0 : 180, 0)); T->SetHiddenInGame(true);
			W->AdoptNode(T, Road, T->GetName());
			VanText.Add(T);
		}
		Usable(Body, TEXT("ff:visit:van"), [this, Guard] { return Guard.IsValid() && bLive && VanHere() ? FString(TEXT("Painted van / the Socorro Sky Watch")) : FString(); }, [this, Guard] { if (Guard.IsValid() && bLive && VanHere()) OpenVan(); });
	}
	// Baca's porch: his door, on the house's south side (S47TownSites' "baca").
	if (const S47TownSites::FSite* Site = S47TownSites::Find(TEXT("baca")))
	{
		const FVector2D Door = S47TownSites::ToRoad(*Site, Site->EntryX, Site->EntryZ - .55);
		UStaticMeshComponent* C = Part(TEXT("S47VisitBacaDoor"), Cube, 0x5a4330, Road, true);
		C->SetWorldLocation(S47Three::ToUE(FVector(8000 + Door.X, S47FarmWorld::FloorRoad(Door.X, Door.Y) + 1.05, Door.Y))); C->SetWorldScale3D(FVector(1.0, .08, 2.1)); C->SetHiddenInGame(false);
		Porch = C;
		Usable(C, TEXT("ff:visit:baca"), [this, Guard] { return Guard.IsValid() && bLive ? FString(BacaGone() ? TEXT("The Baca door / it stands open") : TEXT("Eloy Baca / knock")) : FString(); }, [this, Guard] { if (Guard.IsValid() && bLive) OpenBaca(); });
	}
	// Two sets each of the cars and the helicopter, one for each area a place can be in (the road's land, SARO's).
	for (const int32 Root : { Road, Saro })
	{
		const FString Tag = Root == Road ? TEXT("Road") : TEXT("Saro");
		for (int32 I = 0; I < 2; ++I)
		{
			UStaticMeshComponent* Car = Part(FString::Printf(TEXT("S47VisitSedan%s%d"), *Tag, I), Sedan ? Sedan : Cube, Sedan ? 0 : 0x101012, Root, true);
			if (Sedan) { auto* Black = Mat(0x0d0d10, .35f); for (int32 S = 0; S < Car->GetNumMaterials(); ++S) Car->SetMaterial(S, Black); }
			CarParts.Add(Car);
		}
		HeliParts.Add(Part(TEXT("S47VisitHeliBody") + Tag, Ball, 0x3d4232, Root, false));
		HeliParts.Add(Part(TEXT("S47VisitHeliTail") + Tag, Cube, 0x3d4232, Root, false));
		HeliParts.Add(Part(TEXT("S47VisitHeliRotor") + Tag, Cyl, 0x1a1a18, Root, false));
	}
	bBuilt = true;
	UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors ready: van, Baca's door, %s sedans, helicopter stand-ins"), Sedan ? TEXT("model") : TEXT("box"));
	return true;
}

void FS47Visitors::ShowVan(bool bShow)
{
	for (const auto& P : VanParts) if (auto* C = P.Get()) { C->SetHiddenInGame(!bShow); }
	for (const auto& T : VanText) if (auto* C = T.Get()) C->SetHiddenInGame(!bShow);
	if (VanParts.Num() && VanParts[0].Get()) VanParts[0]->SetCollisionEnabled(bShow ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

// ---------- step ----------

void FS47Visitors::Step(double Dt)
{
	if (!M || !M->Freeform || !bLive) return;
	if (!bBuilt && !Build()) return;
	if (Scene.Get() != M->Web.Get()) { bBuilt = false; VanParts.Reset(); VanText.Reset(); HeliParts.Reset(); CarParts.Reset(); return; }
	if (auto* C = Porch.Get()) C->SetHiddenInGame(false);
	ShowVan(VanHere());
	StepMilitary(Dt);
	StepIn -= Dt;
	if (StepIn > 0) return;
	StepIn = 5;
	FS47FreeformSession& F = *M->Freeform;
	const int32 Today = F.Day(), Night = FS47FreeformSession::NightOf(F.Clock());
	const double H = Hour();
	if (BacaBack > 0 && (Today > BacaBack || (Today == BacaBack && H >= 8))) { BacaBack = 0; UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors: Baca home again")); M->RequestAutosave(); }
	if (H < 4 || H >= 8 || !F.EventPlaces || !F.RulesLive()) return;
	// After a night: Baca taken by what came to his place; the Sky Watch's night out on their own; the military's morning.
	if (BacaCheckedNight != Night)
	{
		BacaCheckedNight = Night;
		if (BacaBack == 0 && F.EventPlaces->LastNight(TEXT("baca")) == Night && FS47NightRhythm::Roll(F.Seed(), Night, 0x42414341u) < .34f) VanishBaca();
	}
	if (!OutPlace.IsEmpty() && OutNight == Night && !Flags.Contains(TEXT("van_back")) && !Flags.Contains(TEXT("van_empty")))
	{
		// One time in three they are not heard from again: the van stands empty out there, doors open.
		if (FS47NightRhythm::Roll(F.Seed(), Night, 0x4f555421u) < .34f) Note(TEXT("van_empty")); else Note(TEXT("van_back"));
		if (F.Places && FS47Places::Def(OutPlace)) F.Places->Hear(OutPlace);
	}
	if (CheckedNight == Night) return;
	CheckedNight = Night;
	const bool bKnown = Flags.Contains(TEXT("let_in")) || Flags.Contains(TEXT("tie_seen"));
	for (const FS47EventPlace& P : F.EventPlaces->All())
	{
		if (F.EventPlaces->LastNight(P.Id) != Night || !Big(F.EventPlaces->LastKind(P.Id)) || !Comes(F.Seed(), Night, P.Id, bKnown)) continue;
		FVisit V; V.Place = P.Id; V.Night = Night; V.Start = FMath::FloorToDouble(F.Clock() / 86400.) * 86400. + 6 * 3600;
		// The player already standing at it before they come: nothing to take before it is seen.
		V.bLooked = F.EventPlaces->DistanceFromPlayer(P) >= 0 && F.EventPlaces->DistanceFromPlayer(P) < 60 && F.Clock() < V.Start;
		Visits.Add(V);
		UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors: the military at %s after night %d (%s)"), *P.Id, Night, *F.EventPlaces->LastKind(P.Id));
		M->RequestAutosave();
		break;
	}
}

void FS47Visitors::PlaceCars(const FVisit& V, bool bShow)
{
	const FS47EventPlace* P = M->Freeform->EventPlaces ? M->Freeform->EventPlaces->Find(V.Place) : nullptr;
	const FString Area = P ? FS47EventPlaces::AreaOf(*P) : FString();
	for (int32 I = 0; I < CarParts.Num(); ++I)
	{
		UStaticMeshComponent* C = CarParts[I].Get();
		if (!C) continue;
		const bool bSaroSet = I >= 2, bMine = bShow && P && ((Area == TEXT("saro")) == bSaroSet) && Area != TEXT("station01");
		C->SetHiddenInGame(!bMine);
		C->SetCollisionEnabled(bMine ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		if (!bMine) continue;
		const double X = P->X + 9 + (I % 2) * 4.5, Z = P->Z + 6;
		if (C->bHiddenInGame == false && !bCarsLogged) { bCarsLogged = true; UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors: black sedans at %s, road %.0f %.0f, area %s"), *V.Place, X, Z, *Area); }
		const FVector At = S47OpenLand::FromRoad(Area, X, Z);
		C->SetWorldLocationAndRotation(S47Three::ToUE(FVector(At.X, S47FarmWorld::FloorRoad(X, Z) + (C->GetStaticMesh() && C->GetStaticMesh()->GetName() == TEXT("Cube") ? .7 : 0), At.Z)), FRotator(0, 90 + I * 6, 0));
		if (C->GetStaticMesh() && C->GetStaticMesh()->GetName() == TEXT("Cube")) C->SetWorldScale3D(FVector(4.8, 1.8, 1.4));
	}
}

void FS47Visitors::PlaceHeli(const FVisit& V, double Age)
{
	const FS47EventPlace* P = M->Freeform->EventPlaces ? M->Freeform->EventPlaces->Find(V.Place) : nullptr;
	const FString Area = P ? FS47EventPlaces::AreaOf(*P) : FString();
	const bool bFly = P && Age >= 0 && Age <= 80 && Area != TEXT("station01");
	// North to south over the place at 45 m, lower over it, one pass at about 45 m/s.
	const double K = FMath::Clamp(Age / 80., 0., 1.);
	const double X = P ? P->X - 200 + 400 * K : 0, Z = P ? P->Z - 1800 + 3600 * K : 0;
	const double Over = P ? FMath::Abs(Z - P->Z) : 1e9, Up = 30 + 25 * FMath::Clamp(Over / 600., 0., 1.);
	const FVector At = bFly ? S47OpenLand::FromRoad(Area, X, Z) : FVector::ZeroVector;
	const double Y = bFly ? S47FarmWorld::FloorRoad(X, Z) + Up : 0;
	for (int32 I = 0; I < HeliParts.Num(); ++I)
	{
		UStaticMeshComponent* C = HeliParts[I].Get();
		if (!C) continue;
		const bool bSaroSet = I >= 3, bMine = bFly && ((Area == TEXT("saro")) == bSaroSet);
		C->SetHiddenInGame(!bMine);
		if (!bMine) continue;
		const FVector Base = S47Three::ToUE(FVector(At.X, Y, At.Z));
		const int32 Kind = I % 3;
		if (Kind == 0) { C->SetWorldLocationAndRotation(Base, FRotator(0, 90, 0)); C->SetWorldScale3D(FVector(4.5, 2.0, 2.0)); }
		else if (Kind == 1) { C->SetWorldLocationAndRotation(Base + FVector(0, -480, 40), FRotator(0, 90, 0)); C->SetWorldScale3D(FVector(5.0, .35, .45)); }
		else { C->SetWorldLocationAndRotation(Base + FVector(0, 0, 130), FRotator(0, Age * 1400.0, 0)); C->SetWorldScale3D(FVector(11, 11, .06)); }
	}
	// The thump of the blades, louder as it comes over (the engine's own beeper until there is a recording).
	if (bFly && M->Audio && M->Web && M->Web->Area() != TEXT("") && M->Player)
	{
		FVector2D Me; const bool bMe = M->Freeform->EventPlaces->PlayerRoad(Me);
		const double D = bMe ? FVector2D::Distance(Me, FVector2D(X, Z)) : 1e9;
		ThumpIn -= 1.0 / 60.0;
		if (D < 2500 && ThumpIn <= 0) { ThumpIn = .19; M->Audio->Beep(52.f, .06f, float(.14 * FMath::Clamp(1 - D / 2500., 0., 1.)), 0.f, TEXT("sine")); }
	}
}

void FS47Visitors::StepMilitary(double Dt)
{
	FS47FreeformSession& F = *M->Freeform;
	const FVisit* Live = nullptr;
	for (FVisit& V : Visits)
	{
		if (V.bGone) continue;
		const double Since = F.Clock() - V.Start;
		const FS47EventPlace* P = F.EventPlaces ? F.EventPlaces->Find(V.Place) : nullptr;
		const double Away = P ? F.EventPlaces->DistanceFromPlayer(*P) : -1;
		// Before they come: the player there first sees the ground as it was.
		if (Since < 0) { if (Away >= 0 && Away < 40) V.bLooked = true; continue; }
		if (Since > 5 * 3600 || (Away >= 0 && Away < 35))
		{
			// They pull out as the player comes near, or are gone by late morning; the ground they leave behind says so.
			if (Away >= 0 && Away < 35 && M->UI && !V.bLooked) M->UI->Toast(TEXT("Two black sedans pull out onto the road and go, unhurried. No plates you can read."), 6);
			V.bGone = true;
			Note(TEXT("military_seen"));
			continue;
		}
		Live = &V;
	}
	// The ground they went over, read when the player gets to it.
	for (FVisit& V : Visits)
	{
		if (V.bLooked || F.Clock() < V.Start) continue;
		const FString Id = FString::Printf(TEXT("military_%s_%d"), *V.Place, V.Night);
		if (F.HasFinding(Id)) continue;
		const FS47EventPlace* P = F.EventPlaces ? F.EventPlaces->Find(V.Place) : nullptr;
		if (!P || F.EventPlaces->DistanceFromPlayer(*P) < 0 || F.EventPlaces->DistanceFromPlayer(*P) > 30) continue;
		const FString Text = FString::Printf(TEXT("%s, the morning after: tire tracks over everything, the ground raked smooth in a wide circle, and a cigarette end of a brand nobody sells in this county. Whatever was there has been taken."), *PlaceTitle(V.Place));
		F.RegisterFinding(Id, Text);
		if (M->UI) M->UI->Open(S47Kit::Shell(M->UI, TEXT("SOMEBODY GOT HERE FIRST"), S47Kit::Col({ S47Kit::Text(Text), S47Kit::Small(TEXT("Kept as a field record.")) }), 760));
		// Cleared() stays true for this night: the marks were gone before the player came.
		V.bGone = true;
		break;
	}
	if (Live)
	{
		HeliAge = HeliAge < 0 ? 0 : HeliAge + Dt;
		PlaceHeli(*Live, HeliAge);
		PlaceCars(*Live, HeliAge > 20);
	}
	else
	{
		HeliAge = -1;
		for (const auto& P : HeliParts) if (auto* C = P.Get()) C->SetHiddenInGame(true);
		for (const auto& P : CarParts) if (auto* C = P.Get()) { C->SetHiddenInGame(true); C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	}
}

// ---------- the neighbor ----------

bool FS47Visitors::VanishBaca()
{
	if (!M || !M->Freeform || BacaBack > 0) return false;
	BacaBack = M->Freeform->Day() + 2 + int32(FS47NightRhythm::Roll(M->Freeform->Seed(), M->Freeform->Day(), 0x42414b21u) * 3.f) % 3;
	UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors: Baca gone until day %d"), BacaBack);
	M->RequestAutosave();
	return true;
}

FS47Visitors::FTale FS47Visitors::Tale(int32 Day) const
{
	using namespace S47VisitorsLocal;
	FTale T;
	if (!M || !M->Freeform) return T;
	const FS47FreeformSession& F = *M->Freeform;
	const int32 Night = FS47FreeformSession::NightOf(F.Clock());
	// Something lately: he saw it, or says he did, and as often as not puts it somewhere else or on another night.
	if (F.EventPlaces)
	{
		const FS47EventPlace* Best = nullptr; int32 BestNight = 0;
		for (const FS47EventPlace& P : F.EventPlaces->All()) { const int32 L = F.EventPlaces->LastNight(P.Id); if (L > BestNight && L >= Night - 3 && P.Id != TEXT("baca")) { Best = &P; BestNight = L; } }
		if (Best && !BacaTold.Contains(FString::Printf(TEXT("event:%s:%d"), *Best->Id, BestNight)))
		{
			T.Key = FString::Printf(TEXT("event:%s:%d"), *Best->Id, BestNight);
			const float Roll = FS47NightRhythm::Roll(F.Seed(), BestNight, FCrc::StrCrc32(*Best->Id) ^ 0x4c494521u);
			FString Where = Best->Title; int32 When = BestNight;
			if (Roll < .5f)
			{
				// Wrong: the open place nearest to it that is not it, or the night before.
				T.bTrue = false;
				const FS47EventPlace* Near = nullptr; double D = 1e9;
				for (const FS47EventPlace& P : F.EventPlaces->All()) { const double E = FVector2D::Distance(FVector2D(P.X, P.Z), FVector2D(Best->X, Best->Z)); if (P.Id != Best->Id && P.Id != TEXT("baca") && E < D) { D = E; Near = &P; } }
				if (Near && Roll < .3f) Where = Near->Title; else When = FMath::Max(1, BestNight - 1);
			}
			T.Place = Best->Id;
			T.Text = FString::Printf(TEXT("%s night there were lights down over %s. I watched them from this porch with my coffee. Then they went up, slow, like they had all night."), *WeekdayOf(When), *Where);
			return T;
		}
	}
	// Every third day, somewhere to go and look: half the time real, half the time nowhere at all.
	if (Day % 3 == 0)
	{
		const bool bReal = FS47NightRhythm::Roll(F.Seed(), Day, 0x4c4f4f4bu) < .5f;
		const FString New = bReal && F.Places ? F.Places->NextUnknown(Day * 17) : FString();
		if (!New.IsEmpty() && !BacaTold.Contains(TEXT("look:") + New)) { T.Key = TEXT("look:") + New; T.Place = New; T.Text = FString::Printf(TEXT("You want to look at something, go out to %s. Go in the daytime. I mean that."), *PlaceTitle(New)); return T; }
		const FBunk& B = BacaBunk[Day % 3];
		if (!BacaTold.Contains(FString(TEXT("bunk:")) + B.Label)) { T.Key = FString(TEXT("bunk:")) + B.Label; T.bTrue = false; T.Text = FString::Printf(TEXT("Nobody goes where I'm going to tell you. %s. It's on no map, so I'll put it on yours."), B.Label); return T; }
	}
	for (int32 I = 0; I < NumTales; ++I)
	{
		const int32 K = (I + Day) % NumTales;
		if (BacaTold.Contains(FString::Printf(TEXT("old:%d"), K))) continue;
		T.Key = FString::Printf(TEXT("old:%d"), K); T.Text = OldTales[K]; T.bTrue = false; return T;
	}
	T.Key = TEXT("repeat"); T.Text = OldTales[Day % NumTales]; T.bTrue = false;
	return T;
}

void FS47Visitors::OpenBaca()
{
	if (!M || !M->UI || !M->Freeform) return;
	FS47FreeformSession& F = *M->Freeform;
	TArray<TSharedRef<SWidget>> Rows;
	if (BacaGone())
	{
		Rows.Add(S47Kit::Paper(TEXT("The door stands open. Coffee on the stove, cold, and a cup poured and not drunk. The dog is under the porch and will not come out for you. The truck is in the shed.")));
		const FString Id = FString::Printf(TEXT("baca_gone_%d"), BacaBack);
		if (!F.HasFinding(Id)) F.RegisterFinding(Id, TEXT("The Baca place under the east rim: the door open, coffee poured and cold, the truck in the shed, Eloy Baca nowhere."));
		Rows.Add(S47Kit::Small(TEXT("Kept as a field record.")));
		Rows.Add(S47Kit::Button(TEXT("Leave it"), [this] { if (M && M->UI) M->UI->Close(); }));
		M->UI->Open(S47Kit::Shell(M->UI, TEXT("THE BACA PLACE"), S47Kit::Col(Rows), 780));
		return;
	}
	Rows.Add(S47Kit::Paper(TEXT("An old man in a pressed shirt comes to the screen door before you knock. \"You're from the dishes. Sit down. I've been waiting for one of you to come out here for four years.\"")));
	FString Said = BacaLast;
	if (BacaDay != F.Day() || BacaLast.IsEmpty())
	{
		const FTale T = Tale(F.Day());
		Said = T.Text; BacaLast = T.Text; BacaDay = F.Day(); BacaTold.Add(T.Key);
		if (T.Key.StartsWith(TEXT("look:")) && F.Places) F.Places->Hear(T.Place);
		if (T.Key != TEXT("repeat")) { const FString Id = TEXT("baca_") + T.Key.Replace(TEXT(":"), TEXT("_")); if (!F.HasFinding(Id)) F.RegisterFinding(Id, TEXT("Eloy Baca says (unconfirmed): ") + T.Text); }
		UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors: Baca tells %s (%s)"), *T.Key, T.bTrue ? TEXT("as it was") : TEXT("not as it was"));
		M->RequestAutosave();
	}
	Rows.Add(S47Kit::Text(TEXT("ELOY BACA: ") + Said));
	Rows.Add(S47Kit::Small(TEXT("Kept as a field record, unconfirmed. He is sure of it. He was sure of the last one too.")));
	Rows.Add(S47Kit::Button(TEXT("Thank him and go"), [this] { if (M && M->UI) M->UI->Close(); }));
	M->UI->Open(S47Kit::Shell(M->UI, TEXT("THE BACA PLACE / ELOY BACA"), S47Kit::Col(Rows), 820));
}

// ---------- the enthusiasts ----------

bool FS47Visitors::VanHere() const
{
	if (!M || !M->Freeform || Flags.Contains(TEXT("van_empty"))) return false;
	const int32 Today = M->Freeform->Day();
	const double H = Hour();
	return (VanDay(M->Freeform->Seed(), Today) || ForcedVanDay == Today) && H >= 10 && H < 20;
}

void FS47Visitors::DevVan() { if (M && M->Freeform) { ForcedVanDay = M->Freeform->Day(); Flags.Remove(TEXT("van_empty")); VanChoiceDay = 0; } }

void FS47Visitors::OpenVan()
{
	if (!M || !M->UI || !M->Freeform) return;
	FS47FreeformSession& F = *M->Freeform;
	TArray<TSharedRef<SWidget>> Rows;
	Rows.Add(S47Kit::Paper(TEXT("THE SOCORRO SKY WATCH, painted by hand on the side of a cream van. Three of them: a retired schoolteacher with binoculars on a strap, her nephew with a tape recorder, and a quiet man in the back seat in a tie, who doesn't get out.")));
	if (VanChoiceDay == F.Day())
	{
		Rows.Add(S47Kit::Text(VanChoice == TEXT("in") ? TEXT("\"Tonight, then! We'll be at your gate at nine sharp.\" The nephew is already checking his batteries.")
			: VanChoice == TEXT("away") ? TEXT("\"Then we'll find our own.\" The schoolteacher unfolds a map with a circle drawn on it.") : TEXT("The back seat is empty now. The schoolteacher says he got a ride.")));
	}
	else
	{
		Rows.Add(S47Kit::Text(TEXT("THE SCHOOLTEACHER: We're scientists, more or less. We only want to see the instruments. One night. We'll bring donuts. We'll make a donation.")));
		Rows.Add(S47Kit::Button(TEXT("Let them in tonight / a $15 donation"), [this]
		{
			FS47FreeformSession& G = *M->Freeform; VanChoiceDay = G.Day(); VanChoice = TEXT("in"); G.Earn(15); Note(TEXT("let_in"));
			// Their newsletter, mimeographed: a rumor in it, and the quiet one has seen the station's records.
			const FString New = G.Places ? G.Places->NextUnknown(G.Day() * 29) : FString();
			if (!New.IsEmpty()) G.Places->Hear(New);
			if (M->UI) M->UI->Toast(New.IsEmpty() ? TEXT("$15 and a newsletter. The quiet one asked to see the logbooks, and you let him.") : FString::Printf(TEXT("$15 and their newsletter. It says to watch %s. The quiet one asked to see the logbooks, and you let him."), *PlaceTitle(New)), 8);
			OpenVan();
		}));
		Rows.Add(S47Kit::Button(TEXT("Turn them down"), [this]
		{
			FS47FreeformSession& G = *M->Freeform; VanChoiceDay = G.Day(); VanChoice = TEXT("away"); Note(TEXT("turned_away"));
			// They go out on their own tonight, to somewhere far.
			FS47PlaceNeed Need; Need.Kind = TEXT("skywatch"); Need.All = ES47PlaceTrait::Far; Need.FarShare = 1.f;
			const FS47PlacePick Pick = G.EventPlaces ? G.EventPlaces->Pick(Need) : FS47PlacePick();
			OutPlace = Pick.Id; OutNight = FS47FreeformSession::NightOf(G.Clock()) + (Hour() >= 8 ? 0 : -1);
			if (!OutPlace.IsEmpty()) Flags.Remove(TEXT("van_back"));
			UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors: the Sky Watch goes out to %s tonight"), *OutPlace);
			OpenVan();
		}));
		Rows.Add(S47Kit::Button(TEXT("Ask who the quiet one in the back is"), [this]
		{
			FS47FreeformSession& G = *M->Freeform; VanChoiceDay = G.Day(); VanChoice = TEXT("tie"); Note(TEXT("tie_seen"));
			if (M->UI) M->UI->Toast(TEXT("\"From Albuquerque,\" he says. His shoes are from somewhere else. He gets out, thanks them for the ride, and walks off toward the highway."), 8);
			OpenVan();
		}));
	}
	Rows.Add(S47Kit::Button(TEXT("Leave them to it"), [this] { if (M && M->UI) M->UI->Close(); }));
	M->UI->Open(S47Kit::Shell(M->UI, TEXT("MESA DINER LOT / THE SKY WATCH"), S47Kit::Col(Rows), 840));
}

// ---------- the map ----------

void FS47Visitors::FillMap(FS47MapView& V) const
{
	using namespace S47VisitorsLocal;
	double SaroX = 0, SaroZ = 0;
	S47OpenLand::RoadOf(TEXT("saro"), 9, 0, SaroX, SaroZ);
	for (const FBunk& B : BacaBunk) if (BacaTold.Contains(FString(TEXT("bunk:")) + B.Label)) { FS47MapPin P; P.East = B.X; P.South = B.Z - SaroZ; P.Label = B.Label; P.bHeard = true; V.Pins.Add(P); }
}

// ---------- F10 ----------

bool FS47Visitors::DevMilitary(const FString& Place)
{
	if (!M || !M->Freeform || !M->Freeform->EventPlaces || !M->Freeform->EventPlaces->Find(Place)) return false;
	FVisit V; V.Place = Place; V.Night = FS47FreeformSession::NightOf(M->Freeform->Clock()); V.Start = M->Freeform->Clock();
	Visits.RemoveAll([&V](const FVisit& X) { return X.Place == V.Place && X.Night == V.Night; });
	Visits.Add(V);
	return true;
}

// ---------- tests ----------

bool FS47Visitors::SelfTest(TArray<FString>& Lines)
{
	using namespace S47VisitorsLocal;
	bool bAll = true;
	const auto Check = [&](bool bOk, const FString& What) { bAll &= bOk; Lines.Add(FString(bOk ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("visitors: ") + What); };
	int32 Vans = 0; bool bEarly = false;
	for (int32 D = 1; D <= 60; ++D) { Vans += VanDay(47, D) ? 1 : 0; bEarly |= D < 4 && VanDay(47, D); }
	Check(!bEarly && Vans >= 5 && Vans <= 22, FString::Printf(TEXT("the Sky Watch comes %d days of 60, never before day 4"), Vans));
	Check(Big(TEXT("site_visit")) && Big(TEXT("landing_trace")) && !Big(TEXT("peek")) && !Big(TEXT("door_visit")), TEXT("the military come after visits and landings, not after a face at a window"));
	int32 Plain = 0, Known = 0;
	for (int32 N = 1; N <= 200; ++N) { Plain += Comes(47, N, TEXT("farm"), false) ? 1 : 0; Known += Comes(47, N, TEXT("farm"), true) ? 1 : 0; }
	Check(Plain > 30 && Plain < 90 && Known > Plain, FString::Printf(TEXT("they come after %d big nights in 200, %d once the station's records have been seen"), Plain, Known));
	{
		FS47Visitors A(nullptr); A.BacaTold.Add(TEXT("old:3")); A.Flags.Add(TEXT("let_in")); A.BacaDay = 5; A.BacaLast = TEXT("A story."); A.BacaBack = 9; A.VanChoiceDay = 6; A.VanChoice = TEXT("in");
		FVisit V; V.Place = TEXT("farm"); V.Night = 4; V.Start = 4 * 86400.; A.Visits.Add(V);
		FS47Visitors B(nullptr); B.Begin(S47J::Read(S47J::Write(A.Save())));
		Check(B.BacaGone() && B.Has(TEXT("let_in")) && B.Cleared(TEXT("farm"), 4) && !B.Cleared(TEXT("farm"), 5) && S47J::Write(B.Save()) == S47J::Write(A.Save()), TEXT("Baca gone, the van's answer and the military's morning survive a Continue"));
		FS47Visitors None(nullptr); None.Begin(nullptr);
		Check(None.Save()->Values.Num() == 0, TEXT("an older save has had no visitors and writes nothing"));
	}
	{
		bool bClean = true; FString Bad;
		TArray<FString> All;
		for (const TCHAR* T : OldTales) All.Add(T);
		for (const FBunk& B : BacaBunk) { All.Add(B.Label); bClean &= S47OpenLand::InRim(B.X, B.Z, 30); }
		for (const FString& S : All)
		{
			FString Word; const FString Lower = S.ToLower() + TEXT(" ");
			for (const TCHAR Ch : Lower) { if (FChar::IsAlpha(Ch)) { Word.AppendChar(Ch); continue; } for (const TCHAR* N : NotSaid) if (Word == N) { bClean = false; Bad = Word; } Word.Reset(); }
			bClean &= !S.Contains(TEXT("1947")) && !S.Contains(TEXT("'47"));
		}
		Check(bClean, bClean ? FString(TEXT("Baca's stories: nothing of 1947, no brands, American spelling, his places on the land")) : TEXT("not said: ") + Bad);
	}
	return bAll;
}

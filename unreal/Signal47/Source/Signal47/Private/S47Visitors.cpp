#include "S47Visitors.h"
#include "S47Drive.h"
#include "S47EventPlaces.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47GameStyle.h"
#include "S47FreeformDirector.h"
#include "S47Geo.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47NightRhythm.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47Audio.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47TownSites.h"
#include "S47Townsfolk.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/PointLightComponent.h"
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
	// M7 (C): from the date the HUD shows, so that a game on the old calendar (day 1 Sunday, April 13) has the right day too.
	FString WeekdayOf(int32 Day, bool bOldCalendar) { return Weekdays[FMath::Clamp(int32(S47Game::GameDate(true, FMath::Max(1, Day), 0, bOldCalendar).GetDayOfWeek()), 0, 6)]; }
	// The van on the Mesa's lot, north of the diner (road-local, clear of its walls and its lot).
	constexpr double VanX = -40, VanZ = 1982;
	// SARO's visitors' lot by the gate (SARO's own x -18.2 to -7.8, z 10.5 to 20.5), road-local: where the Sky Watch pulls in.
	constexpr double GateX = -13 + S47Geo::SARO_IN_ROAD.X, GateZ = 15.5 + S47Geo::SARO_IN_ROAD.Z;
	// The raked circle they leave, and how long it reads as fresh.
	constexpr double RakeRadius = 6, FreshFor = 2 * 86400.;
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
	const TCHAR* SetArea(int32 Set) { return Set == 1 ? TEXT("saro") : TEXT("road"); }
	// The helicopter, in parts (critics: a black disc over the farm by day read as a saucer): an olive drab body, a tail boom with its
	// fin and tail rotor, skids on two cross tubes, a mast and two long thin blades that turn. UE centimetres from the body's middle,
	// nose toward +Y (it flies south); Spin: 1 and 2 the main blades, 3 the tail rotor.
	struct FHeliBit { int32 Mesh; uint32 Hex; FVector Off, Size; int32 Spin; };
	const FHeliBit HeliBits[] = {
		{ 2, 0x56603f, FVector(0, 0, 0), FVector(2.3, 4.4, 2.1), 0 },
		{ 0, 0x56603f, FVector(0, -470, 50), FVector(.36, 5.2, .42), 0 },
		{ 0, 0x56603f, FVector(0, -740, 120), FVector(.1, .8, 1.3), 0 },
		{ 1, 0x40463a, FVector(0, 0, 112), FVector(.22, .22, .5), 0 },
		{ 0, 0x34382f, FVector(0, 0, 140), FVector(11.6, .3, .05), 1 },
		{ 0, 0x34382f, FVector(0, 0, 140), FVector(11.6, .3, .05), 2 },
		{ 0, 0x34382f, FVector(22, -745, 125), FVector(.04, 1.7, .14), 3 },
		{ 0, 0x2f332b, FVector(-100, 30, -135), FVector(.12, 3.8, .1), 0 },
		{ 0, 0x2f332b, FVector(100, 30, -135), FVector(.12, 3.8, .1), 0 },
		{ 0, 0x2f332b, FVector(0, -50, -118), FVector(2.1, .08, .08), 0 },
		{ 0, 0x2f332b, FVector(0, 110, -118), FVector(2.1, .08, .08), 0 } };
	constexpr int32 HeliPer = UE_ARRAY_COUNT(HeliBits);
	// The raked ground: a few overlapping discs for a ragged edge (x, z off the middle and a radius, metres), the rake's strokes across
	// it, and two ruts of tire track in four lengths each that follow the ground.
	const FVector RakeDiscs[] = { FVector(0, 0, 5.6), FVector(1.0, .4, 4.8), FVector(-.9, .7, 4.9), FVector(.3, -1.0, 4.7), FVector(-.5, -.6, 5.2) };
	constexpr int32 RakeDiscN = UE_ARRAY_COUNT(RakeDiscs), RakeStripeN = 8, RakeRutN = 8, RakePer = RakeDiscN + RakeStripeN + RakeRutN;
	int32 SetOf(const FString& Area) { return Area == TEXT("saro") ? 1 : 0; }
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

bool FS47Visitors::MilitaryAfter(int32 Seed, int32 Night, const FString& Place, bool bKnown, double Clock, FVisit& Out)
{
	if (!Comes(Seed, Night, Place, bKnown)) return false;
	// First light on the morning after: six on the calendar day night N ends on. Five hours later they are gone, seen or not.
	Out = FVisit(); Out.Place = Place; Out.Night = Night; Out.Start = double(Night) * 86400. + 6 * 3600.;
	Out.bGone = Clock > Out.Start + 5 * 3600.;
	return true;
}

bool FS47Visitors::Cleared(const FString& Place, int32 Night) const
{
	const double Now = M && M->Freeform ? M->Freeform->Clock() : 1e12;
	return Visits.ContainsByPredicate([&](const FVisit& V) { return V.Place == Place && V.Night == Night && !V.bLooked && Now >= V.Start; });
}

void FS47Visitors::RakeAt(const FString& Place, int32 Night, double RoadX, double RoadZ)
{
	for (FVisit& V : Visits)
		if (V.Place == Place && V.Night == Night && !V.bRake && FMath::IsFinite(RoadX) && FMath::IsFinite(RoadZ))
		{
			V.RakeX = RoadX; V.RakeZ = RoadZ; V.bRake = true;
			UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors: the ground raked at %s, road %.0f %.0f"), *Place, RoadX, RoadZ);
		}
}

bool FS47Visitors::OnHighway(double RoadX, double RoadZ)
{
	// The state highway runs north-south along x = 0 (S47RoadTerrain): its paved lanes and shoulders, and a little more.
	return FMath::Abs(RoadX) < 8 && RoadZ > S47RoadTerrain::HWY.Z0 && RoadZ < S47RoadTerrain::HWY.Z1;
}

bool FS47Visitors::FreeSpot(const FVector2D& From, const FVector2D& Keep, double Clear, int32 Count, double HalfW, double HalfL, double Gap, const TArray<FBox2D>& Solid, TArray<FVector2D>& Out)
{
	const auto Fits = [&](const FVector2D& C)
	{
		for (int32 K = 0; K < Count; ++K)
		{
			const FVector2D At = C + FVector2D(K * Gap, 0);
			if (FVector2D::Distance(At, Keep) < Clear || !S47OpenLand::InRim(At.X, At.Y, 20)) return false;
			const FBox2D Room(At - FVector2D(HalfW + 1, HalfL + 1), At + FVector2D(HalfW + 1, HalfL + 1));
			for (const FBox2D& B : Solid) if (B.bIsValid && B.Intersect(Room)) return false;
			for (const double Sx : { -1.0, 1.0 }) for (const double Sz : { -1.0, 0.0, 1.0 }) if (OnHighway(At.X + Sx * HalfW, At.Y + Sz * HalfL)) return false;
		}
		return true;
	};
	const FVector2D Out0 = (From - Keep).IsNearlyZero() ? FVector2D(1, 0) : (From - Keep).GetSafeNormal();
	const double A0 = FMath::Atan2(Out0.Y, Out0.X);
	for (double R = 0; R <= 40; R += 2)
	{
		const int32 Steps = R <= 0 ? 1 : FMath::Max(8, int32(R * 2));
		for (int32 I = 0; I < Steps; ++I)
		{
			// Straight out from the place first, then round to either side of that.
			const double A = A0 + (I % 2 ? 1 : -1) * double((I + 1) / 2) * (2 * UE_DOUBLE_PI / Steps);
			const FVector2D C = From + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R;
			if (!Fits(C)) continue;
			Out.Reset();
			for (int32 K = 0; K < Count; ++K) Out.Add(C + FVector2D(K * Gap, 0));
			return true;
		}
	}
	return false;
}

bool FS47Visitors::CarsAt(const FS47EventPlace& P, const TArray<FBox2D>& Solid, FVector2D& A, FVector2D& B)
{
	// At its way in, side by side, long way along the land's z; never on the ground they came to go over.
	TArray<FVector2D> Two;
	if (!FreeSpot(FVector2D(P.EntryX, P.EntryZ), FVector2D(P.X, P.Z), S47VisitorsLocal::RakeRadius + 4, 2, .9, 2.4, 3.4, Solid, Two)) return false;
	A = Two[0]; B = Two[1];
	return true;
}

TArray<FBox2D> FS47Visitors::SolidNear(const FString& Area, const FVector2D& Road, double Within) const
{
	TArray<FBox2D> Out;
	const double W2 = Within * Within;
	for (const FBox2D& B : S47TownSites::SolidFootprints()) if (B.ComputeSquaredDistanceToPoint(Road) < W2) Out.Add(B);
	if (!M || !IsValid(M->Web.Get())) return Out;
	// Whatever the walker cannot walk through here (Caliche's walls, SARO's, the farm's, a parked car), in road-local terms.
	for (const FS47Rect& R : M->Web->CollidersMutable(Area))
	{
		if (!R.bEnabled || R.Id.StartsWith(TEXT("visit:")) || S47OpenLand::IsBoundaryLine(R.MinX, R.MaxX, R.MinZ, R.MaxZ) || R.MaxX - R.MinX > 150 || R.MaxZ - R.MinZ > 150) continue;
		double X0 = 0, Z0 = 0, X1 = 0, Z1 = 0;
		if (!S47OpenLand::RoadOf(Area, R.MinX, R.MinZ, X0, Z0) || !S47OpenLand::RoadOf(Area, R.MaxX, R.MaxZ, X1, Z1)) continue;
		const FBox2D B(FVector2D(FMath::Min(X0, X1), FMath::Min(Z0, Z1)), FVector2D(FMath::Max(X0, X1), FMath::Max(Z0, Z1)));
		if (B.ComputeSquaredDistanceToPoint(Road) < W2) Out.Add(B);
	}
	return Out;
}

bool FS47Visitors::CarsSpot(const FString& Place, FVector2D& A, FVector2D& B) const
{
	const FS47EventPlace* P = M && M->Freeform && M->Freeform->EventPlaces ? M->Freeform->EventPlaces->Find(Place) : nullptr;
	if (!P) return false;
	const FString Area = FS47EventPlaces::AreaOf(*P);
	return CarsAt(*P, SolidNear(Area == TEXT("saro") ? Area : FString(TEXT("road")), FVector2D(P->EntryX, P->EntryZ), 70), A, B);
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
	BacaTold.Reset(); Flags.Reset(); Visits.Reset(); VanSpotKey.Empty();
	BacaDay = BacaBack = BacaCheckedNight = VanChoiceDay = ForcedVanDay = OutNight = CheckedNight = VanArrivedDay = 0; BacaLast.Empty(); VanChoice.Empty(); OutPlace.Empty();
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
			const TArray<double> Rake = S47J::Nums(O, TEXT("rake"));
			if (Rake.Num() == 2 && FMath::IsFinite(Rake[0]) && FMath::IsFinite(Rake[1]) && FMath::Abs(Rake[0]) < 1e5 && FMath::Abs(Rake[1]) < 1e5) { X.RakeX = Rake[0]; X.RakeZ = Rake[1]; X.bRake = true; }
			if (!X.Place.IsEmpty() && X.Night > 0) Visits.Add(X);
		}
	}
	bLive = M != nullptr;
	if (M && IsValid(M->Web.Get()) && Scene.Get() != M->Web.Get()) { bBuilt = false; for (FVanSet& S : Vans) S = FVanSet(); HeliParts.Reset(); CarParts.Reset(); RakeParts.Reset(); Blocks.Reset(); }
}

void FS47Visitors::Stop()
{
	bLive = false;
	if (!IsValid(Scene.Get())) return;
	for (FVanSet& S : Vans)
	{
		for (const auto& P : S.Parts) if (auto* C = P.Get()) { C->SetHiddenInGame(true); C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
		for (const auto& P : S.Open) if (auto* C = P.Get()) C->SetHiddenInGame(true);
		for (const auto& T : S.Words) if (auto* C = T.Get()) C->SetHiddenInGame(true);
		if (auto* L = S.Dome.Get()) L->SetVisibility(false);
	}
	for (const auto& P : HeliParts) if (auto* C = P.Get()) C->SetHiddenInGame(true);
	for (const auto& P : CarParts) if (auto* C = P.Get()) { C->SetHiddenInGame(true); C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	for (const auto& P : RakeParts) if (auto* C = P.Get()) C->SetHiddenInGame(true);
	if (auto* C = Porch.Get()) C->SetHiddenInGame(true);
	if (auto* L = BacaLamp.Get()) L->SetVisibility(false);
	// Out of the walker's and the truck's way again.
	TArray<FString> Ids; Blocks.GetKeys(Ids);
	for (const FString& Id : Ids) { const FBlock B = Blocks[Id]; SetBlock(Id, B.Area, false, FVector2D::ZeroVector, 0, 0); }
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
		for (const FVisit& V : Visits)
		{
			auto X = S47J::New(); S47J::Set(X, TEXT("place"), V.Place); S47J::Set(X, TEXT("night"), V.Night); S47J::Set(X, TEXT("start"), V.Start);
			if (V.bLooked) S47J::Set(X, TEXT("looked"), true);
			if (V.bRake) S47J::Set(X, TEXT("rake"), TArray<double>{ FMath::RoundToDouble(V.RakeX * 10) / 10, FMath::RoundToDouble(V.RakeZ * 10) / 10 });
			A.Add(MakeShared<FJsonValueObject>(X));
		}
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
	const auto Lamp = [W](const FString& Name, float Lumens, double Reach, const FLinearColor& Colour) -> UPointLightComponent*
	{
		auto* L = NewObject<UPointLightComponent>(W, *Name);
		L->SetMobility(EComponentMobility::Movable); L->SetupAttachment(W->GetRootComponent());
		L->SetIntensityUnits(ELightUnits::Lumens); L->SetIntensity(Lumens); L->SetAttenuationRadius(float(Reach * 100.)); L->SetLightColor(Colour); L->SetCastShadows(false);
		L->RegisterComponent(); L->SetVisibility(false);
		return L;
	};
	const TWeakPtr<uint8> Guard = Alive;
	const auto Usable = [&](UStaticMeshComponent* C, const FString& Id, TFunction<FString()> Label, TFunction<void()> Use)
	{
		const int32 Node = Nodes.FindRef(C);
		FS47Interactable I; I.Id = Id; I.Object = W->ObjAt(Node); I.Range = 3.5; I.Label = Label; I.Use = Use; I.Examine = Label;
		M->Inter->Add(I); if (Node >= 0) W->MarkUsable(Node, Id);
	};
	// The van, one in each area: a box with a stripe, four wheels, words on both sides; and for when it stands empty, the side door
	// swung out, the dark doorway, a flashlight on the ground, and the dome light on inside.
	for (int32 S = 0; S < 2; ++S)
	{
		FVanSet& V = Vans[S];
		const int32 Root = S == 1 ? Saro : Road;
		const FString Tag = S == 1 ? TEXT("Saro") : TEXT("");
		const auto Put = [&V](UStaticMeshComponent* C, const FVector& Off, const FVector& Size) { C->SetWorldScale3D(Size); V.Parts.Add(C); V.Off.Add(Off); };
		UStaticMeshComponent* Body = Part(TEXT("S47VisitVan") + Tag, Cube, 0xd8ccb0, Root, true);
		Put(Body, FVector(0, 0, 1.4), FVector(2.0, 5.4, 2.0));
		Put(Part(TEXT("S47VisitVanStripe") + Tag, Cube, 0x3c6e8f, Root, false), FVector(0, 0, 1.2), FVector(2.02, 5.42, .3));
		for (const double Sx : { -.85, .85 }) for (const double Sy : { -1.7, 1.7 })
		{
			UStaticMeshComponent* Wheel = Part(FString::Printf(TEXT("S47VisitVanWheel%s%d%d"), *Tag, int32(Sx > 0), int32(Sy > 0)), Cyl, 0x1c1b1a, Root, false);
			Put(Wheel, FVector(Sx, Sy, .38), FVector(.76, .76, .25)); Wheel->SetWorldRotation(FRotator(0, 0, 90));
		}
		const auto Open = [&V](UStaticMeshComponent* C, const FVector& Off, const FVector& Size) { C->SetWorldScale3D(Size); V.Open.Add(C); V.OpenOff.Add(Off); };
		Open(Part(TEXT("S47VisitVanDoor") + Tag, Cube, 0xd8ccb0, Root, false), FVector(1.62, .95, 1.15), FVector(1.2, .06, 1.7));
		Open(Part(TEXT("S47VisitVanDoorway") + Tag, Cube, 0x0b0b0c, Root, false), FVector(1.012, .4, 1.15), FVector(.03, 1.2, 1.6));
		UStaticMeshComponent* Torch = Part(TEXT("S47VisitVanTorch") + Tag, Cyl, 0x26241f, Root, false);
		Open(Torch, FVector(2.5, -.8, .05), FVector(.06, .06, .26)); Torch->SetWorldRotation(FRotator(0, 0, 90));
		V.Dome = Lamp(TEXT("S47VisitVanDome") + Tag, 420.f, 7, FLinearColor(1.f, .8f, .55f));
		for (const double Side : { -1.0, 1.0 })
		{
			auto* T = NewObject<UTextRenderComponent>(W, *FString::Printf(TEXT("S47VisitVanWords%s%d"), *Tag, int32(Side > 0)));
			T->SetMobility(EComponentMobility::Movable); T->SetupAttachment(W->GetRootComponent()); T->SetText(FText::FromString(TEXT("SOCORRO\nSKY WATCH"))); T->SetWorldSize(32.f);
			T->SetHorizontalAlignment(EHTA_Center); T->SetVerticalAlignment(EVRTA_TextCenter); T->SetTextRenderColor(FColor(40, 70, 110)); T->SetCollisionEnabled(ECollisionEnabled::NoCollision); T->RegisterComponent();
			T->SetWorldRotation(FRotator(0, Side > 0 ? 0 : 180, 0)); T->SetHiddenInGame(true);
			W->AdoptNode(T, Root, T->GetName());
			V.Words.Add(T); V.WordsOff.Add(FVector(Side * 1.02, 0, 1.75));
		}
		const int32 Mine = S;
		Usable(Body, S == 1 ? TEXT("ff:visit:van_saro") : TEXT("ff:visit:van"), [this, Guard, Mine]
		{
			FString Area; FVector2D At; bool bOpen = false;
			if (!Guard.IsValid() || !bLive || !VanWhere(Area, At, bOpen) || SetOf(Area) != Mine) return FString();
			return FString(bOpen ? TEXT("The Sky Watch van / empty, the side door open") : Area == TEXT("saro") ? TEXT("The Sky Watch / at your gate") : TEXT("Painted van / the Socorro Sky Watch"));
		}, [this, Guard, Mine]
		{
			FString Area; FVector2D At; bool bOpen = false;
			if (!Guard.IsValid() || !bLive || !VanWhere(Area, At, bOpen) || SetOf(Area) != Mine) return;
			if (bOpen) OpenVanEmpty(); else if (Area == TEXT("saro")) OpenVanGate(); else OpenVan();
		});
	}
	// Baca's porch: his door, on the house's south side (S47TownSites' "baca"), and the light he leaves on over it until ten.
	if (const S47TownSites::FSite* Site = S47TownSites::Find(TEXT("baca")))
	{
		const FVector2D Door = S47TownSites::ToRoad(*Site, Site->EntryX, Site->EntryZ - .55);
		UStaticMeshComponent* C = Part(TEXT("S47VisitBacaDoor"), Cube, 0x5a4330, Road, true);
		const FVector At = S47Three::ToUE(FVector(8000 + Door.X, S47FarmWorld::FloorRoad(Door.X, Door.Y) + 1.05, Door.Y));
		C->SetWorldLocation(At); C->SetWorldScale3D(FVector(1.0, .08, 2.1)); C->SetHiddenInGame(false);
		PorchHinge = At - FVector(50, 0, 0);
		Porch = C;
		BacaLamp = Lamp(TEXT("S47VisitBacaLamp"), 380.f, 9, FLinearColor(1.f, .74f, .45f));
		if (auto* L = BacaLamp.Get()) L->SetWorldLocation(At + FVector(0, 70, 135));
		Usable(C, TEXT("ff:visit:baca"), [this, Guard] { return Guard.IsValid() && bLive ? FString(BacaGone() ? TEXT("The Baca door / it stands open") : TEXT("Eloy Baca / knock")) : FString(); }, [this, Guard] { if (Guard.IsValid() && bLive) OpenBaca(); });
	}
	// Two sets each of the cars, the helicopter and the raked ground, one for each area a place can be in (the road's land, SARO's).
	for (const int32 Root : { Road, Saro })
	{
		const FString Tag = Root == Road ? TEXT("Road") : TEXT("Saro");
		for (int32 I = 0; I < 2; ++I)
		{
			UStaticMeshComponent* Car = Part(FString::Printf(TEXT("S47VisitSedan%s%d"), *Tag, I), Sedan ? Sedan : Cube, Sedan ? 0 : 0x101012, Root, true);
			if (Sedan) { auto* Black = Mat(0x0d0d10, .35f); for (int32 S = 0; S < Car->GetNumMaterials(); ++S) Car->SetMaterial(S, Black); }
			CarParts.Add(Car);
		}
		for (int32 K = 0; K < HeliPer; ++K)
			HeliParts.Add(Part(FString::Printf(TEXT("S47VisitHeli%s%d"), *Tag, K), HeliBits[K].Mesh == 2 ? Ball : HeliBits[K].Mesh == 1 ? Cyl : Cube, HeliBits[K].Hex, Root, false));
		// The ground gone over: raked earth a shade lighter than the land round it, a ragged edge, the rake's strokes, and the ruts of
		// the cars across it, darker and pressed in.
		for (int32 K = 0; K < RakePer; ++K)
		{
			const uint32 Hex = K < RakeDiscN ? 0x806652 : K < RakeDiscN + RakeStripeN ? 0x6e5847 : 0x4c3c2f;
			UStaticMeshComponent* T = Part(FString::Printf(TEXT("S47VisitRaked%s%d"), *Tag, K), K < RakeDiscN ? Cyl : Cube, Hex, Root, false);
			T->SetCastShadow(false); RakeParts.Add(T);
		}
	}
	bBuilt = true;
	UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors ready: two vans, Baca's door and lamp, %s sedans, helicopter and raked ground stand-ins"), Sedan ? TEXT("model") : TEXT("box"));
	return true;
}

void FS47Visitors::SetBlock(const FString& Id, const FString& Area, bool bOn, const FVector2D& Road, double HalfX, double HalfZ)
{
	if (!M || !IsValid(M->Web.Get())) return;
	FBlock& B = Blocks.FindOrAdd(Id);
	// Unchanged since the last frame: nothing to do (looked at again now and then, in case the lists were made anew).
	if (B.bApplied && B.bOn == bOn && B.Area == Area && (!bOn || B.At.Equals(Road, .01)) && ++B.Since < 120) return;
	B.bApplied = true; B.bOn = bOn; B.At = Road; B.Since = 0;
	B.Area = Area;
	const FVector C = S47OpenLand::FromRoad(Area, Road.X, Road.Y);
	const double MinX = C.X - HalfX, MaxX = C.X + HalfX, MinZ = C.Z - HalfZ, MaxZ = C.Z + HalfZ;
	// The walker's list: found by id, moved and switched.
	TArray<FS47Rect>& Walk = M->Web->CollidersMutable(Area);
	FS47Rect* R = Walk.FindByPredicate([&Id](const FS47Rect& X) { return X.Id == Id; });
	if (!R && bOn) { FS47Rect N; N.Id = Id; R = &Walk.Add_GetRef(N); }
	if (R) { R->bEnabled = bOn; if (bOn) { R->MinX = MinX; R->MaxX = MaxX; R->MinZ = MinZ; R->MaxZ = MaxZ; } }
	// The truck's: others keep places in that list by index, so a box of ours is never removed, only sent far off and brought back.
	IS47DriveArea* D = M->World ? M->World->LegArea(Area) : nullptr;
	if (!D) return;
	constexpr double Away = 1000000;
	FS47Obstacle* O = B.bAdded ? D->Obstacles.FindByPredicate([&B](const FS47Obstacle& X)
	{
		return !X.bCircle && FMath::IsNearlyEqual(X.MinZ, B.MinZ, 1e-6) && FMath::IsNearlyEqual(X.MaxZ, B.MaxZ, 1e-6) && (FMath::IsNearlyEqual(X.MinX, B.MinX, 1e-6) || FMath::IsNearlyEqual(X.MinX, B.MinX + Away, 1e-6));
	}) : nullptr;
	if (!O && !bOn) return;
	if (bOn) { B.MinX = MinX; B.MaxX = MaxX; B.MinZ = MinZ; B.MaxZ = MaxZ; }
	const FS47Obstacle Want = FS47Obstacle::Box(B.MinX + (bOn ? 0 : Away), B.MaxX + (bOn ? 0 : Away), B.MinZ, B.MaxZ);
	if (O) *O = Want; else { D->Obstacles.Add(Want); B.bAdded = true; }
}

bool FS47Visitors::VanWhere(FString& OutArea, FVector2D& OutRoad, bool& bOpen) const
{
	using namespace S47VisitorsLocal;
	bOpen = false;
	if (!M || !M->Freeform) return false;
	const FS47FreeformSession& F = *M->Freeform;
	const int32 Today = F.Day();
	const double H = Hour();
	const auto Spot = [this](const FString& Key, const FString& Area, const FVector2D& From, const FVector2D& Keep, double Clear) -> FVector2D
	{
		if (VanSpotKey != Key)
		{
			TArray<FVector2D> One;
			VanSpotRoad = FreeSpot(From, Keep, Clear, 1, 1.0, 2.7, 0, SolidNear(Area, From, 60), One) ? One[0] : From;
			VanSpotKey = Key;
		}
		return VanSpotRoad;
	};
	// Out where they went, the morning after their night and for three days: standing empty.
	if (Flags.Contains(TEXT("van_empty")))
	{
		const FS47EventPlace* P = OutPlace.IsEmpty() || !F.EventPlaces ? nullptr : F.EventPlaces->Find(OutPlace);
		if (!P || Today > OutNight + 3) return false;
		OutArea = FS47EventPlaces::AreaOf(*P);
		if (OutArea != TEXT("saro") && OutArea != TEXT("road")) return false;
		OutRoad = Spot(TEXT("out:") + OutPlace, OutArea, FVector2D(P->EntryX, P->EntryZ) + FVector2D(9, 6), FVector2D(P->X, P->Z), 6);
		bOpen = true;
		return true;
	}
	// The evening they were let in: at SARO's gate from nine, until they are seen off or it is midnight.
	if (VanChoice == TEXT("in") && VanChoiceDay == Today && H >= 21)
	{
		OutArea = TEXT("saro");
		OutRoad = Spot(TEXT("gate"), OutArea, FVector2D(GateX, GateZ), FVector2D(GateX, GateZ - 30), 0);
		return true;
	}
	if (!VanHere()) return false;
	OutArea = TEXT("road"); OutRoad = FVector2D(VanX, VanZ);
	return true;
}

void FS47Visitors::ShowVan()
{
	using namespace S47VisitorsLocal;
	FString Area; FVector2D At; bool bOpen = false;
	const bool bAny = VanWhere(Area, At, bOpen);
	const FString Here = M->Web ? M->Web->Area() : FString();
	for (int32 S = 0; S < 2; ++S)
	{
		FVanSet& V = Vans[S];
		const bool bMine = bAny && SetOf(Area) == S;
		const FVector Base = bMine ? S47Three::ToUE(S47OpenLand::FromRoad(Area, At.X, At.Y) + FVector(0, S47FarmWorld::FloorRoad(At.X, At.Y), 0)) : FVector::ZeroVector;
		for (int32 I = 0; I < V.Parts.Num(); ++I) if (auto* C = V.Parts[I].Get()) { C->SetHiddenInGame(!bMine); if (bMine) C->SetWorldLocation(Base + V.Off[I] * 100.0); }
		for (int32 I = 0; I < V.Open.Num(); ++I) if (auto* C = V.Open[I].Get()) { C->SetHiddenInGame(!(bMine && bOpen)); if (bMine) C->SetWorldLocation(Base + V.OpenOff[I] * 100.0); }
		for (int32 I = 0; I < V.Words.Num(); ++I) if (auto* C = V.Words[I].Get()) { C->SetHiddenInGame(!bMine); if (bMine) C->SetWorldLocation(Base + V.WordsOff[I] * 100.0); }
		if (V.Parts.Num() && V.Parts[0].Get()) V.Parts[0]->SetCollisionEnabled(bMine ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		// The dome light: on in an empty van with its door open, and in the one at the gate, where they are getting out with their things.
		const bool bLitHere = S == 1 ? Here == TEXT("saro") : (Here == TEXT("road") || Here == TEXT("diner"));
		if (auto* L = V.Dome.Get()) { L->SetVisibility(bMine && bLitHere && (bOpen || S == 1) && (Hour() >= 19 || Hour() < 7)); if (bMine) L->SetWorldLocation(Base + FVector(0, 0, 190)); }
		SetBlock(FString::Printf(TEXT("visit:van%d"), S), SetArea(S), bMine, At, 1.05, 2.75);
	}
	if (!bAny || !M->Freeform) return;
	FS47FreeformSession& F = *M->Freeform;
	FVector2D Me; const bool bMe = F.EventPlaces && F.EventPlaces->PlayerRoad(Me);
	const double Near = bMe ? FVector2D::Distance(Me, At) : 1e9;
	// They pull in at the gate as the player is about: doors, and their engine running on a while.
	if (!bOpen && Area == TEXT("saro") && VanArrivedDay != F.Day() && Near < 300 && M->Audio)
	{
		VanArrivedDay = F.Day();
		const FVector Three = S47OpenLand::FromRoad(Area, At.X, At.Y) + FVector(0, S47FarmWorld::FloorRoad(At.X, At.Y) + 1, 0);
		M->Audio->Play(TEXT("engineIdle"), .5f, Three);
		FS47Sound O; O.At = Three + FVector(1.2, 0, 0); O.Gain = .45f; O.When = 2.2f; M->Audio->Sfx(TEXT("thudSoft"), O); O.When = 2.9f; M->Audio->Sfx(TEXT("thudSoft"), O);
		UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors: the Sky Watch at SARO's gate, road %.0f %.0f"), At.X, At.Y);
	}
	// The empty van, found: kept as a field record once the player has come up to it.
	const FString Empty = FString::Printf(TEXT("skywatch_empty_%d"), OutNight);
	if (bOpen && Near < 15 && !F.HasFinding(Empty))
	{
		F.RegisterFinding(Empty, FString::Printf(TEXT("%s: the Socorro Sky Watch's van, standing empty, the side door open and the dome light on. Binoculars on the seat. A flashlight on the ground, switched off. Nobody."), *PlaceTitle(OutPlace)));
		if (M->UI) M->UI->Toast(TEXT("Their van. The side door open, nobody in it. Kept as a field record."), 6);
	}
}

// ---------- step ----------

void FS47Visitors::StepBaca()
{
	// His door open while he is gone (swung on its west edge), shut while he is home; the porch light on in the evening until ten.
	if (auto* C = Porch.Get())
	{
		C->SetHiddenInGame(false);
		const double Open = BacaGone() ? 70 : 0;
		C->SetWorldLocationAndRotation(PorchHinge + FVector(FMath::Cos(FMath::DegreesToRadians(Open)) * 50, FMath::Sin(FMath::DegreesToRadians(Open)) * 50, 0), FRotator(0, Open, 0));
	}
	if (auto* L = BacaLamp.Get())
	{
		const double H = Hour();
		const FString Here = M->Web ? M->Web->Area() : FString();
		L->SetVisibility(!BacaGone() && H >= 19 && H < 22 && (Here == TEXT("road") || Here == TEXT("diner")));
	}
}

void FS47Visitors::Step(double Dt)
{
	if (!M || !M->Freeform || !bLive) return;
	if (!bBuilt && !Build()) return;
	if (Scene.Get() != M->Web.Get()) { bBuilt = false; for (FVanSet& S : Vans) S = FVanSet(); HeliParts.Reset(); CarParts.Reset(); RakeParts.Reset(); Blocks.Reset(); return; }
	StepBaca();
	ShowVan();
	StepMilitary(Dt);
	PlaceRaked();
	StepIn -= Dt;
	if (StepIn > 0) return;
	StepIn = 5;
	FS47FreeformSession& F = *M->Freeform;
	const int32 Today = F.Day();
	const double H = Hour();
	if (BacaBack > 0 && (Today > BacaBack || (Today == BacaBack && H >= 8))) { BacaBack = 0; UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors: Baca home again")); M->RequestAutosave(); }
	if (!F.EventPlaces || !F.RulesLive()) return;
	// After a night: Baca taken by what came to his place; the Sky Watch's night out on their own; the military's morning. Every night
	// over since the last one settled (a rest straight to 08:00 has slept through the hours this used to wait for).
	const int32 Done = FS47FreeformSession::NightsOver(F.Clock());
	const int32 Checked = FMath::Min(CheckedNight, BacaCheckedNight);
	if (Done <= Checked) return;
	for (int32 N = FMath::Max(Checked + 1, Done - 2); N <= Done; ++N) Settle(N);
	CheckedNight = FMath::Max(CheckedNight, Done);
	BacaCheckedNight = FMath::Max(BacaCheckedNight, Done);
}

void FS47Visitors::Settle(int32 Night)
{
	FS47FreeformSession& F = *M->Freeform;
	if (Night > BacaCheckedNight && BacaBack == 0 && F.EventPlaces->LastNight(TEXT("baca")) == Night && FS47NightRhythm::Roll(F.Seed(), Night, 0x42414341u) < .34f)
	{
		UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors: night %d settled: Baca taken"), Night);
		VanishBaca();
	}
	if (!OutPlace.IsEmpty() && OutNight == Night && !Flags.Contains(TEXT("van_back")) && !Flags.Contains(TEXT("van_empty")))
	{
		// Most nights they come back with a story. Not when something came that night where they were, or near it: then the van stands
		// empty out there, the door open.
		bool bSomething = F.EventPlaces->LastNight(OutPlace) == Night;
		if (const FS47EventPlace* Out = F.EventPlaces->Find(OutPlace))
			for (const FS47EventPlace& P : F.EventPlaces->All())
				bSomething |= F.EventPlaces->LastNight(P.Id) == Night && Big(F.EventPlaces->LastKind(P.Id)) && FVector2D::Distance(FVector2D(P.X, P.Z), FVector2D(Out->X, Out->Z)) < 600;
		const bool bEmpty = FS47NightRhythm::Roll(F.Seed(), Night, 0x4f555421u) < (bSomething ? .75f : .2f);
		Note(bEmpty ? TEXT("van_empty") : TEXT("van_back"));
		VanSpotKey.Empty();
		if (F.Places && FS47Places::Def(OutPlace)) F.Places->Hear(OutPlace);
		UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors: night %d settled: the Sky Watch %s (%s)"), Night, bEmpty ? TEXT("not heard from, the van empty") : TEXT("back"), bSomething ? TEXT("something came near them") : TEXT("a quiet night out there"));
	}
	if (Night <= CheckedNight) return;
	const bool bKnown = Flags.Contains(TEXT("let_in")) || Flags.Contains(TEXT("tie_seen"));
	for (const FS47EventPlace& P : F.EventPlaces->All())
	{
		FVisit V;
		if (F.EventPlaces->LastNight(P.Id) != Night || !Big(F.EventPlaces->LastKind(P.Id)) || !MilitaryAfter(F.Seed(), Night, P.Id, bKnown, F.Clock(), V)) continue;
		// The player already standing at it before they come: nothing to take before it is seen.
		V.bLooked = !V.bGone && F.EventPlaces->DistanceFromPlayer(P) >= 0 && F.EventPlaces->DistanceFromPlayer(P) < 60 && F.Clock() < V.Start;
		Visits.RemoveAll([&V](const FVisit& X) { return X.Place == V.Place && X.Night == V.Night; });
		Visits.Add(V);
		while (Visits.Num() > 32) Visits.RemoveAt(0);
		UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors: the military at %s after night %d (%s)%s"), *P.Id, Night, *F.EventPlaces->LastKind(P.Id), V.bGone ? TEXT(", come and gone while the player slept") : TEXT(""));
		M->RequestAutosave();
		break;
	}
}

void FS47Visitors::PlaceCars(FVisit& V, bool bShow, double Dt)
{
	using namespace S47VisitorsLocal;
	(void)Dt;
	const FS47EventPlace* P = M->Freeform->EventPlaces ? M->Freeform->EventPlaces->Find(V.Place) : nullptr;
	const FString Area = P ? FS47EventPlaces::AreaOf(*P) : FString();
	if (P && !V.bSpot)
	{
		// At its way in, clear of every building and off the highway; where there is no such room, a little off its front all the same.
		if (!CarsSpot(V.Place, V.CarA, V.CarB)) { V.CarA = FVector2D(P->EntryX + 9, P->EntryZ + 6); V.CarB = V.CarA + FVector2D(3.4, 0); }
		V.bSpot = true;
	}
	// Driving off: along their length, away from the place, gathering speed.
	const double Dir = P && V.CarA.Y < P->Z ? -1 : 1, Gone = V.Leave >= 0 ? .5 * 3. * V.Leave * V.Leave : 0;
	for (int32 I = 0; I < CarParts.Num(); ++I)
	{
		UStaticMeshComponent* C = CarParts[I].Get();
		if (!C) continue;
		const int32 Set = I / 2;
		const bool bMine = bShow && P && SetOf(Area) == Set && Area != TEXT("station01");
		C->SetHiddenInGame(!bMine);
		C->SetCollisionEnabled(bMine ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		const FVector2D At = (I % 2 ? V.CarB : V.CarA) + FVector2D(0, Dir * Gone);
		SetBlock(FString::Printf(TEXT("visit:car%d"), I), SetArea(Set), bMine, At, .95, 2.45);
		if (!bMine) continue;
		if (!bCarsLogged) { bCarsLogged = true; UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors: black sedans at %s, road %.1f %.1f and %.1f %.1f, area %s"), *V.Place, V.CarA.X, V.CarA.Y, V.CarB.X, V.CarB.Y, *Area); }
		const bool bBox = C->GetStaticMesh() && C->GetStaticMesh()->GetName() == TEXT("Cube");
		const FVector Three = S47OpenLand::FromRoad(Area, At.X, At.Y);
		C->SetWorldLocationAndRotation(S47Three::ToUE(FVector(Three.X, S47FarmWorld::FloorRoad(At.X, At.Y) + (bBox ? .7 : 0), Three.Z)), FRotator(0, 90 + (I % 2) * 3, 0));
		if (bBox) C->SetWorldScale3D(FVector(4.8, 1.8, 1.4));
	}
}

void FS47Visitors::PlaceHeli(const FVisit& V, double Age, double Dt)
{
	const FS47EventPlace* P = M->Freeform->EventPlaces ? M->Freeform->EventPlaces->Find(V.Place) : nullptr;
	const FString Area = P ? FS47EventPlaces::AreaOf(*P) : FString();
	const bool bFly = P && Age >= 0 && Age <= 90 && Area != TEXT("station01");
	// In from the north at about fifty metres a second, lower as it comes; twenty seconds over the place, turning slowly round it at
	// thirty metres (they are looking); then away south.
	const bool bHover = Age >= 35 && Age < 55;
	const double K = Age < 35 ? .5 * Age / 35. : bHover ? .5 : FMath::Clamp(.5 + .5 * (Age - 55) / 35., .5, 1.);
	const double Round = bHover ? FMath::Sin((Age - 35) / 20. * UE_DOUBLE_PI) * 14 : 0;
	const double X = P ? P->X - 200 + 400 * K + Round * FMath::Cos((Age - 35) * .35) : 0, Z = P ? P->Z - 1800 + 3600 * K + Round * FMath::Sin((Age - 35) * .35) : 0;
	const double Over = P ? FMath::Abs(Z - P->Z) : 1e9, Up = 30 + 25 * FMath::Clamp(Over / 600., 0., 1.);
	const FVector At = bFly ? S47OpenLand::FromRoad(Area, X, Z) : FVector::ZeroVector;
	const double Y = bFly ? S47FarmWorld::FloorRoad(X, Z) + Up : 0;
	for (int32 I = 0; I < HeliParts.Num(); ++I)
	{
		UStaticMeshComponent* C = HeliParts[I].Get();
		if (!C) continue;
		using namespace S47VisitorsLocal;
		const bool bSaroSet = I >= HeliPer, bMine = bFly && ((Area == TEXT("saro")) == bSaroSet);
		C->SetHiddenInGame(!bMine);
		if (!bMine) continue;
		const FVector Base = S47Three::ToUE(FVector(At.X, Y, At.Z));
		const FHeliBit& B = HeliBits[I % HeliPer];
		const FRotator Turn = B.Spin == 1 ? FRotator(0, Age * 1400.0, 0) : B.Spin == 2 ? FRotator(0, Age * 1400.0 + 90, 0) : B.Spin == 3 ? FRotator(0, 0, Age * 2600.0) : FRotator::ZeroRotator;
		C->SetWorldLocationAndRotation(Base + B.Off, Turn); C->SetWorldScale3D(B.Size);
	}
	// The thump of the blades, louder as it comes over (the engine's own beeper until there is a recording): in the world's seconds,
	// so that it is the same beat at thirty frames a second as at a hundred and forty.
	if (bFly && M->Audio && M->Web && M->Web->Area() != TEXT("") && M->Player)
	{
		FVector2D Me; const bool bMe = M->Freeform->EventPlaces->PlayerRoad(Me);
		const double D = bMe ? FVector2D::Distance(Me, FVector2D(X, Z)) : 1e9;
		ThumpIn -= Dt;
		if (D < 2500 && ThumpIn <= 0) { ThumpIn = .19; M->Audio->Beep(52.f, .06f, float(.14 * FMath::Clamp(1 - D / 2500., 0., 1.)), 0.f, TEXT("sine")); }
	}
}

void FS47Visitors::StepMilitary(double Dt)
{
	FS47FreeformSession& F = *M->Freeform;
	FVisit* Live = nullptr;
	for (FVisit& V : Visits)
	{
		if (V.bGone) continue;
		const double Since = F.Clock() - V.Start;
		const FS47EventPlace* P = F.EventPlaces ? F.EventPlaces->Find(V.Place) : nullptr;
		const double Away = P ? F.EventPlaces->DistanceFromPlayer(*P) : -1;
		// Before they come: the player there first sees the ground as it was.
		if (Since < 0) { if (Away >= 0 && Away < 40) V.bLooked = true; continue; }
		// Gone by late morning, seen or not; the ground they leave behind says so.
		if (Since > 5 * 3600 && V.Leave < 0) { V.bGone = true; Note(TEXT("military_seen")); continue; }
		// The player comes up while they stand there: doors, both engines, and they pull out, unhurried, and are gone.
		if (V.Leave < 0 && HeliAge > 20 && !(V.bHold && HeliAge < 56) && ((Away >= 0 && Away < 60) || (V.bSpot && [&] { FVector2D Me; return F.EventPlaces && F.EventPlaces->PlayerRoad(Me) && FVector2D::Distance(Me, V.CarA) < 45; }())))
		{
			V.Leave = 0;
			const FString Area = P ? FS47EventPlaces::AreaOf(*P) : FString(TEXT("road"));
			const FVector Three = S47OpenLand::FromRoad(Area, V.CarA.X, V.CarA.Y) + FVector(0, S47FarmWorld::FloorRoad(V.CarA.X, V.CarA.Y) + 1, 0);
			if (M->Audio)
			{
				FS47Sound O; O.At = Three; O.Gain = .5f; M->Audio->Sfx(TEXT("thudSoft"), O); O.When = .5f; M->Audio->Sfx(TEXT("thudSoft"), O);
				M->Audio->Play(TEXT("engineIdle"), .6f, Three, .85f);
			}
			UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors: the sedans at %s drive off as the player comes up (%.0f m)"), *V.Place, Away);
		}
		if (V.Leave >= 0)
		{
			V.Leave += Dt;
			if (V.Leave > 7) { V.Leave = -1; V.bGone = true; Note(TEXT("military_seen")); continue; }
		}
		Live = &V;
	}
	// The ground they went over, read when the player gets to it on foot, while it is fresh (two days), once they have gone.
	FVector2D Me; const bool bMe = F.EventPlaces && F.EventPlaces->PlayerRoad(Me);
	const bool bFree = bMe && !(M->World && M->World->bDriving) && !M->ModalOpen() && !(F.Director && F.Director->Busy());
	for (FVisit& V : Visits)
	{
		if (!bFree || V.bLooked || !V.bGone || F.Clock() < V.Start || F.Clock() > V.Start + S47VisitorsLocal::FreshFor) continue;
		const FString Id = FString::Printf(TEXT("military_%s_%d"), *V.Place, V.Night);
		if (F.HasFinding(Id)) continue;
		const FS47EventPlace* P = F.EventPlaces ? F.EventPlaces->Find(V.Place) : nullptr;
		if (!P) continue;
		const FVector2D Raked = V.bRake ? FVector2D(V.RakeX, V.RakeZ) : FVector2D(P->X, P->Z);
		if (FVector2D::Distance(Me, Raked) > S47VisitorsLocal::RakeRadius + 6) continue;
		const FString Text = FString::Printf(TEXT("%s, the morning after: tire tracks over everything, the ground raked smooth in a wide circle, and a cigarette end of a brand nobody sells in this county. Whatever was there has been taken."), *PlaceTitle(V.Place));
		F.RegisterFinding(Id, Text);
		if (M->UI) M->UI->Open(S47Kit::Shell(M->UI, TEXT("SOMEBODY GOT HERE FIRST"), S47Kit::Col({ S47Kit::Text(Text), S47Kit::Small(TEXT("Kept as a field record.")) }), 760));
		UE_LOG(LogS47Visitors, Display, TEXT("S47: visitors: the raked ground at %s read, night %d"), *V.Place, V.Night);
		break;
	}
	if (Live)
	{
		HeliAge = HeliAge < 0 ? 0 : HeliAge + Dt;
		PlaceHeli(*Live, HeliAge, Dt);
		PlaceCars(*Live, HeliAge > 20, Dt);
		// Their engines running while they stand there: heard as the player comes up.
		const bool bIdle = HeliAge > 20 && Live->Leave < 0 && Live->bSpot && bMe && FVector2D::Distance(Me, Live->CarA) < 120 && M->Audio;
		if (bIdle && !bCarsIdling)
		{
			const FS47EventPlace* P = F.EventPlaces ? F.EventPlaces->Find(Live->Place) : nullptr;
			const FString Area = P ? FS47EventPlaces::AreaOf(*P) : FString(TEXT("road"));
			FS47Sound O; O.At = S47OpenLand::FromRoad(Area, Live->CarA.X, Live->CarA.Y) + FVector(0, S47FarmWorld::FloorRoad(Live->CarA.X, Live->CarA.Y) + .6, 0); O.Gain = .3f; O.Rate = .8f; O.bLoop = true;
			M->Audio->LoopSfx(TEXT("visitCars"), TEXT("engineIdle"), O);
		}
		if (!bIdle && bCarsIdling && M->Audio) M->Audio->Stop(TEXT("visitCars"), .6f);
		bCarsIdling = bIdle;
	}
	else
	{
		HeliAge = -1;
		if (bCarsIdling && M->Audio) M->Audio->Stop(TEXT("visitCars"), .6f);
		bCarsIdling = false;
		for (const auto& P : HeliParts) if (auto* C = P.Get()) C->SetHiddenInGame(true);
		for (int32 I = 0; I < CarParts.Num(); ++I)
		{
			if (auto* C = CarParts[I].Get()) { C->SetHiddenInGame(true); C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
			if (const FBlock* B = Blocks.Find(FString::Printf(TEXT("visit:car%d"), I))) { const FString Area = B->Area; SetBlock(FString::Printf(TEXT("visit:car%d"), I), Area, false, FVector2D::ZeroVector, 0, 0); }
		}
	}
}

void FS47Visitors::PlaceRaked()
{
	using namespace S47VisitorsLocal;
	FS47FreeformSession& F = *M->Freeform;
	const double Now = F.Clock();
	// The freshest ground they went over, if it is fresh and near enough to be seen.
	FVisit* Best = nullptr;
	for (FVisit& V : Visits) if (!V.bLooked && Now >= V.Start && Now < V.Start + FreshFor && (!Best || V.Start > Best->Start)) Best = &V;
	const FS47EventPlace* P = Best && F.EventPlaces ? F.EventPlaces->Find(Best->Place) : nullptr;
	const FString Area = P ? FS47EventPlaces::AreaOf(*P) : FString();
	FVector2D Me; const bool bMe = F.EventPlaces && F.EventPlaces->PlayerRoad(Me);
	const FVector2D Middle = Best && Best->bRake ? FVector2D(Best->RakeX, Best->RakeZ) : P ? FVector2D(P->X, P->Z) : FVector2D::ZeroVector;
	const bool bShow = P && bMe && (Area == TEXT("road") || Area == TEXT("saro")) && FVector2D::Distance(Me, Middle) < 700;
	if (bShow && !Best->bSpot)
	{
		if (!CarsSpot(Best->Place, Best->CarA, Best->CarB)) { Best->CarA = FVector2D(P->EntryX + 9, P->EntryZ + 6); Best->CarB = Best->CarA + FVector2D(3.4, 0); }
		Best->bSpot = true;
	}
	const int32 Mine = bShow ? SetOf(Area) : -1;
	for (int32 I = 0; I < RakeParts.Num(); ++I)
	{
		UStaticMeshComponent* C = RakeParts[I].Get();
		if (!C) continue;
		const int32 Set = I / RakePer, Kind = I % RakePer;
		C->SetHiddenInGame(Set != Mine);
		if (Set != Mine) continue;
		const FVector2D Along = (Best->CarA - Middle).GetSafeNormal(), Side(-Along.Y, Along.X);
		const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(Along.Y, Along.X));
		const auto Lay = [&](const FVector2D& Where, double Lift, const FVector& Size, double Turn)
		{
			const FVector A3 = S47OpenLand::FromRoad(Area, Where.X, Where.Y);
			C->SetWorldLocationAndRotation(S47Three::ToUE(FVector(A3.X, S47FarmWorld::FloorRoad(Where.X, Where.Y) + Lift, A3.Z)), FRotator(0, Turn, 0));
			C->SetWorldScale3D(Size);
		};
		if (Kind < RakeDiscN)
		{
			const FVector& D = RakeDiscs[Kind];
			Lay(Middle + FVector2D(D.X, D.Y), .03 + Kind * .002, FVector(D.Z * 2, D.Z * 2, .004), 0);
			continue;
		}
		if (Kind < RakeDiscN + RakeStripeN)
		{
			// The rake's strokes, side by side across the circle, short near its edge.
			const double Off = (Kind - RakeDiscN - (RakeStripeN - 1) * .5) * 1.15, Half = FMath::Sqrt(FMath::Max(.5, FMath::Square(4.7) - Off * Off));
			Lay(Middle + Along * Off, .045, FVector(.07, Half * 2, .003), Yaw);
			continue;
		}
		// Two ruts, in four lengths each, from behind the circle out toward where the cars stood: on the raked earth inside it, on the
		// land outside.
		const int32 Rut = (Kind - RakeDiscN - RakeStripeN) / 4, Seg = (Kind - RakeDiscN - RakeStripeN) % 4;
		const FVector2D At = Middle + Along * (-6 + 4 * Seg + 2) + Side * (Rut ? .8 : -.8);
		Lay(At, FVector2D::Distance(At, Middle) < 5 ? .05 : .015, FVector(4.05, .2, .004), Yaw);
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
	// Something lately: lights he saw, or says he did, and as often as not puts somewhere else or on another night. Only lights (what
	// one sees from a porch miles off), and never the motel's nights, which are its own.
	if (F.EventPlaces)
	{
		const FS47EventPlace* Best = nullptr; int32 BestNight = 0;
		for (const FS47EventPlace& P : F.EventPlaces->All())
		{
			const int32 L = F.EventPlaces->LastNight(P.Id);
			if (L > BestNight && L >= Night - 3 && P.Id != TEXT("baca") && P.Id != TEXT("motel") && FS47Townsfolk::Talk(F.EventPlaces->LastKind(P.Id)) == TEXT("lights")) { Best = &P; BestNight = L; }
		}
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
				for (const FS47EventPlace& P : F.EventPlaces->All()) { const double E = FVector2D::Distance(FVector2D(P.X, P.Z), FVector2D(Best->X, Best->Z)); if (P.Id != Best->Id && P.Id != TEXT("baca") && P.Id != TEXT("motel") && E < D) { D = E; Near = &P; } }
				if (Near && Roll < .3f) Where = Near->Title; else When = FMath::Max(1, BestNight - 1);
			}
			T.Place = Best->Id;
			T.Text = FString::Printf(TEXT("%s night there were lights down over %s. I watched them from this porch with my coffee. Then they went up, slow, like they had all night."), *WeekdayOf(When, F.OldCalendar()), *Where);
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
	// After ten he is in bed, and an old man alone under the rim does not open to a knock at night.
	const double H = Hour();
	if (H >= 22 || H < 6)
	{
		Rows.Add(S47Kit::Paper(TEXT("The house is dark. You knock, and after a long while a light goes on behind the curtain, and then off again. He doesn't come to the door.")));
		Rows.Add(S47Kit::Button(TEXT("Leave him be"), [this] { if (M && M->UI) M->UI->Close(); }));
		M->UI->Open(S47Kit::Shell(M->UI, TEXT("THE BACA PLACE"), S47Kit::Col(Rows), 760));
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
	// On the Mesa's lot, by day, on its days: unless they are out there standing empty, or have gone on to SARO's gate tonight.
	if (!M || !M->Freeform || Flags.Contains(TEXT("van_empty"))) return false;
	const int32 Today = M->Freeform->Day();
	const double H = Hour();
	return (VanDay(M->Freeform->Seed(), Today) || ForcedVanDay == Today) && H >= 10 && H < 20;
}

void FS47Visitors::DevVan() { if (M && M->Freeform) { ForcedVanDay = M->Freeform->Day(); Flags.Remove(TEXT("van_empty")); VanChoiceDay = 0; VanSpotKey.Empty(); } }
void FS47Visitors::DevVanGate() { if (M && M->Freeform) { Flags.Remove(TEXT("van_empty")); VanChoice = TEXT("in"); VanChoiceDay = M->Freeform->Day(); Flags.Add(TEXT("let_in")); VanArrivedDay = 0; VanSpotKey.Empty(); } }
bool FS47Visitors::DevVanEmpty(const FString& Place)
{
	if (!M || !M->Freeform || !M->Freeform->EventPlaces || !M->Freeform->EventPlaces->Find(Place)) return false;
	OutPlace = Place; OutNight = FS47FreeformSession::NightOf(M->Freeform->Clock()) - 1;
	Flags.Remove(TEXT("van_back")); Flags.Add(TEXT("van_empty")); VanSpotKey.Empty();
	return true;
}

void FS47Visitors::OpenVan()
{
	if (!M || !M->UI || !M->Freeform) return;
	FS47FreeformSession& F = *M->Freeform;
	TArray<TSharedRef<SWidget>> Rows;
	Rows.Add(S47Kit::Paper(TEXT("THE SOCORRO SKY WATCH, painted by hand on the side of a cream van. Three of them: a retired schoolteacher with binoculars on a strap, her nephew with a tape recorder, and a quiet man in the back seat in a tie, who doesn't get out.")));
	if (VanChoiceDay == F.Day())
	{
		Rows.Add(S47Kit::Text(VanChoice.StartsWith(TEXT("in")) ? TEXT("\"Tonight, then! We'll be at your gate at nine sharp.\" The nephew is already checking his batteries.")
			: VanChoice == TEXT("away") ? TEXT("\"Then we'll find our own.\" The schoolteacher unfolds a map with a circle drawn on it.") : TEXT("The back seat is empty now. The schoolteacher says he got a ride.")));
	}
	else
	{
		Rows.Add(S47Kit::Text(TEXT("THE SCHOOLTEACHER: We're scientists, more or less. We only want to see the instruments. One night. We'll bring donuts. We'll make a donation.")));
		Rows.Add(S47Kit::Button(TEXT("Let them in tonight / a $15 donation"), [this]
		{
			FS47FreeformSession& G = *M->Freeform; VanChoiceDay = G.Day(); VanChoice = TEXT("in"); G.Earn(15); Note(TEXT("let_in")); VanArrivedDay = 0; VanSpotKey.Empty();
			// Their newsletter, mimeographed: a rumor in it. Tonight at nine they are at SARO's gate.
			const FString New = G.Places ? G.Places->NextUnknown(G.Day() * 29) : FString();
			if (!New.IsEmpty()) G.Places->Hear(New);
			if (M->UI) M->UI->Toast(New.IsEmpty() ? TEXT("$15 and a newsletter. Tonight at nine, at SARO's gate.") : FString::Printf(TEXT("$15 and their newsletter. It says to watch %s. Tonight at nine, at SARO's gate."), *PlaceTitle(New)), 8);
			OpenVan();
		}));
		Rows.Add(S47Kit::Button(TEXT("Turn them down"), [this]
		{
			FS47FreeformSession& G = *M->Freeform; VanChoiceDay = G.Day(); VanChoice = TEXT("away"); Note(TEXT("turned_away"));
			// They go out on their own tonight, to somewhere far (somewhere a van can stand: not the motel, not the other station).
			FS47PlaceNeed Need; Need.Kind = TEXT("skywatch"); Need.All = ES47PlaceTrait::Far; Need.FarShare = 1.f; Need.Skip = { TEXT("station01"), TEXT("motel"), TEXT("baca") };
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

void FS47Visitors::OpenVanGate()
{
	if (!M || !M->UI || !M->Freeform) return;
	FS47FreeformSession& F = *M->Freeform;
	TArray<TSharedRef<SWidget>> Rows;
	Rows.Add(S47Kit::Paper(TEXT("Nine sharp, as she said. The schoolteacher has a thermos and a box of donuts; her nephew is already recording the hum off the fence wire. The quiet man in the tie asks to see the logbooks, and you let him. He reads them standing up under the dome light, page by page, and hands them back without a word.")));
	const FString Id = FString::Printf(TEXT("skywatch_visit_%d"), F.Day());
	if (!F.HasFinding(Id)) { F.RegisterFinding(Id, TEXT("The Socorro Sky Watch at SARO's gate in the evening: three of them. The third, in a tie, read the station's logbooks through and said nothing.")); M->RequestAutosave(); }
	Rows.Add(S47Kit::Small(TEXT("Kept as a field record.")));
	Rows.Add(S47Kit::Button(TEXT("Thank them and see them off"), [this]
	{
		VanChoice = TEXT("in_done");
		if (M && M->UI) { M->UI->Close(); M->UI->Toast(TEXT("The van backs out onto the highway. Its taillights go a long way before they are gone."), 6); }
		if (M && M->Audio) { FString Area; FVector2D At; bool bOpen = false; if (VanWhere(Area, At, bOpen)) M->Audio->Play(TEXT("engineIdle"), .5f, S47OpenLand::FromRoad(Area, At.X, At.Y)); }
	}, true));
	Rows.Add(S47Kit::Button(TEXT("Let them stay a while"), [this] { if (M && M->UI) M->UI->Close(); }));
	M->UI->Open(S47Kit::Shell(M->UI, TEXT("SARO GATE / THE SKY WATCH"), S47Kit::Col(Rows), 820));
}

void FS47Visitors::OpenVanEmpty()
{
	if (!M || !M->UI || !M->Freeform) return;
	TArray<TSharedRef<SWidget>> Rows;
	Rows.Add(S47Kit::Paper(TEXT("The Sky Watch's van. The side door slid open, the dome light on and the battery nearly gone with it. Binoculars on the front seat, the strap cut. A thermos on the floor, full. The tape recorder is not here. A flashlight lies on the ground a few steps off, switched off. No tracks lead away from the van.")));
	const FString Id = FString::Printf(TEXT("skywatch_empty_%d"), OutNight);
	if (!M->Freeform->HasFinding(Id)) M->Freeform->RegisterFinding(Id, FString::Printf(TEXT("%s: the Socorro Sky Watch's van, standing empty, the side door open and the dome light on. Binoculars on the seat. A flashlight on the ground, switched off. Nobody."), *PlaceTitle(OutPlace)));
	Rows.Add(S47Kit::Small(TEXT("Kept as a field record.")));
	Rows.Add(S47Kit::Button(TEXT("Leave it as it is"), [this] { if (M && M->UI) M->UI->Close(); }));
	M->UI->Open(S47Kit::Shell(M->UI, TEXT("THE SKY WATCH VAN"), S47Kit::Col(Rows), 800));
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

bool FS47Visitors::DevMilitary(const FString& Place, bool bHold)
{
	if (!M || !M->Freeform || !M->Freeform->EventPlaces || !M->Freeform->EventPlaces->Find(Place)) return false;
	FVisit V; V.Place = Place; V.Night = FS47FreeformSession::NightOf(M->Freeform->Clock()); V.Start = M->Freeform->Clock(); V.bHold = bHold;
	Visits.RemoveAll([&V](const FVisit& X) { return X.Place == V.Place && X.Night == V.Night; });
	Visits.Add(V);
	return true;
}

bool FS47Visitors::DevRaked(const FString& Place)
{
	if (!DevMilitary(Place)) return false;
	// As they left it, hours ago: nobody here now, the ground gone over.
	FVisit& V = Visits.Last(); V.Start = M->Freeform->Clock() - 5.5 * 3600; V.bGone = true;
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
	// Settled in arrears (critics, round 3): a visit at the farm at two in the morning of night N, the player rests to 08:00. That
	// morning night N is over and settled: the cars stand from six to eleven, so they are still there at eight. Asleep for two days,
	// they have come and gone.
	{
		int32 N = 2;
		while (N < 400 && !Comes(47, N, TEXT("farm"), false)) ++N;
		const double At8 = double(N) * 86400. + 8 * 3600.;
		FVisit Morning, Later, None;
		const bool bCome = MilitaryAfter(47, N, TEXT("farm"), false, At8, Morning) && MilitaryAfter(47, N, TEXT("farm"), false, At8 + 2 * 86400., Later);
		int32 Quiet = N + 1; while (Quiet < 400 && Comes(47, Quiet, TEXT("farm"), false)) ++Quiet;
		Check(N < 400 && bCome && FS47FreeformSession::NightsOver(At8) == N && Morning.Start == double(N) * 86400. + 6 * 3600. && !Morning.bGone && At8 < Morning.Start + 5 * 3600. && Later.bGone && !MilitaryAfter(47, Quiet, TEXT("farm"), false, At8, None),
			FString::Printf(TEXT("night %d's visit at the farm slept through to 08:00: settled that morning, the cars there until eleven; two days on, come and gone"), N));
	}
	// The cars at a place: at its way in, never in one of the boxes (S47TownSites) or on the highway, at every place on the catalogue.
	{
		FS47EventPlaces Catalogue(nullptr);
		const TArray<FBox2D> Solid = S47TownSites::SolidFootprints();
		int32 Placed = 0, All = 0; FString Bad;
		for (const FS47EventPlace& P : Catalogue.All())
		{
			const FString Area = FS47EventPlaces::AreaOf(P);
			if (Area != TEXT("road") && Area != TEXT("saro")) continue;
			++All;
			FVector2D A, B;
			bool bOk = CarsAt(P, Solid, A, B);
			for (const FVector2D& C : { A, B })
			{
				const FBox2D Car(C - FVector2D(.9, 2.4), C + FVector2D(.9, 2.4));
				for (const FBox2D& S : Solid) bOk &= !S.Intersect(Car);
				bOk &= !OnHighway(C.X - .9, C.Y) && !OnHighway(C.X + .9, C.Y) && FVector2D::Distance(C, FVector2D(P.X, P.Z)) >= RakeRadius + 4 - 1e-6 && FVector2D::Distance(C, FVector2D(P.EntryX, P.EntryZ)) <= 45;
			}
			if (bOk) ++Placed; else if (Bad.IsEmpty()) Bad = P.Id;
		}
		Check(All > 20 && Placed == All, Bad.IsEmpty() ? FString::Printf(TEXT("the black sedans stand at the way in at all %d places, clear of every box and off the highway"), All) : TEXT("no room for the cars at ") + Bad);
	}
	{
		FS47Visitors A(nullptr); A.BacaTold.Add(TEXT("old:3")); A.Flags.Add(TEXT("let_in")); A.BacaDay = 5; A.BacaLast = TEXT("A story."); A.BacaBack = 9; A.VanChoiceDay = 6; A.VanChoice = TEXT("in");
		FVisit V; V.Place = TEXT("farm"); V.Night = 4; V.Start = 4 * 86400.; A.Visits.Add(V);
		A.RakeAt(TEXT("farm"), 4, 66.5, 238.2);
		FS47Visitors B(nullptr); B.Begin(S47J::Read(S47J::Write(A.Save())));
		Check(B.BacaGone() && B.Has(TEXT("let_in")) && B.Cleared(TEXT("farm"), 4) && !B.Cleared(TEXT("farm"), 5) && B.Visits.Num() == 1 && B.Visits[0].bRake && FMath::IsNearlyEqual(B.Visits[0].RakeX, 66.5) && S47J::Write(B.Save()) == S47J::Write(A.Save()),
			TEXT("Baca gone, the van's answer, the military's morning and where they raked survive a Continue"));
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

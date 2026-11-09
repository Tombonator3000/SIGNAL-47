#include "S47TownUse.h"
#include "S47Album.h"
#include "S47CalicheAccess.h"
#include "S47EventPlaces.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47RadioTips.h"
#include "S47RoomDecor.h"
#include "S47Three.h"
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

DEFINE_LOG_CATEGORY_STATIC(LogS47TownUse, Log, All);

namespace S47TownUseLocal
{
	const TCHAR* const Radio = TEXT("radio");
	const TCHAR* const Feed = TEXT("feed");
	const TCHAR* const Souvenir = TEXT("souvenir");
	// The interactables' ids, one a counter.
	FString UseId(const FString& Counter) { return TEXT("ff:town:") + Counter; }
	// What each counter sells, its price there and what it is for (FS47FreeformWorld's supplies, unpacked into SARO's stock).
	using FWare = FS47TownUse::FWare;
	const TArray<FWare>& RadioWares()
	{
		static const TArray<FWare> W = {
			{ TEXT("recorder_donor"), 18, TEXT("Earl's back room: a used field recorder, untested. Strip it at SARO's bench for a housing, a timer, a lead.") },
			{ TEXT("cell"), 3, TEXT("For the flashlight, and the bench.") } };
		return W;
	}
	const TArray<FWare>& FeedWares()
	{
		static const TArray<FWare> W = {
			{ TEXT("water"), 1, TEXT("For a radiator that runs hot out on the land.") },
			{ TEXT("spare"), 15, TEXT("Mounted and aired up. The jack is already behind your seat.") },
			{ TEXT("cell"), 3, TEXT("Flashlight and lantern cells. The bench at SARO uses them too.") },
			{ TEXT("foil"), 3, TEXT("Aluminum flashing off the roll: the bench shields a lead with it.") },
			{ TEXT("rope"), 12, TEXT("Hooked to a fence post, it pulls the truck out of sand: about 10 minutes instead of 25. Kept, not used up.") },
			{ TEXT("wire"), 4, TEXT("Run out from the trip camera, its line catches what crosses 30 m out instead of 18.") },
			{ TEXT("salt"), 3, TEXT("Set it out at a place on the land. In the morning, see what came to it.") },
			{ TEXT("feed"), 6, TEXT("Range cubes: the same, and more comes to them.") } };
		return W;
	}
	const TArray<FWare>& SouvenirWares()
	{
		static const TArray<FWare> W = {
			{ TEXT("film"), 9, TEXT("Thirty-six exposures. Load it in the photo lab at SARO.") } };
		return W;
	}
	const TArray<FWare>& WaresOf(const FString& Counter) { return Counter == Radio ? RadioWares() : Counter == Feed ? FeedWares() : SouvenirWares(); }
	// What a print looks like to the souvenir man.
	const TCHAR* const Dishes[] = { TEXT("antenna"), TEXT("dish"), TEXT("array"), TEXT("saro") };
	// Words nobody in Socorro County says in 1986 here: brand names, and the King's English (the radio's own check, S47RadioTips).
	const TCHAR* const NotSaid[] = { TEXT("chevy"), TEXT("chevrolet"), TEXT("ford"), TEXT("dodge"), TEXT("gmc"), TEXT("jeep"), TEXT("winnebago"), TEXT("kodak"), TEXT("polaroid"), TEXT("coke"), TEXT("pepsi"), TEXT("tyre"), TEXT("tyres"), TEXT("grey"), TEXT("colour"), TEXT("petrol"), TEXT("lorry"), TEXT("aluminium"), TEXT("programme") };
	// The souvenir man's patter on the tour, and where he always ends up.
	const TCHAR* const Patter[] = {
		TEXT("He drives you out in his own station wagon with the air on high. The fence where the rancher found the foil, he says (it is a fence). The arroyo where the bodies were laid out (it is an arroyo). The rock that was scorched (he has painted the scorch back on since)."),
		TEXT("Two hours in the wagon, with a tape of his own voice playing the parts he gets tired of saying. Mostly fence lines and a cattle guard. He stops for a long time at a place where nothing is and says nothing, and that part is not on the tape."),
		TEXT("He has a pointer from a schoolroom and uses it out the window. Most of it is the book's story, told well. At the turnaround he gets out and stands with his back to you, looking a long while at nothing in particular.") };
	bool CleanWords(const FString& Said, FString& Bad)
	{
		FString Word;
		const auto Test = [&Word, &Bad]() { for (const TCHAR* N : NotSaid) if (Word == N) { Bad = Word; return false; } return true; };
		for (const TCHAR Ch : Said.ToLower()) { if (FChar::IsAlpha(Ch)) Word.AppendChar(Ch); else if (!Word.IsEmpty()) { if (!Test()) return false; Word.Reset(); } }
		return Word.IsEmpty() || Test();
	}
}

// ---------- the rules (tested) ----------

bool FS47TownUse::OpenAt(const FString& Counter, double Hour)
{
	// The station's lobby is open while anyone is in the building; the feed store keeps ranch hours; the souvenir man, tourist ones.
	if (Counter == S47TownUseLocal::Radio) return Hour >= 7 && Hour < 23;
	if (Counter == S47TownUseLocal::Feed) return Hour >= 7 && Hour < 18;
	if (Counter == S47TownUseLocal::Souvenir) return Hour >= 10 && Hour < 17;
	return false;
}

FString FS47TownUse::HoursOf(const FString& Counter)
{
	return Counter == S47TownUseLocal::Radio ? TEXT("7 AM to 11 PM") : Counter == S47TownUseLocal::Feed ? TEXT("7 AM to 6 PM") : TEXT("10 AM to 5 PM");
}

const TArray<FS47TownUse::FTourSite>& FS47TownUse::SaucerMap()
{
	// Drawn by hand on butcher paper and printed in Socorro: some of it is out there (FS47Places), some of it is not anywhere.
	static const TArray<FTourSite> Map = {
		{ TEXT("Windmill that won't turn"), 170, 720, TEXT("windmill") },
		{ TEXT("The mine they shut"), -560, 1340, TEXT("mine") },
		{ TEXT("Rock pile on the rise"), -320, 430, TEXT("cairn") },
		{ TEXT("Line camp"), 440, 930, TEXT("shack") },
		{ TEXT("Saucer Rock"), -760, 2240, nullptr },
		{ TEXT("The Glowing Wash"), 620, 1980, nullptr },
		{ TEXT("Hermit's sighting"), -1250, 1720, nullptr },
		{ TEXT("Landing circle (fenced)"), 950, 2700, nullptr },
		{ TEXT("Crash site, 150 mi. east"), 2150, 1800, nullptr } };
	return Map;
}

bool FS47TownUse::Shy(const FVector2D& LureAt, int32 SetNight, const TArray<TPair<FVector2D, int32>>& UsedPlaces)
{
	for (const auto& U : UsedPlaces) if (U.Value >= SetNight && U.Value > 0 && FVector2D::Distance(U.Key, LureAt) <= ShyReach) return true;
	return false;
}

int32 FS47TownUse::PrintValue(const FS47AlbumShot& Shot, bool bRecord, FString& Why)
{
	if (Shot.bUnavailable) { Why = TEXT("he cannot make anything of it"); return 0; }
	if (!Shot.IsDeveloped()) { Why = TEXT("not developed yet"); return 0; }
	// A dollar for a print of anything, more as there is more in it: something named, the dishes (tourists buy the dishes), the
	// night, a darkroom enlargement, what the station was looking for, a sighting on the record, and most for the strange ones.
	int32 V = 1;
	const FString Caption = Shot.Text.ToLower();
	const bool bNamed = !Shot.Text.IsEmpty() && Shot.Text != FS47Album::NOBODY;
	if (bNamed) V += 1;
	for (const TCHAR* D : S47TownUseLocal::Dishes) if (Caption.Contains(D)) { V += 1; break; }
	const double Hour = FMath::Fmod(FMath::Max(0., Shot.Clock), 86400.) / 3600.;
	if (Hour >= 20 || Hour < 5) V += 1;
	if (!Shot.Enlarged.IsEmpty()) V += 1;
	if (Shot.Targets.Num() > 0) V += 4;
	if (bRecord) V += 3;
	if (Shot.bOdd) V += 10;
	Why = Shot.bOdd ? TEXT("he goes quiet over it, and then names a price") : V >= 5 ? TEXT("he likes it") : V >= 3 ? TEXT("a fair postcard") : TEXT("a print is a print");
	return FMath::Min(V, 20);
}

// ---------- the session ----------

double FS47TownUse::Hour() const { return M && M->Freeform ? FMath::Fmod(M->Freeform->Clock(), 86400.) / 3600. : 12.; }
int32 FS47TownUse::Night() const { return M && M->Freeform ? FS47FreeformSession::NightOf(M->Freeform->Clock()) : 1; }
bool FS47TownUse::InTown() const { return M && M->Web && (M->Web->Area() == TEXT("road") || M->Web->Area() == TEXT("diner")); }

void FS47TownUse::Note(const FString& What)
{
	if (What.IsEmpty() || Flags.Contains(What)) return;
	Flags.Add(What);
	UE_LOG(LogS47TownUse, Display, TEXT("S47: town: seen %s"), *What);
	if (M) M->RequestAutosave();
}

void FS47TownUse::Begin(const S47J::FObj& Saved)
{
	Stop();
	bLive = M != nullptr;
	Flags.Reset(); SoldPrints.Reset(); Lures.Reset(); Basket.Reset(); BasketAt.Empty();
	bMap = false; TourDay = SoldDay = SoldToday = Postcards = 0;
	if (Saved.IsValid())
	{
		for (const FString& F : S47J::Strs(Saved, TEXT("seen"))) if (!F.IsEmpty() && F.Len() < 24) Flags.Add(F);
		for (const FString& P : S47J::Strs(Saved, TEXT("sold"))) if (!P.IsEmpty() && P.Len() < 160 && SoldPrints.Num() < 400) SoldPrints.Add(P);
		bMap = S47J::Bool(Saved, TEXT("map"));
		TourDay = FMath::Clamp(S47J::Int(Saved, TEXT("tourDay")), 0, 1000000);
		SoldDay = FMath::Clamp(S47J::Int(Saved, TEXT("soldDay")), 0, 1000000);
		SoldToday = FMath::Clamp(S47J::Int(Saved, TEXT("soldToday")), 0, PrintsADay);
		Postcards = FMath::Clamp(S47J::Int(Saved, TEXT("postcards")), 0, 1000);
		for (const auto& V : S47J::Arr(Saved, TEXT("lures")))
		{
			if (!V || V->Type != EJson::Object || Lures.Num() >= 32) continue;
			const auto O = V->AsObject(); FLure L;
			L.Place = S47J::Str(O, TEXT("place")).Left(64); L.Kind = S47J::Str(O, TEXT("kind")).Left(8);
			L.Night = FMath::Clamp(S47J::Int(O, TEXT("night")), 0, 1000000); L.bRead = S47J::Bool(O, TEXT("read")); L.Said = S47J::Str(O, TEXT("said")).Left(400);
			if (!L.Place.IsEmpty() && (L.Kind == TEXT("salt") || L.Kind == TEXT("feed")) && L.Night > 0) Lures.Add(L);
		}
	}
	if (!M || !IsValid(M->Web.Get())) return;
	if (Scene.Get() != M->Web.Get()) { bBuilt = false; Props.Reset(); }
	Scene = M->Web;
	if (Build()) SetPropsShown(true);
}

void FS47TownUse::Stop()
{
	bLive = false;
	Basket.Reset();
	if (IsValid(Scene.Get())) SetPropsShown(false);
}

void FS47TownUse::Step(double Dt)
{
	(void)Dt;
	if (!M || !M->IsFreeform() || !IsValid(M->Web.Get())) return;
	if (Scene.Get() != M->Web.Get()) { Scene = M->Web; bBuilt = false; Props.Reset(); }
	if (!bBuilt && Build()) SetPropsShown(true);
}

S47J::FObj FS47TownUse::Save() const
{
	auto O = S47J::New();
	if (!Flags.IsEmpty()) { TArray<FString> A = Flags.Array(); A.Sort(); S47J::Set(O, TEXT("seen"), A); }
	if (!SoldPrints.IsEmpty()) { TArray<FString> A = SoldPrints.Array(); A.Sort(); S47J::Set(O, TEXT("sold"), A); }
	if (bMap) S47J::Set(O, TEXT("map"), true);
	if (TourDay) S47J::Set(O, TEXT("tourDay"), TourDay);
	if (SoldDay) { S47J::Set(O, TEXT("soldDay"), SoldDay); S47J::Set(O, TEXT("soldToday"), SoldToday); }
	if (Postcards) S47J::Set(O, TEXT("postcards"), Postcards);
	if (!Lures.IsEmpty())
	{
		TArray<TSharedPtr<FJsonValue>> List;
		for (const FLure& L : Lures)
		{
			auto X = S47J::New(); S47J::Set(X, TEXT("place"), L.Place); S47J::Set(X, TEXT("kind"), L.Kind); S47J::Set(X, TEXT("night"), L.Night);
			if (L.bRead) { S47J::Set(X, TEXT("read"), true); S47J::Set(X, TEXT("said"), L.Said); }
			List.Add(MakeShared<FJsonValueObject>(X));
		}
		O->SetArrayField(TEXT("lures"), List);
	}
	return O;
}

// ---------- the counters in the world ----------

FVector FS47TownUse::CounterPoint(const FString& Counter) const
{
	// Three-space, the road's land (x + 8000). The lobby's cork board on its south end wall, across from the door (Config/Props/
	// radio_station.json: the lobby is x 8019.3 to 8021.94 and z 2031 to 2038.32, floor 4.497; its east wall is the studio glass); a
	// stand on the Feed & Seed's floor by the loading door, clear of its crates (CalicheAccess Feed: counter x 7981.6 to 7982.33, z
	// 2195.4 to 2200.1); a card standing on the souvenir shop's own cash counter (Tools/calichestreet/souvenir/build_souvenir.py:
	// CashCounterTop at Blender 1.80, -2.72, top 1.20 m; world x = 8025 + Blender y, z = 2064 + Blender x).
	if (Counter == S47TownUseLocal::Radio) return FVector(8020.6, 4.497 + 1.55, 2038.30);
	if (Counter == S47TownUseLocal::Feed)
	{
		const double F = S47CalicheAccess::FloorAt(7983.9, 2198.6);
		return FVector(7983.9, (FMath::IsFinite(F) ? F : 11.3256) + 1.38, 2198.6);
	}
	const double F = S47CalicheAccess::FloorAt(8021.6, 2065.2);
	return FVector(8022.28, (FMath::IsFinite(F) ? F : 4.9528) + 1.2, 2065.0);
}

bool FS47TownUse::Build()
{
	using namespace S47TownUseLocal;
	if (bBuilt) return true;
	if (!M || !IsValid(M->Web.Get()) || !IsValid(M->Inter)) return false;
	AS47Web* W = M->Web.Get();
	const int32 Root = W->Handle(TEXT("world.legs.road.area.group"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* Lit = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
	if (Root < 0 || !Cube || !Lit) return false;
	const TWeakPtr<uint8> Guard = Alive;
	const auto Mat = [W, Lit](uint32 Hex) { UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Lit, W); D->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(Hex)); D->SetScalarParameterValue(TEXT("Roughness"), .85f); D->SetScalarParameterValue(TEXT("NativeLighting"), 1); return D; };
	// A box at a three-space middle and size (x, up, z); usable for Counter when it is not empty (the first such part is what the
	// interactable names).
	TMap<FString, int32> FirstNode;
	const auto Part = [&](const FString& Name, const FVector& At, const FVector& Size, uint32 Hex, const FString& Counter) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, *Name);
		C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(W->GetRootComponent()); C->SetStaticMesh(Cube); C->SetMaterial(0, Mat(Hex)); C->SetCastShadow(true);
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		C->RegisterComponent();
		C->SetWorldLocation(S47Three::ToUE(At)); C->SetWorldScale3D(FVector(Size.X, Size.Z, Size.Y));
		const int32 Node = W->AdoptNode(C, Root, Name);
		if (!Counter.IsEmpty() && Node >= 0) { W->MarkUsable(Node, UseId(Counter)); if (!FirstNode.Contains(Counter)) FirstNode.Add(Counter, Node); }
		Props.Add(C);
		return C;
	};
	const auto Words = [&](const FString& Name, const FString& Text, const FVector& At, double FacingYaw, double Size, const FColor& Ink)
	{
		UTextRenderComponent* T = NewObject<UTextRenderComponent>(W, *Name);
		T->SetMobility(EComponentMobility::Movable); T->SetupAttachment(W->GetRootComponent());
		T->SetText(FText::FromString(Text)); T->SetWorldSize(float(Size * 100.0)); T->SetHorizontalAlignment(EHTA_Center); T->SetVerticalAlignment(EVRTA_TextCenter);
		T->SetTextRenderColor(Ink); T->SetCollisionEnabled(ECollisionEnabled::NoCollision); T->SetCastShadow(false); T->RegisterComponent();
		T->SetWorldLocationAndRotation(S47Three::ToUE(At), FRotator(0, FacingYaw, 0));
		W->AdoptNode(T, Root, Name);
		Props.Add(T);
	};
	const FColor Ink(30, 28, 26), Chalk(232, 228, 210);
	// 1420 AM: a cork board on the lobby's south end wall, facing the door (north: Unreal yaw -90).
	{
		const FVector At = CounterPoint(Radio);
		Part(TEXT("S47TownRadioBoard"), At, FVector(.95, .75, .03), 0x9a7a52, Radio);
		Words(TEXT("S47TownRadioWords"), TEXT("1420 AM\nTIPS & REQUESTS"), At + FVector(0, .2, -.02), -90, .085, Ink);
	}
	// Feed & Seed: a post and a board on the floor between the counter and the front wall, readable from the door and the counter.
	{
		const FVector Board = CounterPoint(Feed);
		Part(TEXT("S47TownFeedPost"), FVector(Board.X, Board.Y - .69, Board.Z), FVector(.06, 1.38, .06), 0x4a3b2c, Feed);
		Part(TEXT("S47TownFeedBoard"), Board, FVector(.03, .45, .62), 0x2b2a27, Feed);
		Words(TEXT("S47TownFeedWordsE"), TEXT("FEED & SEED\nASK HERE"), Board + FVector(.02, 0, 0), 0, .075, Chalk);
		Words(TEXT("S47TownFeedWordsW"), TEXT("FEED & SEED\nASK HERE"), Board + FVector(-.02, 0, 0), 180, .075, Chalk);
	}
	// Souvenirs: a card standing on the shop's own cash counter, toward the door and the window (west: Unreal yaw 180).
	{
		const FVector Top = CounterPoint(Souvenir);
		Part(TEXT("S47TownSouvenirSign"), FVector(Top.X, Top.Y + .19, Top.Z), FVector(.03, .36, .6), 0x2b2a27, Souvenir);
		Words(TEXT("S47TownSouvenirWords"), TEXT("TOURS - FILM\nWE BUY PHOTOS"), FVector(Top.X - .02, Top.Y + .19, Top.Z), 180, .055, Chalk);
	}
	const auto Label = [this, Guard](const FString& Counter) -> FString
	{
		if (!Guard.IsValid() || !bLive || !M || !M->IsFreeform()) return FString();
		const bool bOpen = OpenAt(Counter, Hour());
		const FString Name = Counter == S47TownUseLocal::Radio ? TEXT("1420 AM / tips and requests") : Counter == S47TownUseLocal::Feed ? TEXT("Feed & Seed / the counter") : TEXT("Souvenirs / the counter");
		return bOpen ? Name : Name + TEXT(" (closed)");
	};
	for (const FString& Counter : { FString(Radio), FString(Feed), FString(Souvenir) })
	{
		FS47Interactable I; I.Id = UseId(Counter); I.Range = 2.8;
		if (const int32* Node = FirstNode.Find(Counter)) I.Object = W->ObjAt(*Node);
		I.Label = [Label, Counter] { return Label(Counter); };
		I.Use = [this, Guard, Counter] { if (!Guard.IsValid() || !bLive) return; if (Counter == S47TownUseLocal::Radio) OpenRadio(false); else if (Counter == S47TownUseLocal::Feed) OpenFeed(); else OpenSouvenir(); };
		I.Examine = [Counter]
		{
			return Counter == S47TownUseLocal::Radio ? FString(TEXT("A cork board: lost dogs, a hay baler, the church supper. A slot marked TIPS & REQUESTS."))
				: Counter == S47TownUseLocal::Feed ? FString(TEXT("A price board in chalk: salt, range cubes, wire, a few things for a truck."))
				: FString(TEXT("SOUVENIRS. TOURS. FILM. Painted by the same hand as the sign outside."));
		};
		M->Inter->Add(I);
	}
	bBuilt = true;
	UE_LOG(LogS47TownUse, Display, TEXT("S47: town counters ready: 1420 AM lobby board, Feed & Seed stand, souvenir counter (%d parts)"), Props.Num());
	return true;
}

void FS47TownUse::SetPropsShown(bool bShow)
{
	for (const TWeakObjectPtr<UPrimitiveComponent>& P : Props)
		if (UPrimitiveComponent* C = P.Get())
		{
			C->SetHiddenInGame(!bShow);
			if (!C->IsA<UTextRenderComponent>()) C->SetCollisionEnabled(bShow ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		}
	if (AS47Web* W = Scene.Get()) for (FS47Rect& R : W->CollidersMutable(TEXT("road"))) if (R.Id.StartsWith(TEXT("townuse:"))) R.bEnabled = bShow;
}

// ---------- the basket ----------

void FS47TownUse::BasketRows(const FString& Counter, const TArray<FWare>& Wares, TArray<TSharedRef<SWidget>>& Rows, TFunction<void()> Reopen)
{
	if (BasketAt != Counter) { Basket.Reset(); BasketAt = Counter; }
	int32 Total = 0;
	for (const FWare& Ware : Wares)
	{
		const FString Id = Ware.Id;
		const int32 Count = Basket.FindRef(Id), Have = M->Freeform->InventoryCount(Id);
		Total += Count * Ware.Price;
		Rows.Add(S47Kit::Two(
			S47Kit::Col({ S47Kit::Text(FString::Printf(TEXT("%s / $%d"), *FS47FreeformWorld::SupplyTitle(Id), Ware.Price)), S47Kit::Small(FString::Printf(TEXT("%s At SARO: %d."), Ware.For, Have)) }, 2.f),
			S47Kit::Row({ S47Kit::Button(TEXT("-"), [this, Id, Reopen] { if (int32* N = Basket.Find(Id)) { if (--*N <= 0) Basket.Remove(Id); } Reopen(); }, false, Count > 0),
				S47Kit::Text(FString::Printf(TEXT("  %d  "), Count)),
				S47Kit::Button(TEXT("+"), [this, Id, Reopen] { int32& N = Basket.FindOrAdd(Id); N = FMath::Min(N + 1, 9); Reopen(); }, false, Count < 9 && Have + Count < 99) }),
			.72f, .28f));
	}
	const bool bHands = M->Freeform->Field && M->Freeform->Field->ParcelStage() == 0;
	if (!bHands) Rows.Add(S47Kit::Small(TEXT("You already have a parcel out. Load it in the truck or unpack it at SARO's receiving table first; one parcel at a time.")));
	Rows.Add(S47Kit::Button(Total > 0 ? FString::Printf(TEXT("Pay $%d and take the parcel / budget $%d"), Total, M->Freeform->Budget()) : FString(TEXT("Nothing in the basket")),
		[this, Counter, Wares] { if (Pay(Counter, Wares) && M && M->UI) M->UI->Close(); }, true, Total > 0 && bHands && M->Freeform->Budget() >= Total));
}

bool FS47TownUse::Pay(const FString& Counter, const TArray<FWare>& Wares)
{
	if (!M || !M->Freeform || !M->Freeform->Field || Basket.IsEmpty()) return false;
	int32 Total = 0;
	for (const auto& Pair : Basket)
	{
		const FWare* Ware = Wares.FindByPredicate([&Pair](const FWare& Wr) { return Pair.Key == Wr.Id; });
		if (!Ware) return false;
		Total += Ware->Price * Pair.Value;
	}
	FS47FreeformWorld& Field = *M->Freeform->Field;
	if (Field.ParcelStage() != 0 || (M->World && M->World->bDriving)) { if (M->UI) M->UI->Toast(TEXT("One parcel at a time: load or unpack the one you have first."), 4); return false; }
	if (!M->Freeform->Spend(Total)) { if (M->UI) M->UI->Toast(M->Freeform->FeedbackText(), 4); return false; }
	const FString Where = Counter == S47TownUseLocal::Radio ? TEXT("1420 AM") : Counter == S47TownUseLocal::Feed ? TEXT("the Feed & Seed") : TEXT("the souvenir shop");
	if (!Field.TakePostedParcel(Basket, CounterPoint(Counter), FString::Printf(TEXT("Parcel from %s carried. Load it in the truck, then unpack it at SARO's GARAGE RECEIVING table into stock."), *Where)))
	{
		M->Freeform->Earn(Total);
		if (M->UI) M->UI->Toast(TEXT("The parcel could not be handed over here. Nothing was charged."), 4);
		return false;
	}
	UE_LOG(LogS47TownUse, Display, TEXT("S47: town: paid $%d at %s for %d kinds of supply"), Total, *Counter, Basket.Num());
	if (M->UI) M->UI->Toast(FString::Printf(TEXT("Paid $%d. The parcel is in your hands: load it in the truck and unpack it at SARO."), Total), 5);
	Basket.Reset();
	M->RequestAutosave();
	return true;
}

// ---------- 1420 AM ----------

FString FS47TownUse::TipRows(TArray<TSharedRef<SWidget>>& Rows, bool bPhone)
{
	FS47FreeformSession& F = *M->Freeform;
	FS47RadioTips* Tips = F.Tips.Get();
	if (!Tips) return FString();
	const FString Notes = Tips->TakeNotes();
	if (!Notes.IsEmpty()) Rows.Add(S47Kit::Paper(bPhone ? TEXT("Earl, before you start: \"") + Notes.Replace(TEXT(" E."), TEXT("")) + TEXT("\"") : TEXT("A note on the spike with your name on it:\n\n") + Notes));
	Rows.Add(S47Kit::Section(TEXT("A TIP FOR THE AIR"), TEXT("one a day")));
	// What the player can tell of: places found on the land, and places where something went on in the last three nights.
	TArray<TPair<FString, FString>> Can;
	const auto AddPlace = [&Can, Tips](const FString& Id, const FString& Title) { if (!Tips->Gave(Id) && !Can.ContainsByPredicate([&Id](const TPair<FString, FString>& C) { return C.Key == Id; })) Can.Emplace(Id, Title); };
	if (F.Places) for (const FS47Places::FDef& D : FS47Places::Defs()) if (F.Places->Found(D.Id) && FString(D.Id) != TEXT("motel")) AddPlace(D.Id, D.Title);
	if (F.EventPlaces) for (const FS47EventPlace& P : F.EventPlaces->All()) { const int32 Last = F.EventPlaces->LastNight(P.Id); if (Last > 0 && Last >= Night() - 3) AddPlace(P.Id, P.Title); }
	if (!Tips->CanGive()) Rows.Add(S47Kit::Small(TEXT("Earl takes one a day from any one caller. Tomorrow.")));
	else if (Can.IsEmpty()) Rows.Add(S47Kit::Small(TEXT("Nothing to tell him yet: a place you have been out to, or one where something went on in the last few nights.")));
	else
	{
		Rows.Add(S47Kit::Small(TEXT("He reads it on 1420 in his own time, and the county calls back about it. Listen in the truck (R in the cab).")));
		for (int32 I = 0; I < FMath::Min(Can.Num(), 8); ++I)
		{
			const FString Id = Can[I].Key, Title = Can[I].Value;
			Rows.Add(S47Kit::Button(FString::Printf(TEXT("Something going on out by %s"), *Title), [this, Id, Title, bPhone]
			{
				if (!M || !M->Freeform || !M->Freeform->Tips || !M->Freeform->Tips->Give(Id)) return;
				if (M->UI) { M->UI->Close(); M->UI->Toast(bPhone ? TEXT("\"Got it. Tonight, if the phones let me.\" Earl hangs up first.") : TEXT("Into the slot. Earl reads them out at night, on 1420."), 5); }
			}));
		}
	}
	for (const FS47RadioTips::FGiven& G : Tips->Given())
	{
		const FString Title = F.EventPlaces ? F.EventPlaces->TitleOf(G.Place) : G.Place;
		const FString State = G.Stage == 0 ? TEXT("waiting for Earl") : G.Stage == 1 ? TEXT("read on the air; nobody has called back yet") : TEXT("called back: ") + G.Back;
		Rows.Add(S47Kit::Small(FString::Printf(TEXT("Day %d, %s: %s"), G.Day, *Title, *State)));
	}
	return Notes;
}

void FS47TownUse::OpenRadio(bool bPhone)
{
	using namespace S47TownUseLocal;
	if (!M || !M->UI || !M->Freeform) return;
	TArray<TSharedRef<SWidget>> Rows;
	if (!bPhone && !OpenAt(Radio, Hour()))
	{
		Rows.Add(S47Kit::Paper(FString::Printf(TEXT("1420 AM. The lobby is locked. A card in the door: LOBBY %s. TIPS AND REQUESTS ANY HOUR ON THE STUDIO LINE."), *HoursOf(Radio).ToUpper())));
		M->UI->Open(S47Kit::Shell(M->UI, TEXT("1420 AM / LOBBY"), S47Kit::Col(Rows), 760));
		return;
	}
	Rows.Add(S47Kit::Paper(bPhone ? TEXT("The request line rings a long time. Then Earl, eating something: \"Fourteen-twenty, you're on. No, you're not. Go ahead.\"")
		: TEXT("1420 AM. A cork board by the studio glass: lost dogs, a hay baler for sale, the church supper. A slot in a box marked TIPS & REQUESTS, and a card: USED RADIO PARTS & BATTERIES, ASK EARL.")));
	TipRows(Rows, bPhone);
	if (!bPhone)
	{
		Rows.Add(S47Kit::Section(TEXT("USED RADIO PARTS AND BATTERIES")));
		BasketRows(Radio, RadioWares(), Rows, [this] { OpenRadio(false); });
	}
	Rows.Add(S47Kit::Button(bPhone ? TEXT("Hang up") : TEXT("Leave"), [this] { if (M && M->UI) M->UI->Close(); }));
	M->UI->Open(S47Kit::Shell(M->UI, bPhone ? TEXT("1420 AM / REQUEST LINE") : TEXT("1420 AM / LOBBY"), S47Kit::Col(Rows), 860));
}

// ---------- Feed & Seed ----------

void FS47TownUse::OpenFeed()
{
	using namespace S47TownUseLocal;
	if (!M || !M->UI || !M->Freeform) return;
	TArray<TSharedRef<SWidget>> Rows;
	if (!OpenAt(Feed, Hour()))
	{
		Rows.Add(S47Kit::Paper(FString::Printf(TEXT("FEED & SEED. Nobody behind the counter. The hours are painted on the door: %s, CLOSED SUNDAYS EXCEPT IN CALVING."), *HoursOf(Feed).ToUpper())));
		M->UI->Open(S47Kit::Shell(M->UI, TEXT("FEED & SEED"), S47Kit::Col(Rows), 760));
		return;
	}
	Rows.Add(S47Kit::Paper(TEXT("FEED & SEED. Range cubes on a pallet, salt blocks stacked like bricks, rolls of wire, a rack of tools. The man behind the counter writes every sale in a receipt book with a carpenter's pencil and asks after the weather out your way.")));
	BasketRows(Feed, FeedWares(), Rows, [this] { OpenFeed(); });
	Rows.Add(S47Kit::Small(TEXT("He sells spades, jacks and lug wrenches too. SARO's set rides behind your seat already. A lantern takes the same cells.")));
	Rows.Add(S47Kit::Button(TEXT("Leave"), [this] { if (M && M->UI) M->UI->Close(); }));
	M->UI->Open(S47Kit::Shell(M->UI, TEXT("FEED & SEED"), S47Kit::Col(Rows), 900));
}

// ---------- the souvenir shop ----------

bool FS47TownUse::BuyMap()
{
	if (!M || !M->Freeform || bMap) return false;
	if (!M->Freeform->Spend(2)) { if (M->UI) M->UI->Toast(M->Freeform->FeedbackText(), 4); return false; }
	bMap = true;
	// The ones that are out there go on the survey sheet as heard of; the rest are pencilled in from the map as they stand.
	if (M->Freeform->Places) for (const FTourSite& S : SaucerMap()) if (S.Place) M->Freeform->Places->Hear(S.Place);
	Note(TEXT("map"));
	UE_LOG(LogS47TownUse, Display, TEXT("S47: town: saucer map bought, %d sites pencilled in"), SaucerMap().Num());
	return true;
}

bool FS47TownUse::Tour()
{
	if (!M || !M->Freeform || !M->UI) return false;
	FS47FreeformSession& F = *M->Freeform;
	if (TourDay == F.Day()) return false;
	if (!F.Spend(5)) { M->UI->Toast(F.FeedbackText(), 4); return false; }
	TourDay = F.Day();
	F.SpendTime(2 * 3600);
	Note(TEXT("tour"));
	// He always knows one more place: somewhere the player has neither found nor heard of, by the game's seed and the day.
	const FString New = F.Places ? F.Places->NextUnknown(F.Day() * 7 + 3) : FString();
	FString Said = S47TownUseLocal::Patter[F.Day() % 3];
	if (!New.IsEmpty())
	{
		F.Places->Hear(New);
		const FString Title = F.EventPlaces ? F.EventPlaces->TitleOf(New) : New;
		Said += FString::Printf(TEXT("\n\nOn the way back, not part of the tour, he says: \"You want my advice, go look at %s. I don't take people there. I don't go there.\" It is on your map now, with a question mark."), *Title);
	}
	else Said += TEXT("\n\nOn the way back he asks what you people at the dishes have found. You tell him about as much as he told you.");
	UE_LOG(LogS47TownUse, Display, TEXT("S47: town: tour taken on day %d, points at %s"), F.Day(), New.IsEmpty() ? TEXT("nothing new") : *New);
	TArray<TSharedRef<SWidget>> Rows;
	Rows.Add(S47Kit::Paper(Said));
	Rows.Add(S47Kit::Button(TEXT("Back to the shop"), [this] { OpenSouvenir(); }, true));
	M->UI->Open(S47Kit::Shell(M->UI, TEXT("SAUCER CRASH 1947 / THE TOUR"), S47Kit::Col(Rows), 820));
	M->RequestAutosave();
	return true;
}

bool FS47TownUse::SellCopy(const FString& PhotoId)
{
	if (!M || !M->Freeform || !M->Night || !M->Night->Album) return false;
	FS47FreeformSession& F = *M->Freeform;
	if (SoldDay != F.Day()) { SoldDay = F.Day(); SoldToday = 0; }
	if (SoldToday >= PrintsADay || SoldPrints.Contains(PhotoId)) return false;
	const FS47AlbumShot* Shot = M->Night->Album->Shots().FindByPredicate([&PhotoId](const FS47AlbumShot& S) { return S.Id == PhotoId; });
	if (!Shot) return false;
	bool bRecord = false;
	if (F.Director) for (const auto& O : F.Director->Observations()) bRecord |= O.PhotoId == PhotoId;
	FString Why;
	const int32 Pay = PrintValue(*Shot, bRecord, Why);
	if (Pay <= 0) return false;
	// He has copies made off it at the drugstore in Socorro: the player keeps the print, the field record keeps its photograph.
	SoldPrints.Add(PhotoId); ++SoldToday;
	F.Earn(Pay);
	if (Shot->bOdd || Shot->Targets.Num() > 0) Note(TEXT("sold_odd"));
	UE_LOG(LogS47TownUse, Display, TEXT("S47: town: a copy of print %s sold for $%d (%s)"), *PhotoId, Pay, *Why);
	if (M->UI) M->UI->Toast(FString::Printf(TEXT("$%d for the right to copy it. You keep your print."), Pay), 4);
	M->RequestAutosave();
	return true;
}

void FS47TownUse::SellRows(TArray<TSharedRef<SWidget>>& Rows)
{
	FS47FreeformSession& F = *M->Freeform;
	if (SoldDay != F.Day()) { SoldDay = F.Day(); SoldToday = 0; }
	Rows.Add(S47Kit::Section(TEXT("HE BUYS PRINTS"), FString::Printf(TEXT("%d of %d today"), SoldToday, PrintsADay)));
	Rows.Add(S47Kit::Small(TEXT("He has copies made off your print in Socorro and sells them as postcards. You keep the print. A dollar for anything, more for the dishes, for night, for what the station looks for, and a good deal more for the strange ones.")));
	const FS47Album* Album = M->Night && M->Night->Album ? M->Night->Album.Get() : nullptr;
	if (!Album) return;
	struct FOffer { FString Id, Caption; int32 Pay = 0; FString Why; };
	TArray<FOffer> Offers;
	for (const FS47AlbumShot& S : Album->Shots())
	{
		if (SoldPrints.Contains(S.Id)) continue;
		bool bRecord = false;
		if (F.Director) for (const auto& O : F.Director->Observations()) bRecord |= O.PhotoId == S.Id;
		FOffer O; O.Id = S.Id; O.Pay = PrintValue(S, bRecord, O.Why);
		if (O.Pay <= 0) continue;
		O.Caption = (S.Place.IsEmpty() ? FString() : S.Place + TEXT(": ")) + S.Text;
		Offers.Add(O);
	}
	Offers.Sort([](const FOffer& A, const FOffer& B) { return A.Pay > B.Pay; });
	if (Offers.IsEmpty()) { Rows.Add(S47Kit::Small(TEXT("Nothing developed to show him, or he has copies of all of it."))); return; }
	for (int32 I = 0; I < FMath::Min(6, Offers.Num()); ++I)
	{
		const FOffer O = Offers[I];
		Rows.Add(S47Kit::Two(S47Kit::Small(O.Caption.Left(140) + TEXT(" (") + O.Why + TEXT(")")),
			S47Kit::Button(FString::Printf(TEXT("Sell a copy / $%d"), O.Pay), [this, O] { SellCopy(O.Id); OpenSouvenir(); }, false, SoldToday < PrintsADay), .7f, .3f));
	}
}

void FS47TownUse::OpenSouvenir()
{
	using namespace S47TownUseLocal;
	if (!M || !M->UI || !M->Freeform) return;
	FS47FreeformSession& F = *M->Freeform;
	TArray<TSharedRef<SWidget>> Rows;
	if (!OpenAt(Souvenir, Hour()))
	{
		Rows.Add(S47Kit::Paper(FString::Printf(TEXT("SAUCER CRASH 1947. CLOSED. The hours on the card in the door: %s. Under them, in pen: OR KNOCK."), *HoursOf(Souvenir).ToUpper())));
		M->UI->Open(S47Kit::Shell(M->UI, TEXT("SAUCER CRASH 1947"), S47Kit::Col(Rows), 760));
		return;
	}
	Rows.Add(S47Kit::Paper(TEXT("SAUCER CRASH 1947. Postcards on a wire rack, rubber spacemen in a bin, a hand-drawn map under the glass. The man from the stoop is behind the counter in the daytime, in the same hat.")));
	// The map of saucer sites: once.
	if (!bMap)
		Rows.Add(S47Kit::Two(S47Kit::Small(TEXT("A map of the saucer sites, drawn by hand. Some of them are out there. He will not say which.")),
			S47Kit::Button(TEXT("Buy the map / $2"), [this] { BuyMap(); OpenSouvenir(); }, false, F.Budget() >= 2), .7f, .3f));
	else
	{
		TArray<FString> Names;
		for (const FTourSite& S : SaucerMap()) Names.Add(S.Label);
		Rows.Add(S47Kit::Small(TEXT("Your copy of his map, pencilled onto the survey sheet: ") + FString::Join(Names, TEXT(", ")) + TEXT(".")));
	}
	// The tour: two hours, once a day, while he has the light for it.
	const bool bTourHours = Hour() >= 10 && Hour() < 15;
	Rows.Add(S47Kit::Two(S47Kit::Small(TourDay == F.Day() ? TEXT("You took the tour today.") : bTourHours ? TEXT("The tour: two hours in his station wagon, the sites the book talks about. He always knows one more.") : TEXT("Tours leave between 10 AM and 3 PM.")),
		S47Kit::Button(TEXT("Take the tour / $5 / 2 hours"), [this] { Tour(); }, false, bTourHours && TourDay != F.Day() && F.Budget() >= 5), .7f, .3f));
	// Postcards and a rubber spaceman: the town notices both.
	Rows.Add(S47Kit::Two(S47Kit::Small(FString::Printf(TEXT("Postcards of the dishes at sunset, five for a dollar. You have bought %d."), Postcards)),
		S47Kit::Button(TEXT("Five postcards / $1"), [this] { if (M->Freeform->Spend(1)) { Postcards += 5; Note(TEXT("postcard")); M->RequestAutosave(); } OpenSouvenir(); }, false, F.Budget() >= 1), .7f, .3f));
	Rows.Add(S47Kit::Two(S47Kit::Small(Seen(TEXT("spaceman")) ? TEXT("The rubber spaceman rides on your dash now. People notice.") : TEXT("A rubber spaceman with a suction cup, gray, big eyes. For the dash.")),
		S47Kit::Button(TEXT("A rubber spaceman / $3"), [this] { if (M->Freeform->Spend(3)) Note(TEXT("spaceman")); OpenSouvenir(); }, false, !Seen(TEXT("spaceman")) && F.Budget() >= 3), .7f, .3f));
	Rows.Add(S47Kit::Section(TEXT("FILM")));
	BasketRows(Souvenir, SouvenirWares(), Rows, [this] { OpenSouvenir(); });
	SellRows(Rows);
	Rows.Add(S47Kit::Button(TEXT("Leave"), [this] { if (M && M->UI) M->UI->Close(); }));
	M->UI->Open(S47Kit::Shell(M->UI, TEXT("SAUCER CRASH 1947 / SOUVENIRS"), S47Kit::Col(Rows), 940));
}

// ---------- the survey sheet ----------

void FS47TownUse::FillMap(FS47MapView& V) const
{
	double SaroX = 0, SaroZ = 0;
	S47OpenLand::RoadOf(TEXT("saro"), 9, 0, SaroX, SaroZ);
	(void)SaroX;
	if (bMap) for (const FTourSite& S : SaucerMap()) if (!S.Place) { FS47MapPin P; P.East = S.X; P.South = S.Z - SaroZ; P.Label = S.Label; P.bHeard = true; V.Pins.Add(P); }
	if (M && M->Freeform && M->Freeform->Places)
		for (const FLure& L : Lures)
		{
			double X = 0, Z = 0;
			if (!M->Freeform->Places->Spot(L.Place, X, Z)) continue;
			FS47MapPin P; P.East = X; P.South = Z - SaroZ; P.Label = L.Kind == TEXT("salt") ? TEXT("salt") : TEXT("cubes"); P.bHeard = false; V.Pins.Add(P);
		}
}

// ---------- lures ----------

bool FS47TownUse::SetLure(const FString& Place, const FString& Kind)
{
	if (!M || !M->Freeform || (Kind != TEXT("salt") && Kind != TEXT("feed")) || !FS47Places::Def(Place)) return false;
	if (!M->Freeform->Consume(Kind)) return false;
	Lures.RemoveAll([&Place](const FLure& L) { return L.Place == Place; });
	FLure L; L.Place = Place; L.Kind = Kind; L.Night = Night();
	Lures.Add(L);
	Note(TEXT("lure"));
	UE_LOG(LogS47TownUse, Display, TEXT("S47: town: %s set out at %s, night %d"), *Kind, *Place, L.Night);
	M->RequestAutosave();
	return true;
}

FString FS47TownUse::ReadLure(const FString& Place)
{
	FLure* L = Lures.FindByPredicate([&Place](const FLure& X) { return X.Place == Place; });
	if (!L || !M || !M->Freeform) return FString();
	if (L->bRead) return L->Said;
	if (Night() <= L->Night) return FString();
	FS47FreeformSession& F = *M->Freeform;
	double X = 0, Z = 0;
	if (!F.Places || !F.Places->Spot(Place, X, Z)) return FString();
	TArray<TPair<FVector2D, int32>> Used;
	if (F.EventPlaces) for (const FS47EventPlace& P : F.EventPlaces->All()) if (const int32 Last = F.EventPlaces->LastNight(P.Id)) Used.Emplace(FVector2D(P.X, P.Z), Last);
	const bool bShy = Shy(FVector2D(X, Z), L->Night, Used);
	const bool bSalt = L->Kind == TEXT("salt");
	const int32 Pick = int32((FCrc::StrCrc32(*Place) + uint32(L->Night) * 31u) % 3u);
	static const TCHAR* const SaltSaid[] = {
		TEXT("Licked hollow on one side. Cattle tracks all round it, and a coyote's along the edge of them."),
		TEXT("Licked down a little. Deer tracks, a doe and a small one, and they took their time."),
		TEXT("Licked, and the ground round it trampled. Cattle, by the tracks and by what they left.") };
	static const TCHAR* const FeedSaid[] = {
		TEXT("The cubes are gone. Javelina, by the tracks and the mess they made of the sack."),
		TEXT("Gone. Coyote tracks over cattle tracks, and the coyote came last."),
		TEXT("Half gone. Quail tracks everywhere, and a good many cattle.") };
	if (bShy)
		L->Said = FString::Printf(TEXT("The %s is as you left it. Tracks come in from three sides, cattle and something smaller, and every one of them stops twenty feet short and turns back the way it came."), bSalt ? TEXT("salt block") : TEXT("sack of cubes"));
	else L->Said = bSalt ? SaltSaid[Pick] : FeedSaid[Pick];
	L->bRead = true;
	if (bShy)
	{
		const FString Id = FString::Printf(TEXT("lure_shy_%s_%d"), *Place, L->Night);
		const FString Where = F.EventPlaces ? F.EventPlaces->TitleOf(Place) : Place;
		if (!F.HasFinding(Id)) F.RegisterFinding(Id, FString::Printf(TEXT("%s set out at %s: untouched. Animal tracks stop short of it and turn back."), bSalt ? TEXT("Salt") : TEXT("Range cubes"), *Where));
	}
	UE_LOG(LogS47TownUse, Display, TEXT("S47: town: %s at %s read: %s"), *L->Kind, *Place, bShy ? TEXT("shy") : TEXT("ordinary"));
	M->RequestAutosave();
	return L->Said;
}

void FS47TownUse::LureRows(const FString& Place, TArray<TSharedRef<SWidget>>& Rows)
{
	if (!M || !M->Freeform) return;
	const FLure* L = Lures.FindByPredicate([&Place](const FLure& X) { return X.Place == Place; });
	if (L)
	{
		const FString Said = ReadLure(Place);
		if (!Said.IsEmpty()) Rows.Add(S47Kit::Paper(FString::Printf(TEXT("The %s you set out: %s"), L->Kind == TEXT("salt") ? TEXT("salt") : TEXT("range cubes"), *Said)));
		else Rows.Add(S47Kit::Small(TEXT("Your lure is out here. Come back after a night has gone by.")));
		if (Said.IsEmpty()) return;
	}
	for (const TCHAR* Kind : { TEXT("salt"), TEXT("feed") })
	{
		if (M->Freeform->InventoryCount(Kind) <= 0) continue;
		const FString K = Kind;
		Rows.Add(S47Kit::Button(K == TEXT("salt") ? TEXT("Set out a salt lick block here") : TEXT("Set out a sack of range cubes here"), [this, Place, K]
		{
			if (SetLure(Place, K) && M && M->UI) { M->UI->Close(); M->UI->Toast(TEXT("Set out where it will be seen from the open side. Come back after a night has gone by."), 5); }
		}));
	}
}

// ---------- F10 ----------

FString FS47TownUse::DevTour()
{
	if (!M || !M->Freeform) return FString();
	TourDay = 0;
	if (M->Freeform->Budget() < 5) M->Freeform->Earn(5);
	return Tour() ? FString(TEXT("The tour: two hours gone, a place heard of if any were left.")) : FString();
}

bool FS47TownUse::DevLureNight(const FString& Place, bool bShy, FString& Said)
{
	if (!M || !M->Freeform || !M->Freeform->Places) return false;
	M->Freeform->AddItem(TEXT("salt"), 1);
	if (!SetLure(Place, TEXT("salt"))) return false;
	FLure* L = Lures.FindByPredicate([&Place](const FLure& X) { return X.Place == Place; });
	if (!L) return false;
	// As if it had been set out last night; and, for a shy one, something came by it last night.
	L->Night = Night() - 1;
	if (bShy && M->Freeform->EventPlaces)
	{
		double X = 0, Z = 0;
		M->Freeform->Places->Spot(Place, X, Z);
		if (const FS47EventPlace* P = M->Freeform->EventPlaces->NearestTo(FVector2D(X, Z), ShyReach)) M->Freeform->EventPlaces->Used(P->Id, TEXT("dev_lure"));
	}
	Said = ReadLure(Place);
	return !Said.IsEmpty();
}

// ---------- tests ----------

bool FS47TownUse::SelfTest(TArray<FString>& Lines)
{
	using namespace S47TownUseLocal;
	bool bAll = true;
	const auto Check = [&](bool bOk, const FString& What) { bAll &= bOk; Lines.Add(FString(bOk ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("town use: ") + What); };
	Check(OpenAt(Feed, 7) && !OpenAt(Feed, 18) && OpenAt(Souvenir, 10) && !OpenAt(Souvenir, 9.9) && OpenAt(Radio, 22.5) && !OpenAt(Radio, 2), TEXT("the feed store keeps ranch hours, the souvenir man tourist ones, the lobby is shut in the small hours"));
	bool bSupplies = true; FString Off;
	for (const FString& C : { FString(Radio), FString(Feed), FString(Souvenir) })
		for (const FWare& Wr : WaresOf(C)) if (FS47FreeformWorld::ShopPrice(Wr.Id) <= 0 || Wr.Price <= 0 || Wr.Price > 40) { bSupplies = false; Off = Wr.Id; }
	Check(bSupplies, bSupplies ? FString(TEXT("everything sold is a supply SARO's stock takes, so it goes by parcel to the receiving table")) : TEXT("not a supply: ") + Off);
	TSet<FString> FeedIds; for (const FWare& Wr : FeedWares()) FeedIds.Add(Wr.Id);
	Check(FeedIds.Contains(TEXT("water")) && FeedIds.Contains(TEXT("spare")) && FeedIds.Contains(TEXT("cell")) && FeedIds.Contains(TEXT("rope")) && FeedIds.Contains(TEXT("wire")) && FeedIds.Contains(TEXT("salt")) && FeedIds.Contains(TEXT("feed")),
		TEXT("the Feed & Seed has water, a spare, cells, rope, wire, salt and range cubes"));
	// The saucer map: real places are FS47Places', the nonsense ones are on the land and nowhere near anything.
	int32 Real = 0, Bunk = 0; bool bMapOk = true;
	for (const FTourSite& S : SaucerMap())
	{
		bMapOk &= S47OpenLand::InRim(S.X, S.Z, 30);
		if (S.Place) { ++Real; const FS47Places::FDef* D = FS47Places::Def(S.Place); bMapOk &= D && FVector2D::Distance(FVector2D(D->X, D->Z), FVector2D(S.X, S.Z)) < 1; continue; }
		++Bunk;
		for (const FS47Places::FDef& D : FS47Places::Defs()) if (FString(D.Id) != TEXT("motel")) bMapOk &= FVector2D::Distance(FVector2D(D.X, D.Z), FVector2D(S.X, S.Z)) > 300;
	}
	Check(bMapOk && Real >= 3 && Bunk >= 3, FString::Printf(TEXT("the saucer map has %d real sites and %d that are nothing at all"), Real, Bunk));
	// Animals as a warning: shy when something used a place near it that night or after, not before and not far off.
	{
		const FVector2D At(170, 720);
		const auto One = [](double X, double Z, int32 N) { TArray<TPair<FVector2D, int32>> A; A.Emplace(FVector2D(X, Z), N); return A; };
		Check(Shy(At, 5, One(300, 800, 5)) && Shy(At, 5, One(170, 720, 6)) && !Shy(At, 5, One(170, 720, 4)) && !Shy(At, 5, One(170, 1400, 5)) && !Shy(At, 5, TArray<TPair<FVector2D, int32>>()),
			TEXT("a lure is shy when something came within 500 m of it that night, and only then"));
	}
	// Prints: better ones pay more.
	{
		FS47AlbumShot Plain; Plain.Id = TEXT("a"); Plain.LabStage = 3; Plain.Clock = 12 * 3600; Plain.Text = FS47Album::NOBODY;
		FS47AlbumShot Named = Plain; Named.Text = TEXT("the antennas");
		FS47AlbumShot Night = Named; Night.Clock = 23 * 3600;
		FS47AlbumShot Target = Night; Target.Targets.Emplace(TEXT("starhole"), FVector2D(.5, .5));
		FS47AlbumShot Odd = Target; Odd.bOdd = true;
		FS47AlbumShot Wet = Odd; Wet.LabStage = 1;
		FString Why;
		const int32 A = PrintValue(Plain, false, Why), B = PrintValue(Named, false, Why), C = PrintValue(Night, false, Why), D = PrintValue(Target, false, Why), E = PrintValue(Odd, false, Why), R = PrintValue(Target, true, Why);
		Check(A == 1 && A < B && B < C && C < D && D < R && R < E && E <= 20 && PrintValue(Wet, false, Why) == 0, FString::Printf(TEXT("prints pay $%d, $%d, $%d, $%d, $%d on the record, $%d strange; undeveloped nothing"), A, B, C, D, R, E));
	}
	// Words: no brand and no British spelling anywhere the player reads them here.
	{
		bool bClean = true; FString Bad;
		TArray<FString> Said;
		for (const FString& C : { FString(Radio), FString(Feed), FString(Souvenir) }) for (const FWare& Wr : WaresOf(C)) Said.Add(Wr.For);
		for (const TCHAR* P : Patter) Said.Add(P);
		for (const FTourSite& S : SaucerMap()) Said.Add(S.Label);
		for (const FString& S : Said) if (!CleanWords(S, Bad)) bClean = false;
		Check(bClean, bClean ? FString(TEXT("no brand names and American spelling in the counters' words")) : TEXT("not said here: ") + Bad);
	}
	// What was bought, seen and set out survives a Continue.
	{
		FS47TownUse A(nullptr); A.Flags.Add(TEXT("lure")); A.Flags.Add(TEXT("spaceman")); A.bMap = true; A.TourDay = 4; A.Postcards = 5; A.SoldPrints.Add(TEXT("p1"));
		FLure L; L.Place = TEXT("windmill"); L.Kind = TEXT("salt"); L.Night = 3; L.bRead = true; L.Said = TEXT("Licked."); A.Lures.Add(L);
		FS47TownUse B(nullptr); B.Begin(S47J::Read(S47J::Write(A.Save())));
		Check(B.Seen(TEXT("lure")) && B.Seen(TEXT("spaceman")) && B.HasMap() && B.TourDay == 4 && B.Postcards == 5 && B.SoldPrints.Contains(TEXT("p1")) && B.Lures.Num() == 1 && B.Lures[0].bRead && S47J::Write(B.Save()) == S47J::Write(A.Save()),
			TEXT("the map, the tour, the lures and what the town has seen survive a Continue"));
		FS47TownUse None(nullptr); None.Begin(nullptr);
		Check(None.Save()->Values.Num() == 0, TEXT("an older save has bought nothing and writes nothing"));
	}
	return bAll;
}

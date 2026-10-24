#include "S47Takeaway.h"

#include "S47Audio.h"
#include "S47Evening.h"
#include "S47EveningPack.h"
#include "S47FieldCamera.h"
#include "S47Figures.h"
#include "S47Flashlight.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Seen.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47Truck.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "InputCoreTypes.h"
#include "Widgets/SCompoundWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	// A small action panel owns navigation itself; it never depends on a tiny world target
	// or on a mouse pointer to release the player's hands.
	class STakeawayMenu : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(STakeawayMenu) {} SLATE_END_ARGS()
		void Construct(const FArguments&, US47UI* UI, TArray<TPair<FString,TFunction<void()>>> InActions)
		{
			Actions=MoveTemp(InActions); const TWeakObjectPtr<US47UI> WeakUI=UI; Close=[WeakUI] { if(WeakUI.IsValid()) WeakUI->Close(true); };
			TArray<S47Kit::FW> Rows;
			Rows.Add(S47Kit::Text(TEXT("Your supper and coffee. Set them on a nearby clear surface, keep them for later, or throw them away.")));
			for(int32 I=0;I<Actions.Num();++I) Rows.Add(S47Kit::Row({
				S47Kit::Text(TAttribute<FString>::CreateLambda([this,I] {return Selected==I?FString(TEXT(">")):FString(TEXT(" "));}),TAttribute<int32>(0)),
				S47Kit::Button(Actions[I].Key,[this,I] { Selected=I; Activate(); }) }));
			Rows.Add(S47Kit::Small(TEXT("D-PAD / ARROWS: choose   A / ENTER: confirm   B / ESC: back")));
			ChildSlot[S47Kit::Shell(UI,TEXT("TAKEAWAY / COFFEE"),S47Kit::Col(Rows),620)];
		}
		virtual bool SupportsKeyboardFocus() const override { return true; }
		virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent& E) override
		{
			const FKey K=E.GetKey();
			if(K==EKeys::Escape || K==EKeys::Gamepad_FaceButton_Right) { const auto Fn=Close; if(!E.IsRepeat() && Fn) Fn(); return FReply::Handled(); }
			if(K==EKeys::Up || K==EKeys::Gamepad_DPad_Up || K==EKeys::Down || K==EKeys::Gamepad_DPad_Down) {
				Selected=FMath::Clamp(Selected+((K==EKeys::Up || K==EKeys::Gamepad_DPad_Up)?-1:1),0,Actions.Num()-1); return FReply::Handled().SetUserFocus(AsShared());
			}
			if(K==EKeys::Enter || K==EKeys::Gamepad_FaceButton_Bottom) { if(!E.IsRepeat()) Activate(); return FReply::Handled(); }
			return FReply::Unhandled();
		}
	private:
		TArray<TPair<FString,TFunction<void()>>> Actions;
		TFunction<void()> Close;
		int32 Selected=0;
		void Activate() { if(Actions.IsValidIndex(Selected)) { const auto Fn=Actions[Selected].Value; if(Fn) Fn(); } }
	};
	UMaterialInstanceDynamic* Paint(FS47Group& G, uint32 Color)
	{
		auto* Mat = G.Basic(S47Three::Hex(Color), TEXT("M_Web_Lit_Opaque"));
		if (Mat) Mat->SetScalarParameterValue(TEXT("Roughness"), .82f);
		return Mat;
	}
	void Pose(FS47Group& G, const FVector& P, const FQuat& Q = FQuat::Identity)
	{
		G.Root().Position = P; G.Root().Rotation = S47Three::EulerXYZ(Q); G.Root().Apply();
	}
	void Food(FS47Group& G, bool bUsed)
	{
		auto* Paper = Paint(G, 0xb89a70);
		auto* Fold = Paint(G, 0x806746);
		auto* White = Paint(G, 0xddd5bc);
		if (!bUsed)
		{
			G.Box(nullptr, .23, .30, .16, Paper, FVector(0, .15, 0));
			G.Box(nullptr, .235, .035, .055, Fold, FVector(0, .305, 0));
			G.Box(nullptr, .13, .04, .003, White, FVector(0, .18, -.081));
		}
		else
		{
			G.Box(nullptr, .29, .009, .23, Paper, FVector(0, .006, 0), FVector(0, -.2, 0));
			G.Box(nullptr, .14, .025, .10, Fold, FVector(-.06, .018, .04), FVector(.06, .35, .03));
			G.Box(nullptr, .105, .013, .08, White, FVector(.07, .014, -.05), FVector(0, -.4, 0));
		}
		// Both takeaway orders include the same coffee. Its lid is retained beside the empty cup.
		G.Capsule(nullptr, .042, .065, White, FVector(.245, .073, 0));
		if (!bUsed) G.Sphere(nullptr, .046, White, FVector(.245, .115, 0), FVector(1, .12, 1));
		else
		{
			G.Sphere(nullptr, .035, Paint(G, 0x24180d), FVector(.245, .111, 0), FVector(1, .04, 1));
			G.Sphere(nullptr, .046, White, FVector(.245, .008, .10), FVector(1, .12, 1));
		}
		if (G.Root().C.IsValid())
		{
			TArray<USceneComponent*> Children; G.Root().C->GetChildrenComponents(true, Children);
			for (USceneComponent* C : Children) if (auto* Mesh = Cast<UStaticMeshComponent>(C)) Mesh->SetCastShadow(true);
		}
		G.AddToScene(); G.SetVisible(false);
	}
	void TargetState(FS47Obj O, bool bEnabled, bool bDraw, const FVector& P)
	{
		if (!O.IsValid()) return;
		O.SetPosition(O.Parent().WorldToLocal(P));
		O.SetVisible(bDraw && bEnabled);
		if (const auto* N = O.W->Node(O.I)) if (auto* C = Cast<UStaticMeshComponent>(N->Comp.Get()))
		{
			C->SetVisibility(bDraw && bEnabled);
			C->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		}
	}
}

FS47Takeaway::FS47Takeaway(US47Main* InMain, FS47EveningPack& InPack) : M(InMain), Pack(InPack) {}
FS47Takeaway::~FS47Takeaway()
{
	Lifetime.Reset();
	// UObject shutdown may already have destroyed Story/Web. Release only owned presentation.
	for (const auto& C : Targets) if (C.IsValid()) C->DestroyComponent();
}

void FS47Takeaway::Begin(const S47J::FObj& Saved)
{
	Stop(); bEnabled = true; Mode = EMode::None; Spot.Empty();
	const FString Value = S47J::Str(Saved, TEXT("mode"));
	if (Value == TEXT("held")) Mode = EMode::Held;
	else if (Value == TEXT("placed")) Mode = EMode::Placed;
	else if (Value == TEXT("eaten")) Mode = EMode::Eaten;
	else if (Value == TEXT("stowed")) Mode = EMode::Stowed;
	else if (Value == TEXT("discarded")) Mode = EMode::Discarded;
	Spot = S47J::Str(Saved, TEXT("spotId"));
	Build();
	if (!Spot.IsEmpty() && !Find(Spot)) Spot.Empty();
	if (Mode == EMode::Placed && Spot.IsEmpty()) Mode = EMode::Held;
	// The pack is authoritative: no paid meal or no handover cannot create a new serving.
	SyncPack(); UpdateSpots(); Present();
}

void FS47Takeaway::Stop()
{
	bEnabled = false;
	if (bOwnFade && M && M->UI) M->UI->Fade(false);
	bOwnFade = false; MealLeft = 0;
	MenuOptions.Reset();
	for (FS47Group* G : {Held.Get(), Placed.Get(), Trace.Get()}) if (G) G->SetVisible(false);
	for (FSpot& S : Spots) for (FS47Obj O : {S.Place, S.Pickup, S.Eat, S.RoadPlace, S.RoadPickup, S.RoadEat}) O.SetVisible(false);
	for (const auto& C : Targets) if (C.IsValid()) { C->SetVisibility(false); C->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
}

void FS47Takeaway::SyncPack()
{
	if (!Pack.Flag(TEXT("food_bought")) || !Pack.Flag(TEXT("food_taken"))) { Mode = EMode::None; Spot.Empty(); return; }
	if (Pack.Flag(TEXT("food_eaten"))) { Mode = EMode::Eaten; return; }
	if (Mode == EMode::None || Mode == EMode::Eaten)
	{
		Mode = EMode::Held; Spot.Empty();
		if (M && M->UI) M->UI->Toast(TEXT("Takeaway packed. Set it down on a clear surface; supper can wait until SARO."), 6);
	}
}

void FS47Takeaway::Build()
{
	if (bBuilt || !M || !M->Web || !M->Inter || !M->Web->IsLoaded()) return;
	bBuilt = true;
	const TWeakPtr<uint8> Life=Lifetime;
	M->OnAction(TEXT("alt"),[this,Life] { return Life.IsValid() && OpenFoodMenu(); });
	for (const TCHAR* Id : {TEXT("diner"), TEXT("desk"), TEXT("annex"), TEXT("hood"), TEXT("passenger"), TEXT("bench")})
	{
		FSpot S; S.Id = Id; Spots.Add(S);
	}
	AS47Web* W = M->Web;
	const FS47Obj Saro = W->Obj(TEXT("room.group"));
	const FS47Obj Counter = W->Obj(TEXT("world.diner.proxies.counter"));
	const FS47Obj Diner = Counter.Parent();
	// The archive table's old use box reached 27 cm above the actual tabletop,
	// enclosing a bag/napkin placed there. Interaction::Pick stops at that box
	// even while its chapter label is empty. Keep the original footprint, ID,
	// label and records action, but align its query surface with the authored top.
	FS47Obj WorkTable = W->Obj(TEXT("annex.objs.workTable"));
	if (WorkTable)
	{
		FVector Scale = WorkTable.Scale(); Scale.Y *= .01 / .3;
		WorkTable.SetScale(Scale); WorkTable.SetY(.775);
	}
	auto Target = [&](const FS47Obj& Parent, const FString& Name, const FVector& Size, uint32 Color)
	{
		const auto* N = W->Node(Parent.I); if (!N || !N->Comp.IsValid()) return FS47Obj();
		auto* C = NewObject<UStaticMeshComponent>(W);
		C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		C->SetupAttachment(N->Comp.Get()); C->SetMobility(EComponentMobility::Movable);
		C->SetRelativeScale3D(S47Three::DirToUE(Size)); C->SetCastShadow(false);
		C->SetCollisionResponseToAllChannels(ECR_Ignore); C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetVisibility(false);
		if (auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque")))
		{
			auto* Mat = UMaterialInstanceDynamic::Create(Base, W); Mat->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(Color)); C->SetMaterial(0, Mat);
		}
		C->RegisterComponent(); Targets.Add(C);
		FS47Obj O = W->ObjAt(W->AdoptNode(C, Parent.I, Name));
		// AdoptNode initially keeps the component transform. Store this scale in the node too,
		// so moving its small target later cannot reset it to a one-metre cube.
		O.SetScale(Size); return O;
	};
	for (FSpot& S : Spots)
	{
		const bool bDiner = S.Id == TEXT("diner") || S.Id == TEXT("bench");
		const FS47Obj Parent = bDiner ? Diner : Saro;
		S.Place = Target(Parent, TEXT("TakeawayPlace_") + S.Id, FVector(.24, .018, .18), 0xc8b18c);
		S.Pickup = Target(Parent, TEXT("TakeawayPickup_") + S.Id, FVector(.25, .32, .18), 0xb89a70);
		S.Eat = Target(Parent, TEXT("TakeawayEat_") + S.Id, FVector(.11, .018, .12), 0xe1d8bf);
		if (S.Id == TEXT("hood") || S.Id == TEXT("passenger"))
		{
			S.RoadPlace = Target(Diner, TEXT("TakeawayRoadPlace_") + S.Id, FVector(.24, .018, .18), 0xc8b18c);
			S.RoadPickup = Target(Diner, TEXT("TakeawayRoadPickup_") + S.Id, FVector(.25, .32, .18), 0xb89a70);
			S.RoadEat = Target(Diner, TEXT("TakeawayRoadEat_") + S.Id, FVector(.11, .018, .12), 0xe1d8bf);
		}
		for (const FString& Action : {FString(TEXT("place")), FString(TEXT("pickup")), FString(TEXT("eat"))})
		{
			const FString Id = TEXT("takeaway:") + Action + TEXT(":") + S.Id;
			const FString SpotId = S.Id;
			FS47Obj Object = Action == TEXT("place") ? S.Place : Action == TEXT("pickup") ? S.Pickup : S.Eat;
			M->Inter->Add({Id, Object, [this, Action, SpotId]
			{
				const FSpot* At = Find(SpotId); if (!bEnabled || !At || !Here(*At) || !Free()) return FString();
				if (Action == TEXT("place")) return Carried() ? FString(TEXT("Set down takeaway")) : FString();
				if (Action == TEXT("pickup")) return PlacedSpot() == SpotId ? FString(TEXT("Pick up the takeaway bag")) : FString();
				return (SpotId == TEXT("desk") || SpotId == TEXT("annex")) && (Carried() || PlacedSpot() == SpotId) && CanEat() ? FString(TEXT("Eat supper (12 minutes)")) : FString();
			}, [this, Id] { Use(Id); }, 2.4});
			const FS47Obj RoadObject = Action == TEXT("place") ? S.RoadPlace : Action == TEXT("pickup") ? S.RoadPickup : S.RoadEat;
			if (RoadObject) W->MarkUsable(RoadObject.I, Id);
		}
	}
	Held = MakeUnique<FS47Group>(W, TEXT("TakeawayInHand")); Food(*Held, false); Held->Root().Scale=FVector(.75);
	Placed = MakeUnique<FS47Group>(W, TEXT("TakeawayOnSurface")); Food(*Placed, false);
	Trace = MakeUnique<FS47Group>(W, TEXT("TakeawayAfterSupper")); Food(*Trace, true);
}

void FS47Takeaway::UpdateSpots()
{
	if (!M || !M->Web || !M->World) return;
	for (FSpot& S : Spots)
	{
		S.Area = TEXT("saro");
		if (S.Id == TEXT("desk")) S.Position = FVector(2.05, .785, 2.19);
		else if (S.Id == TEXT("annex")) S.Position = FVector(-3.70, .78, 8.30);
		else if (S.Id == TEXT("diner"))
		{
			S.Area = TEXT("diner"); S.Position = M->Web->Obj(TEXT("world.diner.proxies.counter")).WorldPosition() + FVector(0, .13, 1.65);
		}
		else if (S.Id == TEXT("bench"))
		{
			S.Area = TEXT("diner");
			const FS47Obj Diner = M->Web->Obj(TEXT("world.diner.proxies.counter")).Parent();
			S.Position = Diner.LocalToWorld(FVector(2.43, .46, 5));
		}
		else
		{
			const FVector Local = S.Id == TEXT("hood") ? FVector(-.22, .95, -1.58) : FVector(.38, .47, .10);
			if (M->World->bCommuteVehicle)
			{
				FS47Truck* Car = M->World->CabTruck(); if (!Car) Car = M->World->TruckHere();
				S.Area = M->World->Area;
				if (Car) S.Position = Car->LocalToWorld(Local);
			}
			else if (M->Evening)
				S.Position = M->Evening->PersonalCarPosition() + S47Three::Euler(0, M->Evening->PersonalCarHeading(), 0).RotateVector(Local);
		}
	}
}

bool FS47Takeaway::Free() const
{
	return bEnabled && M && M->Mode == US47Main::EMode::Play && M->World && !M->World->bDriving && !M->World->bBusy
		&& !M->bPausedByMenu && !M->ModalOpen() && !MealBusy() && M->Story && !M->Story->IsPaused() && !M->Story->IsCinematic()
		&& !(M->FieldCamera && M->FieldCamera->bRaised);
}
bool FS47Takeaway::Here(const FSpot& S) const
{
	if (!M || !M->World) return false;
	return S.Area == M->World->Area || (S.Area == TEXT("diner") && M->World->Area == TEXT("road"));
}

bool FS47Takeaway::CanEat() const
{
	if (!Free() || !M->Story || !M->World || M->World->Area != TEXT("saro") || !Pack.Flag(TEXT("handover_done"))
		|| !Pack.Flag(TEXT("food_bought")) || !Pack.Flag(TEXT("food_taken")) || Pack.Flag(TEXT("food_eaten"))
		|| !(Carried() || Mode == EMode::Placed) || M->OneAtATime() || (M->Audio && M->Audio->PhonePlaying())
		|| M->Story->ChapterKey == TEXT("ch6")) return false;
	if (Mode == EMode::Placed && Spot != TEXT("desk") && Spot != TEXT("annex")) return false;
	if (const auto* Scene = Pack.CurrentScene(); Scene && (Scene->Channel == TEXT("phone") || Scene->Channel == TEXT("walkie"))) return false;
	const ES47Phase P = M->Story->Phase();
	if (P != ES47Phase::Shift && P != ES47Phase::Survey && P != ES47Phase::Ch1) return false;
	const double Before = M->Story->Clock();
	return S47Timeline::AdvanceStoryClock(Before, 12 * 60, int32(P) < int32(ES47Phase::Residual)) - Before >= 12 * 60 - .001;
}

void FS47Takeaway::Present()
{
	if (!bBuilt || !M || !M->World || !M->Player) return;
	const bool bPlay = bEnabled && M->Mode == US47Main::EMode::Play;
	const bool bTools = M->ModalOpen() || M->bPausedByMenu || (M->FieldCamera && M->FieldCamera->bRaised)
		|| (M->Night && M->Night->Flashlight && M->Night->Flashlight->bOn);
	const bool bHand = bPlay && Carried() && !bTools && !M->World->bDriving && !M->Story->IsCinematic() && !MealBusy();
	const FS47Cam Cam = FS47Cam::Of(M->Player);
	Pose(*Held, Cam.Pos + Cam.Forward * .60 + Cam.Right * .30 - Cam.Up * .62, Cam.Quaternion());
	Held->SetVisible(bHand);
	const FSpot* At = Find(Spot);
	const bool bPlacedHere = bPlay && At && Here(*At);
	if (At) { Pose(*Placed, At->Position); Pose(*Trace, At->Position); }
	Placed->SetVisible(bPlacedHere && Mode == EMode::Placed);
	Trace->SetVisible(bPlacedHere && Mode == EMode::Eaten && !MealBusy());
	const bool bFree = Free();
	for (FSpot& S : Spots)
	{
		const bool bHere = bPlay && Here(S);
		const bool bPlace = bHere && bFree && Carried();
		const bool bPickup = bHere && bFree && PlacedSpot() == S.Id;
		const bool bEat = bHere && (S.Id == TEXT("desk") || S.Id == TEXT("annex")) && (Carried() || PlacedSpot() == S.Id) && CanEat();
		const bool bRoad = (S.Id == TEXT("hood") || S.Id == TEXT("passenger")) && M->World->bCommuteVehicle;
		const FVector EatAt = S.Position + FVector(.38, .012, 0);
		TargetState(S.Place, bPlace && !bRoad, true, S.Position + FVector(0, .006, 0));
		TargetState(S.Pickup, bPickup && !bRoad, false, S.Position + FVector(0, .16, 0));
		TargetState(S.Eat, bEat && !bRoad, true, EatAt);
		TargetState(S.RoadPlace, bPlace && bRoad, true, S.Position + FVector(0, .006, 0));
		TargetState(S.RoadPickup, bPickup && bRoad, false, S.Position + FVector(0, .16, 0));
		TargetState(S.RoadEat, bEat && bRoad, true, EatAt);
		for (const FString& Action : {FString(TEXT("place")), FString(TEXT("pickup")), FString(TEXT("eat"))})
			if (auto* I = M->Inter->Get(TEXT("takeaway:") + Action + TEXT(":") + S.Id))
				I->Object = Action == TEXT("place") ? (bRoad ? S.RoadPlace : S.Place) : Action == TEXT("pickup") ? (bRoad ? S.RoadPickup : S.Pickup) : (bRoad ? S.RoadEat : S.Eat);
	}
}

void FS47Takeaway::Step(double Dt)
{
	if (!bEnabled) return;
	Build(); SyncPack(); UpdateSpots();
	if (MealBusy() && M && !M->bPausedByMenu && !M->ModalOpen() && !(M->Story && M->Story->IsPaused()))
	{
		MealLeft = FMath::Max(0., MealLeft - Dt);
		if (MealLeft == 0)
		{
			if (bOwnFade && M->UI) { M->UI->Fade(false); M->UI->Toast(TEXT("Supper finished. The wrapper and empty coffee cup are left on the surface."), 5); }
			bOwnFade = false;
		}
	}
	Present();
	if (Free() && (Carried() || Stowed()) && M->UI)
		M->UI->Prompt(M->Inter && M->Player ? M->Inter->LabelOf(M->Player->FocusedId()) : FString(),TEXT("Manage takeaway"));
}

FString FS47Takeaway::NearbySurface() const
{
	if(!M || !M->Player || !M->Web || !M->Web->GetWorld() || !Carried()) return {};
	double Best=2.4*2.4; FString Id;
	for(const FSpot& S:Spots) if(Here(S)) {
		const double D=FVector::DistSquared(S.Position,S47Three::ToThree(M->Player->EyeUE()));
		if(D>=Best) continue;
		const FVector Eye=M->Player->EyeUE(), Target=S47Three::ToUE(S.Position+FVector(0,.006,0));
		// A menu may ease aiming, but cannot place supper through a wall or a closed door.
		// Check physical scenery rather than blank/inert use boxes enclosing a tabletop.
		FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(S47FoodSurface),true,M->Player);
		if(M->Web->GetWorld()->LineTraceSingleByChannel(Hit,Eye,Target,ECC_Visibility,Query) && Hit.Distance<FVector::Dist(Eye,Target)-2) continue;
		Best=D; Id=S.Id;
	}
	return Id;
}

bool FS47Takeaway::OpenFoodMenu()
{
	if(!Free() || !(Carried() || Stowed()) || !M->UI) return false;
	UpdateSpots(); MenuOptions.Reset();
	TArray<TPair<FString,TFunction<void()>>> Actions;
	const TWeakPtr<uint8> Life=Lifetime;
	auto Add=[&](const FString& Id,const FString& Label) {
		MenuOptions.Add(Id);
		Actions.Add({Label,[this,Life,Id] { if(!Life.IsValid() || !M || !M->UI) return; M->UI->Close(true); Manage(Id); }});
	};
	if(Carried()) {
		const FString Near=NearbySurface();
		if(!Near.IsEmpty()) Add(TEXT("place:")+Near,TEXT("Set down on ")+ (Near==TEXT("desk")?FString(TEXT("the control-room desk")):Near==TEXT("annex")?FString(TEXT("the archive table")):Near==TEXT("bench")?FString(TEXT("the diner bench")):Near==TEXT("diner")?FString(TEXT("the diner counter")):Near==TEXT("hood")?FString(TEXT("the car hood")):FString(TEXT("the passenger seat"))));
		Add(TEXT("stow"),TEXT("Stow takeaway for later"));
	}
	else Add(TEXT("takeout"),TEXT("Take out takeaway"));
	Add(TEXT("discard"),TEXT("Discard takeaway"));
	Add(TEXT("cancel"),TEXT("Keep it as it is"));
	M->UI->Open(SNew(STakeawayMenu,M->UI.Get(),MoveTemp(Actions)));
	Present(); return true;
}

bool FS47Takeaway::Manage(const FString& Action)
{
	if(!Free()) return false;
	if(Action.StartsWith(TEXT("place:"))) { const FString Id=Action.Mid(6); return NearbySurface()==Id && PlaceAt(Id); }
	if(Action==TEXT("stow") && Carried()) { Mode=EMode::Stowed; Spot.Empty(); }
	else if(Action==TEXT("takeout") && Stowed()) { Mode=EMode::Held; Spot.Empty(); }
	else if(Action==TEXT("discard") && (Carried() || Stowed())) { Mode=EMode::Discarded; Spot.Empty(); }
	else return false;
	Changed();
	if(M->UI) M->UI->Toast(Mode==EMode::Stowed?TEXT("Takeaway stowed. Your hands are free; use Manage takeaway to take it out again."):Mode==EMode::Discarded?TEXT("Takeaway discarded. Your hands are free."):TEXT("Takeaway and coffee taken out."),4);
	return true;
}

bool FS47Takeaway::PlaceAt(const FString& Id)
{
	const FSpot* At = Find(Id); if (!At || !Here(*At) || !Free() || !Carried()) return false;
	Mode = EMode::Placed; Spot = Id; Changed();
	if (M->UI) M->UI->Toast(TEXT("Takeaway and coffee set down. Your hands are free."), 3);
	return true;
}
bool FS47Takeaway::PickupAt(const FString& Id)
{
	const FSpot* At = Find(Id); if (!At || !Here(*At) || !Free() || PlacedSpot() != Id) return false;
	Mode = EMode::Held; Spot.Empty(); Changed();
	if (M->UI) M->UI->Toast(TEXT("Takeaway and coffee picked up."), 3);
	return true;
}
bool FS47Takeaway::Eat(const FString& Id)
{
	if (!CanEat()) return false;
	FString Chosen = Id;
	if (Chosen.IsEmpty())
	{
		if (Mode == EMode::Placed) Chosen = Spot;
		else
		{
			double Best = TNumericLimits<double>::Max();
			for (const FSpot& S : Spots) if (S.Id == TEXT("desk") || S.Id == TEXT("annex"))
			{
				const double D = FVector::DistSquared(S.Position, M->Player->Pos);
				if (D < Best) { Best = D; Chosen = S.Id; }
			}
		}
	}
	const FSpot* At = Find(Chosen);
	if (!At || (Chosen != TEXT("desk") && Chosen != TEXT("annex")) || !Here(*At) || (Mode == EMode::Placed && Spot != Chosen)) return false;
	const double Before = M->Story->Clock();
	const double After = S47Timeline::AdvanceStoryClock(Before, 12 * 60, int32(M->Story->Phase()) < int32(ES47Phase::Residual));
	if (!Pack.EatTakeaway()) return false;
	// Commit state and clock synchronously, before requesting a save or starting transient presentation.
	Mode = EMode::Eaten; Spot = Chosen; M->Story->SetClock(After);
	MealLeft = 4; bOwnFade = true;
	if (M->UI) M->UI->Fade(true, TEXT("SUPPER / TWELVE QUIET MINUTES"));
	Changed();
	UE_LOG(LogTemp, Display, TEXT("S47: takeaway meal committed once at %s: clock %.3f -> %.3f; transient fade 4 s, save/load retains finished meal"), *Spot, Before, After);
	return true;
}
bool FS47Takeaway::Use(const FString& Id)
{
	if (!Id.StartsWith(TEXT("takeaway:"))) return false;
	UpdateSpots();
	FString Action, Name; Id.Mid(9).Split(TEXT(":"), &Action, &Name);
	if (Action == TEXT("place")) PlaceAt(Name);
	else if (Action == TEXT("pickup")) PickupAt(Name);
	else if (Action == TEXT("eat")) Eat(Name == TEXT("held") ? FString() : Name);
	Present(); return true;
}

void FS47Takeaway::Changed() { Present(); if (M) M->RequestAutosave(false); }
FS47Takeaway::FSpot* FS47Takeaway::Find(const FString& Id) { return Spots.FindByPredicate([&](const FSpot& S) { return S.Id == Id; }); }
const FS47Takeaway::FSpot* FS47Takeaway::Find(const FString& Id) const { return Spots.FindByPredicate([&](const FSpot& S) { return S.Id == Id; }); }
bool FS47Takeaway::Carried() const { return Pack.Flag(TEXT("food_bought")) && Pack.Flag(TEXT("food_taken")) && !Pack.Flag(TEXT("food_eaten")) && (Mode == EMode::Held || Mode == EMode::None); }
FString FS47Takeaway::PlacedSpot() const { return !Eaten() && Mode == EMode::Placed ? Spot : FString(); }
bool FS47Takeaway::Eaten() const { return Pack.Flag(TEXT("food_taken")) && Pack.Flag(TEXT("food_eaten")); }
bool FS47Takeaway::Stowed() const { return bEnabled && Mode==EMode::Stowed && Pack.Flag(TEXT("food_taken")) && !Pack.Flag(TEXT("food_eaten")); }
bool FS47Takeaway::Discarded() const { return bEnabled && Mode==EMode::Discarded && Pack.Flag(TEXT("food_taken")) && !Pack.Flag(TEXT("food_eaten")); }
bool FS47Takeaway::HeldVisible() const { return Held && Held->Visible(); }
bool FS47Takeaway::PlacedVisible() const { return Placed && Placed->Visible(); }
bool FS47Takeaway::TraceVisible() const { return Trace && Trace->Visible(); }
TArray<FString> FS47Takeaway::SpotIds() const { TArray<FString> Out; for (const FSpot& S : Spots) Out.Add(S.Id); return Out; }
FVector FS47Takeaway::SpotPosition(const FString& Id) const { const auto* S = Find(Id); return S ? S->Position : FVector::ZeroVector; }
FString FS47Takeaway::SpotArea(const FString& Id) const { const auto* S = Find(Id); return S ? S->Area : FString(); }
FString FS47Takeaway::Location() const { return Eaten() ? TEXT("eaten") : Carried() ? TEXT("held") : Stowed()?TEXT("stowed"):Discarded()?TEXT("discarded"):Mode == EMode::Placed ? Spot : TEXT("none"); }
S47J::FObj FS47Takeaway::Save() const
{
	auto O = S47J::New(); S47J::Set(O, TEXT("version"), 2);
	// A dialogue callback can flip food_taken before our next frame. Serialize the effective
	// state immediately so its purchase/reply autosave already contains the packed meal.
	S47J::Set(O, TEXT("mode"), Eaten() ? TEXT("eaten") : Carried() ? TEXT("held") : Stowed()?TEXT("stowed"):Discarded()?TEXT("discarded"):Mode == EMode::Placed ? TEXT("placed") : TEXT("none"));
	S47J::Set(O, TEXT("spotId"), Spot); return O;
}

bool FS47Takeaway::SelfTest(TArray<FString>& Lines)
{
	bool Ok = true;
	auto Check = [&](bool Good, const TCHAR* Text) { Ok &= Good; Lines.Add(FString(Good ? TEXT("PASS takeaway: ") : TEXT("FAIL takeaway: ")) + Text); };
	FS47EveningPack P; FString Error; P.Initialize(Error);
	FS47Takeaway T(nullptr, P); T.Begin();
	Check(!T.Carried() && !T.Eaten(), TEXT("unpaid meal is never manufactured"));
	auto State = P.Save(); auto Flags = S47J::Obj(State, TEXT("flags"));
	S47J::Set(Flags, TEXT("food_bought"), true); S47J::Set(Flags, TEXT("food_taken"), true); P.Load(State, Error);
	T.Begin(); Check(T.Carried(), TEXT("old paid takeaway without placement state migrates to held"));
	auto Saved = T.Save(); FS47Takeaway U(nullptr, P); U.Begin(Saved);
	Check(U.Carried() && !U.MealBusy(), TEXT("held save roundtrip has no transient meal lock"));
	S47J::Set(Saved, TEXT("mode"), TEXT("placed")); S47J::Set(Saved, TEXT("spotId"), TEXT("missing")); U.Begin(Saved);
	Check(U.Carried(), TEXT("unknown old placement safely restores a carried meal"));
	for(const TCHAR* Value:{TEXT("stowed"),TEXT("discarded")}) {
		auto StateOnly=S47J::New(); S47J::Set(StateOnly,TEXT("mode"),Value);
		U.Begin(StateOnly); const auto Restored=U.Save(); U.Begin(Restored);
		Check(!U.Carried() && !U.Eaten() && (U.Stowed() || U.Discarded()) && U.Location()==Value && !P.Flag(TEXT("food_eaten")),
			TEXT("stowed/discarded paid serving roundtrips without respawn, consumption or a meal fade"));
		U.Step(.1);
		Check(!U.Carried() && U.Location()==Value,TEXT("pack synchronization preserves explicit stowed/discarded state"));
	}
	Check(P.EatTakeaway(), TEXT("pack atomically consumes the one paid serving"));
	T.Begin(Saved); Check(T.Eaten() && !T.Carried() && !T.MealBusy(), TEXT("restored consumption cannot replay a meal or keep a fade lock"));
	Check(!P.EatTakeaway(), TEXT("second meal commit is rejected"));
	const double C = S47Timeline::Residual(true) - 100;
	Check(S47Timeline::AdvanceStoryClock(C, 720, true) >= C && S47Timeline::AdvanceStoryClock(C, 720, true) - C < 720,
		TEXT("residual cap has insufficient budget for a promised twelve-minute meal, without rewind"));
	return Ok;
}

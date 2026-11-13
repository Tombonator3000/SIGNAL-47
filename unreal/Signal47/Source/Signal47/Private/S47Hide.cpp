#include "S47Hide.h"
#include "S47Freeform.h"
#include "S47DoorLoop.h"
#include "S47Inter.h"
#include "S47Keys.h"
#include "S47Main.h"
#include "S47Sneak.h"
#include "S47Stress.h"
#include "S47Taken.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "UObject/GarbageCollection.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Hide, Log, All);

namespace S47HideLocal
{
	// Three-space centres under the tops (x, z) and half the room under them. The control-room desk is the one the takeaway is set
	// down on (S47Takeaway: 2.05, 2.19); the archive table is the annex work table (-3.70, 8.30). Sizes are a plain desk's and are
	// not yet measured in the engine: the query box is the knee space, a little proud of the top's footprint on each side.
	struct FSpot { const TCHAR* Id; const TCHAR* Area; double X, Z, HalfX, HalfZ; const TCHAR* Label; const TCHAR* Look; };
	const FSpot Spots[] = {
		{ TEXT("ff:hide:desk"), TEXT("saro"), 2.05, 2.19, 0.60, 0.40, TEXT("Hide under the desk"),
			TEXT("Dark under the desk, the space where your knees go. Enough room to sit with your back to the panel and be still.") },
		{ TEXT("ff:hide:annex"), TEXT("saro"), -3.70, 8.30, 0.80, 0.45, TEXT("Hide under the table"),
			TEXT("The archive table has a shelf of dust and a crate under it, and room beside the crate for a person who does not want to be seen.") },
	};
	constexpr int32 SpotNum = UE_ARRAY_COUNT(Spots);
}

FS47Hide::~FS47Hide() {}

int32 FS47Hide::SpotCount() { return S47HideLocal::SpotNum; }

double FS47Hide::Frame() const
{
	return (M && M->Player) ? LidOf(M->Player->HideK) : 0.0;
}

bool FS47Hide::MayHide(bool bFreeformOn, bool bHeld, bool bDriving, bool bDoorLoop, bool bTaken, bool bModal)
{
	return bFreeformOn && !bHeld && !bDriving && !bDoorLoop && !bTaken && !bModal;
}

bool FS47Hide::InsideSpot(int32 Spot, double X, double Z, FString& OutArea)
{
	if (Spot < 0 || Spot >= S47HideLocal::SpotNum) return false;
	const S47HideLocal::FSpot& S = S47HideLocal::Spots[Spot];
	OutArea = S.Area;
	return FMath::Abs(X - S.X) <= S.HalfX + 0.6 && FMath::Abs(Z - S.Z) <= S.HalfZ + 0.6;
}

bool FS47Hide::Live() const { return M && M->Player && M->Web && M->World && M->Inter && M->IsFreeform() && M->Freeform; }

FString FS47Hide::WhyNot() const
{
	if (!Live()) return TEXT("no Freeform night");
	FS47FreeformSession& F = *M->Freeform;
	if (!F.Active()) return TEXT("session not active");
	if (F.Held()) return TEXT("the session is held");
	if (M->World->bDriving) return TEXT("in the truck");
	if (F.DoorLoop && F.DoorLoop->Active()) return TEXT("the door loop runs");
	if (F.Taken && F.Taken->Busy()) return TEXT("being taken");
	if (M->ModalOpen()) return TEXT("a menu or paper is open");
	return FString();
}

void FS47Hide::Build()
{
	if (bBuilt || !Live() || !M->Web->IsLoaded()) return;
	AS47Web* W = M->Web;
	const FS47Obj Saro = W->Obj(TEXT("room.group"));
	const auto* Parent = W->Node(Saro.I);
	if (!Parent || !Parent->Comp.IsValid()) return;
	bBuilt = true;
	const TWeakPtr<uint8> Guard = Life;
	for (int32 I = 0; I < S47HideLocal::SpotNum; ++I)
	{
		const S47HideLocal::FSpot& S = S47HideLocal::Spots[I];
		// an invisible box over the space under the top, for the line of sight to pick (as the takeaway's targets are)
		const FVector Size(S.HalfX * 2, 0.55, S.HalfZ * 2);
		auto* C = NewObject<UStaticMeshComponent>(W);
		C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		C->SetupAttachment(Parent->Comp.Get()); C->SetMobility(EComponentMobility::Movable);
		C->SetRelativeScale3D(S47Three::DirToUE(Size)); C->SetCastShadow(false);
		C->SetCollisionResponseToAllChannels(ECR_Ignore); C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		// Hidden in game as well as not visible: the web shows a node's component again whenever its area comes back into view
		// (AS47Web::ApplyVisibility), and the box then stood under the desk in the engine's grey checker (Tom, d67).
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetVisibility(false); C->SetHiddenInGame(true);
		C->RegisterComponent(); Targets.Add(C);
		FS47Obj O = W->ObjAt(W->AdoptNode(C, Saro.I, FString(S.Id)));
		O.SetScale(Size);
		O.SetPosition(Saro.WorldToLocal(FVector(S.X, 0.30, S.Z)));
		FS47Interactable It;
		It.Id = S.Id; It.Object = O; It.Range = Reach;
		It.Label = [this, Guard, I]() -> FString
		{
			if (!Guard.IsValid() || bIn || !WhyNot().IsEmpty() || M->World->Area != S47HideLocal::Spots[I].Area) return FString();
			return S47HideLocal::Spots[I].Label;
		};
		It.Use = [this, Guard, I] { if (Guard.IsValid() && !bIn && WhyNot().IsEmpty()) Enter(I); };
		It.Examine = [Guard, I]() -> FString { return Guard.IsValid() ? FString(S47HideLocal::Spots[I].Look) : FString(); };
		M->Inter->Add(It);
	}
	// nothing else is picked or used while the player is under there; E comes out (polled in Step)
	M->Inter->Holds.Add([this, Guard] { return Guard.IsValid() && bIn; });
	UE_LOG(LogS47Hide, Display, TEXT("S47: hide: %d places under tables"), S47HideLocal::SpotNum);
}

void FS47Hide::Enter(int32 Spot)
{
	if (!Live() || Spot < 0 || Spot >= S47HideLocal::SpotNum) return;
	const S47HideLocal::FSpot& S = S47HideLocal::Spots[Spot];
	AS47Walker* P = M->Player;
	Back = FVector2D(P->Pos.X, P->Pos.Z); BackYaw = P->Yaw;
	// in under the top and facing out, the way the player was looking (they crept in backwards, as it were)
	P->Pos.X = S.X; P->Pos.Z = S.Z; P->MoveFrom = P->Pos;
	P->SetCrouch(true); P->bHideLock = true;
	At = Spot; bIn = true; Age = 0;
	if (M->Stress) M->Stress->Startle(Strain);
	if (M->UI) M->UI->Toast(TEXT("Under the desk. Be still. Press E to come out."), 4.f);
	UE_LOG(LogS47Hide, Display, TEXT("S47: hide: in under %s"), S.Id);
}

void FS47Hide::Leave(const TCHAR* Why)
{
	if (!bIn) return;
	bIn = false;
	if (Live())
	{
		AS47Walker* P = M->Player;
		P->bHideLock = false;
		// back where they went in from, if that is still clear, else stay on the spot they are on
		if (P->Walkable(Back.X, Back.Y)) { P->Pos.X = Back.X; P->Pos.Z = Back.Y; P->MoveFrom = P->Pos; }
		P->SetCrouch(false);
	}
	At = -1;
	UE_LOG(LogS47Hide, Display, TEXT("S47: hide: out (%s)"), Why);
}

void FS47Hide::Begin()
{
	bIn = false; At = -1; Age = 0; Tries = 0;
	// a new world: the old targets went with it
	if (bBuilt && Targets.ContainsByPredicate([](const TWeakObjectPtr<UStaticMeshComponent>& T) { return !T.IsValid(); })) { Targets.Reset(); bBuilt = false; }
}

void FS47Hide::Stop()
{
	Leave(TEXT("stop"));
	if (M && M->Player) { M->Player->bHideLock = false; }
}

void FS47Hide::Step(double Dt)
{
	if (!Live()) { if (bIn) Leave(TEXT("no session")); return; }
	if (!bBuilt)
	{
		Tries += Dt;
		if (Tries > 1.0) { Tries = 0; Build(); }
		return;
	}
	if (!bIn) return;
	Age += Dt;
	const FString Why = WhyNot();
	if (!Why.IsEmpty() || M->World->Area != S47HideLocal::Spots[FMath::Clamp(At, 0, S47HideLocal::SpotNum - 1)].Area) { Leave(*Why); return; }
	// the breath: held a little heavy for as long as they are in there
	if (M->Stress) M->Stress->Startle(Strain);
	if (Age < LeaveLock) return;
	const APlayerController* PC = M->Web->GetWorld() ? M->Web->GetWorld()->GetFirstPlayerController() : nullptr;
	if (PC && (S47Keys::Pressed(PC, TEXT("use")) || S47Keys::Pressed(PC, TEXT("jump")))) Leave(TEXT("E"));
}

bool FS47Hide::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s hide: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), What)); bOk &= bCond; };
	Check(MayHide(true, false, false, false, false, false) && !MayHide(false, false, false, false, false, false) && !MayHide(true, true, false, false, false, false)
		&& !MayHide(true, false, true, false, false, false) && !MayHide(true, false, false, true, false, false) && !MayHide(true, false, false, false, true, false)
		&& !MayHide(true, false, false, false, false, true),
		TEXT("allowed in a Freeform night only: never held, driving, in the door loop or its chase, being taken or in a menu"));
	using namespace S47Sneak;
	Check(FMath::IsNearlyEqual(SightReach(2), HiddenSight, 1e-6) && SightReach(1) > 12.0 && SightReach(1.5) < SightReach(1) && SightReach(1.5) > SightReach(2) && HiddenSight >= 2.0 && HiddenSight <= 4.0,
		TEXT("hidden, seen from three metres: not invisible, and the way in is a slope from crouching"));
	Check(StepReach(1.9, 2) <= HiddenSteps + 1e-9 && StepReach(1.9, 2) > 0.5 && StepReach(4.0, 2) <= HiddenSteps + 1e-9 && StepReach(0.2, 2) == 0.0 && StepReach(1.9, 1) > StepReach(1.9, 2),
		TEXT("feet heard from about a metre hidden, and still heard close by"));
	Check(FMath::IsNearlyEqual(HuntPace(2, false), HiddenHuntPace, 1e-6) && HuntPace(2, true) == 1.0 && HuntPace(1.5, false) < HuntPace(1, false) && HuntPace(1.5, false) > HuntPace(2, false),
		TEXT("a visit's interest grows slowly under a table, a lamp gives the player away"));
	Check(ChestHeight(2) == HiddenChest && ChestHeight(2) < ChestHeight(1) && ApproachPace(2, false) == ApproachPace(1, false) && TrackPace(2) == TrackPace(1),
		TEXT("the light looks near the floor; the approach and the light's pace are as when crouching"));
	Check(StillInRoom(2, false, 0) && !StillInRoom(2, true, 0) && HiddenOf(-1) == 0.0 && HiddenOf(0.5) == 0.0 && HiddenOf(9) == 1.0,
		TEXT("a hidden player counts as still in a room, and a lamp ends it"));
	Check(LidOf(0) == 0.0 && LidOf(1) == LidShare && LidOf(2) == LidShare && LidOf(0.5) < LidShare && LidShare > 0.2 && LidShare < 0.6 && HideEyeDrop > 0.3 && HideEyeDrop < 0.6,
		TEXT("the table's edge comes down over the view with the body, and the eye drops further"));
	FString Area;
	Check(SpotCount() == 2 && InsideSpot(0, 2.05, 2.19, Area) && Area == TEXT("saro") && !InsideSpot(0, 8, 8, Area) && !InsideSpot(7, 0, 0, Area) && InsideSpot(1, -3.7, 8.3, Area),
		TEXT("two places, the control-room desk and the archive table, found by where they stand"));
	return bOk;
}

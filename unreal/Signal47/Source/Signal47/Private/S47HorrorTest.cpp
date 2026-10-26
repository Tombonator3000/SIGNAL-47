#include "S47HorrorTest.h"

#include "S47Evening.h"
#include "S47Album.h"
#include "S47Chapter1.h"
#include "S47FieldCamera.h"
#include "S47Flow.h"
#include "S47Horror.h"
#include "S47HorrorVisual.h"
#include "S47HUD.h"
#include "S47Inter.h"
#include "S47Image.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Kit.h"
#include "S47Saves.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47HorrorTest, Log, All);

struct FS47HorrorTest::FState
{
	bool bEnabled = false, bDone = false, bPendingNormalTest = false, bMissHand = false;
	int32 Stage = 0, Checks = 0, Stores = 0, WalkIndex = 0, AfterWalk = 0;
	double Elapsed = 0, Since = 0, Hold = 3, WalkMetres = 0, LastProgress = 0, PausedHandTime = 0;
	double LineAt = 0;
	FString Mode, Root, LastLine, NormalPhoto;
	TArray<FVector> Points;
	FVector LastFoot = FVector::ZeroVector, FirstHand = FVector::ZeroVector;
	S47J::FObj BeforeNormal, AfterNormal, AfterWindow, AfterReport, PendingNormal, FilmReserveBase;
	TArray<FS47AlbumShot> BudgetPhotos;
	TArray<TSharedPtr<FJsonValue>> Evidence;
};

FS47HorrorTest::FS47HorrorTest(US47Main* InM) : State(MakeUnique<FState>()), M(InM)
{
	State->bEnabled = FParse::Value(FCommandLine::Get(), TEXT("S47HorrorContract="), State->Mode);
	State->bMissHand = FParse::Param(FCommandLine::Get(), TEXT("S47HorrorMissHand"));
}
FS47HorrorTest::~FS47HorrorTest() = default;

void FS47HorrorTest::Fail(const FString& Why)
{
	if (State->bDone) return;
	State->bDone = true;
	if (M && M->Player) M->Player->TestMove.Reset();
	UE_LOG(LogS47HorrorTest, Error, TEXT("S47: Horror contract FAIL stage %d at %.2f s: %s"), State->Stage, State->Elapsed, *Why);
	if (!State->Root.IsEmpty())
	{
		auto Result = S47J::New(); S47J::Set(Result, TEXT("status"), TEXT("FAIL"));
		S47J::Set(Result, TEXT("reason"), Why); S47J::Set(Result, TEXT("stage"), State->Stage);
		S47J::Set(Result, TEXT("checks"), State->Checks); Result->SetArrayField(TEXT("evidence"), State->Evidence);
		FFileHelper::SaveStringToFile(S47J::Write(Result, true), *(State->Root / TEXT("result.json")));
	}
	FPlatformMisc::RequestExitWithStatus(false, 1);
}

bool FS47HorrorTest::Check(bool Good, const FString& Why)
{
	++State->Checks;
	if (!Good) { Fail(Why); return false; }
	auto Row = S47J::New(); S47J::Set(Row, TEXT("check"), Why); S47J::Set(Row, TEXT("stage"), State->Stage);
	S47J::Set(Row, TEXT("elapsed"), State->Elapsed); State->Evidence.Add(MakeShared<FJsonValueObject>(Row));
	UE_LOG(LogS47HorrorTest, Display, TEXT("S47: Horror contract PASS: %s"), *Why);
	return true;
}

void FS47HorrorTest::Stage(int32 Next, double Hold)
{
	State->Stage = Next; State->Since = State->Elapsed; State->Hold = Hold;
	UE_LOG(LogS47HorrorTest, Display, TEXT("S47: Horror contract stage %d at %.2f s"), Next, State->Elapsed);
}

FS47Obj FS47HorrorTest::Object(const FString& Id) const
{
	if (Id==TEXT("doorEast") && M->Web->IsDoorOpen(TEXT("doorEast"))) return M->Web->Obj(TEXT("room.doorHinge"));
	if (M->Horror && M->Horror->Visual)
		if (const auto Obj = M->Horror->Visual->Object(Id)) return Obj;
	if (const auto* Item = M->Inter->Get(Id)) return Item->Object;
	if (const auto* Item = M->Web->Item(Id)) return M->Web->ObjAt(Item->Node);
	return {};
}

FVector FS47HorrorTest::Target(const FString& Id) const
{
	const FS47Obj Obj = Object(Id); const FBox Bounds = Obj ? Obj.WorldBox() : FBox(ForceInit);
	return Bounds.IsValid ? Bounds.GetCenter() : Obj ? Obj.WorldPosition() : FVector::ZeroVector;
}

void FS47HorrorTest::Aim(const FVector& Point)
{
	const FVector Eye(M->Player->Pos.X, M->Player->FloorY + M->Player->Eye, M->Player->Pos.Z);
	const FVector D = Point - Eye;
	M->Player->Yaw = FMath::Atan2(-D.X, -D.Z);
	M->Player->Pitch = FMath::Atan2(D.Y, FVector2D(D.X, D.Z).Size());
}

bool FS47HorrorTest::Pick(const FString& Id, const FString& Alternate, bool bUse)
{
	if (!Object(Id)) { Fail(TEXT("missing physical target: ") + Id); return false; }
	const FVector Eye = M->Player->EyeUE(), Direction = (S47Three::ToUE(Target(Id)) - Eye).GetSafeNormal();
	const FString Actual = M->Inter->Pick(Eye, Direction);
	const double Dot = FVector::DotProduct(Direction, M->Player->ForwardUE());
	if ((Actual != Id && (Alternate.IsEmpty() || Actual != Alternate)) || Dot < .94 || (bUse && !M->Player->OnUse))
	{
		Fail(FString::Printf(TEXT("actual aimed ray for %s picked '%s', view dot %.3f"), *Id, *Actual, Dot));
		return false;
	}
	if (bUse) M->Player->OnUse(Actual);
	return Check(true, FString(bUse ? TEXT("actual ray and production use: ") : TEXT("actual camera ray: ")) + Actual);
}

void FS47HorrorTest::Path(const TArray<FVector>& Points)
{
	State->Points = Points; State->WalkIndex = 0; State->LastFoot = M->Player->Pos; State->LastProgress = State->Elapsed;
}

bool FS47HorrorTest::Walk()
{
	const FVector At = M->Player->Pos;
	const double Moved = FVector2D(At.X - State->LastFoot.X, At.Z - State->LastFoot.Z).Size();
	if (Moved > .5) { Fail(TEXT("waypoint route jumped beyond a native Walker step")); return false; }
	State->WalkMetres += Moved; State->LastFoot = At;
	if (Moved > .002) State->LastProgress = State->Elapsed;
	if (State->WalkIndex >= State->Points.Num()) { M->Player->TestMove.Reset(); return true; }
	const FVector D = State->Points[State->WalkIndex] - At;
	const double Distance = FVector2D(D.X, D.Z).Size();
	if (Distance < .09) { ++State->WalkIndex; M->Player->TestMove.Reset(); State->LastProgress = State->Elapsed; return false; }
	if (State->Elapsed - State->LastProgress > 4)
	{
		Fail(FString::Printf(TEXT("native Walker path blocked at %.3f %.3f toward %.3f %.3f"), At.X, At.Z, State->Points[State->WalkIndex].X, State->Points[State->WalkIndex].Z));
		return false;
	}
	M->Player->Yaw = FMath::Atan2(-D.X, -D.Z); M->Player->Pitch = -.06;
	M->Player->TestMove = FVector2D(0, -FMath::Min(1., Distance * 2));
	return false;
}

bool FS47HorrorTest::StoreRestore(const FString& Name, const S47J::FObj& Snapshot)
{
	FS47SaveStore Writer(State->Root / Name); Writer.Init(); FS47SaveCard Card;
	Card.Chapter = TEXT("SC01 live contract"); Card.Place = TEXT("SARO"); Card.Clock = M->Story->ClockText(true);
	FString Error;
	const auto Meta = Writer.Save(1, TEXT("manual"), 0, S47J::Clone(Snapshot), Card, Error);
	if (!Meta) { Fail(TEXT("isolated Writer failed: ") + Error); return false; }
	FS47SaveStore Reader(State->Root / Name); Reader.Init(); const auto Disk = Reader.Load(Meta->Id);
	if (!Check(Disk.IsValid(), TEXT("fresh isolated disk reader: ") + Name)) return false;
	FFileHelper::SaveStringToFile(S47J::Write(Disk, true), *(State->Root / (Name + TEXT(".loaded.json"))));
	++State->Stores;
	US47Flow::FStart Start; Start.State = Disk; Start.CaseId = 0; Start.bDev = true; Start.Playtime = M->Playtime;
	M->Flow->StartGame(Start);
	return Check(M->Flow->bDevRun && M->Flow->CaseId == 0, TEXT("full Flow restore retains isolated developer case: ") + Name);
}

void FS47HorrorTest::Shot(const FString& Name)
{
	FScreenshotRequest::RequestScreenshot(State->Root / (Name + TEXT(".png")), true, false);
}

void FS47HorrorTest::Step(double Dt)
{
	auto& T = *State;
	if (!T.bEnabled || T.bDone) return;
	if (!M || !M->Player || !M->Story || !M->World || !M->Web || !M->Flow || !M->UI || !M->Inter || !M->Evening || !M->Horror || !M->Horror->Visual || !M->Night || !M->Night->Album)
	{ Fail(TEXT("live SC01 runtime dependency missing")); return; }
	T.Elapsed += FMath::Clamp(Dt, 0., .2);
	if (T.Elapsed > 420) { Fail(TEXT("420 s SC01 control timeout")); return; }
	if (!M->Flow->bDevRun || M->Flow->CaseId != 0) { Fail(TEXT("developer case zero required; never write a player case")); return; }
	if (M->Flow->bStarting || M->World->bBusy) return;
	if (T.Hold > 0) { T.Hold -= Dt; return; }
	auto& H = *M->Horror; auto& V = *H.Visual;
	auto Go = [this, &T](const TArray<FVector>& Points, int32 Next)
	{ Path(Points); T.AfterWalk = Next; Stage(900, 0); };
	auto Enter = [this]()
	{
		if (!M->ModalOpen() || !FSlateApplication::IsInitialized()) { Fail(TEXT("actual focused modal/Slate unavailable for Enter")); return false; }
		const bool Handled = FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Enter, FModifierKeysState(), 0, false, 0, 0));
		FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(EKeys::Enter, FModifierKeysState(), 0, false, 0, 0));
		return Check(Handled, TEXT("actual focused Slate Enter activates the displayed primary button"));
	};
	auto Drain = [this, &T, &H, &Enter](int32 Next)
	{
		if (!H.Pack.CurrentScene()) { T.LastLine.Empty(); Stage(Next); return; }
		const auto* Line = H.Pack.CurrentLine();
		if (!Line) { Fail(TEXT("unexpected choice or missing live line in baseline SC01 scene")); return; }
		if (T.LastLine != Line->Id)
		{ T.LastLine = Line->Id; T.LineAt = T.Elapsed; Shot(TEXT("dialogue-") + Line->Id); return; }
		// Recorded lines are allowed to finish through the real audio host. UI-only
		// description lines use normal manual acknowledgement, with no fake takes.
		if (T.Elapsed - T.LineAt < FMath::Max(.5, Line->Duration + 1.0)) return;
		if (!Enter()) return;
		T.LastLine.Empty(); T.Hold = .25;
	};

	if (T.Stage == 900)
	{
		if (!Walk()) return;
		Stage(T.AfterWalk, .3); return;
	}
	if (T.Stage == 0)
	{
		FString UserDir; FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir);
		UserDir = FPaths::ConvertRelativePathToFull(UserDir); FPaths::CollapseRelativeDirectories(UserDir);
		if (T.Mode != TEXT("SC01") || !(UserDir.StartsWith(TEXT("/tmp/")) || UserDir.StartsWith(TEXT("/var/tmp/"))) ||
			!FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()), UserDir))
		{ Fail(TEXT("S47HorrorContract=SC01 and explicit isolated /tmp UserDir required")); return; }
		for (const TCHAR* Flag : {TEXT("S47T01Contract"), TEXT("S47T02Contract"), TEXT("S47T02DriveContract"), TEXT("S47DaleDocumentsContract"), TEXT("S47EveningEntrance"), TEXT("S47SmokeTest")})
			if (FParse::Param(FCommandLine::Get(), Flag)) { Fail(TEXT("do not combine independent live test drivers")); return; }
		T.Root = FPaths::ProjectSavedDir() / TEXT("HorrorContract") / TEXT("SC01");
		IFileManager::Get().MakeDirectory(*T.Root, true);
		const double BeforeClock = M->Story->Clock();
		H.Begin(nullptr, false);
		if (!Check(!H.Enabled() && !H.Save() && M->Story->Clock() == BeforeClock, TEXT("old case without cohort remains disabled and keeps its clock"))) return;
		auto Bad = S47J::New(); S47J::Set(Bad, TEXT("schemaVersion"), 1); S47J::Set(Bad, TEXT("contentRevision"), 999);
		H.Begin(Bad, false);
		if (!Check(!H.Enabled(), TEXT("unknown future content revision retains old flow without invented SC01 credit"))) return;
		S47J::Set(Bad, TEXT("contentRevision"), 1.5); H.Begin(Bad, false);
		if (!Check(!H.Enabled(), TEXT("fractional content revision is rejected"))) return;
		S47J::Set(Bad, TEXT("contentRevision"), 1); S47J::Set(Bad, TEXT("schemaVersion"), true); H.Begin(Bad, false);
		if (!Check(!H.Enabled(), TEXT("boolean schema cannot be coerced to numeric version one"))) return;
		M->UI->Close(true); M->Story->SetCinematic(false); M->Story->ClearTimers();
		M->World->StopDriving(); M->World->bCommuteVehicle = false; M->World->PlaceTruck(TEXT("saro")); M->World->Enter(TEXT("saro"));
		M->Evening->Begin(nullptr, true); M->Story->BeginEveningShift(); M->Story->SetClock(-21600);
		M->Web->SetDoorOpen(TEXT("doorEast"), false); M->Web->SetDoorOpen(TEXT("doorSouth"), false); M->Web->SetDoorOpen(TEXT("exitDoor"), false);
		M->Player->Place(4.2, 1.6, 0); H.Begin(nullptr, true);
		UE_LOG(LogS47HorrorTest, Display, TEXT("S47: SC01 START: post-handover evening fixture and one initial room placement; later film-budget checks use explicitly synthetic album entries. All SC01 door/lamp/camera/window/report facts use real production controls, camera rays and Walker/Resolve thereafter. No human input/listening claim."));
		if (!Check(H.Enabled() && !H.NormalComplete() && !H.Complete(), TEXT("new cohort begins with no SC01 physical completion"))) return;
		Stage(1, .5); return;
	}
	if (T.Stage == 1) { Drain(2); return; }
	if (T.Stage == 2)
	{
		if (!Check(H.Pack.Flag(TEXT("hr_h01_brief_presented")) && !H.Pack.Flag(TEXT("hr_h01_recorded")), TEXT("actual brief acknowledgement does not grant the physical round"))) return;
		T.BeforeNormal = S47J::Clone(M->Flow->GameState());
		if (!StoreRestore(TEXT("before-normal"), T.BeforeNormal)) return;
		Stage(3, .6); return;
	}
	if (T.Stage == 3)
	{
		if (!Check(H.Enabled() && !H.NormalComplete() && !H.Complete() && !V.WindowArmed(), TEXT("fresh disk/full Flow before round remains incomplete and window unarmed"))) return;
		Aim(Target(TEXT("fieldCamera"))); Stage(4); return;
	}
	if (T.Stage == 4)
	{
		if (!Pick(TEXT("fieldCamera")) || !Check(M->FieldCamera->bHave, TEXT("actual field-camera collection grants the existing early camera"))) return;
		Go({FVector(4.2,0,.0), FVector(4.2,0,-1.2)}, 5); return;
	}
	if (T.Stage == 5) { Aim(Target(TEXT("rack"))); Stage(6); return; }
	if (T.Stage == 6)
	{
		if (!Pick(TEXT("rack")) || !Check(M->Story->BankIn(), TEXT("normal rack action powers the receiver"))) return;
		Go({FVector(3.8,0,-1.2), FVector(1.7,0,-1.2), FVector(0,0,-2.1)}, 7); return;
	}
	if (T.Stage == 7) { Aim(Target(TEXT("crtCenter"))); Stage(8); return; }
	if (T.Stage == 8)
	{
		if (!Pick(TEXT("crtCenter")) || !Check(M->Story->bConsoleOpen, TEXT("actual receiver console opens before calibration"))) return;
		M->Story->Rx.Frequency = 1419.900; M->Story->Rx.Azimuth = 42; M->Story->Rx.Gain = 55; M->Story->Rx.Bandwidth = 48;
		M->Story->ConsoleAction();
		if (!Check(M->Story->Rx.Stage == 1, TEXT("normal ConsoleAction accepts the known calibration reference"))) return;
		M->UI->Close(true); M->Story->ConsoleClosed(); Stage(9,.4); return;
	}
	if (T.Stage == 9)
	{
		if (!Check(H.BaselineChecked(), TEXT("coordinator witnesses the calibrated normal receiver"))) return;
		Go({FVector(0,0,-1.2), FVector(3.8,0,-1.2), FVector(4.2,0,1.6)}, 10); return;
	}
	if (T.Stage == 10) { Aim(Target(TEXT("doorEast"))); Stage(11); return; }
	if (T.Stage == 11)
	{
		if (!Pick(TEXT("doorEast")) || !Check(H.DoorEastTried() && M->Web->IsDoorOpen(TEXT("doorEast")), TEXT("actual inside east-door opening is witnessed"))) return;
		Go({FVector(5.2,0,1.6),FVector(6.1,0,1.6),FVector(9.3,0,1.6),FVector(9.3,0,-12),FVector(7.6,0,-12),FVector(6.1,0,-12),FVector(4.45,0,-6.9)},12); return;
	}
	if (T.Stage == 12) { Aim(FS47HorrorVisual::LampPoint()); Stage(13,1.0); return; }
	if (T.Stage == 13)
	{
		if (!Check(V.LampOn() && !V.LampDirectChecked(), TEXT("one actual exterior on-state view is not both lamp states"))) return;
		Aim(Target(TEXT("horror:lampOutside"))); Stage(14); return;
	}
	if (T.Stage == 14)
	{
		if (!Pick(TEXT("horror:lampOutside")) || !Check(!V.LampOn(), TEXT("physical outside lamp switch turns the reference off"))) return;
		Aim(FS47HorrorVisual::LampPoint()); Stage(15,1.0); return;
	}
	if (T.Stage == 15)
	{
		if (!Check(V.LampDirectChecked() && !V.LampGlassChecked(), TEXT("both actual direct lamp states are observed without glass credit"))) return;
		Aim(Target(TEXT("horror:lampOutside"))); Stage(16); return;
	}
	if (T.Stage == 16)
	{
		if (!Pick(TEXT("horror:lampOutside")) || !Check(V.LampOn(), TEXT("outside control restores normal lamp"))) return;
		Aim(FS47HorrorVisual::LampPoint());
		auto* C1 = Cast<US47Chapter1>(M->Story->Chapter(TEXT("ch1")));
		if (!C1) { Fail(TEXT("existing chapter-one camera host unavailable")); return; }
		C1->ToggleCamera(); Stage(17,.5); return;
	}
	if (T.Stage == 17)
	{
		FS47AlbumShot Fallback; Fallback.Id=TEXT("isolated-fallback-negative");Fallback.Url=TEXT("data:image/jpeg;base64,AA==");
		H.Photo(Fallback);
		if(!Check(!H.ReferencePhotographed(),TEXT("fallback JPEG without actual photo-lens target cannot grant normal reference")))return;
		// Synthetic, explicitly labelled album entries exercise the finite film
		// budget. They are not captured photographs or observation evidence.
		TArray<FColor> BudgetPixels; BudgetPixels.Init(FColor::Black,4);
		const FString BudgetUrl=S47Image::ToDataUrl(BudgetPixels,2,2);
		while(M->Night->Album->Data.Shots.Num()<FS47Album::ROLL-1)
		{
			FS47AlbumShot Budget;Budget.Id=FString::Printf(TEXT("synthetic-film-budget-%d"),M->Night->Album->Data.Shots.Num());
			Budget.Url=BudgetUrl;Budget.Text=TEXT("Synthetic film-budget fixture; no photographic evidence.");
			M->Night->Album->Data.Shots.Add(Budget);
		}
		T.BudgetPhotos=M->Night->Album->Shots();
		if(!Check(M->Night->Album->Left()==1 && H.ReferenceFrameReserved(),TEXT("35 synthetic budget entries leave the final real exposure reserved for the required reference")))return;
		T.FilmReserveBase=S47J::Clone(M->Flow->GameState());
		FS47AlbumShot LastBudget=M->Night->Album->Shots().Last();LastBudget.Id=TEXT("synthetic-old-exhausted-roll");
		M->Night->Album->Data.Shots.Add(LastBudget);
		if(!StoreRestore(TEXT("old-exhausted-roll-without-reference"),M->Flow->GameState()))return;
		Stage(170,.6);return;
	}
	if(T.Stage==170)
	{
		if(!Check(!H.Enabled() && !H.ReferencePhotographed() && M->Night->Album->Shots().Num()==FS47Album::ROLL && M->Story->Rx.Stage==1,
		 TEXT("earlier exhausted cohort retains every print and existing receiver progression without invented reference or new SC01 gate")))return;
		if(!StoreRestore(TEXT("reserved-final-frame-restored"),T.FilmReserveBase))return;
		Stage(171,.6);return;
	}
	if(T.Stage==171)
	{
		if(!Check(H.Enabled() && H.ReferenceFrameReserved() && M->Night->Album->Left()==1,TEXT("fresh Flow restores the reserved final frame before any genuine reference")))return;
		Aim(FVector(-10,1.5,-6.9));
		auto* C1=Cast<US47Chapter1>(M->Story->Chapter(TEXT("ch1")));if(!C1){Fail(TEXT("restored camera host unavailable"));return;}
		if(!M->FieldCamera->bRaised)C1->ToggleCamera();
		Stage(172,.35);return;
	}
	if(T.Stage==172)
	{
		auto* C1=Cast<US47Chapter1>(M->Story->Chapter(TEXT("ch1")));C1->Shutter();
		if(!Check(M->Night->Album->Shots().Num()==FS47Album::ROLL-1 && M->Night->Album->Left()==1 && !H.ReferencePhotographed(),
		 TEXT("ordinary shutter refuses another subject without spending the reserved frame or granting reference credit")))return;
		Aim(FS47HorrorVisual::LampPoint());Stage(173,.35);return;
	}
	if(T.Stage==173)
	{
		auto* C1=Cast<US47Chapter1>(M->Story->Chapter(TEXT("ch1")));
		const int32 Before = M->Night->Album->Shots().Num(); C1->Shutter();
		if (!Check(M->Night->Album->Shots().Num() == Before + 1, TEXT("normal camera shutter captures an actual instant-back reference"))) return;
		const auto& Shot = M->Night->Album->Shots().Last(); T.NormalPhoto = Shot.Id;
		if (!Check(!Shot.Url.IsEmpty() && !Shot.bUnavailable && H.ReferencePhotographed(), TEXT("actual rendered reference pixels establish the normal photo"))) return;
		bool Kept=Before==T.BudgetPhotos.Num();for(int32 I=0;I<T.BudgetPhotos.Num() && Kept;++I)
		 {const auto& Prior=T.BudgetPhotos[I];const auto& Now=M->Night->Album->Shots()[I];Kept=Now.Id==Prior.Id && Now.Url==Prior.Url && Now.Clock==Prior.Clock && Now.Targets.Num()==Prior.Targets.Num();}
		if(!Check(Kept && M->Night->Album->Shots().Num()==FS47Album::ROLL && M->Night->Album->Left()==0,
		 TEXT("required real reference uses only the reserved 36th frame and preserves every preceding album entry")))return;
		C1->ToggleCamera();
		Go({FVector(6.1,0,-12),FVector(7.6,0,-12),FVector(9.3,0,-12),FVector(9.3,0,1.6),FVector(6.1,0,1.6),FVector(5.2,0,1.6),FVector(4.2,0,-1.2),FVector(4.45,0,-3.1)},18); return;
	}
	if (T.Stage == 18) { Aim(FS47HorrorVisual::LampPoint()); Stage(19,1.0); return; }
	if (T.Stage == 19)
	{
		if (!Check(!V.LampGlassChecked(), TEXT("one actual glass-view state is not a complete comparison"))) return;
		Aim(Target(TEXT("horror:lamp"))); Stage(20); return;
	}
	if (T.Stage == 20)
	{
		if (!Pick(TEXT("horror:lamp")) || !Check(!V.LampOn(), TEXT("inside physical switch turns the same exterior lamp off"))) return;
		Aim(FS47HorrorVisual::LampPoint()); Stage(21,1.0); return;
	}
	if (T.Stage == 21)
	{
		if (!Check(V.LampGlassChecked() && V.LampChangeChecked(), TEXT("actual through-glass and exterior on/off comparisons both complete"))) return;
		Aim(Target(TEXT("horror:lamp"))); Stage(22); return;
	}
	if (T.Stage == 22)
	{
		if (!Pick(TEXT("horror:lamp"))) return;
		Go({FVector(4.2,0,-1.2),FVector(0,0,-1.2),FVector(-2.4,0,1.0),FVector(-2.4,0,3.4)},23); return;
	}
	if (T.Stage == 23) { Aim(Target(TEXT("doorSouth"))); Stage(24); return; }
	if (T.Stage == 24)
	{
		if (!Pick(TEXT("doorSouth"))) return;
		Go({FVector(-2.4,0,4.0),FVector(-2.4,0,4.8),FVector(-2.4,0,5.7),FVector(-4.6,0,5.7)},25); return;
	}
	if (T.Stage == 25) { Aim(Target(TEXT("exitDoor"))); Stage(26); return; }
	if (T.Stage == 26)
	{
		if (!Pick(TEXT("exitDoor")) || !Check(H.DoorCorridorTried() && M->Web->IsDoorOpen(TEXT("exitDoor")), TEXT("actual corridor-side fire-exit action is witnessed"))) return;
		Go({FVector(-5.3,0,5.7),FVector(-6.4,0,5.7),FVector(-7.5,0,5.7),FVector(-6.4,0,5.7),FVector(-5.3,0,5.7),FVector(-4.6,0,5.7),FVector(-2.4,0,5.7),FVector(-2.4,0,3.4),FVector(-2.4,0,1.0),FVector(0,0,-1.2),FVector(4.2,0,-1.2),FVector(4.45,0,-3.1)},27); return;
	}
	if (T.Stage == 27) { Aim(Target(TEXT("horror:screen"))); Stage(28); return; }
	if (T.Stage == 28)
	{
		if (!Pick(TEXT("horror:screen"))) return;
		Aim(FS47HorrorVisual::LampPoint()); Stage(29,1.1); return;
	}
	if (T.Stage == 29)
	{
		if(H.Pack.CurrentScene() && H.Pack.CurrentScene()->Id==TEXT("H01_CHECK") && !T.bPendingNormalTest)
		{
			T.bPendingNormalTest=true;T.PendingNormal=S47J::Clone(M->Flow->GameState());
			auto Bad=S47J::Clone(T.PendingNormal);S47J::Set(S47J::Obj(Bad,TEXT("horror")),TEXT("referencePhoto"),TEXT("isolated-pending-missing-photo"));
			if(!StoreRestore(TEXT("pending-check-missing-photo"),Bad))return;
			Stage(290,.5);return;
		}
		if (H.Pack.CurrentScene()) { Drain(29); return; }
		if (!Check(V.ScreenClosed() && V.ScreenChecked() && H.NormalComplete() && V.WindowArmed() && V.WindowPhase()==0, TEXT("actual opaque shutter blocks normal lamp, completes reference round and arms an unseen window event"))) return;
		Shot(TEXT("normal-shutter-closed")); T.AfterNormal = S47J::Clone(M->Flow->GameState());
		if (!StoreRestore(TEXT("after-normal"),T.AfterNormal)) return;
		Stage(30,.6); return;
	}
	if(T.Stage==290)
	{
		if(!Check(!H.ReferencePhotographed() && !H.NormalComplete() && !H.Pack.CurrentScene() && !H.Pack.Flag(TEXT("hr_h01_recorded")),TEXT("a missing original photo cancels pending supported conclusion without granting or trapping its result")))return;
		if(!StoreRestore(TEXT("pending-check-original-restored"),T.PendingNormal))return;
		Stage(291,.5);return;
	}
	if(T.Stage==291)
	{
		if(!Check(H.NormalComplete() && H.Pack.CurrentScene() && H.Pack.CurrentScene()->Id==TEXT("H01_CHECK"),TEXT("the actual supported pending H01 check resumes after original-photo restore")))return;
		Stage(29,.2);return;
	}
	if (T.Stage == 30)
	{
		if (!Check(H.NormalComplete() && H.BaselineChecked() && H.DoorEastTried() && H.DoorCorridorTried() && !H.Complete() && V.ScreenClosed(), TEXT("full Flow restore retains real round facts without window completion"))) return;
		auto Bad = S47J::Clone(T.AfterNormal);
		for(const auto& Value:S47J::Arr(S47J::Obj(Bad,TEXT("album")),TEXT("shots")))
		{
			const auto Shot=Value->AsObject();
			if(S47J::Str(Shot,TEXT("id"))==T.NormalPhoto)Shot->RemoveField(TEXT("targets"));
		}
		if(!StoreRestore(TEXT("missing-reference-target"),Bad))return;
		Stage(301,.5);return;
	}
	if(T.Stage==301)
	{
		if(!Check(!H.ReferencePhotographed() && !H.NormalComplete(),TEXT("fresh Flow refuses reference credit when real retained photo lacks the actual lamp target")))return;
		if(!StoreRestore(TEXT("reference-target-restored"),T.AfterNormal))return;
		Stage(302,.5);return;
	}
	if(T.Stage==302)
	{
		if(!Check(H.ReferencePhotographed() && H.NormalComplete(),TEXT("actual original photo target restores its valid reference credit")))return;
		auto Bad = S47J::Clone(T.AfterNormal); S47J::Set(S47J::Obj(Bad,TEXT("horror")),TEXT("referencePhoto"),TEXT("isolated-missing-photo"));
		if(!StoreRestore(TEXT("missing-reference-id"),Bad))return;
		Stage(303,.5);return;
	}
	if(T.Stage==303)
	{
		if(!Check(!H.ReferencePhotographed() && !H.NormalComplete(),TEXT("a missing original album photo cannot retain normal-reference credit")))return;
		if(!StoreRestore(TEXT("reference-id-restored"),T.AfterNormal))return;
		Stage(304,.5);return;
	}
	if(T.Stage==304)
	{
		auto Bad = S47J::Clone(T.AfterNormal); S47J::Set(S47J::Obj(Bad,TEXT("horror")),TEXT("contentRevision"),999);
		if (!StoreRestore(TEXT("bad-content-revision"),Bad)) return;
		Stage(31,.5); return;
	}
	if (T.Stage == 31)
	{
		if (!Check(!H.Enabled() && !H.Complete(), TEXT("actual full Flow rejects unsupported SC content without granting completion"))) return;
		if (!StoreRestore(TEXT("normal-restored"),T.AfterNormal)) return;
		Stage(32,.6); return;
	}
	if (T.Stage == 32)
	{
		if (!Check(H.NormalComplete() && V.WindowPhase()==0, TEXT("good content revision restores normal round after rejected future fixture"))) return;
		Aim(Target(TEXT("horror:screen"))); Stage(33); return;
	}
	if (T.Stage == 33)
	{
		if (!Pick(TEXT("horror:screen"))) return;
		Aim(FS47HorrorVisual::WindowPoint()); Stage(34,T.bMissHand?0:.75); return;
	}
	if (T.Stage == 34)
	{
		if (V.WindowPhase()!=1) { if(T.Elapsed-T.Since>8)Fail(TEXT("armed physical window failed to start in an actual camera view"));return; }
		if(T.bMissHand)
		{
			if(!Check(!V.HandSeen(),TEXT("brief initial window glance does not award the moving-hand witness")))return;
			T.FirstHand=V.HandPosition();Aim(FVector(-10,1.5,8));Stage(340,.5);return;
		}
		T.FirstHand=V.HandPosition();T.PausedHandTime=V.WindowTime();Shot(TEXT("window-hand-first"));
		M->UI->Open(S47Kit::Shell(M->UI,TEXT("ISOLATED MODAL PAUSE CONTROL"),S47Kit::Text(TEXT("The world must remain paused while this panel is open."))));
		Stage(35,1.1);return;
	}
	if(T.Stage==340)
	{
		if(!Check(V.WindowPhase()==1 && !V.HandSeen() && !V.HandPosition().Equals(T.FirstHand,.015),TEXT("hand moves while player looks away without granting a body sighting")))return;
		if(!StoreRestore(TEXT("mid-unwitnessed-window"),M->Flow->GameState()))return;
		Stage(341,.6);return;
	}
	if(T.Stage==341)
	{
		if(!Check(V.WindowArmed() && !V.HandSeen() && V.WindowPhase()>=1,TEXT("full Flow restores the unwitnessed live window without inventing a body sighting")))return;
		Aim(FVector(-10,1.5,8));Stage(38,.2);return;
	}
	if(T.Stage==35)
	{
		if(!Check(M->ModalOpen() && FMath::IsNearlyEqual(V.WindowTime(),T.PausedHandTime,.002) && V.HandPosition().Equals(T.FirstHand,.002),TEXT("actual modal freezes the hand's world time and position without accrued motion")))return;
		M->UI->Close(true);Aim(FS47HorrorVisual::WindowPoint());Stage(36,.6);return;
	}
	if(T.Stage==36)
	{
		if(!Check(V.WindowPhase()==1 && !V.HandPosition().Equals(T.FirstHand,.015),TEXT("rendered physical hand moves while the player keeps viewing the glass")))return;
		Shot(TEXT("window-hand-moved"));
		if(!StoreRestore(TEXT("mid-window"),M->Flow->GameState()))return;
		Stage(37,.5);return;
	}
	if(T.Stage==37)
	{
		if(!Check(V.WindowArmed() && V.WindowPhase()>=1,TEXT("fresh disk/full Flow preserves an armed window instead of replaying the ordinary round")))return;
		Aim(FS47HorrorVisual::WindowPoint());Stage(38,.2);return;
	}
	if(T.Stage==38)
	{
		if(T.bMissHand)Aim(FVector(-10,1.5,8));else Aim(V.HandPosition());
		if(V.WindowPhase()!=2)return;
		if(!Check(V.MarksPresent() && V.HandSeen()!=T.bMissHand && !H.Complete(),T.bMissHand?
		 TEXT("unwitnessed hand leaves physical exterior marks without a body sighting or completed inspection"):
		 TEXT("actually observed hand leaves real exterior marks without granting completed inspection")))return;
		Shot(TEXT("window-marks-before-check"));
		// A call from across the room, looking away, is a refusal, not an observation.
		Aim(FVector(-10,1.5,8));Stage(380,.3);return;
	}
	if(T.Stage==380)
	{
		const bool Before=V.MarksChecked();
		const int32 BeforeMask=S47J::Int(V.Save(),TEXT("marksMask"));
		if(!Check(V.Use(TEXT("horror:marks")) && V.MarksChecked()==Before && S47J::Int(V.Save(),TEXT("marksMask"))==BeforeMask,
		 TEXT("looking away cannot inspect or partially credit the exterior mark target")))return;
		Aim(Target(TEXT("horror:marks")));Stage(39,.35);return;
	}
	if(T.Stage==39)
	{
		if(!Pick(TEXT("horror:marks")) || !Check(!V.MarksChecked(),TEXT("one room-light state is not both actual mark comparisons")))return;
		Aim(Target(TEXT("horror:roomLamp")));Stage(40);return;
	}
	if(T.Stage==40)
	{
		if(!Pick(TEXT("horror:roomLamp")))return;
		Aim(Target(TEXT("horror:marks")));Stage(41,.5);return;
	}
	if(T.Stage==41)
	{
		if(!Pick(TEXT("horror:marks")) || !Check(V.MarksChecked(),TEXT("actual marks persist across both room-lamp states through the real glass")))return;
		Stage(42,.3);return;
	}
	if(T.Stage==42)
	{
		if(H.Pack.CurrentScene()){Drain(42);return;}
		T.AfterWindow=S47J::Clone(M->Flow->GameState());
		if(!StoreRestore(TEXT("after-window"),T.AfterWindow))return;
		Stage(43,.6);return;
	}
	if(T.Stage==43)
	{
		if(!Check(V.HandSeen()!=T.bMissHand && V.MarksPresent() && V.MarksChecked(),T.bMissHand?
		 TEXT("full Flow preserves inspected outside marks while retaining the absence of a witnessed body"):
		 TEXT("full Flow restore preserves actual hand witness and inspected outside marks")))return;
		Go({FVector(4.2,0,-1.2),FVector(0,0,-1.2),FVector(-2.4,0,1),FVector(-2.4,0,3.4),FVector(-2.4,0,5.7),FVector(-4.6,0,5.7)},44);return;
	}
	if(T.Stage==44){Aim(Target(TEXT("exitDoor")));Stage(45);return;}
	if(T.Stage==45)
	{
		if(!Pick(TEXT("exitDoor")) || !Check(!M->Web->IsDoorOpen(TEXT("exitDoor")),TEXT("known fire exit is actually secured from its inside after the window event")))return;
		Go({FVector(-2.4,0,5.7),FVector(-2.4,0,3.4),FVector(-2.4,0,1),FVector(0,0,1),FVector(4.65,0,1.65)},46);return;
	}
	if(T.Stage==46){Aim(Target(TEXT("doorEast")));Stage(47);return;}
	if(T.Stage==47)
	{
		if(!Pick(TEXT("doorEast")) || !Check(!M->Web->IsDoorOpen(TEXT("doorEast")),TEXT("known east door is actually secured after the exterior check")))return;
		Stage(48,.4);return;
	}
	if(T.Stage==48)
	{
		if(H.Pack.CurrentScene()){Drain(48);return;}
		if(!Check(H.Complete() && !H.ReportAcknowledged(),TEXT("SC01 completes after genuine round/marks/security while reporting remains a distinct optional contact")))return;
		Go({FVector(3.8,0,1.4)},49);return;
	}
	if(T.Stage==49){Aim(Target(TEXT("phone")));Stage(50);return;}
	if(T.Stage==50)
	{
		if(!Pick(TEXT("phone")) || !Check(!H.ReportAcknowledged(),TEXT("opening the actual contact menu alone gives no report readback")))return;
		if(T.bMissHand)
		{
			if(!Check(!H.Pack.Flag(TEXT("hr_sc01_window_check_presented")) && !H.Pack.Documents().Contains(TEXT("HR_INITIAL_REPORT")),TEXT("unseen body cannot produce its witness dialogue or dispatch report")))return;
			Shot(TEXT("unwitnessed-window-contacts"));M->UI->Close(true);
			if(!StoreRestore(TEXT("after-unwitnessed-window"),M->Flow->GameState()))return;
			Stage(54,.6);return;
		}
		Stage(51,.3);return;
	}
	if(T.Stage==51)
	{
		if(!Enter() || !Check(!H.ReportAcknowledged(),TEXT("starting the real phone exchange gives no premature report receipt")))return;
		Shot(TEXT("first-report-pending"));
		if(!StoreRestore(TEXT("before-report-readback"),M->Flow->GameState()))return;
		Stage(52,.6);return;
	}
	if(T.Stage==52)
	{
		if(H.ReportAcknowledged()){Stage(53,.3);return;}
		const auto Saved=H.Save();const int32 Cursor=S47J::Int(Saved,TEXT("callCursor"));
		const auto* Scene=H.Pack.FindScene(TEXT("SC01_FIRST_REPORT"));
		if(!Scene || !Scene->Lines.IsValidIndex(Cursor)){Fail(TEXT("pending world phone call has no original response line"));return;}
		const auto& Line=Scene->Lines[Cursor];
		if(T.LastLine!=Line.Id){T.LastLine=Line.Id;T.LineAt=T.Elapsed;return;}
		if(T.Elapsed-T.LineAt<FMath::Max(.5,Line.Duration+1))return;
		if(!Enter())return;T.LastLine.Empty();T.Hold=.25;return;
	}
	if(T.Stage==53)
	{
		if(!Check(H.ReportAcknowledged() && H.Pack.Flag(TEXT("hr_sc01_first_report_presented")) && H.Pack.Documents().Contains(TEXT("HR_INITIAL_REPORT")),TEXT("only completed actual dispatch response grants the distinct report receipt")))return;
		Shot(TEXT("first-report-complete"));T.AfterReport=S47J::Clone(M->Flow->GameState());
		if(!StoreRestore(TEXT("after-report"),T.AfterReport))return;
		Stage(54,.6);return;
	}
	if(T.Stage==54)
	{
		int32 Reports=0;for(const auto& Doc:M->Story->Docs())if(Doc.Id==TEXT("HR_INITIAL_REPORT"))++Reports;
		if(!Check(H.Complete() && H.ReportAcknowledged()!=T.bMissHand && Reports==(T.bMissHand?0:1) && H.ReferencePhotographed(),T.bMissHand?
		 TEXT("fresh disk/full Flow retains completed SC01 and actual reference with no unsupported report"):
		 TEXT("fresh disk/full Flow retains exactly one actual report, real photo and completed SC01")))return;
		if(!Check(!H.Pack.Flag(TEXT("hr_h02_recorded")),TEXT("SC01 acceptance does not falsely complete the later H02 physical route")))return;
		if(!Check(T.WalkMetres>45 && T.Stores>=6,TEXT("live acceptance includes continuous native walking and independent disk readers")))return;
		Go({FVector(4.2,0,1.6)},55);return;
	}
	if(T.Stage==55){Aim(Target(TEXT("doorEast")));Stage(56);return;}
	if(T.Stage==56)
	{
		if(!Pick(TEXT("doorEast")) || !Check(M->Web->IsDoorOpen(TEXT("doorEast")) && H.Complete(),TEXT("reopening a known exit after completed SC01 does not revoke progress")))return;
		if(!StoreRestore(TEXT("completed-door-reopened"),M->Flow->GameState()))return;
		Stage(57,.6);return;
	}
	if(T.Stage==57)
	{
		if(!Check(H.Complete() && H.ReportAcknowledged()!=T.bMissHand && M->Web->IsDoorOpen(TEXT("doorEast")),TEXT("completed SC01 survives Continue with a subsequently reopened exit")))return;
		T.bDone=true;M->Player->TestMove.Reset();
		auto Result=S47J::New();S47J::Set(Result,TEXT("status"),TEXT("PASS"));S47J::Set(Result,TEXT("scenario"),TEXT("SC01"));
		S47J::Set(Result,TEXT("checks"),T.Checks);S47J::Set(Result,TEXT("diskFlowRestores"),T.Stores);S47J::Set(Result,TEXT("nativeWalkMetres"),T.WalkMetres);
		S47J::Set(Result,TEXT("normalPhoto"),T.NormalPhoto);Result->SetArrayField(TEXT("evidence"),T.Evidence);
		S47J::Set(Result,TEXT("missedHand"),T.bMissHand);
		S47J::Set(Result,TEXT("fixture"),TEXT("Post-handover evening, one initial room placement and 35 explicitly synthetic film-budget entries. Receiver dials controlled through production console; subsequent movement uses native Walker/Resolve, interactions require aimed camera rays, UI uses focused Slate Enter. The reserved reference is a new actual rendered capture."));
		S47J::Set(Result,TEXT("limits"),TEXT("Scripted live contract, not human play/listening, full-night pacing, physical Xbox or whole-story acceptance. SC02+ and H01 closure remain separate."));
		FFileHelper::SaveStringToFile(S47J::Write(Result,true),*(T.Root/TEXT("result.json")));
		UE_LOG(LogS47HorrorTest,Display,TEXT("S47: Horror SC01 PASS COMPLETE: %d checks, %d disk/Flow restores, %.2f m native walking"),T.Checks,T.Stores,T.WalkMetres);
		FPlatformMisc::RequestExit(false);return;
	}
}

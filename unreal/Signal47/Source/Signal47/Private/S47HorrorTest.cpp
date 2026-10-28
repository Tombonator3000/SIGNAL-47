#include "S47HorrorTest.h"

#include "S47Evening.h"
#include "S47Album.h"
#include "S47Chapter1.h"
#include "S47FieldCamera.h"
#include "S47Flow.h"
#include "S47Horror.h"
#include "S47HorrorThreat.h"
#include "S47HorrorSC03.h"
#include "S47HorrorVisual.h"
#include "S47HUD.h"
#include "S47Inter.h"
#include "S47Image.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Kit.h"
#include "S47Saves.h"
#include "S47Story.h"
#include "S47Props.h"
#include "S47Substation.h"
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
	double LineAt = 0, SC02Clock = 0, SC02Seen = 0, SC02RunMetres = 0;
	bool bCaptured = false, bRunPath = false, bSC02Escape = false, bSC02EscapeRouteDone = false;
	int32 ArcBursts = 0;
	bool bArcCloseup = false;
	float ArcOriginalFov = 0;
	FS47Substation::FArcSnapshot PausedArc;
	FString Mode, Root, LastLine, NormalPhoto, T01Stamp, Aimed, RestoreSource;
	FVector FieldTruck = FVector::ZeroVector, FieldEgress = FVector::ZeroVector, Door3 = FVector::ZeroVector;
	double DoorOut = 1;
	TArray<FVector> Points;
	FVector LastFoot = FVector::ZeroVector, FirstHand = FVector::ZeroVector;
	S47J::FObj BeforeNormal, AfterNormal, AfterWindow, AfterReport, PendingNormal, FilmReserveBase;
	TArray<FS47AlbumShot> BudgetPhotos;
	TArray<TSharedPtr<FJsonValue>> Evidence;
};

FS47HorrorTest::FS47HorrorTest(US47Main* InM) : State(MakeUnique<FState>()), M(InM)
{
	State->bEnabled = FParse::Value(FCommandLine::Get(), TEXT("S47HorrorContract="), State->Mode);
	// The window event is now a slap heard behind the shade; it cannot be missed by looking away.
	State->bMissHand = false;
	State->bArcCloseup = State->Mode == TEXT("Arc") && FParse::Param(FCommandLine::Get(), TEXT("S47ArcCloseup"));
}
FS47HorrorTest::~FS47HorrorTest() = default;

void FS47HorrorTest::Fail(const FString& Why)
{
	if (State->bDone) return;
	State->bDone = true;
	if (M && M->Player)
	{
		M->Player->TestMove.Reset(); M->Player->bTestRun = false;
		if (State->ArcOriginalFov > 0) { M->Player->Fov = State->ArcOriginalFov; State->ArcOriginalFov = 0; }
	}
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
	if (M && M->Player) { M->Player->TestMove.Reset(); M->Player->bTestRun = false; }
	if (Next != 900)
	{
		State->Points.Reset(); State->WalkIndex = State->AfterWalk = 0; State->bRunPath = false;
	}
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
	M->Player->TestMove.Reset(); M->Player->bTestRun = false;
	State->Points = Points; State->WalkIndex = 0; State->bRunPath = false;
	State->LastFoot = M->Player->Pos; State->LastProgress = State->Elapsed;
}

bool FS47HorrorTest::Walk()
{
	if (M->Story->IsPaused() || M->ModalOpen())
	{
		M->Player->TestMove.Reset(); M->Player->bTestRun = false;
		State->LastFoot = M->Player->Pos; State->LastProgress = State->Elapsed;
		return false;
	}
	const FVector At = M->Player->Pos;
	const double Moved = FVector2D(At.X - State->LastFoot.X, At.Z - State->LastFoot.Z).Size();
	const double MaxStep = State->bRunPath ? FMath::Max(.5, M->Player->Speed * 1.6 * .2 + .1) : .5;
	if (Moved > MaxStep) { Fail(TEXT("waypoint route jumped beyond a native Walker step")); return false; }
	if (State->bSC02Escape && FS47HorrorThreat::InYard(FVector2D(At.X, At.Z), M->Player->Radius))
	{ Fail(TEXT("SC02 escape route entered the closed yard instead of going around it")); return false; }
	State->WalkMetres += Moved; State->LastFoot = At;
	if (State->bRunPath && State->bSC02Escape) State->SC02RunMetres += Moved;
	if (Moved > .002) State->LastProgress = State->Elapsed;
	if (State->WalkIndex >= State->Points.Num()) { M->Player->TestMove.Reset(); return true; }
	const FVector D = State->Points[State->WalkIndex] - At;
	const double Distance = FVector2D(D.X, D.Z).Size();
	if (Distance < .09) { ++State->WalkIndex; M->Player->TestMove.Reset(); State->LastProgress = State->Elapsed; return false; }
	if (State->Elapsed - State->LastProgress > 4)
	{
		for (const FS47Rect& C : M->Web->Colliders())
			if (C.bEnabled && C.MinX < At.X + 4 && C.MaxX > At.X - 4 && C.MinZ < At.Z + 4 && C.MaxZ > At.Z - 4)
				UE_LOG(LogS47HorrorTest, Display, TEXT("S47: blocked walk near collider %s x %.2f..%.2f z %.2f..%.2f"), *C.Id, C.MinX, C.MaxX, C.MinZ, C.MaxZ);
		for (const FS47Rect& Z : M->Web->Zones())
			if (Z.bEnabled && Z.MinX < At.X + 4 && Z.MaxX > At.X - 4 && Z.MinZ < At.Z + 4 && Z.MaxZ > At.Z - 4)
				UE_LOG(LogS47HorrorTest, Display, TEXT("S47: blocked walk near zone %s x %.2f..%.2f z %.2f..%.2f"), *Z.Id, Z.MinX, Z.MaxX, Z.MinZ, Z.MaxZ);
		Fail(FString::Printf(TEXT("native Walker path blocked at %.3f %.3f toward %.3f %.3f"), At.X, At.Z, State->Points[State->WalkIndex].X, State->Points[State->WalkIndex].Z));
		return false;
	}
	M->Player->Yaw = FMath::Atan2(-D.X, -D.Z); M->Player->Pitch = -.06;
	M->Player->bTestRun = State->bRunPath;
	M->Player->TestMove = FVector2D(0, -FMath::Min(1., Distance * 2));
	return false;
}

bool FS47HorrorTest::StoreRestore(const FString& Name, const S47J::FObj& Snapshot, bool bRestore)
{
	FS47SaveStore Writer(State->Root / Name); Writer.Init(); FS47SaveCard Card;
	Card.Chapter = TEXT("SC01 live contract"); Card.Place = TEXT("SARO"); Card.Clock = M->Story->ClockText(true);
	FString Error;
	const auto Meta = Writer.Save(1, TEXT("manual"), 0, S47J::Clone(Snapshot), Card, Error);
	if (!Meta) { Fail(TEXT("isolated Writer failed: ") + Error); return false; }
	FS47SaveStore Reader(State->Root / Name); Reader.Init(); const auto Disk = Reader.Load(Meta->Id);
	if (!Check(Disk.IsValid(), TEXT("fresh isolated disk reader: ") + Name)) return false;
	FFileHelper::SaveStringToFile(S47J::Write(Disk, true), *(State->Root / (Name + TEXT(".loaded.json"))));
	if (!bRestore) return true;
	++State->Stores;
	// A loaded checkpoint starts a new route; never reuse waypoints from the pre-load pose.
	M->Player->TestMove.Reset(); M->Player->bTestRun = false;
	State->Points.Reset(); State->WalkIndex = State->AfterWalk = 0; State->bRunPath = State->bSC02Escape = false;
	State->LastFoot = M->Player->Pos; State->LastProgress = State->Elapsed;
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
	// Only Walk() may request running, and only on the explicitly labelled escape route.
	M->Player->bTestRun = false;
	if (M->bPausedByMenu)
	{ M->Player->TestMove.Reset(); T.LastFoot = M->Player->Pos; T.LastProgress = T.Elapsed; return; }
	// Story pause also covers the modal panels that this controller must acknowledge.
	if (M->Story->IsPaused()) M->Player->TestMove.Reset();
	T.Elapsed += FMath::Clamp(Dt, 0., .2);
	if (T.Elapsed > (T.Mode == TEXT("SC03") ? 1800 : T.Mode == TEXT("SC03Resume") ? 900 : (T.Mode == TEXT("SC02") || T.Mode == TEXT("SC02Resume")) ? 900 : 420)) { Fail(TEXT("live horror contract control timeout")); return; }
	if (!M->Flow->bDevRun || M->Flow->CaseId != 0) { Fail(TEXT("developer case zero required; never write a player case")); return; }
	if (M->Flow->bStarting || M->World->bBusy)
	{
		M->Player->TestMove.Reset();
		if (M->Flow->bStarting && T.Stage == 900 && !T.Points.IsEmpty())
		{ T.Points.Reset(); T.bRunPath = false; Fail(TEXT("Flow restore interrupted a waypoint route; stale points must not resume")); }
		return;
	}
	auto& H = *M->Horror; auto& V = *H.Visual;
	if (T.bSC02Escape)
	{
		const auto Saved = S47J::Obj(H.Save(), TEXT("sc02"));
		if ((Saved && S47J::Bool(Saved, TEXT("retry"))) || (H.Pack.CurrentScene() && H.Pack.CurrentScene()->Id == TEXT("SC02_RETRY")))
		{ Fail(TEXT("SC02 capture/local retry during the expected running escape to SARO 07")); return; }
	}
	if (T.Hold > 0) { T.Hold -= Dt; return; }
	auto Go = [this, &T](const TArray<FVector>& Points, int32 Next, bool bRun = false)
	{ Path(Points); T.bRunPath = bRun; T.AfterWalk = Next; Stage(900, 0); };
	auto Enter = [this]()
	{
		if (!M->ModalOpen() || !FSlateApplication::IsInitialized()) { Fail(TEXT("actual focused modal/Slate unavailable for Enter")); return false; }
		const bool Handled = FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Enter, FModifierKeysState(), 0, false, 0, 0));
		FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(EKeys::Enter, FModifierKeysState(), 0, false, 0, 0));
		return Check(Handled, TEXT("actual focused Slate Enter activates the displayed primary button"));
	};
	auto Drain = [this, &T, &H, &Enter](int32 Next)
	{
		if (!H.Pack.CurrentScene()) { T.LastLine.Empty(); Stage(Next, Next==107?0:.2); return; }
		const auto* Line = H.Pack.CurrentLine();
		if (!Line) { Fail(TEXT("unexpected choice or missing live line in baseline SC01 scene")); return; }
		if (T.LastLine != Line->Id)
		{ T.LastLine = Line->Id; T.LineAt = T.Elapsed; Shot(TEXT("dialogue-") + Line->Id); return; }
		// Recorded lines are allowed to finish through the real audio host. UI-only
		// description lines use normal manual acknowledgement, with no fake takes.
		if (T.Elapsed - T.LineAt < FMath::Max(.5, Line->Duration + 1.0)) return;
		if (!Enter()) return;
		T.LastLine.Empty(); T.Hold = Next==107?0:.25;
	};

	if (T.Stage == 900)
	{
		// SC02's H02 walkie opens on the way to the service front and holds the player;
		// acknowledge it like a player would, then carry on along the same path.
		if ((T.Mode == TEXT("SC02") || T.Mode == TEXT("SC02Resume") || T.Mode == TEXT("SC03") || T.Mode == TEXT("SC03Resume")) && T.AfterWalk >= 100 && H.Pack.CurrentScene())
		{
			if (H.Pack.CurrentScene()->Id == TEXT("SC02_RETRY"))
			{ Fail(TEXT("SC02 capture interrupted a waypoint route before its intended exposure check")); return; }
			M->Player->TestMove.Reset(); T.LastProgress = T.Elapsed; T.LastFoot = M->Player->Pos; Drain(900); return;
		}
		if (!Walk()) return;
		if (T.bSC02Escape && T.bRunPath) T.bSC02EscapeRouteDone = true;
		Stage(T.AfterWalk, .3); return;
	}
	if (T.Stage == 0)
	{
		FString UserDir; FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir);
		UserDir = FPaths::ConvertRelativePathToFull(UserDir); FPaths::CollapseRelativeDirectories(UserDir);
		if ((T.Mode != TEXT("SC01") && T.Mode != TEXT("SC02") && T.Mode != TEXT("SC02Resume") && T.Mode != TEXT("SC03") && T.Mode != TEXT("SC03Resume") && T.Mode != TEXT("Arc") && T.Mode != TEXT("Navigation")) || !(UserDir.StartsWith(TEXT("/tmp/")) || UserDir.StartsWith(TEXT("/var/tmp/"))) ||
			!FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()), UserDir))
		{ Fail(TEXT("S47HorrorContract=SC01, SC02, SC02Resume, SC03, SC03Resume, Arc or Navigation and explicit isolated /tmp UserDir required")); return; }
		for (const TCHAR* Flag : {TEXT("S47T01Contract"), TEXT("S47T02Contract"), TEXT("S47T02DriveContract"), TEXT("S47DaleDocumentsContract"), TEXT("S47EveningEntrance"), TEXT("S47SmokeTest")})
			if (FParse::Param(FCommandLine::Get(), Flag)) { Fail(TEXT("do not combine independent live test drivers")); return; }
		T.Root = FPaths::ProjectSavedDir() / TEXT("HorrorContract") / T.Mode;
		IFileManager::Get().MakeDirectory(*T.Root, true);
		if (T.Mode == TEXT("Navigation"))
		{
			if (!H.Threat) { Fail(TEXT("native navigation probe requires the production threat host")); return; }
			TArray<FString> Lines;
			const bool Good = H.Threat->NativeSelfTest(Lines);
			for (const FString& Line : Lines)
				if (!Check(Line.StartsWith(TEXT("native navigation PASS:")), Line)) return;
			if (!Good) { Fail(TEXT("native navigation boundary probe failed")); return; }
			M->Player->TestMove.Reset(); M->Player->bTestRun = false; T.bDone = true;
			auto Result = S47J::New(); S47J::Set(Result, TEXT("status"), TEXT("PASS"));
			S47J::Set(Result, TEXT("scenario"), TEXT("Navigation")); S47J::Set(Result, TEXT("checks"), T.Checks);
			Result->SetArrayField(TEXT("evidence"), T.Evidence);
			S47J::Set(Result, TEXT("limits"), TEXT("Native production-UWorld capsule sweeps, movable UBox and two-instance HISM bounds/physics. Only transient fixtures move. This is not a gameplay pursuit, dynamic AI detour, human-input or whole-story acceptance test."));
			FFileHelper::SaveStringToFile(S47J::Write(Result, true), *(T.Root / TEXT("result.json")));
			UE_LOG(LogS47HorrorTest, Display, TEXT("S47: Horror native navigation PASS COMPLETE: %d checks"), T.Checks);
			FPlatformMisc::RequestExit(false); return;
		}
		if (T.Mode == TEXT("SC03Resume"))
		{
			if (!FParse::Value(FCommandLine::Get(), TEXT("S47HorrorRestore="), T.RestoreSource) || T.RestoreSource.IsEmpty())
			{ Fail(TEXT("SC03Resume requires an actual saved snapshot through -S47HorrorRestore=/absolute/file.json")); return; }
			T.RestoreSource = FPaths::ConvertRelativePathToFull(T.RestoreSource);
			FString Text;
			if (!FFileHelper::LoadFileToString(Text, *T.RestoreSource)) { Fail(TEXT("could not read SC03Resume source snapshot: ") + T.RestoreSource); return; }
			const auto Saved = S47J::Read(Text);
			const auto Horror = S47J::Obj(Saved, TEXT("horror")), Meeting = S47J::Obj(Horror, TEXT("sc02"));
			if (!Check(Saved && Horror && S47J::Int(Horror, TEXT("contentRevision")) >= 2 && S47J::Str(Meeting, TEXT("phase")) == TEXT("escaped") &&
				!S47J::Str(Meeting, TEXT("escapeStamp")).IsEmpty() && !S47J::Obj(Horror, TEXT("sc03")),
				TEXT("source snapshot is an actual escaped SC02 case before SC03"))) return;
			// Labelled fixture: the revision 2 escape snapshot is relabelled revision 3 so SC03 can be iterated
			// without replaying SC01 and SC02. Full SC03 acceptance runs -S47HorrorContract=SC03 from a new case.
			S47J::Set(Horror, TEXT("contentRevision"), 3);
			if (!StoreRestore(TEXT("sc03-resume-source"), Saved)) return;
			UE_LOG(LogS47HorrorTest, Display, TEXT("S47: SC03Resume loaded %s relabelled as revision 3; SC01/SC02 are not rerun"), *T.RestoreSource);
			Stage(200, 1.0); return;
		}
		if (T.Mode == TEXT("Arc"))
		{
			if (!FParse::Value(FCommandLine::Get(), TEXT("S47HorrorRestore="), T.RestoreSource))
			{ Fail(TEXT("Arc requires an actual pre-meeting snapshot via -S47HorrorRestore=")); return; }
			T.RestoreSource = FPaths::ConvertRelativePathToFull(T.RestoreSource);
			FString Text;
			if (!FFileHelper::LoadFileToString(Text, *T.RestoreSource)) { Fail(TEXT("Arc source snapshot cannot be read")); return; }
			const auto Saved = S47J::Read(Text), Horror = S47J::Obj(Saved, TEXT("horror"));
			const auto Meeting = S47J::Obj(Horror, TEXT("sc02")), Service = S47J::Obj(Saved, TEXT("substation"));
			T.T01Stamp = S47J::Str(Service, TEXT("completedStamp"));
			if (!Check(Saved && S47J::Int(Horror, TEXT("contentRevision")) >= 2 && S47J::Bool(Horror, TEXT("sc01Complete")) &&
				S47J::Str(Meeting, TEXT("phase")) == TEXT("t01") && !S47J::Bool(Meeting, TEXT("threatSeen")) &&
				S47J::Bool(Service, TEXT("journal")) && S47J::Bool(Service, TEXT("complete")) && S47J::Bool(Service, TEXT("observed")) && !T.T01Stamp.IsEmpty(),
				TEXT("Arc source is an actual completed T01 checkpoint before the SC02 meeting"))) return;
			if(FParse::Param(FCommandLine::Get(),TEXT("S47ArcCalm")))M->Flow->bCalmFlash=true;
			if (!StoreRestore(TEXT("arc-pre-meeting"), Saved)) return;
			UE_LOG(LogS47HorrorTest, Display, TEXT("S47: Arc focused contract restores %s; SC01 and T01 actions are retained source evidence, not rerun"), *T.RestoreSource);
			Stage(107, .4); return;
		}
		if (T.Mode == TEXT("SC02Resume"))
		{
			if (!FParse::Value(FCommandLine::Get(), TEXT("S47HorrorRestore="), T.RestoreSource) || T.RestoreSource.IsEmpty())
			{ Fail(TEXT("SC02Resume requires an actual saved snapshot through -S47HorrorRestore=/absolute/file.json")); return; }
			T.RestoreSource = FPaths::ConvertRelativePathToFull(T.RestoreSource);
			FString Text;
			if (!FFileHelper::LoadFileToString(Text, *T.RestoreSource))
			{ Fail(TEXT("could not read SC02Resume source snapshot: ") + T.RestoreSource); return; }
			const auto Saved = S47J::Read(Text);
			const auto Horror = S47J::Obj(Saved, TEXT("horror")), Meeting = S47J::Obj(Horror, TEXT("sc02"));
			const auto Service = S47J::Obj(Saved, TEXT("substation")), Routine = S47J::Obj(Saved, TEXT("routine"));
			const auto Truck = S47J::Obj(Saved, TEXT("truckPose"));
			if (!Check(Saved && S47J::Int(Horror, TEXT("contentRevision")) >= 2 && S47J::Bool(Horror, TEXT("sc01Complete")) &&
				S47J::Str(Meeting, TEXT("phase")) == TEXT("encounter") && S47J::Bool(Meeting, TEXT("threatSeen")) &&
				!S47J::Bool(Meeting, TEXT("retry")) && S47J::Str(Meeting, TEXT("escapeStamp")).IsEmpty(),
				TEXT("source snapshot contains the actual witnessed SC02 meeting before capture and escape"))) return;
			T.T01Stamp = S47J::Str(Service, TEXT("completedStamp"));
			if (!Check(S47J::Bool(Service, TEXT("journal")) && S47J::Bool(Service, TEXT("complete")) && S47J::Bool(Service, TEXT("observed")) &&
				!T.T01Stamp.IsEmpty() && Routine && S47J::Has(Routine, TEXT("clock")) && FMath::IsFinite(S47J::Num(Routine, TEXT("clock"))) && Truck,
				TEXT("source snapshot retains its signed T01 result, real routine clock and parked truck pose"))) return;
			T.SC02Clock = S47J::Num(Routine, TEXT("clock"));
			T.FieldTruck = FVector(S47J::Num(Truck, TEXT("x")), S47J::Num(Truck, TEXT("y")), S47J::Num(Truck, TEXT("z")));
			if (!StoreRestore(TEXT("sc02-resume-source"), Saved)) return;
			UE_LOG(LogS47HorrorTest, Display, TEXT("S47: SC02Resume loaded actual snapshot %s; prior SC01/T01/trip are not rerun"), *T.RestoreSource);
			Stage(1101, 1.0); return;
		}
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
		Aim(FS47HorrorVisual::WindowPoint()); Stage(330,1.3); return;
	}
	if (T.Stage == 330)
	{
		if (!Check(!V.ScreenClosed() && V.WindowArmed() && V.WindowPhase()==0 && !V.MarksPresent() && !V.HandSeen(),
		 TEXT("raising the checked shade shows clear glass and starts no window event"))) return;
		Aim(Target(TEXT("horror:screen"))); Stage(331); return;
	}
	if (T.Stage == 331)
	{
		if (!Pick(TEXT("horror:screen"))) return;
		Aim(FS47HorrorVisual::WindowPoint()); Stage(34,.2); return;
	}
	if (T.Stage == 34)
	{
		if (V.WindowPhase()!=1) { if(T.Elapsed-T.Since>8)Fail(TEXT("lowering the shade for the night did not arm the slap behind it"));return; }
		if (!Check(!V.HandSeen() && !V.MarksPresent(),TEXT("closing the shade for the night arms the slap without hearing or marks yet")))return;
		T.PausedHandTime=V.WindowTime();
		M->UI->Open(S47Kit::Shell(M->UI,TEXT("ISOLATED MODAL PAUSE CONTROL"),S47Kit::Text(TEXT("The world must remain paused while this panel is open."))));
		Stage(35,1.1);return;
	}
	if(T.Stage==35)
	{
		if(!Check(M->ModalOpen() && V.WindowPhase()==1 && FMath::IsNearlyEqual(V.WindowTime(),T.PausedHandTime,.002),TEXT("actual modal holds the pending slap without accrued time")))return;
		M->UI->Close(true);
		if(!StoreRestore(TEXT("mid-window"),M->Flow->GameState()))return;
		Stage(37,.5);return;
	}
	if(T.Stage==37)
	{
		if(!Check(V.WindowArmed() && V.WindowPhase()>=1 && V.ScreenClosed(),TEXT("fresh disk/full Flow keeps the pending slap behind the closed shade")))return;
		Aim(FS47HorrorVisual::WindowPoint());Stage(38,.2);return;
	}
	if(T.Stage==38)
	{
		if(V.WindowPhase()!=2){if(T.Elapsed-T.Since>8)Fail(TEXT("pending slap never sounded"));return;}
		if(!Check(V.MarksPresent() && V.HandSeen() && V.ScreenClosed() && !H.Complete(),TEXT("the slap is heard behind the closed shade and leaves real exterior marks without completing the inspection")))return;
		Shot(TEXT("window-slap-shade-down"));
		Aim(Target(TEXT("horror:screen")));Stage(381,.3);return;
	}
	if(T.Stage==381)
	{
		if(!Pick(TEXT("horror:screen")))return;
		Aim(Target(TEXT("horror:marks")));Stage(379,1.3);return;
	}
	if(T.Stage==379)
	{
		// Screenshot requests are deferred until the frame renders. Keep the
		// actual marks in view until that capture has finished before looking away.
		Shot(TEXT("window-marks-before-check"));Stage(3791,.2);return;
	}
	if(T.Stage==3791)
	{
		// Looking at the marks alone never grants an inspection interaction.
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
		if(T.Mode==TEXT("SC02")){Stage(100,.5);return;}
		T.bDone=true;M->Player->TestMove.Reset();M->Player->bTestRun=false;
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
	// ---------------------------------------------------------------- SC02, meeting 1
	// Continues the same live case after the real SC01 contract. The field trip uses the
	// existing native autopilot like the T01 drive contract; foot movement uses the Walker,
	// every use an aimed camera ray, every acknowledgement focused Slate Enter. Driving
	// clear uses the same labelled autopilot API; manual steering and human play are UNVERIFIED.
	auto& Th=*H.Threat;
	auto Sc02=[&H](){const auto S=S47J::Obj(H.Save(),TEXT("sc02"));return S?S47J::Str(S,TEXT("phase")):FString();};
	auto Retry=[&H](){const auto S=S47J::Obj(H.Save(),TEXT("sc02"));return S && S47J::Bool(S,TEXT("retry"));};
	// Aim on one frame and use the ray on the next, as the SC01 stages do: the camera
	// forward vector follows the new yaw only after a tick.
	auto AimPick=[this,&T](const FString& Id,const FVector& At,bool bUse)
	{
		if(T.Aimed!=Id){Aim(At);T.Aimed=Id;return false;}
		T.Aimed.Empty();return Pick(Id,FString(),bUse);
	};
	auto Node=[](const TCHAR* Id){for(const auto& N:FS47HorrorThreat::Nodes())if(FCString::Strcmp(N.Id,Id)==0)return FVector(N.At.X,0,N.At.Y);return FVector::ZeroVector;};
	if(T.Stage==100)
	{
		if(!Check(H.ContentRevision()>=2 && Th.Ready() && Sc02()==TEXT("t01"),TEXT("revision 2 continues from completed SC01 into the SC02 T01 phase with the native pale body built")))return;
		M->UI->Close(true);M->World->Enter(TEXT("saro"));M->Player->Place(10.8,8,0);M->Player->Pitch=-.08;
		UE_LOG(LogS47HorrorTest,Display,TEXT("S47: SC02 START: one labelled placement beside the parked SARO truck; native autopilot to Caliche thereafter"));
		Stage(101,.5);return;
	}
	if(T.Stage==101)
	{
		if(!AimPick(TEXT("truck"),Target(TEXT("truck")),false))return;
		M->World->Autopilot=10;M->World->DriveOut(false);Stage(102,.6);return;
	}
	if(T.Stage==102)
	{
		if(!M->World->bDriving){Fail(TEXT("outbound native trip ended before Caliche stop"));return;}
		const auto* Truck=M->World->CabTruck();
		if(M->World->Leg==TEXT("road") && Truck->Position.X<7950)M->World->Autopilot=4;
		if(M->World->Leg!=TEXT("road") || Truck->Position.X>7900)return;
		M->World->Autopilot=0;Stage(103,.1);return;
	}
	if(T.Stage==103)
	{
		if(!M->World->bDriving || T.Elapsed-T.Since>15){Fail(TEXT("native approach braking failed"));return;}
		if(FMath::Abs(M->World->DrivingSpeed())>.1 || !M->World->CanGetOut())return;
		T.FieldTruck=M->World->CabTruck()->Position;
		if(!Check(M->World->GetOutHere(),TEXT("stopped SARO truck at Caliche permits normal GetOutHere")))return;
		Stage(104,.65);return;
	}
	if(T.Stage==104)
	{
		T.FieldEgress=M->Player->Pos;
		if(!Check(!Th.Active() && Sc02()==TEXT("t01"),TEXT("no pale body before the T01 result is secured")))return;
		const auto A=M->Substation->ArcSnapshot();
		if(!Check(A.Bursts==0 && !A.bNiagaraActive && !A.bMeshVisible && A.FlashLumens==0 && A.MaterialIntensity==0,
			TEXT("arc geometry, particles and flash remain inert before T01")))return;
		UE_LOG(LogS47HorrorTest,Display,TEXT("S47: SC02 parked truck %.2f %.2f, egress %.2f %.2f"),T.FieldTruck.X,T.FieldTruck.Z,T.FieldEgress.X,T.FieldEgress.Z);
		// The gravel shoulder east of the track is too steep to walk across; stay on the low
		// side, then go up the service track itself as the T01 drive contract does.
		Go({FVector(T.FieldEgress.X,0,FMath::Min(T.FieldEgress.Z,516.5)),FVector(7885,0,516.0),Node(TEXT("track")),Node(TEXT("front"))},105);return;
	}
	if(T.Stage==105)
	{
		if(H.Pack.CurrentScene()){Drain(105);return;}
		if(!M->Substation->HasJournal()){if(!AimPick(TEXT("substation:journal"),FS47Substation::Site(FVector(4.83,-8.99,.77)),true))return;M->UI->Close(true);return;}
		if(!AimPick(TEXT("substation:service"),FS47Substation::Site(FVector(5.52,-9.10,1.42)),true))return;
		Stage(106,.1);return;
	}
	if(T.Stage==106)
	{
		if(M->Inter->LabelOf(TEXT("substation:service"))!=TEXT("Service check complete / compare the lamp"))
		{if(T.Elapsed-T.Since>10)Fail(TEXT("real local service check did not complete"));return;}
		if(!AimPick(TEXT("substation:lamp"),FS47Substation::Site(FVector(4.62,-9.01,1.70)),true))return;
		M->UI->Close(true);
		if(!Check(M->Substation->HasNormalResult(),TEXT("actual panel and lamp comparison give the T01 normal result")))return;
		T.T01Stamp=M->Substation->NormalCompletionStamp();
		// Persist this real pre-meeting result for bounded High/Low/calm/off comparisons.
		if(!StoreRestore(TEXT("arc-pre-meeting"),M->Flow->GameState(),false))return;
		Stage(107,.4);return;
	}
	if(T.Stage==107)
	{
		// H02 presentation from the physical result; the meeting waits for every panel to close.
		if(H.Pack.CurrentScene()){if(Th.Active()){Fail(TEXT("pale body started during an open H02 scene"));return;}Drain(107);return;}
		if(!H.Pack.Flag(TEXT("hr_h02_recorded"))){if(T.Elapsed-T.Since>20)Fail(TEXT("H02 record was not presented after the real T01 result"));return;}
		if(Sc02()!=TEXT("encounter")){if(T.Elapsed-T.Since>25)Fail(TEXT("meeting 1 did not start after the presented T01 record"));return;}
		if(!Check(Th.Active() && M->Substation->HasNormalResult(),TEXT("meeting 1 starts only after the secured, presented T01 result")))return;
		T.SC02Clock=M->Story->Clock();Stage(108,0);return;
	}
	if(T.Stage==108)
	{
		// The body appears only at the unseen post; look away towards the yard, then back at the crossing.
		Aim(Node(TEXT("shed"))+FVector(0,1.4,0));
		if(!Th.Appeared()){if(T.Elapsed-T.Since>20)Fail(TEXT("pale body never appeared at its post"));return;}
		// Substation::Step runs after this controller; inspect the real appearance edge next frame.
		Stage(1080,0);return;
	}
	if(T.Stage>=1080 && T.Stage<=1087)
	{
		const auto A=M->Substation->ArcSnapshot();
		const bool Off=FParse::Param(FCommandLine::Get(),TEXT("S47FxOff")) || FParse::Param(FCommandLine::Get(),TEXT("S47ArcMissingAsset"));
		auto Record=[&](const TCHAR* Label)
		{
			auto R=S47J::New();S47J::Set(R,TEXT("arcReadback"),Label);S47J::Set(R,TEXT("elapsed"),T.Elapsed);
			S47J::Set(R,TEXT("age"),A.Age);S47J::Set(R,TEXT("niagaraAge"),A.NiagaraAge);S47J::Set(R,TEXT("bursts"),A.Bursts);
			S47J::Set(R,TEXT("sparkCount"),A.SparkCount);S47J::Set(R,TEXT("instances"),A.Instances);S47J::Set(R,TEXT("safe"),A.bSafe);
			S47J::Set(R,TEXT("niagaraActive"),A.bNiagaraActive);S47J::Set(R,TEXT("niagaraPaused"),A.bNiagaraPaused);
			S47J::Set(R,TEXT("meshVisible"),A.bMeshVisible);S47J::Set(R,TEXT("flashLumens"),A.FlashLumens);
			S47J::Set(R,TEXT("materialIntensity"),A.MaterialIntensity);S47J::Set(R,TEXT("lampLumens"),A.LampLumens);
			S47J::Set(R,TEXT("diagnosticCameraZoom"),T.bArcCloseup);S47J::Set(R,TEXT("cameraFov"),double(M->Player->Fov));
			T.Evidence.Add(MakeShared<FJsonValueObject>(R));
		};
		if(T.Stage==1080)
		{
			if(T.bArcCloseup)
			{
				T.ArcOriginalFov=M->Player->Fov;M->Player->Fov=30.f;
				UE_LOG(LogS47HorrorTest,Display,TEXT("S47: Arc diagnostic zoom: vertical FOV 30, unchanged player position, UI omitted from live screenshot; not the normal player view"));
			}
			Record(TEXT("appearance"));
			if(Off)
			{
				if(!Check(!A.bReady && A.Bursts==0 && !A.bNiagaraActive && !A.bMeshVisible && A.FlashLumens==0 && A.MaterialIntensity==0 && FMath::IsNearlyEqual(A.LampLumens,95.f,.02f),
					TEXT("disabled or missing Niagara asset leaves geometry, particles, flash and lamp dip wholly inert")))return;
				T.ArcBursts=A.Bursts;Aim(FS47Substation::Site(FVector(1.04,3.25,7.34)));Stage(1085,2.7);return;
			}
			const int32 Count=FS47Substation::ArcSparks(M->Flow->Graphics.Effects==0,M->Flow->bCalmFlash);
			if(!Check(A.bReady && A.bSafe && A.Bursts==T.ArcBursts+1 && A.SparkCount==Count && A.Instances==16 && A.bMaterialAssigned &&
				A.bNiagaraActive && A.bMeshVisible && A.bFlashVisible && A.FlashLumens>0 && A.MaterialIntensity>0,
				TEXT("first real SC02 appearance activates one safe native arc, assigned material, light and quality-scaled Niagara burst")))return;
			if(!Check(FMath::IsNearlyEqual(A.LampLumens,95.f*FS47Substation::ArcDip(A.Age,M->Flow->bCalmFlash),.05f),
				TEXT("actual marked lamp follows the arc dip without changing T01 state")))return;
			T.ArcBursts=A.Bursts;Aim(S47Three::ToThree(A.Origin)-FVector(0,.35,0));Stage(1081,.15);return;
		}
		if(T.Stage==1081)
		{
			if(T.bArcCloseup)FScreenshotRequest::RequestScreenshot(T.Root/TEXT("sc02-arc-diagnostic-zoom.png"),false,false);
			else Shot(TEXT("sc02-arc-live"));
			Stage(1082,0);return;
		}
		if(T.Stage==1082)
		{
			M->UI->Open(S47Kit::Shell(M->UI,TEXT("ISOLATED ARC PAUSE CONTROL"),S47Kit::Text(TEXT("The arc and particles must remain still while this panel is open."))));
			Stage(1087,0);return;
		}
		if(T.Stage==1087)
		{
			// Settle the prior Niagara frame, then measure the held simulation interval.
			T.PausedArc=A;Stage(1083,.35);return;
		}
		if(T.Stage==1083)
		{
			Record(TEXT("modal-held"));
			if(!Check(M->ModalOpen() && A.bNiagaraPaused && FMath::IsNearlyEqual(A.Age,T.PausedArc.Age,.0001) &&
				A.NiagaraAge>=0 && FMath::Abs(A.NiagaraAge-T.PausedArc.NiagaraAge)<.05 && A.Bursts==T.ArcBursts &&
				FMath::IsNearlyEqual(A.FlashLumens,T.PausedArc.FlashLumens,.01f),
				TEXT("actual document panel freezes arc age, Niagara simulation and flash without elapsed drift")))return;
			M->UI->Close(true);Stage(1084,.12);return;
		}
		if(T.Stage==1084)
		{
			Record(TEXT("modal-resumed"));
			if(!Check(!A.bNiagaraPaused && A.Age>T.PausedArc.Age && A.NiagaraAge>T.PausedArc.NiagaraAge && A.Bursts==T.ArcBursts,
				TEXT("closing the actual document resumes the same burst and both effect clocks")))return;
			Stage(1085,2.7);return;
		}
		if(T.Stage==1085)
		{
			if(T.ArcOriginalFov>0){M->Player->Fov=T.ArcOriginalFov;T.ArcOriginalFov=0;}
			Record(TEXT("finished"));
			if(Off)Shot(TEXT("sc02-arc-disabled"));
			if(!Check(A.Bursts==T.ArcBursts && !A.bNiagaraActive && !A.bMeshVisible && A.FlashLumens==0 && A.MaterialIntensity==0 && FMath::IsNearlyEqual(A.LampLumens,95.f,.02f),
				TEXT("arc and particle tail finish, restore the actual lamp, and do not replay while the meeting continues")))return;
			if(T.Mode!=TEXT("Arc")){Stage(109,.2);return;}
			if(!StoreRestore(TEXT("arc-after-appearance"),M->Flow->GameState()))return;
			Stage(1086,2.7);return;
		}
		if(T.Stage==1086)
		{
			Record(TEXT("restored-meeting"));
			if(!Check(A.Bursts==T.ArcBursts && !A.bNiagaraActive && !A.bMeshVisible && A.FlashLumens==0 && A.MaterialIntensity==0,
				TEXT("fresh disk/full Flow Continue inside the meeting remains quiet without an arc replay")))return;
			T.bDone=true;M->Player->TestMove.Reset();
			auto Result=S47J::New();S47J::Set(Result,TEXT("status"),TEXT("PASS"));S47J::Set(Result,TEXT("scenario"),TEXT("Arc"));
			S47J::Set(Result,TEXT("sourceSnapshot"),T.RestoreSource);S47J::Set(Result,TEXT("checks"),T.Checks);S47J::Set(Result,TEXT("diskFlowRestores"),T.Stores);
			S47J::Set(Result,TEXT("quality"),M->Flow->Quality);S47J::Set(Result,TEXT("calmFlash"),M->Flow->bCalmFlash);S47J::Set(Result,TEXT("disabledFixture"),Off);
			S47J::Set(Result,TEXT("diagnosticCameraZoom"),T.bArcCloseup);
			if(T.bArcCloseup)S47J::Set(Result,TEXT("imageNote"),TEXT("Diagnostic 30-degree vertical FOV at the unchanged player position with UI omitted; not the normal player view. Original FOV restored before Continue."));
			Result->SetArrayField(TEXT("evidence"),T.Evidence);
			S47J::Set(Result,TEXT("limits"),TEXT("Focused runtime VFX contract from the named real T01 snapshot. Actual SC02 appearance, component readbacks, document pause and isolated disk/Flow Continue run here. Prior SC01/T01/travel, human input, listening, particle-by-particle visual quality and whole-game performance are not asserted."));
			FFileHelper::SaveStringToFile(S47J::Write(Result,true),*(T.Root/TEXT("result.json")));
			UE_LOG(LogS47HorrorTest,Display,TEXT("S47: Horror Arc PASS COMPLETE: %d checks"),T.Checks);FPlatformMisc::RequestExit(false);return;
		}
		return;
	}
	if(T.Stage==109)
	{
		// It reacts to real sound only: shuffle sideways (actual footsteps) while watching it.
		const FVector2D P=Th.Position();Aim(FVector(P.X,M->Player->FloorY+1.5,P.Y));
		if(!H.Pack.CurrentScene() && Th.Seen()){M->Player->TestMove.Reset();Shot(TEXT("sc02-body-seen"));Stage(110,.6);return;}
		if(H.Pack.CurrentScene()){M->Player->TestMove.Reset();Drain(109);return;}
		M->Player->TestMove=FVector2D(FMath::Sin(T.Elapsed*1.2)>0?.35:-.35,0);
		if(FMath::Fmod(T.Elapsed,2.0)<.05)UE_LOG(LogS47HorrorTest,Display,TEXT("S47: SC02 watching body: state %s at %.1f %.1f"),FS47HorrorThreat::StateName(Th.State()),P.X,P.Y);
		if(T.Elapsed-T.Since>20){M->Player->TestMove.Reset();Fail(TEXT("rendered pale body never registered as seen through the real view test"));}
		return;
	}
	if(T.Stage==110)
	{
		if(!Check(Th.Seen() && Sc02()==TEXT("encounter"),TEXT("the player has actually seen the pale body; no escape credit yet")))return;
		Shot(TEXT("sc02-forewarning"));
		if(!StoreRestore(TEXT("sc02-mid-meeting"),M->Flow->GameState()))return;
		Stage(111,1.0);return;
	}
	if(T.Stage==1101)
	{
		const auto Restored = S47J::Obj(H.Save(), TEXT("sc02"));
		if(!Check(H.ContentRevision()>=2 && Th.Ready() && Th.Active() && Th.Seen() && Sc02()==TEXT("encounter") &&
			S47J::Bool(Restored,TEXT("threatSeen")) && !Retry() && M->Substation->HasNormalResult() &&
			M->Substation->NormalCompletionStamp()==T.T01Stamp && FMath::Abs(M->Story->Clock()-T.SC02Clock)<5,
			TEXT("real Flow Continue restores the witnessed SC02 meeting, its original T01 stamp and clock")))return;
		const FS47Truck* Truck=M->World->TruckHere();
		if(!Check(Truck && !M->World->bDriving && FVector2D::Distance(FVector2D(Truck->Position.X,Truck->Position.Z),FVector2D(T.FieldTruck.X,T.FieldTruck.Z))<.02,
			TEXT("Continue restores the actual parked truck from the source snapshot")))return;
		// The old egress is not stored in a game snapshot. Choose a real return target by
		// the restored truck's doors, using World::DoorSpot's geometry, never a placement.
		const double Co=FMath::Cos(Truck->Rotation.Y), Si=FMath::Sin(Truck->Rotation.Y), R=M->Player->Radius;
		bool Found=false;
		for(const double Side:{-1.0,1.0,-1.4,1.4})
		{
			const double X=Side*1.75, Z=-.3;
			const FVector At=Truck->Position+FVector(X*Co+Z*Si,0,-X*Si+Z*Co);
			if(!M->Player->Walkable(At.X,At.Z))continue;
			bool Blocked=false;
			for(const FS47Rect& C:M->Web->Colliders())
				if(C.bEnabled && C.Id!=TEXT("truck") && At.X+R>C.MinX && At.X-R<C.MaxX && At.Z+R>C.MinZ && At.Z-R<C.MaxZ){Blocked=true;break;}
			if(!Blocked){T.FieldEgress=At;Found=true;break;}
		}
		if(!Check(Found,TEXT("restored truck has a physically walkable, unblocked door approach for the focused escape")))return;
		UE_LOG(LogS47HorrorTest,Display,TEXT("S47: SC02Resume restored truck %.2f %.2f; actual door approach %.2f %.2f"),T.FieldTruck.X,T.FieldTruck.Z,T.FieldEgress.X,T.FieldEgress.Z);
		Stage(111,.2);return;
	}
	if(T.Stage==111)
	{
		const auto Arc=M->Substation->ArcSnapshot();
		if(!Check(Arc.Bursts==T.ArcBursts && !Arc.bNiagaraActive && !Arc.bMeshVisible && Arc.FlashLumens==0 && Arc.MaterialIntensity==0,
			TEXT("SC02 Continue/capture/escape preserves arc silence and the cumulative burst count")))return;
		const FVector2D At(M->Player->Pos.X,M->Player->Pos.Z);
		if(!Check(Sc02()==TEXT("encounter") && FVector2D::Distance(At,Th.Checkpoint())<1.0 && M->Substation->HasNormalResult() && M->Substation->NormalCompletionStamp()==T.T01Stamp,
			TEXT("Continue in the middle of the meeting resumes at the shed checkpoint with T01 kept, never in a half chase")))return;
		Go({Node(TEXT("westMid")),Node(TEXT("westCorner"))},112);return;
	}
	if(T.Stage==112)
	{
		// Exposure test: walk into the open crossing towards the body and stand there.
		// The retry response is the capture's visible result; read it before acknowledging it.
		if(Retry() || (H.Pack.CurrentScene() && H.Pack.CurrentScene()->Id==TEXT("SC02_RETRY"))){M->Player->TestMove.Reset();T.bCaptured=true;Stage(113,.3);return;}
		if(H.Pack.CurrentScene()){Drain(112);return;}
		const FVector2D P=Th.Position();const FVector D=FVector(P.X,0,P.Y)-M->Player->Pos;
		M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=-.04;
		M->Player->TestMove=FVector2D(0,FVector2D(D.X,D.Z).Size()>1.5?-.6:0);
		if(T.Elapsed-T.Since>60){M->Player->TestMove.Reset();Fail(TEXT("exposed player in the open was never captured within 60 s"));}
		return;
	}
	if(T.Stage==113)
	{
		const auto Arc=M->Substation->ArcSnapshot();
		if(!Check(Arc.Bursts==T.ArcBursts && !Arc.bNiagaraActive && !Arc.bMeshVisible && Arc.FlashLumens==0 && Arc.MaterialIntensity==0,
			TEXT("SC02 Continue/capture/escape preserves arc silence and the cumulative burst count")))return;
		M->Player->TestMove.Reset();
		const FVector2D At(M->Player->Pos.X,M->Player->Pos.Z);
		if(!Check(T.bCaptured && Sc02()==TEXT("encounter") && FVector2D::Distance(At,Th.Checkpoint())<1.0,TEXT("capture gives a local retry at the shed checkpoint")))return;
		if(!Check(M->Substation->HasNormalResult() && M->Substation->NormalCompletionStamp()==T.T01Stamp,TEXT("capture keeps the completed T01 result")))return;
		if(!Check(M->Story->Clock()-T.SC02Clock<600,TEXT("capture costs no lost time beyond the played seconds")))return;
		Shot(TEXT("sc02-retry"));Stage(114,.4);return;
	}
	if(T.Stage==114)
	{
		if(H.Pack.CurrentScene()){Drain(114);return;}
		if(!Check(T.bCaptured && !Retry(),TEXT("first deliberate capture is acknowledged before the running escape starts")))return;
		T.bSC02Escape=true;T.bSC02EscapeRouteDone=false;T.SC02RunMetres=0;
		// Route A uses the real run control: west outside the closed yard, then down the track to the truck.
		Go({Node(TEXT("westMid")),Node(TEXT("westCorner")),Node(TEXT("westTrack")),FVector(7885,0,516.0),FVector(T.FieldEgress.X,0,FMath::Min(T.FieldEgress.Z,516.5)),T.FieldEgress},115,true);return;
	}
	if(T.Stage==115)
	{
		if(!Check(T.bSC02EscapeRouteDone && T.SC02RunMetres>10,TEXT("escape runs more than ten measured metres on the native Walker route outside the closed yard")))return;
		if(!Check(!M->Player->bTestRun,TEXT("running input is released before interacting with the parked truck")))return;
		const FVector2D At(M->Player->Pos.X,M->Player->Pos.Z);
		if(!Check(FVector2D::Distance(At,FVector2D(T.FieldEgress.X,T.FieldEgress.Z))<1.5,TEXT("route A on foot reaches the parked truck outside the closed yard")))return;
		if(!AimPick(TEXT("dinerTruck"),Target(TEXT("dinerTruck")),false))return;
		M->World->Autopilot=10;M->World->DriveBack();Stage(116,.6);return;
	}
	if(T.Stage==116)
	{
		if(Sc02()==TEXT("escaped")){T.bSC02Escape=false;Stage(117,.2);return;}
		if(!M->World->bDriving && T.Elapsed-T.Since>3){Fail(TEXT("drive back never started"));return;}
		if(T.Elapsed-T.Since>90)Fail(TEXT("driving clear did not end meeting 1"));
		return;
	}
	if(T.Stage==117)
	{
		// The actual escape response may open during the return trip and hold driving.
		// Acknowledge it before waiting for the native Park; never bypass its host.
		if(H.Pack.CurrentScene()){Drain(117);return;}
		if(M->World->bDriving)
		{
			if(T.Elapsed-T.Since>180)Fail(TEXT("native return trip did not finish parking at SARO within 180 s after the escape response"));
			return;
		}
		const auto S=S47J::Obj(H.Save(),TEXT("sc02"));
		if(!Check(H.SC02Escaped() && S && !S47J::Str(S,TEXT("escapeStamp")).IsEmpty() && S47J::Bool(S,TEXT("threatSeen")),TEXT("driving clear after a real sighting is an escape with its stamp")))return;
		Stage(118,.8);return;
	}
	if(T.Stage==118)
	{
		if(H.Pack.CurrentScene()){Drain(118);return;}
		if(!Check(H.Pack.Flag(TEXT("hr_sc02_escape_presented")),TEXT("SC02_ESCAPE is presented after the escape, not before")))return;
		if(!StoreRestore(TEXT("sc02-escaped"),M->Flow->GameState()))return;
		Stage(119,.6);return;
	}
	if(T.Stage==119)
	{
		const auto Arc=M->Substation->ArcSnapshot();
		if(!Check(Arc.Bursts==T.ArcBursts && !Arc.bNiagaraActive && !Arc.bMeshVisible && Arc.FlashLumens==0 && Arc.MaterialIntensity==0,
			TEXT("SC02 Continue/capture/escape preserves arc silence and the cumulative burst count")))return;
		if(!Check(H.SC02Escaped() && M->Substation->HasNormalResult() && !Th.Active(),TEXT("escaped meeting and T01 survive Continue without restarting the body")))return;
		if(T.Mode==TEXT("SC03")){Stage(200,.5);return;}
		T.bDone=true;M->Player->TestMove.Reset();M->Player->bTestRun=false;
		auto Result=S47J::New();S47J::Set(Result,TEXT("status"),TEXT("PASS"));S47J::Set(Result,TEXT("scenario"),T.Mode==TEXT("SC02Resume")?TEXT("SC02Resume"):TEXT("SC02"));
		if(T.Mode==TEXT("SC02Resume"))S47J::Set(Result,TEXT("sourceSnapshot"),T.RestoreSource);
		S47J::Set(Result,TEXT("checks"),T.Checks);S47J::Set(Result,TEXT("diskFlowRestores"),T.Stores);S47J::Set(Result,TEXT("nativeWalkMetres"),T.WalkMetres);
		S47J::Set(Result,TEXT("captured"),T.bCaptured);S47J::Set(Result,TEXT("nativeRunMetres"),T.SC02RunMetres);Result->SetArrayField(TEXT("evidence"),T.Evidence);
		S47J::Set(Result,TEXT("limits"),T.Mode==TEXT("SC02Resume")?
			TEXT("Focused Continue contract from the named actual snapshot through isolated disk reader and full Flow restore. The source's SC01, T01 and outbound trip are retained evidence, not rerun. Capture, running escape, native drive back and escaped Continue run here; the truck approach derives from its restored physical pose. This is not full SC02 acceptance, human play, manual steering, listening or route B verification."):
			TEXT("Scripted live contract on top of the SC01 contract. Field trip and drive back use the existing native autopilot API like the T01 drive contract; route B, manual steering, human play, listening and fear are UNVERIFIED."));
		FFileHelper::SaveStringToFile(S47J::Write(Result,true),*(T.Root/TEXT("result.json")));
		UE_LOG(LogS47HorrorTest,Display,TEXT("S47: Horror %s PASS COMPLETE: %d checks, %d disk/Flow restores, %.2f m native walking, captured %s"),T.Mode==TEXT("SC02Resume")?TEXT("SC02Resume"):TEXT("SC02"),T.Checks,T.Stores,T.WalkMetres,T.bCaptured?TEXT("yes"):TEXT("no"));
		FPlatformMisc::RequestExit(false);return;
	}

	// ---------------------------------------------------------------- SC03, the help that came
	// From the escaped SC02 case: native autopilot to Caliche, the real payphone and its call
	// lines through focused Slate Enter, waiting in view, the identification, looking away (no
	// observation), Continue mid-lift, the actual sight, cover, traces, the missing report and
	// the deputy's question. Facts are read back only from the host; nothing is assigned here.
	if(T.Stage<200 || (T.Stage>=300 && (T.Stage<2000 || T.Stage>=3000)))return;
	if(!H.SC03){Fail(TEXT("SC03 module missing"));return;}
	auto& S3=*H.SC03;
	using EP=FS47HorrorSC03::EPhase;
	auto Phase3=[&S3](){return FString(FS47HorrorSC03::PhaseName(S3.Phase()));};
	auto Pick3=[this,&T](const FString& Id,const FVector& At,bool bUse)
	{
		if(T.Aimed!=Id){Aim(At);T.Aimed=Id;return false;}
		T.Aimed.Empty();return Pick(Id,FString(),bUse);
	};
	auto CallDrain=[this,&T,&H,&Enter]()
	{
		// A world call is not a pack scene: read its exact line and wait its duration, then Enter.
		const auto Saved=H.Save();
		if(!S47J::Bool(Saved,TEXT("callActive")))return false;
		const FString Scene=S47J::Str(Saved,TEXT("callScene")).IsEmpty()?FString(TEXT("SC01_FIRST_REPORT")):S47J::Str(Saved,TEXT("callScene"));
		const int32 Cursor=S47J::Int(Saved,TEXT("callCursor"));const auto* S=H.Pack.FindScene(Scene);
		if(!S || !S->Lines.IsValidIndex(Cursor)){Fail(TEXT("world call has no line: ")+Scene);return true;}
		const FString Key=Scene+FString::FromInt(Cursor);
		if(T.LastLine!=Key){T.LastLine=Key;T.LineAt=T.Elapsed;Shot(TEXT("sc03-call-")+Key);return true;}
		if(T.Elapsed-T.LineAt<FMath::Max(.5,S->Lines[Cursor].Duration+1))return true;
		if(!M->ModalOpen()){H.Use(TEXT("town:payphone"));return true;}
		if(!Enter())return true;T.LastLine.Empty();T.Hold=.25;return true;
	};
	auto Payphone=[this](){return Target(TEXT("town:payphone"));};
	auto Front=[this](const FVector& At,double D){FVector V=At-M->Player->Pos;V.Y=0;V=V.GetSafeNormal();return FVector(At.X-V.X*D,0,At.Z-V.Z*D);};
	if(T.Stage==200)
	{
		if(H.Pack.CurrentScene()){Drain(200);return;}
		if(S3.Phase()!=EP::Report){if(T.Elapsed-T.Since>10)Fail(TEXT("SC03 report was not owed after the presented SC02 escape: ")+Phase3());return;}
		if(!Check(H.ContentRevision()>=3 && !S3.Facts().bDispatch && !M->Story->Docs().ContainsByPredicate([](const FS47Doc& D){return D.Id==TEXT("HR_DISPATCH_RECEIPT");}),
			TEXT("revision 3: the escape presented makes the Caliche report owed, with no receipt yet")))return;
		M->UI->Close(true);
		if(M->World->bDriving){Fail(TEXT("expected to start SC03 on foot at SARO"));return;}
		Stage(201,.4);return;
	}
	if(T.Stage==201)
	{
		if(!Pick3(TEXT("truck"),Target(TEXT("truck")),false))return;
		UE_LOG(LogS47HorrorTest,Display,TEXT("S47: SC03 drive to Caliche with the existing native autopilot (labelled API target, like the T01 drive contract)"));
		M->World->Autopilot=10;M->World->DriveToDiner(nullptr);Stage(202,1.0);return;
	}
	if(T.Stage==202)
	{
		if(H.Pack.CurrentScene()){Drain(202);return;}
		if(M->World->bDriving){if(T.Elapsed-T.Since>420)Fail(TEXT("native drive to Caliche did not park within 420 s"));return;}
		if(!Check(M->World->Area==TEXT("diner"),TEXT("native drive parked at Caliche")))return;
		const FVector P=Payphone();
		UE_LOG(LogS47HorrorTest,Display,TEXT("S47: SC03 at Caliche: player %.2f %.2f, payphone %.2f %.2f, counter wagon park %.2f %.2f"),M->Player->Pos.X,M->Player->Pos.Z,P.X,P.Z,S3.WagonPoint().X,S3.WagonPoint().Z);
		Go({Front(P,1.1)},203);return;
	}
	if(T.Stage==203)
	{
		if(!Pick3(TEXT("town:payphone"),Payphone(),true))return;
		Stage(204,.3);return;
	}
	if(T.Stage==204)
	{
		if(CallDrain())return;
		if(H.Pack.CurrentScene()){Drain(204);return;}
		if(!Check(S3.Facts().bDispatch && S3.Facts().bAssigned && S3.Phase()==EP::Wait && H.Pack.Flag(TEXT("hr_sc03_relief_contact_presented")),
			TEXT("the completed dispatch and Ward calls give the readback, the assignment and the wait")))return;
		int32 N=0;for(const auto& D:M->Story->Docs())if(D.Id==TEXT("HR_DISPATCH_RECEIPT"))++N;
		if(!Check(N==1,TEXT("one dispatch receipt filed with the completed call")))return;
		Stage(205,.3);return;
	}
	if(T.Stage==205)
	{
		// Wait in view of the counter; the wagon may only come while Reyes is at the fuel stop.
		if(S3.Phase()==EP::Arrival){Shot(TEXT("sc03-wagon-arriving"));if(!StoreRestore(TEXT("sc03-mid-arrival"),M->Flow->GameState()))return;Stage(206,1.0);return;}
		if(S3.Phase()==EP::Meet){Stage(207,.5);return;}
		if(T.Elapsed-T.Since>60)Fail(TEXT("wagon twenty-three did not come while waiting at the fuel stop: ")+Phase3());
		return;
	}
	if(T.Stage==206)
	{
		if(!Check(S3.Phase()==EP::Meet,TEXT("Continue during the arrival restores a parked wagon and Ortiz, no replayed drive")))return;
		Stage(207,.3);return;
	}
	if(T.Stage==207)
	{
		// Ortiz stands at the driver's side; reach her from her own side, around the wagon's solid body.
		const FVector O=S3.OrtizPoint(),Wg=S3.WagonPoint();FVector Out=O-Wg;Out.Y=0;Out=Out.GetSafeNormal();
		const FVector Wide(O.X+Out.X*3.2,0,O.Z+Out.Z*3.2),Near(O.X+Out.X*1.5,0,O.Z+Out.Z*1.5);
		const FVector Mid=(M->Player->Pos+Wide)*.5;FVector Side=Mid-Wg;Side.Y=0;
		const FVector Around=Wg+Side.GetSafeNormal()*4.5;
		UE_LOG(LogS47HorrorTest,Display,TEXT("S47: SC03 approach Ortiz %.2f %.2f from her side via %.2f %.2f"),O.X,O.Z,Wide.X,Wide.Z);
		Go({FVector(Around.X,0,Around.Z),Wide,Near},2071);return;
	}
	if(T.Stage==2071)
	{
		Aim(S3.OrtizPoint());Stage(208,.4);return;
	}
	if(T.Stage==208)
	{
		if(T.Aimed.IsEmpty())Shot(TEXT("sc03-ortiz"));
		if(!Pick3(TEXT("horror:ortiz"),S3.OrtizPoint(),true))return;
		Stage(209,.3);return;
	}
	if(T.Stage==209)
	{
		if(H.Pack.CurrentScene()){Drain(209);return;}
		if(!Check(S3.Facts().bIdentified && !S3.Facts().bObserved && H.Pack.Flag(TEXT("hr_sc03_ortiz_identify_presented")),TEXT("physical identification and Ortiz's lines, no lift yet")))return;
		// Look away before the light: the lift must not count while it is not seen.
		const FVector O=S3.OrtizPoint();Aim(M->Player->Pos*2-FVector(O.X,M->Player->Pos.Y*2-O.Y,O.Z)+FVector(0,0,0));
		Stage(210,.1);return;
	}
	if(T.Stage==210)
	{
		const FVector O=S3.OrtizPoint();const FVector Back(2*M->Player->Pos.X-O.X,O.Y,2*M->Player->Pos.Z-O.Z);Aim(Back);
		if(T.Elapsed-T.Since<12)return;
		if(!Check(S3.Phase()==EP::Lift && S3.LightOn() && !S3.Facts().bObserved,TEXT("looking away during the light gives no observed lift; she hangs in the light")))return;
		Shot(TEXT("sc03-looking-away"));
		if(!StoreRestore(TEXT("sc03-mid-lift-unseen"),M->Flow->GameState()))return;
		Stage(211,1.0);return;
	}
	if(T.Stage==211)
	{
		if(!Check(S3.Phase()==EP::Lift && S3.Facts().bIdentified && !S3.Facts().bObserved,TEXT("Continue during an unseen lift resumes the lift, never a fabricated sighting")))return;
		Stage(212,.2);return;
	}
	if(T.Stage==212)
	{
		Aim(S3.OrtizPoint());
		if(S3.Phase()==EP::Retreat){Shot(TEXT("sc03-taken"));Stage(213,.8);return;}
		if(T.Elapsed-T.Since>25)Fail(TEXT("the lift was never observed while looking at it"));
		return;
	}
	if(T.Stage==213)
	{
		if(!Check(S3.Facts().bObserved && !S3.Facts().bRetreat && !S3.Facts().TakenStamp.IsEmpty(),TEXT("the actual sight gives the observed lift and its stamp")))return;
		const auto Heads=S3.FigureHeads();if(Heads.Num())Aim(Heads[0]);
		UE_LOG(LogS47HorrorTest,Display,TEXT("S47: SC03 figures at %.2f %.2f / %.2f %.2f / %.2f %.2f"),Heads.Num()>0?Heads[0].X:0.,Heads.Num()>0?Heads[0].Z:0.,Heads.Num()>1?Heads[1].X:0.,Heads.Num()>1?Heads[1].Z:0.,Heads.Num()>2?Heads[2].X:0.,Heads.Num()>2?Heads[2].Z:0.);
		Stage(214,.6);return;
	}
	if(T.Stage==214)
	{
		Shot(TEXT("sc03-figures"));
		// Cover inside the fuel station: its real door faces east; go round to it on the low side.
		FS47Rect Door,Zone;bool bDoor=false,bZone=false;
		for(const auto& C:M->Web->Colliders())if(C.Id==TEXT("door:gasDoor")){Door=C;bDoor=true;}
		for(const auto& Z:M->Web->Zones())if(Z.Id==TEXT("prop:gasStation")){Zone=Z;bZone=true;}
		if(!bDoor || !bZone){Fail(TEXT("fuel station door or interior zone not found"));return;}
		const double DX=(Door.MinX+Door.MaxX)/2,DZ=(Door.MinZ+Door.MaxZ)/2,Out=DX>(Zone.MinX+Zone.MaxX)/2?1:-1;
		T.Door3=FVector(DX,0,DZ);T.DoorOut=Out;
		UE_LOG(LogS47HorrorTest,Display,TEXT("S47: SC03 retreat to the station door %.2f %.2f, outside to %s"),DX,DZ,Out>0?TEXT("east"):TEXT("west"));
		Go({FVector(DX+Out*2.2,0,Zone.MinZ-1.0),FVector(DX+Out*1.3,0,DZ)},215,true);return;
	}
	if(T.Stage==215){if(!AS47Props::IsDoorOpen(TEXT("gasDoor"))){if(!Pick3(TEXT("town:gasDoor"),Target(TEXT("town:gasDoor")),true))return;}Stage(216,.8);return;}
	if(T.Stage==216)
	{
		Go({FVector(T.Door3.X-T.DoorOut*2.6,0,T.Door3.Z)},2161);return;
	}
	if(T.Stage==2161)
	{
		if(!S3.Facts().bRetreat){if(T.Elapsed-T.Since>8)Fail(TEXT("no safe retreat registered inside the station"));return;}
		if(H.Pack.CurrentScene()){Drain(2161);return;}
		if(!Check(H.Pack.Flag(TEXT("hr_sc03_helper_taken_presented")) && S3.Phase()==EP::Calm,TEXT("cover inside the station, then the observation text")))return;
		Stage(217,.4);return;
	}
	if(T.Stage==217)
	{
		// Out the same east door and round the low side, then to the wagon.
		const FVector Wg=S3.WagonPoint();const double DX=T.Door3.X,DZ=T.Door3.Z,Out=T.DoorOut;
		FS47Rect Zone;for(const auto& Z:M->Web->Zones())if(Z.Id==TEXT("prop:gasStation"))Zone=Z;
		Go({FVector(DX-Out*.6,0,DZ),FVector(DX+Out*1.3,0,DZ),FVector(DX+Out*2.2,0,Zone.MinZ-1.0),Front(Wg,2.6)},218);return;
	}
	if(T.Stage==218){if(!Pick3(TEXT("horror:wagon23"),Target(TEXT("horror:wagon23")),true))return;Stage(219,.3);return;}
	// The thermos and the marks lie where she stood, several metres from the wagon: walk up to each.
	if(T.Stage==219){M->UI->Close(true);Go({Front(Target(TEXT("horror:thermos")),1.3)},2191);return;}
	if(T.Stage==2191){if(!Pick3(TEXT("horror:thermos"),Target(TEXT("horror:thermos")),true))return;Stage(220,.3);return;}
	if(T.Stage==220){M->UI->Close(true);Go({Front(Target(TEXT("horror:gravelMarks")),1.3)},2201);return;}
	if(T.Stage==2201){if(!Pick3(TEXT("horror:gravelMarks"),Target(TEXT("horror:gravelMarks")),true))return;Stage(221,.6);return;}
	if(T.Stage==221)
	{
		if(H.Pack.CurrentScene()){Drain(221);return;}
		if(!Check(S3.TracesChecked() && H.Pack.Flag(TEXT("hr_sc03_helper_traces_presented")),TEXT("the wagon, the thermos and the marks examined; the traces note presented")))return;
		Go({Front(Payphone(),1.1)},222);return;
	}
	if(T.Stage==222){if(!Pick3(TEXT("town:payphone"),Payphone(),true))return;Stage(223,.3);return;}
	if(T.Stage==223)
	{
		if(CallDrain())return;
		if(H.Pack.CurrentScene()){Drain(223);return;}
		if(!Check(S3.Facts().bMissing && S3.Facts().bLocalOrder && S3.Phase()==EP::Deputy,TEXT("the missing-person report and Ward's local hold, then the deputy is coming")))return;
		if(!StoreRestore(TEXT("sc03-after-missing"),M->Flow->GameState()))return;
		Stage(224,1.0);return;
	}
	if(T.Stage==224)
	{
		if(!S3.Facts().bDeputyHere){if(T.Elapsed-T.Since>30)Fail(TEXT("the deputy never reached the fuel stop"));return;}
		Shot(TEXT("sc03-deputy"));
		Go({Front(Target(TEXT("horror:deputy")),1.5)},225);return;
	}
	if(T.Stage==225){if(!Pick3(TEXT("horror:deputy"),Target(TEXT("horror:deputy")),true))return;Stage(226,.3);return;}
	if(T.Stage==226)
	{
		if(H.Pack.CurrentScene()){Drain(226);return;}
		if(S3.Phase()!=EP::Complete){if(T.Elapsed-T.Since>5)Fail(TEXT("the deputy's question did not complete SC03"));return;}
		if(!StoreRestore(TEXT("sc03-complete"),M->Flow->GameState()))return;
		Stage(227,1.0);return;
	}
	if(T.Stage==227)
	{
		int32 Docs=0;for(const auto& D:M->Story->Docs())if(D.Id.StartsWith(TEXT("HR_DISPATCH")) || D.Id.StartsWith(TEXT("HR_MISSING")) || D.Id.StartsWith(TEXT("HR_LOCAL_HOLD")) || D.Id.StartsWith(TEXT("HR_HELPER_TRACE")))++Docs;
		if(!Check(S3.Complete() && Docs==4,TEXT("completed SC03 and its four papers survive Continue, each once")))return;
		T.bDone=true;M->Player->TestMove.Reset();
		auto Result=S47J::New();S47J::Set(Result,TEXT("status"),TEXT("PASS"));S47J::Set(Result,TEXT("scenario"),T.Mode);
		S47J::Set(Result,TEXT("checks"),T.Checks);S47J::Set(Result,TEXT("diskFlowRestores"),T.Stores);S47J::Set(Result,TEXT("nativeWalkMetres"),T.WalkMetres);Result->SetArrayField(TEXT("evidence"),T.Evidence);
		S47J::Set(Result,TEXT("limits"),TEXT("Scripted live contract. The drive to Caliche uses the native autopilot API; the cab retreat uses FreeDrive without moving. Retreat inside the station, manual steering, human play, listening and the final Ortiz/deputy assets are UNVERIFIED."));
		FFileHelper::SaveStringToFile(S47J::Write(Result,true),*(T.Root/TEXT("result.json")));
		UE_LOG(LogS47HorrorTest,Display,TEXT("S47: Horror %s PASS COMPLETE: %d checks, %d disk/Flow restores, %.2f m native walking"),*T.Mode,T.Checks,T.Stores,T.WalkMetres);
		FPlatformMisc::RequestExit(false);return;
	}

}

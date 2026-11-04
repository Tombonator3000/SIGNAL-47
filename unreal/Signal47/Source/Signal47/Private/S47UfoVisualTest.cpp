#include "S47UfoVisualTest.h"

#include "S47Album.h"
#include "S47FieldCamera.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Startup.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

bool FS47UfoVisualTest::Check(bool Good, const FString& Text)
{
	++Checks;
	auto O = S47J::New(); S47J::Set(O, TEXT("check"), Text); S47J::Set(O, TEXT("pass"), Good); S47J::Set(O, TEXT("stage"), Stage);
	Evidence.Add(MakeShared<FJsonValueObject>(O));
	UE_LOG(LogTemp, Display, TEXT("S47 UfoVisual %s stage%d: %s"), Good ? TEXT("PASS") : TEXT("FAIL"), Stage, *Text);
	if (!Good) Finish(false, Text);
	return Good;
}
void FS47UfoVisualTest::Go(int32 Next) { Stage = Next; StageAt = FPlatformTime::Seconds(); }
int32 FS47UfoVisualTest::VisibleMeshes() const
{
	if (!M || !M->Web) return 0;
	TArray<UStaticMeshComponent*> Parts; M->Web->GetComponents(Parts);
	int32 N = 0;
	for (auto* C : Parts) if (IsValid(C) && C->GetName().StartsWith(TEXT("S47Ufo")) && C->IsRegistered() && C->IsVisible() && !C->bHiddenInGame) ++N;
	return N;
}
int32 FS47UfoVisualTest::ActiveLights() const
{
	if (!M || !M->Web) return 0;
	TArray<ULightComponent*> Lights; M->Web->GetComponents(Lights);
	int32 N = 0;
	for (auto* L : Lights) if (IsValid(L) && L->GetName().StartsWith(TEXT("S47Ufo")) && L->IsRegistered() && L->IsVisible() && L->Intensity > .001) ++N;
	return N;
}
void FS47UfoVisualTest::Capture(const FString& Name)
{
	FScreenshotRequest::RequestScreenshot(Root / (Name + TEXT(".png")), true, false);
	auto O = S47J::New(); S47J::Set(O, TEXT("screenshot"), Name);
	S47J::Set(O, TEXT("eventAge"), M->Freeform->Director->EventAge());
	S47J::Set(O, TEXT("clock"), M->Freeform->Clock());
	S47J::Set(O, TEXT("visibleMeshes"), VisibleMeshes()); S47J::Set(O, TEXT("activeLights"), ActiveLights());
	S47J::Set(O, TEXT("low"), M->Web->GetGraphicsSettings().Effects == 0);
	S47J::Set(O, TEXT("calm"), M->Flow->bCalmFlash);
	const FVector Eye = S47Three::ToThree(M->Player->EyeUE());
	S47J::Set(O, TEXT("cameraEye"), TArray<double>{ Eye.X, Eye.Y, Eye.Z });
	S47J::Set(O, TEXT("cameraYawPitch"), TArray<double>{ M->Player->Yaw, M->Player->Pitch });
	if (M->FieldCamera)
	{
		M->FieldCamera->Sync();
		TArray<UStaticMeshComponent*> Parts; M->Web->GetComponents(Parts);
		for (auto* C : Parts) if (IsValid(C) && C->GetName().StartsWith(TEXT("S47UfoHull")) && C->IsVisible())
		{
			const FVector At = S47Three::ToThree(C->Bounds.Origin);
			const auto Q = M->FieldCamera->Project(At);
			S47J::Set(O, TEXT("hullPosition"), TArray<double>{ At.X, At.Y, At.Z });
			S47J::Set(O, TEXT("hullPhotoUV"), TArray<double>{ Q.U, Q.V });
			S47J::Set(O, TEXT("hullInFront"), Q.bFront);
			break;
		}
	}
	Evidence.Add(MakeShared<FJsonValueObject>(O));
}
void FS47UfoVisualTest::Finish(bool Good, const FString& Why)
{
	if (bDone) return;
	bDone = true;
	if (M && M->bPausedByMenu && M->UI) M->UI->Close();
	auto O = S47J::New(); S47J::Set(O, TEXT("status"), Good ? TEXT("PASS") : TEXT("FAIL"));
	S47J::Set(O, TEXT("reason"), Why); S47J::Set(O, TEXT("checks"), Checks);
	S47J::Set(O, TEXT("method"), TEXT("Isolated /tmp developer case 0. Ordinary Flow starts Freeform; explicit owned-camera fixture, night, outdoor camera placement and Director Force(close_pass). Camera pickup is outside this presentation test's scope. Native screenshots, one actual album exposure, normal pause/resume and scene exit. No synthetic photo, claim, reward or event-time advancement."));
	S47J::Set(O, TEXT("limits"), TEXT("This is a short presentation and lifecycle smoke test, not natural scheduling, driving input, full abduction/finale regression or human visual acceptance."));
	O->SetArrayField(TEXT("evidence"), Evidence);
	if (!Root.IsEmpty()) FFileHelper::SaveStringToFile(S47J::Write(O, true), *(Root / TEXT("result.json")));
	FPlatformMisc::RequestExitWithStatus(false, Good ? 0 : 1);
}
void FS47UfoVisualTest::Step(double Dt)
{
	(void)Dt;
	if (bDone) return;
	const double Now = FPlatformTime::Seconds();
	if (!bInit)
	{
		bInit = true;
		if (!FParse::Param(FCommandLine::Get(), TEXT("S47UfoVisualTest"))) { bDone = true; return; }
		FString User; int32 Case = -1;
		FParse::Value(FCommandLine::Get(), TEXT("UserDir="), User); FParse::Value(FCommandLine::Get(), TEXT("S47Case="), Case);
		User = FPaths::ConvertRelativePathToFull(User); FPaths::CollapseRelativeDirectories(User);
		if (!User.StartsWith(TEXT("/tmp/")) || Case != 0) { Finish(false, TEXT("explicit /tmp UserDir and -S47Case=0 required")); return; }
		Root = User / TEXT("UfoVisualTest"); IFileManager::Get().MakeDirectory(*Root, true);
		WallStart = StageAt = Now;
	}
	if (Now - WallStart > 145) { Finish(false, TEXT("bounded native visual test timeout")); return; }
	if (!M || !M->Flow || M->Flow->bStarting || !M->Web || !M->Web->IsLoaded() || !M->Player || !M->World || !M->UI || S47Startup::Blocking()) return;
	if (Stage == 0)
	{
		US47Flow::FStart Start; Start.bDev = true; Start.bFreeform = true; Start.CaseId = 0;
		M->Flow->StartGame(Start); Go(1); return;
	}
	if (!M->IsFreeform() || !M->Freeform || !M->Freeform->Active() || !M->Freeform->Director) return;
	auto& F = *M->Freeform; auto& D = *F.Director;
	if (Stage == 1)
	{
		if (Now - StageAt < 2 || M->World->bBusy) return;
		if (!Check(M->Flow->bDevRun && M->Flow->CaseId == 0, TEXT("ordinary Flow created an isolated Freeform developer case"))) return;
		if (!Check(M->FieldCamera != nullptr, TEXT("production field camera exists for explicit ownership fixture"))) return;
		// This isolated presentation test does not exercise the photo-lab pickup route.
		M->FieldCamera->bHave = true;
		if (!Check(M->FieldCamera->bHave, TEXT("explicit owned-camera fixture; physical pickup is outside this test's scope"))) return;
		const double Hour = FMath::Fmod(F.Clock(), 86400.);
		if (!Check(F.TestAdvance(FMath::Fmod(22 * 3600. - Hour + 86400., 86400.)), TEXT("explicit world-clock fixture reaches 22:00 without advancing encounter time"))) return;
		M->UI->Close(true); M->Player->Place(32, 28, 0);
		Go(10); return;
	}
	if (Stage == 10)
	{
		// Place changes walking state only. The actual actor/eye transform is
		// applied by Walker::Tick; never aim using the previous room's EyeUE.
		if (Now - StageAt < .35) return;
		const FVector Eye = S47Three::ToThree(M->Player->EyeUE());
		if (!Check(FMath::IsNearlyEqual(Eye.X, 32., .1) && FMath::IsNearlyEqual(Eye.Z, 28., .1), TEXT("native walker eye has reached the fixed outdoor camera position"))) return;
		if (!Check(D.Force(TEXT("close_pass")), TEXT("normal Director Force starts close_pass"))) return;
		FixedAnchor = D.EventAnchor();
		const FVector Aim = FixedAnchor + FVector(26 * 2.3, -6, 0) - Eye;
		M->Player->Yaw = FMath::Atan2(-Aim.X, -Aim.Z); M->Player->Pitch = FMath::Atan2(Aim.Y, FVector2D(Aim.X, Aim.Z).Size());
		BudgetBefore = F.Budget(); Go(2); return;
	}
	if (Stage == 2)
	{
		if (D.EventAge() < 12) return;
		if (!Check(D.PhysicalObjectPresented() && VisibleMeshes() >= 8 && ActiveLights() >= 1, TEXT("red approach has native craft, lamps and actual scene lighting"))) return;
		if (!Check(D.EventAnchor().Equals(FixedAnchor, .001), TEXT("flight origin remains fixed in world space"))) return;
		Capture(TEXT("01-red-approach")); Go(3); return;
	}
	if (Stage == 3)
	{
		if (D.EventAge() < 26) return;
		if (!Check(D.PhysicalObjectPresented() && M->Night && M->Night->Album && M->FieldCamera, TEXT("near pass remains rendered with production camera and album available"))) return;
		Capture(TEXT("02-near-pass")); Go(4); return;
	}
	if (Stage == 4)
	{
		if (Now - StageAt < .35) return;
		M->FieldCamera->Raise(true); M->FieldCamera->Sync();
		const auto Photo = M->Night->Album->Expose(); M->FieldCamera->Raise(false);
		if (!Check(!Photo.bUnavailable && !Photo.Url.IsEmpty() && D.PhotoClaims().Num() == 1 && F.Budget() == BudgetBefore,
			TEXT("actual rendered near-pass exposure retains one unpaid incident claim without shutter credit"))) return;
		const auto& Claim = D.PhotoClaims()[0];
		if (!Check(Photo.Targets.ContainsByPredicate([&](const TPair<FString, FVector2D>& T) { return T.Key == TEXT("freeform:") + Claim.Id; }),
			TEXT("retained photograph carries the actual native incident target"))) return;
		auto Graphics = M->Web->GetGraphicsSettings(); Graphics.Effects = 0; M->Web->ApplyGraphicsSettings(Graphics); M->Flow->bCalmFlash = true;
		Go(5); return;
	}
	if (Stage == 5)
	{
		if (Now - StageAt < 2) return;
		if (!Check(D.PhysicalObjectPresented() && ActiveLights() >= 1 && VisibleMeshes() >= 8, TEXT("Low plus calm-flash keeps craft and physical ground light"))) return;
		Capture(TEXT("03-low-calm")); Go(55); return;
	}
	if (Stage == 55)
	{
		// Let the native frame requested above render before a pause overlay opens.
		if (Now - StageAt < .35) return;
		PausedAge = D.EventAge(); PausedClock = F.Clock();
		M->Flow->OpenPause(); Go(6); return;
	}
	if (Stage == 6)
	{
		if (Now - StageAt < 1) return;
		if (!Check(M->bPausedByMenu && FMath::IsNearlyEqual(D.EventAge(), PausedAge, .001) && FMath::IsNearlyEqual(F.Clock(), PausedClock, .001),
			TEXT("normal pause holds both encounter time and world clock"))) return;
		M->UI->Close();
		if (!Check(!M->bPausedByMenu, TEXT("normal pause close resumes play"))) return;
		M->World->Enter(TEXT("road")); Go(7); return;
	}
	if (Stage == 7)
	{
		if (Now - StageAt < 2 || M->World->bBusy) return;
		if (!Check(M->Web->Area() == TEXT("road") && !D.Busy() && !D.PhysicalObjectPresented() && VisibleMeshes() == 0 && ActiveLights() == 0,
			TEXT("leaving SARO releases the incident and hides all UFO render components and lights"))) return;
		for (const TCHAR* Name : { TEXT("01-red-approach.png"), TEXT("02-near-pass.png"), TEXT("03-low-calm.png") })
		{
			TArray<uint8> Bytes;
			if (!Check(FFileHelper::LoadFileToArray(Bytes, *(Root / Name)) && Bytes.Num() > 1000 && Bytes[0] == 137 && Bytes[1] == 80 && Bytes[2] == 78 && Bytes[3] == 71,
				FString(TEXT("native screenshot written: ")) + Name)) return;
		}
		Finish(true, TEXT("native near-pass visuals, photo claim, Low/comfort, pause and exit completed"));
	}
}

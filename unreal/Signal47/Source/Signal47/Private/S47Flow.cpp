#include "S47Flow.h"

#include "AudioDevice.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "S47Audio.h"
#include "S47Chapter.h"
#include "S47HUD.h"
#include "S47Image.h"
#include "S47Main.h"
#include "S47Opening.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "SS47AreaMap.h"
#include "SS47Notebook.h"
#include "SS47SaveMenu.h"
#include "SS47Screens.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Flow, Log, All);

using EMode = US47Main::EMode;

namespace
{
	FString StorePath() { return FPaths::ProjectSavedDir() / TEXT("SaveGames") / TEXT("settings.json"); }

	// a test flag on the command line: any -S47... but the picture (a setting) and -S47Title
	bool HasTestFlag()
	{
		const TCHAR* S = FCommandLine::Get();
		FString Tok;
		while (FParse::Token(S, Tok, false))
		{
			if (Tok.StartsWith(TEXT("-S47")) && !Tok.StartsWith(TEXT("-S47Picture")) && !Tok.StartsWith(TEXT("-S47Title"))) return true;
		}
		return false;
	}
	bool SmokeTest() { return FParse::Param(FCommandLine::Get(), TEXT("S47SmokeTest")); }

	// the chapter in play counts as Prologue.at('chN'): ch1 < ch2 < ... < ch6
	int32 ChapterNo(const AS47Story* S)
	{
		return S && S->ChapterKey.StartsWith(TEXT("ch")) ? FCString::Atoi(*S->ChapterKey.Mid(2)) : 0;
	}

	// ---------- SARO's places, until the world's modules are ported ----------
	// world/geo.ts MOTEL_RECT; world/Crossing.ts WEST and SHOULDER, outside, atMotel, onHighway;
	// world/Grounds.ts onRoad (ROAD_W, BEND, ROAD_Z, TRUCK_RAMP)
	struct FRectT { double MinX, MaxX, MinZ, MaxZ; };
	const FRectT MOTEL_RECT{ -50 - 2.5, -27.7 - 2.5, 1.5 + 650, 70.8 + 650 };
	const double WEST_Z1 = 64, SHOULDER_X0 = -33.5, SHOULDER_X1 = -15, SHOULDER_Z1 = MOTEL_RECT.MaxZ + 4;
	bool Outside(const FVector& P) { return P.X < -6.3 && P.X > MOTEL_RECT.MinX && P.Z > -5 && P.Z < SHOULDER_Z1; }
	bool AtMotel(const FVector& P) { return P.X >= MOTEL_RECT.MinX && P.X <= MOTEL_RECT.MaxX + 1 && P.Z >= MOTEL_RECT.MinZ - 4 && P.Z <= MOTEL_RECT.MaxZ + 4; }
	bool OnHighway(const FVector& P) { return P.Z > WEST_Z1 - 20 && P.X > SHOULDER_X0 && P.X < SHOULDER_X1 && !AtMotel(P); }
	bool OnRoad(const FVector& P)
	{
		const double ROAD_W = 5.0, Cx = 5.8, Cz = 20.5, R = 4.0, ROAD_Z = Cz + R, FOOT = 16.6;
		if (P.Z > ROAD_Z - ROAD_W / 2 && P.Z < ROAD_Z + ROAD_W / 2 && P.X < Cx && P.X > -20) return true;
		if (P.X > 9.8 - ROAD_W / 2 && P.X < 9.8 + ROAD_W / 2 && P.Z > FOOT && P.Z < Cz) return true;
		const double D = FMath::Sqrt(FMath::Square(P.X - Cx) + FMath::Square(P.Z - Cz));
		return P.X >= Cx && P.Z >= Cz && D > R - ROAD_W / 2 && D < R + ROAD_W / 2;
	}
	// the motel's zone 'office' (world/MotelFront.ts), the third of its zones in the export
	const TCHAR* MOTEL_OFFICE = TEXT("world.motel.zones[2]");

	const TCHAR* MEET_NOTE = TEXT("All three cross at the transit. I stood at it tonight.");
}

// ---------- the store (main.ts store: the settings, kept on this device) ----------

void US47Flow::LoadStore()
{
	FString Text;
	Store = FFileHelper::LoadFileToString(Text, *StorePath()) ? S47J::Read(Text) : S47J::FObj();
	if (!Store.IsValid()) Store = S47J::New();
}

void US47Flow::StoreSet(const TCHAR* K, const TSharedPtr<FJsonValue>& V)
{
	Store->SetField(K, V);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(StorePath()), true);
	FFileHelper::SaveStringToFile(S47J::Write(Store, true), *StorePath(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
void US47Flow::StoreSet(const TCHAR* K, double V) { StoreSet(K, MakeShared<FJsonValueNumber>(V)); }
void US47Flow::StoreSet(const TCHAR* K, bool V) { StoreSet(K, MakeShared<FJsonValueBoolean>(V)); }
void US47Flow::StoreSet(const TCHAR* K, const FString& V) { StoreSet(K, MakeShared<FJsonValueString>(V)); }

UWorld* US47Flow::GameWorld() const { return M && M->Hud ? M->Hud->GetWorld() : nullptr; }

// ---------- boot ----------

void US47Flow::Init(US47Main* InM)
{
	M = InM;
	LoadStore();
	Saves.Init();
	// the settings shared by the pause menu and the title screen; all kept on this device
	// (Ultra on a computer; an automated run starts on High, so the budgets stay testable)
	Quality = S47J::Str(Store, TEXT("quality"), SmokeTest() ? TEXT("high") : TEXT("ultra"));
	bKinski = S47J::Bool(Store, TEXT("kinskiReadAloud"), false);
	bArtBell = S47J::Bool(Store, TEXT("artBellRadioHost"), true);
	bSerling = S47J::Bool(Store, TEXT("serlingNarrator"), true);
	bCharacters = S47J::Bool(Store, TEXT("characterVoices"), true);
	if (M->Audio) { M->Audio->bCharacterVoices = bCharacters; M->Audio->bKinskiCalls = bKinski; }
	Volume = float(S47J::Num(Store, TEXT("vol"), 0.8));
	MusicLevel = float(S47J::Num(Store, TEXT("music"), 0.7));
	RadioLevel = float(S47J::Num(Store, TEXT("radioVol"), 1));
	bRadioCast = S47J::Bool(Store, TEXT("radioCallerVoices"), true);
	bAbductOn = S47J::Bool(Store, TEXT("abductions"), true);
	Sens = float(S47J::Num(Store, TEXT("sens"), 1));
	bInvertY = S47J::Bool(Store, TEXT("invertY"), false);
	BaseFov = float(S47J::Num(Store, TEXT("fov"), 70));
	bLargeText = S47J::Bool(Store, TEXT("largeText"), false);
	if (M->UI) M->UI->bLargeText = bLargeText;
	bCalmFlash = S47J::Bool(Store, TEXT("calmFlash"), false);
	bStillShots = S47J::Bool(Store, TEXT("stillShots"), false);
	// the picture: as chosen, or until one is chosen as the graphics level has it; -S47Picture= on
	// the command line (AS47Web) has the last word
	const FString Chosen = S47J::Str(Store, TEXT("picture"));
	FString Cmd;
	if (FParse::Value(FCommandLine::Get(), TEXT("S47Picture="), Cmd)) Picture = Cmd == TEXT("off") ? TEXT("off") : TEXT("vhs");
	else ApplyPicture(!Chosen.IsEmpty() ? Chosen : Quality != TEXT("low") ? TEXT("vhs") : TEXT("off"), false);
	ApplyVolume();
	ApplyLook();

	// the opening card (core/Opening.ts)
	FS47OpeningHooks H;
	H.Show = [this](const FS47OpeningActions& A) { return M->UI->OpeningCard(A); };
	H.Quiet = [this] { StopMusic(0.f); if (M->Audio) M->Audio->Silence(true); };
	H.Begin = [this](const FS47OpeningChoice& C) { FStart S; S.CaseId = C.CaseId; S.bReplaceCase = C.bReplace; StartGame(S); };
	H.Back = [this]
	{
		if (M->Audio) M->Audio->Silence(false);
		ShowTitleAgain();
		if (TitleEl.IsValid()) StaticCastSharedPtr<SS47Title>(TitleEl)->FocusStart();
	};
	H.Dismissed = [this] { if (M->Audio) M->Audio->Silence(false); if (M->Mode == EMode::Title) MenuMusic(); };
	Opening = MakeShared<FS47Opening>(MoveTemp(H));

	// the host's hooks: Prologue deps loadCase, and game.onCheckpoint (a new safe point, 02:13 or a
	// new chapter, starts a new autosave generation); the chapters' saveCase is M->RequestAutosave()
	if (AS47Story* Host = M->Story)
	{
		Host->LoadCase = [this] { return Restoring.IsValid() ? S47J::Obj(Restoring, TEXT("case")) : S47J::FObj(); };
		Host->OnCheckpoint = [this](const FString&) { RequestAutosave(true); };
		// main.ts chapterBreak, once the next chapter has begun behind the black: the card with what was
		// closed and the night's clock. Without the narrator it ends with the host's 6.4 s, the black with it.
		Host->OnChapterCard = [this](const AS47Story::FBreak& B)
		{
			FS47ChapterCardOpts C;
			C.Closed = B.Closed; C.Recap = B.Recap; C.Next = B.Next; C.Title = B.Title;
			C.Clock = S47Text::ClockText(M->Story->Clock(), false);
			M->UI->Fade(true, FString());
			M->UI->ChapterCard(&C);
			M->Story->After(6.4, [this] { M->UI->ChapterCard(nullptr); });
		};
	}
	// doors.onChange: the chapters that care remember them, and the change is saved
	if (M->Web) M->Web->OnDoorChanged = [this](const FString& Id, bool bOpen) { DoorChanged(Id, bOpen); };

	// main.ts input.onKey (with no panel open; the panels answer their own keys)
	auto Notebook = [this](const TCHAR* Tab)
	{
		return [this, Tab]
		{
			if (M->Mode != EMode::Play) return false;
			if (M->Story && !M->Story->IsCinematic())
			{
				if (Tab) M->UI->NotebookAt(Tab);
				OpenNotebook();
			}
			return true;
		};
	};
	M->OnKey(EKeys::Tab, Notebook(nullptr));
	M->OnKey(EKeys::N, Notebook(nullptr));
	// the map, the kit and the controls, straight to their tabs in the notebook
	M->OnKey(EKeys::M, Notebook(TEXT("map")));
	M->OnKey(EKeys::I, Notebook(TEXT("kit")));
	M->OnKey(EKeys::H, Notebook(TEXT("controls")));
	// Escape or P: the pause menu (with the field camera raised, Escape lowers it: chapter one's own
	// key, registered before this one)
	auto Pause = [this] { if (M->Mode != EMode::Play) return false; OpenPause(); return true; };
	M->OnKey(EKeys::Escape, Pause);
	M->OnKey(EKeys::P, Pause);

	// the window put away (main.ts visibilitychange and the lost pointer lock)
	if (FSlateApplication::IsInitialized()) ActivationHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this, &US47Flow::Activation);
}

void US47Flow::BeginDestroy()
{
	if (ActivationHandle.IsValid() && FSlateApplication::IsInitialized()) FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(ActivationHandle);
	Super::BeginDestroy();
}

void US47Flow::Boot()
{
	const bool bTitle = FParse::Param(FCommandLine::Get(), TEXT("S47Title"));
	if (HasTestFlag() && !bTitle)
	{
		// straight into play, as every test run starts (main.ts devStart: a run that is never saved,
		// unless -S47Case=N gives it a case to save into)
		int32 Case = 0;
		FParse::Value(FCommandLine::Get(), TEXT("S47Case="), Case);
		CaseId = FMath::Clamp(Case, 0, S47Saves::CASES);
		bDevRun = CaseId == 0;
		if (CaseId) Saves.SetActive(CaseId);
		M->Mode = EMode::Play;
		M->Playtime = 0;
		bSoundUnlocked = true;
		PlayInput();
		FString Jump;
		if (FParse::Value(FCommandLine::Get(), TEXT("S47Jump="), Jump)) M->Story->Jump(Jump);
		else M->Story->Start();
		return;
	}
	// the first title: nothing of the night is heard yet (the room tone starts with the night), and
	// the station behind it is as the night begins (the Prologue's constructor resets it)
	if (M->Audio) M->Audio->Stop(TEXT("room"), 0.f);
	if (M->Story) M->Story->Reset();
	M->Mode = EMode::Title;
	ShowTitle();
}

void US47Flow::Step(double Dt)
{
	if (M->Mode == EMode::Title)
	{
		TitleCam(M->T);
		// (a panel closed over the title gives the keys back to the game: they go back to the title)
		APlayerController* PC = M->Hud ? M->Hud->GetOwningPlayerController() : nullptr;
		if (PC && TitleEl.IsValid() && !M->UI->Modal() && !PC->bShowMouseCursor)
		{
			FInputModeUIOnly Mode;
			Mode.SetWidgetToFocus(TitleEl);
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(Mode);
			PC->bShowMouseCursor = true;
			StaticCastSharedPtr<SS47Title>(TitleEl)->FocusMenu();
		}
	}
	// frame(): right after the frame, so the save's picture is this frame
	FlushAutosave();
}

// ---------- the title camera ----------

void US47Flow::TitleCam(double T)
{
	// main.ts titleCam: from (-22.5, 2, 70) towards (2, 9, -60), drifting
	if (!TitleCamera) return;
	const FVector From(-22.5 + FMath::Sin(T * 0.05) * 2.2, 2.0 + FMath::Sin(T * 0.08) * 0.15, 70 - FMath::Sin(T * 0.04) * 3);
	const FVector Look(2 + FMath::Sin(T * 0.03) * 4, 9, -60);
	const double Dx = Look.X - From.X, Dy = Look.Y - From.Y, Dz = Look.Z - From.Z;
	TitleCamera->SetActorLocationAndRotation(S47Three::ToUE(From), S47Three::CameraRotation(FMath::Atan2(-Dx, -Dz), FMath::Atan2(Dy, FMath::Sqrt(Dx * Dx + Dz * Dz))));
	FVector2D Size(16, 9);
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(Size);
	TitleCamera->GetCameraComponent()->SetFieldOfView(S47Three::HorizontalFov(BaseFov, float(Size.X / FMath::Max(1.0, Size.Y))));
}

void US47Flow::TitleView(bool bOn)
{
	APlayerController* PC = M->Hud ? M->Hud->GetOwningPlayerController() : nullptr;
	UWorld* World = GameWorld();
	if (!PC || !World) return;
	if (bOn)
	{
		if (!TitleCamera)
		{
			TitleCamera = World->SpawnActor<ACameraActor>();
			if (TitleCamera) TitleCamera->GetCameraComponent()->bConstrainAspectRatio = false;
		}
		TitleCam(M->T);
		if (TitleCamera) PC->SetViewTarget(TitleCamera);
	}
	else if (M->Player) PC->SetViewTarget(M->Player);
}

void US47Flow::PlayInput()
{
	// input.enabled, the pointer taken: the walker's eyes, the keys to the game
	TitleView(false);
	if (APlayerController* PC = M->Hud ? M->Hud->GetOwningPlayerController() : nullptr)
	{
		FInputModeGameOnly Mode;
		Mode.SetConsumeCaptureMouseDown(false);
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = false;
	}
}

// ---------- the title ----------

void US47Flow::MenuMusic()
{
	// Music on the title screen ("Signal to Noise", the end's music too), from the first key or click
	if (M->Mode != EMode::Title || (Opening && Opening->Active())) return;
	// audio.loop('music', 'titleMusic', { dest: audio.music, gain: 0.6 }): the one before stops
	StopMusic(0.05f);
	USoundBase* S = nullptr;
	for (const TCHAR* F : { TEXT("/Game/Audio/Sfx"), TEXT("/Game/Audio/Music") })
	{
		S = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("%s/title_music.title_music"), F));
		if (S) break;
	}
	UWorld* World = GameWorld();
	if (!S || !World) return;
	Music = UGameplayStatics::CreateSound2D(World, S, 0.6f * MusicLevel, 1.f, 0.f, nullptr, true, false);
	if (!Music) return;
	// (the recording is not marked looping in the import: it starts again when it ends)
	TWeakObjectPtr<UAudioComponent> W = Music;
	Music->OnAudioFinishedNative.AddLambda([W](UAudioComponent*) { if (W.IsValid()) W->Play(); });
	Music->Play();
}

void US47Flow::StopMusic(float Fade)
{
	// audio.stop('music', fade): setTargetAtTime(0, t, fade), gone after six of it
	if (!Music) return;
	UAudioComponent* C = Music;
	Music = nullptr;
	C->OnAudioFinishedNative.Clear();
	if (Fade > 0.f) C->FadeOut(Fade * 3.f, 0.f); else C->Stop();
}

void US47Flow::ShowTitle()
{
	// The first time the menu waits behind "Click or press any key", so that first key brings in
	// the music; back from a night the sound is already going and it starts at once. Not for the
	// tests, which click their way straight in.
	const bool bGate = !bSoundUnlocked && !SmokeTest();
	if (bSoundUnlocked) MenuMusic();
	const TOptional<FS47SaveMeta> Cont = Saves.ContinueSave();
	FS47TitleOpts O;
	O.bGate = bGate;
	O.bTouch = false;
	O.Cont = Cont ? FString::Printf(TEXT("%s · %s · %s · %s played"), *Cont->Chapter, *Cont->Place, *Cont->Clock, *S47Css::Played(Cont->Playtime)) : FString();
	O.OnContinue = [this] { const TOptional<FS47SaveMeta> C = Saves.ContinueSave(); if (C) OpenSave(*C); };
	O.OnStart = [this]
	{
		if (bStarting || Opening->Active() || M->UI->Modal()) return;
		const int32 Free = Saves.FreeCase();
		if (Free) { RequestNewNight(Free); return; }
		// every case slot is taken: pick one to replace
		FS47SaveMenuOpts S;
		S.Mode = TEXT("new"); S.Metas = Saves.List(); S.ActiveCase = Saves.Active();
		S.OnNewCase = [this](int32 C) { RequestNewNight(C, true); };
		S47SaveMenu::Open(M->UI, S);
	};
	O.bCanLoad = Saves.List().Num() > 0;
	O.OnLoad = [this]
	{
		FS47SaveMenuOpts S;
		S.Mode = TEXT("load"); S.Metas = Saves.List(); S.ActiveCase = Saves.Active(); S.bFallback = Saves.bFallback;
		S.OnLoad = [this](const FS47SaveMeta& Mt) { OpenSave(Mt); };
		S.OnDelete = [this](const FS47SaveMeta& Mt, FString& Err) { return Saves.Remove(Mt.Id, Err); };
		S.OnBack = [this] { if (M->Mode == EMode::Title) ShowTitleAgain(); };
		S47SaveMenu::Open(M->UI, S);
	};
	O.OnSettings = [this]
	{
		FS47PauseOpts S = Settings();
		S.Title = TEXT("Settings"); S.bSettingsOnly = true; S.OnResume = [] {}; S.OnTitle = [] {};
		M->UI->Pause(S);
	};
	// the warning before the night: flashing images near the end, or slow fades instead
	O.bCalm = bCalmFlash;
	O.OnFlashPick = [this](bool bCalm) { bCalmFlash = bCalm; StoreSet(TEXT("calmFlash"), bCalm); };
	O.OnFirstTouch = [this] { bSoundUnlocked = true; MenuMusic(); };
	M->UI->RemoveScreen(TitleEl);
	TitleEl = M->UI->Title(O);
	TitleView(true);
	UE_LOG(LogS47Flow, Display, TEXT("S47: title shown%s, %d saves"), bGate ? TEXT(" (gated)") : TEXT(""), Saves.List().Num());
}

void US47Flow::ShowTitleAgain()
{
	M->UI->RemoveScreen(TitleEl);
	TitleEl.Reset();
	ShowTitle();
}

void US47Flow::RequestNewNight(int32 InCaseId, bool bReplace)
{
	if (bStarting || M->Mode != EMode::Title || Opening->Active()) return;
	FS47OpeningChoice C;
	C.CaseId = InCaseId; C.bReplace = bReplace;
	Opening->Open(C);
}

// Open a save: from the title, the pause menu or Continue.
void US47Flow::OpenSave(const FS47SaveMeta& Meta)
{
	Opening->Dismiss();
	if (M->Mode == EMode::Play) LeavePlay();
	const S47J::FObj State = Saves.Load(Meta.Id);
	if (!State.IsValid())
	{
		M->UI->Toast(TEXT("That save could not be read. Your other saves are unchanged."), 5);
		if (M->Mode != EMode::Play) ShowTitleAgain();
		return;
	}
	FStart S;
	S.CaseId = Meta.CaseId; S.Playtime = Meta.Playtime; S.State = State;
	StartGame(S);
}

// ---------- a night ----------

void US47Flow::StartGame(const FStart& O)
{
	if (bStarting) return;
	bStarting = true;
	Opening->Dismiss();
	bDevRun = O.bDev;
	bSoundUnlocked = true;
	M->UI->RemoveScreen(TitleEl);
	TitleEl.Reset();
	M->UI->Close(true);
	M->UI->Fade(true, FString());
	M->Mode = EMode::Play;
	PlayInput();
	// (TUNING RECEIVERS: the sounds are not decoded late here)
	if (M->Audio) M->Audio->Silence(false);
	StopMusic(1.2f); // the title screen's music
	if (O.bReplaceCase)
	{
		FString Err;
		if (!Saves.RemoveCase(O.CaseId, Err))
		{
			bStarting = false;
			M->Mode = EMode::Title;
			M->UI->Fade(false);
			ShowTitleAgain();
			M->UI->Toast(TEXT("The case could not be replaced. Choose a case or try again."), 5);
			return;
		}
	}
	// a save made at the station or in room 6 needs that area (the exported world has them all)
	const FString SavedArea = S47J::Str(O.State, TEXT("area"));
	const FString Area = SavedArea == TEXT("station01") || SavedArea == TEXT("room6") || SavedArea == TEXT("diner") ? SavedArea : FString(TEXT("saro"));
	CaseId = O.CaseId;
	M->Playtime = O.Playtime;
	if (CaseId) Saves.SetActive(CaseId);
	if (M->Audio) M->Audio->StartRoomTone();
	// (audio.startNight, the Score, space = 'room', world.placeTruck: not in the Unreal version yet)
	if (M->Web) M->Web->Enter(TEXT("saro"));
	Restoring = O.State;
	bRestoringNow = true;
	// what is saved for the systems not ported yet goes back into the next save as it was
	Carry = O.State.IsValid() ? S47J::Clone(O.State) : S47J::FObj();
	// (flashlight, seenLog, curve, townSeen, recWalk, stationTaken, truckTaken, the odometer, townEvents,
	// glimpses, cutouts, nightEvents, abduction, lineLights, pasture, motelTv, roofSteps, officePhone,
	// truckB, ninth, starHole, morning, sameThread, album, saroDark: the night's systems, not ported yet)
	const FString Checkpoint = O.State.IsValid() ? S47J::Str(O.State, TEXT("checkpoint")) : FString();
	StartStory(!Checkpoint.IsEmpty() ? Checkpoint : O.Jump);
	if (LoadBoard && O.State.IsValid()) LoadBoard(S47J::Obj(S47J::Obj(O.State, TEXT("case")), TEXT("board")));
	// the doors as they were left
	if (const S47J::FObj Doors = S47J::Obj(O.State, TEXT("doors")); Doors.IsValid() && M->Web)
	{
		for (const auto& KV : Doors->Values)
		{
			bool bOpen = false;
			KV.Value->TryGetBool(bOpen);
			M->Web->SetDoorById(FString(*KV.Key), bOpen, true);
		}
	}
	// (abduction.refile, follow, arrayBack, archive, fieldBook, the town's papers, lostNights, motelTv: not ported yet)
	bRestoringNow = false;
	Restoring.Reset();
	// (roadside, the CB, the phones, the radio: not ported yet)
	if (M->Audio) M->Audio->StopPhoneCalls();
	// world.enter(area); (placeAtStation, placeAtRoom6, placeAtDiner: the world's, not ported yet)
	if (M->Web && Area != TEXT("saro")) M->Web->Enter(Area);
	// put the player back where they stood, if the restored world lets them stand there
	const S47J::FObj Pose = S47J::Obj(O.State, TEXT("pose"));
	if (Pose.IsValid() && Checkpoint != TEXT("residual") && M->Player && M->Player->Walkable(S47J::Num(Pose, TEXT("x")), S47J::Num(Pose, TEXT("z"))))
	{
		M->Player->Place(S47J::Num(Pose, TEXT("x")), S47J::Num(Pose, TEXT("z")), S47J::Num(Pose, TEXT("yaw")));
		M->Player->Pitch = S47J::Num(Pose, TEXT("pitch"));
	}
	bStarting = false;
	FadeInSoon(0.25f);
	UE_LOG(LogS47Flow, Display, TEXT("S47: night started, case %d, %s"), CaseId, Checkpoint.IsEmpty() ? TEXT("new") : *Checkpoint);
}

void US47Flow::StartStory(const FString& Checkpoint)
{
	// Prologue.start(checkpoint): back to the start of the night, the player at the spawn, then the
	// checkpoint's beat, or the night from 23:41
	if (!M->Story) return;
	if (Checkpoint.IsEmpty()) { M->Story->Start(); return; }
	if (M->Web && M->Player) { const FVector Sp = M->Web->SpawnThree(); M->Player->Place(Sp.X, Sp.Z, Sp.Y); }
	M->Story->Jump(Checkpoint);
}

void US47Flow::FadeInSoon(float Secs)
{
	TWeakObjectPtr<US47Flow> Self = this;
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self](float)
	{
		if (Self.IsValid() && Self->M && Self->M->UI) Self->M->UI->Fade(false);
		return false;
	}), Secs);
}

// Stop the night that is running, without the title screen (before opening another save).
void US47Flow::LeavePlay()
{
	M->UI->Fade(true, FString());
	// (fcam.raise(false): the field camera is chapter one's)
	M->UI->Viewfinder(false);
	M->UI->Close(true);
	StopMusic(0.5f);
	if (M->Audio) { M->Audio->StopMotors(); M->Audio->SetCarrier(0, 0, 0); M->Audio->SignalOff(); M->Audio->StopPhoneCalls(); }
	// (motelTv, abduction, saroDark, follow, arrayBack, recWalk, stationTaken, archive, arrayTip,
	// world.stopDriving, roadside, townEvents, nightEvents, roofSteps, the CB, reelTape, officePhone,
	// truckB, truckTaken, the radio, the odometer, the seen log, the curve: not ported yet)
	if (M->Story) M->Story->Reset();
	if (M->Web) M->Web->Enter(TEXT("saro"));
	bPendingAuto = false;
}

void US47Flow::ToTitle()
{
	Opening->Dismiss();
	if (M->Audio) M->Audio->Silence(false);
	M->Mode = EMode::Title;
	M->UI->Viewfinder(false);
	M->UI->Close(true);
	StopMusic(1.5f);
	if (M->Audio) { M->Audio->StopMotors(); M->Audio->SetCarrier(0, 0, 0); M->Audio->SignalOff(); M->Audio->StopPhoneCalls(); }
	// (motelTv, abduction, saroDark, follow, arrayBack, recWalk, stationTaken, truckTaken, archive,
	// arrayTip, world.stopDriving, roadside, the CB, the radio, the curve: not ported yet)
	if (M->Story) M->Story->Reset();
	if (M->Web) M->Web->Enter(TEXT("saro"));
	bPendingAuto = false;
	M->UI->Fade(false);
	ShowTitle();
}

// ---------- pause and the settings ----------

void US47Flow::OpenPause()
{
	if (M->bPausedByMenu) return;
	// (readAloud, earlDialogue, narrator, npcDialogue stop: the voices are not ported yet)
	M->bPausedByMenu = true;
	// audio.suspend(true), and the frame does nothing: the world stops, and its sounds with it
	UGameplayStatics::SetGamePaused(M->Hud, true);
	TWeakObjectPtr<US47Flow> Self = this;
	auto Resume = [Self]
	{
		if (!Self.IsValid()) return;
		Self->M->bPausedByMenu = false;
		UGameplayStatics::SetGamePaused(Self->M->Hud, false);
	};
	FS47PauseOpts O = Settings();
	O.OnResume = Resume;
	O.OnTitle = [this, Resume] { Resume(); AutosaveNow(); ToTitle(); };
	// the case menus sit on top of the pause menu and go back to it
	O.OnSave = [this]
	{
		FS47SaveMenuOpts S;
		S.Mode = TEXT("save"); S.Metas = Saves.List(); S.ActiveCase = CaseId; S.Block = SaveBlock(); S.bFallback = Saves.bFallback;
		S.OnSave = [this](int32 Slot, FString& Err) { return WriteSave(TEXT("manual"), Slot, Err); };
		S.OnBack = [this] { M->bPausedByMenu = false; OpenPause(); };
		S47SaveMenu::Open(M->UI, S);
	};
	O.OnLoad = [this, Resume]
	{
		FS47SaveMenuOpts S;
		S.Mode = TEXT("load"); S.Metas = Saves.List(); S.ActiveCase = CaseId; S.bFallback = Saves.bFallback;
		S.OnLoad = [this, Resume](const FS47SaveMeta& Mt) { Resume(); OpenSave(Mt); };
		S.OnDelete = [this](const FS47SaveMeta& Mt, FString& Err) { return Saves.Remove(Mt.Id, Err); };
		S.OnBack = [this] { M->bPausedByMenu = false; OpenPause(); };
		S47SaveMenu::Open(M->UI, S);
	};
	M->UI->Pause(O);
	UE_LOG(LogS47Flow, Display, TEXT("S47: pause opened"));
}

FS47PauseOpts US47Flow::Settings()
{
	FS47PauseOpts O;
	O.bCharacters = bCharacters;
	O.OnCharacters = [this](bool V) { bCharacters = V; if (M->Audio) { M->Audio->bCharacterVoices = V; if (!V) M->Audio->StopPhoneCalls(); } StoreSet(TEXT("characterVoices"), V); };
	O.bSerling = bSerling;
	O.OnSerling = [this](bool V) { bSerling = V; StoreSet(TEXT("serlingNarrator"), V); };
	O.bKinski = bKinski;
	O.OnKinski = [this](bool V) { bKinski = V; if (M->Audio) { M->Audio->bKinskiCalls = V; if (!V) M->Audio->StopPhoneCalls(); } StoreSet(TEXT("kinskiReadAloud"), V); };
	O.bArtBell = bArtBell;
	O.OnArtBell = [this](bool V) { bArtBell = V; StoreSet(TEXT("artBellRadioHost"), V); }; // (radio.setHostVoice, Earl: not ported yet)
	O.bRadioCast = bRadioCast;
	O.OnRadioCast = [this](bool V) { bRadioCast = V; StoreSet(TEXT("radioCallerVoices"), V); };
	O.Volume = Volume; O.Sens = Sens;
	O.OnVolume = [this](float V) { Volume = V; ApplyVolume(); StoreSet(TEXT("vol"), double(V)); };
	O.Music = MusicLevel;
	O.OnMusic = [this](float V) { MusicLevel = V; if (Music) Music->SetVolumeMultiplier(0.6f * V); StoreSet(TEXT("music"), double(V)); };
	O.Radio = RadioLevel;
	O.OnRadio = [this](float V) { RadioLevel = V; StoreSet(TEXT("radioVol"), double(V)); }; // (the radios are not ported yet)
	O.OnSens = [this](float V) { Sens = V; ApplyLook(); StoreSet(TEXT("sens"), double(V)); };
	O.Quality = Quality;
	O.OnQuality = [this](const FString& Q) { ApplyQuality(Q); };
	O.bUltraOk = true; // a computer
	O.Picture = Picture;
	O.OnPicture = [this](const FString& P) { ApplyPicture(P); };
	O.bInvertY = bInvertY;
	O.OnInvertY = [this](bool V) { bInvertY = V; ApplyLook(); StoreSet(TEXT("invertY"), V); };
	O.Fov = BaseFov;
	O.OnFov = [this](float V) { BaseFov = V; StoreSet(TEXT("fov"), double(V)); ApplyLook(); };
	O.bLargeText = bLargeText;
	O.OnLargeText = [this](bool V) { bLargeText = V; M->UI->bLargeText = V; StoreSet(TEXT("largeText"), V); };
	O.bCalmFlash = bCalmFlash;
	O.OnCalmFlash = [this](bool V) { bCalmFlash = V; StoreSet(TEXT("calmFlash"), V); };
	O.bStillShots = bStillShots;
	O.OnStillShots = [this](bool V) { bStillShots = V; StoreSet(TEXT("stillShots"), V); };
	O.bAbduct = bAbductOn;
	O.OnAbduct = [this](bool V) { bAbductOn = V; StoreSet(TEXT("abductions"), V); };
	return O;
}

void US47Flow::ApplyQuality(const FString& Q)
{
	Quality = Q;
	StoreSet(TEXT("quality"), Q);
	// until a picture is chosen in Settings, the picture follows the graphics level
	if (!S47J::Has(Store, TEXT("picture"))) ApplyPicture(Q != TEXT("low") ? TEXT("vhs") : TEXT("off"), false);
	// (the tape's samples, setQuality, the bought models' pictures, Ultra's lamps and shadows and the
	// pixel ratio are the web renderer's; the Unreal version draws the same on every level for now)
}

void US47Flow::ApplyPicture(const FString& P, bool bChosen)
{
	Picture = P;
	if (bChosen) StoreSet(TEXT("picture"), P);
	// the tape look (core/vhs.ts) is M_WebTone's Vhs in MPC_Web (AS47Web::BeginPlay)
	UWorld* World = GameWorld();
	UMaterialParameterCollection* Mpc = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Web/Base/MPC_Web.MPC_Web"));
	if (UMaterialParameterCollectionInstance* I = World && Mpc ? World->GetParameterCollectionInstance(Mpc) : nullptr)
	{
		I->SetScalarParameterValue(TEXT("Vhs"), P == TEXT("off") ? 0.f : 1.f);
	}
}

void US47Flow::ApplyLook()
{
	// input.sensitivity, invertY, and resize(): the lens is the field of view on a computer
	if (!M->Player) return;
	M->Player->Sensitivity = Sens;
	M->Player->bInvertY = bInvertY;
	M->Player->Fov = BaseFov;
}

void US47Flow::ApplyVolume()
{
	// audio.setVolume: the master level
	if (FAudioDevice* D = GEngine ? GEngine->GetMainAudioDeviceRaw() : nullptr) D->SetTransientPrimaryVolume(Volume);
}

void US47Flow::Activation(bool bActive)
{
	// main.ts: the tab put away (a phone may close it in the background) or the pointer lock lost:
	// save, and pause. (Not in the automated runs, where the window may never have the focus.)
	if (bActive || !M || M->Mode != EMode::Play || SmokeTest()) return;
	AutosaveNow();
	if (!M->bPausedByMenu) OpenPause();
}

void US47Flow::DoorChanged(const FString& Id, bool bOpen)
{
	// Prologue.doorChanged: chapters one, two and four remember the doors
	if (M->Story)
	{
		for (const TCHAR* K : { TEXT("ch1"), TEXT("ch2"), TEXT("ch4") }) if (US47Chapter* C = M->Story->Chapter(K)) C->DoorChanged(Id, bOpen);
	}
	if (M->Mode == EMode::Play && !bRestoringNow) RequestAutosave(false);
}

// ---------- saving ----------

FString US47Flow::SaveBlock() const
{
	if (bDevRun) return TEXT("This is a developer run. Nothing is saved.");
	if (M->Mode != EMode::Play || !CaseId) return TEXT("There is no night running to save.");
	// (the time away's 47 seconds, the array put back, SARO going dark, the archive's phone, the truck's
	// count on B: those races are not in the Unreal version yet)
	return GameSaveBlock();
}

void US47Flow::RequestAutosave(bool bMilestone)
{
	// Progress asks for an autosave; it is written after the next frame, once the game may be saved.
	if (!CaseId || M->Mode != EMode::Play || bRestoringNow) return;
	if (bMilestone) Saves.NewGeneration(CaseId);
	bPendingAuto = true;
}

void US47Flow::FlushAutosave()
{
	if (!bPendingAuto || !SaveBlock().IsEmpty()) return;
	bPendingAuto = false;
	FString Err;
	if (!WriteSave(TEXT("auto"), -1, Err)) M->UI->Toast(TEXT("Autosave failed: ") + Err, 5);
}

void US47Flow::AutosaveNow()
{
	// right now, before leaving
	if (!SaveBlock().IsEmpty()) return;
	bPendingAuto = true;
	FlushAutosave();
}

S47J::FObj US47Flow::GameState()
{
	const FString Checkpoint = CheckpointName();
	const bool bPrologue = Checkpoint == TEXT("residual");
	// (the fields of the systems not in the Unreal version yet, as the opened save had them)
	const S47J::FObj State = Carry.IsValid() ? S47J::Clone(Carry) : S47J::New();
	S47J::Set(State, TEXT("v"), 2);
	S47J::Set(State, TEXT("checkpoint"), Checkpoint);
	S47J::Set(State, TEXT("case"), SnapshotCase());
	S47J::Set(State, TEXT("area"), M->Web ? M->Web->Area() : FString(TEXT("saro")));
	if (M->Web)
	{
		const S47J::FObj Doors = S47J::New();
		for (const TPair<FString, bool>& D : M->Web->DoorStates()) S47J::Set(Doors, *D.Key, D.Value);
		S47J::Set(State, TEXT("doors"), Doors);
	}
	if (bPrologue || !M->Player) S47J::SetNull(State, TEXT("pose"));
	else
	{
		const S47J::FObj Pose = S47J::New();
		S47J::Set(Pose, TEXT("x"), M->Player->Pos.X);
		S47J::Set(Pose, TEXT("z"), M->Player->Pos.Z);
		S47J::Set(Pose, TEXT("yaw"), M->Player->Yaw);
		S47J::Set(Pose, TEXT("pitch"), M->Player->Pitch);
		S47J::Set(State, TEXT("pose"), Pose);
	}
	return State;
}

TOptional<FS47SaveMeta> US47Flow::WriteSave(const FString& Kind, int32 Slot, FString& OutError)
{
	const S47J::FObj State = GameState();
	const bool bPrologue = S47J::Str(State, TEXT("checkpoint")) == TEXT("residual");
	FS47SaveCard Card;
	Card.Chapter = M->Story ? M->Story->ChapterTitle() : FString();
	Card.Place = PlaceName();
	Card.Clock = bPrologue ? FString(TEXT("02:13")) : S47Text::ClockText(M->Story ? M->Story->Clock() : 0, false);
	Card.Playtime = M->Playtime;
	Card.Thumb = CaptureThumb();
	return Saves.Save(CaseId, Kind, Slot, State, Card, OutError);
}

FString US47Flow::CaptureThumb()
{
	// a small picture of what the player sees: 192 x 108, cut from the view as the web version's
	// canvas copy (cover), as a JPEG at .72. A capture from the player's eye carries the world's
	// post process (M_WebTone and the tape), and none of the panels over it.
	AS47Walker* W = M->Player;
	UWorld* World = GameWorld();
	if (!W || !World) return FString();
	if (!ThumbTarget)
	{
		ThumbTarget = NewObject<UTextureRenderTarget2D>(this);
		ThumbTarget->InitCustomFormat(192, 108, PF_B8G8R8A8, true);
		ThumbTarget->UpdateResourceImmediate(true);
	}
	if (!ThumbCapture && M->Hud)
	{
		ThumbCapture = NewObject<USceneCaptureComponent2D>(M->Hud, TEXT("S47Thumb"));
		ThumbCapture->bCaptureEveryFrame = false;
		ThumbCapture->bCaptureOnMovement = false;
		ThumbCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		ThumbCapture->TextureTarget = ThumbTarget;
		ThumbCapture->RegisterComponentWithWorld(World);
	}
	if (!ThumbCapture) return FString();
	FVector2D Size(16, 9);
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(Size);
	const float Aspect = FMath::Min(float(Size.X / FMath::Max(1.0, Size.Y)), 16.f / 9.f);
	ThumbCapture->FOVAngle = S47Three::HorizontalFov(W->Fov, Aspect);
	ThumbCapture->SetWorldLocationAndRotation(W->EyeUE(), FRotationMatrix::MakeFromX(W->ForwardUE()).Rotator());
	ThumbCapture->CaptureScene();
	TArray<FColor> Px;
	if (!S47Image::ReadPixels(ThumbTarget, Px) || Px.Num() != 192 * 108) return FString();
	for (FColor& C : Px) C.A = 255;
	return S47Image::ToDataUrl(Px, 192, 108, true, 72);
}

FString US47Flow::PlaceName() const
{
	// main.ts placeName: the place on a save's card
	const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
	const FString Area = M->Web ? M->Web->Area() : FString(TEXT("saro"));
	auto In = [&](const TCHAR* Path) { return M->Web && M->Web->Rect(Path).Contains(P.X, P.Z); };
	// (world.indoors, the town and the road's stops are the world's, not ported yet: the inside of the
	// hut by its bounds, the rest by the area's own name)
	if (Area == TEXT("station01")) return In(TEXT("world.site.hutBounds")) ? TEXT("STATION 01, field hut") : TEXT("STATION 01");
	if (Area == TEXT("road")) return TEXT("Highway south");
	if (Area == TEXT("room6")) return TEXT("Sierra Motor Court");
	if (Area == TEXT("diner")) return TEXT("Mesa Diner, the lot");
	if (Area == TEXT("roswell")) return TEXT("The old Roswell road");
	if (In(MOTEL_OFFICE)) return TEXT("Sierra Motor Court, office");
	if (OnRoad(P)) return TEXT("Service road");
	if (Outside(P) && P.Z > -5) return AtMotel(P) ? TEXT("Sierra Motor Court") : OnHighway(P) ? TEXT("The highway south") : TEXT("West lot");
	// (world.field.inField and placeAt, the antenna field: world/Field.ts, not ported yet)
	// grounds.onGround: the floor under the player is SARO's ground, below the floors
	if (M->Player && M->Player->FloorY < -0.3) return TEXT("SARO grounds");
	if (CheckpointName() == TEXT("residual")) return TEXT("Control room");
	if (In(TEXT("annex.zone.records")) || In(TEXT("annex.zone.recordsDoor"))) return TEXT("Records room");
	if (In(TEXT("annex.zone.corridor")) || In(TEXT("annex.zone.southDoor"))) return TEXT("South corridor");
	if (In(TEXT("room.bounds"))) return TEXT("Control room");
	if (In(TEXT("yard.zone.lab"))) return TEXT("Photo lab");
	if (In(TEXT("yard.zone.truckPad"))) return TEXT("Truck pad");
	return TEXT("Service yard");
}

// ---------- the case (story/Prologue.ts) ----------

FString US47Flow::CheckpointName() const
{
	// Where a save resumes. The prologue only has safe points at the start of the night and at
	// 02:13; inside a chapter the chapter's own state is the resume point.
	const AS47Story* S = M->Story;
	if (!S) return TEXT("start");
	const FString& K = S->ChapterKey;
	if (K == TEXT("ch5") || K == TEXT("ch6")) return TEXT("chapter5");
	if (K == TEXT("ch4")) return TEXT("chapter4");
	if (K == TEXT("ch3")) return TEXT("chapter3");
	if (K == TEXT("ch2")) return TEXT("chapter2");
	if (K == TEXT("ch1")) return TEXT("chapter1");
	return S->At(ES47Phase::Residual) ? TEXT("residual") : TEXT("start");
}

FString US47Flow::GameSaveBlock() const
{
	// Prologue.saveBlock: why the game cannot be saved right now, or empty when it can
	const AS47Story* S = M->Story;
	if (!S) return TEXT("There is no night running to save.");
	if (!S->At(ES47Phase::Residual)) return TEXT("The night can be saved from 02:13, once the survey has been running for a while.");
	if (S->IsCinematic()) return TEXT("Not right now. Let the moment finish first.");
	if (S->At(ES47Phase::Ringing) && ChapterNo(S) == 0) return TEXT("Not during the call and what follows it. The game saves again when the night moves on.");
	for (const TCHAR* K : { TEXT("ch3"), TEXT("ch4"), TEXT("ch5"), TEXT("ch6") })
	{
		if (S->ChapterKey == K) { const US47Chapter* C = S->Chapter(K); return C ? C->SaveBlock() : FString(); }
	}
	return FString();
}

S47J::FObj US47Flow::SnapshotCase() const
{
	// the case as it stands, for a save: null in the prologue, which keeps no case
	return ChapterNo(M->Story) >= 1 ? ComposeCase() : S47J::FObj();
}

S47J::FObj US47Flow::ComposeCase() const
{
	// One saved case for every chapter: a later chapter's state rides along once it has begun.
	const AS47Story* S = M->Story;
	const S47J::FObj Was = S47J::Obj(Carry, TEXT("case")); // as it was opened, for the parts not ported yet
	const S47J::FObj C = S47J::New();
	auto Part = [&](const TCHAR* Key, const TCHAR* Field, bool bAlways)
	{
		if (const US47Chapter* Ch = S ? S->Chapter(Key) : nullptr)
		{
			if (bAlways || S->Begun.Contains(Key)) { const S47J::FObj St = Ch->SaveState(); if (St.IsValid()) C->SetObjectField(Field, St); }
		}
		else if (Was.IsValid() && Was->HasField(Field)) C->SetField(Field, Was->TryGetField(Field));
	};
	Part(TEXT("ch1"), TEXT("s"), true);
	Part(TEXT("ch2"), TEXT("ch2"), false);
	Part(TEXT("ch3"), TEXT("ch3"), false);
	Part(TEXT("ch4"), TEXT("ch4"), false);
	Part(TEXT("ch5"), TEXT("ch5"), false);
	// ns: Night Shift's papers, relay K3, the console log and the jacket (NightShiftState); the threads
	// (ln), the array's runs and Hondo's telex as the opened case had them until they are ported
	const S47J::FObj WasNs = S47J::Obj(Was, TEXT("ns"));
	const S47J::FObj Ns = S47J::New();
	if (S)
	{
		const FS47NightState& N = S->Night();
		S47J::Set(Ns, TEXT("papers"), N.Papers);
		S47J::Set(Ns, TEXT("k3Open"), N.bK3Open);
		TArray<TSharedPtr<FJsonValue>> Log;
		for (const FS47LogLine& L : N.Log) Log.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{ MakeShared<FJsonValueNumber>(L.Clock), MakeShared<FJsonValueString>(L.Text) }));
		Ns->SetArrayField(TEXT("log"), Log);
		if (N.SolveAt) S47J::Set(Ns, TEXT("solveAt"), *N.SolveAt); else S47J::SetNull(Ns, TEXT("solveAt"));
		S47J::Set(Ns, TEXT("jacket"), N.bJacket);
	}
	S47J::Set(Ns, TEXT("ln"), S47J::Strs(WasNs, TEXT("ln")));
	if (S47J::Has(WasNs, TEXT("array"))) Ns->SetField(TEXT("array"), WasNs->TryGetField(TEXT("array")));
	if (S47J::Has(WasNs, TEXT("hondo"))) S47J::Set(Ns, TEXT("hondo"), S47J::Num(WasNs, TEXT("hondo")));
	S47J::Set(C, TEXT("ns"), Ns);
	S47J::Set(C, TEXT("notes"), S ? S->Notes() : TArray<FString>());
	S47J::Set(C, TEXT("clock"), S ? S->Clock() : 0.0);
	// where the cards lie on the case board
	S47J::FObj Board = SaveBoard ? SaveBoard() : S47J::Obj(Was, TEXT("board"));
	if (!Board.IsValid()) { Board = S47J::New(); S47J::Set(Board, TEXT("pos"), S47J::New()); }
	S47J::Set(C, TEXT("board"), Board);
	return C;
}

// ---------- the notebook ----------

void US47Flow::OpenNotebook()
{
	// Prologue.openNotebook, with main.ts's map, kit and people; the case board from the journal at any time
	const AS47Story* S = M->Story;
	if (!S) return;
	static const TMap<FString, FString> Heading = { { TEXT("ch1"), TEXT("Chapter one") }, { TEXT("ch2"), TEXT("Chapter two") }, { TEXT("ch3"), TEXT("Chapter three") },
		{ TEXT("ch4"), TEXT("Chapter four") }, { TEXT("ch5"), TEXT("Chapter five") }, { TEXT("ch6"), TEXT("Chapter six") } };
	const FString* H = Heading.Find(S->ChapterKey);
	TWeakObjectPtr<US47UI> UI = M->UI;
	TFunction<void()> OnBoard;
	if (M->Board.Open)
	{
		TWeakObjectPtr<US47Main> Main = M;
		OnBoard = [Main] { if (Main.IsValid() && Main->Board.Open) Main->Board.Open(FString(), FString()); };
	}
	// (game.openDoc: T. Vega's field book opens in a view of its own, story/fieldBook.ts, not ported yet)
	M->UI->Notebook(S->Tasks(), S->Notes(), S->Docs(), [UI](const FS47Doc& Doc) { if (UI.IsValid()) UI->Document(Doc, [] {}); },
		H ? *H : FString(TEXT("Tonight")), OnBoard, NotebookExtras());
}

FS47NbExtras US47Flow::NotebookExtras() const
{
	// main.ts game.notebookExtras
	FS47NbExtras X;
	const FS47MapView V = MapView();
	X.Map = V;
	X.MapDescription = S47AreaMap::Description(V);
	if (S47AreaMap::LinesMeet(V)) X.MapNote = MEET_NOTE;
	X.Kit = KitItems();
	X.People = PeopleItems();
	X.bTouch = false;
	return X;
}

FS47MapView US47Flow::MapView() const
{
	const AS47Story* S = M->Story;
	FS47MapView V;
	V.At = MapAt();
	V.bStation = ChapterNo(S) >= 3;
	V.bOldRoad = ChapterNo(S) >= 5;
	// (mapMarks: the night's events and the time away, story/townEvents.ts and abduction.ts, not ported yet)
	const US47Chapter* Ch5 = S ? S->Chapter(TEXT("ch5")) : nullptr;
	V.bLineC = (Ch5 && S->Begun.Contains(TEXT("ch5")) && S47J::Bool(Ch5->SaveState(), TEXT("p13"))) || ChapterNo(S) >= 6;
	return V;
}

FS47MapAt US47Flow::MapAt() const
{
	// main.ts mapAt: where the player is on the pencil map (on foot: the truck is not ported yet)
	const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector, Q = P;
	const FString Area = M->Web ? M->Web->Area() : FString(TEXT("saro"));
	FS47MapAt A;
	if (Area == TEXT("road")) { A.Spot = TEXT("highway"); A.M = Q.Z - 0 + 300; A.bM = true; return A; } // ROAD_ORIGIN.z 0, SARO_IN_ROAD.z -300
	if (Area == TEXT("diner")) { A.Spot = TEXT("diner"); return A; } // (world.roadStopAt: not ported yet)
	if (Area == TEXT("room6")) { A.Spot = TEXT("motel"); return A; }
	if (Area == TEXT("roswell")) { A.Spot = TEXT("oldroad"); A.Mi = 0; A.bMi = true; return A; } // (world.oldRoad.where: not ported yet)
	if (Area == TEXT("station01")) { A.Spot = TEXT("station01"); return A; }
	if ((M->Web && M->Web->Rect(MOTEL_OFFICE).Contains(Q.X, Q.Z)) || (Outside(Q) && AtMotel(Q))) { A.Spot = TEXT("motel"); return A; }
	// on the highway at SARO, or down it, metres south of SARO
	if (Outside(Q) || Q.Z > 64) { A.Spot = TEXT("highway"); A.M = FMath::Max(0.0, Q.Z); A.bM = true; return A; }
	if ((M->Player && M->Player->FloorY < -0.3) || OnRoad(P) || (M->Web && M->Web->Rect(TEXT("yard.zone.truckPad")).Contains(P.X, P.Z))) { A.Spot = TEXT("yard"); return A; }
	A.Spot = TEXT("saro");
	return A;
}

TArray<FString> US47Flow::TownSeen() const
{
	return S47J::Strs(Carry, TEXT("town"));
}

TArray<FS47KitItem> US47Flow::KitItems() const
{
	// main.ts kitItems
	const AS47Story* S = M->Story;
	const TArray<FS47Doc> Docs = S ? S->Docs() : TArray<FS47Doc>();
	int32 Photos = 0, Frames = 0;
	for (const FS47Doc& D : Docs)
	{
		if (D.Kind == TEXT("photo") && !D.Image.IsEmpty()) ++Photos;
		if (D.Id.StartsWith(TEXT("frame")) || D.Id.StartsWith(TEXT("away"))) ++Frames;
	}
	const int32 Papers = Docs.Num() - Photos;
	// (the truck is not ported yet: it stands on its pad)
	const FString TruckAt = TEXT("On the pad by the service yard");
	const TArray<FString> Town = TownSeen();
	const bool bCh1 = ChapterNo(S) >= 1;
	TArray<FS47KitItem> Kit;
	Kit.Add({ TEXT("Notebook"), TEXT("Tab opens it. Tasks, notes, findings, papers, photographs, this map."), FString(), TEXT("notebook"), FString() });
	if (bCh1) Kit.Add({ TEXT("SARO jacket"), TEXT("From the hook by the east door. The reflective band across the back."), TEXT("On"), TEXT("jacket"), FString() });
	if (HaveCamera && HaveCamera())
	{
		Kit.Add({ TEXT("Field camera"), TEXT("The SARO field camera. C raises it, Space takes the picture: the case frames on sheet film, anything else out of the instant back, into the album."),
			FString::Printf(TEXT("%d case frames%s"), Frames, RollText ? *(TEXT(", ") + RollText()) : TEXT("")), TEXT("camera"), FString() });
	}
	// (the flashlight, core/Flashlight.ts: not ported yet)
	if (bCh1) Kit.Add({ TEXT("Truck keys"), TEXT("SARO 07, the service truck. The keys stay in it."), TruckAt, TEXT("keys"), TEXT("SARO 07") });
	if (Papers) Kit.Add({ TEXT("Papers"), TEXT("Folded into the notebook. Papers and Findings have them."), FString::FromInt(Papers), TEXT("papers"), FString() });
	if (Town.Contains(TEXT("nickel"))) Kit.Add({ TEXT("A nickel"), TEXT("From the coin return of the payphone in Caliche. Bright as the day it was struck."), TEXT("1947"), TEXT("nickel"), FString() });
	// (story/archivePhone.ts: from the fire drawer behind the STATION 01 box)
	if (Town.Contains(TEXT("arc:reel")))
	{
		Kit.Add({ TEXT("Reel 047"), TEXT("A reel of tape in a grey box, from the fire drawer behind the STATION 01 box. NOT FOR CATALOGUE, in pencil. It fits the deck on the signal processor."),
			Town.Contains(TEXT("arc:heard")) ? TEXT("Heard") : TEXT("10-AUG-79"), FString(), FString() });
	}
	return Kit;
}

TArray<FS47PersonCard> US47Flow::PeopleItems() const
{
	// main.ts peopleItems: the people in the case as the night meets them, with their faces where
	// the portraits are (core/portraits.ts: Content/WebData/art/portrait_<id>)
	const AS47Story* S = M->Story;
	auto State = [S](const TCHAR* Key) { const US47Chapter* C = S ? S->Chapter(Key) : nullptr; return C && S->Begun.Contains(Key) ? C->SaveState() : S47J::FObj(); };
	TArray<FS47PersonCard> P;
	P.Add({ TEXT("Dale"), TEXT("SARO, the shift before"), TEXT("Left the shift log on the desk and went home. \"Don't break anything. D.\""), TEXT("portrait_dale") });
	if (ChapterNo(S) >= 2) P.Add({ TEXT("Dr. Evelyn Ward"), TEXT("SARO, duty officer"), TEXT("On the line from the main site. Wants the run checked and logged by the book."), TEXT("portrait_ward") });
	if (S47J::Bool(State(TEXT("ch2")), TEXT("called")) || ChapterNo(S) >= 3) P.Add({ TEXT("N. Vega"), TEXT("Sierra Motor Court, room 6"), TEXT("Key holder for STATION 01. Signed both versions of the 1947 record."), TEXT("portrait_nora") });
	if (S47J::Bool(S47J::Obj(State(TEXT("ch4")), TEXT("topics")), TEXT("tomas"))) P.Add({ TEXT("Tomás Vega"), TEXT("Survey crew, 1947"), TEXT("Her brother. Walked out to the closing point with the last reading."), TEXT("portrait_tomas_1947") });
	if (S47J::Bool(State(TEXT("ch5")), TEXT("arrived")))
	{
		P.Add({ TEXT("The waitress"), TEXT("Mesa Diner, all night"), TEXT("Has watched the lights over the mesa since she was eleven."), TEXT("portrait_waitress") });
		P.Add({ TEXT("The driver"), TEXT("Mesa Diner, at the counter"), TEXT("His engine died on the old road last October."), TEXT("portrait_driver") });
	}
	// (townPeople(townSeen): the people of Caliche, story/caliche.ts, not ported yet)
	return P;
}

// ---------- tests ----------

bool US47Flow::OpenTestView(const FString& View)
{
	// what a click on the title or in the pause menu would open (the screenshot runs)
	if (View == TEXT("opening")) { RequestNewNight(FMath::Max(1, Saves.FreeCase())); return true; }
	if (View.StartsWith(TEXT("notebook:"))) { M->UI->NotebookAt(View.Mid(9)); OpenNotebook(); return true; }
	if (View == TEXT("continue")) { const TOptional<FS47SaveMeta> C = Saves.ContinueSave(); if (C) OpenSave(*C); return true; }
	// New night and Begin night on the opening card; the pause menu's Quit to title
	if (View == TEXT("newnight")) { RequestNewNight(FMath::Max(1, Saves.FreeCase())); Opening->Begin(); return true; }
	if (View == TEXT("totitle")) { AutosaveNow(); ToTitle(); return true; }
	if (View == TEXT("settings")) { FS47PauseOpts S = Settings(); S.Title = TEXT("Settings"); S.bSettingsOnly = true; S.OnResume = [] {}; S.OnTitle = [] {}; M->UI->Pause(S); return true; }
	if (View == TEXT("save") || View == TEXT("load"))
	{
		FS47SaveMenuOpts S;
		S.Mode = View; S.Metas = Saves.List(); S.ActiveCase = M->Mode == EMode::Title ? Saves.Active() : CaseId; S.Block = SaveBlock(); S.bFallback = Saves.bFallback;
		S.OnSave = [this](int32 Slot, FString& Err) { return WriteSave(TEXT("manual"), Slot, Err); };
		S.OnLoad = [this](const FS47SaveMeta& Mt) { OpenSave(Mt); };
		S.OnDelete = [this](const FS47SaveMeta& Mt, FString& Err) { return Saves.Remove(Mt.Id, Err); };
		S47SaveMenu::Open(M->UI, S);
		return true;
	}
	if (View == TEXT("card") && M->Story)
	{
		// the prologue's card, as the night goes on into chapter one
		M->Story->ChapterBreak({ TEXT("NIGHT SHIFT // EVERY DISH AT AZ 026"),
			TEXT("The array moved without a command. The phone played the room before it happened. The night is not over."),
			TEXT("CHAPTER ONE"), TEXT("THE SECOND EXPOSURE"), [] {} });
		return true;
	}
	return false;
}

// ---------- self test ----------

bool US47Flow::SelfTest(TArray<FString>& Out)
{
	bool bOk = true;
	auto Check = [&](const TCHAR* Name, bool bPass, const FString& Got = FString())
	{
		Out.Add(bPass ? FString::Printf(TEXT("ok  flow: %s"), Name) : FString::Printf(TEXT("FAIL flow: %s %s"), Name, *Got));
		bOk &= bPass;
	};
	// SaveMenu.ts played()
	Check(TEXT("played 0"), S47Css::Played(0) == TEXT("under a minute"));
	Check(TEXT("played 59"), S47Css::Played(59) == TEXT("under a minute"));
	Check(TEXT("played 60"), S47Css::Played(60) == TEXT("1 min"));
	Check(TEXT("played 3599"), S47Css::Played(3599) == TEXT("59 min"));
	Check(TEXT("played 3600"), S47Css::Played(3600) == TEXT("1 h 0 min"));
	Check(TEXT("played 7385"), S47Css::Played(7385) == TEXT("2 h 3 min"), S47Css::Played(7385));
	FS47SaveMeta Ms; Ms.Kind = TEXT("auto"); Ms.Slot = 1;
	Check(TEXT("kind name"), S47SaveMenu::KindName(Ms) == TEXT("AUTOSAVE 2"));
	const FString W = S47SaveMenu::When(1792670700000.0); // 22 October 2026, 12:05 UTC (the hour by the local zone)
	Check(TEXT("when"), W.StartsWith(TEXT("Oct 22, ")) && W.Len() == 13, W);
	// the opening card's five paragraphs
	Check(TEXT("opening"), FS47Opening::Paragraphs().Num() == 5 && FS47Opening::Paragraphs()[3] == TEXT("The record calls it interference."));
	// a finding's line in the notebook
	FS47Doc D;
	D.Id = TEXT("p04"); D.Page = TEXT("P04  SUPPORTED / The service copy leaves out C.\n\nThe original has A, B and C.");
	Check(TEXT("excerpt"), SS47Notebook::Excerpt(D) == TEXT("The service copy leaves out C. The original has A, B and C."), SS47Notebook::Excerpt(D));
	D.Id = TEXT("e07"); D.Transcript = FString::ChrN(140, TEXT('a')) + TEXT(" bbbbbbbbbbbbbbbbbbbb");
	Check(TEXT("excerpt cut"), SS47Notebook::Excerpt(D) == FString::ChrN(140, TEXT('a')) + TEXT("..."), SS47Notebook::Excerpt(D));
	// the pencil map's places
	FS47MapAt A; A.Spot = TEXT("highway"); A.M = 690;
	Check(TEXT("map highway"), S47AreaMap::SpotXY(A) == FVector2f(420, 261));
	A.Spot = TEXT("track"); A.T = 0.5;
	Check(TEXT("map track"), S47AreaMap::SpotXY(A).Equals(FVector2f((408 + 196) / 2.f, (360 + 372) / 2.f), 0.01f));
	FS47MapView V; V.At = A;
	Check(TEXT("map description"), S47AreaMap::Description(V).Last() == TEXT("The red cross marks me on the track to STATION 01.") && S47AreaMap::Description(V).Contains(TEXT("The old road is closed.")));
	// SARO's places (Crossing.ts, Grounds.ts)
	Check(TEXT("service road"), OnRoad(FVector(0, 0, 24.5)) && !OnRoad(FVector(0, 0, 10)));
	Check(TEXT("outside"), Outside(FVector(-20, 0, 10)) && !Outside(FVector(0, 0, 0)));
	Check(TEXT("at the motel"), AtMotel(FVector(-40, 0, 680)) && !AtMotel(FVector(-40, 0, 100)));
	Check(TEXT("highway south"), OnHighway(FVector(-25, 0, 100)) && !OnHighway(FVector(-30, 0, 680)));
	return bOk;
}

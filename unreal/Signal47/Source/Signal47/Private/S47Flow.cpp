#include "S47Flow.h"
#include "S47RoomDecor.h"
#include "S47EastWing.h"
#include "SS47FreeformStart.h"
#include "S47TitleEraseSound.h"
#include "UnrealClient.h"
#include "S47FieldRadio.h"
#include "S47Freeform.h"
#include "S47Handling.h"
#include "S47TownUse.h"
#include "S47Townsfolk.h"
#include "S47Visitors.h"
#include "S47Rhythm.h"
#include "S47FreeformWorld.h"
#include "S47FreeformDirector.h"
#include "S47HighwayUfo.h"
#include "S47FarmWorld.h"
#include "S47Horror.h"
#include "S47Observing.h"
#include "S47Spine.h"
#include "SS47Dev.h"
#include "S47DevEvents.h"
#include "S47Taken.h"
#include "Misc/App.h"
#include "HAL/PlatformProcess.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "S47Evening.h"
#include "S47Substation.h"
#include "S47Office.h"
#include "S47RoomLights.h"
#include "S47SkyVisitors.h"
#include "S47WorldLife.h"
#include "S47Anomaly.h"
#include "S47AnomalyWorld.h"

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
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "S47Audio.h"
#include "S47CB.h"
#include "S47Chapter.h"
#include "S47Chapter1.h"
#include "S47Chapter2.h"
#include "S47Chapter6.h"
#include "S47FieldCamera.h"
#include "S47HUD.h"
#include "S47HudGauges.h"
#include "S47Image.h"
#include "S47Keys.h"
#include "S47Mirrors.h"
#include "S47Kit.h"
#include "S47Album.h"
#include "S47Flashlight.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Radio.h"
#include "S47ReadAloud.h"
#include "S47Roadside.h"
#include "S47TripRadio.h"
#include "S47Score.h"
#include "S47World.h"
#include "S47Phones.h"
#include "S47Picture.h"
#include "S47SaroNight.h"
#include "S47ArrayBack.h"
#include "S47Away.h"
#include "S47TownLayer.h"
#include "S47Caliche.h"
#include "S47NpcDialogue.h"
#include "S47EarlDialogue.h"
#include "S47TownEvents.h"
#include "S47Town.h"
#include "S47RoadTerrain.h"
#include "S47Lines.h"
#include "S47Opening.h"
#include "S47Story.h"
#include "S47Theme.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "SS47AreaMap.h"
#include "SS47FieldMap.h"
#include "S47Places.h"
#include "S47Tape.h"
#include "S47Happenings.h"
#include "S47Predecessor.h"
#include "SS47Ending.h"
#include "SS47Notebook.h"
#include "SS47SaveMenu.h"
#include "SS47Screens.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Flow, Log, All);

using EMode = US47Main::EMode;

namespace
{
	class SS47LeaveWarning : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SS47LeaveWarning) : _UI(nullptr) {}
			SLATE_ARGUMENT(US47UI*, UI)
			SLATE_ARGUMENT(FString, Reason)
			SLATE_ARGUMENT(FString, ReturnTo)
			SLATE_ARGUMENT(TFunction<void()>, OnCancel)
			SLATE_ARGUMENT(TFunction<void()>, OnLeave)
		SLATE_END_ARGS()
		void Construct(const FArguments& A)
		{
			Cancel = A._OnCancel; Leave = A._OnLeave;
			ChildSlot[S47Kit::Shell(A._UI, TEXT("LEAVE THIS NIGHT?"), S47Kit::Col({
				S47Kit::Text(TEXT("Your current progress could not be saved. ") + A._Reason, -1),
				S47Kit::Text(A._ReturnTo),
				S47Kit::ButtonLive(TAttribute<FString>::CreateLambda([this] { return FString(Selected == 0 ? TEXT("> BACK TO PAUSE") : TEXT("BACK TO PAUSE")); }), [this] { const auto Fn = Cancel; if (Fn) Fn(); }, true),
				S47Kit::ButtonLive(TAttribute<FString>::CreateLambda([this] { return FString(Selected == 1 ? TEXT("> QUIT WITHOUT SAVING") : TEXT("QUIT WITHOUT SAVING")); }), [this] { const auto Fn = Leave; if (Fn) Fn(); }),
				S47Kit::Small(TEXT("UP / DOWN: CHOOSE   ENTER / A: CONFIRM   ESC / B: BACK"))
			}))];
		}
		virtual bool SupportsKeyboardFocus() const override { return true; }
		virtual void Tick(const FGeometry& G, const double InCurrentTime, const float InDeltaTime) override
		{
			SCompoundWidget::Tick(G, InCurrentTime, InDeltaTime);
			S47Theme::KeepFocus(SharedThis(this));
		}
		virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& E) override
		{
			const FKey K = E.GetKey();
			if (K == EKeys::Up || K == EKeys::Down || K == EKeys::Gamepad_DPad_Up || K == EKeys::Gamepad_DPad_Down) { Selected = 1 - Selected; return FReply::Handled(); }
			TFunction<void()> Fn;
			if (K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right) Fn = Cancel;
			else if (K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom) Fn = Selected ? Leave : Cancel;
			else return FReply::Unhandled();
			if (Fn) Fn();
			return FReply::Handled();
		}
	private:
		int32 Selected = 0;
		TFunction<void()> Cancel, Leave;
	};

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

	// SAVE-SARO-20261108 / M7-E: a Freeform case that could be saved at all (not a developer run, not a contact, whose own words come
	// first) and the player not inside SARO: nothing but the departure is filed (S47SaroSave::StationBlock)
	bool CaseInPlay(const US47Main* M, int32 CaseId, bool bDevRun)
	{
		return M && M->IsFreeform() && M->Mode == EMode::Play && CaseId && !bDevRun && !(M->Freeform && M->Freeform->Dead());
	}
	bool AwayFromSaro(const US47Main* M, int32 CaseId, bool bDevRun) { return CaseInPlay(M, CaseId, bDevRun) && !S47SaroSave::InStation(M); }

	// a departure that could not be filed: the stamp of LOG FILED in red (SS47Hud, S47HudGauges::FiledRefused), and a dull dry thud
	// of a stamp that did not take. Critic round 5: Sfx goes by the buffer's name, "thudSoft"; "thud_soft" is its file and played nothing.
	void NotFiled(US47Main* M)
	{
		if (!M) return;
		if (M->UI) M->UI->Filed(S47SaroSave::NotFiledLine());
		if (M->Audio) M->Audio->Sfx(TEXT("thudSoft"), FS47Sound{ .Gain = .4f, .Rate = 1.35f, .Lp = 1600.f });
	}

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
	M->InitRadio();
	LoadStore();
	Saves.Init();
	// the settings shared by the pause menu and the title screen; all kept on this device
	// Old profiles retain their existing lighting; advanced preferences are device-wide.
	Quality = S47J::Str(Store, TEXT("quality"), TEXT("high"));
	Graphics = FS47GraphicsSettings::FromJson(S47J::Obj(Store, TEXT("graphics")), Quality);
	FString QualityOverride;
	if (FParse::Value(FCommandLine::Get(), TEXT("S47Quality="), QualityOverride)) Graphics = FS47GraphicsSettings::ForPreset(QualityOverride);
	Quality = Graphics.LegacyQuality();
	bKinski = S47J::Bool(Store, TEXT("kinskiReadAloud"), false);
	bArtBell = S47J::Bool(Store, TEXT("artBellRadioHost"), true);
	bSerling = S47J::Bool(Store, TEXT("serlingNarrator"), true);
	bCharacters = S47J::Bool(Store, TEXT("characterVoices"), true);
	if (M->Audio) { M->Audio->bCharacterVoices = bCharacters; M->Audio->bKinskiCalls = bKinski; }
	// main.ts: the readers (core/readAloud.ts): Kinski's for papers and conversations, opt-in, and the
	// narrator, Serling's, on unless switched off; ui.readAloud and ui.narrator
	M->UI->ReadAloud = MakeShared<FS47ReadAloud>(M->Audio);
	M->UI->ReadAloud->SetEnabled(bKinski);
	M->UI->Narrator = MakeShared<FS47ReadAloud>(M->Audio, S47Reading::SerlingBank());
	M->UI->Narrator->SetEnabled(bSerling);
	// stopNarration(except): every voice but the one about to speak; onReadingStart, and the phone's call
	// (the character voices' and Earl's starts are wired with them: Chapter4's, TownLayer's)
	M->UI->OnReadingStart = [this](FS47ReadAloud* Reader)
	{
		StopNarration(Reader ? Reader : M->UI->ReadAloud.Get());
		if (M->Audio) M->Audio->StopPhoneCalls();
		if (M->CB.IsValid()) M->CB->Stop();
	};
	if (M->UI->NpcDialogue.IsValid())
	{
		// ui.onCharacterStart: stopNarration(npcDialogue), as well as the calls and the CB it stopped
		TFunction<void()> Before = M->UI->NpcDialogue->OnCharacterStart;
		M->UI->NpcDialogue->OnCharacterStart = [this, Before] { StopNarration(M->UI->NpcDialogue.Get()); if (Before) Before(); };
	}
	if (M->Audio)
	{
		// audio.onPhoneCall: stopNarration() (the readers among them) and cb.stop()
		TFunction<void()> Before = M->Audio->OnPhoneCall;
		M->Audio->OnPhoneCall = [this, Before] { StopNarration(nullptr); if (Before) Before(); };
	}
	// ui.canDescribe: in play, nothing cinematic, no call, the CB quiet, no character or Earl speaking
	M->UI->CanDescribe = [this]
	{
		if (M->Mode != EMode::Play || (M->Story && M->Story->IsCinematic()) || (M->Audio && M->Audio->PhonePlaying()) || (M->CB.IsValid() && M->CB->bBusy)) return false;
		const TSharedPtr<FS47NpcDialogue> Npc = M->UI->NpcDialogue;
		const TSharedPtr<FS47EarlDialogue> Earl = M->UI->EarlDialogue;
		return !(Npc.IsValid() && (Npc->Playing() || Npc->Loading())) && !(Earl.IsValid() && (Earl->Playing() || Earl->Loading()));
	};
	Volume = float(S47J::Num(Store, TEXT("vol"), 0.8));
	MusicLevel = float(S47J::Num(Store, TEXT("music"), 0.7));
	RadioLevel = float(S47J::Num(Store, TEXT("radioVol"), 1));
	bRadioCast = S47J::Bool(Store, TEXT("radioCallerVoices"), true);
	bAbductOn = S47J::Bool(Store, TEXT("abductions"), true);
	Sens = float(S47J::Num(Store, TEXT("sens"), 1));
	bInvertY = S47J::Bool(Store, TEXT("invertY"), false);
	PadLookSpeed = FMath::Clamp(float(S47J::Num(Store, TEXT("padLookSpeed"), 1)), .3f, 2.5f);
	bPadInvertY = S47J::Bool(Store, TEXT("padInvertY"), false);
	BaseFov = float(S47J::Num(Store, TEXT("fov"), 70));
	bLargeText = S47J::Bool(Store, TEXT("largeText"), false);
	if (M->UI) M->UI->bLargeText = bLargeText;
	bCalmFlash = S47J::Bool(Store, TEXT("calmFlash"), false);
	bStillShots = S47J::Bool(Store, TEXT("stillShots"), false);
	bHeadMotion = S47J::Bool(Store, TEXT("headMotion"), true);
	bVehicleMirrors = S47J::Bool(Store, TEXT("vehicleMirrors"), true);
	{
		bool bRun = bVehicleMirrors;
		FParse::Bool(FCommandLine::Get(), TEXT("S47Mirrors="), bRun);
		S47Mirrors::SetEnabled(bRun);
	}
	// An explicit saved look survives quality changes. CLI overrides affect this run only.
	const FString Chosen = S47J::Str(Store, TEXT("picture"));
	PictureStrength = S47Picture::Strength(float(S47J::Num(Store, TEXT("pictureStrength"), 1)));
	FParse::Value(FCommandLine::Get(), TEXT("S47PictureStrength="), PictureStrength);
	PictureStrength = S47Picture::Strength(PictureStrength);
	FString Cmd;
	ApplyPicture(FParse::Value(FCommandLine::Get(), TEXT("S47Picture="), Cmd) ? Cmd : !Chosen.IsEmpty() ? Chosen : S47Picture::DefaultForQuality(Quality), false);
	ApplyGraphics(Graphics, false);
	ApplyVolume();
	if (M->Audio) { M->Audio->SetMusic(MusicLevel); M->Audio->SetRadio(RadioLevel); }
	if (M->Radio) { M->Radio->SetHostVoice(bArtBell); M->Radio->SetCallerVoice(bRadioCast); }
	ApplyLook();
	// the Unreal version's own settings: the brightness, and the keys as the player has bound them
	Brightness = float(S47J::Num(Store, TEXT("brightness"), 0.5));
	ApplyBrightness();
	S47Keys::Load(S47J::Obj(Store, TEXT("keys")));
	// HUD-D-20261109: the HUD's colours, as the player chose them (Settings > Picture > HUD color)
	S47HudGauges::SetHudColor(S47HudGauges::HudColorFrom(S47J::Str(Store, TEXT("hudColor"))));

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
	Opening = MakeShared<FS47Opening>(M->UI->Narrator, MoveTemp(H));

	// the host's hooks: Prologue deps loadCase, and game.onCheckpoint (a new safe point, 02:13 or a
	// new chapter, starts a new autosave generation); the chapters' saveCase is M->RequestAutosave()
	if (AS47Story* Host = M->Story)
	{
		Host->LoadCase = [this] { return Restoring.IsValid() ? S47J::Obj(Restoring, TEXT("case")) : S47J::FObj(); };
		Host->OnCheckpoint = [this](const FString&) { RequestAutosave(true); };
		// main.ts chapterBreak, once the next chapter has begun behind the black: the card with what was
		// closed and the night's clock. It ends with the host's 6.4 s, the black with it; read by the
		// narrator, not before the reading has ended and the title has had 3.2 s of its own (finish)
		Host->OnChapterCard = [this](const AS47Story::FBreak& B)
		{
			FS47ChapterCardOpts C;
			C.Closed = B.Closed; C.Recap = B.Recap; C.Next = B.Next.StartsWith(TEXT("CHAPTER "), ESearchCase::IgnoreCase) ? FString() : B.Next; C.Title = B.Title;
			C.Clock = S47Text::ClockText(M->Story->Clock(), false);
			M->UI->Fade(true, FString());
			struct FCardRun { bool bSpoken = false, bTitleWaiting = false, bTitleReady = false; };
			const TSharedRef<FCardRun> Run = MakeShared<FCardRun>();
			M->UI->ChapterCard(&C, [Run] { Run->bSpoken = true; });
			M->Story->ChapterCardDone = [this, Run]
			{
				const FString& Stage = M->UI->ChapterStage;
				if ((Stage == TEXT("b") || Stage == TEXT("text-b") || Stage == TEXT("error-b")) && !Run->bTitleWaiting)
				{
					Run->bTitleWaiting = true;
					M->Story->After(3.2, [Run] { Run->bTitleReady = true; });
				}
				if (!Run->bSpoken || (Run->bTitleWaiting && !Run->bTitleReady)) return false;
				UE_LOG(LogS47Flow, Display, TEXT("S47: chapter card done (stage %s)"), *Stage);
				M->UI->ChapterCard(nullptr);
				return true;
			};
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
	// (the keys as bound in Settings > Controls, S47Keys: Tab and N, M, I, H, Escape and P, V unless changed)
	M->OnAction(TEXT("notebook"), Notebook(nullptr));
	// the map, the kit and the controls, straight to their tabs in the notebook
	M->OnAction(TEXT("map"), Notebook(TEXT("map")));
	M->OnAction(TEXT("kit"), Notebook(TEXT("kit")));
	M->OnAction(TEXT("controls"), Notebook(TEXT("controls")));
	// Escape or P: the pause menu (with the field camera raised, Escape lowers it: chapter one's own
	// key, registered before this one)
	auto Pause = [this] { if (M->Mode != EMode::Play) return false; OpenPause(); return true; };
	M->OnAction(TEXT("pause"), Pause);
	// F9: keep a developer start point here (the title's Developer lists them)
	M->OnAction(TEXT("devevents"), [this] { if (M->Mode != EMode::Play || !M->DevEvents) return false; M->DevEvents->Open(); return true; });
	M->OnAction(TEXT("devstart"), [this] { if (M->Mode != EMode::Play) return false; DevSave(); return true; });
	// V: the last description read again (ui.readLastDescription), not in a cinematic
	M->OnAction(TEXT("describe"), [this]
	{
		if (!FS47ReadAloud::bOfferInGame || M->Mode != EMode::Play) return false;
		if (!(M->Story && M->Story->IsCinematic())) M->UI->ReadLastDescription();
		return true;
	});

	// the window put away (main.ts visibilitychange and the lost pointer lock)
	if (FSlateApplication::IsInitialized()) ActivationHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this, &US47Flow::Activation);
}

void US47Flow::BeginDestroy()
{
 if(TitleEraseSound) TitleEraseSound->Stop();
	if (ActivationHandle.IsValid() && FSlateApplication::IsInitialized()) FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(ActivationHandle);
	Super::BeginDestroy();
}

void US47Flow::Boot()
{
	const bool bTitle = FParse::Param(FCommandLine::Get(), TEXT("S47Title"));
 if (FParse::Param(FCommandLine::Get(),TEXT("S47FreeformInWorldTest")) || FParse::Param(FCommandLine::Get(),TEXT("S47FreeformJourneyTest")) || FParse::Param(FCommandLine::Get(),TEXT("S47FreeformTest")) || FParse::Param(FCommandLine::Get(),TEXT("S47Freeform")))
 {
  FString User; FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
  if (User.StartsWith(TEXT("/tmp/"))) { FStart S; S.bDev=true; S.bFreeform=true; StartGame(S); return; }
 }
	if (HasTestFlag() && !bTitle)
	{
		// straight into play, as every test run starts (main.ts devStart: a run that is never saved,
		// unless -S47Case=N gives it a case to save into)
		int32 Case = 0;
		FParse::Value(FCommandLine::Get(), TEXT("S47Case="), Case);
		CaseId = FMath::Clamp(Case, 0, S47Saves::CASES);
		FString RouteGuard;
		if (FParse::Value(FCommandLine::Get(),TEXT("S47EveningRoute="),RouteGuard)) CaseId=0; // route controls never mutate a real case

		bDevRun = CaseId == 0;
		// Opt-in arrival acceptance uses the ordinary new-night lifecycle, including
		// its SC content cohort. Old developer fixtures retain their original route.
		FString HorrorArrivalRoute;
		if (FParse::Param(FCommandLine::Get(),TEXT("S47HorrorArrival")) &&
		 FParse::Value(FCommandLine::Get(),TEXT("S47EveningRoute="),HorrorArrivalRoute) &&
		 (HorrorArrivalRoute==TEXT("direct") || HorrorArrivalRoute==TEXT("burger")))
		{
		 FStart Start;Start.CaseId=0;Start.bDev=true;StartGame(Start);return;
		}
		if (CaseId) Saves.SetActive(CaseId);
		M->Mode = EMode::Play;
		M->Playtime = 0;
		bSoundUnlocked = true;
		PlayInput();
		// (startGame's: a new night for the night's layer, and its log listening once the night has begun)
		// (Night::Restore puts SARO's, the times away, the town's and the phones' back too, in main.ts's order)
		if (M->Anomaly) M->Anomaly->Data = FS47AnomalyData();
		if (M->Night) M->Night->Restore(S47J::FObj());
		FString Jump;
		if (FParse::Value(FCommandLine::Get(), TEXT("S47Jump="), Jump)) { M->Story->bEveningTimeline=FParse::Param(FCommandLine::Get(),TEXT("S47EveningTimeline")); M->Story->bOpeningHeld=false; M->Story->Jump(Jump); FString Spine; if (M->Spine && (FParse::Value(FCommandLine::Get(),TEXT("S47SpineContract="),Spine) || FParse::Value(FCommandLine::Get(),TEXT("S47SpineStart="),Spine))) { M->Spine->Begin(nullptr,true); if (Spine==TEXT("SC04")) M->Spine->TestOpenSC04(); if (Spine==TEXT("SC08")) M->Spine->TestOpenSC08(); if (Spine==TEXT("SC11")) M->Spine->TestOpenSC11(); if (Spine==TEXT("SC05")) { M->Spine->TestDoneSC04(); M->Story->SetClock(FMath::Max(M->Story->Clock(), FS47Spine::GateDone(FS47Spine::SC04))); } } }
		else if (FParse::Param(FCommandLine::Get(), TEXT("S47Evening")) || FParse::Value(FCommandLine::Get(), TEXT("S47Evening="), Jump) || FParse::Value(FCommandLine::Get(), TEXT("S47EveningRoute="), Jump))
		{
		 M->Story->bEveningTimeline=true; M->Story->Reset(); M->Evening->Begin();
		 FString Route;
		 if (!FParse::Value(FCommandLine::Get(),TEXT("S47EveningRoute="),Route)) M->Evening->BootstrapTest(Jump.IsEmpty()?TEXT("commute"):Jump);
		}
		else M->Story->Start();
		// then what is refiled after the story has begun, as the start from the title does
		if (M->SaroNight) M->SaroNight->Restored();
		if (M->Phones) M->Phones->Restored();
		if (M->Away) M->Away->Restored();
		if (M->TownLayer) M->TownLayer->Refile(S47J::FObj());
		if (M->Night) M->Night->Restored();
		if (M->Evening && !M->Evening->Enabled() && M->Story->Begun.Contains(TEXT("ch2"))) M->Evening->BeginInvestigation();
		if (M->Office) M->Office->Begin(nullptr, M->Story->bEveningTimeline && M->Evening && M->Evening->Opening());
		if (M->RoomLights) M->RoomLights->Begin();
		if (M->SkyVisitors) M->SkyVisitors->Begin(nullptr, M->Story->bEveningTimeline);
		if(M->FieldRadio) M->FieldRadio->Begin(nullptr);
		if (M->Life) M->Life->PassageReset(false);
		if (M->Anomaly) M->Anomaly->Begin(nullptr);
		if (M->AnomalyWorld) M->AnomalyWorld->Begin();
		if (auto* C1=Cast<US47Chapter1>(M->Story->Chapter(TEXT("ch1")))) C1->RestoreEarlyCamera();

		M->ResetRadio(M->Story ? M->Story->Clock() : 0);
		return;
	}
	// the first title: nothing of the night is heard yet (the room tone starts with the night), and
	// the station behind it is as the night begins (the Prologue's constructor resets it)
	if (M->Audio) M->Audio->Stop(TEXT("room"), 0.f);
	if (M->RoomLights) M->RoomLights->Stop();
	if (M->Observing) M->Observing->Stop();
	if (M->Spine) M->Spine->Stop();
	if (M->DevEvents) M->DevEvents->Stop();
	if (M->FieldRadio) M->FieldRadio->Stop();
	if (M->Horror) M->Horror->Stop();
	if (M->Evening) M->Evening->Stop();
	if (M->AnomalyWorld) M->AnomalyWorld->Reset();
	if (M->Story) M->Story->Reset();
	M->Mode = EMode::Title;
	ShowTitle();
}

void US47Flow::Step(double Dt)
{
 StoryTitleTest(Dt);
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
	if (M->IsFreeform() && M->Freeform && M->Freeform->Dead() && (!bDeathShown || !M->ModalOpen()))
 {
  bDeathShown=true;
  M->UI->Open(S47Kit::Shell(M->UI,TEXT("FREEFORM / CONTACT LOST"),S47Kit::Col({
   S47Kit::Text(TEXT("This assignment uses death and reload. Continue from the latest successful save.")),
   S47Kit::Button(TEXT("Load last save"),[this] { if (const auto Last=Saves.Latest(CaseId)) OpenSave(*Last); else ToTitle(); },true),
   S47Kit::Button(TEXT("Return to title"),[this] { ToTitle(); })
  }),800));
 }
 // Never replace the pre-contact save with a dead session.
 if (M->IsFreeform() && M->Freeform && M->Freeform->Dead()) return;
	// SAVE-SARO-20261108: leaving the station files the log once (before the routine autosave, which it then stands in for)
	StepDeparture(Dt);
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
	// Music on the title screen, from the first key or click: Tom's own "Signal 47 - menu" from the private
	// sound folder (Tom 5 Nov). "Signal to Noise" stays for the end, and for a clone without the private folder.
	if (M->Mode != EMode::Title || (Opening && Opening->Active())) return;
	// audio.loop('music', 'titleMusic', { dest: audio.music, gain: 0.6 }): the one before stops
	// Refreshing title controls must not restart the VHS texture.
	if (M->Audio) M->Audio->Stop(TEXT("music"), 0.05f);
	if (M->Audio)
	{
		const TCHAR* Own = TEXT("lic:signal47_menu");const bool bOwn = M->Audio->Buf.Contains(Own);
		// Mastered 9.4 LU louder than the old track: the same level in the room.
		FS47Sound O; O.DestNode = M->Audio->Music; O.Gain = bOwn ? 0.2f : 0.6f; M->Audio->LoopSfx(TEXT("music"), bOwn ? Own : TEXT("titleMusic"), O); M->Audio->TitleTexture(true);
	}
}

void US47Flow::StopMusic(float Fade)
{
	// audio.stop('music', fade): setTargetAtTime(0, t, fade), gone after six of it
	if (M->Audio) { M->Audio->Stop(TEXT("music"), Fade); M->Audio->TitleTexture(false); }
}

void US47Flow::StoryTitleTest(double Dt)
{
 if(FParse::Param(FCommandLine::Get(),TEXT("S47TitleTextureContract")))
 {
  if(StoryTitleTestStage<0)return;
  StoryTitleTestT+=Dt;
  const FString Proof=FPaths::ProjectSavedDir()/TEXT("TitleTest");
  static TArray<TSharedPtr<FJsonValue>> Evidence;
  auto Finish=[&](bool Good,const FString& Why)
  {
   IFileManager::Get().MakeDirectory(*Proof,true);auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("scope"),TEXT("title-texture-lifecycle-fixture"));
   S47J::Set(O,TEXT("method"),TEXT("Bounded title PCM selftest plus actual Flow MenuMusic/StopMusic/ToTitle and Main mode guard. Explicit Play-mode fixture, not a full StartGame or listening acceptance."));O->SetArrayField(TEXT("evidence"),Evidence);
   FFileHelper::SaveStringToFile(S47J::Write(O),*(Proof/TEXT("result.json")));StoryTitleTestStage=-1;FPlatformMisc::RequestExitWithStatus(false,Good?0:1);
  };
  auto Check=[&](bool Good,const FString& What){auto O=S47J::New();S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("check"),What);Evidence.Add(MakeShared<FJsonValueObject>(O));UE_LOG(LogS47Flow,Display,TEXT("S47 title texture %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),*What);if(!Good)Finish(false,What);return Good;};
  FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
  if(!User.StartsWith(TEXT("/tmp/S47-night-20261103-title-"))){Check(false,TEXT("isolated title contract profile required"));return;}
  if(StoryTitleTestT>30){Finish(false,TEXT("bounded title lifecycle timeout"));return;}
  if(!M||!M->Audio||!M->Web||!M->Web->IsLoaded()||bStarting||StoryTitleTestT<.3)return;
  auto Next=[&](int32 Stage){StoryTitleTestStage=Stage;StoryTitleTestT=0;};
  if(StoryTitleTestStage==0)
  {
   if(!Check(M->Mode==EMode::Title&&TitleEl.IsValid(),TEXT("native title is present before explicit audio unlock")))return;
   TArray<FString> Lines;const bool Good=US47Audio::TitleTextureSelfTest(Lines);for(const auto& Line:Lines)UE_LOG(LogS47Flow,Display,TEXT("S47 title texture PCM: %s"),*Line);
   if(!Check(Good,TEXT("bounded production PCM selftest: finite soft samples, single graph and silence after stop")))return;
   const auto Pass=SS47Title::SampleTitlePass(10.5f),Repeat=SS47Title::SampleTitlePass(45.5f);
   if(!Check(!SS47Title::SampleTitlePass(0).bVisible&&!SS47Title::SampleTitlePass(7.9f).bVisible&&!SS47Title::SampleTitlePass(13.f).bVisible&&!SS47Title::SampleTitlePass(42.9f).bVisible&&Pass.bVisible&&Repeat.bVisible&&Pass.Opacity>0&&Pass.Position.Equals(Repeat.Position,.0001f),TEXT("title pass sample is hidden outside five-second window and repeats after 35 seconds")))return;
   if(!Check(M->Audio->Ctx.IsValid(),TEXT("production audio context is ready before menu sound unlock")))return;
   if(!Check(M->Audio->Buf.Contains(TEXT("lic:signal47_menu"))&&M->Audio->Buf.Contains(TEXT("titleMusic")),TEXT("menu music and the kept title track are both in the sound bank")))return;
   bSoundUnlocked=true;MenuMusic();
   if(!Check(M->Audio->TitleTextureOn(),TEXT("actual title MenuMusic enables texture after explicit unlock")))return;
   ShowTitleAgain();ShowTitleAgain();Next(1);return;
  }
  if(StoryTitleTestStage==1)
  {
   if(!Check(TitleEl.IsValid()&&M->Audio->TitleTextureOn(),TEXT("repeated native menu refresh preserves enabled title texture")))return;
   IFileManager::Get().MakeDirectory(*Proof,true);FScreenshotRequest::RequestScreenshot(Proof/TEXT("title-entry.png"),true,false);Next(2);return;
  }
  if(StoryTitleTestStage==2)
  {
   StopMusic(0);if(!Check(!M->Audio->TitleTextureOn(),TEXT("production StopMusic disables title texture")))return;
   ShowTitleAgain();if(!Check(M->Audio->TitleTextureOn(),TEXT("title refresh restarts texture after explicit stop")))return;
   M->Mode=EMode::Play;Next(3);return; // Main::Step must clear texture after this Flow step.
  }
  if(StoryTitleTestStage==3)
  {
   if(!Check(!M->Audio->TitleTextureOn(),TEXT("Main frame guard stops texture outside title mode")))return;
   ToTitle();
   // Explicit activation event matches the existing title-erasure contract;
   // actual title Tick still enforces gate, visibility and window state.
   FSlateApplication::Get().ProcessApplicationActivationEvent(true);
   UE_LOG(LogS47Flow,Display,TEXT("S47 title texture fixture: synthetic Slate application activation; no human input claimed"));
   Next(4);return;
  }
  if(StoryTitleTestStage==4)
  {
   if(!Check(M->Mode==EMode::Title&&TitleEl.IsValid()&&M->Audio->TitleTextureOn(),TEXT("actual ToTitle restores native menu and texture on reentry")))return;
   FScreenshotRequest::RequestScreenshot(Proof/TEXT("title-reentry.png"),true,false);Next(5);return;
  }
  if(StoryTitleTestStage==5)
  {
   const auto Title=StaticCastSharedPtr<SS47Title>(TitleEl);if(!Title.IsValid()){Check(false,TEXT("native title retained while waiting for visible pass"));return;}
   const float Elapsed=Title->TitlePassElapsed();if(Elapsed<9.f)return;
   if(!Check(Elapsed<13.f&&SS47Title::SampleTitlePass(Elapsed).bVisible&&M->Audio->TitleTextureOn(),TEXT("actual visible native title clock reaches first UFO pass with texture still enabled")))return;
   FScreenshotRequest::RequestScreenshot(Proof/TEXT("title-ufo-pass.png"),true,false);Next(6);return;
  }
  if(StoryTitleTestStage==6){StopMusic(0);if(!Check(!M->Audio->TitleTextureOn(),TEXT("final lifecycle cleanup stops title texture")))return;Finish(true,TEXT("Title pass, texture PCM and native lifecycle passed."));return;}
  return;
 }
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47StoryTitleContract")) || StoryTitleTestStage<0) return;
 StoryTitleTestT+=Dt;
 if(!M || !M->Story || !TitleEl.IsValid() || StoryTitleTestT<1.)return;
 const auto Title=StaticCastSharedPtr<SS47Title>(TitleEl);
 auto Check=[&](bool Good,const TCHAR* What)
 {
  UE_LOG(LogS47Flow,Display,TEXT("S47: story title %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),What);
  if(!Good){StoryTitleTestStage=-1;FPlatformMisc::RequestExitWithStatus(false,1);}return Good;
 };
 FString Proof,User;FParse::Value(FCommandLine::Get(),TEXT("S47ProofDir="),Proof);FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
 if(!User.StartsWith(TEXT("/tmp/S47-story-title-")) || !Proof.StartsWith(TEXT("/home/tom/scratch-bm/Signal47-title-erasure-")))
 {Check(false,TEXT("contract requires an isolated temporary profile and explicit proof directory"));return;}
 auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Proof/(FString(Name)+TEXT(".png")),true,false);};
 if(StoryTitleTestStage==0)
 {
  if(!Check(!Saves.StoryCompleted() && !Saves.Title47Erased() && Title->Number47Opacity()==1,TEXT("fresh profile keeps all three layers of 47")))return;
  Shot(TEXT("before"));StoryTitleTestStage=1;StoryTitleTestT=0;return;
 }
 if(StoryTitleTestStage==1)
 {
  if(StoryTitleTestT<1.2)return;
  auto* Ch=Cast<US47Chapter6>(M->Story->Chapter(TEXT("ch6")));
  if(!Check(Ch!=nullptr,TEXT("registered story finale exists")))return;
  const FString Was=Ch->Stage; const auto Mode=M->Mode;const auto Session=M->SessionMode;
  CaseId=1;bDevRun=false;Ch->Stage=TEXT("end");M->Mode=EMode::End;M->SessionMode=ES47SessionMode::Freeform;
  if(!Check(!CompleteStoryTitle() && !Saves.StoryCompleted(),TEXT("Freeform cannot unlock the story title reward")))return;
  M->SessionMode=ES47SessionMode::Story;bDevRun=true;
  if(!Check(!CompleteStoryTitle(),TEXT("developer finale cannot unlock the reward")))return;
  bDevRun=false;Ch->Stage=TEXT("drive");
  if(!Check(!CompleteStoryTitle(),TEXT("entering the last chapter is not completion")))return;
  Ch->Stage=TEXT("end");
  if(!Check(CompleteStoryTitle() && Saves.StoryCompleted() && !Saves.Title47Erased(),TEXT("explicit completed-finale fixture records pending presentation")))return;
  Ch->Stage=Was;M->Mode=Mode;M->SessionMode=Session;CaseId=0;bDevRun=true;bSoundUnlocked=true;
  ShowTitleAgain();
  // This opt-in automation supplies a declared Slate activation event. A
  // Wayland compositor need not focus an unattended preview on creation.
  FSlateApplication::Get().ProcessApplicationActivationEvent(true);
  UE_LOG(LogS47Flow,Display,TEXT("S47: story title fixture: synthetic Slate application activation; no human input claimed"));
  StoryTitleTestStage=2;StoryTitleTestT=0;return;
 }
 if(StoryTitleTestStage==2)
 {
  if(Title->Erase47Elapsed()<1.45f)
  {if(StoryTitleTestT>12)Check(false,TEXT("visible active title did not advance its own presentation clock"));return;}
  if(!Check(Title->Erase47Started() && !Title->Erase47Done() && TitleEraseStarts==1,TEXT("47-only tear starts one synchronized original cue")))return;
  Shot(TEXT("glitch"));StoryTitleTestStage=3;StoryTitleTestT=0;return;
 }
 if(StoryTitleTestStage==3)
 {
  if(!Title->Erase47Done()){if(StoryTitleTestT>8)Check(false,TEXT("title erasure did not finish"));return;}
  if(!Check(Saves.Title47Erased() && Title->Number47Opacity()==0,TEXT("all number layers stay absent and presentation is retained")))return;
  Shot(TEXT("after"));StoryTitleTestStage=4;StoryTitleTestT=0;return;
 }
 if(StoryTitleTestStage==4)
 {
  auto Fresh=FS47SaveStore(Saves.Dir);Fresh.Init();
  if(!Check(Fresh.StoryCompleted() && Fresh.Title47Erased(),TEXT("fresh store reads permanent reward from disk")))return;
  ShowTitleAgain();const auto Reopened=StaticCastSharedPtr<SS47Title>(TitleEl);
  if(!Check(Reopened->Number47Opacity()==0 && TitleEraseStarts==1,TEXT("reopening the title keeps 47 absent without replay")))return;
  if(!Check(FS47TitleEraseSound::RenderTour(Proof/TEXT("title-sound.wav")),TEXT("normal and calm sound preview rendered")))return;
  UE_LOG(LogS47Flow,Display,TEXT("S47: story title PASS complete"));StoryTitleTestStage=-1;FPlatformMisc::RequestExitWithStatus(false,0);
 }
}

void US47Flow::RememberTitleErasure()
{
 if(TitleEl.IsValid()) TitleEraseResume=StaticCastSharedPtr<SS47Title>(TitleEl)->Erase47Elapsed();
 if(TitleEraseSound) TitleEraseSound->Stop();
}
bool US47Flow::CompleteStoryTitle()
{
 const auto* Chapter=M && M->Story?Cast<US47Chapter6>(M->Story->Chapter(TEXT("ch6"))):nullptr;
 if(!M || M->IsFreeform() || bDevRun || CaseId<1 || M->Mode!=EMode::End || !Chapter || Chapter->Stage!=TEXT("end")) return false;
 const bool Saved=Saves.MarkStoryCompleted();
 UE_LOG(LogS47Flow,Display,TEXT("S47: story completion reward %s"),Saved?TEXT("retained"):TEXT("write failed; will not overwrite unknown profile data"));
 return Saved;
}

void US47Flow::ShowTitle()
{
 RememberTitleErasure();
	// The first time the menu waits behind "Click or press any key", so that first key brings in
	// the music; back from a night the sound is already going and it starts at once. Not for the
	// tests, which click their way straight in.
	const bool bGate = !bSoundUnlocked && !SmokeTest();
	if (bSoundUnlocked) MenuMusic();
	const TOptional<FS47SaveMeta> Cont = Saves.ContinueSave();
	FS47TitleOpts O;
	O.bGate = bGate;
	O.bTouch = false;
 O.bHide47=Saves.Title47Erased(); O.bErase47=Saves.StoryCompleted() && !O.bHide47;
 O.Erase47InitialTime=TitleEraseResume;
 TWeakObjectPtr<US47Flow> Self=this;
 O.OnErase47Start=[Self]
 {
  if(!Self.IsValid() || !Self->bSoundUnlocked) return;
  if(!Self->TitleEraseSound)Self->TitleEraseSound=MakeShared<FS47TitleEraseSound>();
  if(Self->TitleEraseSound->Start(Self->M->Audio,Self->bCalmFlash))++Self->TitleEraseStarts;
 };
 O.OnErase47Done=[Self]
 {
  if(!Self.IsValid())return;
  const bool Saved=Self->Saves.MarkTitle47Erased();
  UE_LOG(LogS47Flow,Display,TEXT("S47: title 47 erasure %s"),Saved?TEXT("retained"):TEXT("not saved"));
 };
	// Continue uses the story name, including legacy numbered save metadata.
	O.Cont = Cont ? FString::Printf(TEXT("CASE %d \u00B7 %s \u00B7 %s \u00B7 %s"), Cont->CaseId, *(Cont->Chapter.StartsWith(TEXT("FREEFORM")) ? Cont->Chapter : S47Theme::ChapterLine(Cont->Chapter).Key), *Cont->Clock, *Cont->Place.ToUpper()) : FString();
	// Quit: the desktop's way out
	O.OnDev = [this] { OpenDev(); };
	O.OnQuit = [this] { UKismetSystemLibrary::QuitGame(M->Hud, M->Hud ? M->Hud->GetOwningPlayerController() : nullptr, EQuitPreference::Quit, false); };
	FString Version;
	if (!GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), Version, GGameIni)) GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), Version, GEngineIni);
	O.Version = FString::Printf(TEXT("UNREAL %s \u00B7 TAPE B"), Version.IsEmpty() ? TEXT("0.0") : *Version);
	O.OnContinue = [this] { const TOptional<FS47SaveMeta> C = Saves.ContinueSave(); if (C) OpenSave(*C); };
	O.OnFreeform = [this] { RequestNewFreeform(); };
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
	O.OnFlashPick = [this](bool bCalm) { bCalmFlash = bCalm; ApplyPicture(Picture, false); StoreSet(TEXT("calmFlash"), bCalm); };
	O.OnFirstTouch = [this] { bSoundUnlocked = true; MenuMusic(); };
 auto Leaving=[this](TFunction<void()> Fn)->TFunction<void()>{return [this,Fn=MoveTemp(Fn)]{RememberTitleErasure();if(Fn)Fn();};};
 O.OnStart=Leaving(MoveTemp(O.OnStart));O.OnContinue=Leaving(MoveTemp(O.OnContinue));
 O.OnFreeform=Leaving(MoveTemp(O.OnFreeform));O.OnLoad=Leaving(MoveTemp(O.OnLoad));
 O.OnSettings=Leaving(MoveTemp(O.OnSettings));O.OnDev=Leaving(MoveTemp(O.OnDev));O.OnQuit=Leaving(MoveTemp(O.OnQuit));
	M->UI->RemoveScreen(TitleEl);
	TitleEl = M->UI->Title(O);
	TitleView(true);
	UE_LOG(LogS47Flow, Display, TEXT("S47: title shown%s, %d saves"), bGate ? TEXT(" (gated)") : TEXT(""), Saves.List().Num());
	// the developer screen straight away, for its screenshot (Scripts/screenshots.sh dev)
	static bool bDevShown = false;
	if (!bDevShown && FParse::Param(FCommandLine::Get(), TEXT("S47DevScreen"))) { bDevShown = true; OpenDev(); }
}

void US47Flow::ShowTitleAgain()
{
 RememberTitleErasure();
	M->UI->RemoveScreen(TitleEl);
	TitleEl.Reset();
	ShowTitle();
}

void US47Flow::RequestNewFreeform()
{
 if (bStarting || M->Mode != EMode::Title) return;
 auto Choose = [this](bool bLethal)
 {
  UE_LOG(LogS47Flow,Display,TEXT("S47: Freeform consequences selected: %s"),bLethal?TEXT("reload"):TEXT("local recovery"));
  M->UI->Close();
  auto Start = [this,bLethal](int32 Id,bool bReplace)
  { FStart S; S.CaseId=Id; S.bReplaceCase=bReplace; S.bFreeform=true; S.bLethal=bLethal; StartGame(S); };
  const int32 Free = Saves.FreeCase();
  if (Free) { Start(Free,false); return; }
  FS47SaveMenuOpts O; O.Mode=TEXT("new"); O.bNewFreeform=true; O.Metas=Saves.List(); O.ActiveCase=Saves.Active();
  O.OnNewCase=[Start](int32 Id) { Start(Id,true); }; S47SaveMenu::Open(M->UI,O);
 };
 // A start choice stays open until an explicit selection or Back. The generic
 // document shell dismisses on backdrop clicks and has no menu keyboard focus.
 M->UI->Open(SNew(SS47FreeformStart).OnChoose(Choose).OnBack([this] { M->UI->Close(); }));
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
	const S47J::FObj State = Saves.Load(Meta.Id);
 if (State)
 {
  const FString Identity=S47J::Str(State,TEXT("sessionMode"),TEXT("story"));
  const auto F=S47J::Obj(State,TEXT("freeform")); const auto V=F?F->TryGetField(TEXT("v")):nullptr;
  if ((Identity!=TEXT("story")&&Identity!=TEXT("freeform")) || (Identity==TEXT("freeform")&&(!V||V->Type!=EJson::Number||V->AsNumber()!=1)))
  { M->UI->Toast(TEXT("This save needs a compatible session version. The current game is still running."),5); return; }
 }
 Opening->Dismiss();
 if (State && M->Mode == EMode::Play) LeavePlay();
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
 RememberTitleErasure();
	if (bStarting) return;
 const FString Identity = S47J::Str(O.State,TEXT("sessionMode"),TEXT("story"));
 const bool bFreeform = O.State ? Identity==TEXT("freeform") : O.bFreeform;
 if (O.State && (Identity!=TEXT("story") && Identity!=TEXT("freeform"))) { M->UI->Toast(TEXT("This session mode is not supported by this version."),5); return; }
 const auto FreeState=S47J::Obj(O.State,TEXT("freeform"));
 const auto FreeVersion=FreeState?FreeState->TryGetField(TEXT("v")):nullptr;
 if (bFreeform && O.State && (!FreeVersion || FreeVersion->Type!=EJson::Number || FreeVersion->AsNumber()!=1))
 { M->UI->Toast(TEXT("This Freeform save version cannot be opened safely."),5); return; }
 if (M->Freeform) M->Freeform->Stop();
 if (M->RoomLights) M->RoomLights->Stop();
 Journal.Load(S47J::Obj(O.State,TEXT("journal")));
 if(M->UI)M->UI->NbTab=TEXT("tasks");
 M->SessionMode = bFreeform ? ES47SessionMode::Freeform : ES47SessionMode::Story;
 // The shared world is built before mode selection. Apply its farm layout before
 // any restored truck/player pose is tested against the area's ground.
 S47FarmWorld::Reconcile(M);
 bDeathShown=false;
	bStarting = true;
	if (M->HighwayUfo) M->HighwayUfo->Stop();
	if (M->Observing) M->Observing->Stop();
	if (M->Spine) M->Spine->Stop();
	if (M->DevEvents) M->DevEvents->Stop();
	if (M->FieldRadio) M->FieldRadio->Stop();
	if (M->Horror) M->Horror->Stop();
	if (M->Evening) M->Evening->Stop();
	const bool bEveningTimeline = !bFreeform && ((!O.State.IsValid() && O.Jump.IsEmpty()) || S47J::Int(O.State,TEXT("timelineVersion"),1)==2);
	const auto EveningSaved = S47J::Obj(O.State,TEXT("evening"));
	const bool bOpening = bEveningTimeline && (!O.State.IsValid() || S47J::Str(O.State,TEXT("checkpoint"))==TEXT("evening"));
	M->Story->bEveningTimeline=bEveningTimeline; M->Story->bOpeningHeld=false;
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
	const FString Area = SavedArea == TEXT("station01") || SavedArea == TEXT("room6") || SavedArea == TEXT("diner") || SavedArea == TEXT("road") ? SavedArea : FString(TEXT("saro"));
	CaseId = O.CaseId;
	M->Playtime = O.Playtime;
	if (CaseId) Saves.SetActive(CaseId);
	if (M->Audio) M->Audio->StartRoomTone();
	// (audio.startNight, space = 'room': the night's sound starts with the HUD)
	// (world.placeTruck and the odometer: S47Away's restore, in main.ts's order)
	if (M->World) M->World->Enter(TEXT("saro")); else if (M->Web) M->Web->Enter(TEXT("saro"));
	Restoring = O.State;
	bRestoringNow = true;
	// what is saved for the systems not ported yet goes back into the next save as it was
	Carry = O.State.IsValid() ? S47J::Clone(O.State) : S47J::FObj();
	// the flashlight, the seen log, the curve, townSeen, glimpses, nightEvents and the album (S47Night)
	if (M->Anomaly) M->Anomaly->Data = FS47AnomalyData();
	if (M->Night) M->Night->Restore(O.State);
	// (Night::Restore puts the rest back in main.ts's order: the town's, the times away, the phones, the thread)
	const FString Checkpoint = O.State.IsValid() ? S47J::Str(O.State, TEXT("checkpoint")) : FString();
	if (bFreeform) M->Story->BeginFreeform(S47J::Obj(O.State,TEXT("routine")));
	else if (bOpening) { M->Story->Reset(); M->Story->bOpeningHeld=true; }
	else StartStory(!Checkpoint.IsEmpty() ? Checkpoint : O.Jump);
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
	// saroDark.restore, follow.restore, arrayBack.restore, reelTape.stop (S47SaroNight)
	if (M->SaroNight) M->SaroNight->Restored();
	// the archive and the field book (S47Phones)
	if (M->Phones) M->Phones->Restored();
	// abduction.refile, lostNights.restore (S47Away)
	if (M->Away) M->Away->Restored();
	// the town's papers, motelTv (S47TownLayer)
	if (M->TownLayer) M->TownLayer->Refile(O.State);
	bRestoringNow = false;
	Restoring.Reset();
	// roadside.reset is in TownLayer::Refile; the CB is its own port.
	M->ResetRadio(M->Story ? M->Story->Clock() : 0);
	if (M->Audio) M->Audio->StopPhoneCalls();
	// world.enter(area); (placeAtStation, placeAtDiner: the world's, not ported yet) (through the world,
	// so its area is the one the chapters ask for: chapter four in room 6)
	if (M->World && Area != TEXT("saro")) M->World->Enter(Area);
	else if (M->Web && Area != TEXT("saro")) M->Web->Enter(Area);
	// if (world.area === 'room6') world.placeAtRoom6()
	if (M->World && M->World->Area == TEXT("room6") && M->Web && M->Player)
	{
		const FS47Pose A = M->Web->Pose(TEXT("world.room6.anchors.arrive"));
		if (A.bValid) M->Player->Place(A.X, A.Z, A.Yaw);
	}
	if (bEveningTimeline && M->Evening) M->Evening->Begin(EveningSaved,!bOpening);
	else if (M->Evening && M->Story->Begun.Contains(TEXT("ch2"))) M->Evening->BeginInvestigation(EveningSaved);
	if (bFreeform) { if(M->Office)M->Office->Begin(S47J::Obj(O.State,TEXT("office")),false); if(M->SkyVisitors)M->SkyVisitors->Begin(nullptr,false); if(M->AnomalyWorld)M->AnomalyWorld->Reset(); }
	if (!bFreeform && M->Office) M->Office->Begin(S47J::Obj(O.State, TEXT("office")), !O.State.IsValid() && O.Jump.IsEmpty());
 if (M->RoomLights) M->RoomLights->Begin(S47J::Obj(O.State,TEXT("roomLights")));
 if (M->EastWing) M->EastWing->Begin(S47J::Obj(O.State,TEXT("eastWingDoors")));
	if (!bFreeform && M->SkyVisitors) M->SkyVisitors->Begin(S47J::Obj(O.State, TEXT("skyVisitors")), !O.State.IsValid() && O.Jump.IsEmpty());
	if(M->FieldRadio) M->FieldRadio->Begin(S47J::Obj(O.State,TEXT("fieldRadio")));
	if (M->Life)
	{
		// Restore playback only after genuine story state; geography is seeded on the first live frame.
		if (O.State.IsValid()) M->Life->PassageLoad(O.State->TryGetField(TEXT("meteorPassage")));
		else M->Life->PassageReset(false);
	}
	if (!bFreeform && M->Substation) M->Substation->Begin(S47J::Obj(O.State, TEXT("substation")));
	if (Checkpoint == TEXT("routine") || Checkpoint == TEXT("nightService")) M->Story->RestoreRoutine(S47J::Obj(O.State,TEXT("routine")));
 // Both restored systems now belong to this case; validate T01 filing against its actual record.
 if (M->Evening) M->Evening->ValidateServiceRestore();
 if (M->Evening) M->Evening->ValidateInvestigationRestore();
	if (!bFreeform && M->Anomaly) M->Anomaly->Begin(S47J::Obj(O.State,TEXT("anomaly")));
	if (!bFreeform && M->AnomalyWorld) M->AnomalyWorld->Begin();
	if (bFreeform && M->Freeform)
 {
  // Legacy Story recovery maps a generic road save back to SARO. Freeform
  // restores the explicitly saved parked service vehicle independently.
  if(M->World)
  {
   FString At=S47J::Str(O.State,TEXT("truck"),TEXT("saro"));if(At!=TEXT("saro")&&At!=TEXT("motel")&&At!=TEXT("station01")&&At!=TEXT("diner"))At=TEXT("saro");
   M->World->PlaceTruck(At,S47J::Obj(O.State,TEXT("truckPose")));
  }
  M->Freeform->Begin(S47J::Obj(O.State,TEXT("freeform")),O.bLethal);
  if (M->Night && M->Night->Flashlight) M->Night->Flashlight->bHave=true;
  // A new operator arrives on foot at the visitor entrance, the station shut and the Jeep in its garage.
  // Tom 5 Nov. Saves and tests written for the old start keep theirs; the normal journey drives from the pad.
  if(!O.State.IsValid()&&!FParse::Param(FCommandLine::Get(),TEXT("S47NormalJourney"))&&(!bDevRun||FParse::Param(FCommandLine::Get(),TEXT("S47GarageStartTest"))||FParse::Param(FCommandLine::Get(),TEXT("S47RescueJourneyTest"))))
  {
   auto Pose=S47J::New();S47J::Set(Pose,TEXT("at"),TEXT("saro"));S47J::Set(Pose,TEXT("x"),35.4);S47J::Set(Pose,TEXT("y"),0.);S47J::Set(Pose,TEXT("z"),-.4);S47J::Set(Pose,TEXT("h"),UE_DOUBLE_PI);
   if(M->World)M->World->PlaceTruck(TEXT("saro"),Pose);
   auto Shut=S47J::New();S47J::Set(Shut,TEXT("personnel"),false);S47J::Set(Shut,TEXT("service"),false);S47J::Set(Shut,TEXT("garage"),0.);S47J::Set(Shut,TEXT("garageUp"),false);
   if(M->EastWing)M->EastWing->Begin(Shut);
   if(M->Web)for(const auto& Door:M->Web->DoorStates())M->Web->SetDoorById(Door.Key,false,true);
   if(M->Player)M->Player->Place(-9.5,5.7,-UE_DOUBLE_PI/2);
  }
 }
 // New Freeform operators collect the camera physically. Every legacy Freeform
 // save had the automatic grant, so absence of this new field preserves that right.
 if (bFreeform && M->FieldCamera)
 {
  M->FieldCamera->Raise(false);
  M->FieldCamera->bHave=O.State.IsValid()?S47J::Bool(O.State,TEXT("freeformCameraOwned"),true):false;
 }
 if (auto* C1=Cast<US47Chapter1>(M->Story->Chapter(TEXT("ch1")))) C1->RestoreEarlyCamera();
 // Before SC01: its normal round counts the evening schedule as part of the work.
 if (!bFreeform && M->Observing) M->Observing->Begin(S47J::Obj(O.State,TEXT("observing")),!O.State.IsValid() && O.Jump.IsEmpty());
 if (!bFreeform && M->Spine) M->Spine->Begin(S47J::Obj(O.State,TEXT("spine")),!O.State.IsValid() && (O.Jump.IsEmpty()),O.State.IsValid() && O.State->HasField(TEXT("spine")));
 if (!bFreeform && M->Horror) M->Horror->Begin(S47J::Obj(O.State,TEXT("horror")),!O.State.IsValid() && O.Jump.IsEmpty());
 if (!bFreeform && M->Spine && !(O.State.IsValid() && O.State->HasField(TEXT("spine")))) M->Spine->ResumeAfterSC03();

 S47FarmWorld::Reconcile(M);
	// put the player back where they stood, if the restored world lets them stand there
	const S47J::FObj Pose = S47J::Obj(O.State, TEXT("pose"));
	if (Pose.IsValid() && Checkpoint != TEXT("residual") && M->Player)
	{
		FVector RestoredPose(S47J::Num(Pose, TEXT("x")), 0, S47J::Num(Pose, TEXT("z")));
		// Migrate changed native interior bounds before rejecting an old valid room pose.
		if (M->Office) M->Office->MigrateRestoredPose(RestoredPose, M->Player->Radius);
		const double X = RestoredPose.X, Z = RestoredPose.Z;
		// Old saves can have a closed door disabling the threshold under their pose.
		// Recover that one doorway before applying the normal saved-pose walkability gate.
		if (!M->Player->Walkable(X, Z) && M->Web) M->Web->RestoreDoorwayAt(FVector(X, 0, Z), M->Player->Radius);
		if (!M->Player->Walkable(X, Z) && M->Office) M->Office->RestoreDoorwayAt(FVector(X, 0, Z), M->Player->Radius);
		if (M->Player->Walkable(X, Z))
		{
			M->Player->Place(X, Z, S47J::Num(Pose, TEXT("yaw")));
			M->Player->Pitch = S47J::Num(Pose, TEXT("pitch"));
		}
	}
	bStarting = false;
	// A chosen departure survives a save before the roof-step wrapper/transition finishes.
	if (O.State.IsValid() && M->Office && (!M->Office->RouteRequired() || M->Office->DepartureReleased()) && M->Story->ChapterKey == TEXT("ch2"))
	{
		if (auto* C2 = Cast<US47Chapter2>(M->Story->Chapter(TEXT("ch2"))); C2 && C2->S.Stage == TEXT("complete") && C2->S.bCalled && C2->S.bP04 && C2->S.bP05)
		{
			TWeakObjectPtr<US47Chapter2> Weak = C2;
			M->Story->After(.3, [Weak] { if (Weak.IsValid() && Weak->bActive && Weak->OnEnd) Weak->OnEnd(); });
		}
	}
	// seenSync(); seenQuiet = false: the night's log listens again
	if (M->Night) M->Night->Restored();
	if (M->HighwayUfo) M->HighwayUfo->Begin(S47J::Obj(O.State,TEXT("highwayUfo")));
	if (!bFreeform && !bEveningTimeline && O.State.IsValid()) M->Story->Note(TEXT("Legacy case timeline retained: original 02:13 references and photographs are unchanged. Any unstored printout acquisition time is marked unavailable."));
	FadeInSoon(0.25f);
	UE_LOG(LogS47Flow, Display, TEXT("S47: night started, case %d, %s"), CaseId, Checkpoint.IsEmpty() ? TEXT("new") : *Checkpoint);
}

void US47Flow::StartStory(const FString& Checkpoint)
{
	// Prologue.start(checkpoint): back to the start of the night, the player at the spawn, then the
	// checkpoint's beat, or the night from 23:41
	if (!M->Story) return;
	if (Checkpoint.IsEmpty() || Checkpoint == TEXT("routine") || Checkpoint == TEXT("nightService")) { M->Story->Start(); return; }
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
	if (M->HighwayUfo) M->HighwayUfo->Stop();
	if (M->Freeform) M->Freeform->Stop();
 if (M->RoomLights) M->RoomLights->Stop();
	M->UI->Fade(true, FString());
	// (fcam.raise(false): the field camera is chapter one's)
	M->UI->Viewfinder(false);
	M->UI->Close(true);
	StopMusic(0.5f);
	if (M->Score) M->Score->Stop();
	if (M->Radio) M->Radio->Reset(M->Story ? M->Story->Clock() : 0);
	if (M->Audio) { M->Audio->StopMotors(); M->Audio->SetCarrier(0, 0, 0); M->Audio->SignalOff(); M->Audio->StopPhoneCalls(); }
	// saroDark.halt, follow, arrayBack, recWalk, reelTape (S47SaroNight)
	if (M->SaroNight) M->SaroNight->Leave();
	// nightEvents, the seen log (quiet) and the curve (S47Night)
	if (M->Night) M->Night->Leave();
	// archive.halt, arrayTip.reset, roofSteps.reset, officePhone.reset (S47Phones)
	if (M->Phones) M->Phones->Leave(false);
	// abduction.halt, truckB.reset, truckTaken.reset, reelTape.stop, the odometer (S47Away)
	if (M->Away) M->Away->Leave(false);
	// motelTv, roadside, townEvents (S47TownLayer)
	if (M->TownLayer) M->TownLayer->Leave(false);
	if (M->Observing) M->Observing->Stop();
	if (M->Spine) M->Spine->Stop();
	if (M->DevEvents) M->DevEvents->Stop();
	if (M->FieldRadio) M->FieldRadio->Stop();
	if (M->Horror) M->Horror->Stop();
	if (M->Evening) M->Evening->Stop();
	if (M->AnomalyWorld) M->AnomalyWorld->Reset();
	if (M->Story) M->Story->Reset();
	if (M->World) M->World->Enter(TEXT("saro")); else if (M->Web) M->Web->Enter(TEXT("saro"));
	bPendingAuto = false;
}

void US47Flow::ToTitle()
{
	if (M->HighwayUfo) M->HighwayUfo->Stop();
	if (M->Freeform) M->Freeform->Stop();
 if (M->RoomLights) M->RoomLights->Stop();
	Opening->Dismiss();
	if (M->Audio) M->Audio->Silence(false);
	M->Mode = EMode::Title;
	M->UI->Viewfinder(false);
	M->UI->Close(true);
	StopMusic(1.5f);
	if (M->Score) M->Score->Stop();
	if (M->Radio) M->Radio->Reset(M->Story ? M->Story->Clock() : 0);
	if (M->Audio) { M->Audio->StopMotors(); M->Audio->SetCarrier(0, 0, 0); M->Audio->SignalOff(); M->Audio->StopPhoneCalls(); }
	// saroDark.halt, follow, arrayBack, recWalk (S47SaroNight)
	if (M->SaroNight) M->SaroNight->Leave(true);
	// the curve (S47Night)
	if (M->Night) M->Night->Leave(true);
	// archive.halt, arrayTip.reset (S47Phones)
	if (M->Phones) M->Phones->Leave(true);
	// abduction.halt, truckTaken.reset (S47Away)
	if (M->Away) M->Away->Leave(true);
	// motelTv, roadside (S47TownLayer)
	if (M->TownLayer) M->TownLayer->Leave(true);
	if (M->Observing) M->Observing->Stop();
	if (M->Spine) M->Spine->Stop();
	if (M->DevEvents) M->DevEvents->Stop();
	if (M->FieldRadio) M->FieldRadio->Stop();
	if (M->Horror) M->Horror->Stop();
	if (M->Evening) M->Evening->Stop();
	if (M->AnomalyWorld) M->AnomalyWorld->Reset();
	if (M->Story) M->Story->Reset();
	if (M->World) M->World->Enter(TEXT("saro")); else if (M->Web) M->Web->Enter(TEXT("saro"));
	bPendingAuto = false;
	M->UI->Fade(false);
	ShowTitle();
}

void US47Flow::Finale()
{
	// main.ts finale(): the 3D game stops on the black. Continue keeps the diner's save.
	US47Chapter6* Ch6 = M->Story ? Cast<US47Chapter6>(M->Story->Chapter(TEXT("ch6"))) : nullptr;
	const TSharedPtr<SS47Ending> End = Ch6 ? Ch6->Ending : nullptr;
	M->Mode = EMode::End;
	if (M->FieldCamera) M->FieldCamera->Raise(false);
	M->UI->Viewfinder(false);
	if (M->Player) M->Player->SetActive(false);
	APlayerController* PC = M->Hud ? M->Hud->GetOwningPlayerController() : nullptr;
	if (PC) PC->FlushPressedKeys();
	M->UI->Close(true);
	M->UI->ClearToasts();
	// Slate's gameplay HUD has separate text fields; the clock and aim already follow Mode.
	M->UI->Objective(FString());
	M->UI->Examine(FString());
	M->UI->ClockSeconds(FString());
	if (PC)
	{
		FInputModeUIOnly Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		if (End.IsValid()) Mode.SetWidgetToFocus(End);
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}
	if (M->World) M->World->StopDriving();
	if (M->Audio) M->Audio->CabRadio(FString());
	if (M->TownLayer && M->TownLayer->Roadside) M->TownLayer->Roadside->Reset();
	if (M->CB) M->CB->Stop();
	if (M->Audio) M->Audio->StopPhoneCalls();
	if (M->Radio) M->Radio->Reset(0);
	if (M->Audio) M->Audio->Silence(false);
	if (M->Score) M->Score->Stop();
	if (!End.IsValid()) { ToTitle(); return; }
	End->Black(true, 0);
	if (M->Audio)
	{
		FS47Sound O; O.DestNode = M->Audio->Music; O.Gain = 0.9f;
		M->Audio->LoopSfx(TEXT("music"), TEXT("titleMusic"), O);
	}
	// Ending.ts owns real-time timers: callbacks replace the three awaited promises.
	TWeakObjectPtr<US47Flow> Self = this;
	TWeakObjectPtr<US47Chapter6> Chapter = Ch6;
	TWeakPtr<SS47Ending> WeakEnd = End;
	UE_LOG(LogS47Flow, Display, TEXT("S47: ch6 ending title"));
	End->Title([Self, Chapter, WeakEnd]
	{
		const TSharedPtr<SS47Ending> Ending = WeakEnd.Pin();
		if (!Self.IsValid() || !Self->M || Self->M->Mode != EMode::End || !Ending.IsValid() || Ending->Disposed()) return;
		UE_LOG(LogS47Flow, Display, TEXT("S47: ch6 ending credits"));
		Ending->Credits([Self, Chapter, WeakEnd]
		{
			const TSharedPtr<SS47Ending> CreditsEnd = WeakEnd.Pin();
			if (!Self.IsValid() || !Self->M || Self->M->Mode != EMode::End || !CreditsEnd.IsValid() || CreditsEnd->Disposed()) return;
			Self->StopMusic(2.5f);
			UE_LOG(LogS47Flow, Display, TEXT("S47: ch6 ending photograph"));
			CreditsEnd->Photo(Chapter.IsValid() ? Chapter->Photo() : nullptr, [Self]
			{
				if (Self.IsValid() && Self->M && Self->M->Audio)
				{
					FS47Sound O; O.Gain = 0.9f; Self->M->Audio->Sfx(TEXT("paper0"), O);
				}
			}, [Self, WeakEnd]
			{
				const TSharedPtr<SS47Ending> PhotoEnd = WeakEnd.Pin();
				if (!Self.IsValid() || !Self->M || Self->M->Mode != EMode::End || !PhotoEnd.IsValid() || PhotoEnd->Disposed()) return;
				Self->CompleteStoryTitle();
				Self->ToTitle();
			});
		});
	});
}

// ---------- pause and the settings ----------

void US47Flow::StopNarration(const void* Except)
{
	// main.ts stopNarration: for (const player of [readAloud, narrator, earlDialogue, npcDialogue]) if (player !== except) player.stop()
	US47UI* UI = M->UI;
	if (UI->ReadAloud.IsValid() && UI->ReadAloud.Get() != Except) UI->ReadAloud->Stop();
	if (UI->Narrator.IsValid() && UI->Narrator.Get() != Except) UI->Narrator->Stop();
	if (UI->EarlDialogue.IsValid() && static_cast<const void*>(UI->EarlDialogue.Get()) != Except) UI->EarlDialogue->Stop();
	if (UI->NpcDialogue.IsValid() && static_cast<const void*>(UI->NpcDialogue.Get()) != Except) UI->NpcDialogue->Stop();
}

void US47Flow::OpenPause()
{
	if (M->bPausedByMenu) return;
	// readAloud, earlDialogue, narrator and npcDialogue stop
	M->UI->StopSpeech();
	M->bPausedByMenu = true;
	if (M->Audio) M->Audio->Suspend(true);
	if (M->Score) M->Score->Pause(true);
	// audio.suspend(true), and the frame does nothing: the world stops, and its sounds with it
	UGameplayStatics::SetGamePaused(M->Hud, true);
	TWeakObjectPtr<US47Flow> Self = this;
	auto Resume = [Self]
	{
		if (!Self.IsValid()) return;
		Self->M->bPausedByMenu = false;
		if (Self->M->Audio) Self->M->Audio->Suspend(false);
		if (Self->M->Score) Self->M->Score->Pause(false);
		UGameplayStatics::SetGamePaused(Self->M->Hud, false);
	};
	FS47PauseOpts O = PauseStatus(Settings());
	O.OnResume = Resume;
	O.OnTitle = [this, Resume] { RequestTitle(Resume); };
	// the case menus sit on top of the pause menu and go back to it
	O.OnSave = [this]
	{
		FS47SaveMenuOpts S;
		S.Mode = TEXT("save"); S.Metas = Saves.List(); S.ActiveCase = CaseId; S.Block = ManualSaveBlock(); S.bFallback = Saves.bFallback;
		// SAVE-SARO-20261108: in Freeform the log is filed at the station; elsewhere the menu says so and offers no save
		S.bFreeform = M->IsFreeform();
		if (S.bFreeform) S.Note = FString::Printf(TEXT("Case %d. The log is filed here at the station, and an autosave is made each time you leave SARO. Manual saves stay until you overwrite them."), CaseId ? CaseId : 1);
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

void US47Flow::RequestTitle(TFunction<void()> Resume)
{
	FString Reason;
	// M7-E: away from SARO nothing is filed on quitting either; the warning says Continue goes back to the log filed on leaving
	const bool bAway = AwayFromSaro(M, CaseId, bDevRun);
	if (bAway) Reason = S47SaroSave::StationOnly();
	else if (AutosaveNow(&Reason)) { if (Resume) Resume(); ToTitle(); return; }
	// Unreal: leaving an unsafe story state or a failed disk write requires an informed choice.
	const TOptional<FS47SaveMeta> Last = CaseId ? Saves.Latest(CaseId) : TOptional<FS47SaveMeta>();
	FString ReturnTo = Last
		? (bAway ? S47SaroSave::QuitAwayText(Last->Chapter, Last->Place, Last->Clock)
			: FString::Printf(TEXT("Continue returns to %s, %s (%s). Progress since that save must be repeated."), *Last->Chapter, *Last->Place, *Last->Clock))
		: FString(TEXT("This night has no saved checkpoint. Leaving now loses this night's progress."));
	if (M->Story && M->Story->ChapterKey == TEXT("ch6")) ReturnTo += TEXT(" The last drive and the ending must be played again from the diner.");
	UE_LOG(LogS47Flow, Display, TEXT("S47: quit warning: %s / %s"), *Reason, *ReturnTo);
	TWeakObjectPtr<US47Flow> Self = this;
	const TSharedRef<bool> Leaving = MakeShared<bool>(false);
	M->UI->Open(SNew(SS47LeaveWarning).UI(M->UI).Reason(Reason).ReturnTo(ReturnTo)
		.OnCancel([Self] { if (Self.IsValid()) Self->M->UI->Close(); })
		.OnLeave([Self, Leaving, Resume]
		{
			if (!Self.IsValid()) return;
			*Leaving = true;
			if (Resume) Resume();
			Self->ToTitle();
		}), [Self, Leaving]
	{
		if (!Self.IsValid() || *Leaving) return;
		Self->M->bPausedByMenu = false;
		Self->OpenPause();
	});
}

FS47PauseOpts US47Flow::PauseStatus(FS47PauseOpts O) const
{
	// the strip's lines: the chapter, the clock and the place, the case and when it was last saved
	AS47Story* S = M->Story;
	O.Chapter = M->IsFreeform() && M->Freeform ? FString::Printf(TEXT("FREEFORM / DAY %d"), M->Freeform->Day()) : S ? S->ChapterTitle() : FString();
	O.Clock = S ? FMath::Fmod(S->Clock()+86400.,86400.) : -1;
	if (M->Evening && M->Evening->Commute()) O.Clock=-1;
	O.Place = PlaceName();
	static const TSet<FString> SaroRooms = { TEXT("Control room"), TEXT("Records room"), TEXT("South corridor"), TEXT("Photo lab"), TEXT("Truck pad"), TEXT("Service yard"), TEXT("Service road") };
	if ((M->Web ? M->Web->Area() : FString(TEXT("saro"))) == TEXT("saro") && SaroRooms.Contains(O.Place)) O.Place += TEXT(", SARO");
	if (bDevRun || !CaseId) O.CaseLine = TEXT("DEVELOPER RUN \u00B7 NOTHING IS SAVED");
	else
	{
		const TOptional<FS47SaveMeta> Last = Saves.Latest(CaseId);
		O.CaseLine = FString::Printf(TEXT("CASE %d \u00B7 %s"), CaseId, Last ? *S47Theme::SavedAgo(Last->SavedAt) : TEXT("NOT SAVED YET"));
	}
	// tonight's card: the chapter's objective, the tasks not done, the latest note
	if (S)
	{
		if (const US47Chapter* C = S->Chapter(S->ChapterKey); C && C->bActive) O.Objective = C->Objective();
		if (M->IsFreeform() && M->Freeform) O.Objective=M->Freeform->Objective();
		for (const FS47Task& T : S->Tasks()) if (!T.bDone) O.OpenTasks.Add(T.Text);
		if (S->Notes().Num()) O.LastNote = S->Notes().Last();
	}
	return O;
}

FS47PauseOpts US47Flow::Settings()
{
	FS47PauseOpts O;
	O.bCharacters = bCharacters;
	O.OnCharacters = [this](bool V) { bCharacters = V; if (M->TownLayer && M->TownLayer->Caliche->NpcDialogue) M->TownLayer->Caliche->NpcDialogue->SetEnabled(V); if (M->Observing) M->Observing->SetCharacterVoicesEnabled(V); if (M->Audio) { M->Audio->bCharacterVoices = V; if (!V) M->Audio->StopPhoneCalls(); } StoreSet(TEXT("characterVoices"), V); };
	O.bSerling = bSerling;
	O.OnSerling = [this](bool V) { bSerling = V; if (M->UI->Narrator.IsValid()) M->UI->Narrator->SetEnabled(V); StoreSet(TEXT("serlingNarrator"), V); };
	O.bKinski = bKinski;
	O.OnKinski = [this](bool V) { bKinski = V; if (M->UI->ReadAloud.IsValid()) M->UI->ReadAloud->SetEnabled(V); if (M->Audio) { M->Audio->bKinskiCalls = V; if (!V) M->Audio->StopPhoneCalls(); } StoreSet(TEXT("kinskiReadAloud"), V); };
	O.bArtBell = bArtBell;
	O.OnArtBell = [this](bool V) { bArtBell = V; if (M->TownLayer && M->TownLayer->Caliche->EarlDialogue) M->TownLayer->Caliche->EarlDialogue->SetEnabled(V); if (M->Radio) M->Radio->SetHostVoice(V); StoreSet(TEXT("artBellRadioHost"), V); };
	O.bRadioCast = bRadioCast;
	O.OnRadioCast = [this](bool V) { bRadioCast = V; if (M->Radio) M->Radio->SetCallerVoice(V); StoreSet(TEXT("radioCallerVoices"), V); };
	O.Volume = Volume; O.Sens = Sens;
	O.OnVolume = [this](float V) { Volume = V; ApplyVolume(); StoreSet(TEXT("vol"), double(V)); };
	O.Music = MusicLevel;
	O.OnMusic = [this](float V) { MusicLevel = V; if (M->Audio) M->Audio->SetMusic(V); StoreSet(TEXT("music"), double(V)); };
	O.Radio = RadioLevel;
	O.OnRadio = [this](float V) { RadioLevel = V; if (M->Audio) M->Audio->SetRadio(V); StoreSet(TEXT("radioVol"), double(V)); };
	O.OnSens = [this](float V) { Sens = V; ApplyLook(); StoreSet(TEXT("sens"), double(V)); };
	O.Graphics = Graphics;
	O.OnGraphics = [this](const FS47GraphicsSettings& G) { ApplyGraphics(G); };
	O.OnReapplyGraphics = [this] { ApplyGraphics(Graphics, false); };
	O.Quality = Quality;
	O.OnQuality = [this](const FString& Q) { ApplyQuality(Q); };
	O.bUltraOk = true; // a computer
	O.Picture = Picture;
	O.OnPicture = [this](const FString& P) { ApplyPicture(P); };
	O.PictureStrength = PictureStrength;
	O.OnPictureStrength = [this](float V) { PictureStrength = S47Picture::Strength(V); ApplyPicture(Picture, false); StoreSet(TEXT("pictureStrength"), double(PictureStrength)); };
	O.PadLookSpeed = PadLookSpeed; O.bPadInvertY = bPadInvertY;
	O.OnPadLookSpeed = [this](float V) { PadLookSpeed = FMath::Clamp(V,.3f,2.5f); ApplyLook(); StoreSet(TEXT("padLookSpeed"), double(PadLookSpeed)); };
	O.OnPadInvertY = [this](bool V) { bPadInvertY = V; ApplyLook(); StoreSet(TEXT("padInvertY"), V); };
	O.bInvertY = bInvertY;
	O.OnInvertY = [this](bool V) { bInvertY = V; ApplyLook(); StoreSet(TEXT("invertY"), V); };
	O.Fov = BaseFov;
	O.OnFov = [this](float V) { BaseFov = V; StoreSet(TEXT("fov"), double(V)); ApplyLook(); };
	O.bLargeText = bLargeText;
	O.OnLargeText = [this](bool V) { bLargeText = V; M->UI->bLargeText = V; StoreSet(TEXT("largeText"), V); };
	O.bCalmFlash = bCalmFlash;
	O.OnCalmFlash = [this](bool V) { bCalmFlash = V; ApplyPicture(Picture, false); StoreSet(TEXT("calmFlash"), V); };
	O.bStillShots = bStillShots;
	O.OnStillShots = [this](bool V) { bStillShots = V; ApplyPicture(Picture, false); StoreSet(TEXT("stillShots"), V); };
	O.bHeadMotion = bHeadMotion;
	O.OnHeadMotion = [this](bool V) { bHeadMotion = V; ApplyLook(); StoreSet(TEXT("headMotion"), V); };
	O.bVehicleMirrors = bVehicleMirrors;
	O.OnVehicleMirrors = [this](bool V) { bVehicleMirrors = V; S47Mirrors::SetEnabled(V); StoreSet(TEXT("vehicleMirrors"), V); };
	O.bAbduct = bAbductOn;
	O.OnAbduct = [this](bool V) { bAbductOn = V; StoreSet(TEXT("abductions"), V); };
	O.Brightness = Brightness;
	O.OnBrightness = [this](float V) { Brightness = V; ApplyBrightness(); StoreSet(TEXT("brightness"), double(V)); };
	O.OnKeys = [this] { StoreSet(TEXT("keys"), MakeShared<FJsonValueObject>(S47Keys::Save())); };
	O.HudColor = S47HudGauges::HudColorId(S47HudGauges::HudColor());
	O.OnHudColor = [this](const FString& Id) { S47HudGauges::SetHudColor(S47HudGauges::HudColorFrom(Id)); StoreSet(TEXT("hudColor"), S47HudGauges::HudColorId(S47HudGauges::HudColor())); };
	return O;
}

void US47Flow::ApplyQuality(const FString& Q)
{
	ApplyGraphics(FS47GraphicsSettings::ForPreset(Q));
}

void US47Flow::ApplyGraphics(const FS47GraphicsSettings& Settings, bool bSave)
{
	Graphics = Settings;
	Graphics.Normalize();
	Quality = Graphics.LegacyQuality();
	if (M && M->Web) M->Web->ApplyGraphicsSettings(Graphics);
	if (bSave && !SmokeTest())
	{
		// One settings write keeps all choices together. Film, input and case saves stay separate.
		Store->SetStringField(TEXT("quality"), Quality);
		StoreSet(TEXT("graphics"), MakeShared<FJsonValueObject>(Graphics.ToJson()));
	}
}

void US47Flow::ApplyPicture(const FString& P, bool bChosen)
{
	Picture = S47Picture::Normalize(P);
	if (bChosen) StoreSet(TEXT("picture"), Picture);
	float Calm = bCalmFlash || bStillShots ? 1.f : 0.f;
	FParse::Value(FCommandLine::Get(), TEXT("S47PictureCalm="), Calm);
	if (M && M->Flow == this) S47Theme::SetTapeComfort(Picture == TEXT("off") ? 0.f : S47Picture::Strength(PictureStrength), Calm >= .5f);
	// Preserve the legacy Vhs blend for scene materials. The tone pass has a separate mode enum.
	UWorld* World = GameWorld();
	UMaterialParameterCollection* Mpc = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Web/Base/MPC_Web.MPC_Web"));
	if (UMaterialParameterCollectionInstance* I = World && Mpc ? World->GetParameterCollectionInstance(Mpc) : nullptr)
	{
		const int32 Mode = S47Picture::Mode(Picture);
		const float Strength = S47Picture::Strength(PictureStrength);
		I->SetScalarParameterValue(TEXT("PictureMode"), float(Mode));
		I->SetScalarParameterValue(TEXT("Vhs"), Mode == 0 ? 0.f : Strength);
		I->SetScalarParameterValue(TEXT("PictureStrength"), Strength);
		I->SetScalarParameterValue(TEXT("PictureCalm"), S47Picture::Strength(Calm));
		float ActualMode = -1.f, ActualVhs = -1.f, ActualStrength = -1.f, ActualCalm = -1.f;
		const bool bMode = I->GetScalarParameterValue(TEXT("PictureMode"), ActualMode);
		const bool bVhs = I->GetScalarParameterValue(TEXT("Vhs"), ActualVhs);
		const bool bStrength = I->GetScalarParameterValue(TEXT("PictureStrength"), ActualStrength);
		const bool bCalm = I->GetScalarParameterValue(TEXT("PictureCalm"), ActualCalm);
		if (bMode && bVhs && bStrength && bCalm)
		{
			UE_LOG(LogS47Flow, Display, TEXT("S47: picture %s MPC mode=%.0f vhs=%.3f strength=%.3f calm=%.3f"), *Picture, ActualMode, ActualVhs, ActualStrength, ActualCalm);
		}
		else
		{
			UE_LOG(LogS47Flow, Warning, TEXT("S47: picture MPC parameter missing (PictureMode=%d Vhs=%d PictureStrength=%d PictureCalm=%d)"), bMode, bVhs, bStrength, bCalm);
		}
	}
}

void US47Flow::ApplyLook()
{
	// input.sensitivity, invertY, and resize(): the lens is the field of view on a computer
	if (!M->Player) return;
	M->Player->Sensitivity = Sens;
	M->Player->bInvertY = bInvertY;
	M->Player->bHeadMotion = bHeadMotion;
	M->Player->PadLookSpeed = PadLookSpeed; M->Player->bPadInvertY = bPadInvertY;
	M->Player->Fov = BaseFov;
}

void US47Flow::ApplyBrightness()
{
	// the light going into M_WebTone's curve: MPC_Web's ToneK (AS47Web: exposure / 0.6, times -S47Scene=)
	// times 2^((b - .5) * 1.5), so .5 is the picture as made and the ends are .59 and 1.68 of it
	UWorld* World = GameWorld();
	UMaterialParameterCollection* Mpc = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Web/Base/MPC_Web.MPC_Web"));
	UMaterialParameterCollectionInstance* I = World && Mpc ? World->GetParameterCollectionInstance(Mpc) : nullptr;
	if (!I || !M->Web) return;
	I->SetScalarParameterValue(TEXT("ToneK"), float(M->Web->ToneMappingExposure / 0.6) * M->Web->SceneScale * FMath::Pow(2.f, (Brightness - 0.5f) * 1.5f));
}

void US47Flow::ApplyVolume()
{
	// audio.setVolume: the master level
	if (M->Audio) M->Audio->SetVolume(Volume);
}

void US47Flow::Activation(bool bActive)
{
	// main.ts: the tab put away (a phone may close it in the background) or the pointer lock lost:
	// save, and pause. (Not in the automated runs, where the window may never have the focus.)
	if (bActive || !M || M->Mode != EMode::Play || SmokeTest()) return;
	// M7-E: away from SARO it only pauses; the routine autosave waits until the player is back inside
	if (!AwayFromSaro(M, CaseId, bDevRun)) AutosaveNow();
	if (!M->bPausedByMenu) OpenPause();
}

void US47Flow::DoorChanged(const FString& Id, bool bOpen)
{
	// Prologue.doorChanged: chapters one, two and four remember the doors
	if (M->Story)
	{
		for (const TCHAR* K : { TEXT("ch1"), TEXT("ch2"), TEXT("ch4") }) if (US47Chapter* C = M->Story->Chapter(K)) C->DoorChanged(Id, bOpen);
	}
	if (!M->IsFreeform() && M->Horror) M->Horror->DoorChanged(Id,bOpen);
	if (M->Mode == EMode::Play && !bRestoringNow) RequestAutosave(false);
}

// ---------- saving ----------

FString US47Flow::FreeformSaveBlock() const
{
 if(bStarting || (M->World && M->World->bBusy))return TEXT("Let the journey finish before saving.");
 if(M->Freeform&&M->Freeform->Taken&&M->Freeform->Taken->Busy())return TEXT("Not now.");
 // SAVE-SARO-20261108: what no save can carry (a visit on the ground, one at a door or a window, the wrong voice): a save in the
 // middle of it would be a way out of it, so the autosaves, the save on quitting and a manual save all wait until it is over.
 if(const FString Out=S47SaroSave::Unsaved(M);!Out.IsEmpty())return Out;
 if(M->World && M->World->bDriving)return TEXT("Park and get out before saving the current supply trip.");
 return FString();
}

FString US47Flow::ManualSaveBlock() const
{
	// Freeform files the log at the station: anywhere else the menu says so (a developer run's, a dead session's own words first).
	// M7-E: the routine autosave, the one on losing the focus and the one on quitting keep the same rule (FlushAutosave, Activation,
	// RequestTitle); only the departure is filed outside (FileDeparture asks SaveBlock).
	return S47SaroSave::StationBlock(CaseInPlay(M, CaseId, bDevRun), S47SaroSave::InStation(M), SaveBlock());
}

// ---------- the departure from SARO (SAVE-SARO-20261108) ----------

bool US47Flow::DepartureLive() const
{
	return M->IsFreeform() && M->Mode == EMode::Play && M->Freeform && M->Freeform->Active() && !bStarting && !bRestoringNow && M->Web && M->Player && M->World;
}

FS47SaroDeparture::FIn US47Flow::DepartureIn(double Dt) const
{
	FS47SaroDeparture::FIn In;
	In.bInside = S47SaroSave::InStation(M);
	In.bDriving = M->World && M->World->bDriving;
	In.bBusy = M->World && M->World->bBusy;
	In.Area = M->Web ? M->Web->Area() : FString();
	if (M->Player) In.At = FVector2D(M->Player->Pos.X, M->Player->Pos.Z);
	// the truck left at the station, and the player beside it: arrived (the farm and the motel share SARO's scene and its place name)
	In.bTruckAtSaro = M->World && M->World->TruckAt == TEXT("saro") && In.Area == TEXT("saro") && S47SaroSave::OnStationGround(In.At.X, In.At.Y);
	In.Dt = Dt;
	return In;
}

void US47Flow::StepDeparture(double Dt)
{
	// a new game, a Continue or the title: nothing armed, nothing waiting
	const void* Session = M->IsFreeform() && M->Freeform ? static_cast<const void*>(M->Freeform.Get()) : nullptr;
	if (!DepartureLive() || Session != DepartureSession) { Departure.Reset(); DepartureSession = Session; DepartureWaitSaid.Empty(); return; }
	if (M->bPausedByMenu) return;
	const FS47SaroDeparture::FIn In = DepartureIn(Dt);
	const FS47SaroDeparture::EHow How = Departure.Look(In);
	if (How != FS47SaroDeparture::EHow::None)
	{
		UE_LOG(LogS47Flow, Display, TEXT("S47: SARO departure, %s, at %s (%s %.1f %.1f)"), FS47SaroDeparture::Name(How), *PlaceName(), *In.Area, In.At.X, In.At.Y);
	}
	else if (Departure.TakeSpaced())
	{
		UE_LOG(LogS47Flow, Display, TEXT("S47: SARO departure within a minute of the last: not filed again"));
		DepartureText = TEXT("not filed: within a minute of the last departure");
	}
	if (!Departure.Pending()) return;
	if (Departure.Expired(In))
	{
		UE_LOG(LogS47Flow, Display, TEXT("S47: SARO departure not filed: no safe moment (%s)"), DepartureWaitSaid.IsEmpty() ? TEXT("gone on") : *DepartureWaitSaid);
		DepartureText = TEXT("not filed: ") + (DepartureWaitSaid.IsEmpty() ? FString(TEXT("the player went on")) : DepartureWaitSaid);
		Departure.Drop(); DepartureWaitSaid.Empty(); bDepartureShow = false;
		NotFiled(M);
		return;
	}
	FileDeparture(false);
}

void US47Flow::DepartingByTruck()
{
	if (!DepartureLive()) return;
	const FS47SaroDeparture::FIn In = DepartureIn(0);
	const FS47SaroDeparture::EHow How = Departure.Drive(In);
	if (How == FS47SaroDeparture::EHow::None) return;
	UE_LOG(LogS47Flow, Display, TEXT("S47: SARO departure, %s, at %s"), FS47SaroDeparture::Name(How), *PlaceName());
	// at the wheel nothing is saved: this is its moment, or none
	FileDeparture(true);
}

void US47Flow::DevDeparture()
{
	Departure.Arm(); bDepartureShow = true;
	DepartureText = TEXT("armed: not left yet");
}

bool US47Flow::FileDeparture(bool bLastChance)
{
	const FString How = FS47SaroDeparture::Name(Departure.PendingHow());
	if (bDevRun || !CaseId)
	{
		// a developer run is never saved: the departure is seen, and its mark shown when F10 asked for it
		UE_LOG(LogS47Flow, Display, TEXT("S47: SARO departure (%s): developer run, nothing is saved"), *How);
		DepartureText = TEXT("seen: developer run, nothing is saved");
		if (bDepartureShow && M->UI) M->UI->Filed(TEXT("LOG FILED  DEV RUN, NOT SAVED"));
		bDepartureShow = false; Departure.Drop(); DepartureWaitSaid.Empty();
		return false;
	}
	// the departure asks SaveBlock, not the station's rule (ManualSaveBlock): leaving is the one save written outside the building
	const FString Danger = S47SaroSave::Danger(M), Block = Danger.IsEmpty() ? SaveBlock() : FString();
	const S47SaroSave::EFile Now = S47SaroSave::FileNow(Danger, Block, bLastChance);
	const FString Why = Danger.IsEmpty() ? Block : Danger;
	if (Now != S47SaroSave::EFile::Write)
	{
		if (Now == S47SaroSave::EFile::NotFiled)
		{
			UE_LOG(LogS47Flow, Display, TEXT("S47: SARO departure (%s) not filed: %s"), *How, *Why);
			DepartureText = TEXT("not filed: ") + Why;
			Departure.Drop(); DepartureWaitSaid.Empty(); bDepartureShow = false;
			NotFiled(M);
			return false;
		}
		if (DepartureWaitSaid != Why)
		{
			UE_LOG(LogS47Flow, Display, TEXT("S47: SARO departure (%s) waits: %s"), *How, *Why);
			DepartureWaitSaid = Why; DepartureText = TEXT("waiting: ") + Why;
		}
		return false;
	}
	// The case's own autosave slots, never a manual one. A slot of its own (a new generation, as a chapter's), and the next routine
	// autosave starts another, so the moment of leaving is kept until the slots have turned round.
	Saves.NewGeneration(CaseId);
	bPendingAuto = false;
	FString Err;
	const TOptional<FS47SaveMeta> Meta = WriteSave(TEXT("auto"), -1, Err);
	DepartureWaitSaid.Empty(); bDepartureShow = false;
	if (!Meta)
	{
		UE_LOG(LogS47Flow, Warning, TEXT("S47: SARO departure (%s): autosave failed: %s"), *How, *Err);
		DepartureText = TEXT("not filed: ") + Err;
		if (M->UI) M->UI->Toast(TEXT("Autosave failed: ") + Err, 5);
		Departure.Drop();
		NotFiled(M);
		return false;
	}
	Saves.NewGeneration(CaseId);
	Departure.Filed();
	UE_LOG(LogS47Flow, Display, TEXT("S47: SARO departure (%s) filed as %s (%s, %s)"), *How, *Meta->Id, *Meta->Place, *Meta->Clock);
	DepartureText = FString::Printf(TEXT("filed as %s (%s, %s)"), *Meta->Id, *Meta->Place, *Meta->Clock);
	if (M->UI) M->UI->Filed(S47HudGauges::FiledLine(Meta->Clock));
	return true;
}

FString US47Flow::SaveBlock() const
{
	if (bDevRun) return TEXT("This is a developer run. Nothing is saved.");
	if (M->Mode != EMode::Play || !CaseId) return TEXT("There is no night running to save.");
	if (M->IsFreeform() && M->Freeform && M->Freeform->Dead()) return TEXT("Load the last save to resume this assignment.");
	if (M->IsFreeform()) return FreeformSaveBlock();
	// (the time away's 47 seconds, the array put back, SARO going dark: not in the Unreal version yet)
	// (nor from the archive's phone ringing until the dishes are down, nor while reel 047 runs)
	if (M->Phones) { const FString B = M->Phones->SaveBlock(); if (!B.IsEmpty()) return B; }
	// (the time away's 47 seconds and the taking, the truck's count on B: S47Away)
	if (M->Away) { const FString B = M->Away->SaveBlock(); if (!B.IsEmpty()) return B; }
	// (the truck's count on B: not in the Unreal version yet)
	// (the time away's 47 seconds, the archive's phone, the truck's count on B: those races are not in
	// the Unreal version yet)
	// (the array on the schedule and going back, and SARO going dark: S47SaroNight)
	if (M->SaroNight) { const FString B = M->SaroNight->SaveBlock(); if (!B.IsEmpty()) return B; }
	return GameSaveBlock();
}

void US47Flow::RequestAutosave(bool bMilestone)
{
	// Progress asks for an autosave; it is written after the next frame, once the game may be saved.
	if (!CaseId || M->Mode != EMode::Play || bRestoringNow) return;
	if (bMilestone) { Saves.NewGeneration(CaseId); bPendingFreshThumb = true; }
	bPendingAuto = true;
}

void US47Flow::FlushAutosave()
{
	// M7-E: in Freeform away from SARO it is not written, and stays asked for until the player is back inside
	if (!bPendingAuto || !ManualSaveBlock().IsEmpty()) return;
	bPendingAuto = false;
	// Rendering a save's picture stops the game for about 60 ms, and Freeform autosaves at least every
	// 30 seconds. Routine autosaves share a recent picture; milestones and every other save take a new one.
	bReuseThumb = !bPendingFreshThumb; bPendingFreshThumb = false;
	FString Err;
	const bool bSaved = WriteSave(TEXT("auto"), -1, Err).IsSet();
	bReuseThumb = false;
	if (!bSaved) M->UI->Toast(TEXT("Autosave failed: ") + Err, 5);
}

bool US47Flow::AutosaveNow(FString* OutError)
{
	// right now, before leaving
	const FString Block = SaveBlock();
	if (!Block.IsEmpty()) { if (OutError) *OutError = Block; return false; }
	bPendingAuto = false;
	FString Err;
	const bool bSaved = WriteSave(TEXT("auto"), -1, Err).IsSet();
	if (OutError) *OutError = Err;
	else if (!bSaved && M->UI) M->UI->Toast(TEXT("Autosave failed: ") + Err, 5);
	return bSaved;
}

S47J::FObj US47Flow::GameState()
{
	const FString Checkpoint = CheckpointName();
	const bool bPrologue = Checkpoint == TEXT("residual");
	// (the fields of the systems not in the Unreal version yet, as the opened save had them)
	const S47J::FObj State = Carry.IsValid() ? S47J::Clone(Carry) : S47J::New();
	S47J::Set(State, TEXT("v"), 3);
	S47J::Set(State,TEXT("sessionMode"),M->IsFreeform()?TEXT("freeform"):TEXT("story"));
	if(M->IsFreeform())S47J::Set(State,TEXT("freeformCameraOwned"),M->FieldCamera&&M->FieldCamera->bHave);
	else State->RemoveField(TEXT("freeformCameraOwned"));
	if (M->HighwayUfo) S47J::Set(State,TEXT("highwayUfo"),M->HighwayUfo->Save());
	if (M->IsFreeform() && M->Freeform) S47J::Set(State,TEXT("freeform"),M->Freeform->Save()); else State->RemoveField(TEXT("freeform"));
	S47J::Set(State,TEXT("journal"),Journal.Save());
	S47J::Set(State,TEXT("timelineVersion"),M->Story && M->Story->bEveningTimeline ? 2 : 1);
	if (M->Evening && M->Evening->Enabled()) S47J::Set(State,TEXT("evening"),M->Evening->Save());
	if (M->Anomaly) S47J::Set(State,TEXT("anomaly"),M->Anomaly->Save());
	if (M->Substation) S47J::Set(State,TEXT("substation"),M->Substation->Save());
	if (M->Office) S47J::Set(State,TEXT("office"),M->Office->Save());
 if (M->RoomLights) S47J::Set(State,TEXT("roomLights"),M->RoomLights->Save());
 if (M->EastWing) S47J::Set(State,TEXT("eastWingDoors"),M->EastWing->Save());
	if (M->SkyVisitors) S47J::Set(State,TEXT("skyVisitors"),M->SkyVisitors->Save());
	if(M->FieldRadio) S47J::Set(State,TEXT("fieldRadio"),M->FieldRadio->Save());
	if (M->Life)
	{
		State->SetField(TEXT("meteorPassage"), M->Life->PassageSave());
	}
	if (M->Observing && M->Observing->Enabled()) S47J::Set(State,TEXT("observing"),M->Observing->Save()); else State->RemoveField(TEXT("observing"));
	if (M->Spine && M->Spine->Enabled()) S47J::Set(State,TEXT("spine"),M->Spine->Save());
 else if (!M->Spine || !M->Spine->PreserveUnknownSave()) State->RemoveField(TEXT("spine"));
 // Otherwise retain Carry's exact unknown JSON value; disabling an unsupported version must not erase it.
	if (M->Horror && M->Horror->Enabled()) S47J::Set(State,TEXT("horror"),M->Horror->Save());
	if (Checkpoint == TEXT("freeform") || Checkpoint == TEXT("routine") || Checkpoint == TEXT("nightService")) S47J::Set(State,TEXT("routine"),M->Story->SaveRoutine()); else State->RemoveField(TEXT("routine"));
	S47J::Set(State, TEXT("town"), M->TownSeen.Array());
	S47J::Set(State, TEXT("checkpoint"), Checkpoint);
	S47J::Set(State, TEXT("case"), SnapshotCase());
	S47J::Set(State, TEXT("area"), M->World ? M->World->Area : M->Web ? M->Web->Area() : FString(TEXT("saro")));
	// (the truck, its pose and the odometer: S47Away's save)
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
	// flashlight, town, seen, curve, album (S47Night)
	if (M->Night) M->Night->Save(State);
	// abduct, odo, the truck and its pose, the pose on your back (S47Away)
	if (M->Away) M->Away->Save(State);
	// AbductSave.dishes while the time away is not ported (S47SaroNight)
	if (M->SaroNight) M->SaroNight->Save(State);
	// townEvents and the rest of the town's (S47TownLayer)
	if (M->TownLayer) M->TownLayer->Save(State);
	return State;
}

TOptional<FS47SaveMeta> US47Flow::WriteSave(const FString& Kind, int32 Slot, FString& OutError)
{
	const S47J::FObj State = GameState();
	const bool bPrologue = S47J::Str(State, TEXT("checkpoint")) == TEXT("residual");
	FS47SaveCard Card;
	Card.Chapter = M->IsFreeform() && M->Freeform ? FString::Printf(TEXT("FREEFORM / DAY %d"),M->Freeform->Day()) : M->Story ? M->Story->ChapterTitle() : FString();
	Card.Place = PlaceName();
	Card.Clock = M->Evening && M->Evening->Commute() ? FString(TEXT("Late afternoon")) : bPrologue ? FString(M->Story && M->Story->bEveningTimeline ? TEXT("00:13") : TEXT("02:13")) : S47Text::ClockText(M->Story ? M->Story->Clock() : 0, false);
	Card.Playtime = M->Playtime;
	// the picture without what stands over you on waking (story/abduction.ts)
	const bool bOver = M->Away && M->Away->HideOver(true);
	const double ThumbNow = FPlatformTime::Seconds();
	if (bReuseThumb && !LastThumb.IsEmpty() && LastThumbCase == CaseId && ThumbNow - LastThumbAt < 120) Card.Thumb = LastThumb;
	else { Card.Thumb = CaptureThumb(); LastThumb = Card.Thumb; LastThumbCase = CaseId; LastThumbAt = ThumbNow; }
	if (bOver) M->Away->HideOver(false);
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
		// as the field camera's: a view state (without one the world's post-process materials, M_WebTone
		// and its exposure, are skipped and the picture comes out dark), one frame with no history,
		// and the eye's near plane
		ThumbCapture->bAlwaysPersistRenderingState = true;
		ThumbCapture->PostProcessBlendWeight = 1.f;
		ThumbCapture->ShowFlags.SetTemporalAA(false);
		ThumbCapture->bOverride_CustomNearClippingPlane = true;
		ThumbCapture->CustomNearClippingPlane = 5.f; // 0.05 m
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
	return PlaceName(M->Player ? M->Player->Pos : FVector::ZeroVector);
}

FString US47Flow::PlaceName(const FVector& P) const
{
	// main.ts placeName: the place on a save's card
	const FString Area = M->Web ? M->Web->Area() : FString(TEXT("saro"));
	if (M->IsFreeform() && (Area == TEXT("saro") || Area == TEXT("road") || Area == TEXT("diner")))
	{
		const FVector Farm = P - S47FarmWorld::RoadToArea(Area, FVector::ZeroVector);
		if (S47FarmWorld::InRoadAccess(Farm.X, Farm.Z)) return TEXT("Alderete farm");
	}
	if (Area == TEXT("saro") && M->Office && M->Office->Contains(P)) return TEXT("SARO Operations office");
	if (M->Substation && M->Substation->Ready() && (Area == TEXT("road") || Area == TEXT("diner")) && P.X >= 7878 && P.X <= 7902 && P.Z >= 519 && P.Z <= 546) return TEXT("Caliche Junction Substation");
	auto In = [&](const TCHAR* Path) { return M->Web && M->Web->Rect(Path).Contains(P.X, P.Z); };
	// (world.indoors, the town and the road's stops are the world's, not ported yet: the inside of the
	// hut by its bounds, the rest by the area's own name)
	if (Area == TEXT("station01")) return In(TEXT("world.site.hutBounds")) ? TEXT("STATION 01, field hut") : TEXT("STATION 01");
	if (Area == TEXT("road")) return TEXT("Highway south");
	if (Area == TEXT("room6")) return TEXT("Sierra Motor Court");
	if (Area == TEXT("diner"))
	{
		if (M->World && M->World->Indoors(P)) return TEXT("Mesa Diner");
		if (M->TownLayer && M->TownLayer->Town->InTown(P.X, P.Z)) return M->TownLayer->Town->PlaceAt(P.X, P.Z);
		for (const auto& Stop : S47RoadTerrain::ROAD_STOPS()) if (P.X - 8000 >= Stop.MinX && P.X - 8000 <= Stop.MaxX && P.Z >= Stop.MinZ && P.Z <= Stop.MaxZ) return Stop.Name;
		return TEXT("Mesa Diner, the lot");
	}
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
	if (M->IsFreeform()) return TEXT("freeform");
	// Where a save resumes. The prologue only has safe points at the start of the night and at
	// 02:13; inside a chapter the chapter's own state is the resume point.
	const AS47Story* S = M->Story;
	if (M->Evening && M->Evening->Opening()) return TEXT("evening");
	if (!S) return TEXT("start");
	if (S->bEveningTimeline && S->Phase() == ES47Phase::Shift) return TEXT("routine");
 if (S->NightSequence() && S->ChapterKey == TEXT("prologue") && S->At(ES47Phase::Residual)) return TEXT("nightService");
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
	if (M->AnomalyWorld && M->AnomalyWorld->Busy()) return TEXT("Let the short studio sound finish before saving.");
	if (M->Evening && M->Evening->Opening()) return M->World && M->World->bBusy ? TEXT("Let the car finish parking first.") : FString();
	if (S->bEveningTimeline && S->Phase() == ES47Phase::Shift && !S->IsCinematic() && (S->BootT < 0 || S->Gt()-S->BootT > 2)) return FString();
	if (!S->At(ES47Phase::Residual)) return S->bEveningTimeline ? TEXT("The survey saves when the routine interval finishes at 00:13. The opening can be saved before the handover.") : TEXT("The night can be saved from 02:13, once the survey has been running for a while.");
 if (S->NightSequence() && S->Phase() == ES47Phase::Residual && !S->bResidualVisible)
  return TEXT("Let the signal transient and emergency switchover finish before saving.");
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
	// ns: Night Shift's papers, relay K3, the console log and the jacket (NightShiftState), the array's
	// tries (story/arrayBack.ts) and Hondo's telex (story/saroDark.ts); the threads (ln) as the opened
	// case had them until they are ported
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
		if (N.LockAt) S47J::Set(Ns, TEXT("lockAt"), *N.LockAt);
		if (N.PrintoutAt) S47J::Set(Ns, TEXT("printoutAt"), *N.PrintoutAt); else S47J::SetNull(Ns, TEXT("printoutAt"));
		S47J::Set(Ns, TEXT("jacket"), N.bJacket);
		// array: copyArraySave(this.ns.array); hondo: this.ns.hondo
		if (const TOptional<FS47ArraySave> A = FS47ArraySave::Copy(N.Array)) Ns->SetObjectField(TEXT("array"), A->ToJson());
		if (N.Hondo) S47J::Set(Ns, TEXT("hondo"), *N.Hondo);
		// (Unreal only, Tom's request: T. Vega's seven keyed on the processor, TAPE A given up; S47DecoderDesk.h)
		if (N.bTapeA) S47J::Set(Ns, TEXT("tapeA"), true);
	}
	S47J::Set(Ns, TEXT("ln"), M->Away ? M->Away->Ln : S47J::Strs(WasNs, TEXT("ln")));
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
	TWeakObjectPtr<US47UI> UI = M->UI;
	TFunction<void()> OnBoard;
	if (M->Board.Open)
	{
		TWeakObjectPtr<US47Main> Main = M;
		OnBoard = [Main] { if (Main.IsValid() && Main->Board.Open) Main->Board.Open(FString(), FString()); };
	}
	// (game.openDoc: T. Vega's field book opens in a view of its own, story/fieldBook.ts)
	TWeakObjectPtr<US47Main> Mn = M;
	TArray<FS47Task> NotebookTasks = S->Tasks();
	if (!M->IsFreeform() && M->Anomaly) NotebookTasks.Append(M->Anomaly->Tasks());
	if (!M->IsFreeform() && M->Substation) NotebookTasks.Append(M->Substation->FollowupTasks());
	M->UI->Notebook(NotebookTasks, S->Notes(), S->Docs(), [UI, Mn](const FS47Doc& Doc)
 {
  if (Mn.IsValid() && !Mn->IsFreeform() && Mn->Anomaly && Mn->Anomaly->OpenDoc(Doc)) return;
  if (Mn.IsValid() && Mn->Phones && Mn->Phones->OpenDoc(Doc)) return;
  if (UI.IsValid() && Mn.IsValid() && Mn->Substation && Doc.Id == TEXT("EV_TRAFO_FOLLOWUP"))
  {
   const FS47Doc Filed = Mn->Substation->FiledFollowupDocument(true);
   UI->Document(Filed.Id.IsEmpty() ? Doc : Filed, [] {}); return;
  }
  if (UI.IsValid()) UI->Document(Doc, [] {});
 },
		M->IsFreeform() && M->Freeform ? FString::Printf(TEXT("SARO / Day %d"), M->Freeform->Day()) : S->ChapterTitle(), OnBoard, NotebookExtras());
}

FS47NbExtras US47Flow::NotebookExtras() const
{
	FS47NbExtras X;
	const bool Free = M->IsFreeform() && M->Freeform; X.bFreeform = Free;
	const FS47MapView V = MapView();
	X.Map = V;
	X.MapDescription = V.bField ? S47FieldMap::Description(V) : S47AreaMap::Description(V);
	if (V.bField)
	{
		TWeakObjectPtr<US47Main> Mw = M;
		X.OnMapNote = [Mw](double Px, double Pz, double Within)
		{
			TArray<FS47MapNote> Notes;
			if (!Mw.IsValid() || !Mw->Freeform || !Mw->Freeform->Places) return Notes;
			Mw->Freeform->Places->ToggleMark(Px, Pz, Within);
			const TArray<FVector2D>& Kept = Mw->Freeform->Places->MapMarks();
			for (int32 I = 0; I < Kept.Num(); ++I) { FS47MapNote Note; Note.X = Kept[I].X; Note.Z = Kept[I].Y; Note.N = I + 1; Notes.Add(Note); }
			return Notes;
		};
	}
	if (!Free && S47AreaMap::LinesMeet(V)) X.MapNote = MEET_NOTE;
	X.Kit = KitItems();
	X.People = Free ? TArray<FS47PersonCard>() : PeopleItems();
	US47Flow* Self = const_cast<US47Flow*>(this);
	X.HintLevel = [Self](const FString& Id) { return Self->JournalHintLevel(Id); };
	X.RevealHint = [Self](const FString& Id) { return Self->RevealJournalHint(Id); };
	X.EntryRead = [Self](const FString& Id) { return Self->JournalEntryRead(Id); };
	X.MarkRead = [Self](const FString& Id) { Self->MarkJournalEntryRead(Id); };
	X.GuideEntries.Add({ TEXT("guide:records"), TEXT("Notebook and retained records"), TEXT("Now lists work already assigned. Knowledge holds retained evidence, notes and known people. Documents keeps the originals and photographs. Kit lists equipment and supplies. The map records known places.\n\nAn observation is not a conclusion. Keep its original record, compare it with a control, and use the case board when the evidence supports a connection. Opening this notebook does not complete work or reveal hints.\n\nQ/E or LB/RB: main tabs. Comma or LT: sections. Arrows or D-pad: entries. Enter or A: the printed action. F1 or RT: reveal the next optional hint on Now.") });
	if (Free)
	{
		const auto F = M->Freeform;
		static const TCHAR* Stages[] = { TEXT("Assigned"), TEXT("Pointed"), TEXT("Recording"), TEXT("Chart waiting"), TEXT("Original collected"), TEXT("Registered"), TEXT("Packet collected"), TEXT("Filed") };
		for (int32 I = 0; I < F->Jobs().Num(); ++I)
		{
			const auto& J = F->Jobs()[I];
			FS47NbWorkItem W;
			W.Id = J.Id + TEXT("/") + FString::FromInt(int32(J.Stage));
			W.Title = J.Id + TEXT(" / ") + FS47Observing::Runs()[FMath::Clamp(J.Source, 0, 3)].Source; W.bCanFollow = true; W.bEarlier = J.Day < F->Day() || J.Stage == FS47FreeformSession::EJobStage::Filed; W.bFollowed = I == F->SelectedJob() && !F->ServiceGuideFocused();
			W.Status = FString(W.bFollowed ? TEXT("Following / ") : TEXT("")) + Stages[FMath::Clamp(int32(J.Stage), 0, 7)];
			W.NextAction = W.bFollowed ? F->NextAction() : F->JobNextAction(J); W.Location = F->JobLocation(J);
			W.bDone = J.Stage == FS47FreeformSession::EJobStage::Filed;
			if (W.bDone) { W.Status = TEXT("Complete / retained"); W.NextAction = TEXT("Result already registered. Select open work at OPS."); }
			if (!W.bDone)
			{
				FString Why;
				switch (J.Stage)
				{
				case FS47FreeformSession::EJobStage::New: Why = TEXT("Aim the antenna array before tuning the receiver. The tracking screen is the right-hand PC."); break;
				case FS47FreeformSession::EJobStage::Pointed: Why = TEXT("Match frequency, gain and bandwidth to this work card. Start when reception reaches 100%."); break;
				case FS47FreeformSession::EJobStage::Integrating: Why = TEXT("The recording keeps valid seconds. Automatic reference scans move the dishes between samples; wait for them to settle."); break;
				case FS47FreeformSession::EJobStage::Chart: Why = TEXT("The measurement is finished. Collect its paper original before registering the result."); break;
				case FS47FreeformSession::EJobStage::Recorded: Why = J.bAnomaly ? TEXT("The original is already retained. Complete the independent comparison requested by this work order.") : TEXT("The original is already retained. A routine result needs no comparison; register it for payment."); break;
				case FS47FreeformSession::EJobStage::Registered: Why = TEXT("Registration is complete. Collect the labeled paper packet before filing it."); break;
				default: Why = TEXT("The packet is already collected. Place it in the records-room binder to finish this order."); break;
				}
				W.Hints = { Why, TEXT("Find this station: ") + W.Location, W.NextAction };
			}
			if (I == F->SelectedJob()) X.WorkItems.Insert(W, 0); else X.WorkItems.Add(W);
		}
		// Keep the followed order first, then unfinished work before completed records.
		if (X.WorkItems.Num() > 1)
		{
			TArray<FS47NbWorkItem> Ordered; Ordered.Add(X.WorkItems[0]);
			for (int32 I = 1; I < X.WorkItems.Num(); ++I) if (!X.WorkItems[I].bDone) Ordered.Add(X.WorkItems[I]);
			for (int32 I = 1; I < X.WorkItems.Num(); ++I) if (X.WorkItems[I].bDone) Ordered.Add(X.WorkItems[I]);
			X.WorkItems = MoveTemp(Ordered);
		}
		for (int32 Line = 0; Line < FS47FreeformCases::CaseCount; ++Line)
		{
			const int32 Test = F->PlannedFieldTest(Line); if (Test < 0 && F->FocusedFieldCase() != Line) continue;
			FS47NbWorkItem W; W.Id = FString::Printf(TEXT("field-plan/%d"), Line);
			W.Title = Test >= 0 ? FString(TEXT("Field test / ")) + FS47FreeformCases::Def(Line).Tests[Test].Name : FString(TEXT("Review / ")) + FS47FreeformCases::Def(Line).Title; W.bCanFollow = true; W.bFollowed = F->FocusedFieldCase() == Line;
			W.Status = W.bFollowed ? TEXT("Following / field plan retained") : TEXT("Field plan retained");
			W.NextAction = F->FieldNextAction(Line); W.Location = TEXT("SARO / field control");
			W.Hints = { TEXT("A field plan remains available while you work on a normal observation."), W.NextAction };
			if (W.bFollowed) X.WorkItems.Insert(W, 0); else X.WorkItems.Add(W);
		}
		if (F->Field && F->Field->ArrayWear().bActive)
			if (const auto* Next = F->Field->ArrayWear().NextBranch())
			{
				const int32 Up = 27 - F->Field->ArrayWear().DownCount();
				const FString Missing = F->Field->BranchNeeds(*Next);
				FS47NbWorkItem W; W.Id = TEXT("array/") + FString(Next->Id); W.Title = FString::Printf(TEXT("Restore the array / %d of 27"), Up); W.Status = TEXT("Station goal");
				W.NextAction = FString::Printf(TEXT("%s: %s %s"), Next->Title, Next->Fault, Missing.IsEmpty() ? TEXT("You have what it needs.") : *(TEXT("Needs ") + Missing + TEXT(".")));
				W.Location = FString::Printf(TEXT("%s junction, beside antenna %s"), Next->Title, Next->Junction);
				X.WorkItems.Add(W);
			}
		if (F->ActiveServiceGuide() > 0)
		{
			FS47NbWorkItem W; W.Id = TEXT("service/") + FString::FromInt(F->ActiveServiceGuide()); W.Title = (F->Field ? F->Field->ServiceDishId() : FString(TEXT("Array"))) + TEXT(" service work order");
			W.bDone = F->ActiveServiceGuide() == 2; W.bFollowed = F->ServiceGuideFocused(); W.Status = W.bDone ? TEXT("Archived") : W.bFollowed ? TEXT("Following / service") : TEXT("Open");
			W.NextAction = F->ServiceObjective(); W.Location = TEXT("Status PC / local service box / ") + (F->Field ? F->Field->ServiceDishId() : FString(TEXT("named service antenna")));
			if (!W.bDone) W.Hints = { TEXT("Start with the current service work order, then inspect the named physical panel."), TEXT("Supply parcels must be delivered at GARAGE RECEIVING before their parts become usable stock."), W.NextAction };
			if (W.bFollowed) X.WorkItems.Insert(W, 0); else X.WorkItems.Add(W);
		}
		if (F->Director) for (const auto& Claim : F->Director->PhotoClaims())
		{
			// Camera guidance is independent of work/service focus and documentation payment.
			if (Claim.ControlPlannedAt >= 0)
			{
				FS47NbWorkItem W; W.Id = TEXT("photo-return/") + Claim.Id; W.Title = TEXT("Return photograph / ") + Claim.Title;
				W.bDone = !Claim.ControlPhotoId.IsEmpty(); W.bEarlier = W.bDone;
				const FS47AlbumShot* Original = M->Night && M->Night->Album ? M->Night->Album->Shots().FindByPredicate([&](const auto& P){return P.Id == Claim.ControlOriginalId;}) : nullptr;
				const bool Available = Original && !Original->bUnavailable && !Original->Url.IsEmpty() && Original->IsDeveloped() && Original->Pose.IsSet() && Original->Pose->Valid();
				const bool Selected = !W.bDone && F->Director->PhotoControlFocusId() == Claim.Id;
				W.bCanFollow = !W.bDone && Available && F->CanRegisterPhotoEvidence();
				W.Status = W.bDone ? TEXT("Comparison retained / no further payment") : !Available ? TEXT("Reference unavailable / plan retained") : Selected ? TEXT("Camera return guide / selected") : TEXT("Camera return plan retained");
				W.NextAction = W.bDone ? TEXT("Both originals and the comparison remain in OPS / SUPPLIES.") : F->Director->PhotoControlHint(Claim.Id);
				W.Location = W.bDone || !Available ? TEXT("OPS / corner PC / SUPPLIES") : TEXT("SARO / original viewpoint; directions from S-03 above");
				if (!W.bDone && Available && M->Night && M->Night->Album)
				{
					bool Exposed=false,Developed=false;
					for (const auto& P : M->Night->Album->Shots()) if (!P.bUnavailable && !P.Url.IsEmpty() && P.Targets.ContainsByPredicate([&](const auto& T){return T.Key == TEXT("freeform-control:") + Claim.Id;})) { Exposed=true; Developed |= P.IsDeveloped(); }
					if (Developed) { W.NextAction=TEXT("OPS / SUPPLIES: compare the two developed originals and retain the comparison; no extra payment."); W.Location=TEXT("OPS / corner PC"); }
					else if (Exposed) { W.NextAction=TEXT("PHOTO LAB: develop, fix and collect the separate return photograph, then compare at OPS."); W.Location=TEXT("PHOTO LAB / wet bench"); }
					else if (Selected) { bool Ready=false; W.NextAction=F->Director->PhotoControlFieldHint(Ready)+TEXT("\n")+W.NextAction; }
				}
				if (!W.bDone && Available && !Selected && !W.bCanFollow) W.NextAction += TEXT("\nSelect this camera return guide at the powered OPS workstation.");
				X.WorkItems.Add(W);
			}
			if (Claim.bPaid) continue;
			FS47NbWorkItem W; W.Id = TEXT("photo/") + Claim.Id; W.Title = TEXT("Photo documentation / ") + Claim.Title;
			const FS47AlbumShot* Photo = M->Night && M->Night->Album ? M->Night->Album->Shots().FindByPredicate([&Claim](const FS47AlbumShot& P) { return P.Id == Claim.PhotoId; }) : nullptr;
			const bool Developed = Photo && Photo->IsDeveloped();
			W.Status = Developed ? TEXT("Developed original / awaiting registration") : TEXT("Exposed original / needs developing");
			W.NextAction = Developed ? FString::Printf(TEXT("OPS: Register the developed original for $%d documentation credit. The image remains in the Album."), Claim.Reward) : TEXT("PHOTO LAB: Develop, fix and collect the original at the wet bench before registering it at OPS.");
			W.Location = Developed ? TEXT("OPS / corner PC") : TEXT("PHOTO LAB / wet bench"); X.WorkItems.Add(W);
		}
		X.OnFollow = [Self](const FString& Id)
		{
			if (!Self->M || !Self->M->Freeform) return;
			const auto F = Self->M->Freeform;
			if (Id.StartsWith(TEXT("photo-return/"))) { if (F->Director) F->Director->FollowPhotoControl(Id.Mid(13)); Self->OpenNotebook(); return; }
			if (Id.StartsWith(TEXT("field-plan/"))) { F->FocusFieldCase(FCString::Atoi(*Id.Mid(11))); Self->OpenNotebook(); return; }
			for (int32 I = 0; I < F->Jobs().Num(); ++I) if (Id.StartsWith(F->Jobs()[I].Id + TEXT("/"))) { F->SelectJob(I); break; }
			Self->OpenNotebook();
		};
		X.GuideEntries.Add({ TEXT("guide:observation"), TEXT("An observing shift"), TEXT("OPS assigns work and registers retained results. The tracking PC points the array; requested coordinates and actual encoders must agree. The receiver tunes frequency, gain and bandwidth and records the signal. Wait for the recording to finish, then collect its original at the printer.\n\nRoutine results return to OPS for payment. If the work order asks for comparison, retain the original and complete the requested control or measurement first. The audio desk compares recorded material; a live signal alone is not a completed review. Register the reviewed packet at OPS, collect it from the printer and file it in the archive binder.") });
		if (HaveCamera && HaveCamera()) X.GuideEntries.Add({ TEXT("guide:photography"), TEXT("Field photography"), FString::Printf(TEXT("%s raises the physical field camera; %s takes a photograph while it is raised. Frame a visible subject clearly and keep the original image in Documents / Photos. The camera remains listed in Kit.\n\nDevelop exposed photographs physically at the PHOTO LAB wet bench: load the tank, transfer to fixer, then collect the print. A qualifying developed incident photograph earns $20 documentation credit when registered at the physical OPS terminal. Now lists pending claims. Each incident pays once; duplicate photographs earn no extra credit. Unclear photographs do not qualify."), *S47Keys::Label(TEXT("camera")), *S47Keys::Label(TEXT("shutter"))) });
		else X.GuideEntries.Add({ TEXT("guide:photography"), TEXT("Collect the field camera"), TEXT("The SARO field camera is on the dry archive bench in the PHOTO LAB, in the service yard east of the control room. Follow the PHOTOGRAPHIC LAB sign, enter the lab and look at the south end of the bench along its east wall. Approach the camera and use Collect field camera. It then appears in Kit.\n\nExpose a photograph with the camera, process it at the wet bench in this same lab, then take a qualifying developed incident photograph to the physical OPS terminal for registration.") });
		X.GuideEntries.Add({ TEXT("guide:transport"), TEXT("Supplies and physical cargo"), TEXT("The service truck starts on its pad outside the SERVICE YARD exit. Check Kit for its current area before setting out. Caliche and its supply counter inside the gas station are south of SARO; the map shows the route and opening hours. Take the truck for the supply trip.\n\nBuy several items at the counter without leaving the shop menu. Paid items are packed together in one parcel beside the counter; collect it when your order is ready. A parcel at the shop, in your hands, in the truck or set down outside is not delivered stock. Carry it to the truck and secure it in the cargo rack. Back at SARO, unload and deliver it at GARAGE RECEIVING. Kit shows the current parcel separately from usable stock. Fuel in the vehicle and generator is separate from spare cans.") });
		X.GuideEntries.Add({ TEXT("guide:service"), TEXT("Power and array service"), TEXT("Read the status and physical service labels before replacing anything. Backup power can keep the station operating while the feeder is repaired. Repairs require the relevant physical panel, parts and tests. The service bay lists retained supplies and workbench progress. If a feeder fault leaves you without a fuse or purchasing power, check the emergency fuse requisition at Caliche's indoor supply counter.") });
	}
	else if (M->Story)
	{
		const auto Tasks = M->Story->Tasks();
		for (int32 I = 0; I < Tasks.Num(); ++I)
		{
			FS47NbWorkItem W; W.Id = FString::Printf(TEXT("story/%d/%08x"), ChapterNo(M->Story), FCrc::StrCrc32(*Tasks[I].Text));
			W.Title = Tasks[I].Text; W.bDone = Tasks[I].bDone; W.Status = W.bDone ? TEXT("Complete") : TEXT("Open");
			W.NextAction = W.bDone ? TEXT("Already completed.") : Tasks[I].Text;
			if (!W.bDone) W.Hints = { TEXT("Read the current task and the most recent retained note together."), TEXT("Documents contains the original records. Knowledge keeps your notes and the people already known to this case."), TEXT("Use only places and records already named in your task. The case board can compare retained evidence.") };
			X.WorkItems.Add(W);
		}
	}
	if (M->Night && M->Night->Album && M->Story) X.Album = M->Night->Album->View(Free ? M->Freeform->Clock() : M->Story->Clock());
	if (X.Album && Free)
	{
		TWeakObjectPtr<US47Flow> Weak = const_cast<US47Flow*>(this);
		// A print a case, a control or a field note points at stays: those records show the image.
		X.Album->CanDiscard = [Weak](const FString& Id)
		{
			if (!Weak.IsValid() || !Weak->M || !Weak->M->Freeform) return false;
			const FS47Album* Album = Weak->M->Night && Weak->M->Night->Album ? Weak->M->Night->Album.Get() : nullptr;
			const FS47AlbumShot* Shot = Album ? Album->Shots().FindByPredicate([&Id](const FS47AlbumShot& S) { return S.Id == Id; }) : nullptr;
			// Targets are what the game saw in the frame: controls and anomalies find their print by them.
			if (!Shot || Shot->bOdd || Shot->Targets.Num() > 0 || (Album->FaceShot() && Album->FaceShot()->Id == Id)) return false;
			const auto& Dir = Weak->M->Freeform->Director;
			if (!Dir) return true;
			if (Dir->HoldsPhoto(Id)) return false;
			if (Weak->M->Freeform->Decor && Weak->M->Freeform->Decor->HoldsPhoto(Id)) return false;
			for (const auto& C : Dir->PhotoClaims()) if (C.PhotoId == Id || C.ControlOriginalId == Id || C.ControlPhotoId == Id || C.LocalNoiseOriginalId == Id) return false;
			for (const auto& O : Dir->Observations()) if (O.PhotoId == Id) return false;
			return true;
		};
		X.Album->OnDiscard = [Weak](const FString& Id)
		{
			if (!Weak.IsValid() || !Weak->M || !Weak->M->Night || !Weak->M->Night->Album) return;
			const bool bGone = Weak->M->Night->Album->Discard(Id);
			UE_LOG(LogS47Flow, Display, TEXT("S47: album discard %s: %s"), *Id, bGone ? TEXT("removed") : TEXT("refused"));
			if (Weak->M->UI) Weak->M->UI->NotebookAt(TEXT("photos"));
			Weak->OpenNotebook();
		};
	}
	return X;
}

FS47MapView US47Flow::MapView() const
{
	const AS47Story* S = M->Story;
	FS47MapView V;
	V.At = MapAt();
	if (M->IsFreeform() && M->Freeform && M->Freeform->Places) M->Freeform->Places->FillMap(V);
	if (M->IsFreeform() && M->Freeform && M->Freeform->Predecessor) M->Freeform->Predecessor->FillMap(V);
	if (M->IsFreeform() && M->Freeform && M->Freeform->Happenings) M->Freeform->Happenings->FillMap(V);
	if (M->IsFreeform() && M->Freeform && M->Freeform->TownUse) M->Freeform->TownUse->FillMap(V); // the saucer map's sites, lures set out
	if (M->IsFreeform() && M->Freeform && M->Freeform->Townsfolk) M->Freeform->Townsfolk->FillMap(V); // where one of the town was last seen
	if (M->IsFreeform() && M->Freeform && M->Freeform->Visitors) M->Freeform->Visitors->FillMap(V); // where Baca says to look
	if (M->IsFreeform())
	{
		// Freeform has no Story chapter clues. It has the survey sheet (MAP-20261108): where the player and the truck are, exactly,
		// on the land's one frame, and the player's own marks.
		V.bStation = true; V.bField = true;
		double X = 0, Z = 0, Heading = 0; FString Area; bool bDriving = false;
		if (S47FieldMap::Me(M, X, Z, Heading, Area, bDriving)) { V.bMe = true; V.MeX = X; V.MeZ = Z; V.MeHeading = Heading; V.MeArea = Area; V.bDriving = bDriving; }
		if (S47FieldMap::Truck(M, X, Z, Heading)) { V.bTruck = true; V.TruckX = X; V.TruckZ = Z; V.TruckHeading = Heading; }
		if (M->Freeform && M->Freeform->Places)
			for (int32 I = 0; I < M->Freeform->Places->MapMarks().Num(); ++I) { const FVector2D& Mk = M->Freeform->Places->MapMarks()[I]; FS47MapNote Note; Note.X = Mk.X; Note.Z = Mk.Y; Note.N = I + 1; V.Notes.Add(Note); }
		S47FieldMap::GatherJobs(M, V);
		return V;
	}
	V.bStation = ChapterNo(S) >= 3;
	V.bOldRoad = ChapterNo(S) >= 5;
	for (const FS47TownMark& Mk : S47TownMarks(M->TownSeen))
	{
		FS47MapMark Mark; Mark.Kind = Mk.Kind; Mark.Clock = Mk.Clock; Mark.Line = S47Lines::LineOfMark(Mk.Kind);
		if (Mk.Kind == TEXT("tv") || Mk.Kind == TEXT("ninth")) { FS47MapAt A; A.Spot = Mk.Kind == TEXT("tv") ? TEXT("office") : TEXT("station01"); Mark.At = A; }
		else if (Mk.Kind == TEXT("cutout") || Mk.Kind == TEXT("car") || Mk.Kind == TEXT("lights") || Mk.Kind == TEXT("cows") || Mk.Kind == TEXT("truck4") || Mk.Kind == TEXT("truck7") || Mk.Kind == TEXT("truck7m")) if (const auto* Pt = S47Lines::PLACES().Find(Mk.Kind)) { FS47MapAt A; A.Spot = TEXT("highway"); A.M = S47Lines::MSouth(Pt->Y); A.bM = true; Mark.At = A; }
		V.Marks.Add(Mark);
	}
	// the times away (S47Away), then all by the clock
	if (M->Away) V.Marks.Append(M->Away->MapMarks());
	V.Marks.Sort([](const FS47MapMark& A, const FS47MapMark& B) { return A.Clock < B.Clock; });
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
	if (Area == TEXT("diner")) { if (M->World && M->World->RoadStopAt(P)) { A.Spot = TEXT("highway"); A.M = P.Z + 300; A.bM = true; } else A.Spot = TEXT("diner"); return A; }
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
	return M ? M->TownSeen.Array() : S47J::Strs(Carry, TEXT("town"));
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
	const auto Truck = M->World ? M->World->TruckSave() : S47J::FObj();
	const FString TruckAt = M->World && M->World->bDriving ? TEXT("Driving") : Truck ? TEXT("Parked / ") + S47J::Str(Truck, TEXT("area")) : TEXT("No retained vehicle position");
	const TArray<FString> Town = TownSeen();
	const bool bCh1 = M->IsFreeform() || ChapterNo(S) >= 1;
	TArray<FS47KitItem> Kit;
	Kit.Add({ TEXT("Notebook"), FString::Printf(TEXT("%s opens it. Now, Knowledge, Documents, Kit, Map and Field Guide."), *S47Keys::Said(TEXT("notebook"))), FString(), TEXT("notebook"), FString() });
	if (bCh1 && !M->IsFreeform()) Kit.Add({ TEXT("SARO jacket"), TEXT("From the hook by the east door. The reflective band across the back."), TEXT("On"), TEXT("jacket"), FString() });
	if (HaveCamera && HaveCamera())
	{
		Kit.Add({ TEXT("Field camera"), FString::Printf(TEXT("The SARO field camera. %s raises it, %s takes the picture: the case frames on sheet film, anything else out of the instant back, into the album."), *S47Keys::Said(TEXT("camera")), *S47Keys::Said(TEXT("shutter"))),
			FString::Printf(TEXT("%d case frames%s"), Frames, RollText ? *(TEXT(", ") + RollText()) : TEXT("")), TEXT("camera"), FString() });
	}
	if (M->Night && M->Night->Flashlight && M->Night->Flashlight->bHave)
	{
		Kit.Add({ TEXT("Flashlight"), FString::Printf(TEXT("From the passenger seat of the truck. %s switches it."), *S47Keys::Said(TEXT("light"))), M->Night->Flashlight->bOn ? TEXT("On") : TEXT("Off"), TEXT("flashlight"), FString() });
	}
	if (bCh1) Kit.Add({ TEXT("Truck keys"), M->IsFreeform() ? TEXT("The service truck starts on the pad outside the SERVICE YARD exit. The keys stay in it. Its current area is recorded below.") : TEXT("The service truck. The keys stay in it."), TruckAt, TEXT("keys"), TEXT("SERVICE") });
	if (Papers) Kit.Add({ TEXT("Papers"), TEXT("Folded into the notebook. Documents keeps the originals; Knowledge links retained evidence."), FString::FromInt(Papers), TEXT("papers"), FString() });
	if (Town.Contains(TEXT("nickel"))) Kit.Add({ TEXT("A nickel"), TEXT("From the coin return of the payphone in Caliche. Bright as the day it was struck."), TEXT("1947"), TEXT("nickel"), FString() });
	// (story/archivePhone.ts: from the fire drawer behind the STATION 01 box)
	if (Town.Contains(TEXT("arc:reel")))
	{
		Kit.Add({ TEXT("Reel 047"), TEXT("A reel of tape in a gray box, from the fire drawer behind the STATION 01 box. NOT FOR CATALOGUE, in pencil. It fits the deck on the signal processor."),
			Town.Contains(TEXT("arc:heard")) ? TEXT("Heard") : TEXT("10-AUG-79"), FString(), FString() });
	}
	// WALKIE-USE-20261108: the set in the player's own hand, and its channel
	if (M->FieldRadio && M->FieldRadio->Owned()) Kit.Add({ TEXT("Field radio"), FString::Printf(TEXT("Portable SARO field set. Channel 1 is Dale's. %s talks on it or answers him, %s turns the channel. A sound alone is not a filed observation."), *S47Keys::Label(TEXT("walkie")), *S47Keys::Label(TEXT("channel"))), FString::Printf(TEXT("Carried / channel %d"), M->FieldRadio->Channel()), FString(), FString() });
	// TAPE-20261108: the field recorder, while it is carried; its cassettes either way, once there are any.
	if (M->IsFreeform() && M->Freeform && M->Freeform->Tape && (M->Freeform->Tape->Carried() || M->Freeform->Tape->Takes().Num()))
	{
		const FS47Tape& Tp = *M->Freeform->Tape;
		Kit.Add({ TEXT("Field recorder"), Tp.Carried()
			? FString::Printf(TEXT("Sony TC-D5 from the desk by the array's PC, a C-60 in it. %s starts and stops it. Play the cassettes back on that desk."), *S47Keys::Said(TEXT("tape")))
			: FString(TEXT("On the desk by the array's PC. Its cassettes play back there.")),
			Tp.Recording() ? FString(TEXT("Recording")) : Tp.Carried() ? FString::Printf(TEXT("Carried / %d %s"), Tp.Takes().Num(), Tp.Takes().Num() == 1 ? TEXT("tape") : TEXT("tapes")) : FString::Printf(TEXT("On the desk / %d %s"), Tp.Takes().Num(), Tp.Takes().Num() == 1 ? TEXT("tape") : TEXT("tapes")),
			FString(), FString() });
	}
	// STONE-POCKET-20261109: the stones in the jacket pocket, said in words, not as a number
	if (M->IsFreeform() && M->Freeform && M->Freeform->Handling && M->Freeform->Handling->PocketCount() > 0)
	{
		Kit.Add({ TEXT("Stones"), FS47Handling::PocketLine(M->Freeform->Handling->PocketCount()) + FString::Printf(TEXT(" Off bare ground, each about the size of a fist. %s with an empty hand brings one out; a tap with a stone in your hand puts it in. Three fit. They knock together when you run."), *S47Keys::Said(TEXT("throw"))),
			TEXT("Jacket pocket"), FString(), FString() });
	}
	if (M->IsFreeform() && M->Freeform && M->Freeform->Predecessor && M->Freeform->Predecessor->Live()) Kit.Add({ TEXT("Station keys"), TEXT("Your set, on a plain ring. The board by the lockers in the east wing takes them, if you are done here."), FString(), TEXT("keys"), TEXT("SARO") });
	if (M->IsFreeform() && M->Freeform && M->Freeform->Field)
	{
		const auto F = M->Freeform->Field;
		Kit.Add({ TEXT("Operating budget"), TEXT("Available funds and outstanding station advance. Register completed work and eligible photographs at OPS; repayments are deducted from earnings."), FString::Printf(TEXT("$%d available / $%d advance"), M->Freeform->Budget(), M->Freeform->Debt()), FString(), FString() });
		for (const auto& Item : TArray<TPair<FString, FString>>{ {TEXT("fuse"), TEXT("Control fuses")}, {TEXT("relay"), TEXT("Compatible relays")}, {TEXT("donor"), TEXT("Donor controllers")}, {TEXT("fuel"), TEXT("Fuel cans")}, {TEXT("film"), TEXT("Film stock")}, {TEXT("recorder_donor"), TEXT("Used field recorders")}, {TEXT("cell"), TEXT("Dry cells")}, {TEXT("foil"), TEXT("Shielding foil")} })
			if (F->InventoryCount(Item.Key) > 0 || (Item.Key != TEXT("recorder_donor") && Item.Key != TEXT("cell") && Item.Key != TEXT("foil")))
			Kit.Add({ Item.Value, TEXT("Usable delivered stock. Parcels still in transit are listed separately."), FString::FromInt(F->InventoryCount(Item.Key)), FString(), FString() });
		if (F->InventoryCount(TEXT("fragment")) > 0) Kit.Add({ TEXT("Field samples"), TEXT("Retained samples for physical workbench comparison."), FString::FromInt(F->InventoryCount(TEXT("fragment"))), FString(), FString() });
		if (F->InventoryCount(TEXT("archived_fragment")) > 0) Kit.Add({ TEXT("Archived samples"), TEXT("Sealed material retained after comparison."), FString::FromInt(F->InventoryCount(TEXT("archived_fragment"))), FString(), FString() });
		Kit.Add({ TEXT("Vehicle fuel"), TEXT("Fuel already in the truck tank; spare cans are separate."), FString::Printf(TEXT("%.1f / 4"), F->VehicleFuel()), FString(), FString() });
		Kit.Add({ TEXT("Backup generator"), TEXT("Fuel already in the generator; spare cans are separate."), FString::Printf(TEXT("%s / %.1f hours"), F->GeneratorRunning() ? TEXT("Running") : TEXT("Stopped"), F->GeneratorHours()), FString(), FString() });
		if (F->ParcelStage() > 0)
		{
			static const TCHAR* Places[] = { TEXT("None"), TEXT("Shop counter"), TEXT("Carried"), TEXT("Loaded in truck"), TEXT("Set down") };
			Kit.Add({ TEXT("Paid parcel"), F->ParcelDescription()+TEXT(". One physical parcel, not yet delivered stock. Unpack it at GARAGE RECEIVING."), Places[FMath::Clamp(F->ParcelStage(), 0, 4)], FString(), FString() });
		}
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
	P.Append(FS47Caliche::TownPeople(M->TownSeen));
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
	if (View == TEXT("totitle")) { if (!AwayFromSaro(M, CaseId, bDevRun)) AutosaveNow(); ToTitle(); return true; }
	// the settings, or the settings at a tab (settings:picture)
	if (View == TEXT("settings") || View.StartsWith(TEXT("settings:")))
	{
		FS47PauseOpts S = Settings(); S.Title = TEXT("Settings"); S.bSettingsOnly = true; S.OnResume = [] {}; S.OnTitle = [] {};
		S.Tab = View.Mid(9);
		M->UI->Pause(S);
		return true;
	}
	if (View == TEXT("save") || View == TEXT("load"))
	{
		FS47SaveMenuOpts S;
		S.Mode = View; S.Metas = Saves.List(); S.ActiveCase = M->Mode == EMode::Title ? Saves.Active() : CaseId; S.Block = ManualSaveBlock(); S.bFallback = Saves.bFallback;
		S.bFreeform = M->IsFreeform() && M->Mode == EMode::Play;
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
	bool bOk = S47Picture::SelfTest(Out);
	bOk &= FS47Rhythm::SelfTest(Out);
	auto Check = [&](const TCHAR* Name, bool bPass, const FString& Got = FString())
	{
		Out.Add(bPass ? FString::Printf(TEXT("ok  flow: %s"), Name) : FString::Printf(TEXT("FAIL flow: %s %s"), Name, *Got));
		bOk &= bPass;
	};
	// SaveMenu.ts played()
	{
		int32 Cancelled = 0, Left = 0;
		const TSharedRef<SS47LeaveWarning> Warning = SNew(SS47LeaveWarning).Reason(TEXT("Not on the road.")).ReturnTo(TEXT("Continue returns to the diner."))
			.OnCancel([&] { ++Cancelled; }).OnLeave([&] { ++Left; });
		auto Key = [&](const FKey& K) { Warning->OnKeyDown(FGeometry(), FKeyEvent(K, FModifierKeysState(), 0, false, 0, 0)); };
		Key(EKeys::Enter);
		Check(TEXT("quit warning defaults to keeping the night open"), Cancelled == 1 && Left == 0);
		Key(EKeys::Down); Key(EKeys::Enter);
		Check(TEXT("quit warning requires choosing the unsaved exit"), Cancelled == 1 && Left == 1);
		Key(EKeys::Gamepad_FaceButton_Right);
		Key(EKeys::Gamepad_DPad_Up); Key(EKeys::Gamepad_FaceButton_Bottom);
		Check(TEXT("quit warning supports Xbox cancellation and confirmation"), Cancelled == 3 && Left == 1);
	}
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
 Check(TEXT("fresh title number is intact before the erasure"),SS47Title::SampleErase47(.99f,false).Opacity==1.f);
 Check(TEXT("calm title effect is a smooth unshifted fade"),FMath::IsNearlyEqual(SS47Title::SampleErase47(1.75f,true).Opacity,.5f) && SS47Title::SampleErase47(1.75f,true).Offset.IsNearlyZero());
 Check(TEXT("both title presentations leave no number at the end"),SS47Title::SampleErase47(2.7f,false).Opacity==0 && SS47Title::SampleErase47(2.7f,true).Opacity==0);
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
	US47Main* Main = NewObject<US47Main>(); US47Flow* Flow = NewObject<US47Flow>(); Flow->M = Main;
	// Test the real apply path without a world or any writes to the user's settings file.
	Flow->Store = S47J::New();
	Check(TEXT("old settings default to head motion enabled"), S47J::Bool(Flow->Store, TEXT("headMotion"), true));
	for (const bool bMotion : { false, true })
	{
		const S47J::FObj MotionSettings = S47J::New();
		S47J::Set(MotionSettings, TEXT("headMotion"), bMotion);
		const S47J::FObj LoadedMotion = S47J::Read(S47J::Write(MotionSettings));
		Flow->bHeadMotion = S47J::Bool(LoadedMotion, TEXT("headMotion"), true);
		const FS47PauseOpts MotionOpts = Flow->Settings();
		Check(bMotion ? TEXT("head motion on survives settings JSON and pause options") : TEXT("head motion off survives settings JSON and pause options"),
			Flow->bHeadMotion == bMotion && MotionOpts.bHeadMotion == bMotion && bool(MotionOpts.OnHeadMotion));
	}
	Check(TEXT("old settings default to vehicle mirrors on"), S47J::Bool(Flow->Store, TEXT("vehicleMirrors"), true));
	for (const bool bMirrors : { false, true })
	{
		const S47J::FObj MirrorSettings = S47J::New();
		S47J::Set(MirrorSettings, TEXT("vehicleMirrors"), bMirrors);
		Flow->bVehicleMirrors = S47J::Bool(S47J::Read(S47J::Write(MirrorSettings)), TEXT("vehicleMirrors"), true);
		const FS47PauseOpts MirrorOpts = Flow->Settings();
		Check(bMirrors ? TEXT("vehicle mirrors on survive settings JSON and pause options") : TEXT("vehicle mirrors off survive settings JSON and pause options"),
			Flow->bVehicleMirrors == bMirrors && MirrorOpts.bVehicleMirrors == bMirrors && bool(MirrorOpts.OnVehicleMirrors));
	}
	S47J::Set(Flow->Store, TEXT("picture"), TEXT("off"));
	const FString SettingsBefore = S47J::Write(Flow->Store);
	Flow->ApplyPicture(TEXT("  CARPENTER  "), false);
	Check(TEXT("runtime override applies normalized look without saving"), Flow->Picture == TEXT("carpenter") && S47J::Write(Flow->Store) == SettingsBefore);
	Flow->PictureStrength = 0.f;
	Flow->ApplyPicture(Flow->Picture, false);
	Check(TEXT("zero strength retains selected look and saved preference"), Flow->Picture == TEXT("carpenter") && Flow->PictureStrength == 0.f && S47J::Write(Flow->Store) == SettingsBefore);
	for (const FString& Id : S47Picture::Ids())
	{
		const auto Settings = S47J::New();
		S47J::Set(Settings, TEXT("picture"), Id); S47J::Set(Settings, TEXT("pictureStrength"), 0.35);
		const auto Loaded = S47J::Read(S47J::Write(Settings));
		Flow->PictureStrength = S47Picture::Strength(float(S47J::Num(Loaded, TEXT("pictureStrength"), 1)));
		Flow->ApplyPicture(S47J::Str(Loaded, TEXT("picture")), false);
		Check(*FString::Printf(TEXT("%s look and strength survive settings JSON"), *Id), Flow->Picture == Id && FMath::IsNearlyEqual(Flow->PictureStrength, 0.35f));
	}
	Flow->Carry = S47J::New(); S47J::Set(Flow->Carry,TEXT("unported"),TEXT("preserved"));
	for (const TCHAR* K : {TEXT("ev:boy"),TEXT("mark:boy@9000"),TEXT("tr:out1"),TEXT("ln:reel")}) Main->TownSeen.Add(K);
	const auto Saved = S47J::Read(S47J::Write(Flow->GameState())); const auto Town = S47J::Strs(Saved,TEXT("town"));
	Check(TEXT("radio and trip findings survive save JSON"), Town.Num() == 4 && Town.Contains(TEXT("ev:boy")) && Town.Contains(TEXT("tr:out1")) && S47J::Str(Saved,TEXT("unported")) == TEXT("preserved"));
	Main->Audio = NewObject<US47Audio>(); Main->Radio = MakeShared<FS47Radio>(Main->Audio); Main->ResetRadio(3 * 3600);
	Check(TEXT("heard boy and old reel prevent a repeat after restore"), Main->Radio->bBoyCalled && !Main->Radio->Receivers.Num());
	Main->TripRadio.Reset(); Main->Radio.Reset();
	return bOk;
}

// ---------- the developer screen ----------

FString US47Flow::DevDir() { return FPaths::ProjectSavedDir() / TEXT("DevStarts"); }

void US47Flow::DevRelaunch(const FString& Args)
{
	// A fresh process with the same flags the contracts use, in the developer case (never saved).
	// Packaged: the binary with the project name, as Signal47.sh starts it; in the editor: -game.
	const FString Exe = FPlatformProcess::ExecutablePath();
	FString Lead = FApp::GetProjectName();
	if (GIsEditor || FPaths::GetCleanFilename(Exe).StartsWith(TEXT("UnrealEditor"))) Lead = FString::Printf(TEXT("\"%s\" -game"), *FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()));
	FString Keep, UserDir;
	if (FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir)) Keep = FString::Printf(TEXT(" -UserDir=\"%s\""), *UserDir);
	const FString Line = FString::Printf(TEXT("%s -S47Case=0 %s%s"), *Lead, *Args, *Keep);
	UE_LOG(LogS47Flow, Display, TEXT("S47: developer start: %s %s"), *Exe, *Line);
	FProcHandle P = FPlatformProcess::CreateProc(*Exe, *Line, true, false, false, nullptr, 0, nullptr, nullptr);
	if (!P.IsValid()) { M->UI->Toast(TEXT("The developer start could not launch a new game process."), 5); return; }
	FPlatformProcess::CloseProc(P);
	UKismetSystemLibrary::QuitGame(M->Hud, M->Hud ? M->Hud->GetOwningPlayerController() : nullptr, EQuitPreference::Quit, false);
}

void US47Flow::DevSave()
{
	if (M->Mode != EMode::Play || bStarting) return;
	const S47J::FObj State = GameState();
	if (!State.IsValid()) { M->UI->Toast(TEXT("No start point: the night has no state to keep yet."), 3); return; }
	const FString Area = M->World ? M->World->Area : FString(TEXT("night"));
	const FString Name = FString::Printf(TEXT("%s %s %s %s"), *FDateTime::Now().ToString(TEXT("%Y-%m-%d %H%M%S")),
		*M->Story->ClockText(false).Replace(TEXT(":"), TEXT("")), *M->Story->ChapterKey, *Area);
	const FString File = DevDir() / (Name.TrimStartAndEnd() + TEXT(".json"));
	if (!FFileHelper::SaveStringToFile(S47J::Write(State, true), *File)) { M->UI->Toast(TEXT("The start point could not be written."), 4); return; }
	UE_LOG(LogS47Flow, Display, TEXT("S47: developer start point saved %s"), *File);
	M->UI->Toast(FString::Printf(TEXT("Developer start point kept: %s, %s. Title > Developer lists it."), *M->Story->ClockText(false), *Area), 4);
}

void US47Flow::OpenDev()
{
	if (bStarting || M->UI->Modal()) return;
	TArray<FS47DevEntry> E;
	const auto Add = [this, &E](const TCHAR* Group, const TCHAR* Label, const TCHAR* Note, const FString& Args)
	{ E.Add({ Group, Label, Note, [this, Args] { DevRelaunch(Args); } }); };
	const FString Night = TEXT("-S47EveningTimeline -S47SpineStart=on -S47Jump=");
	Add(TEXT("Freeform"), TEXT("Test run"), TEXT("a new Freeform game where nothing is saved; F10 starts any event"), TEXT("-S47Freeform"));
	Add(TEXT("Freeform"), TEXT("Test run, the night by its own rules at x15"), TEXT("what a developer run otherwise switches off is on; F10 changes the speed"), TEXT("-S47Freeform -S47NaturalNight=15"));
	Add(TEXT("Evening, 18:00 to 24:00"), TEXT("The drive to SARO"), TEXT("18:00 on the road"), TEXT("-S47Evening=commute"));
	Add(TEXT("Evening, 18:00 to 24:00"), TEXT("Mesa Diner supper"), TEXT("on the way"), TEXT("-S47Evening=diner"));
	Add(TEXT("Evening, 18:00 to 24:00"), TEXT("SARO parking lot"), TEXT("arrived"), TEXT("-S47Evening=parking"));
	Add(TEXT("Evening, 18:00 to 24:00"), TEXT("Dale's handover call"), TEXT("18:00, control room"), TEXT("-S47Evening=phone"));
	Add(TEXT("Night, act one"), TEXT("SC04 Secure the originals"), TEXT("02:55, SARO"), TEXT("-S47EveningTimeline -S47SpineStart=SC04 -S47Jump=residual"));
	Add(TEXT("Night, act one"), TEXT("SC05 The residual on the console"), TEXT("console"), TEXT("-S47EveningTimeline -S47SpineStart=SC05 -S47Jump=residual"));
	Add(TEXT("Night, act one"), TEXT("SC05 The direction solved"), TEXT("printout"), TEXT("-S47EveningTimeline -S47SpineStart=SC05 -S47Jump=printed"));
	Add(TEXT("Night, act one"), TEXT("SC05 The 47 seconds"), TEXT("after the call"), TEXT("-S47EveningTimeline -S47SpineStart=SC05 -S47Jump=countdown"));
	Add(TEXT("Night, acts two to four"), TEXT("SC06 Chapter one"), TEXT("B-12, the second exposure"), Night + TEXT("chapter1"));
	Add(TEXT("Night, acts two to four"), TEXT("SC07 Chapter two, then SC08"), TEXT("records, then the placeholder"), Night + TEXT("chapter2"));
	Add(TEXT("Night, acts two to four"), TEXT("SC08 They point at you"), TEXT("04:00, Earl's reel, the array"), TEXT("-S47EveningTimeline -S47SpineStart=SC08 -S47Jump=chapter2"));
	Add(TEXT("Night, acts two to four"), TEXT("SC09 Chapter three"), TEXT("STATION 01"), Night + TEXT("chapter3"));
	Add(TEXT("Night, acts two to four"), TEXT("SC10 Chapter four, then SC11"), TEXT("Room 6, then the placeholder"), Night + TEXT("chapter4"));
	Add(TEXT("Night, acts two to four"), TEXT("SC11 SARO is searched"), TEXT("05:00, the telex, the siege"), TEXT("-S47EveningTimeline -S47SpineStart=SC11 -S47Jump=chapter4"));
	Add(TEXT("Night, acts two to four"), TEXT("SC12 Chapter five"), TEXT("Mesa Diner"), Night + TEXT("chapter5"));
	Add(TEXT("Night, acts two to four"), TEXT("SC13 Chapter six"), TEXT("the road north"), Night + TEXT("chapter6"));
	Add(TEXT("Night, acts two to four"), TEXT("SC13 The last mile"), TEXT("near C 8.15"), Night + TEXT("chapter6:lastmile"));
	Add(TEXT("Night, acts two to four"), TEXT("The event"), TEXT("05:29"), TEXT("-S47EveningTimeline -S47Jump=event"));
	// F9's own start points, newest first: loaded in this process into the developer case.
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(DevDir() / TEXT("*.json")), true, false);
	Files.Sort([](const FString& A, const FString& B) { return A > B; });
	for (int32 i = 0; i < Files.Num() && i < 24; ++i)
	{
		const FString Path = DevDir() / Files[i];
		E.Add({ TEXT("Your start points (F9)"), FPaths::GetBaseFilename(Files[i]), TEXT("load"), [this, Path]
		{
			FString Text;
			const S47J::FObj State = FFileHelper::LoadFileToString(Text, *Path) ? S47J::Read(Text) : S47J::FObj();
			M->UI->Close(true);
			if (!State.IsValid()) { M->UI->Toast(TEXT("That start point could not be read."), 4); return; }
			M->UI->RemoveScreen(TitleEl); TitleEl.Reset();
			FStart S; S.CaseId = 0; S.bDev = true; S.State = State;
			StartGame(S);
		} });
	}
	if (Files.IsEmpty()) E.Add({ TEXT("Your start points (F9)"), TEXT("None yet"), TEXT("F9 in a night"), [] {} });
	TSharedRef<SS47Dev> El = SNew(SS47Dev).Entries(E).OnBack([this] { M->UI->Close(true); });
	M->UI->Open(El, [this] { if (M->Mode == EMode::Title) ShowTitleAgain(); });
	FSlateApplication::Get().SetKeyboardFocus(El);
	UE_LOG(LogS47Flow, Display, TEXT("S47: developer screen with %d entries, %d start points"), E.Num(), Files.Num());
}

bool US47Flow::ValidJournalHint(const FString& Id)const
{
 if(!M||!M->Story)return false;
 // Story task keys identify the current chapter and visible task, never future facts.
 const auto Tasks=M->Story->Tasks();
 for(int32 I=0;I<Tasks.Num();++I)if(Id==FString::Printf(TEXT("story/%d/%08x"),ChapterNo(M->Story),FCrc::StrCrc32(*Tasks[I].Text)))return true;
 return false;
}
bool US47Flow::ValidJournalEntry(const FString& Id)const
{return FS47Journal::Guide(Id)||(M&&M->Story&&M->Story->Docs().ContainsByPredicate([&](const FS47Doc& D){return D.Id==Id;}));}
int32 US47Flow::JournalHintLevel(const FString& Id)const{return M&&M->IsFreeform()&&M->Freeform?M->Freeform->JournalHintLevel(Id):Journal.Hints.FindRef(Id);}
int32 US47Flow::RevealJournalHint(const FString& Id){if(M&&M->IsFreeform()&&M->Freeform)return M->Freeform->RevealJournalHint(Id);if(!ValidJournalHint(Id))return 0;int32& L=Journal.Hints.FindOrAdd(Id);L=FMath::Min(3,L+1);RequestAutosave(false);return L;}
bool US47Flow::JournalEntryRead(const FString& Id)const{return M&&M->IsFreeform()&&M->Freeform?M->Freeform->JournalEntryRead(Id):Journal.Read.Contains(Id);}
void US47Flow::MarkJournalEntryRead(const FString& Id){if(M&&M->IsFreeform()&&M->Freeform){M->Freeform->MarkJournalEntryRead(Id);return;}if(ValidJournalEntry(Id)&&!Journal.Read.Contains(Id)){Journal.Read.Add(Id);RequestAutosave(false);}}

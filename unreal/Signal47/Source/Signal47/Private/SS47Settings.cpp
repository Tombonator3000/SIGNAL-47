#include "SS47Settings.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "S47Keys.h"
#include "S47Picture.h"
#include "S47Theme.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Settings, Log, All);

namespace S47SettingsLocal
{
	const TCHAR* TabNames[] = { TEXT("GAME"), TEXT("PICTURE"), TEXT("SOUND"), TEXT("VOICES"), TEXT("CONTROLS"), TEXT("ACCESSIBILITY") };
	const float FrameRates[] = { 30.f, 60.f, 90.f, 120.f, 144.f, 0.f };
	const TCHAR* ModeNames[] = { TEXT("FULLSCREEN"), TEXT("BORDERLESS"), TEXT("WINDOWED") };

	FString Percent(float V) { return FString::Printf(TEXT("%d%%"), FMath::RoundToInt32(V * 100.f)); }
	float Snap(float V, float Min, float Max, float Step) { return FMath::Clamp(Min + FMath::RoundToFloat((V - Min) / Step) * Step, Min, Max); }
}

const TArray<FString>& SS47Settings::TabIds()
{
	static const TArray<FString> Ids = { TEXT("game"), TEXT("picture"), TEXT("sound"), TEXT("voices"), TEXT("controls"), TEXT("accessibility") };
	return Ids;
}

int32 SS47Settings::TabIndex(const FString& Id) { return TabIds().IndexOfByKey(Id.ToLower()); }

void SS47Settings::Construct(const FArguments& InArgs)
{
	S47_USING_THEME
	O = InArgs._Opts.IsValid() ? InArgs._Opts : MakeShared<FS47PauseOpts>();
	OnBack = InArgs._OnBack;
	bLarge = O->bLargeText;
	bInv = O->bInvertY;
	ReadDisplay();

	TArray<FString> Names;
	for (const TCHAR* N : S47SettingsLocal::TabNames) Names.Add(N);
	FSlateFontInfo H2 = Heading(54, 2);

	ChildSlot
	[
		SNew(SOverlay)
		// .bg { filter: blur(6px) brightness(.45) } and .tint rgba(4,6,11,.62): what is left of the picture is .17 of it
		+ SOverlay::Slot()[Backdrop(6.f, Css(0x04060b, 0.83f), false)]
		+ SOverlay::Slot().Padding(Pad(90, 64, 90, 84))
		[
			Frame(
				SNew(SOverlay)
				// .head: SETTINGS and the terminal's id
				+ SOverlay::Slot().VAlign(VAlign_Top).Padding(Pad(48, 30, 48, 0))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)[SNew(STextBlock).Text(FText::FromString(TEXT("SETTINGS"))).Font(H2).ColorAndOpacity(Head())]
					+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(0, 0, 0, 13 * U)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("SARO OPS // TERMINAL 3 // SETUP"))).Font(Term(22, 2)).ColorAndOpacity(Sage())
					]
				]
				// .tabs
				+ SOverlay::Slot().VAlign(VAlign_Top).Padding(Pad(48, 118, 48, 0))
				[
					Tabs(Names, TAttribute<int32>::CreateLambda([this] { return Tab; }), [this](int32 T) { if (Listening.IsNone() && ConfirmLeft < 0) SetTab(T); })
				]
				// .rows and .side
				+ SOverlay::Slot().Padding(Pad(48, 186, 48, 76))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(840.f)
					[
						SNew(SBox).MaxDesiredWidth(840 * U)
						[
							SAssignNew(Scroll, SScrollBox).ScrollBarStyle(S47Theme::ScrollStyle()).ScrollBarThickness(FVector2D(3 * U, 3 * U)).ScrollBarPadding(FMargin(0))
							+ SScrollBox::Slot()[SAssignNew(RowBox, SVerticalBox)]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SSpacer).Size(FVector2D(14 * U, 1))]
					+ SHorizontalBox::Slot().FillWidth(470.f)[SNew(SBox).MaxDesiredWidth(470 * U)[Side()]]
				]
				// .foot: the keys, APPLY and REVERT
				+ SOverlay::Slot().VAlign(VAlign_Bottom).Padding(Pad(48, 0, 48, 22))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[Hints()]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).Visibility_Lambda([this] { return HasButtons() ? EVisibility::Visible : EVisibility::Hidden; })
						[
							Button(TEXT("APPLY"), true, [this] { Apply(); }, TAttribute<bool>::CreateLambda([this] { return !(Shown == Applied); }),
								TAttribute<bool>::CreateLambda([this] { return OnButtons() && BtnSel == 0; }))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12 * U, 0, 0, 0)
					[
						SNew(SBox).Visibility_Lambda([this] { return HasButtons() ? EVisibility::Visible : EVisibility::Hidden; })
						[
							Button(TEXT("REVERT"), false, [this] { Revert(); }, TAttribute<bool>::CreateLambda([this] { return !(Shown == Applied); }),
								TAttribute<bool>::CreateLambda([this] { return OnButtons() && BtnSel == 1; }))
						]
					]
				])
		]
		+ SOverlay::Slot()[Scanlines()]
		// keep the new display, or it goes back by itself
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(Box(Night(0.8f))).Padding(0).Visibility_Lambda([this] { return ConfirmLeft >= 0.f ? EVisibility::Visible : EVisibility::Collapsed; })]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).Visibility_Lambda([this] { return ConfirmLeft >= 0.f ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				Frame(
					SNew(SBox).Padding(Pad(36, 28, 36, 26)).WidthOverride(560 * U)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("DISPLAY"))).Font(Term(22, 2)).ColorAndOpacity(Sage())]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 8 * U, 0, 10 * U)[SNew(STextBlock).Text(FText::FromString(TEXT("Keep this display?"))).Font(Heading(32, 1)).ColorAndOpacity(Head())]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Font(Type(17)).ColorAndOpacity(Body()).AutoWrapText(true).LineHeightPercentage(1.3f)
							.Text_Lambda([this] { return FText::FromString(FString::Printf(TEXT("It goes back to the one before in %d seconds."), FMath::CeilToInt32(FMath::Max(0.f, ConfirmLeft)))); })
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 22 * U, 0, 0)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()[Button(TEXT("KEEP"), true, [this] { Keep(); }, true, true)]
							+ SHorizontalBox::Slot().AutoWidth().Padding(12 * U, 0, 0, 0)[Button(TEXT("GO BACK"), false, [this] { Revert(); })]
							+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(SSpacer)]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[KeyHints({ { TEXT("ENTER"), TEXT("KEEP"), nullptr }, { TEXT("ESC"), TEXT("GO BACK"), nullptr } }, 20)]
						]
					])
			]
		]
	];
	const int32 T = TabIndex(InArgs._Tab);
	SetTab(T < 0 ? 0 : T);
}

// ---------- the rows of each tab ----------

void SS47Settings::BuildRows()
{
	S47_USING_THEME
	Rows.Reset();
	FS47PauseOpts* P = O.Get();
	auto Push = [this](FRow R) -> FRow& { return Rows.Add_GetRef(MoveTemp(R)); };
	auto AddSep = [&](const FString& Label = FString()) { FRow R; R.Kind = FRow::EKind::Sep; R.Label = Label; Push(MoveTemp(R)); };
	auto Says = [](const FString& S) { return [S] { return S; }; };
	// an ON / OFF setting
	auto AddToggle = [&](const FString& Label, bool FS47PauseOpts::*Field, TFunction<void(bool)> FS47PauseOpts::*Fn, bool bDefault, const FString& Help)
	{
		FRow R; R.Kind = FRow::EKind::Toggle; R.Label = Label;
		auto Set = [P, Field, Fn](bool V) { if (P->*Field == V) return; P->*Field = V; if (P->*Fn) (P->*Fn)(V); };
		R.On = [P, Field] { return P->*Field; };
		R.Activate = [P, Field, Set] { Set(!(P->*Field)); };
		R.Step = [Set](int32 Dir) { Set(Dir < 0); };
		R.Default = [Set, bDefault] { Set(bDefault); };
		R.Title = Says(Label); R.Text = Says(Help);
		Push(MoveTemp(R));
	};
	// a level on the 20 segments
	auto AddSlider = [&](const FString& Label, float FS47PauseOpts::*Field, TFunction<void(float)> FS47PauseOpts::*Fn, float Min, float Max, float Step, float Def,
		TFunction<FString(float)> Fmt, const FString& Help)
	{
		FRow R; R.Kind = FRow::EKind::Slider; R.Label = Label;
		auto Set = [P, Field, Fn, Min, Max, Step](float V) { V = S47SettingsLocal::Snap(V, Min, Max, Step); if (FMath::IsNearlyEqual(P->*Field, V)) return; P->*Field = V; if (P->*Fn) (P->*Fn)(V); };
		R.Frac = [P, Field, Min, Max] { return (P->*Field - Min) / (Max - Min); };
		R.SetFrac = [Set, Min, Max](float F) { Set(Min + F * (Max - Min)); };
		R.Value = [P, Field, Fmt] { return Fmt(P->*Field); };
		R.Step = [P, Field, Set, Step](int32 Dir) { Set(P->*Field + Dir * Step); };
		R.Default = [Set, Def] { Set(Def); };
		R.Title = Says(Label); R.Text = Says(Help);
		Push(MoveTemp(R));
	};
	// a choice among a few (◀ VALUE ▶)
	auto AddChoice = [&](const FString& Label, TArray<FString> Names, TFunction<int32()> Get, TFunction<void(int32)> Set, int32 Def, TFunction<FString()> Title, TFunction<FString()> Help)
	{
		FRow R; R.Kind = FRow::EKind::Choice; R.Label = Label;
		const int32 N = Names.Num();
		R.Value = [Names, Get] { const int32 I = Get(); return Names.IsValidIndex(I) ? Names[I] : FString(); };
		R.Step = [Get, Set, N](int32 Dir) { Set((Get() + Dir + N) % N); };
		R.Activate = [Get, Set, N] { Set((Get() + 1) % N); };
		R.Default = [Set, Def] { Set(Def); };
		R.Title = Title; R.Text = Help;
		Push(MoveTemp(R));
	};

	switch (Tab)
	{
	case 0: // GAME
	{
		if (P->OnAbduct)
		{
			AddToggle(TEXT("Missing time"), &FS47PauseOpts::bAbduct, &FS47PauseOpts::OnAbduct, true,
				TEXT("Out in the dark at night, now and then, time goes missing: a light, a few single frames, and you come to somewhere else with the minutes gone. Off: the night keeps all of its time."));
		}
		AddChoice(TEXT("Still shots at the end"), { TEXT("MOVING"), TEXT("STILL SHOTS") }, [P] { return P->bStillShots ? 1 : 0; },
			[P](int32 I) { const bool V = I == 1; if (P->bStillShots == V) return; P->bStillShots = V; if (P->OnStillShots) P->OnStillShots(V); }, 0,
			[P] { return FString(P->bStillShots ? TEXT("Still shots") : TEXT("Moving camera")); },
			Says(TEXT("The camera in the last minutes of the night: it moves as the scene does, or it holds still shots one after another, for anyone the movement makes unwell. It also stops film flicker, frame weave and moving tape disturbances.")));
		break;
	}
	case 1: // PICTURE
	{
		auto Display = [&](FRow& R) { R.bDisplay = true; };
		AddChoice(TEXT("Display mode"), { S47SettingsLocal::ModeNames[0], S47SettingsLocal::ModeNames[1], S47SettingsLocal::ModeNames[2] }, [this] { return Shown.Mode; },
			[this](int32 I)
			{
				Shown.Mode = I;
				// borderless covers the screen at its own size
				if (I == 1) if (UGameUserSettings* GS = GEngine ? GEngine->GetGameUserSettings() : nullptr) Shown.Res = GS->GetDesktopResolution();
			}, 0,
			Says(TEXT("Display mode")), Says(TEXT("Fullscreen has the screen to itself. Borderless is a window as large as the screen, quick to leave. Windowed is a window you can move. Choose, then APPLY.")));
		Display(Rows.Last());
		{
			TArray<FString> Names;
			for (const FIntPoint& R : Resolutions) Names.Add(FString::Printf(TEXT("%d × %d"), R.X, R.Y));
			AddChoice(TEXT("Resolution"), Names, [this] { return FMath::Max(0, Resolutions.IndexOfByKey(Shown.Res)); },
				[this](int32 I) { if (Resolutions.IsValidIndex(I)) Shown.Res = Resolutions[I]; }, FMath::Max(0, Resolutions.Num() - 1),
				Says(TEXT("Resolution")), Says(TEXT("The picture's size in pixels, from the sizes this screen offers. Borderless always takes the screen's own. Choose, then APPLY.")));
			Rows.Last().Usable = [this] { return Shown.Mode != 1; };
			Display(Rows.Last());
		}
		AddChoice(TEXT("Frame limit"), { TEXT("30 FPS"), TEXT("60 FPS"), TEXT("90 FPS"), TEXT("120 FPS"), TEXT("144 FPS"), TEXT("UNLIMITED") },
			[this]
			{
				int32 Best = 5;
				for (int32 i = 0; i < 6; ++i) if (FMath::IsNearlyEqual(S47SettingsLocal::FrameRates[i], Shown.Fps, 0.5f)) Best = i;
				return Best;
			},
			[this](int32 I) { Shown.Fps = S47SettingsLocal::FrameRates[I]; }, 5,
			Says(TEXT("Frame limit")), Says(TEXT("The most pictures a second the game will draw. A limit keeps a laptop cooler and quieter. Choose, then APPLY.")));
		Display(Rows.Last());
		{
			FRow R; R.Kind = FRow::EKind::Toggle; R.Label = TEXT("V-sync");
			R.On = [this] { return Shown.bVSync; };
			R.Activate = [this] { Shown.bVSync = !Shown.bVSync; };
			R.Step = [this](int32 Dir) { Shown.bVSync = Dir < 0; };
			R.Default = [this] { Shown.bVSync = false; };
			R.Title = Says(TEXT("V-sync")); R.Text = Says(TEXT("Each picture waits for the screen, so none is torn across the middle. It can add a little delay to the mouse. Choose, then APPLY."));
			R.bDisplay = true;
			Push(MoveTemp(R));
		}
		AddSep();
		{
			// Ultra (shadows, occlusion, bloom) is offered on a computer; phones keep High and Low
			TArray<FString> Ids = P->bUltraOk ? TArray<FString>{ TEXT("low"), TEXT("high"), TEXT("ultra") } : TArray<FString>{ TEXT("low"), TEXT("high") };
			TArray<FString> Names = P->bUltraOk ? TArray<FString>{ TEXT("LOW"), TEXT("HIGH"), TEXT("ULTRA") } : TArray<FString>{ TEXT("LOW"), TEXT("HIGH") };
			AddChoice(TEXT("Quality"), Names, [P, Ids] { return FMath::Max(0, Ids.IndexOfByKey(P->Quality)); },
				[P, Ids](int32 I) { if (!Ids.IsValidIndex(I) || P->Quality == Ids[I]) return; P->Quality = Ids[I]; if (P->OnQuality) P->OnQuality(P->Quality); }, Ids.Num() - 1,
				Says(TEXT("Quality")), Says(TEXT("Low keeps direct lighting and simpler shadows. High adds reflected light and mist. Ultra draws finer shadows and mist.")));
		}
		AddChoice(TEXT("Picture"), S47Picture::Labels(), [P] { return S47Picture::Ids().IndexOfByKey(S47Picture::Normalize(P->Picture)); },
			[P](int32 I)
			{
				if (!S47Picture::Ids().IsValidIndex(I)) return;
				const FString V = S47Picture::Ids()[I];
				if (P->Picture == V) return;
				P->Picture = V;
				if (P->OnPicture) P->OnPicture(V);
			}, 0,
			[P] { return S47Picture::Label(P->Picture); },
			[P] { return S47Picture::Description(P->Picture); });
		AddSlider(TEXT("Picture strength"), &FS47PauseOpts::PictureStrength, &FS47PauseOpts::OnPictureStrength, 0.f, 1.f, 0.05f, 1.f, [](float V) { return S47SettingsLocal::Percent(V); },
			TEXT("How much of the selected film or tape look is blended into the picture. 100% is the full look; 0% gives the base picture and keeps your look selected. Clean stays unchanged."));
		AddSlider(TEXT("Brightness"), &FS47PauseOpts::Brightness, &FS47PauseOpts::OnBrightness, 0.f, 1.f, 0.05f, 0.5f, [](float V) { return S47SettingsLocal::Percent(V); },
			TEXT("How bright the night is, before the picture's curve. At 50% it is as it was made; raise it if the dark hides too much on your screen, but the night should stay dark."));
		AddSlider(TEXT("Field of view"), &FS47PauseOpts::Fov, &FS47PauseOpts::OnFov, 60.f, 90.f, 1.f, 70.f, [](float V) { return FString::Printf(TEXT("%d°"), FMath::RoundToInt32(V)); },
			TEXT("How wide you see, in degrees across the height of the picture. 70 is the game's own."));
		break;
	}
	case 2: // SOUND
		AddSlider(TEXT("Volume"), &FS47PauseOpts::Volume, &FS47PauseOpts::OnVolume, 0.f, 1.f, 0.05f, 0.8f, [](float V) { return S47SettingsLocal::Percent(V); },
			TEXT("Everything you hear. Headphones help: the night has a lot in it that is quiet and far away."));
		AddSlider(TEXT("Music"), &FS47PauseOpts::Music, &FS47PauseOpts::OnMusic, 0.f, 1.f, 0.05f, 0.7f, [](float V) { return S47SettingsLocal::Percent(V); },
			TEXT("The music on the title and the score under the night."));
		AddSlider(TEXT("Radio"), &FS47PauseOpts::Radio, &FS47PauseOpts::OnRadio, 0.f, 1.f, 0.05f, 1.f, [](float V) { return S47SettingsLocal::Percent(V); },
			TEXT("The radios: Night Line on AM 1420, the truck's radio, the diner's set."));
		break;
	case 3: // VOICES (UI.pause's notes)
		AddToggle(TEXT("Character voices"), &FS47PauseOpts::bCharacters, &FS47PauseOpts::OnCharacters, true,
			TEXT("Synthetic parody voices for the attendant, deputy, souvenir man, Nora and the recorded field line."));
		AddToggle(TEXT("Rod Serling narrator"), &FS47PauseOpts::bSerling, &FS47PauseOpts::OnSerling, true,
			TEXT("Synthetic Rod Serling parody for descriptions and chapter transitions. Hold Zoom for a spoken closer look; V repeats a description."));
		AddToggle(TEXT("Kinski read-aloud"), &FS47PauseOpts::bKinski, &FS47PauseOpts::OnKinski, false,
			TEXT("Synthetic Kinski parody for papers, optional conversation reading and other phone voices. Also reads descriptions when the Rod Serling narrator is Off."));
		AddToggle(TEXT("Art Bell radio host"), &FS47PauseOpts::bArtBell, &FS47PauseOpts::OnArtBell, true,
			TEXT("Synthetic Art Bell parody for Earl on Night Line, AM 1420, and when you speak with him in Caliche."));
		AddToggle(TEXT("Radio callers & bulletin"), &FS47PauseOpts::bRadioCast, &FS47PauseOpts::OnRadioCast, true,
			TEXT("Synthetic callers and the 1947 news bulletin on AM 1420."));
		break;
	case 4: // CONTROLS
	{
		AddSlider(TEXT("Mouse sensitivity"), &FS47PauseOpts::Sens, &FS47PauseOpts::OnSens, 0.3f, 2.5f, 0.05f, 1.f, [](float V) { return FString::Printf(TEXT("%.2f"), V); },
			TEXT("How far the view turns for the mouse's movement. 1.00 is the game's own."));
		AddToggle(TEXT("Invert Y"), &FS47PauseOpts::bInvertY, &FS47PauseOpts::OnInvertY, false,
			TEXT("Mouse up looks down, as a pilot's stick does."));
		FString Group;
		for (const S47Keys::FAction& A : S47Keys::Actions())
		{
			if (A.Group != Group) { Group = A.Group; AddSep(Group.ToUpper()); }
			FRow R; R.Kind = FRow::EKind::Keys; R.Label = A.Label; R.Action = A.Id;
			const FName Id = A.Id;
			R.Step = [this, Id](int32 Dir) { const S47Keys::FAction* X = S47Keys::Find(Id); KeySlot = FMath::Clamp(KeySlot + Dir, X && X->bFixedFirst ? 1 : 0, 1); };
			R.Activate = [this, Id] { Listen(Id, KeySlot); };
			R.Default = [this, Id] { S47Keys::Reset(Id); KeysChanged(); Status = TEXT("Back to its own keys."); };
			R.Title = Says(A.Label);
			R.Text = [Help = A.Help] { return Help + TEXT("\n\nEnter, then the new key. Left and right choose the first or the second key; Backspace clears one."); };
			Push(MoveTemp(R));
		}
		AddSep();
		FRow R; R.Kind = FRow::EKind::Action; R.Label = TEXT("Restore every key");
		R.Activate = [this] { S47Keys::ResetAll(); KeysChanged(); Status = TEXT("Every key is back where it was."); };
		R.Default = R.Activate;
		R.Title = Says(TEXT("Restore every key")); R.Text = Says(TEXT("Every action back on the keys the game came with."));
		Push(MoveTemp(R));
		break;
	}
	case 5: // ACCESSIBILITY
		AddChoice(TEXT("Flashing images"), { TEXT("FLASHES"), TEXT("SLOW FADES") }, [P] { return P->bCalmFlash ? 1 : 0; },
			[P](int32 I) { const bool V = I == 1; if (P->bCalmFlash == V) return; P->bCalmFlash = V; if (P->OnCalmFlash) P->OnCalmFlash(V); }, 0,
			[P] { return FString(P->bCalmFlash ? TEXT("Slow fades") : TEXT("Flashes")); },
			Says(TEXT("Near the end of the night there is a short sequence of flashing images. Slow fades shows the same pictures fading in and out instead. It also stops film flicker, frame weave and moving tape disturbances.")));
		AddToggle(TEXT("Large text"), &FS47PauseOpts::bLargeText, &FS47PauseOpts::OnLargeText, false,
			TEXT("Larger words in the notebook and at the end of the night."));
		AddToggle(TEXT("Still shots at the end"), &FS47PauseOpts::bStillShots, &FS47PauseOpts::OnStillShots, false,
			TEXT("The camera in the last minutes holds still shots instead of moving, for anyone the movement makes unwell. It also stops film flicker, frame weave and moving tape disturbances. The same setting as on the Game tab."));
		break;
	default: break;
	}

	// the widgets
	RowBox->ClearChildren();
	RowWidgets.Reset();
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		const FRow& R = Rows[i];
		if (R.Kind == FRow::EKind::Sep)
		{
			TSharedRef<SWidget> W = R.Label.IsEmpty()
				? StaticCastSharedRef<SWidget>(SNew(SBox).Padding(FMargin(18 * U, 10 * U)).HeightOverride(21 * U).VAlign(VAlign_Center)[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Paper(0.08f))]])
				: StaticCastSharedRef<SWidget>(SNew(SBox).Padding(FMargin(21 * U, 18 * U, 18 * U, 6 * U))[SNew(STextBlock).Text(FText::FromString(R.Label)).Font(Term(20, 2)).ColorAndOpacity(Sage())]);
			RowWidgets.Add(W);
			RowBox->AddSlot().AutoHeight()[W];
			continue;
		}
		auto IsSel = [this, i] { return Sel == i; };
		const TFunction<bool()> Usable = R.Usable;
		TAttribute<bool> Can = TAttribute<bool>::CreateLambda([Usable] { return !Usable || Usable(); });
		TSharedRef<SWidget> Value = SNullWidget::NullWidget;
		switch (R.Kind)
		{
		case FRow::EKind::Choice: Value = S47Theme::Choice(TAttribute<FString>::CreateLambda(R.Value), TAttribute<bool>::CreateLambda(IsSel), [this, i](int32 Dir) { Sel = i; if (Rows.IsValidIndex(i) && Rows[i].Step) Rows[i].Step(Dir); }, Can); break;
		case FRow::EKind::Slider: Value = Segments(TAttribute<float>::CreateLambda(R.Frac), TAttribute<FString>::CreateLambda(R.Value), [this, i](float F) { Sel = i; if (Rows.IsValidIndex(i) && Rows[i].SetFrac) Rows[i].SetFrac(F); }, Can); break;
		case FRow::EKind::Toggle: Value = S47Theme::Toggle(TAttribute<bool>::CreateLambda(R.On), [this, i](bool V) { Sel = i; if (Rows.IsValidIndex(i) && Rows[i].On && Rows[i].On() != V && Rows[i].Activate) Rows[i].Activate(); }, Can); break;
		case FRow::EKind::Keys: Value = KeyCaps(R.Action, i); break;
		case FRow::EKind::Action: Value = Keycap(TEXT("ENTER"), 22); break;
		default: break;
		}
		TSharedRef<SWidget> W = SNew(SS47Row).Label(R.Label).Height(R.Kind == FRow::EKind::Keys ? 46.f : 58.f)
			.Selected_Lambda(IsSel).Usable(Can)
			.OnHover([this, i] { if (Listening.IsNone() && ConfirmLeft < 0) Sel = i; })
			.OnClick([this, i] { if (!Listening.IsNone() || ConfirmLeft >= 0) return; Sel = i; if (Rows.IsValidIndex(i) && Rows[i].Kind == FRow::EKind::Action && Rows[i].Activate) Rows[i].Activate(); })
			[Value];
		RowWidgets.Add(W);
		RowBox->AddSlot().AutoHeight()[W];
	}
}

TSharedRef<SWidget> SS47Settings::KeyCaps(FName Action, int32 Index)
{
	S47_USING_THEME
	// the action's two keys; the one the arrows have chosen framed in amber, PRESS A KEY while it is being taken
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (int32 S = 0; S < 2; ++S)
	{
		auto Lit = [this, Index, S] { return Sel == Index && KeySlot == S; };
		auto Taking = [this, Action, S] { return Listening == Action && KeySlot == S; };
		const S47Keys::FAction* A = S47Keys::Find(Action);
		const bool bFixed = A && A->bFixedFirst && S == 0;
		TSharedRef<SWidget> Cap = SNew(SBorder).Padding(FMargin(8 * U, 0)).VAlign(VAlign_Center).HAlign(HAlign_Center)
			.BorderImage_Lambda([Lit, Taking, bFixed]
			{
				if (Taking()) return Box(Amber(0.14f), Amber(), 1.f, 2 * U);
				if (Lit()) return Box(FLinearColor::Transparent, Amber(), 1.f, 2 * U);
				return Box(FLinearColor::Transparent, bFixed ? Css(0x3a3730) : Rule(), 1.f, 2 * U);
			})
			.OnMouseButtonDown_Lambda([this, Index, S, bFixed](const FGeometry&, const FPointerEvent& E)
			{
				if (E.GetEffectingButton() != EKeys::LeftMouseButton || !Listening.IsNone() || ConfirmLeft >= 0) return FReply::Unhandled();
				Sel = Index;
				KeySlot = S;
				if (!bFixed && Rows.IsValidIndex(Index)) Listen(Rows[Index].Action, S);
				return FReply::Handled();
			})
			[
				SNew(SBox).MinDesiredWidth(96 * U).HeightOverride(30 * U).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(Term(22, 1))
					.Text_Lambda([this, Action, S, Taking]
					{
						if (Taking()) return FText::FromString(FMath::Fmod(Blink, 1.f) < 0.6f ? TEXT("PRESS A KEY") : TEXT(""));
						const FKey K = S47Keys::Key(Action, S);
						return FText::FromString(K.IsValid() ? S47Keys::Short(K) : TEXT("·"));
					})
					.ColorAndOpacity_Lambda([Taking, Lit, bFixed] { return FSlateColor(Taking() || Lit() ? Amber() : bFixed ? Mute() : Paper()); })
				]
			];
		Row->AddSlot().AutoWidth().Padding(S ? 10 * U : 0.f, 0, 0, 0)[Cap];
	}
	return Row;
}

TSharedRef<SWidget> SS47Settings::Side()
{
	S47_USING_THEME
	// .side: the tab's name, the selected row's title and what it does
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10 * U)
		[
			SNew(STextBlock).Font(Term(22, 2)).ColorAndOpacity(Sage()).Text_Lambda([this] { return FText::FromString(S47SettingsLocal::TabNames[FMath::Clamp(Tab, 0, 5)]); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12 * U)
		[
			SNew(STextBlock).Font(Heading(32, 1)).ColorAndOpacity(Head()).AutoWrapText(true)
			.Text_Lambda([this]
			{
				if (OnButtons()) return FText::FromString(TEXT("The display"));
				const FRow* R = Current();
				return FText::FromString(R && R->Title ? R->Title() : FString());
			})
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Font(Type(17)).ColorAndOpacity(Body()).AutoWrapText(true).LineHeightPercentage(1.6f)
			.Text_Lambda([this]
			{
				if (OnButtons()) return FText::FromString(TEXT("APPLY puts the display mode, resolution, frame limit and v-sync into force; a new mode or size waits ten seconds for you to keep it. REVERT puts back what is in force now."));
				const FRow* R = Current();
				return FText::FromString(R && R->Text ? R->Text() : FString());
			})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 18 * U, 0, 0)
		[
			SNew(STextBlock).Font(Term(22, 1)).ColorAndOpacity(Phos(0.9f)).AutoWrapText(true)
			.Text_Lambda([this] { return FText::FromString(Status); })
			.Visibility_Lambda([this] { return Status.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 18 * U, 0, 0)
		[
			SNew(STextBlock).Font(Term(22, 1)).ColorAndOpacity(Amber(0.9f)).AutoWrapText(true)
			.Text(FText::FromString(TEXT("Not applied yet: APPLY at the foot puts it into force.")))
			.Visibility_Lambda([this] { const FRow* R = Current(); return R && R->bDisplay && !(Shown == Applied) ? EVisibility::Visible : EVisibility::Collapsed; })
		];
}

TSharedRef<SWidget> SS47Settings::Hints()
{
	S47_USING_THEME
	auto Next = [this](int32 Dir) { if (Listening.IsNone() && ConfirmLeft < 0) SetTab((Tab + Dir + 6) % 6); };
	auto Default = [this] { if (const FRow* R = Current(); R && R->Default && Listening.IsNone()) R->Default(); };
	TSharedRef<SWidget> Normal = KeyHints({
		{ TEXT("UP DOWN"), TEXT("SELECT"), nullptr }, { TEXT("LEFT RIGHT"), TEXT("CHANGE"), nullptr }, { TEXT("Q E"), TEXT("TAB"), [Next] { Next(1); } },
		{ TEXT("R"), TEXT("DEFAULT"), Default }, { TEXT("ESC"), TEXT("BACK"), [this] { Back(); } } });
	TSharedRef<SWidget> Keys = KeyHints({
		{ TEXT("UP DOWN"), TEXT("SELECT"), nullptr }, { TEXT("LEFT RIGHT"), TEXT("KEY"), nullptr }, { TEXT("ENTER"), TEXT("BIND"), [this] { if (const FRow* R = Current()) Listen(R->Action, KeySlot); } },
		{ TEXT("BKSP"), TEXT("CLEAR"), [this] { if (const FRow* R = Current()) { S47Keys::Clear(R->Action, KeySlot); KeysChanged(); } } }, { TEXT("R"), TEXT("DEFAULT"), Default },
		{ TEXT("ESC"), TEXT("BACK"), [this] { Back(); } } });
	TSharedRef<SWidget> Taking = KeyHints({ { TEXT("ANY KEY"), TEXT("BIND"), nullptr }, { TEXT("ESC"), TEXT("CANCEL"), [this] { Listening = NAME_None; Status.Reset(); } } });
	auto Mode = [this] { const FRow* R = Current(); return !Listening.IsNone() ? 2 : R && R->Kind == FRow::EKind::Keys ? 1 : 0; };
	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBox).Visibility_Lambda([Mode] { return Mode() == 0 ? EVisibility::Visible : EVisibility::Collapsed; })[Normal]]
		+ SOverlay::Slot()[SNew(SBox).Visibility_Lambda([Mode] { return Mode() == 1 ? EVisibility::Visible : EVisibility::Collapsed; })[Keys]]
		+ SOverlay::Slot()[SNew(SBox).Visibility_Lambda([Mode] { return Mode() == 2 ? EVisibility::Visible : EVisibility::Collapsed; })[Taking]];
}

// ---------- moving about ----------

void SS47Settings::SetTab(int32 T)
{
	Tab = FMath::Clamp(T, 0, 5);
	Status.Reset();
	Listening = NAME_None;
	KeySlot = 0;
	BuildRows();
	Sel = Rows.IndexOfByPredicate([](const FRow& R) { return R.Kind != FRow::EKind::Sep; });
	if (Sel < 0) Sel = 0;
	if (Scroll.IsValid()) Scroll->ScrollToStart();
}

bool SS47Settings::HasButtons() const
{
	return Rows.ContainsByPredicate([](const FRow& R) { return R.bDisplay; });
}

void SS47Settings::Move(int32 Dir)
{
	const int32 Last = HasButtons() ? Rows.Num() : Rows.Num() - 1;
	int32 I = Sel;
	for (int32 Tries = 0; Tries <= Rows.Num() + 1; ++Tries)
	{
		I += Dir;
		if (I < 0 || I > Last) return; // (no wrap: the ends stay where they are)
		if (I == Rows.Num() || (Rows[I].Kind != FRow::EKind::Sep && (!Rows[I].Usable || Rows[I].Usable()))) break;
	}
	Sel = I;
	if (const FRow* R = Current(); R && R->Kind == FRow::EKind::Keys)
	{
		const S47Keys::FAction* A = S47Keys::Find(R->Action);
		if (A && A->bFixedFirst) KeySlot = 1;
	}
	BtnSel = 0;
	Status.Reset();
	Scrolled();
}

void SS47Settings::Scrolled()
{
	if (!Scroll.IsValid()) return;
	if (Sel >= 0 && Sel < RowWidgets.Num())
	{
		// the group's heading over the first key of a group comes into view with it
		const int32 Show = Sel > 0 && Rows[Sel - 1].Kind == FRow::EKind::Sep ? Sel - 1 : Sel;
		Scroll->ScrollDescendantIntoView(RowWidgets[Show], true, EDescendantScrollDestination::IntoView, 0);
		if (Show != Sel) Scroll->ScrollDescendantIntoView(RowWidgets[Sel], true, EDescendantScrollDestination::IntoView, 0);
	}
}

void SS47Settings::Back()
{
	if (ConfirmLeft >= 0.f) { Revert(); return; }
	if (!Listening.IsNone()) { Listening = NAME_None; Status.Reset(); return; }
	// what was chosen for the display and not applied is let go
	Shown = Applied;
	if (OnBack) OnBack();
}

FReply SS47Settings::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	const FKey K = E.GetKey();
	if (!Listening.IsNone())
	{
		if (K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right) { Listening = NAME_None; Status.Reset(); }
		else if (!E.IsRepeat()) Take(K);
		return FReply::Handled();
	}
	if (ConfirmLeft >= 0.f)
	{
		if (K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom) Keep();
		else if (K == EKeys::Escape || K == EKeys::BackSpace || K == EKeys::Gamepad_FaceButton_Right) Revert();
		return FReply::Handled();
	}
	const FRow* R = Current();
	if (K == EKeys::Up || K == EKeys::Gamepad_DPad_Up) { Move(-1); return FReply::Handled(); }
	if (K == EKeys::Down || K == EKeys::Gamepad_DPad_Down) { Move(1); return FReply::Handled(); }
	if (K == EKeys::Left || K == EKeys::Right || K == EKeys::Gamepad_DPad_Left || K == EKeys::Gamepad_DPad_Right)
	{
		const int32 Dir = K == EKeys::Left || K == EKeys::Gamepad_DPad_Left ? -1 : 1;
		if (OnButtons()) BtnSel = FMath::Clamp(BtnSel + Dir, 0, 1);
		else if (R && R->Step && (!R->Usable || R->Usable())) R->Step(Dir);
		return FReply::Handled();
	}
	if (K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom)
	{
		if (OnButtons()) { if (BtnSel == 0) Apply(); else Revert(); }
		else if (R && R->Activate && (!R->Usable || R->Usable())) R->Activate();
		return FReply::Handled();
	}
	if (K == EKeys::Q || K == EKeys::Gamepad_LeftShoulder || (K == EKeys::Tab && E.IsShiftDown())) { SetTab((Tab + 5) % 6); return FReply::Handled(); }
	if (K == EKeys::E || K == EKeys::Gamepad_RightShoulder || K == EKeys::Tab) { SetTab((Tab + 1) % 6); return FReply::Handled(); }
	if (K == EKeys::R || K == EKeys::Gamepad_FaceButton_Left)
	{
		if (R && R->Default && (!R->Usable || R->Usable())) R->Default();
		return FReply::Handled();
	}
	if ((K == EKeys::BackSpace || K == EKeys::Delete) && R && R->Kind == FRow::EKind::Keys)
	{
		S47Keys::Clear(R->Action, KeySlot);
		KeysChanged();
		return FReply::Handled();
	}
	if (K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right || K == EKeys::Gamepad_Special_Right) { Back(); return FReply::Handled(); }
	return FReply::Handled(); // (the settings keep the keys: nothing reaches the game behind them)
}

FReply SS47Settings::OnPreviewMouseButtonDown(const FGeometry& G, const FPointerEvent& E)
{
	// a mouse button while a key is being taken is the new key (a left click stays Use's)
	if (Listening.IsNone()) return FReply::Unhandled();
	Take(E.GetEffectingButton());
	return FReply::Handled();
}

FNavigationReply SS47Settings::OnNavigation(const FGeometry& G, const FNavigationEvent& E)
{
	// a gamepad's stick comes as navigation (the d-pad and the keys are OnKeyDown's)
	if (!Listening.IsNone() || ConfirmLeft >= 0.f) return FNavigationReply::Stop();
	switch (E.GetNavigationType())
	{
	case EUINavigation::Up: Move(-1); break;
	case EUINavigation::Down: Move(1); break;
	case EUINavigation::Left: case EUINavigation::Right:
	{
		const int32 Dir = E.GetNavigationType() == EUINavigation::Left ? -1 : 1;
		if (OnButtons()) BtnSel = FMath::Clamp(BtnSel + Dir, 0, 1);
		else if (const FRow* R = Current(); R && R->Step) R->Step(Dir);
		break;
	}
	default: break;
	}
	return FNavigationReply::Stop();
}

void SS47Settings::Tick(const FGeometry& G, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(G, InCurrentTime, InDeltaTime);
	S47Theme::KeepFocus(SharedThis(this));
	Blink += InDeltaTime;
	if (ConfirmLeft >= 0.f)
	{
		// (time up: the question is still open as Revert reads it, so the display goes back)
		ConfirmLeft = FMath::Max(0.f, ConfirmLeft - InDeltaTime);
		if (ConfirmLeft <= 0.f) Revert();
	}
}

// ---------- the display (UGameUserSettings) ----------

void SS47Settings::ReadDisplay()
{
	UGameUserSettings* GS = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (GS)
	{
		const EWindowMode::Type M = GS->GetFullscreenMode();
		Applied.Mode = M == EWindowMode::Fullscreen ? 0 : M == EWindowMode::WindowedFullscreen ? 1 : 2;
		Applied.Res = GS->GetScreenResolution();
		Applied.Fps = GS->GetFrameRateLimit();
		Applied.bVSync = GS->IsVSyncEnabled();
	}
	// the window as it is now (a run started with -windowed -ResX= is not what the settings file says)
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->GetWindow().IsValid())
	{
		const TSharedPtr<SWindow> W = GEngine->GameViewport->GetWindow();
		const EWindowMode::Type M = W->GetWindowMode();
		Applied.Mode = M == EWindowMode::Fullscreen ? 0 : M == EWindowMode::WindowedFullscreen ? 1 : 2;
		FVector2D Size(0, 0);
		GEngine->GameViewport->GetViewportSize(Size);
		if (Size.X > 0 && Size.Y > 0) Applied.Res = FIntPoint(FMath::RoundToInt32(Size.X), FMath::RoundToInt32(Size.Y));
	}
	Shown = Applied;
	Resolutions.Reset();
	// the monitor's own list; where the platform gives none (Linux here), the window sizes that fit the
	// desktop and the common 16:9 ones up to its size
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
	TArray<FIntPoint> More;
	UKismetSystemLibrary::GetConvenientWindowedResolutions(More);
	const FIntPoint Desk = GEngine && GEngine->GetGameUserSettings() ? GEngine->GetGameUserSettings()->GetDesktopResolution() : FIntPoint(1920, 1080);
	for (const FIntPoint R : { FIntPoint(1280, 720), FIntPoint(1366, 768), FIntPoint(1600, 900), FIntPoint(1920, 1080), FIntPoint(2560, 1440), FIntPoint(3200, 1800), FIntPoint(3840, 2160) })
	{
		if (R.X <= Desk.X && R.Y <= Desk.Y) More.Add(R);
	}
	if (Desk.X > 0 && Desk.Y > 0) More.Add(Desk);
	for (const FIntPoint& R : More) Resolutions.AddUnique(R);
	Resolutions.AddUnique(Applied.Res);
	Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X * A.Y != B.X * B.Y ? A.X * A.Y < B.X * B.Y : A.X < B.X; });
	UE_LOG(LogS47Settings, Display, TEXT("S47: display %s %dx%d, %d resolutions offered, frame limit %.0f, v-sync %s"), S47SettingsLocal::ModeNames[Applied.Mode], Applied.Res.X, Applied.Res.Y, Resolutions.Num(), Applied.Fps, Applied.bVSync ? TEXT("on") : TEXT("off"));
}

void SS47Settings::ApplyDisplay(const FDisplay& D)
{
	UGameUserSettings* GS = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!GS) return;
	GS->SetFullscreenMode(D.Mode == 0 ? EWindowMode::Fullscreen : D.Mode == 1 ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
	GS->SetScreenResolution(D.Res);
	GS->SetFrameRateLimit(D.Fps);
	GS->SetVSyncEnabled(D.bVSync);
	// (ApplySettings writes GameUserSettings.ini as well)
	GS->ApplySettings(false);
	Applied = D;
}

void SS47Settings::Apply()
{
	if (Shown == Applied) return;
	Before = Applied;
	const bool bAsk = Shown.Mode != Applied.Mode || Shown.Res != Applied.Res;
	ApplyDisplay(Shown);
	ConfirmLeft = bAsk ? 10.f : -1.f;
	Status = bAsk ? FString() : TEXT("Applied.");
	UE_LOG(LogS47Settings, Display, TEXT("S47: display applied: %s %dx%d, frame limit %.0f, v-sync %s%s"), S47SettingsLocal::ModeNames[Shown.Mode], Shown.Res.X, Shown.Res.Y, Shown.Fps, Shown.bVSync ? TEXT("on") : TEXT("off"), bAsk ? TEXT(", to keep or go back") : TEXT(""));
}

void SS47Settings::Keep()
{
	ConfirmLeft = -1.f;
	Status = TEXT("Applied.");
}

void SS47Settings::Revert()
{
	if (ConfirmLeft >= 0.f)
	{
		// the new mode or size was not kept: back to the one before
		ConfirmLeft = -1.f;
		ApplyDisplay(Before);
		Shown = Before;
		UE_LOG(LogS47Settings, Display, TEXT("S47: display put back: %s %dx%d"), S47SettingsLocal::ModeNames[Before.Mode], Before.Res.X, Before.Res.Y);
		Status = TEXT("The display is back as it was.");
		return;
	}
	Shown = Applied;
	Status.Reset();
}

// ---------- the keys ----------

void SS47Settings::Listen(FName Action, int32 Slot)
{
	const S47Keys::FAction* A = S47Keys::Find(Action);
	if (!A) return;
	if (Slot == 0 && A->bFixedFirst) { Status = TEXT("Escape always pauses. The second key can be another."); return; }
	Listening = Action;
	KeySlot = Slot;
	Blink = 0.f;
	Status = FString::Printf(TEXT("Press the new key for %s."), *A->Label.ToLower());
}

void SS47Settings::Take(const FKey& K)
{
	const FName Action = Listening;
	Listening = NAME_None;
	const S47Keys::FAction* A = S47Keys::Find(Action);
	if (!A) return;
	FName Other;
	switch (S47Keys::Bind(Action, KeySlot, K, &Other))
	{
	case S47Keys::EBind::Done: Status = FString::Printf(TEXT("%s: %s."), *A->Label, *S47Keys::Short(K)); break;
	case S47Keys::EBind::Same: Status = FString::Printf(TEXT("%s is already %s's."), *S47Keys::Short(K), *A->Label.ToLower()); break;
	case S47Keys::EBind::Swapped:
	{
		const S47Keys::FAction* B = S47Keys::Find(Other);
		Status = B && Other != Action ? FString::Printf(TEXT("%s was %s's. They have changed places."), *S47Keys::Short(K), *B->Label.ToLower())
			: FString::Printf(TEXT("%s: the two keys have changed places."), *A->Label);
		break;
	}
	case S47Keys::EBind::Shared:
	{
		// (the shutter and a jump: the camera decides which it is)
		const S47Keys::FAction* B = S47Keys::Find(Other);
		Status = B ? FString::Printf(TEXT("%s: %s. It is %s's too; the camera decides which."), *A->Label, *S47Keys::Short(K), *B->Label.ToLower())
			: FString::Printf(TEXT("%s: %s."), *A->Label, *S47Keys::Short(K));
		break;
	}
	case S47Keys::EBind::Moved:
	{
		const S47Keys::FAction* B = S47Keys::Find(Other);
		Status = B ? FString::Printf(TEXT("%s was %s's. Give %s another key."), *S47Keys::Short(K), *B->Label.ToLower(), *B->Label.ToLower())
			: FString::Printf(TEXT("%s: %s."), *A->Label, *S47Keys::Short(K));
		break;
	}
	case S47Keys::EBind::Fixed: Status = TEXT("Escape stays the pause's."); break;
	case S47Keys::EBind::Reserved:
		Status = K == EKeys::LeftMouseButton ? TEXT("A click always uses. Choose another key.")
			: K == EKeys::Escape ? TEXT("Escape is kept for the pause and the menus.") : TEXT("That key cannot be used for this. Choose another.");
		break;
	}
	KeysChanged();
}

void SS47Settings::KeysChanged()
{
	if (O->OnKeys) O->OnKeys();
}

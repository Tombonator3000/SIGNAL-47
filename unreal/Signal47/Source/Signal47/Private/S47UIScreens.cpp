// ui/UI.ts: the screens (title, ungate, openingCard, pause, chapterCard, chapterCut, notebook).
// The widgets are in SS47Screens and SS47Notebook; this is the UI's side of them.
#include "S47UI.h"

#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "S47HUD.h"
#include "S47ReadAloud.h"
#include "S47Story.h"
#include "SS47Notebook.h"
#include "SS47Screens.h"
#include "SS47Settings.h"
#include "SS47Widgets.h"

namespace
{
	// over the game and its HUD (20), under the panels (30): the title; the chapter card over the fade
	constexpr int32 ZTitle = 25, ZCard = 26;
}

TSharedPtr<SWidget> US47UI::Title(const FS47TitleOpts& InO)
{
	UGameViewportClient* V = GEngine ? GEngine->GameViewport : nullptr;
	if (!V) return nullptr;
	FS47TitleOpts O = InO;
	// the note's link: the settings at their Accessibility tab
	TWeakObjectPtr<US47UI> Self = this;
	if (!O.OnSettingsAt && O.OnSettings) O.OnSettingsAt = [Self, Fn = O.OnSettings](const FString& Tab) { if (Self.IsValid()) Self->SettingsAt(Tab); Fn(); };
	TSharedRef<SS47Title> El = SNew(SS47Title).Opts(O);
	// (out of sight under a menu opened over it: the settings and the case menus blur what is behind them)
	El->SetVisibility(TAttribute<EVisibility>::CreateLambda([Self] { return Self.IsValid() && Self->Modal() ? EVisibility::Hidden : EVisibility::Visible; }));
	V->AddViewportWidgetContent(El, ZTitle);
	// the title has the keys and the mouse (it is not a modal, so the HUD does not do this)
	if (APlayerController* PC = Hud ? Hud->GetOwningPlayerController() : nullptr)
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(El);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}
	El->FocusMenu();
	return El;
}

void US47UI::Ungate(const TSharedPtr<SWidget>& El)
{
	if (El.IsValid() && El->GetType() == TEXT("SS47Title")) StaticCastSharedPtr<SS47Title>(El)->Ungate();
}

void US47UI::RemoveScreen(const TSharedPtr<SWidget>& El)
{
	if (El.IsValid() && GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(El.ToSharedRef());
}

FS47OpeningView US47UI::OpeningCard(const FS47OpeningActions& A)
{
	TSharedRef<SS47Opening> El = SNew(SS47Opening).Actions(A);
	Open(El, A.Closed);
	El->FocusBegin();
	TWeakPtr<SS47Opening> W = El;
	TWeakObjectPtr<US47UI> Self = this;
	FS47OpeningView View;
	// close: () => { if (this.modal === el) this.close(true); }
	View.Close = [W, Self] { if (Self.IsValid() && Self->Hud && W.IsValid() && Self->Hud->ModalWidget() == W.Pin()) Self->Close(true); };
	View.State = [W](const FString& S) { if (const TSharedPtr<SS47Opening> P = W.Pin()) P->SetState(S); };
	return View;
}

void US47UI::Pause(const FS47PauseOpts& O)
{
	// one copy of the options for the pause menu and the settings opened from it, so the settings
	// changed there are what the pause menu opens them with next
	const TSharedRef<FS47PauseOpts> Shared = MakeShared<FS47PauseOpts>(O);
	if (O.bSettingsOnly)
	{
		SettingsScreen(Shared, O.Tab.IsEmpty() ? SettingsTab : O.Tab, O.OnResume);
		SettingsTab.Reset();
		return;
	}
	PauseScreen(Shared, 0);
}

void US47UI::PauseScreen(const TSharedRef<FS47PauseOpts>& O, int32 Sel)
{
	TWeakObjectPtr<US47UI> Self = this;
	// set when the pause menu gives way to a menu of its own: the game stays paused under it
	const TSharedRef<bool> bSub = MakeShared<bool>(false);
	TSharedRef<SS47Pause> El = SNew(SS47Pause).Opts(O).Selected(Sel)
		.OnClose([Self](bool bSilent) { if (Self.IsValid()) Self->Close(bSilent); })
		.OnSettings([Self, O, bSub](const FString& Tab, int32 From)
		{
			if (!Self.IsValid()) return;
			*bSub = true;
			Self->SettingsScreen(O, Tab, [Self, O, From] { if (Self.IsValid()) Self->PauseScreen(O, From); });
		})
		.OnSub([bSub](TFunction<void()> Fn) { *bSub = true; if (Fn) Fn(); });
	Open(El, [O, bSub] { if (!*bSub && O->OnResume) O->OnResume(); });
}

void US47UI::SettingsScreen(const TSharedRef<FS47PauseOpts>& O, const FString& Tab, TFunction<void()> OnClose)
{
	TWeakObjectPtr<US47UI> Self = this;
	Open(SNew(SS47Settings).Opts(O).Tab(Tab).OnBack([Self] { if (Self.IsValid()) Self->Close(); }), MoveTemp(OnClose));
	UE_LOG(LogTemp, Display, TEXT("S47: settings opened at %s"), Tab.IsEmpty() ? TEXT("game") : *Tab);
}

void US47UI::ChapterCard(const FS47ChapterCardOpts* O, TFunction<void()> OnSpoken)
{
	const int32 Generation = ++CardGeneration;
	ChapterStage = TEXT("idle");
	if (CardEl.IsValid())
	{
		// the old card fades out (.8 s) and goes after .9 s
		const TSharedPtr<SS47ChapterCard> Old = CardEl;
		CardEl.Reset();
		Old->Out(false);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Old](float)
		{
			if (GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(Old.ToSharedRef());
			return false;
		}), 0.9f);
	}
	if (Narrator.IsValid()) Narrator->Stop();
	// (the promise settles at once when there is no card, or no voice reading it)
	if (!O) { if (OnSpoken) OnSpoken(); return; }
	// this.lastDescription = o.recap (V reads it)
	if (SS47Hud* H = Hud ? Hud->HudWidget() : nullptr) H->LastToast = O->Recap;
	UpdateDescriptionButton();
	UGameViewportClient* V = GEngine ? GEngine->GameViewport : nullptr;
	if (!V) { if (OnSpoken) OnSpoken(); return; }
	TSharedRef<SS47ChapterCard> El = SNew(SS47ChapterCard).Opts(*O).Visibility(EVisibility::HitTestInvisible);
	V->AddViewportWidgetContent(El, ZCard);
	CardEl = El;
	// Without the narrator the card has its own timing (.chapter-card's animations): the stage stays
	// 'idle', and main.ts's chapterBreak ends it at 6.4 s.
	const TSharedPtr<FS47ReadAloud> Reader = Narrator;
	if (!Reader.IsValid() || !Reader->Enabled()) { if (OnSpoken) OnSpoken(); return; }
	El->Voice(TEXT("a")); ChapterStage = TEXT("a");
	const TWeakObjectPtr<US47UI> Self = this;
	const TWeakPtr<SS47ChapterCard> WeakEl = El;
	const TWeakPtr<FS47ReadAloud> WeakReader = Reader;
	auto Current = [Self, Generation, WeakEl] { return Self.IsValid() && Generation == Self->CardGeneration && WeakEl.IsValid() && Self->CardEl == WeakEl.Pin(); };
	if (OnReadingStart) OnReadingStart(Reader.Get());
	// (const run = reader.runId, read once play() has stopped what was reading: the next run is this one)
	const int32 Run = Reader->RunId() + 1;
	Reader->Play({ O->Closed, O->Recap, O->Next, O->Title, O->Clock }, {}, [Self, Current, WeakEl, WeakReader](int32 Index)
	{
		const TSharedPtr<FS47ReadAloud> R = WeakReader.Pin();
		if (Index == 2 && Current() && R.IsValid() && R->Enabled()) { if (const TSharedPtr<SS47ChapterCard> E = WeakEl.Pin()) E->Voice(TEXT("b")); Self->ChapterStage = TEXT("b"); }
	}, [Self, Current, WeakEl, WeakReader, Run, OnSpoken]
	{
		// await narration: then the stage says how the reading ended, and the card's promise settles
		const TSharedPtr<FS47ReadAloud> R = WeakReader.Pin();
		if (Current() && R.IsValid())
		{
			const TSharedPtr<SS47ChapterCard> E = WeakEl.Pin();
			if (!R->Enabled()) { if (E.IsValid()) E->Voice(TEXT("b")); Self->ChapterStage = TEXT("text-b"); }
			else if (R->RunId() != Run) Self->ChapterStage = TEXT("cancelled");
			// A failed load still gives the next chapter readable screen time. No
			// replacement voice or later phase of speech starts after the failure.
			else if (R->State() == TEXT("error")) { if (E.IsValid()) E->Voice(TEXT("b")); Self->ChapterStage = TEXT("error-b"); }
		}
		if (OnSpoken) OnSpoken();
	});
}

void US47UI::ChapterCut()
{
	// the card cut away at once, and the black with it (instead of the slow fade)
	if (CardEl.IsValid()) CardEl->Out(true);
	const TSharedPtr<SS47ChapterCard> Cut = CardEl;
	CardEl.Reset();
	ChapterCard(nullptr);
	if (Cut.IsValid())
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Cut](float)
		{
			if (GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(Cut.ToSharedRef());
			return false;
		}), 0.9f);
	}
	// (.fade.quick: the HUD's fade has one speed)
	Fade(false);
}

void US47UI::Notebook(const TArray<FS47Task>& Tasks, const TArray<FString>& Notes, const TArray<FS47Doc>& Docs, TFunction<void(const FS47Doc&)> OnDoc,
	const FString& Heading, TFunction<void()> OnBoard, const FS47NbExtras& Extras)
{
	Open(SNew(SS47Notebook).UI(this).Tasks(Tasks).Notes(Notes).Docs(Docs).OnDoc(OnDoc).Heading(Heading).OnBoard(OnBoard).Extras(Extras));
}

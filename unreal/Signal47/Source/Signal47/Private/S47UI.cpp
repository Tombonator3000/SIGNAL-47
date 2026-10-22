#include "S47UI.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/Input/SButton.h"
#include "Internationalization/Regex.h"
#include "S47EarlDialogue.h"
#include "S47HUD.h"
#include "S47Flow.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47NpcDialogue.h"
#include "S47ReadAloud.h"
#include "SS47Widgets.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// a .read-aloud box's state: whether its reading is going (aria-pressed), and the status under it
	struct FReadingBox
	{
		bool bActive = false;
		bool bAvailable = false;
		FString State;          // '' until update() has been called; then 'loading' | 'playing' | 'idle' | 'error'
	};

	// UI.readingControls' box, for a reader that is on: the button (Read ... / Stop reading) and its status
	TPair<TSharedRef<SWidget>, TSharedRef<FReadingBox>> MakeReadingBox(US47UI* UI, const TSharedRef<FS47ReadAloud>& Reader, const TArray<FString>& Texts, const FString& Label, bool bPaper = false)
	{
		const TSharedRef<FReadingBox> B = MakeShared<FReadingBox>();
		B->bAvailable = Reader->Available(Texts);
		const TWeakPtr<FS47ReadAloud> R = Reader;
		const TWeakObjectPtr<US47UI> U = UI;
		auto Update = [B](const FString& State) { B->State = State; B->bActive = State == TEXT("loading") || State == TEXT("playing"); };
		const TAttribute<FString> Shown = TAttribute<FString>::CreateLambda([B, Label] { return B->bActive ? FString(TEXT("Stop reading")) : Label; });
		const TFunction<void()> Click = [B, R, U, Texts, Update]
		{
			const TSharedPtr<FS47ReadAloud> P = R.Pin();
			if (!P.IsValid()) return;
			if (B->bActive) { P->Stop(); return; }
			if (U.IsValid() && U->OnReadingStart) U->OnReadingStart(P.Get());
			P->Play(Texts, Update);
		};
		const TAttribute<bool> Enabled = TAttribute<bool>::CreateLambda([B] { return B->bAvailable; });
		// (.notebook > .read-aloud: on the notebook's paper the button is ink on tan, the status darker)
		static const FButtonStyle PaperButton = []
		{
			FButtonStyle St;
			const FSlateRoundedBoxBrush N(S47Kit::Css(0xd4c8ac), 0.f, S47Kit::Css(0x786b51), 1.f), H(S47Kit::Css(0xc9bc9e), 0.f, S47Kit::Css(0x786b51), 1.f);
			St.SetNormal(N).SetHovered(H).SetPressed(H).SetDisabled(N).SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
			return St;
		}();
		const TSharedRef<SWidget> Button = !bPaper ? S47Kit::ButtonLive(Shown, Click, false, Enabled)
			: StaticCastSharedRef<SWidget>(SNew(SButton).ButtonStyle(&PaperButton).ContentPadding(FMargin(14, 6)).IsEnabled(Enabled)
				.OnClicked_Lambda([Click] { Click(); return FReply::Handled(); })
				[
					SNew(SBox).MinDesiredHeight(44.f - 12.f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text_Lambda([Shown] { return FText::FromString(Shown.Get()); }).Font(S47Style::Mono(20.f * 1.25f))
						.ColorAndOpacity_Lambda([B] { return FSlateColor(S47Kit::Css(0x1e2430, B->bAvailable ? 1.f : 0.5f)); })
					]
				]);
		const TSharedRef<SWidget> Status = SNew(STextBlock).Font(S47Style::Body(12.f * 1.25f)).ColorAndOpacity(bPaper ? S47Kit::Css(0x5a5446) : S47Kit::Css(0xb6ae9b)).AutoWrapText(true)
			.Text_Lambda([B, R]
			{
				const TSharedPtr<FS47ReadAloud> P = R.Pin();
				const FString Name = P.IsValid() ? P->Label() : FString();
				if (B->State.IsEmpty()) return FText::FromString(B->bAvailable ? Name : FString(TEXT("No recording for this text.")));
				return FText::FromString(B->State == TEXT("loading") ? FString(TEXT("Loading recording…")) : B->State == TEXT("playing") ? Name
					: B->State == TEXT("error") ? FString(TEXT("Recording could not be played. Try again.")) : Name);
			});
		// .read-aloud { display: flex; align-items: center; flex-wrap: wrap; gap: 6px 14px; padding: 10px 0 }
		const TSharedRef<SWrapBox> Row = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(14, 6) * 1.25f);
		Row->AddSlot().VAlign(VAlign_Center)[Button];
		Row->AddSlot().VAlign(VAlign_Center)[Status];
		return { S47Kit::Unread(SNew(SBox).Padding(FMargin(0, 10 * 1.25f))[Row]), B };
	}

	// the texts the description reader can read in a widget, in their order: the visible ones S47Kit
	// marks readable, each split into its paragraphs (documentReadingParagraphs), none under a part
	// marked not to be read
	void CollectReadable(const TSharedRef<SWidget>& W, TArray<FString>& Out)
	{
		if (!W->GetVisibility().IsVisible()) return;
		if (const TSharedPtr<FS47ReadingMark> M = W->GetMetaData<FS47ReadingMark>())
		{
			if (!M->bRead) return;
			if (W->GetType() == FName(TEXT("STextBlock"))) { Out.Append(S47Reading::DocumentReadingParagraphs(StaticCastSharedRef<STextBlock>(W)->GetText().ToString())); return; }
		}
		if (FChildren* C = W->GetChildren()) for (int32 I = 0; I < C->Num(); ++I) CollectReadable(C->GetChildAt(I), Out);
	}
}

// UI.descriptionReadings: the box that follows the descriptions visible in its parent. Observe text
// changes, not frames: hints, answers and tab changes replace the old queue at once, without restarting
// a recording on every redraw. (No MutationObserver in Slate: the texts are looked at a few times a
// second, US47UI's ticker, and the box is rebuilt only when they have changed.)
class SS47Readings : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Readings) {}
	SLATE_END_ARGS()
	void Construct(const FArguments&, US47UI* InUI, const TSharedRef<SWidget>& InParent, const FString& InKind, TFunction<TArray<FString>()> InExtra)
	{
		UI = InUI; Parent = InParent; Kind = InKind; Extra = MoveTemp(InExtra);
		Reader = InUI ? InUI->DescriptionReader() : nullptr;
		ChildSlot[SNullWidget::NullWidget];
	}
	void Refresh()
	{
		const TSharedPtr<SWidget> P = Parent.Pin();
		if (!P.IsValid() || !UI.IsValid()) return;
		TArray<FString> Texts;
		CollectReadable(P.ToSharedRef(), Texts);
		if (Extra) Texts.Append(Extra());
		const FString Next = FString::Join(Texts, TEXT("\x1f")) + FString::Printf(TEXT("\x1e%d"), Texts.Num());
		if (Next == Key && (!Texts.Num() || Box.IsValid())) return;
		Key = Next;
		if (Box.IsValid() && Box->bActive) if (const TSharedPtr<FS47ReadAloud> R = Reader.Pin()) R->Stop();
		ChildSlot[SNullWidget::NullWidget];
		Box.Reset();
		if (!Texts.Num()) return;
		// this.readingControls(parent, texts, 'Read description', kind)
		const TSharedPtr<FS47ReadAloud> R = UI->DescriptionReader();
		if (!R.IsValid() || !R->Enabled()) return;
		const TPair<TSharedRef<SWidget>, TSharedRef<FReadingBox>> B = MakeReadingBox(UI.Get(), R.ToSharedRef(), Texts, TEXT("Read description"), Kind == TEXT("notebook-description"));
		ChildSlot[B.Key];
		Box = B.Value;
	}
private:
	TWeakObjectPtr<US47UI> UI;
	TWeakPtr<SWidget> Parent;
	FString Kind, Key;
	TFunction<TArray<FString>()> Extra;
	TWeakPtr<FS47ReadAloud> Reader;
	TSharedPtr<FReadingBox> Box;
};

namespace
{
	// the boxes following their descriptions, looked at by one ticker (the observers)
	TArray<TWeakPtr<SS47Readings>> GWatches;
	FTSTicker::FDelegateHandle GWatchTicker;
	void Watch(const TSharedRef<SS47Readings>& W)
	{
		GWatches.Add(W);
		if (GWatchTicker.IsValid()) return;
		GWatchTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
		{
			GWatches.RemoveAll([](const TWeakPtr<SS47Readings>& X) { return !X.IsValid(); });
			for (const TWeakPtr<SS47Readings>& X : TArray<TWeakPtr<SS47Readings>>(GWatches)) if (const TSharedPtr<SS47Readings> P = X.Pin()) P->Refresh();
			if (GWatches.Num()) return true;
			GWatchTicker.Reset();
			return false;
		}), 0.15f);
	}
}

// ---------- the readers ----------

TSharedPtr<FS47ReadAloud> US47UI::DescriptionReader() const
{
	if (Narrator.IsValid() && Narrator->Enabled()) return Narrator;
	if (ReadAloud.IsValid() && ReadAloud->Enabled()) return ReadAloud;
	return nullptr;
}

void US47UI::StopSpeech()
{
	if (ReadAloud.IsValid()) ReadAloud->Stop();
	if (Narrator.IsValid()) Narrator->Stop();
	if (EarlDialogue.IsValid()) EarlDialogue->Stop();
	if (NpcDialogue.IsValid()) NpcDialogue->Stop();
}

TSharedRef<SWidget> US47UI::ReadingControls(const TArray<FString>& Texts, const FString& Label, const FString& Kind)
{
	const TSharedPtr<FS47ReadAloud> Reader = Kind == TEXT("document") || Kind == TEXT("conversation") ? ReadAloud : DescriptionReader();
	if (!Reader.IsValid() || !Reader->Enabled()) return SNullWidget::NullWidget;
	return MakeReadingBox(this, Reader.ToSharedRef(), Texts, Label).Key;
}

TSharedRef<SWidget> US47UI::DescriptionReadings(const TSharedRef<SWidget>& Parent, const FString& Kind, TFunction<TArray<FString>()> Extra)
{
	const TSharedPtr<FS47ReadAloud> Reader = DescriptionReader();
	if (!Reader.IsValid() || !Reader->Enabled()) return SNullWidget::NullWidget;
	const TSharedRef<SS47Readings> W = SNew(SS47Readings, this, Parent, Kind, MoveTemp(Extra));
	W->Refresh();
	Watch(W);
	return W;
}

// ---------- a reading as an action (the paper's READ ALOUD, the notebook's) ----------

void FS47ReadingAction::Toggle()
{
	// the .read-aloud button's click: stop it while it reads; else the others stop and it starts
	const TSharedPtr<FS47ReadAloud> R = Reader.Pin();
	if (!R.IsValid()) return;
	if (bActive) { R->Stop(); return; }
	if (!bAvailable) return;
	if (UI.IsValid() && UI->OnReadingStart) UI->OnReadingStart(R.Get());
	// (the state comes back through the reader; the action may be gone by then)
	const TWeakPtr<FS47ReadingAction> Self = AsWeak();
	R->Play(Texts, [Self](const FString& St)
	{
		if (const TSharedPtr<FS47ReadingAction> A = Self.Pin()) { A->State = St; A->bActive = St == TEXT("loading") || St == TEXT("playing"); }
	});
}

void FS47ReadingAction::Stop()
{
	if (const TSharedPtr<FS47ReadAloud> R = Reader.Pin()) R->Stop();
	bActive = false;
}

FString FS47ReadingAction::Status() const
{
	// (as the box's status line under its button)
	const TSharedPtr<FS47ReadAloud> R = Reader.Pin();
	const FString Name = R.IsValid() ? R->Label() : FString();
	if (State.IsEmpty()) return bAvailable ? Name : FString(TEXT("No recording for this text."));
	return State == TEXT("loading") ? FString(TEXT("Loading recording…")) : State == TEXT("error") ? FString(TEXT("Recording could not be played. Try again.")) : Name;
}

TSharedPtr<FS47ReadingAction> US47UI::ReadingAction(const TArray<FString>& Texts, const FString& Kind)
{
	// the reader readingControls would give the box: Kinski for a paper and a conversation, else the
	// description reader
	const TSharedPtr<FS47ReadAloud> R = Kind == TEXT("document") || Kind == TEXT("conversation") ? ReadAloud : DescriptionReader();
	if (!R.IsValid() || !R->Enabled() || !Texts.Num()) return nullptr;
	const TSharedRef<FS47ReadingAction> A = MakeShared<FS47ReadingAction>();
	A->Reader = R;
	A->UI = this;
	A->Texts = Texts;
	A->bAvailable = R->Available(Texts);
	return A;
}

void US47UI::Noted(const FString& Text)
{
	// (not while a case is opened or a night starts: the lines it had come back then)
	const US47Main* M = US47Main::Get(this);
	if (M && M->Flow && (M->Flow->bStarting || M->Flow->bRestoringNow)) return;
	if (SS47Hud* H = Hud ? Hud->HudWidget() : nullptr) H->Noted(Text);
}

void US47UI::ClearExamineTimer()
{
	if (ExamineTimer.IsValid()) FTSTicker::RemoveTicker(ExamineTimer);
	ExamineTimer.Reset();
}

void US47UI::UpdateDescriptionButton()
{
	SS47Hud* H = Hud ? Hud->HudWidget() : nullptr;
	if (!H) return;
	const TSharedPtr<FS47ReadAloud> R = DescriptionReader();
	const FString Last = LastDescription();
	H->DescButton.bShown = R.IsValid() && R->Enabled() && !(ExamineText.IsEmpty() && Last.IsEmpty()) && !Modal() && !(CanDescribe && !CanDescribe());
	H->DescButton.Text = bDescriptionActive ? TEXT("Stop reading [V]") : !ExamineText.IsEmpty() ? TEXT("Read description [V]") : TEXT("Read last description [V]");
	H->DescButton.bDisabled = !bDescriptionActive && !(R.IsValid() && R->Available({ ExamineText.IsEmpty() ? Last : ExamineText }));
}

void US47UI::PlayDescription(const FString& Text, bool bAutomatic)
{
	const TSharedPtr<FS47ReadAloud> Reader = DescriptionReader();
	if (!Reader.IsValid() || !Reader->Enabled() || Text.IsEmpty()) return;
	if (OnReadingStart) OnReadingStart(Reader.Get());
	const TWeakObjectPtr<US47UI> Self = this;
	Reader->Play({ Text }, [Self, bAutomatic](const FString& State)
	{
		if (!Self.IsValid()) return;
		Self->bDescriptionActive = State == TEXT("playing") || State == TEXT("loading");
		Self->bExamineActive = bAutomatic && Self->bDescriptionActive;
		Self->UpdateDescriptionButton();
	});
}

void US47UI::ReadLastDescription()
{
	const TSharedPtr<FS47ReadAloud> Reader = DescriptionReader();
	if (!Reader.IsValid() || !Reader->Enabled() || Modal() || (CanDescribe && !CanDescribe())) return;
	if (bDescriptionActive) Reader->Stop();
	else PlayDescription(!ExamineText.IsEmpty() ? ExamineText : LastDescription());
}

void US47UI::Toast(const FString& Text, float Secs, TFunction<double()> Clock)
{
	if (SS47Hud* H = Hud ? Hud->HudWidget() : nullptr) H->Toast(Text, Secs, MoveTemp(Clock));
}

void US47UI::ClearToasts()
{
	// (and lastDescription with them, SS47Hud's; the closer look's words; the button)
	if (SS47Hud* H = Hud ? Hud->HudWidget() : nullptr) H->ClearToasts();
	Examine(FString());
	UpdateDescriptionButton();
}

FString US47UI::LastDescription() const
{
	// UI.toast keeps it: the last toast that was not a caption (radio, telephone, the CB: SS47Hud::Toast)
	SS47Hud* H = Hud ? Hud->HudWidget() : nullptr;
	return H ? H->LastToast : FString();
}

void US47UI::Objective(const FString& Text)
{
	if (SS47Hud* H = Hud ? Hud->HudWidget() : nullptr) H->Objective(Text);
}

void US47UI::Examine(const FString& Text)
{
	// What a closer look says (the zoom), under the crosshair; '' clears it. Once it has held a moment
	// the description reader reads it (main.ts calls this every frame; the button follows from here,
	// also for the toasts the story shows without US47UI)
	const TSharedPtr<FS47ReadAloud> Reader = DescriptionReader();
	if (bDescriptionActive && CanDescribe && !CanDescribe() && Reader.IsValid()) Reader->Stop();
	if (SS47Hud* H = Hud ? Hud->HudWidget() : nullptr) H->Examine(Text);
	if (ExamineText == Text) { UpdateDescriptionButton(); return; }
	ExamineText = Text;
	ClearExamineTimer();
	if (bExamineActive && Reader.IsValid()) Reader->Stop();
	bExamineActive = false;
	UpdateDescriptionButton();
	if (Text.IsEmpty() || !Reader.IsValid() || !Reader->Enabled()) return;
	const TWeakObjectPtr<US47UI> Self = this;
	ExamineTimer = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Text](float)
	{
		if (!Self.IsValid()) return false;
		Self->ExamineTimer.Reset();
		const TSharedPtr<FS47ReadAloud> R = Self->DescriptionReader();
		if (!R.IsValid() || !R->Enabled() || Self->Modal() || !(Self->CanDescribe && Self->CanDescribe()) || R->Playing() || R->Loading() || !R->Available({ Text })) return false;
		Self->PlayDescription(Text, true);
		return false;
	}), 0.65f);
}

void US47UI::Fade(bool bOn, const FString& Text)
{
	if (Hud) Hud->Fade(bOn, Text);
}

bool US47UI::Faded() const
{
	SS47Hud* H = Hud ? Hud->HudWidget() : nullptr;
	return H && H->IsFaded();
}

void US47UI::ClockSeconds(const FString& Text)
{
	if (SS47Hud* H = Hud ? Hud->HudWidget() : nullptr) H->ClockSeconds(Text);
}

void US47UI::Document(const FS47Doc& Doc, TFunction<void()> OnClose)
{
	// (open() closes first: the examine timer, stopSpeech, the chapter card; the page's Read document and
	// Read description boxes are made by the view itself, AS47HUD::ShowDocument, which the prologue's
	// papers go through too)
	ClearExamineTimer();
	StopSpeech();
	ChapterCard(nullptr);
	if (Hud) Hud->ShowDocument(Doc, MoveTemp(OnClose));
}

void US47UI::Open(TSharedRef<SWidget> Widget, TFunction<void()> OnClose)
{
	// open() closes what was open first: close(true) clears the examine timer, stops the speech and takes
	// the chapter card away
	ClearExamineTimer();
	StopSpeech();
	ChapterCard(nullptr);
	if (Hud) Hud->OpenModal(Widget, MoveTemp(OnClose));
}

void US47UI::Close(bool bSilent)
{
	(void)bSilent;
	ClearExamineTimer();
	StopSpeech();
	ChapterCard(nullptr);
	if (Hud) Hud->CloseModal();
	UpdateDescriptionButton();
}

bool US47UI::Modal() const { return Hud && Hud->HasModal(); }

void US47UI::Viewfinder(bool bOn, const FS47Viewfinder& V)
{
	SS47Hud* H = Hud ? Hud->HudWidget() : nullptr;
	if (!H) return;
	SS47Hud::FViewfinder F;
	F.bOn = bOn;
	if (bOn)
	{
		F.Rect = FSlateRect(V.X, V.Y, V.X + V.W, V.Y + V.H);
		F.Frame = V.Frame;
		F.Status = V.Status;
		F.bOk = V.bOk;
		F.Free = V.bHasFree ? V.FreeText : FString();
		F.bFreeOk = V.bHasFree && V.bFreeOk;
		F.Hint = V.bTouch ? TEXT("CAMERA: LOWER   SHUTTER: EXPOSE") : TEXT("C  LOWER     SPACE  SHUTTER");
	}
	H->Viewfinder(F);
}

void US47UI::ShutterFlash()
{
	if (SS47Hud* H = Hud ? Hud->HudWidget() : nullptr) H->ShutterFlash();
}

void US47UI::Prompt(const FString& Label, const FString& Alt)
{
	if (Hud) Hud->SetPrompt(Label, Alt);
}

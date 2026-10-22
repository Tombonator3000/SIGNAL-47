#include "S47UI.h"

#include "Internationalization/Regex.h"
#include "S47HUD.h"
#include "SS47Widgets.h"

void US47UI::Toast(const FString& Text, float Secs, TFunction<double()> Clock)
{
	if (SS47Hud* H = Hud ? Hud->HudWidget() : nullptr) H->Toast(Text, Secs, MoveTemp(Clock));
}

void US47UI::ClearToasts()
{
	if (SS47Hud* H = Hud ? Hud->HudWidget() : nullptr) H->ClearToasts();
}

FString US47UI::LastDescription() const
{
	// UI.toast: captions (radio, telephone, the CB) are not descriptions
	SS47Hud* H = Hud ? Hud->HudWidget() : nullptr;
	if (!H) return FString();
	static const FRegexPattern Caption(TEXT("^(?:RADIO(?:,|:)|CB,|WARD:|DR\\. WARD:|NORA:|N\\. VEGA:|THE LINE:|YOU:|TOM[ÁA]S:)"), ERegexPatternFlags::CaseInsensitive);
	FRegexMatcher M(Caption, H->LastToast);
	return M.FindNext() ? FString() : H->LastToast;
}

void US47UI::Objective(const FString& Text)
{
	if (SS47Hud* H = Hud ? Hud->HudWidget() : nullptr) H->Objective(Text);
}

void US47UI::Examine(const FString& Text)
{
	if (SS47Hud* H = Hud ? Hud->HudWidget() : nullptr) H->Examine(Text);
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
	if (Hud) Hud->ShowDocument(Doc, MoveTemp(OnClose));
}

void US47UI::Open(TSharedRef<SWidget> Widget, TFunction<void()> OnClose)
{
	ChapterCard(nullptr); // (open() closes what was open first, and close() takes the chapter card away)
	if (Hud) Hud->OpenModal(Widget, MoveTemp(OnClose));
}

void US47UI::Close(bool bSilent)
{
	(void)bSilent;
	ChapterCard(nullptr);
	if (Hud) Hud->CloseModal();
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

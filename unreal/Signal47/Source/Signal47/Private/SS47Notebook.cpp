#include "SS47Notebook.h"

#include "Engine/Texture2D.h"
#include "Internationalization/Regex.h"
#include "Rendering/DrawElements.h"
#include "S47Image.h"
#include "S47Kit.h"
#include "SS47AreaMap.h"
#include "SS47Screens.h"
#include "SS47Widgets.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

using namespace S47Css;

namespace
{
	const FLinearColor InkBlue() { return C(0x233a7a); }
	const FLinearColor Ink() { return C(0x1e2430); }
	const float NbW = 980.f; // .notebook: width min(980px, 100%)

	// the faint blue rules of a notebook page (repeating-linear-gradient, one every 30 px)
	class SRuled : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SRuled) {}
		SLATE_END_ARGS()
		void Construct(const FArguments&) {}
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1, 1); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle&, bool) const override
		{
			const FVector2f S = FVector2f(G.GetLocalSize());
			// (the page starts 48 px under the notebook's top here, where the web version's section
			// starts at the top under the tabs: the rules stay where they were on the paper)
			for (float Y = 21 * PX; Y < S.Y; Y += 30 * PX)
				FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(S.X, 1.f * PX), FSlateLayoutTransform(FVector2f(0.f, Y))), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, C(0x506eaa, 0.22f));
			return Layer + 1;
		}
	};

	TSharedRef<SWidget> H4(const FString& S)
	{
		return SNew(SBox).Padding(FMargin(0, 0, 0, 14 * PX))[SNew(STextBlock).Text(FText::FromString(S)).Font(Type(18)).ColorAndOpacity(C(0x5a5446))];
	}
	TSharedRef<SWidget> Empty(const FString& S, const FLinearColor& Col = C(0x7d7563))
	{
		return SNew(STextBlock).Text(FText::FromString(S)).Font(Type(15)).ColorAndOpacity(Col).AutoWrapText(true);
	}
	// a picture by its data URL or its art name (core/art.ts), as an image `Width` wide
	UTexture2D* Picture(const FString& Src)
	{
		if (Src.IsEmpty()) return nullptr;
		return Src.StartsWith(TEXT("data:")) ? S47Image::FromDataUrl(Src) : S47Image::Art(Src);
	}
	// a card-like button with a paper fill
	TSharedRef<SButton> PaperButton(TSharedRef<SWidget> Content, const FLinearColor& Fill, const FLinearColor& Line, TFunction<void()> OnClick, const FMargin& Pad)
	{
		static TMap<FString, TSharedPtr<FButtonStyle>> Styles;
		const FString Key = Fill.ToString() + Line.ToString();
		TSharedPtr<FButtonStyle>& S = Styles.FindOrAdd(Key);
		if (!S.IsValid())
		{
			S = MakeShared<FButtonStyle>();
			const FSlateBrush* N = Box(Fill, Line, Line.A > 0.f ? 1.f : 0.f);
			const FSlateBrush* Hv = Box(Fill * FLinearColor(0.97f, 0.97f, 0.97f, 1.f), Line, Line.A > 0.f ? 1.f : 0.f);
			S->SetNormal(*N).SetHovered(*Hv).SetPressed(*Hv).SetDisabled(*N);
			S->SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
		}
		return SNew(SButton).ButtonStyle(S.Get()).ContentPadding(Pad).HAlign(HAlign_Fill)
			.OnClicked_Lambda([OnClick] { if (OnClick) OnClick(); return FReply::Handled(); })[Content];
	}

	// CONTROLS: every control, keyboard and mouse, and a phone or tablet (the hints about them are
	// said only once on a device, core/hints.ts, so they are all kept here)
	struct FCtlGroup { const TCHAR* Head; TArray<TPair<const TCHAR*, const TCHAR*>> Rows; };
	struct FCtlPage { const TCHAR* Title; TArray<FCtlGroup> Groups; };
	const FCtlPage& Controls(bool bPhone)
	{
		static const FCtlPage Desk = { TEXT("Keyboard and mouse"), {
			{ TEXT("On foot"), {
				{ TEXT("W A S D, arrows"), TEXT("Walk. Shift: run.") },
				{ TEXT("Mouse"), TEXT("Look. Click the picture first to take the mouse.") },
				{ TEXT("E, Enter, click"), TEXT("Use what the crosshair is on. Put a paper back.") },
				{ TEXT("Right mouse, held"), TEXT("A closer look at what the crosshair is on.") },
				{ TEXT("V"), TEXT("Read the current or last description. Press again to stop.") },
				{ TEXT("F"), TEXT("The flashlight, once you have it.") },
				{ TEXT("C"), TEXT("Raise or lower the field camera.") },
				{ TEXT("Space"), TEXT("The shutter, with the camera raised.") },
				{ TEXT("Tab, N"), TEXT("The notebook. M: its map. I: the kit. H: these controls.") },
				{ TEXT("Esc, P"), TEXT("Pause: settings, save and load.") } } },
			{ TEXT("In the truck"), {
				{ TEXT("W, S"), TEXT("Throttle and brake. S at a stop: reverse.") },
				{ TEXT("A, D"), TEXT("Steer.") },
				{ TEXT("Mouse"), TEXT("Look round the cab.") },
				{ TEXT("R, or click the radio"), TEXT("The radio: on, the next station, off.") },
				{ TEXT("E, stopped"), TEXT("Get out. When the engine has died: turn the key.") } } } } };
		static const FCtlPage Phone = { TEXT("Phone and tablet"), {
			{ TEXT("On foot"), {
				{ TEXT("Left thumb"), TEXT("Walk.") },
				{ TEXT("Right thumb"), TEXT("Look.") },
				{ TEXT("Tap, or Use"), TEXT("Use a thing. Tap a paper to put it back.") },
				{ TEXT("Zoom, held"), TEXT("A closer look.") },
				{ TEXT("Light"), TEXT("The flashlight, once you have it.") },
				{ TEXT("Camera"), TEXT("Raise or lower the field camera. Shutter: take the picture.") },
				{ TEXT("Notes"), TEXT("The notebook.") },
				{ TEXT("II"), TEXT("Pause: settings, save and load.") } } },
			{ TEXT("In the truck"), {
				{ TEXT("Left stick"), TEXT("Throttle, brake and steering.") },
				{ TEXT("Radio, or tap it"), TEXT("The radio: on, the next station, off.") },
				{ TEXT("Get out"), TEXT("Stopped: out of the truck. Turn the key: when the engine has died.") } } } } };
		return bPhone ? Phone : Desk;
	}

	TSharedRef<SWidget> ControlsBlock(const FCtlPage& P)
	{
		TSharedRef<SVerticalBox> B = SNew(SVerticalBox);
		B->AddSlot().AutoHeight()[H4(P.Title)];
		for (const FCtlGroup& G : P.Groups)
		{
			B->AddSlot().AutoHeight().Padding(0, 12 * PX, 0, 4 * PX)[SNew(STextBlock).Text(FText::FromString(G.Head)).Font(Term(19)).ColorAndOpacity(C(0xa33a2c))];
			for (const TPair<const TCHAR*, const TCHAR*>& R : G.Rows)
			{
				B->AddSlot().AutoHeight().Padding(0, 0, 0, 3 * PX)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(158 * PX)[SNew(STextBlock).Text(FText::FromString(R.Key)).Font(Term(18)).ColorAndOpacity(C(0x2e2a22))]]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(14 * PX, 0, 0, 0)[SNew(STextBlock).Text(FText::FromString(R.Value)).Font(Type(15)).ColorAndOpacity(C(0x4c463a)).AutoWrapText(true)]
				];
			}
		}
		return B;
	}

	/** Initials for an empty frame: first and last name, without a title or "the". */
	FString Initials(const FString& Name)
	{
		TArray<FString> W;
		Name.ParseIntoArrayWS(W);
		W.RemoveAll([](const FString& X) { const FString L = X.ToLower(); return L == TEXT("dr") || L == TEXT("dr.") || L == TEXT("the"); });
		FString S;
		if (W.Num()) S += W[0].Left(1);
		if (W.Num() > 1) S += W.Last().Left(1);
		return S.ToUpper();
	}

	// ui/kitArt.ts KIT_ART: the drawings beside the things in the Kit tab (Content/WebData/art)
	FString KitArt(const FString& Art)
	{
		return Art == TEXT("nickel") ? TEXT("kit_nickel_1947") : TEXT("kit_") + Art;
	}

	const float TILT[8] = { -1.4f, 0.9f, -0.5f, 1.3f, -1.f, 0.4f, 1.1f, -0.7f };

	// repeat(auto-fill, ...) keeps its empty tracks: a row of fewer things keeps the columns' width
	void KeepColumns(const TSharedRef<SUniformGridPanel>& Grid, int32 Count, int32 Cols)
	{
		for (int32 i = Count; i < Cols; ++i) Grid->AddSlot(i, 0)[SNew(SSpacer)];
	}
}

FString SS47Notebook::Excerpt(const FS47Doc& D)
{
	// one line of a finding: its page (P01 and the rest) or its transcript, without the
	// "P01  SUPPORTED / " that starts it, at most 150 characters
	static const FRegexPattern Pnn(TEXT("^p\\d\\d$"));
	FRegexMatcher IsP(Pnn, D.Id);
	FString One = IsP.FindNext() ? D.Page : D.Transcript;
	{
		static const FRegexPattern Lead(TEXT("^\\S*\\s*SUPPORTED / "));
		FRegexMatcher M(Lead, One);
		if (M.FindNext()) One = One.Mid(M.GetMatchEnding());
	}
	// .replace(/\s+/g, ' ')
	FString Flat;
	bool bSpace = false;
	for (const TCHAR Ch : One)
	{
		if (FChar::IsWhitespace(Ch)) { if (!bSpace) Flat.AppendChar(TEXT(' ')); bSpace = true; }
		else { Flat.AppendChar(Ch); bSpace = false; }
	}
	if (Flat.Len() <= 150) return Flat;
	FString Cut = Flat.Left(147);
	static const FRegexPattern Tail(TEXT("[\\s.,;:]+\\S*$"));
	FRegexMatcher M(Tail, Cut);
	if (M.FindNext()) Cut = Cut.Left(M.GetMatchBeginning());
	return Cut + TEXT("...");
}

void SS47Notebook::Construct(const FArguments& InArgs)
{
	UI = InArgs._UI;
	OnBoard = InArgs._OnBoard;
	const TArray<FS47Task>& Tasks = InArgs._Tasks;
	const TArray<FString>& Notes = InArgs._Notes;
	const TArray<FS47Doc>& Docs = InArgs._Docs;
	const FS47NbExtras& X = InArgs._Extras;
	TFunction<void(const FS47Doc&)> OnDoc = InArgs._OnDoc;
	static const FRegexPattern Pnn(TEXT("^p\\d\\d$"));
	auto IsFinding = [](const FS47Doc& D) { FRegexMatcher M(Pnn, D.Id); return M.FindNext() || D.Id == TEXT("finding"); };
	TArray<FS47Doc> Photos, Findings, Papers;
	for (const FS47Doc& D : Docs) if (D.Kind == TEXT("photo") && !D.Image.IsEmpty()) Photos.Add(D);
	for (const FS47Doc& D : Docs) if (IsFinding(D)) Findings.Add(D);
	for (const FS47Doc& D : Docs) if (!(D.Kind == TEXT("photo") && !D.Image.IsEmpty()) && !IsFinding(D)) Papers.Add(D);
	auto KindName = [](const FS47Doc& D) { return D.Kind == TEXT("hand") ? TEXT("Handwritten") : D.Kind == TEXT("printout") ? TEXT("Printout") : D.Kind == TEXT("typed") ? TEXT("Typed") : D.Kind == TEXT("map") ? TEXT("Map") : TEXT("Print"); };
	auto Label = [](const FS47Doc& D)
	{
		if (!D.Stamp.IsEmpty() && D.Stamp.Len() <= 6) return D.Stamp;
		TArray<FString> Parts = S47Text::Split(D.Title, TEXT(" / "));
		return Parts.Num() ? Parts[0] : D.Title;
	};
	auto Open = [OnDoc](const FS47Doc& D) { return [OnDoc, D] { if (OnDoc) OnDoc(D); }; };

	// ---------- the pages ----------
	TSharedRef<SVerticalBox> TasksP = SNew(SVerticalBox);
	TasksP->AddSlot().AutoHeight()[H4(InArgs._Heading.IsEmpty() ? TEXT("Tonight") : InArgs._Heading)];
	const float Li = UI.IsValid() && UI->bLargeText ? 30.f : 26.f, LiH = UI.IsValid() && UI->bLargeText ? 34.f : 30.f;
	if (Tasks.Num())
	{
		for (const FS47Task& T : Tasks)
		{
			// li.done: line-through (the text block keeps its own copy of the style)
			FTextBlockStyle St = FTextBlockStyle().SetFont(Hand(Li)).SetColorAndOpacity(T.bDone ? C(0x6d7895) : InkBlue());
			if (T.bDone)
			{
				FSlateBrush Strike = *FCoreStyle::Get().GetBrush("WhiteBrush");
				Strike.TintColor = C(0x6d7895);
				St.SetStrikeBrush(Strike);
			}
			TasksP->AddSlot().AutoHeight()
			[
				SNew(SBox).MinDesiredHeight(LiH * PX).VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(Li * PX * 1.1f)[SNew(STextBlock).Text(FText::FromString(T.bDone ? TEXT("x") : TEXT("-"))).Font(Hand(Li)).ColorAndOpacity(T.bDone ? C(0x6d7895) : InkBlue())]]
					+ SHorizontalBox::Slot().FillWidth(1.f)[SNew(STextBlock).Text(FText::FromString(T.Text)).TextStyle(&St).AutoWrapText(true)]
				]
			];
		}
	}
	else TasksP->AddSlot().AutoHeight()[Empty(TEXT("Nothing yet. Dale left the shift log on the desk."))];

	TSharedRef<SVerticalBox> NotesP = SNew(SVerticalBox);
	NotesP->AddSlot().AutoHeight()[H4(TEXT("Observations"))];
	if (Notes.Num())
	{
		// .nb-cols: two columns, the first filled first
		TSharedRef<SVerticalBox> L = SNew(SVerticalBox), R = SNew(SVerticalBox);
		const int32 Half = (Notes.Num() + 1) / 2;
		for (int32 i = 0; i < Notes.Num(); ++i)
		{
			(i < Half ? L : R)->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Notes[i])).Font(Hand(Li)).ColorAndOpacity(InkBlue()).AutoWrapText(true).LineHeightPercentage(0.9f)];
		}
		NotesP->AddSlot().AutoHeight()[S47Kit::Two(L, R)];
	}
	else NotesP->AddSlot().AutoHeight()[Empty(TEXT("Nothing worth writing down. Yet."))];

	TSharedRef<SVerticalBox> FindP = SNew(SVerticalBox);
	FindP->AddSlot().AutoHeight()[H4(TEXT("Findings"))];
	if (OnBoard)
	{
		FindP->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(0, 0, 0, 14 * PX)
		[
			BoxButton(TEXT("Lay it all out on the case board"), Term(20), C(0xa33a2c), C(0xa33a2c, 0.06f), C(0xa33a2c, 0.5f), [this] { Close(); if (OnBoard) OnBoard(); }, FMargin(12 * PX, 4 * PX))
		];
	}
	if (Findings.Num())
	{
		for (const FS47Doc& D : Findings)
		{
			TArray<FString> Parts = S47Text::Split(D.Title, TEXT(" / "));
			FString Rest = Parts.Num() > 1 ? FString::Join(TArray<FString>(Parts.GetData() + 1, Parts.Num() - 1), TEXT(" / ")) : FString();
			if (Rest.IsEmpty()) Rest = D.Title;
			FindP->AddSlot().AutoHeight().Padding(0, 0, 0, 10 * PX)
			[
				PaperButton(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
					[
						SNew(SBox).WidthOverride(56 * PX)[SNew(STextBlock).Text(FText::FromString(Label(D))).Font(Term(24)).ColorAndOpacity(C(0x2f6a3c))]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(12 * PX, 0, 0, 0)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Rest)).Font(Type(15)).ColorAndOpacity(Ink()).AutoWrapText(true)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2 * PX, 0, 0)[SNew(STextBlock).Text(FText::FromString(Excerpt(D))).Font(Type(13)).ColorAndOpacity(C(0x6d6656)).AutoWrapText(true)]
					],
					FLinearColor(1, 1, 1, 0.35f), FLinearColor::Transparent, Open(D), FMargin(10 * PX, 8 * PX))
			];
		}
	}
	else FindP->AddSlot().AutoHeight()[Empty(TEXT("Nothing recorded yet. A finding goes here once the records support it."))];

	TSharedRef<SVerticalBox> PapersP = SNew(SVerticalBox);
	PapersP->AddSlot().AutoHeight()[H4(TEXT("Papers"))];
	if (Papers.Num())
	{
		// .cards: repeat(auto-fill, minmax(190px, 1fr)), four on the notebook's width
		TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(5 * PX));
		for (int32 i = 0; i < Papers.Num(); ++i)
		{
			const FS47Doc& D = Papers[i];
			const bool bHand = D.Kind == TEXT("hand");
			Grid->AddSlot(i % 4, i / 4)
			[
				PaperButton(
					SNew(SBox).MinDesiredHeight(64 * PX - 20 * PX).VAlign(VAlign_Top)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(D.Title)).Font(bHand ? Hand(22) : Type(15)).ColorAndOpacity(bHand ? InkBlue() : Ink()).AutoWrapText(true)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 4 * PX, 0, 0)
						[
							SNew(STextBlock).Text(FText::FromString(FString(KindName(D)) + (D.Stamp.IsEmpty() ? FString() : TEXT(" / ") + D.Stamp))).Font(Term(16)).ColorAndOpacity(C(0x8a8270))
						]
					],
					D.Kind == TEXT("printout") ? C(0xeef2e6) : C(0xf4efe1), FLinearColor(0, 0, 0, 0.12f), Open(D), FMargin(12 * PX, 10 * PX))
			];
		}
		KeepColumns(Grid, Papers.Num(), 4);
		PapersP->AddSlot().AutoHeight()[Grid];
	}
	else PapersP->AddSlot().AutoHeight()[Empty(TEXT("None collected."))];

	// the album (ui/album.ts albumPage): the case prints under photo corners on black album paper;
	// the free frames (core/album.ts) are not in the Unreal version yet
	TSharedRef<SVerticalBox> AlbumIn = SNew(SVerticalBox);
	if (Photos.Num())
	{
		AlbumIn->AddSlot().AutoHeight().Padding(0, 8 * PX, 0, 12 * PX)[SNew(STextBlock).Text(FText::FromString(TEXT("The case"))).Font(Hand(28)).ColorAndOpacity(C(0xece6d6, 0.88f))];
		TSharedRef<SWrapBox> Thumbs = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(24 * PX, 26 * PX));
		for (int32 i = 0; i < Photos.Num(); ++i)
		{
			const FS47Doc& D = Photos[i];
			UTexture2D* Tex = Picture(D.Image);
			TSharedRef<SWidget> Print = SNew(SBox).WidthOverride(220 * PX)
			[
				SNew(SButton).ButtonStyle(Flat()).ContentPadding(0).OnClicked_Lambda([F = Open(D)] { F(); return FReply::Handled(); })
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[Tex ? S47Kit::Image(Tex, 220 * PX) : StaticCastSharedRef<SWidget>(SNew(SBox).HeightOverride(150 * PX))]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 10 * PX, 0, 0)[SNew(STextBlock).Text(FText::FromString(D.Title)).Font(Hand(21)).ColorAndOpacity(C(0xe7e1d1)).AutoWrapText(true)]
				]
			];
			Print->SetRenderTransform(FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(TILT[i % 8]))));
			Print->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Thumbs->AddSlot()[Print];
		}
		AlbumIn->AddSlot().AutoHeight().Padding(0, 0, 0, 26 * PX)[Thumbs];
	}
	else AlbumIn->AddSlot().AutoHeight()[Empty(TEXT("No exposures yet. Once you have the field camera, C raises it anywhere."), C(0xb9b29f))];
	TSharedRef<SWidget> AlbumP = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12 * PX)[SNew(STextBlock).Text(FText::FromString(TEXT("Album"))).Font(Type(18)).ColorAndOpacity(C(0x5a5446))]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder).BorderImage(Box(C(0x1b1916), FLinearColor::Transparent, 0.f, 3.f)).Padding(FMargin(20 * PX, 16 * PX, 20 * PX, 28 * PX))[AlbumIn]
		];

	// the map, the kit and the people (main.ts notebookExtras)
	TSharedPtr<SWidget> MapP, KitP, PeopleP;
	if (X.Map)
	{
		TSharedRef<SVerticalBox> M = SNew(SVerticalBox);
		M->AddSlot().AutoHeight()[SNew(SS47AreaMap).View(*X.Map).Width((NbW - 64) * PX)];
		if (!X.MapNote.IsEmpty())
		{
			TSharedRef<SWidget> Note = SNew(STextBlock).Text(FText::FromString(X.MapNote)).Font(Hand(26)).ColorAndOpacity(InkBlue()).AutoWrapText(true);
			Note->SetRenderTransform(FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(-0.6f))));
			M->AddSlot().AutoHeight().Padding(6 * PX, 8 * PX, 0, 0)[Note];
		}
		MapP = M;
	}
	if (X.Kit)
	{
		TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(11 * PX, 6 * PX));
		for (int32 i = 0; i < X.Kit->Num(); ++i)
		{
			const FS47KitItem& K = (*X.Kit)[i];
			TSharedRef<SVerticalBox> Words = SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(K.Name)).Font(Type(19)).ColorAndOpacity(C(0x2e2a22))];
			if (!K.State.IsEmpty()) Words->AddSlot().AutoHeight().Padding(0, 2 * PX, 0, 4 * PX)[SNew(STextBlock).Text(FText::FromString(K.State)).Font(Term(17)).ColorAndOpacity(C(0xa33a2c))];
			Words->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(K.Note)).Font(Type(15)).ColorAndOpacity(C(0x4c463a)).AutoWrapText(true)];
			TSharedRef<SHorizontalBox> Item = SNew(SHorizontalBox);
			if (UTexture2D* Art = K.Art.IsEmpty() ? nullptr : S47Image::Art(KitArt(K.Art)))
			{
				TSharedRef<SOverlay> Pic = SNew(SOverlay) + SOverlay::Slot()[S47Kit::Image(Art, 104 * PX)];
				if (!K.Tag.IsEmpty())
				{
					// the keys' leather tag is blank in the drawing; the tab writes SARO 07 on it
					TSharedRef<SWidget> TagW = SNew(STextBlock).Text(FText::FromString(K.Tag)).Font(Type(104 * 0.095f)).ColorAndOpacity(FLinearColor::FromSRGBColor(FColor(52, 26, 10)) * FLinearColor(1, 1, 1, 0.78f));
					TagW->SetRenderTransform(FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(58.f))));
					TagW->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
					Pic->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(0.8f * 104 * PX - 20 * PX, 0.62f * 104 * PX - 6 * PX, 0, 0)[TagW];
				}
				Item->AddSlot().AutoWidth().Padding(0, 0, 14 * PX, 0)[SNew(SBox).WidthOverride(104 * PX).HeightOverride(104 * PX)[Pic]];
			}
			Item->AddSlot().FillWidth(1.f)[Words];
			Grid->AddSlot(i % 3, i / 3)
			[
				SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBorder")).Padding(FMargin(0, 6 * PX, 0, 10 * PX))[Item]
			];
		}
		KeepColumns(Grid, X.Kit->Num(), 3);
		KitP = SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[H4(TEXT("What I have on me"))] + SVerticalBox::Slot().AutoHeight()[Grid];
	}
	if (X.People)
	{
		TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(11 * PX, 9 * PX));
		for (int32 i = 0; i < X.People->Num(); ++i)
		{
			const FS47PersonCard& P = (*X.People)[i];
			UTexture2D* Face = P.Portrait.IsEmpty() ? nullptr : S47Image::Art(P.Portrait);
			const float Fw = 190 * PX;
			TSharedRef<SWidget> Frame = Face ? StaticCastSharedRef<SWidget>(SNew(SBox).WidthOverride(Fw).HeightOverride(Fw * 5 / 4)[SNew(SImage).Image(S47Kit::BrushFor(Face).Get())])
				: StaticCastSharedRef<SWidget>(SNew(SBox).WidthOverride(Fw).HeightOverride(Fw * 5 / 4)
					[
						SNew(SBorder).BorderImage(Box(C(0xe9e2cf), C(0x3c3228, 0.25f), 1.f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
						[SNew(STextBlock).Text(FText::FromString(Initials(P.Name))).Font(Type(40)).ColorAndOpacity(C(0x6d6656))]
					]);
			Grid->AddSlot(i % 4, i / 4)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)[Frame]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 8 * PX, 0, 0)[SNew(STextBlock).Text(FText::FromString(P.Name)).Font(Type(18)).ColorAndOpacity(C(0x2e2a22))]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(P.Role)).Font(Term(16)).ColorAndOpacity(C(0xa33a2c))]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(P.Note)).Font(Type(14)).ColorAndOpacity(C(0x4c463a)).AutoWrapText(true)]
			];
		}
		KeepColumns(Grid, X.People->Num(), 4);
		PeopleP = SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[H4(TEXT("People in the case"))] + SVerticalBox::Slot().AutoHeight()[Grid];
	}
	// Controls: a phone's first on a phone
	TSharedRef<SWidget> CtlP = S47Kit::Two(ControlsBlock(Controls(X.bTouch)), ControlsBlock(Controls(!X.bTouch)));

	// ---------- the tabs ----------
	TSharedRef<SWrapBox> Tabs = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6 * PX, 0));
	if (OnBoard)
	{
		// .nb-boardtab: a brown tab with a red pin
		Tabs->AddSlot()
		[
			SNew(SButton).ButtonStyle(Flat()).ContentPadding(0).OnClicked_Lambda([this] { Close(); if (OnBoard) OnBoard(); return FReply::Handled(); })
			[
				SNew(SBorder).BorderImage(Box(C(0x8a5f3a), C(0x5a3c22), 1.f)).Padding(FMargin(9 * PX, 0, 12 * PX, 0))
				[
					SNew(SBox).MinDesiredHeight(40 * PX).VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 7 * PX, 0)
						[
							SNew(SBox).WidthOverride(10 * PX).HeightOverride(10 * PX)[SNew(SBorder).BorderImage(Box(C(0xc4241c), FLinearColor::Transparent, 0.f, 5 * PX))]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("Board"))).Font(Term(21)).ColorAndOpacity(C(0xf3e6cf))]
					]
				]
			]
		];
	}
	Tabs->AddSlot()[TabButton(TEXT("tasks"), TEXT("Tasks"), 0)];
	Tabs->AddSlot()[TabButton(TEXT("notes"), TEXT("Notes"), Notes.Num())];
	Tabs->AddSlot()[TabButton(TEXT("findings"), TEXT("Findings"), Findings.Num())];
	Tabs->AddSlot()[TabButton(TEXT("papers"), TEXT("Papers"), Papers.Num())];
	Tabs->AddSlot()[TabButton(TEXT("photos"), TEXT("Album"), Photos.Num())];
	if (MapP.IsValid()) Tabs->AddSlot()[TabButton(TEXT("map"), TEXT("Map"), 0)];
	if (KitP.IsValid()) Tabs->AddSlot()[TabButton(TEXT("kit"), TEXT("Kit"), 0)];
	if (PeopleP.IsValid()) Tabs->AddSlot()[TabButton(TEXT("people"), TEXT("People"), X.People->Num())];
	Tabs->AddSlot()[TabButton(TEXT("controls"), TEXT("Controls"), 0)];

	TSharedRef<SOverlay> Pages = SNew(SOverlay);
	Pages->AddSlot()[Page(TEXT("tasks"), true, TasksP)];
	Pages->AddSlot()[Page(TEXT("notes"), true, NotesP)];
	Pages->AddSlot()[Page(TEXT("findings"), true, FindP)];
	Pages->AddSlot()[Page(TEXT("papers"), false, PapersP)];
	Pages->AddSlot()[Page(TEXT("photos"), false, AlbumP)];
	if (MapP.IsValid()) Pages->AddSlot()[Page(TEXT("map"), true, MapP.ToSharedRef())];
	if (KitP.IsValid()) Pages->AddSlot()[Page(TEXT("kit"), true, KitP.ToSharedRef())];
	if (PeopleP.IsValid()) Pages->AddSlot()[Page(TEXT("people"), true, PeopleP.ToSharedRef())];
	Pages->AddSlot()[Page(TEXT("controls"), false, CtlP)];

	ChildSlot
	[
		// .overlay: a click on it closes the notebook
		SNew(SBorder).BorderImage(Box(C(0x020306, 0.72f))).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(12)
		.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent&) { Close(); return FReply::Handled(); })
		[
			SNew(SBox).WidthOverride(NbW * PX).MaxDesiredHeight_Lambda([this] { return FOptionalSize(FMath::Max(300.f, float(GetCachedGeometry().GetLocalSize().Y) - 24.f)); })
			[
				SNew(SBorder).BorderImage(Box(C(0xebe3cf), FLinearColor(0, 0, 0, 0.5f), 1.f, 4.f)).Padding(0)
				.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(22 * PX, 8 * PX, 14 * PX, 0)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 20 * PX, 0)[Tabs]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
						[
							SNew(SBox).MinDesiredHeight(40 * PX)
							[
								TextButton(TEXT("Close  [Tab]"), Term(22), C(0x5a5446), C(0x3f3a30), [this] { Close(); }, true, C(0x5a5446), false, FMargin(0))
							]
						]
					]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SNew(SScrollBox) + SScrollBox::Slot()[Pages]
					]
				]
			]
		]
	];
	// show(): the tab used last, if this notebook has it
	Show(Has.Contains(UI.IsValid() ? UI->NbTab : FString()) ? UI->NbTab : FString(TEXT("tasks")));
}

TSharedRef<SWidget> SS47Notebook::TabButton(const FString& T, const FString& LabelText, int32 Count)
{
	Has.Add(T);
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(FText::FromString(LabelText)).Font(Term(21))
			.ColorAndOpacity_Lambda([this, T] { return FSlateColor(Tab == T ? C(0x3f3a30) : C(0x7d7563)); })
		];
	// .n: the count in red
	if (Count) Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(5 * PX, 0, 0, 0)[SNew(STextBlock).Text(FText::AsNumber(Count)).Font(Term(16)).ColorAndOpacity(C(0xa33a2c))];
	return SNew(SButton).ButtonStyle(Flat()).ContentPadding(0).OnClicked_Lambda([this, T] { Show(T); return FReply::Handled(); })
		[
			// border-bottom: 2px, red under the open one
			SNew(SBorder).Padding(FMargin(10 * PX, 0, 10 * PX, 0))
			.BorderImage_Lambda([this, T] { return Tab == T ? Box(FLinearColor::Transparent, C(0xa33a2c), 0.f) : Box(FLinearColor::Transparent); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).MinDesiredHeight(38 * PX).VAlign(VAlign_Center)[Row]]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).HeightOverride(2 * PX)[SNew(SBorder).BorderImage_Lambda([this, T] { return Tab == T ? Box(C(0xa33a2c)) : Box(FLinearColor::Transparent); })]
				]
			]
		];
}

TSharedRef<SWidget> SS47Notebook::Page(const FString& T, bool bRuled, TSharedRef<SWidget> Body)
{
	Has.Add(T);
	TSharedRef<SOverlay> O = SNew(SOverlay);
	if (bRuled) O->AddSlot()[SNew(SRuled)];
	O->AddSlot().Padding(FMargin(34 * PX, 10 * PX, 30 * PX, 24 * PX))[SNew(SBox).MinDesiredHeight(340 * PX - 58 * PX)[Body]];
	return SNew(SBox).Visibility_Lambda([this, T] { return Tab == T ? EVisibility::Visible : EVisibility::Collapsed; })[O];
}

void SS47Notebook::Show(const FString& T)
{
	Tab = T;
	if (UI.IsValid())
	{
		UI->NbTab = T;
		if (UI->OnNotebookTab) UI->OnNotebookTab(T);
	}
}

void SS47Notebook::Close()
{
	if (UI.IsValid()) UI->Close();
}

FReply SS47Notebook::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	// main.ts input.onKey with the notebook open: Tab, N, M, I, H, Escape and P close it; B lays
	// the case out on the board
	const FKey K = E.GetKey();
	if (K == EKeys::Tab || K == EKeys::N || K == EKeys::M || K == EKeys::I || K == EKeys::H || K == EKeys::Escape || K == EKeys::P) { Close(); return FReply::Handled(); }
	if (K == EKeys::B && OnBoard) { TFunction<void()> Fn = OnBoard; Close(); Fn(); return FReply::Handled(); }
	return FReply::Unhandled();
}

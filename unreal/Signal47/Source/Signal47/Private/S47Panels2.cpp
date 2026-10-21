#include "S47Panels2.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "S47Image.h"
#include "S47Kit.h"
#include "S47UI.h"
#include "SS47Widgets.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	using S47Kit::FW;
	using S47Kit::Css;
	static constexpr float PX = 1.25f; // the CSS pixel, as S47Kit has it

	FSlateFontInfo Term(float Px) { return S47Style::Mono(Px * PX); }

	const FSlateBrush* Box(const FLinearColor& Fill, const FLinearColor& Line)
	{
		static TMap<FString, TSharedPtr<FSlateBrush>> Cache;
		TSharedPtr<FSlateBrush>& B = Cache.FindOrAdd(Fill.ToString() + Line.ToString());
		if (!B.IsValid()) B = MakeShared<FSlateRoundedBoxBrush>(Fill, 0.f, Line, 1.f);
		return B.Get();
	}

	// button { background: none; border: 0 }: the look is the SBorder inside
	const FButtonStyle* Bare()
	{
		static FButtonStyle S = []
		{
			FButtonStyle B;
			FSlateBrush None;
			None.DrawAs = ESlateBrushDrawType::NoDrawType;
			B.SetNormal(None).SetHovered(None).SetPressed(None).SetDisabled(None);
			B.SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
			return B;
		}();
		return &S;
	}

	struct FState
	{
		TWeakObjectPtr<US47UI> UI;
		S47Panels2::FArchive O;
		TWeakPtr<SBox> Body;
		FString Page;
		int32 Hint04 = 0, Hint05 = 0;
		TMap<FString, FString> Sel;   // destination, survey, marker
		FString Say;                  // the page's [data-say] line
		int32 Tone = 0;
	};
	using FStateRef = TSharedRef<FState>;

	void Render(const FStateRef& S, const FString& Page);

	// say(el, '[data-say]', r): a result is good or bad, a plain string neither
	void Say(const FStateRef& S, const FS47Result& R) { S->Say = R.Text; S->Tone = R.bOk ? 1 : -1; }
	void Say(const FStateRef& S, const FString& Text) { S->Say = Text; S->Tone = 0; }
	FW SayLine(const FStateRef& S)
	{
		return S47Kit::Text(TAttribute<FString>::CreateLambda([S] { return S->Say; }), TAttribute<int32>::CreateLambda([S] { return S->Tone; }));
	}

	// .lp-tab, [aria-selected=true]
	FW Tab(const FString& Label, bool bSelected, TFunction<void()> OnClick)
	{
		return SNew(SButton).ButtonStyle(Bare()).OnClicked_Lambda([OnClick] { OnClick(); return FReply::Handled(); })
			[
				SNew(SBorder)
				.BorderImage(bSelected ? Box(Css(0xc9d6c2), Css(0xc9d6c2)) : Box(FLinearColor::Transparent, Css(0xe9e2cf, 0.18f)))
				.Padding(FMargin(10 * PX, 6 * PX))
				[
					SNew(SBox).MinDesiredHeight(40 * PX - 12 * PX).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(Label)).Font(Term(19.f)).ColorAndOpacity(bSelected ? Css(0x12100e) : Css(0xcfc8b5))
					]
				]
			];
	}

	// .lp-choice with its icon, [aria-pressed=true]
	FW Choice(const FStateRef& S, const FString& Group, const FString& Key, const FString& Label, const FString& Icon)
	{
		auto Pressed = [S, Group, Key] { const FString* V = S->Sel.Find(Group); return V && *V == Key; };
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		if (!Icon.IsEmpty())
			Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 10 * PX, 0)[S47Kit::Image(S47Image::FromDataUrl(Icon), 40 * PX)];
		Row->AddSlot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(FText::FromString(Label)).Font(Term(19.f)).AutoWrapText(true)
			.ColorAndOpacity_Lambda([Pressed] { return FSlateColor(Pressed() ? Css(0x12100e) : Css(0xe9e2cf)); })
		];
		return SNew(SBox).Padding(FMargin(0, 6 * PX, 0, 0))
			[
				SNew(SButton).ButtonStyle(Bare())
				.OnClicked_Lambda([S, Group, Key] { S->Sel.Add(Group, Key); return FReply::Handled(); })
				[
					SNew(SBorder)
					.BorderImage_Lambda([Pressed] { return Pressed() ? Box(Css(0xc9d6c2), Css(0xc9d6c2)) : Box(FLinearColor::Transparent, Css(0xe9e2cf, 0.25f)); })
					.Padding(FMargin(10 * PX, 8 * PX))
					[
						SNew(SBox).MinDesiredHeight(48 * PX - 16 * PX).VAlign(VAlign_Center)[Row]
					]
				]
			];
	}

	// paper(text): the typed sheet, paragraphs on blank lines, line breaks kept
	FW Paper(const FString& Text) { return S47Kit::Paper(Text); }

	FW Source(const FStateRef& S, const FString& Page)
	{
		const S47Panels2::FArchiveSource* Src = S->O.Sources.Find(Page);
		if (!Src) return SNew(SBox);
		if (!Src->bHave) return S47Kit::Text(FString(TEXT("Not on the table yet. ")) + Src->Where);
		FW Figure = Src->Image.IsEmpty() ? FW(SNew(SBox)) : S47Kit::Image(S47Image::FromDataUrl(Src->Image), 480 * PX);
		return S47Kit::Col({
			S47Kit::Two(S47Kit::Col({ S47Kit::Question(Src->Heading), Paper(Src->Text) }), SNew(SBox).VAlign(VAlign_Top)[Figure]),
			S47Kit::Small(TEXT("Read in any order. Use COMPARE and DESTINATION to record findings.")),
		});
	}

	FW Compare(const FStateRef& S)
	{
		const S47Panels2::FArchive& O = S->O;
		if (O.P04 && O.P04()) return S47Kit::Col({ Paper(O.Supported04), S47Kit::Small(TEXT("P04 recorded. The sources stay available above.")) });
		const S47Panels2::FArchiveSource* Orig = O.Sources.Find(TEXT("original"));
		const S47Panels2::FArchiveSource* Amend = O.Sources.Find(TEXT("amended"));
		const bool bBoth = Orig && Orig->bHave && Amend && Amend->bHave;
		auto Pick = [S](const FString& C)
		{
			return [S, C]
			{
				const FS47Result R = S->O.OnCompare ? S->O.OnCompare(C) : FS47Result();
				if (R.bOk) { Render(S, TEXT("compare")); return; }
				Say(S, R);
			};
		};
		return S47Kit::Col({
			S47Kit::Two(
				S47Kit::Col({ S47Kit::Question(TEXT("ORIGINAL / E07")), Paper(TEXT("Three references: A, the fixed survey point; B, the optical comparison vane; C, the closing sight line.\n\nThe mark remained in the plate with the lamp circuit opened.")) }),
				S47Kit::Col({ S47Kit::Question(TEXT("AMENDED / E06")), Paper(TEXT("The incomplete closing sight line has been omitted.\n\nThe additional mark is attributed to a fault in plate development. No repeat observation is required.")) })),
			S47Kit::Question(bBoth ? TEXT("Which finding is supported by the source records?") : TEXT("Read E07 and E06 before recording a finding. The source tabs stay available above.")),
			S47Kit::Col({
				S47Kit::Button(TEXT("The signature proves that the author caused the anomaly."), Pick(TEXT("author-guilt"))),
				S47Kit::Button(TEXT("The amended copy removes C and replaces the retained-mark observation with a development explanation."), Pick(TEXT("omitted-c"))),
				S47Kit::Button(TEXT("The development explanation accounts for both versions without an omitted reference."), Pick(TEXT("development-only"))),
			}),
			S47Kit::Row({ S47Kit::Button(TEXT("Hint"), [S]
			{
				const TArray<FString>& H = S->O.Hints04;
				if (H.Num()) Say(S, H[FMath::Min(S->Hint04, H.Num() - 1)]);
				S->Hint04++;
			}) }),
			SayLine(S),
		});
	}

	FW Route(const FStateRef& S)
	{
		const S47Panels2::FArchive& O = S->O;
		if (O.P05 && O.P05()) return S47Kit::Col({ Paper(O.Supported05), S47Kit::Small(TEXT("P05 recorded. The sources stay available above.")) });
		const S47Panels2::FArchiveSource* Lineage = O.Sources.Find(TEXT("lineage"));
		const S47Panels2::FArchiveSource* Index = O.Sources.Find(TEXT("index"));
		const FString Lead = !(O.P04 && O.P04()) ? TEXT("First record the comparison in P04. You can inspect all four source cards now.")
			: !(Lineage && Lineage->bHave) || !(Index && Index->bHave) ? TEXT("Read the B-12 lineage card and E08 index before preparing the destination.")
			: TEXT("Match both source identifiers. Select a destination, survey ID and fixed-point mark.");
		auto Icon = [&O](const TCHAR* K) { const FString* I = O.Icons.Find(K); return I ? *I : FString(); };
		auto Column = [](const FString& Head, const TArray<FW>& Choices)
		{
			TArray<FW> All = { S47Kit::Small(Head) };
			All.Append(Choices);
			return S47Kit::Col(All, 0.f);
		};
		// .lp-cols3: three equal columns, 12 px apart
		TSharedRef<SHorizontalBox> Cols = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 8 * PX, 0)
			[
				Column(TEXT("FIELD DESTINATION"), { Choice(S, TEXT("destination"), TEXT("old-survey-station"), TEXT("OLD SURVEY STATION"), FString()),
					Choice(S, TEXT("destination"), TEXT("saro-apron"), TEXT("SARO ARRAY APRON"), FString()),
					Choice(S, TEXT("destination"), TEXT("unlisted"), TEXT("UNLISTED FIELD SITE"), FString()) })
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4 * PX, 0, 4 * PX, 0)
			[
				Column(TEXT("SURVEY IDENTIFIER"), { Choice(S, TEXT("survey"), TEXT("STATION 01"), TEXT("STATION 01"), FString()),
					Choice(S, TEXT("survey"), TEXT("S-03"), TEXT("S-03"), FString()),
					Choice(S, TEXT("survey"), TEXT("-39 LY AS A YEAR CODE"), TEXT("-39 LY AS A YEAR CODE"), FString()) })
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8 * PX, 0, 0, 0)
			[
				Column(TEXT("FIXED-POINT MARK"), { Choice(S, TEXT("marker"), TEXT("triangle-bar"), TEXT("OUTLINED TRIANGLE + BAR"), Icon(TEXT("triangle-bar"))),
					Choice(S, TEXT("marker"), TEXT("triangle"), TEXT("TRIANGLE / NO BAR"), Icon(TEXT("triangle"))),
					Choice(S, TEXT("marker"), TEXT("three-bars"), TEXT("THREE HORIZONTAL BARS"), Icon(TEXT("three-bars"))) })
			];
		auto Ready = [S] { return S->Sel.Contains(TEXT("destination")) && S->Sel.Contains(TEXT("survey")) && S->Sel.Contains(TEXT("marker")); };
		return S47Kit::Col({
			S47Kit::Question(Lead),
			SNew(SBox).Padding(FMargin(0, 8 * PX, 0, 10 * PX))[Cols],
			S47Kit::Small(TEXT("SOURCE CHECK / The maintenance card must connect today's B-12 to the same survey and fixed-point reference in the sleeve. A place name alone is insufficient.")),
			S47Kit::Row({
				S47Kit::Button(TEXT("RECORD SUPPORTED FIELD DESTINATION"), [S]
				{
					const FS47Result R = S->O.OnRoute ? S->O.OnRoute(S->Sel.FindRef(TEXT("destination")), S->Sel.FindRef(TEXT("survey")), S->Sel.FindRef(TEXT("marker"))) : FS47Result();
					if (R.bOk) { Render(S, TEXT("route")); return; }
					Say(S, R);
				}, true, TAttribute<bool>::CreateLambda(Ready)),
				S47Kit::Button(TEXT("Hint"), [S]
				{
					const TArray<FString>& H = S->O.Hints05;
					if (H.Num()) Say(S, H[FMath::Min(S->Hint05, H.Num() - 1)]);
					S->Hint05++;
				}),
			}),
			SayLine(S),
		});
	}

	void Render(const FStateRef& S, const FString& Page)
	{
		TSharedPtr<SBox> Body = S->Body.Pin();
		if (!Body.IsValid()) return;
		S->Page = Page;
		S->Say.Empty(); S->Tone = 0;
		static const TArray<TPair<FString, FString>> Tabs = {
			{ TEXT("original"), TEXT("E07 / ORIGINAL") }, { TEXT("amended"), TEXT("E06 / AMENDED") }, { TEXT("lineage"), TEXT("B-12 / LINEAGE") },
			{ TEXT("index"), TEXT("E08 / INDEX") }, { TEXT("compare"), TEXT("P04 / COMPARE") }, { TEXT("route"), TEXT("P05 / DESTINATION") } };
		// .lp-tabs: a wrapping row, 6 px apart
		TSharedRef<SWrapBox> TabRow = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6 * PX, 6 * PX));
		for (const TPair<FString, FString>& T : Tabs)
		{
			const FString K = T.Key;
			TabRow->AddSlot()[Tab(T.Value, K == Page, [S, K] { Render(S, K); })];
		}
		const FW PageW = Page == TEXT("compare") ? Compare(S) : Page == TEXT("route") ? Route(S) : Source(S, Page);
		Body->SetContent(S47Kit::Col({
			S47Kit::Sub(S->O.Status ? S->O.Status() : FString()),
			SNew(SBox).Padding(FMargin(0, 4 * PX, 0, 12 * PX))[TabRow],
			PageW,
		}, 0.f));
	}
}

void S47Panels2::Archive(US47UI* UI, const FArchive& O)
{
	FStateRef S = MakeShared<FState>();
	S->UI = UI;
	S->O = O;
	TSharedRef<SBox> Body = SNew(SBox);
	S->Body = Body;
	Render(S, O.Page);
	if (UI) UI->Open(S47Kit::Shell(UI, TEXT("SARO ARCHIVE / THE AMENDED RECORD"), Body));
}

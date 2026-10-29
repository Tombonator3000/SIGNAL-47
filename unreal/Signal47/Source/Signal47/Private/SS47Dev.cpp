#include "SS47Dev.h"
#include "S47Theme.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

void SS47Dev::Construct(const FArguments& InArgs)
{
	S47_USING_THEME
	Entries = InArgs._Entries;
	OnBack = InArgs._OnBack;
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	FString Group;
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		const FS47DevEntry& D = Entries[i];
		if (D.Group != Group)
		{
			Group = D.Group;
			List->AddSlot().AutoHeight().Padding(Pad(0, 14, 0, 4))
			[SNew(STextBlock).Text(FText::FromString(Group.ToUpper())).Font(Term(24, 2)).ColorAndOpacity(Amber(.9f))];
		}
		TSharedRef<SWidget> Row = SNew(SS47MenuItem).Label(D.Label).Px(26).Spacing(1).Pad(FMargin(12, 2, 14, 2)).bArrow(true)
			.Meta(D.Note)
			.Selected_Lambda([this, i] { return Sel == i; })
			.OnHover([this, i] { Sel = i; })
			.OnClick([this, i] { Sel = i; Choose(); });
		Rows.Add(Row);
		List->AddSlot().AutoHeight().Padding(0, 0, 0, 2 * U)[Row];
	}
	ChildSlot
	[
		SNew(SBorder).BorderImage(Box(Night(.94f))).Padding(Pad(96, 56, 96, 40))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("DEVELOPER / START POINTS"))).Font(Heading(54, 2)).ColorAndOpacity(Head())]
			+ SVerticalBox::Slot().AutoHeight().Padding(Pad(0, 6, 0, 10))
			[
				SNew(STextBlock).AutoWrapText(true).Font(Term(22)).ColorAndOpacity(Body())
				.Text(FText::FromString(TEXT("Each start runs in the developer case: your own cases are never saved over. A start point restarts the game at that place. ")
					TEXT("Press F9 anywhere in a night to keep a start point of your own; it is listed at the bottom. Escape: back to the title.")))
			]
			+ SVerticalBox::Slot().FillHeight(1)[SAssignNew(Scroll, SScrollBox) + SScrollBox::Slot()[List]]
		]
	];
}

void SS47Dev::Move(int32 Dir)
{
	if (Entries.IsEmpty()) return;
	Sel = (Sel + Dir + Entries.Num()) % Entries.Num();
	if (Scroll.IsValid() && Rows.IsValidIndex(Sel)) Scroll->ScrollDescendantIntoView(Rows[Sel], true, EDescendantScrollDestination::IntoView);
}

void SS47Dev::Choose()
{
	if (!Entries.IsValidIndex(Sel)) return;
	// (a copy: choosing closes this screen)
	const TFunction<void()> Fn = Entries[Sel].Fn;
	if (Fn) Fn();
}

FReply SS47Dev::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	const FKey K = E.GetKey();
	if (K == EKeys::Up || K == EKeys::W || K == EKeys::Gamepad_DPad_Up) { Move(-1); return FReply::Handled(); }
	if (K == EKeys::Down || K == EKeys::S || K == EKeys::Gamepad_DPad_Down) { Move(1); return FReply::Handled(); }
	if (K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom) { Choose(); return FReply::Handled(); }
	if (K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right) { if (OnBack) OnBack(); return FReply::Handled(); }
	return FReply::Unhandled();
}

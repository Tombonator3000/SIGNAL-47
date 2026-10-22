#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"
#include "Widgets/SCompoundWidget.h"

class US47UI;
struct FS47BoardImpl;

// ui/Board.ts BoardSpec. The chapters own the evidence and the rules; this widget only draws
// and listens. Layout positions stay in CSS/board pixels, just as they do in the web save.
struct FS47BoardSpec
{
	FString Title;
	TFunction<FString()> Status;
	FVector2D Size = FVector2D::ZeroVector;
	TFunction<TArray<FS47BoardRegion>()> Regions;
	FString Focus;
	TFunction<TArray<FS47BoardCard>()> Cards;
	TFunction<TArray<FS47BoardQuestion>()> Questions;
	TFunction<TArray<FS47BoardNote>()> Notes;
	TFunction<TArray<FS47BoardThread>()> Threads;
	TFunction<FS47Result(const FString&, const FString&)> Connect;
	TFunction<FS47Result(const FString&)> Record;
	TFunction<FString(const FString&)> Hint;
	TFunction<TOptional<FVector2D>(const FString&)> LayoutGet;
	TFunction<void(const FString&, double, double)> LayoutSet;
	TFunction<void()> OnClose;
};

// ui/Board.ts board(): the cork board, cards, pins and red yarn.
class SIGNAL47_API SS47Board : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SS47Board) : _UI(nullptr) {}
		SLATE_ARGUMENT(US47UI*, UI)
		SLATE_ARGUMENT(FS47BoardSpec, Spec)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SS47Board() override;
	virtual void Tick(const FGeometry& Geometry, const double CurrentTime, const float DeltaTime) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	void Render();
	FS47Result Link(const FString& A, const FString& B);
	void Say(const FString& Text);
	void Say(const FS47Result& Result);
	void Look(const FString& Id);
	bool Frame(const TArray<FString>& Ids);
	bool Moved() const;
	bool Show(const FString& Id);
	// The hint offered and shown, and a thread held from one pin over another (-S47BoardTest).
	void OfferHint(const FString& Question);
	void ShowHint(const FString& Question);
	void HoldThread(const FString& From, const FString& To);
	void ShowAll();
	void HideHow();
	// ui/Board.ts view(): scale and offset, for the same inspection tools as the web board.
	FVector View() const;
	static bool SelfTest(TArray<FString>& Lines);

private:
	TSharedPtr<FS47BoardImpl> P;
};

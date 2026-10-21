#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"
#include "S47Docs.h"
#include "S47Json.h"
#include "UObject/Object.h"
#include "S47CaseBoard.generated.h"

class US47Main;
class SS47Board;

// story/caseBoard.ts: the one board of the night; every chapter contributes its rule book.
UCLASS()
class SIGNAL47_API US47CaseBoard : public UObject
{
	GENERATED_BODY()

public:
	void Init(US47Main* InM);
	virtual void BeginDestroy() override;
	TSharedPtr<SS47Board> View;
	TFunction<void(TSharedRef<SS47Board>)> OnOpen;
	// CaseBoardHost.save: the save system connects this to its ordinary case save request.
	TFunction<void()> OnSave;

	struct FPin { FString Pin, Card; };
	TArray<FPin> Pins();
	bool Rests(const FString& Pin) const;
	void Open(const FString& Place = FString(), const FString& Region = FString());
	void Refresh();

	// BoardSave, nested under SavedCase.board, with positions in board pixels (not world space).
	S47J::FObj Save() const;
	void Load(const S47J::FObj& Saved);
	TOptional<FVector2D> GetPosition(const FString& Id) const;
	void SetPosition(const FString& Id, double X, double Y);

	static bool SelfTest(TArray<FString>& Lines);

private:
	UPROPERTY() TObjectPtr<US47Main> M;
	struct FHost
	{
		TFunction<TArray<FS47Doc>()> Docs;
		TFunction<TArray<FS47RuleBook*>()> Books;
		TFunction<FString()> Region;
		TFunction<void()> Save;
	} H;
	TMap<FString, FString> Seen, CardPins;
	TMap<FString, FVector2D> Pos;
	bool bLayoutMoved = false;
	TSharedPtr<class IInputProcessor> NotebookKeys;

	TArray<FS47RuleBook*> Books() const;
	TArray<FS47BoardCard> Cards();
	static FS47BoardCard DocCard(const FS47Doc& D, const FString& Region);
	static FString CardOf(const FString& Node, const TArray<FS47BoardCard>& Cards);
	FString Status() const;
	TArray<FS47BoardRegion> Regions(int32 Now) const;
	static FVector2D SizeAt(int32 Now);
	TArray<FS47BoardQuestion> Questions() const;
	TArray<FS47BoardNote> Notes() const;
	TArray<FS47BoardThread> Threads() const;
	FS47Result Connect(const FString& A, const FString& B);
	FS47Result Record(const FString& Q);
	FString Hint(const FString& Q);
};

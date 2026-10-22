#pragma once

#include "CoreMinimal.h"
#include "S47UI.h"

class FS47ReadAloud;

// core/Opening.ts: the opening card before a new night, a title-screen choice and not a running
// night. Begin always remains a real gesture. The narrator (the Serling bank, core/readAloud.ts) reads
// the five paragraphs while it is on; off, the state goes straight to 'ready'.
struct FS47OpeningChoice { int32 CaseId = 0; bool bReplace = false; };

struct FS47OpeningHooks
{
	TFunction<FS47OpeningView(const FS47OpeningActions&)> Show;
	TFunction<void()> Quiet;
	TFunction<void(const FS47OpeningChoice&)> Begin;
	TFunction<void()> Back;
	TFunction<void()> Dismissed;
};

class SIGNAL47_API FS47Opening
{
public:
	/** Original opening narration. Buttons and the heading are never in the speech queue. */
	static const TArray<FString>& Paragraphs();

	FS47Opening(TSharedPtr<FS47ReadAloud> InReader, FS47OpeningHooks InHooks) : Reader(MoveTemp(InReader)), Hooks(MoveTemp(InHooks)) {}

	bool Active() const { return Choice.IsSet(); }
	const FString& State() const { return CurrentState; }
	bool Open(const FS47OpeningChoice& C);
	/** No waiting here: startGame takes the night in this click. */
	bool Begin();
	bool Cancel();
	/** Another modal, a direct load or DEV abandons the card without replaying it. */
	void Dismiss();

private:
	int32 Generation = 0;
	TOptional<FS47OpeningChoice> Choice;
	TOptional<FS47OpeningView> View;
	FString CurrentState = TEXT("idle");
	TSharedPtr<FS47ReadAloud> Reader;
	FS47OpeningHooks Hooks;
	void Finish(bool bClose);
};

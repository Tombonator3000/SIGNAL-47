#pragma once

#include "CoreMinimal.h"
#include "S47UI.h"

// core/Opening.ts: the opening card before a new night, a title-screen choice and not a running
// night. Begin always remains a real gesture. The narration (the Serling narrator reading the five
// paragraphs, core/readAloud.ts) is not in the Unreal version yet, so the card is read, not heard:
// the state goes straight to 'ready', as the web version's does with the narrator off.
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

	explicit FS47Opening(FS47OpeningHooks InHooks) : Hooks(MoveTemp(InHooks)) {}

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
	FS47OpeningHooks Hooks;
	void Finish(bool bClose);
};

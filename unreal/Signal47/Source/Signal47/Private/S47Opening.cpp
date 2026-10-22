#include "S47Opening.h"

const TArray<FString>& FS47Opening::Paragraphs()
{
	// OPENING_PARAGRAPHS
	static const TArray<FString> P = {
		TEXT("New Mexico, 1986. Twenty-seven antennas listen to a sky that has never answered. On the night shift, silence is a measurement. It can be filed, signed, and forgotten."),
		TEXT("Tonight, the instruments will record something the sky has not sent. A telephone will ring in an empty room. A photograph will remember what no one saw."),
		TEXT("Your name is Reyes. Your shift ends at dawn. Between now and then, forty-seven seconds will refuse to belong to any clock."),
		TEXT("The record calls it interference."),
		TEXT("The record is incomplete."),
	};
	return P;
}

bool FS47Opening::Open(const FS47OpeningChoice& C)
{
	if (Active()) return false;
	++Generation;
	Choice = C;
	CurrentState = TEXT("ready");
	if (Hooks.Quiet) Hooks.Quiet();
	FS47OpeningActions A;
	A.Begin = [this] { Begin(); };
	A.Back = [this] { Cancel(); };
	A.Closed = [this] { Dismiss(); };
	if (Hooks.Show) View = Hooks.Show(A);
	// (this.reader.enabled is false until the narrator is ported: the view is ready at once)
	if (View && View->State) View->State(TEXT("ready"));
	return true;
}

bool FS47Opening::Begin()
{
	if (!Choice) return false;
	const FS47OpeningChoice C = *Choice;
	Finish(true);
	if (Hooks.Begin) Hooks.Begin(C);
	return true;
}

bool FS47Opening::Cancel()
{
	if (!Active()) return false;
	Finish(true);
	if (Hooks.Back) Hooks.Back();
	return true;
}

void FS47Opening::Dismiss()
{
	const bool bWas = Active();
	Finish(true);
	if (bWas && Hooks.Dismissed) Hooks.Dismissed();
}

void FS47Opening::Finish(bool bClose)
{
	if (!Active() && !View) return;
	++Generation;
	Choice.Reset();
	CurrentState = TEXT("idle");
	TOptional<FS47OpeningView> V = MoveTemp(View);
	View.Reset();
	// (this.reader.stop(): no narrator yet)
	if (bClose && V && V->Close) V->Close();
}

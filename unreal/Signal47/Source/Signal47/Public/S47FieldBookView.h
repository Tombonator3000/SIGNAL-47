#pragma once

#include "CoreMinimal.h"

class US47UI;
class US47Audio;

// web/src/ui/fieldBookView.ts. T. Vega's third field book and the envelope for plate 4
// (story/fieldBook.ts), each in the document view's own frame: the thing on the left, what it says on
// the right. The book is turned a page at a time (the buttons, the arrow keys, or a click on the
// page), so its last page comes by itself. The envelope is held up to the lamp with a button, and its
// picture changes. What is written on them is drawn in code (AGENTS.md: text that must be right).
// (The pages' paper is drawn here as the CSS draws it: the ruling, the two red margin lines, the
// coffee ring and the light on it. The read-aloud's boxes, ui.readingControls, are on the right: Read
// document for the book's typed page, Read description for the envelope.)
namespace S47FieldBookView
{
	struct FBookPage { FString Hand, Typed; bool bLast = false; };
	struct FBookOpts
	{
		FString Title;
		TArray<FBookPage> Pages;
		int32 Start = 0;
		/** hears each page turned to */
		TFunction<void(int32)> OnPage;
		/** the last page seen */
		TFunction<void(int32)> OnClose;
	};
	struct FBookView { TFunction<int32()> Page; TFunction<void(int32)> Go; };
	/** The field book, open at Start. */
	SIGNAL47_API FBookView Book(US47UI* UI, US47Audio* Audio, const FBookOpts& O);

	struct FEnvelopeOpts
	{
		FString Title;
		/** the two pictures, as data URLs */
		FString Front, Lit;
		FString Text, LitText;
		TFunction<void()> OnHold, OnClose;
	};
	struct FEnvelopeView { TFunction<void()> Hold; };
	/** The envelope on the office counter: its face, and a button to hold it up to the lamp. */
	SIGNAL47_API FEnvelopeView Envelope(US47UI* UI, US47Audio* Audio, const FEnvelopeOpts& O);

	/** The envelope lying on the office counter, seen from above in the office's light (a JPEG data URL). */
	SIGNAL47_API FString EnvelopeFront();
	/** The same envelope held up in front of the counter lamp: the paper lit through, the flaps behind
	 *  it, and the print inside as a shadow: the transit on its tripod, and three people by it. */
	SIGNAL47_API FString EnvelopeLit();

	/** The painter's pieces that need no screen (the colours, the gradients, the blur). */
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}

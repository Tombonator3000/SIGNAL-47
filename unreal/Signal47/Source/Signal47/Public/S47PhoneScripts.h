#pragma once

#include "CoreMinimal.h"
#include "S47Audio.h"

// core/phoneScripts.ts: the telephone calls as data. Exact authored speech; actor labels are
// captions, never spoken into the clone. A line is FS47PhoneCue (S47Audio.h: text, by, aside,
// pause), the shape audio.phoneCall takes.
namespace S47PhoneScripts
{
	// NORA_CB_TEXTS: these existing CB words share Nora's character voice and small channel preload.
	SIGNAL47_API const TArray<FString>& NoraCbTexts();

	// PHONE_CALLS: 'ward' | 'nora' | 'fieldNora'
	SIGNAL47_API const TMap<FString, TArray<FS47PhoneCue>>& PhoneCalls();
	SIGNAL47_API const TArray<FS47PhoneCue>& Ward();      // PHONE_CALLS.ward
	SIGNAL47_API const TArray<FS47PhoneCue>& Nora();      // PHONE_CALLS.nora
	SIGNAL47_API const TArray<FS47PhoneCue>& FieldNora(); // PHONE_CALLS.fieldNora

	// PHONE_TEXTS: the 24 telephone lines the web version had recorded in the Kinski clone's bank.
	// KINSKI-READ-OUT (Tom 9 November 2026): that bank is retired and none of them is Dale's, so they
	// are captions until they are recorded in the speakers' own voices; the list stays as the set to
	// record (literal source strings, so a generator can extract them without running game code).
	SIGNAL47_API const TArray<FString>& PhoneTexts();

	// story/Chapter2.ts WARD_ARRAY: Ward calls back once, a few seconds after her first call
	// (FORSLAG.md, 20 October, item 12): the array. These two lines have no recording, so they are
	// captions on the line, as every unrecorded call is (audio.phoneCall gives null).
	SIGNAL47_API const TArray<FS47PhoneCue>& WardArray();

	// checks against the TypeScript's own strings and counts
	bool SelfTest(TArray<FString>& Lines);
}

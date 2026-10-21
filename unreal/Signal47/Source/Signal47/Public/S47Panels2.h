#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"

class US47UI;

// ui/Panels.ts for chapter two. Today's chapter two lays its records out on the case board
// (story/caseBoard.ts, M->Board) and uses no panel of its own; Panels.ts keeps only its
// SourceKey. Until the board is in this version, the work table falls back to the panel the
// chapter had before the board: the archive table from Panels.ts as it stood before commit
// 46ad542 ("Evidence board for the archive table in chapter two"), ported here piece for piece,
// with Chapter2.ts's own compare() and route() behind its buttons. Chapter one's helper writes
// S47Panels.* at the same time; the two are to be merged into one later.
namespace S47Panels2
{
	// SourceKey: 'original' | 'amended' | 'lineage' | 'index'
	// ArchivePage: a SourceKey, or 'compare' | 'route'
	struct FArchiveSource { bool bHave = false; FString Heading, Text, Image, Where; };

	// archive(ui, o): one panel at the records room work table, after the Unity dossier
	// (WorldCase22): the four sources as tabs, then P04 (what changed) and P05 (where the
	// reference leads). A source that is still on the shelf says where it is instead.
	struct FArchive
	{
		FString Page;
		TMap<FString, FArchiveSource> Sources;
		TFunction<FString()> Status;
		TFunction<bool()> P04, P05;
		FString Supported04, Supported05;
		TFunction<FS47Result(const FString& Choice)> OnCompare;   // 'author-guilt' | 'omitted-c' | 'development-only'
		TFunction<FS47Result(const FString& Destination, const FString& Survey, const FString& Marker)> OnRoute;
		TArray<FString> Hints04, Hints05;
		TMap<FString, FString> Icons;                               // Marker -> a picture's data URL
	};
	SIGNAL47_API void Archive(US47UI* UI, const FArchive& O);
}

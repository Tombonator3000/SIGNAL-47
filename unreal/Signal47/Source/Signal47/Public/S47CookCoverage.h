#pragma once

#include "CoreMinimal.h"

// PACKAGE-GAS-TRIM-20261114: assets reached only by a path from a data table (LoadObject on a string) are cooked only when a cook
// rule covers them; nothing else references them. Two ways this went wrong before d70: the gas station's trim meshes
// (/Game/Free/Town/gas_station/Trim, named by Content/Free/free_models.json) were deleted by a model re-import and still asked for,
// and the shop's look material (/Game/Signal47/Look, named by Config/Props/gas_look.json) was on disk with no cook rule.
//
// The check, pure and the same in the editor and the package: every /Game path a table names (Config/Props/*.json, the free models'
// trims, the town's patch tables) lies under a +DirectoriesToAlwaysCook directory of Config/DefaultGame.ini, and every path that is
// not load-if-present (a trim a prop asks for, a patch table's mesh) exists as a package (on disk in the editor, in the container in
// a package).
namespace S47CookCoverage
{
	/** The +DirectoriesToAlwaysCook paths of Config/DefaultGame.ini ("/Game/Free", ...); empty when the file is not here. */
	TArray<FString> CookRules();
	/** The package of a /Game object path lies under one of the rules. */
	bool Covered(const FString& AssetPath, const TArray<FString>& Rules);
	bool SelfTest(TArray<FString>& Lines);
}

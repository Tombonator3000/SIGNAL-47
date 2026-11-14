#pragma once

#include "CoreMinimal.h"

// PACKAGE-GAS-TRIM-20261114: assets reached only by a path from a data table (LoadObject on a string) are cooked only when a cook
// rule covers them; nothing else references them. Two ways this went wrong before d70: the gas station's trim meshes
// (/Game/Free/Town/gas_station/Trim, named by Content/Free/free_models.json) were deleted by a model re-import and still asked for,
// and the shop's look material (/Game/Signal47/Look, named by Config/Props/gas_look.json) was on disk with no cook rule.
//
// The check, pure and the same in the editor and the package: every /Game path a table names (Config/Props/*.json, the free models'
// trims, the town's patch tables) lies under a +DirectoriesToAlwaysCook directory of Config/DefaultGame.ini, and every path that is
// not load-if-present (a patch table's mesh, among them the composed patches that cut the old gas station out of the town) exists as
// a package (on disk in the editor, in the container in a package). A trim a prop asks for: its hidden rows need no mesh; a cut row
// needs its mesh, or (the gas station's, since the 13 November re-import deleted them) a composed town patch that takes out at least
// as much of the station. A file the check needs that cannot be read FAILS in the editor; in a package (where Config/DefaultGame.ini
// and the tables are not proven readable) that part is not run, said in an INFO line, never counted as a pass.
namespace S47CookCoverage
{
	/** The +DirectoriesToAlwaysCook paths of Config/DefaultGame.ini ("/Game/Free", ...); empty when the file is not here. */
	TArray<FString> CookRules();
	/** The package of a /Game object path lies under one of the rules. */
	bool Covered(const FString& AssetPath, const TArray<FString>& Rules);
	bool SelfTest(TArray<FString>& Lines);
}

#pragma once

#include "CoreMinimal.h"

class US47Main;

// The free models imported by Tools/freemodels/import_free.py (/Game/Free, Content/Free/free_models.json).
// Unreal only. Nothing places them in the world yet; this is the test that looks at them.
namespace S47FreeModels
{
	/** -S47FreeModels[=name,name,...]: 1.5 s after the world is up, the models (by the manifest's name or
	 *  asset name; a few of each kind by default) stand in a row in front of the player, on the ground,
	 *  with the gap between them their own size; the log says what stood where and how big. */
	SIGNAL47_API void Test(US47Main* M);
}

#pragma once

#include "CoreMinimal.h"

class AS47Web;

// One fixed Operations rear window. Does not change navigation, state or doors.
namespace S47OfficeWindow
{
 // Call only after the updated Office Room/Dressing import and normal portal
 // build succeed. Both original rear-wall meshes (g313 and g342) must retain
 // their imported CPU buffers. Audit covers this rear cut and passive pane;
 // the authored jambs/blind and actual daylight still need native visual QA.
 bool Build(AS47Web* World);
 bool Audit(AS47Web* World, TArray<FString>& Lines);
}

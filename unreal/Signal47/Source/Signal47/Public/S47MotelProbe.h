#pragma once
#include "CoreMinimal.h"
class US47Main;
// Opt-in read-only diagnostics for the motel office (PLAY-31-01) and the pool lump (FIX-04).
// Driven by -S47MenuPointerOffice in S47MenuPointerTest. Nothing here changes the world.
namespace S47MotelProbe
{
	/** Visible office components and their materials (two-sided or not), the old native parts' visibility, and a winding-versus-normal audit of every LOD. */
	void LogOffice(US47Main* Main);
	/** Complex rays from the player's eye (forward, a horizontal fan, up, down): what is hit first, and whether it is drawn. */
	void LogRays(US47Main* Main, const TCHAR* Name);
	/** Every visible mesh and instance with bounds around the pool fence, smallest first. */
	void LogPool(US47Main* Main);
}

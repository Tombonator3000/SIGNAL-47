#pragma once
#include "CoreMinimal.h"

class AS47Web;
class FS47Fleet;
struct FS47Truck;

/** Tom's existing Tripo art; the original transport/story controllers remain authoritative. */
class SIGNAL47_API FS47TripoFleet
{
public:
 static TUniquePtr<FS47Fleet> InstallJeep(AS47Web* Web,FS47Truck& Truck);
 static void InstallWorld(AS47Web* Web);
};

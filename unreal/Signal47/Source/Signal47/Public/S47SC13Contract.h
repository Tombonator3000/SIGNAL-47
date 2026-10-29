#pragma once
#include "CoreMinimal.h"
class US47Main;
namespace S47SC13Contract
{
 // Explicit -S47SpineContract=sc13 only. Dispatch also observes the real ending/title timers.
 SIGNAL47_API void Step(US47Main* Main,double Dt);
}

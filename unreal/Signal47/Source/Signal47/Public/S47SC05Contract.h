#pragma once
#include "CoreMinimal.h"
class US47Main;
namespace S47SC05Contract
{
 // Opt-in actual SC04 checkpoint consumer; never manufactures earlier scenario facts.
 void Step(US47Main* Main,double Dt);
}

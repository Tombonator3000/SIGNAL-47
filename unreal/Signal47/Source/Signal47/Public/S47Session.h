#pragma once
#include "CoreMinimal.h"
// Session identity is independent of title/play/end presentation. Missing saves mean Story.
enum class ES47SessionMode : uint8 { Story, Freeform };

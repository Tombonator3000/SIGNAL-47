#pragma once
#include "CoreMinimal.h"
class AS47Web;
// Fixed runtime cameras and frame samples. Only active with -S47GraphicsReview=<directory>.
namespace S47GraphicsReview { void Tick(AS47Web* Web, float Dt); }

#pragma once
#include "CoreMinimal.h"
class AS47Web;
// Fixed runtime cameras and raw wall-frame samples. Only active with -S47GraphicsReview=<directory>.
// -S47GraphicsViews=control,annex selects a short subset; the Python runner splits full tours.
namespace S47GraphicsReview { void Tick(AS47Web* Web, float Dt); }

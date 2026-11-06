#pragma once
#include "CoreMinimal.h"
class AS47Web;
// Fixed runtime cameras and raw wall-frame samples. Only active with -S47GraphicsReview=<directory>.
// -S47GraphicsViews=control,annex selects a short subset; the Python runner splits full tours.
namespace S47GraphicsReview { void Tick(AS47Web* Web, float Dt); }
// Frame cost before, during and after one settings change. Only active with -S47GraphicsTouch=<directory>.
namespace S47GraphicsTouch { void Tick(AS47Web* Web); }
// Always on, one line per episode: a sustained slide show in play, with what it needs to be understood.
namespace S47SlowFrames { void Tick(AS47Web* Web); }

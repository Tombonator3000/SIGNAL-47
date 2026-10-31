#pragma once
#include "CoreMinimal.h"
#include "S47RoswellMotion.h"
#include "S47WebAudio.h"

class US47Audio;

struct FS47RoswellFoley
{
 bool bRoof=false, bMetal=false, bOccluded=false, bHeld=false;
 double Crouch=0;
};

// Contact-driven, small-body Foley. Occlusion must come from the world's
// actual listener-to-contact trace, including the roof, rather than distance.
class FS47RoswellAudio
{
public:
 void Step(US47Audio* Audio,const FS47RoswellContacts& Contacts,const FS47RoswellFoley& Surface);
 void Stop();
 void Reset();
 int32 Played() const { return PlayedCount; }
 // A separate context: never advances or records the running game's graph.
 static bool RenderTour(const FString& Path);
private:
 TArray<TSharedPtr<FS47AudioBuffer>> Bank;
 TArray<TSharedPtr<FS47BufferSourceNode>> Sources;
 int32 PlayedCount=0;
 uint32 Pick=0;
 int32 Last=-1;
};

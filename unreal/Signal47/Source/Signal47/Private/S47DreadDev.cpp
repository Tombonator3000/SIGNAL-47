#include "S47DevFreeform.h"
#include "S47Audio.h"
#include "S47DevEvents.h"
#include "S47EncounterSound.h"
#include "S47FieldRadio.h"
#include "S47Main.h"

// F10 "Dread sound (T)" (DREAD-20261110, Tom: Horror Ambience as the ground under a scary meeting, Horror Screamers cut in the
// moment something is seen). Each line plays one part by itself, whatever was heard last, so that the levels in
// S47EncounterSound.cpp (namespace Dread) can be set by ear. The sounds are the private clone's (lic:dread_*, lic:scare_*,
// lic:sky_*): a line says so when its file is not there. Nothing here is saved and nothing in the night is moved.
void FS47DevFreeform::AddDread(FS47DevEvents& E)
{
 static const TCHAR* const Group=TEXT("Dread sound (T)");
 const auto Bed=[this](double Low,double High,const TCHAR* What)
 {
  if(!M->FieldRadio||!M->Audio){Tell(false,TEXT("no sound yet"));return;}
  const bool bHave=M->Audio->Lic(TEXT("dread_low")).IsValid()&&(High<=0||M->Audio->Lic(TEXT("dread_high")).IsValid());
  if(bHave)M->FieldRadio->DreadAudition(Low,High,12);
  Tell(bHave,bHave?FString::Printf(TEXT("%s, 12 seconds, then it sinks away"),What):FString(TEXT("the bought ground is not in this build (Content/Licensed)")));
 };
 const auto Hit=[this](int32 Kind,const TCHAR* What)
 {
  const bool bOk=M->FieldRadio&&M->FieldRadio->Scare(Kind,1.,true);
  Tell(bOk,bOk?FString(What):FString(TEXT("that bought hit is not in this build (Content/Licensed)")));
 };
 E.AddFreeform(Group,TEXT("Dread: the ground alone"),TEXT("the low loop that comes up when something is near and unseen"),[Bed]{Bed(1,0,TEXT("the ground alone"));});
 E.AddFreeform(Group,TEXT("Dread: the ground with the layer over it"),TEXT("as when it is seen close, or hunts"),[Bed]{Bed(1,1,TEXT("the ground and the layer over it"));});
 E.AddFreeform(Group,TEXT("Dread: hit, something seen"),TEXT("the softer hit at a first sight; two recordings in turn"),[Hit]{Hit(0,TEXT("the hit at a sight"));});
 E.AddFreeform(Group,TEXT("Dread: hit, hunted"),TEXT("the hard hit when one comes for you; three recordings in turn"),[Hit]{Hit(1,TEXT("the hit at a hunt"));});
 E.AddFreeform(Group,TEXT("Dread: a light near"),TEXT("the braam when a landing or the big night is first seen close"),[Hit]{Hit(2,TEXT("the braam for a light near"));});
 E.AddFreeform(Group,TEXT("Dread: a light far off"),TEXT("the swell when a light or a figure is first seen at a distance"),[Hit]{Hit(3,TEXT("the swell for a light far off"));});
}

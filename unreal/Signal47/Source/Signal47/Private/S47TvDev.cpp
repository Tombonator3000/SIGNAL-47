#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47Main.h"
#include "S47Office.h"
#include "S47OfficeTv.h"
#include "S47TvProgramme.h"

// F10 "Television (T)" (TV-CHANNEL-20261110, Tom: every set on the one channel, the flag and the anthem at the end of the
// broadcast day, static and flicker with odd sounds when something is near, and a switch on Dale's set). The station's own
// sign-off comes at two at night and only in front of whoever is awake for it; these lines bring each part at any hour. Nothing
// here is saved except the switch, which is the office's own.
void FS47DevFreeform::AddTv(FS47DevEvents& E)
{
 static const TCHAR* const Group=TEXT("Television (T)");
 E.AddFreeform(Group,TEXT("TV: the station signs off now"),TEXT("the flag and the anthem on every set that is on, a minute of it, then the hour's own picture"),[this]
 {
  FS47TvProgramme::SignOffNow();
  Tell(true,TEXT("the station signs off: the flag and the anthem on Dale's set and the motel office's, about a minute"),8);
 });
 const auto Jam=[this](double Level,const TCHAR* What)
 {
  FS47TvProgramme::JamFor(Level,25);
  const FS47OfficeTv* Set=M->Office?M->Office->Television():nullptr;
  Tell(true,FString::Printf(TEXT("%s, 25 seconds%s"),What,Set&&!Set->Switched()?TEXT(" (Dale's set is switched off)"):TEXT("")),8);
 };
 E.AddFreeform(Group,TEXT("TV: something on the air, strong"),TEXT("the sets as they are with a light or a visitor close: rolling, tearing, snow, the tone sliding about"),[Jam]{Jam(.9,TEXT("something strong on the air near the sets"));});
 E.AddFreeform(Group,TEXT("TV: something on the air, a little"),TEXT("the sets as they are with something far off: a tear now and then, a little snow"),[Jam]{Jam(.3,TEXT("a little on the air near the sets"));});
 E.AddFreeform(Group,TEXT("TV: Dale's set, the switch"),TEXT("off to a dot, on from a line (the player's own Use on the set in Operations)"),[this]
 {
  FS47OfficeTv* Set=M->Office?M->Office->Television():nullptr;
  if(Set)Set->Switch();
  Tell(Set!=nullptr,Set?FString::Printf(TEXT("Dale's set switched %s"),Set->Switched()?TEXT("on"):TEXT("off")):FString(TEXT("Dale's set is not built here")),6);
 });
}

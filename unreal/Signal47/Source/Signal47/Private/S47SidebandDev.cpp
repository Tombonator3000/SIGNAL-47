#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47Main.h"
#include "S47Score.h"
#include "S47Sideband.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"

// F10 "Sideband (music)" (SIDEBAND-20261113): each of the four bands held at a value against whatever the score plays, a flare of
// each tier whatever the rules say, the prototypes' 80 s night scene, and the music bus kept as a WAVE file (Saved/Sideband).
// Nothing here is saved and nothing in the night is moved. A band lets go by itself after twenty seconds.
void FS47DevFreeform::AddSideband(FS47DevEvents& E)
{
 static const TCHAR* const Group=TEXT("Sideband (music)");
 static const TCHAR* const Bands[4]={TEXT("WORN"),TEXT("BROKEN"),TEXT("WIDE"),TEXT("FAR")};
 static const TCHAR* const Notes[4]={TEXT("late, tired, something is off: wow, tape, hiss"),TEXT("it is close, the gear is failing: the gentle one"),
  TEXT("open land and wind: a drifting halo"),TEXT("after: the music far off in a long room")};
 const auto Have=[this]()->FS47Sideband*
 {
  if(!M||!M->Sideband){Tell(false,TEXT("SIDEBAND is not in this run (-S47NoSideband, or no sound yet)"));return nullptr;}
  return M->Sideband.Get();
 };
 for(int32 B=0;B<4;++B)for(const double V:{.3,.6,1.})
 {
  E.AddFreeform(Group,FString::Printf(TEXT("Sideband: %s %.1f"),Bands[B],V),Notes[B],[this,Have,B,V]
  {
   FS47Sideband* S=Have();if(!S)return;
   S->Audition(B,V,20);
   const bool bMusic=M->Score&&!M->Score->Playing.IsEmpty();
   Tell(true,FString::Printf(TEXT("%s at %.1f for twenty seconds%s"),Bands[B],V,bMusic?TEXT(""):TEXT(" (no piece is playing just now: wait for the score)")));
  });
 }
 E.AddFreeform(Group,TEXT("Sideband: let the bands go"),TEXT("back to what the night asks for"),[this,Have]
 {
  FS47Sideband* S=Have();if(!S)return;
  for(int32 B=0;B<4;++B)S->Audition(B,-1);
  Tell(true,TEXT("the bands follow the night again"));
 });
 const auto Flare=[this,Have](S47Sideband::ETier Tier)
 {
  FS47Sideband* S=Have();if(!S)return;
  FString What;const bool bOk=S->AuditionFlare(Tier,What);
  Tell(bOk,bOk?FString::Printf(TEXT("%s flare: %s"),S47Sideband::TierName(Tier),*What):What);
 };
 E.AddFreeform(Group,TEXT("Sideband: flare, hint"),TEXT("one low note far under the music (18 to 24); the two in turn"),[Flare]{Flare(S47Sideband::ETier::Hint);});
 E.AddFreeform(Group,TEXT("Sideband: flare, glimpse"),TEXT("one far, soft stinger in the music's own room (00 to 02); the three in turn"),[Flare]{Flare(S47Sideband::ETier::Glimpse);});
 E.AddFreeform(Group,TEXT("Sideband: flare, full"),TEXT("stinger and hit, the music 36 dB down for three seconds, then far; the four in turn"),[Flare]{Flare(S47Sideband::ETier::Full);});
 const auto Record=[this](FS47Sideband* S,double Secs)
 {
  const FString Path=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Sideband")/FString::Printf(TEXT("sideband_%s.wav"),*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))));
  return S->Record(Secs,Path)?Path:FString();
 };
 E.AddFreeform(Group,TEXT("Sideband: the night scene, 80 s"),TEXT("the desk, the walk out, the sight at 46 s, after: against whatever plays"),[this,Have]
 {
  FS47Sideband* S=Have();if(!S)return;
  S->StartScene();
  Tell(true,TEXT("the 80 s scene: desk to 20 s, the walk out, seen at 46 s, far from 49 s"),10);
 });
 E.AddFreeform(Group,TEXT("Sideband: the night scene, recorded"),TEXT("the same, and the music bus kept as a WAVE file in Saved/Sideband"),[this,Have,Record]
 {
  FS47Sideband* S=Have();if(!S)return;
  const FString Path=Record(S,84);
  if(Path.IsEmpty()){Tell(false,TEXT("a recording is already running"));return;}
  S->StartScene();
  Tell(true,FString::Printf(TEXT("the 80 s scene, recording to %s"),*Path),10);
 });
 E.AddFreeform(Group,TEXT("Sideband: record the music bus, 90 s"),TEXT("what SIDEBAND sends on (score, colours, flares) as a WAVE file in Saved/Sideband"),[this,Have,Record]
 {
  FS47Sideband* S=Have();if(!S)return;
  const FString Path=Record(S,90);
  Tell(!Path.IsEmpty(),Path.IsEmpty()?FString(TEXT("a recording is already running")):FString::Printf(TEXT("recording 90 s to %s"),*Path),10);
 });
 E.AddFreeform(Group,TEXT("Sideband: what it is doing"),TEXT("the four bands, the tension, the event and whose ground it is"),[this,Have]
 {
  FS47Sideband* S=Have();if(!S)return;
  Tell(true,S->Describe(),12);
 });
}

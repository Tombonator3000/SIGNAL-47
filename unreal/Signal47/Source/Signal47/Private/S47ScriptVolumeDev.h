#pragma once
#include "CoreMinimal.h"
class US47Main;

// DIRECTOR-MODE-01 (slice 3): the trigger volumes' in-game check (S47Script.h: Inside, ModeHolds; FS47ScriptPlayer::NowFor). It asks
// S47Script::Ready with the real session's Now about triggers it builds around where the player stands and looks; it plays nothing,
// previews nothing and changes nothing in the save.
namespace S47ScriptVolumeDev
{
 // The checks; empty when every one holds, otherwise the first that does not.
 FString Check(US47Main* M);
 // The F10 group "Director volume" (only in a developer's run, as the script player's own lines): runs Check and says the result.
 // Added once; the menu outlives a session.
 void AddDevLine(US47Main* M);
 // -S47DevEventsContract=/tmp/dir -S47DevEventsGroup="Director volume" (FS47DevEvents::ControlTest): waits for the night to settle,
 // runs Check, logs "S47: director volume contract COMPLETE PASS" or "... FAIL <what>" and exits 0 or 1. True once it is over.
 bool Contract(US47Main* M,double Dt);
}

#pragma once
#include "CoreMinimal.h"

class US47Main;

// Tom's test of d36 at SARO (9 November 2026, TODO "Toms test av d36"): PT-RAMP, PT-PATHS, PT-STOP, PT-HALL, PT-DALE-LIGHT,
// PT-RECORDS and PT-SWITCH. Each part is built once SARO's web scene is there and retried until what it needs is ready; a part
// whose source differs from what it was measured on leaves the old one as it was and says so in the log (LogS47SaroFixes).
// -S47SaroFixesBefore leaves everything as it was before, for the before pictures of F10 "SARO fixes (T)".
namespace S47SaroFixes
{
 bool Before();
 void Update(US47Main* M, double Dt);
 // One line per part: built, waiting (and why) or off.
 void Report(US47Main* M, TArray<FString>& Lines);
}

// PT-RECORDS: the records room's realism round (shelves of binders and boxes, filing cabinets, cartons, dust, a desk with a lamp,
// 1986 labels), from existing interior models. S47RecordsDressing.cpp.
namespace S47RecordsDressing
{
 void Update(US47Main* M, double Dt);
 void Report(US47Main* M, TArray<FString>& Lines);
}

// RECORDS-ROOM-LIFT-20261113: the records room's realism lift after Tom's reference sheet (block walls and vinyl tile laid on the
// export's own walls and floor, a hung ceiling with two troffers, the lights in their new fittings, the hero props), every part
// load-if-present with the old look as its fallback. S47RecordsRoom.cpp; -S47RecordsRoomOff leaves the room as before.
namespace S47RecordsRoom
{
 void Update(US47Main* M, double Dt);
 void Report(US47Main* M, TArray<FString>& Lines);
 bool SelfTest(TArray<FString>& Lines);
}

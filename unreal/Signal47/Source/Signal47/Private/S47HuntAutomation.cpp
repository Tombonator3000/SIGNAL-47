#include "S47HorrorThreat.h"
#include "S47SC08.h"
#include "S47SC11.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
// Runs production state machines without loading the world or touching player saves.
// Physical sweeps and perceived encounter pacing still need the native scenarios.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FS47HuntAutomation, "S47.Gameplay.Hunt",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FS47HuntAutomation::RunTest(const FString& Parameters)
{
 TArray<FString> Lines;
 const bool Threat = FS47HorrorThreat::SelfTest(Lines);
 const bool Array = FS47SC08::SelfTest(Lines);
 const bool Saro = FS47SC11::SelfTest(Lines);
 for (const FString& Line : Lines)
 {
  if (Line.Contains(TEXT("FAIL"))) AddError(Line);
  else AddInfo(Line);
 }
 return Threat && Array && Saro;
}
#endif

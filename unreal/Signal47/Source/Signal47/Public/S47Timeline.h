#pragma once
#include "CoreMinimal.h"

// One signed night axis: Sunday evening is negative, Monday begins at zero.
// Legacy cases retain their original event clock and document stamps.
namespace S47Timeline
{
 inline constexpr int32 LegacyVersion = 1, EveningVersion = 2;
 inline constexpr double ShiftStart = -6 * 3600, Finale = 5 * 3600 + 29 * 60, Departure = 5 * 3600 + 20 * 60;
 inline double Day(double C) { return FMath::Fmod(FMath::Fmod(C, 86400.) + 86400., 86400.); }
 inline double Night(double C) { const double D = Day(C); return D >= 12 * 3600 ? D - 86400 : D; }
 inline double Residual(bool E) { return (E ? 0 : 2 * 3600) + 13 * 60 + 47; }
 // Reserve scheduled story beats during arbitrarily long free activity, without rewinding a loaded clock.
 inline double AdvanceStoryClock(double Current, double Dt, bool bBeforeResidual)
 {
  const double Limit = bBeforeResidual ? Residual(true) - 6 : Departure;
  return FMath::Max(Current, FMath::Min(Current + Dt, Limit));
 }
 // Authored short jumps on the same signed night axis. Only completed work requests them.
 inline constexpr double Calibration = -5 * 3600 - 30 * 60;
 inline constexpr double Sky01 = -2 * 3600 - 30 * 60, EveningFiled = -20 * 60;
 inline constexpr double FeederDrive = 1 * 3600 + 35 * 60, FirstEncounter = 2 * 3600, TownReport = 2 * 3600 + 15 * 60;
 inline double AdvanceMilestone(double Current, double Requested, bool bBeforeResidual)
 {
  if (!FMath::IsFinite(Requested)) return Current;
  const double Limit = bBeforeResidual ? Residual(true) - 6 : Departure;
  return FMath::Max(Current, FMath::Min(Requested, Limit));
 }
 // Evening cases follow the approved SC01 to SC13 timeline (29 October 2026): SC09, SC10, SC12.
 inline double Chapter3(bool E) { return E ? 4 * 3600 + 25 * 60 : 3 * 3600 + 40 * 60; }
 inline double Chapter4(bool E) { return E ? 4 * 3600 + 50 * 60 : 4 * 3600 + 35 * 60; }
 inline double Chapter5(bool E) { return E ? 5 * 3600 + 12 * 60 : 5 * 3600; }
 // Reserve the exact final 47 seconds for the CB approach even on a slow drive.
 inline double ApproachClock(double Current, double Pace, bool bCbSkipped)
 {
  return FMath::Max(Current, FMath::Min(Pace, Finale - (bCbSkipped ? 0.05 : 47.0)));
 }
 inline bool LocalOnAir(double C) { const double D = Day(C); return D >= 18 * 3600 || D < 4 * 3600; }
 inline bool EarlOnDuty(double C) { return Day(C) < 4 * 3600; }
 inline bool DeputyOnDuty(double C) { const double D = Day(C); return D < 2 * 3600 + 45 * 60 || D >= 18 * 3600 || (D >= 4 * 3600 + 30 * 60 && D < 12 * 3600); }
 inline bool NightPerson(double C, double Leave) { const double D = Day(C); return D >= 18 * 3600 || D < Leave; }
 // Only call for freshly generated 1986 residual references, never on saved text or 1947 records.
 inline FString ResidualText(FString S, bool E)
 {
  if (E) { S.ReplaceInline(TEXT("02:13"), TEXT("00:13")); S.ReplaceInline(TEXT("02:14"), TEXT("00:14")); }
  return S;
 }
}

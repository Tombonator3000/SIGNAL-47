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
 inline double Chapter3(bool E) { return E ? 2 * 3600 + 20 * 60 : 3 * 3600 + 40 * 60; }
 inline double Chapter4(bool E) { return E ? 3 * 3600 + 20 * 60 : 4 * 3600 + 35 * 60; }
 inline double Chapter5(bool E) { return E ? 4 * 3600 + 10 * 60 : 5 * 3600; }
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

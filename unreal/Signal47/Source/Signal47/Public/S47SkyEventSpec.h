#pragma once

// Shared by sky rendering, the lifecycle, observation and future photograph descriptions.
// Angles are degrees; timing is seconds. Existing recorded evidence retains its original extent.
struct FS47StarHoleSpec { double Az, El, Inner, Outer, After, Fade; };
inline constexpr FS47StarHoleSpec STAR_HOLE = { 26, 32, 6, 8, 10, 8 };

namespace S47SkyEvent
{
 inline constexpr double PassageLength = 36;
 inline constexpr double PassageTail = .2;
 // v1 played from event+4 to event+15.2. Never use new duration to replay a spent old save.
 inline constexpr double LegacyPassageStartAfter = 4;
 inline constexpr double LegacyPassageWindow = 11.2;
}

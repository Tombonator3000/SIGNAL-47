#pragma once
#include "CoreMinimal.h"

class US47Main;

// T16-13 and T16-14. The massive light outside, tried from inside: the SARO control room, the old house, the garage and the
// Plaza cinema, one after the other, with the real light (FS47OutsideLight) moving outside each.
//
// -S47MenuPointerFloodIndoors, from an ordinary game started through the menu: takes the eye to fixed views, pictures over
// time in Saved/AlbumTest (flood-<room>-<view>.png, no HUD), and frame times with the light off and on per view in the log
// ("S47: flood probe: ..."). First the rooms' haze with the lamps on, from out in the yard (flood-haze-*.png). Then it exits.
// -S47FloodRoom=<control|house|garage|cinema> takes one room. Values to try: -ExecCmds="s47.flood.Scatter 5,s47.flood.GridPixel 6"
// (S47OutsideLight.cpp lists them).
//
// F10 (Massive light outside): the same light where the player stands or in one of the rooms, 40 seconds, free to walk.
namespace S47FloodProbe
{
 void BeginTest(US47Main* Main,const FString& OnlyRoom);
 // Room: a name above, or empty for where the player stands. False, with the reason, if no light can reach them there.
 bool BeginLive(US47Main* Main,const FString& Room,FString& OutWhy);
 void StopLive(US47Main* Main);
 bool LiveActive();
 /** A step of the test's own ticker, or of FS47DevEvents' while LiveActive; false when it has finished. */
 bool Step(US47Main* Main,float Dt);
}

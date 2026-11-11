#pragma once
#include "CoreMinimal.h"

class US47Main;

// FARM-LIGHTS (Tom, 9 November about 13:30: "Inni låven må det være en lyspære som dingler med lys. Et lys ute på gården. Ekte lys
// innenfra huset der."). Freeform only, wherever the farm's own buildings stand (S47FarmWorld: the saro, road and diner areas).
//
//  The barn: a bare bulb on its cord from the tie beam over the aisle, under a small tin shade. The cord swings as a damped pendulum:
//   a little in the barn's own draught, more with the day's wind (S47Weather::Live) and a gust (FS47Omen's ordinary one), and it is
//   knocked when something comes near (FS47Omen onset). Its one shadowed point light goes with it, so the posts' shadows walk the walls.
//   Lit dusk to dawn; it flickers as something comes near (FS47Omen::SenseAt, the lamp's channel).
//  The yard: one dusk-to-dawn mercury lamp, as the co-op hung on farms then, on a bracket on the barn's gable over the yard: the street
//   lamps' wood arm and cobra head with its photocell (S47StreetLamps' meshes, used as they are), boxes when they are missing. It comes
//   up slowly after the photocell trips, blue-green to white, and drops out and restrikes when something comes near.
//  The old house: a room really lit behind the window. The sitting room's ceiling bowl (build_house.py) glows and one shadowed light
//   under it throws the window and its blind out on the ground; the exported glow sprite at the window (S47FarmWorld) is kept only from
//   afar. Nobody lives there (S47EventPlaces "oldhouse", S47Happenings "where nobody lives"): the Alderetes keep a lamp on a plug-in
//   timer, dusk to 22:30, so the place does not look empty from the road. Later it is lit only by the night's "house_lit"
//   (S47Happenings), and goes out as one comes up to it, as that event says.
//
// -S47FarmLightsOff leaves all of it out (before and after from one build). s47.farmlights.* tune the levels.
namespace S47FarmLights
{
 void Update(US47Main* Main,double Dt);

 // ---------- the pure rules (self-tested) ----------
 bool BarnOn(double Hour);
 bool YardOn(double Hour);
 bool HouseTimerOn(double Hour);
 // How much of a lamp is left as something comes near (Level 0 to 1, FS47Omen), at a time T in seconds: 1 below a faint level.
 double Flicker(double Level,double T);
 // The bulb on its cord: angles (radians) about the two horizontal axes, road x and z, and their rates.
 struct FSwing { double A=0,B=0,VA=0,VB=0; };
 constexpr double CordLength=1.3,MaxSwing=.3;
 // One step; Drive is the push in m/s2 along road x and z. True when the hard stop at MaxSwing had to hold it.
 bool StepSwing(FSwing& S,double Dt,double DriveA,double DriveB);
 // The push at a time: Strength in m/s2, the wind's way (three-space x, z), a seed.
 FVector2D Drive(double T,double Strength,double DirX,double DirZ);
 // How hard the barn's draught pushes: wind 0 to 1, a gust 0 to 1, something near 0 to 1.
 double DriveStrength(double Wind,double Gust,double Near);

 // ---------- where things are, road-local three-space metres ----------
 FVector BarnPivot();
 FVector YardMount();
 FVector YardOut();
 FVector HouseLamp();

 // ---------- F10 ----------
 // A push to the bulb (radians per second about road x and z).
 void DevKick(US47Main* Main,double VA,double VB);
 // A line on what each light is doing now.
 FString DevState(US47Main* Main);

 bool SelfTest(TArray<FString>& Lines);
}

#pragma once

class US47Main;

// Opt-in visual observer of the ordinary C6 drive/event/pullout/ending.
// This never controls gameplay, camera, UI, audio or the game's clocks.
struct FS47FinaleMovie
{
	static void Step(US47Main* Main, double Dt);
};

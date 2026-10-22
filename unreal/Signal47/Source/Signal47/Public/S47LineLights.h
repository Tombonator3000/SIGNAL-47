#pragma once

#include "CoreMinimal.h"
#include "S47Seen.h"

class AS47Web;
class UMaterialBillboardComponent;

// web/src/story/lineLights.ts. HENDELSER.md, event 7: three lights, low over the desert west of the
// highway, keep pace with the truck for a stretch south of SARO, quite silent, and then turn off along
// B and go out. They follow the line, not the truck. Once in a night, driving south between SARO and
// the motel in the dark (main.ts decides when it may run). In SARO's terms (x east, z south).
//
// B runs 210 to 270 m west of the highway there, from SARO's turn-off south-west to STATION 01
// (story/lines.ts): they come up abreast of the truck over it, and when they leave the truck they
// go back onto it and along it, away south-west towards the station, and out.
//
// The three points (THREE.Points, 4 pixels across whatever the distance, additive) are three glow
// billboards made here (the export's 'lineLights' is hidden and has no billboards); each frame they are
// sized to keep four pixels on the screen (Draw), as sizeAttenuation: false does.

struct FS47LineLightsIn
{
	bool bOk = false;
	/** Driving in SARO's area, and where the truck is. */
	TOptional<FVector> Truck;
	double Clock = 0;
	TFunction<double(double, double)> Ground;
	/** The night's curve (story/curve.ts) lets them come now (an event); unset, they may. */
	TOptional<bool> May;
};

class SIGNAL47_API FS47LineLights
{
public:
	FS47LineLights(AS47Web* InWeb, TFunction<void(const FString&)> InNote, TFunction<void(double)> InOnDone);

	/** 'none' | 'pace' | 'off' */
	FString State = TEXT("none");
	bool bDone = false;
	/** T. Vega's field book has them (story/fieldBook.ts): not seen yet, they may come northbound too,
	 *  on the way back. */
	bool bAnyWay = false;

	void Update(double Dt, const FS47LineLightsIn& S);
	/** Where they are now (the middle one, in SARO's terms), or unset when they are not there. */
	TOptional<FVector> At() const;
	/** A new night or a case opened: whether it has been seen in it. */
	void Reset(bool bInDone);
	/** The points as the camera sees them: four pixels across (Unreal: billboards are sized in the world). */
	void Draw(const FS47Cam& Cam, double ViewportH);
	/** Now, abreast of the truck (the tests): `Ahead` +1 driving south, -1 north. */
	bool Now(const FS47LineLightsIn& S, int32 Ahead = 1);
	/** How bright they are (tests). */
	double Opacity() const { return Op; }

	static bool SelfTest(TArray<FString>& Lines);

private:
	TFunction<void(const FString&)> Note;
	TFunction<void(double)> OnDone;
	TWeakObjectPtr<AS47Web> Web;
	TArray<TWeakObjectPtr<UMaterialBillboardComponent>> Pts;
	bool bVisible = false;
	double Op = 0;
	TArray<FVector> Base;
	TArray<FVector> Shown;
	double T = 0;
	double LastZ = 0;
	/** Which way they go when they leave the truck: onto B and along it towards STATION 01. */
	FVector Off;

	void TurnOff();
	void Begin(const FVector& Tr, const FS47LineLightsIn& S, int32 Ahead = 1);
	void SetVisible(bool bOn);
	void Apply();
};

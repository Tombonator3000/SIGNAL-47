#pragma once

#include "CoreMinimal.h"

class AActor;
class UBoxComponent;
class UPostProcessComponent;
class US47Main;
class UStaticMeshComponent;

// GAS-STATION-LIFT-01 (Tom, 11 November: lift the Caliche gas station): the shop's own picture. Inside the shop the frame should
// look like a 1980s photograph under fluorescent tubes: a warm yellow-green cast, less contrast, lifted blacks, soft highlights
// that do not clip, a little grain, a mild vignette, less colour. The rest of the game keeps its look: the grade is a bounded
// post-process volume on the shop's interior only, so it is on when the camera is in the room and off on the forecourt (seen
// through the window and the door), with a short blend at the walls.
//
// Built once the station's mesh is there (AS47Props::PlacedMesh("gasStation")), as the shop's Cosmos props are: one plain actor
// whose root is a box (the shop's interior in the station model's own terms, S47Three::ToUE of its town metres) with a
// UPostProcessComponent under it, which takes the box as its bounds. A spawned APostProcessVolume has no brush at run time, so it
// is not used. Only in Freeform (Story uses the Freeform world too). The values are data: Config/Props/gas_look.json, read when
// the volume is built; a missing or broken file leaves the defaults below. Its optional "exterior" object adds a second, much
// milder grade for the forecourt: a sphere round the canopy with a wide blend (FOutside), off unless enabled there.
//
// Note (read before tuning): the game's own tone pass, M_WebTone, replaces Unreal's tonemapper (S47Web.cpp, the global volume's
// blendable). While it does, Unreal's colour grading, film grain and vignette settings are not drawn by the engine; they take
// effect the day the tonemapper runs, or through a look material (gas_look.json "material", a post-process material after the
// tone pass; loaded only if present and given the same values as parameters). The volume, the box and the data are the same
// either way; the log says which case it is.
class SIGNAL47_API FS47StationLook
{
public:
	explicit FS47StationLook(US47Main* Main) : M(Main) {}

	/** Every frame. Builds the volume the first time the station is there in Freeform; keeps it on only in Freeform. */
	void Step();

	/** The grade as the file has it (the defaults are Tom's starting values). */
	struct FLook
	{
		double Saturation = .78, Contrast = .88;
		FVector Gamma = FVector(1, 1, 1);
		FVector Gain = FVector(1, .985, .86);               // warm yellow-green
		FVector OffsetShadows = FVector(.012, .013, .010);  // lifted blacks
		FVector GainHighlights = FVector(.92, .92, .92);    // soft highlights
		double Grain = .35, GrainSize = 1, Vignette = .45, Bloom = .9;
		double BlendCm = 70;        // the blend's reach outside the box
		double Priority = 10;       // above the game's global volume (engine default 0, S47Web.cpp)
		double WindowInsetCm = 30;  // the box pulled in from the window and door wall so the blend stays off the pavement
		bool bEnabled = true;
		FString Material;           // optional post-process material, loaded if present
	};
	/** The forecourt's own, much milder grade: a sphere round the canopy with a wide blend, so no edge is ever seen; below the
	 *  shop's box (priority one under it), above the global volume. Off unless the file's "exterior" says "enabled": true. */
	struct FOutside
	{
		FLook Grade;                            // its own defaults (set in ReadFile); Grade.Bloom is not overridden
		FVector Centre = FVector(6.4, 0, -9.9); // town metres from the station's pivot: the canopy's middle
		double RadiusM = 42, BlendM = 28;
	};
	/** Config/Props/gas_look.json read over the defaults: the flat keys are the shop's grade, "exterior" the forecourt's. False when
	 *  it does not read (both keep the defaults; the forecourt off). */
	static bool ReadFile(FLook& Out, FOutside& Outside, FString* Error = nullptr);
	/** The box in the station model's terms (Unreal cm, relative to the station's pivot) for a look: its min and max corners. */
	static void BoxOf(const FLook& L, FVector& Min, FVector& Max);
	/** The file reads (or the defaults stand, said so); every value in a sane range; the box inside the shop, pulled in from the
	 *  window wall; whether the look material is here (passes with and without it). */
	static bool SelfTest(TArray<FString>& Lines);

	bool Built() const { return Volume.IsValid(); }

private:
	US47Main* M = nullptr;
	TWeakObjectPtr<UStaticMeshComponent> BuiltFor;
	TWeakObjectPtr<AActor> Actor, OutActor;
	TWeakObjectPtr<UPostProcessComponent> Volume, OutVolume;
	bool bOn = false;
	int32 Builds = 0;
	void Build(UStaticMeshComponent* Station);
	void SetOn(bool bEnable);
};

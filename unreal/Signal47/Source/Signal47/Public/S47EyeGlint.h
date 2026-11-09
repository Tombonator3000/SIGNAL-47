#pragma once
#include "CoreMinimal.h"

class UObject;
class USceneComponent;
class USkinnedMeshComponent;
class UStaticMeshComponent;

// The faint white dot in each big black alien eye (Tom, 6 and 9 November: "alle aliens må ha øyeblink"). Two tiny points, seen in
// the dark the way a figure in a dark farmhouse is only made out by them. Tom's limits, 6 November: only in the dark, never past
// 50 m, faint, and never a light on anything round it: the dots are emissive specks, not lamps, and give no light, shadow or
// reflection to the world. One switch turns every dot off: -S47EyeGlintOff, or the console variable s47.EyeGlint 0.
// Where the eyes are is measured per body on the model (Tools/triponpcs/measure_eyes.py) and held in For().
namespace S47EyeGlint
{
 // The two dots of one body: component centimetres from the head bone at its rest pose (forward +Y, up +Z), the body's left (+X)
 // and right, 1.5 mm out from the black of each eye.
 struct FEyes { FVector Left, Right; };
 // The measured eyes of a body by its asset's name (SK_Tripo_PaleOne ...); null for a body with none measured.
 SIGNAL47_API const FEyes* For(const FString& AssetName);
 // How bright a dot is, 0..1: the dark (the sky's Daylight(), 0 at night) times the distance (full to 30 m, none from 50 m) times
 // how far the face is turned to the eye (Facing: the head's forward against the line to the viewer, 1 straight on).
 SIGNAL47_API double Level(double Daylight, double Metres, double Facing = 1.);
 inline constexpr double FadeFrom = 30., GoneAt = 50.;
 // Two dots on a skinned body's head bone. Out gets them, to be dropped with the body.
 SIGNAL47_API void Add(UObject* Outer, USkinnedMeshComponent* Mesh, const FEyes& Eyes, TArray<TWeakObjectPtr<UStaticMeshComponent>>& Out,
  FName Bone = TEXT("head"));
 // A static body baked in a pose (Tom's Tripo figures of 9 November, S47TripoNew): its eyes in the mesh's own centimetres from
 // its foot pivot, forward +Y, up +Z, measured on the baked mesh; null for a mesh with none measured.
 SIGNAL47_API const FEyes* ForStatic(const FString& AssetName);
 // Two dots on a static body, children of the mesh itself: they move, turn and hide with it, and Step treats them as the rest.
 SIGNAL47_API void AddStatic(UObject* Outer, UStaticMeshComponent* Mesh, const FEyes& Eyes, TArray<TWeakObjectPtr<UStaticMeshComponent>>& Out);
 // The same, when the mesh is one with its eyes measured: false for any other (nothing is added).
 SIGNAL47_API bool AddStaticIfKnown(UObject* Outer, UStaticMeshComponent* Mesh, TArray<TWeakObjectPtr<UStaticMeshComponent>>& Out);
 // Every dot once a frame, from the viewer's eye (Unreal space) and the sky's light.
 SIGNAL47_API void Step(const FVector& EyeUE, double Daylight);
 // Dots drawn now.
 SIGNAL47_API int32 Shown();
 // Dots that exist (on built bodies, shown or not).
 SIGNAL47_API int32 Count();
 // Every dot is a speck and not a lamp: no shadow, no light to what is round it, no light component under it. Why: the first that
 // is not.
 SIGNAL47_API bool GivesNoLight(FString* Why = nullptr);
 // Off by the flag or the console variable.
 SIGNAL47_API bool Enabled();
 // Tests: shown by day as well (a close look against the face), and the slow drift and the blink held still, so a picture shows
 // the dot at its rest point.
 extern SIGNAL47_API bool bTestAlways;
 extern SIGNAL47_API bool bTestStill;
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}

#pragma once

#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"

class AS47Web;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPrimitiveComponent;
class USceneCaptureComponent2D;
class UStaticMeshComponent;
class UTextureRenderTarget2D;
struct FS47Truck;

// Unreal only (the web version has no mirrors): the Jeep's two door mirrors and its inside rear-view
// mirror, each a small scene capture into its own render target, shown mirrored on the glass.
//
// The glass is the model's own when the Jeep has a material slot named MI_JeepMirrorGlass_L, _R or
// _Rear (any slot or material name containing JeepMirrorGlass_<key>); otherwise a thin plane of our
// own, placed from the table at the top of S47Mirrors.cpp, over the existing mirror housings.
// Captures only while the player sits in this Jeep and Settings > Graphics > Vehicle mirrors is on,
// one mirror a frame at most, each at most MirrorHz; otherwise nothing is captured and the glass is
// an ordinary dark mirror.
namespace S47Mirrors
{
	/** The Options switch (US47Flow keeps it in the settings file as "vehicleMirrors"). */
	SIGNAL47_API void SetEnabled(bool bOn);
	SIGNAL47_API bool Enabled();
}

class SIGNAL47_API FS47Mirrors
{
public:
	static constexpr int32 Count = 3; // left, right, rear (the table's order)
	/** The Jeep's mirrors; null for any other vehicle. Call after FS47Fleet::Install, with its parts. */
	static TUniquePtr<FS47Mirrors> Install(AS47Web* W, FS47Truck& T, const TArray<TWeakObjectPtr<UStaticMeshComponent>>& Parts);
	~FS47Mirrors();

	/** Once a frame from US47World::Frame for the truck being driven: whether the player is at its wheel,
	 *  the eye and the look (three.js world). Not called for a frame or two: the mirrors go idle. */
	void Step(const FS47Truck& T, bool bAtWheel, const FVector& EyeThree, const FQuat& LookThree);

	bool Live() const { return bLive; }
	/** Any capture component active (only while live). */
	bool CaptureActive() const;
	/** Captures asked for since this was made. */
	int32 Captures() const { return CaptureCount; }
	/** Mirror I's render target (made the first time the mirrors go live). */
	UTextureRenderTarget2D* Target(int32 I) const;
	/** Whether mirror I uses the model's own MI_JeepMirrorGlass_* slot (else the fallback plane). */
	bool UsesModelGlass(int32 I) const;

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47Mirrors() = default;
	struct FGlass
	{
		TWeakObjectPtr<UStaticMeshComponent> Mesh;     // the model's part or our plane
		int32 Slot = 0;
		bool bModel = false;
		TStrongObjectPtr<UMaterialInterface> Off;      // the model's own glass, or our dark mirror
		TStrongObjectPtr<UMaterialInstanceDynamic> Live;
		TStrongObjectPtr<UTextureRenderTarget2D> Target;
		TStrongObjectPtr<USceneCaptureComponent2D> Capture;
		double Last = -1;                              // FPlatformTime of its last capture
		bool bImage = false;                           // captured since it went live
	};
	FGlass Glass[Count];
	TArray<TWeakObjectPtr<UPrimitiveComponent>> Hidden;    // never in any mirror: our own planes
	TArray<TWeakObjectPtr<UPrimitiveComponent>> CarParts;  // the Jeep itself: not in the inside mirror
	TArray<TWeakObjectPtr<UStaticMeshComponent>> Made;     // our fallback planes and the rear mirror's backing
	TWeakObjectPtr<AS47Web> Web;
	FVector Aim[Count];                                    // truck-local glass normals the view is worked from
	FDelegateHandle Tick;
	uint64 FedFrame = 0;
	int32 CaptureCount = 0;
	bool bLive = false;

	void MakeCaptures(UObject* Outer, bool bRegister);
	void EnsureTarget(int32 I);
	void SetLive(bool bOn);
	void ShowGlass(int32 I);
	/** The frame's work: the car's frame (three world), the eye and look, and the time now. */
	void StepAt(bool bWant, const FQuat& CarQ, const FVector& CarP, const FVector& Eye, const FQuat& Look, double Now);
};

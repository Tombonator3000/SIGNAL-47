#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"

class AS47Web;
class UPrimitiveComponent;
class UStaticMeshComponent;

// Native presentation shared by the road encounter, close passes and abductions.
// The owner supplies all time and phase: this never advances gameplay or saves it.
class SIGNAL47_API FS47UfoPresentation
{
public:
	struct FFrame
	{
		// Three coordinates: metres, Y up. Forward is projected onto the ground plane.
		FVector Position = FVector(0, 24, 0);
		FVector GroundTarget = FVector::ZeroVector;
		FVector Forward = FVector(0, 0, -1);
		double Seconds = 0;
		double Width = 8;
		double Opacity = 1;
		double Beam = 0;
		double NearWhite = 0;
		bool bHull = true;
		bool bSlowBeam = false;
		bool bCalm = false;
	};

	explicit FS47UfoPresentation(AS47Web* InWeb);
	~FS47UfoPresentation();
	bool Prepare();
	void Show(const FFrame& Frame);
	void Hide();
	bool Presented() const;
	// Optional native target for the owner's ordinary sight/photo trace.
	const UPrimitiveComponent* SightTarget() const;
	// Physical hull and lamps only; excludes additive air and camera-facing halos.
	TArray<UStaticMeshComponent*> PhotoParts() const;
	S47J::FObj Readback() const;

private:
	struct FVisual;
	TUniquePtr<FVisual> Visual;
};

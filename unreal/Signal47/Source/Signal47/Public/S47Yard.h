#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"
#include "UObject/Object.h"
#include "S47Yard.generated.h"

class US47Main;
class UTextureRenderTarget2D;
class FWidgetRenderer;
class SS47Bus;

// world/ServiceYard.ts YARD: the places in the yard the story measures against
namespace S47YardPlaces
{
	// where FRAME 01 must be taken
	constexpr double ApronMinX = 8.0, ApronMaxX = 12.6, ApronMinZ = -15.2, ApronMaxZ = -8.6;
	inline FVector SightMark() { return FVector(10.0, 0, -15.0); }  // FRAME 02 is taken near this
	inline FVector Vane() { return FVector(9.9, 2.35, -18.35); }    // centre of the B-12 vane board
	constexpr double EchoOffset = 0.45;                               // film-only stripe, beside the vane
	// the service truck's pad below the east landing (chapter three drives out from here)
	constexpr double TruckX = 8.9, TruckZ = 8.0, TruckHeading = PI;
}

// world/ServiceYard.ts, the behaviour: chapter one's location east of the control room. The
// geometry is the web version's own (Tools/webexport); this holds its parts by their paths and
// does what the class's methods do: the B-12 vane, the work lamp (its fake flood and its glow),
// the motor bus screen, the print in the fixer and the drying line.
UCLASS()
class SIGNAL47_API US47Yard : public UObject
{
	GENERATED_BODY()

public:
	void Init(US47Main* InM);
	virtual void BeginDestroy() override;

	// objs: motorBus, b12Control, vane, wetBench, archive, records, r07, labDoor, truck
	FS47Obj Obj(const FString& Name) const;
	FS47Obj VaneObj, VaneStripe, Echo, WetPrint, LabDoorHinge;
	TArray<FS47Obj> DryPrints;
	TArray<FS47Obj> Occluders;
	double WorkLampBase = 7;

	void SetVane(double YawDeg);
	void SetWorkLamp(double Level);
	// Drawn on the motor bus inspection glass.
	void DrawBus(double Az, int32 Commands, bool bWarn, double T);
	UTextureRenderTarget2D* BusTarget() const { return Bus; }

	// the parts of Content/WebData/manifest.json the port reads beside AS47Web (the flood pools and
	// the dishes' places), parsed once
	static TArray<TSharedPtr<class FJsonValue>> ManifestSection(const FString& Key);

private:
	UPROPERTY() TObjectPtr<US47Main> M;
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> Bus;
	FWidgetRenderer* Renderer = nullptr;
	TSharedPtr<SS47Bus> BusWidget;
	int32 FloodSetIdx = -1, WorkLamp = -1; // the site floods and the work lamp's pool in them
	int32 WorkGlow = -1;                    // its point in the yard's glow
	void FindWorkLamp();
};

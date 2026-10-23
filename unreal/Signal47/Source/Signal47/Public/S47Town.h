#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"

class US47Main;

// world/Town.ts: the exported Caliche, the people and the night's changing lamps.
class SIGNAL47_API FS47Town
{
public:
	explicit FS47Town(US47Main* InM);
	FS47Obj Group;
	FS47Obj Proxy(const FString& Id) const;
	bool bDeputyIn = true, bPaperIn = false, bWomanIn = true, bSmokerIn = true;
	bool bRinging = false, bCutoutOut = false, bCarOut = false, bStudioEmpty = false;
	TFunction<void()> OnAnswer;
	TFunction<void(const FS47Area&)> OnCarCollider;
	FVector CutoutAt = FVector(-6.4, 0, 1450), CarShoulder = FVector(6.9, 0, 1690);
	void SetCutout(bool bOn);
	// Pilot owns one physical figure; the old town event keeps its historical flags only.
	bool bAnomalyCutoutOwned = false, bAnomalyBaseline = false;
	FS47Obj CutoutObject() const { return Cutout; }
	void SetPilotCutout(bool bMoved);
	void ReleasePilotCutout();
	FVector PilotCutoutAt(bool bMoved) const;
	void LampWave(double T) { WaveT0 = T; }
	TArray<FVector> WaveOrder() const;
	static constexpr double WAVE_STEP = 1.3, WAVE_HOLD = 24;
	void SetPeople(bool bWoman, bool bSmoker);
	void SetDeputy(bool bOn);
	void SetCarOut(bool bOn);
	void SetPaper(bool bOn);
	void Update(double T, double Clock, bool bHold = false);
	void Shine(int32 Src, const TArray<int32>& Slots, double K);
	void LampsTo(int32 Out, const TArray<int32>& Slots, const FVector& At, double K);
	bool InTown(double X, double Z) const;
	FString PlaceAt(double X, double Z) const;
	FString SurfaceAt(double X, double Z) const;
	double FloorAt(double X, double Z) const;
	static bool SelfTest(TArray<FString>& Lines);
private:
	US47Main* M = nullptr;
	FVector Origin = FVector(8000, 0, 0);
	FS47Obj Car, CarDoor, Paper, Woman, Smoker, Cutout;
	int32 Flood = -1;
	bool bDrawing = false, bBlinkOn = true;
	double WaveT0 = -1e9;
	struct FLamp { double Z = 0, Off = 0; int32 Slot = -1, Quad = -1; bool bOn = true; };
	TArray<FLamp> Lamps;
	TArray<TPair<FString, int32>> CarColliders;
	void PlaceCar();
	void SetPanel(const FString& Id, double K, TOptional<int32> Only = {});
	void SetQuadRect(bool bAir);
	void GlowColor(int32 I, uint32 C);
	void SetFlood(int32 I, double W);
};

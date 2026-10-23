#pragma once
#include "CoreMinimal.h"
#include "S47Obj.h"
#include "S47Audio.h"

class US47Main;
class FS47Town;
class FS47AnomalyDread;
struct FS47AlbumShot;

// Unreal pilot's physical controls. Investigation judgments belong to FS47Anomaly;
// this module owns only the persistent figure variant, lamp and source of the sound.
class SIGNAL47_API FS47AnomalyWorld
{
public:
	explicit FS47AnomalyWorld(US47Main* InM) : M(InM) {}
	void Begin();
	void Step(double Dt);
	void Reset();
	void DreadStep(double Dt);
	void BeforePhoto();
	void AfterPhoto();
	FVector N02TraceAt() const;
	FVector PrintShadowAt() const;
	void TagPhoto(FS47AlbumShot& Shot) const;
	bool Busy() const;
	FVector ReferenceAt() const;
	FVector ChangedAt() const;
	FVector LampAt() const;
	FVector StudioAt() const;
	static bool MayMove(bool bReference, bool bMoved, double Clock, double Distance);
	static bool MayPlay(bool bPending, bool bHeard, bool bPanel, bool bFree, bool bHere, double Clock);
	static bool SelfTest(TArray<FString>& Lines);
private:
	US47Main* M = nullptr;
	FS47Town* Town = nullptr;
	FS47Obj Lamp, LampShade;
	TArray<FS47Obj> N02Film, A02Film;
	TSharedPtr<FS47AnomalyDread> Dread;
	mutable bool bFilmN02 = false, bFilmA02 = false;
	bool bBuilt = false, bDisabled = false;
	double SoundLeft = 0, SoundRetry = 0;
	double ApproachTime = 0, StillTime = 0;
	FVector LastPlayer = FVector::ZeroVector;
	FS47Play Printer, InitialPrinter;
	TSharedPtr<FS47PannerNode> PrinterPan;
	void StopPrinter();
	bool FilmLineClear(const FVector& To) const;
	FVector World(const FVector& Local) const;
	bool Here() const;
	bool Free() const;
	bool ControlStand() const;
	void InspectLamp();
	void InspectFigure();
	void InspectStudio();
	void InstallGeometry();
	void PlaceFigure();
	void Toast(const FString& Text) const;
};

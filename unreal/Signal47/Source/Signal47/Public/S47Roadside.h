#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"
#include "S47Seen.h"

class US47Main;
class FS47GainNode;
class FS47PannerNode;

// core/roadside.ts: the highway shoulder's line, distant headlights, 47-second silence and roof.
struct FS47RoadsideIn
{
	bool bOk = false;
	FVector Pos = FVector::ZeroVector;
	FS47Cam Cam;
	bool bSilenceDone = false;
	TFunction<void(double, double)> Hush;
	TFunction<void(double)> Flicker;
	TFunction<void()> SilenceOver;
	/** 'car' | 'silence' | 'roof'; unset, they may. */
	TFunction<bool(const FString&)> May;
};

// Silhouette's geometry is the one exported from the running web game, never rebuilt here.
class SIGNAL47_API FS47Silhouette
{
public:
	FS47Silhouette(AS47Web* Web, const FString& Name, uint32 Edge, const FString& Want = TEXT("stand"));
	FS47Obj Group;
	void Wake();
	void Pose(const FString& P);
	double HeadY() const { return 1.1; }
	void Rim(uint32 Hex);
	bool Painted() const { return false; }
private:
	uint32 Edge = 0;
	FString Want;
};

class SIGNAL47_API FS47Roadside
{
public:
	explicit FS47Roadside(US47Main* InM);
	~FS47Roadside();
	bool bSilent = false;
	struct FLast { FString What; double At = 0; };
	TOptional<FLast> Last;

	void Update(double Dt, const FS47RoadsideIn& O);
	void CarNow(const FVector& From);
	struct FCarState { FString State; double Z = 0; };
	TOptional<FCarState> CarState() const;
	TOptional<FVector> CarAt() const;
	TOptional<FVector> RoofAt() const;
	void SilenceNow(const FS47RoadsideIn& O);
	void RoofNow();
	bool RoofUp() const { return Roof == TEXT("up"); }
	void Reset();
	static bool SelfTest(TArray<FString>& Lines);
private:
	US47Main* M = nullptr;
	double T = 0, OnFoot = 0, Walked = 0, LastZ = 0, SilenceT = 0;
	struct FHum { TSharedPtr<FS47GainNode> G; TSharedPtr<FS47PannerNode> Pan; TFunction<void()> Stop; };
	TOptional<FHum> Hum;
	struct FCar { FS47Obj Pts; double Z = 0; FString State = TEXT("come"); double T = 0; };
	TOptional<FCar> Car;
	double CarNext = 0, CarAgain = 0;
	FS47Silhouette Sil;
	FS47Obj Figure;
	FString Roof = TEXT("none");
	double RoofT = 0;
	void LineHum(double Dt, const FS47RoadsideIn& O);
	void CarStep(double Dt, const FS47RoadsideIn& O);
	void SilenceStep(double Dt, const FS47RoadsideIn& O);
	void RoofStep(double Dt, const FS47RoadsideIn& O);
	void HideRoof();
	double OffCentre(const FS47Cam& Cam) const;
};

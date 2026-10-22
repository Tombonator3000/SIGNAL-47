#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Seen.h"
#include "UObject/Object.h"
#include "S47TownLayer.generated.h"

class US47Main;
class FS47Town;
class FS47Diner;
class FS47Caliche;
class FS47TownEvents;
class FS47Roadside;
class FS47Cutouts;
class FS47Herd;
class FS47Pasture;
class FS47NinthModel;
class FS47NinthCow;
class FS47MotelTv;
class FS47DinerCooler;
class FS47MorningHolds;
struct FS47CutoutsIn;
struct FS47RoadsideIn;
struct FS47MorningIn;

// main.ts's town, road and motel wiring. Each TypeScript module keeps its own C++ module;
// these are only boot's deps, context builders and the calls in main.ts's step/save order.
UCLASS()
class SIGNAL47_API US47TownLayer : public UObject
{
	GENERATED_BODY()
public:
	void Init(US47Main* InM);
	TSharedPtr<FS47Town> Town;
	TSharedPtr<FS47Diner> Diner;
	TSharedPtr<FS47Caliche> Caliche;
	TSharedPtr<FS47TownEvents> TownEvents;
	TSharedPtr<FS47Roadside> Roadside;
	TSharedPtr<FS47Cutouts> Cutouts;
	TArray<TSharedPtr<FS47Herd>> Herds;
	TSharedPtr<FS47Pasture> Pasture;
	TSharedPtr<FS47NinthModel> NinthModel;
	TSharedPtr<FS47NinthCow> Ninth;
	TSharedPtr<FS47MotelTv> MotelTv;
	TSharedPtr<FS47DinerCooler> DinerCooler;
	TSharedPtr<FS47MorningHolds> Morning;
	FS47RoadsideIn RoadsideIn(bool bModal) const;
	FS47CutoutsIn CutoutIn(bool bOk) const;
	FS47MorningIn MorningIn(bool bModal) const;
	void WorldStep(double Dt);
	void ShareLights();
	void RoadsideStep(double Dt, bool bModal);
	void TownStep(double Dt, bool bModal);
	void CutoutsStep(double Dt, bool bModal);
	void EventsOnB(double Dt, bool bModal);
	void NinthStep(double Dt, bool bModal);
	void MorningStep(double Dt, bool bModal);
	void CurveStep(double Dt);
	void SeenStep();
	void SeenSync();
	// ch3.freeShot: a frame close to the ninth goes into the free album.
	bool FreeShot() const;
	void ResetTown();
	void ResetCutouts();
	void ResetPastureTv();
	void ResetNinthMorning();
	void Refile(const S47J::FObj& State);
	void Save(const S47J::FObj& State) const;
	void Leave(bool bTitle);
	static bool SelfTest(TArray<FString>& Lines);
private:
	UPROPERTY() TObjectPtr<US47Main> M;
	FString SilencePlace;
	FVector NinthEarAt = FVector::ZeroVector;
	TSharedPtr<class FS47AudioNode> NinthEar;
	double LineBCheck = 0, PostFlick = 0, PostSpeed = 0;
	int32 CutPlaced = 0, PostBuild = -1;
	FVector PostLast = FVector::ZeroVector;
	int32 WasCutSeen = 0;
	FString WasRoad;
	double WasRoadAt = -1;
	FString LoggedTown, LoggedPasture, LoggedTv, LoggedNinth, LoggedMorning;
	bool bLoggedEnd = false;
	FString Area() const;
	FString Phase() const;
	bool At(const FString& Ch) const;
	bool Driving() const;
	bool Cinematic() const;
	bool Is(const TFunction<bool()>& F) const { return F && F(); }
	double Clock() const;
	FS47Cam Camera() const;
	FVector PlayerAt() const;
	void PostStep(double Dt);
	void TestFlags();
};

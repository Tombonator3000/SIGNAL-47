#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
#include "S47FieldLogger.h"
#include "S47Workshop.h"
#include "UObject/StrongObjectPtr.h"
class US47Main;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UTextRenderComponent;
class FS47FreeformCarry;

// Freeform-only physical supply, service and transport layer. No Story facts.
class FS47FreeformWorld
{
public:
 explicit FS47FreeformWorld(US47Main* InM);
 ~FS47FreeformWorld();
 void Begin(const S47J::FObj& Saved = nullptr);
 void Stop();
 void Step(double Dt);
 void AdvanceUnobservedTo(double Clock); // settle battery/power before saving a rest or wait jump
 S47J::FObj Save() const;
 bool Use(const FString& Id);
 FString Label(const FString& Id) const;
 int32 InventoryCount(const FString& Item) const;
 void AddItem(const FString& Item, int32 Count = 1);
 bool Consume(const FString& Item);
 bool Purchase(const FString& Item, bool bEmergency = false);
 bool RequestEmergencyFuse();
 bool EmergencyFuseAvailable() const;
 void OpenShop();
 void OpenWorkbench();
 void OpenWorkshop();
 // unpack, test <part id>, scrap <part id>, build <recipe>, strip <recipe>. Hands free, at the bench.
 bool WorkshopAction(const FString& Action,const FString& Arg=FString());
 FString WorkshopBlock(const FString& Action,const FString& Arg=FString()) const;
 const FS47Workshop& Workshop() const { return Crafting; }
 bool LoadFilmAtPhotoLab();
 bool LoggerAccess() const;
 FString LoggerPreparationBlock(const FString& Action) const; // shared tune/collect availability and reason
 bool LoggerAction(const FString& Action,const FString& TapeId=FString(),const FString& OriginalId=FString());
 const FS47FieldLogger& FieldLogger() const { return Logger; }
 bool AnalyzeSample();
 bool StartDonorTest();
 bool TriggerServiceFault(bool bTraining=false,const FString& RequestedDish=FString());
 const FString& ServiceDishId() const { return ServiceTarget; }
 int32 ServiceDishIndex() const;
 FVector ServicePosition() const;
 bool HasArrayFault() const { return Service >= 1 && Service < 9; }
 int32 FaultedDishCount() const { return HasArrayFault() ? 1 : 0; }
 int32 ServiceStage() const { return Service; }
 int32 FeederStage() const { return Feeder; }
 bool FeederIsolated() const { return Feeder == 3 || Feeder == 4; }
 void DiagnoseService();
 FString ServiceStatus() const;
 bool GeneratorRunning() const { return bGenerator && GeneratorSeconds > 0; }
 bool HasBackupPower() const { return GeneratorRunning(); }
 double GeneratorHours() const { return GeneratorSeconds / 3600.; }
 double VehicleFuel() const { return Fuel; }
 bool HasVehicleFuel() const { return Fuel > .001; }
 bool StartPump();
 void StopPump();
 bool PumpActive() const { return bPumping; }
 double PumpDelivered() const { return PumpVolume; }
 int32 PumpCharges() const { return PumpPaid; }
 double PumpProgress() const { return PumpElapsed / 2.; }
 FString PumpDisplay() const;
 bool TripoPumpLoaded() const;
 int32 ParcelStage() const { return Payload; } // 0 empty, 1 shop, 2 carried, 3 loaded, 4 set down
 FString ParcelItem() const { return Item; }
 const TMap<FString,int32>& ParcelContents() const { return Contents; }
 FString ParcelDescription() const;
 bool DropParcel(bool bDrop = false);
 void OpenCarryMenu();
 FVector ParcelPosition() const;
 bool ParcelSimulating() const;
 bool CarryBlocked() const;
 int32 ParcelImpactCount() const;
 int32 DeliveredParcels() const { return Delivered; }
 int32 CargoDeliveries() const { return DeliveredByTruck; }
 int32 ServiceSupplyDeliveries() const { return ServiceDeliveries; }
 int32 DonorTestsCompleted() const { return DonorTests; }
 int32 ServiceCompletions() const { return ServiceTests; }
 bool BenchBusy() const { return !BenchJob.IsEmpty(); }
 bool ShopOpen() const;
 FVector InteractionPoint(const FString& Id) const;
 // Where a player stands to use a station (the casework fronts face different ways).
 FVector StandPoint(const FString& Id) const;
 // Field-control staging owns these objects, including visibility and rebuild identity.
 // AdoptNode defaults its transform scale; retain dimensions for later native rotations.
 FS47Obj StageBox(const FString& Name,const FVector& At,const FVector& Size,uint32 Colour,int32 Parent)
 {FS47Obj O=Box(Name,At,Size,Colour,Parent);if(O)O.SetScale(Size);Parts.RemoveAll([&](const FS47Obj& P){return P==O;});return O;}
 static bool SelfTest(TArray<FString>& Lines);
private:
 FS47FieldLogger Logger;
 FS47Workshop Crafting;
 FS47Obj TrayParts[FS47Workshop::TraySize];
 void FinishWorkshopJob(const FString& Job);
 FS47Obj LoggerPosts[2],LoggerBody;
 bool bLoggerCalibrating=false;
 FString LoggerJob;
 double LoggerJobElapsed=0;
 FVector LoggerPoint(int32 Site) const;
 void StepLogger(double Dt,double WorldDelta);
 US47Main* M = nullptr;
 TSharedPtr<uint8> Life = MakeShared<uint8>(0);
 bool bActive = false, bBuilt = false, bGenerator = false, bFuelHeld = false;
 FString ServiceTarget=TEXT("S-03");
 int32 OrdinaryServiceSerial=0;
 int32 Service = 0, Feeder = 0, LastFaultDay = 0, EmergencyDay = 0;
 int32 PowerFaultSerial = 0, EmergencyFuseFault = 0;
 bool bTrackedPowerFault = false;
 int32 Payload = 0;
 TMap<FString,int32> Contents;
 bool CanAddShopItem() const;
 int32 Delivered = 0, DeliveredByTruck = 0, ServiceDeliveries = 0, DonorTests = 0, ServiceTests = 0;
 bool bParcelWasLoaded = false;
 bool bPumping = false;
 double PumpRemaining = 0, PumpElapsed = 0, PumpVolume = 0;
 int32 PumpPaid = 0;
 TWeakObjectPtr<UTextRenderComponent> PumpTextEast, PumpTextWest, ServiceText;
 FString LastServiceDisplay;
 FString LastPumpDisplay;
 bool bTripoPump = false;
 FS47Obj PumpModel;
 TWeakObjectPtr<UStaticMeshComponent> PumpStation;
 TStrongObjectPtr<UStaticMesh> PreviousStation;
 TArray<TStrongObjectPtr<UMaterialInterface>> PumpOriginalMaterials;
 TSharedPtr<FS47FreeformCarry> Carry;
 FString Item, BenchJob;
 TMap<FString,int32> Stock;
 double GeneratorSeconds = 0, Fuel = 3, LastClock = -1, BenchSeconds = 0, FeederSeconds = 0, Settle = 0, ServiceAz = 0, ServiceEl = 0;
 FVector Dropped = FVector::ZeroVector;
 FS47Obj Bench, SampleBench, Delivery, Depot, BenchMeter, BenchLamp, BenchSample, Shop, Panel, Lid, Relay, Selector, Generator, FeederPanel, FeederLever, FeederLamp;
 FS47Obj ParcelSaro, ParcelRoad, CargoSaro, CargoRoad;
 TArray<FS47Obj> Parts;
 struct FOverride { bool bLook = false; TFunction<FString()> Label, Examine; TFunction<void()> Use; };
 TMap<FString,FOverride> Overrides;
 void OverrideExisting();
 bool Build();
 FS47Obj Box(const FString& Name, const FVector& At, const FVector& Size, uint32 Colour, int32 Parent, const FString& Id = FString());
 FS47Obj Model(const FString& Name, const TCHAR* Asset, const FVector& At, double Width, int32 Parent, double Yaw = 0);
 void Bind(FS47Obj Object, const FString& Id, double Range = 2.6);
 void Apply();
 bool Near(const FVector& At, double Range = 3.0) const;
 bool Ready() const;
 void TrackPowerFault(bool bFault);
 bool CanIssueEmergencyFuse(bool bFault) const;
 FString ServiceWords(const FString& Text) const;
 FVector ShopPoint() const;
 FVector ParcelShopPoint() const;
 bool AtShopCounter() const;
 FVector TruckPoint() const;
 FVector PumpPoint() const;
 bool PumpConditions() const;
 bool AtDiesel() const;
 void StepPump(double Dt);
 void ApplyPump();
 bool InstallPumpModel(int32 RoadParent);
 void RestorePumpModel();
 bool InstallServiceCabinet();
 void RestoreServiceCabinet();
 struct FCabinetOriginal {
  TWeakObjectPtr<UStaticMeshComponent> Component;
  TStrongObjectPtr<UStaticMesh> Mesh;
  TArray<TStrongObjectPtr<UMaterialInterface>> Materials;
 };
 TArray<FCabinetOriginal> CabinetOriginals;
 FS47Obj CabinetBody;
 bool bServiceCabinet = false;
 void Change(const FString& Message = FString());
 bool ServiceUse();
 bool FeederUse();
 bool GeneratorUse();
 bool ParcelUse();
 bool CargoUse();
 bool Deposit(const FString& Station);
 void OpenDepot();
 void OpenSampleLab();
 void DonorTest();
 void RefuelTruck();
 void RoadsideAssist();
};

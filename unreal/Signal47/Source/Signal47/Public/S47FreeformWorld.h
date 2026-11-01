#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
class US47Main;
class UStaticMeshComponent;
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
 S47J::FObj Save() const;
 bool Use(const FString& Id);
 FString Label(const FString& Id) const;
 int32 InventoryCount(const FString& Item) const;
 void AddItem(const FString& Item, int32 Count = 1);
 bool Consume(const FString& Item);
 bool Purchase(const FString& Item, bool bEmergency = false);
 void OpenShop();
 void OpenWorkbench();
 bool AnalyzeSample();
 bool StartDonorTest();
 bool TriggerServiceFault();
 bool HasArrayFault() const { return Service >= 1 && Service < 9; }
 int32 FaultedDishCount() const { return HasArrayFault() ? 1 : 0; }
 int32 ServiceStage() const { return Service; }
 void DiagnoseService();
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
 int32 ParcelStage() const { return Payload; } // 0 empty, 1 shop, 2 carried, 3 loaded, 4 set down
 FString ParcelItem() const { return Item; }
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
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M = nullptr;
 TSharedPtr<uint8> Life = MakeShared<uint8>(0);
 bool bActive = false, bBuilt = false, bGenerator = false, bFuelHeld = false;
 int32 Service = 0, Feeder = 0, LastFaultDay = 0, EmergencyDay = 0;
 int32 Payload = 0;
 int32 Delivered = 0, DeliveredByTruck = 0, ServiceDeliveries = 0, DonorTests = 0, ServiceTests = 0;
 bool bParcelWasLoaded = false;
 bool bPumping = false;
 double PumpRemaining = 0, PumpElapsed = 0, PumpVolume = 0;
 int32 PumpPaid = 0;
 TWeakObjectPtr<UTextRenderComponent> PumpTextEast, PumpTextWest;
 FString LastPumpDisplay;
 TSharedPtr<FS47FreeformCarry> Carry;
 FString Item, BenchJob;
 TMap<FString,int32> Stock;
 double GeneratorSeconds = 0, Fuel = 3, LastClock = -1, BenchSeconds = 0, FeederSeconds = 0, Settle = 0, ServiceAz = 0, ServiceEl = 0;
 FVector Dropped = FVector::ZeroVector;
 FS47Obj Bench, Depot, BenchMeter, BenchLamp, BenchSample, Shop, Panel, Lid, Relay, Selector, Generator, FeederPanel, FeederLever, FeederLamp;
 FS47Obj ParcelSaro, ParcelRoad, CargoSaro, CargoRoad;
 TArray<FS47Obj> Parts;
 struct FOverride { TFunction<FString()> Label, Examine; TFunction<void()> Use; };
 TMap<FString,FOverride> Overrides;
 void OverrideExisting();
 bool Build();
 FS47Obj Box(const FString& Name, const FVector& At, const FVector& Size, uint32 Colour, int32 Parent, const FString& Id = FString());
 FS47Obj Model(const FString& Name, const TCHAR* Asset, const FVector& At, double Width, int32 Parent);
 void Bind(FS47Obj Object, const FString& Id, double Range = 2.6);
 void Apply();
 bool Near(const FVector& At, double Range = 3.0) const;
 bool Ready() const;
 int32 S03() const;
 FVector ShopPoint() const;
 FVector TruckPoint() const;
 FVector PumpPoint() const;
 bool PumpConditions() const;
 bool AtDiesel() const;
 void StepPump(double Dt);
 void ApplyPump();
 void Change(const FString& Message = FString());
 bool ServiceUse();
 bool FeederUse();
 bool GeneratorUse();
 bool ParcelUse();
 bool CargoUse();
 bool Deposit();
 void DonorTest();
 void RefuelTruck();
 void RoadsideAssist();
};

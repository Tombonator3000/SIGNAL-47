#pragma once
#include "CoreMinimal.h"
#include "S47Obj.h"
class AS47Web;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
struct FS47Truck;
/** Production C10 art follows the existing truck controller and live instrument nodes. */
class SIGNAL47_API FS47Fleet
{
public:
 static TUniquePtr<FS47Fleet> Install(AS47Web* W,FS47Truck& Truck);
 void Wheels(double Travel,double Steer,const double Drops[4]);
 bool Audit(TArray<FString>& Lines) const;
 void Lenses(const FLinearColor& Color,double Head,double Tail);
 bool Ready() const {return bReady;}
 static FVector Headlamp(int32 Side){return Side==0?FVector(-.720,.684,-2.073):FVector(.728,.687,-2.070);}
private:
 bool bReady=false;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> WheelParts;
 TArray<FVector> WheelAt;
 TArray<double> WheelRadius;
 TArray<FTransform> ExpectedWheels;
 TWeakObjectPtr<AS47Web> Web;
 TArray<int32> RetiredNodes;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> NativeParts;
 FLinearColor ExpectedHead=FLinearColor::Black,ExpectedTail=FLinearColor::Black;
 TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> HeadMids,TailMids;
};
/** Atomic replacement of the two staff-bay primitives; root owns the single Web hook. */
class SIGNAL47_API FS47FleetParked
{
public:
 static TSharedPtr<FS47FleetParked> Build(AS47Web* Web);
 bool Ready()const{return bReady;}
 int32 Parts()const{return Components.Num();}
 bool Audit(TArray<FString>& Lines)const;
private:
 bool bReady=false;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Components;
 TWeakObjectPtr<UStaticMeshComponent> TrimmedDressing;
 TWeakObjectPtr<class UMaterialInterface> DressingMaterial;
};

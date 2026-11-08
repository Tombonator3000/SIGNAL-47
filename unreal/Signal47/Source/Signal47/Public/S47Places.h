#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
class US47Main;
class AS47Web;
class UStaticMeshComponent;
struct FS47MapView;

// Freeform only: somewhere to drive to (Tom, 7 November). Small places out on the land, each with one thing to find, and a place
// lights can stand over. Simple props from basic shapes where the project has no model; they want real models later. Heard
// about on the radio, or found: a pencil mark on the notebook's map. A mended branch of the array gives a bearing, a sector on
// the same map: which way, never what. Also where a cot can be put up, and what the magnetometer reads.
class FS47Places
{
public:
 explicit FS47Places(US47Main* Main):M(Main){}
 struct FDef{const TCHAR* Id;const TCHAR* Title;const TCHAR* Kind;double X,Z;const TCHAR* Finding;const TCHAR* Needle;};
 static const TArray<FDef>& Defs();
 static const FDef* Def(const FString& Id);
 static FString UseId(const FString& Id){return TEXT("ff:place:")+Id;}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Found(const FString& Id) const{return bHasFound(Id);}
 bool Heard(const FString& Id) const{return HeardOf.Contains(Id);}
 void Hear(const FString& Id);
 // The first place the player has neither found nor heard of, by this game's seed; empty when there is none.
 FString NextUnknown(int32 Salt) const;
 bool Use(const FString& Id);
 void FillMap(FS47MapView& View) const;
 // Registered work with mended branches gives one bearing a night. True if it gave one.
 bool AfterRegistered();
 struct FBearing{int32 Day=0;double Az=0,Spread=0;FString Target;};
 const TArray<FBearing>& Bearings() const{return Fixes;}
 static double SpreadFor(int32 Branches){return Branches<=0?0.:Branches==1?20.:Branches==2?12.:Branches==3?7.:4.;}
 static double AzimuthOf(double FromX,double FromZ,double ToX,double ToZ); // Degrees, north 0, east 90. Road-local: z runs south.
 // Some small things are not where they were: metres from where they first stood, following from the game and the day, so a
 // Continue changes nothing. Only what could be carried. Only moved while nobody is near.
 static FVector2D DriftOf(const FString& Id,int32 Seed,int32 Day);
 static bool Drifts(const FString& Id);
 // Where the place really stands, road-local, after the land has had its say.
 bool Spot(const FString& Id,double& OutX,double& OutZ) const;
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 struct FBuilt{FString Id;double X=0,Z=0,Y=0;bool bSeated=false;FVector2D Shift=FVector2D::ZeroVector;int32 ShiftDay=0;TArray<TWeakObjectPtr<UStaticMeshComponent>> Parts;TArray<FVector> Local;FS47Obj Hit;};
 TArray<FBuilt> Built;
 TSet<FString> HeardOf;
 TArray<FBearing> Fixes;
 TSharedPtr<uint8> Life;
 TWeakObjectPtr<AS47Web> Scene;
 bool bBuilt=false;
 double NeedleIn=0;
 int32 LastFixDay=0;
 bool bHasFound(const FString& Id) const;
 bool Build();
 void Seat(FBuilt& B);
 void StepNeedle(double Dt);
 void Panel(const FDef& D,bool bFirst,const FString& Extra);
};

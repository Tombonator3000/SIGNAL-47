#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Docs.h"
#include "S47Obj.h"
#include "S47PaleBody.h"
class US47Main;
class UPointLightComponent;
class SVerticalBox;
struct FS47Task;

// New nights only: Earl's original isolated reel, the operator/reference comparison,
// then one reactive meeting among the existing Row One pedestals. No new audio masters.
class FS47SC08
{
 friend class FS47SC08Contract;
public:
 enum class EStep:uint8 { Earl, Array, Compare, Hunt, Hold, Done };
 enum class EBody:uint8 { Approach, Search, Chase, Grab, Leave };
 explicit FS47SC08(US47Main* InM):M(InM){}
 ~FS47SC08();
 void Begin(const S47J::FObj& Saved,bool Active);
 void Stop();
 bool Step(double Dt);
 S47J::FObj Save() const;
 S47J::FObj Readback() const;
 bool Busy() const{return bActive&&P!=EStep::Done;}
 bool Use(const FString& Id);
 FString Label(const FString& Id) const;
 // The ordinary panel's button callbacks call exactly this method.
 void Choice(const FString& Id);
 FString Objective() const;
 TArray<FS47Task> Tasks() const;
 EStep Phase() const{return P;}
 bool HuntLive() const{return bActive&&P==EStep::Hunt;}
 bool SourcesReady() const;
 static FVector2D Point(const FString& Id);
 static FVector2D Bearing(double DishX,double DishZ,double Scale,const FVector& Eye);
 struct FNode{const TCHAR* Id;FVector2D At;};
 static const TArray<FNode>& Nodes();
 static const TArray<TPair<int32,int32>>& Edges();
 static bool CanAdoptSC07(const S47J::FObj& State);
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 EStep P=EStep::Earl;
 EBody Behavior=EBody::Approach;
 bool bActive=false,bBuilt=false,bDialog=false,bPlaying=false,bUnavailable=false,bRefRecorded=false,bIsolated=false,bLabelRead=false,bCompared=false,bEscaped=false,bVoiced=false;
 int32 Epoch=0,Plays=0,Captures=0,Patrol=0;
 double T=0,StateT=0,Lost=0,Noise=0,Still=0,HoldT=0,FadeT=-1,BodyYaw=0,Gait=0,Stride=0;
 FString First,Dialogue,LastTape;
 double EscapedAt=0;
 FVector RefAt=FVector::ZeroVector,LastFieldEye=FVector::ZeroVector,LastPlayer=FVector::ZeroVector;
 FVector2D Pos=FVector2D(-1,-88),LastKnown=FVector2D::ZeroVector;
 FTransform RefTransform=FTransform::Identity;
 bool bHavePlayer=false,bShown=false,bResumeHunt=false;
 TArray<int32> Dishes,Path;
 TArray<S47J::FObj> Samples;
 TArray<FS47Doc> SourcePapers;
 S47J::FObj LogBaseline,AppliedRestore;
 TArray<TArray<double>> RestoredPointings;
 FS47PaleBody Pale;
 FS47Obj Boards[3],RefLamp,Isolator;
 TWeakObjectPtr<UPointLightComponent> RefLight;
 TSharedPtr<SVerticalBox> Body;
 TFunction<FString()> OldEarlLabel;
 TMap<FString,TFunction<FString()>> OldTruckLabels;
 void Build();
 void Refile();
 void KeepSources();
 void File(const FS47Doc& Doc);
 void Open(const FString& Panel);
 void Refresh();
 void Play();
 bool AtPoint(const FString& Id) const;
 bool FixedReference() const;
 bool NoCommand() const;
 bool Settled() const;
 void Turn(double Dt,const FVector& Eye,bool Home=false);
 void Record(const FString& Id);
 void StartHunt(bool Retry);
 void Hunt(double Dt);
 bool ClearWalk(const FVector2D& From,const FVector2D& To) const;
 bool ClearSight(const FVector& From,const FVector& To) const;
 bool SeesPlayer() const;
 int32 Nearest(const FVector2D& At) const;
 void RouteTo(int32 Goal);
 bool WalkBody(double Speed,double Dt);
 void Enter(EBody Next);
 void Capture();
 void Escape();
 double Ground(const FVector2D& At) const;
};

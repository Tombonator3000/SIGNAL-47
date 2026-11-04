#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
struct FS47AlbumShot;
// Explicit isolated integration driver. Fixture placement is reported; not human acceptance.
class FS47FreeformTest
{
public:
 explicit FS47FreeformTest(US47Main* Main):M(Main){}
 void Step(double Dt);
private:
 friend class FS47CaseFieldTest;
 TSharedPtr<class FS47CaseFieldTest> CaseFieldTest;
 void StepDuskOrb(double Dt);
 US47Main* M=nullptr;
 bool bInit=false,bDone=false,bAudioMarked=false,bFieldResume=false,bCarRoundTrip=false;
 int32 Stage=0,Checks=0,Restores=0,JobIndex=0,EventIndex=0,DayRuns=0,TripHoldSamples=0;
 double Age=0,At=0,Hold=0,InitialAz=0,InitialEl=0,PausedAz=0,PausedEl=0;
 FString Root,PhotoClaimId,RetainedPhotoId,RetainedPhotoUrl,PhotoEvent;
 int32 PhotoFigureId=-1,CameraShots=0,CameraFilm=0;
 int32 OpsNavStep=0;
 int32 ServiceTrial=0,ServiceBefore=0,ServiceRelays=0;
 bool ServiceMoved=false;
 FString ServiceTarget;
 TWeakPtr<class SWidget> OpsModal;
 int32 ShopCash=0,ShopFuses=0,ShopRelays=0,ShopDelivered=0,ShopCargo=0,ShopClick=0;
 int32 PhotoBudget=0,PrinterBudget=0,WorkBudget=0;
 double PrinterHeld=0;
 FString PrinterId,PrinterOriginal,PrinterStamp,GuidanceExpected,TutorialId;
 S47J::FObj Snapshot;
 TArray<TSharedPtr<FJsonValue>> Evidence;
 bool Check(bool Good,const FString& Text);
 void Go(int32 Next,double Delay=0);
 bool Restore(const FString& Name);
 bool FileAtBinder();
 void PoseForEvent();
 bool PosePhotoFigure();
 void LogPhotoFrame(const FString& Label,const FS47AlbumShot& Photo);
 void Shot(const FString& Name);
 void Finish(bool Good,const FString& Why=FString());
};

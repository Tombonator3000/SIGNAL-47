#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"

// Loaned passive RF noise logger. It measures local electrical interference,
// never copies a SARO original or claims an independently detected sky signal.
class FS47FieldLogger
{
public:
 struct FTape
 {
  FString Id,Job;
  TArray<double> Levels; // one retained demodulated noise envelope per simulated minute
  int32 Site=0,Day=0;
  double Start=0,End=0,Frequency=1420.11,Offset=0,Battery=0,Overlap=0,NoiseSum=0,Seconds=0,GapSeconds=0;
  bool bRetrieved=false,bCompared=false,bBatteryEmpty=false;
 };
 int32 Assembly=0,Site=-1,Serial=0;
 bool bCalibrated=false,bRecording=false;
 // First issue is at parts storage; all subsequent handling is at the bench.
 bool bLoanIssued=false,bCheckedOut=false;
 int32 Checkouts=0,Returns=0;
 double Battery=0,Offset=2.4,Frequency=1420.11,Calibration=0;
 TArray<FTape> Tapes;
 static constexpr double Capacity=6*3600., Window=2*3600.;
 bool Connect();
 bool Calibrate(double Seconds);
 bool Start(int32 At,int32 Day,double Clock);
 void Sample(double Clock,double Delta,bool Generator,bool FeederFault,const FString& Job,double AcquiredDelta,double JobFrequency);
 void Finish(double Clock);
 bool Retrieve();
 FString Report(const FTape& T) const;
 static bool CompleteWindow(const FTape& T) { return T.bRetrieved&&T.Overlap>=5.&&T.Seconds>=Window-.001&&T.GapSeconds==0&&!T.bBatteryEmpty; }
 S47J::FObj Save() const;
 void Load(const S47J::FObj& O);
 static bool SelfTest(TArray<FString>& Lines);
};

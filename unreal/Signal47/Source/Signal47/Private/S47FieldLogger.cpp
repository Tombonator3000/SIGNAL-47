#include "S47FieldLogger.h"
#include "S47FreeformCases.h"

bool FS47FieldLogger::Connect()
{
 if(Assembly>=3||Site>=0)return false;
 ++Assembly;bCalibrated=false;Calibration=0;if(Assembly==3)Battery=Capacity;return true;
}
bool FS47FieldLogger::Calibrate(double Seconds)
{
 if(Assembly!=3||Site>=0||!FMath::IsFinite(Seconds)||Seconds<=0)return false;
 Calibration=FMath::Min(3.,Calibration+Seconds);
 if(Calibration>=3){bCalibrated=true;Battery=Capacity;return true;}return false;
}
bool FS47FieldLogger::Start(int32 At,int32 Day,double Clock)
{
 if(!bCalibrated||Assembly!=3||Site>=0||bRecording||Battery<=0||At<0||At>1||Tapes.Num()>=256||!FMath::IsFinite(Clock)||Clock<0)return false;
 Site=At;bRecording=true;FTape T;T.Id=FString::Printf(TEXT("fieldlogger01.tape%d"),++Serial);while(Tapes.ContainsByPredicate([&](const FTape& Existing){return Existing.Id==T.Id;}))T.Id=FString::Printf(TEXT("fieldlogger01.tape%d"),++Serial);T.Site=At;T.Day=Day;T.Start=T.End=Clock;T.Frequency=Frequency;T.Offset=Offset;T.Battery=Battery;Tapes.Add(T);return true;
}
void FS47FieldLogger::Sample(double Clock,double Delta,bool Generator,bool FeederFault,const FString& Job,double AcquiredDelta,double JobFrequency)
{
 if(!bRecording||Tapes.IsEmpty()||!FMath::IsFinite(Clock)||!FMath::IsFinite(Delta)||Delta<=0)return;
 auto& T=Tapes.Last();const double D=FMath::Max(0.,FMath::Min3(Delta,Battery,Window-(T.End-T.Start)));
 if(Delta>3.){Skip(Clock,Delta);return;}
 // Local oscillator band at 1420.110 MHz. Site A is beside SARO's
 // generator, site B is the quiet array edge. Hardware state drives noise.
 const double Band=FMath::Exp(-FMath::Square((Frequency-1420.11)/.025));
 const double Local=(Generator?(Site==0?.8:.08):0)+(FeederFault?(Site==0?.25:.04):0);
 if(D>0&&(T.Levels.IsEmpty()||int32((T.Seconds+D)/60)>int32(T.Seconds/60)))T.Levels.Add(.02+Band*Local);
 T.NoiseSum+=D*(.02+Band*Local);T.Seconds+=D;Battery=FMath::Max(0.,Battery-D);T.End=FMath::Min(Clock,T.End+D);
 if(D>=Delta-.001&&AcquiredDelta>0&&!Job.IsEmpty()&&FMath::Abs(Frequency-JobFrequency)<=.001)
 {
  // Overlap is accumulated from frames in which the SARO original actually
  // advanced, not from an estimated end-time or a previously recorded tape.
  if(T.Job.IsEmpty())T.Job=Job;
  if(T.Job==Job)T.Overlap+=AcquiredDelta;
 }
 if(Battery<=0){T.bBatteryEmpty=true;bRecording=false;}
 if(T.End-T.Start>=Window-.001)bRecording=false;
}
void FS47FieldLogger::Skip(double Clock,double Delta)
{
 if(!bRecording||Tapes.IsEmpty()||!FMath::IsFinite(Clock)||!FMath::IsFinite(Delta)||Delta<=0)return;
 auto& T=Tapes.Last();const double D=FMath::Max(0.,FMath::Min3(Delta,Battery,Window-(T.End-T.Start)));
 // Clock movement is battery usage, not a historical measurement or a negative observation.
 T.GapSeconds+=D;T.End=FMath::Min(Clock,T.End+D);Battery=FMath::Max(0.,Battery-D);
 if(Battery<=0){T.bBatteryEmpty=true;bRecording=false;}
 if(T.End-T.Start>=Window-.001)bRecording=false;
}
void FS47FieldLogger::Finish(double Clock)
{
 if(!bRecording||Tapes.IsEmpty())return;
 Tapes.Last().End=FMath::Max(Tapes.Last().Start,FMath::Min(Clock,Tapes.Last().End));bRecording=false;
}
bool FS47FieldLogger::Retrieve()
{
 if(Site<0||Tapes.IsEmpty())return false;
 bRecording=false;Tapes.Last().bRetrieved=true;Site=-1;return true;
}
FString FS47FieldLogger::Report(const FTape& T)const
{
 return FString::Printf(TEXT("SARO LOAN LOGGER / DEVICE fieldlogger01\nORIGINAL %s / SITE %s\nDAY %d / %s to %s / %.3f MHz\nCLOCK READBACK: logger %+.1f s ahead of SARO; correction retained\nSAMPLED %.0f s / UNSAMPLED GAP %.0f s / LOCAL NOISE %.3f\nBATTERY: %s\nSIMULTANEOUS SARO ORIGINAL: %s / %.2f acquisition seconds\n\nLocal electrical-noise monitor only. A quiet record does not rule out a visit or a sky signal. This instrument does not copy SARO pulse traces. No origin is established."),*T.Id,T.Site==0?TEXT("A / generator service path"):TEXT("B / array east edge"),T.Day,*FS47FreeformCases::Clock(T.Start),*FS47FreeformCases::Clock(T.End),T.Frequency,T.Offset,T.Seconds,T.GapSeconds,T.Seconds>0?T.NoiseSum/T.Seconds:0,T.bBatteryEmpty?TEXT("EXHAUSTED / incomplete window"):TEXT("available at stop"),T.Job.IsEmpty()?TEXT("none at matching frequency"):*T.Job,T.Overlap);
}
S47J::FObj FS47FieldLogger::Save()const
{
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("loanIssued"),bLoanIssued);S47J::Set(O,TEXT("checkedOut"),bCheckedOut);S47J::Set(O,TEXT("checkouts"),Checkouts);S47J::Set(O,TEXT("returns"),Returns);S47J::Set(O,TEXT("assembly"),Assembly);S47J::Set(O,TEXT("site"),Site);S47J::Set(O,TEXT("serial"),Serial);S47J::Set(O,TEXT("calibrated"),bCalibrated);S47J::Set(O,TEXT("recording"),bRecording);S47J::Set(O,TEXT("battery"),Battery);S47J::Set(O,TEXT("frequency"),Frequency);S47J::Set(O,TEXT("calibration"),Calibration);
 TArray<TSharedPtr<FJsonValue>> A;for(const auto& T:Tapes){auto V=S47J::New();S47J::Set(V,TEXT("id"),T.Id);S47J::Set(V,TEXT("levels"),T.Levels);S47J::Set(V,TEXT("job"),T.Job);S47J::Set(V,TEXT("site"),T.Site);S47J::Set(V,TEXT("day"),T.Day);S47J::Set(V,TEXT("start"),T.Start);S47J::Set(V,TEXT("end"),T.End);S47J::Set(V,TEXT("frequency"),T.Frequency);S47J::Set(V,TEXT("offset"),T.Offset);S47J::Set(V,TEXT("battery"),T.Battery);S47J::Set(V,TEXT("overlap"),T.Overlap);S47J::Set(V,TEXT("noise"),T.NoiseSum);S47J::Set(V,TEXT("seconds"),T.Seconds);S47J::Set(V,TEXT("gapSeconds"),T.GapSeconds);S47J::Set(V,TEXT("retrieved"),T.bRetrieved);S47J::Set(V,TEXT("compared"),T.bCompared);S47J::Set(V,TEXT("empty"),T.bBatteryEmpty);A.Add(MakeShared<FJsonValueObject>(V));}O->SetArrayField(TEXT("tapes"),A);return O;
}
void FS47FieldLogger::Load(const S47J::FObj& O)
{
 *this=FS47FieldLogger();if(!O||S47J::Int(O,TEXT("v"))!=1)return;
 auto N=[](const S47J::FObj& V,const TCHAR* K,double Min,double Max,double Default=0.){const double X=S47J::Num(V,K,Default);return FMath::IsFinite(X)?FMath::Clamp(X,Min,Max):Default;};
 Assembly=int32(N(O,TEXT("assembly"),0,3));Site=int32(N(O,TEXT("site"),-1,1,-1));Serial=int32(N(O,TEXT("serial"),0,1000000));Battery=N(O,TEXT("battery"),0,Capacity);Frequency=N(O,TEXT("frequency"),1419.5,1420.7,1420.11);Calibration=N(O,TEXT("calibration"),0,3);bCalibrated=Assembly==3&&Calibration>=3&&S47J::Bool(O,TEXT("calibrated"));bRecording=bCalibrated&&Site>=0&&Battery>0&&S47J::Bool(O,TEXT("recording"));
 for(const auto& V:S47J::Arr(O,TEXT("tapes")))
 {
  const S47J::FObj* P=nullptr;if(!V.IsValid()||!V->TryGetObject(P)||!P||Tapes.Num()>=256)continue;const auto& Q=*P;FTape T;T.Id=S47J::Str(Q,TEXT("id"));if(T.Id.IsEmpty()||Tapes.ContainsByPredicate([&](const FTape& A){return A.Id==T.Id;}))continue;
  T.Levels=S47J::Nums(Q,TEXT("levels"));T.Levels.RemoveAll([](double X){return !FMath::IsFinite(X)||X<0||X>2;});if(T.Levels.Num()>121)T.Levels.SetNum(121);
  if(T.Id.StartsWith(TEXT("fieldlogger01.tape")))Serial=FMath::Max(Serial,FMath::Clamp(FCString::Atoi(*T.Id.Mid(18)),0,1000000));
  T.Job=S47J::Str(Q,TEXT("job"));T.Site=int32(N(Q,TEXT("site"),0,1));T.Day=int32(N(Q,TEXT("day"),1,1000000,1));T.Start=N(Q,TEXT("start"),0,1e12);T.End=N(Q,TEXT("end"),T.Start,T.Start+Window,T.Start);T.Frequency=N(Q,TEXT("frequency"),1419.5,1420.7,1420.11);T.Offset=N(Q,TEXT("offset"),-60,60);T.Battery=N(Q,TEXT("battery"),0,Capacity);T.Overlap=N(Q,TEXT("overlap"),0,Window);T.Seconds=N(Q,TEXT("seconds"),0,Window);T.GapSeconds=N(Q,TEXT("gapSeconds"),0,Window);T.NoiseSum=N(Q,TEXT("noise"),0,Window*2);T.bRetrieved=S47J::Bool(Q,TEXT("retrieved"));T.bCompared=T.bRetrieved&&S47J::Bool(Q,TEXT("compared"));T.bBatteryEmpty=S47J::Bool(Q,TEXT("empty"));Tapes.Add(T);
 }
 bLoanIssued=S47J::Bool(O,TEXT("loanIssued"),Assembly>0||!Tapes.IsEmpty());
 bCheckedOut=bLoanIssued&&S47J::Bool(O,TEXT("checkedOut"),Site>=0);
 Checkouts=int32(N(O,TEXT("checkouts"),0,1000000,bLoanIssued?1:0));Returns=int32(N(O,TEXT("returns"),0,Checkouts));
 Serial=FMath::Max(Serial,Tapes.Num());if(Tapes.IsEmpty()||Tapes.Last().bRetrieved){Site=-1;bRecording=false;}else if(Tapes.Last().End-Tapes.Last().Start>=Window-.001)bRecording=false;
}
bool FS47FieldLogger::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;auto C=[&](bool B,const TCHAR* Text){OK&=B;Lines.Add(FString(B?TEXT("PASS "):TEXT("FAIL "))+TEXT("field logger: ")+Text);};
 FS47FieldLogger L;C(!L.Start(0,1,100),TEXT("unassembled loan cannot record"));L.Connect();L.Connect();L.Connect();C(!L.Start(0,1,100),TEXT("calibration required after physical connections"));L.Calibrate(3);L.Start(0,1,100);for(int32 I=1;I<=60;++I)L.Sample(100+I,1,true,false,TEXT("job1"),1./60.,1420.11);for(int32 I=61;I<=120;++I)L.Sample(100+I,1,true,false,TEXT("old"),0,1420.11);
 FS47FieldLogger R;R.Load(S47J::Read(S47J::Write(L.Save())));C(R.Site==0&&R.bRecording&&R.Tapes.Num()==1&&FMath::IsNearlyEqual(R.Tapes[0].Overlap,1.,.00001)&&R.Battery==Capacity-120,TEXT("Continue preserves identity, battery and real acquisition overlap"));
 R.Retrieve();C(!R.Retrieve()&&R.Tapes[0].bRetrieved,TEXT("original retrieval is once per deployment"));R.Start(1,1,300);for(int32 I=1;I<=60;++I)R.Sample(300+I,1,true,false,TEXT("wrong-band"),1./60.,1420.4);C(R.Tapes[1].NoiseSum/R.Tapes[1].Seconds<L.Tapes[0].NoiseSum/L.Tapes[0].Seconds&&R.Tapes[1].Overlap==0,TEXT("distance changes local interference and mismatched frequency cannot compare"));R.Battery=1;R.Sample(362,2,false,false,TEXT("job"),1,1420.11);C(!R.bRecording&&R.Tapes.Last().bBatteryEmpty&&R.Tapes.Last().Overlap==0,TEXT("empty battery is incomplete, never a negative observation"));
 auto Sparse=R.Save();S47J::Set(Sparse,TEXT("serial"),0);auto Entries=S47J::Arr(Sparse,TEXT("tapes"));S47J::Set(Entries.Last()->AsObject(),TEXT("id"),TEXT("fieldlogger01.tape88"));S47J::Set(Entries.Last()->AsObject(),TEXT("retrieved"),true);Sparse->SetArrayField(TEXT("tapes"),Entries);FS47FieldLogger SparseCopy;SparseCopy.Load(Sparse);SparseCopy.Calibrate(3);SparseCopy.Start(0,2,600);C(SparseCopy.Tapes.Last().Id==TEXT("fieldlogger01.tape89"),TEXT("sparse retained tape IDs cannot be reused after restore"));
 FS47FreeformCases Access;Access.Reset(47);C(!Access.HasAccess(TEXT("access.fieldlogger")),TEXT("new case does not authorize the loan"));auto Granted=Access.Save();S47J::Set(Granted,TEXT("access"),TArray<FString>{TEXT("access.fieldlogger")});Access.Load(Granted,47);C(Access.HasAccess(TEXT("access.fieldlogger")),TEXT("supported-case access survives restore for the world gate"));
 FS47FieldLogger Gap;Gap.Connect();Gap.Connect();Gap.Connect();Gap.Calibrate(3);Gap.Start(0,1,100);Gap.Sample(100+8*3600.,8*3600.,true,true,TEXT("old-job"),18,1420.11);C(!Gap.bRecording&&Gap.Tapes.Last().GapSeconds==Window&&Gap.Tapes.Last().Seconds==0&&Gap.Tapes.Last().NoiseSum==0&&Gap.Tapes.Last().Overlap==0&&Gap.Tapes.Last().Levels.IsEmpty()&&Gap.Battery==Capacity-Window,TEXT("sleep jump drains the planned window without invented historical samples or overlap"));FS47FieldLogger GapCopy;GapCopy.Load(S47J::Read(S47J::Write(Gap.Save())));C(GapCopy.Tapes.Last().GapSeconds==Window&&!GapCopy.bRecording,TEXT("unsampled gaps survive Continue"));
 FS47FieldLogger Full;Full.Connect();Full.Connect();Full.Connect();Full.Calibrate(3);Full.Start(1,1,100);Full.Sample(101,1,false,false,TEXT("job"),6,1420.11);Full.Retrieve();C(!CompleteWindow(Full.Tapes.Last()),TEXT("brief retrieval with overlap is an incomplete field window"));Full.Start(1,1,200);for(int32 I=1;I<=7200;++I)Full.Sample(200+I,1,false,false,TEXT("job"),I<11?.6:0,1420.11);Full.Retrieve();C(CompleteWindow(Full.Tapes.Last()),TEXT("full sampled window and substantial actual overlap qualify after retrieval"));
 FS47FieldLogger ShortGap;ShortGap.Connect();ShortGap.Connect();ShortGap.Connect();ShortGap.Calibrate(3);ShortGap.Start(0,1,100);ShortGap.Skip(102,2);
 C(ShortGap.Tapes.Last().GapSeconds==2&&ShortGap.Tapes.Last().Seconds==0&&ShortGap.Tapes.Last().Overlap==0&&ShortGap.Battery==Capacity-2,TEXT("explicit short unobserved skip never masquerades as a normal sampled tick"));
 FS47FieldLogger Legacy;Legacy.Load(nullptr);C(Legacy.Assembly==0&&Legacy.Tapes.IsEmpty(),TEXT("old saves invent no equipment or originals"));return OK;
}

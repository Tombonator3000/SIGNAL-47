#include "S47FreeformCases.h"

namespace
{
 uint32 Mix(uint32 V){V^=V>>16;V*=0x7feb352du;V^=V>>15;V*=0x846ca68bu;return V^(V>>16);}
 double DayDelta(double A,double B)
 {
  const double D=FMath::Abs(FMath::Fmod(A-B,86400.));return FMath::Min(D,86400.-D);
 }
 const TCHAR* CauseKeys[]={TEXT("feeder"),TEXT("transmitter"),TEXT("sky")};
}

FS47FreeformCases::ECause FS47FreeformCases::Seeded(int32 Seed,int32 Line)
{
 return ECause(Mix(uint32(Seed)^Mix(0x43415345u+uint32(Line)))%uint32(CauseCount));
}
void FS47FreeformCases::Reset(int32 Seed)
{
 SeedValue=Seed;Cases.SetNum(CaseCount);Access.Reset();
 for(int32 I=0;I<CaseCount;++I){Cases[I]=FCase();Cases[I].Truth=Seeded(Seed,I);}
}
void FS47FreeformCases::Load(const S47J::FObj& Saved,int32 Seed)
{
 Reset(Seed);if(!Saved||S47J::Int(Saved,TEXT("v"))!=1)return;
 for(const FString& A:S47J::Strs(Saved,TEXT("access")))if(!A.IsEmpty()&&A.Len()<64)Access.Add(A);
 const auto List=S47J::Arr(Saved,TEXT("cases"));
 for(int32 I=0;I<FMath::Min(CaseCount,List.Num());++I)
 {
  if(!List[I]||List[I]->Type!=EJson::Object)continue;const auto O=List[I]->AsObject();FCase& C=Cases[I];
  C.bOpen=S47J::Bool(O,TEXT("open"));C.bConcluded=C.bOpen&&S47J::Bool(O,TEXT("concluded"));
  C.OpenDay=FMath::Clamp(S47J::Int(O,TEXT("openDay")),0,36500);
  const double Clock=S47J::Num(O,TEXT("openClock"));C.OpenClock=FMath::IsFinite(Clock)?FMath::Clamp(Clock,0.,86399.):0;
  C.Planned=FMath::Clamp(S47J::Int(O,TEXT("planned"),-1),-1,TestCount-1);C.PlannedJob=C.Planned>=0?S47J::Str(O,TEXT("plannedJob")):FString();
  if(C.PlannedJob.IsEmpty())C.Planned=-1;
  C.Conclusion=C.bConcluded?FMath::Clamp(S47J::Int(O,TEXT("conclusion"),-1),-1,CauseCount-1):-1;
  C.ValidTests=FMath::Clamp(S47J::Int(O,TEXT("validTests")),0,1000);
  // The cause is never trusted from disk: it is a pure function of the session seed.
  const auto R=S47J::Arr(O,TEXT("results"));
  for(int32 T=0;T<FMath::Min(TestCount,R.Num());++T)if(R[T]&&R[T]->Type==EJson::Object)
  {
   const auto X=R[T]->AsObject();auto& Out=C.Results[T];Out.bDone=S47J::Bool(X,TEXT("done"));
   Out.Day=FMath::Clamp(S47J::Int(X,TEXT("day")),0,36500);
   // A recorded result always agrees with the physical rule for the seeded cause.
   Out.bTicks=Out.bDone&&TicksFor(C.Truth,ETest(T));
  }
  if(C.bConcluded&&C.Conclusion<0)C.bConcluded=false;
 }
 // A saved conclusion must still be supported by its retained tests.
 for(int32 I=0;I<Cases.Num();++I)if(Cases[I].bConcluded&&(Excluded(I,ECause(Cases[I].Conclusion))||Remaining(I)!=1)){Cases[I].bConcluded=false;Cases[I].Conclusion=-1;}
}
S47J::FObj FS47FreeformCases::Save() const
{
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);
 TArray<FString> A=Access.Array();A.Sort();S47J::Set(O,TEXT("access"),A);
 TArray<TSharedPtr<FJsonValue>> List;
 for(const FCase& C:Cases)
 {
  auto X=S47J::New();S47J::Set(X,TEXT("open"),C.bOpen);S47J::Set(X,TEXT("concluded"),C.bConcluded);S47J::Set(X,TEXT("openDay"),C.OpenDay);S47J::Set(X,TEXT("openClock"),C.OpenClock);
  S47J::Set(X,TEXT("planned"),C.Planned);S47J::Set(X,TEXT("plannedJob"),C.PlannedJob);S47J::Set(X,TEXT("conclusion"),C.Conclusion);S47J::Set(X,TEXT("validTests"),C.ValidTests);
  TArray<TSharedPtr<FJsonValue>> R;
  for(const FResult& Res:C.Results){auto Y=S47J::New();S47J::Set(Y,TEXT("done"),Res.bDone);S47J::Set(Y,TEXT("ticks"),Res.bTicks);S47J::Set(Y,TEXT("day"),Res.Day);R.Add(MakeShared<FJsonValueObject>(Y));}
  X->SetArrayField(TEXT("results"),R);List.Add(MakeShared<FJsonValueObject>(X));
 }
 O->SetArrayField(TEXT("cases"),List);return O;
}
bool FS47FreeformCases::Open(int32 Line,int32 Day,double DayClock)
{
 if(!Cases.IsValidIndex(Line)||Cases[Line].bOpen||Day<1||!FMath::IsFinite(DayClock))return false;
 FCase& C=Cases[Line];C.bOpen=true;C.OpenDay=Day;C.OpenClock=FMath::Clamp(FMath::Fmod(FMath::Max(0.,DayClock),86400.),0.,86399.);return true;
}
bool FS47FreeformCases::Plan(int32 Line,ETest Test,const FString& JobId)
{
 if(!Cases.IsValidIndex(Line)||JobId.IsEmpty())return false;FCase& C=Cases[Line];
 const int32 T=int32(Test);if(!C.bOpen||C.bConcluded||C.Planned>=0||T<0||T>=TestCount||C.Results[T].bDone)return false;
 C.Planned=T;C.PlannedJob=JobId;return true;
}
void FS47FreeformCases::CancelPlan(int32 Line){if(Cases.IsValidIndex(Line)){Cases[Line].Planned=-1;Cases[Line].PlannedJob.Empty();}}
bool FS47FreeformCases::Record(int32 Line,ETest Test,int32 Day,bool bValid,bool bTicks,bool& bFirstValid)
{
 bFirstValid=false;if(!Cases.IsValidIndex(Line))return false;FCase& C=Cases[Line];const int32 T=int32(Test);
 if(!C.bOpen||C.Planned!=T)return false;
 C.Planned=-1;C.PlannedJob.Empty();
 if(!bValid)return true; // the plan is released so the test can be repeated properly
 // The physical recording is the authority; it must agree with the seeded cause.
 C.Results[T].bDone=true;C.Results[T].bTicks=bTicks;C.Results[T].Day=Day;
 bFirstValid=C.ValidTests==0;++C.ValidTests;return true;
}
bool FS47FreeformCases::TicksFor(ECause Cause,ETest Test)
{
 switch(Test)
 {
  case ETest::Generator:return Cause!=ECause::Feeder;          // feeder-borne ticks vanish on generator power
  case ETest::OffSource:return Cause!=ECause::Sky;             // a sky-fixed source vanishes off the beam
  case ETest::Repeat:return Cause!=ECause::LocalTransmitter;   // the intermittent local carrier does not recur
 }
 return true;
}
bool FS47FreeformCases::Excluded(int32 Line,ECause Cause) const
{
 const FCase* C=Get(Line);if(!C)return false;
 for(int32 T=0;T<TestCount;++T)if(C->Results[T].bDone&&TicksFor(Cause,ETest(T))!=C->Results[T].bTicks)return true;
 return false;
}
int32 FS47FreeformCases::Remaining(int32 Line) const
{
 int32 N=0;for(int32 I=0;I<CauseCount;++I)N+=Excluded(Line,ECause(I))?0:1;return N;
}
bool FS47FreeformCases::Conclude(int32 Line,ECause Cause,FString& Why)
{
 if(!Cases.IsValidIndex(Line)||!Cases[Line].bOpen){Why=TEXT("No open case.");return false;}
 FCase& C=Cases[Line];if(C.bConcluded){Why=TEXT("This case is already concluded; its record is retained.");return false;}
 if(Excluded(Line,Cause)){Why=CauseName(Cause)+TEXT(" is contradicted by your own retained test. Read the result again.");return false;}
 TArray<FString> Open;for(int32 I=0;I<CauseCount;++I)if(I!=int32(Cause)&&!Excluded(Line,ECause(I)))Open.Add(CauseName(ECause(I)));
 if(!Open.IsEmpty()){Why=TEXT("Not yet supported. Your tests have not excluded: ")+FString::Join(Open,TEXT(", "))+TEXT(". Choose a test whose prediction separates them.");return false;}
 C.bConcluded=true;C.Conclusion=int32(Cause);
 Access.Add(TEXT("access.fieldlogger"));Access.Add(FString(TEXT("conclusion."))+CauseKeys[int32(Cause)]);
 Why=TEXT("Conclusion supported by your retained tests.");return true;
}
bool FS47FreeformCases::InRepeatWindow(int32 Line,int32 Day,double DayClock) const
{
 const FCase* C=Get(Line);return C&&C->bOpen&&Day>C->OpenDay&&FMath::IsFinite(DayClock)&&DayDelta(DayClock,C->OpenClock)<=RepeatWindow;
}
FString FS47FreeformCases::Title(int32 Line){return Line==0?TEXT("THE REPEATED INTERVAL"):TEXT("CASE");}
FString FS47FreeformCases::Question(int32 Line)
{
 return Line==0?TEXT("What produces the extra ticks between 9 and 13 seconds of the pulsar record? Three explanations fit the first original. Each test below predicts a different result for each of them."):FString();
}
FString FS47FreeformCases::CauseName(ECause Cause)
{
 switch(Cause){case ECause::Feeder:return TEXT("Feeder interference");case ECause::LocalTransmitter:return TEXT("Local intermittent transmitter");default:return TEXT("Follows the sky source");}
}
FString FS47FreeformCases::TestName(ETest Test)
{
 switch(Test){case ETest::Generator:return TEXT("Generator power");case ETest::OffSource:return TEXT("Off-source pointing");default:return TEXT("Repeat in the same window");}
}
FString FS47FreeformCases::Prediction(ETest Test)
{
 switch(Test)
 {
  case ETest::Generator:return TEXT("Feeder: ticks vanish. Transmitter: ticks remain. Sky: ticks remain.");
  case ETest::OffSource:return TEXT("Feeder: ticks remain. Transmitter: ticks remain. Sky: ticks vanish.");
  default:return TEXT("Feeder: ticks remain. Transmitter: ticks vanish. Sky: ticks remain.");
 }
}
FString FS47FreeformCases::Instruction(ETest Test)
{
 switch(Test)
 {
  case ETest::Generator:return TEXT("Fuel and start the backup generator by the service bay, then record the whole pulsar observation on generator power. One fuel can runs six game-hours.");
  case ETest::OffSource:return FString::Printf(TEXT("The tracking PC points the array %.1f degrees beside the pulsar. Record the full observation there."),OffSourceDegrees);
  default:return TEXT("Record the pulsar again on a later day, within 90 minutes of the clock time of the first original.");
 }
}
FString FS47FreeformCases::RecordsTelex(ECause Cause)
{
 switch(Cause)
 {
  case ECause::Feeder:return TEXT("SOCORRO ELECTRIC CO-OP / LINE RECORDS\nRE: SARO RECEIVER INTERFERENCE\n\nMarch 1979: an arcing insulator on the SARO feeder, pole 47-C, produced periodic interference on station receivers until replaced. The pole was noted for recurrent tracking in wet weather.\n\nThe co-op will inspect on request. Your generator comparison has been forwarded to the line crew.");
  case ECause::LocalTransmitter:return TEXT("FCC FIELD OFFICE / ALBUQUERQUE\nRE: UNLICENSED INTERMITTENT CARRIER\n\nTwo ranchers near US-60 reported an intermittent carrier in 1984. The source was never located.\n\nYour repeat and generator records exclude the station itself. A portable field logger is approved on loan so the carrier can be recorded from a second site.");
  default:return TEXT("NRAO / SOCORRO\nRE: SARO TIMING SHEET\n\nYour extra pulse group matches no catalogued companion and survives both power and repeat controls while vanishing off the beam.\n\nA source fixed on the sky rises 3 minutes 56 seconds earlier each night; anything on the ground keeps clock time. Repeat against sidereal time before claiming more. A field logger is approved on loan for an independent second site.");
 }
}
FString FS47FreeformCases::Clock(double DayClock)
{
 const int32 S=int32(FMath::Fmod(FMath::Max(0.,DayClock),86400.));return FString::Printf(TEXT("%02d:%02d"),S/3600,(S/60)%60);
}
bool FS47FreeformCases::SelfTest(TArray<FString>& Lines)
{
 bool Good=true;
 // Every pair of different tests separates all three causes.
 for(int32 A=0;A<TestCount;++A)for(int32 B=A+1;B<TestCount;++B)
 {
  TSet<int32> Signatures;for(int32 C=0;C<CauseCount;++C)Signatures.Add((TicksFor(ECause(C),ETest(A))?1:0)|(TicksFor(ECause(C),ETest(B))?2:0));
  Good&=Signatures.Num()==CauseCount;
 }
 // Seeded causes are deterministic and all occur.
 TSet<int32> Seen;for(int32 Seed=0;Seed<64;++Seed){Good&=Seeded(Seed,0)==Seeded(Seed,0);Seen.Add(int32(Seeded(Seed,0)));}Good&=Seen.Num()==CauseCount;
 for(int32 Truth=0;Truth<CauseCount;++Truth)
 {
  int32 Seed=0;while(Seeded(Seed,0)!=ECause(Truth))++Seed;
  FS47FreeformCases K;K.Reset(Seed);FString Why;bool First=false;
  Good&=!K.Plan(0,ETest::Generator,TEXT("J"));             // closed case
  Good&=K.Open(0,1,20*3600.)&&!K.Open(0,2,0);
  Good&=!K.Conclude(0,ECause(Truth),Why);                  // no tests: a correct guess is not supported
  Good&=K.Plan(0,ETest::Generator,TEXT("J1"))&&!K.Plan(0,ETest::OffSource,TEXT("J2"));
  Good&=K.Record(0,ETest::Generator,2,false,false,First)&&!First&&!K.Get(0)->Results[0].bDone&&K.Get(0)->Planned<0; // invalid releases plan
  Good&=K.Plan(0,ETest::Generator,TEXT("J1"))&&K.Record(0,ETest::Generator,2,true,TicksFor(ECause(Truth),ETest::Generator),First)&&First;
  Good&=!K.Plan(0,ETest::Generator,TEXT("J3"));            // a done test is not repeated
  Good&=K.Plan(0,ETest::OffSource,TEXT("J4"))&&K.Record(0,ETest::OffSource,3,true,TicksFor(ECause(Truth),ETest::OffSource),First)&&!First;
  Good&=K.Remaining(0)==1&&!K.Excluded(0,ECause(Truth));
  for(int32 Wrong=0;Wrong<CauseCount;++Wrong)if(Wrong!=Truth)Good&=!K.Conclude(0,ECause(Wrong),Why);
  // Save/Continue retains the case; the cause is rederived, never read from disk.
  auto Disk=S47J::Read(S47J::Write(K.Save()));FS47FreeformCases R;R.Load(Disk,Seed);
  Good&=R.Get(0)->bOpen&&R.Get(0)->ValidTests==2&&R.Remaining(0)==1&&R.Get(0)->Truth==ECause(Truth);
  Good&=R.Conclude(0,ECause(Truth),Why)&&R.HasAccess(TEXT("access.fieldlogger"))&&!R.Conclude(0,ECause(Truth),Why);
  FS47FreeformCases Reloaded;Reloaded.Load(S47J::Read(S47J::Write(R.Save())),Seed);Good&=Reloaded.Get(0)->bConcluded&&Reloaded.Get(0)->Conclusion==Truth&&Reloaded.HasAccess(TEXT("access.fieldlogger"));
 }
 // Repeat window: a later day, within 90 minutes across midnight.
 FS47FreeformCases W;W.Reset(1);W.Open(0,3,23.5*3600);
 Good&=!W.InRepeatWindow(0,3,23.5*3600)&&W.InRepeatWindow(0,4,.5*3600)&&!W.InRepeatWindow(0,4,2*3600)&&W.InRepeatWindow(0,5,22.1*3600);
 // Corrupt or foreign data is rejected without crashing.
 FS47FreeformCases Bad;auto Junk=S47J::Read(TEXT("{\"v\":1,\"cases\":[{\"open\":true,\"planned\":99,\"openClock\":1e99,\"concluded\":true,\"conclusion\":7}]}"));Bad.Load(Junk,5);
 Good&=Bad.Get(0)->Planned==-1&&Bad.Get(0)->OpenClock<=86399&&!Bad.Get(0)->bConcluded&&Bad.Get(0)->Conclusion==-1;
 Lines.Add(FString::Printf(TEXT("%s / Freeform cases: seeded hidden cause, any two tests separate three explanations, guesses refused until supported, invalid tests release the plan, repeat window across midnight, Save/Continue keeps progress"),Good?TEXT("PASS"):TEXT("FAIL")));
 return Good;
}

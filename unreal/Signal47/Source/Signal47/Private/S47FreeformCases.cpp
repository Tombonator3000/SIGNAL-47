#include "S47FreeformCases.h"

namespace
{
 using EC=FS47FreeformCases::ECondition;
 uint32 Mix(uint32 V){V^=V>>16;V*=0x7feb352du;V^=V>>15;V*=0x846ca68bu;return V^(V>>16);}
 double DayDelta(double A,double B){const double D=FMath::Abs(FMath::Fmod(A-B,86400.));return FMath::Min(D,86400.-D);}
 bool DatedKey(const FString& Id,const TCHAR* Base)
 {
  if(Id==Base)return true;const FString Prefix=FString(Base)+TEXT(".day");if(!Id.StartsWith(Prefix))return false;
  const FString Suffix=Id.Mid(Prefix.Len());bool Digits=!Suffix.IsEmpty();for(TCHAR C:Suffix)Digits&=C>=TEXT('0')&&C<=TEXT('9');return Digits;
 }
 const FS47FreeformCases::FCaseDef Defs[FS47FreeformCases::CaseCount]={
 {
  TEXT("THE REPEATED INTERVAL"),
  TEXT("What produces the extra ticks between 9 and 13 seconds of the pulsar record? Three explanations fit the first original. Each test below predicts a different result for each of them."),
  FS47FreeformCases::EObservable::ExtraTicks,
  {TEXT("Feeder interference"),TEXT("Local intermittent transmitter"),TEXT("Follows the sky source")},
  {TEXT("Feeder"),TEXT("Transmitter"),TEXT("Sky")},
  {{TEXT("Generator power"),EC::Generator,TEXT("Fuel and start the backup generator by the service bay, then record the whole pulsar observation on generator power. One fuel can runs six game-hours.")},
   {TEXT("Off-source pointing"),EC::OffSource,TEXT("The tracking PC points the array 0.6 degrees beside the pulsar. Record the full observation there.")},
   {TEXT("Repeat in the same window"),EC::Repeat,TEXT("Record the pulsar again on a later day, within 90 minutes of the clock time of the first original.")}},
  // Generator, OffSource, Repeat
  {{false,true,true},   // feeder-borne ticks vanish on generator power
   {true,true,false},   // the intermittent local carrier does not recur
   {true,false,true}},  // a sky-fixed source vanishes off the beam
  {TEXT("SOCORRO ELECTRIC CO-OP / LINE RECORDS\nRE: SARO RECEIVER INTERFERENCE\n\nMarch 1979: an arcing insulator on the SARO feeder, pole 47-C, produced periodic interference on station receivers until replaced. The pole was noted for recurrent tracking in wet weather.\n\nThe co-op will inspect on request. Your generator comparison has been forwarded to the line crew."),
   TEXT("FCC FIELD OFFICE / ALBUQUERQUE\nRE: UNLICENSED INTERMITTENT CARRIER\n\nTwo ranchers near US-60 reported an intermittent carrier in 1984. The source was never located.\n\nYour repeat and generator records exclude the station itself. A portable field logger is approved on loan so the carrier can be recorded from a second site."),
   TEXT("NRAO / SOCORRO\nRE: SARO TIMING SHEET\n\nYour extra pulse group matches no catalogued companion and survives both power and repeat controls while vanishing off the beam.\n\nA source fixed on the sky rises 3 minutes 56 seconds earlier each night; anything on the ground keeps clock time. Repeat against sidereal time before claiming more. A field logger is approved on loan for an independent second site.")},
  TEXT("Records reply: the repeated interval"),TEXT("access.fieldlogger")
 },
 {
  TEXT("DISAGREEING RECORDS"),
  TEXT("Why does the receiver's own counter disagree with the independent counter by about 3.2 seconds? Three explanations fit the retained papers. Each test below predicts a different result for each of them."),
  FS47FreeformCases::EObservable::ClockDelta,
  {TEXT("Receiver timing relay fault"),TEXT("Mains-locked station clock"),TEXT("Someone uses the station at night")},
  {TEXT("Relay"),TEXT("Mains"),TEXT("Someone")},
  {{TEXT("Swap the timing relay"),EC::RelaySwap,TEXT("Fit one spare relay from the depot into the receiver timing path when the recording starts. The relay is used up.")},
   {TEXT("Generator power"),EC::Generator,TEXT("Fuel and start the backup generator by the service bay, then record the whole observation on generator power, off the mains.")},
   {TEXT("Sealed station"),EC::DoorsSealed,TEXT("Close both SARO doors and stay in the control room for the whole recording. Opening a door spoils the test.")}},
  // RelaySwap, Generator, DoorsSealed
  {{false,true,true},   // a faulty timing relay is replaced
   {true,false,true},   // a mains-locked clock follows the feeder, not the generator
   {true,true,false}},  // nobody can reach the receiver through shut doors
  {TEXT("FIELD SERVICE / ALBUQUERQUE\nRE: SARO RECEIVER TIMING PATH\n\nTiming relays from the 1983 batch drift when warm. Your swap test isolates the fault exactly as the bulletin describes. Replace the remaining relays at the next service visit.\n\nThe disagreement is an instrument fault. The original papers remain valid with the offset noted."),
   TEXT("SOCORRO ELECTRIC CO-OP / FREQUENCY LOG\nRE: SARO CLOCK COMPLAINT\n\nThe feeder frequency sagged on the nights you list. Any clock driven from the mains loses time while the line is loaded.\n\nYour generator records show the offset vanish off the mains. Use the generator for timing-critical work until the station clock is replaced."),
   TEXT("SOCORRO COUNTY SHERIFF / NIGHT LOG\nRE: SARO ANNEX\n\nA deputy on the night patrol noted lights in the SARO annex at 02:40 on two nights when the operator was off shift. No forced entry was found and nothing was reported missing.\n\nYour sealed-station recording shows the receiver keeps time when nobody else can reach it. Keep the doors shut at night. Report anyone you see.")},
  TEXT("Records reply: disagreeing records"),TEXT("access.nightwatch")
 },
 {
  TEXT("VISITORS AROUND THE BASE"),
  TEXT("What is moving around SARO at night? Roof impacts, a figure near the array and marks on the ground are retained as separate observations; they are not yet one cause. Each test below predicts a different result for each explanation."),
  FS47FreeformCases::EObservable::FieldTrace,
  {TEXT("Wind and loose fittings"),TEXT("Local human activity"),TEXT("Unexplained visit")},
  {TEXT("Wind"),TEXT("People"),TEXT("Unexplained")},
  {{TEXT("Secure the roof fitting"),EC::FieldWork,TEXT("Mark and fasten the loose roof plate, then listen from the same reference point on a comparably windy night."),TEXT("impacts continue"),TEXT("impacts stop")},
   {TEXT("Controlled access"),EC::FieldWork,TEXT("Seal the service gate and lay tracking strips before the same observation window; inspect both afterwards."),TEXT("seal broken and a continuous track"),TEXT("seal intact, no continuous track")},
   {TEXT("Two simultaneous sites"),EC::FieldWork,TEXT("Calibrate the loan field logger, keep simultaneous originals from the roof reference and a field post, then compare time and direction."),TEXT("simultaneous disturbance at both sites"),TEXT("no simultaneous disturbance"),TEXT("access.fieldlogger")}},
  // Roof, Access, TwoSites (present = Yes)
  {{false,false,false},  // wind: the fastened plate stops it, nobody crosses, only the roof reference moves
   {true,true,false},    // people: impacts continue, the seal is broken, the sequence follows one path
   {true,false,true}},   // unexplained: impacts continue, no crossing, both sites at once
  {TEXT("COUNTY BUILDING INSPECTOR / SOCORRO\nRE: SARO ANNEX ROOF\n\nThe roof plate you fastened was the last unsecured sheet on the 1962 annex. Wind loading on that roof is noted in the 1979 inspection.\n\nNo further action. Your sealed-gate and two-site records show nothing crossing to the station."),
   TEXT("SOCORRO COUNTY SHERIFF\nRE: SARO SERVICE GATE\n\nYour broken seal and the track along the service road match reports of people taking copper and scrap from remote sites this autumn.\n\nA deputy will drive past SARO on night patrol. Keep the gate sealed and photograph any vehicle."),
   TEXT("SARO OPERATIONS / FILE NOTE\nRE: VISITORS AROUND THE BASE\n\nThe fastened plate did not stop the impacts, the sealed gate stayed intact, and two separate sites recorded the same disturbance at the same minute.\n\nNo person, vehicle or animal accounts for it. The observation is retained without an identity. Do not approach it alone.")},
  TEXT("Records reply: visitors around the base"),TEXT("access.visitors")
 }};
}

const FS47FreeformCases::FCaseDef& FS47FreeformCases::Def(int32 Line){return Defs[FMath::Clamp(Line,0,CaseCount-1)];}
int32 FS47FreeformCases::LineForFinding(const FString& Id)
{
 if(Id==TEXT("pulse-review")||DatedKey(Id,TEXT("extra_pulse")))return 0;
 if(DatedKey(Id,TEXT("time_disagreement"))||DatedKey(Id,TEXT("duplicate_print"))||DatedKey(Id,TEXT("unsigned_telex")))return 1;
 if(DatedKey(Id,TEXT("roof_steps"))||DatedKey(Id,TEXT("observer"))||DatedKey(Id,TEXT("landing_trace")))return 2;
 return -1;
}
int32 FS47FreeformCases::LineForObservation(const FString& Event)
{
 return Event==TEXT("roof_steps")||Event==TEXT("observer")||Event==TEXT("landing_trace")||Event==TEXT("feeder_presence")||Event==TEXT("array_presence")?2:-1;
}
bool FS47FreeformCases::AddLead(int32 Line,const FString& ObservationId,int32 Day,double DayClock)
{
 if(!Cases.IsValidIndex(Line)||ObservationId.IsEmpty()||ObservationId.Len()>160)return false;
 FCase& C=Cases[Line];if(C.Leads.Contains(ObservationId))return false;
 const bool Opened=Open(Line,Day,DayClock);
 if(!C.bOpen)return false;
 if(C.Leads.Num()<MaxLeads)C.Leads.Add(ObservationId);
 (void)Opened;return true;
}
int32 FS47FreeformCases::Seeded(int32 Seed,int32 Line){return int32(Mix(uint32(Seed)^Mix(0x43415345u+uint32(Line)))%uint32(CauseCount));}
bool FS47FreeformCases::PresentFor(int32 Line,int32 Cause,int32 Test)
{
 if(Line<0||Line>=CaseCount||Cause<0||Cause>=CauseCount||Test<0||Test>=TestCount)return true;return Defs[Line].Present[Cause][Test];
}
void FS47FreeformCases::Reset(int32 Seed)
{
 Cases.SetNum(CaseCount);Access.Reset();
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
  // The cause is never trusted from disk: it is a pure function of the session seed,
  // and a retained result always agrees with the physical rule for that cause.
  const auto R=S47J::Arr(O,TEXT("results"));
  for(int32 T=0;T<FMath::Min(TestCount,R.Num());++T)if(R[T]&&R[T]->Type==EJson::Object)
  {
   const auto X=R[T]->AsObject();auto& Out=C.Results[T];Out.bDone=S47J::Bool(X,TEXT("done"));
   Out.Day=FMath::Clamp(S47J::Int(X,TEXT("day")),0,36500);Out.bPresent=Out.bDone&&PresentFor(I,C.Truth,T);
  }
  for(const FString& L:S47J::Strs(O,TEXT("leads")))if(!L.IsEmpty()&&L.Len()<=160&&!C.Leads.Contains(L)&&C.Leads.Num()<MaxLeads)C.Leads.Add(L);
  if(C.bConcluded&&C.Conclusion<0)C.bConcluded=false;
 }
 // A saved conclusion must still be supported by its retained tests.
 for(int32 I=0;I<Cases.Num();++I)if(Cases[I].bConcluded&&(Excluded(I,Cases[I].Conclusion)||Remaining(I)!=1)){Cases[I].bConcluded=false;Cases[I].Conclusion=-1;}
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
  for(const FResult& Res:C.Results){auto Y=S47J::New();S47J::Set(Y,TEXT("done"),Res.bDone);S47J::Set(Y,TEXT("present"),Res.bPresent);S47J::Set(Y,TEXT("day"),Res.Day);R.Add(MakeShared<FJsonValueObject>(Y));}
  S47J::Set(X,TEXT("leads"),C.Leads);
  X->SetArrayField(TEXT("results"),R);List.Add(MakeShared<FJsonValueObject>(X));
 }
 O->SetArrayField(TEXT("cases"),List);return O;
}
bool FS47FreeformCases::Open(int32 Line,int32 Day,double DayClock)
{
 if(!Cases.IsValidIndex(Line)||Cases[Line].bOpen||Day<1||!FMath::IsFinite(DayClock))return false;
 FCase& C=Cases[Line];C.bOpen=true;C.OpenDay=Day;C.OpenClock=FMath::Clamp(FMath::Fmod(FMath::Max(0.,DayClock),86400.),0.,86399.);return true;
}
bool FS47FreeformCases::Plan(int32 Line,int32 Test,const FString& JobId)
{
 if(!Cases.IsValidIndex(Line)||JobId.IsEmpty()||Test<0||Test>=TestCount)return false;FCase& C=Cases[Line];
 if(!C.bOpen||C.bConcluded||C.Planned>=0||C.Results[Test].bDone)return false;
 C.Planned=Test;C.PlannedJob=JobId;return true;
}
void FS47FreeformCases::CancelPlan(int32 Line){if(Cases.IsValidIndex(Line)){Cases[Line].Planned=-1;Cases[Line].PlannedJob.Empty();}}
bool FS47FreeformCases::Record(int32 Line,int32 Test,int32 Day,bool bValid,bool bPresent,bool& bFirstValid)
{
 bFirstValid=false;if(!Cases.IsValidIndex(Line))return false;FCase& C=Cases[Line];
 if(!C.bOpen||C.Planned!=Test||Test<0)return false;
 C.Planned=-1;C.PlannedJob.Empty();
 if(!bValid)return true; // the plan is released so the test can be repeated properly
 C.Results[Test].bDone=true;C.Results[Test].bPresent=bPresent;C.Results[Test].Day=Day;
 bFirstValid=ValidTestsTotal()==0;++C.ValidTests;return true;
}
int32 FS47FreeformCases::ValidTestsTotal() const{int32 N=0;for(const FCase& C:Cases)N+=C.ValidTests;return N;}
bool FS47FreeformCases::Excluded(int32 Line,int32 Cause) const
{
 const FCase* C=Get(Line);if(!C)return false;
 for(int32 T=0;T<TestCount;++T)if(C->Results[T].bDone&&PresentFor(Line,Cause,T)!=C->Results[T].bPresent)return true;
 return false;
}
int32 FS47FreeformCases::Remaining(int32 Line) const{int32 N=0;for(int32 I=0;I<CauseCount;++I)N+=Excluded(Line,I)?0:1;return N;}
bool FS47FreeformCases::Conclude(int32 Line,int32 Cause,FString& Why)
{
 if(!Cases.IsValidIndex(Line)||!Cases[Line].bOpen||Cause<0||Cause>=CauseCount){Why=TEXT("No open case.");return false;}
 FCase& C=Cases[Line];const FCaseDef& D=Defs[Line];
 if(C.bConcluded){Why=TEXT("This case is already concluded; its record is retained.");return false;}
 if(Excluded(Line,Cause)){Why=FString(D.Causes[Cause])+TEXT(" is contradicted by your own retained test. Read the result again.");return false;}
 TArray<FString> Open;for(int32 I=0;I<CauseCount;++I)if(I!=Cause&&!Excluded(Line,I))Open.Add(D.Causes[I]);
 if(!Open.IsEmpty()){Why=TEXT("Not yet supported. Your tests have not excluded: ")+FString::Join(Open,TEXT(", "))+TEXT(". Choose a test whose prediction separates them.");return false;}
 C.bConcluded=true;C.Conclusion=Cause;Access.Add(D.Access);
 Why=TEXT("Conclusion supported by your retained tests.");return true;
}
bool FS47FreeformCases::InRepeatWindow(int32 Line,int32 Day,double DayClock) const
{
 const FCase* C=Get(Line);return C&&C->bOpen&&Day>C->OpenDay&&FMath::IsFinite(DayClock)&&DayDelta(DayClock,C->OpenClock)<=RepeatWindow;
}
FString FS47FreeformCases::Prediction(int32 Line,int32 Test)
{
 const FCaseDef& D=Def(Line);const bool Ticks=D.Observable==EObservable::ExtraTicks;TArray<FString> Out;
 const FTestDef& T=D.Tests[FMath::Clamp(Test,0,TestCount-1)];
 for(int32 C=0;C<CauseCount;++C)
 {
  const bool P=PresentFor(Line,C,Test);
  const FString Word=T.Yes&&T.No?FString(P?T.Yes:T.No):FString(P?(Ticks?TEXT("ticks remain"):TEXT("records still disagree")):(Ticks?TEXT("ticks vanish"):TEXT("records agree")));
  Out.Add(FString(D.Short[C])+TEXT(": ")+Word);
 }
 return FString::Join(Out,TEXT(". "))+TEXT(".");
}
FString FS47FreeformCases::TestResultText(int32 Line,int32 Test,bool bPresent)
{
 const FTestDef& T=Def(Line).Tests[FMath::Clamp(Test,0,TestCount-1)];
 return T.Yes&&T.No?FString(bPresent?T.Yes:T.No).ToUpper():ResultText(Line,bPresent);
}
FString FS47FreeformCases::ResultText(int32 Line,bool bPresent)
{
 if(Def(Line).Observable==EObservable::ExtraTicks)return bPresent?TEXT("EXTRA TICKS PRESENT"):TEXT("NO EXTRA TICKS");
 return bPresent?FString::Printf(TEXT("CLOCK DELTA %+.3f s / RECORDS STILL DISAGREE"),ClockOffset):FString(TEXT("CLOCK DELTA +0.000 s / RECORDS AGREE"));
}
FString FS47FreeformCases::Clock(double DayClock)
{
 const int32 S=int32(FMath::Fmod(FMath::Fmod(DayClock,86400.)+86400.,86400.));return FString::Printf(TEXT("%02d:%02d"),S/3600,(S/60)%60);
}
bool FS47FreeformCases::SelfTest(TArray<FString>& Lines)
{
 bool Good=true;
 for(int32 Line=0;Line<CaseCount;++Line)
 {
  // Every pair of different tests separates all three causes.
  for(int32 A=0;A<TestCount;++A)for(int32 B=A+1;B<TestCount;++B)
  {TSet<int32> Sig;for(int32 C=0;C<CauseCount;++C)Sig.Add((PresentFor(Line,C,A)?1:0)|(PresentFor(Line,C,B)?2:0));Good&=Sig.Num()==CauseCount;}
  TSet<int32> Seen;for(int32 Seed=0;Seed<64;++Seed)Seen.Add(Seeded(Seed,Line));Good&=Seen.Num()==CauseCount;
  for(int32 Truth=0;Truth<CauseCount;++Truth)
  {
   int32 Seed=0;while(Seeded(Seed,Line)!=Truth)++Seed;
   FS47FreeformCases K;K.Reset(Seed);FString Why;bool First=false;
   Good&=!K.Plan(Line,0,TEXT("J"));
   Good&=K.Open(Line,1,20*3600.)&&!K.Open(Line,2,0);
   Good&=!K.Conclude(Line,Truth,Why);                       // a correct guess alone is not supported
   Good&=K.Plan(Line,0,TEXT("J1"))&&!K.Plan(Line,1,TEXT("J2"));
   Good&=K.Record(Line,0,2,false,false,First)&&!First&&!K.Get(Line)->Results[0].bDone&&K.Get(Line)->Planned<0;
   Good&=K.Plan(Line,0,TEXT("J1"))&&K.Record(Line,0,2,true,PresentFor(Line,Truth,0),First)&&First;
   Good&=!K.Plan(Line,0,TEXT("J3"));
   Good&=K.Plan(Line,1,TEXT("J4"))&&K.Record(Line,1,3,true,PresentFor(Line,Truth,1),First)&&!First;
   Good&=K.Remaining(Line)==1&&!K.Excluded(Line,Truth);
   for(int32 Wrong=0;Wrong<CauseCount;++Wrong)if(Wrong!=Truth)Good&=!K.Conclude(Line,Wrong,Why);
   FS47FreeformCases R;R.Load(S47J::Read(S47J::Write(K.Save())),Seed);
   Good&=R.Get(Line)->bOpen&&R.Get(Line)->ValidTests==2&&R.Remaining(Line)==1&&R.Get(Line)->Truth==Truth;
   Good&=R.Conclude(Line,Truth,Why)&&R.HasAccess(Def(Line).Access)&&!R.Conclude(Line,Truth,Why);
   FS47FreeformCases Again;Again.Load(S47J::Read(S47J::Write(R.Save())),Seed);Good&=Again.Get(Line)->bConcluded&&Again.Get(Line)->Conclusion==Truth&&Again.HasAccess(Def(Line).Access);
  }
 }
 // Observation leads open the visitors case once, deduplicate and survive Continue.
 {
  FS47FreeformCases L;L.Reset(9);
  Good&=LineForObservation(TEXT("observer"))==2&&LineForObservation(TEXT("close_pass"))==-1;
  Good&=L.AddLead(2,TEXT("obs-1"),2,22*3600.)&&L.Get(2)->bOpen&&L.Get(2)->OpenDay==2&&!L.AddLead(2,TEXT("obs-1"),3,0)&&L.AddLead(2,TEXT("obs-2"),3,1*3600.);
  FS47FreeformCases LR;LR.Load(S47J::Read(S47J::Write(L.Save())),9);Good&=LR.Get(2)->Leads.Num()==2&&LR.Get(2)->OpenDay==2&&Def(2).Tests[2].Access!=nullptr;
  Good&=TestResultText(2,1,true).Contains(TEXT("SEAL BROKEN"))&&Prediction(2,0).Contains(TEXT("impacts stop"));
 }
 // The first-test reward is granted once across all cases.
 FS47FreeformCases Both;Both.Reset(3);bool First=false;Both.Open(0,1,0);Both.Open(1,1,0);
 Both.Plan(0,1,TEXT("A"));Both.Record(0,1,1,true,true,First);Good&=First;Both.Plan(1,1,TEXT("B"));Both.Record(1,1,1,true,true,First);Good&=!First;
 Good&=LineForFinding(TEXT("pulse-review"))==0&&LineForFinding(TEXT("extra_pulse.day4"))==0&&LineForFinding(TEXT("extra_pulse.daylight"))==-1&&LineForFinding(TEXT("duplicate_print"))==1&&LineForFinding(TEXT("unsigned_telex.day3"))==1&&LineForFinding(TEXT("observer"))==2&&LineForFinding(TEXT("close_pass"))==-1;
 FS47FreeformCases W;W.Reset(1);W.Open(0,3,23.5*3600);
 Good&=!W.InRepeatWindow(0,3,23.5*3600)&&W.InRepeatWindow(0,4,.5*3600)&&!W.InRepeatWindow(0,4,2*3600)&&W.InRepeatWindow(0,5,22.1*3600)&&Clock(-1800)==TEXT("23:30");
 FS47FreeformCases Bad;Bad.Load(S47J::Read(TEXT("{\"v\":1,\"cases\":[{\"open\":true,\"planned\":99,\"openClock\":1e99,\"concluded\":true,\"conclusion\":7}]}")),5);
 Good&=Bad.Get(0)->Planned==-1&&Bad.Get(0)->OpenClock<=86399&&!Bad.Get(0)->bConcluded&&Bad.Get(0)->Conclusion==-1;
 Lines.Add(FString::Printf(TEXT("%s / Freeform cases: three data-driven cases, observation leads, seeded hidden cause, any two tests separate three explanations, guesses refused until supported, invalid tests release the plan, one first-test reward, repeat window across midnight, Save/Continue keeps progress"),Good?TEXT("PASS"):TEXT("FAIL")));
 return Good;
}

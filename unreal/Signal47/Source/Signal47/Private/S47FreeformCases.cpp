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
 using ER=FS47FreeformCases::EReply;
 // The first mystery. A burst on the receiver, something at the remote feeder, and three ways to read them. Every reading
 // is deniable and every reading can turn up under any cause, so no text alone gives the cause away.
 const FS47FreeformCases::FMysteryDef SameMinute={
  2,TEXT("Reply: the same minute"),
  {{TEXT("carrier_saro"),TEXT("SARO receiver chart"),{
    TEXT("Receiver chart, the minute of the burst. Three even blocks of hash, eleven seconds on and four off, then nothing. It looks keyed, the way a hand keys a set. A loose connector can do the same."),
    TEXT("Receiver chart, the minute of the burst. The hash rides on mains hum and grows as the station load comes up, then fades. It looks like the line. A generator close by would look much the same."),
    TEXT("Receiver chart, the minute of the burst. The hash rises and falls as slowly as a source crossing the beam, and the reference channel stays flat. It looks as if it came in through the dishes. A drifting gain stage can do the same.")}},
   {TEXT("carrier_feeder"),TEXT("Remote feeder on the county road"),{
    TEXT("Remote feeder, after the burst. Fresh tyre marks on the shoulder and one cut strand in the fence by the panel. A rancher after a stray could have done as much."),
    TEXT("Remote feeder, after the burst. A scorched smell at the panel and a dark streak down the top insulator. They all darken with age."),
    TEXT("Remote feeder, after the burst. No marks at all in the soft dirt round the panel, and the fence wire ticks when you touch it. Dry air does that out here.")}}},
  {{{TEXT("SOCORRO COUNTY SHERIFF\nRE: SARO REMOTE FEEDER\n\nA deputy sat on the county road at the hour you gave. He turned back two men with a pickup and a portable generator, stripping ground wire from the feeder run.\n\nThe co-op sends a spare control fuse with its thanks. It is in your depot."),ER::Spare},
    {TEXT("SOCORRO COUNTY SHERIFF\nRE: SARO REMOTE FEEDER\n\nThe deputy found no one and no tracks worth the name. He did think the top insulator looked burnt and passed that to the co-op, who have it on a list.\n\nIt let go before they got there. Measure the feeder at the service bay."),ER::FeederFault},
    {TEXT("SOCORRO COUNTY SHERIFF\nRE: SARO REMOTE FEEDER\n\nA deputy sat on the county road from midnight. Nobody came, no vehicle, no tracks.\n\nHe says the fence was ticking, and he would rather not make that drive again for this."),ER::None}},
   {{TEXT("SOCORRO ELECTRIC CO-OP / LINE RECORDS\nRE: SARO FEEDER CALL-OUT\n\nOur crew climbed the pole. The insulators are sound. They did find a cut fence strand and fresh tyre marks at the panel and have told the sheriff.\n\nThe call-out is charged to your station."),ER::CallOut},
    {TEXT("SOCORRO ELECTRIC CO-OP / LINE RECORDS\nRE: SARO FEEDER CALL-OUT\n\nOur crew found the top insulator cracked and tracking, and replaced it before it failed. No charge.\n\nA spare control fuse for the panel is in your depot."),ER::Spare},
    {TEXT("SOCORRO ELECTRIC CO-OP / LINE RECORDS\nRE: SARO FEEDER CALL-OUT\n\nOur crew climbed the pole. Insulators sound, line balanced, nothing on the recorder at the hour you gave. We cannot find your fault.\n\nThe call-out is charged to your station."),ER::CallOut}},
   {{TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE SAME MINUTE\n\nFiled as not accounted for, so nobody was sent. Overnight someone took the ground lead off the feeder panel.\n\nThe line is unstable. Measure it at the service bay."),ER::FeederFault},
    {TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE SAME MINUTE\n\nFiled as not accounted for, so nobody was sent. The top insulator on the feeder let go this morning.\n\nThe line is unstable. Measure it at the service bay."),ER::FeederFault},
    {TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE SAME MINUTE\n\nFiled as not accounted for, so nobody was sent. Nothing further has happened.\n\nA rancher on US-60 rang the office to ask whether we had noticed anything at that minute. He did not say why."),ER::None}}}
 };
 const FS47FreeformCases::FMysteryDef LostTime={
  4,TEXT("Personnel: the lost hours"),
  {{TEXT("lost_clock"),TEXT("The OPS clock"),
    {TEXT("The workstation clock stuck at the hour I lost and then corrected itself in one step, as a counter does when its relay catches up. My watch has never done that."),
     TEXT("The workstation clock put itself right as soon as I logged on. It was the only thing that disagreed with my watch, and my watch is the one I would not swear to."),
     TEXT("The workstation clock and my watch agree now. The log shows nobody on shift for the whole gap and the door contact never opened. Whatever else was going on, the clock kept running.")}},
   {TEXT("lost_ground"),TEXT("Where it happened"),
    {TEXT("At the place itself nothing shows but my own tracks. Going by the sun they are hours older than I remember them being, if the clock I went by was right."),
     TEXT("At the place itself my tracks go in a small circle and stop. It is the kind of track somebody leaves who is not paying attention to where they are walking."),
     TEXT("At the place itself my tracks stop in the open. They do not start again anywhere I could find.")}}},
  {{{TEXT("FIELD SERVICE / ALBUQUERQUE\nRE: SARO STATION CLOCK\n\nWe pulled the timing relay on your report. The contacts were pitted and it was slipping under load.\n\nA spare has gone into your depot. That accounts for the clock. It does not account for your whereabouts, which are not our department."),ER::Spare},
    {TEXT("FIELD SERVICE / ALBUQUERQUE\nRE: SARO STATION CLOCK\n\nWe bench-tested the timing path on your report and it holds to a tenth of a second over a week. We cannot find your fault.\n\nThe call-out is charged to your station."),ER::CallOut},
    {TEXT("FIELD SERVICE / ALBUQUERQUE\nRE: SARO STATION CLOCK\n\nThe timing path holds. The station log for that night is complete and shows no operator for the period you list.\n\nThe call-out is charged to your station."),ER::CallOut}},
   {{TEXT("SOCORRO GENERAL / OUTPATIENT\nRE: REFERRAL\n\nBlood pressure and reflexes unremarkable. Nothing here suggests a lapse. Your own form says the station clock was out.\n\nThe visit is charged to your station."),ER::CallOut},
    {TEXT("SOCORRO GENERAL / OUTPATIENT\nRE: REFERRAL\n\nNight shifts, irregular meals, long drives alone. It happens to people in your line. Sleep more and do not drive tired.\n\nNo charge to the station for the first visit."),ER::None},
    {TEXT("SOCORRO GENERAL / OUTPATIENT\nRE: REFERRAL\n\nUnremarkable. There are some small marks you did not mention and could not account for. Probably nothing. Come back if they change.\n\nThe visit is charged to your station."),ER::CallOut}},
   {{TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE LOST HOURS\n\nFiled as not accounted for. Field service has since found the timing relay slipping and charged nothing for it.\n\nThe note stands as you wrote it."),ER::None},
    {TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE LOST HOURS\n\nFiled as not accounted for. Nobody has asked about it. You might see somebody about your sleep."),ER::None},
    {TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE LOST HOURS\n\nFiled as not accounted for. A portable magnetometer from the old survey is on the stores list if you want to look at that ground properly. Nobody here is going to ask why."),ER::None}}}
 };
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
  {TEXT("SOCORRO ELECTRIC CO-OP / LINE RECORDS\nRE: SARO RECEIVER INTERFERENCE\n\nMarch 1979: an arcing insulator on the SARO feeder, pole 47-C, produced periodic interference on station receivers until replaced. The pole was noted for recurrent tracking in wet weather.\n\nYour generator comparison has been forwarded to the line crew. A spare control fuse is in your depot."),
   TEXT("FCC FIELD OFFICE / ALBUQUERQUE\nRE: UNLICENSED INTERMITTENT CARRIER\n\nTwo ranchers near US-60 reported an intermittent carrier in 1984. The source was never located.\n\nYour repeat and generator records exclude the station itself. A portable field logger is approved on loan so the carrier can be recorded from a second site."),
   TEXT("NRAO / SOCORRO\nRE: SARO TIMING SHEET\n\nYour extra pulse group matches no catalogued companion and survives both power and repeat controls while vanishing off the beam.\n\nA source fixed on the sky rises 3 minutes 56 seconds earlier each night; anything on the ground keeps clock time. A field logger is approved on loan for an independent second site.")},
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
  {TEXT("FIELD SERVICE / ALBUQUERQUE\nRE: SARO RECEIVER TIMING PATH\n\nTiming relays from the 1983 batch drift when warm. Your swap test isolates the fault exactly as the bulletin describes. Two replacements are in your depot.\n\nThe disagreement is an instrument fault. The original papers remain valid with the offset noted."),
   TEXT("SOCORRO ELECTRIC CO-OP / FREQUENCY LOG\nRE: SARO CLOCK COMPLAINT\n\nThe feeder frequency sagged on the nights you list. Any clock driven from the mains loses time while the line is loaded.\n\nYour generator records show the offset vanish off the mains. Expect it again whenever the line is loaded."),
   TEXT("SOCORRO COUNTY SHERIFF / NIGHT LOG\nRE: SARO ANNEX\n\nA deputy on the night patrol noted lights in the SARO annex at 02:40 on two nights when the operator was off shift. No forced entry was found and nothing was reported missing.\n\nYour sealed-station recording shows the receiver keeps time when nobody else can reach it. Keep the doors shut at night. The patrol will take in the SARO stretch from now on.")},
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
  {TEXT("COUNTY BUILDING INSPECTOR / SOCORRO\nRE: SARO ANNEX ROOF\n\nThe roof plate you fastened was the last unsecured sheet on the 1962 annex. Wind loading on that roof is noted in the 1979 inspection.\n\nNo further action. Your sealed-gate record shows nothing crossing to the station."),
   TEXT("SOCORRO COUNTY SHERIFF\nRE: SARO SERVICE GATE\n\nYour broken seal and the track along the service road match reports of people taking copper and scrap from remote sites this autumn.\n\nA deputy will drive past SARO on night patrol. Keep the gate sealed and photograph any vehicle."),
   TEXT("SARO OPERATIONS / FILE NOTE\nRE: VISITORS AROUND THE BASE\n\nThe fastened plate did not stop the impacts and the sealed gate stayed intact.\n\nNo person, vehicle or animal accounts for it. The observation is retained without an identity. Do not approach it alone.")},
  TEXT("Records reply: visitors around the base"),TEXT("access.visitors")
 },
 {
  TEXT("THE SAME MINUTE"),
  TEXT("A burst of hash crossed the receiver one night, and the remote feeder on the county road may have something to do with it. Whichever you have seen, the other is worth a look: the chart at the rack, or the feeder after midnight, and neither lasts past noon. The traces need not agree, and nobody is going to tell you which reading is right. Test recordings show whether the extra ticks come back. File what you think it was: the reply comes the next morning, and acting on the wrong answer has a cost."),
  FS47FreeformCases::EObservable::ExtraTicks,
  {TEXT("Someone at the feeder"),TEXT("A failing line insulator"),TEXT("Not accounted for")},
  {TEXT("Someone"),TEXT("Insulator"),TEXT("Unaccounted")},
  {{TEXT("Generator power"),EC::Generator,TEXT("Fuel and start the backup generator by the service bay, then record the whole pulsar observation on generator power, off the feeder. One fuel can runs six game-hours.")},
   {TEXT("Off-source pointing"),EC::OffSource,TEXT("The tracking PC points the array 0.6 degrees beside the pulsar. Record the full observation there.")},
   {TEXT("Repeat in the same window"),EC::Repeat,TEXT("Record the pulsar again on a later day, within 90 minutes of the clock time of the first original.")}},
  // Generator, OffSource, Repeat
  {{true,true,false},   // whoever it was has gone by the next night
   {false,true,true},   // what is on the line stays on the line, and off the generator
   {true,false,true}},  // what arrives through the beam goes when the beam moves
  {TEXT(""),TEXT(""),TEXT("")},
  TEXT("Station log: the same minute"),TEXT("access.sameminute"),&SameMinute
 },
 {
  TEXT("THE LOST HOURS"),
  TEXT("There is a stretch of night you cannot account for. The station's clocks say one thing and your memory says nothing at all. It may be the clocks, or you, or neither. Log on at OPS and go back to where it happened; test recordings compare the receiver's counter with the independent one. File what you think it was. The reply comes the next morning, and whichever you file, someone is going to act on it."),
  FS47FreeformCases::EObservable::ClockDelta,
  {TEXT("The station clocks were wrong"),TEXT("A lapse of my own"),TEXT("Not accounted for")},
  {TEXT("Clocks"),TEXT("A lapse"),TEXT("Unaccounted")},
  {{TEXT("Swap the timing relay"),EC::RelaySwap,TEXT("Fit one spare relay from the depot into the receiver timing path when the recording starts. The relay is used up.")},
   {TEXT("Generator power"),EC::Generator,TEXT("Fuel and start the backup generator by the service bay, then record the whole observation on generator power, off the mains.")},
   {TEXT("Sealed station"),EC::DoorsSealed,TEXT("Close both SARO doors and stay in the control room for the whole recording. Opening a door spoils the test.")}},
  // RelaySwap, Generator, DoorsSealed
  {{false,true,true},    // a slipping relay: gone with the relay, whatever else is done
   {false,false,false},  // the clocks were never wrong
   {true,true,false}},   // not the clocks and not the power: it keeps time only with nobody else able to reach it
  {TEXT(""),TEXT(""),TEXT("")},
  TEXT("Station log: the lost hours"),TEXT("access.lostHours"),&LostTime
 }};
}

const FS47FreeformCases::FCaseDef& FS47FreeformCases::Def(int32 Line){return Defs[FMath::Clamp(Line,0,CaseCount-1)];}
int32 FS47FreeformCases::LineForFinding(const FString& Id)
{
 for(int32 Line=0;Line<CaseCount;++Line)if(const auto* Y=Defs[Line].Mystery)for(const auto& Clue:Y->Clues)if(DatedKey(Id,Clue.Finding))return Line;
 if(Id==TEXT("pulse-review")||DatedKey(Id,TEXT("extra_pulse")))return 0;
 if(DatedKey(Id,TEXT("time_disagreement"))||DatedKey(Id,TEXT("duplicate_print"))||DatedKey(Id,TEXT("unsigned_telex")))return 1;
 if(DatedKey(Id,TEXT("roof_steps"))||DatedKey(Id,TEXT("observer"))||DatedKey(Id,TEXT("landing_trace")))return 2;
 if(Id.Contains(TEXT("_daylight_d"))||Id.StartsWith(TEXT("taken-"))||Id.EndsWith(TEXT("_visit"))||Id.StartsWith(TEXT("peek-"))||Id.StartsWith(TEXT("flood-room-"))||Id.StartsWith(TEXT("door-visit-"))||Id.StartsWith(TEXT("door-marks-")))return 2;
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
FString FS47FreeformCases::UnavailableReason(int32 Line,int32 Test)
{
 return Line==2&&Test==2?TEXT("Two-site contact recording is unavailable: the loan field logger measures local RF interference, not roof sounds. A paired contact recorder is not available. Choose the roof or gate control on CASE BOARD."):FString();
}
void FS47FreeformCases::Reset(int32 Seed)
{
 Cases.SetNum(CaseCount);Access.Reset();CaseSeed=Seed;
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
  C.Hunch=FMath::Clamp(S47J::Int(O,TEXT("hunch"),-1),-1,CauseCount-1);
  C.OpenDay=FMath::Clamp(S47J::Int(O,TEXT("openDay")),0,36500);
  const double Clock=S47J::Num(O,TEXT("openClock"));C.OpenClock=FMath::IsFinite(Clock)?FMath::Clamp(Clock,0.,86399.):0;
  C.Planned=FMath::Clamp(S47J::Int(O,TEXT("planned"),-1),-1,TestCount-1);C.PlannedJob=C.Planned>=0?S47J::Str(O,TEXT("plannedJob")):FString();
  if(C.PlannedJob.IsEmpty())C.Planned=-1;
  C.Conclusion=C.bConcluded?FMath::Clamp(S47J::Int(O,TEXT("conclusion"),-1),-1,CauseCount-1):-1;
  C.ValidTests=FMath::Clamp(S47J::Int(O,TEXT("validTests")),0,1000);
  // Older saves have neither: no assessment could have been filed in them.
  C.ConcludeDay=C.bConcluded?FMath::Clamp(S47J::Int(O,TEXT("concludeDay")),0,36500):0;C.bReplied=C.bConcluded&&S47J::Bool(O,TEXT("replied"));
  // The cause is never trusted from disk: it is a pure function of the session seed,
  // but retained observations are evidence, not recomputed answers from the hidden cause.
  const auto R=S47J::Arr(O,TEXT("results"));
  for(int32 T=0;T<FMath::Min(TestCount,R.Num());++T)if(R[T]&&R[T]->Type==EJson::Object)
  {
   const auto X=R[T]->AsObject();auto& Out=C.Results[T];Out.bDone=S47J::Bool(X,TEXT("done"));
   Out.Day=FMath::Clamp(S47J::Int(X,TEXT("day")),0,36500);Out.bPresent=Out.bDone&&S47J::Bool(X,TEXT("present"),PresentFor(I,C.Truth,T));
  }
  for(const FString& L:S47J::Strs(O,TEXT("leads")))if(!L.IsEmpty()&&L.Len()<=160&&!C.Leads.Contains(L)&&C.Leads.Num()<MaxLeads)C.Leads.Add(L);
  if(C.bConcluded&&C.Conclusion<0)C.bConcluded=false;
 }
 // A saved conclusion must still be supported by its retained tests.
 // An assessment needs only not to contradict them.
 for(int32 I=0;I<Cases.Num();++I)if(Cases[I].bConcluded&&(Excluded(I,Cases[I].Conclusion)||(!Defs[I].Mystery&&Remaining(I)!=1))){Cases[I].bConcluded=false;Cases[I].Conclusion=-1;Cases[I].ConcludeDay=0;Cases[I].bReplied=false;}
}
S47J::FObj FS47FreeformCases::Save() const
{
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);
 TArray<FString> A=Access.Array();A.Sort();S47J::Set(O,TEXT("access"),A);
 TArray<TSharedPtr<FJsonValue>> List;
 for(const FCase& C:Cases)
 {
  auto X=S47J::New();S47J::Set(X,TEXT("open"),C.bOpen);S47J::Set(X,TEXT("concluded"),C.bConcluded);S47J::Set(X,TEXT("openDay"),C.OpenDay);S47J::Set(X,TEXT("openClock"),C.OpenClock);
  S47J::Set(X,TEXT("hunch"),C.Hunch);S47J::Set(X,TEXT("planned"),C.Planned);S47J::Set(X,TEXT("plannedJob"),C.PlannedJob);S47J::Set(X,TEXT("conclusion"),C.Conclusion);S47J::Set(X,TEXT("validTests"),C.ValidTests);
  TArray<TSharedPtr<FJsonValue>> R;
  for(const FResult& Res:C.Results){auto Y=S47J::New();S47J::Set(Y,TEXT("done"),Res.bDone);S47J::Set(Y,TEXT("present"),Res.bPresent);S47J::Set(Y,TEXT("day"),Res.Day);R.Add(MakeShared<FJsonValueObject>(Y));}
  S47J::Set(X,TEXT("leads"),C.Leads);
  if(C.ConcludeDay>0)S47J::Set(X,TEXT("concludeDay"),C.ConcludeDay);
  if(C.bReplied)S47J::Set(X,TEXT("replied"),true);
  X->SetArrayField(TEXT("results"),R);List.Add(MakeShared<FJsonValueObject>(X));
 }
 O->SetArrayField(TEXT("cases"),List);return O;
}
bool FS47FreeformCases::Open(int32 Line,int32 Day,double DayClock)
{
 if(!Cases.IsValidIndex(Line)||Cases[Line].bOpen||Day<1||!FMath::IsFinite(DayClock))return false;
 FCase& C=Cases[Line];C.bOpen=true;C.OpenDay=Day;C.OpenClock=FMath::Clamp(FMath::Fmod(FMath::Max(0.,DayClock),86400.),0.,86399.);return true;
}
bool FS47FreeformCases::SetHunch(int32 Line,int32 Cause)
{
 if(!Cases.IsValidIndex(Line)||!Cases[Line].bOpen||Cases[Line].bConcluded||Cause < -1||Cause>=CauseCount)return false;
 Cases[Line].Hunch=Cause;return true;
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
 const auto ByTests=[&](int32 Which){for(int32 T=0;T<TestCount;++T)if(C->Results[T].bDone&&PresentFor(Line,Which,T)!=C->Results[T].bPresent)return true;return false;};
 if(ByTests(Cause))return true;
 // Unless the retained tests have already closed everything else: no case ends with nothing left to conclude.
 if(C->bBeyondWind&&Cause==0)for(int32 Other=1;Other<CauseCount;++Other)if(!ByTests(Other))return true;
 return false;
}
void FS47FreeformCases::RuleOutWind(int32 Line)
{
 if(Line!=2||!Cases.IsValidIndex(Line)||Cases[Line].bConcluded)return;
 FCase& C=Cases[Line];C.bBeyondWind=true;
 // The hidden cause follows: what the tests go on to show has to be able to end somewhere else.
 if(C.Truth==0)C.Truth=2;
}
FString FS47FreeformCases::QuestionText(int32 Line) const
{
 const FCase* C=Get(Line);
 if(C&&C->bBeyondWind)return TEXT("What came for you out there, and is it what moves around SARO at night? Wind does not take time from anyone or light up a room. Roof impacts, a figure near the array and marks on the ground are retained as separate observations; they are not yet one cause. Each test below predicts a different result for each explanation that is left.");
 return Def(Line).Question;
}
int32 FS47FreeformCases::Remaining(int32 Line) const{int32 N=0;for(int32 I=0;I<CauseCount;++I)N+=Excluded(Line,I)?0:1;return N;}
bool FS47FreeformCases::Conclude(int32 Line,int32 Cause,FString& Why,int32 Day)
{
 if(!Cases.IsValidIndex(Line)||!Cases[Line].bOpen||Cause<0||Cause>=CauseCount){Why=TEXT("No open case.");return false;}
 FCase& C=Cases[Line];const FCaseDef& D=Defs[Line];
 if(C.bConcluded){Why=TEXT("This case is already concluded; its record is retained.");return false;}
 if(Excluded(Line,Cause)){Why=FString(D.Causes[Cause])+TEXT(" is contradicted by your own retained test. Read the result again.");return false;}
 if(D.Mystery)
 {
  // On any evidence, right or wrong. An unfinished plan no longer tests anything, and no filing can be taken back.
  C.bConcluded=true;C.Conclusion=Cause;C.ConcludeDay=FMath::Clamp(Day,0,36500);C.Planned=-1;C.PlannedJob.Empty();Access.Add(D.Access);
  Why=TEXT("Assessment filed.");return true;
 }
 TArray<FString> Open;for(int32 I=0;I<CauseCount;++I)if(I!=Cause&&!Excluded(Line,I))Open.Add(D.Causes[I]);
 if(!Open.IsEmpty()){Why=TEXT("Not yet supported. Your tests have not excluded: ")+FString::Join(Open,TEXT(", "))+TEXT(". Choose a test whose prediction separates them.");return false;}
 C.bConcluded=true;C.Conclusion=Cause;Access.Add(D.Access);
 Why=TEXT("Conclusion supported by your retained tests.");return true;
}
int32 FS47FreeformCases::ClueReading(int32 Line,int32 Clue) const
{
 const FCase* C=Get(Line);if(!C||!Def(Line).Mystery||Clue<0||Clue>=ClueCount)return -1;
 const uint32 Draw=Mix(uint32(CaseSeed)^Mix(0x434c5545u+uint32(Line)));
 const int32 Straight=int32(Draw%uint32(ClueCount));
 const int32 Stray=(C->Truth+1+int32((Draw>>8)%uint32(CauseCount-1)))%CauseCount;
 return Clue==Straight?C->Truth:Stray;
}
FString FS47FreeformCases::ClueText(int32 Line,int32 Clue) const
{
 const int32 Reading=ClueReading(Line,Clue);
 return Reading<0?FString():FString(Def(Line).Mystery->Clues[Clue].Read[Reading]);
}
bool FS47FreeformCases::TrailCold(int32 Line,int32 Day) const
{
 const FCase* C=Get(Line);return C&&C->bOpen&&Def(Line).Mystery&&Day>C->OpenDay+Def(Line).Mystery->FreshDays;
}
bool FS47FreeformCases::ReplyDue(int32 Line,int32 Day) const
{
 const FCase* C=Get(Line);return C&&C->bConcluded&&!C->bReplied&&Def(Line).Mystery&&Day>C->ConcludeDay;
}
const FS47FreeformCases::FReplyDef* FS47FreeformCases::Reply(int32 Line) const
{
 const FCase* C=Get(Line);if(!C||!C->bConcluded||C->Conclusion<0||C->Conclusion>=CauseCount||!Def(Line).Mystery)return nullptr;
 return &Def(Line).Mystery->Replies[C->Conclusion][C->Truth];
}
void FS47FreeformCases::MarkReplied(int32 Line){if(Cases.IsValidIndex(Line))Cases[Line].bReplied=true;}
FString FS47FreeformCases::ConclusionText(int32 Line) const
{
 const FCase* C=Get(Line);
 if(!C||!C->bConcluded||C->Conclusion<0||C->Conclusion>=CauseCount)return TEXT("No supported conclusion is retained.");
 const FCaseDef& D=Def(Line);
 FString Text=FString::Printf(TEXT("SARO OPERATIONS / RECORDS REVIEW\nRE: %s\n\nRETAINED VALID TESTS\n"),D.Title);
 for(int32 T=0;T<TestCount;++T)
 {
  const FResult& R=C->Results[T];
  if(R.bDone)Text+=FString::Printf(TEXT("DAY %d / %s: %s\n"),R.Day,D.Tests[T].Name,*TestResultText(Line,T,R.bPresent));
  else Text+=FString::Printf(TEXT("%s: NOT ESTABLISHED BY A RETAINED VALID TEST\n"),D.Tests[T].Name);
 }
 Text+=TEXT("\nINTERPRETATION\n");
 if(D.Mystery)
  Text+=FString::Printf(TEXT("Filed as the station's assessment: %s. It rests on the traces and the tests listed above, and it has not been checked. It does not identify an unobserved source or person."),D.Causes[C->Conclusion]);
 else if(Line==2&&C->Conclusion==2)
  Text+=TEXT("The retained controls do not support the tested loose-fitting or service-gate explanations. The cause remains unexplained. This does not establish a visitor, an alien, or an identity. An intact gate excludes only a crossing through that gate during the valid control.");
 else
  Text+=FString::Printf(TEXT("%s is supported among the explanations tested. This is limited to the controls listed above; it does not identify an unobserved source or person."),D.Causes[C->Conclusion]);
 Text+=TEXT("\n\nKeep the original observations and test records. Tests without a retained valid result establish neither presence nor absence.");
 if(Line==0)Text+=TEXT(" A field logger is approved on loan for an independent comparison; collect it at EQUIPMENT CHECKOUT in the parts store.");
 return Text;
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
 {
  int32 WindSeed=0;while(Seeded(WindSeed,2)!=0)++WindSeed;
  FS47FreeformCases K;K.Reset(WindSeed);K.Open(2,1,0);K.RuleOutWind(2);
  Good&=K.Excluded(2,0)&&K.Get(2)->Truth==2&&K.Remaining(2)==2&&K.QuestionText(2)!=Def(2).Question;
  // The fastened plate already stopped the impacts, which closes the other two: wind stays, so that the case can end.
  FS47FreeformCases L;L.Reset(WindSeed);L.Open(2,1,0);bool bFirstValid=false;L.Plan(2,0,TEXT("J"));L.Record(2,0,1,true,false,bFirstValid);L.RuleOutWind(2);
  Good&=L.Remaining(2)==1&&!L.Excluded(2,0);
 }
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
   Good&=K.SetHunch(Line,Truth)&&K.Get(Line)->Hunch==Truth;
   if(!Def(Line).Mystery)Good&=!K.Conclude(Line,Truth,Why); // a correct guess alone is not supported
   Good&=K.Plan(Line,0,TEXT("J1"))&&!K.Plan(Line,1,TEXT("J2"));
   Good&=K.Record(Line,0,2,false,false,First)&&!First&&!K.Get(Line)->Results[0].bDone&&K.Get(Line)->Planned<0;
   Good&=K.Plan(Line,0,TEXT("J1"))&&K.Record(Line,0,2,true,PresentFor(Line,Truth,0),First)&&First;
   Good&=!K.Plan(Line,0,TEXT("J3"));
   Good&=K.Plan(Line,1,TEXT("J4"))&&K.Record(Line,1,3,true,PresentFor(Line,Truth,1),First)&&!First;
   Good&=K.Remaining(Line)==1&&!K.Excluded(Line,Truth);
   for(int32 Wrong=0;Wrong<CauseCount;++Wrong)if(Wrong!=Truth)Good&=!K.Conclude(Line,Wrong,Why);
   FS47FreeformCases R;R.Load(S47J::Read(S47J::Write(K.Save())),Seed);
   Good&=R.Get(Line)->Hunch==Truth;
   Good&=R.Get(Line)->bOpen&&R.Get(Line)->ValidTests==2&&R.Remaining(Line)==1&&R.Get(Line)->Truth==Truth;
   Good&=R.Conclude(Line,Truth,Why)&&R.HasAccess(Def(Line).Access)&&!R.Conclude(Line,Truth,Why);
   FS47FreeformCases Again;Again.Load(S47J::Read(S47J::Write(R.Save())),Seed);Good&=Again.Get(Line)->bConcluded&&Again.Get(Line)->Conclusion==Truth&&Again.HasAccess(Def(Line).Access);
   const FString Report=Again.ConclusionText(Line);
   Good&=Report.Contains(TEXT("DAY 2 / "))&&Report.Contains(TEXT("DAY 3 / "))&&Report.Contains(FString(Def(Line).Tests[2].Name)+TEXT(": NOT ESTABLISHED"));
   Good&=Report==R.ConclusionText(Line)&&!Report.Contains(TEXT("two separate sites recorded"));
   if(Line==2&&Truth==2)Good&=Report.Contains(TEXT("cause remains unexplained"));
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
 // A retained measurement is not silently rewritten to agree with the hidden cause.
 {
  FS47FreeformCases Evidence;Evidence.Reset(7);Evidence.Open(2,1,0);bool FirstResult=false;
  const bool Observed=!PresentFor(2,Evidence.Get(2)->Truth,0);
  Evidence.Plan(2,0,TEXT("retained-original"));Evidence.Record(2,0,2,true,Observed,FirstResult);
  FS47FreeformCases Restored;Restored.Load(S47J::Read(S47J::Write(Evidence.Save())),7);
  Good&=Restored.Get(2)->Results[0].bDone&&Restored.Get(2)->Results[0].bPresent==Observed;
 }
 // The first-test reward is granted once across all cases.
 FS47FreeformCases Both;Both.Reset(3);bool First=false;Both.Open(0,1,0);Both.Open(1,1,0);
 Both.Plan(0,1,TEXT("A"));Both.Record(0,1,1,true,true,First);Good&=First;Both.Plan(1,1,TEXT("B"));Both.Record(1,1,1,true,true,First);Good&=!First;
 Good&=LineForFinding(TEXT("pulse-review"))==0&&LineForFinding(TEXT("extra_pulse.day4"))==0&&LineForFinding(TEXT("extra_pulse.daylight"))==-1&&LineForFinding(TEXT("duplicate_print"))==1&&LineForFinding(TEXT("unsigned_telex.day3"))==1&&LineForFinding(TEXT("observer"))==2&&LineForFinding(TEXT("close_pass"))==-1&&LineForFinding(TEXT("peek-4"))==2&&LineForFinding(TEXT("flood-room-5-2"))==2&&LineForFinding(TEXT("door-visit-5"))==2&&LineForFinding(TEXT("door-marks-5"))==2;
 FS47FreeformCases W;W.Reset(1);W.Open(0,3,23.5*3600);
 Good&=!W.InRepeatWindow(0,3,23.5*3600)&&W.InRepeatWindow(0,4,.5*3600)&&!W.InRepeatWindow(0,4,2*3600)&&W.InRepeatWindow(0,5,22.1*3600)&&Clock(-1800)==TEXT("23:30");
 FS47FreeformCases Bad;Bad.Load(S47J::Read(TEXT("{\"v\":1,\"cases\":[{\"open\":true,\"planned\":99,\"openClock\":1e99,\"concluded\":true,\"conclusion\":7}]}")),5);
 Good&=Bad.Get(0)->Planned==-1&&Bad.Get(0)->OpenClock<=86399&&!Bad.Get(0)->bConcluded&&Bad.Get(0)->Conclusion==-1;
 // A mystery: an assessment on any evidence, possibly wrong; what is found leans different ways; the reply waits a day.
 {
  const FMysteryDef* Y=Def(MysteryLine).Mystery;Good&=Y!=nullptr&&Defs[0].Mystery==nullptr;
  Good&=LineForFinding(TEXT("carrier_saro"))==MysteryLine&&LineForFinding(TEXT("carrier_feeder"))==MysteryLine&&LineForFinding(TEXT("carrier_elsewhere"))==-1;
  TSet<int32> HonestAt,Strays;
  for(int32 Seed=0;Y&&Seed<256;++Seed)
  {
   FS47FreeformCases K;K.Reset(Seed);const int32 Truth=K.Get(MysteryLine)->Truth;
   const int32 A=K.ClueReading(MysteryLine,0),B=K.ClueReading(MysteryLine,1);
   // One place tells it straight and the other leans elsewhere, whichever way round.
   Good&=A!=B&&(A==Truth||B==Truth)&&A>=0&&B>=0&&A<CauseCount&&B<CauseCount;
   HonestAt.Add(A==Truth?0:1);Strays.Add((A==Truth?B:A)-Truth);
   // Text is drawn from the one shared pool: it never names a cause or says what it was.
   for(int32 Clue=0;Clue<ClueCount;++Clue)
   {
    const FString Trace=K.ClueText(MysteryLine,Clue);Good&=!Trace.IsEmpty();
    for(int32 Cause=0;Cause<CauseCount;++Cause)Good&=!Trace.Contains(Def(MysteryLine).Causes[Cause]);
   }
   FS47FreeformCases Again;Again.Load(S47J::Read(S47J::Write(K.Save())),Seed);Good&=Again.Get(MysteryLine)->Truth==Truth&&Again.ClueReading(MysteryLine,0)==A&&Again.ClueReading(MysteryLine,1)==B;
  }
  Good&=HonestAt.Num()==2&&Strays.Num()>=2;
  for(int32 Sent=0;Y&&Sent<CauseCount;++Sent)for(int32 Was=0;Was<CauseCount;++Was)
  {
   const FReplyDef& Answer=Y->Replies[Sent][Was];Good&=Answer.Text&&Answer.Text[0];
   // A wrong filing never pays. The unexplained, filed and right, pays nothing either: no reward for guessing its origin.
   if(Sent!=Was)Good&=Answer.Effect!=EReply::Spare;
   else Good&=Answer.Effect!=EReply::FeederFault&&Answer.Effect!=EReply::CallOut;
  }
  Good&=Y&&Y->Replies[2][2].Effect==EReply::None;
  // Filed with nothing but a hunch, against the truth: accepted, remembered, answered once, a day later.
  int32 ProbeSeed=0;for(FS47FreeformCases Probe;ProbeSeed<256;++ProbeSeed){Probe.Reset(ProbeSeed);if(Probe.Get(MysteryLine)->Truth==1)break;}
  FS47FreeformCases M1;M1.Reset(ProbeSeed);FString Reason;bool bFirstTest=false;
  Good&=!M1.Conclude(MysteryLine,0,Reason,3)&&M1.Open(MysteryLine,3,21*3600.);
  Good&=!M1.TrailCold(MysteryLine,5)&&M1.TrailCold(MysteryLine,6)&&!M1.ReplyDue(MysteryLine,9)&&M1.Reply(MysteryLine)==nullptr;
  // Off the beam the insulator's ticks stay, which rules out the third; the third cannot be filed after that.
  Good&=M1.Plan(MysteryLine,1,TEXT("J"))&&M1.Record(MysteryLine,1,3,true,PresentFor(MysteryLine,1,1),bFirstTest)&&M1.Excluded(MysteryLine,2)&&!M1.Conclude(MysteryLine,2,Reason,3)&&Reason.Contains(TEXT("contradicted"));
  // A plan still open when the filing goes in is released.
  Good&=M1.Plan(MysteryLine,0,TEXT("J2"))&&M1.Conclude(MysteryLine,0,Reason,3)&&M1.Get(MysteryLine)->bConcluded&&M1.Get(MysteryLine)->Conclusion==0&&M1.Get(MysteryLine)->Planned<0&&M1.Get(MysteryLine)->PlannedJob.IsEmpty();
  Good&=!M1.ReplyDue(MysteryLine,3)&&M1.ReplyDue(MysteryLine,4)&&!M1.Plan(MysteryLine,0,TEXT("J3"))&&!M1.Conclude(MysteryLine,1,Reason,3);
  const FString FiledText=M1.ConclusionText(MysteryLine);
  Good&=FiledText.Contains(Def(MysteryLine).Causes[0])&&FiledText.Contains(TEXT("not been checked"))&&!FiledText.Contains(Def(MysteryLine).Causes[1])&&!FiledText.Contains(TEXT("supported among"));
  Good&=Y&&M1.Reply(MysteryLine)==&Y->Replies[0][1]&&M1.Reply(MysteryLine)->Effect==EReply::FeederFault;
  FS47FreeformCases M2;M2.Load(S47J::Read(S47J::Write(M1.Save())),ProbeSeed);
  Good&=M2.Get(MysteryLine)->bConcluded&&M2.Get(MysteryLine)->Conclusion==0&&M2.Get(MysteryLine)->ConcludeDay==3&&M2.ReplyDue(MysteryLine,4)&&M2.Get(MysteryLine)->Results[1].bDone;
  M2.MarkReplied(MysteryLine);FS47FreeformCases M3;M3.Load(S47J::Read(S47J::Write(M2.Save())),ProbeSeed);Good&=M3.Get(MysteryLine)->bReplied&&!M3.ReplyDue(MysteryLine,9);
  // An old save has three cases and none of this: the new line waits, closed, and nothing else moves.
  FS47FreeformCases Old;Old.Load(S47J::Read(TEXT("{\"v\":1,\"access\":[\"access.fieldlogger\"],\"cases\":[{\"open\":true},{},{}]}")),ProbeSeed);
  Good&=Old.Get(MysteryLine)&&!Old.Get(MysteryLine)->bOpen&&Old.Get(MysteryLine)->Truth==1&&Old.Get(0)->bOpen&&!Old.ReplyDue(MysteryLine,9)&&Old.HasAccess(TEXT("access.fieldlogger"));
 }
 Lines.Add(FString::Printf(TEXT("%s / Freeform cases: four data-driven cases, observation leads, seeded hidden cause, any two tests separate three explanations, guesses refused until supported, invalid tests release the plan, one first-test reward, repeat window across midnight, Save/Continue keeps progress; a mystery takes an assessment that can be wrong, traces that lean different ways, its reply one day later and once, older saves unchanged"),Good?TEXT("PASS"):TEXT("FAIL")));
 return Good;
}

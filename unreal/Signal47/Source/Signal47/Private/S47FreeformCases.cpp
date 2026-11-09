#include "S47FreeformCases.h"
#include "S47Happenings.h"

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
    TEXT("Remote feeder, after the burst. Fresh tire tracks on the shoulder and one cut strand in the fence by the panel. A rancher after a stray could have done as much."),
    TEXT("Remote feeder, after the burst. A scorched smell at the panel and a dark streak down the top insulator. They all darken with age."),
    TEXT("Remote feeder, after the burst. No marks at all in the soft dirt around the panel, and the fence wire ticks when you touch it. Dry air does that out here.")}}},
  {{{TEXT("SOCORRO COUNTY SHERIFF\nRE: SARO REMOTE FEEDER\n\nA deputy sat on the county road at the hour you gave. He turned back two men with a pickup and a portable generator, stripping ground wire from the feeder run.\n\nThe co-op sends a spare control fuse with its thanks. It is in your depot."),ER::Spare},
    {TEXT("SOCORRO COUNTY SHERIFF\nRE: SARO REMOTE FEEDER\n\nThe deputy found no one and no tracks worth the name. He did think the top insulator looked burnt and passed that to the co-op, who put it on their list.\n\nIt let go before they got there. Measure the feeder at the service bay."),ER::FeederFault},
    {TEXT("SOCORRO COUNTY SHERIFF\nRE: SARO REMOTE FEEDER\n\nA deputy sat on the county road from midnight. Nobody came, no vehicle, no tracks.\n\nHe says the fence was ticking, and he would rather not make that drive again for this."),ER::None}},
   {{TEXT("CALICHE VALLEY ELECTRIC CO-OP / LINE RECORDS\nRE: SARO FEEDER CALL-OUT\n\nOur crew climbed the pole. The insulators are sound. They did find a cut fence strand and fresh tire tracks at the panel and have told the sheriff.\n\nThe call-out is charged to your station."),ER::CallOut},
    {TEXT("CALICHE VALLEY ELECTRIC CO-OP / LINE RECORDS\nRE: SARO FEEDER CALL-OUT\n\nOur crew found the top insulator cracked and tracking, and replaced it before it failed. No charge.\n\nA spare control fuse for the panel is in your depot."),ER::Spare},
    {TEXT("CALICHE VALLEY ELECTRIC CO-OP / LINE RECORDS\nRE: SARO FEEDER CALL-OUT\n\nOur crew climbed the pole. Insulators sound, line balanced, nothing on the recorder at the hour you gave. We cannot find your fault.\n\nThe call-out is charged to your station."),ER::CallOut}},
   {{TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE SAME MINUTE\n\nFiled as not accounted for, so nobody was sent. Overnight someone took the ground lead off the feeder panel.\n\nThe line is unstable. Measure it at the service bay."),ER::FeederFault},
    {TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE SAME MINUTE\n\nFiled as not accounted for, so nobody was sent. The top insulator on the feeder let go this morning.\n\nThe line is unstable. Measure it at the service bay."),ER::FeederFault},
    {TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE SAME MINUTE\n\nFiled as not accounted for, so nobody was sent. Nothing further has happened.\n\nA rancher on US-60 called the office to ask whether we had noticed anything at that minute. He did not say why."),ER::None}}}
,
  true
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
    {TEXT("SOCORRO GENERAL / OUTPATIENT\nRE: REFERRAL\n\nNight shifts, irregular meals, long drives alone. It happens to people in your line of work. Sleep more and do not drive tired.\n\nNo charge to the station for the first visit."),ER::None},
    {TEXT("SOCORRO GENERAL / OUTPATIENT\nRE: REFERRAL\n\nUnremarkable. There are some small marks you did not mention and could not account for. Probably nothing. Come back if they change.\n\nThe visit is charged to your station."),ER::CallOut}},
   {{TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE LOST HOURS\n\nFiled as not accounted for. Field service has since found the timing relay slipping and charged nothing for it.\n\nThe note stands as you wrote it."),ER::None},
    {TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE LOST HOURS\n\nFiled as not accounted for. Nobody has asked about it. You might see somebody about your sleep."),ER::None},
    {TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE LOST HOURS\n\nFiled as not accounted for. The survey office will let the portable magnetometer go for $25 if you want to look at that ground properly. Nobody here is going to ask why."),ER::None}}}
,
  // Critic round 5: what the player wakes to is not looked into. One trace (the clock at OPS, or the place gone back to in daylight)
  // is what opened the question; a filing counts with both, or with a control (Supported).
  true
 };
 // M7, weeks three and four. Somebody in Caliche gone from their post (FS47Townsfolk): their place in town, and what the station's
 // horizon channel logged toward town that night. No recording tests a person (UnavailableReason): the other place is the control.
 const FS47FreeformCases::FMysteryDef EmptyPost={
  3,TEXT("Reply: the empty post"),
  {{TEXT("post_empty"),TEXT("Their place in Caliche"),{
    TEXT("Their place in town: the till locked and the float gone with the key, the coat hook by the back door empty, the radio switched off at the wall. People who mean to go tidy up first. People who mean to come back do too."),
    TEXT("Their place in town: the door on the latch and two sets of tire tracks in the dust out front, one of them wide, like a county car or bigger. A cigarette end on the step, not a brand anybody here smokes. Plenty of people stop on that street."),
    TEXT("Their place in town: everything where it was left, the phone ringing to nobody, a cup half full on the counter with a skin on it. The clock on the wall says ten past three and has stopped there. Clocks stop.")},
    TEXT("not looked at yet. Their place in town, while it still stands as it was left.")},
   {TEXT("post_chart"),TEXT("SARO receiver log, that night"),{
    TEXT("Receiver log, the night they went: nothing on the horizon channel toward town but the town's own lights, steady all night, as on any night. Whatever happened there made no noise the dishes could hear."),
    TEXT("Receiver log, the night they went: a burst of ignition hash on the horizon channel toward town a little after three, then a second one, fainter, going away north. Engines start every night out there."),
    TEXT("Receiver log, the night they went: the horizon channel toward town goes flat for a minute or so after three, as if every light in Caliche had gone out at once. Nobody in town says they did. A relay can drop out the same way.")},
    TEXT("not looked at yet. The receiver log for that night is at the rack in the control room.")}},
  {{{TEXT("SOCORRO COUNTY SHERIFF\nRE: YOUR REPORT, CALICHE\n\nFiled as gone of their own accord. They telephoned the substation from Belen this morning: a sister ill, they said, and they would be back at work in a day or two.\n\nThe county thanks the station for not making a search of it."),ER::None},
    {TEXT("SOCORRO COUNTY SHERIFF\nRE: YOUR REPORT, CALICHE\n\nFiled as gone of their own accord, so nobody was sent. A neighbor has since reported a car on the street that night that nobody in town knows. It has not been found.\n\nWe would have liked to hear of it sooner."),ER::None},
    {TEXT("SOCORRO COUNTY SHERIFF\nRE: YOUR REPORT, CALICHE\n\nFiled as gone of their own accord. No relative has heard from them, no bus or ride out of town had them on it, and their car is where it always is.\n\nThe file stays open. It is not ours to close on your word."),ER::None}},
   {{TEXT("SOCORRO COUNTY SHERIFF\nRE: YOUR REPORT, CALICHE\n\nA deputy drove the county roads for two nights on your report. They had gone to Belen to see a sister, and say so.\n\nThe search is charged to your station."),ER::CallOut},
    {TEXT("SOCORRO COUNTY SHERIFF\nRE: YOUR REPORT, CALICHE\n\nA deputy found the car you describe at the motor court, with a man asleep in it: a relative with a grievance about money. He has been told to leave the county, and they have been driven home.\n\nThank you for a report that gave us something to look for."),ER::None},
    {TEXT("SOCORRO COUNTY SHERIFF\nRE: YOUR REPORT, CALICHE\n\nA deputy sat on the street for two nights. No car came, nobody was seen, and nothing is missing from the place.\n\nThe search is charged to your station. He says he will not do it a third night."),ER::CallOut}},
   {{TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE EMPTY POST\n\nFiled as not accounted for, so nobody was asked. Word in town is that they went to see family in Belen. Nobody here asked further."),ER::None},
    {TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE EMPTY POST\n\nFiled as not accounted for. The county has since had word of a strange car on the street that night. The substation asks that the station call next time, not file it."),ER::None},
    {TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE EMPTY POST\n\nFiled as not accounted for. Nobody in town will say where they were. One of them asked the attendant whether the station had heard anything that night, and did not say why."),ER::None}}},
  true
 };
 // The Socorro Sky Watch's bulletin (FS47Visitors' van) prints a night and a minute, and the receiver has a burst at it. The chart at
 // the rack, and the cairn on the rise where they said to watch from.
 const FS47FreeformCases::FMysteryDef PrintedMinute={
  2,TEXT("Reply: the printed minute"),
  {{TEXT("minute_chart"),TEXT("SARO receiver chart, the minute"),{
    TEXT("Receiver chart, ten to one. Hash in a block that starts on the minute and stops on the minute, the way a hand on a key would. A watch is all it takes to keep an appointment."),
    TEXT("Receiver chart, ten to one. The hash comes up with a step in the mains hum, as a pump or a heater does when something switches it on, and dies away as the load settles. Plenty on that line runs on a clock."),
    TEXT("Receiver chart, ten to one. The hash swells and fades as slowly as something crossing the beam, and the reference channel beside it stays flat. A drifting gain stage can draw the same shape.")},
    TEXT("not looked at yet. The chart at the rack keeps the night.")},
   {TEXT("minute_cairn"),TEXT("The cairn on the rise"),{
    TEXT("At the cairn: tire tracks up the rise and a dozen cigarette ends in a ring, and a length of thin wire tied off at the top stone, cut. Anybody watching the sky would bring a radio."),
    TEXT("At the cairn: nobody's tracks but your own. From the top, a light at the pump house on the county road goes on and off by itself every few minutes. Ranches leave lights on timers."),
    TEXT("At the cairn: the stones restacked, a hand higher than the survey tag says they stood, and no prints in the dust around it. The wind on the rise does not stack rocks. Somebody did, some time.")},
    TEXT("not looked at yet. The cairn on the rise, where the bulletin said to watch from; whatever is there will not last long.")}},
  {{{TEXT("SOUTHWEST RADIO MONITORING OFFICE / ALBUQUERQUE\nRE: SARO REPORT, UNLICENSED CARRIER\n\nOn your report we spoke to the Socorro Sky Watch. One of their members has a transmitter he is not licensed for, and keyed it at the minute in their bulletin to see whether your station would hear it.\n\nHe has been told. The bulletin will not print another minute."),ER::None},
    {TEXT("SOUTHWEST RADIO MONITORING OFFICE / ALBUQUERQUE\nRE: SARO REPORT, UNLICENSED CARRIER\n\nWe found no unlicensed set. The co-op tells us a stock-tank heater on the county line switches on at that hour every night.\n\nThe call-out is charged to your station."),ER::CallOut},
    {TEXT("SOUTHWEST RADIO MONITORING OFFICE / ALBUQUERQUE\nRE: SARO REPORT, UNLICENSED CARRIER\n\nNobody in the Sky Watch owns a transmitter, and nobody keyed anything. Their bulletin had the minute on good authority, they say, and they will not say whose.\n\nThe call-out is charged to your station."),ER::CallOut}},
   {{TEXT("CALICHE VALLEY ELECTRIC CO-OP / LINE RECORDS\nRE: SARO REPORT, A TIMER ON THE LINE\n\nOur crew checked every timer on the county line and found each on its own hours. A rancher on the line says some people with a van sat on the rise that night with a radio.\n\nThe call-out is charged to your station."),ER::CallOut},
    {TEXT("CALICHE VALLEY ELECTRIC CO-OP / LINE RECORDS\nRE: SARO REPORT, A TIMER ON THE LINE\n\nA stock-tank heater at the pump house on the county road, on a timer set for ten to one and arcing as it switched. Replaced. No charge.\n\nA spare control fuse for your panel is in your depot, with our thanks."),ER::Spare},
    {TEXT("CALICHE VALLEY ELECTRIC CO-OP / LINE RECORDS\nRE: SARO REPORT, A TIMER ON THE LINE\n\nNo timer on the line switches at that hour, and our recorder shows nothing at ten to one.\n\nThe call-out is charged to your station."),ER::CallOut}},
   {{TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE PRINTED MINUTE\n\nFiled as not accounted for. The Sky Watch's next bulletin says a member made contact with SARO at the minute they printed, and asks for donations."),ER::None},
    {TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE PRINTED MINUTE\n\nFiled as not accounted for, so nobody was sent. The heater at the pump house on the county road shorted out the next night and took the line with it.\n\nThe line is unstable. Measure it at the service bay."),ER::FeederFault},
    {TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE PRINTED MINUTE\n\nFiled as not accounted for. The Sky Watch wrote to ask whether you heard it. They had the minute from a man who rode with them once, they say, and none of them knows where he got it."),ER::None}}},
  true
 };
 // After a big night the military went over the ground before the player came (FS47Visitors::Cleared): the raked circle, and what the
 // horizon channel logged that morning. No recording tests what they came for.
 const FS47FreeformCases::FMysteryDef RakedGround={
  3,TEXT("Reply: the raked ground"),
  {{TEXT("raked_ground"),TEXT("The raked ground"),{
    TEXT("The raked ground: a wide circle combed smooth, and at its edge a strip of foil and a nylon line cut short. Aircraft drop things. So do weather balloons."),
    TEXT("The raked ground: combed smooth in a wide circle, and outside it the tracks of the two cars and a third set, a truck with a trailer, going off toward the county road. Somebody brought something to carry away."),
    TEXT("The raked ground: combed smooth in a wide circle, and in the middle of it the rake marks go round and round one place and stop short of it, as if whoever held the rake did not want to step there.")},
    TEXT("not looked at yet. The ground they went over, while the rake marks are still in it.")},
   {TEXT("raked_chart"),TEXT("SARO receiver log, that morning"),{
    TEXT("Receiver log, first light: rotor hash on the horizon channel, and under it a beacon tone that steps off a few minutes after six. Aircraft carry beacons, and they are switched off when they are found."),
    TEXT("Receiver log, first light: rotor hash on the horizon channel, then engine noise for an hour, low and heavy, like a truck working in low gear. Trucks work out there every day."),
    TEXT("Receiver log, first light: rotor hash on the horizon channel, and before it, for most of an hour, nothing at all on that bearing: not the wind, not the ranch generators, not the highway. Then the rotors, and everything back.")},
    TEXT("not looked at yet. The receiver log for that morning is at the rack in the control room.")}},
  {{{TEXT("RANGE OFFICE / PUBLIC AFFAIRS\nRE: YOUR INQUIRY\n\nA target drone from the range came down short on the night you give, and a recovery team went out for it at first light. The ground was dressed afterward, as is usual.\n\nThe range thanks the station for its discretion."),ER::None},
    {TEXT("RANGE OFFICE / PUBLIC AFFAIRS\nRE: YOUR INQUIRY\n\nThe range had nothing in the air that night and sent nobody. We have passed your report to the county, and the sheriff writes that he will charge the station for the patrol he sent."),ER::CallOut},
    {TEXT("RANGE OFFICE / PUBLIC AFFAIRS\nRE: YOUR INQUIRY\n\nThe range had nothing in the air that night. We have no record of a recovery at the place you give.\n\nPlease do not inquire further on this matter."),ER::None}},
   {{TEXT("SOCORRO COUNTY SHERIFF\nRE: SARO REPORT, VEHICLES ON STATE LAND\n\nThe vehicles were federal and had a right to be there. The deputy who drove out was turned back at the cattle guard.\n\nThe patrol is charged to your station."),ER::CallOut},
    {TEXT("SOCORRO COUNTY SHERIFF\nRE: SARO REPORT, VEHICLES ON STATE LAND\n\nThe trailer tracks you describe led to a man hauling scrap up the mine track, who had been picking over what was left out there before anyone came. He has been cited. The cars were not his.\n\nThank you for the tracks."),ER::None},
    {TEXT("SOCORRO COUNTY SHERIFF\nRE: SARO REPORT, VEHICLES ON STATE LAND\n\nThe deputy found no trailer, no third vehicle and nothing hauled away. He found the raked circle and did not walk into it.\n\nThe patrol is charged to your station."),ER::CallOut}},
   {{TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE RAKED GROUND\n\nFiled as not accounted for, so nobody was asked. The Ledger prints a rancher's story of a little jet with no pilot, found hung on his fence the same week."),ER::None},
    {TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE RAKED GROUND\n\nFiled as not accounted for. Somebody has been back to the place since with a truck and a trailer, and the raked circle is gone under their tracks."),ER::None},
    {TEXT("SARO OPERATIONS / FILE NOTE\nRE: THE RAKED GROUND\n\nFiled as not accounted for. Nobody has asked about it. The two cars were seen again on the highway two nights later, going north, slowly, with no lights on."),ER::None}}},
  true
 };
 // CASES-OPS-20261109: the senders above and below are made up. Real bodies (the observatory at Socorro, the federal radio regulator,
 // the county's electric cooperative) never sign for anything the story invents; the county sheriff stands as geography only.
 const FS47FreeformCases::FCaseDef Defs[FS47FreeformCases::CaseCount]={
 {
  TEXT("THE REPEATED INTERVAL"),
  TEXT("What produces the extra ticks between 9 and 13 seconds of the pulsar record? Three explanations fit the first original. Each test below predicts a different result for each of them."),
  FS47FreeformCases::EObservable::ExtraTicks,
  {TEXT("Feeder interference"),TEXT("Local intermittent transmitter"),TEXT("Follows the sky source")},
  {TEXT("Feeder"),TEXT("Transmitter"),TEXT("Sky")},
  {{TEXT("Generator power"),EC::Generator,TEXT("Fuel and start the backup generator by the service bay, then record the whole pulsar observation on generator power. One can of fuel runs it for six hours.")},
   {TEXT("Off-source pointing"),EC::OffSource,TEXT("The tracking PC points the array 0.6 degrees beside the pulsar. Record the full observation there.")},
   {TEXT("Repeat in the same window"),EC::Repeat,TEXT("Record the pulsar again on a later day, within 90 minutes of the clock time of the first original.")}},
  // Generator, OffSource, Repeat
  {{false,true,true},   // feeder-borne ticks vanish on generator power
   {true,true,false},   // the intermittent local carrier does not recur
   {true,false,true}},  // a sky-fixed source vanishes off the beam
  {TEXT("CALICHE VALLEY ELECTRIC CO-OP / LINE RECORDS\nRE: SARO RECEIVER INTERFERENCE\n\nMarch 1979: an arcing insulator on the SARO feeder, pole 31-B, produced periodic interference on station receivers until replaced. The pole was noted for recurrent tracking in wet weather.\n\nYour generator comparison has been forwarded to the line crew. A spare control fuse is in your depot."),
   TEXT("SOUTHWEST RADIO MONITORING OFFICE / ALBUQUERQUE\nRE: UNLICENSED INTERMITTENT CARRIER\n\nTwo ranchers near US-60 reported an intermittent carrier in 1984. The source was never located.\n\nYour repeat and generator records exclude the station itself. A portable field logger is approved on loan so the carrier can be recorded from a second site."),
   TEXT("SOUTHWEST RADIO OBSERVATORIES / TIMING DESK\nRE: SARO TIMING SHEET\n\nYour extra pulse group matches no catalogued companion and survives both power and repeat controls while vanishing off the beam.\n\nA source fixed on the sky rises 3 minutes 56 seconds earlier each night; anything on the ground keeps clock time. A field logger is approved on loan for an independent second site.")},
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
   TEXT("CALICHE VALLEY ELECTRIC CO-OP / FREQUENCY LOG\nRE: SARO CLOCK COMPLAINT\n\nThe feeder frequency sagged on the nights you list. Any clock driven from the mains loses time while the line is loaded.\n\nYour generator records show the offset vanish off the mains. Expect it again whenever the line is loaded."),
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
   {TEXT("Two simultaneous sites"),EC::FieldWork,TEXT("Set the second logger's contact pickups at the field post on the east array edge. It compares what shakes there with what shakes on the roof, over one night's window. Inspect it afterwards."),TEXT("simultaneous disturbance at both sites"),TEXT("no simultaneous disturbance"),TEXT("access.fieldlogger")}},
  // Roof, Access, TwoSites (present = Yes)
  {{false,false,false},  // wind: the fastened plate stops it, nobody crosses, only the roof reference moves
   {true,true,false},    // people: impacts continue, the seal is broken, the sequence follows one path
   {true,false,true}},   // unexplained: impacts continue, no crossing, both sites at once
  {TEXT("COUNTY BUILDING INSPECTOR / SOCORRO\nRE: SARO ANNEX ROOF\n\nThe roof plate you fastened was the last unsecured sheet on the 1962 annex. Wind loading on that roof is noted in the 1979 inspection.\n\nNo further action. Your sealed-gate record shows nothing crossing to the station."),
   TEXT("SOCORRO COUNTY SHERIFF\nRE: SARO SERVICE GATE\n\nYour broken seal and the track along the service road match reports of people taking copper and scrap from remote sites this spring.\n\nA deputy will drive past SARO on night patrol. Keep the gate sealed and photograph any vehicle."),
   TEXT("SARO OPERATIONS / FILE NOTE\nRE: VISITORS AROUND THE BASE\n\nThe fastened plate did not stop the impacts and the sealed gate stayed intact.\n\nNo person, vehicle or animal accounts for it. Do not approach it alone.")},
  TEXT("Records reply: visitors around the base"),TEXT("access.visitors")
 },
 {
  TEXT("THE SAME MINUTE"),
  TEXT("A burst of hash crossed the receiver one night, and the remote feeder on the county road may have something to do with it. Whichever you have seen, the other is worth a look: the chart at the rack, or the feeder after midnight, and neither lasts past noon. The traces need not agree, and nobody is going to tell you which reading is right. Test recordings show whether the extra ticks come back. File what you think it was: the reply comes the next morning, and acting on the wrong answer has a cost."),
  FS47FreeformCases::EObservable::ExtraTicks,
  {TEXT("Someone at the feeder"),TEXT("A failing line insulator"),TEXT("Not accounted for")},
  {TEXT("Someone"),TEXT("Insulator"),TEXT("Unaccounted")},
  {{TEXT("Generator power"),EC::Generator,TEXT("Fuel and start the backup generator by the service bay, then record the whole pulsar observation on generator power, off the feeder. One can of fuel runs it for six hours.")},
   {TEXT("Off-source pointing"),EC::OffSource,TEXT("The tracking PC points the array 0.6 degrees beside the pulsar. Record the full observation there.")},
   {TEXT("Repeat in the same window"),EC::Repeat,TEXT("Record the pulsar again on a later day, within 90 minutes of the clock time the case was opened.")}},
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
 },
 {
  TEXT("THE EMPTY POST"),
  TEXT("Somebody in Caliche is not at their post, and their place stands as if they had only stepped out. Look at it while it is like that, and look at what the station logged toward town that night: the receiver log is at the rack. No recording here tests a person; the traces need not agree. File what you think it was. The reply comes the next morning, and the county acts on what you file."),
  FS47FreeformCases::EObservable::ClockDelta,
  {TEXT("They left on their own"),TEXT("Somebody came for them"),TEXT("Not accounted for")},
  {TEXT("Left"),TEXT("Somebody"),TEXT("Unaccounted")},
  {{TEXT("Swap the timing relay"),EC::RelaySwap,TEXT("Fit one spare relay from the depot into the receiver timing path when the recording starts. The relay is used up.")},
   {TEXT("Generator power"),EC::Generator,TEXT("Fuel and start the backup generator by the service bay, then record the whole observation on generator power, off the mains.")},
   {TEXT("Sealed station"),EC::DoorsSealed,TEXT("Close both SARO doors and stay in the control room for the whole recording. Opening a door spoils the test.")}},
  // Never offered (UnavailableReason): kept separating, as every line's are, for the shared rules.
  {{false,true,true},
   {true,false,true},
   {true,true,false}},
  {TEXT(""),TEXT(""),TEXT("")},
  TEXT("Station log: the empty post"),TEXT("access.emptyPost"),&EmptyPost
 },
 {
  TEXT("THE PRINTED MINUTE"),
  TEXT("The Sky Watch's bulletin gave a night and a minute, and at that minute the receiver chart has a burst on it. Look at the chart at the rack, and at the cairn on the rise where they said to watch from; neither keeps long, and they need not agree. Test recordings show whether the extra ticks come back. File what you think it was: the reply comes the next morning, and acting on the wrong answer has a cost."),
  FS47FreeformCases::EObservable::ExtraTicks,
  {TEXT("Somebody keying a set on time"),TEXT("A timer on the county line"),TEXT("Not accounted for")},
  {TEXT("Somebody"),TEXT("Timer"),TEXT("Unaccounted")},
  {{TEXT("Generator power"),EC::Generator,TEXT("Fuel and start the backup generator by the service bay, then record the whole pulsar observation on generator power, off the feeder. One can of fuel runs it for six hours.")},
   {TEXT("Off-source pointing"),EC::OffSource,TEXT("The tracking PC points the array 0.6 degrees beside the pulsar. Record the full observation there.")},
   {TEXT("Repeat in the same window"),EC::Repeat,TEXT("Record the pulsar again on a later day, within 90 minutes of the clock time the case was opened.")}},
  // Generator, OffSource, Repeat
  {{true,true,false},   // a hand on a key keeps one appointment, not the next night's
   {false,true,true},   // what switches on the line stays on the line, and off the generator
   {true,false,true}},  // what arrives through the beam goes when the beam moves
  {TEXT(""),TEXT(""),TEXT("")},
  TEXT("Station log: the printed minute"),TEXT("access.printedMinute"),&PrintedMinute
 },
 {
  TEXT("THE RAKED GROUND"),
  TEXT("After a night out on the land somebody came at first light, a helicopter low and two cars at the way in, and went over the ground before you got there. Look at it while the rake marks are still in it, and at what the station heard that morning: the receiver log is at the rack. No recording here tests what they came for; the traces need not agree. File what you think it was. The reply comes the next morning."),
  FS47FreeformCases::EObservable::ClockDelta,
  {TEXT("Something of theirs came down"),TEXT("Somebody cleaning up after themselves"),TEXT("Not accounted for")},
  {TEXT("Theirs"),TEXT("Cleaning up"),TEXT("Unaccounted")},
  {{TEXT("Swap the timing relay"),EC::RelaySwap,TEXT("Fit one spare relay from the depot into the receiver timing path when the recording starts. The relay is used up.")},
   {TEXT("Generator power"),EC::Generator,TEXT("Fuel and start the backup generator by the service bay, then record the whole observation on generator power, off the mains.")},
   {TEXT("Sealed station"),EC::DoorsSealed,TEXT("Close both SARO doors and stay in the control room for the whole recording. Opening a door spoils the test.")}},
  // Never offered (UnavailableReason), as for the empty post.
  {{false,true,true},
   {true,false,true},
   {true,true,false}},
  {TEXT(""),TEXT(""),TEXT("")},
  TEXT("Station log: the raked ground"),TEXT("access.rakedGround"),&RakedGround
 }};
}

const FS47FreeformCases::FCaseDef& FS47FreeformCases::Def(int32 Line){return Defs[FMath::Clamp(Line,0,CaseCount-1)];}
int32 FS47FreeformCases::LineForFinding(const FString& Id)
{
 for(int32 Line=0;Line<CaseCount;++Line)if(const auto* Y=Defs[Line].Mystery)for(const auto& Clue:Y->Clues)if(DatedKey(Id,Clue.Finding))return Line;
 if(Id==TEXT("pulse-review")||DatedKey(Id,TEXT("extra_pulse")))return 0;
 if(DatedKey(Id,TEXT("time_disagreement"))||DatedKey(Id,TEXT("duplicate_print"))||DatedKey(Id,TEXT("unsigned_telex")))return 1;
 if(DatedKey(Id,TEXT("roof_steps"))||DatedKey(Id,TEXT("observer"))||DatedKey(Id,TEXT("landing_trace")))return 2;
 if(Id.Contains(TEXT("_daylight_d"))||Id.Contains(TEXT("_moved_d"))||Id.StartsWith(TEXT("tripcam_d"))||Id.StartsWith(TEXT("roadcar_b"))||Id.StartsWith(TEXT("taken-"))||Id.EndsWith(TEXT("_visit"))||Id.StartsWith(TEXT("peek-"))||Id.StartsWith(TEXT("flood-room-"))||Id.StartsWith(TEXT("door-visit-"))||Id.StartsWith(TEXT("door-marks-")))return 2;
 // What the night's events leave out on the land (FS47Happenings): leads for the visitors, not yet one cause.
 if(FS47Happenings::IsNightFinding(Id))return 2;
 return -1;
}
bool FS47FreeformCases::IsCaseRecord(const FString& Key)
{
 return Key.Len()>5&&Key.StartsWith(TEXT("case"),ESearchCase::CaseSensitive)&&FChar::IsDigit(Key[4]);
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
FString FS47FreeformCases::UnavailableReason(int32 Line,int32 Test,bool bSecondLogger)
{
 // M7: what has gone from a post in town, or what was raked over out on the land, is not on any recording the station can make.
 if(Line==PostLine||Line==RakedLine)return TEXT("No station recording tests this. The control is the other place: look at both, then file.");
 return Line==2&&Test==2&&!bSecondLogger?TEXT("Two-site contact recording is unavailable: the loan field logger measures local RF interference, not roof sounds. A paired contact recorder is not available. Choose the roof or gate control on CASE BOARD."):FString();
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
  // Older saves have no reference: the window goes from the opening, as it always did.
  const int32 Ref=S47J::Int(O,TEXT("refDay"));const double RefAt=S47J::Num(O,TEXT("refClock"));
  if(Ref>0&&FMath::IsFinite(RefAt)){C.RefDay=FMath::Clamp(Ref,1,36500);C.RefClock=FMath::Clamp(RefAt,0.,86399.);}else{C.RefDay=C.OpenDay;C.RefClock=C.OpenClock;}
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
  // CASES-OPS-20261109: a save from before the visitors' lock was fixed can hold results that contradict all three explanations
  // (the hidden cause moved after a result was kept). The latest result goes, one at a time, until one explanation survives them:
  // that test can be run again. The count of valid tests stands; the work was done.
  for(int32 Guard=0;Guard<TestCount&&!C.bConcluded;++Guard)
  {
   bool bAny=false;
   for(int32 Cause=0;Cause<CauseCount&&!bAny;++Cause){bool bFits=true;for(int32 T=0;T<TestCount;++T)bFits&=!C.Results[T].bDone||PresentFor(I,Cause,T)==C.Results[T].bPresent;bAny|=bFits;}
   if(bAny)break;
   int32 Latest=-1;for(int32 T=0;T<TestCount;++T)if(C.Results[T].bDone&&(Latest<0||C.Results[T].Day>=C.Results[Latest].Day))Latest=T;
   if(Latest<0)break;
   C.Results[Latest]=FResult();if(C.Planned==Latest){C.Planned=-1;C.PlannedJob.Empty();}
  }
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
  if(C.bOpen&&(C.RefDay!=C.OpenDay||C.RefClock!=C.OpenClock)){S47J::Set(X,TEXT("refDay"),C.RefDay);S47J::Set(X,TEXT("refClock"),C.RefClock);}
  if(C.bReplied)S47J::Set(X,TEXT("replied"),true);
  X->SetArrayField(TEXT("results"),R);List.Add(MakeShared<FJsonValueObject>(X));
 }
 O->SetArrayField(TEXT("cases"),List);return O;
}
bool FS47FreeformCases::Open(int32 Line,int32 Day,double DayClock)
{
 if(!Cases.IsValidIndex(Line)||Cases[Line].bOpen||Day<1||!FMath::IsFinite(DayClock))return false;
 FCase& C=Cases[Line];C.bOpen=true;C.OpenDay=Day;C.OpenClock=FMath::Clamp(FMath::Fmod(FMath::Max(0.,DayClock),86400.),0.,86399.);
 C.RefDay=C.OpenDay;C.RefClock=C.OpenClock;return true;
}
void FS47FreeformCases::SetReference(int32 Line,int32 Day,double DayClock)
{
 if(!Cases.IsValidIndex(Line)||!Cases[Line].bOpen||Cases[Line].bConcluded||Day<1||!FMath::IsFinite(DayClock))return;
 FCase& C=Cases[Line];for(const FResult& R:C.Results)if(R.bDone)return;
 C.RefDay=FMath::Min(Day,C.OpenDay);C.RefClock=FMath::Clamp(FMath::Fmod(FMath::Max(0.,DayClock),86400.),0.,86399.);
}
bool FS47FreeformCases::SetHunch(int32 Line,int32 Cause)
{
 if(!Cases.IsValidIndex(Line)||!Cases[Line].bOpen||Cases[Line].bConcluded||Cause < -1||Cause>=CauseCount)return false;
 Cases[Line].Hunch=Cause;return true;
}
bool FS47FreeformCases::Plan(int32 Line,int32 Test,const FString& JobId)
{
 if(!Cases.IsValidIndex(Line)||JobId.IsEmpty()||Test<0||Test>=TestCount)return false;FCase& C=Cases[Line];
 if(!C.bOpen||C.bConcluded||C.Planned>=0||C.Results[Test].bDone||Remaining(Line)<=1)return false;
 C.Planned=Test;C.PlannedJob=JobId;return true;
}
void FS47FreeformCases::CancelPlan(int32 Line){if(Cases.IsValidIndex(Line)){Cases[Line].Planned=-1;Cases[Line].PlannedJob.Empty();}}
bool FS47FreeformCases::Record(int32 Line,int32 Test,int32 Day,bool bValid,bool bPresent,bool& bFirstValid)
{
 bFirstValid=false;if(!Cases.IsValidIndex(Line))return false;FCase& C=Cases[Line];
 if(!C.bOpen||C.bConcluded||C.Planned!=Test||Test<0)return false;
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
 // The hidden cause follows: what the tests go on to show has to be able to end somewhere else. But a retained result is never
 // contradicted after the fact (CASES-OPS-20261109, the lock): the cause moves only to one that every result already kept agrees
 // with, the unexplained first. With none such it stays the wind, and the tests kept have already closed the other two, so the
 // wind is left as the one explanation to conclude (Excluded) and no further test is planned (Plan).
 if(C.Truth!=0)return;
 for(const int32 Cause:{2,1})
 {
  bool bFits=true;for(int32 T=0;T<TestCount;++T)bFits&=!C.Results[T].bDone||PresentFor(Line,Cause,T)==C.Results[T].bPresent;
  if(bFits){C.Truth=Cause;return;}
 }
}
FString FS47FreeformCases::QuestionText(int32 Line) const
{
 const FCase* C=Get(Line);
 if(C&&C->bBeyondWind)return TEXT("What is moving around SARO at night? Wind does not account for lost time or a room full of light. Roof impacts, a figure near the array and marks on the ground are retained as separate observations; they are not yet one cause. Each test below predicts a different result for each explanation that is left.");
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
 // As a filing does (CASES-OPS-20261109): a plan still open tests nothing once the case is concluded.
 C.bConcluded=true;C.Conclusion=Cause;C.Planned=-1;C.PlannedJob.Empty();Access.Add(D.Access);
 Why=TEXT("Conclusion supported by your retained tests.");return true;
}
bool FS47FreeformCases::Supported(int32 Line,const bool Kept[ClueCount]) const
{
 const FCase* C=Get(Line);if(!C||!C->bOpen)return false;
 const FMysteryDef* Y=Def(Line).Mystery;
 if(!Y||C->ValidTests>0)return true;
 int32 Traces=0;for(int32 I=0;I<ClueCount;++I)Traces+=Kept&&Kept[I]?1:0;
 return Traces>(Y->bOpensOnTrace?1:0);
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
  Text+=FString::Printf(TEXT("Filed as the station's assessment: %s. It rests on the traces and the tests listed above, and it has not been checked."),D.Causes[C->Conclusion]);
 else if(Line==2&&C->Conclusion==2)
  Text+=TEXT("The retained controls do not support the tested loose-fitting or service-gate explanations. The cause remains unexplained. An intact gate rules out only a crossing through that gate during the valid control.");
 else
  Text+=FString::Printf(TEXT("%s is supported among the explanations tested, and only by the controls listed above."),D.Causes[C->Conclusion]);
 Text+=TEXT("\n\nKeep the original observations and test records. Tests without a retained valid result establish neither presence nor absence.");
 if(Line==0)Text+=TEXT(" A field logger is approved on loan for an independent comparison; collect it at EQUIPMENT CHECKOUT in the parts store.");
 return Text;
}
bool FS47FreeformCases::InRepeatWindow(int32 Line,int32 Day,double DayClock) const
{
 // From the first original's own clock and day (SetReference), not from when the case was opened (CASES-OPS-20261109).
 const FCase* C=Get(Line);return C&&C->bOpen&&Day>C->RefDay&&FMath::IsFinite(DayClock)&&DayDelta(DayClock,C->RefClock)<=RepeatWindow;
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
 // CASES-OPS-20261109, the visitors' lock: every order of the three tests, with the taking (RuleOutWind) before any of them or
 // after any one or two, under every cause, never leaves the case with nothing to conclude, never contradicts a retained result,
 // and never offers a test once one explanation is left.
 {
  bool bNoLock=true;
  static const int32 Orders[6][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
  for(int32 Truth=0;Truth<CauseCount;++Truth)for(const auto& Order:Orders)for(int32 SwapAt=0;SwapAt<=TestCount;++SwapAt)
  {
   int32 Seed=0;while(Seeded(Seed,2)!=Truth)++Seed;
   FS47FreeformCases V;V.Reset(Seed);V.Open(2,1,0);bool bFirstValid=false;
   for(int32 Step=0;Step<=TestCount;++Step)
   {
    if(Step==SwapAt)V.RuleOutWind(2);
    if(Step==TestCount)break;
    const int32 Test=Order[Step];
    const bool bCan=V.Remaining(2)>1;
    if(!V.Plan(2,Test,TEXT("V"))){bNoLock&=!bCan||V.Get(2)->Results[Test].bDone;continue;}
    bNoLock&=bCan;
    // What the recording shows follows the hidden cause as it stands now, as StartAcquisition and the field tests have it.
    V.Record(2,Test,2+Step,true,PresentFor(2,V.Get(2)->Truth,Test),bFirstValid);
    for(int32 T=0;T<TestCount;++T)bNoLock&=!V.Get(2)->Results[T].bDone||PresentFor(2,V.Get(2)->Truth,T)==V.Get(2)->Results[T].bPresent;
   }
   bNoLock&=V.Remaining(2)>=1;
   // The case can always end: with one left it is concluded; with more, a test is still to be had.
   if(V.Remaining(2)==1){int32 Left=-1;for(int32 C=0;C<CauseCount;++C)if(!V.Excluded(2,C))Left=C;FString Why;bNoLock&=V.Conclude(2,Left,Why);}
   else{bool bOpenTest=false;for(int32 T=0;T<TestCount;++T)bOpenTest|=!V.Get(2)->Results[T].bDone;bNoLock&=bOpenTest;}
  }
  Good&=bNoLock;
  if(!bNoLock)Lines.Add(TEXT("FAIL Freeform cases: the visitors' case can lock: three tests with the wind ruled out in between"));
  // A conclusion with a plan still open clears the plan, and the planned test cannot be recorded after it.
  int32 Seed=0;while(Seeded(Seed,0)!=0)++Seed;
  FS47FreeformCases P;P.Reset(Seed);P.Open(0,1,0);bool bFirst=false;FString Why;
  // (the feeder: on generator power the ticks vanish, which closes the other two at once)
  Good&=P.Plan(0,0,TEXT("P1"))&&P.Record(0,0,2,true,PresentFor(0,0,0),bFirst)&&P.Remaining(0)==1&&!P.Plan(0,1,TEXT("P2"));
  // (off the beam they remain: only the sky is out, and the next test can still be planned)
  FS47FreeformCases Q;Q.Reset(Seed);Q.Open(0,1,0);
  Good&=Q.Plan(0,1,TEXT("Q1"))&&Q.Record(0,1,2,true,PresentFor(0,0,1),bFirst)&&Q.Remaining(0)==2&&Q.Plan(0,2,TEXT("Q2"));
  // A plan made before the deciding result (an older save, or a field plan) is cleared by the conclusion.
  FS47FreeformCases Y;Y.Reset(Seed);Y.Open(0,1,0);Y.Plan(0,1,TEXT("Y1"));Y.Record(0,1,2,true,true,bFirst);Y.Plan(0,2,TEXT("Y2"));
  Y.Cases[0].Results[0].bDone=true;Y.Cases[0].Results[0].bPresent=false;Y.Cases[0].Results[0].Day=3; // the deciding result kept meanwhile
  Good&=Y.Get(0)->Planned==2&&Y.Remaining(0)==1&&Y.Conclude(0,0,Why)&&Y.Get(0)->Planned<0&&Y.Get(0)->PlannedJob.IsEmpty();
  Good&=!Y.Record(0,2,4,true,true,bFirst)&&!Y.Get(0)->Results[2].bDone;
  // The repeat window goes from the first original's clock, not from the reel deck's: kept 23:50 on day 3, reviewed 03:00 day 4.
  FS47FreeformCases W;W.Reset(1);W.Open(0,4,3*3600.);W.SetReference(0,3,23*3600.+50*60);
  Good&=W.InRepeatWindow(0,4,23.5*3600)&&!W.InRepeatWindow(0,4,3*3600.)&&W.Get(0)->RefDay==3;
  FS47FreeformCases WR;WR.Load(S47J::Read(S47J::Write(W.Save())),1);Good&=WR.Get(0)->RefDay==3&&FMath::IsNearlyEqual(WR.Get(0)->RefClock,23*3600.+50*60)&&WR.InRepeatWindow(0,4,23.5*3600);
  // No later reference once a result is kept.
  FS47FreeformCases WT;WT.Reset(1);WT.Open(0,4,3*3600.);WT.Plan(0,1,TEXT("W"));WT.Record(0,1,4,true,true,bFirst);WT.SetReference(0,3,12*3600.);Good&=WT.Get(0)->RefDay==4;
  // A save locked by the old rule (wind's roof result, then a two-site result under the unexplained) loses its latest result.
  FS47FreeformCases Locked;Locked.Load(S47J::Read(TEXT("{\"v\":1,\"cases\":[{},{},{\"open\":true,\"openDay\":1,\"results\":[{\"done\":true,\"present\":false,\"day\":2},{},{\"done\":true,\"present\":true,\"day\":4}]}]}")),Seed);
  Good&=Locked.Remaining(2)>=1&&Locked.Get(2)->Results[0].bDone&&!Locked.Get(2)->Results[2].bDone;
  // Case records are the case file's own.
  Good&=IsCaseRecord(TEXT("case1-test2"))&&IsCaseRecord(TEXT("case4-reply"))&&!IsCaseRecord(TEXT("casefield"))&&!IsCaseRecord(TEXT("carrier_saro"));
 }
 // CASES-OPS-20261109: no real body signs for what the story invents.
 for(const FCaseDef& D:Defs)
 {
  TArray<const TCHAR*> Texts;for(const TCHAR* T:D.Telex)Texts.Add(T);
  if(D.Mystery)for(const auto& Row:D.Mystery->Replies)for(const FReplyDef& R:Row)Texts.Add(R.Text);
  for(const TCHAR* T:Texts)if(T){const FString Up=FString(T).ToUpper();Good&=!Up.Contains(TEXT("NRAO"))&&!Up.Contains(TEXT("FCC "))&&!Up.Contains(TEXT("SOCORRO ELECTRIC"));}
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
   // One deciding result already leaves one explanation: then no further test is planned (CASES-OPS-20261109).
   const bool bOneLeft=K.Remaining(Line)==1;
   if(bOneLeft)Good&=!K.Plan(Line,1,TEXT("J4"))&&K.Get(Line)->Planned<0;
   else Good&=K.Plan(Line,1,TEXT("J4"))&&K.Record(Line,1,3,true,PresentFor(Line,Truth,1),First)&&!First;
   Good&=K.Remaining(Line)==1&&!K.Excluded(Line,Truth)&&!K.Plan(Line,2,TEXT("J5"));
   for(int32 Wrong=0;Wrong<CauseCount;++Wrong)if(Wrong!=Truth)Good&=!K.Conclude(Line,Wrong,Why);
   FS47FreeformCases R;R.Load(S47J::Read(S47J::Write(K.Save())),Seed);
   Good&=R.Get(Line)->Hunch==Truth;
   Good&=R.Get(Line)->bOpen&&R.Get(Line)->ValidTests==(bOneLeft?1:2)&&R.Remaining(Line)==1&&R.Get(Line)->Truth==Truth;
   Good&=R.Conclude(Line,Truth,Why)&&R.HasAccess(Def(Line).Access)&&!R.Conclude(Line,Truth,Why);
   FS47FreeformCases Again;Again.Load(S47J::Read(S47J::Write(R.Save())),Seed);Good&=Again.Get(Line)->bConcluded&&Again.Get(Line)->Conclusion==Truth&&Again.HasAccess(Def(Line).Access);
   const FString Report=Again.ConclusionText(Line);
   Good&=Report.Contains(TEXT("DAY 2 / "))&&Report.Contains(TEXT("DAY 3 / "))!=bOneLeft&&Report.Contains(FString(Def(Line).Tests[2].Name)+TEXT(": NOT ESTABLISHED"));
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
 // M7: a filing on the same minute with only the trace that opened it has nothing behind it; the other place looked at, or a
 // control recorded, has. Critic round 5: the lost hours too, since waking up is no looking into anything: one trace is not enough,
 // the clock at OPS and the place both are. A tested line has its tests.
 {
  FS47FreeformCases Filed;Filed.Reset(4);FString Said;const bool NoTrace[ClueCount]={false,false},OneTrace[ClueCount]={true,false},TwoTraces[ClueCount]={true,true};
  Filed.Open(MysteryLine,3,0);Filed.Conclude(MysteryLine,0,Said,3);
  Good&=Filed.Get(MysteryLine)->bConcluded&&!Filed.Supported(MysteryLine,NoTrace)&&!Filed.Supported(MysteryLine,OneTrace)&&Filed.Supported(MysteryLine,TwoTraces);
  Filed.Open(LostLine,5,0);Filed.Conclude(LostLine,1,Said,5);
  Good&=Filed.Get(LostLine)->bConcluded&&!Filed.Supported(LostLine,NoTrace)&&!Filed.Supported(LostLine,OneTrace)&&Filed.Supported(LostLine,TwoTraces);
  FS47FreeformCases Tested;Tested.Reset(4);bool bFirstTest=false;Tested.Open(MysteryLine,3,0);Tested.Plan(MysteryLine,0,TEXT("J"));Tested.Record(MysteryLine,0,4,true,true,bFirstTest);Tested.Conclude(MysteryLine,2,Said,4);
  Good&=Tested.Supported(MysteryLine,OneTrace)&&!Tested.Supported(1,TwoTraces);
 }
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
 // M7: the three late lines. Each is a mystery that opens on one of its own traces, so a filing needs the other place too (or, for
 // the printed minute, a control); each has its own places and replies; what they say never names a cause; a wrong filing never pays
 // and a right one never charges; the two with nothing to record offer no test; and none of it says what Freeform must not.
 {
  static const TCHAR* const Far[]={TEXT("forty-seven"),TEXT("forty seven"),TEXT("47"),TEXT("tomas"),TEXT("tomás"),TEXT("silhouette"),TEXT("cable"),TEXT("alien"),TEXT("ufo"),TEXT("saucer"),TEXT("abduct"),TEXT("taken"),TEXT("1947"),TEXT("8.15"),TEXT("the event"),TEXT("old road"),TEXT("chevy"),TEXT("winnebago"),TEXT("grey"),TEXT("tyre")};
  TSet<FString> Ids;bool bLate=true,bClean=true;FString Offender;
  const auto Clean=[&](const TCHAR* Text){const FString Low=FString(Text).ToLower();for(const TCHAR* W:Far)if(Low.Contains(W)){bClean=false;Offender=FString(W)+TEXT(": ")+FString(Text).Left(50);}};
  for(int32 Line=0;Line<CaseCount;++Line)if(const FMysteryDef* Y=Defs[Line].Mystery)for(const FClueDef& Clue:Y->Clues)Ids.Add(Clue.Finding);
  for(int32 Line=LateFirst;Line<CaseCount;++Line)
  {
   const FCaseDef& D=Defs[Line];const FMysteryDef* Y=D.Mystery;
   bLate&=Y&&Y->bOpensOnTrace&&D.Access&&D.Access[0];
   if(!Y)continue;
   Clean(D.Title);Clean(D.Question);for(int32 C=0;C<CauseCount;++C){Clean(D.Causes[C]);Clean(D.Short[C]);}
   for(int32 I=0;I<ClueCount;++I)
   {
    const FClueDef& Clue=Y->Clues[I];bLate&=LineForFinding(Clue.Finding)==Line&&Clue.Hint&&Clue.Hint[0];
    for(const TCHAR* Read:Clue.Read){Clean(Read);for(int32 C=0;C<CauseCount;++C)bLate&=!FString(Read).Contains(D.Causes[C]);}
   }
   for(int32 Sent=0;Sent<CauseCount;++Sent)for(int32 Was=0;Was<CauseCount;++Was)
   {
    const FReplyDef& Answer=Y->Replies[Sent][Was];Clean(Answer.Text);bLate&=Answer.Text&&Answer.Text[0];
    if(Sent!=Was)bLate&=Answer.Effect!=EReply::Spare;else bLate&=Answer.Effect!=EReply::FeederFault&&Answer.Effect!=EReply::CallOut;
   }
   const bool bRecorded=Line==MinuteLine;
   for(int32 T=0;T<TestCount;++T)bLate&=UnavailableReason(Line,T).IsEmpty()==bRecorded;
   // Filed on the one trace that opened it: nothing behind it. The other place looked at too: something.
   FS47FreeformCases K;K.Reset(11);FString Said;const bool One[ClueCount]={true,false},Two[ClueCount]={true,true};
   K.Open(Line,13,.83*3600);K.Conclude(Line,2,Said,14);
   bLate&=K.Get(Line)->bConcluded&&!K.Supported(Line,One)&&K.Supported(Line,Two)&&!K.ReplyDue(Line,14)&&K.ReplyDue(Line,15)&&K.HasAccess(D.Access);
  }
  Good&=bLate&&bClean&&Ids.Num()==ClueCount*(CaseCount-3);
  if(!bClean)Lines.Add(TEXT("FAIL Freeform cases: a late case says what it must not: ")+Offender);
  // A save from before them has five lines: the three new ones wait, closed.
  FS47FreeformCases Old;Old.Load(S47J::Read(TEXT("{\"v\":1,\"cases\":[{\"open\":true},{},{},{},{}]}")),3);
  Good&=Old.Get(0)->bOpen&&Old.Get(PostLine)&&!Old.Get(PostLine)->bOpen&&!Old.Get(RakedLine)->bOpen;
 }
 Lines.Add(FString::Printf(TEXT("%s / Freeform cases: eight data-driven cases (three of them M7's late ones: the empty post, the printed minute, the raked ground), observation leads, seeded hidden cause, any two tests separate three explanations, guesses refused until supported, invalid tests release the plan, one first-test reward, repeat window across midnight, Save/Continue keeps progress; a mystery takes an assessment that can be wrong, traces that lean different ways, its reply one day later and once, older saves unchanged; the visitors' case never locks (three tests with the wind ruled out in between), no test once one explanation is left, a conclusion clears its plan and its test is refused after, the repeat window from the first original's clock, made-up senders"),Good?TEXT("PASS"):TEXT("FAIL")));
 return Good;
}

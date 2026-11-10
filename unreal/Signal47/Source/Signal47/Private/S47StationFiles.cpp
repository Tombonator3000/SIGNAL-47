#include "S47StationFiles.h"
#include "S47Script.h"

namespace S47StationFiles
{
 // Canon kept (10 Nov): 27 antennas S-01 to S-27, the 1962 annex, the March 1979 feeder trouble, the 1983 receivers, the 1984 carrier,
 // Station 01 of the survey, the hill man's years ('58, '66, '73, '79), initials at the rock back to 1951, population 412, the couch
 // at the gate, eleven years of field maintenance. New here: first antennas 1958, the last in 1971, one contract operator from 1984.
 static const FFile Files[]=
 {
  {TEXT("station-01"),TEXT("01 STATION HISTORY"),1,TEXT(R"TXT(SARO / STATION FILE 01
STATION HISTORY, SUMMARY FOR NEW STAFF
Revised January 1986

1955  The survey party at Station 01 reports its radios useless on some nights, for no reason it can find. The district asks why. Nobody can tell it.
1956  The district and the county agree to build a listening station on the flat north of Caliche. Cheap ground, dry air, and nothing out here to make radio noise. That was the argument.
1958  First six antennas in service. First night log.
1962  Annex built: records room, photo lab, east wing.
1971  Antenna S-27 in service. The array has not grown since.
1979  Standby generator installed after the feeder trouble of that spring. See file 02.
1983  Receivers and terminals replaced.
1984  Night operation reduced to one contract operator.

SARO has two tasks.

The first is the sky survey: the same sources, measured every night, year after year. That is what the county is told it pays for, and it is true.

The second has never had a line in the budget. Things are seen and heard over this valley that are not in any catalog. The station was put here because of them. Its job is to be listening, with a record that will stand, when they happen.

Do not explain this in town. They know.)TXT")},
  {TEXT("station-02"),TEXT("02 STANDBY GENERATOR"),1,TEXT(R"TXT(SARO / STATION FILE 02
MEMORANDUM
TO: All operators
FROM: Field maintenance
DATE: June 4, 1979
RE: Standby generator

The generator behind the control room is in service as of today. One can of fuel runs it six hours. Fuel is in the depot. If the can is empty, that is your fault and not mine.

Why it is there. Three times this spring the station lost the mains with nothing wrong on the feeder. No storm. No fault bell. The co-op walked the line and found it sound.

Each time the line recorder shows the same thing: the voltage sags in steps for most of a minute, the lamps dip, and then everything goes. Each time the night log has a light in it. And each time the receivers were dead while it was overhead, so we have a line in a log and no record.

So. When the lamps dip under a clear sky you do not go to the window. You go out the back and start the generator. Then you go to the window.

A station that is dark has seen nothing.

- D.)TXT")},
  {TEXT("station-03"),TEXT("03 THE VALLEY IN NORWAY"),3,TEXT(R"TXT(SARO / STATION FILE 03
MEMORANDUM
TO: The duty officer
FROM: Records
DATE: November 12, 1984
RE: The valley in Norway

You asked for what we have on Hessdalen.

It is a farming valley in the mountains of central Norway, a couple of hundred people. Since December 1981 the people there have reported lights: low along the ridges, some standing still for an hour, some crossing the valley faster than anything should. As many as twenty reports in a week.

Last winter a group of volunteers went up with instruments for five weeks: a radar, a magnetometer, a spectrum analyzer, cameras with gratings. They caught lights on film and on radar at the same moment. They did not find out what the lights are. They did establish that the lights are there, which is more than anyone has done here.

Compare. Reports from this valley go back to the first ranches. We have had antennas on this ground since 1958 and three hundred clear nights a year. Norway did in five weeks, with borrowed equipment, what this station has not done in twenty-six years. They went out and made pictures. We stayed in and listened.

Recommendation: the night operator's duties to include field work. A camera in the truck. Places, times, samples, photographs.

Whatever this valley is, it is this country's Hessdalen, and it has been at it longer.

Noted. Approved for the next contract. - W.)TXT")},
  {TEXT("station-04"),TEXT("04 OPERATOR'S TERMS"),1,TEXT(R"TXT(SARO / STATION FILE 04
CONTRACT NIGHT OPERATOR: TERMS OF ENGAGEMENT
Form 4-N, revised 1985

The operator is engaged by the district, by the week, and paid by the job filed.

The operator will:
1. Run the evening's observing jobs and register the results.
2. Keep the array in repair, with field maintenance.
3. Go out when the terminals or the telex report something on the land, and bring back what can be filed: a photograph, a sample, a reading, a time and a place.
4. Keep every original. A thing that was seen and not recorded was not seen.

The operator will not:
1. Discuss the station's records in town or with the newspaper.
2. Carry a story back in place of a record.

The operator is advised to be indoors by 02:00. The station has no opinion on why. The advice was added to the form in 1985, at the request of field maintenance.

Operators engaged under this form since 1984: seven.
Longest engagement: five months.
Shortest: one night.
The form does not ask why they leave.)TXT")},
  {TEXT("station-05"),TEXT("05 TOWN LEAFLET"),4,TEXT(R"TXT(SARO / STATION FILE 05
CLIPPING, KEPT BY RECORDS
Caliche Chamber of Commerce, visitor leaflet, 1985

WELCOME TO CALICHE, POP. 412
"A Nice Town Under a Big Sky"

Visitors sometimes ask about the lights. We say: what about them! Caliche has more clear nights than anyplace in the state, and on some of them the sky puts on a little extra. Most are airplanes, weather balloons or the evening star. The rest have never hurt anybody that we can name.

A FEW FRIENDLY TIPS
- If your car radio makes a noise it has not made before, it will stop. Keep driving.
- If your dash clock is wrong when you get to town, the diner has a good one. Set it and have the pie.
- Cattle all standing one way are just cattle.
- The dishes north of town are a science station. They listen. They do not send. Please do not leave furniture at the gate.
- Porch lights off by eleven is a custom, not a law.

Caliche: come for the sky, stay for the pie.

(In pencil, in the margin: "that we can name." Keep this. - W.))TXT")},
  {TEXT("station-06"),TEXT("06 FIELD MAINTENANCE"),5,TEXT(R"TXT(SARO / STATION FILE 06
PERSONNEL NOTE, FIELD MAINTENANCE
Duty officer's file. Not for circulation.

Dale. District field maintenance, on the SARO list since 1975. Eleven years. Nobody else on that list has reached four.

He is loud. He is right about the equipment more often than the manuals are. He has turned down two transfers to Socorro in writing, the second time in capitals.

He keeps his own log. It is not the station's log and he has not offered it. From what he orders and when, I judge that he has worked out for himself what the station is listening for. He has never said so. He asks instead why the generator memo was never withdrawn, why the operator's form has a clock time on it, and who signs for all the film.

He has been asked more than once why he stays. The answer on record: "Because the machines here are honest. Out there, I do not know. But the machines are honest."

He does not come in after dark. He does answer the telephone.

Assessment: keep him. Do not explain anything to him. He would only shout, and then he would be right.)TXT")},
  {TEXT("station-07"),TEXT("07 REPORTS BY YEAR"),2,TEXT(R"TXT(SARO / STATION FILE 07
REPORTS RECEIVED, BY YEAR (EXTRACT)
Kept by Records from telephone calls, the sheriff's substation and the station's own night log.

1951  Initials and a date cut in the rock the town has a name for. The earliest dated thing we have. No report with it.
1958  Spring. Cattle on a lease east of the station found standing along the east fence at dawn, every head. First week of observing.
1966  Radio noise on ranch sets, several nights. A federal man asked questions in the valley. He did not call here.
1973  A light over the playa on three nights running, seen from the station roof. Receivers down for maintenance. No record.
1979  Mains lost three times under a clear sky. One rancher reports an afternoon he cannot account for.
1980  Reports to the sheriff: 9.
1982  Reports to the sheriff: 14.
1984  Reports to the sheriff: 22. An unexplained carrier near the highway.
1985  Fall. Lights over the mesa, many callers.
1986  To April: 11. The sheriff's office has asked us to stop asking.

Every spring. Most years the fall as well. The town says ditch fires, trucks, the evening star. The town is sometimes right.

What this list does not have, for any year, is one photograph with a time and a place on the back.)TXT")},
  {TEXT("station-08"),TEXT("08 WHAT THE STATION KEEPS"),1,TEXT(R"TXT(SARO / STATION FILE 08
WHAT THE STATION KEEPS
Card for the night operator. Read it once.

The work goes in a circle, and it starts at the desks.

1. THE TERMINALS. OPS issues the evening's jobs at 08:00. TRACKING points the array, RECEIVER records, the PRINTER gives the original, STATUS shows what is broken. The terminals and the telex also tell you when something out on the land wants looking at: a place, a bearing, a time.

2. THE LAND. Take the truck, the field radio, the recorder and the camera. Go to the place. Look before you touch.

3. WHAT TO BRING BACK, in order of worth:
- a photograph, developed here, with its time and place;
- a sample, bagged, for the bench;
- a reading or a tape made on the spot;
- a time and a place in your own hand.
A story is worth nothing. Two of the above that agree are worth a case.

4. THE DESKS AGAIN. Develop the film in the photo lab. Register the finding on OPS. It goes to RECORDS, to PHOTOGRAPHS or to the CASE BOARD, and the board says what would settle it. Then the terminals send you out again.

You are paid for the jobs. The station is kept open for the rest.)TXT")},
 };
 int32 Count(){return UE_ARRAY_COUNT(Files);}
 const FFile& Get(int32 Index){return Files[FMath::Clamp(Index,0,Count()-1)];}
 FString Lint()
 {
  static const TCHAR* Not[]={TEXT("47"),TEXT("8.15"),TEXT("alien"),TEXT("ufo"),TEXT("saucer"),TEXT("abduct"),TEXT("roswell"),TEXT("reyes"),TEXT("ortiz"),TEXT("tomas"),TEXT("silhouette"),TEXT("cable"),TEXT("taken"),TEXT("caliche county"),TEXT("kinski"),
   TEXT("grey"),TEXT("colour"),TEXT("programme"),TEXT("tyre"),TEXT("petrol"),TEXT("whilst")};
  FString Bad;TSet<FString> Keys;
  for(const FFile& F:Files)
  {
   const FString T(F.Text);
   if(const FString Why=S47Script::LintText(T);!Why.IsEmpty())Bad+=FString::Printf(TEXT("%s: %s | "),F.Key,*Why);
   for(const TCHAR* W:Not)if(T.Contains(W))Bad+=FString::Printf(TEXT("%s: says '%s' | "),F.Key,W);
   if(Keys.Contains(F.Key)||F.Day<1||F.Day>7||T.Len()<300||T.Len()>1800||!T.StartsWith(FString::Printf(TEXT("SARO / STATION FILE %s"),*FString(F.Title).Left(2))))Bad+=FString::Printf(TEXT("%s: key, day, length or heading | "),F.Key);
   Keys.Add(F.Key);
   for(const TCHAR Ch:T)if(Ch>126){Bad+=FString::Printf(TEXT("%s: a character the tube's font may lack | "),F.Key);break;}
  }
  if(Get(1).Day!=1||Get(7).Day!=1)Bad+=TEXT("the generator memo and the operator's card are there from the first day | ");
  return Bad;
 }
}

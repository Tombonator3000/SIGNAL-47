// story/caliche.ts: Caliche's street, its people and the copies in Reyes's notebook.
#include "S47Caliche.h"
#include "S47Props.h"

#include "S47ReadAloud.h"
#include "Widgets/SNullWidget.h"
#include "S47UI.h"
#include "S47Audio.h"
#include "S47Image.h"
#include "S47EarlDialogue.h"
#include "S47Timeline.h"
#include "S47NpcDialogue.h"
#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47Seen.h"
#include "S47Story.h"
#include "S47Town.h"
#include "S47World.h"
#include "Widgets/SBoxPanel.h"

namespace
{
	struct FTopic
	{
		FString Id, Label;
		TArray<FS47TalkLine> Lines;
		FString Note;
		TOptional<FS47Doc> Paper;
		FString After;
		TArray<FString> Need;
		TOptional<double> NotBefore;
		FString Filed;
		bool bReel = false;
		FString Unless;
	};
	struct FPerson
	{
		FString Id, Title, Status, Bye;
		FS47PersonCard Card;
		TArray<FS47TalkLine> First, Again, Back;
		TArray<FTopic> Topics;
	};
	constexpr double DEPUTY_IN = 4 * 3600 + 30 * 60;
	const TCHAR* DEPUTY_BACK = TEXT("The deputy, back at his desk after half past four. He says he drove the loop and came straight back. He went out at a quarter to three.");
// story/caliche.ts: PEOPLE, in the TypeScript order, with the same lines and papers.
	const TArray<FPerson>& People()
	{
		static const TArray<FPerson> All = []
		{
			TArray<FPerson> Out;
			{
				FPerson P; P.Id = TEXT("attendant"); P.Title = TEXT("GAS STATION / THE ATTENDANT"); P.Status = TEXT("The night window, a hand's width open."); P.Bye = TEXT("Let him read");
				P.Card = { TEXT("The attendant"), TEXT("Caliche, the gas station"), TEXT("Reads behind the night window. \"Cold one tonight.\""), TEXT("portrait_attendant") };
				P.First = { { TEXT(""), TEXT("You knock on the night window. He takes his time, and slides it open a hand's width.") }, { TEXT("THE ATTENDANT"), TEXT("Pumps are locked till six. You want something from in here, I can pass it through.") }, { TEXT("THE ATTENDANT"), TEXT("Cold one tonight. Radio says it'll") }, { TEXT(""), TEXT("He coughs into his fist.") }, { TEXT("THE ATTENDANT"), TEXT("warm up by Wednesday.") } };
				P.Again = { { TEXT(""), TEXT("He slides the window open again.") }, { TEXT("THE ATTENDANT"), TEXT("Still out, huh.") } };
				{ FTopic T; T.Id = TEXT("roads"); T.Label = TEXT("Ask about the roads"); T.Lines = { { TEXT("YOU"), TEXT("Which way is quickest back up to the observatory?") }, { TEXT("THE ATTENDANT"), TEXT("Highway north. There is no other way that goes anywhere.") }, { TEXT("THE ATTENDANT"), TEXT("The old road's closed at the crossing. County says they'll grade it this summer. County says that every summer.") }, { TEXT("THE ATTENDANT"), TEXT("Your people come down for gas on Fridays. Nice folks. Don't say much.") } };
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("flyer"); T.Label = TEXT("Ask about the flyer on the door"); T.Lines = { { TEXT("YOU"), TEXT("SEEN LIGHTS? CALL THE NIGHT LINE.") }, { TEXT("THE ATTENDANT"), TEXT("Earl brings those. The radio fella, across the street.") }, { TEXT("THE ATTENDANT"), TEXT("I let him tape them up. He buys his gas here.") }, { TEXT("THE ATTENDANT"), TEXT("People call him. I couldn't tell you why they'd want to.") } };
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("old"); T.Label = TEXT("Ask if he has ever been out on the old road at night"); T.Lines = { { TEXT("YOU"), TEXT("You ever go out the old road at night?") }, { TEXT("THE ATTENDANT"), TEXT("Towed a rig in off it once. Last October.") }, { TEXT("THE ATTENDANT"), TEXT("Left here a quarter to two. Got back twenty-five past four.") }, { TEXT("THE ATTENDANT"), TEXT("It's a forty minute job.") }, { TEXT(""), TEXT("He says it like a sum he has done a lot of times and cannot get to come out.") }, { TEXT("THE ATTENDANT"), TEXT("Anyway. Cold one tonight.") } };
					T.Note = TEXT("The attendant towed a rig in off the old road one night last October. He left at a quarter to two and was back at twenty-five past four, for a forty minute job.");
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("tow"); T.Label = TEXT("Ask if he wrote a ticket for that tow"); T.Lines = { { TEXT("YOU"), TEXT("The rig you towed in off the old road last October. Did you write it up?") }, { TEXT(""), TEXT("He takes a spike of yellow carbons down off the nail by the register and works one loose from near the bottom.") }, { TEXT("THE ATTENDANT"), TEXT("Left here a quarter to two. Got back twenty-five past four.") }, { TEXT("THE ATTENDANT"), TEXT("It's a forty minute job.") }, { TEXT(""), TEXT("He pushes the carbon out under the window.") }, { TEXT("THE ATTENDANT"), TEXT("Keep it. I write down what goes in the till. I want it to add up.") }, { TEXT(""), TEXT("I keep the carbon.") } };
					T.Note = TEXT("The attendant gave me the carbon of the tow ticket from last October: out at 1:45, in at 4:25, 2.7 hours for a forty minute job. The line for who called it in is blank. Remarks: the driver in the cab said his engine quit at 11:58, and his watch said so too.");
					T.After = TEXT("old");
					T.Filed = TEXT("The tow ticket is in Papers.");
					FS47Doc Doc;
					Doc.Id = TEXT("towslip");
					Doc.Title = TEXT("Tow ticket / Caliche Service, No. 0419");
					Doc.Kind = TEXT("typed");
					Doc.Stamp = TEXT("TOW");
					Doc.Page = TEXT("CALICHE SERVICE\nGAS . DIESEL . TOWING\nCALICHE, N. MEX.          No. 0419\n\nDATE\n~ 10-20-85\nCALLED IN               BY\n~ 1:40 AM               ---\nOUT\n~ 1:45 AM\nVEHICLE\n~ Tractor & reefer trailer\nFROM\n~ Old Roswell Rd, past 8 mi post\nTO\n~ Mesa Diner lot\nREMARKS\n~ Driver in cab. Says engine quit 11:58.\n~ His watch says so too.\nIN\n~ 4:25 AM\nHOURS\n~ 2.7\n\n~ 40 min job. 16 mi. Where's the other 2 hrs?");
					Doc.Transcript = TEXT("A yellow carbon off the spike of tow tickets by the register at the gas station, filled in in ballpoint. A tractor and a refrigerated trailer, towed in off the old road from just past the eight-mile post, to the diner's lot. Out at a quarter to two, in at twenty-five past four: 2.7 hours, for a forty minute job.\n\nThe line for who called it in is blank. In the remarks: the driver was in the cab, and said his engine quit at 11:58. His watch said so too.\n\nUnder it, in the attendant's pencil, a sum that does not come out.");
					T.Paper = Doc;
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("earlier"); T.Label = TEXT("Tell him you were here before"); T.Lines = { { TEXT("YOU"), TEXT("I was here before. You told me that.") }, { TEXT("THE ATTENDANT"), TEXT("Wasn't me, then. I've been here since ten.") }, { TEXT(""), TEXT("He looks up at the clock over the register for a long time, as if it might say something else.") } };
					T.Need = { TEXT("repeat") };
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("phone"); T.Label = TEXT("Show him the nickel from the payphone"); T.Lines = { { TEXT("YOU"), TEXT("The payphone gave me this. After the printer stopped.") }, { TEXT(""), TEXT("You set the bright nickel on the sill. He turns it over without picking it up.") }, { TEXT("THE ATTENDANT"), TEXT("That phone takes dimes.") }, { TEXT("YOU"), TEXT("Nineteen forty-seven.") }, { TEXT("THE ATTENDANT"), TEXT("I can read it.") }, { TEXT(""), TEXT("He pushes it back with the edge of his magazine.") }, { TEXT("THE ATTENDANT"), TEXT("Keep it. I write down what goes in the till. I want it to add up.") }, { TEXT(""), TEXT("I copy his words into the notebook, beside the account of the call.") } };
					T.Note = TEXT("The attendant examined the nickel from the payphone. \"That phone takes dimes.\" He pushed it back and would not put it in the till.");
					T.After = TEXT("old");
					T.Need = { TEXT("ev:phone1"), TEXT("nickel") };
					FS47Doc Doc;
					Doc.Id = TEXT("witness-phone");
					Doc.Title = TEXT("Witness / The attendant and the payphone");
					Doc.Kind = TEXT("hand");
					Doc.Stamp = TEXT("REYES");
					Doc.Page = TEXT("REYES / NOTEBOOK COPY\n\nMAIN STREET PAYPHONE\n\nOn the line: the control room printer. Afterwards, a bright nickel dated 1947 in the coin return.\n\nTHE ATTENDANT, shown the coin:\n\"That phone takes dimes.\"\n\"Keep it. I write down what goes in the till. I want it to add up.\"\n\nHe pushed the coin back across the sill. I kept it.");
					Doc.Transcript = TEXT("Copied into my notebook after the conversation. My observation and the witness's words.\n\nMAIN STREET PAYPHONE\n\nOn the line: the control room printer. Afterwards, a bright nickel dated 1947 in the coin return.\n\nTHE ATTENDANT, shown the coin:\n\"That phone takes dimes.\"\n\"Keep it. I write down what goes in the till. I want it to add up.\"\n\nHe pushed the coin back across the sill. I kept it.");
					T.Paper = Doc;
					P.Topics.Add(MoveTemp(T)); }
				Out.Add(MoveTemp(P));
			}
			{
				FPerson P; P.Id = TEXT("deputy"); P.Title = TEXT("SHERIFF SUBSTATION / THE DEPUTY"); P.Status = TEXT("The door open on the chain, then off it."); P.Bye = TEXT("Let him work");
				P.Card = { TEXT("The deputy"), TEXT("Caliche, sheriff's substation"), TEXT("\"I don't believe in little men. I believe in maps.\""), TEXT("portrait_deputy") };
				P.First = { { TEXT(""), TEXT("You knock. A chair scrapes. He opens the door on the chain, looks at your jacket, and takes the chain off.") }, { TEXT("THE DEPUTY"), TEXT("Help you?") }, { TEXT("YOU"), TEXT("Reyes. From the observatory up the road.") }, { TEXT("THE DEPUTY"), TEXT("SARO. Sure. You folks lose something?") } };
				P.Again = { { TEXT("THE DEPUTY"), TEXT("Back again.") } };
				P.Back = { { TEXT(""), TEXT("He opens before you have finished knocking.") }, { TEXT("THE DEPUTY"), TEXT("Quiet night. Drove the loop out to the county line and back.") }, { TEXT(""), TEXT("There is dried blood under his nose, a thin dark line down to the lip. He has not wiped it. He does not seem to know it is there.") }, { TEXT(""), TEXT("He looks at his watch, and then at the clock on the wall, and does not say anything about either.") } };
				{ FTopic T; T.Id = TEXT("map"); T.Label = TEXT("Ask about the map with the pins"); T.Lines = { { TEXT("YOU"), TEXT("The map on your wall.") }, { TEXT("THE DEPUTY"), TEXT("Calls. Every call I get that I can't write up as anything.") }, { TEXT("THE DEPUTY"), TEXT("Red is engines dying. Blue is cattle. Yellow is lights. Green is people who lost track of the time.") }, { TEXT("THE DEPUTY"), TEXT("I don't believe in little men. I believe in maps.") }, { TEXT("THE DEPUTY"), TEXT("Most of them are all over. But you put enough pins in anything, and some of them start to line up.") } };
					T.Note = TEXT("The deputy's pins: red for engines that died, blue for cattle, yellow for lights, green for people who lost time. \"I don't believe in little men. I believe in maps.\"");
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("east"); T.Label = TEXT("Ask about the row going east"); T.Lines = { { TEXT("YOU"), TEXT("The row running east.") }, { TEXT("THE DEPUTY"), TEXT("The old road. Red, mostly. Engines. Out past where the county stops grading.") }, { TEXT("THE DEPUTY"), TEXT("Every one of those is a car somebody had to tow back.") }, { TEXT("THE DEPUTY"), TEXT("You want to know where, drive it with your eyes open. I'm not putting it in writing.") } };
					T.After = TEXT("map");
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("green"); T.Label = TEXT("Ask about the green pins"); T.Lines = { { TEXT("YOU"), TEXT("The green ones.") }, { TEXT("THE DEPUTY"), TEXT("Three of them are right here in town.") }, { TEXT("THE DEPUTY"), TEXT("The fella at the gas station. October nineteenth, last year. Went out to tow a rig and lost two hours he can't account for.") }, { TEXT("THE DEPUTY"), TEXT("A man whose wife sits out at the laundromat nights. March of '83.") }, { TEXT("THE DEPUTY"), TEXT("And a kid who called the radio station in September of '81. Family moved away after.") }, { TEXT("THE DEPUTY"), TEXT("Don't go asking them about it. They don't know they've got a pin.") } };
					T.Note = TEXT("Green pins in Caliche, people who lost time: the gas station attendant, October 19, 1985; the husband of the woman at the laundromat, March 1983; a boy who called the radio, September 1981.");
					T.After = TEXT("map");
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("nose"); T.Label = TEXT("Tell him there is blood under his nose"); T.Lines = { { TEXT("YOU"), TEXT("You have blood under your nose.") }, { TEXT(""), TEXT("He touches his lip and looks at his fingers. Nothing comes away. It is dry, and has been for a while.") }, { TEXT("THE DEPUTY"), TEXT("I drove the loop. I came straight back.") }, { TEXT(""), TEXT("He does not wipe it off.") } };
					T.Note = TEXT("The deputy, back at his desk after half past four: dried blood under his nose, and he had not noticed. When I told him, he touched it, looked at his fingers, and said he drove the loop and came straight back.");
					T.NotBefore = 16200;
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("car"); T.Label = TEXT("Tell him his patrol car was empty"); T.Lines = { { TEXT("YOU"), TEXT("Your car was on the shoulder north of town. Engine running, door open. Nobody in it.") }, { TEXT("THE DEPUTY"), TEXT("I drove the loop. I came straight back.") }, { TEXT("YOU"), TEXT("Your hat was on the seat.") }, { TEXT(""), TEXT("He looks at the hat hanging beside the door.") }, { TEXT("THE DEPUTY"), TEXT("Write down where you saw it.") }, { TEXT(""), TEXT("He opens his logbook, then lays a loose sheet over the page.") }, { TEXT("THE DEPUTY"), TEXT("Here. In your own words.") }, { TEXT(""), TEXT("I write it on his sheet and keep a copy in the notebook.") } };
					T.Note = TEXT("The deputy, back at his desk: I described his empty patrol car and the hat on its seat. He said he drove the loop and came straight back, then asked for my account on a separate sheet.");
					T.After = TEXT("green");
					T.Need = { TEXT("ev:car") };
					T.NotBefore = 16200;
					FS47Doc Doc;
					Doc.Id = TEXT("witness-car");
					Doc.Title = TEXT("Witness / The empty patrol car");
					Doc.Kind = TEXT("hand");
					Doc.Stamp = TEXT("REYES");
					Doc.Page = TEXT("REYES / NOTEBOOK COPY\n\nHIGHWAY NORTH OF CALICHE\n\nThe deputy's car on the shoulder. Engine running, driver's door open, nobody in it. His hat was on the seat.\n\nTHE DEPUTY, back at the substation:\n\"I drove the loop. I came straight back.\"\n\nWhen I mentioned the hat, he looked at the one hanging beside the door. He asked for my account on a loose sheet. This is my notebook copy.");
					Doc.Transcript = TEXT("Copied into my notebook after the conversation. My observation and the witness's words.\n\nHIGHWAY NORTH OF CALICHE\n\nThe deputy's car on the shoulder. Engine running, driver's door open, nobody in it. His hat was on the seat.\n\nTHE DEPUTY, back at the substation:\n\"I drove the loop. I came straight back.\"\n\nWhen I mentioned the hat, he looked at the one hanging beside the door. He asked for my account on a loose sheet. This is my notebook copy.");
					T.Paper = Doc;
					P.Topics.Add(MoveTemp(T)); }
				Out.Add(MoveTemp(P));
			}
			{
				FPerson P; P.Id = TEXT("host"); P.Title = TEXT("1420 AM / EARL"); P.Status = TEXT("A record on. Three minutes and forty seconds."); P.Bye = TEXT("Let him go back in");
				P.Card = { TEXT("Earl"), TEXT("1420 AM, the Night Line"), TEXT("Keeps every call on reel but one."), TEXT("portrait_host") };
				P.First = { { TEXT(""), TEXT("You lean on the buzzer. After a while the door opens on a man with his headphones pushed down round his neck.") }, { TEXT("EARL"), TEXT("I've got a record on. Three minutes and forty seconds, so talk fast.") } };
				P.Again = { { TEXT("EARL"), TEXT("You again. Song and a half this time.") } };
				{ FTopic T; T.Id = TEXT("calls"); T.Label = TEXT("Ask what people call in about"); T.Lines = { { TEXT("YOU"), TEXT("What do people call in about?") }, { TEXT("EARL"), TEXT("Lights, mostly. Cattle, some nights. Engines.") }, { TEXT("EARL"), TEXT("And missing time. You read that book? Everybody's read that book now.") }, { TEXT("EARL"), TEXT("Half of them, I think they read it first and then it happened to them.") }, { TEXT("EARL"), TEXT("The other half never read anything in their lives.") } };
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("visitors"); T.Label = TEXT("Ask what he thinks they are"); T.Lines = { { TEXT("YOU"), TEXT("What do you think they are?") }, { TEXT("EARL"), TEXT("On the air I call them our visitors. It keeps it friendly.") }, { TEXT("EARL"), TEXT("Off the air I don't call them anything.") } };
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("tape"); T.Label = TEXT("Ask if he keeps the calls"); T.Lines = { { TEXT("YOU"), TEXT("Do you keep the calls?") }, { TEXT("EARL"), TEXT("Every one, on reel. Twelve years of them.") }, { TEXT("EARL"), TEXT("Except one. September of '81, a boy called in. I've never played it back.") }, { TEXT("EARL"), TEXT("I wasn't on the air that night. The station was off.") }, { TEXT(""), TEXT("He looks back at the studio, where the reels are turning.") }, { TEXT("EARL"), TEXT("Record's ending.") } };
					T.Note = TEXT("Earl at 1420 AM keeps every call on reel. One he has never played: September 1981, a boy, on a night the station was off the air.");
					T.After = TEXT("calls");
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("boy"); T.Label = TEXT("Ask about the boy you heard tonight"); T.Lines = { { TEXT("YOU"), TEXT("I heard the boy on your show tonight. The star with the little tail.") }, { TEXT("EARL"), TEXT("You heard him too.") }, { TEXT(""), TEXT("He pulls a programme sheet from his pocket. A line in pencil reads BOY / COMET. The space for a caller's number is blank.") }, { TEXT("EARL"), TEXT("I put the receiver down and tried the line again. Dial tone. Just a phone.") }, { TEXT("YOU"), TEXT("Like the call you kept from eighty-one?") }, { TEXT("EARL"), TEXT("I haven't played that one. I told you.") }, { TEXT(""), TEXT("He folds the sheet along the same crease.") }, { TEXT("EARL"), TEXT("I wrote this one down.") }, { TEXT(""), TEXT("I copy the entry and his words into the notebook.") } };
					T.Note = TEXT("Earl showed me tonight's programme sheet: BOY / COMET in pencil, no caller number. He checked the line afterwards and got an ordinary dial tone. He still has not played the 1981 reel.");
					T.After = TEXT("tape");
					T.Unless = TEXT("ln:reel");
					T.Need = { TEXT("ev:boy") };
					FS47Doc Doc;
					Doc.Id = TEXT("witness-boy");
					Doc.Title = TEXT("Witness / Earl's programme sheet");
					Doc.Kind = TEXT("hand");
					Doc.Stamp = TEXT("REYES");
					Doc.Page = TEXT("REYES / NOTEBOOK COPY\n\n1420 AM / NIGHT LINE\n\nI heard a boy call about a star with a little tail, low in the south.\n\nEARL'S PROGRAMME SHEET, copied by hand:\nBOY / COMET\nCaller number: blank.\n\nEARL:\n\"I put the receiver down and tried the line again. Dial tone. Just a phone.\"\n\nHe still has not played the reel from September 1981. I have not heard that reel.");
					Doc.Transcript = TEXT("Copied into my notebook after the conversation. My observation and the witness's words.\n\n1420 AM / NIGHT LINE\n\nI heard a boy call about a star with a little tail, low in the south.\n\nEARL'S PROGRAMME SHEET, copied by hand:\nBOY / COMET\nCaller number: blank.\n\nEARL:\n\"I put the receiver down and tried the line again. Dial tone. Just a phone.\"\n\nHe still has not played the reel from September 1981. I have not heard that reel.");
					T.Paper = Doc;
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("reel"); T.Label = TEXT("Tell him the date: September 22, 1981"); T.Lines = { { TEXT("YOU"), TEXT("September twenty-second, 1981. Eight minutes to two in the morning.") }, { TEXT(""), TEXT("He looks at you for a long moment. He does not ask how you know.") }, { TEXT(""), TEXT("He goes back in. Through the glass you watch him take a box down off the top shelf and thread the reel on the second deck. He turns the monitor up and leaves the door open.") } };
					T.After = TEXT("tape");
					T.Need = { TEXT("ln:81") };
					T.bReel = true;
					P.Topics.Add(MoveTemp(T)); }
				Out.Add(MoveTemp(P));
			}
			{
				FPerson P; P.Id = TEXT("radio_woman"); P.Title = TEXT("LAUNDROMAT / THE WOMAN"); P.Status = TEXT("Hiss from the radio in her lap."); P.Bye = TEXT("Leave her to it");
				P.Card = { TEXT("The woman on the bench"), TEXT("Caliche, outside the laundromat"), TEXT("A radio tuned between two stations. \"It's louder tonight.\""), TEXT("portrait_radio_woman") };
				P.First = { { TEXT(""), TEXT("A transistor radio in her lap, tuned between two stations. Hiss, and under it something that is not quite a voice.") }, { TEXT("THE WOMAN"), TEXT("It's louder tonight.") }, { TEXT(""), TEXT("She does not look at you when she says it.") } };
				P.Again = { { TEXT("THE WOMAN"), TEXT("Still louder.") } };
				{ FTopic T; T.Id = TEXT("radio"); T.Label = TEXT("Ask what she is listening to"); T.Lines = { { TEXT("YOU"), TEXT("What are you listening to?") }, { TEXT("THE WOMAN"), TEXT("Nothing. That's the point. In between.") }, { TEXT("THE WOMAN"), TEXT("My husband said you could hear it in between, if you were quiet enough.") }, { TEXT("THE WOMAN"), TEXT("Most nights it's just this. Tonight it's louder.") } };
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("husband"); T.Label = TEXT("Ask about her husband"); T.Lines = { { TEXT("YOU"), TEXT("Your husband.") }, { TEXT("THE WOMAN"), TEXT("He went out to look at the cattle one night. March, three years ago.") }, { TEXT("THE WOMAN"), TEXT("Forty-seven minutes. I had the clock right there on the stove.") }, { TEXT("THE WOMAN"), TEXT("He came back in and sat down and took off his boots, and he never said one word about it. Not then, not since.") }, { TEXT("THE WOMAN"), TEXT("He sleeps fine. I don't.") } };
					T.Note = TEXT("The woman on the laundromat bench: her husband went out to the cattle one night in March 1983 and came back 47 minutes later. He has never said a word about it.");
					T.After = TEXT("radio");
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("why"); T.Label = TEXT("Ask why she sits out here"); T.Lines = { { TEXT("YOU"), TEXT("Why sit out here?") }, { TEXT("THE WOMAN"), TEXT("Washer's broke at home.") }, { TEXT(""), TEXT("There is no basket on the bench beside her.") } };
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("cattle"); T.Label = TEXT("Tell her about the missing cow"); T.Lines = { { TEXT("YOU"), TEXT("The pasture south of the motel. There were nine. Now there are eight, and a ring of frost in the grass.") }, { TEXT("THE WOMAN"), TEXT("You counted them?") }, { TEXT("YOU"), TEXT("Both times.") }, { TEXT(""), TEXT("She turns the radio down. The hiss is still there, very faint.") }, { TEXT("THE WOMAN"), TEXT("Write both numbers. Before you start thinking you counted wrong.") }, { TEXT("YOU"), TEXT("Did your husband find one missing?") }, { TEXT("THE WOMAN"), TEXT("He never said.") }, { TEXT(""), TEXT("She turns the radio back up.") }, { TEXT(""), TEXT("I copy both counts and her words into the notebook.") } };
					T.Note = TEXT("I told the woman about the pasture: nine cattle, then eight, and the frost ring. She asked whether I counted both times. Her husband never told her what he saw that night.");
					T.After = TEXT("husband");
					T.Need = { TEXT("ev:cows") };
					FS47Doc Doc;
					Doc.Id = TEXT("witness-cows");
					Doc.Title = TEXT("Witness / Two counts at the pasture");
					Doc.Kind = TEXT("hand");
					Doc.Stamp = TEXT("REYES");
					Doc.Page = TEXT("REYES / NOTEBOOK COPY\n\nPASTURE SOUTH OF THE MOTEL\n\nFirst count: nine cattle.\nSecond count: eight. A ring of flattened grass, white with frost, where the ninth stood.\n\nTHE WOMAN AT THE LAUNDROMAT:\n\"You counted them?\"\n\"Write both numbers. Before you start thinking you counted wrong.\"\n\nI asked if her husband found one missing the night he went out to the cattle in March 1983.\n\"He never said.\"");
					Doc.Transcript = TEXT("Copied into my notebook after the conversation. My observation and the witness's words.\n\nPASTURE SOUTH OF THE MOTEL\n\nFirst count: nine cattle.\nSecond count: eight. A ring of flattened grass, white with frost, where the ninth stood.\n\nTHE WOMAN AT THE LAUNDROMAT:\n\"You counted them?\"\n\"Write both numbers. Before you start thinking you counted wrong.\"\n\nI asked if her husband found one missing the night he went out to the cattle in March 1983.\n\"He never said.\"");
					T.Paper = Doc;
					P.Topics.Add(MoveTemp(T)); }
				Out.Add(MoveTemp(P));
			}
			{
				FPerson P; P.Id = TEXT("souvenir_man"); P.Title = TEXT("SAUCER CRASH 1947 / THE SOUVENIR MAN"); P.Status = TEXT("Ash tapped off the step."); P.Bye = TEXT("Leave him his smoke");
				P.Card = { TEXT("The souvenir man"), TEXT("Caliche, Saucer Crash 1947"), TEXT("Knows the whole story the way the book tells it. Has never seen a thing himself."), TEXT("portrait_souvenir_man") };
				P.First = { { TEXT("THE SOUVENIR MAN"), TEXT("Shop's closed, friend. Open at ten. Tours on Saturdays.") }, { TEXT(""), TEXT("He taps the ash off the end of his cigarette onto the step.") } };
				P.Again = { { TEXT("THE SOUVENIR MAN"), TEXT("Still closed.") } };
				{ FTopic T; T.Id = TEXT("crash"); T.Label = TEXT("Ask about the crash"); T.Lines = { { TEXT("YOU"), TEXT("The crash. 1947.") }, { TEXT("THE SOUVENIR MAN"), TEXT("Summer of '47. A rancher out north finds a pasture full of junk. Foil you can't crease. Little beams with writing on them.") }, { TEXT("THE SOUVENIR MAN"), TEXT("The Army says flying disc. Same day, they say weather balloon.") }, { TEXT("THE SOUVENIR MAN"), TEXT("Then nothing for thirty years, till the book.") }, { TEXT("THE SOUVENIR MAN"), TEXT("The boys from '47. That's what I call them. The boys.") } };
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("seen"); T.Label = TEXT("Ask if he has ever seen anything"); T.Lines = { { TEXT("YOU"), TEXT("Have you ever seen anything yourself?") }, { TEXT("THE SOUVENIR MAN"), TEXT("Me?") }, { TEXT(""), TEXT("He takes a long time over the cigarette.") }, { TEXT("THE SOUVENIR MAN"), TEXT("Forty years on this street. Everybody I sell to has a story. Lights. Cattle. The hum.") }, { TEXT("THE SOUVENIR MAN"), TEXT("I sit out here every night it's clear. Never seen a thing.") }, { TEXT("THE SOUVENIR MAN"), TEXT("You'd think they'd come by the shop once. Out of courtesy.") } };
					T.Note = TEXT("The souvenir man sits out every clear night and has never seen anything. \"You'd think they'd come by the shop once.\"");
					T.After = TEXT("crash");
					P.Topics.Add(MoveTemp(T)); }
				{ FTopic T; T.Id = TEXT("alien"); T.Label = TEXT("Ask about the cardboard alien"); T.Lines = { { TEXT("YOU"), TEXT("The cardboard alien in the window.") }, { TEXT("THE SOUVENIR MAN"), TEXT("Had it made in Albuquerque. Kids take it about once a month. It always turns up.") }, { TEXT("THE SOUVENIR MAN"), TEXT("Last time it was out on the highway north of town. Standing up, facing the traffic.") }, { TEXT("THE SOUVENIR MAN"), TEXT("Somebody's idea of a joke.") } };
					P.Topics.Add(MoveTemp(T)); }
				Out.Add(MoveTemp(P));
			}
			return Out;
		}();
		return All;
	}
	const FPerson* Person(const FString& W) { return People().FindByPredicate([&](const FPerson& P) { return P.Id == W; }); }
	bool Asked(const FS47TownSeen& S, const FString& W, const FString& Id) { return S.Contains(TEXT("ask:") + W + TEXT(":") + Id); }
	bool AvailableTopic(const FS47TownSeen& S, const FString& W, const FTopic& T, double Day, bool bReelReady)
	{
		if (Asked(S, W, T.Id) || (!T.After.IsEmpty() && !Asked(S, W, T.After)) || (T.bReel && !bReelReady) || (!T.Unless.IsEmpty() && S.Contains(T.Unless))) return false;
		for (const FString& K : T.Need) if (!S.Contains(K)) return false;
		return !T.NotBefore || S47Timeline::Night(Day) >= S47Timeline::Night(*T.NotBefore);
	}
}

FS47Caliche::FS47Caliche(US47Main* InM, FS47Town* InTown) : M(InM), Town(InTown)
{
	// main.ts: one npcDialogue for the night (ui.npcDialogue: N. Vega's talk in room 6 shares it)
	NpcDialogue = M->UI && M->UI->NpcDialogue.IsValid() ? M->UI->NpcDialogue : MakeShared<FS47NpcDialogue>(M->Audio);
	if (M->UI && !M->UI->NpcDialogue.IsValid()) M->UI->NpcDialogue = NpcDialogue;
	EarlDialogue = MakeShared<FS47EarlDialogue>(M->Audio);
	NpcDialogue->SetEnabled(M->Flow ? M->Flow->bCharacters : true);
	EarlDialogue->SetEnabled(M->Flow ? M->Flow->bArtBell : true);
	BindTown();
}
bool FS47Caliche::Here() const { return M->World && M->World->Area == TEXT("diner"); }
double FS47Caliche::Day() const { return FMath::Fmod(FMath::Fmod(M->Story->Clock(), 86400.) + 86400., 86400.); }
void FS47Caliche::Save() const { if (M->Mode == US47Main::EMode::Play) M->RequestAutosave(false); }
void FS47Caliche::Once(const FString& Id, const FString& Text)
{
	if (M->TownSeen.Contains(Id)) return;
	M->TownSeen.Add(Id); M->Story->Note(M->Story->ClockText(false) + TEXT(". ") + Text); Save();
}
void FS47Caliche::BindTown()
{
	// bindTown.add: a missing exported proxy is not an invented object.
	auto Add = [this](const TCHAR* Id, TFunction<FString()> Label, TFunction<void()> Use, TFunction<FString()> Examine = {}, double Range = 2.6)
	{
		const FS47Obj Object = Town->Proxy(Id); if (!Object) return;
		FS47Interactable I; I.Id = FString(TEXT("town:")) + Id; I.Object = Object; I.Range = Range;
		I.Label = [this, Label] { return Here() ? Label() : FString(); }; I.Use = MoveTemp(Use);
		if (Examine) I.Examine = [this, Examine] { return Here() ? Examine() : FString(); };
		I.ExamineRange = FMath::Max(8., Range + 2); M->Inter->Add(I);
	};
	auto Say = [this](const FString& T, double S = 3.6) -> TFunction<void()> { return [this, T, S] { M->UI->Toast(T, S); }; };
	auto Flyer = [this] { Once(TEXT("flyer"), TEXT("Caliche, the gas station door: a flyer, SEEN LIGHTS? CALL THE NIGHT LINE, 1420 AM, midnight to four.")); };
	// Unreal only (Tom, 23.10): the station is walked into now, its door (S47Props) opens; where the
	// model is not there the door is locked as in the web version
	Add(TEXT("gasDoor"), [] { return AS47Props::IsDoorOpen(TEXT("gasDoor")) ? TEXT("Close the door") : TEXT("Gas station door"); }, [this, Flyer] { if (AS47Props::UseDoor(TEXT("gasDoor"))) { Flyer(); return; } M->UI->Toast(TEXT("Locked, with a night window and a bell. The attendant looks up, sees you are not a car, and goes back to his magazine. Taped inside the glass: SEEN LIGHTS? CALL THE NIGHT LINE."), 5.4); Flyer(); }, [Flyer] { Flyer(); return TEXT("SEEN LIGHTS? CALL THE NIGHT LINE. 1420 AM, MIDNIGHT TO FOUR. Typed, copied, and copied again."); });
	Add(TEXT("gasWindow"), [] { return TEXT("Night window"); }, [this] { Talk(TEXT("attendant")); }, [] { return TEXT("Oil cans in rows, a calendar still on April, the register. Behind the counter a man in a pressed shirt turns a page. He has not looked at the clock once."); });
	Add(TEXT("pumps"), [] { return TEXT("Pumps"); }, Say(TEXT("Regular, 89.9. It has come down all spring. The nozzles are padlocked for the night."), 3.8), [] { return TEXT("Two pumps on a concrete island. The counters stand at zero."); });
	Add(TEXT("payphone"), [this] { return Town->bRinging ? TEXT("Answer the payphone") : TEXT("Payphone"); }, [this] { if (Town->bRinging) { if (Town->OnAnswer) Town->OnAnswer(); return; } M->UI->Toast(M->TownSeen.Contains(TEXT("nickel")) ? TEXT("A dial tone. The coin return is empty now.") : TEXT("A dial tone, and under it something like wind. Nobody to call at this hour."), 3.6); }, [] { return TEXT("A payphone on a post, a phone book on a chain with half its pages gone."); });
	Add(TEXT("gasSign"), [] { return TEXT("Sign"); }, Say(TEXT("GAS. DIESEL. No brand, just the words."), 2.8), {}, 3);
	Add(TEXT("paper"), [this] { return Town->bPaperIn ? FString(TEXT("Newspapers")) : FString(); }, [this] { if (!Town->bPaperIn) return; M->UI->Toast(TEXT("The Pecos Valley Sentinel, Monday, April 14, 1986, still in its twine. NICKLAUS, 46, WINS MASTERS. Dropped from a car that did not stop."), 5); Once(TEXT("paper"), TEXT("Caliche: the Pecos Valley Sentinel dropped at the gas station before five. Monday, April 14, 1986.")); }, [this] { return Town->bPaperIn ? FString(TEXT("NICKLAUS, 46, WINS MASTERS. Under the fold, smaller: County to grade the old Roswell road this summer.")) : FString(); });
	auto Map = [this] { Once(TEXT("pins"), TEXT("Caliche, the sheriff's substation: a county map on the wall, stuck with pins. Most are scattered. Two rows run straight, one east along the old road, one west into the empty part of the map.")); };
	Add(TEXT("sheriffWindow"), [] { return TEXT("Window"); }, [this, Map] { M->UI->Toast(Town->bDeputyIn ? TEXT("A county map on the wall, stuck with pins: red, blue, yellow. Most are scattered. Two rows are not. The deputy at the desk looks up at you, and back at the map.") : TEXT("The office is dark. Through the bars you can still make out the map and its pins, and two rows of them running straight."), 5.2); Map(); }, [Map] { Map(); return TEXT("One row of pins runs out east along the old road. The other runs west, off the highway, into country with nothing drawn on it."); });
	Add(TEXT("sheriffDoor"), [this] { return Town->bDeputyIn ? TEXT("Knock") : TEXT("Door"); }, [this] { if (Town->bDeputyIn) Talk(TEXT("deputy")); else M->UI->Toast(TEXT("Locked. A card in the frame, in ballpoint: BACK SOON."), 3.6); }, [this] { return Town->bDeputyIn ? FString(TEXT("COUNTY SHERIFF, SUBSTATION. A light under the door, and a radio turned low.")) : FString(); });
	Add(TEXT("patrolCar"), [this] { return Town->bDeputyIn || Town->bCarOut ? FString(TEXT("Patrol car")) : FString(); }, [this] { M->UI->Toast(Town->bCarOut ? TEXT("The engine is running. The driver's door hangs open with the keys in it, his hat on the seat, the radio on the dash turned all the way down. Nobody.") : TEXT("The deputy's car. A clipboard on the passenger seat, a thermos, a county map folded small."), Town->bCarOut ? 5.6 : 3.8); }, [this] { return Town->bCarOut ? FString(TEXT("No footprints going off the shoulder that you can see. The exhaust hangs in the cold, straight up.")) : Town->bDeputyIn ? FString(TEXT("The hood is still warm. Pins in a cigar box on the dash.")) : FString(); });
	Add(TEXT("laundromat"), [] { return TEXT("Laundromat"); }, Say(TEXT("Six washers, one tube light, nobody. One machine is running.")), [] { return TEXT("The one machine turns, stops, turns. There is no basket on the bench and no one waiting."); });
	Add(TEXT("bench"), [this] { return Town->bWomanIn ? FString() : FString(TEXT("Bench")); }, Say(TEXT("A bench under the window. The paint is worn through in one place, where somebody sits often.")));
	Add(TEXT("cinema"), [] { return TEXT("The Plaza"); }, Say(TEXT("Closed. The doors are chained. The posters behind the glass have faded to the same blue."), 3.8), [] { return TEXT("A box office with its blind down, and a card: CLOSED."); });
	Add(TEXT("marquee"), [] { return FString(); }, [] {}, [] { return TEXT("CLOSED. THANK YOU CALICHE. FOR 31 YEARS. A letter has fallen out of the last line."); }, 4);
	Add(TEXT("feed"), [] { return TEXT("Feed store"); }, Say(TEXT("FEED & SEED. Sacks on a pallet on the dock, a bulb over the door with moths round it.")), {}, 3);
	auto OnAir = [this] { return S47Timeline::LocalOnAir(Day()); };
	auto EarlHere = [this] { return S47Timeline::EarlOnDuty(Day()); };
	Add(TEXT("studio"), [] { return TEXT("Studio window"); }, [this, OnAir] { if (Day() >= 18 * 3600) { M->UI->Toast(TEXT("The evening presenter at the microphone. A record plays. The red ON AIR light is on; Earl starts the Night Line at midnight."), 4.6); return; } M->UI->Toast(OnAir() ? TEXT("The Night Line, through the glass: the host at the microphone with his headphones on, the red ON AIR light. He does not look up.") : TEXT("The studio is empty and the red light is off. Through the wall, the carrier: a hum with nothing in it."), 4.6); }, [OnAir] { return OnAir() ? TEXT("Reels turning on the deck behind him. A coffee can of pencils, a phone with its line light blinking.") : TEXT("His chair pushed back. The phone's line light still blinking."); });
	Add(TEXT("amDoor"), [EarlHere] { return EarlHere() ? TEXT("Buzzer") : TEXT("Door"); }, [this, EarlHere] { if (EarlHere()) Talk(TEXT("host")); else if (Day() >= 18 * 3600) M->UI->Toast(TEXT("The presenter waves from behind the glass. A card: NIGHT LINE / EARL / MIDNIGHT TO FOUR."), 4.2); else M->UI->Toast(TEXT("1420 AM. A buzzer and a card: STUDIO IN USE, PLEASE WAIT. Nobody comes. Through the door, the carrier."), 4.2); });
	Add(TEXT("souvenir"), [] { return TEXT("Shop window"); }, [this] { M->UI->Toast(M->TownSeen.Contains(TEXT("ev:cutout")) && !Town->bCutoutOut ? TEXT("The cardboard alien is back in the window. There is dew on it, and grit from the road along its feet.") : TEXT("A cardboard alien, life size if they come in that size. A rubber mask. Bags of crumpled foil, a dollar each: GENUINE DEBRIS."), 4.6); }, [] { return TEXT("The foil has been crumpled by hand. There is a price sticker on the alien too."); });
	Add(TEXT("souvenirSign"), [] { return TEXT("Sign"); }, Say(TEXT("SAUCER CRASH 1947. TOURS. ASK INSIDE. Painted by hand, and painted over at least once."), 3.8), {}, 3.4);
	Add(TEXT("souvenirDoor"), [] { return TEXT("Door"); }, Say(TEXT("CLOSED. The hours are on a card faded white."), 3));
	Add(TEXT("letters"), [] { return TEXT("Letter board"); }, Say(TEXT("THE HEAVENS DECLARE THE GLORY OF GOD. PS 19:1. A bulb over it, burning all night."), 3.8), {}, 3);
	Add(TEXT("churchDoor"), [] { return TEXT("Church door"); }, Say(TEXT("Locked. Sunday 10 AM, it says. All welcome."), 3));
	auto Tv1On = [this] { return Day() >= 18 * 3600 || Day() < 3 * 3600 + 40 * 60; };
	Add(TEXT("tv1"), [] { return TEXT("Window"); }, [this, Tv1On] { M->UI->Toast(Tv1On() ? TEXT("A television on in a dark front room, and someone asleep in the chair in front of it.") : TEXT("The television is off now. The chair is empty."), 3.8); }, [Tv1On] { return Tv1On() ? FString(TEXT("The late movie, black and white, with the sound down.")) : FString(); }, 6.5);
	Add(TEXT("tv2"), [] { return TEXT("Window"); }, Say(TEXT("A television on with nobody watching it. Snow, then the test pattern, then snow."), 3.8), {}, 6.5);
	Add(TEXT("kitchen"), [] { return TEXT("Window"); }, Say(TEXT("A kitchen light behind a curtain, and a shadow that does not move."), 3.4), {}, 4);
	Add(TEXT("woman"), [this] { return Town->bWomanIn ? FString(TEXT("Woman on the bench")) : FString(); }, [this] { if (Town->bWomanIn) Talk(TEXT("radio_woman")); }, [this] { return Town->bWomanIn ? FString(TEXT("A woman in a winter coat with a transistor radio in her lap. No laundry with her.")) : FString(); });
	Add(TEXT("smoker"), [this] { return Town->bSmokerIn ? FString(TEXT("Man on the stoop")) : FString(); }, [this] { if (Town->bSmokerIn) Talk(TEXT("souvenir_man")); }, [this] { return Town->bSmokerIn ? FString(TEXT("A man in a hat on his front step, smoking, watching the street.")) : FString(); }, 5.6);
}

bool FS47Caliche::Available(const FString& W, const FString& Id) const
{
	const FPerson* P = Person(W); if (!P) return false;
	const FTopic* T = P->Topics.FindByPredicate([&](const FTopic& X) { return X.Id == Id; });
	return T && AvailableTopic(M->TownSeen, W, *T, Day(), Reel.Ready && Reel.Ready());
}
void FS47Caliche::Talk(const FString& W)
{
	const FPerson* P = Person(W); if (!P) return;
	StopSpeech(); ShownLines.Reset();
	const bool bFirst = !M->TownSeen.Contains(TEXT("met:") + W); Who = W; M->TownSeen.Add(TEXT("met:") + W);
	Lines = W == TEXT("deputy") && Day() >= DEPUTY_IN && Day() < 12 * 3600 ? P->Back : bFirst ? P->First : P->Again;
	if (W == TEXT("attendant")) Attendant(bFirst);
	if (W == TEXT("deputy") && Day() >= DEPUTY_IN && Day() < 12 * 3600) Once(TEXT("deputyBack"), DEPUTY_BACK);
	Save(); Body = SNew(SVerticalBox);
	// ui/Panels.ts talk: Slate holds the panel and redraws its body in place.
	M->UI->Open(S47Kit::Shell(M->UI, P->Title, Body.ToSharedRef()), [this] { Close(); }); Refresh();
}
void FS47Caliche::Refresh()
{
	const FPerson* P = Person(Who); if (!P || !Body.IsValid()) return;
	if (M->Flow)
	{
		if (NpcDialogue->Enabled() != M->Flow->bCharacters) NpcDialogue->SetEnabled(M->Flow->bCharacters);
		if (EarlDialogue->Enabled() != M->Flow->bArtBell) EarlDialogue->SetEnabled(M->Flow->bArtBell);
	}
	const bool bFresh = Lines != ShownLines;
	// (a new exchange: ui.descriptionReader?.stop(); ui.readAloud?.stop(), and the voices)
	if (bFresh)
	{
		ShownLines = Lines; StopSpeech();
		if (const TSharedPtr<FS47ReadAloud> D = M->UI ? M->UI->DescriptionReader() : nullptr) D->Stop();
		if (M->UI && M->UI->ReadAloud) M->UI->ReadAloud->Stop();
	}
	const bool bEarl = Who == TEXT("host") && EarlDialogue->Enabled() && Lines.ContainsByPredicate([](const FS47TalkLine& L) { return L.Who == TEXT("EARL"); });
	const FString Voice = Who == TEXT("attendant") ? TEXT("stanton") : Who == TEXT("deputy") ? TEXT("stack") : Who == TEXT("souvenir_man") ? TEXT("lynch") : TEXT("");
	const bool bCharacter = !Voice.IsEmpty() && NpcDialogue->Enabled() && !FS47NpcDialogue::CharacterLines(Voice, Lines).IsEmpty();
	const TArray<FS47TalkLine> Speech = Lines;
	Body->ClearChildren();
	Body->AddSlot().AutoHeight()[S47Kit::Sub(bReeling ? TEXT("The reel turning on the second deck. The monitor up.") : P->Status)];
	if (UTexture2D* Face = S47Image::Art(P->Card.Portrait)) Body->AddSlot().AutoHeight()[S47Kit::Image(Face, 150)];
	// Slate has no CSS animation-delay; keep the same line and speaker order. (The lines are .talk-line,
	// read by the Read aloud box below, not as the panel's descriptions.)
	for (const FS47TalkLine& L : Lines) Body->AddSlot().AutoHeight().Padding(0, 3)[S47Kit::Unread(L.Who.IsEmpty() ? S47Kit::Small(L.Text) : S47Kit::Text(L.Who + TEXT("\n") + L.Text))];
	if (!bReeling) for (const FTopic& T : P->Topics) if (Available(Who, T.Id)) Body->AddSlot().AutoHeight().Padding(0, 4)[S47Kit::Button(T.Label, [this, Id = T.Id] { Choice(Id); })];
	Body->AddSlot().AutoHeight().Padding(0, 4)[S47Kit::Button(bReeling ? TEXT("Walk away") : P->Bye, [this] { M->UI->Close(); })];
	// ui.readingControls(reading, texts): Kinski reads the lines no voice has (Earl's while his voice is on,
	// the character's while the character voices are)
	if (bFresh || !Reading.IsValid())
	{
		const TArray<FS47TalkLine> Voiced = !Voice.IsEmpty() && NpcDialogue->Enabled() ? FS47NpcDialogue::CharacterLines(Voice, Lines) : TArray<FS47TalkLine>();
		TArray<FString> Texts;
		for (const FS47TalkLine& L : Lines) if ((!bEarl || L.Who != TEXT("EARL")) && !Voiced.Contains(L)) Texts.Add(L.Text);
		Reading = Texts.Num() && M->UI ? M->UI->ReadingControls(Texts) : SNullWidget::NullWidget;
	}
	Body->AddSlot().AutoHeight()[Reading.ToSharedRef()];
	// UI.characterReadingControls / earlReadingControls: whole authored replies, replay and stop.
	bool bAvailable = false;
	if (bEarl || bCharacter)
	{
		FS47RecordedDialogue* Reader = bEarl ? static_cast<FS47RecordedDialogue*>(EarlDialogue.Get()) : static_cast<FS47RecordedDialogue*>(NpcDialogue.Get());
		bAvailable = bEarl ? EarlDialogue->Available(Speech) : NpcDialogue->Available({ Voice, Speech });
		const bool bActive = Reader->Loading() || Reader->Playing();
		const FString Label = bEarl ? bActive ? TEXT("Stop Earl") : TEXT("Replay Earl") : bActive ? TEXT("Stop voice") : TEXT("Replay voice");
		const FString Loading = bEarl ? TEXT("Loading Earl…") : TEXT("Loading voice…");
		const FString Parody = bEarl ? TEXT("Synthetic Art Bell parody") : TEXT("Synthetic ") + FS47NpcDialogue::CharacterLabels().FindRef(Voice) + TEXT(" parody");
		const FString Status = Reader->Loading() ? Loading : Reader->State() == TEXT("error") ? FString(TEXT("Recording could not be played. Try again."))
			: !bAvailable ? FString(TEXT("No recording for this reply.")) : Parody;
		Body->AddSlot().AutoHeight().Padding(0, 4)[S47Kit::Row({ S47Kit::Button(Label, [this, bEarl, Voice, Speech]
		{
			FS47RecordedDialogue* R = bEarl ? static_cast<FS47RecordedDialogue*>(EarlDialogue.Get()) : static_cast<FS47RecordedDialogue*>(NpcDialogue.Get());
			if (R->Loading() || R->Playing()) R->Stop(); else StartVoice(bEarl, Voice, Speech);
		}, false, bAvailable), S47Kit::Small(Status) })];
	}
	// P.talk starts new voiced replies once; refreshing the same lines never repeats them.
	if (bFresh && bAvailable) StartVoice(bEarl, Voice, Speech);
}
void FS47Caliche::StartVoice(bool bEarl, const FString& Voice, const TArray<FS47TalkLine>& Speech)
{
	if (bEarl) { NpcDialogue->Stop(); if (OnEarlStart) OnEarlStart(); }
	else { EarlDialogue->Stop(); if (OnCharacterStart) OnCharacterStart(); }
	if (M->Audio) M->Audio->StopPhoneCalls();
	auto OnState = [this](const FString&) { if (Body.IsValid()) Refresh(); };
	if (bEarl) EarlDialogue->Play(Speech, OnState); else NpcDialogue->Play({ Voice, Speech }, OnState);
}
void FS47Caliche::StopSpeech() { EarlDialogue->Stop(); NpcDialogue->Stop(); }

void FS47Caliche::Choice(const FString& Id)
{
	if (Id == TEXT("bye")) { M->UI->Close(); return; }
	const FPerson* P = Person(Who); if (!P || bReeling || !Available(Who, Id)) return;
	const FTopic* T = P->Topics.FindByPredicate([&](const FTopic& X) { return X.Id == Id; }); if (!T) return;
	const FString W = Who;
	StopSpeech();
	if (!T->bReel) M->TownSeen.Add(TEXT("ask:") + W + TEXT(":") + Id);
	Lines = T->Lines;
	if (!T->Note.IsEmpty()) Once(TEXT("note:") + W + TEXT(":") + Id, T->Note);
	if (T->Paper) for (const FS47Doc& Doc : TownWitnessDocs(M->TownSeen)) if (Doc.Id == T->Paper->Id) { M->Story->AddDoc(Doc); M->UI->Toast(T->Filed.IsEmpty() ? TEXT("Witness note filed in Papers.") : T->Filed, 3); break; }
	Save();
	if (T->bReel && Reel.Play)
	{
		bReeling = true;
		Reel.Play([this, W](const TArray<FS47TalkLine>& L) { if (Who != W) return; Lines = L; Refresh(); }, [this, W, Id](bool bComplete)
		{
			bReeling = false;
			if (bComplete) { M->TownSeen.Add(TEXT("ask:") + W + TEXT(":") + Id); M->UI->Toast(TEXT("Witness note filed in Papers."), 3); }
			Save(); if (Who == W) Refresh();
		});
	}
	Refresh();
}
void FS47Caliche::Close() { Who.Empty(); StopSpeech(); if (bReeling && Reel.Stop) Reel.Stop(); Body.Reset(); ShownLines.Reset(); Reading.Reset(); }
void FS47Caliche::Attendant(bool bFirst)
{
	FString At; for (const FString& K : M->TownSeen.Array()) if (K.StartsWith(TEXT("attendant@"))) { At = K; break; }
	if (bFirst || At.IsEmpty()) { if (At.IsEmpty()) M->TownSeen.Add(FString::Printf(TEXT("attendant@%.0f"), FMath::FloorToDouble(M->Story->Clock() + 0.5))); return; }
	const double T0 = FCString::Atod(*At.Mid(10)); if (M->Story->Clock() - T0 < 2 * 3600) return;
	Lines = { { TEXT(""), TEXT("He slides the window open a hand's width, and looks at you as if you had just pulled in.") }, { TEXT("THE ATTENDANT"), TEXT("Cold one tonight. Radio says it'll") }, { TEXT(""), TEXT("He coughs into his fist.") }, { TEXT("THE ATTENDANT"), TEXT("warm up by Wednesday.") } };
	M->TownSeen.Add(FString::Printf(TEXT("mark:repeat@%.0f"), FMath::FloorToDouble(M->Story->Clock() + 0.5)));
	Once(TEXT("repeat"), FString::Printf(TEXT("Station attendant, %s and %s. The same sentence, the same cough."), *S47Text::ClockText(T0, false), *M->Story->ClockText(false)));
}
TArray<FS47PersonCard> FS47Caliche::TownPeople(const FS47TownSeen& Seen)
{
	TArray<FS47PersonCard> Out; for (const FPerson& P : People()) if (Seen.Contains(TEXT("met:") + P.Id)) Out.Add(P.Card); return Out;
}
TArray<FS47Doc> FS47Caliche::TownWitnessDocs(const FS47TownSeen& Seen)
{
	TArray<FS47Doc> Out; for (const FPerson& P : People()) for (const FTopic& T : P.Topics) if (T.Paper && Asked(Seen, P.Id, T.Id)) Out.Add(*T.Paper); return Out;
}
bool FS47Caliche::SelfTest(TArray<FString>& Out)
{
	bool bOk = true; auto Check = [&](bool b, const TCHAR* S) { bOk &= b; Out.Add(FString(b ? TEXT("ok caliche: ") : TEXT("FAIL caliche: ")) + S); };
	FS47TownSeen S; S.Add(TEXT("met:host")); S.Add(TEXT("met:attendant"));
	const auto P = TownPeople(S); Check(P.Num() == 2 && P[0].Name == TEXT("The attendant") && P[1].Name == TEXT("Earl"), TEXT("People follows the TypeScript order"));
	const FTopic& Phone = Person(TEXT("attendant"))->Topics.Last(); Check(!AvailableTopic(S, TEXT("attendant"), Phone, 0, false), TEXT("a follow-up needs its observations"));
	S.Add(TEXT("ask:attendant:old")); S.Add(TEXT("ev:phone1")); S.Add(TEXT("nickel")); Check(AvailableTopic(S, TEXT("attendant"), Phone, 0, false), TEXT("the bright nickel opens the attendant's follow-up"));
	S.Add(TEXT("ask:attendant:phone")); auto D = TownWitnessDocs(S); Check(D.Num() == 1 && D[0].Id == TEXT("witness-phone") && D[0].Page.Contains(TEXT("That phone takes dimes.")), TEXT("witness copies refile from the saved keys"));
	S.Add(TEXT("ask:deputy:green")); S.Add(TEXT("ev:car")); const FTopic& Car = Person(TEXT("deputy"))->Topics.Last(); Check(!AvailableTopic(S, TEXT("deputy"), Car, DEPUTY_IN - 1, false) && AvailableTopic(S, TEXT("deputy"), Car, DEPUTY_IN, false), TEXT("the deputy returns at 04:30"));
	Check(!AvailableTopic(S, TEXT("deputy"), Car, 18 * 3600, false), TEXT("evening deputy cannot recount his future patrol"));
	bOk &= FS47RecordedDialogue::SelfTest(Out);
	bOk &= FS47NpcDialogue::SelfTest(Out);
	Check(S47Timeline::ShiftStart < 0 && S47Timeline::Night(18 * 3600) == S47Timeline::ShiftStart && S47Timeline::Night(23 * 3600 + 59 * 60) < 0, TEXT("evening timeline: signed clock crosses midnight forwards"));
 Check(S47Timeline::Residual(true) == 827 && S47Timeline::Residual(false) == 8027, TEXT("evening timeline: new residual 00:13:47, legacy remains 02:13:47"));
 Check(S47Timeline::Chapter3(true) < S47Timeline::Chapter4(true) && S47Timeline::Chapter4(true) < S47Timeline::Chapter5(true) && S47Timeline::Chapter5(true) < S47Timeline::Departure && S47Timeline::Finale - S47Timeline::Departure == 9 * 60, TEXT("evening timeline: chapter order and fixed finale"));
 Check(S47Timeline::LocalOnAir(-6 * 3600) && !S47Timeline::EarlOnDuty(-6 * 3600) && S47Timeline::EarlOnDuty(0) && !S47Timeline::LocalOnAir(4 * 3600), TEXT("evening timeline: local presenter until midnight; Earl leaves at four"));
 Check(S47Timeline::DeputyOnDuty(-6 * 3600) && !S47Timeline::DeputyOnDuty(3 * 3600) && S47Timeline::DeputyOnDuty(DEPUTY_IN), TEXT("evening timeline: deputy working in evening, patrol 02:45 to 04:30"));
 Check(S47Timeline::ApproachClock(S47Timeline::Finale - 80, S47Timeline::Finale - 18, false) == S47Timeline::Finale - 47 && S47Timeline::ApproachClock(S47Timeline::Finale - 47, S47Timeline::Finale - 90, false) == S47Timeline::Finale - 47, TEXT("evening timeline: slow approach reserves exact CB time and reversing never rewinds"));
 Check(S47Timeline::ResidualText(TEXT("02:13:47 / 02:14"), true) == TEXT("00:13:47 / 00:14") && S47Timeline::ResidualText(TEXT("02:17:00 / 02:17:47"), true) == TEXT("02:17:00 / 02:17:47"), TEXT("evening timeline: no change to 1947 timing record"));
 bOk &= AS47Story::TimelineSelfTest(Out);
 bOk &= FS47EarlDialogue::SelfTest(Out);
	return bOk;
}

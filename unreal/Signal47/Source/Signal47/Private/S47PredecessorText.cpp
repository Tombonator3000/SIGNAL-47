#include "S47PredecessorText.h"

namespace S47PredecessorText
{
const TArray<FDocDef>& Docs()
{
 static const TArray<FDocDef> Table={
  // ---- the locker
  {TEXT("note"),TEXT("A note to self, on the back of a fuel slip"),TEXT("hand"),TEXT("REYES"),
   TEXT("~ MONDAY\n~ call Dale re truck (brakes. he'll yell. call anyway)\n~ coffee filters. the real ones\n~ pay Danny back $20\n~ do NOT sign the fuel sheet till they fix the total\n~ type up RFI list\n~ ask Ward re a Sunday off in May"),
   TEXT("Reyes's list for a Monday. Dale, coffee filters, twenty dollars owed, a list to type up. The Monday was April 14."),nullptr},
  {TEXT("list"),TEXT("Dale's interference list, on a clipboard"),TEXT("typed"),TEXT("SARO / FIELD"),
   TEXT("FROM: DALE, FIELD MAINT.\nTO: REYES\n\nRFI SOURCE LIST. YES, ANOTHER ONE.\n\nSomebody's pump is in my receiver and I will find it if I have to dig up New Mexico with my hands. Every motor, every fence charger, every bad ground inside ten miles. You go and you look. You tag it, you date it, you initial it. Daylight only and no overtime, Ward will have my head and I am using it.\n\nDo not be clever. Clever I can get anywhere.\n\n~ start with the windmill on the Baca lease\n~ he underlined \"look\" three times. I'll work it out as I go - R."),
   TEXT("Dale had Reyes driving the land for interference sources, tagging each stop. Reyes's list starts with the windmill on the Baca lease."),TEXT("windmill")},
  {TEXT("home"),TEXT("A letter, stamped, not mailed"),TEXT("hand"),TEXT("NOT SENT"),
   TEXT("April 10\n\nDear Mom,\n\nThe job's fine. It's mostly paper. You sit up all night with the machines and write down what they say, and in the morning somebody you never meet tells you to write it down a different way.\n\nIt's very quiet. You'd like the sky. You'd hate the coffee.\n\nThe money is what they said it would be. I can send something at the end of the month and I will, so stop telling Aunt Lupe I won't.\n\nI drive around some when I get off. Nobody out here minds.\n\nI'll call Sunday before I go on.\n\nLove,\nme"),
   TEXT("A letter home from Reyes, dated April 10, stamped and never mailed. It says the job is fine. It says Reyes will call on Sunday."),nullptr},
  {TEXT("mail"),TEXT("A letter that came after, opened by you"),TEXT("hand"),TEXT("POSTMARK APR 18"),
   TEXT("April 18\n\nIt's me.\n\nYou didn't call Sunday. Your father says you're busy and to leave you alone.\n\nA man from the county telephoned here Thursday and asked when we last heard from you. I told him Easter, and then I remembered the call in March, so I told him that too. He thanked me. He didn't say what for.\n\nCall when you get this. It doesn't matter how late.\n\nI put a twenty in. Don't argue.\n\nMom"),
   TEXT("A letter from Reyes's mother, postmarked April 18, never opened until you did. The county had telephoned. The twenty is still folded in it."),nullptr},
  {TEXT("pay"),TEXT("Final pay voucher"),TEXT("typed"),TEXT("NOT CALLED FOR"),
   TEXT("SOCORRO COUNTY / DISTRICT PAYROLL\n\nPAY TO:          REYES\nPOSITION:        NIGHT OPERATOR, SARO\nLAST DAY WORKED: 04/13/86\nNET:             $312.40\n\nFINAL PAY. HOLD AT SITE FOR PICK-UP.\n\nNOT CALLED FOR."),
   TEXT("Reyes's final pay, held at the site and not called for. Somebody in payroll has already typed a last day worked: April 13."),nullptr},
  // ---- the binder
  {TEXT("gap"),TEXT("April's shift binder"),TEXT("typed"),TEXT("SARO / RECORDS"),
   TEXT("SHIFT BINDER / APRIL 1986\n\nApril 1 to April 13, 18:00: all present, all initialled R.\n\nAfter that the rings hold nothing. The receiver log for the night of the 13th is gone. So is the strip chart, the shift journal and the feeder test journal.\n\n~ Originals not on file. County has asked. Do not reconstruct. - W."),
   TEXT("The originals of the night of April 13 are gone from the binder. Receiver log, strip chart, shift journal, feeder journal. Ward's note says not to reconstruct them."),nullptr},
  // ---- the survey tags, out on the land, in the order Reyes went
  {TEXT("tag_windmill"),TEXT("Survey tag at the windmill"),TEXT("hand"),TEXT("SARO RFI / 1"),
   TEXT("SARO RFI SURVEY   POINT 1\nDATE 3-24-86   OPR REYES\nSOURCE: no motor. Brake on. Nothing.\n\n~ This is what I'm doing with my mornings.\n~ Somebody wired the vane round. Not a rancher's knot. Not Dale's either. I asked, and got ten minutes on whether I know what a knot is.\n~ Next: the stock tank on the old grade."),
   TEXT("Reyes's first tag, March 24, at the windmill. The wired vane was not Dale's. Next, the tank on the old grade."),TEXT("tank")},
  {TEXT("tag_tank"),TEXT("Survey tag at the water tank"),TEXT("hand"),TEXT("SARO RFI / 2"),
   TEXT("SARO RFI SURVEY   POINT 2\nDATE 3-27-86   OPR REYES\nSOURCE: none. No pump. No charger.\n\n~ Ate my sandwich here to see if anything would come and drink. Nothing did.\n~ Not a complaint. For the file.\n~ Next: the line camp east of the highway. Bring water this time."),
   TEXT("Reyes at the tank, March 27. Nothing came to drink. Next, the line camp east of the highway."),TEXT("shack")},
  {TEXT("tag_shack"),TEXT("Survey tag at the line shack"),TEXT("hand"),TEXT("SARO RFI / 3"),
   TEXT("SARO RFI SURVEY   POINT 3\nDATE 4-1-86   OPR REYES\nSOURCE: a stove. No line power out here.\n\n~ April Fool. Slept an hour on that cot frame. Worse than it sounds. Do not tell Ward.\n~ Somebody's been keeping a count on the door. I did not add to it.\n~ Somebody at the diner says you can see the whole layout from the rise. Next: the cairn."),
   TEXT("Reyes at the line shack, April 1. Slept there an hour and left the count on the door alone. Next, the cairn on the rise."),TEXT("cairn")},
  {TEXT("tag_cairn"),TEXT("Survey tag at the cairn"),TEXT("hand"),TEXT("SARO RFI / 4"),
   TEXT("SARO RFI SURVEY   POINT 4\nDATE 4-3-86   OPR REYES\nSOURCE: none. Clear line to the array.\n\n~ Good spot. The dishes and the Alderete place both. On a still night you'd see anyone on either road.\n~ Who drives out here to stack rocks? (I drove out here.)\n~ Alderete's boy says one of theirs went through a fence way down south. Going to look."),
   TEXT("Reyes at the cairn, April 3. A good view of the station and the farm. Next, an Alderete cow gone through a fence, far to the south."),TEXT("cow")},
  {TEXT("tag_cow"),TEXT("Survey tag by the dead cow"),TEXT("hand"),TEXT("SARO RFI / 5"),
   TEXT("SARO RFI SURVEY   POINT 5\nDATE 4-7-86   OPR REYES\nSOURCE: none. Ear tag.\n\n~ Dead a few days, by the smell. No flies.\n~ Told the deputy, it being an Alderete brand. Deputy wrote down what I saw and not one word of what I said I thought. Fair enough.\n~ Salt lick further along the same fence is next."),
   TEXT("Reyes's tag at the cow is dated April 7, and says she had been dead a few days. Reyes told the deputy. Next, the salt lick along the fence."),TEXT("trough")},
  {TEXT("tag_trough"),TEXT("Survey tag at the salt lick"),TEXT("hand"),TEXT("SARO RFI / 6"),
   TEXT("SARO RFI SURVEY   POINT 6\nDATE 4-9-86   OPR REYES\nSOURCE: none. Iron.\n\n~ Prints stop ten feet short. I paced it. Went back and paced it again so I could say I had.\n~ Two clocks at the station disagreed last night. Not related. I am not saying it's related.\n~ Earl had a caller about a balloon down east of the highway. Should be a nice dull one."),
   TEXT("Reyes at the salt lick, April 9. Paced the prints twice. Next, a balloon east of the highway."),TEXT("balloon")},
  {TEXT("tag_balloon"),TEXT("Survey tag at the balloon"),TEXT("hand"),TEXT("SARO RFI / 7"),
   TEXT("SARO RFI SURVEY   POINT 7\nDATE 4-10-86   OPR REYES\nSOURCE: a balloon.\n\n~ It's a balloon. I could kiss it.\n~ Box is empty. Label says mail it in, so I copied the label and mailed them a card. We'll see.\n~ Coming back: one stake off the track, fresh. Not on any job I can find."),
   TEXT("Reyes at the balloon, April 10. It was a balloon. Next, a stake off the track that was on nobody's job."),TEXT("stake")},
  {TEXT("tag_stake"),TEXT("Survey tag at the stake"),TEXT("hand"),TEXT("SARO RFI / 8"),
   TEXT("SARO RFI SURVEY   POINT 8\nDATE 4-11-86   OPR REYES\nSOURCE: none.\n\n~ Called the county road office. No survey on this track. Called the BLM. No survey. The woman at the BLM asked why I cared.\n~ Copied down what's on it. Did not think about it.\n~ Last one on my list is the mine on the west side. Saturday, in daylight."),
   TEXT("Reyes at the stake, April 11. Neither the county nor the BLM had a survey there. Last, the mine on the west side."),TEXT("mine")},
  {TEXT("tag_mine"),TEXT("Survey tag at the mine"),TEXT("hand"),TEXT("SARO RFI / 9"),
   TEXT("SARO RFI SURVEY   POINT 9\nDATE 4-12-86   OPR REYES\nSOURCE: none at the mouth. Did not go in.\n\n~ Cold air coming out steady as a fan. Boards stacked neat. I'm not going in there by myself for an RFI list.\n~ Told Dale. Dale called me several things and then said regional keeps a tech for the back-country jobs. White wagon, number 23. If this goes any further I'll ask.\n~ List's done. I'll type it up Monday."),
   TEXT("Reyes's last tag, Saturday April 12, at the mine. Reyes did not go in. It mentions a regional tech in a white wagon, number 23."),nullptr},
  // ---- among the sheets of a case
  {TEXT("case_interval"),TEXT("Note in Reyes's hand, among the interval sheets"),TEXT("hand"),TEXT("REYES"),
   TEXT("~ B1133 again. Extra ticks. Four, a gap, seven. Not on the pulsar's period.\n~ Filed unresolved, like it says to. Ward: \"Retain and compare.\"\n~ If it's local it's close, which is why Dale has me out driving. Started at the windmill."),
   TEXT("Reyes saw the extra ticks too, and filed them unresolved. Reyes looked for a local source, starting at the windmill."),TEXT("windmill")},
  {TEXT("case_records"),TEXT("Note in Reyes's hand, clipped to a timing sheet"),TEXT("hand"),TEXT("REYES"),
   TEXT("~ Station clock and reference disagree, two nights running. Service says relay.\n~ I wrote both numbers down. Do not pick one.\n~ Thought about it at the salt lick, of all places. Not related."),
   TEXT("Reyes had two clocks disagreeing as well, and wrote down both numbers. The note mentions the salt lick."),TEXT("trough")},
  {TEXT("case_visitors"),TEXT("Note in Reyes's hand, inside the roof log"),TEXT("hand"),TEXT("REYES"),
   TEXT("~ Steps on the roof about 3. Went up with the lamp. A loose plate, maybe. Maybe.\n~ Didn't put it on the sheet. Putting it here.\n~ You can see this roof from the cairn on a clear night. I checked that much."),
   TEXT("Reyes heard steps on the roof too, and kept it off the sheet. The note says the roof can be seen from the cairn."),TEXT("cairn")},
  // ---- wagon 23
  {TEXT("callout"),TEXT("Call-out sheet, from wagon 23's clipboard"),TEXT("typed"),TEXT("DISTRICT MAINT."),
   TEXT("DISTRICT MAINTENANCE / UNIT 23\nTECH: M. ORTIZ\n\nCALL-OUT: NIGHT OF 04/13/86\nFROM: DISTRICT DISPATCH\nAUTH: WARD\nTO: CALICHE, THEN SARO\n\n~ took the big thermos\n~ back by 6"),
   TEXT("Unit 23's call-out sheet. Mara Ortiz, sent on Ward's authority, the night of April 13. Her note says back by six."),nullptr},
 };
 return Table;
}
const FDocDef* Doc(const FString& Id){return Docs().FindByPredicate([&](const FDocDef& D){return Id==D.Id;});}
const TArray<FTagDef>& Tags()
{
 static const TArray<FTagDef> Table={
  {TEXT("windmill"),TEXT("tag_windmill"),6,4},{TEXT("tank"),TEXT("tag_tank"),6.5,3},{TEXT("shack"),TEXT("tag_shack"),5,5},{TEXT("cairn"),TEXT("tag_cairn"),4,3},
  {TEXT("cow"),TEXT("tag_cow"),-4,3.5},{TEXT("trough"),TEXT("tag_trough"),-1,3.5},{TEXT("balloon"),TEXT("tag_balloon"),-4,3},{TEXT("stake"),TEXT("tag_stake"),3,3},{TEXT("mine"),TEXT("tag_mine"),-3.5,-4.5},
 };
 return Table;
}
const TArray<FCaseNote>& CaseNotes()
{
 static const TArray<FCaseNote> Table={{TEXT("THE REPEATED INTERVAL"),TEXT("case_interval")},{TEXT("DISAGREEING RECORDS"),TEXT("case_records")},{TEXT("VISITORS AROUND THE BASE"),TEXT("case_visitors")}};
 return Table;
}
const TArray<FPerson>& People()
{
 static const TCHAR* const You=TEXT("YOU");
 static const TCHAR* const Att=TEXT("THE ATTENDANT");
 static const TCHAR* const Dep=TEXT("THE DEPUTY");
 static const TCHAR* const Earl=TEXT("EARL");
 static const TCHAR* const Wom=TEXT("THE WOMAN");
 static const TCHAR* const Man=TEXT("THE SHOP MAN");
 static const TCHAR* const Ward=TEXT("DR. WARD");
 static const TCHAR* const Nora=TEXT("THE VOICE");
 static const TCHAR* const Dale=TEXT("DALE");
 static const TArray<FPerson> Table={
  {TEXT("attendant"),TEXT("GAS STATION / THE ATTENDANT"),
   {{TEXT(""),TEXT("He slides the night window open a hand's width and looks at you over his book.")},{Att,TEXT("You're the new one up at the dishes.")},{TEXT(""),TEXT("It isn't a question.")}},
   {
    {TEXT("reyes"),TEXT("Ask if he knew Reyes"),nullptr,nullptr,TEXT("The attendant knew Reyes to sell gas to. Reyes paid cash and kept the receipt."),
     {{You,TEXT("Did you know Reyes?")},{Att,TEXT("Regular, black coffee, paid cash, kept the receipt. I knew Reyes to sell gas to.")},{Att,TEXT("Last fill-up was the Saturday. Said see you Monday. People say that.")}}},
    {TEXT("wagon"),TEXT("Ask about the white wagon"),TEXT("wagon"),nullptr,TEXT("The attendant says wagon 23 was by his pumps that night, and still there when he went home at six."),
     {{You,TEXT("There's a white district wagon out on the flat. Number 23.")},{Att,TEXT("It was out front of here that night. The deputy's asked me. The county's asked me. A woman from the district asked me.")},{Att,TEXT("I told them all the same. I was reading. I looked up and it was parked. I looked up again and there was nobody by it.")},{Att,TEXT("It was there at six when I went home. How it got where it is, you tell me. I write down what goes in the till.")}}},
    {TEXT("closing"),TEXT("Ask what people are saying about SARO"),nullptr,nullptr,TEXT("Caliche has heard that the county may close SARO."),
     {{You,TEXT("What are people saying about the station?")},{Att,TEXT("That the county's shutting you down. That true?")},{You,TEXT("End of May, maybe.")},{Att,TEXT("Then I won't order extra.")}}},
   }},
  {TEXT("deputy"),TEXT("SHERIFF SUBSTATION / THE DEPUTY"),
   {{TEXT(""),TEXT("He opens the door and looks at your jacket before he looks at you.")},{Dep,TEXT("So you're what they sent. Help you?")}},
   {
    {TEXT("reyes"),TEXT("Ask about Reyes"),nullptr,nullptr,TEXT("The deputy keeps a file on Reyes: night operator at SARO, not seen since the morning of April 14. He writes down what is seen, not what is thought."),
     {{You,TEXT("I took over from Reyes. What happened?")},{Dep,TEXT("I've got two files on this desk. One's Reyes. Night operator at SARO, last logged the night of April 13, not seen since the morning of the 14th.")},{Dep,TEXT("I don't put in a file what somebody thinks. That's a rule of mine. So what's in there is short.")},{You,TEXT("Can I see it?")},{Dep,TEXT("No.")}}},
    {TEXT("ortiz"),TEXT("Ask about the other file"),nullptr,TEXT("wagon"),TEXT("The second file is Mara Ortiz, district tech, unit 23, called out the same night. Her wagon is on the flat off the old grade, under a county tag."),
     {{You,TEXT("You said two files.")},{Dep,TEXT("Mara Ortiz. District tech, unit 23. Called out that same night.")},{Dep,TEXT("Her wagon's out on the flat, off the old grade, west of the highway. A rancher phoned it in. I tagged it. The district says they'll send a flatbed. They've said so twice.")},{Dep,TEXT("Nobody saw it get out there. Seat's still run up for somebody five foot four. Leave it how you find it.")}}},
    {TEXT("originals"),TEXT("Ask about the station's records"),nullptr,nullptr,TEXT("The deputy asked for the night's receiver log and strip chart, and says the county does not have them."),
     {{You,TEXT("The records for that night aren't in the binder.")},{Dep,TEXT("I know they're not. I asked for the receiver log and the strip chart that night, for the Ortiz file.")},{You,TEXT("Did you get them?")},{Dep,TEXT("You tell me. You work there.")}}},
    {TEXT("advice"),TEXT("Ask what you should do if you see something"),nullptr,nullptr,TEXT("The deputy's advice: write the time down, then call."),
     {{You,TEXT("And if I see something out there?")},{Dep,TEXT("Write the time down. Then call me. In that order.")}}},
   }},
  {TEXT("host"),TEXT("1420 AM / EARL"),
   {{TEXT(""),TEXT("A record is going. Earl leans in the door of the booth with one eye on the turntable.")},{Earl,TEXT("New voice. You'd be from the dishes. I've got you on my list already. Somebody driving around the back roads at night.")}},
   {
    {TEXT("reyes"),TEXT("Ask if Reyes ever called in"),nullptr,TEXT("balloon"),TEXT("Earl says Reyes called the night line once, about a balloon."),
     {{You,TEXT("Did Reyes ever call your show?")},{Earl,TEXT("Once. Didn't give a name, but I know that station's hum on a line. Asked if anybody had seen where a balloon came down. Polite about it.")},{Earl,TEXT("People ask me now what I think. I say here's a record.")}}},
    {TEXT("wagon"),TEXT("Ask what the truckers are saying"),nullptr,TEXT("wagon"),TEXT("Drivers on the CB talk of a carrier on the service band, somewhere off the old grade."),
     {{You,TEXT("Anything on the CB I should know about?")},{Earl,TEXT("There's a fella hauling hay says somebody's left a carrier up on the service band, out off the old grade. Been there weeks.")},{Earl,TEXT("I didn't put that one on the air.")}}},
   }},
  {TEXT("radio_woman"),TEXT("LAUNDROMAT / THE WOMAN"),
   {{TEXT(""),TEXT("She looks up from the radio in her lap, and turns it down one notch and no further.")},{Wom,TEXT("You're from out there.")}},
   {
    {TEXT("reyes"),TEXT("Ask about Reyes"),nullptr,nullptr,TEXT("Reyes washed work shirts at the laundromat on Thursdays. One shirt was left behind and is still there."),
     {{You,TEXT("Did you know the operator before me? Reyes?")},{Wom,TEXT("Thursdays. Two work shirts and a towel. Read the notices and didn't say much.")},{Wom,TEXT("One of those shirts is still in the back. It's clean. Somebody ought to want it.")}}},
    {TEXT("think"),TEXT("Ask what she thinks happened"),nullptr,nullptr,TEXT("The woman at the laundromat will not say what she thinks happened."),
     {{You,TEXT("What do you think happened?")},{Wom,TEXT("I think I've got a load in.")}}},
   }},
  {TEXT("souvenir_man"),TEXT("SOUVENIR SHOP / THE SHOP MAN"),
   {{TEXT(""),TEXT("Ash, tapped off the end of a cigarette, on the counter.")},{Man,TEXT("Hats are half off. They've been half off since Christmas.")}},
   {
    {TEXT("reyes"),TEXT("Ask if Reyes ever came in"),nullptr,TEXT("cairn"),TEXT("Reyes asked the shop man who stacked the rocks on the rise."),
     {{You,TEXT("Did Reyes from the station ever come in?")},{Man,TEXT("Bought nothing. Looked at everything. Asked me who'd been stacking the rocks on that rise out past the array.")},{Man,TEXT("I said kids. There aren't any kids.")}}},
   }},
  {TEXT("ward"),TEXT("SARO OPERATIONS LINE / DR. WARD"),
   {{TEXT(""),TEXT("The line clicks twice.")},{Ward,TEXT("Ward.")}},
   {
    {TEXT("review"),TEXT("Ask where the review stands"),nullptr,nullptr,nullptr,{}},
    {TEXT("why"),TEXT("Ask why you got this job"),nullptr,nullptr,TEXT("Ward says you got the job because you said yes. The posting went up on April 15."),
     {{You,TEXT("Why me?")},{Ward,TEXT("Because you said yes. I don't say that to be unkind.")},{Ward,TEXT("The posting went up on the 15th. You were the fourth to read it and the first to sign.")}}},
    {TEXT("reyes"),TEXT("Ask about Reyes"),nullptr,nullptr,TEXT("Ward says Reyes was a careful operator, and that she has told the county what she knows."),
     {{You,TEXT("What happened to Reyes?")},{Ward,TEXT("Reyes was a good operator. Careful. I've told the county what I know, and you won't get more of it on this line.")},{You,TEXT("Is Reyes dead?")},{Ward,TEXT("I don't know. I would say if I did.")}}},
    {TEXT("originals"),TEXT("Ask where the originals went"),TEXT("gap"),nullptr,TEXT("Ward says the night's originals are not on file, not with the county, and not in her office."),
     {{You,TEXT("The originals for that night are gone.")},{Ward,TEXT("They're not on file. They're not with the county. They're not in my office, if that was next.")},{You,TEXT("Do you know where they are?")},{Ward,TEXT("No. I would very much like to.")}}},
    {TEXT("ortiz"),TEXT("Ask about Mara Ortiz"),TEXT("wagon"),nullptr,TEXT("Ward confirms that she sent Mara Ortiz out that night. It is in the record, over her signature."),
     {{You,TEXT("I've seen wagon 23.")},{Ward,TEXT("I sent her. That's in the record, over my signature.")},{TEXT(""),TEXT("The line is quiet for long enough that you check it.")},{Ward,TEXT("Is there something you need from me regarding the station?")}}},
    {TEXT("leave"),TEXT("Ask what happens if you quit"),nullptr,nullptr,TEXT("Ward says that if you want out, you hang your keys on the board by the lockers."),
     {{You,TEXT("And if I quit?")},{Ward,TEXT("The board's by the lockers. Hang your keys on it. You needn't call me first.")}}},
   }},
  {TEXT("dale"),TEXT("SERVICE LINE / DALE"),
   {{TEXT(""),TEXT("It picks up before it has finished ringing. Somewhere behind him a bench fan, and something being dropped on purpose.")},{Dale,TEXT("What. No, wait, I know this breathing. You are the new one. You have four minutes. I am holding a hot iron and I am not putting it down for you.")}},
   {
    {TEXT("work"),TEXT("Ask what you should be doing"),nullptr,nullptr,TEXT("Dale on the work: do what is on the card, write down what the instrument says, and do not have a feeling about the amber lamps."),
     {{You,TEXT("What should I be doing?")},{Dale,TEXT("What is on the card! It is printed! Somebody typed it so that I would not have to say it to you on the telephone!")},{Dale,TEXT("Point, tune, record, register, file. Amber is normal. If the amber changes you write it down. You do not have a feeling about it. Nobody has ever fixed a receiver with a feeling.")}}},
    {TEXT("array"),TEXT("Ask about the array"),nullptr,nullptr,TEXT("Dale wants the outer branches mended in the order on the sheet, and his tools put back."),
     {{You,TEXT("Half the outer array is down.")},{Dale,TEXT("Half! You went and counted, or you are guessing? No. Do not tell me. Count them. Then the outer branches, in the order on the sheet, not in the order you like.")},{Dale,TEXT("And when you finish you put my tools back where they were. Himmel. Where they were. I will know.")}}},
    {TEXT("reyes"),TEXT("Ask about Reyes"),nullptr,nullptr,TEXT("Dale will not talk about Reyes."),
     {{You,TEXT("Did you work with Reyes?")},{Dale,TEXT("No.")},{You,TEXT("You sent Reyes out on that list.")},{Dale,TEXT("I said no. Nein. Is this the service line? Then ask me about service.")}}},
    {TEXT("list"),TEXT("Ask about the interference list"),TEXT("list"),nullptr,TEXT("Dale says the list was only ever about a pump. He wants Reyes's tags left where they are."),
     {{You,TEXT("I found your list, in the locker.")},{Dale,TEXT("It is a list! I sent somebody to look for a pump in somebody's fence! That is what that was. A pump.")},{You,TEXT("The tags are still out there.")},{Dale,TEXT("Then you leave them wired where they are. They are good tags. Dated, initialled, one to a stop. Do you know how rare that is, in this county? Do you?")},{TEXT(""),TEXT("The fan runs for a while.")},{Dale,TEXT("Leave them.")}}},
    {TEXT("wagon"),TEXT("Tell him you went out to wagon 23"),TEXT("wagon"),nullptr,TEXT("Dale was angry that you went out to wagon 23 alone, and told you to leave it alone."),
     {{You,TEXT("I went out to wagon 23.")},{Dale,TEXT("Alone. In that truck, with those brakes. And at what hour? Dummkopf. Did you touch it?")},{You,TEXT("I switched the radio off.")},{Dale,TEXT("Then you have learned nothing, and it cost you a tank of gas. Stay off that flat. I do not need another form to fill in.")}}},
    {TEXT("message"),TEXT("Ask if Reyes ever called you"),TEXT("known6"),nullptr,TEXT("Reyes used to call Dale on the service line at night. There is one message Dale has played once and will not discuss."),
     {{You,TEXT("Did Reyes ever call you, on this line?")},{Dale,TEXT("Every night. About the coffee. About a lamp. Reyes would read me the same number twice because it was funny, and it was not funny.")},{You,TEXT("And that last night?")},{Dale,TEXT("There is a message on my machine. I have played it one time.")},{You,TEXT("What did it say?")},{Dale,TEXT("It said what it said. You are not going to hear it. Go and point your antennas.")}}},
    {TEXT("hours"),TEXT("Tell him about the hours you can't account for"),TEXT("taken"),nullptr,TEXT("Dale's answer about the lost hours: write the time down, and stay in the building tonight."),
     {{You,TEXT("I've got a hole in my log. Hours. I don't know where they went.")},{Dale,TEXT("You wrote down when it starts? When it stops? Then you have two numbers, which is two more than you had. Nein. I do not want to hear what you think it was.")},{TEXT(""),TEXT("Nothing on the line for a moment, not even the fan.")},{Dale,TEXT("Sleep in the building tonight. Door shut. That is not an order. Nobody here can give you an order, apparently.")}}},
    {TEXT("eat"),TEXT("Ask why he keeps picking up"),TEXT("known6"),nullptr,TEXT("Dale keeps picking up because somebody has to. There are crackers in the bottom drawer."),
     {{You,TEXT("Why do you keep answering, if I'm so much trouble?")},{Dale,TEXT("Because the telephone rings! What kind of question is that? Somebody at that station calls, at night, and somebody picks up. That is how it is supposed to work.")},{TEXT(""),TEXT("Something is set down, gently, for once.")},{Dale,TEXT("Have you eaten? There are crackers in the bottom drawer. Still in the wrapper. You did not hear it from me.")}}},
    {TEXT("quit"),TEXT("Tell him you're thinking of quitting"),nullptr,nullptr,TEXT("Dale told you to quit if you want to, and to use any hook but the first."),
     {{You,TEXT("I've been thinking about hanging up the keys.")},{Dale,TEXT("So hang them! Go! Everybody goes! I have worked with people who could not find their own feet with both hands and they all had somewhere better to be!")},{Dale,TEXT("Not on hook one. Any other hook. Hook one you leave.")}}},
   }},
  {TEXT("nora"),TEXT("SARO OPERATIONS LINE / INCOMING"),
   {{TEXT(""),TEXT("A woman, not young, unhurried. There is a television on somewhere behind her, with the sound down.")},{Nora,TEXT("Is this the night operator?")},{You,TEXT("Who's calling?")},{Nora,TEXT("Nora. I'm at the Sierra Motor Court, and I have been for some time. Don't come by. You've been going through Reyes's things.")}},
   {
    {TEXT("how"),TEXT("Ask how she knows"),nullptr,nullptr,TEXT("A woman called Nora, at the Sierra Motor Court, telephoned SARO. She knows where your truck goes."),
     {{You,TEXT("How would you know that?")},{Nora,TEXT("Everybody in Caliche knows where your truck goes. It's a small place and you are the only thing happening in it.")}}},
    {TEXT("who"),TEXT("Ask what she is to Reyes"),nullptr,nullptr,TEXT("Nora says she is nobody to Reyes. Reyes came to see her once."),
     {{You,TEXT("What are you to Reyes?")},{Nora,TEXT("Nobody. Somebody Reyes came to see, once.")},{You,TEXT("What did Reyes want?")},{Nora,TEXT("No. If I told you what I think, you would write it down, and then it would be one more thing on paper that isn't so.")}}},
    {TEXT("warn"),TEXT("Ask why she called"),nullptr,nullptr,TEXT("Nora's advice: do the work, keep your originals, and do not stay out on the land because you want an answer."),
     {{You,TEXT("Then why call?")},{Nora,TEXT("Do the work. Keep your originals. Don't stay out on that land after dark because you want an answer.")},{Nora,TEXT("I can't promise you that's safe either. I would just rather have said it.")}}},
   }},
 };
 return Table;
}
const TArray<FBark>& Barks()
{
 static const TArray<FBark> Table={
  {TEXT("hello"),TEXT("letter"),1,TEXT("Dale, field maintenance. You are the new one. Amber is NORMAL. Do not touch Bank Three. I do not care who you are. Dale out.")},
  {TEXT("filed1"),TEXT("filed1"),1,TEXT("One job filed. I will alert the newspapers. Now the next one.")},
  {TEXT("locker"),TEXT("locker"),1,TEXT("Who told you to go in that locker? Nobody. That is who. Leave the mug where it is.")},
  {TEXT("tag"),TEXT("tag_windmill"),1,TEXT("You are out on my survey now? Nobody asked you. You read the tags, you do not add to them. Dale out.")},
  {TEXT("late"),TEXT("late"),3,TEXT("It is after two. Are you in the building? Do not tell me about the sky. Be in the building.")},
  {TEXT("branch"),TEXT("branch1"),1,TEXT("You mended a branch. I looked at the line from here. It is a clean joint. Genau. That is the first thing anyone has done right out there since April. Do not get used to me saying it.")},
  {TEXT("filed10"),TEXT("filed10"),1,TEXT("Ten jobs. Ward told me, I did not ask her to. It is almost competent. Keep going.")},
  {TEXT("hole"),TEXT("taken"),1,TEXT("Your log has a hole in it. You write down when it started. You write down when it stopped. Then you call me. In that order, Mensch.")},
  {TEXT("twice"),TEXT("taken2"),1,TEXT("Twice now. Ward told me. One more and they give your nights to somebody else. After dark you stay in the building. Verstanden?")},
  {TEXT("near"),TEXT("assess3"),1,TEXT("Three days to Ward's assessment. She reads numbers. I have seen your numbers. Stop looking out of the window and file something.")},
 };
 return Table;
}
const FPerson* Person(const FString& Id){return People().FindByPredicate([&](const FPerson& P){return Id==P.Id;});}

FString Letter(bool bOlderGame,const FString& Dated,const FString& Assess)
{
 if(bOlderGame)return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\n%s\n\nTo the night operator, SARO.\n\nYou were put on nights in a hurry and nobody sat you down, so I will be plain now.\n\nReyes, who had those nights before you, has not been seen since the morning of April 14. The county has the file and I can't add to it. Since then my office has been asked why the county and the district should keep paying for 27 antennas.\n\nI will assess SARO on %s. If the work is being done, the array is in repair and the accounts are not in arrears, it stays open and so do you. If not, it closes.\n\nNothing about your pay or the work you've done so far changes. Keep your originals. You'll have a word from me each Monday.\n\nField maintenance is Dale, on the service line and on the walkie. You will not meet him. He will shout. He is usually right, and that does not help.\n\nReyes's locker is in the storage room in the east wing, and it has not been cleared. Leave it or clear it as you like.\n\nIf you decide this isn't for you, there's no need to call. Hang your keys on the board by the lockers.\n\nWard"),*Dated,*Assess);
 return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\n%s\n\nTo the night operator, SARO.\n\nWelcome. I will be plain, since nobody else will have been.\n\nThe station has had no night operator since April 14. Reyes, who had the job, has not been seen since that morning. The county has the file and I can't add to it. My office has been asked why the county and the district should keep paying for 27 antennas that nobody is looking after, and I don't have a good answer yet. You are meant to give me one.\n\nI will assess SARO on %s. If the work is being done, the array is in repair and the accounts are not in arrears, it stays open and so do you. If not, it closes.\n\nYou should know you got this job because nobody else wanted it. You wrote on your application that you need the money. So you'll understand that I need the numbers. Three jobs come up on the OPS terminal each morning at 08:00. Register what you measure, file what you register, and keep your originals. You'll have a word from me each Monday.\n\nField maintenance is Dale, on the service line and on the walkie. You will not meet him. He will shout. He is usually right, and that does not help.\n\nReyes's locker is in the storage room in the east wing, and it has not been cleared. Leave it or clear it as you like.\n\nIf you decide this isn't for you, there's no need to call. Hang your keys on the board by the lockers. It has been done before.\n\nWard"),*Dated,*Assess);
}
FString WeekNote(int32 Week,int32 Filed,int32 Concluded,int32 Dishes,int32 Clean,int32 Arrears,int32 DaysLeft,const FString& Assess,int32 Points,int32 PointsNeeded,int32 Taken)
{
 // What she says is what she will assess by: cases carried through, the array, records that can be read. A pile of cards is not it.
 const TCHAR* Remark=Points>=PointsNeeded?TEXT("If I had to assess today, I could defend it. Keep it that way.")
  :Points*2>=PointsNeeded?TEXT("That is enough to argue with. It is not enough to win. The county will not count cards: it wants cases carried through, the array mended and records it can read.")
  :TEXT("I can't defend that. Filing cards is not what I am asked about. Carry a case through, mend the array, keep your records readable.");
 FString Hours;
 if(Taken==1)Hours=TEXT("\n\nYour log has a stretch of hours in it that nobody can account for. Write down what you can check. I would rather have that than a story.");
 else if(Taken>=2)Hours=FString::Printf(TEXT("\n\nYour log has %d stretches of hours in it that nobody can account for. I have had one operator go missing from this station. I am not going to have another. If it happens again I will put somebody else on nights."),Taken);
 return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\nWEEK %d\n\nFiled so far: %d, of them %d clean enough to read. Cases carried to a conclusion: %d. Array: %d of 27. Carried on the sheet: $%d.\n\n%s%s%s\n\nThe assessment is %s, %d days from now.\n\nWard"),Week,Filed,Clean,Concluded,Dishes,Arrears,Remark,Arrears>0?TEXT(" The county will read the carried line first."):TEXT(""),*Hours,*Assess,DaysLeft);
}
FString TakenWarning(const FString& Dated,int32 Taken)
{
 return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\n%s\n\nTo the night operator, SARO.\n\nThe log has %d stretches in it now where the station has no operator and the operator has no account. I am not asking what happened. I am telling you what I will do.\n\nIf there is another, I will take you off nights and put somebody else on the station, whatever the numbers say. That is not a punishment. It is the only thing I can still do about it.\n\nStay in the building after dark if you can. Keep your originals.\n\nWard"),*Dated,Taken);
}
FString ClosedNow(const FString& Dated)
{
 return FString::Printf(TEXT("SARO / CLOSING ORDER IN EFFECT\n%s\n\nThe county line was opened at the meter this morning. The array has been driven to stow and left there; nothing points at anything. The district has tagged the truck for its flatbed and has the keys.\n\nThe station's records stay where they are until Dr. Ward signs for them. The building is not locked. There is nobody to lock it.\n\nThe generator still runs if somebody fuels it."),*Dated);
}
FString Stays(const FString& Dated,int32 Filed,int32 Concluded,int32 Dishes,int32 Arrears,bool bKnowsMuch)
{
 return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\n%s\n\nTo the night operator, SARO.\n\nAssessment. %d jobs filed, %d cases carried to a conclusion, the array at %d of 27, $%d carried on the sheet.\n\nI have recommended that the station stay open through the end of the fiscal year, and the county has not argued. That is more than I expected to be writing.\n\nYou are kept on at the same rate. The files on Reyes and on Mara Ortiz stay open and stay with the county.%s\n\nKeep your originals.\n\nWard"),*Dated,Filed,Concluded,Dishes,Arrears,bKnowsMuch?TEXT("\n\nI know how far you've gone into that. I won't ask you to stop. I will ask you to write down what you saw and leave out what you think."):TEXT(""));
}
FString KnowsEnough(const FString& Dated,int32 Filed,int32 Concluded,int32 Dishes,int32 Arrears,const FString& CloseOn)
{
 return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\n%s\n\nTo the night operator, SARO.\n\nAssessment. %d jobs filed, %d cases carried to a conclusion, the array at %d of 27, $%d carried on the sheet.\n\nThat isn't enough for me to defend, and I didn't try. SARO closes on %s. You have until then.\n\nThe deputy tells me you found the wagon and went through Reyes's things, and that you keep better notes than the county. It isn't an answer, and I won't pretend it is. It's more than anyone else has.\n\nWard"),*Dated,Filed,Concluded,Dishes,Arrears,*CloseOn);
}
FString WhatYouKnow()
{
 return TEXT("What you have, in your hand and in Reyes's: Reyes had the nights at SARO until April 14. Mara Ortiz was called out that same night. The originals of that night are gone from the file. Neither of them has been seen since. Nobody you have spoken to says anything else, and nobody says why.");
}
FString ClosedOriginals(const FString& Dated,int32 Filed,int32 Concluded,int32 Dishes,int32 Arrears,int32 Originals,int32 Photos,const FString& CloseOn)
{
 return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\n%s\n\nTo the night operator, SARO.\n\nAssessment. %d jobs filed, %d cases carried to a conclusion, the array at %d of 27, $%d carried on the sheet.\n\nThat isn't enough. SARO closes on %s, and you'll be paid through the notice.\n\nYour %d originals and %d photographs are the station's record, and nobody is throwing them out. Box them when you're ready and I will sign for them, or keep them until you are sure of me. If anyone asks what they show, tell them what they show.\n\nWard"),*Dated,Filed,Concluded,Dishes,Arrears,*CloseOn,Originals,Photos);
}
FString ClosedNothing(const FString& Dated,int32 Filed,int32 Concluded,int32 Dishes,int32 Arrears,const FString& CloseOn)
{
 return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\n%s\n\nTo the night operator, SARO.\n\nAssessment. %d jobs filed, %d cases carried to a conclusion, the array at %d of 27, $%d carried on the sheet.\n\nThere is nothing here I can point to. SARO closes on %s. You'll be paid through the notice, and the county will want the truck back.\n\nWard"),*Dated,Filed,Concluded,Dishes,Arrears,*CloseOn);
}
FString NextOperator(const FString& Dated,const FString& YourLast,int32 Originals,int32 Photos,int32 Filed)
{
 return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\n%s\n\nTo the night operator, SARO.\n\nWelcome. I will be plain, since nobody else will have been.\n\nThe station has had no night operator since %s. Your predecessor has not been seen since then, and the county has the file. There are two names in the locker bank now and I can't add to either.\n\nYou'll find %d jobs on file, %d originals in the binder and %d photographs in the album, all in order. Whoever it was kept good records.\n\nYou should know you got this job because nobody else wanted it.\n\nThree jobs come up on the OPS terminal each morning at 08:00. Keep your originals.\n\nWard"),*Dated,*YourLast,Filed,Originals,Photos);
}
}

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
   TEXT("SHIFT BINDER / APRIL 1986\n\nApril 1 to April 13, 18:00: all present, all initialed R.\n\nAfter that the rings hold nothing. The receiver log for the night of the 13th is gone. So is the strip chart, the shift journal and the feeder test journal.\n\n~ Originals not on file. County has asked. Do not reconstruct. - W."),
   TEXT("The originals of the night of April 13 are gone from the binder. Receiver log, strip chart, shift journal, feeder journal. Ward's note says not to reconstruct them."),nullptr},
  // ---- the survey tags, out on the land, in the order Reyes went
  {TEXT("tag_windmill"),TEXT("Survey tag at the windmill"),TEXT("hand"),TEXT("SARO RFI / 1"),
   TEXT("SARO RFI SURVEY   POINT 1\nDATE 3-24-86   OPR REYES\nSOURCE: no motor. Brake on. Nothing.\n\n~ This is what I'm doing with my mornings.\n~ Somebody wired the vane to the tower. Not a rancher's knot. Not Dale's either. I asked, and got ten minutes on whether I know what a knot is.\n~ Next: the stock tank on the old grade."),
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
  // ---- M7, weeks three and four: wagon 23's second look, and the thermos it led to (S47PredecessorLate.cpp)
  {TEXT("trip"),TEXT("Wagon 23's trip meter"),TEXT("typed"),TEXT("UNIT 23 / THE DASH"),
   TEXT("TRIP      38.2\nODOMETER  61,904\n\n~ The district yard in Socorro to the pumps at Caliche is thirty-eight miles by the highway. The call-out sheet says TO CALICHE, THEN SARO.\n~ The flat is nine miles past Caliche, by the grade. The trip meter does not have them.\n~ Street tar on the tires. The dust on the mat is the flat's."),
   TEXT("Wagon 23's trip meter reads 38.2: the district yard to the pumps at Caliche, and not a mile more. The flat it stands on is nine miles past the town."),nullptr},
  {TEXT("ortiz_reply"),TEXT("A letter to the night operator, by post, from Socorro"),TEXT("hand"),TEXT("BY POST"),
   TEXT("To the night operator at SARO,\n\nThe district sent me Mara's thermos, with a note that said it was you who found who had it. Thank you.\n\nShe took that thermos on every back-country job and brought it home empty. This time it came back full. That is how I know she never got where she was going. I am not saying anything else by it.\n\nIf the county ever asks you, tell them that. They have stopped asking me.\n\nMara's sister"),
   TEXT("Mara Ortiz's sister wrote after the district sent the thermos on. It came back full, and she says that is how she knows Mara never got where she was going."),nullptr},
  // ---- M7: the Sky Watch's bulletin, by post (S47PredecessorLate.cpp; THE PRINTED MINUTE)
  {TEXT("skywatch"),TEXT("A mimeographed sheet, by post: the Socorro Sky Watch"),TEXT("typed"),TEXT("SOCORRO SKY WATCH"),
   TEXT("SOCORRO SKY WATCH / BULLETIN\nTo members and friends. Dues are a dollar.\n\nWe have it on good authority that something will be heard on the 1420 dial, and seen from the cairn on the rise past the SARO dishes, on the third night from the postmark on this envelope, at ten minutes to one exactly.\n\nBring warm clothes, binoculars and a watch you trust. No flashlights on the rise, please.\n\nThe people from the station are welcome. They never come."),
   TEXT("The Socorro Sky Watch sent the station its bulletin: something to be heard and seen from the cairn on the rise at ten to one, the third night from the postmark."),TEXT("cairn")},
  // ---- M7: among the sheets of the late cases
  {TEXT("case_post"),TEXT("Note in Reyes's hand, in the back of the April log"),TEXT("hand"),TEXT("REYES"),
   TEXT("~ Nobody at the gas station's night window Tuesday or Wednesday. Back Thursday. Wouldn't say where.\n~ Deputy says people go to Belen.\n~ Everybody in this county goes to Belen. I have never once seen Belen."),
   TEXT("In April somebody in Caliche was gone from their post for two nights too, and would not say where. Reyes noted it in the back of the log."),nullptr},
  {TEXT("case_minute"),TEXT("Note in Reyes's hand, folded into a Sky Watch bulletin"),TEXT("hand"),TEXT("REYES"),
   TEXT("~ The van people left one of these at the Mesa. On good authority. Whose?\n~ Went up to the cairn at the time it said. Nothing on the dial. Nothing in the sky. Something on the chart when I got back.\n~ Cairn's higher than when I tagged it. Not measuring it again."),
   TEXT("Reyes had a Sky Watch bulletin too, went up to the cairn at its minute, and found something on the chart afterward. The note says the cairn had grown."),TEXT("cairn")},
  {TEXT("case_raked"),TEXT("Note in Reyes's hand, inside the field folder"),TEXT("hand"),TEXT("REYES"),
   TEXT("~ Two dark cars at the stake Friday, early. They pulled out when I came up the track.\n~ Ground round it raked smooth. I did not walk on it. I don't know why not.\n~ Called the county road office again. They have stopped picking up."),
   TEXT("Reyes saw two dark cars at the stake one Friday morning in April, and the ground raked smooth around it."),TEXT("stake")},
  // ---- by post, after KEPT OPEN, when the final pay was routed back (M7)
  {TEXT("pay_reply"),TEXT("A letter to the night operator, by post"),TEXT("hand"),TEXT("BY POST"),
   TEXT("To the night operator at SARO,\n\nSomebody from Dr. Ward's office sent on the pay, with a note that said it was you who routed it back. Thank you. Nobody had to do that.\n\nWe don't know anything more than we did. Nobody calls. If you ever learn something, I would rather hear it from you than from the county.\n\nThe station is staying open, the note said. Then somebody is still there at night. That helps more than I can say in a letter.\n\nReyes's mother"),
   TEXT("Reyes's mother wrote to the station after the final pay was routed back to the family. She thanks you, and asks to hear it from you, if you ever learn something."),nullptr},
  // ---- T16-17 (→ M3/M7): Reyes's own pocket notebook, page by page through the month (Book() below, S47PredecessorBook.cpp)
  {TEXT("book_backroom"),TEXT("A page from a pocket notebook, in the back room of the old house"),TEXT("hand"),TEXT("REYES"),
   TEXT("MARCH 30\n\n~ This book is mine. Dale gets the tags. This is for what doesn't go on a tag.\n~ Came over to the old house on my break, to see what the back window sees. The pasture. The last branch of the array. The rise past it, if you know where to look.\n~ Somebody sits in this chair. Nobody has lived in the house for years, and the dust on the seat says one person, a few nights a week. Not me. Not till tonight.\n~ Copied the times off the sill into the back of this book. They aren't mine and they aren't the station's.\n~ Leaving a page at the line shack too, behind the calendar. In case this one walks off."),
   TEXT("A page of Reyes's own notebook, March 30, under the coffee can in the back room of the old house. Somebody else sat in that chair. Reyes left another page at the line shack."),TEXT("shack")},
  {TEXT("book_shack"),TEXT("A page from the same notebook, behind the calendar at the line shack"),TEXT("hand"),TEXT("REYES"),
   TEXT("APRIL 2\n\n~ The count on the door frame. Yesterday the last group stopped at two. Today it stops at three.\n~ Nobody lives out here. I sat in the truck an hour with the doors locked to be sure, and felt stupid, and stayed the hour.\n~ I don't put this on the sheet. Ward reads the sheet. I keep carbons of all of it in a folder at the station: R / OWN TIME, behind April's binder, where nobody files anything.\n~ Dale asked on the line why I sound tired. I said the coffee. He said buy better coffee."),
   TEXT("Reyes's page at the line shack, April 2: the count on the door went up by one overnight. Reyes kept carbons in a folder behind April's binder at the station."),nullptr},
  {TEXT("book_signout"),TEXT("A sign-out card behind April's binder"),TEXT("typed"),TEXT("SARO / RECORDS"),
   TEXT("SIGN-OUT CARD\n\nFOLDER:     R / OWN TIME\nOUT:        4-16-86\nTO:         SOCORRO CO. S.O., FOR THE REYES FILE\nSIGNED:     a deputy's initials, pressed hard\nRETURN TO:  SARO / RECORDS\n\n~ The slot behind April's binder is empty but for this card. The folder went to the sheriff's office three days after the last night in the binder."),
   TEXT("Reyes's own folder was signed out of the records room on April 16, to the sheriff's office for the Reyes file. It is to come back to the station."),nullptr},
  {TEXT("book_file"),TEXT("Reyes's folder, R / OWN TIME, returned by post"),TEXT("typed"),TEXT("BY POST / RETURNED"),
   TEXT("SOCORRO COUNTY SHERIFF / SUBSTATION\nRETURNED TO SARO. COPIED FOR THE FILE. NOTHING IN IT THE COUNTY WANTS.\n\n----\n\nR / OWN TIME\nCarbons. Not for the binder.\n\n3-24  windmill. Vane wired. Not Dale's knot.\n3-29  old house, back room, 01:10 to 02:30. The chair. Somebody else uses it.\n3-30  the times on the sill, copied into the book.\n4-1   line shack. Count on the door, two in the last group.\n4-2   line shack. Three.\n4-5   station clock and reference disagree. Both written down.\n4-6   windmill. The vane is turned again, the wire new again. Took a bearing along it from the tower foot.\n4-8   windmill, with the book. A page in the pump housing.\n\n~ Ward asked if I was keeping up with the sheet. I am. This is on top of the sheet."),
   TEXT("The deputy sent Reyes's folder back to the station. Carbons of March and April: the windmill's vane turned twice, and a page of the notebook left in its pump housing."),TEXT("windmill")},
  {TEXT("book_windmill"),TEXT("A page from Reyes's notebook, in the windmill's pump housing"),TEXT("hand"),TEXT("REYES"),
   TEXT("APRIL 8\n\n~ The bearing along the vane from the tower foot runs to the cairn on the rise. I walked it as far as the fence to be sure.\n~ Somebody climbs this tower to turn the vane. There are no ladder marks. I am not going to think about that up here.\n~ The rest of the book goes up to the cairn, under the top stone, where it stays dry. The cairn is just a good dry place. That's all it is.\n~ Mom says call Sunday. I will."),
   TEXT("Reyes's page in the windmill's pump housing, April 8: the vane points at the cairn on the rise, and the rest of the notebook went under its top stone."),TEXT("cairn")},
  {TEXT("book_cairn"),TEXT("The rest of Reyes's notebook, under the cairn's top stone"),TEXT("hand"),TEXT("REYES"),
   TEXT("APRIL 12\n\n~ Saturday. Did the mine this morning. Didn't go in.\n~ Up here at six with the book. The station on one side, the old house on the other, and the back room's window looks straight at this rise. I hadn't seen that till I stood here.\n~ Laid it all side by side: the sill, the shack door, my two clocks, the vane. It doesn't line up into anything. Good. I'd rather it didn't.\n~ Shift tomorrow at six. Coffee filters. Danny's twenty.\n~ Bring the book down Monday and start a clean one.\n\n(The rest of the pages are blank.)"),
   TEXT("The rest of Reyes's notebook, under the cairn's top stone. The last page is Saturday, April 12, at six: a shift tomorrow, coffee filters, and the book to be brought down on Monday."),nullptr},
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
 static const TArray<FCaseNote> Table={{TEXT("THE REPEATED INTERVAL"),TEXT("case_interval")},{TEXT("DISAGREEING RECORDS"),TEXT("case_records")},{TEXT("VISITORS AROUND THE BASE"),TEXT("case_visitors")},
  // M7: the late cases turn up Reyes's April too.
  {TEXT("THE EMPTY POST"),TEXT("case_post")},{TEXT("THE PRINTED MINUTE"),TEXT("case_minute")},{TEXT("THE RAKED GROUND"),TEXT("case_raked")}};
 return Table;
}
const TArray<FBookStep>& Book()
{
 // T16-17 (→ M3/M7): week one at the old house and the line shack, week two in the records room (and the deputy, who has the
 // folder), week three the folder back by post and the windmill and the cairn it leads to, then the deputy, who takes the book.
 // Each page that points to a place puts it on the map, and the page there is already due when it does (the self-test holds it).
 static const TArray<FBookStep> Table={
  {TEXT("book_backroom"),TEXT("backroom"),nullptr,2,TEXT("Under the coffee can of cigarette ends: a page torn from a pocket notebook, folded in four. Pencil, a careful hand."),TEXT("Unfold the page")},
  {TEXT("book_shack"),TEXT("shack"),TEXT("book_backroom"),2,TEXT("Behind the calendar for 1971, folded small: another page from the same pocket notebook."),TEXT("Take it down from behind the calendar")},
  {TEXT("book_signout"),TEXT("binder"),TEXT("book_shack"),8,TEXT("Behind April's binder, in the slot where a folder stood: a sign-out card."),TEXT("Behind the binder: a sign-out card")},
  {TEXT("book_file"),TEXT("post"),TEXT("book_shack"),18,nullptr,nullptr},
  {TEXT("book_windmill"),TEXT("windmill"),TEXT("book_file"),18,TEXT("In the pump-rod housing, wired shut with the same bright wire as the vane: a page in a sandwich bag."),TEXT("Undo the wire")},
  {TEXT("book_cairn"),TEXT("cairn"),TEXT("book_windmill"),18,TEXT("Under the top stone, in a coffee can with its lid taped down: what is left of a pocket notebook. The stones over it are dry."),TEXT("Open the can")},
 };
 return Table;
}
const FBookStep* BookStep(const FString& Doc){return Book().FindByPredicate([&](const FBookStep& S){return Doc==S.Doc;});}
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
    // M7: wagon 23's trip meter stops at his pumps.
    {TEXT("thermos"),TEXT("Ask whether anything of hers was left at the pumps"),TEXT("trip"),nullptr,TEXT("The attendant found a district thermos by the air hose the morning after wagon 23 stood at his pumps, cap on and full. The county did not want it. He gave it to you."),
     {{You,TEXT("The wagon's trip meter stops at your pumps. Was anything of hers left here?")},{Att,TEXT("The county asked me that. I said no, because what they asked was had I found a person.")},
      {TEXT(""),TEXT("He reaches under the counter and sets a steel thermos on it, a big one, DISTRICT 23 stenciled down the side in white.")},
      {Att,TEXT("By the air hose, the Monday. Cap on, full to the top. Nobody'd had any of it. I kept it for whoever came asking right.")},{Att,TEXT("Take it. It's been looking at me.")}}},
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
    // T16-17 (→ M3/M7): Reyes's own folder, signed out of the station; and the notebook, which ends with him.
    {TEXT("folder"),TEXT("Ask about the folder signed out of the station"),TEXT("book_signout"),nullptr,TEXT("The deputy signed Reyes's own folder out of the station on April 16. It goes back to the station by post when the county attorney is through with it."),
     {{You,TEXT("Your office signed a folder out of the station in April. R, own time.")},{Dep,TEXT("I did. The county attorney has it now, which means it's on a pile. When he's through, it goes back to the station by post.")},{Dep,TEXT("He's through when he's through. Give it a week. Then give it another.")}}},
    {TEXT("book"),TEXT("Give him Reyes's notebook"),TEXT("book_cairn"),nullptr,TEXT("The deputy put Reyes's notebook in his file, with where each page was found. The last page is Saturday, April 12, at the cairn on the rise."),
     {{You,TEXT("This was Reyes's. A notebook. I found it a page at a time. The last of it was under the top stone of the cairn.")},{TEXT(""),TEXT("He reads it standing up, all of it, and turns the blank pages too.")},
      {Dep,TEXT("Times and places, and not one idea in it. Reyes would have made a decent deputy.")},{Dep,TEXT("It goes in the file. I'll write down where you found each page, and that's all I'll write. The twelfth is the last day anybody has in Reyes's own hand.")}}},
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
     {{You,TEXT("And if I quit?")},{Ward,TEXT("The board's by the lockers. Hang your keys on it. You don't have to call me first.")}}},
    {TEXT("dale"),TEXT("Ask about Dale"),nullptr,nullptr,TEXT("Ward says Dale has been in field maintenance with the district for eleven years and files a grievance about the trucks every April. If he shouts, let him finish."),
     {{You,TEXT("What should I know about Dale?")},{Ward,TEXT("Field maintenance. Eleven years with the district. He files a grievance about the trucks every April, and I approve about half of them.")},{Ward,TEXT("If he shouts, let him finish. It goes faster.")}}},
   }},
  {TEXT("dale"),TEXT("SERVICE LINE / DALE"),
   {{TEXT(""),TEXT("It picks up before it has finished ringing. Somewhere behind him a bench fan, and something being dropped on purpose.")},{Dale,TEXT("What. No, wait. The station line. So you are the new one. You have four minutes. I have a soldering iron in my hand and I am not putting it down for you.")}},
   {
    {TEXT("work"),TEXT("Ask what you should be doing"),nullptr,nullptr,TEXT("Dale on the work: do what is on the card, write down what the instrument says, and do not have a feeling about the amber lamps."),
     {{You,TEXT("What should I be doing?")},{Dale,TEXT("What is on the card! It is printed! Somebody typed it so that I would not have to say it to you on the telephone!")},{Dale,TEXT("Point, tune, record, register, file. Amber is normal. If the amber changes you write it down. You do not have a feeling about it. Nobody has ever fixed a receiver with a feeling.")}}},
    {TEXT("array"),TEXT("Ask about the array"),nullptr,nullptr,TEXT("Dale wants the outer branches mended in the order on the sheet, and his tools put back."),
     {{You,TEXT("Half the outer array is down.")},{Dale,TEXT("Half! You went and counted, or you are guessing? No. Do not tell me. Count them. Then the outer branches, in the order on the sheet, not in the order you like.")},{Dale,TEXT("And when you finish you put my tools back where they were. Himmel. Where they were. I will know.")}}},
    {TEXT("reyes"),TEXT("Ask about Reyes"),nullptr,nullptr,TEXT("Dale will not talk about Reyes."),
     {{You,TEXT("Did you work with Reyes?")},{Dale,TEXT("No.")},{You,TEXT("You sent Reyes out on that list.")},{Dale,TEXT("I said no. Is this the service line? Then you ask me about service.")}}},
    {TEXT("list"),TEXT("Ask about the interference list"),TEXT("list"),nullptr,TEXT("Dale says the list was only ever about a pump. He wants Reyes's tags left where they are."),
     {{You,TEXT("I found your list, in the locker.")},{Dale,TEXT("It is a list! I sent somebody to look for a pump in somebody's fence! That is what that was. A pump.")},{You,TEXT("The tags are still out there.")},{Dale,TEXT("Then you leave them wired where they are. They are good tags. Dated, initialed, one to a stop. Do you know how rare that is, in this county? Do you?")},{TEXT(""),TEXT("The fan runs for a while.")},{Dale,TEXT("Leave them.")}}},
    {TEXT("wagon"),TEXT("Tell him you went out to wagon 23"),TEXT("wagon"),nullptr,TEXT("Dale was angry that you went out to wagon 23 alone, and told you to leave it alone."),
     {{You,TEXT("I went out to wagon 23.")},{Dale,TEXT("Alone. In that truck, with those brakes, and at what hour? Verdammt. Did you touch it?")},{You,TEXT("I switched the radio off.")},{Dale,TEXT("Then you have learned nothing, and it cost you a tank of gas. Stay off that flat. I do not need another form to fill in.")}}},
    {TEXT("message"),TEXT("Ask if Reyes ever called you"),TEXT("known6"),nullptr,TEXT("Reyes used to call Dale on the service line at night. There is one message Dale has played once and will not discuss."),
     {{You,TEXT("Did Reyes ever call you, on this line?")},{Dale,TEXT("Every night. About the coffee. About a lamp. Reyes would read me the same number twice because it was funny, and it was not funny.")},{You,TEXT("And that last night?")},{Dale,TEXT("There is a message on my machine. I have played it one time.")},{You,TEXT("What did it say?")},{Dale,TEXT("It said what it said. You are not going to hear it. Go and point your antennas.")}}},
    {TEXT("hours"),TEXT("Tell him about the hours you can't account for"),TEXT("taken"),nullptr,TEXT("Dale's answer about the lost hours: write the time down, and stay in the building tonight."),
     {{You,TEXT("I've got a hole in my log. Hours. I don't know where they went.")},{Dale,TEXT("You wrote down when it starts? When it stops? Then you have two numbers, which is two more than you had. Nein. I do not want to hear what you think it was.")},{TEXT(""),TEXT("Nothing on the line for a moment, not even the fan.")},{Dale,TEXT("Sleep in the building tonight. Door shut. That is not an order. Nobody here can give you an order, apparently.")}}},
    {TEXT("eat"),TEXT("Ask why he keeps picking up"),TEXT("known6"),nullptr,TEXT("Dale keeps picking up because somebody has to. There are crackers in the bottom drawer."),
     {{You,TEXT("Why do you keep answering, if I'm so much trouble?")},{Dale,TEXT("Because the telephone rings! What kind of question is that? Somebody at that station calls, at night, and somebody picks up. That is how it is supposed to work.")},{TEXT(""),TEXT("Something is set down, gently, for once.")},{Dale,TEXT("Have you eaten? There are crackers in the bottom drawer. Still in the wrapper. You did not hear it from me.")}}},
    {TEXT("quit"),TEXT("Tell him you're thinking of quitting"),nullptr,nullptr,TEXT("Dale told you to quit if you want to, and to use any hook but the first."),
     {{You,TEXT("I've been thinking about hanging up the keys.")},{Dale,TEXT("So hang them! Go! Everybody goes! I have worked with people who could not find their own feet with both hands and they all had somewhere better to be!")},{Dale,TEXT("Not on hook one. Any other hook. Hook one you leave.")}}},
    {TEXT("gap"),TEXT("Ask about the missing originals"),TEXT("gap"),nullptr,TEXT("Dale says he signs for parts and Ward signs for paper. The originals of April 13 are not in his drawer and not on his bench."),
     {{You,TEXT("The originals for the night of the 13th aren't in the binder.")},{Dale,TEXT("You call the service line about paper? I sign for parts. Ward signs for paper. Ask Ward.")},{You,TEXT("I'm asking you.")},{TEXT(""),TEXT("The iron goes back on its stand, hard.")},{Dale,TEXT("Then the answer is the same, only louder. Not in my drawer. Not on my bench. Ask Ward.")}}},
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
  // D46-20261110 (D46-3): the first night's array turned by itself at 00:41 (FS47Happenings::TurnFirstArray), and his bench was his cover
  // for the printer the day before. His first line on the second day, before anything else of his; only while it is fresh (Meets).
  {TEXT("turned"),TEXT("turned1"),2,TEXT("Not my bench. My bench was off all night. Who put my whole array on one bearing at twenty to one? You write down the time, and you lock the door. Dale out.")},
  {TEXT("filed1"),TEXT("filed1"),1,TEXT("One job filed. I will alert the newspapers. Now the next one.")},
  {TEXT("locker"),TEXT("locker"),1,TEXT("Who told you to go in that locker? Nobody. That is who. Leave the mug where it is.")},
  {TEXT("tag"),TEXT("tag_windmill"),1,TEXT("You are out on my survey now? Nobody asked you. You read the tags, you do not add to them. Dale out.")},
  {TEXT("late"),TEXT("late"),3,TEXT("It is after two. Are you in the building? Do not tell me about the sky. Be in the building.")},
  {TEXT("branch"),TEXT("branch1"),1,TEXT("You mended a branch. I looked at the line from here. It is a clean joint. Genau. That is the first thing anyone has done right out there since April. Do not get used to me saying it.")},
  {TEXT("filed10"),TEXT("filed10"),1,TEXT("Ten jobs. Ward told me, I did not ask her to. It is almost competent. Keep going.")},
  {TEXT("hole"),TEXT("taken"),1,TEXT("Your log has a hole in it. You put a time on each end of it, in ink, and then you call me. Not before, Mensch.")},
  {TEXT("twice"),TEXT("taken2"),1,TEXT("Twice now. Ward told me. One more and they give your nights to somebody else. After dark you stay in the building. Verstanden?")},
  {TEXT("near"),TEXT("assess3"),1,TEXT("Three days to Ward's assessment. She reads numbers. I have seen your numbers. Stop looking out the window and file something.")},
  {TEXT("wagon"),TEXT("wagon"),1,TEXT("The deputy called my bench. Somebody has been out at wagon 23, he says, and he has an idea who. Leave that one to the county.")},
  {TEXT("mine"),TEXT("tag_mine"),1,TEXT("Somebody saw you out at the mine on the west side. That is as far as you go. Not in. Not for a list, not for anybody. Dale out.")},
  {TEXT("pool"),nullptr,8,TEXT("You are still on nights. They had a pool on you at the district. I said three days. Now I owe a man in Socorro five dollars.")},
  {TEXT("kept"),TEXT("kept-salute"),1,TEXT("So she kept us. I ran the whole array up and down from my bench, to see if it still moves. It moves. Do not get used to it. Dale out.")},
  // M7, weeks two to four: what has really gone on (FS47Predecessor::Meets), each once, one a day like the rest.
  // The short-lived ones first (a case open for a day or two, somebody away), so that one a day does not crowd them out.
  {TEXT("minute"),TEXT("case:6+minute_chart"),12,TEXT("Your chart has a burst on it at ten to one. Ten to one! Who keeps appointments in this county? You run the control, then you file. Not the other way.")},
  {TEXT("post"),TEXT("case:5"),12,TEXT("You are asking around Caliche after somebody who is not at their post? The county rang my bench. You are the station, not the county. Write what you saw.")},
  {TEXT("raked"),TEXT("case:7"),12,TEXT("You walked out on their raked ground? Do not tell me. Write down how wide the circle was and the time, and do not go back. Verstanden?")},
  {TEXT("away"),TEXT("town:away"),9,TEXT("Somebody in Caliche is not at their post, the county says. People go to Belen. People come back. You do your card, and you lock the door. Dale out.")},
  {TEXT("wiped"),TEXT("trip_due"),15,TEXT("The deputy drove past wagon 23. Somebody has been in it since April, he says, and wiped the dash. Not him, not the district. You stay off that flat. Dale out.")},
  {TEXT("week4"),TEXT("assess7"),23,TEXT("A week to Ward's numbers. I have shut the pool at the district: nobody would take the other side of it. Do not make me open it again. Dale out.")},
  {TEXT("van"),TEXT("visit:van_empty"),5,TEXT("Their van is out there with the door open, the deputy says. You do not go and close it. You write down where, and you stay in the building.")},
  {TEXT("copter"),TEXT("visit:military_seen"),8,TEXT("Helicopter low over the land this morning, and two cars that are not the county's. You did not see them. No. You write down that you did, with the time.")},
  {TEXT("thermos"),TEXT("talk_attendant_thermos"),15,TEXT("The attendant says you have the thermos from unit 23. Tag it and put it in the outgoing tray; the district runs it to Socorro. Do not drink from it, Mensch.")},
  {TEXT("trip"),TEXT("trip"),15,TEXT("You were out at wagon 23 again, reading the dash? The deputy read it in April. He wrote down the number and not one word of what he thought. Do the same.")},
  {TEXT("inspector"),TEXT("inspector"),22,TEXT("The county inspector wrote to you? He wrote to my bench too. He wants to know whether the array moves. I said ask the operator. So make it move.")},
  {TEXT("skywatch"),TEXT("visit:let_in"),5,TEXT("You let the people with the van into my station? They read my logbooks. I know, because the logbooks are in order now, and they never are.")},
  {TEXT("filed"),TEXT("filed:case"),8,TEXT("You filed an assessment. Now the county reads it and rings my bench to ask what it means, and I say ask the operator. Do not make me a liar.")},
  {TEXT("halfway"),TEXT("array_low"),14,TEXT("Two weeks, and half my array is still lying on its back. Ward reads that line first, the county second, and I read it every morning. Mend something.")},
  // T16-17 (→ M3/M7): Reyes's notebook, the first page and the last.
  {TEXT("oldhouse"),TEXT("book_backroom"),2,TEXT("You were over in the old house on your break? Reyes did that. Every break. I said it was a waste of a break. That is all I said. Dale out.")},
  {TEXT("notebook"),TEXT("talk_deputy_book"),18,TEXT("The deputy says you brought him a notebook. Good. That is where it goes. Not on my bench, not in your locker. Now do the card. Dale out.")},
 };
 return Table;
}
const TArray<FWalkieLine>& WalkieLines()
{
 static const TArray<FWalkieLine> Table={
  // He calls.
  {TEXT("hail"),TEXT("SARO night, this is Dale. Come in.")},
  {TEXT("hail_again"),TEXT("Night operator! I know you have the set. Come in!")},
  {TEXT("hail_gone"),TEXT("Nothing? Fine. Then it goes on a slip by the phone. Dale out.")},
  {TEXT("finally"),TEXT("Finally. Next time you answer the first time.")},
  // D45-20261110 (D45-26): after his hello, the cover for the night's burst on the printer (FS47Predecessor::StepBench). No take yet.
  {TEXT("bench"),TEXT("One more thing. If the printer runs a line by itself tonight, that is my bench in the shop, running the array up and down. You ignore it. Dale out.")},
  // He is called: by the hour, and by what the player is doing.
  {TEXT("again"),TEXT("Again? What. Nothing? Then take your thumb off the button. Dale out.")},
  {TEXT("enough"),TEXT("That is four times today. Four! I am turning my set to the wall. Call the service line in the morning. Dale out.")},
  {TEXT("missed"),TEXT("Now you call! I called you before, on channel one. You keep it on one, and when I call, you answer. Dale out.")},
  {TEXT("driving"),TEXT("You are driving and talking on my channel? Both hands on the wheel. Out.")},
  {TEXT("hours"),TEXT("You sound wrong. Both times are in the log? Good. Tonight you stay in the building. Dale out.")},
  {TEXT("late_in"),TEXT("At this hour? You are inside? Good. Stay in. You do not open the door to look at anything. Dale out.")},
  {TEXT("late_out"),TEXT("After two and you are outside? Nein. I do not want to know why. Go in. Now. Dale out.")},
  {TEXT("midnight"),TEXT("Do you know what time it is? I know what time it is. Is something on fire? No? Then write it in the log. Dale out.")},
  {TEXT("day"),TEXT("I am up a pole in the sun with a meter in my teeth. Call the service line like a person. Dale out.")},
  {TEXT("station"),TEXT("You are standing next to a telephone. A telephone! Use the card, use the telephone, and leave my channel alone.")},
  {TEXT("array"),TEXT("Antennas down, and you call me? The sheet has the order. Read the sheet, then mend them. Dale out.")},
  {TEXT("far"),TEXT("Where are you, out there with my truck? You bring it back with four tires and gas in it. Dale out.")},
  {TEXT("evening"),TEXT("What. Is it the amber lamps? The amber is normal. Do the card. Dale out.")},
  // What a stranger rang in, passed on.
  {TEXT("relay_pre"),TEXT("The county put a call through to my bench, so now it is your problem.")},
  {TEXT("relay_post"),TEXT("I am not driving out there. Dale out.")},
 };
 return Table;
}
FString WalkieLine(const TCHAR* Id)
{
 if(!Id)return FString();
 for(const FWalkieLine& L:WalkieLines())if(FCString::Strcmp(L.Id,Id)==0)return L.Line;
 return FString();
}
const FPerson* Person(const FString& Id){return People().FindByPredicate([&](const FPerson& P){return Id==P.Id;});}

FString Letter(bool bOlderGame,const FString& Dated,const FString& Assess)
{
 if(bOlderGame)return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\n%s\n\nTo the night operator, SARO.\n\nYou were put on nights in a hurry and nobody sat you down, so I will be plain now.\n\nReyes, who had those nights before you, has not been seen since the morning of April 14. The county has the file and I can't add to it. Since then my office has been asked why the county and the district should keep paying for 27 antennas.\n\nI will assess SARO on %s. If the work is being done, at least one of the station's open questions is carried through to the end on the OPS case board, the array is in repair and the weekly sheet carries less than $80, it stays open and so do you. If not, it closes. One of those cases has to rest on something you brought in from out on the land (a place, a mark, a test or a time noted in the field), not only on what can be read at the desk.\n\nNothing about your pay or the work you've done so far changes. Keep your originals. You'll hear from me every Monday.\n\nField maintenance is Dale, on the service line and on the walkie. You will not meet him. He will shout. He is usually right, and that does not help.\n\nReyes's locker is in the storage room in the east wing, and it has not been cleaned out. Leave it or clean it out, as you like.\n\nIf you decide this isn't for you, there's no need to call. Hang your keys on the board by the lockers.\n\nWard"),*Dated,*Assess);
 // FIX-20261110 (Tom, point 4): the welcome letter out of the box at the road. It says how to get in (the keys are in its
 // envelope, the station is dark, the walkie and the recorder are on the dresser inside the door) and what the job is: the
 // terminals say what to receive and where to go and look, and what comes back is a picture, a sample, a time and a place.
 return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\n%s\n\nTo the new night operator, SARO.\n\nWelcome. The keys in this envelope open every outside door. Nobody has been in for some weeks, so you will find the station dark: each room has a light switch on the wall by its door. The field radio and the tape recorder are on the dresser inside the visitor entrance. They are yours now. Carry them.\n\nThe job is this. SARO listens to the sky over Caliche with 27 antennas, and it writes down what it hears. Three jobs come up on the OPS terminal each morning at 08:00: routine work at the receivers, the reel deck and the darkroom. The terminals will also tell you when something out on the land wants looking at. Then you drive out and look, and you bring back what can be filed: a picture, a sample, a reading, a time and a place. Register what you measure, file what you register, and keep your originals. When something on the case board does not add up, see it through. The station's own files are on the OPS terminal under RECORDS: read 02 and 08 before your first night.\n\nNow the part nobody else will have told you. The station has had no night operator since April 14. Reyes, who had the job, has not been seen since that morning. The county has the file and I can't add to it. My office has been asked why the county and the district should keep paying for 27 antennas that nobody is looking after, and I don't have a good answer yet. You are meant to give me one.\n\nI will assess SARO on %s. If the work is being done, at least one of the station's open questions is carried through to the end on the OPS case board, the array is in repair and the weekly sheet carries less than $80, it stays open and so do you. If not, it closes. One of those cases has to rest on something you brought in from out on the land (a place, a mark, a test or a time noted in the field), not only on what can be read at the desk.\n\nYou should know you got this job because nobody else wanted it. You wrote on your application that you need the pay. So you'll understand that I need the numbers. You'll hear from me every Monday.\n\nField maintenance is Dale, on the service line and on the walkie. You will not meet him. He will shout. He is usually right, and that does not help.\n\nReyes's locker is in the storage room in the east wing, and it has not been cleaned out. Leave it or clean it out, as you like.\n\nIf you decide this isn't for you, there's no need to call. Hang your keys on the board by the lockers. It has been done before.\n\nWard"),*Dated,*Assess);
}
FString WeekNote(int32 Week,int32 Filed,int32 Concluded,int32 Dishes,int32 Clean,int32 Arrears,int32 DaysLeft,const FString& Assess,int32 Points,int32 PointsNeeded,int32 Taken,int32 Land,bool bWouldStay,const FString& Levers,int32 Reprieved,int32 FieldCases)
{
 // CHAIN-2-20261109: the same line every week, so the player sees where it stands.
 const int32 FromLand=FieldCases<0?Concluded:FieldCases;
 const FString LandCase=FString::Printf(TEXT(" Of those, with something brought in from out on the land: %d. I need one.%s"),FromLand,FromLand<1?TEXT(" A place, a mark or a time noted out there counts; what you read at your own desk does not."):TEXT("")); // CHAIN-TEXT-20261110
 // What the land gave at night is something she can use: what was seen, where and when, not what it was.
 const FString Field=Land>0?FString::Printf(TEXT(" Field records from out on the land at night: %d %s. Those I can use, as long as they say what you saw and where, not what you think it was."),Land,Land==1?TEXT("kind"):TEXT("kinds")):FString();
 // What she says is what she will assess by (M7): cases carried through, the array, records that can be read. A pile of cards is
 // not it, and the count of cards no longer earns anything past the floor.
 // With the points there and a case carried through, what still fails is elsewhere on the sheet (Decide: arrears, the floor).
 const TCHAR* Remark=bWouldStay?TEXT("If I had to assess today, I could defend it. Keep it that way.")
  :Points>=PointsNeeded?(Concluded<1?TEXT("The rest of it is close to what I need. What is missing is a case carried through to the end: the county will not keep a station open on routine.")
   :FromLand<1?TEXT("The rest of it is close to what I need. What is not there yet is a case with something in it from out on the land: a place, a mark, a time from the field. The county will not keep a station open on what it can see from its own desk.")
   :TEXT("The rest of it is close to what I need. What is not is on this sheet, and the county will read all of it."))
  :Points*2>=PointsNeeded?TEXT("That is enough to argue with. It is not enough to win. The county will not count cards: it wants cases carried through, the array mended and records it can read.")
  :TEXT("I can't defend that. Filing cards is not what I am asked about. Carry a case through, mend the array, keep your records readable.");
 FString Hours;
 // LOOP-GONE-LEGIBLE-20261113 (→ M7): after the first she says the rule too, in the same words as her letter (HoursSaid)
 if(Taken==1)Hours=TEXT("\n\nYour log has a stretch of hours in it that nobody can account for. Write down what you can check. I would rather have that than a story. If it comes to three times, I will give the post to somebody else.");
 // What she has done only once "Not yet" has answered this very count; before that (noticed, not yet acted on) only that she meant it.
 else if(Taken>=3&&Reprieved>0&&Taken==Reprieved)Hours=FString::Printf(TEXT("\n\nYour log has %d stretches of hours in it that nobody can account for. I said what I would do after another, and I did it. You asked for the nights back, and you have them, on my word and not on the numbers. There will not be a second time."),Taken);
 else if(Taken>=3)Hours=FString::Printf(TEXT("\n\nYour log has %d stretches of hours in it that nobody can account for. I said what I would do after another. I meant it."),Taken);
 else if(Taken>=2)Hours=FString::Printf(TEXT("\n\nYour log has %d stretches of hours in it that nobody can account for. I have had one operator go missing from this station. I am not going to have another. If it happens again I will put somebody else on nights."),Taken);
 const FString More=Levers.IsEmpty()?FString():TEXT("\n\n")+Levers;
 return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\nWEEK %d\n\nFiled so far: %d, and %d of them clean enough to read. Cases carried to a conclusion: %d.%s Array: %d of 27. Carried on the sheet: $%d.%s\n\n%s%s%s%s\n\nThe assessment is %s, %d days from now.\n\nWard"),Week,Filed,Clean,Concluded,*LandCase,Dishes,Arrears,*Field,Remark,Arrears>0&&Arrears<CarriedWarn?TEXT(" The county will read the carried line first."):TEXT(""),*More,*Hours,*Assess,DaysLeft);
}
FString Levers(bool bLogsCount,bool bTapesCount,int32 TrustPoints,bool bPhotosCount,int32 Guessed,int32 Arrears,int32 Cap)
{
 TArray<FString> L;
 L.Add(bLogsCount?TEXT("Your night sheets reach me. Keep sending them."):TEXT("Your night sheets reach me when you telex them, and a station that reports its nights counts for something."));
 L.Add(bTapesCount?TEXT("The tapes you played back are something I can send on."):TEXT("A tape with something on it that nobody heard out there is something I could send on."));
 L.Add(TrustPoints>0?TEXT("Your weekly reports have been whole. That is noticed."):TrustPoints<0?TEXT("Your weekly reports leave things out, and I have found some of them. I read those reports."):TEXT("Send me the weekly report from OPS, and send it whole. I read those reports."));
 L.Add(bPhotosCount?TEXT("The photographs are on file."):TEXT("A few photographs of what you saw would help. Prints of the same frame are not more photographs."));
 if(Guessed>0)L.Add(TEXT("A filing with nothing behind it I can't put in front of the county. Look at the other place, or run a control, before you file."));
 if(Arrears>=CarriedWarn)L.Add(FString::Printf(TEXT("$%d is carried on the sheet. Past $%d on that line I can't put my name to keeping the station open, whatever else is in order."),Arrears,Cap));
 return FString::Join(L,TEXT(" "));
}
FString Apart(int32 Files)
{
 if(Files<1)return FString();
 if(Files==1)return TEXT(" One thing in your field notes stands apart from the rest now, by your own notes. Keep it apart, and keep to what you saw.");
 static const TCHAR* const Many[]={TEXT("Two"),TEXT("Three"),TEXT("Four"),TEXT("Five")};
 return FString::Printf(TEXT(" Keep them apart. %s separate things, by your own notes."),Many[FMath::Clamp(Files,2,5)-2]);
}
FString KeptNote(int32 Week,int32 Filed,int32 Dishes,int32 Arrears,const FString& FiscalEnd,int32 Taken,const FString& Levers)
{
 const FString Hours=Taken>0?FString(TEXT("\n\nThe hours in your log nobody can account for are still in it. Stay in the building after dark if you can.")):FString();
 return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\nWEEK %d\n\nThe station is open. Filed: %d. Array: %d of 27. Carried on the sheet: $%d.\n\nThere is nothing to win this time. SARO stays open through the end of the fiscal year, %s, and after that it goes into next year's budget with the rest of the district. Keep the work going, and keep your originals.%s%s%s\n\nWard"),Week,Filed,Dishes,Arrears,*FiscalEnd,Levers.IsEmpty()?TEXT(""):TEXT("\n\n"),*Levers,*Hours);
}
FString Followed(bool bWagon,bool bDeputy,bool bPayReturned,bool bThermos,bool bBook)
{
 TArray<FString> L;
 if(bBook)L.Add(TEXT("The deputy writes that Reyes's notebook is in his file now, with where each page of it was found. He says it is the first thing in that file in Reyes's own hand."));
 if(bThermos)L.Add(TEXT("The district sent Mara Ortiz's thermos on to her family. Her sister has written to my office to ask that I thank whoever found it."));
 if(bWagon)L.Add(TEXT("The district has sent its flatbed for wagon 23 at last, and the search for Mara Ortiz is back on the sheriff's desk, with your notes in the file."));
 if(bDeputy)L.Add(TEXT("The deputy asked for a copy of your log for the Ortiz file. I sent it. He said it was the first one that gave him times and not ideas."));
 if(bPayReturned)L.Add(TEXT("Reyes's final pay has gone from my office to the family, with a note from me saying who routed it back. If they write, it will come to the station."));
 return FString::Join(L,TEXT(" "));
}
FString HoursSaid(int32 Taken)
{
 // LOOP-GONE-LEGIBLE-20261113 (→ M7): the rule in her words: hours not accounted for, how many times, and what a third does. Never how.
 if(Taken==1)return TEXT("Your log shows hours nobody can account for. It has happened once. If it comes to three times, I will give the post to somebody else.");
 if(Taken==2)return TEXT("Your log shows hours nobody can account for. It has happened twice. If it happens again, I will give the post to somebody else.");
 return FString();
}
FString TakenWarning(const FString& Dated,int32 Taken)
{
 // LOOP-GONE-LEGIBLE-20261113: a letter after the first stretch as well as after the second, each with the count and the rule
 if(Taken==1)return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\n%s\n\nTo the night operator, SARO.\n\nThe log has a stretch in it where the station had no operator and the operator has no account. It has happened once. I am not asking what happened. I am telling you what I will do.\n\nIf it comes to three times, I will take you off nights and give the post to somebody else, whatever the numbers say. I would rather tell you now than after.\n\nStay in the building after dark if you can. Keep your originals.\n\nWard"),*Dated);
 return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\n%s\n\nTo the night operator, SARO.\n\nThe log has %d stretches in it now where the station has no operator and the operator has no account. It has happened twice. I am not asking what happened. I am telling you what I will do.\n\nIf there is another, I will take you off nights and give the post to somebody else, whatever the numbers say. That is not a punishment. It is the only thing I can still do about it.\n\nStay in the building after dark if you can. Keep your originals.\n\nWard"),*Dated,Taken);
}
FString ClosedNow(const FString& Dated)
{
 return FString::Printf(TEXT("SARO / CLOSING ORDER IN EFFECT\n%s\n\nThe county cut the power at the meter this morning. The array has been driven to stow and left there; nothing points at anything. The district has tagged the truck for its flatbed and has the keys.\n\nThe station's records stay where they are until Dr. Ward signs for them. The building is not locked. There is nobody to lock it.\n\nThe generator still runs if somebody fuels it."),*Dated);
}
FString Stays(const FString& Dated,int32 Filed,int32 Concluded,int32 Dishes,int32 Arrears,bool bKnowsMuch,const FString& FoundOut)
{
 return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\n%s\n\nTo the night operator, SARO.\n\nAssessment. %d jobs filed, %d cases carried to a conclusion, the array at %d of 27, $%d carried on the sheet.\n\nI have recommended that the station stay open through the end of the fiscal year, and the county has not argued. That is more than I expected to be writing.\n\nYou are kept on at the same rate. The files on Reyes and on Mara Ortiz stay open and stay with the county.%s%s%s\n\nKeep your originals.\n\nWard"),*Dated,Filed,Concluded,Dishes,Arrears,FoundOut.IsEmpty()?TEXT(""):TEXT(" "),*FoundOut,bKnowsMuch?TEXT("\n\nI know how far you've gone into that. I won't ask you to stop. I will ask you to write down what you saw and leave out what you think."):TEXT(""));
}
FString Inspector(const FString& Dated,const FString& Assess,int32 Weakest)
{
 static const TCHAR* const First[]={
  TEXT("I will ask first for a case your station has carried through to the end, with something behind it that was brought in from out on the land. Routine I can get from any station."), // CHAIN-2-20261109
  TEXT("I will look first at the array. A station with a third of its antennas down is a station I am asked to close."),
  TEXT("I will look first at the work filed. I am told there is not much of it."),
  TEXT("I will read the carried line on the weekly sheet first. A station that cannot keep its accounts does not get kept."),
  TEXT("I will read the filed records first. A record nobody can read is not a record."),
  TEXT("I have read the weekly sheets and I have no questions yet. That is unusual, and I will have some on the day."),
 };
 return FString::Printf(TEXT("SOCORRO COUNTY / OFFICE OF THE INSPECTOR\n%s\n\nTo the night operator, SARO.\n\nDr. Ward's assessment of the station is %s. I will be reading it, and the county will go by what I say about it.\n\n%s\n\nI do not need to see you. I need to see the station's records.\n\nOffice of the County Inspector"),*Dated,*Assess,First[FMath::Clamp(Weakest,0,5)]);
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
 return FString::Printf(TEXT("SARO / OFFICE OF THE DUTY OFFICER\n%s\n\nTo the night operator, SARO.\n\nWelcome. I will be plain, since nobody else will have been.\n\nThe station has had no night operator since %s. Your predecessor has not been seen since then, and the county has the file. There are two names in the locker bank now and I can't add to either.\n\nYou'll find %d jobs on file, %d originals in the binder and %d photographs in the album, all where they belong.\n\nYou should know you got this job because nobody else wanted it.\n\nThree jobs come up on the OPS terminal each morning at 08:00. Keep your originals.\n\nWard"),*Dated,*YourLast,Filed,Originals,Photos);
}
}

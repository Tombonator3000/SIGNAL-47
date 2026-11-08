#include "S47Ledger.h"

// Everything the Ledger can print, in American English of 1986. Every person, business and film here is made up. Freeform's line
// holds (Tom, 7 November): the outside of what happened and nothing from Story's far side; S47Ledger::SelfTest refuses the words.
namespace S47LedgerText{using S47Ledger::ETopic;}

const TArray<const TCHAR*>& S47LedgerText::Locals()
{
 static const TArray<const TCHAR*> Pot={
  TEXT("The county commission voted two to one Monday to grade the Baca lease road before the summer rains. Commissioner Pruitt voted no, saying the rains never come anyway."),
  TEXT("The water board has put off three new well permits east of the highway until its June meeting. Chairman Ignacio Sedillo said the aquifer study is not finished."),
  TEXT("Irrigators on the Salado ditch are reminded that the ditch boss sets the turns this year, not the first one out with a shovel."),
  TEXT("Feeder calves brought $62 to $68 a hundredweight at the sale barn last week, steers a little more. Hay is up five dollars a ton since March."),
  TEXT("Cull cows sold for $38 to $42 a hundredweight, down from April. \"Too many cows and not enough rain,\" one seller said."),
  TEXT("The high school sent four runners to the district track meet in Socorro. Junior Delia Montoya placed second in the mile."),
  TEXT("Spring football practice opens Monday at the high school field. Coach Harlan Mims has eleven boys and would like a few more."),
  TEXT("The Caliche Wildcats lost to Magdalena 7 to 5 in baseball Saturday after leading through six innings."),
  TEXT("The high school rodeo club holds a jackpot roping at the county arena Saturday. Entry is $10; the concession stand will be open."),
  TEXT("Entries are open for the Memorial Day rodeo. Barrel racers sign up at the feed store. Bull riders, the arena manager says, sign up wherever they like."),
  TEXT("The Altar Society's spring bazaar is Saturday from 9 to 3 in the parish hall: quilts, enchiladas and the cake walk. Proceeds go to the new roof."),
  TEXT("The Methodist women serve a chicken supper at the church Friday at 6. Adults $3.50, children $1.50."),
  TEXT("The volunteer fire department holds its pancake breakfast at the station Sunday. Firemen will test any smoke detector brought along."),
  TEXT("A public hearing on the proposed truck stop at the highway junction south of town is set for May 20 at the courthouse annex. The plans call for twelve diesel pumps and a cafe."),
  TEXT("Owners along the highway say a truck stop would bring jobs and noise in about equal measure. The planning board has fourteen letters on file, nine against."),
  TEXT("The state highway department will patch and seal the highway between the diner and the county line through the end of the month. Expect a flagman and a wait."),
  TEXT("The county clerk reminds voters that registration for the June primary closes at the end of the month."),
  TEXT("The library is closed Tuesday while new shelves go in. Overdue books may be left in the box by the door, and no fines will be charged this week."),
  TEXT("The 4-H club weighs lambs at the fairgrounds Saturday morning. Members should bring their record books."),
  TEXT("Extension agent Verna Kittredge will speak on windbreaks and soil blowing at the grange hall Thursday at 7. Coffee provided."),
  TEXT("The veterans' post will place flags at the cemetery for Memorial Day and asks families to leave the gravesides clear by Saturday."),
  TEXT("The cemetery association is collecting its $5 dues for upkeep. The mowing is done by volunteers, and the association says it shows."),
  TEXT("Graduation for the class of '86 is May 23 in the high school gym. There are nineteen seniors this year."),
  TEXT("The school board accepted a bid of $8,400 to reroof the elementary school. Work starts when school lets out."),
  TEXT("The school bus east of the highway runs fifteen minutes early through May while the arroyo road bridge is closed."),
  TEXT("The arroyo road bridge is closed to anything over three tons until the county engineer has looked at the pilings."),
  TEXT("The electric co-op will replace poles along the old grade next week. Members on that line may be without power for two hours Wednesday morning."),
  TEXT("The telephone co-op says a new switch will let rural members dial Socorro without the operator by fall."),
  TEXT("Births: a son, Joaquin, 7 lb. 2 oz., to Ramon and Elsie Gallegos of Caliche; a daughter, Amber Lynn, 6 lb. 9 oz., to Dwight and Cora Pettit."),
  TEXT("Births: twin daughters to Lorenzo and Paula Archuleta, at the hospital in Socorro. Mother and girls are doing well. The father is resting."),
  TEXT("Died: Wendell Ashby, 81, who ran sheep on the west side for fifty years and the grange for thirty. Services Saturday at 10 at the Methodist church."),
  TEXT("Died: Ramona Quintana, 74, retired teacher, who taught most of Caliche to read. Rosary Friday at 7, Mass Saturday at 9."),
  TEXT("Died: Floyd Hensley, 68, while fixing fence. He leaves his wife, two sons, and a dog that will not get out of the truck."),
  TEXT("Died: Esperanza Lucero, 92, the oldest member of the parish. She remembered the first automobile in Caliche and did not think much of it."),
  TEXT("The Feed & Seed has moved its loading dock to the north side. Customers are asked not to park on the old one."),
  TEXT("The post office closes at noon on Memorial Day. Rural carriers will not run that day."),
  TEXT("Rural patrons are reminded to keep the ground in front of their mailboxes clear, so the carrier can reach without leaving the vehicle."),
  TEXT("Wind Tuesday took the roof off a hay shed on the Vigil place and set it down in the neighbor's tank. Nobody was hurt. The hay is another matter."),
  TEXT("A late frost Sunday night nipped the apricots along the river. Growers say that makes three years running."),
  TEXT("Burning is banned in the county until further notice because of the wind. The transfer station still takes trash on Saturdays."),
  TEXT("The transfer station is open Saturdays from 8 to 4. Tires $1 each, appliances $5. The attendant asks that nobody leave a couch at the gate again."),
  TEXT("The garden club meets Tuesday at the home of Iva Dean. The program is growing tomatoes in caliche, which members insist can be done."),
  TEXT("The senior center serves lunch weekdays at 11:30 for a dollar. This week: green chile stew, meat loaf, and fish on Friday."),
  TEXT("Bingo returns to the parish hall on Friday nights at 7. The jackpot is $100."),
  TEXT("The fair board needs a superintendent for the poultry barn. The last one retired after the business with the rooster."),
  TEXT("The Rotary Club heard a talk Thursday on the history of the old mine on the west side. It never paid out, the speaker said, which surprised nobody at the table."),
  TEXT("A dance at the community center Saturday benefits the ambulance fund. Music by the Rio Salado Ramblers. Admission $3."),
  TEXT("The ambulance crew is two short and asks for volunteers who can take calls at night. Training is free."),
  TEXT("The cattlemen's association meets at the Mesa Diner Tuesday at noon to talk about the brand inspection fee."),
  TEXT("Brand inspector Leon Pacheco reminds ranchers that stock moved across the county line needs papers, even when it is only going to the neighbor's."),
  TEXT("The extension office has soil test kits for $4. Results come back from the university in about three weeks."),
  TEXT("The highway rest area north of town closes for repairs for two weeks. Travelers should plan ahead, the state says."),
  TEXT("Caliche Drug will be closed Monday for inventory."),
  TEXT("The Plaza will show a free matinee for children on the last day of school. Popcorn is a dime."),
  TEXT("Sixth graders planted a row of elms along the playground fence and have promised to water them all summer."),
  TEXT("The sheriff's office has a second patrol unit on nights through the end of the month, the sheriff said, \"for as long as the overtime holds out.\""),
  TEXT("The county will spray for mosquitoes along the river after the first warm week. Beekeepers should call the extension office."),
  TEXT("A coyote has been getting chickens along the river road. The county trapper says he will be out when he gets out."),
  TEXT("Lost and found at the courthouse: two sets of keys, a child's boot and a pair of reading glasses. Ask at the clerk's window."),
  TEXT("The Methodist youth wash cars Saturday in front of the Feed & Seed. Donations go to summer camp."),
  TEXT("The town well dropped four feet over the winter, the water board reports. Residents are asked to water lawns only after six."),
  TEXT("Engineers for the proposed truck stop measured the highway junction Friday. A spokesman said nothing has been decided."),
  TEXT("The county road crew graded the track to the old survey station up the hill. Past the second cattle guard it is still four-wheel drive."),
  TEXT("The school district counts 214 children this spring, six fewer than last year."),
  TEXT("The adult softball league starts in June. Teams of ten sign up at the gas station."),
  TEXT("The courthouse clock has been fixed. It was running eleven minutes slow, which some say was the best thing about it."),
  TEXT("The county agent warns that grasshoppers are hatching early on the east side. Bait is available at cost."),
  TEXT("Members of the Caliche Homemakers will judge pies at the June fair and are taking applications from people who like pie."),
 };
 return Pot;
}

const TArray<const TCHAR*>& S47LedgerText::Sheriff()
{
 static const TArray<const TCHAR*> Pot={
  TEXT("A cow loose on the highway north of the Mesa Diner at 6:40 a.m. Owner notified."),
  TEXT("A car in the bar ditch on the arroyo road. The driver said a deer ran out. No injuries."),
  TEXT("Break-in at the Feed & Seed overnight. Missing: two sacks of seed oats and a pair of gloves."),
  TEXT("A rancher reported a gate left open on the Baca lease road. Deputy closed it."),
  TEXT("Barking dog on the east side, the third call this month. Owner advised."),
  TEXT("Hubcap found on the highway at mile 12. Ask at the substation."),
  TEXT("A pickup with a flat tire left on the shoulder south of town overnight. The owner came for it in the morning."),
  TEXT("Deputy helped a motorist out of gas near the motel."),
  TEXT("Horses reported on the highway by the cattle guard. Gone when the deputy arrived."),
  TEXT("Shoplifting at Caliche Drug: a bottle of aspirin and a comb. Juvenile released to parents."),
  TEXT("Prowler reported behind the laundromat. Deputy found a cat."),
  TEXT("Fight in the community center parking lot after the dance. Two men cited."),
  TEXT("Mailbox knocked down on the river road. Probably a truck, the owner said."),
  TEXT("Grass fire along the railroad right-of-way, put out by the volunteer fire department."),
  TEXT("A dog on the highway shoulder for two days. Animal control notified."),
  TEXT("Somebody painted words on the water tank on the old grade. The words were misspelled."),
  TEXT("Saddle stolen from a tack room west of town: brown, roughout, a name burned under the cantle."),
  TEXT("Car parked at the cemetery after dark. Two teenagers, sent home."),
  TEXT("Power pole hit by a stock trailer on the highway. Co-op crew on scene by midnight."),
  TEXT("Guest at the motel playing a radio too loud. Guest turned it down."),
  TEXT("Deputy stood by while a family moved out of a house on the east side."),
  TEXT("Loose bull in the schoolyard before school. Owner arrived with a trailer and an explanation."),
  TEXT("Tumbleweed the size of a car blocked a lane of the highway. Removed by a passing trucker."),
  TEXT("Shots reported west of the highway. Deputy found a man shooting cans and asked him to quit at dark."),
  TEXT("Speeding citation on the highway near the diner, 78 in a 55."),
  TEXT("A woman reported her clothesline stolen. Found in the neighbor's yard after the wind."),
  TEXT("Juveniles on the roof of the elementary school. They said they were looking at the stars."),
  TEXT("A truck driver asleep in his cab at the diner was reported as a dead man. He was not."),
 };
 return Pot;
}

const TArray<S47LedgerText::FAdDef>& S47LedgerText::Ads()
{
 static const TArray<FAdDef> Pot={
  // With a coupon (S47Ledger::Coupons): the counter in Caliche, the motel, or the catalog.
  {TEXT("CALICHE FEED & SEED"),TEXT("Dry cells in four sizes. Fence pliers, stock salt, garden seed. Clip this: dry cells $3 each through Wednesday."),TEXT("cell3")},
  {TEXT("CALICHE FEED & SEED"),TEXT("Shielding foil and tape just in from Albuquerque. With this coupon, foil $2 a roll this week."),TEXT("foil2")},
  {TEXT("CALICHE FEED & SEED"),TEXT("Spare wheels mounted and balanced while you wait. $12 with this coupon through Wednesday."),TEXT("spare12")},
  {TEXT("CALICHE FEED & SEED"),TEXT("Control fuses in every size we could get. $4 apiece with this ad."),TEXT("fuse4")},
  {TEXT("CALICHE FEED & SEED"),TEXT("Drive relays for pumps and motors, tested on the bench. $21 with this coupon, this week only."),TEXT("relay21")},
  {TEXT("CALICHE SERVICE"),TEXT("Flats fixed $3. Oil and filter $11.95. Fuel cans filled and capped: a six-hour can $8 with this coupon."),TEXT("fuel8")},
  {TEXT("TURQUOISE TRADING POST"),TEXT("Postcards, rocks and minerals, jewelry, maps of the old mines. Film and flashbulbs: film $8 a roll with this ad."),TEXT("film8")},
  {TEXT("SIERRA MOTOR COURT"),TEXT("Clean rooms, quiet nights, color television, weekly rates. Bring this ad: $6 a night through Wednesday."),TEXT("motel6")},
  {TEXT("WESTERN SURPLUS & RADIO SUPPLY"),TEXT("Albuquerque. Tubes, relays, wire, test gear, field recorders. Why drive to town? Postage paid on any order mailed this week."),TEXT("post0")},
  {TEXT("WESTERN SURPLUS & RADIO SUPPLY"),TEXT("Spring clearance: reconditioned antenna drive motors, $22 by mail while they last."),TEXT("motor22")},
  {TEXT("WESTERN SURPLUS & RADIO SUPPLY"),TEXT("Used field recorders, as is, for parts or repair: $12 by mail this month."),TEXT("recorder12")},
  // Plain ones.
  {TEXT("MESA DINER"),TEXT("Open 6 a.m. to 10 p.m. Chicken fried steak Thursdays. Pie by the slice, and coffee you can stand a spoon in."),nullptr},
  {TEXT("MESA DINER"),TEXT("Mother's Day dinner Sunday, May 11: roast turkey with the trimmings, $5.95. No reservations needed, but call anyway."),nullptr},
  {TEXT("MESA DINER"),TEXT("Breakfast special before 9: two eggs, potatoes, toast and green chile, $1.95."),nullptr},
  {TEXT("SIERRA MOTOR COURT"),TEXT("Truckers welcome. Room for rigs on the north lot. The ice machine works."),nullptr},
  {TEXT("1420 AM / THE VOICE OF THE VALLEY"),TEXT("Farm and ranch report at 6:15 and 12:15. Swap Shop Saturdays 9 to 11: call in what you have and what you want."),nullptr},
  {TEXT("1420 AM"),TEXT("Saw something you can't explain? The late show takes your calls after eleven. You don't have to give your name."),nullptr},
  {TEXT("1420 AM"),TEXT("Wildcats baseball Saturday at 1, brought to you by the merchants of Caliche."),nullptr},
  {TEXT("CALICHE COIN LAUNDRY"),TEXT("Wash 50 cents. Dry a dime for ten minutes. The change machine is fixed."),nullptr},
  {TEXT("TURQUOISE TRADING POST"),TEXT("Rock shop open Sundays after church. Petrified wood, geodes, and arrowheads found right here in the county."),nullptr},
  {TEXT("CALICHE DRUG"),TEXT("Prescriptions, school supplies, greeting cards. The fountain is open till 5."),nullptr},
  {TEXT("RIO GRANDE LAND OFFICE"),TEXT("Ranchland: a section west of the highway with a windmill and a dirt tank. Owner will carry the note. Socorro."),nullptr},
  {TEXT("MONTANO WELL & PUMP"),TEXT("Wells drilled, windmills repaired, pumps pulled. Fifty years in the county."),nullptr},
  {TEXT("VALLEY VETERINARY"),TEXT("Large and small animals. Brucellosis testing and Coggins. House calls on the west side Thursdays."),nullptr},
  {TEXT("THE CURL UP & DYE"),TEXT("Permanents $18. Walk-ins welcome Tuesdays and Fridays."),nullptr},
  {TEXT("HIGHWAY AUTO SALES"),TEXT("Clean used pickups and sedans. Your job is your credit."),nullptr},
  {TEXT("PETTIT INSURANCE"),TEXT("Farm, ranch, home and truck. Hail season is coming."),nullptr},
  {TEXT("FIRST BAPTIST CHURCH"),TEXT("Revival May 18 through 21, 7 nightly. All welcome."),nullptr},
  {TEXT("CALICHE SERVICE"),TEXT("We check your water without being asked. Wrecker service, day or night."),nullptr},
  {TEXT("ORTEGA BROTHERS HAULING"),TEXT("Hay, gravel, livestock. No job too small, a few too big."),nullptr},
 };
 return Pot;
}

const TArray<S47LedgerText::FLetterDef>& S47LedgerText::Letters()
{
 static const TArray<FLetterDef> Pot={
  // Some point at a place out on the land (FS47Places): read, it goes on the map with a question mark.
  {TEXT("LIGHTS AGAIN"),TEXT("Editor: I have lived on the west side forty years and I know a plane when I see one. What sat over the windmill on the Baca lease the other night was not a plane. I don't need anybody to tell me what it was. I would like somebody to tell me what it wasn't. / Baca lease"),TEXT("windmill")},
  {TEXT("LOST DOG"),TEXT("Red heeler, answers to Biscuit, lost near the old mine on the west side. Green collar. Reward. Leave word at the Feed & Seed."),TEXT("mine")},
  {TEXT("LOST"),TEXT("Black Lab mix on three legs, named Tripod. Last seen after a jackrabbit toward the salt lick and trough south of the turnoff. He will come for bologna."),TEXT("trough")},
  {TEXT("FOUND"),TEXT("Binoculars in a leather case, left on the cairn on the rise west of the radio station. Describe to claim. Box 12, Caliche."),TEXT("cairn")},
  {TEXT("WANTED"),TEXT("Old weather balloons and their boxes, for a science project. We will come and get them. Call the high school office."),TEXT("balloon")},
  {TEXT("SEEN ON THE GRADE"),TEXT("Editor: Coming home late Saturday I saw a car stopped by the water tank on the old grade with its lights on and nobody in it. When I came back by twenty minutes later it was gone. If it was yours, you left your door open. / Night driver"),TEXT("tank")},
  {TEXT("LOST"),TEXT("Plain gold wedding band, somewhere between the line shack and the highway. It means more to me than to you. Reward."),TEXT("shack")},
  {TEXT("NOTICE"),TEXT("Whoever keeps driving stakes with orange flagging off the track to the old station: that is my lease. Come and talk to me first."),TEXT("stake")},
  {TEXT("THE COW BY THE FENCE"),TEXT("Editor: The cow found by the fence past the turnoff was not killed by any coyote I know of. Coyotes eat. / South of the turnoff"),TEXT("cow")},
  // The rest.
  {TEXT("THAT STATION"),TEXT("Editor: That radio station by the highway hums all night. You can hear it from my porch when the wind is right. Has anybody measured what those dishes put out? My hens have quit laying. / Name withheld"),nullptr},
  {TEXT("IN DEFENSE OF SARO"),TEXT("Editor: The dishes at SARO listen. They do not send. I worked out there two summers. They are quieter than your hens. / Former employee"),nullptr},
  {TEXT("RADIATION"),TEXT("Editor: My husband's hearing aid whistles every time we drive past the observatory. If it does that to a hearing aid, what does it do to the rest of us? / Mrs. A. P."),nullptr},
  {TEXT("THE TRUCK STOP"),TEXT("Editor: Twelve diesel pumps at the junction means twelve trucks idling all night under my window. I moved out here for the quiet. / East side"),nullptr},
  {TEXT("THE TRUCK STOP, TOO"),TEXT("Editor: My boy drives to Socorro to work. If the truck stop means a job in town, build it. / A mother"),nullptr},
  {TEXT("WATER"),TEXT("Editor: Before the water board hands out three more well permits, I would like them to come look at my windmill. It has been pumping air since March. / West side"),nullptr},
  {TEXT("QUIET NIGHTS"),TEXT("Editor: My dogs have quit barking at night. All three of them. They used to bark at everything. I would rather they barked. / River road"),nullptr},
  {TEXT("ANOTHER VIEW"),TEXT("Editor: Every spring somebody sees lights. It's ranchers burning ditches, it's trucks on the highway, it's the evening star. Go to bed. / Retired schoolteacher"),nullptr},
  {TEXT("FOR SALE"),TEXT("Camper shell for a long-bed pickup, one window cracked. $75 or trade for hay."),nullptr},
  {TEXT("FOR SALE"),TEXT("Saddle, 15-inch seat, good tree, needs a cinch. $120. Two bridles thrown in."),nullptr},
  {TEXT("FOR SALE"),TEXT("Hay in small bales, $3.25 at the stack. Bring your own help."),nullptr},
  {TEXT("FOR SALE"),TEXT("1974 three-quarter ton pickup, four-speed, new tires, runs. $1,100."),nullptr},
  {TEXT("WANTED"),TEXT("Ranch hand who knows cattle and fence and owns boots. Room and board. Ask at the Mesa Diner for Mr. Ruiz."),nullptr},
  {TEXT("WANTED"),TEXT("Used generator, five kilowatts or so, running or not. Cash. Call evenings."),nullptr},
  {TEXT("GARAGE SALE"),TEXT("Saturday 8 to noon, third house past the church on the east side. Baby things, tools, and a freezer that works."),nullptr},
  {TEXT("FREE"),TEXT("Barn kittens, will mouse. Free to good homes or any homes."),nullptr},
  {TEXT("FOR RENT"),TEXT("Two-bedroom house on the east side, $150 a month, water paid. Ask at the gas station."),nullptr},
  {TEXT("PERSONAL"),TEXT("To the lady in the green coat at the laundry Tuesday: you left your quarters in the dryer. They are at the counter."),nullptr},
  {TEXT("SERVICES"),TEXT("Saddles repaired, tack made to order. Leatherwork at the Feed & Seed Saturdays."),nullptr},
  {TEXT("A REQUEST"),TEXT("Editor: Would whoever drives the highway at three in the morning with no headlights please stop. You will kill somebody. / A rancher"),nullptr},
 };
 return Pot;
}

const TArray<const TCHAR*>& S47LedgerText::Films()
{
 static const TArray<const TCHAR*> Pot={
  TEXT("RODEO WIDOWS (PG-13)"),TEXT("THE COUNTERFEIT COWBOY (PG)"),TEXT("SUMMER OF THE BLUE CAR (PG)"),TEXT("NINJA ACCOUNTANT (PG-13)"),
  TEXT("THE HAILSTORM KID (G)"),TEXT("HOT ROD ANGELS (PG-13)"),TEXT("SECRET OF THE BLACK MESA (PG)"),TEXT("BREAKDANCE BEACH (PG)"),
  TEXT("THE WELL DIGGER'S SON (PG)"),TEXT("LASER SQUADRON (PG)"),TEXT("GRANDMA'S RANCH (G)"),TEXT("VALLEY HIGH REUNION (PG-13)"),
  TEXT("CHOPPER PATROL III (R)"),TEXT("A KISS FOR KATIE (PG)"),
 };
 return Pot;
}

const TArray<S47LedgerText::FFrontDef>& S47LedgerText::Fronts()
{
 static const TArray<FFrontDef> Pot={
  {ETopic::Missing,TEXT("PICKUP FOUND, DRIVER STILL MISSING"),TEXT("A blue pickup belonging to a ranch hand reported missing turned up out by {place} {night}, the keys still in it. Deputies walked the ground around it and found no sign of him. His family asks anyone who saw him to call the substation.")},
  {ETopic::Called,TEXT("NIGHT OPERATOR REPORTS MISSING MAN'S TRUCK"),TEXT("The pickup of a ranch hand missing since last week was called in {night} by the new night operator at the SARO observatory, who came across it out by {place}. Deputies are searching the area. The family's reward went to the station.")},
  {ETopic::Carcass,TEXT("COW FOUND DEAD, UNTOUCHED"),TEXT("A cow was found dead out by {place} {night}, and nothing had been at her: no coyotes, no birds. The brand inspector says the cause will be hard to find. Ranchers are asked to report any more.")},
  {ETopic::Cattle,TEXT("CATTLE ACTING STRANGE, RANCHERS SAY"),TEXT("Stock out by {place} were seen standing close together {night}, every head turned the same way and not a sound out of them. The extension agent suggests a coyote or a change in the weather. A rancher who asked that his name not be printed says cattle do not stand like that for a coyote.")},
  {ETopic::Cattle,TEXT("A HERD THAT WOULD NOT MOVE"),TEXT("A rancher checking water out by {place} {night} says his cattle stood in a tight ring and would not be moved, not for the truck, not for the dog. By morning they were grazing as if nothing had happened.")},
  {ETopic::Landing,TEXT("BURNT PATCH PUZZLES RANCHER"),TEXT("A ring of scorched grass turned up on open ground out by {place} after {night}. The extension agent says lightning can do odd things to dry ground in the spring. The man who found it says lightning does not do circles.")},
  {ETopic::Landing,TEXT("MARKS ON THE GROUND"),TEXT("Somebody, or something, left pressed marks in the dirt out by {place} {night}. The sheriff's office went and looked and took no report. \"It's no crime to leave marks,\" a deputy said.")},
  {ETopic::Lights,TEXT("LIGHTS REPORTED OVER THE VALLEY"),TEXT("Callers told the sheriff's substation they saw lights low over {place} {night}. A deputy drove out and found nothing he could write down. The airfield in Socorro says it had nothing up. \"They didn't blink like a plane,\" one caller said. \"They just sat there.\"")},
  {ETopic::Lights,TEXT("MORE LIGHTS, NO ANSWERS"),TEXT("Again this spring people are talking about lights at night, this time over {place}, {night}. The sheriff's office has no explanation and says it is not looking for one. Anyone who saw something is asked to call the substation, not the newspaper.")},
  {ETopic::Lights,TEXT("WHAT THE DRIVERS SAW"),TEXT("Two drivers on the highway say they pulled over {night} to watch lights near {place}. A third, riding with one of them, says he saw nothing at all. The Ledger reports all three.")},
  {ETopic::Rumor,TEXT("TALK AT THE COUNTER"),TEXT("At the gas station and the diner this week the talk was of lights out by {place}, seen by a friend of a friend. Nobody the Ledger reached saw them firsthand. Those who say they did may call the substation.")},
  {ETopic::Rumor,TEXT("A FRIEND OF A FRIEND"),TEXT("A reader says her cousin saw lights come down slowly out by {place} and go out on the ground. The cousin would not talk to the Ledger. The reader would like somebody to go and look.")},
  {ETopic::Stalled,TEXT("CAR DIES, THEN STARTS BY ITSELF"),TEXT("A woman driving the highway {night} says her engine, lights and radio all quit at once and came back by themselves a minute later. A mechanic in town went over the car and found nothing wrong. \"They do that sometimes,\" he said. \"Not usually all at once.\"")},
  {ETopic::Outage,TEXT("LIGHTS OUT ON THE EAST SIDE"),TEXT("Power went out on the east side of town for about ten minutes {night}. The electric co-op found no fault on the line and no reason for it. Members who lost a freezer are asked to call the co-op.")},
  {ETopic::EmptyRig,TEXT("RIG FOUND RUNNING, NOBODY IN IT"),TEXT("A truck was found on a ranch road out by {place} {night} with the engine running and the cab door open. The driver has not been found. The trucking company in El Paso says he is a careful man.")},
  {ETopic::Camper,TEXT("CAMPER LEFT ON OPEN LAND"),TEXT("A camper with every light on and nobody aboard was reported out by {place} {night}. Deputies say the owner, from out of state, has not been reached. Visitors are reminded that the land out there belongs to somebody.")},
  {ETopic::Walker,TEXT("WOMAN SEEN WALKING THE HIGHWAY"),TEXT("Two motorists reported a woman walking the highway shoulder after dark {night}, a long way from anywhere. A deputy drove the stretch twice and found nobody. Anyone missing a relative is asked to call the substation.")},
 };
 return Pot;
}

const TArray<S47LedgerText::FFrontDef>& S47LedgerText::Quiet()
{
 static const TArray<FFrontDef> Pot={
  {ETopic::Count,TEXT("SEARCH FOR MISSING PAIR SCALED BACK"),TEXT("The sheriff's office has scaled back its search for the SARO observatory's night operator, Reyes, and for Mara Ortiz, a district maintenance driver who was out on a call the same night. Both have been missing since April 14. The district's white wagon was found on the flat west of the old grade and is being kept there under a hold. Anyone with information is asked to call the substation.")},
  {ETopic::Count,TEXT("COUNTY TO HEAR FUTURE OF RADIO STATION"),TEXT("The county commission will hear the district's report on the SARO radio observatory at its last meeting in May. A new night operator has started at the station. \"We'll see what the numbers say,\" said a district spokesman, who would not say which numbers.")},
  {ETopic::Count,TEXT("TRUCK STOP PLAN DRAWS A CROWD"),TEXT("Forty people came to the courthouse annex Tuesday to hear plans for a truck stop at the highway junction south of town: twelve diesel pumps, a cafe and a scale. The planning board votes in June. About half the room clapped.")},
  {ETopic::Count,TEXT("WATER BOARD PUTS OFF WELL PERMITS"),TEXT("Three new well permits east of the highway will wait for the aquifer study, the water board decided Monday. Ranchers on the west side say their windmills have been pumping air since March.")},
  {ETopic::Count,TEXT("WIND TAKES A ROOF AND A BILLBOARD"),TEXT("Gusts near sixty miles an hour Saturday took the roof off a hay shed, laid a billboard flat and rolled an empty stock trailer into the bar ditch on the river road. Nobody was hurt.")},
  {ETopic::Count,TEXT("MEMORIAL DAY RODEO GROWS"),TEXT("Entries for the Memorial Day rodeo are up by a third, the arena manager says, with barrel racers coming from three counties. The chutes have been rebuilt and the bleachers painted.")},
  {ETopic::Count,TEXT("SCHOOL BOARD PICKS A ROOFER"),TEXT("The elementary school gets a new roof this summer for $8,400, the school board decided after a short vote and a long discussion of the old roof.")},
  {ETopic::Count,TEXT("DUST CLOSES HIGHWAY FOR AN HOUR"),TEXT("Blowing dust cut the view to a few yards on the highway south of the diner one afternoon this week, and the state police held traffic for about an hour. Two cars ended up in the ditch. Nobody was hurt.")},
  {ETopic::Count,TEXT("NINETEEN TO GRADUATE"),TEXT("The class of '86 graduates May 23 in the high school gym. Valedictorian Delia Montoya will speak, and says it will be short.")},
  {ETopic::Count,TEXT("LATE FROST HITS ORCHARDS"),TEXT("A hard frost Sunday night took most of the apricots along the river for the third year running. Growers say the apples may yet come through.")},
 };
 return Pot;
}

const S47LedgerText::FLetterDef& S47LedgerText::Thanks()
{
 static const FLetterDef Line={TEXT("THANK YOU"),TEXT("Editor: Thanks to the night operator from the radio station who pulled our rig out of the sand last week and would not take more than gas money. There are still people out here who stop. / A trucker from Amarillo"),nullptr};
 return Line;
}

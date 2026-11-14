#include "S47Ledger.h"

// Everything the Ledger can print, in American English of 1986. Every person, business and film here is made up. Freeform's line
// holds (Tom, 7 November): the outside of what happened and nothing from Story's far side; S47Ledger::SelfTest refuses the words.
// Nothing here describes weather the sky does not show (no dust storms, frost or rain: S47Weather), and nothing promises a thing
// the player could go and find missing (a flagman on the highway, a second night patrol). One news item lives in one place: the
// county's front-page stories are not repeated in the briefs, and items that belong to one story share a thread.
namespace S47LedgerText{using S47Ledger::ETopic;}

bool S47LedgerText::InWindow(int32 First,int32 Last,const FDateTime& Date)
{
 const int32 On=Date.GetMonth()*100+Date.GetDay();
 return (First<=0||On>=First)&&(Last<=0||On<=Last);
}

const TArray<S47LedgerText::FItemDef>& S47LedgerText::Locals()
{
 static const TArray<FItemDef> Pot={
  {TEXT("The county commission voted two to one Monday to grade the Baca lease road before the summer rains. Commissioner Pruitt voted no, saying the rains never come anyway."),nullptr,0,0},
  {TEXT("The water board has put off three new well permits east of the highway until its June meeting. Chairman Ignacio Sedillo said the aquifer study is not finished."),TEXT("water"),0,531},
  {TEXT("Irrigators on the Salado ditch are reminded that the ditch boss sets the turns this year, not the first one out with a shovel."),nullptr,0,0},
  {TEXT("Feeder calves brought $62 to $68 a hundredweight at the sale barn last week, steers a little more. Hay is up five dollars a ton since March."),TEXT("sale"),0,0},
  {TEXT("The high school sent four runners to the district track meet in Socorro. Junior Teresa Chavez placed second in the mile."),nullptr,0,531},
  {TEXT("Spring football practice opens Monday at the high school field. Coach Harlan Mims has eleven boys and would like a few more."),nullptr,0,516},
  {TEXT("The Caliche Wildcats lost to Magdalena 7 to 5 in baseball Saturday after leading through six innings."),nullptr,0,531},
  {TEXT("The high school rodeo club holds a jackpot roping at the county arena Saturday. Entry is $10; the concession stand will be open."),TEXT("rodeo"),0,524},
  {TEXT("Entries are open for the Memorial Day rodeo. Barrel racers sign up at the feed store. Bull riders, the arena manager says, sign up wherever they like."),TEXT("rodeo"),0,518},
  {TEXT("Volunteers are wanted Saturday to paint the bleachers at the county arena before the rodeo. Bring a brush; the paint is donated."),TEXT("rodeo"),0,512},
  {TEXT("The Altar Society's spring bazaar is Saturday from 9 to 3 in the San Isidro parish hall: quilts, enchiladas and the cake walk. Proceeds go to the church roof fund."),nullptr,0,531},
  {TEXT("The Methodist women serve a chicken supper at the church Friday at 6. Adults $3.50, children $1.50."),nullptr,0,0},
  {TEXT("The volunteer fire department holds its pancake breakfast at the station Sunday. Firemen will test any smoke detector brought along."),nullptr,0,0},
  {TEXT("A public hearing on the proposed truck stop at the highway junction south of town is set for May 20 at the community center. The plans call for twelve diesel pumps and a cafe."),TEXT("truckstop"),0,519},
  {TEXT("Owners along the highway say a truck stop would bring jobs and noise in about equal measure. The planning board has fourteen letters on file, nine against."),TEXT("truckstop"),0,519},
  {TEXT("Engineers for the proposed truck stop measured the highway junction Friday. A spokesman said nothing has been decided."),TEXT("truckstop"),0,519},
  {TEXT("The state highway department will patch and seal the road to Magdalena through the end of the month. Expect a flagman and a wait."),nullptr,0,531},
  {TEXT("The county clerk reminds voters that registration for the June primary closes at the end of the month."),nullptr,0,527},
  {TEXT("The library is closed Tuesday while new shelves go in. Overdue books may be left in the box by the door, and no fines will be charged this week."),TEXT("library"),0,507},
  {TEXT("The 4-H club weighs lambs at the fairgrounds Saturday morning. Members should bring their record books."),nullptr,0,0},
  {TEXT("Extension agent Verna Kittredge will speak on windbreaks and soil blowing at the grange hall Thursday at 7. Coffee provided."),nullptr,0,0},
  {TEXT("The veterans' post will place flags at the cemetery for Memorial Day and asks families to leave the gravesides clear by Saturday."),TEXT("memorial"),0,524},
  {TEXT("The cemetery association is collecting its $5 dues for upkeep. The mowing is done by volunteers, and the association says it shows."),nullptr,0,0},
  {TEXT("Graduation for the class of '86 is May 23 in the high school gym. There are nineteen seniors this year."),TEXT("graduation"),0,522},
  {TEXT("The high school band plays its spring concert Tuesday at 7 in the gym. Admission is free; a hat will be passed for new uniforms."),nullptr,0,522},
  {TEXT("The electric co-op will replace poles along the river road next week. Members on that line may be without power for two hours Wednesday morning."),nullptr,0,0},
  {TEXT("Births: a son, Joaquin, 7 lb. 2 oz., to Ramon and Elsie Gallegos of Caliche; a daughter, Amber Lynn, 6 lb. 9 oz., to Dwight and Cora Pettit."),nullptr,0,0},
  {TEXT("Births: twin daughters to Lorenzo and Paula Archuleta, at the hospital in Socorro. Mother and girls are doing well. The father is resting."),nullptr,0,0},
  {TEXT("Died: Wendell Ashby, 81, who ran sheep on the west side for fifty years and the grange for thirty. Services Saturday at 10 at the Methodist church."),nullptr,0,0},
  {TEXT("Died: Ramona Quintana, 74, retired teacher, who taught most of Caliche to read. Rosary Friday at 7, Mass Saturday at 9."),nullptr,0,0},
  {TEXT("Died: Floyd Hensley, 68, rancher, at home on the east side. He leaves his wife, Norma, and two sons. Services Tuesday at 2 at the Methodist church."),nullptr,0,0},
  {TEXT("Died: Esperanza Lucero, 92, the oldest member of San Isidro parish, who remembered when the highway was a wagon road. Rosary Thursday at 7, Mass Friday at 10."),nullptr,0,0},
  {TEXT("The Feed & Seed will stay open until 7 on Fridays through the summer, the owners say, for ranchers who cannot get in before dark."),nullptr,0,0},
  {TEXT("The post office is closed Memorial Day, and rural carriers will not run that day."),TEXT("memorial"),0,525},
  {TEXT("Rural patrons are reminded to keep the ground in front of their mailboxes clear, so the carrier can reach without leaving the vehicle."),nullptr,0,0},
  {TEXT("Burning is banned in the county until further notice because of the dry spring. The transfer station still takes trash on Saturdays."),nullptr,0,0},
  {TEXT("The transfer station is open Saturdays from 8 to 4. Tires $1 each, appliances $5. The attendant asks that nobody leave a couch at the gate again."),nullptr,0,0},
  {TEXT("The garden club meets Tuesday at the home of Iva Dean. The program is growing tomatoes in caliche, which members insist can be done."),nullptr,0,0},
  {TEXT("The garden club's plant sale is Saturday morning on the church steps. Tomato and chile starts, a dime apiece."),nullptr,0,0},
  {TEXT("The senior center serves lunch weekdays at 11:30 for a dollar. This week: green chile stew, meat loaf, and fish on Friday."),nullptr,0,0},
  {TEXT("Bingo returns to the San Isidro parish hall on Friday nights at 7. The jackpot is $100."),nullptr,0,0},
  {TEXT("San Isidro parish holds First Communion for eleven children Sunday at the ten o'clock Mass."),nullptr,0,0},
  {TEXT("The fair board needs a superintendent for the poultry barn. The last one retired after the business with the rooster."),nullptr,0,0},
  {TEXT("The Rotary Club heard a talk Thursday on the history of the old mine on the west side. It never paid out, the speaker said, which surprised nobody at the table."),nullptr,0,0},
  {TEXT("FROM OUR FILES, 1934: Old-timers on Main Street still say the Trujillo cave draws a long breath in and lets it out with the hours of the day, and that around two in the morning it holds that breath. The family says it is the weather."),nullptr,0,0}, // N1110 (pace)
  {TEXT("A dance at the community center Saturday benefits the ambulance fund. Music by the Rio Salado Ramblers. Admission $3."),nullptr,0,0},
  {TEXT("The ambulance crew is two short and asks for volunteers who can take calls at night. Training is free."),nullptr,0,0},
  {TEXT("The cattlemen's association meets at the Mesa Diner Tuesday at noon to talk about the brand inspection fee."),nullptr,0,0},
  {TEXT("Brand inspector Leon Pacheco reminds ranchers that stock moved across the county line needs papers, even when it is only going to the neighbor's."),nullptr,0,0},
  {TEXT("The extension office has soil test kits for $4. Results come back from the university in about three weeks."),nullptr,0,0},
  {TEXT("The state rest area on the road to Magdalena closes for repairs for two weeks. Travelers should plan ahead, the state says."),nullptr,0,0},
  {TEXT("Caliche Drug will be closed Monday for inventory."),nullptr,0,0},
  {TEXT("The Plaza will show a free matinee for children on the last day of school. Popcorn is a dime."),nullptr,0,522},
  {TEXT("Sixth graders planted a row of elms along the playground fence and have promised to water them all summer."),nullptr,0,0},
  {TEXT("The county will spray for mosquitoes along the river after the first warm week. Beekeepers should call the extension office."),nullptr,0,0},
  {TEXT("A coyote has been getting chickens along the river road. The county trapper says he will be out when he gets out."),nullptr,0,0},
  {TEXT("Lost and found at the sheriff's substation: two sets of keys, a child's boot and a pair of reading glasses. Ask at the window."),nullptr,0,0},
  {TEXT("The Methodist youth wash cars Saturday in front of the Feed & Seed. Donations go to summer camp."),nullptr,0,0},
  {TEXT("The county road crew graded the track to the old survey station up the hill. Past the second cattle guard it is still four-wheel drive."),nullptr,0,0},
  {TEXT("The school district counts 214 children this spring, six fewer than last year."),nullptr,0,0},
  {TEXT("The adult softball league starts in June. Teams of ten sign up at the gas station."),nullptr,0,531},
  {TEXT("The county agent warns that grasshoppers are hatching early on the east side. Bait is available at cost."),nullptr,0,0},
  {TEXT("Members of the Caliche Homemakers will judge pies at the June fair and are taking applications from people who like pie."),nullptr,0,0},
  {TEXT("The Little League has two teams this year and one set of catcher's gear. Parents with a spare mask are asked to call the coach."),nullptr,0,0},
  {TEXT("The county fair board has set the dates for August and is looking for a new announcer. The old one says his voice has gone."),nullptr,0,0},
  // REAL-1986-20261108 (14 Nov): the world outside the county, in a wire brief's words: facts only, no names, no quotes, nothing sold.
  // Each is printed on or after its real date (the paper is Thursday, May 1 and on); the raid, the shuttle and the price are already past.
  {TEXT("WIRE: The Soviet Union said April 28 that an accident at a nuclear power plant in the Ukraine damaged a reactor. Sweden measured higher than normal radiation in the air first."),nullptr,428,531},
  {TEXT("Halley's comet is fading as it pulls away from the sun, astronomers say. It can still be found low in the southern sky on a dark night with binoculars."),nullptr,0,531},
  {TEXT("WIRE: Security at airports and U.S. embassies abroad remains tight since the air raid on Libya in mid-April, officials say."),nullptr,0,531},
  {TEXT("WIRE: The space shuttle fleet remains grounded since the Challenger accident in January. A review panel is still at work and no launch date has been set."),nullptr,0,0},
  {TEXT("WIRE: Organizers say people will join hands in a line across the country on Sunday, May 25, to raise money for the hungry and homeless. The line is to cross New Mexico."),nullptr,0,525},
  {TEXT("WIRE: Regular gasoline averaged about 90 cents a gallon in a national survey, down sharply from a year ago, as oil prices fall."),nullptr,0,0},
  // N1110 (Caliche history, approved by Tom 10 November; Docs/Lore/CALICHE_HISTORIE.md section 4): the paper's column "From Our Files",
  // one notice at a time out of the same pot as the rest. The years are the paper's own; nothing here is a spoiler.
  {TEXT("FROM OUR FILES, 1926: Lights were seen again over the salt flat Tuesday night, four in a row. Mr. Baca says they were not his. The editor has no further theory and would welcome one."),nullptr,0,0},
  {TEXT("FROM OUR FILES, 1931: The Trujillo cave under Main Street opens to visitors Sunday. Lanterns provided. Ladies are advised to bring a wrap, as the cave keeps its own weather."),nullptr,0,0},
  {TEXT("FROM OUR FILES, 1946: The fire siren will be tested the first Monday of each month. The long tone means fire. The short tones mean go inside and stay there. Both mean do as you are told."),nullptr,0,0},
  {TEXT("FROM OUR FILES, 1956: The county and the district have agreed to a listening station on the flat north of town. Postmaster Quintana says it will bring jobs and ask nothing of us but quiet."),nullptr,0,0},
  {TEXT("FROM OUR FILES, 1953: Uranium hunters left the Esperanza mine on the west side Saturday with nothing to show. They pulled the boards off the adit. Somebody has stacked them neatly since."),nullptr,0,0},
  {TEXT("FROM OUR FILES, 1968: The new interstate is open by way of Socorro. The Mesa Diner reports twelve fewer cars at lunch and says it will keep the pie the same size."),nullptr,0,0},
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
  TEXT("Power pole hit by a stock trailer on the river road. Co-op crew on scene by midnight."),
  TEXT("Guest at the motel playing a radio too loud. Guest turned it down."),
  TEXT("Deputy stood by while a family moved out of a house on the east side."),
  TEXT("Loose bull in the schoolyard before school. Owner arrived with a trailer and an explanation."),
  TEXT("Tumbleweed the size of a car blocked a lane of the highway. Removed by a passing trucker."),
  TEXT("Shots reported west of the highway. Deputy found a man shooting cans and asked him to quit at dark."),
  TEXT("Speeding citation on the highway near the diner, 78 in a 55."),
  TEXT("A woman reported her clothesline stolen. Found in the neighbor's yard."),
  TEXT("Juveniles on the roof of the elementary school. They said they were looking at the comet."),
  TEXT("A truck driver asleep in his cab at the diner was reported as a dead man. He was not."),
 };
 return Pot;
}

const TArray<S47LedgerText::FAdDef>& S47LedgerText::Ads()
{
 static const TArray<FAdDef> Pot={
  // With a coupon (S47Ledger::Coupons): the counter in Caliche, the motel, or the catalog.
  {TEXT("CALICHE FEED & SEED"),TEXT("Dry cells in four sizes. Fence pliers, stock salt, garden seed. Clip this: dry cells $3 each through Wednesday."),TEXT("cell3"),0,0},
  {TEXT("CALICHE FEED & SEED"),TEXT("Shielding foil and tape just in from Albuquerque. With this coupon, foil $2 a roll this week."),TEXT("foil2"),0,0},
  {TEXT("CALICHE FEED & SEED"),TEXT("Spare wheels mounted and balanced while you wait. $12 with this coupon through Wednesday."),TEXT("spare12"),0,0},
  {TEXT("CALICHE FEED & SEED"),TEXT("Control fuses in every size we could get. $4 apiece with this ad."),TEXT("fuse4"),0,0},
  {TEXT("CALICHE FEED & SEED"),TEXT("Drive relays for pumps and motors, tested on the bench. $21 with this coupon, this week only."),TEXT("relay21"),0,0},
  {TEXT("CALICHE SERVICE"),TEXT("Flats fixed $3. Oil and filter $11.95. Fuel cans filled and capped: a six-hour can $8 with this coupon."),TEXT("fuel8"),0,0},
  {TEXT("TURQUOISE TRADING POST"),TEXT("Postcards, rocks and minerals, jewelry, maps of the old mines. Film and flashbulbs: film $8 a roll with this ad."),TEXT("film8"),0,0},
  {TEXT("SIERRA MOTOR COURT"),TEXT("Clean rooms, quiet nights, color television, weekly rates. Bring this ad: $6 a night through Wednesday."),TEXT("motel6"),0,0},
  {TEXT("WESTERN SURPLUS & RADIO SUPPLY"),TEXT("Albuquerque. Tubes, relays, wire, test gear, field recorders. Why drive to town? Postage paid on any order mailed this week."),TEXT("post0"),0,0},
  {TEXT("WESTERN SURPLUS & RADIO SUPPLY"),TEXT("Spring clearance: reconditioned antenna drive motors, $22 by mail while they last."),TEXT("motor22"),0,0},
  {TEXT("WESTERN SURPLUS & RADIO SUPPLY"),TEXT("Used field recorders, as is, for parts or repair: $12 by mail this month."),TEXT("recorder12"),0,0},
  // Plain ones.
  {TEXT("MESA DINER"),TEXT("Open 6 a.m. to 10 p.m. Chicken fried steak Thursdays. Pie by the slice, and coffee you can stand a spoon in."),nullptr,0,0},
  {TEXT("MESA DINER"),TEXT("Mother's Day dinner Sunday, May 11: roast turkey with the trimmings, $5.95. No reservations needed, but call anyway."),nullptr,0,510},
  {TEXT("MESA DINER"),TEXT("Breakfast special before 9: two eggs, potatoes, toast and green chile, $1.95."),nullptr,0,0},
  {TEXT("SIERRA MOTOR COURT"),TEXT("Truckers welcome. Room for rigs on the north lot. The ice machine works."),nullptr,0,0},
  {TEXT("1420 AM / THE VOICE OF THE VALLEY"),TEXT("Farm and ranch report at 6:15 and 12:15. Swap Shop Saturdays 9 to 11: call in what you have and what you want."),nullptr,0,0},
  {TEXT("1420 AM"),TEXT("Saw something you can't explain? The late show takes your calls after eleven. You don't have to give your name."),nullptr,0,0},
  {TEXT("1420 AM"),TEXT("Wildcats baseball Saturday at 1, brought to you by the merchants of Caliche."),nullptr,0,531},
  {TEXT("CALICHE COIN LAUNDRY"),TEXT("Wash 50 cents. Dry a dime for ten minutes. The change machine is fixed."),nullptr,0,0},
  {TEXT("TURQUOISE TRADING POST"),TEXT("Rock shop open Sundays after church. Petrified wood, geodes, and arrowheads found right here in the county."),nullptr,0,0},
  {TEXT("CALICHE DRUG"),TEXT("Prescriptions, school supplies, greeting cards. The fountain is open till 5."),nullptr,0,0},
  {TEXT("RIO GRANDE LAND OFFICE"),TEXT("Ranchland: a section west of the highway with a windmill and a dirt tank. Owner will carry the note. Socorro."),nullptr,0,0},
  {TEXT("MONTANO WELL & PUMP"),TEXT("Wells drilled, windmills repaired, pumps pulled. Fifty years in the county."),nullptr,0,0},
  {TEXT("VALLEY VETERINARY"),TEXT("Large and small animals. Brucellosis testing and Coggins. House calls on the west side Thursdays."),nullptr,0,0},
  {TEXT("THE CURL UP & DYE"),TEXT("Permanents $18. Walk-ins welcome Tuesdays and Fridays."),nullptr,0,0},
  {TEXT("HIGHWAY AUTO SALES"),TEXT("Clean used pickups and sedans. Your job is your credit."),nullptr,0,0},
  {TEXT("PETTIT INSURANCE"),TEXT("Farm, ranch, home and truck. Hail season is coming."),nullptr,0,0},
  {TEXT("FIRST BAPTIST CHURCH"),TEXT("Revival May 18 through 21, 7 nightly. All welcome."),nullptr,0,517},
  {TEXT("CALICHE SERVICE"),TEXT("We check your water without being asked. Wrecker service, day or night."),nullptr,0,0},
  {TEXT("ORTEGA BROTHERS HAULING"),TEXT("Hay, gravel, livestock. No job too small, a few too big."),nullptr,0,0},
 };
 return Pot;
}

const TArray<S47LedgerText::FLetterDef>& S47LedgerText::Letters()
{
 static const TArray<FLetterDef> Pot={
  // Some point at a place out on the land (FS47Places): read, it goes on the map with a question mark.
  {TEXT("LIGHTS AGAIN"),TEXT("Editor: I have lived on the west side forty years and I know a plane when I see one. What sat over the windmill on the Baca lease the other night was not a plane. I don't need anybody to tell me what it was. I would like somebody to tell me what it wasn't. / Baca lease"),TEXT("windmill"),nullptr,0,0},
  {TEXT("LOST DOG"),TEXT("Red heeler, answers to Biscuit, lost near the old mine on the west side. Green collar. Reward. Leave word at the Feed & Seed."),TEXT("mine"),nullptr,0,0},
  {TEXT("LOST"),TEXT("Black Lab mix on three legs, named Tripod. Last seen after a jackrabbit toward the salt lick and trough south of the turnoff. He will come for bologna."),TEXT("trough"),nullptr,0,0},
  {TEXT("FOUND"),TEXT("Binoculars in a leather case, left on the cairn on the rise west of the radio station. Describe to claim. Box 12, Caliche."),TEXT("cairn"),nullptr,0,0},
  {TEXT("WANTED"),TEXT("Old weather balloons and their boxes, for a science project. We will come and get them. Call the high school office."),TEXT("balloon"),nullptr,0,0},
  {TEXT("SEEN ON THE GRADE"),TEXT("Editor: Coming home late Saturday I saw a flashlight going along the fence by the water tank on the old grade, slow, like somebody looking for something. Whoever you are, the cattle guard there is broken and you will lose a wheel in it. / Night driver"),TEXT("tank"),nullptr,0,0},
  {TEXT("LOST"),TEXT("Plain gold wedding band, somewhere between the line shack and the highway. It means more to me than to you. Reward."),TEXT("shack"),nullptr,0,0},
  {TEXT("NOTICE"),TEXT("Whoever keeps driving stakes with orange flagging off the track to the old station: that is my lease. Come and talk to me first."),TEXT("stake"),nullptr,0,0},
  {TEXT("THE COW BY THE FENCE"),TEXT("Editor: The cow found by the fence past the turnoff was not killed by any coyote I know of. Coyotes eat. / South of the turnoff"),TEXT("cow"),nullptr,0,0},
  // The rest.
  {TEXT("THAT STATION"),TEXT("Editor: That radio station by the highway hums all night. You can hear it from my porch when the wind is right. Has anybody measured what those dishes put out? My hens have quit laying. / Name withheld"),nullptr,TEXT("saro"),0,0},
  {TEXT("IN DEFENSE OF SARO"),TEXT("Editor: The dishes at SARO listen. They do not send. I worked out there two summers. They are quieter than your hens. / Former employee"),nullptr,TEXT("saro"),0,0},
  {TEXT("RADIATION"),TEXT("Editor: My husband's hearing aid whistles every time we drive past the observatory. If it does that to a hearing aid, what does it do to the rest of us? / Mrs. A. P."),nullptr,TEXT("saro"),0,0},
  {TEXT("THE TRUCK STOP"),TEXT("Editor: Twelve diesel pumps at the junction means twelve trucks idling all night under my window. I moved out here for the quiet. / East side"),nullptr,TEXT("truckstop"),0,531},
  {TEXT("THE TRUCK STOP, TOO"),TEXT("Editor: My boy drives to Socorro to work. If the truck stop means a job in town, build it. / A mother"),nullptr,TEXT("truckstop"),0,531},
  {TEXT("WATER"),TEXT("Editor: Before the water board hands out three more well permits, I would like them to come look at my windmill. It has been pumping air since March. / West side"),nullptr,TEXT("water"),0,531},
  {TEXT("QUIET NIGHTS"),TEXT("Editor: My dogs have quit barking at night. All three of them. They used to bark at everything. I would rather they barked. / River road"),nullptr,nullptr,0,0},
  {TEXT("ANOTHER VIEW"),TEXT("Editor: Every spring somebody sees lights. It's ranchers burning ditches, it's trucks on the highway, it's the evening star. Go to bed. / Retired schoolteacher"),nullptr,nullptr,0,0},
  {TEXT("FOR SALE"),TEXT("Camper shell for a long-bed pickup, one window cracked. $75 or trade for hay."),nullptr,nullptr,0,0},
  {TEXT("FOR SALE"),TEXT("Saddle, 15-inch seat, good tree, needs a cinch. $120. Two bridles thrown in."),nullptr,nullptr,0,0},
  {TEXT("FOR SALE"),TEXT("Hay in small bales, $3.25 at the stack. Bring your own help."),nullptr,nullptr,0,0},
  {TEXT("FOR SALE"),TEXT("1974 three-quarter ton pickup, four-speed, new tires, runs. $1,100."),nullptr,nullptr,0,0},
  {TEXT("WANTED"),TEXT("Ranch hand who knows cattle and fence and owns boots. Room and board. Ask at the Mesa Diner for Mr. Ruiz."),nullptr,nullptr,0,0},
  {TEXT("WANTED"),TEXT("Used generator, five kilowatts or so, running or not. Cash. Call evenings."),nullptr,nullptr,0,0},
  {TEXT("GARAGE SALE"),TEXT("Saturday 8 to noon, third house past the church on the east side. Baby things, tools, and a freezer that works."),nullptr,nullptr,0,0},
  {TEXT("FREE"),TEXT("Barn kittens, will mouse. Free to good homes or any homes."),nullptr,nullptr,0,0},
  {TEXT("FOR RENT"),TEXT("Two-bedroom house on the east side, $150 a month, water paid. Ask at the gas station."),nullptr,nullptr,0,0},
  {TEXT("PERSONAL"),TEXT("To the lady in the green coat at the laundry Tuesday: you left your quarters in the dryer. They are at the counter."),nullptr,nullptr,0,0},
  {TEXT("SERVICES"),TEXT("Saddles repaired, tack made to order. Leatherwork at the Feed & Seed Saturdays."),nullptr,nullptr,0,0},
  {TEXT("A REQUEST"),TEXT("Editor: Would whoever drives the highway at three in the morning with no headlights please stop. You will kill somebody. / A rancher"),nullptr,nullptr,0,0},
 };
 return Pot;
}

const TArray<const TCHAR*>& S47LedgerText::Films()
{
 // Small-town bills of the spring of 1986: a western, a family picture, a teen comedy, an action picture, all made up.
 static const TArray<const TCHAR*> Pot={
  TEXT("RODEO WIDOWS (PG-13)"),TEXT("THE COUNTERFEIT COWBOY (PG)"),TEXT("SUMMER OF THE BLUE CAR (PG)"),TEXT("THE SUMMER WE MOVED (PG)"),
  TEXT("THE HAILSTORM KID (G)"),TEXT("HOT ROD ANGELS (PG-13)"),TEXT("SECRET OF THE BLACK MESA (PG)"),TEXT("FOURTH AND LONG (PG-13)"),
  TEXT("THE WELL DIGGER'S SON (PG)"),TEXT("STEEL HORIZON (R)"),TEXT("GRANDMA'S RANCH (G)"),TEXT("VALLEY HIGH REUNION (PG-13)"),
  TEXT("CHOPPER PATROL III (R)"),TEXT("A KISS FOR KATIE (PG)"),
 };
 return Pot;
}

const TArray<S47LedgerText::FFrontDef>& S47LedgerText::Fronts()
{
 // What the station registered, told from the outside: at least two ways for each kind of night, three for the ones the nights give
 // most (a dead cow, cattle, lights, a car that dies), so a second week of the same does not read the same.
 static const TArray<FFrontDef> Pot={
  {ETopic::Called,TEXT("NIGHT OPERATOR REPORTS MISSING MAN'S TRUCK"),TEXT("The pickup of a ranch hand missing since last week was called in {night} by the new night operator at the SARO observatory, who came across it out by {place}. Deputies are searching the area. The family's reward went to the station."),nullptr,0,0},
  {ETopic::Called,TEXT("MISSING MAN'S PICKUP FOUND"),TEXT("Deputies went out after {night} to a pickup the radio observatory's night operator found out by {place}. It belongs to a ranch hand nobody has seen since last week. The search goes on from there."),nullptr,0,0},
  {ETopic::Missing,TEXT("PICKUP FOUND, DRIVER STILL MISSING"),TEXT("A blue pickup belonging to a ranch hand reported missing turned up out by {place} {night}, the keys still in it and his coat on the seat. His tracks go down into the wash. His family asks anyone who saw him to call the substation."),nullptr,0,0},
  {ETopic::Missing,TEXT("NO WORD OF MISSING RANCH HAND"),TEXT("Word is that the pickup of the ranch hand missing since last week sits out by {place}, the keys in it. It was seen there {night}. The sheriff's office asks people to leave it be until a deputy has been out."),nullptr,0,0},
  {ETopic::Carcass,TEXT("COW FOUND DEAD, CUTS CLEAN"),TEXT("A cow was found dead out by {place} {night}, lying in a ring of flattened grass. Every cut was clean, there was no blood on the ground and no tracks going in or out. The brand inspector says the cause will be hard to find. Ranchers are asked to report any more."),nullptr,0,0},
  {ETopic::Carcass,TEXT("A DEAD COW AND NO TRACKS"),TEXT("The cow found {night} out by {place} had an ear, an eye and a strip of hide off her, the edges as clean as a knife's, and no blood under her. There were no tracks to her, hers included. The brand inspector would like to hear of any more."),nullptr,0,0},
  {ETopic::Carcass,TEXT("RANCHERS ASK ABOUT DEAD COW"),TEXT("Talk at the sale barn this week was of the cow found out by {place} {night}: clean cuts, no blood, and no tracks. \"Coyotes would have been at her by morning,\" one rancher said. \"They weren't.\""),nullptr,0,0},
  {ETopic::Cattle,TEXT("CATTLE ACTING STRANGE, RANCHERS SAY"),TEXT("Stock out by {place} were seen standing close together {night}, every head turned the same way and not a sound out of them. The extension agent suggests a coyote or a change in the weather. A rancher who asked that his name not be printed says cattle do not stand like that for a coyote."),nullptr,0,0},
  {ETopic::Cattle,TEXT("A HERD THAT WOULD NOT MOVE"),TEXT("Cattle out by {place} stood in a tight ring {night} and would not be moved, not for a truck and not for a man walking in among them. By morning they were grazing as if nothing had happened."),nullptr,0,0},
  {ETopic::Cattle,TEXT("SEVEN HEAD, ONE WAY"),TEXT("Seven head of cattle were seen {night} out by {place}, standing in a ring with their heads all turned one way, quiet as fence posts. The brand inspector says cattle will bunch for weather. \"Not like that,\" said the man who saw them."),nullptr,0,0},
  {ETopic::Landing,TEXT("BURNT PATCH PUZZLES RANCHER"),TEXT("A ring of scorched ground turned up out by {place} after {night}, with square marks pressed into it. The extension agent says lightning can do odd things to dry ground in the spring. The man who found it says lightning does not do circles."),nullptr,0,0},
  {ETopic::Landing,TEXT("MARKS ON THE GROUND"),TEXT("Somebody, or something, left pressed marks in the dirt out by {place} {night}. The sheriff's office went and looked and took no report. \"It's no crime to leave marks,\" a deputy said."),nullptr,0,0},
  {ETopic::Landing,TEXT("A RING IN THE BRUSH"),TEXT("The brush is burnt brown in a circle out by {place}, found after {night}, and the beetles inside it are dead. Outside the ring nothing is touched. The county agent has sent a jar of the soil to the university."),nullptr,0,0},
  {ETopic::Lights,TEXT("LIGHTS REPORTED OVER THE VALLEY"),TEXT("Callers told the sheriff's substation they saw lights low over {place} {night}. A deputy drove out and found nothing he could write down. The airfield in Socorro says it had nothing up. \"They didn't blink like a plane,\" one caller said. \"They came down slow and went out.\""),nullptr,0,0},
  {ETopic::Lights,TEXT("MORE LIGHTS, NO ANSWERS"),TEXT("Again this spring people are talking about lights at night, this time over {place}, {night}. The sheriff's office has no explanation and says it is not looking for one. Anyone who saw something is asked to call the substation, not the newspaper."),nullptr,0,0},
  {ETopic::Lights,TEXT("WHAT THE DRIVERS SAW"),TEXT("Two drivers on the highway say they pulled over {night} to watch lights come down over {place}. A third, riding with one of them, says he saw nothing at all. The Ledger reports all three."),nullptr,0,0},
  {ETopic::LitHouse,TEXT("A LIGHT AFTER HOURS"),TEXT("A light was seen in a window at {place} {night}, at an hour when nobody should have been there. The sheriff's office asks whoever it was to stay out; the place is not theirs."),nullptr,0,0},
  {ETopic::LitHouse,TEXT("A WINDOW LIT LATE"),TEXT("A driver going by {place} {night} saw one window lit, long after anyone should have been there. By the time he had turned around and come back, it was dark."),nullptr,0,0},
  {ETopic::LitHouse,TEXT("WHO TURNED ON THE LIGHT?"),TEXT("Neighbors noticed a light on at {place} late {night}, and nobody with a key was out there. When somebody went to look, the window was dark again. A deputy found the doors shut."),nullptr,0,0},
  {ETopic::Stalled,TEXT("LIGHTS AND RADIO QUIT ON THE HIGHWAY"),TEXT("A driver on the highway near {place} {night} says the headlights, the dash and the radio all went dead at once while a light went over, low, and came back by themselves a few seconds later. A mechanic in town says that can be a bad ground. \"Not usually all at once,\" he said."),nullptr,0,0},
  {ETopic::Stalled,TEXT("ENGINE DIES UNDER A LIGHT"),TEXT("A driver says the engine quit near {place} {night} with a white light standing over the road, and the light was gone when it would start again. The state police had no other reports."),nullptr,0,0},
  {ETopic::Stalled,TEXT("TROUBLE ON THE NIGHT ROAD"),TEXT("A driver on the highway near {place} {night} told the diner the headlights flickered out and back as something passed over. The co-op says its lines were fine that night."),nullptr,0,0},
  {ETopic::EmptyRig,TEXT("RIG FOUND IDLING, DRIVER GONE"),TEXT("A truck was found out by {place} {night}, stuck in the sand with the engine idling and the CB on, after its driver had called for help. His boot prints go out along the fence and stop. The trucking company in El Paso says he is a careful man."),nullptr,0,0},
  {ETopic::EmptyRig,TEXT("SEARCH FOR TRUCK DRIVER"),TEXT("Deputies are looking for a truck driver whose rig was found out by {place} {night} with the engine still running. He had called for help on the CB. Anyone who heard that call is asked to come forward."),nullptr,0,0},
  {ETopic::Camper,TEXT("CAMPER LEFT ON OPEN LAND"),TEXT("A camper with every light on was reported out by {place} {night}, and nobody around it. Deputies say the owner, from out of state, has not been reached. Visitors are reminded that the land out there belongs to somebody."),nullptr,0,0},
  {ETopic::Camper,TEXT("CHEMICAL SMELL AT CAMPER"),TEXT("Deputies are asking about a camper found out by {place} {night} with its lights on and a strong chemical smell inside. Nobody has come back for it."),nullptr,0,0},
  {ETopic::Walker,TEXT("WOMAN SEEN WALKING THE HIGHWAY"),TEXT("Motorists reported a woman walking the highway shoulder after dark {night}, a long way from anywhere. A deputy drove the stretch twice and found nobody. Anyone missing a relative is asked to call the substation."),nullptr,0,0},
  {ETopic::Walker,TEXT("HITCHHIKER ON THE NIGHT ROAD"),TEXT("A woman thumbing a ride on the highway {night} was seen by more than one driver. Nobody has said where she went."),nullptr,0,0},
  {ETopic::Outage,TEXT("LIGHTS OUT IN TOWN"),TEXT("Every light in Caliche went out {night}, one street after another, and came back on a minute or two later. People near the transformer say it hummed loud enough to hear across town. The electric co-op found no fault on the line."),nullptr,0,0},
  {ETopic::Outage,TEXT("CO-OP CAN'T EXPLAIN OUTAGE"),TEXT("The electric co-op has no explanation for the minute {night} when the lights went out all over Caliche, one block after another. Members who lost a freezer are asked to call the co-op."),nullptr,0,0},
  {ETopic::Fled,TEXT("JACKRABBITS ON THE RUN"),TEXT("Jackrabbits by the dozen came across the land near {place} {night}, all running one way. A rancher says he has seen them run from a grass fire like that. There was no fire."),nullptr,0,0},
  {ETopic::Fled,TEXT("RABBITS ON THE ROAD"),TEXT("Drivers near {place} {night} had to brake for jackrabbits, dozens of them, crossing all one way and paying no mind to the headlights."),nullptr,0,0},
  {ETopic::ClockFast,TEXT("THE CLOCK ON THE DASH"),TEXT("A reader who drives the highway at night writes that after {night} the clock on the dash was forty minutes ahead of the watch on the wrist. The jeweler on Main Street says a dash clock can jump when the voltage does."),nullptr,0,0},
  {ETopic::ClockFast,TEXT("FORTY MINUTES"),TEXT("A driver on the highway {night} says the clock in the car gained forty minutes in one night and the wristwatch did not. The man at the service station checked the battery and the clock and found nothing wrong with either."),nullptr,0,0},
  {ETopic::Rumor,TEXT("TALK AT THE COUNTER"),TEXT("At the gas station and the diner this week the talk was of lights out by {place}, seen by a friend of a friend. Nobody the Ledger reached saw them firsthand. Those who say they did may call the substation."),nullptr,0,0},
  {ETopic::Rumor,TEXT("A FRIEND OF A FRIEND"),TEXT("A reader says her cousin saw lights come down slowly out by {place} and go out on the ground. The cousin would not talk to the Ledger. The reader would like somebody to go and look."),nullptr,0,0},
  {ETopic::Rumor,TEXT("SOMEBODY SAW SOMETHING"),TEXT("The Ledger has had three calls this week about something seen out by {place} at night. None of the callers would give a name, and no two of them described the same thing."),nullptr,0,0},
  {ETopic::Rumor,TEXT("WHAT THE HERDER SAID"),TEXT("A herder bringing sheep down to the river this week told people in town he would not bed his band out by {place} again. He did not say why, and he did not stay to be asked."),nullptr,0,0},
 };
 return Pot;
}

const TArray<S47LedgerText::FFrontDef>& S47LedgerText::Quiet()
{
 static const TArray<FFrontDef> Pot={
  {ETopic::Count,TEXT("SEARCH FOR MISSING PAIR SCALED BACK"),TEXT("The sheriff's office has scaled back its search for the SARO observatory's night operator, Reyes, and for Mara Ortiz, a district technician who was out on a call the same night. Both have been missing since April 14. The district's white wagon was found on the flat west of the old grade and is being kept there under a hold. Anyone with information is asked to call the substation."),TEXT("search"),0,0},
  {ETopic::Count,TEXT("COUNTY TO HEAR FUTURE OF RADIO STATION"),TEXT("The county commission will hear the district's report on the SARO radio observatory at its last meeting in May. A new night operator has started at the station. \"We'll see what the numbers say,\" said a district spokesman, who would not say which numbers."),TEXT("saro"),0,527},
  {ETopic::Count,TEXT("TRUCK STOP PLAN DRAWS A CROWD"),TEXT("Forty people came to the community center Tuesday to hear plans for a truck stop at the highway junction south of town: twelve diesel pumps, a cafe and a scale. The planning board votes in June. About half the room clapped."),TEXT("truckstop"),521,0},
  {ETopic::Count,TEXT("MEMORIAL DAY RODEO GROWS"),TEXT("Entries for the Memorial Day rodeo are up by a third, the arena manager says, with barrel racers coming from three counties. The chutes have been rebuilt and the bleachers painted."),TEXT("rodeo"),513,525},
  {ETopic::Count,TEXT("SCHOOL BOARD PICKS A ROOFER"),TEXT("The elementary school gets a new roof this summer for $8,400, the school board decided after a short vote and a long discussion of the old roof. Work starts when school lets out."),TEXT("roof"),0,0},
  {ETopic::Count,TEXT("NINETEEN GRADUATE"),TEXT("The class of '86 got its diplomas Friday night in the high school gym. Valedictorian Delia Montoya kept her speech short, as promised, and the families stayed for cake."),TEXT("graduation"),524,0},
  {ETopic::Count,TEXT("TOWN WELL DOWN FOUR FEET"),TEXT("The town well has dropped four feet since fall, the water board reported Monday. Residents are asked to water lawns only after six, and the board says the aquifer study should be done by June."),TEXT("water"),0,0},
  {ETopic::Count,TEXT("TELEPHONE CO-OP TO DROP THE OPERATOR"),TEXT("Rural members will be able to dial Socorro direct by fall, the telephone co-op says, once a new switch goes in at the exchange. The operator, who has worked the board for twenty-two years, says she will believe it when she sees it."),TEXT("phone"),0,0},
  {ETopic::Count,TEXT("ARROYO BRIDGE CLOSED TO HEAVY LOADS"),TEXT("The arroyo road bridge is closed to anything over three tons until the county engineer has looked at the pilings. School buses east of the highway run fifteen minutes early, and ranchers hauling hay are going the long way around."),TEXT("bridge"),0,0},
  {ETopic::Count,TEXT("CULL COWS DOWN AT THE SALE BARN"),TEXT("Cull cows sold for $38 to $42 a hundredweight at the sale barn this week, down from April. \"Too many cows and not enough rain,\" one seller said. Hay holds at five dollars a ton over March."),TEXT("sale"),505,0},
  {ETopic::Count,TEXT("LIBRARY HAS SHELVES, NEEDS BOOKS"),TEXT("The new shelves are up at the county library, and there are more of them than books to put on them. Books in good shape may be left at the desk; the librarian asks that nobody bring encyclopedias from before the war."),TEXT("library"),508,0},
 };
 return Pot;
}

const TArray<S47LedgerText::FLogDef>& S47LedgerText::Logs()
{
 static const TArray<FLogDef> Pot={
  {ETopic::Carcass,TEXT("A dead cow reported out by {place}. Brand inspector notified.")},
  {ETopic::Cattle,TEXT("Cattle reported standing bunched out by {place}, not moving. Owner notified.")},
  {ETopic::Landing,TEXT("Burnt ground reported out by {place}. Deputy looked; no report made.")},
  {ETopic::Lights,TEXT("Lights reported low over {place}. Nothing found.")},
  {ETopic::LitHouse,TEXT("A light reported in a window at {place} after hours. Deputy found the doors shut.")},
  {ETopic::Stalled,TEXT("A motorist reported the lights and radio cutting out near {place}. No damage.")},
  {ETopic::EmptyRig,TEXT("A truck reported idling out by {place}, the driver not with it. Company notified.")},
  {ETopic::Camper,TEXT("A camper reported out by {place} with its lights on and nobody around. Owner not reached.")},
  {ETopic::Walker,TEXT("A woman reported walking the highway shoulder after dark. Deputy drove the stretch; nobody found.")},
  {ETopic::Outage,TEXT("Power out all over town for a minute or two. Co-op notified.")},
  {ETopic::Fled,TEXT("Jackrabbits crossing the road in numbers near {place}. Reported as a hazard.")},
 };
 return Pot;
}

const S47LedgerText::FLetterDef& S47LedgerText::Thanks()
{
 static const FLetterDef Line={TEXT("THANK YOU"),TEXT("Editor: Thanks to the night operator from the radio station who pulled our rig out of the sand last week and would not take more than gas money. There are still people out here who stop. / A trucker from Amarillo"),nullptr,nullptr,0,0};
 return Line;
}

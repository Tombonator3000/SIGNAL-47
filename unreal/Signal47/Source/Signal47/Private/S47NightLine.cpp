#include "S47NightLine.h"
#include "S47Radio.h"
#include "S47Weather.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47Happenings.h"
#include "S47RadioTips.h"
#include "S47EventPlaces.h"
#include "S47Places.h"
#include "Internationalization/Regex.h"

// The text filter every other voice on the air goes through (S47Happenings.cpp), and its compass words.
namespace S47Hap
{
 bool Clean(const FString& Text);
 FString Compass(double Az);
}

// Written for Socorro County in the spring of 1986: Earl, the people who call him, the town and its roads. Nobody real, no make of
// anything, no real song or singer (the records and their singers are made up), American spelling. NIGHT-RADIO-20261109.
namespace
{
 struct FPiece { const TCHAR* Id; const TCHAR* W1; const TCHAR* T1; const TCHAR* W2=nullptr; const TCHAR* T2=nullptr; const TCHAR* W3=nullptr; const TCHAR* T3=nullptr; };

 // Earl between the records. The first three are his recorded words, kept exactly so that his own voice says them.
 const FPiece EARL[]={
  {TEXT("rec_cold"),TEXT("host"),TEXT("Cold one out there. Thirty-eight on the bank sign, and the wind has laid down."),TEXT("host"),TEXT("Phones are open. If you are up anyway, you might as well talk to somebody.")},
  {TEXT("rec_crew"),TEXT("host"),TEXT("This one goes out to the night crew up at the observatory. Somebody has to listen to the sky.")},
  {TEXT("rec_comet"),TEXT("host"),TEXT("Halley's comet is low in the south this week. Get out past the last streetlight and give your eyes ten minutes.")},
  {TEXT("e01"),TEXT("host"),TEXT("My mother used to say nothing good happens after midnight. She never ran a radio station.")},
  {TEXT("e02"),TEXT("host"),TEXT("Somebody left a sack of green chile on the station steps tonight. No note. Whoever you are, thank you. They're hot ones.")},
  {TEXT("e03"),TEXT("host"),TEXT("Fella asked me why we play the same records every night. It's because we own the same records.")},
  {TEXT("e04"),TEXT("host"),TEXT("If you're on the county road tonight, dim your brights for the other fella. There's maybe three of you out there. Be nice to each other.")},
  {TEXT("e05"),TEXT("host"),TEXT("The coffee in here has been on the burner since six. It isn't coffee anymore. It's a decision.")},
  {TEXT("e06"),TEXT("host"),TEXT("Got a letter from a lady in Belen who says she can hear us on her stove. Ma'am, I don't know how, but welcome.")},
  {TEXT("e07"),TEXT("host"),TEXT("Quiet night. The phone's just sitting there like it's thinking something over.")},
  {TEXT("e08"),TEXT("host"),TEXT("The air conditioner in this studio quit in nineteen seventy-nine. Nobody misses it until about June.")},
  {TEXT("e09"),TEXT("host"),TEXT("For everybody working nights, at the gas station, the hospital in Socorro, out at the dishes: we see you. Well. We hear you.")},
  {TEXT("e10"),TEXT("host"),TEXT("My nephew wants me to play some of that new music. I told him we'll play it when it's old.")},
  {TEXT("e11"),TEXT("host"),TEXT("There's a moth in the studio the size of a silver dollar. He's been sitting on the microphone an hour. He's my producer now.")},
  {TEXT("e12"),TEXT("host"),TEXT("Somebody asked what I do between records. Mostly I listen to the transmitter hum and think about pie.")},
  {TEXT("e13"),TEXT("host"),TEXT("A word from the county: no burn barrels when the wind's up. The wind is always up. Use your judgment.")},
  {TEXT("e14"),TEXT("host"),TEXT("I want to thank the lady who calls every night just to say good night and hang up. Good night to you too.")},
  {TEXT("e15"),TEXT("host"),TEXT("We're the only thing on this end of the dial for a hundred miles. Some nights that feels like a lot to carry.")},
  {TEXT("e16"),TEXT("host"),TEXT("Out the studio window I can see the light on at the feed store and two stars. That's the view. I've had worse.")},
  {TEXT("e17"),TEXT("host"),TEXT("If your dog's barking at nothing, it's a skunk. If he isn't barking at all, let him in anyway.")},
  {TEXT("e18"),TEXT("host"),TEXT("A man came in today to ask if we take requests. We do. We just don't play them.")},
  {TEXT("e19"),TEXT("host"),TEXT("That's the end of my sandwich, and the end of my news. Back to the records.")},
  {TEXT("e20"),TEXT("host"),TEXT("My uncle says a radio station is a lighthouse for people in cars. He sells insurance, but he's right about that one.")},
  {TEXT("e21"),TEXT("host"),TEXT("If you hear static for a minute, that's not your set. That's the transmitter catching its breath. It's older than I am.")},
  {TEXT("e22"),TEXT("host"),TEXT("Somebody's got a cow bawling down by the river and it carries all the way into town. Somebody go see what she wants.")},
  {TEXT("e23"),TEXT("host"),TEXT("Spring in Socorro County: you can get sunburned and frostbit the same day. I have.")},
  {TEXT("e24"),TEXT("host"),TEXT("Lost and found: one brown work glove, left hand, at the Mesa. If you've got the right one, you know where the left one is.")},
  {TEXT("e25"),TEXT("host"),TEXT("Long night yet, folks. Put the kettle on.")},
 };
 // CRIT9: the two recorded lines that say what the night is like play only on a night that is so (the weather in the same hour says
 // the rest); on any other night their slot gets these written ones instead, once each where the recording would have been.
 const FPiece EARL_MILD={TEXT("e_mild"),TEXT("host"),TEXT("Not a bad night out there for the time of year. Phones are open. If you are up anyway, you might as well talk to somebody.")};
 const FPiece EARL_CLOUD={TEXT("e_cloud"),TEXT("host"),TEXT("Halley's comet is up there somewhere this week, low in the south, behind all that cloud. Try again tomorrow night.")};

 // Callers who cannot sleep, and what Earl says back.
 const FPiece SLEEPLESS[]={
  {TEXT("c01"),TEXT("woman"),TEXT("Earl, I can't sleep, so I'm ironing. I've ironed everything in the house. I ironed a dish towel."),TEXT("host"),TEXT("Iron mine next. I'll leave them on your step.")},
  {TEXT("c02"),TEXT("man"),TEXT("I drive the milk truck out of Socorro. I go on at four, so I don't see the point of going to bed."),TEXT("host"),TEXT("Drive careful, and say hello to the cows.")},
  {TEXT("c03"),TEXT("old"),TEXT("My wife's been gone two years, and the house is too quiet without the radio. That's all. Thank you for being on."),TEXT("host"),TEXT("We're here till four, sir. Every night.")},
  {TEXT("c04"),TEXT("woman"),TEXT("The baby's teething. We're walking the floor. Could you play something slow?"),TEXT("host"),TEXT("Next one's slow. Next three, if you need them.")},
  {TEXT("c05"),TEXT("man"),TEXT("Earl, settle a bet. Is the Rio Salado a river or a creek?"),TEXT("host"),TEXT("It's whatever it is that week. Go to bed, both of you.")},
  {TEXT("c06"),TEXT("woman"),TEXT("I work the desk at the motel. Nobody's checked in since supper. I've done the crossword twice."),TEXT("host"),TEXT("Do it in pen the third time. Keeps it interesting.")},
  {TEXT("c07"),TEXT("old"),TEXT("The doctor says it's my age that keeps me up. I told him I'm the same age I always was."),TEXT("host"),TEXT("That's the spirit.")},
  {TEXT("c08"),TEXT("man"),TEXT("Just got off at the gravel pit. Nobody up at home, so I'm calling you."),TEXT("host"),TEXT("Glad you did. Go eat something.")},
  {TEXT("c09"),TEXT("woman"),TEXT("My husband snores like a stock trailer on a washboard road. I'm out in the kitchen with you."),TEXT("host"),TEXT("Kitchen's the best room in any house anyhow.")},
  {TEXT("c10"),TEXT("man"),TEXT("I can never sleep before a sale. Taking forty head to the barn in the morning."),TEXT("host"),TEXT("Good luck to you, and good prices.")},
  {TEXT("c11"),TEXT("woman"),TEXT("My daughter's at college in Las Cruces and she hasn't called all week. Is that normal?"),TEXT("host"),TEXT("She'll call Sunday wanting money, ma'am. That's normal.")},
  {TEXT("c12"),TEXT("old"),TEXT("I listen every night on a set I built myself in nineteen fifty-two. Still works. Just wanted you to know."),TEXT("host"),TEXT("Works better than ours, I'd bet.")},
  {TEXT("c13"),TEXT("man"),TEXT("I'm parked at the Y with my rig, and the little TV in the sleeper only gets snow out here. You're the only one awake."),TEXT("host"),TEXT("Then keep me company, and I'll keep you company.")},
  {TEXT("c14"),TEXT("woman"),TEXT("I'm up baking for a funeral tomorrow. Fourteen pies. I wanted to hear somebody's voice."),TEXT("host"),TEXT("I'm sorry for your loss, ma'am. Fourteen pies is a lot of love.")},
  {TEXT("c15"),TEXT("man"),TEXT("My wife's having the baby at the hospital in Socorro, and they sent me out for coffee. I'm calling you instead."),TEXT("host"),TEXT("Go back in there. Right now. Call us with a name.")},
  {TEXT("c16"),TEXT("old"),TEXT("I drove a school bus in this county for thirty-one years. I still wake up at a quarter to six."),TEXT("host"),TEXT("Then you've got some sleeping to catch up on, sir.")},
  {TEXT("c17"),TEXT("woman"),TEXT("Earl, is there a word for being tired and not sleepy at the same time?"),TEXT("host"),TEXT("Yes, ma'am. Radio.")},
  {TEXT("c18"),TEXT("man"),TEXT("The wind took my shed roof off this afternoon. I'm sitting up with a flashlight waiting for it to come back."),TEXT("host"),TEXT("If it comes back, call us. That'd be news.")},
  {TEXT("c19"),TEXT("woman"),TEXT("I'm a nurse on the night ward. I've got ten minutes of break, and I spent it calling you."),TEXT("host"),TEXT("Ten minutes well spent. Back to work.")},
  {TEXT("c20"),TEXT("old"),TEXT("My knees say rain's coming. They've been wrong since nineteen sixty-one, but I keep listening."),TEXT("host"),TEXT("That's loyal of you.")},
  {TEXT("c21"),TEXT("man"),TEXT("I'm a welder on swing shift. I eat my supper at midnight, like everybody else eats breakfast."),TEXT("host"),TEXT("Supper at midnight sounds about right in here too.")},
  {TEXT("c22"),TEXT("woman"),TEXT("The kids are asleep and the dishes are done, and it's the first quiet I've had all week. I don't want to waste it sleeping."),TEXT("host"),TEXT("Then don't. We'll be quiet with you.")},
  {TEXT("c23"),TEXT("man"),TEXT("I'm studying for my contractor's license, and the words have quit making sense. Say something normal."),TEXT("host"),TEXT("Corn's up a nickel at the feed store. That normal enough?")},
  {TEXT("c24"),TEXT("old"),TEXT("I don't hear so good, so I put the radio on the pillow. If you hear snoring, it's me."),TEXT("host"),TEXT("Sleep well, sir.")},
  {TEXT("c25"),TEXT("woman"),TEXT("Our cow had her calf this evening. Heifer. We named her after you."),TEXT("host"),TEXT("Well. That's the nicest thing anybody's done for me in a while.")},
  {TEXT("c26"),TEXT("man"),TEXT("I teach at the high school, and I've got thirty papers on the Louisiana Purchase to grade. Talk me through it."),TEXT("host"),TEXT("Grade them easy. It was a long time ago.")},
  {TEXT("c27"),TEXT("woman"),TEXT("I sit up for my husband. He drives the bread route and gets home late. I like knowing somebody else is up."),TEXT("host"),TEXT("Somebody is. Tell him to drive careful.")},
  {TEXT("c28"),TEXT("old"),TEXT("My brother and I haven't spoken in nine years. I thought about calling him tonight. I called you instead."),TEXT("host"),TEXT("Call him tomorrow. In the daytime. That's when it works.")},
 };

 // Before midnight (Hint): what anybody could explain away, and Earl does. The first is a recorded caller and Earl's recorded answer.
 const FPiece STRANGE_HINT[]={
  {TEXT("rec_dolores"),TEXT("woman"),TEXT("Earl, it's Dolores out on the Hondo road. The lights are back over the mesa. Same as last fall."),TEXT("host"),TEXT("Dolores, I believe you. Now turn the porch light off and go to bed.")},
  {TEXT("h01"),TEXT("man"),TEXT("There's a light sitting on the ridge west of town, and it hasn't moved in an hour. Somebody tell me it's a radio tower."),TEXT("host"),TEXT("It's a radio tower. Probably.")},
  {TEXT("h02"),TEXT("woman"),TEXT("My television's had a hum on channel seven all evening. Like a tuning fork. Anybody else?"),TEXT("host"),TEXT("Wind on the antenna, I'd guess.")},
  {TEXT("h03"),TEXT("old"),TEXT("The well pump came on by itself tonight. Twice. Nobody running any water."),TEXT("host"),TEXT("Pressure switch. They get tired, same as us.")},
  {TEXT("h04"),TEXT("man"),TEXT("Saw a light go across slow over the Magdalenas, and then it just quit. No sound at all."),TEXT("host"),TEXT("Satellites do that. Or so I'm told.")},
  {TEXT("h05"),TEXT("woman"),TEXT("The dog won't go out tonight. Sat at the door with his ears down, then went and lay under the bed."),TEXT("host"),TEXT("Some nights they just don't want to. Usually it's a bath coming.")},
  {TEXT("h06"),TEXT("man"),TEXT("Anybody else's radio squeal for a second about an hour ago? Not you, Earl. The whole set."),TEXT("host"),TEXT("Sunspots, they tell me. They tell me a lot of things.")},
  {TEXT("h07"),TEXT("old"),TEXT("There's a hum out by the stock tank. Low. You feel it more than you hear it. Three nights now."),TEXT("host"),TEXT("Could be the power line. Could be the windmill.")},
  {TEXT("h08"),TEXT("woman"),TEXT("My kitchen clock and my wristwatch have been five minutes apart since supper. They never were before."),TEXT("host"),TEXT("Pick the one you like better.")},
  {TEXT("h09"),TEXT("man"),TEXT("Coming back from Socorro my headlights flickered the whole way past the dishes. The battery's new."),TEXT("host"),TEXT("Bad ground. It's always a bad ground.")},
  {TEXT("h10"),TEXT("woman"),TEXT("The coyotes all started up at once tonight and quit all at once. Like somebody turned them off."),TEXT("host"),TEXT("They do that. Mostly.")},
  {TEXT("h11"),TEXT("old"),TEXT("Little green light down in the arroyo a while ago. Kids with a flashlight, I'd guess. Didn't hear any kids."),TEXT("host"),TEXT("Kids are quiet when they're up to something.")},
  {TEXT("h12"),TEXT("man"),TEXT("There's a smell like a struck match all down the county road tonight. No fire anywhere I can see."),TEXT("host"),TEXT("Somebody burning trash where they shouldn't. Call the fire department if you see smoke.")},
  {TEXT("h13"),TEXT("woman"),TEXT("The porch light keeps going dim and coming back. The kitchen light doesn't."),TEXT("host"),TEXT("Bulb's going. Or the co-op's tired.")},
  // N1110 (Caliche history, approved by Tom 10 November): a grandmother's rule, the cave under Main Street, the motel's register card.
  {TEXT("h14"),TEXT("woman"),TEXT("Earl, my grandmother told me never to wave at the lights. I waved at one tonight. Just a little. I'd like to know if anybody else ever did that, and what happened. No, I'm fine. I'm just asking."),TEXT("host"),TEXT("Your grandmother was a wise woman. You're fine. Just don't make a habit of it.")},
  {TEXT("h15"),TEXT("man"),TEXT("Earl, the Breather was going the wrong way this evening. I felt it come up through the grate on Main Street. Cold as a cellar. I went home and turned the porch light off."),TEXT("host"),TEXT("Caves do that when the weather changes. Porch light was a good idea anyway.")},
  {TEXT("h16"),TEXT("woman"),TEXT("Earl, I'm calling from the motel. I've got the register card here, and it has a box that says Lights, with a question mark. Do I check it if I'm not sure?"),TEXT("host"),TEXT("Check it. Everybody does. Then get some sleep.")},
 };

 // Midnight to two (Glimpse): a figure or a light a long way off, gone when anybody looks again. Never near, never touched. The
 // first is recorded; it says "since one", so it comes in the hour from one.
 const FPiece STRANGE_GLIMPSE[]={
  {TEXT("rec_dog"),TEXT("man"),TEXT("My dog won't come out from under the trailer. He's been under there since one."),TEXT("host"),TEXT("Smart dog.")},
  {TEXT("g01"),TEXT("man"),TEXT("Coming home on sixty I saw somebody standing way out on the flat, past the fence line. Just standing. No car. I didn't stop."),TEXT("host"),TEXT("Rancher checking water, probably. Probably.")},
  {TEXT("g02"),TEXT("woman"),TEXT("Something tall crossed the road way up ahead of me in the high beams. Too tall for an elk. It was gone when I got there."),TEXT("host"),TEXT("Elk get big out here, ma'am.")},
  {TEXT("g03"),TEXT("old"),TEXT("Somebody's walking the ridge line with no light. I can see him against the sky. He's been walking a good while."),TEXT("host"),TEXT("The sheriff's number is in the front of your phone book, sir.")},
  {TEXT("g04"),TEXT("man"),TEXT("Pulled over to stretch at the turnout, and there was something out by the windmill, a long way off. It didn't move, and neither did I."),TEXT("host"),TEXT("Then you both did the right thing.")},
  {TEXT("g05"),TEXT("woman"),TEXT("My boy says there was a man way out past the corral, by the far fence. Thin, he says. When I looked there wasn't anybody."),TEXT("host"),TEXT("Kids see things at night. Lock the door anyway.")},
  {TEXT("g06"),TEXT("man"),TEXT("Something low ran across the road way up ahead of me on all fours. Not a dog. Moved wrong for a dog."),TEXT("host"),TEXT("Badger, could be. They're bigger than you'd think.")},
  {TEXT("g07"),TEXT("old"),TEXT("There's a light way out on the playa that goes on and off, slow. Like something breathing."),TEXT("host"),TEXT("Somebody's lantern, sir.")},
  {TEXT("g08"),TEXT("woman"),TEXT("There was a figure at the edge of my lights by the mile marker. I looked again and it was a fence post. I'm almost sure it was a fence post."),TEXT("host"),TEXT("Most things are fence posts.")},
  {TEXT("g09"),TEXT("man"),TEXT("Something's standing out in the stock tank, way out, up to its knees. Hasn't moved in twenty minutes. I've got the binoculars on it from the porch."),TEXT("host"),TEXT("Stay on the porch.")},
  {TEXT("g10"),TEXT("old"),TEXT("All week the cattle have faced the same way at night. Tonight they're all facing my house."),TEXT("host"),TEXT("Maybe they want breakfast.")},
  {TEXT("g11"),TEXT("woman"),TEXT("Somebody was standing way out in the middle of the alfalfa with his arms down at his sides. Too far to see a face. I flashed the porch light and he was gone."),TEXT("host"),TEXT("Leave the light on tonight.")},
  {TEXT("g12"),TEXT("man"),TEXT("Way out east there's a row of little lights, low down, like a road where there isn't any road."),TEXT("host"),TEXT("Highway crew working nights, could be.")},
  {TEXT("g13"),TEXT("woman"),TEXT("The neighbor's horses are bunched up tight in the corner of the pasture, all looking at the hill. I can't see what at."),TEXT("host"),TEXT("Horses know things. Close your curtains.")},
 };

 // From two (Full): what came close. Earl stops joking. The first is recorded; it is about a little after two.
 const FPiece STRANGE_FULL[]={
  {TEXT("rec_tv"),TEXT("man"),TEXT("Anybody else lose the TV a little after two? Just a hum, most of a minute. Then it came back."),TEXT("host"),TEXT("We lost the transmitter about then. Needle went to nothing for most of a minute. Ray, if you are listening, that one is yours.")},
  {TEXT("f01"),TEXT("woman"),TEXT("Earl, there's a light in my yard so bright I can read the clock by it, and there's no sound. None. The dog isn't even barking."),TEXT("host"),TEXT("Ma'am, stay inside. Stay on the line with me.")},
  {TEXT("f02"),TEXT("man"),TEXT("My brother went out to the truck for his cigarettes at midnight and came back in at two. He says he was gone five minutes."),TEXT("host"),TEXT("Is he all right?"),TEXT("man"),TEXT("He keeps washing his hands.")},
  {TEXT("f03"),TEXT("old"),TEXT("Something knocked on my window. Three times. I sleep upstairs."),TEXT("host"),TEXT("Don't open it. I mean that.")},
  {TEXT("f04"),TEXT("woman"),TEXT("I woke up and my husband was standing at the bedroom window looking out. He doesn't remember getting up. There's dirt on his feet."),TEXT("host"),TEXT("Keep him in tonight, ma'am.")},
  {TEXT("f05"),TEXT("man"),TEXT("I'm at the pay phone at the Y. My truck died, the lights died, and there's something in the field right beside it. I'm not going back out there."),TEXT("host"),TEXT("Stay in the light, son. I'm calling the sheriff right now.")},
  {TEXT("f06"),TEXT("woman"),TEXT("The baby's been laughing in the dark for ten minutes, at the window. He's never done that."),TEXT("host"),TEXT("Pick him up and go in the kitchen. Turn on every light you've got.")},
  {TEXT("f07"),TEXT("old"),TEXT("I lost two hours tonight. I was watching the late news, and then it was the flag on the screen. I didn't sleep. I know I didn't."),TEXT("host"),TEXT("Write down what you remember, sir. Write it down now.")},
  {TEXT("f08"),TEXT("man"),TEXT("There were three of them at the end of my driveway. Small. No car. I turned the yard light on, and there were two."),TEXT("host"),TEXT("Lock your door. Now.")},
  {TEXT("f09"),TEXT("woman"),TEXT("My kitchen radio's getting you and something under you. Like somebody breathing right up against a microphone."),TEXT("host"),TEXT("That's not us, ma'am. I'd know.")},
  {TEXT("f10"),TEXT("man"),TEXT("My mare came back to the barn shaking, and there's a ring burned in the grass behind the corral, round as a plate."),TEXT("host"),TEXT("Keep her in tonight. Keep yourself in.")},
  {TEXT("f11"),TEXT("old"),TEXT("There's a man in a long coat standing in my yard looking at the house. He's been there an hour, and I don't believe he's breathing."),TEXT("host"),TEXT("Sir, call the sheriff. Then call me back.")},
  {TEXT("f12"),TEXT("woman"),TEXT("I came to in my car on the shoulder of sixty with the engine off and the keys in my lap. I left the house at eleven. Look at the time."),TEXT("host"),TEXT("Ma'am, are you hurt? Stay where you are. Somebody'll come.")},
  {TEXT("f13"),TEXT("man"),TEXT("Something just walked across my roof. Slow, heavy steps, and then nothing. No bird is that heavy."),TEXT("host"),TEXT("Stay off that roof till daylight.")},
 };

 // What the town wants said: church, school, the county. {day} is the night's.
 const TCHAR* const NOTICES[]={
  TEXT("The Altar Society at the Catholic church is holding its enchilada supper {day} evening. Three dollars, and kids under six eat free."),
  TEXT("The Caliche volunteer fire department meets {day} at seven at the firehouse. They need two more volunteers and somebody who can fix the siren."),
  TEXT("The high school band is selling peanut brittle door to door, and they'll be on your street {day}. Buy some. It's for new uniforms."),
  TEXT("The county extension office says the youth rabbit show is {day} at the fairgrounds in Socorro."),
  TEXT("The library will be open late {day} evening for story hour. Bring the little ones in their pajamas."),
  TEXT("Choir practice at the Methodist church is moved to {day} evening. The organist has a dentist appointment."),
  TEXT("Blood drive at the school gym {day}. They give you a cookie and orange juice, and you save a life. That's a good deal."),
  TEXT("The rosary for Manuel Griego is {day} evening at the Catholic church, and the family thanks everybody for all the food."),
  TEXT("Caliche plays Magdalena in baseball {day}. First pitch at four, if the wind lets them."),
  TEXT("The school mothers' bake sale is {day} morning out in front of the post office. Mrs. Trujillo's bizcochitos are gone by ten, so plan accordingly."),
  TEXT("Rummage sale at the veterans' hall {day}. Bring your own bags."),
  TEXT("The school board meets {day} at seven about the new roof on the gym. The public is invited, and so is the rain."),
  TEXT("Bingo at the senior center {day} night. Bring quarters."),
  TEXT("Free tuberculosis screening at the county clinic {day}. It doesn't hurt, they promise."),
  TEXT("First Communion classes start {day} at the parish hall. See Sister Agnes."),
  TEXT("Kindergarten roundup at the elementary school {day}. Bring the birth certificate and the shot record."),
  TEXT("Square dance at the community hall {day} night. Beginners welcome, and so are folks with two left feet."),
  TEXT("The garden club's plant swap is {day} behind the library. Bring a cutting, take a cutting."),
  TEXT("They're branding at the Lucero place {day} and need hands. They feed you."),
  TEXT("The high school track team is washing cars at the gas station {day} to get to the state meet."),
  TEXT("Revival meeting {day} night under the tent behind the Assembly church. Everybody's welcome. Bring a lawn chair."),
  TEXT("The county commission meets {day} morning in Socorro to hear about the gravel on the Alderete road. If you've got an opinion, and you do, go say it."),
  TEXT("Spelling bee at the elementary school {day}. Last year's champion is back, and she is not nervous."),
  TEXT("The Ladies Auxiliary is collecting canned goods for the food pantry. Leave them at the church {day}."),
  TEXT("The youth group from the Baptist church is selling light bulbs door to door {day}. Buy a few. Nobody ever had too many light bulbs."),
  TEXT("Graduation pictures get taken at the high school {day}. Seniors, wear a clean shirt."),
  TEXT("The Mesa Diner will be closed {day} for the cook's daughter's wedding. Congratulations to the family."),
  TEXT("The swimming pool committee meets {day}. There isn't any pool yet. That's what the committee is for."),
  TEXT("Free cheese and butter from the county {day} at the community center. Bring a sack."),
  TEXT("The Catholic church needs drivers {day} to take the shut-ins to the clinic in Socorro. Call the rectory."),
 };
 // Days only (CRIT9): a notice that needs a time of day says it in its own words, so a day never brings a second time.
 const TCHAR* const NOTICE_DAYS[]={TEXT("Saturday"),TEXT("Friday"),TEXT("Thursday"),TEXT("Sunday"),TEXT("Tuesday"),TEXT("next Saturday"),TEXT("Wednesday"),TEXT("Monday")};

 // The roads, as the deputy or somebody on the CB has them.
 const TCHAR* const ROADS[]={TEXT("U.S. sixty west of Magdalena"),TEXT("the county road past the Y"),TEXT("the Rio Salado bridge"),TEXT("the grade up toward Datil"),TEXT("the interstate south of Socorro"),TEXT("the Alderete road"),TEXT("the highway north of Caliche"),TEXT("the road out past the dishes"),TEXT("the old grade by the cattle pens"),TEXT("the river road below Lemitar")};
 const TCHAR* const ROAD_IS[]={TEXT("has cattle out on the pavement, so go slow"),TEXT("still has the highway crew's cones out in one lane"),TEXT("has fresh gravel where they patched it. Mind your windshield"),TEXT("has a deer down on the shoulder"),TEXT("has sand drifted across the low spots"),TEXT("has the state police running radar"),TEXT("has a stock trailer broke down on the shoulder with its flashers going"),TEXT("has a washout sign up at the arroyo crossing. Believe it"),TEXT("is slow behind a hay truck that isn't in any hurry"),TEXT("is clear and empty, and that's the whole report"),TEXT("has a tumbleweed the size of a car sitting in the eastbound lane"),TEXT("has a soft shoulder where the rain took it last fall")};
 const TCHAR* const ROAD_HEAD[]={TEXT("On the roads tonight: "),TEXT("Road report from the deputy: "),TEXT("If you're driving tonight, "),TEXT("Word from a driver on the CB: ")};

 // The records (made up, every one) and how Earl brings them on. Tune: which of the station's three recordings plays (S47Radio).
 struct FRecord { const TCHAR* Title; const TCHAR* Artist; const TCHAR* Tune; };
 const FRecord RECORDS[]={
  {TEXT("Chamisa in the Rain"),TEXT("Lupe Archuleta"),TEXT("waltz")},{TEXT("Two Hours to Socorro"),TEXT("the Dry Creek Ramblers"),TEXT("twostep")},
  {TEXT("Her Porch Light Stays On"),TEXT("Wendell Pruitt"),TEXT("ballad")},{TEXT("Rio Salado Waltz"),TEXT("the Baca Brothers"),TEXT("waltz")},
  {TEXT("A Dollar Two a Gallon"),TEXT("Hollis Ray Tatum"),TEXT("twostep")},{TEXT("Last Dance at the Grange"),TEXT("Ida Mae Curry"),TEXT("waltz")},
  {TEXT("Cattle Guard Rattle"),TEXT("the Pecos Valley Five"),TEXT("twostep")},{TEXT("Whatever Became of Lorena"),TEXT("Wendell Pruitt"),TEXT("ballad")},
  {TEXT("Datil Grade"),TEXT("Sonny Villanueva"),TEXT("twostep")},{TEXT("The Night Shift Waltz"),TEXT("June Kettering"),TEXT("waltz")},
  {TEXT("Pie at Ten O'Clock"),TEXT("the Dry Creek Ramblers"),TEXT("twostep")},{TEXT("Salt Cedar Blues"),TEXT("Hollis Ray Tatum"),TEXT("ballad")},
  {TEXT("Moon Over the Mesa Diner"),TEXT("Lupe Archuleta"),TEXT("ballad")},{TEXT("She Took the Pickup and the Dog"),TEXT("Buck Lindell"),TEXT("twostep")},
  {TEXT("Wind in the Tamarisk"),TEXT("June Kettering"),TEXT("waltz")},{TEXT("Slow Train to Belen"),TEXT("the Baca Brothers"),TEXT("ballad")},
  {TEXT("The Windmill Don't Turn"),TEXT("Buck Lindell"),TEXT("ballad")},{TEXT("Magdalena Two-Step"),TEXT("the Pecos Valley Five"),TEXT("twostep")},
  {TEXT("Leave the Gate Like You Found It"),TEXT("Sonny Villanueva"),TEXT("twostep")},{TEXT("Coffee Cold at the Y"),TEXT("Ida Mae Curry"),TEXT("ballad")},
  {TEXT("Nobody Waltzes Anymore"),TEXT("Hollis Ray Tatum"),TEXT("waltz")},{TEXT("Arroyo After Rain"),TEXT("Lupe Archuleta"),TEXT("waltz")},
  {TEXT("Ninety Miles of Fence"),TEXT("Buck Lindell"),TEXT("twostep")},{TEXT("Senorita of the Sale Barn"),TEXT("the Baca Brothers"),TEXT("twostep")},
  {TEXT("Long Way Home from Lemitar"),TEXT("Wendell Pruitt"),TEXT("ballad")},{TEXT("Last Light on the Magdalenas"),TEXT("June Kettering"),TEXT("ballad")},
  {TEXT("Hay Truck Heartache"),TEXT("Sonny Villanueva"),TEXT("twostep")},{TEXT("The Porch Swing Waltz"),TEXT("Ida Mae Curry"),TEXT("waltz")},
  {TEXT("Tumbleweed Telephone"),TEXT("the Dry Creek Ramblers"),TEXT("twostep")},{TEXT("Dust on the Dashboard"),TEXT("the Pecos Valley Five"),TEXT("ballad")},
 };
 const TCHAR* const INTROS[]={
  TEXT("Here's {artist} with \"{title}.\""),TEXT("\"{title},\" from {artist}. I've about worn this one thin."),TEXT("Now {artist}, and \"{title}.\" My mother's favorite."),
  TEXT("Here's one for the folks on the road. {artist}, \"{title}.\""),TEXT("Off the shelf: {artist}, \"{title}.\""),TEXT("Back to the records. {artist}, \"{title}.\""),
  TEXT("Somebody asked for this one last week, so here it is. {artist}, \"{title}.\""),TEXT("{artist}. \"{title}.\" Turn it up if you're alone."),
 };
 // How long each of the station's recordings is let play: nearly the whole record (S47Radio's Track: 246.4, 211.8 and 159 seconds).
 double RecordLen(const FString& Tune){return Tune==TEXT("ballad")?238.:Tune==TEXT("twostep")?204.:152.;}

 // The hour's station call. At midnight Earl's recorded one, word for word.
 const TCHAR* const CALLS[]={TEXT("Fourteen-twenty, Caliche. The Night Line. It's {time}."),TEXT("It's {time} on the Night Line, fourteen-twenty."),
  TEXT("You're up late with Earl on fourteen-twenty, Caliche. It's {time}."),TEXT("{Time}, Caliche. This is fourteen-twenty, and this is the Night Line.")};
 const TCHAR* const QUIET[]={TEXT("Bear with us, folks. Got to change a reel."),TEXT("Hang on. The transmitter's thinking about something."),TEXT("Give me a minute here. The board's acting up.")};

 // ---------------------------------------------------------------- the seed
 uint32 Mix(uint32 A,uint32 B)
 {
  uint32 H=A*0x9E3779B1u^(B+0x7F4A7C15u+(A<<6)+(A>>2));
  H^=H>>16;H*=0x85EBCA6Bu;H^=H>>13;H*=0xC2B2AE35u;H^=H>>16;return H;
 }
 // A list gone through in the seed's order, a fresh order each time round: in the first time round (Count a night, N items) nothing
 // comes twice, so with N at least 14 x Count nothing repeats in the first two weeks.
 int32 Pick(int32 N,int32 Count,int32 Night,int32 K,uint32 Seed,uint32 Salt)
 {
  const int32 G=FMath::Max(0,Night-1)*Count+K,Round=G/N,At=G%N;
  TArray<int32> P;P.SetNum(N);for(int32 I=0;I<N;++I)P[I]=I;
  const uint32 S=Mix(Mix(Seed,Salt),uint32(Round));
  for(int32 I=N-1;I>0;--I)P.Swap(I,int32(Mix(S,uint32(I))%uint32(I+1)));
  return P[At];
 }
 double Roll(uint32 Seed,int32 Night,uint32 Salt){return double(Mix(Mix(Seed,uint32(Night)),Salt)&0xFFFFFF)/double(0x1000000);}

 FString Cap(FString S){if(!S.IsEmpty())S[0]=FChar::ToUpper(S[0]);return S;}
 // Story's number is never said: a minute on is as good a time.
 FString SafeTime(double Clock){const int32 M=FMath::FloorToInt(FMath::Fmod(FMath::Fmod(Clock,86400.)+86400.,86400.)/60.)%60;return S47Radio::SayTime(M==47?Clock+60:Clock);}
 FString Number(int32 N)
 {
  static const TCHAR* const Ones[]={TEXT("zero"),TEXT("one"),TEXT("two"),TEXT("three"),TEXT("four"),TEXT("five"),TEXT("six"),TEXT("seven"),TEXT("eight"),TEXT("nine"),TEXT("ten"),TEXT("eleven"),TEXT("twelve"),TEXT("thirteen"),TEXT("fourteen"),TEXT("fifteen"),TEXT("sixteen"),TEXT("seventeen"),TEXT("eighteen"),TEXT("nineteen")};
  static const TCHAR* const Tens[]={TEXT(""),TEXT(""),TEXT("twenty"),TEXT("thirty"),TEXT("forty"),TEXT("fifty"),TEXT("sixty"),TEXT("seventy"),TEXT("eighty"),TEXT("ninety")};
  N=FMath::Clamp(N,0,109);if(N==47)N=48;
  if(N>=100)return N==100?FString(TEXT("a hundred")):FString(TEXT("a hundred and "))+Ones[FMath::Min(N-100,19)];
  if(N<20)return Ones[N];
  return FString(Tens[N/10])+(N%10?FString(TEXT("-"))+Ones[N%10]:FString());
 }

 struct FItem { FString Id; ES47Allowed Tier=ES47Allowed::Hint; TArray<FS47RadioLine> Lines; };
 FS47RadioLine Line(const FString& Text,const FString& Who){return FS47RadioLine(Text,Who,Who==TEXT("host")?FString():FString(TEXT("a caller")));}
 FItem Of(const FPiece& P,ES47Allowed Tier)
 {
  FItem I;I.Id=P.Id;I.Tier=Tier;
  for(const auto& WT:{TPair<const TCHAR*,const TCHAR*>(P.W1,P.T1),TPair<const TCHAR*,const TCHAR*>(P.W2,P.T2),TPair<const TCHAR*,const TCHAR*>(P.W3,P.T3)})
   if(WT.Key&&WT.Value)I.Lines.Add(Line(WT.Value,WT.Key));
  return I;
 }
 FItem Host(const FString& Id,const FString& Text){FItem I;I.Id=Id;I.Lines.Add(Line(Text,TEXT("host")));return I;}
 FItem HostAt(const FString& Id,const FString& Template)
 {
  FItem I;I.Id=Id;
  I.Lines.Add(FS47RadioLine([Template](double C){return FString(Template).Replace(TEXT("{Time}"),*Cap(SafeTime(C)),ESearchCase::CaseSensitive).Replace(TEXT("{time}"),*SafeTime(C),ESearchCase::CaseSensitive);},TEXT("host")));
  return I;
 }

 // The night's weather, from the day the sky and the paper have (S47Weather).
 FString Sky(float Cloud,bool bNight){return Cloud<.2f?(bNight?TEXT("clear"):TEXT("sunny")):Cloud<.5f?(bNight?TEXT("a few clouds"):TEXT("partly cloudy")):TEXT("mostly cloudy");}
 FItem Weather(const FS47NightLineIn& In,int32 Hour,int32 NowF=-1)
 {
  const S47Weather::FDay Today=S47Weather::Day(In.Seed,FMath::Max(1,In.Night)),Next=S47Weather::Forecast(In.Seed,FMath::Max(1,In.Night),FMath::Max(1,In.Night)+1);
  const int32 V=int32(Mix(Mix(uint32(In.Seed),uint32(In.Night)),uint32(Hour))%3u);
  const FString Wind=Today.NightStorm>.5f?TEXT("Southwest wind hanging on past midnight, gusty at times."):Today.NightStorm>.2f?TEXT("A little breeze out of the southwest."):TEXT("Winds light.");
  FString Tomorrow=FString::Printf(TEXT("Tomorrow %s, high near %s"),*Sky(Next.CloudPm,false),*Number(Next.HighF));
  if(Next.bDust)Tomorrow+=TEXT(", with a strong southwest wind in the afternoon and blowing sand");
  else if(Next.bShower)Tomorrow+=TEXT(", and a stray shower late in the day, most of it drying up before it gets here");
  else if(Next.Windy())Tomorrow+=TEXT(", and breezy");
  Tomorrow+=TEXT(".");
  static const TCHAR* const Heads[]={TEXT("Socorro County weather."),TEXT("Weather, from the man at the airport in Socorro."),TEXT("Here's your weather.")};
  FItem I;I.Id=FString::Printf(TEXT("wx%d.%d.%d"),Hour,Next.LowF,Next.HighF);
  if(Hour>=26){I.Lines.Add(Line(FString(TEXT("For anybody planning a day: "))+Tomorrow,TEXT("host")));return I;}
  // Tonight's low is the one before tomorrow's daylight (the forecast's for the next day); now is on the way down to it.
  const int32 Low=Next.LowF,Now=NowF>=0?NowF:FMath::RoundToInt(Low+(Today.HighF-Low)*FMath::Clamp((26.-Hour)*.08,0.,.5));
  I.Lines.Add(Line(FString::Printf(TEXT("%s Tonight %s, low around %s. %s"),Heads[V],*Sky(Today.CloudNight,true),*Number(Low),*Wind),TEXT("host")));
  // D46-20261110 (D46-4): a storm night's lightning, where the sky flashes it and the paper prints it
  if(Today.Storm())I.Lines.Add(Line(FString::Printf(TEXT("And there's dry lightning out to the %s tonight, a long way off. No rain in it, so mind your cigarettes in the brush."),*S47Weather::LightningWay(Today)),TEXT("host")));
  if(Hour>=22)I.Lines.Add(Line(FString::Printf(TEXT("It's %s degrees in Caliche right now. %s"),*Number(Now),*Tomorrow),TEXT("host")));
  else I.Lines.Add(Line(Tomorrow,TEXT("host")));
  return I;
 }
 FItem Road(const FS47NightLineIn& In,int32 K)
 {
  const int32 NR=UE_ARRAY_COUNT(ROADS),NC=UE_ARRAY_COUNT(ROAD_IS);
  const int32 P=Pick(NR*NC,2,In.Night,K,uint32(In.Seed),0x524f4144u),R=P/NC,C=P%NC;
  const int32 H=int32(Mix(uint32(In.Seed)+uint32(In.Night),uint32(K))%UE_ARRAY_COUNT(ROAD_HEAD));
  FString Head=ROAD_HEAD[H];
  FString Text=Head+(Head.EndsWith(TEXT(": "))?Cap(ROADS[R]):FString(ROADS[R]))+TEXT(" ")+ROAD_IS[C]+TEXT(".");
  FItem I;I.Id=FString::Printf(TEXT("rd%d"),P);I.Lines.Add(Line(Text,TEXT("host")));
  // CB-TRUCK / the county's trust: the deputy says he'll come by (FS47FreeformSession::SheriffTrust).
  if(K==0&&In.SheriffTrust>=1){I.Id+=TEXT("+trust");I.Lines.Add(Line(TEXT("The deputy asked me to say he'll swing by the dishes tonight, so the night crew knows it's him at the gate."),TEXT("host")));}
  return I;
 }
 FItem Notice(const FS47NightLineIn& In,int32 K)
 {
  const int32 N=UE_ARRAY_COUNT(NOTICES),T=Pick(N,2,In.Night,K,uint32(In.Seed),0x4e4f5449u);
  const int32 D=int32(Mix(uint32(In.Seed)^0x44415953u,uint32(In.Night*3+K))%UE_ARRAY_COUNT(NOTICE_DAYS));
  static const TCHAR* const Heads[]={TEXT("Around town: "),TEXT("From the bulletin board: "),TEXT("Somebody asked me to read this. "),TEXT("")};
  const int32 H=int32(Mix(uint32(In.Seed),uint32(In.Night*7+K))%UE_ARRAY_COUNT(Heads));
  FItem I;I.Id=FString::Printf(TEXT("an%02d"),T);I.Lines.Add(Line(FString(Heads[H])+FString(NOTICES[T]).Replace(TEXT("{day}"),NOTICE_DAYS[D]),TEXT("host")));
  return I;
 }

 struct FHourPlan { TArray<TSharedPtr<FS47RadioSeg>> Segs; TArray<FString> Ids; TArray<ES47Allowed> Tiers; double Secs=0; };
 void AddTalk(FHourPlan& P,const FItem& I)
 {
  if(I.Lines.IsEmpty())return;
  auto S=MakeShared<FS47RadioSeg>();S->Kind=TEXT("talk");S->Lines=I.Lines;P.Segs.Add(S);P.Ids.Add(I.Id);P.Tiers.Add(I.Tier);
  P.Secs+=0.4;for(const FS47RadioLine& L:I.Lines)P.Secs+=S47Radio::SecsOf(L.At(0))+0.7;
 }
 void AddRecord(FHourPlan& P,const FS47NightLineIn& In,int32 Hour,int32 K,bool bIntro)
 {
  const int32 NR=UE_ARRAY_COUNT(RECORDS);
  const uint32 H=Mix(Mix(uint32(In.Seed)^0x52454353u,uint32(In.Night)),uint32(Hour*64+K));
  const FRecord& R=RECORDS[H%uint32(NR)];
  if(bIntro)
  {
   const int32 V=int32((H>>8)%UE_ARRAY_COUNT(INTROS));
   FString Said=FString(INTROS[V]).Replace(TEXT("{title}"),R.Title).Replace(TEXT("{artist}"),R.Artist);
   FItem I;I.Id=FString::Printf(TEXT("mu%02d.%d"),int32(H%uint32(NR)),V);I.Lines.Add(Line(Cap(Said),TEXT("host")));AddTalk(P,I);
  }
  auto S=MakeShared<FS47RadioSeg>();S->Kind=TEXT("music");S->Tune=R.Tune;S->Len=RecordLen(R.Tune);P.Segs.Add(S);P.Secs+=S->Len;
 }
 void AddQuiet(FHourPlan& P,const FS47NightLineIn& In,int32 Hour)
 {
  const uint32 H=Mix(uint32(In.Seed)^0x51554945u,uint32(In.Night*32+Hour));
  if(H%3u)AddTalk(P,Host(TEXT("qt"),QUIET[(H>>4)%UE_ARRAY_COUNT(QUIET)]));
  // S47Radio's "static" kind: the station still on the air, nothing on it but the carrier's hiss.
  auto S=MakeShared<FS47RadioSeg>();S->Kind=TEXT("static");S->Len=40+double((H>>8)%60u);P.Segs.Add(S);P.Secs+=S->Len;
  P.Ids.Add(TEXT("st"));P.Tiers.Add(ES47Allowed::Hint);
 }
 // What a voice calls a place, as the night events have it (FS47Happenings::Spoken): never a sign's capitals, never all of "the salt
 // lick and trough".
 FString Said(const FString& Title)
 {
  if(Title.Contains(TEXT("STATION")))return TEXT("the old survey station up the track");
  bool bCaps=Title.Len()>3;for(const TCHAR C:Title)if(FChar::IsLower(C)){bCaps=false;break;}
  if(bCaps)return Title.ToLower();
  FString Short=Title;const int32 And=Short.Find(TEXT(" and "));
  if(And>4)Short=Short.Left(And);
  return Short;
 }
 // What the player has done, as Earl has heard of it. Empty: nothing tonight.
 TArray<FItem> Answering(const FS47NightLineIn& In,int32 Hour)
 {
  TArray<FItem> Out;
  const uint32 V=Mix(uint32(In.Seed),uint32(In.Night))%3u;
  // The big night, the three nights after (FS47Happenings: 1420 talks about it). Earl's own words; the callers have theirs.
  if(Hour==20&&In.BigNight>0&&In.Night>In.BigNight&&In.Night<=In.BigNight+3)
  {
   const FString Where=In.BigPlace.IsEmpty()?FString(TEXT("out past the dishes")):FString(TEXT("out toward "))+Said(In.BigPlace);
   static const TCHAR* const Big[]={
    TEXT("Phones haven't quit since the other night. Everybody saw it {where}, and nobody saw the same thing. I'll take your calls. I won't take sides."),
    TEXT("Still getting calls about the other night, {where}. The sheriff's office says to quit calling the sheriff's office."),
    TEXT("A few of you are still asking about the other night, {where}. I don't know any more than you do. Less, probably.")};
   Out.Add(Host(FString::Printf(TEXT("rx_big%d"),FMath::Clamp(In.Night-In.BigNight,1,3)),FString(Big[FMath::Clamp(In.Night-In.BigNight,1,3)-1]).Replace(TEXT("{where}"),*Where)));
  }
  // A tip of the player's that Earl read, the night or two after (FS47RadioTips: TOWN-USE-20261108).
  if(Hour==21&&!In.TipPlace.IsEmpty()&&In.TipNight>0&&In.Night>In.TipNight&&In.Night<=In.TipNight+2)
  {
   static const TCHAR* const Tip[]={
    TEXT("Phones lit up after that note from the dishes about {place}. Most of you saw nothing. One of you saw a coyote. Thank you all the same."),
    TEXT("About that note from out at the dishes, {place}: nobody's called to say they saw a thing. I'm reading that as good news."),
    TEXT("Whoever's working nights at the dishes, folks are still talking about {place}. You've given this town something to do.")};
   Out.Add(Host(TEXT("rx_tip"),FString(Tip[V]).Replace(TEXT("{place}"),*Said(In.TipPlace))));
  }
  // The county's trust (FS47FreeformSession::SheriffTrust): the sheriff's office has a word for the station.
  if(Hour==22&&In.SheriffTrust>=2)Out.Add(Host(TEXT("rx_sheriff"),TEXT("The sheriff's office called to say thank you to whoever's keeping that log out at the observatory. I don't know what that means. I'm passing it on.")));
  // Last night's records (the night log): a time, and which way from the dishes, as the bearing on the map has it.
  if(Hour==22)for(const FS47NightLineIn::FLogged& L:In.Logged)
  {
   const FString When=L.Clock>=0?FString::Printf(TEXT(" a little after %s"),*SafeTime(L.Clock)):FString();
   const FString Way=L.Az>=0?FString::Printf(TEXT(" off to the %s of the dishes"),*S47Hap::Compass(L.Az)):FString(TEXT(" out past the dishes"));
   if(L.Kind==TEXT("landing")){Out.Add(Host(TEXT("rx_land"),FString::Printf(TEXT("Sheriff's office says they had a couple of calls last night about lights coming down low%s,%s. Flares off the range, they figure. That's what they figure."),*Way,*When)));break;}
   if(L.Kind==TEXT("burst")){Out.Add(Host(TEXT("rx_burst"),FString::Printf(TEXT("Some of you lost the TV picture last night%s. Snow for a second, then back. If you live%s, that was you."),*When,*Way)));break;}
   if(L.Kind==TEXT("dark")){Out.Add(Host(TEXT("rx_dark"),FString::Printf(TEXT("Caliche lost the lights last night%s. The co-op says it was a transformer. The co-op always says it was a transformer."),*When)));break;}
  }
  return Out;
 }
}

TArray<TSharedPtr<FS47RadioSeg>> S47NightLine::Hour(const FS47NightLineIn& In,int32 Hour,TArray<FString>* OutIds,TArray<ES47Allowed>* OutTiers)
{
 Hour=FMath::Clamp(Hour,20,27);
 const uint32 Seed=uint32(In.Seed);const int32 Night=FMath::Max(1,In.Night);
 // CRIT9: Earl's recorded "Thirty-eight on the bank sign, and the wind has laid down" only in the midnight hour of a still night
 // whose low is at most thirty-eight (and then the weather in the same hour says thirty-eight right now); the comet only under a sky
 // that is not mostly cloudy. Otherwise the slot's written stand-in (the same slot, so nothing comes twice).
 const S47Weather::FDay WxToday=S47Weather::Day(In.Seed,Night),WxNext=S47Weather::Forecast(In.Seed,Night,Night+1);
 bool bColdSaid=false;
 const auto Earl=[&](int32 K)
 {
  const FPiece& E=EARL[Pick(UE_ARRAY_COUNT(EARL),2,Night,K,Seed,0x4541524cu)];
  if(FCString::Strcmp(E.Id,TEXT("rec_cold"))==0)
  {
   if(Hour==24&&WxToday.NightStorm<=.2f&&WxNext.LowF<=38){bColdSaid=true;return Of(E,ES47Allowed::Hint);}
   return Of(EARL_MILD,ES47Allowed::Hint);
  }
  if(FCString::Strcmp(E.Id,TEXT("rec_comet"))==0&&WxToday.CloudNight>=.5f)return Of(EARL_CLOUD,ES47Allowed::Hint);
  return Of(E,ES47Allowed::Hint);
 };
 const auto Caller=[&](int32 K){return Of(SLEEPLESS[Pick(UE_ARRAY_COUNT(SLEEPLESS),2,Night,K,Seed,0x534c4550u)],ES47Allowed::Hint);};
 // In the hour the tier allows, and never earlier: before midnight a hint, the hour from one a glimpse, from two what came close.
 const auto Strange=[&](const FPiece* List,int32 N,ES47Allowed Tier,uint32 Salt){return Of(List[Pick(N,1,Night,0,Seed,Salt)],Tier);};

 FHourPlan P;
 // The hour's talk, in order, with a record or two after each.
 TArray<FItem> Talk;
 if(Hour==20)Talk.Add(HostAt(TEXT("op_on"),TEXT("Good evening. This is fourteen-twenty, Caliche, New Mexico, and this is the Night Line. I'm Earl, and I'm with you till four. It's {time}.")));
 else if(Hour==24)
 {
  // Earl's recorded call, the words his recording has (the time is his own recorded time words).
  FItem Call;Call.Id=TEXT("op_rec");
  Call.Lines.Add(FS47RadioLine(TEXT("This is the Night Line on fourteen-twenty, Caliche. We are on till four."),TEXT("host")));
  Call.Lines.Add(FS47RadioLine([](double C){return FString(TEXT("It's "))+SafeTime(C)+FString(TEXT(" in Caliche."));},TEXT("host")));
  Talk.Add(Call);
 }
 else{const int32 V=int32(Mix(Seed^0x43414c4cu,uint32(Night*32+Hour))%UE_ARRAY_COUNT(CALLS));Talk.Add(HostAt(FString::Printf(TEXT("op%d"),V),CALLS[V]));}
 Talk.Append(Answering(In,Hour));
 switch(Hour)
 {
  case 20:Talk.Add(Notice(In,0));Talk.Add(Weather(In,Hour));Talk.Add(Earl(0));break;
  case 21:Talk.Add(Caller(0));Talk.Add(Road(In,0));Talk.Add(Strange(STRANGE_HINT,UE_ARRAY_COUNT(STRANGE_HINT),ES47Allowed::Hint,0x48494e54u));break;
  case 22:Talk.Add(Notice(In,1));Talk.Add(Weather(In,Hour));break;
  case 23:Talk.Add(Road(In,1));break;
  case 24:Talk.Add(Earl(1));Talk.Add(Weather(In,Hour,bColdSaid?38:-1));break;
  case 25:Talk.Add(Strange(STRANGE_GLIMPSE,UE_ARRAY_COUNT(STRANGE_GLIMPSE),ES47Allowed::Glimpse,0x474c494du));break;
  case 26:Talk.Add(Strange(STRANGE_FULL,UE_ARRAY_COUNT(STRANGE_FULL),ES47Allowed::Full,0x46554c4cu));Talk.Add(Weather(In,Hour));break;
  default:Talk.Add(Caller(1));break;
 }
 // A stretch of static once or twice a night: always in one of the late hours, sometimes before midnight too.
 const int32 QuietHour=24+int32(Mix(Seed^0x51484f55u,uint32(Night))%3u);
 const bool bQuiet=Hour==QuietHour||(Hour==22&&Roll(Seed,Night,0x51323232u)<.5);
 int32 K=0;
 for(int32 I=0;I<Talk.Num();++I)
 {
  AddTalk(P,Talk[I]);
  AddRecord(P,In,Hour,K++,I>0);
  if(I==1&&bQuiet)AddQuiet(P,In,Hour);
  const bool bIntro=(K&1)==0;AddRecord(P,In,Hour,K++,bIntro);
 }
 // The last hour: toward its end, Earl says what is left.
 if(Hour==27){while(P.Secs<2700){const bool bIntro=(K%3)!=2;AddRecord(P,In,Hour,K++,bIntro);}AddTalk(P,Host(TEXT("op_late"),TEXT("Getting on toward four. A few more records, and then the anthem.")));}
 // Records to the end of the hour and a little past it: the next hour's call cuts in at its own time.
 while(P.Secs<3900){const bool bIntro=(K%3)!=2;AddRecord(P,In,Hour,K++,bIntro);}
 if(OutIds)*OutIds=P.Ids;
 if(OutTiers)*OutTiers=P.Tiers;
 return P.Segs;
}

void S47NightLine::ReadLogged(const FString& Text,double& OutClock,double& OutAz)
{
 OutClock=-1;OutAz=-1;
 FRegexMatcher T(FRegexPattern(TEXT("^At ([0-9]{2}):([0-9]{2})")),Text);
 if(T.FindNext())OutClock=FCString::Atoi(*T.GetCaptureGroup(1))*3600.+FCString::Atoi(*T.GetCaptureGroup(2))*60.;
 FRegexMatcher A(FRegexPattern(TEXT("about ([0-9]{1,3}) degrees from SARO")),Text);
 if(A.FindNext())OutAz=FCString::Atod(*A.GetCaptureGroup(1));
}

bool S47NightLine::Fill(const US47Main* M,FS47NightLineIn& In)
{
 if(!M||!M->IsFreeform()||!M->Freeform)return false;
 const FS47FreeformSession& F=*M->Freeform;
 In=FS47NightLineIn();
 In.Night=FMath::Max(1,F.Day());In.Seed=F.Seed();In.SheriffTrust=F.SheriffTrust();In.TownTrust=F.TownTrust();
 if(F.Happenings.IsValid()){In.BigNight=FMath::Max(0,F.Happenings->BigLatest());In.BigPlace=F.Happenings->BigPlaceTitle();} // CHAIN-4-20261109: the latest round's
 if(F.Tips.IsValid())for(const FS47RadioTips::FGiven& G:F.Tips->Given())
 {
  if(G.Stage<1||G.ReadDay<=0||G.ReadDay>=In.Night||G.ReadDay<In.TipNight)continue;
  In.TipNight=G.ReadDay;
  In.TipPlace=F.EventPlaces.IsValid()?F.EventPlaces->TitleOf(G.Place):FS47Places::Def(G.Place)?FString(FS47Places::Def(G.Place)->Title):FString();
  if(In.TipPlace==TEXT("somewhere out there"))In.TipPlace.Empty();
 }
 // Last night's records, by the keys the night's events keep (FS47Happenings).
 const int32 Last=In.Night-1;
 const TPair<const TCHAR*,const TCHAR*> Keys[]={{TEXT("landing_seen_d"),TEXT("landing")},{TEXT("landing_logged_d"),TEXT("landing")},{TEXT("array_burst_d"),TEXT("burst")},{TEXT("blackout_d"),TEXT("dark")}};
 if(Last>=1)for(const auto& Key:Keys)
 {
  const FString Id=FString(Key.Key)+FString::FromInt(Last);
  if(!F.HasFinding(Id))continue;
  FS47NightLineIn::FLogged L;L.Kind=Key.Value;ReadLogged(F.FindingText(Id),L.Clock,L.Az);
  if(!In.Logged.ContainsByPredicate([&L](const FS47NightLineIn::FLogged& X){return X.Kind==L.Kind;}))In.Logged.Add(L);
 }
 return true;
}

bool S47NightLine::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const FString& Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s night line: %s"),B?TEXT("PASS"):TEXT("FAIL"),*Why));};
 // The pools are long enough for two weeks without a repeat (Count a night each).
 Check(UE_ARRAY_COUNT(EARL)>=28&&UE_ARRAY_COUNT(SLEEPLESS)>=28&&UE_ARRAY_COUNT(STRANGE_HINT)>=14&&UE_ARRAY_COUNT(STRANGE_GLIMPSE)>=14&&UE_ARRAY_COUNT(STRANGE_FULL)>=14&&UE_ARRAY_COUNT(NOTICES)>=28,
  TEXT("every list holds two weeks of nights without saying anything twice"));
 // Words a voice on the air never says, beyond the night events' own filter: the radio tips' (no British spelling, no make).
 static const TCHAR* const NotSaid[]={TEXT("kerb"),TEXT("petrol"),TEXT("lorry"),TEXT("kinski"),TEXT("tyre"),TEXT("tyres"),TEXT("grey"),TEXT("colour"),TEXT("programme"),TEXT("cadillac"),TEXT("chevy"),TEXT("ford"),TEXT("jeep")};
 const auto Plain=[](const FString& Text)
 {
  if(!S47Hap::Clean(Text))return false;
  FString L=Text.ToLower(),W;TArray<FString> Words;
  for(const TCHAR C:L){if(FChar::IsAlpha(C))W.AppendChar(C);else if(!W.IsEmpty()){Words.Add(W);W.Reset();}}
  if(!W.IsEmpty())Words.Add(W);
  for(const FString& X:Words)for(const TCHAR* Bad:NotSaid)if(X==Bad)return false;
  for(const TCHAR* Far:{TEXT("alien"),TEXT("ufo"),TEXT("saucer"),TEXT("abduct"),TEXT("flying")})if(L.Contains(Far))return false;
  return Text.Len()<260;
 };
 for(const int32 Seed:{470001,1986,90210})
 {
  TSet<FString> Nights;TMap<FString,int32> FirstNight;bool bRepeat=false,bTier=true,bClean=true,bStrange=true;FString Repeat,Tier,Dirty;
  for(int32 Night=1;Night<=30;++Night)
  {
   FS47NightLineIn In;In.Night=Night;In.Seed=Seed;FString Whole;int32 Glimpses=0,Fulls=0;
   for(int32 H=20;H<=27;++H)
   {
    TArray<FString> Ids;TArray<ES47Allowed> Tiers;const auto Segs=Hour(In,H,&Ids,&Tiers);
    for(int32 I=0;I<Ids.Num();++I)
    {
     Whole+=Ids[I]+TEXT(" ");
     // Before midnight only hints; nothing ever holds more than its hour allows.
     const ES47Allowed Allowed=S47NightTiers::At(FMath::Fmod(double(H),24.)+.01,Night);
     if(uint8(Tiers[I])>uint8(Allowed)||(H<24&&Tiers[I]!=ES47Allowed::Hint)){bTier=false;Tier=FString::Printf(TEXT("%s at %d, night %d"),*Ids[I],H,Night);}
     Glimpses+=Tiers[I]==ES47Allowed::Glimpse;Fulls+=Tiers[I]==ES47Allowed::Full;
     // The written ones (Earl's, the callers', the town's and the roads') never twice in the first two weeks.
     const FString& Id=Ids[I];
     const bool bWritten=Id.StartsWith(TEXT("e"))||Id.StartsWith(TEXT("c"))||Id.StartsWith(TEXT("h"))||Id.StartsWith(TEXT("g"))||Id.StartsWith(TEXT("f"))||Id.StartsWith(TEXT("rec_"))||Id.StartsWith(TEXT("an"))||Id.StartsWith(TEXT("rd"));
     if(bWritten&&Night<=14)
     {
      if(const int32* Was=FirstNight.Find(Id)){bRepeat=true;Repeat=FString::Printf(TEXT("%s on nights %d and %d"),*Id,*Was,Night);}
      else FirstNight.Add(Id,Night);
     }
    }
    for(const auto& S:Segs)for(const FS47RadioLine& L:S->Lines)for(const double At:{H*3600.+600,H*3600.+47*60})
    {const FString Text=L.At(FMath::Fmod(At,86400.));if(!Plain(Text)){bClean=false;Dirty=Text.Left(80);}}
   }
   bStrange&=Glimpses==1&&Fulls==1;
   Nights.Add(Whole);
  }
  Check(Nights.Num()==30,FString::Printf(TEXT("seed %d: no two of nights 1 to 30 have the same programme"),Seed));
  Check(!bRepeat,FString::Printf(TEXT("seed %d: nothing written is said twice before the third week%s"),Seed,bRepeat?*(TEXT(": ")+Repeat):TEXT("")));
  Check(bTier,FString::Printf(TEXT("seed %d: before midnight only hints, a far glimpse from one, what came close from two%s"),Seed,bTier?TEXT(""):*(TEXT(": ")+Tier)));
  Check(bStrange,FString::Printf(TEXT("seed %d: one far glimpse and one close call a night, no more"),Seed));
  Check(bClean,FString::Printf(TEXT("seed %d: the text filter lets every line through, the time checks too%s"),Seed,bClean?TEXT(""):*(TEXT(": ")+Dirty)));
 }
 // Every written line, whatever night it would come on.
 {
  bool bAll=true;FString Bad;
  const auto Each=[&](const FPiece* List,int32 N){for(int32 I=0;I<N;++I)for(const TCHAR* T:{List[I].T1,List[I].T2,List[I].T3})if(T&&!Plain(T)){bAll=false;Bad=FString(T).Left(80);}};
  Each(EARL,UE_ARRAY_COUNT(EARL));Each(&EARL_MILD,1);Each(&EARL_CLOUD,1);Each(SLEEPLESS,UE_ARRAY_COUNT(SLEEPLESS));Each(STRANGE_HINT,UE_ARRAY_COUNT(STRANGE_HINT));Each(STRANGE_GLIMPSE,UE_ARRAY_COUNT(STRANGE_GLIMPSE));Each(STRANGE_FULL,UE_ARRAY_COUNT(STRANGE_FULL));
  for(const TCHAR* T:NOTICES)for(const TCHAR* D:NOTICE_DAYS)if(!Plain(FString(T).Replace(TEXT("{day}"),D))){bAll=false;Bad=FString(T).Left(80);}
  // CRIT9: a day is only a day, so a notice never has two times of day ("Thursday at seven ... First pitch at four").
  for(const TCHAR* D:NOTICE_DAYS)for(const TCHAR* W:{TEXT(" at "),TEXT("morning"),TEXT("afternoon"),TEXT("evening"),TEXT("night")})if(FString(D).Contains(W)){bAll=false;Bad=D;}
  for(const TCHAR* R:ROADS)for(const TCHAR* C:ROAD_IS)if(!Plain(FString(R)+TEXT(" ")+C)){bAll=false;Bad=FString(R)+TEXT(" ")+C;}
  for(const FRecord& R:RECORDS)for(const TCHAR* I:INTROS)if(!Plain(FString(I).Replace(TEXT("{title}"),R.Title).Replace(TEXT("{artist}"),R.Artist))){bAll=false;Bad=R.Title;}
  for(int32 N=0;N<110;++N)bAll&=!Number(N).Contains(TEXT("forty-seven"));
  Check(bAll,FString(TEXT("the text filter lets through every written line, notice, road, record and number"))+(bAll?TEXT(""):*(TEXT(": ")+Bad)));
 }
 // CRIT9: Earl's recorded cold line never against the weather of its hour, the comet never under a mostly cloudy night.
 {
  bool bFits=true;FString Why;
  for(const int32 Seed:{470001,1986,90210,7,123})for(int32 Night=1;Night<=40;++Night)for(const int32 H:{20,24})
  {
   FS47NightLineIn In;In.Night=Night;In.Seed=Seed;TArray<FString> Ids;const auto Segs=Hour(In,H,&Ids);
   const S47Weather::FDay T=S47Weather::Day(Seed,Night),Nx=S47Weather::Forecast(Seed,Night,Night+1);
   if(Ids.Contains(TEXT("rec_cold")))
   {
    bool bNow38=false;for(const auto& S:Segs)for(const FS47RadioLine& L:S->Lines)bNow38|=L.At(0).Contains(TEXT("thirty-eight degrees in Caliche right now"));
    if(H!=24||T.NightStorm>.2f||Nx.LowF>38||!bNow38){bFits=false;Why=FString::Printf(TEXT("rec_cold, seed %d night %d at %d"),Seed,Night,H);}
   }
   if(Ids.Contains(TEXT("rec_comet"))&&T.CloudNight>=.5f){bFits=false;Why=FString::Printf(TEXT("rec_comet under cloud, seed %d night %d"),Seed,Night);}
  }
  Check(bFits,FString(TEXT("Earl's recorded cold night and comet only on a night that is so"))+(bFits?TEXT(""):*(TEXT(": ")+Why)));
 }
 // What the player has done comes back on the air.
 {
  FS47NightLineIn In;In.Night=5;In.Seed=1986;In.SheriffTrust=2;In.BigNight=3;In.BigPlace=TEXT("the windmill on the Baca lease");In.TipPlace=TEXT("the old mine");In.TipNight=4;
  FS47NightLineIn::FLogged L;L.Kind=TEXT("landing");L.Clock=2*3600+14*60;L.Az=63;In.Logged.Add(L);
  TArray<FString> All;FString Land;
  for(int32 H=20;H<=27;++H){TArray<FString> Ids;const auto Segs=Hour(In,H,&Ids);All.Append(Ids);if(H==22)for(const auto& S:Segs)for(const FS47RadioLine& X:S->Lines)if(X.At(0).Contains(TEXT("lights coming down")))Land=X.At(0);}
  Check(All.Contains(TEXT("rx_big2"))&&All.Contains(TEXT("rx_tip"))&&All.Contains(TEXT("rx_sheriff"))&&All.ContainsByPredicate([](const FString& Id){return Id.EndsWith(TEXT("+trust"));}),
   TEXT("the big night, a tip Earl read and the county's trust are talked of the nights after"));
  Check(Land.Contains(TEXT("north-east"))&&Land.Contains(TEXT("two fourteen"))&&Plain(Land),TEXT("last night's landing comes back with its time and its side of the dishes, as the log has them"));
  FS47NightLineIn None;None.Night=5;None.Seed=1986;TArray<FString> Quiet;for(int32 H=20;H<=27;++H){TArray<FString> Ids;Hour(None,H,&Ids);Quiet.Append(Ids);}
  Check(!Quiet.ContainsByPredicate([](const FString& Id){return Id.StartsWith(TEXT("rx_"))||Id.EndsWith(TEXT("+trust"));}),TEXT("with nothing done, nothing said about it"));
  double C=0,A=0;ReadLogged(TEXT("At 02:14 lights came down out on the land, about 63 degrees from SARO, give or take 12, and went out on the ground."),C,A);
  Check(FMath::IsNearlyEqual(C,2*3600.+14*60)&&FMath::IsNearlyEqual(A,63.),TEXT("a night log line's time and bearing are read back"));
 }
 // The hours: on from eight in the evening, off from the sign-off at four.
 Check(OnAir(20*3600)&&OnAir(23.9*3600)&&OnAir(0)&&OnAir(3.9*3600)&&!OnAir(4*3600)&&!OnAir(12*3600)&&!OnAir(19.9*3600),TEXT("1420 is on from eight in the evening to four"));
 {
  FS47NightLineIn In;In.Night=2;In.Seed=7;double Secs=0;
  for(const auto& S:Hour(In,23))Secs+=S->Kind==TEXT("talk")?2:S->Len;
  Check(Secs>=3600,TEXT("an hour's programme fills its hour"));
 }
 return OK;
}

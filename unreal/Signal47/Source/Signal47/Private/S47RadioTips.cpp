#include "S47RadioTips.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47Places.h"
#include "S47EventPlaces.h"
#include "S47TownUse.h"
#include "S47Townsfolk.h"
#include "S47Visitors.h"
#include "S47CB.h"
#include "S47FieldRadio.h"
#include "S47Radio.h"
#include "S47RadioVoice.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47World.h"
#include "Misc/Crc.h"

// Written for April 1986: drivers, ranch hands and night callers in Socorro County. Nobody real. Short, dry, and most of it about
// nothing. A place id means it points somewhere; Needs is a finding the player must already hold, for the ones that talk about
// the player; Trust is how well the county has to know the station before anybody says it.
const TArray<FS47RadioTips::FLine>& FS47RadioTips::CbLines()
{
 static const TArray<FLine> L={
  // The county on nineteen, spring 1986: the road, the weather, the cattle. None of it means anything.
  {TEXT("cb37"),TEXT("Northbound, you got a county mounty with radar at the turnoff. Keep it on the double nickel."),nullptr,nullptr,0},
  {TEXT("cb38"),TEXT("Brand inspector's set up at the Y checking trailers. Have your papers, or have a real good story."),nullptr,nullptr,0},
  {TEXT("cb39"),TEXT("Rattlers are out on the blacktop after dark, soaking up the heat. Counted three on the county road. Watch where you step."),nullptr,nullptr,0},
  {TEXT("cb40"),TEXT("Rain's hanging out of that cloud over the Magdalenas like a curtain. Not a drop of it getting down here."),nullptr,nullptr,0},
  {TEXT("cb41"),TEXT("Ladies are selling tamales out of a cooler by the church after Mass Sunday. Four dollars a dozen, red or green. Get there early."),nullptr,nullptr,0},
  {TEXT("cb42"),TEXT("Following a cattle truck into Socorro and the wind's coming off it. If I go quiet, that's why."),nullptr,nullptr,0},
  {TEXT("cb43"),TEXT("Anybody headed back from Socorro? My daughter needs a corsage for the prom Saturday, and that's the closest florist there is."),nullptr,nullptr,0},
  {TEXT("cb01"),TEXT("Break one-nine. Anybody northbound, there's a county car sitting at the grain elevator with his lights off."),nullptr,nullptr,0},
  {TEXT("cb02"),TEXT("You got the Tumbleweed here, hauling hay to Magdalena. Coffee's gone and I'm talking to myself."),nullptr,nullptr,0},
  {TEXT("cb03"),TEXT("Eastbound on sixty, you've got a mattress in the left lane about mile ninety. Queen size."),nullptr,nullptr,0},
  {TEXT("cb04"),TEXT("Anybody got a radio check? ... Well, I hear you fine. Who is this?"),nullptr,nullptr,0},
  {TEXT("cb05"),TEXT("Diner in Caliche quit serving pie at ten. I'm just telling you so you don't get your hopes up."),nullptr,nullptr,0},
  {TEXT("cb06"),TEXT("Gas is ninety-one a gallon at the Y and it's a dollar two at the place by the church. Same gas."),nullptr,nullptr,0},
  {TEXT("cb07"),TEXT("Wind's quit up on the flats. You can hear your own tires."),nullptr,nullptr,0},
  {TEXT("cb08"),TEXT("Lost my mud flap around the Rio Salado bridge. If you see it, it's got a lady on it. Keep it."),nullptr,nullptr,0},
  {TEXT("cb09"),TEXT("Ten-four, it's a cabover, not a long-nose, and I'll thank you to remember that."),nullptr,nullptr,0},
  {TEXT("cb10"),TEXT("Elk on the road past Datil. Big one. Not moving for anybody."),nullptr,nullptr,0},
  {TEXT("cb11"),TEXT("My wife says it's going to be a wet spring. My wife has not been right about the weather in her life."),nullptr,nullptr,0},
  {TEXT("cb12"),TEXT("Anybody know a tire man open this side of Socorro? Anybody? ... That's what I figured."),nullptr,nullptr,0},
  {TEXT("cb13"),TEXT("Seen that comet yet? I got out and looked. Looks like a thumbprint on a window."),nullptr,nullptr,0},
  {TEXT("cb14"),TEXT("Cold front's supposed to come through Friday. They said that last Friday."),nullptr,nullptr,0},
  {TEXT("cb15"),TEXT("Whoever's stepping on channel nineteen with that heater, knock it off. Nobody wants to hear your carrier."),nullptr,nullptr,0},
  {TEXT("cb16"),TEXT("Radio's picking up a station out of Juarez clear as a bell tonight. Must be the air."),nullptr,nullptr,0},
  {TEXT("cb17"),TEXT("Next time you pass that yellow house at the junction, wave. She's up all night and she waves back."),nullptr,nullptr,0},
  {TEXT("cb18"),TEXT("I'm parked, I'm eating a sandwich. That's the report."),nullptr,nullptr,0},
  {TEXT("cb52"),TEXT("Did the whole channel just drop out for everybody, or just me? Not static. Nothing."),nullptr,nullptr,0},
  {TEXT("cb53"),TEXT("Somebody's painting arrows on the blacktop south of the turnoff. White, every half mile or so, all of them pointing back the way I came."),nullptr,nullptr,0},
  {TEXT("cb54"),TEXT("Coyotes are quiet tonight. Most nights they carry on till sunup."),nullptr,nullptr,0},
  {TEXT("cb55"),TEXT("What time's everybody got? My watch says one thing and the dash clock says another."),nullptr,nullptr,0},
  {TEXT("cb56"),TEXT("There's a bear taking pictures southbound past Lemitar. Back it down, drivers."),nullptr,nullptr,0},
  {TEXT("cb19"),TEXT("Somebody's wired the tail on the Baca windmill so she won't turn. Why would a man do that?"),TEXT("windmill"),nullptr,0},
  {TEXT("cb20"),TEXT("Went by the old mine road. Somebody took the boards off. Stacked them real nice."),TEXT("mine"),nullptr,0},
  {TEXT("cb21"),TEXT("There's a dead cow up against the fence south of the turnoff and nothing's been at her. Been three days I've been looking at that cow."),TEXT("cow"),nullptr,0},
  {TEXT("cb22"),TEXT("Fella at the feed store says there's a stake with a flag on it off the survey track, brand new. Nobody's surveying anything."),TEXT("stake"),nullptr,0},
  {TEXT("cb23"),TEXT("Saw a big rag of orange cloth out east of the road, near that stock trail. Weather balloon or a real ugly parachute."),TEXT("balloon"),nullptr,0},
  {TEXT("cb24"),TEXT("Cattle won't go near that salt trough down the Tres Lagunas side. I'd move it, but it isn't mine."),TEXT("trough"),nullptr,0},
  {TEXT("cb25"),TEXT("You been up to that tank on the old grade? Full of water and not a track around it."),TEXT("tank"),nullptr,0},
  {TEXT("cb26"),TEXT("Somebody's been up at the line shack. There was a light in it Tuesday. Hasn't been a line rider up there in fifteen years."),TEXT("shack"),nullptr,0},
  {TEXT("cb27"),TEXT("That rock pile up on the rise has gotten taller. I go by it every week. Two rocks higher, maybe three."),TEXT("cairn"),nullptr,0},
  {TEXT("cb28"),TEXT("That white truck from the observatory was out at the windmill. I'm not saying anything. It's a free country."),nullptr,TEXT("place_windmill"),0},
  {TEXT("cb29"),TEXT("You the new one from SARO? Somebody saw you up at the old mine. Bring a coat, is all I'll say."),nullptr,TEXT("place_mine"),0},
  {TEXT("cb30"),TEXT("Heard somebody from the antennas went looking at that cow. Well? ... No, I guess you wouldn't say."),nullptr,TEXT("place_cow"),0},
  {TEXT("cb31"),TEXT("Saw somebody on foot out past the fence the other night, no truck anywhere near. That you? You all right?"),nullptr,TEXT("lost_ground"),0},
  {TEXT("cb32"),TEXT("Deputy says you people at SARO write a good letter. He's been driving out your way. Thought you'd want to know."),nullptr,nullptr,1},
  // Talk the day after: what somebody else saw, or says they saw. As often wrong as right.
  {TEXT("cb44"),TEXT("Used to be three, four rigs sleeping at the pull-off north of the dishes. Lately everybody pushes on through."),nullptr,nullptr,0},
  {TEXT("cb45"),TEXT("Anybody pick up a lady walking south of the cattle guard last night? I didn't stop. I'd like somebody to tell me they stopped."),nullptr,nullptr,0},
  {TEXT("cb46"),TEXT("Something lit up the wash under the west mesa about three and then quit. Flare off the range, I guess. Long way north for a flare."),nullptr,nullptr,0},
  {TEXT("cb47"),TEXT("My nephew swears the sky over the playa went white for a second last night. My nephew also drives with one headlight."),nullptr,nullptr,0},
  {TEXT("cb48"),TEXT("Fella at the Mesa's looking for his brother-in-law, a schoolteacher. Took his camper out on the flats a week ago. Cream, brown stripe."),nullptr,nullptr,0},
  // Said to the player only for what the player has done: a place found (Needs), or a station the county knows (Trust).
  {TEXT("cb49"),TEXT("Heard one of you from the dishes was down at that salt trough. Cattle still won't go near it. How close did you get?"),nullptr,TEXT("place_trough"),0},
  {TEXT("cb50"),TEXT("Hear you've been up to that rock pile on the rise. Don't take any rocks off it. I mean that."),nullptr,TEXT("place_cairn"),0},
  {TEXT("cb51"),TEXT("You're the dish people that keep a log? Then put this in it: lights over the Hondo road Sunday, about two, low and slow."),nullptr,nullptr,1},
  // Once you have opened the locker next to yours, the air has more to say about who had it (FS47Predecessor).
  {TEXT("cb35"),TEXT("Anybody with a scanner picking up an open carrier on the service band out off the old grade? Been sitting there for weeks. Nobody ever comes on it."),nullptr,TEXT("reyes_locker"),0},
  {TEXT("cb36"),TEXT("That white district wagon's still out on the flat, county tag on the mirror. I slowed down. I didn't get out."),nullptr,TEXT("reyes_locker"),0},
  // What the town has seen the player buy (S47TownUse): somebody always notices.
  {TEXT("cb57"),TEXT("Somebody's setting out salt blocks on open range, nowhere near a tank. Cattle are walking off the lease after them."),nullptr,TEXT("shop:lure"),0},
  {TEXT("cb59"),TEXT("Old Baca's dog is out on the county road all by itself. Anybody been up to his place lately?"),nullptr,TEXT("visit:baca_gone"),0},
  {TEXT("cb60"),TEXT("That painted van with the Sky Watch people is out on the flats with the side door open. Nobody in it. Nobody."),nullptr,TEXT("visit:van_empty"),0},
  {TEXT("cb61"),TEXT("Two black sedans at first light, government plates, parked off the road like they owned it. Didn't wave."),nullptr,TEXT("visit:military"),0},
  {TEXT("cb62"),TEXT("Helicopter came over real low at sunup, no markings I could see. Cattle went every which way."),nullptr,TEXT("visit:military_seen"),0},
  {TEXT("cb58"),TEXT("White pickup from the dishes has a little rubber spaceman on the dash. Waved at me. The spaceman, I mean."),nullptr,TEXT("shop:spaceman"),0},
  {TEXT("cb33"),TEXT("You got a friend at the sheriff's now? Ask him who else has been asking about that survey track."),TEXT("stake"),nullptr,1},
  {TEXT("cb34"),TEXT("Between you and me, that tank's not on the county's plat. Nobody knows who put it there. Ask anybody over sixty."),TEXT("tank"),nullptr,2},
 };
 return L;
}
const TArray<FS47RadioTips::FCall>& FS47RadioTips::AmCalls()
{
 static const TArray<FCall> L={
  // Caliche on the phone: the stew, the boosters, the frost. Most calls are this.
  {TEXT("am23"),TEXT("woman"),TEXT("The Mesa's got the green chile stew back. Thursdays only, and she says don't come in asking on a Wednesday."),TEXT("I came in asking on a Wednesday."),nullptr,nullptr,0},
  {TEXT("am24"),TEXT("woman"),TEXT("The high school boosters are washing cars at the gas station Saturday, two dollars. It's for new lights on the football field."),TEXT("For two dollars they can have my truck. They'll be sorry."),nullptr,nullptr,0},
  {TEXT("am25"),TEXT("old"),TEXT("Don't anybody put their chile in yet. There was frost on my windshield this morning."),TEXT("After Mother's Day, my mother always said. Not a day before."),nullptr,nullptr,0},
  {TEXT("am26"),TEXT("man"),TEXT("Branding at the Sedillo place Saturday. They want two who can rope and one who can cook, and not the same fella for both."),TEXT("Then I'm cooking."),nullptr,nullptr,0},
  {TEXT("am27"),TEXT("woman"),TEXT("The souvenir place has postcards of the dishes now, a quarter apiece. I bought one and I can't think who to send it to."),TEXT("Send it to the dishes."),nullptr,nullptr,0},
  {TEXT("am28"),TEXT("man"),TEXT("Drove by the Plaza about one in the morning and the doors were flickering like there was a picture on. Nothing shows past ten."),TEXT("Then somebody fell asleep in the booth."),nullptr,nullptr,0},
  {TEXT("am29"),TEXT("woman"),TEXT("There's a light on in that old house at the south end of town again. Nobody's lived there since the Montoyas went to Belen."),TEXT("Kids, I'd guess. It's usually kids."),nullptr,nullptr,0},
  {TEXT("am01"),TEXT("man"),TEXT("Earl, can you tell the fella with the blue pickup he left his lunch at the elevator."),TEXT("He heard you. Everybody heard you."),nullptr,nullptr,0},
  {TEXT("am02"),TEXT("woman"),TEXT("Earl, the church rummage sale is Saturday now, not Friday."),TEXT("Saturday. Bring your own bags."),nullptr,nullptr,0},
  {TEXT("am03"),TEXT("old"),TEXT("I've got a question about the comet. Is it coming back?"),TEXT("Seventy-six years, they tell me. I wouldn't wait up."),nullptr,nullptr,0},
  {TEXT("am04"),TEXT("man"),TEXT("My neighbor's rooster goes off at three in the morning. Is there an ordinance?"),TEXT("Not for roosters."),nullptr,nullptr,0},
  {TEXT("am05"),TEXT("woman"),TEXT("I just wanted to say the music's been nice tonight."),TEXT("Well. It's been the same record for a while."),nullptr,nullptr,0},
  {TEXT("am06"),TEXT("old"),TEXT("Used to be you could see the lights of Magdalena from my porch."),TEXT("Used to be there were lights in Magdalena."),nullptr,nullptr,0},
  {TEXT("am07"),TEXT("man"),TEXT("Anybody selling a good used stock trailer, I'm at the Y."),TEXT("Y's a big place, friend, but all right."),nullptr,nullptr,0},
  {TEXT("am08"),TEXT("woman"),TEXT("The lady at the bank says the sign's been stuck on thirty-eight since Sunday."),TEXT("I'd been wondering why it was so steady."),nullptr,nullptr,0},
  {TEXT("am09"),TEXT("man"),TEXT("Earl, my horses won't come in to the barn tonight. Stood out at the far fence till I quit calling them."),TEXT("Could be a coyote. Could be they just like the fence."),nullptr,nullptr,0},
  {TEXT("am10"),TEXT("old"),TEXT("We had rain in '41 you wouldn't believe. Washed the road right out."),TEXT("I believe you, Pete."),nullptr,nullptr,0},
  {TEXT("am11"),TEXT("woman"),TEXT("There's a cat at the post office. Orange. If it's yours, come get it."),TEXT("It's nobody's, Ruth. It's been there since the flood."),nullptr,nullptr,0},
  {TEXT("am12"),TEXT("man"),TEXT("There's a gate standing open on the observatory's service road. Cattle are going to find it before they do."),TEXT("Well, they've heard now."),nullptr,nullptr,0},
  {TEXT("am13"),TEXT("man"),TEXT("That windmill on the Baca lease isn't turning. Hasn't got a thing wrong with it. It's wired off."),TEXT("Maybe they don't need the water."),TEXT("windmill"),nullptr,0},
  {TEXT("am14"),TEXT("old"),TEXT("My brother worked that mine on the west side. He always said they shut it for no good reason. Now somebody's opened it up."),TEXT("Your brother said a lot of things."),TEXT("mine"),nullptr,0},
  {TEXT("am15"),TEXT("woman"),TEXT("You know the rock pile on the rise past the Alderete road? You can see all the way to those dishes from there. I don't like it."),TEXT("It's a rock pile."),TEXT("cairn"),nullptr,0},
  {TEXT("am16"),TEXT("man"),TEXT("There's a cow dead by the fence, past the turnoff. Vet won't come look. Says he's busy."),TEXT("Vets are busy."),TEXT("cow"),nullptr,0},
  {TEXT("am17"),TEXT("old"),TEXT("Line shack up there has got a tally on the door. I never made that tally and I'm the last one who worked it."),TEXT("How long ago was that?"),TEXT("shack"),nullptr,0},
  {TEXT("am18"),TEXT("woman"),TEXT("Earl, I hear somebody from out at the dishes has been driving around the back roads at night. Should we be worried?"),TEXT("I expect they got turned around."),nullptr,TEXT("place_tank"),0},
  {TEXT("am19"),TEXT("man"),TEXT("The new one at the dishes went out to that balloon, I hear. It's just a balloon. I could have told them it's just a balloon."),TEXT("Well, now they know."),nullptr,TEXT("place_balloon"),0},
  {TEXT("am20"),TEXT("woman"),TEXT("Somebody ought to check on whoever's working nights at SARO. Their truck went by my place at four in the morning, slow, like they didn't know the road."),TEXT("I'm sure they know about it."),nullptr,TEXT("lost_clock"),0},
  {TEXT("am21"),TEXT("man"),TEXT("I'll say it on the air: the deputy's been out by the station every other night, and I'd like to know what for."),TEXT("You could ask him."),nullptr,nullptr,1},
  {TEXT("am22"),TEXT("old"),TEXT("Seeing as the SARO people listen in now. There was a survey party out here when I was a boy. Never said what for. Their line ran the way those dishes point."),TEXT("Thank you, Pete. Go to bed."),TEXT("stake"),nullptr,2},
  {TEXT("am30"),TEXT("old"),TEXT("Earl, the one who was out at the dishes before this one, the quiet one. Anybody hear anything?"),TEXT("Not that I've heard, and I hear most."),nullptr,TEXT("reyes_locker"),0},
  {TEXT("am31"),TEXT("woman"),TEXT("That girl from the district, with the wagon. They ever find her?"),TEXT("No, ma'am. Next caller."),nullptr,TEXT("reyes_locker"),0},
  {TEXT("am32"),TEXT("woman"),TEXT("Earl, somebody left a whole box of canning jars on my step tonight, washed, lids on. If they're yours, I'm keeping them."),TEXT("That's the nicest thing I've heard all week."),nullptr,nullptr,0},
  {TEXT("am33"),TEXT("old"),TEXT("Not a breath of wind out here, and the porch swing's going. Slow. I watched it a good while."),TEXT("Go on back in the house, Pete."),nullptr,nullptr,0},
  {TEXT("am34"),TEXT("man"),TEXT("Anybody know a farrier this side of Magdalena? Mine moved to Belen."),TEXT("Anybody? Phones are open."),nullptr,nullptr,0},
  // What the night left behind, as the callers have it. Hearsay; Earl believes none of it out loud.
  {TEXT("am40"),TEXT("man"),TEXT("Feed and Seed's out of salt blocks again. Third week running. Who in this county is buying up all the salt?"),TEXT("Not me. I'd like to know too."),nullptr,nullptr,0},
  {TEXT("am41"),TEXT("old"),TEXT("There's water running in the arroyo out past the Y. Hasn't rained since March. I'd like to know where it's coming from."),TEXT("Snowmelt off the high country, Pete. Most likely."),nullptr,nullptr,0},
  {TEXT("am42"),TEXT("woman"),TEXT("Father says somebody's been lighting candles in the church after he locks up. Paying for them, too. Exact change in the box."),TEXT("Father's a patient man."),nullptr,nullptr,0},
  {TEXT("am35"),TEXT("old"),TEXT("Coyotes out under the east rim have quit. Three nights now. I didn't know I listened for them."),nullptr,nullptr,nullptr,0},
  {TEXT("am36"),TEXT("old"),TEXT("That man they were looking for out on the flats. Anybody hear if he turned up?"),TEXT("If he did, he hasn't called me."),nullptr,nullptr,0},
  {TEXT("am37"),TEXT("old"),TEXT("My dad used to say the old-timers never built a house facing that flat out past the dishes. He never said why, and I never asked."),TEXT("Thank you. I think."),nullptr,nullptr,2},
  {TEXT("am38"),TEXT("old"),TEXT("Somebody from the dishes found that flag stake, they tell me. So who put it there? You people ought to know."),TEXT("Maybe they don't."),nullptr,TEXT("place_stake"),0},
  {TEXT("am50"),TEXT("woman"),TEXT("Those Sky Watch people were in the Mesa all morning telling everybody what they saw last night. I'd like my booth back."),TEXT("They'll be on next week, I expect."),nullptr,TEXT("visit:van_back"),0},
  {TEXT("am43"),TEXT("man"),TEXT("The souvenir man's got a new picture in his window. Lights over the flats, he says, taken by somebody from the dishes."),TEXT("I'll go look. I won't believe it, but I'll look."),nullptr,TEXT("shop:sold_odd"),0},
  {TEXT("am44"),TEXT("old"),TEXT("Took the souvenir man's tour Sunday. Five dollars to look at a rock. He says it's the rock."),TEXT("It's a nice rock, Pete."),nullptr,TEXT("shop:tour"),0},
  {TEXT("am45"),TEXT("woman"),TEXT("My sister in Belen got a postcard of the dishes from somebody who works out there. She wants to know is it safe."),TEXT("Tell her it's a postcard."),nullptr,TEXT("shop:postcard"),0},
  // One of the town's own gone (S47Townsfolk).
  {TEXT("am46"),TEXT("woman"),TEXT("Earl, the deputy's car hasn't been in front of the substation in a couple of days and nobody answers the phone there."),TEXT("I'll call Socorro for you."),nullptr,TEXT("town:away_deputy"),0},
  {TEXT("am47"),TEXT("man"),TEXT("Gas station's got a card in the night window. BACK SOON. It's been up a good while now."),TEXT("He'll turn up. They do."),nullptr,TEXT("town:away_attendant"),0},
  {TEXT("am48"),TEXT("old"),TEXT("The shop man's not on his stoop. Thirty years he's sat on that stoop, Earl."),TEXT("I noticed, Pete."),nullptr,TEXT("town:away_souvenir_man"),0},
  {TEXT("am49"),TEXT("woman"),TEXT("The Mesa's short a waitress this week. The cook's doing both and he is not happy about it."),TEXT("Tip him, then."),nullptr,TEXT("town:away_waitress"),0},
  // And back again (S47Townsfolk: "back_", three days after): the town notices that too, and what is not right about it.
  {TEXT("am51"),TEXT("man"),TEXT("Deputy's car was back out front of the substation this morning. He didn't wave. He always waves."),TEXT("Give him a day."),nullptr,TEXT("town:back_deputy"),0},
  {TEXT("am52"),TEXT("man"),TEXT("Gas station's open again. He gave me my change twice and never noticed. I gave it back."),TEXT("Good man."),nullptr,TEXT("town:back_attendant"),0},
  {TEXT("am53"),TEXT("old"),TEXT("The shop man's back on his stoop. Sat there all morning with his hat in his lap. Thirty years I never saw him without it on."),TEXT("I saw that too, Pete."),nullptr,TEXT("town:back_souvenir_man"),0},
  {TEXT("am54"),TEXT("woman"),TEXT("The waitress is back at the Mesa. Poured my coffee this morning and kept pouring after the cup was full. Just stood there."),TEXT("Be kind to her."),nullptr,TEXT("town:back_waitress"),0},
  {TEXT("am39"),TEXT("woman"),TEXT("I live in the yellow house at the junction. The last one on nights at the dishes waved every morning on the way by. I still look up when that truck comes past."),nullptr,nullptr,TEXT("reyes_locker"),0},
 };
 return L;
}
void FS47RadioTips::Begin(const S47J::FObj& Saved)
{
 Used.Reset();Tipped.Reset();CbIn=FMath::FRandRange(30.f,60.f);AmIn=FMath::FRandRange(40.f,80.f);
 for(const FString& S:S47J::Strs(Saved,TEXT("said")))if(!S.IsEmpty()&&S.Len()<16)Used.Add(S);
 for(const auto& V:S47J::Arr(Saved,TEXT("given")))
 {
  if(!V||V->Type!=EJson::Object||Tipped.Num()>=64)continue;const auto O=V->AsObject();FGiven G;
  G.Place=S47J::Str(O,TEXT("place")).Left(64);G.Day=FMath::Clamp(S47J::Int(O,TEXT("day")),0,1000000);G.Stage=FMath::Clamp(S47J::Int(O,TEXT("stage")),0,2);
  G.ReadDay=FMath::Clamp(S47J::Int(O,TEXT("read")),0,1000000);G.Back=S47J::Str(O,TEXT("back")).Left(400);G.BackPlace=S47J::Str(O,TEXT("backPlace")).Left(64);
  if(!G.Place.IsEmpty()&&G.Day>0&&!Gave(G.Place))Tipped.Add(G);
 }
}
S47J::FObj FS47RadioTips::Save() const
{
 auto O=S47J::New();if(Used.IsEmpty()&&Tipped.IsEmpty())return O;
 if(!Used.IsEmpty()){TArray<FString> A=Used.Array();A.Sort();S47J::Set(O,TEXT("said"),A);}
 if(!Tipped.IsEmpty())
 {
  TArray<TSharedPtr<FJsonValue>> List;
  for(const FGiven& G:Tipped)
  {
   auto X=S47J::New();S47J::Set(X,TEXT("place"),G.Place);S47J::Set(X,TEXT("day"),G.Day);S47J::Set(X,TEXT("stage"),G.Stage);S47J::Set(X,TEXT("read"),G.ReadDay);
   if(!G.Back.IsEmpty())S47J::Set(X,TEXT("back"),G.Back);if(!G.BackPlace.IsEmpty())S47J::Set(X,TEXT("backPlace"),G.BackPlace);
   List.Add(MakeShared<FJsonValueObject>(X));
  }
  O->SetArrayField(TEXT("given"),List);
 }
 return O;
}
bool FS47RadioTips::Ready(const TCHAR* Needs,int32 Trust) const
{
 auto& F=*M->Freeform;
 if(Trust>F.TownTrust())return false;
 // Something the town has seen the player do (S47TownUse::Seen): "shop:lure" and the like.
 if(Needs&&FString(Needs).StartsWith(TEXT("shop:")))return F.TownUse&&F.TownUse->Seen(FString(Needs).Mid(5));
 // One of Caliche's regulars gone (S47Townsfolk::Has): "town:away_deputy" and the like.
 if(Needs&&FString(Needs).StartsWith(TEXT("town:")))return F.Townsfolk&&F.Townsfolk->Has(FString(Needs).Mid(5));
 // From outside town (S47Visitors::Has): "visit:baca_gone", "visit:military" and the like.
 if(Needs&&FString(Needs).StartsWith(TEXT("visit:")))return F.Visitors&&F.Visitors->Has(FString(Needs).Mid(6));
 return !Needs||F.HasFinding(Needs);
}
void FS47RadioTips::Step(double Dt)
{
 if(!M||!M->Freeform||!M->World||!M->World->bDriving||M->ModalOpen()||!M->Freeform->RulesLive())return;
 CbIn-=Dt;AmIn-=Dt;
 if(CbIn<=0){CbIn=FMath::FRandRange(70.f,150.f);StepCb();}
 if(AmIn<=0){AmIn=FMath::FRandRange(90.f,180.f);StepAm();}
}
void FS47RadioTips::StepCb()
{
 // Only where there is a CB: the truck, or the base set in the SARO building. Never on foot.
 if(!FS47FieldRadio::CbAtHand(M))return;
 // Several that mean nothing for each that does; each line once in a game, until there are none left.
 TArray<const FLine*> Plain,Telling;
 for(const FLine& L:CbLines())
 {
  if(Used.Contains(L.Id)||!Ready(L.Needs,L.Trust))continue;
  if(L.Place&&M->Freeform->Places&&(M->Freeform->Places->Found(L.Place)||M->Freeform->Places->Heard(L.Place)))continue;
  (L.Place||L.Needs?Telling:Plain).Add(&L);
 }
 const bool bTell=!Telling.IsEmpty()&&(Plain.IsEmpty()||FMath::FRand()<.2f+.1f*M->Freeform->TownTrust());
 const auto& Pool=bTell?Telling:Plain;if(Pool.IsEmpty())return;
 const FLine* L=Pool[FMath::RandRange(0,Pool.Num()-1)];
 // CB-WALKIE-20261108: out of the CB's own speaker and on its HUD line, in a driver's voice, and said once it has really begun: a
 // call cut off before its first word can come again. Busy with another call: soon again.
 static const TCHAR* const Drivers[]={TEXT("man"),TEXT("old"),TEXT("man"),TEXT("woman")};
 const FString SaidId=L->Id;const TWeakObjectPtr<US47Main> WeakMain=M;
 const auto Begun=[WeakMain,SaidId]{if(WeakMain.IsValid()&&WeakMain->Freeform&&WeakMain->Freeform->Tips)WeakMain->Freeform->Tips->MarkSaid(SaidId);};
 if(FS47FieldRadio::CbSay(M,{FString(L->Text)},Drivers[FCrc::StrCrc32(L->Id)%4],Begun))return;
 if(M->CB.IsValid()&&M->CB->bBusy){CbIn=FMath::Min(CbIn,12.);return;}
 // No sound to play it with: the words.
 if(M->UI)M->UI->Toast(FString(TEXT("CB: "))+L->Text,7);
 MarkSaid(SaidId);
}
void FS47RadioTips::MarkSaid(const FString& Id)
{
 if(!M||!M->Freeform||Used.Contains(Id))return;
 const FLine* L=CbLines().FindByPredicate([&Id](const FLine& Each){return Id==Each.Id;});
 if(!L)return;
 Used.Add(Id);
 if(L->Place&&M->Freeform->Places)M->Freeform->Places->Hear(L->Place);
 UE_LOG(LogTemp,Display,TEXT("S47: CB %s%s"),L->Id,L->Place?TEXT(" (points somewhere)"):TEXT(""));
 M->RequestAutosave();
}
void FS47RadioTips::StepAm()
{
 // Only with the truck's radio on 1420, and in its own turn on the station's programme.
 if(!M->CarRadio||!M->CarRadio->Live()||M->CarRadio->Station!=TEXT("caliche")||!M->Radio||!M->Story)return;
 const auto* Found=M->Radio->Stations.Find(TEXT("caliche"));if(!Found||!Found->IsValid())return;
 // A tip of the player's comes first, in its own turn (TOWN-USE-20261108).
 if(StepTipped(**Found))return;
 TArray<const FCall*> Plain,Telling;
 for(const FCall& C:AmCalls())
 {
  if(Used.Contains(C.Id)||!Ready(C.Needs,C.Trust))continue;
  (C.Place||C.Needs?Telling:Plain).Add(&C);
 }
 const bool bTell=!Telling.IsEmpty()&&(Plain.IsEmpty()||FMath::FRand()<.25f+.1f*M->Freeform->TownTrust());
 const auto& Pool=bTell?Telling:Plain;if(Pool.IsEmpty())return;
 const FCall* C=Pool[FMath::RandRange(0,Pool.Num()-1)];
 TArray<FS47RadioLine> Lines;
 Lines.Add(FS47RadioLine(FString(C->Text),FString(C->Who),TEXT("a caller")));
 if(C->Answer&&*C->Answer)Lines.Add(FS47RadioLine(FString(C->Answer),TEXT("host")));
 if(!(*Found)->Slot(Lines,M->Story->Clock()))return; // Not its turn: it tries again later, with nothing spent.
 Used.Add(C->Id);
 if(C->Place&&M->Freeform->Places)M->Freeform->Places->Hear(C->Place);
 UE_LOG(LogTemp,Display,TEXT("S47: 1420 AM %s%s"),C->Id,C->Place?TEXT(" (points somewhere)"):TEXT(""));
 M->RequestAutosave();
}
bool FS47RadioTips::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s radio tips: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 int32 Plain=0,Telling=0;TSet<FString> Ids;bool bPlaces=true,bClean=true,bPlainWords=true;FString Offender;
 // The text critic, 8 November: every word of the lines as they are said, the callers' and Earl's answers too. No make of car or
 // drink, and the county's own American spelling.
 static const TCHAR* const NotSaid[]={TEXT("chevy"),TEXT("chevrolet"),TEXT("ford"),TEXT("dodge"),TEXT("gmc"),TEXT("jeep"),TEXT("winnebago"),TEXT("cadillac"),TEXT("coke"),TEXT("pepsi"),TEXT("kinski"),TEXT("tyre"),TEXT("tyres"),TEXT("windscreen"),TEXT("grey"),TEXT("programme"),TEXT("colour"),TEXT("kerb"),TEXT("petrol"),TEXT("lorry")};
 const auto Words=[](const FString& Said)
 {
  TArray<FString> Out;FString Word;
  for(const TCHAR Ch:Said.ToLower()){if(FChar::IsAlpha(Ch))Word.AppendChar(Ch);else if(!Word.IsEmpty()){Out.Add(Word);Word.Reset();}}
  if(!Word.IsEmpty())Out.Add(Word);
  return Out;
 };
 const auto Look=[&](const TCHAR* Id,const TCHAR* Text,const TCHAR* Answer,const TCHAR* Place,const TCHAR* Needs)
 {
  Ids.Add(Id);(Place||Needs?Telling:Plain)+=1;
  if(Place)bPlaces&=FS47Places::Def(Place)!=nullptr;
  for(const TCHAR* Said:{Text,Answer})
  {
   if(!Said)continue;
   const FString T=FString(Said).ToLower();
   for(const TCHAR* W:{TEXT("alien"),TEXT("ufo"),TEXT("flying saucer"),TEXT("abduct")})bClean&=!T.Contains(W);
   bClean&=T.Len()<190;
   for(const FString& Word:Words(T))for(const TCHAR* Bad:NotSaid)if(Word==Bad){bPlainWords=false;Offender=FString(Bad)+TEXT(" in ")+Id;}
  }
 };
 for(const FLine& L:CbLines())Look(L.Id,L.Text,nullptr,L.Place,L.Needs);
 for(const FCall& C:AmCalls())Look(C.Id,C.Text,C.Answer,C.Place,C.Needs);
 Check(bPlainWords,TEXT("no make of car or drink, and no British spelling, in anything said on the CB or on 1420"));
 if(!bPlainWords)Lines.Add(FString(TEXT("FAIL radio tips: "))+Offender);
 Check(Ids.Num()==CbLines().Num()+AmCalls().Num(),TEXT("every line has its own id, so that it comes once"));
 Check(Plain>Telling,TEXT("more of them mean nothing than something"));
 Check(bPlaces,TEXT("what points somewhere points at a place that exists"));
 Check(bClean,TEXT("short, and nobody says what it is"));
 TSet<FString> Pointed;for(const FLine& L:CbLines())if(L.Place)Pointed.Add(L.Place);for(const FCall& C:AmCalls())if(C.Place)Pointed.Add(C.Place);
 Check(Pointed.Num()>=8,TEXT("all the places can be heard of"));
 FS47RadioTips None(nullptr);None.Begin(nullptr);Check(None.Save()->Values.Num()==0,TEXT("an older save has said nothing and writes nothing"));
 // TOWN-USE-20261108: a tip given, read and called back about survives a Continue, and Earl's words never name the thing.
 {
  auto T=S47J::New();TArray<TSharedPtr<FJsonValue>> G;
  auto A=S47J::New();S47J::Set(A,TEXT("place"),TEXT("windmill"));S47J::Set(A,TEXT("day"),3);S47J::Set(A,TEXT("stage"),1);S47J::Set(A,TEXT("read"),3);G.Add(MakeShared<FJsonValueObject>(A));
  auto B=S47J::New();S47J::Set(B,TEXT("place"),TEXT("windmill"));S47J::Set(B,TEXT("day"),4);G.Add(MakeShared<FJsonValueObject>(B));
  T->SetArrayField(TEXT("given"),G);
  FS47RadioTips Tip(nullptr);Tip.Begin(S47J::Read(S47J::Write(T)));
  Check(Tip.Given().Num()==1&&Tip.Gave(TEXT("windmill"))&&Tip.Given()[0].Stage==1&&Tip.Given()[0].ReadDay==3,TEXT("a tip given to 1420 survives a Continue, each place once"));
  FS47RadioTips Again(nullptr);Again.Begin(S47J::Read(S47J::Write(Tip.Save())));
  Check(Again.Given().Num()==1&&S47J::Write(Again.Save())==S47J::Write(Tip.Save()),TEXT("and is written again the same"));
  const FString Air=OnAirLine(TEXT("the windmill on the Baca lease")).ToLower();
  Check(Air.Contains(TEXT("windmill"))&&!Air.Contains(TEXT("alien"))&&!Air.Contains(TEXT("ufo"))&&Air.Len()<220,TEXT("Earl reads a tip with its place and without saying what it was"));
 }
 auto O=S47J::New();S47J::Set(O,TEXT("said"),TArray<FString>{TEXT("am02"),TEXT("cb01")});FS47RadioTips Back(nullptr);Back.Begin(S47J::Read(S47J::Write(O)));Check(Back.Said().Num()==2&&S47J::Write(Back.Save())==S47J::Write(O),TEXT("what was said stays said after a Continue"));
 return OK;
}

bool FS47RadioTips::DevCb()
{
 // Said at once with no sound, or begun on the CB (marked said as its first word comes).
 const int32 Before=Used.Num();const bool bWasBusy=M&&M->CB.IsValid()&&M->CB->bBusy;
 if(M&&M->Freeform)StepCb();
 return Used.Num()>Before||(!bWasBusy&&M&&M->CB.IsValid()&&M->CB->bBusy);
}

bool FS47RadioTips::DevAm()
{
 const int32 Before=Used.Num();
 if(M&&M->Freeform)StepAm();
 return Used.Num()>Before;
}

// ---------- TOWN-USE-20261108: the player's tips to 1420 AM ----------

FString FS47RadioTips::TitleOf(const FString& Place) const
{
 if(M&&M->Freeform&&M->Freeform->EventPlaces)return M->Freeform->EventPlaces->TitleOf(Place);
 if(const FS47Places::FDef* D=FS47Places::Def(Place))return D->Title;
 return TEXT("somewhere out there");
}

bool FS47RadioTips::CanGive() const
{
 if(!M||!M->Freeform)return false;
 const int32 Today=M->Freeform->Day();
 return !Tipped.ContainsByPredicate([Today](const FGiven& G){return G.Day==Today;});
}

bool FS47RadioTips::Give(const FString& Place)
{
 if(!M||!M->Freeform||Place.IsEmpty()||Gave(Place)||!CanGive()||Tipped.Num()>=64)return false;
 FGiven G;G.Place=Place.Left(64);G.Day=M->Freeform->Day();Tipped.Add(G);
 UE_LOG(LogTemp,Display,TEXT("S47: 1420 tip given: %s, day %d"),*G.Place,G.Day);
 M->RequestAutosave();
 return true;
}

FString FS47RadioTips::OnAirLine(const FString& Title)
{
 // Earl as he is with the callers: dry, and nothing in it that says what anybody thinks it was.
 return FString::Printf(TEXT("Got a note here from somebody who works nights out at the dishes. Something going on out by %s, they say, late. Anybody else out that way, phones are open."),*Title);
}

FString FS47RadioTips::CallBack(const FGiven& G,FString& OutPlace,FString& OutAnswer,FString* OutVoice) const
{
 OutPlace.Empty();OutAnswer.Empty();
 const FString Tip=TitleOf(G.Place);
 const uint32 Salt=FCrc::StrCrc32(*G.Place)+uint32(G.Day);
 // Never back at the place the tip was about (it may itself be one nobody has told the player of yet).
 if(M&&M->Freeform&&M->Freeform->Places)for(int32 Try=0;Try<8&&(OutPlace.IsEmpty()||OutPlace==G.Place);++Try)OutPlace=M->Freeform->Places->NextUnknown(int32((Salt+uint32(Try)*7919u)%100000u));
 if(OutPlace==G.Place)OutPlace.Empty();
 const int32 Pick=int32(Salt%3u);
 if(!OutPlace.IsEmpty())
 {
  const FString New=TitleOf(OutPlace);
  static const TCHAR* const Lines[]={
   TEXT("That caller from the dishes, about {tip}. I didn't see a thing there. You want to see something, go look at {new}. I'm not saying more on the radio."),
   TEXT("Earl, about {tip}. My husband saw a light that same night, only it was out by {new}. He won't talk about it, so I am."),
   TEXT("Whoever called about {tip}. Ask them if they've been out to {new}. I go by it every week and I don't stop anymore.")};
  static const TCHAR* const Answers[]={TEXT("Well. There you go."),TEXT("Thank you, ma'am. Phones are open."),TEXT("I'll pass it along.")};
  // The voice goes with the words: a woman about her husband (and "ma'am" back), a man for the other two.
  static const TCHAR* const LineVoices[]={TEXT("man"),TEXT("woman"),TEXT("old")};
  OutAnswer=Answers[Pick];if(OutVoice)*OutVoice=LineVoices[Pick];
  return FString(Lines[Pick]).Replace(TEXT("{tip}"),*Tip).Replace(TEXT("{new}"),*New);
 }
 static const TCHAR* const Nothing[]={
  TEXT("About {tip}. I drove by in the morning. Nothing there. Tire tracks, is all, and I expect those were theirs."),
  TEXT("Earl, about {tip}. My dog wouldn't go out that night, if that's worth anything."),
  TEXT("About {tip}: I was up all night with the baby and I'd have seen it. I didn't see it.")};
 // The drive by is a man's; the dog an old man's; up with the baby, a young mother.
 static const TCHAR* const NothingVoices[]={TEXT("man"),TEXT("old"),TEXT("woman")};
 OutAnswer=TEXT("Well, now we know.");if(OutVoice)*OutVoice=NothingVoices[Pick];
 return FString(Nothing[Pick]).Replace(TEXT("{tip}"),*Tip);
}

void FS47RadioTips::Back(FGiven& G)
{
 FString Where,Answer;
 G.Back=CallBack(G,Where,Answer);G.BackPlace=Where;G.Stage=2;
 if(!Where.IsEmpty()&&M&&M->Freeform&&M->Freeform->Places)M->Freeform->Places->Hear(Where);
 UE_LOG(LogTemp,Display,TEXT("S47: 1420 tip %s called back about: %s"),*G.Place,Where.IsEmpty()?TEXT("nothing new"):*Where);
 if(M)M->RequestAutosave();
}

bool FS47RadioTips::StepTipped(FS47Station& Station)
{
 if(!M||!M->Freeform||!M->Story)return false;
 const int32 Today=M->Freeform->Day();
 for(FGiven& G:Tipped)
 {
  TArray<FS47RadioLine> Lines;
  if(G.Stage==0)Lines.Add(FS47RadioLine(OnAirLine(TitleOf(G.Place)),TEXT("host")));
  else if(G.Stage==1&&Today>G.ReadDay)
  {
   FString Where,Answer,Voice;const FString Said=CallBack(G,Where,Answer,&Voice);
   Lines.Add(FS47RadioLine(Said,Voice.IsEmpty()?FString(TEXT("man")):Voice,TEXT("a caller")));
   Lines.Add(FS47RadioLine(Answer,TEXT("host")));
  }
  else continue;
  if(!Station.Slot(Lines,M->Story->Clock()))return true; // Not its turn: it waits, and nothing else takes the turn meant for it.
  // M7 (Tom, 9 November): 1420 pays five dollars for a tip it reads, inside the week's cap on side money (FS47FreeformSession::EarnSide)
  if(G.Stage==0)
  {
   G.Stage=1;G.ReadDay=Today;const int32 Paid=M->Freeform->EarnSide(TipPay,TEXT("1420 tip"));UE_LOG(LogTemp,Display,TEXT("S47: 1420 tip %s read on the air, $%d"),*G.Place,Paid);
   // CRIT6-20261109: what the tip paid is said, as the photo credit and the souvenir copies say theirs (one set of words for the cap).
   if(M->UI)M->UI->Toast(Paid>=TipPay?FString::Printf(TEXT("1420 / Earl reads your tip: $%d from the station."),Paid)
    :Paid>0?FString::Printf(TEXT("1420 / Earl reads your tip: $%d of the $%d, and with that this week's field money is used up."),Paid,int32(TipPay))
    :FString(TEXT("1420 / Earl reads your tip. This week's field money is used up: no payment.")),5);
   M->RequestAutosave();
  }
  else Back(G);
  return true;
 }
 return false;
}

FString FS47RadioTips::TakeNotes()
{
 if(!M||!M->Freeform)return FString();
 const int32 Today=M->Freeform->Day();
 TArray<FString> Notes;
 for(FGiven& G:Tipped)
 {
  // Read on the air or not, by the second day the call-back has come in; nobody heard it in the truck, so it is on Earl's spike.
  if(G.Stage>=2||Today<G.Day+2)continue;
  const bool bWasRead=G.Stage==1;
  Back(G);
  // Earl's own words, with the caller's inside them in single quotes; one paragraph a tip (S47TownUse frames them).
  Notes.Add(FString::Printf(TEXT("%sSomebody called back, said '%s'"),bWasRead?TEXT(""):TEXT("Read your note on the air. "),*G.Back));
 }
 return FString::Join(Notes,TEXT("\n\n"));
}

FString FS47RadioTips::DevNext()
{
 if(!M||!M->Freeform)return FString();
 for(FGiven& G:Tipped)
 {
  if(G.Stage==0){G.Stage=1;G.ReadDay=M->Freeform->Day()-1;const FString Air=OnAirLine(TitleOf(G.Place));UE_LOG(LogTemp,Display,TEXT("S47: 1420 tip %s read on the air (F10)"),*G.Place);return TEXT("EARL: ")+Air;}
  if(G.Stage==1){Back(G);return TEXT("CALLER: ")+G.Back;}
 }
 return FString();
}

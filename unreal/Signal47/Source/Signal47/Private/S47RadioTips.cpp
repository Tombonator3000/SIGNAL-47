#include "S47RadioTips.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47Places.h"
#include "S47Radio.h"
#include "S47RadioVoice.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47World.h"

// Written for April 1986: drivers, ranch hands and night callers in Socorro County. Nobody real. Short, dry, and most of it about
// nothing. A place id means it points somewhere; Needs is a finding the player must already hold, for the ones that talk about
// the player; Trust is how well the county has to know the station before anybody says it.
const TArray<FS47RadioTips::FLine>& FS47RadioTips::CbLines()
{
 static const TArray<FLine> L={
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
  {TEXT("cb19"),TEXT("Somebody's wired the tail on the Baca windmill so she won't turn. Why would a man do that?"),TEXT("windmill"),nullptr,0},
  {TEXT("cb20"),TEXT("Went by the old mine road. Somebody took the boards off. Stacked them real nice."),TEXT("mine"),nullptr,0},
  {TEXT("cb21"),TEXT("There's a dead cow up against the fence south of the turnoff and nothing's been at her. Been three days I've been looking at that cow."),TEXT("cow"),nullptr,0},
  {TEXT("cb22"),TEXT("Fella at the feed store says there's a stake with a flag on it off the survey track, brand new. Nobody's surveying anything."),TEXT("stake"),nullptr,0},
  {TEXT("cb23"),TEXT("Saw a big rag of orange cloth out east of the road, near that stock trail. Weather balloon or a real ugly parachute."),TEXT("balloon"),nullptr,0},
  {TEXT("cb24"),TEXT("Cattle won't go near that salt trough down the Tres Lagunas side. I'd move it, but it isn't mine."),TEXT("trough"),nullptr,0},
  {TEXT("cb25"),TEXT("You been up to that tank on the old grade? Full of water and not a track around it."),TEXT("tank"),nullptr,0},
  {TEXT("cb26"),TEXT("Somebody's been up at the line shack. There was a light in it Tuesday. Hasn't been a line rider up there in fifteen years."),TEXT("shack"),nullptr,0},
  {TEXT("cb27"),TEXT("That rock pile up on the rise has got taller. I go by it every week. I know what I'm saying."),TEXT("cairn"),nullptr,0},
  {TEXT("cb28"),TEXT("That white truck from the observatory was out at the windmill. I'm not saying anything. It's a free country."),nullptr,TEXT("place_windmill"),0},
  {TEXT("cb29"),TEXT("You the SARO fella? Somebody saw you up at the old mine. Bring a coat, is all I'll say."),nullptr,TEXT("place_mine"),0},
  {TEXT("cb30"),TEXT("Heard somebody from the antennas went looking at that cow. Well? ... No, I guess you wouldn't say."),nullptr,TEXT("place_cow"),0},
  {TEXT("cb31"),TEXT("Saw you walking out past the fence the other night with no truck anywhere near. You all right?"),nullptr,TEXT("lost_ground"),0},
  {TEXT("cb32"),TEXT("Deputy says you people at SARO write a good letter. He's been driving out your way. Thought you'd want to know."),nullptr,nullptr,1},
  {TEXT("cb33"),TEXT("You got a friend at the sheriff's now? Ask him who else has been asking about that survey track."),TEXT("stake"),nullptr,1},
  {TEXT("cb34"),TEXT("Between you and me, that tank's not on the county's plat. Nobody knows who put it there. Ask anybody over sixty."),TEXT("tank"),nullptr,2},
 };
 return L;
}
const TArray<FS47RadioTips::FCall>& FS47RadioTips::AmCalls()
{
 static const TArray<FCall> L={
  {TEXT("am01"),TEXT("man"),TEXT("Earl, can you tell the fella with the blue pickup he left his lunch at the elevator."),TEXT("He heard you. Everybody heard you."),nullptr,nullptr,0},
  {TEXT("am02"),TEXT("woman"),TEXT("Earl, the church rummage sale is Saturday now, not Friday."),TEXT("Saturday. Bring your own bags."),nullptr,nullptr,0},
  {TEXT("am03"),TEXT("old"),TEXT("I've got a question about the comet. Is it coming back?"),TEXT("Seventy-six years, they tell me. I wouldn't wait up."),nullptr,nullptr,0},
  {TEXT("am04"),TEXT("man"),TEXT("My neighbor's rooster goes off at three in the morning. Is there an ordinance?"),TEXT("Not for roosters."),nullptr,nullptr,0},
  {TEXT("am05"),TEXT("woman"),TEXT("I just wanted to say the music's been nice tonight."),TEXT("Well. It's been the same record for a while."),nullptr,nullptr,0},
  {TEXT("am06"),TEXT("old"),TEXT("Used to be you could see the lights of Magdalena from my porch."),TEXT("Used to be there were lights in Magdalena."),nullptr,nullptr,0},
  {TEXT("am07"),TEXT("man"),TEXT("Anybody selling a good used stock trailer, I'm at the Y."),TEXT("Y's a big place, friend, but all right."),nullptr,nullptr,0},
  {TEXT("am08"),TEXT("woman"),TEXT("The lady at the bank says the sign's been stuck on thirty-eight since Sunday."),TEXT("I'd been wondering why it was so steady."),nullptr,nullptr,0},
  {TEXT("am09"),TEXT("man"),TEXT("Earl, my cattle are bunched up against the north fence and they won't graze."),TEXT("Could be a coyote. Could be they just like the fence."),nullptr,nullptr,0},
  {TEXT("am10"),TEXT("old"),TEXT("We had rain in '41 you wouldn't believe. Washed the road right out."),TEXT("I believe you, Pete."),nullptr,nullptr,0},
  {TEXT("am11"),TEXT("woman"),TEXT("There's a cat at the post office. Orange. If it's yours, come get it."),TEXT("It's nobody's, Ruth. It's been there since the flood."),nullptr,nullptr,0},
  {TEXT("am12"),TEXT("man"),TEXT("Somebody ought to tell those observatory people they left a gate open out on the service road."),TEXT("Somebody just did."),nullptr,nullptr,0},
  {TEXT("am13"),TEXT("man"),TEXT("That windmill on the Baca lease isn't turning. Hasn't got a thing wrong with it. It's wired off."),TEXT("Maybe they don't need the water."),TEXT("windmill"),nullptr,0},
  {TEXT("am14"),TEXT("old"),TEXT("My brother worked that mine on the west side. He always said they shut it for no good reason. Now somebody's opened it up."),TEXT("Your brother said a lot of things."),TEXT("mine"),nullptr,0},
  {TEXT("am15"),TEXT("woman"),TEXT("You know the rock pile on the rise past the Alderete road? You can see all the way to those dishes from there. I don't like it."),TEXT("It's a rock pile."),TEXT("cairn"),nullptr,0},
  {TEXT("am16"),TEXT("man"),TEXT("There's a cow dead by the fence, past the turnoff. Vet won't come look. Says he's busy."),TEXT("Vets are busy."),TEXT("cow"),nullptr,0},
  {TEXT("am17"),TEXT("old"),TEXT("Line shack up there has got a tally on the door. I never made that tally and I'm the last one who worked it."),TEXT("How long ago was that?"),TEXT("shack"),nullptr,0},
  {TEXT("am18"),TEXT("woman"),TEXT("Earl, I hear somebody from out at the dishes has been driving around the back roads at night. Should we be worried?"),TEXT("I expect he's lost."),nullptr,TEXT("place_tank"),0},
  {TEXT("am19"),TEXT("man"),TEXT("That young fella from the station asked me about the balloon. It's just a balloon. I told him it's just a balloon."),TEXT("And what did he say?"),nullptr,TEXT("place_balloon"),0},
  {TEXT("am20"),TEXT("woman"),TEXT("Somebody ought to check on whoever's working nights at SARO. I saw their truck sitting out with the door open and nobody in it."),TEXT("I'm sure they know about it."),nullptr,TEXT("lost_clock"),0},
  {TEXT("am21"),TEXT("man"),TEXT("I'll say it on the air: the deputy's been out by the station every other night, and I'd like to know what for."),TEXT("You could ask him."),nullptr,nullptr,1},
  {TEXT("am22"),TEXT("old"),TEXT("Since you've got that station people listening. There was a survey party out here in forty-seven. They never did say what for. Same bearing as those dishes now."),TEXT("Thank you, Pete. Go to bed."),TEXT("stake"),nullptr,2},
 };
 return L;
}
void FS47RadioTips::Begin(const S47J::FObj& Saved)
{
 Used.Reset();CbIn=FMath::FRandRange(30.f,60.f);AmIn=FMath::FRandRange(40.f,80.f);
 for(const FString& S:S47J::Strs(Saved,TEXT("said")))if(!S.IsEmpty()&&S.Len()<16)Used.Add(S);
}
S47J::FObj FS47RadioTips::Save() const
{
 auto O=S47J::New();if(Used.IsEmpty())return O;
 TArray<FString> A=Used.Array();A.Sort();S47J::Set(O,TEXT("said"),A);return O;
}
bool FS47RadioTips::Ready(const TCHAR* Needs,int32 Trust) const
{
 auto& F=*M->Freeform;
 if(Trust>F.TownTrust())return false;
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
 Used.Add(L->Id);
 if(M->UI)M->UI->Toast(FString(TEXT("CB: "))+L->Text,7);
 if(L->Place&&M->Freeform->Places)M->Freeform->Places->Hear(L->Place);
 UE_LOG(LogTemp,Display,TEXT("S47: CB %s%s"),L->Id,L->Place?TEXT(" (points somewhere)"):TEXT(""));
 M->RequestAutosave();
}
void FS47RadioTips::StepAm()
{
 // Only with the truck's radio on 1420, and in its own turn on the station's programme.
 if(!M->CarRadio||!M->CarRadio->Live()||M->CarRadio->Station!=TEXT("caliche")||!M->Radio||!M->Story)return;
 const auto* Found=M->Radio->Stations.Find(TEXT("caliche"));if(!Found||!Found->IsValid())return;
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
 int32 Plain=0,Telling=0;TSet<FString> Ids;bool bPlaces=true,bClean=true;
 const auto Look=[&](const TCHAR* Id,const TCHAR* Text,const TCHAR* Place,const TCHAR* Needs)
 {
  Ids.Add(Id);(Place||Needs?Telling:Plain)+=1;
  if(Place)bPlaces&=FS47Places::Def(Place)!=nullptr;
  const FString T=FString(Text).ToLower();
  for(const TCHAR* W:{TEXT("alien"),TEXT("ufo"),TEXT("flying saucer"),TEXT("abduct")})bClean&=!T.Contains(W);
  bClean&=T.Len()<190;
 };
 for(const FLine& L:CbLines())Look(L.Id,L.Text,L.Place,L.Needs);
 for(const FCall& C:AmCalls())Look(C.Id,C.Text,C.Place,C.Needs);
 Check(Ids.Num()==CbLines().Num()+AmCalls().Num(),TEXT("every line has its own id, so that it comes once"));
 Check(Plain>Telling,TEXT("more of them mean nothing than something"));
 Check(bPlaces,TEXT("what points somewhere points at a place that exists"));
 Check(bClean,TEXT("short, and nobody says what it is"));
 TSet<FString> Pointed;for(const FLine& L:CbLines())if(L.Place)Pointed.Add(L.Place);for(const FCall& C:AmCalls())if(C.Place)Pointed.Add(C.Place);
 Check(Pointed.Num()>=8,TEXT("all the places can be heard of"));
 FS47RadioTips None(nullptr);None.Begin(nullptr);Check(None.Save()->Values.Num()==0,TEXT("an older save has said nothing and writes nothing"));
 auto O=S47J::New();S47J::Set(O,TEXT("said"),TArray<FString>{TEXT("am02"),TEXT("cb01")});FS47RadioTips Back(nullptr);Back.Begin(S47J::Read(S47J::Write(O)));Check(Back.Said().Num()==2&&S47J::Write(Back.Save())==S47J::Write(O),TEXT("what was said stays said after a Continue"));
 return OK;
}

bool FS47RadioTips::DevCb()
{
 const int32 Before=Used.Num();
 if(M&&M->Freeform)StepCb();
 return Used.Num()>Before;
}

bool FS47RadioTips::DevAm()
{
 const int32 Before=Used.Num();
 if(M&&M->Freeform)StepAm();
 return Used.Num()>Before;
}

#include "S47Happenings.h"
#include "S47Audio.h"
#include "S47Drive.h"
#include "S47FarmSite.h"
#include "S47EventPlaces.h"
#include "S47FieldRadio.h"
#include "S47Flow.h"
#include "S47Flashlight.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47NightRhythm.h"
#include "S47Omen.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47Radio.h"
#include "S47Rhythm.h"
#include "S47SaroNight.h"
#include "S47SaroPower.h"
#include "S47ScriptPlayer.h"
#include "S47Story.h"
#include "S47Taken.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47Trouble.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47WeekCase.h"
#include "CoreGlobals.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47BigNight,Log,All);

// M7, the first big night (Tom, 9 November 2026: «Første uken må ha litt mer innhold. Det må skje noe stort natt to eller tre for å
// gi både pekepinn på hva som kan skje fremover og hekte spillere»). One authored thing, seeded: the night (2 or 3), the hour (after
// two), and one of three shapes out on the land. What the player meets, by the seconds of the event's own age (FS47Happenings::Now):
//    0  the night goes quiet: crickets, a dog far off, the room's hum drops; the gear starts to feel something near (S47Omen)
//    3  a low note comes up, two tones close together that beat, under everything, for half a minute
//    6  every instrument at once: the rack's alarms, relays, the printer, the phone once; the truck's lamps blink, the lamp cuts out
//    8  the power flickers: SARO's lights and the yard's sodium lamps, Caliche's street lamps one by one
//   10  the array turns by itself, every dish on remote together, to one point low on the horizon, the motors loud in the yard
//   27  out there, in that direction, for nine seconds: a column of light stands up off the land and goes down into it ("column"),
//       or a light comes down fast and lights the ground where it goes out ("fall"), or a dozen small lights rise together and go
//       out all at once ("rise"); SARO goes dark under it, the truck's lights die
//   36  the note stops dead; silence. A boom arrives late with the distance (not after the rise: a gust)
//   42  the lights come back, the sodium lamps warming slowly; the printer runs one line by itself; the array stays where it went
//  110  over; the ground is quiet a while (it held the rhythm's ground throughout)
// The day after: the array still pointed there and its record (the night log's instrument line), Dale on the walkie (or his slip by
// the phone), 1420 talking about the lights, the CB in the evening, the Thursday Ledger from the findings, and the mark at the place
// with a lead to somewhere else (the visitors' case). Never 47 seconds, never its hour 02:29, nothing of Story's far side.
namespace S47Hap
{
 FString ClockText(double Clock);
 double Bearing(double Az);
 FString Compass(double Az);
 bool Clean(const FString& Text);
 double HourOf(double Clock);
}
namespace S47HapOmen
{
 void Forewarn(US47Main* M,const FString& Id,const FVector& Eye,const FVector* At,double Dist,double Severity,double Seconds);
}
namespace S47Big
{
 constexpr double Tone=3,Rack=6,Flick=8,Turn=10,Climax=27,ClimaxEnd=36,Back=42,Record=44,Len=110;
 const TCHAR* const Names[]={TEXT("column"),TEXT("fall"),TEXT("rise")};
 constexpr uint32 SaltNight=0x42494731u,SaltHour=0x42494732u,SaltVar=0x42494733u;
 double Smooth(double A,double B,double X){const double T=FMath::Clamp((X-A)/FMath::Max(1e-6,B-A),0.,1.);return T*T*(3-2*T);}
 // Level in three's terms (y up): the way along the ground, x and z (FVector::GetSafeNormal2D keeps x and y, which is wrong here).
 FVector Level(const FVector& V){return FVector(V.X,0,V.Z).GetSafeNormal();}
 // The hour as the records write it, never at Story's minute: 02:47 is written 02:48.
 FString Clock(double At){FString T=S47Hap::ClockText(At);if(T.EndsWith(TEXT(":47")))T=T.LeftChop(2)+TEXT("48");return T;}
 // Story's dark minute and the minutes round it: the big night never starts in them, however late it is let go.
 bool StoryWindow(double NightHour){return NightHour>=26.33&&NightHour<26.67;}
 // The words with their places filled in (FString::Printf takes only a literal).
 FString Put(const TCHAR* Text,const FString& Place,const FString& Clock=FString(),const FString& Way=FString(),double Az=0,double El=0)
 {
  return FString(Text).Replace(TEXT("{place}"),*Place).Replace(TEXT("{clock}"),*Clock).Replace(TEXT("{way}"),*Way)
   .Replace(TEXT("{az}"),*FString::Printf(TEXT("%03.0f"),Az)).Replace(TEXT("{el}"),*FString::Printf(TEXT("%.0f"),El));
 }
 // Dale's own words on the walkie: his set's prefix, so that no line of his here names him.
 FString Dale(const FString& Text){return FString(TEXT("DALE"))+TEXT(": ")+Text;}
 // What each shape leaves and how it is told. {place}: the place as spoken; {clock}: the hour; {way}: which way; {az}, {el}: the array.
 struct FWords
 {
  const TCHAR* Seen;      // the record when the player saw it
  const TCHAR* DaleLine;  // his second line
  const TCHAR* Caller;    // 1420
  const TCHAR* Voice;     // the caller's voice
  const TCHAR* Earl;      // the host's answer
  const TCHAR* Cb;        // the CB the next evening
  const TCHAR* Passed;    // the same as Dale passes it on
  const TCHAR* Title;     // the mark's panel
  const TCHAR* Mark;      // what the mark is, looked at
  const TCHAR* Lead;      // the thread out of it: {place} somewhere not heard of yet
  const TCHAR* Found;     // the record at the mark
 };
 const FWords Words[]={
  {TEXT("At {clock} a column of pale light stood up off the land out past {place}, {way}, for a few seconds, and went back down into the ground. The note under everything stopped with it."),
   TEXT("And a boy at the trailer park says a light stood up out past {place} like a fence post a mile high, and the yard lamps went out with it. Boys say things. The lamps I can see."),
   TEXT("Earl, I was up with the baby the other night, after two, and a light stood straight up out past {place}. Like a searchlight with nobody under it. Then every light in the house went."),TEXT("woman"),
   TEXT("You're not the first call about that, ma'am, and nobody's told me what it was. If it was the base, they're not saying."),
   TEXT("Breaker one-nine. Anybody else lose their dash lights last night, after two, out by {place}? Whole truck went dark and the radio sang one note at me."),
   TEXT("A trucker says his lights went out last night by {place}, and the radio sang one note."),
   TEXT("GLASSED GROUND"),
   TEXT("A disc of ground about two yards across has gone to glass: dark, glossy, cracked like a dry pond, still a little warm under the hand. A ring of pale ash around it. In the ash, small bare prints go round the glass twice, close together, and then go off in one line."),
   TEXT(" The line of prints goes off toward {place}. It is on your map."),
   TEXT("Out past {place} a disc of ground gone to glass, still warm, with a ring of ash around it. Small bare prints in the ash go round it twice and then off in one line.")},
  {TEXT("At {clock} a light came down fast out past {place}, {way}, and lit up the ground where it went out. A boom came after it, late."),
   TEXT("The county had three calls before four about something coming down out past {place}. And a boom that woke every dog in Caliche. A boom. Not a plane. I asked them."),
   TEXT("Earl, something came down out past {place} the other night. Lit the whole sky white and then a boom like they used to make at the mine. My windows are still talking about it."),TEXT("man"),
   TEXT("The county says there's no plane missing. That's all I've got, folks. If you find a piece of anything out there, leave it be and call the sheriff."),
   TEXT("Breaker one-nine. Who else heard that boom out by {place} last night? Lit me up like noon in the mirror. I pulled over and waited, and nothing came."),
   TEXT("A trucker says something came down by {place} last night, with a boom, and lit his mirror up."),
   TEXT("THE FURROW"),
   TEXT("A furrow about twenty yards long, a foot deep at its end, the caliche thrown up in a lip on either side and burnt black along the bottom. It starts shallow and goes deeper, and at the deep end there is nothing: no hole, no piece of anything. The brush either side is laid down flat, all pointing away."),
   TEXT(" Sighted along it, the furrow points back the way it came: toward {place}. It is on your map."),
   TEXT("Out past {place} a burnt furrow about twenty yards long, deepest at the end, and nothing at the end of it. The brush either side laid flat, pointing away.")},
  {TEXT("At {clock} a dozen small lights rose together off the land out past {place}, {way}, slowly, keeping together, and went out all at once high up."),
   TEXT("And the ranch out past {place} says the cattle went through two fences before the lights went up. Before! Something told them first."),
   TEXT("Earl, my cows are in my neighbor's yard and his are in mine. Something went up out past {place}, a whole string of lights going up slow, and every animal on the place ran from it."),TEXT("old"),
   TEXT("That's the third one about lights going up. I'm just the radio, folks. I don't know either."),
   TEXT("Breaker one-nine. Anybody see the lights go up out by {place} last night? A string of them, slow. My dog won't come out from under the trailer."),
   TEXT("A trucker says he saw a string of lights go up slow out by {place} last night."),
   TEXT("STONES TURNED OVER"),
   TEXT("In a ring about ten yards across every stone has been turned over, damp side up, as if each one were lifted and put back the wrong way. The middle is swept bare. Cattle tracks run out of the ring in every direction, flat out, the way animals go when they run."),
   TEXT(" One line of tracks runs straight and does not scatter: toward {place}. It is on your map."),
   TEXT("Out past {place} a ring of ground where every stone was turned over, damp side up, and the middle swept bare. Cattle tracks running out of it every way.")},
 };
 static_assert(UE_ARRAY_COUNT(Words)==3,"one set of words for each shape");
 // The same words on every line of Dale's morning, whatever the shape. PT-BIGNIGHT-20261109 (Tom): Dale says "Herrgott", awake or
 // slept through; it is his (DECISIONS: an angry German) and no filter takes it out (S47Hap::Clean, the self test below).
 const TCHAR* const DaleFirst=TEXT("Herrgott. Every dish on remote is looking at {az} this morning, {el} degrees up, like dogs at a door. Not on my log. Not on yours? Good. Then nobody did it.");
 const TCHAR* const DaleSlept=TEXT("Herrgott. You slept through it? Good for you. Every dish on remote is looking at {az} this morning, {el} degrees up, and the yard lamps are full of dead moths. Not on my log.");
 const TCHAR* const DaleLast=TEXT("Leave the array where it is until I have read the encoders myself. And if you go out there, you go by daylight, and you take the walkie. Out.");
 // CRIT6-20261109: his hail as he calls the station (S47FieldRadio's own): never by name. Reyes is the night operator who went missing.
 const TCHAR* const HailFirst=TEXT("SARO, Dale. Pick up.");
 const TCHAR* const HailAgain=TEXT("SARO! The walkie. Now.");
 const TCHAR* const HailGiveUp=TEXT("Fine. I leave it on the slip by the phone.");
 const TCHAR* const Slip=TEXT("A slip by the phone, in a hard square hand: \"Array on remote all at {az} this morning, {el} up. Not me. Not you? Leave it so. If you go out to {place}, go by day. D.\"");
}

// ---------------------------------------------------------------- the plan
FS47Happenings::FBigPlan FS47Happenings::BigPlanFor(int32 Seed)
{
 FBigPlan P;
 P.Night=FS47NightRhythm::Roll(Seed,0,S47Big::SaltNight)<.5f?2:3;
 // After two, before four; never the 02:20s to the 02:40s, where Story's own minute is.
 P.Hour=26.1+1.5*double(FS47NightRhythm::Roll(Seed,0,S47Big::SaltHour));
 if(P.Hour>=26.33&&P.Hour<26.67)P.Hour+=.5;
 P.Variant=FMath::Clamp(int32(FS47NightRhythm::Roll(Seed,0,S47Big::SaltVar)*3.f),0,2);
 return P;
}
const TCHAR* FS47Happenings::BigVariantName(int32 Variant){return S47Big::Names[FMath::Clamp(Variant,0,2)];}
int32 FS47Happenings::BigVariantOf(const FString& Name){for(int32 I=0;I<3;++I)if(Name==S47Big::Names[I])return I;return -1;}
FS47Happenings::FBigPlan FS47Happenings::BigPlan() const
{
 auto* F=M?M->Freeform.Get():nullptr;
 FBigPlan P=BigPlanFor(F?F->Seed():47);
 // A calm night has no lights: the other of the two, when that one is not calm. Both calm, it comes anyway.
 if(F&&F->QuietNight(P.Night)&&!F->QuietNight(P.Night==2?3:2))P.Night=P.Night==2?3:2;
 return P;
}
bool FS47Happenings::BigWakes(int32 Night) const
{
 return BigNight==0&&M&&M->Freeform&&!M->Freeform->InTutorial()&&!(M->Flow&&M->Flow->bDevRun&&!bBigDevNatural)&&BigPlan().Night==Night;
}
bool FS47Happenings::BigOwns(int32 Night) const
{
 if(!M||!M->Freeform||M->Freeform->InTutorial())return false;
 return BigOwnsNight(Night,BigPlan().Night,BigNight,!(M->Flow&&M->Flow->bDevRun&&!bBigDevNatural));
}
FString FS47Happenings::BigSpoken() const{return Spoken(BigTitle.IsEmpty()?FString(TEXT("the flat")):BigTitle);}

// ---------------------------------------------------------------- saving
void FS47Happenings::ResetBig()
{
 BigNight=0;BigVar=-1;BigSerial=0;BigDaleAt=0;BigClock=0;BigAz=0;BigEl=3;BigX=BigZ=0;BigTry=BigFollowTry=BigFlick=BigDaleIn=0;
 BigPlace.Empty();BigTitle.Empty();BigFollow.Reset();BigDaleLines.Reset();
 bBigSawNight=false;bBigPower=false;bBigMotors=false;bBigDimmed=false;bBigToneOn=false;bBigDevNatural=false;bBigSlept=false;BigSoon=-1;BigLastClock=-1;
}
void FS47Happenings::LoadBig(const S47J::FObj& O)
{
 if(!O.IsValid())return;
 BigNight=FMath::Clamp(S47J::Int(O,TEXT("night")),-1,100000);BigVar=FMath::Clamp(S47J::Int(O,TEXT("variant"),-1),-1,2);
 BigClock=S47J::Num(O,TEXT("clock"));BigAz=FMath::Fmod(FMath::Max(0.,S47J::Num(O,TEXT("az"))),360.);BigEl=FMath::Clamp(S47J::Num(O,TEXT("el"),3),0.,20.);
 BigX=S47J::Num(O,TEXT("x"));BigZ=S47J::Num(O,TEXT("z"));
 if(!FMath::IsFinite(BigX)||!FMath::IsFinite(BigZ)||FMath::Abs(BigX)>1e5||FMath::Abs(BigZ)>1e5){BigX=BigZ=0;}
 BigPlace=S47J::Str(O,TEXT("place")).Left(64);BigTitle=S47J::Str(O,TEXT("title")).Left(120);
 bBigSawNight=S47J::Bool(O,TEXT("saw"));
 static const TCHAR* const Known[]={TEXT("array"),TEXT("dale"),TEXT("slip"),TEXT("hail"),TEXT("missed"),TEXT("radio"),TEXT("cb"),TEXT("seen"),TEXT("felt"),TEXT("trace"),TEXT("unseen"),TEXT("forced"),TEXT("turned")};
 for(const FString& W:S47J::Strs(O,TEXT("follow")))for(const TCHAR* K:Known)if(W==K)BigFollow.Add(W);
 // A hail out when it was saved is not out any more.
 BigFollow.Remove(TEXT("hail"));
 if(BigNight>0&&BigVar<0)BigVar=0;
}
S47J::FObj FS47Happenings::SaveBig() const
{
 if(BigNight==0&&!bBigSawNight)return nullptr;
 auto O=S47J::New();
 S47J::Set(O,TEXT("night"),BigNight);S47J::Set(O,TEXT("variant"),BigVar);S47J::Set(O,TEXT("clock"),BigClock);S47J::Set(O,TEXT("az"),BigAz);S47J::Set(O,TEXT("el"),BigEl);
 S47J::Set(O,TEXT("x"),BigX);S47J::Set(O,TEXT("z"),BigZ);
 if(!BigPlace.IsEmpty())S47J::Set(O,TEXT("place"),BigPlace);
 if(!BigTitle.IsEmpty())S47J::Set(O,TEXT("title"),BigTitle);
 if(bBigSawNight)S47J::Set(O,TEXT("saw"),true);
 if(!BigFollow.IsEmpty())S47J::Set(O,TEXT("follow"),BigFollow.Array());
 return O;
}

// ---------------------------------------------------------------- the place
bool FS47Happenings::BigSite(int32 Night,double Clock)
{
 auto* F=M->Freeform.Get();
 using T=ES47PlaceTrait;
 // Open land out past the low ground, far enough to be a light on the horizon from SARO and near enough to walk to the next day.
 const uint32 Any=uint32(T::Open)|uint32(T::Wash)|uint32(T::Far),Not=uint32(T::Town)|uint32(T::Saro)|uint32(T::Indoors)|uint32(T::Array)|uint32(T::People)|uint32(T::Road);
 FSpot S=Where(TEXT("big_night"),0,Any,Not,900,3600,.4f,0x42494734u^uint32(Night));
 if(!S.bSet)S=Where(TEXT("big_night"),0,Any,Not,0,0,1.f,0x42494735u^uint32(Night));
 if(!S.bSet)return false;
 // Off the place's own point, on open ground.
 const double A=FS47NightRhythm::Roll(F->Seed(),Night,0x42494736u)*2*UE_DOUBLE_PI;
 BigX=S.X+FMath::Cos(A)*55;BigZ=S.Z+FMath::Sin(A)*55;BigPlace=S.Place;BigTitle=S.Title;
 double SaroX=0,SaroZ=0;S47OpenLand::RoadOf(TEXT("saro"),9,0,SaroX,SaroZ);
 BigAz=S47Hap::Bearing(FS47Places::AzimuthOf(SaroX,SaroZ,BigX,BigZ));
 const double Far=FVector2D::Distance(FVector2D(SaroX,SaroZ),FVector2D(BigX,BigZ));
 BigEl=FMath::RoundToDouble(FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(260.,FMath::Max(300.,Far))),2.,9.));
 Sites.RemoveAll([](const FSite& Old){return Old.Kind==TEXT("bigmark");});
 FSite R;R.Kind=TEXT("bigmark");R.Variant=BigVariantName(BigVar);R.Place=BigPlace;R.Title=BigTitle;R.Night=Night;R.Serial=Serial+1;R.X=BigX;R.Z=BigZ;R.Yaw=A;
 R.Az=BigAz;R.Spread=6;R.bKnown=false;R.Until=Clock+7*86400.;
 Sites.Add(R);UsedPlace(BigPlace,TEXT("big_night"));
 UE_LOG(LogS47BigNight,Display,TEXT("S47: big night %s at %s (%s), %.0f m from SARO, bearing %.0f, %.0f up"),BigVariantName(BigVar),*BigTitle,*BigPlace,Far,BigAz,BigEl);
 return true;
}

// ---------------------------------------------------------------- starting it
bool FS47Happenings::BeginBig(const FString& Variant,bool bForced,FString& Why)
{
 auto* F=M->Freeform.Get();
 if(!bForced&&BigNight!=0){Why=TEXT("once in a game");return false;}
 if(!bForced&&!MayStart(TEXT("big_night"))&&HourOfNight(F->Clock())<28.5){Why=TEXT("the rhythm says not now");return false;}
 const int32 Asked=BigVariantOf(Variant);
 const int32 Night=F->Day();
 const int32 WasVar=BigVar;
 BigVar=Asked>=0?Asked:BigPlan().Variant;
 if(!BigSite(Night,F->Clock())){BigVar=WasVar;Why=TEXT("no open ground for it");return false;}
 // F10's run is a run: the day after follows it, as in play, but it says it was F10's.
 if(bForced){BigFollow.Reset();BigDaleLines.Reset();BigFollow.Add(TEXT("forced"));}
 BigNight=Night;BigClock=F->Clock();bBigSawNight=true;
 Now=FActive();Now.Id=TEXT("big_night");Now.Variant=BigVariantName(BigVar);Now.Place=BigPlace;Now.Title=BigTitle;Now.X=BigX;Now.Z=BigZ;Now.Yaw=BigAz;
 Now.Len=S47Big::Len;Now.bForced=bForced;Now.Serial=Serial+1;Now.Clock=F->Clock();
 BigFlick=0;
 // The night goes quiet first, from the place's direction: crickets, birds, the gear starting to feel it.
 FVector Ground;const bool bHere=WorldOf(BigX,BigZ,Ground);
 S47HapOmen::Forewarn(M,TEXT("happen:big_night"),Eye(),bHere?&Ground:nullptr,180,.55,40);
 // What will stand out there is built now, hidden, not in the second it is wanted.
 if(bHere){Prop(FString(TEXT("bigglow_"))+BigVariantName(BigVar));if(BigVar==1)Prop(TEXT("bigflash"));}
 if(US47Audio* A=M->Audio.Get())
 {
  A->HoldRoomTone(.25f,.6f);bBigToneOn=true;
  if(bHere)A->Far(TEXT("dog1"),Eye()+S47Big::Level(Ground-Eye())*120.+FVector(0,2,0),.3f,.9f);
 }
 Noticed(TEXT("big_night"),-1,TEXT("here"),Serial+1);
 // PT-FEEDER-20261109 (d37): the big night owns its night and points at its own place. Marks some lights left tonight stay what they
 // are, a record to find, but nothing more comes to them and no second call sends the player there; and it is the night's one
 // pointing-out (FS47FreeformDirector::MayPointOut), so no bearing slip sends the player anywhere else after it.
 if(F->FarmSite)F->FarmSite->YieldTo(TEXT("big_night"));
 if(!bForced&&F->Director&&F->Director->MayPointOut(Night))F->Director->NotePointOut(Night);
 UE_LOG(LogS47BigNight,Display,TEXT("S47: big night begins day %d %s, %s, in %s%s"),Night,*S47Hap::ClockText(F->Clock()),BigVariantName(BigVar),*M->Web->Area(),bForced?TEXT(", F10"):TEXT(""));
 return true;
}

// The array: every dish on remote to the one point, together. bSnap: it is found there (it turned while the player was away).
void FS47Happenings::BigTurnArray(bool bSnap)
{
 AS47Web* W=M&&M->Web?M->Web.Get():nullptr;
 if(!W||W->Area()!=TEXT("saro")||W->DishCount()<=0)return;
 int32 Turned=0;
 for(int32 I=0;I<W->DishCount();++I)
 {
  if(!W->DishRemoteAvailable(I))continue;
  if(bSnap)W->SnapDish(I,BigAz,BigEl);
  if(W->PointDish(I,BigAz,BigEl,bSnap?9.:3.5))++Turned;
 }
 if(!bSnap&&Turned>0&&M->Audio&&!bBigMotors){M->Audio->StartMotors();bBigMotors=true;}
 if(Turned>0)BigFollow.Add(TEXT("turned"));
 UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: %d dishes on remote %s %.0f, %.0f up"),Turned,bSnap?TEXT("found at"):TEXT("turn to"),BigAz,BigEl);
}
void FS47Happenings::BigRecordArray(bool bWasThere)
{
 auto* F=M->Freeform.Get();
 if(BigFollow.Contains(TEXT("array")))return;
 BigFollow.Add(TEXT("array"));
 if(!BigFollow.Contains(TEXT("turned")))BigTurnArray(true);
 if(FSite* S=SiteOf(TEXT("bigmark")))S->bKnown=true;
 const FString Key=FString::Printf(TEXT("array_burst_d%d_turn"),BigNight);
 if(!F->HasFinding(Key))
  F->RecordEvidence(Key,FString::Printf(TEXT("At %s every antenna on remote turned by itself, all together, to about %03.0f degrees and %.0f up, low on the horizon, and stayed there. Nothing on the command log. The printer ran one line by itself: ALL CHANNELS OVER RANGE."),*S47Big::Clock(BigClock),BigAz,BigEl));
 if(M->UI)M->UI->Toast(bWasThere?FString::Printf(TEXT("ARRAY / Every antenna on remote has turned by itself to %03.0f, %.0f up. Nothing on the command log. The bearing is on your map (TAB)."),BigAz,BigEl)
  :FString::Printf(TEXT("ARRAY / Every antenna on remote is pointed at %03.0f, %.0f up. Nobody commanded it. On the printer's strip, one line by itself at %s: ALL CHANNELS OVER RANGE. The bearing is on your map (TAB)."),BigAz,BigEl,*S47Big::Clock(BigClock)),8);
 UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: the array's record kept (%s)"),bWasThere?TEXT("there when it turned"):TEXT("found the morning after"));
 M->RequestAutosave();
}
void FS47Happenings::EndBigEffects()
{
 if(IsEngineExitRequested()||GExitPurge||!IsValid(M)){bBigPower=bBigMotors=bBigDimmed=bBigToneOn=false;return;}
 if(bBigPower&&IsValid(M->SaroNight)&&M->SaroNight->Power)M->SaroNight->Power->Restore();
 if(bBigMotors&&IsValid(M->Audio))M->Audio->StopMotors();
 if(bBigDimmed&&IsValid(M->World))M->World->Trouble.Dim=1;
 if(bBigToneOn&&IsValid(M->Audio)){M->Audio->HoldRoomTone(1.f,.8f);M->Audio->Stop(TEXT("hapBigTone"),.3f);}
 bBigPower=bBigMotors=bBigDimmed=bBigToneOn=false;
 if(IsValid(M->Web.Get())&&!IsGarbageCollecting())for(const TCHAR* K:{TEXT("bigglow_column"),TEXT("bigglow_fall"),TEXT("bigglow_rise"),TEXT("bigflash")})ShowBig(K,FVector::ZeroVector,0,FVector(1),0,false);
}

// ---------------------------------------------------------------- the night itself
void FS47Happenings::StepBig(double Dt)
{
 using namespace S47Big;
 auto* F=M->Freeform.Get();
 US47Audio* A=M->Audio.Get();
 const double Age=Now.Age,Was=Age-Dt;
 const auto Crossed=[&](double T){return Was<T&&Age>=T;};
 const FString Area=M->Web->Area();
 const bool bSaro=Area==TEXT("saro"),bDrive=Driving(),bOut=Outdoors()&&!bDrive;
 FVector Ground;const bool bHere=WorldOf(BigX,BigZ,Ground);
 // The gear feels it from the start: a source at the player, strong enough for the lamp, the walkie, the truck and the watch.
 if(M->Omen&&Age<Back)
 {
  FS47OmenSource S;S.Id=TEXT("night:big_night");S.At=Eye()+(bHere?S47Big::Level(Ground-Eye())*25.:FVector(25,0,0));S.Severity=Age<Climax?.45:.65;S.Reach=320;S.bComing=Age<Flick;
  M->Omen->Post(S,1.);
 }
 if(Crossed(Tone)&&A)
 {
  // Two low tones close together, beating, with one above: under everything, until the light out there goes.
  FS47Sound O;O.Gain=.5f;O.bLoop=true;A->LoopSfx(TEXT("hapBigTone"),Synth(TEXT("lowtone")),O);
  A->HoldRoomTone(.1f,1.5f);bBigToneOn=true;
 }
 if(Crossed(Rack))
 {
  if(bSaro&&A)
  {
   // Every instrument at once: the rack's alarms, relays going, the printer, the phone once.
   for(int32 K=0;K<10;++K)A->Beep(K%2?1180.f:1460.f,.07f,.06f,.11f*K,TEXT("square"));
   A->Sfx(TEXT("rack_switch"),FS47Sound{.Gain=.7f});
   for(int32 K=0;K<7;++K){FS47Sound C;C.Gain=.35f;C.Rate=FMath::FRandRange(.85f,1.25f);C.When=.2f+.23f*K;A->Sfx(TEXT("click"),C);}
   {FS47Sound P;P.At=FVector(-4,1,2);P.Gain=.32f;P.When=.6f;const FS47Play Line=A->Sfx(TEXT("printer"),P);if(Line&&A->Ctx.IsValid())Line.Src->Stop(A->Ctx->CurrentTime()+5.);}
   {FS47Sound R;R.Gain=.35f;R.When=1.4f;R.Dur=1.1f;A->Sfx(TEXT("phoneRing"),R);}
  }
  if(bDrive){FS47TroubleOpts O;O.Count=4.;O.Per=.5;M->World->Trouble.Start(ES47Trouble::Blink,O);}
  if(M->Night&&M->Night->Flashlight&&M->Night->Flashlight->bOn)M->Night->Flashlight->FlickerOut(.9);
 }
 if(Crossed(Flick))
 {
  if(bSaro&&IsValid(M->SaroNight)&&M->SaroNight->Power&&!M->SaroNight->Power->Out()){M->SaroNight->Power->Cut();bBigPower=true;}
  // Caliche's street lamps go out one after another down the street.
  double X=0,Z=0;
  if((Area==TEXT("road")||Area==TEXT("diner"))&&PlayerRoad(X,Z)&&Z>1800&&Z<2450&&FMath::Abs(X)<320)if(FS47Town* T=M->TownLayer?M->TownLayer->Town.Get():nullptr)T->LampWave(M->T);
 }
 // The power: flickering, a dim steady, dark under it, then back with the sodium lamps warming.
 if(bBigPower&&IsValid(M->SaroNight)&&M->SaroNight->Power)
 {
  FS47SaroPower& P=*M->SaroNight->Power;
  if(Age<Turn+6){if(M->T>=BigFlick){BigFlick=M->T+FMath::FRandRange(.06f,.24f);P.Set(FMath::FRand()<.35f?.05:FMath::FRandRange(.5f,1.f),FMath::FRandRange(0.f,.7f));}}
  else if(Age<Climax)P.Set(.38,.18);
  else if(Age<Back)P.Set(0,0);
  else if(Age<Back+8)P.Set(1,Smooth(Back,Back+8,Age)*.8);
  else{P.Restore();bBigPower=false;}
 }
 if(!bSaro&&bOut&&Age>=Flick&&Age<Climax&&M->T>=BigFlick&&M->Night&&M->Night->Flashlight&&M->Night->Flashlight->bOn){BigFlick=M->T+FMath::FRandRange(.6f,1.4f);M->Night->Flashlight->Flicker(.8,TEXT("big_night"));}
 if(Crossed(Turn))BigTurnArray(false);
 if(bBigMotors&&Age>Turn+3&&M->Web&&!M->Web->DishesMoving()){if(A)A->StopMotors();bBigMotors=false;}
 if(Crossed(Climax))
 {
  if(bDrive){M->World->Trouble.Dim=.04;bBigDimmed=true;}
  if(M->Night&&M->Night->Flashlight&&M->Night->Flashlight->bOn)M->Night->Flashlight->FlickerOut(ClimaxEnd-Climax);
 }
 // Out there, in that direction: the shape of tonight.
 if(bHere&&Age>=Climax-.5&&Age<ClimaxEnd+8)
 {
  const double T=(Age-Climax)/(ClimaxEnd-Climax);
  const FVector Away=S47Big::Level(Ground-Eye());
  FVector At=Ground;FString Kind;bool bShow=false;FVector Scale(1);float K=1;
  if(BigVar==0)
  {
   // Up off the land in a second, standing, and back down into it.
   const double H=Smooth(0,.14,T)*(1-Smooth(.84,1,T));
   Kind=TEXT("bigglow_column");bShow=T>=0&&T<1&&H>.01;Scale=FVector(1,FMath::Max(.01,H),1);K=float(H);
   At=Ground+FVector(0,FMath::Max(0.,(1-H))*-2,0);
  }
  else if(BigVar==1)
  {
   // From high and behind it, at a slant, faster as it comes: then the ground lit where it went out, fading.
   const double D=FMath::Clamp(T/.42,0.,1.);
   Kind=TEXT("bigglow_fall");bShow=T>=0&&D<1;
   At=Ground+Away*(1500.*(1-D*D))+FVector(0,1150.*(1-D*D)+6,0);
   if(D>=1)
   {
    const double Since=(T-.42)*(ClimaxEnd-Climax);
    ShowBig(TEXT("bigflash"),Ground+FVector(0,30,0),0,FVector(1),float(FMath::Exp(-Since*.55)),Since<14);
   }
  }
  else
  {
   // A dozen, together, slowly up; and out all at once.
   const double Up=Smooth(0,.92,T);
   Kind=TEXT("bigglow_rise");bShow=T>=0&&T<.92;At=Ground+FVector(0,8+780.*Up,0);
  }
  if(!Kind.IsEmpty())ShowBig(Kind,At,Age*.05,Scale,K,bShow);
  if(bShow&&!BigFollow.Contains(TEXT("seen")))
  {
   const FVector Look=BigVar==0?Ground+FVector(0,250,0):At;
   if(SeenLit(Look,14000,48,LitOwn,*Kind))
   {
    BigFollow.Add(TEXT("seen"));
    Noticed(TEXT("big_night"),AwayFrom(Ground),TEXT("sight"),Now.Serial);
    UE_LOG(LogS47BigNight,Display,TEXT("S47: big night %s seen, %.0f m off"),BigVariantName(BigVar),FVector::Dist(Eye(),Look));
   }
  }
 }
 if(Crossed(ClimaxEnd))
 {
  if(A)A->Stop(TEXT("hapBigTone"),.04f);
  UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: the note stops, %s"),BigFollow.Contains(TEXT("seen"))?TEXT("seen"):TEXT("not seen"));
 }
 // The sound of it arrives late, with the distance.
 {
  const double Dist=bHere?FVector::Dist(Eye(),Ground):2500.;
  const double Late=ClimaxEnd+FMath::Clamp(Dist/340.,1.5,8.)-(BigVar==1?6.:0.);
  if(Crossed(Late)&&A)
  {
   if(BigVar==2){FS47Sound G;G.Gain=.4f;G.Rate=.7f;A->Sfx(TEXT("gust"),G);}
   else
   {
    A->Boom(BigVar==1?.85f:.5f);
    if(bSaro&&!Outdoors())for(int32 K=0;K<3;++K){FS47Sound O;O.Gain=.18f;O.Rate=1.3f;O.When=.15f*K;O.Lp=1800;A->Sfx(TEXT("thudSoft"),O);}
   }
  }
 }
 if(Crossed(Back))
 {
  if(bBigDimmed&&M->World){M->World->Trouble.Dim=1;bBigDimmed=false;}
  if(A)A->HoldRoomTone(1.f,2.f);
  bBigToneOn=false;
  if(bSaro&&A){FS47Sound P;P.At=FVector(-4,1,2);P.Gain=.3f;P.When=1.2f;const FS47Play Line=A->Sfx(TEXT("printer"),P);if(Line&&A->Ctx.IsValid())Line.Src->Stop(A->Ctx->CurrentTime()+2.5);}
 }
 if(Crossed(Record))
 {
  // Whatever stood out there is gone from the scene now (the fall's lit ground has faded by this).
  for(const TCHAR* K:{TEXT("bigglow_column"),TEXT("bigglow_fall"),TEXT("bigglow_rise"),TEXT("bigflash")})ShowBig(K,FVector::ZeroVector,0,FVector(1),0,false);
  const FString Clock=S47Big::Clock(BigClock);
  if(bSaro)BigRecordArray(true);
  if(BigFollow.Contains(TEXT("seen")))
  {
   const FString Way=FString::Printf(TEXT("about %03.0f degrees from SARO"),BigAz);
   F->RecordEvidence(FString::Printf(TEXT("landing_seen_d%d_wide"),BigNight),S47Big::Put(S47Big::Words[BigVar].Seen,BigTitle,Clock,Way));
   Deed(FString::Printf(TEXT("You watched the light out past %s the night the array turned."),*BigTitle));
  }
  else
  {
   BigFelt();
  }
  M->RequestAutosave();
 }
 if(Age>=Now.Len)
 {
  EndBigEffects();
  UE_LOG(LogS47BigNight,Display,TEXT("S47: big night over, %s"),BigFollow.Contains(TEXT("seen"))?TEXT("seen"):TEXT("felt"));
  EndEvent(true);
 }
}

// Slept through, or kept from it until five: it happened all the same, and the morning finds what it left. bSlept: the player slept
// through it (Dale's words say so); otherwise they were awake and only felt it, and that is what is kept.
void FS47Happenings::BigUnseen(int32 Night,bool bSlept)
{
 auto* F=M->Freeform.Get();
 const FBigPlan P=BigPlan();
 BigVar=P.Variant;
 // Its own hour on its own night, for the records.
 BigClock=(double(Night)-1.)*86400.+8*3600.+(P.Hour-8.)*3600.;
 if(!BigSite(Night,F->Clock())){BigNight=-1;UE_LOG(LogS47BigNight,Display,TEXT("S47: big night on night %d, and no ground for it"),Night);return;}
 BigNight=Night;
 if(bSlept)BigFollow.Add(TEXT("unseen"));else BigFelt();
 UE_LOG(LogS47BigNight,Display,TEXT("S47: big night %s on night %d (%s): the morning finds what it left"),bSlept?TEXT("slept through"):TEXT("kept off until five"),Night,BigVariantName(BigVar));
 M->RequestAutosave();
}
// Felt and not seen: the lights going and the note, under a key of its own (a lead for the visitors' case, not Caliche's outage).
void FS47Happenings::BigFelt()
{
 auto* F=M->Freeform.Get();
 BigFollow.Add(TEXT("felt"));
 const FString Key=FString::Printf(TEXT("big_felt_d%d"),BigNight);
 if(F&&!F->HasFinding(Key))F->RecordEvidence(Key,FString::Printf(TEXT("At %s every light went down and came back, and a low note came up out of the ground for half a minute and stopped all at once."),*S47Big::Clock(BigClock)));
}

// Due at its hour on its night, by the rhythm; past five, it happened without the player. Asked in the night's own rules only, and
// in the first shift too, so that a long first shift or a long sleep never loses it.
void FS47Happenings::StepBigDue()
{
 auto* F=M->Freeform.Get();
 const double Was=BigLastClock;BigLastClock=F->Clock();
 if(BigNight!=0)return;
 // A developer run (the scenario tests among them) has none of its own unless F10's natural line asks: the tests built on other
 // nights stay as they were. A real game, and -S47NormalJourney, have it.
 if(M->Flow&&M->Flow->bDevRun&&!bBigDevNatural)return;
 const FBigPlan P=BigPlan();
 const int32 Night=FS47FreeformSession::NightOf(F->Clock());
 const double H=HourOfNight(F->Clock());
 // Seen in this game before its night is over: from now on it cannot be lost, only slept through.
 if(Night<P.Night){bBigSawNight=true;return;}
 // The clock went on a long way at once: a rest, a wait or a trip. Over its night, it was slept through.
 const bool bJump=Was>0&&F->Clock()-Was>1800;
 if(Night>P.Night)
 {
  if(bBigSawNight)BigUnseen(P.Night,true);
  else{BigNight=-1;UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: this game came in after its night %d; not made up for"),P.Night);}
  return;
 }
 if(!bBigSawNight){bBigSawNight=true;UE_LOG(LogS47BigNight,Display,TEXT("S47: big night tonight, night %d, from %s, %s"),P.Night,*S47Hap::ClockText(FMath::Fmod(P.Hour,24.)*3600.),BigVariantName(P.Variant));}
 // Woken on its night (FS47FreeformDirector::DrawForNight) after one: it comes soon after the waking, not at an hour slept through again.
 if(bJump&&H>=25.&&H<28.5){BigSoon=FMath::Max(26.05,H+.04);UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: awake at %s on its night, so it comes soon"),*S47Hap::ClockText(F->Clock()));}
 if(bJump)bBigSlept=true;else if(H>=25.)bBigSlept=false;
 if(H>=29.){BigUnseen(P.Night,bBigSlept);return;}
 double From=P.Hour;if(BigSoon>0)From=FMath::Min(From,BigSoon);
 if(S47Big::StoryWindow(From))From=26.67;
 // (CRIT6-20261109: nor on top of a director script; it waits for the script's end)
 if(H<From||S47Big::StoryWindow(H)||Paced<BigTry||!Now.Id.IsEmpty()||M->ModalOpen()||F->InTutorial()||FS47ScriptPlayer::Busy(M)||FS47WeekCase::Holds(M))return;
 // Late in the night the rhythm's rest no longer holds it back, but nothing else on the ground and nobody being taken.
 if(H>=28.5)
 {
  const bool bBusy=(M->Rhythm&&!M->Rhythm->ReadNow().Busy.IsNone())||(F->Taken&&F->Taken->Busy())||(F->Director&&F->Director->Busy())||FS47WeekCase::Holds(M);
  if(bBusy){BigTry=Paced+5;return;}
 }
 BigTry=Paced+5;
 FString Why;
 if(StartEvent(TEXT("big_night"),FString(),false,Why))return;
 Refused(TEXT("big_night"),Why);
}

// ---------------------------------------------------------------- the day after
void FS47Happenings::StepBigAfter()
{
 if(BigNight<=0||!M||!M->Freeform||BigLive())return;
 auto* F=M->Freeform.Get();
 // Cut short by a Continue before its records: it was felt at least.
 if(!BigFollow.Contains(TEXT("seen"))&&!BigFollow.Contains(TEXT("felt"))&&!BigFollow.Contains(TEXT("unseen")))BigFelt();
 const int32 Day=F->Day();
 const double Hour=S47Hap::HourOf(F->Clock());
 const double NightH=HourOfNight(F->Clock());
 const bool bSaro=M->Web&&M->Web->Area()==TEXT("saro");
 const bool bMorning=Day==BigNight+1;
 const S47Big::FWords& W=S47Big::Words[FMath::Clamp(BigVar,0,2)];
 // Dale's lines, one at a time, as he says them.
 if(!BigDaleLines.IsEmpty()&&Paced>=BigDaleIn&&M->FieldRadio.IsValid()&&!M->ModalOpen())
 {
  if(M->FieldRadio->Message(BigDaleLines[0])){BigDaleLines.RemoveAt(0);BigDaleIn=Paced+7.5;}
  else BigDaleIn=Paced+3;
 }
 if(Paced<BigFollowTry)return;
 BigFollowTry=Paced+2;
 // The array where it went: found there on the first visit to SARO before the evening after, if nobody saw it turn.
 if(!BigFollow.Contains(TEXT("array")))
 {
  if(Day>BigNight+1||(bMorning&&Hour>=19&&Hour<24)){BigFollow.Add(TEXT("array"));UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: the array was never looked at by the evening after"));}
  else if(bSaro&&M->Web->DishCount()>0&&M->Web->IsLoaded()){BigTurnArray(true);BigRecordArray(false);}
 }
 // Dale, from half past eight the morning after: a hail on the walkie; his slip by the phone if nobody answers or has the set.
 if(bMorning&&!BigFollow.Contains(TEXT("dale"))&&!BigFollow.Contains(TEXT("slip")))
 {
  const FString Place=BigSpoken();
  if(Hour>=8.5&&Hour<18&&!BigFollow.Contains(TEXT("hail"))&&!BigFollow.Contains(TEXT("missed"))&&M->FieldRadio.IsValid()&&M->FieldRadio->Owned()&&!M->ModalOpen())
  {
   const TWeakPtr<uint8> Guard=Alive;
   FS47FieldRadio::FHail Words{S47Big::HailFirst,S47Big::HailAgain,S47Big::HailGiveUp};
   const bool bSlept=BigFollow.Contains(TEXT("unseen"));
   if(M->FieldRadio->Hail(Words,[this,Guard,Place,bSlept]
    {
     if(!Guard.IsValid())return;
     const S47Big::FWords& Wd=S47Big::Words[FMath::Clamp(BigVar,0,2)];
     BigDaleLines.Reset();
     BigDaleLines.Add(S47Big::Dale(S47Big::Put(bSlept?S47Big::DaleSlept:S47Big::DaleFirst,Place,FString(),FString(),BigAz,BigEl)));
     BigDaleLines.Add(S47Big::Dale(S47Big::Put(Wd.DaleLine,Place)));
     BigDaleLines.Add(S47Big::Dale(S47Big::DaleLast));
     BigDaleIn=Paced+1.5;
     BigFollow.Add(TEXT("dale"));
     if(FSite* S=SiteOf(TEXT("bigmark")))S->bKnown=true;
     UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: Dale on the walkie, answered"));
     if(M)M->RequestAutosave();
    },[this,Guard]{if(Guard.IsValid()){BigFollow.Remove(TEXT("hail"));BigFollow.Add(TEXT("missed"));UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: Dale's hail not answered: his slip by the phone"));}}))
   {BigFollow.Add(TEXT("hail"));UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: Dale hails on the walkie"));}
   else if(Hour>=10.5){BigFollow.Add(TEXT("missed"));UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: the walkie would not take Dale's hail (another channel, or left out): his slip by the phone"));}
  }
  // No set, or no answer: the slip, read at the station indoors.
  const bool bNoSet=!M->FieldRadio.IsValid()||!M->FieldRadio->Owned();
  if((bNoSet||BigFollow.Contains(TEXT("missed")))&&Hour>=8.5&&bSaro&&!Outdoors()&&!M->ModalOpen()&&M->UI)
  {
   M->UI->Toast(S47Big::Put(S47Big::Slip,Place,FString(),FString(),BigAz,BigEl),11);
   BigFollow.Add(TEXT("slip"));
   if(FSite* S=SiteOf(TEXT("bigmark")))S->bKnown=true;
   UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: Dale's slip by the phone read"));
   M->RequestAutosave();
  }
 }
 // 1420, in the program's own turn with the truck's radio on it: Freeform's 1420 is on the air from midnight to four, so the nights
 // after (from the next one to the third).
 if(!BigFollow.Contains(TEXT("radio"))&&Day>BigNight&&Day<=BigNight+3&&M->CarRadio&&M->CarRadio->Live()&&M->CarRadio->Station==TEXT("caliche")&&M->Radio&&M->Story)
 {
  const auto* St=M->Radio->Stations.Find(TEXT("caliche"));
  if(St&&St->IsValid())
  {
   TArray<FS47RadioLine> Lines;
   Lines.Add(FS47RadioLine(S47Big::Put(W.Caller,BigSpoken()),FString(W.Voice),TEXT("a caller")));
   Lines.Add(FS47RadioLine(FString(W.Earl),TEXT("host")));
   if((*St)->Slot(Lines,M->Story->Clock()))
   {
    BigFollow.Add(TEXT("radio"));
    if(FSite* S=SiteOf(TEXT("bigmark")))S->bKnown=true;
    UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: 1420 has a caller about it"));
    M->RequestAutosave();
   }
  }
 }
 // The CB the evening after, from someone out on the road that night.
 if(!BigFollow.Contains(TEXT("cb"))&&bMorning&&NightH>=20&&NightH<23&&Now.Id.IsEmpty()&&Uneasy.I<0&&(CanHearCb()||(M->FieldRadio.IsValid()&&M->FieldRadio->Owned())))
 {
  const FString Place=BigSpoken();
  Tip({S47Big::Put(W.Cb,Place)},BigVar==2?TEXT("old"):TEXT("man"),S47Big::Put(W.Passed,Place));
  BigFollow.Add(TEXT("cb"));
  UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: talked of on the CB the evening after"));
  M->RequestAutosave();
 }
}
void FS47Happenings::DevBigNatural()
{
 if(BigLive())EndEvent(false);
 EndBigEffects();
 ResetBig();bBigDevNatural=true;
}
bool FS47Happenings::DevBigSlept(const FString& Variant,FString& Why)
{
 auto* F=M&&M->Freeform?M->Freeform.Get():nullptr;
 if(!F||!F->Active()||!M->IsFreeform()){Why=TEXT("Freeform only");return false;}
 if(BigLive())EndEvent(false);
 const int32 V=BigVariantOf(Variant);
 const int32 WasVar=BigVar;
 BigVar=V>=0?V:BigPlan().Variant;
 BigFollow.Reset();BigDaleLines.Reset();
 const int32 Night=FS47FreeformSession::NightOf(F->Clock());
 BigClock=F->Clock();
 if(!BigSite(Night,BigClock)){BigVar=WasVar;Why=TEXT("no open ground for it from here");return false;}
 BigNight=Night;bBigSawNight=true;BigFollow.Add(TEXT("unseen"));BigFollow.Add(TEXT("forced"));
 UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: F10, slept through on night %d (%s)"),Night,BigVariantName(BigVar));
 return true;
}
void FS47Happenings::DevBigMorning()
{
 if(BigNight<=0||!M||!M->Freeform)return;
 // As the morning after would find it: what has not come yet comes at the next look.
 BigFollow.Remove(TEXT("hail"));BigFollow.Remove(TEXT("missed"));BigFollowTry=0;
 if(M->FieldRadio.IsValid()&&!M->FieldRadio->Owned())M->FieldRadio->TestOwn();
}

// ---------------------------------------------------------------- the mark, looked at
void FS47Happenings::UseBig()
{
 FSite* S=SiteOf(TEXT("bigmark"));if(!S||!M->UI||!M->Freeform)return;
 auto* F=M->Freeform.Get();
 const int32 V=FMath::Max(0,BigVariantOf(S->Variant));
 const S47Big::FWords& W=S47Big::Words[V];
 const bool bFirst=!S->bFound;S->bFound=true;S->bKnown=true;
 FString Page=W.Mark;
 // The thread out of it: somewhere nobody has told the player of yet.
 if(bFirst&&F->Places)
 {
  const FString Next=F->Places->NextUnknown(S->Night*7+3);
  if(const FS47Places::FDef* D=Next.IsEmpty()?nullptr:FS47Places::Def(Next)){F->Places->Hear(Next);Page+=S47Big::Put(W.Lead,FString(D->Title));}
 }
 TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Text(Page));
 if(GeigerBuilt())Rows.Add(S47Kit::Text(TEXT("COUNTER / Over background all through it, highest in the middle.")));
 const FString Key=FString::Printf(TEXT("lights_trace_bignight_d%d"),S->Night);
 if(!F->HasFinding(Key))F->RecordEvidence(Key,S47Big::Put(W.Found,S->Title));
 {double SaroX=0,SaroZ=0;S47OpenLand::RoadOf(TEXT("saro"),9,0,SaroX,SaroZ);S->Az=S47Hap::Bearing(FS47Places::AzimuthOf(SaroX,SaroZ,S->X,S->Z));S->Spread=3;}
 BigFollow.Add(TEXT("trace"));
 Deed(FString::Printf(TEXT("You walked out to the marked ground past %s."),*S->Title));
 Rows.Add(S47Kit::Small(bFirst?TEXT("Kept as a field record."):TEXT("Already in your field records.")));
 UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: the mark (%s) looked at%s"),*S->Variant,bFirst?TEXT(", first time"):TEXT(""));
 M->UI->Open(S47Kit::Shell(M->UI,W.Title,S47Kit::Col(Rows),760));
 M->RequestAutosave();
}

// ---------------------------------------------------------------- self-test
bool FS47Happenings::BigSelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const FString& Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s big night: %s"),B?TEXT("PASS"):TEXT("FAIL"),*Why));};
 // The plan: always the second or third night, after two and before four, never in Story's minute, and the three shapes all come.
 {
  int32 Two=0,Three=0,Bad=0,Var[3]={0,0,0};double Lo=99,Hi=0;bool bSame=true;
  for(int32 Seed=1;Seed<=2000;++Seed)
  {
   const FBigPlan P=BigPlanFor(Seed);
   Two+=P.Night==2;Three+=P.Night==3;
   if(P.Night<2||P.Night>3||P.Hour<26.||P.Hour>=28.||(P.Hour>=26.33&&P.Hour<26.67)||P.Variant<0||P.Variant>2)++Bad;
   else ++Var[P.Variant];
   Lo=FMath::Min(Lo,P.Hour);Hi=FMath::Max(Hi,P.Hour);
   const FBigPlan Q=BigPlanFor(Seed);bSame&=Q.Night==P.Night&&Q.Hour==P.Hour&&Q.Variant==P.Variant;
  }
  Check(Bad==0&&Two>700&&Three>700,FString::Printf(TEXT("always night 2 or 3 (%d and %d of 2000 games), none outside"),Two,Three));
  Check(Bad==0&&Lo>=26.&&Hi<28.,FString::Printf(TEXT("always after two and before four: from %s to %s"),*S47Hap::ClockText(FMath::Fmod(Lo,24.)*3600.),*S47Hap::ClockText(FMath::Fmod(Hi,24.)*3600.)));
  Check(Bad==0,TEXT("never between 02:20 and 02:40, where Story's dark minute is"));
  Check(Var[0]>450&&Var[1]>450&&Var[2]>450,FString::Printf(TEXT("all three shapes come: column %d, fall %d, rise %d"),Var[0],Var[1],Var[2]));
  Check(bSame,TEXT("the same seed draws the same night, hour and shape"));
 }
 // Once: what is saved says it came, and a load does not bring it again.
 {
  FS47Happenings H(nullptr);H.ResetBig();
  H.BigNight=2;H.BigVar=1;H.BigAz=132;H.BigEl=4;H.BigTitle=TEXT("the dry lake");H.BigFollow.Add(TEXT("array"));H.BigFollow.Add(TEXT("hail"));H.bBigSawNight=true;
  const S47J::FObj O=H.SaveBig();
  FS47Happenings L(nullptr);L.ResetBig();L.LoadBig(O);
  Check(L.BigNight==2&&L.BigVar==1&&FMath::IsNearlyEqual(L.BigAz,132.)&&L.BigHas(TEXT("array"))&&!L.BigHas(TEXT("hail"))&&L.bBigSawNight,TEXT("saved once it has come: a load keeps its night, its shape, the bearing and what followed, and lets a hail that was out go"));
  FS47Happenings E(nullptr);E.ResetBig();
  Check(E.SaveBig()==nullptr&&E.BigHappened()==0,TEXT("a game that has not reached it saves nothing of it"));
 }
 // The rhythm's file has it, after two, on the night system, never a danger.
 {
  FS47RhythmData D;FString Why;FString Text;
  if(FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("Director/director.json")))&&FS47Rhythm::Parse(Text,D,Why))
  {
   const FS47RhythmEvent* E=D.Find(TEXT("big_night"));
   Check(E&&E->System==TEXT("night")&&E->Level!=ES47RhythmLevel::Danger&&E->FromDay==2&&E->bGround&&E->Hours==ES47RhythmHours::Window&&E->From>=2&&E->To<=5,TEXT("director.json: on the night system, on the ground, no danger, from day 2, in a window after two"));
  }
  else Lines.Add(TEXT("PASS big night: no director.json beside this build to read"));
 }
 // The words: nothing of Story's far side, no names or brands, nothing that tells the answer, and the numbers never Story's.
 {
  TArray<FString> All;
  for(const S47Big::FWords& W:S47Big::Words)for(const TCHAR* T:{W.Seen,W.DaleLine,W.Caller,W.Earl,W.Cb,W.Passed,W.Title,W.Mark,W.Lead,W.Found})All.Add(T);
  for(const TCHAR* T:{S47Big::DaleFirst,S47Big::DaleSlept,S47Big::DaleLast,S47Big::Slip})All.Add(T);
  // (his hail names him as a call sign does, "SARO, Dale.", as S47FieldRadio's own; Clean keeps "Dale" out of every other line)
  for(const TCHAR* T:{S47Big::HailFirst,S47Big::HailAgain,S47Big::HailGiveUp})All.Add(FString(T).Replace(TEXT("SARO, Dale."),TEXT("SARO.")));
  FString Bad;for(const FString& T:All)if(!S47Hap::Clean(T)&&Bad.Len()<200)Bad+=T.Left(50)+TEXT(" / ");
  Check(Bad.IsEmpty(),TEXT("every line is clean of Story's far side, names and brands ")+Bad);
  Check(FString(S47Big::DaleFirst).StartsWith(TEXT("Herrgott."))&&FString(S47Big::DaleSlept).StartsWith(TEXT("Herrgott."))&&S47Hap::Clean(S47Big::DaleFirst)
   &&S47Big::Dale(S47Big::Put(S47Big::DaleSlept,TEXT("the dry lake"),FString(),FString(),132,4)).Contains(TEXT("DALE: Herrgott.")),
   TEXT("his morning starts \"Herrgott.\" whether the night was seen or slept through, and nothing takes it out"));
  // CRIT6-20261109: Dale never calls the player Reyes (the one who went missing), in any line of his here.
  FString Named;
  for(const TCHAR* T:{S47Big::DaleFirst,S47Big::DaleSlept,S47Big::DaleLast,S47Big::Slip,S47Big::HailFirst,S47Big::HailAgain,S47Big::HailGiveUp})if(FString(T).Contains(TEXT("Reyes")))Named+=FString(T).Left(40)+TEXT(" / ");
  for(const S47Big::FWords& W:S47Big::Words)if(FString(W.DaleLine).Contains(TEXT("Reyes")))Named+=FString(W.DaleLine).Left(40)+TEXT(" / ");
  Check(Named.IsEmpty(),TEXT("CRIT6: no line of Dale's in the big night says Reyes ")+Named);
  bool bNum=true;for(int32 Az=0;Az<360;++Az){const FString S=FString::Printf(TEXT("%03.0f"),S47Hap::Bearing(Az));bNum&=!S.Contains(TEXT("47"));}
  Check(bNum,TEXT("no bearing it gives reads as Story's number"));
  Check(S47Big::ClimaxEnd-S47Big::Tone!=47&&S47Big::Len!=47&&S47Big::Back-S47Big::Flick!=47,TEXT("nothing in it lasts Story's number of seconds"));
 }
 return OK;
}

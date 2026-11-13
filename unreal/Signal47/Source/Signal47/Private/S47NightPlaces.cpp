#include "S47Happenings.h"
#include "S47Ambience.h"
#include "S47Audio.h"
#include "S47Drive.h"
#include "S47EventPlaces.h"
#include "S47Flashlight.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47GasBayDoor.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47NightRhythm.h"
#include "S47NightTiers.h"
#include "S47Rhythm.h"
#include "S47Script.h"
#include "S47StoryDirector.h"
#include "S47FieldRadio.h"
#include "S47Places.h"
#include "S47Walker.h"
#include "S47Weather.h"
#include "S47Web.h"
#include "S47World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47NightPlaces,Log,All);

// NIGHT31-PLACES-20261114 (TODO NIGHT31-20261108 and ANYWHERE-20261108; S47Happenings.h has the list). Nine more of the night's
// happenings, at places of the catalogue beyond the farm, made of what the world has: the recorded knocks and creaks, the game's own
// lamps, the gas station's bay door, the needle's history, the night log. Nothing new to learn: each is heard, seen by its own light,
// or found close to by day or by the lamp, and each one noticed is kept as a night record. Tiers, weeks, chances and hours are
// director.json's (system "night"); PlanTonight draws them, Due and the rhythm hold them to their hours, BeginPlace to where the
// player is. Text: no kind of being named, nothing explained, no digits but the clock (never on the minute that reads as Story's).
namespace S47Hap
{
 bool Clean(const FString& Text);
 bool Concluding(const FString& Text);
 bool Background(const FString& Id);
 const TCHAR* SiteKindOf(const FString& Id);
}
namespace S47NightPlacesLocal
{
 // How each that is heard is heard: the recording (and the one it alternates with), how loud, how fast, how muffled, every how many
 // seconds, in bursts of how many and how far apart, how high over its place, and the band of distance it starts and is heard in.
 struct FHear{const TCHAR* Sound;const TCHAR* SoundB;float Gain,Rate,RateB,Lp;double Every;int32 Burst;double Gap,Up,MinM,MaxM,StopM,Len;};
 const FHear* HearOf(const FString& Id)
 {
  // The pump rod of a windmill working: one knock down, a lighter one up. A swing's chain: a creak at each end of its swing. Down a
  // shaft: three slow knocks and a long wait. The bay door: its own sound (S47GasBayDoor) and the chain's rattle as it rolls.
  static const FHear Windmill={TEXT("metalTap"),TEXT("metalTap"),.5f,.62f,.56f,1600.f,1.7,1,0.,1.,45.,320.,28.,70.};
  static const FHear Swing={TEXT("doorCreak0"),TEXT("doorCreak1"),.22f,1.35f,1.25f,2600.f,1.25,1,0.,1.8,35.,240.,22.,60.};
  static const FHear Mine={TEXT("thudSoft"),TEXT("thudSoft"),.65f,.55f,.5f,500.f,9.,3,.8,-2.,25.,220.,25.,75.};
  static const FHear Bay={TEXT("metalTap"),TEXT("metalTap"),.22f,1.6f,1.5f,3000.f,99.,4,.45,2.,50.,280.,0.,10.};
  return Id==TEXT("windmill_knock")?&Windmill:Id==TEXT("school_swing")?&Swing:Id==TEXT("mine_knock")?&Mine:Id==TEXT("bay_door")?&Bay:nullptr;
 }
 // What the drawn light is, by the event's id.
 const TCHAR* LitKind(const FString& Id){return Id==TEXT("cemetery_lamp")?TEXT("lantern"):Id==TEXT("shack_lamp")?TEXT("shacklamp"):Id==TEXT("road_flare")?TEXT("flare"):TEXT("");}
 // The places a flare may be set out at: beside the highway.
 const TCHAR* const FlarePlaces[]={TEXT("road:cattle_guard"),TEXT("road:turnoff"),TEXT("road:pullout_north"),TEXT("road:pullout_south")};
 // A lamp's strength, 0 to 1 and a little over, for its age in seconds (S47 strobe rule: never more than three swells a second, never
 // dark, steady with the calm-flash setting). The lamp carried among the graves sways slowly; the shack's is all but steady; the
 // flare wavers, at under two and a half a second.
 float Flicker(const FString& Id,double Age,bool bCalmFlash)
 {
  if(bCalmFlash)return 1.f;
  const double Tau=2.*UE_DOUBLE_PI;
  if(Id==TEXT("cemetery_lamp"))return float(.9+.1*FMath::Sin(Age*Tau*.55));
  if(Id==TEXT("shack_lamp"))return float(.95+.05*FMath::Sin(Age*Tau*.3));
  if(Id==TEXT("road_flare"))return float(.82+.1*FMath::Sin(Age*Tau*1.3)+.08*FMath::Sin(Age*Tau*2.1+1.));
  return 1.f;
 }
 // In Caliche's street (as FS47Happenings::SituationNow has it): road-local, on the town's ground, not at the wheel.
 bool InTown(const FString& Area,double X,double Z){return (Area==TEXT("road")||Area==TEXT("diner"))&&Z>1800&&Z<2450&&FMath::Abs(X)<320;}
 // What the voice that repeats you (cb_echo) may say back: what the player did.
 FString DeedOf(const FString& Id,int32 Outcome)
 {
  const bool bWent=Outcome>=1;
  if(Id==TEXT("windmill_knock"))return bWent?TEXT("You walked up to the windmill on the Baca lease while it was knocking."):TEXT("You listened to the windmill knocking out on the Baca lease.");
  if(Id==TEXT("school_swing"))return bWent?TEXT("You walked over to the swings at the school."):TEXT("You listened to something creak on the school playground.");
  if(Id==TEXT("mine_knock"))return bWent?TEXT("You walked up to the shaft of the old mine."):TEXT("You listened to the knocking down in the old mine.");
  if(Id==TEXT("bay_door"))return TEXT("You heard the bay door at the gas station go up.");
  if(Id==TEXT("cemetery_lamp"))return bWent?TEXT("You walked to the cemetery gate after a lamp."):TEXT("You watched a lamp move among the graves.");
  if(Id==TEXT("shack_lamp"))return bWent?TEXT("You walked out to the line shack when its lamp was lit."):TEXT("You saw a lamp lit in the line shack.");
  if(Id==TEXT("road_flare"))return bWent?TEXT("You stopped for a flare burning on the shoulder."):TEXT("You drove past a flare burning on the shoulder.");
  if(Id==TEXT("cairn_stones"))return TEXT("You looked at the stones around the cairn on the rise.");
  if(Id==TEXT("trough_birds"))return TEXT("You looked at the dead birds around the trough.");
  return FString();
 }
 // The ones whose record says there was no wind: only on a still night (FS47Happenings::PlaceStill).
 bool NeedsStill(const FString& Id){return Id==TEXT("windmill_knock")||Id==TEXT("school_swing");}
 // How long a mark stays out there: the cairn's until eight two evenings on, the birds' a day more, so that the evenings it may be
 // talked of (PlaceTalkNow) have it still there; the sane cap on waiting to be said or found.
 double MarkUntil(bool bCairn,double Clock){return FS47FreeformSession::NightOf(Clock)*86400.+(bCairn?44.:68.)*3600.;}
 // The words a record may not have, as whole words, beyond S47Script::LintText and S47Hap::Clean (the brief's list for player text).
 bool Plain(const FString& Text)
 {
  const FString L=Text.ToLower();
  for(const TCHAR* W:{TEXT("taken"),TEXT("47"),TEXT("1947"),TEXT("tomas"),TEXT("silhouette"),TEXT("cable"),TEXT("8.15")})
  {
   const int32 N=FCString::Strlen(W);
   for(int32 At=L.Find(W);At!=INDEX_NONE;At=L.Find(W,ESearchCase::IgnoreCase,ESearchDir::FromStart,At+1))
   {
    const bool bBefore=At>0&&FChar::IsAlnum(L[At-1]),bAfter=At+N<L.Len()&&FChar::IsAlnum(L[At+N]);
    if(!bBefore&&!bAfter)return false;
   }
  }
  // Nor a capital C standing by itself.
  for(int32 I=0;I<Text.Len();++I)if(Text[I]==TEXT('C')&&(I==0||!FChar::IsAlnum(Text[I-1]))&&(I+1>=Text.Len()||!FChar::IsAlnum(Text[I+1])))return false;
  return true;
 }
}

const TArray<FS47Happenings::FPlaceDef>& FS47Happenings::PlaceDefs()
{
 // Id, place, shape, first night, the usual chance (director.json's per_night has the say), the drawn hours, whether a calm night may
 // have it (only what is heard), and its record's key.
 static const TArray<FPlaceDef> L={
  {TEXT("windmill_knock"),TEXT("windmill"),EPlaceShape::Heard,1,.22f,20.5,3.,true,TEXT("windmill_knock_d")},
  {TEXT("school_swing"),TEXT("school"),EPlaceShape::Heard,2,.2f,20.5,3.5,true,TEXT("school_swing_d")},
  {TEXT("mine_knock"),TEXT("mine"),EPlaceShape::Heard,6,.2f,24.25,2.75,false,TEXT("mine_knock_d")},
  {TEXT("bay_door"),TEXT("gas"),EPlaceShape::Heard,3,.15f,21.,5.,true,TEXT("bay_door_d")},
  {TEXT("cemetery_lamp"),TEXT("cemetery"),EPlaceShape::Lit,4,.18f,21.5,3.5,false,TEXT("cemetery_lamp_d")},
  {TEXT("shack_lamp"),TEXT("shack"),EPlaceShape::Lit,2,.18f,21.,4.5,false,TEXT("shack_lamp_d")},
  {TEXT("road_flare"),TEXT(""),EPlaceShape::Lit,2,.2f,21.,5.5,false,TEXT("road_flare_d")},
  {TEXT("cairn_stones"),TEXT("cairn"),EPlaceShape::Mark,5,.16f,24.5,3.,false,TEXT("cairn_stones_d")},
  {TEXT("trough_birds"),TEXT("trough"),EPlaceShape::Mark,9,.14f,26.25,2.25,false,TEXT("trough_birds_d")},
 };
 return L;
}
const FS47Happenings::FPlaceDef* FS47Happenings::PlaceDef(const FString& Id)
{
 for(const FPlaceDef& D:PlaceDefs())if(Id==D.Id)return &D;
 return nullptr;
}
uint32 FS47Happenings::PlaceSalt(const FString& Id){return FCrc::StrCrc32(*Id)^0x504c4143u;}
FString FS47Happenings::PlaceClock(double Clock)
{
 const int32 S=int32(FMath::Fmod(FMath::Max(0.,Clock),86400.));
 int32 Mi=(S/60)%60;if(Mi==47)Mi=48;
 return FString::Printf(TEXT("%02d:%02d"),S/3600,Mi);
}
FString FS47Happenings::PlaceRecord(const FString& Id,const FString& Clock,int32 Outcome,const FString& Title)
{
 const bool bWent=Outcome==1;
 if(Id==TEXT("windmill_knock"))
  return FString::Printf(TEXT("At %s %s was knocking out in the dark, the pump rod going down and up, slow and even. There was hardly any wind.%s"),*Clock,*Title,
   bWent?TEXT(" It stopped before I got to it, and the wheel was standing still."):TEXT(" I did not go over."));
 if(Id==TEXT("school_swing"))
  return FString::Printf(TEXT("At %s something on the playground at the school creaked, slow and even, the way a swing does with somebody on it. There was no wind.%s"),*Clock,
   bWent?TEXT(" When I got to the fence the swings were hanging still."):TEXT(" I did not go over."));
 if(Id==TEXT("mine_knock"))
  return FString::Printf(TEXT("At %s there was knocking down in the old mine on the west side, three at a time and slow, like a fist on timber, then a long wait.%s"),*Clock,
   bWent?TEXT(" It stopped when I came up to the shaft, and did not start again."):TEXT(""));
 if(Id==TEXT("bay_door"))
  return FString::Printf(TEXT("At %s the roll-up door on the service bay at the gas station went up. No light came on in the bay, and nobody came out."),*Clock);
 if(Id==TEXT("cemetery_lamp"))
  return FString::Printf(TEXT("At %s a lamp was moving among the graves behind the church, slow, at a walking pace, low to the ground.%s"),*Clock,
   bWent?TEXT(" When I got to the gate it was out, and nobody came past me."):TEXT(""));
 if(Id==TEXT("shack_lamp"))
  return FString::Printf(TEXT("At %s there was a lamp lit in the window of the line shack, a long way out.%s"),*Clock,
   bWent?TEXT(" It went out before I got there. The stove inside was cold, and the lamp chimney was still warm."):TEXT(""));
 if(Id==TEXT("road_flare"))
  return FString::Printf(TEXT("At %s a road flare was burning on the shoulder by %s. No car anywhere, and nobody near it.%s"),*Clock,*Title,
   bWent?TEXT(" It burned down to nothing as I came up."):TEXT(""));
 if(Id==TEXT("cairn_stones"))
  return FString::Printf(TEXT("At %s at the cairn on the rise: eight stones laid on the ground in a ring around it, evenly spaced, the dirt under them still dark from the night.%s"),*Clock,
   Outcome==2?TEXT(" In the night I had heard stones knock together out that way."):TEXT(""));
 if(Id==TEXT("trough_birds"))
  return FString::Printf(TEXT("At %s at the salt lick and trough: small birds lying dead all around the trough, fourteen of them, on their backs. Not a mark on any of them. The water was still."),*Clock);
 return FString();
}

// ---------------------------------------------------------------- starting one
bool FS47Happenings::BeginPlace(const FString& Id,bool bForced,FString& Why)
{
 using namespace S47NightPlacesLocal;
 const FPlaceDef* Def=PlaceDef(Id);
 auto* F=M?M->Freeform.Get():nullptr;
 if(!Def||!F||!M->Player||!M->Web||!M->World){Why=TEXT("Freeform only");return false;}
 const auto No=[&Why](const TCHAR* Text){Why=Text;return false;};
 const double Clock=F->Clock();
 const bool bDrive=Driving(),bOut=Outdoors()&&!bDrive;
 const FString Area=M->Web->Area();
 const double Hd=FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;
 if(!bForced&&!(Hd>=19.5||Hd<5.5))return No(TEXT("not in the dark"));
 if(!PlaceMayRun(bForced,FS47StoryDirector::Of(M)!=nullptr))return No(TEXT("not on a Story night"));
 double Px=0,Pz=0;const bool bWhere=PlayerRoad(Px,Pz);
 const FS47EventPlace* P=nullptr;
 if(*Def->Place)
 {
  P=F->EventPlaces?F->EventPlaces->Find(Def->Place):nullptr;
  if(!P)return No(TEXT("its place is not in the catalogue"));
 }
 const double Away=P&&bWhere?FVector2D::Distance(FVector2D(P->X,P->Z),FVector2D(Px,Pz)):-1.;
 const auto Band=[&](double MinM,double MaxM){return bForced||(Away>=MinM&&Away<=MaxM);};
 const auto Ask=[&]{return bForced||MayStart(Id);};
 const auto Arm=[&](double Len)
 {
  Now=FActive();Now.Id=Id;Now.Len=Len;Now.bForced=bForced;Now.Serial=Serial+1;Now.Clock=Clock;
  if(P){Now.Place=P->Id;Now.Title=P->Title;Now.X=P->X;Now.Z=P->Z;}
 };
 // A still night: the knocking and the creaking are no wind's (S47Weather, the hour's own).
 const double WindMph=S47Weather::Now(F->Seed(),int32(FMath::FloorToDouble(Clock/86400.))+1,FMath::Fmod(Clock,86400.)).Mph;
 if(Def->Shape==EPlaceShape::Heard)
 {
  const FHear* H=HearOf(Id);if(!H)return No(TEXT("no such happening"));
  if(!bOut&&!bForced)return No(TEXT("not out on foot"));
  const bool bTownPlace=Id==TEXT("school_swing")||Id==TEXT("bay_door");
  if(bTownPlace&&!bForced&&!(bWhere&&InTown(Area,Px,Pz)))return No(TEXT("not in Caliche"));
  if(!Band(H->MinM,H->MaxM))return No(TEXT("not within earshot of its place, or too near it"));
  if(!bForced&&NeedsStill(Id)&&!PlaceStill(WindMph))return No(TEXT("too windy tonight: it would be the wind"));
  if(Id==TEXT("bay_door"))
  {
   if(!S47GasBayDoor::Built())return No(TEXT("the gas station is not in this scene"));
   if(S47GasBayDoor::IsOpen())return No(TEXT("the bay door is up already"));
  }
  FVector At;if(!WorldOf(P->X,P->Z,At))return No(TEXT("its place is not in this area"));
  if(!Ask())return No(TEXT("the rhythm says not now"));
  Arm(H->Len);Now.At=At+FVector(0,H->Up,0);Now.Yaw=.4;
  return true;
 }
 if(Def->Shape==EPlaceShape::Lit)
 {
  const TCHAR* Kind=LitKind(Id);
  if(Id==TEXT("road_flare"))
  {
   double Hx=0,Hz=0,Speed=0;
   if(!OnHighway(Hx,Hz,Speed))return No(TEXT("not on the highway"));
   if(!bForced&&FMath::Abs(Speed)<8)return No(TEXT("the truck is hardly moving"));
   // Which way the road runs ahead of the truck (as the crossing far ahead has it): its heading, measured ten metres on.
   const FS47DriveController* D=M->World->CabDrive();if(!D)return No(TEXT("not in the truck"));
   const FVector Ahead(-FMath::Sin(D->Heading),0,-FMath::Cos(D->Heading));
   double Dir=Ahead.Z<0?-1.:1.;
   {double Ax=0,Az=0;if(RoadOfWorld(D->Pos+Ahead*10.,Ax,Az)&&!FMath::IsNearlyEqual(Az,Hz,.5))Dir=Az<Hz?-1.:1.;}
   const FS47EventPlace* Best=nullptr;double BestAhead=1e9;
   if(F->EventPlaces)for(const TCHAR* Pid:FlarePlaces)if(const FS47EventPlace* E=F->EventPlaces->Find(Pid))
   {
    const double Ah=(E->Z-Hz)*Dir;
    if(Ah>=(bForced?120.:250.)&&Ah<=(bForced?3000.:700.)&&Ah<BestAhead){BestAhead=Ah;Best=E;}
   }
   if(!Best&&!bForced)return No(TEXT("no pull-off ahead at the right distance"));
   if(!Prop(Kind))return No(TEXT("nothing to draw it with"));
   if(!Ask())return No(TEXT("the rhythm says not now"));
   Arm(200);
   if(Best){Now.Place=Best->Id;Now.Title=Best->Title;Now.X=Best->X<0?-7.:7.;Now.Z=Best->Z;}
   else{Now.Title=TEXT("the highway");Now.X=7.;Now.Z=Hz+Dir*300.;}
   Now.Dir=FVector(0,0,Dir);
   return true;
  }
  if(!bOut&&!(Id==TEXT("shack_lamp")&&bDrive)&&!bForced)return No(TEXT("not out where it can be seen"));
  if(Id==TEXT("cemetery_lamp")&&!bForced&&!(bWhere&&InTown(Area,Px,Pz)))return No(TEXT("not in Caliche"));
  if(!Band(Id==TEXT("cemetery_lamp")?60.:150.,Id==TEXT("cemetery_lamp")?450.:1300.))return No(TEXT("not in sight of its place, or too near it"));
  FVector At;if(!WorldOf(P->X,P->Z,At))return No(TEXT("its place is not in this area"));
  // Only with its place in front of the player (the cab's nose, or where he looks): a light behind him burns for nobody.
  if(!bForced&&!Seen(At+FVector(0,1,0),1400,PlaceStartDeg))return No(TEXT("its place is not in front of the player"));
  if(!Prop(Kind))return No(TEXT("nothing to draw it with"));
  if(!Ask())return No(TEXT("the rhythm says not now"));
  Arm(Id==TEXT("cemetery_lamp")?240.:420.);
  // The shack's window turned to where the player stood when it was lit.
  Now.Yaw=FMath::Atan2(M->Player->Pos.X-At.X,M->Player->Pos.Z-At.Z);
  return true;
 }
 // A mark: left out there while nobody is near it, and found later.
 const bool bCairn=Id==TEXT("cairn_stones");
 const TCHAR* SiteKind=bCairn?TEXT("cairnring"):TEXT("deadbirds");
 if(HasSite(SiteKind))return No(TEXT("one is out there already"));
 if(!bForced&&Away>=0&&Away<(bCairn?250.:400.))return No(TEXT("the player is too near its place"));
 if(!Ask())return No(TEXT("the rhythm says not now"));
 FSite S;S.Kind=SiteKind;S.Place=P->Id;S.Title=P->Title;S.Night=F->Day();S.Serial=Serial+1;S.X=P->X;S.Z=P->Z;
 S.Yaw=FS47NightRhythm::Roll(F->Seed(),F->Day(),PlaceSalt(Id))*2*UE_DOUBLE_PI;
 // The cairn's until eight the evening after; the birds a day more, so that the rancher's word the next evening is about something there.
 S.Until=MarkUntil(bCairn,Clock);
 // Within earshot on foot (not on top of it): the stones knocking together out that way, a few at a time, dry.
 if(bCairn&&bOut&&Away>=0&&Away<=CairnHeardM)
 {
  FVector At;
  if(WorldOf(P->X,P->Z,At)&&M->Audio)
  {
   for(int32 K=0;K<5;++K){FS47Sound O;O.At=At+FVector(0,.5,0);O.Gain=.35f;O.Rate=1.4f+.08f*K;O.Lp=1800;O.When=.17f*K+.09f*(K%2);M->Audio->Sfx(TEXT("thudSoft"),O);}
   S.bHeard=true;
  }
 }
 Sites.Add(S);UsedPlace(P->Id,Id);
 if(S.bHeard)Noticed(Id,Away,TEXT("sound"),S.Serial);
 UE_LOG(LogS47NightPlaces,Display,TEXT("S47: night place %s left at %s, %.0f m from the player%s"),*Id,*S.Title,Away,S.bHeard?TEXT(", heard"):TEXT(""));
 return true;
}

// ---------------------------------------------------------------- while it holds the ground
bool FS47Happenings::StepPlace(double Dt)
{
 using namespace S47NightPlacesLocal;
 static_cast<void>(Dt);
 const FPlaceDef* Def=PlaceDef(Now.Id);
 if(!Def||Def->Shape==EPlaceShape::Mark)return false;
 // Nothing burns on into the morning: a sleep or a hold that ran past half past five, or into the next day, ends it as it stands.
 if(M->Freeform&&PlaceOver(Now.Clock,M->Freeform->Clock())){EndPlace(Now.Stage==2?1:0);return true;}
 US47Audio* A=M->Audio.Get();
 FVector Here;
 if(!WorldOf(Now.X,Now.Z,Here)){EndPlace(0);return true;}
 const bool bLampOn=M->Night&&M->Night->Flashlight&&M->Night->Flashlight->bOn;
 if(Def->Shape==EPlaceShape::Heard)
 {
  const FHear* H=HearOf(Now.Id);if(!H){EndPlace(0);return true;}
  const FVector At=Here+FVector(0,H->Up,0);Now.At=At;
  const double Away=AwayFrom(At);
  if(Now.Stage==0&&Now.Age>=Now.Yaw)
  {
   Now.Yaw=Now.Age+H->Every;
   const bool bOther=(int32(Now.Dir.X)%2)==1;
   // The bay door: raised by nobody, with its own sound at the leaf, and the chain's rattle as it rolls up.
   if(Now.Id==TEXT("bay_door")&&Now.Dir.X<.5)S47GasBayDoor::SetOpen(true,false);
   if(A)for(int32 K=0;K<H->Burst;++K)
   {
    FS47Sound O;O.At=At;O.Gain=H->Gain;O.Rate=bOther?H->RateB:H->Rate;O.Lp=H->Lp;O.When=float(K*H->Gap);
    A->Sfx(bOther?H->SoundB:H->Sound,O);
   }
   Now.Dir.X+=1;
   if(!Now.bSeen&&PlaceHeardAt(Away,H->MaxM))
   {
    Now.bSeen=true;Noticed(Now.Id,Away,TEXT("sound"));
    if(!Now.bForced&&M->Rhythm)M->Rhythm->Began(FName(*Now.Id));
   }
   // The bay door stays up whatever comes after (a save, a sleep): its record is kept as it goes up.
   if(Now.Id==TEXT("bay_door")&&Now.bSeen)KeepPlace(0);
  }
  // Come up to it, or the lamp on the swings: it stops, and the crickets with it a while.
  const bool bLampOnIt=Now.Id==TEXT("school_swing")&&bLampOn&&Seen(At,60,10);
  if(Now.Stage==0&&H->StopM>0&&(Away<H->StopM||bLampOnIt))
  {
   Now.Stage=2;Now.Len=FMath::Min(Now.Len,Now.Age+1.5);
   if(A&&A->Night)A->Night->Hush(6,.05,.2,2.5);
  }
  if(Now.Age>=Now.Len)EndPlace(Now.Stage==2?1:0);
  return true;
 }
 // Seen by its own light.
 const FString Kind=LitKind(Now.Id);
 FProp* Pp=Prop(Kind);
 FVector At=Here;double Turn=Now.Yaw;
 const bool bFlare=Now.Id==TEXT("road_flare"),bGraves=Now.Id==TEXT("cemetery_lamp");
 if(bGraves)
 {
  // Carried at a walk among the graves, low, back and forth.
  const double Tau=2.*UE_DOUBLE_PI,Dx=6.*FMath::Sin(Now.Age*Tau/50.),Dz=3.*FMath::Sin(Now.Age*Tau/31.);
  if(!WorldOf(Now.X+Dx,Now.Z+Dz,At))At=Here;
  At.Y+=.75;Turn=0;
 }
 else if(bFlare){At.Y+=.02;Turn=0;}
 else At.Y+=1.4;
 const double Away=AwayFrom(At);
 const bool bBurning=Now.Stage<2||(bFlare&&Now.Age<Now.Dir.Y+3.);
 if(Pp){PoseProp(*Pp,At,Turn);ShowProp(*Pp,bBurning);}
 float K=Flicker(Now.Id,Now.Age,M->Flow&&M->Flow->bCalmFlash);
 if(bFlare&&Now.Stage==2)K*=float(FMath::Clamp(1.-(Now.Age-Now.Dir.Y)/3.,0.,1.));
 if(Pp)PropLamps(*Pp,K);
 if(!Now.bSeen&&Now.Stage<2&&SeenLit(At+FVector(0,.1,0),bFlare?750.:bGraves?480.:1350.,bFlare?30.:PlaceSeenDeg,LitOwn,*Kind))
 {
  Now.bSeen=true;Noticed(Now.Id,Away,TEXT("sight"));
  if(!Now.bForced&&M->Rhythm)M->Rhythm->Began(FName(*Now.Id));
 }
 if(Now.Stage<2)
 {
  if(bFlare)
  {
   // Stopped by it, or walked up to it: it burns down. Driven past: it is behind, and that is all.
   const FS47DriveController* D=Driving()?M->World->CabDrive():nullptr;
   if((D&&FMath::Abs(D->Speed)<1&&Away<45)||(!D&&Away<15)){Now.Stage=2;Now.Dir.Y=Now.Age;Now.Len=FMath::Min(Now.Len,Now.Age+3.5);}
   else if(D){double Rx=0,Rz=0;if(RoadOfWorld(D->Pos,Rx,Rz)&&(Rz-Now.Z)*Now.Dir.Z>90)Now.Len=FMath::Min(Now.Len,Now.Age+.1);}
  }
  else if(Away<(bGraves?35.:30.))
  {
   Now.Stage=2;Now.Len=FMath::Min(Now.Len,Now.Age+1.);
   if(A&&!bGraves){FS47Sound O;O.At=At;O.Gain=.2f;O.Rate=1.2f;O.Lp=1800;A->Sfx(TEXT("thudSoft"),O);}
  }
 }
 if(Now.Age>=Now.Len)EndPlace(Now.Stage==2?1:0);
 return true;
}
void FS47Happenings::KeepPlace(int32 Outcome)
{
 using namespace S47NightPlacesLocal;
 auto* F=M?M->Freeform.Get():nullptr;
 const FPlaceDef* Def=PlaceDef(Now.Id);
 if(!F||!Def||!Now.bSeen||Now.bDone)return;
 Now.bDone=true;
 const FString Key=FString::Printf(TEXT("%s%d"),Def->Key,F->Day());
 F->RecordEvidence(Key,PlaceRecord(Now.Id,PlaceClock(Now.Clock),Outcome,Now.Title));
 Deed(DeedOf(Now.Id,Outcome));
 // LOOP-4-20261109: the needle remembers where it was (FS47Places::Use, two nights).
 if(!Now.bForced){Past.Add({Now.X,Now.Z,F->Day()});while(Past.Num()>MostPast)Past.RemoveAt(0);}
 UE_LOG(LogS47NightPlaces,Display,TEXT("S47: night place %s kept as %s (%s)"),*Now.Id,*Key,Outcome==1?TEXT("walked up to"):TEXT("from off"));
 M->RequestAutosave();
}
void FS47Happenings::EndPlace(int32 Outcome)
{
 KeepPlace(Outcome);
 // One nobody noticed did not happen for the night: its day is given back, so that it may still come while its window lasts (the
 // rhythm was never told it began).
 const FString Id=Now.Id;const bool bUnnoticed=!Now.bSeen&&!Now.bForced;
 const int32* Before=LastDays.Find(Id);const int32 Prev=Before?*Before:0;
 EndEvent(Now.bSeen);
 if(bUnnoticed)
 {
  if(Prev>0)LastDays.Add(Id,Prev);else LastDays.Remove(Id);
  UE_LOG(LogS47NightPlaces,Display,TEXT("S47: night place %s ended unnoticed: neither its day nor its start is spent"),*Id);
 }
}
bool FS47Happenings::PlaceOver(double StartClock,double Clock)
{
 return NightHour(Clock)>=29.5||FS47FreeformSession::NightOf(Clock)!=FS47FreeformSession::NightOf(StartClock);
}
FString FS47Happenings::PlaceWord(const FString& Id,const FString& Named,bool bPassed)
{
 if(Id==TEXT("windmill_knock"))return bPassed?FString(TEXT("A driver says the windmill on the Baca lease is knocking tonight, with no wind to turn it."))
  :FString(TEXT("Who's working the Baca pump this late? I can hear that rod knocking clear from the highway, and there's no wind to turn it."));
 if(Id==TEXT("mine_knock"))return bPassed?FString(TEXT("A driver heard knocking from the old mine on the west side, three at a time."))
  :FString(TEXT("Anybody up by the old mine on the west side? Something's knocking down that shaft, three at a time. Carries a long way tonight."));
 if(Id==TEXT("school_swing"))return bPassed?FString(TEXT("A driver heard the swings going at the school this late, and no wind."))
  :FString(TEXT("Somebody ought to tell the sheriff there's kids on the swings at the school this late. Heard them from the highway."));
 if(Id==TEXT("cairn_stones"))return bPassed?FString::Printf(TEXT("A rancher says somebody has laid stones out in a ring around %s."),*Named)
  :FString::Printf(TEXT("Somebody's been fooling with %s. Took the rocks and laid them out in a ring around it, neat as you please."),*Named);
 if(Id==TEXT("trough_birds"))return bPassed?FString::Printf(TEXT("A rancher found dead birds all around his trough out by %s. Not a mark on them."),*Named)
  :FString::Printf(TEXT("Anybody else find dead birds at their water? A whole ring of them around my stock tank out by %s, and not a mark on one."),*Named);
 return FString();
}

// ---------------------------------------------------------------- the marks
void FS47Happenings::StepPlaceMarks()
{
 using namespace S47NightPlacesLocal;
 auto* F=M?M->Freeform.Get():nullptr;if(!F)return;
 const double Clock=F->Clock(),Hn=HourOfNight(Clock);
 // A word goes out only where it is heard at once (the CB in the seat, or Dale's walkie): otherwise it waits for another evening.
 const bool bEars=CanHearCb()||(M->FieldRadio.IsValid()&&M->FieldRadio->Owned());
 for(FSite& S:Sites)
 {
  const bool bCairn=S.Kind==TEXT("cairnring"),bBirds=S.Kind==TEXT("deadbirds");
  if(!bCairn&&!bBirds)continue;
  const FString Id=bCairn?TEXT("cairn_stones"):TEXT("trough_birds");
  FVector At;
  // Found close to, on foot, by day or in the lamp.
  if(!S.bFound&&WorldOf(S.X,S.Z,At))
  {
   const double Away=AwayFrom(At);
   if(Away<9&&!Driving()&&SeenLit(At+FVector(0,.1,0),10,50,LitGround,*S.Kind))
   {
    S.bFound=true;
    Noticed(Id,Away,TEXT("sight"),S.Serial);
    F->RecordEvidence(FString::Printf(TEXT("%s%d"),bCairn?TEXT("cairn_stones_d"):TEXT("trough_birds_d"),S.Night),PlaceRecord(Id,PlaceClock(Clock),bCairn&&S.bHeard?2:1,S.Title));
    Deed(DeedOf(Id,1));
    UE_LOG(LogS47NightPlaces,Display,TEXT("S47: night place %s found at %.0f m"),*Id,Away);
    M->RequestAutosave();
   }
  }
  // Not found: whoever it belongs to says so on an evening after (19:30 to 01:00, any evening until it is said or the mark is gone), and
  // the place goes on the map as heard of, which points out there by day.
  if(!S.bFound&&!S.bKnown&&bEars&&PlaceTalkNow(Hn,S.Night,F->Day())&&Now.Id.IsEmpty()&&Paced>=RumorTry&&!FS47StoryDirector::Of(M))
  {
   RumorTry=Paced+40;S.bKnown=true;
   const FString Named=Spoken(S.Title);
   Tip({PlaceWord(Id,Named,false)},TEXT("old"),PlaceWord(Id,Named,true));
   if(F->Places)F->Places->Hear(S.Place);
   UE_LOG(LogS47NightPlaces,Display,TEXT("S47: night place %s at %s: talked of on an evening after"),*Id,*S.Title);
   M->RequestAutosave();
  }
 }
}
// One heard far off: tonight's draw for it has come, the player is too far off to hear it (at SARO, at the wheel, across the land), and a
// driver on the CB or Dale on the walkie says so, once a night; its place goes on the map as heard of. The thing itself still plays if he
// goes out there in its window (Due: its day is not spent by the word). Not the bay door (it would have to be up), never on a Story night.
void FS47Happenings::StepPlaceWords()
{
 using namespace S47NightPlacesLocal;
 auto* F=M?M->Freeform.Get():nullptr;
 if(!F||!F->RulesLive()||F->InTutorial()||!Now.Id.IsEmpty()||Paced<RumorTry||FS47StoryDirector::Of(M))return;
 if(!(CanHearCb()||(M->FieldRadio.IsValid()&&M->FieldRadio->Owned())))return;
 const double Clock=F->Clock(),Hn=HourOfNight(Clock);
 const int32 Night=FS47FreeformSession::NightOf(Clock);
 double Px=0,Pz=0;const bool bWhere=PlayerRoad(Px,Pz);
 for(const FPlaceDef& D:PlaceDefs())
 {
  if(D.Shape!=EPlaceShape::Heard||FString(D.Id)==TEXT("bay_door"))continue;
  const FString Id=D.Id;
  const double* From=Plan.Find(Id);
  if(!From||Hn<*From||Hn>=*From+DrawWindow||LastDays.FindRef(Id)==F->Day()||PlaceWordNight.FindRef(Id)==Night)continue;
  const FHear* H=HearOf(Id);const FS47EventPlace* P=F->EventPlaces?F->EventPlaces->Find(D.Place):nullptr;
  if(!H||!P)continue;
  const double Away=bWhere?FVector2D::Distance(FVector2D(P->X,P->Z),FVector2D(Px,Pz)):1e9;
  if(Away<=H->MaxM)continue; // near enough to hear it for himself
  if(NeedsStill(Id)&&!PlaceStill(S47Weather::Now(F->Seed(),int32(FMath::FloorToDouble(Clock/86400.))+1,FMath::Fmod(Clock,86400.)).Mph))continue;
  PlaceWordNight.Add(Id,Night);RumorTry=Paced+40;
  Tip({PlaceWord(Id,FString(),false)},TEXT("man"),PlaceWord(Id,FString(),true));
  if(F->Places)F->Places->Hear(P->Id);
  UE_LOG(LogS47NightPlaces,Display,TEXT("S47: night place %s told of on the air, the player %.0f m off"),*Id,Away);
  return;
 }
}

// ---------------------------------------------------------------- self-test
bool FS47Happenings::PlaceSelfTest(TArray<FString>& Lines)
{
 using namespace S47NightPlacesLocal;
 bool OK=true;
 const auto Check=[&](bool B,const FString& What){OK&=B;Lines.Add(FString::Printf(TEXT("%s happenings places: %s"),B?TEXT("PASS"):TEXT("FAIL"),*What));};
 // Each is one of the night's, at a place the catalogue has, with a record the night counts, and never a danger.
 {
  FS47EventPlaces Catalogue(nullptr);
  FString Bad;
  for(const FPlaceDef& D:PlaceDefs())
  {
   const FString Id=D.Id;
   bool bOne=Ids().Contains(Id)&&!IsDanger(Id)&&NeedMask(Id)!=0&&IsNightFinding(FString(D.Key)+TEXT("5"))&&NightFindingKind(FString(D.Key)+TEXT("5"))==Id;
   if(*D.Place)bOne&=Catalogue.Find(D.Place)!=nullptr&&D.Place!=FString(TEXT("farm"));
   else for(const TCHAR* Pid:FlarePlaces)bOne&=Catalogue.Find(Pid)!=nullptr;
   if(!bOne)Bad+=Id+TEXT(" ");
  }
  Check(PlaceDefs().Num()==9&&Bad.IsEmpty(),TEXT("the nine are the night's own, each at a place of the catalogue off the farm, none a danger, each kept as a night record ")+Bad);
  Check(NeedMask(TEXT("school_swing"))!=0&&FloorHeard(TEXT("bay_door"))&&FloorHeard(TEXT("mine_knock"))&&S47Hap::Background(TEXT("cairn_stones"))&&S47Hap::Background(TEXT("trough_birds"))&&!S47Hap::Background(TEXT("road_flare")),
   TEXT("the bay door and the mine are heard where the floor offers; the marks hold no ground, the rest do"));
 }
 // The rules (the embedded director.json): its tier, its first night, its hours; drawn on an eligible night by the planner's dice at an
 // hour the rhythm lets it start.
 {
  FS47Rhythm Rules(nullptr);Rules.UseEmbedded();
  const FS47RhythmData& Data=Rules.Data();
  const auto MayAt=[&Rules](const FString& Which,int32 Night,double NightHr)
  {
   FS47RhythmNow Asked;Asked.Day=Night;Asked.Clock=(Night-1)*86400.+NightHr*3600.;
   return Rules.May(FName(*Which),Asked,nullptr,true).bMay;
  };
  bool bRows=true,bWeeks=true,bDrawn=true,bInHours=true;FString Why;
  for(const FPlaceDef& D:PlaceDefs())
  {
   const FString Id=D.Id;
   const FS47RhythmEvent* E=Data.Find(FName(*Id));
   if(!E){bRows=false;Why+=Id+TEXT(" (no row) ");continue;}
   const ES47Allowed Want=Id==TEXT("trough_birds")?ES47Allowed::Full:Id==TEXT("mine_knock")||Id==TEXT("cairn_stones")?ES47Allowed::Glimpse:ES47Allowed::Hint;
   bRows&=E->System==TEXT("night")&&E->Level!=ES47RhythmLevel::Danger&&E->Tier==Want&&!E->bEarly&&E->FromDay==D.FromDay&&E->DrawFrom>0&&E->DrawTo>E->DrawFrom;
   // Its tier's first hour, in every week: nothing of a glimpse before midnight, nothing near before two.
   bRows&=E->DrawFrom>=(Want==ES47Allowed::Full?S47NightTiers::FullFrom:Want==ES47Allowed::Glimpse?S47NightTiers::GlimpseFrom:19.);
   const double Mid=(E->DrawFrom+E->DrawTo)*.5;
   if(D.FromDay>1)bWeeks&=!MayAt(Id,D.FromDay-1,Mid)&&MayAt(Id,D.FromDay,Mid);
   else bWeeks&=MayAt(Id,2,Mid);
   int32 Got=0;
   for(int32 Seed=1;Seed<=200;++Seed)for(int32 Night=2;Night<=30;++Night)
   {
    if(Night<D.FromDay||(FS47NightRhythm::Quiet(Seed,Night)&&!D.bCalm))continue;
    // PlanTonight's own draw: the file's chance for the night, times its week's worth for its level.
    float Base=E->PerNight>=0?float(E->PerNight):D.Usual;
    for(const TPair<int32,double>& Lt:E->Later)if(Night>=Lt.Key)Base=float(Lt.Value);
    const float Chance=FMath::Clamp(Base*Data.Worth(E->Level,Night),0.f,1.f);
    const uint32 Salt=PlaceSalt(Id);
    if(!(Chance>0&&FS47NightRhythm::Roll(Seed,Night,Salt)<Chance))continue;
    ++Got;
    const double At=E->DrawFrom+(E->DrawTo-E->DrawFrom)*FS47NightRhythm::Roll(Seed,Night,Salt+1);
    if(!MayAt(Id,Night,At)&&bInHours){bInHours=false;Why+=FString::Printf(TEXT("%s night %d at %.2f "),*Id,Night,At);}
   }
   if(Got<30){bDrawn=false;Why+=FString::Printf(TEXT("%s drawn %d "),*Id,Got);}
  }
  Check(bRows,TEXT("each is in director.json under the night system in its tier (hints; the mine and the cairn from midnight; the birds from two), in the week the table says, drawn inside its tier's hours ")+Why);
  Check(bWeeks,TEXT("none comes before its first night (the windmill alone in the first week, nothing dangerous in it)"));
  Check(bDrawn&&bInHours,TEXT("over 200 games each is drawn on eligible nights by the planner's own dice, and may start at the hour it is drawn for ")+Why);
  Check(!MayAt(TEXT("mine_knock"),8,23.5)&&MayAt(TEXT("mine_knock"),8,25.)&&!MayAt(TEXT("trough_birds"),12,25.5)&&MayAt(TEXT("trough_birds"),12,26.5)&&!MayAt(TEXT("school_swing"),5,26.),
   TEXT("the knocking in the mine is not before midnight, the birds not before two, the swings not after their evening"));
 }
 // The words: each record, deed and word of the rancher keeps to Freeform's limits, names nothing, and concludes nothing.
 {
  TArray<FString> Said;
  for(const FPlaceDef& D:PlaceDefs())for(int32 Out=0;Out<=2;++Out)
  {
   Said.Add(PlaceRecord(D.Id,PlaceClock(23*3600.+47*60.),Out,D.Id==FString(TEXT("windmill_knock"))?FString(TEXT("the windmill on the Baca lease")):FString(TEXT("the cattle guard on the highway"))));
   Said.Add(DeedOf(D.Id,Out));
  }
  int32 Words=0;
  for(const TCHAR* Which:{TEXT("windmill_knock"),TEXT("mine_knock"),TEXT("school_swing"),TEXT("cairn_stones"),TEXT("trough_birds")})for(const bool bPassed:{false,true})
  {
   const FString W=PlaceWord(Which,FCString::Strcmp(Which,TEXT("cairn_stones"))==0?FString(TEXT("the cairn on the rise")):FString(TEXT("the salt lick")),bPassed);
   Said.Add(W);Words+=W.IsEmpty()?0:1;
  }
  FString Bad;bool bAll=true;
  for(const FString& T:Said)
  {
   const bool bOk=!T.IsEmpty()&&S47Script::LintText(T).IsEmpty()&&S47Hap::Clean(T)&&!S47Hap::Concluding(T)&&Plain(T)&&!T.Contains(TEXT("%"));
   if(!bOk&&Bad.Len()<200)Bad+=T.Left(60)+TEXT(" | ");
   bAll&=bOk;
  }
  Check(bAll&&Said.Num()==64&&Words==10&&PlaceWord(TEXT("bay_door"),FString(),false).IsEmpty(),TEXT("every record, deed and word passes the lint (no Story spoilers, no clock on that minute, no kind named, no conclusion) ")+Bad);
  Check(PlaceClock(23*3600.+47*60.)==TEXT("23:48")&&PlaceClock(86400.+3600.+5*60.)==TEXT("01:05")&&PlaceRecord(TEXT("bay_door"),TEXT("23:10"),0,FString()).StartsWith(TEXT("At 23:10 ")),TEXT("the record's clock never reads as Story's minute"));
 }
 // NIGHT31-PLACES review («felt, not fired»): each rule by itself.
 {
  // 1. A light starts only in front of the player, and is seen in a cone that takes in the shack from the highway (some 19 degrees
  //    off the cab's nose at 420 m); it was 6 degrees.
  Check(PlaceInView(19,420,1300)&&PlaceInView(PlaceStartDeg,1300,1300)&&!PlaceInView(40,420,1300)&&!PlaceInView(10,1500,1300)&&!PlaceInView(10,-1,1300)
   &&PlaceSeenDeg>19&&PlaceSeenDeg<=25&&PlaceSeenDeg<PlaceStartDeg,TEXT("review 1: a light starts only with its place within 35 degrees of the player's view, and counts as seen within 22"));
  // 2. Heard within half the reach: the bay door's rattle at 280 m reach is heard to 140 m, not 308; the cairn's stones to 400 m.
  const FHear* Bay=HearOf(TEXT("bay_door"));const FHear* Mill=HearOf(TEXT("windmill_knock"));
  Check(Bay&&Mill&&PlaceHeardAt(140,Bay->MaxM)&&!PlaceHeardAt(141,Bay->MaxM)&&!PlaceHeardAt(308,Bay->MaxM)&&PlaceHeardAt(160,Mill->MaxM)&&!PlaceHeardAt(200,Mill->MaxM)&&!PlaceHeardAt(-1,Mill->MaxM)&&CairnHeardM<=400,
   TEXT("review 2: a sound counts as heard within half its reach, the cairn's stones within 400 m"));
  // 3. No Story night but F10's.
  Check(!PlaceMayRun(false,true)&&PlaceMayRun(true,true)&&PlaceMayRun(false,false),TEXT("review 3: none of them on a Story night, except from F10"));
  // 4. Every record that says there was no wind is gated on a still night, below 5 mph.
  bool bStill=PlaceStill(4.9)&&!PlaceStill(5.)&&!PlaceStill(11.9);
  for(const FPlaceDef& D:PlaceDefs())for(int32 Out=0;Out<=1;++Out)if(PlaceRecord(D.Id,TEXT("22:10"),Out,TEXT("the place")).Contains(TEXT(" wind.")))bStill&=NeedsStill(D.Id);
  Check(bStill,TEXT("review 4: the windmill and the swing (whose records say no wind) only below 5 mph"));
  // 5 and 6. The marks are talked of from 19:30 to 01:00 on any evening after, until said or found, and are still there to be gone to.
  const double Made=4*86400.+2.5*3600.; // night 5, 02:30
  const int32 Nt=FS47FreeformSession::NightOf(Made);
  const bool bTalk=!PlaceTalkNow(21,Nt,Nt)&&PlaceTalkNow(19.5,Nt,Nt+1)&&PlaceTalkNow(24.9,Nt,Nt+1)&&!PlaceTalkNow(25.1,Nt,Nt+1)&&!PlaceTalkNow(19.4,Nt,Nt+1)&&PlaceTalkNow(22,Nt,Nt+2);
  // The last talk of the first evening after (00:59 on the night after), and of the second for the birds, comes before the mark is gone.
  const double FirstEve=(Nt)*86400.+24.98*3600.,SecondEve=FirstEve+86400.;
  Check(bTalk&&MarkUntil(true,Made)>FirstEve&&MarkUntil(false,Made)>SecondEve&&MarkUntil(false,Made)-Made<4*86400.,
   TEXT("review 5, 6: the cairn and the birds are talked of on an evening after (19:30 to 01:00, again until said), and stand until then, within a cap of three days"));
  // 7. Told of far off: a word for each heard one but the bay door, and each mark's.
  Check(!PlaceWord(TEXT("windmill_knock"),FString(),false).IsEmpty()&&!PlaceWord(TEXT("mine_knock"),FString(),true).IsEmpty()&&!PlaceWord(TEXT("school_swing"),FString(),false).IsEmpty()&&PlaceWord(TEXT("bay_door"),FString(),true).IsEmpty(),
   TEXT("review 7: the windmill, the mine and the swings are told of on the CB or the walkie to one too far off to hear them"));
  // 8. Over at half past five, and when the day turns: nothing burns on after a sleep.
  const double Start=4*86400.+23*3600.;
  Check(!PlaceOver(Start,Start+60)&&!PlaceOver(Start,4*86400.+29.4*3600.)&&PlaceOver(Start,4*86400.+29.6*3600.)&&PlaceOver(Start,5*86400.+9*3600.)&&PlaceOver(Start,5*86400.+13*3600.)&&!PlaceOver(4*86400.+20*3600.,4*86400.+23*3600.),
   TEXT("review 8: what holds the ground ends at 05:30 or when the day turns, never left lit into the morning"));
 }
 // The strobe rule: the lamps waver, at most three swells a second and never out; the calm-flash setting holds them steady.
 {
  bool bSoft=true;
  for(const TCHAR* Which:{TEXT("cemetery_lamp"),TEXT("shack_lamp"),TEXT("road_flare")})
  {
   int32 Peaks=0;float Least=2.f;double Prev=Flicker(Which,0,false),Here=Flicker(Which,1./240.,false);
   for(int32 I=2;I<240*20;++I)
   {
    const double Next=Flicker(Which,I/240.,false);
    if(Here>Prev&&Here>=Next)++Peaks;
    Least=FMath::Min(Least,float(Here));Prev=Here;Here=Next;
   }
   bSoft&=Peaks<=3*20&&Least>=.5f&&Flicker(Which,3.3,true)==1.f;
  }
  Check(bSoft,TEXT("the lamp, the window and the flare waver at most three times a second, never go dark, and are steady with calm flashes"));
 }
 // Consequences: the marks are saved where they are, found or not, talked of or not; an older save has none.
 {
  FS47Happenings A(nullptr);A.Begin(nullptr);
  FSite Ring;Ring.Kind=TEXT("cairnring");Ring.Place=TEXT("cairn");Ring.Title=TEXT("the cairn on the rise");Ring.Night=6;Ring.Serial=4;Ring.X=-320;Ring.Z=430;Ring.Until=6*86400.+20*3600.;Ring.bHeard=true;A.Sites.Add(Ring);
  FSite Birds;Birds.Kind=TEXT("deadbirds");Birds.Place=TEXT("trough");Birds.Title=TEXT("the salt lick and trough");Birds.Night=10;Birds.Serial=5;Birds.X=240;Birds.Z=1740;Birds.Until=10*86400.+44*3600.;Birds.bKnown=true;A.Sites.Add(Birds);
  FS47Happenings B(nullptr);B.Begin(S47J::Read(S47J::Write(A.Save())));
  const FSite* R=B.SiteOf(TEXT("cairnring"));const FSite* Bd=B.SiteOf(TEXT("deadbirds"));
  Check(R&&R->bHeard&&!R->bFound&&R->X==-320.&&Bd&&Bd->bKnown&&Bd->Night==10&&B.HasSite(TEXT("deadbirds")),TEXT("the cairn's ring and the birds survive a Continue, with what the player heard and was told"));
  const TArray<FStir> Left={{-320,430,6}};
  Check(Stirred(Left,-300,450,7)&&FCString::Strcmp(S47Hap::SiteKindOf(TEXT("cairn_stones")),TEXT("cairnring"))==0&&FCString::Strcmp(S47Hap::SiteKindOf(TEXT("trough_birds")),TEXT("deadbirds"))==0, TEXT("what was out there stirs the needle near its place for two nights (FS47Places::Use)"));
  A.Stop();B.Stop();
 }
 return OK;
}

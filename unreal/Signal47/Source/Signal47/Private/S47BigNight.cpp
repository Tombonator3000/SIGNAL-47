#include "S47Happenings.h"
#include "S47Audio.h"
#include "S47CB.h"
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
#include "S47SkyVisitors.h"
#include "S47Three.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
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
 bool Concluding(const FString& Text);
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
 // BIGNIGHT-SEEN-20261109 (Tom, d37: «mange lysende disker stablet over hverandre», and he nearly missed it): its height waits for
 // the player's eyes. From the moment the power starts to go, a glow stands over the place's horizon, seen from anywhere; the shape
 // itself comes when the player has looked that way, with a clear line, for a few seconds, or at the latest HoldMost later.
 constexpr double HoldAt=Climax-.6,GazeNeed=2.5,HoldMost=40,GazeDeg=24;
 struct FHold{double Held=0,Gaze=0,Since=1e9;bool bReleased=false;};
 // RawAge: the event's own age; the age the night is at comes back (held at HoldAt while it waits).
 double StepHold(FHold& H,double RawAge,double Dt,bool bLooking)
 {
  if(bLooking){H.Gaze+=Dt;H.Since=0;}else{H.Gaze=FMath::Max(0.,H.Gaze-Dt*.35);H.Since+=Dt;}
  double Age=RawAge-H.Held;
  if(!H.bReleased&&Age>HoldAt)
  {
   if((H.Gaze>=GazeNeed&&H.Since<1.5)||H.Held>=HoldMost)H.bReleased=true;
   else{H.Held+=Age-HoldAt;Age=HoldAt;}
  }
  return Age;
 }
 // Its words while it goes on and just after (Dale on the walkie, a stranger on the CB, a caller on 1420), one at a time. {azwords}:
 // the array's bearing digit by digit; {place}: the place as spoken; {way}: which way it is from SARO; {miles}: how far; {from}: where
 // the stranger is. Concrete about where, never about what. Dale's are text only until they are recorded (templates in the manifest).
 const TCHAR* const DaleSee=TEXT("Are you seeing this? Bearing {azwords} from the station, low on the horizon, out past {place}. Every dish on remote just swung round to it by itself. Look that way.");
 const TCHAR* const DaleNudge=TEXT("Get outside and look {way} of the station, over {place}. The sky there is lit like a ballpark. Go and look, before it stops.");
 const TCHAR* const DaleArray=TEXT("Herrgott. The printer ran by itself: all channels over range, nine seconds of it, every one on {azwords}. And the array is still looking there. I did not tell it to.");
 const TCHAR* const DaleGo=TEXT("Out past {place}, {miles} from the station. Put it on your map. Not tonight: by daylight, with the walkie, and you look at the ground before you walk on it.");
 const TCHAR* const DaleWard=TEXT("And write it in the night log while you still have it: the hour, the bearing, what you saw with your own eyes. Ward wants a case carried through, with times in it. Not stories.");
 // On the CB from somebody else out on the road, as the shape stands: first line, second line, for each shape.
 const TCHAR* const CbLive[3][2]={
  {TEXT("Breaker one-nine, anybody else seeing this? There's a light standing straight up out past {place}. I'm {from} and I can read my map by it."),TEXT("It's just standing there, like a post. And my radio's singing one note at me.")},
  {TEXT("Breaker one-nine! Something just came down out past {place}. I'm {from} and the whole sky went white."),TEXT("Did anybody hear that? Did anybody else hear that?")},
  {TEXT("Breaker one-nine, anybody seeing this? Lights going up out past {place}, slow, a whole string of them. I'm {from}."),TEXT("They're still going up. Lord. They're still going up.")}};
 const TCHAR* const CbFrom[2]={TEXT("parked at the pumps in Caliche"),TEXT("pulled over on the highway up by the motel")};
 // On 1420 while it is up, in the program's own turn (Earl's Night Line is on the air from midnight to four).
 const TCHAR* const RadioLive[3]={
  TEXT("Earl, are you seeing this? I'm at my kitchen window and the whole sky out past {place} is lit up. It's standing there like a searchlight."),
  TEXT("Earl, something just came down out past {place}. The windows shook. Are you seeing this?"),
  TEXT("Earl, there are lights going up out past {place}, right now, while I'm talking to you. Slow. All together.")};
 // MSG-ONE-20261109's danger lane (US47UI::Alert, before any caption): the boom as it starts, and the glow, which way from the player.
 const TCHAR* const AlertBoom=TEXT("A low boom rolls in over the land. Every light you have flickers.");
 const TCHAR* const AlertGlow=TEXT("The sky {way} of you is lit, low, out past {place}.");
 const TCHAR* const RadioHost=TEXT("I see it from the studio window too, ma'am. Everybody stay inside and keep your lights on. I'm not going anywhere.");
 // The bearing as it is said on the air: 175 is "one-seven-five".
 FString AzWords(double Az)
 {
  static const TCHAR* const D[]={TEXT("zero"),TEXT("one"),TEXT("two"),TEXT("three"),TEXT("four"),TEXT("five"),TEXT("six"),TEXT("seven"),TEXT("eight"),TEXT("nine")};
  const FString N=FString::Printf(TEXT("%03.0f"),Az);FString Out;
  for(const TCHAR C:N)if(FChar::IsDigit(C)){if(!Out.IsEmpty())Out+=TEXT("-");Out+=D[C-'0'];}
  return Out;
 }
 // How far, as a man in 1986 New Mexico says it.
 FString Miles(double Metres)
 {
  const double Mi=Metres/1609.34;
  if(Mi<.75)return TEXT("under a mile");
  if(Mi<1.25)return TEXT("about a mile");
  if(Mi<1.75)return TEXT("a mile and a half");
  static const TCHAR* const N[]={TEXT("two"),TEXT("three"),TEXT("four"),TEXT("five"),TEXT("six")};
  const int32 K=FMath::RoundToInt(Mi);
  return K<=6?FString::Printf(TEXT("about %s miles"),N[FMath::Clamp(K,2,6)-2]):FString(TEXT("a good way"));
 }
 FString Fill(const TCHAR* Text,const FString& Place,double Az,const FString& Way=FString(),const FString& Far=FString(),const FString& From=FString())
 {
  return FString(Text).Replace(TEXT("{azwords}"),*AzWords(Az)).Replace(TEXT("{place}"),*Place).Replace(TEXT("{way}"),*Way).Replace(TEXT("{miles}"),*Far).Replace(TEXT("{from}"),*From);
 }
}

// ---------------------------------------------------------------- what is drawn (BIGNIGHT-SEEN-20261109)
// Not meshes: soft additive glows (M_WebGlow, the export's own glow sprite), many overlapping so that nothing has an edge: a shaft is
// a row of long soft ellipses along its axis turned to the eye about that axis, a glow over the horizon and one up in the sky are
// big faint ones turned to the eye, and real lights light the ground under it. Laid out here, pure (three's metres from its foot,
// y up), so that the self test can count the parts and measure their shapes.
namespace S47BigLook
{
 TAutoConsoleVariable<float> CvGlow(TEXT("s47.bignight.Glow"),1.f,TEXT("The big night's glow: brightness of every soft part, times this. 0 hides them (the lights stay)."));
 TAutoConsoleVariable<float> CvLamp(TEXT("s47.bignight.Lamp"),1.f,TEXT("The big night's glow: strength of the lights on the ground under it, times this."));
 enum ERole:uint8{Horizon=1,Sky,Shaft,Foot,Point,Trail,Head,Flash};
 struct FSprite{FVector At=FVector::ZeroVector,Axis=FVector::ZeroVector;double W=1,H=1,Bright=0;FLinearColor Colour=FLinearColor::White;int32 Id=0;uint8 Role=0;};
 struct FLamps{double FootK=0,UpK=0;FVector UpAt=FVector::ZeroVector;};
 constexpr int32 MostSprites=40,RisePoints=9;
 // Each segment 2*Hh long, centres Step*Hh apart: three or more over every point of the shaft, so there is no gap and no band.
 constexpr double Step=.6;
 // From Bottom to Top along up, N segments W wide; brighter low (Low) or high (High), each with its own flicker number.
 void AddShaft(TArray<FSprite>& Out,int32 N,double Bottom,double Top,double W,double Bright,double Low,double High,const FLinearColor& C,int32 Id0)
 {
  const double Len=Top-Bottom;
  if(Len<1||Bright<=1e-4||N<1)return;
  const double Hh=Len/(Step*(N-1)+1.5);
  for(int32 I=0;I<N;++I)
  {
   const double Y=Bottom+.75*Hh+I*Step*Hh;
   const double U=N>1?I/double(N-1):.5;
   FSprite S;S.Role=Shaft;S.At=FVector(0,Y,0);S.Axis=FVector(0,1,0);S.W=W;S.H=2*Hh;S.Bright=Bright*FMath::Lerp(Low,High,U);S.Colour=C;S.Id=Id0+I;
   Out.Add(S);
  }
 }
 FSprite Face(uint8 Role,const FVector& At,double W,double H,double Bright,const FLinearColor& C,int32 Id)
 {
  FSprite S;S.Role=Role;S.At=At;S.W=W;S.H=H;S.Bright=Bright;S.Colour=C;S.Id=Id;return S;
 }
 // Var: the shape. T: 0 to 1 over its nine seconds (below 0 before, over 1 after). Pre: 0 to 1, the glow over the horizon before
 // it. Time: seconds, for what moves. Away: level, from the eye toward the place (the fall comes in from behind it).
 void Layout(int32 Var,double T,double Pre,double Time,const FVector& Away,TArray<FSprite>& Out,FLamps& L)
 {
  using S47Big::Smooth;
  Out.Reset();L=FLamps();
  const bool bOn=T>=0&&T<1;
  const double After=T>=1?FMath::Exp(-(T-1)*3.):0.;
  const double Breathe=.86+.14*FMath::Sin(Time*1.3);
  const FLinearColor Pale(.80f,.87f,1.f),White(1.f,1.f,1.f),Warm(1.f,.93f,.80f),Green(.82f,1.f,.90f);
  const FLinearColor Tint=Var==2?FMath::Lerp(Warm,Green,float(.5+.5*FMath::Sin(Time*.45))):FMath::Lerp(Pale,White,float(.5+.5*FMath::Sin(Time*.6)));
  double Up=0;
  if(Var==0)
  {
   // A shaft stands up off the land in a second, holds, and goes back down into it: its top comes down.
   const double Top=900.*Smooth(0,.14,T)*(1-Smooth(.84,1,T));
   if(bOn&&Top>2)
   {
    AddShaft(Out,10,0,Top,7,1.5,1.,.55,White,100);
    AddShaft(Out,8,0,Top,24,.45,1.,.45,Tint,120);
    AddShaft(Out,6,0,Top*1.04,78,.11,1.,.35,Pale,140);
    Out.Add(Face(Foot,FVector(0,9,0),120,32,1.1,White,160));
   }
   Up=bOn?1:0;
   L.FootK=.12*Pre+Up;L.UpAt=FVector(0,FMath::Max(10.,Top*.45),0);L.UpK=.5*Up;
  }
  else if(Var==1)
  {
   // From high and behind the place, at a slant, faster as it comes, a long soft tail behind it; then the ground lit where it goes out.
   const double D=FMath::Clamp(T/.42,0.,1.);
   const FVector From=Away*1500.+FVector(0,1150,0),Ground(0,6,0);
   const FVector HeadAt=Ground+(From-Ground)*(1-D*D);
   const FVector Back=(From-Ground).GetSafeNormal();
   const bool bAir=bOn&&D<1;
   if(bAir)
   {
    Out.Add(Face(S47BigLook::Head,HeadAt,30,30,2.2,White,200));
    Out.Add(Face(S47BigLook::Head,HeadAt,130,130,.22,Tint,201));
    const double Tail=70+260*D;
    for(int32 I=0;I<4;++I)
    {
     FSprite S;S.Role=S47BigLook::Trail;S.Axis=Back;S.At=HeadAt+Back*(Tail*(.45+I*.55));S.W=16-3*I;S.H=Tail*1.3;S.Bright=.7*FMath::Pow(.6,I);S.Colour=I==0?White:Tint;S.Id=210+I;
     Out.Add(S);
    }
   }
   const double Since=T>=.42?(T-.42)*(S47Big::ClimaxEnd-S47Big::Climax):-1;
   const double Flash=Since>=0?FMath::Exp(-Since*.55):0.;
   if(Flash>.01)
   {
    Out.Add(Face(S47BigLook::Flash,FVector(0,22,0),280,95,2.*Flash,White,220));
    Out.Add(Face(Foot,FVector(0,6,0),140,26,1.2*Flash,Tint,221));
   }
   Up=FMath::Max(bAir?.6:0.,Flash);
   L.FootK=.12*Pre+1.3*Flash;L.UpAt=bAir?HeadAt:FVector(0,30,0);L.UpK=bAir?1.:0.;
  }
  else
  {
   // A string of small lights, a loose handful, rising slowly together out of a glow on the ground, with a soft column of light left
   // standing under them; and out all at once, high up. No two of them one over another: each its own place across.
   const double Rise=Smooth(0,.92,T);
   const bool bLit=T>=0&&T<.92;
   const double Yc=8+780.*Rise;
   if(bLit)
   {
    for(int32 I=0;I<RisePoints;++I)
    {
     const double A=I*2.39996+.4,R=6+(I%3)*7+(I%2)*3.5;
     const double Dy=double((I*37)%9-4)*6.+FMath::Sin(Time*.6+I*1.7)*2.;
     const FVector P(FMath::Cos(A)*R+FMath::Sin(Time*.31+I)*.8,Yc+Dy,FMath::Sin(A)*R+FMath::Cos(Time*.27+I)*.8);
     const double Size=6.+(I%4)*1.5;
     Out.Add(Face(Point,P,Size,Size,1.2,I%3?Warm:White,300+I));
    }
    // The column under them, brighter near them; and the ground they came off, glowing until they are well up.
    AddShaft(Out,8,0,FMath::Max(12.,Yc-10),30,.26,.45,1.,Tint,320);
    AddShaft(Out,6,0,FMath::Max(12.,Yc-10),9,.5,.35,1.,White,330);
    Out.Add(Face(Foot,FVector(0,7,0),100,26,.25+.85*(1-Rise),Warm,340));
   }
   Up=bLit?1:0;
   L.FootK=.12*Pre+(bLit?.35+.65*(1-Rise):0.);L.UpAt=FVector(0,Yc,0);L.UpK=bLit?1.:0.;
  }
  // Over the place's horizon, seen from anywhere: before it (Pre), brighter while it stands, fading after. And up on the sky.
  const double Glow=Pre*Breathe+Up+After;
  if(Glow>.005)
  {
   Out.Add(Face(Horizon,FVector(0,150,0),1500,560,.055*Pre*Breathe+.17*Up+.17*After,Tint,10));
   Out.Add(Face(Sky,FVector(0,760,0),2600,1400,.03*Pre*Breathe+.075*Up+.075*After,Pale,11));
  }
 }
 // Where on the land it stands: the horizon's glow is over the foot, so its bearing from SARO is the place's.
 FVector HorizonOffset(int32 Var){TArray<FSprite> S;FLamps L;Layout(Var,.5,1,0,FVector(0,0,-1),S,L);for(const FSprite& P:S)if(P.Role==Horizon)return P.At;return FVector(1e9);}
}

// The big night's own state while it is on and just after, not saved: the wait for the eyes, the words to come, what is drawn.
struct FS47Happenings::FBigNow
{
 S47Big::FHold Hold;double LastAge=0;
 bool bSee=false,bNudge=false,bCb=false,bRadio=false,bAfter=false,bUp=false;
 struct FSay{int32 Kind=0;TArray<FString> Lines;FString Voice;double At=0,Until=0;};
 TArray<FSay> Says;double FreeAt=0;
 TWeakObjectPtr<AS47Web> Web;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Planes;TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> Mids;
 TWeakObjectPtr<UPointLightComponent> FootLight,UpLight;
 bool bShown=false;
};

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
 if(BigNow){BigNow->Says.Reset();BigNow->Hold=S47Big::FHold();BigNow->LastAge=0;}
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
 // BIGNIGHT-SEEN-20261109: what stands out there is soft light drawn where the player is (DrawBig), not a prop built per area; its
 // wait for the eyes and its words start afresh.
 {FBigNow& B=BigState();B.Hold=S47Big::FHold();B.LastAge=0;B.bSee=B.bNudge=B.bCb=B.bRadio=B.bAfter=B.bUp=false;B.Says.Reset();B.FreeAt=0;}
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
 if(IsValid(M->Web.Get())&&!IsGarbageCollecting())
 {
  HideBigLook();
  // (props from a build before BIGNIGHT-SEEN-20261109 are never made now; put away all the same)
  for(const TCHAR* K:{TEXT("bigglow_column"),TEXT("bigglow_fall"),TEXT("bigglow_rise"),TEXT("bigflash")})ShowBig(K,FVector::ZeroVector,0,FVector(1),0,false);
 }
}

// ---------------------------------------------------------------- the night itself
void FS47Happenings::StepBig(double Dt)
{
 using namespace S47Big;
 auto* F=M->Freeform.Get();
 US47Audio* A=M->Audio.Get();
 FBigNow& B=BigState();
 const FString Area=M->Web->Area();
 const bool bSaro=Area==TEXT("saro"),bDrive=Driving(),bOut=Outdoors()&&!bDrive;
 FVector Ground;const bool bHere=WorldOf(BigX,BigZ,Ground);
 // BIGNIGHT-SEEN-20261109: its height waits for the player's eyes (StepHold). Looking: toward the sky over the place, within a
 // narrow cone and with nothing solid between (SeenLit, stricter than the old 48 degrees), from the time the glow is up over it.
 const FVector Over=Ground+FVector(0,160,0);
 const bool bLooking=bHere&&!B.Hold.bReleased&&B.LastAge>=Flick&&SeenLit(Over,16000,GazeDeg,LitOwn,nullptr);
 const bool bWasHeld=!B.Hold.bReleased;
 const double Age=StepHold(B.Hold,Now.Age,Dt,bLooking),Was=B.LastAge;
 B.LastAge=Age;
 Now.Len=S47Big::Len+B.Hold.Held;
 if(bWasHeld&&B.Hold.bReleased)UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: its height comes, %s after %.1f s of waiting (looked at it %.1f s)"),B.Hold.Held>=HoldMost?TEXT("unlooked for"):TEXT("looked for"),B.Hold.Held,B.Hold.Gaze);
 const auto Crossed=[&](double T){return Was<T&&Age>=T;};
 const FString Place=BigSpoken();
 // The gear feels it from the start: a source at the player, strong enough for the lamp, the walkie, the truck and the watch (and the
 // car radio drifting, S47GearFlicker); stronger than before, since Tom nearly missed it.
 if(M->Omen&&Age<Back)
 {
  FS47OmenSource S;S.Id=TEXT("night:big_night");S.At=Eye()+(bHere?S47Big::Level(Ground-Eye())*25.:FVector(25,0,0));S.Severity=Age<Climax?.55:.75;S.Reach=320;S.bComing=Age<Flick;
  M->Omen->Post(S,1.);
 }
 // The first second: a low boom over the whole land, the truck's lamps and the hand lamp blink (the night has gone quiet already).
 if(Crossed(.4))
 {
  if(A)A->Boom(.32f);
  if(M->UI)M->UI->Alert(AlertBoom,5);
  if(bDrive){FS47TroubleOpts O;O.Count=3.;O.Per=.35;M->World->Trouble.Start(ES47Trouble::Blink,O);}
  if(M->Night&&M->Night->Flashlight&&M->Night->Flashlight->bOn)M->Night->Flashlight->Flicker(1.2,TEXT("big_night"));
 }
 if(Crossed(Tone)&&A)
 {
  // Two low tones close together, beating, with one above: under everything, until the light out there goes.
  FS47Sound O;O.Gain=.62f;O.bLoop=true;A->LoopSfx(TEXT("hapBigTone"),Synth(TEXT("lowtone")),O);
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
 // Out on foot or in the truck, the lamps go on blinking while it builds (and while it waits for the eyes).
 if(!bSaro&&bOut&&Age>=Flick&&Age<Climax&&M->T>=BigFlick&&M->Night&&M->Night->Flashlight&&M->Night->Flashlight->bOn){BigFlick=M->T+FMath::FRandRange(.6f,1.4f);M->Night->Flashlight->Flicker(.8,TEXT("big_night"));}
 if(bDrive&&Age>=Turn&&Age<Climax&&M->T>=BigFlick){BigFlick=M->T+FMath::FRandRange(3.f,6.f);FS47TroubleOpts O;O.Count=2.;O.Per=.3;M->World->Trouble.Start(ES47Trouble::Blink,O);}
 if(Crossed(Flick+1.5)&&bHere&&M->UI)M->UI->Alert(Fill(AlertGlow,Place,BigAz,DirectionFrom(Eye(),Ground)),6);
 if(Crossed(Turn))BigTurnArray(false);
 if(bBigMotors&&Age>Turn+3&&M->Web&&!M->Web->DishesMoving()){if(A)A->StopMotors();bBigMotors=false;}
 // Dale, as the array turns: where to look. Then, while it waits for the eyes and nobody looks, once more.
 if(Crossed(Turn+2.5)&&!B.bSee){B.bSee=true;QueueBig(0,{Dale(Fill(DaleSee,Place,BigAz))},FString(),0,25);}
 if(!B.Hold.bReleased&&!B.bNudge&&B.Hold.Held>=14&&!bLooking){B.bNudge=true;QueueBig(0,{Dale(Fill(DaleNudge,Place,BigAz,S47Hap::Compass(BigAz)))},FString(),0,20);}
 if(Crossed(Climax))
 {
  B.bUp=true;
  if(bDrive){M->World->Trouble.Dim=.04;bBigDimmed=true;}
  if(M->Night&&M->Night->Flashlight&&M->Night->Flashlight->bOn)M->Night->Flashlight->FlickerOut(ClimaxEnd-Climax);
  if(A)A->Boom(.22f);
  // A stranger on the CB who sees it from somewhere else, and a caller on 1420 if the truck's radio is on it.
  if(!B.bCb&&CanHearCb())
  {
   B.bCb=true;double X=0,Z=0;const bool bTown=PlayerRoad(X,Z)&&Z>1700;
   const int32 V=FMath::Clamp(BigVar,0,2);
   QueueBig(1,{Fill(CbLive[V][0],Place,BigAz,FString(),FString(),CbFrom[bTown?1:0]),FString(CbLive[V][1])},BigVar==2?TEXT("old"):TEXT("man"),2,20);
  }
  if(!B.bRadio&&M->CarRadio&&M->CarRadio->Live()&&M->CarRadio->Station==TEXT("caliche"))
  {
   B.bRadio=true;QueueBig(2,{Fill(RadioLive[FMath::Clamp(BigVar,0,2)],Place,BigAz),FString(RadioHost)},TEXT("woman"),14,40);
  }
 }
 // Out there, in that direction: the shape of tonight, in soft light, and the glow over its horizon before it (DrawBig).
 DrawBig(Age,Ground,bHere);
 if(bHere&&Age>=Climax&&Age<ClimaxEnd&&!BigFollow.Contains(TEXT("seen")))
 {
  // Seen: looked at with a clear line, as for the wait, at where the shape is now.
  TArray<S47BigLook::FSprite> Spr;S47BigLook::FLamps L;
  S47BigLook::Layout(FMath::Clamp(BigVar,0,2),(Age-Climax)/(ClimaxEnd-Climax),0,M->T,S47Big::Level(Ground-Eye()),Spr,L);
  const FVector Look=BigVar==0?Ground+FVector(0,250,0):Ground+L.UpAt;
  if(SeenLit(Look,16000,30,LitOwn,nullptr)||SeenLit(Over,16000,GazeDeg,LitOwn,nullptr))
  {
   BigFollow.Add(TEXT("seen"));
   Noticed(TEXT("big_night"),AwayFrom(Ground),TEXT("sight"),Now.Serial);
   UE_LOG(LogS47BigNight,Display,TEXT("S47: big night %s seen, %.0f m off"),BigVariantName(BigVar),FVector::Dist(Eye(),Look));
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
  HideBigLook();
  const FString Clock=S47Big::Clock(BigClock);
  if(bSaro){BigRecordArray(true);B.FreeAt=FMath::Max(B.FreeAt,Paced+9);}
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
  // Just after: Dale says what the array kept, where to go and what Ward will want, one line at a time.
  if(!B.bAfter)
  {
   B.bAfter=true;
   double SaroX=0,SaroZ=0;S47OpenLand::RoadOf(TEXT("saro"),9,0,SaroX,SaroZ);
   const FString Far=Miles(FVector2D::Distance(FVector2D(SaroX,SaroZ),FVector2D(BigX,BigZ)));
   QueueBig(0,{Dale(Fill(DaleArray,Place,BigAz))},FString(),6,60);
   QueueBig(0,{Dale(Fill(DaleGo,Place,BigAz,FString(),Far))},FString(),7,80);
   QueueBig(0,{Dale(Fill(DaleWard,Place,BigAz))},FString(),8,100);
   if(FSite* S=SiteOf(TEXT("bigmark")))S->bKnown=true;
  }
  M->RequestAutosave();
 }
 if(Age>=S47Big::Len)
 {
  EndBigEffects();
  UE_LOG(LogS47BigNight,Display,TEXT("S47: big night over, %s"),BigFollow.Contains(TEXT("seen"))?TEXT("seen"):TEXT("felt"));
  EndEvent(true);
 }
}

// ---------------------------------------------------------------- BIGNIGHT-SEEN-20261109: drawn, and its words
FS47Happenings::FBigNow& FS47Happenings::BigState(){if(!BigNow)BigNow=MakeShared<FBigNow>();return *BigNow;}
double FS47Happenings::BigHeldFor() const{return BigNow?BigNow->Hold.Held:0.;}
bool FS47Happenings::BigCameUp() const{return BigNow&&BigNow->bUp;}
void FS47Happenings::HideBigLook()
{
 if(!BigNow||!BigNow->bShown)return;
 BigNow->bShown=false;
 if(IsEngineExitRequested()||GExitPurge||IsGarbageCollecting())return;
 for(const TWeakObjectPtr<UStaticMeshComponent>& C:BigNow->Planes)if(UStaticMeshComponent* P=C.Get())P->SetVisibility(false);
 for(UPointLightComponent* L:{BigNow->FootLight.Get(),BigNow->UpLight.Get()})if(L){L->SetIntensity(0);L->SetVisibility(false);}
}
// One set of soft parts and two lights for every area, placed each frame at the place in the area the player is in (WorldOf): never
// a copy left in another area, and nothing built per area (before 9 November one prop was built in road and another in diner).
void FS47Happenings::DrawBig(double Age,const FVector& Ground,bool bHere)
{
 using namespace S47Big;
 FBigNow& B=BigState();
 AS47Web* W=M&&M->Web?M->Web.Get():nullptr;
 if(!bHere||!IsValid(W)||!W->IsLoaded()||IsEngineExitRequested()||IsGarbageCollecting()||Age<Flick||Age>=ClimaxEnd+8){HideBigLook();return;}
 if(B.Web.Get()!=W){B.Planes.Reset();B.Mids.Reset();B.FootLight.Reset();B.UpLight.Reset();B.Web=W;B.bShown=false;}
 const double T=(Age-Climax)/(ClimaxEnd-Climax);
 const double Pre=Smooth(Flick,HoldAt,Age)*(1-Smooth(Climax,ClimaxEnd,Age));
 const FVector EyeAt=Eye();
 const int32 Var=FMath::Clamp(BigVar,0,2);
 TArray<S47BigLook::FSprite> Spr;S47BigLook::FLamps L;
 S47BigLook::Layout(Var,T,Pre,M->T,S47Big::Level(Ground-EyeAt),Spr,L);
 const int32 Want=FMath::Min(Spr.Num(),S47BigLook::MostSprites);
 if(B.Planes.Num()<Want)
 {
  auto* Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_WebGlow.M_WebGlow"));
  auto* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
  if(!Mat||!Plane){UE_LOG(LogS47BigNight,Warning,TEXT("S47: big night: no glow material or plane to draw it with"));HideBigLook();return;}
  while(B.Planes.Num()<Want)
  {
   auto* C=NewObject<UStaticMeshComponent>(W,MakeUniqueObjectName(W,UStaticMeshComponent::StaticClass(),FName(TEXT("S47BigGlow"))));
   C->SetupAttachment(W->GetRootComponent());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Plane);
   auto* Mid=UMaterialInstanceDynamic::Create(Mat,W);Mid->SetScalarParameterValue(TEXT("Brightness"),0.f);C->SetMaterial(0,Mid);
   C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);
   C->SetCastShadow(false);C->bAffectDistanceFieldLighting=false;C->bVisibleInReflectionCaptures=false;C->bVisibleInRealTimeSkyCaptures=false;
   C->SetVisibility(false);C->RegisterComponent();
   B.Planes.Add(C);B.Mids.Add(Mid);
  }
  UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: %d soft parts to draw it with, in %s"),B.Planes.Num(),*W->Area());
 }
 for(TWeakObjectPtr<UPointLightComponent>* Slot:{&B.FootLight,&B.UpLight})if(!Slot->IsValid())
 {
  auto* Lc=NewObject<UPointLightComponent>(W,MakeUniqueObjectName(W,UPointLightComponent::StaticClass(),FName(TEXT("S47BigGlowLamp"))));
  Lc->SetupAttachment(W->GetRootComponent());Lc->SetMobility(EComponentMobility::Movable);Lc->SetIntensityUnits(ELightUnits::Lumens);Lc->SetIntensity(0);
  Lc->SetAttenuationRadius(Slot==&B.FootLight?250000.f:150000.f);Lc->SetCastShadows(false);Lc->SetVolumetricScatteringIntensity(Slot==&B.FootLight?.35f:.2f);
  Lc->SetVisibility(false);Lc->RegisterComponent();*Slot=Lc;
 }
 B.bShown=true;
 const float K=FMath::Max(0.f,S47BigLook::CvGlow.GetValueOnGameThread());
 const FVector EyeUE=S47Three::ToUE(EyeAt);
 for(int32 I=0;I<B.Planes.Num();++I)
 {
  UStaticMeshComponent* C=B.Planes[I].Get();UMaterialInstanceDynamic* Mid=B.Mids.IsValidIndex(I)?B.Mids[I].Get():nullptr;
  if(!C||!Mid)continue;
  if(I>=Want||K<=0||Spr[I].Bright<=1e-4){C->SetVisibility(false);continue;}
  const S47BigLook::FSprite& S=Spr[I];
  const FVector At=Ground+S.At,AtUE=S47Three::ToUE(At);
  FVector ToEye=(EyeUE-AtUE).GetSafeNormal();if(ToEye.IsNearlyZero())ToEye=FVector::ForwardVector;
  FMatrix R;
  if(S.Axis.IsNearlyZero())R=FMath::Abs(ToEye.Z)>.999?FRotationMatrix::MakeFromZ(ToEye):FRotationMatrix::MakeFromZX(ToEye,FVector::UpVector);
  else{const FVector Ax=S47Three::DirToUE(S.Axis).GetSafeNormal();R=FMath::Abs(FVector::DotProduct(Ax,ToEye))>.999?FRotationMatrix::MakeFromX(Ax):FRotationMatrix::MakeFromXZ(Ax,ToEye);}
  // The engine's plane is a metre square across its local x and y, its face along z: x along the long way.
  C->SetWorldLocationAndRotation(AtUE,R.Rotator());C->SetWorldScale3D(FVector(S.H,S.W,1));
  // A little of the sky lights' twinkle and colour (FS47SkyGlint), less on the big glows, so it is never quite still.
  const double Elev=FMath::Atan2(At.Y-EyeAt.Y,FMath::Max(1.,FVector2D::Distance(FVector2D(At.X,At.Z),FVector2D(EyeAt.X,EyeAt.Z))));
  const FVector G=FMath::Lerp(FVector::OneVector,FS47SkyGlint::Gains(M->T,S.Id,Elev),S.Role==S47BigLook::Horizon||S.Role==S47BigLook::Sky?.25:.5);
  Mid->SetVectorParameterValue(TEXT("Color"),FLinearColor(float(S.Colour.R*G.X),float(S.Colour.G*G.Y),float(S.Colour.B*G.Z),1.f));
  Mid->SetScalarParameterValue(TEXT("Brightness"),float(S.Bright*K));
  C->SetVisibility(true);
 }
 // The ground under it and the land round it lit: the light's own colour, unsteady.
 const float LK=FMath::Max(0.f,S47BigLook::CvLamp.GetValueOnGameThread());
 const double Flick2=.85+.15*FMath::Sin(M->T*9.1)*FMath::Sin(M->T*3.7);
 const FLinearColor Col=Var==2?S47Three::Hex(0xfff0d2):S47Three::Hex(0xdfe8ff);
 const float FootLm=Var==1?120000000.f:Var==0?80000000.f:30000000.f;
 if(UPointLightComponent* Lc=B.FootLight.Get()){Lc->SetWorldLocation(S47Three::ToUE(Ground+FVector(0,30,0)));Lc->SetLightColor(Col);Lc->SetIntensity(float(FootLm*FMath::Clamp(L.FootK,0.,1.5)*Flick2*LK));Lc->SetVisibility(L.FootK>.001);}
 if(UPointLightComponent* Lc=B.UpLight.Get()){Lc->SetWorldLocation(S47Three::ToUE(Ground+L.UpAt));Lc->SetLightColor(Col);Lc->SetIntensity(float(20000000.f*FMath::Clamp(L.UpK,0.,1.)*Flick2*LK));Lc->SetVisibility(L.UpK>.001);}
}
void FS47Happenings::QueueBig(int32 Kind,const TArray<FString>& Lines,const FString& Voice,double In,double Within)
{
 if(Lines.IsEmpty())return;
 FBigNow& B=BigState();
 FBigNow::FSay S;S.Kind=Kind;S.Lines=Lines;S.Voice=Voice;S.At=Paced+In;S.Until=S.At+Within;B.Says.Add(S);
}
// Its words go out one at a time, in step with the night and never over a hail or a line of Dale's still being said: the walkie
// (0, Dale's), the CB (1, a stranger's, only in the truck), 1420 (2, a caller, only with the truck's radio on it). What cannot be
// heard where the player is now goes: the morning (his hail or his slip) still has the same news.
void FS47Happenings::SayBig()
{
 if(!BigNow||BigNow->Says.IsEmpty()||!M)return;
 FBigNow& B=*BigNow;
 const FBigNow::FSay S=B.Says[0];
 const auto Drop=[&](const TCHAR* Why){UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: a line let go (%s): %s"),Why,*S.Lines[0].Left(60));B.Says.RemoveAt(0);};
 if(Paced>S.Until){Drop(TEXT("too late"));return;}
 if(Paced<S.At||Paced<B.FreeAt||M->ModalOpen())return;
 FS47FieldRadio* R=M->FieldRadio.IsValid()?M->FieldRadio.Get():nullptr;
 if(R&&(R->Hailing()||R->DaleSpeaking()))return;
 double Secs=0;bool bSaid=false;
 if(S.Kind==0)
 {
  if(!R||!R->Owned()){Drop(TEXT("no walkie"));return;}
  bSaid=R->Message(S.Lines[0]);Secs=4.+S.Lines[0].Len()/15.;
 }
 else if(S.Kind==1)
 {
  if(!CanHearCb()){Drop(TEXT("not at the CB"));return;}
  if(M->CB.IsValid()&&M->CB->bBusy)return;
  bSaid=FS47FieldRadio::CbSay(M,S.Lines,S.Voice);
  for(const FString& Ln:S.Lines)Secs+=S47CB::SecsOf(Ln)+.6;
 }
 else
 {
  if(!(M->CarRadio&&M->CarRadio->Live()&&M->CarRadio->Station==TEXT("caliche")&&M->Radio&&M->Story)){Drop(TEXT("1420 not on"));return;}
  const auto* St=M->Radio->Stations.Find(TEXT("caliche"));
  if(St&&St->IsValid())
  {
   TArray<FS47RadioLine> Ls;Ls.Add(FS47RadioLine(S.Lines[0],S.Voice,TEXT("a caller")));
   if(S.Lines.Num()>1)Ls.Add(FS47RadioLine(S.Lines[1],TEXT("host")));
   bSaid=(*St)->Slot(Ls,M->Story->Clock()).IsValid();
  }
  Secs=16;
 }
 if(!bSaid){B.Says[0].At=Paced+2;return;}
 UE_LOG(LogS47BigNight,Display,TEXT("S47: big night: said on the %s: %s"),S.Kind==0?TEXT("walkie"):S.Kind==1?TEXT("CB"):TEXT("1420"),*S.Lines[0].Left(70));
 // (the shared queue, FS47MessageQueue, shows them one at a time; this only keeps them from all coming in at once)
 B.FreeAt=Paced+FMath::Max(4.,Secs*.6);
 B.Says.RemoveAt(0);
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
 // Its words from the night itself, one at a time, during it and just after (BIGNIGHT-SEEN-20261109).
 SayBig();
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
 // BIGNIGHT-SEEN-20261109 (Tom, d37): what is drawn is soft light and not a stack of discs; the glow over the horizon stands over the
 // place; its height waits for the eyes, or comes at the latest HoldMost later; and every new line passes the same filters.
 {
  using namespace S47BigLook;
  bool bSoft=true,bCount=true,bShaft=true,bApart=true,bFinite=true;int32 Most=0,PointsSeen=0;
  for(int32 V=0;V<3;++V)for(int32 K=-2;K<=12;++K)
  {
   const double T=K/10.;TArray<FSprite> S;FLamps L;Layout(V,T,K<0?1.:0.,K*.37,FVector(0,0,-1),S,L);
   Most=FMath::Max(Most,S.Num());bCount&=S.Num()<=MostSprites;
   TArray<const FSprite*> Pts;TMap<int32,TArray<const FSprite*>> Shafts;
   for(const FSprite& P:S)
   {
    bFinite&=FMath::IsFinite(P.W)&&FMath::IsFinite(P.H)&&P.W>0&&P.H>0&&!P.At.ContainsNaN()&&FMath::IsFinite(P.Bright)&&P.Bright>=0;
    bSoft&=P.Role>=Horizon&&P.Role<=S47BigLook::Flash;
    if(P.Role==Point){Pts.Add(&P);bSoft&=P.W<=12&&FMath::IsNearlyEqual(P.W,P.H);}
    if(P.Role==Shaft)Shafts.FindOrAdd(P.Id/10).Add(&P);
   }
   for(int32 I=0;I<Pts.Num();++I)for(int32 J=I+1;J<Pts.Num();++J)if(FVector2D::Distance(FVector2D(Pts[I]->At.X,Pts[I]->At.Z),FVector2D(Pts[J]->At.X,Pts[J]->At.Z))<3.)bApart=false;
   if(V==2&&T>=0&&T<.92)PointsSeen=FMath::Max(PointsSeen,Pts.Num());
   for(const auto& It:Shafts)
   {
    const TArray<const FSprite*>& Sg=It.Value;
    for(int32 I=0;I<Sg.Num();++I){bShaft&=Sg[I]->Axis.Equals(FVector(0,1,0));if(I>0){const double Gap=Sg[I]->At.Y-Sg[I-1]->At.Y;bShaft&=Gap>0&&Gap<=.65*Sg[I]->H*.5+1e-6;}}
   }
  }
  Check(bSoft&&bFinite&&bCount,FString::Printf(TEXT("BIGNIGHT-SEEN: drawn only in soft glows, at most %d parts at once (%d at the most), none of them a solid shape"),MostSprites,Most));
  Check(bApart&&PointsSeen==RisePoints,FString::Printf(TEXT("BIGNIGHT-SEEN: the rise is %d small round lights, none of them over another (not a stack of discs)"),PointsSeen));
  Check(bShaft,TEXT("BIGNIGHT-SEEN: every shaft is soft parts along up, each overlapping the next, with no gap or band"));
  bool bDir=true;
  for(int32 V=0;V<3;++V){const FVector O=HorizonOffset(V);bDir&=FVector2D(O.X,O.Z).Size()<.5&&O.Y>0&&O.Y<1000;}
  double SaroX=0,SaroZ=0;S47OpenLand::RoadOf(TEXT("saro"),9,0,SaroX,SaroZ);
  const FVector O=HorizonOffset(2);
  for(int32 I=0;I<24;++I)
  {
   const double A=FMath::DegreesToRadians(I*15.+3.),X=SaroX+FMath::Sin(A)*2000.,Z=SaroZ-FMath::Cos(A)*2000.;
   const double Got=S47Hap::Bearing(FS47Places::AzimuthOf(SaroX,SaroZ,X+O.X,Z+O.Z)),Want=S47Hap::Bearing(FS47Places::AzimuthOf(SaroX,SaroZ,X,Z));
   bDir&=FMath::Abs(Got-Want)<1.01||FMath::Abs(Got-Want)>358.9;
  }
  Check(bDir,TEXT("BIGNIGHT-SEEN: the glow over the horizon stands over the place itself, so its bearing from SARO is the place's (the array's) every way round"));
 }
 {
  using namespace S47Big;
  const auto Run=[](double LookFrom,double& At)
  {
   FHold H;double Raw=0,Was=0;At=-1;
   for(int32 I=0;I<2000&&At<0;++I){Raw+=.1;const double Age=StepHold(H,Raw,.1,Raw>=LookFrom&&Was>=Flick);if(Age>=Climax)At=Raw;Was=Age;}
  };
  double Never=0,Always=0,Late=0;Run(1e9,Never);Run(0,Always);Run(50,Late);
  Check(FMath::Abs(Never-(Climax+HoldMost))<1.,FString::Printf(TEXT("BIGNIGHT-SEEN: never looked at, its height comes at the latest %.0f s later (at %.1f s)"),HoldMost,Never));
  Check(Always>0&&Always<Climax+.5,FString::Printf(TEXT("BIGNIGHT-SEEN: looked at as it builds, its height comes on time (at %.1f s)"),Always));
  Check(Late>50+GazeNeed-.2&&Late<50+GazeNeed+1.5,FString::Printf(TEXT("BIGNIGHT-SEEN: looked at from 50 s on, it waits for %.1f s of looking and comes then (at %.1f s)"),GazeNeed,Late));
 }
 {
  using namespace S47Big;
  TArray<FString> All;
  for(const TCHAR* T:{DaleSee,DaleNudge,DaleArray,DaleGo,DaleWard,RadioHost,CbFrom[0],CbFrom[1],AlertBoom,AlertGlow})All.Add(T);
  for(int32 V=0;V<3;++V){All.Add(CbLive[V][0]);All.Add(CbLive[V][1]);All.Add(RadioLive[V]);}
  FString Bad;
  for(const FString& T:All)
  {
   const FString Filled=Fill(*T,TEXT("the salt lick"),175,TEXT("south"),Miles(2018),CbFrom[0]);
   if((!S47Hap::Clean(Filled)||S47Hap::Concluding(Filled)||Filled.Contains(TEXT("Reyes"))||Filled.Contains(TEXT("{")))&&Bad.Len()<200)Bad+=Filled.Left(50)+TEXT(" / ");
  }
  Check(Bad.IsEmpty(),TEXT("BIGNIGHT-SEEN: every new line passes the filters (Story's far side, names, brands, telling the answer, Reyes) ")+Bad);
  bool bSaid=AzWords(175)==TEXT("one-seven-five")&&AzWords(8)==TEXT("zero-zero-eight")&&Miles(1500)==TEXT("about a mile")&&Miles(2018)==TEXT("a mile and a half")&&Miles(3300)==TEXT("about two miles");
  for(int32 Az=0;Az<360;++Az)bSaid&=!AzWords(S47Hap::Bearing(Az)).Contains(TEXT("four-seven"));
  Check(bSaid,TEXT("BIGNIGHT-SEEN: the bearing is said digit by digit (175: one-seven-five), never as Story's number, and the distance in miles"));
 }
 return OK;
}

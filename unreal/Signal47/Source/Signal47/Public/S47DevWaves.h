#pragma once
#include "CoreMinimal.h"

class US47Main;
class AS47Walker;
class FS47DevEvents;
class SWidget;
struct FHitResult;
struct FS47DevWaveBody;
struct FS47DevWaveHud;
struct FS47DevWaveFx;

// DEV TOY, not part of the game (Tom, 13 November 2026: "lag fps mode skytespill med waves med aliens i og rundt SARO, brukes kun
// fra F10 dev"; the same day: it is to FEEL like a big shooter). A first-person wave shooter in the SARO yard, started and stopped
// only from F10 ("Wave shooter (dev toy)"). The Director never draws it, nothing of it is saved, and it does not start in a run
// nobody plays (but -S47DevWaves=auto, which puts a bot at the trigger for pictures). The rules live here as plain numbers and small
// structs so that -S47SelfTest holds them ("devWaves:" lines, from FS47DevFreeform::SelfTest). Everything in it is our own: an
// invented carbine (ArtSource/DevWaves), no other game's weapons, names, screen or sounds.
namespace S47DevWaves
{
 // Small: the small Roswell. Pale: the Pale One. Tall: the Tall One. Crab: the Pale One on all fours.
 enum class EKind : uint8 { Small, Pale, Tall, Crab, Num };
 enum class EPhase : uint8 { Off, Banner, Fight, Dead };

 inline constexpr int32 MagSize=30, StartReserve=150, MostReserve=300, CrateAmmo=120, CrateHeal=40, StartHealth=100, TouchDamage=12, WaveHeal=10, MostAtOnce=8, MostInWave=40;
 inline constexpr double ReloadSeconds=2.2, ShotGap=.1, TouchReach=1.1, TouchGap=.9, Range=120, BannerSeconds=4., CardSeconds=6., StreakGap=4., CrateReach=1.7;
 // The muzzle's light: never closer together than this, so at most three in any second (the project's flash limit), and none at
 // all with Calm flashes chosen in Settings. (The small glow at the muzzle itself is a sprite, not a light on the world.)
 inline constexpr double FlashGap=.34;
 // The Tall One reels when struck, but not again at once: fire cannot hold it in place.
 inline constexpr double StaggerSeconds=.55, StaggerRest=1.3;

 // ---- the waves ----
 inline int32 WaveSize(int32 Wave) { return FMath::Clamp(3+3*FMath::Max(1,Wave),1,MostInWave); }     // 6, 9, 12 ... 40
 inline int32 WaveAtOnce(int32 Wave) { return FMath::Clamp(2+FMath::Max(1,Wave),1,MostAtOnce); }     // 3, 4, 5 ... 8
 inline double WaveSpeed(int32 Wave) { return FMath::Min(2.+.22*(FMath::Max(1,Wave)-1),4.2); }       // m/s; the player runs at about 3
 inline double WaveGap(int32 Wave) { return FMath::Max(.45,1.6-.12*(FMath::Max(1,Wave)-1)); }       // seconds between two coming out
 inline EKind KindOf(int32 Wave,int32 Index)
 {
  if(Wave>=4&&Index%5==4)return EKind::Tall;
  if(Wave>=3&&Index%4==2)return EKind::Crab;
  if(Wave>=2&&Index%3==1)return EKind::Pale;
  return EKind::Small;
 }
 // How many rounds each takes (a shot in the head counts double).
 inline int32 HealthOf(EKind K) { return K==EKind::Tall?10:K==EKind::Pale?4:K==EKind::Crab?3:2; }
 inline double PaceOf(EKind K) { return K==EKind::Crab?1.3:K==EKind::Small?1.15:K==EKind::Tall?.85:1.; }
 inline double HeightOf(EKind K) { return K==EKind::Tall?2.5:K==EKind::Pale?2.:K==EKind::Crab?.95:1.2; }
 inline double RadiusOf(EKind K) { return K==EKind::Crab?.6:K==EKind::Small?.32:.4; }
 inline int32 ScoreOf(EKind K) { return K==EKind::Tall?600:K==EKind::Crab?250:K==EKind::Pale?200:100; }
 inline bool HeadShot(EKind K,double HitHeight) { return K!=EKind::Crab&&HitHeight>=HeightOf(K)*.8; }
 inline int32 ShotDamage(bool bHead) { return bHead?2:1; }
 // Kills in a row, none more than StreakGap after the one before and no touch in between: every three more is one more times the score, to five.
 inline int32 StreakTimes(int32 Streak) { return FMath::Clamp(1+FMath::Max(0,Streak)/3,1,5); }
 // Who comes round the side: every third straight on, the others from the left and from the right (radians off the straight line).
 inline double FlankOf(int32 Serial) { const int32 S=((Serial%3)+3)%3; return S==0?0.:S==1?.95:-.95; }

 // ---- the reload, beat by beat (fractions of ReloadSeconds): the hand does one thing at a time, and each has its sound ----
 enum class EBeat : uint8 { Lower, MagOut, MagIn, BoltBack, BoltHome, Raise, Num };
 inline double BeatAt(EBeat B)
 {
  switch(B)
  {
  case EBeat::MagOut:return .17;
  case EBeat::MagIn:return .58;
  case EBeat::BoltBack:return .75;
  case EBeat::BoltHome:return .87;
  case EBeat::Raise:return .93;
  default:return 0.;
  }
 }
 inline EBeat BeatOf(double Frac) { EBeat B=EBeat::Lower; for(int32 I=0;I<int32(EBeat::Num);++I)if(Frac>=BeatAt(EBeat(I)))B=EBeat(I); return B; }

 // ---- the gun: a magazine and what is carried besides, a pause between shots, a reload that takes its time ----
 struct FGun
 {
  int32 Mag=MagSize, Reserve=StartReserve;
  double Reload=0, Cool=0;
  bool CanFire() const { return Mag>0&&Reload<=0&&Cool<=0; }
  bool Fire() { if(!CanFire())return false; --Mag; Cool=ShotGap; return true; }
  bool StartReload() { if(Reload>0||Mag>=MagSize||Reserve<=0)return false; Reload=ReloadSeconds; return true; }
  // 0 as it starts to 1 as the magazine is full; 0 with no reload under way.
  double ReloadFrac() const { return Reload>0?FMath::Clamp(1.-Reload/ReloadSeconds,0.,1.):0.; }
  // True in the step the magazine is filled (from what is carried, as far as that goes).
  bool Step(double Dt)
  {
   Cool=FMath::Max(0.,Cool-Dt);
   if(Reload<=0)return false;
   Reload-=Dt;
   if(Reload>0)return false;
   Reload=0;
   const int32 Moved=FMath::Min(MagSize-Mag,Reserve);
   Mag+=Moved; Reserve-=Moved;
   return true;
  }
  void Supply() { Reserve=FMath::Min(MostReserve,Reserve+CrateAmmo); }
 };

 // ---- how wide the shots go, degrees: the stance (Ads 0 from the hip to 1 down the sights), the walk (Move 0 to 1), and what fire has added ----
 struct FSpread
 {
  double Bloom=0, Still=9;
  static double Base(double Ads,double Move) { return FMath::Lerp(1.05,.1,FMath::Clamp(Ads,0.,1.))+FMath::Clamp(Move,0.,1.5)*FMath::Lerp(.9,.22,FMath::Clamp(Ads,0.,1.)); }
  void Shot(double Ads) { const double A=FMath::Clamp(Ads,0.,1.); Bloom=FMath::Min(FMath::Lerp(3.2,.75,A),Bloom+FMath::Lerp(.42,.1,A)); Still=0; }
  void Step(double Dt) { Still+=Dt; if(Still>.11)Bloom=FMath::Max(0.,Bloom-Dt*7.); }
  double Deg(double Ads,double Move) const { return Base(Ads,Move)+Bloom; }
 };

 // A damped spring that rests at nothing: what a kick does to the view and to the gun in the hand.
 struct FSpring
 {
  double X=0, V=0;
  // Peak: about how far the kick throws it at this stiffness.
  void Kick(double Peak,double Hz) { V+=Peak*Hz*14.; }
  void Step(double Dt,double Hz,double Damp)
  {
   const double Om=2.*UE_DOUBLE_PI*Hz;
   for(double Left=FMath::Clamp(Dt,0.,.1);Left>1e-6;)
   {
    const double H=FMath::Min(Left,1./240.);
    Left-=H;
    V+=(-Om*Om*X-2.*Damp*Om*V)*H;
    X+=V*H;
   }
  }
 };

 inline int32 Hurt(int32 Health,int32 Damage) { return FMath::Max(0,Health-FMath::Max(0,Damage)); }
 inline bool FlashMay(double Now,double Last,bool bCalm) { return !bCalm&&Now-Last>=FlashGap; }

 // ---- one round: the waves in turn, the health, the score ----
 struct FRound
 {
  EPhase Phase=EPhase::Off;
  int32 Wave=0, ToCome=0, Out=0, Health=StartHealth, Score=0, Kills=0, Streak=0, BestStreak=0, Heads=0;
  double Wait=0, StreakLeft=0;
  bool bCrate=false; // a supply crate waits to be fetched
  void Begin() { *this=FRound(); Wave=1; ToCome=WaveSize(1); Phase=EPhase::Banner; Wait=BannerSeconds; }
  int32 Left() const { return ToCome+Out; }
  bool MaySpawn() const { return Phase==EPhase::Fight&&ToCome>0&&Out<WaveAtOnce(Wave); }
  void Spawned() { if(ToCome>0){ --ToCome; ++Out; } }
  // One that was out is gone without being shot (its body went with its world): it comes again.
  void Lost() { if(Out>0){ --Out; ++ToCome; } }
  // What the kill scored.
  int32 Killed(EKind K,bool bHead=false)
  {
   Out=FMath::Max(0,Out-1); ++Kills;
   if(bHead)++Heads;
   const int32 Got=ScoreOf(K)*FMath::Max(1,Wave)*StreakTimes(Streak)*(bHead?3:2)/2;
   Score+=Got; ++Streak; BestStreak=FMath::Max(BestStreak,Streak); StreakLeft=StreakGap;
   if(Phase==EPhase::Fight&&Left()==0)
   {
    ++Wave; ToCome=WaveSize(Wave); Phase=EPhase::Banner; Wait=BannerSeconds;
    Health=FMath::Min(StartHealth,Health+WaveHeal); bCrate=true;
   }
   return Got;
  }
  void Touched()
  {
   if(Phase!=EPhase::Fight)return;
   Health=Hurt(Health,TouchDamage); Streak=0; StreakLeft=0;
   if(Health<=0){ Phase=EPhase::Dead; Wait=CardSeconds; }
  }
  // True in the step the crate is taken: health comes back (the caller fills the gun).
  bool TakeCrate()
  {
   if(!bCrate||Phase==EPhase::Dead||Phase==EPhase::Off)return false;
   bCrate=false; Health=FMath::Min(StartHealth,Health+CrateHeal);
   return true;
  }
  // True once the "you went down" card has been up its time: the round is over, and a new one begins.
  bool Step(double Dt)
  {
   if(StreakLeft>0){ StreakLeft-=Dt; if(StreakLeft<=0){ StreakLeft=0; Streak=0; } }
   if(Phase==EPhase::Banner){ Wait-=Dt; if(Wait<=0){ Wait=0; Phase=EPhase::Fight; } }
   else if(Phase==EPhase::Dead){ Wait-=Dt; if(Wait<=0)return true; }
   return false;
  }
 };

 // How far along a shot (Eye, unit Dir; scene metres, y up) a standing body is hit, as an upright capsule from its feet; below 0: missed.
 inline double RayBody(const FVector& Eye,const FVector& Dir,const FVector& Feet,double Height,double Radius)
 {
  const FVector Low=Feet+FVector(0,Radius,0), Spine=FVector(0,FMath::Max(0.,Height-2*Radius),0), Off=Eye-Low;
  const double DirSpine=FVector::DotProduct(Dir,Spine), SpineLen2=Spine.SizeSquared(), DirOff=FVector::DotProduct(Dir,Off), SpineOff=FVector::DotProduct(Spine,Off);
  const double Den=SpineLen2-DirSpine*DirSpine;
  double Up=SpineLen2<1e-9||Den<1e-9?0.:FMath::Clamp((SpineOff-DirSpine*DirOff)/Den,0.,1.);
  double Along=FMath::Max(0.,DirSpine*Up-DirOff);
  if(SpineLen2>=1e-9){ Up=FMath::Clamp((SpineOff+DirSpine*Along)/SpineLen2,0.,1.); Along=FMath::Max(0.,DirSpine*Up-DirOff); }
  return FVector::Dist(Eye+Dir*Along,Low+Spine*Up)<=Radius?Along:-1.;
 }

 // What is given back when it stops: where the player stood and looked. (The clock is not moved at all: the night is held.)
 struct FStand
 {
  FVector Pos=FVector::ZeroVector;
  double FloorY=0, Yaw=0, Pitch=0;
  bool bCrouch=false;
  bool operator==(const FStand& O) const { return Pos==O.Pos&&FloorY==O.FloorY&&Yaw==O.Yaw&&Pitch==O.Pitch&&bCrouch==O.bCrouch; }
 };
}

// DEV TOY: the wave shooter itself. Owned by FS47DevFreeform (the F10 lines of a Freeform night), stepped from its Step.
class SIGNAL47_API FS47DevWaves
{
public:
 explicit FS47DevWaves(US47Main* InM);
 ~FS47DevWaves();
 // The F10 group "Wave shooter (dev toy)": start and stop.
 void AddEntries(FS47DevEvents& E);
 void Step(double Dt);
 void Start();
 // Over at once: the bodies away, the gun and the screen gone, the player back where they stood, every hold let go.
 void Stop(const TCHAR* Why);
 bool Running() const { return Round.Phase!=S47DevWaves::EPhase::Off; }
 // The shooter runs in this game: asked by what must wait (the session's Held, OneAtATime, the Director, the rhythm, the taking, saving).
 static bool On(const US47Main* InM);
 // -S47DevWaves=auto on the command line: a developer run that starts the shooter by itself with a bot at the trigger, for pictures.
 static bool AutoWanted();
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 S47DevWaves::FRound Round;
 S47DevWaves::FGun Gun;
 S47DevWaves::FSpread Spread;
 // ---- what is given back ----
 S47DevWaves::FStand Stood;
 FString StoodArea;
 double StoodClock=0;
 TFunction<void(const FString&)> StoodUse;
 TWeakObjectPtr<AS47Walker> Shooter;
 S47DevWaves::FStand ReadStand() const;
 void WriteStand(const S47DevWaves::FStand& S) const;
 FString WhyNot() const;
 FString WhyOff() const;
 void Say(const FString& Text,float Seconds=5) const;
 // A new round after the card: bodies away, the gun full, back at the yard's middle.
 void Restart();
 // ---- the aliens ----
 TArray<TSharedPtr<FS47DevWaveBody>> Bodies;
 bool bNoKind[int32(S47DevWaves::EKind::Num)]={};
 int32 BodySerial=0, SpawnSerial=0;
 double SpawnWait=0, SpawnDry=0;
 FS47DevWaveBody* Take(S47DevWaves::EKind Want);
 bool Spawn();
 void StepBody(FS47DevWaveBody& B,double Dt);
 bool PathClear(const FS47DevWaveBody& B,double Heading) const;
 void Show(FS47DevWaveBody& B,bool bShow,double Dt) const;
 void PutAway(FS47DevWaveBody& B) const;
 // ---- the gun in the hand, what it throws out, what it sounds like, the bot (all built at Start and gone at Stop) ----
 TSharedPtr<FS47DevWaveFx> Fx;
 double Age=0, FightAge=0, HurtSaid=-10, LastFlash=-10, LastShot=-10, TailOwed=-1, HeartWait=0;
 int32 ShotSerial=0, LastBeat=-1;
 bool bTriggerWas=false;
 void BuildGun();
 void DropGun();
 void PoseGun(double Dt);
 void Fire(bool bPulled);
 void Reload();
 void StepReload(double Dt);
 void StepCrate(double Dt);
 void StepSound(double Dt);
 void StepBot(double Dt);
 void StepAuto(double Dt);
 void StepPictures(double Dt);
 void Strike(const FHitResult& Wall,const FVector& DirUE);
 // ---- the screen ----
 TSharedPtr<FS47DevWaveHud> HudText;
 TSharedPtr<SWidget> Hud;
 void ShowHud(bool bShow);
 void WriteHud(double Dt);
};

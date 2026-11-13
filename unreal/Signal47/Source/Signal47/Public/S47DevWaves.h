#pragma once
#include "CoreMinimal.h"

class US47Main;
class AS47Walker;
class FS47DevEvents;
class SWidget;
class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
struct FS47DevWaveBody;
struct FS47DevWaveHud;

// DEV TOY, not part of the game (Tom, 13 November 2026: "lag fps mode skytespill med waves med aliens i og rundt SARO, brukes kun
// fra F10 dev"). A first-person wave shooter in the SARO yard, started and stopped only from F10 ("Wave shooter (dev toy)").
// The Director never draws it, nothing of it is saved, and it does not start in a run nobody plays. The rules live here as plain
// numbers and small structs so that -S47SelfTest holds them ("devWaves:" lines, from FS47DevFreeform::SelfTest).
namespace S47DevWaves
{
 // Small: the small Roswell. Pale: the Pale One. Tall: the Tall One. Crab: the Pale One on all fours.
 enum class EKind : uint8 { Small, Pale, Tall, Crab, Num };
 enum class EPhase : uint8 { Off, Banner, Fight, Dead };

 inline constexpr int32 MagSize=12, StartHealth=100, TouchDamage=12, WaveHeal=25, MostAtOnce=8, MostInWave=40;
 inline constexpr double ReloadSeconds=1.4, ShotGap=.25, TouchReach=1.1, TouchGap=.9, Range=120, BannerSeconds=3.5, CardSeconds=6;
 // The muzzle's light: never closer together than this, so at most three in any second (the project's flash limit), and none at
 // all with Calm flashes chosen in Settings.
 inline constexpr double FlashGap=.34;

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
 inline int32 HealthOf(EKind K) { return K==EKind::Tall?3:K==EKind::Pale?2:1; }
 inline double PaceOf(EKind K) { return K==EKind::Crab?1.3:K==EKind::Small?1.15:K==EKind::Tall?.85:1.; }
 inline double HeightOf(EKind K) { return K==EKind::Tall?2.5:K==EKind::Pale?2.:K==EKind::Crab?.95:1.2; }
 inline double RadiusOf(EKind K) { return K==EKind::Crab?.6:K==EKind::Small?.32:.4; }
 inline int32 ScoreOf(EKind K) { return 100*HealthOf(K)+(K==EKind::Crab?50:0); }

 // ---- the gun: a magazine, a pause between shots, a reload that takes its time ----
 struct FGun
 {
  int32 Mag=MagSize;
  double Reload=0, Cool=0;
  bool CanFire() const { return Mag>0&&Reload<=0&&Cool<=0; }
  bool Fire() { if(!CanFire())return false; --Mag; Cool=ShotGap; return true; }
  bool StartReload() { if(Reload>0||Mag>=MagSize)return false; Reload=ReloadSeconds; return true; }
  // True in the step the magazine is full again.
  bool Step(double Dt)
  {
   Cool=FMath::Max(0.,Cool-Dt);
   if(Reload<=0)return false;
   Reload-=Dt;
   if(Reload>0)return false;
   Reload=0; Mag=MagSize;
   return true;
  }
 };

 inline int32 Hurt(int32 Health,int32 Damage) { return FMath::Max(0,Health-FMath::Max(0,Damage)); }
 inline bool FlashMay(double Now,double Last,bool bCalm) { return !bCalm&&Now-Last>=FlashGap; }

 // ---- one round: the waves in turn, the health, the score ----
 struct FRound
 {
  EPhase Phase=EPhase::Off;
  int32 Wave=0, ToCome=0, Out=0, Health=StartHealth, Score=0, Kills=0;
  double Wait=0;
  void Begin() { *this=FRound(); Wave=1; ToCome=WaveSize(1); Phase=EPhase::Banner; Wait=BannerSeconds; }
  int32 Left() const { return ToCome+Out; }
  bool MaySpawn() const { return Phase==EPhase::Fight&&ToCome>0&&Out<WaveAtOnce(Wave); }
  void Spawned() { if(ToCome>0){ --ToCome; ++Out; } }
  // One that was out is gone without being shot (its body went with its world): it comes again.
  void Lost() { if(Out>0){ --Out; ++ToCome; } }
  void Killed(EKind K)
  {
   Out=FMath::Max(0,Out-1); ++Kills; Score+=ScoreOf(K)*FMath::Max(1,Wave);
   if(Phase==EPhase::Fight&&Left()==0)
   {
    ++Wave; ToCome=WaveSize(Wave); Phase=EPhase::Banner; Wait=BannerSeconds;
    Health=FMath::Min(StartHealth,Health+WaveHeal);
   }
  }
  void Touched()
  {
   if(Phase!=EPhase::Fight)return;
   Health=Hurt(Health,TouchDamage);
   if(Health<=0){ Phase=EPhase::Dead; Wait=CardSeconds; }
  }
  // True once the "wave reached" card has been up its time: the round is over.
  bool Step(double Dt)
  {
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
 static bool On(const US47Main* M);
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 S47DevWaves::FRound Round;
 S47DevWaves::FGun Gun;
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
 // ---- the gun in the hand ----
 TWeakObjectPtr<USceneComponent> GunRoot;
 TWeakObjectPtr<UStaticMeshComponent> Barrel, Tracer;
 TWeakObjectPtr<UPointLightComponent> Muzzle;
 TWeakObjectPtr<UMaterialInstanceDynamic> TracerMat;
 TArray<TWeakObjectPtr<USceneComponent>> GunParts;
 double Kick=0, TracerLeft=0, MuzzleLeft=0, LastFlash=-10, Age=0, HurtSaid=-10;
 void BuildGun();
 void DropGun();
 void PoseGun(double Dt);
 void Fire(bool bPulled);
 void Reload();
 // ---- the screen ----
 TSharedPtr<FS47DevWaveHud> HudText;
 TSharedPtr<SWidget> Hud;
 void ShowHud(bool bShow);
 void WriteHud();
};

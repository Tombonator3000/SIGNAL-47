#include "S47DevWaves.h"
#include "S47Abduction.h"
#include "S47Attention.h"
#include "S47Audio.h"
#include "S47Away.h"
#include "S47DevEvents.h"
#include "S47DevFreeform.h"
#include "S47DoorLoop.h"
#include "S47FarmSite.h"
#include "S47FieldCamera.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Gamepad.h"
#include "S47Handling.h"
#include "S47IndoorDread.h"
#include "S47Keys.h"
#include "S47Main.h"
#include "S47PaleBody.h"
#include "S47Phones.h"
#include "S47RoswellMotion.h"
#include "S47ScriptPlayer.h"
#include "S47Taken.h"
#include "S47Theme.h"
#include "S47Three.h"
#include "S47TripoCreature.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47WeekCase.h"
#include "S47World.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "UnrealClient.h"
#include "Widgets/SLeafWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47DevWaves,Log,All);

// DEV TOY (Tom, 13 November 2026): the F10 wave shooter at SARO. See S47DevWaves.h. Nothing here is part of the game: it is started
// only from the F10 line, holds the night while it runs (FS47DevWaves::On, asked where the director's camera is asked) and gives
// the player back as they stood. It uses the aliens' own bodies and motion (FS47PaleBody, FS47RoswellMotion). The gun is our own
// (ArtSource/DevWaves/build_carbine.py, /Game/Signal47/Dev/Waves); its feel is code: springs for the kick, the sway and the landing,
// a reload in beats, casings and chips that fall, soft sprites for flash, smoke, dust and the aliens' light.

// One alien body of the shooter: built once on the web actor's root (as the F10 "Eye glint" bodies are), out or put away.
struct FS47DevWaveBody
{
 FS47PaleBody Body;
 FS47RoswellMotion Motion;
 S47DevWaves::EKind Kind=S47DevWaves::EKind::Pale;
 bool bOut=false;
 FVector At=FVector::ZeroVector; // the feet, scene metres
 FVector Scale0=FVector::OneVector; // its root's own scale, given back when it is put away
 // Fall: below 0 on its feet, else seconds since it was shot down. Arrive: seconds until it stands where its light is.
 double Yaw=0,Clock=0,Travelled=0,Speed=0,TouchWait=0,Stagger=0,StaggerRest=0,Fall=-1,Blocked=0,Steer=0,SteerLeft=0,Arrive=0,Flank=0,PaceK=1,MoteWait=0;
 S47DevWaves::FSpring Lean,Twist; // struck: back and round with the round, and up again
 int32 Hp=1;
 bool bFizzed=false;
};

// What the screen reads: written by the shooter each step, read by the widget (which never touches the shooter itself).
struct FS47DevWaveHud
{
 S47DevWaves::EPhase Phase=S47DevWaves::EPhase::Off;
 int32 Wave=1,Left=0,Score=0,Health=100,Mag=0,Reserve=0,Times=1,Kills=0,Best=0,Heads=0;
 float Gap=8,Shown=1,Reload=0,HitK=0,KillK=0,HurtK=0,LowK=0,Pulse=0,CardK=0,CardT=0,GotK=0,DeadLeft=0;
 float HurtAt[3]={0,0,0},HurtDeg[3]={0,0,0}; // up to three marks: what is left of each, and where from (degrees clockwise from ahead)
 FString Card,CardSub,Got;
 bool bCrate=false,bBot=false,bHeadKill=false;
 float CrateDeg=0,CrateM=0;
};

// One soft sprite: a plane turned to the eye, a glow (added light) or a puff (smoke, dust).
struct FS47DevWaveSprite
{
 TWeakObjectPtr<UStaticMeshComponent> Comp;
 TWeakObjectPtr<UMaterialInstanceDynamic> Mid;
 bool bLive=false,bAlong=false,bVel=false;
 FVector Pos=FVector::ZeroVector,Vel=FVector::ZeroVector,Axis=FVector::ForwardVector; // Unreal centimetres
 FLinearColor Tint=FLinearColor::White;
 double Age=0,Life=1,Size0=10,Size1=10,Len0=0,Len1=0,Bright=1,Alpha=0,Gravity=0,Drag=0,Pow=1,Delay=0;
};

// One small thing that flies and falls: a casing, a chip.
struct FS47DevWaveBit
{
 TWeakObjectPtr<UStaticMeshComponent> Comp;
 TWeakObjectPtr<UMaterialInstanceDynamic> Mid;
 bool bLive=false;
 FVector Pos=FVector::ZeroVector,Vel=FVector::ZeroVector,Size=FVector::OneVector;
 FRotator Rot=FRotator::ZeroRotator,Spin=FRotator::ZeroRotator;
 double Age=0,Life=3,Ground=0,Bounce=.35;
 int32 Hits=0;
 FString Tink;
 float TinkGain=0;
};

// One mark a round leaves: a decal that fades.
struct FS47DevWaveMark
{
 TWeakObjectPtr<UDecalComponent> Comp;
 TWeakObjectPtr<UMaterialInstanceDynamic> Mid;
 double Age=0,Life=0,Alpha=0;
};

namespace S47DevWavesLocal
{
 const TCHAR* const HoldName=TEXT("devwaves");
 const TCHAR* const MenuGroup=TEXT("Wave shooter (dev toy)");
 const TCHAR* const AssetRoot=TEXT("/Game/Signal47/Dev/Waves/");
 // The SARO yard, as F10's "Go: SARO yard".
 constexpr double YardX=7,YardZ=19;
 constexpr double FallSeconds=.5,LieSeconds=.35,GoneSeconds=.95,ArriveSeconds=.9;
 constexpr int32 MostBodies=14;
 // No alien runs faster than this share over its kind's pace, and none lunges harder than this at the end (so wave 1 stays slower than the player's run).
 constexpr double PaceSpread=.12,LungeTimes=1.12;
 // The gun's own centimetres: x ahead, y right, z up, the origin on the sight line at the rear ring (as the model is built).
 const FVector MuzzleAt(66.,0.,-5.5),PortAt(9.5,2.4,-4.1),LampAt(48.4,0.,-9.9),MagPivot(12.2,0.,-9.2);

 FS47DevWaves*& Live(){static FS47DevWaves* P=nullptr;return P;}
 inline double Rnd(){return double(FMath::FRand());}
 inline double Rnd(double A,double B){return A+(B-A)*double(FMath::FRand());}

 bool AutoFlag()
 {
  // (the command line does not change: read once, not every frame)
  static const bool bAuto=[]
  {
   const TCHAR* Cmd=FCommandLine::Get();
   FString V;
   return FParse::Value(Cmd,TEXT("S47DevWaves="),V)&&V.Equals(TEXT("auto"),ESearchCase::IgnoreCase)&&!FParse::Param(Cmd,TEXT("S47SelfTest"));
  }();
  return bAuto;
 }

 // A run nobody plays, or one that tries every F10 line by itself: the shooter is never started there (but for -S47DevWaves=auto,
 // which asks for exactly that).
 bool NobodyPlays()
 {
  const TCHAR* Cmd=FCommandLine::Get();
  FString Dir;
  if(FParse::Param(Cmd,TEXT("S47SelfTest"))||FParse::Value(Cmd,TEXT("S47DevSweep="),Dir)||FParse::Value(Cmd,TEXT("S47DevEventsContract="),Dir)||FParse::Value(Cmd,TEXT("S47DevProof="),Dir)||FParse::Value(Cmd,TEXT("S47ShotStills="),Dir))return true;
  if(AutoFlag())return false;
  return FApp::IsUnattended()||FParse::Param(Cmd,TEXT("unattended"))||FParse::Param(Cmd,TEXT("S47SmokeTest"));
 }

 const TCHAR* KindName(S47DevWaves::EKind K)
 {
  switch(K)
  {
  case S47DevWaves::EKind::Small:return TEXT("small Roswell");
  case S47DevWaves::EKind::Tall:return TEXT("Tall One");
  case S47DevWaves::EKind::Crab:return TEXT("crab");
  default:return TEXT("Pale One");
  }
 }

 // What a round struck, by the names of the thing and what it is made of (the world has no physical materials).
 enum class ESurf:uint8{Dirt,Metal,Stone};
 ESurf SurfOf(const FHitResult& Hit)
 {
  FString Name;
  if(const UPrimitiveComponent* C=Hit.GetComponent())
  {
   Name=C->GetName();
   if(const UMaterialInterface* Mat=C->GetMaterial(0))Name+=TEXT(" ")+Mat->GetName();
  }
  Name=Name.ToLower();
  static const TCHAR* const Metal[]={TEXT("metal"),TEXT("steel"),TEXT("iron"),TEXT("tin"),TEXT("alu"),TEXT("rust"),TEXT("galv"),TEXT("dish"),TEXT("truck"),TEXT("pipe"),TEXT("fence"),TEXT("tank"),TEXT("antenna"),TEXT("sign"),TEXT("door"),TEXT("barrel"),TEXT("chrome"),TEXT("jeep"),TEXT("mast")};
  for(const TCHAR* K:Metal)if(Name.Contains(K))return ESurf::Metal;
  static const TCHAR* const Dirt[]={TEXT("sand"),TEXT("dirt"),TEXT("ground"),TEXT("soil"),TEXT("gravel"),TEXT("terrain"),TEXT("desert"),TEXT("grass"),TEXT("relief"),TEXT("caliche")};
  for(const TCHAR* K:Dirt)if(Name.Contains(K))return ESurf::Dirt;
  return Hit.ImpactNormal.Z>.7?ESurf::Dirt:ESurf::Stone;
 }

 // The shooter's screen, in the look of the game's own panels: terminal type, amber and phosphor on the night, nothing but what
 // the hand needs. Painted, not laid out: the crosshair opens with the spread, the marks fade by themselves. It takes no clicks.
 class SS47DevWavesHud:public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SS47DevWavesHud){}
   SLATE_ARGUMENT(TSharedPtr<FS47DevWaveHud>,Source)
  SLATE_END_ARGS()
  void Construct(const FArguments& InArgs)
  {
   Source=InArgs._Source;
   SetVisibility(EVisibility::HitTestInvisible);
  }
  virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(1920,1080);}
  virtual int32 OnPaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Cull,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& InStyle,bool bParentEnabled) const override
  {
   using namespace S47Theme;
   using S47DevWaves::EPhase;
   const FS47DevWaveHud* H=Source.Get();
   if(!H||H->Phase==EPhase::Off||!FSlateApplication::IsInitialized())return Layer;
   const FVector2f Sz=FVector2f(G.GetLocalSize());
   const FVector2f Mid=Sz*.5f;
   const TSharedRef<FSlateFontMeasure> Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
   // Words at a point (Align 0: from it, .5: about it, 1: up to it), with a dark copy under them so they read on any ground.
   const auto Words=[&](const FString& S,const FSlateFontInfo& Font,FVector2f At,float Align,const FLinearColor& Col)
   {
    const FVector2f Ext(Measure->Measure(S,Font));
    At.X-=Ext.X*Align;
    FSlateDrawElement::MakeText(Out,Layer+3,G.ToPaintGeometry(Ext+FVector2f(6,6),FSlateLayoutTransform(At+FVector2f(Px(1.5f),Px(1.5f)))),S,Font,ESlateDrawEffect::None,Night(Col.A*.9f));
    FSlateDrawElement::MakeText(Out,Layer+4,G.ToPaintGeometry(Ext+FVector2f(6,6),FSlateLayoutTransform(At)),S,Font,ESlateDrawEffect::None,Col);
    return Ext;
   };
   const auto Stroke=[&](const FVector2f& A,const FVector2f& B,const FLinearColor& Col,float Thick,int32 L)
   {
    TArray<FVector2f> P;
    P.Add(A);P.Add(B);
    FSlateDrawElement::MakeLines(Out,L,G.ToPaintGeometry(),MoveTemp(P),ESlateDrawEffect::None,Col,true,Thick);
   };
   const bool bDead=H->Phase==EPhase::Dead;
   // ---- low health and the blow: the edges close in and redden (a slow pulse, never a flash) ----
   {
    const float Dark=FMath::Clamp(.55f*H->LowK+.25f*H->HurtK+(bDead?.6f:0.f),0.f,.8f);
    if(Dark>.01f)Vignette(Out,Layer,G,.5f,Dark);
    const float Red=FMath::Clamp(.34f*H->HurtK+(.1f+.07f*H->Pulse)*H->LowK,0.f,.5f);
    if(Red>.005f)
    {
     TArray<TPair<float,FLinearColor>> In,Back;
     In.Emplace(0.f,Neon(Red));In.Emplace(1.f,Neon(0.f));
     Back.Emplace(0.f,Neon(0.f));Back.Emplace(1.f,Neon(Red));
     Gradient(Out,Layer+1,G,FVector2f(0,0),FVector2f(Sz.X*.2f,Sz.Y),In,true);
     Gradient(Out,Layer+1,G,FVector2f(Sz.X*.8f,0),FVector2f(Sz.X*.2f,Sz.Y),Back,true);
     Gradient(Out,Layer+1,G,FVector2f(0,0),FVector2f(Sz.X,Sz.Y*.2f),In,false);
     Gradient(Out,Layer+1,G,FVector2f(0,Sz.Y*.8f),FVector2f(Sz.X,Sz.Y*.2f),Back,false);
    }
   }
   // ---- where the blow came from ----
   for(int32 I=0;I<3;++I)if(H->HurtAt[I]>.01f)
   {
    const float A=FMath::DegreesToRadians(H->HurtDeg[I]),K=FMath::Clamp(H->HurtAt[I],0.f,1.f);
    const FVector2f Dir(FMath::Sin(A),-FMath::Cos(A)),Side(-Dir.Y,Dir.X);
    const FVector2f Base=Mid+Dir*Px(150);
    Triangle(Out,Layer+2,G,Base+Dir*Px(34),Base-Side*Px(46),Base+Side*Px(46),Neon(.62f*K));
    Triangle(Out,Layer+2,G,Base+Dir*Px(20),Base-Side*Px(26),Base+Side*Px(26),Night(.55f*K));
   }
   // ---- the crosshair: four strokes that open with the spread, gone down the sights ----
   if(!bDead&&H->Shown>.02f)
   {
    const float Gap=Px(H->Gap),Len=Px(9);
    const FLinearColor Col=Paper(.92f*H->Shown),Under=Night(.75f*H->Shown);
    for(const FVector2f& D:{FVector2f(1,0),FVector2f(-1,0),FVector2f(0,1),FVector2f(0,-1)})
    {
     Stroke(Mid+D*(Gap-Px(1)),Mid+D*(Gap+Len+Px(1)),Under,Px(4.5f),Layer+2);
     Stroke(Mid+D*Gap,Mid+D*(Gap+Len),Col,Px(2),Layer+3);
    }
    Fill(Out,Layer+3,G,Mid-FVector2f(Px(1.2f),Px(1.2f)),FVector2f(Px(2.4f),Px(2.4f)),Col);
   }
   // ---- struck, and down: four ticks round the middle (white; red and wider for a kill, amber for one in the head) ----
   {
    const auto Ticks=[&](float K,float R0,float Len,float Thick,const FLinearColor& Col)
    {
     for(const FVector2f& D:{FVector2f(1,1),FVector2f(-1,1),FVector2f(1,-1),FVector2f(-1,-1)})
     {
      const FVector2f N=D*.7071f;
      Stroke(Mid+N*(R0-Px(1)),Mid+N*(R0+Len+Px(1)),Night(.8f*K),Thick+Px(2.5f),Layer+3);
      Stroke(Mid+N*R0,Mid+N*(R0+Len),Col,Thick,Layer+4);
     }
    };
    if(H->KillK>.01f)Ticks(H->KillK,Px(8.f+10.f*(1.f-H->KillK)),Px(13),Px(3),H->bHeadKill?Amber(H->KillK):Neon(H->KillK));
    else if(H->HitK>.01f)Ticks(H->HitK,Px(6.f+5.f*(1.f-H->HitK)),Px(8),Px(2),Paper(H->HitK));
   }
   // ---- top: what this is, the wave, the score ----
   Words(H->bBot?TEXT("DEV TOY  /  WAVE SHOOTER  /  BOT AT THE TRIGGER"):TEXT("DEV TOY  /  WAVE SHOOTER"),Term(18,1),FVector2f(Px(28),Px(20)),0.f,Mute(.9f));
   Words(FString::Printf(TEXT("WAVE %02d"),H->Wave),Heading(40,3),FVector2f(Mid.X,Px(10)),.5f,Paper(.96f));
   Words(H->Phase==EPhase::Fight?FString::Printf(TEXT("HOSTILES  %02d"),H->Left):H->Phase==EPhase::Banner?FString(TEXT("STAND BY")):FString(TEXT("DOWN")),Term(26,2),FVector2f(Mid.X,Px(68)),.5f,H->Phase==EPhase::Fight?Amber(.95f):Mute(.9f));
   Fill(Out,Layer+2,G,FVector2f(Mid.X-Px(80),Px(64)),FVector2f(Px(160),Px(1.5f)),Amber(.55f));
   Words(TEXT("SCORE"),Term(18,1),FVector2f(Sz.X-Px(28),Px(70)),1.f,Mute(.9f));
   Words(FString::Printf(TEXT("%06d"),H->Score),Term(38,1),FVector2f(Sz.X-Px(28),Px(86)),1.f,Paper(.96f));
   if(H->Times>1)Words(FString::Printf(TEXT("STREAK  x%d"),H->Times),Term(24,1),FVector2f(Sz.X-Px(28),Px(132)),1.f,Amber(.95f));
   if(H->GotK>.01f&&!H->Got.IsEmpty())Words(H->Got,Term(22,1),FVector2f(Sz.X-Px(28),Px(H->Times>1?162.f:132.f)),1.f,Phos(FMath::Min(1.f,H->GotK)));
   // ---- where the crate is ----
   if(H->bCrate&&!bDead)
   {
    Words(FString::Printf(TEXT("SUPPLY CRATE  %d M"),FMath::RoundToInt(H->CrateM)),Term(22,1),FVector2f(Mid.X,Px(104)),.5f,Phos(.95f));
    const float A=FMath::DegreesToRadians(H->CrateDeg);
    const FVector2f Dir(FMath::Sin(A),-FMath::Cos(A)),Side(-Dir.Y,Dir.X);
    const FVector2f Base=Mid+Dir*Px(118);
    Triangle(Out,Layer+2,G,Base+Dir*Px(16),Base-Side*Px(9),Base+Side*Px(9),Phos(.8f));
   }
   // ---- bottom left: the body ----
   {
    const FLinearColor Col=H->Health>50?Phos(.95f):H->Health>25?Amber(.95f):Neon(.95f);
    const float X0=Px(30),Y0=Sz.Y-Px(96);
    Words(TEXT("VITALS"),Term(18,1),FVector2f(X0,Y0-Px(4)),0.f,Mute(.9f));
    Words(FString::Printf(TEXT("%03d"),H->Health),Term(44,1),FVector2f(X0,Y0+Px(14)),0.f,Col);
    const float Bx=X0+Px(88),By=Y0+Px(36),Bw=Px(236),Bh=Px(13);
    Fill(Out,Layer+2,G,FVector2f(Bx-Px(3),By-Px(3)),FVector2f(Bw+Px(6),Bh+Px(6)),Night(.7f));
    for(int32 I=0;I<10;++I)
    {
     const bool bOn=H->Health>I*10;
     const float Part=bOn?FMath::Clamp((H->Health-I*10)/10.f,0.f,1.f):0.f;
     const float W=Bw/10.f-Px(3);
     Fill(Out,Layer+3,G,FVector2f(Bx+I*Bw/10.f,By),FVector2f(W,Bh),Rule(.55f));
     if(bOn)Fill(Out,Layer+4,G,FVector2f(Bx+I*Bw/10.f,By),FVector2f(W*Part,Bh),Col);
    }
   }
   // ---- bottom right: the gun: the rounds as a magazine, the count, what is carried ----
   {
    const bool bLow=H->Mag<=8;
    const FLinearColor Col=bLow?Neon(.95f):Amber(.95f);
    const float Xr=Sz.X-Px(30),Y0=Sz.Y-Px(104);
    Words(TEXT("SARO SEC-7 CARBINE"),Term(18,1),FVector2f(Xr,Y0-Px(4)),1.f,Mute(.9f));
    // the magazine: thirty rounds in two columns, emptied from the top
    const float Mw=Px(34),Mh=Px(66),Mx=Xr-Mw,My=Y0+Px(22);
    Fill(Out,Layer+2,G,FVector2f(Mx-Px(4),My-Px(4)),FVector2f(Mw+Px(8),Mh+Px(8)),Night(.7f));
    Fill(Out,Layer+3,G,FVector2f(Mx-Px(4),My+Mh+Px(4)),FVector2f(Mw+Px(8),Px(3)),Rule(.9f));
    for(int32 I=0;I<S47DevWaves::MagSize;++I)
    {
     const int32 Row=I/2,ColI=I%2;
     const float Py=My+Mh-(Row+1)*(Mh/15.f)+Px(.8f);
     Fill(Out,Layer+3,G,FVector2f(Mx+ColI*(Mw/2.f)+Px(1),Py),FVector2f(Mw/2.f-Px(2),Mh/15.f-Px(1.6f)),I<H->Mag?Col:Rule(.45f));
    }
    const FVector2f Big=Words(FString::Printf(TEXT("%02d"),H->Mag),Term(68,1),FVector2f(Mx-Px(86),Y0+Px(8)),1.f,Col);
    (void)Big;
    Words(FString::Printf(TEXT("/ %03d"),H->Reserve),Term(30,1),FVector2f(Mx-Px(78),Y0+Px(42)),0.f,H->Reserve>0?Paper(.9f):Neon(.9f));
    if(H->Reload>.001f)
    {
     Words(TEXT("RELOADING"),Term(18,2),FVector2f(Mx-Px(14),Y0+Px(78)),1.f,Amber(.95f));
     Fill(Out,Layer+2,G,FVector2f(Mx-Px(180),Y0+Px(100)),FVector2f(Px(166),Px(4)),Rule(.6f));
     Fill(Out,Layer+3,G,FVector2f(Mx-Px(180),Y0+Px(100)),FVector2f(Px(166)*FMath::Clamp(H->Reload,0.f,1.f),Px(4)),Amber(.95f));
    }
    else if(H->Mag<=0&&H->Reserve<=0)Words(TEXT("NO ROUNDS  /  FETCH THE CRATE"),Term(18,1),FVector2f(Mx-Px(14),Y0+Px(78)),1.f,Neon(.95f));
    else if(bLow)Words(TEXT("RELOAD  [R]"),Term(18,2),FVector2f(Mx-Px(14),Y0+Px(78)),1.f,Neon(.9f));
   }
   // ---- the card: the wave coming, or the round lost ----
   if(H->CardK>.01f&&!H->Card.IsEmpty())
   {
    const float K=FMath::Clamp(H->CardK,0.f,1.f),Grow=FMath::Clamp(H->CardT/.45f,0.f,1.f),Ease=1.f-(1.f-Grow)*(1.f-Grow);
    const float Cy=Sz.Y*.27f,Half=Px(62);
    TArray<TPair<float,FLinearColor>> Band;
    Band.Emplace(0.f,Night(0.f));Band.Emplace(.3f,Night(.74f*K));Band.Emplace(.7f,Night(.74f*K));Band.Emplace(1.f,Night(0.f));
    Gradient(Out,Layer+2,G,FVector2f(0,Cy-Half),FVector2f(Sz.X,Half*2),Band,true);
    const FLinearColor Line=bDead?Neon(.9f*K):Amber(.9f*K);
    const float Wide=Sz.X*.2f*Ease;
    Fill(Out,Layer+3,G,FVector2f(Mid.X-Wide,Cy-Half+Px(8)),FVector2f(Wide*2,Px(2)),Line);
    Fill(Out,Layer+3,G,FVector2f(Mid.X-Wide,Cy+Half-Px(10)),FVector2f(Wide*2,Px(2)),Line);
    Words(H->Card,Heading(58,8),FVector2f(Mid.X+Px(40)*(1.f-Ease),Cy-Px(50)),.5f,Paper(.98f*K));
    if(!H->CardSub.IsEmpty())Words(H->CardSub,Term(26,2),FVector2f(Mid.X-Px(40)*(1.f-Ease),Cy+Px(18)),.5f,bDead?Paper(.9f*K):Amber(.95f*K));
    if(bDead)
    {
     Words(FString::Printf(TEXT("NEW ROUND IN %d"),FMath::Max(1,FMath::CeilToInt(H->DeadLeft))),Term(20,2),FVector2f(Mid.X,Cy+Half+Px(14)),.5f,Mute(.95f));
     Words(TEXT("F10  /  WAVE SHOOTER: STOP  ENDS IT"),Term(15,1),FVector2f(Mid.X,Cy+Half+Px(42)),.5f,Mute(.8f));
    }
   }
   return Layer+5;
  }
 private:
  TSharedPtr<FS47DevWaveHud> Source;
 };
}

// The gun in the hand and everything it throws out: built at Start, gone at Stop. Places are Unreal centimetres.
struct FS47DevWaveFx
{
 US47Main* M=nullptr;
 TWeakObjectPtr<AS47Walker> Walker;
 TWeakObjectPtr<AS47Web> Web;
 TWeakObjectPtr<UCameraComponent> Cam;
 TArray<TWeakObjectPtr<USceneComponent>> Parts; // everything made here, to be taken down again
 TWeakObjectPtr<USceneComponent> Holder;
 TWeakObjectPtr<UStaticMeshComponent> MagComp,BoltComp,ChargeComp,CrateComp;
 TWeakObjectPtr<UPointLightComponent> MuzzleLight,FillLight;
 TWeakObjectPtr<USpotLightComponent> Lamp,CrateLamp;
 TWeakObjectPtr<UStaticMesh> CubeMesh,CasingMesh,MagMesh;
 TWeakObjectPtr<UMaterialInterface> LitMat,MarkMat;
 TArray<FS47DevWaveSprite> Glows,Softs;
 TArray<FS47DevWaveBit> Bits;
 TArray<FS47DevWaveMark> Marks;
 int32 MarkNext=0,Freeze=0;
 bool bModel=false,bFpFov=false;
 // ---- the hand ----
 S47DevWaves::FSpring KickBack,KickPitch,KickRoll,KickSide,ViewPitch,ViewYaw,Land;
 double Climb=0,ClimbYaw=0,AdsK=0,SprintK=0,ReloadK=0,MoveK=0,BobPhase=0,SwayYaw=0,SwayPitch=0,SwayRoll=0,Retract=0,DeadK=0,BoltPos=0,ShotPulse=0,FpK=1,MuzzleLeft=0,DayK=0,Enclosed=0,EnclosedWait=0,PrevYaw=0,PrevPitch=0;
 float FovK=1;
 FVector PrevPos=FVector::ZeroVector,PlayerVel=FVector::ZeroVector;
 bool bPrevAir=false,bLockedBack=false,bPrevValid=false,bTrigger=false,bMagGone=false;
 // ---- what is given back ----
 TFunction<bool(double,double&,double&,float&)> PrevLayer;
 double PrevMoveScale=1;
 bool bSatSet=false;
 // ---- the crate ----
 FVector CrateAt=FVector::ZeroVector; // scene metres
 bool bCrateOut=false;
 // ---- the sounds ----
 TArray<FString> SoundKeys;
 TSet<FString> BoughtKeys;
 // ---- the bot and the pictures (a run with -S47DevWaves=auto) ----
 bool bBot=false,bBotFire=false,bBotReload=false,bVideoOn=false,bVideoDone=false,bForcedLow=false,bForcedDead=false,bPicturesSaid=false;
 double BotBurst=0,BotRest=0,BotStrafe=1,BotStrafeLeft=0,BotZoom=0,LastSnap=-10,VideoFrom=5,VideoLen=14,VideoAge=0;
 int32 VideoFrame=0;
 FString ShotsDir,VideoDir,Tag;
 TSet<FString> Snapped;

 // Where a point of the gun (its own centimetres) is seen in the world: the gun is drawn with a field of view of its own, which
 // draws it narrower or wider about the view's axis than the world round it.
 FVector GunPoint(const FVector& GunCm) const
 {
  const USceneComponent* H=Holder.Get();
  const UCameraComponent* C=Cam.Get();
  if(!H)return FVector::ZeroVector;
  const FVector World=H->GetComponentTransform().TransformPosition(GunCm);
  if(!C||!bFpFov)return World;
  const FVector InView=C->GetComponentTransform().InverseTransformPosition(World);
  return C->GetComponentTransform().TransformPosition(FVector(InView.X,InView.Y*FpK,InView.Z*FpK));
 }

 // One of the shooter's own sounds (the bought take when it is in this build, else our synthesized one); with neither, a sound the game has.
 void Snd(const TCHAR* Name,const FS47Sound& O,const TCHAR* Else=nullptr) const
 {
  if(!M||!M->Audio)return;
  const FString Key=FString(TEXT("dw:"))+Name;
  if(M->Audio->Buf.Contains(Key))M->Audio->Sfx(Key,O);
  else if(Else)M->Audio->Sfx(Else,O);
 }
 // The bought takes are louder than ours where ours were set by layer: this evens the tails.
 float TailGain(const TCHAR* Name) const { return BoughtKeys.Contains(FString(TEXT("dw:"))+Name)?.7f:1.f; }

 void LoadSounds()
 {
  if(!M||!M->Audio||!M->Audio->Ctx.IsValid())return;
  static const TCHAR* const Names[]={TEXT("shot_transient"),TEXT("shot_body"),TEXT("shot_tail_yard"),TEXT("shot_tail_walls"),TEXT("shot_mech"),TEXT("dry_click"),TEXT("mag_out"),TEXT("mag_in"),
   TEXT("bolt_pull"),TEXT("bolt_release"),TEXT("casing_1"),TEXT("casing_2"),TEXT("casing_3"),TEXT("impact_dirt"),TEXT("impact_metal"),TEXT("impact_concrete"),TEXT("alien_hit"),TEXT("alien_death"),
   TEXT("alien_dissolve"),TEXT("tall_stagger"),TEXT("wave_sting"),TEXT("wave_clear"),TEXT("heartbeat"),TEXT("hit_marker"),TEXT("kill_marker"),TEXT("pickup"),TEXT("player_hurt"),TEXT("ads_in")};
  // Bought takes (Tom's library, never in Git: Content/Licensed) first when they are in this build; ours (Tools/devwaves/make_sounds.py) always.
  const FString Bought=FPaths::ProjectContentDir()/TEXT("Licensed/DevWaves"),Ours=FPaths::ProjectContentDir()/TEXT("DevWaves/Audio");
  const bool bOursOnly=FParse::Param(FCommandLine::Get(),TEXT("S47DevWavesSynth"));
  int32 NumBought=0,NumOurs=0;
  for(const TCHAR* Name:Names)
  {
   const FString Key=FString(TEXT("dw:"))+Name;
   if(M->Audio->Buf.Contains(Key))continue;
   TArray<uint8> Bytes;
   TSharedPtr<FS47AudioBuffer> Buffer;
   bool bBought=false;
   if(!bOursOnly&&FFileHelper::LoadFileToArray(Bytes,*((Bought/Name)+TEXT(".wav")),FILEREAD_Silent)){Buffer=S47Wa::DecodeWav(Bytes);bBought=Buffer.IsValid();}
   if(!Buffer.IsValid()&&FFileHelper::LoadFileToArray(Bytes,*((Ours/Name)+TEXT(".wav")),FILEREAD_Silent))Buffer=S47Wa::DecodeWav(Bytes);
   if(!Buffer.IsValid())continue;
   M->Audio->Buf.Add(Key,Buffer);
   SoundKeys.Add(Key);
   if(bBought){BoughtKeys.Add(Key);++NumBought;}
   else ++NumOurs;
  }
  UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: %d sounds (%d bought takes, %d our own)"),NumBought+NumOurs,NumBought,NumOurs);
 }
 void DropSounds()
 {
  if(M&&M->Audio)for(const FString& Key:SoundKeys)M->Audio->Buf.Remove(Key);
  SoundKeys.Reset();BoughtKeys.Reset();
 }

 FS47DevWaveSprite* Spark(bool bSoft,const FVector& PosCm,const FVector& VelCm,const FLinearColor& Tint,double Size0,double Size1,double Life,double Bright,double Alpha=0)
 {
  FS47DevWaveSprite* Pick=nullptr;
  for(FS47DevWaveSprite& S:(bSoft?Softs:Glows))
  {
   if(!S.Comp.IsValid())continue;
   if(!S.bLive){Pick=&S;break;}
   if(!Pick||S.Age/S.Life>Pick->Age/Pick->Life)Pick=&S;
  }
  if(!Pick)return nullptr;
  const TWeakObjectPtr<UStaticMeshComponent> C=Pick->Comp;
  const TWeakObjectPtr<UMaterialInstanceDynamic> Mi=Pick->Mid;
  *Pick=FS47DevWaveSprite();
  Pick->Comp=C;Pick->Mid=Mi;
  Pick->bLive=true;Pick->Pos=PosCm;Pick->Vel=VelCm;Pick->Tint=Tint;Pick->Size0=Size0;Pick->Size1=Size1;Pick->Life=FMath::Max(Life,.01);Pick->Bright=Bright;Pick->Alpha=Alpha;
  return Pick;
 }

 FS47DevWaveBit* Throw(UStaticMesh* Mesh,const FVector& PosCm,const FVector& VelCm,const FVector& Size,double Life,double GroundCm,const FLinearColor* Colour=nullptr)
 {
  FS47DevWaveBit* Pick=nullptr;
  for(FS47DevWaveBit& B:Bits)
  {
   if(!B.Comp.IsValid())continue;
   if(!B.bLive){Pick=&B;break;}
   if(!Pick||B.Age/B.Life>Pick->Age/Pick->Life)Pick=&B;
  }
  UStaticMeshComponent* C=Pick?Pick->Comp.Get():nullptr;
  if(!C||!Mesh)return nullptr;
  UMaterialInstanceDynamic* Mi=Pick->Mid.Get();
  *Pick=FS47DevWaveBit();
  Pick->Comp=C;Pick->Mid=Mi;
  Pick->bLive=true;Pick->Pos=PosCm;Pick->Vel=VelCm;Pick->Size=Size;Pick->Life=Life;Pick->Ground=GroundCm;
  Pick->Rot=FRotator(S47DevWavesLocal::Rnd(-180,180),S47DevWavesLocal::Rnd(-180,180),S47DevWavesLocal::Rnd(-180,180));
  Pick->Spin=FRotator(S47DevWavesLocal::Rnd(-900,900),S47DevWavesLocal::Rnd(-900,900),S47DevWavesLocal::Rnd(-900,900));
  if(C->GetStaticMesh()!=Mesh)C->SetStaticMesh(Mesh);
  // its own materials, or the plain one in a colour
  const int32 Slots=C->GetNumMaterials();
  for(int32 I=0;I<Slots;++I)C->SetMaterial(I,Colour&&Mi?static_cast<UMaterialInterface*>(Mi):nullptr);
  if(Colour&&Mi)Mi->SetVectorParameterValue(TEXT("Color"),*Colour);
  C->SetWorldLocationAndRotation(PosCm,Pick->Rot);
  C->SetWorldScale3D(Size);
  C->SetVisibility(true);
  return Pick;
 }

 void Mark(const FVector& PosCm,const FVector& Normal,double SizeCm,const FLinearColor& Colour,double Alpha,double Life)
 {
  if(Marks.Num()==0)return;
  FS47DevWaveMark& K=Marks[MarkNext++%Marks.Num()];
  UDecalComponent* D=K.Comp.Get();
  UMaterialInstanceDynamic* Mi=K.Mid.Get();
  if(!D||!Mi)return;
  K.Age=0;K.Life=Life;K.Alpha=Alpha;
  D->DecalSize=FVector(6.,SizeCm,SizeCm);
  D->SetWorldLocationAndRotation(PosCm+Normal*1.,FRotationMatrix::MakeFromX(-Normal).Rotator());
  Mi->SetVectorParameterValue(TEXT("Color"),Colour);
  Mi->SetScalarParameterValue(TEXT("Opacity"),float(Alpha));
  D->SetVisibility(true);
  D->MarkRenderStateDirty();
 }

 void Place(FS47DevWaveSprite& S,const FVector& EyeCm,bool bSoft) const
 {
  UStaticMeshComponent* C=S.Comp.Get();
  UMaterialInstanceDynamic* Mi=S.Mid.Get();
  if(!C||!Mi)return;
  const double T=FMath::Clamp(S.Age/FMath::Max(S.Life,1e-3),0.,1.);
  const double Wide=FMath::Lerp(S.Size0,S.Size1,T),Long=S.bAlong?FMath::Lerp(S.Len0,S.Len1,T):Wide;
  FVector ToEye=(EyeCm-S.Pos).GetSafeNormal();
  if(ToEye.IsNearlyZero())ToEye=FVector::UpVector;
  FMatrix R;
  if(S.bAlong)
  {
   const FVector Ax=(S.bVel&&!S.Vel.IsNearlyZero()?S.Vel:S.Axis).GetSafeNormal();
   R=FMath::Abs(FVector::DotProduct(Ax,ToEye))>.999?FRotationMatrix::MakeFromX(Ax):FRotationMatrix::MakeFromXZ(Ax,ToEye);
  }
  else R=FMath::Abs(ToEye.Z)>.999?FRotationMatrix::MakeFromZ(ToEye):FRotationMatrix::MakeFromZX(ToEye,FVector::UpVector);
  // The engine's plane is a metre square across its own x and y, its face along z.
  C->SetWorldLocationAndRotation(S.Pos,R.Rotator());
  C->SetWorldScale3D(FVector(Long/100.,Wide/100.,1.));
  const float Fade=float(FMath::Pow(1.-T,S.Pow));
  Mi->SetVectorParameterValue(TEXT("Color"),S.Tint);
  if(bSoft)
  {
   Mi->SetScalarParameterValue(TEXT("Brightness"),float(S.Bright));
   Mi->SetScalarParameterValue(TEXT("Opacity"),float(S.Alpha)*Fade);
  }
  else Mi->SetScalarParameterValue(TEXT("Brightness"),float(S.Bright)*Fade);
  if(!C->IsVisible())C->SetVisibility(true);
 }

 void Step(double Dt)
 {
  const AS47Walker* W=Walker.Get();
  if(!W)return;
  const FVector EyeCm=W->EyeUE();
  // (a picture asked for this frame is written in the next: what it is of stands still until then)
  const double Tick=Freeze>0?0.:Dt;
  if(Freeze>0)--Freeze;
  for(int32 Pass=0;Pass<2;++Pass)for(FS47DevWaveSprite& S:(Pass==0?Glows:Softs))
  {
   if(!S.bLive)continue;
   if(S.Delay>0){S.Delay-=Tick;continue;}
   S.Age+=Tick;
   if(S.Age>=S.Life)
   {
    S.bLive=false;
    if(UStaticMeshComponent* C=S.Comp.Get())C->SetVisibility(false);
    continue;
   }
   S.Vel.Z-=S.Gravity*Tick;
   S.Vel*=FMath::Exp(-S.Drag*Tick);
   S.Pos+=S.Vel*Tick;
   Place(S,EyeCm,Pass==1);
  }
  for(FS47DevWaveBit& B:Bits)
  {
   if(!B.bLive)continue;
   UStaticMeshComponent* C=B.Comp.Get();
   if(!C){B.bLive=false;continue;}
   B.Age+=Tick;
   if(B.Age>=B.Life){B.bLive=false;C->SetVisibility(false);continue;}
   if(B.Hits<99)
   {
    B.Vel.Z-=980.*Tick;
    B.Pos+=B.Vel*Tick;
    B.Rot+=B.Spin*Tick;
    if(B.Pos.Z<=B.Ground&&B.Vel.Z<0)
    {
     // down: it rings, loses most of its way, and after a bounce or two lies where it is
     B.Pos.Z=B.Ground;
     const double Hard=-B.Vel.Z;
     if(!B.Tink.IsEmpty()&&B.Hits<2&&Hard>60.)Snd(*B.Tink,FS47Sound{.Gain=B.TinkGain/float(1+B.Hits),.Rate=float(S47DevWavesLocal::Rnd(.93,1.08)),.At=S47Three::ToThree(B.Pos)});
     ++B.Hits;
     if(Hard*B.Bounce<45.||B.Hits>3)
     {
      B.Hits=99;B.Vel=FVector::ZeroVector;B.Spin=FRotator::ZeroRotator;
      B.Rot.Pitch=0;B.Rot.Roll=FMath::RoundToDouble(B.Rot.Roll/90.)*90.;
     }
     else{B.Vel.Z=Hard*B.Bounce;B.Vel.X*=.55;B.Vel.Y*=.55;B.Spin=B.Spin*.5;}
    }
   }
   C->SetWorldLocationAndRotation(B.Pos,B.Rot);
   C->SetWorldScale3D(B.Size*FMath::Clamp((B.Life-B.Age)/.35,0.,1.));
  }
  for(FS47DevWaveMark& K:Marks)
  {
   if(K.Life<=0)continue;
   UDecalComponent* D=K.Comp.Get();
   UMaterialInstanceDynamic* Mi=K.Mid.Get();
   K.Age+=Tick;
   if(!D||!Mi||K.Age>=K.Life){K.Life=0;if(D)D->SetVisibility(false);continue;}
   const double Left=K.Life-K.Age;
   if(Left<2.)Mi->SetScalarParameterValue(TEXT("Opacity"),float(K.Alpha*Left/2.));
  }
 }

 // A picture of this frame for the run's folder, once each: true when asked for.
 bool Snap(const TCHAR* Name,double Now)
 {
  if(ShotsDir.IsEmpty()||Snapped.Contains(Name)||Now-LastSnap<.45||FScreenshotRequest::IsScreenshotRequested())return false;
  Snapped.Add(Name);
  LastSnap=Now;
  Freeze=2;
  const FString File=ShotsDir/(Tag.IsEmpty()?FString(Name):Tag+TEXT("_")+Name)+TEXT(".png");
  FScreenshotRequest::RequestScreenshot(File,true,false);
  UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: picture %s"),*File);
  return true;
 }
};

FS47DevWaves::FS47DevWaves(US47Main* InM):M(InM){}

FS47DevWaves::~FS47DevWaves()
{
 // The game itself is going: nothing is given back, only let go.
 if(S47DevWavesLocal::Live()==this)
 {
  S47DevWavesLocal::Live()=nullptr;
  FS47ScriptPlayer::Hold(S47DevWavesLocal::HoldName,false);
 }
 Fx.Reset();
 if(Hud&&!IsEngineExitRequested()&&GEngine&&GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(Hud.ToSharedRef());
 Hud.Reset();
}

bool FS47DevWaves::On(const US47Main* InM)
{
 const FS47DevWaves* W=S47DevWavesLocal::Live();
 return InM&&W&&W->M==InM&&W->Running();
}

bool FS47DevWaves::AutoWanted(){return S47DevWavesLocal::AutoFlag();}

void FS47DevWaves::Say(const FString& Text,float Seconds) const
{
 if(M&&M->UI)M->UI->Toast(Text,Seconds);
 UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: %s"),*Text);
}

void FS47DevWaves::AddEntries(FS47DevEvents& E)
{
 E.AddFreeform(S47DevWavesLocal::MenuGroup,TEXT("Wave shooter: start"),
  TEXT("DEV TOY, not part of the game. Puts you in the SARO yard with the SARO security carbine; aliens come in waves. Left mouse fires, right mouse aims down the sights, R reloads, Shift runs. A supply crate is marked between waves. The night waits, nothing is saved, and you are put back where you stood when it stops."),
  [this]{Start();});
 E.AddFreeform(S47DevWavesLocal::MenuGroup,TEXT("Wave shooter: stop"),
  TEXT("DEV TOY. Ends the shooter at once: aliens and gun gone, you back where you stood, the night running again."),
  [this]
  {
   if(Running())Stop(TEXT("stopped from F10"));
   else Say(TEXT("Wave shooter is not running."),3);
  });
}

// Why it may not begin now (empty: it may). As the director's camera asks: nothing of the night is cut off half way.
FString FS47DevWaves::WhyNot() const
{
 if(S47DevWavesLocal::NobodyPlays())return TEXT("a run nobody plays");
 if(!M||M->Mode!=US47Main::EMode::Play||M->bPausedByMenu)return TEXT("not in play");
 const FS47FreeformSession* F=M->Freeform.Get();
 if(!M->IsFreeform()||!F||!F->Active())return TEXT("Freeform only");
 const AS47Walker* W=M->Player.Get();
 if(!W||!W->IsActive()||!M->Web||!M->Web->IsLoaded()||!M->World)return TEXT("the player is not in your hands");
 if(M->World->bDriving)return TEXT("at the wheel");
 if(M->Web->Area()!=TEXT("saro"))return TEXT("only at SARO (F10, Time and place, Go: SARO yard)");
 if(M->ModalOpen())return TEXT("a panel is open");
 if(W->bCameraHeld)return TEXT("the camera is held by something else");
 if(M->FieldCamera&&M->FieldCamera->bRaised)return TEXT("the field camera is raised");
 if((F->Handling&&F->Handling->Holding())||(F->Field&&F->Field->ParcelStage()==2))return TEXT("your hands are full: put it down first");
 if((F->Taken&&F->Taken->Busy())||(M->Away&&M->Away->Abduction&&M->Away->Abduction->Busy()))return TEXT("being taken");
 if(F->Attention&&F->Attention->Visiting())return TEXT("a visit at SARO is under way");
 if(F->DoorLoop&&F->DoorLoop->Active())return TEXT("the door loop is under way");
 if(F->FarmSite&&F->FarmSite->VisitWantsGround())return TEXT("a visit is out for the player");
 if(F->IndoorDread&&F->IndoorDread->DoorActive())return TEXT("a door visit is under way");
 if(FS47WeekCase::Holds(M))return TEXT("the week's case is on the ground");
 if(const FS47ScriptPlayer* P=FS47ScriptPlayer::Of(M);P&&P->Running())return TEXT("a script is playing");
 FString Big;
 if(FS47ScriptPlayer::BigEvent(M,Big))return TEXT("a big event holds the night: ")+Big;
 if(M->DevEvents&&M->DevEvents->FreeformDev()&&M->DevEvents->FreeformDev()->Natural())return TEXT("Natural rhythm is on: turn it off first");
 if(M->OneAtATime())return TEXT("something else holds the night");
 if(F->Held())return TEXT("the night is held");
 return FString();
}

// Why it must end now (empty: it goes on).
FString FS47DevWaves::WhyOff() const
{
 if(!M||M->Mode!=US47Main::EMode::Play)return TEXT("out of play");
 if(!M->IsFreeform()||!M->Freeform||!M->Freeform->Active())return TEXT("the session stopped");
 const AS47Walker* W=Shooter.Get();
 if(!W||W!=M->Player.Get()||!M->Web||!M->Web->IsLoaded())return TEXT("the player went");
 if(M->Web->Area()!=StoodArea)return TEXT("left SARO");
 if(M->World&&M->World->bDriving)return TEXT("at the wheel");
 if((M->Freeform->Taken&&M->Freeform->Taken->Busy())||(M->Away&&M->Away->Abduction&&M->Away->Abduction->Busy()))return TEXT("taken");
 return FString();
}

S47DevWaves::FStand FS47DevWaves::ReadStand() const
{
 S47DevWaves::FStand S;
 if(const AS47Walker* W=M?M->Player.Get():nullptr)
 {
  S.Pos=W->Pos;S.FloorY=W->FloorY;S.Yaw=W->Yaw;S.Pitch=W->Pitch;S.bCrouch=W->bCrouch;
 }
 return S;
}

void FS47DevWaves::WriteStand(const S47DevWaves::FStand& S) const
{
 AS47Walker* W=Shooter.Get();
 if(!W)return;
 // (Place reads the floor from near the floor it has: the floor it stood on first)
 W->FloorY=S.FloorY;
 W->Place(S.Pos.X,S.Pos.Z,S.Yaw);
 W->Pitch=S.Pitch;
 if(S.bCrouch)W->SetCrouch(true);
}

void FS47DevWaves::Start()
{
 using namespace S47DevWaves;
 if(Running()){Say(TEXT("Wave shooter is already running. F10, Wave shooter: stop ends it."),3);return;}
 const FString Why=WhyNot();
 if(!Why.IsEmpty()){Say(TEXT("Wave shooter not started: ")+Why+TEXT("."));return;}
 // Whatever another F10 line left out goes first (its bodies, its walks).
 if(M->DevEvents&&M->DevEvents->FreeformDev())M->DevEvents->FreeformDev()->ClearRunning();
 AS47Walker* W=M->Player.Get();
 Shooter=W;
 Stood=ReadStand();StoodArea=M->Web->Area();StoodClock=M->Freeform->Clock();
 // Use is put away while it runs: nothing is picked up, opened or driven, so hands, pockets and doors are as they were after.
 StoodUse=W->OnUse;
 W->OnUse=[](const FString&){};
 W->Place(S47DevWavesLocal::YardX,S47DevWavesLocal::YardZ,UE_DOUBLE_PI);
 FS47ScriptPlayer::Hold(S47DevWavesLocal::HoldName,true);
 S47DevWavesLocal::Live()=this;
 Gun=FGun();
 Spread=FSpread();
 Round.Begin();
 SpawnSerial=0;SpawnWait=0;SpawnDry=0;LastFlash=-10;LastShot=-10;TailOwed=-1;HeartWait=0;Age=0;FightAge=0;HurtSaid=-10;ShotSerial=0;LastBeat=-1;bTriggerWas=false;
 for(bool& bNo:bNoKind)bNo=false;
 BuildGun();
 ShowHud(true);
 if(Fx)Fx->Snd(TEXT("wave_sting"),FS47Sound{.Gain=.7f},TEXT("switch"));
 WriteHud(0);
 UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter on, the player from %.1f %.1f (%s) at clock %.0f, the gun %s"),Stood.Pos.X,Stood.Pos.Z,*StoodArea,StoodClock,Fx&&Fx->bModel?TEXT("the carbine"):TEXT("the stand-in boxes (no /Game/Signal47/Dev/Waves in this build)"));
 if(M->UI&&!S47DevWavesLocal::AutoFlag())M->UI->Toast(TEXT("DEV TOY: wave shooter. Left mouse fires, right mouse aims, R reloads, Shift runs. F10, Wave shooter: stop ends it. The night waits; nothing is saved."),7);
}

void FS47DevWaves::PutAway(FS47DevWaveBody& B) const
{
 if(UBoxComponent* Root=B.Body.RootComp.Get())
 {
  Root->SetRelativeScale3D(B.Scale0);
  Root->SetVisibility(false,true);
 }
 B.bOut=false;B.Fall=-1;B.Arrive=0;
}

void FS47DevWaves::Restart()
{
 using namespace S47DevWaves;
 AS47Walker* W=Shooter.Get();
 if(!W){Stop(TEXT("the player went"));return;}
 UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: a new round (the last one: wave %d, %d down, score %d, best streak %d)"),Round.Wave,Round.Kills,Round.Score,Round.BestStreak);
 for(const TSharedPtr<FS47DevWaveBody>& B:Bodies)if(B&&B->Body.RootComp.IsValid())PutAway(*B);
 W->Place(S47DevWavesLocal::YardX,S47DevWavesLocal::YardZ,UE_DOUBLE_PI);
 Gun=FGun();
 Spread=FSpread();
 Round.Begin();
 SpawnSerial=0;SpawnWait=0;SpawnDry=0;FightAge=0;LastBeat=-1;TailOwed=-1;
 if(Fx)
 {
  Fx->bCrateOut=false;Fx->Climb=Fx->ClimbYaw=0;Fx->bLockedBack=false;Fx->bPrevValid=false;
  if(UStaticMeshComponent* C=Fx->CrateComp.Get())C->SetVisibility(false);
  if(USpotLightComponent* L=Fx->CrateLamp.Get())L->SetVisibility(false);
  Fx->Snd(TEXT("wave_sting"),FS47Sound{.Gain=.7f},TEXT("switch"));
 }
}

void FS47DevWaves::Stop(const TCHAR* Why)
{
 if(!Running())return;
 const int32 Reached=Round.Wave,Points=Round.Score,Down=Round.Kills;
 Round=S47DevWaves::FRound();
 if(S47DevWavesLocal::Live()==this)S47DevWavesLocal::Live()=nullptr;
 FS47ScriptPlayer::Hold(S47DevWavesLocal::HoldName,false);
 for(const TSharedPtr<FS47DevWaveBody>& B:Bodies)if(B&&B->Body.RootComp.IsValid())PutAway(*B);
 Bodies.RemoveAll([](const TSharedPtr<FS47DevWaveBody>& B){return !B||!B->Body.RootComp.IsValid();});
 DropGun();
 ShowHud(false);
 bool bBack=false;
 if(AS47Walker* W=Shooter.Get())
 {
  W->OnUse=StoodUse;
  // Only in the place it was taken from: a point of one area means nothing in another.
  if(M&&M->Web&&M->Web->IsLoaded()&&M->Web->Area()==StoodArea){WriteStand(Stood);bBack=true;}
 }
 StoodUse=nullptr;Shooter.Reset();
 const double ClockNow=M&&M->Freeform?M->Freeform->Clock():StoodClock;
 if(FMath::Abs(ClockNow-StoodClock)>.5){UE_LOG(LogS47DevWaves,Warning,TEXT("S47: dev wave shooter: the clock moved %.1f s while it ran (it should stand)"),ClockNow-StoodClock);}
 UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter off (%s): wave %d, %d down, score %d, the player %s"),Why?Why:TEXT("-"),Reached,Down,Points,bBack?TEXT("put back"):TEXT("left where they are"));
 if(M&&M->UI&&M->Mode==US47Main::EMode::Play)
  M->UI->Toast(FString::Printf(TEXT("Wave shooter off (%s). Wave reached: %d, score %d.%s"),Why?Why:TEXT("-"),Reached,Points,bBack?TEXT(" You are back where you stood."):TEXT("")),5);
}

// ---------- the aliens ----------

void FS47DevWaves::Show(FS47DevWaveBody& B,bool bShow,double Dt) const
{
 using namespace S47DevWaves;
 if(!B.Body.bBuilt||!B.Body.RootComp.IsValid())return;
 const AS47Walker* W=Shooter.Get();
 const double Face=W?FMath::Atan2(W->Pos.Z-B.At.Z,W->Pos.X-B.At.X):B.Yaw;
 const bool bMoving=B.Speed>.05;
 if(B.Kind==EKind::Crab){B.Body.ApplyCrab(bShow,B.At,B.Yaw,Face,B.Clock,1.,true);return;}
 if(B.Kind==EKind::Small)
 {
  // As the F10 "Eye glint" stands a small Roswell: its root placed, then its own motion.
  UBoxComponent* Root=B.Body.RootComp.Get();
  Root->SetVisibility(bShow,true);
  if(!bShow)return;
  Root->SetWorldLocationAndRotation(S47Three::ToUE(B.At),FRotator(0,FMath::RadiansToDegrees(B.Yaw),0));
  FS47RoswellPose P;
  P.Mode=B.Speed>1.6?ES47RoswellMotion::Run:bMoving?ES47RoswellMotion::Walk:ES47RoswellMotion::Idle;
  P.SpeedMps=B.Speed;P.DistanceTravelled=B.Travelled;
  B.Motion.Apply(B.Body.Creature,P,FMath::Min(Dt,.1));
  return;
 }
 // The Pale One and the Tall One, as F10's gait lines walk them: the chase when moving, listening when it stands.
 B.Body.Apply(bShow,B.At,B.Yaw,Face,bMoving?1.4:0.,B.Clock*3,B.Clock,bMoving?FS47PaleBody::EPose::Chase:FS47PaleBody::EPose::Listen);
}

FS47DevWaveBody* FS47DevWaves::Take(S47DevWaves::EKind Want)
{
 using namespace S47DevWaves;
 if(!M||!M->Web||!M->Web->IsLoaded())return nullptr;
 // A body whose parts went with their world is forgotten.
 Bodies.RemoveAll([](const TSharedPtr<FS47DevWaveBody>& B){return !B||(!B->bOut&&!B->Body.RootComp.IsValid());});
 if(bNoKind[int32(Want)])Want=EKind::Pale;
 for(const TSharedPtr<FS47DevWaveBody>& B:Bodies)if(!B->bOut&&B->Kind==Want)return B.Get();
 if(Bodies.Num()<S47DevWavesLocal::MostBodies)
 {
  TSharedPtr<FS47DevWaveBody> N=MakeShared<FS47DevWaveBody>();
  N->Kind=Want;
  N->Body.CreatureAsset=Want==EKind::Small?S47KindBody::Roswell:Want==EKind::Tall?S47KindBody::Tall:Want==EKind::Crab?S47KindBody::Crab:S47KindBody::Pale;
  if(Want==EKind::Tall)N->Body.Style=ES47PaleStyle::Tall;
  if(N->Body.Build(M->Web.Get(),TEXT("#root"),*FString::Printf(TEXT("DevWaves%d_"),++BodySerial)))
  {
   if(Want!=EKind::Pale&&!N->Body.Creature.Ready())
   {
    // No skinned model for this kind in this build: it comes as the pale stand-in, and so do the rest of its kind.
    bNoKind[int32(Want)]=true;N->Kind=EKind::Pale;
    UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: no skinned body for the %s here; the pale body stands in"),S47DevWavesLocal::KindName(Want));
   }
   if(const UBoxComponent* Root=N->Body.RootComp.Get())N->Scale0=Root->GetRelativeScale3D();
   Bodies.Add(N);
   return N.Get();
  }
  UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: a %s body could not be built"),S47DevWavesLocal::KindName(Want));
  if(Want!=EKind::Pale)bNoKind[int32(Want)]=true;
 }
 // None of that kind free: any body put away will do.
 for(const TSharedPtr<FS47DevWaveBody>& B:Bodies)if(!B->bOut)return B.Get();
 return nullptr;
}

// One more out of sight: on the ground round the player, behind something or behind their back when there is such a place. Its
// light and its sound stand there a moment first (the aliens' own cue), so the player knows which way to turn.
bool FS47DevWaves::Spawn()
{
 using namespace S47DevWaves;
 using namespace S47DevWavesLocal;
 AS47Walker* W=Shooter.Get();
 UWorld* World=M&&M->Web?M->Web->GetWorld():nullptr;
 if(!W||!World)return false;
 const FVector Feet(W->Pos.X,W->FloorY,W->Pos.Z);
 const FVector Eye=S47Three::ToThree(W->EyeUE());
 const FVector2D Ahead(-FMath::Sin(W->Yaw),-FMath::Cos(W->Yaw));
 TOptional<FVector> Best;
 double BestScore=-1;
 const double Turn=FMath::FRand()*2*PI;
 for(int32 I=0;I<12;++I)for(const double Metres:{34.,26.,42.})
 {
  const double Ang=Turn+I*(2*PI/12);
  const FVector2D C(Feet.X+FMath::Cos(Ang)*Metres,Feet.Z+FMath::Sin(Ang)*Metres);
  FCollisionQueryParams Under(SCENE_QUERY_STAT(S47DevWavesSpawnGround),true,W);
  FHitResult Ground;
  if(!World->LineTraceSingleByChannel(Ground,S47Three::ToUE(C.X,Feet.Y+10.,C.Y),S47Three::ToUE(C.X,Feet.Y-10.,C.Y),ECC_Visibility,Under))continue;
  const double G=S47Three::ToThree(Ground.ImpactPoint).Y;
  if(G-Feet.Y>2.5)continue; // a roof, not the ground
  FCollisionQueryParams Sight(SCENE_QUERY_STAT(S47DevWavesSpawnSight),true,W);
  FHitResult Block;
  const bool bHidden=World->LineTraceSingleByChannel(Block,S47Three::ToUE(Eye),S47Three::ToUE(C.X,G+1.,C.Y),ECC_Visibility,Sight);
  const bool bBehind=FVector2D::DotProduct(Ahead,(C-FVector2D(Feet.X,Feet.Z)).GetSafeNormal())<.2;
  const double Score=(bHidden?4.:0.)+(bBehind?2.:0.)+(W->Walkable(C.X,C.Y)?1.:0.)+FMath::FRand()*.9;
  if(Score>BestScore){BestScore=Score;Best=FVector(C.X,G,C.Y);}
 }
 if(!Best)return false;
 const EKind Want=KindOf(Round.Wave,SpawnSerial);
 FS47DevWaveBody* B=Take(Want);
 if(!B)return false;
 B->bOut=true;B->At=*Best;B->Yaw=FMath::Atan2(Feet.Z-B->At.Z,Feet.X-B->At.X);
 B->Clock=FMath::FRand()*3;B->Travelled=0;B->Speed=0;B->TouchWait=0;B->Stagger=0;B->StaggerRest=0;B->Fall=-1;B->Blocked=0;B->Steer=0;B->SteerLeft=0;B->MoteWait=0;B->bFizzed=false;
 B->Lean=FSpring();B->Twist=FSpring();
 B->Arrive=ArriveSeconds;
 B->Flank=FlankOf(SpawnSerial)*Rnd(.7,1.15);
 B->PaceK=1.+Rnd(-PaceSpread,PaceSpread);
 B->Hp=HealthOf(B->Kind);
 B->Motion.Reset();
 ++SpawnSerial;
 if(UBoxComponent* Root=B->Body.RootComp.Get()){Root->SetRelativeScale3D(B->Scale0);Root->SetVisibility(false,true);}
 // the cue: a cold light where it will stand, and the aliens' own sound from there
 const double Tall=HeightOf(B->Kind);
 if(Fx)
 {
  const FVector Mid=S47Three::ToUE(B->At+FVector(0,Tall*.5,0));
  const bool bCalm=M->Flow&&M->Flow->bCalmFlash;
  if(FS47DevWaveSprite* S=Fx->Spark(false,Mid,FVector::ZeroVector,FLinearColor(.45f,.8f,1.f),40.,Tall*130.,ArriveSeconds+.25,bCalm?.5:1.3))S->Pow=.6;
  if(FS47DevWaveSprite* S=Fx->Spark(false,S47Three::ToUE(B->At+FVector(0,.08,0)),FVector::ZeroVector,FLinearColor(.4f,.75f,1.f),60.,260.,ArriveSeconds+.5,bCalm?.3:.7))S->Pow=1.4;
 }
 if(M->Audio)M->Audio->Sfx(TEXT("alienRadio"),FS47Sound{.Gain=.55f,.Rate=float(B->Kind==EKind::Tall?.7:B->Kind==EKind::Small?1.35:1.),.At=B->At+FVector(0,Tall*.6,0),.Dur=.8f});
 return true;
}

bool FS47DevWaves::PathClear(const FS47DevWaveBody& B,double Heading) const
{
 using namespace S47DevWaves;
 const AS47Walker* W=Shooter.Get();
 UWorld* World=M&&M->Web?M->Web->GetWorld():nullptr;
 if(!World)return true;
 FCollisionQueryParams Probe(SCENE_QUERY_STAT(S47DevWavesProbe),true,W);
 for(const auto& Part:B.Body.All)if(const auto* Own=Part.Get())Probe.AddIgnoredComponent(Own);
 const FVector From=B.At+FVector(0,FMath::Min(.9,HeightOf(B.Kind)*.42),0);
 const FVector To=From+FVector(FMath::Cos(Heading),0,FMath::Sin(Heading))*(RadiusOf(B.Kind)+.6);
 FHitResult Hit;
 return !World->LineTraceSingleByChannel(Hit,S47Three::ToUE(From),S47Three::ToUE(To),ECC_Visibility,Probe);
}

// Simple chase on the ground: towards the player (some round the side), round what stands in the way, and through it after a
// while (a dev toy: none is left stuck behind a wall). At touch range it hurts. Struck, it gives with the round; shot down, it
// goes over with its weight, lies a moment, and thins away into its own light.
void FS47DevWaves::StepBody(FS47DevWaveBody& B,double Dt)
{
 using namespace S47DevWaves;
 using namespace S47DevWavesLocal;
 AS47Walker* W=Shooter.Get();
 UWorld* World=M&&M->Web?M->Web->GetWorld():nullptr;
 UBoxComponent* Root=B.Body.RootComp.Get();
 if(!Root||!W||!World)
 {
  if(B.Fall<0)Round.Lost();
  B.bOut=false;
  return;
 }
 B.Clock+=Dt;
 const bool bCalm=M->Flow&&M->Flow->bCalmFlash;
 const double Tall=HeightOf(B.Kind);
 if(B.Arrive>0)
 {
  // its light stands where it will; then it is there
  B.Arrive-=Dt;
  if(B.Arrive>0)return;
  B.Arrive=0;
  Show(B,true,0);
  if(Fx)for(int32 I=0;I<5;++I)
   if(FS47DevWaveSprite* S=Fx->Spark(false,S47Three::ToUE(B.At+FVector(Rnd(-.3,.3),Rnd(.2,Tall),Rnd(-.3,.3))),FVector(Rnd(-30,30),Rnd(-30,30),Rnd(20,90)),FLinearColor(.5f,.85f,1.f),5.,2.,Rnd(.4,.8),bCalm?.6:1.6))S->Pow=1.5;
  return;
 }
 if(B.Fall>=0)
 {
  B.Fall+=Dt;
  const double Down=FallSeconds+LieSeconds;
  if(B.Fall>=Down+GoneSeconds){PutAway(B);return;}
  // over with its weight: slow off its feet, fast onto the ground, one small give as it lands
  const double K=FMath::Min(1.,B.Fall/FallSeconds);
  double Tip=86.*K*K*K;
  if(B.Fall>FallSeconds){const double After=B.Fall-FallSeconds;Tip-=7.*FMath::Exp(-After*9.)*FMath::Sin(After*22.);}
  Root->SetWorldRotation(FRotator(Tip,FMath::RadiansToDegrees(B.Yaw),B.Twist.X));
  if(B.Fall>=Down)
  {
   // then it goes: thinner and thinner, and its light rises out of where it lay
   const double Gone=(B.Fall-Down)/GoneSeconds;
   Root->SetRelativeScale3D(B.Scale0*FMath::Max(.02,1.-Gone*Gone));
   if(Fx)
   {
    if(!B.bFizzed){B.bFizzed=true;Fx->Snd(TEXT("alien_dissolve"),FS47Sound{.Gain=.5f,.Rate=float(B.Kind==EKind::Tall?.8:1.1),.At=B.At+FVector(0,.3,0)});}
    B.MoteWait-=Dt;
    if(B.MoteWait<=0)
    {
     B.MoteWait=.04;
     // (it lies back from its feet, along its yaw turned about: the motes come from along that line)
     const double Along=Rnd(0.,Tall)*(1.-Gone*.5);
     const FVector From=B.At-FVector(FMath::Cos(B.Yaw),0,FMath::Sin(B.Yaw))*Along+FVector(Rnd(-.2,.2),Rnd(.05,.3),Rnd(-.2,.2));
     if(FS47DevWaveSprite* S=Fx->Spark(false,S47Three::ToUE(From),FVector(Rnd(-25,25),Rnd(-25,25),Rnd(50,130)),FLinearColor(.55f,.9f,1.f),Rnd(3.,6.),1.5,Rnd(.7,1.3),bCalm?.5:1.5)){S->Pow=1.3;S->Drag=.6;}
     if(FS47DevWaveSprite* S=Fx->Spark(true,S47Three::ToUE(From),FVector(Rnd(-15,15),Rnd(-15,15),Rnd(20,60)),FLinearColor(.02f,.025f,.03f),18.,46.,Rnd(.5,.9),.02,.3)){S->Pow=1.2;S->Drag=1.;}
    }
   }
  }
  return;
 }
 const FVector Feet(W->Pos.X,W->FloorY,W->Pos.Z);
 const FVector2D To(Feet.X-B.At.X,Feet.Z-B.At.Z);
 const double Dist=To.Size(),Face=FMath::Atan2(To.Y,To.X);
 B.TouchWait=FMath::Max(0.,B.TouchWait-Dt);
 B.Stagger=FMath::Max(0.,B.Stagger-Dt);
 B.StaggerRest=FMath::Max(0.,B.StaggerRest-Dt);
 B.Lean.Step(Dt,4.2,.5);
 B.Twist.Step(Dt,4.8,.5);
 const bool bFight=Round.Phase==EPhase::Fight;
 // round the side while it is far, straight in when it is near
 const double Want=Face+B.Flank*FMath::Clamp((Dist-5.)/14.,0.,1.);
 double Pace=0,Head=Want;
 if(bFight&&B.Stagger<=0&&Dist>TouchReach*.8)
 {
  B.SteerLeft-=Dt;
  if(B.SteerLeft<=0)B.Steer=0;
  bool bClear=false;
  for(const double Try:{B.Steer,0.,.75,-.75,1.5,-1.5})
  {
   if(!PathClear(B,Want+Try))continue;
   if(Try!=0&&Try!=B.Steer){B.Steer=Try;B.SteerLeft=.6;}
   Head=Want+Try;bClear=true;
   break;
  }
  if(bClear)B.Blocked=0;else B.Blocked+=Dt;
  if(bClear||B.Blocked>1.2)Pace=WaveSpeed(Round.Wave)*PaceOf(B.Kind)*B.PaceK*(Dist<6.&&B.Kind!=EKind::Tall?LungeTimes:1.);
 }
 B.Yaw+=FMath::UnwindRadians(Head-B.Yaw)*FMath::Min(1.,Dt*7.);
 const double Moved=FMath::Min(Pace*Dt,FMath::Max(0.,Dist-TouchReach*.7));
 B.At.X+=FMath::Cos(Head)*Moved;B.At.Z+=FMath::Sin(Head)*Moved;
 if(B.Stagger>0&&B.Kind==EKind::Tall)
 {
  // the Tall One reels: a step and a half back off the line of the fire
  const double Back=1.5*FMath::Min(1.,B.Stagger/StaggerSeconds)*Dt;
  B.At.X-=FMath::Cos(Face)*Back;B.At.Z-=FMath::Sin(Face)*Back;
 }
 B.Travelled+=Moved;B.Speed=Dt>0?Moved/Dt:0.;
 // The ground under it, not its own stand-in parts.
 {
  FCollisionQueryParams Under(SCENE_QUERY_STAT(S47DevWavesFloor),true,W);
  for(const auto& Part:B.Body.All)if(const auto* Own=Part.Get())Under.AddIgnoredComponent(Own);
  FHitResult Floor;
  if(World->LineTraceSingleByChannel(Floor,S47Three::ToUE(B.At.X,B.At.Y+1.,B.At.Z),S47Three::ToUE(B.At.X,B.At.Y-3.,B.At.Z),ECC_Visibility,Under))
   B.At.Y=FMath::FInterpTo(B.At.Y,S47Three::ToThree(Floor.ImpactPoint).Y,Dt,12.);
 }
 if(bFight&&B.Stagger<=0&&B.TouchWait<=0&&Dist<=TouchReach&&FMath::Abs(B.At.Y-Feet.Y)<2.)
 {
  B.TouchWait=TouchGap;
  Round.Touched();
  if(W->bHeadMotion)W->Shake=FMath::Max(W->Shake,.75);
  if(Fx)
  {
   Fx->Snd(TEXT("player_hurt"),FS47Sound{.Gain=.85f,.Rate=float(Rnd(.92,1.06))},TEXT("thudSoft"));
   Fx->ViewPitch.Kick(-1.6,7.);
   Fx->ViewYaw.Kick(Rnd(-1.4,1.4),7.);
  }
  if(M->Audio&&Age-HurtSaid>1.6){HurtSaid=Age;M->Audio->Gasp();}
  if(HudText)
  {
   // where from, for the mark on the screen: clockwise from straight ahead
   const FVector2D Ahead(-FMath::Sin(W->Yaw),-FMath::Cos(W->Yaw)),Right(FMath::Cos(W->Yaw),-FMath::Sin(W->Yaw)),From(-To.X,-To.Y);
   int32 Slot=0;
   for(int32 I=1;I<3;++I)if(HudText->HurtAt[I]<HudText->HurtAt[Slot])Slot=I;
   HudText->HurtAt[Slot]=1;
   HudText->HurtDeg[Slot]=float(FMath::RadiansToDegrees(FMath::Atan2(FVector2D::DotProduct(From,Right),FVector2D::DotProduct(From,Ahead))));
   HudText->HurtK=1;
  }
  if(Round.Phase==EPhase::Dead)
  {
   UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: the player went down in wave %d, score %d"),Round.Wave,Round.Score);
   if(M->Audio)M->Audio->Sfx(TEXT("alienRadio"),FS47Sound{.Gain=.5f,.Rate=.7f,.Dur=2.5f});
  }
 }
 Show(B,true,Dt);
 // struck: back and round with the round, over its own walk
 if(FMath::Abs(B.Lean.X)>.05||FMath::Abs(B.Twist.X)>.05)Root->AddLocalRotation(FRotator(B.Lean.X,B.Twist.X,0));
}

// ---------- the gun ----------

void FS47DevWaves::BuildGun()
{
 using namespace S47DevWavesLocal;
 DropGun();
 AS47Walker* W=Shooter.Get();
 AS47Web* Web=M?M->Web.Get():nullptr;
 if(!W||!Web||!W->GetRootComponent()||!Web->GetRootComponent())return;
 Fx=MakeShared<FS47DevWaveFx>();
 FS47DevWaveFx& F=*Fx;
 F.M=M;F.Walker=W;F.Web=Web;
 F.Cam=W->FindComponentByClass<UCameraComponent>();
 const auto Asset=[](const TCHAR* Name){return LoadObject<UStaticMesh>(nullptr,*(FString(AssetRoot)+Name+TEXT(".")+Name));};
 UStaticMesh* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 UStaticMesh* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
 UMaterialInterface* Lit=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 UMaterialInterface* Glow=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_WebGlow.M_WebGlow"));
 UMaterialInterface* Soft=LoadObject<UMaterialInterface>(nullptr,*(FString(AssetRoot)+TEXT("Materials/M_DevWaves_Soft.M_DevWaves_Soft")));
 UMaterialInterface* MarkBase=LoadObject<UMaterialInterface>(nullptr,*(FString(AssetRoot)+TEXT("Materials/M_DevWaves_Decal.M_DevWaves_Decal")));
 UStaticMesh* GunMesh=Asset(TEXT("SM_DevWaves_Carbine"));
 UStaticMesh* MagMesh=Asset(TEXT("SM_DevWaves_Mag"));
 UStaticMesh* BoltMesh=Asset(TEXT("SM_DevWaves_Bolt"));
 UStaticMesh* ChargeMesh=Asset(TEXT("SM_DevWaves_Charge"));
 UStaticMesh* CasingMesh=Asset(TEXT("SM_DevWaves_Casing"));
 UStaticMesh* CrateMesh=Asset(TEXT("SM_DevWaves_Crate"));
 F.CubeMesh=Cube;F.CasingMesh=CasingMesh;F.MagMesh=MagMesh;F.LitMat=Lit;F.MarkMat=MarkBase;
 F.bModel=GunMesh&&MagMesh&&BoltMesh&&ChargeMesh;
 // The view itself (the walker's camera): the gun hangs under it and turns with the look, the kick and the shake.
 USceneComponent* Eye=F.Cam.IsValid()?static_cast<USceneComponent*>(F.Cam.Get()):W->GetRootComponent();
 USceneComponent* Holder=NewObject<USceneComponent>(W);
 Holder->SetupAttachment(Eye);Holder->SetMobility(EComponentMobility::Movable);Holder->RegisterComponent();
 F.Holder=Holder;F.Parts.Add(Holder);
 // The gun is drawn with its own field of view (the engine's first-person primitives), so it keeps its shape whatever the view does.
 F.bFpFov=F.Cam.IsValid()&&!FParse::Param(FCommandLine::Get(),TEXT("S47DevWavesNoFp"));
 if(F.bFpFov){F.Cam->SetEnableFirstPersonFieldOfView(true);F.Cam->SetFirstPersonFieldOfView(74.f);}
 const bool bFp=F.bFpFov;
 const auto Part=[&F,W,Holder,bFp](UStaticMesh* Mesh,const FVector& AtCm,const FVector& SizeM)
 {
  UStaticMeshComponent* C=NewObject<UStaticMeshComponent>(W);
  C->SetupAttachment(Holder);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);
  C->bVisibleInReflectionCaptures=false;C->bVisibleInRealTimeSkyCaptures=false;C->bAffectDistanceFieldLighting=false;
  C->SetLightingChannels(true,true,false); // (the second channel: the small light that falls on the gun alone)
  if(bFp)C->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
  C->SetRelativeLocation(AtCm);C->SetRelativeScale3D(SizeM);
  C->RegisterComponent();
  F.Parts.Add(C);
  return C;
 };
 if(F.bModel)
 {
  Part(GunMesh,FVector::ZeroVector,FVector::OneVector);
  F.MagComp=Part(MagMesh,FVector::ZeroVector,FVector::OneVector);
  F.BoltComp=Part(BoltMesh,FVector::ZeroVector,FVector::OneVector);
  F.ChargeComp=Part(ChargeMesh,FVector::ZeroVector,FVector::OneVector);
 }
 else if(Cube)
 {
  // No model in this build: boxes stand in (a box is one metre: the scale is the size in metres).
  const auto Tint=[Lit,W](const FLinearColor& Shade,float Rough)
  {
   UMaterialInstanceDynamic* Mat=Lit?UMaterialInstanceDynamic::Create(Lit,W):nullptr;
   if(Mat){Mat->SetVectorParameterValue(TEXT("Color"),Shade);Mat->SetScalarParameterValue(TEXT("Roughness"),Rough);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);}
   return Mat;
  };
  UMaterialInstanceDynamic* Steel=Tint(FLinearColor(.035f,.038f,.045f),.42f);
  UMaterialInstanceDynamic* Grip=Tint(FLinearColor(.07f,.045f,.03f),.8f);
  const auto Box=[&Part,Cube](const FVector& AtCm,const FVector& SizeM,UMaterialInstanceDynamic* Mat){UStaticMeshComponent* C=Part(Cube,AtCm,SizeM);if(Mat)C->SetMaterial(0,Mat);return C;};
  Box(FVector(7,0,-5),FVector(.30,.05,.085),Steel);
  Box(FVector(42,0,-5.5),FVector(.48,.024,.024),Steel);
  Box(FVector(-3,0,-13),FVector(.045,.04,.12),Grip);
  F.MagComp=Box(FVector(12,0,-15),FVector(.035,.03,.13),Steel);
  Box(FVector(-19,0,-7),FVector(.22,.035,.06),Grip);
 }
 // A steady lamp under the barrel, so that what comes can be seen at night. Steady: it never flashes.
 {
  USpotLightComponent* L=NewObject<USpotLightComponent>(W);
  L->SetupAttachment(Holder);L->SetMobility(EComponentMobility::Movable);
  L->SetIntensityUnits(ELightUnits::Candelas);L->SetIntensity(480.f);
  L->SetLightColor(S47Three::Hex(0xffe6c0));
  L->SetAttenuationRadius(48.f*100.f);L->SetOuterConeAngle(22.f);L->SetInnerConeAngle(9.f);
  L->SetCastShadows(true);
  L->SetRelativeLocation(LampAt+FVector(3,0,0));
  L->RegisterComponent();
  F.Lamp=L;F.Parts.Add(L);
 }
 // The muzzle's light: local (5 m), on for a twentieth of a second, never more than three a second, never with Calm flashes.
 {
  UPointLightComponent* L=NewObject<UPointLightComponent>(W);
  L->SetupAttachment(Holder);L->SetMobility(EComponentMobility::Movable);
  L->SetIntensityUnits(ELightUnits::Candelas);L->SetIntensity(46.f);
  L->SetLightColor(S47Three::Hex(0xffc070));
  L->SetAttenuationRadius(500.f);L->SetCastShadows(false);
  L->SetRelativeLocation(MuzzleAt+FVector(14,0,0));
  L->SetVisibility(false);
  L->RegisterComponent();
  F.MuzzleLight=L;F.Parts.Add(L);
 }
 // A small steady light on the gun alone (its own lighting channel), so its shape reads in the dark as it would in the hand.
 {
  UPointLightComponent* L=NewObject<UPointLightComponent>(W);
  L->SetupAttachment(Holder);L->SetMobility(EComponentMobility::Movable);
  L->SetIntensityUnits(ELightUnits::Candelas);L->SetIntensity(3.5f);
  L->SetSourceRadius(14.f);
  L->SetLightColor(S47Three::Hex(0xc8d8ff));
  L->SetAttenuationRadius(160.f);L->SetCastShadows(false);
  L->SetLightingChannels(false,true,false);
  L->SetRelativeLocation(FVector(18,-34,26));
  L->RegisterComponent();
  F.FillLight=L;F.Parts.Add(L);
 }
 USceneComponent* WorldRoot=Web->GetRootComponent();
 const auto Loose=[&F,Web,WorldRoot](UStaticMesh* Mesh)
 {
  UStaticMeshComponent* C=NewObject<UStaticMeshComponent>(Web);
  C->SetupAttachment(WorldRoot);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);
  C->SetCastShadow(false);C->bAffectDistanceFieldLighting=false;C->bVisibleInReflectionCaptures=false;C->bVisibleInRealTimeSkyCaptures=false;
  C->SetVisibility(false);
  C->RegisterComponent();
  F.Parts.Add(C);
  return C;
 };
 // The soft sprites: glows (added light: flash, tracer, sparks, the aliens' light) and puffs (smoke, dust, the dark burst).
 if(Plane)for(int32 Pass=0;Pass<2;++Pass)
 {
  UMaterialInterface* Base=Pass==0?Glow:Soft;
  if(!Base)continue;
  TArray<FS47DevWaveSprite>& Pool=Pass==0?F.Glows:F.Softs;
  for(int32 I=0;I<(Pass==0?72:28);++I)
  {
   FS47DevWaveSprite S;
   UStaticMeshComponent* C=Loose(Plane);
   UMaterialInstanceDynamic* Mi=UMaterialInstanceDynamic::Create(Base,Web);
   Mi->SetScalarParameterValue(TEXT("Brightness"),0.f);
   C->SetMaterial(0,Mi);
   C->SetTranslucentSortPriority(Pass==0?3:2);
   S.Comp=C;S.Mid=Mi;
   Pool.Add(S);
  }
 }
 // The small things that fall: casings and chips.
 if(Cube)for(int32 I=0;I<44;++I)
 {
  FS47DevWaveBit B;
  UStaticMeshComponent* C=Loose(Cube);
  UMaterialInstanceDynamic* Mi=Lit?UMaterialInstanceDynamic::Create(Lit,Web):nullptr;
  if(Mi){Mi->SetScalarParameterValue(TEXT("Roughness"),.85f);Mi->SetScalarParameterValue(TEXT("NativeLighting"),1);}
  B.Comp=C;B.Mid=Mi;
  F.Bits.Add(B);
 }
 // The marks the rounds leave.
 if(MarkBase)for(int32 I=0;I<16;++I)
 {
  FS47DevWaveMark K;
  UDecalComponent* D=NewObject<UDecalComponent>(Web);
  D->SetupAttachment(WorldRoot);D->SetMobility(EComponentMobility::Movable);
  UMaterialInstanceDynamic* Mi=UMaterialInstanceDynamic::Create(MarkBase,Web);
  D->SetDecalMaterial(Mi);
  D->DecalSize=FVector(6.,8.,8.);
  D->SetVisibility(false);
  D->RegisterComponent();
  F.Parts.Add(D);
  K.Comp=D;K.Mid=Mi;
  F.Marks.Add(K);
 }
 // The supply crate and the steady lamp over it.
 if(CrateMesh||Cube)
 {
  UStaticMeshComponent* C=Loose(CrateMesh?CrateMesh:Cube);
  if(!CrateMesh)C->SetWorldScale3D(FVector(.5,.34,.3));
  C->SetCastShadow(true);
  F.CrateComp=C;
  USpotLightComponent* L=NewObject<USpotLightComponent>(Web);
  L->SetupAttachment(WorldRoot);L->SetMobility(EComponentMobility::Movable);
  L->SetIntensityUnits(ELightUnits::Candelas);L->SetIntensity(900.f);
  L->SetLightColor(S47Three::Hex(0x9cffb4));
  L->SetAttenuationRadius(900.f);L->SetOuterConeAngle(34.f);L->SetInnerConeAngle(16.f);
  L->SetCastShadows(false);
  L->SetVisibility(false);
  L->RegisterComponent();
  F.CrateLamp=L;F.Parts.Add(L);
 }
 // What the view does with the kick (and the sights' closer view): a layer on the walker's own, over whatever layer was there.
 F.PrevLayer=W->ViewLayer;
 F.PrevMoveScale=W->MoveScale;
 {
  const TWeakPtr<FS47DevWaveFx> Weak=Fx;
  W->ViewLayer=[Weak](double LayerDt,double& LayerYaw,double& LayerPitch,float& LayerFov)
  {
   const TSharedPtr<FS47DevWaveFx> P=Weak.Pin();
   if(!P)return false;
   const double Yaw0=LayerYaw,Pitch0=LayerPitch;
   const float Fov0=LayerFov;
   if(P->PrevLayer&&!P->PrevLayer(LayerDt,LayerYaw,LayerPitch,LayerFov)){LayerYaw=Yaw0;LayerPitch=Pitch0;LayerFov=Fov0;}
   LayerPitch+=FMath::DegreesToRadians(P->Climb+P->ViewPitch.X);
   LayerYaw+=FMath::DegreesToRadians(P->ClimbYaw+P->ViewYaw.X);
   LayerFov*=P->FovK;
   return true;
  };
 }
 F.LoadSounds();
 // -S47DevWaves=auto: the bot, and where its pictures go
 if(AutoFlag())
 {
  F.bBot=true;
  const TCHAR* Cmd=FCommandLine::Get();
  FParse::Value(Cmd,TEXT("S47DevWavesShots="),F.ShotsDir);
  FParse::Value(Cmd,TEXT("S47DevWavesVideo="),F.VideoDir);
  FParse::Value(Cmd,TEXT("S47DevWavesTag="),F.Tag);
  FParse::Value(Cmd,TEXT("S47DevWavesVideoFrom="),F.VideoFrom);
  FParse::Value(Cmd,TEXT("S47DevWavesVideoLen="),F.VideoLen);
  if(!F.VideoDir.IsEmpty())F.ShotsDir.Reset();
  UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: a bot at the trigger (pictures: %s, video: %s)"),F.ShotsDir.IsEmpty()?TEXT("none"):*F.ShotsDir,F.VideoDir.IsEmpty()?TEXT("none"):*F.VideoDir);
 }
}

void FS47DevWaves::DropGun()
{
 if(!Fx)return;
 FS47DevWaveFx& F=*Fx;
 if(AS47Walker* W=F.Walker.Get())
 {
  W->ViewLayer=F.PrevLayer;
  W->MoveScale=F.PrevMoveScale;
  if(F.bBot){W->TestMove.Reset();W->bTestRun=false;}
 }
 if(F.bBot&&M)M->ZoomK=0;
 if(UCameraComponent* C=F.Cam.Get())
 {
  if(F.bFpFov)C->SetEnableFirstPersonFieldOfView(false);
  if(F.bSatSet){C->PostProcessSettings.bOverride_ColorSaturation=false;C->PostProcessSettings.ColorSaturation=FVector4(1,1,1,1);}
 }
 // (the children first: the holder is the first one in)
 for(int32 I=F.Parts.Num()-1;I>=0;--I)if(USceneComponent* C=F.Parts[I].Get())C->DestroyComponent();
 F.DropSounds();
 Fx.Reset();
}

// Where the gun is in the hand this frame: the hip, the sights, the run, the reload, and over them the kick, the sway, the walk,
// the breath, the landing, and a wall too near.
void FS47DevWaves::PoseGun(double Dt)
{
 using namespace S47DevWaves;
 using namespace S47DevWavesLocal;
 if(!Fx)return;
 FS47DevWaveFx& F=*Fx;
 AS47Walker* W=Shooter.Get();
 USceneComponent* Holder=F.Holder.Get();
 if(!W||!Holder)return;
 const bool bDead=Round.Phase==EPhase::Dead;
 const double Tick=FMath::Max(Dt,1e-4);
 // ---- how the player moves and looks ----
 const FVector PosNow=S47Three::ToUE(W->Pos.X,W->FloorY,W->Pos.Z);
 if(!F.bPrevValid){F.PrevPos=PosNow;F.PrevYaw=W->Yaw;F.PrevPitch=W->Pitch;F.bPrevValid=true;}
 const FVector Went=PosNow-F.PrevPos;
 F.PrevPos=PosNow;
 double SpeedM=FVector2D(Went.X,Went.Y).Size()/100./Tick;
 if(SpeedM>12.)SpeedM=0; // (put somewhere else, not walked there)
 else F.PlayerVel=FVector(Went.X,Went.Y,0)/Tick;
 const double YawRate=FMath::Clamp(FMath::UnwindRadians(W->Yaw-F.PrevYaw)/Tick,-8.,8.),PitchRate=FMath::Clamp((W->Pitch-F.PrevPitch)/Tick,-8.,8.);
 F.PrevYaw=W->Yaw;F.PrevPitch=W->Pitch;
 F.MoveK+=(FMath::Clamp(SpeedM/1.9,0.,1.7)-F.MoveK)*FMath::Min(1.,Dt*9.);
 const bool bAir=W->InAir();
 const bool bWantsGun=F.bTrigger||Gun.Reload>0||(M&&M->ZoomK>.05);
 F.SprintK+=((SpeedM>2.45&&!bAir&&!bWantsGun&&!bDead?1.:0.)-F.SprintK)*FMath::Min(1.,Dt*(bWantsGun?16.:7.));
 const double ReloadF=Gun.ReloadFrac();
 F.ReloadK+=((Gun.Reload>0?FMath::Min(1.,FMath::Min(ReloadF/.1,(1.-ReloadF)/.07)):0.)-F.ReloadK)*FMath::Min(1.,Dt*16.);
 const double Zoom=M?M->ZoomK:0.;
 const double AdsWas=F.AdsK;
 F.AdsK=FMath::Clamp(Zoom*(1.-F.ReloadK)*(1.-F.SprintK)*(bDead?0.:1.),0.,1.);
 if(AdsWas<.25&&F.AdsK>=.25)F.Snd(TEXT("ads_in"),FS47Sound{.Gain=.35f,.Rate=float(Rnd(.95,1.08))});
 // ---- the view: the sights bring it closer (over the game's own closer look, which RMB holds), the gun keeps its own ----
 F.FovK=float(FMath::Lerp(1.,.7,F.AdsK)/FMath::Max(.2,1.-.55*Zoom));
 if(UCameraComponent* C=F.Cam.Get())
 {
  if(F.bFpFov)
  {
   const float Fp=float(FMath::Lerp(74.,48.,F.AdsK));
   C->SetFirstPersonFieldOfView(Fp);
   F.FpK=FMath::Tan(FMath::DegreesToRadians(double(C->FieldOfView)*.5))/FMath::Tan(FMath::DegreesToRadians(double(Fp)*.5));
  }
  // low health: the colour goes out of the world
  const double Low=HudText?double(HudText->LowK):0.;
  if(Low>.01||F.bSatSet)
  {
   const double Sat=1.-.6*Low;
   C->PostProcessSettings.bOverride_ColorSaturation=Low>.01;
   C->PostProcessSettings.ColorSaturation=FVector4(Sat,Sat,Sat,1.);
   F.bSatSet=Low>.01;
  }
 }
 W->MoveScale=F.PrevMoveScale*FMath::Lerp(1.,.55,F.AdsK);
 // ---- the springs ----
 F.KickBack.Step(Dt,13.,.62);F.KickPitch.Step(Dt,11.,.5);F.KickRoll.Step(Dt,9.,.5);F.KickSide.Step(Dt,10.,.6);
 F.ViewPitch.Step(Dt,7.,.65);F.ViewYaw.Step(Dt,7.,.65);F.Land.Step(Dt,5.5,.5);
 // the climb comes back down once the fire stops
 if(Age-LastShot>.12)
 {
  const double Back=FMath::Min(1.,Dt*7.);
  F.Climb-=F.Climb*Back;F.ClimbYaw-=F.ClimbYaw*Back;
 }
 // the gun follows the look a little late
 const double SwayS=FMath::Lerp(1.,.3,F.AdsK);
 F.SwayYaw+=(FMath::Clamp(YawRate*1.7,-5.,5.)*SwayS-F.SwayYaw)*FMath::Min(1.,Dt*10.);
 F.SwayPitch+=(FMath::Clamp(-PitchRate*1.5,-4.,4.)*SwayS-F.SwayPitch)*FMath::Min(1.,Dt*10.);
 F.SwayRoll+=(FMath::Clamp(-YawRate*1.3,-5.,5.)*SwayS-F.SwayRoll)*FMath::Min(1.,Dt*8.);
 if(F.bPrevAir&&!bAir)F.Land.Kick(-1.,5.5);
 else if(!F.bPrevAir&&bAir)F.Land.Kick(.35,5.5);
 F.bPrevAir=bAir;
 // the walk and the breath
 F.BobPhase+=SpeedM*Dt*7.2;
 const double BobS=F.MoveK*FMath::Lerp(1.,.12,F.AdsK)*(W->bHeadMotion?1.:.4)*(bAir?0.:1.);
 const double BobY=FMath::Sin(F.BobPhase*.5)*.5*BobS,BobZ=(.2-FMath::Abs(FMath::Sin(F.BobPhase*.5))*.42)*BobS,BobRoll=FMath::Sin(F.BobPhase*.5)*.9*BobS;
 const double BreathS=FMath::Lerp(1.,.4,F.AdsK);
 const double BrY=FMath::Sin(Age*1.07)*.07*BreathS,BrZ=FMath::Sin(Age*.83+1.)*.1*BreathS,BrPitch=FMath::Sin(Age*.9+.4)*.2*BreathS;
 // a wall too near: the gun comes back and up instead of going into it
 {
  double Want=0;
  if(UWorld* World=W->GetWorld())
  {
   FCollisionQueryParams Reach(SCENE_QUERY_STAT(S47DevWavesReach),true,W);
   FHitResult Hit;
   const FVector From=W->EyeUE();
   if(World->LineTraceSingleByChannel(Hit,From,From+W->ForwardUE()*96.,ECC_Visibility,Reach))Want=FMath::Clamp(96.-double(Hit.Distance),0.,44.);
  }
  F.Retract+=(Want-F.Retract)*FMath::Min(1.,Dt*12.);
 }
 F.DeadK+=((bDead?1.:0.)-F.DeadK)*FMath::Min(1.,Dt*5.);
 // ---- the poses, blended ----
 const double AdsE=F.AdsK*F.AdsK*(3.-2.*F.AdsK),RunE=F.SprintK*F.SprintK*(3.-2.*F.SprintK),RelE=F.ReloadK*F.ReloadK*(3.-2.*F.ReloadK);
 FVector Loc=FMath::Lerp(FVector(22.,9.4,-10.3),FVector(15.5,0.,0.),AdsE);
 FRotator Rot=FMath::Lerp(FRotator(1.2,-1.6,-3.),FRotator::ZeroRotator,AdsE);
 Loc=FMath::Lerp(Loc,FVector(15.,8.,-15.5),RunE);
 Rot=FMath::Lerp(Rot,FRotator(22.,-38.,-12.),RunE);
 Loc+=FVector(-2.,-2.6,-3.4)*RelE;
 Rot+=FRotator(11.,-6.,22.)*RelE;
 Loc+=FVector(-F.KickBack.X-F.Retract*.8,F.KickSide.X+F.SwayYaw*.12+BobY+BrY,F.SwayPitch*.1+BobZ+BrZ+F.Land.X*2.8-F.Retract*.1-26.*F.DeadK);
 Rot+=FRotator(F.KickPitch.X+F.SwayPitch+BrPitch+F.Land.X*3.5+F.Retract*.55-38.*F.DeadK,F.SwayYaw,F.KickRoll.X+F.SwayRoll+BobRoll+25.*F.DeadK);
 Holder->SetRelativeLocationAndRotation(Loc,Rot);
 // ---- the parts that move: the bolt and its handle with every round, held back on the empty magazine ----
 F.ShotPulse=FMath::Max(0.,F.ShotPulse-Dt/.06);
 double BoltWant=0;
 if(Gun.Reload>0)BoltWant=ReloadF>=BeatAt(EBeat::BoltHome)?0.:ReloadF>=BeatAt(EBeat::BoltBack)?1.:F.bLockedBack?1.:0.;
 else if(Gun.Mag<=0){BoltWant=1;F.bLockedBack=true;}
 else
 {
  F.bLockedBack=false;
  BoltWant=F.ShotPulse>.6?(1.-F.ShotPulse)/.4:F.ShotPulse/.6;
 }
 F.BoltPos=Gun.Reload<=0&&Gun.Mag>0?BoltWant:FMath::FInterpConstantTo(F.BoltPos,BoltWant,Dt,BoltWant<F.BoltPos?1./.03:1./.07);
 if(UStaticMeshComponent* C=F.BoltComp.Get())C->SetRelativeLocation(FVector(-6.4*F.BoltPos,0,0));
 if(UStaticMeshComponent* C=F.ChargeComp.Get())C->SetRelativeLocation(FVector(-6.4*F.BoltPos,0,0));
 // the magazine: out and away, a new one up and home
 if(UStaticMeshComponent* MagC=F.MagComp.Get();MagC&&F.bModel)
 {
  const FVector Axis=FVector(.17,0,-1).GetSafeNormal();
  double Slide=0,Tip=0;
  bool bShown=true;
  if(Gun.Reload>0)
  {
   const double Out0=BeatAt(EBeat::MagOut),In1=BeatAt(EBeat::MagIn);
   if(ReloadF>=Out0&&ReloadF<Out0+.08){const double T=(ReloadF-Out0)/.08;Slide=17.*T*T;Tip=8.*T;}
   else if(ReloadF>=Out0+.08&&ReloadF<In1-.2)bShown=false;
   else if(ReloadF>=In1-.2&&ReloadF<In1){const double T=(ReloadF-(In1-.2))/.2,E=1.-FMath::Square(1.-T);Slide=21.*(1.-E);Tip=7.*(1.-E);}
  }
  const FRotator TipRot(Tip,0,0);
  MagC->SetRelativeLocationAndRotation(MagPivot+TipRot.RotateVector(-MagPivot)+Axis*Slide,TipRot);
  if(MagC->IsVisible()!=bShown)MagC->SetVisibility(bShown);
 }
 // the lights: the muzzle's goes out after its moment; the fill only as far as the night needs it
 F.MuzzleLeft-=Dt;
 if(UPointLightComponent* L=F.MuzzleLight.Get())if(F.MuzzleLeft<=0&&L->IsVisible())L->SetVisibility(false);
 {
  const double Hour=M&&M->Freeform?FMath::Fmod(M->Freeform->Clock(),86400.)/3600.:0.;
  F.DayK=FMath::Clamp(FMath::Min((Hour-5.5)/1.5,(19.5-Hour)/1.5),0.,1.);
  if(UPointLightComponent* L=F.FillLight.Get())L->SetIntensity(float(FMath::Lerp(3.5,.6,F.DayK)*FMath::Lerp(1.,.35,F.AdsK))); // (down the sights the eye looks along the gun's top: less, or it glares)
 }
}

void FS47DevWaves::Reload()
{
 if(!Gun.StartReload())return;
 LastBeat=0;
 if(Fx)
 {
  Fx->bMagGone=false;
  Fx->KickPitch.Kick(-.8,11.);
 }
}

// The reload in its beats: each has its sound and its small jolt in the hand. (The magazine let go slides out of view; one that
// falls and lies on the ground wants a mesh with its own middle.)
void FS47DevWaves::StepReload(double Dt)
{
 using namespace S47DevWaves;
 using namespace S47DevWavesLocal;
 const bool bWas=Gun.Reload>0;
 const bool bFull=Gun.Step(Dt);
 if(!Fx)return;
 FS47DevWaveFx& F=*Fx;
 if(bWas&&!bFull)
 {
  const double Frac=Gun.ReloadFrac();
  const int32 Beat=int32(BeatOf(Frac));
  if(Beat!=LastBeat)
  {
   LastBeat=Beat;
   switch(EBeat(Beat))
   {
   case EBeat::MagOut:F.Snd(TEXT("mag_out"),FS47Sound{.Gain=.75f,.Rate=float(Rnd(.96,1.04))},TEXT("rack_switch"));F.KickRoll.Kick(1.2,9.);break;
   // (the bought take seats the magazine a quarter of a second in: it starts that much before the beat would)
   case EBeat::MagIn:F.Snd(TEXT("mag_in"),FS47Sound{.Gain=.8f,.Rate=float(Rnd(.96,1.04))},TEXT("switch"));F.KickPitch.Kick(1.3,11.);F.KickBack.Kick(-.7,13.);break;
   case EBeat::BoltBack:F.Snd(TEXT("bolt_pull"),FS47Sound{.Gain=.75f,.Rate=float(Rnd(.96,1.04))},TEXT("rack_switch"));F.KickBack.Kick(.9,13.);break;
   case EBeat::BoltHome:F.Snd(TEXT("bolt_release"),FS47Sound{.Gain=.85f,.Rate=float(Rnd(.96,1.04))},TEXT("metalTap"));F.KickBack.Kick(-1.1,13.);F.KickPitch.Kick(.7,11.);break;
   default:break;
   }
  }
 }
 if(bFull)LastBeat=-1;
}

// What a round does to the world where it lands: dust off the ground, sparks off metal, chips off the wall; a mark that fades.
void FS47DevWaves::Strike(const FHitResult& Wall,const FVector& DirUE)
{
 using namespace S47DevWavesLocal;
 if(!Fx||!M)return;
 FS47DevWaveFx& F=*Fx;
 const bool bCalm=M->Flow&&M->Flow->bCalmFlash;
 const FVector At=Wall.ImpactPoint,N=Wall.ImpactNormal.GetSafeNormal();
 const FVector Out=(DirUE-2.*FVector::DotProduct(DirUE,N)*N).GetSafeNormal();
 const ESurf Surf=SurfOf(Wall);
 const FVector AtM=S47Three::ToThree(At);
 // what the sprites show by day and by night: unlit puffs would glow in the dark, so the night dims them
 const float Lit=float(FMath::Lerp(.035,.5,F.DayK));
 const double GroundCm=Shooter.IsValid()?Shooter->FloorY*100.:At.Z;
 UStaticMesh* Cube=F.CubeMesh.Get();
 if(Surf==ESurf::Metal)
 {
  F.Snd(TEXT("impact_metal"),FS47Sound{.Gain=.6f,.Rate=float(Rnd(.9,1.12)),.At=AtM},TEXT("metalTap"));
  if(FS47DevWaveSprite* S=F.Spark(false,At+N*2.,FVector::ZeroVector,FLinearColor(1.f,.8f,.5f),16.,5.,.06,bCalm?1.:4.))S->Pow=.5;
  for(int32 I=0;I<8;++I)
   if(FS47DevWaveSprite* S=F.Spark(false,At+N*1.5,(Out*Rnd(.3,1.)+N*Rnd(.2,.8)+FMath::VRand()*.55).GetSafeNormal()*Rnd(220.,620.),FLinearColor(1.f,.72f,.3f),1.2,.6,Rnd(.2,.42),bCalm?1.2:4.5))
   {S->bAlong=true;S->bVel=true;S->Len0=9.;S->Len1=3.;S->Gravity=700.;S->Drag=1.2;S->Pow=.7;}
  F.Mark(At,N,5.,FLinearColor(.02f,.02f,.022f),.8,7.);
 }
 else if(Surf==ESurf::Stone)
 {
  F.Snd(TEXT("impact_concrete"),FS47Sound{.Gain=.6f,.Rate=float(Rnd(.9,1.12)),.At=AtM},TEXT("thudSoft"));
  for(int32 I=0;I<3;++I)
   if(FS47DevWaveSprite* S=F.Spark(true,At+N*3.,(N*Rnd(40.,120.)+FMath::VRand()*40.),FLinearColor(.5f,.49f,.46f),Rnd(8.,14.),Rnd(38.,60.),Rnd(.45,.75),Lit,.34)){S->Drag=3.;S->Pow=1.3;}
  for(int32 I=0;I<2;++I)
   if(FS47DevWaveSprite* S=F.Spark(false,At+N*1.5,(Out+FMath::VRand()*.7).GetSafeNormal()*Rnd(200.,420.),FLinearColor(1.f,.75f,.4f),1.,.5,Rnd(.12,.25),bCalm?.8:2.5))
   {S->bAlong=true;S->bVel=true;S->Len0=6.;S->Len1=2.;S->Gravity=700.;S->Pow=.7;}
  if(Cube)
  {
   const FLinearColor Grey(.33f,.32f,.3f);
   for(int32 I=0;I<5;++I)F.Throw(Cube,At+N*2.,(N*Rnd(.5,1.2)+FMath::VRand()*.6)*Rnd(120.,300.),FVector(Rnd(.008,.022),Rnd(.008,.02),Rnd(.004,.012)),Rnd(1.2,2.2),GroundCm+.4,&Grey);
  }
  F.Mark(At,N,7.5,FLinearColor(.035f,.034f,.032f),.85,8.);
 }
 else
 {
  F.Snd(TEXT("impact_dirt"),FS47Sound{.Gain=.6f,.Rate=float(Rnd(.9,1.12)),.At=AtM},TEXT("thudSoft"));
  for(int32 I=0;I<4;++I)
   if(FS47DevWaveSprite* S=F.Spark(true,At+N*4.,(N*Rnd(70.,190.)+FMath::VRand()*45.),FLinearColor(.46f,.38f,.28f),Rnd(10.,16.),Rnd(55.,90.),Rnd(.6,1.),Lit,.36)){S->Drag=2.6;S->Pow=1.3;S->Gravity=-12.;}
  if(Cube)
  {
   const FLinearColor Clod(.2f,.15f,.1f);
   for(int32 I=0;I<4;++I)F.Throw(Cube,At+N*2.,(N*Rnd(.8,1.6)+FMath::VRand()*.5)*Rnd(140.,320.),FVector(Rnd(.008,.02),Rnd(.008,.02),Rnd(.006,.014)),Rnd(1.,1.8),GroundCm+.4,&Clod);
  }
  F.Mark(At,N,9.,FLinearColor(.03f,.024f,.018f),.7,8.);
 }
 if(F.Snapped.Contains(TEXT("muzzle")))F.Snap(TEXT("impact"),Age);
}

// A shot: somewhere inside the spread round the view's line, to the nearest alien on it unless something of the world stands
// before it; and everything a shot does to the hand, the eye and the ear.
void FS47DevWaves::Fire(bool bPulled)
{
 using namespace S47DevWaves;
 using namespace S47DevWavesLocal;
 AS47Walker* W=Shooter.Get();
 UWorld* World=M&&M->Web?M->Web->GetWorld():nullptr;
 if(!W||!World||!Fx)return;
 FS47DevWaveFx& F=*Fx;
 if(F.SprintK>.4)return; // the gun is still coming up from the run
 if(!Gun.Fire())
 {
  // Empty: the trigger clicks, and the magazine is changed.
  if(bPulled&&Gun.Mag<=0&&Gun.Reload<=0)
  {
   F.Snd(TEXT("dry_click"),FS47Sound{.Gain=.8f},TEXT("click"));
   Reload();
  }
  return;
 }
 ++ShotSerial;
 const bool bCalm=M->Flow&&M->Flow->bCalmFlash;
 const bool bFast=Age-LastShot<.2;
 const double Ads=F.AdsK;
 const FVector EyeUE=W->EyeUE(),Fwd=W->ForwardUE();
 FVector Right=FVector::RightVector,Up=FVector::UpVector;
 if(const UCameraComponent* C=F.Cam.Get()){Right=C->GetRightVector();Up=C->GetUpVector();}
 const double Cone=FMath::Tan(FMath::DegreesToRadians(Spread.Deg(Ads,F.MoveK)*(W->bCrouch?.8:1.)))*FMath::Sqrt(Rnd()),Turn=Rnd()*2.*UE_DOUBLE_PI;
 const FVector AimUE=(Fwd+Right*(FMath::Cos(Turn)*Cone)+Up*(FMath::Sin(Turn)*Cone)).GetSafeNormal();
 const FVector Eye=S47Three::ToThree(EyeUE);
 const FVector Dir=FVector(AimUE.X,AimUE.Z,AimUE.Y).GetSafeNormal();
 FCollisionQueryParams Line(SCENE_QUERY_STAT(S47DevWavesShot),true,W);
 for(const TSharedPtr<FS47DevWaveBody>& B:Bodies)for(const auto& Part:B->Body.All)if(const auto* Own=Part.Get())Line.AddIgnoredComponent(Own);
 FHitResult Wall;
 double Reach=Range;
 const bool bWall=World->LineTraceSingleByChannel(Wall,EyeUE,EyeUE+AimUE*(Range*100.),ECC_Visibility,Line);
 if(bWall)Reach=double(Wall.Distance)/100.;
 FS47DevWaveBody* Struck=nullptr;
 double StruckAt=Reach+.3;
 for(const TSharedPtr<FS47DevWaveBody>& B:Bodies)
 {
  if(!B->bOut||B->Fall>=0||B->Arrive>0)continue;
  const double Along=RayBody(Eye,Dir,B->At,HeightOf(B->Kind),RadiusOf(B->Kind));
  if(Along>=0&&Along<StruckAt){StruckAt=Along;Struck=B.Get();}
 }
 const FVector EndUE=EyeUE+AimUE*((Struck?StruckAt:Reach)*100.);
 // ---- the hand and the eye ----
 F.KickBack.Kick(FMath::Lerp(3.1,1.2,Ads),13.);
 F.KickPitch.Kick(FMath::Lerp(2.3,.5,Ads),11.);
 F.KickRoll.Kick((Rnd()-.5)*FMath::Lerp(3.,.8,Ads),9.);
 F.KickSide.Kick((Rnd()-.5)*FMath::Lerp(.7,.15,Ads),10.);
 F.ViewPitch.Kick(FMath::Lerp(.5,.34,Ads),7.);
 F.ViewYaw.Kick((Rnd()-.42)*FMath::Lerp(.5,.3,Ads),7.);
 F.Climb=FMath::Min(5.5,F.Climb+FMath::Lerp(.24,.15,Ads));
 F.ClimbYaw=FMath::Clamp(F.ClimbYaw+(Rnd()-.44)*FMath::Lerp(.2,.12,Ads),-2.2,2.2);
 F.ShotPulse=1;
 Spread.Shot(Ads);
 if(W->bHeadMotion)W->Shake=FMath::Max(W->Shake,FMath::Lerp(.2,.13,Ads));
 LastShot=Age;
 // ---- the ear: the crack, the body, the action, and the tail the place gives it (the open yard, or the walls) ----
 F.Snd(TEXT("shot_transient"),FS47Sound{.Gain=.45f,.Rate=float(Rnd(.97,1.03))},TEXT("metalTap"));
 F.Snd(TEXT("shot_body"),FS47Sound{.Gain=.66f,.Rate=float(Rnd(.96,1.04))},TEXT("thudSoft"));
 F.Snd(TEXT("shot_mech"),FS47Sound{.Gain=.3f,.Rate=float(Rnd(.97,1.03))});
 {
  const float TailK=bFast?.38f:1.f;
  F.Snd(TEXT("shot_tail_yard"),FS47Sound{.Gain=float(.5*(1.-.75*F.Enclosed))*TailK*F.TailGain(TEXT("shot_tail_yard"))});
  if(F.Enclosed>.12)F.Snd(TEXT("shot_tail_walls"),FS47Sound{.Gain=float(.55*F.Enclosed)*TailK*F.TailGain(TEXT("shot_tail_walls"))});
  TailOwed=bFast?Age:-1.; // (a burst's last round gets its whole tail once the fire stops: StepSound)
 }
 // ---- the muzzle: a small bright star and a wisp of smoke; its light on the world only as often as the flash limit lets it ----
 const FVector MuzzleUE=F.GunPoint(MuzzleAt);
 const double Big=F.bFpFov?F.FpK:1.;
 const double FlashLife=FMath::Max(.045,1.7*FApp::GetDeltaTime());
 bool bLit=false;
 if(FlashMay(Age,LastFlash,bCalm))if(UPointLightComponent* L=F.MuzzleLight.Get())
 {
  LastFlash=Age;F.MuzzleLeft=FMath::Max(.05,1.7*FApp::GetDeltaTime());
  L->SetVisibility(true);
  bLit=true;
 }
 {
  const float Hot=bCalm?.6f:Ads>.5?2.1f:2.9f;
  const FLinearColor Flame(1.f,.74f,.4f);
  if(FS47DevWaveSprite* S=F.Spark(false,MuzzleUE+Fwd*4.,FVector::ZeroVector,Flame,8.*Big,4.5*Big,FlashLife,Hot))S->Pow=.6;
  if(FS47DevWaveSprite* S=F.Spark(false,MuzzleUE+Fwd*8.*Big,FVector::ZeroVector,FLinearColor(1.f,.6f,.25f),3.4*Big,2.2*Big,FlashLife,Hot))
  {S->bAlong=true;S->Axis=Fwd;S->Len0=16.*Big;S->Len1=9.*Big;S->Pow=.6;}
  const double Spin=Rnd()*UE_DOUBLE_PI;
  for(int32 I=0;I<3;++I)
  {
   const double A=Spin+I*(2.*UE_DOUBLE_PI/3.)+Rnd(-.3,.3);
   const FVector Ray=(Right*FMath::Cos(A)+Up*FMath::Sin(A)+Fwd*.35).GetSafeNormal();
   if(FS47DevWaveSprite* S=F.Spark(false,MuzzleUE+Ray*3.6*Big,FVector::ZeroVector,Flame,2.3*Big,1.4*Big,FlashLife,Hot*.8f))
   {S->bAlong=true;S->Axis=Ray;S->Len0=Rnd(7.5,11.5)*Big;S->Len1=4.5*Big;S->Pow=.6;}
  }
  const float Lit=float(FMath::Lerp(.04,.45,F.DayK));
  if(FS47DevWaveSprite* S=F.Spark(true,MuzzleUE+Fwd*8.,Fwd*Rnd(50.,90.)+Up*Rnd(25.,45.)+Right*Rnd(-14.,14.)+F.PlayerVel*.5,FLinearColor(.62f,.6f,.58f),7.*Big,Rnd(30.,44.),Rnd(.5,.8),Lit,.2)){S->Drag=2.2;S->Pow=1.4;}
 }
 // a tracer every few rounds: a thin bright line that flies, not a light
 if(ShotSerial%3==1||!bFast)
 {
  const FVector From=MuzzleUE+AimUE*140.;
  const double Len=FVector::Dist(From,EndUE);
  if(Len>320.)
  {
   const FVector Along=(EndUE-From)/Len;
   if(FS47DevWaveSprite* S=F.Spark(false,From,Along*17000.,FLinearColor(1.f,.62f,.25f),2.6,2.6,FMath::Min(Len/17000.,.4),bCalm?1.:3.4))
   {S->bAlong=true;S->Axis=Along;S->Len0=S->Len1=FMath::Min(280.,Len*.5);S->Pow=.12;}
  }
 }
 // the casing: out to the right, up and back, ringing where it lands
 if(UStaticMesh* Case=F.CasingMesh.Get())
  if(FS47DevWaveBit* B=F.Throw(Case,F.GunPoint(PortAt),F.PlayerVel+Right*Rnd(150.,230.)+Up*Rnd(110.,180.)-Fwd*Rnd(10.,70.),FVector(1.15),4.,W->FloorY*100.+.6))
  {
   static const TCHAR* const Tinks[]={TEXT("casing_1"),TEXT("casing_2"),TEXT("casing_3")};
   B->Tink=Tinks[ShotSerial%3];B->TinkGain=.3f;B->Bounce=.38;
  }
 if(bLit&&FightAge>2.55&&!F.Snapped.Contains(TEXT("muzzle")))F.Snap(TEXT("muzzle"),Age);
 else if(Ads>.9&&F.Snapped.Contains(TEXT("alienhit")))F.Snap(TEXT("adsfire"),Age);
 if(!Struck)
 {
  if(bWall)Strike(Wall,AimUE);
  return;
 }
 // ---- an alien struck: no gore: a dark burst and its own cold light, and it gives with the round ----
 const FVector HitM=Eye+Dir*StruckAt,HitUE=S47Three::ToUE(HitM);
 const bool bHead=HeadShot(Struck->Kind,HitM.Y-Struck->At.Y);
 Struck->Hp-=ShotDamage(bHead);
 const float Voice=float(Struck->Kind==EKind::Tall?.72:Struck->Kind==EKind::Small?1.28:Struck->Kind==EKind::Crab?1.1:1.)*float(Rnd(.95,1.06));
 if(FS47DevWaveSprite* S=F.Spark(false,HitUE-AimUE*6.,FVector::ZeroVector,FLinearColor(.6f,.92f,1.f),26.,9.,.13,bCalm?.9:2.8))S->Pow=.8;
 if(FS47DevWaveSprite* S=F.Spark(false,HitUE-AimUE*4.,FVector::ZeroVector,FLinearColor(.45f,.8f,1.f),15.,24.,.45,bCalm?.4:1.))S->Pow=1.6;
 for(int32 I=0;I<2;++I)
  if(FS47DevWaveSprite* S=F.Spark(true,HitUE-AimUE*5.,AimUE*Rnd(40.,110.)+FMath::VRand()*55.,FLinearColor(.012f,.014f,.018f),10.,Rnd(34.,48.),Rnd(.3,.45),.02,.6)){S->Drag=3.;S->Pow=1.2;}
 if(UStaticMesh* Cube=F.CubeMesh.Get())
 {
  const FLinearColor Dark(.01f,.011f,.014f);
  for(int32 I=0;I<6;++I)F.Throw(Cube,HitUE,(AimUE*.5+FMath::VRand()).GetSafeNormal()*Rnd(120.,300.),FVector(Rnd(.008,.02)),Rnd(.5,.9),W->FloorY*100.+.4,&Dark);
 }
 // (which way the round turned it: struck off its middle, it twists that way)
 const FVector2D Across(-Dir.Z,Dir.X);
 const double Off=FVector2D::DotProduct(FVector2D(HitM.X-Struck->At.X,HitM.Z-Struck->At.Z),Across.GetSafeNormal());
 Struck->Twist.Kick(FMath::Clamp(Off*30.,-12.,12.),4.8);
 if(HudText){HudText->HitK=1;}
 if(Struck->Hp<=0)
 {
  Struck->Fall=0;Struck->Speed=0;Struck->MoteWait=0;Struck->bFizzed=false;
  const int32 Got=Round.Killed(Struck->Kind,bHead);
  F.Snd(TEXT("alien_death"),FS47Sound{.Gain=.7f,.Rate=Voice,.At=Struck->At+FVector(0,HeightOf(Struck->Kind)*.7,0)},TEXT("alienRadio"));
  F.Snd(TEXT("kill_marker"),FS47Sound{.Gain=.5f},TEXT("switch"));
  if(W->bHeadMotion&&StruckAt<7.)W->Shake=FMath::Max(W->Shake,.35);
  if(HudText)
  {
   HudText->KillK=1;HudText->bHeadKill=bHead;HudText->GotK=1;
   HudText->Got=FString::Printf(TEXT("+%d  %s%s"),Got,*FString(KindName(Struck->Kind)).ToUpper(),bHead?TEXT("  /  HEAD"):TEXT(""));
  }
  F.Snap(TEXT("kill"),Age);
  // (the wave was cleared by this one: the next is announced, and a crate is marked)
  if(Round.Phase==EPhase::Banner)
  {
   SpawnSerial=0;SpawnWait=0;
   F.Snd(TEXT("wave_clear"),FS47Sound{.Gain=.65f,.When=.35f},TEXT("switch"));
   UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: wave %d cleared, score %d, health %d"),Round.Wave-1,Round.Score,Round.Health);
  }
 }
 else
 {
  F.Snd(TEXT("alien_hit"),FS47Sound{.Gain=.6f,.Rate=Voice,.At=HitM},TEXT("thudSoft"));
  F.Snd(TEXT("hit_marker"),FS47Sound{.Gain=.4f});
  Struck->Lean.Kick(Struck->Kind==EKind::Tall?7.:11.,4.2);
  if(Struck->Kind==EKind::Tall)
  {
   if(Struck->StaggerRest<=0)
   {
    // the Tall One reels, for all to see; then it cannot be made to again for a while
    Struck->Stagger=StaggerSeconds;Struck->StaggerRest=StaggerSeconds+StaggerRest;
    Struck->Lean.Kick(24.,4.2);
    F.Snd(TEXT("tall_stagger"),FS47Sound{.Gain=.75f,.At=Struck->At+FVector(0,1.4,0)},TEXT("thudSoft"));
    const float Lit=float(FMath::Lerp(.035,.5,F.DayK));
    for(int32 I=0;I<3;++I)
     if(FS47DevWaveSprite* S=F.Spark(true,S47Three::ToUE(Struck->At+FVector(Rnd(-.3,.3),.1,Rnd(-.3,.3))),FVector(Rnd(-40,40),Rnd(-40,40),Rnd(30,70)),FLinearColor(.46f,.38f,.28f),30.,95.,Rnd(.7,1.),Lit,.3)){S->Drag=2.;S->Pow=1.3;}
   }
  }
  else Struck->Stagger=FMath::Max(Struck->Stagger,.16);
  F.Snap(TEXT("alienhit"),Age);
 }
}

// The supply crate: out at a lit spot when a wave is cleared, taken by walking up to it.
void FS47DevWaves::StepCrate(double Dt)
{
 using namespace S47DevWaves;
 using namespace S47DevWavesLocal;
 if(!Fx)return;
 FS47DevWaveFx& F=*Fx;
 AS47Walker* W=Shooter.Get();
 UWorld* World=M&&M->Web?M->Web->GetWorld():nullptr;
 UStaticMeshComponent* Crate=F.CrateComp.Get();
 if(!W||!World||!Crate)return;
 if(Round.bCrate&&!F.bCrateOut)
 {
  // a place on the yard's own ground, some way off, with a clear line from the player when there is one
  TOptional<FVector> Best;
  double BestScore=-1;
  const double Turn=Rnd()*2.*UE_DOUBLE_PI;
  for(int32 I=0;I<10;++I)for(const double Metres:{9.,12.,7.})
  {
   const double Ang=Turn+I*(2.*UE_DOUBLE_PI/10.);
   const FVector2D C(YardX+FMath::Cos(Ang)*Metres,YardZ+FMath::Sin(Ang)*Metres);
   if(!W->Walkable(C.X,C.Y))continue;
   FCollisionQueryParams Under(SCENE_QUERY_STAT(S47DevWavesCrate),true,W);
   FHitResult Ground;
   if(!World->LineTraceSingleByChannel(Ground,S47Three::ToUE(C.X,W->FloorY+3.,C.Y),S47Three::ToUE(C.X,W->FloorY-4.,C.Y),ECC_Visibility,Under))continue;
   const double G=S47Three::ToThree(Ground.ImpactPoint).Y;
   if(FMath::Abs(G-W->FloorY)>1.2)continue;
   FHitResult Block;
   const bool bSeen=!World->LineTraceSingleByChannel(Block,W->EyeUE(),S47Three::ToUE(C.X,G+.6,C.Y),ECC_Visibility,Under);
   const double Far=FVector2D::Distance(C,FVector2D(W->Pos.X,W->Pos.Z));
   const double Score=(bSeen?3.:0.)+(Far>6.&&Far<16.?2.:0.)+Rnd();
   if(Score>BestScore){BestScore=Score;Best=FVector(C.X,G,C.Y);}
  }
  if(!Best)Best=FVector(W->Pos.X+2.5,W->FloorY,W->Pos.Z+2.5);
  F.CrateAt=*Best;F.bCrateOut=true;
  Crate->SetWorldLocationAndRotation(S47Three::ToUE(F.CrateAt),FRotator(0,Rnd(0.,360.),0));
  Crate->SetVisibility(true);
  if(USpotLightComponent* L=F.CrateLamp.Get())
  {
   L->SetWorldLocationAndRotation(S47Three::ToUE(F.CrateAt+FVector(0,5.5,0)),FRotator(-90,0,0));
   L->SetVisibility(true);
  }
  UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: a supply crate at %.1f %.1f"),F.CrateAt.X,F.CrateAt.Z);
 }
 if(!F.bCrateOut)return;
 if(!Round.bCrate)
 {
  F.bCrateOut=false;Crate->SetVisibility(false);
  if(USpotLightComponent* L=F.CrateLamp.Get())L->SetVisibility(false);
  return;
 }
 if(FVector2D::Distance(FVector2D(F.CrateAt.X,F.CrateAt.Z),FVector2D(W->Pos.X,W->Pos.Z))<=CrateReach&&Round.TakeCrate())
 {
  Gun.Supply();
  F.Snd(TEXT("pickup"),FS47Sound{.Gain=.7f},TEXT("switch"));
  if(HudText){HudText->GotK=1.6f;HudText->Got=FString::Printf(TEXT("+%d ROUNDS   +%d VITALS"),CrateAmmo,CrateHeal);}
  const bool bCalm=M->Flow&&M->Flow->bCalmFlash;
  for(int32 I=0;I<8;++I)
   if(FS47DevWaveSprite* S=F.Spark(false,S47Three::ToUE(F.CrateAt+FVector(Rnd(-.25,.25),.3,Rnd(-.2,.2))),FVector(Rnd(-30,30),Rnd(-30,30),Rnd(60,150)),FLinearColor(.5f,1.f,.6f),4.,1.5,Rnd(.5,.9),bCalm?.5:1.4))S->Pow=1.4;
  F.bCrateOut=false;Crate->SetVisibility(false);
  if(USpotLightComponent* L=F.CrateLamp.Get())L->SetVisibility(false);
  UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: the crate taken: %d in the magazine, %d carried, health %d"),Gun.Mag,Gun.Reserve,Round.Health);
 }
}

// The sounds that are not one event: the tail a burst is owed, the heart at low health, and how closed in the place is.
void FS47DevWaves::StepSound(double Dt)
{
 using namespace S47DevWaves;
 using namespace S47DevWavesLocal;
 if(!Fx)return;
 FS47DevWaveFx& F=*Fx;
 AS47Walker* W=Shooter.Get();
 if(!W)return;
 if(TailOwed>=0&&Age-LastShot>.16)
 {
  TailOwed=-1;
  F.Snd(TEXT("shot_tail_yard"),FS47Sound{.Gain=float(.42*(1.-.75*F.Enclosed))*F.TailGain(TEXT("shot_tail_yard"))});
  if(F.Enclosed>.12)F.Snd(TEXT("shot_tail_walls"),FS47Sound{.Gain=float(.45*F.Enclosed)*F.TailGain(TEXT("shot_tail_walls"))});
 }
 // how much stands round the player within fourteen metres: the open yard, or between the buildings
 F.EnclosedWait-=Dt;
 if(F.EnclosedWait<=0)
 {
  F.EnclosedWait=.5;
  int32 Hits=0;
  if(UWorld* World=W->GetWorld())
  {
   FCollisionQueryParams Round8(SCENE_QUERY_STAT(S47DevWavesPlace),true,W);
   const FVector From=W->EyeUE();
   for(int32 I=0;I<8;++I)
   {
    FHitResult Hit;
    const double A=I*(UE_DOUBLE_PI/4.);
    if(World->LineTraceSingleByChannel(Hit,From,From+FVector(FMath::Cos(A),FMath::Sin(A),0)*1400.,ECC_Visibility,Round8))++Hits;
   }
  }
  F.Enclosed+=(FMath::Clamp(Hits/5.,0.,1.)-F.Enclosed)*.5;
 }
 // the heart: slow and soft at a third of the health, quicker and nearer below that (a sound and a slow pulse of the edges, never a flash)
 const double Low=Round.Phase==EPhase::Fight||Round.Phase==EPhase::Banner?FMath::Clamp(1.-Round.Health/34.,0.,1.):0.;
 if(Low>.01)
 {
  HeartWait-=Dt;
  if(HeartWait<=0)
  {
   HeartWait=FMath::Lerp(1.,.66,Low);
   F.Snd(TEXT("heartbeat"),FS47Sound{.Gain=float(.3+.5*Low)});
   if(HudText)HudText->Pulse=1;
  }
 }
 else HeartWait=0;
}

// -S47DevWaves=auto: a bot at the trigger, so that the shooter can be seen without anybody at the keys. It turns the walker's own
// look, holds the walker's own test keys, and pulls the same trigger the mouse does.
void FS47DevWaves::StepBot(double Dt)
{
 using namespace S47DevWaves;
 using namespace S47DevWavesLocal;
 if(!Fx||!Fx->bBot)return;
 FS47DevWaveFx& F=*Fx;
 AS47Walker* W=Shooter.Get();
 if(!W)return;
 F.bBotFire=false;F.bBotReload=false;
 bool bAds=false,bRun=false,bShoot=false;
 FVector2D Move(0,0);
 TOptional<FVector> AimAt;
 const FVector Eye=S47Three::ToThree(W->EyeUE());
 const FVector2D Ahead(-FMath::Sin(W->Yaw),-FMath::Cos(W->Yaw));
 F.BotStrafeLeft-=Dt;
 if(F.BotStrafeLeft<=0){F.BotStrafe=FMath::RandBool()?1.:-1.;F.BotStrafeLeft=Rnd(1.2,2.6);}
 if(Round.Phase==EPhase::Fight)
 {
  const FS47DevWaveBody* Near=nullptr;
  double NearM=1e9;
  for(const TSharedPtr<FS47DevWaveBody>& B:Bodies)
  {
   if(!B->bOut||B->Fall>=0||B->Arrive>0)continue;
   const double Far=FVector2D(B->At.X-W->Pos.X,B->At.Z-W->Pos.Z).Size();
   if(Far<NearM){NearM=Far;Near=B.Get();}
  }
  if(Round.Wave==1&&FightAge<3.5&&!F.ShotsDir.IsEmpty())
  {
   // the first seconds are for the pictures: the hip, the sights, then a few rounds into the ground ahead
   if(FightAge>1.&&FightAge<2.3)bAds=true;
   if(FightAge>=2.3){AimAt=FVector(W->Pos.X+Ahead.X*7.,W->FloorY+.02,W->Pos.Z+Ahead.Y*7.);bShoot=FightAge>2.6;}
  }
  else if(Near)
  {
   AimAt=Near->At+FVector(0,HeightOf(Near->Kind)*(Near->Kind==EKind::Crab?.5:.68),0);
   bAds=NearM>13.;
   bShoot=NearM<48.;
   if(NearM<5.5)Move=FVector2D(F.BotStrafe,1.);
   else if(NearM<16.)Move=FVector2D(F.BotStrafe*.6,0.);
  }
  if(Gun.Mag<=0||(Gun.Mag<10&&!Near))F.bBotReload=true;
 }
 else if(Round.Phase==EPhase::Banner)
 {
  if(Round.bCrate&&F.bCrateOut)
  {
   // between the waves: to the crate, at a run while it is far
   AimAt=F.CrateAt+FVector(0,.35,0);
   const double Far=FVector2D(F.CrateAt.X-W->Pos.X,F.CrateAt.Z-W->Pos.Z).Size();
   const double Off=FMath::Abs(FMath::UnwindRadians(FMath::Atan2(-(F.CrateAt.X-W->Pos.X),-(F.CrateAt.Z-W->Pos.Z))-W->Yaw));
   if(Off<.5)Move=FVector2D(0.,-1.);
   bRun=Far>4.5&&Off<.3;
  }
  if(Gun.Mag<MagSize&&Round.Wait<BannerSeconds-.6)F.bBotReload=true;
 }
 if(AimAt)
 {
  // the look turns to it (less the climb, so that the view, not only the look, is on it)
  const FVector To=*AimAt-Eye;
  const double WantYaw=FMath::Atan2(-To.X,-To.Z)-FMath::DegreesToRadians(F.ClimbYaw+F.ViewYaw.X);
  const double WantPitch=FMath::Atan2(To.Y,FVector2D(To.X,To.Z).Size())-FMath::DegreesToRadians(F.Climb+F.ViewPitch.X);
  const double TurnK=FMath::Min(1.,Dt*9.);
  const double Dy=FMath::UnwindRadians(WantYaw-W->Yaw),Dp=WantPitch-W->Pitch;
  W->Yaw+=Dy*TurnK;
  W->Pitch=FMath::Clamp(W->Pitch+Dp*TurnK,-1.35,1.25);
  if(bShoot&&FMath::Abs(Dy)<.035&&FMath::Abs(Dp)<.045)
  {
   // in bursts, as a hand would
   if(F.BotRest>0)F.BotRest-=Dt;
   else
   {
    F.bBotFire=true;
    F.BotBurst+=Dt;
    if(F.BotBurst>(bAds?.45:.8)){F.BotBurst=0;F.BotRest=bAds?.3:.2;}
   }
  }
 }
 W->TestMove=Move;
 W->bTestRun=bRun;
 // (the sights are the game's own closer look, which a held right mouse raises: the bot raises the same value itself)
 F.BotZoom+=((bAds?1.:0.)-F.BotZoom)*FMath::Min(1.,Dt*9.);
 M->ZoomK=F.BotZoom<.002?0.:F.BotZoom;
}

// -S47DevWaves=auto: starts the shooter by itself once the night is in the player's hands (at SARO, where a developer run begins).
// -S47DevWavesHour=13 sets the hour first.
void FS47DevWaves::StepAuto(double Dt)
{
 static bool bDone=false;
 static double Waited=0,Told=0;
 if(bDone||!S47DevWavesLocal::AutoFlag()||!M)return;
 if(Running()){bDone=true;return;}
 Waited+=FMath::Clamp(Dt,0.,.1);
 if(Waited<4.)return;
 if(Waited>70.){bDone=true;UE_LOG(LogS47DevWaves,Warning,TEXT("S47: dev wave shooter: -S47DevWaves=auto gave up: %s"),*WhyNot());return;}
 // (a sheet the night put in front of the player is put down: nobody is there to read it)
 if(M->Mode==US47Main::EMode::Play&&M->ModalOpen()&&M->UI){M->UI->Close();return;}
 const FString Why=WhyNot();
 if(!Why.IsEmpty())
 {
  if(Waited-Told>5.){Told=Waited;UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: -S47DevWaves=auto waits: %s"),*Why);}
  return;
 }
 double Hour=-1;
 if(FParse::Value(FCommandLine::Get(),TEXT("S47DevWavesHour="),Hour)&&Hour>=0&&Hour<24&&M->Freeform)
 {
  const double Now=FMath::Fmod(M->Freeform->Clock(),86400.);
  M->Freeform->TestAdvance(FMath::Fmod(Hour*3600.-Now+86400.,86400.));
  UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: -S47DevWavesHour: the clock set to %.1f h"),Hour);
 }
 Start();
 bDone=Running();
}

// The pictures of a bot's run (-S47DevWavesShots=<folder>): each once, when what it is of is on the screen. And the frames of a
// short film (-S47DevWavesVideo=<folder>): every frame the screen draws for a while (as fast as the pictures can be written: a
// slide show, not a smooth film; the engine's clock is left alone, since a fixed step changed how the night's exposure settled).
void FS47DevWaves::StepPictures(double Dt)
{
 using namespace S47DevWaves;
 using namespace S47DevWavesLocal;
 if(!Fx||!Fx->bBot)return;
 FS47DevWaveFx& F=*Fx;
 if(!F.VideoDir.IsEmpty())
 {
  if(F.bVideoDone)return;
  if(!F.bVideoOn&&Round.Phase==EPhase::Fight&&Round.Wave==1&&FightAge>=F.VideoFrom)
  {
   F.bVideoOn=true;F.VideoAge=0;F.VideoFrame=0;
   UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: film frames to %s"),*F.VideoDir);
  }
  if(F.bVideoOn)
  {
   F.VideoAge+=Dt;
   if(!FScreenshotRequest::IsScreenshotRequested())FScreenshotRequest::RequestScreenshot(F.VideoDir/FString::Printf(TEXT("f_%05d.png"),F.VideoFrame++),true,false);
   if(F.VideoAge>=F.VideoLen)
   {
    F.bVideoOn=false;F.bVideoDone=true;
    UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: film done, %d frames"),F.VideoFrame);
   }
  }
  return;
 }
 if(F.ShotsDir.IsEmpty())return;
 const bool bFight=Round.Phase==EPhase::Fight;
 if(Round.Phase==EPhase::Banner&&Round.Wave==1&&Round.Wait<BannerSeconds-1.7)F.Snap(TEXT("card"),Age);
 if(bFight&&Round.Wave==1)
 {
  if(FightAge>.5&&FightAge<1.)F.Snap(TEXT("hip"),Age);
  if(FightAge>1.8&&FightAge<2.3&&F.AdsK>.97)F.Snap(TEXT("ads"),Age);
  if(FightAge>9.&&!F.bTrigger&&Gun.Reload<=0)F.Snap(TEXT("hud"),Age);
  // (a wave the bot cannot finish must not hold the pictures up)
  if(FightAge>50.)
  {
   UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: pictures: wave 1 taken as cleared after 50 s"));
   for(const TSharedPtr<FS47DevWaveBody>& B:Bodies)if(B->bOut)PutAway(*B);
   for(int32 Guard=0;Guard<200&&Round.Phase==EPhase::Fight&&Round.Wave==1;++Guard){if(Round.Out==0)Round.Spawned();Round.Killed(EKind::Small);}
   SpawnSerial=0;SpawnWait=0;
  }
 }
 if(Gun.Reload>0)
 {
  const double Frac=Gun.ReloadFrac();
  if(Frac>.3&&Frac<.5)F.Snap(TEXT("reload"),Age);
  if(Frac>.78&&Frac<.86)F.Snap(TEXT("reloadbolt"),Age);
 }
 for(const TSharedPtr<FS47DevWaveBody>& B:Bodies)if(B->bOut&&B->Fall>FallSeconds+LieSeconds+.3){F.Snap(TEXT("dissolve"),Age);break;}
 if(F.SprintK>.9)F.Snap(TEXT("sprint"),Age);
 if(Round.Phase==EPhase::Banner&&Round.Wave>=2)
 {
  F.Snap(TEXT("cleared"),Age);
  if(F.bCrateOut&&Shooter.IsValid()&&FVector2D(F.CrateAt.X-Shooter->Pos.X,F.CrateAt.Z-Shooter->Pos.Z).Size()<6.5)F.Snap(TEXT("crate"),Age);
 }
 if(bFight&&Round.Wave>=2)
 {
  // the low-health look, shown for the picture and then given back; and at the end the card of a round lost
  if(FightAge>4.&&FightAge<7.5){F.bForcedLow=true;Round.Health=FMath::Min(Round.Health,20);if(FightAge>5.2)F.Snap(TEXT("low"),Age);}
  else if(F.bForcedLow&&FightAge>=7.5){F.bForcedLow=false;Round.Health=FMath::Max(Round.Health,80);}
  if(FightAge>18.&&!F.bForcedDead)
  {
   F.bForcedDead=true;
   Round.Health=0;Round.Phase=EPhase::Dead;Round.Wait=CardSeconds;
  }
 }
 if(Round.Phase==EPhase::Dead&&Round.Wait<CardSeconds-1.5)F.Snap(TEXT("dead"),Age);
 if(F.bForcedDead&&Round.Phase!=EPhase::Dead&&!F.bPicturesSaid)
 {
  F.bPicturesSaid=true;
  UE_LOG(LogS47DevWaves,Display,TEXT("S47: dev wave shooter: pictures done (%d)"),F.Snapped.Num());
 }
}

// ---------- the screen ----------

void FS47DevWaves::ShowHud(bool bShow)
{
 if(!GEngine||!GEngine->GameViewport)return;
 if(bShow&&!Hud)
 {
  HudText=MakeShared<FS47DevWaveHud>();
  Hud=SNew(S47DevWavesLocal::SS47DevWavesHud).Source(HudText);
  GEngine->GameViewport->AddViewportWidgetContent(Hud.ToSharedRef(),27);
 }
 else if(!bShow&&Hud)
 {
  GEngine->GameViewport->RemoveViewportWidgetContent(Hud.ToSharedRef());
  Hud.Reset();
  HudText.Reset();
 }
}

void FS47DevWaves::WriteHud(double Dt)
{
 using namespace S47DevWaves;
 if(!HudText)return;
 FS47DevWaveHud& H=*HudText;
 const float D=float(Dt);
 const EPhase Was=H.Phase;
 H.Phase=Round.Phase;H.Wave=Round.Wave;H.Left=Round.Left();H.Score=Round.Score;H.Health=Round.Health;H.Mag=Gun.Mag;H.Reserve=Gun.Reserve;
 H.Times=StreakTimes(Round.Streak);H.Kills=Round.Kills;H.Best=Round.BestStreak;H.Heads=Round.Heads;
 H.Reload=float(Gun.ReloadFrac());
 H.bBot=Fx&&Fx->bBot;
 const double Ads=Fx?Fx->AdsK:0.,Moving=Fx?Fx->MoveK:0.;
 // the crosshair: its gap is the spread, as wide on the screen as it is in the world (the view about 70 degrees high over 900 lines)
 H.Gap=float(4.+Spread.Deg(Ads,Moving)*12.);
 H.Shown=float(FMath::Clamp(1.-Ads*1.6,0.,1.)*(Fx?FMath::Clamp(1.-Fx->SprintK*1.5,0.,1.)*FMath::Clamp(1.-Fx->ReloadK,0.,1.):1.));
 H.HitK=FMath::Max(0.f,H.HitK-D/.22f);
 H.KillK=FMath::Max(0.f,H.KillK-D/.5f);
 H.HurtK=FMath::Max(0.f,H.HurtK-D/.7f);
 H.GotK=FMath::Max(0.f,H.GotK-D/1.6f);
 H.Pulse=FMath::Max(0.f,H.Pulse-D/.5f);
 for(float& Mark:H.HurtAt)Mark=FMath::Max(0.f,Mark-D/1.4f);
 const float Low=Round.Phase==EPhase::Dead?0.f:float(FMath::Clamp(1.-Round.Health/34.,0.,1.));
 H.LowK+=(Low-H.LowK)*FMath::Min(1.f,D*4.f);
 // the card
 if(Was!=H.Phase)H.CardT=0;
 H.CardT+=D;
 if(Round.Phase==EPhase::Banner)
 {
  H.Card=FString::Printf(TEXT("WAVE %d"),Round.Wave);
  bool bPale=false,bCrab=false,bTall=false;
  for(int32 I=0;I<WaveSize(Round.Wave);++I){const EKind K=KindOf(Round.Wave,I);bPale|=K==EKind::Pale;bCrab|=K==EKind::Crab;bTall|=K==EKind::Tall;}
  FString Who=FString::Printf(TEXT("%d CONTACTS"),WaveSize(Round.Wave));
  if(bTall)Who+=TEXT("   /   TALL ONE AMONG THEM");
  else if(bCrab)Who+=TEXT("   /   CRAWLERS AMONG THEM");
  else if(bPale)Who+=TEXT("   /   PALE ONES AMONG THEM");
  H.CardSub=Round.Wave>1?FString::Printf(TEXT("WAVE %d CLEARED   /   %s"),Round.Wave-1,*Who):Who;
  H.CardK=float(FMath::Clamp(FMath::Min(double(H.CardT)/.25,Round.Wait/.5),0.,1.));
 }
 else if(Round.Phase==EPhase::Dead)
 {
  H.Card=TEXT("YOU WENT DOWN");
  H.CardSub=FString::Printf(TEXT("WAVE %d   /   SCORE %d   /   %d DOWN   /   BEST STREAK %d"),Round.Wave,Round.Score,Round.Kills,Round.BestStreak);
  H.CardK=FMath::Clamp(H.CardT/.4f,0.f,1.f);
  H.DeadLeft=float(Round.Wait);
 }
 else H.CardK=FMath::Max(0.f,H.CardK-D/.3f);
 // the crate
 H.bCrate=Fx&&Fx->bCrateOut&&Round.bCrate;
 if(H.bCrate)if(const AS47Walker* W=Shooter.Get())
 {
  const FVector2D To(Fx->CrateAt.X-W->Pos.X,Fx->CrateAt.Z-W->Pos.Z),Ahead(-FMath::Sin(W->Yaw),-FMath::Cos(W->Yaw)),Right(FMath::Cos(W->Yaw),-FMath::Sin(W->Yaw));
  H.CrateM=float(To.Size());
  H.CrateDeg=float(FMath::RadiansToDegrees(FMath::Atan2(FVector2D::DotProduct(To,Right),FVector2D::DotProduct(To,Ahead))));
 }
}

// ---------- each frame ----------

void FS47DevWaves::Step(double Dt)
{
 using namespace S47DevWaves;
 StepAuto(Dt);
 if(!Running())return;
 const FString Off=WhyOff();
 if(!Off.IsEmpty()){Stop(*Off);return;}
 // A menu or a panel holds it, as it holds the night.
 if(M->bPausedByMenu||M->ModalOpen()||!FMath::IsFinite(Dt)||Dt<=0)return;
 const double D=FMath::Min(Dt,.1);
 Age+=D;
 const EPhase Was=Round.Phase;
 // The card of a round lost has been up its time: a new round, from wave 1 (F10's stop line ends the shooter).
 if(Round.Step(D)){Restart();return;}
 if(Was==EPhase::Banner&&Round.Phase==EPhase::Fight)FightAge=0;
 if(Round.Phase==EPhase::Fight)FightAge+=D;
 AS47Walker* W=Shooter.Get();
 StepBot(D);
 // The trigger and the reload, read straight off the mouse, the keyboard and the pad (no rows in the key table: they mean
 // something only here). The sights are the game's own closer look (right mouse), which the gun follows.
 bool bHeld=false,bReloadKey=false;
 if(Round.Phase!=EPhase::Dead&&W->IsActive())if(const APlayerController* PC=Cast<APlayerController>(W->GetController()))
 {
  bReloadKey=PC->WasInputKeyJustPressed(EKeys::R)||S47Gamepad::Pressed(EKeys::Gamepad_FaceButton_Left);
  bHeld=PC->IsInputKeyDown(EKeys::LeftMouseButton)||S47Gamepad::Down(EKeys::Gamepad_RightTrigger);
 }
 if(Fx&&Fx->bBot&&Round.Phase!=EPhase::Dead){bHeld|=Fx->bBotFire;bReloadKey|=Fx->bBotReload;}
 const bool bPulled=bHeld&&!bTriggerWas;
 bTriggerWas=bHeld;
 if(Fx)Fx->bTrigger=bHeld;
 if(bReloadKey)Reload();
 if(bHeld)Fire(bPulled);
 Spread.Step(D);
 StepReload(D);
 // The wave comes out one at a time, never more at once than the wave allows.
 SpawnWait-=D;
 if(Round.MaySpawn()&&SpawnWait<=0)
 {
  if(Spawn()){Round.Spawned();SpawnWait=WaveGap(Round.Wave);SpawnDry=0;}
  else
  {
   SpawnWait=.5;SpawnDry+=.5;
   if(SpawnDry>8&&Round.Out==0){Stop(TEXT("no alien body or free ground here"));return;}
  }
 }
 for(const TSharedPtr<FS47DevWaveBody>& B:Bodies)if(B->bOut)StepBody(*B,D);
 StepCrate(D);
 StepSound(D);
 PoseGun(D);
 if(Fx)Fx->Step(D);
 WriteHud(D);
 StepPictures(D);
}

// ---------- the rules, held by -S47SelfTest ----------

bool FS47DevWaves::SelfTest(TArray<FString>& Lines)
{
 using namespace S47DevWaves;
 bool bOk=true;
 const auto Check=[&](bool bGood,const FString& What){Lines.Add(FString::Printf(TEXT("%s devWaves: %s"),bGood?TEXT("ok"):TEXT("FAIL"),*What));bOk&=bGood;};
 {
  bool bGrows=true,bCapped=true;
  for(int32 N=1;N<60;++N)
  {
   bGrows&=WaveSize(N+1)>=WaveSize(N)&&WaveSpeed(N+1)>=WaveSpeed(N)&&WaveAtOnce(N+1)>=WaveAtOnce(N)&&WaveGap(N+1)<=WaveGap(N);
   bCapped&=WaveSize(N)<=MostInWave&&WaveAtOnce(N)<=MostAtOnce&&WaveSpeed(N)<=4.2&&WaveGap(N)>=.45;
  }
  Check(bGrows&&WaveSize(2)>WaveSize(1)&&WaveSpeed(2)>WaveSpeed(1),FString::Printf(TEXT("each wave is at least as big and as fast as the one before (wave 1: %d at %.2f m/s, wave 5: %d at %.2f m/s)"),WaveSize(1),WaveSpeed(1),WaveSize(5),WaveSpeed(5)));
  Check(bCapped&&WaveSize(0)==WaveSize(1)&&WaveSize(-3)==WaveSize(1),TEXT("size, speed and how many at once stop at their caps; a wave below 1 is wave 1"));
  Check(WaveSpeed(1)*PaceOf(EKind::Crab)<3.04&&WaveSpeed(1)*PaceOf(EKind::Small)*(1.+S47DevWavesLocal::PaceSpread)*S47DevWavesLocal::LungeTimes<3.04,
   TEXT("in wave 1 even the fastest kind, at its quickest and lunging, is slower than the player's run"));
 }
 {
  bool bSmall=true,bPale=false,bCrab=false,bTall=false;
  for(int32 I=0;I<WaveSize(1);++I)bSmall&=KindOf(1,I)==EKind::Small;
  for(int32 I=0;I<WaveSize(4);++I){const EKind K=KindOf(4,I);bPale|=K==EKind::Pale;bCrab|=K==EKind::Crab;bTall|=K==EKind::Tall;}
  Check(bSmall&&bPale&&bCrab&&bTall,TEXT("wave 1 is small Roswells only; by wave 4 the Pale One, the crab and the Tall One come too"));
  Check(HealthOf(EKind::Tall)>HealthOf(EKind::Pale)&&HealthOf(EKind::Pale)>HealthOf(EKind::Small)&&ScoreOf(EKind::Tall)>ScoreOf(EKind::Small),TEXT("the bigger the body, the more shots it takes and the more it scores"));
  Check(HeadShot(EKind::Pale,1.9)&&!HeadShot(EKind::Pale,1.2)&&!HeadShot(EKind::Crab,.9)&&ShotDamage(true)==2*ShotDamage(false),TEXT("a round in the head counts double; the crab on all fours has no such place"));
  Check(FlankOf(0)==0&&FlankOf(1)>0&&FlankOf(2)<0&&FlankOf(3)==0&&FlankOf(-1)<0,TEXT("of every three that come, one comes straight on, one round the left and one round the right"));
 }
 {
  FGun G;
  int32 Shots=0;
  for(int32 I=0;I<400&&G.Mag>0;++I){if(G.Fire())++Shots;G.Step(ShotGap);}
  Check(Shots==MagSize&&G.Mag==0&&!G.Fire(),FString::Printf(TEXT("a magazine is %d shots, and an empty gun does not fire"),MagSize));
  FGun Quick;
  const bool bFirst=Quick.Fire(),bAtOnce=Quick.Fire();
  Quick.Step(ShotGap*.5);
  const bool bHalf=Quick.Fire();
  Quick.Step(ShotGap*.5);
  Check(bFirst&&!bAtOnce&&!bHalf&&Quick.Fire(),TEXT("a shot waits out the gap after the one before"));
  FGun Full;
  Check(!Full.StartReload(),TEXT("a full magazine is not changed"));
  const bool bBegan=G.StartReload(),bTwice=G.StartReload();
  const bool bEarly=G.Step(ReloadSeconds*.5);
  const bool bDuring=G.Fire();
  const double Half=G.ReloadFrac();
  const bool bDone=G.Step(ReloadSeconds*.5+.001);
  Check(bBegan&&!bTwice&&!bEarly&&!bDuring&&bDone&&G.Mag==MagSize&&G.Reserve==StartReserve-MagSize&&Half>.45&&Half<.55&&G.ReloadFrac()==0&&G.Fire(),
   TEXT("a reload takes its time, nothing fires during it, and the magazine is filled from what is carried"));
  FGun Last;
  Last.Mag=0;Last.Reserve=7;
  const bool bLast=Last.StartReload();
  Last.Step(ReloadSeconds+.01);
  FGun None;
  None.Mag=3;None.Reserve=0;
  Check(bLast&&Last.Mag==7&&Last.Reserve==0&&!None.StartReload(),TEXT("the last rounds carried make a short magazine; with nothing carried there is no reload"));
  None.Supply();
  FGun Rich;
  Rich.Reserve=MostReserve-5;
  Rich.Supply();
  Check(None.Reserve==CrateAmmo&&Rich.Reserve==MostReserve,TEXT("a crate gives rounds to carry, never more than can be carried"));
  bool bOrder=true;
  for(int32 I=1;I<int32(EBeat::Num);++I)bOrder&=BeatAt(EBeat(I))>BeatAt(EBeat(I-1));
  Check(bOrder&&BeatOf(0)==EBeat::Lower&&BeatOf(.2)==EBeat::MagOut&&BeatOf(.6)==EBeat::MagIn&&BeatOf(.8)==EBeat::BoltBack&&BeatOf(.9)==EBeat::BoltHome&&BeatOf(.99)==EBeat::Raise&&BeatAt(EBeat::Raise)<1,
   TEXT("the reload goes in its beats: lowered, magazine out, magazine in, bolt back, bolt home, raised, all before it is done"));
 }
 {
  FSpread S;
  const double HipStill=S.Deg(0,0),AdsStill=S.Deg(1,0),HipWalk=S.Deg(0,1);
  for(int32 I=0;I<60;++I)S.Shot(0);
  const double HipHot=S.Deg(0,0);
  S.Step(.05);
  const double Held=S.Deg(0,0);
  for(int32 I=0;I<120;++I)S.Step(1/60.);
  Check(AdsStill<HipStill*.2&&HipWalk>HipStill&&HipHot>HipStill+2&&HipHot<HipStill+3.3&&Held==HipHot&&FMath::IsNearlyEqual(S.Deg(0,0),HipStill,1e-9),
   FString::Printf(TEXT("the shots go wider from the hip than down the sights, wider walking, wider with fire up to a cap, and narrow again when the fire stops (%.2f, %.2f, %.2f degrees)"),AdsStill,HipStill,HipHot));
  FSpring K;
  K.Kick(3.,13.);
  double Peak=0,Back=0;
  for(int32 I=0;I<240;++I){K.Step(1/120.,13.,.62);Peak=FMath::Max(Peak,K.X);Back=FMath::Min(Back,K.X);}
  Check(Peak>1.8&&Peak<4.2&&Back>-1.&&FMath::Abs(K.X)<.01&&FMath::Abs(K.V)<.5,FString::Printf(TEXT("a kick throws the gun about as far as it says and it comes back to rest (asked 3.0, went %.2f)"),Peak));
  FSpring Slow;
  Slow.Kick(1.,5.);
  Slow.Step(10.,5.,.5);
  Check(FMath::IsFinite(Slow.X)&&FMath::Abs(Slow.X)<1.5,TEXT("one very long frame does not throw a spring away"));
 }
 {
  Check(Hurt(100,12)==88&&Hurt(5,12)==0&&Hurt(40,-9)==40&&Hurt(0,12)==0,TEXT("a touch takes its damage, never below nothing, and nothing heals by a negative one"));
  FRound R;
  R.Begin();
  const bool bWaits=R.Phase==EPhase::Banner&&!R.MaySpawn();
  R.Step(BannerSeconds+.01);
  bool bFight=R.Phase==EPhase::Fight&&R.MaySpawn()&&R.Left()==WaveSize(1);
  int32 Most=0;
  while(R.MaySpawn()){R.Spawned();Most=FMath::Max(Most,R.Out);}
  bFight&=Most==WaveAtOnce(1)&&!R.MaySpawn();
  Check(bWaits&&bFight,TEXT("a round begins with a banner, then wave 1 comes out, never more at once than the wave allows"));
  for(int32 I=0;I<3;++I)R.Touched();
  const int32 Before=R.Health;
  int32 Guard=0,Want=0,Nth=0;
  while(R.Wave==1&&Guard++<200){if(R.Out==0)R.Spawned();Want+=ScoreOf(EKind::Small)*StreakTimes(Nth++);R.Killed(EKind::Small);}
  Check(R.Wave==2&&R.Phase==EPhase::Banner&&R.Kills==WaveSize(1)&&R.ToCome==WaveSize(2)&&R.Out==0&&R.Health==FMath::Min(StartHealth,Before+WaveHeal)&&R.Score==Want&&R.bCrate&&R.Streak==WaveSize(1),
   TEXT("the last one of a wave down: the next wave is announced, a little health comes back, a crate is marked, and the score counts each kill by the streak"));
  const int32 AtBanner=R.Health;
  R.Touched();
  Check(R.Health==AtBanner,TEXT("nothing hurts during the banner between waves"));
  const bool bTook=R.TakeCrate(),bAgain=R.TakeCrate();
  Check(bTook&&!bAgain&&R.Health==FMath::Min(StartHealth,AtBanner+CrateHeal)&&!R.bCrate,TEXT("the crate is taken once, and gives health back"));
  Check(StreakTimes(0)==1&&StreakTimes(2)==1&&StreakTimes(3)==2&&StreakTimes(11)==4&&StreakTimes(99)==5&&StreakTimes(-4)==1,TEXT("every three kills in a row is one more times the score, to five"));
  R.Step(StreakGap+.01);
  const bool bCold=R.Streak==0;
  FRound Hot;
  Hot.Begin();Hot.Step(BannerSeconds+.01);Hot.Spawned();Hot.Spawned();
  const int32 Body=Hot.Killed(EKind::Pale),Head=Hot.Killed(EKind::Pale,true);
  const bool bRan=Hot.Streak==2;
  Hot.Spawned();Hot.Touched();
  Check(bCold&&Body==ScoreOf(EKind::Pale)&&Head==ScoreOf(EKind::Pale)*3/2&&Hot.Heads==1&&bRan&&Hot.Streak==0&&Hot.BestStreak==2,TEXT("a streak ends when too long passes or the player is touched; a kill by the head scores half as much again"));
  int32 Touches=0;
  while(R.Phase==EPhase::Fight&&Touches<100){R.Touched();++Touches;}
  const bool bCard=R.Phase==EPhase::Dead&&R.Health==0&&!R.Step(CardSeconds*.5)&&!R.TakeCrate();
  Check(bCard&&R.Step(CardSeconds*.5+.01)&&R.Wave==2,TEXT("at no health the round is over: the card says the wave reached and stays its time, then a new round begins"));
  FRound L;
  L.Begin();L.Step(BannerSeconds+.01);L.Spawned();
  const int32 Was=L.Left();
  L.Lost();
  Check(L.Left()==Was&&L.Out==0&&L.Kills==0,TEXT("a body that went with its world is not a kill: it comes again"));
 }
 {
  // The trigger held for ten seconds at sixty frames a second: the muzzle's light, in any one second.
  FGun G;
  TArray<double> Flashes;
  double Last=-10;
  int32 CalmFlashes=0,Fired=0;
  for(int32 Frame=0;Frame<600;++Frame)
  {
   const double Now=Frame/60.;
   if(G.Mag<=0)G.Mag=MagSize;
   if(G.Fire())
   {
    ++Fired;
    if(FlashMay(Now,Last,false)){Last=Now;Flashes.Add(Now);}
    if(FlashMay(Now,-10,true))++CalmFlashes;
   }
   G.Step(1/60.);
  }
  int32 Worst=0;
  for(int32 I=0;I<Flashes.Num();++I){int32 N=0;for(int32 K=I;K<Flashes.Num()&&Flashes[K]<Flashes[I]+1.;++K)++N;Worst=FMath::Max(Worst,N);}
  Check(Fired>60&&Flashes.Num()>10&&Worst<=3,FString::Printf(TEXT("the muzzle's light comes at most three times in any second with the trigger held (%d rounds, %d at most)"),Fired,Worst));
  Check(CalmFlashes==0,TEXT("with Calm flashes chosen the muzzle gives no light at all"));
 }
 {
  const FVector Eye(0,1.62,0),North(0,0,-1);
  const double Hit=RayBody(Eye,(FVector(0,.7,-10)-Eye).GetSafeNormal(),FVector(0,0,-10),1.2,.32);
  Check(Hit>9.6&&Hit<10.1,FString::Printf(TEXT("a shot aimed at the chest of a small one ten metres off strikes it (at %.2f m)"),Hit));
  Check(RayBody(Eye,(FVector(.8,.7,-10)-Eye).GetSafeNormal(),FVector(0,0,-10),1.2,.32)<0,TEXT("a shot past its side misses"));
  Check(RayBody(Eye,North,FVector(0,0,-10),1.2,.32)<0,TEXT("a level shot from the eye goes over a small one's head"));
  Check(RayBody(Eye,North,FVector(0,0,10),2.,.4)<0,TEXT("one behind the player is not struck"));
  Check(RayBody(Eye,FVector(0,.05,-1).GetSafeNormal(),FVector(0,0,-10),2.5,.4)>0,TEXT("a shot a little high still strikes a tall one"));
 }
 {
  // What is given back is what was kept, whatever happened in between.
  FStand Kept;
  Kept.Pos=FVector(-3.25,0,-29);Kept.FloorY=.4;Kept.Yaw=1.1;Kept.Pitch=-.2;Kept.bCrouch=true;
  FStand Cur=Kept;
  Cur.Pos=FVector(7,0,19);Cur.FloorY=0;Cur.Yaw=UE_DOUBLE_PI;Cur.Pitch=.5;Cur.bCrouch=false;
  const bool bMoved=!(Cur==Kept);
  Cur=Kept;
  Check(bMoved&&Cur==Kept&&Cur.Pos==FVector(-3.25,0,-29)&&Cur.bCrouch,TEXT("stopping gives back the place, the floor, the look and the crouch the player had"));
  FRound Fresh;
  Check(Fresh.Phase==EPhase::Off&&!Fresh.MaySpawn()&&!Fresh.TakeCrate(),TEXT("a round not begun is off: nothing comes out, and nothing is held"));
 }
 return bOk;
}

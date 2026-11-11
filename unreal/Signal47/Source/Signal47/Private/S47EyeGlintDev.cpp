#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47EventPlaces.h"
#include "S47EyeGlint.h"
#include "S47Flashlight.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47OpenLand.h"
#include "S47PaleBody.h"
#include "S47RoswellMotion.h"
#include "S47Sky.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47WorldLife.h"
#include "Components/BoxComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/Ticker.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47EyeGlintDev, Log, All);

// F10 "Eye glint" (ALIEN-EYES, Tom 9 November: "alle aliens må ha øyeblink"). Each alien body in turn out on the open land west of the
// highway, facing you, no flashlight: at 23:00 at 5, 15, 40 and 55 m, and by day; then each close up by day with the dots held on,
// to see that they sit in the black of the eyes. Each line counts the body's own dots that are drawn, against what Tom set: two in
// the dark within 50 m, none past it, none by day, none with the switch off, and never a light. Nothing here is saved.
// -S47DevSweep=/tmp/dir "-S47DevSweepGroup=Eye glint" runs the group in turn; the lines' own pictures are eye-<body>-<view>.png.
namespace S47EyeGlintDevLocal
{
 struct FKind { const TCHAR* Name; const TCHAR* File; int32 Style; }; // Style: 0 upright, 1 tall, 2 on all fours, 3 small Roswell
 const FKind Kinds[] = { { TEXT("Pale One"), TEXT("paleone"), 0 }, { TEXT("Tall One"), TEXT("tallone"), 1 }, { TEXT("Crab"), TEXT("crab"), 2 },
  { TEXT("Roswell 2"), TEXT("roswell2"), 3 }, { TEXT("Old Roswell"), TEXT("roswell"), 3 }, { TEXT("Old Pale (Story)"), TEXT("pale"), 0 } };
 constexpr int32 KindCount = UE_ARRAY_COUNT(Kinds);
 const TCHAR* AssetOf(int32 K)
 {
  switch (K)
  {
  case 0: return S47KindBody::Pale;
  case 1: return S47KindBody::Tall;
  case 2: return S47KindBody::Crab;
  case 3: return S47KindBody::Roswell;
  case 4: return S47KindBody::OldRoswell;
  default: return S47KindBody::Old;
  }
 }

 // The bodies, built once each on the web actor's root, and the one standing now, posed every frame.
 struct FEyeFixture
 {
  FS47PaleBody Bodies[KindCount];
  FS47RoswellMotion Roswell[KindCount];
  int32 Active = -1;
  FVector Feet = FVector::ZeroVector;
  double Yaw = 0, Clock = 0;
  TWeakObjectPtr<US47Main> M;

  void Pose(int32 K, bool bShow, double Dt)
  {
   FS47PaleBody& B = Bodies[K];
   if (!B.bBuilt || !B.Creature.Ready()) return;
   const int32 Style = Kinds[K].Style;
   if (Style == 2) { B.ApplyCrab(bShow, Feet, Yaw, Yaw, Clock, 0., false); return; }
   if (Style == 3)
   {
    UBoxComponent* R = B.RootComp.Get();
    if (!R) return;
    R->SetVisibility(bShow, true);
    if (!bShow) return;
    R->SetWorldLocationAndRotation(S47Three::ToUE(Feet), FRotator(0, FMath::RadiansToDegrees(Yaw), 0));
    FS47RoswellPose P; P.Mode = ES47RoswellMotion::Idle;
    Roswell[K].Apply(B.Creature, P, Dt);
    return;
   }
   B.Apply(bShow, Feet, Yaw, Yaw, 0, 0, Clock, FS47PaleBody::EPose::Walk);
  }
  void Hide() { for (int32 K = 0; K < KindCount; ++K) Pose(K, false, 0); Active = -1; }
  void Step(double Dt)
  {
   const US47Main* Main = M.Get();
   if (Active < 0 || !Main || Main->Mode != US47Main::EMode::Play) return;
   // A body whose skin has gone with its world (a new game) is let go, not posed.
   if (!Bodies[Active].Creature.Ready()) { Active = -1; return; }
   Clock += Dt; Pose(Active, true, FMath::Min(Dt, .1));
  }
  // The dots of the body standing now: how many are drawn, and where between them (three metres).
  int32 Shown(FVector* Mid = nullptr) const
  {
   if (Active < 0) return 0;
   int32 N = 0; FVector Sum = FVector::ZeroVector; int32 All = 0;
   for (const auto& Weak : Bodies[Active].Creature.Glints) if (const UStaticMeshComponent* G = Weak.Get())
   {
    Sum += S47Three::ToThree(G->GetComponentLocation()); ++All;
    if (G->GetVisibleFlag() && !G->bHiddenInGame) ++N;
   }
   if (Mid && All) *Mid = Sum / All;
   return N;
  }
 };
}

void FS47DevFreeform::AddEyeGlint(FS47DevEvents& E)
{
 using namespace S47EyeGlintDevLocal;
 static const TCHAR* const Group = TEXT("Eye glint");
 TSharedRef<FEyeFixture> Fx = MakeShared<FEyeFixture>();
 Fx->M = M;
 {
  TWeakPtr<FEyeFixture> Weak = Fx;
  FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak](float Dt)
  {
   const TSharedPtr<FEyeFixture> F = Weak.Pin();
   if (!F) return false;
   F->Step(Dt);
   return true;
  }));
 }
 EyeGlintClear = [this, Fx]
 {
  Fx->Hide();
  S47EyeGlint::bTestAlways = S47EyeGlint::bTestStill = false;
  if (IConsoleVariable* Cv = IConsoleManager::Get().FindConsoleVariable(TEXT("s47.EyeGlint"))) Cv->Set(1);
  if (M && M->Player) M->Player->Eye = 1.62;
 };
 const auto Daylight = [this] { return M && M->Life && M->Life->Sky.IsValid() ? M->Life->Sky->Daylight() : -1.; };
 const auto Shot = [this](const FString& Name) { if (!SweepDir.IsEmpty()) FScreenshotRequest::RequestScreenshot(SweepDir / Name + TEXT(".png"), false, false); };
 // Out on the open land at an hour; the body Metres ahead of you on its own ground, facing you; you looking at its eyes. Then.
 // Flags, set after Go has cleared the last line: 1 the dots held still, 2 shown by day too, 4 the switch off.
 const auto Stand = [this, Fx](int32 K, double Hour, double Metres, int32 Flags, TFunction<void()> Then)
 {
  FS47FreeformSession* Fs = Session(); if (!Fs) { Tell(false, TEXT("no session")); return; }
  const FS47EventPlace* Wash = Fs->EventPlaces ? Fs->EventPlaces->Find(TEXT("land:wash_west")) : nullptr;
  if (!Wash) { Tell(false, TEXT("no open land on the list")); return; }
  const FString Area = FS47EventPlaces::AreaOf(*Wash);
  const FVector Stood = S47OpenLand::FromRoad(Area, Wash->X, Wash->Z);
  FStage S; S.From = Hour; S.To = Hour + .4; S.Area = Area; S.Where = [Stood] { return FVector2D(Stood.X, Stood.Z); };
  S.Yaw = S47OpenLand::Turned(TEXT("road"), Area, PI); // looking south along the wash
  Go(S, [this, Fx, K, Metres, Flags, Then]
  {
   if (!M->Web || !M->Player) { Tell(false, TEXT("no player")); return; }
   S47EyeGlint::bTestStill = (Flags & 1) != 0; S47EyeGlint::bTestAlways = (Flags & 2) != 0;
   if (Flags & 4) if (IConsoleVariable* Cv = IConsoleManager::Get().FindConsoleVariable(TEXT("s47.EyeGlint"))) Cv->Set(0);
   if (M->Night && M->Night->Flashlight) M->Night->Flashlight->bOn = false;
   FS47PaleBody& B = Fx->Bodies[K];
   if (!B.bBuilt)
   {
    B.CreatureAsset = AssetOf(K);
    if (Kinds[K].Style == 1) B.Style = ES47PaleStyle::Tall;
    if (!B.Build(M->Web.Get(), TEXT("#root"), *FString::Printf(TEXT("EyeGlintDev%d"), K))) { Tell(false, TEXT("this body could not be built here")); return; }
   }
   if (!B.Creature.Ready()) { Tell(false, TEXT("this body needs the private skinned model")); return; }
   const FVector P(M->Player->Pos.X, M->Player->FloorY, M->Player->Pos.Z);
   const double Yaw = M->Player->Yaw;
   const FVector2D C(P.X - FMath::Sin(Yaw) * Metres, P.Z - FMath::Cos(Yaw) * Metres);
   FCollisionQueryParams Under(SCENE_QUERY_STAT(S47EyeGlintGround), true);
   for (const auto& Part : B.All) if (const auto* Own = Part.Get()) Under.AddIgnoredComponent(Own);
   FHitResult Ground; double G = P.Y;
   if (M->Web->GetWorld()->LineTraceSingleByChannel(Ground, S47Three::ToUE(C.X, P.Y + 15., C.Y), S47Three::ToUE(C.X, P.Y - 15., C.Y), ECC_Visibility, Under))
    G = S47Three::ToThree(Ground.ImpactPoint).Y;
   Fx->Hide();
   Fx->Active = K; Fx->Clock = 0; Fx->Feet = FVector(C.X, G, C.Y); Fx->Yaw = FMath::Atan2(P.Z - C.Y, P.X - C.X);
   Fx->Roswell[K].Reset();
   // A moment for its pose, then the eyes are where it stands now.
   After(.5, [this, Fx, Then]
   {
    FVector Mid = FVector::ZeroVector; Fx->Shown(&Mid); if (!Mid.IsZero()) AimAt(Mid);
    After(.9, Then);
   });
  });
 };
 const auto Worn = [Fx](int32 K)
 {
  const auto* Mesh = Fx->Bodies[K].Creature.Mesh.Get();
  return Mesh && Mesh->GetSkinnedAsset() ? Mesh->GetSkinnedAsset()->GetName() : FString(TEXT("no skin"));
 };

 // ---- in the dark: 5, 15 and 40 m show both dots, 55 m none ----
 for (int32 K = 0; K < KindCount; ++K)
  for (const double Metres : { 5., 15., 40., 55. })
  {
   const bool bWant = Metres < S47EyeGlint::GoneAt;
   E.AddFreeform(Group, FString::Printf(TEXT("%s at %.0f m, night"), Kinds[K].Name, Metres),
    FString::Printf(TEXT("23:00 on the open land, no flashlight, the body %.0f m ahead facing you: %s"), Metres, bWant ? TEXT("two faint dots, nothing lit round them") : TEXT("past 50 m, no dots")),
    [this, Fx, Stand, Shot, Daylight, Worn, K, Metres, bWant]
   {
    Stand(K, 23, Metres, 1, [this, Fx, Shot, Daylight, Worn, K, Metres, bWant]
    {
     Shot(FString::Printf(TEXT("eye-%s-night-%02.0fm"), Kinds[K].File, Metres));
     After(.3, [this, Fx, Daylight, Worn, K, Metres, bWant]
     {
      const int32 N = Fx->Shown(); FString Why; const bool bDark = S47EyeGlint::GivesNoLight(&Why);
      const double Day = Daylight();
      const FString Line = FString::Printf(TEXT("%s (%s) at %.0f m, 23:00, daylight %.3f: %d dots drawn (want %d), level %.2f%s"), Kinds[K].Name, *Worn(K), Metres, Day, N,
       bWant ? 2 : 0, S47EyeGlint::Level(Day, Metres), bDark ? TEXT("") : *(TEXT("; ") + Why));
      UE_LOG(LogS47EyeGlintDev, Display, TEXT("S47: eye glint %s"), *Line);
      Tell(N == (bWant ? 2 : 0) && bDark, Line, 8);
     });
    });
   });
  }
 // ---- the switch, and the dots alive ----
 E.AddFreeform(Group, TEXT("Switched off: s47.EyeGlint 0"), TEXT("23:00, the Pale One at 5 m with the console variable at 0: no dots"), [this, Fx, Stand, Shot]
 {
  Stand(0, 23, 5, 5, [this, Fx, Shot]
  {
   Shot(TEXT("eye-off-night-05m"));
   After(.3, [this, Fx]
   {
    const int32 N = Fx->Shown();
    Tell(N == 0 && !S47EyeGlint::Enabled(), FString::Printf(TEXT("Switched off: %d dots drawn at 5 m in the dark (want 0)"), N), 8);
   });
  });
 });
 E.AddFreeform(Group, TEXT("Drift and blink, Pale One at 3 m"), TEXT("23:00, 3 m: 14 s watching the dots move slowly in the eyes and go out now and then; the log has how far and how often"), [this, Fx, Stand]
 {
  Stand(0, 23, 3, 0, [this, Fx]
  {
   struct FSeen { double Started = -1, Most = 0; int32 Out = 0; bool bWasOut = false; TMap<const UStaticMeshComponent*, FVector> Rest; };
   TSharedRef<FSeen> Seen = MakeShared<FSeen>();
   WaitFor([this, Fx, Seen]
   {
    const double Now = M->Web->GetWorld()->GetTimeSeconds();
    if (Seen->Started < 0) Seen->Started = Now;
    if (Fx->Active < 0) return true;
    bool bAnyOut = false;
    for (const auto& Weak : Fx->Bodies[Fx->Active].Creature.Glints) if (const UStaticMeshComponent* G = Weak.Get())
    {
     const FVector* R = Seen->Rest.Find(G);
     if (!R) R = &Seen->Rest.Add(G, G->GetRelativeLocation());
     // In centimetres of the world, through the head bone's own scale (100 on the Tripo skins): its units are metres there,
     // and the drift that threw the dots 40 cm out of the head read here as .4.
     const USceneComponent* Head = G->GetAttachParent();
     const FTransform Bone = Head ? Head->GetSocketTransform(G->GetAttachSocketName()) : FTransform::Identity;
     Seen->Most = FMath::Max(Seen->Most, Bone.TransformVector(G->GetRelativeLocation() - *R).Size());
     bAnyOut |= G->bHiddenInGame;
    }
    if (bAnyOut && !Seen->bWasOut) ++Seen->Out;
    Seen->bWasOut = bAnyOut;
    return Now - Seen->Started > 14.;
   }, 30., [this, Seen]
   {
    const FString Line = FString::Printf(TEXT("Drift and blink over 14 s: the dots moved up to %.2f cm in the eye, went out %d times"), Seen->Most, Seen->Out);
    UE_LOG(LogS47EyeGlintDev, Display, TEXT("S47: eye glint %s"), *Line);
    Tell(Seen->Most > .1 && Seen->Most < 1.5, Line, 8);
   }, [this] { Tell(false, TEXT("the watch did not finish")); });
  });
 });
 // ---- by day: none ----
 for (int32 K = 0; K < KindCount; ++K)
  E.AddFreeform(Group, FString::Printf(TEXT("%s at 15 m, day"), Kinds[K].Name), TEXT("11:00 on the open land, the body 15 m ahead: no dots by day"), [this, Fx, Stand, Shot, Daylight, Worn, K]
  {
   Stand(K, 11, 15, 1, [this, Fx, Shot, Daylight, Worn, K]
   {
    Shot(FString::Printf(TEXT("eye-%s-day-15m"), Kinds[K].File));
    After(.3, [this, Fx, Daylight, Worn, K]
    {
     const int32 N = Fx->Shown();
     const FString Line = FString::Printf(TEXT("%s (%s) at 15 m, 11:00, daylight %.3f: %d dots drawn (want 0)"), Kinds[K].Name, *Worn(K), Daylight(), N);
     UE_LOG(LogS47EyeGlintDev, Display, TEXT("S47: eye glint %s"), *Line);
     Tell(N == 0, Line, 8);
    });
   });
  });
 // ---- close by day with the dots held on: do they sit in the black? ----
 for (int32 K = 0; K < KindCount; ++K)
  E.AddFreeform(Group, FString::Printf(TEXT("%s close, day, dots held on"), Kinds[K].Name), TEXT("11:00, half a metre from the face with the dots shown by day and held still: each should sit in the black of its eye"), [this, Fx, Stand, Shot, Worn, K]
  {
   Stand(K, 11, 3, 3, [this, Fx, Shot, Worn, K]
   {
    FVector Mid = FVector::ZeroVector;
    if (Fx->Shown(&Mid) != 2 || Mid.IsZero()) { Tell(false, FString::Printf(TEXT("%s: no dots to look at"), Kinds[K].Name)); return; }
    const FVector2D Out = FVector2D(M->Player->Pos.X - Mid.X, M->Player->Pos.Z - Mid.Z).GetSafeNormal();
    const double Away = Kinds[K].Style == 3 ? .45 : .6;
    M->Player->Place(Mid.X + Out.X * Away, Mid.Z + Out.Y * Away, M->Player->Yaw);
    M->Player->Eye = Mid.Y - M->Player->FloorY;
    AimAt(Mid);
    After(.6, [this, Fx, Shot, Worn, K]
    {
     Shot(FString::Printf(TEXT("eye-%s-close"), Kinds[K].File));
     After(.3, [this, Fx, Worn, K]
     {
      const FString Line = FString::Printf(TEXT("%s (%s) close by day: %d dots held on; see the picture for where they sit"), Kinds[K].Name, *Worn(K), Fx->Shown());
      UE_LOG(LogS47EyeGlintDev, Display, TEXT("S47: eye glint %s"), *Line);
      Tell(Fx->Shown() == 2, Line, 8);
     });
    });
   });
  });
}

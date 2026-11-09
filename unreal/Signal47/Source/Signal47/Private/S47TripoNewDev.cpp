#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47EventPlaces.h"
#include "S47EyeGlint.h"
#include "S47Flashlight.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47OpenLand.h"
#include "S47Sky.h"
#include "S47Three.h"
#include "S47TripoNew.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47WorldLife.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47TripoNewDev, Log, All);

// F10 "Tripo new" (TRIPO-NEW-20261108, Tom's nine Tripo models of 9 November): each model out on the open land west of the highway,
// by day at 11:00 and by night at 23:00 with no flashlight, a figure facing you, a vehicle side on; then all nine in a row, day and
// night; then the three aliens close by day with their eye dots held on. Without the private models (a build from Git alone) each
// line shows the stand-in, says so and passes: nothing here may fail for want of a licensed file. Nothing is saved.
// -S47DevSweep=/tmp/dir "-S47DevSweepGroup=Tripo new" runs the group; pictures are tripo-<id>-<view>.png.
namespace S47TripoNewDevLocal
{
 using S47TripoNew::FModel;
 using S47TripoNew::EKind;
 const TCHAR* const Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
 const TCHAR* const Lit = TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque");

 struct FShown
 {
  TWeakObjectPtr<UStaticMeshComponent> C;
  TArray<TWeakObjectPtr<UStaticMeshComponent>> Glints;
  // PT-NORDIC-COSMIC-20261109: the Cosmic one's light, still (as it is drawn here, without the breath)
  S47TripoNew::FCosmicGlow Cosmic;
  bool bOwn = false, bCube = false;
  int32 Model = -1;
 };
 struct FFixture
 {
  TArray<FShown> Now;
  void Clear()
  {
   for (FShown& S : Now)
   {
    for (const auto& G : S.Glints) if (UStaticMeshComponent* Gc = G.Get()) Gc->DestroyComponent();
    for (const auto& G : S.Cosmic.Stars) if (UMaterialBillboardComponent* Gc = G.Get()) Gc->DestroyComponent();
    if (UStaticMeshComponent* C = S.C.Get()) C->DestroyComponent();
   }
   Now.Reset();
  }
  int32 Dots() const
  {
   int32 N = 0;
   for (const FShown& S : Now) for (const auto& W : S.Glints) if (const UStaticMeshComponent* G = W.Get()) if (G->GetVisibleFlag() && !G->bHiddenInGame) ++N;
   return N;
  }
 };
 // Metres a model is seen from: the whole of it in view at the player's lens.
 double ViewM(const FModel& M)
 {
  switch (M.Kind)
  {
  case EKind::Vehicle: return M.SizeM * 1.6 + 2.;
  case EKind::Prop: return 1.4;
  default: return FMath::Max(4.5, M.SizeM * 2.6);
  }
 }
 // Its size as built: a figure's or the walkie's height, a vehicle's or the counter's longest side, metres.
 double SizeOf(const UStaticMeshComponent* C, const FModel& M)
 {
  const UStaticMesh* Mesh = C ? C->GetStaticMesh() : nullptr;
  if (!Mesh) return 0.;
  const FBox B = Mesh->GetBoundingBox();
  const FVector S = B.GetSize() * C->GetComponentScale() / 100.;
  return M.Kind == EKind::Vehicle || FString(M.Id) == TEXT("geiger") ? FMath::Max(S.X, S.Y) : S.Z;
 }
}

void FS47DevFreeform::AddTripoNew(FS47DevEvents& E)
{
 using namespace S47TripoNewDevLocal;
 static const TCHAR* const Group = TEXT("Tripo new");
 TSharedRef<FFixture> Fx = MakeShared<FFixture>();
 TripoNewClear = [this, Fx]
 {
  Fx->Clear();
  S47EyeGlint::bTestAlways = S47EyeGlint::bTestStill = false;
  if (M && M->Player) M->Player->Eye = 1.62;
 };
 const auto Daylight = [this] { return M && M->Life && M->Life->Sky.IsValid() ? M->Life->Sky->Daylight() : -1.; };
 const auto Shot = [this](const FString& Name) { if (!SweepDir.IsEmpty()) FScreenshotRequest::RequestScreenshot(SweepDir / Name + TEXT(".png"), false, false); };
 // One model drawn with its foot at Foot (Three metres), turned Yaw: its own if this build has it, else its stand-in, else a box.
 const auto Draw = [this, Fx](int32 K, const FVector& Foot, double Yaw) -> FShown*
 {
  if (!M->Web) return nullptr;
  AS47Web* W = M->Web.Get();
  const FModel& Mo = S47TripoNew::Models()[K];
  bool bOwn = false;
  const TCHAR* Path = S47TripoNew::Resolve(Mo, &bOwn);
  UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
  if (!Mesh) { Path = Cube; Mesh = LoadObject<UStaticMesh>(nullptr, Cube); bOwn = false; }
  if (!Mesh) return nullptr;
  const bool bCube = FString(Path) == Cube;
  auto* C = NewObject<UStaticMeshComponent>(W, MakeUniqueObjectName(W, UStaticMeshComponent::StaticClass(), FName(*FString::Printf(TEXT("S47TripoNewDev_%s"), Mo.Id))));
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(W->GetRootComponent()); C->SetStaticMesh(Mesh);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCanEverAffectNavigation(false); C->SetCastShadow(true);
  if (!bOwn && Mo.StandInColour)
   if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, Lit))
   {
    auto* Mat = UMaterialInstanceDynamic::Create(Base, W);
    Mat->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(Mo.StandInColour)); Mat->SetScalarParameterValue(TEXT("Roughness"), .8f);
    Mat->SetScalarParameterValue(TEXT("NativeLighting"), 1); Mat->SetScalarParameterValue(TEXT("EmissivePower"), 0);
    for (int32 I = 0; I < FMath::Max(1, C->GetNumMaterials()); ++I) C->SetMaterial(I, Mat);
   }
  C->RegisterComponent();
  // A box stands in at the model's size: a vehicle 2 m wide and its length long, a figure a third as wide as tall, centred on its foot.
  FVector Size(1);
  FVector Lift = FVector::ZeroVector;
  if (bCube)
  {
   const double S = Mo.SizeM;
   Size = Mo.Kind == EKind::Vehicle ? FVector(2.2, S * .42, S) : Mo.Kind == EKind::Prop ? FVector(S * .5, S, S * .4) : FVector(S * .32, S, S * .22);
   Lift = FVector(0, Size.Y / 2, 0);
  }
  double Ex[16]; S47Three::Compose(Foot + Lift, S47Three::Euler(0, Yaw, 0), Size, Ex);
  C->SetWorldTransform(S47Three::MatToUE(Ex));
  FShown Sh; Sh.C = C; Sh.bOwn = bOwn; Sh.bCube = bCube; Sh.Model = K;
  if (bOwn) S47EyeGlint::AddStaticIfKnown(W, C, Sh.Glints);
  if (bOwn) S47TripoNew::AddCosmicGlow(W, C, Sh.Cosmic);
  Fx->Now.Add(Sh);
  return &Fx->Now.Last();
 };
 // The ground under a point on the land (Three metres), the drawn models aside.
 const auto Ground = [this, Fx](double X, double Z, double Y0)
 {
  FCollisionQueryParams Under(SCENE_QUERY_STAT(S47TripoNewGround), true);
  for (const FShown& S : Fx->Now) if (const auto* C = S.C.Get()) Under.AddIgnoredComponent(C);
  FHitResult Hit;
  if (M->Web->GetWorld()->LineTraceSingleByChannel(Hit, S47Three::ToUE(X, Y0 + 15., Z), S47Three::ToUE(X, Y0 - 15., Z), ECC_Visibility, Under))
   return S47Three::ToThree(Hit.ImpactPoint).Y;
  return Y0;
 };
 // Out on the open land at an hour, no flashlight, looking south along the wash; then.
 const auto Out = [this, Fx](double Hour, int32 Flags, TFunction<void()> Then)
 {
  FS47FreeformSession* Fs = Session(); if (!Fs) { Tell(false, TEXT("no session")); return; }
  const FS47EventPlace* Wash = Fs->EventPlaces ? Fs->EventPlaces->Find(TEXT("land:wash_west")) : nullptr;
  if (!Wash) { Tell(false, TEXT("no open land on the list")); return; }
  const FString Area = FS47EventPlaces::AreaOf(*Wash);
  const FVector Stood = S47OpenLand::FromRoad(Area, Wash->X, Wash->Z);
  FStage S; S.From = Hour; S.To = Hour + .4; S.Area = Area; S.Where = [Stood] { return FVector2D(Stood.X, Stood.Z); };
  S.Yaw = S47OpenLand::Turned(TEXT("road"), Area, PI);
  Go(S, [this, Fx, Flags, Then]
  {
   if (!M->Web || !M->Player) { Tell(false, TEXT("no player")); return; }
   Fx->Clear();
   S47EyeGlint::bTestStill = (Flags & 1) != 0; S47EyeGlint::bTestAlways = (Flags & 2) != 0;
   if (M->Night && M->Night->Flashlight) M->Night->Flashlight->bOn = false;
   Then();
  });
 };
 // A point Metres ahead of the player on the land, and the yaw that turns a figure's face (+Z) to the player.
 const auto Ahead = [this, Ground](double Metres, double Side, FVector& Foot, double& Face)
 {
  const FVector P(M->Player->Pos.X, M->Player->FloorY, M->Player->Pos.Z);
  const double Yaw = M->Player->Yaw;
  const double X = P.X - FMath::Sin(Yaw) * Metres + FMath::Cos(Yaw) * Side, Z = P.Z - FMath::Cos(Yaw) * Metres - FMath::Sin(Yaw) * Side;
  Foot = FVector(X, Ground(X, Z, P.Y), Z);
  Face = FMath::Atan2(P.X - X, P.Z - Z);
 };
 const auto Report = [this, Fx, Daylight](const FString& What, bool bWantDots)
 {
  int32 Own = 0, Stand = 0; FString Sizes;
  for (const FShown& S : Fx->Now)
  {
   const FModel& Mo = S47TripoNew::Models()[S.Model];
   (S.bOwn ? Own : Stand) += 1;
   Sizes += FString::Printf(TEXT(" %s %s %.2fm"), Mo.Id, S.bOwn ? TEXT("own") : S.bCube ? TEXT("box") : TEXT("stand-in"), SizeOf(S.C.Get(), Mo));
  }
  bool bSized = true;
  for (const FShown& S : Fx->Now) if (S.bOwn)
  {
   const FModel& Mo = S47TripoNew::Models()[S.Model];
   bSized &= FMath::Abs(SizeOf(S.C.Get(), Mo) - Mo.SizeM) < Mo.SizeM * .08 + .02;
  }
  int32 Glinted = 0; for (const FShown& S : Fx->Now) Glinted += S.Glints.Num();
  const int32 Dots = Fx->Dots();
  const bool bDotsOk = bWantDots ? Dots == Glinted : Dots == 0;
  FString Why; const bool bDark = S47EyeGlint::GivesNoLight(&Why);
  const FString Line = FString::Printf(TEXT("%s, daylight %.3f: %d own, %d standing in;%s; eye dots %d of %d (want %s)%s%s"), *What, Daylight(), Own, Stand, *Sizes, Dots, Glinted,
   bWantDots ? TEXT("all") : TEXT("none"), bSized ? TEXT("") : TEXT("; a model is not its prepared size"), bDark ? TEXT("") : *(TEXT("; ") + Why));
  UE_LOG(LogS47TripoNewDev, Display, TEXT("S47: tripo new %s"), *Line);
  Tell(!Fx->Now.IsEmpty() && bSized && bDotsOk && bDark, Line, 8);
 };

 const int32 Count = S47TripoNew::Models().Num();
 // ---- each model by itself, day and night ----
 for (int32 K = 0; K < Count; ++K)
  for (const bool bNight : { false, true })
  {
   const FModel& Mo = S47TripoNew::Models()[K];
   E.AddFreeform(Group, FString::Printf(TEXT("%s, %s"), Mo.Label, bNight ? TEXT("night") : TEXT("day")),
    FString::Printf(TEXT("%s on the open land, %.0f m ahead%s; its own model, or what stands in for it"), bNight ? TEXT("23:00, no flashlight") : TEXT("11:00"), ViewM(Mo),
     Mo.Kind == EKind::Vehicle ? TEXT(", side on") : TEXT(", facing you")),
    [this, Out, Ahead, Draw, Report, Shot, K, bNight]
   {
    Out(bNight ? 23. : 11., bNight ? 1 : 0, [this, Ahead, Draw, Report, Shot, K, bNight]
    {
     const FModel& Mo = S47TripoNew::Models()[K];
     FVector Foot; double Face = 0;
     Ahead(ViewM(Mo), 0, Foot, Face);
     // A vehicle side on: its front (-Z) to the player's right.
     const double Yaw = Mo.Kind == EKind::Vehicle ? Face + PI / 2 : Face;
     if (!Draw(K, Foot, Yaw)) { Tell(false, TEXT("nothing could be drawn")); return; }
     const double Mid = Mo.Kind == EKind::Prop ? .1 : Mo.Kind == EKind::Vehicle ? .9 : Mo.SizeM * .55;
     AimAt(Foot + FVector(0, Mid, 0));
     After(1.2, [this, Report, Shot, K, bNight]
     {
      const FModel& Mo2 = S47TripoNew::Models()[K];
      Shot(FString::Printf(TEXT("tripo-%s-%s"), Mo2.Id, bNight ? TEXT("night") : TEXT("day")));
      After(.3, [Report, K, bNight]
      {
       const FModel& Mo3 = S47TripoNew::Models()[K];
       Report(FString::Printf(TEXT("%s, %s"), Mo3.Id, bNight ? TEXT("23:00") : TEXT("11:00")), bNight);
      });
     });
    });
   });
  }
 // ---- all nine in a row, day and night ----
 for (const bool bNight : { false, true })
  E.AddFreeform(Group, FString::Printf(TEXT("All nine in a row, %s"), bNight ? TEXT("night") : TEXT("day")),
   TEXT("The nine side by side 16 m ahead, small things in front: a contact sheet in the world"),
   [this, Out, Ahead, Draw, Report, Shot, Count, bNight]
  {
   Out(bNight ? 23. : 11., bNight ? 1 : 0, [this, Ahead, Draw, Report, Shot, Count, bNight]
   {
    double Side = -11.;
    FVector Mid = FVector::ZeroVector;
    for (int32 K = 0; K < Count; ++K)
    {
     const FModel& Mo = S47TripoNew::Models()[K];
     const bool bSmall = Mo.Kind == EKind::Prop;
     const double Wide = Mo.Kind == EKind::Vehicle ? 3.4 : bSmall ? 0. : 1.6;
     FVector Foot; double Face = 0;
     Ahead(bSmall ? 6. : 16., bSmall ? (FString(Mo.Id) == TEXT("walkie") ? -.5 : .5) : Side + Wide / 2, Foot, Face);
     if (!bSmall) Side += Wide + .4;
     Draw(K, Foot, Mo.Kind == EKind::Vehicle ? Face + PI / 2 : Face);
     if (K == 4) Mid = Foot;
    }
    AimAt(Mid + FVector(0, 1.2, 0));
    After(1.5, [this, Report, Shot, bNight]
    {
     Shot(FString::Printf(TEXT("tripo-all-%s"), bNight ? TEXT("night") : TEXT("day")));
     After(.3, [Report, bNight] { Report(FString::Printf(TEXT("all nine, %s"), bNight ? TEXT("23:00") : TEXT("11:00")), bNight); });
    });
   });
  });
 // ---- the aliens close by day, dots held on: in the black of the eyes? ----
 for (int32 K = 0; K < Count; ++K)
 {
  const FModel& Mo = S47TripoNew::Models()[K];
  if (Mo.Kind != EKind::Alien) continue;
  E.AddFreeform(Group, FString::Printf(TEXT("%s close, eye dots held on"), Mo.Id),
   TEXT("11:00, close to the face with the dots shown by day and held still: each should sit in the black of its eye"),
   [this, Fx, Out, Ahead, Draw, Shot, K]
  {
   Out(11., 3, [this, Fx, Ahead, Draw, Shot, K]
   {
    FVector Foot; double Face = 0;
    Ahead(4., 0, Foot, Face);
    FShown* S = Draw(K, Foot, Face);
    if (!S) { Tell(false, TEXT("nothing could be drawn")); return; }
    if (S->Glints.Num() != 2)
    {
     Tell(true, FString::Printf(TEXT("%s: %s here, no eyes to look at (the stand-in has none)"), S47TripoNew::Models()[K].Id, S->bOwn ? TEXT("own model without measured eyes") : TEXT("stand-in")), 8);
     return;
    }
    After(.3, [this, Fx, Shot, K]
    {
     FVector Mid = FVector::ZeroVector; int32 N = 0;
     for (const auto& G : Fx->Now[0].Glints) if (const UStaticMeshComponent* Gc = G.Get()) { Mid += S47Three::ToThree(Gc->GetComponentLocation()); ++N; }
     if (N != 2) { Tell(false, TEXT("the dots went")); return; }
     Mid /= 2;
     const FVector2D Away = FVector2D(M->Player->Pos.X - Mid.X, M->Player->Pos.Z - Mid.Z).GetSafeNormal();
     M->Player->Place(Mid.X + Away.X * .6, Mid.Z + Away.Y * .6, M->Player->Yaw);
     M->Player->Eye = Mid.Y - M->Player->FloorY;
     AimAt(Mid);
     After(.6, [this, Fx, Shot, K]
     {
      Shot(FString::Printf(TEXT("tripo-%s-eyes"), S47TripoNew::Models()[K].Id));
      After(.3, [this, Fx, K]
      {
       const FString Line = FString::Printf(TEXT("%s close by day: %d dots held on; see the picture for where they sit"), S47TripoNew::Models()[K].Id, Fx->Dots());
       UE_LOG(LogS47TripoNewDev, Display, TEXT("S47: tripo new %s"), *Line);
       Tell(Fx->Dots() == 2, Line, 8);
      });
     });
    });
   });
  });
 }
}

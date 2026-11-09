#include "S47DevFreeform.h"
#include "S47Blockers.h"
#include "S47DevEvents.h"
#include "S47Main.h"
#include "S47SaroClip.h"
#include "S47SaroFixes.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SaroFixesDev, Log, All);

// F10, "SARO fixes (T)" (Tom's test of d36, 9 November 2026): each of his SARO pictures seen again from where he stood, and each fix
// close up: the ramp and the walks by day and at dusk, the stop sign's corner, the hall from the corridor (the cabinet, the bar over
// the Operations door, the restroom door), the switches in the corridor, the records room and the control room, Dale's ceiling by
// day and at night, and the records room. Lines that measure write to the log (LogS47SaroFixesDev): what was built and why not,
// what stands at the walks' dark domes, every light in Dale's office with what is near it, what goes into the walls (S47SaroClip)
// and what stops one with nothing drawn (S47Blockers). Nothing here is saved.
// Before and after: the same sweep with -S47SaroFixesBefore (everything as in d36) and without.
// -S47DevSweep=/tmp/dir -S47DevSweepGroup="SARO fixes (T)" runs the group in turn, a picture after each line.
void FS47DevFreeform::AddSaroFixes(FS47DevEvents& E)
{
 static const TCHAR* const Group = TEXT("SARO fixes (T)");
 const auto Log = [](const TArray<FString>& Lines) { for (const FString& L : Lines) UE_LOG(LogS47SaroFixesDev, Display, TEXT("S47: SARO fixes %s"), *L); };
 // Stand at (X, Z) in SARO's terms at an hour and look at a point (three metres).
 const auto View = [this](double Hour, const FVector2D& Stand, const FVector& Look, const FString& Say)
 {
  FStage S; S.From = Hour; S.To = Hour + 1.2; S.Area = TEXT("saro"); S.Where = [Stand] { return Stand; };
  Go(S, [this, Look, Say]
  {
   if (!M->Player) { Tell(false, TEXT("no player")); return; }
   AimAt(Look);
   After(1.4, [this, Say] { Tell(true, Say, 8); });
  });
 };
 struct FView { const TCHAR* Label; double Hour; FVector2D Stand; FVector Look; const TCHAR* Say; };
 static const FView Views[] = {
  // Tom's first picture: on the walk west of the ramp, looking at the entrance.
  { TEXT("Ramp and walk: Tom's view of the entrance"), 11, FVector2D(-14.2, 5.7), FVector(-6.4, .6, 5.7), TEXT("PT-RAMP / PT-PATHS: the ramp in worn concrete, the walk's edge in the ground's own dust") },
  { TEXT("Ramp close up"), 11, FVector2D(-11.8, 7.2), FVector(-9.0, -.4, 5.7), TEXT("PT-RAMP: concrete, the nosing, the landing") },
  { TEXT("Ramp at dusk"), 19.4, FVector2D(-14.2, 5.7), FVector(-6.4, .6, 5.7), TEXT("PT-RAMP at dusk") },
  { TEXT("Walks: the branch to the lot"), 11, FVector2D(-11.6, 9.8), FVector(-11.6, -.6, 6.0), TEXT("PT-PATHS: joints, the slab's edge, the dust over it") },
  { TEXT("Walks: the edge close up"), 11, FVector2D(-14.0, 7.3), FVector(-14.0, -.6, 6.1), TEXT("PT-PATHS: the edge at the foot of the eye") },
  { TEXT("Walks: east of the lot"), 11, FVector2D(-8.2, 19.8), FVector(-8.2, -.6, 12.6), TEXT("PT-PATHS: the walk east of the visitors' lot") },
  { TEXT("Walks: to the annex's back"), 11, FVector2D(-6.0, 13.6), FVector(4.0, -.6, 12.2), TEXT("PT-PATHS: the walk along the annex") },
  // Tom's second picture: from the service road's mouth, looking at the highway.
  { TEXT("Stop sign: Tom's view of the corner"), 13.6, FVector2D(-15.2, 22.4), FVector(-26.0, .9, 21.2), TEXT("PT-STOP: the corner where the stop sign stood") },
  // Tom's third and fourth: the hall from the corridor's west, and the cabinet.
  { TEXT("Hall: Tom's view east"), 11, FVector2D(-2.4, 5.55), FVector(1.9, 1.2, 6.1), TEXT("PT-HALL: the cabinet out of the bay, no bar over the door, the restroom door's note") },
  { TEXT("Hall: the cabinet"), 11, FVector2D(-1.35, 5.25), FVector(-1.35, .85, 6.45), TEXT("PT-HALL: the service cabinet, no sign over it") },
  { TEXT("Hall: the bay and the Operations door"), 11, FVector2D(-.3, 5.35), FVector(.45, 2.0, 7.1), TEXT("PT-HALL: the bay, the door and the space over it") },
  { TEXT("Hall: the restroom door"), 11, FVector2D(.75, 5.75), FVector(1.96, 1.4, 5.7), TEXT("PT-HALL: the restroom door, locked with Dale's note") },
  // Tom's fifth: the switches.
  { TEXT("Switch: corridor"), 11, FVector2D(-3.1, 5.75), FVector(-3.15, 1.3, 4.83), TEXT("PT-SWITCH: one switch by the control room door") },
  { TEXT("Switch: records"), 11, FVector2D(-4.65, 7.75), FVector(-4.7, 1.3, 6.8), TEXT("PT-SWITCH: one switch in the records room") },
  { TEXT("Switch: control room"), 11, FVector2D(-2.4, 3.3), FVector(-2.4, 1.3, 4.48), TEXT("PT-SWITCH: the control room's side of the wall") },
  { TEXT("Dale's office: the ceiling by day"), 11, FVector2D(.5, 7.75), FVector(.47, 2.68, 9.0), TEXT("PT-DALE-LIGHT: the fitting the light comes from") },
  { TEXT("Dale's office: the ceiling at night"), 22.5, FVector2D(.5, 7.75), FVector(.47, 2.68, 9.0), TEXT("PT-DALE-LIGHT: the fitting lit at night") },
  { TEXT("Dale's office: the room at night"), 22.5, FVector2D(.6, 7.45), FVector(.4, 1.1, 10.3), TEXT("PT-DALE-LIGHT: the room under it") },
  { TEXT("Records: from the door"), 11, FVector2D(-3.9, 7.35), FVector(-4.2, 1.0, 10.2), TEXT("PT-RECORDS: the room from its door") },
  { TEXT("Records: the east end"), 11, FVector2D(-4.9, 9.7), FVector(-1.6, 1.0, 8.2), TEXT("PT-RECORDS: shelves, cabinets, the desk") },
  { TEXT("Records: the west end"), 11, FVector2D(-1.7, 9.7), FVector(-5.6, 1.0, 8.4), TEXT("PT-RECORDS: the other way") },
  { TEXT("Records: at night"), 22.5, FVector2D(-3.9, 7.35), FVector(-4.2, 1.0, 10.2), TEXT("PT-RECORDS: at night, the desk lamp") },
 };
 for (const FView& V : Views)
 {
  const FView Copy = V;
  E.AddFreeform(Group, Copy.Label, FString::Printf(TEXT("%.0f:%02.0f at SARO; a picture in a sweep"), FMath::FloorToDouble(Copy.Hour), FMath::Frac(Copy.Hour) * 60), [View, Copy] { View(Copy.Hour, Copy.Stand, Copy.Look, Copy.Say); });
 }

 // ---- what was built, and why a part waits ----
 E.AddFreeform(Group, TEXT("What was built"), TEXT("11:00 at SARO: each part's state; the list goes to the log"), [this, Log]
 {
  FStage S; S.From = 11; S.To = 12.5; S.Area = TEXT("saro"); S.Where = [] { return FVector2D(-2.4, 5.6); };
  Go(S, [this, Log]
  {
   After(2.5, [this, Log]
   {
    TArray<FString> Lines; S47SaroFixes::Report(M, Lines); S47RecordsDressing::Report(M, Lines);
    Log(Lines);
    int32 Stuck = 0; for (const FString& L : Lines) Stuck += L.Contains(TEXT("waiting")) ? 1 : 0;
    Tell(Stuck == 0, FString::Printf(TEXT("%d lines, %d waiting (the log has each)"), Lines.Num(), Stuck), 10);
   });
  });
 });

 // ---- what stands at a place: every drawn thing whose box takes in the point, smallest first ----
 const auto Here = [this, Log](const FString& Title, const TArray<FVector>& Points, double Radius, double MaxSize)
 {
  if (!M->Web) return;
  TArray<FString> Lines; Lines.Add(Title);
  TArray<UPrimitiveComponent*> All; M->Web->GetComponents(All);
  for (const FVector& P : Points)
  {
   const FBox Probe(S47Three::ToUE(P) - FVector(Radius * 100), S47Three::ToUE(P) + FVector(Radius * 100));
   TArray<TPair<double, FString>> Found;
   for (UPrimitiveComponent* C : All)
   {
    if (!C || !C->IsVisible() || C->bHiddenInGame) continue;
    const FBox B = C->Bounds.GetBox();
    if (!B.Intersect(Probe)) continue;
    const double Size = B.GetSize().GetMax() / 100;
    if (Size > MaxSize) continue;
    const UStaticMeshComponent* SM = Cast<UStaticMeshComponent>(C);
    const FVector Mid = S47Three::ToThree(B.GetCenter());
    Found.Add({ Size, FString::Printf(TEXT("    %.2f m %s (%s) at %.2f %.2f %.2f"), Size, *C->GetName(), SM && SM->GetStaticMesh() ? *SM->GetStaticMesh()->GetName() : *C->GetClass()->GetName(), Mid.X, Mid.Y, Mid.Z) });
   }
   Found.Sort([](const TPair<double, FString>& A, const TPair<double, FString>& B) { return A.Key < B.Key; });
   Lines.Add(FString::Printf(TEXT("  at %.2f %.2f %.2f: %d things"), P.X, P.Y, P.Z, Found.Num()));
   for (int32 K = 0; K < FMath::Min(25, Found.Num()); ++K) Lines.Add(Found[K].Value);
  }
  Log(Lines);
 };
 E.AddFreeform(Group, TEXT("What stands at the walks' domes"), TEXT("11:00, by the ramp's foot: every drawn thing under 3 m whose box takes in the domes Tom saw; the log has the list"), [this, Here]
 {
  FStage S; S.From = 11; S.To = 12.5; S.Area = TEXT("saro"); S.Where = [] { return FVector2D(-13.5, 7.0); };
  Go(S, [this, Here]
  {
   AimAt(FVector(-10.6, -.5, 5.7));
   After(1.5, [this, Here]
   {
    Here(TEXT("walk domes:"), { FVector(-10.7, -.55, 4.95), FVector(-10.7, -.55, 6.45), FVector(-11.0, -.55, 6.35), FVector(-12.2, -.55, 6.35), FVector(-11.6, -.55, 10.5) }, .35, 3);
    Tell(true, TEXT("the log has what stands at each"), 8);
   });
  });
 });
 E.AddFreeform(Group, TEXT("What stands over the Operations door"), TEXT("11:00, in the bay: every drawn thing under 3 m over the door and the bay; the log has the list"), [this, Here]
 {
  FStage S; S.From = 11; S.To = 12.5; S.Area = TEXT("saro"); S.Where = [] { return FVector2D(-.3, 5.4); };
  Go(S, [this, Here]
  {
   AimAt(FVector(.45, 2.0, 7.1));
   After(1.5, [this, Here]
   {
    Here(TEXT("over the Operations door:"), { FVector(.45, 2.1, 6.57), FVector(.1, 2.1, 6.9), FVector(.8, 2.1, 6.9), FVector(-.5, 1.0, 6.8), FVector(1.96, 1.4, 5.7) }, .2, 3);
    Tell(true, TEXT("the log has what stands at each"), 8);
   });
  });
 });
 E.AddFreeform(Group, TEXT("Lights in Dale's office"), TEXT("22:30 in the office: every light in it, on or off, and the drawn things within 25 cm of each; the log has the list"), [this, Here, Log]
 {
  FStage S; S.From = 22.5; S.To = 23.5; S.Area = TEXT("saro"); S.Where = [] { return FVector2D(.5, 7.75); };
  Go(S, [this, Here, Log]
  {
   AimAt(FVector(.47, 2.68, 9.0));
   After(1.5, [this, Here, Log]
   {
    TArray<ULightComponent*> Lights; if (M->Web) M->Web->GetComponents(Lights);
    TArray<FString> Lines; Lines.Add(TEXT("lights in Dale's office:")); TArray<FVector> At;
    for (ULightComponent* L : Lights)
    {
     if (!L) continue;
     const FVector P = S47Three::ToThree(L->GetComponentLocation());
     if (P.X < -.95 || P.X > 1.95 || P.Z < 7.0 || P.Z > 10.5 || P.Y < -.1 || P.Y > 3) continue;
     Lines.Add(FString::Printf(TEXT("  %s at %.2f %.2f %.2f: %s, %.1f %s"), *L->GetName(), P.X, P.Y, P.Z, L->IsVisible() ? TEXT("on") : TEXT("off"), L->Intensity, *L->GetClass()->GetName()));
     At.Add(P);
    }
    Log(Lines);
    Here(TEXT("near each light:"), At, .25, 3);
    Tell(At.Num() > 0, FString::Printf(TEXT("%d lights in the office (the log has each)"), At.Num()), 8);
   });
  });
 });
 E.AddFreeform(Group, TEXT("What goes into the walls: hall, records, office"), TEXT("11:00, from the corridor, the records room and the office: every drawn vertex further than 2 cm into a wall (S47SaroClip); the log has the list"), [this, Log]
 {
  FStage S; S.From = 11; S.To = 12.5; S.Area = TEXT("saro"); S.Where = [] { return FVector2D(-2.4, 5.6); };
  Go(S, [this, Log]
  {
   struct FPlace { const TCHAR* Name; double X, Z; };
   static const FPlace Places[] = { { TEXT("corridor"), -2.4, 5.6 }, { TEXT("records"), -3.5, 8.7 }, { TEXT("office"), .5, 8.5 } };
   // Each thing once, however many of the three places measure it (sweep of 9 November: two things, counted three times, were "6").
   TSharedRef<TSet<FString>> Count = MakeShared<TSet<FString>>();
   TSharedRef<TFunction<void(int32)>> Next = MakeShared<TFunction<void(int32)>>();
   *Next = [this, Log, Count, Next](int32 I)
   {
    if (I >= int32(UE_ARRAY_COUNT(Places)))
    {
     Tell(Count->Num() == 0, FString::Printf(TEXT("%d things in the walls of the hall, records and office%s (the log has each)"), Count->Num(), Count->Num() ? *(TEXT(": ") + FString::Join(Count->Array(), TEXT("; "))) : TEXT("")), 10);
     return;
    }
    M->Player->Place(Places[I].X, Places[I].Z, 0);
    After(.8, [this, Log, Count, Next, I]
    {
     TArray<FString> Lines; Lines.Add(FString::Printf(TEXT("walls, from the %s:"), Places[I].Name));
     // What the fixes placed: the hall's service cabinet, the parts made here (sarofix:) and the records room's own dressing
     // (S47RecordsDressing's nodes, records:...). Not every name with "records" in it: the records door's frame (S47SaroDoorDetail)
     // sits in its opening as a frame does, and S47Office's green face on the records partition runs to the export's records / office
     // partition (Annex.ts, x -1.0) over the 6 cm the clip table gives that wall; neither is a fix's, and both were in the log as
     // "6 things" on 9 November.
     for (const S47SaroClip::FHit& H : S47SaroClip::Run(M->Web->GetWorld(), .02, Lines))
      if (H.Thing.Contains(TEXT("office:serviceCabinet")) || H.Thing.Contains(TEXT("sarofix")) || H.Thing.Contains(TEXT("[records:")) || H.Thing.Contains(TEXT("S47Records")))
       Count->Add(H.Thing + TEXT(" into ") + H.Wall);
     Log(Lines);
     (*Next)(I + 1);
    });
   };
   (*Next)(0);
  });
 });
 E.AddFreeform(Group, TEXT("What stops one with nothing drawn, at SARO"), TEXT("11:00, SARO: the invisible blockers (S47Blockers); the log has the list"), [this, Log]
 {
  FStage S; S.From = 11; S.To = 12.5; S.Area = TEXT("saro"); S.Where = [] { return FVector2D(-13.0, 15.0); };
  Go(S, [this, Log]
  {
   After(1.0, [this, Log]
   {
    TArray<FString> Lines;
    const TArray<S47Blockers::FBlocker> Found = S47Blockers::Run(M, .35, Lines);
    Log(Lines);
    int32 Near = 0;
    for (const S47Blockers::FBlocker& B : Found) if (B.MaxX > -21 && B.MinX < 8 && B.MaxZ > 4 && B.MinZ < 23) ++Near;
    Tell(Near == 0, FString::Printf(TEXT("%d invisible blockers at SARO, %d by the walks, the ramp and the stop sign's corner (the log has each)"), Found.Num(), Near), 10);
   });
  });
 });
}

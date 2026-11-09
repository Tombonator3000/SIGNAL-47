#include "S47DevFreeform.h"
#include "S47Blockers.h"
#include "S47CalicheGround.h"
#include "S47DevEvents.h"
#include "S47FarmWorld.h"
#include "S47Main.h"
#include "S47SaroClip.h"
#include "S47StreetLamps.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47Three.h"
#include "CollisionQueryParams.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "S47World.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Looks, Log, All);

// F10, M6 (SARO-CLIP, DALE-WINDOW, FENCES-INVISIBLE, STREETLAMPS, 8 November): what goes into SARO's walls, measured from every place
// inside the buildings; what stops one where nothing is drawn, at SARO and on the road by the substation; how far each street lamp's
// foot stands from the ground; and the views before and after: Dale's window from both sides, the substation by day and night, the
// lamps by day and night. The measuring lines write their whole list to the log (LogS47Looks). Nothing here is saved.
// -S47DevSweep=/tmp/dir -S47DevSweepGroup="Looks (M6)" runs the group in turn, a picture after each line.
void FS47DevFreeform::AddLooks(FS47DevEvents& E)
{
 static const TCHAR* const Group = TEXT("Looks (M6)");
 const auto Log = [](const TArray<FString>& Lines) { for (const FString& L : Lines) UE_LOG(LogS47Looks, Display, TEXT("S47: M6 %s"), *L); };
 // Stand at a place given in the road's terms (as the relief probe's views are), at an hour, and look at another, Lift metres over the eye.
 const auto View = [this](const FString& Area, double Hour, const FVector2D& EyeRoad, const FVector& LookRoad, const FString& Say)
 {
  const FVector Eye = S47FarmWorld::RoadToArea(Area, FVector(EyeRoad.X, 0, EyeRoad.Y));
  const FVector Look = S47FarmWorld::RoadToArea(Area, FVector(LookRoad.X, 0, LookRoad.Z));
  FStage S; S.From = Hour; S.To = Hour + 1.5; S.Area = Area; S.Where = [Eye] { return FVector2D(Eye.X, Eye.Z); };
  Go(S, [this, Look, LookRoad, Say]
  {
   if (!M->Player) { Tell(false, TEXT("no player")); return; }
   AimAt(FVector(Look.X, M->Player->FloorY + M->Player->Eye + LookRoad.Y, Look.Z));
   After(1.2, [this, Say] { Tell(true, Say, 8); });
  });
 };
 // SARO's own terms to the road's.
 const auto Saro = [](double X, double Z) { return FVector2D(X + 24, Z - 300); };
 const auto Saro3 = [](double X, double Lift, double Z) { return FVector(X + 24, Lift, Z - 300); };

 // ---- SARO-CLIP: from every room in turn, since a room's things are drawn only while one is near them ----
 E.AddFreeform(Group, TEXT("SARO: things in the walls"), TEXT("11:00, from each room of the station in turn: every drawn vertex further than 2 cm into a wall; the list goes to the log"), [this, Log]
 {
  struct FPlace { const TCHAR* Name; double X, Z; };
  static const FPlace Places[] = { { TEXT("control room"), 0, 0 }, { TEXT("corridor"), -3, 5.7 }, { TEXT("records"), -3, 8.7 }, { TEXT("office"), .5, 8.5 },
   { TEXT("photo lab"), 16, -2.5 }, { TEXT("wing lab"), 23, 0 }, { TEXT("wing storage"), 28.8, -2.5 }, { TEXT("wing calibration"), 28.8, 3 }, { TEXT("garage"), 35, 0 } };
  FStage S; S.From = 11; S.To = 12.5; S.Area = TEXT("saro"); S.Where = [] { return FVector2D(0, 0); };
  Go(S, [this, Log]
  {
   TSharedRef<TMap<FString, S47SaroClip::FHit>> All = MakeShared<TMap<FString, S47SaroClip::FHit>>();
   TSharedRef<TFunction<void(int32)>> Next = MakeShared<TFunction<void(int32)>>();
   // (critic round 4: every run wrote some 400 MB of vertices; now only on -S47SaroClipDump)
   const FString Dir = FParse::Param(FCommandLine::Get(), TEXT("S47SaroClipDump")) ? (SweepDir.IsEmpty() ? FPaths::ProjectSavedDir() / TEXT("Looks") : SweepDir) / TEXT("saro-clip") : FString();
   *Next = [this, All, Next, Dir, Log](int32 I)
   {
    if (I >= int32(UE_ARRAY_COUNT(Places)))
    {
     TArray<S47SaroClip::FHit> Every; All->GenerateValueArray(Every);
     Every.Sort([](const S47SaroClip::FHit& A, const S47SaroClip::FHit& B) { return A.Depth > B.Depth; });
     // What is built into a wall by design is listed apart: the export's own walls, frames and sills (its unnamed batches), door and
     // window frames, trims, the room shells and partitions, the outside skin, floor grime that runs under a wall, weeds at its foot,
     // conduit through a partition, and REEL047's fire drawer, which stands in the wall until it is pulled out (archive047.ts).
     static const TCHAR* const BuiltIn[] = { TEXT("Frame"), TEXT("frame"), TEXT("Bead"), TEXT("Trim"), TEXT("Door"), TEXT("authoredRoom"), TEXT("Overlay"),
      TEXT("LivedSaroExterior"), TEXT("rampEntrance"), TEXT("AccessDressing"), TEXT("weeds"), TEXT("Conduit"), TEXT("Reel047"), TEXT("partition"), TEXT("Skirting"),
      // the photo lab's service entry at the floor and its safelight, recessed in the north wall (S47PhotoLabDetail)
      TEXT("UtilityDetail"), TEXT("SafelightHousing") };
     const auto IsBuiltIn = [](const FString& Thing)
     {
      for (const TCHAR* Word : BuiltIn) if (Thing.Contains(Word, ESearchCase::CaseSensitive)) return true;
      int32 At = Thing.Find(TEXT("[N")); // an unnamed node of the export: [N123]
      return At >= 0 && At + 2 < Thing.Len() && FChar::IsDigit(Thing[At + 2]);
     };
     TArray<S47SaroClip::FHit> Hits, Built;
     for (const auto& H : Every) (IsBuiltIn(H.Thing) ? Built : Hits).Add(H);
     TArray<FString> Lines; Lines.Add(FString::Printf(TEXT("SARO walls, all rooms: %d things into a wall deeper than 20 mm (and %d parts built into walls by design, listed after)"), Hits.Num(), Built.Num()));
     for (const auto& H : Hits) Lines.Add(FString::Printf(TEXT("  %.0f mm into %s: %s, %d vertices at x %.2f..%.2f y %.2f..%.2f z %.2f..%.2f"), H.Depth * 1000, *H.Wall, *H.Thing, H.Count,
      H.Where.Min.X, H.Where.Max.X, H.Where.Min.Y, H.Where.Max.Y, H.Where.Min.Z, H.Where.Max.Z));
     for (const auto& H : Built) Lines.Add(FString::Printf(TEXT("  built in: %.0f mm into %s: %s"), H.Depth * 1000, *H.Wall, *H.Thing));
     Log(Lines);
     FString Worst; for (int32 K = 0; K < FMath::Min(3, Hits.Num()); ++K) Worst += FString::Printf(TEXT("; %.0f mm %s in %s"), Hits[K].Depth * 1000, *Hits[K].Thing.Left(60), *Hits[K].Wall);
     Tell(Hits.IsEmpty(), FString::Printf(TEXT("%d things go into SARO's walls (the log has each)%s"), Hits.Num(), *Worst), 12);
     return;
    }
    M->Player->Place(Places[I].X, Places[I].Z, 0);
    After(.8, [this, All, Next, Dir, Log, I]
    {
     TArray<FString> Lines; Lines.Add(FString::Printf(TEXT("from the %s:"), Places[I].Name));
     for (const auto& H : S47SaroClip::Run(M->Web->GetWorld(), .02, Lines, Dir / FString::FromInt(I)))
     {
      const FString Key = H.Thing + TEXT("|") + H.Wall;
      if (S47SaroClip::FHit* Kept = All->Find(Key)) { if (H.Depth > Kept->Depth) *Kept = H; }
      else All->Add(Key, H);
     }
     Log(Lines);
     (*Next)(I + 1);
    });
   };
   (*Next)(0);
  });
 });

 // ---- FENCES-INVISIBLE: at SARO, and on the road by the substation (Caliche, the motel and the farm are the same walk) ----
 const auto Blockers = [this, Log](const FString& Area, const FVector2D& At, const FString& Where)
 {
  FStage S; S.From = 11; S.To = 12.5; S.Area = Area; S.Where = [At] { return At; };
  Go(S, [this, Log, Where]
  {
   After(1.0, [this, Log, Where]
   {
    TArray<FString> Lines;
    const TArray<S47Blockers::FBlocker> Found = S47Blockers::Run(M, .35, Lines);
    Log(Lines);
    FString First; for (int32 K = 0; K < FMath::Min(3, Found.Num()); ++K) First += FString::Printf(TEXT("; %s %s"), *Found[K].Kind, *Found[K].Id.Left(70));
    Tell(Found.IsEmpty(), FString::Printf(TEXT("%s: %d blockers with nothing drawn by them (the log has each)%s"), *Where, Found.Num(), *First), 12);
   });
  });
 };
 E.AddFreeform(Group, TEXT("Invisible blockers at SARO"), TEXT("11:00 in SARO's yard: every walk collider, drive obstacle and kept-out box here whose edge has nothing drawn by it"), [Blockers]
 { Blockers(TEXT("saro"), FVector2D(10, 14), TEXT("SARO")); });
 E.AddFreeform(Group, TEXT("Invisible blockers on Main Street"), TEXT("11:00 on Main Street: the same for Caliche, measured where it is drawn"), [Blockers]
 { Blockers(TEXT("diner"), FVector2D(8000 - 2, 2140), TEXT("Main Street")); });
 E.AddFreeform(Group, TEXT("Invisible blockers on the road"), TEXT("11:00 by the substation: the same for the road's walk, which holds Caliche, the motel, the substation and the farm"), [Blockers]
 { Blockers(TEXT("diner"), FVector2D(7896, 520), TEXT("the road")); });

 // ---- STREETLAMPS: each lamp's foot against the ground under it, where the lamps are drawn ----
 const auto Feet = [this, Log](const FString& Area, const FVector2D& At)
 {
  FStage S; S.From = 11; S.To = 12.5; S.Area = Area; S.Where = [At] { return At; };
  Go(S, [this, Log]
  {
   After(.8, [this, Log]
   {
    TArray<FString> Lines;
    const int32 Bad = S47StreetLamps::MeasureFeet(M->Web, Lines);
    Log(Lines);
    Tell(Bad == 0 && Lines.Num() > 1, Lines.Num() ? Lines[0] : FString(TEXT("no street lamps")), 12);
   });
  });
 };
 E.AddFreeform(Group, TEXT("Highway lamps: feet on the ground"), TEXT("11:00 on the highway by SARO: each lamp's foot measured against what is under it"), [Feet]
 { Feet(TEXT("saro"), FVector2D(-14, 36)); });
 E.AddFreeform(Group, TEXT("Caliche's lamps: feet on the ground"), TEXT("11:00 on Main Street: each lamp's foot measured against its footing and the sidewalk"), [Feet]
 { Feet(TEXT("diner"), FVector2D(8000 - 2, 2140)); });

 // ---- STREETLAMP-GLOW: each lamp's spot in its head under the bowl, and its bowl and halo lit exactly with it ----
 const auto Glow = [this, Log](const FString& Area, const FVector2D& At, double Hour, bool bNight)
 {
  FStage S; S.From = Hour; S.To = Hour + 1.5; S.Area = Area; S.Where = [At] { return At; };
  Go(S, [this, Log, bNight]
  {
   After(1.0, [this, Log, bNight]
   {
    TArray<FString> Lines; int32 Lit = 0;
    const int32 Bad = S47StreetLamps::MeasureGlow(M->Web, Lines, Lit);
    Log(Lines);
    Tell(Bad == 0 && Lines.Num() > 1 && (bNight ? Lit > 0 : Lit == 0), Lines.Num() ? Lines[0] : FString(TEXT("no street lamps")), 12);
   });
  });
 };
 E.AddFreeform(Group, TEXT("Highway lamps: glow at night"), TEXT("22:00 on the highway by SARO: each lamp's spot in its head, its bowl and halo lit with it (the log has each)"), [Glow]
 { Glow(TEXT("saro"), FVector2D(-14, 36), 22, true); });
 E.AddFreeform(Group, TEXT("Highway lamps: dark by day"), TEXT("11:00 on the highway by SARO: no bowl or halo lit"), [Glow]
 { Glow(TEXT("saro"), FVector2D(-14, 36), 11, false); });
 E.AddFreeform(Group, TEXT("Caliche's lamps: glow at night"), TEXT("22:00 on Main Street: the same for Caliche's seven"), [Glow]
 { Glow(TEXT("diner"), FVector2D(8000 - 2, 2140), 22, true); });

 // ---- the views ----
 E.AddFreeform(Group, TEXT("Dale's window from outside, day"), TEXT("11:00 south of the annex, looking at the office's window"), [View, Saro, Saro3]
 { View(TEXT("saro"), 11, Saro(.4, 17), Saro3(.4, .55, 10.8), TEXT("Dale's window from the yard: glass, frame and sill, the blind and the room behind it")); });
 E.AddFreeform(Group, TEXT("Dale's window from inside"), TEXT("11:00 in the office, looking out of the window"), [View, Saro, Saro3]
 { View(TEXT("saro"), 11, Saro(1.0, 8.0), Saro3(.4, .1, 10.8), TEXT("Dale's window from the office: the same opening, the yard outside")); });
 E.AddFreeform(Group, TEXT("The records room"), TEXT("11:00 in the records room, looking at its south wall: what the walk colliders there stand for"), [View, Saro, Saro3]
 { View(TEXT("saro"), 11, Saro(-3.0, 7.4), Saro3(-3.6, -.6, 10.2), TEXT("The records room: shelves and boxes against the south wall")); });
 E.AddFreeform(Group, TEXT("The substation by day"), TEXT("11:00 at the gate, looking in"), [View]
 { View(TEXT("diner"), 11, FVector2D(-104, 516), FVector(-110, -.4, 537), TEXT("The substation: fence and its wire, transformers on their pads, insulators, signs, gravel")); });
 E.AddFreeform(Group, TEXT("The substation from the east"), TEXT("11:00 east of the fence"), [View]
 { View(TEXT("diner"), 11, FVector2D(-94, 540), FVector(-110, -.4, 537), TEXT("The substation from the side: the yard's walk edge should be the fence")); });
 E.AddFreeform(Group, TEXT("Highway lamps by day"), TEXT("11:00 on the highway west of SARO"), [View]
 { View(TEXT("saro"), 11, FVector2D(11, -254), FVector(5.5, 4.5, -264), TEXT("A highway lamp: pole, arm and cobra head")); });
 E.AddFreeform(Group, TEXT("A lamp's foot"), TEXT("11:00 close to a highway lamp's foot"), [View]
 { View(TEXT("saro"), 11, FVector2D(8.5, -261), FVector(5.5, -1.6, -264), TEXT("A highway lamp's foot on its concrete, on the ground")); });
 E.AddFreeform(Group, TEXT("Caliche's lamps by day"), TEXT("11:00 on Main Street"), [View]
 { View(TEXT("diner"), 11, FVector2D(3, 2138), FVector(-6, 4.5, 2150), TEXT("Main Street's lamps by day")); });
 E.AddFreeform(Group, TEXT("Dale's window from outside, night"), TEXT("22:00 south of the annex"), [View, Saro, Saro3]
 { View(TEXT("saro"), 22, Saro(.4, 17), Saro3(.4, .55, 10.8), TEXT("Dale's window at night from the yard")); });
 E.AddFreeform(Group, TEXT("The substation at night"), TEXT("22:00 at the gate"), [View]
 { View(TEXT("diner"), 22, FVector2D(-104, 516), FVector(-110, -.4, 537), TEXT("The substation at night")); });
 E.AddFreeform(Group, TEXT("Highway lamps at night"), TEXT("22:00 on the highway west of SARO"), [View]
 { View(TEXT("saro"), 22, FVector2D(11, -254), FVector(5.5, 4.5, -264), TEXT("Highway lamps at night: sodium yellow, a warm cone on the road")); });
 E.AddFreeform(Group, TEXT("SARO's yard lamps at night"), TEXT("22:00 in the service yard"), [View]
 { View(TEXT("saro"), 22, FVector2D(41, -291), FVector(35.25, 2.2, -298.4), TEXT("The service yard's lamps at night")); });
 // ---- ARRAY-LAMPS: the field's eleven posts are cobra heads now; the one by S-02 from the spine, and every one measured ----
 E.AddFreeform(Group, TEXT("Array field lamps at night"), TEXT("22:00 on the spine by S-02, looking at its lamp: no old post or glow left, each lamp's head, lens, halo and spot in the head (the log has each)"), [this, Log, Saro, Saro3]
 {
  const FVector2D EyeRoad = Saro(-1.5, -36);
  const FVector Eye = S47FarmWorld::RoadToArea(TEXT("saro"), FVector(EyeRoad.X, 0, EyeRoad.Y));
  const FVector LookRoad = Saro3(-11.1, 2.5, -44);
  const FVector Look = S47FarmWorld::RoadToArea(TEXT("saro"), FVector(LookRoad.X, 0, LookRoad.Z));
  FStage S; S.From = 22; S.To = 23.5; S.Area = TEXT("saro"); S.Where = [Eye] { return FVector2D(Eye.X, Eye.Z); };
  Go(S, [this, Log, Look, LookRoad]
  {
   if (!M->Player) { Tell(false, TEXT("no player")); return; }
   AimAt(FVector(Look.X, M->Player->FloorY + M->Player->Eye + LookRoad.Y, Look.Z));
   After(1.2, [this, Log]
   {
    TArray<FString> Lines; int32 Lit = 0;
    const int32 Bad = S47StreetLamps::MeasureField(M->Web, Lines);
    const FString Field = Lines.Num() ? Lines[0] : FString(TEXT("no field lamps"));
    const int32 Glow = S47StreetLamps::MeasureGlow(M->Web, Lines, Lit);
    Log(Lines);
    Tell(Bad == 0 && Glow == 0 && Lit > 0, FString::Printf(TEXT("%s; %d street lamps lit here, %d glow wrong (the log has each)"), *Field, Lit, Glow), 12);
   });
  });
 });
 E.AddFreeform(Group, TEXT("Caliche's lamps at night"), TEXT("22:00 on Main Street"), [View]
 { View(TEXT("diner"), 22, FVector2D(3, 2138), FVector(-6, 4.5, 2150), TEXT("Main Street's lamps at night")); });

 // ---- M6 part 2 (9 November): the motel, the farm's and the motel's lamps, Caliche's glows and its buildings' feet ----
 // The motel stands in SARO's terms (S47Geo::MOTEL_AT, the court group at x -2.5, z 650): the reception x -48.1..-43.7, z 672.3..677.5,
 // its door at x -43.7, z 674.1; the television at (-47.77, floor + 1.5, 676.85), turned to the counter and the door.
 E.AddFreeform(Group, TEXT("Motel office from the door"), TEXT("11:00 in the motel office's doorway, looking in"), [View, Saro, Saro3]
 { View(TEXT("saro"), 11, Saro(-43.9, 674.1), Saro3(-48.0, -.15, 675.6), TEXT("The motel office from the door: walls, counter, shelves and the television")); });
 E.AddFreeform(Group, TEXT("Motel office, south wall"), TEXT("11:00 inside the office, looking at its south wall and window"), [View, Saro, Saro3]
 { View(TEXT("saro"), 11, Saro(-45.6, 676.6), Saro3(-45.0, -.2, 672.3), TEXT("The office's south wall from inside: the right side of every face")); });
 E.AddFreeform(Group, TEXT("Motel office from outside"), TEXT("11:00 on the walk south-east of the office"), [View, Saro, Saro3]
 { View(TEXT("saro"), 11, Saro(-40.6, 671.6), Saro3(-43.8, -.25, 674.4), TEXT("The office from the walk: the window and the door in their wall")); });
 // MOTEL-REAL: what the office's front is made of, ray by ray from the walk: mesh, material, where each ray stops and which side
 // of the face it meets (the window reads as a box standing out of the wall). The list goes to the log.
 E.AddFreeform(Group, TEXT("Motel office window: what is drawn"), TEXT("11:00 on the walk: a fan of rays over the office's window and door; the list goes to the log"), [this, Log]
 {
  FStage S; S.From = 11; S.To = 12.5; S.Area = TEXT("saro"); S.Where = [] { return FVector2D(-40.6, 671.6); };
  Go(S, [this, Log]
  {
   if (!M->Player) { Tell(false, TEXT("no player")); return; }
   AimAt(FVector(-43.8, M->Player->FloorY + M->Player->Eye - .25, 674.4));
   After(1.0, [this, Log]
   {
    AS47Walker* P = M->Player; UWorld* World = M->Web->GetWorld();
    TArray<FString> Lines; TMap<FString, int32> Seen;
    for (int32 Y = -6; Y <= 6; ++Y) for (int32 Pi = -2; Pi <= 2; ++Pi)
    {
     const double Yaw = P->Yaw + Y * FMath::DegreesToRadians(5.), Pitch = P->Pitch + Pi * FMath::DegreesToRadians(5.);
     const FVector Fwd(-FMath::Sin(Yaw) * FMath::Cos(Pitch), FMath::Sin(Pitch), -FMath::Cos(Yaw) * FMath::Cos(Pitch));
     const FVector Start = P->EyeUE(), End = Start + S47Three::DirToUE(Fwd) * 2500;
     FHitResult H; FCollisionQueryParams Q(SCENE_QUERY_STAT(S47LooksMotelRay), true, P); Q.bReturnFaceIndex = true;
     if (!World->LineTraceSingleByChannel(H, Start, End, ECC_Visibility, Q)) continue;
     UStaticMeshComponent* C = Cast<UStaticMeshComponent>(H.GetComponent()); int32 Section = INDEX_NONE;
     UMaterialInterface* Mat = C ? C->GetMaterialFromCollisionFaceIndex(H.FaceIndex, Section) : nullptr;
     const FS47WebNode* N = M->Web->Node(M->Web->NodeOf(H.GetComponent()));
     const FVector At = S47Three::ToThree(H.ImpactPoint);
     const double Dot = FVector::DotProduct(H.ImpactNormal, (End - Start).GetSafeNormal());
     const FString Key = FString::Printf(TEXT("%s | %s | %s"), N ? *N->Name : *GetNameSafe(H.GetComponent()), C && C->GetStaticMesh() ? *C->GetStaticMesh()->GetName() : TEXT("-"), *GetNameSafe(Mat));
     ++Seen.FindOrAdd(Key);
     Lines.Add(FString::Printf(TEXT("  ray yaw %+d pitch %+d: %s at x %.3f y %.3f z %.3f, %.2f m, %s face (normal.ray %+.2f), %s"), Y * 5, Pi * 5, *Key, At.X, At.Y, At.Z, H.Distance / 100,
      Dot > 0 ? TEXT("BACK") : TEXT("front"), Dot, Mat && Mat->IsTwoSided() ? TEXT("two-sided") : TEXT("one-sided")));
    }
    TArray<FString> Head; Head.Add(FString::Printf(TEXT("Motel office front: %d rays, %d things met:"), Lines.Num(), Seen.Num()));
    for (const auto& K : Seen) Head.Add(FString::Printf(TEXT("  %d x %s"), K.Value, *K.Key));
    Head.Append(Lines); Log(Head);
    Tell(true, Head[0], 10);
   });
  });
 });
 // MOTEL-FACADE-20261109: the office has one front, V2's: the export's wear layer (node 769, g529) no longer meets a ray on the office's
 // front (x -43.6 and out, z 672.0..677.8). The same fan of rays as above; PASS when none of them stops on it there.
 E.AddFreeform(Group, TEXT("Motel office: one front"), TEXT("11:00 on the walk: the fan of rays over the office's front; none may stop on the export's wear layer there"), [this, Log]
 {
  FStage S; S.From = 11; S.To = 12.5; S.Area = TEXT("saro"); S.Where = [] { return FVector2D(-40.6, 671.6); };
  Go(S, [this, Log]
  {
   if (!M->Player) { Tell(false, TEXT("no player")); return; }
   AimAt(FVector(-43.8, M->Player->FloorY + M->Player->Eye - .25, 674.4));
   After(1.0, [this, Log]
   {
    AS47Walker* P = M->Player; UWorld* World = M->Web->GetWorld();
    int32 Rays = 0, Wear = 0, Office = 0, Ground = 0; FString First;
    for (int32 Y = -6; Y <= 6; ++Y) for (int32 Pi = -2; Pi <= 2; ++Pi)
    {
     const double Yaw = P->Yaw + Y * FMath::DegreesToRadians(5.), Pitch = P->Pitch + Pi * FMath::DegreesToRadians(5.);
     const FVector Fwd(-FMath::Sin(Yaw) * FMath::Cos(Pitch), FMath::Sin(Pitch), -FMath::Cos(Yaw) * FMath::Cos(Pitch));
     const FVector Start = P->EyeUE(), End = Start + S47Three::DirToUE(Fwd) * 2500;
     FHitResult H; FCollisionQueryParams Q(SCENE_QUERY_STAT(S47LooksMotelFront), true, P);
     if (!World->LineTraceSingleByChannel(H, Start, End, ECC_Visibility, Q)) continue;
     ++Rays;
     const FVector At = S47Three::ToThree(H.ImpactPoint);
     if (At.X < -43.6 || At.Z < 672.0 || At.Z > 677.8) continue;
     ++Office;
     // A front stands: the layer's flat band of dust on the walk at the wall's foot (g529, 1.5 cm over the ground, layer x -41.0 to
     // -40.5, z 24.8 to 27.8: the sweep of 9 November met it once, at y -2.084, 30 cm out from the wall) is ground, not a second front.
     if (M->Web->NodeOf(H.GetComponent()) == 769)
     {
      if (FMath::Abs(H.ImpactNormal.Z) > .7) { ++Ground; continue; }
      ++Wear; if (First.IsEmpty()) First = FString::Printf(TEXT(" (first at x %.3f y %.3f z %.3f)"), At.X, At.Y, At.Z);
     }
    }
    // what the layer draws: 648 triangles as the trim has it, 596 with the old office front cut out (S47MotelV2 CutOut; without the
    // trim's CPU data it stays 648 and the layer is only set back 3 cm)
    const FS47WebNode* Layer = M->Web->Node(769);
    const UStaticMeshComponent* LayerMesh = Layer ? Cast<UStaticMeshComponent>(Layer->Comp.Get()) : nullptr;
    const UStaticMesh* Drawn = LayerMesh ? LayerMesh->GetStaticMesh() : nullptr;
    const int32 Drawing = Drawn && Drawn->GetRenderData() && Drawn->GetNumLODs() > 0 ? Drawn->GetNumTriangles(0) : -1;
    TArray<FString> Lines; Lines.Add(FString::Printf(TEXT("Motel office front: %d rays hit, %d on the office's front, %d of them on a standing face of the export's wear layer (node 769)%s, %d on its dust on the walk; the layer draws %d triangles (%s)"), Rays, Office, Wear, *First, Ground,
     Drawing, Drawing == 596 ? TEXT("the old front cut out") : Drawing == 648 ? TEXT("not cut: no CPU data on the trim? run Tools/streetlamps/prepare_motel_cpu.py") : TEXT("unexpected")));
    Log(Lines);
    Tell(Wear == 0 && Office > 0, Lines[0], 10);
   });
  });
 });
 E.AddFreeform(Group, TEXT("Motel office window, close"), TEXT("11:00 on the walk by the office, looking along its front at the window"), [View, Saro, Saro3]
 { View(TEXT("saro"), 11, Saro(-41.4, 673.2), Saro3(-43.5, -.1, 676.2), TEXT("The office's window from the walk: frame and sill in V2's wall, one face behind them")); });
 E.AddFreeform(Group, TEXT("Invisible blockers at the farm"), TEXT("11:00 at the farm's turn-off: the same for the farm's yard, barn and road (the road's walk)"), [Blockers]
 { Blockers(TEXT("diner"), FVector2D(8000 + 26, 181), TEXT("the farm")); });
 E.AddFreeform(Group, TEXT("Motel television at night"), TEXT("23:00 in front of the office's television"), [View, Saro, Saro3]
 { View(TEXT("saro"), 23, Saro(-46.3, 675.6), Saro3(-47.77, .05, 676.85), TEXT("The office television at night: what is on the screen")); });
 E.AddFreeform(Group, TEXT("Motel lot lamps by day"), TEXT("11:00 in the motel's lot"), [View, Saro, Saro3]
 { View(TEXT("saro"), 11, Saro(-38.5, 662), Saro3(-32.05, 3.2, 668), TEXT("The motel's lot lamp: pole, arm and head")); });
 E.AddFreeform(Group, TEXT("Motel lot lamps at night"), TEXT("22:00 in the motel's lot"), [View, Saro, Saro3]
 { View(TEXT("saro"), 22, Saro(-38.5, 662), Saro3(-32.05, 3.2, 668), TEXT("The motel's lot lamps at night")); });
 E.AddFreeform(Group, TEXT("Farm lamp by day"), TEXT("11:00 at the farm's turn-off from the highway (SARO's terms, where the farm is drawn)"), [View]
 { View(TEXT("saro"), 11, FVector2D(26, 181), FVector(15, 4.5, 190), TEXT("The lamp on the pole at the farm's turn-off")); });
 E.AddFreeform(Group, TEXT("Farm lamp at night"), TEXT("22:00 at the farm's turn-off"), [View]
 { View(TEXT("saro"), 22, FVector2D(26, 181), FVector(15, 4.5, 190), TEXT("The farm's turn-off lamp at night")); });
 E.AddFreeform(Group, TEXT("Substation track lamp by day"), TEXT("11:00 where the service track leaves the highway"), [View]
 { View(TEXT("diner"), 11, FVector2D(-1, 503), FVector(-10, 4.5, 512), TEXT("The lamp where the substation's track leaves the highway")); });
 E.AddFreeform(Group, TEXT("SARO's yard lamps by day"), TEXT("11:00 in the service yard"), [View]
 { View(TEXT("saro"), 11, FVector2D(41, -291), FVector(35.25, 2.2, -298.4), TEXT("The service yard's lamps by day")); });
 E.AddFreeform(Group, TEXT("A Caliche lamp close, night"), TEXT("22:00 under a lamp on Main Street"), [View]
 { View(TEXT("diner"), 22, FVector2D(1.5, 2143), FVector(-5.2, 5.5, 2150), TEXT("One of Caliche's lamps close at night: a small glow at the bowl, a pool on the ground")); });
 E.AddFreeform(Group, TEXT("Caliche from the highway, day"), TEXT("11:00 on the highway south of Caliche"), [View]
 { View(TEXT("diner"), 11, FVector2D(4, 2005), FVector(-4, 6, 2120), TEXT("Main Street's lamps from the south by day: no glows")); });
 // CALICHE-REAL: every building shell's feet against the land, the list to the log; the picture is the worst corner, if one floats.
 E.AddFreeform(Group, TEXT("Caliche: buildings in the ground"), TEXT("11:00 on Main Street: each building's corners and edge middles against the ground outside them; the list goes to the log"), [this, Log]
 {
  FStage S; S.From = 11; S.To = 12.5; S.Area = TEXT("diner"); S.Where = [] { return FVector2D(8000 - 2, 2140); };
  Go(S, [this, Log]
  {
   After(1.0, [this, Log]
   {
    TArray<FString> Lines; FVector Worst = FVector::ZeroVector;
    const int32 Bad = S47CalicheGround::Measure(M->Web, Lines, &Worst);
    Log(Lines);
    if (!Worst.IsZero() && M->Player)
    {
     // eight metres out from the worst corner, toward the street, looking at its foot
     const FVector2D From(Worst.X + (Worst.X < 8000 - 5 ? -6. : 6.), Worst.Z - 5);
     M->Player->Place(From.X, From.Y, 0);
     After(.6, [this, Worst] { AimAt(FVector(Worst.X, Worst.Y + .4, Worst.Z)); });
    }
    After(1.8, [this, Bad, Lines] { Tell(Bad == 0 && Lines.Num() > 1, Lines.Num() ? Lines[0] : FString(TEXT("no buildings")), 12); });
   });
  });
 });
}

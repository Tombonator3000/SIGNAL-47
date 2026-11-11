#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47Drive.h"
#include "S47FarmWorld.h"
#include "S47Main.h"
#include "S47RoadTerrain.h"
#include "S47Walker.h"
#include "S47World.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47FarmGateDev,Log,All);

// F10, "Farm gate (T)" (FARM-GATE-20261109, Tom's test of d37: an invisible wall in the farm's open gate). 11:00 at the Alderete gate in
// SARO's terms: first the lists (S47FarmWorld::GateLive: the truck both ways in both legs, on foot both ways), then the service Jeep
// driven in through the gate by the ordinary drive (TestInput, 3 m/s along the ranch road) to the yard's lane and, set back past the
// gate, driven out again; then on foot in and out (the walker's own TestMove). Each step says where it stopped and what held it
// (LogS47FarmGateDev too). The truck goes back where it stood when the line began. The line saves nothing itself; a drive started
// while SARO's departure is armed files that one autosave, as any drive from the station does (SAVE-SARO-20261108).
// -S47DevSweep=/tmp/dir "-S47DevSweepGroup=Farm gate (T)" runs it in turn.
void FS47DevFreeform::AddFarmGate(FS47DevEvents& E)
{
 static const TCHAR* const Group=TEXT("Farm gate (T)");
 E.AddFreeform(Group,TEXT("Farm gate: drive and walk through, both ways"),TEXT("11:00 at the Alderete gate: the lists, then the Jeep in through the open gate and out again, then on foot in and out"),[this]
 {
  if(!M->World||!M->Player||!S47FarmWorld::Enabled(M)){Tell(false,TEXT("Freeform only"));return;}
  const auto Saro=[](double RoadX){return S47FarmWorld::RoadToArea(TEXT("saro"),FVector(RoadX,S47FarmWorld::FloorRoad(RoadX,S47RoadTerrain::RanchZ(RoadX)),S47RoadTerrain::RanchZ(RoadX)));};
  const auto RoadX=[](const FVector& P){return P.X-S47FarmWorld::RoadToArea(TEXT("saro"),FVector::ZeroVector).X;};
  TSharedRef<TArray<FString>> Said=MakeShared<TArray<FString>>();
  TSharedRef<bool> Good=MakeShared<bool>(true);
  const auto Note=[Said,Good](bool bOk,const FString& Line)
  {
   UE_LOG(LogS47FarmGateDev,Display,TEXT("S47: %s %s"),bOk?TEXT("PASS"):TEXT("FAIL"),*Line);
   Said->Add(FString(bOk?TEXT("PASS "):TEXT("FAIL "))+Line);*Good&=bOk;
  };
  const auto Finish=[this,Said,Good]
  {
   if(M->World)M->World->TestInput=nullptr;if(M->Player)M->Player->TestMove.Reset();
   const FString Text=FString::Printf(TEXT("FARM-GATE: %s. %s"),*Good?TEXT("through both ways, at the wheel and on foot"):TEXT("held (log: LogS47FarmGateDev)"),*FString::Join(*Said,TEXT("; ")));
   UE_LOG(LogS47FarmGateDev,Display,TEXT("S47: %s"),*Text);
   Tell(*Good,Text.Left(400),14);
  };
  // On foot from one side of the gate to the other along the road's line, the walker's own move; then Then.
  const auto Walk=[this,Saro,RoadX,Note](bool bEast,TFunction<void()> Then)
  {
   const double From=bEast?14.:28.,To=bEast?28.:14.;const FVector P=Saro(From);
   M->Player->Place(P.X,P.Z,bEast?-UE_DOUBLE_HALF_PI:UE_DOUBLE_HALF_PI);
   WaitFor([this,Saro,RoadX,bEast,To]
   {
    if(!M->Player)return true;
    const double X=RoadX(M->Player->Pos);if(bEast?X>=To:X<=To){M->Player->TestMove.Reset();return true;}
    const FVector Aim=Saro(X+(bEast?3.:-3.)),D=Aim-M->Player->Pos;M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=-.08;M->Player->TestMove=FVector2D(0,-1);
    return false;
   },20,[Note,bEast,Then]{Note(true,FString::Printf(TEXT("on foot %s through the gate"),bEast?TEXT("in"):TEXT("out")));Then();},
   [this,RoadX,Note,bEast,Then]
   {
    if(M->Player)M->Player->TestMove.Reset();
    Note(false,FString::Printf(TEXT("on foot %s: stopped at road x %.2f"),bEast?TEXT("in"):TEXT("out"),M->Player?RoadX(M->Player->Pos):0.));Then();
   });
  };
  // The Jeep set on the ranch road on one side, driven by the ordinary drive to the other side; then Then (on foot again).
  const auto Drive=[this,Saro,RoadX,Note](bool bEast,TFunction<void()> Then)
  {
   if(M->World->bDriving)M->World->StopDriving();
   const double From=bEast?12.:30.,To=bEast?29.:13.;
   FS47TripOpts O;O.Legs={TEXT("saro"),TEXT("road"),TEXT("station01")};O.Places={TEXT("diner")};O.Leg=TEXT("saro");
   O.AtPos=Saro(From);O.AtHeading=bEast?-UE_DOUBLE_HALF_PI:UE_DOUBLE_HALF_PI;
   M->World->StartTrip(TEXT("saro"),FString(),O);
   const TWeakObjectPtr<US47World> World=M->World;
   M->World->TestInput=[World,Saro,RoadX,bEast]
   {
    // Along the road's line, 4 m ahead, at walking pace and a little more (FarmWorldTest's Follow, its steering).
    FS47DriveIn I;US47World* W=World.Get();FS47DriveController* D=W?W->CabDrive():nullptr;if(!D)return I;
    const FVector Aim=Saro(RoadX(D->Pos)+(bEast?4.:-4.)),V=Aim-D->Pos;
    const double Right=V.X*FMath::Cos(D->Heading)-V.Z*FMath::Sin(D->Heading);
    I.Steer=D->SteerFor(2*Right/FMath::Max(.1,V.X*V.X+V.Z*V.Z));I.Throttle=FMath::Clamp((3-D->Speed)*.8,-1.,1.);return I;
   };
   WaitFor([this,RoadX,bEast,To]
   {
    FS47DriveController* D=M->World?M->World->CabDrive():nullptr;if(!M->World||!M->World->bDriving||!D)return false;
    const double X=RoadX(D->Pos);return bEast?X>=To:X<=To;
   },25,[this,Note,bEast,Then]
   {
    M->World->TestInput=nullptr;M->World->StopDriving();
    Note(true,FString::Printf(TEXT("the Jeep driven %s through the gate"),bEast?TEXT("in"):TEXT("out")));Then();
   },[this,RoadX,Note,bEast,Then]
   {
    FS47DriveController* D=M->World?M->World->CabDrive():nullptr;const double X=D?RoadX(D->Pos):0;
    // What holds it there: the same lists, as the drive reads them.
    TArray<FString> Lines;S47FarmWorld::GateLive(M,Lines);
    if(M->World){M->World->TestInput=nullptr;M->World->StopDriving();}
    Note(false,FString::Printf(TEXT("the Jeep %s: stopped at road x %.2f; %s"),bEast?TEXT("in"):TEXT("out"),X,*FString::Join(Lines,TEXT(" / "))));Then();
   });
  };
  FStage S;S.From=11;S.To=16;S.Area=TEXT("saro");S.Yaw=-UE_DOUBLE_HALF_PI;
  S.Where=[Saro]{const FVector P=Saro(14);return FVector2D(P.X,P.Z);};
  Go(S,[this,Note,Finish,Walk,Drive]
  {
   TArray<FString> Lines;S47FarmWorld::GateLive(M,Lines);
   for(const FString& L:Lines){const bool bFail=L.StartsWith(TEXT("FAIL"));if(bFail)Note(false,L.Mid(5));else UE_LOG(LogS47FarmGateDev,Display,TEXT("S47: %s"),*L);}
   Drive(true,[Drive,Walk,Finish]{Drive(false,[Walk,Finish]{Walk(true,[Walk,Finish]{Walk(false,Finish);});});});
  });
 });
}

#include "S47FarmWorld.h"
#include "S47Drive.h"
#include "S47Main.h"
#include "S47RoadTerrain.h"
#include "S47Truck.h"
#include "S47Walker.h"
#include "S47World.h"

// FARM-GATE-20261109 (Tom's test of d37, 9 November about 17:45: "I can't drive through the gate at the farm. There's an invisible wall
// there."). In Freeform the pipe gate in the highway's fence at road x 21 stands open (S47HighwayRanch swings it), and US47World::
// ReconcileFarm retires the closed gate's box (ClosedRanchGate, x 21 +/- .028, RanchZ(21) +/- 3.4) from the truck's lists by its slot.
// Two later fixes took obstacles out of the live lists after those slots were read (SARO's stop sign, S47SaroFixes; Caliche's ends,
// BLOCKERS-20261109): every slot behind moved down one, the retired copy went over the obstacle after the gate, and the gate itself
// stayed live. The log of d37: the Jeep stopped at SARO's x -5.1 (road 18.9) again and again, its front (2.12 - .10 m) on x -3.0.
// The walker reads the same list (US47World::OnLand), so one on foot was held there too. Now each entry finds its own obstacle again,
// and the two fixes park theirs far off in their own slots instead of removing them.
namespace
{
 bool Near(double A,double B){return FMath::Abs(A-B)<1e-4;}
 bool SameObstacle(const FS47Obstacle& A,const FS47Obstacle& B)
 {
  if(A.bCircle!=B.bCircle)return false;
  return A.bCircle?Near(A.X,B.X)&&Near(A.Z,B.Z)&&Near(A.R,B.R):Near(A.MinX,B.MinX)&&Near(A.MaxX,B.MaxX)&&Near(A.MinZ,B.MinZ)&&Near(A.MaxZ,B.MaxZ);
 }
 // As ReconcileFarm has always retired one: a million metres east, a circle by its middle.
 FS47Obstacle Retired(FS47Obstacle B){B.MinX+=1000000;B.MaxX+=1000000;B.X+=1000000;return B;}
 FString Describe(const FS47Obstacle& O,const FVector& Origin)
 {
  return O.bCircle?FString::Printf(TEXT("a circle r %.2f at road %.2f %.2f"),O.R,O.X-Origin.X,O.Z-Origin.Z)
   :FString::Printf(TEXT("a box road x %.3f..%.3f z %.2f..%.2f"),O.MinX-Origin.X,O.MaxX-Origin.X,O.MinZ-Origin.Z,O.MaxZ-Origin.Z);
 }
 // The ranch road's line through the gate, road terms: from x 10 (off the highway's shoulder) to 34 (in the yard's lane), heading as
 // the drive has it (forward is (-sin, -cos)).
 constexpr double FromX=10,ToX=34;
 double LineHeading(double X,bool bEast)
 {
  const double A=FMath::Atan2(S47RoadTerrain::RanchZ(X+.5)-S47RoadTerrain::RanchZ(X-.5),1.);
  return bEast?-UE_DOUBLE_HALF_PI-A:UE_DOUBLE_HALF_PI-A;
 }
 const FVector SaroFrame(-24,0,300),RoadFrame(8000,0,0); // S47FarmWorld::RoadToArea's
}

namespace S47FarmWorld
{
 int32 ControlledSlot(const TArray<FS47Obstacle>& Obstacles,int32 Hint,const FS47Obstacle& Own,const TSet<int32>& Taken)
 {
  const FS47Obstacle Off=Retired(Own);
  const auto Fits=[&](int32 I){return Obstacles.IsValidIndex(I)&&!Taken.Contains(I)&&(SameObstacle(Obstacles[I],Own)||SameObstacle(Obstacles[I],Off));};
  if(Fits(Hint))return Hint;
  const int32 Far=FMath::Max(Hint,Obstacles.Num()-Hint)+1;
  for(int32 D=1;D<=Far;++D){if(Fits(Hint-D))return Hint-D;if(Fits(Hint+D))return Hint+D;}
  return INDEX_NONE;
 }

 void ApplyControlled(TArray<FS47Obstacle>& Obstacles,TArray<TPair<int32,FS47Obstacle>>& Controlled,bool bFreeform)
 {
  TSet<int32> Taken;
  for(TPair<int32,FS47Obstacle>& Entry:Controlled)
  {
   const bool Added=Entry.Key<0;
   const int32 I=ControlledSlot(Obstacles,Added?-Entry.Key-1:Entry.Key,Entry.Value,Taken);
   if(I==INDEX_NONE)continue; // gone from the list: never write over another's slot
   Taken.Add(I);Entry.Key=Added?-I-1:I;
   // Freeform retires what it controls of the export's and keeps its own additions; Story the other way round.
   Obstacles[I]=Added!=bFreeform?Retired(Entry.Value):Entry.Value;
  }
 }

 bool GateSweep(const TArray<FS47Obstacle>& Obstacles,const FVector& Origin,double HalfWidth,double HalfLength,bool bEast,FString& Why)
 {
  constexpr int32 Steps=96;
  for(int32 K=0;K<=Steps;++K)
  {
   const double X=FMath::Lerp(bEast?FromX:ToX,bEast?ToX:FromX,double(K)/Steps),Z=S47RoadTerrain::RanchZ(X),H=LineHeading(X,bEast);
   // FS47DriveController::Hit's own give: .05 off the width, .10 off the length.
   if(!TruckHits(Obstacles,X+Origin.X,Z+Origin.Z,H,HalfWidth,HalfLength,.05))continue;
   for(int32 I=0;I<Obstacles.Num();++I)
    if(TruckHits(TArray<FS47Obstacle>{Obstacles[I]},X+Origin.X,Z+Origin.Z,H,HalfWidth,HalfLength,.05))
    {Why=FString::Printf(TEXT("held at road x %.2f by #%d, %s"),X,I,*Describe(Obstacles[I],Origin));return false;}
   Why=FString::Printf(TEXT("held at road x %.2f"),X);return false;
  }
  return true;
 }

 bool GateLive(US47Main* M,TArray<FString>& Lines)
 {
  if(!M||!M->World){Lines.Add(TEXT("FAIL farm gate: no world"));return false;}
  if(!Enabled(M)){Lines.Add(TEXT("SKIP farm gate: Story keeps the ranch gate shut"));return true;}
  US47World* W=M->World;bool Good=true;
  const FS47Truck* T=W->SaroTruck();const double Hw=T?T->HalfWidth():1.02,Hl=T?T->HalfLength():2.12;
  for(const TCHAR* Leg:{TEXT("saro"),TEXT("road")})
  {
   IS47DriveArea* A=W->LegArea(Leg);if(!A){Lines.Add(FString::Printf(TEXT("FAIL farm gate: no %s leg"),Leg));Good=false;continue;}
   const FVector& O=FString(Leg)==TEXT("saro")?SaroFrame:RoadFrame;
   for(const bool bEast:{true,false})
   {
    FString Why;bool Ok=GateSweep(A->Obstacles,O,Hw,Hl,bEast,Why);
    // and where the truck's middle may go (SARO's corridor, the road's right of way), as Hit asks first
    for(int32 K=0;Ok&&K<=48;++K){const double X=FMath::Lerp(FromX,ToX,K/48.);if(A->Blocked(X+O.X,S47RoadTerrain::RanchZ(X)+O.Z)){Ok=false;Why=FString::Printf(TEXT("the leg's corridor stops the truck's middle at road x %.2f"),X);}}
    Lines.Add(FString::Printf(TEXT("%s farm gate: the truck (%.2f x %.2f) through the open gate in the %s leg, %s%s"),Ok?TEXT("PASS"):TEXT("FAIL"),Hw*2,Hl*2,Leg,bEast?TEXT("in (east)"):TEXT("out (west)"),Ok?TEXT(""):*(TEXT(": ")+Why)));
    Good&=Ok;
   }
  }
  // On foot: what the walker asks (OnLand: the land, its exclusions and the same truck's list), in the area the player is in.
  const FString Area=W->Area;
  if(M->Player&&(Area==TEXT("saro")||Area==TEXT("road")||Area==TEXT("diner")))
  {
   const FVector& O=Area==TEXT("saro")?SaroFrame:RoadFrame;
   for(const bool bEast:{true,false})
   {
    FString Why;
    for(int32 K=0;K<=70&&Why.IsEmpty();++K){const double X=FMath::Lerp(bEast?14.:28.,bEast?28.:14.,K/70.);if(!W->OnLand(X+O.X,S47RoadTerrain::RanchZ(X)+O.Z,Area))Why=FString::Printf(TEXT("stops at road x %.2f"),X);}
    Lines.Add(FString::Printf(TEXT("%s farm gate: on foot through the gate in %s, %s%s"),Why.IsEmpty()?TEXT("PASS"):TEXT("FAIL"),*Area,bEast?TEXT("in (east)"):TEXT("out (west)"),Why.IsEmpty()?TEXT(""):*(TEXT(": ")+Why)));
    Good&=Why.IsEmpty();
   }
  }
  else Lines.Add(TEXT("SKIP farm gate: on foot is checked where the player is (SARO's area or the road's)"));
  return Good;
 }

 bool GateSelfTest(TArray<FString>& Lines)
 {
  bool Good=true;
  for(const FVector& O:{SaroFrame,RoadFrame})
  {
   const double Gx=S47RoadTerrain::HWY.Fence+O.X,Gz=S47RoadTerrain::RanchZ(S47RoadTerrain::HWY.Fence)+O.Z;
   // A list as the export's: something a later fix takes out (SARO's stop sign's foot), the gate's posts (the export's radius .2
   // circles at RanchZ(21) +/- 3.4), the closed gate, the obstacle after it, and one of the farm's own (the old house's box).
   TArray<FS47Obstacle> Obs;
   Obs.Add(FS47Obstacle::Box(O.X-19.15,O.X-18.85,O.Z+20.95,O.Z+21.25));
   Obs.Add(FS47Obstacle::Circle(Gx,Gz-3.4,.2));Obs.Add(FS47Obstacle::Circle(Gx,Gz+3.4,.2));
   const FS47Obstacle Gate=FS47Obstacle::Box(Gx-.028,Gx+.028,Gz-3.4,Gz+3.4),After=FS47Obstacle::Box(O.X+60,O.X+62,O.Z+150,O.Z+152),Own=FS47Obstacle::Box(O.X+101,O.X+115,O.Z+187,O.Z+197);
   TArray<TPair<int32,FS47Obstacle>> C;C.Add({Obs.Add(Gate),Gate});Obs.Add(After);C.Add({-Obs.Add(Own)-1,Own});
   const auto Count=[&Obs](const FS47Obstacle& B){int32 N=0;for(const FS47Obstacle& X:Obs)N+=SameObstacle(X,B);return N;};
   const auto Through=[&](bool bEast){FString Why;return GateSweep(Obs,O,1.02,2.12,bEast,Why)&&GateSweep(Obs,O,1.05,2.75,bEast,Why);};
   const auto Open=[&]{return Through(true)&&Through(false)&&Count(Gate)==0&&Count(After)==1&&Count(Own)==1;};
   const auto Shut=[&]{return !Through(true)&&!Through(false)&&Count(Gate)==1&&Count(After)==1&&Count(Own)==0;};
   ApplyControlled(Obs,C,false);const bool StoryShut=Shut();
   ApplyControlled(Obs,C,true);const bool FreeOpen=Open();
   Obs.RemoveAt(0); // what the stop sign's fix did to SARO's live list after the farm had read it
   ApplyControlled(Obs,C,true);const bool StillOpen=Open();
   ApplyControlled(Obs,C,false);const bool ShutAgain=Shut();
   ApplyControlled(Obs,C,true);const bool OpenAgain=Open()&&C[0].Key==2;
   // An entry whose obstacle has gone writes nowhere.
   const int32 Before=Obs.Num();Obs.RemoveAt(C[0].Key);ApplyControlled(Obs,C,false);const bool Gone=Obs.Num()==Before-1&&Count(After)==1&&Count(Gate)==0&&Count(FS47Obstacle::Box(Gx-.028+1000000,Gx+.028+1000000,Gz-3.4,Gz+3.4))==0;
   const bool Ok=StoryShut&&FreeOpen&&StillOpen&&ShutAgain&&OpenAgain&&Gone;Good&=Ok;
   Lines.Add(FString::Printf(TEXT("%s farm gate (%s frame): Story's closed gate holds the truck both ways, Freeform's open gate lets it through both ways, and stays open when an obstacle before it leaves the list; the obstacle after it is kept, an entry whose obstacle is gone writes nowhere (story %d free %d moved %d shut %d open %d gone %d)"),
    Ok?TEXT("PASS"):TEXT("FAIL"),O.X>1000?TEXT("road"):TEXT("SARO"),StoryShut,FreeOpen,StillOpen,ShutAgain,OpenAgain,Gone));
  }
  return Good;
 }
}

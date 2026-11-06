#include "S47FarmHouse.h"
#include "S47RoadTerrain.h"

namespace
{
 // Plan v1 is drawn with the veranda at the bottom. It stands facing the yard, so plan x and y both run backwards.
 constexpr double OriginX=104,OriginZ=216,Width=12,Depth=9,VerandaDepth=2,WallHeight=2.6;
 constexpr double DoorHead=2.05,Sill=.9,WindowHead=2.1;
 struct FOpening { double From,To; bool bDoor; };
 struct FWall { const TCHAR* Name; double X0,Y0,X1,Y1; TArray<FOpening> Openings; };
 const TArray<FWall>& Walls()
 {
  static const TArray<FWall> Data={
   {TEXT("north"),0,0,12,.3,{{1.6,3.4,false},{5.5,6.5,true},{7.4,8.6,false},{10.5,11.3,false}}},
   {TEXT("south"),0,8.7,12,9,{{1.3,3.1,false},{5.5,6.5,true},{8,9.8,false}}},
   {TEXT("west"),0,.3,.3,8.7,{{5.8,7.1,false}}},
   {TEXT("east"),11.7,.3,12,8.7,{{6.2,7.5,false}}},
   {TEXT("hallW"),5.1,.3,5.25,8.7,{{1.8,2.7,true},{6.1,7,true}}},
   {TEXT("hallE"),6.75,.3,6.9,8.7,{{1.8,2.7,true},{6.1,7,true}}},
   {TEXT("midW"),.3,4.4,5.1,4.55,{}},
   {TEXT("midE"),6.9,4.4,11.7,4.55,{}},
   {TEXT("bath"),9.8,.3,9.95,4.4,{{2.6,3.4,true}}}};
  return Data;
 }
 double ToX(double PlanX){return OriginX-PlanX;}
 double ToZ(double PlanY){return OriginZ+Depth-PlanY;}
 // A rectangle in plan metres, as a centre and size in road-local metres.
 void Add(TArray<S47FarmHouse::FPiece>& Out,const FString& Name,double X0,double Y0,double X1,double Y1,double Bottom,double Top,S47FarmHouse::EMat Mat)
 {
  const double Floor=S47FarmHouse::PadY();
  Out.Add({Name,FVector((ToX(X0)+ToX(X1))*.5,Floor+(Bottom+Top)*.5,(ToZ(Y0)+ToZ(Y1))*.5),FVector(FMath::Abs(X1-X0),Top-Bottom,FMath::Abs(Y1-Y0)),Mat});
 }
}

namespace S47FarmHouse
{
 FVector Point(double PlanX,double PlanY){return FVector(ToX(PlanX),PadY(),ToZ(PlanY));}

 bool OnPad(double X,double Z,double Margin)
 {
  return X>=OriginX-Width-Margin&&X<=OriginX+Margin&&Z>=OriginZ-VerandaDepth-Margin&&Z<=OriginZ+Depth+Margin;
 }

 double PadY()
 {
  // A level floor just over the highest ground under it.
  static const double Y=[]
  {
   double High=-1e9;
   for(int32 I=0;I<=6;++I)for(int32 J=0;J<=6;++J)High=FMath::Max(High,S47RoadTerrain::SurfaceY(OriginX-Width*I/6.,OriginZ-VerandaDepth+(Depth+VerandaDepth)*J/6.));
   return High+.18;
  }();
  return Y;
 }

 TArray<FPiece> Pieces()
 {
  TArray<FPiece> Out;
  for(const FWall& W:Walls())
  {
   const bool bAlongX=W.X1-W.X0>W.Y1-W.Y0;
   const double Start=bAlongX?W.X0:W.Y0,End=bAlongX?W.X1:W.Y1;
   auto Run=[&](const FString& Name,double From,double To,double Bottom,double Top)
   {
    if(To-From<.001||Top-Bottom<.001)return;
    if(bAlongX)Add(Out,Name,From,W.Y0,To,W.Y1,Bottom,Top,EMat::Wall);else Add(Out,Name,W.X0,From,W.X1,To,Bottom,Top,EMat::Wall);
   };
   double At=Start;int32 Index=0;
   for(const FOpening& O:W.Openings)
   {
    const FString Base=FString::Printf(TEXT("farm:oldhouse:%s:%d"),W.Name,Index++);
    Run(Base+TEXT(":run"),At,O.From,0,WallHeight);
    if(!O.bDoor)Run(Base+TEXT(":sill"),O.From,O.To,0,Sill);
    Run(Base+TEXT(":head"),O.From,O.To,O.bDoor?DoorHead:WindowHead,WallHeight);
    At=O.To;
   }
   Run(FString::Printf(TEXT("farm:oldhouse:%s:end"),W.Name),At,End,0,WallHeight);
  }
  Add(Out,TEXT("farm:oldhouse:floor"),0,0,Width,Depth,-.3,0,EMat::Floor);
  Add(Out,TEXT("farm:oldhouse:ceiling"),0,0,Width,Depth,WallHeight,WallHeight+.12,EMat::Trim);
  Add(Out,TEXT("farm:oldhouse:roof"),-.4,-.4,Width+.4,Depth+.4,WallHeight+.12,WallHeight+.3,EMat::Roof);
  Add(Out,TEXT("farm:oldhouse:veranda"),0,Depth,Width,Depth+VerandaDepth,-.3,-.04,EMat::Floor);
  Add(Out,TEXT("farm:oldhouse:verandaRoof"),0,Depth,Width,Depth+VerandaDepth+.2,WallHeight-.14,WallHeight,EMat::Roof);
  for(const double X:{.3,4.,8.,11.7})Add(Out,FString::Printf(TEXT("farm:oldhouse:post%.0f"),X*10),X-.08,Depth+VerandaDepth-.16,X+.08,Depth+VerandaDepth,-.04,WallHeight-.14,EMat::Trim);
  return Out;
 }

 TArray<FBox2D> Solids()
 {
  TArray<FBox2D> Out;
  auto Box=[&](double X0,double Y0,double X1,double Y1)
  {
   const double A=ToX(X0),B=ToX(X1),C=ToZ(Y0),D=ToZ(Y1);
   Out.Add(FBox2D(FVector2D(FMath::Min(A,B),FMath::Min(C,D)),FVector2D(FMath::Max(A,B),FMath::Max(C,D))));
  };
  for(const FWall& W:Walls())
  {
   const bool bAlongX=W.X1-W.X0>W.Y1-W.Y0;
   double At=bAlongX?W.X0:W.Y0;const double End=bAlongX?W.X1:W.Y1;
   auto Run=[&](double From,double To){if(To-From<.001)return;if(bAlongX)Box(From,W.Y0,To,W.Y1);else Box(W.X0,From,W.X1,To);};
   for(const FOpening& O:W.Openings)if(O.bDoor){Run(At,O.From);At=O.To;}
   Run(At,End);
  }
  return Out;
 }

 bool SelfTest(TArray<FString>& Lines)
 {
  // Straight through the hall from the front door to the back door, nothing in the way.
  bool bHall=true;const auto Solid=Solids();
  for(double Y=-1;Y<=10;Y+=.05)
  {
   const FVector P=Point(6,Y);
   for(const FBox2D& B:Solid)if(B.IsInside(FVector2D(P.X,P.Z)))bHall=false;
  }
  // Each room is reached from the hall through its door.
  bool bDoors=true;
  for(const double Y:{2.25,6.55})for(const double X:{5.17,6.82})
  {
   const FVector P=Point(X,Y);
   for(const FBox2D& B:Solid)if(B.IsInside(FVector2D(P.X,P.Z)))bDoors=false;
  }
  // Walls that are not doors stay solid, windows included.
  const auto Blocked=[&](double X,double Y){const FVector P=Point(X,Y);for(const FBox2D& B:Solid)if(B.IsInside(FVector2D(P.X,P.Z)))return true;return false;};
  const bool bClosed=Blocked(2.5,.15)&&Blocked(5.17,4)&&Blocked(.15,6.5);
  const bool bGood=bHall&&bDoors&&bClosed&&OnPad(Point(6,4).X,Point(6,4).Z)&&Pieces().Num()>30;
  Lines.Add(FString(bGood?TEXT("PASS "):TEXT("FAIL "))+TEXT("farmhouse plan: hall runs clear front to back, rooms open off it, walls and windows are solid"));
  return bGood;
 }
}

#include "S47EastWing.h"
#include "S47Main.h"
#include "S47Web.h"
#include "S47Obj.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/CommandLine.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47EastWing,Log,All);

FS47EastWing::FS47EastWing(US47Main* InM):M(InM){}

void FS47EastWing::Init()
{
 if(bBuilt||!M||!M->Web||!M->Web->IsLoaded())return;
 bBuilt=true;
 if(FParse::Param(FCommandLine::Get(),TEXT("S47EastWingDump")))Dump();
}

void FS47EastWing::Dump() const
{
 // Developer survey: every exported node whose world box meets the wing, with how much of
 // it lies inside, so retirement can be exact rather than guessed.
 AS47Web* W=M->Web;const FBox Wing=Footprint().ExpandBy(FVector(.6,.2,.6));
 int32 Inside=0,Crossing=0;
 for(int32 Node=0;Node<W->PartCount();++Node)
 {
  const FS47WebNode* N=W->Node(Node);if(!N||N->Geo<0)continue;
  const FBox B=W->ObjAt(Node).WorldBox();if(!B.IsValid||!B.Intersect(Wing))continue;
  const bool Full=Wing.IsInside(B.Min)&&Wing.IsInside(B.Max);(Full?Inside:Crossing)++;
  const FS47WebNode* R=N->Root>=0?W->Node(N->Root):nullptr;
  UE_LOG(LogS47EastWing,Display,TEXT("S47: eastwing node %d %s name=%s root=%s geo=%d min=(%.2f %.2f %.2f) max=(%.2f %.2f %.2f) inst=%d"),Node,Full?TEXT("INSIDE"):TEXT("CROSS"),*N->Name,R?*R->Name:TEXT("-"),N->Geo,B.Min.X,B.Min.Y,B.Min.Z,B.Max.X,B.Max.Y,B.Max.Z,N->bInstanced?1:0);
 }
 UE_LOG(LogS47EastWing,Display,TEXT("S47: eastwing survey inside=%d crossing=%d"),Inside,Crossing);
}

#include "S47SaroPeriodProps.h"

#include "S47Main.h"
#include "S47SaroLegacyCleanup.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SaroPeriodProps, Log, All);

namespace
{
 UStaticMesh* OptionalMesh(const TCHAR* Package)
 {
  if (!FPackageName::DoesPackageExist(Package)) return nullptr;
  const FString Name=FPackageName::GetShortName(Package);
  return LoadObject<UStaticMesh>(nullptr,*(FString(Package)+TEXT(".")+Name),nullptr,LOAD_NoWarn|LOAD_Quiet);
 }

 void MaskLegacy(USceneComponent* Root)
 {
  if (!Root) return;
  TArray<USceneComponent*> Parts;
  Root->GetChildrenComponents(true,Parts); Parts.Add(Root);
  // Draw flags only. The original invisible hit boxes and mesh queries still
  // answer the existing phone/jacket interactions, and state still addresses
  // the same handset node during every call, ring and disk restore.
  for (USceneComponent* Part:Parts) if (UPrimitiveComponent* P=Cast<UPrimitiveComponent>(Part))
  {
   P->SetRenderInMainPass(false); P->SetRenderInDepthPass(false);
   P->SetVisibleInRayTracing(false); P->SetCastShadow(false);
  }
 }

 USceneComponent* Component(AS47Web* W,const FS47Obj& Parent)
 {
  const FS47WebNode* N=Parent?W->Node(Parent.I):nullptr;
  return N?N->Comp.Get():nullptr;
 }

 int32 Place(AS47Web* W,const FS47Obj& Parent,UStaticMesh* Asset,const TCHAR* Name,
             const FVector& At=FVector::ZeroVector,double Scale=1)
 {
  USceneComponent* Root=Component(W,Parent);
  if (!Root || !Asset) return -1;
  UStaticMeshComponent* C=NewObject<UStaticMeshComponent>(W,Name);
  C->SetupAttachment(Root); C->SetMobility(EComponentMobility::Movable);
  C->SetStaticMesh(Asset); C->SetRelativeTransform(FTransform::Identity);
  C->SetCollisionProfileName(TEXT("NoCollision"));
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->SetCollisionResponseToAllChannels(ECR_Ignore);
  C->SetGenerateOverlapEvents(false); C->SetCanEverAffectNavigation(false);
  C->SetCastShadow(true); W->AddInstanceComponent(C); C->RegisterComponent();
  const int32 Id=W->AdoptNode(C,Parent.I,Name);
  if (Id<0) { C->DestroyComponent(); return -1; }
  FS47Obj Obj=W->ObjAt(Id);
  Obj.SetPosition(At); Obj.SetRotation(FVector::ZeroVector); Obj.SetScale(FVector(Scale));
  return Id;
 }
}

struct FS47SaroPeriodProps::FState
{
 TWeakObjectPtr<AS47Web> Web;
 int32 PhoneBody=-1,PhoneHandset=-1,Jacket=-1,ConsoleBin=-1,DeskBin=-1;
 int32 Chairs[3]={-1,-1,-1},ChairParents[3]={-1,-1,-1};
 double Retry=0;
 bool bMissingReported=false,bTrimReported=false,bChairReported=false;

 void Reset()
 {
  if (AS47Web* W=Web.Get())
  {
   for (int32 Id:{PhoneBody,PhoneHandset,Jacket,ConsoleBin,DeskBin})
    if (const FS47WebNode* N=W->Node(Id); N && N->Comp.IsValid()) N->Comp->DestroyComponent();
   for (int32 Id:Chairs)
    if (const FS47WebNode* N=W->Node(Id); N && N->Comp.IsValid()) N->Comp->DestroyComponent();
  }
  PhoneBody=PhoneHandset=Jacket=ConsoleBin=DeskBin=-1;
  for (int32 I=0;I<3;++I) { Chairs[I]=-1; ChairParents[I]=-1; }
  Retry=0; bMissingReported=bTrimReported=bChairReported=false;
 }
};

FS47SaroPeriodProps::FS47SaroPeriodProps(US47Main* InMain):State(MakeUnique<FState>()),M(InMain)
{ bOff=FParse::Param(FCommandLine::Get(),TEXT("S47SaroPeriodPropsOff")) ||
       FParse::Param(FCommandLine::Get(),TEXT("S47Saro1986Off")); }
FS47SaroPeriodProps::~FS47SaroPeriodProps() { State->Reset(); }

void FS47SaroPeriodProps::Update(US47Main* Main,double Dt)
{
 static TMap<TWeakObjectPtr<US47Main>,TSharedPtr<FS47SaroPeriodProps>> Instances;
 for (auto It=Instances.CreateIterator();It;++It) if (!It.Key().IsValid()) It.RemoveCurrent();
 if (!Main) return;
 TSharedPtr<FS47SaroPeriodProps>& Instance=Instances.FindOrAdd(TWeakObjectPtr<US47Main>(Main));
 if (!Instance) Instance=MakeShared<FS47SaroPeriodProps>(Main);
 Instance->Step(Dt);
}

void FS47SaroPeriodProps::TryBuild()
{
 AS47Web* W=M->Web.Get();
 const FS47Obj Room=W->Obj(TEXT("room.group"));
 if (!Room) return;
 if (State->PhoneBody<0)
 {
  const FS47Obj Phone=W->Obj(TEXT("room.objs.phone")), Handset=W->Obj(TEXT("room.handset"));
  UStaticMesh* Body=OptionalMesh(TEXT("/Game/Licensed/Saro1986/SM_Saro_PhoneBody"));
  UStaticMesh* Receiver=OptionalMesh(TEXT("/Game/Licensed/Saro1986/SM_Saro_PhoneHandset"));
  if (Body && Receiver && Phone && Handset && Component(W,Phone) && Component(W,Handset))
  {
   MaskLegacy(Component(W,Phone));
   // Body is in phone-root coordinates. Receiver+cord are in the original
   // handset's (0,.085,-.045) frame, not a second stationary complete phone.
   State->PhoneBody=Place(W,Phone,Body,TEXT("SaroPeriodPhoneBody"));
   State->PhoneHandset=Place(W,Handset,Receiver,TEXT("SaroPeriodPhoneHandset"));
   UE_LOG(LogS47SaroPeriodProps,Display,TEXT("S47: SARO WE500 body and separate handset/cord bound to original phone/rattle/query nodes"));
  }
 }
 if (State->Jacket<0)
 {
  const FS47Obj Jacket=W->Obj(TEXT("room.objs.jacket"));
  UStaticMesh* Cloth=OptionalMesh(TEXT("/Game/Signal47/Art/Saro1986/SM_Saro_HangingJacket"));
  if (Cloth && Jacket && Component(W,Jacket))
  {
   MaskLegacy(Component(W,Jacket));
   // The source is authored locally at the old hook (5.93,0,2.42), with its
   // open neck/cuffs and reflective band inside that original query envelope.
   State->Jacket=Place(W,Jacket,Cloth,TEXT("SaroPeriodHangingJacket"));
   UE_LOG(LogS47SaroPeriodProps,Display,TEXT("S47: SARO folded cloth jacket follows original saved jacket visibility and existing interaction"));
  }
 }
 UStaticMesh* Bin=OptionalMesh(TEXT("/Game/Free/Office/retro_office_props/StaticMeshes/retro_office_props_TrashBin"));
 if (Bin)
 {
  auto ReplaceBin=[&](int32& Id,const TCHAR* EditId,const TCHAR* Name,int32 Geo,
                      const FBox& Bounds,const FVector& At,double Scale,int32 Triangles,float MaxBrightness)
  {
   if (Id>=0) return;
   S47SaroLegacyCleanup::FEdit Edit;
   Edit.Id=EditId; Edit.RootPath=TEXT("room.group"); Edit.GeometryIds={Geo};
   Edit.ThreeBounds=Bounds; Edit.MinTriangles=Edit.MaxTriangles=Triangles;
   Edit.MaxVertexBrightness=MaxBrightness;
   FString Why;
   if (!S47SaroLegacyCleanup::Apply(W,{Edit},&Why))
   {
    if (!State->bTrimReported) UE_LOG(LogS47SaroPeriodProps,Warning,TEXT("S47: SARO open-bin replacement waits for selected legacy skin: %s"),*Why);
    State->bTrimReported=true; return;
   }
   Id=Place(W,Room,Bin,Name,At,Scale);
   UE_LOG(LogS47SaroPeriodProps,Display,TEXT("S47: SARO hollow wastebasket %s placed; only selected legacy skin retired, paper and original floor collider retained"),Name);
  };
  ReplaceBin(State->ConsoleBin,TEXT("period-console-bin"),TEXT("SaroPeriodConsoleBin"),59,
    FBox(FVector(-3.94,-.005,-3.90),FVector(-3.56,.365,-3.50)),
    FVector(-3.75,.002,-3.70),1.17,48,-1);
  ReplaceBin(State->DeskBin,TEXT("period-supervisor-bin"),TEXT("SaroPeriodSupervisorBin"),219,
    FBox(FVector(1.819,-.005,2.199),FVector(2.141,.380,2.521)),
    FVector(1.98,.002,2.36),1.04,96,.45f);
 }
 if (!State->bMissingReported && (State->PhoneBody<0 || State->Jacket<0 || !Bin))
 {
  UE_LOG(LogS47SaroPeriodProps,Display,TEXT("S47: optional SARO period assets pending; original visuals retained for each absent asset"));
  State->bMissingReported=true;
 }
 TryBuildChairs();
}

void FS47SaroPeriodProps::TryBuildChairs()
{
 if (State->Chairs[0]>=0) return;
 AS47Web* W=M->Web.Get();
 UStaticMesh* Chair=OptionalMesh(TEXT("/Game/Licensed/DaleRealism/SM_Dale_Chair_H31"));
 // The approved chair has its feet at local z=0 and is 95cm tall. Wait for
 // native mesh preparation before replacing any of the six legacy pieces.
 if (!Chair || Chair->IsCompiling()) return;
 const FBox Box=Chair->GetBoundingBox();
 if (!Box.IsValid || !FMath::IsNearlyEqual(Box.GetSize().Z,95.,1.)) return;
 const FS47Obj Room=W->Obj(TEXT("room.group"));
 if (!Room) return;
 constexpr int32 Parents[3]={193,206,219};
 constexpr int32 Geos[3][2]={{93,94},{103,104},{113,114}};
 const FVector Positions[3]={{-2.23,0,-2.95},{.05,0,-2.95},{2.07,0,-2.95}};
 const TCHAR* Names[3]={TEXT("SaroPeriodConsoleChairLeft"),TEXT("SaroPeriodConsoleChairCentre"),TEXT("SaroPeriodConsoleChairRight")};
 TArray<UStaticMeshComponent*> Previous;
 bool Valid=true;
 for (int32 I=0;I<3 && Valid;++I)
 {
  const FS47WebNode* Parent=W->Node(Parents[I]);
  Valid=Parent && Parent->Parent==Room.I && Parent->Comp.IsValid() &&
        Parent->P.Equals(Positions[I],.0001) && Parent->S.Equals(FVector::OneVector,.0001);
  if (!Valid) break;
  for (int32 G:Geos[I])
  {
   UStaticMeshComponent* Found=nullptr; int32 Matches=0;
   for (int32 N=0;N<W->PartCount();++N)
    if (const FS47WebNode* Part=W->Node(N); Part && Part->Parent==Parents[I] && Part->Geo==G)
    { ++Matches; Found=Cast<UStaticMeshComponent>(Part->Comp.Get()); }
   if (Matches!=1 || !Found || !Found->GetStaticMesh()) { Valid=false; break; }
   Previous.Add(Found);
  }
 }
 if (!Valid || Previous.Num()!=6)
 {
  if (!State->bChairReported)
   UE_LOG(LogS47SaroPeriodProps,Warning,TEXT("S47: console chair replacement retained original: six exclusive chair geos/parents differ"));
  State->bChairReported=true; return;
 }
 // Every source identity and all three parents passed before placing anything.
 // Keep the original parent yaw. The Dale asset faces +Three Z, while the
 // consoles are to the north, so only the new child receives an extra PI.
 for (int32 I=0;I<3;++I)
 {
  State->Chairs[I]=Place(W,W->ObjAt(Parents[I]),Chair,Names[I]);
  if (State->Chairs[I]<0)
  {
   for (int32 J=0;J<3;++J)
   {
    if (const FS47WebNode* N=W->Node(State->Chairs[J]); N && N->Comp.IsValid()) N->Comp->DestroyComponent();
    State->Chairs[J]=-1;
   }
   return;
  }
  W->ObjAt(State->Chairs[I]).SetRotation(FVector(0,PI,0));
 }
 // Mask exactly these six exclusive meshes. Their original nodes, query
 // shapes and 56cm floor rectangles remain; no shared clutter mesh is cut.
 for (UStaticMeshComponent* C:Previous)
 {
  C->SetRenderInMainPass(false); C->SetRenderInDepthPass(false);
  C->SetVisibleInRayTracing(false); C->SetCastShadow(false);
 }
 for (int32 I=0;I<3;++I) State->ChairParents[I]=Parents[I];
 UE_LOG(LogS47SaroPeriodProps,Display,TEXT("S47: three full-size brown vinyl console chairs installed; exclusive geos93/94,103/104,113/114 masked, original parents/yaw/colliders retained"));
}

void FS47SaroPeriodProps::Step(double Dt)
{
 if (bOff || !M || !M->Web || !M->Web->IsLoaded()) return;
 AS47Web* W=M->Web.Get();
 if (State->Web.Get()!=W) { State->Reset(); State->Web=W; }
 State->Retry-=FMath::Max(0.,Dt);
 if (State->Retry<=0) { TryBuild(); State->Retry=1.; }
 const bool Play=M->Mode==US47Main::EMode::Play;
 auto Follow=[&](int32 Id,const TCHAR* Source)
 {
  if (Id>=0) W->ObjAt(Id).SetVisible(Play && W->Obj(Source).Shown());
 };
 Follow(State->PhoneBody,TEXT("room.objs.phone"));
 Follow(State->PhoneHandset,TEXT("room.handset"));
 Follow(State->Jacket,TEXT("room.objs.jacket"));
 Follow(State->ConsoleBin,TEXT("room.group")); Follow(State->DeskBin,TEXT("room.group"));
 for (int32 I=0;I<3;++I) if (State->Chairs[I]>=0)
  W->ObjAt(State->Chairs[I]).SetVisible(Play && W->ObjAt(State->ChairParents[I]).Shown());
}

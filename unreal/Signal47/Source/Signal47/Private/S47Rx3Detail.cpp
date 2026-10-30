#include "S47Rx3Detail.h"

#include "S47Main.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Rx3Detail, Log, All);

struct FS47Rx3Detail::FState
{
 TWeakObjectPtr<AS47Web> Web;
 TWeakObjectPtr<UStaticMeshComponent> Mesh;
 TWeakObjectPtr<UStaticMeshComponent> Console;
 int32 Node = -1;
 int32 ConsoleNode = -1;
 double RetryWait = 0;
 bool bMissingReported = false;
 void Reset()
 {
  if (UStaticMeshComponent* C = Mesh.Get()) C->DestroyComponent();
  if (UStaticMeshComponent* C = Console.Get()) C->DestroyComponent();
  Mesh.Reset(); Node = -1; RetryWait = 0; bMissingReported = false;
  Console.Reset(); ConsoleNode = -1;
 }
};

FS47Rx3Detail::FS47Rx3Detail(US47Main* InM) : State(MakeUnique<FState>()), M(InM)
{
 bOff = FParse::Param(FCommandLine::Get(), TEXT("S47Rx3DetailOff")) ||
        FParse::Param(FCommandLine::Get(), TEXT("S47Saro1986Off"));
}
FS47Rx3Detail::~FS47Rx3Detail() { State->Reset(); }

void FS47Rx3Detail::Update(US47Main* Main, double Dt)
{
 static TMap<TWeakObjectPtr<US47Main>, TSharedPtr<FS47Rx3Detail>> Instances;
 for (auto It = Instances.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
 if (!Main) return;
 TSharedPtr<FS47Rx3Detail>& Instance = Instances.FindOrAdd(TWeakObjectPtr<US47Main>(Main));
 if (!Instance) Instance = MakeShared<FS47Rx3Detail>(Main);
 Instance->Step(Dt);
}

void FS47Rx3Detail::TryBuild()
{
 AS47Web* W = M->Web.Get();
 const FS47Obj Room = W->Obj(TEXT("room.group"));
 const FS47WebNode* Parent = Room ? W->Node(Room.I) : nullptr;
 if (!Parent || !Parent->Comp.IsValid() || !W->NodeWorldThree(Room.I).Equals(FTransform::Identity, .0001)) return;
 auto Add=[&](const TCHAR* Name,const TCHAR* ComponentName,TWeakObjectPtr<UStaticMeshComponent>& Target,int32& NodeId)
 {
 if (Target.IsValid()) return false;
 const FString Package=FString(TEXT("/Game/Signal47/Art/Saro1986/"))+Name;
 if (!FPackageName::DoesPackageExist(Package))
 {
  if (!State->bMissingReported)
  {
   UE_LOG(LogS47Rx3Detail,Display,TEXT("S47: SARO detail %s unavailable; existing live equipment retained"),Name);
   State->bMissingReported = true;
  }
  return false;
 }
 const FString Path = Package + TEXT(".")+Name;
 UStaticMesh* Asset = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
 if (!Asset || Asset->IsCompiling()) return false;
 USceneComponent* Attach = Parent->Comp.Get();
 auto* C = NewObject<UStaticMeshComponent>(W,ComponentName);
 C->SetupAttachment(Attach); C->SetMobility(EComponentMobility::Movable); C->SetStaticMesh(Asset);
 C->SetCollisionProfileName(TEXT("NoCollision")); C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 C->SetCollisionResponseToAllChannels(ECR_Ignore); C->SetGenerateOverlapEvents(false);
 C->SetCanEverAffectNavigation(false); C->SetCastShadow(true);
 W->AddInstanceComponent(C); C->RegisterComponent(); C->SetWorldTransform(FTransform::Identity);
 Target=C; NodeId=W->AdoptNode(C,Room.I,ComponentName); return true;
 };
 if (Add(TEXT("SM_Saro1986_RX3Detail"),TEXT("Saro1986_RX3Detail"),State->Mesh,State->Node))
  UE_LOG(LogS47Rx3Detail,Display,TEXT("S47: RX3 passive19in hardware placed; original24 LEDs, power lever, service paper and collider retained"));
 if (Add(TEXT("SM_Saro1986_ConsoleTrim"),TEXT("Saro1986_ConsoleTrim"),State->Console,State->ConsoleNode))
  UE_LOG(LogS47Rx3Detail,Display,TEXT("S47: SARO walnut console edges, drawer fronts and physical pulls placed; original desktop and live equipment retained"));
}

void FS47Rx3Detail::Step(double Dt)
{
 if (bOff || !M || !M->Web || !M->Web->IsLoaded()) return;
 AS47Web* W = M->Web.Get();
 if (State->Web.Get() != W) { State->Reset(); State->Web = W; }
 if (!State->Mesh.IsValid() || !State->Console.IsValid())
 {
  State->RetryWait -= FMath::Max(0., Dt);
  if (State->RetryWait <= 0) { TryBuild(); State->RetryWait = 1.; }
 }
 const bool Visible = M->Mode == US47Main::EMode::Play && M->World &&
                      M->World->Area == TEXT("saro") && W->Obj(TEXT("room.group")).Shown();
 if (State->Node>=0) W->ObjAt(State->Node).SetVisible(Visible);
 if (State->ConsoleNode>=0) W->ObjAt(State->ConsoleNode).SetVisible(Visible);
}

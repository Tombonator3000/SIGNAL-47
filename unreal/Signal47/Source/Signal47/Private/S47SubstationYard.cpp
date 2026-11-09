#include "S47SubstationYard.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SubstationYard, Log, All);

namespace S47SubstationYard
{
namespace
{
 // Tools/substation/geometry.py: posts 2.48 m (tube to z 2.48, 4.8 cm), runs split every 2.7 m at most, the gate gap x -2.2..2.2 at y -7.2.
 constexpr double PostTop = 2.48, ArmLength = .45, Strand = .0065, BarbEvery = .14;
 FVector Site(double X, double Y, double Z) { return FVector(7890 - X, -1.10 + Z, 536.20 + Y); }
 FVector Site(const FVector2D& P, double Z) { return Site(P.X, P.Y, Z); }
 // A unit shape (100 cm) stretched from A to B (three metres), Thick metres across.
 FTransform Along(const FVector& A, const FVector& B, double Thick)
 {
  const FVector UA = S47Three::ToUE(A), UB = S47Three::ToUE(B);
  const FVector D = UB - UA;
  return FTransform(FRotationMatrix::MakeFromZ(D.GetSafeNormal()).ToQuat(), (UA + UB) * .5, FVector(Thick, Thick, D.Size() / 100.));
 }
 UInstancedStaticMeshComponent* Batch(AS47Web* W, USceneComponent* Parent, UStaticMesh* Mesh, UMaterialInterface* Mat, const TCHAR* Name)
 {
  auto* C = NewObject<UInstancedStaticMeshComponent>(W, Name);
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(Parent); C->SetStaticMesh(Mesh); C->SetMaterial(0, Mat);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCastShadow(true); C->SetCanEverAffectNavigation(false);
  C->RegisterComponent();
  return C;
 }
}

bool Build(AS47Web* W)
{
 if (!W || FParse::Param(FCommandLine::Get(), TEXT("S47SubstationYardOff")) || FParse::Param(FCommandLine::Get(), TEXT("S47PropsOff"))) return false;
 // Only beside the kit as it stands: its fence and its transformer, placed by Config/Props/substation.json.
 int32 FenceNode = -1, TransformerNode = -1;
 for (int32 I = 0; I < W->PartCount(); ++I)
  if (const FS47WebNode* N = W->Node(I))
  {
   if (N->Name == TEXT("prop:substationFence")) FenceNode = I;
   if (N->Name == TEXT("prop:substationTransformer")) TransformerNode = I;
  }
 const FS47WebNode* Fence = W->Node(FenceNode);
 const FS47WebNode* Transformer = W->Node(TransformerNode);
 UStaticMeshComponent* TransformerMesh = Transformer ? Cast<UStaticMeshComponent>(Transformer->Comp.Get()) : nullptr;
 if (!Fence || !Fence->Comp.IsValid() || !TransformerMesh || !TransformerMesh->GetStaticMesh())
 { UE_LOG(LogS47SubstationYard, Warning, TEXT("S47: substation yard: the kit's fence or transformer is not placed; nothing added")); return false; }
 UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
 UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 UMaterialInterface* Steel = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SubstationDetail/Materials/M_SubstationDetail_GalvanizedSteel.M_SubstationDetail_GalvanizedSteel"));
 UMaterialInterface* Concrete = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SaroSite/Materials/M_SaroSite_Concrete.M_SaroSite_Concrete"));
 if (!Cube || !Cylinder || !Steel || !Concrete) { UE_LOG(LogS47SubstationYard, Warning, TEXT("S47: substation yard: shapes or materials missing; nothing added")); return false; }
 USceneComponent* Parent = Fence->Comp.Get();
 const int32 ParentNode = Fence->Parent;

 // ---- barbed wire: an arm out and up at 45 degrees from the top of every post, three strands along each run ----
 UInstancedStaticMeshComponent* Arms = Batch(W, Parent, Cube, Steel, TEXT("S47SubstationYard_WireArms"));
 UInstancedStaticMeshComponent* Strands = Batch(W, Parent, Cylinder, Steel, TEXT("S47SubstationYard_WireStrands"));
 UInstancedStaticMeshComponent* Barbs = Batch(W, Parent, Cube, Steel, TEXT("S47SubstationYard_WireBarbs"));
 struct FRun { FVector2D A, B, Out; };
 const FRun Runs[] = { { { -9.2, -7.2 }, { -2.2, -7.2 }, { 0, -1 } }, { { 2.2, -7.2 }, { 9.2, -7.2 }, { 0, -1 } },
  { { -9.2, -7.2 }, { -9.2, 7.2 }, { -1, 0 } }, { { 9.2, -7.2 }, { 9.2, 7.2 }, { 1, 0 } }, { { -9.2, 7.2 }, { 9.2, 7.2 }, { 0, 1 } } };
 const double Rise = ArmLength / FMath::Sqrt(2.);
 int32 NArms = 0, NStrands = 0, NBarbs = 0;
 for (const FRun& R : Runs)
 {
  const int32 Count = FMath::CeilToInt(FVector2D::Distance(R.A, R.B) / 2.7);
  for (int32 I = 0; I <= Count; ++I)
  {
   const FVector2D P = R.A + (R.B - R.A) * (double(I) / Count);
   Arms->AddInstance(Along(Site(P, PostTop - .06), Site(P + R.Out * Rise, PostTop - .06 + Rise), .028), true); ++NArms;
  }
  for (int32 K = 1; K <= 3; ++K)
  {
   const double T = Rise * K / 3;
   const FVector A = Site(R.A + R.Out * T, PostTop - .06 + T), B = Site(R.B + R.Out * T, PostTop - .06 + T);
   Strands->AddInstance(Along(A, B, Strand), true); ++NStrands;
   // the barbs: a short twist of wire across the strand every 14 cm, turned a little each time
   const int32 NB = FMath::FloorToInt(FVector::Distance(A, B) / BarbEvery);
   const FVector UA = S47Three::ToUE(A), UB = S47Three::ToUE(B), Dir = (UB - UA).GetSafeNormal();
   for (int32 J = 1; J < NB; ++J)
   {
    const FVector At = UA + (UB - UA) * (double(J) / NB);
    const FQuat Turn(Dir, FMath::DegreesToRadians(37. * J + 19. * K));
    const FQuat Q = Turn * FRotationMatrix::MakeFromZ(Dir).ToQuat();
    Barbs->AddInstance(FTransform(Q, At, FVector(.028, .0035, .0035)), true); ++NBarbs;
   }
  }
 }
 W->AdoptNode(Arms, ParentNode, TEXT("substationYard:wireArms"));
 W->AdoptNode(Strands, ParentNode, TEXT("substationYard:wireStrands"));
 W->AdoptNode(Barbs, ParentNode, TEXT("substationYard:wireBarbs"));

 // ---- a station-service transformer: the kit's own, at 0.55, on a 3.2 x 1.7 m pad 15 cm over the gravel at x -6.6, y 2.4 ----
 const FVector2D T2(-6.6, 2.4);
 constexpr double Scale = .55, PadTop = .15;
 auto* Pad = NewObject<UStaticMeshComponent>(W, TEXT("S47SubstationYard_Pad2"));
 Pad->SetMobility(EComponentMobility::Movable); Pad->SetupAttachment(Parent); Pad->SetStaticMesh(Cube); Pad->SetMaterial(0, Concrete);
 Pad->SetCollisionEnabled(ECollisionEnabled::NoCollision); Pad->SetCastShadow(true); Pad->RegisterComponent();
 Pad->SetWorldTransform(FTransform(FQuat::Identity, S47Three::ToUE(Site(T2, PadTop - .25)), FVector(3.2, 1.7, .5)));
 W->AdoptNode(Pad, ParentNode, TEXT("substationYard:pad2"));
 const FTransform Main = TransformerMesh->GetComponentTransform();
 auto* Second = NewObject<UStaticMeshComponent>(W, TEXT("S47SubstationYard_Transformer2"));
 Second->SetMobility(EComponentMobility::Movable); Second->SetupAttachment(Parent); Second->SetStaticMesh(TransformerMesh->GetStaticMesh());
 for (int32 I = 0; I < TransformerMesh->GetNumMaterials(); ++I) Second->SetMaterial(I, TransformerMesh->GetMaterial(I));
 Second->SetCollisionEnabled(ECollisionEnabled::NoCollision); Second->SetCastShadow(true); Second->RegisterComponent();
 // The kit is authored round its site origin on the pad's top (z 0): the copy's origin goes to its own pad's top.
 Second->SetWorldTransform(FTransform(Main.GetRotation(), S47Three::ToUE(Site(T2, PadTop)), Main.GetScale3D() * Scale));
 W->AdoptNode(Second, ParentNode, TEXT("substationYard:transformer2"));
 UE_LOG(LogS47SubstationYard, Display, TEXT("S47: substation yard: barbed wire on %d arms, %d strands, %d barbs; a second transformer at %.2f on its pad at world %s"),
  NArms, NStrands, NBarbs, Scale, *Site(T2, PadTop).ToString());
 return true;
}
}

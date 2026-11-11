#include "S47SaroWallWatch.h"
#include "S47Main.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

// WORLD-04 (N1110): see the header.
namespace
{
 struct FPiece
 {
  FString Name, Mesh;
  bool bExists = false, bNode = false, bShown = false, bVisible = false, bHidden = false, bRegistered = false, bRender = false;
  int32 Triangles = 0, Materials = 0, NullMaterials = 0, Radius = 0;
  FString Line() const
  {
   if (!bExists) return Name + TEXT(": absent");
   return FString::Printf(TEXT("%s: node %d shown %d visible %d hiddenInGame %d registered %d render %d tris %d materials %d null %d radius %dcm mesh %s"),
    *Name, bNode, bShown, bVisible, bHidden, bRegistered, bRender, Triangles, Materials, NullMaterials, Radius, *Mesh);
  }
  bool Same(const FPiece& O) const { return Line() == O.Line(); }
 };
 struct FSnap { FString Area; TArray<FPiece> Pieces; };
 TMap<TWeakObjectPtr<US47Main>, FSnap> Before;

 FSnap Take(US47Main* M)
 {
  FSnap S;
  AS47Web* W = M ? M->Web.Get() : nullptr;
  if (!W || !W->IsLoaded()) return S;
  S.Area = W->Area();
  const FS47Obj Annex = W->Obj(TEXT("annex.group"));
  for (const TCHAR* Key : {TEXT("office:authoredRoom"), TEXT("saro:rooms:corridorFinishes"), TEXT("office:partitionRecordsWest"), TEXT("office:partitionRecords")})
  {
   FPiece P; P.Name = Key;
   const FS47Obj Obj = Annex ? Annex.Find(Key) : FS47Obj();
   const FS47WebNode* N = Obj ? W->Node(Obj.I) : nullptr;
   P.bExists = N != nullptr;
   if (N)
   {
    P.bNode = N->bVis; P.bShown = Obj.Shown();
    if (auto* C = Cast<UStaticMeshComponent>(N->Comp.Get()))
    {
     P.bVisible = C->IsVisible(); P.bHidden = C->bHiddenInGame; P.bRegistered = C->IsRegistered(); P.bRender = C->IsRenderStateCreated();
     P.Radius = int32(C->Bounds.SphereRadius); P.Materials = C->GetNumMaterials();
     for (int32 I = 0; I < P.Materials; ++I) if (!C->GetMaterial(I)) ++P.NullMaterials;
     if (const UStaticMesh* Mesh = C->GetStaticMesh()) { P.Mesh = Mesh->GetName(); P.Triangles = Mesh->GetNumTriangles(0); }
    }
   }
   S.Pieces.Add(P);
  }
  return S;
 }
 // Empty when nothing changed, otherwise what.
 FString Diff(const FSnap& A, const FSnap& B)
 {
  FString Out;
  for (int32 I = 0; I < A.Pieces.Num() && I < B.Pieces.Num(); ++I)
   if (!A.Pieces[I].Same(B.Pieces[I])) Out += FString::Printf(TEXT("[%s] -> [%s] "), *A.Pieces[I].Line(), *B.Pieces[I].Line());
  return Out;
 }
}

namespace S47SaroWallWatch
{
 void Mark(US47Main* Main, const TCHAR* Phase)
 {
  if (!Main) return;
  const FSnap Now = Take(Main);
  if (Now.Pieces.IsEmpty()) return;
  for (const FPiece& P : Now.Pieces) UE_LOG(LogTemp, Display, TEXT("S47: SARO wall watch %s (area %s) %s"), Phase, *Now.Area, *P.Line());
  const TWeakObjectPtr<US47Main> Key(Main);
  if (FCString::Strcmp(Phase, TEXT("before rest")) == 0) { Before.FindOrAdd(Key) = Now; return; }
  if (const FSnap* Was = Before.Find(Key))
  {
   const FString D = Diff(*Was, Now);
   if (!D.IsEmpty()) UE_LOG(LogTemp, Warning, TEXT("S47: SARO wall watch: the office walls changed across Rest: %s"), *D);
   Before.Remove(Key);
  }
 }

 FString Check(US47Main* Main)
 {
  const FSnap Now = Take(Main);
  if (Now.Pieces.IsEmpty()) return TEXT("SARO walls: the world is not loaded");
  Mark(Main, TEXT("F10 check"));
  FString Bad;
  for (const FPiece& P : Now.Pieces)
   if (P.bExists && P.bNode && (!P.bVisible || P.bHidden || !P.bRegistered || !P.bRender || P.Triangles == 0 || P.NullMaterials > 0)) Bad += P.Name + TEXT(" ");
  FString Why;
  const bool bTest = SelfTest(Why);
  return (Bad.IsEmpty() ? FString(TEXT("SARO walls: every piece that should draw does")) : TEXT("SARO walls: NOT drawing: ") + Bad) + (bTest ? TEXT("; self-test ok") : TEXT("; self-test FAILED: ") + Why);
 }

 bool SelfTest(FString& Why)
 {
  FSnap A; FPiece P; P.Name = TEXT("office:authoredRoom"); P.bExists = P.bNode = P.bShown = P.bVisible = P.bRegistered = P.bRender = true; P.Triangles = 100; P.Materials = 3;
  A.Pieces.Add(P);
  FSnap B = A;
  if (!Diff(A, B).IsEmpty()) { Why = TEXT("equal snapshots differ"); return false; }
  B.Pieces[0].bVisible = false;
  if (Diff(A, B).IsEmpty()) { Why = TEXT("a hidden room was not noticed"); return false; }
  B = A; B.Pieces[0].Triangles = 40;
  if (Diff(A, B).IsEmpty()) { Why = TEXT("a mesh swap was not noticed"); return false; }
  return true;
 }
}

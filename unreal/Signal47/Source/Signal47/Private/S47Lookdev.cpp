#include "S47Lookdev.h"
#include "S47Main.h"
#include "S47Web.h"
#include "S47SaroLegacyCleanup.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectIterator.h"

namespace S47Lookdev
{
namespace
{
 struct FRule
 {
  FString Source, Replacement, Scope;
  TArray<FString> MeshPrefixes;
  TWeakObjectPtr<UMaterialInterface> Loaded;
  bool bAttempted=false;
 };
 struct FBinding
 {
  TWeakObjectPtr<UStaticMeshComponent> Component;
  TWeakObjectPtr<UMaterialInterface> Applied;
  int32 Slot=0, Rule=0;
 };
 struct FState
 {
  TArray<FRule> Rules;
  TArray<FBinding> Bindings;
  TSet<FString> Missing;
  FString Error;
  double UntilScan=0;
  bool bInitialized=false;
 };
 TMap<TWeakObjectPtr<const AS47Web>,FState> States;
 bool Enabled() { return !FParse::Param(FCommandLine::Get(),TEXT("S47LookdevOff")); }
 void Initialize(FState& S)
 {
  S.bInitialized=true;
  FString Text; TSharedPtr<FJsonObject> Root;
  if (!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectConfigDir()/TEXT("Props/lookdev-bindings.json"))) ||
      !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root) || !Root.IsValid())
  { S.Error=TEXT("lookdev-bindings.json unavailable or invalid; original materials retained"); return; }
  double Version=0;
  if (!Root->TryGetNumberField(TEXT("schema_version"),Version) || Version!=1) { S.Error=TEXT("unsupported lookdev binding schema"); return; }
  const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
  if (!Root->TryGetArrayField(TEXT("bindings"),Rows)) { S.Error=TEXT("missing bindings array"); return; }
  for (const auto& V:*Rows)
  {
   const auto O=V->AsObject(); if (!O) continue;
   FRule R; const TArray<TSharedPtr<FJsonValue>>* Prefixes=nullptr;
   if (!O->TryGetStringField(TEXT("source_material_path"),R.Source) ||
       !O->TryGetStringField(TEXT("replacement_material_path"),R.Replacement) ||
       !O->TryGetArrayField(TEXT("mesh_prefixes"),Prefixes)) continue;
   O->TryGetStringField(TEXT("scope"),R.Scope);
   if (!R.Source.StartsWith(TEXT("/Game/")) || !R.Replacement.StartsWith(TEXT("/Game/Signal47/Art/LookdevProduction/"))) continue;
   for (const auto& P:*Prefixes) if (!P->AsString().IsEmpty()) R.MeshPrefixes.Add(P->AsString());
   if (!R.MeshPrefixes.IsEmpty()) S.Rules.Add(MoveTemp(R));
  }
 }
 bool SourceMatches(UMaterialInterface* Material,const FString& Source)
 {
  // A live instance can carry authored overrides. Match its explicit parent, never its display name.
  for (int32 Depth=0;Material && Depth<16;++Depth)
  {
   if (Material->GetPathName()==Source) return true;
   auto* Instance=Cast<UMaterialInstance>(Material);
   Material=Instance?Instance->Parent.Get():nullptr;
  }
  return false;
 }
}
void Update(US47Main* Main,double Dt)
{
 if (!Enabled() || !Main || !Main->Web) return;
 auto* W=Main->Web.Get();
 for (auto It=States.CreateIterator();It;++It) if (!It.Key().IsValid()) It.RemoveCurrent();
 auto& S=States.FindOrAdd(W);
 if (!S.bInitialized) Initialize(S);
 S.UntilScan-=Dt; if (S.UntilScan>0) return; S.UntilScan=2;
 S.Bindings.RemoveAll([](const FBinding& B) { return !B.Component.IsValid() || !B.Component->GetStaticMesh() || B.Component->GetMaterial(B.Slot)!=B.Applied.Get(); });
 for (TObjectIterator<UStaticMeshComponent> It;It;++It)
 {
  auto* C=*It; UStaticMesh* Mesh=C->GetStaticMesh();
  if (!Mesh || C->GetWorld()!=W->GetWorld() || C->IsTemplate()) continue;
  const FString MeshPath=S47SaroLegacyCleanup::SourceAssetPath(Mesh);
  for (int32 Slot=0;Slot<C->GetNumMaterials();++Slot)
  {
   auto* Current=C->GetMaterial(Slot); if (!Current) continue;
   if (S.Bindings.ContainsByPredicate([C,Slot,Current](const FBinding& B) { return B.Component.Get()==C && B.Slot==Slot && B.Applied.Get()==Current; })) continue;
   // Runtime drivers retain their MID pointer. Replacing it would orphan subsequent
   // power/tint updates, so this pass only binds static authored materials/MICs.
   if (Cast<UMaterialInstanceDynamic>(Current)) continue;
   for (int32 RI=0;RI<S.Rules.Num();++RI)
   {
    auto& R=S.Rules[RI]; bool bMesh=false;
    for (const auto& Prefix:R.MeshPrefixes) if (MeshPath.StartsWith(Prefix)) { bMesh=true; break; }
    if (!bMesh || !SourceMatches(Current,R.Source)) continue;
    if (!R.bAttempted || (!R.Loaded.IsValid() && !S.Missing.Contains(R.Replacement)))
    {
     R.Loaded=LoadObject<UMaterialInterface>(nullptr,*R.Replacement); R.bAttempted=true;
     if (!R.Loaded.IsValid()) S.Missing.Add(R.Replacement);
    }
    if (!R.Loaded.IsValid()) break;
    auto* Replacement=UMaterialInstanceDynamic::Create(R.Loaded.Get(),C);
    if (auto* Instance=Cast<UMaterialInstance>(Current)) Replacement->CopyInterpParameters(Instance);
    C->SetMaterial(Slot,Replacement);
    S.Bindings.Add({C,Replacement,Slot,RI});
    break;
   }
  }
 }
}
TSharedPtr<FJsonObject> Readback(const AS47Web* W)
{
 auto O=MakeShared<FJsonObject>(); O->SetBoolField(TEXT("enabled"),Enabled());
 const auto* S=States.Find(W); O->SetNumberField(TEXT("rules"),S?S->Rules.Num():0);
 TArray<TSharedPtr<FJsonValue>> Bound,Missing;
 if (S)
 {
  O->SetStringField(TEXT("error"),S->Error);
  for (const auto& B:S->Bindings)
  {
   if (!B.Component.IsValid() || !B.Component->GetStaticMesh() || !S->Rules.IsValidIndex(B.Rule) || B.Component->GetMaterial(B.Slot)!=B.Applied.Get()) continue;
   const auto& R=S->Rules[B.Rule]; auto Row=MakeShared<FJsonObject>();
   Row->SetStringField(TEXT("component"),B.Component->GetPathName());
   Row->SetStringField(TEXT("mesh"),B.Component->GetStaticMesh()->GetPathName());
   Row->SetStringField(TEXT("sourceMesh"),S47SaroLegacyCleanup::SourceAssetPath(B.Component->GetStaticMesh()));
   Row->SetNumberField(TEXT("slot"),B.Slot); Row->SetStringField(TEXT("scope"),R.Scope);
   Row->SetStringField(TEXT("source"),R.Source); Row->SetStringField(TEXT("replacement"),R.Replacement);
   Bound.Add(MakeShared<FJsonValueObject>(Row));
  }
  for (const auto& P:S->Missing) Missing.Add(MakeShared<FJsonValueString>(P));
 }
 O->SetNumberField(TEXT("boundSlots"),Bound.Num()); O->SetArrayField(TEXT("bindings"),Bound);
 O->SetArrayField(TEXT("missing"),Missing); return O;
}
}

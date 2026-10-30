#include "S47TripoNpcsTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"

bool S47TripoNpcsTest::Check(const TArray<UStaticMeshComponent*>& Components, TArray<FString>& Errors)
{
 const int32 Before = Errors.Num();
 static const TCHAR* Roles[] = {
  TEXT("dale"), TEXT("patron"), TEXT("waitress"), TEXT("nora"), TEXT("attendant"),
  TEXT("deputy"), TEXT("earl"), TEXT("radio_woman"), TEXT("souvenir_man"),
  TEXT("mara_ortiz"), TEXT("hereford_25"), TEXT("shadow_man"), TEXT("tall_one"), TEXT("crawler")
 };
 if (Components.Num() != UE_ARRAY_COUNT(Roles))
  Errors.Add(FString::Printf(TEXT("Expected 14 Tripo components, found %d"), Components.Num()));

 TSet<FString> Seen;
 for (UStaticMeshComponent* Component : Components)
 {
  if (!IsValid(Component)) { Errors.Add(TEXT("Missing Tripo component")); continue; }
  FString Role;
  for (const FName& Tag : Component->ComponentTags)
   if (Tag.ToString().StartsWith(TEXT("S47Tripo:"))) Role = Tag.ToString().RightChop(9);
  bool Known = false;
  for (const TCHAR* Expected : Roles) Known |= Role == Expected;
  if (!Known || Seen.Contains(Role))
  {
   Errors.Add(TEXT("Unknown or repeated Tripo role: ") + Role);
   continue;
  }
  Seen.Add(Role);

  UStaticMesh* Mesh = Component->GetStaticMesh();
  const FString Name = TEXT("SM_Tripo_") + Role;
  const FString ExpectedPath = TEXT("/Game/Licensed/TripoNPC/") + Name + TEXT(".") + Name;
  if (!Mesh || Mesh->GetPathName() != ExpectedPath || Mesh->GetNumLODs() < 1)
  {
   Errors.Add(Role + TEXT(": imported static mesh is missing or replaced by a fallback"));
   continue;
  }
  const FBox Box = Mesh->GetBoundingBox().TransformBy(Component->GetComponentTransform());
  const FVector Size = Box.GetSize();
  double MinimumHeight = 70., MaximumHeight = 250.;
  if (Role == TEXT("tall_one")) { MinimumHeight = 150.; MaximumHeight = 350.; }
  else if (Role == TEXT("crawler")) { MinimumHeight = 20.; MaximumHeight = 180.; }
  else if (Role == TEXT("hereford_25")) { MinimumHeight = 60.; MaximumHeight = 220.; }
  if (!Box.IsValid || Box.Min.ContainsNaN() || Box.Max.ContainsNaN() ||
      Size.X < 5. || Size.Y < 5. || Size.Z < MinimumHeight || Size.Z > MaximumHeight)
   Errors.Add(FString::Printf(TEXT("%s: implausible world bounds in cm %s"), *Role, *Size.ToCompactString()));
  if (!Component->IsRegistered() || Component->GetCollisionEnabled() != ECollisionEnabled::NoCollision ||
      Component->GetGenerateOverlapEvents())
   Errors.Add(Role + TEXT(": mannequin must be registered and have no collision or overlaps"));
  if (Component->GetNumMaterials() < 1) Errors.Add(Role + TEXT(": no material slots"));
  for (int32 Slot = 0; Slot < Component->GetNumMaterials(); ++Slot)
  {
   UMaterialInterface* Material = Component->GetMaterial(Slot);
   if (!Material || Material == UMaterial::GetDefaultMaterial(MD_Surface))
    Errors.Add(FString::Printf(TEXT("%s: material slot %d is missing or uses the engine fallback"), *Role, Slot));
  }
 }
 for (const TCHAR* Role : Roles)
  if (!Seen.Contains(Role)) Errors.Add(FString(Role) + TEXT(": role was not instantiated"));
 return Errors.Num() == Before;
}

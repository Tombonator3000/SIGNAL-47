#pragma once
#include "CoreMinimal.h"

class AS47Web;
class UStaticMesh;
class UStaticMeshComponent;
class USceneComponent;

/** ENV02: fire lukkede boliger og to flyttede originalskilt.
 * Build kalles rett etter TB2, før US47World kopierer veihindringene.
 * Et ufullstendig/ukjent native-sett beholder alle forgjengere og kollisjoner.
 */
class FS47CalicheHomes
{
public:
 static TSharedPtr<FS47CalicheHomes> Build(AS47Web* W, bool bTownReady);
 void Update();
 bool Audit(TArray<FString>& Lines) const;
private:
 struct FProtected
 {
  int32 Node=-1,Parent=-1,Geo=-1;
  TArray<int32> Mats;
  TWeakObjectPtr<USceneComponent> Comp;
  TWeakObjectPtr<UStaticMesh> Mesh;
 };
 struct FInstalled
 {
  int32 Node=-1,Tris=0;
  TWeakObjectPtr<UStaticMeshComponent> Comp;
  TWeakObjectPtr<UStaticMesh> Mesh;
 };
 struct FMovedCollider
 {
  int32 Walk=-1,Drive=-1;
  double Old[4]={},At[4]={};
 };
 TWeakObjectPtr<AS47Web> Web;
 TArray<FProtected> Protected;
 TArray<FInstalled> Installed;
 TArray<int32> Retired;
 TArray<FMovedCollider> Moves;
 int32 HouseParts=0,SignParts=0,PatchParts=0;
};

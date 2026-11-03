#pragma once
#include "CoreMinimal.h"

class AS47Web;
class FJsonObject;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** ART01: two exterior prints behind the active CalicheAccess Plaza glass.
 * Install after CalicheAccess. No geometry or door interaction is mutated.
 * Update is called by the existing web owner, with no independent tick/timer.
 */
class FS47CinemaPosters
{
public:
 static void Install(AS47Web* W);
 static TSharedPtr<FS47CinemaPosters> Build(AS47Web* W, bool bTownReady);
 void Update();
 bool Audit(TArray<FString>& Lines) const;
 TSharedPtr<FJsonObject> Readback() const;

private:
 struct FOriginal
 {
  int32 Node=-1,Parent=-1,MaterialSlot=-1;
  TWeakObjectPtr<UStaticMeshComponent> Comp;
  TWeakObjectPtr<UStaticMesh> Mesh;
  TArray<TWeakObjectPtr<UMaterialInterface>> Materials;
  FTransform Transform;
 };
 struct FPrint
 {
  int32 Node=-1;
  TWeakObjectPtr<UStaticMeshComponent> Comp;
 };
 bool OriginalsIntact() const;
 bool ShouldShow() const;
 TWeakObjectPtr<AS47Web> Web;
 FOriginal Shell,Glass;
 int32 Proxy=-1,ProxyParent=-1,ProxyGeo=-1;
 TWeakObjectPtr<USceneComponent> ProxyComp;
 TArray<int32> ProxyMats;
 FTransform ProxyTransform;
 TArray<FPrint> Prints;
 TSharedPtr<FJsonObject> Binding;
};

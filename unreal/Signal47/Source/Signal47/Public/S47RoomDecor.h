#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
class US47Main;
class AS47Web;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// Freeform only: frames on the control room's bare west wall that hold the player's own developed prints.
class FS47RoomDecor
{
public:
 explicit FS47RoomDecor(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step();
 S47J::FObj Save() const;
 bool Use(const FString& Id);
 FString Label(const FString& Id) const;
 bool Hang(int32 Slot,const FString& PhotoId);
 void TakeDown(int32 Slot);
 bool HoldsPhoto(const FString& PhotoId) const;
 FString SlotPhoto(int32 Slot) const{return Frames.IsValidIndex(Slot)?Frames[Slot].PhotoId:FString();}
 FVector SlotPosition(int32 Slot) const;
 static constexpr int32 SlotCount=4;
private:
 struct FFrame
 {
  FString Id,PhotoId;
  FS47Obj Object;
  TWeakObjectPtr<UStaticMeshComponent> Border,Picture;
  TWeakObjectPtr<UMaterialInstanceDynamic> Material;
 };
 US47Main* M=nullptr;
 TWeakObjectPtr<AS47Web> Scene;
 TSharedPtr<uint8> Life;
 TArray<FFrame> Frames;
 TArray<FString> Pending;
 bool bBuilt=false,bActive=false,bApply=false;
 bool Build();
 void Bind();
 void Show(bool bShow);
 void Apply(FFrame& Frame);
 void OpenPanel(int32 Slot);
 const struct FS47AlbumShot* Shot(const FString& PhotoId) const;
};

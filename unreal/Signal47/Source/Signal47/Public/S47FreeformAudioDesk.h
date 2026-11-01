#pragma once
#include "CoreMinimal.h"
#include "S47Obj.h"
class US47Main;
class AS47Web;
class UTextRenderComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// Physical transport/mark controls on the existing reel deck. Session owns all audio evidence.
class FS47FreeformAudioDesk
{
public:
 explicit FS47FreeformAudioDesk(US47Main* Main):M(Main){}
 ~FS47FreeformAudioDesk();
 void Begin();
 void Stop();
 void Step(double Dt);
 bool Use(const FString& Id);
 FString Label(const FString& Id) const;
 bool Ready() const;
 bool ListeningPosition() const;
 FVector InteractionPoint(const FString& Id) const;
private:
 US47Main* M=nullptr;
 TWeakObjectPtr<AS47Web> Scene;
 TSharedPtr<uint8> Life;
 FS47Obj Deck;
 bool bActive=false,bBuilt=false;
 FVector Anchor=FVector::ZeroVector;
 FString LastReadout;
 TWeakObjectPtr<UTextRenderComponent> Readout;
 struct FControl
 {
  FString Id;
  FS47Obj Object;
  TWeakObjectPtr<UStaticMeshComponent> Mesh;
  TWeakObjectPtr<UMaterialInstanceDynamic> Material;
 };
 TArray<FControl> Controls;
 TArray<TWeakObjectPtr<UTextRenderComponent>> Texts;
 bool Build();
 void BindControls();
 bool Live() const;
 bool Powered() const;
 void Show(bool bShow);
};

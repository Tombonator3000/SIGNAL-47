#pragma once
#include "CoreMinimal.h"
#include "S47Obj.h"
class US47Main;
class AS47Web;
class UTextRenderComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class FS47ReceiverSound;

// Shared physical controls on the existing reel deck. The current mode's
// Freeform session or authored Story observer owns all audio/evidence state.
class FS47FreeformAudioDesk
{
public:
 explicit FS47FreeformAudioDesk(US47Main* Main):M(Main){}
 static TSharedPtr<FS47FreeformAudioDesk> Shared(US47Main* Main);
 ~FS47FreeformAudioDesk();
 void Begin();
 void Stop();
 void Step(double Dt);
 bool Use(const FString& Id);
 FString Label(const FString& Id) const;
 bool Ready() const;
 bool ListeningPosition() const;
	bool DueLit() const { return bDueLit; } // REEL-DIRECT-20261114: the sign's due lamp is lit (the take waits for the deck)
 FString ReferenceLabel() const;
 bool ReferencePlaying() const;
 int32 ReferenceIndex() const; // D48-20261110: the reference loaded on the deck, -1 before NEXT or PLAY has loaded one
 FVector InteractionPoint(const FString& Id) const;
 // Review 14 Nov (B3): the name of a tape on the deck. The station's own tape is the REFERENCE in Freeform's plain anomaly flow
 // (the owner's word), the CONTROL elsewhere (Story's internal control): the second key, the readout and the toasts all say this.
 static const TCHAR* SourceName(int32 Source,bool bPlain);
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 bool PlainNow() const; // the current job is Freeform's plain anomaly flow
 TWeakObjectPtr<AS47Web> Scene;
 TSharedPtr<uint8> Life;
 FS47Obj Deck;
 bool bActive=false,bBuilt=false;
 FVector Anchor=FVector::ZeroVector;
 FString LastReadout;
 TSharedPtr<FS47ReceiverSound> Reference;
 bool bReferenceView=false;
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
 // Freeform marks the station with a wall sign and opens one screen panel instead of the small keys.
 TArray<TWeakObjectPtr<UTextRenderComponent>> SignTexts;
 TWeakObjectPtr<UStaticMeshComponent> SignBoard;
 // REEL-DIRECT-20261114: a lamp on the sign that glows amber while the take waits for the deck (FS47FreeformSession::TapeWaits)
 TWeakObjectPtr<UStaticMeshComponent> DueLamp;TWeakObjectPtr<UMaterialInstanceDynamic> DueMat;double DuePhase=0;bool bDueLit=false;
 bool Build();
 void BindControls();
 bool Live() const;
 bool Powered() const;
 void Show(bool bShow);
 // Freeform gathers the receiver stack on the wall table beside the deck; Story keeps it at the console.
 void PlaceStack(bool bTable);
 TOptional<FTransform> StackHome;
};

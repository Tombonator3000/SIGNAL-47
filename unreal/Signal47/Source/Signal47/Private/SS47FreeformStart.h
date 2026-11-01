#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

// A title-menu decision, kept open until an explicit choice or Back. It owns
// keyboard/controller selection as well as the pointer controls.
class SS47FreeformStart : public SCompoundWidget
{
public:
 SLATE_BEGIN_ARGS(SS47FreeformStart) {}
  SLATE_ARGUMENT(TFunction<void(bool)>, OnChoose)
  SLATE_ARGUMENT(TFunction<void()>, OnBack)
 SLATE_END_ARGS()
 void Construct(const FArguments& Args);
 virtual bool SupportsKeyboardFocus() const override { return true; }
 virtual void Tick(const FGeometry& G,double Time,float Dt) override;
 virtual FReply OnKeyDown(const FGeometry& G,const FKeyEvent& Event) override;
 virtual FNavigationReply OnNavigation(const FGeometry& G,const FNavigationEvent& Event) override;

private:
 TFunction<void(bool)> Choose;
 TFunction<void()> Back;
 int32 Selected=0;
 bool bLeaving=false;
 void Move(int32 Delta);
 void Activate(int32 Index);
 TSharedRef<SWidget> Option(const FString& Label,int32 Index);
};

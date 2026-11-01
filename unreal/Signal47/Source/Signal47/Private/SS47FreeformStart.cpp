#include "SS47FreeformStart.h"
#include "S47Kit.h"
#include "S47Theme.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"

void SS47FreeformStart::Construct(const FArguments& Args)
{
 Choose=Args._OnChoose;
 Back=Args._OnBack;
 ChildSlot
 [
  SNew(SBorder)
  .BorderImage(S47Theme::Box(S47Kit::Css(0x020306,.72f)))
  .Padding(24)
  .OnMouseButtonDown_Lambda([](const FGeometry&,const FPointerEvent&){return FReply::Handled();})
  [
   SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
   [
    SNew(SBox).WidthOverride(1062.f)
    [
     SNew(SBorder)
     .BorderImage(S47Theme::Box(S47Kit::Css(0x0c0a09,.96f),S47Kit::Css(0xff8c6e,.28f),1.f))
     .Padding(FMargin(28,24))
     [
      S47Kit::Col({
       SNew(STextBlock).Text(FText::FromString(TEXT("NEW FREEFORM / CONSEQUENCES")))
        .Font(S47Theme::Term(30,3)).ColorAndOpacity(S47Kit::Css(0xffb7a4)),
       S47Kit::Text(TEXT("A continuing SARO assignment. Work, investigate and improve the base across days and nights.")),
       Option(TEXT("Local recovery / recommended"),0),
       S47Kit::Small(TEXT("A capture costs time or interrupts field work. Filed evidence and repaired equipment survive.")),
       Option(TEXT("Death / reload last save"),1),
       S47Kit::Small(TEXT("Captures end the attempt. Return to the latest saved state. Story cases keep their own rules.")),
       Option(TEXT("Back to title"),2),
       S47Kit::Small(TEXT("UP / DOWN: CHOOSE   ENTER / A: CONFIRM   ESC / B: BACK"))
      },14.f)
     ]
    ]
   ]
  ]
 ];
}

TSharedRef<SWidget> SS47FreeformStart::Option(const FString& Label,int32 Index)
{
 return SNew(SS47MenuItem).Label(Label).Px(26).Spacing(1).Pad(FMargin(14,10)).bArrow(true)
  .Selected_Lambda([this,Index]{return Selected==Index;})
  .OnHover([this,Index]{if(!bLeaving)Selected=Index;})
  .OnClick([this,Index]{Activate(Index);});
}

void SS47FreeformStart::Tick(const FGeometry& G,double Time,float Dt)
{
 SCompoundWidget::Tick(G,Time,Dt);
 if(!bLeaving) S47Theme::KeepFocus(SharedThis(this));
}

void SS47FreeformStart::Move(int32 Delta)
{
 if(!bLeaving) Selected=(Selected+Delta+3)%3;
}

void SS47FreeformStart::Activate(int32 Index)
{
 if(bLeaving) return;
 bLeaving=true;
 // Either callback can remove the entire menu. No member is read afterward.
 if(Index==2){const TFunction<void()> Fn=Back;if(Fn)Fn();return;}
 const TFunction<void(bool)> Fn=Choose;
 if(Fn) Fn(Index==1);
}

FReply SS47FreeformStart::OnKeyDown(const FGeometry&,const FKeyEvent& Event)
{
 const FKey K=Event.GetKey();
 if(K==EKeys::Up || K==EKeys::W || K==EKeys::Gamepad_DPad_Up) Move(-1);
 else if(K==EKeys::Down || K==EKeys::S || K==EKeys::Gamepad_DPad_Down) Move(1);
 else if(K==EKeys::Enter || K==EKeys::SpaceBar || K==EKeys::Gamepad_FaceButton_Bottom) Activate(Selected);
 else if(K==EKeys::Escape || K==EKeys::BackSpace || K==EKeys::Gamepad_FaceButton_Right) Activate(2);
 else return FReply::Unhandled();
 return FReply::Handled();
}

FNavigationReply SS47FreeformStart::OnNavigation(const FGeometry&,const FNavigationEvent& Event)
{
 if(Event.GetNavigationType()==EUINavigation::Up) Move(-1);
 else if(Event.GetNavigationType()==EUINavigation::Down) Move(1);
 return FNavigationReply::Stop();
}

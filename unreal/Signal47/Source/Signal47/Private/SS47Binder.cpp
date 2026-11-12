#include "SS47Binder.h"
#include "S47GameStyle.h"
#include "S47Gamepad.h"
#include "S47Keys.h"
#include "S47Kit.h"
#include "S47Theme.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

// RECORDS-BINDER (-> M3): see SS47Binder.h. Sizes are the game style's mockup pixels at 1600 x 900 (S47Game::P).
namespace S47BinderUi
{
 using S47Game::P;
 using S47Game::C;
 // The binder on the table: 1420 x 720 under its tabs, the two pages either side of a 46 px spine.
 constexpr float Wide=1420.f,High=720.f,Cover=16.f,Spine=46.f,LeftShare=.43f,RightShare=.57f;
 constexpr float ListText=486.f,SheetText=656.f; // the widest a line of type can be on the left and the right page
 const float RingAt[3]={.16f,.5f,.84f};          // the three rings down the spine, and the holes they go through

 FLinearColor Vinyl(){return C(0x1b241f);}
 FLinearColor Ink(){return C(0x25211a);}
 FLinearColor InkDim(){return C(0x25211a,.62f);}
 FLinearColor RuleRed(){return C(0x8a3b2c);}
 FLinearColor FiledGreen(){return C(0x2f5d3a);}
 FLinearColor ToneInk(int32 Tone){return Tone>0?FiledGreen():Tone<0?S47Game::StampRed():InkDim();}
 // Each divider's card stock, faded: white for the index, then yellow, salmon, blue, green and buff.
 FLinearColor Stock(int32 Section)
 {
  static const uint32 Tints[S47Binder::SecCount]={0xe9e3d2,0xe3d28a,0xdba892,0xa9bbc8,0xa8c09f,0xd6be8c};
  return C(Tints[FMath::Clamp(Section,0,int32(S47Binder::SecCount)-1)]);
 }
 const FButtonStyle* Bare(){return &FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder");}
 // A button that is only its content; the binder keeps the keys.
 TSharedRef<SWidget> Press(const TSharedRef<SWidget>& Content,TFunction<void()> Fn)
 {
  return SNew(SButton).ButtonStyle(Bare()).ContentPadding(0).IsFocusable(false).HAlign(HAlign_Fill).VAlign(VAlign_Fill)
   .OnClicked_Lambda([Fn]{if(Fn)Fn();return FReply::Handled();})[Content];
 }
 TSharedRef<SWidget> Typed(const FString& Words,float Px,const FLinearColor& Colour,float WrapAt=0.f,float Spacing=0.f)
 {
  const FSlateFontInfo Font=S47Game::Type(Px,Spacing);
  return SNew(STextBlock).Text(FText::FromString(Words)).Font(Font).ColorAndOpacity(Colour).WrapTextAt(WrapAt>0?P(WrapAt):0.f)
   .LineHeightPercentage(S47Game::LinePercent(Font,Px*1.5f));
 }
 TSharedRef<SWidget> Rule(const FLinearColor& Colour,float Thick=1.f)
 {
  return SNew(SBox).HeightOverride(FMath::Max(1.f,P(Thick)))[SNew(SBorder).BorderImage(S47Game::Box(Colour))];
 }
 // The three rings across the spine and the punched holes they go through, drawn over the open binder.
 class SRingsAndHoles : public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SRingsAndHoles){}
  SLATE_END_ARGS()
  void Construct(const FArguments&){}
  virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(8,8);}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override
  {
   const FVector2f Size=FVector2f(G.GetLocalSize());
   const FSlateBrush* Flat=FCoreStyle::Get().GetBrush("WhiteBrush");
   const float X0=(Size.X-P(Spine))*LeftShare,X1=X0+P(Spine),Reach=P(22),Hole=P(13);
   // the spine between the pages
   FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(FVector2f(P(Spine),Size.Y),FSlateLayoutTransform(FVector2f(X0,0))),Flat,ESlateDrawEffect::None,C(0x121814));
   FSlateDrawElement::MakeBox(Out,Layer+1,G.ToPaintGeometry(FVector2f(P(8),Size.Y),FSlateLayoutTransform(FVector2f(X0+P(Spine-8)*.5f,0))),Flat,ESlateDrawEffect::None,C(0x5f656b,.55f));
   for(const float At:RingAt)
   {
    const float Y=Size.Y*At;
    for(const float Cx:{X0-Reach,X1+Reach})
     FSlateDrawElement::MakeBox(Out,Layer+1,G.ToPaintGeometry(FVector2f(Hole,Hole),FSlateLayoutTransform(FVector2f(Cx-Hole*.5f,Y-Hole*.5f))),S47Game::Box(C(0x121814),C(0x6d6753,.5f),1.f,6.5f),ESlateDrawEffect::None,FLinearColor::White);
    // the ring: steel, a highlight along its top, a shade under it
    FSlateDrawElement::MakeBox(Out,Layer+2,G.ToPaintGeometry(FVector2f(X1-X0+Reach*2,P(9)),FSlateLayoutTransform(FVector2f(X0-Reach,Y-P(4.5f)))),S47Game::Box(C(0x9aa0a6),FLinearColor::Transparent,0.f,4.f),ESlateDrawEffect::None,FLinearColor::White);
    FSlateDrawElement::MakeBox(Out,Layer+3,G.ToPaintGeometry(FVector2f(X1-X0+Reach*2-P(8),P(2)),FSlateLayoutTransform(FVector2f(X0-Reach+P(4),Y-P(3)))),Flat,ESlateDrawEffect::None,C(0xdfe3e6,.7f));
    FSlateDrawElement::MakeBox(Out,Layer+3,G.ToPaintGeometry(FVector2f(X1-X0+Reach*2-P(8),P(2)),FSlateLayoutTransform(FVector2f(X0-Reach+P(4),Y+P(4.5f)))),Flat,ESlateDrawEffect::None,C(0x000000,.28f));
   }
   return Layer+4;
  }
 };
 // One page of the open binder: the sheet, and what is typed on it inside its margins.
 TSharedRef<SWidget> PageOf(const TSharedRef<SWidget>& Content,const FMargin& Margins)
 {
  return SNew(SOverlay)
   +SOverlay::Slot()[SNew(SS47GameSheet).Color(S47Game::Page()).bTorn(false)]
   +SOverlay::Slot().Padding(Margins)[Content];
 }
 FString Keys(bool bPad,const TCHAR* Board,const TCHAR* Pad){return bPad?FString(Pad):FString(Board);}
}

void SS47Binder::Construct(const FArguments& Args)
{
 Where=Args._View.IsValid()?Args._View:MakeShared<S47Binder::FView>();
 Source=Args._Source;OnFile=Args._OnFile;OnShown=Args._OnShown;OnOlder=Args._OnOlder;OnLeave=Args._OnLeave;
 PadSeen=S47Gamepad::Changes();
 Refresh();
}
void SS47Binder::Tick(const FGeometry& G,const double Time,const float Dt)
{
 SCompoundWidget::Tick(G,Time,Dt);
 S47Theme::KeepFocus(SharedThis(this));
 // The hints name the keys of whatever is in the hand now.
 if(const uint32 Pad=S47Gamepad::Changes();Pad!=PadSeen){PadSeen=Pad;Rebuild();}
}
int32 SS47Binder::Section() const{return Where->Tab;}
const S47Binder::FEntry* SS47Binder::Shown() const
{
 return List.IsValidIndex(Chosen)&&Model.Entries.IsValidIndex(List[Chosen].Entry)?&Model.Entries[List[Chosen].Entry]:nullptr;
}
void SS47Binder::Forget(){Where->Note.Reset();Where->NoteKey.Reset();}
void SS47Binder::Refresh()
{
 if(Source)Model=Source();
 S47Binder::FView& V=*Where;
 if(V.Tab<0||V.Tab>=S47Binder::SecCount){V.Tab=Model.Land();V.Pick.Reset();V.Page=0;}
 const auto Find=[this,&V]{return List.IndexOfByPredicate([&V](const S47Binder::FRow& R){return !R.bHead&&R.Id==V.Pick;});};
 List=Model.Rows(V.Tab,V.Pick);Chosen=Find();
 if(Chosen==INDEX_NONE)
 {
  // The line is gone (or none was chosen): the first of the list, and a case chosen this way shows its pages.
  const int32 First=List.IndexOfByPredicate([](const S47Binder::FRow& R){return !R.bHead;});
  V.Pick=First==INDEX_NONE?FString():List[First].Id;V.Page=0;
  List=Model.Rows(V.Tab,V.Pick);Chosen=Find();
 }
 const S47Binder::FEntry* Page=Shown();
 V.Page=Page?FMath::Clamp(V.Page,0,Page->Pages.Num()-1):0;
 if(Page&&OnShown)OnShown(Page->Key);
 Rebuild();
}
void SS47Binder::Turn(int32 Dir)
{
 Forget();Where->Tab=(Section()+Dir+S47Binder::SecCount)%S47Binder::SecCount;Where->Pick.Reset();Where->Page=0;Refresh();
}
void SS47Binder::Move(int32 Dir)
{
 int32 Next=Chosen;
 for(int32 At=Chosen+Dir;List.IsValidIndex(At);At+=Dir)if(!List[At].bHead){Next=At;break;}
 if(Next==Chosen||!List.IsValidIndex(Next))return;
 Forget();Where->Pick=List[Next].Id;Where->Page=0;Refresh();
}
void SS47Binder::Leaf(int32 Dir,bool bRound)
{
 const S47Binder::FEntry* Page=Shown();const int32 Count=Page?Page->Pages.Num():0;if(Count<2)return;
 const int32 To=bRound?(Where->Page+Dir+Count)%Count:FMath::Clamp(Where->Page+Dir,0,Count-1);
 if(To==Where->Page)return;
 Where->Page=To;Rebuild();
}
void SS47Binder::Open(const FString& EntryKey)
{
 const int32 At=Model.Find(EntryKey);if(At<0)return;
 Forget();Where->Tab=Model.Entries[At].Home;Where->Pick=EntryKey;Where->Page=0;Refresh();
}
void SS47Binder::File()
{
 if(!OnFile)return;
 const FString Key=Model.Entries.IsValidIndex(Model.ToFile)?Model.Entries[Model.ToFile].Key:FString();
 FString Words;const bool bFiled=OnFile(Words);
 Where->Note=Words;Where->NoteKey=Key;
 // The new page, where it now is: under FILED TODAY, stamped.
 if(bFiled){Where->Tab=S47Binder::SecToday;Where->Pick=FString(TEXT("today:"))+Key;Where->Page=0;}
 Refresh();
}
void SS47Binder::Act()
{
 if(!List.IsValidIndex(Chosen))return;
 const S47Binder::FRow Line=List[Chosen]; // a copy: what follows makes the list again
 if(Line.bFile){File();return;}
 if(Line.bOlder){if(OnOlder)OnOlder();return;}
 if(!Line.Jump.IsEmpty()){Open(Line.Jump);return;}
 if(Line.Goto>=0){Forget();Where->Tab=Line.Goto;Where->Pick.Reset();Where->Page=0;Refresh();return;}
 Leaf(1,true);
}
FString SS47Binder::ActWords() const
{
 if(!List.IsValidIndex(Chosen))return FString();
 if(!List[Chosen].Act.IsEmpty())return List[Chosen].Act;
 const S47Binder::FEntry* Page=Shown();
 return Page&&Page->Pages.Num()>1&&List[Chosen].Jump.IsEmpty()&&List[Chosen].Goto<0?FString(TEXT("NEXT SHEET")):FString();
}
FReply SS47Binder::OnKeyDown(const FGeometry&,const FKeyEvent& E)
{
 const TSharedRef<SWidget> Keep=AsShared(); // a key may close the binder under its own feet
 const FKey K=E.GetKey();const bool bBoard=!K.IsGamepadKey(),bFresh=!E.IsRepeat();
 if(K==EKeys::Escape||K==EKeys::Tab||K==EKeys::Gamepad_FaceButton_Right||K==EKeys::Gamepad_Special_Left||K==EKeys::Gamepad_Special_Right
  ||(bBoard&&bFresh&&(S47Keys::Is(TEXT("notebook"),K)||S47Keys::Is(TEXT("pause"),K))))
 {
  if(OnLeave)OnLeave();
  return FReply::Handled();
 }
 // The key that opened the binder may still be down: only a fresh press turns a divider or acts.
 if(K==EKeys::Q||K==EKeys::Gamepad_LeftShoulder){if(bFresh)Turn(-1);return FReply::Handled();}
 if(K==EKeys::E||K==EKeys::Gamepad_RightShoulder){if(bFresh)Turn(1);return FReply::Handled();}
 if(K==EKeys::Up||K==EKeys::W||K==EKeys::Gamepad_DPad_Up){Move(-1);return FReply::Handled();}
 if(K==EKeys::Down||K==EKeys::S||K==EKeys::Gamepad_DPad_Down){Move(1);return FReply::Handled();}
 if(K==EKeys::Left||K==EKeys::A||K==EKeys::Gamepad_DPad_Left){Leaf(-1);return FReply::Handled();}
 if(K==EKeys::Right||K==EKeys::D||K==EKeys::Gamepad_DPad_Right){Leaf(1);return FReply::Handled();}
 if(K==EKeys::PageUp||K==EKeys::PageDown)
 {
  if(SheetScroll.IsValid())SheetScroll->SetScrollOffset(FMath::Clamp(SheetScroll->GetScrollOffset()+S47Game::P(K==EKeys::PageUp?-300.f:300.f),0.f,SheetScroll->GetScrollOffsetOfEnd()));
  return FReply::Handled();
 }
 if(K==EKeys::Enter||K==EKeys::SpaceBar||K==EKeys::Gamepad_FaceButton_Bottom){if(bFresh)Act();return FReply::Handled();}
 return FReply::Unhandled();
}
TSharedRef<SWidget> SS47Binder::TabStrip()
{
 using namespace S47BinderUi;
 TSharedRef<SHorizontalBox> Strip=SNew(SHorizontalBox);
 for(int32 S=0;S<S47Binder::SecCount;++S)
 {
  const S47Binder::FDivider& D=Model.Dividers[S];const bool bOpen=S==Section();
  FLinearColor Card=Stock(S);if(!bOpen)Card=Card*FLinearColor(.74f,.74f,.74f,1.f);
  TSharedRef<SHorizontalBox> Label=SNew(SHorizontalBox)
   // the typed slip in the tab's window
   +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
   [
    SNew(SBorder).BorderImage(S47Game::Box(C(0xf4efe1),C(0x000000,.18f),1.f)).Padding(FMargin(P(7),P(1),P(7),0))
    [Typed(D.Tab,15,Ink(),0,1)]
   ];
  if(S!=S47Binder::SecIndex)
   Label->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(P(7),0,0,0)[SNew(STextBlock).Text(FText::AsNumber(D.Entries.Num())).Font(S47Game::Term(19)).ColorAndOpacity(Ink())];
  if(D.New>0)
   Label->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(P(6),0,0,0)[SNew(STextBlock).Text(FText::FromString(S==S47Binder::SecToday?TEXT("TO FILE"):TEXT("NEW"))).Font(S47Game::Term(16,1)).ColorAndOpacity(S47Game::StampRed())];
  Strip->AddSlot().AutoWidth().VAlign(VAlign_Bottom).Padding(0,0,P(5),0)
  [
   Press(SNew(SBorder).BorderImage(S47Game::Box4(Card,FVector4(5,5,0,0))).Padding(FMargin(P(11),P(bOpen?9:6),P(11),P(bOpen?9:5)))[Label],
    [this,S]{if(S==Section())return;Forget();Where->Tab=S;Where->Pick.Reset();Where->Page=0;Refresh();})
  ];
 }
 return Strip;
}
TSharedRef<SWidget> SS47Binder::ListPage()
{
 using namespace S47BinderUi;
 const S47Binder::FDivider& D=Model.Dividers[Section()];
 LineWidgets.Reset();LineWidgets.SetNum(List.Num());
 TSharedRef<SVerticalBox> Col=SNew(SVerticalBox);
 // the divider's own heading, typed, with the date on the right
 Col->AddSlot().AutoHeight()
 [
  SNew(SHorizontalBox)
  +SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Bottom)[Typed(D.Title,22,Ink(),0,2)]
  +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)[Typed(Model.Date,14,InkDim(),0,1)]
 ];
 Col->AddSlot().AutoHeight().Padding(0,P(5),0,P(10))[Rule(RuleRed(),2)];
 if(List.IsEmpty())
 {
  Col->AddSlot().AutoHeight().Padding(0,P(8),0,0)[Typed(D.Empty,17,C(0x25211a,.82f),ListText)];
  return Col;
 }
 ListScroll=SNew(SScrollBox).Style(S47Game::QuietScroll()).ScrollBarStyle(S47Game::QuietBar(C(0x25211a,.3f))).ScrollBarThickness(FVector2D(P(5),P(5)));
 for(int32 At=0;At<List.Num();++At)
 {
  const S47Binder::FRow& R=List[At];
  if(R.bHead)
  {
   ListScroll->AddSlot().Padding(0,P(At==0?0:10),P(10),P(4))
   [
    SNew(SVerticalBox)
    +SVerticalBox::Slot().AutoHeight()[Typed(R.Left,13,RuleRed(),ListText-10,2)]
    +SVerticalBox::Slot().AutoHeight().Padding(0,P(2),0,0)[Rule(C(0x8a3b2c,.4f))]
   ];
   continue;
  }
  const bool bPicked=At==Chosen;const float Inset=R.Indent*22.f;
  const TSharedRef<SWidget> Card=SNew(SBorder)
   .BorderImage(bPicked?S47Game::Box(C(0x233a7a,.10f),C(0x233a7a,.6f),1.f,2.f):S47Game::Box(FLinearColor::Transparent))
   .Padding(FMargin(P(8+Inset),P(5),P(8),P(5)))
   [
    SNew(SVerticalBox)
    +SVerticalBox::Slot().AutoHeight()
    [
     SNew(SHorizontalBox)
     +SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[Typed(R.Indent>0?FString(TEXT("- "))+R.Left:R.Left,14,InkDim(),0,1)]
     +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(P(10),0,0,0)[Typed(R.Right,14,ToneInk(R.bNew&&R.Tone==0?-1:R.Tone),0,1)]
    ]
    +SVerticalBox::Slot().AutoHeight().Padding(0,P(1),0,0)[Typed(R.Mid,16,Ink(),ListText-26-Inset)]
   ];
  const TSharedRef<SWidget> LineButton=Press(Card,[this,Id=R.Id,bPicked,bFile=R.bFile]
  {
   // A click chooses; a click on the line already chosen does what ENTER does. Filing is never a stray click: it has its own key.
   if(bPicked){if(!bFile)Act();return;}
   Forget();Where->Pick=Id;Where->Page=0;Refresh();
  });
  LineWidgets[At]=LineButton;
  ListScroll->AddSlot().Padding(0,0,P(10),P(2))[LineButton];
 }
 Col->AddSlot().FillHeight(1.f)[ListScroll.ToSharedRef()];
 return Col;
}
TSharedRef<SWidget> SS47Binder::SheetPage()
{
 using namespace S47BinderUi;
 SheetScroll.Reset();
 const S47Binder::FEntry* Page=Shown();
 TSharedRef<SVerticalBox> Col=SNew(SVerticalBox);
 if(!Page)
 {
  // The front sheet: whose binder it is and how it is kept.
  Col->AddSlot().FillHeight(1.f)
  [
   SAssignNew(SheetScroll,SScrollBox).Style(S47Game::QuietScroll()).ScrollBarStyle(S47Game::QuietBar(C(0x25211a,.3f))).ScrollBarThickness(FVector2D(P(5),P(5)))
   +SScrollBox::Slot()[Typed(Model.Front,17,Ink(),SheetText)]
  ];
  return Col;
 }
 const S47Binder::FEntry& E=*Page;const int32 Sheets=E.Pages.Num(),Sheet=FMath::Clamp(Where->Page,0,Sheets-1);
 // what kind of page it is, and its reference
 Col->AddSlot().AutoHeight()
 [
  SNew(SHorizontalBox)
  +SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Bottom)[Typed(E.Kind,14,RuleRed(),0,2)]
  +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)[Typed(E.Ref,15,InkDim(),0,1)]
 ];
 Col->AddSlot().AutoHeight().Padding(0,P(4),0,0)[Typed(E.What,22,Ink(),SheetText)];
 const auto Field=[&Col](const TCHAR* Name,const FString& Value,const FLinearColor& Colour)
 {
  if(Value.IsEmpty())return;
  Col->AddSlot().AutoHeight().Padding(0,P(3),0,0)
  [
   SNew(SHorizontalBox)
   +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(P(92))[Typed(Name,13,InkDim(),0,2)]]
   +SHorizontalBox::Slot().FillWidth(1.f)[Typed(Value,16,Colour,SheetText-96)]
  ];
 };
 Col->AddSlot().AutoHeight().Padding(0,P(6),0,0)[SNullWidget::NullWidget];
 Field(TEXT("DATE"),E.When,Ink());
 if(E.Showed!=E.What)Field(TEXT("SHOWED"),E.Showed,Ink());
 Field(TEXT("STATUS"),E.StatusLine,ToneInk(E.Tone));
 // the slip the last filing left on this page
 if(!Where->Note.IsEmpty()&&Where->NoteKey==E.Key)
  Col->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(0,P(10),0,0)
  [
   S47Game::Turned(SNew(SBorder).BorderImage(S47Game::Box(C(0xf1e3a0),C(0x000000,.16f),1.f)).Padding(FMargin(P(12),P(6)))[Typed(Where->Note,16,Ink(),SheetText-40)],-.6f)
  ];
 // the packet in hand: where it goes, and the one thing to do with it
 if(E.bToFile)
 {
  const bool bThis=Model.Entries.IsValidIndex(Model.ToFile)&&Page==&Model.Entries[Model.ToFile];
  if(bThis&&Model.bAtBinder)
  {
   Col->AddSlot().AutoHeight().Padding(0,P(12),0,0)[Typed(FString(TEXT("GOES BEHIND: "))+Model.Dividers[E.Home].Title+TEXT(" / ")+Model.Date,15,InkDim(),SheetText,1)];
   Col->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(0,P(6),0,0)[S47Kit::Action(TEXT("FILE PACKET IN THIS BINDER"),[this]{File();})];
  }
  else Col->AddSlot().AutoHeight().Padding(0,P(12),0,0)
   [Typed(bThis?TEXT("READING COPY. Bring this packet to the binder in the records room to file it."):TEXT("Another packet is filed first. This one follows it."),15,S47Game::StampRed(),SheetText)];
 }
 Col->AddSlot().AutoHeight().Padding(0,P(10),0,P(8))[Rule(C(0x25211a,.3f))];
 // the sheet itself
 Col->AddSlot().FillHeight(1.f)
 [
  SAssignNew(SheetScroll,SScrollBox).Style(S47Game::QuietScroll()).ScrollBarStyle(S47Game::QuietBar(C(0x25211a,.3f))).ScrollBarThickness(FVector2D(P(5),P(5)))
  +SScrollBox::Slot()[Typed(E.Pages[Sheet],17,Ink(),SheetText)]
 ];
 // the foot: the page it belongs with, and the sheets of a long page
 TSharedRef<SHorizontalBox> Foot=SNew(SHorizontalBox);
 if(!E.Link.IsEmpty())
  Foot->AddSlot().AutoWidth().VAlign(VAlign_Center)[Press(Typed(FString(TEXT("> "))+E.LinkLabel,15,S47Game::InkBlue(),0,1),[this,To=E.Link]{Open(To);})];
 Foot->AddSlot().FillWidth(1.f)[SNullWidget::NullWidget];
 if(Sheets>1)
 {
  const auto Arrow=[this](const TCHAR* Glyph,int32 Dir,bool bCan)
  {
   return Press(SNew(SBox).WidthOverride(P(34)).HAlign(HAlign_Center)[SNew(STextBlock).Text(FText::FromString(Glyph)).Font(S47Game::Term(24)).ColorAndOpacity(bCan?S47BinderUi::Ink():S47BinderUi::InkDim()*FLinearColor(1,1,1,.4f))],[this,Dir]{Leaf(Dir);});
  };
  Foot->AddSlot().AutoWidth().VAlign(VAlign_Center)[Arrow(TEXT("<"),-1,Sheet>0)];
  Foot->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(P(4),0)[Typed(FString::Printf(TEXT("SHEET %d OF %d"),Sheet+1,Sheets),14,InkDim(),0,1)];
  Foot->AddSlot().AutoWidth().VAlign(VAlign_Center)[Arrow(TEXT(">"),1,Sheet+1<Sheets)];
 }
 Col->AddSlot().AutoHeight().Padding(0,P(6),0,0)[Foot];
 if(E.Stamp.IsEmpty())return Col;
 // the rubber stamp of a page that is in the binder, a little crooked, over the page's head
 return SNew(SOverlay)
  +SOverlay::Slot()[Col]
  +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0,P(34),P(6),0)[SNew(SBox).Visibility(EVisibility::HitTestInvisible)[S47Game::Stamp(E.Stamp,24,-7)]];
}
TSharedRef<SWidget> SS47Binder::HintStrip()
{
 using namespace S47BinderUi;
 const bool bPad=S47Gamepad::UsingGamepad();
 TSharedRef<SHorizontalBox> Strip=SNew(SHorizontalBox);
 const auto Hint=[&Strip](const FString& Key,const FString& Words){Strip->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,P(22),0)[S47Game::KeyHint(Key,Words,20)];};
 Hint(Keys(bPad,TEXT("Q  E"),TEXT("LB  RB")),TEXT("DIVIDER"));
 Hint(Keys(bPad,TEXT("W  S"),TEXT("D-PAD")),bPad?TEXT("LINE / SHEET"):TEXT("LINE"));
 if(!bPad)Hint(TEXT("A  D"),TEXT("SHEET"));
 if(const FString Does=ActWords();!Does.IsEmpty())Hint(Keys(bPad,TEXT("ENTER"),TEXT("A")),Does);
 Hint(Keys(bPad,TEXT("ESC"),TEXT("B")),TEXT("CLOSE"));
 return Strip;
}
void SS47Binder::Rebuild()
{
 using namespace S47BinderUi;
 // The list keeps its place while the same divider is open; the line chosen is then brought into view.
 const float Kept=ListScroll.IsValid()&&BuiltTab==Section()?ListScroll->GetScrollOffset():0.f;
 ListScroll.Reset();BuiltTab=Section();
 const TSharedRef<SWidget> Left=ListPage(),Right=SheetPage();
 if(ListScroll.IsValid())
 {
  ListScroll->SetScrollOffset(Kept);
  if(LineWidgets.IsValidIndex(Chosen)&&LineWidgets[Chosen].IsValid())ListScroll->ScrollDescendantIntoView(LineWidgets[Chosen],false,EDescendantScrollDestination::IntoView);
 }
 const TFunction<void()> Leave=OnLeave;
 ChildSlot
 [
  // the room dimmed behind the binder; a click beside it closes it
  SNew(SBorder).BorderImage(S47Game::Box(C(0x020306,.76f))).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(12.f))
  .OnMouseButtonDown_Lambda([Leave](const FGeometry&,const FPointerEvent&){if(Leave)Leave();return FReply::Handled();})
  [
   SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
   [
    SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBorder")).Padding(0)
    .OnMouseButtonDown_Lambda([](const FGeometry&,const FPointerEvent&){return FReply::Handled();})
    [
     SNew(SBox).WidthOverride(P(Wide))
     [
      SNew(SVerticalBox)
      +SVerticalBox::Slot().AutoHeight().Padding(P(Cover+18),0,0,0)[TabStrip()]
      +SVerticalBox::Slot().AutoHeight()
      [
       SNew(SBox).HeightOverride(P(High))
       [
        // the cover, a strip of the open divider's card along the top, the two pages and the rings
        SNew(SBorder).BorderImage(S47Game::Box(Vinyl(),C(0x0b0e0c),1.f,7.f)).Padding(P(Cover))
        [
         SNew(SOverlay)
         +SOverlay::Slot()
         [
          SNew(SHorizontalBox)
          +SHorizontalBox::Slot().FillWidth(LeftShare)[PageOf(Left,FMargin(P(26),P(24),P(46),P(18)))]
          +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(P(Spine))]
          +SHorizontalBox::Slot().FillWidth(RightShare)[PageOf(Right,FMargin(P(54),P(24),P(30),P(16)))]
         ]
         +SOverlay::Slot().VAlign(VAlign_Top)[SNew(SBox).HeightOverride(P(7)).Visibility(EVisibility::HitTestInvisible)[SNew(SBorder).BorderImage(S47Game::Box(Stock(Section())))]]
         +SOverlay::Slot()[SNew(SRingsAndHoles).Visibility(EVisibility::HitTestInvisible)]
        ]
       ]
      ]
      +SVerticalBox::Slot().AutoHeight().Padding(P(Cover),P(12),0,0)[HintStrip()]
     ]
    ]
   ]
  ]
 ];
}

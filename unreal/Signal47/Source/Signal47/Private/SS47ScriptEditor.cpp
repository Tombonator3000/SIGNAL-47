#include "SS47ScriptEditor.h"
#include "S47DirectorCam.h"
#include "S47EventPlaces.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47HudGauges.h"
#include "S47Main.h"
#include "S47ScriptPlayer.h"
#include "S47Theme.h"
#include "S47UI.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

// DIRECTOR-TOOL-20261109: the director's panel (SS47ScriptEditor.h). Kept simple on purpose: rows of fields and buttons, built again
// when the draft changes shape; typing writes straight into the draft. Drag and drop, copy and paste come later.

// What is being edited, kept while the panel is closed (one game at a time on this machine). DIRECTOR-MODE-01: declared in the header
// for the director's camera; the old names below stay.
FS47Script& S47ScriptEditor::Draft(){static FS47Script D;return D;}
bool& S47ScriptEditor::HaveDraft(){static bool b=false;return b;}
bool& S47ScriptEditor::Dirty(){static bool b=false;return b;}
namespace S47ScriptEditorLocal{int32& SelectedRaw(){static int32 I=INDEX_NONE;return I;}}
int32 S47ScriptEditor::SelectedShot(){return HaveDraft()?S47DirectorCam::ShotOrNone(Draft().Actions,S47ScriptEditorLocal::SelectedRaw()):INDEX_NONE;}
void S47ScriptEditor::SelectShot(int32 I){S47ScriptEditorLocal::SelectedRaw()=HaveDraft()?S47DirectorCam::ShotOrNone(Draft().Actions,I):INDEX_NONE;}

namespace S47ScriptEditorLocal
{
 using S47ScriptEditor::Draft;
 using S47ScriptEditor::HaveDraft;
 using S47ScriptEditor::Dirty;
 FString NumText(double V){FString S=FString::SanitizeFloat(FMath::RoundToDouble(V*100)/100);if(S.EndsWith(TEXT(".0")))S.LeftChopInline(2);return S;}
 FString PointText(const FS47ScriptPoint& P){return P.bSet?FString::Printf(TEXT("(%.0f, %.0f%s)"),P.X,P.Z,P.Up>.05?*FString::Printf(TEXT(" +%.1f m"),P.Up):TEXT("")):FString(TEXT("(no point)"));}
 FString CaseOption(int32 L){return FString::Printf(TEXT("%d %s"),L,FS47FreeformCases::Def(L).Title);}
 // SHOT-BEAT-01: HERE stores the player's standing point (HereRoad has no height), so a shot's eye set from it is put at eye height
 // over that ground; LOOK puts it at eye height over what is looked at (a roof keeps its height).
 constexpr double ShotEyeUp=1.7;
}

void S47ScriptEditor::Open(US47Main* M)
{
 if(!M||!M->UI)return;
 if(!FS47ScriptPlayer::DevAllowed(M)){M->UI->Toast(TEXT("The script editor is only there in a developer's run, or with -S47Director."),4);return;}
 if(!FS47ScriptPlayer::Of(M)){M->UI->Toast(TEXT("No script player in this session: Freeform only."),4);return;}
 if(M->UI->Modal())M->UI->Close(true);
 TSharedRef<SS47ScriptEditor> W=SNew(SS47ScriptEditor).Main(M);
 M->UI->Open(W);
 FSlateApplication::Get().SetKeyboardFocus(W);
}

FS47ScriptPlayer* SS47ScriptEditor::Player() const{return FS47ScriptPlayer::Of(M);}

void SS47ScriptEditor::Construct(const FArguments& InArgs)
{
 S47_USING_THEME
 M=InArgs._Main;
 using namespace S47ScriptEditorLocal;
 if(!HaveDraft())
 {
  FS47ScriptPlayer* P=Player();
  if(P&&P->Library().Num())Select(P->Library()[0].Id);else NewScript();
 }
 const S47HudGauges::FHudPalette& Pal=S47HudGauges::Palette();
 ChildSlot
 [
  SNew(SBorder).BorderImage(Box(Night(.93f),Css(Pal.Clock,.35f),1.f)).Padding(Pad(40,28,40,22))
  [
   SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()
   [
    SNew(SHorizontalBox)
    +SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("DIRECTOR / SCRIPTED EVENTS"))).Font(Heading(34,2)).ColorAndOpacity(Css(Pal.Clock,Pal.ClockOn))]
    // DIRECTOR-MODE-01: the free camera (S47DirectorCam.h); the panel goes, the draft stays. Not let in: the panel again, and why.
    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(Pad(24,0,0,0))[Btn(TEXT("FLY"),[this]
    {
     US47Main* Main=M;
     CloseIt();
     FString Why;
     if(!S47DirectorCam::Toggle(Main,Why))S47ScriptEditor::Open(Main);
    },true)]
   ]
   +SVerticalBox::Slot().AutoHeight().Padding(Pad(0,4,0,8))
   [
    SNew(STextBlock).AutoWrapText(true).Font(Term(18)).ColorAndOpacity(Mute())
    .Text(FText::FromString(FString::Printf(TEXT("Developer only. Points are road-local metres: walk there, open F10 again and press HERE or LOOK. Saved to %s. Escape closes; the draft stays."),*FS47ScriptPlayer::WriteDir())))
   ]
   +SVerticalBox::Slot().FillHeight(1)
   [
    SNew(SHorizontalBox)
    +SHorizontalBox::Slot().AutoWidth().Padding(Pad(0,0,18,0))[SNew(SBox).WidthOverride(Px(300))[SNew(SScrollBox)+SScrollBox::Slot()[SAssignNew(ListSlot,SBox)]]]
    +SHorizontalBox::Slot().FillWidth(1)[SNew(SScrollBox)+SScrollBox::Slot()[SAssignNew(BodySlot,SBox)]]
   ]
   +SVerticalBox::Slot().AutoHeight().Padding(Pad(0,10,0,0))[BuildBar()]
  ]
 ];
 Rebuild();
}

void SS47ScriptEditor::Rebuild()
{
 if(ListSlot.IsValid())ListSlot->SetContent(BuildList());
 if(BodySlot.IsValid())BodySlot->SetContent(BuildBody());
}

void SS47ScriptEditor::Changed(){S47ScriptEditorLocal::Dirty()=true;Rebuild();}

void SS47ScriptEditor::Say(const FString& Text,bool bBad){Status=Text;bStatusBad=bBad;}

TSharedRef<SWidget> SS47ScriptEditor::Words(const FString& Text,float Size,int32 Tone)
{
 S47_USING_THEME
 const S47HudGauges::FHudPalette& Pal=S47HudGauges::Palette();
 const FLinearColor C=Tone==1?Css(Pal.Clock,Pal.ClockOn):Tone==2?Mute():Tone==3?Css(Pal.Low,Pal.LowOn):Body();
 return SNew(STextBlock).Text(FText::FromString(Text)).Font(Term(Size)).ColorAndOpacity(C);
}

TSharedRef<SWidget> SS47ScriptEditor::Btn(const FString& Text,TFunction<void()> Fn,bool bStrong)
{
 S47_USING_THEME
 const S47HudGauges::FHudPalette& Pal=S47HudGauges::Palette();
 const FLinearColor On=Css(Pal.Clock,Pal.ClockOn),Off=Css(Pal.Clock,bStrong?.85f:.6f);
 TSharedRef<STextBlock> T=SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("[%s]"),*Text))).Font(Term(bStrong?22:19));
 TSharedRef<SButton> B=SNew(SButton).ButtonStyle(FCoreStyle::Get(),"NoBorder").ContentPadding(FMargin(4,1))
  .OnClicked_Lambda([Fn]{if(Fn)Fn();return FReply::Handled();})[T];
 T->SetColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([W=TWeakPtr<SButton>(B),On,Off]{const TSharedPtr<SButton> P=W.Pin();return FSlateColor(P.IsValid()&&P->IsHovered()?On:Off);}));
 return B;
}

TSharedRef<SWidget> SS47ScriptEditor::Field(const FString& Hint,const FString& Value,TFunction<void(const FString&)> Set,float Width)
{
 S47_USING_THEME
 return SNew(SBox).MinDesiredWidth(Px(Width))
 [
  SNew(SEditableTextBox).Text(FText::FromString(Value)).HintText(FText::FromString(Hint)).Font(Term(19))
  .ForegroundColor(FSlateColor(Body())).BackgroundColor(FSlateColor(Ink(.9f)))
  .OnTextChanged_Lambda([Set](const FText& T){if(Set)Set(T.ToString());S47ScriptEditorLocal::Dirty()=true;})
 ];
}

TSharedRef<SWidget> SS47ScriptEditor::Num(const FString& Hint,double Value,TFunction<void(double)> Set,float Width)
{
 return Field(Hint,S47ScriptEditorLocal::NumText(Value),[Set](const FString& S){const FString T=S.TrimStartAndEnd();if(Set&&T.IsNumeric())Set(FCString::Atod(*T));},Width);
}

TSharedRef<SWidget> SS47ScriptEditor::Cycle(const FString& Prefix,const TArray<FString>& Options,const FString& Value,TFunction<void(const FString&)> Set)
{
 const int32 I=Options.IndexOfByKey(Value);
 const FString Next=Options.Num()?Options[(I+1)%Options.Num()]:FString();
 return Btn(Prefix+(Value.IsEmpty()?FString(TEXT("-")):Value),[this,Set,Next]{if(Set)Set(Next);Changed();});
}

TSharedRef<SWidget> SS47ScriptEditor::PointRow(const FString& Label,TFunction<FS47ScriptPoint*()> Get)
{
 const FS47ScriptPoint* Now=Get();
 return SNew(SHorizontalBox)
  +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,6,0)[Words(Label+TEXT(" ")+S47ScriptEditorLocal::PointText(Now?*Now:FS47ScriptPoint()),19,2)]
  +SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("HERE"),[this,Get]{FS47ScriptPoint P;if(Player()&&Player()->HereRoad(P)){if(FS47ScriptPoint* T=Get())*T=P;Changed();}else Say(TEXT("no player on the land here"),true);})]
  +SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("LOOK"),[this,Get]{FS47ScriptPoint P;if(Player()&&Player()->LookRoad(P)){if(FS47ScriptPoint* T=Get())*T=P;Changed();}else Say(TEXT("nothing in view to set a point on"),true);})]
  +SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("CLEAR"),[this,Get]{if(FS47ScriptPoint* T=Get())*T=FS47ScriptPoint();Changed();})];
}

TSharedRef<SWidget> SS47ScriptEditor::BuildList()
{
 using namespace S47ScriptEditorLocal;
 TSharedRef<SVerticalBox> V=SNew(SVerticalBox);
 V->AddSlot().AutoHeight().Padding(0,0,0,6)[Words(TEXT("SCRIPTS"),22,1)];
 FS47ScriptPlayer* P=Player();
 if(P)for(const FS47Script& S:P->Library())
 {
  const FString* Why=P->Refused().Find(S.Id);
  const FString Mark=Why?TEXT("!"):S.bEnabled?TEXT("ON "):TEXT("off");
  const bool bHere=S.Id==Draft().Id;
  V->AddSlot().AutoHeight()[Btn(FString::Printf(TEXT("%s %s%s"),*Mark,*S.Name.Left(22),bHere?TEXT(" <"):TEXT("")),[this,Id=S.Id]{Select(Id);Rebuild();})];
 }
 if(P&&P->Library().IsEmpty())V->AddSlot().AutoHeight()[Words(TEXT("none yet"),19,2)];
 V->AddSlot().AutoHeight().Padding(0,10,0,0)[Btn(TEXT("NEW SCRIPT"),[this]{NewScript();Rebuild();},true)];
 if(P&&P->Running())V->AddSlot().AutoHeight().Padding(0,12,0,0)[Words(FString::Printf(TEXT("PLAYING %s, %.0f s"),*P->RunningId(),P->RunAge()),19,1)];
 return V;
}

TSharedRef<SWidget> SS47ScriptEditor::BuildBody()
{
 using namespace S47ScriptEditorLocal;
 FS47Script& D=Draft();
 TSharedRef<SVerticalBox> V=SNew(SVerticalBox);
 // Name, id, on or off, how often.
 V->AddSlot().AutoHeight().Padding(0,0,0,4)
 [
  SNew(SHorizontalBox)
  +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,6,0)[Words(TEXT("NAME"),19,2)]
  +SHorizontalBox::Slot().AutoWidth()[Field(TEXT("what Tom calls it"),D.Name,[](const FString& S){Draft().Name=S;},260)]
  +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12,0,6,0)[Words(TEXT("ID"),19,2)]
  +SHorizontalBox::Slot().AutoWidth()[Field(TEXT("file name"),D.Id,[](const FString& S){Draft().Id=S47Script::CleanId(S);},200)]
 ];
 V->AddSlot().AutoHeight().Padding(0,0,0,8)
 [
  SNew(SHorizontalBox)
  +SHorizontalBox::Slot().AutoWidth()[Btn(D.bEnabled?TEXT("ON: plays by its trigger"):TEXT("OFF: preview only"),[this]{Draft().bEnabled=!Draft().bEnabled;Changed();})]
  +SHorizontalBox::Slot().AutoWidth()[Cycle(TEXT("REPEAT "),{TEXT("once"),TEXT("nightly"),TEXT("always")},S47Script::RepeatName(D.Repeat),[](const FString& S){Draft().Repeat=S47Script::RepeatFrom(S);})]
  +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8,0,4,0)[Words(D.Repeat==ES47ScriptRepeat::Always?TEXT("again after min"):TEXT(""),19,2)]
  +SHorizontalBox::Slot().AutoWidth()[D.Repeat==ES47ScriptRepeat::Always?Num(TEXT("min"),D.CooldownMin,[](double X){Draft().CooldownMin=FMath::Max(0.,X);}):SNullWidget::NullWidget]
  +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12,0,0,0)[Words(D.File.IsEmpty()?FString(TEXT("not saved yet")):FPaths::GetCleanFilename(D.File),18,2)]
 ];
 V->AddSlot().AutoHeight()[BuildTrigger()];
 V->AddSlot().AutoHeight().Padding(0,12,0,4)[Words(TEXT("ACTIONS (seconds from the start)"),22,1)];
 for(int32 I=0;I<D.Actions.Num();++I)V->AddSlot().AutoHeight().Padding(0,0,0,6)[BuildAction(I)];
 V->AddSlot().AutoHeight().Padding(0,4,0,0)[BuildAdd()];
 // What is wrong, what holds it back, and whether it would start now.
 TArray<FString> Warn;
 const TArray<FString> Errors=S47Script::Lint(D,&Warn);
 for(const FString& E:Errors)V->AddSlot().AutoHeight()[Words(TEXT("REFUSED: ")+E,19,3)];
 for(const FString& W:Warn)V->AddSlot().AutoHeight()[Words(TEXT("NOTE: ")+W,19,2)];
 if(FS47ScriptPlayer* P=Player())
 {
  const FString Why=P->WhyNot(D);
  V->AddSlot().AutoHeight().Padding(0,6,0,0)[Words(Why.IsEmpty()?FString(TEXT("NOW: its trigger holds; switched on it would start.")):TEXT("NOW: would not start: ")+Why,19,2)];
  V->AddSlot().AutoHeight()[Words(FString::Printf(TEXT("played %d times in this game; about %.0f s long"),P->PlayedCount(D.Id),D.Length()),18,2)];
 }
 return V;
}

TSharedRef<SWidget> SS47ScriptEditor::BuildTrigger()
{
 using namespace S47ScriptEditorLocal;
 FS47ScriptTrigger& T=Draft().Trigger;
 TSharedRef<SVerticalBox> V=SNew(SVerticalBox);
 V->AddSlot().AutoHeight().Padding(0,4,0,4)[Words(TEXT("TRIGGER (0 or empty: any)"),22,1)];
 V->AddSlot().AutoHeight().Padding(0,0,0,4)
 [
  SNew(SHorizontalBox)
  +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,6,0)[Words(TEXT("DAYS"),19,2)]
  +SHorizontalBox::Slot().AutoWidth()[Num(TEXT("from"),T.DayFrom,[](double X){Draft().Trigger.DayFrom=FMath::Max(0,int32(X));},60)]
  +SHorizontalBox::Slot().AutoWidth()[Num(TEXT("to"),T.DayTo,[](double X){Draft().Trigger.DayTo=FMath::Max(0,int32(X));},60)]
  +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12,0,6,0)[Words(TEXT("NIGHTS"),19,2)]
  +SHorizontalBox::Slot().AutoWidth()[Num(TEXT("from"),T.NightFrom,[](double X){Draft().Trigger.NightFrom=FMath::Max(0,int32(X));},60)]
  +SHorizontalBox::Slot().AutoWidth()[Num(TEXT("to"),T.NightTo,[](double X){Draft().Trigger.NightTo=FMath::Max(0,int32(X));},60)]
  +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12,0,6,0)[Words(TEXT("HOURS"),19,2)]
  +SHorizontalBox::Slot().AutoWidth()[Field(TEXT("23:30"),T.HourFrom<0?FString():S47Script::HourText(T.HourFrom),[](const FString& S){Draft().Trigger.HourFrom=S47Script::ParseHour(S);},80)]
  +SHorizontalBox::Slot().AutoWidth()[Field(TEXT("04:00"),T.HourTo<0?FString():S47Script::HourText(T.HourTo),[](const FString& S){Draft().Trigger.HourTo=S47Script::ParseHour(S);},80)]
 ];
 // Where: a place from the list, or a point of its own.
 TArray<FString> Places={TEXT("(anywhere)"),TEXT("(point)")};
 const FS47FreeformSession* F=M?M->Freeform.Get():nullptr;
 if(F&&F->EventPlaces)for(const FS47EventPlace& P:F->EventPlaces->All())Places.Add(P.Id);
 const FString Now=!T.Place.IsEmpty()?T.Place:T.Point.bSet?FString(TEXT("(point)")):FString(TEXT("(anywhere)"));
 TSharedRef<SHorizontalBox> Where=SNew(SHorizontalBox);
 Where->AddSlot().AutoWidth()[Cycle(TEXT("WHERE "),Places,Now,[this](const FString& S)
 {
  FS47ScriptTrigger& Tr=Draft().Trigger;
  if(S==TEXT("(anywhere)")){Tr.Place.Reset();Tr.Point=FS47ScriptPoint();}
  else if(S==TEXT("(point)")){Tr.Place.Reset();FS47ScriptPoint Here;if(Player()&&Player()->HereRoad(Here))Tr.Point=Here;Tr.Point.bSet=true;}
  else{Tr.Place=S;Tr.Point=FS47ScriptPoint();}
 })];
 Where->AddSlot().AutoWidth()[Btn(TEXT("NEAREST PLACE"),[this]
 {
  const FS47FreeformSession* Fs=M?M->Freeform.Get():nullptr;
  const FS47EventPlace* P=Fs&&Fs->EventPlaces?Fs->EventPlaces->Nearest(400):nullptr;
  if(!P){Say(TEXT("no place from the list within 400 m"),true);return;}
  Draft().Trigger.Place=P->Id;Draft().Trigger.Point=FS47ScriptPoint();Say(TEXT("place: ")+P->Id+TEXT(", ")+P->Title);Changed();
 })];
 // DIRECTOR-MODE-01, slice 4: one press from any WHERE to a point of its own where the player stands (road-local, on the ground; the
 // volume's shape and size as they were). The POINT row's HERE below does the same once there is a point.
 Where->AddSlot().AutoWidth()[Btn(TEXT("POINT HERE"),[this]
 {
  FS47ScriptPoint Here;
  if(!Player()||!Player()->HereRoad(Here)){Say(TEXT("no player on the land here"),true);return;}
  FS47ScriptTrigger& Tr=Draft().Trigger;
  Tr.Place.Reset();Tr.Point=Here;
  Say(FString::Printf(TEXT("the trigger's point is where you stand: (%.0f, %.0f)"),Here.X,Here.Z));
  Changed();
 })];
 if(Draft().Trigger.HasWhere())
 {
  Where->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(12,0,6,0)[Words(TEXT("WITHIN m"),19,2)];
  Where->AddSlot().AutoWidth()[Num(TEXT("m"),T.Radius,[](double X){Draft().Trigger.Radius=FMath::Max(1.,X);},70)];
 }
 V->AddSlot().AutoHeight().Padding(0,0,0,4)[Where];
 if(T.Place.IsEmpty()&&T.Point.bSet)V->AddSlot().AutoHeight().Padding(0,0,0,4)[PointRow(TEXT("POINT"),[]{return &Draft().Trigger.Point;})];
 // DIRECTOR-MODE-01 (slice 3): the volume around the place or the point: its shape and height band, then what the player does with it
 // and for how long. A box, a look and a leave need a point of their own (the lint says so).
 if(T.HasWhere())
 {
  TSharedRef<SHorizontalBox> ShapeRow=SNew(SHorizontalBox),ModeRow=SNew(SHorizontalBox);
  const auto Put=[](const TSharedRef<SHorizontalBox>& Row,TSharedRef<SWidget> W){Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,4,0)[W];};
  const bool bBox=T.Shape==ES47ScriptShape::Box;
  Put(ShapeRow,Cycle(TEXT("SHAPE "),{TEXT("circle"),TEXT("box")},bBox?TEXT("box"):TEXT("circle"),[](const FString& S){Draft().Trigger.Shape=S==TEXT("box")?ES47ScriptShape::Box:ES47ScriptShape::Circle;}));
  if(bBox)
  {
   Put(ShapeRow,Words(TEXT("HALF x"),19,2));Put(ShapeRow,Num(TEXT("m"),T.HalfX,[](double X){Draft().Trigger.HalfX=X;},60));
   Put(ShapeRow,Words(TEXT("z"),19,2));Put(ShapeRow,Num(TEXT("m"),T.HalfZ,[](double X){Draft().Trigger.HalfZ=X;},60));
   Put(ShapeRow,Words(TEXT("YAW"),19,2));Put(ShapeRow,Num(TEXT("radians"),T.Yaw,[](double X){Draft().Trigger.Yaw=X;},70));
  }
  Put(ShapeRow,Words(TEXT("HEIGHT m (0: any)"),19,2));Put(ShapeRow,Num(TEXT("m"),T.Height,[](double X){Draft().Trigger.Height=X;},60));
  V->AddSlot().AutoHeight().Padding(0,0,0,4)[ShapeRow];
  const TCHAR* const ModeNow=T.Mode==ES47ScriptMode::Look?TEXT("look"):T.Mode==ES47ScriptMode::Leave?TEXT("leave"):TEXT("enter");
  Put(ModeRow,Cycle(TEXT("MODE "),{TEXT("enter"),TEXT("look"),TEXT("leave")},ModeNow,[](const FString& S){Draft().Trigger.Mode=S==TEXT("look")?ES47ScriptMode::Look:S==TEXT("leave")?ES47ScriptMode::Leave:ES47ScriptMode::Enter;}));
  if(T.Mode==ES47ScriptMode::Look)
  {
   Put(ModeRow,Words(TEXT("WITHIN deg"),19,2));Put(ModeRow,Num(TEXT("deg"),T.Degrees,[](double X){Draft().Trigger.Degrees=X;},60));
   Put(ModeRow,Words(TEXT("FROM m"),19,2));Put(ModeRow,Num(TEXT("m"),T.LookRange,[](double X){Draft().Trigger.LookRange=X;},60));
  }
  Put(ModeRow,Words(TEXT("DWELL s"),19,2));Put(ModeRow,Num(TEXT("s"),T.Dwell,[](double X){Draft().Trigger.Dwell=X;},60));
  V->AddSlot().AutoHeight().Padding(0,0,0,4)[ModeRow];
 }
 for(int32 I=0;I<T.Conds.Num();++I)V->AddSlot().AutoHeight().Padding(0,0,0,2)[BuildCond(I)];
 V->AddSlot().AutoHeight()[Btn(TEXT("+ CONDITION"),[this]{FS47ScriptCond C;C.If=TEXT("outdoors");Draft().Trigger.Conds.Add(C);Changed();})];
 return V;
}

TSharedRef<SWidget> SS47ScriptEditor::BuildCond(int32 I)
{
 using namespace S47ScriptEditorLocal;
 FS47ScriptCond& C=Draft().Trigger.Conds[I];
 TSharedRef<SHorizontalBox> H=SNew(SHorizontalBox);
 H->AddSlot().AutoWidth()[Cycle(TEXT("IF "),S47Script::CondKinds(),C.If,[I](const FString& S){FS47ScriptCond& Cc=Draft().Trigger.Conds[I];Cc.If=S;if(S==TEXT("weather")&&!S47Script::WeatherWords().Contains(Cc.Arg))Cc.Arg=TEXT("clear");if(S.StartsWith(TEXT("moon_")))Cc.Num=.5;if(S==TEXT("near")||S==TEXT("far"))Cc.Num=100;})];
 if(C.If==TEXT("finding")||C.If==TEXT("no_finding"))H->AddSlot().AutoWidth()[Field(TEXT("finding id, * at the end for any"),C.Arg,[I](const FString& S){Draft().Trigger.Conds[I].Arg=S.TrimStartAndEnd();},300)];
 else if(C.If.StartsWith(TEXT("case_")))
 {
  TArray<FString> Lines;for(int32 L=0;L<FS47FreeformCases::CaseCount;++L)Lines.Add(CaseOption(L));
  H->AddSlot().AutoWidth()[Cycle(TEXT("CASE "),Lines,CaseOption(FMath::Clamp(int32(C.Num),0,FS47FreeformCases::CaseCount-1)),[I](const FString& S){Draft().Trigger.Conds[I].Num=FCString::Atoi(*S);})];
 }
 else if(C.If==TEXT("weather"))H->AddSlot().AutoWidth()[Cycle(TEXT(""),S47Script::WeatherWords(),C.Arg,[I](const FString& S){Draft().Trigger.Conds[I].Arg=S;})];
 else if(C.If.StartsWith(TEXT("moon_")))H->AddSlot().AutoWidth()[Num(TEXT("lit 0-1"),C.Num,[I](double X){Draft().Trigger.Conds[I].Num=FMath::Clamp(X,0.,1.);})];
 else if(C.If==TEXT("near")||C.If==TEXT("far"))
 {
  H->AddSlot().AutoWidth()[Field(TEXT("place id"),C.Arg,[I](const FString& S){Draft().Trigger.Conds[I].Arg=S.TrimStartAndEnd();},180)];
  H->AddSlot().AutoWidth()[Num(TEXT("m"),C.Num,[I](double X){Draft().Trigger.Conds[I].Num=FMath::Max(0.,X);})];
 }
 else if(C.If==TEXT("story_flag")||C.If==TEXT("no_story_flag")) // DIRECTOR-MODE-01: a story flag's id, as the lint asks
  H->AddSlot().AutoWidth()[Field(TEXT("story flag id"),C.Arg,[I](const FString& S){Draft().Trigger.Conds[I].Arg=S.TrimStartAndEnd();},200)];
 H->AddSlot().AutoWidth().Padding(8,0,0,0)[Btn(TEXT("DEL"),[this,I]{Draft().Trigger.Conds.RemoveAt(I);Changed();})];
 return H;
}

TSharedRef<SWidget> SS47ScriptEditor::BuildAction(int32 I)
{
 using namespace S47ScriptEditorLocal;
 FS47ScriptAction& A=Draft().Actions[I];
 const auto Act=[I]()->FS47ScriptAction*{return Draft().Actions.IsValidIndex(I)?&Draft().Actions[I]:nullptr;};
 TSharedRef<SVerticalBox> V=SNew(SVerticalBox);
 // DIRECTOR-MODE-01, slice 4: a shot's row is also the shot list: which shot it is, its timing and lens at a glance, SELECT (the one
 // the director's camera works on, drawn yellow there) and PREVIEW FROM HERE. The selection follows the rows as they move or go.
 const bool bShot=A.Do==ES47ScriptAct::Shot;
 const bool bPicked=bShot&&S47ScriptEditor::SelectedShot()==I;
 TSharedRef<SHorizontalBox> Head=SNew(SHorizontalBox)
  +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,6,0)[Words(FString::Printf(TEXT("%s%d %s AT"),bPicked?TEXT("> "):TEXT(""),I+1,*FString(S47Script::ActName(A.Do)).ToUpper()),20,1)]
  +SHorizontalBox::Slot().AutoWidth()[Num(TEXT("s"),A.At,[Act](double X){if(auto* P=Act())P->At=FMath::Max(0.,X);},70)]
  +SHorizontalBox::Slot().AutoWidth().Padding(8,0,0,0)[Btn(TEXT("UP"),[this,I]{if(I>0){const int32 Was=S47ScriptEditor::SelectedShot();Draft().Actions.Swap(I,I-1);S47ScriptEditor::SelectShot(S47DirectorCam::AfterSwap(Was,I,I-1));Changed();}})]
  +SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("DOWN"),[this,I]{if(I+1<Draft().Actions.Num()){const int32 Was=S47ScriptEditor::SelectedShot();Draft().Actions.Swap(I,I+1);S47ScriptEditor::SelectShot(S47DirectorCam::AfterSwap(Was,I,I+1));Changed();}})]
  +SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("DEL"),[this,I]
  {
   if(!Draft().Actions.IsValidIndex(I))return;
   const int32 Was=S47ScriptEditor::SelectedShot();
   Draft().Actions.RemoveAt(I);
   S47ScriptEditor::SelectShot(S47DirectorCam::AfterRemove(Draft().Actions,Was,I));
   Changed();
  })];
 if(bShot)
 {
  const FString Lens=A.Fov2>0&&A.Fov2!=A.Fov?FString::Printf(TEXT("fov %s to %s"),*NumText(A.Fov),*NumText(A.Fov2)):FString::Printf(TEXT("fov %s"),*NumText(A.Fov));
  const FString Sum=FString::Printf(TEXT("shot %d: from %s s for %s s, %s, %s"),S47DirectorCam::ShotNumber(Draft().Actions,I),*NumText(A.At),*NumText(A.Secs),*Lens,A.Path.Num()>1?TEXT("moves to eye 2"):TEXT("one eye"));
  Head->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(12,0,6,0)[Words(Sum,19,bPicked?1:2)];
  Head->AddSlot().AutoWidth()[Btn(bPicked?TEXT("SELECTED"):TEXT("SELECT"),[this,I]{S47ScriptEditor::SelectShot(I);Say(TEXT("selected: the director's camera works on this shot (F8)"));Rebuild();})];
  Head->AddSlot().AutoWidth()[Btn(TEXT("PREVIEW FROM HERE"),[this,I]
  {
   if(!Draft().Actions.IsValidIndex(I))return;
   S47ScriptEditor::SelectShot(I);
   PreviewIt(Draft().Actions[I].At);
  })];
 }
 V->AddSlot().AutoHeight()[Head];
 TSharedRef<SHorizontalBox> H=SNew(SHorizontalBox);
 const auto Add=[&H](TSharedRef<SWidget> W){H->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,4,0)[W];};
 const auto Text=[this,&Add,Act](const FString& Hint,const FString& Value,float W){Add(Field(Hint,Value,[Act](const FString& S){if(auto* P=Act())P->Text=S;},W));};
 const auto Secs=[this,&Add,Act](const FString& Hint,double Value){Add(Words(Hint,19,2));Add(Num(TEXT("s"),Value,[Act](double X){if(auto* P=Act())P->Secs=FMath::Max(0.,X);},60));};
 const auto Point=[this,&V,I](const FString& Label){V->AddSlot().AutoHeight().Padding(24,0,0,0)[PointRow(Label,[I]{return Draft().Actions.IsValidIndex(I)?&Draft().Actions[I].Point:nullptr;})];};
 switch(A.Do)
 {
 case ES47ScriptAct::Figure:
  Add(Cycle(TEXT("BODY "),S47Script::Bodies(),A.Kind,[Act](const FString& S){if(auto* P=Act())P->Kind=S;}));
  Add(Words(TEXT("m/s"),19,2));Add(Num(TEXT("m/s"),A.Speed,[Act](double X){if(auto* P=Act())P->Speed=FMath::Clamp(X,.2,6.);},60));
  Secs(TEXT("stands"),A.Secs);
  Add(Btn(A.bFlag?TEXT("GONE WHEN SEEN"):TEXT("STAYS WHEN SEEN"),[this,Act]{if(auto* P=Act())P->bFlag=!P->bFlag;Changed();}));
  V->AddSlot().AutoHeight().Padding(24,0,0,0)[H];
  V->AddSlot().AutoHeight().Padding(24,0,0,0)
  [
   SNew(SHorizontalBox)
   +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,6,0)[Words(FString::Printf(TEXT("PATH %d points %s"),A.Path.Num(),A.Path.Num()?*PointText(A.Path.Last()):TEXT("")),19,2)]
   +SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("+ HERE"),[this,Act]{FS47ScriptPoint P;if(Player()&&Player()->HereRoad(P)){if(auto* X=Act())X->Path.Add(P);Changed();}else Say(TEXT("no player on the land here"),true);})]
   +SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("+ LOOK"),[this,Act]{FS47ScriptPoint P;if(Player()&&Player()->LookRoad(P)){P.Up=0;if(auto* X=Act())X->Path.Add(P);Changed();}else Say(TEXT("nothing in view to set a point on"),true);})]
   +SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("- LAST"),[this,Act]{if(auto* X=Act())if(X->Path.Num())X->Path.Pop();Changed();})]
  ];
  return V;
 case ES47ScriptAct::Sky:
  Add(Cycle(TEXT("FORM "),S47Script::Forms(),A.Kind,[Act](const FString& S){if(auto* P=Act())P->Kind=S;}));
  if(A.Kind!=TEXT("high_track"))Add(Cycle(TEXT("PACE "),S47Script::Paces(),A.Who,[Act](const FString& S){if(auto* P=Act())P->Who=S;}));
  Add(Words(TEXT("ahead of the player, outdoors"),18,2));
  break;
 case ES47ScriptAct::Sound:
  Text(TEXT("recording, e.g. thudSoft"),A.Text,200);
  Add(Cycle(TEXT(""),S47Script::SoundPresets(),A.Text,[Act](const FString& S){if(auto* P=Act())P->Text=S;}));
  Add(Cycle(TEXT("ON "),S47Script::SoundWhere(),A.Who,[Act](const FString& S){if(auto* P=Act())P->Who=S;}));
  Add(Words(TEXT("gain"),19,2));Add(Num(TEXT("gain"),A.Gain,[Act](double X){if(auto* P=Act())P->Gain=FMath::Clamp(X,0.,2.);},60));
  V->AddSlot().AutoHeight().Padding(24,0,0,0)[H];
  if(A.Who==TEXT("placed"))Point(TEXT("FROM"));
  return V;
 case ES47ScriptAct::Dish:
  Add(Words(TEXT("az"),19,2));Add(Num(TEXT("az"),A.Az,[Act](double X){if(auto* P=Act())P->Az=FMath::Fmod(X+360.,360.);},60));
  Add(Words(TEXT("el"),19,2));Add(Num(TEXT("el"),A.El,[Act](double X){if(auto* P=Act())P->El=FMath::Clamp(X,0.,90.);},60));
  Add(Words(TEXT("slew"),19,2));Add(Num(TEXT("slew"),A.Slew,[Act](double X){if(auto* P=Act())P->Slew=FMath::Max(.5,X);},60));
  Secs(TEXT("back after (0 stays)"),A.Secs);
  Add(Words(TEXT("antenna (-1 all)"),19,2));Add(Num(TEXT("-1"),A.Index,[Act](double X){if(auto* P=Act())P->Index=FMath::Clamp(int32(X),-1,26);},50));
  break;
 case ES47ScriptAct::Fault:
  Add(Cycle(TEXT("KIND "),S47Script::FaultKinds(),A.Kind,[Act](const FString& S){if(auto* P=Act())P->Kind=S;}));
  if(A.Kind==TEXT("omen")){Add(Words(TEXT("severity"),19,2));Add(Num(TEXT("0-1"),A.Gain,[Act](double X){if(auto* P=Act())P->Gain=FMath::Clamp(X,0.,1.);},60));}
  if(A.Kind!=TEXT("power"))Secs(TEXT("for"),A.Secs);
  else Add(Words(TEXT("the feeder fails until mended"),18,2));
  V->AddSlot().AutoHeight().Padding(24,0,0,0)[H];
  if(A.Kind==TEXT("omen"))Point(TEXT("AT (none: the player)"));
  return V;
 case ES47ScriptAct::Line:
  Add(Cycle(TEXT("WHO "),S47Script::Voices(),A.Who,[Act](const FString& S){if(auto* P=Act())P->Who=S;}));
  Text(TEXT("the words, in English"),A.Text,420);
  break;
 case ES47ScriptAct::Toast:
  Text(TEXT("on the screen"),A.Text,420);Secs(TEXT("for"),A.Secs);
  break;
 case ES47ScriptAct::Note:
  Add(Field(TEXT("title"),A.Title,[Act](const FString& S){if(auto* P=Act())P->Title=S;},160));
  Text(TEXT("the note, by hand"),A.Text,380);
  break;
 case ES47ScriptAct::Evidence:
  Add(Field(TEXT("finding id"),A.Kind,[Act](const FString& S){if(auto* P=Act())P->Kind=S.TrimStartAndEnd();},200));
  Text(TEXT("what is kept"),A.Text,360);
  break;
 case ES47ScriptAct::Look:
  Secs(TEXT("turn takes"),A.Secs);
  V->AddSlot().AutoHeight().Padding(24,0,0,0)[H];
  Point(TEXT("AT"));
  return V;
 case ES47ScriptAct::Wait:
  Secs(A.bFlag?TEXT("at most"):TEXT("for"),A.Secs);
  Add(Btn(A.bFlag?TEXT("UNTIL LOOKED AT"):TEXT("PLAIN WAIT"),[this,Act]{if(auto* P=Act())P->bFlag=!P->bFlag;Changed();}));
  V->AddSlot().AutoHeight().Padding(24,0,0,0)[H];
  if(A.bFlag)Point(TEXT("AT"));
  return V;
 case ES47ScriptAct::Shot:
 {
  // SHOT-BEAT-01: the lens (and the one it ends on, 0: the same), the hold, the ease in and out; then the eyes and what it looks at.
  Add(Words(TEXT("fov"),19,2));Add(Num(TEXT("deg"),A.Fov,[Act](double X){if(auto* P=Act())P->Fov=FMath::Clamp(X,20.,90.);},60));
  Add(Words(TEXT("to (0 same)"),19,2));Add(Num(TEXT("deg"),A.Fov2,[Act](double X){if(auto* P=Act())P->Fov2=X<=0?0.:FMath::Clamp(X,20.,90.);},60));
  Secs(TEXT("held"),A.Secs);
  Add(Words(TEXT("in"),19,2));Add(Num(TEXT("s"),A.In,[Act](double X){if(auto* P=Act())P->In=FMath::Clamp(X,.3,3.);},50));
  Add(Words(TEXT("out"),19,2));Add(Num(TEXT("s"),A.Out,[Act](double X){if(auto* P=Act())P->Out=FMath::Clamp(X,.3,3.);},50));
  V->AddSlot().AutoHeight().Padding(24,0,0,0)[H];
  for(int32 Which=0;Which<2;++Which)
  {
   const bool bHas=A.Path.IsValidIndex(Which);
   if(Which==1&&A.Path.IsEmpty())break; // a second eye only after a first
   // Sets eye Which (the first, or the second once there is a first); false when there is no such place in the path.
   const auto SetEye=[I,Which](const FS47ScriptPoint& Pt)
   {
    if(!Draft().Actions.IsValidIndex(I))return false;
    TArray<FS47ScriptPoint>& Eyes=Draft().Actions[I].Path;
    if(Which>Eyes.Num())return false;
    if(Which==Eyes.Num())Eyes.Add(Pt);else Eyes[Which]=Pt;
    return true;
   };
   V->AddSlot().AutoHeight().Padding(24,0,0,0)
   [
    SNew(SHorizontalBox)
    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,6,0)[Words((Which==0?FString(TEXT("EYE ")):FString(TEXT("EYE 2, moves there (optional) ")))+PointText(bHas?A.Path[Which]:FS47ScriptPoint()),19,2)]
    +SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("HERE"),[this,SetEye]{FS47ScriptPoint Pt;if(Player()&&Player()->HereRoad(Pt)){Pt.Up=ShotEyeUp;if(SetEye(Pt))Changed();}else Say(TEXT("no player on the land here"),true);})]
    +SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("LOOK"),[this,SetEye]{FS47ScriptPoint Pt;if(Player()&&Player()->LookRoad(Pt)){Pt.Up+=ShotEyeUp;if(SetEye(Pt))Changed();}else Say(TEXT("nothing in view to set a point on"),true);})]
    +SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("CLEAR"),[this,I,Which]{if(Draft().Actions.IsValidIndex(I)&&Draft().Actions[I].Path.IsValidIndex(Which))Draft().Actions[I].Path.RemoveAt(Which);Changed();})]
   ];
  }
  Point(TEXT("LOOKS AT"));
  return V;
 }
 default:break;
 }
 V->AddSlot().AutoHeight().Padding(24,0,0,0)[H];
 return V;
}

TSharedRef<SWidget> SS47ScriptEditor::BuildAdd()
{
 TSharedRef<SWrapBox> W=SNew(SWrapBox).UseAllottedSize(true);
 W->AddSlot()[Words(TEXT("ADD "),20,1)];
 for(int32 K=0;K<int32(ES47ScriptAct::Count);++K)
  W->AddSlot()[Btn(FString(TEXT("+ "))+FString(S47Script::ActName(ES47ScriptAct(K))).ToUpper(),[this,K]
  {
   FS47Script& D=S47ScriptEditorLocal::Draft();
   FS47ScriptAction A;A.Do=ES47ScriptAct(K);
   A.At=D.Actions.Num()?D.Actions.Last().At+2:0;
   switch(A.Do)
   {
   case ES47ScriptAct::Figure:A.Kind=TEXT("pale");A.Secs=6;A.bFlag=true;break;
   case ES47ScriptAct::Sky:A.Kind=TEXT("v");A.Who=TEXT("slow");break;
   case ES47ScriptAct::Sound:A.Text=TEXT("thudSoft");A.Who=TEXT("here");break;
   case ES47ScriptAct::Dish:A.Secs=30;break;
   case ES47ScriptAct::Fault:A.Kind=TEXT("omen");A.Gain=.6;A.Secs=15;break;
   case ES47ScriptAct::Line:A.Who=TEXT("dale");break;
   case ES47ScriptAct::Toast:A.Secs=4;break;
   case ES47ScriptAct::Evidence:A.Kind=TEXT("script_")+D.Id;break;
   case ES47ScriptAct::Look:A.Secs=1.5;if(Player())Player()->LookRoad(A.Point);break;
   case ES47ScriptAct::Wait:A.Secs=5;break;
   case ES47ScriptAct::Shot: // SHOT-BEAT-01: from where the player stands, at eye height, onto what they look at
   {
    A.Secs=6;
    FS47ScriptPoint Eye;
    if(Player()&&Player()->HereRoad(Eye)){Eye.Up=S47ScriptEditorLocal::ShotEyeUp;A.Path.Add(Eye);}
    if(Player())Player()->LookRoad(A.Point);
    break;
   }
   default:break;
   }
   D.Actions.Add(A);
   if(A.Do==ES47ScriptAct::Shot)S47ScriptEditor::SelectShot(D.Actions.Num()-1); // DIRECTOR-MODE-01: a new shot is the selected one
   Changed();
  })];
 return W;
}

TSharedRef<SWidget> SS47ScriptEditor::BuildBar()
{
 const auto Line=[this]()->FText
 {
  const FString Mark=S47ScriptEditorLocal::Dirty()?TEXT("UNSAVED  "):TEXT("");
  return FText::FromString(Mark+Status);
 };
 S47_USING_THEME
 const S47HudGauges::FHudPalette& Pal=S47HudGauges::Palette();
 const FLinearColor Ok=Css(Pal.Clock,Pal.ClockOn),Bad=Css(Pal.Low,Pal.LowOn);
 return SNew(SVerticalBox)
  +SVerticalBox::Slot().AutoHeight()
  [
   SNew(SHorizontalBox)
   +SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("SAVE"),[this]{SaveIt();},true)]
   +SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("PREVIEW (play now)"),[this]{PreviewIt();},true)]
   +SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("STOP"),[this]{StopIt();},true)]
   +SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("READ AGAIN"),[this]{if(Player())Player()->Reload();Say(TEXT("read the scripts again"));Rebuild();})]
   +SHorizontalBox::Slot().AutoWidth()[Btn(TEXT("CLOSE"),[this]{CloseIt();},true)]
  ]
  +SVerticalBox::Slot().AutoHeight().Padding(0,4,0,0)
  [
   SNew(STextBlock).Font(Term(19)).AutoWrapText(true).Text_Lambda(Line)
   .ColorAndOpacity_Lambda([this,Ok,Bad]{return FSlateColor(bStatusBad?Bad:Ok);})
  ];
}

void SS47ScriptEditor::Select(const FString& Id)
{
 using namespace S47ScriptEditorLocal;
 const FS47ScriptPlayer* P=Player();
 const FS47Script* S=P?P->Find(Id):nullptr;
 if(!S)return;
 Draft()=*S;HaveDraft()=true;Dirty()=false;
 S47ScriptEditor::SelectShot(INDEX_NONE);
 const FString* Why=P->Refused().Find(Id);
 Say(Why?TEXT("this one will not play: ")+*Why:TEXT("opened ")+Id,Why!=nullptr);
}

void SS47ScriptEditor::NewScript()
{
 using namespace S47ScriptEditorLocal;
 FS47Script S;
 S.Id=FString::Printf(TEXT("script_%s"),*FDateTime::Now().ToString(TEXT("%m%d_%H%M%S")));
 S.Name=TEXT("New script");
 S.bEnabled=false;
 S.Trigger.HourFrom=23;S.Trigger.HourTo=4;
 Draft()=S;HaveDraft()=true;Dirty()=true;
 S47ScriptEditor::SelectShot(INDEX_NONE);
 Say(TEXT("a new script: give it a name, a trigger and actions, then SAVE"));
}

void SS47ScriptEditor::SaveIt()
{
 using namespace S47ScriptEditorLocal;
 FS47ScriptPlayer* P=Player();
 if(!P){Say(TEXT("no script player"),true);return;}
 FString Why;
 if(P->Store(Draft(),Why)){Dirty()=false;if(const FS47Script* S=P->Find(Draft().Id))Draft().File=S->File;Say(TEXT("saved: ")+Why);}
 else Say(TEXT("not saved: ")+Why,true);
 Rebuild();
}

void SS47ScriptEditor::PreviewIt(double From)
{
 FS47ScriptPlayer* P=Player();
 if(!P){Say(TEXT("no script player"),true);return;}
 FString Why;
 // DIRECTOR-MODE-01, slice 4: from a shot's start, the actions before it left out (S47DirectorCam::FromTime); 0: the whole draft.
 if(!P->Preview(From>0?S47DirectorCam::FromTime(S47ScriptEditorLocal::Draft(),From):S47ScriptEditorLocal::Draft(),Why)){Say(TEXT("not played: ")+Why,true);Rebuild();return;}
 // The game has to run for it to play: the panel goes, the draft stays.
 US47Main* Main=M;
 const FString Name=S47ScriptEditorLocal::Draft().Name+(From>0?FString::Printf(TEXT(", from %s s"),*S47ScriptEditorLocal::NumText(From)):FString());
 CloseIt();
 if(Main&&Main->UI)Main->UI->Toast(TEXT("Preview: ")+Name+(Why.IsEmpty()?FString():TEXT(" (")+Why+TEXT(")")),4);
}

void SS47ScriptEditor::StopIt()
{
 if(FS47ScriptPlayer* P=Player()){P->StopRun(TEXT("the editor"));Say(TEXT("stopped"));}
 Rebuild();
}

void SS47ScriptEditor::CloseIt()
{
 if(M&&M->UI)M->UI->Close();
}

FReply SS47ScriptEditor::OnKeyDown(const FGeometry& G,const FKeyEvent& E)
{
 if(E.GetKey()==EKeys::Escape||E.GetKey()==EKeys::F10){CloseIt();return FReply::Handled();}
 return FReply::Unhandled();
}

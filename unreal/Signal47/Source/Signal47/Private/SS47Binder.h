#pragma once
#include "CoreMinimal.h"
#include "S47RecordsBinder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SCompoundWidget.h"

// RECORDS-BINDER (-> M3): the records-room binder, open on the table. A vinyl ring binder with its dividers' tabs along the top:
// the left-hand page is the divider's list (one line for each page behind it), the right-hand page is the page chosen, typed, a
// sheet at a time. It draws S47Binder::FModel and nothing else; what is in the binder is the session's (BinderSource).
//
// The keys are the notebook's (SS47Notebook): Q and E (LB, RB) turn the dividers, the arrows or W and S (the D-pad) choose a line,
// A and D (D-pad left and right) turn the sheets of a long page, Page Up and Page Down move a sheet that runs past the page,
// Enter, Space or the pad's A does the one thing the line offers (file the packet in hand, turn to the page's own divider, turn to
// the case it belongs to, the next sheet), and Escape, Tab or the pad's B closes it. Every line, tab and turn is a button as well.
// EVIDENCE-DOSSIER slice 3: what a line's Do asks of the session: the Do, the name field's text, the slip's words; true when it was done.
using FS47BinderDo=TFunction<bool(const FString&,const FString&,FString&)>;
class SS47Binder : public SCompoundWidget
{
public:
 SLATE_BEGIN_ARGS(SS47Binder){}
  SLATE_ARGUMENT(TSharedPtr<S47Binder::FView>,View)         // where it is open; the session keeps it between visits
  SLATE_ARGUMENT(TFunction<S47Binder::FModel()>,Source)     // the binder as it stands now
  SLATE_ARGUMENT(TFunction<bool(FString&)>,OnFile)          // puts the packet in hand in; the words for the slip either way
  SLATE_ARGUMENT(TFunction<void(const FString&)>,OnShown)   // this entry's page is on the right (a station file is then read)
  SLATE_ARGUMENT(TFunction<void()>,OnOlder)                 // the older binder on the same shelf
  SLATE_ARGUMENT(TFunction<void()>,OnLeave)
  // EVIDENCE-DOSSIER slice 3: a line's Do ("name:2", "merge:1:3", "out:1:3"), with what is in the name field; the slip's words either way
  SLATE_ARGUMENT(FS47BinderDo,OnDo)
 SLATE_END_ARGS()
 void Construct(const FArguments& Args);
 virtual bool SupportsKeyboardFocus() const override{return true;}
 virtual void Tick(const FGeometry& G,const double Time,const float Dt) override;
 virtual FReply OnKeyDown(const FGeometry& G,const FKeyEvent& E) override;
 // What the keys do, one step each.
 void Turn(int32 Dir);                   // the dividers
 void Move(int32 Dir);                   // the lines of the list
 void Leaf(int32 Dir,bool bRound=false); // the sheets of the page; bRound: after the last comes the first
 void Act();                             // the one thing the chosen line offers
 void Open(const FString& EntryKey);     // turn to an entry behind its own divider
 int32 Section() const;
 const S47Binder::FEntry* Shown() const; // the entry on the right-hand page, null on the front sheet
private:
 TSharedPtr<S47Binder::FView> Where;
 TFunction<S47Binder::FModel()> Source;
 TFunction<bool(FString&)> OnFile;
 TFunction<void(const FString&)> OnShown;
 TFunction<void()> OnOlder,OnLeave;
 FS47BinderDo OnDo;
 // EVIDENCE-DOSSIER slice 3: the name field on a file of the unknown at the binder (the notebook's search field's rules: the keys
 // are its own while it has them, Enter writes the name, Escape gives the keys back to the binder)
 TSharedPtr<class SEditableTextBox> NameBox;
 void Perform(const FString& Do);
 S47Binder::FModel Model;
 TArray<S47Binder::FRow> List;
 int32 Chosen=INDEX_NONE,BuiltTab=-1;
 uint32 PadSeen=0;
 TSharedPtr<SScrollBox> ListScroll,SheetScroll;
 TArray<TSharedPtr<SWidget>> LineWidgets;
 void Refresh(); // the model again, the choice kept, the pages drawn again
 void Rebuild();
 void File();
 void Forget();  // the slip from the last filing goes when the page is left
 FString ActWords() const;
 TSharedRef<SWidget> TabStrip();
 TSharedRef<SWidget> ListPage();
 TSharedRef<SWidget> SheetPage();
 TSharedRef<SWidget> HintStrip();
};

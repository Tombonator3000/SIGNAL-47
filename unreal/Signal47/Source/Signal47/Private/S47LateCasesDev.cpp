#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47Main.h"
#include "S47Predecessor.h"

// F10, "Late cases (M7)": weeks three and four (S47PredecessorLate.cpp). Each line sets up what play would bring, and the Go lines
// stand you where the trace is kept: somebody gone from a post in Caliche, the Sky Watch's bulletin and its minute (at the rack, or at
// the cairn), the raked ground at the farm, wagon 23's trip meter, the attendant's thermos and the letter after it. The day gates
// are let go for the rest of the session; nothing here is saved in a developer run.
void FS47DevFreeform::AddLateCases(FS47DevEvents& E)
{
 static const TCHAR* const G=TEXT("Late cases (M7)");
 const auto Pred=[this]()->FS47Predecessor*{auto* F=Session();if(!F||!F->Predecessor){Tell(false,TEXT("Freeform only"));return nullptr;}return F->Predecessor.Get();};
 const auto Do=[this,Pred](const FString& What){if(auto* P=Pred()){FString Why;const bool bOk=P->TestLate(What,Why);Tell(bOk,Why,7);}};
 const auto GoTo=[this,Pred](const FString& What,const FString& Say,double From,double To)
 {
  auto* P=Pred();if(!P)return;
  FString Area;double X=0,Z=0;
  if(!P->LateSpot(What,Area,X,Z)){Tell(false,TEXT("Nowhere to go yet: set it up first."));return;}
  FStage Stage;Stage.Area=Area;Stage.From=From;Stage.To=To;Stage.Where=[X,Z]{return FVector2D(X,Z);};
  Go(Stage,[this,Say]{Tell(true,Say,6);});
 };
 E.AddFreeform(G,TEXT("The empty post: somebody in Caliche gone now"),TEXT("the first of the four at their post goes, as the night's events would take them"),[Do]{Do(TEXT("post"));});
 E.AddFreeform(G,TEXT("Go: the empty post"),TEXT("in front of the place of whoever is gone, by day: the telephone or the radio from 45 m, kept at the door"),[GoTo]{GoTo(TEXT("post"),TEXT("Their place. Listen, then walk up to the door."),10,16);});
 E.AddFreeform(G,TEXT("Go: the control room, for the printer at the rack"),TEXT("a late case open: the printer runs a strip by itself, kept three seconds later"),[GoTo]{GoTo(TEXT("rack"),TEXT("The control room. The printer at the rack runs if a late case waits for its log."),10,16);});
 E.AddFreeform(G,TEXT("The printed minute: the Sky Watch's bulletin now"),TEXT("in the box at the road, or the OPS tray; its minute is ten to one on the third night from today"),[Do]{Do(TEXT("bulletin"));});
 E.AddFreeform(G,TEXT("The printed minute: one minute from now"),TEXT("then the meter and the printer at the rack (stay in the control room), or the wind at the cairn"),[Do]{Do(TEXT("minute"));});
 E.AddFreeform(G,TEXT("Go: the cairn on the rise"),TEXT("at night; after the minute the wind sounds in the stones from 40 m, kept within 10 m"),[GoTo]{GoTo(TEXT("cairn"),TEXT("The cairn. Walk up to it."),0.5,1.5);});
 E.AddFreeform(G,TEXT("The raked ground: at the farm, as the military left it"),TEXT("walk up to it: the visitors' own panel, and the case opens the same moment"),[Do]{Do(TEXT("raked"));});
 E.AddFreeform(G,TEXT("Go: the raked ground at the farm"),TEXT("by day, at its way in"),[GoTo]{GoTo(TEXT("raked"),TEXT("The farm. Walk out to the raked circle."),9,15);});
 E.AddFreeform(G,TEXT("Wagon 23: the trip meter"),TEXT("the wagon's panel as from day 15 with the call-out read: somebody has wiped the dust off the trip meter"),[Do]{Do(TEXT("trip"));});
 E.AddFreeform(G,TEXT("The attendant: the thermos"),TEXT("his talk, once the trip meter has been read"),[Do]{Do(TEXT("thermos"));});
 E.AddFreeform(G,TEXT("The thermos's letter now"),TEXT("as three days after the thermos went to the district: by post, or the OPS tray"),[Do]{Do(TEXT("reply"));});
 E.AddFreeform(G,TEXT("Open a late case on the case board"),TEXT("the board as it stands; the empty post if none is open"),[this]
 {
  auto* F=Session();if(!F){Tell(false,TEXT("Freeform only"));return;}
  int32 Line=FS47FreeformCases::PostLine;
  for(int32 I=FS47FreeformCases::LateFirst;I<FS47FreeformCases::CaseCount;++I)if(const auto* C=F->CaseFile().Get(I);C&&C->bOpen){Line=I;break;}
  F->OpenCaseBoard(Line);
 });
}

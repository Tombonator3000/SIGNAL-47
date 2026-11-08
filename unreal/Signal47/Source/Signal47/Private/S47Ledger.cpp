#include "S47Ledger.h"
#include "S47GameStyle.h"
#include "S47Places.h"
#include "S47Weather.h"
#include "SS47Widgets.h"
#include "Math/RandomStream.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace S47LedgerLocal
{
 using namespace S47Ledger;
 namespace X=S47LedgerText;
 // How many of each a paper carries: short enough to read in a minute, with pots that last far past the thirty days.
 constexpr int32 LocalsPer=3,SheriffPer=3,PlainAdsPer=2,FilmsPer=2;
 // The issue's volume: founded 1925, so the spring of 1986 is volume 61.
 constexpr int32 Volume=61;
 const TCHAR* const WeekDays[]={TEXT("MONDAY"),TEXT("TUESDAY"),TEXT("WEDNESDAY"),TEXT("THURSDAY"),TEXT("FRIDAY"),TEXT("SATURDAY"),TEXT("SUNDAY")};
 const TCHAR* const ShortDays[]={TEXT("MON"),TEXT("TUE"),TEXT("WED"),TEXT("THU"),TEXT("FRI"),TEXT("SAT"),TEXT("SUN")};
 const TCHAR* const Months[]={TEXT("JANUARY"),TEXT("FEBRUARY"),TEXT("MARCH"),TEXT("APRIL"),TEXT("MAY"),TEXT("JUNE"),TEXT("JULY"),TEXT("AUGUST"),TEXT("SEPTEMBER"),TEXT("OCTOBER"),TEXT("NOVEMBER"),TEXT("DECEMBER")};
 // A fixed order for one pot in one game: every issue takes the next few, so nothing comes round again for weeks.
 TArray<int32> Order(int32 Count,int32 Seed,uint32 Salt)
 {
  TArray<int32> Out;Out.Reserve(Count);for(int32 I=0;I<Count;++I)Out.Add(I);
  FRandomStream Stream(int32(uint32(Seed)*2654435761u^Salt*40503u));
  for(int32 I=Count-1;I>0;--I)Out.Swap(I,Stream.RandRange(0,I));
  return Out;
 }
 // Take: from the pot's own order, the Per entries of issue No (1-based).
 TArray<int32> Take(const TArray<int32>& Within,int32 No,int32 Per)
 {
  TArray<int32> Got;if(Within.IsEmpty())return Got;
  for(int32 K=0;K<Per;++K)Got.Add(Within[((FMath::Max(1,No)-1)*Per+K)%Within.Num()]);
  return Got;
 }
 FString Titled(const FString& St){if(St.IsEmpty())return St;FString Out=St;Out[0]=FChar::ToUpper(Out[0]);return Out;}
 FString Fill(const TCHAR* Text,const FString& Place,const FString& Night)
 {
  FString Out=Text;
  Out.ReplaceInline(TEXT("{Place}"),*Titled(Place.IsEmpty()?FString(TEXT("the old grade")):Place),ESearchCase::CaseSensitive);
  Out.ReplaceInline(TEXT("{place}"),Place.IsEmpty()?TEXT("the old grade"):*Place,ESearchCase::CaseSensitive);
  Out.ReplaceInline(TEXT("{night}"),Night.IsEmpty()?TEXT("one night this week"):*Night,ESearchCase::CaseSensitive);
  return Out;
 }
 FStory Story(const X::FFrontDef& D,const FString& Place,const FString& Night,const FString& Lead)
 {
  FStory St;St.Head=Fill(D.Head,Place,Night);St.Body=Fill(D.Body,Place,Night);St.Lead=Lead;return St;
 }
 // The paper's look: yellowed newsprint, ink, thin and heavy rules.
 FLinearColor Paper(){return S47Game::C(0xe6dec8);}
 FLinearColor Ink(){return S47Game::C(0x221f1a);}
 FLinearColor Faded(){return S47Game::C(0x4d473d);}
 // Slate units: the text area inside the panel (S47Kit::Shell at 1120 CSS pixels leaves about 1300).
 constexpr float TextW=1280.f,Cssp=1.25f;
 TSharedRef<SWidget> Words(const FString& St,const FSlateFontInfo& Font,float Wrap,const FLinearColor& Tone,ETextJustify::Type Justify=ETextJustify::Left)
 {
  return SNew(STextBlock).Text(FText::FromString(St)).Font(Font).ColorAndOpacity(Tone).WrapTextAt(Wrap).Justification(Justify).LineHeightPercentage(1.08f);
 }
 TSharedRef<SWidget> Rule(float Thick){return SNew(SBox).HeightOverride(Thick)[SNew(SBorder).BorderImage(S47Game::Box(Ink())).Padding(0)];}
 TSharedRef<SWidget> Upright(float Thick){return SNew(SBox).WidthOverride(Thick)[SNew(SBorder).BorderImage(S47Game::Box(Faded())).Padding(0)];}
 FSlateFontInfo Head(float Px){return S47Style::Display(Px*Cssp);}
 FSlateFontInfo Body(float Px){return S47Style::Body(Px*Cssp);}
 FSlateFontInfo Small(float Px){return S47Style::Mono(Px*Cssp);}
 // A column of short items under a heading.
 TSharedRef<SWidget> Column(const FString& Title,const TArray<FString>& Entries,float Wrap)
 {
  TSharedRef<SVerticalBox> Box=SNew(SVerticalBox);
  Box->AddSlot().AutoHeight().Padding(0,0,0,4)[Words(Title,Head(15.f),Wrap,Ink())];
  Box->AddSlot().AutoHeight().Padding(0,0,0,6)[Rule(1.f)];
  for(const FString& St:Entries)Box->AddSlot().AutoHeight().Padding(0,0,0,8)[Words(St,Body(11.5f),Wrap,Ink())];
  return Box;
 }
}

const TArray<S47Ledger::FCoupon>& S47Ledger::Coupons()
{
 static const TArray<FCoupon> Table={
  {TEXT("cell3"),TEXT("cell"),3,TEXT("Dry cells for $3 at the Caliche counter")},
  {TEXT("foil2"),TEXT("foil"),2,TEXT("Shielding foil for $2 at the Caliche counter")},
  {TEXT("spare12"),TEXT("spare"),12,TEXT("A spare wheel for $12 at the Caliche counter")},
  {TEXT("fuse4"),TEXT("fuse"),4,TEXT("Control fuses for $4 at the Caliche counter")},
  {TEXT("relay21"),TEXT("relay"),21,TEXT("Drive relays for $21 at the Caliche counter")},
  {TEXT("fuel8"),TEXT("fuel"),8,TEXT("A six-hour fuel can for $8 at the Caliche counter")},
  {TEXT("film8"),TEXT("film"),8,TEXT("A roll of film for $8 at the Caliche counter")},
  {TEXT("motel6"),TEXT("motel"),6,TEXT("A room at the Sierra Motor Court for $6")},
  {TEXT("post0"),TEXT("post"),0,TEXT("Postage paid on a mail order")},
  {TEXT("motor22"),TEXT("mail:drive_motor"),22,TEXT("A drive motor by mail for $22")},
  {TEXT("recorder12"),TEXT("mail:recorder_donor"),12,TEXT("A used field recorder by mail for $12")},
 };
 return Table;
}
const S47Ledger::FCoupon* S47Ledger::Coupon(const FString& Id){return Coupons().FindByPredicate([&](const FCoupon& Cp){return Id==Cp.Id;});}
FString S47Ledger::DateLine(const FDateTime& Date)
{
 namespace L=S47LedgerLocal;
 return FString::Printf(TEXT("%s, %s %d, %d"),L::WeekDays[int32(Date.GetDayOfWeek())],L::Months[Date.GetMonth()-1],Date.GetDay(),Date.GetYear());
}

TArray<S47Ledger::FStory> S47Ledger::Front(int32 Seed,int32 No,const TArray<FFact>& Facts,const FString& UnknownId,const FString& UnknownTitle)
{
 namespace L=S47LedgerLocal;namespace X=S47LedgerText;
 TArray<FFact> Sorted=Facts;
 Sorted.StableSort([](const FFact& A,const FFact& B){return uint8(A.Topic)<uint8(B.Topic);});
 // One story to a topic, the man and his truck once, and no place twice.
 TArray<FStory> Out;TSet<uint8> Told;TSet<FString> Where;
 for(const FFact& Fa:Sorted)
 {
  if(Out.Num()>=2)break;
  if(Fa.Topic==ETopic::Helped||Fa.Topic==ETopic::Count||Told.Contains(uint8(Fa.Topic)))continue;
  if(Fa.Topic==ETopic::Missing&&Told.Contains(uint8(ETopic::Called)))continue;
  if(!Fa.Place.IsEmpty()&&Where.Contains(Fa.Place))continue;
  TArray<const X::FFrontDef*> Variants;for(const X::FFrontDef& D:X::Fronts())if(D.Topic==Fa.Topic)Variants.Add(&D);
  if(Variants.IsEmpty())continue;
  const X::FFrontDef* D=Variants[uint32(Seed+No*7+int32(Fa.Topic)*3)%uint32(Variants.Num())];
  Out.Add(L::Story(*D,Fa.Place,Fa.Night,Fa.PlaceId));
  Told.Add(uint8(Fa.Topic));if(!Fa.Place.IsEmpty())Where.Add(Fa.Place);
 }
 const auto& Quiet=X::Quiet();
 if(No<=1)
 {
  // The first paper the new operator sees: the search, whatever else the week brought.
  if(Out.Num()>1)Out.SetNum(1);
  Out.Add(L::Story(Quiet[0],FString(),FString(),TEXT("wagon")));
  if(Out.Num()<2)Out.Add(L::Story(Quiet[1],FString(),FString(),FString()));
 }
 else
 {
  // After that the county's own news, the station's review first, the rest in this game's order.
  TArray<int32> Rest;Rest.Add(1);for(const int32 I:L::Order(Quiet.Num()-2,Seed,77))Rest.Add(I+2);
  for(int32 Slot=Out.Num();Slot<2;++Slot)Out.Add(L::Story(Quiet[Rest[((No-2)*2+Slot)%Rest.Num()]],FString(),FString(),FString()));
 }
 // A week that points at nothing gets the talk about somewhere the player has not heard of.
 const bool bLead=Out.ContainsByPredicate([](const FStory& St){return !St.Lead.IsEmpty();});
 if(!bLead&&!UnknownId.IsEmpty())
 {
  TArray<const X::FFrontDef*> Talk;for(const X::FFrontDef& D:X::Fronts())if(D.Topic==ETopic::Rumor)Talk.Add(&D);
  if(!Talk.IsEmpty())Out.Last()=L::Story(*Talk[uint32(Seed+No)%uint32(Talk.Num())],UnknownTitle,FString(),UnknownId);
 }
 return Out;
}

void S47Ledger::Fill(FIssue& Issue,int32 Seed,bool bHelped)
{
 namespace L=S47LedgerLocal;namespace X=S47LedgerText;
 const int32 No=FMath::Max(1,Issue.No);
 Issue.Locals.Reset();Issue.Sheriff.Reset();Issue.Ads.Reset();Issue.Letters.Reset();Issue.Week.Reset();
 for(const int32 I:L::Take(L::Order(X::Locals().Num(),Seed,101),No,L::LocalsPer))Issue.Locals.Add(X::Locals()[I]);
 // The sheriff's log runs back over the week, a day to an entry.
 const TArray<int32> Log=L::Take(L::Order(X::Sheriff().Num(),Seed,102),No,L::SheriffPer);
 for(int32 K=0;K<Log.Num();++K)
 {
  const FDateTime On=Issue.Date-FTimespan::FromDays(6-K*2-(K==2?1:0));
  Issue.Sheriff.Add(FString::Printf(TEXT("%s. %s"),*L::Titled(FString(L::WeekDays[int32(On.GetDayOfWeek())]).ToLower()),X::Sheriff()[Log[K]]));
 }
 // Ads: one with a coupon (the catalog's in the first paper, the week the catalog came), two without.
 TArray<int32> WithCoupon,Plain;int32 CatalogAd=-1;
 for(int32 I=0;I<X::Ads().Num();++I)
 {
  if(!X::Ads()[I].Coupon)Plain.Add(I);
  else if(FCString::Strcmp(X::Ads()[I].Coupon,TEXT("post0"))==0)CatalogAd=I;
  else WithCoupon.Add(I);
 }
 TArray<int32> CouponOrder;for(const int32 I:L::Order(WithCoupon.Num(),Seed,103))CouponOrder.Add(WithCoupon[I]);
 const int32 Pick=No==1&&CatalogAd>=0?CatalogAd:CouponOrder.IsEmpty()?-1:L::Take(CouponOrder,No-1,1)[0];
 if(Pick>=0)Issue.Ads.Add({X::Ads()[Pick].Name,X::Ads()[Pick].Body,X::Ads()[Pick].Coupon});
 {
  TArray<int32> PlainOrder;for(const int32 I:L::Order(Plain.Num(),Seed,104))PlainOrder.Add(Plain[I]);
  for(const int32 I:L::Take(PlainOrder,No,L::PlainAdsPer))Issue.Ads.Add({X::Ads()[I].Name,X::Ads()[I].Body,FString()});
 }
 {
  const TArray<int32> Films=L::Take(L::Order(X::Films().Num(),Seed,105),No,L::FilmsPer);
  Issue.Showing=FString::Printf(TEXT("%s at 7. %s at 9. Saturday matinee at 2. Adults $2.50, children $1."),X::Films()[Films[0]],X::Films()[Films[1]]);
 }
 // Letters: one that points somewhere, one that does not, and the trucker's thanks when there is cause for it.
 if(bHelped)Issue.Letters.Add({X::Thanks().Head,X::Thanks().Body,FString()});
 TArray<int32> Leads,Others;
 for(int32 I=0;I<X::Letters().Num();++I)(X::Letters()[I].Lead?Leads:Others).Add(I);
 {
  TArray<int32> LeadOrder,OtherOrder;
  for(const int32 I:L::Order(Leads.Num(),Seed,106))LeadOrder.Add(Leads[I]);
  for(const int32 I:L::Order(Others.Num(),Seed,107))OtherOrder.Add(Others[I]);
  for(const int32 I:L::Take(LeadOrder,No,1))Issue.Letters.Add({X::Letters()[I].Head,X::Letters()[I].Body,X::Letters()[I].Lead});
  for(const int32 I:L::Take(OtherOrder,No,1))Issue.Letters.Add({X::Letters()[I].Head,X::Letters()[I].Body,FString()});
 }
 // The weather: what the forecaster says for the five days from the issue, and what the courthouse thermometer read last week.
 for(int32 K=0;K<5;++K)
 {
  const S47Weather::FDay Said=S47Weather::Forecast(Seed,Issue.Day,Issue.Day+K);
  const FDateTime On=Issue.Date+FTimespan::FromDays(K);
  Issue.Week.Add(K==0?TEXT("TODAY. ")+S47Weather::Words(Said):FString::Printf(TEXT("%s  %s"),L::ShortDays[int32(On.GetDayOfWeek())],*S47Weather::Words(Said,true)));
 }
 int32 High=-100,Low=200,Wet=0,Dust=0;
 for(int32 D=FMath::Max(1,Issue.Day-7);D<Issue.Day;++D){const S47Weather::FDay Word=S47Weather::Day(Seed,D);High=FMath::Max(High,Word.HighF);Low=FMath::Min(Low,Word.LowF);Wet+=Word.bShower?1:0;Dust+=Word.bDust?1:0;}
 Issue.LastWeek=High<-50?FString(TEXT("The courthouse thermometer has nothing to report yet.")):FString::Printf(TEXT("Last week at the courthouse: high %d, low %d, %s. Blowing dust on %d %s."),High,Low,Wet>0?TEXT("a trace of rain"):TEXT("no rain"),Dust,Dust==1?TEXT("day"):TEXT("days"));
 {
  const FDateTime NewMoon=S47Weather::NextPhase(Issue.Date,0),FullMoon=S47Weather::NextPhase(Issue.Date,2);
  const FDateTime& Next=NewMoon<FullMoon?NewMoon:FullMoon;
  Issue.Almanac=FString::Printf(TEXT("Sun rises %s, sets %s. Moon: %s. %s %s, %s %d."),*S47Weather::ClockWords(S47Weather::SunriseClock),*S47Weather::ClockWords(S47Weather::SunsetClock),*S47Weather::MoonPhase(Issue.Date),
   NewMoon<FullMoon?TEXT("New moon"):TEXT("Full moon"),*L::Titled(FString(L::WeekDays[int32(Next.GetDayOfWeek())]).ToLower()),*L::Titled(FString(L::Months[Next.GetMonth()-1]).ToLower()),Next.GetDay());
 }
}

TSharedRef<SWidget> S47Ledger::Page(const FIssue& Issue)
{
 namespace L=S47LedgerLocal;
 const float FrontW=L::TextW*.64f-24.f,SideW=L::TextW*.36f-24.f,ColW=(L::TextW-2*28.f)/3.f,AdW=(L::TextW-2*16.f)/3.f-20.f;
 TSharedRef<SVerticalBox> Sheet=SNew(SVerticalBox);
 // The masthead.
 Sheet->AddSlot().AutoHeight().HAlign(HAlign_Center)[L::Words(TEXT("THE CALICHE COUNTY LEDGER"),L::Head(42.f),L::TextW,L::Ink(),ETextJustify::Center)];
 Sheet->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0,0,0,6)[L::Words(TEXT("Serving Caliche and the valley since 1925. Published Thursdays."),L::Body(10.5f),L::TextW,L::Faded(),ETextJustify::Center)];
 Sheet->AddSlot().AutoHeight()[L::Rule(3.f)];
 Sheet->AddSlot().AutoHeight().Padding(0,4)
 [
  SNew(SHorizontalBox)
  +SHorizontalBox::Slot().FillWidth(1.f)[L::Words(FString::Printf(TEXT("VOL. %d, NO. %d"),L::Volume,Issue.Date.GetDayOfYear()/7+1),L::Small(12.f),L::TextW/3,L::Ink())]
  +SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center)[L::Words(DateLine(Issue.Date),L::Small(12.f),L::TextW/3,L::Ink(),ETextJustify::Center)]
  +SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right)[L::Words(TEXT("TWENTY-FIVE CENTS"),L::Small(12.f),L::TextW/3,L::Ink(),ETextJustify::Right)]
 ];
 Sheet->AddSlot().AutoHeight().Padding(0,0,0,10)[L::Rule(1.f)];
 // The front page and, beside it, the weather, the almanac and the picture show.
 TSharedRef<SVerticalBox> FrontCol=SNew(SVerticalBox);
 for(int32 I=0;I<Issue.Front.Num();++I)
 {
  const FStory& St=Issue.Front[I];
  if(I>0)FrontCol->AddSlot().AutoHeight().Padding(0,4,0,8)[L::Rule(1.f)];
  FrontCol->AddSlot().AutoHeight().Padding(0,0,0,6)[L::Words(St.Head,L::Head(I==0?27.f:20.f),FrontW,L::Ink())];
  FrontCol->AddSlot().AutoHeight().Padding(0,0,0,6)[L::Words(St.Body,L::Body(I==0?13.f:12.f),FrontW,L::Ink())];
 }
 TSharedRef<SVerticalBox> Side=SNew(SVerticalBox);
 Side->AddSlot().AutoHeight().Padding(0,0,0,4)[L::Words(TEXT("THE WEEK'S WEATHER"),L::Head(16.f),SideW,L::Ink())];
 Side->AddSlot().AutoHeight().Padding(0,0,0,6)[L::Rule(1.f)];
 for(const FString& St:Issue.Week)Side->AddSlot().AutoHeight().Padding(0,0,0,4)[L::Words(St,L::Small(11.5f),SideW,L::Ink())];
 Side->AddSlot().AutoHeight().Padding(0,4,0,10)[L::Words(Issue.LastWeek,L::Body(10.5f),SideW,L::Faded())];
 Side->AddSlot().AutoHeight().Padding(0,0,0,4)[L::Words(TEXT("ALMANAC"),L::Head(15.f),SideW,L::Ink())];
 Side->AddSlot().AutoHeight().Padding(0,0,0,10)[L::Words(Issue.Almanac,L::Body(10.5f),SideW,L::Ink())];
 Side->AddSlot().AutoHeight().Padding(0,0,0,4)[L::Words(TEXT("AT THE PLAZA"),L::Head(15.f),SideW,L::Ink())];
 Side->AddSlot().AutoHeight()[L::Words(Issue.Showing,L::Body(10.5f),SideW,L::Ink())];
 Sheet->AddSlot().AutoHeight()
 [
  SNew(SHorizontalBox)
  +SHorizontalBox::Slot().FillWidth(.64f).Padding(0,0,22,0)[FrontCol]
  +SHorizontalBox::Slot().AutoWidth()[L::Upright(1.f)]
  +SHorizontalBox::Slot().FillWidth(.36f).Padding(22,0,0,0)[Side]
 ];
 Sheet->AddSlot().AutoHeight().Padding(0,10)[L::Rule(2.f)];
 // Three columns: the county, the sheriff's log, the letters and classifieds.
 TArray<FString> Notes;for(const FNote& Nt:Issue.Letters)Notes.Add(Nt.Head+TEXT(". ")+Nt.Body);
 Sheet->AddSlot().AutoHeight()
 [
  SNew(SHorizontalBox)
  +SHorizontalBox::Slot().FillWidth(1.f).Padding(0,0,14,0)[L::Column(TEXT("AROUND THE COUNTY"),Issue.Locals,ColW)]
  +SHorizontalBox::Slot().AutoWidth()[L::Upright(1.f)]
  +SHorizontalBox::Slot().FillWidth(1.f).Padding(14,0)[L::Column(TEXT("SHERIFF'S LOG"),Issue.Sheriff,ColW)]
  +SHorizontalBox::Slot().AutoWidth()[L::Upright(1.f)]
  +SHorizontalBox::Slot().FillWidth(1.f).Padding(14,0,0,0)[L::Column(TEXT("LETTERS AND CLASSIFIEDS"),Notes,ColW)]
 ];
 Sheet->AddSlot().AutoHeight().Padding(0,10)[L::Rule(2.f)];
 // The ads, each in its own box; a coupon's box is drawn heavier.
 TSharedRef<SHorizontalBox> Ads=SNew(SHorizontalBox);
 for(int32 I=0;I<Issue.Ads.Num();++I)
 {
  const FAd& A=Issue.Ads[I];const bool bCoupon=!A.Coupon.IsEmpty();
  TSharedRef<SVerticalBox> In=SNew(SVerticalBox);
  In->AddSlot().AutoHeight().Padding(0,0,0,4)[L::Words(A.Name,L::Head(15.f),AdW,L::Ink())];
  In->AddSlot().AutoHeight()[L::Words(A.Body,L::Body(11.f),AdW,L::Ink())];
  if(bCoupon)In->AddSlot().AutoHeight().Padding(0,6,0,0)[L::Words(TEXT("- - - COUPON - - - CUT ON THE LINE - - -"),L::Small(10.f),AdW,L::Faded())];
  Ads->AddSlot().FillWidth(1.f).Padding(I==0?0.f:8.f,0,I+1<Issue.Ads.Num()?8.f:0.f,0)
  [
   SNew(SBorder).BorderImage(S47Game::Box(L::Paper(),L::Ink(),bCoupon?2.f:1.f)).Padding(10)[In]
  ];
 }
 Sheet->AddSlot().AutoHeight()[Ads];
 return SNew(SBorder).BorderImage(S47Game::Box(L::Paper(),S47Game::C(0x8a826f),1.f)).Padding(FMargin(18,16))[Sheet];
}

FString S47Ledger::PlainText(const FIssue& Issue)
{
 FString Out=FString::Printf(TEXT("THE CALICHE COUNTY LEDGER\n%s\n\n"),*DateLine(Issue.Date));
 for(const FStory& St:Issue.Front)Out+=St.Head+TEXT("\n")+St.Body+TEXT("\n\n");
 Out+=TEXT("THE WEEK'S WEATHER\n");for(const FString& St:Issue.Week)Out+=St+TEXT("\n");
 Out+=Issue.LastWeek+TEXT("\n")+Issue.Almanac+TEXT("\n\nAROUND THE COUNTY\n");
 for(const FString& St:Issue.Locals)Out+=St+TEXT("\n");
 Out+=TEXT("\nSHERIFF'S LOG\n");for(const FString& St:Issue.Sheriff)Out+=St+TEXT("\n");
 Out+=TEXT("\nLETTERS AND CLASSIFIEDS\n");for(const FNote& Nt:Issue.Letters)Out+=Nt.Head+TEXT(". ")+Nt.Body+TEXT("\n");
 Out+=TEXT("\nAT THE PLAZA\n")+Issue.Showing+TEXT("\n\n");
 for(const FAd& A:Issue.Ads)Out+=A.Name+TEXT(". ")+A.Body+TEXT("\n");
 return Out;
}

bool S47Ledger::Clean(const FString& Text,FString& Word)
{
 // Whole words where it matters ("taken", not "mistaken"); plain pieces for numbers and phrases with stops in them.
 static const TCHAR* const Spoken[]={
  TEXT("forty-seven"),TEXT("forty seven"),TEXT("tomas"),TEXT("tomás"),TEXT("vega"),TEXT("silhouette"),TEXT("third figure"),TEXT("cable"),TEXT("the cut"),TEXT("sight line"),TEXT("closing point"),
  TEXT("kinski"),TEXT("alien"),TEXT("ufo"),TEXT("saucer"),TEXT("abduct"),TEXT("taken"),TEXT("roswell"),TEXT("photograph of"),TEXT("the event"),TEXT("old road"),
  TEXT("chevy"),TEXT("chevrolet"),TEXT("ford"),TEXT("pontiac"),TEXT("dodge"),TEXT("toyota"),TEXT("datsun"),TEXT("winnebago"),TEXT("cadillac"),TEXT("pepsi"),TEXT("coke"),TEXT("coca"),TEXT("coors"),
  TEXT("budweiser"),TEXT("levi"),TEXT("stetson"),TEXT("john deere"),TEXT("polaroid"),TEXT("kodak"),TEXT("zenith"),TEXT("motorola"),TEXT("radio shack"),TEXT("sears"),TEXT("kmart"),TEXT("k mart"),
  TEXT("walmart"),TEXT("texaco"),TEXT("exxon"),TEXT("conoco"),TEXT("greyhound"),TEXT("jeep"),TEXT("holiday inn"),TEXT("mcdonald"),TEXT("dairy queen"),
  TEXT("tyre"),TEXT("grey"),TEXT("programme"),TEXT("colour"),TEXT("neighbour"),TEXT("favour"),TEXT("harbour"),TEXT("centre"),TEXT("metre"),TEXT("kerb"),TEXT("cheque"),TEXT("aluminium"),
  TEXT("lorry"),TEXT("petrol"),TEXT("fortnight"),TEXT("whilst"),TEXT("windscreen"),TEXT("jewellery"),TEXT("realise"),TEXT("organise"),TEXT("recognise"),
  TEXT("reyes, he"),TEXT("reyes, she"),TEXT("mr. reyes"),TEXT("ms. reyes"),TEXT("mrs. reyes"),TEXT("reyes's wife"),TEXT("reyes's husband")};
 static const TCHAR* const Pieces[]={TEXT("47"),TEXT("8.15"),TEXT("u.f.o")};
 const FString Low=Text.ToLower();
 for(const TCHAR* Bit:Pieces)if(Low.Contains(Bit)){Word=Bit;return false;}
 for(const TCHAR* Token:Spoken)
 {
  const int32 Len=FCString::Strlen(Token);
  for(int32 At=Low.Find(Token,ESearchCase::CaseSensitive);At!=INDEX_NONE;At=Low.Find(Token,ESearchCase::CaseSensitive,ESearchDir::FromStart,At+1))
  {
   const bool bStart=At==0||!FChar::IsAlpha(Low[At-1]);
   const int32 After=At+Len;
   const bool bEnd=After>=Low.Len()||!FChar::IsAlpha(Low[After])||(Low[After]==TEXT('s')&&(After+1>=Low.Len()||!FChar::IsAlpha(Low[After+1])));
   if(bStart&&bEnd){Word=Token;return false;}
  }
 }
 return true;
}

TArray<FString> S47Ledger::AllText()
{
 namespace X=S47LedgerText;
 TArray<FString> Out;
 for(const TCHAR* St:X::Locals())Out.Add(St);
 for(const TCHAR* St:X::Sheriff())Out.Add(St);
 for(const X::FAdDef& A:X::Ads()){Out.Add(A.Name);Out.Add(A.Body);}
 for(const X::FLetterDef& Nt:X::Letters()){Out.Add(Nt.Head);Out.Add(Nt.Body);}
 Out.Add(X::Thanks().Head);Out.Add(X::Thanks().Body);
 for(const TCHAR* St:X::Films())Out.Add(St);
 for(const X::FFrontDef& D:X::Fronts()){Out.Add(D.Head);Out.Add(D.Body);}
 for(const X::FFrontDef& D:X::Quiet()){Out.Add(D.Head);Out.Add(D.Body);}
 for(const FCoupon& Cp:Coupons())Out.Add(Cp.Line);
 return Out;
}

bool S47Ledger::SelfTest(TArray<FString>& Lines)
{
 namespace L=S47LedgerLocal;namespace X=S47LedgerText;
 bool OK=true;
 const auto Check=[&](bool B,const FString& Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s ledger: %s"),B?TEXT("PASS"):TEXT("FAIL"),*Why));};
 Check(X::Locals().Num()>=60&&X::Ads().Num()>=25&&X::Letters().Num()>=25&&X::Sheriff().Num()>=20&&X::Films().Num()>=10&&X::Quiet().Num()>=8,
  FString::Printf(TEXT("the pots: %d county items, %d ads, %d letters and classifieds, %d sheriff's entries, %d films"),X::Locals().Num(),X::Ads().Num(),X::Letters().Num(),X::Sheriff().Num(),X::Films().Num()));
 {
  bool bWords=true;FString Bad;
  for(const FString& St:AllText()){FString Word;if(!Clean(St,Word)){bWords=false;Bad=Word+TEXT(" in: ")+St.Left(60);}}
  Check(bWords,bWords?FString(TEXT("nothing in the pots says a word from Story's far side, a brand or a British spelling")):TEXT("a word the paper must not print: ")+Bad);
  FString Probe;
  Check(!Clean(TEXT("She was taken."),Probe)&&Clean(TEXT("He was mistaken."),Probe)&&!Clean(TEXT("Lot 47."),Probe)&&Clean(TEXT("We can afford it."),Probe),TEXT("the word check takes whole words and every number with the forbidden digits"));
 }
 bool bCoupons=true;for(const X::FAdDef& A:X::Ads())if(A.Coupon)bCoupons&=Coupon(A.Coupon)!=nullptr;
 Check(bCoupons,TEXT("every coupon an ad carries exists"));
 bool bLeads=true;int32 LeadCount=0;for(const X::FLetterDef& Nt:X::Letters())if(Nt.Lead){++LeadCount;bLeads&=FS47Places::Def(Nt.Lead)!=nullptr;}
 Check(bLeads&&LeadCount>=5,TEXT("letters that point somewhere point at real places on the land"));
 bool bTopics=true;for(uint8 Tp=0;Tp<uint8(ETopic::Helped);++Tp)bTopics&=X::Fronts().ContainsByPredicate([Tp](const X::FFrontDef& D){return uint8(D.Topic)==Tp;});
 Check(bTopics,TEXT("each kind of night has a front-page story"));
 // Five weeks of a game, twice: the same paper both times, and nothing in the pots printed twice.
 const auto Make=[](int32 Seed,int32 No,const TArray<FFact>& Facts)
 {
  FIssue I;I.No=No;I.Day=4+(No-1)*7;I.Date=FDateTime(1986,5,1)+FTimespan::FromDays((No-1)*7);
  I.Front=Front(Seed,No,Facts,TEXT("shack"),TEXT("the line shack"));Fill(I,Seed,No==3);return I;
 };
 const TArray<FFact> Week={{ETopic::Lights,3,TEXT("Tuesday night"),TEXT("the windmill on the Baca lease"),TEXT("windmill")},{ETopic::Cattle,3,TEXT("Tuesday night"),TEXT("the Alderete place"),FString()}};
 bool bSame=true,bUnique=true,bFull=true,bShort=true,bPrinted=true;TSet<FString> Seen;int32 MostWords=0;
 for(const int32 TrySeed:{470047,1,-77,90210})for(int32 No=1;No<=5;++No)
 {
  const FIssue A=Make(TrySeed,No,No==2?Week:TArray<FFact>()),B=Make(TrySeed,No,No==2?Week:TArray<FFact>());
  bSame&=PlainText(A)==PlainText(B);
  if(TrySeed==470047)
  {
   TArray<FString> Printed=A.Locals;Printed.Append(A.Sheriff);for(const FNote& Nt:A.Letters)if(Nt.Head!=X::Thanks().Head)Printed.Add(Nt.Body);for(const FAd& Ad:A.Ads)Printed.Add(Ad.Body);
   for(const FString& Bit:Printed){bUnique&=!Seen.Contains(Bit);Seen.Add(Bit);}
  }
  bFull&=A.Front.Num()==2&&A.Locals.Num()==L::LocalsPer&&A.Sheriff.Num()==L::SheriffPer&&A.Ads.Num()==1+L::PlainAdsPer&&A.Week.Num()==5&&!A.Showing.IsEmpty()&&!A.Almanac.IsEmpty();
  TArray<FString> Counted;PlainText(A).ParseIntoArrayWS(Counted);MostWords=FMath::Max(MostWords,Counted.Num());
  const FString Text=PlainText(A);bShort&=!Text.Contains(TEXT("{"))&&!Text.Contains(TEXT("}"));
  FString Word;bPrinted&=Clean(Text,Word);
 }
 Check(bPrinted,TEXT("no paper as printed says a forbidden word, the weather and the dates included"));
 Check(bSame,TEXT("a game's paper is the same every time it is opened"));
 Check(bUnique,TEXT("no county item, log entry, letter or ad is printed twice in five weeks"));
 Check(bFull,TEXT("every paper has its front page, county news, the log, ads, weather, almanac and the picture show"));
 Check(bShort&&MostWords<=600,FString::Printf(TEXT("short enough to read through in a minute or so (%d words at most) and nothing left unfilled"),MostWords));
 {
  const FIssue First=Make(470047,1,TArray<FFact>());
  Check(First.Front[0].Head.Contains(TEXT("SEARCH"))&&First.Front[0].Lead==TEXT("wagon"),TEXT("the first paper carries the search for the two missing since April 14, and points at the wagon"));
  const FIssue Second=Make(470047,2,Week);
  Check(Second.Front[0].Body.Contains(TEXT("the Alderete place"))||Second.Front[0].Body.Contains(TEXT("the windmill on the Baca lease")),TEXT("a week with lights and cattle puts them on the front page, at the place they were"));
  Check(Second.Front.ContainsByPredicate([](const FStory& St){return St.Lead==TEXT("windmill");}),TEXT("and the story at a place on the land points at it"));
  const FIssue Quiet=Make(470047,3,TArray<FFact>());
  Check(Quiet.Front.Last().Lead==TEXT("shack")&&Quiet.Front.Last().Body.Contains(TEXT("the line shack")),TEXT("a quiet week gets talk about a place nobody has mentioned yet"));
  Check(Quiet.Letters.Num()==3&&Quiet.Letters[0].Head==X::Thanks().Head,TEXT("the trucker's thanks is printed the week there was cause"));
  Check(Second.Week[1].EndsWith(S47Weather::Words(S47Weather::Forecast(470047,Second.Day,Second.Day+1),true)),TEXT("the forecast is S47Weather's, for the days after the paper"));
 }
 return OK;
}

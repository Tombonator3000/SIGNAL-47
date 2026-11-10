#include "S47Ledger.h"
#include "S47GameStyle.h"
#include "S47Places.h"
#include "S47Weather.h"
#include "SS47Widgets.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Math/RandomStream.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateTypes.h"
#include "TextureResource.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Text/STextBlock.h"

namespace S47LedgerLocal
{
 using namespace S47Ledger;
 namespace X=S47LedgerText;
 // How many of each a paper carries: short enough to read in a minute, with pots that last far past the thirty days.
 constexpr int32 LocalsPer=3,SheriffPer=3,PlainAdsPer=2,FilmsPer=2,MostLogged=2;
 // The issue's volume: founded 1925, so the spring of 1986 is volume 61.
 constexpr int32 Volume=61;
 const TCHAR* const Masthead=TEXT("THE CALICHE LEDGER");
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
 // What a paper prints from the pots that tell stories with threads and dates (the county briefs, the plain ads, the letters that
 // point somewhere and the rest), and the threads they put on the page. Every pot, in this game's own order, gives the first items
 // that are in their window on the paper's date, were not printed in an earlier paper, and are not on a thread already on the page
 // (the pots in a fixed order, sharing the page). The earlier papers are played through the same way, all pots together, by their
 // dates (a paper every Thursday), so what this paper takes is exactly what was not printed before, and a Continue prints the same.
 // Only the pots decide it: the front page keeps clear of these threads (Front), never the other way round. Only when a pot has run
 // dry does an item come round again.
 struct FPotItem{FString Thread;int32 First=0,Last=0;};
 struct FPlanned{TArray<int32> Locals,PlainAds,LeadLetters,OtherLetters;TSet<FString> Threads;};
 FPlanned PlanIssue(int32 Seed,int32 No,const FDateTime& Date)
 {
  struct FPot{TArray<FPotItem> Items;TArray<int32> Index,Ord;TSet<int32> Printed;int32 Per=1;TArray<int32> FPlanned::* Into=nullptr;};
  TArray<FPot> Pots;Pots.SetNum(4);
  for(int32 I=0;I<X::Locals().Num();++I){const X::FItemDef& D=X::Locals()[I];Pots[0].Items.Add({D.Thread?FString(D.Thread):FString(),D.First,D.Last});Pots[0].Index.Add(I);}
  for(int32 I=0;I<X::Ads().Num();++I)if(!X::Ads()[I].Coupon){const X::FAdDef& D=X::Ads()[I];Pots[1].Items.Add({FString(),D.First,D.Last});Pots[1].Index.Add(I);}
  for(int32 I=0;I<X::Letters().Num();++I){const X::FLetterDef& D=X::Letters()[I];FPot& P=Pots[D.Lead?2:3];P.Items.Add({D.Thread?FString(D.Thread):FString(),D.First,D.Last});P.Index.Add(I);}
  const uint32 Salts[]={101u,104u,106u,107u};const int32 Pers[]={LocalsPer,PlainAdsPer,1,1};
  TArray<int32> FPlanned::* const Intos[]={&FPlanned::Locals,&FPlanned::PlainAds,&FPlanned::LeadLetters,&FPlanned::OtherLetters};
  for(int32 P=0;P<4;++P){Pots[P].Ord=Order(Pots[P].Items.Num(),Seed,Salts[P]);Pots[P].Per=Pers[P];Pots[P].Into=Intos[P];}
  const int32 Last=FMath::Max(1,No);
  FPlanned Out;
  for(int32 K=1;K<=Last;++K)
  {
   const FDateTime On=Date-FTimespan::FromDays(7*(Last-K));
   FPlanned Page;
   for(FPot& Pot:Pots)
   {
    TArray<int32> This;
    for(const bool bAgain:{false,true})
    {
     for(const int32 I:Pot.Ord)
     {
      if(This.Num()>=Pot.Per)break;
      const FPotItem& It=Pot.Items[I];
      if(This.Contains(I)||(!bAgain&&Pot.Printed.Contains(I))||!X::InWindow(It.First,It.Last,On)||(!It.Thread.IsEmpty()&&Page.Threads.Contains(It.Thread)))continue;
      This.Add(I);if(!It.Thread.IsEmpty())Page.Threads.Add(It.Thread);
     }
     if(This.Num()>=Pot.Per)break;
    }
    for(const int32 I:This){Pot.Printed.Add(I);(Page.*Pot.Into).Add(Pot.Index[I]);}
   }
   if(K==Last)Out=MoveTemp(Page);
  }
  return Out;
 }
 FString Titled(const FString& St){if(St.IsEmpty())return St;FString Out=St;Out[0]=FChar::ToUpper(Out[0]);return Out;}
 bool NeedsPlace(const TCHAR* Text){return FString(Text).Contains(TEXT("{place}"),ESearchCase::IgnoreCase);}
 // Where and when. A story that needs a place is never written without one (FS47Mail drops it): the neutral words below only stand
 // in so that nothing is ever left unfilled, and they name no place in the game.
 FString Fill(const TCHAR* Text,const FString& Place,const FString& Night)
 {
  FString Out=Text;
  const FString Where=Place.IsEmpty()?FString(TEXT("somewhere out of town")):Place;
  Out.ReplaceInline(TEXT("{Place}"),*Titled(Where),ESearchCase::CaseSensitive);
  Out.ReplaceInline(TEXT("{place}"),*Where,ESearchCase::CaseSensitive);
  Out.ReplaceInline(TEXT("{night}"),Night.IsEmpty()?TEXT("one night this week"):*Night,ESearchCase::CaseSensitive);
  return Out;
 }
 FStory Story(const X::FFrontDef& D,const FString& Place,const FString& Night,const FString& Lead,const FString& FactId)
 {
  FStory St;St.Head=Fill(D.Head,Place,Night);St.Body=Fill(D.Body,Place,Night);St.Lead=Lead;St.Key=D.Head;St.FactId=FactId;return St;
 }
 // Of these templates, the one not printed before (in this game's order), else the one printed longest ago.
 const X::FFrontDef* Choose(const TArray<const X::FFrontDef*>& Variants,const TMap<FString,int32>& Used,int32 Seed,uint32 Salt)
 {
  if(Variants.IsEmpty())return nullptr;
  const TArray<int32> Ord=Order(Variants.Num(),Seed,Salt);
  for(const int32 I:Ord)if(!Used.Contains(Variants[I]->Head))return Variants[I];
  const X::FFrontDef* Oldest=nullptr;int32 When=MAX_int32;
  for(const int32 I:Ord){const int32 At=Used.FindRef(Variants[I]->Head);if(At<When){When=At;Oldest=Variants[I];}}
  return Oldest;
 }
 const X::FFrontDef* QuietOf(const FString& Key){return X::Quiet().FindByPredicate([&](const X::FFrontDef& D){return Key==D.Head;});}
 // The paper's look: newsprint, ink, thin and heavy rules. D45-24-20261110: the paper a little greyer and the ink deeper, as
 // newsprint and press ink are (MAP_L_ledger.md); the player's pencil a graphite grey.
 FLinearColor Paper(){return S47Game::C(0xe9e3d2);}
 FLinearColor Ink(){return S47Game::C(0x1d1b18);}
 FLinearColor Faded(){return S47Game::C(0x5a5347);}
 FLinearColor Graphite(float A){return S47Game::C(0x4a4a50,A);}
 // Slate units: the text area inside the panel (S47Kit::Shell at 1120 CSS pixels leaves about 1300).
 constexpr float TextW=1280.f,Cssp=1.25f;
 TSharedRef<SWidget> Words(const FString& St,const FSlateFontInfo& Font,float Wrap,const FLinearColor& Tone,ETextJustify::Type Justify=ETextJustify::Left)
 {
  return SNew(STextBlock).Text(FText::FromString(St)).Font(Font).ColorAndOpacity(Tone).WrapTextAt(Wrap).Justification(Justify).LineHeightPercentage(1.08f);
 }
 TSharedRef<SWidget> Rule(float Thick){return SNew(SBox).HeightOverride(Thick)[SNew(SBorder).BorderImage(S47Game::Box(Ink())).Padding(0)];}
 TSharedRef<SWidget> Upright(float Thick){return SNew(SBox).WidthOverride(Thick)[SNew(SBorder).BorderImage(S47Game::Box(Faded())).Padding(0)];}

 // ---------- D45-24-20261110: a 1986 small-town weekly ----------
 // Tom: «skal se ut som en ekte avis». Six columns of 200 with gutters of 16 (6 x 200 + 5 x 16 = TextW) and a hairline in each
 // gutter between stories; heads across the columns in a condensed face (Oswald Medium, the look of the condensed gothic heads of
 // eighties weeklies), the body in a serif (Liberation Serif, SIL OFL 1.1, in Content/Fonts beside the others, read raw like them
 // by S47Style::Font; the typewriter face where the files are missing), the nameplate between its ears, thick-thin rules,
 // datelines, bylines, a folio and an index, a halftone on the front, newsprint with grain and the fold. Slate has no full
 // justification (ETextJustify), so the columns are set ragged right, as many small weeklies set theirs.
 constexpr float ColW=200.f,Gut=16.f;
 constexpr float SpanW(int32 N){return ColW*N+Gut*(N-1);}
 const TCHAR* const Nameplate=TEXT("The Caliche Ledger");
 const TCHAR* const Motto=TEXT("Serving Caliche and the valley since 1925. Published Thursdays.");
 const TCHAR* const Town=TEXT("CALICHE, NEW MEXICO");
 const TCHAR* const Price=TEXT("TWENTY-FIVE CENTS");
 const TCHAR* const Dateline=TEXT("CALICHE — ");
 const TCHAR* const ByStaff=TEXT("BY THE LEDGER STAFF");
 const TCHAR* const ByReport=TEXT("LEDGER STAFF REPORT");
 const TCHAR* const EarToday=TEXT("TODAY");
 const TCHAR* const EarInside=TEXT("PAGE 2");
 const TCHAR* const EarInsideText=TEXT("The sheriff's log, letters and classifieds, the weather and the picture show.");
 const TCHAR* const InsideHead=TEXT("INSIDE");
 const TCHAR* const InsideItems[]={TEXT("Sheriff's log"),TEXT("Letters and classifieds"),TEXT("The week's weather"),TEXT("Almanac"),TEXT("At the Plaza"),TEXT("This week's coupon")};
 const TCHAR* const SectionHeads[]={TEXT("AROUND THE COUNTY"),TEXT("SHERIFF'S LOG"),TEXT("LETTERS AND CLASSIFIEDS"),TEXT("THE WEEK'S WEATHER"),TEXT("ALMANAC"),TEXT("AT THE PLAZA")};
 const TCHAR* const CutLine=TEXT("CLIP ON THE DOTTED LINE");
 const TCHAR* const WagonTitle=TEXT("the district's wagon on the flat west of the old grade");
 const TCHAR* const PhotoLine=TEXT(", Wednesday. Ledger photo.");
 const TCHAR* const WildCaption=TEXT("The valley south of Caliche, Wednesday. Ledger photo.");
 // The player's pencil in the margin: on a story that points somewhere, and once it is on the map.
 const TCHAR* const NoteAsk=TEXT("Pencil it in on the map?");
 const TCHAR* const NoteDone=TEXT("on the map");

 bool HasFont(const TCHAR* File){return IFileManager::Get().FileExists(*FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()/TEXT("Fonts")/File));}
 enum class EFace:uint8{Regular,Bold,Italic};
 FSlateFontInfo Serif(float Px,EFace Face=EFace::Regular,int32 Spacing=0)
 {
  static const TCHAR* const Files[]={TEXT("LiberationSerif-Regular.ttf"),TEXT("LiberationSerif-Bold.ttf"),TEXT("LiberationSerif-Italic.ttf")};
  static const bool bHave[]={HasFont(Files[0]),HasFont(Files[1]),HasFont(Files[2])};
  const int32 Which=int32(Face);
  FSlateFontInfo Font=bHave[Which]?S47Style::Font(Files[Which],Px*Cssp):S47Style::Body(Px*Cssp);
  Font.LetterSpacing=Spacing;
  return Font;
 }
 FSlateFontInfo Cond(float Px,int32 Spacing=0){FSlateFontInfo Font=S47Style::Font(TEXT("Oswald-Medium.ttf"),Px*Cssp);Font.LetterSpacing=Spacing;return Font;}
 // Rich text for the run-ins (the dateline, the day in the sheriff's log): the paper's words escaped, <b> bold, <i> italic.
 FString Esc(const FString& St)
 {
  FString Out=St;
  Out.ReplaceInline(TEXT("&"),TEXT("&amp;"),ESearchCase::CaseSensitive);
  Out.ReplaceInline(TEXT("<"),TEXT("&lt;"),ESearchCase::CaseSensitive);
  Out.ReplaceInline(TEXT(">"),TEXT("&gt;"),ESearchCase::CaseSensitive);
  return Out;
 }
 FString Bold(const FString& St){return TEXT("<b>")+Esc(St)+TEXT("</>");}
 struct FRunStyle{FTextBlockStyle Base;TSharedPtr<FSlateStyleSet> Set;};
 const FRunStyle& Runs(float Px,const FLinearColor& Tone)
 {
  static TMap<FString,TSharedPtr<FRunStyle>> Cache;
  const FString Key=FString::Printf(TEXT("%.2f/%s"),Px,*Tone.ToString());
  TSharedPtr<FRunStyle>& Got=Cache.FindOrAdd(Key);
  if(!Got.IsValid())
  {
   Got=MakeShared<FRunStyle>();
   Got->Base=FTextBlockStyle().SetFont(Serif(Px)).SetColorAndOpacity(FSlateColor(Tone));
   Got->Set=MakeShared<FSlateStyleSet>(FName(*(TEXT("S47Ledger/")+Key)));
   FTextBlockStyle Heavy=Got->Base;Heavy.SetFont(Serif(Px,EFace::Bold));Got->Set->Set(TEXT("b"),Heavy);
   FTextBlockStyle Slant=Got->Base;Slant.SetFont(Serif(Px,EFace::Italic));Got->Set->Set(TEXT("i"),Slant);
  }
  return *Got;
 }
 TSharedRef<SWidget> Para(const FString& Markup,float Px,float Wrap,const FLinearColor& Tone,ETextJustify::Type Justify=ETextJustify::Left)
 {
  const FRunStyle& Style=Runs(Px,Tone);
  return SNew(SRichTextBlock).Text(FText::FromString(Markup)).TextStyle(&Style.Base).DecoratorStyleSet(Style.Set.Get()).WrapTextAt(Wrap).Justification(Justify).LineHeightPercentage(1.1f);
 }
 TSharedRef<SWidget> ThickThin()
 {
  return SNew(SVerticalBox)+SVerticalBox::Slot().AutoHeight()[Rule(4.f)]+SVerticalBox::Slot().AutoHeight().Padding(0,2,0,0)[Rule(1.f)];
 }
 // A section's head between a heavy rule and a thin one.
 TSharedRef<SWidget> Section(const FString& Title,float Wrap)
 {
  return SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()[Rule(2.f)]
   +SVerticalBox::Slot().AutoHeight().Padding(0,3,0,2)[Words(Title,Cond(15.f,40),Wrap,Ink())]
   +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,6)[Rule(1.f)];
 }
 // One piece on the grid, Span columns wide; between two stories a hairline in the gutter.
 void Cell(const TSharedRef<SHorizontalBox>& Row,int32 Span,const TSharedRef<SWidget>& Content,bool bRule)
 {
  if(Row->NumSlots()>0)
  {
   if(bRule)Row->AddSlot().AutoWidth().Padding((Gut-1.f)*.5f,0.f)[Upright(1.f)];
   else Row->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(Gut)];
  }
  Row->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(SpanW(Span))[Content]];
 }
 // A transient picture from pixels (the S47Game::Soft pattern), kept for the session by its owner.
 UTexture2D* MakeTex(int32 W,int32 H,const TArray<FColor>& Pixels,bool bSRGB)
 {
  if(!FApp::CanEverRender()||Pixels.Num()!=W*H)return nullptr;
  UTexture2D* T=UTexture2D::CreateTransient(W,H,PF_B8G8R8A8);if(!T)return nullptr;
  T->SRGB=bSRGB;T->Filter=TF_Bilinear;
  void* Data=T->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
  FMemory::Memcpy(Data,Pixels.GetData(),Pixels.Num()*sizeof(FColor));
  T->GetPlatformData()->Mips[0].BulkData.Unlock();
  T->UpdateResource();
  return T;
 }
 // Newsprint's grain: a tile of fine dark specks with a fleck of ink here and there, repeated over the sheet.
 const FSlateBrush* Grain()
 {
  static TStrongObjectPtr<UTexture2D> Tex;
  static TSharedPtr<FSlateBrush> Brush;
  if(Brush.IsValid())return Brush.Get();
  constexpr int32 N=128;
  TArray<FColor> Pixels;Pixels.SetNum(N*N);
  FRandomStream Stream(1986);
  for(FColor& Px:Pixels){const float V=Stream.FRand();const bool bFleck=Stream.FRand()<.01f;Px=FColor(0,0,0,uint8(FMath::Clamp(V*V*110.f+(bFleck?170.f:0.f),0.f,255.f)));}
  UTexture2D* T=MakeTex(N,N,Pixels,false);if(!T)return nullptr;
  Tex.Reset(T);
  Brush=MakeShared<FSlateBrush>();
  Brush->SetResourceObject(T);Brush->ImageSize=FVector2D(N,N);Brush->DrawAs=ESlateBrushDrawType::Image;Brush->Tiling=ESlateBrushTileType::Both;
  return Brush.Get();
 }
 // Over the sheet, letting every click through: the grain, and the fold across the middle where it lay folded in the box.
 class SNewsprint:public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SNewsprint){}
  SLATE_END_ARGS()
  void Construct(const FArguments&){}
  virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(1.,1.);}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& Geo,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool)const override
  {
   const FVector2f Size=FVector2f(Geo.GetLocalSize());
   if(const FSlateBrush* Specks=Grain())FSlateDrawElement::MakeBox(Out,Layer,Geo.ToPaintGeometry(),Specks,ESlateDrawEffect::None,FLinearColor(1.f,1.f,1.f,.55f));
   const FSlateBrush* Flat=S47Game::Box(FLinearColor::White);
   const float Fold=FMath::RoundToFloat(Size.Y*.5f);
   for(int32 K=1;K<=8;++K)
   {
    const float Shade=.012f*float(9-K)/8.f;
    FSlateDrawElement::MakeBox(Out,Layer+1,Geo.ToPaintGeometry(FVector2f(Size.X,3.f),FSlateLayoutTransform(FVector2f(0.f,Fold-3.f*K))),Flat,ESlateDrawEffect::None,FLinearColor(0.f,0.f,0.f,Shade));
    FSlateDrawElement::MakeBox(Out,Layer+1,Geo.ToPaintGeometry(FVector2f(Size.X,3.f),FSlateLayoutTransform(FVector2f(0.f,Fold+1.f+3.f*(K-1)))),Flat,ESlateDrawEffect::None,FLinearColor(0.f,0.f,0.f,Shade*.7f));
   }
   FSlateDrawElement::MakeBox(Out,Layer+2,Geo.ToPaintGeometry(FVector2f(Size.X,1.f),FSlateLayoutTransform(FVector2f(0.f,Fold-1.f))),Flat,ESlateDrawEffect::None,FLinearColor(1.f,1.f,1.f,.35f));
   FSlateDrawElement::MakeBox(Out,Layer+2,Geo.ToPaintGeometry(FVector2f(Size.X,1.f),FSlateLayoutTransform(FVector2f(0.f,Fold))),Flat,ESlateDrawEffect::None,FLinearColor(0.f,0.f,0.f,.13f));
   return Layer+3;
  }
 };

 // The front page's photo, a halftone drawn here (no picture file, and nothing from the far side: no lights, no figures): the
 // flat, far mesas with flat tops, scrub, and the thing the story is about, set in a 45-degree screen of ink dots on the paper,
 // with the muddy contrast of a weekly's press. Kind: FS47Places::FDef::Kind, "wagon", or empty for a picture of the valley.
 constexpr int32 PhotoW=416,PhotoH=220;
 float Hash1(int32 I,uint32 Seed){uint32 H=uint32(I)*374761393u+Seed*668265263u;H=(H^(H>>13))*1274126177u;return float(H^(H>>16))/4294967295.f;}
 float Noise1(float At,uint32 Seed){const int32 I=FMath::FloorToInt(At);const float T=At-float(I);return FMath::Lerp(Hash1(I,Seed),Hash1(I+1,Seed),T*T*(3.f-2.f*T));}
 bool InBox(float Qx,float Qy,float X0,float Y0,float X1,float Y1){return Qx>=X0&&Qx<=X1&&Qy>=Y0&&Qy<=Y1;}
 bool InOval(float Qx,float Qy,float Cx,float Cy,float Rx,float Ry){return FMath::Square((Qx-Cx)/Rx)+FMath::Square((Qy-Cy)/Ry)<=1.f;}
 bool OnSeg(float Qx,float Qy,float Ax,float Ay,float Bx,float By,float Thick)
 {
  const FVector2f P(Qx,Qy),A(Ax,Ay),AB=FVector2f(Bx,By)-A;
  const float T=FMath::Clamp(FVector2f::DotProduct(P-A,AB)/FMath::Max(AB.SizeSquared(),1e-4f),0.f,1.f);
  return FVector2f::Distance(P,A+AB*T)<=Thick*.5f;
 }
 // The subject's tone where it covers (Qx,Qy), or -1 where it does not. Bx,By: where it stands on the flat.
 float Subject(const FString& Kind,float Qx,float Qy,float Bx,float By)
 {
  if(Kind==TEXT("windmill"))
  {
   const float Cy=By-128.f;
   if(FMath::Square(Qx-Bx)+FMath::Square(Qy-Cy)<=26.f*26.f){const float D=FMath::Sqrt(FMath::Square(Qx-Bx)+FMath::Square(Qy-Cy));if(D>=22.5f)return .12f;const float A=FMath::Atan2(Qy-Cy,Qx-Bx);if(D<5.f||FMath::Abs(FMath::Sin(A*9.f))<.16f)return .16f;}
   if(OnSeg(Qx,Qy,Bx,Cy,Bx+40.f,Cy+2.f,2.f)||InBox(Qx,Qy,Bx+34.f,Cy-9.f,Bx+50.f,Cy+11.f))return .14f;
   if(OnSeg(Qx,Qy,Bx-15.f,By,Bx-3.f,Cy+4.f,2.6f)||OnSeg(Qx,Qy,Bx+15.f,By,Bx+3.f,Cy+4.f,2.6f))return .12f;
   for(int32 K=1;K<=3;++K){const float Y=By-K*30.f,Half=15.f-12.f*(By-Y)/(By-Cy);if(OnSeg(Qx,Qy,Bx-Half,Y,Bx+Half,Y,1.6f))return .16f;}
   if(OnSeg(Qx,Qy,Bx-14.f,By-2.f,Bx+11.f,By-30.f,1.2f)||OnSeg(Qx,Qy,Bx+14.f,By-2.f,Bx-11.f,By-30.f,1.2f))return .2f;
   return -1.f;
  }
  if(Kind==TEXT("tank"))
  {
   if(InOval(Qx,Qy,Bx,By-24.f,52.f,5.f))return .6f;
   if(InBox(Qx,Qy,Bx-52.f,By-24.f,Bx+52.f,By))return FMath::Abs(FMath::Sin(Qx*.5f))<.25f?.22f:.32f;
   return -1.f;
  }
  if(Kind==TEXT("mine"))
  {
   if(InBox(Qx,Qy,Bx-13.f,By-32.f,Bx-9.f,By)||InBox(Qx,Qy,Bx+9.f,By-32.f,Bx+13.f,By)||InBox(Qx,Qy,Bx-15.f,By-36.f,Bx+15.f,By-31.f))return .58f;
   if(InBox(Qx,Qy,Bx-9.f,By-31.f,Bx+9.f,By))return .05f;
   if(InBox(Qx,Qy,Bx+26.f,By-8.f,Bx+50.f,By))return FMath::Abs(FMath::Sin(Qy*1.4f))<.3f?.3f:.55f;
   if(Qy<=By&&InOval(Qx,Qy,Bx,By+16.f,120.f,66.f))return .38f+.06f*Noise1(Qx*.2f+Qy*.3f,9u);
   return -1.f;
  }
  if(Kind==TEXT("balloon"))
  {
   if(InBox(Qx,Qy,Bx-44.f,By-15.f,Bx-28.f,By))return InBox(Qx,Qy,Bx-42.f,By-13.f,Bx-30.f,By-2.f)?.78f:.2f;
   if(InOval(Qx,Qy,Bx,By-6.f,28.f,8.f))return .62f+.1f*FMath::Sin(Qx*.6f);
   if(InOval(Qx,Qy,Bx+38.f,By-4.f,22.f,6.f))return .42f;
   if(OnSeg(Qx,Qy,Bx+16.f,By-6.f,Bx+28.f,By-4.f,1.f))return .2f;
   return -1.f;
  }
  if(Kind==TEXT("cow"))
  {
   if(InOval(Qx,Qy,Bx,By-11.f,34.f,11.f)||InOval(Qx,Qy,Bx-38.f,By-6.f,10.f,6.f))return .14f;
   if(OnSeg(Qx,Qy,Bx+8.f,By-18.f,Bx+24.f,By-34.f,3.f)||OnSeg(Qx,Qy,Bx+20.f,By-16.f,Bx+38.f,By-28.f,3.f))return .16f;
   return -1.f;
  }
  if(Kind==TEXT("stake"))
  {
   if(InBox(Qx,Qy,Bx+1.f,By-36.f,Bx+11.f,By-29.f))return .38f;
   if(OnSeg(Qx,Qy,Bx,By,Bx,By-36.f,2.6f)||OnSeg(Qx,Qy,Bx+14.f,By,Bx+14.f,By-20.f,2.2f))return .18f;
   return -1.f;
  }
  if(Kind==TEXT("shack"))
  {
   if(InBox(Qx,Qy,Bx-8.f,By-28.f,Bx+8.f,By)||InBox(Qx,Qy,Bx+18.f,By-30.f,Bx+30.f,By-20.f))return .06f;
   if(Qy>=By-60.f&&Qy<By-40.f&&FMath::Abs(Qx-Bx)<=50.f*(Qy-(By-62.f))/22.f)return .18f;
   if(InBox(Qx,Qy,Bx-44.f,By-40.f,Bx+44.f,By))return FMath::Abs(FMath::Sin(Qx*.7f))<.2f?.24f:.34f;
   return -1.f;
  }
  if(Kind==TEXT("trough"))
  {
   if(InBox(Qx,Qy,Bx+56.f,By-12.f,Bx+70.f,By))return .84f;
   if(InBox(Qx,Qy,Bx-42.f,By-14.f,Bx+42.f,By-10.f))return .1f;
   if(InBox(Qx,Qy,Bx-46.f,By-14.f,Bx+46.f,By))return .26f;
   return -1.f;
  }
  if(Kind==TEXT("cairn"))
  {
   if(OnSeg(Qx,Qy,Bx,By-36.f,Bx,By-66.f,2.6f))return .16f;
   const float Ys[]={6.f,17.f,27.f,35.f},Rxs[]={22.f,17.f,12.f,8.f},Rys[]={7.f,6.f,5.f,4.f};
   for(int32 K=0;K<4;++K)if(InOval(Qx,Qy,Bx+(K==1?-2.f:K==2?1.f:0.f),By-Ys[K],Rxs[K],Rys[K]))return .3f+.08f*Noise1(Qx*.5f+K*7.f,3u);
   return -1.f;
  }
  if(Kind==TEXT("saucer rock"))
  {
   if(InOval(Qx,Qy,Bx,By-74.f,40.f,9.f))return .34f;
   if(Qy<=By&&Qy>=By-72.f&&FMath::Abs(Qx-Bx)<=9.f+6.f*(Qy-(By-72.f))/72.f)return .42f+.08f*Noise1(Qy*.3f,5u);
   return -1.f;
  }
  if(Kind==TEXT("motel"))
  {
   if(OnSeg(Qx,Qy,Bx+110.f,By,Bx+110.f,By-70.f,3.f)||InBox(Qx,Qy,Bx+92.f,By-96.f,Bx+128.f,By-70.f))return .24f;
   if(OnSeg(Qx,Qy,Bx-122.f,By-31.f,Bx+92.f,By-31.f,3.f))return .2f;
   if(InBox(Qx,Qy,Bx-120.f,By-30.f,Bx+90.f,By)){const float Bay=FMath::Fmod(Qx-(Bx-120.f),30.f);return Bay>8.f&&Bay<18.f&&Qy>By-22.f?.18f:.66f;}
   return -1.f;
  }
  if(Kind==TEXT("wagon"))
  {
   if(FMath::Square(Qx-(Bx-30.f))+FMath::Square(Qy-(By-5.f))<=49.f||FMath::Square(Qx-(Bx+30.f))+FMath::Square(Qy-(By-5.f))<=49.f)return .05f;
   if(InBox(Qx,Qy,Bx-36.f,By-28.f,Bx-8.f,By-20.f)||InBox(Qx,Qy,Bx-4.f,By-28.f,Bx+26.f,By-20.f))return .16f;
   if(OnSeg(Qx,Qy,Bx-48.f,By-12.f,Bx+48.f,By-12.f,1.4f))return .32f;
   if(InBox(Qx,Qy,Bx-48.f,By-18.f,Bx+48.f,By-5.f)||InBox(Qx,Qy,Bx-40.f,By-31.f,Bx+34.f,By-18.f))return .84f;
   if(InOval(Qx,Qy,Bx,By,56.f,4.f))return .2f;
   return -1.f;
  }
  return -1.f;
 }
 float Scene(const FString& Kind,uint32 Seed,float Qx,float Qy)
 {
  const float W=PhotoW,H=PhotoH,Hz=H*.56f;
  // The far mesas: flat on top, steep at the sides.
  const float Lump=Noise1(Qx/W*5.f,Seed)*.6f+Noise1(Qx/W*13.f,Seed+7u)*.4f;
  const float Top=Hz-H*(.03f+.13f*FMath::Clamp((Lump-.42f)*4.f,0.f,1.f));
  float Lum;
  if(Qy<Top)Lum=.80f+.12f*(Qy/Top);
  else if(Qy<Hz)Lum=.50f+.08f*Noise1(Qx*.2f,Seed+3u);
  else{const float D=(Qy-Hz)/(H-Hz);Lum=.70f-.30f*D+.06f*(Noise1(Qx*.15f+Qy*.3f,Seed+5u)-.5f);}
  // Scrub on the flat, larger nearer.
  if(Qy>=Hz)for(int32 K=0;K<46;++K)
  {
   const float Ty=Hz+FMath::Pow(Hash1(K,Seed+13u),1.6f)*(H-Hz),R=1.5f+6.f*(Ty-Hz)/(H-Hz);
   if(InOval(Qx,Qy,Hash1(K,Seed+11u)*W,Ty,R*1.7f,R)){Lum=FMath::Min(Lum,.28f);break;}
  }
  // A picture of the valley has a fence going away across it.
  if(Kind.IsEmpty())
  {
   for(int32 K=0;K<9;++K)
   {
    const float T=float(K)/8.f,Px=W*(.12f+.7f*T),Py=H*.97f-(H*.97f-Hz-5.f)*T,Tall=34.f*(1.f-.85f*T);
    if(OnSeg(Qx,Qy,Px,Py,Px,Py-Tall,3.f*(1.f-.7f*T)))return .14f;
   }
   const float T=FMath::Clamp((Qx-W*.12f)/(W*.7f),0.f,1.f),Py=H*.97f-(H*.97f-Hz-5.f)*T,Tall=34.f*(1.f-.85f*T);
   if(Qx>=W*.12f&&Qx<=W*.82f&&(FMath::Abs(Qy-(Py-Tall*.45f))<.7f||FMath::Abs(Qy-(Py-Tall*.85f))<.7f))return .3f;
   return FMath::Clamp(Lum,0.f,1.f);
  }
  const float Sub=Subject(Kind,Qx,Qy,W*.6f,Hz+(H-Hz)*.62f);
  return FMath::Clamp(Sub>=0.f?Sub:Lum,0.f,1.f);
 }
 UTexture2D* Halftone(const FString& Kind,uint32 Seed)
 {
  static TMap<FString,TStrongObjectPtr<UTexture2D>> Made;
  const uint32 Var=Seed%3u;
  const FString Key=FString::Printf(TEXT("%s/%u"),*Kind,Var);
  if(const TStrongObjectPtr<UTexture2D>* Got=Made.Find(Key))return Got->Get();
  if(!FApp::CanEverRender())return nullptr;
  constexpr float Cell=4.2f,Rt=.70710678f;
  const FColor Dot=Ink().ToFColor(true);
  TArray<FColor> Pixels;Pixels.SetNumZeroed(PhotoW*PhotoH);
  for(int32 Iy=0;Iy<PhotoH;++Iy)for(int32 Ix=0;Ix<PhotoW;++Ix)
  {
   const float Fx=Ix+.5f,Fy=Iy+.5f,U=(Fx+Fy)*Rt,V=(Fy-Fx)*Rt;
   const float Cu=(FMath::FloorToFloat(U/Cell)+.5f)*Cell,Cv=(FMath::FloorToFloat(V/Cell)+.5f)*Cell;
   const float Sx=FMath::Clamp((Cu-Cv)*Rt,0.f,PhotoW-1.f),Sy=FMath::Clamp((Cu+Cv)*Rt,0.f,PhotoH-1.f);
   const float Lum=FMath::Clamp((Scene(Kind,Var,Sx,Sy)-.5f)*1.25f+.55f,0.f,1.f);
   const float Rad=Cell*.72f*FMath::Sqrt(1.f-Lum),D=FMath::Sqrt(FMath::Square(U-Cu)+FMath::Square(V-Cv));
   Pixels[Iy*PhotoW+Ix]=FColor(Dot.R,Dot.G,Dot.B,uint8(FMath::RoundToInt(FMath::Clamp(Rad-D+.5f,0.f,1.f)*235.f)));
  }
  UTexture2D* T=MakeTex(PhotoW,PhotoH,Pixels,true);
  if(T)Made.Add(Key,TStrongObjectPtr<UTexture2D>(T));
  return T;
 }
 FString Caption(const FString& Lead)
 {
  if(Lead==TEXT("wagon"))return Titled(WagonTitle)+PhotoLine;
  if(const FS47Places::FDef* D=FS47Places::Def(Lead))return Titled(D->Title)+PhotoLine;
  return WildCaption;
 }
 TSharedRef<SWidget> Photo(const FString& Lead,uint32 Seed)
 {
  FString Kind;
  if(Lead==TEXT("wagon"))Kind=Lead;else if(const FS47Places::FDef* D=FS47Places::Def(Lead))Kind=D->Kind;
  UTexture2D* T=Halftone(Kind,Seed);
  const TSharedRef<SWidget> Pic=T?TSharedRef<SWidget>(SNew(SImage).Image(S47Game::Picture(T))):TSharedRef<SWidget>(SNew(SBorder).BorderImage(S47Game::Box(S47Game::C(0xa8a294))));
  return SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()
   [
    SNew(SBox).WidthOverride(SpanW(2)).HeightOverride(float(PhotoH))[SNew(SBorder).BorderImage(S47Game::Box(FLinearColor::Transparent,Ink(),.8f)).Padding(1.f)[Pic]]
   ]
   +SVerticalBox::Slot().AutoHeight().Padding(0,4,0,0)[Words(Caption(Lead),Serif(9.5f,EFace::Italic),SpanW(2),Ink())];
 }

 // The pencil ring round a story that points somewhere: faint while the pointer is on it (or the pad's focus), firm with a tick
 // once its place is on the map. Drawn by hand, so a little wavy and not quite closed; past the story's edges, into the gutters.
 class SRing:public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SRing):_Seed(0){}
   SLATE_ATTRIBUTE(bool,Hot)
   SLATE_ATTRIBUTE(bool,On)
   SLATE_ARGUMENT(uint32,Seed)
  SLATE_END_ARGS()
  void Construct(const FArguments& InArgs){Hot=InArgs._Hot;On=InArgs._On;Seed=InArgs._Seed;}
  virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(1.,1.);}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& Geo,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool)const override
  {
   const bool bOn=On.Get(),bHot=Hot.Get();
   if(!bOn&&!bHot)return Layer;
   const FVector2f Size=FVector2f(Geo.GetLocalSize());
   const float X0=-7.f,Y0=-6.f,X1=Size.X+7.f,Y1=Size.Y+6.f,Rad=FMath::Min(18.f,FMath::Min(X1-X0,Y1-Y0)*.3f);
   FRandomStream Stream(static_cast<int32>(Seed));
   TArray<FVector2f> Path;
   const auto Edge=[&Path,&Stream](FVector2f From,FVector2f To)
   {
    const int32 Steps=FMath::Max(1,int32(FVector2f::Distance(From,To)/44.f));
    for(int32 K=0;K<Steps;++K)Path.Add(From+(To-From)*(float(K)/Steps)+FVector2f(Stream.FRandRange(-1.3f,1.3f),Stream.FRandRange(-1.3f,1.3f)));
   };
   const auto Arc=[&Path,Rad](FVector2f Centre,float From){for(int32 K=0;K<6;++K){const float Ang=From+UE_HALF_PI*K/5.f;Path.Add(Centre+FVector2f(FMath::Cos(Ang),FMath::Sin(Ang))*Rad);}};
   Edge(FVector2f(X0+Rad,Y0),FVector2f(X1-Rad,Y0));Arc(FVector2f(X1-Rad,Y0+Rad),-UE_HALF_PI);
   Edge(FVector2f(X1,Y0+Rad),FVector2f(X1,Y1-Rad));Arc(FVector2f(X1-Rad,Y1-Rad),0.f);
   Edge(FVector2f(X1-Rad,Y1),FVector2f(X0+Rad,Y1));Arc(FVector2f(X0+Rad,Y1-Rad),UE_HALF_PI);
   Edge(FVector2f(X0,Y1-Rad),FVector2f(X0,Y0+Rad));Arc(FVector2f(X0+Rad,Y0+Rad),UE_PI);
   // The pencil runs on past where it started.
   for(int32 K=0;K<2&&K<Path.Num();++K)Path.Add(Path[K]+FVector2f(4.f+3.f*K,-2.f-K));
   FSlateDrawElement::MakeLines(Out,Layer,Geo.ToPaintGeometry(),Path,ESlateDrawEffect::None,Graphite(bOn?.85f:.45f),true,bOn?2.f:1.6f);
   if(bOn)FSlateDrawElement::MakeLines(Out,Layer,Geo.ToPaintGeometry(),TArray<FVector2f>{FVector2f(X1-30.f,Y0+12.f),FVector2f(X1-23.f,Y0+20.f),FVector2f(X1-9.f,Y0+2.f)},ESlateDrawEffect::None,Graphite(.85f),true,2.2f);
   return Layer+1;
  }
 private:
  TAttribute<bool> Hot,On;
  uint32 Seed=0;
 };
 const FButtonStyle* Bare()
 {
  static const FButtonStyle Style=[]
  {
   FButtonStyle Out;FSlateBrush None;None.DrawAs=ESlateBrushDrawType::NoDrawType;
   Out.SetNormal(None).SetHovered(None).SetPressed(None).SetDisabled(None);
   Out.SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
   return Out;
  }();
  return &Style;
 }
 // D45-24 step 1: an article with a lead the map can show is a button: hover rings it in pencil and asks in the margin, a click
 // (or A on the pad) asks the reader to pencil it in (Hooks.Mark). Whether it is on the map is the map's own (Hooks.Marked).
 TSharedRef<SWidget> Pencilled(const TSharedRef<SWidget>& Content,const FString& Lead,const FString& Head,const FLeadHooks& Hooks)
 {
  if(!Hooks.Mark||LeadTitle(Lead).IsEmpty())return Content;
  const TSharedRef<TWeakPtr<SWidget>> Holder=MakeShared<TWeakPtr<SWidget>>();
  const TFunction<bool(const FString&)> IsMarked=Hooks.Marked;
  const TFunction<void(const FString&,const FString&)> DoMark=Hooks.Mark;
  const TAttribute<bool> Hot=TAttribute<bool>::CreateLambda([Holder]{const TSharedPtr<SWidget> Btn=Holder->Pin();return Btn.IsValid()&&(Btn->IsHovered()||Btn->HasAnyUserFocus().IsSet());});
  const TAttribute<bool> On=TAttribute<bool>::CreateLambda([IsMarked,Lead]{return IsMarked&&IsMarked(Lead);});
  const TSharedRef<SWidget> Note=SNew(STextBlock)
   .Text_Lambda([On]{return FText::FromString(On.Get()?FString(NoteDone):FString(NoteAsk));})
   .Font(S47Style::Hand(22.f*Cssp)).ColorAndOpacity(Graphite(.92f))
   .Visibility_Lambda([On,Hot]{return On.Get()||Hot.Get()?EVisibility::HitTestInvisible:EVisibility::Hidden;});
  TSharedPtr<SButton> Button;
  SAssignNew(Button,SButton).ButtonStyle(Bare()).ContentPadding(FMargin(0)).Cursor(EMouseCursor::Hand).IsFocusable(true)
   .OnClicked_Lambda([DoMark,Lead,Head]{if(DoMark)DoMark(Lead,Head);return FReply::Handled();})
   [
    SNew(SOverlay)
    +SOverlay::Slot()[Content]
    +SOverlay::Slot()[SNew(SRing).Hot(Hot).On(On).Seed(GetTypeHash(Lead)).Visibility(EVisibility::HitTestInvisible)]
    +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f,0.f,6.f,-20.f)[S47Game::Turned(Note,-3.f)]
   ];
  *Holder=Button;
  return Button.ToSharedRef();
 }
 // An ear beside the nameplate: a word in caps and a line under it, in a thin box.
 TSharedRef<SWidget> Ear(const FString& Title,const FString& Line)
 {
  const float Wrap=SpanW(1)-16.f;
  return SNew(SBox).VAlign(VAlign_Center)
  [
   SNew(SBorder).BorderImage(S47Game::Box(FLinearColor::Transparent,Ink(),1.f)).Padding(FMargin(7,5))
   [
    SNew(SVerticalBox)
    +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Words(Title,Cond(12.f,80),Wrap,Ink(),ETextJustify::Center)]
    +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0,2,0,0)[Words(Line,Serif(9.5f,EFace::Italic),Wrap,Ink(),ETextJustify::Center)]
   ]
  ];
 }
 // A letter or a classified: its head in bold caps, the words, and who signed it (after " / ") set right in italics.
 TSharedRef<SWidget> NoteBlock(const FNote& Nt,float Wrap)
 {
  FString Said=Nt.Body,Signed;
  const int32 Cut=Nt.Body.Find(TEXT(" / "),ESearchCase::CaseSensitive,ESearchDir::FromEnd);
  if(Cut!=INDEX_NONE){Said=Nt.Body.Left(Cut);Signed=Nt.Body.Mid(Cut+3);}
  const TSharedRef<SVerticalBox> Stack=SNew(SVerticalBox);
  Stack->AddSlot().AutoHeight().Padding(0,0,0,2)[Words(Nt.Head,Serif(10.5f,EFace::Bold,40),Wrap,Ink())];
  Stack->AddSlot().AutoHeight()[Para(Esc(Said),12.f,Wrap,Ink())];
  if(!Signed.IsEmpty())Stack->AddSlot().AutoHeight().HAlign(HAlign_Right).Padding(0,2,0,0)[Words(TEXT("— ")+Signed,Serif(10.5f,EFace::Italic),Wrap,Faded(),ETextJustify::Right)];
  return Stack;
 }
 // "Tuesday night. Cattle..." with its day run in bold.
 FString RunIn(const FString& Entry)
 {
  const int32 Stop=Entry.Find(TEXT(". "),ESearchCase::CaseSensitive);
  return Stop==INDEX_NONE||Stop>24?Esc(Entry):Bold(Entry.Left(Stop+1))+TEXT(" ")+Esc(Entry.Mid(Stop+2));
 }
 TSharedRef<SWidget> AdBlock(const FAd& Ad,float Wide)
 {
  const bool bCoupon=!Ad.Coupon.IsEmpty();
  const float Wrap=Wide-(bCoupon?36.f:24.f);
  const TSharedRef<SVerticalBox> In=SNew(SVerticalBox);
  In->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0,0,0,4)[Words(Ad.Name,Cond(17.f,20),Wrap,Ink(),ETextJustify::Center)];
  In->AddSlot().AutoHeight().HAlign(HAlign_Center)[Para(Esc(Ad.Body),12.f,Wrap,Ink(),ETextJustify::Center)];
  if(!bCoupon)return SNew(SBorder).BorderImage(S47Game::Box(Paper(),Ink(),1.f)).Padding(10)[In];
  In->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0,8,0,0)[Words(CutLine,Serif(8.5f,EFace::Bold,90),Wrap,Faded(),ETextJustify::Center)];
  return SNew(SOverlay)
   +SOverlay::Slot()[SNew(SBorder).BorderImage(S47Game::Box(Paper())).Padding(16)[In]]
   +SOverlay::Slot()[SNew(SS47GameDash).Color(Ink()).Inset(2.f).Visibility(EVisibility::HitTestInvisible)];
 }

 // The front: the nameplate between its ears, the folio line, the lead story with its photo over four columns, the county's
 // briefs in a column of their own, the second story set wide over two, and the index of what is inside.
 void FrontSheet(const TSharedRef<SVerticalBox>& Page,const FIssue& Issue,const FLeadHooks& Hooks)
 {
  FString Today=Issue.Week.IsEmpty()?FString():Issue.Week[0];Today.RemoveFromStart(TEXT("TODAY. "));
  const TSharedRef<SVerticalBox> Plate=SNew(SVerticalBox);
  Plate->AddSlot().AutoHeight().HAlign(HAlign_Center)[Words(Nameplate,Serif(66.f,EFace::Bold,-10),SpanW(4),Ink(),ETextJustify::Center)];
  Plate->AddSlot().AutoHeight().HAlign(HAlign_Center)[Words(Motto,Serif(11.f,EFace::Italic),SpanW(4),Faded(),ETextJustify::Center)];
  const TSharedRef<SHorizontalBox> Top=SNew(SHorizontalBox);
  Cell(Top,1,Ear(EarToday,Today),false);Cell(Top,4,Plate,false);Cell(Top,1,Ear(EarInside,EarInsideText),false);
  Page->AddSlot().AutoHeight()[Top];
  Page->AddSlot().AutoHeight().Padding(0,8,0,0)[ThickThin()];
  const FSlateFontInfo FolioFont=Serif(9.5f,EFace::Bold,70);
  Page->AddSlot().AutoHeight().Padding(0,4)
  [
   SNew(SHorizontalBox)
   +SHorizontalBox::Slot().FillWidth(1.f)[Words(FString::Printf(TEXT("VOL. %d, NO. %d"),Volume,Issue.Date.GetDayOfYear()/7+1),FolioFont,TextW/4,Ink())]
   +SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center)[Words(Town,FolioFont,TextW/4,Ink(),ETextJustify::Center)]
   +SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center)[Words(DateLine(Issue.Date),FolioFont,TextW/4,Ink(),ETextJustify::Center)]
   +SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right)[Words(Price,FolioFont,TextW/4,Ink(),ETextJustify::Right)]
  ];
  Page->AddSlot().AutoHeight().Padding(0,0,0,10)[Rule(1.f)];
  const TSharedRef<SHorizontalBox> Stories=SNew(SHorizontalBox);
  // The lead story: its head over three columns, the photo under it over two, the words in the third.
  {
   const FStory Lead=Issue.Front.IsValidIndex(0)?Issue.Front[0]:FStory();
   const TSharedRef<SHorizontalBox> Under=SNew(SHorizontalBox);
   Cell(Under,2,Photo(Lead.Lead,uint32(FMath::Max(1,Issue.No))),false);
   const TSharedRef<SVerticalBox> Words1=SNew(SVerticalBox);
   Words1->AddSlot().AutoHeight().Padding(0,0,0,4)[Words(ByStaff,Serif(8.5f,EFace::Bold,80),ColW,Faded())];
   Words1->AddSlot().AutoHeight()[Para(Bold(Dateline)+Esc(Lead.Body),13.f,ColW,Ink())];
   Cell(Under,1,Words1,false);
   const TSharedRef<SVerticalBox> Block=SNew(SVerticalBox);
   Block->AddSlot().AutoHeight().Padding(0,0,0,8)[Words(Lead.Head,Cond(36.f),SpanW(3),Ink())];
   Block->AddSlot().AutoHeight()[Under];
   Cell(Stories,3,Pencilled(Block,Lead.Lead,Lead.Head,Hooks),true);
  }
  // Around the county: the briefs down one column.
  {
   const TSharedRef<SVerticalBox> Briefs=SNew(SVerticalBox);
   Briefs->AddSlot().AutoHeight()[Section(SectionHeads[0],ColW)];
   for(const FString& St:Issue.Locals)Briefs->AddSlot().AutoHeight().Padding(0,0,0,9)[Para(Esc(St),12.f,ColW,Ink())];
   Cell(Stories,1,Briefs,true);
  }
  // The second story set wide, and under it the index.
  {
   const TSharedRef<SVerticalBox> Side=SNew(SVerticalBox);
   if(Issue.Front.IsValidIndex(1))
   {
    const FStory& St=Issue.Front[1];
    const TSharedRef<SVerticalBox> Block=SNew(SVerticalBox);
    Block->AddSlot().AutoHeight().Padding(0,0,0,4)[Words(St.Head,Cond(24.f),SpanW(2),Ink())];
    Block->AddSlot().AutoHeight().Padding(0,0,0,4)[Words(ByReport,Serif(8.5f,EFace::Bold,80),SpanW(2),Faded())];
    Block->AddSlot().AutoHeight()[Para(Bold(Dateline)+Esc(St.Body),12.5f,SpanW(2),Ink())];
    Side->AddSlot().AutoHeight().Padding(0,0,0,14)[Pencilled(Block,St.Lead,St.Head,Hooks)];
   }
   const bool bCoupon=Issue.Ads.ContainsByPredicate([](const FAd& Ad){return !Ad.Coupon.IsEmpty();});
   const TSharedRef<SVerticalBox> Index=SNew(SVerticalBox);
   Index->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0,0,0,4)[Words(InsideHead,Cond(14.f,120),SpanW(2)-20.f,Ink(),ETextJustify::Center)];
   const int32 Items=int32(UE_ARRAY_COUNT(InsideItems));
   for(int32 K=0;K<Items;++K)
   {
    if(K==Items-1&&!bCoupon)break;
    Index->AddSlot().AutoHeight().Padding(0,1)
    [
     SNew(SHorizontalBox)
     +SHorizontalBox::Slot().FillWidth(1.f)[Words(InsideItems[K],Serif(11.f),SpanW(2)-60.f,Ink())]
     +SHorizontalBox::Slot().AutoWidth()[Words(TEXT("2"),Serif(11.f,EFace::Bold),30.f,Ink(),ETextJustify::Right)]
    ];
   }
   Side->AddSlot().AutoHeight()[SNew(SBorder).BorderImage(S47Game::Box(FLinearColor::Transparent,Ink(),1.f)).Padding(FMargin(10,6))[Index]];
   Cell(Stories,2,Side,true);
  }
  Page->AddSlot().AutoHeight()[Stories];
  Page->AddSlot().AutoHeight().Padding(0,12,0,0)[ThickThin()];
 }
 // Inside: the folio, the sheriff's log, the letters and classifieds in two columns, the week's weather, the almanac and the
 // picture show; the ads along the foot, the coupon's in a dashed box.
 void InsideSheet(const TSharedRef<SVerticalBox>& Page,const FIssue& Issue,const FLeadHooks& Hooks)
 {
  const FSlateFontInfo FolioFont=Serif(9.5f,EFace::Bold,70);
  Page->AddSlot().AutoHeight().Padding(0,0,0,3)
  [
   SNew(SHorizontalBox)
   +SHorizontalBox::Slot().FillWidth(1.f)[Words(TEXT("2"),Serif(11.f,EFace::Bold),TextW/3,Ink())]
   +SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center)[Words(Masthead,FolioFont,TextW/3,Ink(),ETextJustify::Center)]
   +SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right)[Words(DateLine(Issue.Date),FolioFont,TextW/3,Ink(),ETextJustify::Right)]
  ];
  Page->AddSlot().AutoHeight().Padding(0,0,0,10)[Rule(1.f)];
  const TSharedRef<SHorizontalBox> Row=SNew(SHorizontalBox);
  {
   const TSharedRef<SVerticalBox> Log=SNew(SVerticalBox);
   Log->AddSlot().AutoHeight()[Section(SectionHeads[1],ColW)];
   for(const FString& St:Issue.Sheriff)Log->AddSlot().AutoHeight().Padding(0,0,0,9)[Para(RunIn(St),12.f,ColW,Ink())];
   Cell(Row,1,Log,true);
  }
  {
   const TSharedRef<SHorizontalBox> Two=SNew(SHorizontalBox);
   const int32 Left=(Issue.Letters.Num()+1)/2;
   for(int32 Half=0;Half<2;++Half)
   {
    const TSharedRef<SVerticalBox> Stack=SNew(SVerticalBox);
    for(int32 K=Half==0?0:Left;K<(Half==0?Left:Issue.Letters.Num());++K)
    {
     const FNote& Nt=Issue.Letters[K];
     Stack->AddSlot().AutoHeight().Padding(0,0,0,12)[Pencilled(NoteBlock(Nt,ColW),Nt.Lead,Nt.Head,Hooks)];
    }
    Cell(Two,1,Stack,false);
   }
   const TSharedRef<SVerticalBox> Notes=SNew(SVerticalBox);
   Notes->AddSlot().AutoHeight()[Section(SectionHeads[2],SpanW(2))];
   Notes->AddSlot().AutoHeight()[Two];
   Cell(Row,2,Notes,true);
  }
  {
   const float Wide=SpanW(2);
   const TSharedRef<SVerticalBox> Sky=SNew(SVerticalBox);
   Sky->AddSlot().AutoHeight()[Section(SectionHeads[3],Wide)];
   for(const FString& St:Issue.Week)
   {
    FString Day=St,Said;
    if(St.StartsWith(TEXT("TODAY. "))){Day=TEXT("TODAY");Said=St.Mid(7);}
    else if(!St.Split(TEXT("  "),&Day,&Said)){Day=FString();Said=St;}
    Sky->AddSlot().AutoHeight().Padding(0,2)
    [
     SNew(SHorizontalBox)
     +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(64.f)[Words(Day,Serif(9.5f,EFace::Bold,60),62.f,Ink())]]
     +SHorizontalBox::Slot().FillWidth(1.f)[Para(Esc(Said),12.f,Wide-66.f,Ink())]
    ];
    Sky->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(1.f)[SNew(SBorder).BorderImage(S47Game::Box(Faded())).Padding(0)]];
   }
   Sky->AddSlot().AutoHeight().Padding(0,6,0,12)[Para(TEXT("<i>")+Esc(Issue.LastWeek)+TEXT("</>"),11.f,Wide,Faded())];
   Sky->AddSlot().AutoHeight()[Section(SectionHeads[4],Wide)];
   Sky->AddSlot().AutoHeight()[Para(Esc(Issue.Almanac),12.f,Wide,Ink())];
   Cell(Row,2,Sky,true);
  }
  {
   const TSharedRef<SVerticalBox> Show=SNew(SVerticalBox);
   Show->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0,0,0,6)[Words(SectionHeads[5],Cond(18.f,60),ColW-24.f,Ink(),ETextJustify::Center)];
   Show->AddSlot().AutoHeight().HAlign(HAlign_Center)[Para(Esc(Issue.Showing),12.f,ColW-24.f,Ink(),ETextJustify::Center)];
   Cell(Row,1,SNew(SBox).VAlign(VAlign_Top)[SNew(SBorder).BorderImage(S47Game::Box(Paper(),Ink(),2.f)).Padding(10)[Show]],true);
  }
  Page->AddSlot().AutoHeight()[Row];
  Page->AddSlot().AutoHeight().Padding(0,12,0,10)[ThickThin()];
  if(!Issue.Ads.IsEmpty())
  {
   const TSharedRef<SHorizontalBox> Foot=SNew(SHorizontalBox);
   const int32 Num=FMath::Min(Issue.Ads.Num(),6),Each=6/Num;
   for(int32 K=0;K<Num;++K)
   {
    const int32 Span=K==Num-1?6-Each*(Num-1):Each;
    Cell(Foot,Span,SNew(SBox).VAlign(VAlign_Top)[AdBlock(Issue.Ads[K],SpanW(Span))],false);
   }
   Page->AddSlot().AutoHeight()[Foot];
  }
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

TArray<S47Ledger::FStory> S47Ledger::Front(int32 Seed,int32 No,const FDateTime& Date,const TArray<FFact>& Facts,const TMap<FString,int32>& Used,const FString& UnknownId,const FString& UnknownTitle)
{
 namespace L=S47LedgerLocal;namespace X=S47LedgerText;
 // Most newsworthy first; of two of a kind, the later night.
 TArray<FFact> Sorted=Facts;
 Sorted.StableSort([](const FFact& A,const FFact& B){return A.Topic!=B.Topic?uint8(A.Topic)<uint8(B.Topic):A.Day>B.Day;});
 // One story to a topic, the man and his truck once, no place twice, and never a place the station did not register.
 TArray<FStory> Out;TSet<uint8> Told;TSet<FString> Where;
 for(const FFact& Fa:Sorted)
 {
  if(Out.Num()>=2)break;
  if(Fa.Topic==ETopic::Helped||Fa.Topic==ETopic::Count||Fa.Topic==ETopic::Rumor||Told.Contains(uint8(Fa.Topic)))continue;
  if((Fa.Topic==ETopic::Missing&&Told.Contains(uint8(ETopic::Called)))||(Fa.Topic==ETopic::Called&&Told.Contains(uint8(ETopic::Missing))))continue;
  if(!Fa.Place.IsEmpty()&&Where.Contains(Fa.Place))continue;
  TArray<const X::FFrontDef*> Variants;
  for(const X::FFrontDef& D:X::Fronts())if(D.Topic==Fa.Topic&&(!Fa.Place.IsEmpty()||(!L::NeedsPlace(D.Head)&&!L::NeedsPlace(D.Body))))Variants.Add(&D);
  const X::FFrontDef* D=L::Choose(Variants,Used,Seed,uint32(Fa.Topic)*7u+uint32(No));
  if(!D)continue;
  Out.Add(L::Story(*D,Fa.Place,Fa.Night,Fa.PlaceId,Fa.Id));
  Told.Add(uint8(Fa.Topic));if(!Fa.Place.IsEmpty())Where.Add(Fa.Place);
 }
 // The county's own news where the week left room: the search in the first paper; after that the station's review first, then the
 // rest in this game's order, each in its window, one to a thread, and none printed before while another is left.
 const auto& Quiet=X::Quiet();
 const TSet<FString> PotThreads=L::PlanIssue(Seed,No,Date).Threads;
 const auto OnPage=[&](const X::FFrontDef& D)
 {
  if(!D.Thread)return false;
  if(PotThreads.Contains(D.Thread))return true;
  for(const FStory& St:Out)if(const X::FFrontDef* Q=L::QuietOf(St.Key))if(Q->Thread&&FCString::Strcmp(Q->Thread,D.Thread)==0)return true;
  return false;
 };
 const auto MoreQuiet=[&]
 {
  TArray<int32> Rest;Rest.Add(1);for(const int32 I:L::Order(Quiet.Num()-2,Seed,77))Rest.Add(I+2);
  const X::FFrontDef* Best=nullptr;int32 When=MAX_int32;
  for(const int32 I:Rest)
  {
   const X::FFrontDef& D=Quiet[I];
   if(!X::InWindow(D.First,D.Last,Date)||OnPage(D)||Out.ContainsByPredicate([&](const FStory& St){return St.Key==D.Head;}))continue;
   const int32 At=Used.Contains(D.Head)?Used.FindRef(D.Head):-1;
   if(At<When){When=At;Best=&D;}
   if(At<0)break;
  }
  if(Best)Out.Add(L::Story(*Best,FString(),FString(),FString(),FString()));
  return Best!=nullptr;
 };
 if(No<=1)
 {
  // The first paper the new operator sees: the search, whatever else the week brought.
  if(Out.Num()>1)Out.SetNum(1);
  Out.Add(L::Story(Quiet[0],FString(),FString(),TEXT("wagon"),FString()));
 }
 while(Out.Num()<2&&MoreQuiet()){}
 // A week that points at nothing gets the talk about somewhere the player has not heard of, in place of the county's own news.
 const bool bLead=Out.ContainsByPredicate([](const FStory& St){return !St.Lead.IsEmpty();});
 if(!bLead&&!UnknownId.IsEmpty()&&!Out.IsEmpty()&&Out.Last().FactId.IsEmpty())
 {
  TArray<const X::FFrontDef*> Talk;for(const X::FFrontDef& D:X::Fronts())if(D.Topic==ETopic::Rumor)Talk.Add(&D);
  if(const X::FFrontDef* D=L::Choose(Talk,Used,Seed,uint32(No)*13u))Out.Last()=L::Story(*D,UnknownTitle,FString(),UnknownId,FString());
 }
 return Out;
}

TArray<S47Ledger::FLogLine> S47Ledger::LogLines(const TArray<FFact>& Facts,const TArray<FStory>& FrontPage,int32 OnDay,int32 Most)
{
 namespace L=S47LedgerLocal;namespace X=S47LedgerText;
 TArray<FFact> Sorted=Facts;
 Sorted.StableSort([](const FFact& A,const FFact& B){return A.Topic!=B.Topic?uint8(A.Topic)<uint8(B.Topic):A.Day>B.Day;});
 TArray<FLogLine> Out;TSet<uint8> Told;
 for(const FStory& St:FrontPage)if(!St.FactId.IsEmpty())if(const FFact* Fa=Facts.FindByPredicate([&](const FFact& Q){return Q.Id==St.FactId;}))Told.Add(uint8(Fa->Topic));
 for(const FFact& Fa:Sorted)
 {
  if(Out.Num()>=Most)break;
  if(Told.Contains(uint8(Fa.Topic))||FrontPage.ContainsByPredicate([&](const FStory& St){return St.FactId==Fa.Id;}))continue;
  const X::FLogDef* D=X::Logs().FindByPredicate([&](const X::FLogDef& Q){return Q.Topic==Fa.Topic;});
  if(!D||(Fa.Place.IsEmpty()&&L::NeedsPlace(D->Line)))continue;
  FLogLine Line;Line.Ago=FMath::Max(0,OnDay-Fa.Day);Line.Text=L::Titled(Fa.Night)+TEXT(". ")+L::Fill(D->Line,Fa.Place,Fa.Night);
  Out.Add(Line);Told.Add(uint8(Fa.Topic));
 }
 return Out;
}

void S47Ledger::Fill(FIssue& Issue,int32 Seed,bool bHelped)
{
 namespace L=S47LedgerLocal;namespace X=S47LedgerText;
 const int32 No=FMath::Max(1,Issue.No);
 Issue.Locals.Reset();Issue.Sheriff.Reset();Issue.Ads.Reset();Issue.Letters.Reset();Issue.Week.Reset();
 // The briefs, plain ads and letters, as the pots give them (the front page keeps clear of their threads).
 const L::FPlanned Plan=L::PlanIssue(Seed,No,Issue.Date);
 for(const int32 I:Plan.Locals)Issue.Locals.Add(X::Locals()[I].Text);
 // The sheriff's log runs back over the week, a day to an entry: first what the station's nights left that the front had no room
 // for (Issue.Log, kept with the paper), then the usual business.
 {
  struct FEntry{int32 Ago;FString Text;};
  TArray<FEntry> Entries;
  for(const FLogLine& Line:Issue.Log)if(Entries.Num()<L::MostLogged)Entries.Add({Line.Ago,Line.Text});
  const TArray<int32> Log=L::Take(L::Order(X::Sheriff().Num(),Seed,102),No,L::SheriffPer);
  for(int32 K=0;K<Log.Num()&&Entries.Num()<L::SheriffPer;++K)
  {
   const int32 Ago=6-K*2-(K==2?1:0);
   const FDateTime On=Issue.Date-FTimespan::FromDays(Ago);
   Entries.Add({Ago,FString::Printf(TEXT("%s. %s"),*L::Titled(FString(L::WeekDays[int32(On.GetDayOfWeek())]).ToLower()),X::Sheriff()[Log[K]])});
  }
  Entries.StableSort([](const FEntry& A,const FEntry& B){return A.Ago>B.Ago;});
  for(const FEntry& E:Entries)Issue.Sheriff.Add(E.Text);
 }
 // Ads: one with a coupon (the catalog's in the first paper, the week the catalog came), two without.
 TArray<int32> WithCoupon;int32 CatalogAd=-1;
 for(int32 I=0;I<X::Ads().Num();++I)
 {
  if(!X::Ads()[I].Coupon)continue;
  if(FCString::Strcmp(X::Ads()[I].Coupon,TEXT("post0"))==0)CatalogAd=I;
  else WithCoupon.Add(I);
 }
 TArray<int32> CouponOrder;for(const int32 I:L::Order(WithCoupon.Num(),Seed,103))CouponOrder.Add(WithCoupon[I]);
 const int32 Pick=No==1&&CatalogAd>=0?CatalogAd:CouponOrder.IsEmpty()?-1:L::Take(CouponOrder,No-1,1)[0];
 if(Pick>=0)Issue.Ads.Add({X::Ads()[Pick].Name,X::Ads()[Pick].Body,X::Ads()[Pick].Coupon});
 for(const int32 I:Plan.PlainAds)Issue.Ads.Add({X::Ads()[I].Name,X::Ads()[I].Body,FString()});
 {
  const TArray<int32> Films=L::Take(L::Order(X::Films().Num(),Seed,105),No,L::FilmsPer);
  Issue.Showing=FString::Printf(TEXT("%s at 7. %s at 9. Saturday matinee at 2. Adults $2.50, children $1."),X::Films()[Films[0]],X::Films()[Films[1]]);
 }
 // Letters: the trucker's thanks when there is cause for it, one that points somewhere, one that does not.
 if(bHelped)Issue.Letters.Add({X::Thanks().Head,X::Thanks().Body,FString()});
 for(const TArray<int32>* Group:{&Plan.LeadLetters,&Plan.OtherLetters})for(const int32 I:*Group)
 {
  const X::FLetterDef& D=X::Letters()[I];
  Issue.Letters.Add({D.Head,D.Body,D.Lead?FString(D.Lead):FString()});
 }
 // The weather: what the forecaster says for the five days from the issue, and what the week before was in Caliche: the sky's own.
 for(int32 K=0;K<5;++K)
 {
  const S47Weather::FDay Said=S47Weather::Forecast(Seed,Issue.Day,Issue.Day+K);
  const FDateTime On=Issue.Date+FTimespan::FromDays(K);
  Issue.Week.Add(K==0?TEXT("TODAY. ")+S47Weather::Words(Said):FString::Printf(TEXT("%s  %s"),L::ShortDays[int32(On.GetDayOfWeek())],*S47Weather::Words(Said,true)));
 }
 int32 High=-100,Low=200;
 for(int32 D=FMath::Max(1,Issue.Day-7);D<Issue.Day;++D){const S47Weather::FDay Word=S47Weather::Day(Seed,D);High=FMath::Max(High,Word.HighF);Low=FMath::Min(Low,Word.LowF);}
 Issue.LastWeek=High<-50?FString(TEXT("Caliche has no week of weather to report yet.")):S47Weather::LastWeek(Seed,FMath::Max(1,Issue.Day-7),Issue.Day);
 Issue.Almanac=S47Weather::Almanac(Issue.Date);
}

FString S47Ledger::LeadTitle(const FString& Lead)
{
 // D45-24-20261110: what the map can show (FS47Places::Hear and FS47Predecessor::HearOf take nothing else).
 if(Lead.IsEmpty())return FString();
 if(Lead==TEXT("wagon"))return S47LedgerLocal::WagonTitle;
 const FS47Places::FDef* D=FS47Places::Def(Lead);
 return D?FString(D->Title):FString();
}
TArray<S47Ledger::FLeadRef> S47Ledger::LeadsOf(const FIssue& Issue)
{
 TArray<FLeadRef> Out;
 const auto Add=[&Out](const FString& Lead,const FString& Head,int32 Sheet)
 {
  if(LeadTitle(Lead).IsEmpty()||Out.ContainsByPredicate([&](const FLeadRef& Q){return Q.Lead==Lead&&Q.Sheet==Sheet;}))return;
  FLeadRef Ref;Ref.Lead=Lead;Ref.Head=Head;Ref.Sheet=Sheet;Out.Add(Ref);
 };
 for(const FStory& St:Issue.Front)Add(St.Lead,St.Head,0);
 for(const FNote& Nt:Issue.Letters)Add(Nt.Lead,Nt.Head,1);
 return Out;
}

TSharedRef<SWidget> S47Ledger::Page(const FIssue& Issue,int32 Sheet,const FLeadHooks& Hooks)
{
 // D45-24-20261110: two sheets with the look of a real weekly (S47LedgerLocal::FrontSheet, InsideSheet), on newsprint with its
 // grain and the fold drawn over everything and letting every click through to the stories under it.
 namespace L=S47LedgerLocal;
 const TSharedRef<SVerticalBox> Content=SNew(SVerticalBox);
 if(Sheet<=0)L::FrontSheet(Content,Issue,Hooks);else L::InsideSheet(Content,Issue,Hooks);
 return SNew(SBorder).BorderImage(S47Game::Box(L::Paper(),S47Game::C(0x8a826f),1.f)).Padding(0)
 [
  SNew(SOverlay)
  +SOverlay::Slot().Padding(FMargin(18,16))[SNew(SBox).WidthOverride(L::TextW)[Content]]
  +SOverlay::Slot()[SNew(L::SNewsprint).Visibility(EVisibility::HitTestInvisible)]
 ];
}

FString S47Ledger::PlainText(const FIssue& Issue)
{
 namespace L=S47LedgerLocal;
 FString Out=FString::Printf(TEXT("%s\n%s\n\n"),L::Masthead,*DateLine(Issue.Date));
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
  TEXT("kinski"),TEXT("alien"),TEXT("ufo"),TEXT("saucer"),TEXT("abduct"),TEXT("taken"),TEXT("roswell"),TEXT("photograph of"),TEXT("the event"),TEXT("old road"),TEXT("nineteen forty"),
  TEXT("chevy"),TEXT("chevrolet"),TEXT("ford"),TEXT("pontiac"),TEXT("dodge"),TEXT("toyota"),TEXT("datsun"),TEXT("winnebago"),TEXT("cadillac"),TEXT("pepsi"),TEXT("coke"),TEXT("coca"),TEXT("coors"),
  TEXT("budweiser"),TEXT("levi"),TEXT("stetson"),TEXT("john deere"),TEXT("polaroid"),TEXT("kodak"),TEXT("zenith"),TEXT("motorola"),TEXT("radio shack"),TEXT("sears"),TEXT("kmart"),TEXT("k mart"),
  TEXT("walmart"),TEXT("texaco"),TEXT("exxon"),TEXT("conoco"),TEXT("greyhound"),TEXT("jeep"),TEXT("holiday inn"),TEXT("mcdonald"),TEXT("dairy queen"),TEXT("u.s. mail"),TEXT("usps"),
  TEXT("tyre"),TEXT("grey"),TEXT("programme"),TEXT("colour"),TEXT("neighbour"),TEXT("favour"),TEXT("harbour"),TEXT("centre"),TEXT("metre"),TEXT("kerb"),TEXT("cheque"),TEXT("aluminium"),
  TEXT("lorry"),TEXT("petrol"),TEXT("fortnight"),TEXT("whilst"),TEXT("windscreen"),TEXT("jewellery"),TEXT("realise"),TEXT("organise"),TEXT("recognise"),
  TEXT("initialled"),TEXT("labelled"),TEXT("travelled"),TEXT("travelling"),TEXT("by post"),TEXT("postcode"),TEXT("postman"),TEXT("car park"),TEXT("motorway"),
  TEXT("caliche county"),
  TEXT("reyes, he"),TEXT("reyes, she"),TEXT("mr. reyes"),TEXT("ms. reyes"),TEXT("mrs. reyes"),TEXT("reyes's wife"),TEXT("reyes's husband")};
 static const TCHAR* const Pieces[]={TEXT("47"),TEXT("8.15"),TEXT("u.f.o"),TEXT("1947")};
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
 for(const X::FItemDef& D:X::Locals())Out.Add(D.Text);
 for(const TCHAR* St:X::Sheriff())Out.Add(St);
 for(const X::FAdDef& A:X::Ads()){Out.Add(A.Name);Out.Add(A.Body);}
 for(const X::FLetterDef& Nt:X::Letters()){Out.Add(Nt.Head);Out.Add(Nt.Body);}
 Out.Add(X::Thanks().Head);Out.Add(X::Thanks().Body);
 for(const TCHAR* St:X::Films())Out.Add(St);
 for(const X::FFrontDef& D:X::Fronts()){Out.Add(D.Head);Out.Add(D.Body);}
 for(const X::FFrontDef& D:X::Quiet()){Out.Add(D.Head);Out.Add(D.Body);}
 for(const X::FLogDef& D:X::Logs())Out.Add(D.Line);
 for(const FCoupon& Cp:Coupons())Out.Add(Cp.Line);
 Out.Add(S47LedgerLocal::Masthead);Out.Add(S47Weather::Almanac());for(int32 Dd=0;Dd<30;++Dd)Out.Add(S47Weather::Almanac(FDateTime(1986,4,28)+FTimespan::FromDays(Dd)));
 {
  // D45-24-20261110: what the new page prints by itself, the captions and the pencil's words in the margin.
  namespace L=S47LedgerLocal;
  for(const TCHAR* St:{L::Nameplate,L::Motto,L::Town,L::Price,L::Dateline,L::ByStaff,L::ByReport,L::EarToday,L::EarInside,L::EarInsideText,L::InsideHead,L::CutLine,L::WagonTitle,L::WildCaption,L::NoteAsk,L::NoteDone})Out.Add(St);
  for(const TCHAR* St:L::InsideItems)Out.Add(St);
  for(const TCHAR* St:L::SectionHeads)Out.Add(St);
  Out.Add(L::Caption(TEXT("wagon")));Out.Add(L::Caption(FString()));
  for(const TCHAR* Id:{TEXT("windmill"),TEXT("tank"),TEXT("mine"),TEXT("cow"),TEXT("shack"),TEXT("cairn")})Out.Add(L::Caption(Id));
 }
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
  Check(bWords,bWords?FString(TEXT("nothing in the pots says a word from Story's far side, a brand or a British word")):TEXT("a word the paper must not print: ")+Bad);
  FString Probe;
  Check(!Clean(TEXT("She was taken."),Probe)&&Clean(TEXT("He was mistaken."),Probe)&&!Clean(TEXT("Lot 47."),Probe)&&Clean(TEXT("We can afford it."),Probe)&&!Clean(TEXT("All initialled R."),Probe)&&!Clean(TEXT("THE CALICHE COUNTY LEDGER"),Probe),
   TEXT("the word check takes whole words, every number with the forbidden digits, British spellings and the county that is not one"));
 }
 bool bCoupons=true;for(const X::FAdDef& A:X::Ads())if(A.Coupon)bCoupons&=Coupon(A.Coupon)!=nullptr;
 Check(bCoupons,TEXT("every coupon an ad carries exists"));
 bool bLeads=true;int32 LeadCount=0;for(const X::FLetterDef& Nt:X::Letters())if(Nt.Lead){++LeadCount;bLeads&=FS47Places::Def(Nt.Lead)!=nullptr;}
 Check(bLeads&&LeadCount>=5,TEXT("letters that point somewhere point at real places on the land"));
 {
  // Each kind of night has a story, the ones the nights give most three of them, each needing a place it is given.
  bool bTopics=true;
  for(uint8 Tp=0;Tp<uint8(ETopic::Helped);++Tp)
  {
   int32 Count=0;for(const X::FFrontDef& D:X::Fronts())Count+=uint8(D.Topic)==Tp?1:0;
   const ETopic T=ETopic(Tp);
   bTopics&=Count>=((T==ETopic::Carcass||T==ETopic::Cattle||T==ETopic::Lights||T==ETopic::Stalled)?3:2);
  }
  Check(bTopics,TEXT("each kind of night has at least two front-page stories, a dead cow, cattle, lights and a car that dies three"));
 }
 // A game's papers, made as FS47Mail makes them: the front from the week's findings and the templates used before, the rest Fill's.
 const auto DateOf=[](int32 No){return FDateTime(1986,5,1)+FTimespan::FromDays((No-1)*7);};
 const auto Make=[&](int32 Seed,int32 No,const TArray<FFact>& Facts,TMap<FString,int32>& Used)
 {
  FIssue I;I.No=No;I.Day=4+(No-1)*7;I.Date=DateOf(No);
  I.Front=Front(Seed,No,I.Date,Facts,Used,TEXT("shack"),TEXT("the line shack"));
  I.Log=LogLines(Facts,I.Front,I.Day,L::MostLogged);
  Fill(I,Seed,No==3);
  for(const FStory& St:I.Front)Used.Add(St.Key,No);
  return I;
 };
 const TArray<FFact> Week={{ETopic::Lights,9,TEXT("Tuesday night"),TEXT("the windmill on the Baca lease"),TEXT("windmill"),TEXT("landing_seen_d9")},{ETopic::Cattle,9,TEXT("Tuesday night"),TEXT("the Alderete place"),FString(),TEXT("cattle_ring_d9")}};
 bool bSame=true,bUnique=true,bFull=true,bShort=true,bPrinted=true,bDated=true,bThreads=true;TSet<FString> Seen;int32 MostWords=0;FString Late;
 // Every item's window and thread, by its text.
 TMap<FString,TPair<int32,int32>> Windows;TMap<FString,FString> ThreadOf;
 for(const X::FItemDef& D:X::Locals()){Windows.Add(D.Text,{D.First,D.Last});if(D.Thread)ThreadOf.Add(D.Text,D.Thread);}
 for(const X::FAdDef& D:X::Ads())Windows.Add(D.Body,{D.First,D.Last});
 for(const X::FLetterDef& D:X::Letters()){Windows.Add(D.Body,{D.First,D.Last});if(D.Thread)ThreadOf.Add(D.Body,D.Thread);}
 for(const X::FFrontDef& D:X::Quiet()){Windows.Add(D.Head,{D.First,D.Last});if(D.Thread)ThreadOf.Add(D.Head,D.Thread);}
 for(const int32 TrySeed:{470047,1,-77,90210})
 {
  TMap<FString,int32> UsedA,UsedB;
  for(int32 No=1;No<=6;++No)
  {
   const TArray<FFact> Facts=No==2?Week:TArray<FFact>();
   const FIssue A=Make(TrySeed,No,Facts,UsedA),B=Make(TrySeed,No,Facts,UsedB);
   bSame&=PlainText(A)==PlainText(B);
   TArray<FString> Printed=A.Locals;Printed.Append(A.Sheriff);for(const FNote& Nt:A.Letters)if(Nt.Head!=X::Thanks().Head)Printed.Add(Nt.Body);for(const FAd& Ad:A.Ads)Printed.Add(Ad.Body);
   for(const FStory& St:A.Front)if(St.FactId.IsEmpty())Printed.Add(St.Head);
   if(TrySeed==470047&&No<=5)for(const FString& Bit:Printed){bUnique&=!Seen.Contains(Bit);Seen.Add(Bit);}
   // Nothing with a date after its date (or before it, for what came of it), and one thread to a page.
   TSet<FString> PageThreads;
   for(const FString& Bit:Printed)
   {
    if(const TPair<int32,int32>* W=Windows.Find(Bit))if(!X::InWindow(W->Key,W->Value,A.Date)){bDated=false;Late=Bit.Left(50);}
    if(const FString* Th=ThreadOf.Find(Bit)){bThreads&=!PageThreads.Contains(*Th);PageThreads.Add(*Th);}
   }
   bFull&=A.Front.Num()==2&&A.Locals.Num()==L::LocalsPer&&A.Sheriff.Num()==L::SheriffPer&&A.Ads.Num()==1+L::PlainAdsPer&&A.Week.Num()==5&&!A.Showing.IsEmpty()&&!A.Almanac.IsEmpty();
   TArray<FString> Counted;PlainText(A).ParseIntoArrayWS(Counted);MostWords=FMath::Max(MostWords,Counted.Num());
   const FString Text=PlainText(A);bShort&=!Text.Contains(TEXT("{"))&&!Text.Contains(TEXT("}"))&&!Text.Contains(TEXT("somewhere out of town"),ESearchCase::CaseSensitive);
   FString Word;bPrinted&=Clean(Text,Word);
   bPrinted&=!Text.Contains(TEXT("dust"))&&!Text.Contains(TEXT("sprinkle"))&&!Text.Contains(TEXT("frost")); // the moon is in the sky since MOON-20261108
  }
 }
 Check(bPrinted,TEXT("no paper as printed says a forbidden word, or weather the sky does not show (dust, sprinkles, frost)"));
 Check(bSame,TEXT("a game's paper is the same every time it is opened"));
 Check(bUnique,TEXT("no county item, log entry, letter, ad or county front page is printed twice in five weeks"));
 Check(bDated,bDated?FString(TEXT("nothing with a date is printed after it, or before it for what came of it")):TEXT("printed outside its dates: ")+Late);
 Check(bThreads,TEXT("one item of a story (the truck stop, the rodeo, the graduation) to a page, the front included"));
 Check(bFull,TEXT("every paper has its front page, county news, the log, ads, weather, almanac and the picture show"));
 Check(bShort&&MostWords<=700,FString::Printf(TEXT("short enough to read through in a few minutes (%d words at most), nothing left unfilled and no stand-in for a place"),MostWords));
 {
  TMap<FString,int32> Used;
  const FIssue First=Make(470047,1,TArray<FFact>(),Used);
  Check(First.Front[0].Head.Contains(TEXT("SEARCH"))&&First.Front[0].Lead==TEXT("wagon"),TEXT("the first paper carries the search for the two missing since April 14, and points at the wagon"));
  const FIssue Second=Make(470047,2,Week,Used);
  const FStory* Lit=Second.Front.FindByPredicate([](const FStory& St){return St.FactId==TEXT("landing_seen_d9");});
  const FStory* Herd=Second.Front.FindByPredicate([](const FStory& St){return St.FactId==TEXT("cattle_ring_d9");});
  Check(Lit&&Herd&&Lit->Body.Contains(TEXT("the windmill on the Baca lease"))&&Herd->Body.Contains(TEXT("the Alderete place"))&&Lit->Body.Contains(TEXT("Tuesday night"))&&Herd->Body.Contains(TEXT("Tuesday night")),
   TEXT("a week with lights and cattle puts both on the front page, each at the place it was and on its night"));
  Check(Lit&&Lit->Lead==TEXT("windmill"),TEXT("and the story at a place on the land points at it"));
  const FIssue Quiet=Make(470047,3,TArray<FFact>(),Used);
  Check(Quiet.Front.Last().Lead==TEXT("shack")&&Quiet.Front.Last().Body.Contains(TEXT("the line shack")),TEXT("a quiet week gets talk about a place nobody has mentioned yet"));
  Check(Quiet.Letters.Num()==3&&Quiet.Letters[0].Head==X::Thanks().Head,TEXT("the trucker's thanks is printed the week there was cause"));
  Check(Second.Week[1].EndsWith(S47Weather::Words(S47Weather::Forecast(470047,Second.Day,Second.Day+1),true))&&Second.Almanac==S47Weather::Almanac(Second.Date),TEXT("the forecast and the almanac are S47Weather's, the sky's own"));
 }
 {
  // The critics' week: cattle in a ring by the church Tuesday, a light in the farmhouse Tuesday, a dead cow by the church Wednesday.
  // All three in the paper, each at its place and on its night: two on the front, the third in the sheriff's log.
  const TArray<FFact> Three={{ETopic::Cattle,9,TEXT("Tuesday night"),TEXT("the church"),FString(),TEXT("cattle_ring_d9")},
   {ETopic::LitHouse,9,TEXT("Tuesday night"),TEXT("the old house"),FString(),TEXT("house_lit_d9")},
   {ETopic::Carcass,10,TEXT("Wednesday night"),TEXT("the church"),FString(),TEXT("carcass_d10")}};
  TMap<FString,int32> Used;
  const FIssue P=Make(1,2,Three,Used);
  const FString Text=PlainText(P);
  const bool bCow=Text.Contains(TEXT("out by the church Wednesday night"))||Text.Contains(TEXT("Wednesday night out by the church"));
  const bool bLight=Text.Contains(TEXT("at the old house Tuesday night"))||Text.Contains(TEXT("by the old house Tuesday night"))||Text.Contains(TEXT("at the old house late Tuesday night"));
  const bool bHerd=P.Sheriff.ContainsByPredicate([](const FString& S){return S.StartsWith(TEXT("Tuesday night. Cattle"))&&S.Contains(TEXT("the church"));})||Text.Contains(TEXT("out by the church were seen"))||Text.Contains(TEXT("out by the church stood"));
  Check(bCow&&bLight&&bHerd,TEXT("cattle at the church Tuesday, a lit house Tuesday and a dead cow at the church Wednesday are all printed at the right place and night"));
  // A finding with no place gets no story that names one, and nothing stands in for it.
  const TArray<FFact> Nowhere={{ETopic::Cattle,9,TEXT("Tuesday night"),FString(),FString(),TEXT("cattle_ring_d9")}};
  TMap<FString,int32> Fresh;const FIssue N=Make(1,2,Nowhere,Fresh);
  Check(!N.Front.ContainsByPredicate([](const FStory& St){return St.FactId==TEXT("cattle_ring_d9")||St.Body.Contains(TEXT("old grade"));})&&N.Log.IsEmpty(),TEXT("a finding without a place is never put at one, and never at the old grade"));
 }
 {
  // The same kind of night five weeks running: no front page twice in three weeks, and the county's own news never twice.
  TMap<FString,int32> Used;TSet<FString> Heads;bool bFresh=true;
  for(int32 No=2;No<=4;++No)
  {
   const TArray<FFact> Herd={{ETopic::Cattle,4+(No-1)*7-2,TEXT("Tuesday night"),No%2?TEXT("the church"):TEXT("the Alderete place"),FString(),FString::Printf(TEXT("cattle_ring_d%d"),No)}};
   const FIssue I=Make(470047,No,Herd,Used);
   for(const FStory& St:I.Front){bFresh&=!Heads.Contains(St.Key);Heads.Add(St.Key);}
  }
  Check(bFresh,TEXT("cattle three weeks running get three different front pages, and the county news beside them differs too"));
 }
 {
  // D45-24-20261110: what the reader can pencil onto the map. Every lead a paper prints is one the map can show; each article
  // that has one is offered once, on the sheet it is printed on (the front's stories on the front, letters inside), and nothing
  // without a lead is offered. The first paper's search story is on the front and points at the wagon; a quiet week's talk points
  // at the place nobody has mentioned; the lights over the windmill point at the windmill.
  TMap<FString,int32> Used;bool bResolve=true,bOffered=true,bOnly=true;
  FIssue FirstIss,SecondIss,QuietIss;
  for(int32 No=1;No<=6;++No)
  {
   const FIssue Iss=Make(470047,No,No==2?Week:TArray<FFact>(),Used);
   if(No==1)FirstIss=Iss;else if(No==2)SecondIss=Iss;else if(No==3)QuietIss=Iss;
   const TArray<FLeadRef> Refs=LeadsOf(Iss);
   for(const FStory& St:Iss.Front)if(!St.Lead.IsEmpty()){bResolve&=!LeadTitle(St.Lead).IsEmpty();bOffered&=Refs.ContainsByPredicate([&](const FLeadRef& Ref){return Ref.Sheet==0&&Ref.Lead==St.Lead;});}
   for(const FNote& Nt:Iss.Letters)if(!Nt.Lead.IsEmpty()){bResolve&=!LeadTitle(Nt.Lead).IsEmpty();bOffered&=Refs.ContainsByPredicate([&](const FLeadRef& Ref){return Ref.Sheet==1&&Ref.Lead==Nt.Lead;});}
   for(const FLeadRef& Ref:Refs)
   {
    bOnly&=!Ref.Lead.IsEmpty()&&(Ref.Sheet==0?Iss.Front.ContainsByPredicate([&](const FStory& St){return St.Lead==Ref.Lead&&St.Head==Ref.Head;}):Iss.Letters.ContainsByPredicate([&](const FNote& Nt){return Nt.Lead==Ref.Lead&&Nt.Head==Ref.Head;}));
    bOnly&=Refs.FilterByPredicate([&](const FLeadRef& Q){return Q.Lead==Ref.Lead&&Q.Sheet==Ref.Sheet;}).Num()==1;
   }
  }
  Check(bResolve,TEXT("D45-24: every lead a paper prints is a place the map can show, or the wagon"));
  Check(bOffered&&bOnly,TEXT("D45-24: each article with a lead is offered once, on its own sheet, and nothing else is"));
  Check(LeadsOf(FirstIss).ContainsByPredicate([](const FLeadRef& Ref){return Ref.Lead==TEXT("wagon")&&Ref.Sheet==0&&Ref.Head.Contains(TEXT("SEARCH"));})
   &&LeadsOf(SecondIss).ContainsByPredicate([](const FLeadRef& Ref){return Ref.Lead==TEXT("windmill")&&Ref.Sheet==0;})
   &&LeadsOf(QuietIss).ContainsByPredicate([](const FLeadRef& Ref){return Ref.Lead==TEXT("shack")&&Ref.Sheet==0;}),
   TEXT("D45-24: the search story points at the wagon, the lights at the windmill and a quiet week's talk at the line shack, each on the front"));
  Check(LeadTitle(TEXT("wagon")).Contains(TEXT("wagon"))&&LeadTitle(TEXT("cairn"))==TEXT("the cairn on the rise")&&LeadTitle(TEXT("church")).IsEmpty()&&LeadTitle(FString()).IsEmpty(),
   TEXT("D45-24: a lead is named as the map names it, and what the map cannot show is not offered"));
  // The photo's caption is the paper's, never "taken" or a "photograph of" (Clean, over AllText, holds the fixed ones).
  bool bCaptions=true;FString Bad;
  for(const FS47Places::FDef& D:FS47Places::Defs()){const FString Cap=L::Caption(D.Id);bCaptions&=Cap.EndsWith(TEXT("Ledger photo."))&&!Cap.ToLower().Contains(TEXT("taken"))&&!Cap.ToLower().Contains(TEXT("photograph of"));if(!bCaptions&&Bad.IsEmpty())Bad=Cap;}
  Check(bCaptions,bCaptions?FString(TEXT("D45-24: every photo caption reads \"<place>, Wednesday. Ledger photo.\"")):TEXT("D45-24: a caption the paper must not print: ")+Bad);
 }
 return OK;
}

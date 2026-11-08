#include "S47Mail.h"
#include "S47Audio.h"
#include "S47Docs.h"
#include "S47EventPlaces.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47Ledger.h"
#include "S47MailBox.h"
#include "S47Main.h"
#include "S47Openings.h"
#include "S47Places.h"
#include "S47Predecessor.h"
#include "S47Story.h"
#include "S47Taken.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Weather.h"
#include "S47Web.h"
#include "S47WebAudio.h"
#include "S47World.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"

namespace S47MailLocal
{
 const TCHAR* const BoxUse=TEXT("ff:mail:box");
 const TCHAR* const BoxRect=TEXT("ff:mailbox");
 const TCHAR* const WeekNames[]={TEXT("Monday"),TEXT("Tuesday"),TEXT("Wednesday"),TEXT("Thursday"),TEXT("Friday"),TEXT("Saturday"),TEXT("Sunday")};
 const TCHAR* const MonthNames[]={TEXT("January"),TEXT("February"),TEXT("March"),TEXT("April"),TEXT("May"),TEXT("June"),TEXT("July"),TEXT("August"),TEXT("September"),TEXT("October"),TEXT("November"),TEXT("December")};
 double Draw(int32 Seed,int32 Which,uint32 Salt)
 {
  uint32 Hash=uint32(Seed)*2246822519u^uint32(Which)*3266489917u^(Salt+7u)*668265263u;
  Hash^=Hash>>15;Hash*=2654435761u;Hash^=Hash>>13;Hash*=2246822519u;Hash^=Hash>>16;
  return double(Hash)/4294967296.;
 }
 S47J::FObj GoodsJson(const TMap<FString,int32>& Goods){auto O=S47J::New();for(const auto& G:Goods)S47J::Set(O,*G.Key,G.Value);return O;}
 TMap<FString,int32> GoodsOf(const S47J::FObj& O)
 {
  TMap<FString,int32> Got;if(!O.IsValid())return Got;
  for(const auto& Pair:O->Values){double Cnt=0;if(Pair.Value.IsValid()&&Pair.Value->TryGetNumber(Cnt)&&FMath::IsFinite(Cnt)&&Cnt>=1&&Cnt<=99)Got.Add(Pair.Key,int32(Cnt));}
  return Got;
 }
 S47J::FObj PieceJson(const FS47Mail::FPiece& Pc)
 {
  auto O=S47J::New();S47J::Set(O,TEXT("id"),Pc.Id);S47J::Set(O,TEXT("kind"),Pc.Kind);S47J::Set(O,TEXT("day"),Pc.Day);
  if(!Pc.Ref.IsEmpty())S47J::Set(O,TEXT("ref"),Pc.Ref);
  if(!Pc.Text.IsEmpty())S47J::Set(O,TEXT("text"),Pc.Text);
  if(!Pc.Goods.IsEmpty())S47J::Set(O,TEXT("goods"),GoodsJson(Pc.Goods));
  return O;
 }
 bool PieceOf(const TSharedPtr<FJsonValue>& V,FS47Mail::FPiece& Out)
 {
  if(!V.IsValid()||V->Type!=EJson::Object)return false;const auto O=V->AsObject();if(!O.IsValid())return false;
  Out.Id=S47J::Str(O,TEXT("id"));Out.Kind=S47J::Str(O,TEXT("kind"));Out.Ref=S47J::Str(O,TEXT("ref"));Out.Text=S47J::Str(O,TEXT("text"));
  Out.Day=FMath::Clamp(S47J::Int(O,TEXT("day")),0,36500);Out.Goods=GoodsOf(S47J::Obj(O,TEXT("goods")));
  static const TCHAR* const KindNames[]={TEXT("ward"),TEXT("ledger"),TEXT("catalog"),TEXT("parcel"),TEXT("bill"),TEXT("nobody")};
  bool bKind=false;for(const TCHAR* K:KindNames)bKind|=Out.Kind==K;
  return bKind&&!Out.Id.IsEmpty();
 }
 // A catalog line's goods id, without "kit:".
 FString KitOf(const FString& Goods){return Goods.StartsWith(TEXT("kit:"))?Goods.Mid(4):FString();}
}

FS47Mail::FS47Mail(US47Main* Main):M(Main){}
// The scene's parts belong to the scene; nothing is touched on the way out (the session's Stop has put them away).
FS47Mail::~FS47Mail(){}

// ---------- the catalog ----------
const TArray<FS47Mail::FLine>& FS47Mail::Catalog()
{
 // Cheaper than the Caliche counter, line by line, before the postage; the gear from OPS a little cheaper too, once it is on offer.
 static const TArray<FLine> Table={
  {TEXT("fuse"),TEXT("Control fuse, slow-blow"),4},
  {TEXT("relay"),TEXT("Drive relay, 24 volt, bench tested"),19},
  {TEXT("donor"),TEXT("Used controller, for its relay"),11},
  {TEXT("recorder_donor"),TEXT("Field recorder, used, sold as is"),15},
  {TEXT("cell"),TEXT("Dry cell, heavy duty"),3},
  {TEXT("foil"),TEXT("Shielding foil, 25-foot roll"),2},
  {TEXT("film"),TEXT("Film, 36 exposures"),7},
  {TEXT("drive_motor"),TEXT("Antenna drive motor, reconditioned"),27},
  {TEXT("kit:geiger_tube"),TEXT("Geiger-Muller tube and a meter movement"),20},
  {TEXT("kit:film"),TEXT("Fast film and a better lens"),24},
 };
 return Table;
}

// ---------- a game's life ----------
bool FS47Mail::Live() const{return M&&M->Freeform&&M->Freeform->Active()&&(bTest||M->Freeform->RulesLive());}
void FS47Mail::Begin(const S47J::FObj& Saved,bool bSavedGame)
{
 namespace L=S47MailLocal;
 Stop();Life=MakeShared<uint8>(0);
 Box.Reset();Pending.Reset();Fetched.Reset();Posted.Reset();ReadIssues.Reset();Orders.Reset();Coupons.Reset();Issues.Reset();Cart.Reset();
 bFlag=bCatalog=bBound=false;LastRound=Rounds=NobodySent=0;NextSerial=1;
 const int32 Today=M&&M->Freeform?M->Freeform->Day():1;
 if(Saved.IsValid()&&Saved->Values.Num()>0)
 {
  bFlag=S47J::Bool(Saved,TEXT("flag"));bCatalog=S47J::Bool(Saved,TEXT("catalog"));
  LastRound=FMath::Clamp(S47J::Int(Saved,TEXT("lastRound")),0,36500);Rounds=FMath::Max(0,S47J::Int(Saved,TEXT("rounds")));
  NextSerial=FMath::Max(1,S47J::Int(Saved,TEXT("serial"),1));NobodySent=FMath::Clamp(S47J::Int(Saved,TEXT("nobody")),0,9);
  for(const auto& V:S47J::Arr(Saved,TEXT("box"))){FPiece Pc;if(L::PieceOf(V,Pc)&&!Box.ContainsByPredicate([&](const FPiece& Q){return Q.Id==Pc.Id;}))Box.Add(Pc);}
  for(const auto& V:S47J::Arr(Saved,TEXT("pending"))){FPiece Pc;if(L::PieceOf(V,Pc))Pending.Add(Pc);}
  for(const auto& V:S47J::Arr(Saved,TEXT("fetched"))){FPiece Pc;if(L::PieceOf(V,Pc))Fetched.Add(Pc);}
  for(const FString& Id:S47J::Strs(Saved,TEXT("posted")))Posted.Add(Id);
  for(const FString& Id:S47J::Strs(Saved,TEXT("read")))ReadIssues.Add(Id);
  for(const FPiece& Pc:Box)Posted.Add(Pc.Id);for(const FPiece& Pc:Pending)Posted.Add(Pc.Id);
  for(const auto& V:S47J::Arr(Saved,TEXT("orders")))
  {
   if(!V.IsValid()||V->Type!=EJson::Object)continue;const auto O=V->AsObject();FOrder Ord;
   Ord.Serial=FMath::Max(1,S47J::Int(O,TEXT("serial"),1));Ord.Paid=FMath::Clamp(S47J::Int(O,TEXT("paid")),0,10000);
   Ord.Picked=FMath::Clamp(S47J::Int(O,TEXT("picked")),0,36500);Ord.Due=FMath::Clamp(S47J::Int(O,TEXT("due")),0,36600);Ord.Goods=L::GoodsOf(S47J::Obj(O,TEXT("goods")));
   if(!Ord.Goods.IsEmpty()){Orders.Add(Ord);NextSerial=FMath::Max(NextSerial,Ord.Serial+1);}
  }
  for(const auto& V:S47J::Arr(Saved,TEXT("coupons")))
  {
   if(!V.IsValid()||V->Type!=EJson::Object)continue;const auto O=V->AsObject();FClip Cp;
   Cp.Id=S47J::Str(O,TEXT("id"));Cp.What=S47J::Str(O,TEXT("what"));Cp.Price=FMath::Clamp(S47J::Int(O,TEXT("price")),0,10000);Cp.Until=FMath::Clamp(S47J::Int(O,TEXT("until")),0,36600);
   if(S47Ledger::Coupon(Cp.Id))Coupons.Add(Cp);
  }
  for(const auto& V:S47J::Arr(Saved,TEXT("issues")))
  {
   if(!V.IsValid()||V->Type!=EJson::Object)continue;const auto O=V->AsObject();FKept K;
   K.No=FMath::Max(1,S47J::Int(O,TEXT("no"),1));K.Day=FMath::Max(1,S47J::Int(O,TEXT("day"),1));K.bThanks=S47J::Bool(O,TEXT("thanks"));
   K.Heads=S47J::Strs(O,TEXT("heads"));K.Bodies=S47J::Strs(O,TEXT("bodies"));K.Leads=S47J::Strs(O,TEXT("leads"));
   if(K.Heads.Num()==K.Bodies.Num()&&K.Heads.Num()==K.Leads.Num()&&!Issues.ContainsByPredicate([&](const FKept& Q){return Q.No==K.No;}))Issues.Add(K);
  }
  Issues.Sort([](const FKept& A,const FKept& B){return A.No<B.No;});
 }
 else
 {
  // A new game, or a save from before the post: the catalog is in the box and the flag is up. A save under way does not get the
  // rounds it never had: they start from today's.
  FPiece Pc;Pc.Id=TEXT("catalog");Pc.Kind=TEXT("catalog");Pc.Day=Today;Box.Add(Pc);Posted.Add(Pc.Id);bFlag=true;
  if(bSavedGame&&M&&M->Freeform)LastRound=M->Freeform->Clock()>=RoundClock(M->Freeform->Seed(),Today)?Today:Today-1;
 }
 // A long time away (an old save) is caught up from two weeks back at most.
 LastRound=FMath::Max(LastRound,Today-14);
 UE_LOG(LogTemp,Display,TEXT("S47: mail: %d in the box, %d on the way, %d orders, flag %s, last round day %d"),Box.Num(),Pending.Num(),Orders.Num(),bFlag?TEXT("up"):TEXT("down"),LastRound);
}
void FS47Mail::Stop()
{
 Life.Reset();bBound=false;
 if(IsEngineExitRequested()||IsGarbageCollecting()||!IsValid(M)||M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed))return;
 if(Model)Model->Show(false);
 if(IsValid(M->Web.Get())&&M->Web->HasWalkArea(TEXT("saro")))for(auto& Rect:M->Web->CollidersMutable(TEXT("saro")))if(Rect.Id==S47MailLocal::BoxRect)Rect.bEnabled=false;
}
S47J::FObj FS47Mail::Save() const
{
 namespace L=S47MailLocal;
 auto O=S47J::New();
 S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("flag"),bFlag);S47J::Set(O,TEXT("lastRound"),LastRound);S47J::Set(O,TEXT("rounds"),Rounds);S47J::Set(O,TEXT("serial"),NextSerial);
 if(bCatalog)S47J::Set(O,TEXT("catalog"),true);
 if(NobodySent>0)S47J::Set(O,TEXT("nobody"),NobodySent);
 const auto Pieces=[&](const TCHAR* Key,const TArray<FPiece>& List)
 {
  if(List.IsEmpty())return;
  TArray<TSharedPtr<FJsonValue>> A;for(const FPiece& Pc:List)A.Add(MakeShared<FJsonValueObject>(L::PieceJson(Pc)));
  O->SetArrayField(Key,A);
 };
 Pieces(TEXT("box"),Box);Pieces(TEXT("pending"),Pending);Pieces(TEXT("fetched"),Fetched);
 if(!Posted.IsEmpty()){TArray<FString> Ids=Posted.Array();Ids.Sort();S47J::Set(O,TEXT("posted"),Ids);}
 if(!ReadIssues.IsEmpty()){TArray<FString> Ids=ReadIssues.Array();Ids.Sort();S47J::Set(O,TEXT("read"),Ids);}
 if(!Orders.IsEmpty())
 {
  TArray<TSharedPtr<FJsonValue>> A;
  for(const FOrder& Ord:Orders){auto V=S47J::New();S47J::Set(V,TEXT("serial"),Ord.Serial);S47J::Set(V,TEXT("paid"),Ord.Paid);S47J::Set(V,TEXT("picked"),Ord.Picked);S47J::Set(V,TEXT("due"),Ord.Due);S47J::Set(V,TEXT("goods"),L::GoodsJson(Ord.Goods));A.Add(MakeShared<FJsonValueObject>(V));}
  O->SetArrayField(TEXT("orders"),A);
 }
 if(!Coupons.IsEmpty())
 {
  TArray<TSharedPtr<FJsonValue>> A;
  for(const FClip& Cp:Coupons){auto V=S47J::New();S47J::Set(V,TEXT("id"),Cp.Id);S47J::Set(V,TEXT("what"),Cp.What);S47J::Set(V,TEXT("price"),Cp.Price);S47J::Set(V,TEXT("until"),Cp.Until);A.Add(MakeShared<FJsonValueObject>(V));}
  O->SetArrayField(TEXT("coupons"),A);
 }
 if(!Issues.IsEmpty())
 {
  TArray<TSharedPtr<FJsonValue>> A;
  for(const FKept& K:Issues){auto V=S47J::New();S47J::Set(V,TEXT("no"),K.No);S47J::Set(V,TEXT("day"),K.Day);if(K.bThanks)S47J::Set(V,TEXT("thanks"),true);S47J::Set(V,TEXT("heads"),K.Heads);S47J::Set(V,TEXT("bodies"),K.Bodies);S47J::Set(V,TEXT("leads"),K.Leads);A.Add(MakeShared<FJsonValueObject>(V));}
  O->SetArrayField(TEXT("issues"),A);
 }
 return O;
}
bool FS47Mail::Build()
{
 namespace L=S47MailLocal;
 if(!M||!IsValid(M->Web.Get())||!M->Web->IsLoaded()||!IsValid(M->Inter))return false;
 const int32 Parent=M->Web->Handle(TEXT("ext.group"));if(Parent<0)return false;
 if(!Model)Model=MakeUnique<FS47MailBoxModel>();
 if(!Model->Built()){if(!Model->Build(M->Web.Get(),Parent))return false;bBound=false;}
 if(bBound)return true;
 bBound=true;Model->Show(true);
 const TWeakPtr<uint8> Guard=Life;
 FS47Interactable I;I.Id=L::BoxUse;I.Object=Model->Pick;I.Range=2.6;
 I.Label=[this,Guard]
 {
  if(!Guard.IsValid()||!Live())return FString();
  return bFlag?FString(TEXT("Mailbox / the flag is up")):FString(TEXT("Mailbox / SARO"));
 };
 I.Use=[this,Guard]{if(Guard.IsValid()&&Live())OpenBox();};
 I.Examine=[this,Guard]
 {
  if(!Guard.IsValid()||!Live())return FString();
  return bFlag?FString(TEXT("A rural mailbox on a post, SARO stenciled on the side, the red flag up.")):FString(TEXT("A rural mailbox on a post, SARO stenciled on the side. The flag is down."));
 };
 M->Inter->Add(I);M->Web->MarkUsable(Model->Pick.I,I.Id);
 // Nobody walks through it.
 auto& Rects=M->Web->CollidersMutable(TEXT("saro"));
 const double PostE=FS47MailBoxModel::PostX,PostS=FS47MailBoxModel::PostZ;
 if(FS47Rect* Mine=Rects.FindByPredicate([](const FS47Rect& Rect){return Rect.Id==S47MailLocal::BoxRect;}))Mine->bEnabled=true;
 else Rects.Add({PostE-.42,PostE+.12,PostS-.12,PostS+.12,L::BoxRect,true});
 return true;
}
void FS47Mail::Step(double Dt)
{
 if(!Live())return;
 auto& Ses=*M->Freeform;
 if(!Build())return;
 if(!Model->Seated()&&M->Web->Area()==TEXT("saro")&&M->Player&&FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(FS47MailBoxModel::PostX,FS47MailBoxModel::PostZ))<250)Model->Seat(M->Web.Get());
 Model->SetFlag(bFlag);Model->Step(Dt);
 if(Ses.Held())return;
 // The rounds that are due: today's once its hour has come, and any a long rest went past. No rounds on Sundays or Memorial Day.
 const int32 Today=Ses.Day();
 for(int32 OnDay=LastRound+1;OnDay<=Today;++OnDay)
 {
  if(OnDay==Today&&Ses.Clock()<RoundClock(Ses.Seed(),OnDay))break;
  LastRound=OnDay;
  if(RoundOn(Ses.DateOfDay(OnDay)))Round(OnDay,OnDay==Today);
 }
}

// ---------- the carrier ----------
bool FS47Mail::RoundOn(const FDateTime& Date)
{
 if(Date.GetDayOfWeek()==EDayOfWeek::Sunday)return false;
 // Memorial Day: the last Monday in May. The Ledger says so too.
 if(Date.GetMonth()==5&&Date.GetDayOfWeek()==EDayOfWeek::Monday&&Date.GetDay()+7>31)return false;
 return true;
}
double FS47Mail::RoundClock(int32 Seed,int32 ShiftDay)
{
 const int32 Cnt=FMath::Max(1,ShiftDay);
 return double(Cnt-1)*86400.+12*3600.+FMath::FloorToDouble(S47MailLocal::Draw(Seed,Cnt,1)*90.)*60.;
}
int32 FS47Mail::ShipRounds(int32 Seed,int32 Serial){return 1+FMath::Min(2,int32(S47MailLocal::Draw(Seed,Serial,2)*3.));}
int32 FS47Mail::NobodyDay(int32 Which) const
{
 const int32 Seed=M&&M->Freeform?M->Freeform->Seed():0;
 return (Which==0?8:19)+int32(S47MailLocal::Draw(Seed,Which,3)*7.);
}
void FS47Mail::Round(int32 OnDay,bool bHere)
{
 if(!M||!M->Freeform)return;
 auto& Ses=*M->Freeform;
 ++Rounds;
 int32 Left=0,Picked=0;
 // Out: the orders waiting in the box, with their money orders.
 for(FOrder& Ord:Orders)if(Ord.Picked==0)
 {
  Ord.Picked=OnDay;int32 Due=OnDay;
  for(int32 K=0,Tries=0;K<ShipRounds(Ses.Seed(),Ord.Serial)&&Tries<20;++Tries){++Due;if(RoundOn(Ses.DateOfDay(Due)))++K;}
  Ord.Due=Due;++Picked;
  UE_LOG(LogTemp,Display,TEXT("S47: mail: the carrier took order %d, due day %d"),Ord.Serial,Due);
 }
 // In: what was posted for today, parcels that are due, Thursday's paper, and now and then what nobody sent.
 for(int32 I=Pending.Num()-1;I>=0;--I)if(Pending[I].Day<=OnDay)
 {
  FPiece Pc=Pending[I];Pc.Day=OnDay;Pending.RemoveAt(I);Box.Insert(Pc,0);++Left;
 }
 for(int32 I=Orders.Num()-1;I>=0;--I)if(Orders[I].Picked>0&&Orders[I].Due<=OnDay)
 {
  const FOrder Ord=Orders[I];Orders.RemoveAt(I);
  FPiece Pc;Pc.Id=FString::Printf(TEXT("parcel:%d"),Ord.Serial);Pc.Kind=TEXT("parcel");Pc.Ref=FString::FromInt(Ord.Serial);Pc.Day=OnDay;Pc.Goods=Ord.Goods;Pc.Text=SlipText(Ord.Serial,Ord.Goods,Ord.Paid);
  Box.Add(Pc);Posted.Add(Pc.Id);++Left;
  UE_LOG(LogTemp,Display,TEXT("S47: mail: parcel %d in the box, day %d"),Ord.Serial,OnDay);
 }
 if(Ses.DateOfDay(OnDay).GetDayOfWeek()==EDayOfWeek::Thursday){PrintLedger(OnDay);++Left;}
 if(NobodySent<2&&OnDay>=NobodyDay(NobodySent)&&!Ses.InTutorial())
 {
  const int32 V0=FMath::Abs(Ses.Seed())%3,Variant=NobodySent==0?V0:(V0+1+(FMath::Abs(Ses.Seed())/3)%2)%3;
  FPiece Pc;Pc.Id=FString::Printf(TEXT("nobody:%d"),NobodySent+1);Pc.Kind=TEXT("nobody");Pc.Ref=FString::FromInt(Variant);Pc.Day=OnDay;
  Pc.Text=NobodyText(Variant,DayText(OnDay+9).ToUpper());
  Box.Add(Pc);Posted.Add(Pc.Id);++NobodySent;++Left;
  UE_LOG(LogTemp,Display,TEXT("S47: mail: something with no sender in the box, day %d"),OnDay);
 }
 bFlag=!Box.IsEmpty();
 UE_LOG(LogTemp,Display,TEXT("S47: mail: round %d on day %d: left %d, picked up %d, %d in the box, flag %s"),Rounds,OnDay,Left,Picked,Box.Num(),bFlag?TEXT("up"):TEXT("down"));
 if(bHere)Truck(Left+Picked>0);
 M->RequestAutosave();
}
void FS47Mail::Truck(bool bStops)
{
 // The carrier's car up the highway from the south: heard, not seen, from the station; it stops at the box only when it has
 // something to leave or to take, the lid clanks twice, and it goes on north. From what the game already has; nothing new recorded.
 if(!M||!IsValid(M->Audio)||!M->Audio->Ctx.IsValid()||!M->Player||!M->Web||M->Web->Area()!=TEXT("saro")||!M->Freeform)return;
 const double Away=FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(FS47MailBoxModel::PostX,FS47MailBoxModel::PostZ));
 if(Away>700)return;
 const bool bIn=M->Freeform->Taken&&M->Freeform->Taken->Indoors();
 const float Near=float(FMath::Clamp(28./(28.+Away),.04,1.))*(bIn?.35f:1.f);
 const double Now=M->Audio->Ctx->CurrentTime();
 FS47Sound Motor;Motor.bLoop=true;Motor.Lp=bIn?900.f:2300.f;Motor.Dur=bStops?21.f:13.f;Motor.Rate=.92f;
 const FS47Play Engine=M->Audio->Sfx(TEXT("engineDrive"),Motor);
 if(Engine.Gain.IsValid())
 {
  auto& G=Engine.Gain->Gain;const double Peak=.5*Near,Faint=.05*Near;
  G.SetValueAtTime(0.,Now);G.LinearRampToValueAtTime(Faint,Now+1.5);
  if(bStops){G.LinearRampToValueAtTime(Peak,Now+7.);G.LinearRampToValueAtTime(Peak*.3,Now+8.);G.LinearRampToValueAtTime(Peak*.3,Now+12.5);G.LinearRampToValueAtTime(Peak,Now+14.);G.LinearRampToValueAtTime(0.,Now+21.);}
  else{G.LinearRampToValueAtTime(Peak,Now+6.);G.LinearRampToValueAtTime(0.,Now+13.);}
 }
 if(bStops)
 {
  FS47Sound Idle;Idle.bLoop=true;Idle.When=7.5f;Idle.Dur=5.5f;Idle.Gain=.4f*Near;Idle.Lp=Motor.Lp;M->Audio->Sfx(TEXT("engineIdle"),Idle);
  FS47Sound Clank;Clank.When=9.2f;Clank.Gain=.8f*Near;M->Audio->Sfx(TEXT("metalTap"),Clank);
  Clank.When=10.9f;Clank.Gain=.55f*Near;M->Audio->Sfx(TEXT("metalTap"),Clank);
 }
 UE_LOG(LogTemp,Display,TEXT("S47: mail: the carrier's car on the highway, %.0f m from the box%s"),Away,bStops?TEXT(", stopping"):TEXT(", going by"));
}
bool FS47Mail::Post(const FPiece& Piece,bool bNow)
{
 if(Piece.Id.IsEmpty()||Posted.Contains(Piece.Id))return false;
 Posted.Add(Piece.Id);
 FPiece Pc=Piece;
 const int32 Today=M&&M->Freeform?M->Freeform->Day():1;
 if(bNow){Pc.Day=Today;Box.Add(Pc);bFlag=true;}
 else
 {
  // With today's round if it has not come yet, otherwise with the next.
  Pc.Day=LastRound>=Today?Today+1:Today;Pending.Add(Pc);
 }
 UE_LOG(LogTemp,Display,TEXT("S47: mail: %s %s, day %d"),*Pc.Id,bNow?TEXT("in the box"):TEXT("posted"),Pc.Day);
 if(M)M->RequestAutosave();
 return true;
}
bool FS47Mail::PostWard(const FString& TrayId,bool bNow)
{
 if(!Live())return false;
 FPiece Pc;Pc.Id=TEXT("ward:")+TrayId;Pc.Kind=TEXT("ward");Pc.Ref=TrayId;
 Post(Pc,bNow);
 return true;
}
void FS47Mail::PostWeekStatement(int32 Week)
{
 if(!Live())return;
 FPiece Pc;Pc.Id=FString::Printf(TEXT("bill:week-%d"),Week);Pc.Kind=TEXT("bill");Pc.Ref=FString::Printf(TEXT("week-review-%d"),Week);
 Post(Pc);
}
void FS47Mail::PostTowBill(int32 Fee,bool bPaidInCash)
{
 if(!Live()||!M||!M->Freeform)return;
 const int32 Today=M->Freeform->Day();
 FPiece Pc;Pc.Id=FString::Printf(TEXT("bill:tow-%d-%d"),Today,Rounds);Pc.Kind=TEXT("bill");
 Pc.Text=FString::Printf(TEXT("CALICHE COUNTY ROAD DEPARTMENT\nWRECKER SERVICE / STATEMENT\n\nBILL TO: SARO, DISTRICT SERVICE TRUCK\nDATE OF SERVICE: %s\n\nTOW TO THE STATION, HOOK-UP AND MILEAGE   $%d\n\n%s\n\nKeep the truck on the road and we will both be happier."),
  *DayText(Today).ToUpper(),Fee,bPaidInCash?TEXT("PAID FROM THE STATION BUDGET. THANK YOU."):TEXT("NOT PAID: CARRIED TO THE STATION'S WEEKLY SHEET."));
 Post(Pc);
}
bool FS47Mail::Holds(const FString& Id) const{return Box.ContainsByPredicate([&](const FPiece& Pc){return Pc.Id==Id;});}
bool FS47Mail::Coming(const FString& Id) const{return Pending.ContainsByPredicate([&](const FPiece& Pc){return Pc.Id==Id;});}

// ---------- the paper ----------
TArray<S47Ledger::FFact> FS47Mail::WeekFacts(int32 FromDay,int32 ToDay) const
{
 using S47Ledger::ETopic;
 TArray<S47Ledger::FFact> Out;
 if(!M||!M->Freeform)return Out;
 const auto& Ses=*M->Freeform;
 // How a record would reach the town: by its kind (FS47Happenings, the Director's observations, the marks left at a place).
 struct FKind{const TCHAR* Prefix;ETopic Topic;};
 static const FKind KindNames[]={
  {TEXT("missing_reported_d"),ETopic::Called},{TEXT("missing_d"),ETopic::Missing},{TEXT("carcass_d"),ETopic::Carcass},
  {TEXT("cattle_ring_d"),ETopic::Cattle},{TEXT("animals_flee_d"),ETopic::Cattle},
  {TEXT("lights_trace_"),ETopic::Landing},{TEXT("farm_trace"),ETopic::Landing},{TEXT("geiger_landing_"),ETopic::Landing},{TEXT("geiger_trace_"),ETopic::Landing},
  {TEXT("landing_seen_d"),ETopic::Lights},{TEXT("landing_logged_d"),ETopic::Lights},{TEXT("observation."),ETopic::Lights},{TEXT("house_lit_d"),ETopic::Lights},{TEXT("car_light_d"),ETopic::Lights},
  {TEXT("car_dies_d"),ETopic::Stalled},{TEXT("clock_ahead_d"),ETopic::Stalled},{TEXT("blackout_d"),ETopic::Outage},
  {TEXT("cb_empty_d"),ETopic::EmptyRig},{TEXT("rv_d"),ETopic::Camper},{TEXT("hitcher_d"),ETopic::Walker},{TEXT("cb_helped_d"),ETopic::Helped}};
 // Where: the place on the night's list something happened at that night, never the station's own ground if there is another.
 const auto PlaceOn=[&](int32 Night,FString& OutTitle,FString& OutId)
 {
  OutTitle.Empty();OutId.Empty();if(!Ses.EventPlaces)return;
  for(const FS47EventPlace& Pc:Ses.EventPlaces->All())if(Ses.EventPlaces->LastNight(Pc.Id)==Night)
  {
   const bool bStation=Pc.Region==TEXT("saro");
   if(!OutTitle.IsEmpty()&&bStation)continue;
   OutTitle=Pc.Title;OutId=FS47Places::Def(Pc.Id)?Pc.Id:FString();
   if(!bStation)break;
  }
 };
 const auto NightWords=[&](int32 OnDay){return FString(S47MailLocal::WeekNames[int32(Ses.DateOfDay(OnDay).GetDayOfWeek())])+TEXT(" night");};
 TSet<int32> NightsTold;
 for(const FString& Id:Ses.FindingIdsFrom(FromDay))
 {
  const int32 OnDay=Ses.FindingDay(Id);if(OnDay<FromDay||OnDay>ToDay)continue;
  const FKind* K=nullptr;for(const FKind& Each:KindNames)if(Id.StartsWith(Each.Prefix)){K=&Each;break;}
  if(!K)continue;
  S47Ledger::FFact Fact;Fact.Topic=K->Topic;Fact.Day=OnDay;Fact.Night=NightWords(OnDay);PlaceOn(OnDay,Fact.Place,Fact.PlaceId);
  Out.Add(Fact);NightsTold.Add(OnDay);
 }
 // What the town heard of that the player never saw: talk.
 if(Ses.EventPlaces)for(const FS47EventPlace& Pc:Ses.EventPlaces->All())
 {
  const int32 Night=Ses.EventPlaces->LastNight(Pc.Id);
  if(Night<FromDay||Night>ToDay||NightsTold.Contains(Night)||Pc.Region==TEXT("saro"))continue;
  S47Ledger::FFact Fact;Fact.Topic=ETopic::Rumor;Fact.Day=Night;Fact.Night=NightWords(Night);Fact.Place=Pc.Title;Fact.PlaceId=FS47Places::Def(Pc.Id)?Pc.Id:FString();
  Out.Add(Fact);NightsTold.Add(Night);
 }
 return Out;
}
void FS47Mail::PrintLedger(int32 OnDay)
{
 if(!M||!M->Freeform)return;
 auto& Ses=*M->Freeform;
 const int32 No=Issues.Num()+1;
 const int32 FromDay=Issues.IsEmpty()?1:Issues.Last().Day;
 const TArray<S47Ledger::FFact> Facts=WeekFacts(FromDay,OnDay);
 FString UnknownId,UnknownTitle;
 if(Ses.Places){UnknownId=Ses.Places->NextUnknown(No*13+5);if(const FS47Places::FDef* D=FS47Places::Def(UnknownId))UnknownTitle=D->Title;else UnknownId.Empty();}
 const TArray<S47Ledger::FStory> Front=S47Ledger::Front(Ses.Seed(),No,Facts,UnknownId,UnknownTitle);
 FKept K;K.No=No;K.Day=OnDay;K.bThanks=Facts.ContainsByPredicate([](const S47Ledger::FFact& Fact){return Fact.Topic==S47Ledger::ETopic::Helped;});
 for(const S47Ledger::FStory& St:Front){K.Heads.Add(St.Head);K.Bodies.Add(St.Body);K.Leads.Add(St.Lead);}
 Issues.Add(K);
 FPiece Pc;Pc.Id=FString::Printf(TEXT("ledger:%d"),No);Pc.Kind=TEXT("ledger");Pc.Ref=FString::FromInt(No);Pc.Day=OnDay;
 Box.Add(Pc);Posted.Add(Pc.Id);
 UE_LOG(LogTemp,Display,TEXT("S47: mail: the Ledger no. %d printed, day %d: %d facts from day %d, front \"%s\" / \"%s\""),No,OnDay,Facts.Num(),FromDay,Front.Num()>0?*Front[0].Head:TEXT(""),Front.Num()>1?*Front[1].Head:TEXT(""));
}
bool FS47Mail::IssueOf(int32 No,S47Ledger::FIssue& Out) const
{
 if(!M||!M->Freeform)return false;
 const FKept* K=Issues.FindByPredicate([No](const FKept& Q){return Q.No==No;});if(!K)return false;
 Out=S47Ledger::FIssue();Out.No=K->No;Out.Day=K->Day;Out.Date=M->Freeform->DateOfDay(K->Day);
 for(int32 I=0;I<K->Heads.Num();++I)Out.Front.Add({K->Heads[I],K->Bodies[I],K->Leads[I]});
 S47Ledger::Fill(Out,M->Freeform->Seed(),K->bThanks);
 return true;
}
void FS47Mail::OpenLedger(int32 No)
{
 if(!M||!M->UI||!M->Freeform)return;
 auto& Ses=*M->Freeform;
 S47Ledger::FIssue Issue;if(!IssueOf(No,Issue))return;
 const FString ReadKey=FString::Printf(TEXT("ledger:%d"),No);
 if(!ReadIssues.Contains(ReadKey))
 {
  // The first reading: what the paper mentions goes on the map in pencil, and wagon 23 with it.
  ReadIssues.Add(ReadKey);int32 Marked=0;
  TArray<FString> Leads;for(const S47Ledger::FStory& St:Issue.Front)Leads.Add(St.Lead);for(const S47Ledger::FNote& Cnt:Issue.Letters)Leads.Add(Cnt.Lead);
  for(const FString& Lead:Leads)
  {
   if(Lead.IsEmpty())continue;
   if(Lead==TEXT("wagon")){if(Ses.Predecessor&&!Ses.Predecessor->Heard(Lead)&&!Ses.Predecessor->Found(Lead)){Ses.Predecessor->HearOf(Lead);++Marked;}}
   else if(Ses.Places&&FS47Places::Def(Lead)&&!Ses.Places->Heard(Lead)&&!Ses.Places->Found(Lead)){Ses.Places->Hear(Lead);++Marked;}
  }
  if(Marked>0)M->UI->Toast(TEXT("You pencil what the paper mentions onto your map (TAB)."),5);
  UE_LOG(LogTemp,Display,TEXT("S47: mail: the Ledger no. %d read, %d places marked"),No,Marked);
  M->RequestAutosave();
 }
 if(M->Story)
 {
  FS47Doc D;D.Id=TEXT("mail-")+ReadKey;D.Title=FString::Printf(TEXT("The Caliche County Ledger, %s"),*DayText(Issue.Day));D.Kind=TEXT("typed");D.Stamp=TEXT("THE LEDGER");
  D.Page=S47Ledger::PlainText(Issue);D.Transcript=D.Page;M->Story->AddDoc(D);
 }
 TArray<S47Kit::FW> Panel;
 Panel.Add(S47Ledger::Page(Issue));
 const TWeakPtr<uint8> Guard=Life;const int32 Until=Issue.Day+6;
 for(const S47Ledger::FAd& Ad:Issue.Ads)
 {
  const S47Ledger::FCoupon* Cp=Ad.Coupon.IsEmpty()?nullptr:S47Ledger::Coupon(Ad.Coupon);if(!Cp)continue;
  const FString Id=Cp->Id,Line=Cp->Line,Through=DayText(Until);
  const bool bValid=Ses.Day()<=Until;
  Panel.Add(S47Kit::ButtonLive(TAttribute<FString>::CreateLambda([this,Guard,Id,Line,Through,bValid]
  {
   if(!bValid||!Guard.IsValid())return FString::Printf(TEXT("Coupon: %s. It ran out on %s."),*Line,*Through);
   return Clipped(Id)?FString::Printf(TEXT("Clipped: %s, good through %s"),*Line,*Through):FString::Printf(TEXT("Clip the coupon: %s, good through %s"),*Line,*Through);
  }),[this,Guard,Id,Until]{if(Guard.IsValid()&&!Clipped(Id))Clip(Id,Until);},false,TAttribute<bool>::CreateLambda([this,Guard,Id,bValid]{return bValid&&Guard.IsValid()&&!Clipped(Id);})));
 }
 Panel.Add(S47Kit::Button(TEXT("Fold the paper"),[this]{if(M&&M->UI)M->UI->Close();},true));
 if(M->Audio)M->Audio->Sfx(TEXT("paper0"),FS47Sound{.Gain=.5f});
 M->UI->Open(S47Kit::Shell(M->UI,FString::Printf(TEXT("THE LEDGER / %s"),*S47Ledger::DateLine(Issue.Date)),S47Kit::Col(Panel),1120));
}
void FS47Mail::Clip(const FString& CouponId,int32 Until)
{
 const S47Ledger::FCoupon* Cp=S47Ledger::Coupon(CouponId);if(!Cp)return;
 Coupons.RemoveAll([&](const FClip& Q){return Q.Id==CouponId;});
 FClip Mine;Mine.Id=Cp->Id;Mine.What=Cp->What;Mine.Price=Cp->Price;Mine.Until=Until;Coupons.Add(Mine);
 if(M&&M->Audio)M->Audio->Sfx(TEXT("paper1"),FS47Sound{.Gain=.5f});
 UE_LOG(LogTemp,Display,TEXT("S47: mail: coupon %s clipped, good through day %d"),*CouponId,Until);
 if(M)M->RequestAutosave();
}
bool FS47Mail::Clipped(const FString& CouponId) const
{
 const int32 Today=M&&M->Freeform?M->Freeform->Day():1;
 return Coupons.ContainsByPredicate([&](const FClip& Cp){return Cp.Id==CouponId&&Cp.Until>=Today;});
}
int32 FS47Mail::CounterPrice(const FString& Item,int32 Usual) const
{
 if(!Live())return Usual;
 const int32 Today=M->Freeform->Day();int32 Best=Usual;
 for(const FClip& Cp:Coupons)if(Cp.What==Item&&Cp.Until>=Today)Best=FMath::Min(Best,Cp.Price);
 return Best;
}
int32 FS47Mail::MotelPrice(int32 Usual) const{return CounterPrice(TEXT("motel"),Usual);}

// ---------- the box ----------
FString FS47Mail::DayText(int32 ShiftDay) const
{
 namespace L=S47MailLocal;
 if(!M||!M->Freeform)return FString();
 const FDateTime D=M->Freeform->DateOfDay(FMath::Max(1,ShiftDay));
 return FString::Printf(TEXT("%s, %s %d"),L::WeekNames[int32(D.GetDayOfWeek())],L::MonthNames[D.GetMonth()-1],D.GetDay());
}
FString FS47Mail::PieceTitle(const FPiece& Pc) const
{
 if(Pc.Kind==TEXT("ward"))return M&&M->Freeform&&M->Freeform->Predecessor?M->Freeform->Predecessor->PaperTitle(Pc.Ref):FString(TEXT("A letter from Dr. Ward's office"));
 if(Pc.Kind==TEXT("ledger"))return FString::Printf(TEXT("The Caliche County Ledger, %s"),*DayText(Pc.Day));
 if(Pc.Kind==TEXT("catalog"))return TEXT("Western Surplus & Radio Supply, spring catalog");
 if(Pc.Kind==TEXT("parcel"))return TEXT("A parcel from Western Surplus, Albuquerque");
 if(Pc.Kind==TEXT("bill"))return Pc.Ref.StartsWith(TEXT("week-review-"))?FString::Printf(TEXT("The station's weekly statement, week %s"),*Pc.Ref.Mid(12)):FString(TEXT("A bill from the county road department"));
 return Pc.Ref==TEXT("2")?TEXT("A postcard"):TEXT("An envelope with no stamp");
}
FString FS47Mail::PieceText(const FPiece& Pc) const
{
 if(Pc.Kind==TEXT("bill")&&Pc.Text.IsEmpty()&&M&&M->Freeform)
 {
  const FString Sheet=M->Freeform->FindingText(Pc.Ref);
  return Sheet.IsEmpty()?FString(TEXT("A statement from the district office. The figures are the ones on the OPS sheet.")):TEXT("DISTRICT OFFICE / STATEMENT OF ACCOUNT\nCOPY FOR THE NIGHT OPERATOR\n\n")+Sheet;
 }
 return Pc.Text;
}
void FS47Mail::FileDoc(const FPiece& Pc)
{
 if(!M||!M->Story||Pc.Kind==TEXT("ward")||Pc.Kind==TEXT("ledger")||Pc.Kind==TEXT("catalog"))return;
 FS47Doc D;D.Id=TEXT("mail-")+Pc.Id;D.Title=PieceTitle(Pc);D.Kind=Pc.Kind==TEXT("nobody")?TEXT("hand"):TEXT("typed");D.Stamp=Pc.Kind==TEXT("nobody")?FString():FString(TEXT("SARO / MAIL"));
 D.Page=PieceText(Pc);D.Transcript=D.Page;M->Story->AddDoc(D);
}
TArray<FS47Mail::FPiece> FS47Mail::TakeOut()
{
 TArray<FPiece> Out;
 if(!M||!M->Freeform)return Out;
 auto& Ses=*M->Freeform;
 // Everything but a parcel comes out at once; a parcel wants both hands and waits for them.
 for(int32 I=0;I<Box.Num();)
 {
  const FPiece Pc=Box[I];
  if(Pc.Kind==TEXT("parcel")){++I;continue;}
  Box.RemoveAt(I);Fetched.Add(Pc);Out.Add(Pc);
  if(Pc.Kind==TEXT("ward")&&Ses.Predecessor)Ses.Predecessor->FromMail(Pc.Ref);
  if(Pc.Kind==TEXT("catalog"))bCatalog=true;
  FileDoc(Pc);
 }
 // Only the last few stay on the desk to be read again; the rest are filed where they belong.
 while(Fetched.Num()>12)Fetched.RemoveAt(0);
 bFlag=!Box.IsEmpty()||Orders.ContainsByPredicate([](const FOrder& O){return O.Picked==0;});
 if(!Out.IsEmpty())
 {
  if(M->Audio){M->Audio->Sfx(TEXT("metalTap"),FS47Sound{.Gain=.55f});M->Audio->Sfx(TEXT("paper0"),FS47Sound{.Gain=.45f,.When=.35f});}
  UE_LOG(LogTemp,Display,TEXT("S47: mail: %d taken out of the box, day %d"),Out.Num(),Ses.Day());
  M->RequestAutosave();
 }
 return Out;
}
void FS47Mail::ReadPiece(const FPiece& Pc)
{
 if(!M||!M->UI||!M->Freeform)return;
 auto& Ses=*M->Freeform;
 if(Pc.Kind==TEXT("ward")){if(Ses.Predecessor){if(Pc.Ref==TEXT("assessment"))Ses.Predecessor->OpenAssessment();else Ses.Predecessor->ReadDoc(Pc.Ref);}return;}
 if(Pc.Kind==TEXT("ledger")){OpenLedger(FCString::Atoi(*Pc.Ref));return;}
 if(Pc.Kind==TEXT("catalog")){OpenCatalog();return;}
 FS47Doc D;D.Id=TEXT("mail-")+Pc.Id;D.Title=PieceTitle(Pc);D.Kind=Pc.Kind==TEXT("nobody")?TEXT("hand"):TEXT("typed");D.Stamp=Pc.Kind==TEXT("nobody")?FString():FString(TEXT("SARO / MAIL"));
 D.Page=PieceText(Pc);D.Transcript=D.Page;
 M->UI->Document(D);
}
bool FS47Mail::LiftParcel(const FString& Id)
{
 namespace L=S47MailLocal;
 if(!M||!M->Freeform||!M->Freeform->Field||!Model)return false;
 auto& Ses=*M->Freeform;
 const int32 At=Box.IndexOfByPredicate([&](const FPiece& Pc){return Pc.Id==Id&&Pc.Kind==TEXT("parcel");});if(At<0)return false;
 const FPiece Pc=Box[At];
 TMap<FString,int32> Stock;TArray<FString> Gear;
 for(const auto& G:Pc.Goods){const FString Kit=L::KitOf(G.Key);if(Kit.IsEmpty())Stock.Add(G.Key,G.Value);else Gear.Add(Kit);}
 // The supplies in it travel as any parcel does: in both hands, to the truck or GARAGE RECEIVING.
 if(!Stock.IsEmpty()&&!Ses.Field->TakePostedParcel(Stock,Model->Lid()))
 {
  if(M->UI)M->UI->Toast(TEXT("Your hands are full. The parcel stays in the box until they are free."),5);
  return false;
 }
 for(const FString& Kit:Gear)Ses.ReceiveKit(Kit);
 Box.RemoveAt(At);Fetched.Add(Pc);FileDoc(Pc);
 bFlag=!Box.IsEmpty()||Orders.ContainsByPredicate([](const FOrder& O){return O.Picked==0;});
 if(M->Audio)M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.5f});
 UE_LOG(LogTemp,Display,TEXT("S47: mail: parcel %s lifted out: %d supplies in hand, %d pieces of gear signed for"),*Id,Stock.Num(),Gear.Num());
 if(M->UI)
 {
  M->UI->Close();
  M->UI->Toast(Stock.IsEmpty()?FString(TEXT("Signed for. The gear is the station's now: see OPS, SUPPLIES.")):FString(TEXT("The parcel is in your hands. GARAGE RECEIVING unpacks it into stock, or load it on the truck.")),6);
 }
 M->RequestAutosave();
 return true;
}
void FS47Mail::OpenBox()
{
 if(!M||!M->UI||!M->Freeform||!Live())return;
 const TArray<FPiece> Came=TakeOut();
 TArray<S47Kit::FW> Panel;
 const TWeakPtr<uint8> Guard=Life;
 Panel.Add(S47Kit::Text(TEXT("A rural box on a weathered four-by-four, galvanized and gone dull, SARO stenciled on the side. The door drops on its hinge.")));
 const auto ReadButton=[&](const FPiece& Pc,bool bPrimary)
 {
  const FPiece Copy=Pc;
  return S47Kit::Button(TEXT("Read: ")+PieceTitle(Pc),[this,Guard,Copy]{if(Guard.IsValid())ReadPiece(Copy);},bPrimary);
 };
 if(!Came.IsEmpty())
 {
  Panel.Add(S47Kit::Section(TEXT("WHAT CAME")));
  for(int32 I=0;I<Came.Num();++I)Panel.Add(ReadButton(Came[I],I==0));
 }
 bool bParcelShown=false;
 for(const FPiece& Pc:Box)if(Pc.Kind==TEXT("parcel"))
 {
  if(!bParcelShown){Panel.Add(S47Kit::Section(TEXT("IN THE BOX")));bParcelShown=true;}
  Panel.Add(S47Kit::Paper(Pc.Text));
  const FString Id=Pc.Id;
  Panel.Add(S47Kit::Button(TEXT("Lift the parcel out"),[this,Guard,Id]{if(Guard.IsValid())LiftParcel(Id);},true));
 }
 if(Came.IsEmpty()&&Box.IsEmpty())Panel.Add(S47Kit::Small(TEXT("Nothing in it. The carrier comes up the highway around noon, Monday through Saturday.")));
 // An order waiting for the carrier, and what is on its way.
 for(const FOrder& Ord:Orders)
 {
  if(Ord.Picked==0)
  {
   Panel.Add(S47Kit::Small(FString::Printf(TEXT("Your order to Western Surplus is in the box with a money order for $%d, the flag up for the carrier."),Ord.Paid)));
   Panel.Add(S47Kit::Button(TEXT("Take the order back out"),[this,Guard]{if(Guard.IsValid()){TakeOrderBack();OpenBox();}}));
  }
  else Panel.Add(S47Kit::Small(FString::Printf(TEXT("On its way from Albuquerque: order %d, due about %s."),Ord.Serial,*DayText(Ord.Due))));
 }
 if(bCatalog)Panel.Add(S47Kit::Button(TEXT("The Western Surplus catalog / order by mail"),[this,Guard]{if(Guard.IsValid())OpenCatalog();}));
 // Papers kept from before, to read again: the newest few.
 {
  TArray<S47Kit::FW> Again;
  for(int32 I=Fetched.Num()-1-Came.Num();I>=0&&Again.Num()<4;--I)
  {
   const FPiece& Pc=Fetched[I];
   if(Pc.Kind==TEXT("ward")||Pc.Kind==TEXT("catalog")||Pc.Kind==TEXT("parcel"))continue;
   Again.Add(ReadButton(Pc,false));
  }
  if(!Again.IsEmpty()){Panel.Add(S47Kit::Section(TEXT("KEPT")));Panel.Append(Again);}
 }
 Panel.Add(S47Kit::Button(TEXT("Shut the box"),[this]{if(M&&M->UI)M->UI->Close();}));
 if(Came.IsEmpty()&&M->Audio)M->Audio->Sfx(TEXT("metalTap"),FS47Sound{.Gain=.4f});
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("THE MAILBOX AT THE ROAD"),S47Kit::Col(Panel),820));
}

// ---------- the catalog and its orders ----------
bool FS47Mail::Offered(const FLine& Line) const
{
 if(!M||!M->Freeform)return false;
 const FString Kit=S47MailLocal::KitOf(Line.Id);
 if(Kit.IsEmpty())return true;
 // The gear OPS sells, once OPS would sell it, and not twice.
 const bool bOrdered=Orders.ContainsByPredicate([&](const FOrder& O){return O.Goods.Contains(Line.Id);})||Box.ContainsByPredicate([&](const FPiece& Pc){return Pc.Goods.Contains(Line.Id);});
 return !M->Freeform->HasKit(Kit)&&M->Freeform->KitWaits(Kit).IsEmpty()&&!bOrdered;
}
int32 FS47Mail::LinePrice(const FLine& Line) const
{
 const int32 Today=M&&M->Freeform?M->Freeform->Day():1;int32 Best=Line.Price;
 for(const FClip& Cp:Coupons)if(Cp.What==FString(TEXT("mail:"))+Line.Id&&Cp.Until>=Today)Best=FMath::Min(Best,Cp.Price);
 return Best;
}
int32 FS47Mail::OrderTotal(const TMap<FString,int32>& Lines,int32& OutPostage) const
{
 int32 Sum=0;
 for(const auto& Pair:Lines)if(const FLine* L=Catalog().FindByPredicate([&](const FLine& Q){return Pair.Key==Q.Id;}))Sum+=LinePrice(*L)*Pair.Value;
 const int32 Today=M&&M->Freeform?M->Freeform->Day():1;
 OutPostage=Coupons.ContainsByPredicate([&](const FClip& Cp){return Cp.What==TEXT("post")&&Cp.Until>=Today;})?0:Postage;
 return Lines.IsEmpty()?0:Sum+OutPostage;
}
bool FS47Mail::WriteOrder(const TMap<FString,int32>& Lines,FString& Why)
{
 if(!M||!M->Freeform||!Live()){Why=TEXT("no post in this game");return false;}
 auto& Ses=*M->Freeform;
 if(Lines.IsEmpty()){Why=TEXT("nothing on the form");return false;}
 if(Orders.ContainsByPredicate([](const FOrder& O){return O.Picked==0;})){Why=TEXT("one order is in the box already");return false;}
 int32 Count=0;
 for(const auto& Pair:Lines)
 {
  const FLine* L=Catalog().FindByPredicate([&](const FLine& Q){return Pair.Key==Q.Id;});
  if(!L||!Offered(*L)||Pair.Value<1||Pair.Value>(S47MailLocal::KitOf(L->Id).IsEmpty()?MostPerLine:1)){Why=TEXT("a line the catalog does not have");return false;}
  Count+=Pair.Value;
 }
 if(Count>MostPerOrder){Why=FString::Printf(TEXT("more than %d things on one form"),MostPerOrder);return false;}
 int32 Stamp=0;const int32 Total=OrderTotal(Lines,Stamp);
 if(Ses.Budget()<Total){Why=FString::Printf(TEXT("the money order would be $%d, and the budget has $%d"),Total,Ses.Budget());return false;}
 if(!Ses.Spend(Total)){Why=Ses.FeedbackText();return false;}
 FOrder Ord;Ord.Serial=NextSerial++;Ord.Paid=Total;Ord.Goods=Lines;Orders.Add(Ord);
 bFlag=true;Cart.Reset();
 if(M->Audio){M->Audio->Sfx(TEXT("paper1"),FS47Sound{.Gain=.5f});M->Audio->Sfx(TEXT("click"),FS47Sound{.Gain=.45f,.When=.6f});}
 UE_LOG(LogTemp,Display,TEXT("S47: mail: order %d written, $%d (postage $%d), %d things; flag up"),Ord.Serial,Total,Stamp,Count);
 M->RequestAutosave();
 return true;
}
void FS47Mail::TakeOrderBack()
{
 if(!M||!M->Freeform)return;
 const int32 At=Orders.IndexOfByPredicate([](const FOrder& O){return O.Picked==0;});if(At<0)return;
 const FOrder Ord=Orders[At];Orders.RemoveAt(At);
 M->Freeform->Earn(Ord.Paid);
 bFlag=!Box.IsEmpty();
 UE_LOG(LogTemp,Display,TEXT("S47: mail: order %d taken back, $%d back"),Ord.Serial,Ord.Paid);
 M->RequestAutosave();
}
void FS47Mail::OpenCatalog()
{
 if(!M||!M->UI||!M->Freeform)return;
 auto& Ses=*M->Freeform;
 const TWeakPtr<uint8> Guard=Life;
 TArray<S47Kit::FW> Panel;
 Panel.Add(S47Kit::Paper(TEXT("WESTERN SURPLUS & RADIO SUPPLY\nP.O. BOX 2090, ALBUQUERQUE, NEW MEXICO\nSPRING 1986\n\nParts for radio, instrument and field work, tested where it says so. Send the order form with a money order; we ship the day it comes in. $2 postage on any order. Rural routes, allow one to three days.")));
 const bool bWaiting=Orders.ContainsByPredicate([](const FOrder& O){return O.Picked==0;});
 for(const FLine& Line:Catalog())
 {
  if(!Offered(Line))continue;
  const FString Id=Line.Id,Kit=S47MailLocal::KitOf(Id);
  const int32 Here=LinePrice(Line);
  const int32 There=Kit.IsEmpty()?FS47FreeformWorld::ShopPrice(Id):Ses.KitPrice(Kit);
  const int32 Qty=Cart.FindRef(Id);
  const FString Detail=Kit.IsEmpty()?FString::Printf(TEXT("$%d in Caliche"),There):FString::Printf(TEXT("$%d at OPS, SUPPLIES"),There);
  Panel.Add(S47Kit::Status(Line.Title,Qty>0?FString::Printf(TEXT("%d x $%d"),Qty,Here):FString::Printf(TEXT("$%d"),Here),Qty>0?1:0,Detail));
  const int32 Most=Kit.IsEmpty()?MostPerLine:1;
  Panel.Add(S47Kit::Row({
   S47Kit::Button(TEXT("Add one"),[this,Guard,Id]{if(Guard.IsValid()){++Cart.FindOrAdd(Id);OpenCatalog();}},false,!bWaiting&&Qty<Most),
   S47Kit::Button(TEXT("Take one off"),[this,Guard,Id]{if(Guard.IsValid()){if(int32* Cnt=Cart.Find(Id)){if(--*Cnt<=0)Cart.Remove(Id);}OpenCatalog();}},false,Qty>0)}));
 }
 int32 Stamp=0;const int32 Total=OrderTotal(Cart,Stamp);
 if(!Cart.IsEmpty())Panel.Add(S47Kit::Status(TEXT("ORDER FORM"),FString::Printf(TEXT("$%d"),Total),0,Stamp>0?FString::Printf(TEXT("with $%d postage"),Stamp):FString(TEXT("postage paid with the coupon"))));
 if(bWaiting)Panel.Add(S47Kit::Small(TEXT("An order is in the box already, waiting for the carrier.")));
 Panel.Add(S47Kit::Button(FString::Printf(TEXT("Buy a money order for $%d and leave the order in the box, flag up"),Total),[this,Guard]
 {
  if(!Guard.IsValid())return;
  FString Why;
  if(WriteOrder(Cart,Why)){if(M&&M->UI){M->UI->Close();M->UI->Toast(TEXT("The order is in the box and the flag is up. The carrier takes it around noon."),5);}}
  else if(M&&M->UI)M->UI->Toast(TEXT("Not sent: ")+Why+TEXT("."),5);
 },true,!Cart.IsEmpty()&&!bWaiting&&Ses.Budget()>=Total));
 Panel.Add(S47Kit::Small(FString::Printf(TEXT("Station budget $%d."),Ses.Budget())));
 Panel.Add(S47Kit::Button(TEXT("Put the catalog back"),[this,Guard]{if(Guard.IsValid())OpenBox();}));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("WESTERN SURPLUS / ORDER BY MAIL"),S47Kit::Col(Panel),860));
}
FString FS47Mail::SlipText(int32 Serial,const TMap<FString,int32>& Goods,int32 Paid)
{
 FString Lines;
 for(const FLine& Line:Catalog())if(const int32* Cnt=Goods.Find(Line.Id))Lines+=FString::Printf(TEXT("%d x %s\n"),*Cnt,Line.Title);
 return FString::Printf(TEXT("WESTERN SURPLUS & RADIO SUPPLY / PACKING SLIP\nORDER %d / SHIP TO: NIGHT OPERATOR, SARO\n\n%s\nPAID BY MONEY ORDER, $%d. THANK YOU."),Serial,*Lines,Paid);
}
FString FS47Mail::NobodyText(int32 Variant,const FString& Stamped)
{
 if(Variant==1)return FString::Printf(TEXT("An envelope with no stamp and no postmark. Inside, a black and white print, curled, gone yellow at the edges the way old prints go in a drawer: SARO from the highway, the building, the dishes pointed at the sky, the truck pad empty. Nobody in it.\n\nOn the back, in the purple ink of a photo lab's stamp: PROCESSED %s.\n\nThat date has not come yet."),*Stamped);
 if(Variant==2)return TEXT("A picture postcard of the Plaza in Caliche, the marquee lit, the colors gone soft. No stamp, no postmark. The message side is blank. The address side says SARO, in pencil, and nothing else.");
 return TEXT("A plain white envelope: no stamp, no postmark, no return address. NIGHT OPERATOR, SARO, in pencil, in capitals.\n\nInside, one sheet of typing paper folded in three. Nothing on it. Held up to the light, nothing on it either.");
}

// ---------- what the station is told ----------
FString FS47Mail::TaskLine() const
{
 if(!Live())return FString();
 if(Box.ContainsByPredicate([](const FPiece& Pc){return Pc.Kind==TEXT("parcel");}))return TEXT("MAILBOX / A parcel waits in the box at the road. Lift it out and carry it to GARAGE RECEIVING.");
 if(Orders.ContainsByPredicate([](const FOrder& O){return O.Picked==0;}))return TEXT("MAILBOX / Your order is in the box at the road with the flag up, for the carrier around noon.");
 if(!Orders.IsEmpty())return FString::Printf(TEXT("MAIL ORDER / A parcel from Western Surplus, due about %s."),*DayText(Orders[0].Due));
 return FString();
}
bool FS47Mail::WorldSpot(FString& OutArea,double& OutX,double& OutZ) const
{
 OutArea=TEXT("saro");OutX=FS47MailBoxModel::PostX-1.25;OutZ=FS47MailBoxModel::PostZ+.15;
 return true;
}

// ---------- F10 and the tests ----------
void FS47Mail::TestOn(){bTest=true;Build();}
bool FS47Mail::TestRound(FString& Why)
{
 TestOn();if(!M||!M->Freeform){Why=TEXT("Freeform only");return false;}
 const int32 Today=M->Freeform->Day(),Before=Box.Num();
 LastRound=FMath::Max(LastRound,Today);
 Round(Today,true);
 Why=FString::Printf(TEXT("The carrier came: %d new in the box, %d in it now, the flag %s."),Box.Num()-Before,Box.Num(),bFlag?TEXT("up"):TEXT("down"));
 return true;
}
void FS47Mail::TestFlag(bool bUp){TestOn();bFlag=bUp;if(Model)Model->SetFlag(bUp);}
bool FS47Mail::TestFetch(){TestOn();const int32 Before=Box.Num();OpenBox();return Before>0;}
void FS47Mail::TestLedger()
{
 TestOn();if(!M||!M->Freeform||!M->UI)return;
 auto& Ses=*M->Freeform;
 const int32 No=Issues.Num()+1,FromDay=Issues.IsEmpty()?1:Issues.Last().Day;
 FString UnknownId,UnknownTitle;
 if(Ses.Places){UnknownId=Ses.Places->NextUnknown(No*13+5);if(const FS47Places::FDef* D=FS47Places::Def(UnknownId))UnknownTitle=D->Title;else UnknownId.Empty();}
 const TArray<S47Ledger::FFact> Facts=WeekFacts(FromDay,Ses.Day());
 S47Ledger::FIssue Issue;Issue.No=No;Issue.Day=Ses.Day();Issue.Date=Ses.DateOfDay(Ses.Day());
 Issue.Front=S47Ledger::Front(Ses.Seed(),No,Facts,UnknownId,UnknownTitle);
 S47Ledger::Fill(Issue,Ses.Seed(),Facts.ContainsByPredicate([](const S47Ledger::FFact& Fact){return Fact.Topic==S47Ledger::ETopic::Helped;}));
 UE_LOG(LogTemp,Display,TEXT("S47: mail: test: this week's Ledger from %d facts since day %d"),Facts.Num(),FromDay);
 TArray<S47Kit::FW> Panel;Panel.Add(S47Ledger::Page(Issue));Panel.Add(S47Kit::Button(TEXT("Fold the paper"),[this]{if(M&&M->UI)M->UI->Close();},true));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("THE LEDGER / AS IT WOULD BE PRINTED TODAY"),S47Kit::Col(Panel),1120));
}
void FS47Mail::TestParcel()
{
 TestOn();
 const int32 Serial=NextSerial++;TMap<FString,int32> Goods;Goods.Add(TEXT("cell"),2);
 FPiece Pc;Pc.Id=FString::Printf(TEXT("parcel:%d"),Serial);Pc.Kind=TEXT("parcel");Pc.Ref=FString::FromInt(Serial);Pc.Goods=Goods;Pc.Text=SlipText(Serial,Goods,8);
 Pc.Day=M&&M->Freeform?M->Freeform->Day():1;Box.Add(Pc);Posted.Add(Pc.Id);bFlag=true;
}
void FS47Mail::TestNobody(int32 Variant)
{
 TestOn();
 const int32 V=FMath::Clamp(Variant,0,2),Today=M&&M->Freeform?M->Freeform->Day():1;
 FPiece Pc;Pc.Id=FString::Printf(TEXT("nobody:test-%d-%d"),V,Rounds+Box.Num());Pc.Kind=TEXT("nobody");Pc.Ref=FString::FromInt(V);Pc.Day=Today;Pc.Text=NobodyText(V,DayText(Today+9).ToUpper());
 Box.Add(Pc);Posted.Add(Pc.Id);bFlag=true;
}
bool FS47Mail::TestOrder(const TMap<FString,int32>& Lines,FString& Why){TestOn();return WriteOrder(Lines,Why);}
bool FS47Mail::TestLift()
{
 TestOn();
 const FPiece* First=Box.FindByPredicate([](const FPiece& Pc){return Pc.Kind==TEXT("parcel");});
 return First&&LiftParcel(FString(First->Id));
}

TArray<FString> FS47Mail::AllText()
{
 TArray<FString> Out;
 for(const FLine& Line:Catalog())Out.Add(Line.Title);
 for(int32 V=0;V<3;++V)Out.Add(NobodyText(V,TEXT("THURSDAY, MAY 15")));
 TMap<FString,int32> Goods;Goods.Add(TEXT("cell"),2);Goods.Add(TEXT("kit:geiger_tube"),1);Out.Add(SlipText(3,Goods,28));
 return Out;
}
bool FS47Mail::SelfTest(TArray<FString>& Lines)
{
 bool OK=S47Weather::SelfTest(Lines);
 OK&=S47Ledger::SelfTest(Lines);
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s mail: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 // The carrier keeps a rural route's week.
 Check(!RoundOn(FDateTime(1986,5,4))&&!RoundOn(FDateTime(1986,5,26))&&RoundOn(FDateTime(1986,5,1))&&RoundOn(FDateTime(1986,5,19))&&RoundOn(FDateTime(1986,4,28)),TEXT("no mail on Sundays or on Memorial Day, and every other day"));
 bool bNoon=true,bShip=true;
 for(int32 Cnt=1;Cnt<=40;++Cnt)
 {
  const double At=RoundClock(470047,Cnt)-double(Cnt-1)*86400.;
  bNoon&=At>=12*3600.&&At<=13.5*3600.&&RoundClock(470047,Cnt)==RoundClock(470047,Cnt);
  const int32 Ship=ShipRounds(470047,Cnt);bShip&=Ship>=1&&Ship<=3;
 }
 Check(bNoon,TEXT("the carrier comes between noon and half past one, the same for a seed and a day"));
 Check(bShip,TEXT("a parcel comes one to three rounds after the carrier takes the order"));
 // The catalog is cheaper than the counter and OPS, line by line, and sells what the station can use.
 bool bCheaper=true,bKnown=true;
 for(const FLine& Line:Catalog())
 {
  const FString Kit=S47MailLocal::KitOf(Line.Id);
  if(Kit.IsEmpty()){const int32 There=FS47FreeformWorld::ShopPrice(Line.Id);bKnown&=There>0;bCheaper&=Line.Price<There;}
  else{const FS47KitOffer* O=FS47Openings::Def(Kit);bKnown&=O!=nullptr;bCheaper&=O&&Line.Price<O->Price;}
 }
 Check(bKnown,TEXT("every line in the catalog is a supply of the counter or gear of OPS"));
 Check(bCheaper,TEXT("by mail every line is cheaper than in Caliche or at OPS, before postage"));
 bool bCoupons=true;
 for(const S47Ledger::FCoupon& Cp:S47Ledger::Coupons())
 {
  const FString What=Cp.What;
  if(What==TEXT("motel"))bCoupons&=Cp.Price<FS47FreeformSession::MotelRoom;
  else if(What==TEXT("post"))bCoupons&=Cp.Price<Postage;
  else if(What.StartsWith(TEXT("mail:"))){const FLine* L=Catalog().FindByPredicate([&](const FLine& Q){return What.Mid(5)==Q.Id;});bCoupons&=L&&Cp.Price<L->Price;}
  else{const int32 There=FS47FreeformWorld::ShopPrice(What);bCoupons&=There>0&&Cp.Price<There;}
 }
 Check(bCoupons,TEXT("every coupon in the paper takes something off a real price"));
 bool bWords=true;for(const FString& St:AllText()){FString Word;bWords&=S47Ledger::Clean(St,Word);}
 Check(bWords,TEXT("the catalog, the packing slip and what nobody sent say nothing they must not"));
 // A save from before the post finds the catalog in the box with the flag up.
 FS47Mail Fresh(nullptr);Fresh.Begin(nullptr,true);
 Check(Fresh.Holds(TEXT("catalog"))&&Fresh.FlagUp()&&Fresh.Ever(TEXT("catalog")),TEXT("a new game or an older save: the catalog is in the box and the flag is up"));
 // Everything survives a Continue.
 {
  FS47Mail A(nullptr);A.Begin(nullptr,false);
  FPiece Ward;Ward.Id=TEXT("ward:letter");Ward.Kind=TEXT("ward");Ward.Ref=TEXT("letter");A.Post(Ward,true);
  FPiece Sheet;Sheet.Id=TEXT("bill:week-1");Sheet.Kind=TEXT("bill");Sheet.Ref=TEXT("week-review-1");A.Post(Sheet,false);
  FPiece Box2;Box2.Id=TEXT("parcel:4");Box2.Kind=TEXT("parcel");Box2.Goods.Add(TEXT("cell"),3);Box2.Goods.Add(TEXT("kit:film"),1);Box2.Text=SlipText(4,Box2.Goods,30);A.Post(Box2,true);
  FOrder Ord;Ord.Serial=5;Ord.Paid=21;Ord.Picked=3;Ord.Due=5;Ord.Goods.Add(TEXT("relay"),1);A.Orders.Add(Ord);A.NextSerial=6;
  FClip Clipping;Clipping.Id=TEXT("cell3");Clipping.What=TEXT("cell");Clipping.Price=3;Clipping.Until=10;A.Coupons.Add(Clipping);
  FKept K;K.No=1;K.Day=4;K.bThanks=true;K.Heads={TEXT("A")};K.Bodies={TEXT("B")};K.Leads={TEXT("windmill")};A.Issues.Add(K);
  A.ReadIssues.Add(TEXT("ledger:1"));A.LastRound=4;A.Rounds=3;A.NobodySent=1;
  FS47Mail B(nullptr);B.Begin(S47J::Read(S47J::Write(A.Save())),true);
  const bool bBox=B.Holds(TEXT("ward:letter"))&&B.Holds(TEXT("parcel:4"))&&B.Holds(TEXT("catalog"))&&B.Coming(TEXT("bill:week-1"))&&B.FlagUp();
  const FPiece* Back=B.Box.FindByPredicate([](const FPiece& Pc){return Pc.Id==TEXT("parcel:4");});
  const bool bGoods=Back&&Back->Goods.FindRef(TEXT("cell"))==3&&Back->Goods.FindRef(TEXT("kit:film"))==1&&Back->Text.Contains(TEXT("PACKING SLIP"));
  const bool bRest=B.Orders.Num()==1&&B.Orders[0].Due==5&&B.Orders[0].Goods.FindRef(TEXT("relay"))==1&&B.NextSerial==6&&B.Coupons.Num()==1&&B.Coupons[0].Until==10
   &&B.Issues.Num()==1&&B.Issues[0].bThanks&&B.Issues[0].Leads[0]==TEXT("windmill")&&B.ReadIssues.Contains(TEXT("ledger:1"))&&B.LastRound==4&&B.Rounds==3&&B.NobodySent==1;
  Check(bBox&&bGoods&&bRest,TEXT("the box, the post on its way, orders, coupons, printed papers and the rounds survive a Continue"));
  Check(!B.Post(Ward,true),TEXT("nothing is posted twice, a Continue included"));
 }
 return OK;
}

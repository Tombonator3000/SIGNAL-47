#include "S47Mail.h"
#include "S47Audio.h"
#include "S47Balloon.h"
#include "S47Docs.h"
#include "S47EventPlaces.h"
#include "S47FieldCamera.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Image.h"
#include "S47Inter.h"
#include "S47Keys.h"
#include "S47Kit.h"
#include "S47Ledger.h"
#include "S47Legs.h"
#include "S47MailBox.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Openings.h"
#include "S47Places.h"
#include "S47Predecessor.h"
#include "S47RoadTerrain.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Taken.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Weather.h"
#include "S47Web.h"
#include "S47WebAudio.h"
#include "S47World.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "UObject/GarbageCollection.h"

namespace S47MailLocal
{
 const TCHAR* const BoxUse=TEXT("ff:mail:box");
 const TCHAR* const BoxRect=TEXT("ff:mailbox");
 // D45-24-20261110: kept among the read issues once the reader has been told a story can be clicked onto the map (no new save key).
 const TCHAR* const LedgerHint=TEXT("ledger:hint");
 FString LedgerHintText(){return FString::Printf(TEXT("A story in the paper names a place. Click it, or use the buttons under the page, to pencil it in on your map (%s)."),*S47Keys::MapSaid());}
 const TCHAR* const WeekNames[]={TEXT("Monday"),TEXT("Tuesday"),TEXT("Wednesday"),TEXT("Thursday"),TEXT("Friday"),TEXT("Saturday"),TEXT("Sunday")};
 const TCHAR* const MonthNames[]={TEXT("January"),TEXT("February"),TEXT("March"),TEXT("April"),TEXT("May"),TEXT("June"),TEXT("July"),TEXT("August"),TEXT("September"),TEXT("October"),TEXT("November"),TEXT("December")};
 const TCHAR* const Points[]={TEXT("north"),TEXT("northeast"),TEXT("east"),TEXT("southeast"),TEXT("south"),TEXT("southwest"),TEXT("west"),TEXT("northwest")};
 // The carrier's car (FS47MailCarModel): up the highway's northbound lane from the south end of SARO's stretch at a rural carrier's
 // pace, in to the edge of the road over the last stretch before the box, the driver's window (on the right, a little ahead of
 // the middle) at the box's door; then on north out of sight. Metres, metres a second, seconds; Lead and Late in the world's clock.
 constexpr double LaneX=S47Legs::SARO_HWY.X+1.8,CurbX=FS47MailBoxModel::PostX-.38-.42-FS47MailCarModel::Width*.5,StopZ=FS47MailBoxModel::PostZ+.5;
 constexpr double Cruise=21.,Brake=4.,Pull=2.6,Decide=75.,PullIn=70.,PullOut=40.,Dwell=6.5;
 constexpr double Lead=15*60.,Late=8*60.,Jumped=45*60.;
 double Draw(int32 Seed,int32 Which,uint32 Salt)
 {
  uint32 Hash=uint32(Seed)*2246822519u^uint32(Which)*3266489917u^(Salt+7u)*668265263u;
  Hash^=Hash>>15;Hash*=2654435761u;Hash^=Hash>>13;Hash*=2246822519u;Hash^=Hash>>16;
  return double(Hash)/4294967296.;
 }
 double Smooth(double T){const double K=FMath::Clamp(T,0.,1.);return K*K*(3.-2.*K);}
 S47J::FObj GoodsJson(const TMap<FString,int32>& Goods){auto O=S47J::New();for(const auto& G:Goods)S47J::Set(O,*G.Key,G.Value);return O;}
 TMap<FString,int32> GoodsOf(const S47J::FObj& O)
 {
  TMap<FString,int32> Got;if(!O.IsValid())return Got;
  for(const auto& Pair:O->Values){double Cnt=0;if(Pair.Value.IsValid()&&Pair.Value->TryGetNumber(Cnt)&&FMath::IsFinite(Cnt)&&Cnt>=1&&Cnt<=99)Got.Add(FString(*Pair.Key),int32(Cnt));}
  return Got;
 }
 S47J::FObj PieceJson(const FS47Mail::FPiece& Pc)
 {
  auto O=S47J::New();S47J::Set(O,TEXT("id"),Pc.Id);S47J::Set(O,TEXT("kind"),Pc.Kind);S47J::Set(O,TEXT("day"),Pc.Day);
  if(!Pc.Ref.IsEmpty())S47J::Set(O,TEXT("ref"),Pc.Ref);
  if(!Pc.Text.IsEmpty())S47J::Set(O,TEXT("text"),Pc.Text);
  if(!Pc.Place.IsEmpty())S47J::Set(O,TEXT("place"),Pc.Place);
  if(!Pc.Goods.IsEmpty())S47J::Set(O,TEXT("goods"),GoodsJson(Pc.Goods));
  return O;
 }
 bool PieceOf(const TSharedPtr<FJsonValue>& V,FS47Mail::FPiece& Out)
 {
  if(!V.IsValid()||V->Type!=EJson::Object)return false;const auto O=V->AsObject();if(!O.IsValid())return false;
  Out.Id=S47J::Str(O,TEXT("id"));Out.Kind=S47J::Str(O,TEXT("kind"));Out.Ref=S47J::Str(O,TEXT("ref"));Out.Text=S47J::Str(O,TEXT("text"));Out.Place=S47J::Str(O,TEXT("place"));
  Out.Day=FMath::Clamp(S47J::Int(O,TEXT("day")),0,36500);Out.Goods=GoodsOf(S47J::Obj(O,TEXT("goods")));
  static const TCHAR* const KindNames[]={TEXT("ward"),TEXT("ledger"),TEXT("catalog"),TEXT("parcel"),TEXT("bill"),TEXT("nobody"),TEXT("letter")}; // (letter: D48-20261110, a reply to PostOut)
  bool bKind=false;for(const TCHAR* K:KindNames)bKind|=Out.Kind==K;
  return bKind&&!Out.Id.IsEmpty();
 }
 // A catalog line's goods id, without "kit:".
 FString KitOf(const FString& Goods){return Goods.StartsWith(TEXT("kit:"))?Goods.Mid(4):FString();}
 // The findings the town might hear of, by their kind (FS47Happenings, FS47FarmSite), and how the paper files them. The station's
 // own records (the Director's observations, what only the array logged, the counter's readings) it never hears of.
 bool TopicOf(const FString& Id,S47Ledger::ETopic& Out)
 {
  using S47Ledger::ETopic;
  struct FKind{const TCHAR* Prefix;ETopic Topic;};
  static const FKind Kinds[]={
   {TEXT("missing_reported_d"),ETopic::Called},{TEXT("missing_d"),ETopic::Missing},{TEXT("carcass_d"),ETopic::Carcass},{TEXT("cattle_ring_d"),ETopic::Cattle},
   {TEXT("lights_trace_"),ETopic::Landing},{TEXT("farm_trace"),ETopic::Landing},{TEXT("landing_seen_d"),ETopic::Lights},{TEXT("house_lit_d"),ETopic::LitHouse},
   {TEXT("car_dies_d"),ETopic::Stalled},{TEXT("car_light_d"),ETopic::Stalled},{TEXT("cb_empty_d"),ETopic::EmptyRig},{TEXT("rv_d"),ETopic::Camper},
   {TEXT("hitcher_d"),ETopic::Walker},{TEXT("blackout_d"),ETopic::Outage},{TEXT("animals_flee_d"),ETopic::Fled},{TEXT("clock_ahead_d"),ETopic::ClockFast},
   {TEXT("cb_helped_d"),ETopic::Helped}};
  for(const FKind& K:Kinds)if(Id.StartsWith(K.Prefix,ESearchCase::CaseSensitive)){Out=K.Topic;return true;}
  return false;
 }
}

// The car's engine through a panner that follows it down the road.
struct FS47Mail::FCarSound
{
 TSharedPtr<FS47PannerNode> Pan;
 FS47Play Engine;
 float Rate=-1.f,Gain=-1.f;
 void Stop()
 {
  if(Engine.Src)Engine.Src->Stop();
  Engine={};if(Pan)Pan->Disconnect();
  Pan.Reset();Rate=Gain=-1.f;
 }
};

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
 Box.Reset();Pending.Reset();Fetched.Reset();Posted.Reset();ReadIssues.Reset();Orders.Reset();Coupons.Reset();Issues.Reset();Cart.Reset();Seen.Reset();Unread.Reset();Outbound.Reset();
 bFlag=bCatalog=bBound=bPrimed=false;LastRound=Rounds=NobodySent=0;NextSerial=1;SeenCount=-1;Drive=FDrive();
 const int32 Today=M&&M->Freeform?M->Freeform->Day():1;
 if(Saved.IsValid()&&Saved->Values.Num()>0)
 {
  bCatalog=S47J::Bool(Saved,TEXT("catalog"));
  LastRound=FMath::Clamp(S47J::Int(Saved,TEXT("lastRound")),0,36500);Rounds=FMath::Max(0,S47J::Int(Saved,TEXT("rounds")));
  NextSerial=FMath::Max(1,S47J::Int(Saved,TEXT("serial"),1));NobodySent=FMath::Clamp(S47J::Int(Saved,TEXT("nobody")),0,9);
  for(const auto& V:S47J::Arr(Saved,TEXT("box"))){FPiece Pc;if(L::PieceOf(V,Pc)&&!Box.ContainsByPredicate([&](const FPiece& Q){return Q.Id==Pc.Id;}))Box.Add(Pc);}
  for(const auto& V:S47J::Arr(Saved,TEXT("pending"))){FPiece Pc;if(L::PieceOf(V,Pc))Pending.Add(Pc);}
  for(const auto& V:S47J::Arr(Saved,TEXT("fetched"))){FPiece Pc;if(L::PieceOf(V,Pc))Fetched.Add(Pc);}
  for(const FString& Id:S47J::Strs(Saved,TEXT("unread")))if(Fetched.ContainsByPredicate([&](const FPiece& Pc){return Pc.Id==Id;}))Unread.Add(Id);
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
  // D48-20261110: what waits in the box for the carrier that is no order; an older save has none
  for(const auto& V:S47J::Arr(Saved,TEXT("out")))
  {
   if(!V.IsValid()||V->Type!=EJson::Object)continue;const auto O=V->AsObject();FOut Sent;
   Sent.Id=S47J::Str(O,TEXT("id")).Left(64);Sent.What=S47J::Str(O,TEXT("what")).Left(160);Sent.Rounds=FMath::Clamp(S47J::Int(O,TEXT("rounds"),1),1,30);
   if(!Sent.Id.IsEmpty()&&L::PieceOf(O->TryGetField(TEXT("reply")),Sent.Reply)&&!Sent.Reply.Id.IsEmpty()&&!Outbound.ContainsByPredicate([&](const FOut& Q){return Q.Id==Sent.Id;}))Outbound.Add(Sent);
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
   K.Heads=S47J::Strs(O,TEXT("heads"));K.Bodies=S47J::Strs(O,TEXT("bodies"));K.Leads=S47J::Strs(O,TEXT("leads"));K.Keys=S47J::Strs(O,TEXT("keys"));
   K.Log=S47J::Strs(O,TEXT("log"));for(const double Ago:S47J::Nums(O,TEXT("logAgo")))K.LogAgo.Add(FMath::Clamp(int32(Ago),0,14));
   if(K.Log.Num()!=K.LogAgo.Num()){K.Log.Reset();K.LogAgo.Reset();}
   {const FString Pic=S47J::Str(O,TEXT("photo"));if(S47Ledger::PhotoValid(Pic))K.Photo=Pic;} // D46-1-20261110: an older save has none: the drawn picture
   if(K.Heads.Num()==K.Bodies.Num()&&K.Heads.Num()==K.Leads.Num()&&!Issues.ContainsByPredicate([&](const FKept& Q){return Q.No==K.No;}))Issues.Add(K);
  }
  Issues.Sort([](const FKept& A,const FKept& B){return A.No<B.No;});
  for(const auto& V:S47J::Arr(Saved,TEXT("seen")))
  {
   if(!V.IsValid()||V->Type!=EJson::Object)continue;const auto O=V->AsObject();FSeen Note;
   Note.Id=S47J::Str(O,TEXT("id"));Note.Day=FMath::Clamp(S47J::Int(O,TEXT("day")),0,36500);Note.Place=S47J::Str(O,TEXT("place")).Left(120);Note.PlaceId=S47J::Str(O,TEXT("placeId")).Left(64);
   if(!Note.Id.IsEmpty()&&!Seen.ContainsByPredicate([&](const FSeen& Q){return Q.Id==Note.Id;}))Seen.Add(Note);
  }
 }
 else
 {
  // A new game, or a save from before the post: the catalog is in the box. A save under way does not get the rounds it never had:
  // they start from today's.
  FPiece Pc;Pc.Id=TEXT("catalog");Pc.Kind=TEXT("catalog");Pc.Day=Today;Box.Add(Pc);Posted.Add(Pc.Id);
  if(bSavedGame&&M&&M->Freeform)LastRound=M->Freeform->Clock()>=RoundClock(M->Freeform->Seed(),Today)?Today:Today-1;
 }
 // The flag is the house's: up only while an order waits for the carrier (older saves had it up for anything in the box).
 bFlag=Orders.ContainsByPredicate([](const FOrder& O){return O.Picked==0;})||!Outbound.IsEmpty(); // (D48: or the balloon's box for the carrier)
 // A long time away (an old save) is caught up from two weeks back at most.
 LastRound=FMath::Max(LastRound,Today-14);
 UE_LOG(LogTemp,Display,TEXT("S47: mail: %d in the box, %d on the way, %d orders, flag %s, last round day %d, %d findings noted"),Box.Num(),Pending.Num(),Orders.Num(),bFlag?TEXT("up"):TEXT("down"),LastRound,Seen.Num());
}
void FS47Mail::Stop()
{
 Life.Reset();bBound=false;
 if(IsEngineExitRequested()||IsGarbageCollecting()||!IsValid(M)||M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)){Drive=FDrive();return;}
 EndCar();
 if(Model)Model->Show(false);
 if(IsValid(M->Web.Get())&&M->Web->HasWalkArea(TEXT("saro")))for(auto& Rect:M->Web->CollidersMutable(TEXT("saro")))if(Rect.Id==S47MailLocal::BoxRect)Rect.bEnabled=false;
}
S47J::FObj FS47Mail::Save() const
{
 namespace L=S47MailLocal;
 auto O=S47J::New();
 S47J::Set(O,TEXT("v"),2);S47J::Set(O,TEXT("flag"),bFlag);S47J::Set(O,TEXT("lastRound"),LastRound);S47J::Set(O,TEXT("rounds"),Rounds);S47J::Set(O,TEXT("serial"),NextSerial);
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
 // Only what is still not read (IsUnread: read at OPS, in the notebook, or a Ledger issue read) is kept as such.
 {
  TArray<FString> Ids;for(const FPiece& Pc:Fetched)if(Unread.Contains(Pc.Id)&&IsUnread(Pc))Ids.Add(Pc.Id);
  if(!Ids.IsEmpty()){Ids.Sort();S47J::Set(O,TEXT("unread"),Ids);}
 }
 if(!Orders.IsEmpty())
 {
  TArray<TSharedPtr<FJsonValue>> A;
  for(const FOrder& Ord:Orders){auto V=S47J::New();S47J::Set(V,TEXT("serial"),Ord.Serial);S47J::Set(V,TEXT("paid"),Ord.Paid);S47J::Set(V,TEXT("picked"),Ord.Picked);S47J::Set(V,TEXT("due"),Ord.Due);S47J::Set(V,TEXT("goods"),L::GoodsJson(Ord.Goods));A.Add(MakeShared<FJsonValueObject>(V));}
  O->SetArrayField(TEXT("orders"),A);
 }
 if(!Outbound.IsEmpty()) // D48-20261110: a new key, written only when something waits (an older save reads as before)
 {
  TArray<TSharedPtr<FJsonValue>> A;
  for(const FOut& Sent:Outbound){auto V=S47J::New();S47J::Set(V,TEXT("id"),Sent.Id);S47J::Set(V,TEXT("what"),Sent.What);S47J::Set(V,TEXT("rounds"),Sent.Rounds);S47J::Set(V,TEXT("reply"),L::PieceJson(Sent.Reply));A.Add(MakeShared<FJsonValueObject>(V));}
  O->SetArrayField(TEXT("out"),A);
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
  for(const FKept& K:Issues)
  {
   auto V=S47J::New();S47J::Set(V,TEXT("no"),K.No);S47J::Set(V,TEXT("day"),K.Day);if(K.bThanks)S47J::Set(V,TEXT("thanks"),true);
   S47J::Set(V,TEXT("heads"),K.Heads);S47J::Set(V,TEXT("bodies"),K.Bodies);S47J::Set(V,TEXT("leads"),K.Leads);S47J::Set(V,TEXT("keys"),K.Keys);
   if(!K.Photo.IsEmpty())S47J::Set(V,TEXT("photo"),K.Photo); // D46-1-20261110
   if(!K.Log.IsEmpty()){TArray<double> Ago;for(const int32 Each:K.LogAgo)Ago.Add(Each);S47J::Set(V,TEXT("log"),K.Log);S47J::Set(V,TEXT("logAgo"),Ago);}
   A.Add(MakeShared<FJsonValueObject>(V));
  }
  O->SetArrayField(TEXT("issues"),A);
 }
 if(!Seen.IsEmpty())
 {
  TArray<TSharedPtr<FJsonValue>> A;
  for(const FSeen& Note:Seen){auto V=S47J::New();S47J::Set(V,TEXT("id"),Note.Id);S47J::Set(V,TEXT("day"),Note.Day);if(!Note.Place.IsEmpty())S47J::Set(V,TEXT("place"),Note.Place);if(!Note.PlaceId.IsEmpty())S47J::Set(V,TEXT("placeId"),Note.PlaceId);A.Add(MakeShared<FJsonValueObject>(V));}
  O->SetArrayField(TEXT("seen"),A);
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
  return bFlag?FString(TEXT("A rural mailbox on a post, SARO and BOX 19 painted on the side, the red flag up for the carrier.")):FString(TEXT("A rural mailbox on a post, SARO and BOX 19 painted on the side. The flag is down."));
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
 namespace L=S47MailLocal;
 if(!Live())return;
 auto& Ses=*M->Freeform;
 if(!Build())return;
 if(!Model->Seated()&&M->Web->Area()==TEXT("saro")&&M->Player&&FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(FS47MailBoxModel::PostX,FS47MailBoxModel::PostZ))<250)Model->Seat(M->Web.Get());
 Model->SetFlag(bFlag);Model->SetParcel(ParcelOnBox());Model->Step(Dt);
 // What the nights leave is noted as it is registered, with where the player is then.
 NoteFindings();
 if(Ses.Held())return;
 // A rest or a jump in time while the car was on the road: it does not turn up hours later; its round is made as it would have been.
 if(Drive.bOn&&Ses.Clock()-Drive.Started>L::Jumped)
 {
  const bool bOwed=Drive.bApply&&!Drive.bDone;const int32 OnDay=Drive.OnDay;
  EndCar();
  if(bOwed){LastRound=FMath::Max(LastRound,OnDay);Round(OnDay);}
 }
 StepCar(Dt);
 // The rounds that are due. Today's: the car sets out up the highway a little before its hour when the player is at the station to
 // see it, and the round is made when it reaches the box; otherwise (away, asleep, the hour long past) at its hour without it.
 // Any a long rest went past are made as they were. No rounds on Sundays or Memorial Day.
 const int32 Today=Ses.Day();
 for(int32 OnDay=LastRound+1;OnDay<=Today;++OnDay)
 {
  if(!RoundOn(Ses.DateOfDay(OnDay))){LastRound=OnDay;continue;}
  if(OnDay==Today)
  {
   if(Drive.bOn&&Drive.bApply&&Drive.OnDay==OnDay)break;
   const double Due=RoundClock(Ses.Seed(),OnDay),Now=Ses.Clock();
   if(Now<Due-L::Lead)break;
   if(Now<Due+L::Late&&!Drive.bOn&&StartCar(OnDay,true,S47Legs::SARO_HWY.Z1-5,false))break;
   if(Now<Due)break;
  }
  LastRound=OnDay;Round(OnDay);
 }
 // D46-1-20261110: the newest paper printed while the world could not be photographed (night, not loaded) gets its picture when it can.
 PhotoWait-=Dt;
 if(PhotoWait<=0&&!Issues.IsEmpty())
 {
  PhotoWait=4.;
  FKept& Newest=Issues.Last();
  if(Newest.Photo.IsEmpty()&&Today-Newest.Day<=6&&PhotoTries.FindRef(Newest.No)<3)TakePhoto(Newest);
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
bool FS47Mail::RoundBrings(int32 OnDay) const
{
 if(!Outbound.IsEmpty())return true; // D48: something left for the carrier (also without a game)
 if(!M||!M->Freeform)return false;
 const auto& Ses=*M->Freeform;
 // Something to take (an order with the flag up) or to leave (what was posted, a parcel that is due, Thursday's paper, the rest).
 if(Orders.ContainsByPredicate([&](const FOrder& O){return O.Picked==0||O.Due<=OnDay;}))return true;
 if(Pending.ContainsByPredicate([&](const FPiece& Pc){return Pc.Day<=OnDay;}))return true;
 if(Ses.DateOfDay(OnDay).GetDayOfWeek()==EDayOfWeek::Thursday)return true;
 return NobodySent<2&&OnDay>=NobodyDay(NobodySent)&&!Ses.InTutorial();
}
void FS47Mail::Round(int32 OnDay)
{
 if(!M||!M->Freeform)return;
 auto& Ses=*M->Freeform;
 ++Rounds;
 int32 Left=0,Picked=0;
 // Out: the orders waiting in the box, with their money orders. The carrier puts the flag down.
 for(FOrder& Ord:Orders)if(Ord.Picked==0)
 {
  Ord.Picked=OnDay;int32 Due=OnDay;
  for(int32 K=0,Tries=0;K<ShipRounds(Ses.Seed(),Ord.Serial)&&Tries<20;++Tries){++Due;if(RoundOn(Ses.DateOfDay(Due)))++K;}
  Ord.Due=Due;++Picked;
  UE_LOG(LogTemp,Display,TEXT("S47: mail: the carrier took order %d, due day %d"),Ord.Serial,Due);
 }
 // D48-20261110: what else was left for the carrier (the balloon's instrument box): taken, and its reply posted for a few rounds on.
 for(int32 I=Outbound.Num()-1;I>=0;--I)
 {
  FOut Sent=Outbound[I];Outbound.RemoveAt(I);int32 Due=OnDay;
  for(int32 K=0,Tries=0;K<Sent.Rounds&&Tries<40;++Tries){++Due;if(RoundOn(Ses.DateOfDay(Due)))++K;}
  Sent.Reply.Day=Due;Pending.Add(Sent.Reply);Posted.Add(Sent.Reply.Id);++Picked;
  UE_LOG(LogTemp,Display,TEXT("S47: mail: the carrier took %s; %s comes back about day %d"),*Sent.Id,*Sent.Reply.Id,Due);
 }
 // In: what was posted for today, parcels that are due (on top of the box), Thursday's paper, and now and then what nobody sent.
 for(int32 I=Pending.Num()-1;I>=0;--I)if(Pending[I].Day<=OnDay)
 {
  FPiece Pc=Pending[I];Pc.Day=OnDay;Pending.RemoveAt(I);Box.Insert(Pc,0);++Left;
 }
 for(int32 I=Orders.Num()-1;I>=0;--I)if(Orders[I].Picked>0&&Orders[I].Due<=OnDay)
 {
  const FOrder Ord=Orders[I];Orders.RemoveAt(I);
  FPiece Pc;Pc.Id=FString::Printf(TEXT("parcel:%d"),Ord.Serial);Pc.Kind=TEXT("parcel");Pc.Ref=FString::FromInt(Ord.Serial);Pc.Day=OnDay;Pc.Goods=Ord.Goods;Pc.Text=SlipText(Ord.Serial,Ord.Goods,Ord.Paid);
  Box.Add(Pc);Posted.Add(Pc.Id);++Left;
  UE_LOG(LogTemp,Display,TEXT("S47: mail: parcel %d on the box, day %d"),Ord.Serial,OnDay);
 }
 if(Ses.DateOfDay(OnDay).GetDayOfWeek()==EDayOfWeek::Thursday){PrintLedger(OnDay);++Left;}
 if(NobodySent<2&&OnDay>=NobodyDay(NobodySent)&&!Ses.InTutorial())
 {
  const int32 V0=FMath::Abs(Ses.Seed())%3;
  int32 Variant=NobodySent==0?V0:(V0+1+(FMath::Abs(Ses.Seed())/3)%2)%3;
  // The map circles a place the player has not heard of; with none left, it is one of the others.
  FString Circled;
  if(Variant==1)
  {
   if(Ses.Places){Circled=Ses.Places->NextUnknown(OnDay*7+NobodySent);if(!FS47Places::Def(Circled))Circled.Empty();}
   // (the second is a map only when the first was the sheet or the postcard: it becomes the other one)
   if(Circled.IsEmpty())Variant=NobodySent==0?0:2-V0;
  }
  FPiece Pc;Pc.Id=FString::Printf(TEXT("nobody:%d"),NobodySent+1);Pc.Kind=TEXT("nobody");Pc.Ref=FString::FromInt(Variant);Pc.Day=OnDay;Pc.Place=Circled;
  const FS47Places::FDef* D=Circled.IsEmpty()?nullptr:FS47Places::Def(Circled);
  Pc.Text=NobodyText(Variant,D?FString(D->Title):FString());
  Box.Add(Pc);Posted.Add(Pc.Id);++NobodySent;++Left;
  UE_LOG(LogTemp,Display,TEXT("S47: mail: something with no sender in the box, day %d, variant %d"),OnDay,Variant);
 }
 bFlag=Orders.ContainsByPredicate([](const FOrder& O){return O.Picked==0;})||!Outbound.IsEmpty();
 UE_LOG(LogTemp,Display,TEXT("S47: mail: round %d on day %d: left %d, picked up %d, %d in the box, flag %s"),Rounds,OnDay,Left,Picked,Box.Num(),bFlag?TEXT("up"):TEXT("down"));
 M->RequestAutosave();
}

// ---------- the carrier's car ----------
bool FS47Mail::StartCar(int32 OnDay,bool bApply,double FromZ,bool bForceStop)
{
 namespace L=S47MailLocal;
 if(!M||!IsValid(M->Web.Get())||!M->Web->IsLoaded()||M->Web->Area()!=TEXT("saro")||!M->Player||!M->Freeform)return false;
 if(!Car)Car=MakeUnique<FS47MailCarModel>();
 if(!Car->Build(M->Web.Get()))return false;
 EndCar();
 Drive=FDrive();Drive.bOn=true;Drive.bApply=bApply;Drive.bForceStop=bForceStop;Drive.OnDay=OnDay;Drive.Z=FromZ;Drive.X=L::LaneX;Drive.Speed=L::Cruise;
 Drive.Started=M->Freeform->Clock();
 Drive.bStops=bForceStop||RoundBrings(OnDay);
 Car->Show(true);
 if(!CarSound)CarSound=MakeUnique<FCarSound>();
 if(IsValid(M->Audio)&&M->Audio->Ctx.IsValid())
 {
  const bool bIn=M->Freeform->Taken&&M->Freeform->Taken->Indoors();
  CarSound->Pan=M->Audio->Ctx->CreatePanner();CarSound->Pan->RefDistance=14;CarSound->Pan->Connect(M->Audio->SfxBus);
  FS47Sound S;S.Gain=0.f;S.Rate=.9f;S.bLoop=true;S.DestNode=CarSound->Pan;S.Lp=bIn?900.f:2400.f;
  CarSound->Engine=M->Audio->Sfx(TEXT("engineDrive"),S);
 }
 PlaceCar();
 UE_LOG(LogTemp,Display,TEXT("S47: mail: the carrier's car on the highway, %.0f m south of the box, %s%s"),FromZ-L::StopZ,Drive.bStops?TEXT("stopping"):TEXT("going by"),bApply?TEXT(""):TEXT(" (a test's show)"));
 return true;
}
void FS47Mail::EndCar()
{
 Drive.bOn=false;Drive.bStopped=false;
 if(Car)Car->Show(false);
 if(CarSound)CarSound->Stop();
}
void FS47Mail::PlaceCar()
{
 if(!Car||!M||!IsValid(M->Web.Get()))return;
 const auto Floor=[&](double AtZ){return US47Night::SaroFloorAt(M->Web.Get(),Drive.X,AtZ);};
 const double Y=Floor(Drive.Z),Front=Floor(Drive.Z-1.45),Back=Floor(Drive.Z+1.45);
 Car->Place(FVector(Drive.X,Y,Drive.Z),FMath::Atan2(Front-Back,2.9));
 if(CarSound&&CarSound->Pan)CarSound->Pan->SetPosition(Drive.X,Y+1,Drive.Z);
}
void FS47Mail::CarEngine(float Rate,float Gain)
{
 if(!CarSound||!CarSound->Engine.Src||!IsValid(M->Audio)||!M->Audio->Ctx.IsValid())return;
 const double Now=M->Audio->Ctx->CurrentTime();
 const bool bIn=M->Freeform&&M->Freeform->Taken&&M->Freeform->Taken->Indoors();
 const float Want=Gain*(bIn?.4f:1.f);
 if(FMath::Abs(CarSound->Rate-Rate)>.02f){CarSound->Rate=Rate;CarSound->Engine.Src->PlaybackRate.SetTargetAtTime(Rate,Now,.35);}
 if(CarSound->Engine.Gain.IsValid()&&FMath::Abs(CarSound->Gain-Want)>.01f){CarSound->Gain=Want;CarSound->Engine.Gain->Gain.SetTargetAtTime(Want,Now,.35);}
}
void FS47Mail::Clank(float Gain)
{
 if(!M||!IsValid(M->Audio))return;
 FS47Sound S;S.Gain=Gain;S.At=Model?Model->Lid():FVector(FS47MailBoxModel::PostX,0.7,FS47MailBoxModel::PostZ);
 M->Audio->Sfx(TEXT("metalTap"),S);
}
void FS47Mail::ApplyRound()
{
 if(Drive.bDone)return;
 Drive.bDone=true;
 if(!Drive.bApply)return;
 LastRound=FMath::Max(LastRound,Drive.OnDay);
 Round(Drive.OnDay);
}
void FS47Mail::StepCar(double Dt)
{
 namespace L=S47MailLocal;
 if(!Drive.bOn)return;
 // The player left SARO's stretch: the car is not on this one any more, and its round is made without anyone seeing it.
 if(!M||!IsValid(M->Web.Get())||M->Web->Area()!=TEXT("saro")||!Car||!Car->Built())
 {
  const bool bOwed=Drive.bApply&&!Drive.bDone;const int32 OnDay=Drive.OnDay;
  EndCar();
  if(bOwed){LastRound=FMath::Max(LastRound,OnDay);Round(OnDay);}
  return;
 }
 const double Tick=FMath::Clamp(Dt,0.,.1);
 if(Drive.bStopped)
 {
  // At the box, the roof light going: the door down, what there is goes in, an order comes out and the flag comes down, the door up.
  Drive.Wait+=Tick;
  if(!Drive.bClank1&&Drive.Wait>1.){Drive.bClank1=true;Clank(.8f);}
  if(!Drive.bDone&&Drive.Wait>1.8)ApplyRound();
  if(!Drive.bClank2&&Drive.Wait>4.4){Drive.bClank2=true;Clank(.6f);}
  Car->Beacon(FMath::Fmod(Drive.Wait,.8)<.4);
  CarEngine(.62f,.2f);
  if(Drive.Wait>=L::Dwell){Drive.bStopped=false;Drive.Speed=0;}
  return;
 }
 const double ToBox=Drive.Z-L::StopZ;
 // Whether it stops is settled at the braking point: an order left in the box while the car was coming is still picked up.
 if(!Drive.bDone&&ToBox>0&&ToBox<L::Decide)Drive.bStops=Drive.bForceStop||RoundBrings(Drive.OnDay);
 double Want=L::Cruise;
 if(!Drive.bDone&&Drive.bStops&&ToBox>0)Want=FMath::Min(Want,FMath::Sqrt(2.*L::Brake*ToBox)+.3);
 if(!Drive.bDone&&Drive.bStops&&ToBox<=.05)
 {
  Drive.Z=L::StopZ;Drive.X=L::CurbX;Drive.Speed=0;Drive.bStopped=true;Drive.Wait=0;Drive.StopAt=Drive.Z;Drive.XAtStop=Drive.X;PlaceCar();
  UE_LOG(LogTemp,Display,TEXT("S47: mail: the carrier's car stops at the box"));
  return;
 }
 // Somebody standing in the road ahead (on its way back out it swings to the lane, so a player at the box is passed). Somebody at
 // the box itself: the carrier stops short and hands it out of the window. Anywhere else it waits for them.
 if(M->Player)
 {
  const FVector P=M->Player->Pos;const double Ahead=Drive.Z-P.Z;
  const double PathX=Drive.bDone?L::LaneX:Drive.X;
  if(FMath::Abs(P.X-PathX)<1.3&&Ahead>1.6&&Ahead<8.)
  {
   if(!Drive.bDone&&Drive.bStops&&ToBox<12.)
   {
    Drive.Speed=0;Drive.bStopped=true;Drive.Wait=0;Drive.StopAt=Drive.Z;Drive.XAtStop=Drive.X;PlaceCar();
    UE_LOG(LogTemp,Display,TEXT("S47: mail: the carrier's car stops short of the box, somebody standing at it"));
    return;
   }
   Want=0;
  }
 }
 const double Acc=Want>Drive.Speed?L::Pull:L::Brake*1.5;
 Drive.Speed=Want>Drive.Speed?FMath::Min(Want,Drive.Speed+Acc*Tick):FMath::Max(Want,Drive.Speed-Acc*Tick);
 Drive.Z-=Drive.Speed*Tick;
 // Going by without stopping: the round is made as it passes, with nothing in it.
 if(!Drive.bDone&&Drive.Z<=L::StopZ)ApplyRound();
 // In to the edge of the road for the box, and back out to the lane after it.
 if(Drive.bDone&&Drive.bStops)Drive.X=FMath::Lerp(Drive.XAtStop,L::LaneX,L::Smooth((Drive.StopAt-Drive.Z)/L::PullOut));
 else Drive.X=Drive.bStops?FMath::Lerp(L::LaneX,L::CurbX,L::Smooth(1.-(Drive.Z-L::StopZ)/L::PullIn)):L::LaneX;
 Car->Beacon(!Drive.bDone&&Drive.bStops&&ToBox<L::PullIn&&FMath::Fmod(ToBox,6.)<3.);
 CarEngine(float(.7+.35*Drive.Speed/L::Cruise),float(.16+.22*Drive.Speed/L::Cruise));
 PlaceCar();
 if(Drive.Z<=S47Legs::SARO_HWY.Z0+5)
 {
  const bool bOwed=Drive.bApply&&!Drive.bDone;const int32 OnDay=Drive.OnDay;
  EndCar();
  if(bOwed){LastRound=FMath::Max(LastRound,OnDay);Round(OnDay);}
 }
}

bool FS47Mail::Post(const FPiece& Piece,bool bNow)
{
 if(Piece.Id.IsEmpty()||Posted.Contains(Piece.Id))return false;
 Posted.Add(Piece.Id);
 FPiece Pc=Piece;
 const int32 Today=M&&M->Freeform?M->Freeform->Day():1;
 if(bNow){Pc.Day=Today;Box.Add(Pc);}
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
FString FS47Mail::TowText(const FString& Date,int32 Fee,bool bPaidInCash)
{
 return FString::Printf(TEXT("SOCORRO COUNTY ROAD DEPARTMENT\nWRECKER SERVICE / STATEMENT\n\nBILL TO: SARO, DISTRICT SERVICE TRUCK\nDATE OF SERVICE: %s\n\nTOW TO THE STATION, HOOK-UP AND MILEAGE   $%d\n\n%s\n\nKeep the truck on the road and we will both be happier."),
  *Date,Fee,bPaidInCash?TEXT("PAID FROM THE STATION BUDGET. THANK YOU."):TEXT("NOT PAID: CARRIED TO THE STATION'S WEEKLY SHEET."));
}
namespace S47MailLocal
{
 // M7 (C): every tow gets its own statement, two on one day included: the first number not yet posted (Posted is saved already).
 FString TowBillId(int32 Today,int32 Rounds,const TSet<FString>& Posted)
 {
  FString Id;int32 N=0;
  do{Id=FString::Printf(TEXT("bill:tow-%d-%d-%d"),Today,Rounds,N++);}while(Posted.Contains(Id)&&N<1000);
  return Id;
 }
}
void FS47Mail::PostTowBill(int32 Fee,bool bPaidInCash)
{
 if(!Live()||!M||!M->Freeform)return;
 const int32 Today=M->Freeform->Day();
 FPiece Pc;Pc.Id=S47MailLocal::TowBillId(Today,Rounds,Posted);Pc.Kind=TEXT("bill");
 Pc.Text=TowText(DayText(Today).ToUpper(),Fee,bPaidInCash);
 Post(Pc);
}
bool FS47Mail::PostOut(const FString& Id,const FString& What,int32 InRounds,const FPiece& Reply)
{
 // D48-20261110: as an order is left (WriteOrder), without the money: once in a game, its reply once too, the flag up for the carrier.
 if(Id.IsEmpty()||Reply.Id.IsEmpty()||Posted.Contains(TEXT("out:")+Id)||Posted.Contains(Reply.Id)||OutWaiting(Id))return false;
 FOut Sent;Sent.Id=Id;Sent.What=What;Sent.Rounds=FMath::Clamp(InRounds,1,30);Sent.Reply=Reply;
 Outbound.Add(Sent);Posted.Add(TEXT("out:")+Id);bFlag=true;
 if(M&&M->Audio){M->Audio->Sfx(TEXT("paper1"),FS47Sound{.Gain=.5f});M->Audio->Sfx(TEXT("click"),FS47Sound{.Gain=.45f,.When=.6f});}
 UE_LOG(LogTemp,Display,TEXT("S47: mail: %s left in the box for the carrier; flag up"),*Id);
 if(M)M->RequestAutosave();
 return true;
}
bool FS47Mail::OutWaiting(const FString& Id) const{return Outbound.ContainsByPredicate([&](const FOut& S){return S.Id==Id;});}
bool FS47Mail::Holds(const FString& Id) const{return Box.ContainsByPredicate([&](const FPiece& Pc){return Pc.Id==Id;});}
bool FS47Mail::Coming(const FString& Id) const{return Pending.ContainsByPredicate([&](const FPiece& Pc){return Pc.Id==Id;});}
bool FS47Mail::ParcelOnBox() const{return Box.ContainsByPredicate([](const FPiece& Pc){return Pc.Kind==TEXT("parcel");});}

// ---------- what the nights left, for the paper ----------
bool FS47Mail::PlaceOf(const FString& Id,const FString& Text,bool bHere,FString& OutPlace,FString& OutId) const
{
 namespace L=S47MailLocal;
 OutPlace.Empty();OutId.Empty();
 if(!M||!M->Freeform)return false;
 const auto& Ses=*M->Freeform;
 // Lights seen coming down from the station: where they were is a bearing, and the paper says it as one.
 if(Id.StartsWith(TEXT("landing_seen_d")))
 {
  const int32 At=Text.Find(TEXT("about "));
  if(At!=INDEX_NONE&&Text.Contains(TEXT(" degrees from SARO")))
  {
   const double Az=FCString::Atod(*Text.Mid(At+6));
   OutPlace=FString::Printf(TEXT("the land %s of the radio station"),L::Points[((FMath::RoundToInt(Az/45.)%8)+8)%8]);
   return true;
  }
  return false;
 }
 if(Id.StartsWith(TEXT("blackout_d"))){OutPlace=TEXT("Caliche");return true;}
 // A place the finding names: the longest title of the land's places found in its words.
 int32 Best=0;
 if(Ses.EventPlaces)for(const FS47EventPlace& Pc:Ses.EventPlaces->All())
  if(Pc.Title.Len()>Best&&Text.Contains(Pc.Title,ESearchCase::CaseSensitive)){Best=Pc.Title.Len();OutPlace=Pc.Title;OutId=FS47Places::Def(Pc.Id)?Pc.Id:FString();}
 for(const FS47Places::FDef& D:FS47Places::Defs())
 {
  const FString Title=D.Title;
  if(Title.Len()>Best&&Text.Contains(Title,ESearchCase::CaseSensitive)){Best=Title.Len();OutPlace=Title;OutId=D.Id;}
 }
 if(Best>0)return true;
 // What happened where the player was (on the road, rabbits running past, a figure by the highway): the place nearest them then.
 if(!bHere||!Ses.EventPlaces)return false;
 const bool bRoad=Id.StartsWith(TEXT("car_"))||Id.StartsWith(TEXT("clock_ahead_d"))||Id.StartsWith(TEXT("hitcher_d"))||Id.StartsWith(TEXT("animals_flee_d"));
 if(const FS47EventPlace* Near=Ses.EventPlaces->Nearest(bRoad?1500.:400.))
 {
  OutPlace=Near->Title;OutId=FS47Places::Def(Near->Id)?Near->Id:FString();
  return true;
 }
 return false;
}
void FS47Mail::NoteFindings()
{
 namespace L=S47MailLocal;
 if(!M||!M->Freeform)return;
 const auto& Ses=*M->Freeform;
 const int32 Count=Ses.FindingCount();
 if(Count==SeenCount)return;
 SeenCount=Count;
 // The first look after a game is loaded finds what was there before: those are placed by their own words only, never by where the
 // player happens to stand now.
 const bool bHere=bPrimed;bPrimed=true;
 for(const FString& Id:Ses.FindingIdsFrom(FMath::Max(1,Ses.Day()-8)))
 {
  S47Ledger::ETopic Topic;
  if(!L::TopicOf(Id,Topic)||Seen.ContainsByPredicate([&](const FSeen& Q){return Q.Id==Id;}))continue;
  FSeen Note;Note.Id=Id;Note.Day=Ses.FindingDay(Id);
  PlaceOf(Id,Ses.FindingText(Id),bHere,Note.Place,Note.PlaceId);
  Seen.Add(Note);
  UE_LOG(LogTemp,Display,TEXT("S47: mail: noted %s, day %d, at %s"),*Id,Note.Day,Note.Place.IsEmpty()?TEXT("no place"):*Note.Place);
 }
 while(Seen.Num()>80)Seen.RemoveAt(0);
}
TArray<S47Ledger::FFact> FS47Mail::WeekFacts(int32 FromDay,int32 ToDay) const
{
 namespace L=S47MailLocal;
 TArray<S47Ledger::FFact> Out;
 if(!M||!M->Freeform)return Out;
 const auto& Ses=*M->Freeform;
 const auto NightWords=[&](int32 OnDay){return FString(L::WeekNames[int32(Ses.DateOfDay(OnDay).GetDayOfWeek())])+TEXT(" night");};
 TSet<FString> Told;
 // What was noted as it happened, at the place it was.
 for(const FSeen& Note:Seen)
 {
  S47Ledger::ETopic Topic;
  if(Note.Day<FromDay||Note.Day>ToDay||!L::TopicOf(Note.Id,Topic))continue;
  S47Ledger::FFact Fact;Fact.Topic=Topic;Fact.Day=Note.Day;Fact.Night=NightWords(Note.Day);Fact.Place=Note.Place;Fact.PlaceId=Note.PlaceId;Fact.Id=Note.Id;
  Out.Add(Fact);Told.Add(Note.Id);
 }
 // A finding from before the box kept notes (an older save): its place from its own words only.
 for(const FString& Id:Ses.FindingIdsFrom(FromDay))
 {
  S47Ledger::ETopic Topic;
  const int32 OnDay=Ses.FindingDay(Id);
  if(Told.Contains(Id)||OnDay<FromDay||OnDay>ToDay||!L::TopicOf(Id,Topic))continue;
  S47Ledger::FFact Fact;Fact.Topic=Topic;Fact.Day=OnDay;Fact.Night=NightWords(OnDay);Fact.Id=Id;
  PlaceOf(Id,Ses.FindingText(Id),false,Fact.Place,Fact.PlaceId);
  Out.Add(Fact);
 }
 Out.StableSort([](const S47Ledger::FFact& A,const S47Ledger::FFact& B){return A.Day<B.Day;});
 return Out;
}

// ---------- the paper ----------
void FS47Mail::PrintLedger(int32 OnDay)
{
 if(!M||!M->Freeform)return;
 auto& Ses=*M->Freeform;
 const int32 No=Issues.Num()+1;
 // The week up to the paper: from the last paper's day through the night before this one, so no night is told twice.
 const int32 FromDay=Issues.IsEmpty()?1:Issues.Last().Day;
 const TArray<S47Ledger::FFact> Facts=WeekFacts(FromDay,OnDay-1);
 FString UnknownId,UnknownTitle;
 if(Ses.Places){UnknownId=Ses.Places->NextUnknown(No*13+5);if(const FS47Places::FDef* D=FS47Places::Def(UnknownId))UnknownTitle=D->Title;else UnknownId.Empty();}
 TMap<FString,int32> Used;for(const FKept& Was:Issues)for(const FString& Key:Was.Keys)Used.Add(Key,Was.No);
 const TArray<S47Ledger::FStory> Front=S47Ledger::Front(Ses.Seed(),No,Ses.DateOfDay(OnDay),Facts,Used,UnknownId,UnknownTitle);
 const TArray<S47Ledger::FLogLine> Log=S47Ledger::LogLines(Facts,Front,OnDay,2);
 FKept K;K.No=No;K.Day=OnDay;K.bThanks=Facts.ContainsByPredicate([](const S47Ledger::FFact& Fact){return Fact.Topic==S47Ledger::ETopic::Helped;});
 for(const S47Ledger::FStory& St:Front){K.Heads.Add(St.Head);K.Bodies.Add(St.Body);K.Leads.Add(St.Lead);K.Keys.Add(St.Key);}
 for(const S47Ledger::FLogLine& Line:Log){K.Log.Add(Line.Text);K.LogAgo.Add(Line.Ago);}
 TakePhoto(K); // D46-1-20261110: if the world cannot be photographed now, Step tries again when it can
 Issues.Add(K);
 FPiece Pc;Pc.Id=FString::Printf(TEXT("ledger:%d"),No);Pc.Kind=TEXT("ledger");Pc.Ref=FString::FromInt(No);Pc.Day=OnDay;
 Box.Add(Pc);Posted.Add(Pc.Id);
 UE_LOG(LogTemp,Display,TEXT("S47: mail: the Ledger no. %d printed, day %d: %d facts from day %d to %d, front \"%s\" / \"%s\", %d in the sheriff's log"),No,OnDay,Facts.Num(),FromDay,OnDay-1,
  Front.Num()>0?*Front[0].Head:TEXT(""),Front.Num()>1?*Front[1].Head:TEXT(""),Log.Num());
}
// D46-1-20261110 (Tom: «tar et svarthvitt bilde fra verdenen som noenlunde passer til artikkelen»): the front page's photo. Taken in the
// world at the lead story's place, by day only (so no light, figure or night-film object is in it: the film-only objects are left
// off, as the album's trips do not show them either), from a standing eye 38 to 52 m off, looking at the place. Then grey, flattened
// and small (S47Ledger::PhotoFromPixels), and kept with the issue. The wagon and stories with no place keep the drawn picture.
bool FS47Mail::TakePhoto(FKept& K)
{
 if(!K.Photo.IsEmpty()||K.Leads.IsEmpty()||!M||!M->Freeform||!M->FieldCamera||!M->Player||!IsValid(M->Web.Get())||!M->Web->IsLoaded())return false;
 auto& Ses=*M->Freeform;
 const FString Lead=K.Leads[0];
 if(!FS47Places::Def(Lead)||!Ses.Places.IsValid())return false;
 const double Hour=FMath::Fmod(Ses.Clock(),86400.)/3600.;
 if(Hour<S47Weather::SunriseClock/3600.+2.5||Hour>S47Weather::SunsetClock/3600.-2.5)return false;
 UWorld* World=M->Web->GetWorld();
 double Wx=0,Wz=0;
 if(!World||!Ses.Places->Spot(Lead,Wx,Wz))return false;
 const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 const FVector Target(Road.X+Wx,Road.Y+S47RoadTerrain::SurfaceY(Wx,Wz)+2.4,Road.Z+Wz);
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47LedgerPhoto),true);
 // The first of eight bearings (by the paper's number) where the ground is there to stand on, level enough with the place, and the
 // line to it clear.
 bool bFound=false;FVector Eye=FVector::ZeroVector;
 for(int32 Try=0;Try<16&&!bFound;++Try)
 {
  const double Ang=((K.No*3+Try)%8)*UE_DOUBLE_PI/4.,Away=Try<8?38.:52.;
  const double Rx=Wx+FMath::Cos(Ang)*Away,Rz=Wz+FMath::Sin(Ang)*Away;
  const double Base=Road.Y+S47RoadTerrain::SurfaceY(Rx,Rz);
  FHitResult Hit;
  if(!World->LineTraceSingleByChannel(Hit,S47Three::ToUE(FVector(Road.X+Rx,Base+30.,Road.Z+Rz)),S47Three::ToUE(FVector(Road.X+Rx,Base-30.,Road.Z+Rz)),ECC_Visibility,Q))continue;
  const FVector Stand=S47Three::ToThree(Hit.ImpactPoint);
  if(FMath::Abs(Stand.Y-(Target.Y-2.4))>12.)continue;
  const FVector From(Stand.X,Stand.Y+1.7,Stand.Z);
  FHitResult Block;
  if(World->LineTraceSingleByChannel(Block,S47Three::ToUE(From),S47Three::ToUE(Target),ECC_Visibility,Q)&&FVector::Dist(S47Three::ToThree(Block.ImpactPoint),Target)>6.)continue;
  Eye=From;bFound=true;
 }
 if(!bFound){++PhotoTries.FindOrAdd(K.No);return false;}
 US47FieldCamera* Cam=M->FieldCamera;
 const FVector OldLoc=Cam->PhotoLoc;const FRotator OldRot=Cam->PhotoRot;
 const TFunction<void(bool)> OldExposure=Cam->OnExposure;
 Cam->OnExposure=nullptr; // the film-only objects (the director's orbs) stay off
 Cam->PhotoLoc=S47Three::ToUE(Eye);
 Cam->PhotoRot=(S47Three::ToUE(Target)-Cam->PhotoLoc).Rotation();
 FS47SnapOptions O;O.W=480;O.Seed=K.No*7919.;
 const FS47Snap Snap=Cam->Snap(O);
 Cam->PhotoLoc=OldLoc;Cam->PhotoRot=OldRot;Cam->OnExposure=OldExposure;
 if(Snap.bRendered)K.Photo=S47Ledger::PhotoFromPixels(Snap.Px,Snap.W,Snap.H);
 if(K.Photo.IsEmpty()){++PhotoTries.FindOrAdd(K.No);UE_LOG(LogTemp,Warning,TEXT("S47: mail: the Ledger no. %d: no photo of %s (rendered %d)"),K.No,*Lead,Snap.bRendered?1:0);return false;}
 UE_LOG(LogTemp,Display,TEXT("S47: mail: the Ledger no. %d: a photo of %s, %d chars, from %.0f m"),K.No,*Lead,K.Photo.Len(),FVector::Dist(Eye,Target));
 return true;
}
bool FS47Mail::IssueOf(int32 No,S47Ledger::FIssue& Out) const
{
 if(!M||!M->Freeform)return false;
 const FKept* K=Issues.FindByPredicate([No](const FKept& Q){return Q.No==No;});if(!K)return false;
 Out=S47Ledger::FIssue();Out.No=K->No;Out.Day=K->Day;Out.Date=M->Freeform->DateOfDay(K->Day);
 for(int32 I=0;I<K->Heads.Num();++I)
 {
  S47Ledger::FStory St;St.Head=K->Heads[I];St.Body=K->Bodies[I];St.Lead=K->Leads[I];St.Key=K->Keys.IsValidIndex(I)?K->Keys[I]:FString();
  Out.Front.Add(St);
 }
 for(int32 I=0;I<K->Log.Num();++I){S47Ledger::FLogLine Line;Line.Text=K->Log[I];Line.Ago=K->LogAgo[I];Out.Log.Add(Line);}
 Out.Photo=K->Photo;
 S47Ledger::Fill(Out,M->Freeform->Seed(),K->bThanks);
 return true;
}
FString FS47Mail::IssueText(int32 No) const
{
 S47Ledger::FIssue Issue;
 return IssueOf(No,Issue)?S47Ledger::PlainText(Issue):FString();
}
void FS47Mail::OpenLedger(int32 No,int32 Sheet)
{
 if(!M||!M->UI||!M->Freeform)return;
 S47Ledger::FIssue Issue;if(!IssueOf(No,Issue))return;
 const FString ReadKey=FString::Printf(TEXT("ledger:%d"),No);
 if(!ReadIssues.Contains(ReadKey))
 {
  // D45-24-20261110: the first reading used to pencil every lead onto the map without a word the player noticed (Tom's game: two
  // places marked he never saw). Now nothing is marked by reading; the reader clicks a story (MarkLead). Once in a game, the first
  // paper with a lead not on the map yet says how ("ledger:hint" among the read, so nothing new is saved).
  ReadIssues.Add(ReadKey);
  const TArray<S47Ledger::FLeadRef> Refs=S47Ledger::LeadsOf(Issue);
  const bool bNew=Refs.ContainsByPredicate([this](const S47Ledger::FLeadRef& Ref){return !LeadMarked(Ref.Lead);});
  if(bNew&&!ReadIssues.Contains(S47MailLocal::LedgerHint))
  {
   ReadIssues.Add(S47MailLocal::LedgerHint);
   M->UI->Toast(S47MailLocal::LedgerHintText(),7);
  }
  UE_LOG(LogTemp,Display,TEXT("S47: mail: the Ledger no. %d read, %d articles point at a place"),No,Refs.Num());
  M->RequestAutosave();
 }
 if(M->Story)
 {
  FS47Doc D;D.Id=TEXT("mail-")+ReadKey;D.Title=FString::Printf(TEXT("The Caliche Ledger, %s"),*DayText(Issue.Day));D.Kind=TEXT("typed");D.Stamp=TEXT("THE LEDGER");
  D.Page=S47Ledger::PlainText(Issue);D.Transcript=D.Page;M->Story->AddDoc(D);
 }
 ShowLedger(Issue,Sheet,FString::Printf(TEXT("THE LEDGER / %s"),*S47Ledger::DateLine(Issue.Date)),No);
}
void FS47Mail::ShowLedger(const S47Ledger::FIssue& Issue,int32 Sheet,const FString& Title,int32 No)
{
 if(!M||!M->UI||!M->Freeform)return;
 auto& Ses=*M->Freeform;
 const int32 Side=FMath::Clamp(Sheet,0,S47Ledger::Sheets-1);
 const TWeakPtr<uint8> Guard=Life;
 // D45-24-20261110: a story is clicked on the page (S47Ledger::Page) or by its button under it, for the keyboard and the pad;
 // both pencil it in by MarkLead, and both show what the map knows.
 S47Ledger::FLeadHooks Hooks;
 Hooks.Marked=[this,Guard](const FString& Id){return Guard.IsValid()&&LeadMarked(Id);};
 Hooks.Mark=[this,Guard,No](const FString& Id,const FString& Said){if(Guard.IsValid())MarkLead(No,Id,Said);};
 TArray<S47Kit::FW> Panel;
 Panel.Add(S47Ledger::Page(Issue,Side,Hooks));
 TArray<S47Kit::FW> Pencil;
 for(const S47Ledger::FLeadRef& Ref:S47Ledger::LeadsOf(Issue))
 {
  if(Ref.Sheet!=Side)continue;
  const FString LeadId=Ref.Lead,HeadText=Ref.Head,Where=S47Ledger::LeadTitle(Ref.Lead);
  Pencil.Add(S47Kit::ButtonLive(TAttribute<FString>::CreateLambda([this,Guard,LeadId,HeadText,Where]
  {
   return Guard.IsValid()&&LeadMarked(LeadId)?FString::Printf(TEXT("On your map: %s (%s)"),*HeadText,*Where):FString::Printf(TEXT("Pencil it in on the map: %s (%s)"),*HeadText,*Where);
  }),[this,Guard,No,LeadId,HeadText]{if(Guard.IsValid())MarkLead(No,LeadId,HeadText);},false,TAttribute<bool>::CreateLambda([this,Guard,LeadId]{return Guard.IsValid()&&!LeadMarked(LeadId);})));
 }
 if(!Pencil.IsEmpty())Panel.Add(S47Kit::Row(Pencil));
 // The coupon is clipped from the inside page, where its ad is (the front's ear says so).
 if(Side==1)
 {
  const int32 Until=Issue.Day+6;
  for(const S47Ledger::FAd& Ad:Issue.Ads)
  {
   const S47Ledger::FCoupon* Cp=Ad.Coupon.IsEmpty()?nullptr:S47Ledger::Coupon(Ad.Coupon);if(!Cp)continue;
   const FString Id=Cp->Id,Line=Cp->Line,Through=DayText(Until);
   const bool bValid=Ses.Day()<=Until&&No>0;
   Panel.Add(S47Kit::ButtonLive(TAttribute<FString>::CreateLambda([this,Guard,Id,Line,Through,bValid]
   {
    if(!bValid||!Guard.IsValid())return FString::Printf(TEXT("Coupon: %s. It ran out on %s."),*Line,*Through);
    return Clipped(Id)?FString::Printf(TEXT("Clipped: %s, good through %s"),*Line,*Through):FString::Printf(TEXT("Clip the coupon: %s, good through %s"),*Line,*Through);
   }),[this,Guard,Id,Until]{if(Guard.IsValid()&&!Clipped(Id))Clip(Id,Until);},false,TAttribute<bool>::CreateLambda([this,Guard,Id,bValid]{return bValid&&Guard.IsValid()&&!Clipped(Id);})));
  }
 }
 TArray<S47Kit::FW> Foot;
 const S47Ledger::FIssue Kept=Issue;
 Foot.Add(S47Kit::Button(Side==0?TEXT("Turn the page"):TEXT("Back to the front"),[this,Guard,Kept,Side,Title,No]{if(Guard.IsValid())ShowLedger(Kept,1-Side,Title,No);},true));
 Foot.Add(S47Kit::Button(TEXT("Fold the paper"),[this]{if(M&&M->UI)M->UI->Close();}));
 Panel.Add(S47Kit::Row(Foot));
 if(M->Audio)M->Audio->Sfx(TEXT("paper0"),FS47Sound{.Gain=.5f});
 M->UI->Open(S47Kit::Shell(M->UI,Title,S47Kit::Col(Panel),1120));
}
bool FS47Mail::LeadMarked(const FString& Lead) const
{
 if(!M||!M->Freeform)return false;
 const auto& Ses=*M->Freeform;
 if(Lead==TEXT("wagon"))return Ses.Predecessor&&(Ses.Predecessor->Heard(Lead)||Ses.Predecessor->Found(Lead));
 return Ses.Places&&(Ses.Places->Heard(Lead)||Ses.Places->Found(Lead));
}
bool FS47Mail::MarkLead(int32 No,const FString& Lead,const FString& Head)
{
 if(!M||!M->Freeform)return false;
 auto& Ses=*M->Freeform;
 const FString Where=S47Ledger::LeadTitle(Lead);
 const bool bWagon=Lead==TEXT("wagon");
 if(Where.IsEmpty()||(bWagon&&!Ses.Predecessor)||(!bWagon&&!Ses.Places))return false;
 const bool bFound=bWagon?Ses.Predecessor->Found(Lead):Ses.Places->Found(Lead);
 const FString Key=S47Keys::MapSaid();
 if(bFound||LeadMarked(Lead))
 {
  if(M->UI)M->UI->Toast(bFound?FString::Printf(TEXT("You have already been to %s. It is on your map (%s)."),*Where,*Key):FString::Printf(TEXT("Already on your map: %s (%s)."),*Where,*Key),4);
  return false;
 }
 if(bWagon)Ses.Predecessor->HearOf(Lead);else Ses.Places->Hear(Lead);
 if(M->UI)M->UI->Toast(FString::Printf(TEXT("Marked on your map: %s (%s)."),*Where,*Key),5);
 if(M->Audio)M->Audio->Sfx(TEXT("paper1"),FS47Sound{.Gain=.4f});
 UE_LOG(LogTemp,Display,TEXT("S47: mail: the Ledger no. %d: \"%s\" pencilled onto the map by the player (%s)"),No,*Head,*Lead);
 M->RequestAutosave();
 return true;
}
TArray<S47Ledger::FLeadRef> FS47Mail::IssueLeads(int32 No) const
{
 S47Ledger::FIssue Issue;
 return IssueOf(No,Issue)?S47Ledger::LeadsOf(Issue):TArray<S47Ledger::FLeadRef>();
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
 if(Pc.Kind==TEXT("ledger"))return FString::Printf(TEXT("The Caliche Ledger, %s"),*DayText(Pc.Day));
 if(Pc.Kind==TEXT("catalog"))return TEXT("Western Surplus & Radio Supply, spring catalog");
 if(Pc.Kind==TEXT("parcel"))return TEXT("A parcel from Western Surplus, Albuquerque");
 if(Pc.Kind==TEXT("letter"))return Pc.Ref.IsEmpty()?FString(TEXT("A letter")):Pc.Ref; // (D48: a reply to PostOut carries its title)
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
// PT-MAIL-20261109 (Tom: the letters in the box were gone once he had opened the mail order, and he never read them). The box was
// emptied the moment it was opened and what came was listed on that one panel only: the catalog's page, the parcel lifted down or the
// order sent closed it, and opened again the box listed nothing new and kept Dr. Ward's papers off its KEPT list (they had gone
// quietly to the OPS tray). Now what is taken out stays NOT READ YET on the box's panel until it is read, and the list of things to do
// and the parcel's note say so.
TArray<FS47Mail::FPiece> FS47Mail::Unbox()
{
 TArray<FPiece> Out;
 for(int32 I=0;I<Box.Num();)
 {
  const FPiece Pc=Box[I];
  if(Pc.Kind==TEXT("parcel")){++I;continue;}
  Box.RemoveAt(I);Fetched.Add(Pc);Out.Add(Pc);Unread.Add(Pc.Id);
 }
 // Only the last few stay on the desk to be read again; the rest are filed where they belong. What is not read yet goes last.
 while(Fetched.Num()>12)
 {
  int32 Old=Fetched.IndexOfByPredicate([this](const FPiece& Pc){return !IsUnread(Pc);});
  if(Old<0)Old=0;
  Unread.Remove(Fetched[Old].Id);Fetched.RemoveAt(Old);
 }
 // The letters before the catalog: it is a book to sit down with, they are what came.
 Out.StableSort([](const FPiece& A,const FPiece& B){return A.Kind!=TEXT("catalog")&&B.Kind==TEXT("catalog");});
 return Out;
}
bool FS47Mail::IsUnread(const FPiece& Pc) const
{
 if(!Unread.Contains(Pc.Id))return false;
 // Dr. Ward's are read at OPS as well: gone from the tray, read. A paper read from elsewhere counts too.
 if(Pc.Kind==TEXT("ward")&&M&&M->Freeform&&M->Freeform->Predecessor&&!M->Freeform->Predecessor->InTray(Pc.Ref))return false;
 if(Pc.Kind==TEXT("ledger")&&ReadIssues.Contains(Pc.Id))return false;
 // Critic round 7: the rest are filed as the notebook's papers (FileDoc, "mail-<id>"); opened there, they are read.
 if(M&&M->Freeform&&M->Freeform->JournalEntryRead(TEXT("mail-")+Pc.Id))return false;
 return true;
}
void FS47Mail::MarkRead(const FString& Id){Unread.Remove(Id);}
TArray<FS47Mail::FPiece> FS47Mail::UnreadPieces() const
{
 TArray<FPiece> Out;
 for(const FPiece& Pc:Fetched)if(Pc.Kind!=TEXT("parcel")&&IsUnread(Pc))Out.Add(Pc);
 Out.StableSort([](const FPiece& A,const FPiece& B){return A.Kind!=TEXT("catalog")&&B.Kind==TEXT("catalog");});
 return Out;
}
FString FS47Mail::UnreadNote() const
{
 int32 Ward=0,Other=0;
 for(const FPiece& Pc:UnreadPieces())++(Pc.Kind==TEXT("ward")?Ward:Other);
 if(Ward+Other==0)return FString();
 if(Other==0)return TEXT("Not read yet: Dr. Ward's mail, in the OPS tray.");
 const FString Papers=Other==1?FString(TEXT("one paper from the mailbox")):FString::Printf(TEXT("%d papers from the mailbox"),Other);
 return Ward>0?FString::Printf(TEXT("Not read yet: %s, and Dr. Ward's mail in the OPS tray."),*Papers):FString::Printf(TEXT("Not read yet: %s. %s at the box."),*Papers,*S47Keys::Said(TEXT("use")));
}
TArray<FS47Mail::FPiece> FS47Mail::TakeOut()
{
 TArray<FPiece> Out;
 if(!M||!M->Freeform)return Out;
 auto& Ses=*M->Freeform;
 // Everything but a parcel comes out at once; a parcel wants both hands and waits on top of the box for them.
 int32 Marked=0;
 Out=Unbox();
 for(const FPiece& Pc:Out)
 {
  if(Pc.Kind==TEXT("ward")&&Ses.Predecessor)Ses.Predecessor->FromMail(Pc.Ref);
  if(Pc.Id==TEXT("ward:letter"))Ses.StationKeyFound(); // FIX-20261110: the station's keys are in the envelope
  if(Pc.Kind==TEXT("catalog"))bCatalog=true;
  // D48-20261110: a check in a letter (the weather service's finder's fee for the balloon's instrument) is cashed to the budget here, once.
  if(Pc.Kind==TEXT("letter"))if(const int32 Dollars=FMath::Clamp(Pc.Goods.FindRef(TEXT("dollars")),0,1000);Dollars>0)
  {
   Ses.Earn(Dollars);
   if(M->UI)M->UI->Toast(FString::Printf(TEXT("In the envelope, a check for $%d. It goes to the station's budget."),Dollars),6);
   UE_LOG(LogTemp,Display,TEXT("S47: mail: a check for $%d in %s, cashed"),Dollars,*Pc.Id);
  }
  // The map with a place circled: the circle goes onto the player's own map, as a place heard of.
  if(Pc.Kind==TEXT("nobody")&&!Pc.Place.IsEmpty()&&Ses.Places&&FS47Places::Def(Pc.Place)&&!Ses.Places->Heard(Pc.Place)&&!Ses.Places->Found(Pc.Place)){Ses.Places->Hear(Pc.Place);++Marked;}
  FileDoc(Pc);
 }
 if(!Out.IsEmpty())
 {
  if(M->Audio){M->Audio->Sfx(TEXT("metalTap"),FS47Sound{.Gain=.55f});M->Audio->Sfx(TEXT("paper0"),FS47Sound{.Gain=.45f,.When=.35f});}
  if(Marked>0&&M->UI)M->UI->Toast(FString::Printf(TEXT("You copy the pencil circle onto your map (%s)."),*S47Keys::MapSaid()),5);
  UE_LOG(LogTemp,Display,TEXT("S47: mail: %d taken out of the box, day %d"),Out.Num(),Ses.Day());
  M->RequestAutosave();
 }
 return Out;
}
void FS47Mail::ReadPiece(const FPiece& Pc)
{
 MarkRead(Pc.Id);
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
  if(M->UI)M->UI->Toast(TEXT("Your hands are full. The parcel stays on the box until they are free."),5);
  return false;
 }
 for(const FString& Kit:Gear)Ses.ReceiveKit(Kit);
 Box.RemoveAt(At);Fetched.Add(Pc);FileDoc(Pc);
 Model->SetParcel(ParcelOnBox());
 if(M->Audio)M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.5f});
 UE_LOG(LogTemp,Display,TEXT("S47: mail: parcel %s lifted down: %d supplies in hand, %d pieces of gear signed for"),*Id,Stock.Num(),Gear.Num());
 if(M->UI)
 {
  M->UI->Close();
  const FString Note=UnreadNote();
  M->UI->Toast((Stock.IsEmpty()?FString(TEXT("Signed for. The gear is the station's now: see OPS, SUPPLIES.")):FString(TEXT("The parcel is in your hands. GARAGE RECEIVING unpacks it into stock, or load it on the truck.")))
   +(Note.IsEmpty()?FString():TEXT(" ")+Note),Note.IsEmpty()?6:9);
 }
 M->RequestAutosave();
 return true;
}
void FS47Mail::OpenBox()
{
 if(!M||!M->UI||!M->Freeform||!Live())return;
 const bool bWasLocked=M->Freeform->bStationLocked;
 const TArray<FPiece> Came=TakeOut();
 TArray<S47Kit::FW> Panel;
 const TWeakPtr<uint8> Guard=Life;
 Panel.Add(S47Kit::Text(TEXT("A rural box on a weathered four-by-four, the galvanizing gone dull and gray, rust in thin lines at the seams, SARO and BOX 19 painted by hand on the side. The door drops on its hinge.")));
 const auto ReadButton=[&](const FPiece& Pc,bool bPrimary)
 {
  const FPiece Copy=Pc;
  return S47Kit::Button(TEXT("Read: ")+PieceTitle(Pc),[this,Guard,Copy]{if(Guard.IsValid())ReadPiece(Copy);},bPrimary);
 };
 // What came, and what came before and is not read yet: it stays here until it is (PT-MAIL-20261109).
 const TArray<FPiece> Unopened=UnreadPieces();
 if(!Unopened.IsEmpty())
 {
  Panel.Add(S47Kit::Section(Came.IsEmpty()?TEXT("NOT READ YET"):TEXT("WHAT CAME")));
  bool bWard=false;
  for(int32 I=0;I<Unopened.Num();++I){Panel.Add(ReadButton(Unopened[I],I==0));bWard|=Unopened[I].Kind==TEXT("ward");}
  // FIX-20261110: the keys came out with the letter, and the panel says so where the letter is listed.
  if(bWasLocked&&!M->Freeform->bStationLocked)Panel.Add(S47Kit::Paper(TEXT("In the envelope with the letter: a ring of keys on a paper tag, SARO in ballpoint. The station's outside doors are yours to open.")));
  if(bWard)Panel.Add(S47Kit::Small(TEXT("Dr. Ward's mail goes to the OPS tray as well, and waits there until it is read.")));
 }
 bool bParcelShown=false;
 for(const FPiece& Pc:Box)if(Pc.Kind==TEXT("parcel"))
 {
  if(!bParcelShown){Panel.Add(S47Kit::Section(TEXT("ON TOP OF THE BOX")));bParcelShown=true;}
  Panel.Add(S47Kit::Paper(Pc.Text));
  const FString Id=Pc.Id;
  Panel.Add(S47Kit::Button(TEXT("Lift the parcel down"),[this,Guard,Id]{if(Guard.IsValid())LiftParcel(Id);},true));
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
 // D48-20261110: the balloon's instrument box: left here for the carrier (postage paid, a check comes back), or waiting in the box.
 for(const FOut& Sent:Outbound)Panel.Add(S47Kit::Small(Sent.What+TEXT(", in the box for the carrier, the flag up.")));
 if(M->Freeform->Balloon&&M->Freeform->Balloon->Carried())
  Panel.Add(S47Kit::Button(TEXT("Leave the balloon's instrument box for the carrier / postage paid, a finder's fee comes back"),[this,Guard]{if(Guard.IsValid()&&M&&M->Freeform&&M->Freeform->Balloon){M->Freeform->Balloon->Mail();OpenBox();}},true));
 if(bCatalog&&!Unread.Contains(TEXT("catalog")))Panel.Add(S47Kit::Button(TEXT("The Western Surplus catalog / order by mail"),[this,Guard]{if(Guard.IsValid())OpenCatalog();}));
 // Papers kept from before, to read again: the newest few.
 {
  TArray<S47Kit::FW> Again;
  for(int32 I=Fetched.Num()-1;I>=0&&Again.Num()<4;--I)
  {
   const FPiece& Pc=Fetched[I];
   if(Pc.Kind==TEXT("ward")||Pc.Kind==TEXT("catalog")||Pc.Kind==TEXT("parcel")||IsUnread(Pc))continue;
   Again.Add(ReadButton(Pc,false));
  }
  if(!Again.IsEmpty()){Panel.Add(S47Kit::Section(TEXT("KEPT")));Panel.Append(Again);}
 }
 Panel.Add(S47Kit::Button(TEXT("Shut the box"),[this]{if(M&&M->UI)M->UI->Close();}));
 if(Came.IsEmpty()&&M->Audio)M->Audio->Sfx(TEXT("metalTap"),FS47Sound{.Gain=.4f});
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("THE MAILBOX AT THE ROAD"),S47Kit::Col(Panel),820));
 // Critics, 10 Nov: the keys came out with the letter, and the letter is what says what the job is. It is opened in the hand at
 // once, not left as a button a new operator can shut the box on.
 if(bWasLocked&&!M->Freeform->bStationLocked)for(const FPiece& Pc:Came)if(Pc.Id==TEXT("ward:letter")){const FPiece Copy=Pc;ReadPiece(Copy);break;}
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
 bFlag=!Outbound.IsEmpty();
 UE_LOG(LogTemp,Display,TEXT("S47: mail: order %d taken back, $%d back, flag down"),Ord.Serial,Ord.Paid);
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
  if(WriteOrder(Cart,Why)){if(M&&M->UI){const FString Note=UnreadNote();M->UI->Close();M->UI->Toast(FString(TEXT("The order is in the box and the flag is up. The carrier takes it around noon."))+(Note.IsEmpty()?FString():TEXT(" ")+Note),Note.IsEmpty()?5:8);}}
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
FString FS47Mail::NobodyText(int32 Variant,const FString& Place)
{
 // Nothing from the past and no date that has not come: a sheet with nothing on it, a postcard of the town, a map of the county.
 if(Variant==1)return FString::Printf(TEXT("An envelope with no stamp and no postmark. NIGHT OPERATOR, SARO, in pencil.\n\nInside, a county road map from a gas station rack, folded the wrong way. One place on it is circled in pencil, pressed hard enough to dent the paper: %s. Nothing else on the map is marked, and nothing is written on it."),*Place);
 if(Variant==2)return TEXT("A picture postcard of the Plaza in Caliche, the marquee lit, the colors gone soft. No stamp, no postmark. The message side is blank. The address side says SARO, in pencil, and nothing else.");
 return TEXT("A plain white envelope: no stamp, no postmark, no return address. NIGHT OPERATOR, SARO, in pencil, in capitals.\n\nInside, one sheet of typing paper folded in three. Nothing on it. Held up to the light, nothing on it either.");
}

// ---------- what the station is told ----------
FString FS47Mail::TaskLine() const
{
 if(!Live())return FString();
 // The player's own order, and when what they sent for is due. Nothing that tells what has come: the box shows that.
 if(Orders.ContainsByPredicate([](const FOrder& O){return O.Picked==0;}))return TEXT("MAILBOX / Your order is in the box at the road, the flag up for the carrier.");
 if(!Outbound.IsEmpty())return TEXT("MAILBOX / ")+Outbound[0].What+TEXT(" is in the box at the road, the flag up for the carrier."); // D48
 // Taken out and not read (PT-MAIL-20261109). Dr. Ward's own list line says hers (FS47Predecessor::TaskLine): only the rest here.
 {
  int32 Other=0;for(const FPiece& Pc:UnreadPieces())Other+=Pc.Kind!=TEXT("ward")&&Pc.Kind!=TEXT("catalog");
  if(Other>0)return Other==1?FString(TEXT("MAILBOX / One paper you took out of the box is not read yet.")):FString::Printf(TEXT("MAILBOX / %d papers you took out of the box are not read yet."),Other);
 }
 int32 Due=0;
 for(const FOrder& Ord:Orders)Due=Due==0?Ord.Due:FMath::Min(Due,Ord.Due);
 for(const FPiece& Pc:Box)if(Pc.Kind==TEXT("parcel"))Due=Due==0?Pc.Day:FMath::Min(Due,Pc.Day);
 return Due>0?FString::Printf(TEXT("MAIL ORDER / A parcel from Western Surplus, due about %s."),*DayText(Due)):FString();
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
 namespace L=S47MailLocal;
 TestOn();if(!M||!M->Freeform){Why=TEXT("Freeform only");return false;}
 const int32 Today=M->Freeform->Day(),Before=Box.Num();
 LastRound=FMath::Max(LastRound,Today);
 Round(Today);
 // Shown as it would be seen: the car comes up the last stretch and stands at the box (the round is made already).
 StartCar(Today,false,L::StopZ+140.,true);
 Why=FString::Printf(TEXT("The carrier came: %d new in the box, %d in it now, the flag %s."),Box.Num()-Before,Box.Num(),bFlag?TEXT("up"):TEXT("down"));
 return true;
}
bool FS47Mail::TestCarrier()
{
 namespace L=S47MailLocal;
 TestOn();if(!M||!M->Freeform)return false;
 return StartCar(M->Freeform->Day(),true,L::StopZ+320.,false);
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
 NoteFindings();
 const TArray<S47Ledger::FFact> Facts=WeekFacts(FromDay,Ses.Day());
 TMap<FString,int32> Used;for(const FKept& Was:Issues)for(const FString& Key:Was.Keys)Used.Add(Key,Was.No);
 S47Ledger::FIssue Issue;Issue.No=No;Issue.Day=Ses.Day();Issue.Date=Ses.DateOfDay(Ses.Day());
 Issue.Front=S47Ledger::Front(Ses.Seed(),No,Issue.Date,Facts,Used,UnknownId,UnknownTitle);
 Issue.Log=S47Ledger::LogLines(Facts,Issue.Front,Issue.Day,2);
 S47Ledger::Fill(Issue,Ses.Seed(),Facts.ContainsByPredicate([](const S47Ledger::FFact& Fact){return Fact.Topic==S47Ledger::ETopic::Helped;}));
 UE_LOG(LogTemp,Display,TEXT("S47: mail: test: this week's Ledger from %d facts since day %d"),Facts.Num(),FromDay);
 // D45-24-20261110: the same panel as the box's paper, both sheets, the stories clickable (on the map of this developer run).
 ShowLedger(Issue,0,TEXT("THE LEDGER / AS IT WOULD BE PRINTED TODAY"),0);
}
void FS47Mail::TestParcel()
{
 TestOn();
 const int32 Serial=NextSerial++;TMap<FString,int32> Goods;Goods.Add(TEXT("cell"),2);
 FPiece Pc;Pc.Id=FString::Printf(TEXT("parcel:%d"),Serial);Pc.Kind=TEXT("parcel");Pc.Ref=FString::FromInt(Serial);Pc.Goods=Goods;Pc.Text=SlipText(Serial,Goods,8);
 Pc.Day=M&&M->Freeform?M->Freeform->Day():1;Box.Add(Pc);Posted.Add(Pc.Id);
}
void FS47Mail::TestNobody(int32 Variant)
{
 TestOn();
 const int32 V=FMath::Clamp(Variant,0,2),Today=M&&M->Freeform?M->Freeform->Day():1;
 FPiece Pc;Pc.Id=FString::Printf(TEXT("nobody:test-%d-%d"),V,Rounds+Box.Num());Pc.Kind=TEXT("nobody");Pc.Ref=FString::FromInt(V);Pc.Day=Today;
 if(V==1)
 {
  // A place the player has not heard of if there is one, else any on the land.
  if(M&&M->Freeform&&M->Freeform->Places)Pc.Place=M->Freeform->Places->NextUnknown(Today*7);
  if(!FS47Places::Def(Pc.Place)&&!FS47Places::Defs().IsEmpty())Pc.Place=FS47Places::Defs()[0].Id;
 }
 const FS47Places::FDef* D=FS47Places::Def(Pc.Place);
 Pc.Text=NobodyText(V,D?FString(D->Title):FString(TEXT("the windmill")));
 Box.Add(Pc);Posted.Add(Pc.Id);
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
 for(int32 V=0;V<3;++V)Out.Add(NobodyText(V,TEXT("the windmill on the Baca lease")));
 TMap<FString,int32> Goods;Goods.Add(TEXT("cell"),2);Goods.Add(TEXT("kit:geiger_tube"),1);Out.Add(SlipText(3,Goods,28));
 Out.Add(TowText(TEXT("THURSDAY, MAY 15"),40,true));Out.Add(TowText(TEXT("THURSDAY, MAY 15"),40,false));
 // What the box, its panel and the list say (the strings above are kept in step with these by hand).
 for(const TCHAR* Line:{TEXT("A rural box on a weathered four-by-four, the galvanizing gone dull and gray, rust in thin lines at the seams, SARO and BOX 19 painted by hand on the side. The door drops on its hinge."),
  TEXT("A rural mailbox on a post, SARO and BOX 19 painted on the side, the red flag up for the carrier."),TEXT("A rural mailbox on a post, SARO and BOX 19 painted on the side. The flag is down."),
  TEXT("Nothing in it. The carrier comes up the highway around noon, Monday through Saturday."),TEXT("MAILBOX / Your order is in the box at the road, the flag up for the carrier."),
  TEXT("MAIL ORDER / A parcel from Western Surplus, due about Friday, May 2."),TEXT("The Caliche Ledger, Thursday, May 1"),TEXT("A bill from the county road department"),
  TEXT("Your hands are full. The parcel stays on the box until they are free."),
  TEXT("The order is in the box and the flag is up. The carrier takes it around noon."),TEXT("NOT READ YET"),TEXT("Dr. Ward's mail goes to the OPS tray as well, and waits there until it is read."),
  TEXT("Not read yet: Dr. Ward's mail, in the OPS tray."),TEXT("Not read yet: 2 papers from the mailbox, and Dr. Ward's mail in the OPS tray."),
  TEXT("MAILBOX / One paper you took out of the box is not read yet."),TEXT("MAILBOX / 2 papers you took out of the box are not read yet.")})Out.Add(Line);
 Out.Add(FString::Printf(TEXT("You copy the pencil circle onto your map (%s)."),*S47Keys::MapSaid()));
 // D45-24-20261110: the paper's pencil: the hint, the buttons under the page and what a click says.
 {
  const FString Key=S47Keys::MapSaid(),Where=TEXT("the windmill on the Baca lease"),Head=TEXT("LIGHTS AGAIN");
  Out.Add(S47MailLocal::LedgerHintText());
  Out.Add(FString::Printf(TEXT("Marked on your map: %s (%s)."),*Where,*Key));Out.Add(FString::Printf(TEXT("Already on your map: %s (%s)."),*Where,*Key));
  Out.Add(FString::Printf(TEXT("You have already been to %s. It is on your map (%s)."),*Where,*Key));
  Out.Add(FString::Printf(TEXT("Pencil it in on the map: %s (%s)"),*Head,*Where));Out.Add(FString::Printf(TEXT("On your map: %s (%s)"),*Head,*Where));
  for(const TCHAR* Line:{TEXT("Turn the page"),TEXT("Back to the front"),TEXT("Fold the paper")})Out.Add(Line);
 }
 Out.Add(FString::Printf(TEXT("Not read yet: one paper from the mailbox. %s at the box."),*S47Keys::Said(TEXT("use"))));
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
 // The car's road: the northbound lane, in to the box's edge of the pavement, its window at the box, never across the box.
 {
  namespace L=S47MailLocal;
  const bool bLane=L::LaneX>S47Legs::SARO_HWY.X&&L::LaneX<S47Legs::SARO_HWY.X+S47Legs::SARO_HWY.Half;
  const bool bCurb=L::CurbX+FS47MailCarModel::Width*.5<FS47MailBoxModel::PostX-.38&&L::CurbX>L::LaneX;
  const bool bTime=L::Lead>0&&L::Late>0&&FMath::Sqrt(2.*L::Brake*L::Decide)>=L::Cruise;
  Check(bLane&&bCurb&&bTime,TEXT("the carrier drives the northbound lane, pulls in short of the box, and has room to stop from its pace"));
 }
 // D48-20261110: something left for the carrier that is no order (the balloon's box): once, the flag up, its reply kept through a
 // Continue with its title and its check, and the carrier's round takes it (RoundBrings).
 {
  FS47Mail P(nullptr);P.Begin(nullptr,false);
  FPiece R;R.Id=TEXT("letter:t");R.Kind=TEXT("letter");R.Ref=TEXT("A letter with a check");R.Text=TEXT("Thank you.");R.Goods.Add(TEXT("dollars"),5);
  const bool bPost=P.PostOut(TEXT("t"),TEXT("The thing"),3,R),bFlag=P.FlagUp(),bWait=P.OutWaiting(TEXT("t")),bOnce=!P.PostOut(TEXT("t"),TEXT("The thing"),3,R),bNew=!P.Ever(TEXT("letter:t")),bRound=P.RoundBrings(1),bTask=P.TaskLine().IsEmpty();
  if(!(bPost&&bFlag&&bWait&&bOnce&&bNew&&bRound&&bTask))UE_LOG(LogTemp,Warning,TEXT("S47: mail D48 parts post %d flag %d wait %d once %d new %d round %d task %d (%s)"),bPost,bFlag,bWait,bOnce,bNew,bRound,bTask,*P.TaskLine());
  const bool bLeft=bPost&&bFlag&&bWait&&bOnce&&bNew&&bRound&&bTask;
  FS47Mail Q(nullptr);Q.Begin(S47J::Read(S47J::Write(P.Save())),true);
  const bool bKept=Q.OutWaiting(TEXT("t"))&&Q.FlagUp()&&Q.Outbound.Num()==1&&Q.Outbound[0].Reply.Kind==TEXT("letter")&&Q.Outbound[0].Reply.Ref==R.Ref&&Q.Outbound[0].Reply.Goods.FindRef(TEXT("dollars"))==5&&Q.Outbound[0].Rounds==3;
  FPiece NoReply;
  Check(bLeft&&bKept&&!Q.PostOut(TEXT("u"),TEXT("Nothing"),1,NoReply)&&Q.PieceTitle(R)==R.Ref,TEXT("the balloon's box left for the carrier: once, the flag up, the reply with its check kept through a Continue, and the round comes for it"));
 }
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
 {
  bool bWords=true;FString Bad;for(const FString& St:AllText()){FString Word;if(!S47Ledger::Clean(St,Word)){bWords=false;Bad=Word;}}
  Check(bWords,TEXT("the catalog, the slip, the tow bill, the box's words and what nobody sent say nothing they must not (Story's far side, brands, British words, the county that is not one)"));
  bool bNoPast=true;for(int32 V=0;V<3;++V){const FString T=NobodyText(V,TEXT("the cairn")).ToLower();bNoPast&=!T.Contains(TEXT("photo"))&&!T.Contains(TEXT("print"))&&!T.Contains(TEXT("processed"))&&!T.Contains(TEXT("yellow"))&&!T.Contains(TEXT("has not come"));}
  Check(bNoPast,TEXT("nothing with no sender is an old picture or carries a date that has not come"));
 }
 // A new game or a save from before the post finds the catalog in the box, the flag down: the flag is never the carrier's.
 FS47Mail Fresh(nullptr);Fresh.Begin(nullptr,true);
 Check(Fresh.Holds(TEXT("catalog"))&&!Fresh.FlagUp()&&Fresh.MailIn()&&Fresh.Ever(TEXT("catalog")),TEXT("a new game or an older save: the catalog is in the box and the flag is down"));
 // Everything survives a Continue.
 {
  FS47Mail A(nullptr);A.Begin(nullptr,false);
  FPiece Ward;Ward.Id=TEXT("ward:letter");Ward.Kind=TEXT("ward");Ward.Ref=TEXT("letter");A.Post(Ward,true);
  FPiece Sheet;Sheet.Id=TEXT("bill:week-1");Sheet.Kind=TEXT("bill");Sheet.Ref=TEXT("week-review-1");A.Post(Sheet,false);
  FPiece Box2;Box2.Id=TEXT("parcel:4");Box2.Kind=TEXT("parcel");Box2.Goods.Add(TEXT("cell"),3);Box2.Goods.Add(TEXT("kit:film"),1);Box2.Text=SlipText(4,Box2.Goods,30);A.Post(Box2,true);
  FPiece Map;Map.Id=TEXT("nobody:1");Map.Kind=TEXT("nobody");Map.Ref=TEXT("1");Map.Place=TEXT("cairn");Map.Text=NobodyText(1,TEXT("the cairn"));A.Post(Map,true);
  FOrder Ord;Ord.Serial=5;Ord.Paid=21;Ord.Picked=3;Ord.Due=5;Ord.Goods.Add(TEXT("relay"),1);A.Orders.Add(Ord);
  FOrder Waiting;Waiting.Serial=6;Waiting.Paid=8;Waiting.Goods.Add(TEXT("cell"),2);A.Orders.Add(Waiting);A.NextSerial=7;A.bFlag=true;
  FClip Clipping;Clipping.Id=TEXT("cell3");Clipping.What=TEXT("cell");Clipping.Price=3;Clipping.Until=10;A.Coupons.Add(Clipping);
  FKept K;K.No=1;K.Day=4;K.bThanks=true;K.Heads={TEXT("A")};K.Bodies={TEXT("B")};K.Leads={TEXT("windmill")};K.Keys={TEXT("KEY")};K.Log={TEXT("Tuesday night. Something.")};K.LogAgo={2};K.Photo=TEXT("data:image/jpeg;base64,/9j/4AAQSkZJRgABAQAAAAAAAAAAAAAA");A.Issues.Add(K);
  FKept Plain;Plain.No=2;Plain.Day=11;Plain.Heads={TEXT("C")};Plain.Bodies={TEXT("D")};Plain.Leads={FString()};Plain.Photo=TEXT("data:image/png;base64,AAAA");A.Issues.Add(Plain);
  FSeen Note;Note.Id=TEXT("cattle_ring_d3");Note.Day=3;Note.Place=TEXT("the church");A.Seen.Add(Note);
  A.ReadIssues.Add(TEXT("ledger:1"));A.LastRound=4;A.Rounds=3;A.NobodySent=1;
  FS47Mail B(nullptr);B.Begin(S47J::Read(S47J::Write(A.Save())),true);
  const bool bBox=B.Holds(TEXT("ward:letter"))&&B.Holds(TEXT("parcel:4"))&&B.Holds(TEXT("catalog"))&&B.Holds(TEXT("nobody:1"))&&B.Coming(TEXT("bill:week-1"))&&B.FlagUp();
  const FPiece* Back=B.Box.FindByPredicate([](const FPiece& Pc){return Pc.Id==TEXT("parcel:4");});
  const FPiece* Circle=B.Box.FindByPredicate([](const FPiece& Pc){return Pc.Id==TEXT("nobody:1");});
  const bool bGoods=Back&&Back->Goods.FindRef(TEXT("cell"))==3&&Back->Goods.FindRef(TEXT("kit:film"))==1&&Back->Text.Contains(TEXT("PACKING SLIP"))&&Circle&&Circle->Place==TEXT("cairn");
  const bool bRest=B.Orders.Num()==2&&B.Orders[0].Due==5&&B.Orders[0].Goods.FindRef(TEXT("relay"))==1&&B.NextSerial==7&&B.Coupons.Num()==1&&B.Coupons[0].Until==10
   &&B.Issues.Num()==2&&B.Issues[0].Photo.EndsWith(TEXT("AAAAAAAA"))&&B.Issues[1].Photo.IsEmpty()&&B.Issues[0].bThanks&&B.Issues[0].Leads[0]==TEXT("windmill")&&B.Issues[0].Keys[0]==TEXT("KEY")&&B.Issues[0].Log.Num()==1&&B.Issues[0].LogAgo[0]==2
   &&B.Seen.Num()==1&&B.Seen[0].Place==TEXT("the church")&&B.ReadIssues.Contains(TEXT("ledger:1"))&&B.LastRound==4&&B.Rounds==3&&B.NobodySent==1;
  Check(bBox&&bGoods&&bRest,TEXT("the box, the post on its way, orders, coupons, printed papers with their templates, log and photo (D46-1: a photo that is not a small JPEG is dropped, the drawn picture stays), the noted findings and the rounds survive a Continue"));
  Check(!B.Post(Ward,true),TEXT("nothing is posted twice, a Continue included"));
  // The flag follows the orders, not the box: with the waiting order taken back it is down, the box still full.
  B.Orders.RemoveAll([](const FOrder& O){return O.Picked==0;});B.Begin(S47J::Read(S47J::Write(B.Save())),true);
  Check(!B.FlagUp()&&B.MailIn(),TEXT("the flag is up only for an order waiting for the carrier, never for what came"));
 }
 {
  // D45-24-20261110: with no game nothing is pencilled in, and the mark that the hint was given is kept among the read across a
  // Continue without reading as a paper (no new save key).
  FS47Mail H(nullptr);H.Begin(nullptr,false);H.ReadIssues.Add(S47MailLocal::LedgerHint);
  FS47Mail G(nullptr);G.Begin(S47J::Read(S47J::Write(H.Save())),true);
  Check(!H.MarkLead(1,TEXT("wagon"),TEXT("SEARCH FOR MISSING PAIR SCALED BACK"))&&!H.LeadMarked(TEXT("windmill"))&&H.IssueLeads(1).IsEmpty()
   &&G.ReadIssues.Contains(S47MailLocal::LedgerHint)&&G.UnreadPieces().Num()==H.UnreadPieces().Num(),
   TEXT("D45-24: the paper marks nothing on its own, and the hint is given once in a game and survives a Continue"));
 }
 {
  // PT-MAIL-20261109 (Tom): the letters went missing when he opened the mail order first. The box with Dr. Ward's letter, a statement,
  // the catalog and a parcel on top; opened, the catalog read first and the parcel lifted down (its panel gone with either), then the
  // box opened again: before, it listed nothing, and her letter had gone to the OPS tray without a word.
  FS47Mail U(nullptr);U.Begin(nullptr,false);
  FPiece Ward;Ward.Id=TEXT("ward:letter");Ward.Kind=TEXT("ward");Ward.Ref=TEXT("letter");U.Post(Ward,true);
  FPiece Sheet;Sheet.Id=TEXT("bill:week-1");Sheet.Kind=TEXT("bill");Sheet.Ref=TEXT("week-review-1");U.Post(Sheet,true);
  FPiece Crate;Crate.Id=TEXT("parcel:3");Crate.Kind=TEXT("parcel");Crate.Goods.Add(TEXT("cell"),2);Crate.Text=SlipText(3,Crate.Goods,8);U.Post(Crate,true);
  const TArray<FPiece> Came=U.Unbox();
  Check(Came.Num()==3&&Came.Last().Kind==TEXT("catalog")&&Came[0].Kind!=TEXT("catalog")&&U.Holds(TEXT("parcel:3"))&&U.UnreadPieces().Num()==3,
   TEXT("PT-MAIL: opened, the box gives up the letters and the catalog (the letters first) and keeps the parcel on top; all three not read yet"));
  // The catalog read first, the parcel lifted down (as LiftParcel files it), and the box opened again.
  U.MarkRead(TEXT("catalog"));
  U.Box.RemoveAll([](const FPiece& Pc){return Pc.Id==TEXT("parcel:3");});U.Fetched.Add(Crate);
  const TArray<FPiece> Again=U.Unbox();const TArray<FPiece> Left=U.UnreadPieces();const FString Note=U.UnreadNote();
  Check(Again.IsEmpty()&&Left.Num()==2&&Left[0].Id==TEXT("ward:letter")&&Left[1].Id==TEXT("bill:week-1")&&Note.Contains(TEXT("OPS tray"))&&Note.Contains(TEXT("one paper")),
   TEXT("PT-MAIL: opened again after the catalog and the parcel, the box still shows Dr. Ward's letter and the statement as not read, and says hers is in the OPS tray"));
  FS47Mail V(nullptr);V.Begin(S47J::Read(S47J::Write(U.Save())),true);
  Check(V.UnreadPieces().Num()==2&&!V.UnreadNote().IsEmpty(),TEXT("PT-MAIL: what is not read yet survives a Continue"));
  U.MarkRead(TEXT("ward:letter"));U.MarkRead(TEXT("bill:week-1"));
  Check(U.UnreadPieces().IsEmpty()&&U.UnreadNote().IsEmpty(),TEXT("PT-MAIL: read, nothing is left to point at"));
 }
 {
  // M7 (C): two tows on one shift day, before the carrier has been, are two statements in the box.
  FS47Mail T(nullptr);T.Begin(nullptr,false);
  FPiece First;First.Id=S47MailLocal::TowBillId(16,3,T.Posted);First.Kind=TEXT("bill");First.Text=T.TowText(TEXT("MAY 13"),60,true);
  const bool bFirst=T.Post(First,true);
  FPiece Second;Second.Id=S47MailLocal::TowBillId(16,3,T.Posted);Second.Kind=TEXT("bill");Second.Text=T.TowText(TEXT("MAY 13"),45,false);
  const bool bSecond=T.Post(Second,true);
  Check(bFirst&&bSecond&&First.Id!=Second.Id&&T.Holds(First.Id)&&T.Holds(Second.Id),TEXT("M7: two tows on the same day are two statements in the box"));
 }
 {
  // Where the paper puts what the nights left: a finding's own words name the place.
  S47Ledger::ETopic T=S47Ledger::ETopic::Count;
  Check(S47MailLocal::TopicOf(TEXT("animals_flee_d4"),T)&&T==S47Ledger::ETopic::Fled&&S47MailLocal::TopicOf(TEXT("house_lit_d4"),T)&&T==S47Ledger::ETopic::LitHouse
   &&S47MailLocal::TopicOf(TEXT("clock_ahead_d4"),T)&&T==S47Ledger::ETopic::ClockFast&&S47MailLocal::TopicOf(TEXT("missing_reported_d4"),T)&&T==S47Ledger::ETopic::Called
   &&!S47MailLocal::TopicOf(TEXT("observation.roof"),T)&&!S47MailLocal::TopicOf(TEXT("landing_logged_d4"),T)&&!S47MailLocal::TopicOf(TEXT("geiger_landing_3"),T),
   TEXT("rabbits running, a lit window and a fast clock each get their own kind of story; the station's own records never reach the paper"));
 }
 return OK;
}

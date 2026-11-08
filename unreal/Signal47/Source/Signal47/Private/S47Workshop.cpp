#include "S47Workshop.h"

namespace
{
 const TCHAR* DonorParts[]={TEXT("housing"),TEXT("recorder"),TEXT("timer"),TEXT("lead")};
 bool ValidKind(const FString& K){for(const TCHAR* D:DonorParts)if(K==D)return true;return false;}
 // Used equipment is not uniformly bad: each donor has a fixed, testable fault.
 FString FaultOf(int32 Donor){const int32 N=(Donor-1)%3;return N==0?TEXT("timer"):N==1?TEXT("recorder"):FString();}
}

const TArray<FS47Workshop::FRecipe>& FS47Workshop::Recipes()
{
 static const TArray<FRecipe> R={
  {TEXT("own_logger"),TEXT("Field logger, operator-built"),TEXT("Your own local-noise logger. Needs no loan or records approval; still calibrate it at this bench."),{TEXT("housing"),TEXT("recorder"),TEXT("timer")},1,0},
  {TEXT("trip_camera"),TEXT("Trip camera, operator-built"),TEXT("A timer and a wire on a recorder housing, with a cheap camera in it. Leave it at marks or a place overnight and read it in the morning. It records only what crosses its wire."),{TEXT("housing"),TEXT("timer")},1,0},
  {TEXT("battery_pack"),TEXT("Logger battery pack"),TEXT("Doubles logger running time from six to twelve hours."),{TEXT("housing")},2,0},
  {TEXT("shielded_lead"),TEXT("Foil-shielded antenna lead"),TEXT("Reduces local electrical pickup on the logger by three quarters."),{TEXT("lead")},0,1},
  {TEXT("cable_splice"),TEXT("Feed-cable splice"),TEXT("Made up from two good leads and foil. Mends the wet feed cable on the array's center branch."),{TEXT("lead"),TEXT("lead")},0,1,TEXT("splice_kit")},
  // GEIGER-20261108: also wants the tube from the stores (FS47FreeformWorld::WorkshopBlock asks FS47Happenings::GeigerBench).
  {TEXT("geiger"),TEXT("Geiger counter, operator-built"),TEXT("A recorder housing, a lead and the tube from the stores, on a dry cell. On foot it ticks. Over scorched or pressed-down ground it ticks faster, and in places the needle pegs. Its dial comes up in your hand when it climbs. It does not say what."),{TEXT("housing"),TEXT("lead")},1,0}};
 return R;
}
const FS47Workshop::FRecipe* FS47Workshop::Recipe(const FString& Id){return Recipes().FindByPredicate([&](const FRecipe& R){return Id==R.Id;});}
FString FS47Workshop::PartTitle(const FString& K)
{
 return K==TEXT("housing")?TEXT("recorder housing"):K==TEXT("recorder")?TEXT("recording module"):K==TEXT("timer")?TEXT("timer board"):K==TEXT("lead")?TEXT("antenna lead"):K;
}
void FS47Workshop::AddPart(const FString& Kind,int32 Donor,bool bTested,bool bFaulty)
{
 FPart P;P.Id=NextId++;P.Donor=Donor;P.Kind=Kind;P.bTested=bTested;P.bFaulty=bFaulty;Tray.Add(P);Seen.AddUnique(Kind);
}
bool FS47Workshop::Unpack()
{
 if(!CanUnpack())return false;
 ++Donors;const FString Fault=FaultOf(Donors);
 for(const TCHAR* K:DonorParts)AddPart(K,Donors,false,Fault==K);
 return true;
}
bool FS47Workshop::Test(int32 Id)
{
 const int32 I=Tray.IndexOfByPredicate([Id](const FPart& P){return P.Id==Id;});
 if(I<0||Tray[I].bTested)return false;
 Tray[I].bTested=true;return true;
}
bool FS47Workshop::Scrap(int32 Id){return Tray.RemoveAll([Id](const FPart& P){return P.Id==Id;})>0;}
bool FS47Workshop::Known(const FRecipe& R) const
{
 if(IsBuilt(R.Id))return true;
 for(const TCHAR* K:R.Parts)if(Seen.Contains(K))return true;
 return false;
}
FString FS47Workshop::Missing(const FRecipe& R,int32 Cells,int32 Foil) const
{
 TArray<FString> Out;TArray<int32> Used;
 for(const TCHAR* K:R.Parts)
 {
  const FPart* Good=Tray.FindByPredicate([&](const FPart& P){return P.Kind==K&&P.Usable()&&!Used.Contains(P.Id);});
  if(Good){Used.Add(Good->Id);continue;}
  const bool Untested=Tray.ContainsByPredicate([&](const FPart& P){return P.Kind==K&&!P.bTested&&!Used.Contains(P.Id);});
  Out.Add(PartTitle(K)+(Untested?TEXT(" (test it first)"):TEXT(" (tested good)")));
 }
 if(Cells<R.Cells)Out.Add(FString::Printf(TEXT("%d dry cell%s, have %d"),R.Cells,R.Cells==1?TEXT(""):TEXT("s"),Cells));
 if(Foil<R.Foil)Out.Add(TEXT("shielding foil"));
 if(IsBuilt(R.Id))Out.Add(TEXT("already built"));
 return FString::Join(Out,TEXT(", "));
}
bool FS47Workshop::Build(const FString& RecipeId,int32 Cells,int32 Foil)
{
 const FRecipe* R=Recipe(RecipeId);if(!R||!Missing(*R,Cells,Foil).IsEmpty())return false;
 for(const TCHAR* K:R->Parts)
 {
  const int32 I=Tray.IndexOfByPredicate([&](const FPart& P){return P.Kind==K&&P.Usable();});
  if(I<0)return false; // unreachable after Missing; never half-consume
  Tray.RemoveAt(I);
 }
 if(!R->Stock)Built.Add(RecipeId);return true;
}
bool FS47Workshop::CanStrip(const FString& RecipeId) const
{
 const FRecipe* R=Recipe(RecipeId);return R&&IsBuilt(RecipeId)&&Tray.Num()+R->Parts.Num()<=TraySize;
}
bool FS47Workshop::Strip(const FString& RecipeId)
{
 if(!CanStrip(RecipeId))return false;
 for(const TCHAR* K:Recipe(RecipeId)->Parts)AddPart(K,0,true,false); // parts come back; cells and foil are spent
 Built.Remove(RecipeId);return true;
}
S47J::FObj FS47Workshop::Save() const
{
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("donors"),Donors);S47J::Set(O,TEXT("nextId"),NextId);
 S47J::Set(O,TEXT("built"),Built);S47J::Set(O,TEXT("seen"),Seen);
 TArray<TSharedPtr<FJsonValue>> A;
 for(const auto& P:Tray){auto V=S47J::New();S47J::Set(V,TEXT("id"),P.Id);S47J::Set(V,TEXT("donor"),P.Donor);S47J::Set(V,TEXT("kind"),P.Kind);S47J::Set(V,TEXT("tested"),P.bTested);S47J::Set(V,TEXT("faulty"),P.bFaulty);A.Add(MakeShared<FJsonValueObject>(V));}
 O->SetArrayField(TEXT("tray"),A);return O;
}
void FS47Workshop::Load(const S47J::FObj& O)
{
 *this=FS47Workshop();if(!O||S47J::Int(O,TEXT("v"))!=1)return;
 Donors=FMath::Clamp(S47J::Int(O,TEXT("donors")),0,1000000);int32 MaxId=0;
 for(const auto& V:S47J::Arr(O,TEXT("tray")))
 {
  const S47J::FObj* Q=nullptr;if(!V.IsValid()||!V->TryGetObject(Q)||!Q||Tray.Num()>=TraySize)continue;
  FPart P;P.Id=S47J::Int(*Q,TEXT("id"));P.Donor=FMath::Clamp(S47J::Int(*Q,TEXT("donor")),0,1000000);P.Kind=S47J::Str(*Q,TEXT("kind"));P.bTested=S47J::Bool(*Q,TEXT("tested"),false);P.bFaulty=S47J::Bool(*Q,TEXT("faulty"),false);
  if(P.Id<=0||!ValidKind(P.Kind)||Part(P.Id))continue;
  MaxId=FMath::Max(MaxId,P.Id);Tray.Add(P);
 }
 NextId=FMath::Clamp(S47J::Int(O,TEXT("nextId")),MaxId+1,2000000000);
 for(const auto& B:S47J::Strs(O,TEXT("built")))if(Recipe(B))Built.AddUnique(B);
 for(const auto& K:S47J::Strs(O,TEXT("seen")))if(ValidKind(K))Seen.AddUnique(K);
}
bool FS47Workshop::SelfTest(TArray<FString>& Lines)
{
 bool Good=true;auto C=[&](bool Ok,const TCHAR* Text){Lines.Add(FString(Ok?TEXT("PASS "):TEXT("FAIL "))+Text);Good&=Ok;};
 FS47Workshop W;
 C(!W.Known(*Recipe(TEXT("own_logger"))),TEXT("workshop recipes stay hidden until a usable part kind has been handled"));
 C(W.Unpack()&&W.Tray.Num()==4&&W.Known(*Recipe(TEXT("own_logger"))),TEXT("workshop donor gives four identified parts and reveals its recipes"));
 C(W.Missing(*Recipe(TEXT("own_logger")),1,0).Contains(TEXT("test it first")),TEXT("workshop untested parts are refused with an exact reason"));
 const auto Id=[&](const TCHAR* K){const auto* P=W.Tray.FindByPredicate([&](const FPart& X){return X.Kind==K;});return P?P->Id:0;};
 for(const auto& P:TArray<FPart>(W.Tray))W.Test(P.Id);
 C(!W.Test(Id(TEXT("timer"))),TEXT("workshop a part is tested once"));
 C(W.Part(Id(TEXT("timer")))->bFaulty&&!W.Part(Id(TEXT("timer")))->Usable(),TEXT("workshop first donor's fault is revealed by testing"));
 C(!W.Build(TEXT("own_logger"),1,0)&&W.Tray.Num()==4&&W.Built.IsEmpty(),TEXT("workshop refused build consumes nothing"));
 C(W.Missing(*Recipe(TEXT("own_logger")),0,0).Contains(TEXT("dry cell")),TEXT("workshop missing cells are named"));
 C(W.Scrap(Id(TEXT("timer")))&&W.Tray.Num()==3&&!W.Scrap(9999),TEXT("workshop scrapping removes only that part"));
 C(W.CanUnpack()==false||W.Tray.Num()+4<=8,TEXT("workshop tray capacity is respected"));
 C(W.Unpack()&&W.Tray.Num()==7,TEXT("workshop second donor fits after scrapping"));
 FS47Workshop Full=W;C(!Full.CanUnpack()&&!Full.Unpack()&&Full.Donors==2,TEXT("workshop full tray refuses a donor without spending it"));
 for(const auto& P:TArray<FPart>(W.Tray))if(!P.bTested)W.Test(P.Id);
 FS47Workshop R;R.Load(S47J::Read(S47J::Write(W.Save())));
 C(R.Tray.Num()==W.Tray.Num()&&R.Donors==2&&R.NextId==W.NextId&&R.Part(W.Tray[0].Id)&&R.Part(W.Tray[0].Id)->Kind==W.Tray[0].Kind&&R.Seen.Num()==W.Seen.Num(),TEXT("workshop parts and faults survive a save round trip"));
 C(W.Build(TEXT("own_logger"),1,0)&&W.IsBuilt(TEXT("own_logger"))&&W.Tray.Num()==4,TEXT("workshop build removes exactly its three parts"));
 C(!W.Build(TEXT("own_logger"),1,0)&&W.Tray.Num()==4,TEXT("workshop cannot build the same instrument twice"));
 {FS47Workshop S;S.Unpack();S.Unpack();for(const auto& P:TArray<FPart>(S.Tray))S.Test(P.Id);C(!S.Missing(*Recipe(TEXT("cable_splice")),0,1).IsEmpty()==false&&S.Build(TEXT("cable_splice"),0,1)&&!S.IsBuilt(TEXT("cable_splice"))&&S.Tray.Num()==6,TEXT("workshop a splice takes both leads and stays buildable as a stock item"));}
 const int32 Before=W.Tray.Num();
 C(W.Strip(TEXT("own_logger"))&&!W.IsBuilt(TEXT("own_logger"))&&W.Tray.Num()==Before+3&&W.Tray.Last().Usable()&&!W.Strip(TEXT("own_logger")),TEXT("workshop strip returns parts once, tested good, and cannot duplicate"));
 auto Bad=W.Save();S47J::Set(Bad,TEXT("built"),TArray<FString>{TEXT("nonsense")});FS47Workshop X;X.Load(Bad);C(X.Built.IsEmpty(),TEXT("workshop unknown saved recipes are dropped"));
 FS47Workshop Empty;Empty.Load(nullptr);C(Empty.Tray.IsEmpty()&&Empty.NextId==1,TEXT("workshop older saves start with an empty bench"));
 return Good;
}

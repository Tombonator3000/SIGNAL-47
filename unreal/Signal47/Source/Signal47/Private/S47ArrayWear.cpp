#include "S47ArrayWear.h"

const TArray<FS47ArrayWear::FBranch>& FS47ArrayWear::Branches()
{
 static const TArray<FBranch> B={
  {TEXT("west"),TEXT("WEST BRANCH"),TEXT("S-13"),TEXT("Blown control fuses in the branch junction."),{TEXT("S-13"),TEXT("S-15"),TEXT("S-21")},{{TEXT("fuse"),2}}},
  {TEXT("east"),TEXT("EAST BRANCH"),TEXT("S-16"),TEXT("Burned-out command relays in the branch junction."),{TEXT("S-16"),TEXT("S-20"),TEXT("S-23"),TEXT("S-26")},{{TEXT("relay"),2}}},
  {TEXT("centre"),TEXT("CENTER BRANCH"),TEXT("S-14"),TEXT("Water in the feed cable. It needs a made-up splice from the workshop."),{TEXT("S-14"),TEXT("S-18"),TEXT("S-19"),TEXT("S-22")},{{TEXT("splice_kit"),1}}},
  {TEXT("far"),TEXT("FAR BRANCH"),TEXT("S-17"),TEXT("Seized drive motor and its relay on the long run out."),{TEXT("S-17"),TEXT("S-24"),TEXT("S-25"),TEXT("S-27")},{{TEXT("drive_motor"),1},{TEXT("relay"),1}}}};
 return B;
}
const FS47ArrayWear::FBranch* FS47ArrayWear::Branch(const FString& Id){return Branches().FindByPredicate([&](const FBranch& B){return Id==B.Id;});}
double FS47ArrayWear::RateForCount(int32 Count)
{
 const int32 N=FMath::Clamp(Count,0,27);
 if(N<MinDishes)return 0;
 if(N>=24)return double(N*(N-1))/(27.*26.);
 // Pairs alone would make early observing unbearably slow at 12 antennas (19%).
 return FMath::Lerp(.25,552./702.,double(N-MinDishes)/double(24-MinDishes));
}
const FS47ArrayWear::FBranch* FS47ArrayWear::BranchOfDish(const FString& Dish) const
{
 if(!bActive)return nullptr;
 for(const auto& B:Branches())if(!IsRestored(B.Id))for(const TCHAR* D:B.Dishes)if(Dish==D)return &B;
 return nullptr;
}
const FS47ArrayWear::FBranch* FS47ArrayWear::DownJunction(const FString& Dish) const
{
 const FBranch* B=BranchOfDish(Dish);return B&&Dish==B->Junction?B:nullptr;
}
const FS47ArrayWear::FBranch* FS47ArrayWear::NextBranch() const
{
 if(!bActive)return nullptr;
 for(const auto& B:Branches())if(!IsRestored(B.Id))return &B;
 return nullptr;
}
int32 FS47ArrayWear::DownCount() const
{
 if(!bActive)return 0;
 int32 N=0;for(const auto& B:Branches())if(!IsRestored(B.Id))N+=B.Dishes.Num();
 return N;
}
bool FS47ArrayWear::Restore(const FString& Id)
{
 if(!bActive||!Branch(Id)||IsRestored(Id))return false;
 Restored.Add(Id);return true;
}
S47J::FObj FS47ArrayWear::Save() const
{
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("active"),bActive);S47J::Set(O,TEXT("restored"),Restored);S47J::Set(O,TEXT("job"),Job);S47J::Set(O,TEXT("jobSeconds"),JobSeconds);return O;
}
void FS47ArrayWear::Load(const S47J::FObj& O)
{
 *this=FS47ArrayWear();if(!O||S47J::Int(O,TEXT("v"))!=1)return;
 bActive=S47J::Bool(O,TEXT("active"),false);
 for(const auto& R:S47J::Strs(O,TEXT("restored")))if(Branch(R))Restored.AddUnique(R);
 const FString J=S47J::Str(O,TEXT("job"));if(Branch(J)&&!IsRestored(J)&&bActive){Job=J;const double S=S47J::Num(O,TEXT("jobSeconds"));JobSeconds=FMath::IsFinite(S)?FMath::Clamp(S,0.,3.):0;}
}
bool FS47ArrayWear::SelfTest(TArray<FString>& Lines)
{
 bool Good=true;auto C=[&](bool Ok,const TCHAR* Text){Lines.Add(FString(Ok?TEXT("PASS "):TEXT("FAIL "))+Text);Good&=Ok;};
 FS47ArrayWear W;C(W.DownCount()==0&&!W.BranchOfDish(TEXT("S-13")),TEXT("array wear: an inactive state leaves every antenna working"));
 TSet<FString> Seen;int32 Total=0;for(const auto& B:Branches())for(const TCHAR* D:B.Dishes){Seen.Add(D);++Total;}
 C(Total==15&&Seen.Num()==15&&!Seen.Contains(TEXT("S-03")),TEXT("array wear: fifteen distinct outer antennas, never the tutorial's S-03"));
 W.Start();C(W.DownCount()==15&&27-W.DownCount()==12,TEXT("array wear: a new station starts on twelve of twenty-seven"));
 C(W.DownJunction(TEXT("S-13"))&&!W.DownJunction(TEXT("S-15"))&&W.BranchOfDish(TEXT("S-15"))&&!W.BranchOfDish(TEXT("S-03")),TEXT("array wear: only the junction antenna opens its branch"));
 C(W.Restore(TEXT("west"))&&W.DownCount()==12&&!W.Restore(TEXT("west"))&&!W.Restore(TEXT("nowhere")),TEXT("array wear: a branch is restored once and only known branches count"));
 W.Job=TEXT("east");W.JobSeconds=1.5;FS47ArrayWear R;R.Load(S47J::Read(S47J::Write(W.Save())));
 C(R.bActive&&R.IsRestored(TEXT("west"))&&R.DownCount()==12&&R.Job==TEXT("east")&&R.JobSeconds==1.5,TEXT("array wear: progress and a running repair survive save"));
 FS47ArrayWear Old;Old.Load(nullptr);C(!Old.bActive&&Old.DownCount()==0,TEXT("array wear: older saves keep their fully working array"));
 C(RateForCount(5)==0&&FMath::IsNearlyEqual(RateForCount(6),.25)&&FMath::IsNearlyEqual(RateForCount(24),552./702.,1e-9)&&RateForCount(27)==1.,TEXT("array wear: measurement floor at six, joining the pair rate at twenty-four"));
 bool Rising=true;for(int32 N=6;N<27;++N)Rising&=RateForCount(N+1)>RateForCount(N);C(Rising,TEXT("array wear: every repaired antenna makes measuring faster"));
 return Good;
}

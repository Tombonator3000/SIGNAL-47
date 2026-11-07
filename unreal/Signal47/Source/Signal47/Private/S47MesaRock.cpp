#include "S47MesaRock.h"
#include "S47Web.h"
#include "S47FarmWorld.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47MesaRock,Log,All);

namespace
{
 TWeakObjectPtr<UMaterialInstanceDynamic> GroundMid,SeamMid;
 // Measured against photographs of this desert (7 November): the bare ground read 230 where the photographs read
 // 150, so in Freeform it comes down to real soil and shows more of its gravel. Story keeps the look it was approved in.
 // Each frame, since it must be right the moment a game opens; it only writes when the answer changes.
 void ApplyGround(AS47Web* Web)
 {
  auto* Mid=GroundMid.Get();if(!Mid)return;
  static TWeakObjectPtr<UMaterialInstanceDynamic> Read;static FLinearColor StoryTint;static float StoryGravel,StoryCrust,LastDay=-1;static bool bLastReal=false;
  if(Read.Get()!=Mid){Read=Mid;Mid->GetVectorParameterValue(TEXT("GroundTint"),StoryTint);Mid->GetScalarParameterValue(TEXT("GravelAmount"),StoryGravel);Mid->GetScalarParameterValue(TEXT("CrustAmount"),StoryCrust);LastDay=-1;}
  const US47Main* Main=US47Main::Get(Web);const bool bReal=S47FarmWorld::Enabled(Main);
  // Only the day changes: a darker soil would take the moonlit ground, already at the edge of sight, down to black.
  const double Hour=Main&&Main->Freeform?FMath::Fmod(Main->Freeform->Clock(),86400.)/3600.:12.;
  const float Day=float(FMath::Clamp(FMath::Min((Hour-5.75)/1.25,(19.25-Hour)/1.25),0.,1.));
  if(bReal==bLastReal&&FMath::Abs(Day-LastDay)<.002f)return;
  bLastReal=bReal;LastDay=Day;
  const auto Tint=[&](const FLinearColor& Real){return bReal?FMath::Lerp(StoryTint,Real,Day):StoryTint;};
  Mid->SetVectorParameterValue(TEXT("GroundTint"),Tint(FLinearColor(.24f,.265f,.275f)));
  Mid->SetScalarParameterValue(TEXT("GravelAmount"),bReal?.85f:StoryGravel);
  Mid->SetScalarParameterValue(TEXT("CrustAmount"),bReal?.75f:StoryCrust);
  if(auto* Seam=SeamMid.Get())
  {
   Seam->SetVectorParameterValue(TEXT("GroundTint"),Tint(FLinearColor(.24f*.525f,.265f*.35f,.275f*.28f)));
   Seam->SetScalarParameterValue(TEXT("GravelAmount"),bReal?.85f:StoryGravel);
   Seam->SetScalarParameterValue(TEXT("CrustAmount"),bReal?.75f:StoryCrust);
  }
 }
}

void S47MesaRock::Step(AS47Web* Web,double Dt)
{
 if(Web)ApplyGround(Web);
 static double Wait=0;static TWeakObjectPtr<UMaterialInterface> Rock;static bool bMissing=false;
 static TSet<TWeakObjectPtr<UStaticMeshComponent>> Done;
 if(!Web||bMissing||(Wait-=Dt)>0)return;
 Wait=2.0;
 if(!Rock.IsValid())
 {
  Rock=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Licensed/Backdrop/M_S47_MesaRock.M_S47_MesaRock"));
  if(!Rock.IsValid()){bMissing=true;UE_LOG(LogS47MesaRock,Display,TEXT("S47: mesa rock material not installed; exported mesa colours kept"));return;}
 }
 static TWeakObjectPtr<UMaterialInterface> Ground;static bool bNoGround=false;
 if(!Ground.IsValid()&&!bNoGround){Ground=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Backdrop/M_S47_DesertGround.M_S47_DesertGround"));bNoGround=!Ground.IsValid();}
 for(int32 I=0;const FS47WebNode* N=Web->Node(I);++I)
 {
  // Desert ground (export materials using art/desert: SARO yard 5, saro corridor 416, road 574, old road 774).
  if(Ground.IsValid()&&N->Mats.Num()==1&&(N->Mats[0]==5||N->Mats[0]==416||N->Mats[0]==574||N->Mats[0]==774))
  {
   UStaticMeshComponent* G=Cast<UStaticMeshComponent>(N->Comp.Get());
   if(G&&!Done.Contains(G)){
    // The corridor's seam mesh has no vertex colours, so the same tint lands much brighter on it: its own instance.
    const bool bSeam=N->Name==TEXT("saroCorridorGround");
    TWeakObjectPtr<UMaterialInstanceDynamic>& Mine=bSeam?SeamMid:GroundMid;
    if(!Mine.IsValid())Mine=UMaterialInstanceDynamic::Create(Ground.Get(),GetTransientPackage());
    G->SetMaterial(0,Mine.IsValid()?(UMaterialInterface*)Mine.Get():Ground.Get());Done.Add(G);UE_LOG(LogS47MesaRock,Display,TEXT("S47: desert ground material on node %d (%s)"),I,*N->Name);}
   continue;
  }
  const bool Legacy=N->Name.StartsWith(TEXT("mesas")),Backdrop=N->Name==TEXT("prop:streetGroundMesas");
  if(!Legacy&&!Backdrop)continue;
  UStaticMeshComponent* C=Cast<UStaticMeshComponent>(N->Comp.Get());
  if(!C||Done.Contains(C))continue;
  // Caliche CN5 backdrop slots (Tools/calichestreet/backdrop): talus, sandstone beds, pale cap,
  // recessive beds, eroded talus, distant ridges. New Mexico palette over the photo sandstone.
  static const FLinearColor Palette[6]={{1.05f,.98f,.90f},{1.18f,.88f,.72f},{1.22f,1.14f,1.f},{1.f,.76f,.62f},{1.1f,.94f,.84f},{.86f,.88f,.96f}};
  for(int32 S=0;S<C->GetNumMaterials();++S)
  {
   UMaterialInstanceDynamic* D=UMaterialInstanceDynamic::Create(Rock.Get(),C);
   D->SetScalarParameterValue(TEXT("UseVertexColor"),Legacy?1.f:0.f);
   D->SetVectorParameterValue(TEXT("Tint"),Backdrop?Palette[S%6]:FLinearColor::White);
   C->SetMaterial(S,D);
  }
  Done.Add(C);
  UE_LOG(LogS47MesaRock,Display,TEXT("S47: mesa rock applied to %s (node %d, %d slots)"),*N->Name,I,C->GetNumMaterials());
 }
}

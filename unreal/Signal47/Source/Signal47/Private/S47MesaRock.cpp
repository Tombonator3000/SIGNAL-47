#include "S47MesaRock.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47MesaRock,Log,All);

void S47MesaRock::Step(AS47Web* Web,double Dt)
{
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
   if(G&&!Done.Contains(G)){G->SetMaterial(0,Ground.Get());Done.Add(G);UE_LOG(LogS47MesaRock,Display,TEXT("S47: desert ground material on node %d (%s)"),I,*N->Name);}
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

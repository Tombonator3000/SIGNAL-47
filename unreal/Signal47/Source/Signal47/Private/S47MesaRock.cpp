#include "S47MesaRock.h"
#include "S47FreeformTerrain.h"
#include "S47Web.h"
#include "S47FarmWorld.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ContentStreaming.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47MesaRock,Log,All);

namespace
{
 TWeakObjectPtr<UMaterialInstanceDynamic> GroundMid,SeamMid;
 struct FMesaSlot{TWeakObjectPtr<UMaterialInstanceDynamic> Mid;FLinearColor Story=FLinearColor::White;};
 TArray<FMesaSlot> MesaSlots;bool bMesaFresh=false;
 // The low mips of the ground's sets read far paler than the full texture, so a game opened to a white yard for seconds. All mips stay resident for
 // good; the fast response lets them past the streaming pool's mip bias, and a new set of ground waits up to a second for them (once, at the start).
 void HoldResident(UMaterialInterface* M,bool bWait)
 {
  if(!M)return;
  M->SetForceMipLevelsToBeResident(true,true,-1.f,0,true);
  if(bWait)IStreamingManager::Get().StreamAllResources(1.f);
 }
 // 0 at night, 1 in full day, with an hour and a quarter of dawn and dusk; 1 outside Freeform's clock.
 // Dawn mirrors dusk about the sun, sunrise 05:30 to sunset 18:27. It ran to 07:00, an hour behind the sun: at 06:00 with the day's light on it the ground
 // still had four fifths of the night's far paler tint.
 float DayAt(const US47Main* Main)
 {
  const double Hour=Main&&Main->Freeform?FMath::Fmod(Main->Freeform->Clock(),86400.)/3600.:12.;
  return float(FMath::Clamp(FMath::Min((Hour-4.72)/1.25,(19.25-Hour)/1.25),0.,1.));
 }
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
  const float Day=DayAt(Main);
  // The backdrop's palette read as Monument Valley beside photographs of this country: drier and further away in Freeform,
  // with its beds much closer together, since their contrast drew the steps and the dark squares.
  if(bMesaFresh||bReal!=bLastReal)
  {
   bMesaFresh=false;MesaSlots.RemoveAll([](const FMesaSlot& S){return !S.Mid.IsValid();});
   for(const FMesaSlot& S:MesaSlots)S.Mid->SetVectorParameterValue(TEXT("Tint"),bReal?FMath::Lerp(S.Story,FLinearColor(1.07f,.92f,.84f),.7f)*FLinearColor(.66f,.76f,.90f):S.Story);
  }
  if(bReal==bLastReal&&FMath::Abs(Day-LastDay)<.002f)return;
  bLastReal=bReal;LastDay=Day;
  const auto Tint=[&](const FLinearColor& Real){return bReal?FMath::Lerp(StoryTint,Real,Day):StoryTint;};
  // (Where the relief is on this is only the old road's ground. .175 .225 .265 read 170,134,110; now toward the one soil, see SoilDay.)
  Mid->SetVectorParameterValue(TEXT("GroundTint"),Tint(FLinearColor(.164f,.211f,.240f)));
  Mid->SetScalarParameterValue(TEXT("GravelAmount"),bReal?.85f:StoryGravel);
  Mid->SetScalarParameterValue(TEXT("CrustAmount"),bReal?.75f:StoryCrust);
  if(auto* Seam=SeamMid.Get())
  {
   // Beside the ring in one picture at 11:18, the old .525 .35 .28 read 134,89,71 to its 176,141,118: linear 1.83 2.66 2.87 too dark.
   // .168 .209 .212 read 168,130,103 in relief-11-farm (7 November): this is that against the one soil, 165,130,105.
   Seam->SetVectorParameterValue(TEXT("GroundTint"),Tint(FLinearColor(.161f,.209f,.220f)));
   Seam->SetScalarParameterValue(TEXT("GravelAmount"),bReal?.85f:StoryGravel);
   Seam->SetScalarParameterValue(TEXT("CrustAmount"),bReal?.75f:StoryCrust);
  }
 }
 // Freeform's relief meshes have their own material (S47FreeformTerrain) with the same parameters: the same soil by day and
 // the same gravel and crust. By night it keeps the tint it was authored with, as the old ground keeps Story's.
 void ApplyFreeformGround(AS47Web* Web)
 {
  auto* Mid=S47FreeformTerrain::GroundMid(Web);if(!Mid)return;
  static TWeakObjectPtr<UMaterialInstanceDynamic> Read;static FLinearColor NightTint,NightCrust(.94f,.94f,.94f,1.f),NightGravel(.94f,.94f,.94f,1.f);static float LastDay=-1;
  if(Read.Get()!=Mid)
  {
   Read=Mid;Mid->GetVectorParameterValue(TEXT("GroundTint"),NightTint);LastDay=-1;
   Mid->GetVectorParameterValue(TEXT("CrustTint"),NightCrust);Mid->GetVectorParameterValue(TEXT("GravelTint"),NightGravel);
   HoldResident(Mid,true);
   Mid->SetScalarParameterValue(TEXT("GravelAmount"),.85f);Mid->SetScalarParameterValue(TEXT("CrustAmount"),.75f);
  }
  // The range's foot is this same soil, so it takes the same tint and amounts the moment it appears.
  auto* Range=S47FreeformTerrain::RangeMid(Web);
  static TWeakObjectPtr<UMaterialInstanceDynamic> RangeRead;const bool bRangeNew=Range&&RangeRead.Get()!=Range;
  if(bRangeNew){RangeRead=Range;Range->SetScalarParameterValue(TEXT("GravelAmount"),.85f);Range->SetScalarParameterValue(TEXT("CrustAmount"),.75f);}
  // The SARO corridor's flat ground, the same soil with no mesh data of its own (S47FreeformTerrain).
  auto* Corridor=S47FreeformTerrain::CorridorMid(Web);
  static TWeakObjectPtr<UMaterialInstanceDynamic> CorridorRead;const bool bCorridorNew=Corridor&&CorridorRead.Get()!=Corridor;
  if(bCorridorNew){CorridorRead=Corridor;Corridor->SetScalarParameterValue(TEXT("GravelAmount"),.85f);Corridor->SetScalarParameterValue(TEXT("CrustAmount"),.75f);}
  const float Day=DayAt(US47Main::Get(Web));if(!bRangeNew&&!bCorridorNew&&FMath::Abs(Day-LastDay)<.002f)return;LastDay=Day;
  // SoilDay: one bare soil in the sun over the whole of Freeform's land, sRGB 165,130,105 (Tom, 7 November), measured in captures.
  // One soil, not three (Tom, 7 November: "ett jordfarge"). The crust layer is a pale grey cream, linear mean .726 .581 .431 against the sand's
  // .617 .387 .272 and the gravel's .581 .425 .306 (T_Ground*_D, 2048 maps measured). Where the 1100 m noise field turns the crust on (the town,
  // 1.6 to 2 km down the road, relief-11-town) the land read 175,164,147 against 165,130,105, and where it is mostly sand (the farm's yard, 11:18)
  // it read 193,149,120. Every layer's mean is brought to the sand's, and the sand's to the mixture mean the tint was reckoned for (.482 .304 .208,
  // import_ground_relief.py DEFAULT_TINT): CrustTint = sand / crust = .850 .666 .631, GravelTint = sand / gravel = 1.062 .911 .889, and the sand,
  // which has no tint of its own, is .781 .785 .765 (mix / sand) through GroundTint. By the day only, so night keeps what it had. Patterns, cracks,
  // stones and gravel stay; only their mean colour is the same. Expect the town at about 151,125,108 and the yard at about 175,131,105 by the
  // measured means: these are the numbers to retune if a capture still shows a layer.
  // 8 November: the town still read pale cream, 194,183,165 looking down by the church at 11:00 (w2 g-church-11), against 169,136,110 on
  // SARO's sand. Its crust is warmed and darkened by most of that ratio (sRGB .87 .74 .66, linear about .85 .72 .62), not all of it:
  // caliche is a pale soil, and the crust's patches out in the desert should stay a little lighter than the sand round them.
  const FLinearColor SandToMix(.781f,.785f,.765f,1.f),CrustDay(.723f,.480f,.391f,1.f),GravelDay(1.062f,.911f,.889f,1.f);
  const FLinearColor Tint=FMath::Lerp(NightTint,FLinearColor(.24f,.285f,.30f),Day)*FMath::Lerp(FLinearColor::White,SandToMix,Day);
  const FLinearColor Crust=FMath::Lerp(NightCrust,CrustDay,Day),Gravel=FMath::Lerp(NightGravel,GravelDay,Day);
  for(UMaterialInstanceDynamic* M:{Mid,Range,Corridor})
  {
   if(!M)continue;
   M->SetVectorParameterValue(TEXT("GroundTint"),Tint);M->SetVectorParameterValue(TEXT("CrustTint"),Crust);M->SetVectorParameterValue(TEXT("GravelTint"),Gravel);
  }
 }
 // The other ground the player stands on keeps its own exported MI_W* material (the station's sand, its gravel, the gravel
 // tracks and the farm road). Measured at 11:00 in Freeform against the soil of the time, 170,134,110: the station's sand
 // 215,172,130, the farm road 221,188,156, the antenna path 223,199,175, the roadside gravel 208,199,189. Each one's own
 // "Color" is scaled toward the soil by day; at night and in Story the scale is 1, so those keep the look they had.
 // Scales are linear, from the sRGB ratios against the soil (.79 .78 .85 for the sand, 0.80 for the farm road in red), then
 // .936 .937 .904 for 170,134,110 to SoilDay's 165,130,105. They are the only numbers to retune if a new capture still shows a difference.
 struct FUnder{int32 Node,Mat;FLinearColor Scale;};
 // 8 November: the station's sand still read 202,152,111 (d-st-a-11) against 165,130,105, orange beside the soil round it: its scale
 // times the linear ratio (.641 .708 .885). Its gravel pad keeps the old scale, so it stays a little apart from the sand, not a stain.
 const FLinearColor SandScale(.360f,.384f,.560f),StationGravelScale(.562f,.543f,.633f),RoadScale(.655f,.581f,.524f),GravelScale(.805f,.768f,.705f);
 const FUnder UnderTable[]={
  {1103,484,SandScale},{1120,496,SandScale},{1104,485,StationGravelScale},      // station ground, its corridor, the station's gravel
  {959,418,RoadScale},{1250,576,RoadScale},                              // the farm road in the SARO corridor and in the road area
  {955,418,GravelScale},{1244,576,GravelScale},{1121,497,GravelScale},   // gravel tracks
  {801,353,GravelScale}};                                                // the pebble paths among the antennas
 struct FUnderSlot{TWeakObjectPtr<UMaterialInstanceDynamic> Mid;FLinearColor Story=FLinearColor::White,Scale=FLinearColor::White;};
 TArray<FUnderSlot> UnderSlots;TSet<TWeakObjectPtr<UStaticMeshComponent>> UnderDone;bool bUnderFresh=false;
 void TickUnderfoot(AS47Web* Web,double Dt)
 {
  static double Wait=0;static float LastDay=-1;static bool bLastReal=false;
  if((Wait-=Dt)<=0)
  {
   Wait=2.0;
   for(const FUnder& U:UnderTable)
   {
    const FS47WebNode* N=Web->Node(U.Node);if(!N||N->Mats.Num()!=1||N->Mats[0]!=U.Mat)continue;
    UStaticMeshComponent* G=Cast<UStaticMeshComponent>(N->Comp.Get());if(!G||G->GetNumMaterials()<1||UnderDone.Contains(G))continue;
    // An existing dynamic instance is reused, so whoever made it keeps it.
    UMaterialInstanceDynamic* Mid=G->CreateDynamicMaterialInstance(0,nullptr);FLinearColor Story;
    UnderDone.Add(G);if(!Mid||!Mid->GetVectorParameterValue(TEXT("Color"),Story))continue;
    UnderSlots.Add({Mid,Story,U.Scale});bUnderFresh=true;
    UE_LOG(LogS47MesaRock,Display,TEXT("S47: underfoot tone on node %d (%s, material %d)"),U.Node,*N->Name,U.Mat);
   }
  }
  if(UnderSlots.IsEmpty())return;
  const US47Main* Main=US47Main::Get(Web);const bool bReal=S47FarmWorld::Enabled(Main);const float Day=bReal?DayAt(Main):0.f;
  if(!bUnderFresh&&bReal==bLastReal&&FMath::Abs(Day-LastDay)<.002f)return;
  bUnderFresh=false;bLastReal=bReal;LastDay=Day;
  UnderSlots.RemoveAll([](const FUnderSlot& S){return !S.Mid.IsValid();});
  for(const FUnderSlot& S:UnderSlots)S.Mid->SetVectorParameterValue(TEXT("Color"),FMath::Lerp(S.Story,S.Story*S.Scale,Day));
 }
}

void S47MesaRock::Step(AS47Web* Web,double Dt)
{
 if(Web){ApplyGround(Web);ApplyFreeformGround(Web);TickUnderfoot(Web,Dt);}
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
   // (not while the relief holds it: S47FreeformTerrain gives it back to this as it was, and this picks it up then)
   if(G&&!Done.Contains(G)&&!S47FreeformTerrain::HoldsMaterial(Web,G)){
    // The corridor's seam mesh has no vertex colours, so the same tint lands much brighter on it: its own instance.
    const bool bSeam=N->Name==TEXT("saroCorridorGround");
    TWeakObjectPtr<UMaterialInstanceDynamic>& Mine=bSeam?SeamMid:GroundMid;
    if(!Mine.IsValid())
    {
     Mine=UMaterialInstanceDynamic::Create(Ground.Get(),GetTransientPackage());
     // The low mips of these sets read far paler than the full texture: a game opened to a white yard for seconds.
     if(Mine.IsValid())HoldResident(Mine.Get(),false);
    }
    G->SetMaterial(0,Mine.IsValid()?(UMaterialInterface*)Mine.Get():Ground.Get());Done.Add(G);UE_LOG(LogS47MesaRock,Display,TEXT("S47: desert ground material on node %d (%s)"),I,*N->Name);}
   continue;
  }
  const bool Legacy=N->Name.StartsWith(TEXT("mesas")),Backdrop=N->Name==TEXT("prop:streetGroundMesas");
  if(!Legacy&&!Backdrop)continue;
  UStaticMeshComponent* C=Cast<UStaticMeshComponent>(N->Comp.Get());
  if(!C||Done.Contains(C))continue;
  // (not while Freeform's range stands on it: S47FreeformTerrain gives it back to this as it was, and this picks it up then)
  if(S47FreeformTerrain::HoldsMaterial(Web,C))continue;
  // Caliche CN5 backdrop slots (Tools/calichestreet/backdrop): talus, sandstone beds, pale cap,
  // recessive beds, eroded talus, distant ridges. New Mexico palette over the photo sandstone.
  static const FLinearColor Palette[6]={{1.05f,.98f,.90f},{1.18f,.88f,.72f},{1.22f,1.14f,1.f},{1.f,.76f,.62f},{1.1f,.94f,.84f},{.86f,.88f,.96f}};
  for(int32 S=0;S<C->GetNumMaterials();++S)
  {
   UMaterialInstanceDynamic* D=UMaterialInstanceDynamic::Create(Rock.Get(),C);
   D->SetScalarParameterValue(TEXT("UseVertexColor"),Legacy?1.f:0.f);
   D->SetVectorParameterValue(TEXT("Tint"),Backdrop?Palette[S%6]:FLinearColor::White);
   if(Backdrop){MesaSlots.Add({D,Palette[S%6]});bMesaFresh=true;}
   C->SetMaterial(S,D);
  }
  Done.Add(C);
  UE_LOG(LogS47MesaRock,Display,TEXT("S47: mesa rock applied to %s (node %d, %d slots)"),*N->Name,I,C->GetNumMaterials());
 }
}

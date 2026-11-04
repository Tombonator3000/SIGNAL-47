#include "S47EastWingProps.h"
#include "S47Main.h"
#include "S47Web.h"
#include "S47Three.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47EastWingProps,Log,All);

namespace
{
 // All authored models stay at unit scale. GreenBench's worktop is .915+.035/2
 // above its ground pivot; .002-bottom glassware sits flush at .9305.
 // Imported Blender -Y fronts face site +Z; UE yaw180 faces the south wall's inward side.
 struct FPlacement
 {
  const TCHAR* Asset; const TCHAR* Group; FVector At; double Yaw;
  bool bFoot=false; bool bFixture=false; bool bPrivate=false;
 };
 const FPlacement Layout[]={
  {TEXT("SM_EW_GreenBench"),TEXT("Workshop"),{24.2,0,4.72},180,true},
  {TEXT("SM_EW_LabFridge"),TEXT("Lab"),{21.35,0,-4.68},0,true},
  {TEXT("SM_EW_Microscope"),TEXT("Lab"),{23.59,.9325,4.65},180},
  {TEXT("SM_EW_Centrifuge"),TEXT("Lab"),{24.18,.9325,4.65},180},
  {TEXT("SM_EW_LabScale"),TEXT("Lab"),{24.78,.9325,4.63},180},
  {TEXT("SM_EW_SampleJar"),TEXT("Lab"),{23.86,.9305,4.90},180},
  {TEXT("SM_EW_ErlenmeyerFlask"),TEXT("Lab"),{24.52,.9305,4.87},180},
  // Separate instrument bench; the functional calibration bench at27/1.25 is untouched.
  {TEXT("SM_EW_GreenBench"),TEXT("Workshop"),{30.3,0,4.72},180,true},
  {TEXT("SM_EW_Oscilloscope_RM565"),TEXT("Lab"),{29.82,.9325,4.66},180,false,false,true},
  {TEXT("SM_EW_Multimeter"),TEXT("Lab"),{30.38,.9325,4.63},180},
  {TEXT("SM_EW_SolderingIron"),TEXT("Lab"),{30.82,.9325,4.63},180},
  {TEXT("SM_EW_RedToolChest"),TEXT("Workshop"),{27.02,0,4.72},180,true},
  {TEXT("SM_EW_DrawerCabinet"),TEXT("Workshop"),{28.24,0,4.72},180,true},
  {TEXT("SM_EW_Pegboard"),TEXT("Workshop"),{30.30,1.28,5.16},180},
  {TEXT("SM_EW_WallTools"),TEXT("Workshop"),{30.30,1.72,5.11},180},
  // North storage wall and east return. The depot28.8/-3.8 and its approach stay free.
  {TEXT("SM_EW_ShelvingStocked"),TEXT("Workshop"),{28.42,0,-4.91},0,true},
  {TEXT("SM_EW_GalvanizedShelving"),TEXT("Workshop"),{30.86,0,-.98},90,true},
  {TEXT("SM_EW_CrateFuses"),TEXT("Workshop"),{30.86,.5725,-.98},90},
  {TEXT("SM_EW_CrateRelays"),TEXT("Workshop"),{30.86,1.0125,-.98},90},
  {TEXT("SM_EW_CableDrum"),TEXT("Workshop"),{27.35,0,-1.35},0,true},
  // Garage perimeter only: delivery32.6/-4.15, central Jeep bay and doors are unobstructed.
  {TEXT("SM_EW_ChemicalCabinet"),TEXT("Workshop"),{37.82,0,-4.79},0,true},
  {TEXT("SM_EW_PartsCart"),TEXT("Workshop"),{37.87,0,-2.64},90,true},
  {TEXT("SM_EW_TyreStack"),TEXT("Workshop"),{37.86,0,-.44},0,true},
  {TEXT("SM_EW_FireExtinguisher"),TEXT("Workshop"),{38.17,.70,3.38},90},
  {TEXT("SM_EW_DrainGrate"),TEXT("Workshop"),{35.40,-.0535,.20},0},
  {TEXT("SM_EW_FluorescentFixture"),TEXT("Workshop"),{23.40,3.22,-.70},0,false,true},
  {TEXT("SM_EW_FluorescentFixture"),TEXT("Workshop"),{28.75,3.22,-2.10},90,false,true},
  {TEXT("SM_EW_FluorescentFixture"),TEXT("Workshop"),{28.85,3.22,3.10},90,false,true},
  {TEXT("SM_EW_FluorescentFixture"),TEXT("Workshop"),{35.40,3.22,.10},0,false,true},
  // Claude extras (Props/Extras): wall items keep their back on the inner wall face
  // (exterior inner faces x20.85 / z+-5.25, partitions .15 thick at x26, x31.6, z.5). Yaw-90 faces +x.
  {TEXT("SM_EW_LabSink"),TEXT("Extras"),{21.165,0,-1.20},-90,true},
  {TEXT("SM_EW_GlassCabinet"),TEXT("Extras"),{21.075,0,1.30},-90,true},
  {TEXT("SM_EW_ReagentShelf"),TEXT("Extras"),{20.862,1.55,-1.20},-90},
  {TEXT("SM_EW_LabStool"),TEXT("Extras"),{23.35,0,-3.15},0},
  {TEXT("SM_EW_LabStool"),TEXT("Extras"),{23.95,0,4.05},0},
  {TEXT("SM_EW_PosterGlasses"),TEXT("Extras"),{24.20,1.32,5.246},180},
  {TEXT("SM_EW_WallClock"),TEXT("Extras"),{25.889,2.02,-1.00},90},
  {TEXT("SM_EW_NoticeBoard"),TEXT("Extras"),{25.909,1.00,.45},90},
  {TEXT("SM_EW_PosterRF"),TEXT("Extras"),{30.30,1.25,.579},0},
  {TEXT("SM_EW_Calendar1986"),TEXT("Extras"),{27.30,1.22,.422},180},
  // Second stocked rack along the storage west wall.
  {TEXT("SM_EW_ShelvingStocked"),TEXT("Workshop"),{26.34,0,-2.60},-90,true},
  {TEXT("SM_EW_FirstAidBox"),TEXT("Extras"),{31.74,1.35,2.45},-90},
  // Garage working side (west wall between the two doors, clear of the Jeep lane x33.6..37.2).
  {TEXT("SM_EW_MechanicBench"),TEXT("Extras"),{32.05,0,.45},-90,true},
  {TEXT("SM_EW_AirCompressor"),TEXT("Extras"),{31.98,0,4.93},-90,true},
  {TEXT("SM_EW_JerryCans"),TEXT("Extras"),{32.95,0,-4.98},0,true},
  {TEXT("SM_EW_FloorJack"),TEXT("Extras"),{32.85,0,2.05},0},
  {TEXT("SM_EW_OilStains"),TEXT("Extras"),{35.30,0,-.60},20},
  // Room plaques beside each interior door, on the side of the room they name (bottom edge at 1.5 m).
  {TEXT("SM_EW_PlaqueLab"),TEXT("Extras"),{26.081,1.50,3.75},-90},
  {TEXT("SM_EW_PlaqueCalibration"),TEXT("Extras"),{25.919,1.50,2.05},90},
  {TEXT("SM_EW_PlaqueCalibration"),TEXT("Extras"),{31.681,1.50,4.30},-90},
  {TEXT("SM_EW_PlaqueStorage"),TEXT("Extras"),{29.75,1.50,.581},0},
  {TEXT("SM_EW_PlaqueStorage"),TEXT("Extras"),{31.681,1.50,-1.75},-90},
  {TEXT("SM_EW_PlaqueGarage"),TEXT("Extras"),{31.519,1.50,2.55},90},
  {TEXT("SM_EW_PlaqueGarage"),TEXT("Extras"),{31.519,1.50,-3.35},90},
  // Exterior: rooftop evaporative cooler over the garage; hose reel north of the east service door.
  {TEXT("SM_EW_SwampCooler"),TEXT("Extras"),{35.40,3.60,3.70},0},
  {TEXT("SM_EW_HoseReel"),TEXT("Extras"),{38.632,0,-.80},-90},
 };
 // SM_EW_RollUpDoorSegment is deliberately not a decorative duplicate of the shell's actual door.
}

bool FS47EastWingProps::Build(US47Main* Main,TArray<TWeakObjectPtr<USceneComponent>>& Parts)
{
 if(bBuilt)return true;
 if(!Main||!Main->Web)return false;
 AS47Web* Web=Main->Web;
 TMap<FString,UStaticMesh*> Meshes;
 bool Complete=true;
 for(const FPlacement& P:Layout)
 {
  const FString Path=FString(P.bPrivate?TEXT("/Game/Licensed/EastWing/Props/"):TEXT("/Game/EastWing/Props/"))+P.Group+TEXT("/")+P.Asset+TEXT(".")+P.Asset;
  if(Meshes.Contains(Path))continue;
  UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);
  if(!Mesh){UE_LOG(LogS47EastWingProps,Error,TEXT("S47: east wing furnishing missing %s; no partial furnishing installed"),*Path);Complete=false;}
  Meshes.Add(Path,Mesh);
 }
 if(!Complete)return false; // private RM565 is required for this complete local arrangement too.
 TArray<FS47Rect>& Colliders=Web->CollidersMutable(TEXT("saro"));
 for(int32 Index=0;Index<UE_ARRAY_COUNT(Layout);++Index)
 {
  const FPlacement& P=Layout[Index];
  const FString Path=FString(P.bPrivate?TEXT("/Game/Licensed/EastWing/Props/"):TEXT("/Game/EastWing/Props/"))+P.Group+TEXT("/")+P.Asset+TEXT(".")+P.Asset;
  UStaticMesh* Mesh=Meshes[Path];
  UStaticMeshComponent* C=NewObject<UStaticMeshComponent>(Web,*FString::Printf(TEXT("EastWingProp_%02d_%s"),Index,P.Asset));
  C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);C->SetCastShadow(true);
  // Imported UCX boxes are not interaction/collision proxies for small apparatus or open shelves.
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->SetupAttachment(Web->GetRootComponent());C->RegisterComponent();
  const FTransform Transform(FRotator(0,P.Yaw,0),S47Three::ToUE(P.At),FVector::OneVector);
  C->SetWorldTransform(Transform);Parts.Add(C);
  if(P.bFoot)
  {
   const FBox WorldBox=Mesh->GetBoundingBox().TransformBy(Transform);
   const FVector Min=S47Three::ToThree(WorldBox.Min),Max=S47Three::ToThree(WorldBox.Max);
   Colliders.Add({Min.X,Max.X,Min.Z,Max.Z,FString::Printf(TEXT("eastwing:prop:%02d"),Index),true});
  }
  for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)
  {
   UMaterialInterface* Original=C->GetMaterial(Slot);if(!Original)continue;
   FLinearColor Glow=FLinearColor::Black;
   const bool HasGlow=Original->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveFactor")),Glow);
   const bool Tube=P.bFixture&&Mesh->GetStaticMaterials().IsValidIndex(Slot)&&Mesh->GetStaticMaterials()[Slot].MaterialSlotName==TEXT("M_EW_FluorescentGlass");
   if(!HasGlow||(!Tube&&Glow.R+Glow.G+Glow.B<.001f))continue;
   UMaterialInstanceDynamic* Dynamic=UMaterialInstanceDynamic::Create(Original,C);
   if(Tube)Glow=FLinearColor(2.4f,2.25f,1.95f,1.f);
   Dynamic->SetVectorParameterValue(TEXT("EmissiveFactor"),FLinearColor::Black);
   C->SetMaterial(Slot,Dynamic);Emission.Add({Dynamic,Glow});
  }
 }
 bBuilt=true;
 UE_LOG(LogS47EastWingProps,Display,TEXT("S47: east wing furnishing: %d instances, %d distinct imported models, %d powered emissive slots; actual roll-up door supplied by shell"),UE_ARRAY_COUNT(Layout),Meshes.Num(),Emission.Num());
 return true;
}

void FS47EastWingProps::SetPowered(bool bPowered)
{
 for(const FEmission& E:Emission)if(UMaterialInstanceDynamic* M=E.Material.Get())M->SetVectorParameterValue(TEXT("EmissiveFactor"),bPowered?E.Powered:FLinearColor::Black);
}

#include "S47EastWingProps.h"
#include "S47Main.h"
#include "S47Web.h"
#include "S47Three.h"
#include "S47Canvas.h"
#include "S47Freeform.h"
#include "S47Story.h"
#include "S47GameStyle.h"
#include "Engine/Texture2D.h"
#include "Math/RotationMatrix.h"
#include "TextureResource.h"
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
  // Reference pass: long calibration casework run on the south wall replaces the bench + drawer cabinet.
  // (the 2.4 m calibration run itself is the Freeform station, built in S47FreeformWorld)
  {TEXT("SM_EW_LabStool"),TEXT("Extras"),{29.20,0,4.05},0},
  {TEXT("SM_EW_LabStool"),TEXT("Extras"),{30.35,0,4.10},0},
  {TEXT("SM_EW_Oscilloscope_RM565"),TEXT("Lab"),{29.45,.90,4.80},180,false,false,true},
  {TEXT("SM_EW_Multimeter"),TEXT("Lab"),{30.10,.90,4.75},180},
  {TEXT("SM_EW_SolderingIron"),TEXT("Lab"),{30.55,.90,4.75},180},
  {TEXT("SM_EW_RedToolChest"),TEXT("Workshop"),{27.02,0,4.72},180,true},
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
  // Photo pass: the 7 x 10 m garage and the lab get extra fixtures (each has a rect light in S47EastWing).
  {TEXT("SM_EW_FluorescentFixture"),TEXT("Workshop"),{35.40,3.22,-3.20},0,false,true},
  {TEXT("SM_EW_FluorescentFixture"),TEXT("Workshop"),{35.40,3.22,3.40},0,false,true},
  {TEXT("SM_EW_FluorescentFixture"),TEXT("Workshop"),{23.40,3.22,2.60},0,false,true},
  // Photo pass: waxed VCT over lab, storage and calibration (garage keeps bare concrete).
  {TEXT("SM_EW_VinylFloor"),TEXT("Extras"),{26.1875,0,0},0},
  // Reference pass: exposed steel joists under the deck (clear of fixtures and partitions) and ceiling EMT.
  {TEXT("SM_EW_SteelJoist"),TEXT("Extras"),{21.70,3.12,0},90},
  {TEXT("SM_EW_SteelJoist"),TEXT("Extras"),{22.80,3.12,0},90},
  {TEXT("SM_EW_SteelJoist"),TEXT("Extras"),{24.10,3.12,0},90},
  {TEXT("SM_EW_SteelJoist"),TEXT("Extras"),{25.30,3.12,0},90},
  {TEXT("SM_EW_SteelJoist"),TEXT("Extras"),{26.80,3.12,0},90},
  {TEXT("SM_EW_SteelJoist"),TEXT("Extras"),{27.90,3.12,0},90},
  {TEXT("SM_EW_SteelJoist"),TEXT("Extras"),{29.60,3.12,0},90},
  {TEXT("SM_EW_SteelJoist"),TEXT("Extras"),{30.70,3.12,0},90},
  {TEXT("SM_EW_SteelJoist"),TEXT("Extras"),{32.40,3.12,0},90},
  {TEXT("SM_EW_SteelJoist"),TEXT("Extras"),{33.60,3.12,0},90},
  {TEXT("SM_EW_SteelJoist"),TEXT("Extras"),{34.60,3.12,0},90},
  {TEXT("SM_EW_SteelJoist"),TEXT("Extras"),{36.30,3.12,0},90},
  {TEXT("SM_EW_SteelJoist"),TEXT("Extras"),{37.40,3.12,0},90},
  {TEXT("SM_EW_CeilingConduit"),TEXT("Extras"),{23.85,3.0,-1.40},0},
  {TEXT("SM_EW_CeilingConduit"),TEXT("Extras"),{23.85,3.0,4.40},0},
  {TEXT("SM_EW_CeilingConduit"),TEXT("Extras"),{28.85,3.0,-1.40},0},
  {TEXT("SM_EW_CeilingConduit"),TEXT("Extras"),{28.85,3.0,4.40},0},
  {TEXT("SM_EW_CeilingConduit"),TEXT("Extras"),{33.85,3.0,-1.40},0},
  {TEXT("SM_EW_CeilingConduit"),TEXT("Extras"),{33.85,3.0,4.40},0},
  // Reference pass: lab casework under the north windows and a two-sided island (concept board).
  // (the 2.4 m lab run under the windows is the Freeform sample station, built in S47FreeformWorld)
  {TEXT("SM_EW_CaseworkRun12"),TEXT("Extras"),{24.82,0,-4.925},0,true},
  {TEXT("SM_EW_CaseworkIsland"),TEXT("Extras"),{23.60,0,1.60},90,true},
  // Claude extras (Props/Extras): wall items keep their back on the inner wall face
  // (exterior inner faces x20.85 / z+-5.25, partitions .15 thick at x26, x31.6, z.5). Yaw-90 faces +x.
  {TEXT("SM_EW_LabSink"),TEXT("Extras"),{21.165,0,-1.20},-90,true},
  {TEXT("SM_EW_GlassCabinet"),TEXT("Extras"),{21.075,0,1.30},-90,true},
  {TEXT("SM_EW_ReagentShelf"),TEXT("Extras"),{20.862,1.55,-1.20},-90},
  {TEXT("SM_EW_LabStool"),TEXT("Extras"),{22.55,0,-4.10},0},
  {TEXT("SM_EW_LabStool"),TEXT("Extras"),{23.95,0,4.05},0},
  {TEXT("SM_EW_PosterGlasses"),TEXT("Extras"),{24.20,1.32,5.246},180},
  {TEXT("SM_EW_WallClock"),TEXT("Extras"),{25.889,2.02,-1.00},90},
  {TEXT("SM_EW_NoticeBoard"),TEXT("Extras"),{25.909,1.00,.45},90},
  {TEXT("SM_EW_PosterRF"),TEXT("Extras"),{30.30,1.25,.579},0},
  {TEXT("SM_EW_Calendar1986"),TEXT("Extras"),{27.30,1.22,.422},180},
  // T16-08: the same calendar (the same mesh, one more instance) on the control room's west wall, between the map and the
  // filing cabinets (map frame ends z-2.29, cabinets start z-1.48), clear of the windows (north wall) and the bulletin board
  // (south wall). Yaw-90 faces +x; the wall's front is x-5.975, the pivot sits 3 mm off it as the wing's own calendar does.
  {TEXT("SM_EW_Calendar1986"),TEXT("Extras"),{-5.972,1.40,-1.90},-90},
  // Second stocked rack along the storage west wall.
  {TEXT("SM_EW_ShelvingStocked"),TEXT("Workshop"),{26.34,0,-2.60},-90,true},
  {TEXT("SM_EW_FirstAidBox"),TEXT("Extras"),{31.74,1.35,2.45},-90},
  // Garage working side (west wall between the two doors, clear of the Jeep lane x33.6..37.2).
  {TEXT("SM_EW_MechanicBench"),TEXT("Extras"),{32.05,0,.45},-90,true},
  {TEXT("SM_EW_AirCompressor"),TEXT("Extras"),{31.98,0,4.93},-90,true},
  {TEXT("SM_EW_JerryCans"),TEXT("Extras"),{32.95,0,-4.98},0,true},
  {TEXT("SM_EW_FloorJack"),TEXT("Extras"),{32.85,0,2.05},0},
  // Photo pass 3: unique painted garage slab (lines, tyre tracks, oil, sand) replaces the flat stain decal mesh.
  {TEXT("SM_EW_GarageSlab"),TEXT("Extras"),{35.0125,0,0},0},
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

namespace
{
 // The printed month under SM_EW_Calendar1986's picture, redrawn for the game's date. The blade is 0.30 x 0.45 m; its
 // dark month bar starts 46.9% of the way down, so the overlay covers the lower 53.1% (0.30 x 0.239 m) at 1024 x 816 px
 // (3413 px/m). Same print: cream paper, dark green bar, condensed caps, red Sundays, a red ballpoint ring on today.
 // No notes, no Story text: the month, the year, the weekdays and the dates only.
 constexpr int32 CalW=1024,CalH=816;
 constexpr double CalBladeW=.298,CalBladeH=.238; // overlay size in metres (CalW:CalH)
 void DrawCalendar(const FDateTime& Today,TArray<FColor>& Out)
 {
  static const TCHAR* Months[]={TEXT("JANUARY"),TEXT("FEBRUARY"),TEXT("MARCH"),TEXT("APRIL"),TEXT("MAY"),TEXT("JUNE"),TEXT("JULY"),TEXT("AUGUST"),TEXT("SEPTEMBER"),TEXT("OCTOBER"),TEXT("NOVEMBER"),TEXT("DECEMBER")};
  static const TCHAR* Days[]={TEXT("SUN"),TEXT("MON"),TEXT("TUE"),TEXT("WED"),TEXT("THU"),TEXT("FRI"),TEXT("SAT")};
  const uint32 Ink=0x22221e,Red=0x78281e,Rule=0x96928a;
  int32 First=0,Count=0,Rows=0;S47Game::MonthGrid(Today.GetYear(),Today.GetMonth(),First,Count,Rows);
  FS47Canvas G(CalW,CalH);
  G.SetFill(0xf0ece0);G.FillRect(0,0,CalW,CalH);
  G.SetFill(0x1e2822);G.FillRect(0,0,CalW,92);
  G.SetFont(TEXT("Oswald-Medium.ttf"),58);G.TextAlign=FS47Canvas::EAlign::Center;
  G.SetFill(0xecE6d2);G.FillText(FString::Printf(TEXT("%s %d"),Months[Today.GetMonth()-1],Today.GetYear()),CalW/2,70);
  const double X0=41,CW=(CalW-82)/7.,Top=170,Bottom=695,CH=(Bottom-Top)/Rows;
  G.SetFont(TEXT("Oswald-Regular.ttf"),30);
  for(int32 C=0;C<7;++C){G.SetFill(C==0?Red:Ink);G.FillText(Days[C],X0+C*CW+CW/2,150);}
  G.SetStroke(Rule);G.LineWidth=2;
  const int32 Size=Rows>=6?36:42;
  for(int32 Cell=0;Cell<Rows*7;++Cell)
  {
   const int32 R=Cell/7,C=Cell%7,Day=Cell-First+1;
   const double X=X0+C*CW,Y=Top+R*CH;
   G.StrokeRect(X,Y,CW,CH);
   if(Day<1||Day>Count)continue;
   G.SetFont(TEXT("Oswald-Medium.ttf"),Size);G.TextAlign=FS47Canvas::EAlign::Left;
   G.SetFill(C==0?Red:Ink);G.FillText(FString::FromInt(Day),X+12,Y+Size+8);
   if(Day==Today.GetDay())
   {
    // the day ringed in red ballpoint, drawn twice a little off true
    G.SetStroke(0xa01e1a,.9);G.LineWidth=5;
    const double Rad=FMath::Min(CW,CH)*.46,Cx=X+CW/2,Cy=Y+CH/2;
    G.BeginPath();G.Arc(Cx,Cy,Rad,.2,2*PI+.35);G.Stroke();
    G.BeginPath();G.Arc(Cx+2,Cy-1.5,Rad+1.5,2.5,2*PI+2.9);G.Stroke();
    G.SetStroke(Rule);G.LineWidth=2;
   }
  }
  G.SetFont(TEXT("Oswald-Regular.ttf"),30);G.TextAlign=FS47Canvas::EAlign::Center;
  G.SetFill(0x3c3830);G.FillText(TEXT("CALICHE FEED & SUPPLY  \u00b7  ROUTE 47"),CalW/2,CalH-30);
  Out=MoveTemp(G.Px);
  // the print's aged edge: a soft darkening over the outer 40 px
  for(int32 Y=0;Y<CalH;++Y)for(int32 X=0;X<CalW;++X)
  {
   const double E=FMath::Min(FMath::Min(X,CalW-1-X),FMath::Min(Y,CalH-1-Y));
   if(E>=40)continue;
   const double K=1.-.16*FMath::Square(1.-E/40.);
   FColor& P=Out[Y*CalW+X];P.R=uint8(P.R*K);P.G=uint8(P.G*K);P.B=uint8(P.B*K);
  }
 }
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
 TArray<UStaticMeshComponent*> Blades;
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
  if(FCString::Strcmp(P.Asset,TEXT("SM_EW_Calendar1986"))==0)Blades.Add(C);
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
 BuildCalendars(Web,Blades,Parts);
 bBuilt=true;
 UE_LOG(LogS47EastWingProps,Display,TEXT("S47: east wing furnishing: %d instances, %d distinct imported models, %d powered emissive slots; actual roll-up door supplied by shell"),UE_ARRAY_COUNT(Layout),Meshes.Num(),Emission.Num());
 return true;
}

void FS47EastWingProps::SetLevel(float Level)
{
 for(const FEmission& E:Emission)if(UMaterialInstanceDynamic* M=E.Material.Get())M->SetVectorParameterValue(TEXT("EmissiveFactor"),E.Powered*Level);
}

void FS47EastWingProps::BuildCalendars(AS47Web* Web,const TArray<UStaticMeshComponent*>& Blades,TArray<TWeakObjectPtr<USceneComponent>>& Parts)
{
 UStaticMesh* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
 UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 UTexture2D* Tex=UTexture2D::CreateTransient(CalW,CalH,PF_B8G8R8A8);
 if(!Plane||!Base||!Tex||Blades.IsEmpty()){UE_LOG(LogS47EastWingProps,Display,TEXT("S47: wall calendars keep their printed sheet (plane %d, material %d, texture %d, blades %d)"),Plane!=nullptr,Base!=nullptr,Tex!=nullptr,Blades.Num());return;}
 Tex->SRGB=true;Tex->Filter=TF_Bilinear;
 UMaterialInstanceDynamic* Mat=UMaterialInstanceDynamic::Create(Base,Web);
 Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.86f,.86f,.86f));Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
 Mat->SetScalarParameterValue(TEXT("Roughness"),.86f);Mat->SetScalarParameterValue(TEXT("Metalness"),0);Mat->SetScalarParameterValue(TEXT("EmissivePower"),0);
 Mat->SetScalarParameterValue(TEXT("UseMap"),1);Mat->SetScalarParameterValue(TEXT("MapIsLinear"),0);Mat->SetTextureParameterValue(TEXT("Map"),Tex);
 // Engine plane (100 cm, normal +Z, u=(x+50)/100, v=(y+50)/100). Turned so its normal is the blade's front (the mesh's local +Y,
 // where Blender -Y lands) with plane +X to the reader's right and plane +Y down: the image reads upright with u'=u, v'=v.
 Mat->SetVectorParameterValue(TEXT("MapU"),FLinearColor(1,0,0,0));Mat->SetVectorParameterValue(TEXT("MapV"),FLinearColor(0,1,0,0));
 const FQuat Face=FRotationMatrix::MakeFromXZ(FVector(1,0,0),FVector(0,1,0)).ToQuat();
 int32 N=0;
 for(UStaticMeshComponent* Blade:Blades)
 {
  UStaticMeshComponent* Sheet=NewObject<UStaticMeshComponent>(Web,*FString::Printf(TEXT("EastWingCalendarSheet_%d"),N++));
  Sheet->SetMobility(EComponentMobility::Movable);Sheet->SetStaticMesh(Plane);Sheet->SetMaterial(0,Mat);
  Sheet->SetCollisionEnabled(ECollisionEnabled::NoCollision);Sheet->SetCastShadow(false);
  Sheet->SetupAttachment(Blade);Sheet->RegisterComponent();
  // blade z 0..45 cm, print face 0.04 cm in front of the mesh origin plane; the overlay sits 0.14 cm out, flush with the blade's bottom
  Sheet->SetRelativeTransform(FTransform(Face,FVector(0,.14,CalBladeH*50.),FVector(CalBladeW,CalBladeH,1)));
  Parts.Add(Sheet);CalendarSheets.Add(Sheet);
 }
 CalendarTexture=Tex;
}

void FS47EastWingProps::UpdateCalendar(US47Main* Main)
{
 UTexture2D* Tex=CalendarTexture.Get();
 if(!Main||!Tex||CalendarSheets.IsEmpty())return;
 const bool Free=Main->IsFreeform();
 const FDateTime Date=S47Game::GameDate(Free,Free&&Main->Freeform?Main->Freeform->Day():1,Main->Story?Main->Story->Clock():12*3600.);
 const int32 Key=Date.GetYear()*10000+Date.GetMonth()*100+Date.GetDay();
 if(Key==CalendarKey)return; // the sheet changes with the day, not with the frame
 TArray<FColor> Px;DrawCalendar(Date,Px);
 if(Px.Num()!=CalW*CalH)return;
 void* Data=Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
 FMemory::Memcpy(Data,Px.GetData(),Px.Num()*sizeof(FColor));
 Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
 Tex->UpdateResource();
 CalendarKey=Key;
 UE_LOG(LogS47EastWingProps,Display,TEXT("S47: wall calendar sheet drawn: %s, %d calendars (wing and control room share one %dx%d sheet)"),*Date.ToString(TEXT("%a %b %d %Y")).ToUpper(),CalendarSheets.Num(),CalW,CalH);
}

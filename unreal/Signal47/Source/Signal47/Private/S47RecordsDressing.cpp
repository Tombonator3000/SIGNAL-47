// PT-RECORDS (Tom's test of d36, 9 November 2026): the records room's realism round. Everything here is reused: the owned
// archive carton (/Game/Licensed/SaroInteriors, private, guarded; plain card cartons when it is absent), the retro office
// desk, lamp and desk things (/Game/Free/Office/retro_office_props), engine cubes, planes and cylinders for binders, rolls
// and flattened cartons, and one label atlas typed at run time with the project's own Special Elite / Reenie Beanie fonts
// (FS47Canvas, as the east wing's calendar). No new model, no import.
//
// Three metres (x, height y, z). The room: x -6.0 (west wall) to -1.0 (east wall), z 6.8 (north partition) to 10.5 (south
// wall), walk zone annex.zone.records x -5.75..-1.25, z 7.05..10.25, player radius 0.27. What stands there already and is
// left as it is: the ArchiveProps shelving (x -5.99..-5.48, z 7.1..10.2; four full rows, the top shelf at 2.131 empty), the
// three filing cabinets (x -5.25..-3.67, z 9.68..10.48), the bookcase (x -3.3..-1.4, z 10.12..10.48; its middle shelf at
// 0.45 empty), the card index (x -1.73..-0.985, z 7.49..8.42, top 1.078, the LINEAGE plate above it on the wall), the work
// table and its chair, typewriter, phone and lamp, the wall phone (z 9.1..9.4), the switchboard and recorder (z 10.0..10.4),
// the REEL047 fire drawer and the STATION 01 box (x -6..-5.4, z 8.4..8.8, y 1.1..1.4), the papers' fall area on the floor
// (x -5.27..-4.45, z 8.0..9.2), the door and its sweep (hinge x -3.42, z 6.82; quarter circle to x -4.38 and -2.46, open
// leaf x -3.42..-2.47 at z 6.8..6.9), the extinguisher (x -4.75, z 6.95) and the switch (-4.75, 1.35, 6.825).
//
// Placed (all NoCollision; walking is kept off the larger pieces with colliders in area "saro"):
//   shelf top     eight cartons on the empty top shelf (2.131), two stacks of two, typed 1986 labels to the room;
//   bookcase      eighteen ring binders on the empty middle shelf, typed spine slips, one leaning, two lying flat;
//   index top     two binders lying on the card index, clear under the LINEAGE plate;
//   clerk desk    a narrow steel desk against the north partition east of the door's open leaf (x -2.38..-1.02,
//                 z 6.82..7.28), its lamp with one small warm rect light, a carton of returns, a sign-out ledger, a tray;
//   floor stack   two cartons in the gap between the cabinets and the bookcase (x -3.67..-3.30);
//   corners       rolled plans in the dead pocket between the shelving and the cabinets, flattened cartons leaning on the
//                 north wall west of the extinguisher.
// The desk's collider, widened by the walker's 0.27 m, still clears the freeform test walk (-3.9, 7.5) > (-2.3, 7.6) >
// (-2.3, 9.6) by about 3 cm, and the desk leaves the door's open leaf 9 cm.
#include "S47SaroFixes.h"

#include "S47Canvas.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/RectLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "TextureResource.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47RecordsDressing, Log, All);

namespace S47RecordsDressing
{
namespace
{
 enum EPart : int32 { ShelfTop, Bookcase, IndexTop, Desk, FloorStack, Corners, PartCount };
 const TCHAR* PartNames[PartCount] = {TEXT("shelf top"),TEXT("bookcase binders"),TEXT("index cabinet top"),TEXT("clerk desk"),TEXT("floor stack"),TEXT("corners")};

 const TCHAR* CartonPackage=TEXT("/Game/Licensed/SaroInteriors/SM_InteriorOwned_ArchiveBox");
 const TCHAR* RetroRoot=TEXT("/Game/Free/Office/retro_office_props/StaticMeshes/retro_office_props_");

 struct FPartState
 {
  bool bBuilt=false, bSaid=false;
  int32 Props=0;
  FString Why;
  TArray<FString> Missing;
  TArray<TWeakObjectPtr<UPrimitiveComponent>> Parts;
 };
 struct FState
 {
  TWeakObjectPtr<AS47Web> Web;
  FPartState Parts[PartCount];
  double Retry=0;
  bool bAtlasTried=false, bAtlasText=false, bCartonSaid=false;
  TWeakObjectPtr<UTexture2D> Atlas;
  TMap<uint32,TWeakObjectPtr<UMaterialInstanceDynamic>> Colours;
  TMap<int32,TWeakObjectPtr<UMaterialInstanceDynamic>> Tints;
  TWeakObjectPtr<URectLightComponent> Lamp;
 };
 TMap<TWeakObjectPtr<US47Main>,FState> States;

 // ---------- the label atlas: 1024 px, 24 carton cards (256 x 128) over 16 spine slips (256 x 64) ----------
 constexpr int32 AtlasSize=1024, CardW=256, CardH=128, SpineH=64, CardCells=24, SpineTop=768;
 struct FCardText { const TCHAR* A; const TCHAR* B; bool bHand; };
 // Typed on the station's typewriter between 1977 and 1986; ordinary station business only.
 const FCardText Cards[]={
  {TEXT("OBS LOGS"),TEXT("1979-80"),false},            // 0
  {TEXT("RECEIVER CAL"),TEXT("1984"),false},           // 1
  {TEXT("PURCHASE ORDERS"),TEXT("FY85"),false},        // 2
  {TEXT("DISH S-14"),TEXT("SERVICE"),false},           // 3
  {TEXT("STRIP CHARTS"),TEXT("JAN-JUN 1983"),false},   // 4
  {TEXT("TIME CARDS"),TEXT("1982-84"),false},          // 5
  {TEXT("GENERATOR LOGS"),TEXT("1981"),false},         // 6
  {TEXT("INTERFERENCE RPTS"),TEXT("1985"),false},      // 7
  {TEXT("SITE PLANS"),TEXT("SUPERSEDED"),false},       // 8
  {TEXT("MAINT. TICKETS"),TEXT("1985"),false},         // 9
  {TEXT("CORRESPONDENCE"),TEXT("1980-81"),false},      // 10
  {TEXT("RETURNS"),TEXT("TO BE REFILED"),false},       // 11
  {TEXT("DO NOT DISCARD"),nullptr,true},               // 12
  {TEXT("FCC FILINGS"),TEXT("1977-84"),false},         // 13
  {TEXT("CRYO SERVICE"),TEXT("1983"),false},           // 14
  {TEXT("SURVEY PLATES"),TEXT("AZ 000-090"),false},    // 15
  {TEXT("FILE SIGN-OUT"),TEXT("1986"),false},          // 16
  {TEXT("TAPE CATALOGUE"),TEXT("1978-81"),false},      // 17
 };
 const TCHAR* Spines[]={
  TEXT("PURCHASE ORDERS FY85"),TEXT("PURCHASE ORDERS FY86"),TEXT("RECEIVER CAL 1984"),TEXT("RECEIVER CAL 1985"),
  TEXT("DISH S-14 SERVICE"),TEXT("DISH S-09 SERVICE"),TEXT("OBS LOGS 1979"),TEXT("OBS LOGS 1980"),
  TEXT("SAFETY MANUAL REV C"),TEXT("DRIVE MOTORS VOL 2"),TEXT("LO CHAIN SCHEMATICS"),TEXT("SITE VISITS 1984"),
  TEXT("BUDGET FY86"),TEXT("INVENTORY 1985"),TEXT("WEATHER LOG 1985"),TEXT("CORRESPONDENCE 85"),
 };
 constexpr int32 SpineCell(int32 K) { return CardCells+K; }

 FIntRect CellPx(int32 Cell)
 {
  if (Cell<CardCells) { const int32 X=(Cell%4)*CardW, Y=(Cell/4)*CardH; return FIntRect(X,Y,X+CardW,Y+CardH); }
  const int32 K=Cell-CardCells, X=(K%4)*CardW, Y=SpineTop+(K/4)*SpineH;
  return FIntRect(X,Y,X+CardW,Y+SpineH);
 }

 bool DrawAtlas(TArray<FColor>& Out)
 {
  static const uint32 Stock[]={0xe9e2cc,0xe2d9bf,0xefe8d4,0xd9cfb3};
  FS47Canvas G(AtlasSize,AtlasSize);
  G.SetFill(0xe6dfca); G.FillRect(0,0,AtlasSize,AtlasSize);
  bool bText=true;
  for (int32 I=0; I<UE_ARRAY_COUNT(Cards); ++I)
  {
   const FIntRect R=CellPx(I); const double Cx=R.Min.X+CardW*.5;
   G.SetFill(Stock[I%4]); G.FillRect(R.Min.X,R.Min.Y,CardW,CardH);
   G.SetStroke(0x8f866f,.7); G.LineWidth=2; G.StrokeRect(R.Min.X+6,R.Min.Y+6,CardW-12,CardH-12);
   G.TextAlign=FS47Canvas::EAlign::Center;
   if (Cards[I].bHand)
   {
    G.SetFont(TEXT("ReenieBeanie-Regular.ttf"),54); G.SetFill(0x30303a);
    bText&=G.FillText(Cards[I].A,Cx,R.Min.Y+80,CardW-24);
    continue;
   }
   G.SetFill(0x24221d);
   G.SetFont(TEXT("SpecialElite-Regular.ttf"),31); bText&=G.FillText(Cards[I].A,Cx,R.Min.Y+55,CardW-26);
   if (Cards[I].B) { G.SetFont(TEXT("SpecialElite-Regular.ttf"),28); bText&=G.FillText(Cards[I].B,Cx,R.Min.Y+100,CardW-26); }
  }
  for (int32 K=0; K<UE_ARRAY_COUNT(Spines); ++K)
  {
   const FIntRect R=CellPx(SpineCell(K));
   G.SetFill(Stock[(K+1)%4]); G.FillRect(R.Min.X,R.Min.Y,CardW,SpineH);
   G.SetStroke(0x9a917a,.6); G.LineWidth=2; G.StrokeRect(R.Min.X+3,R.Min.Y+3,CardW-6,SpineH-6);
   G.TextAlign=FS47Canvas::EAlign::Center; G.SetFill(0x24221d);
   G.SetFont(TEXT("SpecialElite-Regular.ttf"),26); bText&=G.FillText(Spines[K],R.Min.X+CardW*.5,R.Min.Y+41,CardW-16);
  }
  Out=MoveTemp(G.Px);
  return bText;
 }

 UTexture2D* Atlas(FState& S)
 {
  if (S.Atlas.IsValid() || S.bAtlasTried) return S.Atlas.Get();
  S.bAtlasTried=true;
  TArray<FColor> Px; S.bAtlasText=DrawAtlas(Px);
  UTexture2D* Tex=UTexture2D::CreateTransient(AtlasSize,AtlasSize,PF_B8G8R8A8);
  if (!Tex || Px.Num()!=AtlasSize*AtlasSize) return nullptr;
  Tex->SRGB=true; Tex->Filter=TF_Bilinear;
  void* Data=Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
  FMemory::Memcpy(Data,Px.GetData(),Px.Num()*sizeof(FColor));
  Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
  Tex->UpdateResource();
  S.Atlas=Tex;
  if (!S.bAtlasText) UE_LOG(LogS47RecordsDressing,Warning,TEXT("S47: records labels: the fonts could not be drawn here; the cards stay blank"));
  return Tex;
 }

 // ---------- placing ----------
 struct FCtx
 {
  AS47Web* W=nullptr; FState* S=nullptr; FPartState* P=nullptr;
  int32 Parent=-1; USceneComponent* ParentComp=nullptr;
  UStaticMesh* Cube=nullptr; UStaticMesh* Plane=nullptr; UStaticMesh* Cylinder=nullptr; UMaterialInterface* Base=nullptr;
 };

 UStaticMesh* Optional(const FString& Package)
 {
  if (!FPackageName::DoesPackageExist(Package)) return nullptr;
  UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*(Package+TEXT(".")+FPackageName::GetShortName(Package)),nullptr,LOAD_NoWarn|LOAD_Quiet);
  return Mesh && !Mesh->IsCompiling() ? Mesh : nullptr;
 }
 UStaticMesh* Retro(FCtx& C,const TCHAR* Name)
 {
  const FString Package=FString(RetroRoot)+Name;
  UStaticMesh* Mesh=Optional(Package);
  if (!Mesh) C.P->Missing.AddUnique(Package);
  return Mesh;
 }

 UMaterialInterface* Colour(FCtx& C,uint32 Hex,float Rough=.85f,float Metal=0.f)
 {
  const uint32 Key=Hex^(uint32(Rough*100)<<24)^(uint32(Metal*10)<<28);
  if (const auto* Found=C.S->Colours.Find(Key)) if (Found->IsValid()) return Found->Get();
  UMaterialInstanceDynamic* Mat=UMaterialInstanceDynamic::Create(C.Base,C.W);
  Mat->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(Hex)); Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
  Mat->SetScalarParameterValue(TEXT("Roughness"),Rough); Mat->SetScalarParameterValue(TEXT("Metalness"),Metal);
  Mat->SetScalarParameterValue(TEXT("EmissivePower"),0);
  C.S->Colours.Add(Key,Mat);
  return Mat;
 }

 // Three yaw (degrees, about +y) after a local tilt; the UE yaw is its negative (as S47FieldModels).
 FQuat Turn(double YawDeg,const FQuat& Tilt=FQuat::Identity) { return FQuat(FRotator(0,-YawDeg,0))*Tilt; }

 enum class ESeat : uint8 { Pivot, Bottom, BottomCentre };
 // A mesh at a Three point. Pivot: the mesh's own origin there. Bottom: its lowest point on At.Y. BottomCentre: and its
 // bounds centred on At's x and z. MaxSpan (metres): scaled down, never up, so its footprint is no wider than that.
 UStaticMeshComponent* Place(FCtx& C,const FString& Name,UStaticMesh* Mesh,const FVector& At,const FQuat& Rot,const FVector& ScaleUE,
                             UMaterialInterface* Mat,ESeat Seat=ESeat::Pivot,double MaxSpan=0,bool bShadow=true)
 {
  if (!Mesh || !C.ParentComp) return nullptr;
  UStaticMeshComponent* Comp=NewObject<UStaticMeshComponent>(C.W,*Name);
  Comp->SetupAttachment(C.ParentComp); Comp->SetMobility(EComponentMobility::Movable); Comp->SetStaticMesh(Mesh);
  if (Mat) Comp->SetMaterial(0,Mat);
  Comp->SetCollisionProfileName(TEXT("NoCollision")); Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Comp->SetCollisionResponseToAllChannels(ECR_Ignore); Comp->SetGenerateOverlapEvents(false); Comp->SetCanEverAffectNavigation(false);
  Comp->SetCastShadow(bShadow);
  C.W->AddInstanceComponent(Comp); Comp->RegisterComponent();
  Comp->SetWorldTransform(FTransform(Rot,S47Three::ToUE(At),ScaleUE));
  if (MaxSpan>0)
  {
   const FVector Size=Comp->Bounds.GetBox().GetSize()/100.0;
   const double Span=FMath::Max(Size.X,Size.Y);
   if (Span>MaxSpan) Comp->SetWorldScale3D(ScaleUE*(MaxSpan/Span));
  }
  if (Seat!=ESeat::Pivot)
  {
   const FBox B=Comp->Bounds.GetBox(); const FVector Want=S47Three::ToUE(At);
   FVector Delta(0,0,Want.Z-B.Min.Z);
   if (Seat==ESeat::BottomCentre) { Delta.X=Want.X-B.GetCenter().X; Delta.Y=Want.Y-B.GetCenter().Y; }
   Comp->AddWorldOffset(Delta);
  }
  // Adopted once it stands where it belongs: the node takes its place among annex.interior's children, so it shows and
  // hides with the records room.
  C.W->AdoptNode(Comp,C.Parent,Name);
  C.P->Parts.Add(Comp); ++C.P->Props;
  return Comp;
 }
 // A box of the given Three size (x, height, z) around its centre.
 UStaticMeshComponent* Box(FCtx& C,const FString& Name,const FVector& Centre,const FVector& Size,UMaterialInterface* Mat,
                           double YawDeg=0,const FQuat& Tilt=FQuat::Identity,bool bShadow=true)
 {
  return Place(C,Name,C.Cube,Centre,Turn(YawDeg,Tilt),FVector(Size.X,Size.Z,Size.Y),Mat,ESeat::Pivot,0,bShadow);
 }
 // One cell of the atlas on a flat card: its face toward Normal, its text running along Right (both Three directions).
 // Length along Right, Height across it, in metres.
 UStaticMeshComponent* Label(FCtx& C,const FString& Name,int32 Cell,const FVector& Centre,const FVector& Normal,const FVector& Right,
                             double Length,double Height)
 {
  UTexture2D* Tex=Atlas(*C.S);
  if (!Tex || !C.Plane) return nullptr;
  const FIntRect R=CellPx(Cell);
  UMaterialInstanceDynamic* Mat=UMaterialInstanceDynamic::Create(C.Base,C.W);
  Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.9f,.9f,.9f)); Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
  Mat->SetScalarParameterValue(TEXT("Roughness"),.9f); Mat->SetScalarParameterValue(TEXT("Metalness"),0);
  Mat->SetScalarParameterValue(TEXT("EmissivePower"),0);
  Mat->SetScalarParameterValue(TEXT("UseMap"),1); Mat->SetScalarParameterValue(TEXT("MapIsLinear"),0); Mat->SetTextureParameterValue(TEXT("Map"),Tex);
  // Engine plane: u=(x+50)/100, v=(y+50)/100; plane +X along the text, +Y down the card (as the east wing's calendar).
  // The cell, half a texel in from its edges: u'=Su*u+U0, v'=Sv*v+V0.
  const float U0=(R.Min.X+.5f)/AtlasSize, V0=(R.Min.Y+.5f)/AtlasSize, Su=(R.Width()-1.f)/AtlasSize, Sv=(R.Height()-1.f)/AtlasSize;
  Mat->SetVectorParameterValue(TEXT("MapU"),FLinearColor(Su,0,U0,0)); Mat->SetVectorParameterValue(TEXT("MapV"),FLinearColor(0,Sv,V0,0));
  const FQuat Q=FRotationMatrix::MakeFromXZ(S47Three::DirToUE(Right).GetSafeNormal(),S47Three::DirToUE(Normal).GetSafeNormal()).ToQuat();
  return Place(C,Name,C.Plane,Centre,Q,FVector(Length,Height,1),Mat,ESeat::Pivot,0,false);
 }
 // Three directions of the card faces used here, with the reader's right for each.
 const FVector ToEast(1,0,0), ToWest(-1,0,0), ToSouth(0,0,1), ToNorth(0,0,-1), Up(0,1,0), Down(0,-1,0);

 FBox ThreeBox(const UPrimitiveComponent* Comp)
 {
  const FBox U=Comp->Bounds.GetBox();
  return FBox(FVector(U.Min.X,U.Min.Z,U.Min.Y)/100.0,FVector(U.Max.X,U.Max.Z,U.Max.Y)/100.0);
 }

 // An archive carton standing on At (bottom centre), its long side along Three x at yaw 0. The owned carton when it is
 // here (a tint per carton: aged, dusty, greyer board), else a card body and lid from cubes. Its Three bounds.
 FBox Carton(FCtx& C,const FString& Name,const FVector& At,double YawDeg,int32 Tint)
 {
  static const FLinearColor Tints[]={FLinearColor(1,1,1),FLinearColor(.9f,.88f,.84f),FLinearColor(.82f,.82f,.8f),FLinearColor(.96f,.9f,.8f)};
  static const uint32 Board[]={0xa3865d,0x9a7e58,0xb8ae98,0xb0946a};
  if (UStaticMesh* Owned=Optional(CartonPackage))
  {
   UStaticMeshComponent* Comp=Place(C,Name,Owned,At,Turn(YawDeg),FVector::OneVector,nullptr,ESeat::Bottom);
   if (!Comp) return FBox(ForceInit);
   const int32 K=Tint%UE_ARRAY_COUNT(Tints);
   UMaterialInstanceDynamic* Mid=nullptr;
   if (const auto* Found=C.S->Tints.Find(K)) Mid=Found->Get();
   if (!Mid && Comp->GetMaterial(0))
   {
    Mid=UMaterialInstanceDynamic::Create(Comp->GetMaterial(0),C.W);
    Mid->SetVectorParameterValue(TEXT("TextureTint"),Tints[K]);
    C.S->Tints.Add(K,Mid);
   }
   if (Mid) Comp->SetMaterial(0,Mid);
   return ThreeBox(Comp);
  }
  if (!C.S->bCartonSaid) UE_LOG(LogS47RecordsDressing,Display,TEXT("S47: records cartons: %s absent; plain card cartons instead"),CartonPackage);
  C.S->bCartonSaid=true;
  C.P->Missing.AddUnique(FString(CartonPackage)+TEXT(" (plain cartons instead)"));
  const uint32 Hex=Board[Tint%UE_ARRAY_COUNT(Board)];
  UStaticMeshComponent* Body=Box(C,Name,At+FVector(0,.115,0),FVector(.42,.23,.34),Colour(C,Hex,.92f),YawDeg);
  UStaticMeshComponent* Lid=Box(C,Name+TEXT("Lid"),At+FVector(0,.241,0),FVector(.432,.022,.352),Colour(C,Hex-0x080808,.92f),YawDeg);
  FBox B(ForceInit);
  if (Body) B+=ThreeBox(Body);
  if (Lid) B+=ThreeBox(Lid);
  return B;
 }
 // A carton's label on its face toward Dir (east, south or north): the bounds end at the lid's overhang (5.5 to 8 mm
 // proud of the owned carton's board, 6 mm of the plain one), so the card sits 4.5 mm inside them.
 void CartonLabel(FCtx& C,const FString& Name,const FBox& B,const FVector& Dir,int32 Cell,double Up01=.42)
 {
  if (!B.IsValid) return;
  constexpr double In=.0045;
  const double Y=B.Min.Y+(B.Max.Y-B.Min.Y)*Up01;
  if (Dir.X>.5) Label(C,Name,Cell,FVector(B.Max.X-In,Y,B.GetCenter().Z),ToEast,ToNorth,.19,.095);
  else if (Dir.Z>.5) Label(C,Name,Cell,FVector(B.GetCenter().X,Y,B.Max.Z-In),ToSouth,ToEast,.19,.095);
  else Label(C,Name,Cell,FVector(B.GetCenter().X,Y,B.Min.Z+In),ToNorth,ToWest,.19,.095);
 }
 // A thin grey-brown film of dust on a lid, a little inside its edge.
 void Dust(FCtx& C,const FString& Name,const FBox& B)
 {
  if (!B.IsValid) return;
  Box(C,Name,FVector(B.GetCenter().X,B.Max.Y+.0008,B.GetCenter().Z),FVector(B.GetSize().X-.035,.0015,B.GetSize().Z-.035),Colour(C,0xb3aa95,1.f),0,FQuat::Identity,false);
 }

 void Collider(AS47Web* W,const TCHAR* Id,double X0,double X1,double Z0,double Z1)
 {
  TArray<FS47Rect>& All=W->CollidersMutable(TEXT("saro"));
  for (FS47Rect& R:All) if (R.Id==Id) { R.MinX=X0; R.MaxX=X1; R.MinZ=Z0; R.MaxZ=Z1; R.bEnabled=true; return; }
  All.Add({X0,X1,Z0,Z1,FString(Id),true});
 }

 // ArchiveProps furniture (S47FieldModels, all or nothing) where its assembly puts it. The binders and cartons stand on
 // its shelves and tops, which the web's own furniture does not have at the same heights.
 bool Furniture(FCtx& C,const TCHAR* Name,const FVector& Expect,FString& Why)
 {
  const FS47Obj Found=C.W->ObjAt(C.Parent).Find(Name);
  const FS47WebNode* N=Found?C.W->Node(Found.I):nullptr;
  const USceneComponent* Comp=N?N->Comp.Get():nullptr;
  if (!Comp) { Why=FString::Printf(TEXT("waiting: %s not built (field models off or rejected)"),Name); return false; }
  const FVector At=S47Three::ToThree(Comp->GetComponentLocation());
  if (!At.Equals(Expect,.02)) { Why=FString::Printf(TEXT("waiting: %s stands at (%.3f, %.3f, %.3f), measured at (%.3f, %.3f, %.3f)"),Name,At.X,At.Y,At.Z,Expect.X,Expect.Y,Expect.Z); return false; }
  return true;
 }

 // ---------- the parts ----------
 bool BuildShelfTop(FCtx& C)
 {
  if (!Furniture(C,TEXT("SM_ArchiveProps_ShelvingStorage"),FVector(-5.735,0,8.65),C.P->Why)) return false;
  // Top shelf surface 2.131 (sheet at 2.12, 22 mm). Uprights at z 7.15, 8.65, 10.15 (x -5.947 and -5.523) and the back
  // braces at x -5.96 bound two bays, z 7.19..8.61 and 8.69..10.11; a carton's back at -5.945, its front at -5.52. The
  // inventory plate (x -5.489, y 2.2..2.29, z 7.32..7.75) hides the first carton's face, so it has no label.
  constexpr double Y=2.132, X=-5.7325;
  struct FSpot { double Z; int32 Tint, Cell; double TopZ; int32 TopTint, TopCell; };
  // Bay one 7.40 / 7.86 (its label from 7.765, past the plate) / 8.30; bay two 8.88 / 9.52 / 9.92.
  const FSpot Spots[]={
   {7.40,2,-1,0,0,-1},{7.86,0,0,7.845,2,5},{8.30,3,4,0,0,-1},
   {8.88,1,1,0,0,-1},{9.52,2,6,9.535,1,12},{9.92,0,10,0,0,-1},
  };
  int32 N=0;
  for (const FSpot& Spot:Spots)
  {
   const FString Tag=FString::Printf(TEXT("records:shelfCarton%d"),N++);
   const FBox B=Carton(C,Tag,FVector(X,Y,Spot.Z),0,Spot.Tint);
   if (Spot.Cell>=0) CartonLabel(C,Tag+TEXT("Label"),B,ToEast,Spot.Cell,.4);
   if (Spot.TopZ>0 && B.IsValid)
   {
    const FBox T=Carton(C,Tag+TEXT("Top"),FVector(X+.004,B.Max.Y+.001,Spot.TopZ),0,Spot.TopTint);
    if (Spot.TopCell>=0) CartonLabel(C,Tag+TEXT("TopLabel"),T,ToEast,Spot.TopCell,.45);
   }
  }
  return true;
 }

 bool BuildBookcase(FCtx& C)
 {
  if (!Furniture(C,TEXT("SM_ArchiveProps_Bookcase"),FVector(-2.35,0,10.3),C.P->Why)) return false;
  // Middle shelf top 0.45 (shelf at .43, 40 mm), the shelf above from .86; inside faces x -3.26 and -1.42; front edge
  // z 10.12, hardboard back from 10.46. Vinyl ring binders .27 deep, spines 15 mm behind the front edge.
  constexpr double Y=.45, Front=10.135, Depth=.27;
  static const uint32 Vinyl[]={0x26364f,0x1d1d20,0x5a2626,0x3d4a33,0x26364f,0x8a7a5a,0x1d1d20,0x5d6062};
  static const double Widths[]={.062,.070,.055,.075,.060,.068,.058,.072};
  static const double Heights[]={.305,.31,.29,.315,.30};
  int32 Spine=0, N=0;
  auto Upright=[&](double Left,int32 I,bool bLabel)
  {
   const double Wd=Widths[I%8], Ht=Heights[I%5];
   const FString Tag=FString::Printf(TEXT("records:binder%d"),N++);
   Box(C,Tag,FVector(Left+Wd*.5,Y+Ht*.5,Front+Depth*.5),FVector(Wd,Ht,Depth),Colour(C,Vinyl[I%8],.55f));
   // The finger hole low on the spine, and the typed slip in its window.
   Box(C,Tag+TEXT("Hole"),FVector(Left+Wd*.5,Y+.045,Front-.0006),FVector(.022,.022,.002),Colour(C,0x101010,.9f),0,FQuat::Identity,false);
   if (bLabel && Spine<UE_ARRAY_COUNT(Spines))
    Label(C,Tag+TEXT("Spine"),SpineCell(Spine++),FVector(Left+Wd*.5,Y+Ht*.6,Front-.0012),ToNorth,Down,.165,Wd-.016);
   return Left+Wd+.002;
  };
  // West group, packed against the side; the last one leans back onto the row.
  double X=-3.255;
  for (int32 I=0; I<8; ++I) X=Upright(X,I,I!=5);
  {
   const double Wd=.066, Ht=.305, Lean=9.;
   const FQuat Tilt(FVector(0,1,0),FMath::DegreesToRadians(-Lean)); // top toward -x
   const double Sn=FMath::Sin(FMath::DegreesToRadians(Lean)), Cs=FMath::Cos(FMath::DegreesToRadians(Lean));
   // Its lower east corner on the shelf 5 cm past the row, its top resting on the row's last binder.
   const double Cx=X+.05+Wd*.5*Cs-Ht*.5*Sn, Cy=Y+Ht*.5*Cs+Wd*.5*Sn;
   Box(C,FString::Printf(TEXT("records:binder%d"),N++),FVector(Cx,Cy,Front+Depth*.5),FVector(Wd,Ht,Depth),Colour(C,0x5a2626,.55f),0,Tilt);
  }
  // East group from the middle, then two lying flat at the east side.
  X=-2.42;
  for (int32 I=0; I<7; ++I) X=Upright(X,I+3,true);
  for (int32 K=0; K<2; ++K)
  {
   const double Th=.055, Yc=Y+Th*.5+K*Th;
   const FString Tag=FString::Printf(TEXT("records:binderFlat%d"),K);
   Box(C,Tag,FVector(-1.59+K*.008,Yc,Front+Depth*.5+K*.006),FVector(.31,Th,Depth),Colour(C,K?0x3d4a33:0x26364f,.55f));
   if (Spine<UE_ARRAY_COUNT(Spines)) Label(C,Tag+TEXT("Spine"),SpineCell(Spine++),FVector(-1.59+K*.008,Yc,Front+K*.006-.0012),ToNorth,ToWest,.17,.04);
  }
  return true;
 }

 bool BuildIndexTop(FCtx& C)
 {
  if (!Furniture(C,TEXT("SM_ArchiveProps_CardIndex"),FVector(-1.25,0,7.95),C.P->Why)) return false;
  // Top overhang x -1.515..-0.985, z 7.485..8.415, at 1.078. The LINEAGE plate is on the wall above it (x -1.009,
  // y 1.21..1.39, z 7.7..8.2): two binders, 11 cm together, lie in front of its south end and stay under it.
  constexpr double Top=1.078;
  for (int32 K=0; K<2; ++K)
  {
   const double Th=.055, Yc=Top+Th*.5+K*Th, Xc=-1.36+K*.012, Zc=8.255-K*.02;
   const FString Tag=FString::Printf(TEXT("records:indexBinder%d"),K);
   Box(C,Tag,FVector(Xc,Yc,Zc),FVector(.27,Th,.31),Colour(C,K?0x5a2626:0x1d1d20,.55f));
   Label(C,Tag+TEXT("Spine"),SpineCell(8+K*6),FVector(Xc-.1362,Yc,Zc),ToWest,ToSouth,.18,.042);
  }
  return true;
 }

 bool BuildDesk(FCtx& C)
 {
  UStaticMesh* DeskMesh=Retro(C,TEXT("PI_TankerDesk"));
  UStaticMesh* LampMesh=Retro(C,TEXT("PI_DeskLamp"));
  if (!DeskMesh || !LampMesh) { C.P->Why=TEXT("waiting: the retro office desk or lamp is absent"); return false; }
  // The tanker desk of Dale's office, made a narrow clerk's desk 1.36 x 0.46 m at its own height, its kneehole to the
  // room (yaw 0; Dale's stands at yaw 180 with the chair on -z). Its west end 9 cm clear of the open door leaf.
  constexpr double X0=-2.38, X1=-1.02, Z0=6.82, Z1=7.28;
  const FBox Raw=DeskMesh->GetBoundingBox();
  const FVector RawSize=Raw.GetSize();
  if (RawSize.X<1 || RawSize.Y<1) { C.P->Why=TEXT("waiting: the desk mesh has no bounds yet"); return false; }
  const FVector Scale((X1-X0)*100/RawSize.X,(Z1-Z0)*100/RawSize.Y,1);
  UStaticMeshComponent* DeskComp=Place(C,TEXT("records:clerkDesk"),DeskMesh,FVector((X0+X1)*.5,0,(Z0+Z1)*.5),Turn(0),Scale,nullptr,ESeat::BottomCentre);
  if (!DeskComp) { C.P->Why=TEXT("waiting: annex.interior has no component"); return false; }
  Collider(C.W,TEXT("records:clerkDesk"),X0,X1,Z0-.02,Z1);
  const double T=ThreeBox(DeskComp).Max.Y;
  // West end: a carton of returns waiting to be refiled, its label to the room.
  const FBox Returns=Carton(C,TEXT("records:deskReturns"),FVector(-2.13,T,7.04),0,0);
  CartonLabel(C,TEXT("records:deskReturnsLabel"),Returns,ToSouth,11,.42);
  // The lamp, back of the desk west of the middle, shade to the room.
  UStaticMeshComponent* LampComp=Place(C,TEXT("records:deskLamp"),LampMesh,FVector(-1.68,T,6.95),Turn(180),FVector::OneVector,nullptr,ESeat::Bottom,.44);
  // The sign-out ledger, a pen on it; the in-tray and a pencil at the east end.
  UStaticMeshComponent* Ledger=Box(C,TEXT("records:signOutLedger"),FVector(-1.47,T+.014,7.17),FVector(.30,.028,.22),Colour(C,0x2e4a38,.8f),-4);
  if (Ledger)
  {
   Label(C,TEXT("records:signOutLabel"),16,FVector(-1.47,T+.0284,7.15),Up,ToEast,.13,.065);
   if (UStaticMesh* Pen=Retro(C,TEXT("Pen_B"))) Place(C,TEXT("records:signOutPen"),Pen,FVector(-1.40,T+.0285,7.20),Turn(75),FVector::OneVector,nullptr,ESeat::Bottom,.16);
  }
  if (UStaticMesh* Tray=Retro(C,TEXT("FileContainer"))) Place(C,TEXT("records:deskTray"),Tray,FVector(-1.19,T,7.0),Turn(-3),FVector(.8),nullptr,ESeat::BottomCentre,.30);
  if (UStaticMesh* Pencil=Retro(C,TEXT("Pencil_B"))) Place(C,TEXT("records:deskPencil"),Pencil,FVector(-1.20,T,7.22),Turn(12),FVector::OneVector,nullptr,ESeat::Bottom,.2);
  // One small warm pool under the shade, as Dale's OperationsDeskLight.
  if (LampComp && !C.S->Lamp.IsValid())
  {
   const FBox L=ThreeBox(LampComp);
   URectLightComponent* Light=NewObject<URectLightComponent>(C.W,TEXT("RecordsDeskLight"));
   Light->SetMobility(EComponentMobility::Movable); Light->SetupAttachment(C.ParentComp);
   Light->SetWorldLocation(S47Three::ToUE(FVector(L.GetCenter().X,L.Max.Y-.06,L.GetCenter().Z))); Light->SetWorldRotation(FRotator(-90,0,0));
   Light->SetSourceWidth(34); Light->SetSourceHeight(6); Light->SetIntensityUnits(ELightUnits::Lumens); Light->SetIntensity(2.2f);
   Light->SetLightColor(FLinearColor(1,.72f,.46f)); Light->SetAttenuationRadius(80); Light->SetCastShadows(true);
   Light->SetVolumetricScatteringIntensity(0); Light->SetVisibility(false);
   C.W->AddInstanceComponent(Light); Light->RegisterComponent();
   C.S->Lamp=Light;
  }
  return true;
 }

 bool BuildFloorStack(FCtx& C)
 {
  // The gap between the cabinets (x max -3.67) and the bookcase (x min -3.30), against the south wall (10.49): two
  // cartons turned with their short faces to the room, dust on the upper lid.
  const FBox Low=Carton(C,TEXT("records:floorCarton0"),FVector(-3.485,0,10.258),90,1);
  if (!Low.IsValid) { C.P->Why=TEXT("waiting: annex.interior has no component"); return false; }
  CartonLabel(C,TEXT("records:floorCarton0Label"),Low,ToNorth,2,.42);
  const FBox High=Carton(C,TEXT("records:floorCarton1"),FVector(-3.485,Low.Max.Y+.001,10.268),90,2);
  CartonLabel(C,TEXT("records:floorCarton1Label"),High,ToNorth,7,.45);
  Dust(C,TEXT("records:floorCartonDust"),High);
  Collider(C.W,TEXT("records:floorCartons"),-3.67,-3.30,10.04,10.48);
  return true;
 }

 bool BuildCorners(FCtx& C)
 {
  if (!C.Cylinder) { C.P->Missing.AddUnique(TEXT("/Engine/BasicShapes/Cylinder")); C.P->Why=TEXT("waiting: engine cylinder absent"); return false; }
  // Rolled plans in the pocket between the shelving's south foot (x -5.478) and the cabinets' west side (x -5.25), out of
  // reach of the walk: their feet at x -5.43, each leaning (about 9 to 10 degrees) until its top touches the cabinet.
  static const uint32 Paper[]={0xdcdcd2,0xcfd6dc,0xd8cfb4};
  static const double Zs[]={9.96,10.11,10.27}, Lengths[]={.90,.84,.92}, Dia[]={.07,.06,.075};
  for (int32 K=0; K<3; ++K)
  {
   const double Lean=FMath::Asin((.177-Dia[K]*.5)/Lengths[K]);
   const FQuat Tilt(FVector(0,1,0),Lean); // top toward +x
   const double Cx=-5.43+Lengths[K]*.5*FMath::Sin(Lean), Cy=Lengths[K]*.5*FMath::Cos(Lean)+Dia[K]*.5*FMath::Sin(Lean);
   Place(C,FString::Printf(TEXT("records:rolledPlans%d"),K),C.Cylinder,FVector(Cx,Cy,Zs[K]),Turn(0,Tilt),FVector(Dia[K],Dia[K],Lengths[K]),Colour(C,Paper[K],.95f));
  }
  // Flattened cartons on the north wall (face z 6.806) between the shelving's north end (x -5.478) and the extinguisher
  // (x -4.83), leaning 14 degrees, 1.5 cm apart.
  struct FBoard { double X, W, H, Foot; uint32 Hex; };
  const FBoard Boards[]={{-5.20,.54,.80,7.02,0x9c8460},{-5.16,.46,.70,7.035,0xa58d68}};
  int32 N=0;
  for (const FBoard& B:Boards)
  {
   const double Lean=FMath::DegreesToRadians(14.);
   const FQuat Tilt(FVector(1,0,0),Lean); // top toward -z
   Box(C,FString::Printf(TEXT("records:flatCarton%d"),N++),FVector(B.X,B.H*.5*FMath::Cos(Lean),B.Foot-B.H*.5*FMath::Sin(Lean)),FVector(B.W,B.H,.012),Colour(C,B.Hex,.95f),0,Tilt);
  }
  Collider(C.W,TEXT("records:flatCartons"),-5.48,-4.92,6.80,7.06);
  return true;
 }

 bool PartsAlive(const FPartState& P)
 {
  for (const auto& Part:P.Parts) if (!Part.IsValid()) return false;
  return true;
 }

 FString OffReason()
 {
  if (S47SaroFixes::Before()) return TEXT("off: -S47SaroFixesBefore (the room as it was, for the before pictures)");
  if (FParse::Param(FCommandLine::Get(),TEXT("S47RecordsDressingOff"))) return TEXT("off: -S47RecordsDressingOff");
  return FString();
 }
}

void Update(US47Main* M,double Dt)
{
 if (!M || !OffReason().IsEmpty()) return;
 for (auto It=States.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
 AS47Web* W=M->Web.Get(); if (!W || !W->IsLoaded()) return;
 FState& S=States.FindOrAdd(TWeakObjectPtr<US47Main>(M));
 if (S.Web.Get()!=W) { S=FState(); S.Web=W; }
 const bool bSaro=M->World && M->World->Area==TEXT("saro") && W->Area()==TEXT("saro");
 const FS47Obj Interior=W->Obj(TEXT("annex.interior"));
 S.Retry-=Dt;
 bool bAll=true;
 for (const FPartState& P:S.Parts) bAll&=P.bBuilt && PartsAlive(P);
 if (!bAll && S.Retry<=0 && bSaro)
 {
  S.Retry=2;
  FCtx C; C.W=W; C.S=&S; C.Parent=Interior.I;
  const FS47WebNode* N=W->Node(Interior.I); C.ParentComp=N?N->Comp.Get():nullptr;
  C.Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
  C.Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
  C.Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
  C.Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
  for (int32 I=0; I<PartCount; ++I)
  {
   FPartState& P=S.Parts[I];
   if (P.bBuilt && PartsAlive(P)) continue;
   // Rebuilt whole when the web dropped any of its pieces.
   for (const auto& Part:P.Parts) if (UPrimitiveComponent* Comp=Part.Get()) Comp->DestroyComponent();
   if (I==Desk) if (URectLightComponent* Light=S.Lamp.Get()) { Light->DestroyComponent(); S.Lamp.Reset(); }
   const bool bWasSaid=P.bSaid; const FString OldWhy=P.Why;
   P=FPartState();
   C.P=&P;
   if (!C.ParentComp || !C.Cube || !C.Plane || !C.Base) { P.Why=TEXT("waiting: annex.interior, engine shapes or the web material absent"); continue; }
   bool bOk=false;
   switch (I)
   {
    case ShelfTop: bOk=BuildShelfTop(C); break;
    case Bookcase: bOk=BuildBookcase(C); break;
    case IndexTop: bOk=BuildIndexTop(C); break;
    case Desk: bOk=BuildDesk(C); break;
    case FloorStack: bOk=BuildFloorStack(C); break;
    case Corners: bOk=BuildCorners(C); break;
    default: break;
   }
   P.bBuilt=bOk;
   if (bOk)
   {
    P.Why.Reset();
    UE_LOG(LogS47RecordsDressing,Display,TEXT("S47: records %s built: %d props%s%s"),PartNames[I],P.Props,
           P.Missing.Num()?TEXT("; missing "):TEXT(""),*FString::Join(P.Missing,TEXT(", ")));
   }
   else
   {
    // A part that stopped half way leaves nothing behind.
    for (const auto& Part:P.Parts) if (UPrimitiveComponent* Comp=Part.Get()) Comp->DestroyComponent();
    P.Parts.Reset(); P.Props=0;
    // Said once per reason, not every retry.
    if (!bWasSaid || OldWhy!=P.Why) UE_LOG(LogS47RecordsDressing,Display,TEXT("S47: records %s %s"),PartNames[I],*P.Why);
    P.bSaid=true;
   }
  }
 }
 if (URectLightComponent* Light=S.Lamp.Get())
 {
  const bool bOn=M->Mode==US47Main::EMode::Play && bSaro && Interior.Shown() && M->Story && !M->Story->bDark;
  if (Light->IsVisible()!=bOn) Light->SetVisibility(bOn);
 }
}

void Report(US47Main* M,TArray<FString>& Lines)
{
 const FString Off=OffReason();
 const FState* S=M?States.Find(TWeakObjectPtr<US47Main>(M)):nullptr;
 for (int32 I=0; I<PartCount; ++I)
 {
  FString Line=FString::Printf(TEXT("PT-RECORDS %s: "),PartNames[I]);
  if (!Off.IsEmpty()) Line+=Off;
  else if (!S || !S->Web.IsValid()) Line+=TEXT("waiting: SARO's web scene is not loaded yet");
  else
  {
   const FPartState& P=S->Parts[I];
   if (P.bBuilt && PartsAlive(P)) Line+=FString::Printf(TEXT("built, %d props"),P.Props);
   else Line+=P.Why.IsEmpty()?FString(TEXT("waiting: not tried yet (SARO not entered)")):P.Why;
   if (P.Missing.Num()) Line+=TEXT("; missing ")+FString::Join(P.Missing,TEXT(", "));
   if (I==Desk && P.bBuilt) Line+=S->Lamp.IsValid()?(S->Lamp->IsVisible()?TEXT("; desk lamp lit"):TEXT("; desk lamp dark")):TEXT("; no desk lamp light");
  }
  Lines.Add(Line);
 }
 if (Off.IsEmpty() && S && S->bAtlasTried)
  Lines.Add(FString::Printf(TEXT("PT-RECORDS labels: %s"),!S->Atlas.IsValid()?TEXT("atlas texture failed, no labels"):S->bAtlasText?TEXT("typed atlas drawn"):TEXT("atlas made but the fonts did not draw (blank cards)")));
}
}

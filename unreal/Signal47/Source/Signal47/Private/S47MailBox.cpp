#include "S47MailBox.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"

namespace S47MailBoxLocal
{
 const TCHAR* const CubePath=TEXT("/Engine/BasicShapes/Cube.Cube");
 const TCHAR* const DrumPath=TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
 const TCHAR* const LitPath=TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque");
 const TCHAR* const WoodPath=TEXT("/Game/FarmBuildings/Materials/M_Farm_Wood.M_Farm_Wood");
 // Galvanized steel a good many summers old: gray and matte, not bright (the critic's picture had it white in the sun), darker on top
 // where the sun and the dust sit, the door a little lighter, the pressed beads catching some light; thin rust, road dust, soil,
 // paint red gone a little toward brick, the reflector's amber, kraft paper and twine for a parcel.
 constexpr uint32 Galvanized=0x8c8f8a,Weathered=0x7b7f7b,DoorTone=0x959893,Bead=0xa2a5a0,Rust=0x6e4228,Dust=0x8f8372,Scratch=0xb9bbb6,Soil=0x5e5446,FlagRed=0xc0281c,
  Bolt=0x55585a,Amber=0xc0561f,Plank=0x6b5d4c,Kraft=0x9c7c55,Twine=0xcdb995,Label=0xe6e1d4;
 bool Exists(const TCHAR* Path){return FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(Path)));}
 UMaterialInstanceDynamic* Tone(AS47Web* Web,UMaterialInterface* ParentMat,uint32 Rgb,float Rough,float Metal)
 {
  if(!ParentMat)return nullptr;
  auto* Mat=UMaterialInstanceDynamic::Create(ParentMat,Web);
  Mat->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(Rgb));Mat->SetScalarParameterValue(TEXT("Roughness"),Rough);Mat->SetScalarParameterValue(TEXT("Metalness"),Metal);
  Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);Mat->SetScalarParameterValue(TEXT("EmissivePower"),0);
  return Mat;
 }
 UStaticMeshComponent* Part(AS47Web* Web,USceneComponent* Onto,UStaticMesh* Mesh,UMaterialInterface* Mat,const FTransform& At)
 {
  auto* Comp=NewObject<UStaticMeshComponent>(Web);Comp->SetMobility(EComponentMobility::Movable);Comp->SetupAttachment(Onto);Comp->SetStaticMesh(Mesh);
  if(Mat)Comp->SetMaterial(0,Mat);
  Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);Comp->SetCastShadow(true);Comp->bCastContactShadow=false;Comp->SetReceivesDecals(false);
  Comp->SetRelativeTransform(At);Comp->RegisterComponent();
  return Comp;
 }
}

bool FS47MailBoxModel::Built() const{return Root.IsValid()&&For.IsValid();}
bool FS47MailBoxModel::Build(AS47Web* Web,int32 Parent)
{
 namespace L=S47MailBoxLocal;
 if(Built()&&For.Get()==Web)return true;
 const FS47WebNode* Under=Web?Web->Node(Parent):nullptr;
 if(!Under||!Under->Comp.IsValid())return false;
 UStaticMesh* CubeMesh=LoadObject<UStaticMesh>(nullptr,L::CubePath);UStaticMesh* DrumMesh=LoadObject<UStaticMesh>(nullptr,L::DrumPath);
 UMaterialInterface* Lit=LoadObject<UMaterialInterface>(nullptr,L::LitPath);
 if(!CubeMesh||!DrumMesh||!Lit)return false;
 UMaterialInterface* Wood=L::Exists(L::WoodPath)?LoadObject<UMaterialInterface>(nullptr,L::WoodPath):nullptr;
 if(!Wood)Wood=L::Tone(Web,Lit,L::Plank,.92f,0.f);
 // Matte: roughness about .7 and little metal, so the sun does not turn it white.
 UMaterialInterface* Steel=L::Tone(Web,Lit,L::Galvanized,.72f,.3f),*TopMat=L::Tone(Web,Lit,L::Weathered,.78f,.25f),*Door=L::Tone(Web,Lit,L::DoorTone,.7f,.3f),*Beads=L::Tone(Web,Lit,L::Bead,.62f,.35f);
 UMaterialInterface* RustMat=L::Tone(Web,Lit,L::Rust,.92f,0.f),*DustMat=L::Tone(Web,Lit,L::Dust,.96f,0.f),*ScratchMat=L::Tone(Web,Lit,L::Scratch,.5f,.45f),*SoilMat=L::Tone(Web,Lit,L::Soil,.95f,0.f);
 UMaterialInterface* Red=L::Tone(Web,Lit,L::FlagRed,.62f,.08f),*BoltMat=L::Tone(Web,Lit,L::Bolt,.55f,.5f),*Reflector=L::Tone(Web,Lit,L::Amber,.3f,.1f);
 UMaterialInterface* KraftMat=L::Tone(Web,Lit,L::Kraft,.9f,0.f),*TwineMat=L::Tone(Web,Lit,L::Twine,.95f,0.f),*LabelMat=L::Tone(Web,Lit,L::Label,.85f,0.f);
 For=Web;Parts.Reset();ParcelParts.Reset();
 auto* Base0=NewObject<USceneComponent>(Web);Base0->SetMobility(EComponentMobility::Movable);Base0->SetupAttachment(Under->Comp.Get());Base0->RegisterComponent();Root=Base0;
 auto* Pivot=NewObject<USceneComponent>(Web);Pivot->SetMobility(EComponentMobility::Movable);Pivot->SetupAttachment(Base0);Pivot->RegisterComponent();FlagPivot=Pivot;
 // Centimetres under the post's foot: x toward the box's back (east), y south, z up. Rotations: pitch 90 lays a cylinder along x,
 // roll 90 along y.
 Pivot->SetRelativeLocation(FVector(6.,8.75,116.5));
 const auto Piece=[&](UStaticMesh* Mesh,UMaterialInterface* Mat,const FVector& At,const FRotator& Turn,const FVector& SizeCm,USceneComponent* Onto)
 {
  UStaticMeshComponent* Comp=L::Part(Web,Onto,Mesh,Mat,FTransform(Turn,At,SizeCm/100.));Parts.Add(Comp);
  return Comp;
 };
 const FRotator Flat(0,0,0),AlongX(90,0,0),AlongY(0,0,90);
 // The post, a four-by-four a hand into the ground, the soil that splashes up it, the board and its brace.
 Piece(CubeMesh,Wood,FVector(0,0,44.5),Flat,FVector(8.9,8.9,119),Base0);
 Piece(CubeMesh,SoilMat,FVector(0,0,7),Flat,FVector(9.1,9.1,14),Base0);
 Piece(CubeMesh,Wood,FVector(-14,0,105.9),Flat,FVector(47,14,3.8),Base0);
 {
  const FVector A(-4.45,0,74),B(-31,0,104.1),D=B-A;const double Len=D.Size();
  const FQuat Q=FRotationMatrix::MakeFromZX(D/Len,FVector(0,1,0)).ToQuat();
  auto* Comp=Piece(CubeMesh,Wood,(A+B)*.5,Q.Rotator(),FVector(3.8,6.,Len),Base0);
  Comp->SetRelativeRotation(Q);
 }
 Piece(DrumMesh,BoltMat,FVector(-4.6,0,90.5),AlongX,FVector(1.4,1.4,1.2),Base0);   // the brace's lag bolt
 // The box: straight sides 13.5 cm high, the rounded top, two pressed beads, the door with its rim, latch and hinge.
 Piece(CubeMesh,Steel,FVector(-14,0,114.55),Flat,FVector(47.2,16.5,13.5),Base0);
 Piece(DrumMesh,TopMat,FVector(-14,0,121.3),AlongX,FVector(16.5,16.5,48),Base0);
 for(const double X:{-32.5,5.})
 {
  Piece(DrumMesh,Beads,FVector(X,0,121.3),AlongX,FVector(16.9,16.9,.8),Base0);
  Piece(CubeMesh,Beads,FVector(X,0,114.55),Flat,FVector(.8,16.9,13.5),Base0);
 }
 Piece(CubeMesh,Door,FVector(-38.35,0,114.55),Flat,FVector(.7,16.9,13.7),Base0);
 Piece(DrumMesh,Door,FVector(-38.35,0,121.3),AlongX,FVector(16.9,16.9,.7),Base0);
 Piece(CubeMesh,Door,FVector(-39.25,0,127.4),Flat,FVector(1.2,2.8,2.),Base0);       // the thumb latch
 Piece(DrumMesh,BoltMat,FVector(-38.45,0,107.95),AlongY,FVector(.9,.9,16.),Base0);   // the hinge
 // Wear: road dust on the lower half of both sides, a few scratches by the door where hands go, and rust only as thin lines: along
 // the bottom seams, a narrow run down the door from under the latch, and a little at the back.
 for(const double Side:{-1.,1.})
 {
  Piece(CubeMesh,DustMat,FVector(-14.5,Side*8.29,110.6),Flat,FVector(45,.08,5.2),Base0);
  Piece(CubeMesh,RustMat,FVector(-15,Side*8.33,108.15),Flat,FVector(42,.1,.55),Base0);
 }
 Piece(CubeMesh,ScratchMat,FVector(-33.5,-8.31,117.2),FRotator(14,0,0),FVector(5.5,.06,.22),Base0);
 Piece(CubeMesh,ScratchMat,FVector(-31.,-8.31,115.6),FRotator(-9,0,0),FVector(3.8,.06,.2),Base0);
 Piece(CubeMesh,ScratchMat,FVector(-34.,8.31,118.4),FRotator(6,0,0),FVector(4.2,.06,.2),Base0);
 Piece(CubeMesh,RustMat,FVector(-38.75,.4,123.2),Flat,FVector(.1,.7,4.2),Base0);
 Piece(CubeMesh,RustMat,FVector(9.75,-3.,108.9),Flat,FVector(.12,4.,.7),Base0);
 // A reflector on the post, toward the highway.
 Piece(DrumMesh,Reflector,FVector(-4.75,0,95.),AlongX,FVector(7.5,7.5,.6),Base0);
 // The flag: its pivot bolt on the side, a short arm and a big plate, all one red. Down it lies along the side toward the door; up
 // it stands a foot over the top, its face north and south, so it shows edge-on to nobody: from the truck pad, north-east of it,
 // it reads as a red square a good deal bigger than the box's end.
 Piece(DrumMesh,BoltMat,FVector(6.,8.85,116.5),AlongY,FVector(1.6,1.6,1.3),Base0);
 Piece(CubeMesh,Red,FVector(-6,.2,0),Flat,FVector(12,.35,2.4),Pivot);
 Piece(CubeMesh,Red,FVector(-27,.25,0),Flat,FVector(30,.4,20),Pivot);
 // Painted by hand, as rural boxes were: SARO and the box number large on the side toward the station and the yard, the number on
 // the side the flag leaves clear and on the door, where the carrier looks.
 struct FStencil{const TCHAR* Words;double X,Y,Z;float Yaw,Size;};
 for(const FStencil& Mark:{FStencil{TEXT("SARO"),-14.,-8.37,116.8,-90.f,6.5f},FStencil{TEXT("BOX 19"),-14.,-8.37,110.2,-90.f,3.1f},
  FStencil{TEXT("19"),-1.5,8.37,112.,90.f,5.f},FStencil{TEXT("19"),-38.82,0.,113.6,180.f,3.4f}})
 {
  auto* Stencil=NewObject<UTextRenderComponent>(Web);Stencil->SetMobility(EComponentMobility::Movable);Stencil->SetupAttachment(Base0);
  Stencil->SetText(FText::FromString(Mark.Words));Stencil->SetWorldSize(Mark.Size);
  Stencil->SetHorizontalAlignment(EHTA_Center);Stencil->SetVerticalAlignment(EVRTA_TextCenter);Stencil->SetTextRenderColor(FColor(30,28,26));
  Stencil->SetCollisionEnabled(ECollisionEnabled::NoCollision);Stencil->SetCastShadow(false);Stencil->RegisterComponent();
  Stencil->SetRelativeLocationAndRotation(FVector(Mark.X,Mark.Y,Mark.Z),FRotator(0,Mark.Yaw,0));
  Parts.Add(Stencil);
 }
 // A parcel on top: brown paper, twine both ways, a white label toward the road and one toward the yard.
 {
  const FRotator Turn(0,7,0);
  const auto Parcel=[&](UMaterialInterface* Mat,const FVector& At,const FVector& SizeCm)
  {
   UStaticMeshComponent* Comp=L::Part(Web,Base0,CubeMesh,Mat,FTransform(Turn,At,SizeCm/100.));Comp->SetHiddenInGame(true);ParcelParts.Add(Comp);
  };
  Parcel(KraftMat,FVector(-15,0,137.6),FVector(31,22,16));
  Parcel(TwineMat,FVector(-15,0,137.6),FVector(31.3,.5,16.3));
  Parcel(TwineMat,FVector(-15,0,137.6),FVector(.5,22.3,16.3));
  Parcel(LabelMat,FVector(-30.6,2.5,139.),FVector(.15,9,6));
  Parcel(LabelMat,FVector(-10.,-11.08,139.),FVector(9,.15,6));
 }
 // What the eye's ray finds: the box and the top of its post, nobody sees it.
 {
  auto* Comp=NewObject<UStaticMeshComponent>(Web);Comp->SetMobility(EComponentMobility::Movable);Comp->SetupAttachment(Base0);Comp->SetStaticMesh(CubeMesh);
  Comp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Comp->SetCollisionResponseToAllChannels(ECR_Ignore);Comp->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Comp->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
  Comp->SetRelativeTransform(FTransform(FRotator::ZeroRotator,FVector(-15,0,118),FVector(.58,.3,.56)));Comp->SetHiddenInGame(true);Comp->SetCastShadow(false);Comp->RegisterComponent();
  PickPart=Comp;
  Place();
  Pick=Web->ObjAt(Web->AdoptNode(Comp,Parent,TEXT("ff:mailbox")));
 }
 Place();
 UE_LOG(LogTemp,Display,TEXT("S47: mail: the box at the road, %d pieces, at %.2f %.2f"),Parts.Num()+ParcelParts.Num(),PostX,PostZ);
 return true;
}
void FS47MailBoxModel::Place()
{
 if(USceneComponent* Base0=Root.Get())Base0->SetWorldLocationAndRotation(S47Three::ToUE(FVector(PostX,Ground,PostZ)),FRotator(-.8,-2.5,1.3));
}
void FS47MailBoxModel::Seat(AS47Web* Web)
{
 if(bSeated||!Web||Web!=For.Get())return;
 UWorld* Wld=Web->GetWorld();if(!Wld)return;
 FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47MailBoxGround),true);
 if(UPrimitiveComponent* Comp=PickPart.Get())Q.AddIgnoredComponent(Comp);
 for(const auto& Ptr:Parts)if(UPrimitiveComponent* Comp=Ptr.Get())Q.AddIgnoredComponent(Comp);
 for(const auto& Ptr:ParcelParts)if(UPrimitiveComponent* Comp=Ptr.Get())Q.AddIgnoredComponent(Comp);
 if(!Wld->LineTraceSingleByChannel(Hit,S47Three::ToUE(FVector(PostX,DesertY+25,PostZ)),S47Three::ToUE(FVector(PostX,DesertY-25,PostZ)),ECC_Visibility,Q))return;
 Ground=S47Three::ToThree(Hit.ImpactPoint).Y;bSeated=true;Place();
 UE_LOG(LogTemp,Display,TEXT("S47: mail: the box's post stands on the ground at %.2f"),Ground);
}
void FS47MailBoxModel::SetParcel(bool bOn)
{
 if(bParcel==bOn)return;
 bParcel=bOn;
 // Hidden in game rather than invisible, so that a parent shown again does not bring it back with it.
 for(const auto& Ptr:ParcelParts)if(UPrimitiveComponent* Comp=Ptr.Get())Comp->SetHiddenInGame(!(bParcel&&bShown));
}
void FS47MailBoxModel::Step(double Dt)
{
 if(FMath::IsNearlyEqual(FlagK,FlagTo))return;
 FlagK=FMath::FInterpConstantTo(FlagK,FlagTo,float(FMath::Max(0.,Dt)),2.2f);
 // Up: the arm turns from along the side, pointing at the door, to straight up.
 if(USceneComponent* Ptr=FlagPivot.Get())Ptr->SetRelativeRotation(FRotator(-90.f*FlagK,0,0));
}
void FS47MailBoxModel::Show(bool bOn)
{
 bShown=bOn;
 if(USceneComponent* Base0=Root.Get())Base0->SetVisibility(bOn,true);
 for(const auto& Ptr:ParcelParts)if(UPrimitiveComponent* Comp=Ptr.Get())Comp->SetHiddenInGame(!(bParcel&&bOn));
 if(UPrimitiveComponent* Comp=PickPart.Get()){Comp->SetCollisionEnabled(bOn?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);Comp->SetHiddenInGame(true);}
}
FVector FS47MailBoxModel::Front() const{return FVector(PostX-1.25,Ground,PostZ+.15);}
FVector FS47MailBoxModel::Lid() const{return FVector(PostX-.2,Ground+1.32,PostZ);}

// ---------- the carrier's car ----------
bool FS47MailCarModel::Built() const{return Root.IsValid()&&For.IsValid();}
bool FS47MailCarModel::Build(AS47Web* Web)
{
 namespace L=S47MailBoxLocal;
 if(Built()&&For.Get()==Web)return true;
 if(!IsValid(Web)||!Web->GetRootComponent())return false;
 UStaticMesh* CubeMesh=LoadObject<UStaticMesh>(nullptr,L::CubePath);UStaticMesh* DrumMesh=LoadObject<UStaticMesh>(nullptr,L::DrumPath);
 UMaterialInterface* Lit=LoadObject<UMaterialInterface>(nullptr,L::LitPath);
 if(!CubeMesh||!DrumMesh||!Lit)return false;
 // White paint with the road on its lower half, dark glass, tires, a gray bumper, and the roof light.
 UMaterialInterface* Paint=L::Tone(Web,Lit,0xe4e2da,.5f,.05f),*Lower=L::Tone(Web,Lit,0xc9c2b2,.75f,.03f),*Glass=L::Tone(Web,Lit,0x1a1e22,.12f,.2f);
 UMaterialInterface* Tire=L::Tone(Web,Lit,0x1c1c1c,.92f,0.f),*Rim=L::Tone(Web,Lit,0x8e9090,.5f,.6f),*Trim=L::Tone(Web,Lit,0x4f5254,.6f,.4f);
 UMaterialInterface* Lamp=L::Tone(Web,Lit,0xd9d6c8,.3f,.1f),*Tail=L::Tone(Web,Lit,0x7a1410,.4f,.05f);
 UMaterialInstanceDynamic* Light=L::Tone(Web,Lit,L::Amber,.3f,.05f);
 if(Light)Light->SetVectorParameterValue(TEXT("Emissive"),S47Three::Hex(0xff8a1c));
 For=Web;BeaconMat=Light;
 auto* Base0=NewObject<USceneComponent>(Web);Base0->SetMobility(EComponentMobility::Movable);Base0->SetupAttachment(Web->GetRootComponent());Base0->RegisterComponent();Root=Base0;
 // Metres in its own three-space: x to its right, y up, z toward the back; the nose at z -1.85.
 const auto Box=[&](UMaterialInterface* Mat,double X,double Y,double Z,double W,double H,double D)
 {
  L::Part(Web,Base0,CubeMesh,Mat,FTransform(FRotator::ZeroRotator,S47Three::ToUE(X,Y,Z),S47Three::DirToUE(FVector(W,H,D))));
 };
 const auto Drum=[&](UMaterialInterface* Mat,const FRotator& Turn,double X,double Y,double Z,double Dia,double Len)
 {
  L::Part(Web,Base0,DrumMesh,Mat,FTransform(Turn,S47Three::ToUE(X,Y,Z),FVector(Dia,Dia,Len)));
 };
 const FRotator Across(90,0,0),Lengthwise(0,0,90),Upright(0,0,0);
 for(const double Sx:{-1.,1.})for(const double Wz:{-1.22,1.22})
 {
  Drum(Tire,Across,Sx*.72,.32,Wz,.64,.2);
  Drum(Rim,Across,Sx*.83,.32,Wz,.3,.02);
 }
 Box(Lower,0,.56,0,1.62,.18,3.6);            // the sills and the road dust on them
 Box(Paint,0,.8,0,1.62,.32,3.7);             // the body
 Box(Paint,0,1.08,-1.4,1.54,.24,.9);         // the short hood
 Box(Paint,0,1.43,.45,1.62,.94,2.8);         // the square cab and the box behind it
 Box(Glass,0,1.5,-.955,1.38,.52,.02);        // the windshield
 for(const double Sx:{-1.,1.})Box(Glass,Sx*.815,1.52,-.55,.02,.46,.7);   // the side windows; the right one is the carrier's
 Box(Glass,0,1.55,1.855,1.2,.42,.02);        // the back doors' window
 Box(Trim,0,.82,-1.855,1.1,.2,.02);          // a plain grille
 for(const double Sx:{-1.,1.})
 {
  Drum(Lamp,Lengthwise,Sx*.58,.84,-1.86,.17,.03);
  Box(Tail,Sx*.7,.9,1.855,.12,.2,.02);
  Box(Trim,Sx*.9,1.38,-.95,.05,.16,.1);      // the mirrors
 }
 Box(Trim,0,.45,-1.9,1.66,.14,.1);           // the bumpers
 Box(Trim,0,.45,1.9,1.66,.14,.1);
 Box(Trim,0,1.91,-.5,.22,.02,.22);           // the light's base
 Drum(Light?static_cast<UMaterialInterface*>(Light):Lamp,Upright,0,1.98,-.5,.14,.12);
 Show(false);
 UE_LOG(LogTemp,Display,TEXT("S47: mail: the carrier's car is built"));
 return true;
}
void FS47MailCarModel::Place(const FVector& At,double Pitch)
{
 USceneComponent* Base0=Root.Get();if(!Base0)return;
 double Mat[16];S47Three::Compose(At,S47Three::Euler(Pitch,0,0,TEXT("YXZ")),FVector::OneVector,Mat);
 Base0->SetWorldTransform(S47Three::MatToUE(Mat));
}
void FS47MailCarModel::Show(bool bOn)
{
 bShown=bOn;
 if(USceneComponent* Base0=Root.Get())Base0->SetVisibility(bOn,true);
 if(!bOn)Beacon(false);
}
void FS47MailCarModel::Beacon(bool bOn)
{
 if(bBeacon==bOn)return;
 bBeacon=bOn;
 if(UMaterialInstanceDynamic* Mat=BeaconMat.Get())Mat->SetScalarParameterValue(TEXT("EmissivePower"),bOn?6.f:0.f);
}

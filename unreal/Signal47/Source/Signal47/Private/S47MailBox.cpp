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
 // Galvanized steel a few summers old, a little darker on top where the sun and the dust sit, the door brighter (they get replaced),
 // the pressed beads catching the light; rust brown, soil, paint red gone a little toward brick, and the reflector's amber.
 constexpr uint32 Galvanized=0x9ea19d,Weathered=0x8c908c,DoorTone=0xaaada8,Bead=0xb4b7b2,Rust=0x7a4a2c,Soil=0x5e5446,FlagRed=0xbf2a1f,Bolt=0x55585a,Amber=0xc0561f,Plank=0x6b5d4c;
 bool Exists(const TCHAR* Path){return FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(Path)));}
 UMaterialInterface* Tone(AS47Web* Web,UMaterialInterface* ParentMat,uint32 Rgb,float Rough,float Metal)
 {
  if(!ParentMat)return nullptr;
  auto* Mat=UMaterialInstanceDynamic::Create(ParentMat,Web);
  Mat->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(Rgb));Mat->SetScalarParameterValue(TEXT("Roughness"),Rough);Mat->SetScalarParameterValue(TEXT("Metalness"),Metal);
  Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);Mat->SetScalarParameterValue(TEXT("EmissivePower"),0);
  return Mat;
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
 UMaterialInterface* Steel=L::Tone(Web,Lit,L::Galvanized,.46f,.55f),*TopMat=L::Tone(Web,Lit,L::Weathered,.55f,.5f),*Door=L::Tone(Web,Lit,L::DoorTone,.42f,.55f),*Beads=L::Tone(Web,Lit,L::Bead,.38f,.6f);
 UMaterialInterface* RustMat=L::Tone(Web,Lit,L::Rust,.9f,0.f),*SoilMat=L::Tone(Web,Lit,L::Soil,.95f,0.f),*Red=L::Tone(Web,Lit,L::FlagRed,.5f,.12f),*BoltMat=L::Tone(Web,Lit,L::Bolt,.5f,.6f),*Reflector=L::Tone(Web,Lit,L::Amber,.3f,.1f);
 For=Web;Parts.Reset();
 auto* Base0=NewObject<USceneComponent>(Web);Base0->SetMobility(EComponentMobility::Movable);Base0->SetupAttachment(Under->Comp.Get());Base0->RegisterComponent();Root=Base0;
 auto* Pivot=NewObject<USceneComponent>(Web);Pivot->SetMobility(EComponentMobility::Movable);Pivot->SetupAttachment(Base0);Pivot->RegisterComponent();FlagPivot=Pivot;
 // Centimetres under the post's foot: x toward the box's back (east), y south, z up. Rotations: pitch 90 lays a cylinder along x,
 // roll 90 along y.
 Pivot->SetRelativeLocation(FVector(6.,8.75,116.5));
 const auto Piece=[&](UStaticMesh* Mesh,UMaterialInterface* Mat,const FVector& At,const FRotator& Turn,const FVector& SizeCm,USceneComponent* Onto)
 {
  auto* Comp=NewObject<UStaticMeshComponent>(Web);Comp->SetMobility(EComponentMobility::Movable);Comp->SetupAttachment(Onto);Comp->SetStaticMesh(Mesh);
  if(Mat)Comp->SetMaterial(0,Mat);
  Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);Comp->SetCastShadow(true);Comp->bCastContactShadow=false;Comp->SetReceivesDecals(false);
  Comp->SetRelativeTransform(FTransform(Turn,At,SizeCm/100.));Comp->RegisterComponent();Parts.Add(Comp);
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
 // Rust: along the bottom seams, and in a run under the latch.
 for(const double Side:{-1.,1.})Piece(CubeMesh,RustMat,FVector(-15,Side*8.33,108.7),Flat,FVector(40,.15,1.4),Base0);
 Piece(CubeMesh,RustMat,FVector(-38.75,1.5,124.),Flat,FVector(.15,3.5,5.),Base0);
 Piece(CubeMesh,RustMat,FVector(9.75,-3.,110.),Flat,FVector(.15,5.,2.),Base0);
 // A reflector on the post, toward the highway.
 Piece(DrumMesh,Reflector,FVector(-4.75,0,95.),AlongX,FVector(7.5,7.5,.6),Base0);
 // The flag: its pivot bolt on the side, the arm and the plate, all one red.
 Piece(DrumMesh,BoltMat,FVector(6.,8.85,116.5),AlongY,FVector(1.6,1.6,1.3),Base0);
 // A big flag, as the ones that went on boxes far from the house: up, it shows over the box at the yard's distance.
 Piece(CubeMesh,Red,FVector(-11,.15,0),Flat,FVector(22,.3,1.6),Pivot);
 Piece(CubeMesh,Red,FVector(-21,.2,0),Flat,FVector(16,.35,11),Pivot);
 // SARO and the box number stenciled on the side toward the station; on the flag's side only the number, under the flag.
 struct FStencil{const TCHAR* Words;double Side,Height;float Size;};
 for(const FStencil& Mark:{FStencil{TEXT("SARO"),-1.,115.5,4.6f},FStencil{TEXT("BOX 19"),-1.,110.4,2.3f},FStencil{TEXT("BOX 19"),1.,109.6,2.1f}})
 {
  auto* Stencil=NewObject<UTextRenderComponent>(Web);Stencil->SetMobility(EComponentMobility::Movable);Stencil->SetupAttachment(Base0);
  Stencil->SetText(FText::FromString(Mark.Words));Stencil->SetWorldSize(Mark.Size);
  Stencil->SetHorizontalAlignment(EHTA_Center);Stencil->SetVerticalAlignment(EVRTA_TextCenter);Stencil->SetTextRenderColor(FColor(36,34,31));
  Stencil->SetCollisionEnabled(ECollisionEnabled::NoCollision);Stencil->SetCastShadow(false);Stencil->RegisterComponent();
  Stencil->SetRelativeLocationAndRotation(FVector(-14,Mark.Side*8.36,Mark.Height),FRotator(0,Mark.Side*90,0));
  Parts.Add(Stencil);
 }
 // What the eye's ray finds: the box and the top of its post, nobody sees it.
 {
  auto* Comp=NewObject<UStaticMeshComponent>(Web);Comp->SetMobility(EComponentMobility::Movable);Comp->SetupAttachment(Base0);Comp->SetStaticMesh(CubeMesh);
  Comp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Comp->SetCollisionResponseToAllChannels(ECR_Ignore);Comp->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Comp->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
  Comp->SetRelativeTransform(FTransform(FRotator::ZeroRotator,FVector(-15,0,114),FVector(.58,.3,.44)));Comp->SetHiddenInGame(true);Comp->SetCastShadow(false);Comp->RegisterComponent();
  PickPart=Comp;
  Place();
  Pick=Web->ObjAt(Web->AdoptNode(Comp,Parent,TEXT("ff:mailbox")));
 }
 Place();
 UE_LOG(LogTemp,Display,TEXT("S47: mail: the box at the road, %d pieces, at %.2f %.2f"),Parts.Num(),PostX,PostZ);
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
 if(!Wld->LineTraceSingleByChannel(Hit,S47Three::ToUE(FVector(PostX,DesertY+25,PostZ)),S47Three::ToUE(FVector(PostX,DesertY-25,PostZ)),ECC_Visibility,Q))return;
 Ground=S47Three::ToThree(Hit.ImpactPoint).Y;bSeated=true;Place();
 UE_LOG(LogTemp,Display,TEXT("S47: mail: the box's post stands on the ground at %.2f"),Ground);
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
 if(USceneComponent* Base0=Root.Get())Base0->SetVisibility(bOn,true);
 if(UPrimitiveComponent* Comp=PickPart.Get()){Comp->SetCollisionEnabled(bOn?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);Comp->SetHiddenInGame(true);}
}
FVector FS47MailBoxModel::Front() const{return FVector(PostX-1.25,Ground,PostZ+.15);}
FVector FS47MailBoxModel::Lid() const{return FVector(PostX-.2,Ground+1.32,PostZ);}

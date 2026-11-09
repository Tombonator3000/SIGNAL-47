#include "S47EyeGlint.h"
#include "Components/SkinnedMeshComponent.h"
#include "Engine/SkinnedAsset.h"
#include "Components/LightComponentBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace S47EyeGlint
{
 bool bTestAlways=false;
 bool bTestStill=false;
}

namespace S47EyeGlint::Detail
{
 TAutoConsoleVariable<int32> CVarEyeGlint(TEXT("s47.EyeGlint"),1,TEXT("The faint dot in the aliens' eyes: 1 on, 0 off (S47EyeGlint)."));

 // Measured with Tools/triponpcs/measure_eyes.py on the prepared .blend of each body, 9 November: the centre of the black as seen
 // from in front, 1.5 mm out from the surface there. FaceYaw: the old Pale's head looks 40 degrees to its right at rest.
 struct FBody { const TCHAR* Asset; S47EyeGlint::FEyes Eyes; double FaceYawDeg; };
 const FBody Bodies[]={
  {TEXT("SK_Tripo_PaleOne"),{FVector(4.69,12.96,.43),FVector(-5.88,12.63,.21)},0.},
  {TEXT("SK_Tripo_TallOne"),{FVector(4.57,12.48,4.62),FVector(-6.15,12.22,4.45)},0.},
  {TEXT("SK_Tripo_Roswell2"),{FVector(4.80,10.96,3.41),FVector(-5.16,10.96,3.26)},0.},
  {TEXT("SK_Tripo_Roswell"),{FVector(5.02,10.96,3.51),FVector(-5.32,10.91,3.33)},0.},
  {TEXT("SK_Tripo_Pale"),{FVector(-.40,15.95,-.19),FVector(-10.77,10.44,3.72)},-40.},
 };

 // Rest of a dot in its bone's frame, the bone-frame directions of the body's sideways, up and the face's forward, and a seed
 // shared by both eyes of one body, so that they drift and blink together.
 struct FRest { FVector Base,Side,Up,Fwd; uint32 Seed=0; };
 TArray<TWeakObjectPtr<UStaticMeshComponent>> All;
 TMap<TWeakObjectPtr<UStaticMeshComponent>,FRest> Rests;
 uint32 NextSeed=1;

 // The smallest dot (scale of the engine's 1 m sphere: 4.2 mm across), and how it grows with distance so that it holds about a
 // pixel on screen instead of vanishing under one past a few metres. Glow: the emissive at full level.
 constexpr double MinSize=.0042,PerMetre=.0018,Glow=3.;

 // A repeatable 0..1 number from a seed and a slot.
 double Rand01(uint32 Seed,int64 Slot,uint32 Salt){return double((HashCombineFast(HashCombineFast(Seed,GetTypeHash(Slot)),Salt*2654435761u)>>8)&0xFFFFu)/65536.;}

 // The blink of one body at a time: about one in eight seconds, at a random moment, the dots go out over .12 s, stay out .1 s and
 // come back over .18 s. 1 open, 0 shut.
 double Blink(uint32 Seed,double T)
 {
  constexpr double Gap=8.;
  const int64 Slot=int64(FMath::FloorToDouble(T/Gap));
  if(Rand01(Seed,Slot,4)>.75)return 1.;
  const double Dt=FMath::Fmod(T,Gap)-Gap*(.1+.75*Rand01(Seed,Slot,5));
  if(Dt<0||Dt>=.4)return 1.;
  return Dt<.12?1.-Dt/.12:Dt<.22?0.:(Dt-.22)/.18;
 }

 // The slow drift of the gaze: two slow waves on each axis, a few seconds long, as the eyes seem to move under the dot.
 FVector2D Drift(uint32 Seed,double T)
 {
  const double P1=6.283*Rand01(Seed,0,6),P2=6.283*Rand01(Seed,0,7),P3=6.283*Rand01(Seed,0,8),P4=6.283*Rand01(Seed,0,9);
  return FVector2D(.65*FMath::Sin(T*.31+P1)+.35*FMath::Sin(T*.73+P2),.6*FMath::Sin(T*.23+P3)+.4*FMath::Sin(T*.61+P4));
 }
}

// The helpers above belong to this namespace, and stay in it in a unity build.
namespace S47EyeGlint { using namespace Detail; }

const S47EyeGlint::FEyes* S47EyeGlint::For(const FString& AssetName)
{
 for(const FBody& B:Bodies)if(AssetName==B.Asset)return &B.Eyes;
 return nullptr;
}

bool S47EyeGlint::Enabled()
{
 static const bool bOff=FParse::Param(FCommandLine::Get(),TEXT("S47EyeGlintOff"));
 return !bOff&&CVarEyeGlint.GetValueOnGameThread()!=0;
}

double S47EyeGlint::Level(double Daylight,double Metres,double Facing)
{
 if(!FMath::IsFinite(Daylight)||!FMath::IsFinite(Metres)||!FMath::IsFinite(Facing)||Metres>=GoneAt)return 0.;
 const double Dark=1.-FMath::SmoothStep(.02,.25,Daylight);
 const double Near=1.-FMath::SmoothStep(FadeFrom,GoneAt,Metres);
 // A face turned away shows no eye: full from about 70 degrees off the line, none past 100.
 const double Turned=FMath::SmoothStep(-.15,.35,Facing);
 return FMath::Clamp(Dark*Near*Turned,0.,1.);
}

namespace S47EyeGlint::Detail
{
 // Static bodies baked in a pose (S47TripoNew, 9 November): the dot in the mesh's own centimetres from its foot pivot, forward +Y.
 // Measured with Tools/triponpcs/measure_eyes.py on each <name>_eyes.blend (Tools/triponew/prepare.py), --dark .12, 9 November;
 // dot_world as (100x, -100y, 100z), the picture checked by eye. Only Zet has the big black eyes: the Nordic's are a man's, blue,
 // and the Cosmic one has no eyes at all, a glowing head; neither gets a dot until Tom says otherwise.
 struct FStaticBody { const TCHAR* Asset; S47EyeGlint::FEyes Eyes; };
 const FStaticBody StaticBodies[]={
  {TEXT("SM_Tripo_Zet"),{FVector(7.04,12.27,121.77),FVector(-6.03,12.19,121.61)}},
 };

 // Two dots under Parent (at Socket): Head is the frame the eyes are given in, in Parent's own space; FaceFwd the face's forward in it.
 void MakeDots(UObject* Outer,USceneComponent* Parent,FName Socket,const FTransform& Head,const FVector& FaceFwd,const S47EyeGlint::FEyes& Eyes,
  TArray<TWeakObjectPtr<UStaticMeshComponent>>& Out);
}

const S47EyeGlint::FEyes* S47EyeGlint::ForStatic(const FString& AssetName)
{
 for(const FStaticBody& B:StaticBodies)if(AssetName==B.Asset)return &B.Eyes;
 return nullptr;
}

void S47EyeGlint::AddStatic(UObject* Outer,UStaticMeshComponent* Mesh,const FEyes& Eyes,TArray<TWeakObjectPtr<UStaticMeshComponent>>& Out)
{
 if(!Outer||!Mesh)return;
 MakeDots(Outer,Mesh,NAME_None,FTransform::Identity,FVector(0,1,0),Eyes,Out);
}

bool S47EyeGlint::AddStaticIfKnown(UObject* Outer,UStaticMeshComponent* Mesh,TArray<TWeakObjectPtr<UStaticMeshComponent>>& Out)
{
 const UStaticMesh* Asset=Mesh?Mesh->GetStaticMesh():nullptr;
 const FEyes* Eyes=Asset?ForStatic(Asset->GetName()):nullptr;
 if(!Eyes)return false;
 AddStatic(Outer,Mesh,*Eyes,Out);
 return true;
}

void S47EyeGlint::Add(UObject* Outer,USkinnedMeshComponent* Mesh,const FEyes& Eyes,TArray<TWeakObjectPtr<UStaticMeshComponent>>& Out,FName Bone)
{
 if(!Outer||!Mesh)return;
 const int32 BoneIndex=Mesh->GetBoneIndex(Bone);
 if(BoneIndex==INDEX_NONE)return;
 double FaceYaw=0.;
 if(const USkinnedAsset* Asset=Mesh->GetSkinnedAsset())
  for(const FBody& B:Bodies)if(Asset->GetName()==B.Asset)FaceYaw=FMath::DegreesToRadians(B.FaceYawDeg);
 // The bone at its rest pose in component space: the body is built in it, before any pose.
 const FTransform Head=Mesh->GetBoneTransform(BoneIndex,FTransform::Identity);
 const FVector FaceFwd(FMath::Sin(FaceYaw),FMath::Cos(FaceYaw),0.);
 MakeDots(Outer,Mesh,Bone,Head,FaceFwd,Eyes,Out);
}

void S47EyeGlint::Detail::MakeDots(UObject* Outer,USceneComponent* Mesh,FName Bone,const FTransform& Head,const FVector& FaceFwd,const S47EyeGlint::FEyes& Eyes,
 TArray<TWeakObjectPtr<UStaticMeshComponent>>& Out)
{
 auto* Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 if(!Sphere||!Base)return;
 const uint32 Seed=NextSeed++*7919u;
 for(int32 I=0;I<2;++I)
 {
  auto* Mat=UMaterialInstanceDynamic::Create(Base,Outer);
  Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor::Black);Mat->SetVectorParameterValue(TEXT("Emissive"),FLinearColor::Black);
  Mat->SetScalarParameterValue(TEXT("EmissivePower"),1);
  auto* G=NewObject<UStaticMeshComponent>(Outer,MakeUniqueObjectName(Outer,UStaticMeshComponent::StaticClass(),FName(*FString::Printf(TEXT("%sGlint%d"),*Mesh->GetName(),I))));
  G->SetMobility(EComponentMobility::Movable);G->SetupAttachment(Mesh,Bone);G->SetStaticMesh(Sphere);G->SetMaterial(0,Mat);
  G->SetCollisionEnabled(ECollisionEnabled::NoCollision);G->SetGenerateOverlapEvents(false);G->SetCanEverAffectNavigation(false);
  // A point seen, not a lamp: no shadow, no light given to what is round it, not in reflections or the sky's capture.
  G->SetCastShadow(false);G->bAffectDynamicIndirectLighting=false;G->bAffectDistanceFieldLighting=false;G->bAffectIndirectLightingWhileHidden=false;
  G->bVisibleInReflectionCaptures=false;G->bVisibleInRealTimeSkyCaptures=false;G->bReceivesDecals=false;G->SetVisibleInRayTracing(false);
  G->SetVisibility(false);G->SetHiddenInGame(true);
  G->RegisterComponent();
  G->SetRelativeLocation(Head.InverseTransformPosition(Head.GetLocation()+(I?Eyes.Right:Eyes.Left)));
  G->SetWorldScale3D(FVector(MinSize));
  Out.Add(G);All.Add(G);
  FRest R;R.Base=G->GetRelativeLocation();R.Side=Head.InverseTransformVectorNoScale(FVector(1,0,0));R.Up=Head.InverseTransformVectorNoScale(FVector(0,0,1));
  R.Fwd=Head.InverseTransformVectorNoScale(FaceFwd);R.Seed=Seed;
  Rests.Add(G,R);
 }
}

void S47EyeGlint::Step(const FVector& EyeUE,double Daylight)
{
 All.RemoveAll([](const TWeakObjectPtr<UStaticMeshComponent>& G){return !G.IsValid();});
 for(auto It=Rests.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
 const bool bOn=Enabled();
 for(const auto& Weak:All)if(UStaticMeshComponent* G=Weak.Get())
 {
  const FRest* R=Rests.Find(G);
  USceneComponent* Parent=G->GetAttachParent();
  // The body hidden (the dot inherits its visible flag), or no rest to place it by: nothing to draw. Not IsVisible(): in a game world
  // it is false while hidden in game, and a dot hidden for the dark would never come back.
  if(!R||!Parent||!G->GetVisibleFlag()||!bOn){if(!G->bHiddenInGame)G->SetHiddenInGame(true);continue;}
  const FTransform Bone=Parent->GetSocketTransform(G->GetAttachSocketName());
  const FVector At=Bone.TransformPosition(R->Base);
  const double Metres=FVector::Dist(EyeUE,At)/100.;
  const FVector Face=Bone.TransformVectorNoScale(R->Fwd).GetSafeNormal();
  const double Facing=FVector::DotProduct(Face,(EyeUE-At).GetSafeNormal());
  const double T=G->GetWorld()?G->GetWorld()->GetTimeSeconds():0.;
  double Lit=bTestAlways?(Metres<GoneAt?1.:0.):S47EyeGlint::Level(Daylight,Metres,Facing);
  if(!bTestStill)Lit*=Blink(R->Seed,T);
  const bool bShow=Lit>.01;
  if(G->bHiddenInGame==bShow)G->SetHiddenInGame(!bShow);
  if(!bShow)continue;
  // The drift is a few millimetres inside the eye, so it is only worth moving within 15 m.
  FVector Place=R->Base;
  if(!bTestStill&&Metres<15.)
  {
   const FVector2D D=Drift(R->Seed,T)*.4*(1.-FMath::SmoothStep(10.,15.,Metres));
   Place+=R->Side*D.X+R->Up*D.Y;
  }
  if(!Place.Equals(G->GetRelativeLocation(),KINDA_SMALL_NUMBER))G->SetRelativeLocation(Place);
  G->SetWorldScale3D(FVector(FMath::Max(MinSize,Metres*PerMetre)));
  if(auto* Mat=Cast<UMaterialInstanceDynamic>(G->GetMaterial(0)))Mat->SetVectorParameterValue(TEXT("Emissive"),FLinearColor(1,1,1)*float(Glow*Lit));
 }
}

int32 S47EyeGlint::Shown()
{
 int32 N=0;
 for(const auto& Weak:All)if(const UStaticMeshComponent* G=Weak.Get())if(G->GetVisibleFlag()&&!G->bHiddenInGame)++N;
 return N;
}

bool S47EyeGlint::GivesNoLight(FString* Why)
{
 for(const auto& Weak:All)if(const UStaticMeshComponent* G=Weak.Get())
 {
  const TCHAR* Bad=G->CastShadow?TEXT("casts a shadow"):G->bAffectDynamicIndirectLighting?TEXT("lights what is round it"):
   G->bAffectDistanceFieldLighting?TEXT("is in the distance-field light"):G->bVisibleInReflectionCaptures?TEXT("is in reflections"):nullptr;
  if(!Bad)for(const USceneComponent* Child:G->GetAttachChildren())if(Child&&Child->IsA<ULightComponentBase>())Bad=TEXT("has a light under it");
  if(Bad){if(Why)*Why=FString::Printf(TEXT("%s %s"),*G->GetName(),Bad);return false;}
 }
 return true;
}

int32 S47EyeGlint::Count()
{
 int32 N=0;
 for(const auto& Weak:All)if(Weak.IsValid())++N;
 return N;
}

bool S47EyeGlint::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;auto Check=[&](bool Pass,const TCHAR* Text){Lines.Add(FString(Pass?TEXT("PASS "):TEXT("FAIL "))+Text);OK&=Pass;};
 Check(Level(0,5)==1.&&Level(0,15)==1.&&Level(0,30)==1.,TEXT("eye glint: full in the dark to 30 m"));
 Check(Level(0,40)>0.&&Level(0,40)<Level(0,35)&&Level(0,49.9)<.01,TEXT("eye glint: fades from 30 m and is all but gone at 50 m"));
 Check(Level(0,50)==0.&&Level(0,51)==0.&&Level(0,400)==0.,TEXT("eye glint: nothing past 50 m (Tom: it shone for hundreds of metres)"));
 Check(Level(1,5)==0.&&Level(.6,5)==0.&&Level(.25,5)==0.,TEXT("eye glint: nothing by day or in a bright dusk"));
 Check(Level(.1,5)>0.&&Level(.1,5)<1.,TEXT("eye glint: comes in gradually as the light goes"));
 Check(Level(0,5,-1.)==0.&&Level(0,5,-.2)==0.&&Level(0,5,.1)<Level(0,5,.5),TEXT("eye glint: a face turned away shows none"));
 Check(Level(0,FMath::Sqrt(-1.))==0.&&Level(FMath::Sqrt(-1.),5)==0.,TEXT("eye glint: a broken distance or sky gives none"));
 bool bAll=true,bSides=true;
 for(const TCHAR* Name:{TEXT("SK_Tripo_PaleOne"),TEXT("SK_Tripo_TallOne"),TEXT("SK_Tripo_Roswell2"),TEXT("SK_Tripo_Roswell"),TEXT("SK_Tripo_Pale")})
 {
  const FEyes* E=For(Name);
  if(!E){bAll=false;continue;}
  bSides&=E->Left.X>E->Right.X+5.&&E->Left.Size()<20.&&E->Right.Size()<20.&&E->Left.Y>8.&&E->Right.Y>8.;
 }
 Check(bAll,TEXT("eye glint: every alien body has its eyes measured (Pale One, Tall One, Roswell 2, and the old Roswell and Pale)"));
 Check(bSides,TEXT("eye glint: each body's two dots on either side of its face, ahead of the head bone, within its head"));
 Check(!For(TEXT("SK_Tripo_Hereford"))&&!For(FString()),TEXT("eye glint: no dots on a body that is not measured"));
 double Shut=1.;for(double T=0;T<400;T+=.02)Shut=FMath::Min(Shut,Blink(7919u,T));
 int32 Blinks=0;bool bWas=true;for(double T=0;T<400;T+=.02){const bool bOpen=Blink(7919u,T)>.5;if(!bOpen&&bWas)++Blinks;bWas=bOpen;}
 Check(Shut==0.&&Blinks>=25&&Blinks<=50,TEXT("eye glint: blinks shut now and then, about once in ten seconds"));
 double Most=0;for(double T=0;T<200;T+=.05)Most=FMath::Max(Most,Drift(7919u,T).GetAbsMax());
 Check(Most>.5&&Most<=1.0001,TEXT("eye glint: the drift stays inside its reach"));
 return OK;
}

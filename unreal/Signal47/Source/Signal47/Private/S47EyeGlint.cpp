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
 // The head (Tom, 9 November: "the dot goes outside the body"): the box of the head's own vertices round the head bone at rest,
 // in the face's frame (the face's yaw undone), and the middle of its width at eye height, measured on the same .blend files
 // (eyeglint2 head_box.py, 9 November). The self-test holds each body's two dots inside it and either side of that middle.
 struct FBody { const TCHAR* Asset; S47EyeGlint::FEyes Eyes; double FaceYawDeg; FVector HeadLo,HeadHi; double MidX; };
 const FBody Bodies[]={
  {TEXT("SK_Tripo_PaleOne"),{FVector(4.69,12.96,.43),FVector(-5.88,12.63,.21)},0.,FVector(-12.48,-27.42,-3.64),FVector(11.60,15.88,22.41),-.44},
  {TEXT("SK_Tripo_TallOne"),{FVector(4.57,12.48,4.62),FVector(-6.15,12.22,4.45)},0.,FVector(-12.27,-29.14,-3.96),FVector(11.31,15.74,23.45),-.48},
  {TEXT("SK_Tripo_Roswell2"),{FVector(4.80,10.96,3.41),FVector(-5.16,10.96,3.26)},0.,FVector(-11.15,-12.09,-1.91),FVector(10.69,12.56,18.52),-.23},
  {TEXT("SK_Tripo_Roswell"),{FVector(5.02,10.96,3.51),FVector(-5.32,10.91,3.33)},0.,FVector(-11.15,-12.09,-1.91),FVector(10.69,12.56,18.52),-.23},
  {TEXT("SK_Tripo_Pale"),{FVector(-.40,15.95,-.19),FVector(-10.77,10.44,3.72)},-40.,FVector(-8.34,-13.34,-3.57),FVector(17.22,17.58,25.05),4.40},
 };

 // Rest of a dot in its bone's frame, and the bone-frame vectors that are one centimetre of the body's sideways, up and the face's
 // forward in the component (the Tripo skins' root bone carries the file's unit, scale 100, and every bone under it with it: a
 // unit vector of the bone's frame is a metre, and the drift of 9 November threw the dots up to 40 cm out of the head). A seed
 // shared by both eyes of one body, so that they drift and blink together.
 struct FRest { FVector Base,Side,Up,Fwd; uint32 Seed=0; };
 // The dot's rest in the frame of Head (component space, any scale and turn) from the eye's offset in component centimetres.
 FRest MakeRest(const FTransform& Head,const FVector& FaceFwd,const FVector& Eye)
 {
  FRest R;
  R.Base=Head.InverseTransformPosition(Head.GetLocation()+Eye);
  const FVector Fwd=FaceFwd.GetSafeNormal2D();
  R.Side=Head.InverseTransformVector(FVector(Fwd.Y,-Fwd.X,0.));
  R.Up=Head.InverseTransformVector(FVector(0,0,1));
  R.Fwd=Head.InverseTransformVector(Fwd);
  return R;
 }
 // Drift: how far the dot wanders in the eye, component centimetres (the black of the smallest eye is about 3.5 cm across).
 constexpr double DriftCm=.4;
 // Where a dot is drawn, in the world, from the bone's world transform now (socket: its pose, the body's turn and scale):
 // its rest, the drift (DriftCm units, in the body's own centimetres, so it scales with the body) and Sink world centimetres
 // back along the face, so that a dot grown for distance sinks into the head by as much as it grew and never stands out of it.
 FVector PlaceWorld(const FTransform& Bone,const FRest& R,const FVector2D& Drift,double Sink)
 {
  const FVector Face=Bone.TransformVector(R.Fwd).GetSafeNormal();
  return Bone.TransformPosition(R.Base)+Bone.TransformVector(R.Side*Drift.X+R.Up*Drift.Y)-Face*Sink;
 }
 TArray<TWeakObjectPtr<UStaticMeshComponent>> All;
 TMap<TWeakObjectPtr<UStaticMeshComponent>,FRest> Rests;
 uint32 NextSeed=1;

 // The smallest dot (scale of the engine's 1 m sphere: 4.2 mm across), and how it grows with distance so that it holds about a
 // pixel on screen instead of vanishing under one past a few metres. Glow: the emissive at full level.
 constexpr double MinSize=.0042,PerMetre=.0018,Glow=3.;
 // The dot's size at a distance, and how far it sinks for it (world cm: the engine's sphere is 50 cm in radius at scale 1).
 double SizeAt(double Metres){return FMath::Max(MinSize,Metres*PerMetre);}
 double SinkAt(double Metres){return (SizeAt(Metres)-MinSize)*50.;}

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
 // The head's box and middle as the skinned bodies', in the mesh's centimetres (Zet: its one-bone head at (.94, .81, 115.84)).
 struct FStaticBody { const TCHAR* Asset; S47EyeGlint::FEyes Eyes; FVector HeadLo,HeadHi; double MidX; };
 const FStaticBody StaticBodies[]={
  {TEXT("SM_Tripo_Zet"),{FVector(7.04,12.27,121.77),FVector(-6.03,12.19,121.61)},FVector(-14.54,-13.03,113.54),FVector(15.44,18.10,140.00),.45},
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
  // Its visible flag starts as its body's: a body hidden now shows its dots with itself (SetVisibility(true,true): S47Happenings,
  // S47TripoCreature), and one already shown (F10 "Tripo new" draws Zet shown) has them at once. Step's hidden-in-game is the dark's.
  // (Sweep of 9 November: always false here, Zet's dots stayed off on a body that was never shown again with its children.)
  G->SetVisibility(Mesh->GetVisibleFlag());G->SetHiddenInGame(true);
  G->RegisterComponent();
  FRest R=MakeRest(Head,FaceFwd,I?Eyes.Right:Eyes.Left);R.Seed=Seed;
  G->SetRelativeLocation(R.Base);
  G->SetWorldScale3D(FVector(MinSize));
  Out.Add(G);All.Add(G);
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
  // The face's forward through the bone's own turn and scale (a body stretched unevenly still faces where its face does).
  const FVector Face=Bone.TransformVector(R->Fwd).GetSafeNormal();
  const double Facing=FVector::DotProduct(Face,(EyeUE-At).GetSafeNormal());
  const double T=G->GetWorld()?G->GetWorld()->GetTimeSeconds():0.;
  // Held on for a test or a look, a face turned away still hides its dots (Tom, 9 November).
  double Lit=bTestAlways?(Metres<GoneAt?FMath::SmoothStep(-.15,.35,Facing):0.):S47EyeGlint::Level(Daylight,Metres,Facing);
  if(!bTestStill)Lit*=Blink(R->Seed,T);
  const bool bShow=Lit>.01;
  if(G->bHiddenInGame==bShow)G->SetHiddenInGame(!bShow);
  if(!bShow)continue;
  // The drift is a few millimetres inside the eye, so it is only worth moving within 15 m; in the body's centimetres, not the
  // bone's units (a metre on the Tripo skins).
  FVector2D D=FVector2D::ZeroVector;
  if(!bTestStill&&Metres<15.)D=Drift(R->Seed,T)*DriftCm*(1.-FMath::SmoothStep(10.,15.,Metres));
  const FVector Place=Bone.InverseTransformPosition(PlaceWorld(Bone,*R,D,SinkAt(Metres)));
  if(!Place.Equals(G->GetRelativeLocation(),1e-5))G->SetRelativeLocation(Place);
  G->SetWorldScale3D(FVector(SizeAt(Metres)));
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
 // Each body's dots inside its head's box, in its front half, and either side of the head's middle by the same (within 1 cm),
 // in the face's frame: the old Pale's head looks 40 degrees aside at rest.
 const auto InHead=[](const S47EyeGlint::FEyes& E,double YawDeg,const FVector& Lo,const FVector& Hi,double MidX)
 {
  const double Yaw=FMath::DegreesToRadians(YawDeg),C=FMath::Cos(Yaw),S=FMath::Sin(Yaw);
  const auto Turn=[&](const FVector& P){return FVector(P.X*C-P.Y*S,P.X*S+P.Y*C,P.Z);};
  const FVector L=Turn(E.Left),R=Turn(E.Right);
  const FBox Box(Lo,Hi);
  const bool bInside=Box.IsInsideOrOn(L)&&Box.IsInsideOrOn(R)&&L.Y>(Lo.Y+Hi.Y)/2&&R.Y>(Lo.Y+Hi.Y)/2;
  const bool bEven=FMath::Abs((L.X+R.X)/2-MidX)<1.&&L.X-MidX>3.&&MidX-R.X>3.&&FMath::Abs(L.Z-R.Z)<5.;
  return bInside&&bEven;
 };
 bool bHeads=true;
 for(const FBody& B:Bodies)bHeads&=InHead(B.Eyes,B.FaceYawDeg,B.HeadLo,B.HeadHi,B.MidX);
 for(const FStaticBody& B:StaticBodies)bHeads&=InHead(B.Eyes,0.,B.HeadLo,B.HeadHi,B.MidX);
 Check(bHeads,TEXT("eye glint: every body's two dots inside its head's box, in front, either side of its middle alike (Pale One, Tall One, Roswell 2, old Roswell, old Pale, Zet)"));
 // Placed as Step places them, on a head bone with the Tripo skins' scale of 100 and a turn, under a body turned and grown:
 // at rest on the eye, the drift a few millimetres of the body's own and never out of the head, a dot grown for 50 m sunk so
 // that its front stays where the near dot's is. Before 9 November the drift was in bone units: 40 cm on these skins.
 bool bRest=true,bDrift=true,bSink=true,bUneven=true;
 const FTransform Body(FRotator(0,-90,0),FVector(120,-40,5),FVector(1.25));
 for(const FBody& B:Bodies)for(int32 Side=0;Side<2;++Side)
 {
  const FVector Eye=Side?B.Eyes.Right:B.Eyes.Left;
  const FVector Fwd(FMath::Sin(FMath::DegreesToRadians(B.FaceYawDeg)),FMath::Cos(FMath::DegreesToRadians(B.FaceYawDeg)),0.);
  for(const FVector& Scale:{FVector(100),FVector(100,80,125)})
  {
   const FTransform Head(FQuat(FVector(.3,-.5,.8).GetSafeNormal(),1.9),FVector(0,3.5,202.6),Scale);
   const FRest R=MakeRest(Head,Fwd,Eye);
   const FTransform Bone=Head*Body;
   const FVector Want=Body.TransformPosition(Head.GetLocation()+Eye);
   const FVector At=PlaceWorld(Bone,R,FVector2D::ZeroVector,0.);
   const bool bEven=Scale.X==Scale.Y;
   (bEven?bRest:bUneven)&=At.Equals(Want,.01);
   for(const FVector2D& D:{FVector2D(DriftCm,DriftCm),FVector2D(-DriftCm,DriftCm),FVector2D(DriftCm,-DriftCm)})
   {
    const FVector P=PlaceWorld(Bone,R,D,0.);
    const double Moved=FVector::Dist(P,At)/Body.GetScale3D().X;
    (bEven?bDrift:bUneven)&=Moved<DriftCm*1.4143&&Moved>DriftCm;
   }
   const FVector Face=Body.TransformVector(Fwd).GetSafeNormal();
   const FVector Far=PlaceWorld(Bone,R,FVector2D::ZeroVector,SinkAt(50.));
   const FVector Near=PlaceWorld(Bone,R,FVector2D::ZeroVector,SinkAt(2.));
   const double FrontFar=FVector::DotProduct(Far-Want,Face)+SizeAt(50.)*50.,FrontNear=FVector::DotProduct(Near-Want,Face)+SizeAt(2.)*50.;
   const FVector Inside=Body.InverseTransformPosition(Far)-Head.GetLocation();
   const double Yaw=FMath::DegreesToRadians(B.FaceYawDeg);
   const FVector InFace(Inside.X*FMath::Cos(Yaw)-Inside.Y*FMath::Sin(Yaw),Inside.X*FMath::Sin(Yaw)+Inside.Y*FMath::Cos(Yaw),Inside.Z);
   (bEven?bSink:bUneven)&=FMath::IsNearlyEqual(FrontFar,FrontNear,.01)&&FBox(B.HeadLo,B.HeadHi).IsInsideOrOn(InFace);
  }
 }
 Check(bRest,TEXT("eye glint: on a head bone of scale 100 under a turned, grown body, each dot rests on its measured eye"));
 Check(bDrift,TEXT("eye glint: the drift is 4 to 6 mm of the body's own, not the bone's units (it threw the dots 40 cm out of the head)"));
 Check(bSink,TEXT("eye glint: a dot grown for 50 m sinks into the head as it grows, its front where the near dot's is, its centre inside the head"));
 Check(bUneven,TEXT("eye glint: the same on a head bone scaled unevenly"));
 double Shut=1.;for(double T=0;T<400;T+=.02)Shut=FMath::Min(Shut,Blink(7919u,T));
 int32 Blinks=0;bool bWas=true;for(double T=0;T<400;T+=.02){const bool bOpen=Blink(7919u,T)>.5;if(!bOpen&&bWas)++Blinks;bWas=bOpen;}
 Check(Shut==0.&&Blinks>=25&&Blinks<=50,TEXT("eye glint: blinks shut now and then, about once in ten seconds"));
 double Most=0;for(double T=0;T<200;T+=.05)Most=FMath::Max(Most,Drift(7919u,T).GetAbsMax());
 Check(Most>.5&&Most<=1.0001,TEXT("eye glint: the drift stays inside its reach"));
 return OK;
}

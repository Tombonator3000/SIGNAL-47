// ENV-03 (Unreal only): the crossing's hanging flasher, an original detailed model that sways.
// Town.ts crossing() and FS47Town::Update still own the blink; this file only moves its places.
#include "S47TownSignal.h"
#include "S47Web.h"
#include "S47Three.h"
#include "S47Glow.h"
#include "S47Wind.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "StaticMeshResources.h"
DEFINE_LOG_CATEGORY_STATIC(LogS47TownSignal,Log,All);
namespace
{
 // ArtSource/TrafficSignal/model_contract.json, measured from the immutable web export.
 const FVector PIVOT(8000.5,11.459,2000.0);          // three world metres, the span clamp
 const FVector POOL(0,-1.05,0),GLOW(0,-.63,0);         // the old crossing's light and glows, pivot-relative
 // The old 1.1 m box (no shadow casting) held its light inside; the new 0.44 m section would
 // be lit from beneath at -1.05. The light rides at the lens centre, inside the casting, which
 // likewise casts no shadow: the road pool, glows and blink stay, the underside stays dark.
 const FVector LIGHT(0,-.63,0);
 const FVector WIRE(.4319906,0,-.9019583),ACROSS(.9019583,0,.4319906);
 const int32 HEAD_TRIS[3]={15716,8564,3360},SPAN_TRIS[3]={13172,6788,1448};
 const FVector HEAD_MIN=FVector(-0.469814,-0.469814,-0.931500),HEAD_MAX=FVector(0.469814,0.469814,0.014151);
 const FVector SPAN_MIN=FVector(-9.264600,-19.167604,-7.455625),SPAN_MAX=FVector(9.241585,19.167604,0.385000); // Blender local metres
 const FLinearColor AMBER(1.f,.72f,.28f);const float LENS_GAIN=2.5f;
 UStaticMesh* Load(const TCHAR* Name,const int32* Tris,const FVector& Lo,const FVector& Hi)
 {
  auto* M=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/TownSignal/%s.%s"),Name,Name));
  if(!M||!M->GetRenderData()||M->GetRenderData()->LODResources.Num()!=3)return nullptr;
  for(int32 L=0;L<3;++L)if(M->GetNumTriangles(L)!=Tris[L])return nullptr;
  // Blender (x,y,z) metres arrive as UE (100x,-100y,100z) centimetres.
  const FVector Center((Lo.X+Hi.X)*50,-(Lo.Y+Hi.Y)*50,(Lo.Z+Hi.Z)*50);
  if(!M->GetBounds().Origin.Equals(Center,.3)||!M->GetBounds().BoxExtent.Equals((Hi-Lo)*50,.3))return nullptr;
  for(const auto& Slot:M->GetStaticMaterials())if(!Slot.MaterialInterface||!Slot.MaterialInterface->GetName().StartsWith(TEXT("M_TownSignal_")))return nullptr;
  return M;
 }
 FVector Turn(const FQuat& Q,const FVector& V){return Q.RotateVector(V);}
}
FVector FS47TownSignal::Swing(double T,double Strength,const FVector2D& Dir)
{
 // A span-wire head is a stiff, heavily damped pendulum (about 2.7 s here, clamp to lenses
 // 0.63 m plus the wire's own give). The wind's gusts (world/wind.ts strength 0.05 to 0.08)
 // set the reach; a wind across the wire swings it, one along the wire barely moves it.
 const double S=FMath::Clamp(Strength/.07,0.,1.6),Across=FMath::Abs(Dir.X*ACROSS.X+Dir.Y*ACROSS.Z);
 const double Gust=.75+.25*FMath::Sin(UE_TWO_PI*T/11.+.4);
 const double Wire=S*(.55+.45*Across)*(2.0*Gust*(.72*FMath::Sin(UE_TWO_PI*T/2.7)+.28*FMath::Sin(UE_TWO_PI*T/4.3+1.1))+.6*Across);
 const double Along=S*.5*FMath::Sin(UE_TWO_PI*T/3.1+.7);
 const double Twist=S*1.3*FMath::Sin(UE_TWO_PI*T/6.4+2.);
 return FVector(Wire,Along,Twist);
}
FQuat FS47TownSignal::Rotation(const FVector& Deg)
{
 // Three's terms (right-handed, y up): twist about y, then the swing about the wire and along it.
 return FQuat(FVector(0,1,0),FMath::DegreesToRadians(Deg.Z))*FQuat(WIRE,FMath::DegreesToRadians(Deg.X))*FQuat(ACROSS,FMath::DegreesToRadians(Deg.Y));
}
TSharedPtr<FS47TownSignal> FS47TownSignal::Prepare(AS47Web* W)
{
 if(!W||FParse::Param(FCommandLine::Get(),TEXT("S47TownSignalOff")))return {};
 auto S=MakeShared<FS47TownSignal>();S->Web=W;
 auto No=[](const TCHAR* Why){UE_LOG(LogS47TownSignal,Error,TEXT("S47: crossing signal rejected: %s"),Why);return TSharedPtr<FS47TownSignal>();};
 S->HeadMesh=Load(TEXT("SM_Caliche_Signal_Head"),HEAD_TRIS,HEAD_MIN,HEAD_MAX);S->SpanMesh=Load(TEXT("SM_Caliche_Signal_Span"),SPAN_TRIS,SPAN_MIN,SPAN_MAX);
 if(!S->HeadMesh||!S->SpanMesh)return No(TEXT("native meshes/LODs/bounds/materials"));
 const int32 Slot=S->HeadMesh->GetMaterialIndex(TEXT("M_TownSignal_Lens"));FLinearColor E;
 if(Slot<0||!S->HeadMesh->GetMaterial(Slot)->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveFactor")),E))return No(TEXT("lens emissive slot"));
 S->Parent=W->Handle(TEXT("world.town.group"));const auto* P=W->Node(S->Parent);
 if(!P||!P->Comp.IsValid())return No(TEXT("town group"));
 S->PivotWorld=PIVOT;S->PivotLocal=W->NodeWorldThree(S->Parent).InverseTransformPosition(PIVOT);
 if(!S->PivotLocal.Equals(FVector(.5,11.459,2000),.001))return No(TEXT("town placement"));
 // Town.ts crossing(): light('blink') is flood slot 0, glows 0 and 1 are its halo.
 S->Flood=W->FloodSet(TEXT("town"));FVector At;float Width;FLinearColor C;
 if(!W->FloodPool(S->Flood,0,At,Width,C)||!At.Equals(PIVOT+POOL,.01))return No(TEXT("blink flood slot 0"));
 for(int32 I=0;I<2;++I)
 {
  S->GlowNodes[I]=W->Handle(FString::Printf(TEXT("world.town.runtime.glows[%d]"),I));FS47GlowPoints* G=W->Glow(S->GlowNodes[I]);
  if(!G||G->Num()!=1||!G->Pos[0].Equals(S->PivotLocal+GLOW,.01))return No(TEXT("blink glows 0/1"));
 }
 if(W->PathsUnder(TEXT("world.town.runtime.panels.blink.")).Num()!=4)return No(TEXT("four blink panels"));
 return S;
}
void FS47TownSignal::Commit(TArray<int32>& Retired)
{
 AS47Web* W=Web.Get();if(!W)return;USceneComponent* ParentComp=W->Node(Parent)->Comp.Get();
 auto Make=[&](UStaticMesh* M,const TCHAR* Name){
  auto* C=NewObject<UStaticMeshComponent>(W);C->SetupAttachment(ParentComp);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(M);
  // Seven metres over the highway: nothing walks or drives into it. The head holds the blink
  // light (see LIGHT) and, like the old box, casts no shadow; the span does.
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(M==SpanMesh);C->RegisterComponent();
  C->SetWorldLocation(S47Three::ToUE(PivotWorld));C->SetWorldRotation(FRotator::ZeroRotator);W->AdoptNode(C,Parent,FString(TEXT("town-signal:"))+Name);return C;};
 Span=Make(SpanMesh,TEXT("span"));UStaticMeshComponent* H=Make(HeadMesh,TEXT("head"));Head=H;
 const int32 Slot=HeadMesh->GetMaterialIndex(TEXT("M_TownSignal_Lens"));
 if(auto* Mid=H->CreateDynamicMaterialInstance(Slot)){Mid->SetVectorParameterValue(TEXT("EmissiveFactor"),FLinearColor::Black);Lens=Mid;}
 // The old atlas lens quads go; their panel state stays and is mirrored on the native lenses.
 for(const FString& Path:W->PathsUnder(TEXT("world.town.runtime.panels.blink.")))if(const auto* N=W->Node(W->Handle(Path)))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))
 {Retired.AddUnique(W->Handle(Path));C->SetVisibility(false,false);C->SetHiddenInGame(true,false);C->SetCastShadow(false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
 UE_LOG(LogS47TownSignal,Display,TEXT("S47: crossing signal ready: detailed head swinging about the span clamp at %.3f,%.3f,%.3f; blink slot 0 and glows 0/1 follow"),PivotWorld.X,PivotWorld.Y,PivotWorld.Z);
}
void FS47TownSignal::Update()
{
 AS47Web* W=Web.Get();UStaticMeshComponent* H=Head.Get();if(!W||!H)return;
 const S47Wind::FWind& Wd=S47Wind::Wind();const double T=W->GetWorld()?W->GetWorld()->GetTimeSeconds():0.;
 Angles=Swing(T,Wd.Strength,Wd.Dir);const FQuat Q=Rotation(Angles);
 if(!Q.Equals(Last,1e-6))
 {
  Last=Q;
  // The same turn in Unreal's axes (x, z, y): the images of Unreal's basis under it.
  const FVector X=S47Three::DirToUE(Turn(Q,FVector(1,0,0))),Y=S47Three::DirToUE(Turn(Q,FVector(0,0,1))),Z=S47Three::DirToUE(Turn(Q,FVector(0,1,0)));
  H->SetWorldLocationAndRotation(S47Three::ToUE(PivotWorld),FMatrix(X,Y,Z,FVector::ZeroVector).ToQuat());
  FVector At;float Width;FLinearColor C;
  if(W->FloodPool(Flood,0,At,Width,C))W->SetFloodPool(Flood,0,PivotWorld+Turn(Q,LIGHT),Width,C);
  for(int32 I=0;I<2;++I)if(FS47GlowPoints* G=W->Glow(GlowNodes[I]))G->SetPosition(0,PivotLocal+Turn(Q,GLOW));
 }
 // FS47Town's blink sets slot 0 to 5 (on) or 0 (off): the lenses follow it, the off state
 // as dim as the old panel's 0.06.
 const float Level=FMath::Clamp(W->FloodPos(Flood,0).A/5.f,0.f,1.f);
 if(Level!=LastLevel){LastLevel=Level;if(auto* Mid=Lens.Get())Mid->SetVectorParameterValue(TEXT("EmissiveFactor"),AMBER*(LENS_GAIN*(.04f+.96f*Level)));}
}
bool FS47TownSignal::Audit(TArray<FString>& Lines) const
{
 AS47Web* W=Web.Get();const UStaticMeshComponent* H=Head.Get();bool Ok=W&&H&&Span.IsValid()&&Lens.IsValid()&&H->GetStaticMesh()==HeadMesh&&Span->GetStaticMesh()==SpanMesh;
 double PoolErr=1e9,GlowErr=1e9,PivotErr=1e9;
 if(Ok)
 {
  const FQuat Q=Last;FVector At;float Width;FLinearColor C;
  if(W->FloodPool(Flood,0,At,Width,C))PoolErr=FVector::Dist(At,PivotWorld+Turn(Q,LIGHT));
  GlowErr=0;for(int32 I=0;I<2;++I){const FS47GlowPoints* G=W->Glow(GlowNodes[I]);GlowErr=FMath::Max(GlowErr,G&&G->Num()==1?FVector::Dist(G->Pos[0],PivotLocal+Turn(Q,GLOW)):1e9);}
  PivotErr=FVector::Dist(S47Three::ToThree(H->GetComponentLocation()),PivotWorld);
  // The mesh's lens centre, through the component's actual transform, against the turned contract.
  const FVector Lens0=S47Three::ToThree(H->GetComponentTransform().TransformPosition(S47Three::ToUE(FVector(.255,-.63,0))));
  PivotErr=FMath::Max(PivotErr,FVector::Dist(Lens0,PivotWorld+Turn(Q,FVector(.255,-.63,0))));
  for(const FString& Path:W->PathsUnder(TEXT("world.town.runtime.panels.blink.")))if(const auto* N=W->Node(W->Handle(Path)))if(const auto* Cp=Cast<UStaticMeshComponent>(N->Comp.Get()))Ok&=!Cp->IsVisible();
 }
 Ok&=PoolErr<.001&&GlowErr<.001&&PivotErr<.001&&FMath::Abs(Angles.X)<4&&FMath::Abs(Angles.Y)<2&&FMath::Abs(Angles.Z)<3;
 Lines.Add(FString::Printf(TEXT("crossing signal %s swing wire=%.3f along=%.3f twist=%.3f deg; pool err=%.5fm glow err=%.5fm lens/pivot err=%.5fm; level=%.2f"),Ok?TEXT("PASS"):TEXT("FAIL"),Angles.X,Angles.Y,Angles.Z,PoolErr,GlowErr,PivotErr,LastLevel));
 return Ok;
}

#include "S47FinaleSky.h"
#include "S47Flow.h"
#include "S47Main.h"
#include "S47OldRoad.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInstance.h"
#include "MaterialShared.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "StaticMeshResources.h"
#include "RHIShaderPlatform.h"
#if WITH_EDITOR
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshCompiler.h"
#endif

namespace
{
 const TCHAR* CraftPath = TEXT("/Game/FinaleSky/SM_S47_FinalCraft.SM_S47_FinalCraft");
 const TCHAR* CloudPath = TEXT("/Game/FinaleSky/M_S47_FinalCloud.M_S47_FinalCloud");
 const TCHAR* MatNames[] = {TEXT("Hull"),TEXT("Ribs"),TEXT("Recess"),TEXT("Light")};
 double Smooth(double A,double B,double X) { const double V=FMath::Clamp((X-A)/(B-A),0.,1.);return V*V*(3-2*V); }
 FString Digest(FSHA1& Hash) { uint8 Bytes[20]; Hash.Final();Hash.GetHash(Bytes);return BytesToHex(Bytes,20).ToLower(); }
 template<typename T> void Feed(FSHA1& H,const T& V) { H.Update(reinterpret_cast<const uint8*>(&V),sizeof(T)); }
 TSharedPtr<FJsonValue> JV(const S47J::FObj& O) { return MakeShared<FJsonValueObject>(O); }
 S47J::FObj MeshAudit(UStaticMesh* Mesh)
 {
  auto O=S47J::New(); S47J::Set(O,TEXT("path"),GetPathNameSafe(Mesh));
  const auto* R=Mesh?Mesh->GetRenderData():nullptr;
  const bool CPU=Mesh&&Mesh->bAllowCPUAccess;
  S47J::Set(O,TEXT("allowCPUAccess"),CPU); TArray<TSharedPtr<FJsonValue>> Lods;
  TArray<FString> Materials;
  if(Mesh)for(const auto& M:Mesh->GetStaticMaterials())Materials.Add(GetPathNameSafe(M.MaterialInterface.Get()));
  S47J::Set(O,TEXT("materials"),Materials);
  if(R&&CPU)for(int32 L=0;L<R->LODResources.Num();++L)
  {
   const auto& D=R->LODResources[L]; const auto& P=D.VertexBuffers.PositionVertexBuffer; const auto& V=D.VertexBuffers.StaticMeshVertexBuffer;
   auto Row=S47J::New(); S47J::Set(Row,TEXT("lod"),L);S47J::Set(Row,TEXT("vertices"),int32(P.GetNumVertices()));
   S47J::Set(Row,TEXT("texcoords"),int32(V.GetNumTexCoords())); S47J::Set(Row,TEXT("triangles"),int32(D.GetNumTriangles()));
   FSHA1 VH,IH;
   for(uint32 I=0;I<P.GetNumVertices();++I)
   {
    const FVector3f Pos=P.VertexPosition(I);Feed(VH,Pos.X);Feed(VH,Pos.Y);Feed(VH,Pos.Z);
    const FVector4f X=V.VertexTangentX(I),Z=V.VertexTangentZ(I);
    Feed(VH,X.X);Feed(VH,X.Y);Feed(VH,X.Z);Feed(VH,X.W);Feed(VH,Z.X);Feed(VH,Z.Y);Feed(VH,Z.Z);Feed(VH,Z.W);
    for(uint32 U=0;U<V.GetNumTexCoords();++U){const FVector2f UV=V.GetVertexUV(I,U);Feed(VH,UV.X);Feed(VH,UV.Y);}
    const FColor C=D.VertexBuffers.ColorVertexBuffer.GetNumVertices()?D.VertexBuffers.ColorVertexBuffer.VertexColor(I):FColor::White;Feed(VH,C.DWColor());
   }
   const auto Indices=D.IndexBuffer.GetArrayView();for(int32 I=0;I<Indices.Num();++I){const uint32 N=Indices[I];Feed(IH,N);}
   S47J::Set(Row,TEXT("packedVertexSHA1"),Digest(VH));S47J::Set(Row,TEXT("normalizedUint32IndexSHA1"),Digest(IH));
   TArray<TSharedPtr<FJsonValue>> Sections;
   for(const auto& S:D.Sections)
   {
    auto Q=S47J::New();S47J::Set(Q,TEXT("material"),S.MaterialIndex);S47J::Set(Q,TEXT("first"),int32(S.FirstIndex));S47J::Set(Q,TEXT("triangles"),int32(S.NumTriangles));
    S47J::Set(Q,TEXT("min"),int32(S.MinVertexIndex));S47J::Set(Q,TEXT("max"),int32(S.MaxVertexIndex));
    S47J::Set(Q,TEXT("collision"),S.bEnableCollision);S47J::Set(Q,TEXT("shadow"),S.bCastShadow);S47J::Set(Q,TEXT("forceOpaque"),S.bForceOpaque);Sections.Add(JV(Q));
   }
   Row->SetArrayField(TEXT("sections"),Sections);Lods.Add(JV(Row));
  }
  O->SetArrayField(TEXT("lods"),Lods);
  if(Mesh){const auto B=Mesh->GetBoundingBox();S47J::Set(O,TEXT("minCm"),TArray<double>{B.Min.X,B.Min.Y,B.Min.Z});S47J::Set(O,TEXT("maxCm"),TArray<double>{B.Max.X,B.Max.Y,B.Max.Z});}
  return O;
 }
 bool ShaderReady(UMaterialInterface* M)
 {
  if(!M||!M->GetMaterial())return false;
#if WITH_EDITOR
  UMaterial* B=M->GetMaterial();
  const auto* R=B->GetMaterialResource(GMaxRHIShaderPlatform);
  return R&&R->GetGameThreadShaderMap()&&R->IsGameThreadShaderMapComplete()&&!B->IsCompilingOrHadCompileError(GMaxRHIShaderPlatform);
#else
  return true;
#endif
 }
}

S47J::FObj FS47FinaleSky::InspectAssets()
{
 auto O=S47J::New();UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,CraftPath);auto Audit=MeshAudit(Mesh);S47J::Set(O,TEXT("mesh"),Audit);
 UMaterialInterface* Cloud=LoadObject<UMaterialInterface>(nullptr,CloudPath);
 bool Good=Mesh&&Mesh->bAllowCPUAccess&&S47J::Arr(Audit,TEXT("lods")).Num()==3&&Mesh->GetStaticMaterials().Num()==4&&Cloud&&Cloud->GetMaterial()&&Cloud->GetBlendMode()==BLEND_Translucent&&Cloud->GetMaterial()->GetMaterialDomain()==MD_Surface;
 TArray<TSharedPtr<FJsonValue>> Mats;
 for(const auto* N:MatNames)
 {
  const FString Path=FString::Printf(TEXT("/Game/FinaleSky/MI_S47_Final%s.MI_S47_Final%s"),N,N);
  auto* M=LoadObject<UMaterialInstance>(nullptr,*Path);auto Row=S47J::New();S47J::Set(Row,TEXT("path"),Path);S47J::Set(Row,TEXT("parent"),M?GetPathNameSafe(M->Parent.Get()):FString());
  FLinearColor Color=FLinearColor::Black;float Roughness=0,Metallic=0,Glow=0;
  const bool Read=M&&M->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")),Color)&&M->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Roughness")),Roughness)
   &&M->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Metallic")),Metallic)&&M->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Glow")),Glow);
  S47J::Set(Row,TEXT("read"),Read);S47J::Set(Row,TEXT("color"),TArray<double>{Color.R,Color.G,Color.B,Color.A});S47J::Set(Row,TEXT("roughness"),Roughness);S47J::Set(Row,TEXT("metallic"),Metallic);S47J::Set(Row,TEXT("glow"),Glow);
  S47J::Set(Row,TEXT("shaderReady"),ShaderReady(M));Good&=Read&&M->GetBlendMode()==BLEND_Opaque;Mats.Add(JV(Row));
 }
 O->SetArrayField(TEXT("materials"),Mats);S47J::Set(O,TEXT("cloudPath"),GetPathNameSafe(Cloud));S47J::Set(O,TEXT("cloudShaderReady"),ShaderReady(Cloud));
 S47J::Set(O,TEXT("status"),Good?TEXT("MEASURED"):TEXT("REJECTED"));return O;
}

FS47FinaleSky::FS47FinaleSky(US47Main* InMain):Main(InMain) {}
FS47FinaleSky::~FS47FinaleSky(){Hide();if(Craft.IsValid())Craft->DestroyComponent();for(auto& B:Banks)if(B.IsValid())B->DestroyComponent();}

bool FS47FinaleSky::Prepare()
{
 if(bReady)return true;auto* M=Main.Get();if(!M||!M->Web||!M->World||!M->World->OldRoad())return false;
 FString Json;if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("WebData/finalesky/binding.json")))){Failure=TEXT("fresh verified FinaleSky binding missing");return false;}
 Binding=S47J::Read(Json);
 if(S47J::Int(Binding,TEXT("schema"))!=1||S47J::Str(Binding,TEXT("status"))!=TEXT("PASS")||S47J::Int(Binding,TEXT("freshPID"))==S47J::Int(Binding,TEXT("importPID"))||S47J::Int(Binding,TEXT("freshPID"))<=0){Failure=TEXT("fresh separate-PID binding rejected");return false;}
 ActualAssets=InspectAssets();const auto Expected=S47J::Obj(Binding,TEXT("assets"));
 const auto ExpectedMesh=S47J::Obj(Expected,TEXT("mesh")),ActualMesh=S47J::Obj(ActualAssets,TEXT("mesh"));
 bool Same=ExpectedMesh&&ActualMesh&&FJsonValue::CompareEqual(FJsonValueObject(ExpectedMesh),FJsonValueObject(ActualMesh));
 const auto StableMaterials=[](const S47J::FObj& R){auto O=S47J::New();TArray<TSharedPtr<FJsonValue>> Rows;for(const auto& V:S47J::Arr(R,TEXT("materials"))){auto C=S47J::Clone(V->AsObject());C->RemoveField(TEXT("shaderReady"));Rows.Add(JV(C));}O->SetArrayField(TEXT("materials"),Rows);return O;};
 Same=Same&&FJsonValue::CompareEqual(FJsonValueObject(StableMaterials(Expected)),FJsonValueObject(StableMaterials(ActualAssets)));
 if(!Same||S47J::Str(ActualAssets,TEXT("status"))!=TEXT("MEASURED")){Failure=TEXT("actual complete three-LOD/material payload differs from fresh native binding");return false;}
 // Asynchronous shader completion is a live gate, never a pinned mutable snapshot.
 if(!S47J::Bool(ActualAssets,TEXT("cloudShaderReady"))){Failure=TEXT("own cloud material still compiling");return false;}
 for(const auto& V:S47J::Arr(ActualAssets,TEXT("materials")))if(!S47J::Bool(V->AsObject(),TEXT("shaderReady"))){Failure=TEXT("own craft material still compiling");return false;}
 const auto* Road=M->World->OldRoad();Crossing=Road->Lane(0);const FVector D=S47Three::ToUE(Road->Lane(20)-Crossing).GetSafeNormal2D();ForwardUE=D;RightUE=FVector::CrossProduct(FVector::UpVector,D).GetSafeNormal();
 UStaticMesh* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,CraftPath);UMaterialInterface* Cloud=LoadObject<UMaterialInterface>(nullptr,CloudPath);
 if(!Cube||!Mesh||!Cloud)return false;
 const auto Make=[&](const TCHAR* Name,UStaticMesh* Asset){auto* C=NewObject<UStaticMeshComponent>(M->Web,MakeUniqueObjectName(M->Web,UStaticMeshComponent::StaticClass(),FName(Name)));C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(M->Web->GetRootComponent());C->SetStaticMesh(Asset);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->bAffectDistanceFieldLighting=false;C->SetVisibleInRayTracing(false);C->SetVisibility(false);C->RegisterComponent();return C;};
 Craft=Make(TEXT("SC13OriginalFinalCraft"),Mesh);
 for(int32 I=0;I<2;++I){Banks[I]=Make(I?TEXT("SC13CloudBankRight"):TEXT("SC13CloudBankLeft"),Cube);auto* Mid=UMaterialInstanceDynamic::Create(Cloud,M->Web);BankMids[I]=Mid;Banks[I]->SetMaterial(0,Mid);Banks[I]->SetTranslucentSortPriority(4+I);}
 bReady=true;Failure.Reset();return true;
}

void FS47FinaleSky::Hide(){bShown=false;if(Craft.IsValid())Craft->SetVisibility(false);for(auto& B:Banks)if(B.IsValid())B->SetVisibility(false);}

void FS47FinaleSky::Step(double Mile,const FString& Stage,double E,double P)
{
 if(!Prepare())return;auto* M=Main.Get();if(!M||!M->World||M->World->Area!=TEXT("roswell")||Stage==TEXT("end")){Hide();return;}
 bLow=M->Web->GetGraphicsSettings().Effects==0;
 Phase=Stage==TEXT("drive")?-80*(1-Smooth(5.9,8.15,Mile)):Stage==TEXT("event")?E:47+P;
 Growth=Smooth(-80,6,Phase);Reveal=Smooth(5,21,Phase);bShown=Growth>.001;
 const FVector Base=S47Three::ToUE(Crossing),Up=FVector::UpVector;
 // Radius680m, height116m. The original hull is opaque for the entire reveal; banks do the hiding.
 const FVector CraftAt=Base+ForwardUE*110000+RightUE*12000+Up*(31500+160*FMath::Sin(Phase*.024));
 Craft->SetWorldLocation(CraftAt);Craft->SetWorldScale3D(FVector(680));Craft->SetWorldRotation(FRotator(0,0,-1.6));Craft->SetVisibility(bShown);
 for(int32 I=0;I<2;++I)
 {
  const double Side=I?1.:-1.;const FVector Extent(42000,56000,25500);
  const FVector Center=Base+ForwardUE*(92000-9000*Smooth(-65,22,Phase))+RightUE*(Side*(49500+23500*Reveal))+Up*(35100+1800*FMath::Sin(Phase*.018+I));
  auto* B=Banks[I].Get();auto* D=BankMids[I].Get();if(!B||!D)continue;
  B->SetWorldLocation(Center);B->SetWorldRotation(FRotationMatrix::MakeFromXZ(ForwardUE,Up).Rotator());B->SetWorldScale3D(Extent/50.);B->SetVisibility(bShown);
  D->SetVectorParameterValue(TEXT("BankCenter"),FLinearColor(Center.X,Center.Y,Center.Z,0));D->SetVectorParameterValue(TEXT("BankExtent"),FLinearColor(Extent.X,Extent.Y,Extent.Z,0));
  D->SetVectorParameterValue(TEXT("Forward"),FLinearColor(ForwardUE.X,ForwardUE.Y,ForwardUE.Z,0));D->SetVectorParameterValue(TEXT("Right"),FLinearColor(RightUE.X,RightUE.Y,RightUE.Z,0));
  D->SetScalarParameterValue(TEXT("Phase"),Phase);D->SetScalarParameterValue(TEXT("Growth"),Growth);D->SetScalarParameterValue(TEXT("Side"),Side);D->SetScalarParameterValue(TEXT("Samples"),bLow?28:56);D->SetScalarParameterValue(TEXT("Glow"),.12+.22*Smooth(0,17,Phase));
 }
}

S47J::FObj FS47FinaleSky::Readback() const
{
 auto O=S47J::New();S47J::Set(O,TEXT("ready"),bReady);S47J::Set(O,TEXT("shown"),bShown);S47J::Set(O,TEXT("failure"),Failure);S47J::Set(O,TEXT("low"),bLow);
 S47J::Set(O,TEXT("phase"),Phase);S47J::Set(O,TEXT("growth"),Growth);S47J::Set(O,TEXT("reveal"),Reveal);S47J::Set(O,TEXT("samples"),bLow?28:56);S47J::Set(O,TEXT("craftOpacity"),1.);
 S47J::Set(O,TEXT("binding"),Binding);S47J::Set(O,TEXT("actualAssets"),ActualAssets);
 TArray<TSharedPtr<FJsonValue>> Parts;
 const auto Part=[&](const TCHAR* Id,UStaticMeshComponent* C){auto R=S47J::New();S47J::Set(R,TEXT("id"),Id);S47J::Set(R,TEXT("component"),GetPathNameSafe(C));if(C){const FVector P=C->GetComponentLocation(),S=C->GetComponentScale(),E=C->Bounds.BoxExtent;S47J::Set(R,TEXT("positionUE"),TArray<double>{P.X,P.Y,P.Z});S47J::Set(R,TEXT("scale"),TArray<double>{S.X,S.Y,S.Z});S47J::Set(R,TEXT("boundsExtentUE"),TArray<double>{E.X,E.Y,E.Z});S47J::Set(R,TEXT("visible"),C->IsVisible());S47J::Set(R,TEXT("mesh"),GetPathNameSafe(C->GetStaticMesh().Get()));S47J::Set(R,TEXT("collision"),int32(C->GetCollisionEnabled()));S47J::Set(R,TEXT("castShadow"),bool(C->CastShadow));TArray<FString> Ms;for(int32 I=0;I<C->GetNumMaterials();++I)Ms.Add(GetPathNameSafe(C->GetMaterial(I)));S47J::Set(R,TEXT("materials"),Ms);}Parts.Add(JV(R));};
 Part(TEXT("opaque-original-craft"),Craft.Get());Part(TEXT("left-finite-volume"),Banks[0].Get());Part(TEXT("right-finite-volume"),Banks[1].Get());O->SetArrayField(TEXT("parts"),Parts);
 TArray<TSharedPtr<FJsonValue>> Params;
 for(int32 I=0;I<2;++I)
 {
  auto Q=S47J::New();auto* D=BankMids[I].Get();bool Complete=D!=nullptr;S47J::Set(Q,TEXT("bank"),I);S47J::Set(Q,TEXT("parent"),D?GetPathNameSafe(D->Parent.Get()):FString());
  for(const auto* K:{TEXT("Phase"),TEXT("Growth"),TEXT("Side"),TEXT("Samples"),TEXT("Glow")}){float V=0;const bool Good=D&&D->GetScalarParameterValue(FHashedMaterialParameterInfo(K),V);Complete&=Good;S47J::Set(Q,K,V);}
  for(const auto* K:{TEXT("BankCenter"),TEXT("BankExtent"),TEXT("Forward"),TEXT("Right")}){FLinearColor V=FLinearColor::Black;const bool Good=D&&D->GetVectorParameterValue(FHashedMaterialParameterInfo(K),V);Complete&=Good;S47J::Set(Q,K,TArray<double>{V.R,V.G,V.B,V.A});}
  S47J::Set(Q,TEXT("complete"),Complete);Params.Add(JV(Q));
 }
 O->SetArrayField(TEXT("actualBankParameters"),Params);
 S47J::Set(O,TEXT("occlusionBasis"),TEXT("Own finite-volume shader clips integration against actual opaque SceneDepth converted with PixelDepth to camera-ray distance; craft never fades. Images/actual GPU and performance remain separate acceptance."));return O;
}

#if WITH_EDITOR
namespace
{
 S47J::FObj SavedTriangles(UStaticMesh* Mesh)
 {
  auto O=S47J::New();TArray<TSharedPtr<FJsonValue>> Lods;
  if(Mesh)for(int32 L=0;L<Mesh->GetNumSourceModels();++L)
  {
   auto Row=S47J::New();S47J::Set(Row,TEXT("lod"),L);TArray<TSharedPtr<FJsonValue>> Tris,Groups;
   const FMeshDescription* D=Mesh->GetMeshDescription(L);
   if(D)
   {
    const FStaticMeshConstAttributes A(*D);const auto Positions=A.GetVertexPositions();const auto UVs=A.GetVertexInstanceUVs();const auto Names=A.GetPolygonGroupMaterialSlotNames();
    for(const auto G:D->PolygonGroups().GetElementIDs())
    {auto Q=S47J::New();S47J::Set(Q,TEXT("id"),G.GetValue());S47J::Set(Q,TEXT("name"),Names[G].ToString());S47J::Set(Q,TEXT("slot"),Mesh->GetMaterialIndexFromImportedMaterialSlotName(Names[G]));Groups.Add(JV(Q));}
    for(const auto T:D->Triangles().GetElementIDs())
    {
     auto Q=S47J::New();TArray<TSharedPtr<FJsonValue>> Ps,Ts;
     for(const auto V:D->GetTriangleVertexInstances(T))
     {
      const auto P=Positions[D->GetVertexInstanceVertex(V)];const auto U=UVs.Get(V,0);
      auto Pj=S47J::New();S47J::Set(Pj,TEXT("v"),TArray<double>{P.X,P.Y,P.Z});Ps.Add(JV(Pj));
      auto Uj=S47J::New();S47J::Set(Uj,TEXT("v"),TArray<double>{U.X,U.Y});Ts.Add(JV(Uj));
     }
     Q->SetArrayField(TEXT("p"),Ps);Q->SetArrayField(TEXT("uv"),Ts);S47J::Set(Q,TEXT("material"),Names[D->GetTrianglePolygonGroup(T)].ToString());Tris.Add(JV(Q));
    }
   }
   Row->SetArrayField(TEXT("triangles"),Tris);Row->SetArrayField(TEXT("groups"),Groups);Lods.Add(JV(Row));
  }
  O->SetArrayField(TEXT("lods"),Lods);return O;
 }
 FAutoConsoleCommand AssetsReader(TEXT("S47.FinaleSkyAssets"),TEXT("Read only own actual render LOD/indices/sections/materials; argument is fresh output JSON."),FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& A)
 {
  if(A.Num()!=1||FPaths::IsRelative(A[0])||FPaths::IsUnderDirectory(A[0],FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()))||IFileManager::Get().FileExists(*A[0]))return;
  UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,CraftPath);
  if(Mesh){TArray<UStaticMesh*> Own{Mesh};FStaticMeshCompilingManager::Get().FinishCompilation(Own);}
  const auto R=FS47FinaleSky::InspectAssets();S47J::Set(R,TEXT("savedSource"),SavedTriangles(Mesh));
  if(FFileHelper::SaveStringToFile(S47J::Write(R,true),*A[0]))UE_LOG(LogTemp,Display,TEXT("S47: FinaleSky own assets measured %s"),*A[0]);
 }));
}
#endif

#include "S47SaroSeam.h"
#include "S47FreeformTerrain.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/SecureHash.h"
#include "Math/Float16.h"
#include "PhysicsEngine/BodySetup.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "StaticMeshResources.h"
#include <cfloat>
#if WITH_EDITOR
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshCompiler.h"
#endif

namespace
{
 constexpr double Z0=-264, Z1=-232, Outer=11.25;
 // Geometry ends at site68. Keep native floor sampling through100, where the retained
 // g4 underlay has sunk beneath the highway. Ending at68 would reintroduce a 5.5cm
 // analytic shoulder drop despite continuous visible ground.
 constexpr double FloorZ1=-200;
 constexpr int32 Revision=1;
 const TCHAR* UnderlayPath=TEXT("/Game/MotelV2/Trim/g4.g4");
 const TCHAR* Sources[]={TEXT("/Game/Web/Geo/geo_005/StaticMeshes/g664.g664"),TEXT("/Game/MotelV2/Trim/g663.g663")};
 const TCHAR* Targets[]={TEXT("/Game/Signal47/Art/SaroRoad/SM_SaroRoadSeam.SM_SaroRoadSeam"),TEXT("/Game/Signal47/Art/SaroRoad/SM_SaroVergeSeam.SM_SaroVergeSeam")};
 struct FCorner
 {
  FVector3f P,X,Y,N; FColor Color; TArray<FVector2f> UV;
 };
 struct FTriangle { FCorner C[3]; int32 Slot=0; bool bEdited=false; };
 struct FFloor { FVector P[3]; int32 Part=0; }; // corridor local, Three metres
 struct FInstalled
 {
  TWeakObjectPtr<UStaticMeshComponent> C[3]; TWeakObjectPtr<UStaticMesh> Mesh[3];
  FTransform Transform[3]; TArray<FFloor> Floor;
 };
 TMap<TWeakObjectPtr<AS47Web>,FInstalled> Installed;
 bool Fail(FString& Why,const TCHAR* Text){Why=Text;return false;}
 double Blend(double Z){const double T=FMath::Clamp((Z-Z0)/(Z1-Z0),0.0,1.0);return T*T*(3-2*T);}
 double Width(double Z){return 4+2*Blend(Z);}
 bool Row(double Z){return Z>=Z0-.0001&&Z<Z1-.0001;}
 FVector Three(const FCorner& C){return S47Three::ToThree(FVector(C.P));}
 bool Band(const FTriangle& T)
 {
  double Min=DBL_MAX,Max=-DBL_MAX,Ax=0;
  for(const auto& C:T.C){const FVector P=Three(C);Min=FMath::Min(Min,P.Z);Max=FMath::Max(Max,P.Z);Ax=FMath::Max(Ax,FMath::Abs(P.X));}
  return Min>=Z0-.0001&&Max<=Z1+.0001&&Min<Z1-.0001&&Ax<=Outer+.0001;
 }
 bool FloorBand(const FTriangle& T)
 {
  double Min=DBL_MAX,Max=-DBL_MAX,Ax=0;
  for(const auto& C:T.C){const FVector P=Three(C);Min=FMath::Min(Min,P.Z);Max=FMath::Max(Max,P.Z);Ax=FMath::Max(Ax,FMath::Abs(P.X));}
  return Min>=Z0-.0001&&Max<=FloorZ1+.0001&&Min<FloorZ1-.0001&&Ax<=Outer+.0001;
 }
 bool Read(UStaticMesh* Mesh,TArray<FTriangle>& Out,bool RequireCPU,FString& Why)
 {
  if(!Mesh||Mesh->IsCompiling()||(RequireCPU&&!Mesh->bAllowCPUAccess)||!Mesh->GetRenderData()||Mesh->GetRenderData()->LODResources.Num()!=1)
   return Fail(Why,TEXT("ready single render LOD / retained CPU data required"));
  const auto& L=Mesh->GetRenderData()->LODResources[0];const auto& B=L.VertexBuffers;
  const uint32 Count=B.PositionVertexBuffer.GetNumVertices(),Colors=B.ColorVertexBuffer.GetNumVertices(),UVs=B.StaticMeshVertexBuffer.GetNumTexCoords();
  if(!Count||!UVs||!B.PositionVertexBuffer.GetVertexData()||!B.StaticMeshVertexBuffer.GetTangentData()||!B.StaticMeshVertexBuffer.GetTexCoordData()||
     !L.IndexBuffer.GetIndexDataSize()||(Colors&&(Colors!=Count||!B.ColorVertexBuffer.GetVertexData())))return Fail(Why,TEXT("incomplete CPU position/tangent/UV/color/index buffers"));
  TArray<uint32> Indices;L.IndexBuffer.GetCopy(Indices);Out.Reset();
  for(const auto& S:L.Sections)
  {
   if(S.MaterialIndex>=uint32(Mesh->GetStaticMaterials().Num())||S.FirstIndex+S.NumTriangles*3>uint32(Indices.Num())||!S.bEnableCollision)
    return Fail(Why,TEXT("invalid section range/material or disabled query triangles"));
   for(uint32 I=S.FirstIndex;I<S.FirstIndex+S.NumTriangles*3;I+=3)
   {
    FTriangle T;T.Slot=S.MaterialIndex;
    for(int32 K=0;K<3;++K)
    {
     const uint32 V=Indices[I+K];if(V>=Count)return Fail(Why,TEXT("index outside render vertices"));auto& C=T.C[K];
     C.P=B.PositionVertexBuffer.VertexPosition(V);C.X=FVector3f(B.StaticMeshVertexBuffer.VertexTangentX(V));
     C.Y=FVector3f(B.StaticMeshVertexBuffer.VertexTangentY(V));C.N=FVector3f(B.StaticMeshVertexBuffer.VertexTangentZ(V));
     C.Color=Colors?B.ColorVertexBuffer.VertexColor(V):FColor::White;
     for(uint32 U=0;U<UVs;++U)C.UV.Add(B.StaticMeshVertexBuffer.GetVertexUV(V,U));
     if(C.P.ContainsNaN()||C.N.ContainsNaN()||C.X.ContainsNaN())return Fail(Why,TEXT("nonfinite render corner"));
    }
    Out.Add(MoveTemp(T));
   }
  }
  return Out.Num()==int32(L.GetNumTriangles())||Fail(Why,TEXT("section coverage differs from LOD triangles"));
 }
 // Winding is retained. Quantization is solely for a portable saved shape/UV/color identity;
 // actual floor queries always use unrounded native positions.
 FString CornerKey(const FCorner& C)
 {
  auto Clean=[](double V,double E){return FMath::Abs(V)<E?0.:V;};
  FString K=FString::Printf(TEXT("%.3f,%.3f,%.3f:%u,%u,%u,%u"),Clean(C.P.X,.0005),Clean(C.P.Y,.0005),Clean(C.P.Z,.0005),C.Color.R,C.Color.G,C.Color.B,C.Color.A);
  for(const auto& UV:C.UV)K+=FString::Printf(TEXT(":%.5f,%.5f"),Clean(UV.X,.000005),Clean(UV.Y,.000005));return K;
 }
 FString TriangleKey(const FTriangle& T)
 {
  FString K[3]={CornerKey(T.C[0]),CornerKey(T.C[1]),CornerKey(T.C[2])};
  FString Ordered=K[0]+TEXT("|")+K[1]+TEXT("|")+K[2];
  for(int32 I=1;I<3;++I){const FString R=K[I]+TEXT("|")+K[(I+1)%3]+TEXT("|")+K[(I+2)%3];if(R<Ordered)Ordered=R;}
  return FString::Printf(TEXT("%d:"),T.Slot)+Ordered;
 }
 FString Signature(const TArray<FTriangle>& Ts,bool Outside=false)
 {
  TArray<FString> Lines;
  for(const auto& T:Ts)
  {
   if(Outside&&Band(T))continue;
   Lines.Add(TriangleKey(T));
  }
  Lines.Sort();const FString S=FString::Join(Lines,TEXT("\n"));FTCHARToUTF8 B(*S);return FSHA1::HashBuffer(B.Get(),B.Length()).ToString();
 }
 bool At(const FVector* P,double X,double Z,double& Y)
 {
  const double D=(P[1].Z-P[2].Z)*(P[0].X-P[2].X)+(P[2].X-P[1].X)*(P[0].Z-P[2].Z);
  if(FMath::Abs(D)<1.e-12)return false;
  const double A=((P[1].Z-P[2].Z)*(X-P[2].X)+(P[2].X-P[1].X)*(Z-P[2].Z))/D;
  const double B=((P[2].Z-P[0].Z)*(X-P[2].X)+(P[0].X-P[2].X)*(Z-P[2].Z))/D;
  if(A< -1.e-8||B< -1.e-8||A+B>1+1.e-8)return false;
  Y=A*P[0].Y+B*P[1].Y+(1-A-B)*P[2].Y;return true;
 }
 bool Sample(const TArray<FTriangle>& Ts,double X,double Z,double& Y)
 {
  bool Found=false;Y=-DBL_MAX;
  for(const auto& T:Ts){const FVector P[3]={Three(T.C[0]),Three(T.C[1]),Three(T.C[2])};double H;
   if(At(P,X,Z,H)){Y=FMath::Max(Y,H);Found=true;}}
  return Found;
 }
 bool FloorAt(const TArray<FFloor>& Ts,double X,double Z,double& Y,bool* Road)
 {
  bool Found=false;Y=-DBL_MAX;
  for(const auto& T:Ts){double H;if(At(T.P,X,Z,H)&&(!Found||H>Y+1.e-8|| (FMath::Abs(H-Y)<=1.e-8&&T.Part==0)))
   {Y=H;Found=true;if(Road)*Road=T.Part==0;}}
  return Found;
 }
 bool MaterialsSame(UStaticMesh* A,UStaticMesh* B)
 {
  const auto& X=A->GetStaticMaterials();const auto& Y=B->GetStaticMaterials();if(X.Num()!=Y.Num())return false;
  for(int32 I=0;I<X.Num();++I)if(X[I].MaterialSlotName!=Y[I].MaterialSlotName||X[I].ImportedMaterialSlotName!=Y[I].ImportedMaterialSlotName||X[I].MaterialInterface!=Y[I].MaterialInterface)return false;
  return true;
 }
 bool Candidate(UStaticMesh* Source,UStaticMesh* Mesh,int32 Role,TArray<FTriangle>& Ts,FString& Why)
 {
  if(!Source||!Mesh||Source->GetPathName()!=Sources[Role]||Mesh->GetPathName()!=Targets[Role])return Fail(Why,TEXT("actual source/derivative path differs"));
  const auto* P=Cast<US47SaroSeamProof>(Mesh->GetAssetUserDataOfClass(US47SaroSeamProof::StaticClass()));
  if(!P||P->Source!=Source||P->Revision!=Revision||P->SourceTriangles!=Source->GetNumTriangles(0)||
     !P->SourceMin.Equals(Source->GetBoundingBox().Min,.001)||!P->SourceMax.Equals(Source->GetBoundingBox().Max,.001)||
     !MaterialsSame(Source,Mesh)||Mesh->bSupportRayTracing!=Source->bSupportRayTracing||Mesh->HasValidNaniteData()||
     !Mesh->GetBodySetup()||Mesh->GetBodySetup()==Source->GetBodySetup()||Mesh->GetBodySetup()->CollisionTraceFlag!=CTF_UseComplexAsSimple)
   return Fail(Why,TEXT("derived proof/material/RT/independent complex collision differs"));
#if WITH_EDITORONLY_DATA
  if(Mesh->ComplexCollisionMesh)return Fail(Why,TEXT("separate complex collision asset is not allowed"));
#endif
  if(!Read(Mesh,Ts,true,Why))return false;
  return (P->Triangles==Ts.Num()&&Signature(Ts)==P->SurfaceSignature)||Fail(Why,TEXT("saved native triangles/UV/color signature differs"));
 }
#if WITH_EDITOR
 bool Make(const TArray<FTriangle>& Source,const TArray<FTriangle>& Road,bool Ground,TArray<FTriangle>& Out,FString& Why)
 {
  Out=Source;int32 Changed=0;TSet<int32> Rows;
  for(auto& T:Out)
  {
   bool Edited=false;
   for(auto& C:T.C)
   {
    FVector P=Three(C);if(!Row(P.Z))continue;
    if(Ground&&FMath::Abs(FMath::Abs(P.X)-6)>.0001)continue;
    if(!Ground&&FMath::Abs(P.X)>6.0001)return Fail(Why,TEXT("road width exceeds measured source"));
    const double RowIndex=(P.Z-Z0)/8,SourceRow=Z0+8*FMath::RoundToDouble(RowIndex);
    if(FMath::Abs(P.Z-SourceRow)>.0001)return Fail(Why,TEXT("unexpected seam row spacing"));
    const FVector Old=P;const double U=C.UV[0].X;
    // The trimmed verge's native position can carry ~20 micrometres of bake error;
    // its validated original +/-6m column still retains the exact authored +/-1.2 UV.
    const double ExpectedU=Ground?FMath::Sign(P.X)*6./5.:(6-P.X)/12;
    // Imported render UVs may be half precision (for example .7 becomes .700195).
    // Accept that exact quantization, never a guessed flip or a broad UV tolerance.
    const double HalfU=FFloat16(float(ExpectedU)).GetFloat();
    if(FMath::Abs(U-ExpectedU)>1.e-6&&FMath::Abs(U-HalfU)>1.e-6)
     return Fail(Why,*FString::Printf(TEXT("source transverse UV differs: ground=%d X=%.9f Z=%.9f U=%.9f expected=%.9f half=%.9f"),Ground,P.X,P.Z,U,ExpectedU,HalfU));
    P.Z=SourceRow; // Join the validated authored row despite the trimmed mesh's tiny bake error.
    P.X=Ground?FMath::Sign(P.X)*Width(P.Z):FMath::Clamp(P.X,-Width(P.Z),Width(P.Z));
    double H;if(!Sample(Road,P.X,P.Z,H))return Fail(Why,TEXT("source road triangle missing at clipped vertex"));
    if(Ground)
    {
     double Edge=0;const bool Found=Sample(Road,FMath::Sign(Old.X)*6,SourceRow,Edge);
     if(!Found||FMath::Abs(Edge-Old.Y)>.001)
      return Fail(Why,*FString::Printf(TEXT("ground inner edge differs: X=%.9f Z=%.9f Y=%.9f roadY=%.9f found=%d"),Old.X,Old.Z,Old.Y,Edge,Found));
    }
    P.Y=FMath::Lerp(-.58,H,Blend(P.Z));C.P=FVector3f(S47Three::ToUE(P));
    C.UV[0].X=float(Ground?P.X/5:(6-P.X)/12); // V and all other channels remain unchanged.
    Edited|=!P.Equals(Old,1.e-8);Rows.Add(FMath::RoundToInt(RowIndex));
   }
   if(!Edited)continue;
   if(!Band(T))return Fail(Why,TEXT("deformation reaches outside local road/verge band"));
   ++Changed;T.bEdited=true;
   const FVector3f E1=T.C[1].P-T.C[0].P,E2=T.C[2].P-T.C[0].P;
   FVector3f N=FVector3f::CrossProduct(E1,E2).GetSafeNormal();
   if(N.IsNearlyZero())continue; // Only newly collapsed edge triangles are removed below.
   if(FVector3f::DotProduct(N,T.C[0].N)<0)N=-N;
   const FVector2f A=T.C[1].UV[0]-T.C[0].UV[0],B=T.C[2].UV[0]-T.C[0].UV[0];const float D=A.X*B.Y-A.Y*B.X;
   if(FMath::Abs(D)<1.e-12f)return Fail(Why,TEXT("nondegenerate edited triangle has degenerate UV"));
   FVector3f X=(E1*B.Y-E2*A.Y)/D;X=(X-N*FVector3f::DotProduct(N,X)).GetSafeNormal();
   const FVector3f RawY=(E2*A.X-E1*B.X)/D;FVector3f Y=FVector3f::CrossProduct(N,X);if(FVector3f::DotProduct(Y,RawY)<0)Y=-Y;
   for(auto& C:T.C){C.N=N;C.X=X;C.Y=Y;}
  }
  if(!Changed||Rows.Num()!=4)return Fail(Why,TEXT("four expected changed 8m rings not found"));
  Out.RemoveAll([](const FTriangle& T){return T.bEdited&&FVector3f::CrossProduct(T.C[1].P-T.C[0].P,T.C[2].P-T.C[0].P).SizeSquared()<1.e-8f;});
  if(Signature(Source,true)!=Signature(Out,true))return Fail(Why,TEXT("geometry/UV/color outside band changed"));
  return true;
 }
 bool Store(UStaticMesh* Source,UStaticMesh* Fixed,const TArray<FTriangle>& Ts,FString& Why)
 {
  FMeshDescription D;FStaticMeshAttributes A(D);A.Register();
  auto P=A.GetVertexPositions();auto N=A.GetVertexInstanceNormals();auto X=A.GetVertexInstanceTangents();
  auto Signs=A.GetVertexInstanceBinormalSigns();auto Colors=A.GetVertexInstanceColors();auto UV=A.GetVertexInstanceUVs();
  auto Slots=A.GetPolygonGroupMaterialSlotNames();UV.SetNumChannels(Ts[0].C[0].UV.Num());
  for(int32 S=0;S<Source->GetStaticMaterials().Num();++S){const FPolygonGroupID G(S);D.CreatePolygonGroupWithID(G);Slots[G]=Source->GetStaticMaterials()[S].ImportedMaterialSlotName;Fixed->GetSectionInfoMap().Set(0,S,FMeshSectionInfo(S));}
  // Native render corners are the input: current MotelV2 holes and all UV channels survive.
  // Separate source corner instances also preserve intentional hard normals/UV seams.
  for(const auto& T:Ts)
  {
   FVertexInstanceID C[3];for(int32 K=0;K<3;++K){const auto& R=T.C[K];const FVertexID V=D.CreateVertex();P[V]=R.P;
    // Packed native directions are not necessarily unit length. Normalize before UE
    // fills missing bases, preserving direction while avoiding a skewed generated tangent.
    C[K]=D.CreateVertexInstance(V);N[C[K]]=R.N.GetSafeNormal();X[C[K]]=R.X.GetSafeNormal();Signs[C[K]]=FVector3f::DotProduct(FVector3f::CrossProduct(R.N,R.X),R.Y)<0?-1.f:1.f;
    Colors[C[K]]=FVector4f(FLinearColor::FromSRGBColor(R.Color));for(int32 I=0;I<R.UV.Num();++I)UV.Set(C[K],I,R.UV[I]);}
   D.CreateTriangle(FPolygonGroupID(T.Slot),C);
  }
  D.BuildIndexers();Fixed->SetNumSourceModels(1);Fixed->CreateMeshDescription(0,MoveTemp(D));Fixed->CommitMeshDescription(0);
  auto& B=Fixed->GetSourceModel(0).BuildSettings;B.BuildScale3D=FVector::OneVector;B.bRecomputeNormals=false;B.bRecomputeTangents=false;
  B.bRemoveDegenerates=false;B.bGenerateLightmapUVs=false;B.bUseFullPrecisionUVs=true;B.bUseHighPrecisionTangentBasis=true;
  Fixed->bAllowCPUAccess=true;Fixed->NeverStream=true;Fixed->ComplexCollisionMesh=nullptr;
  if(!Source->GetBodySetup())return Fail(Why,TEXT("source query body absent"));
  // Clone the policy, invalidate all old cooked shapes, and cook the NEW native triangles.
  auto* Body=DuplicateObject<UBodySetup>(Source->GetBodySetup(),Fixed);Fixed->SetBodySetup(Body);
  Body->CollisionTraceFlag=CTF_UseComplexAsSimple;Body->InvalidatePhysicsData();
  TArray<FText> Errors;Fixed->Build(true,&Errors);if(Errors.Num())return Fail(Why,TEXT("native derivative Build reported errors"));
  TArray<UStaticMesh*> Pending={Fixed};FStaticMeshCompilingManager::Get().FinishCompilation(Pending);
  Body=Fixed->GetBodySetup();Body->InvalidatePhysicsData();Body->CreatePhysicsMeshes();
  if(Body->bFailedToCreatePhysicsMeshes)return Fail(Why,TEXT("new collision cook failed"));
  return true;
 }
 bool VerifyOutput(const TArray<FTriangle>& Expected,const TArray<FTriangle>& Actual,FString& Why,int32& FilledLeafCorners,int32& FilledLeafNormals)
 {
  if(Expected.Num()!=Actual.Num()||Signature(Expected)!=Signature(Actual))return Fail(Why,TEXT("native output differs from intended oriented triangles/UV/color/materials"));
  // Repacking tangents can normalize them. Verify direction, not raw packed bytes.
  struct FExpected { const FCorner* Corner; bool UndefinedLeaf; };
  TMap<FString,TArray<FExpected>> Corners;FilledLeafCorners=0;FilledLeafNormals=0;
  for(const auto& T:Expected)
  {
   // Original groundWithTufts blades have one constant UV and no tangent basis.
   // UE fills missing tangents even when recomputation is off; no defined direction is lost.
   const bool Leaf=!T.bEdited&&T.C[0].UV[0]==T.C[1].UV[0]&&T.C[0].UV[0]==T.C[2].UV[0];
   // Bind to the same oriented triangle as well as its corner: opposite leaf faces
   // must not borrow one another's undefined basis to excuse a changed valid normal.
   const FString Face=TriangleKey(T);
   for(const auto& C:T.C)Corners.FindOrAdd(Face+TEXT("::")+CornerKey(C)).Add({&C,Leaf&&C.X.IsNearlyZero(1.e-8f)&&C.Y.IsNearlyZero(1.e-8f)});
  }
  for(const auto& T:Actual)for(const auto& C:T.C)
  {
   const auto* Matches=Corners.Find(TriangleKey(T)+TEXT("::")+CornerKey(C));bool Match=false;const FCorner* Best=nullptr;float BestMin=-2;bool BestLeaf=false;
   if(Matches)for(const auto& Entry:*Matches)
   {
    const FCorner* E=Entry.Corner;
    const float DN=FVector3f::DotProduct(C.N.GetSafeNormal(),E->N.GetSafeNormal());
    const float DX=FVector3f::DotProduct(C.X.GetSafeNormal(),E->X.GetSafeNormal());
    const float DY=FVector3f::DotProduct(C.Y.GetSafeNormal(),E->Y.GetSafeNormal());
    const float Minimum=FMath::Min3(DN,DX,DY);if(Minimum>BestMin){BestMin=Minimum;Best=E;BestLeaf=Entry.UndefinedLeaf;}
    if(Minimum>0.99999f){Match=true;break;}
    const bool UndefinedNormal=E->N.IsNearlyZero(1.e-8f);
    if(Entry.UndefinedLeaf&&(DN>0.99999f||UndefinedNormal)&&FMath::Abs(C.N.SizeSquared()-1)<.001f&&FMath::Abs(C.X.SizeSquared()-1)<.001f&&FMath::Abs(C.Y.SizeSquared()-1)<.001f&&
       FMath::Abs(FVector3f::DotProduct(C.N,C.X))<.001f&&FMath::Abs(FVector3f::DotProduct(C.N,C.Y))<.001f&&
       FMath::Abs(FVector3f::DotProduct(C.X,C.Y))<.001f)
    {Match=true;++FilledLeafCorners;if(UndefinedNormal)++FilledLeafNormals;break;}
   }
   if(!Match)return Fail(Why,*FString::Printf(TEXT("native tangent mismatch at %s minDot=%.9f leaf=%d DN=%.9f lengths=%.9f,%.9f,%.9f dots=%.9f,%.9f,%.9f UV=%.9f/%.9f,%.9f/%.9f,%.9f/%.9f actual N=%s X=%s Y=%s expected N=%s X=%s Y=%s"),
    *C.P.ToString(),BestMin,BestLeaf,Best?FVector3f::DotProduct(C.N.GetSafeNormal(),Best->N.GetSafeNormal()):-2,
    C.N.SizeSquared(),C.X.SizeSquared(),C.Y.SizeSquared(),FVector3f::DotProduct(C.N,C.X),FVector3f::DotProduct(C.N,C.Y),FVector3f::DotProduct(C.X,C.Y),
    T.C[0].UV[0].X,T.C[0].UV[0].Y,T.C[1].UV[0].X,T.C[1].UV[0].Y,T.C[2].UV[0].X,T.C[2].UV[0].Y,
    *C.N.ToString(),*C.X.ToString(),*C.Y.ToString(),Best?*Best->N.ToString():TEXT("none"),Best?*Best->X.ToString():TEXT("none"),Best?*Best->Y.ToString():TEXT("none")));
  }
  return true;
 }
#endif
 bool Current(const FInstalled& I,const AS47Web* W)
 {
  for(int32 K=0;K<3;++K)
  {
   // Freeform's relief stands in for the retained ring: it has the same triangles in the seam's band, where its table is 0.
   if(!I.C[K].IsValid()||I.C[K]->GetOwner()!=W||(I.C[K]->GetStaticMesh()!=I.Mesh[K].Get()&&!(K==2&&S47FreeformTerrain::IsRelief(W,I.C[K]->GetStaticMesh())))||
      !I.C[K]->GetComponentTransform().Equals(I.Transform[K],.0001))return false;
  }
  return true;
 }
 bool CollisionMatches(const FInstalled& I,FString& Why)
 {
  for(const auto& T:I.Floor)
  {
   const int32 K=T.Part;const FVector P=(T.P[0]+T.P[1]+T.P[2])/3;
   const FVector AtWorld=S47Three::ToUE(P+FVector(-24,0,300));FHitResult Hit;
   FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SaroSeam),true);
   if(!I.C[K]->LineTraceComponent(Hit,AtWorld+FVector(0,0,50),AtWorld-FVector(0,0,50),Q)||FMath::Abs(Hit.ImpactPoint.Z-AtWorld.Z)>.1)
    return Fail(Why,TEXT("derived component collision does not match rendered seam triangle within 1mm"));
  }
  return true;
 }
}

FString US47SaroSeamTools::Prepare(UStaticMesh* SourceRoad,UStaticMesh* SourceGround,UStaticMesh* Underlay,UStaticMesh* Road,UStaticMesh* Ground,bool bVerifyOnly)
{
 auto R=MakeShared<FJsonObject>();FString Why;bool Ok=false;
#if WITH_EDITOR
 auto Run=[&]() -> bool
 {
  if(!SourceRoad||!SourceGround||!Underlay||!Road||!Ground)return Fail(Why,TEXT("missing source or derivative"));
  TArray<UStaticMesh*> Pending={SourceRoad,SourceGround,Underlay,Road,Ground};FStaticMeshCompilingManager::Get().FinishCompilation(Pending);
  UStaticMesh* Source[2]={SourceRoad,SourceGround};UStaticMesh* Fixed[2]={Road,Ground};TArray<FTriangle> Original[2],Expected[2],Actual[2];
  TArray<FTriangle> Base;TArray<FVector> UnderlayCorners;
  if(!Underlay||Underlay->GetPathName()!=UnderlayPath||!Read(Underlay,Base,false,Why)||Base.Num()!=23126)
   return Fail(Why,TEXT("actual retained MotelV2 g4 native source differs"));
  for(const auto& T:Base)
  {
   FBox Box(ForceInit);for(const auto& C:T.C)Box+=Three(C)+FVector(24,0,-300);
   if(Box.Max.X < -Outer||Box.Min.X>Outer||Box.Max.Z<Z0||Box.Min.Z>FloorZ1)continue;
   if(T.C[0].UV[0]==T.C[1].UV[0]&&T.C[0].UV[0]==T.C[2].UV[0])return Fail(Why,TEXT("unexpected tuft in retained solid-ground cache"));
   for(const auto& C:T.C)UnderlayCorners.Add(Three(C)+FVector(24,0,-300));
  }
  if(UnderlayCorners.IsEmpty()||UnderlayCorners.Num()>512*3)return Fail(Why,TEXT("local retained-ground triangle count outside bounded cache"));
  for(int32 K=0;K<2;++K)if(!Source[K]||!Fixed[K]||Source[K]==Fixed[K]||Source[K]->GetPathName()!=Sources[K]||Fixed[K]->GetPathName()!=Targets[K]||
     !MaterialsSame(Source[K],Fixed[K])||Source[K]->HasValidNaniteData()||!Read(Source[K],Original[K],false,Why))return Fail(Why,TEXT("editor source/duplicate identity or native readback failed"));
  if(Original[0].Num()!=1296||Original[1].Num()!=12806)return Fail(Why,TEXT("expected current g664 / MotelV2 trimmed g663 triangle counts differ"));
  for(int32 K=0;K<2;++K)
  {
   if(!Make(Original[K],Original[0],K==1,Expected[K],Why))return false;
   if(!bVerifyOnly&&!Store(Source[K],Fixed[K],Expected[K],Why))return false;
   int32 FilledLeafCorners=0,FilledLeafNormals=0;
   if(!Read(Fixed[K],Actual[K],true,Why)||!VerifyOutput(Expected[K],Actual[K],Why,FilledLeafCorners,FilledLeafNormals))return false;
   R->SetNumberField(K==0?TEXT("road_filled_leaf_tangent_corners"):TEXT("ground_filled_leaf_tangent_corners"),FilledLeafCorners);
   R->SetNumberField(K==0?TEXT("road_filled_leaf_normal_corners"):TEXT("ground_filled_leaf_normal_corners"),FilledLeafNormals);
   if(!bVerifyOnly)
   {
    Fixed[K]->RemoveUserDataOfClass(US47SaroSeamProof::StaticClass());auto* P=NewObject<US47SaroSeamProof>(Fixed[K]);
    P->Source=Source[K];P->Revision=Revision;P->SourceTriangles=Original[K].Num();P->Triangles=Actual[K].Num();
    P->SourceMin=Source[K]->GetBoundingBox().Min;P->SourceMax=Source[K]->GetBoundingBox().Max;P->SurfaceSignature=Signature(Actual[K]);
    if(K==0){P->Underlay=Underlay;P->UnderlayTriangles=Base.Num();P->UnderlayCorners=UnderlayCorners;}
    Fixed[K]->AddAssetUserData(P);Fixed[K]->MarkPackageDirty();
   }
   if(!Candidate(Source[K],Fixed[K],K,Actual[K],Why))return false;
   if(K==0)
   {
    const auto* P=Cast<US47SaroSeamProof>(Fixed[0]->GetAssetUserDataOfClass(US47SaroSeamProof::StaticClass()));
    if(!P||P->Underlay!=Underlay||P->UnderlayTriangles!=Base.Num()||P->UnderlayCorners!=UnderlayCorners)
     return Fail(Why,TEXT("retained-ground proof differs from actual native source triangles"));
   }
   R->SetNumberField(K==0?TEXT("road_triangles"):TEXT("ground_triangles"),Actual[K].Num());
  }
  R->SetNumberField(TEXT("cached_underlay_triangles"),UnderlayCorners.Num()/3);return true;
 };Ok=Run();
#else
 Why=TEXT("derivative preparation is editor-only");
#endif
 R->SetStringField(TEXT("status"),Ok?TEXT("PASS"):TEXT("FAIL"));R->SetStringField(TEXT("error"),Why);
 FString Json;FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&Json));return Json;
}

bool S47SaroSeam::Build(AS47Web* W)
{
 if(!W||!W->IsLoaded())return false;
 for(auto It=Installed.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
 if(const auto* Old=Installed.Find(W))return Current(*Old,W);
 if(FParse::Param(FCommandLine::Get(),TEXT("S47SaroSeamOff")))
 {UE_LOG(LogTemp,Display,TEXT("S47: SARO road seam disabled; original render/collision/floors retained"));return false;}
 UStaticMeshComponent* C[2]={};UStaticMesh* Source[2]={};UStaticMesh* Mesh[2]={};TArray<UMaterialInterface*> Mats[2];
 FInstalled I;FString Why;
 auto Skip=[&](){UE_LOG(LogTemp,Warning,TEXT("S47: SARO road seam skipped; original render/collision/floors retained: %s"),*Why);return false;};
 const auto* Parent=W->Node(952);
 if(!Parent||Parent->Name!=TEXT("saroCorridor")||!Parent->P.Equals(FVector(-24,0,300),.0001)||!Parent->Comp.IsValid()||
    !Parent->Comp->GetComponentTransform().Equals(FTransform(FRotator::ZeroRotator,S47Three::ToUE(-24,0,300),FVector::OneVector),.0001)) {Why=TEXT("corridor parent");return Skip();}
 for(int32 K=0;K<2;++K)
 {
  const auto* N=W->Node(K==0?954:953);C[K]=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
  if(!N||N->Geo!=(K==0?664:663)||N->Parent!=952||N->bInstanced||N->Mats.Num()!=1||!C[K]||
     C[K]->GetAttachParent()!=Parent->Comp.Get()||!C[K]->GetRelativeTransform().Equals(FTransform::Identity,.0001)||
     C[K]->GetCollisionEnabled()!=ECollisionEnabled::QueryOnly||C[K]->GetCollisionResponseToChannel(ECC_Visibility)!=ECR_Block)
   {Why=TEXT("actual road/verge component identity or query policy");return Skip();}
  Source[K]=C[K]->GetStaticMesh();Mesh[K]=LoadObject<UStaticMesh>(nullptr,Targets[K]);TArray<FTriangle> Ts;
  if(!Candidate(Source[K],Mesh[K],K,Ts,Why))return Skip();
  I.C[K]=C[K];I.Mesh[K]=Mesh[K];I.Transform[K]=C[K]->GetComponentTransform();Mats[K]=C[K]->GetMaterials();
  for(const auto& T:Ts)if(FloorBand(T)){FFloor F;F.Part=K;for(int32 J=0;J<3;++J)F.P[J]=Three(T.C[J]);I.Floor.Add(F);}
 }
 if(I.Floor.Num()!=122){Why=TEXT("expected 90 road +32 inner-verge native floor triangles");return Skip();}
 const auto* BaseNode=W->Node(7);auto* BaseComp=BaseNode?Cast<UStaticMeshComponent>(BaseNode->Comp.Get()):nullptr;
 const auto* Proof=Cast<US47SaroSeamProof>(Mesh[0]->GetAssetUserDataOfClass(US47SaroSeamProof::StaticClass()));
 if(!BaseNode||BaseNode->Geo!=4||BaseNode->Parent!=6||!BaseComp||!Proof||!Proof->Underlay||
    BaseComp->GetStaticMesh()!=Proof->Underlay||Proof->Underlay->GetPathName()!=UnderlayPath||Proof->UnderlayTriangles!=Proof->Underlay->GetNumTriangles(0)||
    !BaseComp->GetComponentTransform().Equals(FTransform::Identity,.0001)||BaseComp->GetCollisionEnabled()!=ECollisionEnabled::QueryOnly||
    BaseComp->GetCollisionResponseToChannel(ECC_Visibility)!=ECR_Block||Proof->UnderlayCorners.IsEmpty()||Proof->UnderlayCorners.Num()%3||Proof->UnderlayCorners.Num()>512*3)
  {Why=TEXT("actual retained g4 underlay or local cached source differs");return Skip();}
 I.C[2]=BaseComp;I.Mesh[2]=BaseComp->GetStaticMesh();I.Transform[2]=BaseComp->GetComponentTransform();
 for(int32 J=0;J<Proof->UnderlayCorners.Num();J+=3)
 {
  FFloor F;F.Part=2;for(int32 K=0;K<3;++K){F.P[K]=Proof->UnderlayCorners[J+K];if(F.P[K].ContainsNaN()){Why=TEXT("nonfinite underlay proof");return Skip();}}
  I.Floor.Add(F);
 }
 // Both candidate assets and floors are ready before either existing component changes.
 // No tick/callback is yielded during these swaps. A failed collision check rolls back BOTH.
 for(int32 K=0;K<2;++K){C[K]->SetStaticMesh(Mesh[K]);for(int32 S=0;S<Mats[K].Num();++S)C[K]->SetMaterial(S,Mats[K][S]);}
 if(!Current(I,W)||!CollisionMatches(I,Why))
 {
  for(int32 K=0;K<2;++K){C[K]->SetStaticMesh(Source[K]);for(int32 S=0;S<Mats[K].Num();++S)C[K]->SetMaterial(S,Mats[K][S]);}
  return Skip();
 }
 const int32 Cached=I.Floor.Num();Installed.Add(W,MoveTemp(I));UE_LOG(LogTemp,Display,TEXT("S47: SARO road seam installed: 122 native road/verge + %d retained-ground triangles cached; new collision and scene-scoped floors active together"),Cached-122);
 if(FParse::Param(FCommandLine::Get(),TEXT("S47SaroSeamProbe")))
 {
  TArray<FString> Lines;const bool Ok=Audit(W,Lines);for(const auto& Line:Lines)UE_LOG(LogTemp,Display,TEXT("%s"),*Line);
  UE_LOG(LogTemp,Display,TEXT("S47: SARO road seam probe %s"),Ok?TEXT("PASS"):TEXT("FAIL"));
 }
 return true;
}

bool S47SaroSeam::Height(const AS47Web* W,double X,double Z,double& Y,bool* bRoad)
{
 if(!W||!FMath::IsFinite(X)||!FMath::IsFinite(Z)||Z<36||Z>100||FMath::Abs(X+24)>Outer)return false;
 const auto* I=Installed.Find(const_cast<AS47Web*>(W));return I&&Current(*I,W)&&FloorAt(I->Floor,X+24,Z-300,Y,bRoad);
}

bool S47SaroSeam::Audit(AS47Web* W,TArray<FString>& Lines)
{
 const auto* I=W?Installed.Find(W):nullptr;FString Why;bool Ok=I&&Current(*I,W)&&CollisionMatches(*I,Why);
 if(I)for(double Z:{36.,44.,52.,60.,68.,84.,100.})for(double X:{-3.6,0.,3.6})
 {
  double Y=0;bool Road=false;Ok&=Height(W,X-24,Z,Y,&Road)&&Road;
  if(Z==36)Ok&=FMath::Abs(Y+.58)<.00001;
 }
 Lines.Add(FString::Printf(TEXT("SARO road seam %s: actual native triangles/collision and scene-scoped floor; %s"),Ok?TEXT("PASS"):TEXT("FAIL"),*Why));return Ok;
}

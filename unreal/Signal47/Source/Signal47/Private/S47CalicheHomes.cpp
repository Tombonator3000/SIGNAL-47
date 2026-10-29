#include "S47CalicheHomes.h"
#include "S47Web.h"
#include "S47Three.h"
#include "S47RoadTerrain.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshResources.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47CalicheHomes,Log,All);
namespace
{
 using FJ=TSharedPtr<FJsonObject>;
 using FA=TArray<TSharedPtr<FJsonValue>>;
 FJ Obj(const FJ& J,const TCHAR* Key){const FJ* P=nullptr;return J&&J->TryGetObjectField(Key,P)?*P:FJ();}
 const FA* Arr(const FJ& J,const TCHAR* Key){const FA* P=nullptr;return J&&J->TryGetArrayField(Key,P)?P:nullptr;}
 FString Str(const FJ& J,const TCHAR* Key){FString V;if(J)J->TryGetStringField(Key,V);return V;}
 int32 Int(const FJ& J,const TCHAR* Key){double V=-1;if(!J||!J->TryGetNumberField(Key,V)||!FMath::IsFinite(V)||V!=FMath::FloorToDouble(V))return -1;return int32(V);}
 bool Flag(const FJ& J,const TCHAR* Key,bool Expected){bool V=!Expected;return J&&J->TryGetBoolField(Key,V)&&V==Expected;}
 bool Numbers(const FA* A,int32 N,TArray<double>& Out)
 {
  Out.Reset();if(!A||A->Num()!=N)return false;
  for(const auto& V:*A){double X=0;if(!V||!V->TryGetNumber(X)||!FMath::IsFinite(X))return false;Out.Add(X);}return true;
 }
 bool Vector(const FJ& J,const TCHAR* Key,FVector& Out)
 {
  TArray<double> A;if(!Numbers(Arr(J,Key),3,A))return false;Out=FVector(A[0],A[1],A[2]);return true;
 }
 bool EqualInts(const FA* A,const TArray<int32>& Expected)
 {
  TArray<double> Values;if(!Numbers(A,Expected.Num(),Values))return false;
  for(int32 I=0;I<Expected.Num();++I)if(Values[I]!=Expected[I])return false;return true;
 }
 FJ Json(const FString& Relative)
 {
  FString Text;FJ J;if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/Relative))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),J))return {};return J;
 }
 bool Fingerprint(const FString& Relative,const FString& Expected)
 {
  TArray<uint8> Data;if(Expected.Len()!=40||!FFileHelper::LoadFileToArray(Data,*(FPaths::ProjectContentDir()/Relative)))return false;
  FSHAHash Hash;FSHA1::HashBuffer(Data.GetData(),Data.Num(),Hash.Hash);return Hash.ToString().Equals(Expected,ESearchCase::IgnoreCase);
 }
 int32 Triangles(UStaticMesh* M){return M&&M->GetRenderData()&&M->GetRenderData()->LODResources.Num()?M->GetNumTriangles(0):-1;}
 bool Mesh(UStaticMesh* M,const FJ& R,int32 NumLods,bool bHouse)
 {
  const auto* Counts=Arr(R,TEXT("lod_triangles"));const auto* Materials=Arr(R,TEXT("materials"));const auto* B=Arr(R,TEXT("bounds_local_m"));
  if(!M||!M->GetRenderData()||M->GetRenderData()->LODResources.Num()!=NumLods||!Counts||Counts->Num()!=NumLods||!Materials||Materials->Num()!=M->GetStaticMaterials().Num()||!B||B->Num()!=2)return false;
  const FA *LowArray=nullptr,*HighArray=nullptr;TArray<double> Lo,Hi;
  if(!(*B)[0]->TryGetArray(LowArray)||!(*B)[1]->TryGetArray(HighArray)||!Numbers(LowArray,3,Lo)||!Numbers(HighArray,3,Hi))return false;
  const FVector Low(Lo[0],Lo[1],Lo[2]),High(Hi[0],Hi[1],Hi[2]);
  if(High.X<Low.X||High.Y<Low.Y||High.Z<Low.Z)return false;
  const FVector Center((Low.X+High.X)*50,-(Low.Y+High.Y)*50,(Low.Z+High.Z)*50);
  if(!M->GetBounds().Origin.Equals(Center,.3)||!M->GetBounds().BoxExtent.Equals((High-Low)*50,.3))return false;
  for(int32 L=0;L<NumLods;++L){double Count=0;if(!(*Counts)[L]->TryGetNumber(Count)||Count<=0||M->GetNumTriangles(L)!=Count)return false;}
  const auto* Assets=Arr(R,TEXT("material_assets"));if(bHouse&&(!Assets||Assets->Num()!=Materials->Num()))return false;
  for(int32 I=0;I<Materials->Num();++I)
  {
   FString Name;if(!(*Materials)[I]->TryGetString(Name)||!Name.StartsWith(TEXT("M_Homes_")))return false;
   const auto& Slot=M->GetStaticMaterials()[I];if(Slot.MaterialSlotName.ToString()!=Name||!Slot.MaterialInterface)return false;
   if(bHouse){FString Path;if(!(*Assets)[I]->TryGetString(Path)||Slot.MaterialInterface->GetPathName()!=Path||!Slot.MaterialInterface->GetName().StartsWith(TEXT("MI_Homes_")))return false;}
  }
  return true;
 }
 bool RectEquals(const FS47Rect& R,const double* B){return FMath::Abs(R.MinX-B[0])<1e-6&&FMath::Abs(R.MaxX-B[1])<1e-6&&FMath::Abs(R.MinZ-B[2])<1e-6&&FMath::Abs(R.MaxZ-B[3])<1e-6;}
 bool RectJson(const TSharedPtr<FJsonValue>& V,const double* B)
 {
  const FA* A=nullptr;TArray<double> Values;if(!V||!V->TryGetArray(A)||!Numbers(A,4,Values))return false;
  for(int32 I=0;I<4;++I)if(FMath::Abs(Values[I]-B[I])>1e-6)return false;return true;
 }
 void Retire(UStaticMeshComponent* C){C->SetVisibility(false,false);C->SetHiddenInGame(true,false);C->SetCastShadow(false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
 struct FExpectedPatch{int32 Geo,Node,Mat,Before,After;};
 constexpr FExpectedPatch ExpectedPatches[]={{999,1448,638,1404,1356},{1000,1449,641,132,108},{1001,1450,642,8,4},{1002,1451,639,332,140},{1003,1452,643,48,0},{1005,1454,645,120,0},{1006,1455,646,154,106},{1013,1462,652,120,0}};
 constexpr int32 ProtectedNodes[]={1436,1437,1438,1439,1440,1441,1442,1443,1444,1445,1446,1447,1534,1535,1536,1537,1538,1539,1546,1547,1551,1552,1553,1554,1563,1594,1595,1596};
}

TSharedPtr<FS47CalicheHomes> FS47CalicheHomes::Build(AS47Web* W,bool bTownReady)
{
 if(!W||FParse::Param(FCommandLine::Get(),TEXT("S47CalicheHomesOff"))||FParse::Param(FCommandLine::Get(),TEXT("S47PropsOff")))return {};
 auto Reject=[](const FString& Why){UE_LOG(LogS47CalicheHomes,Warning,TEXT("S47: Caliche homes rejected: %s; original homes, signs and road colliders retained"),*Why);};
 if(!bTownReady){Reject(TEXT("complete TB2 predecessor required"));return {};}
 const auto A=Json(TEXT("CalicheHomes/assembly.json"));const auto P=Obj(A,TEXT("parts")),C=Obj(A,TEXT("meshes")),S=Obj(A,TEXT("signs"));
 if(!A){UE_LOG(LogS47CalicheHomes,Display,TEXT("S47: Caliche homes inactive: no verified assembly; original homes and signs retained"));return {};}
 if(Int(A,TEXT("version"))!=1||Str(A,TEXT("source_revision"))!=TEXT("caliche-homes-20261029-v1")||Str(A,TEXT("status"))!=TEXT("STAGED_NATIVE_VERIFIED_RUNTIME_UNVERIFIED")||Int(A,TEXT("required_town_native_parts"))!=28||Int(A,TEXT("removed_house_total"))!=528||Int(A,TEXT("removed_sign_total"))!=76||!P||P->Values.Num()!=12||!C||C->Values.Num()!=8||!S||S->Values.Num()!=2){Reject(TEXT("independent 12/8/2 contract"));return {};}
 if(Str(A,TEXT("manifest_sha256"))!=TEXT("69fcf95db3adf8eff76274f59b03f94e662936a147f1ee7389630c1a8a0925c9")||!Fingerprint(TEXT("WebData/manifest.json"),Str(A,TEXT("manifest_sha1")))||!Fingerprint(TEXT("TownBuildings/town-patches.json"),Str(A,TEXT("town_patches_sha1")))){Reject(TEXT("original scene / composed TB2 fingerprint"));return {};}
 const int32 Parent=W->Handle(TEXT("world.town.group"));const auto* ParentNode=W->Node(Parent);
 if(Parent!=1431||W->Handle(TEXT("world.town.st"))!=1432||!ParentNode||!ParentNode->Comp.IsValid()||!W->WorldThree(Parent).Equals(FVector(8000,0,0),.00001)){Reject(TEXT("town parent"));return {};}
 auto F=MakeShared<FS47CalicheHomes>();F->Web=W;
 struct FReady{FString Name;UStaticMesh* Mesh=nullptr;FVector At;TArray<UMaterialInterface*> Materials;UStaticMeshComponent* Comp=nullptr;};
 struct FCut{int32 Node;UStaticMeshComponent* Comp;UStaticMesh* Mesh;TArray<UMaterialInterface*> Materials;};
 TArray<FReady> Ready;TArray<FCut> Cuts;
 const FVector Roots[]={FVector(8028.5,7.314197063446045,2133),FVector(8028.25,8.003066062927246,2156.5),FVector(8029.5,8.81874942779541,2185),FVector(7976.5,11.268553733825684,2241)};
 for(int32 H=0;H<4;++H)for(const TCHAR* Role:{TEXT("Shell"),TEXT("Roof"),TEXT("Fixtures")})
 {
  const FString Name=FString::Printf(TEXT("SM_Homes_H%02d_%s"),H+1,Role),Path=TEXT("/Game/CalicheHomes/")+Name+TEXT(".")+Name;const auto R=Obj(P,*Name);FVector At;
  auto* M=LoadObject<UStaticMesh>(nullptr,*Path);
  if(!R||Str(R,TEXT("asset"))!=Path||Str(R,TEXT("parent_handle"))!=TEXT("world.town.group")||!Vector(R,TEXT("site_world_three_m"),At)||!At.Equals(Roots[H],.00001)||!Mesh(M,R,3,true)){Reject(Name+TEXT(" native/material/LOD/pivot"));return {};}
  Ready.Add({Name,M,At,{},{}});
 }
 for(const auto& E:ExpectedPatches)
 {
  const FString Name=FString::Printf(TEXT("g%d"),E.Geo);const auto R=Obj(C,*Name);const auto* N=W->Node(E.Node);auto* Comp=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
  if(!R||Int(R,TEXT("node"))!=E.Node||Int(R,TEXT("geo"))!=E.Geo||Int(R,TEXT("parent"))!=1432||Int(R,TEXT("result_tris"))!=E.After||!EqualInts(Arr(R,TEXT("accepted_previous_tris")),{E.Before})||!EqualInts(Arr(R,TEXT("source_material_ids")),{E.Mat})||!N||N->Geo!=E.Geo||N->Parent!=1432||N->Mats!=TArray<int32>{E.Mat}||!Comp||Triangles(Comp->GetStaticMesh())!=E.Before||Comp->GetNumMaterials()!=1||!Comp->GetMaterial(0)){Reject(Name+TEXT(" exact composed predecessor"));return {};}
  if(E.After==0)
  {
   if(!Flag(R,TEXT("hidden"),true)||!Str(R,TEXT("asset")).IsEmpty()||!EqualInts(Arr(R,TEXT("original_material_indices")),{})){Reject(Name+TEXT(" empty trim policy"));return {};}
   F->Retired.Add(E.Node);continue;
  }
  const FString Path=TEXT("/Game/CalicheHomes/Trim/")+Name+TEXT(".")+Name;auto* M=LoadObject<UStaticMesh>(nullptr,*Path);
  const auto* Mats=Arr(R,TEXT("materials"));FString MatName;
  if(!Flag(R,TEXT("hidden"),false)||Str(R,TEXT("asset"))!=Path||!EqualInts(Arr(R,TEXT("original_material_indices")),{0})||!Mats||Mats->Num()!=1||!(*Mats)[0]->TryGetString(MatName)||MatName!=TEXT("M_Homes_Trim_")+Name+TEXT("_slot0")||!Mesh(M,R,1,false)||Triangles(M)!=E.After){Reject(Name+TEXT(" exact trim asset/slot"));return {};}
  Cuts.Add({E.Node,Comp,M,{Comp->GetMaterial(0)}});
 }
 for(int32 K=0;K<2;++K)
 {
  const FString Label=K==0?TEXT("North"):TEXT("South"),Name=TEXT("SM_Homes_Sign")+Label,Path=TEXT("/Game/CalicheHomes/")+Name+TEXT(".")+Name;
  const auto R=Obj(S,*Name);auto* M=LoadObject<UStaticMesh>(nullptr,*Path);FVector At;const auto* Ground=Arr(R,TEXT("ground_samples"));const auto* Mats=Arr(R,TEXT("materials"));
  const double X=K==0?-8.6:8.6,Z=K==0?1895.:2315.;
  if(!R||Str(R,TEXT("asset"))!=Path||Str(R,TEXT("parent_handle"))!=TEXT("world.town.group")||!Vector(R,TEXT("site_world_three_m"),At)||FMath::Abs(At.X-(8000+X))>.00001||FMath::Abs(At.Z-Z)>.00001||!EqualInts(Arr(R,TEXT("source_nodes")),{1448,1449,1450})||!EqualInts(Arr(R,TEXT("source_material_ids")),{638,641,642})||!EqualInts(Arr(R,TEXT("original_material_indices")),{0,0,0})||!Ground||Ground->Num()!=3||!Mats||Mats->Num()!=3||!Mesh(M,R,1,false)||Triangles(M)!=38){Reject(Name+TEXT(" source/three-slot/geometry"));return {};}
  for(int32 G=0;G<3;++G)
  {
   const auto Sample=(*Ground)[G]->AsObject();double SX=0,SY=0,SZ=0;const double DX=G==0?0:G==1?-1.4:1.4;
   if(!Sample||!Sample->TryGetNumberField(TEXT("x"),SX)||!Sample->TryGetNumberField(TEXT("y"),SY)||!Sample->TryGetNumberField(TEXT("z"),SZ)||!FMath::IsFinite(SX)||!FMath::IsFinite(SY)||!FMath::IsFinite(SZ)||FMath::Abs(SX-(X+DX))>.00001||FMath::Abs(SZ-Z)>.00001||FMath::Abs(S47RoadTerrain::SurfaceY(SX,SZ)-SY)>.002||(G==0&&FMath::Abs(At.Y-SY)>.00001)){Reject(Name+TEXT(" actual terrain contacts"));return {};}
  }
  TArray<UMaterialInterface*> Source;
  for(int32 Slot=0;Slot<3;++Slot)
  {
   FString NameSlot;const auto* N=W->Node(1448+Slot);auto* Comp=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
   if(!(*Mats)[Slot]->TryGetString(NameSlot)||NameSlot!=TEXT("M_Homes_Sign")+Label+FString::Printf(TEXT("_slot%d"),Slot)||!Comp||Comp->GetNumMaterials()!=1||!Comp->GetMaterial(0)){Reject(Name+TEXT(" original material slot0"));return {};}
   Source.Add(Comp->GetMaterial(0));
  }
  Ready.Add({Name,M,At,Source,{}});
 }
 // Kontroller alle beskyttede originaler, også levende paneler og proxyer.
 // Lagre bare identiteter. Story kan lovlig endre synlighet, transform og
 // materialparametere etter denne kildebevarende utskiftingen.
 const auto* Protected=Arr(A,TEXT("protected_nodes"));const auto* Handles=Arr(A,TEXT("protected_handles"));
 if(!Protected||!Handles||Protected->Num()<UE_ARRAY_COUNT(ProtectedNodes)||Handles->Num()!=10){Reject(TEXT("protected identity contract"));return {};}
 TSet<int32> Seen;
 for(const auto& V:*Protected)
 {
  const auto R=V->AsObject();const int32 I=Int(R,TEXT("i"));const auto* N=W->Node(I);
  const auto* Materials=Arr(R,TEXT("mats"));
  if(!N||Seen.Contains(I)||!N->Comp.IsValid()||N->Parent!=Int(R,TEXT("p"))||N->Geo!=Int(R,TEXT("geo"))||(Materials?!EqualInts(Materials,N->Mats):!N->Mats.IsEmpty())){Reject(TEXT("protected node identity"));return {};}
  Seen.Add(I);auto* Comp=Cast<UStaticMeshComponent>(N->Comp.Get());F->Protected.Add({I,N->Parent,N->Geo,N->Mats,N->Comp,Comp?Comp->GetStaticMesh():nullptr});
 }
 for(int32 I:ProtectedNodes)if(!Seen.Contains(I)){Reject(TEXT("protected node missing"));return {};}
 for(const auto& V:*Handles){FString Name;if(!V->TryGetString(Name)||!Seen.Contains(W->Handle(Name))){Reject(TEXT("live proxy/person handle"));return {};}}
 // g1007 er det opprinnelige mørke H02-vinduet, utenfor kuttet på 528 triangler.
 const auto* Glass=W->Node(1456);auto* GlassComp=Glass?Cast<UStaticMeshComponent>(Glass->Comp.Get()):nullptr;
 if(!Glass||Glass->Geo!=1007||Glass->Parent!=1432||Glass->Mats!=TArray<int32>{647}||!GlassComp||Triangles(GlassComp->GetStaticMesh())!=2){Reject(TEXT("retained H02 dark glass"));return {};}
 F->Protected.Add({1456,Glass->Parent,Glass->Geo,Glass->Mats,Glass->Comp,GlassComp->GetStaticMesh()});
 const auto Road=Obj(Obj(W->DriveData(),TEXT("legs")),TEXT("road"));const auto* Obstacles=Arr(Road,TEXT("obstacles"));const auto* MoveRows=Arr(A,TEXT("collider_moves"));
 const auto& Walk=W->CollidersMutable(TEXT("road"));
 if(!Road||!Obstacles||!MoveRows||MoveRows->Num()!=4){Reject(TEXT("paired road/drive collider data"));return {};}
 FA NewObstacles=*Obstacles;TSet<int32> SeenWalk,SeenDrive;
 for(int32 I=0;I<4;++I)
 {
  const auto R=(*MoveRows)[I]->AsObject();TArray<double> Old,At;
  if(!Numbers(Arr(R,TEXT("old")),4,Old)||!Numbers(Arr(R,TEXT("new")),4,At)){Reject(TEXT("collider array"));return {};}
  const double CX=I<2?7990.+(I%2)*2.8:8007.2+(I%2)*2.8,OldZ=I<2?1880:2335,NewZ=I<2?1895:2315;
  const double ExpectedOld[]={CX-.15,CX+.15,OldZ-.15,OldZ+.15},ExpectedNew[]={CX-.15,CX+.15,NewZ-.15,NewZ+.15};
  FMovedCollider Move;int32 WalkMatches=0,DriveMatches=0;
  for(int32 J=0;J<4;++J){if(FMath::Abs(Old[J]-ExpectedOld[J])>1e-6||FMath::Abs(At[J]-ExpectedNew[J])>1e-6){Reject(TEXT("exact sign post bounds"));return {};}Move.Old[J]=Old[J];Move.At[J]=At[J];}
  for(int32 J=0;J<Walk.Num();++J)if(RectEquals(Walk[J],Move.Old)&&Walk[J].bEnabled){Move.Walk=J;++WalkMatches;}
  for(int32 J=0;J<Obstacles->Num();++J)if(RectJson((*Obstacles)[J],Move.Old)){Move.Drive=J;++DriveMatches;}
  if(WalkMatches!=1||DriveMatches!=1||SeenWalk.Contains(Move.Walk)||SeenDrive.Contains(Move.Drive)){Reject(TEXT("unique paired post colliders"));return {};}
  SeenWalk.Add(Move.Walk);SeenDrive.Add(Move.Drive);F->Moves.Add(Move);
  FA NewBox;for(double Value:At)NewBox.Add(MakeShared<FJsonValueNumber>(Value));NewObstacles[Move.Drive]=MakeShared<FJsonValueArray>(NewBox);
 }
 // Klargjør uregistrerte komponenter etter samlet asset-/kilde-/kollisjonskontroll.
 // Ingen original tegneflate eller kollisjon endres før alle er klargjort.
 for(auto& R:Ready)
 {
  R.Comp=NewObject<UStaticMeshComponent>(W);if(!R.Comp){for(auto& Q:Ready)if(Q.Comp)Q.Comp->DestroyComponent();Reject(TEXT("component preparation"));return {};}
  R.Comp->SetupAttachment(W->Node(Parent)->Comp.Get());R.Comp->SetMobility(EComponentMobility::Movable);R.Comp->SetStaticMesh(R.Mesh);
  for(int32 Slot=0;Slot<R.Materials.Num();++Slot)R.Comp->SetMaterial(Slot,R.Materials[Slot]);
  R.Comp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);R.Comp->SetCollisionResponseToAllChannels(ECR_Ignore);R.Comp->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);R.Comp->SetCastShadow(true);R.Comp->SetVisibleInRayTracing(true);
 }
 // Aktiver ett samlet sett før US47World::Init/EnsureLegs. Kopien av
 // kjørehindringene får dermed de fire stolpene på deres nye plass.
 for(auto& R:Ready)
 {
  R.Comp->RegisterComponent();R.Comp->SetWorldLocation(S47Three::ToUE(R.At));R.Comp->SetWorldRotation(FRotator::ZeroRotator);
  const int32 I=W->AdoptNode(R.Comp,Parent,TEXT("caliche-homes:")+R.Name);F->Installed.Add({I,Triangles(R.Mesh),R.Comp,R.Mesh});
  if(R.Name.StartsWith(TEXT("SM_Homes_Sign")))++F->SignParts;else ++F->HouseParts;
 }
 for(const auto& Cut:Cuts)
 {
  Cut.Comp->SetStaticMesh(Cut.Mesh);for(int32 I=0;I<Cut.Materials.Num();++I)Cut.Comp->SetMaterial(I,Cut.Materials[I]);
  F->Installed.Add({Cut.Node,Triangles(Cut.Mesh),Cut.Comp,Cut.Mesh});++F->PatchParts;
 }
 for(int32 I:F->Retired)if(const auto* N=W->Node(I))if(auto* Comp=Cast<UStaticMeshComponent>(N->Comp.Get()))Retire(Comp);
 auto& MutableWalk=W->CollidersMutable(TEXT("road"));
 for(const auto& Move:F->Moves){auto& R=MutableWalk[Move.Walk];R.MinX=Move.At[0];R.MaxX=Move.At[1];R.MinZ=Move.At[2];R.MaxZ=Move.At[3];}
 Road->SetArrayField(TEXT("obstacles"),NewObstacles);
 F->Update();UE_LOG(LogS47CalicheHomes,Display,TEXT("S47: Caliche homes ready: 4 houses / 12 native parts, 528 house triangles replaced, 4 stoops + H02 glass retained; 2 original signs at z1895/2315, 4 paired road/drive colliders moved"));return F;
}

void FS47CalicheHomes::Update()
{
 if(auto* W=Web.Get())for(int32 I:Retired)if(const auto* N=W->Node(I))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))Retire(C);
}

bool FS47CalicheHomes::Audit(TArray<FString>& Lines) const
{
 auto* W=Web.Get();bool Ok=W&&HouseParts==12&&SignParts==2&&PatchParts==5&&Installed.Num()==19&&Retired.Num()==3&&Moves.Num()==4;
 for(const auto& R:Installed)
 {
  const auto* N=W?W->Node(R.Node):nullptr;auto* C=R.Comp.Get();
  Ok&=N&&C&&N->Comp.Get()==C&&C->GetStaticMesh()==R.Mesh.Get()&&Triangles(C->GetStaticMesh())==R.Tris;
 }
 for(const auto& R:Protected)
 {
  const auto* N=W?W->Node(R.Node):nullptr;auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
  Ok&=N&&N->Comp==R.Comp&&N->Parent==R.Parent&&N->Geo==R.Geo&&N->Mats==R.Mats&&(!R.Mesh.IsValid()||(C&&C->GetStaticMesh()==R.Mesh.Get()));
 }
 for(int32 I:Retired){const auto* N=W?W->Node(I):nullptr;auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;Ok&=C&&!C->IsVisible()&&C->bHiddenInGame&&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision;}
 if(W)
 {
  const auto& Walk=W->CollidersMutable(TEXT("road"));const auto* Obstacles=Arr(Obj(Obj(W->DriveData(),TEXT("legs")),TEXT("road")),TEXT("obstacles"));
  for(const auto& Move:Moves)Ok&=Walk.IsValidIndex(Move.Walk)&&Walk[Move.Walk].bEnabled&&RectEquals(Walk[Move.Walk],Move.At)&&Obstacles&&Obstacles->IsValidIndex(Move.Drive)&&RectJson((*Obstacles)[Move.Drive],Move.At);
 }
 Lines.Add(FString::Printf(TEXT("Caliche homes %s houses=%d/12 signs=%d/2 trims=%d/5 retired=%d/3 protected=%d paired-posts=%d/4"),Ok?TEXT("PASS"):TEXT("FAIL"),HouseParts,SignParts,PatchParts,Retired.Num(),Protected.Num(),Moves.Num()));return Ok;
}

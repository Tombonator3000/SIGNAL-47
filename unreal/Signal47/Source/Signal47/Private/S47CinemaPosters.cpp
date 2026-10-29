// ART01: additive exterior prints. Original TB2 geometry/materials and Closed interaction stay authoritative.
#include "S47CinemaPosters.h"
#include "S47Web.h"
#include "S47Three.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstance.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshResources.h"
#if WITH_EDITOR
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UObject/Package.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogS47CinemaPosters, Log, All);
namespace
{
 using FJ=TSharedPtr<FJsonObject>;
 using FA=TArray<TSharedPtr<FJsonValue>>;
 const TCHAR* SpecSha=TEXT("27fd6475b28a984464ea3f9db358268dd790ee9f37d891515fed25f9594efece");
 const TCHAR* RootPath=TEXT("/Game/Signal47/Art/CinemaPosters/");
 const TCHAR* MasterPath=TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque");
 const FVector Site(7985,8.987393474578857,2162);
 constexpr double ArtWidth=1.61*2./3.,ArtHeight=1.61,Lift=.001,Tolerance=.00025;
 struct FExpected {const TCHAR* Id;const TCHAR* Texture;const TCHAR* Material;double Z;};
 const FExpected Expected[]={
  {TEXT("desert-sky"),TEXT("T_Cinema_DesertSky"),TEXT("MI_Cinema_DesertSky"),7},
  {TEXT("friday-matinee"),TEXT("T_Cinema_FridayMatinee"),TEXT("MI_Cinema_FridayMatinee"),-7}
 };
 FJ Obj(const FJ& J,const TCHAR* Key){const FJ* O=nullptr;return J&&J->TryGetObjectField(Key,O)?*O:FJ();}
 const FA* Arr(const FJ& J,const TCHAR* Key){const FA* A=nullptr;return J&&J->TryGetArrayField(Key,A)?A:nullptr;}
 FString Str(const FJ& J,const TCHAR* Key){FString S;if(J)J->TryGetStringField(Key,S);return S;}
 bool Number(const FJ& J,const TCHAR* Key,double& Out){return J&&J->TryGetNumberField(Key,Out)&&FMath::IsFinite(Out);}
 bool Vector(const FJ& J,const TCHAR* Key,FVector& Out)
 {
  const FA* A=Arr(J,Key);double V[3];if(!A||A->Num()!=3)return false;
  for(int32 I=0;I<3;++I)if(!(*A)[I]->TryGetNumber(V[I])||!FMath::IsFinite(V[I]))return false;
  Out=FVector(V[0],V[1],V[2]);return true;
 }
 FA Values(std::initializer_list<double> ValuesIn){FA A;for(double V:ValuesIn)A.Add(MakeShared<FJsonValueNumber>(V));return A;}
 FA VectorJson(const FVector& V){return Values({V.X,V.Y,V.Z});}
 FVector DirectionThree(const FVector& UE){return FVector(UE.X,UE.Z,UE.Y);}
 FString Asset(const TCHAR* Name){return FString(RootPath)+Name+TEXT(".")+Name;}
 FJ Json()
 {
  FString Text;FJ J;
  if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("WebData/cinema-posters-binding.json")))
   ||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),J))return {};
  return J;
 }
 int32 UniqueNode(AS47Web* W,const TCHAR* Name)
 {
  int32 Found=-1;
  for(int32 I=0;I<W->PartCount();++I)if(const auto* N=W->Node(I))if(N->Name==Name){if(Found>=0)return -1;Found=I;}
  return Found;
 }
 int32 NamedSlot(UStaticMesh* Mesh,const TCHAR* Name)
 {
  int32 Found=-1;if(!Mesh)return -1;
  for(int32 I=0;I<Mesh->GetStaticMaterials().Num();++I)if(Mesh->GetStaticMaterials()[I].MaterialSlotName==FName(Name)){if(Found>=0)return -1;Found=I;}
  return Found;
 }
 bool Counts(UStaticMesh* Mesh,const FJ& Record)
 {
  const FA* A=Arr(Record,TEXT("lod_triangles"));
  if(!Mesh||!Mesh->GetRenderData()||Mesh->GetRenderData()->LODResources.Num()!=3||!A||A->Num()!=3)return false;
  for(int32 I=0;I<3;++I){double N=0;if(!(*A)[I]->TryGetNumber(N)||N<=0||Mesh->GetNumTriangles(I)!=N)return false;}
  return true;
 }
 bool OriginalMeasurement(const FJ& Record,const TCHAR* Id,bool bGlass,FVector& Centre,FVector2D& Size)
 {
  const auto Surfaces=Obj(Record,TEXT("surfaces"));const FA* Lods=Arr(Surfaces,Id);
  if(!Lods||Lods->Num()!=3)return false;
  for(int32 L=0;L<3;++L)
  {
   const auto J=(*Lods)[L]->AsObject();FVector C;double N=0,Lod=-1;const FA* S=Arr(J,TEXT("size_m"));
   double Width=0,Height=0;
   if(!Vector(J,TEXT("centre_local_three_m"),C)||!Number(J,TEXT("triangles"),N)||N!=2
    ||!Number(J,TEXT("lod"),Lod)||Lod!=L||!S||S->Num()!=2
    ||!(*S)[0]->TryGetNumber(Width)||!(*S)[1]->TryGetNumber(Height))return false;
   const FExpected* E=nullptr;for(const auto& Candidate:Expected)if(FString(Id)==Candidate.Id)E=&Candidate;
   if(!E||!C.Equals(FVector(bGlass?.25:.162,1.51,E->Z),Tolerance)
    ||!FMath::IsFinite(Width)||!FMath::IsFinite(Height)
    ||FMath::Abs(Width-(bGlass?1.16:1.13))>Tolerance||FMath::Abs(Height-(bGlass?1.64:1.61))>Tolerance
    ||Str(J,TEXT("triangle_payload_sha256")).Len()!=64)return false;
   const auto MaterialBinding=Obj(J,TEXT("material_binding"));const FA* Groups=Arr(MaterialBinding,TEXT("source_groups"));
   const FA* Sections=Arr(MaterialBinding,TEXT("matched_render_sections"));double Slot=-1,SourceTriangles=0,RenderTriangles=0;
   if(!Number(Record,TEXT("material_slot"),Slot)||!Groups||Groups->IsEmpty()||!Sections||Sections->IsEmpty()
    ||!Number(MaterialBinding,TEXT("source_material_triangles"),SourceTriangles)
    ||!Number(MaterialBinding,TEXT("render_material_triangles"),RenderTriangles)||SourceTriangles<2||SourceTriangles!=RenderTriangles)return false;
   for(const auto& Value:*Groups)
   {
    const auto Group=Value->AsObject();double Resolved=-1;const FA* Matches=Arr(Group,TEXT("matching_global_imported_slots"));double Match=-1;
    if(Str(Group,TEXT("imported_material_slot_name"))!=Str(Record,TEXT("material_slot_name"))
     ||!Number(Group,TEXT("resolved_material_slot"),Resolved)||Resolved!=Slot||!Matches||Matches->Num()!=1
     ||!(*Matches)[0]->TryGetNumber(Match)||Match!=Slot
     ||Str(Obj(Group,TEXT("resolved_material")),TEXT("material_asset"))!=Str(Record,TEXT("material_asset")))return false;
   }
   for(const auto& Value:*Sections)
   {
    const auto Section=Value->AsObject();double ActualSlot=-1;
    if(!Number(Section,TEXT("render_material_slot"),ActualSlot)||ActualSlot!=Slot
     ||Str(Obj(Section,TEXT("render_material")),TEXT("material_asset"))!=Str(Record,TEXT("material_asset")))return false;
   }
   if(L==0){Centre=C;Size=FVector2D(Width,Height);}
  }
  return true;
 }
 bool Plane(UStaticMesh* Mesh,const FJ& Output,const FA* SavedTriangles)
 {
  bool Ok=Mesh&&Mesh->bAllowCPUAccess&&Mesh->GetRenderData()&&Mesh->GetRenderData()->LODResources.Num()==1
   &&Mesh->GetNumTriangles(0)==2&&Mesh->GetStaticMaterials().Num()==1;
  FA Rows;
  if(Ok)
  {
   const auto& L=Mesh->GetRenderData()->LODResources[0];const auto& P=L.VertexBuffers.PositionVertexBuffer;
   const auto& V=L.VertexBuffers.StaticMeshVertexBuffer;
   Ok=P.GetVertexData()&&V.GetTangentData()&&V.GetTexCoordData()&&V.GetNumTexCoords()>=1
    &&P.GetNumVertices()==V.GetNumVertices()&&L.IndexBuffer.GetIndexDataSize()>0;
   TSet<FIntPoint> Corners;
   if(Ok)for(uint32 I=0;I<P.GetNumVertices();++I)
   {
    const FVector Point(P.VertexPosition(I)),Normal(V.VertexTangentZ(I));const FVector2D UV(V.GetVertexUV(I,0));
    Ok&=FMath::Abs(FMath::Abs(Point.X)-50)<.0001&&FMath::Abs(FMath::Abs(Point.Y)-50)<.0001
     &&FMath::Abs(Point.Z)<.0001&&Normal.Equals(FVector::UpVector,.0001)
     &&FMath::Abs(UV.X-(Point.X+50)/100)<.00001&&FMath::Abs(UV.Y-(Point.Y+50)/100)<.00001;
    Corners.Add(FIntPoint(Point.X>0,Point.Y>0));
    auto Row=MakeShared<FJsonObject>();Row->SetArrayField(TEXT("position_ue_cm"),VectorJson(Point));
    Row->SetArrayField(TEXT("normal_ue"),VectorJson(Normal));Row->SetArrayField(TEXT("uv0"),Values({UV.X,UV.Y}));Rows.Add(MakeShared<FJsonValueObject>(Row));
   }
   TArray<uint32> Indices;if(Ok)L.IndexBuffer.GetCopy(Indices);
   Ok&=Corners.Num()==4&&Indices.Num()==6;
   for(uint32 I:Indices)Ok&=I<P.GetNumVertices();
   // Match oriented triangles, including UVs, against the actually measured saved
   // Engine-copy description. Triangle order/cyclic start may change in the build.
   Ok&=SavedTriangles&&SavedTriangles->Num()==2;
   TSet<int32> Matched;
   if(Ok)for(int32 T=0;T<2;++T)
   {
    int32 Found=-1;
    for(int32 S=0;S<2;++S)if(!Matched.Contains(S))
    {
     const auto Saved=(*SavedTriangles)[S]->AsObject();const FA* Positions=Arr(Saved,TEXT("positions_ue_cm"));const FA* UVs=Arr(Saved,TEXT("uv0"));
     if(!Positions||Positions->Num()!=3||!UVs||UVs->Num()!=3)continue;
     for(int32 Start=0;Start<3;++Start)
     {
      bool Same=true;
      for(int32 Corner=0;Corner<3;++Corner)
      {
       const int32 Source=(Corner+Start)%3;const FA* Point=nullptr;const FA* UV=nullptr;
       if(!(*Positions)[Source]->TryGetArray(Point)||Point->Num()!=3||!(*UVs)[Source]->TryGetArray(UV)||UV->Num()!=2){Same=false;break;}
       double Q[3]={},R[2]={};
       for(int32 K=0;K<3;++K)Same&=(*Point)[K]->TryGetNumber(Q[K])&&FMath::IsFinite(Q[K]);
       for(int32 K=0;K<2;++K)Same&=(*UV)[K]->TryGetNumber(R[K])&&FMath::IsFinite(R[K]);
       if(!Same)break;
       const uint32 Vertex=Indices[T*3+Corner];const FVector Actual(P.VertexPosition(Vertex));const FVector2D ActualUV(V.GetVertexUV(Vertex,0));
       Same&=Actual.Equals(FVector(Q[0],Q[1],Q[2]),.0001)&&ActualUV.Equals(FVector2D(R[0],R[1]),.00001);
      }
      if(Same){Found=S;break;}
     }
     if(Found>=0)break;
    }
    Ok&=Found>=0;if(Found>=0)Matched.Add(Found);
   }
  }
  if(Output){Output->SetBoolField(TEXT("live_cpu_uv_valid"),Ok);Output->SetArrayField(TEXT("actual_plane_vertices"),Rows);}
  return Ok;
 }
 bool Material(UMaterialInterface* Material,UTexture2D* Texture,const FExpected& E,const FJ& Output)
 {
  const auto* Instance=Cast<UMaterialInstance>(Material);UTexture* Map=nullptr;
  bool Ok=Instance&&Texture&&Instance->GetPathName()==Asset(E.Material)&&Texture->GetPathName()==Asset(E.Texture)
   &&Instance->Parent&&Instance->Parent->GetPathName()==MasterPath
   &&Instance->GetTextureParameterValue(FMaterialParameterInfo(TEXT("Map")),Map)&&Map==Texture
   &&Texture->SRGB&&Texture->GetSizeX()>=1024&&Texture->GetSizeY()>=1024&&Texture->GetSizeX()<=2048&&Texture->GetSizeY()<=2048;
  auto Scalars=MakeShared<FJsonObject>();auto Vectors=MakeShared<FJsonObject>();
  const TPair<const TCHAR*,float> Required[]={
   {TEXT("NativeLighting"),1},{TEXT("UseMap"),1},{TEXT("MapIsLinear"),0},{TEXT("Roughness"),.88f},
   {TEXT("Metalness"),0},{TEXT("EmissivePower"),0},{TEXT("UseVertexColor"),0},{TEXT("UseInstanceColor"),0}
  };
  for(const auto& P:Required)
  {
   float Actual=-100;const bool Have=Material&&Material->GetScalarParameterValue(FMaterialParameterInfo(P.Key),Actual);
   Ok&=Have&&FMath::IsNearlyEqual(Actual,P.Value,.00001f);Scalars->SetNumberField(P.Key,Actual);
  }
  const TPair<const TCHAR*,FLinearColor> VectorValues[]={
   {TEXT("Color"),FLinearColor::White},{TEXT("MapU"),FLinearColor(1,0,0,0)},{TEXT("MapV"),FLinearColor(0,1,0,0)}
  };
  for(const auto& P:VectorValues)
  {
   FLinearColor Actual=FLinearColor::Transparent;const bool Have=Material&&Material->GetVectorParameterValue(FMaterialParameterInfo(P.Key),Actual);
   Ok&=Have&&Actual.Equals(P.Value,.00001f);Vectors->SetArrayField(P.Key,Values({Actual.R,Actual.G,Actual.B,Actual.A}));
  }
  if(Output)
  {
   Output->SetStringField(TEXT("actual_material_slot0"),GetPathNameSafe(Material));Output->SetStringField(TEXT("actual_Map"),GetPathNameSafe(Map));
   Output->SetStringField(TEXT("actual_material_parent"),Instance?GetPathNameSafe(Instance->Parent):TEXT("none"));
   Output->SetObjectField(TEXT("actual_scalars"),Scalars);Output->SetObjectField(TEXT("actual_vectors"),Vectors);
   Output->SetArrayField(TEXT("actual_texture_size_px"),Values({Texture?Texture->GetSizeX():0,Texture?Texture->GetSizeY():0}));
   Output->SetBoolField(TEXT("actual_texture_srgb"),Texture&&Texture->SRGB);
   Output->SetBoolField(TEXT("actual_material_valid"),Ok);
  }
  return Ok;
 }
}

#if WITH_EDITOR
namespace
{
 // Editor-only readback for data not reflected by UStaticMeshDescription. UE5.8's
 // mesh builder resolves PolygonGroup's ImportedMaterialSlotName into a global
 // slot; source group IDs and compact render-section indices are independent.
 void ReadSavedMaterials(const TArray<FString>& Args)
 {
  if(Args.Num()!=2)
  {UE_LOG(LogS47CinemaPosters,Error,TEXT("S47.Cinema.ReadSavedMaterials requires exact mesh object path and fresh absolute report path"));return;}
  FString Output=FPaths::ConvertRelativePathToFull(Args[1]);FPaths::NormalizeFilename(Output);FPaths::CollapseRelativeDirectories(Output);
  const FString Temporary=Output+TEXT(".tmp");
  const bool Owned=Args[0]==TEXT("/Game/TownBuildings/SM_Town_Cinema_Shell.SM_Town_Cinema_Shell")
   ||Args[0]==TEXT("/Game/TownBuildings/SM_Town_Cinema_Glass.SM_Town_Cinema_Glass");
  if(!Owned||FPaths::IsRelative(Args[1])||FPaths::IsUnderDirectory(Output,FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()))
   ||FPaths::IsUnderDirectory(Output,FPaths::ConvertRelativePathToFull(FPaths::EngineContentDir()))
   ||IFileManager::Get().FileExists(*Output)||IFileManager::Get().FileExists(*Temporary))
  {UE_LOG(LogS47CinemaPosters,Error,TEXT("S47 cinema saved-material reader rejected asset/report input; no existing file is replaced"));return;}
  auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("asset"),Args[0]);Report->SetStringField(TEXT("reader_contract_sha256"),SpecSha);
  Report->SetStringField(TEXT("method"),TEXT("read-only saved FMeshDescription PolygonGroup ImportedMaterialSlotName; global imported/display slot names; actual render sections and SectionInfoMap"));
  const UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*Args[0]);bool Ok=Mesh&&Mesh->GetRenderData();
  const bool DirtyBefore=Mesh&&Mesh->GetPackage()->IsDirty();FA Slots,Lods;
  auto Slot=[&](int32 Index)
  {
   auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("index"),Index);
   const bool Valid=Mesh&&Mesh->GetStaticMaterials().IsValidIndex(Index);Row->SetBoolField(TEXT("valid"),Valid);
   if(Valid)
   {
    const auto& Material=Mesh->GetStaticMaterials()[Index];
    Row->SetStringField(TEXT("display_name"),Material.MaterialSlotName.ToString());Row->SetStringField(TEXT("imported_name"),Material.ImportedMaterialSlotName.ToString());
    Row->SetStringField(TEXT("material_asset"),GetPathNameSafe(Material.MaterialInterface.Get()));
   }
   return Row;
  };
  if(Mesh)for(int32 I=0;I<Mesh->GetStaticMaterials().Num();++I)Slots.Add(MakeShared<FJsonValueObject>(Slot(I)));
  if(Ok)
  {
   const auto* Render=Mesh->GetRenderData();const auto& SectionMap=Mesh->GetSectionInfoMap();
   Report->SetNumberField(TEXT("source_lod_count"),Mesh->GetNumSourceModels());Report->SetNumberField(TEXT("render_lod_count"),Render->LODResources.Num());
   Ok&=Mesh->GetNumSourceModels()==Render->LODResources.Num();
   for(int32 L=0;L<Mesh->GetNumSourceModels();++L)
   {
    auto Row=MakeShared<FJsonObject>();Row->SetNumberField(TEXT("lod"),L);FA Groups,Sections;
    const FMeshDescription* Description=Mesh->GetMeshDescription(L);
    bool Complete=Description&&Render->LODResources.IsValidIndex(L);Row->SetBoolField(TEXT("saved_description_present"),Description!=nullptr);
    if(Description)
    {
     Row->SetNumberField(TEXT("saved_triangle_count"),Description->Triangles().Num());
     const FStaticMeshConstAttributes Attributes(*Description);const auto Names=Attributes.GetPolygonGroupMaterialSlotNames();
     Complete&=Names.IsValid();int32 GroupOrder=0;
     if(Names.IsValid())for(const FPolygonGroupID Id:Description->PolygonGroups().GetElementIDs())
     {
      const FName Name=Names[Id];const int32 Resolved=Mesh->GetMaterialIndexFromImportedMaterialSlotName(Name);
      auto Group=MakeShared<FJsonObject>();Group->SetNumberField(TEXT("polygon_group_id"),Id.GetValue());Group->SetNumberField(TEXT("source_iteration_index"),GroupOrder++);
      Group->SetStringField(TEXT("imported_material_slot_name"),Name.ToString());Group->SetNumberField(TEXT("triangles"),Description->GetNumPolygonGroupTriangles(Id));
      Group->SetNumberField(TEXT("resolved_material_slot"),Resolved);Group->SetObjectField(TEXT("resolved_material"),Slot(Resolved));
      FA MatchingSlots;
      for(int32 S=0;S<Mesh->GetStaticMaterials().Num();++S)if(Mesh->GetStaticMaterials()[S].ImportedMaterialSlotName==Name)MatchingSlots.Add(MakeShared<FJsonValueNumber>(S));
      Group->SetArrayField(TEXT("matching_global_imported_slots"),MatchingSlots);Groups.Add(MakeShared<FJsonValueObject>(Group));
     }
    }
    if(Render->LODResources.IsValidIndex(L))
    {
     const auto& Resources=Render->LODResources[L];Row->SetNumberField(TEXT("render_triangle_count"),Mesh->GetNumTriangles(L));
     for(int32 S=0;S<Resources.Sections.Num();++S)
     {
      const auto& Source=Resources.Sections[S];const auto Info=SectionMap.Get(L,S);auto Section=MakeShared<FJsonObject>();
      Section->SetNumberField(TEXT("section_index"),S);Section->SetNumberField(TEXT("render_material_slot"),Source.MaterialIndex);
      Section->SetObjectField(TEXT("render_material"),Slot(Source.MaterialIndex));Section->SetNumberField(TEXT("triangles"),Source.NumTriangles);
      Section->SetNumberField(TEXT("first_index"),Source.FirstIndex);Section->SetBoolField(TEXT("section_info_explicit"),SectionMap.IsValidSection(L,S));
      Section->SetNumberField(TEXT("section_info_material_slot"),Info.MaterialIndex);Section->SetObjectField(TEXT("section_info_material"),Slot(Info.MaterialIndex));
      Section->SetBoolField(TEXT("section_info_collision"),Info.bEnableCollision);Section->SetBoolField(TEXT("section_info_cast_shadow"),Info.bCastShadow);
      Sections.Add(MakeShared<FJsonValueObject>(Section));
     }
    }
    Row->SetArrayField(TEXT("polygon_groups"),Groups);Row->SetArrayField(TEXT("render_sections"),Sections);
    Row->SetBoolField(TEXT("complete"),Complete);Lods.Add(MakeShared<FJsonValueObject>(Row));Ok&=Complete;
   }
  }
  const bool DirtyAfter=Mesh&&Mesh->GetPackage()->IsDirty();Ok&=DirtyBefore==DirtyAfter;
  Report->SetBoolField(TEXT("package_dirty_before"),DirtyBefore);Report->SetBoolField(TEXT("package_dirty_after"),DirtyAfter);
  Report->SetArrayField(TEXT("global_material_slots"),Slots);Report->SetArrayField(TEXT("lods"),Lods);Report->SetStringField(TEXT("status"),Ok?TEXT("PASS"):TEXT("FAIL"));
  FString Text;const bool Serialized=FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Text));
  const bool Written=Serialized&&IFileManager::Get().MakeDirectory(*FPaths::GetPath(Output),true)
   &&FFileHelper::SaveStringToFile(Text,*Temporary)&&IFileManager::Get().Move(*Output,*Temporary,false,false,false,true);
  UE_LOG(LogS47CinemaPosters,Display,TEXT("S47 cinema saved-material reader %s asset=%s report=%s; no asset/save/setter"),Ok&&Written?TEXT("PASS"):TEXT("FAIL"),*Args[0],*Output);
 }
 FAutoConsoleCommand ReadSavedMaterialsCommand(TEXT("S47.Cinema.ReadSavedMaterials"),
  TEXT("Read only: exact cinema mesh object path and fresh report path; original assets are never saved."),
  FConsoleCommandWithArgsDelegate::CreateStatic(&ReadSavedMaterials));
}
#endif

TSharedPtr<FS47CinemaPosters> FS47CinemaPosters::Build(AS47Web* W,bool bTownReady)
{
 if(!W||FParse::Param(FCommandLine::Get(),TEXT("S47CinemaPostersOff"))||FParse::Param(FCommandLine::Get(),TEXT("S47PropsOff")))return {};
 auto Reject=[](const FString& Why){UE_LOG(LogS47CinemaPosters,Warning,TEXT("S47: cinema posters inactive: %s; all original prints/glass/materials/Closed proxy retained"),*Why);};
 if(!bTownReady){Reject(TEXT("complete TB2 required"));return {};}
 const auto BindingData=Json();const auto Originals=Obj(BindingData,TEXT("originals"));const FA* Surfaces=Arr(BindingData,TEXT("surfaces"));
 double Version=-1;FVector BoundSite;
 bool FreshReader=false;const auto NativeAssets=Obj(BindingData,TEXT("native_assets"));
 if(Str(BindingData,TEXT("status"))!=TEXT("PASS")||Str(BindingData,TEXT("spec_sha256"))!=SpecSha
  ||!BindingData->TryGetBoolField(TEXT("fresh_saved_reader"),FreshReader)||!FreshReader
  ||Str(BindingData,TEXT("import_report_sha256")).Len()!=64||Str(BindingData,TEXT("protected_native_inventory_sha256")).Len()!=64
  ||!NativeAssets||NativeAssets->Values.Num()!=5
  ||!Number(BindingData,TEXT("version"),Version)||Version!=1||Str(BindingData,TEXT("master"))!=MasterPath
  ||Str(BindingData,TEXT("plane_asset"))!=Asset(TEXT("SM_CinemaPosterPlane"))
  ||!Vector(BindingData,TEXT("site_world_three_m"),BoundSite)||!BoundSite.Equals(Site,Tolerance)
  ||!Originals||!Surfaces||Surfaces->Num()!=2){Reject(TEXT("complete matching saved import/verify binding required"));return {};}
 for(const TCHAR* Name:{TEXT("SM_CinemaPosterPlane"),TEXT("T_Cinema_DesertSky"),TEXT("T_Cinema_FridayMatinee"),TEXT("MI_Cinema_DesertSky"),TEXT("MI_Cinema_FridayMatinee")})
 {
  const auto Native=Obj(NativeAssets,*Asset(Name));double Bytes=0;
  if(Str(Native,TEXT("sha256")).Len()!=64||!Number(Native,TEXT("bytes"),Bytes)||Bytes<=0){Reject(TEXT("complete saved native asset inventory"));return {};}
 }
 auto F=MakeShared<FS47CinemaPosters>();F->Web=W;F->Binding=BindingData;
 F->Proxy=W->Handle(TEXT("world.town.proxies.cinema"));const auto* ProxyNode=W->Node(F->Proxy);
 if(F->Proxy!=1514||!ProxyNode||!ProxyNode->Comp.IsValid()){Reject(TEXT("existing cinema proxy"));return {};}
 F->ProxyParent=ProxyNode->Parent;F->ProxyGeo=ProxyNode->Geo;F->ProxyMats=ProxyNode->Mats;
 F->ProxyComp=ProxyNode->Comp;F->ProxyTransform=ProxyNode->Comp->GetComponentTransform();
 auto Original=[&](FOriginal& Out,const TCHAR* Kind,const TCHAR* MeshName,const TCHAR* MatName)
 {
  Out.Node=UniqueNode(W,*(FString(TEXT("town-production:"))+MeshName));const auto* N=W->Node(Out.Node);
  auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;UStaticMesh* Mesh=C?C->GetStaticMesh().Get():nullptr;
  const auto Record=Obj(Originals,Kind);const FString Path=FString(TEXT("/Game/TownBuildings/"))+MeshName+TEXT(".")+MeshName;
  Out.MaterialSlot=NamedSlot(Mesh,MatName);double RecordedSlot=-1;
  const FString MaterialPath=FString(TEXT("/Game/TownBuildings/Materials/"))+MatName+TEXT(".")+MatName;
  const FA* SlotNames=Arr(Record,TEXT("all_material_slot_names"));
  if(!C||!Mesh||!N||N->Parent!=W->Handle(TEXT("world.town.group"))||Mesh->GetPathName()!=Path
   ||Str(Record,TEXT("asset"))!=Path||!Counts(Mesh,Record)||Out.MaterialSlot<0
   ||Str(Record,TEXT("native_sha256")).Len()!=64
   ||Str(Record,TEXT("native_material_readback_sha256")).Len()!=64
   ||!Number(Record,TEXT("material_slot"),RecordedSlot)||RecordedSlot!=Out.MaterialSlot
   ||Str(Record,TEXT("material_slot_name"))!=MatName||Str(Record,TEXT("material_asset"))!=MaterialPath
   ||!C->GetMaterial(Out.MaterialSlot)||C->GetMaterial(Out.MaterialSlot)->GetPathName()!=MaterialPath
   ||!SlotNames||SlotNames->Num()!=Mesh->GetStaticMaterials().Num()
   ||!S47Three::ToThree(C->GetComponentLocation()).Equals(Site,Tolerance)
   ||!C->GetComponentScale().Equals(FVector::OneVector,.00001)||!C->GetComponentQuat().Equals(FQuat::Identity,.00001))return false;
  const FA* B=Arr(Record,TEXT("bounds_local_ue_cm"));FVector Lo,Hi;
  if(!B||B->Num()!=2)return false;
  auto BoundsObject=MakeShared<FJsonObject>();BoundsObject->SetArrayField(TEXT("lo"),(*B)[0]->AsArray());BoundsObject->SetArrayField(TEXT("hi"),(*B)[1]->AsArray());
  if(!Vector(BoundsObject,TEXT("lo"),Lo)||!Vector(BoundsObject,TEXT("hi"),Hi)
   ||!Mesh->GetBounds().Origin.Equals((Lo+Hi)/2,.025)||!Mesh->GetBounds().BoxExtent.Equals((Hi-Lo)/2,.025))return false;
  for(int32 I=0;I<SlotNames->Num();++I)if((*SlotNames)[I]->AsString()!=Mesh->GetStaticMaterials()[I].MaterialSlotName.ToString())return false;
  for(const auto& E:Expected){FVector Centre;FVector2D Size;if(!OriginalMeasurement(Record,E.Id,FString(Kind)==TEXT("glass"),Centre,Size))return false;}
  Out.Parent=N->Parent;Out.Comp=C;Out.Mesh=Mesh;Out.Transform=C->GetComponentTransform();
  for(int32 I=0;I<C->GetNumMaterials();++I)Out.Materials.Add(C->GetMaterial(I));
  return true;
 };
 if(!Original(F->Shell,TEXT("shell"),TEXT("SM_Town_Cinema_Shell"),TEXT("M_Town_PrintAtlas"))
  ||!Original(F->Glass,TEXT("glass"),TEXT("SM_Town_Cinema_Glass"),TEXT("M_Town_Glass")))
 {Reject(TEXT("actual TB2 mesh/slots/LODs/transform or saved original-plane measurements"));return {};}
 auto* PlaneMesh=LoadObject<UStaticMesh>(nullptr,*Asset(TEXT("SM_CinemaPosterPlane")));
 UMaterialInterface* Materials[2]={};
 for(int32 I=0;I<2;++I)
 {
  Materials[I]=LoadObject<UMaterialInterface>(nullptr,*Asset(Expected[I].Material));
  auto* Texture=LoadObject<UTexture2D>(nullptr,*Asset(Expected[I].Texture));
  if(!Material(Materials[I],Texture,Expected[I],{})||Str((*Surfaces)[I]->AsObject(),TEXT("id"))!=Expected[I].Id)
  {Reject(TEXT("complete two-texture/material set"));return {};}
 }
 if(!Plane(PlaneMesh,{},Arr(BindingData,TEXT("plane_saved_triangles")))){Reject(TEXT("own measured CPU plane/UV source"));return {};}
 // No original is hidden, trimmed or rebound. Prepare both unregistered components
 // before exposing either overlay; a failed registration destroys both additions.
 TArray<UStaticMeshComponent*> Pending;
 for(int32 I=0;I<2;++I)
 {
  auto* C=NewObject<UStaticMeshComponent>(W);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(F->Shell.Comp.Get());
  C->SetStaticMesh(PlaneMesh);C->SetMaterial(0,Materials[I]);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);
  C->SetVisibility(false,false);C->SetHiddenInGame(true,false);
  const FQuat Orientation=FRotationMatrix::MakeFromXY(FVector(0,-1,0),FVector(0,0,-1)).ToQuat();
  C->SetRelativeTransform(FTransform(Orientation,S47Three::ToUE(FVector(.162+Lift,1.51,Expected[I].Z)),FVector(ArtWidth,ArtHeight,1)));
  Pending.Add(C);
 }
 for(auto* C:Pending)C->RegisterComponent();
 for(auto* C:Pending)if(!C->IsRegistered()){for(auto* P:Pending)P->DestroyComponent();Reject(TEXT("complete overlay registration"));return {};}
 for(int32 I=0;I<2;++I)
 {
  const auto* C=Pending[I];FVector OriginalCentre=FVector::ZeroVector;FVector2D Size=FVector2D::ZeroVector;
  const bool Measured=OriginalMeasurement(Obj(Originals,TEXT("shell")),Expected[I].Id,false,OriginalCentre,Size);
  const FVector ExpectedAt=F->Shell.Comp->GetComponentTransform().TransformPosition(S47Three::ToUE(OriginalCentre+FVector(Lift,0,0)));
  const auto& X=C->GetComponentTransform();
  const bool Ready=Measured&&C->GetComponentLocation().Equals(ExpectedAt,Tolerance*100)
   &&DirectionThree(X.TransformVectorNoScale(FVector(0,0,1))).Equals(FVector(1,0,0),.00001)
   &&DirectionThree(X.TransformVectorNoScale(FVector(1,0,0))).Equals(FVector(0,0,-1),.00001)
   &&DirectionThree(X.TransformVectorNoScale(FVector(0,1,0))).Equals(FVector(0,-1,0),.00001)
   &&FMath::Abs(X.TransformVector(FVector(100,0,0)).Size()/100-ArtWidth)<Tolerance
   &&FMath::Abs(X.TransformVector(FVector(0,100,0)).Size()/100-ArtHeight)<Tolerance
   &&C->GetAttachParent()==F->Shell.Comp.Get()&&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!C->CastShadow;
  if(!Ready){for(auto* P:Pending)P->DestroyComponent();Reject(TEXT("actual complete surface metric/basis/parent preflight"));return {};}
 }
 for(int32 I=0;I<2;++I)
 {
  const int32 Node=W->AdoptNode(Pending[I],F->Shell.Node,FString(TEXT("cinema-poster:"))+Expected[I].Id);
  F->Prints.Add({Node,Pending[I]});
 }
 F->Update();UE_LOG(LogS47CinemaPosters,Display,TEXT("S47: cinema posters ready: two additive 2:3 exterior prints; all originals and Closed proxy retained"));
 return F;
}

bool FS47CinemaPosters::OriginalsIntact() const
{
 AS47Web* W=Web.Get();if(!W||!W->TownBuildings.IsValid())return false;
 const auto* ProxyNode=W->Node(Proxy);
 if(!ProxyNode||ProxyNode->Comp!=ProxyComp||!ProxyComp.IsValid()||ProxyNode->Parent!=ProxyParent||ProxyNode->Geo!=ProxyGeo
  ||ProxyNode->Mats!=ProxyMats||!ProxyComp->GetComponentTransform().Equals(ProxyTransform,.00001))return false;
 for(const FOriginal* O:{&Shell,&Glass})
 {
  const auto* N=W->Node(O->Node);const auto* C=O->Comp.Get();
  if(!N||!C||N->Parent!=O->Parent||N->Comp.Get()!=C||C->GetStaticMesh()!=O->Mesh.Get()
   ||!C->GetComponentTransform().Equals(O->Transform,.00001)||C->GetNumMaterials()!=O->Materials.Num())return false;
  for(int32 I=0;I<O->Materials.Num();++I)if(C->GetMaterial(I)!=O->Materials[I].Get())return false;
  const auto Record=Obj(Obj(Binding,TEXT("originals")),O==&Shell?TEXT("shell"):TEXT("glass"));
  if(!Counts(C->GetStaticMesh(),Record)||NamedSlot(C->GetStaticMesh(),O==&Shell?TEXT("M_Town_PrintAtlas"):TEXT("M_Town_Glass"))!=O->MaterialSlot)return false;
 }
 return true;
}
bool FS47CinemaPosters::ShouldShow() const
{
 AS47Web* W=Web.Get();return W&&OriginalsIntact()&&W->ObjAt(Shell.Node).Shown()&&W->ObjAt(Glass.Node).Shown()
  &&Shell.Comp->IsVisible()&&!Shell.Comp->bHiddenInGame&&Glass.Comp->IsVisible()&&!Glass.Comp->bHiddenInGame;
}
void FS47CinemaPosters::Update()
{
 AS47Web* W=Web.Get();if(!W)return;const bool Show=ShouldShow();
 for(const auto& P:Prints)if(auto* C=P.Comp.Get())
 {
  W->SetVisible(P.Node,Show);C->SetHiddenInGame(!Show,false);C->SetVisibility(Show,false);
 }
}

TSharedPtr<FJsonObject> FS47CinemaPosters::Readback() const
{
 auto Result=MakeShared<FJsonObject>();AS47Web* W=Web.Get();const bool Show=ShouldShow();
 bool Ok=W&&Prints.Num()==2&&OriginalsIntact();FA Rows,OriginalRows;
 Result->SetStringField(TEXT("scope"),TEXT("two exterior prints only; API/native readback, no human visual acceptance"));
 Result->SetBoolField(TEXT("active"),true);
 Result->SetStringField(TEXT("area"),W?W->Area():TEXT("none"));Result->SetStringField(TEXT("spec_sha256"),SpecSha);
 Result->SetBoolField(TEXT("expected_visible"),Show);Result->SetBoolField(TEXT("originals_intact"),OriginalsIntact());
 Result->SetNumberField(TEXT("preserved_cinema_proxy_node"),Proxy);Result->SetStringField(TEXT("preserved_cinema_proxy_component"),GetPathNameSafe(ProxyComp.Get()));
 auto ProxyRow=MakeShared<FJsonObject>();const auto* ProxyNode=W?W->Node(Proxy):nullptr;
 ProxyRow->SetNumberField(TEXT("node"),Proxy);ProxyRow->SetNumberField(TEXT("parent"),ProxyNode?ProxyNode->Parent:-1);
 ProxyRow->SetNumberField(TEXT("geo"),ProxyNode?ProxyNode->Geo:-1);ProxyRow->SetStringField(TEXT("component"),GetPathNameSafe(ProxyComp.Get()));
 ProxyRow->SetBoolField(TEXT("metadata_shown"),W&&W->ObjAt(Proxy).Shown());
 if(const auto* C=Cast<UPrimitiveComponent>(ProxyComp.Get()))
 {
  ProxyRow->SetNumberField(TEXT("component_object_id"),C->GetUniqueID());
  ProxyRow->SetArrayField(TEXT("actual_centre_three_m"),VectorJson(S47Three::ToThree(C->GetComponentLocation())));
  const FBox Box=C->Bounds.GetBox();ProxyRow->SetArrayField(TEXT("actual_bounds_min_three_m"),VectorJson(S47Three::ToThree(Box.Min)));
  ProxyRow->SetArrayField(TEXT("actual_bounds_max_three_m"),VectorJson(S47Three::ToThree(Box.Max)));
 }
 Result->SetObjectField(TEXT("proxy"),ProxyRow);
 Result->SetObjectField(TEXT("saved_import_binding"),Binding);
 for(const FOriginal* O:{&Shell,&Glass})
 {
  const auto* C=O->Comp.Get();const auto* N=W?W->Node(O->Node):nullptr;auto Row=MakeShared<FJsonObject>();
  Row->SetNumberField(TEXT("node"),O->Node);Row->SetNumberField(TEXT("parent"),N?N->Parent:-1);
  Row->SetStringField(TEXT("component"),GetPathNameSafe(C));Row->SetStringField(TEXT("mesh"),C?GetPathNameSafe(C->GetStaticMesh()):TEXT("none"));
  Row->SetStringField(TEXT("print_or_glass_material"),C?GetPathNameSafe(C->GetMaterial(O->MaterialSlot)):TEXT("none"));
  Row->SetNumberField(TEXT("named_material_slot"),O->MaterialSlot);Row->SetBoolField(TEXT("metadata_shown"),W&&W->ObjAt(O->Node).Shown());
  Row->SetBoolField(TEXT("component_visible"),C&&C->IsVisible());Row->SetBoolField(TEXT("hidden_in_game"),C&&C->bHiddenInGame);
  Row->SetBoolField(TEXT("original_cpu_access"),C&&C->GetStaticMesh()&&C->GetStaticMesh()->bAllowCPUAccess);
  if(C)
  {
   Row->SetArrayField(TEXT("actual_world_origin_three_m"),VectorJson(S47Three::ToThree(C->GetComponentLocation())));
   Row->SetArrayField(TEXT("actual_component_scale"),VectorJson(C->GetComponentScale()));
   const FQuat Q=C->GetComponentQuat();Row->SetArrayField(TEXT("actual_component_rotation_xyzw"),Values({Q.X,Q.Y,Q.Z,Q.W}));
   if(const UStaticMesh* Mesh=C->GetStaticMesh().Get())
   {
    FA LodCounts,Slots,Materials;
    for(int32 L=0;L<Mesh->GetNumLODs();++L)LodCounts.Add(MakeShared<FJsonValueNumber>(Mesh->GetNumTriangles(L)));
    for(const auto& Slot:Mesh->GetStaticMaterials())Slots.Add(MakeShared<FJsonValueString>(Slot.MaterialSlotName.ToString()));
    for(int32 K=0;K<C->GetNumMaterials();++K)Materials.Add(MakeShared<FJsonValueString>(GetPathNameSafe(C->GetMaterial(K))));
    Row->SetArrayField(TEXT("actual_lod_triangles"),LodCounts);Row->SetArrayField(TEXT("actual_material_slot_names"),Slots);
    Row->SetArrayField(TEXT("actual_material_assets"),Materials);Row->SetNumberField(TEXT("actual_mesh_object_id"),Mesh->GetUniqueID());
    Row->SetArrayField(TEXT("actual_mesh_bounds_origin_ue_cm"),VectorJson(Mesh->GetBounds().Origin));
    Row->SetArrayField(TEXT("actual_mesh_bounds_extent_ue_cm"),VectorJson(Mesh->GetBounds().BoxExtent));
   }
  }
  OriginalRows.Add(MakeShared<FJsonValueObject>(Row));
 }
 for(int32 I=0;I<Prints.Num()&&I<2;++I)
 {
  const auto& E=Expected[I];const auto& P=Prints[I];const auto* C=P.Comp.Get();const auto* N=W?W->Node(P.Node):nullptr;
  auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("id"),E.Id);Row->SetNumberField(TEXT("node"),P.Node);
  Row->SetStringField(TEXT("component"),GetPathNameSafe(C));Row->SetNumberField(TEXT("parent"),N?N->Parent:-1);
  Row->SetStringField(TEXT("actual_attach_parent_component"),C?GetPathNameSafe(C->GetAttachParent()):TEXT("none"));
  Row->SetStringField(TEXT("actual_mesh"),C?GetPathNameSafe(C->GetStaticMesh()):TEXT("none"));
  if(C&&C->GetStaticMesh()&&C->GetStaticMesh()->GetStaticMaterials().Num()==1)
   Row->SetStringField(TEXT("actual_mesh_slot0_name"),C->GetStaticMesh()->GetStaticMaterials()[0].MaterialSlotName.ToString());
  auto* Texture=LoadObject<UTexture2D>(nullptr,*Asset(E.Texture));
  bool Good=C&&N&&N->Parent==Shell.Node&&C->GetAttachParent()==Shell.Comp.Get()&&C->GetNumMaterials()==1
   &&C->GetStaticMesh()&&C->GetStaticMesh()->GetPathName()==Asset(TEXT("SM_CinemaPosterPlane"))
   &&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision&&!C->CastShadow&&C->IsVisible()==Show&&C->bHiddenInGame==!Show
   &&W->ObjAt(P.Node).Shown()==Show;
  // Always evaluate all probes so a failed material cannot hide a UV diagnostic.
  Good&=Material(C?C->GetMaterial(0):nullptr,Texture,E,Row);Good&=Plane(C?C->GetStaticMesh():nullptr,Row,Arr(Binding,TEXT("plane_saved_triangles")));
  FVector OriginalCentre=FVector::ZeroVector,GlassCentre=FVector::ZeroVector;FVector2D Frame=FVector2D::ZeroVector,Glazing=FVector2D::ZeroVector;
  Good&=OriginalMeasurement(Obj(Obj(Binding,TEXT("originals")),TEXT("shell")),E.Id,false,OriginalCentre,Frame);
  Good&=OriginalMeasurement(Obj(Obj(Binding,TEXT("originals")),TEXT("glass")),E.Id,true,GlassCentre,Glazing);
  if(C&&Shell.Comp.IsValid())
  {
   const FVector Centre=S47Three::ToThree(C->GetComponentLocation());
   const FVector OriginalWorld=S47Three::ToThree(Shell.Comp->GetComponentTransform().TransformPosition(S47Three::ToUE(OriginalCentre)));
   const FVector Right=DirectionThree(C->GetComponentTransform().TransformVectorNoScale(FVector(1,0,0)));
   const FVector Down=DirectionThree(C->GetComponentTransform().TransformVectorNoScale(FVector(0,1,0)));
   const FVector Normal=DirectionThree(C->GetComponentTransform().TransformVectorNoScale(FVector(0,0,1)));
   const double Width=C->GetComponentTransform().TransformVector(FVector(100,0,0)).Size()/100;
   const double Height=C->GetComponentTransform().TransformVector(FVector(0,100,0)).Size()/100;
   const FVector Offset=Centre-OriginalWorld;
   Good&=Offset.Equals(FVector(Lift,0,0),Tolerance)&&FMath::Abs(Width-ArtWidth)<Tolerance&&FMath::Abs(Height-ArtHeight)<Tolerance
    &&Right.Equals(FVector(0,0,-1),.00001)&&Down.Equals(FVector(0,-1,0),.00001)&&Normal.Equals(FVector(1,0,0),.00001)
    &&Width<=Frame.X&&Height<=Frame.Y+Tolerance&&Centre.X<Site.X+GlassCentre.X;
   Row->SetArrayField(TEXT("actual_world_centre_three_m"),VectorJson(Centre));Row->SetArrayField(TEXT("actual_art_size_m"),Values({Width,Height}));
   const FBox Box=C->Bounds.GetBox();Row->SetArrayField(TEXT("actual_bounds_min_three_m"),VectorJson(S47Three::ToThree(Box.Min)));
   Row->SetArrayField(TEXT("actual_bounds_max_three_m"),VectorJson(S47Three::ToThree(Box.Max)));
   Row->SetArrayField(TEXT("saved_original_frame_size_m"),Values({Frame.X,Frame.Y}));Row->SetArrayField(TEXT("saved_original_glass_size_m"),Values({Glazing.X,Glazing.Y}));
   Row->SetArrayField(TEXT("original_centre_transformed_by_actual_TB2_three_m"),VectorJson(OriginalWorld));
   Row->SetArrayField(TEXT("actual_offset_from_original_three_m"),VectorJson(Offset));
   Row->SetArrayField(TEXT("actual_right_three"),VectorJson(Right));Row->SetArrayField(TEXT("actual_down_three"),VectorJson(Down));Row->SetArrayField(TEXT("actual_normal_three"),VectorJson(Normal));
   Row->SetBoolField(TEXT("component_visible"),C->IsVisible());Row->SetBoolField(TEXT("hidden_in_game"),C->bHiddenInGame);
   Row->SetBoolField(TEXT("no_collision"),C->GetCollisionEnabled()==ECollisionEnabled::NoCollision);Row->SetBoolField(TEXT("casts_shadow"),C->CastShadow);
  }
  Row->SetStringField(TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));Rows.Add(MakeShared<FJsonValueObject>(Row));Ok&=Good;
 }
 Result->SetArrayField(TEXT("original_components"),OriginalRows);Result->SetArrayField(TEXT("prints"),Rows);
 Result->SetStringField(TEXT("status"),Ok?TEXT("PASS"):TEXT("FAIL"));return Result;
}
bool FS47CinemaPosters::Audit(TArray<FString>& Lines) const
{
 const auto J=Readback();const bool Ok=Str(J,TEXT("status"))==TEXT("PASS");
 Lines.Add(FString::Printf(TEXT("cinema posters %s: two live component/material/UV/metric/parent/visibility audits; original TB2 frames use fresh saved-native import measurements; Closed proxy %d retained"),Ok?TEXT("PASS"):TEXT("FAIL"),Proxy));
 if(const FA* Rows=Arr(J,TEXT("prints")))for(const auto& Value:*Rows)
 {
  const auto Row=Value->AsObject();
  Lines.Add(FString::Printf(TEXT("cinema poster %s %s component=%s slot0=%s Map=%s parentMaterial=%s; full actual UV/size/basis/offset values in Readback"),
   *Str(Row,TEXT("id")),*Str(Row,TEXT("status")),*Str(Row,TEXT("component")),*Str(Row,TEXT("actual_material_slot0")),*Str(Row,TEXT("actual_Map")),*Str(Row,TEXT("actual_material_parent"))));
 }
 return Ok;
}

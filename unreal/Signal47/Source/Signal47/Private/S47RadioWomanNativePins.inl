// Kun generatorutdata fra faktisk R11 bake + separat ferskleser kan brukes.
// Denne malen med @@-felt er ikke en kompilert pin eller kontaktaksept.
// Inkluderes i RadioWoman-anonymnamespace ETTER eksisterende VertexHash.
namespace S47R11NativePins
{
 struct FSectionPin
 {
  int32 Material;
  uint32 Triangles, First, Min, Max;
  bool Collision, CastShadow, VisibleRT;
 };
 struct FLODPin
 {
  uint32 Vertices, Triangles;
  const TCHAR* PackedVertexSHA1;
  const TCHAR* UInt32IndexSHA1;
  const FSectionPin* Sections;
  int32 SectionCount;
  bool FullUV, HighTangents;
 };
 // Faktisk saved asset SHA256: b9f91fd923f725ae7fed0d80a9bfd5250d5c1437032f679a2f767d02c0aa06da
 // Faktisk outer VERIFY SHA256: 27dc5e44e655b5177f952a755e689b4c271182f2667ec056148179f7b0827543
 // Faktisk separat native VERIFY SHA256: 9025a0f539f593f61f540063ac17e97f7fb71ed394554fd3cf03853620e59119
 // bAffectDistanceFieldLighting/bForceOpaque er ikke eksportert av editor548 og er ikke målepåstander.
 const TCHAR* const AcceptedObject=TEXT("/Game/MetaHumans/Mannequins/RadioWoman/BakeR11/SM_RadioWoman_Seated.SM_RadioWoman_Seated");
 const TCHAR* const AcceptedMaterials[]={TEXT("/Game/MetaHumans/MHC_Waitress/Body/Materials/MI_Body_Baked.MI_Body_Baked"),TEXT("/Game/MetaHumans/MHC_Waitress/Face/Materials/MI_Teeth_Baked.MI_Teeth_Baked"),TEXT("/Game/MetaHumans/Common/Materials/M_Hide.M_Hide"),TEXT("/Game/MetaHumans/MHC_Waitress/Face/Materials/MI_EyeL_Baked.MI_EyeL_Baked"),TEXT("/Game/MetaHumans/MHC_Waitress/Face/Materials/MI_EyeR_Baked.MI_EyeR_Baked"),TEXT("/Game/MetaHumans/MHC_Waitress/Face/Materials/MI_Face_EyeShell.MI_Face_EyeShell"),TEXT("/Game/MetaHumans/MHC_Waitress/Face/Materials/MI_Face_Eyelashes.MI_Face_Eyelashes"),TEXT("/Game/MetaHumans/MHC_Waitress/Face/Materials/MI_Face_LacrimalFluid.MI_Face_LacrimalFluid"),TEXT("/Game/MetaHumans/Common/Materials/M_Hide.M_Hide"),TEXT("/Game/MetaHumans/MHC_Waitress/Face/Materials/MI_Face_Skin_Baked_LOD1.MI_Face_Skin_Baked_LOD1"),TEXT("/Game/MetaHumans/MHC_Waitress/Face/Materials/MI_Face_EyelashesHiLODs.MI_Face_EyelashesHiLODs"),TEXT("/Game/MetaHumans/MHC_Waitress/Face/Materials/MI_Face_Skin_Baked_LOD3.MI_Face_Skin_Baked_LOD3"),TEXT("/Game/MetaHumans/MHC_Waitress/Face/Materials/MI_Face_Skin_Baked_LOD5to7.MI_Face_Skin_Baked_LOD5to7"),TEXT("/Game/MetaHumans/Mannequins/RadioWoman/BakeR10/M_RW_Coat.M_RW_Coat"),TEXT("/Game/MetaHumans/Mannequins/RadioWoman/BakeR10/M_RW_Trousers.M_RW_Trousers"),TEXT("/Game/MetaHumans/Mannequins/RadioWoman/BakeR10/M_RW_Shoe.M_RW_Shoe"),TEXT("/Game/MetaHumans/Mannequins/RadioWoman/BakeR10/M_RW_Trim.M_RW_Trim"),TEXT("/Game/MetaHumans/Mannequins/RadioWoman/BakeR10/M_RW_Button.M_RW_Button"),TEXT("/Game/MetaHumans/Waitress/Uniform/M_Waitress_HairCards.M_Waitress_HairCards"),TEXT("/Game/MetaHumans/Mannequins/RadioWoman/BakeR10/M_RadioWoman_BrowCards.M_RadioWoman_BrowCards")};
 const FSectionPin Sections0[]={
  {0,60816,0,0,32333,true,true,true},
  {1,5950,182448,44561,47916,true,true,true},
  {2,596,200298,47917,48340,true,true,true},
  {3,720,202086,48341,48726,true,true,true},
  {4,720,204246,48727,49112,true,true,true},
  {5,560,206406,49113,49442,true,true,true},
  {6,1218,208086,49443,51002,true,true,true},
  {7,386,211740,51003,51278,true,true,true},
  {8,504,212898,51279,51616,true,true,true},
  {9,23860,214410,32334,44560,true,true,true},
  {13,11032,285990,51617,57193,true,true,true},
  {14,7776,319086,57194,61172,true,true,true},
  {15,1144,342414,61173,61952,true,true,true},
  {16,1156,345846,61953,62734,true,true,true},
  {17,180,349314,62735,62932,true,true,true},
  {18,15441,349854,62933,80457,true,true,true},
  {19,2744,396177,80458,84655,true,true,true},
 };
 const FSectionPin Sections1[]={
  {0,15196,0,0,8567,true,true,true},
  {1,796,45588,8568,9114,true,true,true},
  {3,216,47976,9115,9236,true,true,true},
  {4,216,48624,9237,9358,true,true,true},
  {5,196,49272,9359,9486,true,true,true},
  {7,150,49860,9487,9604,true,true,true},
  {10,250,50310,9605,9978,true,true,true},
  {11,5048,51060,9979,12632,true,true,true},
  {13,11032,66204,12633,18209,true,true,true},
  {14,7776,99300,18210,22188,true,true,true},
  {15,1144,122628,22189,22968,true,true,true},
  {16,1156,126060,22969,23750,true,true,true},
  {17,180,129528,23751,23948,true,true,true},
  {18,5494,130068,23949,30432,true,true,true},
  {19,1322,146550,30433,32596,true,true,true},
 };
 const FSectionPin Sections2[]={
  {0,1952,0,0,1238,true,true,true},
  {1,20,5856,1239,1262,true,true,true},
  {3,40,5916,1263,1287,true,true,true},
  {4,40,6036,1288,1312,true,true,true},
  {12,256,6156,1313,1481,true,true,true},
  {13,11032,6924,1482,7058,true,true,true},
  {14,7776,40020,7059,11037,true,true,true},
  {15,1144,63348,11038,11817,true,true,true},
  {16,1156,66780,11818,12599,true,true,true},
  {17,180,70248,12600,12797,true,true,true},
  {18,5494,70788,12798,19281,true,true,true},
  {19,1322,87270,19282,21445,true,true,true},
 };
 const FLODPin AcceptedLODs[]={
  {84656,134803,TEXT("98ace3df9ca76cd5ad46908b2c429b3dd29250fa"),TEXT("0e02dbc076219b9a904a017b09798288fefc0a5d"),Sections0,17,true,true},
  {32597,50172,TEXT("005bb3b4bc0450920abc0373459b539c2516183d"),TEXT("3a1e06297a5efb0cf37f028747f0c5d4d82146b5"),Sections1,15,true,true},
  {21446,30412,TEXT("e47e766a9f291927c595a13bdc6eae4011c60910"),TEXT("482dfa1d9ef028a2b5545afaf7592804d7631a36"),Sections2,12,true,true},
 };
 bool Validate(UStaticMesh* Mesh,FString& Why)
 {
  const auto Reject=[&](const TCHAR* Text){Why=Text;return false;};
  if(!Mesh || Mesh->GetPathName()!=AcceptedObject || !Mesh->bAllowCPUAccess
   || !Mesh->bSupportRayTracing || Mesh->GetNaniteSettings().bEnabled)
   return Reject(TEXT("actual R11 native asset/CPU/RT policy differs"));
  const auto* Data=Mesh->GetRenderData();
  if(!Data || !Data->IsInitialized() || Data->LODResources.Num()!=3)
   return Reject(TEXT("all three actual R11 render LODs must be ready"));
  const auto& Materials=Mesh->GetStaticMaterials();
  if(Materials.Num()!=int32(UE_ARRAY_COUNT(AcceptedMaterials)))
   return Reject(TEXT("R11 complete native material table count differs"));
  for(int32 I=0;I<Materials.Num();++I)
  {
   const UMaterialInterface* Material=Materials[I].MaterialInterface.Get();
   if(!Material || Material->GetPathName()!=AcceptedMaterials[I])
    return Reject(TEXT("R11 native material path/order differs"));
  }
  for(int32 I=0;I<3;++I)
  {
   const auto& L=Data->LODResources[I];const auto& Pin=AcceptedLODs[I];
   const auto& VB=L.VertexBuffers.StaticMeshVertexBuffer;
   if(L.GetNumVertices()!=int32(Pin.Vertices) || L.GetNumTriangles()!=int32(Pin.Triangles)
    || VB.GetUseFullPrecisionUVs()!=Pin.FullUV || VB.GetUseHighPrecisionTangentBasis()!=Pin.HighTangents
    || L.Sections.Num()!=Pin.SectionCount)
    return Reject(TEXT("R11 actual LOD counts/format/section count differ"));
   if(!VertexHash(L).Equals(Pin.PackedVertexSHA1,ESearchCase::IgnoreCase))
    return Reject(TEXT("R11 packed position/tangent/UV/color payload differs or is unavailable"));
   const uint32 ExpectedIndices=Pin.Triangles*3;
   if(!L.IndexBuffer.GetAllowCPUAccess() || L.IndexBuffer.GetNumIndices()!=int32(ExpectedIndices)
    || L.IndexBuffer.GetIndexDataSize()!=int32(ExpectedIndices*(L.IndexBuffer.Is32Bit()?4:2)))
    return Reject(TEXT("R11 full CPU index payload/count is unavailable or differs"));
   TArray<uint32> Indices;L.IndexBuffer.GetCopy(Indices);
   if(Indices.Num()!=int32(ExpectedIndices))return Reject(TEXT("R11 normalized uint32 index count differs"));
   for(const uint32 Index:Indices)if(Index>=Pin.Vertices)return Reject(TEXT("R11 actual index outside native vertex buffer"));
   FSHA1 H;H.Update(reinterpret_cast<const uint8*>(Indices.GetData()),Indices.Num()*sizeof(uint32));
   H.Final();FSHAHash Hash;H.GetHash(Hash.Hash);
   if(!Hash.ToString().Equals(Pin.UInt32IndexSHA1,ESearchCase::IgnoreCase))
    return Reject(TEXT("R11 normalized uint32 index order/payload differs"));
   for(int32 J=0;J<Pin.SectionCount;++J)
   {
    const auto& A=L.Sections[J];const auto& P=Pin.Sections[J];
    if(A.MaterialIndex!=P.Material || A.NumTriangles!=P.Triangles || A.FirstIndex!=P.First
     || A.MinVertexIndex!=P.Min || A.MaxVertexIndex!=P.Max || A.bEnableCollision!=P.Collision
     || A.bCastShadow!=P.CastShadow || A.bVisibleInRayTracing!=P.VisibleRT)
     return Reject(TEXT("R11 actual render section layout/material/observed flags differ"));
   }
  }
  Why=TEXT("actual three-LOD R11 native payload matches independent saved-reader pins; contact remains separate");
  return true;
 }
}

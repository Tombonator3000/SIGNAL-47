// Faktisk uavhengig native VERIFY-aksept SHA256: 78e57318c98b3ad89c9764066ae0834bad07317358a8d0d8769b26ac57bcf53c
// Generert fra faktisk separat lagret R12-leser og uavhengig nativefasekvittering.
// Nativepayloadens identitet kontrolleres her; levende kontakt og spillerreise krever egne bevis.
// Inkluderes i RadioWoman-anonymnamespace ETTER eksisterende VertexHash.
namespace S47R12NativePins
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
 // Faktisk saved asset SHA256: ac78d509bd404343e651fada9f36b6c357cde03d9321e31e23d0acd0ae13cdce
 // Faktisk outer VERIFY SHA256: 0ad4109a1f090b735308f0570505c74be7ad66111233b9962967685f22c15a71
 // Faktisk separat native VERIFY SHA256: 7acb053b409c64f5030cb5d583a26e37a743bfaa0bd9c74a8d9805ce4921e0f3
 // bAffectDistanceFieldLighting/bForceOpaque er ikke eksportert i dette målte nativeformatet og er ikke målepåstander.
 const TCHAR* const AcceptedObject=TEXT("/Game/MetaHumans/Mannequins/RadioWoman/BakeR12/SM_RadioWoman_Seated.SM_RadioWoman_Seated");
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
  {13,13112,285990,51617,58233,true,true,true},
  {14,7776,325326,58234,62212,true,true,true},
  {15,1144,348654,62213,62992,true,true,true},
  {16,1156,352086,62993,63774,true,true,true},
  {17,180,355554,63775,63972,true,true,true},
  {18,15441,356094,63973,81497,true,true,true},
  {19,2744,402417,81498,85695,true,true,true},
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
  {13,13112,66204,12633,19249,true,true,true},
  {14,7776,105540,19250,23228,true,true,true},
  {15,1144,128868,23229,24008,true,true,true},
  {16,1156,132300,24009,24790,true,true,true},
  {17,180,135768,24791,24988,true,true,true},
  {18,5494,136308,24989,31472,true,true,true},
  {19,1322,152790,31473,33636,true,true,true},
 };
 const FSectionPin Sections2[]={
  {0,1952,0,0,1238,true,true,true},
  {1,20,5856,1239,1262,true,true,true},
  {3,40,5916,1263,1287,true,true,true},
  {4,40,6036,1288,1312,true,true,true},
  {12,256,6156,1313,1481,true,true,true},
  {13,13112,6924,1482,8098,true,true,true},
  {14,7776,46260,8099,12077,true,true,true},
  {15,1144,69588,12078,12857,true,true,true},
  {16,1156,73020,12858,13639,true,true,true},
  {17,180,76488,13640,13837,true,true,true},
  {18,5494,77028,13838,20321,true,true,true},
  {19,1322,93510,20322,22485,true,true,true},
 };
 const FLODPin AcceptedLODs[]={
  {85696,136883,TEXT("d781a33f19cc46d3db6c57d3f5f38302ec5ade07"),TEXT("f0bfcbc98a097b120d8e040bd983f7ca408c64f5"),Sections0,17,true,true},
  {33637,52252,TEXT("8cbf154624247baea6e066faba73a519cdbcd696"),TEXT("2ecbc973c4da201ce28afc663edf4a5c30660e41"),Sections1,15,true,true},
  {22486,32492,TEXT("8896edf95c6fc1aae28051744c69cc29e4984741"),TEXT("21ef0a9c8544fa88442cf05edee200271c0409fe"),Sections2,12,true,true},
 };
 bool Validate(UStaticMesh* Mesh,FString& Why)
 {
  const auto Reject=[&](const TCHAR* Text){Why=Text;return false;};
  if(!Mesh || Mesh->GetPathName()!=AcceptedObject || !Mesh->bAllowCPUAccess
   || !Mesh->bSupportRayTracing || Mesh->HasValidNaniteData())
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

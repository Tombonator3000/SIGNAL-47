// Isolert ENV02-kontroll. API-styrt World/Walker, aldri menneskelig input.
// Nordturen starter i ordinær BeginCommute og flytter aldri bilen direkte.
// Sørgående start og case1-disk er tydelig avgrensede utviklerfixtures.
#include "S47Web.h"
#include "S47Main.h"
#include "S47Anomaly.h"
#include "S47Evening.h"
#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Story.h"
#include "S47Startup.h"
#include "S47Three.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "TimerManager.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47HomesContract,Log,All);
namespace
{
 using FJ=S47J::FObj;
 constexpr int32 ProtectedNodes[]={1436,1437,1438,1439,1440,1441,1442,1443,1444,1445,1446,1447,1534,1535,1536,1537,1538,1539,1546,1547,1551,1552,1553,1554,1563,1594,1595,1596,1456};
 const TCHAR* ProxyPaths[]={TEXT("world.town.proxies.tv1"),TEXT("world.town.proxies.tv2"),TEXT("world.town.proxies.kitchen"),TEXT("world.town.proxies.smoker")};
 const TCHAR* ProxyIds[]={TEXT("town:tv1"),TEXT("town:tv2"),TEXT("town:kitchen"),TEXT("town:smoker")};
 constexpr double ProxyRanges[]={6.5,6.5,4.,5.6};
 struct FHouse {const TCHAR* Name;double Z;};
 constexpr FHouse Houses[]={{TEXT("H04"),2241},{TEXT("H03"),2185},{TEXT("H02"),2156.5},{TEXT("H01"),2133}};
 void Vector(const FJ& O,const TCHAR* Key,const FVector& V){S47J::Set(O,Key,TArray<double>{V.X,V.Y,V.Z});}
 FJ Transform(const FTransform& T)
 {
  auto O=S47J::New();Vector(O,TEXT("location_ue_cm"),T.GetLocation());Vector(O,TEXT("scale"),T.GetScale3D());
  const auto Q=T.GetRotation();S47J::Set(O,TEXT("quaternion_ue"),TArray<double>{Q.X,Q.Y,Q.Z,Q.W});return O;
 }
 bool Same(const FTransform& A,const FTransform& B)
 {return A.GetLocation().Equals(B.GetLocation(),.02)&&A.GetScale3D().Equals(B.GetScale3D(),1e-6)&&A.GetRotation().Equals(B.GetRotation(),1e-6);}
 bool SameJson(const FJ& A,const FJ& B)
 {return A&&B&&FJsonValue::CompareEqual(FJsonValueObject(A),FJsonValueObject(B));}
 bool SameDoors(const TMap<FString,bool>& A,const TMap<FString,bool>& B)
 {
  if(A.Num()!=B.Num())return false;
  for(const auto& Entry:A){const bool* Value=B.Find(Entry.Key);if(!Value||*Value!=Entry.Value)return false;}return true;
 }
 bool SaveJson(const FString& File,const FJ& O){return FFileHelper::SaveStringToFile(S47J::Write(O,true),*File);}
 FJ ReadJson(const FString& File){FString Text;return FFileHelper::LoadFileToString(Text,*File)?S47J::Read(Text):FJ();}
 double Angle(double A){return FMath::Atan2(FMath::Sin(A),FMath::Cos(A));}
 bool InMain(const FVector& P,double Margin=.28)
 {return P.X>=7982.4+Margin&&P.X<=8019.5-Margin&&P.Z>=2015+Margin&&P.Z<=2256-Margin;}
 bool Rect(const FS47Obstacle& O,const double* B)
 {return !O.bCircle&&FMath::Abs(O.MinX-B[0])<1e-6&&FMath::Abs(O.MaxX-B[1])<1e-6&&FMath::Abs(O.MinZ-B[2])<1e-6&&FMath::Abs(O.MaxZ-B[3])<1e-6;}

 bool PngReceipt(const FString& File,FJ& Out,FString& Why)
 {
  TArray<uint8> B;if(!FFileHelper::LoadFileToArray(B,*File)){Why=TEXT("PNG file pending");return false;}
  const uint8 Magic[]={137,80,78,71,13,10,26,10};
  if(B.Num()<33||FMemory::Memcmp(B.GetData(),Magic,8)){Why=TEXT("PNG signature/header incomplete");return false;}
  auto U32=[&](int64 I){return (uint32(B[I])<<24)|(uint32(B[I+1])<<16)|(uint32(B[I+2])<<8)|B[I+3];};
  int64 At=8,Idat=0;int32 Chunks=0;uint32 Width=0,Height=0;bool End=false;
  while(At+12<=B.Num())
  {
   const uint32 Size=U32(At);if(int64(Size)>B.Num()-At-12){Why=TEXT("PNG chunk incomplete");return false;}
   const uint8* Type=B.GetData()+At+4;
   if(FCrc::MemCrc32(Type,Size+4)!=U32(At+8+Size)){Why=TEXT("PNG chunk CRC mismatch");return false;}
   if(Chunks==0)
   {
    if(FMemory::Memcmp(Type,"IHDR",4)||Size!=13){Why=TEXT("PNG IHDR missing");return false;}
    Width=U32(At+8);Height=U32(At+12);
   }
   if(!FMemory::Memcmp(Type,"IDAT",4))Idat+=Size;
   ++Chunks;At+=int64(Size)+12;
   if(!FMemory::Memcmp(Type,"IEND",4)){End=Size==0;break;}
  }
  if(!End||At!=B.Num()||Idat<=0||Width<320||Height<200){Why=TEXT("PNG/IEND/full payload incomplete");return false;}
  FSHAHash Hash;FSHA1::HashBuffer(B.GetData(),B.Num(),Hash.Hash);Out=S47J::New();
  S47J::Set(Out,TEXT("file"),File);S47J::Set(Out,TEXT("sha1"),Hash.ToString());S47J::Set(Out,TEXT("bytes"),B.Num());
  S47J::Set(Out,TEXT("width"),double(Width));S47J::Set(Out,TEXT("height"),double(Height));S47J::Set(Out,TEXT("chunks"),Chunks);
  S47J::Set(Out,TEXT("idat_bytes"),double(Idat));S47J::Set(Out,TEXT("iend_at_eof"),true);S47J::Set(Out,TEXT("all_chunk_crc_verified"),true);return true;
 }

 struct FProtected
 {
  int32 Node=-1,Parent=-1,Geo=-1;TArray<int32> Mats;
  FString PosePolicy=TEXT("original exported local pose");
  TWeakObjectPtr<USceneComponent> Comp;TWeakObjectPtr<UStaticMesh> Mesh;
  TArray<TWeakObjectPtr<UMaterialInterface>> Materials;
  FTransform Relative=FTransform::Identity;
  FVector P=FVector::ZeroVector,S=FVector::OneVector;FQuat Q=FQuat::Identity;
 };
 struct FBinding {FString Path,Id;int32 Node=-1;double Range=0,ExamineRange=0;bool Look=false,Use=false,Examine=false;};
 struct FControl : TSharedFromThis<FControl>
 {
  TWeakObjectPtr<AS47Web> Web;TWeakObjectPtr<US47World> WorldIdentity;FTimerHandle Timer;
  FString Root;int32 Stage=0,Checks=0,HouseShot=0,Waypoint=0;bool Done=false,Started=false,Passed[4]={};
  double Began=FPlatformTime::Seconds(),At=Began,Last=Began,ProgressAt=Began,LastProgressZ=2335,ReadyAt=0;
  double NorthMetres=0,SouthMetres=0,WalkMetres=0,LastSpeed=0;int32 Track=0;
  FVector LastTruck=FVector::ZeroVector,LastFoot=FVector::ZeroVector,FootStart=FVector::ZeroVector;
  TArray<FVector> Walk;TArray<FProtected> Protected;TArray<FBinding> Bindings;TMap<FString,int32> Handles;
  TMap<FString,bool> Doors;TArray<TSharedPtr<FJsonValue>> Photos,Events;
  FS47DriveController* RoadDrive=nullptr;
  FString PhotoName;TWeakObjectPtr<UStaticMeshComponent> PhotoTarget;bool PhotoRequested=false;double PhotoAt=0;FJ PhotoMeta;
  FJ BeforeSave;FString SaveId;FVector SavedFoot=FVector::ZeroVector;double SavedYaw=0,SavedPitch=0,SavedClock=0;

  US47Main* Main() const {return Web.IsValid()?US47Main::Get(Web.Get()):nullptr;}
  void Result(bool Pass,const FString& Why)
  {
   // ClearTimer may release the delegate's last owner while this callback runs.
   const TSharedRef<FControl> KeepAlive=AsShared();
   if(Done)return;Done=true;auto* M=Main();if(M&&M->Player){M->Player->TestMove.Reset();M->Player->bTestRun=false;}
   if(M&&M->World){M->World->Autopilot.Reset();M->World->TestInput=nullptr;}
   auto O=S47J::New();S47J::Set(O,TEXT("status"),Pass?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);
   S47J::Set(O,TEXT("stage"),Stage);S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("seconds"),FPlatformTime::Seconds()-Began);
   S47J::Set(O,TEXT("north_metres"),NorthMetres);S47J::Set(O,TEXT("south_metres"),SouthMetres);S47J::Set(O,TEXT("street_walk_metres"),WalkMetres);
   S47J::Set(O,TEXT("evidence_scope"),TEXT("API Autopilot/TestInput through ordinary World/Drive/Walker; full Flow title/Continue; actual viewport PNGs with CRC/IEND"));
   S47J::Set(O,TEXT("limits"),TEXT("Human input, visual approval, night-time house schedules and entry to closed residences unverified; no facade walk/house entry performed. Southbound start and first case1 disk are explicit isolated fixtures."));
   O->SetArrayField(TEXT("photos"),Photos);O->SetArrayField(TEXT("events"),Events);
   const bool Written=!Root.IsEmpty()&&SaveJson(Root/TEXT("result.json"),O);if(!Written)Pass=false;
   if(auto* W=Web.Get())W->GetWorldTimerManager().ClearTimer(Timer);
   if(Pass){UE_LOG(LogS47HomesContract,Display,TEXT("S47: Caliche homes contract PASS north=%.2fm south=%.2fm walk=%.2fm PNGs=%d checks=%d evidence=%s; API, human input UNVERIFIED"),NorthMetres,SouthMetres,WalkMetres,Photos.Num(),Checks,*Root);FPlatformMisc::RequestExit(false);}
   else{UE_LOG(LogS47HomesContract,Error,TEXT("S47: Caliche homes contract FAIL stage=%d: %s; evidence=%s"),Stage,Written?*Why:TEXT("could not write complete result evidence"),*Root);FPlatformMisc::RequestExitWithStatus(false,1);}
  }
  bool Check(bool Okay,const FString& Why)
  {++Checks;if(!Okay){Result(false,Why);return false;}UE_LOG(LogS47HomesContract,Display,TEXT("S47: Caliche homes contract check PASS: %s"),*Why);return true;}
  void Next(int32 N,const TCHAR* Why)
  {
   Stage=N;At=FPlatformTime::Seconds();auto O=S47J::New();S47J::Set(O,TEXT("stage"),N);S47J::Set(O,TEXT("at_seconds"),At-Began);S47J::Set(O,TEXT("reason"),Why);Events.Add(MakeShared<FJsonValueObject>(O));
   UE_LOG(LogS47HomesContract,Display,TEXT("S47: Caliche homes contract stage %d: %s"),N,Why);
  }
  UStaticMeshComponent* Part(const FString& Name) const
  {auto* W=Web.Get();if(W)for(int32 I=0;I<W->PartCount();++I)if(const auto* N=W->Node(I);N&&N->Name==TEXT("caliche-homes:")+Name)return Cast<UStaticMeshComponent>(N->Comp.Get());return nullptr;}
  bool RoadPosts()
  {
   auto* M=Main();if(!M||WorldIdentity.Get()!=M->World||!RoadDrive||!RoadDrive->Area)return Check(false,TEXT("original road controller identity unavailable"));
   auto Rows=S47J::New();TArray<TSharedPtr<FJsonValue>> Values;
   for(int32 I=0;I<4;++I)
   {
    const double X=I<2?7990.+(I%2)*2.8:8007.2+(I%2)*2.8,Z=I<2?1895:2315,OldZ=I<2?1880:2335;
    const double B[]={X-.15,X+.15,Z-.15,Z+.15},Old[]={X-.15,X+.15,OldZ-.15,OldZ+.15};int32 Found=0,Stale=0;
    for(const auto& C:RoadDrive->Area->Obstacles){Found+=Rect(C,B);Stale+=Rect(C,Old);}
    if(!Check(Found==1&&Stale==0,FString::Printf(TEXT("actual road controller post %d moved exactly once, old footprint absent"),I)))return false;
    auto R=S47J::New();S47J::Set(R,TEXT("rect"),TArray<double>{B[0],B[1],B[2],B[3]});S47J::Set(R,TEXT("matches"),Found);S47J::Set(R,TEXT("stale_matches"),Stale);Values.Add(MakeShared<FJsonValueObject>(R));
   }
   Rows->SetArrayField(TEXT("posts"),Values);
   if(!SaveJson(Root/FString::Printf(TEXT("road-posts-stage-%d.json"),Stage),Rows)){Result(false,TEXT("could not write road-post evidence"));return false;}return true;
  }
  bool Preserve(const FString& Name,bool Capture=false)
  {
   auto* M=Main();auto* W=Web.Get();TArray<FString> Lines;
   if(!Check(W&&W->AuditCalicheHomes(Lines),TEXT("AuditCalicheHomes at ")+Name))return false;
   for(const auto& Line:Lines)UE_LOG(LogS47HomesContract,Display,TEXT("S47: homes audit %s"),*Line);
   if(Capture)
   {
    int32 MainZones=0;
    // World.Enter(diner) aliases the exported road walkzones (Web WalkKey).
    for(const auto& Z:W->ZonesMutable(TEXT("road")))if(Z.Id==TEXT("main"))
    {
     if(!Check(Z.bEnabled&&FMath::Abs(Z.MinX-7982.4)<1e-6&&FMath::Abs(Z.MaxX-8019.5)<1e-6&&FMath::Abs(Z.MinZ-2015)<1e-6&&FMath::Abs(Z.MaxZ-2256)<1e-6,TEXT("street-only contract uses unchanged production main zone bounds")))return false;
     ++MainZones;
    }
    if(!Check(MainZones==1,TEXT("exactly one original main street walkzone")))return false;
    const auto Source=ReadJson(FPaths::ProjectContentDir()/TEXT("WebData/manifest.json"));const auto Nodes=S47J::Arr(Source,TEXT("nodes"));
    const auto Assembly=ReadJson(FPaths::ProjectContentDir()/TEXT("CalicheHomes/assembly.json"));TSet<int32> Ids;
    for(int32 I:ProtectedNodes)Ids.Add(I);
    for(const auto& V:S47J::Arr(Assembly,TEXT("protected_nodes")))Ids.Add(S47J::Int(V->AsObject(),TEXT("i"),-1));
    for(const auto& Path:S47J::Strs(Assembly,TEXT("protected_handles")))Handles.Add(Path,W->Handle(Path));
    for(const TCHAR* Path:ProxyPaths)Handles.Add(Path,W->Handle(Path));
    Handles.Add(TEXT("world.town.smoker"),W->Handle(TEXT("world.town.smoker")));
    for(const TCHAR* Prefix:{TEXT("world.town.runtime.panels.tv1."),TEXT("world.town.runtime.panels.tv2."),TEXT("world.town.runtime.panels.warm.")})
     for(const auto& Path:W->PathsUnder(Prefix))Handles.Add(Path,W->Handle(Path));
    for(const auto& H:Handles)
    {
     if(!Check(H.Value>=0,TEXT("original handle exists: ")+H.Key))return false;
     W->ObjAt(H.Value).Traverse([&](FS47Obj O){Ids.Add(O.I);});
    }
    TArray<int32> Ordered=Ids.Array();Ordered.Sort();int32 AnomalyDetails=0;
    for(int32 I:Ordered)
    {
     const auto* N=W->Node(I);auto* C=N?N->Comp.Get():nullptr;
     if(!Check(N&&C,FString::Printf(TEXT("protected live component node %d"),I)))return false;
     FVector P=FVector::ZeroVector,S=FVector::OneVector;FQuat Q=FQuat::Identity;FString Policy=TEXT("original exported local pose");FJ Src;
     if(Nodes.IsValidIndex(I))
     {
      Src=Nodes[I]->AsObject();const auto Matrix=S47J::Nums(Src,TEXT("m"));
      if(!Check(Matrix.Num()==16&&S47J::Int(Src,TEXT("i"),-1)==I,TEXT("source matrix identity")))return false;
      S47Three::Decompose(Matrix.GetData(),P,Q,S);
      // AnomalyWorld.Begin/PlaceFigure uses the ordinary A02 reference position.
      // Only this known movable root differs from the old export at z1450.
      if(I==1493)
      {
       if(!Check(W->Handle(TEXT("world.town.cutout"))==I&&W->Handle(TEXT("world.town.proxies.souvenir"))==1524&&Nodes.IsValidIndex(1524)&&M->Anomaly&&!M->Anomaly->Data.bLegacyCutout&&!M->Anomaly->Data.A02Moved&&!M->Anomaly->Data.A02FacingSet&&M->TownLayer->Town->bAnomalyCutoutOwned,TEXT("cutout uses the normal fresh A02 reference, no later movement or legacy fixture")))return false;
       const auto Anchor=Nodes[1524]->AsObject();const auto AM=S47J::Nums(Anchor,TEXT("m"));
       if(!Check(AM.Num()==16&&S47J::Int(Anchor,TEXT("p"),-1)==1431&&N->Parent==1431,TEXT("A02 reference height comes from original souvenir proxy under town")))return false;
       P=FVector(20.31,AM[13]-.62,2062.3);Q=S47Three::Euler(0,UE_DOUBLE_PI/2,0);
       Policy=TEXT("ordinary AnomalyWorld.PlaceFigure fresh A02 reference: x20.31, original souvenir proxy Y-.62, z2062.3, yaw PI/2");
      }
     }
     else
     {
      // These exact authored children are adopted by AnomalyWorld.InstallGeometry.
      // They have no old manifest row. Do not exempt unknown native descendants.
      if(N->Name==TEXT("AnomalyPriceTag")){P=FVector(-.16,.37,-.028);S=FVector(.22,.13,.018);}
      else if(N->Name==TEXT("AnomalySplitEdge")){P=FVector(.115,.17,-.029);S=FVector(.012,.15,.012);}
      else if(N->Name==TEXT("AnomalySplitEdgeNick")){P=FVector(.12,.235,-.03);S=FVector(.04,.018,.014);}
      else{Result(false,FString::Printf(TEXT("unknown non-exported protected descendant %d %s"),I,*N->Name));return false;}
      const auto* Static=Cast<UStaticMeshComponent>(C);
      if(!Check(N->Parent==1493&&N->Geo==-1&&N->Mats.IsEmpty()&&Static&&GetPathNameSafe(Static->GetStaticMesh())==TEXT("/Engine/BasicShapes/Cube.Cube"),TEXT("exact ordinary AnomalyWorld cutout detail identity: ")+N->Name))return false;
      Policy=TEXT("ordinary AnomalyWorld.InstallGeometry authored cutout detail; explicit name/parent/mesh/P/Q/S");++AnomalyDetails;
     }
     double ExpectedMatrix[16];S47Three::Compose(P,Q,S,ExpectedMatrix);const FTransform Expected=S47Three::MatToUE(ExpectedMatrix);
     const bool PoseOkay=N->P.Equals(P,1e-7)&&N->S.Equals(S,1e-7)&&N->Q.Equals(Q,1e-7)&&Same(C->GetRelativeTransform(),Expected);
     if(!PoseOkay||I==1493||!Src)
     {
      auto O=S47J::New();S47J::Set(O,TEXT("node"),I);S47J::Set(O,TEXT("name"),N->Name);S47J::Set(O,TEXT("pose_policy"),Policy);if(Src)S47J::Set(O,TEXT("original_export"),Src);
      Vector(O,TEXT("expected_metadata_p"),P);Vector(O,TEXT("actual_metadata_p"),N->P);Vector(O,TEXT("expected_metadata_s"),S);Vector(O,TEXT("actual_metadata_s"),N->S);
      S47J::Set(O,TEXT("expected_metadata_q"),TArray<double>{Q.X,Q.Y,Q.Z,Q.W});S47J::Set(O,TEXT("actual_metadata_q"),TArray<double>{N->Q.X,N->Q.Y,N->Q.Z,N->Q.W});
      S47J::Set(O,TEXT("expected_relative"),Transform(Expected));S47J::Set(O,TEXT("actual_relative"),Transform(C->GetRelativeTransform()));S47J::Set(O,TEXT("pass"),PoseOkay);
      if(!SaveJson(Root/FString::Printf(TEXT("protected-pose-%d.json"),I),O)){Result(false,TEXT("could not write protected pose readback"));return false;}
     }
     if(!Check(PoseOkay,FString::Printf(TEXT("metadata and actual rendered local pose %d obey %s"),I,*Policy)))return false;
     FProtected R;R.Node=I;R.Parent=N->Parent;R.Geo=N->Geo;R.Mats=N->Mats;R.Comp=C;R.Relative=C->GetRelativeTransform();R.P=N->P;R.Q=N->Q;R.S=N->S;
     R.PosePolicy=Policy;
     if(auto* Static=Cast<UStaticMeshComponent>(C))R.Mesh=Static->GetStaticMesh();
     if(auto* Primitive=Cast<UPrimitiveComponent>(C))for(int32 Slot=0;Slot<Primitive->GetNumMaterials();++Slot)R.Materials.Add(TWeakObjectPtr<UMaterialInterface>(Primitive->GetMaterial(Slot)));
     Protected.Add(MoveTemp(R));
    }
    if(!Check(AnomalyDetails==3,TEXT("all three ordinary AnomalyWorld cutout detail descendants retained")))return false;
    for(int32 I=0;I<4;++I)
    {
     const auto* It=M->Inter->Get(ProxyIds[I]);if(!Check(It&&It->Object.I==W->Handle(ProxyPaths[I])&&FMath::Abs(It->Range-ProxyRanges[I])<1e-8,TEXT("original proxy binding/range: ")+FString(ProxyIds[I])))return false;
     Bindings.Add({ProxyPaths[I],ProxyIds[I],It->Object.I,It->Range,It->ExamineRange,It->bLook,bool(It->Use),bool(It->Examine)});
    }
   }
   auto Evidence=S47J::New();S47J::Set(Evidence,TEXT("phase"),Name);TArray<TSharedPtr<FJsonValue>> Rows;
   for(const auto& R:Protected)
   {
    const auto* N=W->Node(R.Node);auto* C=N?N->Comp.Get():nullptr;auto* Static=Cast<UStaticMeshComponent>(C);auto* Primitive=Cast<UPrimitiveComponent>(C);
    bool Okay=N&&C==R.Comp.Get()&&N->Parent==R.Parent&&N->Geo==R.Geo&&N->Mats==R.Mats&&N->P.Equals(R.P,1e-7)&&N->S.Equals(R.S,1e-7)&&N->Q.Equals(R.Q,1e-7)&&Same(C->GetRelativeTransform(),R.Relative)&&(!R.Mesh.IsValid()||(Static&&Static->GetStaticMesh()==R.Mesh.Get()));
    Okay&=(!Primitive&&R.Materials.IsEmpty())||(Primitive&&Primitive->GetNumMaterials()==R.Materials.Num());
    if(Okay&&Primitive)for(int32 I=0;I<R.Materials.Num();++I)Okay&=Primitive->GetMaterial(I)==R.Materials[I].Get();
    if(!Check(Okay,FString::Printf(TEXT("preserved pose/parent/mesh/material binding node %d at %s"),R.Node,*Name)))return false;
    auto O=S47J::New();S47J::Set(O,TEXT("node"),R.Node);S47J::Set(O,TEXT("name"),N->Name);S47J::Set(O,TEXT("parent"),N->Parent);S47J::Set(O,TEXT("geo"),N->Geo);
    S47J::Set(O,TEXT("pose_policy"),R.PosePolicy);
    S47J::Set(O,TEXT("mesh"),GetPathNameSafe(Static?Static->GetStaticMesh():nullptr));S47J::Set(O,TEXT("actual_relative"),Transform(C->GetRelativeTransform()));S47J::Set(O,TEXT("actual_world"),Transform(C->GetComponentTransform()));
    TArray<FString> Materials;for(const auto& V:R.Materials)Materials.Add(GetPathNameSafe(V.Get()));S47J::Set(O,TEXT("materials"),Materials);S47J::Set(O,TEXT("own_visible"),C->IsVisible());Rows.Add(MakeShared<FJsonValueObject>(O));
   }
   for(const auto& H:Handles)if(!Check(W->Handle(H.Key)==H.Value,TEXT("handle binding retained: ")+H.Key))return false;
   TArray<TSharedPtr<FJsonValue>> Interactions;
   for(int32 I=0;I<Bindings.Num();++I)
   {
    const auto& B=Bindings[I];const auto* It=M->Inter->Get(B.Id);
    const FString Expected=M->World->Area!=TEXT("diner")?FString():I==3?(M->TownLayer->Town->bSmokerIn?FString(TEXT("Man on the stoop")):FString()):FString(TEXT("Window"));
    if(!Check(It&&It->Object.I==B.Node&&W->Handle(B.Path)==B.Node&&It->Range==B.Range&&It->ExamineRange==B.ExamineRange&&It->bLook==B.Look&&bool(It->Use)==B.Use&&bool(It->Examine)==B.Examine&&M->Inter->LabelOf(B.Id)==Expected,TEXT("live context/proxy/callback binding: ")+B.Id))return false;
    auto O=S47J::New();S47J::Set(O,TEXT("id"),B.Id);S47J::Set(O,TEXT("node"),B.Node);S47J::Set(O,TEXT("range_m"),B.Range);S47J::Set(O,TEXT("label"),Expected);S47J::Set(O,TEXT("callbacks_present"),B.Use);Interactions.Add(MakeShared<FJsonValueObject>(O));
   }
   Evidence->SetArrayField(TEXT("protected_nodes"),Rows);Evidence->SetArrayField(TEXT("proxy_bindings"),Interactions);S47J::Set(Evidence,TEXT("dynamic_visibility_and_light_colors"),TEXT("Production schedule remains active; their values are recorded, not frozen"));
   if(!SaveJson(Root/(Name+TEXT(".json")),Evidence)){Result(false,TEXT("could not write protected-node evidence"));return false;}return true;
  }
  void QueuePhoto(const FString& Name,UStaticMeshComponent* Target)
  {PhotoName=Name;PhotoTarget=Target;PhotoRequested=false;PhotoAt=FPlatformTime::Seconds();PhotoMeta=S47J::New();}
  bool Photo()
  {
   auto* M=Main();auto* D=M->World->CabDrive();auto* T=M->World->CabTruck();auto* C=PhotoTarget.Get();
   if(!D||!T||!C||!C->IsVisible()||C->bHiddenInGame){Result(false,TEXT("live cockpit/native photo target missing"));return false;}
   FVector Target=C->Bounds.Origin;if(PhotoName.Contains(TEXT("sign")))Target.Z+=C->Bounds.BoxExtent.Z*.45;
   const FVector Direction=S47Three::ToThree(Target)-D->CamPos;
   const FQuat Body=S47Three::Euler(T->Rotation.X,T->Rotation.Y,T->Rotation.Z,TEXT("YXZ"));const FVector Local=Body.UnrotateVector(Direction);
   const double Yaw=FMath::Atan2(-Local.X,-Local.Z),Pitch=FMath::Atan2(Local.Y,FVector2D(Local.X,Local.Z).Size());
   if(FMath::Abs(Yaw)>1.85||Pitch<-.88||Pitch>.58){Result(false,TEXT("photo target outside ordinary cab head limits"));return false;}D->SetLook(Yaw,Pitch);
   const double Now=FPlatformTime::Seconds();const FString File=Root/(PhotoName+TEXT(".png"));
   if(!PhotoRequested)
   {
    if(Now-PhotoAt<.4)return false;
    const FVector Eye=M->Player->EyeUE(),Forward=M->Player->ForwardUE();FVector2D Pixel,Size;auto* PC=Cast<APlayerController>(M->Player->GetController());
    if(GEngine&&GEngine->GameViewport)GEngine->GameViewport->GetViewportSize(Size);
    if(!Check(PC&&Size.X>=320&&Size.Y>=200&&PC->ProjectWorldLocationToScreen(Target,Pixel,true)&&Pixel.X>=0&&Pixel.X<Size.X&&Pixel.Y>=0&&Pixel.Y<Size.Y&&FVector::DotProduct(Forward,(Target-Eye).GetSafeNormal())>.98&&S47Three::ToThree(Eye).Equals(D->CamPos,.08),TEXT("actual settled cockpit camera projects native target: ")+PhotoName))return false;
    Vector(PhotoMeta,TEXT("eye_three_m"),S47Three::ToThree(Eye));Vector(PhotoMeta,TEXT("target_three_m"),S47Three::ToThree(Target));Vector(PhotoMeta,TEXT("truck_three_m"),T->Position);
    S47J::Set(PhotoMeta,TEXT("target_mesh"),GetPathNameSafe(C->GetStaticMesh()));S47J::Set(PhotoMeta,TEXT("heading"),D->Heading);S47J::Set(PhotoMeta,TEXT("speed_m_s"),D->Speed);
    S47J::Set(PhotoMeta,TEXT("target_pixel"),TArray<double>{Pixel.X,Pixel.Y});S47J::Set(PhotoMeta,TEXT("viewport"),TArray<double>{Size.X,Size.Y});
    S47J::Set(PhotoMeta,TEXT("fixture"),Track==2?TEXT("explicit isolated southbound saved-car start; actual cockpit/World stepping"):TEXT("ordinary northbound BeginCommute route; no vehicle relocation"));
    FScreenshotRequest::RequestScreenshot(File,true,false);PhotoRequested=true;return false;
   }
   FJ Receipt;FString Why;
   if(!PngReceipt(File,Receipt,Why)){if(Now-PhotoAt>12)Result(false,PhotoName+TEXT(": ")+Why);return false;}
   S47J::Set(PhotoMeta,TEXT("png"),Receipt);
   if(!SaveJson(Root/(PhotoName+TEXT(".capture.json")),PhotoMeta)){Result(false,TEXT("could not write cockpit capture evidence"));return false;}Photos.Add(MakeShared<FJsonValueObject>(PhotoMeta));
   UE_LOG(LogS47HomesContract,Display,TEXT("S47: Caliche homes cockpit PNG PASS %s; complete CRC/IEND"),*PhotoName);
   PhotoName.Empty();PhotoTarget.Reset();PhotoMeta.Reset();return true;
  }
  void Brake()
  {
   auto* M=Main();M->World->Autopilot.Reset();TWeakObjectPtr<US47World> Weak=M->World;
   M->World->TestInput=[Weak]{FS47DriveIn In;if(auto* W=Weak.Get()){const double V=W->DrivingSpeed();In.Throttle=V>.06?-1:V<-.06?1:0;}return In;};
  }
  bool Monitor(double Now)
  {
   auto* M=Main();if(!Track)return true;const auto* T=M->World->CabTruck();const auto* D=M->World->CabDrive();
   if(!T||!D||M->World->Leg!=TEXT("road")||!M->World->bCommuteVehicle){Result(false,TEXT("continuous route lost ordinary road/car controller"));return false;}
   const double Dt=FMath::Max(.001,Now-Last),Moved=FVector2D::Distance(FVector2D(T->Position.X,T->Position.Z),FVector2D(LastTruck.X,LastTruck.Z));
   if(Moved>FMath::Max(2.,FMath::Max(FMath::Abs(LastSpeed),FMath::Abs(D->Speed))*Dt+1.)||T->Position.X<7995||T->Position.X>8005)
   {Result(false,TEXT("vehicle discontinuity or departure from existing main road"));return false;}
   if(Track==1){NorthMetres+=Moved;for(int32 I=0;I<4;++I)Passed[I]|=T->Position.Z<Houses[I].Z-6;}
   else SouthMetres+=Moved;
   if(FMath::Abs(T->Position.Z-LastProgressZ)>1.){ProgressAt=Now;LastProgressZ=T->Position.Z;}
   if((Stage==2||Stage==6)&&Now-ProgressAt>12){Result(false,TEXT("normal road controller stalled; no placement fallback"));return false;}
   LastTruck=T->Position;Last=Now;LastSpeed=D->Speed;return true;
  }
  void TrackFrom(int32 Kind)
  {auto* M=Main();Track=Kind;LastTruck=M->World->CabTruck()->Position;LastSpeed=M->World->DrivingSpeed();Last=ProgressAt=FPlatformTime::Seconds();LastProgressZ=LastTruck.Z;}
  bool Floor()
  {
   auto* M=Main();auto* P=M->Player.Get();if(!Check(InMain(P->Pos,P->Radius)&&P->Walkable(P->Pos.X,P->Pos.Z),TEXT("Walker remains inside original main walkzone")))return false;
   FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47HomesStreetFloor),true,P);
   const bool Hit=Web->GetWorld()->LineTraceSingleByChannel(H,S47Three::ToUE(P->Pos.X,P->FloorY+.12,P->Pos.Z),S47Three::ToUE(P->Pos.X,P->FloorY-.35,P->Pos.Z),ECC_Visibility,Q);
   if(!Check(Hit&&FMath::Abs(S47Three::ToThree(H.ImpactPoint).Y-P->FloorY)<.08,TEXT("actual rendered street ground under normal Walker")))return false;
   return true;
  }
  bool Restored()
  {
   auto* M=Main();const auto Actual=M->Flow->GameState();const auto OldEvening=S47J::Obj(BeforeSave,TEXT("evening")),NewEvening=S47J::Obj(Actual,TEXT("evening"));const auto A=S47J::Obj(OldEvening,TEXT("car")),B=S47J::Obj(NewEvening,TEXT("car"));
   bool Okay=M->Evening->Commute()&&!M->World->bDriving&&M->World->Area==TEXT("diner")&&M->World->TruckAt==TEXT("diner")&&M->Player->Pos.Equals(SavedFoot,.015)&&FMath::Abs(Angle(M->Player->Yaw-SavedYaw))<.001&&FMath::Abs(M->Player->Pitch-SavedPitch)<.001&&FMath::Abs(M->Story->Clock()-SavedClock)<.001&&SameDoors(M->Web->DoorStates(),Doors);
   for(const TCHAR* K:{TEXT("x"),TEXT("y"),TEXT("z"),TEXT("h")})Okay&=A&&B&&FMath::Abs(S47J::Num(A,K)-S47J::Num(B,K))<.015;
   Okay&=A&&B&&!S47J::Bool(B,TEXT("driving"))&&S47J::Str(A,TEXT("at"))==S47J::Str(B,TEXT("at"))&&SameJson(S47J::Obj(OldEvening,TEXT("pack")),S47J::Obj(NewEvening,TEXT("pack")));
   auto O=S47J::New();S47J::Set(O,TEXT("expected"),BeforeSave);S47J::Set(O,TEXT("actual"),Actual);S47J::Set(O,TEXT("pass"),Okay);
   if(!SaveJson(Root/TEXT("continue-readback.json"),O)){Result(false,TEXT("could not write Continue readback evidence"));return false;}
   return Check(Okay,TEXT("ordinary full Flow Continue preserved clock/car/pack/player/doors"));
  }
  void Tick()
  {
   const TSharedRef<FControl> KeepAlive=AsShared();
   if(Done)return;const double Now=FPlatformTime::Seconds();auto* M=Main();
   if(Now-Began>240){Result(false,TEXT("240 s contract timeout"));return;}
   if(!M||!M->Flow||!M->World||!M->Player||!M->Story||!M->UI||!M->Inter||!M->Evening||!M->TownLayer||!M->TownLayer->Town||S47Startup::Blocking()){ReadyAt=0;return;}
   if(!Started)
   {
    if(M->Flow->bStarting||M->World->bBusy){ReadyAt=0;return;}
    if(ReadyAt==0)ReadyAt=Now;if(Now-ReadyAt<2)return;
    if(!Check(M->Flow->bDevRun&&M->Flow->CaseId==0&&M->Evening->Commute()&&M->World->bDriving&&M->World->Leg==TEXT("road")&&M->World->CabTruck()&&M->World->CabTruck()->Position.Z>2334.99&&M->World->CabTruck()->Position.Z<2335.01&&FMath::Abs(M->World->CabDrive()->Heading)<.001,TEXT("actual fresh BeginCommute start z2335 heading north; requires -S47Evening=commute")))return;
    WorldIdentity=M->World;RoadDrive=M->World->CabDrive();if(!Preserve(TEXT("protected-before"),true)||!RoadPosts())return;
    Started=true;TrackFrom(1);Brake();QueuePhoto(TEXT("south-sign-northbound-start"),Part(TEXT("SM_Homes_SignSouth")));Next(1,TEXT("ordinary stationary commute cockpit south-sign capture"));return;
   }
   if(!Monitor(Now))return;
   if(M->Flow->bStarting||M->World->bBusy)return;
   if(Stage!=10&&M->Mode!=US47Main::EMode::Play){Result(false,TEXT("unexpected title/end during physical route"));return;}
   if(Stage==1)
   {if(!Photo())return;M->World->TestInput=nullptr;M->World->Autopilot=8.;ProgressAt=Now;Next(2,TEXT("continuous normal northbound Autopilot; no pose writes"));return;}
   if(Stage==2)
   {
    if(!PhotoName.IsEmpty()){Photo();return;}
    if(HouseShot<4&&M->World->CabTruck()->Position.Z<Houses[HouseShot].Z+23)
    {const FString House=Houses[HouseShot++].Name;QueuePhoto(House+TEXT("-northbound-cab"),Part(TEXT("SM_Homes_")+House+TEXT("_Shell")));return;}
    if(M->World->CabTruck()->Position.Z<=1870){Brake();Next(3,TEXT("normal braking after all four houses and both sign locations"));}return;
   }
   if(Stage==3)
   {
    if(FMath::Abs(M->World->DrivingSpeed())>.06)return;
    if(!Check(NorthMetres>460&&HouseShot==4&&Passed[0]&&Passed[1]&&Passed[2]&&Passed[3]&&Photos.Num()==5,TEXT("continuous measured northbound route passed four native houses and both original-sign locations"))||!Preserve(TEXT("protected-after-north")))return;
    Track=0;auto Car=S47J::New();S47J::Set(Car,TEXT("leg"),TEXT("road"));S47J::Set(Car,TEXT("x"),7998.2);S47J::Set(Car,TEXT("y"),0.);S47J::Set(Car,TEXT("z"),1875.);S47J::Set(Car,TEXT("h"),UE_DOUBLE_PI);
    if(!SaveJson(Root/TEXT("explicit-southbound-start-fixture.json"),Car)){Result(false,TEXT("could not write southbound start fixture evidence"));return;}M->World->TestInput=nullptr;M->World->BeginCommute(Car);Next(4,TEXT("EXPLICIT FIXTURE: isolated southbound saved-car start z1875 heading PI"));return;
   }
   if(Stage==4)
   {
    if(!M->World->bDriving)return;
    if(!Check(M->World->CabTruck()&&FMath::Abs(M->World->CabTruck()->Position.X-7998.2)<.01&&FMath::Abs(M->World->CabTruck()->Position.Z-1875)<.01&&FMath::Abs(Angle(M->World->CabDrive()->Heading-UE_DOUBLE_PI))<.001,TEXT("southbound fixture placed only at fragment start")))return;
    TrackFrom(2);Brake();if(!RoadPosts())return;QueuePhoto(TEXT("north-sign-southbound-fixture-start"),Part(TEXT("SM_Homes_SignNorth")));Next(5,TEXT("actual southbound cockpit north-sign capture"));return;
   }
   if(Stage==5)
   {
    if(!Photo())return;TWeakObjectPtr<US47World> Weak=M->World;M->World->Autopilot.Reset();
    M->World->TestInput=[Weak]{FS47DriveIn In;if(auto* W=Weak.Get())if(auto* D=W->CabDrive()){const double Aim=FMath::Atan2(-(7998.2-D->Pos.X),-12.);In.Steer=FMath::Clamp(-Angle(Aim-D->Heading)*1.5,-.4,.4);In.Throttle=FMath::Clamp((8.-D->Speed)*.5,-1.,1.);}return In;};
    M->World->CabDrive()->SetLook(0,-.04);ProgressAt=Now;Next(6,TEXT("normal southbound World.TestInput to original main street walkzone"));return;
   }
   if(Stage==6)
   {if(M->World->CabTruck()->Position.Z>=2206){Brake();Next(7,TEXT("normal braking for street egress"));}return;}
   if(Stage==7)
   {
    if(FMath::Abs(M->World->DrivingSpeed())>.06)return;
    if(!Check(SouthMetres>330&&InMain(M->World->CabTruck()->Position)&&M->World->CanGetOut(),TEXT("southbound fragment stopped normally inside original main zone")))return;
    Track=0;M->World->TestInput=nullptr;if(!Check(M->World->GetOutHere(),TEXT("ordinary GetOutHere supplies actual egress pose")))return;Next(8,TEXT("production parking handover; no foot placement fixture"));return;
   }
   if(Stage==8)
   {
    if(Now-At<.8)return;if(!Floor())return;FootStart=LastFoot=M->Player->Pos;Walk={FootStart+FVector(2,0,0),FootStart+FVector(2,0,-4),FootStart};Waypoint=0;Last=Now;
    for(const auto& P:Walk)if(!Check(InMain(P,M->Player->Radius)&&M->Player->Walkable(P.X,P.Z),TEXT("short street waypoint belongs to existing walkzone, no facade access")))return;
    Next(9,TEXT("normal Walker.TestMove on original main street only"));return;
   }
   if(Stage==9)
   {
    const double Moved=FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(LastFoot.X,LastFoot.Z));
    if(Moved>FMath::Max(.5,(Now-Last)*8+.1)||!InMain(M->Player->Pos,M->Player->Radius)){Result(false,TEXT("Walker discontinuity or left original zone"));return;}
    WalkMetres+=Moved;LastFoot=M->Player->Pos;Last=Now;
    if(Waypoint<Walk.Num())
    {
     const FVector Delta=Walk[Waypoint]-M->Player->Pos;const double Distance=FVector2D(Delta.X,Delta.Z).Size();
     if(Distance<.09){M->Player->TestMove.Reset();if(!Floor())return;++Waypoint;return;}
     if(Now-At>35){Result(false,TEXT("physical street walk blocked; no placement fallback"));return;}
     M->Player->Yaw=FMath::Atan2(-Delta.X,-Delta.Z);M->Player->Pitch=-.06;M->Player->TestMove=FVector2D(0,-FMath::Min(1.,Distance/.5));M->Player->bTestRun=false;return;
    }
    M->Player->TestMove.Reset();if(!Check(WalkMetres>8&&M->Flow->GameSaveBlock().IsEmpty(),TEXT("real street walk complete and ordinary gameplay save boundary available"))||!Preserve(TEXT("protected-before-save")))return;
    BeforeSave=M->Flow->GameState();SavedFoot=M->Player->Pos;SavedYaw=M->Player->Yaw;SavedPitch=M->Player->Pitch;SavedClock=M->Story->Clock();Doors=M->Web->DoorStates();
    FS47SaveStore Writer(Root/TEXT("isolated-case1"));Writer.Init();FS47SaveCard Card;Card.Chapter=TEXT("ENV02 commute control");Card.Place=M->Flow->PlaceName();Card.Clock=M->Story->ClockText(false);Card.Playtime=M->Playtime;
    FString Error;const auto Meta=Writer.Save(1,TEXT("manual"),0,BeforeSave,Card,Error);if(!Check(Meta.IsSet(),TEXT("EXPLICIT FIXTURE: copy actual developer GameState to isolated case1 disk: ")+Error))return;
    FS47SaveStore Reader(Writer.Dir);Reader.Init();const auto Disk=Reader.Load(Meta->Id);if(!Check(SameJson(Disk,BeforeSave),TEXT("fresh store reads complete actual snapshot from disk")))return;
    SaveId=Meta->Id;if(!SaveJson(Root/TEXT("before-continue.disk.json"),Disk)){Result(false,TEXT("could not write pre-Continue disk evidence"));return;}M->Flow->Saves=FS47SaveStore(Writer.Dir);M->Flow->Saves.Init();M->Flow->ToTitle();Next(10,TEXT("ordinary title; case1 fixture stays inside isolated /tmp profile"));return;
   }
   if(Stage==10)
   {
    if(Now-At<.6)return;if(!Check(M->Mode==US47Main::EMode::Title&&M->Flow->Saves.ContinueSave()&&M->Flow->Saves.ContinueSave()->Id==SaveId,TEXT("ordinary title Continue resolves isolated disk fixture")))return;
    M->Flow->OpenTestView(TEXT("continue"));Next(11,TEXT("ordinary OpenTestView continue / OpenSave / full Flow.StartGame"));return;
   }
   if(Stage==11)
   {
    if(Now-At<.8)return;
    if(!M->Flow->SaveBlock().IsEmpty()){if(Now-At>15)Result(false,TEXT("ordinary post-Continue SaveBlock did not clear: ")+M->Flow->SaveBlock());return;}
    if(!Restored()||!Preserve(TEXT("protected-after-continue"))||!RoadPosts()||!Floor())return;
    const auto ExpectedDisk=M->Flow->GameState();
    FString Error;const auto Meta=M->Flow->WriteSave(TEXT("manual"),1,Error);if(!Check(Meta.IsSet(),TEXT("ordinary Flow.WriteSave after Continue at legal boundary: ")+Error))return;
    FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();const auto Disk=Reader.Load(Meta->Id);
    if(!Check(Disk&&S47J::Str(Disk,TEXT("checkpoint"))==TEXT("evening")&&SameJson(Disk,ExpectedDisk),TEXT("fresh reader finds complete ordinary post-Continue snapshot")))return;
    if(!SaveJson(Root/TEXT("after-continue-normal-save.disk.json"),Disk)){Result(false,TEXT("could not write post-Continue disk evidence"));return;}Next(12,TEXT("full PNG/IEND final receipts before PASS"));return;
   }
   if(Stage==12)
   {
    if(!Check(Photos.Num()==6,TEXT("both signs and all four house cockpit captures present")))return;
    for(const auto& V:Photos)
    {
     const auto R=S47J::Obj(V->AsObject(),TEXT("png"));FJ Fresh;FString Why;
     if(!Check(PngReceipt(S47J::Str(R,TEXT("file")),Fresh,Why)&&S47J::Str(Fresh,TEXT("sha1"))==S47J::Str(R,TEXT("sha1")),TEXT("final whole PNG CRC/IEND/hash receipt: ")+S47J::Str(R,TEXT("file"))))return;
    }
    Result(true,TEXT("ordinary northbound commute, explicit southbound control, existing-zone Walker, protected poses/bindings and isolated full Flow Continue"));
   }
  }
 };
}

void AS47Web::BeginCalicheHomesContract()
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47CalicheHomesContract")))return;
 FString UserDir,Evening;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);UserDir=FPaths::ConvertRelativePathToFull(UserDir);FPaths::CollapseRelativeDirectories(UserDir);
 const bool Safe=(UserDir.StartsWith(TEXT("/tmp/"))||UserDir.StartsWith(TEXT("/var/tmp/")))&&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),UserDir);
 bool Conflicting=FParse::Param(FCommandLine::Get(),TEXT("S47Title"));FString Value;
 for(const TCHAR* Key:{TEXT("S47Case="),TEXT("S47EveningRoute="),TEXT("S47Drive="),TEXT("S47Walk="),TEXT("S47WalkFrom="),TEXT("S47Face=")})Conflicting|=FParse::Value(FCommandLine::Get(),Key,Value);
 for(const TCHAR* Flag:{TEXT("S47MotelV2Route"),TEXT("S47GetOut"),TEXT("S47EveningEntrance"),TEXT("S47SaroParking"),TEXT("S47NightSequenceContract"),TEXT("S47T01DriveContract"),TEXT("S47SmokeTest")})Conflicting|=FParse::Param(FCommandLine::Get(),Flag);
 if(!Safe||Conflicting||!FParse::Value(FCommandLine::Get(),TEXT("S47Evening="),Evening)||Evening!=TEXT("commute"))
 {UE_LOG(LogS47HomesContract,Error,TEXT("S47: Caliche homes contract FAIL requires isolated /tmp UserDir, -S47Evening=commute, case0 and no competing route driver"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
 const auto S=MakeShared<FControl>();S->Web=this;S->Root=FPaths::ProjectSavedDir()/TEXT("CalicheHomesContract")/FGuid::NewGuid().ToString(EGuidFormats::Digits);
 if(!IFileManager::Get().MakeDirectory(*S->Root,true)){UE_LOG(LogS47HomesContract,Error,TEXT("S47: Caliche homes contract FAIL evidence directory"));FPlatformMisc::RequestExitWithStatus(false,1);return;}
 GetWorldTimerManager().SetTimer(S->Timer,[S]{S->Tick();},.05f,true);
 UE_LOG(LogS47HomesContract,Display,TEXT("S47: Caliche homes contract armed; ordinary commute north, explicitly separate southbound and disk fixtures; API control, human input UNVERIFIED; evidence=%s"),*S->Root);
}

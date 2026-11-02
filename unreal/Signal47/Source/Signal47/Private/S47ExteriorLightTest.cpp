#include "S47ExteriorLightTest.h"
#include "S47Main.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47Walker.h"
#include "S47UI.h"
#include "S47Startup.h"
#include "S47Three.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformProperties.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"

namespace
{
 struct FTarget {const TCHAR* Name;const TCHAR* Area;FVector Eye,At;};
 const FTarget Targets[]={
  {TEXT("office-exterior"),TEXT("saro"),FVector(-40.50,-.45,674.10),FVector(-43.205,.44143,674.10)},
  {TEXT("office-ceiling"),TEXT("saro"),FVector(-45.50,-.48,675.90),FVector(-45.50,.52343,675.20)},
  {TEXT("laundry-exterior"),TEXT("diner"),FVector(7988.4,9.63,2136.22),FVector(7985.34,10.70,2136.22)}
 };
 struct FAllowed {const TCHAR* Name;int32 Slot;const TCHAR* Material;int32 Driver;float Reference;};
 const FAllowed Allowed[]={
  {TEXT("SM_Sierra_GlowOffice"),0,TEXT("M_MotelV2_GlowOfficeDaylight"),0,6},
  {TEXT("SM_Sierra_GlowExterior"),0,TEXT("M_MotelV2_GlowExterior"),-1,1},
  {TEXT("SM_Sierra_GlowRoom06"),0,TEXT("M_MotelV2_GlowRoom06"),6,4},
  {TEXT("SM_Sierra_GlowSign"),0,TEXT("M_MotelV2_GlowSign"),8,9},
  {TEXT("SM_Access_Laundry_Fixtures"),10,TEXT("M_Town_WarmGlow"),-1,1},
  {TEXT("SM_Access_Feed_Fixtures"),10,TEXT("M_Town_WarmGlow"),-1,1},
  {TEXT("SM_Access_Church_Fixtures"),6,TEXT("M_Town_ChurchPorchGlow"),-1,1},
  {TEXT("SM_Access_Cinema_Fixtures"),4,TEXT("M_Town_CinemaBulb"),-1,1},
  {TEXT("SM_Access_Cinema_Fixtures"),7,TEXT("M_Town_CinemaReaderGlow"),-1,1},
  {TEXT("SM_Access_Cinema_Fixtures"),6,TEXT("M_Town_CinemaNeonRed"),-1,1}
 };
 FString OriginalMaterialHash()
 {
  if(FPlatformProperties::RequiresCookedData())return LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/MotelV2/Materials/M_MotelV2_GlowOffice.M_MotelV2_GlowOffice"))?TEXT("COOKED_RESOURCE_IDENTITY_ONLY"):FString();
  TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*(FPaths::ProjectContentDir()/TEXT("MotelV2/Materials/M_MotelV2_GlowOffice.uasset"))))return {};
  FSHAHash Hash;FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash.Hash);return Hash.ToString();
 }
 struct FRun
 {
  TWeakObjectPtr<AS47Web> Web;TWeakObjectPtr<US47Main> Main;
  double Began=FPlatformTime::Seconds(),Since=0;int32 Stage=0,Capture=0;
  bool Done=false,OverrideInstalled=false;
  FString Root,OriginalHash,OfficeMesh,OfficeComponent,PendingFailure,FailureImage;
  S47J::FObj FailureMetadata;
  TArray<int32> OfficeLods;
  TArray<TSharedPtr<FJsonValue>> Checks,Frames;
  TFunction<bool(double,const FVector2D&,const FVector2D&,FS47View&)> PreviousCamera;
  TOptional<FLinearColor> OfficeDayEmission;double OfficeDayDriver=0;
  const FTarget& Target() const {return Targets[Capture%3];}
  FString Image()const{return Root/(FString(Capture<3?TEXT("08-"):TEXT("22-"))+Target().Name+TEXT(".png"));}
  void Finish(bool Good,const FString& Why)
  {
   if(Done)return;Done=true;auto* M=Main.Get();
   if(M&&M->Player&&OverrideInstalled)M->Player->CameraOverride=MoveTemp(PreviousCamera);
   auto O=S47J::New();S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));S47J::Set(O,TEXT("reason"),Why);S47J::Set(O,TEXT("wallSeconds"),FPlatformTime::Seconds()-Began);S47J::Set(O,TEXT("originalOfficeMaterialProofBefore"),OriginalHash);S47J::Set(O,TEXT("originalOfficeMaterialProofAfter"),OriginalMaterialHash());O->SetArrayField(TEXT("checks"),Checks);O->SetArrayField(TEXT("frames"),Frames);if(FailureMetadata){S47J::Set(O,TEXT("failureMetadata"),FailureMetadata);S47J::Set(O,TEXT("failureScreenshot"),FailureImage);}
   S47J::Set(O,TEXT("method"),TEXT("Isolated developer Freeform. CameraOverride fixtures, explicit SARO/diner area changes and one production TestAdvance from 08 to 22. Actual native EyeUE/forward, mesh/material/MID parameters, flood-family feeds and day masks sampled after camera/lighting settlement. Six native paired PNGs require root visual review. No WalkerPlace-based camera target, route, Story regression or GPU-pixel material-mask acceptance claimed."));
   S47J::Set(O,TEXT("scope"),TEXT("Exterior fixture allowlist only; mixed Office shader scalar checked independently from unchanged court0/6 whole-mesh feed. Original material package hash compared in editor; cooked builds verify resource identity only, with source byte preservation established by the preparation receipt. Four installed OfficeGlow LOD identities/counts retained; authored source immutability additionally belongs to material preparation receipt."));
   if(!Root.IsEmpty())FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("result.json")));
   UE_LOG(LogTemp,Display,TEXT("S47 exterior light %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),*Why);FPlatformMisc::RequestExitWithStatus(false,Good?0:1);
  }
  bool Check(bool Good,const FString& Why)
  {
   auto C=S47J::New();S47J::Set(C,TEXT("pass"),Good);S47J::Set(C,TEXT("check"),Why);Checks.Add(MakeShared<FJsonValueObject>(C));if(!Good)Finish(false,Why);return Good;
  }
  void Next(int32 S){Stage=S;Since=FPlatformTime::Seconds();}
  S47J::FObj ComponentMetadata(AS47Web* W,UStaticMeshComponent* C)
  {
   auto O=S47J::New();const int32 Index=W->NodeOf(C);const auto* Node=W->Node(Index);
   S47J::Set(O,TEXT("component"),C->GetPathName());S47J::Set(O,TEXT("mesh"),C->GetStaticMesh()?C->GetStaticMesh()->GetPathName():FString());S47J::Set(O,TEXT("node"),Index);S47J::Set(O,TEXT("fullNodeName"),Node?Node->Name:TEXT("unregistered"));S47J::Set(O,TEXT("parent"),Node?Node->Parent:-1);
   S47J::Set(O,TEXT("visible"),C->IsVisible());S47J::Set(O,TEXT("hiddenInGame"),C->bHiddenInGame);S47J::Set(O,TEXT("registered"),C->IsRegistered());S47J::Set(O,TEXT("ownerHidden"),!C->GetOwner()||C->GetOwner()->IsHidden());S47J::Set(O,TEXT("FS47ObjShown"),Index>=0&&W->ObjAt(Index).Shown());
   S47J::Set(O,TEXT("nativeAttachParent"),GetPathNameSafe(C->GetAttachParent()));
   auto Mask=S47J::New();S47J::Set(Mask,TEXT("currentArea"),W->Area());S47J::Set(Mask,TEXT("rootIndex"),Node?Node->Root:-1);const auto* RootNode=Node?W->Node(Node->Root):nullptr;
   S47J::Set(Mask,TEXT("rootName"),RootNode?RootNode->Name:FString());S47J::Set(Mask,TEXT("rootOwnVisibility"),RootNode&&RootNode->bVis);S47J::Set(Mask,TEXT("rootShown"),RootNode&&W->ObjAt(Node->Root).Shown());S47J::Set(O,TEXT("actualAreaMaskState"),Mask);
   TArray<TSharedPtr<FJsonValue>> Chain;
   for(int32 Parent=Index,Guard=0;Parent>=0&&Guard++<64;)
   {
    const auto* N=W->Node(Parent);if(!N)break;auto R=S47J::New();S47J::Set(R,TEXT("node"),Parent);S47J::Set(R,TEXT("name"),N->Name);S47J::Set(R,TEXT("parent"),N->Parent);S47J::Set(R,TEXT("ownVisible"),N->bVis);S47J::Set(R,TEXT("shown"),W->ObjAt(Parent).Shown());Chain.Add(MakeShared<FJsonValueObject>(R));Parent=N->Parent;
   }
   O->SetArrayField(TEXT("metadataParentChain"),Chain);const FVector P=S47Three::ToThree(C->Bounds.Origin),E=S47Three::ToThree(C->Bounds.BoxExtent);
   S47J::Set(O,TEXT("actualBoundsOrigin"),TArray<double>{P.X,P.Y,P.Z});S47J::Set(O,TEXT("actualBoundsExtent"),TArray<double>{E.X,E.Y,E.Z});
   TArray<FString> Materials;for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)Materials.Add(GetPathNameSafe(C->GetMaterial(Slot)));S47J::Set(O,TEXT("boundMaterials"),Materials);return O;
  }
  void VisibilityFailure(AS47Web* W,UStaticMeshComponent* Hidden,const S47J::FObj& Frame,const TArray<TSharedPtr<FJsonValue>>& Rows,const TArray<UStaticMeshComponent*>& Components)
  {
   PendingFailure=TEXT("photographed native light component is actually visible");FailureMetadata=ComponentMetadata(W,Hidden);
   TArray<TSharedPtr<FJsonValue>> Nearby;
   const FVector TargetUE=S47Three::ToUE(Target().At);
   for(auto* C:Components)if(C&&C->GetStaticMesh()&&C->IsRegistered()&&C->IsVisible()&&!C->bHiddenInGame&&C->GetOwner()&&!C->GetOwner()->IsHidden())
   {
    const FBox Box=C->Bounds.GetBox();const FVector Near=Box.GetClosestPointTo(TargetUE);
    if(FVector::DistSquared(Near,TargetUE)<=FMath::Square(2000.))Nearby.Add(MakeShared<FJsonValueObject>(ComponentMetadata(W,C)));
   }
   FailureMetadata->SetArrayField(TEXT("visibleNativeMeshesWithin20mOfTargetBounds"),Nearby);
   Frame->SetArrayField(TEXT("nativeFixturesBeforeFailure"),Rows);S47J::Set(Frame,TEXT("visibilityFailure"),FailureMetadata);Frames.Add(MakeShared<FJsonValueObject>(Frame));
   auto C=S47J::New();S47J::Set(C,TEXT("pass"),false);S47J::Set(C,TEXT("check"),PendingFailure);Checks.Add(MakeShared<FJsonValueObject>(C));
   FailureImage=Root/(FString(TEXT("FAILED-"))+(Capture<3?TEXT("08-"):TEXT("22-"))+Target().Name+TEXT(".png"));
   FFileHelper::SaveStringToFile(S47J::Write(Frame,true),*(Root/TEXT("visibility-failure.json")));
   FScreenshotRequest::RequestScreenshot(FailureImage,true,false);Next(99);
  }
  bool Audit(AS47Web* W)
  {
   auto* M=Main.Get();auto Frame=S47J::New();S47J::Set(Frame,TEXT("image"),Image());S47J::Set(Frame,TEXT("clock"),M->Freeform->Clock());S47J::Set(Frame,TEXT("area"),M->World->Area);
   const FVector Eye=S47Three::ToThree(M->Player->EyeUE()),Forward=S47Three::ToThree(M->Player->ForwardUE()*100.);
   S47J::Set(Frame,TEXT("actualCameraEye"),TArray<double>{Eye.X,Eye.Y,Eye.Z});S47J::Set(Frame,TEXT("actualCameraForward"),TArray<double>{Forward.X,Forward.Y,Forward.Z});
   if(!Check(Eye.Equals(Target().Eye,.025),TEXT("native camera reached explicit eye: ")+Image()))return false;
   const double Expected=Capture<3?0.:1.;TArray<TSharedPtr<FJsonValue>> Rows;
   TArray<UStaticMeshComponent*> Components;W->GetComponents(Components);
   for(const auto& A:Allowed)
   {
    const bool Town=FString(A.Name).StartsWith(TEXT("SM_Access_"));const int32 Set=W->FloodSet(Town?TEXT("town"):TEXT("court"));
    const int32 DaySlot=Town?1:A.Driver==8?8:A.Driver==6?6:2;
    const float Day=W->ExteriorDaylightMask(Set,DaySlot);
    if(!Check(FMath::IsNearlyEqual(double(Day),Expected,.001),FString(TEXT("actual daylight mask "))+A.Name))return false;
    int32 Found=0;
    for(auto* C:Components)if(C&&C->GetStaticMesh()&&C->GetStaticMesh()->GetName()==A.Name)
    {
     auto* Mid=Cast<UMaterialInstanceDynamic>(C->GetMaterial(A.Slot));auto* Base=Mid?Mid->GetMaterial():nullptr;
     if(!Check(Base&&Base->GetName()==A.Material,FString(TEXT("exact installed material binding "))+A.Name+FString::FromInt(A.Slot)))return false;
     ++Found;
     const bool TargetComponent=(Capture%3<2&&FString(A.Name)==TEXT("SM_Sierra_GlowOffice"))||(Capture%3==2&&FString(A.Name)==TEXT("SM_Access_Laundry_Fixtures"));
     if(TargetComponent&&!(C->IsRegistered()&&C->IsVisible()&&!C->bHiddenInGame&&C->GetOwner()&&!C->GetOwner()->IsHidden()))
     {VisibilityFailure(W,C,Frame,Rows,Components);return false;}
     FLinearColor Actual,Factor;
     if(!Check(Mid->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveFactor")),Actual)&&Base->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveFactor")),Factor),TEXT("native/base emissive parameter available")))return false;
     const float Feed=A.Driver>=0?FMath::Clamp(W->FloodPos(Set,A.Driver).A/A.Reference,0.f,1.f):A.Driver==-2?0.f:1.f;
     const bool Office=FString(A.Name)==TEXT("SM_Sierra_GlowOffice");const float Mult=Feed*(Office?1.f:Day)*(A.Driver==8?2.6f:1.f);
     if(!Check(Actual.Equals(Factor*Mult,.003f),FString(TEXT("existing family driver and local daylight retained "))+A.Name))return false;
     auto R=S47J::New();S47J::Set(R,TEXT("mesh"),C->GetStaticMesh()->GetPathName());S47J::Set(R,TEXT("component"),C->GetPathName());S47J::Set(R,TEXT("material"),Base->GetPathName());S47J::Set(R,TEXT("slot"),A.Slot);S47J::Set(R,TEXT("dayMask"),Day);S47J::Set(R,TEXT("familyFeed"),Feed);S47J::Set(R,TEXT("visible"),C->IsVisible()&&!C->bHiddenInGame);
     S47J::Set(R,TEXT("emissiveFactor"),TArray<double>{Actual.R,Actual.G,Actual.B,Actual.A});
     const FVector Origin=S47Three::ToThree(C->Bounds.Origin);S47J::Set(R,TEXT("actualBoundsOrigin"),TArray<double>{Origin.X,Origin.Y,Origin.Z});
     if(Office)
     {
      float Mask=-1;if(!Check(Mid->GetScalarParameterValue(FMaterialParameterInfo(TEXT("ExteriorDaylightMask")),Mask)&&FMath::IsNearlyEqual(double(Mask),Expected,.001),TEXT("mixed Office scalar isolates exterior while keeping indoor whole-mesh feed")))return false;
      S47J::Set(R,TEXT("mixedOfficeScalar"),Mask);TArray<int32> Lods;for(int32 L=0;L<C->GetStaticMesh()->GetNumLODs();++L)Lods.Add(C->GetStaticMesh()->GetNumTriangles(L));
      TArray<double> SavedLods;for(int32 N:Lods)SavedLods.Add(N);S47J::Set(R,TEXT("installedLodTriangles"),SavedLods);
      if(OfficeMesh.IsEmpty()){OfficeMesh=C->GetStaticMesh()->GetPathName();OfficeComponent=C->GetPathName();OfficeLods=Lods;OfficeDayEmission=Actual;OfficeDayDriver=Feed;}
      if(!Check(Lods.Num()==4&&Lods==OfficeLods&&OfficeMesh==C->GetStaticMesh()->GetPathName()&&OfficeComponent==C->GetPathName(),TEXT("all four OfficeGlow LODs and actual mesh/component identity retained")))return false;
      if(Capture>=3&&FMath::IsNearlyEqual(double(Feed),OfficeDayDriver,.001)&&!Check(OfficeDayEmission&&Actual.Equals(OfficeDayEmission.GetValue(),.003f),TEXT("stable court0/6 retains identical Office EmissiveFactor across day/night")))return false;
     }
     Rows.Add(MakeShared<FJsonValueObject>(R));
    }
    if(!Check(Found==1,FString(TEXT("one exact owned fixture component "))+A.Name+FString::FromInt(A.Slot)))return false;
   }
   Frame->SetArrayField(TEXT("nativeFixtures"),Rows);Frames.Add(MakeShared<FJsonValueObject>(Frame));return true;
  }
  void Update(AS47Web* W,const TSharedPtr<FRun>& Self)
  {
   if(Done)return;
   if(Stage==99)
   {
    TArray<uint8> PNG;const bool Ready=FFileHelper::LoadFileToArray(PNG,*FailureImage)&&PNG.Num()>1024&&PNG[0]==137&&PNG[1]==80&&PNG[2]==78&&PNG[3]==71;
    if(Ready||FPlatformTime::Seconds()-Since>6||FPlatformTime::Seconds()-Began>100){S47J::Set(FailureMetadata,TEXT("failurePngSaved"),Ready);Finish(false,PendingFailure);}return;
   }
   if(FPlatformTime::Seconds()-Began>100){Finish(false,TEXT("bounded100-second timeout"));return;}
   auto* M=US47Main::Get(W);if(!M||!M->Flow||!M->World||!M->Player||!M->Freeform||!W->IsLoaded()||M->Flow->bStarting||S47Startup::Blocking())return;Main=M;
   if(Stage==0){US47Flow::FStart Start;Start.bDev=true;Start.bFreeform=true;M->Flow->StartGame(Start);Next(1);return;}
   if(!M->Freeform->Active()||M->World->bBusy)return;
   if(Stage==1)
   {
    if(!Check(M->IsFreeform()&&M->Flow->CaseId==0&&M->Flow->bDevRun,TEXT("isolated actual Freeform developer session")))return;
    if(OriginalHash.IsEmpty()){OriginalHash=OriginalMaterialHash();if(!Check(!OriginalHash.IsEmpty(),TEXT("original OfficeGlow material proof available")))return;}
    if(Capture==3)
    {
     const double Delta=22*3600.-FMath::Fmod(M->Freeform->Clock(),86400.);
     if(!Check(Delta>0&&M->Freeform->TestAdvance(Delta),TEXT("production TestAdvance changes actual Freeform clock from08 to22")))return;
    }
    if(M->World->Area!=Target().Area)M->World->Enter(Target().Area);
    if(!OverrideInstalled){PreviousCamera=M->Player->CameraOverride;OverrideInstalled=true;}
    const TWeakPtr<FRun> Weak=Self;
    M->Player->CameraOverride=[Weak](double,const FVector2D&,const FVector2D&,FS47View& Out)
    {
     const auto S=Weak.Pin();if(!S||S->Done)return false;const auto& T=S->Target();const FVector D=T.At-T.Eye;
     Out.Eye=T.Eye;Out.Yaw=FMath::Atan2(-D.X,-D.Z);Out.Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());return true;
    };
    M->Player->SetActive(false);M->UI->Close(true);Next(2);return;
   }
   if(FPlatformTime::Seconds()-Since<1.5)return;
   if(Stage==2){if(!Audit(W))return;FScreenshotRequest::RequestScreenshot(Image(),true,false);Next(3);return;}
   TArray<uint8> PNG;if(!FFileHelper::LoadFileToArray(PNG,*Image())||PNG.Num()<1024){if(FPlatformTime::Seconds()-Since>6)Finish(false,TEXT("native screenshot unavailable"));return;}
   if(!Check(PNG[0]==137&&PNG[1]==80&&PNG[2]==78&&PNG[3]==71,TEXT("native paired PNG saved: ")+Image()))return;
   if(++Capture<6){Next(1);return;}
   if(!Check(OriginalMaterialHash()==OriginalHash,TEXT("original OfficeGlow material proof stable (editor bytes; cooked resource identity)")))return;
   Finish(true,TEXT("six paired native captures and exact component/MID/family-driver checks complete; pixel QA remains root review"));
  }
 };
}
void S47ExteriorLightTest::Update(AS47Web* Web)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47ExteriorLightTest")))return;
 static TSharedPtr<FRun> Run;
 if(!Run)
 {
  Run=MakeShared<FRun>();Run->Web=Web;FString User;FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
  if(!User.StartsWith(TEXT("/tmp/S47-exterior-light-"))){Run->Finish(false,TEXT("explicit isolated temporary profile required"));return;}
  Run->Root=FPaths::ProjectSavedDir()/TEXT("ExteriorLightTest");IFileManager::Get().MakeDirectory(*Run->Root,true);
 }
 Run->Update(Web,Run);
}

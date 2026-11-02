#include "S47SaroWallTest.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47Graphics.h"
#include "S47Story.h"
#include "S47Startup.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Inter.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "StaticMeshResources.h"
#include "Containers/Ticker.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"


namespace
{
 const TCHAR* Base=TEXT("/home/tom/scratch-bm/Signal47-saro-wall-investigation-20261101/");
 constexpr double Evening=23*3600+49*60;
 S47J::FObj Vec(const FVector& V)
 {auto O=S47J::New();S47J::Set(O,TEXT("x"),V.X);S47J::Set(O,TEXT("y"),V.Y);S47J::Set(O,TEXT("z"),V.Z);return O;}
 struct FRun
 {
  TWeakObjectPtr<US47Main> Main;
  FString Fixture,Output;
  int32 Stage=0;
  double Age=0,At=0;
  double X=-3.,Z=5.5,Yaw=-2.48,Pitch=-.12;
  double HeldFreeClock=0;
  bool bOpsOpened=false,bOpsRendered=false,bOpsClosedSameClock=false,bRestoredCorridor=false;
  FString GeometryIdentity,FirstCaptureIdentity;
  double GeometryChangedAt=0,GeometryPollAt=-1;
  int32 GeometryChanges=0;
  bool bComparisonGeometryStable=true;
  TArray<TSharedPtr<FJsonValue>> Frames;
  bool bRestAccepted=false;
  bool Finish(bool Good,const FString& Why)
  {
   auto O=S47J::New();S47J::Set(O,TEXT("diagnosticComplete"),Good);S47J::Set(O,TEXT("reason"),Why);
   S47J::Set(O,TEXT("fixture"),Fixture);S47J::Set(O,TEXT("restAccepted"),bRestAccepted);
   S47J::Set(O,TEXT("geometryStableAcrossCaptures"),bComparisonGeometryStable);
   S47J::Set(O,TEXT("opsOpenedThroughActualUseRay"),bOpsOpened);S47J::Set(O,TEXT("opsGlassRendered"),bOpsRendered);
   S47J::Set(O,TEXT("opsClosedAtUnchangedClock"),bOpsClosedSameClock);S47J::Set(O,TEXT("corridorPoseVerified"),bRestoredCorridor);
   S47J::Set(O,TEXT("scope"),TEXT("Nearby saved state; explicit standing corridor and OPS-approach placement fixtures, Low graphics, simulation clock held. Actual aimed OPS use and close first, then clock-only morning/evening and production Rest through real OPS. No natural navigation or exact user-error frame claimed. Active raster LOD is unavailable for multi-LOD auto selection; logged forced/resident values are not a render-thread per-view readback. Screenshots and records require comparison; completion is not a wall-fix PASS."));
   O->SetArrayField(TEXT("frames"),Frames);
   if(!Output.IsEmpty()) FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Output/TEXT("result.json")));
   UE_LOG(LogTemp,Display,TEXT("S47 wall diagnostic %s: %s"),Good?TEXT("COMPLETE"):TEXT("FAILED"),*Why);
   FPlatformMisc::RequestExitWithStatus(false,Good?0:1);return false;
  }
  FS47Obj Find(const FString& Name)const
  {
   auto* W=Main->Web.Get();if(W->Handle(Name)>=0)return W->ObjAt(W->Handle(Name));
   for(int32 I=0;I<W->PartCount();++I)if(const auto* N=W->Node(I);N&&N->Name==Name)return W->ObjAt(I);
   return {};
  }
  void PlaceCorridor()
  {
   auto* M=Main.Get();M->Player->Place(X,Z,Yaw);M->Player->Pitch=Pitch;
   M->Player->Eye=1.62;M->Player->bHeadMotion=false;M->Player->TestMove.Reset();M->Player->SetActive(false);
  }
  bool CorridorReady()const
  {
   const auto* M=Main.Get();const FVector Eye=S47Three::ToThree(M->Player->EyeUE());
   return M->World->Area==TEXT("saro")&&M->Web->Area()==TEXT("saro")&&!M->World->bDriving&&
    M->Player->Walkable(X,Z)&&FVector2D(M->Player->Pos.X-X,M->Player->Pos.Z-Z).Size()<.02&&
    FVector::Dist(Eye,FVector(X,M->Player->FloorY+1.62,Z))<.03&&!M->ModalOpen()&&!M->Freeform->TerminalOnGlass();
  }
  bool ApproachOps()
  {
   auto* M=Main.Get();FVector C,U,A;FVector2D Size;
   if(!M->Web->CrtGlass(3,C,U,A,Size))return false;
   FVector Normal=FVector::CrossProduct(A,U).GetSafeNormal();
   if(FVector::DotProduct(Normal,M->Player->EyeUE()-C)<0)Normal=-Normal;
   const FVector Approach=S47Three::ToThree(C+Normal*85.);
   M->Player->Place(Approach.X,Approach.Z,0);
   const auto* Item=M->Web->Item(TEXT("opsTerminal"));if(!Item)return false;
   const auto Object=M->Web->ObjAt(Item->Node);const auto B=Object.WorldBox();
   const FVector Target=B.IsValid?B.GetCenter():Object.WorldPosition();
   const FVector D=Target-FVector(Approach.X,M->Player->FloorY+M->Player->Eye,Approach.Z);
   M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
   M->Player->SetActive(false);return true;
  }
  bool UseOps()
  {
   auto* M=Main.Get();
   if(!M->Inter||!M->Player->OnUse||M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE())!=TEXT("opsTerminal"))return false;
   M->Player->OnUse(TEXT("opsTerminal"));M->bPausedByMenu=true;M->Player->SetActive(false);
   return M->Freeform->TerminalOnGlass()&&M->Freeform->ActiveTerminalScreen()==3;
  }
  bool GeometrySettled()
  {
   if(Age-GeometryPollAt>=.25)
   {
    GeometryPollAt=Age;FString Identity;auto* W=Main->Web.Get();
    const FBox Region(FVector(-7,-.5,4.4),FVector(3,3.6,11));
    for(int32 I=0;I<W->PartCount();++I)
    {
     const auto* N=W->Node(I);auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
     UStaticMesh* Mesh=C?C->GetStaticMesh():nullptr;if(!Mesh)continue;
     const FBox UB=Mesh->GetBoundingBox().TransformBy(C->GetComponentTransform());
     const FVector A=S47Three::ToThree(UB.Min),B=S47Three::ToThree(UB.Max);
     if(!FBox(A.ComponentMin(B),A.ComponentMax(B)).Intersect(Region))continue;
     Identity+=FString::Printf(TEXT("%d:%s:%s:%d:%d\n"),I,*C->GetPathName(),*Mesh->GetPathName(),Mesh->GetNumLODs(),Mesh->GetNumTriangles(0));
    }
    if(Identity!=GeometryIdentity)
    {
     GeometryIdentity=MoveTemp(Identity);GeometryChangedAt=Age;++GeometryChanges;
     UE_LOG(LogTemp,Display,TEXT("S47 wall diagnostic geometry changed at %.2fs (stage %d); awaiting two stable seconds"),Age,Stage);
    }
   }
   return !GeometryIdentity.IsEmpty() && Age-GeometryChangedAt>=2.;
  }
  bool Capture(const FString& Name)
  {
   auto* M=Main.Get();auto* W=M->Web.Get();auto O=S47J::New();
   S47J::Set(O,TEXT("phase"),Name);S47J::Set(O,TEXT("storyClock"),M->Story->Clock());S47J::Set(O,TEXT("freeformClock"),M->Freeform->Clock());
   S47J::Set(O,TEXT("player"),Vec(M->Player->Pos));S47J::Set(O,TEXT("eye"),Vec(S47Three::ToThree(M->Player->EyeUE())));
   S47J::Set(O,TEXT("actualEyeForwardUE"),Vec(M->Player->ForwardUE()));
   if(const auto* Camera=M->Player->FindComponentByClass<UCameraComponent>())
   {S47J::Set(O,TEXT("cameraRotationUE"),Vec(Camera->GetComponentRotation().Euler()));S47J::Set(O,TEXT("cameraFov"),Camera->FieldOfView);}
   S47J::Set(O,TEXT("worldArea"),M->World->Area);S47J::Set(O,TEXT("webArea"),W->Area());S47J::Set(O,TEXT("acousticSpace"),M->Space);
   S47J::Set(O,TEXT("playMode"),int32(M->Mode));S47J::Set(O,TEXT("driving"),M->World->bDriving);S47J::Set(O,TEXT("floorY"),M->Player->FloorY);
   S47J::Set(O,TEXT("modalOpen"),M->ModalOpen());S47J::Set(O,TEXT("pausedByMenu"),M->bPausedByMenu);S47J::Set(O,TEXT("cameraOverrideCallbackInstalled"),bool(M->Player->CameraOverride));
   S47J::Set(O,TEXT("terminalOnGlass"),M->Freeform->TerminalOnGlass());S47J::Set(O,TEXT("terminalRendered"),M->Freeform->TerminalWindowRendered());S47J::Set(O,TEXT("terminalScreen"),M->Freeform->ActiveTerminalScreen());
   S47J::Set(O,TEXT("corridorPose"),CorridorReady());
   TArray<TSharedPtr<FJsonValue>> Groups;
   for(const TCHAR* NameKey:{TEXT("annex.group"),TEXT("annex.interior"),TEXT("operations:room"),TEXT("office:authoredRoom"),TEXT("office:partitionRecordsWest"),TEXT("office:partitionRecords"),TEXT("office:partitionLeft"),TEXT("office:partitionRight"),TEXT("saro:rooms:corridorFinishes"),TEXT("saro:rooms:controlFinishes")})
   {
    const auto Obj=Find(NameKey);const auto* N=Obj?W->Node(Obj.I):nullptr;auto G=S47J::New();S47J::Set(G,TEXT("name"),FString(NameKey));S47J::Set(G,TEXT("exists"),bool(Obj));
    if(N){S47J::Set(G,TEXT("node"),Obj.I);S47J::Set(G,TEXT("parent"),N->Parent);S47J::Set(G,TEXT("localVisible"),N->bVis);S47J::Set(G,TEXT("shown"),Obj.Shown());if(const auto* C=N->Comp.Get()){S47J::Set(G,TEXT("componentVisible"),C->IsVisible());S47J::Set(G,TEXT("hiddenInGame"),bool(C->bHiddenInGame));}}
    Groups.Add(MakeShared<FJsonValueObject>(G));
   }
   O->SetArrayField(TEXT("visibilityGroups"),Groups);
   S47J::Set(O,TEXT("yaw"),M->Player->Yaw);S47J::Set(O,TEXT("pitch"),M->Player->Pitch);
   S47J::Set(O,TEXT("annexInteriorShown"),W->Obj(TEXT("annex.interior")).Shown());
   S47J::Set(O,TEXT("graphics"),W->GetGraphicsSettings().PresetId());
   if(FirstCaptureIdentity.IsEmpty())FirstCaptureIdentity=GeometryIdentity;
   const bool SameGeometry=GeometryIdentity==FirstCaptureIdentity;
   bComparisonGeometryStable&=SameGeometry;
   S47J::Set(O,TEXT("sameGeometryAsFirstCapture"),SameGeometry);
   S47J::Set(O,TEXT("geometryStableSeconds"),Age-GeometryChangedAt);
   S47J::Set(O,TEXT("geometryChangesObserved"),GeometryChanges);
   const FBox Region(FVector(-7,-.5,4.4),FVector(3,3.6,11));
   TArray<TSharedPtr<FJsonValue>> Nodes;
   for(int32 I=0;I<W->PartCount();++I)
   {
    const auto* N=W->Node(I);auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
    UStaticMesh* Mesh=C?C->GetStaticMesh():nullptr;if(!Mesh)continue;
    const FBox UB=Mesh->GetBoundingBox().TransformBy(C->GetComponentTransform());
    const FVector A=S47Three::ToThree(UB.Min),B=S47Three::ToThree(UB.Max);
    const FBox Bounds(A.ComponentMin(B),A.ComponentMax(B));if(!Bounds.Intersect(Region))continue;
    auto R=S47J::New();S47J::Set(R,TEXT("node"),I);S47J::Set(R,TEXT("parent"),N->Parent);S47J::Set(R,TEXT("name"),N->Name);
    S47J::Set(R,TEXT("component"),C->GetName());S47J::Set(R,TEXT("mesh"),Mesh->GetPathName());
    S47J::Set(R,TEXT("localVisible"),N->bVis);S47J::Set(R,TEXT("shown"),W->ObjAt(I).Shown());
    S47J::Set(R,TEXT("componentVisible"),C->IsVisible());S47J::Set(R,TEXT("hiddenInGame"),C->bHiddenInGame);
    S47J::Set(R,TEXT("min"),Vec(Bounds.Min));S47J::Set(R,TEXT("max"),Vec(Bounds.Max));
    S47J::Set(R,TEXT("lods"),Mesh->GetNumLODs());S47J::Set(R,TEXT("lod0Triangles"),Mesh->GetNumTriangles(0));
    S47J::Set(R,TEXT("forcedLodModel"),C->GetForcedLodModel());S47J::Set(R,TEXT("forcedLodConvention"),TEXT("0 auto; positive value selects raster LOD value-1"));
    S47J::Set(R,TEXT("activeRasterLod"),Mesh->GetNumLODs()==1&&!Mesh->HasValidNaniteData()?0:-1);
    S47J::Set(R,TEXT("activeRasterLodMeaning"),Mesh->GetNumLODs()==1&&!Mesh->HasValidNaniteData()?TEXT("only resident raster LOD"):TEXT("unknown: per-view render-thread selection is not read from the game thread"));
    S47J::Set(R,TEXT("hasNaniteData"),Mesh->HasValidNaniteData());S47J::Set(R,TEXT("minimumLodOverride"),C->GetOverrideMinLOD());S47J::Set(R,TEXT("minimumLod"),C->GetMinLOD());
    S47J::Set(R,TEXT("boundsSphereRadiusCm"),C->Bounds.SphereRadius);S47J::Set(R,TEXT("eyeDistanceToBoundsCm"),FVector::Dist(M->Player->EyeUE(),C->Bounds.Origin));
    S47J::Set(R,TEXT("nativeBoundsOriginUE"),Vec(C->Bounds.Origin));S47J::Set(R,TEXT("nativeBoundsExtentCm"),Vec(C->Bounds.BoxExtent));
    S47J::Set(R,TEXT("minDrawDistance"),C->MinDrawDistance);S47J::Set(R,TEXT("maxDrawDistance"),C->CachedMaxDrawDistance);S47J::Set(R,TEXT("recentlyRendered"),C->WasRecentlyRendered(.5f));
    if(const auto* Render=Mesh->GetRenderData())
    {
     S47J::Set(R,TEXT("firstResidentLod"),Render->GetCurrentFirstLODIdx(0));TArray<TSharedPtr<FJsonValue>> Lods;
     for(int32 L=0;L<Render->LODResources.Num();++L){auto V=S47J::New();S47J::Set(V,TEXT("index"),L);S47J::Set(V,TEXT("triangles"),Mesh->GetNumTriangles(L));S47J::Set(V,TEXT("screenSize"),Render->ScreenSize[L].GetValue());Lods.Add(MakeShared<FJsonValueObject>(V));}R->SetArrayField(TEXT("rasterLods"),Lods);
    }
    TArray<TSharedPtr<FJsonValue>> Mats;
    for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)if(auto* Mat=C->GetMaterial(Slot))
    {
     auto T=S47J::New();S47J::Set(T,TEXT("slot"),Slot);S47J::Set(T,TEXT("path"),Mat->GetPathName());
     S47J::Set(T,TEXT("blendMode"),int32(Mat->GetBlendMode()));S47J::Set(T,TEXT("twoSided"),Mat->IsTwoSided());
     for(const TCHAR* Key:{TEXT("Opacity"),TEXT("NativeLighting"),TEXT("Cutout"),TEXT("OpacityMask")})
     {float V=0;if(Mat->GetScalarParameterValue(FMaterialParameterInfo(Key),V))S47J::Set(T,Key,double(V));}
     Mats.Add(MakeShared<FJsonValueObject>(T));
    }
    R->SetArrayField(TEXT("materials"),Mats);Nodes.Add(MakeShared<FJsonValueObject>(R));
   }
   O->SetArrayField(TEXT("nearbyMeshes"),Nodes);
   TArray<TSharedPtr<FJsonValue>> Rays;
   for(const FVector& Point:{FVector(-2.15,.64,6.591),FVector(-2.15,1.8,6.577),FVector(-.51,.64,7.111)})
   {
    auto R=S47J::New();const FVector From=M->Player->EyeUE(),Target=S47Three::ToUE(Point);
    FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(S47WallProbe),true);Params.AddIgnoredActor(M->Player.Get());
    const bool Contact=M->Web->GetWorld()->LineTraceSingleByChannel(Hit,From,Target+(Target-From).GetSafeNormal()*100.,ECC_Visibility,Params);
    S47J::Set(R,TEXT("targetThree"),Vec(Point));S47J::Set(R,TEXT("contact"),Contact);S47J::Set(R,TEXT("meaning"),TEXT("actual collision component identification; collision may remain enabled on an invisible mesh"));
    if(Contact){S47J::Set(R,TEXT("impactThree"),Vec(S47Three::ToThree(Hit.ImpactPoint)));if(const auto* C=Hit.GetComponent())S47J::Set(R,TEXT("component"),C->GetName());}
    Rays.Add(MakeShared<FJsonValueObject>(R));
   }
   O->SetArrayField(TEXT("wallIdentificationRays"),Rays);Frames.Add(MakeShared<FJsonValueObject>(O));
   const bool Written=FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Output/(Name+TEXT(".json"))));
   FScreenshotRequest::RequestScreenshot(Output/(Name+TEXT(".png")),true,false);
   UE_LOG(LogTemp,Display,TEXT("S47 wall diagnostic %s: %d mesh records, annex shown=%d, StoryClock=%.1f"),*Name,Nodes.Num(),W->Obj(TEXT("annex.interior")).Shown(),M->Story->Clock());
   return Written;
  }
  void Next(int32 S){Stage=S;At=Age;}
  bool Step(float Dt)
  {
   Age+=Dt;if(Age>115)return Finish(false,TEXT("bounded 115-second diagnostic timeout"));
   auto* M=Main.Get();if(!M||!M->Flow||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->Story||!M->Freeform||!M->UI||!M->World||S47Startup::Blocking()||M->Flow->bStarting)return true;
   if(Stage==0)
   {
    FString Text;if(!FFileHelper::LoadFileToString(Text,*Fixture))return Finish(false,TEXT("fixture unavailable"));
    const auto Envelope=S47J::Read(Text);auto State=S47J::Obj(Envelope,TEXT("state"));
    if(!State||S47J::Str(State,TEXT("sessionMode"))!=TEXT("freeform")||S47J::Str(State,TEXT("area"))!=TEXT("saro"))return Finish(false,TEXT("requires nearby SARO Freeform fixture"));
    auto Free=S47J::Obj(State,TEXT("freeform"));if(!Free)return Finish(false,TEXT("fixture missing Freeform state"));
    S47J::Set(Free,TEXT("clock"),Evening);
    M->Flow->Graphics=FS47GraphicsSettings::ForPreset(TEXT("low"));M->Web->ApplyGraphicsSettings(M->Flow->Graphics);
    US47Flow::FStart Start;Start.bDev=true;Start.State=State;M->Flow->StartGame(Start);Next(1);return true;
   }
   if(!M->IsFreeform()||!M->Freeform->Active()||M->Flow->CaseId!=0)return Finish(false,TEXT("isolated developer Freeform case0 required"));
   if(Stage==1)
   {
    if(Age-At<2)return true;
    if(M->World->bDriving)return Finish(false,TEXT("fixture must expose an on-foot player; probe will not forcibly exit a vehicle"));
    M->UI->Close(true);M->bPausedByMenu=true;PlaceCorridor();M->Story->SetClock(Evening);
    HeldFreeClock=M->Freeform->Clock();Next(2);return true;
   }
   // Only the simulation clock is held. Native visibility, mesh preparation,
   // player camera and the real physical terminal virtual window keep ticking.
   M->bPausedByMenu=true;M->Player->SetActive(false);
   const bool Settled=GeometrySettled();if(Age-At<1.5)return true;
   if((Stage==2||Stage==4||Stage==5||Stage==7||Stage==9||Stage==12||Stage==13)&&!Settled)return true;
   if(Stage==2)
   {
    if(Age-At<10)return true;
    if(!CorridorReady())return Finish(false,TEXT("camera is not at the required standing SARO corridor fixture; outside/override captures are rejected"));
    bRestoredCorridor=true;if(!Capture(TEXT("01-evening-corridor")))return Finish(false,TEXT("snapshot write failed"));
    Next(30);return true;
   }
   if(Stage==30)
   {if(!ApproachOps())return Finish(false,TEXT("actual OPS geometry unavailable"));Next(3);return true;}
   if(Stage==3)
   {
    bOpsOpened=UseOps();if(!bOpsOpened)return Finish(false,TEXT("actual aimed OPS ray did not open the production physical terminal"));
    Next(4);return true;
   }
   if(Stage==4)
   {
    bOpsRendered=M->Freeform->TerminalWindowRendered();
    if(!bOpsRendered)return true;
    if(!FMath::IsNearlyEqual(M->Story->Clock(),Evening,.01)||!FMath::IsNearlyEqual(M->Freeform->Clock(),HeldFreeClock,.01))return Finish(false,TEXT("clock advanced during isolated OPS cycle"));
    if(!Capture(TEXT("02-real-ops-open-same-clock")))return Finish(false,TEXT("snapshot write failed"));
    Next(40);return true;
   }
   if(Stage==40)
   {M->UI->Close(true);M->bPausedByMenu=true;PlaceCorridor();Next(5);return true;}
   if(Stage==5)
   {
    bOpsClosedSameClock=FMath::IsNearlyEqual(M->Story->Clock(),Evening,.01)&&FMath::IsNearlyEqual(M->Freeform->Clock(),HeldFreeClock,.01)&&CorridorReady();
    if(!bOpsClosedSameClock)return Finish(false,TEXT("OPS close did not restore the same held-clock corridor pose without an orphan modal/camera"));
    if(!Capture(TEXT("03-real-ops-closed-same-clock")))return Finish(false,TEXT("snapshot write failed"));Next(6);return true;
   }
   if(Stage==6){M->Story->SetClock(8*3600);Next(7);return true;}
   if(Stage==7){if(!Capture(TEXT("04-clock-only-morning")))return Finish(false,TEXT("snapshot write failed"));Next(8);return true;}
   if(Stage==8){M->Story->SetClock(Evening);Next(9);return true;}
   if(Stage==9)
   {
    if(!Capture(TEXT("05-clock-only-evening")))return Finish(false,TEXT("snapshot write failed"));
    Next(100);return true;
   }
   if(Stage==100)
   {if(!ApproachOps())return Finish(false,TEXT("actual OPS approach unavailable before Rest"));Next(10);return true;}
   if(Stage==10)
   {if(!UseOps())return Finish(false,TEXT("actual OPS use failed before production Rest"));Next(11);return true;}
   if(Stage==11)
   {
    if(!M->Freeform->TerminalWindowRendered())return true;
    bRestAccepted=M->Freeform->Rest();M->bPausedByMenu=true;
    if(!bRestAccepted){M->UI->Close(true);return Finish(false,TEXT("production Rest refused fixture; active work/event is not falsified"));}
    PlaceCorridor();Next(12);return true;
   }
   if(Stage==12)
   {
    if(!CorridorReady()||!FMath::IsNearlyEqual(M->Story->Clock(),8*3600.,.01))return Finish(false,TEXT("production Rest did not leave the same corridor fixture at morning"));
    if(!Capture(TEXT("06-production-rest-corridor")))return Finish(false,TEXT("snapshot write failed"));Next(13);return true;
   }
   for(const TCHAR* Name:{TEXT("01-evening-corridor"),TEXT("02-real-ops-open-same-clock"),TEXT("03-real-ops-closed-same-clock"),TEXT("04-clock-only-morning"),TEXT("05-clock-only-evening"),TEXT("06-production-rest-corridor")})
   {
    TArray<uint8> PNG;if(!FFileHelper::LoadFileToArray(PNG,*(Output/(FString(Name)+TEXT(".png"))))||PNG.Num()<1024||PNG[0]!=137||PNG[1]!=80||PNG[2]!=78||PNG[3]!=71)return Finish(false,TEXT("native screenshot missing or invalid"));
   }
   return Finish(true,bComparisonGeometryStable?
    TEXT("six native captures: same-clock physical OPS cycle, fixed-pose clock controls and real Rest; stable mesh identities, inspect images before diagnosing"):
    TEXT("six captures complete but geometry changed: comparison remains confounded and is not a wall-fix PASS"));
  }
 };
}

void S47SaroWallTest::Start(US47Main* Main)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47SaroWallTest")))return;
 auto Run=MakeShared<FRun>();Run->Main=Main;FString User;
 FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
 FParse::Value(FCommandLine::Get(),TEXT("S47WallFixture="),Run->Fixture);
 FParse::Value(FCommandLine::Get(),TEXT("S47WallOutput="),Run->Output);
 if(!User.StartsWith(TEXT("/tmp/S47-saro-wall-"))||!Run->Fixture.StartsWith(FString(Base)+TEXT("fixtures/"))||!Run->Output.StartsWith(FString(Base)+TEXT("runs/")))
 {Run->Output.Empty();Run->Finish(false,TEXT("requires isolated UserDir and explicit private fixture/output paths"));return;}
 FParse::Value(FCommandLine::Get(),TEXT("S47WallX="),Run->X);FParse::Value(FCommandLine::Get(),TEXT("S47WallZ="),Run->Z);
 FParse::Value(FCommandLine::Get(),TEXT("S47WallYaw="),Run->Yaw);FParse::Value(FCommandLine::Get(),TEXT("S47WallPitch="),Run->Pitch);
 if(!FMath::IsFinite(Run->X)||!FMath::IsFinite(Run->Z)||!FMath::IsFinite(Run->Yaw)||!FMath::IsFinite(Run->Pitch)||Run->X<-3.3||Run->X>-2.7||Run->Z<5.2||Run->Z>5.8||Run->Yaw<-2.7||Run->Yaw>-2.1||Run->Pitch<-.35||Run->Pitch>.05)
 {Run->Finish(false,TEXT("requires corridor wall fixture near (-3,5.5), aimed at Records_front_1; old outside-field coordinates are rejected"));return;}
 IFileManager::Get().MakeDirectory(*Run->Output,true);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float Dt){return Run->Step(Dt);}));
}

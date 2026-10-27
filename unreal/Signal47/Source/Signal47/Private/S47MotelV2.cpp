// Unreal-only replacement of the motel presentation. Imported assets never own story interactions.
#include "S47MotelV2.h"
#include "S47Web.h"
#include "S47Three.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StaticMeshResources.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include <limits>

DEFINE_LOG_CATEGORY_STATIC(LogS47MotelV2,Log,All);
TArray<FS47MotelV2::FFloor> FS47MotelV2::Floors;
namespace
{
 using FValues=TArray<TSharedPtr<FJsonValue>>;
 FVector Vec(const FValues& A){return FVector(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber());}
 bool VectorValid(const FValues& A,int32 N){if(A.Num()!=N)return false;for(const auto& V:A)if(V->Type!=EJson::Number||!FMath::IsFinite(V->AsNumber()))return false;return true;}
 int32 Tris(UStaticMesh* M){return M&&M->GetRenderData()&&M->GetNumLODs()>0?M->GetNumTriangles(0):-1;}
 UStaticMeshComponent* MeshAt(AS47Web* W,int32 I){const auto* N=W->Node(I);return N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;}
 void Retire(UStaticMeshComponent* C){C->SetVisibility(false,false);C->SetHiddenInGame(true,false);C->SetCastShadow(false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
 int32 FindChild(AS47Web* W,int32 Root,const FString& Name)
 {
  TArray<int32> Pending;Pending.Add(Root);
  while(!Pending.IsEmpty()){const int32 I=Pending.Pop(EAllowShrinking::No);const auto* N=W->Node(I);if(!N)continue;if(N->Name==Name)return I;Pending.Append(N->Children);}
  return INDEX_NONE;
 }
 FS47Rect ParseMotelRect(const FValues& A){FS47Rect R;R.MinX=A[0]->AsNumber();R.MaxX=A[1]->AsNumber();R.MinZ=A[2]->AsNumber();R.MaxZ=A[3]->AsNumber();return R;}
 bool Same(const FS47Rect& A,const FS47Rect& B){return FMath::Abs(A.MinX-B.MinX)<.06&&FMath::Abs(A.MaxX-B.MaxX)<.06&&FMath::Abs(A.MinZ-B.MinZ)<.06&&FMath::Abs(A.MaxZ-B.MaxZ)<.06;}
 bool ValidRect(const FValues& A){return VectorValid(A,4)&&A[0]->AsNumber()<A[1]->AsNumber()&&A[2]->AsNumber()<A[3]->AsNumber();}
 FValues RectValues(const FS47Rect& R){return {MakeShared<FJsonValueNumber>(R.MinX),MakeShared<FJsonValueNumber>(R.MaxX),MakeShared<FJsonValueNumber>(R.MinZ),MakeShared<FJsonValueNumber>(R.MaxZ)};}
}

TSharedPtr<FS47MotelV2> FS47MotelV2::Build(AS47Web* W)
{
 Floors.Reset();
 if(!W||FParse::Param(FCommandLine::Get(),TEXT("S47MotelV2Off"))||FParse::Param(FCommandLine::Get(),TEXT("S47TownBuildingsOff"))||FParse::Param(FCommandLine::Get(),TEXT("S47PropsOff")))return {};
 FString Text;TSharedPtr<FJsonObject> J;
 if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("MotelV2/assembly.json")))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),J)||!J.IsValid())
 {UE_LOG(LogS47MotelV2,Warning,TEXT("S47: motel V2 metadata unavailable; existing motel retained"));return {};}
 auto Reject=[](const FString& Reason){UE_LOG(LogS47MotelV2,Error,TEXT("S47: motel V2 rejected: %s; existing motel retained"),*Reason);};
 const TSharedPtr<FJsonObject>* PartsJson=nullptr;
 if(!J->TryGetObjectField(TEXT("parts"),PartsJson)||(*PartsJson)->Values.IsEmpty()){Reject(TEXT("missing parts"));return {};}
 const int32 Motel=W->Handle(TEXT("world.motel.group"));if(!W->Node(Motel)||W->Node(Motel)->Parent>=0||!W->Node(Motel)->Q.Equals(FQuat::Identity,.00001)||!W->Node(Motel)->S.Equals(FVector::OneVector,.00001)){Reject(TEXT("motel parent must remain an unrotated translation root"));return {};}
 struct FPart {FString Name;UStaticMesh* Mesh;int32 Parent;FVector At;};
 struct FPatch {UStaticMeshComponent* Comp;UStaticMesh* Mesh;TArray<int32> Slots;bool Hidden;};
 struct FCar {int32 Node;FVector Target;FString Id;};
 TArray<FPart> NewParts;TArray<FPatch> Patches;TArray<UStaticMeshComponent*> Retired;TArray<FCar> Cars;
 TArray<FS47Rect> Disable,Add,Zones;TArray<TPair<FS47Rect,FS47Rect>> CarRects;TArray<FFloor> NewFloors;
 TMap<int32,FVector> Protected;
 for(const TCHAR* H:{TEXT("world.motel.tv.group"),TEXT("world.motel.objs.envelope"),TEXT("world.motel.objs.message"),TEXT("world.motel.objs.key6Hook"),TEXT("world.motel.objs.officePhoneHandset"),TEXT("world.motel.objs.room6DoorLeaf"),TEXT("world.motel.proxies.register"),TEXT("world.motel.proxies.room6Door")})
 {
  const int32 I=W->Handle(H);if(!W->Node(I)||!W->Node(I)->Comp.IsValid()){Reject(FString(TEXT("protected handle "))+H);return {};}
  Protected.Add(I,W->WorldThree(I));
 }
 // The room6 window owns a live lit/dark material. Its old native replacement had hidden this leaf.
 auto* RoomWindow=MeshAt(W,686);const auto* WindowNode=W->Node(686);
 if(!RoomWindow||!WindowNode||WindowNode->Geo!=471||WindowNode->Parent!=663||RoomWindow->GetNumMaterials()!=1){Reject(TEXT("room6 live window predecessor"));return {};}
 // Four existing native exterior parts must exist. Room 6 is deliberately a separate untouched set.
 for(const TCHAR* Name:{TEXT("SM_Town_Motel_Shell"),TEXT("SM_Town_Motel_Glass"),TEXT("SM_Town_Motel_Office"),TEXT("SM_Town_Motel_Fixtures")})
 {
  auto* C=MeshAt(W,FindChild(W,Motel,FString(TEXT("town-production:"))+Name));
  if(!C||!C->GetStaticMesh()||C->GetStaticMesh()->GetName()!=Name){Reject(FString(TEXT("native predecessor "))+Name);return {};}
  Retired.Add(C);
 }
 for(const auto& Pair:(*PartsJson)->Values)
 {
  const auto R=Pair.Value->AsObject();if(!R){Reject(TEXT("part object"));return {};}
  FString Asset,Parent;const FValues *At=nullptr,*Lods=nullptr,*Bounds=nullptr;
  if(!R->TryGetStringField(TEXT("asset"),Asset)||!Asset.StartsWith(TEXT("/Game/MotelV2/"))||!R->TryGetStringField(TEXT("parent_handle"),Parent)||(Parent!=TEXT("world.motel.group")&&!(FString(Pair.Key.ToView()).StartsWith(TEXT("SM_Sierra_Room6Lining"))&&Parent==TEXT("world.room6.group")))||!R->TryGetArrayField(TEXT("site_world_three_m"),At)||!VectorValid(*At,3)||!R->TryGetArrayField(TEXT("lod_triangles"),Lods)||Lods->IsEmpty()||!R->TryGetArrayField(TEXT("bounds_local_m"),Bounds)||Bounds->Num()!=2||!VectorValid((*Bounds)[0]->AsArray(),3)||!VectorValid((*Bounds)[1]->AsArray(),3))
  {Reject(FString(Pair.Key.ToView())+TEXT(" metadata"));return {};}
  const FVector Expected=Parent==TEXT("world.room6.group")?FVector(0,0,-8000):FVector(-45.9,-2.0985683853163195,674.9);
  if(!Vec(*At).Equals(Expected,.001)){Reject(Asset+TEXT(" site anchor"));return {};}
  auto* M=LoadObject<UStaticMesh>(nullptr,*Asset);const int32 P=W->Handle(Parent);
  if(!M||!W->Node(P)||!M->GetRenderData()||M->GetNumLODs()!=Lods->Num()){Reject(Asset+TEXT(" asset/LOD count"));return {};}
  for(int32 L=0;L<Lods->Num();++L)if(M->GetNumTriangles(L)!=int32((*Lods)[L]->AsNumber())||M->GetNumTriangles(L)<=0){Reject(Asset+TEXT(" triangles"));return {};}
  const FVector Lo=Vec((*Bounds)[0]->AsArray()),Hi=Vec((*Bounds)[1]->AsArray());
  const FVector Origin((Lo.X+Hi.X)*50,-(Lo.Y+Hi.Y)*50,(Lo.Z+Hi.Z)*50);
  if(!M->GetBounds().Origin.Equals(Origin,.5)||!M->GetBounds().BoxExtent.Equals((Hi-Lo)*50,.5)){Reject(Asset+TEXT(" metric pivot"));return {};}
  for(const auto& Slot:M->GetStaticMaterials())if(!Slot.MaterialInterface){Reject(Asset+TEXT(" missing material"));return {};}
  NewParts.Add({FString(Pair.Key.ToView()),M,P,Vec(*At)});
 }
 const FValues* Rows=nullptr;
 if(J->TryGetArrayField(TEXT("patches"),Rows))for(const auto& V:*Rows)
 {
  const auto R=V->AsObject();const FValues *Counts=nullptr,*Slots=nullptr;double Node,Geo,Parent,Result;bool Hidden=false;FString Asset;
  if(!R||!R->TryGetNumberField(TEXT("node"),Node)||!R->TryGetNumberField(TEXT("geo"),Geo)||!R->TryGetNumberField(TEXT("parent"),Parent)||!R->TryGetNumberField(TEXT("result_tris"),Result)||!R->TryGetArrayField(TEXT("accepted_previous_tris"),Counts)||!R->TryGetBoolField(TEXT("hidden"),Hidden)) {Reject(TEXT("patch metadata"));return {};}
  auto* C=MeshAt(W,int32(Node));const auto* N=W->Node(int32(Node));bool Accepted=false;
  if(C)for(const auto& Count:*Counts)Accepted|=Tris(C->GetStaticMesh())==int32(Count->AsNumber());
  if(!C||!N||N->Geo!=int32(Geo)||N->Parent!=int32(Parent)||!Accepted){Reject(FString::Printf(TEXT("patch predecessor node%d g%d"),int32(Node),int32(Geo)));return {};}
  FPatch Patch{C,nullptr,{},Hidden};
  if(!Hidden)
  {
   if(!R->TryGetStringField(TEXT("asset"),Asset)||!Asset.StartsWith(TEXT("/Game/MotelV2/"))||!R->TryGetArrayField(TEXT("original_material_indices"),Slots)){Reject(TEXT("patch asset metadata"));return {};}
   Patch.Mesh=LoadObject<UStaticMesh>(nullptr,*Asset);
   if(!Patch.Mesh||Tris(Patch.Mesh)!=int32(Result)||Slots->Num()!=Patch.Mesh->GetStaticMaterials().Num()){Reject(Asset+TEXT(" patch count"));return {};}
   for(const auto& Slot:*Slots){const int32 S=int32(Slot->AsNumber());if(S<0||S>=C->GetNumMaterials()){Reject(Asset+TEXT(" patch material"));return {};}Patch.Slots.Add(S);}
  }
  Patches.Add(Patch);
 }
 if(J->TryGetArrayField(TEXT("retire_nodes"),Rows))for(const auto& V:*Rows)
 {
  const auto R=V->AsObject();double I,G,P;if(!R||!R->TryGetNumberField(TEXT("node"),I)||!R->TryGetNumberField(TEXT("geo"),G)||!R->TryGetNumberField(TEXT("parent"),P)){Reject(TEXT("retire metadata"));return {};}
  auto* C=MeshAt(W,int32(I));const auto* N=W->Node(int32(I));
  if(!C||N->Geo!=int32(G)||N->Parent!=int32(P)||Protected.Contains(int32(I))){Reject(TEXT("retire predecessor"));return {};}Retired.AddUnique(C);
 }
 for(const TCHAR* Prop:{TEXT("motelBlade"),TEXT("motelNeon")})
 {
  if(auto* C=MeshAt(W,FindChild(W,Motel,FString(TEXT("prop:"))+Prop)))Retired.AddUnique(C);
  for(const auto& R:W->CollidersMutable(TEXT("saro")))if(R.Id==FString(TEXT("prop:"))+Prop)Disable.Add(R);
 }
 auto ReadRects=[&](const TCHAR* Field,TArray<FS47Rect>& Out){const FValues* A=nullptr;if(J->TryGetArrayField(Field,A))for(const auto& V:*A){const auto& Q=V->AsArray();if(!ValidRect(Q))return false;Out.Add(ParseMotelRect(Q));}return true;};
 if(!ReadRects(TEXT("disable_colliders"),Disable)||!ReadRects(TEXT("add_colliders"),Add)||!ReadRects(TEXT("add_zones"),Zones)){Reject(TEXT("collision rectangles"));return {};}
 if(J->TryGetArrayField(TEXT("floors"),Rows))for(const auto& V:*Rows)
 {
  const auto R=V->AsObject();const FValues* A=nullptr;double Y;
  if(!R||!R->TryGetArrayField(TEXT("rect"),A)||!ValidRect(*A)||!R->TryGetNumberField(TEXT("y"),Y)||!FMath::IsFinite(Y)){Reject(TEXT("floor metadata"));return {};}
  const auto Q=ParseMotelRect(*A);NewFloors.Add({Q.MinX,Q.MaxX,Q.MinZ,Q.MaxZ,Y});
 }
 for(const auto& R:Disable)
 {
  bool Found=false;for(const auto& C:W->CollidersMutable(TEXT("saro")))Found|=Same(C,R);
  if(!Found){Reject(FString::Printf(TEXT("missing collider predecessor %.2f %.2f %.2f %.2f"),R.MinX,R.MaxX,R.MinZ,R.MaxZ));return {};}
 }
 // The real highway starts east of the court. A presentation-scene pool must never overlap it.
 for(const auto& R:Add)if(R.MaxX>-30.1){Reject(TEXT("new collider crosses highway shoulder"));return {};}
 const TSharedPtr<FJsonObject>* CarJson=nullptr;
 if(!J->TryGetObjectField(TEXT("car_positions"),CarJson)){Reject(TEXT("three original car placements missing"));return {};}
 for(const TCHAR* Id:{TEXT("motelCar1"),TEXT("motelCar2"),TEXT("motelCar3")})
 {
  const int32 I=FindChild(W,Motel,FString(TEXT("prop:"))+Id);const FValues* A=nullptr;
  if(!MeshAt(W,I)||W->Node(I)->Parent!=Motel||!(*CarJson)->TryGetArrayField(Id,A)||!VectorValid(*A,3)){Reject(FString(TEXT("original car "))+Id);return {};}
  FVector Target=Vec(*A);Target.Y=W->WorldThree(I).Y;Cars.Add({I,Target,Id});
 }
 // All validation ends before the first mutation. A missing cooked asset leaves the full predecessor visible.
 auto F=MakeShared<FS47MotelV2>();F->Web=W;F->ProtectedPositions=Protected;F->ExpectedPartCount=NewParts.Num()+1;
 for(const auto& Pair:Protected)F->ProtectedComponents.Add(Pair.Key,W->Node(Pair.Key)->Comp);
 for(const FPart& P:NewParts)
 {
  auto* C=NewObject<UStaticMeshComponent>(W);C->SetupAttachment(W->Node(P.Parent)->Comp.Get());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(P.Mesh);
  const bool Glass=P.Name.Contains(TEXT("Glass"))||P.Name.Contains(TEXT("Water"));C->SetCollisionEnabled(Glass?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryOnly);
  C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,Glass?ECR_Ignore:ECR_Block);C->SetCastShadow(!Glass);C->RegisterComponent();
  C->SetWorldLocation(S47Three::ToUE(P.At));C->SetWorldRotation(FRotator::ZeroRotator);W->AdoptNode(C,P.Parent,TEXT("motel-v2:")+P.Name);F->Parts.Add(C);
  const int32 Slot=P.Name.Contains(TEXT("GlowRoom06"))?6:P.Name.Contains(TEXT("GlowOffice"))?0:P.Name.Contains(TEXT("GlowSign"))?8:INDEX_NONE;
  if(Slot!=INDEX_NONE)for(int32 K=0;K<C->GetNumMaterials();++K)if(auto* Mid=C->CreateDynamicMaterialInstance(K))
   F->Glows.Add({Mid,Mid->K2_GetVectorParameterValue(TEXT("EmissiveFactor")),Slot,Slot==6?4.f:Slot==0?6.f:9.f});
 }
 // Copy only the dedicated pane, sharing the original dynamic material so gameplay still switches it.
 {
  auto* C=NewObject<UStaticMeshComponent>(W);C->SetupAttachment(W->Node(663)->Comp.Get());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(RoomWindow->GetStaticMesh());
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->RegisterComponent();C->SetWorldTransform(RoomWindow->GetComponentTransform());
  C->SetMaterial(0,W->SharedMid(335));W->AdoptNode(C,663,TEXT("motel-v2:live-room6-window"));F->Parts.Add(C);
 }
 for(const FPatch& P:Patches)
 {
  if(P.Hidden){Retired.AddUnique(P.Comp);continue;}
  TArray<UMaterialInterface*> Mats;for(auto* M:P.Comp->GetMaterials())Mats.Add(M);P.Comp->SetStaticMesh(P.Mesh);
  for(int32 S=0;S<P.Slots.Num();++S)P.Comp->SetMaterial(S,Mats[P.Slots[S]]);
 }
 for(auto* C:Retired){Retire(C);F->Retired.Add(C);}
 auto& Colliders=W->CollidersMutable(TEXT("saro"));
 for(int32 I=0;I<Colliders.Num();++I)for(const auto& R:Disable)if(Same(Colliders[I],R))
 {
  // SARO driving copies rectangle coordinates even when bEnabled=false. Preserve indices but remove both collisions.
  Colliders[I].bEnabled=false;Colliders[I].MinX=Colliders[I].MinZ=1.e9;Colliders[I].MaxX=Colliders[I].MaxZ=1.e9+.01;F->DisabledColliders.AddUnique(I);break;
 }
 for(const FCar& Car:Cars)
 {
  const FVector Old=W->WorldThree(Car.Node),Delta=Car.Target-Old;const auto* N=W->Node(Car.Node);W->SetPosition(Car.Node,N->P+Delta);
  for(auto& R:Colliders)if(R.Id==TEXT("prop:")+Car.Id){const FS47Rect Before=R;R.MinX+=Delta.X;R.MaxX+=Delta.X;R.MinZ+=Delta.Z;R.MaxZ+=Delta.Z;CarRects.Add({Before,R});}
  ++F->CarCount;
 }
 for(auto R:Add){R.Id=FString::Printf(TEXT("motel-v2:%d"),F->AddedColliders++);Colliders.Add(R);}
 for(auto R:Zones){R.Id=TEXT("motel-v2:walk");W->ZonesMutable(TEXT("saro")).Add(R);}
 // Drive metadata can replace the initial collider copy; compose the same exact changes into that list.
 const auto D=W->DriveData();const TSharedPtr<FJsonObject> *Legs=nullptr,*Saro=nullptr;const FValues* Obstacles=nullptr;
 if(D&&D->TryGetObjectField(TEXT("legs"),Legs)&&(*Legs)->TryGetObjectField(TEXT("saro"),Saro)&&(*Saro)->TryGetArrayField(TEXT("obstacles"),Obstacles))
 {
  FValues Out;for(const auto& V:*Obstacles)
  {
   const auto& A=V->AsArray();bool Remove=false,Moved=false;
   if(ValidRect(A))
   {
    const FS47Rect Old=ParseMotelRect(A);for(const auto& R:Disable)Remove|=Same(Old,R);
    if(!Remove)for(const auto& Pair:CarRects)if(Same(Old,Pair.Key)){Out.Add(MakeShared<FJsonValueArray>(RectValues(Pair.Value)));Moved=true;break;}
   }
   if(!Remove&&!Moved)Out.Add(V);
  }
  for(const auto& R:Add)Out.Add(MakeShared<FJsonValueArray>(RectValues(R)));(*Saro)->SetArrayField(TEXT("obstacles"),Out);
 }
 Floors=MoveTemp(NewFloors);F->PatchCount=Patches.Num();F->bInitialAnchorsPreserved=true;
 for(const auto& Pair:Protected)F->bInitialAnchorsPreserved&=W->WorldThree(Pair.Key).Equals(Pair.Value,.0001);
 F->Update();TArray<FString> Checks;const bool Pass=F->Audit(Checks);
 for(const auto& Line:Checks)UE_LOG(LogS47MotelV2,Display,TEXT("S47: %s"),*Line);
 UE_LOG(LogS47MotelV2,Display,TEXT("S47: motel V2 installed; audit=%s; existing KELVORA, story anchors and three original cars retained"),Pass?TEXT("PASS"):TEXT("FAIL"));
 return F;
}
void FS47MotelV2::Update()
{
 for(const auto& P:Retired)if(auto* C=P.Get())Retire(C);
 AS47Web* W=Web.Get();if(!W)return;const int32 Set=W->FloodSet(TEXT("court"));
 for(const auto& G:Glows)if(auto* Mid=G.Mid.Get())Mid->SetVectorParameterValue(TEXT("EmissiveFactor"),G.Base*FMath::Clamp(W->FloodPos(Set,G.Slot).A/G.Reference,0.f,1.f));
}
bool FS47MotelV2::Audit(TArray<FString>& Lines) const
{
 AS47Web* W=Web.Get();bool Ok=W&&ExpectedPartCount>1&&Parts.Num()==ExpectedPartCount&&CarCount==3&&ProtectedComponents.Num()==8&&bInitialAnchorsPreserved;
 for(const auto& P:Parts)Ok&=P.IsValid()&&P->GetStaticMesh()&&Tris(P->GetStaticMesh())>0;
 for(const auto& P:Retired)Ok&=P.IsValid()&&!P->IsVisible()&&P->bHiddenInGame&&P->GetCollisionEnabled()==ECollisionEnabled::NoCollision;
 if(W){for(const auto& P:ProtectedComponents)Ok&=P.Value.IsValid()&&W->Node(P.Key)&&W->Node(P.Key)->Comp==P.Value;const auto& Cs=W->CollidersMutable(TEXT("saro"));for(int32 I:DisabledColliders)Ok&=Cs.IsValidIndex(I)&&!Cs[I].bEnabled&&Cs[I].MinX>=1.e9;}
 Lines.Add(FString::Printf(TEXT("motel V2 %s parts=%d/%d patches=%d retired=%d cars=%d/3 protected=%d/8 disabledColliders=%d addedColliders=%d floors=%d"),Ok?TEXT("PASS"):TEXT("FAIL"),Parts.Num(),ExpectedPartCount,PatchCount,Retired.Num(),CarCount,ProtectedComponents.Num(),DisabledColliders.Num(),AddedColliders,Floors.Num()));return Ok;
}
double FS47MotelV2::FloorAt(double X,double Z)
{
 for(const auto& F:Floors)if(X>=F.MinX&&X<=F.MaxX&&Z>=F.MinZ&&Z<=F.MaxZ)return F.Y;
 return std::numeric_limits<double>::quiet_NaN();
}

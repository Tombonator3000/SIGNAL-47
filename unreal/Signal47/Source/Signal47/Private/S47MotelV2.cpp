// Unreal-only replacement of the motel presentation. Imported assets never own story interactions.
#include "S47MotelV2.h"
#include "S47Web.h"
#include "S47Glow.h"
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
TWeakObjectPtr<UStaticMeshComponent> FS47MotelV2::PhotoSignComponent;
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
 Floors.Reset();PhotoSignComponent.Reset();
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
 TArray<int32> RetiredRoots;TArray<FMovedInteraction> Moves;
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
 // Two dedicated legacy material leaves were outside the mixed structural trim recipes.
 // court/paint contains the box-car body and obsolete parking stripes; V2 owns the new markings.
 for(const FIntVector& Leaf:{FIntVector(673,458,322),FIntVector(688,473,337)})
 {
  const auto* N=W->Node(Leaf.X);auto* C=MeshAt(W,Leaf.X);
  if(!N||!C||N->Parent!=663||N->Geo!=Leaf.Y||N->Mats.Num()!=1||N->Mats[0]!=Leaf.Z){Reject(TEXT("legacy car paint / pool water predecessor"));return {};}
  Retired.AddUnique(C);
 }
 struct FSignGlow {int32 Node,Point;FVector Position;float Size;};
 const FSignGlow SignGlows[]={
  {36,23,{-32.9,3.7014316154,680.2},12},{36,24,{-31.5,1.9014316154,682},5},
  {1125,53,{-8.9,3.7014316154,380.2},12},{1125,54,{-7.5,1.9014316154,382},5},
  {1312,165,{-8.9,3.7014316154,380.2},12},{1312,166,{-7.5,1.9014316154,382},5}};
 for(const auto& G:SignGlows)
 {
  const auto* P=W->Glow(G.Node);
  if(!P||!P->Pos.IsValidIndex(G.Point)||!P->Size.IsValidIndex(G.Point)||!P->Pos[G.Point].Equals(G.Position,.002)||!FMath::IsNearlyEqual(P->Size[G.Point],G.Size,.001f))
  {Reject(FString::Printf(TEXT("legacy sign glow node%d point%d"),G.Node,G.Point));return {};}
 }
 struct FSignPool {const TCHAR* Key;int32 Slot;FVector Position;};
 const FSignPool SignPools[]={
  {TEXT("site"),10,{-31.5,3.9014316154,682}},{TEXT("site"),11,{-31.5,1.9014316154,682}},
  {TEXT("court"),8,{-33.4,2.5014316154,680}},{TEXT("court"),9,{-32.5,1.7014316154,680}}};
 for(const auto& P:SignPools)
 {
  const int32 Set=W->FloodSet(P.Key);const FLinearColor At=W->FloodPos(Set,P.Slot);
  if(Set<0||!FVector(At.R,At.G,At.B).Equals(P.Position,.002)){Reject(TEXT("legacy sign flood predecessor"));return {};}
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
 int32 PhotoSignParts=0;for(const auto& P:NewParts)if(P.Name==TEXT("SM_Sierra_GlowSign"))++PhotoSignParts;
 if(PhotoSignParts!=1){Reject(TEXT("single authored sign face for existing photo subject"));return {};}
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
 if(J->TryGetArrayField(TEXT("retire_named_nodes"),Rows))for(const auto& V:*Rows)
 {
  const auto R=V->AsObject();double I,P;FString Name;
  if(!R||!R->TryGetNumberField(TEXT("node"),I)||!R->TryGetNumberField(TEXT("parent"),P)||!R->TryGetStringField(TEXT("name"),Name)) {Reject(TEXT("named retirement metadata"));return {};}
  const auto* N=W->Node(int32(I));
  // Only the measured active reception lamp is superseded. Keep its asset and credit.
  if(int32(I)!=737||int32(P)!=689||Name!=TEXT("lic:lamp_marble")||!N||N->Parent!=int32(P)||N->Name!=Name||Protected.Contains(int32(I))) {Reject(TEXT("named retirement predecessor"));return {};}
  RetiredRoots.Add(int32(I));TArray<int32> Pending=N->Children;int32 Meshes=0;
  while(!Pending.IsEmpty()) {const int32 K=Pending.Pop(EAllowShrinking::No);const auto* Child=W->Node(K);if(!Child){Reject(TEXT("named retirement child"));return {};}Pending.Append(Child->Children);if(auto* C=MeshAt(W,K)){Retired.AddUnique(C);++Meshes;}}
  if(Meshes!=2){Reject(TEXT("reception lamp must contain exactly two original leaves"));return {};}
 }
 if(J->TryGetArrayField(TEXT("move_interaction_nodes"),Rows))for(const auto& V:*Rows)
 {
  const auto R=V->AsObject();double I,G,P;FString Handle;const FValues *Expected=nullptr,*Target=nullptr,*Scale=nullptr;
  if(!R||!R->TryGetNumberField(TEXT("node"),I)||!R->TryGetNumberField(TEXT("geo"),G)||!R->TryGetNumberField(TEXT("parent"),P)||!R->TryGetStringField(TEXT("handle"),Handle)||!R->TryGetArrayField(TEXT("expected_world_three_m"),Expected)||!VectorValid(*Expected,3)||!R->TryGetArrayField(TEXT("world_three_m"),Target)||!VectorValid(*Target,3)||!R->TryGetArrayField(TEXT("scale"),Scale)||!VectorValid(*Scale,3)) {Reject(TEXT("interaction relocation metadata"));return {};}
  const auto* N=W->Node(int32(I));const FVector Size=Vec(*Scale);
  if(Handle!=TEXT("world.motel.proxies.iceMachine")||W->Handle(Handle)!=int32(I)||!N||N->Geo!=int32(G)||N->Parent!=int32(P)||!W->WorldThree(int32(I)).Equals(Vec(*Expected),.01)||!N->S.Equals(FVector::OneVector,.0001)||Size.GetMin()<=0||Protected.Contains(int32(I))) {Reject(TEXT("interaction relocation predecessor"));return {};}
  Moves.Add({int32(I),Vec(*Target),Size});
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
  Cars.Add({I,Vec(*A),Id});
 }
 // All validation ends before the first mutation. A missing cooked asset leaves the full predecessor visible.
 auto F=MakeShared<FS47MotelV2>();F->Web=W;F->ProtectedPositions=Protected;F->ExpectedPartCount=NewParts.Num()+1;
 F->RetiredNamedRoots=RetiredRoots;F->MovedInteractions=Moves;F->PayphoneNode=FindChild(W,Motel,TEXT("prop:motelPhone"));
 for(const auto& Pair:Protected)F->ProtectedComponents.Add(Pair.Key,W->Node(Pair.Key)->Comp);
 for(const auto& G:SignGlows)F->RetiredSignGlows.Add(FIntPoint(G.Node,G.Point));
 for(const auto& P:SignPools){const int32 Set=W->FloodSet(P.Key);F->RetiredSignPools.Add(FIntPoint(Set,P.Slot));W->SuppressFloodLight(Set,P.Slot);}
 for(const FPart& P:NewParts)
 {
  auto* C=NewObject<UStaticMeshComponent>(W);C->SetupAttachment(W->Node(P.Parent)->Comp.Get());C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(P.Mesh);
  const bool Glass=P.Name.Contains(TEXT("Glass"))||P.Name.Contains(TEXT("Water"));C->SetCollisionEnabled(Glass?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryOnly);
  C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,Glass?ECR_Ignore:ECR_Block);C->SetCastShadow(!Glass);C->RegisterComponent();
  C->SetWorldLocation(S47Three::ToUE(P.At));C->SetWorldRotation(FRotator::ZeroRotator);W->AdoptNode(C,P.Parent,TEXT("motel-v2:")+P.Name);F->Parts.Add(C);
  if(P.Name==TEXT("SM_Sierra_GlowSign"))PhotoSignComponent=C;
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
 for(const int32 I:RetiredRoots)W->SetVisible(I,false);
 for(const auto& Move:Moves){const auto* N=W->Node(Move.Node);W->SetPosition(Move.Node,Move.Position-W->WorldThree(N->Parent));W->SetScale(Move.Node,Move.Scale);}
 auto& Colliders=W->CollidersMutable(TEXT("saro"));
 for(int32 I=0;I<Colliders.Num();++I)for(const auto& R:Disable)if(Same(Colliders[I],R))
 {
  // SARO driving copies rectangle coordinates even when bEnabled=false. Preserve indices but remove both collisions.
  const FString StableId=FString::Printf(TEXT("motel-v2:retired-collider:%d"),I);
  Colliders[I].Id=StableId;Colliders[I].bEnabled=false;Colliders[I].MinX=Colliders[I].MinZ=1.e9;Colliders[I].MaxX=Colliders[I].MaxZ=1.e9+.01;
  F->DisabledColliderIds.AddUnique(StableId);break;
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
 for(const int32 I:RetiredNamedRoots)W->SetVisible(I,false);
 // Retired sign halos are mirrored in SARO, the road and STATION01. Keep only these six points dark.
 for(const auto& P:RetiredSignGlows)if(auto* G=W->Glow(P.X)){G->SetSize(P.Y,0);G->SetColor(P.Y,FLinearColor::Black);}
 // Keep the historical flicker width as the new sign's emission feed, but remove the old coloured light.
 for(const auto& P:RetiredSignPools)if(!W->FloodCol(P.X,P.Y).Equals(FLinearColor::Black))W->SetFloodColor(P.X,P.Y,FLinearColor::Black);
 for(const auto& G:Glows)if(auto* Mid=G.Mid.Get())Mid->SetVectorParameterValue(TEXT("EmissiveFactor"),G.Base*FMath::Clamp(W->FloodPos(Set,G.Slot).A/G.Reference,0.f,1.f));
}
bool FS47MotelV2::Audit(TArray<FString>& Lines) const
{
 AS47Web* W=Web.Get();bool Ok=true;
 auto Check=[&](bool Pass,const FString& Detail){if(!Pass){Ok=false;Lines.Add(TEXT("motel V2 audit FAIL ")+Detail);}};
 Check(W!=nullptr,TEXT("world reference expired"));
 Check(ExpectedPartCount>1&&Parts.Num()==ExpectedPartCount,FString::Printf(TEXT("part count actual=%d expected=%d"),Parts.Num(),ExpectedPartCount));
 Check(CarCount==3,FString::Printf(TEXT("original car count=%d/3"),CarCount));
 Check(ProtectedComponents.Num()==8,FString::Printf(TEXT("protected component count=%d/8"),ProtectedComponents.Num()));
 Check(bInitialAnchorsPreserved,TEXT("protected anchors changed during installation"));
 for(int32 I=0;I<Parts.Num();++I)
 {
  auto* C=Parts[I].Get();UStaticMesh* Mesh=C?C->GetStaticMesh():nullptr;const int32 Count=Tris(Mesh);
  Check(C&&Mesh&&Count>0,FString::Printf(TEXT("part[%d] component=%s mesh=%s lod0Tris=%d lods=%d"),I,*GetNameSafe(C),*GetNameSafe(Mesh),Count,Mesh?Mesh->GetNumLODs():-1));
 }
 for(int32 I=0;I<Retired.Num();++I)
 {
  auto* C=Retired[I].Get();
  Check(C&&!C->IsVisible()&&C->bHiddenInGame&&C->GetCollisionEnabled()==ECollisionEnabled::NoCollision,
   FString::Printf(TEXT("retired[%d] component=%s mesh=%s visible=%d hidden=%d collision=%d"),I,*GetNameSafe(C),*GetNameSafe(C?C->GetStaticMesh():nullptr),C?C->IsVisible():-1,C?int32(C->bHiddenInGame):-1,C?int32(C->GetCollisionEnabled()):-1));
 }
 if(W)
 {
  for(const int32 I:RetiredNamedRoots)Check(!W->IsVisible(I),FString::Printf(TEXT("superseded reception lamp root%d remains visible"),I));
  for(const auto& Move:MovedInteractions)
  {
   const auto* N=W->Node(Move.Node);
   Check(N&&W->WorldThree(Move.Node).Equals(Move.Position,.001)&&N->S.Equals(Move.Scale,.0001),FString::Printf(TEXT("relocated interaction node%d no longer matches native front"),Move.Node));
  }
  const auto* Phone=MeshAt(W,PayphoneNode);
  Check(Phone&&Phone->GetStaticMesh()&&Phone->GetStaticMesh()->GetName()==TEXT("payphone_blue"),TEXT("original payphone asset missing"));
  if(Phone)
  {
   // AdoptNode records a prop's position only. Measure the actual rendered transform,
   // whose imported glTF axes use the Three-to-Unreal swap, rather than its identity node Q.
   Check(S47Three::ToThree(Phone->GetComponentLocation()).Equals(FVector(-43.25,-2.0985683853163195,671.32),.01),TEXT("payphone base differs from measured path height"));
   const FVector KeypadFront=Phone->GetComponentQuat().RotateVector(S47Three::DirToUE(FVector(0,0,1)));
   Check(KeypadFront.Equals(S47Three::DirToUE(FVector(0,0,-1)),.001),TEXT("payphone keypad does not face north path"));
  }
  for(const auto& Pair:ProtectedComponents)
  {
   const auto* N=W->Node(Pair.Key);auto* Expected=Pair.Value.Get();auto* Actual=N?N->Comp.Get():nullptr;
   Check(Expected&&Actual==Expected,FString::Printf(TEXT("protected node=%d expected=%s actual=%s expectedValid=%d"),Pair.Key,*GetNameSafe(Expected),*GetNameSafe(Actual),Pair.Value.IsValid()));
  }
  const auto& Cs=W->CollidersMutable(TEXT("saro"));
  for(const FString& Id:DisabledColliderIds)
  {
   // SetDoorOpen removes/appends leaves in this array. A construction-time index is not an identity.
   int32 Matches=0;
   for(int32 I=0;I<Cs.Num();++I)if(Cs[I].Id==Id)
   {
    ++Matches;const auto& C=Cs[I];
    Check(!C.bEnabled&&C.MinX>=1.e9&&C.MinZ>=1.e9,
     FString::Printf(TEXT("retired collider id=%s currentIndex=%d enabled=%d rect=[%.3f,%.3f,%.3f,%.3f]"),*Id,I,C.bEnabled,C.MinX,C.MaxX,C.MinZ,C.MaxZ));
   }
   Check(Matches==1,FString::Printf(TEXT("retired collider id=%s matches=%d expected=1 sharedArraySize=%d"),*Id,Matches,Cs.Num()));
  }
 }
 if(W)
 {
  Check(RetiredSignGlows.Num()==6&&RetiredSignPools.Num()==4,TEXT("legacy sign light retirement count"));
  for(const auto& P:RetiredSignGlows)
  {
   const auto* G=W->Glow(P.X);
   Check(G&&G->Size.IsValidIndex(P.Y)&&G->Col.IsValidIndex(P.Y)&&FMath::IsNearlyZero(G->Size[P.Y])&&G->Col[P.Y].Equals(FLinearColor::Black),FString::Printf(TEXT("legacy sign glow remains node%d point%d"),P.X,P.Y));
  }
  for(const auto& P:RetiredSignPools)Check(W->FloodCol(P.X,P.Y).Equals(FLinearColor::Black),FString::Printf(TEXT("legacy sign flood remains set%d slot%d"),P.X,P.Y));
 }
 const auto PhotoTarget=SignPhotoTarget();
 Check(PhotoTarget.IsSet(),TEXT("authored sign photo target unavailable"));
 if(PhotoTarget.IsSet())Check(PhotoTarget->Equals(FVector(-41.2320000172,2.1300242489,669.1897799015),.01),TEXT("authored sign photo target differs from verified sign-face bounds"));
 const auto Landscape=LandscapeFootprints();
 auto Cleared=[&Landscape](double X,double Z){for(const auto& B:Landscape)if(X>=B.X&&X<=B.Y&&Z>=B.Z&&Z<=B.W)return true;return false;};
 // Integration boundary checks: the new paved/water surfaces are clear; neighbouring desert is untouched.
 Check(Landscape.Num()==Floors.Num()+1&&!Floors.IsEmpty(),TEXT("landscape exclusion count does not match installed floors plus water"));
 for(const FVector2D& P:{FVector2D(-63,674),FVector2D(-58.9,674),FVector2D(-53.2,680.6),FVector2D(-47,670.7)})
  Check(Cleared(P.X,P.Y),FString::Printf(TEXT("landscape footprint misses built surface %.2f %.2f"),P.X,P.Y));
 for(const FVector2D& P:{FVector2D(-65.2,674),FVector2D(-59,665.6),FVector2D(-52.2,680.6),FVector2D(-47,669.5)})
  Check(!Cleared(P.X,P.Y),FString::Printf(TEXT("landscape footprint clears neighbouring desert %.2f %.2f"),P.X,P.Y));
 Lines.Add(FString::Printf(TEXT("motel V2 %s parts=%d/%d patches=%d retired=%d cars=%d/3 protected=%d/8 disabledColliders=%d addedColliders=%d floors=%d"),Ok?TEXT("PASS"):TEXT("FAIL"),Parts.Num(),ExpectedPartCount,PatchCount,Retired.Num(),CarCount,ProtectedComponents.Num(),DisabledColliderIds.Num(),AddedColliders,Floors.Num()));return Ok;
}
double FS47MotelV2::FloorAt(double X,double Z)
{
 for(const auto& F:Floors)if(X>=F.MinX&&X<=F.MaxX&&Z>=F.MinZ&&Z<=F.MaxZ)return F.Y;
 return std::numeric_limits<double>::quiet_NaN();
}

TArray<FVector4> FS47MotelV2::LandscapeFootprints()
{
 TArray<FVector4> Out;
 if(Floors.IsEmpty())return Out; // Missing assets / explicit opt-out preserve the original desert.
 for(const auto& F:Floors)Out.Add(FVector4(F.MinX,F.MaxX,F.MinZ,F.MaxZ));
 // Water deliberately has no walking floor, but must also reject grass, scrub and stones.
 Out.Add(FVector4(-61,-56.8,669.8,678));
 return Out;
}

TOptional<FVector> FS47MotelV2::SignPhotoTarget()
{
 const auto* C=PhotoSignComponent.Get();const UStaticMesh* Mesh=C?C->GetStaticMesh():nullptr;
 if(!C||!Mesh)return {};
 // The illuminated lettering bounds exclude the long support posts; aim at the actual installed sign face.
 return S47Three::ToThree(C->GetComponentTransform().TransformPosition(Mesh->GetBounds().Origin));
}
